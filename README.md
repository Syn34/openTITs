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

2,791 of 24,164 functions. Every function is checked with `tools/ab.sh`: the game's memory (data, bss, heap pages and RSX memory) must be identical with and without the decompiled code. The check also reports how many decompiled functions actually ran.

| Area | Folder | Decompiled |
|---|---|---|
| Start-up: `main`, command line, `Main_InitSystems` | `src/`, `src/system/` | 7 |
| System start-up: SPUs, sysmodules, SPURS, language | `src/system/` | 3 |
| Memory: allocators, pools, heaps | `src/memory/` | 9 |
| Strings and files | `src/string/`, `src/file/` | 3 |
| `Game_Load`: tuning, UI, render targets, pads, formatting | `src/game/load*.cpp` | 16 |
| `Game_Update` and its frame helpers | `src/game/update*.cpp` | 17 |
| XML reader (`.xmb`), vectors, free lists, string helpers | `src/xml/`, `src/container/`, `src/string/`, `src/memory/` | 26 |
| Per frame: players, UI focus and timelines, audio, world submission | `src/game/player.cpp`, `src/game/frame*.cpp`, `src/ui/` | 11 |
| World render submission, audio emitters and mix, voices, math (acosf/atan2f) | `src/world/`, `src/audio/`, `src/math/` | 22 |
| UI text: SetText, layout, measuring, UTF-8, string table; bucket/stack/default heaps | `src/text/`, `src/heap/` | 10 |
| Sound handles and voices, emitter params/stop/focus, look-at, UI render; text format, boot check, element rect/scale | `src/audio/`, `src/world/`, `src/ui/`, `src/text/`, `src/system/` | 20 |
| Sound library handlers, bank start, spatialisation, draw commands; glyph building, element world matrix/size/colour | `src/audio/`, `src/world/`, `src/text/`, `src/uielem/`, `src/view/` | 32 |
| Sound library: handler alloc/start/params, voice stealing, channels, Doppler; focus sound start | `src/audio/` | 17 |
| UI element construction, text constructors and buffers, typewriter and quad animation | `src/uielem/` | 11 |
| Emitter init and key map, handle pools, arrays, sound channels and effects, libm log | `src/audio/` | 15 |
| UI base and type-5 element classes, timeline keys, resources, fonts and font sets, XML element loaders | `src/uielem/`, `src/font/`, `src/uiload/` | 51 |
| StateMachine base class, World init/state/destruct, sub-machines, level registry | `src/world/`, `src/game/levels.cpp` | 26 |
| World_Update, state requests, fades, post-FX state, message dialogs; trophies, upgrades, awards; draw state | `src/world/`, `src/game/meta*.cpp`, `src/gfx/draw_state.cpp` | 20 |
| UI element methods, system message box, heaps, allocators, free lists | `src/uielem/`, `src/common/` | 74 |
| Platform, FIOS, archives, IFF, resources, debug and graphics start-up | `src/platform/`, `src/file/`, `src/resource/`, `src/debug/`, `src/gfx/` | 13 |
| File layer (zip archives, disc files, archive streams), threads, tag map, shaders | `src/file/`, `src/platform/`, `src/resource/`, `src/gfx/` | 25 |
| File I/O on FIOS, read cache, write buffer, archive mounting, zip directory and index | `src/file/` | 18 |
| FIOS operations and wrappers, read-ahead cache slots, zip end record and raw reads | `src/file/` | 28 |
| Tiles and zcull, depth targets, render surfaces, shader groups, post-process and blur programs | `src/gfx/` | 23 |
| Per-frame post-process draws: bloom, DOF, warp, motion trail, thermal, monochrome, blur, MSAA resolve | `src/gfx/fx_*.cpp` | 20 |
| Shader program loading, materials, post-process effect set-up (bloom, DOF, god rays, ...), copy/blur draws | `src/gfx/` | 19 |
| UI file loader, element builders and clones (types 0-9), textures and includes | `src/uiload/`, `src/uielem/`, `src/font/` | 25 |
| World construction: member constructors, save-data defaults, 270 anim-script command/variable types; World event helpers | `src/world/construct*.cpp`, `src/world/event_small.cpp`, `src/world/misc.cpp` | 360 |
| World-object base class, vtable slots, slots/peers, deferred destruction; texture decoders (BMP, TGA, GTF); 9-slice UI element; timeline key builders | `src/obj/`, `src/font/image*.cpp`, `src/uielem/build6.cpp`, `src/uiload/tlbuild.cpp` | 40 |
| World-object classes: child lists, clone, set-class, bound radius, zone mask; light, emitter and camera instances; GeomDef | `src/obj/obj4*.cpp`, `src/obj/class_*.cpp` | 39 |
| World event handler callees: handle resolvers, save-thread polls, message dialogs, brightness/gamma, object-list jobs; remaining anim-script constructors; timed callbacks, job runners, push stack, material copy, message posting; level start, save tasks, job queue, sessions, layer stacks; push-stack blend, language switch, level progress, loading finish; string tables, effects, subtitles, settings save, streaming; effect lookup/destruction, bindings, spawners, subtitles tick, overlay; sound free/play, music layers, AVL map erase, object lists, surfaces; AVL rotations, script args, scene queue, view queries, holders; object attach/release/update, deferred effects, tracks, layer stacks, treasure timer, world teardown; probes, volumes, track rings, camera shake, physics step, level loader, movies; level load/release, loader ticks, HUD prompts, laser volumes, beams, POW meter; loader start/stop/reset, requests, obstacles, target reach tests | `src/world/hevent*.cpp`, `src/world/construct2_animscript.cpp` | 384 |
| Engine core helpers: matrices, boxes, Bezier, file/memory readers, input and pad devices, event slots, default heap, draw-record list | `src/obj/obj14*.cpp` | 104 |
| World-object helpers: box and zone tests, tree walk, copy, skeleton creation, slot entries, light slots, instance factories, skeleton init/copy, emitter properties, topology copy, IFF chunk loop, light animation; point and spline instances, ApeObj base slots, topology draws, skeleton compose; LOD update, adding objects to the world, component queries, spline math; spline defs and evaluation, skeleton update, child LOD, oscillation queries; peer animators, definition classes, bone lookup, IFF handlers; spline followers, quaternions, view culling, instance free; draw submission, LOD select, light grid, IFF readers, Bezier length/split, matrix multiply; object lighting, followers, draw records, skin packets, string helpers | `src/obj/box.cpp`, `src/obj/zone.cpp`, `src/obj/obj5*.cpp` .. `src/obj/obj13*.cpp` | 286 |
| Shader types (pick shader per material/item) and shaders (bind, end), skinned depth draw, proxy draw, renderer init; water, decal, warp, fog, particle and character shader binds and draws; shader-type create/destroy for all 57 types, shader registry, view fog, lit draws; shader load, per-frame view set-up, water/warp/decal/shrub/skinned draws; UV animation matrix, perspective, view constants, env-map, terrain, panorama, character draws; snow, prop, mirror, eye and fur draws (all shader draw slots now done) | `src/gfx/draw3*.cpp`, `src/gfx/shader2*.cpp` .. `src/gfx/shader6*.cpp` | 414 |
| Renderer small functions: vtable stubs, thunks, typed free lists, deleting destructors, refcount releases, matrix split/rotation; free-list destructors, post effects, occlusion query slots, command stream, render sync, textures | `src/gfx/gfx7*.cpp`, `src/gfx/gfx8.cpp` | 434 |
| Stencil shadows (cap and silhouette volumes, z-fail/z-pass), occlusion queries, clears, sphere projection, material animation | `src/gfx/pool*.cpp` | 25 |
| Renderer: render layers and jobs, cached draw state, bloom/noise pass, split-screen views, layer sorting, backdrop, camera split records, 4x4 inverse, render-layer construction and registration, derived layers, depth/resolve passes, material bind, render pool | `src/gfx/render*.cpp`, `src/gfx/layer2.cpp`, `src/gfx/layers3.cpp`, `src/gfx/layers4*.cpp`, `src/gfx/bloomnoise.cpp`, `src/view/`, `src/world/rjobs.cpp` | 81 |
| World objects (lookup, name, scale, instancing), maps, texture allocation/decoding, timeline keys | `src/obj/`, `src/font/`, `src/uiload/` | 11 |
| XML cursor helpers, texture/material helpers, timelines, BMP font loading, type-5 text update | `src/uiload/`, `src/font/`, `src/uielem/` | 16 |
| Video mode selection and configuration, display targets, surfaces, render targets | `src/gfx/` | 9 |
| Zip entry info and open, libgcm/RSX start-up, RSX memory pools, command buffers | `src/file/`, `src/gfx/` | 9 |
| Text element classes (destruct, clone, update, draw), timelines, axis-angle matrix, hash map; zip local header, display and frame surfaces, RSX heap | `src/uielem/`, `src/view/`, `src/file/`, `src/gfx/` | 19 |

`symbols.txt` names many more functions that haven't been decompiled yet. See [docs/DECOMP.md](docs/DECOMP.md) to help.

## Legal

openTITs is an independent project. It is not affiliated with or endorsed by Sony Interactive Entertainment or Sanzaru Games. *Sly Cooper* is a trademark of Sony Interactive Entertainment. You must own the game to use this project, and the repository will never host game code, assets, or recompiled output.
