#ifndef ARVB_PERCORRER_H
#define ARVB_PERCORRER_H

/* percorre as folhas da esquerda para a direita (usando o encadeamento
   'proximo'), ou seja, em ordem crescente de chave, chamando
   'visitante(chave, valor, contexto)' para cada par armazenado         */
void percorrerArv(pArvB arv, FuncaoVisitanteArv visitante, void* contexto){

    pNohArv atual = arv->raiz;
    while (atual != NULL && !atual->ehFolha)
        atual = atual->filhos[0];

    while (atual != NULL){
        int i;
        for (i = 0; i < atual->n; i++)
            visitante(atual->chaves[i], atual->valores[i], contexto);
        atual = atual->proximo;
    }
}

#endif
