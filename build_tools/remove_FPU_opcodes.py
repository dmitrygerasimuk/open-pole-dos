#!/usr/bin/env python3
 
# Патчит 8087-only инструкции FDISI/FENI:
#   DB E1  -> FDISI (esc 3, group 4, subfunction 1)
#   DB E0  -> FENI  (esc 3, group 4, subfunction 0)
#
 

import argparse
import os
import shutil
import sys
from typing import List, Tuple

PATTERNS = [
    (b"\xDB\xE1", "FDISI (8087-only) DB E1"),
    (b"\xDB\xE0", "FENI  (8087-only) DB E0"),
]

REPLACEMENTS = {
    "nop":  b"\x90\x90",  # NOP NOP
    "fnop": b"\xD9\xD0",  # FNOP (x87 no-op)
}

def find_all(data: bytes, needle: bytes) -> List[int]:
    offs = []
    i = 0
    while True:
        j = data.find(needle, i)
        if j < 0:
            return offs
        offs.append(j)
        i = j + 1

def looks_like_mz(data: bytes) -> bool:
    return len(data) >= 2 and data[0:2] == b"MZ"

def main() -> int:
    ap = argparse.ArgumentParser(description="Patch 8087-only FPU opcodes (DB E1/DB E0) to safe no-ops.")
    ap.add_argument("file", help="Input binary (EXE/COM/etc)")
    ap.add_argument("--out", help="Output file (default: <input>.fpu_fix)", default=None)
    ap.add_argument("--inplace", action="store_true", help="Patch in place (overwrites input)")
    ap.add_argument("--backup", action="store_true", help="If --inplace, create <input>.bak before patching")
    ap.add_argument("--replace", choices=sorted(REPLACEMENTS.keys()), default="nop",
                    help="Replacement: 'nop' (90 90) or 'fnop' (D9 D0). Default: nop")
    ap.add_argument("--dry-run", action="store_true", help="Only report occurrences, do not write output")
    args = ap.parse_args()

    in_path = args.file
    if not os.path.isfile(in_path):
        print(f"ERROR: file not found: {in_path}", file=sys.stderr)
        return 2

    with open(in_path, "rb") as f:
        data = f.read()

  
    if looks_like_mz(data):
        fmt = "MZ/EXE"
    else:
        fmt = "raw/COM/other"
    print(f"[i] Input: {in_path} ({fmt}), size={len(data)} bytes")

    total_hits: List[Tuple[int, str, bytes]] = []
    for pat, desc in PATTERNS:
        offs = find_all(data, pat)
        for o in offs:
            total_hits.append((o, desc, pat))

    total_hits.sort(key=lambda t: t[0])

    if not total_hits:
        print("[i] No DB E1 / DB E0 patterns found. Nothing to patch.")
        return 0

    print(f"[i] Found {len(total_hits)} hit(s):")
    for o, desc, pat in total_hits:
        print(f"    - 0x{o:08X}: {desc} (bytes {pat.hex(' ')})")

    if args.dry_run:
        print("[i] Dry-run: not writing anything.")
        return 0

    repl = REPLACEMENTS[args.replace]
    if len(repl) != 2:
        print("ERROR: replacement must be 2 bytes", file=sys.stderr)
        return 3

    patched = bytearray(data)
    for o, _desc, pat in total_hits:
        patched[o:o+2] = repl

    if args.inplace:
        out_path = in_path
        if args.backup:
            bak = in_path + ".bak"
            if not os.path.exists(bak):
                shutil.copy2(in_path, bak)
                print(f"[i] Backup created: {bak}")
            else:
                print(f"[i] Backup exists, not overwriting: {bak}")
    else:
        out_path = args.out or (in_path + ".fpu_fix")

    with open(out_path, "wb") as f:
        f.write(patched)

    print(f"[+] Patched with '{args.replace}' ({repl.hex(' ')})")
    print(f"[+] Output: {out_path}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
