// Game code, large-function sweep part 11: player re-initialisation.
#include "big11.h"

namespace sly4::obj {

namespace {
inline be_u32& W(u32 base, u32 off) { return gm<be_u32>(base + off); }
inline f64 F(u32 ea) { return LfsW(gm<be_u32>(ea)); }
inline void St(u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); }
inline void CopyW(u32 dst, u32 src) { gm<be_u32>(dst) = gm<be_u32>(src).get(); }
}  // namespace

namespace {
constexpr u32 kEffectHash = 0x000E0B64u;   // Obj_EffectHash356_000E0B64 (o, hash, 1)
constexpr u32 kDropPlayer = 0x0046B6CCu;   // World_DropPlayer46B6CC (world, o)
constexpr u32 kAnimStop = 0x00254814u;     // Anim_Stop254814 (anim, 0)
constexpr u32 kAnimReset = 0x002980ACu;    // Anim_Reset2980AC (anim, 0)
constexpr u32 kEmitterStop = 0x00443424u;  // Emitter_Stop (emitter, 1)
constexpr u32 kRefRelease = 0x004432DCu;   // RefNode_Release (node)
constexpr u32 kRemoveEntity = 0x0092FFC8u; // Hk_RemoveEntity92FFC8 (out, entity, skel) (Havok wrapper)
constexpr u32 kSkelRelease = 0x007D4EA8u;  // AnimSkel_Release7D4EA8 (skel)
constexpr u32 kFormat = 0x005534D0u;       // String_Format
constexpr u32 kQueryInit = 0x000CDD28u;    // Query_Init0CDD28 (22 arguments; f1, f2)
constexpr u32 kApplyQuery = 0x000D2F04u;   // Obj_ApplyQuery0D2F04 (o, Query*)
constexpr u32 kAttach = 0x000D135Cu;       // Obj_Attach0D135C (o, child, 1)
constexpr u32 kFindByKey = 0x000D1454u;    // Obj_FindByKey_000D1454 (o, key)
constexpr u32 kCallK2 = 0x000D94F4u;       // CallK2_000D94F4 (o)
constexpr u32 kNodePush = 0x00446984u;     // Node_PushF65B7C (name hash, set)
constexpr u32 kSetShadow = 0x000E0000u;    // Obj_SetShadow0E0000 (o, 0)
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
        if (gm<be_u32>(e + 12) == k) return true;
    return false;
}

// +1012 = hash (and the effect switched) unless it already is.
void SetEffectHash(u32 o, u32 hash) {
    gm<be_u32>(o + 1012) = hash;
    call(kEffectHash, o, hash, 1, hash);
}

// Mark a released node (+136 |= 0x200).
void MarkDead(u32 n) { gm<be_u32>(n + 136) = gm<be_u32>(n + 136) | 0x200u; }
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
// handle (+784) dispatched when it changes and the world user set. The
// player setup / rate callees are native (big12 / big13) but are called
// through the bridge here: the simulator check of this function cannot
// drive their inlined code with its generated inputs.
void Player_ResetFromDef238270(u32 o, u32 def) {
    GuestFrame frame(272);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    switch (gm<be_u32>(o + 1012)) {
    case 0xCE2E6B3Du: SetEffectHash(o, 0x9B04BE36u); break;
    case 0x594B024Bu: SetEffectHash(o, 0x273F0E72u); break;
    case 0xE15D38C8u: SetEffectHash(o, 0x4090D96Fu); break;
    case 0xAB6ADA7Au: SetEffectHash(o, 0x32AB4EB3u); break;
    default: break;
    }
    call(kDropPlayer, gm<be_u32>(kWorldPtr), o);
    if (vcall<u32>(host<u8>(o), 60) != 0) {
        call(kAnimStop, vcall<u32>(host<u8>(o), 60), 0u);
        call(kAnimReset, vcall<u32>(host<u8>(o), 60), 0u);
    }
    if (const u32 h = gm<be_u32>(o + 164); h != 0) {
        vcall(host<u8>(h), 4);
        gm<be_u32>(o + 164) = 0;
    }
    call(0x002349E0u, o);   // Player_DropLinks2349E0 (native; called through the bridge, see below)
    if (const u32 q = gm<be_u32>(o + 160); q != 0) {
        vcall(host<u8>(q), 40, 0, 1);
        const u32 q2 = gm<be_u32>(o + 160);
        gm<be_u32>(q2 + 280) = 0;
        gm<be_u32>(q2 + 276) = 0x02235C33u;
        vcall(host<u8>(q2), 8);
        gm<be_u32>(o + 160) = 0;
    }
    gm<be_u32>(o + 744) = 0;
    gm<be_u32>(o + 948) = 0;
    for (u32 k = 0; k < 3; ++k) call(kEmitterStop, gm<be_u32>(o + 864 + 4 * k), 1);
    for (u32 off : {896u, 916u, 920u, 924u, 928u}) call(kRefRelease, gm<be_u32>(o + off));
    for (u32 k = 0; k < 4; ++k) call(kRefRelease, gm<be_u32>(o + 900 + 4 * k));
    if (const u32 n = gm<be_u32>(o + 748); n != 0) {
        MarkDead(n);
        gm<be_u32>(o + 748) = 0;
        gm<be_u32>(o + 752) = 0;
        gm<be_u32>(o + 756) = 0;
    }
    for (u32 off : {768u, 776u})
        if (const u32 n = gm<be_u32>(o + off); n != 0) {
            MarkDead(n);
            gm<be_u32>(o + off) = 0;
        }
    if (u32 sk = gm<be_u32>(o + 672); sk != 0) {
        if (const u32 ent = gm<be_u32>(sk + 8); ent != 0) {
            call(kRemoveEntity, sp + 368, ent, sk, ent);
            sk = gm<be_u32>(o + 672);
        }
        call(kSkelRelease, sk);
        gm<be_u32>(o + 672) = 0;
    }
    if (const u32 n = gm<be_u32>(o + 356); n != 0) {
        MarkDead(n);
        gm<be_u32>(o + 356) = 0;
    }
    // Detach the children (vector +248): the last one moves into the freed
    // slot, which is examined again.
    for (u32 i = 0, off = 0;; i += 1, off += 4) {
        const u32 v = gm<be_u32>(o + 248);
        const u32 n = v != 0 ? gm<be_u16>(v + 8) : 0;
        if (static_cast<s32>(i) >= static_cast<s32>(n)) break;
        const u32 e = gm<be_u32>(gm<be_u32>(v) + off);
        if (e == 0 || !ChildKind(e)) continue;
        vcall(host<u8>(e), 4);
        const u32 v2 = gm<be_u32>(o + 248);
        const u32 c = gm<be_u16>(v2 + 8) - 1;
        gm<be_u16>(v2 + 8) = static_cast<u16>(c);
        if (i != (c & 0xFFFFu)) {
            const u32 d = gm<be_u32>(v2);
            gm<be_u32>(d + off) = gm<be_u32>(d + ((c << 2) & 0x3FFFCu));
        }
        i -= 1;
        off -= 4;
    }

    gm<be_u32>(o + 396) = def;
    const u32 port = static_cast<u32>(static_cast<s32>(static_cast<s8>(gm<u8>(o + 172))));
    gm<be_u32>(o + 216) = def;
    const u32 slot = gm<be_u32>(kWorldPtr) + port * 0x20C80u + 4208;
    const u32 name = call<u32>(kFormat, kFmtControllable, gm<u8>(slot + 1) + 1u);
    const f64 k0 = F(kK);
    const u32 q = sp + 224;   // Query
    // the floats sit in arguments 20 / 21 (r1 +200 / +208), which the callee does not read
    call(kQueryInit, q, 4u, 128u, name, def, 0u, kEmpty, kIdentity, kOnes, 1u, 0u, 0u, 1u, 0u, 0u, 0u, 0u, 0u, 0xFFFFFFFFu, k0, k0, 0u);
    call(kApplyQuery, o, q);
    for (u32 j = 0, off = 0;; j += 1, off += 4) {
        const u32 v = gm<be_u32>(gm<be_u32>(o + 216) + 16);
        const u32 n = v != 0 ? gm<be_u16>(v + 8) : 0;
        if (j >= n) break;
        const u32 e = gm<be_u32>(gm<be_u32>(v) + off);
        if (e == 0 || !ChildKind(e)) continue;
        gm<be_u32>(e + 8) = o;
        call(kAttach, o, e, 1);
        gm<be_u32>(e + 8) = 0;
    }
    call(0x00231470u, o);   // Player_Setup231470
    const u32 key = call<u32>(kFindByKey, o, gm<be_u32>(gm<be_u32>(o + 396) + 824 + 24));
    gm<be_u32>(o + 744) = key;
    const u32 k2 = call<u32>(kCallK2, o);
    const u32 w616 = gm<be_u32>(o + 616);
    gm<be_u32>(o + 948) = k2;
    if (k2 != 0) {
        if (const u32 a = gm<be_u32>(o + 744); a != 0) gm<be_u32>(k2 + 1216) = a;
        gm<be_u32>(k2 + 1240) = w616;
    }
    {
        const u32 base = gm<be_u32>(o + 396) + 748;   // {set, name hashes...}
        static constexpr u32 kSlots[5] = {896, 916, 920, 924, 928};
        for (u32 k = 0; k < 5; ++k)
            gm<be_u32>(o + kSlots[k]) = call<u32>(kNodePush, gm<be_u32>(base + 36 + 4 * k), gm<be_u32>(base));
        for (u32 k = 0; k < 4; ++k)
            gm<be_u32>(o + 900 + 4 * k) = call<u32>(kNodePush, gm<be_u32>(base + 4 + 8 * k), gm<be_u32>(base));
    }
    call(0x00231928u, o);   // Player_Setup231928
    call(0x00230CA0u, o);   // Player_Setup230CA0
    call(0x00232F00u, o, gm<be_u32>(o + 612));   // Player_Setup232F00
    call(kSetShadow, o, 0);
    CopyW(slot + 104, gm<be_u32>(o + 396) + 1272 + 68);
    const f64 k1 = F(kK + 4);
    call(0x002371F8u, o, k1);   // Player_SetRate2371F8
    bool rate2 = true;
    if (const u32 s = gm<be_u32>(o + 612); s != 0) {
        if (const u32 t = gm<be_u32>(s + 28); t != 0) {
            const u32 n = static_cast<u32>(static_cast<s32>(static_cast<s8>(gm<u8>(t + 32))));
            rate2 = false;
            u32 p = gm<be_u32>(s + 72);
            for (u32 i = 0; n != 0;) {
                if (gm<be_u32>(p + 36) != 0) {
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
        const u32 d = gm<be_u32>(o + 396) + 1528;
        call(0x00236868u, o, d, k1, F(d + 16));   // Player_SetRate2_236868
    }
    if (gm<be_u32>(o + 1012) != 0x3F2950FFu) SetEffectHash(o, 0x3F2950FFu);
    {
        const u32 a = gm<be_u32>(def + 740);
        const u32 w = gm<be_u32>(o + 172);
        gm<be_u32>(o + 172) = (w & ~0x00C00000u) | (std::rotl(a, 22) & 0x00C00000u);
        const bool b = (gm<be_u32>(def + 744) & 0x80000000u) != 0;
        u64 x = gm<be_u64>(o + 184);
        x = b ? (x | 0x20u) : (x & ~u64{0x20});
        gm<be_u64>(o + 184) = x;
    }
    {
        const u32 h = call<u32>(kResolve, gm<be_u32>(gm<be_u32>(o + 396) + 824 + 40));
        if (const u32 prev = gm<be_u32>(o + 784); h != prev) {
            call(kDispatch, prev, 0, 1, 0);   // the previous handle
            gm<be_u32>(o + 784) = h;
        }
    }
    {
        const u32 v = gm<be_u32>(o + 216);
        const u32 pt = static_cast<u32>(static_cast<s32>(static_cast<s8>(gm<u8>(o + 172))));
        const u32 world = gm<be_u32>(kWorldPtr);
        const u32 user = vcall<u32>(host<u8>(v), 8);
        call(kSetUser, world, pt, user, 1);
    }
    if (const u32 qv = gm<be_u32>(q); qv != 0) {
        call(kVecClear, qv);
        call(kNop, qv);
        call(kSmallFree, gm<be_u32>(q), 20);
    }
}

}  // namespace sly4::obj

NATIVE(00238270, sly4::obj::Player_ResetFromDef238270)
