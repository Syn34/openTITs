#!/usr/bin/env python3
"""Add every DECOMP() / NATIVE() in src/ to symbols.txt that it doesn't list yet, and
mark listed ones as decompiled.  Usage: tools/sync_symbols.py [src dir]"""
import os, re, sys
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
pat = re.compile(r'^(?:DECOMP|NATIVE)\(([0-9A-Fa-f]{8}),\s*(?:[A-Za-z0-9_]+::)*([A-Za-z0-9_]+)\)', re.M)
found = {}
src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, 'src')
for d, _, files in os.walk(src):
    for f in files:
        if f.endswith(('.cpp', '.cc')):
            p = os.path.join(d, f)
            rel = 'src/' + os.path.relpath(p, src).replace(os.sep, '/')
            for m in pat.finditer(open(p, encoding='utf-8').read()):
                found[m.group(1).upper()] = (m.group(2), rel)
sym = os.path.join(root, 'symbols.txt')
lines = open(sym, encoding='utf-8').read().rstrip('\n').split('\n')
seen = set()
for i, l in enumerate(lines):
    m = re.match(r'([0-9A-F]{8}) ', l)
    if not m: continue
    a = m.group(1); seen.add(a)
    if a in found and 'decompiled:' not in l:
        lines[i] = l + ('; ' if '//' in l else ' // ') + 'decompiled: ' + found[a][1]
new = [f'{a} {n:<24} // decompiled: {f}' for a, (n, f) in sorted(found.items()) if a not in seen]
if new:
    lines += ['', '# Decompiled functions not listed above (added by tools/sync_symbols.py)'] + new
open(sym, 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
print(f'{len(found)} decompiled, {len(new)} added')
