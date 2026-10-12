#!/usr/bin/env python3
"""Convert original HGSS ribbon NCGR sprites and NCLR palette to indexed PNGs.

Inputs are byte-for-byte extracted from HeartGold a/0/3/9:
NCGR members 72-119 and NCLR member 136. No ROM data is bundled.
Produces 32x32 indexed sprites with original tile pixels and BGR555 colours.
The 5 palette variants must be matched using sRibbonInfo.nclrId before live use.
Requires Pillow.
"""
import argparse
import hashlib
import io
import json
import re
import zipfile
from pathlib import Path
from PIL import Image


def convert(ncgr, nclr, bank):
    if ncgr[:4] != b'RGCN' or ncgr[16:20] != b'RAHC':
        raise ValueError('Invalid NCGR')
    if nclr[:4] != b'RLCN' or nclr[16:20] != b'TTLP':
        raise ValueError('Invalid NCLR')
    if len(ncgr) != 560 or len(nclr) != 552:
        raise ValueError('Unexpected resource size')
    if int.from_bytes(nclr[28:32], 'little') != 0:
        raise ValueError('Expected standard palette')
    if not 0 <= bank < 5:
        raise ValueError('Invalid ribbon palette bank')
    raw = ncgr[48:560]
    indices = bytearray(1024)
    for tile in range(16):
        for pixel in range(64):
            b = raw[tile * 32 + pixel // 2]
            colour = b >> 4 if pixel & 1 else b & 15
            x = tile % 4 * 8 + pixel % 8
            y = tile // 4 * 8 + pixel // 8
            indices[y * 32 + x] = colour
    rgb = []
    for i in range(16):
        pos = 40 + 2 * (bank * 16 + i)
        colour = int.from_bytes(nclr[pos:pos + 2], 'little')
        rgb.extend(((colour & 31) * 255 // 31,
                    ((colour >> 5) & 31) * 255 // 31,
                    ((colour >> 10) & 31) * 255 // 31))
    im = Image.frombytes('P', (32, 32), bytes(indices))
    im.putpalette(rgb + [0] * (768 - len(rgb)))
    return im


def main():
    p = argparse.ArgumentParser()
    p.add_argument('ncgr_zip', type=Path)
    p.add_argument('nclr_zip', type=Path)
    p.add_argument('output_zip', type=Path)
    args = p.parse_args()
    manifest = {'source_ncgr': 'a/0/3/9:72-119',
                'source_nclr': 'a/0/3/9:136',
                'variants': []}
    with zipfile.ZipFile(args.ncgr_zip) as graphics, zipfile.ZipFile(args.nclr_zip) as palettes:
        nclr = palettes.read('a_0_3_9_member_136.NCLR')
        names = sorted(n for n in graphics.namelist()
                       if re.fullmatch(r'a_0_3_9_member_\d{3}\.bin', n))
        ids = sorted(int(re.search(r'(\d{3})\.bin', n).group(1)) for n in names)
        if ids != list(range(72, 120)):
            raise ValueError('Expected all 48 ribbon NCGR members')
        with zipfile.ZipFile(args.output_zip, 'w', zipfile.ZIP_DEFLATED) as output:
            for name in names:
                member = int(re.search(r'(\d{3})\.bin', name).group(1))
                raw = graphics.read(name)
                for bank in range(5):
                    image = convert(raw, nclr, bank)
                    data = io.BytesIO()
                    image.save(data, format='PNG', transparency=0)
                    path = f'ribbon_ncgr_{member:03d}_nclr_{bank}.png'
                    content = data.getvalue()
                    output.writestr(path, content)
                    manifest['variants'].append({'path': path, 'ncgr_member': member,
                                                 'nclr_bank': bank,
                                                 'sha256': hashlib.sha256(content).hexdigest()})
            output.writestr('manifest.json', json.dumps(manifest, indent=2) + '\n')
    print('Converted 48 authentic sprites x 5 retail palettes')


if __name__ == '__main__':
    main()
