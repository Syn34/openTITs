// Game code, medium sweep part 10: leaf helpers (geometry tests, copies).
#include "mid10.h"

using namespace sly4;
using namespace sly4::mid10;

namespace {

// fmsubs: a * c - b rounded once to single.
inline f32 Fmsubs(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), -static_cast<f64>(b))); }

}  // namespace

// bool Poly_IsEar_004A6F40(Poly* p, int i, int j, int k, int n, const s32* idx)
// The ear test of Poly_Triangulate (004A71D0) on the x / z plane of the
// 16-byte points p->0[idx[]]: 0 when the corner (i, j, k) is reflex (its
// cross product below 1e-6, 0x004A6F34) or any other of the n points lies
// inside the triangle (all three edge tests >= 0, 0x004A6F38); else 1.
DECOMP(004A6F40, Poly_IsEar_004A6F40) {
    const u32 pts = load<u32>(arg<u32>(ctx, 0));
    const s32 i = arg<s32>(ctx, 1), j = arg<s32>(ctx, 2), k = arg<s32>(ctx, 3), n = arg<s32>(ctx, 4);
    const u32 idx = arg<u32>(ctx, 5);
    auto pt = [&](s32 m) { return pts + (load<u32>(idx + static_cast<u32>(m) * 4u) << 4); };
    const u32 a = pt(i), b = pt(j), c = pt(k);
    const f32 ax = F32(load<u32>(a)), az = F32(load<u32>(a + 8));
    const f32 bx = F32(load<u32>(b)), bz = F32(load<u32>(b + 8));
    const f32 cx = F32(load<u32>(c)), cz = F32(load<u32>(c + 8));
    const f32 dbz = bz - az, dbx = bx - ax;
    const f32 cross = Fmsubs(dbx, cz - az, dbz * (cx - ax));
    ctx.fpr[1] = cross;  // f1 as the original leaves it
    if (cross < F32(load<u32>(0x004A6F34))) {
        ret(ctx, 0u);
        return;
    }
    if (n <= 0) {
        ret(ctx, 1u);
        return;
    }
    const f32 ex = cx - bx, ez = cz - bz, gx = ax - cx, gz = az - cz;
    const f32 zero = F32(load<u32>(0x004A6F38));
    ctx.fpr[1] = zero;
    for (s32 m = 0; m < n; ++m) {
        if (m == i || m == k || m == j) continue;
        const u32 p = pts + (load<u32>(idx + static_cast<u32>(m) * 4u) << 4);
        const f32 px = F32(load<u32>(p)), pz = F32(load<u32>(p + 8));
        const f32 t1 = Fmsubs(ex, pz - bz, ez * (px - bx));
        const f32 t2 = Fmsubs(pz - az, dbx, (px - ax) * dbz);
        const f32 t3 = Fmsubs(gx, pz - cz, gz * (px - cx));
        if (t1 >= zero && t3 >= zero && t2 >= zero) {
            ret(ctx, 0u);
            return;
        }
    }
    ret(ctx, 1u);
}

// void Obj_CopyLinkedXform_0001DE74(Obj* o)  (called through a pointer)
// When o's per-slot enable byte (o->308->12)[4 * (*(0x00F94884))->64] is
// set, asks o->48 (vtable slot 144) for the linked object and, when that
// has +8 set, copies its 4x3 transform rows (+240, 16-byte stride, xyz) to
// o+128.. (through a stack copy, as the original). The original returns
// with r3 = its frame + 164 after a copy (kept) and f1 the last word moved.
DECOMP(0001DE74, Obj_CopyLinkedXform_0001DE74) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 slot = load<u32>(load<u32>(0x00F94874 + 16) + 64);
    const u32 en = load<u8>(load<u32>(load<u32>(o + 308) + 12) + (slot << 2));
    if (en == 0) {
        ret(ctx, en);
        return;
    }
    const u32 link = static_cast<u32>(VCall(ctx, load<u32>(o + 48), 144, {load<u32>(o + 48)}));
    if (link == 0 || load<u32>(link + 8) == 0) {
        ret(ctx, link);
        return;
    }
    u32 m[12];
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) m[3 * r + c] = load<u32>(link + 240 + 16 * r + 4 * c);
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) store<u32>(o + 128 + 16 * r + 4 * c, m[3 * r + c]);
    ctx.gpr[3] = static_cast<u32>(ctx.gpr[1]) - 192 + 164;  // as the original leaves r3
    ctx.fpr[1] = static_cast<f64>(F32(m[11]));
}

namespace {

inline f32 Fmadds(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); }

// |e+144 - t|^2 as the original rounds it (y first).
inline f32 Dist2(u32 e, f32 tx, f32 ty, f32 tz) {
    const f32 dy = F32(load<u32>(e + 144 + 4)) - ty, dx = F32(load<u32>(e + 144)) - tx, dz = F32(load<u32>(e + 144 + 8)) - tz;
    return Fmadds(dz, dz, Fmadds(dx, dx, dy * dy));
}

}  // namespace

// Obj* Obj_PickNearest_0001AC80(Picker* o, Obj* t)
// 0 unless o->40 <= 0x0001AC78. With o->20 > 0: the nearest (to t+176) of
// the o->20 candidates o->3700[] closer than 0x0001AC7C, 0 when that one
// is null or lacks flag 2 in +192 (the distance bound still tightens). Else
// with o->16 == 1: o->3664[0]. Else the current player (*(0x00F65CA0) +
// 4208)->8 when it has flag 2, replaced by the nearer of o->3664[o->16].
DECOMP(0001AC80, Obj_PickNearest_0001AC80) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1) + 176;
    f64 f1 = LoadF(o + 40);
    const s32 n2 = load<s32>(o + 20), n1 = load<s32>(o + 16);
    u32 best = 0;
    if (!(F32(load<u32>(o + 40)) <= F32(load<u32>(0x0001AC78)))) {
        ctx.fpr[1] = f1;
        ret(ctx, 0u);
        return;
    }
    const f32 tx = F32(load<u32>(t)), ty = F32(load<u32>(t + 4)), tz = F32(load<u32>(t + 8));
    if (n2 > 0) {
        f32 lim = F32(load<u32>(0x0001AC7C));
        for (s32 i = 0; i < n2; ++i) {
            const u32 e = load<u32>(o + 3700 + 4 * static_cast<u32>(i));
            const f32 d = Dist2(e, tx, ty, tz);  // read even when e is null, as the original
            f1 = d;
            if (d < lim) {
                best = (e != 0 && (load<u32>(e + 192) & 2u)) ? e : 0;
                lim = d;
            }
        }
    } else if (n1 == 1) {
        best = load<u32>(o + 3664);
    } else {
        const u32 p = load<u32>(load<u32>(0x00F65CA0) + 4208 + 8);
        if (p != 0 && (load<u32>(p + 192) & 2u)) best = p;
        if (best != 0) {
            const f32 dy = F32(load<u32>(best + 148)) - ty;
            f1 = static_cast<f32>(dy * dy);
            f32 lim = Dist2(best, tx, ty, tz);
            for (s32 i = 0; i < n1; ++i) {
                const u32 e = load<u32>(o + 3664 + 4 * static_cast<u32>(i));
                const f32 d = Dist2(e, tx, ty, tz);
                f1 = d;
                if (d < lim) {
                    best = e;
                    lim = d;
                }
            }
        }
    }
    ctx.fpr[1] = f1;
    ret(ctx, best);
}

namespace {

// The u64 value (low word) of the first {+0 key, +8 u64} entry of the
// 16-byte array at e->124 (count e->128) with the key; found = false when
// none.
inline u32 PropFind(u32 e, u32 key, bool& found) {
    const s32 n = load<s32>(e + 128);
    for (s32 i = 0; i < n; ++i) {
        const u32 a = load<u32>(e + 124) + 16 * static_cast<u32>(i);
        if (load<u32>(a) == key) {
            found = true;
            return load<u32>(a + 12);
        }
    }
    found = false;
    return 0;
}

}  // namespace

// void Obj_OnPairTouch_000C21EC(Obj* o, Pair* p)  (via AdjThunk_000C23D4)
// Marks p->24 (byte +15 |= 8). When o->432 is one of p->4 / p->8, takes the
// other one: if its property 0xA734004D is 0x7A30D7E3, its property
// 0x91C7CF5B is a record with bit 0 of +188 set and (o->216)->262 is set,
// calls vtable slot 32 (o, 0) and on a 0 / 1 result slot 28 (o, 0, 2).
// r3 is left as the original leaves it on each path.
DECOMP(000C21EC, Obj_OnPairTouch_000C21EC) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 m = load<u32>(p + 24);
    store<u8>(m + 15, static_cast<u8>(load<u8>(m + 15) | 8));
    const u32 x = load<u32>(o + 432), d = load<u32>(o + 216);
    if (x == 0) {
        ret(ctx, d);
        return;
    }
    const u32 a = load<u32>(p + 4), b = load<u32>(p + 8);
    const u32 other = a == x ? b : (b == x ? a : 0);
    u32 rec = 0;
    if (other != 0) {
        bool f = false;
        const u32 kind = PropFind(other, 0xA734004Du, f);
        if (f && kind == 0x7A30D7E3u) rec = PropFind(other, 0x91C7CF5Bu, f);
    }
    const u32 en = load<u8>(d + 262);
    if (en == 0 || rec == 0) {
        ret(ctx, en);
        return;
    }
    if ((load<u32>(rec + 188) & 1u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
    if (r != 0 && r != 1) {
        ret(ctx, load<u32>(o));
        return;
    }
    VCall(ctx, o, 28, {o, 0, 2});
}

// void Cam_ExportView_000FD408(CamState* s, r4, View* d)  (called through a pointer)
// Copies s's view into d: +64 = s->80 when >= 0x000FD3FC, +80 / +76 / +68 =
// s->92 / 96 / 100, byte +72 = the sign bit of s->88, +84 = (s->88 >> 23) &
// 0xFF, and the 4x3 matrix +0.. = s+16.. (rows of 16 bytes). With a look-at
// target s->84 (vtable slot 156 fills a vec3), the rows are rebuilt instead:
// forward = normalized (target - pos) (its length stored to +68), right =
// normalize(up x forward) with up the vec3 at 0x01164B60, up' = forward x
// right; 1.0 is the f32 at 0x000FD400.
DECOMP(000FD408, Cam_ExportView_000FD408) {
    const u32 s = arg<u32>(ctx, 0), d = arg<u32>(ctx, 2);
    const f32 f80 = F32(load<u32>(s + 80));
    if (f80 >= F32(load<u32>(0x000FD3FC))) CopyW(d + 64, s + 80);
    CopyW(d + 80, s + 92);
    CopyW(d + 76, s + 96);
    store<u8>(d + 72, static_cast<u8>(load<u32>(s + 88) >> 31));
    CopyW(d + 68, s + 100);
    store<u32>(d + 84, (load<u32>(s + 88) >> 23) & 0xFFu);
    u32 m[12];
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) m[3 * r + c] = load<u32>(s + 16 + 16 * r + 4 * c);
    const u32 cam = load<u32>(s + 84);
    if (cam == 0) {
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 3; ++c) store<u32>(d + 16 * r + 4 * c, m[3 * r + c]);
        ctx.gpr[3] = 0;
        ctx.fpr[1] = static_cast<f64>(F32(m[0]));
        return;
    }
    Stack frame(ctx, 16);
    const u32 out = frame.addr();
    VCall(ctx, cam, 156, {out, cam, 0});
    const f32 px = F32(m[9]), py = F32(m[10]), pz = F32(m[11]);
    f32 fy = F32(load<u32>(out + 4)) - py, fx = F32(load<u32>(out)) - px, fz = F32(load<u32>(out + 8)) - pz;
    const f32 one = F32(load<u32>(0x000FD400));
    const f32 ux = F32(load<u32>(0x01164B60)), uz = F32(load<u32>(0x01164B68)), uy = F32(load<u32>(0x01164B64));
    const f32 len = std::sqrt(Fmadds(fz, fz, Fmadds(fx, fx, fy * fy)));
    const f32 inv = one / len;
    store<u32>(d + 68, W32(len));
    fz = fz * inv;
    fy = fy * inv;
    fx = fx * inv;
    const f32 a = ux * fz, b = uz * fy, c = uy * fx;
    f32 ry = Fmsubs(uz, fx, a);
    f32 rx = Fmsubs(uy, fz, b);
    f32 rz = Fmsubs(ux, fy, c);
    const f32 rinv = one / std::sqrt(Fmadds(rz, rz, Fmadds(rx, rx, ry * ry)));
    rx = rx * rinv;
    ry = ry * rinv;
    rz = rz * rinv;
    const f32 vz = Fmsubs(fx, ry, fy * rx);
    const f32 vx = Fmsubs(fy, rz, fz * ry);
    const f32 vy = Fmsubs(fz, rx, fx * rz);
    const f32 rows[9] = {rx, ry, rz, vx, vy, vz, fx, fy, fz};
    for (u32 r = 0; r < 3; ++r)
        for (u32 c = 0; c < 3; ++c) store<u32>(d + 16 * r + 4 * c, W32(rows[3 * r + c]));
    for (u32 c = 0; c < 3; ++c) store<u32>(d + 48 + 4 * c, m[9 + c]);
    ctx.gpr[3] = 0x01160000;  // as the original leaves r3
    ctx.fpr[1] = static_cast<f64>(rx);
}

// u32 Obj_ApplyTrackKey_000B6A58(Player* o)
// When vtable slot 32 (o, 0) answers 2: takes key o->576 (20-byte records)
// of track o->724 (20-byte entries at o->216 + 256: +0 keys, u16 +8 count),
// calls slot 28 (o, 1, the byte of 0x00F6471C[kind]) and writes the key's
// value to o->20 + u16 o->6 by kind (0: three words and a float, 1: two
// floats, 2: two words, 3 / 4: one float), then advances o->576 (wrapping
// to 0 at the count). The slot 28 call leaves the table base, kind * 4 and
// the entry in r6..r8 (its OPD is not in a register).
DECOMP(000B6A58, Obj_ApplyTrackKey_000B6A58) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 st = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
    if (st != 2) {
        ret(ctx, st);
        return;
    }
    constexpr u32 kKindByte = 0x00F6471C;
    const u32 trk = load<u32>(o + 216) + load<u32>(o + 724) * 20u + 256;
    const u32 key = load<u32>(trk) + load<u32>(o + 576) * 20u;
    const u32 opd = load<u32>(load<u32>(o) + 28);
    const u32 k4 = load<u32>(key) << 2;
    call_ptr(ctx, opd, {o, 1, load<u32>(kKindByte + k4) & 0xFFu, kKindByte, k4, load<u32>(opd)});
    const s32 kind = load<s32>(key);
    if (kind >= 0 && kind <= 4) {
        const u32 dst = load<u32>(o + 20) + load<u16>(o + 6);
        static constexpr u32 kWords[5] = {4, 2, 2, 1, 1};
        for (u32 k = 0; k < kWords[kind]; ++k) CopyW(dst + 4 * k, key + 4 + 4 * k);
        if (kind != 2) ctx.fpr[1] = LoadF(key + 4 * kWords[kind]);  // the last lfs
    }
    const u32 n = load<u32>(o + 576) + 1;
    store<u32>(o + 576, n);
    if (n < load<u16>(trk + 8)) {
        ret(ctx, n);
        return;
    }
    store<u32>(o + 576, 0);
    ret(ctx, 0u);
}

// s16 Obj_EvalChannels_0038B3A8(Chan* o, Ctx* c)
// Evaluates o's channels with the per-type function tables (OPD at +4):
// u16 +24 = 0x00F6503C[o->18](o->18, c) (28-byte entries); then, when byte
// +16 is 0, u16 +34 = 0x00F65100[o->28](o->28, c) (44-byte entries); else
// for each of the s16 o->28 sub-channels j = o->46 + 1 + i of the list
// (c->0 - 152) (132-byte records from +152, u16 count +144, used when the
// record's +0 < 10): its +10 = 0x00F65100[+4](+4, c). A sub-channel past the
// count reads from address 4 and writes address 10, as the original.
DECOMP(0038B3A8, Obj_EvalChannels_0038B3A8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    auto eval = [&](u32 tab, u32 stride, u32 idxp) {
        const s32 i = static_cast<s16>(load<u16>(idxp));
        const u32 opd = load<u32>(tab + static_cast<u32>(i) * stride + 4);
        return call_ptr(ctx, opd, {static_cast<u64>(static_cast<s64>(i)), c, opd, load<u32>(opd)});
    };
    store<u16>(o + 24, static_cast<u16>(eval(0x00F6503C, 28, o + 18)));
    if (load<u8>(o + 16) == 0) {
        const u64 r = eval(0x00F65100, 44, o + 28);
        store<u16>(o + 34, static_cast<u16>(r));
        ctx.gpr[3] = r;
        return;
    }
    const u32 list = load<u32>(c) - 152;
    s32 cnt = static_cast<s16>(load<u16>(o + 28));
    for (u32 i = 0; static_cast<s32>(i) < cnt; i = (i + 1) & 0xFFFFu) {
        const s32 j = static_cast<s16>(static_cast<u16>(load<u16>(o + 46) + i + 1));
        u32 rec = 0;
        if (j < static_cast<s32>(load<u16>(list + 144))) {
            rec = list + 152 + static_cast<u32>(j) * 132u;
            if (load<s32>(rec) >= 10) rec = 0;
        }
        const u32 sub = rec + 4;  // 4 when rec is 0 (quirk)
        store<u16>(sub + 6, static_cast<u16>(eval(0x00F65100, 44, sub)));
        cnt = static_cast<s16>(load<u16>(o + 28));
    }
    ret(ctx, cnt);
}

// void Obj_ShowPart_002F9AC0(Obj* o, bool on, u32 key)
// For each of o's three parts whose key o->900[k] is key: the part object
// o->952[k] gets vtable slot 88 (part, on, 1, 1, 1) and bit 0x80 of byte
// +444 = on; when on, its 4x3 matrix +352.. is set to the one at 0x01164BD0
// with the translation replaced by o->1072[k]+128 plus the vec3 at
// 0x0106AC80 when o->1072[k] is set. r3 / f1 are left as the original's.
DECOMP(002F9AC0, Obj_ShowPart_002F9AC0) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const u32 b = on & 0xFFu;
    constexpr u32 kMat = 0x01164BD0, kOfs = 0x0106AC80;
    u64 r3 = 0;
    for (u32 k = 0; k < 3; ++k) {
        r3 = load<u32>(o + 900 + 4 * k);
        if (static_cast<s32>(r3) != static_cast<s32>(key)) continue;
        const u32 slot = o + 952 + 4 * k;
        VCall(ctx, load<u32>(slot), 88, {load<u32>(slot), b, 1, 1, 1});
        u32 p = load<u32>(slot);
        store<u8>(p + 444, static_cast<u8>((load<u8>(p + 444) & 0x7Fu) | ((on & 1u) << 7)));
        r3 = p;
        if (b == 0) continue;
        u32 m[12];
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 3; ++c) m[3 * r + c] = load<u32>(kMat + 16 * r + 4 * c);
        const u32 t = load<u32>(load<u32>(o + 1072) + 4 * k);
        if (t != 0)
            for (u32 c = 0; c < 3; ++c) m[9 + c] = W32(F32(load<u32>(t + 128 + 4 * c)) + F32(load<u32>(kOfs + 4 * c)));
        p = load<u32>(slot);
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 3; ++c) store<u32>(p + 352 + 16 * r + 4 * c, m[3 * r + c]);
        r3 = p;
        ctx.fpr[1] = static_cast<f64>(F32(m[11]));
    }
    ctx.gpr[3] = r3;
}

// void Quat_FromTo_0074D4B8(Quat* q, const Vec3* a, const Vec3* b)
// The shortest-arc rotation taking a to b (both normalized first): xyz =
// a' x b', w = (a' + b') . a', then normalized. When |w| <= the f32 at
// 0x0074D4AC (opposite vectors) it rotates half a turn about an axis
// perpendicular to a (in the x / z or y / z plane, whichever of a'.x /
// a'.y is larger), or the identity when that axis is shorter than the f32
// at 0x0074D4B0. 1.0 is the f32 at 0x0074D4A8. Leaves f1 as the original,
// and f5 / f6 too: the slerp at 00282638 reads both after the call (f5 = w
// and f6 = the length of the unnormalized quat, or f5 = the f32 at
// 0x0074D4B0 and f6 = b'.z for opposite vectors).
DECOMP(0074D4B8, Quat_FromTo_0074D4B8) {
    const u32 q = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    constexpr u32 kLit = 0x0074D4A8;
    const f32 ay = F32(load<u32>(a + 4)), by = F32(load<u32>(b + 4)), ax = F32(load<u32>(a)), bx = F32(load<u32>(b));
    const f32 az = F32(load<u32>(a + 8)), bz = F32(load<u32>(b + 8));
    const f32 la = std::sqrt(Fmadds(az, az, Fmadds(ax, ax, ay * ay)));
    const f32 one = F32(load<u32>(kLit)), eps = F32(load<u32>(kLit + 4));
    const f32 lb = std::sqrt(Fmadds(bz, bz, Fmadds(bx, bx, by * by)));
    const f32 ia = one / la, ib = one / lb;
    const f32 y1 = ia * ay, z1 = ia * az, x1 = ia * ax;
    const f32 y2 = ib * by, x2 = ib * bx, z2 = ib * bz;
    const f32 w = Fmadds(z1 + z2, z1, Fmadds(x1 + x2, x1, (y1 + y2) * y1));
    if (std::fabs(w) > eps) {
        const f32 qx = Fmsubs(y1, z2, z1 * y2), qy = Fmsubs(z1, x2, x1 * z2), qz = Fmsubs(x1, y2, y1 * x2);
        store<u32>(q + 12, W32(w));
        store<u32>(q, W32(qx));
        store<u32>(q + 4, W32(qy));
        store<u32>(q + 8, W32(qz));
        const f32 q1 = F32(load<u32>(q + 4)), q2 = F32(load<u32>(q + 8)), q3 = F32(load<u32>(q + 12)), q0 = F32(load<u32>(q));
        const f32 qlen = std::sqrt(Fmadds(q3, q3, Fmadds(q2, q2, Fmadds(q0, q0, q1 * q1))));
        const f32 inv = one / qlen;
        const f32 fw = w * inv;
        ctx.fpr[5] = w;
        ctx.fpr[6] = qlen;
        store<u32>(q, W32(qx * inv));
        store<u32>(q + 4, W32(qy * inv));
        store<u32>(q + 8, W32(qz * inv));
        store<u32>(q + 12, W32(fw));
        ctx.fpr[1] = fw;
        return;
    }
    const f32 tiny = F32(load<u32>(kLit + 8));
    ctx.fpr[5] = tiny;
    ctx.fpr[6] = z2;
    const bool xz = x1 > y1;
    const f32 len = std::sqrt(Fmadds(xz ? x1 : y1, xz ? x1 : y1, z1 * z1));
    if (len < tiny) {
        store<u32>(q, 0);
        store<u32>(q + 4, 0);
        store<u32>(q + 8, 0);
        store<u32>(q + 12, 0x3F800000u);
        ctx.fpr[1] = one;
        return;
    }
    const f32 inv = one / len;
    if (xz) {
        store<u32>(q + 4, 0);
        store<u32>(q + 12, 0);
        const f32 nz = -(inv * z1), fx = inv * x1;
        store<u32>(q + 8, W32(fx));
        store<u32>(q, W32(nz));
        ctx.fpr[1] = fx;
    } else {
        store<u32>(q, 0);
        store<u32>(q + 12, 0);
        const f32 ny = -(inv * y1), fz = inv * z1;
        store<u32>(q + 4, W32(fz));
        store<u32>(q + 8, W32(ny));
        ctx.fpr[1] = fz;
    }
}

namespace {

// The member-wise copy shared by the 0x00B19540 record's copy constructor
// and assignment: words +4..+16, the five vec3s +32 / +48 / +64 / +80 /
// +96, +112..+120, f32 +124 / +128, bits 0 / 1 / 2 / 3..10 of +132 one store
// each (the rest kept), +136, bytes +140..+179, +180, bits 0x80..0x02 of
// byte +184 one store each (0x01 kept); vt0 / vt1 (when non-zero) stored to
// +0 first / before +180, as the copy constructor does. Leaves f1 = s->128.
void AnimRecCopy(PPUContext& ctx, u32 d, u32 s, u32 vt0, u32 vt1) {
    if (vt0 != 0) store<u32>(d, vt0);
    for (u32 k = 4; k <= 16; k += 4) store<u32>(d + k, load<u32>(s + k));
    for (u32 v = 32; v <= 96; v += 16)
        for (u32 c = 0; c < 12; c += 4) CopyW(d + v + c, s + v + c);
    for (u32 k = 112; k <= 120; k += 4) store<u32>(d + k, load<u32>(s + k));
    u32 w = load<u32>(d + 132) & 0x7FFFFFFFu;
    CopyW(d + 124, s + 124);
    CopyW(d + 128, s + 128);
    w |= load<u32>(s + 132) & 0x80000000u;
    store<u32>(d + 132, w);
    w = (w & ~0x40000000u) | (load<u32>(s + 132) & 0x40000000u);
    store<u32>(d + 132, w);
    w = (w & ~0x20000000u) | (load<u32>(s + 132) & 0x20000000u);
    store<u32>(d + 132, w);
    w = (w & ~0x1FE00000u) | (load<u32>(s + 132) & 0x1FE00000u);
    store<u32>(d + 132, w);
    store<u32>(d + 136, load<u32>(s + 136));
    for (u32 k = 0; k < 40; ++k) store<u8>(d + 140 + k, load<u8>(s + 140 + k));
    u32 b = load<u8>(d + 184);
    if (vt1 != 0) store<u32>(d, vt1);
    store<u32>(d + 180, load<u32>(s + 180));
    for (u32 bit = 0x80; bit >= 0x02; bit >>= 1) {
        b = (b & ~bit) | (load<u8>(s + 184) & bit);
        store<u8>(d + 184, static_cast<u8>(b));
    }
    ctx.fpr[1] = LoadF(s + 128);
    ret(ctx, d);
}

}  // namespace

// Rec* AnimRec_CopyCtor_007464A0(Rec* d, const Rec* s)  (called by Obj_SetAnimSet0D1B10)
// The copy constructor: vtable 0x00B12260 (the base), the member-wise copy
// (AnimRecCopy), vtable 0x00B19540.
DECOMP(007464A0, AnimRec_CopyCtor_007464A0) { AnimRecCopy(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), 0x00B12260u, 0x00B19540u); }

// Rec& AnimRec_Assign_007466D8(Rec* d, const Rec* s)  (called by Obj_SetAnimSet0D1B10)
// The assignment: the member-wise copy (AnimRecCopy) without the vtables.
DECOMP(007466D8, AnimRec_Assign_007466D8) { AnimRecCopy(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), 0, 0); }

// void NodeList_Remove_003ED8D0(NodeList* l, Node* key)  (called by 003EFE40 / 003EFFA0)
// key->220 = 0; finds the record r (l->0[i], u16 count +8) whose +0 is key
// and, when found, detaches it from its eight neighbours r->232..260 (each
// neighbour's back link cleared: 232 / 236, 240 / 244, 248 / 260, 252 / 256
// pair up; its f32 +224 += the f32 at 0x003ED8C8), shifts the later
// entries down, decrements the count and parks r in the freed last slot.
// The original then searches the list again without effect (left out).
DECOMP(003ED8D0, NodeList_Remove_003ED8D0) {
    const u32 l = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    store<u32>(key + 220, 0);
    const u32 n = load<u16>(l + 8);
    u32 i = 0, r = 0;
    for (; i < n; ++i) {
        r = load<u32>(load<u32>(l) + 4 * i);
        if (load<u32>(r) == key) break;
    }
    if (n == 0 || i >= n) return;
    const f32 k = F32(load<u32>(0x003ED8C8));
    ctx.fpr[1] = k;
    static constexpr u32 kLink[8][2] = {{232, 236}, {236, 232}, {240, 244}, {244, 240}, {248, 260}, {252, 256}, {256, 252}, {260, 248}};
    for (const auto& lk : kLink) {
        const u32 nb = load<u32>(r + lk[0]);
        if (nb == 0) continue;
        const f32 v = F32(load<u32>(nb + 224)) + k;
        store<u32>(load<u32>(r + lk[0]) + lk[1], 0);
        store<u32>(nb + 224, W32(v));
        if (lk[0] == 260) ctx.fpr[1] = v;
    }
    for (; i < static_cast<u32>(load<u16>(l + 8)) - 1u; ++i) store<u32>(load<u32>(l) + 4 * i, load<u32>(load<u32>(l) + 4 * i + 4));
    const u32 last = load<u16>(l + 8) - 1u;  // >= 0: the count is at least 1 here
    store<u16>(l + 8, static_cast<u16>(last));
    store<u32>(load<u32>(l) + 4 * last, r);
}

// u32 Obj_FollowXform_000F2D88(Obj* o, Obj* t)  (called through a pointer)
// o's 4x3 matrix +448: with a target t, t's rotation rows (+96, 16-byte
// stride) and the point t's vtable slot 284 (out, t) fills (that call has
// the entry in r5 and the OPD in r6); else the identity at 0x01164BD0 and
// o+128 * k + o+144 (k the f32 at 0x000F2D80). Then o->440, when set, gets
// vtable slot 84 (o->440, o+448).
DECOMP(000F2D88, Obj_FollowXform_000F2D88) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    u32 m[12];
    if (t != 0) {
        for (u32 r = 0; r < 3; ++r)
            for (u32 c = 0; c < 3; ++c) m[3 * r + c] = load<u32>(t + 96 + 16 * r + 4 * c);
        Stack frame(ctx, 16);
        const u32 out = frame.addr();
        const u32 opd = load<u32>(load<u32>(t) + 284);
        call_ptr(ctx, opd, {out, t, load<u32>(opd), opd});
        for (u32 c = 0; c < 3; ++c) m[9 + c] = load<u32>(out + 4 * c);
        ctx.fpr[1] = static_cast<f64>(F32(m[9]));
    } else {
        constexpr u32 kIdent = 0x01164BD0;
        const f32 k = F32(load<u32>(0x000F2D80));
        for (u32 r = 0; r < 3; ++r)
            for (u32 c = 0; c < 3; ++c) m[3 * r + c] = load<u32>(kIdent + 16 * r + 4 * c);
        for (u32 c = 0; c < 3; ++c) m[9 + c] = W32(Fmadds(F32(load<u32>(o + 128 + 4 * c)), k, F32(load<u32>(o + 144 + 4 * c))));
        ctx.fpr[1] = static_cast<f64>(F32(m[9]));
        ctx.gpr[3] = o + 128;  // as the original leaves r3 (else the slot 284 result)
    }
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) store<u32>(o + 448 + 16 * r + 4 * c, m[3 * r + c]);
    const u32 p = load<u32>(o + 440);
    if (p != 0) VCall(ctx, p, 84, {p, o + 448});
}
