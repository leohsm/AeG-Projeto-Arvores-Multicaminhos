/* ---------------------------------------------------------------
   Benchmark: Persistencia COM indice (arvore B+) vs SEM indice
   (varredura linear pura, reimplementando fielmente o comportamento
   original da biblioteca antes do indice existir).

   Mede, para varios tamanhos de arquivo (N registros), o tempo de:
     - createe (insercao de N registros)
     - retrieve (R buscas por chave)
     - update   (R atualizacoes por chave)
     - deletee  (D remocoes por chave)
     - queryAll (listar tudo)
     - queryBy  (filtrar por predicado sobre o registro inteiro)

   Resultado eh salvo em CSV: operacao,modo,N,tempo_ms
   --------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Persistencia.h"

/* ------------------------------------------------------------- */
struct Pessoa{
    int  cpf;
    char nome[30];
    int  idade;
};

int comparaChavePessoa(void *info1, void *info2){
    int *chave = (int *) info1;
    struct Pessoa *p = (struct Pessoa*) info2;
    return *chave - p->cpf;
}

void* extraiChaveCpf(void* dados){
    return &(((struct Pessoa*)dados)->cpf);
}

int comparaChaveCpf(void* k1, void* k2){
    return *(int*)k1 - *(int*)k2;
}

int maiorDeIdade(void* info){
    struct Pessoa *p = (struct Pessoa*) info;
    return (p->idade >= 18) ? 1 : 0;
}

/* ------------------------------------------------------------- *
 *  Reimplementacao SEM indice (varredura linear pura) -- eh
 *  exatamente o que createe/retrieve/update/deletee/queryAll/
 *  queryBy faziam antes de existir a arvore B+ como indice.
 * ------------------------------------------------------------- */
typedef struct {
    FILE* arquivo;
    char  nomeArquivo[64];
    int   tamanhoRegistro;
} ArqSI;

ArqSI* abrirSI(const char* nome, int tamanho){
    ArqSI* a = (ArqSI*) malloc(sizeof(ArqSI));
    strcpy(a->nomeArquivo, nome);
    a->arquivo = fopen(nome, "r+b");
    if (a->arquivo == NULL)
        a->arquivo = fopen(nome, "w+b");
    a->tamanhoRegistro = tamanho;
    return a;
}

void createeSI(ArqSI* a, void* dados){
    fseek(a->arquivo, 0, SEEK_END);
    fwrite(dados, a->tamanhoRegistro, 1, a->arquivo);
}

void* retrieveSI(ArqSI* a, void* chave, FuncaoComparacao pfc){
    void* reg = malloc(a->tamanhoRegistro);
    rewind(a->arquivo);
    while (fread(reg, a->tamanhoRegistro, 1, a->arquivo) == 1){
        if (pfc(chave, reg) == 0)
            return reg;
    }
    free(reg);
    return NULL;
}

void updateSI(ArqSI* a, void* chave, void* dados, FuncaoComparacao pfc){
    void* reg = retrieveSI(a, chave, pfc);
    if (reg != NULL){
        fseek(a->arquivo, -a->tamanhoRegistro, SEEK_CUR);
        fwrite(dados, a->tamanhoRegistro, 1, a->arquivo);
        free(reg);
    }
}

void deleteeSI(ArqSI* a, void* chave, FuncaoComparacao pfc){
    rewind(a->arquivo);

    int capacidade = 1024, n = 0, achou = 0;
    void* buffer = malloc((size_t)a->tamanhoRegistro * capacidade);
    void* reg = malloc(a->tamanhoRegistro);

    while (fread(reg, a->tamanhoRegistro, 1, a->arquivo) == 1){
        if (pfc(chave, reg) == 0){
            achou = 1;
        } else {
            if (n == capacidade){
                capacidade *= 2;
                buffer = realloc(buffer, (size_t)a->tamanhoRegistro * capacidade);
            }
            memcpy((char*)buffer + (size_t)n * a->tamanhoRegistro, reg, a->tamanhoRegistro);
            n++;
        }
    }
    free(reg);

    if (achou){
        fclose(a->arquivo);
        a->arquivo = fopen(a->nomeArquivo, "w+b");
        fwrite(buffer, a->tamanhoRegistro, n, a->arquivo);
    }
    free(buffer);
}

void* queryAllSI(ArqSI* a, int* nOut){
    rewind(a->arquivo);
    int capacidade = 1024, n = 0;
    void* buffer = malloc((size_t)a->tamanhoRegistro * capacidade);
    void* reg = malloc(a->tamanhoRegistro);

    while (fread(reg, a->tamanhoRegistro, 1, a->arquivo) == 1){
        if (n == capacidade){
            capacidade *= 2;
            buffer = realloc(buffer, (size_t)a->tamanhoRegistro * capacidade);
        }
        memcpy((char*)buffer + (size_t)n * a->tamanhoRegistro, reg, a->tamanhoRegistro);
        n++;
    }
    free(reg);
    *nOut = n;
    return buffer;
}

void* queryBySI(ArqSI* a, FuncaoPredicado pfp, int* nOut){
    rewind(a->arquivo);
    int capacidade = 1024, n = 0;
    void* buffer = malloc((size_t)a->tamanhoRegistro * capacidade);
    void* reg = malloc(a->tamanhoRegistro);

    while (fread(reg, a->tamanhoRegistro, 1, a->arquivo) == 1){
        if (pfp(reg) == 1){
            if (n == capacidade){
                capacidade *= 2;
                buffer = realloc(buffer, (size_t)a->tamanhoRegistro * capacidade);
            }
            memcpy((char*)buffer + (size_t)n * a->tamanhoRegistro, reg, a->tamanhoRegistro);
            n++;
        }
    }
    free(reg);
    *nOut = n;
    return buffer;
}

void fecharSI(ArqSI* a){
    fclose(a->arquivo);
    free(a);
}

/* ------------------------------------------------------------- */
double agora(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* embaralha um vetor de inteiros (Fisher-Yates) */
void embaralhar(int* v, int n){
    int i;
    for (i = n - 1; i > 0; i--){
        int j = rand() % (i + 1);
        int t = v[i]; v[i] = v[j]; v[j] = t;
    }
}

FILE* csv;

void registrar(const char* operacao, const char* modo, int n, double tempoMs){
    fprintf(csv, "%s,%s,%d,%.4f\n", operacao, modo, n, tempoMs);
    printf("  %-10s %-12s N=%-6d %8.3f ms\n", operacao, modo, n, tempoMs);
}

/* ------------------------------------------------------------- */
int main(){
    srand(12345);

    csv = fopen("resultados.csv", "w");
    fprintf(csv, "operacao,modo,N,tempo_ms\n");

    int tamanhosN[] = {1000, 5000, 20000, 50000, 100000};
    int qtdeN = sizeof(tamanhosN) / sizeof(int);

    int R = 200; /* buscas/atualizacoes por rodada */
    int D = 30;  /* remocoes por rodada */

    int in;
    for (in = 0; in < qtdeN; in++){
        int N = tamanhosN[in];
        printf("\n=== N = %d ===\n", N);

        /* ---- prepara as sequencias aleatorias (as MESMAS para os
           dois modos, para a comparacao ser justa) ---------------- */
        int* ordemInsercao = malloc(sizeof(int) * N);
        int i;
        for (i = 0; i < N; i++) ordemInsercao[i] = i;
        embaralhar(ordemInsercao, N);

        int* chavesConsulta = malloc(sizeof(int) * R);
        for (i = 0; i < R; i++) chavesConsulta[i] = rand() % N;

        int* chavesUpdate = malloc(sizeof(int) * R);
        for (i = 0; i < R; i++) chavesUpdate[i] = rand() % N;

        /* D chaves distintas para remover */
        int* chavesDelete = malloc(sizeof(int) * D);
        int* usadas = calloc(N, sizeof(int));
        int qtd = 0;
        while (qtd < D && qtd < N){
            int c = rand() % N;
            if (!usadas[c]){ usadas[c] = 1; chavesDelete[qtd++] = c; }
        }
        free(usadas);

        /* ============================================================
           MODO COM INDICE (biblioteca Persistencia real)
           ============================================================ */
        remove("bench_idx.dat");
        remove("bench_idx.dat.idx");

        pDFile arqIdx = abrir("bench_idx.dat", sizeof(struct Pessoa), sizeof(int),
                               extraiChaveCpf, comparaChaveCpf);

        double t0, t1;

        t0 = agora();
        for (i = 0; i < N; i++){
            struct Pessoa p;
            memset(&p, 0, sizeof(p));
            p.cpf = 1000 + ordemInsercao[i];
            sprintf(p.nome, "Pessoa%d", ordemInsercao[i]);
            p.idade = 10 + (ordemInsercao[i] % 60);
            createe(arqIdx, &p);
        }
        t1 = agora();
        registrar("createe", "com_indice", N, (t1 - t0) * 1000.0);

        t0 = agora();
        for (i = 0; i < R; i++){
            int cpf = 1000 + chavesConsulta[i];
            void* r = retrieve(arqIdx, &cpf, comparaChavePessoa);
            free(r);
        }
        t1 = agora();
        registrar("retrieve", "com_indice", N, (t1 - t0) * 1000.0);

        t0 = agora();
        for (i = 0; i < R; i++){
            struct Pessoa novo;
            memset(&novo, 0, sizeof(novo));
            novo.cpf = 1000 + chavesUpdate[i];
            sprintf(novo.nome, "Atualizado%d", chavesUpdate[i]);
            novo.idade = 50;
            int chaveBusca = novo.cpf;
            update(arqIdx, &chaveBusca, &novo, comparaChavePessoa);
        }
        t1 = agora();
        registrar("update", "com_indice", N, (t1 - t0) * 1000.0);

        t0 = agora();
        for (i = 0; i < D && i < N; i++){
            int cpf = 1000 + chavesDelete[i];
            deletee(arqIdx, &cpf, comparaChavePessoa);
        }
        t1 = agora();
        registrar("deletee", "com_indice", N, (t1 - t0) * 1000.0);

        int reps = 5, r;
        t0 = agora();
        for (r = 0; r < reps; r++){
            pDLista todos = queryAll(arqIdx);
            pNoh atual = todos->primeiro;
            while (atual != NULL){ pNoh prox = atual->prox; free(atual->info); free(atual); atual = prox; }
            free(todos);
        }
        t1 = agora();
        registrar("queryAll", "com_indice", N, ((t1 - t0) / reps) * 1000.0);

        t0 = agora();
        for (r = 0; r < reps; r++){
            pDLista maiores = queryBy(arqIdx, maiorDeIdade);
            pNoh atual = maiores->primeiro;
            while (atual != NULL){ pNoh prox = atual->prox; free(atual->info); free(atual); atual = prox; }
            free(maiores);
        }
        t1 = agora();
        registrar("queryBy", "com_indice", N, ((t1 - t0) / reps) * 1000.0);

        /* checagem de corretude (nao cronometrada): apos D remocoes,
           o total de registros deve ser exatamente N - D             */
        {
            pDLista checagem = queryAll(arqIdx);
            int esperado = N - D;
            if (checagem->quantidade != esperado){
                printf("  !! CORRETUDE (com indice): esperado %d registros, achou %d\n",
                       esperado, checagem->quantidade);
            } else {
                printf("  corretude (com indice) ok: %d registros\n", checagem->quantidade);
            }
            pNoh atual = checagem->primeiro;
            while (atual != NULL){ pNoh prox = atual->prox; free(atual->info); free(atual); atual = prox; }
            free(checagem);
        }

        fechar(arqIdx);

        /* ============================================================
           MODO SEM INDICE (varredura linear pura)
           ============================================================ */
        remove("bench_si.dat");

        ArqSI* arqSI = abrirSI("bench_si.dat", sizeof(struct Pessoa));

        t0 = agora();
        for (i = 0; i < N; i++){
            struct Pessoa p;
            memset(&p, 0, sizeof(p));
            p.cpf = 1000 + ordemInsercao[i];
            sprintf(p.nome, "Pessoa%d", ordemInsercao[i]);
            p.idade = 10 + (ordemInsercao[i] % 60);
            createeSI(arqSI, &p);
        }
        t1 = agora();
        registrar("createe", "sem_indice", N, (t1 - t0) * 1000.0);

        t0 = agora();
        for (i = 0; i < R; i++){
            int cpf = 1000 + chavesConsulta[i];
            void* r = retrieveSI(arqSI, &cpf, comparaChavePessoa);
            free(r);
        }
        t1 = agora();
        registrar("retrieve", "sem_indice", N, (t1 - t0) * 1000.0);

        t0 = agora();
        for (i = 0; i < R; i++){
            struct Pessoa novo;
            memset(&novo, 0, sizeof(novo));
            novo.cpf = 1000 + chavesUpdate[i];
            sprintf(novo.nome, "Atualizado%d", chavesUpdate[i]);
            novo.idade = 50;
            int chaveBusca = novo.cpf;
            updateSI(arqSI, &chaveBusca, &novo, comparaChavePessoa);
        }
        t1 = agora();
        registrar("update", "sem_indice", N, (t1 - t0) * 1000.0);

        t0 = agora();
        for (i = 0; i < D && i < N; i++){
            int cpf = 1000 + chavesDelete[i];
            deleteeSI(arqSI, &cpf, comparaChavePessoa);
        }
        t1 = agora();
        registrar("deletee", "sem_indice", N, (t1 - t0) * 1000.0);

        t0 = agora();
        for (r = 0; r < reps; r++){
            int nTodos;
            void* todos = queryAllSI(arqSI, &nTodos);
            free(todos);
        }
        t1 = agora();
        registrar("queryAll", "sem_indice", N, ((t1 - t0) / reps) * 1000.0);

        t0 = agora();
        for (r = 0; r < reps; r++){
            int nMaiores;
            void* maiores = queryBySI(arqSI, maiorDeIdade, &nMaiores);
            free(maiores);
        }
        t1 = agora();
        registrar("queryBy", "sem_indice", N, ((t1 - t0) / reps) * 1000.0);

        /* checagem de corretude (nao cronometrada) */
        {
            int nChecagem;
            void* checagem = queryAllSI(arqSI, &nChecagem);
            int esperado = N - D;
            if (nChecagem != esperado){
                printf("  !! CORRETUDE (sem indice): esperado %d registros, achou %d\n",
                       esperado, nChecagem);
            } else {
                printf("  corretude (sem indice) ok: %d registros\n", nChecagem);
            }
            free(checagem);
        }

        fecharSI(arqSI);

        free(ordemInsercao);
        free(chavesConsulta);
        free(chavesUpdate);
        free(chavesDelete);
    }

    fclose(csv);
    printf("\nResultados salvos em resultados.csv\n");
    return 0;
}
