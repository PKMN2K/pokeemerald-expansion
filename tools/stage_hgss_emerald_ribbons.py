#!/usr/bin/env python3
"""Stage authentic HGSS ribbon PNGs selected by the Emerald compatibility CSV.

No scaling, drawing, palette approximation or replacement of live graphics.
"""
import argparse
import csv
import hashlib
import io
import json
import zipfile
from pathlib import Path
from PIL import Image


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('converted_zip', type=Path, help='HGSS_ribbon_authentic_converted_32px.zip')
    parser.add_argument('mapping_csv', type=Path, help='emerald_ribbon_compat.csv')
    parser.add_argument('output_zip', type=Path)
    args = parser.parse_args()
    with args.mapping_csv.open(newline='', encoding='utf-8') as stream:
        rows = list(csv.DictReader(line for line in stream if not line.startswith('#')))
    assert len(rows) == 32, 'Expected exactly 32 Emerald ribbons'
    assert [int(r['emerald_ribbon_id']) for r in rows] == list(range(32))
    manifest = []
    with zipfile.ZipFile(args.converted_zip) as source, zipfile.ZipFile(args.output_zip, 'w', zipfile.ZIP_DEFLATED) as dest:
        for row in rows:
            ncgr, bank = int(row['ncgr_member']), int(row['nclr_bank'])
            assert 72 <= ncgr <= 119 and 0 <= bank <= 4
            name = f'ribbon_ncgr_{ncgr:03d}_nclr_{bank}.png'
            data = source.read(name)
            image = Image.open(io.BytesIO(data))
            assert image.size == (32, 32) and image.mode == 'P'
            out = f"emerald_{int(row['emerald_ribbon_id']):02d}_{row['emerald_symbol'].lower()}.png"
            dest.writestr(out, data)
            manifest.append({**row, 'asset': out, 'sha256': hashlib.sha256(data).hexdigest()})
        dest.writestr('manifest.json', json.dumps(manifest, indent=2) + '\n')
    print(f'Staged {len(manifest)} original-resolution ribbon combinations; no live art replaced.')


if __name__ == '__main__':
    main()
