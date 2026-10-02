#define main mainBenchmarkFornecido
#include "../solucao/source/Benchmark/bench.c"
#undef main

static unsigned long long comparacoes=0;
static volatile unsigned long long checksum=0;
static int comparaContada(void* a,void* b){comparacoes++;return comparaChaveCpf(a,b);}
static void visitanteSomar(void* chave,void* valor,void* ctx){(void)ctx;checksum+=*(int*)chave+*(long*)valor;}
static void liberar(pDLista l){pNoh p=l->primeiro;while(p){pNoh q=p->prox;free(p->info);free(p);p=q;}free(l);}
static void medirListagem(FILE* out,pDFile a,const char* layout,int n){
    ArqSI si={a->arquivo,"",a->tamanhoRegistro};int reps=5,count;
    // Aquecimento: as quatro alternativas recebem o mesmo arquivo em cache.
    liberar(queryAll(a));liberar(queryAllSequencial(a));free(queryAllSI(&si,&count));
    for(int modo=0;modo<4;modo++){
        double t=agora();
        for(int i=0;i<reps;i++){
            if(modo==0){pDLista l=queryAll(a);if(l->quantidade!=n)exit(1);liberar(l);}
            if(modo==1){pDLista l=queryAllSequencial(a);if(l->quantidade!=n)exit(1);liberar(l);}
            if(modo==2){void* b=queryAllSI(&si,&count);if(count!=n)exit(1);free(b);}
            if(modo==3)percorrerArv(a->indice,visitanteSomar,NULL);
        }
        const char* nomes[]={"indice_lista","sequencial_lista","sequencial_buffer","somente_folhas"};
        fprintf(out,"queryAll,%s,%s,%d,%d,%d,%.6f,0\n",layout,nomes[modo],n,ORDEM_INDICE,reps,(agora()-t)*1000/reps);
    }
}
int main(int argc,char** argv){
    int n=100000,q=100000;srand(argc>1?(unsigned)strtoul(argv[1],NULL,10):12345);
    FILE* out=fopen("estudo.csv","w");fprintf(out,"operacao,layout,modo,N,ordem,operacoes,tempo_ms,comparacoes_por_busca\n");
    int* seq=malloc(n*sizeof(int));for(int i=0;i<n;i++)seq[i]=i;
    // Aleatório primeiro; reutilizar as mesmas consultas em cada ordem.
    for(int layout=0;layout<2;layout++){
        for(int i=0;i<n;i++)seq[i]=i;if(layout==0)embaralhar(seq,n);
        remove("estudo.dat");remove("estudo.dat.idx");
        pDFile a=abrir("estudo.dat",sizeof(struct Pessoa),sizeof(int),extraiChaveCpf,comparaChaveCpf);
        for(int i=0;i<n;i++){
            struct Pessoa p={0};p.cpf=1000+seq[i];p.idade=i%90;sprintf(p.nome,"Pessoa%d",seq[i]);createe(a,&p);
        }
        fflush(a->arquivo);
        medirListagem(out,a,layout?"ordenado":"aleatorio",n);
        if(layout==0){
            int* consultas=malloc(q*sizeof(int));for(int i=0;i<q;i++)consultas[i]=1000+rand()%n;
            comparacoes=0;double t=agora();
            for(int i=0;i<q;i++){long* off=buscarArv(a->indice,&consultas[i],comparaContada);if(!off)exit(1);checksum+=*off;free(off);}
            fprintf(out,"buscarArv,aleatorio,somente_indice,%d,%d,%d,%.6f,%.6f\n",n,ORDEM_INDICE,q,(agora()-t)*1000,(double)comparacoes/q);free(consultas);
        }
        fechar(a);
    }
    free(seq);fclose(out);printf("Estudo OK ordem=%d checksum=%llu\n",ORDEM_INDICE,checksum);return 0;
}
