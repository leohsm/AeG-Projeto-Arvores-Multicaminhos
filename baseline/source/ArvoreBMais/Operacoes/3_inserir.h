#ifndef ARVB_INSERIR_H
#define ARVB_INSERIR_H

/* insere (chave, valor) numa folha, mantendo a ordenacao.
   a arvore guarda copias proprias de chave e valor (o chamador
   continua dono dos ponteiros originais que passou)            */
void inserirNaFolha(pArvB arv, pNohArv folha, void* chave, void* valor, FuncaoComparacaoArv pfc){

    int i = folha->n;
    while (i > 0 && pfc(chave, folha->chaves[i-1]) < 0){
        folha->chaves[i]  = folha->chaves[i-1];
        folha->valores[i] = folha->valores[i-1];
        i--;
    }

    void* copiaChave = malloc(arv->tamanhoChave);
    memcpy(copiaChave, chave, arv->tamanhoChave);

    void* copiaValor = malloc(arv->tamanhoValor);
    memcpy(copiaValor, valor, arv->tamanhoValor);

    folha->chaves[i]  = copiaChave;
    folha->valores[i] = copiaValor;
    folha->n++;
}

/* --------------------------------------------------------- */
/* insere uma chave promovida (ja alocada pela propria arvore)
   e o filho a direita dela num noh interno                    */
void inserirChaveNoInterno(pNohArv noh, void* chave, pNohArv filhoDireito, FuncaoComparacaoArv pfc){

    int i = noh->n;
    while (i > 0 && pfc(chave, noh->chaves[i-1]) < 0){
        noh->chaves[i]   = noh->chaves[i-1];
        noh->filhos[i+1] = noh->filhos[i];
        i--;
    }

    noh->chaves[i]     = chave;
    noh->filhos[i+1]   = filhoDireito;
    filhoDireito->pai  = noh;
    noh->n++;
}

/* --------------------------------------------------------- */
/* divide uma folha cheia (n == ordem) em duas; a primeira
   chave da nova folha da direita eh copiada para servir de
   separador no noh pai                                        */
pNohArv dividirFolha(pArvB arv, pNohArv folha, void** chavePromovidaOut){

    arv->splitsFolha++; splitsFolhaTotal++;
    int meio = arv->ordem / 2;

    pNohArv nova = criarNoh(arv->ordem, 1);

    int i, j;
    for (i = meio, j = 0; i < folha->n; i++, j++){
        nova->chaves[j]  = folha->chaves[i];
        nova->valores[j] = folha->valores[i];
    }
    nova->n  = folha->n - meio;
    folha->n = meio;

    nova->proximo  = folha->proximo;
    folha->proximo = nova;
    nova->pai      = folha->pai;

    void* chavePromovida = malloc(arv->tamanhoChave);
    memcpy(chavePromovida, nova->chaves[0], arv->tamanhoChave);
    *chavePromovidaOut = chavePromovida;

    return nova;
}

/* --------------------------------------------------------- */
/* divide um noh interno cheio (n == ordem); diferente da folha,
   a chave do meio SOBE para o pai e NAO fica duplicada aqui   */
pNohArv dividirInterno(pArvB arv, pNohArv noh, void** chavePromovidaOut){

    arv->splitsInterno++; splitsInternoTotal++;
    int nOriginal = noh->n;
    int meio      = arv->ordem / 2;

    pNohArv novo = criarNoh(arv->ordem, 0);

    int i, j;
    for (i = meio + 1, j = 0; i < nOriginal; i++, j++)
        novo->chaves[j] = noh->chaves[i];
    novo->n = nOriginal - (meio + 1);

    for (i = meio + 1, j = 0; i <= nOriginal; i++, j++){
        novo->filhos[j] = noh->filhos[i];
        novo->filhos[j]->pai = novo;
    }

    *chavePromovidaOut = noh->chaves[meio];

    novo->pai = noh->pai;
    noh->n    = meio;

    return novo;
}

/* --------------------------------------------------------- */
void inserirArv(pArvB arv, void* chave, void* valor, FuncaoComparacaoArv pfc){

    pNohArv folha = encontrarFolha(arv, chave, pfc);

    inserirNaFolha(arv, folha, chave, valor, pfc);

    if (folha->n < arv->ordem)
        return; /* sem overflow, terminou */

    void* chavePromovida;
    pNohArv esquerdo = folha;
    pNohArv direito  = dividirFolha(arv, folha, &chavePromovida);

    /* propaga a divisao para cima ate parar de estourar */
    while (1){

        pNohArv pai = esquerdo->pai;

        if (pai == NULL){
            /* esquerdo era a raiz: cria uma nova raiz interna */
            pNohArv novaRaiz = criarNoh(arv->ordem, 0);
            novaRaiz->chaves[0] = chavePromovida;
            novaRaiz->filhos[0] = esquerdo;
            novaRaiz->filhos[1] = direito;
            novaRaiz->n = 1;
            esquerdo->pai = novaRaiz;
            direito->pai  = novaRaiz;
            arv->raiz = novaRaiz;
            return;
        }

        inserirChaveNoInterno(pai, chavePromovida, direito, pfc);

        if (pai->n < arv->ordem)
            return; /* pai nao estourou: terminou */

        /* pai tambem estourou: divide e continua propagando */
        void* novaChavePromovida;
        pNohArv novoDireito = dividirInterno(arv, pai, &novaChavePromovida);

        chavePromovida = novaChavePromovida;
        esquerdo = pai;
        direito  = novoDireito;
    }
}

#endif
