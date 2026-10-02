#ifndef ARVB_DESTRUIR_H
#define ARVB_DESTRUIR_H

void destruirNohArvRecursivo(pNohArv noh){

    if (noh == NULL)
        return;

    if (noh->ehFolha){
        int i;
        for (i = 0; i < noh->n; i++){
            free(noh->chaves[i]);
            free(noh->valores[i]);
        }
        free(noh->chaves);
        free(noh->valores);
    } else {
        int i;
        for (i = 0; i <= noh->n; i++)
            destruirNohArvRecursivo(noh->filhos[i]);
        for (i = 0; i < noh->n; i++)
            free(noh->chaves[i]);
        free(noh->chaves);
        free(noh->filhos);
    }

    free(noh);
}

void destruirArvore(pArvB arv){
    destruirNohArvRecursivo(arv->raiz);
    free(arv);
}

#endif
