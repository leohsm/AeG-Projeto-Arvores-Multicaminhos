#ifndef ARVB_REMOVER_H
#define ARVB_REMOVER_H

/* ocupacao minima de qualquer noh (exceto a raiz), tanto para
   folhas quanto para nohs internos -- simplificacao didatica
   consistente para os dois tipos de noh                       */
int ocupacaoMinima(pArvB arv){
    return (arv->ordem - 1) / 2;
}

/* --------------------------------------------------------- *
 *  emprestimos e fusoes entre folhas                          *
 * --------------------------------------------------------- */
void emprestarDeEsquerdaFolha(pArvB arv, pNohArv pai, int idx, pNohArv esquerda, pNohArv noh){

    int i;
    for (i = noh->n; i > 0; i--){
        noh->chaves[i]  = noh->chaves[i-1];
        noh->valores[i] = noh->valores[i-1];
    }

    noh->chaves[0]  = esquerda->chaves[esquerda->n - 1];
    noh->valores[0] = esquerda->valores[esquerda->n - 1];
    noh->n++;
    esquerda->n--;

    free(pai->chaves[idx-1]);
    pai->chaves[idx-1] = malloc(arv->tamanhoChave);
    memcpy(pai->chaves[idx-1], noh->chaves[0], arv->tamanhoChave);
}

void emprestarDeDireitaFolha(pArvB arv, pNohArv pai, int idx, pNohArv noh, pNohArv direita){

    noh->chaves[noh->n]  = direita->chaves[0];
    noh->valores[noh->n] = direita->valores[0];
    noh->n++;

    int i;
    for (i = 0; i < direita->n - 1; i++){
        direita->chaves[i]  = direita->chaves[i+1];
        direita->valores[i] = direita->valores[i+1];
    }
    direita->n--;

    free(pai->chaves[idx]);
    pai->chaves[idx] = malloc(arv->tamanhoChave);
    memcpy(pai->chaves[idx], direita->chaves[0], arv->tamanhoChave);
}

void mesclarFolhas(pArvB arv, pNohArv pai, int idxSeparador, pNohArv esquerdo, pNohArv direito){

    int i;
    for (i = 0; i < direito->n; i++){
        esquerdo->chaves[esquerdo->n + i]  = direito->chaves[i];
        esquerdo->valores[esquerdo->n + i] = direito->valores[i];
    }
    esquerdo->n += direito->n;
    esquerdo->proximo = direito->proximo;

    free(pai->chaves[idxSeparador]);
    for (i = idxSeparador; i < pai->n - 1; i++)
        pai->chaves[i] = pai->chaves[i+1];
    for (i = idxSeparador + 1; i < pai->n; i++)
        pai->filhos[i] = pai->filhos[i+1];
    pai->n--;

    free(direito->chaves);
    free(direito->valores);
    free(direito);
}

/* --------------------------------------------------------- *
 *  emprestimos e fusoes entre nohs internos                   *
 * --------------------------------------------------------- */
void emprestarDeEsquerdaInterno(pArvB arv, pNohArv pai, int idx, pNohArv esquerda, pNohArv noh){

    int i;
    for (i = noh->n; i > 0; i--)
        noh->chaves[i] = noh->chaves[i-1];
    for (i = noh->n + 1; i > 0; i--)
        noh->filhos[i] = noh->filhos[i-1];

    noh->chaves[0] = malloc(arv->tamanhoChave);
    memcpy(noh->chaves[0], pai->chaves[idx-1], arv->tamanhoChave);

    noh->filhos[0] = esquerda->filhos[esquerda->n];
    noh->filhos[0]->pai = noh;
    noh->n++;

    free(pai->chaves[idx-1]);
    pai->chaves[idx-1] = esquerda->chaves[esquerda->n - 1]; /* transfere posse */

    esquerda->n--;
}

void emprestarDeDireitaInterno(pArvB arv, pNohArv pai, int idx, pNohArv noh, pNohArv direita){

    noh->chaves[noh->n] = malloc(arv->tamanhoChave);
    memcpy(noh->chaves[noh->n], pai->chaves[idx], arv->tamanhoChave);

    noh->filhos[noh->n + 1] = direita->filhos[0];
    noh->filhos[noh->n + 1]->pai = noh;
    noh->n++;

    free(pai->chaves[idx]);
    pai->chaves[idx] = direita->chaves[0]; /* transfere posse */

    int i;
    for (i = 0; i < direita->n - 1; i++)
        direita->chaves[i] = direita->chaves[i+1];
    for (i = 0; i < direita->n; i++)
        direita->filhos[i] = direita->filhos[i+1];
    direita->n--;
}

void mesclarInternos(pArvB arv, pNohArv pai, int idxSeparador, pNohArv esquerdo, pNohArv direito){

    esquerdo->chaves[esquerdo->n] = pai->chaves[idxSeparador]; /* transfere posse */
    esquerdo->n++;

    int i;
    for (i = 0; i < direito->n; i++)
        esquerdo->chaves[esquerdo->n + i] = direito->chaves[i];
    for (i = 0; i <= direito->n; i++){
        esquerdo->filhos[esquerdo->n + i] = direito->filhos[i];
        esquerdo->filhos[esquerdo->n + i]->pai = esquerdo;
    }
    esquerdo->n += direito->n;

    for (i = idxSeparador; i < pai->n - 1; i++)
        pai->chaves[i] = pai->chaves[i+1];
    for (i = idxSeparador + 1; i < pai->n; i++)
        pai->filhos[i] = pai->filhos[i+1];
    pai->n--;

    free(direito->chaves);
    free(direito->filhos);
    free(direito);
}

/* --------------------------------------------------------- */
void corrigirUnderflow(pArvB arv, pNohArv noh){

    if (noh == arv->raiz){
        if (!noh->ehFolha && noh->n == 0){
            /* raiz interna ficou sem chaves: seu unico filho vira a nova raiz */
            pNohArv novaRaiz = noh->filhos[0];
            novaRaiz->pai = NULL;
            arv->raiz = novaRaiz;
            free(noh->chaves);
            free(noh->filhos);
            free(noh);
        }
        /* raiz-folha nao tem ocupacao minima: nada a corrigir */
        return;
    }

    int min = ocupacaoMinima(arv);
    if (noh->n >= min)
        return;

    pNohArv pai = noh->pai;

    int idx = 0;
    while (pai->filhos[idx] != noh)
        idx++;

    pNohArv esquerda = (idx > 0)      ? pai->filhos[idx-1] : NULL;
    pNohArv direita   = (idx < pai->n) ? pai->filhos[idx+1] : NULL;

    if (noh->ehFolha){
        if (esquerda != NULL && esquerda->n > min){
            emprestarDeEsquerdaFolha(arv, pai, idx, esquerda, noh);
            return;
        }
        if (direita != NULL && direita->n > min){
            emprestarDeDireitaFolha(arv, pai, idx, noh, direita);
            return;
        }
        if (esquerda != NULL)
            mesclarFolhas(arv, pai, idx - 1, esquerda, noh);
        else
            mesclarFolhas(arv, pai, idx, noh, direita);
    } else {
        if (esquerda != NULL && esquerda->n > min){
            emprestarDeEsquerdaInterno(arv, pai, idx, esquerda, noh);
            return;
        }
        if (direita != NULL && direita->n > min){
            emprestarDeDireitaInterno(arv, pai, idx, noh, direita);
            return;
        }
        if (esquerda != NULL)
            mesclarInternos(arv, pai, idx - 1, esquerda, noh);
        else
            mesclarInternos(arv, pai, idx, noh, direita);
    }

    /* a fusao pode ter deixado o pai tambem abaixo do minimo */
    corrigirUnderflow(arv, pai);
}

/* --------------------------------------------------------- */
int removerArv(pArvB arv, void* chave, FuncaoComparacaoArv pfc){

    pNohArv folha = encontrarFolha(arv, chave, pfc);

    int i = 0;
    while (i < folha->n && pfc(chave, folha->chaves[i]) != 0)
        i++;

    if (i == folha->n)
        return 0; /* chave nao encontrada */

    free(folha->chaves[i]);
    free(folha->valores[i]);

    int j;
    for (j = i; j < folha->n - 1; j++){
        folha->chaves[j]  = folha->chaves[j+1];
        folha->valores[j] = folha->valores[j+1];
    }
    folha->n--;

    corrigirUnderflow(arv, folha);

    return 1;
}

#endif
