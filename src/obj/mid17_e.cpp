// Game code, medium sweep part 17: mesh buffers.
#include "mid17.h"

using namespace sly4;
using namespace sly4::mid17;

// u32 Mesh_InitGrid25D114(-, Mesh* m, u32 w, u32 h)
// (called by 0025D370) Sets up a fan-strip grid mesh: m->0 = 5, m->58 =
// (u16)(w * h + 1) vertices, m->60 = 0, two vertex buffers (m+4, m+20) of
// 24 * that many bytes and an index buffer m+36 of (u16) 3 (2wh - h)
// indices (m->52 = 1, m->56 = the count) with Buffer_Alloc52F198 (align
// 128, where 1 / 1 / 0, flags 2); index buffer +8 bits 0xE = 0x4. Then per
// row (h rows, v counting on as a u16 from 1, n = m->58): {0, f(v), v},
// then for each further column {v - 1, prev, v, prev, f(v), v}, with f(v)
// = v + w - ((v + w - 1) / (n - 1)) * (n - 1) (the ring wrap; divwu).
// Returns the index pointer after the last write.
DECOMP(0025D114, Mesh_InitGrid25D114) {
    const u32 m = arg<u32>(ctx, 1), w = arg<u32>(ctx, 2), h = arg<u32>(ctx, 3);
    const u32 cnt = w * h + 1;
    store<u32>(m, 5);
    store<u16>(m + 58, static_cast<u16>(cnt));
    store<u16>(m + 60, 0);
    const u32 vb = 24 * (cnt & 0xFFFF);
    call(ctx, 0x0052F198u, {m + 4, vb, 128, 1, 2});  // Buffer_Alloc52F198
    call(ctx, 0x0052F198u, {m + 20, vb, 128, 1, 2});
    const u32 ni = 3 * ((w << 1) * h - h);
    store<u32>(m + 52, 1);
    store<u16>(m + 56, static_cast<u16>(ni));
    const u32 b = m + 36;
    call(ctx, 0x0052F198u, {b, (ni << 1) & 0x1FFFEu, 128, 0, 2});
    store<u32>(b + 8, (load<u32>(b + 8) & ~0xEu) | 0x4u);
    u32 p = load<u32>(b);
    if (h == 0) return ret(ctx, p);
    auto wrap = [&](u32 v) {
        const u32 n = load<u16>(m + 58);
        const u32 q = static_cast<u32>(rt::divwu(v + w - 1, n - 1));
        return (v + w) - (q * n - q);
    };
    u32 v = 1;
    for (u32 y = 0; y < h; ++y) {
        u32 prev = wrap(v);
        store<u16>(p, 0);
        store<u16>(p + 2, static_cast<u16>(prev));
        store<u16>(p + 4, static_cast<u16>(v));
        p += 6;
        v = (v + 1) & 0xFFFF;
        if (w - 1 == 0) continue;
        s32 c = static_cast<s32>(w - 1);
        do {
            const u32 nx = wrap(v);
            store<u16>(p, static_cast<u16>(v - 1));
            store<u16>(p + 2, static_cast<u16>(prev));
            store<u16>(p + 4, static_cast<u16>(v));
            store<u16>(p + 6, static_cast<u16>(prev));
            store<u16>(p + 8, static_cast<u16>(nx));
            store<u16>(p + 10, static_cast<u16>(v));
            p += 12;
            prev = nx;
            --c;
            v = (v + 1) & 0xFFFF;
        } while (c > 0);
    }
    ret(ctx, p);
}

namespace {
constexpr u32 kSys = 0x011DE094u;  // the SystemState singleton (built on first use)

// First-use setup of kSys (flag byte 0x011DE280), as Obj_SetFromSingleton_004C0728.
inline void SysEnsure(PPUContext& ctx) {
    if (load<u8>(0x011DE280u) != 0) return;
    const u8 b = load<u8>(kSys + 32);
    store<u32>(kSys + 20, 0);
    store<u32>(kSys + 24, 0);
    store<u8>(kSys + 32, static_cast<u8>(b & 0x3Fu));
    call(ctx, 0x00726E54u, {kSys + 92, 0, 400});  // memset
    store<u32>(0x011972E8u, kSys);
    store<u8>(0x011DE280u, 1);
}
}  // namespace

// Obj* Clone4C07D0(const Obj* s)
// (no direct callers: a vtable clone) p = SmallAlloc(68); when it
// succeeds: p->24 = 8, p->4 (u16) = 1, p->0 = the vtable 0x00B21F40, p->28
// = kSys->16. Then (also through a null p, as the original) copies s's
// +24, +8 and the f32s +36, +40, +48, +44, +52..+64, sets p->28 and p->32 =
// kSys->16 (after the first-use setup of kSys) and returns p (f1 = that
// value).
DECOMP(004C07D0, Clone4C07D0) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 q = static_cast<u32>(call(ctx, 0x0055238Cu, {68}));  // SmallAlloc
    u32 p = 0;
    if (q != 0) {
        store<u32>(q + 24, 8);
        store<u16>(q + 4, 1);
        store<u32>(q, 0x00B21F40u);
        SysEnsure(ctx);
        p = q;
        Cw(p + 28, kSys + 16);
    }
    // a failed allocation writes through p = 0 (quirk kept)
    Cw(p + 24, s + 24);
    Cw(p + 8, s + 8);
    for (u32 off : {36u, 40u, 48u, 44u, 52u, 56u, 60u, 64u}) Cw(p + off, s + off);
    SysEnsure(ctx);
    Cw(p + 28, kSys + 16);
    const u32 v = load<u32>(kSys + 16);
    store<u32>(p + 32, v);
    ctx.fpr[1] = LfsF(v);
    ret(ctx, p);
}

// u32 Area_TryEnter2140C0(Area* o, Actor* a)
// (no direct callers: a handler) Only for a == the player (game
// *(0x00F65CA0) +4208 +8) unless the state o->428 is 0 or 3 (else 0). The
// level o->484 = K8 when |v| < K4, else K0 (K = 0x002140B4), with v = K0,
// or for d = o->216 with a link d->180 = l: l->16, replaced by
// T(0x01056B00) (the f32 at +72) below K0; with o->612 set, o->484 is
// instead what 00213870(o, &copy of game +0x128260) leaves in f1. Then
// (o->613 clear, l set) o joins l's list: o->616 = l->112 (whose +620 =
// o; else List_PushFront_00441C68(l) first), o->620 = 0, l->112 = o,
// o->613 = 1; Field96_SetIfMoved_00441D18(l; o->484) when l->112 is o;
// PushStack_Push(d->188; d->192) when set. Returns Area_Enter(o, a)
// unless o->428 is 0 or 3, else 1.
DECOMP(002140C0, Area_TryEnter2140C0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    Stack frame(ctx, 160);
    const u32 sp = frame.addr();
    constexpr u32 K = 0x002140B4u;
    const u32 st = load<u32>(o + 428);
    const u32 d = load<u32>(o + 216);
    const u32 g = load<u32>(0x00F65CA0u);
    if (!(st == 0 || st == 3) && a != load<u32>(g + 4208 + 8)) return ret(ctx, 0);
    u32 r28 = d;
    if (load<u8>(o + 612) != 0) {
        const u32 src = g + 0x128260u;
        CopyVec3(sp + 112, src);
        ctx.fpr[1] = Lf(src + 8);
        call(ctx, 0x00213870u, {o, sp + 112, src});
        r28 = load<u32>(o + 216);
        Sf(o + 484, ctx.fpr[1]);
    } else {
        f64 v = Lf(K);
        if (d != 0 && load<u32>(d + 180) != 0) {
            v = Lf(load<u32>(d + 180) + 16);
            if (v < Lf(K)) v = Lf(0x01056B00u + 72);
        }
        Sf(o + 484, std::fabs(v) < Lf(K + 4) ? Lf(K + 8) : Lf(K));
    }
    if (r28 != 0) {
        const u32 l = load<u32>(r28 + 180);
        if (l != 0 && load<u8>(o + 613) == 0) {
            u32 n = load<u32>(l + 112);
            if (n != 0) {
                store<u32>(n + 620, o);
            } else {
                call(ctx, 0x00441C68u, {l});  // List_PushFront_00441C68
                n = load<u32>(l + 112);
            }
            store<u32>(o + 620, 0);
            store<u32>(o + 616, n);
            store<u32>(l + 112, o);
            store<u8>(o + 613, 1);
        }
    }
    {
        const u32 e = load<u32>(o + 216);
        ctx.fpr[1] = Lf(o + 484);
        if (e != 0) {
            const u32 l = load<u32>(e + 180);
            if (l != 0 && load<u32>(l + 112) == o) call(ctx, 0x00441D18u, {l});  // Field96_SetIfMoved_00441D18 (f1)
        }
    }
    if (d != 0 && load<u32>(d + 188) != 0) callf(ctx, 0x0044D5BCu, {load<u32>(d + 188)}, {Lf(d + 192)});  // PushStack_Push
    const u32 st2 = load<u32>(o + 428);
    if (st2 != 0 && st2 != 3) {
        call(ctx, 0x0020E7A4u, {o, a});  // Area_Enter
        return;
    }
    ret(ctx, 1);
}

namespace {
// Takes area o out of the list of l (o->613 linked; prev o->620, next
// o->616, head l->112): List_Unlink_00441CB8(l) once the list is empty.
inline void AreaUnlink(PPUContext& ctx, u32 o, u32 l) {
    if (load<u8>(o + 613) == 0) return;
    const u32 prev = load<u32>(o + 620);
    u32 nx, pv = 0;
    if (prev != 0) {
        store<u32>(prev + 616, load<u32>(o + 616));
        nx = load<u32>(o + 616);
        pv = load<u32>(o + 620);
    } else {
        nx = load<u32>(o + 616);
        store<u32>(l + 112, nx);
    }
    if (nx != 0) store<u32>(nx + 620, pv);
    if (load<u32>(l + 112) == 0) call(ctx, 0x00441CB8u, {l});  // List_Unlink_00441CB8
    store<u8>(o + 613, 0);
}
}  // namespace

// u32 Area_TryLeave2142E0(Area* o, Actor* a)
// (no direct callers: a handler) The counterpart of Area_TryEnter2140C0
// (K = 0x002142D8): with o->612 set, or when |v| < K4 (v = K0, or the link
// l = o->216->180's +16, replaced by T(0x01056B00) below K0), o->484 = 0,
// Field96_SetIfMoved_00441D18(l; K0) when l->112 is o, and o leaves l's
// list. Then PushStack_Remove(d->188; d->192) when set (d = o->216, read
// without a null check), and Area_Leave(o, a) unless o->428 is 0 or 3
// (r3 = o->428 then).
DECOMP(002142E0, Area_TryLeave2142E0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    constexpr u32 K = 0x002142D8u;
    const u32 d = load<u32>(o + 216);
    auto field = [&](u32 r) {  // r = o->216 or d; returns the area to unlink from
        if (r == 0) return r;
        const u32 l = load<u32>(r + 180);
        if (l != 0 && load<u32>(l + 112) == o) {
            call(ctx, 0x00441D18u, {l});  // Field96_SetIfMoved_00441D18 (f1 = K0)
            return load<u32>(o + 216);
        }
        return r;
    };
    auto unlink = [&](u32 r) {
        if (r == 0) return;
        const u32 l = load<u32>(r + 180);
        if (l != 0) AreaUnlink(ctx, o, l);
    };
    if (load<u8>(o + 612) != 0) {
        store<u32>(o + 484, 0);
        ctx.fpr[1] = Lf(K);
        unlink(field(d));
    } else {
        f64 v = Lf(K);
        if (d != 0 && load<u32>(d + 180) != 0) {
            v = Lf(load<u32>(d + 180) + 16);
            if (v < Lf(K)) v = Lf(0x01056B00u + 72);
        }
        ctx.fpr[1] = std::fabs(v);
        if (std::fabs(v) < Lf(K + 4)) {
            ctx.fpr[1] = Lf(K);
            store<u32>(o + 484, 0);
            unlink(field(d));
        }
    }
    const u32 ps = load<u32>(d + 188);  // d may be null (read anyway)
    if (ps != 0) callf(ctx, 0x0044D614u, {ps}, {Lf(d + 192)});  // PushStack_Remove
    const u32 st = load<u32>(o + 428);
    if (st == 0 || st == 3) return ret(ctx, st);
    call(ctx, 0x0020EBC8u, {o, a});  // Area_Leave
}
