"""Prepara cópias rastreáveis do framework fornecido no ZIP."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'original/FrameworkPersistencia_com_indice/source'

def read(path):
    return path.read_text(encoding='cp1252')

def write(path, text):
    path.write_text(text, encoding='utf-8')

for variant in ['baseline', 'solucao']:
    target = ROOT / variant / 'source'
    for src in ORIGINAL.rglob('*'):
        if src.is_file() and src.suffix in ['.h', '.c']:
            dst = target / src.relative_to(ORIGINAL)
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
    path = target / 'Operacoes/0_structs.h'
    write(path, read(path).replace('#define ORDEM_INDICE 4', '#ifndef ORDEM_INDICE\n#define ORDEM_INDICE 4\n#endif'))
    path = target / 'ArvoreBMais/Operacoes/0_structs.h'
    text = read(path).replace('struct arvB{', 'static unsigned long long nosCriadosArv = 0, splitsFolhaTotal = 0, splitsInternoTotal = 0;\n\nstruct arvB{\n    unsigned long long splitsFolha, splitsInterno, emprestimos, fusoes;')
    write(path, text)
    path = target / 'ArvoreBMais/Operacoes/1_criar.h'
    text = read(path).replace('noh->ehFolha = ehFolha;', 'nosCriadosArv++;\n    noh->ehFolha = ehFolha;')
    text = text.replace('arv->ordem        = ordem;', 'arv->splitsFolha = arv->splitsInterno = 0;\n    arv->emprestimos = arv->fusoes = 0;\n    arv->ordem        = ordem;')
    write(path, text)
    path = target / 'ArvoreBMais/Operacoes/3_inserir.h'
    text = read(path).replace('int meio = arv->ordem / 2;', 'arv->splitsFolha++; splitsFolhaTotal++;\n    int meio = arv->ordem / 2;')
    text = text.replace('int nOriginal = noh->n;', 'arv->splitsInterno++; splitsInternoTotal++;\n    int nOriginal = noh->n;')
    write(path, text)
    path = target / 'ArvoreBMais/Operacoes/4_remover.h'
    text = read(path)
    for call in ['emprestarDeEsquerdaFolha(arv, pai, idx, esquerda, noh);', 'emprestarDeDireitaFolha(arv, pai, idx, noh, direita);', 'emprestarDeEsquerdaInterno(arv, pai, idx, esquerda, noh);', 'emprestarDeDireitaInterno(arv, pai, idx, noh, direita);']:
        text = text.replace(call, 'arv->emprestimos++;\n            ' + call)
    text = text.replace('    int i;\n    for (i = 0; i < direito->n; i++){', '    arv->fusoes++;\n    int i;\n    for (i = 0; i < direito->n; i++){')
    text = text.replace('    esquerdo->chaves[esquerdo->n] = pai->chaves[idxSeparador];', '    arv->fusoes++;\n    esquerdo->chaves[esquerdo->n] = pai->chaves[idxSeparador];')
    write(path, text)
    # Mesmo relógio monotônico nos dois executáveis. Algoritmos do bench preservados.
    path = target / 'Benchmark/bench.c'
    text = read(path).replace('#include <time.h>', '#include <time.h>\n#include "timer.h"').replace('#include "Persistencia.h"', '#include "Persistencia.h"\n#include "metricas.h"')
    start = text.index('double agora(void){')
    end = text.index('\n}', start) + 2
    text = text[:start] + 'double agora(void){ return tempoMonotonico(); }' + text[end:]
    text = text.replace('int main(){', 'int main(int argc, char** argv){')
    text = text.replace('srand(12345);', 'srand(argc > 1 ? (unsigned)strtoul(argv[1], NULL, 10) : 12345);\n    FILE* metricas = fopen("metricas.csv", "w");\n    fprintf(metricas, "N,ordem,altura,nos,folhas,splits_folha,splits_interno,nos_criados,emprestimos_delete,fusoes_delete,splits_delete,nos_criados_delete\\n");')
    text = text.replace('        pDFile arqIdx = abrir(', '        nosCriadosArv = 0; splitsFolhaTotal = splitsInternoTotal = 0;\n        pDFile arqIdx = abrir(')
    text = text.replace('        registrar("createe", "com_indice", N, (t1 - t0) * 1000.0);', '        registrar("createe", "com_indice", N, (t1 - t0) * 1000.0);\n        EstatisticasArv est = estatisticasArv(arqIdx->indice);\n        unsigned long long sf = arqIdx->indice->splitsFolha, si = arqIdx->indice->splitsInterno, nc = nosCriadosArv;')
    text = text.replace('        registrar("deletee", "com_indice", N, (t1 - t0) * 1000.0);', '        registrar("deletee", "com_indice", N, (t1 - t0) * 1000.0);\n        fprintf(metricas, "%d,%d,%d,%d,%d,%llu,%llu,%llu,%llu,%llu,%llu,%llu\\n", N, ORDEM_INDICE, est.altura, est.nos, est.folhas, sf, si, nc, arqIdx->indice->emprestimos, arqIdx->indice->fusoes, splitsFolhaTotal + splitsInternoTotal - sf - si, nosCriadosArv - nc);')
    # Original apenas imprimia divergências; agora a execução falha em vez de gerar dados inválidos.
    text = text.replace('                printf("  !! CORRETUDE', '                exit(EXIT_FAILURE);\n                printf("  !! CORRETUDE')
    text = text.replace('    fclose(csv);', '    fclose(metricas);\n    fclose(csv);')
    write(path, text)

shutil.copy2(ROOT / 'scripts/delete_incremental.h', ROOT / 'solucao/source/Operacoes/5_delete.h')
path = ROOT / 'solucao/source/TAD_Persistencia.h'
text = path.read_text(encoding='cp1252').replace('pDLista queryAll  (pDFile);', 'pDLista queryAll  (pDFile);\npDLista queryAllSequencial (pDFile); // ordem física, para listagem sem ordenação')
write(path, text)
path = ROOT / 'solucao/source/Operacoes/7_queryAll.h'
sequential = '''/* Alternativa explícita: preserva pDLista, mas lê sequencialmente.
   Após swap, a ordem física difere da ordem de chave. */
pDLista queryAllSequencial(pDFile arq){
    if (!arq || !arq->arquivo) return NULL;
    pDLista registros = criarLista();
    rewind(arq->arquivo);
    for (;;){
        void* registro = malloc(arq->tamanhoRegistro);
        if (!registro) break;
        if (fread(registro, arq->tamanhoRegistro, 1, arq->arquivo) != 1){
            free(registro); break;
        }
        incluirInfo(registros, registro);
    }
    return registros;
}
'''
write(path, read(path).replace('#endif', sequential + '#endif'))
print('Cópias baseline/source e solucao/source preparadas.')
