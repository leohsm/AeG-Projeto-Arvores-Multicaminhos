#ifndef ARVB_STRUCTS_H
#define ARVB_STRUCTS_H

/* -------------------------------------------------------
   Noh da arvore B+.

   Se ehFolha == 1:
      - chaves[0..n-1]  e valores[0..n-1] guardam os pares
        (chave, valor) desse noh, em ordem crescente de chave
      - proximo aponta para a folha seguinte (permite
        percorrer todas as folhas em ordem, sem subir/descer
        na arvore)
      - filhos nao eh usado

   Se ehFolha == 0 (noh interno):
      - chaves[0..n-1] sao apenas separadores de roteamento
      - filhos[0..n] aponta para os n+1 subarvores
      - valores e proximo nao sao usados
   ------------------------------------------------------- */
struct nohArv{

    int        ehFolha;
    int        n;          // quantidade de chaves ocupadas no noh

    void**     chaves;     // vetor com capacidade para 'ordem' ponteiros
    void**     valores;    // vetor com capacidade para 'ordem' ponteiros (so folhas)
    struct nohArv** filhos; // vetor com capacidade para 'ordem+1' ponteiros (so internos)

    struct nohArv*  proximo; // encadeamento de folhas (so folhas)
    struct nohArv*  pai;
};

/* -------------------------------------------------------
   Descritor da arvore B+
   ------------------------------------------------------- */
struct arvB{

    struct nohArv* raiz;

    int  ordem;          // numero maximo de filhos por noh interno
    int  tamanhoChave;   // tamanho (bytes) de cada chave
    int  tamanhoValor;   // tamanho (bytes) de cada valor
};

#endif
