# Decompilation

openTITs is a functional, incremental decompilation of Sly Cooper: Thieves in Time (BCES01284 v01.02). It runs on the static recompiler and runtime in `engine/` (see [ENGINE.md](ENGINE.md)).

Every game function starts out as recompiled C++, which is correct but unreadable. Functions are then rewritten by hand, one at a time, as readable C++ in `src/`. A hand-written function replaces the recompiled one at the same address, and the game keeps running at every step. The aim is matching *behaviour*, not matching assembly: the C++ doesn't have to compile back to the original PowerPC.

This repository holds only hand-written code and our own names. The generated code is still derived from your EBOOT and never goes in the repo.

## Layout

| Path | What |
|---|---|
| `src/sly4.h` | shared declarations: import stubs, syscalls, globals, structs, not-yet-decompiled functions |
| `src/<area>/*.cpp` | decompiled functions, grouped by subsystem (`system/`, `file/`, ...) |
| `symbols.txt` | names for function addresses, decompiled or not |
| `engine/runtime/include/opentit/decomp.h` | the API decompiled code uses |

## Writing a function

```cpp
#include "../sly4.h"
using namespace sly4;

// bool File_Exists(const char* path)
DECOMP(0055D78C, File_Exists) {
    const u32 path = arg<u32>(ctx, 0);
    const u32 file = static_cast<u32>(call(ctx, fn::File_Open, {path, kModeRead}));
    if (file != 0) call_ptr(ctx, load<u32>(load<u32>(file) + 8), {file});  // virtual close
    ret(ctx, file != 0 ? 1u : 0u);
}
```

A decompiled function keeps the PPU calling convention, so recompiled callers and callees work around it unchanged:

- **Arguments:** `arg<T>(ctx, n)` reads r3+n and `farg(ctx, n)` reads f1+n.
- **Results:** `ret(ctx, v)` writes r3 (sign-extended for signed types) or f1.
- **Calls:** `call(ctx, addr, {args...})` calls any guest function: recompiled, decompiled or an import stub. `callf` passes float arguments, and `call_ptr` calls through a function descriptor (function pointers, vtables). `syscall(ctx, num, {args...})` makes an LV2 system call. Each call gets its own linkage area, and r2/LR are preserved.
- **Memory:** guest memory is big endian.
  - Describe structures with `be<T>` fields (`be_u32`, `be_f32`, `gptr`, ...) and view them in place with `ref<T>(ea)`.
  - `load<T>(ea)` and `store<T>(ea, v)` read and write single values.
- **Stack space:** `Stack frame(ctx, bytes)` reserves guest stack for out-parameters; `frame.addr()` is its address.

Only the 32-bit addresses the game uses are valid guest pointers. Keep them as `u32`.

## The workflow

1. **Pick a function.** Small leaf functions and wrappers are easiest. Start from what you know: a caller you've already decompiled, or a string.
2. **Study it** with `funcinfo`:
   ```sh
   ./build/tools/opentit-recomp funcinfo <EBOOT.elf> 0055D78C --symbols symbols.txt
   ```
   It prints:
   - callers and callees, with names from `symbols.txt` and import stubs;
   - every constant the function builds and every string it references, including TOC loads;
   - the annotated disassembly.
3. **Name it** in `symbols.txt`. Mark guesses with `(name?)` in the note.
4. **Write it** in `src/<area>/`, with `DECOMP(<address>, <name>)` at the start of a line. Anything it touches that has no name yet goes in `sly4.h`.
5. **Regenerate** so the recompiler leaves the original body to you:
   ```sh
   ./build/tools/opentit-recomp recompile <EBOOT.elf> -o gen --decomp src --symbols symbols.txt
   cmake --build build/game
   ```
   `tools/build.sh` / `tools/build.ps1` already pass both flags. Only the generated files that changed are rewritten, so the rebuild is quick. If `src/` and the generated code disagree, the build stops and tells you to regenerate.
6. **Compare** against the original. The recompiled body is still linked in, as `f_<addr>_recomp`:
   ```sh
   ./build/game/opentit <dump> --log debug > with.log  2>&1
   ./build/game/opentit <dump> --log debug --no-decomp > without.log 2>&1
   ./build/game/opentit <dump> --decomp-off 0055D78C,0056C0B4    # switch off just these
   ```
   The HLE and syscall logs of both runs should match. For a stricter check, `--dump-mem <file>` writes the game's data and bss when the run ends (exit or crash), so you can compare the two runs byte for byte:
   ```sh
   ./build/game/opentit <dump> --dump-mem with.bin
   ./build/game/opentit <dump> --dump-mem without.bin --no-decomp
   cmp -l with.bin without.bin | wc -l
   ```
   `--dump-mem` also writes `<file>.pages`, a hash of every 64 KiB page of user memory (the game's heaps) and RSX local memory. `tools/ab.sh` compares both, and `--decomp-coverage <file>` lists which decompiled functions actually ran, so you know what a check covered.

   Some bytes differ between any two runs because of timing. The VBlank counter at 0x010F0E00 is one of them: run `--no-decomp` twice to see the rest. A byte that differs only when your function is switched on is a bug.

## Leftover registers

Compiled code sometimes reads a register that its caller never set, so it sees whatever the previous call left there. `Draw_BeginPrims`, for example, ORs r7 into a command word, and its callers rely on the r7 that `Draw_SetTexture` returns with. A decompiled function doesn't naturally leave its volatile registers the way the original did.

When you decompile a function:
- check what its callees read before writing;
- when a known caller depends on a register your function leaves behind, set it explicitly (`ctx.gpr[7] = ...`) and add a comment.

Known dependencies on still-recompiled functions (whoever decompiles them must keep the register at exit):
- `0056AEFC` (Debug_Draw): `View_EndDraw_00522140` builds stream record 22 from the r4 it leaves.
- `0045717C` reads r7 after `FrameBuffer_Reset_00523584` and after `CallbackList_Invoke`; both set or pass it through as the original does.
- `00282638` reads f6 after its call to `Quat_FromTo_0074D4B8` at 002827F0 and f5 after the one at 00282D58; `Quat_FromTo_0074D4B8` sets both as the original leaves them.

## Conventions

- **Names:** `Subsystem_Verb` for functions (`File_Exists`, `Np_Init`), `g::name` for globals, and PascalCase for structs.
- **Comments:** document the original signature above each function, e.g. `// bool File_Exists(const char* path)`, and add a short note on what the function is for.
- **Behaviour:** keep the original's quirks, such as wrap-around counters and unchecked results, unless a note says why something differs.
- **Literals:** prefer named constants (SDK enum names where they apply) to bare numbers.

## Progress

See the table in the [README](../README.md#progress). In the cloud test setup the boot stops at the missing `Sly4.psarc`, before `Game_Load` runs. The A/B check there covers start-up, memory, strings, files and system init. `Game_Load` and `Game_Update` need a full dump to be compared at run time.
