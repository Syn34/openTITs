// Game code, medium sweep part 21: hit tests and smaller callees of part 20.
#include "mid21.h"

using namespace sly4;
using namespace sly4::mid21;

namespace {
constexpr u32 kObjFindComponent = 0x000D1288u;  // Obj_FindComponent(o, kind, 0)
constexpr u32 kPartAt = 0x001BD51Cu;         // Field_At388x96_001BD51C(t, i): t->388 + 96 * i
constexpr u32 kPartBit15 = 0x001BD4A0u;      // Field_404Bit15_001BD4A0(t)
constexpr u32 kPartHit = 0x001F46D4u;        // 0x001F46D4(o, i): the hit on segment i
constexpr u32 kSegDistSq = 0x004A44C8u;      // Seg_DistSq4A44C8(p0, p1, q0, q1) -> f1
}  // namespace

// bool Part_HitTest_001F4DC0(Obj* o, const f32* p0, const f32* p1; f32 r2):
// with a part chain t = o->820 and bit 0x10000000 of o->980, walks the
// u16 t->392 - 1 segments: the points +48 of Field_At388x96(t, i) and
// (o->820, i + 1) (copied to sp+112 / sp+128) against the segment p0-p1 with
// Seg_DistSq4A44C8; the first segment with r2 >= distance^2 ends the walk.
// Then, when Field_404Bit15(o->820) and bit 0x100 of o->820->172:
// 0x001F46D4(o, i) and r3 = 1; every other way out returns 0.
DECOMP(001F4DC0, Part_HitTest_001F4DC0) {
    const u32 o = arg<u32>(ctx, 0), p0 = arg<u32>(ctx, 1), p1 = arg<u32>(ctx, 2);
    const f64 r2 = ctx.fpr[1];
    Stack frame(ctx, 96);
    const u32 sp = frame.addr() - 112;
    u32 t = load<u32>(o + 820);
    if (t == 0 || (load<u32>(o + 980) & 0x10000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    u32 i = 0;
    for (;;) {
        if (!(i < static_cast<u32>(load<u16>(t + 392) - 1))) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 a = static_cast<u32>(call(ctx, kPartAt, {t, i})) + 48;
        for (u32 k = 0; k < 3; ++k) Cw(sp + 112 + 4 * k, a + 4 * k);
        const u32 b = static_cast<u32>(call(ctx, kPartAt, {load<u32>(o + 820), i + 1})) + 48;
        for (u32 k = 0; k < 3; ++k) Cw(sp + 128 + 4 * k, b + 4 * k);
        ctx.fpr[1] = Lf(b + 8);
        call(ctx, kSegDistSq, {p0, p1, sp + 112, sp + 128});
        if (r2 >= ctx.fpr[1]) break;
        ++i;
        t = load<u32>(o + 820);
    }
    if (call(ctx, kPartBit15, {load<u32>(o + 820)}) == 0 || (load<u32>(load<u32>(o + 820) + 172) & 0x100u) == 0) {
        ctx.gpr[3] = 0;  // r28, never set
        return;
    }
    call(ctx, kPartHit, {o, i});
    ctx.gpr[3] = 1;
}

// u32 Obj_CanTakeHitBase_000DF960(Obj* o, Obj* src, s32 kind): the base hit
// filter. Kind 3 needs the component 27 (Obj_FindComponent(o, 27, 0)) with
// its byte +16 set and s32 +68 <= 1, else 0. A handler o->244 must accept
// (vfn[84]() != 0), else 0. Without src or with bit 0x01 of src->192 clear:
// 1. Else a = o->vfn[300](o, kind, src) (0: 1) and r3 = ((src->vfn[296](src,
// kind, o) & a) == a).
DECOMP(000DF960, Obj_CanTakeHitBase_000DF960) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), kind = arg<u32>(ctx, 2);
    if (static_cast<s32>(kind) == 3) {
        const u32 c = static_cast<u32>(call(ctx, kObjFindComponent, {o, 27, 0}));
        if (c == 0 || load<u8>(c + 16) == 0 || static_cast<s32>(load<u32>(c + 68)) > 1) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    const u32 h = load<u32>(o + 244);
    if (h != 0 && static_cast<u32>(VCall(ctx, h, 84, {h})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (src == 0 || (load<u32>(src + 192) & 1) == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 a = static_cast<u32>(VCall(ctx, o, 300, {o, kind, src}));
    if (a == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 b = static_cast<u32>(VCall(ctx, src, 296, {src, kind, o}));
    ctx.gpr[3] = (b & a) == a ? 1 : 0;
}

// Scene_RemoveObj12544(Scene* L, Obj* x) (L = *0x00F557F0): drops x from the
// scene lists. One pass i < s32 L->0 over list B (L+592) and list A (L+1616,
// i < s32 L->4): A[i] == x swaps the last A entry in (L->4 -= 1, the old last
// slot = 0); B[i]->52 == x clears it; B[i] == x swaps the last B entry in
// (L->0 -= 1, slot = 0); either removal re-examines slot i. Then, when
// x->vfn[124]() (x is a slot owner), x->60 is swapped out of list C (L+3664,
// s32 L->16) the same way. Otherwise every C[j] whose handler t = C[j]->244
// has t->256 == x->48 (non-zero) is released: t->256->vfn[40](t->256, 0)
// unless t->vfn[124](), t->256 = 0, t->48->vfn[96](t->48, 0) and the u16
// t->48->164->40 gets bit 0x1000 cleared and 0x400 set. r3 as the original
// leaves it (the C index / slot address / L->16).
DECOMP(00012544, Scene_RemoveObj12544) {
    const u32 L = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    s32 i = 0;
    u32 r4 = load<u32>(L);
    if (static_cast<s32>(r4) > 0) {
        u32 pa = L + 1616, pb = L + 592;
        do {
            const u32 m = load<u32>(L + 4);
            bool rem = false;
            if (i < static_cast<s32>(m) && load<u32>(pa) == x) {
                store<u32>(L + 4, m - 1);
                r4 = load<u32>(L);
                store<u32>(pa, load<u32>(L + 1616 + 4 * (m - 1)));
                store<u32>(L + 1616 + 4 * load<u32>(L + 4), 0);
                rem = true;
            }
            u32 r10 = r4;
            const u32 b = load<u32>(pb);
            if (load<u32>(b + 52) == x) store<u32>(b + 52, 0);
            if (b == x) {
                r4 -= 1;
                store<u32>(L, r4);
                store<u32>(pb, load<u32>(L + 592 + 4 * r4));
                r4 = load<u32>(L);
                r10 = load<u32>(L);
                store<u32>(L + 592 + 4 * r4, 0);
                rem = true;
            }
            if (rem) {
                pa -= 4;
                pb -= 4;
                --i;
            }
            r4 = r10;
            ++i;
            pa += 4;
            pb += 4;
        } while (i < static_cast<s32>(r4));
    }
    if (static_cast<u32>(VCall(ctx, x, 124, {x})) != 0) {
        const u32 n = load<u32>(L + 16);
        const u32 y = load<u32>(x + 60);
        u32 r3 = 0;
        for (s32 j = 0; j < static_cast<s32>(n); ++j) {
            if (load<u32>(L + 3664 + 4 * j) == y) {
                store<u32>(L + 16, n - 1);
                store<u32>(L + 3664 + 4 * j, load<u32>(L + 3660 + 4 * n));
                r3 = L + 3664 + 4 * load<u32>(L + 16);
                store<u32>(r3, 0);
                ctx.gpr[3] = r3;
                return;
            }
            r3 = j + 1;
        }
        ctx.gpr[3] = r3;
        return;
    }
    u32 n = load<u32>(L + 16);
    for (s32 j = 0; j < static_cast<s32>(n); ++j, n = load<u32>(L + 16)) {
        const u32 t = load<u32>(load<u32>(L + 3664 + 4 * j) + 244);
        if (t == 0) continue;
        const u32 h = load<u32>(t + 256);
        if (!(h == load<u32>(x + 48) && h != 0)) continue;
        if (static_cast<u32>(VCall(ctx, t, 124, {t})) == 0) {
            const u32 h2 = load<u32>(t + 256);
            VCall(ctx, h2, 40, {h2, 0});
        }
        const u32 a = load<u32>(t + 48);
        store<u32>(t + 256, 0);
        VCall(ctx, a, 96, {a, 0});
        const u32 c = load<u32>(load<u32>(t + 48) + 164);
        if (c != 0) store<u16>(c + 40, static_cast<u16>((load<u16>(c + 40) & ~0x1000) | 0x400));
    }
    ctx.gpr[3] = n;
}

namespace {
constexpr u32 kAnimRestart = 0x002775A8u;  // Anim_Restart2775A8
constexpr u32 kChildC = 0x00405C18u;       // child state 12 (Child*; f32 dt)
constexpr u32 kChildB = 0x00405FB8u;       // child state 11
constexpr u32 kChildA = 0x00406A40u;       // child state 10
constexpr u32 kChildRun = 0x00406E60u;     // child states 1 / 2

// The "start" child states: the timers c->116 (= 0) / c->124 / c->128, state
// c->112 = 10 (c->120 = 0 too for state 9); with an owner o = c->4 its
// position o+144 is copied to c+96 and, when o->vfn[60]() (the animator) is
// set, Anim_Restart2775A8(o->vfn[60](), key, 0, 0, -1, 0, 0, 0, -, -, 0, -1;
// 1.0, 0.25, 0.0, 0.0). `swapped`: the first vfn[60] call of state 9 has the
// entry address in r4 and the OPD in r5 (the reverse of the usual order).
void ChildStart(PPUContext& ctx, u32 c, u32 t124, u32 t128, bool st9, u32 key) {
    constexpr u32 C = 0x00407120u;  // 0.0f, 1.0f, 0.25f
    store<u32>(c + 116, 0);
    store<u32>(c + 124, t124);
    const u32 o = load<u32>(c + 4);
    store<u32>(c + 128, t128);
    store<u32>(c + 112, 10);
    if (st9) store<u32>(c + 120, 0);
    ctx.gpr[3] = o;
    if (o == 0) return;
    for (u32 k = 0; k < 3; ++k) Cw(c + 96 + 4 * k, o + 144 + 4 * k);
    ctx.fpr[1] = Lf(o + 152);
    u32 r;
    if (st9) {
        const u32 opd = load<u32>(load<u32>(o) + 60);
        r = static_cast<u32>(call_ptr(ctx, opd, {o, load<u32>(opd), opd}));
    } else {
        r = static_cast<u32>(VCall(ctx, o, 60, {o}));
    }
    if (r == 0) return;
    const u32 o2 = load<u32>(c + 4);
    const u32 an = static_cast<u32>(VCall(ctx, o2, 60, {o2}));
    // the 9th / 10th arguments are the uninitialised stack words 112(r1) / 120(r1)
    callf(ctx, kAnimRestart, {an, key, 0, 0, ~0ull, 0, 0, 0, 0, 0, 0, ~0ull}, {Lf(C + 4), Lf(C + 8), Lf(C), Lf(C)});
}
}  // namespace

// Spawner_TickChild_00407130(Child* c; f32 dt): the child state machine
// c->112: 0 -> 1 with c->124 = c->60; 1 / 2 -> 0x00406E60(c); 3 / 6 / 9
// start the 1.2 / 3.5 (key 0x9284C2DB), 1.7 / 1.9 and 1.5 / 2.5 (both key
// 0xFD6C2598) phases (ChildStart); 10 / 11 / 12 -> 0x00406A40 / 0x00405FB8
// / 0x00405C18(c) (dt stays in f1). Other states do nothing (r3 = the
// state).
DECOMP(00407130, Spawner_TickChild_00407130) {
    const u32 c = arg<u32>(ctx, 0);
    const s32 s = static_cast<s32>(load<u32>(c + 112));
    ctx.gpr[3] = static_cast<u32>(s);
    switch (s) {
    case 0:
        ctx.gpr[3] = 1;
        ctx.fpr[1] = Lf(c + 60);
        store<u32>(c + 112, 1);
        Cw(c + 124, c + 60);
        break;
    case 1:
    case 2: call(ctx, kChildRun, {c}); break;
    case 3: ChildStart(ctx, c, 0x3F99999Au, 0x40600000u, false, 0x9284C2DBu); break;
    case 6: ChildStart(ctx, c, 0x3FD9999Au, 0x3FF33333u, false, 0xFD6C2598u); break;
    case 9: ChildStart(ctx, c, 0x3FC00000u, 0x40200000u, true, 0xFD6C2598u); break;
    case 10: call(ctx, kChildA, {c}); break;
    case 11: call(ctx, kChildB, {c}); break;
    case 12: call(ctx, kChildC, {c}); break;
    default: break;
    }
}

namespace {
constexpr u32 kSmallAlloc = 0x0055238Cu;    // SmallAlloc(size)
constexpr u32 kReserve = 0x00551718u;       // PtrVector_Reserve(v, n)
constexpr u32 kChildInit = 0x00405560u;     // 0x00405560(child, def, spawner, 0x802B5FF9, def->8)
constexpr u32 kChildVt = 0x00B20990u;       // the spawned child's vtable
}  // namespace

// Child* Spawner_Spawn_004089F0(Spawner* o; f32 scale): a 224-byte child p
// = SmallAlloc(224) (vtable 0x00B20990, state +76 = 3, +60 = 5.0f, the
// other header words 0) is appended to the u16-counted vector o+100 (count
// +108, capacity +104, doubled by PtrVector_Reserve when full; high-water
// mark +110), o->200 += 1, 0x00405560(p, o->88, o, 0x802B5FF9, o->88->8),
// o->212 = 0 and p->116 = scale * o->88->28. r3 = p, f1 = p->116. A failed
// allocation still stores the null pointer and writes 0x74 (quirk).
DECOMP(004089F0, Spawner_Spawn_004089F0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 scale = ctx.fpr[1];
    u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {224}));
    if (p != 0) {
        store<u32>(p + 4, 0);
        store<u32>(p + 0, kChildVt);
        store<u32>(p + 8, 0);
        for (u32 k = 28; k <= 56; k += 4) store<u32>(p + k, 0);
        store<u32>(p + 64, 0);
        store<u32>(p + 60, 0x40A00000u);
        store<u32>(p + 68, 0);
        store<u32>(p + 72, 0);
        store<u32>(p + 80, 0);
        store<u32>(p + 76, 3);
        store<u32>(p + 84, 0);
        for (u32 k = 112; k <= 128; k += 4) store<u32>(p + k, 0);
    }
    u32 n = load<u16>(o + 108);
    const u32 cap = load<u16>(o + 104);
    if (n == cap) {
        call(ctx, kReserve, {o + 100, (cap << 1) & 0x1FFFEu});
        n = load<u16>(o + 108);
    }
    store<u32>(load<u32>(o + 100) + ((n << 2) & 0x3FFFCu), p);
    const u32 n1 = load<u16>(o + 108) + 1u;
    const u32 hw = load<u16>(o + 110);
    const u32 def = load<u32>(o + 88);
    const u32 cnt = load<u32>(o + 200);
    store<u16>(o + 108, static_cast<u16>(n1));
    const u32 d8 = load<u32>(def + 8);
    store<u16>(o + 110, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    store<u32>(o + 200, cnt + 1);
    call(ctx, kChildInit, {p, def, o, 0x802B5FF9u, d8});
    const u32 def2 = load<u32>(o + 88);
    store<u32>(o + 212, 0);
    const f64 v = S(scale * Lf(def2 + 28));
    ctx.fpr[1] = v;
    Sf(p + 116, v);
    ctx.gpr[3] = p;
}

// Obj_ProbeContacts_001CCF40(Obj* o, u8* a, u8* b) (leaf): with o->216 set,
// *a / *b = o->768 / o->772 <= 0x001CCF3C's threshold; a set u16 o->820
// forces both to 1; with a link o->752 both become (u16 link->352 != 0);
// otherwise the first world slot q (u8 count +29 of *0x00F65CA0, slots of
// 0x20C80 bytes at +4208, q = slot->8) whose position q+144 lies within
// squared distance o->768 of o+608 sets *a = 1, and *b = 1 when also within
// o->772. r3 / f1 as the original leaves them.
DECOMP(001CCF40, Obj_ProbeContacts_001CCF40) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    constexpr u32 kThresh = 0x001CCF3Cu;
    constexpr u32 kWorld = 0x00F65CA0u;
    ctx.gpr[3] = o;
    if (load<u32>(o + 216) == 0) return;
    const f64 c = Lf(kThresh);
    ctx.fpr[1] = c;
    store<u8>(a, Lf(o + 768) <= c ? 1 : 0);
    store<u8>(b, Lf(o + 772) <= c ? 1 : 0);
    if (load<u16>(o + 820) != 0) {
        store<u8>(a, 1);
        store<u8>(b, 1);
        ctx.gpr[3] = 1;
        return;
    }
    const u32 link = load<u32>(o + 752);
    if (link != 0) {
        const u32 h = load<u16>(link + 352);
        store<u8>(a, h != 0 ? 1 : 0);
        store<u8>(b, h != 0 ? 1 : 0);
        ctx.gpr[3] = h;
        return;
    }
    const f64 r1 = Lf(o + 768);
    ctx.fpr[1] = r1;
    const f64 px = Lf(o + 608), py = Lf(o + 612), pz = Lf(o + 616);
    for (u32 i = 0, off = 0;; ++i, off += 0x20C80u) {
        const u32 w = load<u32>(kWorld);
        if (!(i < load<u8>(w + 29))) return;
        const u32 q = load<u32>(w + off + 4208 + 8);
        if (q == 0) continue;
        const f64 dy = S(Lf(q + 148) - py), dx = S(Lf(q + 144) - px), dz = S(Lf(q + 152) - pz);
        const f64 d = Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)));
        if (!(d <= r1)) continue;
        const f64 r2 = Lf(o + 772);
        ctx.fpr[1] = r2;
        store<u8>(a, 1);
        if (d <= r2) {
            store<u8>(b, 1);
            ctx.gpr[3] = 1;
        }
        return;
    }
}

namespace {
constexpr u32 kSetHash = 0x004CF988u;     // Effect_SetHash4CF988(fx, hash)
constexpr u32 kSelectColor = 0x00289DC4u;  // Obj_SelectColor_00289DC4(x, 0)
constexpr u32 kSetTarget = 0x00296758u;    // Obj_SetTarget_00296758(x, 0, 0)
constexpr u32 kPathRest = 0x01164B40u;     // the rest vector copied into o+832 / +848 / +864
}  // namespace

// Path_SetActive_001D218C(Obj* o, u8 on): a change of bit 0x08000000 of
// o->972 (to bit 0 of on) switches the path: on -> Effect_SetHash(o->356,
// 0xA6CB0022) (with an effect); off -> o->888 = 0, the follower o->820 loses
// bit 0x40000000 of +1268 and Obj_SetTarget_00296758(o->820, 0, 0), o->816
// gets Obj_SelectColor_00289DC4(o->816, 0), the vectors o+832 / +848 / +864
// = *0x01164B40, o->880 = o->884 = 0 and Effect_SetHash(o->356, 0xF6239C09).
// Then (always) with bit 0x20000000 of o->972: o->vfn[88](o, active, 1, 1,
// 1), else o->816->vfn[48](o->816, active, 1) when set. r3 as left.
DECOMP(001D218C, Path_SetActive_001D218C) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 972);
    if (((w >> 27) & 1) != (v & 0xFF)) {
        store<u32>(o + 972, (w & ~0x08000000u) | ((v & 1) << 27));
        if ((v & 1) != 0) {
            const u32 fx = load<u32>(o + 356);
            if (fx != 0) call(ctx, kSetHash, {fx, 0xA6CB0022u});
        } else {
            const u32 f = load<u32>(o + 820);
            store<u32>(o + 888, 0);
            if (f != 0) {
                store<u32>(f + 1268, load<u32>(f + 1268) & ~0x40000000u);
                call(ctx, kSetTarget, {load<u32>(o + 820), 0, 0});
            }
            const u32 t = load<u32>(o + 816);
            if (t != 0) call(ctx, kSelectColor, {t, 0});
            Cw(o + 832, kPathRest);
            Cw(o + 836, kPathRest + 4);
            Cw(o + 840, kPathRest + 8);
            Cw(o + 848, kPathRest);
            Cw(o + 852, kPathRest + 4);
            Cw(o + 856, kPathRest + 8);
            Cw(o + 864, kPathRest);
            const u32 w868 = load<u32>(kPathRest + 4);
            const u32 fx = load<u32>(o + 356);
            store<u32>(o + 868, w868);
            const u32 w872 = load<u32>(kPathRest + 8);
            store<u32>(o + 880, 0);
            store<u32>(o + 872, w872);
            store<u32>(o + 884, 0);
            ctx.fpr[1] = LfsF(w872);
            if (fx != 0) call(ctx, kSetHash, {fx, 0xF6239C09u});
        }
    }
    const u32 w2 = load<u32>(o + 972);
    const u32 act = (w2 >> 27) & 1;
    if ((w2 & 0x20000000u) != 0) {
        ctx.gpr[3] = VCall(ctx, o, 88, {o, act, 1, 1, 1});
        return;
    }
    const u32 t = load<u32>(o + 816);
    ctx.gpr[3] = act;
    if (t != 0) ctx.gpr[3] = VCall(ctx, t, 48, {t, act, 1});
}

// u32 Path_Step_001D0450(Obj* o, u8 hold): one step along the path records
// (28 bytes each at o->556, index o->892, u16 count o->564; direction bit
// 0x40000000 of o->972 = backwards). o->904 = rec->8, o->896 = 0; unless
// hold, o->896 = the forward / backward speed o->956 / o->960 when > 0, else
// rec->20 / rec->24, and o->900 = rec->8 when that is > 0. With more than
// one record the index moves one step; at the end (last record forwards,
// record 0 backwards) the end mode o->964 decides: 0 reverses the direction
// bit and steps back, 1 wraps (to 0 / count - 1, o->900 = that rec->8,
// r3 = 1), others step past the end. o->944 = 0, or, when the index moved,
// C[1] / C[2] / C[0] as the new rec->4 is above / below / equal to the old
// one (unsigned); o->892 = the new index. r3 = 1 after a wrap, else 0.
DECOMP(001D0450, Path_Step_001D0450) {
    const u32 o = arg<u32>(ctx, 0), hold = arg<u32>(ctx, 1) & 0xFF;
    constexpr u32 C = 0x001D0444u;  // 0.0f and the two record-change values
    const u32 i = load<u32>(o + 892);
    const u32 base = load<u32>(o + 556);
    const u32 rec = base + 28 * i;
    u32 r3 = 0;
    Cw(o + 904, rec + 8);
    f64 f1 = Lf(C);
    store<u32>(o + 896, 0);
    if (hold == 0) {
        const bool rev = (load<u32>(o + 972) & 0x40000000u) != 0;
        u32 sp = rev ? load<u32>(o + 960) : load<u32>(o + 956);  // lfs / stfs: raw words
        if (!(LfsF(sp) > f1)) sp = rev ? load<u32>(rec + 24) : load<u32>(rec + 20);
        store<u32>(o + 896, sp);
        if (LfsF(sp) > f1) Cw(o + 900, rec + 8);
    }
    const u32 n = load<u16>(o + 564);
    u32 j = i;
    if (n > 1) {
        u32 w = load<u32>(o + 972);
        const bool rev = (w & 0x40000000u) != 0;
        bool step = true;
        if ((!rev && !(i < n - 1)) || (rev && i == 0)) {
            const u32 mode = load<u32>(o + 964);
            if (mode == 0) {
                w = (w & ~0x40000000u) | (rev ? 0 : 0x40000000u);
                store<u32>(o + 972, w);
            } else if (mode == 1) {
                j = rev ? n - 1 : 0;
                r3 = 1;
                Cw(o + 900, base + 28 * j + 8);
                step = false;
            }
        }
        if (step) j = i + ((w & 0x40000000u) != 0 ? 0xFFFFFFFFu : 1u);
    }
    store<u32>(o + 944, 0);
    if (j != i) {
        const u32 a = load<u32>(base + 28 * j + 4), b = load<u32>(rec + 4);
        if (a > b) f1 = Lf(C + 4);
        else if (a < b) f1 = Lf(C + 8);
        Sf(o + 944, f1);
    }
    store<u32>(o + 892, j);
    ctx.fpr[1] = f1;
    ctx.gpr[3] = r3;
}
