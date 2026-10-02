#ifndef QUERY_H
#define QUERY_H

/* contexto usado para reconstruir cada registro (via offset do
   indice) e aplicar o predicado, enquanto percorrerArv visita as
   chaves em ordem                                                 */
struct ContextoQueryBy{
    pDFile           arq;
    FuncaoPredicado  pfp;
    pDLista          registros;
};

void visitanteQueryBy(void* chave, void* valor, void* contexto){

    struct ContextoQueryBy* ctx = (struct ContextoQueryBy*) contexto;
    long offset = *(long*) valor;

    void* registro = malloc(ctx->arq->tamanhoRegistro);
    fseek(ctx->arq->arquivo, offset, SEEK_SET);
    fread(registro, ctx->arq->tamanhoRegistro, 1, ctx->arq->arquivo);

    if (ctx->pfp(registro) == 1)
        incluirInfo(ctx->registros, registro);
    else
        free(registro);
}

pDLista queryBy  (pDFile arq, FuncaoPredicado pfp){

   if (arq->arquivo == NULL){
       printf("Arquivo n�o foi aberto!");
       return NULL;
   }

   pDLista registros = criarLista();

   struct ContextoQueryBy ctx;
   ctx.arq       = arq;
   ctx.pfp       = pfp;
   ctx.registros = registros;

   /* percorre o indice (arvore B+) em ordem de chave; o predicado eh
      avaliado sobre o registro inteiro (nao apenas a chave indexada),
      entao ainda precisamos buscar cada registro -- mas o acesso ao
      arquivo agora eh sempre via offset do indice, nunca varredura   */
   percorrerArv(arq->indice, visitanteQueryBy, &ctx);

   return registros;
}

#endif

