from pathlib import Path
import json,subprocess
b=Path(__file__).resolve().parent
cmd=json.loads((b.parent/'compile-command.json').read_text())
subprocess.run(cmd+['-c',str(b/'area_data_native.c'),'-o',str(b/'area_data_native.o')],check=True)
