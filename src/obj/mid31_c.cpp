// Game code, medium sweep part 31: player / physics resets and state
// helpers.
#include "mid31.h"

using namespace sly4;
using namespace sly4::mid31;

namespace {
using namespace m31fn;
constexpr u32 kZeroVec31 = 0x01164B40u;  // the zero vector (BSS)
}  // namespace

// void f(T* o): the player's action reset. The grab record ((+612) +156)
// is cleared (+8, +12, u8 +16 / +17), the partner +948 loses flag
// 0x40000000 of +1268, +932 / +936 = 0, vslots 136 and 384 get the zero
// vector; in the level 0x04E8CCF7 ((+20)->0) a body (vslot 144) that is in
// a world (+8) is made dynamic (Hk_SetMotion_0091D470(body, 5, 1, 0)).
// The three emitters +864 / +868 / +872 are stopped (Emitter_Stop(handle,
// 1), each handle also spilled to 48(r1) as the original does) and cleared
// with +876..+888 / u8 +892; then tail Obj_RestartAnim_00229B30(o).
DECOMP(0023CC2C, Player_ResetAction_0023CC2C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 612);
    const u32 e = load<u32>(o + 948);
    if (a != 0) {
        const u32 g = load<u32>(a + 156);
        if (g != 0) {
            store<u32>(g + 8, 0);
            store<u32>(g + 12, 0);
            store<u8>(g + 16, 0);
            store<u8>(g + 17, 0);
        }
    }
    if (e != 0) store<u32>(e + 1268, load<u32>(e + 1268) & ~0x40000000u);
    store<u32>(o + 932, 0);
    store<u32>(o + 936, 0);
    VCall(ctx, o, 136, {o, kZeroVec31});
    VCall(ctx, o, 384, {o, kZeroVec31});
    if (load<u32>(load<u32>(o + 20)) == 0x04E8CCF7u && static_cast<u32>(VCall(ctx, o, 144, {o})) != 0) {
        const u32 b = static_cast<u32>(VCall(ctx, o, 144, {o}));
        if (load<u32>(b + 8) != 0) {
            const u32 b2 = static_cast<u32>(VCall(ctx, o, 144, {o}));
            call(ctx, 0x0091D470u, {b2, 5, 1, 0});  // Hk_SetMotion_0091D470
        }
    }
    {
        Stack frame(ctx, 64);
        for (u32 off : {864u, 868u, 872u}) {
            const u32 h = load<u32>(o + off);
            store<u32>(frame.addr() + 48, h);
            call(ctx, 0x00443424u, {h, 1});  // Emitter_Stop
        }
    }
    store<u32>(o + 864, 0);
    store<u32>(o + 868, 0);
    store<u32>(o + 872, 0);
    store<u32>(o + 876, 0);
    store<u32>(o + 880, 0);
    store<u32>(o + 884, 0);
    store<u32>(o + 888, 0);
    store<u8>(o + 892, 0);
    ret(ctx, call(ctx, 0x00229B30u, {o}));  // Obj_RestartAnim_00229B30
}

// void f(Target* o): the collision group of the target object +16 (body
// via vslot 64 -> Obj_GetBody_00252940 when +192 bit 0x8; the rider
// ((+612) +24) with +200 bit 0x800; vslot 144 with +200 bit 0x2000) +44,
// low half replaced by 16, becomes o's +48 when it changed and is pushed
// to o's entities: +20 / +24 (when in a world: +44 = group,
// Hk_UpdateEntityGroup(world, e, 0, 1)), +272 / +1028 (+44 = group,
// Hk_SetQualityLayer9315F0(world, e, 1)); world = the named-object table's
// +4. Nothing without the world or the target. r3 as the original leaves it.
DECOMP(000FE828, Target_Update0FE828) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(load<u32>(load<u32>(0x00F65CA0u) + 0x001281D4u) + 4);
    const u32 t = load<u32>(o + 16);
    if (w == 0 || t == 0) {
        ret(ctx, o);
        return;
    }
    u32 grp;
    if ((load<u32>(t + 192) & 0x8u) != 0) {
        if (static_cast<u32>(VCall(ctx, t, 64, {t})) == 0) {
            ret(ctx, 0u);
            return;
        }
        u32 c = static_cast<u32>(VCall(ctx, t, 64, {t}));
        if (static_cast<u32>(call(ctx, 0x00252940u, {c})) == 0) {  // Obj_GetBody_00252940
            ret(ctx, 0u);
            return;
        }
        c = static_cast<u32>(VCall(ctx, t, 64, {t}));
        grp = load<u32>(static_cast<u32>(call(ctx, 0x00252940u, {c})) + 44);
    } else if ((load<u32>(t + 200) & 0x800u) != 0) {
        grp = load<u32>(load<u32>(load<u32>(t + 612) + 24) + 44);
    } else if ((load<u32>(t + 200) & 0x2000u) != 0) {
        grp = load<u32>(static_cast<u32>(VCall(ctx, t, 144, {t})) + 44);
    } else {
        ret(ctx, 0u);
        return;
    }
    u32 v = (grp & 0xFFFF0000u) | 16u;
    if (v == load<u32>(o + 48)) {
        ret(ctx, v);
        return;
    }
    store<u32>(o + 48, v);
    for (u32 off : {20u, 24u}) {
        const u32 e = load<u32>(o + off);
        if (e != 0 && load<u32>(e + 8) != 0) {
            store<u32>(e + 44, v);
            call(ctx, 0x00930B70u, {w, e, 0, 1, v, e + 36});  // Hk_UpdateEntityGroup (r7 / r8 left as set)
            v = load<u32>(o + 48);
        }
    }
    const u32 p = load<u32>(o + 272);
    if (p != 0 && p + 16 != 0) {
        store<u32>(p + 44, v);
        call(ctx, 0x009315F0u, {w, p, 1, p + 16, v});  // Hk_SetQualityLayer9315F0 (r6 / r7 left as set)
        v = load<u32>(o + 48);
    }
    const u32 q = load<u32>(o + 1028);
    if (q == 0 || q + 16 == 0) {
        ret(ctx, v);
        return;
    }
    store<u32>(q + 44, v);
    call(ctx, 0x009315F0u, {w, q, 1, q + 16, v});
}

// void f(Hud* o, const f32 pos[3]): a HUD target marker update. Needs the
// target +44 with +172 bit 0x40000, the marker +36 and its model (+24).
// hidden = marker +160 >> 1 == 0; off = Obj_OffScreen_000DADA8(target;
// 0.05). On screen and hidden: Handle_Call363A3C(marker, 0, 1, .., 0, 0,
// 0x01164CA4; -1) shows it; off screen and shown: Obj_Dispatch362
// 0x0036325C(marker, 0, 1, 0) hides it. On screen, or still shown: the
// model takes pos (+64..+72, WorldObj_MarkDirty) and, with a colour slot
// +64, its vslot 40(model, 0, 1), the RGBA bytes +53..+56 / 255 into
// (+68) +64..+76 and World_AddTimedCallback(game, 6, 0x00F357E0, o + 64,
// 0). r3 / f1 as the original leaves them.
DECOMP(002BB168, HudMarker_Update_002BB168) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 pos = arg<u32>(ctx, 1);
    constexpr u32 K = 0x002BB158u;  // f32 {0.05, 1/255, -1}
    const u32 a = load<u32>(o + 44);
    if (a == 0) {
        ret(ctx, o);
        return;
    }
    if ((load<u32>(a + 172) & 0x40000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 h = load<u32>(o + 36);
    if (h == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 m = load<u32>(h + 24);
    if (m == 0) {
        ret(ctx, h);
        return;
    }
    const bool hidden = (load<u32>(h + 160) >> 1) == 0;
    callf(ctx, 0x000DADA8u, {a}, {Lf(K)});  // Obj_OffScreen_000DADA8
    const u32 off = static_cast<u32>(ctx.gpr[3]);
    if (off == 0 && hidden) {
        callf(ctx, 0x00363A3Cu, {load<u32>(o + 36), 0, 1, K, 0, 0, 0x01164CA4u}, {Lf(K + 8)});  // Handle_Call363A3C
    } else if (off != 0 && !hidden) {
        call(ctx, 0x0036325Cu, {load<u32>(o + 36), 0, 1, 0});  // Obj_Dispatch362
    }
    if (off != 0) {
        const u32 sh = load<u32>(load<u32>(o + 36) + 160) >> 1;
        if (sh == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    for (u32 k = 0; k < 3; ++k) store<u32>(m + 64 + 4 * k, load<u32>(pos + 4 * k));
    ctx.fpr[1] = Lf(pos + 8);
    call(ctx, 0x0057D000u, {m});  // WorldObj_MarkDirty
    if (load<u32>(o + 64) == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, m, 40, {m, 0, 1});
    const f64 k1 = Lf(K + 4);
    const u32 p = load<u32>(o + 68);
    f64 v = 0;
    for (u32 k = 0; k < 4; ++k) {
        v = Fmuls31(Sgl28(static_cast<f64>(load<u8>(o + 53 + k))), k1);
        store<u32>(p + 64 + 4 * k, W32(static_cast<f32>(v)));
    }
    Stack frame(ctx, 128);
    store<u64>(frame.addr() + 112, load<u8>(o + 56));  // the last fcfid temporary
    ctx.fpr[1] = v;
    call(ctx, 0x0046B5F8u, {load<u32>(0x00F65CA0u), 6, 0x00F357E0u, o + 64, 0});  // World_AddTimedCallback
}

// void f(Area* o, Obj* x): x leaves the area o. With the debug byte
// *0x01056A08 both get vslot 76. x's key +168 is looked up in o's maps
// +364 (the node found, if any, is detached (Obj_Detach_0012E8B0) when its
// +192 has 0x8, and gets vslot 40(n, 0)) and +388 (vslot 40(n, 0)) and
// removed from each (SortedMap_Find759654 / SortedMap_Remove7597DC, a map
// skipped when its u16 count +2 is 0). When x is in the member vector +344
// (u16 count +352) it gets vslot 40(x, 0) and is swap-removed; then o's
// vslot 96(o, any left), and vslot 28(o, 0, (u8) (vslot 32(o, 0) - 1)).
// r3: 4 * count when x is not a member, else the last vslot result.
DECOMP(0020EF60, Area_Remove_0020EF60) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (load<u8>(0x01056A08u) != 0) {
        VCall(ctx, o, 76, {o});
        VCall(ctx, x, 76, {x});
    }
    Stack frame(ctx, 16);
    const u32 k1 = frame.addr() + 4, k0 = frame.addr();  // 116(r1) / 112(r1)
    u32 key = load<u32>(x + 168);
    store<u32>(k1, key);
    if (load<u16>(o + 366) != 0) {
        const u32 n = load<u32>(static_cast<u32>(call(ctx, 0x00759654u, {o + 364, k1})));  // SortedMap_Find759654
        if (n != 0) {
            if ((load<u32>(n + 192) & 0x8u) != 0) call(ctx, 0x0012E8B0u, {n, o});  // Obj_Detach_0012E8B0
            VCall(ctx, n, 40, {n, 0});
        }
        call(ctx, 0x007597DCu, {o + 364, k1});  // SortedMap_Remove7597DC
        key = load<u32>(x + 168);
    }
    store<u32>(k0, key);
    if (load<u16>(o + 390) != 0) {
        const u32 n = load<u32>(static_cast<u32>(call(ctx, 0x00759654u, {o + 388, k0})));
        if (n != 0) VCall(ctx, n, 40, {n, 0});
        call(ctx, 0x007597DCu, {o + 388, k0});
    }
    s32 i = 0;
    for (;; ++i) {
        if (!(i < static_cast<s32>(load<u16>(o + 352)))) {
            ret(ctx, static_cast<u32>(4 * i));
            return;
        }
        if (load<u32>(load<u32>(o + 344) + 4 * static_cast<u32>(i)) == x) break;
    }
    VCall(ctx, x, 40, {x, 0});
    u32 n = load<u16>(o + 352) - 1u;
    store<u16>(o + 352, static_cast<u16>(n));
    if (static_cast<u32>(i) != (n & 0xFFFF)) {
        const u32 v = load<u32>(o + 344);
        store<u32>(v + 4 * static_cast<u32>(i), load<u32>(v + ((n << 2) & 0x3FFFCu)));
        n = load<u16>(o + 352);
    }
    VCall(ctx, o, 96, {o, (n & 0xFFFF) != 0 ? 1u : 0u});
    const u32 s = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
    VCall(ctx, o, 28, {o, 0, (s - 1) & 0xFF});
}

namespace {
// The root of a collidable chain (parent +12).
u32 HkRoot31(u32 n) {
    while (load<u32>(n + 12) != 0) n = load<u32>(n + 12);
    return n;
}
// The owner of a root collidable: n + (s8) +16 when its (s8) type +24 is 1,
// else 0.
u32 HkOwner31(u32 n) {
    return static_cast<s8>(load<u8>(n + 24)) == 1 ? n + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(n + 16)))) : 0u;
}
}  // namespace

// void f(T* o, Contact* ev): a contact callback. With o's shape (+8,
// vslot 64, +32) in a body (Hk_GetBody9D19C8): the other side of the
// contact (B's root when A's root belongs to that body, else A's root)
// has its owner's property 0x91C7CF5B (16-byte rows +124, count +128,
// u64 value +8; read even for a missing owner, at address 0) looked up;
// an object with +192 bit 0x2 that is in o's list +352 (count +356) sets
// ev +32 = 1. r3 as the original leaves it.
DECOMP(002CFF18, Contact_MarkListed_002CFF18) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ev = arg<u32>(ctx, 1);
    u32 c = load<u32>(o + 8);
    if (c == 0) {
        ret(ctx, 0u);
        return;
    }
    c = static_cast<u32>(VCall(ctx, c, 64, {c}));
    if (c == 0 || load<u32>(c + 32) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 body = static_cast<u32>(call(ctx, 0x009D19C8u, {load<u32>(c + 32)}));  // Hk_GetBody9D19C8
    if (body == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 a = load<u32>(ev + 0);
    const u32 other = HkOwner31(HkRoot31(a)) == body ? HkRoot31(load<u32>(ev + 4)) : HkRoot31(a);
    const u32 p = HkOwner31(other);
    u32 v = 0;
    for (s32 i = 0; i < static_cast<s32>(load<u32>(p + 128)); ++i) {
        const u32 row = load<u32>(p + 124) + 16 * static_cast<u32>(i);
        if (load<u32>(row) == 0x91C7CF5Bu) {
            v = load<u32>(row + 12);  // the low word of the u64 value
            break;
        }
    }
    if (v == 0 || (load<u32>(v + 192) & 0x2u) == 0) {
        ret(ctx, p);
        return;
    }
    s32 i = 0;
    for (; i < static_cast<s32>(load<u32>(o + 356)); ++i) {
        if (load<u32>(load<u32>(o + 352) + 4 * static_cast<u32>(i)) == v) {
            store<u32>(ev + 32, 1);
            ret(ctx, 1u);
            return;
        }
    }
    ret(ctx, static_cast<u32>(4 * i));
}
