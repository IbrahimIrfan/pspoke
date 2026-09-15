"""Fail-closed Thumb source-to-native-C translator, extended from
test_out/soulsilver-native-codegen/translate.py for whole-file translation.

Additions over the parent translator:
  * ASR (immediate) and ROR (register) with original Thumb-1 flag semantics.
  * BLX/BL through a register (indirect call) with the uniform four-register ABI.
  * Direct BL to another function translated from the same file.
  * Typed direct BL to native C/SDK functions, including stack arguments.
  * Incoming stack arguments (ARM args 5+) are materialised above the frame so
    [sp, #N] reads past the frame hit real caller-supplied values instead of
    running off the end of the local frame array.

Everything not explicitly supported still raises; no silent success.
"""
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
SRC = HERE.parents[1] / 'soulsilver-research/pokeheartgold-slop/asm'

# name -> (statement template, argument count). Same contract as the parent.
CALLS = {}
# names translated from the same assembly file: name -> incoming stack arg count
LOCALS = {}
# (containing function, register) -> stack arguments the original sets up for
# that indirect call site. Default 0.
INDIRECT_STACK = {}
import json as _json
STACK_ARGS = _json.loads((HERE / 'stack-args.json').read_text()) if (HERE / 'stack-args.json').exists() else {}

BRANCHES = ('b', 'beq', 'bne', 'bhi', 'blo', 'blt', 'bge', 'bhs', 'bls', 'bgt', 'ble', 'bpl', 'bmi', 'bcc', 'bcs', 'bvs', 'bvc')
COND = {'bpl': '!f.n', 'bmi': 'f.n', 'beq': 'f.z', 'bne': '!f.z', 'bhi': 'f.c&&!f.z', 'blo': '!f.c',
        'blt': 'f.n!=f.v', 'bge': 'f.n==f.v', 'bhs': 'f.c', 'bls': '!f.c||f.z', 'bgt': '!f.z&&f.n==f.v',
        'ble': 'f.z||f.n!=f.v', 'bcc': '!f.c', 'bcs': 'f.c', 'bvs': 'f.v', 'bvc': '!f.v', 'b': '1'}
LOWREG = re.compile(r'r[0-7]')


def splitargs(s):
    return [a.strip() for a in re.split(r',\s*(?![^\[]*\])', s)]


def val(s):
    return s.removeprefix('#')


_IMM = re.compile(r'[0-9a-fA-FxX+\-*/%()<>&|~ ]+')


def iexpr(text):
    """Evaluate an assembler immediate. The preprocessed sources contain
    constant expressions such as 768>>8, so a restricted arithmetic evaluation
    is needed; anything outside that character set raises."""
    text = text.strip().removeprefix('#').strip()
    if not _IMM.fullmatch(text):
        raise ValueError('unsupported immediate expression: ' + text)
    # Bound the evaluation: a stray huge shift would otherwise build an
    # arbitrarily large integer and exhaust memory.
    for shift in re.findall(r'<<\s*(0x[0-9a-fA-F]+|\d+)', text):
        if int(shift, 0) > 64:
            raise ValueError('immediate shift out of range: ' + text)
    value = int(eval(text, {'__builtins__': {}}, {}))
    if not -(1 << 63) <= value < (1 << 63):
        raise ValueError('immediate out of range: ' + text)
    return value


def mem(s):
    a = splitargs(s.strip('[]'))
    return a[0] + (' + ' + val(a[1]) if len(a) > 1 else '')


# Jump-table labels: `_021F362C` or a named local label ending in the address (`ov18_021F362C`).
SWITCH_LABEL = r'(?:[A-Za-z_]\w*_|_)[0-9A-Fa-f]{8}'


def normalize_switches(body):
    lines = [line.split(';')[0].strip() for line in body.splitlines()]
    out = []
    i = 0
    while i < len(lines):
        m = re.fullmatch(r'add (r[0-7]), (r[0-7]), \2', lines[i])
        if m and i + 7 < len(lines):
            dst, src = m.groups()
            seq = [f'add {dst}, pc', f'ldrh {dst}, [{dst}, #6]', f'lsl {dst}, {dst}, #0x10',
                   f'asr {dst}, {dst}, #0x10', f'add pc, {dst}']
            if lines[i + 1:i + 6] == seq and re.fullmatch(SWITCH_LABEL + ':', lines[i + 6]):
                table = lines[i + 6][:-1]
                targets = []
                j = i + 7
                while j < len(lines):
                    entry = re.fullmatch(r'\.short (' + SWITCH_LABEL + r') - ' + re.escape(table) + r' - 2', lines[j])
                    if not entry:
                        break
                    target = entry.group(1)
                    offset = int(target[-8:], 16) - int(table[-8:], 16) - 2
                    if not -32768 <= offset <= 32767:
                        raise ValueError('switch target offset outside signed16')
                    targets.append(target)
                    j += 1
                if targets:
                    out.append('ssswitch ' + ','.join([dst, src, table] + targets))
                    i = j
                    continue
        out.append(lines[i])
        i += 1
    return '\n'.join(out)


# (base, size, backing symbol) - DS address windows the SDK port keeps in arrays.
_HW_WINDOWS = [
    (0x027FFC00, 0x400, 's_HW_MAIN_MEM_SYSTEM'),   # boot info, reset params, ROM header copy
    (0x05000000, 0x200, 's_HW_BG_PLTT'), (0x05000200, 0x200, 's_HW_OBJ_PLTT'),
    (0x05000400, 0x200, 's_HW_DB_BG_PLTT'), (0x05000600, 0x200, 's_HW_DB_OBJ_PLTT'),
    (0x06000000, 0x80000, 's_HW_BG_VRAM'), (0x06200000, 0x20000, 's_HW_DB_BG_VRAM'),
    (0x06400000, 0x40000, 's_HW_OBJ_VRAM'), (0x06600000, 0x20000, 's_HW_DB_OBJ_VRAM'),
    (0x06800000, 0xA4000, 's_HW_LCDC_VRAM'),
    (0x07000000, 0x400, 's_HW_OAM'), (0x07000400, 0x400, 's_HW_DB_OAM'),
]
def _hw_literal(expr):
    if not re.fullmatch(r'0x[0-9a-fA-F]+|[0-9]+', expr):
        return None
    v = iexpr(expr)
    for base, size, sym in _HW_WINDOWS:
        if base <= v < base + size:
            off = v - base
            return (f'((uint32_t)(uintptr_t){sym} + 0x{off:x}u)', sym)
    return None


def _prepare(name, body, relocs):  # noqa
    """Strip literal pools; return (lines, literals, externs)."""
    body = normalize_switches(body)
    literals = {}
    externs = set()
    io = False
    clean = []
    for raw in body.splitlines():
        line = raw.split(';')[0].strip()
        m = re.fullmatch(r'(\w+):\s*\.word\s+(.+)', line)
        if m:
            label, expr = m.groups()
            expr = expr.strip()
            # FS_OVERLAY_ID(name) in a literal pool: resolve like build_data.py does.
            if 'FS_OVERLAY_ID(' in expr:
                import json as _json, os as _os
                _ids = _json.load(open(_os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..', '..', 'soulsilver-native-core', 'overlay-ids.json')))
                expr = re.sub(r'FS_OVERLAY_ID\((\w+)\)', lambda mm: str(_ids[mm[1]]), expr)
            # Assembler constant arithmetic from expanded macros, e.g.
            # std_trainer_approach -> ((740) - 1 + 3000). Digits, hex, parentheses and
            # + - * / << >> only; evaluated here, then treated as a plain number.
            if re.fullmatch(r'[\s0-9a-fA-FxX()+\-*/<>]+', expr) and re.search(r'[()+\-*/<>]', expr) and not re.fullmatch(r'0x[0-9a-fA-F]+|[0-9]+', expr):
                if not re.fullmatch(r'[\s()+\-*/<>]*(?:0x[0-9a-fA-F]+|[0-9]+)(?:[\s()+\-*/<>]+(?:0x[0-9a-fA-F]+|[0-9]+))*[\s()+\-*/<>]*', expr):
                    raise ValueError('unsupported literal expression: ' + expr)
                expr = str(eval(expr, {'__builtins__': {}}, {}) & 0xFFFFFFFF)
            # DS system-area literal used by every app's VBlank callback:
            # *(0x027E0000 + 0x3FF8) |= OS_IE_V_BLANK is OS_SetIrqCheckFlag by hand
            # (HW_INTR_CHECK_BUF). The SDK port keeps that word in s_HW_INTR_CHECK_BUF,
            # which its OS_WaitIrq reads, so the literal maps to that variable's
            # address minus the fixed 0x3FF8 displacement (56 sites in 45 files).
            if expr.lower() == '0x027e0000':
                externs.add('s_HW_INTR_CHECK_BUF')
                literals[label] = '((uint32_t)(uintptr_t)s_HW_INTR_CHECK_BUF - 0x3FF8u)'
                continue
            # Other fixed DS hardware/system addresses: the SDK port backs each region
            # with a named array (simulator/simvariables.h), so a literal inside a
            # known window becomes that array's address plus the displacement.
            hw = _hw_literal(expr)
            if hw:
                literals[label], sym = hw
                externs.add(sym)
                continue
            if re.fullmatch(r'0x[0-9a-fA-F]+|[0-9]+', expr):
                value = iexpr(expr)
                if 0x04000000 <= value < 0x05000000:
                    # DS I/O register address: accesses in this function are
                    # routed to the SDK register variables at run time.
                    io = True
                elif 0x02000000 <= value < 0x02800000:
                    raise ValueError('possible fixed DS memory/MMIO literal ' + name + ' ' + expr)
                # 0x05F5E100 (100,000,000), odd values etc.: constants, not addresses.
                literals[label] = expr
            elif re.fullmatch(r'[A-Za-z_]\w*(?:\s*[+]\s*(?:0x[0-9a-fA-F]+|[0-9]+))?', expr):
                pieces = re.split(r'\s*[+]\s*', expr)
                symbol = relocs.get(pieces[0], pieces[0])
                externs.add(symbol)
                literals[label] = f'(uint32_t)(uintptr_t){symbol}' + (' + ' + pieces[1] if len(pieces) > 1 else '')
            elif re.fullmatch(r'[A-Za-z_]\w*\s*[+\-]\s*[\s0-9a-fA-FxX()+\-*/<>]+', expr) and re.fullmatch(
                    r'[\s()+\-*/<>]*(?:0x[0-9a-fA-F]+|[0-9]+)(?:[\s()+\-*/<>]+(?:0x[0-9a-fA-F]+|[0-9]+))*[\s()+\-*/<>]*',
                    re.split(r'\s*[+\-]\s*', expr, 1)[1]):
                # Symbol plus constant arithmetic, e.g. `ov18_021F9DE4 + 7 * 8 + 2` (struct array
                # element address): evaluate the offset, keep the symbol relocatable.
                m_sym = re.match(r'([A-Za-z_]\w*)\s*([+\-])\s*(.*)$', expr, re.S)
                off = eval(m_sym[3], {'__builtins__': {}}, {})
                if m_sym[2] == '-':
                    off = -off
                symbol = relocs.get(m_sym[1], m_sym[1])
                externs.add(symbol)
                literals[label] = f'(uint32_t)((uintptr_t){symbol} + ({off}))'
            else:
                raise ValueError('unsupported literal expression: ' + expr)
        else:
            clean.append(raw)
    return '\n'.join(clean), literals, externs, io


def scan(name, body, relocs=None):
    """First pass only: returns (peak frame bytes, incoming stack-arg words)."""
    body, literals, externs, _io = _prepare(name, body, relocs or {})
    lines = [l.split(';')[0].strip() for l in body.splitlines()]
    lines = [l for l in lines if l and not l.startswith('.')]
    labels = {l[:-1]: i for i, l in enumerate(lines) if l.endswith(':')}
    todo = [(0, 0)]
    seen = {}
    peak = 0
    nstack = 0
    while todo:
        i, depth = todo.pop()
        if i >= len(lines):
            raise ValueError('fallthrough without return')
        if i in seen:
            if seen[i] != depth:
                raise ValueError('inconsistent stack depth at merge')
            continue
        seen[i] = depth
        line = lines[i]
        op, _, args = line.partition(' ')
        # record sp-relative accesses above the current frame: incoming arguments
        for m in re.finditer(r'\[sp(?:,\s*#(0x[0-9a-fA-F]+|\d+))?\]', line):
            off = iexpr(m.group(1)) if m.group(1) else 0
            if off >= depth:
                if (off - depth) % 4:
                    raise ValueError('unaligned incoming stack argument')
                nstack = max(nstack, (off - depth) // 4 + 1)
        if op in ('push', 'pop'):
            depth += (1 if op == 'push' else -1) * len(args.strip('{}').split(',')) * 4
        elif re.match(r'(add|sub) sp,', line):
            amt = iexpr(args.split('#')[-1])
            depth += (1 if op == 'sub' else -1) * amt
        if depth < 0 or depth % 4:
            raise ValueError('unsupported stack layout')
        peak = max(peak, depth)
        if (op == 'pop' and 'pc' in args) or line == 'bx lr':
            if depth:
                raise ValueError('unbalanced return stack')
            continue
        if op == 'ssswitch':
            fields = args.split(',')
            for target in fields[3:]:
                if target not in labels:
                    raise ValueError('switch target missing')
                todo.append((labels[target], depth))
            continue
        if op in BRANCHES:
            if args not in labels:
                raise ValueError('external tail branch')
            todo.append((labels[args], depth))
            if op == 'b':
                continue
        if op == 'bx':
            if depth:
                raise ValueError('unbalanced register tail branch')
            continue
        todo.append((i + 1, depth))
    nstack = max(nstack, _indirect_frame_reach(lines, seen, literals))
    # stack-args.json: incoming stack-arg words the scan cannot see, e.g. a
    # struct passed by value whose tail is copied through a pointer (ldmia loop
    # or a payload pointer handed to a C copier). Without it the callee reads
    # past its frame array (Elder Li's trainer message: ov12_022639B8).
    nstack = max(nstack, STACK_ARGS.get(name, 0))
    # An implausible count means the frame analysis mis-read an address
    # computation; refuse rather than emit a function with a bogus signature.
    if nstack > 16:
        raise ValueError(f'{name}: implausible incoming stack argument count {nstack}')
    return peak, nstack


def _s32(v):
    return ((v + 0x80000000) & 0xffffffff) - 0x80000000


def _indirect_frame_reach(lines, depths, literals):
    """Second, conservative sweep for frame accesses built as `add rX, sp[, #N]`
    followed by `[rX, #M]`. Thumb-1 reaches deep frames and incoming arguments
    that way, and those offsets are invisible to the plain `[sp, #N]` pattern.
    Register state is abstract (constant / sp+offset / unknown) and is dropped
    at every label, so the result is an upper bound, never an underestimate of
    what the plain pattern already found."""
    state = {}
    nstack = 0
    for i, line in enumerate(lines):
        if line.endswith(':'):
            state = {}
            continue
        depth = depths.get(i)
        op, _, args = line.partition(' ')
        a = splitargs(args.strip())
        # reads/writes through a register that currently holds sp + offset
        m = re.match(r'(ldr|ldrh|ldrb|ldrsh|ldrsb|str|strh|strb)$', op)
        if m and len(a) == 2 and a[1].startswith('['):
            inner = splitargs(a[1].strip('[]'))
            base = state.get(inner[0])
            off = 0
            if len(inner) > 1:
                if inner[1].startswith('#'):
                    off = iexpr(inner[1])
                else:
                    # register offset: usable when that register is a known constant
                    idx = state.get(inner[1])
                    off = idx[1] if idx and idx[0] == 'c' else None
            if base and base[0] == 'sp' and off is not None and depth is not None:
                total = _s32(base[1] + off)
                if total >= depth:
                    nstack = max(nstack, (total - depth) // 4 + 1)
        # abstract transfer function
        dst = a[0] if a else None
        if op == 'ldr' and len(a) == 2 and LOWREG.fullmatch(a[0]) and not a[1].startswith('['):
            text = literals.get(a[1], '')
            state[a[0]] = ('c', int(text, 0)) if re.fullmatch(r'0x[0-9a-fA-F]+|\d+', text) else None
        elif op == 'mov' and len(a) == 2 and LOWREG.fullmatch(a[0]) and a[1].startswith('#'):
            state[a[0]] = ('c', iexpr(a[1]))
        elif op == 'add' and len(a) == 3 and a[1] == 'sp' and a[2].startswith('#'):
            state[a[0]] = ('sp', iexpr(a[2]))
        elif op == 'add' and len(a) == 2 and a[1] == 'sp':
            prev = state.get(a[0])
            state[a[0]] = ('sp', _s32(prev[1])) if prev and prev[0] == 'c' else None
        elif op == 'add' and len(a) == 2 and a[1].startswith('#'):
            prev = state.get(a[0])
            state[a[0]] = (prev[0], _s32(prev[1] + iexpr(a[1]))) if prev else None
        elif op == 'add' and len(a) == 3 and a[2].startswith('#'):
            prev = state.get(a[1])
            state[a[0]] = (prev[0], _s32(prev[1] + iexpr(a[2]))) if prev else None
        elif op in ('push', 'pop'):
            for r in args.strip('{}').split(','):
                state.pop(r.strip(), None)
        elif op in ('bl', 'blx'):
            state = {}
        elif dst and LOWREG.fullmatch(dst) and op not in ('cmp', 'tst', 'str', 'strh', 'strb'):
            state[dst] = None
    return nstack


def _shared_epilogue(body, label):
    """MWCC shares one function epilogue: `bl _label` where _label is inside the
    same function and its code runs straight into `pop {..., pc}`. The bl only
    sets lr; the pop returns from the enclosing function, so the call is a goto.
    A local block that returns through lr (`bx lr`) is a real subroutine and is
    left to the unsupported path so it is noticed."""
    lines = [l.split(';')[0].strip() for l in body.splitlines()]
    try:
        i = lines.index(label + ':')
    except ValueError:
        return False
    for l in lines[i + 1:]:
        if not l or l.startswith('.'):
            continue
        if l.startswith('pop ') and 'pc' in l:
            return True
        if l.startswith('bx ') or l.startswith('mov pc') or l.startswith('b ') or l.startswith('b.'):
            return False   # returns through lr, or leaves the block: not a plain epilogue
    return False

def _far_branch_label(body, label):
    """PSP port (Pokeathlon ov96_02216C38): MWCC also emits `bl _label` as a long-range branch to a label inside the
    same function that is not an epilogue (e.g. a loop head). If the function never returns through lr (no `bx lr`,
    no `mov pc, lr`), that bl cannot be a real local subroutine, so it is a goto. Otherwise leave it unsupported."""
    lines = [l.split(';')[0].strip() for l in body.splitlines()]
    if label + ':' not in lines:
        return False
    for l in lines:
        if l == 'bx lr' or l.replace(' ', '') == 'movpc,lr':
            return False
    return True

def emit(name, body, relocs=None):
    relocs = relocs or {}
    peak, nstack = scan(name, body, relocs)
    body, literals, externs, io = _prepare(name, body, relocs)
    words = max(1, peak // 4 + nstack)
    base = peak // 4
    params = ['uint32_t a0', 'uint32_t a1', 'uint32_t a2', 'uint32_t a3'] + [f'uint32_t a{4 + i}' for i in range(nstack)]
    out = [*(f'extern unsigned char {symbol}[];' for symbol in sorted(externs)),
           f'uint32_t native_{name}(' + ','.join(params) + ') {',
           'uint32_t r0=a0,r1=a1,r2=a2,r3=a3,r4=0,r5=0,r6=0,r7=0,lr=SS_LR_MARKER;',
           'uint32_t r8=0,r9=0,sl=0,fp=0,ip=0;(void)r8;(void)r9;(void)sl;(void)fp;(void)ip;',
           f'uint32_t stack[{words}]; uint32_t sp=(uint32_t)(uintptr_t)(stack+{base});',
           'Flags f={0}; uint32_t x,y,res;']
    for i in range(nstack):
        out.append(f'st32(sp+{4 * i},a{4 + i});')
    ops = []
    for raw in body.splitlines():
        line = raw.split(';')[0].strip()
        if not line:
            continue
        if line.startswith('.balign '):
            continue
        if line.startswith('.'):
            raise ValueError('unsupported directive: ' + line)
        if line.endswith(':'):
            if line[:-1] != name:
                out.append(line)
            continue
        op, _, args = line.partition(' ')
        args = args.strip()
        a = splitargs(args)
        ops.append(op)
        if op in ('push', 'pop'):
            regs = [s.strip() for s in args.strip('{}').split(',')]
            if op == 'push':
                out.append(f'sp-={len(regs) * 4};')
                out.extend(f'st32(sp+{i * 4},{r});' for i, r in enumerate(regs))
            else:
                out.extend(f'{r}=ld32(sp+{i * 4});' for i, r in enumerate(regs) if r != 'pc')
                out.append(f'sp+={len(regs) * 4};')
                if 'pc' in regs:
                    out.append('return r0;')
        elif op in ('ldr', 'ldrh', 'ldrb', 'str', 'strh', 'strb', 'ldrsh', 'ldrsb'):
            if not a[1].startswith('['):
                if op != 'ldr' or a[1] not in literals:
                    raise ValueError('unresolved literal relocation')
                out.append(f'{a[0]}={literals[a[1]]};')
                continue
            width = {'ldr': 32, 'ldrh': 16, 'ldrb': 8, 'str': 32, 'strh': 16, 'strb': 8, 'ldrsh': 16, 'ldrsb': 8}[op]
            out.append(f'{a[0]}=' + ('(int32_t)(int16_t)' if op == 'ldrsh' else '(int32_t)(int8_t)' if op == 'ldrsb' else '')
                       + f'ld{width}({mem(a[1])});' if op.startswith('ld') else f'st{width}({mem(a[1])},{a[0]});')
        elif op in ('stmia', 'ldmia'):
            match = re.fullmatch(r'(r[0-7])!,\s*\{([^}]+)\}', args)
            if not match:
                raise ValueError('unsupported multiple-transfer syntax')
            b, listed = match.groups()
            regs = [r.strip() for r in listed.split(',')]
            if any(not re.fullmatch(r'r[0-7]', r) for r in regs) or (op == 'stmia' and b in regs):
                raise ValueError('unsupported multiple-transfer register alias/range')
            # Thumb LDMIA with the base in the list (e.g. `ldmia r2!, {r1, r2}`): every word is read
            # from the ORIGINAL base and there is no write-back (the loaded value wins, ARMv4T/v5TE).
            out.append(f'x={b};')
            for i, reg in enumerate(regs):
                out.append(f'st32(x+{i * 4},{reg});' if op == 'stmia' else f'{reg}=ld32(x+{i * 4});')
            if not (op == 'ldmia' and b in regs):
                out.append(f'{b}=x+{len(regs) * 4};')
        elif op == 'ssswitch':
            dst, src, table, *targets = args.split(',')
            out.append(f'x={src}; res=x+x; af(&f,x,x,res); switch(x) {{')
            for index, target in enumerate(targets):
                offset = int(target[-8:], 16) - int(table[-8:], 16) - 2
                out.append(f'case {index}: {dst}=(uint32_t)(int32_t)(int16_t){offset}; nz(&f,{dst}); f.c=0; goto {target};')
            out.append('default: __builtin_trap(); }')
        elif op == 'mov':
            if a[1].startswith('#'):
                out.append(f'{a[0]}={val(a[1])};')
                # Thumb-1 immediate MOV updates NZ; carry/overflow unchanged.
                out.append(f'nz(&f,{a[0]});')
            elif LOWREG.fullmatch(a[0]) and LOWREG.fullmatch(a[1]):
                raise ValueError('low-register MOV needs original opcode/flag audit')
            else:
                # MOV involving a high register is the flag-preserving encoding.
                out.append(f'{a[0]}={a[1]};')
        elif op in ('add', 'sub', 'cmp'):
            lhs = a[0] if len(a) == 2 else a[1]
            rhs = a[-1]
            out.append(f'x={val(lhs)}; y={val(rhs)}; res=x' + ('+' if op == 'add' else '-') + 'y;')
            if op != 'cmp':
                out.append(f'{a[0]}=res;')
            if 'sp' not in a:
                out.append(('af' if op == 'add' else 'sf') + '(&f,x,y,res);')
        elif op == 'lsl':
            src = a[0] if len(a) == 2 else a[1]
            out.append(f'{a[0]}=shift(&f,{src},{val(a[-1])});')
        elif op == 'lsr':
            src = a[0] if len(a) == 2 else a[1]
            out.append(f'{a[0]}=shift_right(&f,{src},{val(a[-1])});')
        elif op == 'asr':
            src = a[0] if len(a) == 2 else a[1]
            if a[-1].startswith('#'):
                out.append(f'{a[0]}=shift_arith(&f,{src},{val(a[-1])});')
            else:
                # Thumb-1 ASR Rd,Rs: register amount; 0 leaves the value (and carry) unchanged, unlike ASR #0 (=32).
                out.append(f'{a[0]}=shift_arith_reg(&f,{src},{val(a[-1])});')
        elif op == 'ror':
            if len(a) != 2 or not LOWREG.fullmatch(a[0]) or not LOWREG.fullmatch(a[1]):
                raise ValueError('unsupported ROR form: ' + line)
            out.append(f'{a[0]}=rotate_right(&f,{a[0]},{a[1]});')
        elif op in ('adc', 'sbc'):
            if len(a) != 2 or any(not LOWREG.fullmatch(r) for r in a):
                raise ValueError('unsupported ' + op + ' register class')
            if op == 'adc':
                out.append(f'x={a[0]}; y={a[1]}; res=x+y+f.c; adcf(&f,x,y,f.c,res); {a[0]}=res;')
            else:
                out.append(f'x={a[0]}; y=~{a[1]}; res=x+y+f.c; adcf(&f,x,y,f.c,res); {a[0]}=res;')
        elif op == 'neg':
            out.append(f'x=0; y={a[1]}; res=x-y; {a[0]}=res; sf(&f,x,y,res);')
        elif op == 'mvn':
            if len(a) != 2 or any(not LOWREG.fullmatch(r) for r in a):
                raise ValueError('unsupported MVN register class')
            out.append(f'{a[0]}=~{a[1]}; nz(&f,{a[0]});')  # Thumb-1 MVN updates NZ, preserves CV.
        elif op in ('orr', 'and', 'eor', 'tst', 'mul', 'bic'):
            symbol = {'orr': '|', 'and': '&', 'eor': '^', 'tst': '&', 'mul': '*', 'bic': '&~'}[op]
            out.append(f'res={a[0]}{symbol}{a[1]}; nz(&f,res);')
            if op != 'tst':
                out.append(f'{a[0]}=res;')
        elif op in BRANCHES:
            out.append(f'if ({COND[op]}) goto {args};')
        elif op == 'nop':
            pass
        elif op == 'bx' and args == 'lr':
            out.append('return r0;')
        elif op == 'bx' and LOWREG.fullmatch(args):
            # Original ARM/Thumb interworking tail call through a register.
            n = INDIRECT_STACK.get((name, args), 0)
            extra = ''.join(f',ld32(sp+{4 * i})' for i in range(n))
            # The same encoding is both an interworking tail call and the
            # "pop the saved lr into a register and branch to it" return
            # idiom. The saved link register carries a marker, so the two
            # cases are told apart at run time instead of being guessed.
            out.append(f'if ({args}==SS_LR_MARKER) return r0;')
            out.append(f'return ((SSThumbFn{n})(uintptr_t)({args}&~1u))(r0,r1,r2,r3{extra});')
        elif op in ('bl', 'blx') and LOWREG.fullmatch(args):
            n = INDIRECT_STACK.get((name, args), 0)
            extra = ''.join(f',ld32(sp+{4 * i})' for i in range(n))
            out.append(f'r0=((SSThumbFn{n})(uintptr_t)({args}&~1u))(r0,r1,r2,r3{extra});')
            out.append('r1=0xa1a1a1a1; r2=0xa2a2a2a2; r3=0xa3a3a3a3; f=(Flags){0};')
        elif op == 'bl' and args not in LOCALS and _shared_epilogue(body, args):   # before CALLS: port.py binds every unknown bl target
            out.append(f'goto {args}; /* bl to the shared epilogue */')
        elif op == 'bl' and args not in LOCALS and re.fullmatch(r'_[0-9A-F]{8}', args) and _far_branch_label(body, args):
            out.append(f'goto {args}; /* bl used as a far branch inside the function */')
        elif op == 'bl' and args in LOCALS:
            extra = ''.join(f',ld32(sp+{4 * i})' for i in range(LOCALS[args]))
            out.append(f'r0=native_{args}(r0,r1,r2,r3{extra});')
            out.append('r1=0xa1a1a1a1; r2=0xa2a2a2a2; r3=0xa3a3a3a3; f=(Flags){0}; /* caller-clobbered */')
        elif op == 'bl' and args in CALLS:
            out.append(CALLS[args][0])
            out.append('r1=0xa1a1a1a1; r2=0xa2a2a2a2; r3=0xa3a3a3a3; f=(Flags){0}; /* caller-clobbered */')
        else:
            raise ValueError(f'{name}: unsupported: {line}')
    out.append('}')
    text = '\n'.join(out)
    if io:
        # this function computes DS register addresses arithmetically
        text = re.sub(r'\b(ld|st)(8|16|32)\(', r'ssio_\1\2(', text)
    return text, ops


prefix = '''/* Generated by translate2.py. Real native pointers; no ARM bus, scheduler or interpreter. */
#include <stdint.h>
#include <string.h>
typedef char require_32bit_native_pointer[sizeof(void*)==4?1:-1];
typedef struct {unsigned n,z,c,v;} Flags;
#define SS_LR_MARKER 0x0ffedcb0u
typedef uint32_t (*SSThumbFn0)(uint32_t,uint32_t,uint32_t,uint32_t);
typedef uint32_t (*SSThumbFn1)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
typedef uint32_t (*SSThumbFn2)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
typedef uint32_t (*SSThumbFn3)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
typedef uint32_t (*SSThumbFn4)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
typedef uint32_t (*SSThumbFn5)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
typedef uint32_t (*SSThumbFn6)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
typedef uint32_t (*SSThumbFn7)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
typedef uint32_t (*SSThumbFn8)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
static void nz(Flags*f,uint32_t r){f->n=r>>31;f->z=r==0;}
static void af(Flags*f,uint32_t x,uint32_t y,uint32_t r){nz(f,r);f->c=((uint64_t)x+y)>>32;f->v=((~(x^y)&(x^r))>>31);}
static void sf(Flags*f,uint32_t x,uint32_t y,uint32_t r){nz(f,r);f->c=x>=y;f->v=((x^y)&(x^r))>>31;}
static void adcf(Flags*f,uint32_t x,uint32_t y,unsigned c,uint32_t r){nz(f,r);f->c=((uint64_t)x+y+c)>>32;f->v=((~(x^y)&(x^r))>>31);}
static uint32_t shift(Flags*f,uint32_t x,unsigned n){n&=255;if(n){f->c=n<=32?((x>>(32-n))&1):0;x=n<32?x<<n:0;}nz(f,x);return x;}
static uint32_t shift_right(Flags*f,uint32_t x,unsigned n){n&=255;if(n){f->c=n<=32?((x>>(n-1))&1):0;x=n<32?x>>n:0;}nz(f,x);return x;}
/* Thumb-1 ASR #imm: an encoded 0 means 32. N/Z from the result, C the last bit out. */
static uint32_t shift_arith(Flags*f,uint32_t x,unsigned n){if(n==0||n>32)n=32;f->c=(x>>(n-1>31?31:n-1))&1;x=n>=32?(uint32_t)((int32_t)x>>31):(uint32_t)((int32_t)x>>n);nz(f,x);return x;}
/* Thumb-1 ASR Rd,Rs: only the low byte of Rs counts; 0 leaves value and carry unchanged; >=32 fills with the sign. */
static uint32_t shift_arith_reg(Flags*f,uint32_t x,uint32_t s){unsigned n=s&255;if(n){if(n>=32){f->c=x>>31;x=(uint32_t)((int32_t)x>>31);}else{f->c=(x>>(n-1))&1;x=(uint32_t)((int32_t)x>>n);}}nz(f,x);return x;}
/* Thumb-1 ROR Rd,Rs: only the low byte of Rs counts; a zero rotate leaves the flags' carry alone. */
static uint32_t rotate_right(Flags*f,uint32_t x,uint32_t s){unsigned n=s&255;if(n){unsigned m=n&31;if(m)x=(x>>m)|(x<<(32-m));f->c=x>>31;}nz(f,x);return x;}
#define MEM(N,T) static uint32_t ld##N(uint32_t p){T v;memcpy(&v,(void*)(uintptr_t)p,sizeof(v));return v;} static void st##N(uint32_t p,uint32_t v){T w=v;memcpy((void*)(uintptr_t)p,&w,sizeof(w));}
MEM(8,uint8_t) MEM(16,uint16_t) MEM(32,uint32_t)
/* DS I/O register window: hand assembly loads the literal 0x04xxxxxx address
 * and indexes off it, so those accesses are dispatched to the SDK register
 * variables (ds_ioreg.c, generated from the ARM9 SDK headers). */
extern uint32_t ss_io_read8(uint32_t);extern uint32_t ss_io_read16(uint32_t);extern uint32_t ss_io_read32(uint32_t);
extern void ss_io_write8(uint32_t,uint32_t);extern void ss_io_write16(uint32_t,uint32_t);extern void ss_io_write32(uint32_t,uint32_t);
#define SS_IO(p) (((p)>>24)==0x04u)
static uint32_t ssio_ld8(uint32_t p){return SS_IO(p)?ss_io_read8(p):ld8(p);}
static uint32_t ssio_ld16(uint32_t p){return SS_IO(p)?ss_io_read16(p):ld16(p);}
static uint32_t ssio_ld32(uint32_t p){return SS_IO(p)?ss_io_read32(p):ld32(p);}
static void ssio_st8(uint32_t p,uint32_t v){if(SS_IO(p))ss_io_write8(p,v);else st8(p,v);}
static void ssio_st16(uint32_t p,uint32_t v){if(SS_IO(p))ss_io_write16(p,v);else st16(p,v);}
static void ssio_st32(uint32_t p,uint32_t v){if(SS_IO(p))ss_io_write32(p,v);else st32(p,v);}
'''
