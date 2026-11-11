// Game code, part 45 (continued): the 98 instruction functions and up.
// Each was checked against the original in the PPC simulator (memory
// writes, calls and their arguments, r3 and f1).
#include "obj45.h"

using namespace sly4;
using namespace sly4::obj45;

// ---------------------------------------------------------------- sweep: 98 instructions
// bool f(Nav** nav, const Vec3* p, -, -, -, Vec3* out, f32 r, f32 lo, f32
// hi): the point q = p, or (when *nav is set) its projection on the nav
// mesh (NavQuery_Init630F94(&qy, &p', 0, 0x011AD1AC),
// NavQuery_Point62D25C(&q, &qy, &p', 0), NavQuery_Finish62E094(&qy),
// NavQuery_Free62D08C(&qy)); r3 = 0 unless lo <= q.y - p.y <= hi, else out
// (when set) = q and r3 = (dx^2 + dz^2 + k^2 < r^2) with k = *0x0001097C.
// f1 = that sum (p.z on the early exit).
DECOMP(00010980, Nav_ProjectInRange_00010980) {
    const u32 nav = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), out = arg<u32>(ctx, 5);
    const f64 r = farg(ctx, 0), lo = farg(ctx, 1), hi = farg(ctx, 2);
    u32 qx = load<u32>(p), qy = load<u32>(p + 4), qz = load<u32>(p + 8);
    if (load<u32>(nav) != 0) {
        Stack frame(ctx, 32);
        const u32 s = frame.addr();
        store<u32>(s + 8, qx);
        store<u32>(s + 12, qy);
        store<u32>(s + 16, qz);
        call(ctx, 0x00630F94u, {s, s + 8, 0, 0x011AD1ACu});
        call(ctx, 0x0062D25Cu, {s + 20, s, s + 8, 0});
        qx = load<u32>(s + 20);
        qy = load<u32>(s + 24);
        qz = load<u32>(s + 28);
        call(ctx, 0x0062E094u, {s});
        call(ctx, 0x0062D08Cu, {s});
    }
    const f64 px = Lfs45(p), py = Lfs45(p + 4), pz = Lfs45(p + 8);
    const auto fv = [](u32 w) { return static_cast<f64>(std::bit_cast<f32>(w)); };
    const f64 dy = F45(fv(qy) - py);
    if (dy < lo || dy > hi) {
        ctx.fpr[1] = pz;
        ret(ctx, 0);
        return;
    }
    if (out != 0) {
        store<u32>(out, qx);
        store<u32>(out + 4, qy);
        store<u32>(out + 8, qz);
    }
    const f64 dx = F45(fv(qx) - px), dz = F45(fv(qz) - pz);
    const f64 r2 = F45(r * r);
    f64 k = Lfs45(0x0001097Cu);
    k = F45(k * k);
    f64 d = Fma45(dx, dx, k);
    d = Fma45(dz, dz, d);
    ctx.fpr[1] = d;
    ret(ctx, d < r2 ? 1u : 0u);
}

// T* f(?, XmlCursor* x): a new 40-byte record (Mem_Alloc(40, 0, 16);
// Obj_InitVt_B21C98(p, x), vtable 0x00B0F630, +12..+36 zeroed) whose
// words +12..+36 are read with XmlCursor_ReadHash(x, hash, &field, dflt)
// for the hashes 0x00014B07 (default 0), 0x7A30D7E3, 0x9752205B,
// 0x94DB5F0E, 0x385F3FA3, 0xC830A35D, 0x6EC19EE9 (default: the field).
// r3 = the record (0 when the allocation fails).
DECOMP(00074838, XmlRec_New_B0F630_00074838) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {40, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    store<u32>(p, 0x00B0F630u);
    store<u32>(p + 28, 0);
    store<u32>(p + 32, 0);
    store<u32>(p + 36, 0);
    call(ctx, 0x004FA14Cu, {x, 0x00014B07u, p + 12, 0});
    static const u32 hs[6] = {0x7A30D7E3u, 0x9752205Bu, 0x94DB5F0Eu, 0x385F3FA3u, 0xC830A35Du, 0x6EC19EE9u};
    for (u32 k = 0; k < 6; ++k) call(ctx, 0x004FA14Cu, {x, hs[k], p + 16 + 4 * k, load<u32>(p + 16 + 4 * k)});
    ret(ctx, p);
}

// T* f(?, XmlCursor* x): a new 28-byte record (Mem_Alloc(28, 0, 16);
// Obj_InitVt_B21C98(p, x), vtable 0x00B0F6F8, +12..+20 zeroed, the bytes
// +24..+27 = 0, 1, 1, 0) read from x: hashes 0x7F247172 (default 0),
// 0x7A30D7E3, 0x03D1FCFC into +12..+20, bools 0xF5897C08, 0xCD9F562B,
// 0x5DDDC2A5, 0x469D7C52 into +24..+27 (default: the field). r3 = the
// record (0 when the allocation fails).
DECOMP(00075A7C, XmlRec_New_B0F6F8_00075A7C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B0F6F8u);
    store<u8>(p + 24, 0);
    store<u8>(p + 25, 1);
    store<u8>(p + 26, 1);
    store<u8>(p + 27, 0);
    call(ctx, 0x004FA14Cu, {x, 0x7F247172u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {x, 0x03D1FCFCu, p + 20, load<u32>(p + 20)});
    static const u32 hb[4] = {0xF5897C08u, 0xCD9F562Bu, 0x5DDDC2A5u, 0x469D7C52u};
    for (u32 k = 0; k < 4; ++k) call(ctx, 0x004F986Cu, {x, hb[k], p + 24 + k, load<u8>(p + 24 + k)});
    ret(ctx, p);
}

// T* f(?, XmlCursor* x): a new 36-byte record (Mem_Alloc(36, 0, 16);
// Obj_InitVt_B21C98(p, x), vtable 0x00B10008, +12 = -1, +16..+28 and the
// byte +32 zeroed) read from x: s32 0x3683141C into +12 (default -1; when
// the read fails and +12 > 0, --+12), hashes 0x8A93973F, 0x7A30D7E3,
// 0x98ACCC10, 0x884A0CB4 into +16..+28, bool 0x5B973429 into +32 (default:
// the field as it was before the first read for +16). r3 = the record (0
// when the allocation fails).
DECOMP(00084E9C, XmlRec_New_B10008_00084E9C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {36, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 12, 0xFFFFFFFFu);
    store<u32>(p, 0x00B10008u);
    store<u32>(p + 24, 0);
    store<u32>(p + 28, 0);
    store<u8>(p + 32, 0);
    const u32 ok = static_cast<u32>(call(ctx, 0x004F99BCu, {x, 0x3683141Cu, p + 12, ~0ull}));
    const u32 d16 = load<u32>(p + 16);
    if (ok == 0) {
        const s32 v = static_cast<s32>(load<u32>(p + 12));
        if (v > 0) store<u32>(p + 12, static_cast<u32>(v - 1));
    }
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 16, d16});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004FA14Cu, {x, 0x98ACCC10u, p + 24, load<u32>(p + 24)});
    call(ctx, 0x004FA14Cu, {x, 0x884A0CB4u, p + 28, load<u32>(p + 28)});
    call(ctx, 0x004F986Cu, {x, 0x5B973429u, p + 32, load<u8>(p + 32)});
    ret(ctx, p);
}

// T* f(?, XmlCursor* x): a new 24-byte record (Mem_Alloc(24, 0, 16);
// AnimCmd_ConstructBase(p), vtable 0x00B10E68, +12 / +16 and the bytes
// +20..+23 zeroed) read from x: hash 0x8A93973F into +12 (when found, the
// s32 0x5C8998CD too), hash 0x00255BF3 into +16, bools 0x469D7C52,
// 0xF5897C08, 0x6334AE6F, 0x834744B0 into +20..+23 (all default 0). r3 =
// the record (0 when the allocation fails).
DECOMP(00099DE4, XmlCmd_New_B10E68_00099DE4) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004B9CB8u, {p});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 0);
    store<u8>(p + 21, 0);
    store<u32>(p, 0x00B10E68u);
    store<u8>(p + 22, 0);
    store<u8>(p + 23, 0);
    if (call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0}) != 0) call(ctx, 0x004F99BCu, {x, 0x5C8998CDu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x00255BF3u, p + 16, 0});
    static const u32 hb[4] = {0x469D7C52u, 0xF5897C08u, 0x6334AE6Fu, 0x834744B0u};
    for (u32 k = 0; k < 4; ++k) call(ctx, 0x004F986Cu, {x, hb[k], p + 20 + k, 0});
    ret(ctx, p);
}

// u32 f(T* o): for both attachments o->vt[68]() and o->vt[64]() that are
// set, a->vt[44](a, 1). When the peer of +160 (WorldObj_GetPeer) exists
// and has +136 set: bit 0x20 of the byte Peer_GetVec(peer)->+28 = bit
// 0x80 of the byte +320 of Obj_FindComponent(o, 32, 0) (0 without it).
// When the effect +352 is set: its +136 |= 0x100 and r3 =
// Effect_Release(it, 0xD5E4206F); else r3 = 0.
DECOMP(000DF650, Obj_Attach_000DF650) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 slot : {68u, 64u}) {
        const u32 a = static_cast<u32>(VCall(ctx, o, slot, {o}));
        if (a != 0) VCall(ctx, a, 44, {a, 1});
    }
    const u32 w = load<u32>(o + 160);
    if (w != 0 && call(ctx, 0x0057A428u, {w}) != 0 &&
        load<u32>(static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)})) + 136) != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 32, 0}));
        const u32 w2 = load<u32>(o + 160);
        const u32 bit = c != 0 ? (load<u8>(c + 320) >> 7) & 1u : 0u;
        const u32 peer = static_cast<u32>(call(ctx, 0x0057A428u, {w2}));
        const u32 v = static_cast<u32>(call(ctx, 0x0057BE98u, {peer}));
        store<u8>(v + 28, static_cast<u8>((load<u8>(v + 28) & ~0x20u) | (bit << 5)));
    }
    const u32 fx = load<u32>(o + 352);
    if (fx == 0) {
        ret(ctx, 0);
        return;
    }
    store<u32>(fx + 136, load<u32>(fx + 136) | 0x100u);
    ret(ctx, call(ctx, 0x004CF3C4u, {load<u32>(o + 352), 0xD5E4206Fu}));
}

// Vec3* f(Vec3* out, T* o, f32 t): a curve point between A = +496 and B =
// +512: B when the two are bitwise equal; else with the control point
// C = B + dir(A - B) * min-clamped |A - B| (|A - B| when above
// *0x000F443C, else 1.0) * +668 + +640..+648 * +664: out = (1 - t)^2 B +
// k (1 - t) t C + t^2 A (k = *0x000F4440). r3 = out; f1 = out.z.
DECOMP(000F4448, Curve_Point_000F4448) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const f64 t = farg(ctx, 0);
    const u32 a = o + 496, b = o + 512;
    if (load<u64>(a) == load<u64>(b) && load<u32>(a + 8) == load<u32>(b + 8)) {
        Cp45(out, b);
        Cp45(out + 4, b + 4);
        Cp45(out + 8, b + 8);
        ctx.fpr[1] = Lfs45(b + 8);
        ret(ctx, out);
        return;
    }
    const f64 by = Lfs45(o + 516), ay = Lfs45(a + 4), bx = Lfs45(o + 512);
    f64 dy = F45(ay - by);
    const f64 ax = Lfs45(a), bz = Lfs45(o + 520);
    f64 dx = F45(ax - bx);
    const f64 az = Lfs45(a + 8);
    const u32 k = 0x000F4438u;
    f64 dz = F45(az - bz);
    f64 s = F45(dy * dy);
    const f64 eps = Lfs45(k + 4);
    f64 len = Lfs45(k);
    const f64 one = len;
    s = Fma45(dx, dx, s);
    s = Fma45(dz, dz, s);
    const f64 r = F45(std::sqrt(s));
    if (s > eps) len = r;
    const f64 inv = F45(one / r);
    const f64 g = Lfs45(o + 664);
    const f64 ox = Lfs45(o + 640), oy = Lfs45(o + 644), oz = Lfs45(o + 648);
    len = F45(len * Lfs45(o + 668));
    const f64 k2 = Lfs45(k + 8);
    const f64 u = F45(one - t);
    const f64 tt = F45(t * t);
    const f64 uu = F45(u * u);
    dx = F45(dx * inv);
    dy = F45(dy * inv);
    dz = F45(dz * inv);
    f64 cx = Fma45(dx, len, bx), cy = Fma45(dy, len, by), cz = Fma45(dz, len, bz);
    cx = Fma45(ox, g, cx);
    cy = Fma45(oy, g, cy);
    cz = Fma45(oz, g, cz);
    cx = F45(F45(F45(cx * k2) * u) * t);
    cy = F45(F45(F45(cy * k2) * u) * t);
    cz = F45(F45(F45(cz * k2) * u) * t);
    const f64 rx = Fma45(ax, tt, Fma45(bx, uu, cx));
    const f64 ry = Fma45(ay, tt, Fma45(by, uu, cy));
    const f64 rz = Fma45(az, tt, Fma45(bz, uu, cz));
    store<u32>(out, W45(rx));
    store<u32>(out + 4, W45(ry));
    store<u32>(out + 8, W45(rz));
    ctx.fpr[1] = rz;
    ret(ctx, out);
}

// u32 f(State* st, T* o): unless +348 <= k0 (*0x0016D450) and the anim
// +692 is already 0x03DE9A54: Anim_Restart2775A8(o->vt[60](), 0xD467C476,
// 0, 0, 0, -1, 0, 0, -, -, 0, -1; k1, k2, k0, k0) and +692 = 0xD467C476.
// With the component c = Obj_FindComponent(o, 19, CallK2_000D94F4(o)):
// st->+4 = n = +720, c->+1224 = c->+1228 = k1 / (n + k1) and c->+1232 =
// Obj_RandRange_0025DD60(c). Then +720 = st->+4 + 1 = r3.
DECOMP(0016D460, Obj_EnterState_0016D460) {
    const u32 st = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 k = 0x0016D450u;
    const f64 k0 = Lfs45(k);
    const f64 v = Lfs45(o + 348);
    ctx.fpr[1] = v;
    if (!(v <= k0) || load<u32>(o + 692) != 0x03DE9A54u) {
        const u32 a = static_cast<u32>(VCall(ctx, o, 60, {o}));
        // the 9th / 10th stack slots are not written; the callee does not read them
        callf(ctx, 0x002775A8u, {a, 0xD467C476u, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {Lfs45(k + 4), Lfs45(k + 8), k0, k0});
        store<u32>(o + 692, 0xD467C476u);
    }
    u32 c = static_cast<u32>(call(ctx, 0x000D94F4u, {o}));
    if (c != 0) c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 19, c}));
    if (c != 0) {
        const u32 n = load<u32>(o + 720);
        store<u32>(st + 4, n);
        const f64 k1 = Lfs45(k + 4);
        const f64 d = F45(F45(static_cast<f64>(n)) + k1);
        const f64 q = F45(k1 / d);
        store<u32>(c + 1224, W45(q));
        store<u32>(c + 1228, W45(q));
        ctx.fpr[1] = q;
        call(ctx, 0x0025DD60u, {c});
        store<u32>(c + 1232, W45(ctx.fpr[1]));
    }
    const u32 r = load<u32>(st + 4) + 1;
    store<u32>(o + 720, r);
    ret(ctx, r);
}

// u32 f(?, XmlCursor* x, Def* d): XmlDef_ReadBase_000CBE10(-, x, d), the
// f32 0xE6DF8EF6 into +256; then every child c of x with bit 0x80 of the
// byte +24 and the tag 0x65AF9A49 (+20), while the u16 count +244 < 4,
// that reads a vec2 0x01968AD4 (default *0x01164AFC) successfully is
// appended to the 8-byte vector +236 (PtrVector_Reserve to twice the u16
// capacity +240 when full; the u16 +246 keeps the largest count). r3 = 0.
DECOMP(001B5438, XmlDef_ReadPoints_001B5438) {
    const u32 x = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    call(ctx, 0x000CBE10u, {arg<u32>(ctx, 0), x, d});
    callf(ctx, 0x004F9B20u, {x, 0xE6DF8EF6u, d + 256}, {Lfs45(d + 256)});
    u32 c = static_cast<u32>(call(ctx, 0x004F9140u, {x}));
    if (c != 0) {
        Stack frame(ctx, 16);
        const u32 s = frame.addr();
        do {
            if (((load<u8>(c + 24) >> 7) & 1u) != 0 && load<u32>(c + 20) == 0x65AF9A49u && load<u16>(d + 244) < 4) {
                Cp45(s, 0x01164AFCu);
                Cp45(s + 4, 0x01164B00u);
                ctx.fpr[1] = Lfs45(0x01164AFCu);
                ctx.fpr[2] = Lfs45(0x01164B00u);
                if (call(ctx, 0x004F9C9Cu, {c, 0x01968AD4u, s, s}) == 0) {
                    u32 n = load<u16>(d + 244);
                    const u32 cap = load<u16>(d + 240);
                    if (n == cap) {
                        call(ctx, 0x00551718u, {d + 236, (cap << 1) & 0x1FFFEu});
                        n = load<u16>(d + 244);
                    }
                    const u32 e = load<u32>(d + 236) + ((n << 3) & 0x7FFF8u);
                    Cp45(e, s);
                    Cp45(e + 4, s + 4);
                    ctx.fpr[1] = Lfs45(s + 4);
                    const u32 n1 = load<u16>(d + 244) + 1;
                    const u32 mx = load<u16>(d + 246);
                    store<u16>(d + 244, static_cast<u16>(n1));
                    store<u16>(d + 246, static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(n1 & 0xFFFFu) ? mx : n1));
                }
            }
            c = static_cast<u32>(call(ctx, 0x004F8D74u, {c}));
        } while (c != 0);
    }
    ret(ctx, 0);
}

// u32 f(T* o, const Def* d): Thunk_000C2B90(o, d); the bytes +172..+175 =
// d->+80..+83, vtable 0x00B19510, String_CopyN(o + 140, d->+76, 31),
// +176 = 0, vtable 0x00B1AE70, +180 = d->+88. Then the key d->+84 is
// looked up in the tree of s = Set_Find(0) (+16 -> root; nodes {+4 key,
// +8 value, +12 / +16 children by unsigned order}; the path is pushed on
// the u32 stack +20 with the u16 count +100): found -> s->+104 = node,
// *0x011DE288 = r3 = +176 = node->+8; missing -> s->+104 = the last node
// (empty tree: 0 and *0x011DE288 = 0) and r3 = +176 = 0. No set: r3 = 0.
DECOMP(00221F3C, Obj_Init_B1AE70_00221F3C) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000C2B90u, {o, d});
    store<u8>(o + 172, load<u8>(d + 80));
    store<u8>(o + 173, load<u8>(d + 81));
    store<u32>(o, 0x00B19510u);
    const u8 b82 = load<u8>(d + 82);
    const u32 name = load<u32>(d + 76);
    store<u8>(o + 174, b82);
    store<u8>(o + 175, load<u8>(d + 83));
    call(ctx, 0x00553638u, {o + 140, name, 31});
    const u32 w88 = load<u32>(d + 88);
    store<u32>(o + 176, 0);
    store<u32>(o, 0x00B1AE70u);
    store<u32>(o + 180, w88);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w88));
    const u32 s = static_cast<u32>(call(ctx, 0x004BCE90u, {0, 0, 0x00B1AE70u}));
    if (s == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 key = load<u32>(d + 84);
    u32 n = load<u32>(load<u32>(s + 16));
    if (n == 0) {
        store<u32>(s + 104, 0);
        store<u32>(0x011DE288u, 0);
        store<u32>(o + 176, 0);
        ret(ctx, 0);
        return;
    }
    store<u16>(s + 100, 0);
    u32 r3 = 0;
    for (;;) {
        const u32 cur = n;
        const u32 k = load<u32>(cur + 4);
        if (k == key) {
            store<u32>(s + 104, cur);
            store<u32>(0x011DE288u, load<u32>(cur + 8));
            const u32 hit = load<u32>(s + 104);
            if (hit != 0) r3 = load<u32>(hit + 8);
            break;
        }
        n = k > key ? load<u32>(cur + 12) : load<u32>(cur + 16);
        if (n == 0) {
            // cur is never null here (the 0 branch of the original is dead)
            store<u32>(s + 104, cur);
            break;
        }
        const s32 c = static_cast<s16>(load<u16>(s + 100));
        store<u16>(s + 100, static_cast<u16>(c + 1));
        store<u32>(s + 20 + 4 * static_cast<u32>(c), cur);
    }
    store<u32>(o + 176, r3);
    ret(ctx, r3);
}

// bool f(T* o, f32 dt): a pending +445 is cleared (with
// Effect_SetHash4CF988(+432, 0x04B6FB5B) when +432 is set);
// Area_Update_004AD7C8(+436, dt); Obj_UpdateTeamBase(o, dt) == 0 -> 0.
// Then for each member e of +344 (u16 count +352) with bit 0x40000 of
// +172: present = Obj_HasKey_004ACC34(a = +436, e); new = (+444 == 0 or
// e->+188 bit 0x00100000); present and not new -> Area_4AE160(a, e, 1);
// absent and new -> Area_Test_004ADD68(a, e, 0). r3 = 1.
DECOMP(00224BC8, Obj_UpdateArea_00224BC8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (load<u8>(o + 445) != 0) {
        const u32 fx = load<u32>(o + 432);
        store<u8>(o + 445, 0);
        if (fx != 0) call(ctx, 0x004CF988u, {fx, 0x04B6FB5Bu});
    }
    callf(ctx, 0x004AD7C8u, {load<u32>(o + 436)}, {dt});
    callf(ctx, 0x0020DFD8u, {o}, {dt});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0);
        return;
    }
    for (u32 i = 0; i < load<u16>(o + 352); ++i) {
        const u32 e = load<u32>(load<u32>(o + 344) + 4 * i);
        if ((load<u32>(e + 172) & 0x40000u) == 0) continue;
        u32 nw = 0;
        if (load<u8>(o + 444) == 0 || (load<u32>(e + 188) & 0x00100000u) != 0) nw = 1;
        const u32 has = static_cast<u32>(call(ctx, 0x004ACC34u, {load<u32>(o + 436), e}));
        const u32 ep = load<u32>(o + 344) + 4 * i;
        const u32 a = load<u32>(o + 436);
        if (has != 0 && nw == 0) call(ctx, 0x004AE160u, {a, load<u32>(ep), 1});
        else if (has == 0 && nw != 0) call(ctx, 0x004ADD68u, {a, load<u32>(ep), 0});
    }
    ret(ctx, 1);
}

// T* f(?, XmlCursor* x, u8 flag): for the attribute 0x00AD4C0C of x
// (XmlCursor_FindAttrByName) a new 24-byte record (SmallAlloc; +4 =
// String_HashNoCase(attr), +8 = 0, +12 = 25, vtable 0x00B1C920, +20 =
// 0x03F0D768) registered as {hash, record} in the array 0x01056F04 (u16
// capacity / count / high-water mark / flags 1 at +0..+6, data +8;
// Array_Reserve to twice the capacity when full); +16 = flag and
// XmlCursor_ReadHash(x, 0x0027873A, &+20, +20). r3 = the record. Quirk:
// without the attribute or the allocation, the reads and the flag write
// go through a null record (address 16 / 20) and r3 = 0.
DECOMP(0029FAA0, XmlRec_NewRegistered_0029FAA0) {
    const u32 x = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 2);
    const u32 a = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AD4C0Cu}));
    u32 p = 0;
    if (a != 0) {
        p = static_cast<u32>(call(ctx, 0x0055238Cu, {24}));
        if (p != 0) {
            store<u32>(p, 0x00B0DAACu);
            u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {a}));
            const u32 arr = 0x01056F04u;
            store<u32>(p + 8, 0);
            store<u8>(p + 16, 0);
            const u32 cap = load<u16>(arr);
            u32 n = load<u16>(arr + 2);
            store<u32>(p + 4, h);
            store<u32>(p + 12, 25);
            if (n == cap) {
                call(ctx, 0x00558F3Cu, {arr, (cap << 1) & 0xFFFEu});
                h = load<u32>(p + 4);
                n = load<u16>(arr + 2);
            }
            const u32 data = load<u32>(arr + 8);
            const u32 n2 = load<u16>(arr + 2);
            store<u32>(data + ((n << 3) & 0x7FFF8u), h);
            const u32 n1 = n2 + 1;
            const u32 mx = load<u16>(arr + 4);
            store<u16>(arr + 2, static_cast<u16>(n1));
            store<u32>(data + 8 * n2 + 4, p);
            const u32 fl = load<u16>(arr + 6);
            store<u16>(arr + 4, static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(n1 & 0xFFFFu) ? mx : n1));
            store<u16>(arr + 6, static_cast<u16>(fl | 1u));
            store<u32>(p, 0x00B1C920u);
            store<u32>(p + 20, 0x03F0D768u);
        }
        store<u8>(p + 16, static_cast<u8>(flag));  // quirk: address 16 when the allocation failed
    }
    call(ctx, 0x004FA14Cu, {x, 0x0027873Au, p + 20, load<u32>(p + 20)});
    ret(ctx, p);
}

// u32 f(T* o) (destructor): vtables 0x00B1CB48 (+0) / 0x00B1CB94 (+24),
// Obj_PopAll_002AF294(o), PtrVector_Clear + Nop of +52 and +32,
// Registry_Unlink2_0048F6B0(o + 24), vtable 0x00B1BE30, o taken out of
// the entry array of 0x01056F1C; r3 = List1013B5C_Remove(o).
DECOMP(002AF3AC, Obj_Dtor_B1CB48_002AF3AC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1CB48u);
    store<u32>(o + 24, 0x00B1CB94u);
    call(ctx, 0x002AF294u, {o});
    for (u32 off : {52u, 32u}) {
        call(ctx, 0x0054EC1Cu, {o + off});
        call(ctx, 0x0054EC18u, {o + off});
    }
    call(ctx, 0x0048F6B0u, {o + 24});
    store<u32>(o, 0x00B1BE30u);
    sly4::obj39::Unlist1056F1C39(o);
    ret(ctx, call(ctx, 0x000D6940u, {o}));
}

// u32 f(T* o): for the stance s = Obj_Stance_002C39A0(o) in 0..8, r3 =
// *SortedMap_Find75E260(0x0105ABB8, &key) with key = o->+76[s]; else 0.
DECOMP(002C3B20, Obj_StanceValue_002C3B20) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = static_cast<s32>(call(ctx, 0x002C39A0u, {o}));
    if (s < 0 || s > 8) {
        ret(ctx, 0);
        return;
    }
    Stack frame(ctx, 16);
    const u32 k = frame.addr();
    const u32 key = load<u32>(o + 76 + 4 * static_cast<u32>(s));
    store<u32>(k, key);
    // r5 keeps the key (the callee reads r3 / r4 only)
    ret(ctx, load<u32>(static_cast<u32>(call(ctx, 0x0075E260u, {0x0105ABB8u, k, key}))));
}

// u32 f(T* o, u32 key, u8 b, f32 t): Anim_Restart2775A8(o->vt[60](), key,
// 0, b, -1, -, 0, 0, -, -, 0, -1; k0, t, k1, k1) with k0 / k1 =
// *0x002F63D8 / *0x002F63DC; then for each of the three channels c of
// +900 with the bytes +69 and +68 set, Anim_Restart2775A8(o->vt[60](),
// key, c->+40, b, 0, -1, 0, 0, -, -, 0, -1; same floats). r3 = what the
// last channel left (the channel, 0, or the restart's result).
DECOMP(002F63E0, Obj_RestartAnims_002F63E0) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2) & 0xFFu;
    const f64 t = farg(ctx, 0);
    const u32 a = static_cast<u32>(VCall(ctx, o, 60, {o}));
    const f64 k0 = Lfs45(0x002F63D8u), k1 = Lfs45(0x002F63DCu);
    // r8 and the 9th / 10th stack slots are leftovers the callee does not read
    callf(ctx, 0x002775A8u, {a, key, 0, b, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {k0, t, k1, k1});
    u64 r3 = 0;
    for (u32 i = 0; i < 3; ++i) {
        const u32 cp = o + 900 + 4 * i;
        const u32 c = load<u32>(cp);
        r3 = c;
        if (load<u8>(c + 69) == 0) continue;
        r3 = load<u8>(c + 68);
        if (r3 == 0) continue;
        const u32 a2 = static_cast<u32>(VCall(ctx, o, 60, {o}));
        callf(ctx, 0x002775A8u, {a2, key, load<u32>(load<u32>(cp) + 40), b, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {k0, t, k1, k1});
        r3 = ctx.gpr[3];
    }
    ret(ctx, r3);
}

// u32 f(Def* d, XmlCursor* x): reads hash 0x0024728B into +0, s32
// 0xC8B3AFB8 / 0x9AA8994A / 0x4810FC97 into +4 / +8 / +12, f32 0xC0C3EAED /
// 0xC0C3E9FF into +16 / +20 (each defaulting to the field);
// XmlCursor_FindAttr(x, 0x0024728B); then the warnings "delayBetweenMax
// must be ..." (0x00ADB9B8) when +20 < +16 (checked twice) and "maxActive
// must be ..." (0x00ADBA28) when +8 < +4. r3 = the last call's result or
// +8; f1 = +20.
DECOMP(0033BEA4, SpawnDef_Read_0033BEA4) {
    const u32 d = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    call(ctx, 0x004FA14Cu, {x, 0x0024728Bu, d, load<u32>(d)});
    call(ctx, 0x004F99BCu, {x, 0xC8B3AFB8u, d + 4, sly4::obj39::Sx39(load<u32>(d + 4))});
    call(ctx, 0x004F99BCu, {x, 0x9AA8994Au, d + 8, sly4::obj39::Sx39(load<u32>(d + 8))});
    call(ctx, 0x004F99BCu, {x, 0x4810FC97u, d + 12, sly4::obj39::Sx39(load<u32>(d + 12))});
    callf(ctx, 0x004F9B20u, {x, 0xC0C3EAEDu, d + 16}, {Lfs45(d + 16)});
    callf(ctx, 0x004F9B20u, {x, 0xC0C3E9FFu, d + 20}, {Lfs45(d + 20)});
    call(ctx, 0x004F9534u, {x, 0x0024728Bu});
    ctx.fpr[1] = Lfs45(d + 20);
    if (Lfs45(d + 20) < Lfs45(d + 16)) call(ctx, 0x005534D0u, {0x00ADB9B8u});
    u64 r3 = load<u32>(d + 8);
    if (static_cast<s32>(load<u32>(d + 8)) < static_cast<s32>(load<u32>(d + 4))) r3 = call(ctx, 0x005534D0u, {0x00ADBA28u, load<u32>(d + 4)});
    ctx.fpr[1] = Lfs45(d + 20);
    if (Lfs45(d + 20) < Lfs45(d + 16)) r3 = call(ctx, 0x005534D0u, {0x00ADB9B8u});
    ret(ctx, r3);
}

// u32 f(Pool* p, Slot* s): fills an empty slot (s set, s->+0 == 0) while
// fewer than +52 (0 = no limit) are active (+36): a random entry of the
// free list +132 (u16 count +140; the inlined Rand16 step of 0x01164888,
// the last entry moves into the hole) goes into the used vector +112
// (u16 count / capacity / high-water +120 / +116 / +122;
// PtrVector_Reserve to twice the capacity when full), s->+0 = it,
// it->vt[24](it), s->+4 = 0, ++p->+36. r3 = the new active count; else
// what r3 held (p, s->+0 or the limit).
DECOMP(0033C130, Pool_Acquire_0033C130) {
    const u32 p = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if (s == 0) {
        ret(ctx, p);
        return;
    }
    const u32 cur = load<u32>(s);
    if (cur != 0) {
        ret(ctx, cur);
        return;
    }
    const u32 lim = load<u32>(p + 52);
    if (lim != 0 && !(static_cast<s32>(load<u32>(p + 36)) < static_cast<s32>(lim))) {
        ret(ctx, lim);
        return;
    }
    const u32 g = 0x01164888u;
    const u32 n = load<u16>(p + 140);
    const u32 w = load<u32>(g + 8);
    const u32 q = n != 0 ? w / n : 0;  // divwu by 0 gives 0 in the reference simulator
    const u32 i = w - q * n;
    const u32 x = (std::rotl(w, 4) + std::rotl(w, 16)) ^ 0x87654321u;
    const u32 base = load<u32>(p + 132);
    const u32 n1 = n - 1;
    store<u32>(g + 8, std::rotl(x, 16));
    store<u64>(g, static_cast<u64>(x & 0xFFFFu));
    const u32 e = load<u32>(base + 4 * i);
    store<u16>(p + 140, static_cast<u16>(n1));
    if (i != (n1 & 0xFFFFu)) store<u32>(base + 4 * i, load<u32>(base + ((n1 << 2) & 0x3FFFCu)));
    u32 c = load<u16>(p + 120);
    const u32 cap = load<u16>(p + 116);
    if (c == cap) {
        call(ctx, 0x00551718u, {p + 112, (cap << 1) & 0x1FFFEu});
        c = load<u16>(p + 120);
    }
    store<u32>(load<u32>(p + 112) + ((c << 2) & 0x3FFFCu), e);
    const u32 c1 = load<u16>(p + 120) + 1;
    const u32 mx = load<u16>(p + 122);
    store<u16>(p + 120, static_cast<u16>(c1));
    const u32 vt = load<u32>(e);  // read before the stores below (they may alias it)
    store<u16>(p + 122, static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(c1 & 0xFFFFu) ? mx : c1));
    store<u32>(s, e);
    sly4::obj25::VCallVt(ctx, vt, 24, {e});
    store<u32>(s + 4, 0);
    const u32 act = load<u32>(p + 36) + 1;
    store<u32>(p + 36, act);
    ret(ctx, act);
}

// u32 f(T* o): StartSpeech45 with *0x003772B8, the hash table 0x00F64FC4
// and the owner's entries +184 (count +188).
DECOMP(003772C0, Obj_StartSpeech_003772C0) { StartSpeech45(ctx, 0x003772B8u, 0x00F64FC4u, 188, 184); }

// u32 f(T* o): StartSpeech45 with *0x0037AC68, the hash table 0x00F64FE0
// and the owner's entries +180 (count +184).
DECOMP(0037AC70, Obj_StartSpeech_0037AC70) { StartSpeech45(ctx, 0x0037AC68u, 0x00F64FE0u, 184, 180); }

// u32 f(T* o): StartSpeech45 with *0x0037F198, the hash table 0x00F64FFC
// and the owner's entries +180 (count +184).
DECOMP(0037F1A0, Obj_StartSpeech_0037F1A0) { StartSpeech45(ctx, 0x0037F198u, 0x00F64FFCu, 184, 180); }

// u32 f(T* o): byte +231 = 0; Handle_Call363A3C(0xAEF40D0E, 0, 1, -, 0,
// 0, 0x01164CA4; k = *0x0040A8D8), SetSelect_491000(o, 0x5C327C27),
// o->+220->vt[88](it, 1, 1, 1, 1); the camera c = o->vt[76]()->+0x1281E0
// gets Camera_PushTarget0FC040(c, o->+60, -, 0; k) unless
// Obj_HasWord368_000FC5DC(c, o->+60). With the object +236: its +184 |=
// 0x80000000, it->vt[24](it, 1, 0) and r3 = Obj_Set180_0036EA80(it, 0);
// else r3 = 0.
DECOMP(0040A8E0, Obj_Activate_0040A8E0) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 r6 = ctx.gpr[6];
    store<u8>(o + 231, 0);
    const f64 k = Lfs45(0x0040A8D8u);
    // r6 is the caller's (the callee does not read it)
    callf(ctx, 0x0036735Cu, {0xAEF40D0Eu, 0, 1, r6, 0, 0, 0x01164CA4u}, {k});
    call(ctx, 0x00491000u, {o, 0x5C327C27u});
    const u32 c220 = load<u32>(o + 220);
    VCall(ctx, c220, 88, {c220, 1, 1, 1, 1});
    const u32 b = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 cam = load<u32>(b + 0x001281E0u);
    if (cam != 0 && call(ctx, 0x000FC5DCu, {cam, load<u32>(o + 60)}) == 0) {
        // r5 is a leftover of the previous call (not read)
        callf(ctx, 0x000FC040u, {cam, load<u32>(o + 60), ctx.gpr[5], 0}, {k});
    }
    const u32 x = load<u32>(o + 236);
    if (x == 0) {
        ret(ctx, 0);
        return;
    }
    store<u32>(x + 184, load<u32>(x + 184) | 0x80000000u);
    const u32 x2 = load<u32>(o + 236);
    VCall(ctx, x2, 24, {x2, 1, 0});
    ret(ctx, call(ctx, 0x0036EA80u, {load<u32>(o + 236), 0}));
}

// u32 f(T* o, const Arr* src, u32 a, u32 b, u8 m, u32 c, u8 d):
// Obj_InitVt_B20CC8(o, m, a, b, ...), vtable 0x00B21070, the 76-byte
// element vector +12 (PtrVector_Construct, u16 +18 = 76, PtrVector_Init(
// v, src count, 0, 0)), +32 = c, +36 = a, +40 = b, +44 = 0, byte +48 = d;
// then each 76-byte element of src (data +0, u16 count +8) is appended
// (74 bytes memmoved; PtrVector_Reserve to twice the u16 capacity +16 when
// full; u16 count / high-water +20 / +22). r3 = the vector count (+1
// wide after an append).
DECOMP(00433120, Obj_InitCopyElems_00433120) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const u32 m = arg<u32>(ctx, 4), c = arg<u32>(ctx, 5), d = arg<u32>(ctx, 6);
    call(ctx, 0x0042C9D0u, {o, m & 0xFFu, a, b, m, c, d});
    const u32 v = o + 12;
    store<u32>(o, 0x00B21070u);
    const u32 n = load<u16>(src + 8);
    call(ctx, 0x005516E0u, {v});
    store<u16>(o + 18, 76);
    call(ctx, 0x00551598u, {v, n, 0, 0});
    store<u32>(o + 32, c);
    store<u32>(o + 36, a);
    store<u32>(o + 40, b);
    store<u32>(o + 44, 0);
    u32 cnt = load<u16>(o + 20);
    store<u8>(o + 48, static_cast<u8>(d));
    u32 r3 = cnt;
    for (u32 i = 0; i < load<u16>(src + 8); ++i) {
        const u32 e = load<u32>(src) + 76 * i;
        const u32 cap = load<u16>(o + 16);
        if ((cnt & 0xFFFFu) == cap) {
            call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
            r3 = load<u16>(o + 20);
        }
        call(ctx, 0x0071C3F0u, {load<u32>(o + 12) + 76 * (r3 & 0xFFFFu), e, 74});
        r3 = load<u16>(o + 20) + 1;
        const u32 mx = load<u16>(o + 22);
        store<u16>(o + 20, static_cast<u16>(r3));
        if (static_cast<s32>(mx) > static_cast<s32>(r3 & 0xFFFFu)) {
            cnt = r3 & 0xFFFFu;
            store<u16>(o + 22, static_cast<u16>(mx));
        } else {
            cnt = r3;
            store<u16>(o + 22, static_cast<u16>(r3));
        }
    }
    ret(ctx, r3);
}

// T* f(?, XmlCursor* x): a new 24-byte record (Mem_Alloc(24, 0, 16);
// vtable 0x00B221C8, u16 +4 = 1, +12 / +16 and the bytes +20..+23
// zeroed) read from x as in 0x00099DE4: hash 0x8A93973F into +12 (when
// found, the s32 0x5C8998CD too), hash 0x00255BF3 into +16, bools
// 0x469D7C52, 0xF5897C08, 0x6334AE6F, 0x834744B0 into +20..+23. r3 = the
// record (0 when the allocation fails).
DECOMP(004C3E14, XmlRec_New_B221C8_004C3E14) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    store<u32>(p + 12, 0);
    store<u16>(p + 4, 1);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 0);
    store<u8>(p + 21, 0);
    store<u32>(p, 0x00B221C8u);
    store<u8>(p + 22, 0);
    store<u8>(p + 23, 0);
    if (call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0}) != 0) call(ctx, 0x004F99BCu, {x, 0x5C8998CDu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x00255BF3u, p + 16, 0});
    static const u32 hb[4] = {0x469D7C52u, 0xF5897C08u, 0x6334AE6Fu, 0x834744B0u};
    for (u32 k = 0; k < 4; ++k) call(ctx, 0x004F986Cu, {x, hb[k], p + 20 + k, 0});
    ret(ctx, p);
}

// alloc->vt[20](alloc, ptr, size) with the entry in r6 and the OPD in r7
// (the order the first two frees of 0x0074B0A0 load them).
static void FreeVtR7_45(PPUContext& ctx, u32 al, u32 ptr, u64 size) {
    const u32 opd = load<u32>(load<u32>(al) + 20);
    call_ptr(ctx, opd, {al, ptr, size, load<u32>(opd), opd});
}

// u32 f(T* o) (destructor body): vtables 0x00F1E2B0 (+0) / 0x00F1E318
// (+16), +52 = 0; the three buffers {+48, +56}, {+36, +44}, {+24, +32}
// (pointer, size word; bit 0x80000000 = not owned) are freed through the
// static allocator 0x011DF31C (its vt[20](alloc, ptr, size), the size
// & 0x3FFFFFFF for the first, << 4 (16-byte units) for the others) and
// reset to {0, 0x80000000} (+40 / +28 zeroed too); vtables 0x00B13974
// (+0) / 0x00F1E070 (+16). r3 = 0.
DECOMP(0074B0A0, Buffers_Dtor_0074B0A0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 al = 0x011DF31Cu;
    const u32 w56 = load<u32>(o + 56);
    store<u32>(o + 52, 0);
    store<u32>(o, 0x00F1E2B0u);
    store<u32>(o + 16, 0x00F1E318u);
    if ((w56 & 0x80000000u) == 0) FreeVtR7_45(ctx, al, load<u32>(o + 48), w56 & 0x3FFFFFFFu);
    const u32 w44 = load<u32>(o + 44);
    store<u32>(o + 48, 0);
    store<u32>(o + 56, 0x80000000u);
    store<u32>(o + 40, 0);
    if ((w44 & 0x80000000u) == 0) FreeVtR7_45(ctx, al, load<u32>(o + 36), sly4::obj39::Sx39(w44 << 4));
    const u32 w32 = load<u32>(o + 32);
    store<u32>(o + 36, 0);
    store<u32>(o + 44, 0x80000000u);
    store<u32>(o + 28, 0);
    if ((w32 & 0x80000000u) == 0) VCall(ctx, al, 20, {al, load<u32>(o + 24), sly4::obj39::Sx39(w32 << 4)});
    store<u32>(o + 24, 0);
    store<u32>(o + 32, 0x80000000u);
    store<u32>(o + 16, 0x00F1E070u);
    store<u32>(o, 0x00B13974u);
    ret(ctx, 0);
}

// T* f(s16 id): a new 24-byte node (Mem_Alloc(24, 0, 16); Node_Init(p, id,
// 0x00B1FEB3)); the u16 16 goes into its record ((*0x00F65038)->+24 + 128
// * (s16)+16; address 0 when that index is -1); the four bytes at +20 +
// *(u16*)0x0106CF82 = the +64 of the bindings Node_Bind(p->+12, name, 3,
// 0, -1, 0) for the names 0x00B1FEC3 / C5 / C7 / C9 (each gets +76 = 0).
// r3 = the node (0 when the allocation fails).
DECOMP(00778804, Node_New4Binds_00778804) {
    const u32 id = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x00391694u, {p, sly4::obj39::Sx39(static_cast<u32>(static_cast<s32>(static_cast<s16>(id)))), 0x00B1FEB3u});
    const s32 i = static_cast<s16>(load<u16>(p + 16));
    const u32 tab = load<u32>(0x00F65038u);
    const u32 rec = i != -1 ? load<u32>(tab + 24) + 128 * static_cast<u32>(i) : 0u;
    store<u16>(rec, 16);  // quirk: address 0 when the index is -1
    const u32 dst = load<u32>(p + 20) + load<u16>(0x0106CF82u);
    for (u32 k = 0; k < 4; ++k) {
        const u32 b = static_cast<u32>(call(ctx, 0x004D3088u, {load<u32>(p + 12), 0x00B1FEC3u + 2 * k, 3, 0, ~0ull, 0}));
        const u32 v = load<u32>(b + 64);
        store<u32>(b + 76, 0);
        store<u8>(dst + k, static_cast<u8>(v));
    }
    ret(ctx, p);
}

// ---------------------------------------------------------------- sweep: 99 instructions
// u32 f(T* o): the direction +192 = +112 - +176 (y zeroed when bit 0x20 of
// the byte +305 is clear and bit 0x40 set, or neither and o->vt[132]() ==
// 0), normalized (k0 = *0x0003B7E8 / length); then a =
// |Vec_SignedAngle16680(o + 160, o + 192) * k1| and r3 = o->vt[220](o,
// a > k2 ? 35 : 31) (k1 / k2 = *0x0003B7EC / *0x0003B7F0). f1 = a.
DECOMP(0003B7F8, Obj_TurnAnim_0003B7F8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dx = F45(Lfs45(o + 112) - Lfs45(o + 176));
    const f64 dy = F45(Lfs45(o + 116) - Lfs45(o + 180));
    const f64 dz = F45(Lfs45(o + 120) - Lfs45(o + 184));
    const u32 b = load<u8>(o + 305);
    store<u32>(o + 192, W45(dx));
    store<u32>(o + 196, W45(dy));
    store<u32>(o + 200, W45(dz));
    if ((b & 0x20u) == 0) {
        if ((b & 0x40u) != 0 || static_cast<u32>(VCall(ctx, o, 132, {o})) == 0) store<u32>(o + 196, 0);
    }
    const u32 k = 0x0003B7E8u;
    f64 y = Lfs45(o + 196), x = Lfs45(o + 192);
    f64 s = F45(y * y);
    f64 z = Lfs45(o + 200);
    s = Fma45(x, x, s);
    s = Fma45(z, z, s);
    const f64 inv = F45(Lfs45(k) / F45(std::sqrt(s)));
    x = F45(x * inv);
    y = F45(y * inv);
    z = F45(z * inv);
    store<u32>(o + 192, W45(x));
    store<u32>(o + 196, W45(y));
    store<u32>(o + 200, W45(z));
    call(ctx, 0x00016680u, {o + 160, o + 192});
    const f64 a = std::fabs(F45(ctx.fpr[1] * Lfs45(k + 4)));
    const u32 anim = a > Lfs45(k + 8) ? 35u : 31u;
    ctx.fpr[1] = a;
    ret(ctx, VCall(ctx, o, 220, {o, anim}));
}

// u32 f(T* o): for the 5 x 7 slots of the bank (the second one, 700 bytes
// on, when +308 > 1; banks of 140-byte rows of 20-byte slots at +332):
// each slot with a second object b (+4) ends both b and the first one a
// (+0): Obj_SceneEnd0DD02C(x, 1, 0) and x->vt[88](x, 1, 1, 1, 1);
// Obj_SetEffectHash_000DAEC8(a, 2715), Anim_SetTrackTime_000DC550(a, -,
// 1, 0; *0x00058844), +12 = 3, +8 = 0.5. r3 = 3 when the last slot was
// handled, else 0.
DECOMP(00058848, Bank_ResetSlots_00058848) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 bank = o + (static_cast<s32>(load<u32>(o + 308)) > 1 ? 700u : 0u);
    const f64 t = Lfs45(0x00058844u);
    u32 r3 = 0;
    for (u32 row = 0; row < 5; ++row) {
        for (u32 i = 0; i < 7; ++i) {
            const u32 sl = bank + 140 * row + 20 * i + 332;
            r3 = load<u32>(sl + 4);
            if (r3 == 0) continue;
            call(ctx, 0x000DD02Cu, {r3, 1, 0});
            const u32 bb = load<u32>(sl + 4);
            VCall(ctx, bb, 88, {bb, 1, 1, 1, 1});
            call(ctx, 0x000DD02Cu, {load<u32>(sl), 1, 0});
            const u32 a = load<u32>(sl);
            VCall(ctx, a, 88, {a, 1, 1, 1, 1});
            call(ctx, 0x000DAEC8u, {load<u32>(sl), 2715});
            // r4 is a leftover of the previous call (not read)
            callf(ctx, 0x000DC550u, {load<u32>(sl), ctx.gpr[4], 1, 0}, {t});
            store<u32>(sl + 12, 3);
            store<u32>(sl + 8, 0x3F000000u);
            r3 = 3;
        }
    }
    ret(ctx, r3);
}

// The object behind the id: the component c = Effect_FindComponent(w, id)
// when c->vt[32]() == 5 and c->+28 is a link (+276 == 0x02235C33; its
// +280), else Obj_FindById(id) (0x00071D48).
static u32 LinkOrObj45(PPUContext& ctx, u32 w, u32 id) {
    if (id != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {w, id}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 l = load<u32>(c + 28);
            if (l != 0) {
                const u32 r = load<u32>(l + 276) == 0x02235C33u ? load<u32>(l + 280) : 0u;
                if (r != 0) return r;
            }
        }
    }
    return 0;
}

// u32 f(const Rec* r, World* w): a = the object of the id +12, b = that of
// +16 (LinkOrObj45, falling back to Obj_FindById); r3 =
// Obj_Detach0D72C0(a, b) when both exist, else 0.
DECOMP(00071D48, Rec_Detach_00071D48) {
    const u32 r = arg<u32>(ctx, 0), w = arg<u32>(ctx, 1);
    u32 a = LinkOrObj45(ctx, w, load<u32>(r + 12));
    if (a == 0) a = static_cast<u32>(call(ctx, 0x000D6A28u, {load<u32>(r + 12)}));
    if (a == 0) {
        ret(ctx, 0);
        return;
    }
    u32 b = LinkOrObj45(ctx, w, load<u32>(r + 16));
    if (b == 0) b = static_cast<u32>(call(ctx, 0x000D6A28u, {load<u32>(r + 16)}));
    if (b == 0) {
        ret(ctx, 0);
        return;
    }
    ret(ctx, call(ctx, 0x000D72C0u, {a, b}));
}

// T* f(?, XmlCursor* x): a new 36-byte record (Mem_Alloc(36, 0, 16);
// Obj_InitVt_B21C98(p, x), vtable 0x00B0F7C0, +20 = 1.0, the rest of
// +12..+32 zeroed) read from x: hash 0x8A93973F into +12; only when it
// is absent: hash 0x00255BF3 into +16, f32 0x00210FAA (default
// *0x00077720), 2683 and 0xB7EA3D94 (default *0x00077724) into +20..+28,
// bool 0xF5897C08 into +32. r3 = the record (0 when the allocation
// fails).
DECOMP(00077728, XmlRec_New_B0F7C0_00077728) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {36, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p, 0x00B0F7C0u);
    const f64 k0 = Lfs45(0x00077720u);
    store<u32>(p + 20, 0x3F800000u);
    const f64 k1 = Lfs45(0x00077724u);
    store<u32>(p + 24, 0);
    store<u32>(p + 28, 0);
    store<u8>(p + 32, 0);
    if (call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0}) == 0) {
        call(ctx, 0x004FA14Cu, {x, 0x00255BF3u, p + 16, 0});
        callf(ctx, 0x004F9B20u, {x, 0x00210FAAu, p + 20}, {k0});
        callf(ctx, 0x004F9B20u, {x, 2683, p + 24}, {k1});
        callf(ctx, 0x004F9B20u, {x, 0xB7EA3D94u, p + 28}, {k1});
        call(ctx, 0x004F986Cu, {x, 0xF5897C08u, p + 32, 0});
    }
    ret(ctx, p);
}

// T* f(?, XmlCursor* x): a new 48-byte record (Mem_Alloc(48, 0, 16);
// Obj_InitVt_B21C98(p, x), vtable 0x00B11508, the 76-byte element vector
// +12 (PtrVector_Construct, u16 +18 = 76, PtrVector_Init(v, 16, 0, 1)),
// +32..+40 = 0, the bytes +44 / +45 = 1 / 0) read from x: hashes
// 0x849B0778 (default 0), 0x74EB708F, 0x74EB4BED into +32..+40, bools
// 0x8488565B / 0x5B973429 into +44 / +45 (default: the field); then
// XmlQuery_ReadAll_0049EF40(x, v). r3 = the record (0 when the allocation
// fails).
DECOMP(000A0CF4, XmlRec_New_B11508_000A0CF4) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {48, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    const u32 v = p + 12;
    store<u32>(p, 0x00B11508u);
    call(ctx, 0x005516E0u, {v});
    store<u16>(p + 18, 76);
    call(ctx, 0x00551598u, {v, 16, 0, 1});
    store<u32>(p + 32, 0);
    store<u32>(p + 36, 0);
    store<u32>(p + 40, 0);
    store<u8>(p + 44, 1);
    store<u8>(p + 45, 0);
    call(ctx, 0x004FA14Cu, {x, 0x849B0778u, p + 32, 0});
    call(ctx, 0x004FA14Cu, {x, 0x74EB708Fu, p + 36, load<u32>(p + 36)});
    call(ctx, 0x004FA14Cu, {x, 0x74EB4BEDu, p + 40, load<u32>(p + 40)});
    call(ctx, 0x004F986Cu, {x, 0x8488565Bu, p + 44, load<u8>(p + 44)});
    call(ctx, 0x004F986Cu, {x, 0x5B973429u, p + 45, load<u8>(p + 45)});
    call(ctx, 0x0049EF40u, {x, v});
    ret(ctx, p);
}

// T* f(?, XmlCursor* x): a new 28-byte record (Mem_Alloc(28, 0, 16);
// Obj_InitVt_B21C98(p, x), vtable 0x00B118C8, +12 / +16 = 0, the bytes
// +20..+24 = 0, 1, 1, 1, 0) read from x: hashes 0x7A30D7E3 (default 0) /
// 0x03D1FCFC into +12 / +16, bools 0x469D7C52, 0xF5897C08, 0xCD9F562B,
// 0x5DDDC2A5, 0x80116C17 into +20..+24 (default: the field). r3 = the
// record (0 when the allocation fails).
DECOMP(000A5144, XmlRec_New_B118C8_000A5144) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 0);
    store<u32>(p, 0x00B118C8u);
    store<u8>(p + 21, 1);
    store<u8>(p + 22, 1);
    store<u8>(p + 23, 1);
    store<u8>(p + 24, 0);
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x03D1FCFCu, p + 16, load<u32>(p + 16)});
    static const u32 hb[5] = {0x469D7C52u, 0xF5897C08u, 0xCD9F562Bu, 0x5DDDC2A5u, 0x80116C17u};
    for (u32 k = 0; k < 5; ++k) call(ctx, 0x004F986Cu, {x, hb[k], p + 20 + k, load<u8>(p + 20 + k)});
    ret(ctx, p);
}

// u32 f(): when the byte 0x00F64CC4 is set, each of the UI elements
// 0x00AC5C90 / 0x00AC5CB0 / 0x00AC5CD0 (Ui_Find(name, 0)) that exists gets
// u->vt[152](u; *0x000AB7A8) and u->vt[156](u). r3 = the last call's
// result (0 when the last element is missing or the byte is clear).
DECOMP(000AB7B0, Ui_ResetThree_000AB7B0) {
    const u32 on = load<u8>(0x00F64CC4u);
    if (on == 0) {
        ret(ctx, 0);
        return;
    }
    const f64 k = Lfs45(0x000AB7A8u);
    u64 r3 = 0;
    for (u32 name : {0x00AC5C90u, 0x00AC5CB0u, 0x00AC5CD0u}) {
        const u32 u = static_cast<u32>(call(ctx, 0x004D5544u, {name, 0}));
        r3 = u;
        if (u == 0) continue;
        ctx.fpr[1] = k;
        VCall(ctx, u, 152, {u});
        r3 = VCall(ctx, u, 156, {u});
    }
    ret(ctx, r3);
}

// T* f(a, b, const Def* d, e): builds a 768-byte description on the
// stack (Desc_Init_007430AC(&s, d->+12, d->+16, d->+20),
// Desc_Fill_000BAAC8(a, b, d, e, &s)), copies it into a new 784-byte
// object (SmallAlloc; Obj_FromDesc_000B76E4(p, &s)), then destroys the
// stack copy (vtable 0x00B11E40; PtrVector_Clear + Nop of its vectors at
// +496, +348 / +300 / +252, +232 / +212 / +192, +148 and +128). r3 = the
// object (0 when the allocation fails).
DECOMP(000BB4B0, Obj_NewFromDesc_000BB4B0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2), e = arg<u32>(ctx, 3);
    Stack frame(ctx, 768);
    const u32 s = frame.addr();
    call(ctx, 0x007430ACu, {s, load<u32>(d + 12), load<u32>(d + 16), load<u32>(d + 20)});
    call(ctx, 0x000BAAC8u, {a, b, d, e, s});
    u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {784}));
    if (p != 0) call(ctx, 0x000B76E4u, {p, s});
    store<u32>(s, 0x00B11E40u);
    for (u32 off : {496u, 348u, 300u, 252u, 232u, 212u, 192u, 148u, 128u}) {
        call(ctx, 0x0054EC1Cu, {s + off});
        call(ctx, 0x0054EC18u, {s + off});
    }
    ret(ctx, p);
}

// T* f(T* o): copies 25 tuning values (the +72 f32 of the parameter
// records in 0x010323D0..0x010330C8; +80 / +4 / +8 for two of them) into
// +16..+132. r3 = o; f1 = the last one.
DECOMP(00119C00, Tuning_Load_00119C00) {
    const u32 o = arg<u32>(ctx, 0);
    static const u32 src[25] = {0x01032418u, 0x010324FCu, 0x01032550u, 0x01032554u, 0x01032558u, 0x010325A8u, 0x010325F4u,
                                0x01032640u, 0x0103268Cu, 0x010326D8u, 0x010327BCu, 0x01032808u, 0x01032724u, 0x01032770u,
                                0x01032854u, 0x010328A0u, 0x010328ECu, 0x01032938u, 0x01032984u, 0x010329D0u, 0x01032A1Cu,
                                0x0103302Cu, 0x01033078u, 0x010330C4u, 0x01033110u};
    static const u8 dst[25] = {16, 24, 32, 36, 40, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 120, 124, 128, 132};
    for (u32 k = 0; k < 25; ++k) Cp45(o + dst[k], src[k]);
    ctx.fpr[1] = Lfs45(src[24]);
    ret(ctx, o);
}

// The defaults-record tail of 0x001EB118 / 0x003078D8 (u32 f(a0, c, x,
// tmpl, o)): DamageColDef_Read_000C85A8(a0, c, x, tmpl, o); a defaults
// record on the stack (Def_Construct74FD80(rec, x->+12, name, name, 0, 1,
// 0x9260C26E, 0x01164B40; from the pool: f1..f13 = +0, +16, +12 x4, +4,
// +8, +16, +12 x4; stack floats s204..s228 = +12, s340 / s348 = +16, flag
// s296 set) gets the flag bits set in its word +fl_off;
// XmlCursor_DocPath(a0) when tmpl is set and (tmpl->+t_off & bits) == 0;
// then the record's vector +124 is destroyed (PtrVector_Clear, Nop,
// SmallFree(v, 20)). r3 = the last call's result, the masked word or the
// flag word.
static void DefDefaults45(PPUContext& ctx, u32 pool, u32 name, u32 fl_off, u32 bits, u32 t_off) {
    const u32 a0 = arg<u32>(ctx, 0), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    call(ctx, 0x000C85A8u, {a0, arg<u32>(ctx, 1), x, tmpl, arg<u32>(ctx, 4)});
    Stack frame(ctx, 256);
    const u32 rec = frame.addr();
    const f64 k0 = Lfs45(pool), k4 = Lfs45(pool + 4), k8 = Lfs45(pool + 8), k12 = Lfs45(pool + 12), k16 = Lfs45(pool + 16);
    const u64 f3w = load<u32>(pool + 12), f2w = load<u32>(pool + 16);
    // stack slots 112..192 are not set by the original (not read by the callee)
    callf(ctx, 0x0074FD80u, {rec, load<u32>(x + 12), name, name, 0, 1, 0x9260C26Eu, 0x01164B40u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f3w, f3w, f3w, f3w, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, f2w, f2w, 0}, {k0, k16, k12, k12, k12, k12, k4, k8, k16, k12, k12, k12, k12});
    u64 r3 = load<u32>(rec + fl_off) | bits;
    store<u32>(rec + fl_off, static_cast<u32>(r3));
    if (tmpl != 0) {
        r3 = load<u32>(tmpl + t_off) & bits;
        if (r3 == 0) r3 = call(ctx, 0x004F8944u, {a0});
    }
    const u32 v = load<u32>(rec + 124);
    if (v != 0) {
        call(ctx, 0x0054EC1Cu, {v});
        call(ctx, 0x0054EC18u, {v});
        r3 = call(ctx, 0x00552420u, {load<u32>(rec + 124), 20});
    }
    ret(ctx, r3);
}

// u32 f(a0, c, x, tmpl, o): DefDefaults45 with the pool 0x001EB104, the
// name 0x00AD1E98 and flag 2 in +8 (tmpl: +28).
DECOMP(001EB118, DamageColDef_Read_001EB118) { DefDefaults45(ctx, 0x001EB104u, 0x00AD1E98u, 8, 0x2u, 28); }

// u32 f(a0, c, x, tmpl, o): DefDefaults45 with the pool 0x003078C0, the
// name 0x00AD9DD0 and flag 0x40000 in +0 (tmpl: +20).
DECOMP(003078D8, DamageColDef_Read_003078D8) { DefDefaults45(ctx, 0x003078C0u, 0x00AD9DD0u, 0, 0x40000u, 20); }

// u32 f(T* o, const Pair* m): the far end of the link m (its +8 node,
// or +12 when the +8 node's anchor (+16 + s8 +32, when the s8 +40 is 1)
// is o->+8->vt[144]()): when that node's s8 +40 is 1 and its anchor
// (+16 + s8 +32) is in the vector +240 (u16 count +248) at index i, the
// counter +280[i] drops and at 0 entry i is removed from both vectors
// (the last entry moves in; u16 counts +248 / +288). r3 = 0, the count,
// i, the new +288 count or the moved counter, as the original leaves it.
DECOMP(002AEC9C, Obj_Unlink_002AEC9C) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 c8 = load<u32>(o + 8);
    const u32 self = static_cast<u32>(VCall(ctx, c8, 144, {c8}));
    u32 n = load<u32>(m + 8);
    u32 an = 0;
    if (static_cast<s8>(load<u8>(n + 40)) == 1) an = n + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(n + 32)))) + 16;
    if (an == self) n = load<u32>(m + 12);
    const u32 b = n + 16;
    if (static_cast<s8>(load<u8>(b + 24)) != 1) {
        ret(ctx, 0);
        return;
    }
    const u32 key = b + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(b + 16))));
    if (key == 0) {
        ret(ctx, 0);
        return;
    }
    u32 i = 0;
    for (;; ++i) {
        if (!(i < load<u16>(o + 248))) {
            ret(ctx, i);
            return;
        }
        if (load<u32>(load<u32>(o + 240) + 4 * i) == key) break;
    }
    const u32 cp = load<u32>(o + 280) + 4 * i;
    store<u32>(cp, load<u32>(cp) - 1);
    u32 cnt = load<u32>(o + 280);
    if (load<u32>(cnt + 4 * i) != 0) {
        ret(ctx, i);
        return;
    }
    const u32 n1 = load<u16>(o + 248) - 1;
    store<u16>(o + 248, static_cast<u16>(n1));
    if (i != (n1 & 0xFFFFu)) {
        const u32 v = load<u32>(o + 240);
        store<u32>(v + 4 * i, load<u32>(v + ((n1 << 2) & 0x3FFFCu)));
        cnt = load<u32>(o + 280);
    }
    const u32 m1 = load<u16>(o + 288) - 1;
    store<u16>(o + 288, static_cast<u16>(m1));
    if (i == (m1 & 0xFFFFu)) {
        ret(ctx, m1);
        return;
    }
    const u32 w = load<u32>(cnt + ((m1 << 2) & 0x3FFFCu));
    store<u32>(cnt + 4 * i, w);
    ret(ctx, w);
}

// bool f(XmlCursor* doc, XmlCursor* x, -, Rec* r): the kind r->+0 is an
// index into the 7 hashes of 0x00ADB64C; kind 0 is replaced by the hash
// 2529 of x (XmlCursor_ReadHash; a nonzero result means failure). An
// unknown hash or a failed read -> XmlCursor_DocPath(doc) and r3 = 0.
// Else r->+0 = the hash's index and bools 0xA7BC1270 / 0x8E0B3401 into
// +8 / +9, s32 0xA3F9EF1E into +4, bool 0x9412EDCD into +10 (default 0);
// r3 = 1.
DECOMP(00321D3C, XmlRec_ReadKind_00321D3C) {
    const u32 doc = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), r = arg<u32>(ctx, 3);
    const u32 tab = 0x00ADB64Cu;
    const u32 idx = load<u32>(r);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    u32 h = load<u32>(tab + 4 * idx);
    store<u32>(s, h);
    if (idx == 0) {
        const u32 bad = static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 2529, s, h}));
        h = load<u32>(s);
        if (bad != 0) {
            call(ctx, 0x004F8944u, {doc});
            ret(ctx, 0);
            return;
        }
    }
    u32 i = 0;
    while (i < 7 && load<u32>(tab + 4 * i) != h) ++i;
    if (i >= 7) {
        call(ctx, 0x004F8944u, {doc});
        ret(ctx, 0);
        return;
    }
    store<u32>(r, i);
    call(ctx, 0x004F986Cu, {x, 0xA7BC1270u, r + 8, 0});
    call(ctx, 0x004F986Cu, {x, 0x8E0B3401u, r + 9, 0});
    call(ctx, 0x004F99BCu, {x, 0xA3F9EF1Eu, r + 4, 0});
    call(ctx, 0x004F986Cu, {x, 0x9412EDCDu, r + 10, 0});
    ret(ctx, 1);
}

// u32 f(Loader* o, a, const Msg* m): with neither pending bit of +2576
// set: in state 4 (byte +18) without the world child (o->+8 +0x1281F4)
// o->vt[104](o, 5, 0); with it, messages 13 / 14 (m->+0) for that child
// (m->+24) set bit 0x80000000 / 0x40000000 of +2576. With a pending bit:
// both bits cleared, World_ReleaseChild1281F4(world, child, child) when
// the 0x80000000 one was set and the child is +2572 (nonzero), then
// o->vt[104](o, 5 for 0x80000000 else 6, 0). r3 = Loader_Step_003252D8(o,
// a, m).
DECOMP(00328A04, Loader_Update_00328A04) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const u32 w = load<u32>(o + 2576);
    const u32 ch = load<u32>(o + 2572);
    const u32 hi = w >> 31;
    const u32 wd = load<u32>(o + 8);
    if (hi != 0 || (w & 0x40000000u) != 0) {
        store<u32>(o + 2576, (w & 0x7FFFFFFFu) & ~0x40000000u);
        if (hi != 0 && ch != 0) {
            const u32 c = load<u32>(wd + 0x001281F4u);
            if (c == ch) call(ctx, 0x0047C490u, {wd, c, ch});
        }
        VCall(ctx, o, 104, {o, hi != 0 ? 5u : 6u, 0});
    } else if (load<u8>(o + 18) == 4) {
        const u32 c = load<u32>(wd + 0x001281F4u);
        if (c == 0) {
            VCall(ctx, o, 104, {o, 5, 0});
        } else {
            const s32 k = static_cast<s32>(load<u32>(m));
            if (k == 13) {
                if (load<u32>(m + 24) == c) store<u32>(o + 2576, w | 0x80000000u);
            } else if (k == 14) {
                if (load<u32>(m + 24) == c) store<u32>(o + 2576, w | 0x40000000u);
            }
        }
    }
    ret(ctx, call(ctx, 0x003252D8u, {o, a, m}));
}

// Src+72* f(const Src* s, Dst* d): fills d from s: +16..+24 = s->+0..+8,
// +36 = 2, +8 = s->+20; bits 0x10000000..0x00800000 of +120 = bits
// 0x80000000..0x04000000 of s->+128 (one at a time, each stored); +44 =
// s->+24, +48 = s->+80; +52..+60 = s->+96..+104 * *0x0032AAE4; +64..+80 =
// s->+28..+44; +84..+92 = s->+52..+60; byte +96 = s->+48; +112 = s->+112;
// byte +116 = s->+116, then the word +116 gets that byte on top and bits
// 0x00800000..0x00200000 = s->+128 bits 0x02000000..0x00800000; +124 =
// s->+132; +100..+108 = s->+64..+72. r3 = s + 72; f1 = s->+72.
DECOMP(0032AAE8, Desc_CopyOut_0032AAE8) {
    const u32 s = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    const u32 w20 = load<u32>(s + 20);
    Cp45(d + 16, s);
    store<u32>(d + 36, 2);
    Cp45(d + 20, s + 4);
    store<u32>(d + 8, w20);
    const u32 w8 = load<u32>(s + 8);
    u32 r5 = load<u32>(d + 120);
    store<u32>(d + 24, w8);
    for (u32 k = 0; k < 6; ++k) {
        const u32 m = 0x10000000u >> k;
        r5 = (r5 & ~m) | ((load<u32>(s + 128) >> 3) & m);
        store<u32>(d + 120, r5);
    }
    Cp45(d + 44, s + 24);
    store<u32>(d + 48, load<u32>(s + 80));
    const f64 k = Lfs45(0x0032AAE4u);
    store<u32>(d + 52, W45(F45(Lfs45(s + 96) * k)));
    const f64 y = Lfs45(s + 100);
    u32 w116 = load<u32>(d + 116);
    store<u32>(d + 56, W45(F45(y * k)));
    store<u32>(d + 60, W45(F45(Lfs45(s + 104) * k)));
    for (u32 i = 0; i < 5; ++i) Cp45(d + 64 + 4 * i, s + 28 + 4 * i);
    for (u32 i = 0; i < 3; ++i) store<u32>(d + 84 + 4 * i, load<u32>(s + 52 + 4 * i));
    store<u8>(d + 96, load<u8>(s + 48));
    Cp45(d + 112, s + 112);
    const u32 b = load<u8>(s + 116);
    store<u8>(d + 116, static_cast<u8>(b));
    w116 = (w116 & ~0xFF000000u) | (b << 24);
    for (u32 i = 0; i < 3; ++i) {
        const u32 m = 0x00800000u >> i;
        w116 = (w116 & ~m) | ((load<u32>(s + 128) >> 2) & m);
        store<u32>(d + 116, w116);
    }
    Cp45(d + 124, s + 132);
    Cp45(d + 100, s + 64);
    Cp45(d + 104, s + 68);
    Cp45(d + 108, s + 72);
    ctx.fpr[1] = Lfs45(s + 72);
    ret(ctx, s + 72);
}

// u32 f(T* o): o->vt[20](o, 0), Obj_Place3626E8(o, 0x01164B40, 1); +204 /
// +200 = 0, +160 keeps bit 0 only; the flag word +216 drops bits
// 0x3F000000 except that 0x40000000 takes the old 0x80000000; the n =
// o->+8->+152 (when > 0) 12-byte entries of +208 get their first two
// words zeroed; o->+168->vt[40](it, 0, 1) when set; then o leaves the
// world lists 4 / 0 / 8 of *0x00F65CA0 (World_ListRemove_0046B7A4(w, k,
// list 0x00F37A70 / 0x00F37A78 / 0x00F37A68, o, 0)). r3 = the last
// removal's result.
DECOMP(003767C8, Obj_Reset_003767C8) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8u, {o, 0x01164B40u, 1});
    u32 f = load<u32>(o + 216);
    const u32 w160 = load<u32>(o + 160) & 1u;
    f &= ~0x10000000u;
    f &= ~0x20000000u;
    store<u32>(o + 204, 0);
    f &= ~0x04000000u;
    store<u32>(o + 160, w160);
    f &= ~0x02000000u;
    store<u32>(o + 200, 0);
    u32 g = (f & ~0x40000000u) | ((f >> 1) & 0x40000000u);
    const u32 arr = load<u32>(o + 208);
    g &= ~0x01000000u;
    store<u32>(o + 216, g);
    if (arr != 0) {
        s32 n = static_cast<s32>(load<u32>(load<u32>(o + 8) + 152));
        if (n > 0) {
            u32 e = arr;
            do {
                --n;
                store<u32>(e, 0);
                store<u32>(e + 4, 0);
                e += 12;
            } while (n > 0);
        }
    }
    const u32 c = load<u32>(o + 168);
    if (c != 0) VCall(ctx, c, 40, {c, 0, 1});
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 4, 0x00F37A70u, o, 0});
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 0, 0x00F37A78u, o, 0});
    ret(ctx, call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 8, 0x00F37A68u, o, 0}));
}
