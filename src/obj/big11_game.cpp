// Game code, large-function sweep part 11: player re-initialisation.
#include "big11.h"

using namespace sly4;
using namespace sly4::big11;

namespace {
constexpr u32 kEffectHash = 0x000E0B64u;   // Obj_EffectHash356_000E0B64 (o, hash, 1)
constexpr u32 kDropPlayer = 0x0046B6CCu;   // World_DropPlayer46B6CC (world, o)
constexpr u32 kAnimStop = 0x00254814u;     // Anim_Stop254814 (anim, 0)
constexpr u32 kAnimReset = 0x002980ACu;    // Anim_Reset2980AC (anim, 0)
constexpr u32 kDropLinks = 0x002349E0u;    // (o) (name?)
constexpr u32 kEmitterStop = 0x00443424u;  // Emitter_Stop (emitter, 1)
constexpr u32 kRefRelease = 0x004432DCu;   // RefNode_Release (node)
constexpr u32 kRemoveEntity = 0x0092FFC8u; // Hk_RemoveEntity92FFC8 (out, entity, skel) (Havok wrapper)
constexpr u32 kSkelRelease = 0x007D4EA8u;  // AnimSkel_Release7D4EA8 (skel)
constexpr u32 kFormat = 0x005534D0u;       // String_Format
constexpr u32 kQueryInit = 0x000CDD28u;    // Query_Init0CDD28 (22 arguments; f1, f2)
constexpr u32 kApplyQuery = 0x000D2F04u;   // Obj_ApplyQuery0D2F04 (o, Query*)
constexpr u32 kAttach = 0x000D135Cu;       // Obj_Attach0D135C (o, child, 1)
constexpr u32 kSetup231470 = 0x00231470u;  // (o) (name?)
constexpr u32 kFindByKey = 0x000D1454u;    // Obj_FindByKey_000D1454 (o, key)
constexpr u32 kCallK2 = 0x000D94F4u;       // CallK2_000D94F4 (o)
constexpr u32 kNodePush = 0x00446984u;     // Node_PushF65B7C (name hash, set)
constexpr u32 kSetup231928 = 0x00231928u;  // (o) (name?)
constexpr u32 kSetup230CA0 = 0x00230CA0u;  // (o) (name?)
constexpr u32 kSetup232F00 = 0x00232F00u;  // (o, +612) (name?)
constexpr u32 kSetShadow = 0x000E0000u;    // Obj_SetShadow0E0000 (o, 0)
constexpr u32 kSetRate = 0x002371F8u;      // (o; f32) (name?)
constexpr u32 kSetRate2 = 0x00236868u;     // (o, def + 1528; f32, f32) (name?)
constexpr u32 kResolve = 0x003568E0u;      // Handle_ResolveBit1 (handle)
constexpr u32 kDispatch = 0x0036325Cu;     // Obj_Dispatch362 (old handle, 0, 1, 0)
constexpr u32 kSetUser = 0x0047C168u;      // World_SetUser47C168 (world, s8 port, user, 1)
constexpr u32 kVecClear = 0x0054EC1Cu;     // PtrVector_Clear
constexpr u32 kNop = 0x0054EC18u;          // Nop
constexpr u32 kSmallFree = 0x00552420u;    // SmallFree (p, size)
constexpr u32 kWorldPtr = 0x00F65CA0u;
constexpr u32 kFmtControllable = 0x00AD3D10u;   // "Player%02d_Controllable"
constexpr u32 kEmpty = 0x00AD3C14u;
constexpr u32 kIdentity = 0x01164BD0u;
constexpr u32 kOnes = 0x01164B30u;
constexpr u32 kK = 0x00238268u;            // .text f32 x2

// The kinds of attached children the reset detaches / re-attaches.
bool ChildKind(u32 e) {
    for (u32 k : {15u, 2u, 19u, 1u, 24u, 3u})
        if (load<u32>(e + 12) == k) return true;
    return false;
}

// +1012 = hash (and the effect switched) unless it already is.
void SetEffectHash(PPUContext& ctx, u32 o, u32 hash) {
    store<u32>(o + 1012, hash);
    call(ctx, kEffectHash, {o, hash, 1, hash});
}

// Mark a released node (+136 |= 0x200).
void MarkDead(u32 n) { store<u32>(n + 136, load<u32>(n + 136) | 0x200u); }
}  // namespace

// u32 Player_ResetFromDef238270(Player* o, PlayerDef* def) (name?): the
// pending effect state (+1012) advances (0xCE2E6B3D -> 0x9B04BE36,
// 0x594B024B -> 0x273F0E72, 0xE15D38C8 -> 0x4090D96F, 0xAB6ADA7A ->
// 0x32AB4EB3); the player is dropped from the world, its anim stopped and
// reset, helpers / emitters / ref nodes / Havok skeleton released and the
// attached children of kinds 15/2/19/1/24/3 detached; then it is rebuilt
// from def (+396 / +216): the "Player%02d_Controllable" query applied,
// children re-attached, key objects and ref nodes looked up again, rates
// set, +1012 = 0x3F2950FF, flags from def +740 / +744, the previous controller
// handle (+784) dispatched when it changes and the world user set.
DECOMP(00238270, Player_ResetFromDef238270) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 def = arg<u32>(ctx, 1);
    Stack frame(ctx, 272);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    switch (load<u32>(o + 1012)) {
    case 0xCE2E6B3Du: SetEffectHash(ctx, o, 0x9B04BE36u); break;
    case 0x594B024Bu: SetEffectHash(ctx, o, 0x273F0E72u); break;
    case 0xE15D38C8u: SetEffectHash(ctx, o, 0x4090D96Fu); break;
    case 0xAB6ADA7Au: SetEffectHash(ctx, o, 0x32AB4EB3u); break;
    default: break;
    }
    call(ctx, kDropPlayer, {load<u32>(kWorldPtr), o});
    if (static_cast<u32>(VCall(ctx, o, 60, {o})) != 0) {
        call(ctx, kAnimStop, {VCall(ctx, o, 60, {o}), 0});
        call(ctx, kAnimReset, {VCall(ctx, o, 60, {o}), 0});
    }
    if (const u32 h = load<u32>(o + 164); h != 0) {
        VCall(ctx, h, 4, {h});
        store<u32>(o + 164, 0);
    }
    call(ctx, kDropLinks, {o});
    if (const u32 q = load<u32>(o + 160); q != 0) {
        VCall(ctx, q, 40, {q, 0, 1});
        const u32 q2 = load<u32>(o + 160);
        store<u32>(q2 + 280, 0);
        store<u32>(q2 + 276, 0x02235C33u);
        VCall(ctx, q2, 8, {q2});
        store<u32>(o + 160, 0);
    }
    store<u32>(o + 744, 0);
    store<u32>(o + 948, 0);
    for (u32 k = 0; k < 3; ++k) call(ctx, kEmitterStop, {load<u32>(o + 864 + 4 * k), 1});
    for (u32 off : {896u, 916u, 920u, 924u, 928u}) call(ctx, kRefRelease, {load<u32>(o + off)});
    for (u32 k = 0; k < 4; ++k) call(ctx, kRefRelease, {load<u32>(o + 900 + 4 * k)});
    if (const u32 n = load<u32>(o + 748); n != 0) {
        MarkDead(n);
        store<u32>(o + 748, 0);
        store<u32>(o + 752, 0);
        store<u32>(o + 756, 0);
    }
    for (u32 off : {768u, 776u})
        if (const u32 n = load<u32>(o + off); n != 0) {
            MarkDead(n);
            store<u32>(o + off, 0);
        }
    if (u32 sk = load<u32>(o + 672); sk != 0) {
        if (const u32 ent = load<u32>(sk + 8); ent != 0) {
            call(ctx, kRemoveEntity, {sp + 368, ent, sk, ent});
            sk = load<u32>(o + 672);
        }
        call(ctx, kSkelRelease, {sk});
        store<u32>(o + 672, 0);
    }
    if (const u32 n = load<u32>(o + 356); n != 0) {
        MarkDead(n);
        store<u32>(o + 356, 0);
    }
    // Detach the children (vector +248): the last one moves into the freed
    // slot, which is examined again.
    for (u32 i = 0, off = 0;; i += 1, off += 4) {
        const u32 v = load<u32>(o + 248);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0;
        if (static_cast<s32>(i) >= static_cast<s32>(n)) break;
        const u32 e = load<u32>(load<u32>(v) + off);
        if (e == 0 || !ChildKind(e)) continue;
        VCall(ctx, e, 4, {e});
        const u32 v2 = load<u32>(o + 248);
        const u32 c = load<u16>(v2 + 8) - 1;
        store<u16>(v2 + 8, static_cast<u16>(c));
        if (i != (c & 0xFFFFu)) {
            const u32 d = load<u32>(v2);
            store<u32>(d + off, load<u32>(d + ((c << 2) & 0x3FFFCu)));
        }
        i -= 1;
        off -= 4;
    }

    store<u32>(o + 396, def);
    const u32 port = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(o + 172))));
    store<u32>(o + 216, def);
    const u32 slot = load<u32>(kWorldPtr) + port * 0x20C80u + 4208;
    const u32 name = static_cast<u32>(call(ctx, kFormat, {kFmtControllable, load<u8>(slot + 1) + 1u}));
    const f64 k0 = LfsAt(kK);
    const u32 q = sp + 224;   // Query
    // arguments 20 / 21 (r1 +200 / +208) are not written; the callee does not read them
    callf(ctx, kQueryInit, {q, 4, 128, name, def, 0, kEmpty, kIdentity, kOnes, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0xFFFFFFFFu, 0, 0, 0},
          {k0, k0});
    call(ctx, kApplyQuery, {o, q});
    for (u32 j = 0, off = 0;; j += 1, off += 4) {
        const u32 v = load<u32>(load<u32>(o + 216) + 16);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0;
        if (j >= n) break;
        const u32 e = load<u32>(load<u32>(v) + off);
        if (e == 0 || !ChildKind(e)) continue;
        store<u32>(e + 8, o);
        call(ctx, kAttach, {o, e, 1});
        store<u32>(e + 8, 0);
    }
    call(ctx, kSetup231470, {o});
    const u32 key = static_cast<u32>(call(ctx, kFindByKey, {o, load<u32>(load<u32>(o + 396) + 824 + 24)}));
    store<u32>(o + 744, key);
    const u32 k2 = static_cast<u32>(call(ctx, kCallK2, {o}));
    const u32 w616 = load<u32>(o + 616);
    store<u32>(o + 948, k2);
    if (k2 != 0) {
        if (const u32 a = load<u32>(o + 744); a != 0) store<u32>(k2 + 1216, a);
        store<u32>(k2 + 1240, w616);
    }
    {
        const u32 base = load<u32>(o + 396) + 748;   // {set, name hashes...}
        static constexpr u32 kSlots[5] = {896, 916, 920, 924, 928};
        for (u32 k = 0; k < 5; ++k)
            store<u32>(o + kSlots[k], static_cast<u32>(call(ctx, kNodePush, {load<u32>(base + 36 + 4 * k), load<u32>(base)})));
        for (u32 k = 0; k < 4; ++k)
            store<u32>(o + 900 + 4 * k, static_cast<u32>(call(ctx, kNodePush, {load<u32>(base + 4 + 8 * k), load<u32>(base)})));
    }
    call(ctx, kSetup231928, {o});
    call(ctx, kSetup230CA0, {o});
    call(ctx, kSetup232F00, {o, load<u32>(o + 612)});
    call(ctx, kSetShadow, {o, 0});
    CopyW(slot + 104, load<u32>(o + 396) + 1272 + 68);
    const f64 k1 = LfsAt(kK + 4);
    callf(ctx, kSetRate, {o}, {k1});
    bool rate2 = true;
    if (const u32 s = load<u32>(o + 612); s != 0) {
        if (const u32 t = load<u32>(s + 28); t != 0) {
            const u32 n = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(t + 32))));
            rate2 = false;
            u32 p = load<u32>(s + 72);
            for (u32 i = 0; n != 0;) {
                if (load<u32>(p + 36) != 0) {
                    rate2 = true;
                    break;
                }
                ++i;
                p += 224;
                if (!(n > i)) break;
            }
        }
    }
    if (rate2) {
        const u32 d = load<u32>(o + 396) + 1528;
        callf(ctx, kSetRate2, {o, d}, {k1, LfsAt(d + 16)});
    }
    if (load<u32>(o + 1012) != 0x3F2950FFu) SetEffectHash(ctx, o, 0x3F2950FFu);
    {
        const u32 a = load<u32>(def + 740);
        const u32 w = load<u32>(o + 172);
        store<u32>(o + 172, (w & ~0x00C00000u) | (std::rotl(a, 22) & 0x00C00000u));
        const bool b = (load<u32>(def + 744) & 0x80000000u) != 0;
        u64 x = load<u64>(o + 184);
        x = b ? (x | 0x20u) : (x & ~u64{0x20});
        store<u64>(o + 184, x);
    }
    {
        const u32 h = static_cast<u32>(call(ctx, kResolve, {load<u32>(load<u32>(o + 396) + 824 + 40)}));
        if (const u32 prev = load<u32>(o + 784); h != prev) {
            call(ctx, kDispatch, {prev, 0, 1, 0});   // the previous handle
            store<u32>(o + 784, h);
        }
    }
    {
        const u32 v = load<u32>(o + 216);
        const u32 pt = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(o + 172))));
        const u32 world = load<u32>(kWorldPtr);
        const u32 user = static_cast<u32>(VCall(ctx, v, 8, {v}));
        call(ctx, kSetUser, {world, pt, user, 1});
    }
    if (const u32 qv = load<u32>(q); qv != 0) {
        call(ctx, kVecClear, {qv});
        call(ctx, kNop, {qv});
        call(ctx, kSmallFree, {load<u32>(q), 20});
    }
}
