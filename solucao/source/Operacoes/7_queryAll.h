#ifndef QUERYALL_H
#define QUERYALL_H

/* contexto usado para reconstruir cada registro completo a partir do
   offset guardado no indice, enquanto percorrerArv visita as chaves
   em ordem                                                          */
struct ContextoQueryAll{
    pDFile  arq;
    pDLista registros;
};

void visitanteQueryAll(void* chave, void* valor, void* contexto){

    struct ContextoQueryAll* ctx = (struct ContextoQueryAll*) contexto;
    long offset = *(long*) valor;

    void* registro = malloc(ctx->arq->tamanhoRegistro);
    fseek(ctx->arq->arquivo, offset, SEEK_SET);
    fread(registro, ctx->arq->tamanhoRegistro, 1, ctx->arq->arquivo);

    incluirInfo(ctx->registros, registro);
}

pDLista queryAll(pDFile arq){

   if (arq->arquivo == NULL){
       printf("Arquivo n�o foi aberto!");
       return NULL;
   }

   pDLista registros = criarLista();

   struct ContextoQueryAll ctx;
   ctx.arq       = arq;
   ctx.registros = registros;

   /* percorre o indice (arvore B+), em ordem de chave, buscando cada
      registro no arquivo pelo offset guardado -- em vez de varrer o
      arquivo de dados sequencialmente do inicio ao fim                */
   percorrerArv(arq->indice, visitanteQueryAll, &ctx);

   return registros;

}

/* Alternativa explícita: preserva pDLista, mas lê sequencialmente.
   Após swap, a ordem física difere da ordem de chave. */
pDLista queryAllSequencial(pDFile arq){
    if (!arq || !arq->arquivo) return NULL;
    pDLista registros = criarLista();
    rewind(arq->arquivo);
    for (;;){
        void* registro = malloc(arq->tamanhoRegistro);
        if (!registro) break;
        if (fread(registro, arq->tamanhoRegistro, 1, arq->arquivo) != 1){
            free(registro); break;
        }
        incluirInfo(registros, registro);
    }
    return registros;
}
#endif
