"""Preserva a procedência, gera diff e empacota apenas os artefatos de entrega."""
from pathlib import Path
import hashlib, json, difflib, zipfile

ROOT=Path(__file__).resolve().parents[1]
ORIGINAL=ROOT/'original/FrameworkPersistencia_com_indice/source'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    original_files={str(p.relative_to(ROOT)).replace('\\','/'):sha(p) for p in sorted(ORIGINAL.rglob('*')) if p.is_file()}
    manifest={'zip_original_sha256':sha(ROOT/'FrameworkPersistencia_com_indice.zip') if (ROOT/'FrameworkPersistencia_com_indice.zip').exists() else None,'enunciado_sha256':sha(ROOT/'21_Projeto_Arvores_Multicaminhos-1.pdf'),'arquivos_originais':original_files,'referencia':json.loads((ROOT/'referencia/origem.json').read_text(encoding='utf-8'))}
    (ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False),encoding='utf-8')
    diffs=[]
    for p in sorted(ORIGINAL.rglob('*')):
        if p.suffix not in ['.h','.c']:continue
        rel=p.relative_to(ORIGINAL);modified=ROOT/'solucao/source'/rel
        old=p.read_text(encoding='cp1252').splitlines(keepends=True)
        try:new=modified.read_text(encoding='utf-8').splitlines(keepends=True)
        except UnicodeDecodeError:new=modified.read_text(encoding='cp1252').splitlines(keepends=True)
        diffs.extend(difflib.unified_diff(old,new,fromfile='original/'+rel.as_posix(),tofile='solucao/'+rel.as_posix()))
    (ROOT/'alteracoes.patch').write_text(''.join(diffs),encoding='utf-8')
    files=[]
    for directory in ['baseline','solucao','tests','benchmarks','scripts','plots','referencia','output/pdf']:
        for p in (ROOT/directory).rglob('*'):
            if p.is_file() and '__pycache__' not in p.parts and p.suffix not in ['.pyc','.dat','.idx','.exe','.o']:
                files.append(p)
    for p in (ROOT/'original').rglob('*'):
        if p.is_file() and p.suffix in ['.c','.h','.pdf','.csv']:files.append(p)
    for p in (ROOT/'results').rglob('*'):
        if p.is_file() and p.suffix in ['.csv','.json','.log']:files.append(p)
    files.extend((ROOT/'build').glob('*.compilacao.log'))
    for name in ['README.md','.gitignore','.gitattributes','requirements.txt','alteracoes.patch','manifest.json','21_Projeto_Arvores_Multicaminhos-1.pdf','FrameworkPersistencia_com_indice.zip','tools/zig-download.json']:
        files.append(ROOT/name)
    output=ROOT/'output/PROJETO_2_ENTREGA.zip'
    with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(set(files)):z.write(p,p.relative_to(ROOT).as_posix())
    with zipfile.ZipFile(output) as z:
        assert z.testzip() is None
        assert 'output/pdf/RELATORIO_PROJETO_2.pdf' in z.namelist()
        assert 'solucao/source/Operacoes/5_delete.h' in z.namelist()
        print('ZIP verificado:',output,'arquivos:',len(z.namelist()),'bytes:',output.stat().st_size)

if __name__=='__main__':main()
