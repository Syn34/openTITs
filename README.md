# openTITs

A decompilation of **Sly Cooper: Thieves in Time** (PS3, Sanzaru Games, 2013) into readable, native C++.

The goal is to rewrite every one of the game's 24,164 functions as native C++: typed structures, real parameters and return values, and direct calls, with the PS3 data layout kept. The decompilation is *functional*: a function must behave exactly like the original, but it doesn't have to compile back to the same PowerPC.

> **No game code or data is in this repository.** You need your own disc dump (BCES01284, v01.02).

## Progress

<!-- progress:start -->

**3.7% complete:** 900 of 24,164 functions are native C++.

`█░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░` 3.7%

| Stage | Functions | Share |
|---|---:|---:|
| Native C++ (done) | 900 | 3.7% |
| Decompiled in the old register style, being converted to native | 12,106 | 50.1% |
| Not decompiled yet | 11,158 | 46.2% |

By folder (functions decompiled so far):

| Folder | Area | Native | To convert | Native share |
|---|---|---:|---:|---:|
| `src/` | Start-up: `main`, command line | 0 | 2 | 0.0% |
| `src/audio/` | Sound library, emitters, voices, mixing | 11 | 59 | 15.7% |
| `src/common/` | Heaps, allocators, free lists, UI helpers | 36 | 0 | 100.0% |
| `src/container/` | Vectors and free lists | 3 | 0 | 100.0% |
| `src/debug/` | Debug start-up | 1 | 0 | 100.0% |
| `src/file/` | File layer: FIOS, zip archives, disc files, caches | 0 | 74 | 0.0% |
| `src/font/` | Fonts, font sets, texture loading and decoding | 38 | 0 | 100.0% |
| `src/game/` | `Game_Load`, `Game_Update`, per-frame, levels, trophies | 48 | 0 | 100.0% |
| `src/gfx/` | Renderer: shaders, post-process, particles, RSX/GCM | 538 | 898 | 37.5% |
| `src/heap/` | Bucket, stack and default heaps | 3 | 0 | 100.0% |
| `src/math/` | libm helpers | 5 | 0 | 100.0% |
| `src/memory/` | Allocators, pools, heaps | 10 | 0 | 100.0% |
| `src/obj/` | World objects and game code | 96 | 9,812 | 1.0% |
| `src/platform/` | Platform layer and threads | 6 | 0 | 100.0% |
| `src/resource/` | Resources and IFF loaders | 6 | 0 | 100.0% |
| `src/string/` | String helpers | 6 | 0 | 100.0% |
| `src/system/` | System start-up: SPUs, sysmodules, SPURS | 8 | 0 | 100.0% |
| `src/text/` | UI text: layout, measuring, glyphs, formatting | 12 | 0 | 100.0% |
| `src/ui/` | UI focus and timelines | 4 | 0 | 100.0% |
| `src/uielem/` | UI element classes | 0 | 107 | 0.0% |
| `src/uiload/` | UI file loader and element builders | 31 | 0 | 100.0% |
| `src/view/` | Views, split screen, surfaces | 20 | 0 | 100.0% |
| `src/world/` | World state machine, events, construction | 0 | 1,154 | 0.0% |
| `src/xml/` | XML reader (`.xmb`) | 18 | 0 | 100.0% |

Counts come from `tools/progress.py`, which rewrites this section.

<!-- progress:end -->

Every native function is checked against the original in a differential PowerPC simulator (hundreds of random inputs, results and memory must match), then in the running game with `tools/ab.sh`, which requires the game's memory to be identical with and without the decompiled code.

## How it works

Functions are decompiled one at a time. Each finished function replaces the original at the same address, and functions not decompiled yet still run from code generated locally from your own `EBOOT.elf`, so the game builds and boots at every step. That stand-in disappears as the decompilation completes.

Older work was decompiled in a register style (PPU registers passed around by hand). Those functions are being converted to native C++, and all new decompilation is native.

## Layout

| Path | What |
|---|---|
| `src/` | decompiled functions, grouped by subsystem |
| `src/sly4.h` | shared declarations: imports, syscalls, globals, structures |
| `symbols.txt` | our names for function addresses |
| `engine/` | the runtime the game links against, and the tool that generates stand-in code |
| `docs/NATIVE.md` | how to write and check a native function |
| `docs/DECOMP.md` | the decompilation workflow |
| `docs/BUILD.md` | how to build and run, controls |

## Contributing

Read [docs/NATIVE.md](docs/NATIVE.md) and [docs/DECOMP.md](docs/DECOMP.md). `symbols.txt` names many functions that haven't been decompiled yet. After adding functions, run `tools/progress.py` to update the table above.

## Legal

openTITs is an independent project. It is not affiliated with or endorsed by Sony Interactive Entertainment or Sanzaru Games. *Sly Cooper* is a trademark of Sony Interactive Entertainment. You must own the game to use this project, and the repository will never host game code, assets, or generated output.
