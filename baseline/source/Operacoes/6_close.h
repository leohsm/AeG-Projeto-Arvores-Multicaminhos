#ifndef CLOSE_H
#define CLOSE_H

void fechar(pDFile arq){

    /* salva o indice em disco (arquivo .idx) e libera sua memoria
       antes de fechar o arquivo de dados                          */
    persistirArvore(arq->indice, arq->nomeArquivoIndice);
    destruirArvore(arq->indice);

    fclose(arq->arquivo);

    free(arq);

}

#endif
