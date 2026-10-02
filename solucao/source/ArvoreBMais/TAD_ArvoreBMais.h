#ifndef TAD_ARVOREBMAIS_H
#define TAD_ARVOREBMAIS_H

/* --------------------------------------------------------
   Biblioteca ArvoreBMais
   TAD generico de arvore B+ (chave/valor genericos), sem
   nenhuma dependencia da biblioteca Persistencia.
   -------------------------------------------------------- */

/* --------------------------
   Tipos de dados
   -------------------------- */
typedef struct nohArv  NohArv;
typedef NohArv*        pNohArv;

typedef struct arvB     ArvB;
typedef ArvB*           pArvB;

/* Compara duas chaves entre si (retorna 0 se iguais, <0 se
   chave1 < chave2, >0 se chave1 > chave2 -- mesma convencao
   de strcmp/comparaChavePessoa) */
typedef int (*FuncaoComparacaoArv)(void*, void*);

/* visita um par (chave, valor) durante um percurso ordenado da arvore;
   'contexto' eh repassado sem alteracoes, para o chamador acumular
   resultados sem precisar de variaveis globais                        */
typedef void (*FuncaoVisitanteArv)(void* chave, void* valor, void* contexto);

/* --------------------------------------------------------
   Operacoes
   -------------------------------------------------------- */

/* ordem        = numero maximo de filhos de um noh interno
                  (logo, no maximo ordem-1 chaves por noh)
   tamanhoChave = tamanho em bytes de cada chave
   tamanhoValor = tamanho em bytes de cada valor armazenado
                  nas folhas                                  */
pArvB   criarArvore     (int ordem, int tamanhoChave, int tamanhoValor);

void    inserirArv      (pArvB, void* chave, void* valor, FuncaoComparacaoArv);

/* retorna copia (malloc'd) do valor encontrado, ou NULL       */
void*   buscarArv       (pArvB, void* chave, FuncaoComparacaoArv);

/* retorna 1 se encontrou e removeu, 0 se a chave nao existia  */
int     removerArv      (pArvB, void* chave, FuncaoComparacaoArv);

void    destruirArvore  (pArvB);

/* grava, em ordem, os pares (chave, valor) das folhas em disco */
void    persistirArvore (pArvB, char nomeArquivo[40]);

/* recria a arvore lendo os pares gravados por persistirArvore
   (reinserindo cada par com a funcao de comparacao informada);
   se o arquivo nao existir, retorna uma arvore vazia          */
pArvB   carregarArvore  (char nomeArquivo[40], int ordem, int tamanhoChave,
                         int tamanhoValor, FuncaoComparacaoArv);

/* percorre todos os pares (chave, valor) em ordem crescente de chave
   (da folha mais a esquerda ate a mais a direita), chamando 'visitante'
   para cada um. Base para queryAll/queryBy da Persistencia usarem o
   indice em vez de varrer o arquivo de dados sequencialmente          */
void    percorrerArv    (pArvB, FuncaoVisitanteArv, void* contexto);

#endif
