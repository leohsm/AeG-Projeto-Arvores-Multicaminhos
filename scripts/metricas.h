#ifndef PROJETO_METRICAS_H
#define PROJETO_METRICAS_H
/* Incluir depois de Persistencia.h. Altura em arestas: folha-raiz = 0. */
typedef struct { int altura, nos, folhas; } EstatisticasArv;
static EstatisticasArv estatisticasNoh(pNohArv noh){
    EstatisticasArv e = {0, 1, noh->ehFolha ? 1 : 0};
    if (!noh->ehFolha){
        for (int i = 0; i <= noh->n; i++){
            EstatisticasArv c = estatisticasNoh(noh->filhos[i]);
            if (c.altura + 1 > e.altura) e.altura = c.altura + 1;
            e.nos += c.nos;
            e.folhas += c.folhas;
        }
    }
    return e;
}
static EstatisticasArv estatisticasArv(pArvB arv){ return estatisticasNoh(arv->raiz); }
#endif
