#ifndef STRUCT_FILE_H
#define STRUCT_FILE_H

/* ordem da arvore B+ usada como indice de cada arquivo aberto */
#define ORDEM_INDICE 4

/* --------------------------
   Tipos de dados
   -------------------------- */
struct dFile{

  FILE* arquivo;         // ponteiro para o arquivo aberto em disco

  char  nomeArquivo[30]; // nome do arquivo em disco (necessario para
                         // reabrir/truncar o arquivo em persistAll/deletee)

  int   tamanhoRegistro; // qtde bytes do tipo de dado (struct)

  /* ------ indice (arvore B+): chave -> offset (long) no arquivo ------ */
  pArvB             indice;
  int               tamanhoChave;
  FuncaoChave       pfch;         // extrai a chave de dentro de um registro
  FuncaoComparacao  pfccChave;    // compara duas chaves entre si
  char              nomeArquivoIndice[40]; // nomeArquivo + ".idx"
};

#endif
