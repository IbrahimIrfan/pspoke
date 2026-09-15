from pathlib import Path
import ast,re,subprocess
b=Path(__file__).resolve().parent
s=(b/'pokeplatinum/generated/meson.build').read_text()
s=s[s.index('metang_generators = {')+len('metang_generators = '):s.index('\nc_consts_generators')]
d=ast.literal_eval(s.strip())
out=b/'generated/generated';out.mkdir(exist_ok=True)
for name,args in d.items():
 subprocess.run(['python3',str(b/'metang/metang.py'),args['type'],'--tag-name',args['tag'],'--guard','POKEPLATINUM_GENERATED','--output',str(out/f'{name}.h')]+args.get('extra',[])+[str(b/'pokeplatinum/generated'/f'{name}.txt')],check=True,capture_output=True)
print('Generated',len(d),'real enum headers')
