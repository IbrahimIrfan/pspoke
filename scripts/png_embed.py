#!/usr/bin/env python3
"""png_embed.py IN.png OUT.4bpp NAME : indexed-colour PNG -> packed 4bpp bitmap in 8x8 tile order (first pixel in the low nibble)
plus OUT.4bpp.h, a C array named NAME. Same output as `nitrogfx IN.png OUT.4bpp -embed NAME`, without libpng."""
import struct, sys, zlib

def read_indexed_png(path):
    data = open(path, 'rb').read()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        sys.exit(f'{path}: not a PNG')
    pos, idat, hdr = 8, b'', None
    while pos < len(data):
        n, kind = struct.unpack('>I4s', data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if kind == b'IHDR':
            hdr = struct.unpack('>IIBBBBB', body)
        elif kind == b'IDAT':
            idat += body
        pos += 12 + n
    width, height, depth, colour, _, _, interlace = hdr
    if colour != 3 or depth not in (4, 8) or interlace:
        sys.exit(f'{path}: expected a non-interlaced indexed PNG with 4 or 8 bits per pixel')
    raw = zlib.decompress(idat)
    stride = (width * depth + 7) // 8
    rows, prev, off = [], bytearray(stride), 0
    for _ in range(height):
        ftype, line = raw[off], bytearray(raw[off + 1:off + 1 + stride])
        off += 1 + stride
        for i in range(stride):  # filters operate on whole bytes (bpp rounds up to 1 for depth < 8)
            a = line[i - 1] if i else 0
            b, c = prev[i], (prev[i - 1] if i else 0)
            if ftype == 1: line[i] = (line[i] + a) & 255
            elif ftype == 2: line[i] = (line[i] + b) & 255
            elif ftype == 3: line[i] = (line[i] + (a + b) // 2) & 255
            elif ftype == 4:
                p = a + b - c; pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        prev = line
        if depth == 8:
            rows.append(list(line))
        else:
            rows.append([(line[x // 2] >> (4 if x % 2 == 0 else 0)) & 15 for x in range(width)])
    return width, height, rows

def main():
    src, out, name = sys.argv[1:4]
    width, height, rows = read_indexed_png(src)
    # nitrogfx writes the image as 8x8 tiles (tiles left to right, top to bottom; each tile row by row),
    # two pixels per byte with the first pixel in the low nibble.
    if width % 8 or height % 8:
        sys.exit(f'{src}: width and height must be multiples of 8')
    packed = bytearray()
    for ty in range(0, height, 8):
        for tx in range(0, width, 8):
            for y in range(ty, ty + 8):
                row = rows[y]
                for x in range(tx, tx + 8, 2):
                    packed.append((row[x] & 15) | (row[x + 1] & 15) << 4)
    open(out, 'wb').write(packed)
    guard = f'GUARD_EMBEDDABLE_{name}_H'
    text = f'#ifndef {guard}\n#define {guard}\n\n__attribute__((aligned(4))) const u8 {name}[] = {{\n'
    text += ''.join(f'    0x{b:02X},\n' for b in packed)
    text += f'}};\n\n#endif // {guard}\n'
    open(out + '.h', 'w', newline='\n').write(text)

if __name__ == '__main__':
    main()
