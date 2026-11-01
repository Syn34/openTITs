# openTITs

A decompilation of **Sly Cooper: Thieves in Time** (PS3, Sanzaru Games, 2013) into readable C++, one function at a time.

openTITs holds only the hand-decompiled code. It runs on [openTIT](https://github.com/Syn34/openTIT/tree/decomp), a static recompiler that turns the game's whole PowerPC executable into C++ that compiles and runs on PC.

The decompilation is *functional* and *incremental*:

- Each function in `src/` replaces the recompiled one at the same address. Everything not yet decompiled keeps running as recompiled code, so the game builds and boots at every step.
- The C++ has to behave like the original, not compile back to the same PowerPC.

> **No game code or data is in this repository.** You need your own disc dump (BCES01284, v01.02). The recompiled C++ is generated locally from your decrypted `EBOOT.elf` and must never be committed or shared (`.gitignore` blocks it).

## Layout

| Path | What |
|---|---|
| `src/` | decompiled functions (`DECOMP(address, name) { ... }`), grouped by subsystem |
| `src/sly4.h` | shared declarations: imports, syscalls, globals, structures, functions not decompiled yet |
| `symbols.txt` | our names for function addresses, used by the tools |
| `openTIT/` | the recompiler and runtime (git submodule, `decomp` branch) |
| `docs/DECOMP.md` | how to decompile a function, and current progress |

## Build

You need what openTIT needs: CMake ≥ 3.20, a C++20 compiler, Ninja, and the Vulkan SDK for graphics. You also need your dump with the decrypted `PS3_GAME/USRDIR/EBOOT.elf`; see openTIT's README for dumping and the optional `firmware/libsre.prx`.

```sh
git clone --recursive https://github.com/Syn34/openTITs
cd openTITs
tools/build.sh "/path/to/Sly 4 dump"            # Linux / macOS
.\tools\build.ps1 -Dump "D:\Dumps\Sly 4"        # Windows
./build/game/opentit "/path/to/Sly 4 dump" --log info
```

The script does four things:

1. Builds openTIT's tools.
2. Recompiles your EBOOT, leaving out the functions decompiled in `src/` (`--decomp src`).
3. Recompiles the firmware and SPU code.
4. Builds the game with `src/` linked in.

To compare against the original code, run with `--no-decomp`, or switch off single functions with `--decomp-off <addr,...>`. See [docs/DECOMP.md](docs/DECOMP.md).

## Progress

| Function | Address | File |
|---|---|---|
| `main` | 0x00463658 | `src/main.cpp` |
| `Main_ParseCommandLine` | 0x0046320C | `src/commandline.cpp` |
| `Main_InitSystems` | 0x005DB1D0 | `src/system/init.cpp` |
| `Np_Init` | 0x0056C0B4 | `src/system/np.cpp` |
| `Sysutil_CheckCallback` | 0x0056C184 | `src/system/sysutil.cpp` |
| `Sysutil_RegisterCallback` | 0x0056C1E8 | `src/system/sysutil.cpp` |
| `File_Exists` | 0x0055D78C | `src/file/file.cpp` |

7 of 24,164 functions.
