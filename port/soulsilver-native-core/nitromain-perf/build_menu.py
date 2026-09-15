from pathlib import Path
import json,subprocess
b=Path(__file__).resolve().parent
c=json.loads((b.parent/'compile-command.json').read_text())
subprocess.run(c+['-c',str(b/'menu_native.c'),'-o',str(b/'menu_native.o')],check=True)
