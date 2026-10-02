#ifndef ARVB_CRIAR_H
#define ARVB_CRIAR_H

/* cria um noh vazio, com capacidade para 'ordem' chaves/filhos.
   usado tanto para folhas quanto para nohs internos            */
pNohArv criarNoh(int ordem, int ehFolha){

    pNohArv noh = (pNohArv) malloc(sizeof(NohArv));

    nosCriadosArv++;
    noh->ehFolha = ehFolha;
    noh->n       = 0;

    /* aloca com 1 posicao a mais do que o maximo de 'ordem-1' chaves,
       para caber temporariamente a chave excedente antes da divisao */
    noh->chaves  = (void**) malloc(sizeof(void*) * ordem);

    if (ehFolha){
        noh->valores = (void**) malloc(sizeof(void*) * ordem);
        noh->filhos  = NULL;
    } else {
        noh->valores = NULL;
        noh->filhos  = (struct nohArv**) malloc(sizeof(struct nohArv*) * (ordem + 1));
    }

    noh->proximo = NULL;
    noh->pai     = NULL;

    return noh;
}

/* --------------------------------------------------------- */
pArvB criarArvore(int ordem, int tamanhoChave, int tamanhoValor){

    pArvB arv = (pArvB) malloc(sizeof(ArvB));

    arv->splitsFolha = arv->splitsInterno = 0;
    arv->emprestimos = arv->fusoes = 0;
    arv->ordem        = ordem;
    arv->tamanhoChave  = tamanhoChave;
    arv->tamanhoValor  = tamanhoValor;

    /* a arvore comeca com uma unica folha vazia, que eh a raiz */
    arv->raiz = criarNoh(ordem, 1);

    return arv;
}

#endif
