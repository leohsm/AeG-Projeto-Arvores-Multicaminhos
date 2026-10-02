#ifndef OPEN_H
#define OPEN_H

pDFile abrir(char arquivo[30], int tamanho, int tamanhoChave,
             FuncaoChave pfch, FuncaoComparacao pfcc){

    pDFile pdf = (pDFile) malloc(sizeof(struct dFile));

    strcpy(pdf->nomeArquivo, arquivo);

    pdf->arquivo = fopen(arquivo, "r+b");
    if(pdf->arquivo == NULL)
        pdf->arquivo = fopen(arquivo, "w+b");

    pdf->tamanhoRegistro = tamanho;
    pdf->tamanhoChave    = tamanhoChave;
    pdf->pfch            = pfch;
    pdf->pfccChave        = pfcc;

    strcpy(pdf->nomeArquivoIndice, arquivo);
    strcat(pdf->nomeArquivoIndice, ".idx");

    /* o arquivo de indice ja existia? (precisamos saber antes de
       carregarArvore, que sempre devolve uma arvore -- vazia ou nao) */
    FILE* testeIdx = fopen(pdf->nomeArquivoIndice, "rb");
    int indiceJaExistia = (testeIdx != NULL);
    if (testeIdx != NULL)
        fclose(testeIdx);

    pdf->indice = carregarArvore(pdf->nomeArquivoIndice, ORDEM_INDICE,
                                  tamanhoChave, sizeof(long), pfcc);

    if (!indiceJaExistia){
        /* nao havia indice em disco: reconstroi varrendo o arquivo de
           dados inteiro uma vez (mesma logica de queryAll)            */
        void* registro = malloc(tamanho);
        rewind(pdf->arquivo);

        long posicao = ftell(pdf->arquivo);
        while (fread(registro, tamanho, 1, pdf->arquivo) == 1){
            void* chave = pfch(registro);
            inserirArv(pdf->indice, chave, &posicao, pfcc);
            posicao = ftell(pdf->arquivo);
        }
        free(registro);
    }

    return pdf;
}


#endif
