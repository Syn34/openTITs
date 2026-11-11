// Game code, medium sweep part 15: lists, contacts and probes.
#include "mid15.h"

using namespace sly4;
using namespace sly4::mid15;

namespace {

// The "before" test of PrioList_Insert445F08 between the node (raw value
// b = *node->0, value nv = node->0 ? b : 0) and a list entry (raw x,
// value xv, flag byte +97): a zero raw x puts the node first when b is
// set, a zero b never does (the walk tests those cases itself); then
// s32 +92 descending, then (when
// the entry's +97 is clear) s32 +88 descending. `tie` is the result for
// equal +88 (the head test takes it, the walk does not).
inline bool PrioBefore(u32 b, u32 nv, u32 x, u32 xv, u32 entry, bool tie) {
    if (x == 0) return b != 0;
    if (b == 0) return false;
    const s32 x92 = static_cast<s32>(load<u32>(xv + 92)), n92 = static_cast<s32>(load<u32>(nv + 92));
    if (x92 != n92) return x92 < n92;
    if (load<u8>(entry + 97) != 0) return false;
    const s32 x88 = static_cast<s32>(load<u32>(xv + 88)), n88 = static_cast<s32>(load<u32>(nv + 88));
    if (x88 == n88) return tie;
    return x88 < n88;
}

}  // namespace

// void* PrioList_Insert445F08(Mgr* m, Node* n)
// (called by 00446198) Resets the node's state (+99 = 1, +98 = +97 = 0,
// +112 = 0, +108 = 1) and links it {+116 prev, +120 next}: with byte +96
// set, at the tail of the plain list {+8200 head, +8204 tail}; otherwise
// into the sorted list at +8208 (see PrioBefore; the value of an entry is
// the word its +0 handle points at). A null handle is still dereferenced
// for the raw test (load from 0), the original's quirk. Returns (leftover
// r3) the manager, or the entry the node was linked after.
DECOMP(00445F08, PrioList_Insert445F08) {
    const u32 m = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    store<u8>(n + 99, 1);
    const u32 b96 = load<u8>(n + 96);
    store<u8>(n + 98, 0);
    store<u8>(n + 97, 0);
    store<u32>(n + 112, 0);
    const u32 h = load<u32>(n);
    store<u32>(n + 108, 1);
    if (b96 != 0) {
        const u32 tail = load<u32>(m + 8204);
        if (tail != 0) {
            store<u32>(tail + 120, n);
            store<u32>(n + 116, load<u32>(m + 8204));
            store<u32>(m + 8204, n);
        } else {
            store<u32>(m + 8200, n);
            store<u32>(m + 8204, n);
            store<u32>(n + 116, 0);
        }
        ret(ctx, m);
        return;
    }
    const u32 head = load<u32>(m + 8208);
    bool first = head == 0;
    u32 b = 0, nv = 0;
    if (!first) {
        b = load<u32>(h);  // from 0 for a null handle (the original's quirk)
        nv = h != 0 ? b : 0;
        const u32 p = load<u32>(head), x = load<u32>(p), xv = p != 0 ? x : 0;
        first = PrioBefore(b, nv, x, xv, head, true);
    }
    if (first) {
        store<u32>(m + 8208, n);
        store<u32>(n + 116, 0);
        store<u32>(n + 120, head);
        if (head != 0) store<u32>(head + 116, n);
        ret(ctx, m);
        return;
    }
    u32 cur = head, next;
    for (;;) {
        next = load<u32>(cur + 120);
        if (next == 0) break;
        const u32 q = load<u32>(next), x = load<u32>(q), xv = q != 0 ? x : 0;
        // the walk goes on past `next` unless the node sorts before it; a
        // null value on either side goes on exactly when next's is set
        const bool on = (b == 0 || x == 0) ? x != 0 : !PrioBefore(b, nv, x, xv, next, false);
        if (!on) break;
        cur = next;
    }
    store<u32>(n + 120, next);
    store<u32>(n + 116, cur);
    store<u32>(cur + 120, n);
    if (next != 0) store<u32>(next + 116, n);
    ret(ctx, cur);
}

namespace {
// The owner of a collidable root: root + (s8)+16 when (s8)+24 == 1, else 0.
inline u32 ContactOwner(u32 r) {
    if (static_cast<s8>(load<u8>(r + 24)) != 1) return 0;
    return r + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(r + 16))));
}
// The low word of the u64 at +8 of the first 16-byte entry tagged `tag` in
// the array {+124 data, +128 s32 count} of o, or 0. `n` gets the count.
inline u32 TagFind(u32 o, u32 tag, s32& n) {
    for (s32 i = 0;; ++i) {
        n = static_cast<s32>(load<u32>(o + 128));
        if (i >= n) return 0;
        const u32 d = load<u32>(o + 124);
        if (load<u32>(d + 16 * i) == tag) return load<u32>(d + 16 * i + 12);
    }
}
}  // namespace

// void Contact_Check16A000(Obj* this, const ContactEvent* ev)
// (reached through AdjThunk_0016A330) ev {+0, +4 collidables, +16 the
// contact point (+20 f32 separation), +28 f32 speed}. Of the two roots
// (parent chain at +12) the one whose owner is this+388 is "self", the
// other "other" (when the first root's owner is not this+388 the order
// flips and the separation is negated). The other's owner is searched for
// a 0xA734004D entry holding 0x7A30D7E3, then a 0x91C7CF5B entry giving a
// candidate object; it counts when both +216 links exist and cand->216->52
// == this->216->272. Neither root's +28 low five bits may be 5. A match
// sets +456 = cand, +449 = +448 = 1; with no match, a fast approach (speed
// below -(0x0103901C)->72) with a separation in (k[1], k[0]] (0x00169FF4)
// and no candidate at all sets +448 = 1. f1 is left as the separation.
DECOMP(0016A000, Contact_Check16A000) {
    const u32 self = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    f64 sep = Lf(load<u32>(ev + 16) + 20);
    const u32 ra = Root12(load<u32>(ev));
    u32 other, mine;
    if (ContactOwner(ra) == load<u32>(self + 388)) {
        other = Root12(load<u32>(ev + 4));
        mine = ra;
    } else {
        other = ra;
        mine = Root12(load<u32>(ev + 4));
        sep = -sep;
    }
    ctx.fpr[1] = sep;
    const u32 ow = ContactOwner(other);
    u32 cand = 0, hit = 0;
    if (ow != 0) {
        s32 n;
        const u32 v = TagFind(ow, 0xA734004Du, n);
        if (v == 0x7A30D7E3u) {
            for (s32 i = 0; i < n; ++i) {
                const u32 d = load<u32>(ow + 124);
                if (load<u32>(d + 16 * i) == 0x91C7CF5Bu) {
                    cand = load<u32>(d + 16 * i + 12);
                    break;
                }
            }
            if (cand != 0) {
                const u32 s216 = load<u32>(self + 216), c216 = load<u32>(cand + 216);
                if (s216 != 0 && c216 != 0 && load<u32>(c216 + 52) == load<u32>(s216 + 272)) hit = cand;
            }
        }
    }
    const u32 mk = load<u32>(mine + 28) & 31u, ok = load<u32>(other + 28) & 31u;
    if (mk == 5 || ok == 5) return;
    if (hit != 0) {
        store<u32>(self + 456, hit);
        store<u8>(self + 449, 1);
        store<u8>(self + 448, 1);
        return;
    }
    const f64 spd = Lf(ev + 28);
    if (!(spd < -Lf(0x0103901Cu + 72))) return;
    const u32 k = 0x00169FF4u;  // f32 0.2, f32 -0.2
    if (!(sep <= Lf(k))) return;
    if (!(sep > Lf(k + 4))) return;
    if (cand != 0) return;
    store<u8>(self + 448, 1);
}

// void Probe_Update3CDEA0(Probe* o, const Mat* m, const f32 dir[3])
// (called by 003CE2C0, 003D0598) Fills the ray record at o+112 (r): the
// start (r+32) is copied to r+64; the end r+80 = A+64 + w*t + 0.06*(A+32),
// where A = o->72, w = the unit (A+48 x, 0, A+48 z), and t (clamped to
// [-1.4, 1.4]) is w . (Q - A+64 + 1.2 * (w . d) * w), Q the point of the
// line from A+64 along u = unit(A+64 - o->68+64 in x/z) nearest the start
// (offset 0.13 back), d = start - (m+48 + 0.3 * dir) normalised when longer
// than 0.1. r+0 = 6, r+96 = 0, r+100 = 1.0f; with byte +93 set r+104
// += 0.2. Constants at 0x003CDE78. f1: the last value it computed.
DECOMP(003CDEA0, Probe_Update3CDEA0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1) + 48, q = arg<u32>(ctx, 2);
    const u32 r = load<u32>(o + 112);
    const u32 k = 0x003CDE78u;
    const f64 k0 = Lf(k), k1 = Lf(k + 4);
    f64 dy = Fmadds(Lf(q + 4), k1, k0);
    const f64 sx = Lf(r + 32), sy = Lf(r + 36), sz = Lf(r + 40);
    f64 dx = Fmadds(Lf(q), k1, S(sx - Lf(m)));
    f64 dz = Fmadds(Lf(q + 8), k1, S(sz - Lf(m + 8)));
    f64 len = S(std::sqrt(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)))));
    const u32 flag = load<u8>(o + 93);
    const u32 a = load<u32>(o + 72), bo = load<u32>(o + 68);
    if (len > Lf(k + 8)) {
        const f64 inv = S(Lf(k + 12) / len);
        dx = S(dx * inv);
        dy = S(dy * inv);
        dz = S(dz * inv);
    }
    const f64 kk = S(k0 * k0);
    const f64 px = Lf(a + 64), pz = Lf(a + 72), py = Lf(a + 68);
    f64 ux = S(px - Lf(bo + 64)), uz = S(pz - Lf(bo + 72));
    const f64 wx0 = Lf(a + 48), wz0 = Lf(a + 56);
    const f64 one = Lf(k + 12), k4 = Lf(k + 16), k5 = Lf(k + 20), k6 = Lf(k + 24);
    const f64 iu = S(one / S(std::sqrt(Fmadds(uz, uz, Fmadds(ux, ux, kk)))));
    const f64 iw = S(one / S(std::sqrt(Fmadds(wz0, wz0, Fmadds(wx0, wx0, kk)))));
    const f64 uy = S(k0 * iu);
    ux = S(ux * iu);
    uz = S(uz * iu);
    const f64 ey = S(Fnmsubs(uy, k4, py) - sy);
    const f64 ex = S(Fnmsubs(ux, k4, px) - sx);
    const f64 ez = S(Fnmsubs(uz, k4, pz) - sz);
    const f64 t0 = Fmadds(uz, ez, Fmadds(ux, ex, S(uy * ey)));
    const f64 wy = S(k0 * iw), wx = S(wx0 * iw), wz = S(wz0 * iw);
    const f64 cy = Fmadds(uy, t0, sy), cx = Fmadds(ux, t0, sx), cz = Fmadds(uz, t0, sz);
    const f64 wd = Fmadds(wz, dz, Fmadds(wx, dx, S(wy * dy)));
    const f64 gy = Fmadds(S(wy * wd), k5, S(cy - py));
    const f64 gx = Fmadds(S(wx * wd), k5, S(cx - px));
    const f64 gz = Fmadds(S(wz * wd), k5, S(cz - pz));
    f64 t = Fmadds(wz, gz, Fmadds(wx, gx, S(wy * gy)));
    if (t < k6) t = k6;
    const f64 k7 = Lf(k + 28);
    if (t > k7) t = k7;
    const f64 ox = Fmadds(wx, t, px), oy = Fmadds(wy, t, py), oz = Fmadds(wz, t, pz);
    const f64 k8 = Lf(k + 32);
    const f64 ax = Lf(a + 32), ay = Lf(a + 36), az = Lf(a + 40);
    Cw(r + 64, r + 32);
    Cw(r + 68, r + 36);
    Cw(r + 72, r + 40);
    const f64 rx = Fmadds(ax, k8, ox), ry = Fmadds(ay, k8, oy), rz = Fmadds(az, k8, oz);
    store<u32>(r + 100, 0x3F800000u);
    store<u32>(r + 96, 0);
    store<u32>(r + 0, 6);
    Sf(r + 80, rx);
    Sf(r + 84, ry);
    Sf(r + 88, rz);
    ctx.fpr[1] = rx;
    if (flag != 0) {
        const f64 v = S(Lf(r + 104) + Lf(k + 36));
        Sf(r + 104, v);
        ctx.fpr[1] = v;
    }
    ret(ctx, 0x3F800000u);  // leftover r3: the lis of 1.0f
}

// void Rec_Init7576D8(Rec* o, u32 a, u32 b, u32 c, u32 d, f32 f1..f6,
//                     f32 f7, f32 f8..f13, [stack] ...)
// (called by 001F9078, 001F9BA0, 001FE170) Initialises a 256-byte record
// from 5 GPR, 13 FPR and 21 stack arguments (read at the caller's frame:
// floats at +196..+252 step 8, words at +260, +268 (a byte table or 0),
// +276..+332 step 8, +356, +364, and two vec3 pointers at +412, +420).
// +0 = 2, then | 0x20; +12/16/20 = a, b, c; +36/+40/+72 = -1.0f; +76 =
// f7; +120 (u16) = (old & 0xF) | 0x8200; +88.. = the five floats F - k
// (F = +120 bits 4..7, re-read, so 0); +128 / +144 the two vec3s; +160 = d;
// +164.. the FPR arguments, +212.. the stack floats; +244 the word at
// +260; +252 gets the low bits of eleven stack words as flags 0x8000..
// 0x20 (0x2000, 0x10 and 0x8 cleared); +248..+253 the six table bytes (1
// each with no table).
DECOMP(007576D8, Rec_Init7576D8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const f64 fa[13] = {ctx.fpr[1], ctx.fpr[2], ctx.fpr[3], ctx.fpr[4], ctx.fpr[5], ctx.fpr[6], ctx.fpr[7],
                        ctx.fpr[8], ctx.fpr[9], ctx.fpr[10], ctx.fpr[11], ctx.fpr[12], ctx.fpr[13]};
    const u32 h = load<u16>(o + 120);
    const u32 v120 = (h & 0xFu) | 0x8200u;
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    for (u32 k = 24; k < 32; ++k) store<u8>(o + k, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0xBF800000u);
    store<u32>(o + 40, 0xBF800000u);
    store<u8>(o + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 72, 0xBF800000u);
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    Sf(o + 76, fa[6]);
    store<u32>(o + 80, 0);
    store<u32>(o + 84, 0);
    store<u32>(o + 108, 0);
    store<u32>(o + 112, 0);
    store<u32>(o + 116, 0);
    store<u16>(o + 120, static_cast<u16>(v120));
    store<u16>(o + 122, 0);
    store<u32>(o + 0, 2);
    u32 hv = v120;  // the first value comes from the register, the rest re-read
    for (u32 k = 0; k < 5; ++k) {
        const s32 v = static_cast<s32>(((hv >> 4) & 0xFu) - k);
        Sf(o + 88 + 4 * k, S(static_cast<f64>(static_cast<s64>(v))));
        hv = load<u16>(o + 120);
    }
    const u32 p = SArg(ctx, 412), q = SArg(ctx, 420);
    Cw(o + 128, p);
    u32 fl = load<u32>(o + 252);
    Cw(o + 132, p + 4);
    const u32 w0 = load<u32>(o);
    Cw(o + 136, p + 8);
    Cw(o + 144, q);
    Cw(o + 148, q + 4);
    Cw(o + 152, q + 8);
    store<u32>(o + 160, d);
    for (u32 k = 0; k < 6; ++k) Sf(o + 164 + 4 * k, fa[k]);
    for (u32 k = 0; k < 6; ++k) Sf(o + 188 + 4 * k, fa[7 + k]);
    for (u32 k = 0; k < 8; ++k) Cw(o + 212 + 4 * k, sp + 196 + 8 * k);
    store<u32>(o + 244, SArg(ctx, 260));
    static constexpr u32 kOff[11] = {276, 284, 292, 300, 308, 316, 324, 332, 356, 364, 0};
    static constexpr u32 kBit[11] = {0x8000, 0x4000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0};
    fl &= ~(0x8000u | 0x4000u | 0x2000u | 0x1000u | 0x800u | 0x400u | 0x200u | 0x100u | 0x80u | 0x40u | 0x20u | 0x10u | 0x8u);
    for (u32 k = 0; k < 10; ++k)
        if (SArg(ctx, kOff[k]) & 1u) fl |= kBit[k];
    store<u32>(o + 252, fl);
    store<u32>(o, w0 | 0x20u);
    const u32 tb = SArg(ctx, 268);
    for (u32 k = 0; k < 6; ++k) store<u8>(o + 248 + k, static_cast<u8>(tb != 0 ? load<u8>(tb + k) : 1));
}

namespace {
// The 3x4 copy of the matrix at m (rows 16 bytes apart, the fourth word of
// each row skipped) to d, as the original's lfs / stfs pairs.
inline void Copy34(u32 d, u32 m) {
    for (u32 r = 0; r < 64; r += 16)
        for (u32 k = 0; k < 12; k += 4) Cw(d + r + k, m + r + k);
}
}  // namespace

// u32 Rig_Reset39BFC4(Rig* o)
// (called by 003A1BFC) Two 96-byte joints at o+0 and o+96 get the global
// identity matrix (0x01164BD0) in their 3x4, +64..+80 = 0, +84 = 1.0f, +88
// = 0, +92 &= 0x0FFFFFFF; o+192 the same matrix, o+256 the global vec3 at
// 0x01164B30; +272, +280..+288 = 0, +292 = 1.0f, +296 = (byte
// 0x0106FD68 != 0), +300 = 10.0f, +304, +308 (and u16 +310) = 0; then
// Shape_Reset63778C(o+312), +316..+324 = 0 and +328 = (+328 & 0x07FFFFFF)
// | 0x10000000, which it returns.
DECOMP(0039BFC4, Rig_Reset39BFC4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 I = 0x01164BD0u;
    for (u32 e = o; e < o + 192; e += 96) {
        for (u32 r = 0; r < 48; r += 16)
            for (u32 k = 0; k < 12; k += 4) Cw(e + r + k, I + r + k);
        const u32 f92 = load<u32>(e + 92) & 0x0FFFFFFFu;
        Cw(e + 48, I + 48);
        Cw(e + 52, I + 52);
        store<u32>(e + 64, 0);
        Cw(e + 56, I + 56);
        store<u32>(e + 68, 0);
        store<u32>(e + 72, 0);
        store<u32>(e + 76, 0);
        store<u32>(e + 80, 0);
        store<u32>(e + 84, 0x3F800000u);
        store<u32>(e + 88, 0);
        store<u32>(e + 92, f92);
    }
    Copy34(o + 192, I);
    const u32 v = 0x01164B30u;
    const u32 b = load<u8>(0x0106FD20u + 72) != 0 ? 1u : 0u;
    Cw(o + 256, v);
    Cw(o + 260, v + 4);
    store<u32>(o + 272, 0);
    Cw(o + 264, v + 8);
    store<u32>(o + 280, 0);
    store<u32>(o + 284, 0);
    store<u32>(o + 288, 0);
    store<u32>(o + 292, 0x3F800000u);
    store<u32>(o + 296, b);
    store<u32>(o + 300, 0x41200000u);
    store<u32>(o + 308, 0);
    store<u32>(o + 304, 0);
    store<u16>(o + 310, 0);
    ctx.fpr[1] = Lf(v + 8);  // left in f1 across the call, as the original's copies leave it
    call(ctx, m15fn::Shape_Reset63778C, {o + 312});
    const u32 f = (load<u32>(o + 328) & 0x07FFFFFFu) | 0x10000000u;
    store<u32>(o + 316, 0);
    store<u32>(o + 320, 0);
    store<u32>(o + 324, 0);
    store<u32>(o + 328, f);
    ret(ctx, f);
}

namespace {
// The handle of slot i of the emitter pool at 0x010B0200 {+8 first, +12
// last, +16 count, +28 slots of 272 bytes: +4 next, +20 object, +256
// generation word}: i << 16 | (gen >> 15) & 0xFFFF.
inline u32 PoolHandle15(u32 data, u32 i) {
    return ((i << 16) & 0xFFFF0000u) | ((load<u32>(data + i * 272 + 256) >> 15) & 0xFFFFu);
}
// A handle on a list linked at +120 (u16 +4 index, u16 +6 generation).
inline u32 HandleListFind15(u32 n, u32 h) {
    for (; n != 0; n = load<u32>(n + 120))
        if (load<u16>(n + 4) == (h >> 16) && load<u16>(n + 6) == (h & 0xFFFFu)) return n;
    return 0;
}
}  // namespace

// void Emitter_StopById44CEE8(s32 id, u8 now)
// (called by 003D4C70, 003D7408, Id_Lookup_0044D168) Unless the byte
// 0x010AEC60 is clear or id is -1, walks the emitter pool (handles as
// PoolHandle15, from +8 until the slot +12) and stops (Emitter_Stop(h, 1))
// every emitter whose object has +100 == id: always when `now` is set or
// the object's +88 is 0; otherwise only when its handle is on neither list
// at 0x010AEC48 / 0x010AEC50 or the entry found there has byte +98 set.
// The next handle is taken before the stop. Leftover r3 as the original's.
DECOMP(0044CEE8, Emitter_StopById44CEE8) {
    const s32 id = static_cast<s32>(arg<u32>(ctx, 0));
    const u32 now = arg<u32>(ctx, 1) & 0xFFu;
    const u32 en = load<u8>(0x010AEC60u);
    if (en == 0 || id == -1) {
        ret(ctx, en);
        return;
    }
    const u32 T = 0x010B0200u, L = 0x010ACC40u;
    u32 h = 0;
    if (load<u32>(T + 16) != 0) h = PoolHandle15(load<u32>(T + 28), load<u32>(T + 8));
    ret(ctx, h);
    while ((h & 0xFFFFu) != 0) {
        const u32 idx = h >> 16;
        const u32 data = load<u32>(T + 28);
        const u32 e = data + idx * 272;
        const u32 obj = load<u32>(e + 20);
        u32 next = 0;
        if (static_cast<s32>(idx) != static_cast<s32>(load<u32>(T + 12))) next = PoolHandle15(data, load<u32>(e + 4));
        if (static_cast<s32>(load<u32>(obj + 100)) == id) {
            bool stop = true;
            if (now == 0 && load<u32>(obj + 88) != 0) {
                u32 n = HandleListFind15(load<u32>(L + 8200), h);
                if (n == 0) n = HandleListFind15(load<u32>(L + 8208), h);
                if (n != 0 && load<u8>(n + 98) == 0) stop = false;
            }
            if (stop) call(ctx, m15fn::Emitter_Stop, {h, 1});
        }
        h = next;
        ret(ctx, h & 0xFFFFu);
    }
}

// void Tune_Load106350(Obj* o)
// (no direct callers) Copies the tuning values (the f32 at +72 of each
// 76-byte record from 0x010323D0 on, a few at other offsets, one u32)
// into o+736..+852 and again into o+608..+724, byte +728 = +856, +732 =
// +860, and +496 from 0x01032464 (f1 left as that value). Raw copies in
// the original's store order.
DECOMP(00106350, Tune_Load106350) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 B = 0x01030000u;
    // {source, first destination}; the second copy is 128 bytes lower
    static constexpr u32 kSrc[28] = {9168 + 72, 9320 + 72, 9396 + 72, 9472 + 80, 9552 + 4, 9552 + 8, 9568 + 72,
                                     9644 + 72, 9720 + 72, 9796 + 72, 9872 + 72, 10100 + 72, 10176 + 72, 9948 + 72,
                                     10024 + 72, 10252 + 72, 10328 + 72, 10404 + 72, 10480 + 72, 10556 + 72, 10632 + 72,
                                     10708 + 72, 12184 + 72, 12260 + 72, 12336 + 72, 21448 + 72, 12412 + 72, 12488 + 72};
    static constexpr u32 kDst[28] = {736, 740, 744, 752, 756, 760, 768, 772, 776, 780, 784, 788, 792, 796,
                                     800, 804, 808, 812, 816, 820, 824, 828, 836, 840, 844, 832, 848, 852};
    u32 v[28];
    for (u32 k = 0; k < 28; ++k) {
        v[k] = load<u32>(B + kSrc[k]);
        if (k == 27) store<u32>(o + kDst[0] - 128, v[0]);  // +608 is stored before +852
        store<u32>(o + kDst[k], v[k]);
    }
    static constexpr u32 kOrder[27] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                                       15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 26, 25, 27};
    for (const u32 k : kOrder) store<u32>(o + kDst[k] - 128, v[k]);
    const u32 b = load<u8>(o + 856), w = load<u32>(o + 860);
    store<u8>(o + 728, static_cast<u8>(b));
    store<u32>(o + 732, w);
    Cw(o + 496, B + 9244 + 72);
    ctx.fpr[1] = Lf(B + 9244 + 72);
}

namespace {
// The inlined lookup of `key` in an effect set's map (AVL root at
// *(set+16), nodes {+4 u32 key, +8 value, +12 left (greater), +16 right}),
// recording the path at set+20 (u16 depth +100) and the hit at +104 (the
// last node visited on a miss, 0 for an empty map, which also clears the
// global last value 0x011DE288; a hit stores its value there). Returns the
// value found (re-read through +104) or 0.
inline u32 SetMapFind15(u32 s, u32 key) {
    const u32 glob = 0x011DE288u;
    u32 n = load<u32>(load<u32>(s + 16));
    if (n == 0) {
        store<u32>(s + 104, 0);
        store<u32>(glob, 0);
        return 0;
    }
    store<u16>(s + 100, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) {
            store<u32>(s + 104, n);
            store<u32>(glob, load<u32>(n + 8));
            const u32 h = load<u32>(s + 104);
            return h != 0 ? load<u32>(h + 8) : 0;
        }
        const u32 nx = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
        if (nx == 0) {
            store<u32>(s + 104, n);
            return 0;
        }
        const s32 d = static_cast<s16>(load<u16>(s + 100));
        store<u16>(s + 100, static_cast<u16>(d + 1));
        store<u32>(s + 20 + static_cast<u32>(d) * 4, n);
        n = nx;
    }
}
}  // namespace

// u32* Effect_ResolveSlot2973B8(Obj* o, u32 key, u32 slot)
// (called by Effect_ApplyToTarget_00090FE0) o+40 + 28 * slot = the value
// for key in the default effect set (Set_Find(0)), else in set
// 0x7719A548, else 0 (key 0: 0 without a lookup). Returns that address.
DECOMP(002973B8, Effect_ResolveSlot2973B8) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), slot = arg<u32>(ctx, 2);
    u32 v = 0;
    if (key != 0) {
        const u32 s = static_cast<u32>(call(ctx, m15fn::Set_Find, {0}));
        if (s != 0) v = SetMapFind15(s, key);
        if (v == 0) {
            const u32 s2 = static_cast<u32>(call(ctx, m15fn::Set_Find, {0x7719A548u}));
            if (s2 != 0) v = SetMapFind15(s2, key);
        }
    }
    const u32 a = o + 40 + slot * 28;
    store<u32>(a, v);
    ret(ctx, a);
}

// void Cam_LoadTuning10DA30(CamCtrl* o)
// (called by Obj_SetRef10DD60, CamCtrl_Construct10DE30, 002194DC) With a
// source o->592 whose +200 has 0x400, copies its tuning block (src->216 +
// 240) into o+16..+180 (the two angles +172 / +176 become Math_Sinf of
// their degrees); otherwise o+16..+148 get the defaults (the f32 at +72
// of each tuning record from 0x0103452C on, three of them at other
// offsets). Raw copies in the original's order; r3 / f1 as it leaves them.
DECOMP(0010DA30, Cam_LoadTuning10DA30) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 src = load<u32>(o + 592);
    if (src != 0 && (load<u32>(src + 200) & 0x400u) != 0) {
        const u32 b = load<u32>(src + 216) + 240;
        static constexpr u32 kOff[] = {16,  32,  36,  40,  48,  52,  56,  60,  64,  68,  72,  76,  80,  96,  100, 104,
                                       112, 116, 120, 124, 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168};
        for (const u32 k : kOff) Cw(o + k, b + k);
        const f64 d2r = Lf(0x0010DA28u);  // pi / 180
        Sf(o + 172, callf(ctx, m15fn::Math_Sinf, {}, {S(Lf(b + 172) * d2r)}));
        Sf(o + 176, callf(ctx, m15fn::Math_Sinf, {}, {S(Lf(b + 176) * d2r)}));
        Cw(o + 180, b + 180);
        ctx.fpr[1] = Lf(b + 180);
        return;
    }
    const u32 B = 0x01030000u;
    static constexpr u32 kDst[] = {16, 32, 36, 40, 48, 52, 56, 60, 64, 68, 72, 76, 80,
                                   96, 100, 104, 112, 116, 120, 124, 128, 132, 136, 140, 144, 148};
    static constexpr u32 kSrc[] = {17708 + 72, 17792 + 80, 17872 + 4, 17872 + 8, 17888 + 72, 17964 + 72, 18040 + 72,
                                   18116 + 72, 18192 + 72, 18268 + 72, 18344 + 72, 18420 + 72, 18496 + 72, 18576 + 80,
                                   18656 + 4,  18656 + 8,  18672 + 72, 18748 + 72, 18824 + 72, 18900 + 72, 18976 + 72,
                                   19052 + 72, 19128 + 72, 19204 + 72, 19280 + 72, 19356 + 72};
    for (u32 k = 0; k < 26; ++k) Cw(o + kDst[k], B + kSrc[k]);
    ctx.fpr[1] = Lf(B + 19356 + 72);
    ret(ctx, B + 19204);
}

// void Def_Assign74B228(Def* d, const Def* s)
// (called by 0011BD90) Field-by-field copy of a 480-byte def: the f32s at
// +8..+64, +80..+88, +96..+128, +144..+152, +160..+168, +176..+184,
// +192/+196, +208..+216, +224..+356 (less the gaps the original skips) and
// +368..+476, the bytes +4 / +5 and the u32 +132, in the original's store
// order (raw copies). f1 is left as +476.
DECOMP(0074B228, Def_Assign74B228) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    u32 k;
    for (k = 8; k <= 64; k += 4) Cw(d + k, s + k);
    for (const u32 x : {80u, 84u, 88u}) Cw(d + x, s + x);
    for (k = 96; k <= 128; k += 4) Cw(d + k, s + k);
    for (const u32 x : {144u, 148u, 152u, 160u, 164u, 168u, 176u, 180u, 184u, 192u, 196u, 208u, 212u, 216u}) Cw(d + x, s + x);
    for (k = 224; k <= 272; k += 4) Cw(d + k, s + k);
    store<u8>(d + 4, load<u8>(s + 4));
    Cw(d + 276, s + 276);
    Cw(d + 280, s + 280);
    store<u8>(d + 5, load<u8>(s + 5));
    for (k = 284; k <= 308; k += 4) Cw(d + k, s + k);
    Cw(d + 132, s + 132);
    for (const u32 x : {312u, 316u, 320u, 324u, 336u, 340u, 344u, 352u, 356u, 368u, 372u, 376u, 384u, 388u, 392u}) Cw(d + x, s + x);
    for (k = 400; k <= 476; k += 4) Cw(d + k, s + k);
    ctx.fpr[1] = Lf(s + 476);
}

// Obj* Spawner_Construct330958(Spawner* o, u32 x, const Def* def)
// (called by 00330C88) Obj_Construct_00769BD8(o, x); +2572 = 0, vtable
// 0x00B1E990, +2576 &= 0x3FFFFFFF; then the def's fields are copied in
// (def+0..+91 -> o+2592.. with +8 a byte, +92.. -> +2684.., +120 byte ->
// +2712, ... +304..+316 -> +2896..) in the original's store order, raw.
// Returns o+2896 (f1 left as def+316).
DECOMP(00330958, Spawner_Construct330958) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    call(ctx, m15fn::Obj_Construct_00769BD8, {o, x});
    const u32 fl = load<u32>(o + 2576) & 0x3FFFFFFFu;
    store<u32>(o + 2572, 0);
    store<u32>(o, 0x00B1E990u);
    store<u32>(o + 2576, fl);
    Cw(o + 2592, d);
    Cw(o + 2596, d + 4);
    store<u8>(o + 2600, load<u8>(d + 8));
    Cw(o + 2604, d + 12);
    for (u32 k = 0; k < 76; k += 4) Cw(o + 2608 + k, d + 16 + k);
    for (u32 k = 0; k < 28; k += 4) Cw(o + 2684 + k, d + 92 + k);
    for (const u32 k : {128u, 132u, 136u, 144u, 148u, 152u, 164u}) Cw(o + 2592 + k, d + k);
    store<u8>(o + 2712, load<u8>(d + 120));
    Cw(o + 2760, d + 168);
    Cw(o + 2764, d + 172);
    Cw(o + 2752, d + 160);
    for (const u32 k : {176u, 180u, 184u}) Cw(o + 2592 + k, d + k);
    for (u32 k = 0; k < 24; k += 4) Cw(o + 2784 + k, d + 192 + k);
    for (const u32 k : {224u, 228u, 232u, 240u, 244u, 248u, 256u, 260u, 264u, 268u, 272u}) Cw(o + 2592 + k, d + k);
    Cw(o + 2876, d + 284);
    Cw(o + 2868, d + 276);
    Cw(o + 2872, d + 280);
    Cw(o + 2880, d + 288);
    for (u32 k = 0; k < 16; k += 4) Cw(o + 2896 + k, d + 304 + k);
    ctx.fpr[1] = Lf(d + 316);
    ret(ctx, o + 2896);
}

// void Query_Construct17EAD4(Query* o, u32 a, Def* def, u32 b, u32 c, u32 d,
//                            const f32 p[3], const f32 q[3], f32 f1..f7, [stack] ...)
// (called by 0017ED58) QueryEx_Construct_000DFDF8(o, 5, 32, a, def, b,
// 0x00AC8C70, c, stack: d, 0, 0, b != 0, (u8) s228, (u8) s236, 1, 0,
// (u8) s244, s252, s260), then +200 = s220, +208 = p, +224 = q, +240..+268
// the seven floats with +264 = s164 in place of the sixth's neighbour,
// +272 byte s180, +276 s188, +280 s196, +284 byte s204, +288 = s212 (the
// caller's stack words at those offsets), +4 |= 0x100. An id of 0 or -1
// is replaced by ++(the game's +0x1281D4 ->4 ->116 ->48).
DECOMP(0017EAD4, Query_Construct17EAD4) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4], def = ctx.gpr[5], b = ctx.gpr[6], c = ctx.gpr[7], dd = ctx.gpr[8];
    const u32 p = arg<u32>(ctx, 6), q = arg<u32>(ctx, 7);
    const f64 fv[7] = {ctx.fpr[1], ctx.fpr[2], ctx.fpr[3], ctx.fpr[4], ctx.fpr[5], ctx.fpr[6], ctx.fpr[7]};
    const u32 s228 = SArg(ctx, 228) & 0xFFu, s236 = SArg(ctx, 236) & 0xFFu, s244 = SArg(ctx, 244) & 0xFFu;
    const u32 s180 = SArg(ctx, 180), s204 = SArg(ctx, 204), s164 = SArg(ctx, 164), s188 = SArg(ctx, 188);
    const u32 s196 = SArg(ctx, 196), s252 = SArg(ctx, 252), id = SArg(ctx, 212), s260 = SArg(ctx, 260);
    const u32 s220 = SArg(ctx, 220);
    const u32 nb = static_cast<u32>(b) != 0 ? 1u : 0u;
    call(ctx, m15fn::QueryEx_Construct_000DFDF8,
         {o, 5, 32, a, def, b, 0x00AC8C70u, c, dd, 0, 0, nb, s228, s236, 1, 0, s244, s252, s260});
    Cw(o + 208, p);
    Cw(o + 212, p + 4);
    store<u32>(o + 200, s220);
    Cw(o + 216, p + 8);
    Cw(o + 224, q);
    Cw(o + 228, q + 4);
    Sf(o + 240, fv[0]);
    Cw(o + 232, q + 8);
    for (u32 k = 1; k < 5; ++k) Sf(o + 240 + 4 * k, fv[k]);
    const u32 fl = load<u32>(o + 4) | 0x100u;
    Sf(o + 260, fv[5]);
    store<u32>(o + 264, s164);
    Sf(o + 268, fv[6]);
    store<u8>(o + 272, static_cast<u8>(s180));
    store<u32>(o + 276, s188);
    store<u32>(o + 280, s196);
    store<u8>(o + 284, static_cast<u8>(s204));
    store<u32>(o + 288, id);
    store<u32>(o + 4, fl);
    ctx.fpr[1] = Lf(q + 8);
    ret(ctx, fl);
    if (id == 0 || id == 0xFFFFFFFFu) {
        const u32 t = load<u32>(load<u32>(load<u32>(load<u32>(mid14::kGlobF65CA0) + 0x001281D4u) + 4) + 116);
        const u32 n = load<u32>(t + 48) + 1;
        store<u32>(t + 48, n);
        store<u32>(o + 288, n);
        ret(ctx, t);
    }
}
