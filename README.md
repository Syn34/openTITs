# openTITs

A decompilation of **Sly Cooper: Thieves in Time** (PS3, Sanzaru Games, 2013) into readable C++, one function at a time.

The decompilation is *functional* and *incremental*. A static recompiler in `engine/` first turns the game's whole PowerPC executable into C++ that compiles and runs on PC. Each function decompiled by hand in `src/` then replaces the recompiled one at the same address. Everything not yet decompiled keeps running as recompiled code, so the game builds and boots at every step. The goal is C++ that behaves like the original; it doesn't have to compile back to the same PowerPC.

> **No game code or data is in this repository.** You need your own disc dump (BCES01284, v01.02). The recompiled C++ is generated locally from your decrypted `EBOOT.elf` and must never be committed or shared (`.gitignore` blocks it).

## Layout

| Path | What |
|---|---|
| `src/` | decompiled functions (`DECOMP(address, name) { ... }`), grouped by subsystem |
| `src/sly4.h` | shared declarations: imports, syscalls, globals, structures, functions not decompiled yet |
| `symbols.txt` | our names for function addresses, used by the tools |
| `engine/` | the static recompiler (`opentit-recomp`) and the runtime the game links against |
| `docs/DECOMP.md` | how to decompile a function and check it, plus progress |
| `docs/ENGINE.md`, `docs/ROADMAP.md` | how the engine works, and what it still lacks |

## Build

You need:
- CMake ≥ 3.20 and Ninja;
- a C++20 compiler: MSVC 2022, clang 15+ or GCC 12+;
- for graphics, the [Vulkan SDK](https://vulkan.lunarg.com). On Linux, `libvulkan-dev glslang-dev` work.

SDL2 provides the window; CMake downloads it if it isn't installed. On Windows without SDL2, a native window is used.

### 1. Dump and decrypt

Dump your disc, then decrypt `PS3_GAME/USRDIR/EBOOT.BIN` with RPCS3 (*Utilities → Decrypt PS3 Binaries*). Save the result next to it as `EBOOT.elf`:

```
<dump>/PS3_GAME/USRDIR/EBOOT.elf     <- decrypted
<dump>/PS3_GAME/USRDIR/Sly4.psarc
...
```

**Recommended: the SPURS firmware module.** Sly 4 uses SPURS job chains, so the game runs Sony's real SPURS library.

1. In RPCS3, install the firmware: *File → Install Firmware*. The `PS3_UPDATE/PS3UPDAT.PUP` from your dump works.
2. Decrypt `dev_flash/sys/external/libsre.sprx` with *Utilities → Decrypt PS3 Binaries*.
3. Save the result as `<dump>/firmware/libsre.prx`, or as `<dump>/libsre.prx`.

No keys or firmware are shipped here.

### 2. Build and run

```sh
tools/build.sh "/path/to/Sly 4 dump"            # Linux / macOS
.\tools\build.ps1 -Dump "D:\Dumps\Sly 4"        # Windows, from a VS 2022 Developer PowerShell
./build/game/opentit "/path/to/Sly 4 dump" --log info
```

The script:

1. Builds the tools and runs the tests.
2. Recompiles your EBOOT, leaving out the functions decompiled in `src/` (`--decomp src`).
3. Recompiles the firmware and SPU code.
4. Builds the game with `src/` linked in.

Useful switches:

```sh
./build/game/opentit <dump> --no-decomp            # run the recompiled originals instead
./build/game/opentit <dump> --decomp-off 00463658  # ... for some functions only
./build/game/opentit <dump> --dump-mem run.bin     # save game memory at exit, to compare runs
./build/game/opentit <dump> --spu-record <dump>/sly4.spurec   # record SPU jobs for the next build
./build/game/opentit <dump> -- -T                  # pass switches to the game itself
```

### Controls

Any controller SDL recognises works as a DualShock 3. On the keyboard:

| Keys | Pad |
|---|---|
| W A S D | left stick |
| arrow keys | right stick |
| I J K L | d-pad |
| Space / F / C / R | Cross / Square / Circle / Triangle |
| Q / E | L1 / R1 |
| Left Shift / Left Ctrl | L2 / R2 |
| Z / X | L3 / R3 |
| Enter / Backspace | Start / Select |

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

7 of 24,164 functions. See [docs/DECOMP.md](docs/DECOMP.md) to help.

## Legal

openTITs is an independent project. It is not affiliated with or endorsed by Sony Interactive Entertainment or Sanzaru Games. *Sly Cooper* is a trademark of Sony Interactive Entertainment. You must own the game to use this project, and the repository will never host game code, assets, or recompiled output.
