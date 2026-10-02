#ifndef PERSISTALL_H
#define PERSISTALL_H

void persistAll(pDFile arq, pDLista lista){

    if (arq->arquivo == NULL){
        printf("Arquivo nao foi aberto!");
        return;
    }

    /* fecha e reabre o arquivo em modo "w+b": esse modo trunca (zera)
       o conteudo anterior, permitindo reescrever o arquivo do zero  */
    fclose(arq->arquivo);
    arq->arquivo = fopen(arq->nomeArquivo, "w+b");

    pNoh atual = lista->primeiro;
    while (atual != NULL){
        fwrite(atual->info, arq->tamanhoRegistro, 1, arq->arquivo);
        atual = atual->prox;
    }
}

#endif
