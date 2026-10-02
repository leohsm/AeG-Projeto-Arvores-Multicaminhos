# Projeto Prático 2 - Árvores Multicaminhos

**UTFPR - Câmpus Medianeira | Ciência da Computação | Árvores e Grafos | 2026.2**

**Equipe:** Erik Mazzuco, Letícia Moro, Leonardo Herrero

**Prazo do enunciado:** 04/10/2026

O [relatório PDF](output/pdf/RELATORIO_PROJETO_2.pdf) e o [pacote de entrega](output/PROJETO_2_ENTREGA.zip) estão disponíveis neste repositório. A entrega cobre os itens 4, 5 e 6 do PDF, usando o framework da disciplina recebido no arquivo `FrameworkPersistencia_com_indice.zip`.

## O que foi implementado

- `deletee` troca o registro removido pelo último, trunca um registro, corrige apenas o offset movido e usa `removerArv`. O índice permanece em memória, sem reconstrução.
- `ORDEM_INDICE` configurável pelo compilador; ordens 4, 8, 16, 32 e 64 avaliadas.
- Instrumentação de nós criados, splits, empréstimos e fusões; cálculo de altura e nós vivos fora do trecho cronometrado.
- `queryAll` continua em ordem de chave. `queryAllSequencial` adiciona uma alternativa em ordem física com a mesma saída `pDLista`.
- `bench.c` fornecido foi reexecutado com os mesmos algoritmos e volumes de operações, acrescentando relógio portátil, semente configurável, métricas e falha explícita na divergência de contagem.
- Estudo complementar compara materialização em lista/buffer, arquivo aleatório/ordenado e busca apenas em RAM com comparações contadas.

## Resultados nesta máquina

Três rodadas independentes, sementes 12345, 12346 e 12347. Em N = 100.000 e ordem 4, a mediana de 30 exclusões passou de **5.376,468 ms para 2,665 ms**, aproximadamente **2.018 vezes mais rápida**. Nenhuma exclusão criou nós ou executou splits. O teste completo passou nas cinco ordens.

A altura em arestas passou de 10 na ordem 4 para 2 na ordem 64. A ordem 32 teve a menor mediana de busca somente em RAM, nesta carga. `queryAll` com índice permaneceu mais lenta: a lista de folhas contém chaves e offsets; ela ainda precisa ler todos os registros e materializar a saída. O experimento com a mesma representação separa esse efeito do custo de alocação.

Consulte o relatório para as tabelas completas, a variação das rodadas e as limitações. Os tempos foram medidos em Windows 11 com Zig 0.16.0 / Clang, `-O2 -std=c11`, arquivos locais e cache do SO ativo; não são estimativas nem os números históricos do relatório fornecido.

## Estrutura

```text
original/                 material recebido, sem alterações
baseline/source/          algoritmos originais, relógio e instrumentação iguais
solucao/source/           implementação corrigida
tests/test_persistencia.c  validação funcional e estrutural
benchmarks/estudo.c        controles de queryAll e busca em RAM
scripts/executar.py        compilação, testes e medições sequenciais
scripts/gerar_relatorio.py gráficos, resumo CSV, Markdown e PDF
results/                  dados reais, ambiente, sementes e logs
plots/                    gráficos em PNG
output/pdf/               relatório para entrega
alteracoes.patch          diferenças entre original e solução
manifest.json             hashes de origem
referencia/               identificação da equipe e modelo do Projeto 1
```

## Como executar no Windows

É necessário Python 3 e um compilador C (GCC, Clang ou Zig). No workspace original foi baixado um Zig portátil em `tools/`; o compilador não acompanha o repositório nem o ZIP de entrega. Em um clone novo, use `--compiler gcc`, `--compiler clang` ou informe o caminho para seu `zig.exe`. A origem e o hash do compilador usado nas medições estão em `tools/zig-download.json`.

```powershell
# Testar usando o Zig presente no workspace:
python scripts/executar.py --somente-testes

# Reexecutar a avaliação completa (aproximadamente 7 minutos nesta máquina):
python scripts/executar.py --repeticoes 3

# Alternativamente, indicar seu compilador:
python scripts/executar.py --compiler gcc --repeticoes 3
python scripts/executar.py --compiler "C:/caminho/zig.exe" --somente-testes

# Regenerar o relatório a partir dos resultados:
python -m pip install -r requirements.txt
python scripts/gerar_relatorio.py
```

O script aponta os caches do Zig e do Matplotlib para `tmp/` dentro do projeto. A avaliação interrompe em caso de erro de compilação, falha funcional ou divergência na contagem de registros. Reexecutar sobrescreve os resultados e logs das rodadas com o mesmo nome; preserve uma cópia se quiser comparar ambientes.

### Compilação direta de um benchmark

```powershell
gcc -O2 -std=c11 -DORDEM_INDICE=16 -I solucao/source -I scripts solucao/source/Benchmark/bench.c -o bench.exe
./bench.exe 12345
```

Execute o programa em uma pasta de testes: ele cria seus próprios arquivos `bench_idx.dat`, `bench_si.dat`, `resultados.csv` e `metricas.csv`, e remove arquivos anteriores com esses nomes.

## Linux/macOS

```bash
python3 scripts/executar.py --compiler gcc --repeticoes 3
python3 -m pip install -r requirements.txt
python3 scripts/gerar_relatorio.py
```

O runner habilita `_POSIX_C_SOURCE=200809L` em sistemas POSIX. O código usa `ftruncate` nesse ambiente e `_chsize_s` no Windows. A execução e os resultados desta entrega foram verificados no Windows; a ramificação POSIX foi disponibilizada para reprodução, sem alegar uma medição em Linux.

## Dados e unidades

- `results/benchmark.csv`: tempo em ms; `createe` mede N inserções, `retrieve`/`update` medem 200 operações, `deletee` mede 30. `queryAll`/`queryBy` registram a média de cinco execuções incluindo liberação.
- `results/resumo.csv`: mediana, mínimo e máximo das três sementes, por versão, ordem, operação, modo e N.
- `results/metricas.csv`: altura em arestas, nós vivos, folhas e splits após o povoamento; empréstimos, fusões, splits e novos nós durante o lote de exclusões. Na solução, os dois últimos são sempre zero.
- `results/estudo.csv`: cinco listagens por alternativa (média em ms), ou 100.000 buscas em RAM (tempo total em ms); comparações por busca incluem a descida e a varredura da folha.
- `results/raw/`: CSVs e logs de cada execução; `results/testes.json`: resultado dos testes nas cinco ordens; `results/ambiente.json`: compilador, máquina e horários.

Medianas entre sementes são uma descrição das medições, não intervalos de confiança. No experimento de listagem, o controle `somente_folhas` não retorna registros completos e não deve ser considerado substituto funcional de `queryAll`.

## Contratos e limites

Os registros têm tamanho fixo e chaves únicas. Como na biblioteca fornecida, a inserção não impede automaticamente chaves duplicadas. A exclusão por swap altera a ordem física, enquanto `queryAll` preserva a ordem por chave. O retorno de `buscarArv` é uma cópia: a atualização do offset utiliza o valor real da folha.

A assinatura `void deletee(...)` foi mantida: `errno=0` indica sucesso; `ENOENT` indica ausência; erros de I/O/entrada/alocação são sinalizados em `errno`. A solução verifica offsets e faz uma tentativa de restauração em falha de gravação/truncamento. Não implementa journal, concorrência nem atomicidade contra queda do processo.

O `.idx` continua sendo salvo ao fechar, fora das métricas de CRUD; isso custa O(N). A abertura pode reconstruir o índice; o benchmark não mede esse custo. O formato de structs e offsets depende da ABI. O `persistAll` original não atualiza sozinho o índice após uma regravação arbitrária; essa rotina não é usada pela exclusão corrigida.

## Referência da equipe

Os nomes, o curso e a apresentação institucional foram obtidos de [AeG-Projeto-Arvores-Balanceadas](https://github.com/leohsm/AeG-Projeto-Arvores-Balanceadas), commit `bba1ef7f04c75574ad09a97a458dd82c35f515f7`. O código das árvores do Projeto 1 não é a base desta solução. A base é o framework B+ fornecido no ZIP da disciplina.

Uso de IA na implementação e avaliação permitido pelo item 7 do enunciado. O envio ao Moodle deve ser feito pela equipe; nenhum arquivo foi publicado em serviço externo.
