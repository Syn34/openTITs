// World_HandleEvent callees, part 27f: the level unload (frees the
// level's objects, effects, sound, physics and caches).
#include "hevent27.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;

namespace {
namespace f {
constexpr u32 Effect_Spawn = 0x004A5FC8;           // (def, 0, 0, 1) -> Effect*
constexpr u32 Effect_SetTime = 0x004CF710;         // (Effect*; f32)
constexpr u32 Model_FreeBlocks = 0x0031FEE8;       // (ModelSet*) (Model_FreeBlocks31FEE8)
constexpr u32 Level_FreeMaps = 0x002E98CC;         // () (Level_FreeMaps2E98CC)
constexpr u32 ResTable_Reset = 0x00579BCC;         // (ResTable*)
constexpr u32 Globals_F65F74_Reset = 0x004AE458;   // ()
constexpr u32 World_EndFade = 0x00466678;          // (World*, 0) (World_EndFade466678)
constexpr u32 CamRig_Reset = 0x0042A740;           // ()
constexpr u32 Treasure_Release = 0x00425BA8;       // (Treasure*, 0) (Treasure_Release425BA8)
constexpr u32 Obj_ReleaseEffect156 = 0x00424F24;   // (o)
constexpr u32 World_ClearLayers = 0x0047E3CC;      // (World*)
constexpr u32 Session_Reset = 0x002E4D64;          // (Session*)
constexpr u32 World_FreePartObj = 0x0047E1D4;      // (World*) (World_FreePartObj47E1D4)
constexpr u32 LayerStack_Reset = 0x0042B240;       // () (LayerStack_Reset42B240)
constexpr u32 Obj_ResetFx = 0x003A1500;            // (o) (Obj_ResetFx3A1500)
constexpr u32 Pool_DeleteAll = 0x002EF13C;         // (pool)
constexpr u32 Obj_ShutdownSlots = 0x0001A840;      // (o) (Obj_ShutdownSlots1A840)
constexpr u32 PtrVector_Clear = 0x0054EC1C;        // (PtrVector*)
constexpr u32 RefNode_Release = 0x004432DC;        // (RefNode*)
constexpr u32 Emitters_StopForeign = 0x00449478;   // ()
constexpr u32 WorldObjs_Reset = 0x000CE110;        // (bool) (WorldObjs_Reset0CE110)
constexpr u32 memset = 0x00726E54;
constexpr u32 Pool_Reset = 0x004545B0;             // (pool) (Pool_Reset4545B0)
constexpr u32 Audio_Stop = 0x004A9084;             // () (Audio_Stop4A9084)
constexpr u32 World_EndSequence = 0x0047DFC8;      // (World*) (World_EndSequence47DFC8)
constexpr u32 Objects_ClearAll = 0x0035B668;       // () (Objects_ClearAll35B668)
constexpr u32 Global_F64A68_Clear = 0x002AAEA8;    // ()
constexpr u32 Obj_Set4E3168 = 0x004E3168;          // (const char*, 0)
constexpr u32 Physics_Shutdown = 0x00341D5C;       // () (Physics_Shutdown341D5C)
constexpr u32 Anim_UnloadAll = 0x00348BFC;         // () (Anim_UnloadAll348BFC)
constexpr u32 Effects_FreeDeferred = 0x004D00CC;   // ()
constexpr u32 Sets_Drain = 0x004BD7D4;             // () (Sets_Drain4BD7D4)
constexpr u32 Sound_Shutdown = 0x004495D8;         // () (Sound_Shutdown4495D8, hevent27_a.cpp)
constexpr u32 Obj_FreeLists = 0x004ECE04;          // (non-zero; reads r3)
constexpr u32 GeomSet_Destroy = 0x004EFF08;        // (GeomSet*) (name?)
constexpr u32 GeomScene_Reset = 0x004F41D8;        // (GeomScene*) (GeomScene_Reset4F41D8, hevent27_b.cpp)
constexpr u32 HkMem_IsLive = 0x007D3B8C;           // () -> bool (name?)
constexpr u32 Singleton_011DF320_Get = 0x007D3B80; // () -> object
constexpr u32 World_SetFlags = 0x00599024;         // (u8)
constexpr u32 HkMem_GetStats = 0x007DD478;         // (Stats* out) (name?)
constexpr u32 HkMem_Dump = 0x007DD660;             // (u8) (name?)
constexpr u32 HkMem_Reset = 0x007D3C5C;            // () (name?)
constexpr u32 HkMemory_Report = 0x00355204;        // () (HkMemory_Report355204)
constexpr u32 Overlay_Free = 0x00429090;           // () (Overlay_Free429090)
constexpr u32 Obj_SetMode470750 = 0x00470750;      // (o, 0, 0)
constexpr u32 Nop_0044D9DC = 0x0044D9DC;           // ()
}  // namespace f

// World fields (offsets from the World object).
constexpr u32 kWPlayer = 0x128164;      // Player*: +5504 the level-exit effect def
constexpr u32 kWModels = 0x1281F8;      // 72-byte model set: arrays at +0 and +24
constexpr u32 kWCtl = 0x128224;         // object (vt +4 release)
constexpr u32 kWSlotGroupCount = 0x128610, kWSlotGroups = 0x128628;  // u8 count, 36-byte groups of 4 objects
constexpr u32 kWHud = 0x1281F4;         // object (vt +12 then vt +4)
constexpr u32 kWFlagsA = 0x1282BC, kWFlagsB = 0x1282D8;
constexpr u32 kWTreasure = 0x12847C;    // 176-byte treasure
constexpr u32 kWTimers = 0x128480;      // 13 20-byte entries, u16 +8 cleared
constexpr u32 kWLayers = 0x1070;        // 9 entries of 0x20C80 bytes, +8 object
constexpr u32 kWLayerStride = 0x20C80;
constexpr u32 kWTargets = 0x1281E0;     // u32 + 4 slots
constexpr u32 kWMusic = 0x128584;       // object (vt +4 release)
constexpr u32 kWRefNodes = 0x1285EC;    // 9 RefNode*
constexpr u32 kWSky = 0x128204;         // object (vt +8)
constexpr u32 kWShadow = 0x128208;
constexpr u32 kWPhysObj = 0x1281D4;     // object (vt +4)
constexpr u32 kWGeomLists = 0x1281C0, kWGeomSet = 0x1281C4;
constexpr u32 kWStatsA = 0x1285AC;      // u32 [5] cleared, +0x1285C0 = -1
// Globals.
constexpr u32 kSession = 0x00F64C20;
constexpr u32 kResTable = 0x011930A0;
constexpr u32 kExitFlag = 0x010BB7FD;   // u8
constexpr u32 kFxOwner = 0x00F657FC, kPoolA = 0x00F64C38, kSlotsOwner = 0x00F557F0;
constexpr u32 kVectorsA = 0x010BD588;   // 4 PtrVectors (20 bytes)
constexpr u32 kGrid = 0x010BAB38;       // 3072 bytes cleared
constexpr u32 kGridA = 0x010BA130, kGridB = 0x010BA134, kGridC = 0x010BB738;
constexpr u32 kPoolB = 0x00F65BE8;
constexpr u32 kPhysicsUp = 0x00F64C98;
constexpr u32 kUnloaded = 0x00F64CC4;   // u8
constexpr u32 kStrShadow = 0x00AE65E4;
constexpr u32 kVectorsB = 0x010B7868;   // 6 PtrVectors (20 bytes)
constexpr u32 kGeomScene = 0x00F658D4;
constexpr u32 kCursorSrc = 0x010B7834, kCursorDst = 0x010B7838, kVecC = 0x010B783C, kVecD = 0x010B7854;
constexpr u32 kSelected = 0x00F65C80;
constexpr u32 kPanelA = 0x010B1A90, kPanelB = 0x010B4950;  // +4 bit 15 cleared
constexpr u32 kFaderA = 0x010B9EF0, kFaderB = 0x010B9F3C, kFaderC = 0x010B9F88, kFaderD = 0x010B9FD4;
// Constants at 0x0047E568: f32 1/60, 0.0f, 1.0f.
constexpr u32 kConsts = 0x0047E568;
}  // namespace

// ---------------------------------------------------------------- 0x0047E578
// u32 World_UnloadLevel47E578(World* w, const Msg* m) (name?): nothing for
// messages of kind (+8) 2 or 3. Otherwise plays the player's level-exit
// effect, frees the model set, the HUD and the slot groups' objects, resets
// the session, layers, music and treasure, releases the nine ref nodes,
// stops sound (Sound_Shutdown4495D8), shuts physics down, frees the
// geometry lists and scene (GeomScene_Reset4F41D8), resets Havok memory
// stats and the faders, and finally puts the world in mode 0.
DECOMP(0047E578, World_UnloadLevel47E578) {
    const u32 w = arg<u32>(ctx, 0), msg = arg<u32>(ctx, 1);
    const u32 kind = load<u32>(msg + 8);
    if (kind == 3 || kind == 2) return ret(ctx, kind);

    const u32 pl = load<u32>(w + kWPlayer);
    const u32 fx = static_cast<u32>(call(ctx, f::Effect_Spawn, {pl != 0 ? load<u32>(pl + 5504) : 0u, 0, 0, 1}));
    if (fx != 0) callf(ctx, f::Effect_SetTime, {fx}, {lf(kConsts + 0)});

    if (load<u32>(w + kWModels) != 0) {
        call(ctx, f::Model_FreeBlocks, {load<u32>(w + kWModels)});
        const u32 ms = load<u32>(w + kWModels);
        if (ms != 0) {
            array_release(ctx, ms + 24);
            array_release(ctx, ms + 0);
            call(ctx, h27fn::SmallFree, {ms, 72});
        }
        store<u32>(w + kWModels, 0);
    }
    store<u8>(kExitFlag, 0);
    if (const u32 c = load<u32>(w + kWCtl); c != 0) {
        vcall(ctx, c, 4, {c});
        store<u32>(w + kWCtl, 0);
    }
    for (u32 i = 0; i < load<u8>(w + kWSlotGroupCount); ++i) {
        for (u32 k = 0; k < 4; ++k) {
            const u32 slot = w + 36 * i + kWSlotGroups + 4 * k;
            const u32 o = load<u32>(slot);
            if (o == 0) break;
            vcall(ctx, o, 88, {o, 0, 1, 1, 1});
            vcall(ctx, o, 40, {o, 0});
            if ((load<u32>(o + 172) >> 18) & 1) vcall(ctx, o, 40, {o, 1});
            store<u32>(slot, 0);
        }
    }
    store<u8>(w + kWSlotGroupCount, 0);
    if (const u32 h = load<u32>(w + kWHud); h != 0) {
        vcall(ctx, h, 12, {h});
        const u32 h2 = load<u32>(w + kWHud);
        if (h2 != 0) vcall(ctx, h2, 4, {h2});
        store<u32>(w + kWHud, 0);
    }
    call(ctx, f::Level_FreeMaps);
    call(ctx, f::ResTable_Reset, {kResTable});
    call(ctx, f::Globals_F65F74_Reset);
    store<u32>(w + kWFlagsA, load<u32>(w + kWFlagsA) & 0x7FFFFFFFu);
    store<u32>(w + kWFlagsB, load<u32>(w + kWFlagsB) & 0x7FFFFFFFu);
    call(ctx, f::World_EndFade, {w, 0});
    call(ctx, f::CamRig_Reset);
    if (load<u32>(w + kWTreasure) != 0) {
        call(ctx, f::Treasure_Release, {load<u32>(w + kWTreasure), 0});
        const u32 t = load<u32>(w + kWTreasure);
        if (t != 0) {
            call(ctx, f::Obj_ReleaseEffect156, {t});
            call(ctx, h27fn::SmallFree, {t, 176});
            store<u32>(w + kWTreasure, 0);
        }
    }
    for (u32 k = 0; k < 13; ++k) store<u16>(w + kWTimers + 20 * k + 8, 0);
    call(ctx, f::World_ClearLayers, {w});
    call(ctx, f::Session_Reset, {load<u32>(kSession)});
    call(ctx, f::World_FreePartObj, {w});
    for (u32 i = 0; i < 9; ++i) {
        const u32 e = w + kWLayers + kWLayerStride * i;
        const u32 o = load<u32>(e + 8);
        if (o == 0) continue;
        vcall(ctx, o, 40, {o, 0});
        store<u32>(e + 8, 0);
    }
    call(ctx, f::LayerStack_Reset);
    store<u32>(w + kWTargets, 0);
    for (u32 k = 0; k < 4; ++k) {
        if (load<u32>(w + kWTargets + 4 + 4 * k) != 0) store<u32>(w + kWTargets + 4 + 4 * k, 0);
    }
    if (const u32 mu = load<u32>(w + kWMusic); mu != 0) {
        vcall(ctx, mu, 4, {mu});
        store<u32>(w + kWMusic, 0);
    }
    call(ctx, f::Obj_ResetFx, {load<u32>(kFxOwner)});
    call(ctx, f::Pool_DeleteAll, {load<u32>(kPoolA)});
    call(ctx, f::Obj_ShutdownSlots, {load<u32>(kSlotsOwner)});
    for (u32 k = 0; k < 4; ++k) call(ctx, f::PtrVector_Clear, {kVectorsA + 20 * k});
    for (u32 k = 0; k < 9; ++k) call(ctx, f::RefNode_Release, {load<u32>(w + kWRefNodes + 4 * k)});
    for (u32 k = 0; k < 9; ++k) store<u32>(w + kWRefNodes + 4 * k, 0);
    call(ctx, f::Emitters_StopForeign);
    call(ctx, f::WorldObjs_Reset, {1});
    call(ctx, f::memset, {kGrid, 0, 3072});
    store<u32>(kGridA, 0);
    store<u32>(kGridB, 0);
    const u32 poolb = load<u32>(kPoolB);
    store<u16>(kGridC + 2, 0);
    call(ctx, f::Pool_Reset, {poolb});
    call(ctx, f::Audio_Stop);
    if (load<u8>(w + 31) == 23 && (load<u8>(w + 45) & 1) == 0) {
        call(ctx, f::World_EndSequence, {w});
        store<u8>(w + 45, static_cast<u8>(load<u8>(w + 45) | 1));
    }
    call(ctx, f::Objects_ClearAll);
    store<u8>(kUnloaded, 1);
    call(ctx, f::Global_F64A68_Clear);
    const f32 zero = lf(kConsts + 4);
    store<u32>(w + kWShadow, 0);
    call(ctx, f::Obj_Set4E3168, {kStrShadow, 0});
    if (const u32 s = load<u32>(w + kWSky); s != 0) {
        vcall(ctx, s, 8, {s});
        store<u32>(w + kWSky, 0);
    }
    call(ctx, f::Physics_Shutdown);
    if (load<u32>(w + kWPhysObj) != 0) {
        store<u32>(kPhysicsUp, 0);
        const u32 p = load<u32>(w + kWPhysObj);
        if (p != 0) vcall(ctx, p, 4, {p});
        store<u32>(w + kWPhysObj, 0);
    }
    call(ctx, f::Anim_UnloadAll);
    for (u32 k = 0; k < 6; ++k) call(ctx, f::PtrVector_Clear, {kVectorsB + 20 * k});
    call(ctx, f::Effects_FreeDeferred);
    call(ctx, f::Sets_Drain);
    call(ctx, f::Sound_Shutdown);
    if (const u32 l = load<u32>(w + kWGeomLists); l != 0) call(ctx, f::Obj_FreeLists, {l});
    if (const u32 g = load<u32>(w + kWGeomSet); g != 0) {
        call(ctx, f::GeomSet_Destroy, {g});
        call(ctx, h27fn::Mem_FreeFrom, {g, 0});
        store<u32>(w + kWGeomSet, 0);
        store<u32>(w + kWGeomLists, 0);
    }
    if (const u32 sc = load<u32>(kGeomScene); sc != 0) call(ctx, f::GeomScene_Reset, {sc});
    store<u32>(kCursorDst, load<u32>(kCursorSrc));
    call(ctx, f::PtrVector_Clear, {kVecC});
    store<u32>(kSelected, 0);
    call(ctx, f::PtrVector_Clear, {kVecD});
    if (call(ctx, f::HkMem_IsLive) != 0) {
        const u32 s = static_cast<u32>(call(ctx, f::Singleton_011DF320_Get));
        vcall(ctx, s, 64, {s});
    }
    call(ctx, f::World_SetFlags, {255});
    {
        Stack stats(ctx, 16);
        call(ctx, f::HkMem_GetStats, {stats.addr()});
        const u8 b = load<u8>(stats.addr());
        if (b != 0) {
            call(ctx, f::HkMem_Dump, {b});
            call(ctx, f::HkMem_Reset);
        }
    }
    call(ctx, f::HkMemory_Report);
    store<u32>(w + kWStatsA + 4, 0);
    store<u32>(w + kWStatsA + 0, 0);
    store<u32>(w + kWStatsA + 12, 0);
    store<u32>(w + kWStatsA + 8, 0);
    store<u32>(w + kWStatsA + 16, 0);
    store<u32>(w + kWStatsA + 20, 0xFFFFFFFFu);
    store<u32>(kPanelA + 4, load<u32>(kPanelA + 4) & ~0x8000u);
    store<u32>(kPanelB + 4, load<u32>(kPanelB + 4) & ~0x8000u);
    vcall(ctx, kFaderA, 12, {kFaderA, 0});
    ctx.fpr[1] = static_cast<f64>(zero);
    vcall(ctx, kFaderB, 20, {kFaderB});
    const f32 one = lf(kConsts + 8);
    ctx.fpr[1] = static_cast<f64>(one);
    vcall(ctx, kFaderC, 20, {kFaderC});
    ctx.fpr[1] = static_cast<f64>(one);
    vcall(ctx, kFaderD, 20, {kFaderD});
    call(ctx, f::Overlay_Free);
    call(ctx, f::Obj_SetMode470750, {w, 0, 0});
    call(ctx, f::Nop_0044D9DC);  // r3 as it leaves it
}
