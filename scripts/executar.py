"""Compila, valida e executa benchmarks; medições sequenciais para evitar disputa de I/O."""
from pathlib import Path
import argparse, csv, json, os, platform, subprocess, time, shutil

ROOT=Path(__file__).resolve().parents[1]
ORDENS=[4,8,16,32,64]
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--compiler',help='Caminho para gcc/clang ou zig.exe')
    parser.add_argument('--repeticoes',type=int,default=3)
    parser.add_argument('--somente-testes',action='store_true')
    args=parser.parse_args()
    if args.repeticoes<1:parser.error('repeticoes deve ser positiva')
    if args.compiler:
        compiler=Path(shutil.which(args.compiler) or args.compiler).resolve()
    else:
        compiler=next((ROOT/'tools').glob('zig*/zig.exe'),None)
        if compiler is None:
            parser.error('Informe --compiler gcc, clang ou o caminho para zig.exe.')
    env=os.environ.copy();env['ZIG_GLOBAL_CACHE_DIR']=str(ROOT/'tmp/zig-cache');env['ZIG_LOCAL_CACHE_DIR']=str(ROOT/'tmp/zig-local')
    prefix=[str(compiler),'cc'] if compiler.name.lower() in ['zig.exe','zig'] else [str(compiler)]
    build=ROOT/'build';build.mkdir(exist_ok=True)
    results=ROOT/'results';results.mkdir(exist_ok=True)
    compiler_version=subprocess.check_output([str(compiler),'version'] if len(prefix)==2 else [str(compiler),'--version'],env=env,text=True).strip()
    meta={'sistema':platform.platform(),'processador':platform.processor(),'cpu_logicas':os.cpu_count(),'compilador':str(compiler),'versao':compiler_version,'flags':'-O2 -std=c11','repeticoes':args.repeticoes,'sementes':[12345+i for i in range(args.repeticoes)],'registro_bytes':40,'clock':'QueryPerformanceCounter no Windows; CLOCK_MONOTONIC no POSIX','inicio':time.strftime('%Y-%m-%dT%H:%M:%S%z'),'observacao':'Dados em arquivos locais; cache do SO ativo; sem fsync. Fechamento e persistência do .idx fora do tempo de CRUD.'}
    def compile(source,variant,order,exe):
        cmd=prefix+['-O2','-std=c11']+([] if os.name=='nt' else ['-D_POSIX_C_SOURCE=200809L'])+[f'-DORDEM_INDICE={order}','-I',str(ROOT/variant/'source'),'-I',str(ROOT/'scripts'),str(ROOT/source),'-o',str(exe)]
        r=subprocess.run(cmd,env=env,capture_output=True,text=True)
        (build/(exe.stem+'.compilacao.log')).write_text(r.stdout+r.stderr,encoding='utf-8')
        if r.returncode:raise RuntimeError(r.stdout+r.stderr)
    def run(exe,directory,seed=None):
        directory.mkdir(parents=True,exist_ok=True)
        r=subprocess.run([str(exe)]+([] if seed is None else [str(seed)]),cwd=directory,env=env,capture_output=True,text=True)
        (directory/'execucao.log').write_text(r.stdout+r.stderr,encoding='utf-8')
        if r.returncode or '!! CORRETUDE' in r.stdout:raise RuntimeError(f'{exe.name}: {r.stdout} {r.stderr}')
        return r.stdout.strip()
    test_summary=[]
    for order in ORDENS:
        exe=build/f'test_{order}.exe';compile('tests/test_persistencia.c','solucao',order,exe)
        summary=run(exe,results/'testes'/f'ordem_{order}');print(summary,flush=True);test_summary.append(summary)
    (results/'testes.json').write_text(json.dumps(test_summary,indent=2,ensure_ascii=False),encoding='utf-8')
    if args.somente_testes:return
    rows=[];metrics=[];studies=[]
    def append_csv(path,out,**extra):
        with path.open(encoding='utf-8',newline='') as f:
            for row in csv.DictReader(f):out.append({**extra,**row})
    for variant,orders in [('baseline',[4]),('solucao',ORDENS)]:
        for order in orders:
            exe=build/f'bench_{variant}_{order}.exe';compile(f'{variant}/source/Benchmark/bench.c',variant,order,exe)
            extra_exe=build/f'estudo_{order}.exe'
            if variant=='solucao':compile('benchmarks/estudo.c',variant,order,extra_exe)
            for repeat,seed in enumerate(meta['sementes'],1):
                d=results/'raw'/f'{variant}_{order}'/f'rep_{repeat}'
                start=time.perf_counter();run(exe,d,seed)
                print(f'{variant} ordem={order} repeticao={repeat} OK ({time.perf_counter()-start:.1f}s)',flush=True)
                append_csv(d/'resultados.csv',rows,versao=variant,ordem=order,repeticao=repeat,semente=seed)
                append_csv(d/'metricas.csv',metrics,versao=variant,repeticao=repeat,semente=seed)
                if variant=='solucao':
                    run(extra_exe,d/'estudo',seed);append_csv(d/'estudo/estudo.csv',studies,repeticao=repeat,semente=seed)
    for filename,data in [('benchmark.csv',rows),('metricas.csv',metrics),('estudo.csv',studies)]:
        with (results/filename).open('w',encoding='utf-8',newline='') as f:
            w=csv.DictWriter(f,fieldnames=list(data[0]));w.writeheader();w.writerows(data)
    meta['fim']=time.strftime('%Y-%m-%dT%H:%M:%S%z')
    (results/'ambiente.json').write_text(json.dumps(meta,indent=2,ensure_ascii=False),encoding='utf-8')
    print('Resultados completos salvos em results/',flush=True)

if __name__=='__main__':main()
