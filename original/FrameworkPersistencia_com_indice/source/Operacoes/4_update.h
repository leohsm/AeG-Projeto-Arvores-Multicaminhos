#ifndef UPDATE_H
#define UPDATE_H

void update(pDFile arq, void* chave, void* dados, FuncaoComparacao pfc){

    if (arq->arquivo == NULL){
       printf("Arquivo n�o foi aberto!");
       return;
   }

   /* pfc eh mantido pela assinatura publica ja existente; a localizacao
      do registro usa o indice (retrieve), que compara chaves com a
      funcao guardada no proprio arquivo (arq->pfccChave)              */
   (void) pfc;

   void *registro = retrieve(arq, chave, pfc);
   if(registro != NULL)
   {
       fseek(arq->arquivo, - arq->tamanhoRegistro, SEEK_CUR);
       fwrite(dados, arq->tamanhoRegistro, 1, arq->arquivo);

       /* se a chave do registro mudou com essa atualizacao, o indice
          precisa apontar para a nova chave (o offset continua o mesmo) */
       void* chaveNova = arq->pfch(dados);
       if (arq->pfccChave(chave, chaveNova) != 0){
           long offsetAtual = ftell(arq->arquivo) - arq->tamanhoRegistro;
           removerArv(arq->indice, chave, arq->pfccChave);
           inserirArv(arq->indice, chaveNova, &offsetAtual, arq->pfccChave);
       }

       free(registro);
   }

}

#endif
