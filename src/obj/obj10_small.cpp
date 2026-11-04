// World objects, part 10: small leaves of the world-object range: empty
// slots, deferred-destroy callbacks, chunk handlers and setters.
#include "obj10.h"

using namespace sly4;

namespace {

constexpr u32 kIff_ReadChunks = 0x00569388;           // (reader, obj)
constexpr u32 kGeomInstance_Free = 0x005A1CE0;        // (obj): back to its free list (name?)
constexpr u32 kLightInstance_Free = 0x005A3BEC;       // (obj) (name?)
constexpr u32 kEmitterInstance_Free = 0x005A4F28;     // (obj) (name?)
constexpr u32 kWorldObj_Visit583864 = 0x00583864;     // (obj, u32) (name?)

constexpr u32 kConfig = 0x010C5A20;                   // g::config
constexpr u32 kCfgLight96 = 96, kCfgLight108 = 108;   // u32 (name?)
constexpr u32 kWorldFlagA = 0x01194570;               // u8 (name?)
constexpr u32 kWorldFlagB = 0x01194550;               // u8 (name?)
constexpr u32 kWorldFlagC = 0x010DDA70;               // u8 (layers3.h's byte_da70)

constexpr u32 kObjLight = 288;                        // AglLight* (obj5.h)
constexpr u32 kObj292 = 292;
constexpr u32 kLightFlags212 = 212;                   // u8: 0x80 cleared when +292 is set (name?)
constexpr u32 kObjClass = 208;
constexpr u32 kEdSystem = 116;                        // ApeEmitterDef: the particle system
constexpr u32 kObj340 = 340;
constexpr u32 kSystem68 = 68;                         // the system's pointer to +8 of the value (name?)

}  // namespace

// Empty vtable slots: 0x00B24728 +16 / +32 (the object Bank_Create's
// 0x00586440 builds) and the bank 0x00B24770 +60, and a leaf of 0x0010B348.
DECOMP(0058AC6C, BankSub_Vfn16) {}
DECOMP(0058C538, BankSub_Vfn32) {}
DECOMP(0059AF54, Bank_Vfn60) {}
DECOMP(005A8384, Obj_Nop5A8384) {}

// void Config_CopyLight108()
// g::config +108 = +96 (see classes.h kConfigLights). r3 / r4 = the config
// and the value, as the original leaves them.
DECOMP(005913E0, Config_CopyLight108) {
    const u32 v = load<u32>(kConfig + kCfgLight96);
    store<u32>(kConfig + kCfgLight108, v);
    ctx.gpr[3] = kConfig;
    ctx.gpr[4] = v;
}

// void World_SetFlags(u8 on)
// The same byte to three globals (name?). r4..r6 = their bases as the
// original leaves them.
DECOMP(00599024, World_SetFlags) {
    const u8 v = static_cast<u8>(arg<u32>(ctx, 0));
    store<u8>(kWorldFlagA, v);
    store<u8>(kWorldFlagB, v);
    store<u8>(kWorldFlagC, v);
    ctx.gpr[4] = 0x01193060;
    ctx.gpr[5] = 0x01190000;
    ctx.gpr[6] = 0x010E0000;
}

// Deferred-destroy callbacks (DeferredDestroy's OPDs): back to the free
// lists.
DECOMP(005A1E94, GeomInstance_DeferredFree) { call(ctx, kGeomInstance_Free, {arg<u32>(ctx, 0)}); }        // OPD 0x00F3C888
DECOMP(005A3DA0, LightInstance_DeferredFree) { call(ctx, kLightInstance_Free, {arg<u32>(ctx, 0)}); }      // OPD 0x00F3C8D8
DECOMP(005A50DC, EmitterInstance_DeferredFree) { call(ctx, kEmitterInstance_Free, {arg<u32>(ctx, 0)}); }  // OPD 0x00F3C938

// Callbacks (OPDs 0x00F3C860 / 0x00F3C918, set up by 0x004A6CF8): 0x00583864(o, 0).
DECOMP(005A1524, GeomInstance_Visit0) { call(ctx, kWorldObj_Visit583864, {arg<u32>(ctx, 0), 0}); }
DECOMP(005A48B8, EmitterInstance_Visit0) { call(ctx, kWorldObj_Visit583864, {arg<u32>(ctx, 0), 0}); }

// bool Iff_HandlePvs(IffReader* r) / Iff_HandleOccp: reads the child chunks
// (no object); returns 1.
DECOMP(0059AF58, Iff_HandlePvs) {
    call(ctx, kIff_ReadChunks, {arg<u32>(ctx, 0), 0});
    ret(ctx, 1u);
}
DECOMP(0059B25C, Iff_HandleOccp) {
    call(ctx, kIff_ReadChunks, {arg<u32>(ctx, 0), 0});
    ret(ctx, 1u);
}

// void WorldObj_Set292(Obj* o, u32 v)
// o +292 = v; an object with a light (+288) clears its +212 bit 0x80.
// r3 / r4 / r5 as the original leaves them.
DECOMP(00580AF0, WorldObj_Set292) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 light = load<u32>(o + kObjLight);
    store<u32>(o + kObj292, v);
    ctx.gpr[3] = light;
    ctx.gpr[5] = o;
    if (light == 0) return;
    const u8 b = static_cast<u8>(load<u8>(light + kLightFlags212) & 0x7F);
    store<u8>(light + kLightFlags212, b);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = b;
}

// u32 EmitterInstance_Set340(EmitterInstance* o, u32 v)
// o +340 = v; the class's particle system +68 = v ? v + 8 : 0, which it
// returns.
DECOMP(005A4890, EmitterInstance_Set340) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 cls = load<u32>(o + kObjClass);
    store<u32>(o + kObj340, v);
    const u32 sys = load<u32>(cls + kEdSystem);
    const u32 r = v != 0 ? v + 8 : 0;
    store<u32>(sys + kSystem68, r);
    ctx.gpr[5] = sys;
    ctx.gpr[6] = cls;
    ret(ctx, r);
}

namespace {

constexpr u32 kFrameState = 0x010DDA74;    // draw_state.h's current_surface block: +8 u32 the frame, +20 u32 the view index (name?)
constexpr u32 kObjPerView = 304;           // u32[]: the frame each view last saw the object (name?)
constexpr u32 kMapHolderGeom = 0x00F8F144; // obj.h's kMapHolderB: the geometry instances by name hash
constexpr u32 kMapB_Find = 0x007A6BEC;
constexpr u32 kWorldList = 0x011944D8;     // PtrVector*: u16 count at +8 (name?)
constexpr u32 kWorldList_Run548508 = 0x00548508;  // (arg) (name?)
constexpr u32 kWorldList_Run548550 = 0x00548550;  // (arg, u32) (name?)
constexpr u32 kSplineRecs = 0x01194580 + 12;      // obj7.h kSplineList: 96-byte records
constexpr u32 kObjSplineIndex = 284, kSrMode = 94, kSrSize = 96;
constexpr u32 kObj_Visit583518 = 0x00583518, kObj_Visit5832FC = 0x005832FC;  // (obj) (name?)
constexpr u32 kGeomInstance_Visit5A13A0 = 0x005A13A0, kEmitterInstance_Visit58C53C = 0x0058C53C;  // (obj) (name?)

}  // namespace

// bool WorldObj_SeenThisFrame(Obj* o)
// o->per_view[current view] >= the current frame (unsigned). r4..r6 as
// the original leaves them.
DECOMP(00583EB8, WorldObj_SeenThisFrame) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 view = load<u32>(kFrameState + 20), frame = load<u32>(kFrameState + 8);
    const u32 v = load<u32>(o + kObjPerView + (view << 2));
    ctx.gpr[4] = frame;
    ctx.gpr[5] = v;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(static_cast<s32>(view << 2)));
    ret(ctx, v >= frame ? 1u : 0u);
}

// ApeGeomInstance* GeomInstance_FindByHash(u32 hash)
// The geometry instance map's value for `hash` (the default slot's on a miss).
DECOMP(005801A0, GeomInstance_FindByHash) {
    const u32 hash = arg<u32>(ctx, 0);
    Stack frame(ctx, 128 - 112);  // + call()'s linkage area = the original's 128-byte frame
    const u32 key = frame.addr();  // sp+112
    store<u32>(key, hash);
    const u32 slot = static_cast<u32>(call(ctx, kMapB_Find, {load<u32>(load<u32>(kMapHolderGeom) + 16), key}));
    ret(ctx, load<u32>(slot));
}

// void WorldList_Run548508(arg) / WorldList_Run548550(arg): the world
// list's (0x011944D8) pass when it holds anything (name?).
DECOMP(00599044, WorldList_Run548508) {
    const u32 n = load<u16>(load<u32>(kWorldList) + 8);
    ctx.gpr[4] = n;
    if (n != 0) call(ctx, kWorldList_Run548508, {arg<u32>(ctx, 0)});
}
DECOMP(00599108, WorldList_Run548550) {
    const u32 n = load<u16>(load<u32>(kWorldList) + 8);
    ctx.gpr[4] = n;
    if (n != 0) call(ctx, kWorldList_Run548550, {arg<u32>(ctx, 0), 0});
}

// The second callbacks of the 0x004A6CF8 / 0x004A6DF0 sets (OPDs 0x00F3C868
// / 0x00F3C920): 0x00583518, 0x005832FC, then the class's own pass.
DECOMP(005A1548, GeomInstance_Visit1) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, kObj_Visit583518, {o});
    call(ctx, kObj_Visit5832FC, {o});
    call(ctx, kGeomInstance_Visit5A13A0, {o});
}
DECOMP(005A48DC, EmitterInstance_Visit1) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, kObj_Visit583518, {o});
    call(ctx, kObj_Visit5832FC, {o});
    call(ctx, kEmitterInstance_Visit58C53C, {o});
}

// u8 SplineInstance_GetMode(ApeSplineInstance* o)
// Its spline record's mode (+94), or 0 without a record (+284 = -1).
DECOMP(005A8BA8, SplineInstance_GetMode) {
    const s32 idx = static_cast<s32>(load<u32>(arg<u32>(ctx, 0) + kObjSplineIndex));
    if (idx == -1) {
        ret(ctx, 0u);
        return;
    }
    const u32 base = load<u32>(kSplineRecs);
    const u32 off = static_cast<u32>(idx) * kSrSize;
    ctx.gpr[4] = base;
    ctx.gpr[5] = static_cast<u64>(static_cast<u32>(idx)) << 6;
    ret(ctx, static_cast<u32>(load<u8>(base + off + kSrMode)));
}

// PeerVec* Peer_GetVec(PeerRecord* p)
// The peer's 32-byte vector record (obj8.h PeerVec). r4..r6 as the
// original leaves them.
DECOMP(0057BE98, Peer_GetVec) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 peers = load<u32>(load<u32>(obj3::kSlotPeers));
    const u32 recs = load<u32>(peers), vecs = load<u32>(peers + 4);
    const s32 i = static_cast<s32>(p - recs) / static_cast<s32>(obj4::kPeerSize);
    ctx.gpr[4] = vecs;
    ctx.gpr[5] = static_cast<s32>(p - recs) < 0 ? 1u : 0u;  // the quotient's sign bit
    ctx.gpr[6] = recs;
    ret(ctx, vecs + (static_cast<u32>(i) << 5));
}
