// World_HandleEvent callees, part 11f: group child placement and the HUD
// object's effect registration.
#include <cmath>

#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent11;

namespace {
// Group: +0 WorldObj*[] (+8 u16 count), +20 32-byte child records (+28 u16
// count). Record: +0 vec3 offset, +16 Object*. Object +412 flags (bit 31:
// no body), +416 body.
constexpr u32 kGrpObjs = 0, kGrpObjCount = 8, kGrpRecs = 20, kGrpRecCount = 28, kGrpRecSize = 32;
constexpr u32 kRecObject = 16;
constexpr u32 kObjNoBody = 412, kObjNoBodyBit = 0x80000000, kObjBody = 416;
constexpr u32 kWorldObjPos = 64;             // WorldObj +64 vec3 position
constexpr u32 kSetMatrix = 84, kSetActive = 304;  // Object vtable slots: (Object*, const Mat34*), (Object*, u8)
constexpr u32 kWorldPhysics = 0x001281D4;    // World + this: PhysicsWorld* (+4 Havok world)
constexpr u32 kBodyAdd = 0x00931D10;         // Body_Add931D10 (Havok world, body): Havok range
constexpr u32 kMarkDirty = h7fn::WorldObj_MarkDirty;

f32 wf(u32 w) { return std::bit_cast<f32>(w); }

// HUD object: +8 def (+68 name hash), +12 u32 name hash, +16 effect name,
// +20 Effect*, +24 / +28 script hash / value.
constexpr u32 kHudDef = 8, kDefName = 68, kHudName = 12, kHudFxName = 16, kHudFx = 20;
constexpr u32 kHudScript = 24, kHudScriptArg = 28;
constexpr u32 kEffectSpawn = 0x004A5FC8;     // Effect_Spawn (name, owner, value, u8 start)
constexpr u32 kEffectSetTime = 0x004CF710;   // Effect_SetTime (Effect*; f32)
constexpr u32 kEffectHasCommands = 0x004B95F0;  // (def) -> non-zero (name?)
constexpr u32 kHashToString = 0x0056A6E0;    // Hash_ToString (u32) -> const char*
constexpr u32 kStringFormat = 0x005534D0;
constexpr u32 kFmtNoCommands = 0x00ADC960;   // "hud object '%s' lifetime script '%s' has no commands"
constexpr u32 kEmptyString = 0x00B0F050, kUnknownString = 0x00B0F051;  // "" / "unknown"
constexpr u32 kHudStartTime = 0x0035BD04;    // f32 (0x3C88CE70, about 1/60)
constexpr u32 kWorldHud = 0x001281E0;        // World + this: HUD state (+160 script, +164 value) (name?)
constexpr u32 kCompHud = 0x760CB725, kCompScript = 0x0027469E;  // component type hashes
constexpr u32 kCompState = 32, kCompRelease = 8;  // vtable slots: () -> s32 state, release
constexpr s32 kCompReady = 5;
constexpr u32 kCompScriptRef = 28, kCompArg = 36;
constexpr u32 kScriptRefs = 4;               // script +4 u16 reference count
constexpr u32 kEffectFlags = 136, kEffectFlagAttached = 0x200;

// Points a ready component at script (released old one first, reference
// counted) with arg.
bool set_script(PPUContext& ctx, u32 comp, u32 script_src, u32 arg_src) {
    if (static_cast<s32>(vcall(ctx, comp, kCompState, {comp})) != kCompReady) return false;
    const u32 old = load<u32>(comp + kCompScriptRef);
    const u32 script = load<u32>(script_src);
    if (old != 0) vcall(ctx, old, kCompRelease, {old});
    store<u32>(comp + kCompScriptRef, script);
    if (script != 0) store<u16>(script + kScriptRefs, static_cast<u16>(load<u16>(script + kScriptRefs) + 1));
    store<u32>(comp + kCompArg, load<u32>(arg_src));
    return true;
}
}  // namespace

// void Child_Place31D4A4(Group* g, const Mat34* m) (name?): moves the
// group's world objects to m's translation (marked dirty) and each child
// record's object to m with the record's offset transformed by m (vmaddfp
// lanes), then adds its body to the physics world and activates it unless
// the object has none (+412 bit 31).
DECOMP(0031D4A4, Child_Place31D4A4) {
    const u32 g = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 i = 0, off = 0; i < load<u16>(g + kGrpObjCount); ++i, off += 4) {
        const u32 w = load<u32>(load<u32>(g + kGrpObjs) + off);
        if (w == 0) continue;
        for (u32 k = 0; k < 3; ++k) store<u32>(w + kWorldObjPos + k * 4, load<u32>(m + 48 + k * 4));
        ctx.fpr[1] = lfs_f64(m + 56);  // left in f1 by the original's copy
        call(ctx, kMarkDirty, {w});
    }
    u32 n;
    for (u32 j = 0, off = 0; j < (n = load<u16>(g + kGrpRecCount)); ++j, off += kGrpRecSize) {
        const u32 rec = load<u32>(g + kGrpRecs) + off;
        // r1+112 transformed offset, r1+128 the Mat34 copy (4 rows of 16;
        // the fourth lanes are left as they are).
        Stack frame(ctx, 80);
        const u32 mat = frame.addr() + 16;
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) store<u32>(mat + r * 16 + k * 4, load<u32>(m + r * 16 + k * 4));
        const f32 x = wf(load<u32>(rec)), y = wf(load<u32>(rec + 4)), z = wf(load<u32>(rec + 8));
        for (u32 k = 0; k < 3; ++k) {
            f32 v = std::fma(wf(load<u32>(mat + k * 4)), x, 0.0f);
            v = std::fma(wf(load<u32>(mat + 16 + k * 4)), y, v);
            v = std::fma(wf(load<u32>(mat + 32 + k * 4)), z, v);
            v = wf(load<u32>(mat + 48 + k * 4)) + v;
            store<f32>(frame.addr() + k * 4, v);
        }
        for (u32 k = 0; k < 3; ++k) store<u32>(mat + 48 + k * 4, load<u32>(frame.addr() + k * 4));
        ctx.fpr[1] = lfs_f64(frame.addr() + 8);  // left in f1 / f2 by the original's copy
        ctx.fpr[2] = lfs_f64(frame.addr() + 4);
        const u32 o = load<u32>(rec + kRecObject);
        vcall(ctx, o, kSetMatrix, {o, mat});
        const u32 o2 = load<u32>(rec + kRecObject);
        if ((load<u32>(o2 + kObjNoBody) & kObjNoBodyBit) != 0) continue;
        call(ctx, kBodyAdd, {load<u32>(load<u32>(load<u32>(kCurrentWorld) + kWorldPhysics) + 4), load<u32>(o2 + kObjBody)});
        const u32 o3 = load<u32>(rec + kRecObject);
        vcall(ctx, o3, kSetActive, {o3, 1});
    }
    ret(ctx, n);  // r3 is left as the record count
}

// void Obj_Register35BD08(HudObject* h) (name?): spawns h's effect (+16,
// with the script +24 / +28, started) into +20; points its HUD component
// (0x760CB725) at the world's HUD script and its script component
// (0x0027469E) at h's script when they are ready (state 5); then clears the
// effect's attached bit, sets its time, and reports "hud object '%s'
// lifetime script '%s' has no commands" when the effect def has none. r3 is
// left as the original leaves it.
DECOMP(0035BD08, Obj_Register35BD08) {
    const u32 h = arg<u32>(ctx, 0);
    const u32 fxname = load<u32>(h + kHudFxName);
    if (fxname == 0) return ret(ctx, h);
    const u32 e = static_cast<u32>(call(ctx, kEffectSpawn, {fxname, load<u32>(h + kHudScript), load<u32>(h + kHudScriptArg), 1}));
    store<u32>(h + kHudFx, e);
    if (e == 0) return ret(ctx, 0u);
    const u32 hud = load<u32>(load<u32>(kCurrentWorld) + kWorldHud);
    if (hud != 0) {
        const u32 c = static_cast<u32>(call(ctx, h7fn::Effect_FindComponent, {e, kCompHud}));
        if (c != 0) set_script(ctx, c, hud + 160, hud + 164);
    }
    if (load<u32>(h + kHudScript) != 0) {
        const u32 c = static_cast<u32>(call(ctx, h7fn::Effect_FindComponent, {load<u32>(h + kHudFx), kCompScript}));
        if (c != 0) set_script(ctx, c, h + kHudScript, h + kHudScriptArg);
    }
    const u32 fx = load<u32>(h + kHudFx);
    store<u32>(fx + kEffectFlags, load<u32>(fx + kEffectFlags) & ~kEffectFlagAttached);
    callf(ctx, kEffectSetTime, {load<u32>(h + kHudFx)}, {lfs_f64(kHudStartTime)});
    const u32 def = load<u32>(load<u32>(h + kHudFx));
    if (def != 0) {
        const u32 r = static_cast<u32>(call(ctx, kEffectHasCommands, {def}));
        if (r != 0) return ret(ctx, r);
    }
    u32 name = load<u32>(h + kHudName);
    if (name == 0) {
        const u32 d = load<u32>(h + kHudDef);
        name = d != 0 ? load<u32>(d + kDefName) : 0u;
    }
    const u32 s = static_cast<u32>(call(ctx, kHashToString, {name}));
    const u32 what = load<u32>(load<u32>(h + kHudFx)) != 0 ? kEmptyString : kUnknownString;
    call(ctx, kStringFormat, {kFmtNoCommands, s, what});
}
