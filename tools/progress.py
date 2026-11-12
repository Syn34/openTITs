#!/usr/bin/env python3
"""Count decompilation progress and rewrite the Progress section of README.md.

usage: tools/progress.py [SRC_DIR] [--readme README.md] [--print]

A function is native when it is defined with NATIVE(addr, fn) and still in
the old register style when it is defined with DECOMP(addr, name).
"""
import argparse
import pathlib
import re
import sys

TOTAL = 24164  # functions in BCES01284 v01.02 EBOOT.elf

AREAS = {
    "(root)": "Start-up: `main`, command line",
    "audio": "Sound library, emitters, voices, mixing",
    "common": "Heaps, allocators, free lists, UI helpers",
    "container": "Vectors and free lists",
    "debug": "Debug start-up",
    "file": "File layer: FIOS, zip archives, disc files, caches",
    "font": "Fonts, font sets, texture loading and decoding",
    "game": "`Game_Load`, `Game_Update`, per-frame, levels, trophies",
    "gfx": "Renderer: shaders, post-process, particles, RSX/GCM",
    "heap": "Bucket, stack and default heaps",
    "math": "libm helpers",
    "memory": "Allocators, pools, heaps",
    "obj": "World objects and game code",
    "platform": "Platform layer and threads",
    "resource": "Resources and IFF loaders",
    "string": "String helpers",
    "system": "System start-up: SPUs, sysmodules, SPURS",
    "text": "UI text: layout, measuring, glyphs, formatting",
    "ui": "UI focus and timelines",
    "uielem": "UI element classes",
    "uiload": "UI file loader and element builders",
    "view": "Views, split screen, surfaces",
    "world": "World state machine, events, construction",
    "xml": "XML reader (`.xmb`)",
}

DEF = re.compile(r"^(NATIVE|DECOMP)\([0-9A-Fa-f]{8}", re.M)
START, END = "<!-- progress:start -->", "<!-- progress:end -->"


def count(src: pathlib.Path):
    per = {}
    for f in src.rglob("*.cpp"):
        rel = f.relative_to(src).parts
        folder = rel[0] if len(rel) > 1 else "(root)"
        n = per.setdefault(folder, [0, 0])
        for m in DEF.finditer(f.read_text(errors="replace")):
            n[0 if m.group(1) == "NATIVE" else 1] += 1
    return {k: v for k, v in per.items() if sum(v)}


def pct(a, b):
    return f"{100.0 * a / b:.1f}%" if b else "-"


def bar(frac, width=40):
    full = round(frac * width)
    return "█" * full + "░" * (width - full)


def render(per):
    native = sum(v[0] for v in per.values())
    old = sum(v[1] for v in per.values())
    todo = TOTAL - native - old
    out = [
        START,
        "",
        f"**{pct(native, TOTAL)} complete:** {native:,} of {TOTAL:,} functions are native C++.",
        "",
        f"`{bar(native / TOTAL)}` {pct(native, TOTAL)}",
        "",
        "| Stage | Functions | Share |",
        "|---|---:|---:|",
        f"| Native C++ (done) | {native:,} | {pct(native, TOTAL)} |",
        f"| Decompiled in the old register style, being converted to native | {old:,} | {pct(old, TOTAL)} |",
        f"| Not decompiled yet | {todo:,} | {pct(todo, TOTAL)} |",
        "",
        "By folder (functions decompiled so far):",
        "",
        "| Folder | Area | Native | To convert | Native share |",
        "|---|---|---:|---:|---:|",
    ]
    for k in sorted(per, key=lambda k: (k != "(root)", k)):
        n, o = per[k]
        folder = "`src/`" if k == "(root)" else f"`src/{k}/`"
        out.append(f"| {folder} | {AREAS.get(k, '')} | {n:,} | {o:,} | {pct(n, n + o)} |")
    out += [
        "",
        "Counts come from `tools/progress.py`, which rewrites this section.",
        "",
        END,
    ]
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="?", default="src")
    ap.add_argument("--readme", default="README.md")
    ap.add_argument("--print", action="store_true", help="print instead of rewriting README")
    a = ap.parse_args()
    text = render(count(pathlib.Path(a.src)))
    if a.print:
        print(text)
        return
    readme = pathlib.Path(a.readme)
    body = readme.read_text()
    if START not in body or END not in body:
        sys.exit(f"{readme}: progress markers not found")
    head, rest = body.split(START, 1)
    readme.write_text(head + text + rest.split(END, 1)[1])


if __name__ == "__main__":
    main()
