#ifndef DELETE_H
#define DELETE_H

void deletee(pDFile arq, void* chave, FuncaoComparacao pfc){

    if (arq->arquivo == NULL){
        printf("Arquivo nao foi aberto!");
        return;
    }

    int encontrou = 0;

    /* le todos os registros do arquivo, guardando em uma lista
       apenas os que NAO correspondem a chave a ser excluida     */
    pDLista registros = criarLista();

    void* registro = malloc(arq->tamanhoRegistro);
    rewind(arq->arquivo);

    while (fread(registro, arq->tamanhoRegistro, 1, arq->arquivo) != 0){

        if (pfc(chave, registro) == 0){
            /* este eh o registro que deve ser excluido: descarta */
            encontrou = 1;
            free(registro);
        } else {
            incluirInfo(registros, registro);
        }

        registro = malloc(arq->tamanhoRegistro);
    }
    free(registro); /* ultima alocacao nao foi usada (fim do arquivo) */

    if (!encontrou){
        printf("Registro nao encontrado!");
    } else {
        /* reescreve o arquivo do zero, apenas com os registros restantes */
        persistAll(arq, registros);

        /* como o delete desloca o offset de todos os sobreviventes no
           arquivo, remendar o indice antigo seria mais complexo (e mais
           arriscado) do que reconstrui-lo do zero nesta mesma varredura */
        destruirArvore(arq->indice);
        arq->indice = criarArvore(ORDEM_INDICE, arq->tamanhoChave, sizeof(long));

        long posicao = 0;
        pNoh atualIdx = registros->primeiro;
        while (atualIdx != NULL){
            void* chaveRegistro = arq->pfch(atualIdx->info);
            inserirArv(arq->indice, chaveRegistro, &posicao, arq->pfccChave);
            posicao += arq->tamanhoRegistro;
            atualIdx = atualIdx->prox;
        }
    }

    /* libera a memoria dos registros lidos (ja persistidos em disco) */
    pNoh atual = registros->primeiro;
    while (atual != NULL){
        pNoh prox = atual->prox;
        free(atual->info);
        free(atual);
        atual = prox;
    }
    free(registros);
}

#endif
