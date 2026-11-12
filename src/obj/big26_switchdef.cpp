// Game code, large-function sweep part 26: a player character switching to
// another def (tears down the old body, children and effects, rebuilds the
// "Player%02d_Controllable" query and the rigid body).
#include "../sly4.h"
#include "big26.h"

using namespace sly4;
using namespace sly4::big26;

namespace {
constexpr u32 Obj_EffectHash356 = 0x000E0B64;  // Obj_EffectHash356_000E0B64 (Obj*, u32 hash, 1)
constexpr u32 World_DropPlayer = 0x0046B6CC;   // World_DropPlayer46B6CC (World*, Player*)
constexpr u32 Anim_Stop = 0x00254814;          // Anim_Stop254814 (Animator*, 0)
constexpr u32 Anim_Reset = 0x002980AC;         // Anim_Reset2980AC (Animator*, 0)
constexpr u32 Hk_RemoveEntity = 0x0092FFC8;    // (u8* out, world, body)
constexpr u32 String_Format = 0x005534D0;      // (fmt, ...) -> const char*
constexpr u32 Query_Init = 0x000CDD28;         // Query_Init0CDD28 (22 arguments; f1, f2)
constexpr u32 Obj_ApplyQuery = 0x000D2F04;     // Obj_ApplyQuery0D2F04 (Obj*, Query*)
constexpr u32 Obj_Attach = 0x000D135C;         // Obj_Attach0D135C (Obj*, child, 1)
constexpr u32 Obj_Notify24 = 0x000D949C;       // (Obj*) -> thing or 0
constexpr u32 Obj_UpdateFlags2336 = 0x0029D838;  // (thing, 0)
constexpr u32 CallK2 = 0x000D94F4;             // CallK2_000D94F4 (Obj*) -> controller or 0
constexpr u32 Owner_StartEffects = 0x002464F0;
constexpr u32 Char_CreateRigidBody = 0x00247068;  // Char_CreateRigidBody_00247068 (big26_charbody.cpp)
constexpr u32 Obj_SetShadow = 0x000E0000;      // Obj_SetShadow0E0000 (Obj*, 0)
constexpr u32 Obj_PullBodyXform = 0x0024B064;  // Obj_PullBodyXform_0024B064 (Obj*; f1 left set)
constexpr u32 Handle_ResolveBit1 = 0x003568E0;
constexpr u32 Obj_Dispatch362 = 0x0036325C;    // (handle, 0, 1, 0)
constexpr u32 World_SetUser = 0x0047C168;      // World_SetUser47C168 (World*, user, name, 1)
constexpr u32 Model_FlagShaderParam = 0x00245F70;
constexpr u32 PtrVector_Clear = 0x0054EC1C;
constexpr u32 Nop = 0x0054EC18;
constexpr u32 SmallFree = 0x00552420;          // (p, size)

constexpr u32 kFmtControllable = 0x00AD3D10;  // "Player%02d_Controllable"
constexpr u32 kEmpty = 0x00AD3C14;            // the query's empty string argument
constexpr u32 kIdentity = 0x01164BD0;         // identity matrix
constexpr u32 kZeroVec = 0x01164B30;
constexpr u32 kConstQuery = 0x0024B1DC;       // f32 the query's two floats; +4 the PullBodyXform f1
constexpr u32 kUserBlockSize = 0x20C80;       // Game +4208 + user * 0x20C80: per-user block (+1 its number)
constexpr u32 kFxDefault = 0x3F2950FFu;       // the +1376 effect set at the end
constexpr u32 kChildKind = 0x02235C33u;       // o->+160's +276 kind once released

// The effect hash pairs: a +1376 of `from` becomes `to` (with
// Obj_EffectHash356_000E0B64(o, to, 1)) before the teardown.
struct FxMap {
    u32 from, to;
};
constexpr FxMap kFxMap[4] = {
    {0xCE2E6B3Du, 0x9B04BE36u}, {0x594B024Bu, 0x273F0E72u}, {0xE15D38C8u, 0x4090D96Fu}, {0xAB6ADA7Au, 0x32AB4EB3u}};

void SetFx(PPUContext& ctx, u32 o, u32 h) {
    if (load<u32>(o + 1376) == h) return;
    store<u32>(o + 1376, h);
    call(ctx, Obj_EffectHash356, {o, h, 1});
}

// A child kind the def switch detaches / attaches: 15, 2, 19, 1, 24 or 3
// (the word +12 re-read for each test, as the original does).
bool SwitchKind(u32 e) {
    for (s32 k : {15, 2, 19, 1, 24, 3})
        if (load<s32>(e + 12) == k) return true;
    return false;
}

// o->+X: when set, its flags +136 |= 0x200 and o->+X = 0.
bool Drop136(u32 o, u32 off) {
    const u32 p = load<u32>(o + off);
    if (p == 0) return false;
    store<u32>(p + 136, load<u32>(p + 136) | 0x200u);
    store<u32>(o + off, 0);
    return true;
}
}  // namespace

// T* f(Player* o, Def* nd) (called by 00097E50, 0024BA28). Without a
// current def (o->+216) returns o at once. Otherwise:
// 1. A +1376 effect in kFxMap is switched to its pair; the player leaves
//    the world lists; with an animator, its layer 0 is stopped and reset;
//    o->+164 is released (vfn[4]), o->+160 detached (vfn[40](0, 1), +280 =
//    0, +276 = 0x02235C33, vfn[8]); the rigid body o->+416 is removed from
//    its world and o->+412 released; +396 = 0; +1364 (with +1368 /
//    +1372), +1360, +1352 and +356 get flag 0x200 and are cleared; the
//    children of o->+248 of the switch kinds are released (vfn[4]) and
//    swap-removed.
// 2. o->+216 = nd; the "Player%02d_Controllable" query of the user o->+172
//    (s8) is built (Query_Init0CDD28) and applied; nd's +16 children of
//    the switch kinds are attached (with their +8 = o during the call).
// 3. Obj_Notify24 / Obj_UpdateFlags2336, +440 = 0; with a def the
//    controller (CallK2) into +396 gets the body's +44 high half at +1240,
//    +1380 = 0; effects, Char_CreateRigidBody_00247068, shadow off, the
//    body transform pulled; effect 0x3F2950FF; o->+172 bits 0x00C00000
//    from nd->+740, o->+184 bit 0x20 from nd->+744 bit 31; the old def's
//    +1020 handle resolved into +1268 (Obj_Dispatch362(old one, 0, 1, 0) when it changed);
//    World_SetUser47C168(user, nd->vfn[8]()); Model_FlagShaderParam; the
//    query's vector freed. Returns what the last call leaves.
DECOMP(0024B1E8, Player_SwitchDef_0024B1E8) {
    const u32 o = arg<u32>(ctx, 0), nd = arg<u32>(ctx, 1);
    const u32 old = load<u32>(o + 216);
    if (old == 0) return;  // r3 = o
    Stack frame(ctx, 448);
    const u32 sp = frame.addr();
    const u32 q = sp + 224;  // the query

    // 1. Teardown.
    const u32 h = load<u32>(o + 1376);
    for (const FxMap& m : kFxMap)
        if (h == m.from) {
            SetFx(ctx, o, m.to);
            break;
        }
    call(ctx, World_DropPlayer, {load<u32>(b26fn::Game), o});
    if (static_cast<u32>(VCall(ctx, o, 60, {o})) != 0) {
        const u32 a1 = static_cast<u32>(VCall(ctx, o, 60, {o}));
        call(ctx, Anim_Stop, {a1, 0});
        const u32 a2 = static_cast<u32>(VCall(ctx, o, 60, {o}));
        call(ctx, Anim_Reset, {a2, 0});
    }
    if (const u32 p = load<u32>(o + 164); p != 0) {
        VCall(ctx, p, 4, {p});
        store<u32>(o + 164, 0);
    }
    if (const u32 p = load<u32>(o + 160); p != 0) {
        VCall(ctx, p, 40, {p, 0, 1});
        const u32 c = load<u32>(o + 160);
        store<u32>(c + 280, 0);
        store<u32>(c + 276, kChildKind);
        VCall(ctx, c, 8, {c});
        store<u32>(o + 160, 0);
    }
    if (const u32 b = load<u32>(o + 416); b != 0) {
        if (const u32 w = load<u32>(b + 8); w != 0) call(ctx, Hk_RemoveEntity, {sp + 368, w, b});
        store<u32>(o + 416, 0);
    }
    if (const u32 p = load<u32>(o + 412); p != 0) {
        call(ctx, b26fn::Hk_RemoveReference, {p});
        store<u32>(o + 412, 0);
    }
    store<u32>(o + 396, 0);
    if (Drop136(o, 1364)) {
        store<u32>(o + 1368, 0);
        store<u32>(o + 1372, 0);
    }
    Drop136(o, 1360);
    Drop136(o, 1352);
    Drop136(o, 356);
    for (s32 i = 0;;) {
        const u32 v = load<u32>(o + 248);
        const s32 n = v != 0 ? static_cast<s32>(load<u16>(v + 8)) : 0;
        if (i >= n) break;
        const u32 e = load<u32>(load<u32>(v) + 4u * static_cast<u32>(i));
        if (e == 0 || !SwitchKind(e)) {
            ++i;
            continue;
        }
        VCall(ctx, e, 4, {e});
        const u32 v2 = load<u32>(o + 248);
        const u32 last = static_cast<u32>(load<u16>(v2 + 8)) - 1u;
        store<u16>(v2 + 8, static_cast<u16>(last));
        if (static_cast<u32>(i) != (last & 0xFFFFu)) {
            const u32 data = load<u32>(v2);
            store<u32>(data + 4u * static_cast<u32>(i), load<u32>(data + ((last & 0xFFFFu) << 2)));
        }
        // the same index is tested again
    }

    // 2. The new def's query and children.
    store<u32>(o + 216, nd);
    const s32 user = static_cast<s8>(load<u8>(o + 172));
    const u32 ub = load<u32>(b26fn::Game) + static_cast<u32>(user) * kUserBlockSize + 4208u;
    const u32 name = static_cast<u32>(call(ctx, String_Format, {kFmtControllable, load<u8>(ub + 1) + 1u, ub}));
    const f64 qf = Lf(kConstQuery);
    // Arguments 20 / 21 (r1 +200 / +208) are never stored by the original;
    // the callee does not read them.
    callf(ctx, Query_Init, {q, 4, 128, name, nd, 0, kEmpty, kIdentity, kZeroVec, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0xFFFFFFFFu, 0, 0, 0}, {qf, qf});
    call(ctx, Obj_ApplyQuery, {o, q});
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(load<u32>(o + 216) + 16);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0;
        if (i >= n) break;
        const u32 e = load<u32>(load<u32>(v) + 4 * i);
        if (e == 0 || !SwitchKind(e)) continue;
        store<u32>(e + 8, o);
        call(ctx, Obj_Attach, {o, e, 1});
        store<u32>(e + 8, 0);
    }

    // 3. Rebuild.
    if (const u32 r = static_cast<u32>(call(ctx, Obj_Notify24, {o})); r != 0) call(ctx, Obj_UpdateFlags2336, {r, 0});
    const f64 pull = Lf(kConstQuery + 4);
    store<u32>(o + 440, 0);
    if (load<u32>(o + 216) != 0) {
        u32 hi = 0;
        if (const u32 b = load<u32>(o + 416); b != 0 && load<u32>(b + 8) != 0) hi = load<u32>(b + 44) >> 16;
        const u32 k = static_cast<u32>(call(ctx, CallK2, {o}));
        store<u32>(o + 396, k);
        if (k != 0) store<u32>(k + 1240, hi);
        store<u32>(o + 1380, 0);
    }
    call(ctx, Owner_StartEffects, {o});
    call(ctx, Char_CreateRigidBody, {o});
    call(ctx, Obj_SetShadow, {o, 0});
    ctx.fpr[1] = pull;  // f1 as the original sets it (the callee reads only r3)
    call(ctx, Obj_PullBodyXform, {o});
    SetFx(ctx, o, kFxDefault);
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x00C00000u) | ((load<u32>(nd + 740) << 22) & 0x00C00000u));
    const bool b20 = (load<u32>(nd + 744) >> 31) != 0;
    const u32 hold = load<u32>(old + 1020);
    const u64 f184 = load<u64>(o + 184);
    store<u64>(o + 184, b20 ? (f184 | 0x20u) : (f184 & ~0x20ull));
    const u32 hd = static_cast<u32>(call(ctx, Handle_ResolveBit1, {hold}));
    if (const u32 cur = load<u32>(o + 1268); hd != cur) {
        call(ctx, Obj_Dispatch362, {cur, 0, 1, 0});  // on the old handle
        store<u32>(o + 1268, hd);
    }
    const u32 game = load<u32>(b26fn::Game);
    const u32 d = load<u32>(o + 216);
    const u32 u = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(o + 172))));
    const u32 un = static_cast<u32>(VCall(ctx, d, 8, {d}));
    call(ctx, World_SetUser, {game, u, un, 1});
    call(ctx, Model_FlagShaderParam, {o});
    if (const u32 vec = load<u32>(q); vec != 0) {
        call(ctx, PtrVector_Clear, {vec});
        call(ctx, Nop, {vec});
        call(ctx, SmallFree, {load<u32>(q), 20});
    }
}
