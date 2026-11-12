// Game code, large-function sweep part 26: a player character switching to
// another def (tears down the old body, children and effects, rebuilds the
// "Player%02d_Controllable" query and the rigid body).
#include "big26.h"

namespace sly4::obj {

namespace {
constexpr u32 Obj_EffectHash356 = 0x000E0B64;  // Obj_EffectHash356_000E0B64 (Obj*, u32 hash, 1)
constexpr u32 World_DropPlayer = 0x0046B6CC;   // World_DropPlayer46B6CC (World*, Player*)
constexpr u32 Anim_Stop = 0x00254814;          // Anim_Stop254814 (Animator*, 0)
constexpr u32 Anim_Reset = 0x002980AC;         // Anim_Reset2980AC (Animator*, 0)
constexpr u32 Hk_RemoveEntity = 0x0092FFC8;    // (u8* out, world, body)
constexpr u32 Query_Init = 0x000CDD28;         // Query_Init0CDD28 (22 arguments; f1, f2)
constexpr u32 Obj_ApplyQuery = 0x000D2F04;     // Obj_ApplyQuery0D2F04 (Obj*, Query*)
constexpr u32 Obj_Attach = 0x000D135C;         // Obj_Attach0D135C (Obj*, child, 1)
constexpr u32 Obj_Notify24 = 0x000D949C;       // (Obj*) -> thing or 0
constexpr u32 Obj_UpdateFlags2336 = 0x0029D838;  // (thing, 0)
constexpr u32 CallK2 = 0x000D94F4;             // CallK2_000D94F4 (Obj*) -> controller or 0
constexpr u32 Owner_StartEffects = 0x002464F0;
constexpr u32 Obj_SetShadow = 0x000E0000;      // Obj_SetShadow0E0000 (Obj*, 0)
constexpr u32 Obj_PullBodyXform = 0x0024B064;  // Obj_PullBodyXform_0024B064 (Obj*)
constexpr u32 Handle_ResolveBit1 = 0x003568E0;
constexpr u32 Obj_Dispatch362 = 0x0036325C;    // (handle, 0, 1, 0)
constexpr u32 World_SetUser = 0x0047C168;      // World_SetUser47C168 (World*, user, name, 1)
constexpr u32 Model_FlagShaderParam = 0x00245F70;
constexpr u32 PtrVector_Clear = 0x0054EC1C;
constexpr u32 Nop = 0x0054EC18;

constexpr u32 kFmtControllable = 0x00AD3D10;  // "Player%02d_Controllable"
constexpr u32 kEmpty = 0x00AD3C14;            // the query's empty string argument
constexpr u32 kIdentity = 0x01164BD0;         // identity matrix
constexpr u32 kQueryVec = 0x01164B30;         // the query's vector argument
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

// A released thing (+136 flags).
struct SwThing {
    pad<136> _0;
    be_u32 flags;  // +136
};
// A child: +8 its parent during the attach, +12 its kind.
struct SwChild {
    pad<8> _0;
    be_u32 parent;  // +8
    be_s32 kind;    // +12
};
struct SwPlayer {
    pad<160> _0;
    be_u32 c160;         // +160
    be_u32 c164;         // +164
    pad<4> _168;
    be_u32 f172;         // +172 (its top byte the s8 user)
    pad<8> _176;
    be_u64 f184;         // +184
    pad<24> _192;
    be_u32 def;          // +216
    pad<28> _220;
    be_u32 children;     // +248: PtrVec<SwChild>*
    pad<104> _252;
    be_u32 t356;         // +356
    pad<36> _360;
    be_u32 ctl;          // +396
    pad<12> _400;
    be_u32 crb;          // +412
    be_u32 body;         // +416
    pad<20> _420;
    be_u32 w440;         // +440
    pad<824> _444;
    be_u32 handle;       // +1268
    pad<80> _1272;
    be_u32 t1352;        // +1352
    pad<4> _1356;
    be_u32 t1360;        // +1360
    be_u32 t1364;        // +1364
    be_u32 w1368;        // +1368
    be_u32 w1372;        // +1372
    be_u32 fx;           // +1376
    be_u32 w1380;        // +1380
};
static_assert(offsetof(SwPlayer, f184) == 184 && offsetof(SwPlayer, def) == 216 && offsetof(SwPlayer, children) == 248);
static_assert(offsetof(SwPlayer, t356) == 356 && offsetof(SwPlayer, ctl) == 396 && offsetof(SwPlayer, w440) == 440);
static_assert(offsetof(SwPlayer, handle) == 1268 && offsetof(SwPlayer, t1352) == 1352 && offsetof(SwPlayer, w1380) == 1380);
// The new def: +16 its children, +740 / +744 flags; the old def's +1020 a handle.
struct SwDef {
    pad<16> _0;
    be_u32 children;  // +16: PtrVec<SwChild>*
    pad<720> _20;
    be_u32 w740;      // +740
    be_u32 w744;      // +744
};
static_assert(offsetof(SwDef, w740) == 740);

void SetFx(SwPlayer* o, u32 h) {
    if (o->fx == h) return;
    o->fx = h;
    call(Obj_EffectHash356, o, h, 1u);
}

// A child kind the def switch detaches / attaches: 15, 2, 19, 1, 24 or 3
// (the kind re-read for each test, as the original does).
bool SwitchKind(u32 e) {
    for (s32 k : {15, 2, 19, 1, 24, 3})
        if (gm<SwChild>(e).kind == k) return true;
    return false;
}

// When set, its flags +136 |= 0x200 and the slot = 0.
bool Drop136(be_u32& slot) {
    const u32 p = slot;
    if (p == 0) return false;
    SwThing& t = gm<SwThing>(p);
    t.flags = t.flags | 0x200u;
    slot = 0u;
    return true;
}
}  // namespace

// void f(Player* o, Def* nd) (called by 00097E50, 0024BA28). Nothing without a
// current def (o->+216). Otherwise:
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
//    query's vector freed. No caller reads r3 or f1 (r3.sh).
void Player_SwitchDef_0024B1E8(SwPlayer* o, SwDef* nd) {
    const u32 old = o->def;
    if (old == 0) return;
    GuestFrame frame(448);
    u8* q = frame.as<u8>(224);  // the query (+0 its vector)

    // 1. Teardown.
    const u32 h = o->fx;
    for (const FxMap& m : kFxMap)
        if (h == m.from) {
            SetFx(o, m.to);
            break;
        }
    call(World_DropPlayer, global<be_u32>(kWorld).get(), o);
    if (vcall<u32>(o, 60) != 0) {
        call(Anim_Stop, vcall<u32>(o, 60), 0u);
        call(Anim_Reset, vcall<u32>(o, 60), 0u);
    }
    if (const u32 p = o->c164; p != 0) {
        vcall(&gm<u8>(p), 4);
        o->c164 = 0u;
    }
    if (const u32 p = o->c160; p != 0) {
        vcall(&gm<u8>(p), 40, 0u, 1u);
        const u32 c = o->c160;
        gm<be_u32>(c + 280) = 0u;
        gm<be_u32>(c + 276) = kChildKind;
        vcall(&gm<u8>(c), 8);
        o->c160 = 0u;
    }
    if (const u32 b = o->body; b != 0) {
        if (const u32 w = gm<be_u32>(b + 8); w != 0) call(Hk_RemoveEntity, frame.as<u8>(368), w, b);
        o->body = 0u;
    }
    if (const u32 p = o->crb; p != 0) {
        call(b26fn::Hk_RemoveReference, p);
        o->crb = 0u;
    }
    o->ctl = 0u;
    if (Drop136(o->t1364)) {
        o->w1368 = 0u;
        o->w1372 = 0u;
    }
    Drop136(o->t1360);
    Drop136(o->t1352);
    Drop136(o->t356);
    for (s32 i = 0;;) {
        const u32 v = o->children;
        const s32 n = v != 0 ? static_cast<s32>(gm<PtrVec<SwChild>>(v).count) : 0;
        if (i >= n) break;
        const u32 e = gm<be_u32>(gm<PtrVec<SwChild>>(v).data.ea + 4u * static_cast<u32>(i));
        if (e == 0 || !SwitchKind(e)) {
            ++i;
            continue;
        }
        vcall(&gm<u8>(e), 4);
        PtrVec<SwChild>& v2 = gm<PtrVec<SwChild>>(o->children);
        const u32 last = static_cast<u32>(v2.count) - 1u;
        v2.count = static_cast<u16>(last);
        if (static_cast<u32>(i) != (last & 0xFFFFu)) {
            const u32 data = v2.data.ea;
            gm<be_u32>(data + 4u * static_cast<u32>(i)) = gm<be_u32>(data + ((last & 0xFFFFu) << 2)).get();
        }
        // the same index is tested again
    }

    // 2. The new def's query and children.
    o->def = guest(nd);
    const s32 user = static_cast<s8>(gm<u8>(guest(o) + 172));
    const u32 ub = global<be_u32>(kWorld) + static_cast<u32>(user) * kUserBlockSize + 4208u;
    const u32 name = call<u32>(fn::String_Format, kFmtControllable, gm<u8>(ub + 1) + 1u);
    const f64 qf = lf(global<be_f32>(kConstQuery));
    call(Query_Init, q, 4u, 128u, name, nd, 0u, kEmpty, kIdentity, kQueryVec, 1u, 0u, 0u, 1u, 0u, 0u, 0u, 0u, 0u, 0xFFFFFFFFu, 0u, qf, qf);
    call(Obj_ApplyQuery, o, q);
    for (u32 i = 0;; ++i) {
        const u32 v = gm<SwDef>(o->def).children;
        const u32 n = v != 0 ? gm<PtrVec<SwChild>>(v).count : 0u;
        if (i >= n) break;
        const u32 e = gm<be_u32>(gm<PtrVec<SwChild>>(v).data.ea + 4 * i);
        if (e == 0 || !SwitchKind(e)) continue;
        gm<SwChild>(e).parent = guest(o);
        call(Obj_Attach, o, e, 1u);
        gm<SwChild>(e).parent = 0u;
    }

    // 3. Rebuild.
    if (const u32 r = call<u32>(Obj_Notify24, o); r != 0) call(Obj_UpdateFlags2336, r, 0u);
    o->w440 = 0u;
    if (o->def != 0u) {
        u32 hi = 0;
        if (const u32 b = o->body; b != 0 && gm<be_u32>(b + 8) != 0u) hi = gm<be_u32>(b + 44) >> 16;
        const u32 k = call<u32>(CallK2, o);
        o->ctl = k;
        if (k != 0) gm<be_u32>(k + 1240) = hi;
        o->w1380 = 0u;
    }
    call(Owner_StartEffects, o);
    Char_CreateRigidBody_00247068(reinterpret_cast<CharObj*>(o));
    call(Obj_SetShadow, o, 0u);
    call(Obj_PullBodyXform, o);
    SetFx(o, kFxDefault);
    o->f172 = (o->f172 & ~0x00C00000u) | ((nd->w740 << 22) & 0x00C00000u);
    const bool b20 = (nd->w744 >> 31) != 0;
    const u32 hold = gm<be_u32>(old + 1020);
    const u64 f184 = o->f184;
    o->f184 = b20 ? (f184 | 0x20u) : (f184 & ~0x20ull);
    const u32 hd = call<u32>(Handle_ResolveBit1, hold);
    if (const u32 cur = o->handle; hd != cur) {
        call(Obj_Dispatch362, cur, 0u, 1u, 0u);  // on the old handle
        o->handle = hd;
    }
    const u32 game = global<be_u32>(kWorld);
    const u32 d = o->def;
    const s32 u = static_cast<s8>(gm<u8>(guest(o) + 172));
    const u32 un = vcall<u32>(&gm<u8>(d), 8);
    call(World_SetUser, game, static_cast<u32>(u), un, 1u);  // the user zero-extended from 32 bits, as the original
    call(Model_FlagShaderParam, o);
    if (const u32 vec = gm<be_u32>(guest(q)); vec != 0) {
        call(PtrVector_Clear, vec);
        call(Nop, vec);
        call(fn::SmallFree, gm<be_u32>(guest(q)).get(), 20u);
    }
}

}  // namespace sly4::obj

NATIVE(0024B1E8, sly4::obj::Player_SwitchDef_0024B1E8)
