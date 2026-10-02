#ifndef CREATE_H
#define CREATE_H

void createe (pDFile arq, void* dados){

    if (arq->arquivo == NULL){
        printf("Arquivo n�o foi aberto!");
        return;
    }

    fseek(arq->arquivo, 0, SEEK_END);
    long posicao = ftell(arq->arquivo);

    fwrite(dados, arq->tamanhoRegistro, 1 , arq->arquivo);

    /* mantem o indice atualizado com a chave do novo registro */
    void* chave = arq->pfch(dados);
    inserirArv(arq->indice, chave, &posicao, arq->pfccChave);
}


#endif
