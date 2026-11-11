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

11,034 of 24,164 functions. Every function is checked with `tools/ab.sh`: the game's memory (data, bss, heap pages and RSX memory) must be identical with and without the decompiled code. The check also reports how many decompiled functions actually ran.

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
| World event handler callees: handle resolvers, save-thread polls, message dialogs, brightness/gamma, object-list jobs; remaining anim-script constructors; timed callbacks, job runners, push stack, material copy, message posting; level start, save tasks, job queue, sessions, layer stacks; push-stack blend, language switch, level progress, loading finish; string tables, effects, subtitles, settings save, streaming; effect lookup/destruction, bindings, spawners, subtitles tick, overlay; sound free/play, music layers, AVL map erase, object lists, surfaces; AVL rotations, script args, scene queue, view queries, holders; object attach/release/update, deferred effects, tracks, layer stacks, treasure timer, world teardown; probes, volumes, track rings, camera shake, physics step, level loader, movies; level load/release, loader ticks, HUD prompts, laser volumes, beams, POW meter; loader start/stop/reset, requests, obstacles, target reach tests; user spawning, scene queue, session entries, AVL erase, XML sets, upgrade unlocks; player respawn, session start, spawn points, AVL rotations, XML documents; effect argument binding, scenes finish, layer stacks, body modes, coach HUD; coach enter/leave, player save restore, item blink timers, body pose; speaker lines, camera target stack, player look/aim/abilities, anim layers, coach pages; effect creation and binding, bone matrices, outfits, speaker nodes, named colours, query apply; anim sets and records, skeleton node rebuild, track restart and sync, trophy unlocks; loader and session parts, pause requests, SPU jobs, async save I/O, overlays, object slots; effect def switching and time steps, net/save async state machine, model rings, sound event lookup; object modes, effect instances, track release, Havok body set-up, level restart; timed instance events, Havok skeleton/file loaders, physics shutdown, HUD rescale, shop/speaker modes; map screen refresh, nav graph load and queries, Havok animation registry, float-keyed AVL; effect set registry, completion percentage, camera target, split viewports, model attachments; scroller/UI/path object constructors, follower line of sight, camera controllers, level-entry machine; sound shutdown, geometry sets, level unload, Havok job system, episode percentage; scene load, spawn queue, player spawns, view init, bank open, objects from level defs; scene view/xml/banks, property tables, radar HUD, level start, countdown timer, styled geometry, cloud save upload/download | `src/world/hevent*.cpp`, `src/world/construct2_animscript.cpp` | 742 |
| Engine core helpers: matrices, boxes, Bezier, file/memory readers, input and pad devices, event slots, default heap, draw-record list; SPURS job queue, banks and quads, type registration, IFF loaders, plane math, FIOS async; 86 sorted-map instances, input record/playback, light volumes, header apply; light ticks and cameras, banks, definition readers, OBB/frustum tests; bank teardown, shadow maps, spline/camera/geom readers, resource holders | `src/obj/obj14*.cpp` .. `src/obj/obj18.cpp` | 369 |
| Game code sweep: field getters/setters, flag bits, globals, float and vector math, wrappers, vtable inits, the AVL remove template; condition nodes, hit event queue, vector math, typed values, message records; small 17–48 instruction functions (destructors, constructors, cached ids, virtual-call wrappers, XML readers, random numbers, tree lookups, script-command builders, quaternion math, message filters, clones, handle tables, command list push, hit handlers, range gates, colour picks, spring steps, attach targets, member-function dispatch, tree searches, file/FIOS polling, XML node factories, hit reactions, voice commands, ragdoll release, archive chunk loads, talk state, XML list readers, timed lerps) | `src/obj/obj21*.cpp` .. `src/obj/obj34b.cpp` | 2,612 |
| Game code medium functions: node bindings, effect list, base destructors, query copies, area enter/leave, XML slot defs; section destructors, multiple-inheritance thunks, slot-def factories, slot list push; tuning-variable static constructors/destructors, section mode, level name and element readers, action-script node factories; KVar resolvers, query spawns, template attach/read, talk updates, handle pools; anim clip node, timer lookups, high-score insertion, effect/light constructors, XML element constructors and readers; spawn base constructor, volume transforms, light groups, XML def readers and factories, scene entries; volume activate/prepare, component template readers, anim command lists, spline follow; more template readers, leg/IK/head defs, slot objects, button prompts; trigger defs, hit impulses, body transforms and wake-up, owner-built def factories; leaf def constructors, damage/collision def readers, polygon ear test, quaternion from-to, anim record copies; leaf def readers/factories, more damage/collision defs, hash-keyed registries; damage def record constructors, XML def lists, clip/spec readers, header-record factories | `src/obj/mid1*.cpp` .. `src/obj/mid12*.cpp` | 398 |
| Game code large functions: slot-def reader and factory, map-screen object constructor, damage settings reader; slot-def constructors, damage/collision def reader, deep-copy object constructor, AI controller bind; property-set flags, binding poses, ballistic aim solver, object pools, collision records, damage defs, AI proxy; pool-kind constructors (FX geometry, ribbons, bones), handle pool growth, tree visitor, 46-kind component factory; bone matrices, delayed FX copies, sorted FX geometry, area layer settings, attachment models; keyframed bodies, body shape sync, attachment placement, bone controller targets, character animation states; base update/tick, attachment and aim matrices, bone source, body shapes, volume AABB, ray probes; line of sight, object reinit, physics object init, AI move state; physics base init and mode, mover direction probes, game phase switch, flyer respawn; camera targets and rig update, spline instances, hinge motors, object base init, effect splines, stick-direction anims, player events; camera rig mode handlers, lives and collectible counters, owner motion update, player reset from def; effect-set selection, HUD banner, signed angles, player rig setup and links, trigger proxy body; player link posing and tuning, link part clones, pair anim states, map-set destructor; object set-matrix, link driving, hideout job menu, leaf def base reader | `src/obj/big1*.cpp` .. `src/obj/big14*.cpp` | 112 |
| Template instances, part 20: AVL insert/remove, VMX matrix and vector builders, command records, slot lists, volume-shape and section destructors | `src/obj/obj20_tmpl.cpp` | 332 |
| Template instances project-wide (108 helpers): destructors and deletes, factories, thunks, field accessors, free-list wrappers, small shared algorithms | `src/obj/obj19*.cpp` | 2,093 |
| Template instances in 0x00730000–0x007A0000: vtable-store destructors, deleting destructors, map-unregistering destructors, u32 comparators, tail wrappers, constant returns, empty functions | `src/obj/obj18_tmpl.cpp` | 1,659 |
| World-object helpers: box and zone tests, tree walk, copy, skeleton creation, slot entries, light slots, instance factories, skeleton init/copy, emitter properties, topology copy, IFF chunk loop, light animation; point and spline instances, ApeObj base slots, topology draws, skeleton compose; LOD update, adding objects to the world, component queries, spline math; spline defs and evaluation, skeleton update, child LOD, oscillation queries; peer animators, definition classes, bone lookup, IFF handlers; spline followers, quaternions, view culling, instance free; draw submission, LOD select, light grid, IFF readers, Bezier length/split, matrix multiply; object lighting, followers, draw records, skin packets, string helpers | `src/obj/box.cpp`, `src/obj/zone.cpp`, `src/obj/obj5*.cpp` .. `src/obj/obj13*.cpp` | 286 |
| Shader types (pick shader per material/item) and shaders (bind, end), skinned depth draw, proxy draw, renderer init; water, decal, warp, fog, particle and character shader binds and draws; shader-type create/destroy for all 57 types, shader registry, view fog, lit draws; shader load, per-frame view set-up, water/warp/decal/shrub/skinned draws; UV animation matrix, perspective, view constants, env-map, terrain, panorama, character draws; snow, prop, mirror, eye and fur draws (all shader draw slots now done) | `src/gfx/draw3*.cpp`, `src/gfx/shader2*.cpp` .. `src/gfx/shader6*.cpp` | 414 |
| Renderer small functions: vtable stubs, thunks, typed free lists, deleting destructors, refcount releases, matrix split/rotation; free-list destructors, post effects, occlusion query slots, command stream, render sync, textures; IFF chunk handlers, render targets, query slots, callback lists, shader param loaders; render jobs, particles, warp grid, mesh IFF handlers, stream buffers, list allocators; shadow projection and GPU timer, stereo matrices, warp grid, texture/mesh IFF handlers, emitters, trails; shadow frustum/bounds, frustum planes, draw queues, particles vs planes, GCM state reset; particle emitters and drawing, render thread, MSAA resolve, skin weights, polygon clipping; post-effect chain, frame draw, IFF topology/texture/deflector handlers, display read-back, SPURS queue; pass command interpreter, default textures, stencil light pass, skinning, emitter checks; frame begin/end, emitter draws and SPU jobs, particle spawn shapes, debug shapes; layer drawing, view end, instance chunk readers, deferred destroys, UV constants; tile/billboard builders, sorted draws, debug draw, stream records; cube/sphere meshes, vertex declarations, depth pass, emitter system, IFF mesh/material/DDS handlers; light culling, particle builder, debug font, quad clipping, geometry defs; tuning variables, trophy shutdown, light/camera/point defs, bank draws, LOD, geometry jobs; material apply, thermal shaders, reduced targets, deferred lists, pad read, trophy thread, geometry batches, reflections; view update, crop fit, quad frustum/clip tests, light instances; slot visibility updates, light volume stencil draws, geometry batch draw, frame driver | `src/gfx/gfx7*.cpp` .. `src/gfx/gfx24*.cpp` | 848 |
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
