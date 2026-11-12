# Building

> **No game code or data is in this repository.** You need your own disc dump (BCES01284, v01.02). Code generated from your decrypted `EBOOT.elf` stays on your machine and must never be committed or shared (`.gitignore` blocks it).

You need:
- CMake ≥ 3.20 and Ninja;
- a C++20 compiler: MSVC 2022, clang 15+ or GCC 12+;
- for graphics, the [Vulkan SDK](https://vulkan.lunarg.com). On Linux, `libvulkan-dev glslang-dev` work.

SDL2 provides the window; CMake downloads it if it isn't installed. On Windows without SDL2, a native window is used.

## 1. Dump and decrypt

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

## 2. Build and run

```sh
tools/build.sh "/path/to/Sly 4 dump"            # Linux / macOS
.\tools\build.ps1 -Dump "D:\Dumps\Sly 4"        # Windows, from a VS 2022 Developer PowerShell
./build/game/opentit "/path/to/Sly 4 dump" --log info
```

The script:

1. Builds the tools and runs the tests.
2. Generates code for the functions not decompiled yet from your EBOOT, leaving out everything in `src/` (`--decomp src`).
3. Generates the firmware and SPU code.
4. Builds the game with `src/` linked in.

Useful switches:

```sh
./build/game/opentit <dump> --no-decomp            # run the original functions instead of src/
./build/game/opentit <dump> --decomp-off 00463658  # ... for some functions only
./build/game/opentit <dump> --dump-mem run.bin     # save game memory at exit, to compare runs
./build/game/opentit <dump> --spu-record <dump>/sly4.spurec   # record SPU jobs for the next build
./build/game/opentit <dump> -- -T                  # pass switches to the game itself
```

## Controls

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
