// Game code, medium sweep part 29: object callbacks (damage, owner state
// sync, camera target push).
#include "mid29.h"

using namespace sly4;
using namespace sly4::mid29;

namespace {
using namespace m29fn;

// The health share between o and the player p: p->+344 vs o->+348; the
// smaller one when above lim, else lim (stored to dst).
inline void ShareHealth29(u32 dst, f64 a, f64 b, f64 lim, u32 limWord) {
    const f64 m = b < a ? b : a;
    if (m > lim) store<u32>(dst, Bits28(m));
    else store<u32>(dst, limWord);
}
}  // namespace

// bool f(Obj* o, x, Obj* src, y, f32 dmg): takes damage. 1 when dmg <=
// *0x00170414 (lim) or src has bit 0x400 of +192 (after o->+656 |=
// 0x04000000, o->+748 = 0). Unless bit 0x02000000 of +656 was set: with o
// not the player p (slot word *0x00F65CA0 + 4216) o->+348 = min(p->+348,
// o->+344) (lim when not above it); when o->+348 >= lim
// Obj_ApplyDamage(o, src, src, y, dmg) and, landing on lim, vt[396](o, 12,
// 0) and byte +261 = 0 of the world's +0x1281F4 object (bit 0x20 of its
// +28); then p->+348 = min(o->+348, p->+344) likewise. Returns o->+348 >
// lim. f1 as the original leaves it.
DECOMP(00170418, Obj_TakeDamage_00170418) {
    constexpr u32 kLim = 0x00170414u;
    const u32 o = arg<u32>(ctx, 0);
    const u32 src = arg<u32>(ctx, 2);
    const u32 y = arg<u32>(ctx, 3);
    const f64 dmg = ctx.fpr[1];
    const f64 lim = Lf(kLim);
    if (dmg <= lim) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 w = load<u32>(o + 656) | 0x04000000u;
    store<u32>(o + 748, 0);
    store<u32>(o + 656, w);
    if (src != 0 && (load<u32>(src + 192) & 0x400u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    if ((w & 0x02000000u) == 0) {
        if (const u32 p = PlayerSlot29(0); o != p) ShareHealth29(o + 348, Lf(o + 344), Lf(p + 348), lim, load<u32>(kLim));
        if (Lf(o + 348) >= lim) {
            callf(ctx, 0x000E1738u, {o, src, src, y}, {dmg});  // Obj_ApplyDamage
            if (Lf(o + 348) == lim) {
                VCall(ctx, o, 396, {o, 12, 0});
                const u32 g = load<u32>(load<u32>(kWorld29) + 0x001281F4u);
                if (g != 0 && (load<u32>(g + 28) & 0x20u) != 0) store<u8>(g + 261, 0);
            }
        }
        if (const u32 p = PlayerSlot29(0); o != p) ShareHealth29(p + 348, Lf(p + 344), Lf(o + 348), lim, load<u32>(kLim));
    }
    ctx.fpr[1] = Lf(o + 348);
    ctx.gpr[3] = Lf(o + 348) > lim ? 1u : 0u;
}

// bool f(Obj* o, s32* msg): o->+380's vt[12](), then for message 2 only:
// the def (+20) +48 |= 0x02800000; with an owner (Obj_Owner0D93FC) whose
// +40 has bit 31 the owner's vt[8]() +0 loses bits 26..31, o's vt[64]()
// ->vt[8]() result +168 gains bit 29 (owner +136 +68 bit 0x80) and bit 31
// (owner +40 bit 0x2000); the def +0..+8 gets the unit direction from o
// (+144) to o->+428 or +420 (y from *0x0014CB64, scale *0x0014CB68), else
// o->+128.. unless owner bit 0x8000 or +136 +68 bit 0x800; +40 = 0
// without owner bit 0x4000; o->+432 = 2, o->+184 |= 0x80 (64-bit); 1.
// Without such an owner vt[28](o, 1, 0) and 0. Other messages: 1. f1 as
// the original leaves it.
DECOMP(0014CB70, Obj_OwnerMsg_0014CB70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 msg = arg<u32>(ctx, 1);
    const u32 s = load<u32>(o + 380);
    VCall(ctx, s, 12, {s});
    const u32 d = load<u32>(o + 20);
    const u32 ow = static_cast<u32>(call(ctx, 0x000D93FCu, {o}));  // Obj_Owner0D93FC
    const u32 b = static_cast<u32>(VCall(ctx, o, 64, {o}));
    const u32 r = static_cast<u32>(VCall(ctx, b, 8, {b}));
    if (static_cast<s32>(load<u32>(msg)) != 2) {
        ctx.gpr[3] = 1;
        return;
    }
    store<u32>(d + 48, load<u32>(d + 48) | 0x02800000u);
    if (ow == 0 || (load<u32>(ow + 40) & 0x80000000u) == 0) {
        VCall(ctx, o, 28, {o, 1, 0});
        ctx.gpr[3] = 0;
        return;
    }
    const u32 q = load<u32>(ow + 136);
    const u32 xo = static_cast<u32>(VCall(ctx, ow, 8, {ow}));
    store<u32>(xo, load<u32>(xo) & 0x03FFFFFFu);
    const u32 f1b = (load<u32>(q + 68) & 0x80u) != 0 ? 0x20000000u : 0u;
    const u32 v = load<u32>(r + 168) | f1b;
    store<u32>(r + 168, v);
    store<u32>(r + 168, v | ((load<u32>(ow + 40) & 0x2000u) != 0 ? 0x80000000u : 0u));
    u32 tgt = load<u32>(o + 428);
    if (tgt == 0) tgt = load<u32>(o + 420);
    if (tgt != 0) {
        const f64 y0 = Lf(0x0014CB64u);
        const f64 dx = Sgl28(Lf(tgt + 144) - Lf(o + 144));
        const f64 dz = Sgl28(Lf(tgt + 152) - Lf(o + 152));
        f64 n = Sgl28(y0 * y0);
        n = Sgl28(std::fma(dx, dx, n));
        n = Sgl28(std::fma(dz, dz, n));
        const f64 k = Sgl28(Lf(0x0014CB68u) / Sgl28(std::sqrt(n)));
        const f64 ny = Sgl28(y0 * k);
        store<u32>(d + 0, Bits28(Sgl28(dx * k)));
        store<u32>(d + 4, Bits28(ny));
        store<u32>(d + 8, Bits28(Sgl28(dz * k)));
        ctx.fpr[1] = ny;
    } else if ((load<u32>(ow + 40) & 0x8000u) == 0 && (q == 0 || (load<u32>(q + 68) & 0x800u) == 0)) {
        for (u32 i = 0; i < 3; ++i) store<u32>(d + 4 * i, load<u32>(o + 128 + 4 * i));
        ctx.fpr[1] = Lf(o + 136);
    }
    if ((load<u32>(ow + 40) & 0x4000u) == 0) store<u32>(d + 40, 0);
    store<u32>(o + 432, 2);
    store<u64>(o + 184, load<u64>(o + 184) | 0x80u);
    ctx.gpr[3] = 1;
}

// void f(Obj* o): pushes a camera target for o->+416. With u8 o->+515:
// cleared, the target detached from the player (Obj_Detach0D72C0, the
// player only with bit 2 of +192, else 0), vt[212](t, 0) and +172 bit
// 0x8000 cleared. An old target o->+424 is popped (Obj_PopLayer(t, old,
// f1 = *0x0005FE34)), released (vt[4]) and cleared. The new one (112
// bytes, vtable 0x00B13B38, +4 = the target) takes the camera's (world
// +0x128168) +816 matrix with rows 0 and 2 negated, +80 = its +56 *
// *0x0005FE38, +88 bits 0xFF800000 = 0x00800000, +84 / +92..+100 = 0; o->+424 =
// it (0 when the allocation fails) and Camera_PushTarget0FC040(t, it, t, 0,
// f1 = *0x0005FE3C). r3 / f1 as the original leaves them.
DECOMP(0005FE40, Obj_PushCamTarget_0005FE40) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 515) != 0) {
        const u32 s = load<u32>(o + 416);
        store<u8>(o + 515, 0);
        const u32 pl = PlayerSlot29(0);
        const u32 t = pl != 0 && (load<u32>(pl + 192) & 2u) != 0 ? pl : 0;
        call(ctx, 0x000D72C0u, {t, s});  // Obj_Detach0D72C0
        const u32 s2 = load<u32>(o + 416);
        VCall(ctx, s2, 212, {s2, 0});
        const u32 s3 = load<u32>(o + 416);
        store<u32>(s3 + 172, load<u32>(s3 + 172) & ~0x8000u);
    }
    if (load<u32>(o + 424) != 0) {
        callf(ctx, 0x000FBB88u, {load<u32>(o + 416), load<u32>(o + 424)}, {Lf(0x0005FE34u)});  // Obj_PopLayer
        if (const u32 old = load<u32>(o + 424); old != 0) VCall(ctx, old, 4, {old});
        store<u32>(o + 424, 0);
    }
    const u32 cam = load<u32>(load<u32>(kWorld29) + 0x00128168u);
    const u32 m = cam + 816;
    const u32 scale = Bits28(Sgl28(Lf(cam + 56) * Lf(0x0005FE38u)));
    u32 mw[16];
    for (u32 i = 0; i < 16; ++i) mw[i] = (i % 4 == 3) ? 0 : load<u32>(m + 4 * i);
    const u32 p = static_cast<u32>(call(ctx, SmallAlloc28, {112}));
    if (p != 0) {
        constexpr u32 kNeg = 0x80000000u;  // fneg
        store<u8>(p + 8, 0);
        store<u32>(p + 4, load<u32>(o + 416));
        store<u32>(p + 0, 0x00B13B38u);
        store<u32>(p + 32, mw[4]);
        store<u32>(p + 36, mw[5]);
        const u32 w88 = load<u32>(p + 88);
        store<u32>(p + 16, mw[0] ^ kNeg);
        store<u32>(p + 20, mw[1] ^ kNeg);
        store<u32>(p + 24, mw[2] ^ kNeg);
        store<u32>(p + 40, mw[6]);
        store<u32>(p + 48, mw[8] ^ kNeg);
        store<u32>(p + 52, mw[9] ^ kNeg);
        store<u32>(p + 56, mw[10] ^ kNeg);
        store<u32>(p + 64, mw[12]);
        store<u32>(p + 68, mw[13]);
        store<u32>(p + 72, mw[14]);
        store<u32>(p + 80, scale);
        store<u32>(p + 92, 0);
        store<u32>(p + 88, (w88 & 0x007FFFFFu) | 0x00800000u);
        store<u32>(p + 96, 0);
        store<u32>(p + 100, 0);
        store<u32>(p + 84, 0);
    }
    store<u32>(o + 424, p);
    const u32 t = load<u32>(o + 416);
    callf(ctx, 0x000FC040u, {t, p, t, 0}, {Lf(0x0005FE3Cu)});  // Camera_PushTarget0FC040
}
