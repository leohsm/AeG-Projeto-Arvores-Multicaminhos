#define main mainBenchmarkFornecido
#include "../solucao/source/Benchmark/bench.c"
#undef main
#include <assert.h>
#include <limits.h>

#define MAX_KEYS 4096
static int presente[MAX_KEYS];
static struct Pessoa modelo[MAX_KEYS];
static unsigned long checks = 0;
static void exigir(int cond){ checks++; if (!cond){ fprintf(stderr,"Falha no check %lu (ordem %d)\n", checks, ORDEM_INDICE); exit(1); } }
static void liberarLista(pDLista lista){
    pNoh p = lista->primeiro;
    while(p){ pNoh q=p->prox;free(p->info);free(p);p=q; }free(lista);
}
typedef struct {int min,max,prof,contagem;} Limites;
static Limites validarNoh(pArvB arv,pNohArv p,int raiz){
    exigir(p->n >= 0 && p->n < arv->ordem);
    if(!raiz) exigir(p->n >= ocupacaoMinima(arv));
    for(int i=1;i<p->n;i++) exigir(*(int*)p->chaves[i-1] < *(int*)p->chaves[i]);
    if(p->ehFolha){
        Limites l={p->n?*(int*)p->chaves[0]:INT_MAX,p->n?*(int*)p->chaves[p->n-1]:INT_MIN,0,p->n};return l;
    }
    exigir(p->n >= 1);
    Limites l=validarNoh(arv,p->filhos[0],0);
    for(int i=0;i<=p->n;i++) exigir(p->filhos[i]->pai==p);
    for(int i=1;i<=p->n;i++){
        Limites r=validarNoh(arv,p->filhos[i],0);
        int separador=*(int*)p->chaves[i-1];
        exigir(l.max < separador && separador <= r.min);
        exigir(l.prof==r.prof);l.max=r.max;l.contagem+=r.contagem;
    }
    l.prof++;return l;
}
static void verificar(pDFile a){
    int total=0;
    for(int k=0;k<MAX_KEYS;k++)total+=presente[k];
    exigir(a->indice->raiz->pai==NULL);
    Limites l=validarNoh(a->indice,a->indice->raiz,1);exigir(l.contagem==total);
    exigir(fflush(a->arquivo)==0);exigir(fseek(a->arquivo,0,SEEK_END)==0);
    exigir(ftell(a->arquivo)==(long)total*sizeof(struct Pessoa));
    pDLista all=queryAll(a);exigir(all->quantidade==total);
    int ultimo=-1,count=0;
    for(pNoh p=all->primeiro;p;p=p->prox){
        struct Pessoa* r=p->info;int k=r->cpf;
        exigir(k>=0&&k<MAX_KEYS&&presente[k]);exigir(k>ultimo);ultimo=k;
        exigir(memcmp(r,&modelo[k],sizeof(*r))==0);count++;
        long* off=buscarArv(a->indice,&k,comparaChaveCpf);exigir(off!=NULL);
        struct Pessoa disk;exigir(fseek(a->arquivo,*off,SEEK_SET)==0);exigir(fread(&disk,sizeof(disk),1,a->arquivo)==1);
        exigir(memcmp(&disk,r,sizeof(disk))==0);free(off);
    }
    exigir(count==total);liberarLista(all);
    all=queryAllSequencial(a);exigir(all->quantidade==total);
    int vistos[MAX_KEYS]={0};
    for(pNoh p=all->primeiro;p;p=p->prox){
        struct Pessoa* r=p->info;int k=r->cpf;
        exigir(k>=0&&k<MAX_KEYS&&presente[k]&&!vistos[k]);vistos[k]=1;
        exigir(memcmp(r,&modelo[k],sizeof(*r))==0);
    }
    liberarLista(all);
    for(int k=0;k<MAX_KEYS;k++)if(!presente[k]){
        void* r=retrieve(a,&k,comparaChavePessoa);exigir(r==NULL);free(r);
    }
}
static void inserir(pDFile a,int k,int idade){
    memset(&modelo[k],0,sizeof(modelo[k]));modelo[k].cpf=k;modelo[k].idade=idade;
    sprintf(modelo[k].nome,"Registro-%d-%d",k,idade);createe(a,&modelo[k]);presente[k]=1;
}
static void remover(pDFile a,int k){
    unsigned long long nc=nosCriadosArv,s=splitsFolhaTotal+splitsInternoTotal;
    pArvB indice=a->indice;int existia=presente[k];deletee(a,&k,comparaChavePessoa);
    exigir(existia?errno==0:errno==ENOENT);presente[k]=0;
    exigir(a->indice==indice);exigir(nosCriadosArv==nc);
    exigir(splitsFolhaTotal+splitsInternoTotal==s);
}
int main(void){
    srand(1927);remove("test.dat");remove("test.dat.idx");
    pDFile a=abrir("test.dat",sizeof(struct Pessoa),sizeof(int),extraiChaveCpf,comparaChaveCpf);
    verificar(a);remover(a,0);inserir(a,0,10);remover(a,0);verificar(a);
    inserir(a,1,11);inserir(a,2,12);inserir(a,3,13);
    remover(a,3);verificar(a);remover(a,1);verificar(a);remover(a,2);verificar(a);
    int n=3000;int* seq=malloc(n*sizeof(int));for(int i=0;i<n;i++)seq[i]=i;embaralhar(seq,n);
    for(int i=0;i<n;i++)inserir(a,seq[i],seq[i]%90);verificar(a);
    embaralhar(seq,n);
    for(int i=0;i<n;i++){
        remover(a,seq[i]);if(i%37==0)verificar(a);
        if(i==1500){fechar(a);a=abrir("test.dat",sizeof(struct Pessoa),sizeof(int),extraiChaveCpf,comparaChaveCpf);verificar(a);}
    }
    verificar(a);exigir(a->indice->raiz->ehFolha&&a->indice->raiz->n==0);free(seq);
    for(int i=0;i<5000;i++){
        int k=rand()%512;
        if(rand()%3==0)remover(a,k);
        else if(!presente[k])inserir(a,k,i%100);
        else{modelo[k].idade=i%100;update(a,&k,&modelo[k],comparaChavePessoa);}
        if(i%97==0)verificar(a);
        if(i%1000==999){fechar(a);a=abrir("test.dat",sizeof(struct Pessoa),sizeof(int),extraiChaveCpf,comparaChaveCpf);verificar(a);}
    }
    verificar(a);
    // Atualização de chave única, seguida de exclusão e reabertura.
    if(!presente[0])inserir(a,0,20);
    struct Pessoa novo=modelo[0];novo.cpf=4000;int antiga=0;
    update(a,&antiga,&novo,comparaChavePessoa);presente[0]=0;presente[4000]=1;modelo[4000]=novo;verificar(a);
    remover(a,4000);verificar(a);fechar(a);
    // Reconstrução na abertura quando .idx está ausente também preserva offsets.
    remove("test.dat.idx");a=abrir("test.dat",sizeof(struct Pessoa),sizeof(int),extraiChaveCpf,comparaChaveCpf);verificar(a);fechar(a);
    printf("OK ordem=%d checks=%lu (CRUD, offsets, balanceamento, fusoes, reabertura)\n",ORDEM_INDICE,checks);
    return 0;
}
