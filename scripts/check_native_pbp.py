#!/usr/bin/env python3
"""Static PSP loader checks for our large-memory native PRX PBP.

Firmware section bounds follow uofw/src/kd/loadcore/loadelf.c CheckElfSectionPRX
and CheckElfImage (retail 32MiB limit). This is not hardware launch verification.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path


def check(path):
    data = Path(path).read_bytes()
    errors = []
    def require(ok, message):
        if not ok:
            errors.append(message)
    require(data[:4] == b'\0PBP', 'Not a PBP')
    offsets = struct.unpack_from('<8I', data, 8)
    require(list(offsets) == sorted(offsets) and offsets[0] >= 40 and offsets[-1] <= len(data), 'Invalid PBP offsets')
    sfo = data[offsets[0]:offsets[1]]
    magic, version, keys, values, count = struct.unpack_from('<5I', sfo)
    require(magic == 0x46535000, 'Invalid SFO')
    fields = {}
    for i in range(count):
        key, fmt, length, maximum, offset = struct.unpack_from('<HHIII', sfo, 20 + 16*i)
        name = sfo[keys+key:].split(b'\0', 1)[0].decode('ascii')
        fields[name] = sfo[values+offset:values+offset+length]
    require(fields.get('MEMSIZE') == b'\1\0\0\0', 'MEMSIZE=1 missing')
    require(fields.get('CATEGORY') == b'MG\0', 'PBP category is not memory-stick game')
    prx = data[offsets[6]:offsets[7]]
    require(prx[:7] == b'\x7fELF\1\1\1', 'Not a little-endian ELF32 PRX')
    hdr = struct.unpack_from('<16sHHIIIIIHHHHHH', prx)
    _, kind, machine, _, entry, phoff, shoff, _, _, phsize, phcount, shsize, shcount, namesidx = hdr
    require(kind == 0xffa0 and machine == 8, 'Expected Allegrex/MIPS relocatable PRX')
    require(phsize == 32 and shsize == 40, 'Unexpected ELF table entry sizes')
    sections = [struct.unpack_from('<10I', prx, shoff+i*shsize) for i in range(shcount)]
    require(0 < shcount and namesidx < shcount, 'Invalid section string table')
    names = sections[namesidx]
    strings = prx[names[4]:names[4]+names[5]]
    for i, s in enumerate(sections):
        name = strings[s[0]:].split(b'\0', 1)[0].decode('ascii', errors='replace')
        require(s[0] < len(strings), f'Section {i} has invalid name')
        require(s[4] < 0x2000000 and s[5] < 0x2000000 and s[4]+s[5] < 0x2000000,
                f'{name}: firmware section bound fails: {s[4]:#x}+{s[5]:#x}={s[4]+s[5]:#x} >=32MiB (80020148)')
        if s[1] != 8:
            require(s[4]+s[5] <= len(prx), f'{name}: file contents out of bounds')
    segments = [struct.unpack_from('<8I', prx, phoff+i*phsize) for i in range(phcount)]
    loads = [p for p in segments if p[0] == 1]
    require(0 < len(loads) < 4, 'Unsupported load segment count')
    for p in loads:
        require(p[1]+p[4] <= 0x2000000 and p[4] <= p[5], 'Invalid program segment file range')
        require(p[1]+p[4] <= len(prx), 'Truncated load segment')
    def at(address, length):
        for p in loads:
            if p[2] <= address and address+length <= p[2]+p[4]:
                pos = p[1]+address-p[2]
                return prx[pos:pos+length]
        raise ValueError(f'Unmapped PRX address {address:#x} size {length}')
    modoffset = loads[0][3] & 0x7fffffff
    attr, minor, major, name, gp, exports, exports_end, imports, imports_end = struct.unpack_from('<HBB28s5I', prx, modoffset)
    require(attr & 0xf000 == 0, 'Native application unexpectedly requests privileged module mode')
    require(exports_end > exports, 'Module export table is empty (linker GC removed .lib.ent)')
    nids = []
    cursor = exports
    while cursor < exports_end:
        libname, version, attribute, words, variables, functions, table = struct.unpack('<IHHBBHI', at(cursor, 16))
        require(words >= 4, 'Invalid export record length')
        if words < 4:
            break
        total = variables+functions
        entries = struct.unpack('<'+'I'*(2*total), at(table, total*8))
        if attribute & 0x8000:
            nids.extend(entries[:total])
        cursor += words*4
    require(0xd632acdb in nids, 'module_start syslib export missing')
    require(0xf01d73a7 in nids, 'module_info syslib export missing')
    memory = max(p[2]+p[5] for p in loads)-min(p[2] for p in loads)
    return {'file': str(path), 'sha256': hashlib.sha256(data).hexdigest(), 'module': name.split(b'\0')[0].decode(),
            'sections': shcount, 'load_bytes': memory, 'exports': [hex(n) for n in nids], 'errors': errors,
            'result': 'FAIL' if errors else 'PASS', 'scope': 'static format checks; hardware launch unverified'}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('pbp', type=Path)
    args = parser.parse_args()
    try:
        report = check(args.pbp)
    except (ValueError, struct.error, IndexError) as exc:
        report = {'result': 'FAIL', 'errors': [str(exc)]}
    print(json.dumps(report, indent=2))
    raise SystemExit(report['result'] != 'PASS')
