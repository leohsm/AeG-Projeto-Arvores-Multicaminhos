#ifndef DELETE_H
#define DELETE_H
#include <errno.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

/* Chaves únicas e registros de tamanho fixo, como no bench.c fornecido.
   Apenas o último registro muda de posição. A ordem física não é preservada.
   A assinatura original é mantida; erros de I/O são sinalizados em errno. */
void deletee(pDFile arq, void* chave, FuncaoComparacao pfc){
    (void)pfc;
    if (!arq || !arq->arquivo || !arq->indice || arq->tamanhoRegistro <= 0){
        errno = EINVAL; return;
    }
    long* encontrado = buscarArv(arq->indice, chave, arq->pfccChave);
    if (!encontrado){ errno = ENOENT; return; }
    long offset = *encontrado;
    free(encontrado);
    if (fseek(arq->arquivo, 0, SEEK_END) != 0) return;
    long tamanho = ftell(arq->arquivo);
    long ultimo = tamanho - arq->tamanhoRegistro;
    if (tamanho < arq->tamanhoRegistro || tamanho % arq->tamanhoRegistro ||
        offset < 0 || offset > ultimo || offset % arq->tamanhoRegistro){
        errno = EIO; return;
    }

    void* final = NULL;
    void* anterior = NULL;
    void* valorMovido = NULL;
    if (offset != ultimo){
        final = malloc(arq->tamanhoRegistro);
        anterior = malloc(arq->tamanhoRegistro);
        if (!final || !anterior){ errno = ENOMEM; goto liberar; }
        if (fseek(arq->arquivo, ultimo, SEEK_SET) != 0 ||
            fread(final, arq->tamanhoRegistro, 1, arq->arquivo) != 1 ||
            fseek(arq->arquivo, offset, SEEK_SET) != 0 ||
            fread(anterior, arq->tamanhoRegistro, 1, arq->arquivo) != 1){
            errno = EIO; goto liberar;
        }
        /* buscarArv devolve CÓPIA do valor. A atualização deve escrever
           no valor que pertence à folha. */
        void* chaveMovida = arq->pfch(final);
        pNohArv folha = encontrarFolha(arq->indice, chaveMovida, arq->pfccChave);
        for (int i = 0; i < folha->n; i++){
            if (arq->pfccChave(chaveMovida, folha->chaves[i]) == 0){
                valorMovido = folha->valores[i]; break;
            }
        }
        if (!valorMovido || *(long*)valorMovido != ultimo ||
            arq->pfccChave(chaveMovida, chave) == 0){
            errno = EIO; goto liberar;
        }
        if (fseek(arq->arquivo, offset, SEEK_SET) != 0 ||
            fwrite(final, arq->tamanhoRegistro, 1, arq->arquivo) != 1 ||
            fflush(arq->arquivo) != 0){
            int erro = errno;
            clearerr(arq->arquivo);
            if (fseek(arq->arquivo, offset, SEEK_SET) == 0){
                fwrite(anterior, arq->tamanhoRegistro, 1, arq->arquivo);
                fflush(arq->arquivo);
            }
            errno = erro ? erro : EIO; goto liberar;
        }
    }
    /* Sincronizar o buffer de stdio ANTES de truncar pelo descritor. */
    if (fflush(arq->arquivo) != 0) goto liberar;
#ifdef _WIN32
    int falhou = _chsize_s(_fileno(arq->arquivo), ultimo) != 0;
#else
    int falhou = ftruncate(fileno(arq->arquivo), (off_t)ultimo) != 0;
#endif
    if (falhou){
        int erro = errno;
        if (anterior && fseek(arq->arquivo, offset, SEEK_SET) == 0){
            fwrite(anterior, arq->tamanhoRegistro, 1, arq->arquivo);
            fflush(arq->arquivo);
        }
        errno = erro ? erro : EIO; goto liberar;
    }
    /* Atualizar antes de remover: empréstimos/fusões podem mover a folha,
       mas mantêm a posse do buffer de valor do sobrevivente. */
    if (valorMovido) memcpy(valorMovido, &offset, sizeof(long));
    removerArv(arq->indice, chave, arq->pfccChave);
    clearerr(arq->arquivo);
    errno = 0;
liberar:
    free(final);
    free(anterior);
}
#endif
