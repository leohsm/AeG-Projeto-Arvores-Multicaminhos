#ifndef RETRIEVE_H
#define RETRIEVE_H

void* retrieve (pDFile arq, void* chave, FuncaoComparacao pfc){

   if (arq->arquivo == NULL){
       printf("Arquivo n�o foi aberto!");
       return NULL;
   }

   /* pfc eh mantido apenas para nao quebrar a assinatura publica ja
      existente; a comparacao de fato usa o indice (arvore B+), que
      guarda sua propria funcao de comparacao de chave-com-chave     */
   (void) pfc;

   long* offset = (long*) buscarArv(arq->indice, chave, arq->pfccChave);
   if (offset == NULL)
       return NULL;

   void* registro = malloc(arq->tamanhoRegistro);
   fseek(arq->arquivo, *offset, SEEK_SET);
   fread(registro, arq->tamanhoRegistro, 1, arq->arquivo);

   free(offset);

   return registro;
}


#endif
