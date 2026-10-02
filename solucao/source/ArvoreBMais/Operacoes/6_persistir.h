#ifndef ARVB_PERSISTIR_H
#define ARVB_PERSISTIR_H

/* contexto usado para gravar cada par visitado por percorrerArv
   (definido em Operacoes/7_percorrer.h, chamado mais abaixo) num
   arquivo -- reaproveita o mesmo percurso ordenado usado por
   queryAll/queryBy na biblioteca Persistencia                    */
struct ContextoPersistirArv{
    FILE* arquivo;
    int   tamanhoChave;
    int   tamanhoValor;
};

void visitantePersistirArv(void* chave, void* valor, void* contexto){
    struct ContextoPersistirArv* ctx = (struct ContextoPersistirArv*) contexto;
    fwrite(chave, ctx->tamanhoChave, 1, ctx->arquivo);
    fwrite(valor, ctx->tamanhoValor, 1, ctx->arquivo);
}

/* percorre as folhas da esquerda para a direita (em ordem de chave)
   e grava cada par (chave, valor) sequencialmente, como bytes crus,
   num arquivo proprio                                              */
void persistirArvore(pArvB arv, char nomeArquivo[40]){

    FILE* f = fopen(nomeArquivo, "wb");
    if (f == NULL)
        return;

    struct ContextoPersistirArv ctx;
    ctx.arquivo      = f;
    ctx.tamanhoChave = arv->tamanhoChave;
    ctx.tamanhoValor = arv->tamanhoValor;

    percorrerArv(arv, visitantePersistirArv, &ctx);

    fclose(f);
}

/* --------------------------------------------------------- */
/* reconstroi a arvore reinserindo, um a um, os pares gravados
   por persistirArvore. Se o arquivo de indice ainda nao
   existir, devolve uma arvore vazia (quem chamou decide se
   precisa reconstruir o indice a partir dos dados originais) */
pArvB carregarArvore(char nomeArquivo[40], int ordem, int tamanhoChave,
                     int tamanhoValor, FuncaoComparacaoArv pfc){

    pArvB arv = criarArvore(ordem, tamanhoChave, tamanhoValor);

    FILE* f = fopen(nomeArquivo, "rb");
    if (f == NULL)
        return arv;

    void* chave = malloc(tamanhoChave);
    void* valor = malloc(tamanhoValor);

    while (fread(chave, tamanhoChave, 1, f) == 1 &&
           fread(valor, tamanhoValor, 1, f) == 1){
        inserirArv(arv, chave, valor, pfc);
    }

    free(chave);
    free(valor);
    fclose(f);

    return arv;
}

#endif
