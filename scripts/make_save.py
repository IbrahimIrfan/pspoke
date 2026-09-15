#!/usr/bin/env python3
"""Create a blank 512 KiB save file (erased flash = 0xFF), like a new cartridge. Refuses to overwrite."""
import sys
from pathlib import Path
if len(sys.argv) != 2: sys.exit('usage: make_save.py <path/to/Game.native.sav>')
p = Path(sys.argv[1])
if p.exists(): sys.exit(f'{p} already exists; not overwriting a save')
p.parent.mkdir(parents=True, exist_ok=True); p.write_bytes(b'\xff' * 524288); print('created blank save', p)
