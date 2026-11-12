// Game code, medium sweep part 30: collectible contact messages, ragdoll
// teardown, physics-world membership of bodies and their trigger volumes.
#include "mid30.h"

using namespace sly4;
using namespace sly4::mid30;

namespace {
using namespace m30fn;

constexpr u32 HkAdd30 = 0x0092F2A8u;     // Hk_AddEntity92F2A8(world, body, 1)
constexpr u32 HkRemove30 = 0x0092FFC8u;  // Hk_RemoveEntity92FFC8(u8* out, world, body)

// The physics world: (*(world + 0x1281D4))->+4.
u32 HkWorld30() { return load<u32>(load<u32>(load<u32>(kWorld29) + 0x001281D4u) + 4); }

// The first 16-byte entry keyed k in the table +124 (s32 count +128) of e.
bool HasProp30(u32 e, u32 k) {
    for (s32 i = 0; i < static_cast<s32>(load<u32>(e + 128)); ++i)
        if (load<u32>(load<u32>(e + 124) + 16 * static_cast<u32>(i)) == k) return true;
    return false;
}
}  // namespace

// u32 f(T* o, Msg* m): a collectible's messages, by kind m->+8:
// 0xDD92E72B copies the toucher m->+0's position +144.. to +1584.. and
// sets state 2 (vt[376](2)); 0xDE25AD64 state 3; 0xD77F98D6 with m->+12
// = 0x14FBC state 7; 0xD88EEAE2 / 0x394E70AC set / clear u8 +1700. The
// contact 0xCCF83851 of a live body b (bit 2 of +192) collects o when b
// (or b's owner +372 -- itself carried by o through +432 with bit 0x2000)
// is o or o's carrier +1712 (b becomes the carrier when b->+432 is o):
// +1648 += 1 and b's kind-40 component q, not yet counted (u8 +60), adds
// n * (count / 10 + 1) to the player's +1652 (n = (s32) q->+24) and gets
// 0x002B21D8(q, n, count / 10 + 1); u8 +60 = 1. r3 / f1 as the original
// leaves them.
DECOMP(0018E0C0, Pickup_OnMessage_0018E0C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 1);
    const u32 kind = load<u32>(m + 8);
    if (kind == 0xDD92E72Bu) {
        const u32 b = load<u32>(m + 0);
        if (b == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + 1584 + k, load<u32>(b + 144 + k));
        ctx.fpr[1] = Lf(b + 144);
        ctx.fpr[2] = Lf(b + 148);
        ctx.fpr[3] = Lf(b + 152);
        VCall(ctx, o, 376, {o, 2});
        return;
    }
    if (kind == 0xDE25AD64u) {
        VCall(ctx, o, 376, {o, 3});
        return;
    }
    if (kind == 0xD77F98D6u) {
        if (load<u32>(m + 12) != 0x00014FBCu) {
            ctx.gpr[3] = 0x00014FBCu;
            return;
        }
        VCall(ctx, o, 376, {o, 7});
        return;
    }
    if (kind == 0xD88EEAE2u || kind == 0x394E70ACu) {
        const u8 v = kind == 0xD88EEAE2u ? 1 : 0;
        store<u8>(o + 1700, v);
        ctx.gpr[3] = v;
        return;
    }
    if (kind != 0xCCF83851u) {
        ctx.gpr[3] = kind;
        return;
    }
    const u32 b = load<u32>(m + 0);
    const u32 st = load<u32>(b + 192);
    if ((st & 2) == 0) {
        ctx.gpr[3] = st;
        return;
    }
    u32 own = load<u32>(b + 372);
    if (own != 0 && own != o && (load<u32>(own + 192) & 0x2000u) != 0 && load<u32>(own + 432) == o) own = o;
    u32 r3 = st;
    if (own == 0 || own != o) {
        r3 = st & 0x2000u;
        if (r3 != 0) {
            r3 = load<u32>(b + 432);
            if (r3 == o) store<u32>(o + 1712, b);
        }
    }
    if (own != o) {
        if (own == 0) {
            ctx.gpr[3] = r3;
            return;
        }
        r3 = load<u32>(o + 1712);
        if (own != r3) {
            ctx.gpr[3] = r3;
            return;
        }
    }
    const u32 n = load<u32>(o + 1648) + 1;
    store<u32>(o + 1648, n);
    const u32 tier = n / 10;
    const u32 q = static_cast<u32>(call(ctx, 0x000D1288u, {b, 40, 0}));  // Obj_FindComponent
    if (q == 0) return;
    const u64 fi = rt::fctid(Lf(q + 24), true);
    const u32 k = static_cast<u32>(fi);
    ctx.fpr[1] = std::bit_cast<f64>(fi);
    if (load<u8>(q + 60) != 0) {
        ctx.gpr[3] = k;
        return;
    }
    const u32 p = load<u32>(load<u32>(kWorld29) + 4216);
    store<u32>(p + 1652, load<u32>(p + 1652) + (k * tier + k));
    call(ctx, 0x002B21D8u, {q, k, tier + 1});
    store<u8>(q + 60, 1);
    ctx.gpr[3] = 1;
}

// u32 f(Obj* o, u8 full): with full, a ragdoll's teardown: a filled event
// (EventRec_Fill30_00465B30(ev, o)) goes to World_Query482D14(world, -1,
// ev); the helper +436 gets vt[40](0) and is cleared; physics mode 6
// (0x000E9BA0(o, 6, 0)); the skeletons +392 / +396 are released
// (AnimSkel_Release7D4EA8); the body +388 loses its properties 0xA734004D
// / 0x91C7CF5B (Hk_RemoveProperty93FED8) and is released; +432 gets
// vt[40](0), a targetable +440 vt[40](1), both cleared. Then
// Obj_SetRagdollBase(o, full) (r3).
DECOMP(000ED35C, Ragdoll_Teardown_000ED35C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 full = arg<u32>(ctx, 1) & 0xFF;
    if (full != 0) {
        Stack frame(ctx, 40);  // the results at 112(r1) / 120(r1), the event at 128(r1)..151(r1)
        const u32 sp = frame.addr(), ev = sp + 16;
        for (u32 k = 0; k < 24; k += 4) store<u32>(ev + k, 0);
        call(ctx, 0x00465B30u, {ev, o, 0});  // EventRec_Fill30_00465B30 (r5 = 0 as left)
        call(ctx, 0x00482D14u, {load<u32>(kWorld29), ~0ull, ev});  // World_Query482D14
        if (const u32 a = load<u32>(o + 436); a != 0) {
            VCall(ctx, a, 40, {a, 0});
            store<u32>(o + 436, 0);
        }
        call(ctx, 0x000E9BA0u, {o, 6, 0});
        if (const u32 s = load<u32>(o + 392); s != 0) call(ctx, 0x007D4EA8u, {s});  // AnimSkel_Release7D4EA8
        if (const u32 s = load<u32>(o + 396); s != 0) call(ctx, 0x007D4EA8u, {s});
        if (u32 e = load<u32>(o + 388); e != 0) {
            if (HasProp30(e, 0xA734004Du)) {
                call(ctx, 0x0093FED8u, {sp, e, 0xA734004Du});  // Hk_RemoveProperty93FED8
                e = load<u32>(o + 388);
            }
            if (HasProp30(e, 0x91C7CF5Bu)) {
                call(ctx, 0x0093FED8u, {sp + 8, e, 0x91C7CF5Bu});
                e = load<u32>(o + 388);
            }
            call(ctx, 0x007D4EA8u, {e});
        }
        if (const u32 b = load<u32>(o + 432); b != 0) {
            VCall(ctx, b, 40, {b, 0});
            store<u32>(o + 432, 0);
        }
        if (const u32 c = load<u32>(o + 440); c != 0 && (load<u32>(c + 172) & 0x00040000u) != 0) VCall(ctx, c, 40, {c, 1});
        store<u32>(o + 440, 0);
    }
    call(ctx, 0x000E02F8u, {o, full});  // Obj_SetRagdollBase
}

// u32 f(Obj* o, u8 on): Flag_Set172_80_000DA0CC(o, on); then moves the body
// +416 into / out of the physics world when its presence (+8) says it is
// out of step. Off: Hk_RemoveEntity92FFC8 and Volume_Notify112D8
// (*0x00F557EC, o + 436). On (with a non-zero nibble (u16 +432 >> 12)):
// Hk_AddEntity92F2A8(world, body, 1); when vt[168]() allows, the trigger
// volume o + 436 is rebuilt (Shape_InitBody(shape, *0x00F557EC, body, &v,
// vt[76](), 0xFFFF / 0xFFFE by bit 0x01000000 of def->+300) with v the
// def +216's vt[12] point; Volume_SetShape637D14; Shape_Release638498);
// then vt[84](o + 96) (r3).
DECOMP(001FAF44, Obj_SetPhysicsOn_001FAF44) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFF;
    const u32 def = load<u32>(o + 216);
    call(ctx, 0x000DA0CCu, {o, on});  // Flag_Set172_80_000DA0CC
    const u32 b = load<u32>(o + 416);
    if (b == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 in = load<u32>(b + 8);
    if (((in != 0 ? 1u : 0u) ^ (on == 0 ? 1u : 0u)) == 0) {
        ctx.gpr[3] = b;
        return;
    }
    Stack frame(ctx, 48);  // the point at 112(r1), the shape at 144(r1), the result at 148(r1)
    const u32 sp = frame.addr();
    if (on != 0) {
        call(ctx, HkRemove30, {sp + 36, in, b});
        call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), o + 436});  // Volume_Notify112D8
        return;
    }
    const u32 w = HkWorld30();
    if (((load<u16>(o + 432) >> 12) & 0xF) == 0) {
        ctx.gpr[3] = b;
        return;
    }
    call(ctx, HkAdd30, {w, b, 1});
    if (VCall(ctx, o, 168, {o}) != 0) {
        {
            const u32 d = load<u32>(o + 216);
            const u32 opd = load<u32>(load<u32>(d) + 12);
            call_ptr(ctx, opd, {sp, d, opd, load<u32>(opd)});
        }
        const u32 mask = (load<u32>(def + 300) & 0x01000000u) != 0 ? 0xFFFFu : 0xFFFEu;
        const u32 body = load<u32>(o + 416);
        const u32 opd = load<u32>(load<u32>(o) + 76);
        // the original leaves r5 = 0x00F50000 (the base of the next load) and r6 = the entry
        const u32 kind = static_cast<u32>(call_ptr(ctx, opd, {o, opd, 0x00F50000u, load<u32>(opd)}));
        const u32 shape = sp + 32;
        call(ctx, 0x00011410u, {shape, load<u32>(0x00F557ECu), body, sp, kind, mask});  // Shape_InitBody
        call(ctx, 0x00637D14u, {o + 436, shape});  // Volume_SetShape637D14
        call(ctx, 0x00638498u, {shape});           // Shape_Release638498
    }
    VCall(ctx, o, 84, {o, o + 96});
}

// u32 f(Obj* o, s32 mode): o's physics mode +400. Mode 1 (from another
// mode) wakes o (vt[88](1, 1, 1, 1)), tags the body +388 (+44 = its low 5
// bits | o->+204 << 16) and, when out of the world, adds it (with the
// skeletons +392 / +396 via 0x0092E9D8 and the volume body +548, +44 =
// o->+204 << 16 | 7), storing mode 2; mode 6 removes +388 and +548 from
// the world (Hk_RemoveEntity92FFC8 into the stack bytes 112(r1) / 113(r1)).
// r3 as the original leaves it.
DECOMP(000E9BA0, Obj_SetPhysicsMode_000E9BA0) {
    const u32 o = arg<u32>(ctx, 0);
    s32 mode = static_cast<s32>(arg<u32>(ctx, 1));
    const u32 old = load<u32>(o + 400);
    u32 r3 = old;
    if (old == static_cast<u32>(mode)) {
        ctx.gpr[3] = r3;
        return;
    }
    Stack frame(ctx, 16);  // the results at 112(r1) / 113(r1)
    const u32 sp = frame.addr();
    if (mode == 6) {
        const u32 b = load<u32>(o + 388);
        r3 = b;
        if (b != 0 && load<u32>(b + 8) != 0) r3 = static_cast<u32>(call(ctx, HkRemove30, {sp, HkWorld30(), b}));
        const u32 c = load<u32>(o + 548);
        r3 = c;
        if (c != 0 && load<u32>(c + 8) != 0) r3 = static_cast<u32>(call(ctx, HkRemove30, {sp + 1, HkWorld30(), c}));
    } else if (mode == 1) {
        VCall(ctx, o, 88, {o, 1, 1, 1, 1});
        const u32 b = load<u32>(o + 388);
        r3 = b;
        if (b != 0) {
            const u32 tag = (load<u32>(b + 44) & 0x1Fu) | (load<u32>(o + 204) << 16);
            const u32 in = load<u32>(b + 8);
            store<u32>(b + 44, tag);
            if (in == 0) {
                const u32 w = HkWorld30();
                call(ctx, HkAdd30, {w, b, 1});
                if (const u32 s = load<u32>(o + 392); s != 0) call(ctx, 0x0092E9D8u, {w, s});
                if (const u32 s = load<u32>(o + 396); s != 0) call(ctx, 0x0092E9D8u, {w, s});
                const u32 c = load<u32>(o + 548);
                r3 = c;
                if (c != 0) {
                    store<u32>(c + 44, (load<u32>(o + 204) << 16) | 7);
                    r3 = static_cast<u32>(call(ctx, HkAdd30, {w, c, 1}));
                }
            }
        }
        mode = 2;
    }
    store<u32>(o + 400, static_cast<u32>(mode));
    ctx.gpr[3] = r3;
}
