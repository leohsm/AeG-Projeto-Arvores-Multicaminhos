#ifndef ARVB_BUSCAR_H
#define ARVB_BUSCAR_H

/* desce da raiz ate a folha onde 'chave' deveria estar,
   seguindo a convencao: filhos[i] guarda chaves menores que
   chaves[i]; filhos[n] guarda as chaves maiores ou iguais a
   chaves[n-1]                                                */
pNohArv encontrarFolha(pArvB arv, void* chave, FuncaoComparacaoArv pfc){

    pNohArv atual = arv->raiz;

    while (!atual->ehFolha){

        int i = 0;
        while (i < atual->n && pfc(chave, atual->chaves[i]) >= 0)
            i++;

        atual = atual->filhos[i];
    }

    return atual;
}

/* --------------------------------------------------------- */
void* buscarArv(pArvB arv, void* chave, FuncaoComparacaoArv pfc){

    pNohArv folha = encontrarFolha(arv, chave, pfc);

    int i;
    for (i = 0; i < folha->n; i++){
        if (pfc(chave, folha->chaves[i]) == 0){
            void* copia = malloc(arv->tamanhoValor);
            memcpy(copia, folha->valores[i], arv->tamanhoValor);
            return copia;
        }
    }

    return NULL;
}

#endif
