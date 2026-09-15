from pathlib import Path
import json,subprocess
b=Path(__file__).resolve().parent
cmd=json.loads((b.parent/'compile-command.json').read_text())
subprocess.run(cmd+['-c',str(b/'field_init.c'),'-o',str(b/'field_init.o')],check=True)
