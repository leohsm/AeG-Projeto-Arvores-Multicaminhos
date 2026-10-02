/* --------------------------
   Tipos de dados
   -------------------------- */
typedef struct dFile DFile;
typedef DFile*       pDFile;

typedef int   (*FuncaoComparacao) (void *, void *);
typedef void* (*FuncaoAloca)      ();
typedef void  (*FuncaoImpressao)  (void *);
typedef int   (*FuncaoPredicado)  (void *);

// Uma fun��o de predicato estendida recebe os dados e um seletor para avaliar
// o predicado.
// Por exemplo, quando aplicado ao cadastro de pessoa, pode-se filtrar as
// pessoas que fazem anivers�rio em um determinado m�s, sendo que o
// primeiro par�metro s�o os dados da pessoa e o segundo par�metro � o m�s.
typedef int   (*FuncaoPredicadoExt)  (void*, void*);

// Toda fun��o de atualiza��o precisa definir dois par�metros:
//   1) o primeiro deles � os dados a serem atualizados (em geral a struct), e
//   2) o segundo par�metro � o valor que ser� utilizado para atualizar
// A fun��o deve retornar os dados (da struct) atualizados.
typedef void* (*FuncaoAtualizacao)(void *, void *);

// Extrai o ponteiro para o campo-chave de dentro de um registro. Usada
// internamente para manter o indice (arvore B+) atualizado em createe/update,
// ja que essas operacoes recebem o registro inteiro, nao apenas a chave.
typedef void* (*FuncaoChave) (void *);

/* --------------------------
   Opera��es CRUD
               Create
               Retrieve
               Update
               Delete
   -------------------------- */
// abrir agora tambem recebe o tamanho da chave, a funcao que extrai a chave
// de um registro (pfch) e a funcao que compara duas chaves entre si (pfcc) --
// necessarios para manter um indice em arvore B+ por baixo dos panos.
// As demais operacoes (createe/retrieve/update/deletee) continuam com a
// mesma assinatura de sempre.
pDFile  abrir     (char[30], int, int, FuncaoChave, FuncaoComparacao);
void    createe   (pDFile, void*);                          // dados
void*   retrieve  (pDFile, void*, FuncaoComparacao);        // chave
void    update    (pDFile, void*, void*, FuncaoComparacao); // chave e os dados
void    deletee   (pDFile, void*, FuncaoComparacao);        // chave

void    fechar     (pDFile);

pDLista queryAll  (pDFile);
pDLista queryAllSequencial (pDFile); // ordem física, para listagem sem ordenação
pDLista queryBy   (pDFile, FuncaoPredicado);

void    persistAll (pDFile, pDLista);




