"""Gera gráficos, tabelas consolidadas, relatório Markdown e PDF a partir de medições reais."""
from pathlib import Path
from collections import defaultdict
import csv, json, statistics, math, html, os
os.environ.setdefault('MPLCONFIGDIR', str(Path(__file__).resolve().parents[1]/'tmp/matplotlib'))
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, Image, PageBreak, HRFlowable, Preformatted, KeepTogether
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_JUSTIFY
from reportlab.lib.pagesizes import A4

ROOT=Path(__file__).resolve().parents[1]
RESULTS=ROOT/'results'; OUT=ROOT/'output/pdf'; PLOTS=ROOT/'plots'
ORDERS=[4,8,16,32,64]; NS=[1000,5000,20000,50000,100000]
NAVY='#1D3557'; GREEN='#22755F'; ORANGE='#C77725'; BLUE='#457B9D'

def load(name):
    with (RESULTS/name).open(encoding='utf-8',newline='') as f:return list(csv.DictReader(f))
def fmt(value,d=3):return f'{value:,.{d}f}'.replace(',','_').replace('.',',').replace('_','.')
def integer(value):return fmt(value,0)
def aggregate(rows,keys,field):
    out=defaultdict(list)
    for r in rows:out[tuple(str(r[k]) for k in keys)].append(float(r[field]))
    return {k:(statistics.median(v),min(v),max(v)) for k,v in out.items()}

def main():
    OUT.mkdir(parents=True,exist_ok=True);PLOTS.mkdir(exist_ok=True)
    rows=load('benchmark.csv'); metrics=load('metricas.csv'); study=load('estudo.csv')
    meta=json.loads((RESULTS/'ambiente.json').read_text(encoding='utf-8'))
    tests=json.loads((RESULTS/'testes.json').read_text(encoding='utf-8'))
    b=aggregate(rows,['versao','ordem','operacao','modo','N'],'tempo_ms')
    m={f:aggregate(metrics,['versao','ordem','N'],f) for f in ['altura','nos','folhas','splits_folha','splits_interno','nos_criados','emprestimos_delete','fusoes_delete','splits_delete','nos_criados_delete']}
    s=aggregate(study,['ordem','operacao','layout','modo'],'tempo_ms')
    c=aggregate(study,['ordem','operacao','layout','modo'],'comparacoes_por_busca')
    def B(variant,order,op,mode,n):return b[(variant,str(order),op,mode,str(n))][0]
    def M(field,order,n=100000):return m[field][('solucao',str(order),str(n))][0]
    def S(order,op,layout,mode):return s[(str(order),op,layout,mode)][0]
    def C(order):return c[(str(order),'buscarArv','aleatorio','somente_indice')][0]
    with (RESULTS/'resumo.csv').open('w',encoding='utf-8',newline='') as f:
        w=csv.writer(f);w.writerow(['versao','ordem','operacao','modo','N','mediana_ms','min_ms','max_ms'])
        for key,vals in b.items():w.writerow([*key,*vals])
    plt.rcParams.update({'font.family':'DejaVu Sans','font.size':10,'axes.spines.top':False,'axes.spines.right':False,'axes.labelcolor':NAVY,'text.color':NAVY,'figure.facecolor':'white','axes.grid':True,'grid.alpha':.18})
    def save(fig,name):fig.savefig(PLOTS/name,dpi=220,bbox_inches='tight');plt.close(fig)
    fig,ax=plt.subplots(figsize=(8,3.6))
    for variant,mode,label,color in [('baseline','com_indice','Índice original',BLUE),('baseline','sem_indice','Sem índice',ORANGE),('solucao','com_indice','Índice incremental',GREEN)]:
        vals=[b[(variant,'4','deletee',mode,str(n))] for n in NS]
        ys=[v[0] for v in vals]
        ax.errorbar(NS,ys,yerr=[[v[0]-v[1] for v in vals],[v[2]-v[0] for v in vals]],label=label,color=color,marker='o',capsize=3)
    ax.set_xscale('log');ax.set_yscale('log');ax.set_xlabel('N registros');ax.set_ylabel('30 exclusões (ms, escala log)');ax.legend(frameon=False);save(fig,'deletee.png')
    fig,ax=plt.subplots(figsize=(8,3.5))
    for op,count,label,color in [('createe',None,'createe / registro',BLUE),('retrieve',200,'retrieve / busca',ORANGE),('deletee',30,'deletee / exclusão',GREEN)]:
        ax.plot(NS,[1000*B('solucao',4,op,'com_indice',n)/(count or n) for n in NS],marker='o',color=color,label=label)
    ax.set_xscale('log');ax.set_xlabel('N registros');ax.set_ylabel('Tempo médio por operação (µs)');ax.legend(frameon=False);save(fig,'por_operacao.png')
    fig,axes=plt.subplots(1,2,figsize=(8,3.2))
    axes[0].plot(ORDERS,[M('altura',o) for o in ORDERS],marker='o',color=BLUE);axes[0].set_xlabel('Ordem');axes[0].set_ylabel('Altura em arestas');axes[0].set_xticks(ORDERS)
    axes[1].plot(ORDERS,[C(o) for o in ORDERS],marker='o',color=ORANGE);axes[1].set_xlabel('Ordem');axes[1].set_ylabel('Comparações por busca em RAM');axes[1].set_xticks(ORDERS);fig.tight_layout();save(fig,'ordem_estrutura.png')
    fig,axes=plt.subplots(1,2,figsize=(8,3.3))
    for ax,ops in zip(axes,[['createe'],['retrieve','deletee']]):
        for op in ops:
            count=100000 if op=='createe' else 200 if op=='retrieve' else 30
            ax.plot(ORDERS,[1000*B('solucao',o,op,'com_indice',100000)/count for o in ORDERS],marker='o',label=op)
        ax.set_xlabel('Ordem');ax.set_ylabel('Tempo médio por operação (µs)');ax.set_xticks(ORDERS);ax.legend(frameon=False)
    fig.tight_layout();save(fig,'ordem_tempos.png')
    fig,ax=plt.subplots(figsize=(8,3.3))
    modes=['indice_lista','sequencial_lista','sequencial_buffer','somente_folhas'];labels=['Índice + lista','Sequencial + lista','Sequencial + buffer','Só folhas (sem dados)']
    import numpy as np
    x=np.arange(4)
    for shift,layout,label,color in [(-.18,'aleatorio','Arquivo aleatório',BLUE),(.18,'ordenado','Arquivo ordenado',GREEN)]:
        ax.bar(x+shift,[S(4,'queryAll',layout,mo) for mo in modes],width=.36,label=label,color=color)
    ax.set_yscale('log');ax.set_xticks(x,labels,rotation=10);ax.set_ylabel('Uma listagem (ms, escala log)');ax.legend(frameon=False);save(fig,'queryAll_controle.png')

    speed=B('baseline',4,'deletee','com_indice',100000)/B('solucao',4,'deletee','com_indice',100000)
    baseline_ratio=B('baseline',4,'deletee','com_indice',100000)/B('baseline',4,'deletee','com_indice',1000)
    fixed_ratio=B('solucao',4,'deletee','com_indice',100000)/B('solucao',4,'deletee','com_indice',1000)
    qratio=S(4,'queryAll','aleatorio','indice_lista')/S(4,'queryAll','aleatorio','sequencial_lista')
    best=min(ORDERS,key=lambda o:S(o,'buscarArv','aleatorio','somente_indice'))
    styles=getSampleStyleSheet()
    styles.add(ParagraphStyle(name='BodyPt',fontName='Helvetica',fontSize=10,leading=14.5,alignment=TA_JUSTIFY,spaceAfter=9,textColor=colors.HexColor(NAVY)))
    styles.add(ParagraphStyle(name='TitlePt',fontName='Helvetica-Bold',fontSize=22,leading=29,alignment=TA_CENTER,textColor=colors.HexColor(NAVY),spaceAfter=15))
    styles.add(ParagraphStyle(name='CenterPt',fontName='Helvetica',fontSize=11,leading=18,alignment=TA_CENTER,spaceAfter=5,textColor=colors.HexColor(NAVY)))
    styles.add(ParagraphStyle(name='H1Pt',fontName='Helvetica-Bold',fontSize=15,leading=19,textColor=colors.HexColor(NAVY),spaceAfter=14))
    styles.add(ParagraphStyle(name='H2Pt',fontName='Helvetica-Bold',fontSize=11,leading=15,textColor=colors.HexColor(NAVY),spaceBefore=9,spaceAfter=8))
    styles.add(ParagraphStyle(name='SmallPt',fontName='Helvetica',fontSize=8,leading=11,textColor=colors.HexColor(NAVY),spaceAfter=7))
    styles.add(ParagraphStyle(name='CellPt',fontName='Helvetica',fontSize=8,leading=11,textColor=colors.HexColor(NAVY)))
    story=[];md=[]
    def p(text,small=False):story.append(Paragraph(text,styles['SmallPt' if small else 'BodyPt']));md.append(text.replace('<b>','**').replace('</b>','**').replace('<i>','').replace('</i>',''))
    def h(text):story.append(Paragraph(text,styles['H1Pt']));md.append('## '+text)
    def sub(text):story.append(Paragraph(text,styles['H2Pt']));md.append('### '+text)
    def page():story.append(PageBreak())
    def table(headers,data,widths=None):
        content=[[Paragraph(html.escape(str(cell)),styles['CellPt']) for cell in row] for row in [headers,*data]]
        t=Table(content,colWidths=widths,repeatRows=1,hAlign='LEFT')
        t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),colors.HexColor('#E3ECF2')),('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.white,colors.HexColor('#F5F7F9')]),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),5),('BOTTOMPADDING',(0,0),(-1,-1),5),('LINEBELOW',(0,0),(-1,0),.6,colors.HexColor(BLUE))]))
        story.append(t);story.append(Spacer(1,10))
        md.append('| '+' | '.join(map(str,headers))+' |');md.append('| '+' | '.join(['---']*len(headers))+' |')
        for row in data:md.append('| '+' | '.join(map(str,row))+' |')
    def plot(name,caption,width=475):
        from PIL import Image as PILImage
        with PILImage.open(PLOTS/name) as im:w,hh=im.size
        story.append(Image(str(PLOTS/name),width=width,height=width*hh/w));p(caption,True);md.append(f'![{caption}](../../plots/{name})')

    story.append(Spacer(1,28))
    for text in ['MINISTÉRIO DA EDUCAÇÃO','UNIVERSIDADE TECNOLÓGICA FEDERAL DO PARANÁ','CÂMPUS MEDIANEIRA','CURSO: CIÊNCIA DA COMPUTAÇÃO','DISCIPLINA: ÁRVORES E GRAFOS']:
        story.append(Paragraph(text,styles['CenterPt']))
    story.append(Spacer(1,42));story.append(HRFlowable(width='100%',thickness=2,color=colors.HexColor(NAVY),spaceAfter=20))
    story.append(Paragraph('PROJETO PRÁTICO 2<br/>ÁRVORES MULTICAMINHOS',styles['TitlePt']))
    story.append(Paragraph('Exclusão incremental, ordem da árvore B+<br/>e avaliação crítica de queryAll',styles['CenterPt']))
    story.append(HRFlowable(width='100%',thickness=1,color=colors.HexColor(BLUE),spaceBefore=20,spaceAfter=35))
    for text in ['EQUIPE','Erik Mazzuco','Letícia Moro','Leonardo Herrero']:
        story.append(Paragraph(text,styles['CenterPt']))
    story.append(Spacer(1,32))
    for text in ['Período letivo: 2026.2','Prazo do enunciado: 04 de outubro de 2026','Medianeira - PR','Medições realizadas em '+meta['inicio'][:10]]:
        story.append(Paragraph(text,styles['CenterPt']))
    md.extend(['# Projeto Prático 2 - Árvores Multicaminhos','**UTFPR - Câmpus Medianeira | Ciência da Computação | Árvores e Grafos | 2026.2**','**Equipe:** Erik Mazzuco, Letícia Moro, Leonardo Herrero','**Prazo do enunciado:** 04/10/2026'])
    page();h('1. Objetivos e metodologia')
    p('O projeto avalia os itens 4, 5 e 6 do enunciado: eliminar a reconstrução do índice em cada exclusão, medir o efeito de ORDEM_INDICE e explicar por que percorrer as folhas da B+ não torna a listagem completa automaticamente mais rápida. A implementação parte do FrameworkPersistencia_com_indice.zip fornecido pela disciplina; o repositório do Projeto 1 foi usado para os dados da equipe e a organização do documento.')
    p(f'<b>Resultado principal:</b> em N = 100.000, as 30 exclusões com ordem 4 passaram de {fmt(B("baseline",4,"deletee","com_indice",100000))} ms para {fmt(B("solucao",4,"deletee","com_indice",100000))} ms, uma aceleração de {fmt(speed,1)} vezes nesta máquina. A árvore foi mantida e nenhuma exclusão criou novos nós ou executou splits.')
    table(['Operação','Trabalho cronometrado'],[['createe','N inserções em ordem embaralhada'],['retrieve / update','200 buscas / 200 atualizações de chaves existentes'],['deletee','30 exclusões de chaves distintas'],['queryAll / queryBy','Média de 5 listagens / filtros; inclui liberação da saída']],[125,350])
    p(f'N = 1.000, 5.000, 20.000, 50.000 e 100.000. Ordens = 4, 8, 16, 32 e 64. Foram executadas {meta["repeticoes"]} rodadas, com sementes '+', '.join(map(str,meta['sementes']))+'. Cada combinação começa com arquivos novos. O baseline é a exclusão original na ordem 4; a solução usa a exclusão incremental nas cinco ordens. Os dois modos recebem as mesmas chaves dentro de cada rodada.')
    p('As tabelas mostram medianas das três rodadas. Quando indicado, mínimo e máximo representam variação entre sementes e execuções, sem interpretação de intervalo de confiança. Tempos absolutos do relatório original da disciplina foram medidos em outro ambiente e não foram misturados aos novos resultados.')
    sub('Ambiente e controle experimental')
    p(html.escape(meta['sistema'])+'; '+html.escape(meta['processador'])+f'; {meta["cpu_logicas"]} CPUs lógicas. Compilador Zig {meta["versao"]} (zig cc/Clang), opções -O2 -std=c11. Relógio monotônico QueryPerformanceCounter. Registro Pessoa de 40 bytes; arquivos locais na unidade C:. As execuções foram sequenciais.')
    p('O cache do sistema operacional permanece ativo. Não há fsync nem medição de durabilidade transacional. Abrir/fechar e serializar o arquivo .idx ficam fora das medições de CRUD. A instrumentação é igual no baseline e na solução; altura e nós são calculados fora do intervalo de createe. O bench.c mantém a ordem com índice antes de sem índice, uma limitação metodológica.',True)

    page();h('2. Implementação da exclusão incremental')
    p('A exclusão original lê todos os registros, materializa os sobreviventes, trunca e regrava o arquivo e recria a B+ inserindo todos novamente. Para ordem fixa, isso soma O(N log N) por exclusão, além de O(N) de I/O e memória auxiliar. O custo elevado vem dessa estratégia, não da remoção balanceada da biblioteca.')
    p('Foi escolhida a solução <b>swap com o último registro</b>. Os registros possuem tamanho fixo e chaves únicas, como na carga fornecida. A rotina localiza o offset pela B+, lê o último registro, sobrescreve a posição removida, sincroniza o buffer de stdio e trunca exatamente um registro. Em seguida, corrige o offset do registro movido e chama removerArv para a chave excluída.')
    table(['Offset','Antes','Após remover B'],[['0','A','A'],['40','B','D (movido de 120)'],['80','C','C'],['120','D','Fim do arquivo: 120 bytes']],[90,150,235])
    p('Nesse exemplo, as entradas A → 0 e C → 80 permanecem válidas. Somente D passa de 120 para 40, e B desaparece do índice. Quando o alvo já é o último registro, não há cópia nem atualização de sobrevivente. Uma chave inexistente não modifica arquivo ou índice.')
    sub('Detalhes que garantem a consistência')
    p('buscarArv devolve uma <b>cópia</b> do valor, portanto modificar esse retorno não atualizaria o índice. A rotina encontra a folha do registro movido e altera seu valor long diretamente. A atualização ocorre antes de removerArv, pois empréstimos e fusões podem reorganizar as folhas. Nenhuma inserção é necessária: inserirArv não implementa substituição de uma chave existente e poderia criar duplicatas.')
    p('O arquivo é truncado com _chsize_s no Windows e ftruncate em POSIX, após fflush. As leituras, alinhamento dos offsets e correspondência do último registro são verificados. A assinatura pública void é preservada; errno informa sucesso, ausência ou falha. Há tentativa de restaurar o registro anterior se a escrita/truncamento falhar, sem garantia transacional para falhas repetidas ou interrupções do processo.')
    sub('Complexidade e consequências')
    p('Para ordem m e altura h, as buscas lineares nos nós custam O(mh), com h = O(log_m N). Em ordem fixa, a exclusão é O(log N) no índice e O(1) de volume de dados movimentado: até duas leituras e uma escrita de registro, mais truncamento. A memória auxiliar é O(tamanhoRegistro), independente de N. A ordem física dos registros muda; queryAll continua retornando ordem de chave.')

    page();h('3. Resultados de deletee (item 4)')
    table(['N','Original com índice\n(ms)','Incremental\n(ms)','Sem índice\n(ms)','Aceleração\noriginal/incr.'],[[integer(n),fmt(B('baseline',4,'deletee','com_indice',n)),fmt(B('solucao',4,'deletee','com_indice',n)),fmt(B('baseline',4,'deletee','sem_indice',n)),fmt(B('baseline',4,'deletee','com_indice',n)/B('solucao',4,'deletee','com_indice',n),1)+'x'] for n in NS],[70,110,100,100,95])
    plot('deletee.png','Figura 1. Medianas de 30 exclusões por rodada; barras mostram mínimo e máximo. Ordem 4, eixos logarítmicos.')
    p(f'Aumentar N de 1.000 para 100.000 multiplicou o tempo original por {fmt(baseline_ratio,1)}, enquanto o incremental variou por um fator de {fmt(fixed_ratio,2)}. Essa diferença é compatível com retirar a varredura e a reconstrução completas. O tempo de truncamento e I/O introduz custo fixo e variação; a curva medida não constitui uma prova de complexidade assintótica.')
    p('A versão sem índice ainda varre e regrava todo o arquivo. A solução incremental atua diretamente sobre a posição encontrada pela B+. A comparação utiliza o baseline executado nesta máquina, mantendo os números históricos de 7,85 s e 0,18 s apenas como motivação do enunciado.')

    page();h('4. Comparação por operação e validação')
    plot('por_operacao.png','Figura 2. Tempos normalizados da solução na ordem 4: createe/N, retrieve/200 e deletee/30, em microssegundos.')
    p('O número de inserções varia com N, enquanto as buscas e exclusões usam lotes fixos. Comparar diretamente os tempos totais de createe e deletee produziria uma conclusão incorreta. A normalização mostra o custo médio de cada operação: a exclusão mantém comportamento de operação pontual, com constantes de I/O maiores que uma busca simples.')
    table(['N = 100.000','Original com índice (ms)','Solução, ordem 4 (ms)','Sem índice original (ms)'],[[op,fmt(B('baseline',4,op,'com_indice',100000)),fmt(B('solucao',4,op,'com_indice',100000)),fmt(B('baseline',4,op,'sem_indice',100000))] for op in ['createe','retrieve','update','deletee','queryAll','queryBy']],[85,130,130,130])
    sub('Verificação funcional')
    p('Os testes em C validaram as cinco ordens contra um modelo de referência: arquivo vazio, chave ausente, primeiro/último/único registro, 3.000 inserções e remoção de todos os registros em ordem aleatória, 5.000 operações mistas, alteração de chave, reabertura e reconstrução quando .idx está ausente. Os checks conferem conteúdo completo, tamanho do arquivo, offsets, unicidade, ordenação, ocupação mínima, roteamento dos separadores, pais e profundidade uniforme das folhas.')
    p('Cada deletee é também verificado quanto à permanência do descritor da árvore e à ausência de novos nós e splits. O benchmark interrompe a execução se a contagem N - 30 divergir. Todos os testes e rodadas concluíram com sucesso. As quantidades de checks são verificações individuais de invariantes, não casos de teste independentes.',True)

    page();h('5. Ordem, altura e splits (item 5)')
    p('ORDEM_INDICE passou a aceitar sobrescrita pelo compilador, com valor padrão 4. A instrumentação conta cada criarNoh, cada divisão de folha e de nó interno, além de empréstimos e fusões. A altura e a quantidade de nós vivos são calculadas por percurso estrutural fora do tempo medido. Altura em arestas: uma árvore formada por uma folha tem altura zero.')
    table(['Ordem','Altura','Nós vivos','Folhas','Splits folha','Splits internos'],[[o,integer(M('altura',o)),integer(M('nos',o)),integer(M('folhas',o)),integer(M('splits_folha',o)),integer(M('splits_interno',o))] for o in ORDERS],[55,60,90,90,90,90])
    p('Tabela: medianas da estrutura imediatamente após inserir 100.000 registros embaralhados. Splits contam eventos de divisão; criar uma raiz também aumenta nós criados, mas não é um split adicional. Nós criados são cumulativos, enquanto nós vivos refletem a estrutura atual. A ocupação mínima didática do framework, (ordem - 1)/2, foi preservada para folhas e nós internos.',True)
    plot('ordem_estrutura.png','Figura 3. Maior ordem reduz a altura; a busca linear em vetores maiores pode aumentar as comparações por consulta.')
    p('encontrarFolha percorre os separadores linearmente e buscarArv também varre a folha. Reduzir a altura não elimina esse trabalho. Para separar CPU do custo de acessar o arquivo, o experimento complementar executa 100.000 buscarArv somente no índice em memória e conta todas as chamadas de comparação, incluindo nós internos e folhas.')

    page();h('6. Ordem e desempenho das operações')
    table(['Ordem','createe\nN inserções (ms)','retrieve\n200 buscas (ms)','deletee\n30 exclusões (ms)','100 mil buscas\nem RAM (ms)','Comparações\npor busca'],[[o,fmt(B('solucao',o,'createe','com_indice',100000)),fmt(B('solucao',o,'retrieve','com_indice',100000)),fmt(B('solucao',o,'deletee','com_indice',100000)),fmt(S(o,'buscarArv','aleatorio','somente_indice')),fmt(C(o),2)] for o in ORDERS],[45,95,90,95,85,65])
    plot('ordem_tempos.png','Figura 4. Medianas por operação em N = 100.000; a altura menor não garante a menor latência em todos os CRUDs.',width=425)
    p(f'A menor mediana de busca apenas em RAM ocorreu na ordem {best}, nesta carga. A ordem 4 tem mais níveis, mais nós e mais alocações. Nas ordens maiores, cada nó concentra mais chaves, reduzindo níveis e splits, mas aumentando varreduras e deslocamentos de ponteiros nas inserções. As buscas com I/O podem esconder diferenças de CPU percebidas no experimento em RAM.')
    p('A escolha depende da carga: altura, uso de memória, frequência de escrita, acesso ao arquivo e comparação das chaves. As ordens 4 a 64 foram medidas com três sementes, sem assumir que a maior é sempre melhor. Mais repetições e cargas com outras chaves seriam necessárias para estabelecer uma configuração universal. Uma evolução possível é busca binária nos vetores dos nós, avaliando novamente o custo de comparação e localidade.')
    sub('Efeito da exclusão na estrutura')
    table(['Ordem','Empréstimos\n30 deletes','Fusões\n30 deletes','Splits\n30 deletes','Nós criados\n30 deletes'],[[o,integer(M('emprestimos_delete',o)),integer(M('fusoes_delete',o)),integer(M('splits_delete',o)),integer(M('nos_criados_delete',o))] for o in ORDERS],[65,115,100,95,100])
    p('As 30 exclusões representam uma pequena fração de N. Os testes que removem todos os 3.000 registros também exercitam fusões, empréstimos e redução da raiz. A ausência de splits e novos nós confirma que deletee não reconstrói o índice.',True)

    page();h('7. Análise crítica de queryAll (item 6)')
    p('A B+ contém uma <b>lista encadeada de folhas com chaves e offsets</b>; os registros completos permanecem no arquivo. percorrerArv desce uma vez à primeira folha e segue proximo, com custo O(h + N). O visitante de queryAll executa fseek + fread para cada offset e aloca um registro e um nó da lista. Um arquivo inserido em ordem aleatória exige leituras em ordem de chave que não correspondem à ordem física.')
    p('A varredura sem índice usa rewind e fread consecutivos. Ambas as versões precisam retornar N registros: o índice não reduz a quantidade de dados de uma listagem completa. O ganho da lista de folhas é evitar N buscas desde a raiz e entregar as chaves ordenadas; ele não garante acesso sequencial ao arquivo nem remove o custo de materialização.')
    table(['N','Original com índice (ms)','Incremental com índice (ms)','Sem índice original (ms)'],[[integer(n),fmt(B('baseline',4,'queryAll','com_indice',n)),fmt(B('solucao',4,'queryAll','com_indice',n)),fmt(B('baseline',4,'queryAll','sem_indice',n))] for n in NS],[75,130,140,130])
    sub('Dois fatores de confusão do benchmark fornecido')
    p('Primeiro, o modo com índice devolve pDLista com duas alocações por registro; o sem índice devolve um buffer contínuo que cresce com realloc. Parte da diferença pertence à representação da saída. Segundo, a versão indexada entrega ordem de chave, enquanto a versão sem índice mantém a ordem física de inserção. Não oferecem exatamente a mesma ordenação.')
    p('Por isso foi adicionado um controle que usa o mesmo arquivo e separa quatro alternativas: índice com lista; varredura sequencial com lista; varredura sequencial com buffer; percurso das folhas sem ler registros. As três primeiras incluem materialização e liberação. A quarta serve apenas para medir o percurso e não é uma implementação equivalente de queryAll. Cada alternativa é aquecida e medida cinco vezes, nas três sementes.')
    p('queryAll foi mantida ordenada. A função adicional queryAllSequencial oferece a listagem em ordem física com a mesma representação pDLista. Ela é adequada quando o chamador não exige ordenação. Para exigir ordem de chave após a leitura sequencial, seria necessário ordenar os registros, acrescentando trabalho que este controle não mede.')

    page();h('8. Experimento controlado de listagem')
    table(['Alternativa\nN = 100.000, ordem 4','Arquivo aleatório (ms)','Arquivo ordenado (ms)'],[[label,fmt(S(4,'queryAll','aleatorio',mode)),fmt(S(4,'queryAll','ordenado',mode))] for mode,label in [('indice_lista','Índice + lista'),('sequencial_lista','Sequencial + lista'),('sequencial_buffer','Sequencial + buffer'),('somente_folhas','Somente percurso de folhas')]], [225,125,125])
    plot('queryAll_controle.png','Figura 5. Mesmo arquivo e cache aquecido. “Só folhas” não retorna os registros e serve como controle do custo de percurso.')
    p(f'Com o arquivo aleatório, índice + lista ficou {fmt(qratio,1)} vezes mais lento que sequencial + lista, mantendo a mesma representação. A comparação entre as duas leituras sequenciais evidencia o custo adicional da lista e das alocações. O arquivo ordenado testa a localidade: os offsets crescem em ordem de chave, mas continuam existindo N fseek/fread e a materialização de N registros.')
    p('Os controles sustentam a explicação pela combinação de padrão de acesso e construção da saída, não por uma falha no encadeamento das folhas. Como as leituras estão em cache, os tempos não representam diretamente latência de mídia física; chamadas de stdio, buffering, alocação e efeitos de cache de CPU também participam do custo observado.')
    p('A exclusão por swap pode desfazer a ordenação física prévia. Portanto, mesmo um arquivo inicialmente ordenado pode perder localidade com exclusões sucessivas. Para consultas por faixa, a B+ pode visitar somente as chaves do intervalo; para queryAll, que precisa de todas, a varredura sequencial tende a ser adequada quando não há exigência de ordenação. O filtro por idade de queryBy também exige os registros completos e não é acelerado por um índice apenas de CPF.')

    page();h('9. Conclusões, limites e reprodução')
    p(f'<b>Item 4:</b> a troca com o último registro removeu a reconstrução do índice, manteve os offsets dos demais registros e obteve aceleração de {fmt(speed,1)} vezes para 30 exclusões em N = 100.000 e ordem 4. O custo algorítmico passa a ser de operação pontual na árvore, com I/O de volume constante. A curva medida é compatível com essa mudança e deve ser interpretada junto às constantes de I/O.')
    p(f'<b>Item 5:</b> a ordem altera altura, quantidade de nós, splits e custo de comparação. Aumentar a ordem de 4 para 64 reduziu a altura mediana de {integer(M("altura",4))} para {integer(M("altura",64))}. A menor mediana de busca em RAM ocorreu na ordem {best}; reduzir altura isoladamente não determina o melhor desempenho de todos os CRUDs.')
    p('<b>Item 6:</b> as folhas armazenam offsets, não cópias completas dos registros. O percurso eficiente da B+ não evita a leitura de todos os dados e pode produzir acesso disperso. A representação da saída também penaliza a comparação original. Os controles isolam esses fatores, preservando a consulta ordenada e oferecendo uma alternativa sequencial explícita.')
    sub('Limites da solução e das medições')
    p('A solução pressupõe registros de tamanho fixo e chaves únicas. A biblioteca fornecida não bloqueia duplicatas automaticamente. O formato binário original depende da struct e de sizeof(long), portanto arquivos .idx não são portáveis entre ABIs. Não há controle de concorrência, journal ou recuperação de falhas: o índice é mantido em memória e salvo ao fechar. Fechar continua O(N), e carregarArvore reinserindo os pares continua O(N log N); esses custos não foram transferidos para a métrica de deletee nem incluídos nela.')
    p('Três sementes, uma máquina, ordem fixa dos modos, cache ativo e registros de 40 bytes limitam a generalização. O persistAll original não mantém sozinho o índice consistente após uma regravação arbitrária; esta entrega usa-o apenas no baseline e não o emprega na exclusão corrigida. A otimização não fornece garantias transacionais de produção.')
    sub('Reprodução e arquivos entregues')
    p('1. Compilar e testar: python scripts/executar.py --compiler CAMINHO_DO_COMPILADOR --somente-testes.<br/>2. Reexecutar a avaliação: python scripts/executar.py --compiler CAMINHO_DO_COMPILADOR --repeticoes 3.<br/>3. Gerar tabelas, gráficos e relatório: python scripts/gerar_relatorio.py.',True)
    p('O compilador pode ser gcc/clang ou zig.exe. No Linux/macOS, utilize GCC ou Clang e execute o script em Python 3. Dependências do relatório: matplotlib, reportlab e Pillow. Os caminhos dos executáveis são gerados pelo script. Consulte README.md para os comandos completos e a descrição dos CSVs.',True)
    table(['Local','Conteúdo'],[['solucao/source/','Framework com deletee incremental e queryAllSequencial'],['baseline/source/','Algoritmos originais com o mesmo relógio e instrumentação'],['tests/ e benchmarks/','Validação em C e experimento controlado'],['results/','CSVs brutos/consolidados, ambiente e logs'],['plots/ e output/pdf/','Gráficos e relatório PDF/Markdown']],[150,325])

    page();h('10. Referências e rastreabilidade')
    p('[1] UTFPR - DACOM-MD. <i>Projeto Prático 2 - Árvores Multi-caminhos.</i> Período 2026.2. Documento fornecido: 21_Projeto_Arvores_Multicaminhos-1.pdf. Requisitos dos itens 4, 5, 6 e 8; prazo 04/10/2026.')
    p('[2] UTFPR. <i>Framework de Persistência com Índice (Árvore B+).</i> Pacote fornecido pelo usuário: FrameworkPersistencia_com_indice.zip. Recurso indicado no enunciado: <link href="https://moodle.utfpr.edu.br/mod/resource/view.php?id=2178366" color="#457B9D">moodle.utfpr.edu.br/mod/resource/view.php?id=2178366</link>. A base efetivamente usada foi o ZIP local; não foi necessário autenticar no Moodle.')
    p('[3] <i>Relatório de Desempenho - Índice em Árvore B+ na Biblioteca de Persistência.</i> Relatorio_Desempenho_IndiceBMais.pdf, incluído no framework. Utilizado para a análise crítica e para identificar as limitações da comparação inicial. Seus tempos históricos não substituem as medições desta entrega.')
    origin=json.loads((ROOT/'referencia/origem.json').read_text(encoding='utf-8'))
    p('[4] HERRERO, Leonardo; MAZZUCO, Erik; MORO, Letícia. <i>AeG-Projeto-Arvores-Balanceadas.</i> README.md e modelo de relatório. <link href="https://github.com/leohsm/AeG-Projeto-Arvores-Balanceadas" color="#457B9D">github.com/leohsm/AeG-Projeto-Arvores-Balanceadas</link>. Commit consultado: '+origin['commit']+'. Os nomes e a afiliação foram obtidos dessa referência; o código do Projeto 1 não foi utilizado como biblioteca B+.')
    p('[5] Zig Software Foundation. Compilador Zig '+meta['versao']+'. Distribuição oficial: <link href="https://ziglang.org/download/" color="#457B9D">ziglang.org/download/</link>. Arquivo baixado com verificação de SHA-256 conforme tools/zig-download.json.')
    sub('Evidências verificáveis')
    p('Os CSVs preservam versão, ordem, N, modo, repetição e semente. results/resumo.csv apresenta mediana, mínimo e máximo. results/metricas.csv separa splits de folhas e internos e confirma novos nós/splits durante deletee. results/estudo.csv registra os controles de listagem e as comparações de busca em RAM. Os logs de execução e compilação acompanham a entrega.')
    p('O diretório original/ preserva o material recebido; baseline/ e solucao/ são cópias de trabalho. O arquivo alteracoes.patch explicita as alterações entre original e solução. manifest.json registra hashes SHA-256 dos arquivos-fonte originais e do ZIP, permitindo confirmar a origem do trabalho.')
    sub('Uso de ferramentas de IA')
    p('Foi utilizado apoio de IA na implementação, na elaboração dos testes, na execução da avaliação e na organização do relatório, conforme permitido pelo item 7 do enunciado. Os resultados apresentados foram produzidos pela execução dos programas e não por estimativa textual. Todos os testes funcionais e verificações estruturais concluíram com sucesso.')
    for t in tests:p(html.escape(t),True)
    def footer(canvas,doc):
        if doc.page==1:return
        canvas.saveState();canvas.setStrokeColor(colors.HexColor('#D6DEE6'));canvas.line(55,42,540,42)
        canvas.setFont('Helvetica',8);canvas.setFillColor(colors.HexColor(NAVY));canvas.drawString(55,29,'UTFPR - Árvores e Grafos | Projeto Prático 2 | 2026.2');canvas.drawRightString(540,29,str(doc.page));canvas.restoreState()
    pdf=OUT/'RELATORIO_PROJETO_2.pdf'
    doc=SimpleDocTemplate(str(pdf),pagesize=A4,rightMargin=55,leftMargin=55,topMargin=52,bottomMargin=57,title='Projeto Prático 2 - Árvores Multicaminhos',author='Erik Mazzuco; Letícia Moro; Leonardo Herrero')
    doc.build(story,onFirstPage=footer,onLaterPages=footer)
    (OUT/'RELATORIO_PROJETO_2.md').write_text('\n\n'.join(md),encoding='utf-8')
    print('Relatório gerado:',pdf)
    print(json.dumps({'aceleracao_delete_100k':speed,'fator_original_1k_100k':baseline_ratio,'fator_incremental_1k_100k':fixed_ratio,'melhor_ordem_ram':best,'queryAll_indice_sobre_sequencial_lista':qratio},indent=2))

if __name__=='__main__':main()
