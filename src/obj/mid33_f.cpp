// Game code, medium sweep part 33: player stamina / boost helpers called
// from f_0023C168 and other player-state code.
#include "mid33.h"

using namespace sly4;
using namespace sly4::mid33;

namespace {
using namespace m33fn;
}  // namespace

// Boost step (o; f1 dt): for an active (+384 bit 31) player whose tuning
// +396 enables it (u8 +940): the boost flag +1088 bit 0x10000000 turns on
// when the meter (world player slot +108) is at least tuning +1356 (and
// the controller's slot 12 object's +36 bit 0x10000 is clear) and off
// when the meter runs dry or the boost has run tuning +1352 (> K0)
// seconds; while it stays on (or tuning +1348 seconds have not passed
// since it was started) +1004 = K1 and the meter drains by tuning +1336
// * dt + (+1332 on the first frame), clamped to K0 (slot +0 bit 0x40
// set), +1008 counting the time; otherwise +1004 = K0 and +1008 = 0.
// K = the f32 pair 0x0023B620.
DECOMP(0023B628, Boost_Step_0023B628) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 K = 0x0023B620u;
    if ((load<u32>(o + 384) & 0x80000000u) == 0) { ctx.gpr[3] = 0; return; }
    const u32 tun = load<u32>(o + 396);
    if (load<u8>(tun + 872 + 68) == 0) { ctx.gpr[3] = 0; return; }
    const s32 pi = static_cast<s8>(load<u8>(o + 172));
    const u32 h = load<u32>(o + 952);
    const u32 d = tun + 1272;
    const u32 slot = load<u32>(kWorld33) + static_cast<u32>(pi * 0x20C80) + 4208;
    const u32 q = static_cast<u32>(VCall(ctx, h, 12, {h}));
    const u32 blocked = load<u32>(q + 36) & 0x10000u;
    u32 f = load<u32>(o + 1088);
    const f64 k0 = Lf(K);
    if ((f & 0x10000000u) != 0 && Lf(slot + 108) <= k0) {
        f &= ~0x10000000u;
        store<u32>(o + 1088, f);
    } else if ((f & 0x10000000u) == 0 && blocked == 0 && Lf(d + 84) <= Lf(slot + 108)) {
        f |= 0x10000000u;
        store<u32>(o + 1088, f);
    }
    const f64 cur = Lf(o + 1004);
    bool on = false;
    if ((f & 0x10000000u) != 0) {
        bool stop = false;
        if (cur != k0) {
            const f64 lim = Lf(d + 80);
            stop = lim > k0 && lim < Lf(o + 1008);
        }
        if (stop) {
            store<u32>(o + 1088, f & ~0x10000000u);
        } else {
            if (cur != k0 && Lf(d + 76) > Lf(o + 1008)) on = true;
            else on = blocked != 0;
        }
    }
    const f64 v = on ? Lf(K + 4) : Lf(K);
    store<f32>(o + 1004, static_cast<f32>(v));
    ctx.fpr[1] = cur;
    if (v == Lf(K)) {
        store<u32>(o + 1008, 0);
        ctx.gpr[3] = K;
        return;
    }
    f64 base = Lf(K);
    const f64 rate = Lf(d + 64);
    if (cur == base) base = Lf(d + 60);
    f64 m = Sgl28(Lf(slot + 108) - Fmadds(rate, dt, base));
    if (m < Lf(K)) m = Lf(K);
    const u32 w = load<u32>(slot) | 0x40u;
    store<f32>(slot + 108, static_cast<f32>(m));
    store<u32>(slot, w);
    const f64 t = Sgl28(Lf(o + 1008) + dt);
    store<f32>(o + 1008, static_cast<f32>(t));
    ctx.fpr[1] = t;
    ctx.gpr[3] = w;
}

// Boost animation select (o): with the animator +736 in mode 4 (slot 32
// with 0) and its layer 1 state (slot 32 with 1) s != 4, layer 1 (slot 28
// with 1, v) goes to 3 while boosting (+1088 bit 31); otherwise s == 3
// -> 0 when +964 is 1; s == 2 -> 1 when the speed +936 < K; s == 1 -> 2
// when it is > K; s == 0 -> 2 / 1 by the same tests. K = the f32 at
// 0x0023B9D0. Leaves r3 / f1 as the original does.
DECOMP(0023B9D8, Boost_SelectAnim_0023B9D8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x0023B9D0u;
    u32 a = load<u32>(o + 736);
    ctx.gpr[3] = 0;
    if (a == 0) return;
    if (static_cast<s32>(VCall(ctx, a, 32, {a, 0})) != 4) return;
    a = load<u32>(o + 736);
    const s32 s = static_cast<s32>(VCall(ctx, a, 32, {a, 1}));
    auto set = [&](u32 v) {
        const u32 x = load<u32>(o + 736);
        VCall(ctx, x, 28, {x, 1, v});
    };
    if (s == 4 || s < 0 || s > 3) return;
    if (s == 3) {
        const u32 m = load<u32>(o + 964);
        ctx.gpr[3] = m;
        if (m == 1) set(0);
        return;
    }
    if ((load<u32>(o + 1088) & 0x80000000u) != 0) return set(3);
    const f64 sp = Lf(o + 936), k = Lf(K);
    ctx.gpr[3] = K;
    if (s == 0) {
        ctx.fpr[1] = k;
        if (sp > k) set(2);
        else if (sp < k) set(1);
        return;
    }
    ctx.fpr[1] = sp;
    if (s == 2 && sp < k) set(1);
    if (s == 1 && sp > k) set(2);
}

// Reset (o) (constructor helper of Obj_Ctor_00247AB8): copies the 4x3
// matrix +96 (rows of 16) to +1104, clears the state words, +480 = the
// vector 0x01164B60, +512 = 90.0f, +1280 = the basis 0x01164BD0, resets
// the embedded +1392 object (slot 28 with 0, 1; +1420 = o), clears
// +532 / +676 / +820 / +964 / +1380, +1384 = (+1384 | 0x40) & ~0x80 and
// the four bytes +1385 = those at 0x01164CA0.
DECOMP(00246080, Obj_Reset_00246080) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 432, 0);
    store<u32>(o + 428, 0);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + 1104 + 16 * r + k, load<u32>(o + 96 + 16 * r + k));
    for (u32 off : {392u, 436u, 440u, 444u, 456u, 452u, 448u, 464u}) store<u32>(o + off, 0);
    store<u32>(o + 480, load<u32>(0x01164B60u));
    store<u32>(o + 484, load<u32>(0x01164B64u));
    store<u32>(o + 496, 0);
    store<u32>(o + 488, load<u32>(0x01164B68u));
    for (u32 off : {504u, 508u}) store<u32>(o + off, 0);
    store<u32>(o + 512, 0x42B40000u);  // 90.0f
    for (u32 off : {1176u, 1172u, 1168u, 1192u, 1188u, 1184u, 1208u, 1204u, 1200u}) store<u32>(o + off, 0);
    store<u8>(o + 1217, 0);
    for (u32 off : {1240u, 1236u, 1232u, 1256u, 1252u, 1248u}) store<u32>(o + off, 0);
    constexpr u32 B = 0x01164BD0u;
    store<u32>(o + 1280, load<u32>(B));
    store<u32>(o + 1284, load<u32>(B + 4));
    store<u8>(o + 500, 0);
    store<u32>(o + 1288, load<u32>(B + 8));
    store<u8>(o + 501, 0);
    store<u32>(o + 1296, load<u32>(B + 16));
    store<u32>(o + 1300, load<u32>(B + 20));
    store<u8>(o + 1216, 0);
    store<u32>(o + 1304, load<u32>(B + 24));
    store<u32>(o + 1264, 0);
    store<u32>(o + 1312, load<u32>(B + 32));
    store<u32>(o + 1272, 0);
    store<u32>(o + 1316, load<u32>(B + 36));
    store<u32>(o + 1320, load<u32>(B + 40));
    store<u32>(o + 1328, load<u32>(B + 48));
    store<u32>(o + 1332, load<u32>(B + 52));
    store<u32>(o + 1420, o);
    const u32 last = load<u32>(B + 56);
    store<u32>(o + 1336, last);
    ctx.fpr[1] = LfsF(last);
    VCall(ctx, o + 1392, 28, {o + 1392, 0, 1});
    for (u32 off : {532u, 676u, 820u, 964u}) store<u32>(o + off, 0);
    store<u32>(o + 1380, 0);
    store<u8>(o + 1384, static_cast<u8>((load<u8>(o + 1384) | 0x40u) & ~0x80u));
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 1385 + k, load<u8>(0x01164CA0u + k));
    ctx.gpr[3] = o + 1385;  // r3 as left
}

namespace {
constexpr u32 Rate_Reset33 = 0x002CB098u;  // f_002CB098(o): the counter's reset
}  // namespace

// Rate counter message handler (o, msg) -> 1 unhandled / 0: for message 2,
// by state (slot 32 with 0): 0 -> SetFlag (slot 28) (0, 1) and 0 when the
// u8 +85 is set; 1 -> reset (f_002CB098), +85 = 0, the +88 object
// released (slot 40 with 0) and cleared, SetFlag (0, 2), 0; 2 -> the
// current entry's (+36 + 12 * u8 +84) +8 rate is clamped to >= K[0] (else
// the double K[8]) and when the elapsed +12 ->+0 exceeds K[16] / rate
// (divided again by +32 clamped to [K[32], K[20]] for owners whose
// +28 ->+276 +656 u8 +86 has bit 0x80 without 0x40) SetFlag (0, +85 ? 1 :
// 0). K = the table 0x002CB870.
DECOMP(002CB8A0, Rate_OnMsg_002CB8A0) {
    const u32 o = arg<u32>(ctx, 0), msg = arg<u32>(ctx, 1);
    constexpr u32 K = 0x002CB870u;
    if (static_cast<s32>(load<u32>(msg)) != 2) return ret(ctx, 1u);
    const s32 s = static_cast<s32>(VCall(ctx, o, 32, {o, 0}));
    if (s == 0) {
        if (load<u8>(o + 85) == 0) return ret(ctx, 1u);
        VCall(ctx, o, 28, {o, 0, 1});
        return ret(ctx, 0u);
    }
    if (s == 1) {
        call(ctx, Rate_Reset33, {o});
        const u32 a = load<u32>(o + 88);
        store<u8>(o + 85, 0);
        if (a != 0) {
            VCall(ctx, a, 40, {a, 0});
            store<u32>(o + 88, 0);
        }
        VCall(ctx, o, 28, {o, 0, 2});
        return ret(ctx, 0u);
    }
    if (s != 2) return ret(ctx, 1u);
    const u32 p = o + 12 * load<u8>(o + 84) + 36;
    const f64 v0 = Lf(p + 8);
    const f64 v = Sgl28(Fsel28(Sgl28(v0 - Lf(K)), v0, Ld30(K + 8)));
    store<f32>(p + 8, static_cast<f32>(v));
    f64 lim = Sgl28(Lf(K + 16) / v);
    const u32 q = load<u32>(load<u32>(o + 28) + 276) + 656;
    const f64 sc = Lf(o + 32);
    const f64 el = Lf(load<u32>(o + 12));
    ctx.fpr[1] = el;  // f1 as left
    const u32 b = load<u8>(q + 86);
    if ((b & 0x80u) != 0 && (b & 0x40u) == 0) {
        f64 c = Sgl28(Fsel28(Sgl28(Lf(K + 20) - sc), sc, Ld30(K + 24)));
        c = Sgl28(Fsel28(Sgl28(c - Lf(K + 32)), c, Ld30(K + 40)));
        lim = Sgl28(lim / c);
    }
    if (el > lim) VCall(ctx, o, 28, {o, 0, load<u8>(o + 85) != 0 ? 1u : 0u});
    ret(ctx, 1u);
}

namespace {
constexpr u32 Vec4Transform33 = 0x005539E4u;  // Vec4_Transform(v, m[16])
}  // namespace

// Trail flush (o): unless done (u16 +236 bit 0x1000): +188 = u16 +238 *
// +184; when dirty (+236 bit 0x8000) the ring of 32-byte point pairs +248
// from u16 +242 (wrapping at +238) up to +240 is transformed by the owner
// +212's matrix +80 (as a 4x4 with the last column 0, 0, 0, 1; rebuilt
// before each call) and the bit cleared. Then the +228 object's slot 8
// when +232 is set and the +232 object's slot 8 when +228 is set (sic),
// the owner's slot 8, and the done bit set. Leaves r3 / f1 as the original.
DECOMP(002F24D0, Trail_Flush_002F24D0) {
    const u32 o = arg<u32>(ctx, 0);
    u32 h = load<u16>(o + 236);
    ctx.gpr[3] = h;
    if ((h & 0x1000u) != 0) return;
    const u32 cnt = load<u16>(o + 238);
    const f64 len = Fmuls31(static_cast<f64>(cnt), Lf(o + 184));
    store<f32>(o + 188, static_cast<f32>(len));
    ctx.fpr[1] = len;
    if ((h & 0x8000u) != 0) {
        Stack frame(ctx, 336);
        const u32 m = frame.addr() + 112;
        const u32 src = load<u32>(o + 212) + 80;
        u32 w[12];
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) w[3 * r + k] = load<u32>(src + 16 * r + 4 * k);
        auto build = [&]() {
            for (u32 r = 0; r < 4; ++r) {
                for (u32 k = 0; k < 3; ++k) store<u32>(m + 16 * r + 4 * k, w[3 * r + k]);
                store<u32>(m + 16 * r + 12, r == 3 ? 0x3F800000u : 0u);
            }
        };
        s32 i = static_cast<s32>(load<u16>(o + 242));
        do {
            const u32 e = load<u32>(o + 248) + 32 * static_cast<u32>(i);
            build();
            call(ctx, Vec4Transform33, {e, m});
            build();
            call(ctx, Vec4Transform33, {load<u32>(o + 248) + 32 * static_cast<u32>(i) + 16, m});
            ++i;
            const s32 wrap = static_cast<s32>(load<u16>(o + 238)), end = static_cast<s32>(load<u16>(o + 240));
            if (i == wrap) i = 0;
            if (i == end) break;
        } while (true);
        store<u16>(o + 236, static_cast<u16>(load<u16>(o + 236) & ~0x8000u));
    }
    if (load<u32>(o + 232) != 0) {
        const u32 a = load<u32>(o + 228);
        VCall(ctx, a, 8, {a});
    }
    if (load<u32>(o + 228) != 0) {
        const u32 b = load<u32>(o + 232);
        VCall(ctx, b, 8, {b});
    }
    const u32 ow = load<u32>(o + 212);
    VCall(ctx, ow, 8, {ow});
    h = load<u16>(o + 236) | 0x1000u;
    store<u16>(o + 236, static_cast<u16>(h));
    ctx.gpr[3] = h;
}

namespace {
constexpr u32 Mat34FromRotPos33 = 0x00553B4Cu;  // Mat34_FromRotPos553B4C(out, rot rows, pos)

// divw as the simulator models the undefined cases (divisor 0, INT_MIN / -1): 0.
s32 Divw33(s32 x, s32 y) {
    if (y == 0 || (x == INT32_MIN && y == -1)) return 0;
    return x / y;
}
}  // namespace

// Waypoint mover step (o; f1 dt): idle while the wait timer +44 > K0
// (counted down by dt). Otherwise the owner +8 moves from its position
// +144 towards waypoint +64 + 16 * +56 by +32 * dt (slot 84 with the
// matrix Mat34_FromRotPos553B4C(rotation +96, new position)); on arrival
// it is placed there and, when u8 +48 is set, the index advances (kind +24
// 0xE2E902C9: wrap modulo +52; 0x026F1EA5: stop at the end) and a new wait
// +36 + (+40 - +36) * rand * K[2] is drawn. Index -1 = stopped. K = the
// f32 table 0x002BA220. Leaves r3 / f1 as the original does.
DECOMP(002BA230, Waypoint_Step_002BA230) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 K = 0x002BA220u;
    const s32 idx = static_cast<s32>(load<u32>(o + 56));
    ctx.gpr[3] = static_cast<u32>(idx);
    if (idx == -1) return;
    const f64 t = Lf(o + 44), k0 = Lf(K);
    if (t > k0) {
        const f64 n = Sgl28(t - dt);
        store<f32>(o + 44, static_cast<f32>(n));
        ctx.fpr[1] = n;
        return;
    }
    Stack frame(ctx, 384);
    const u32 sp = frame.addr();
    const u32 p = o + (static_cast<u32>(idx) << 4) + 64;
    const u32 b = load<u32>(o + 8);
    const f64 bx = Lf(b + 144), by = Lf(b + 148), bz = Lf(b + 152);
    const f64 dy = Sgl28(Lf(p + 4) - by), dx = Sgl28(Lf(p) - bx), dz = Sgl28(Lf(p + 8) - bz);
    const f64 step = Fmuls31(Lf(o + 32), dt);
    f64 l = Fmadds(dx, dx, Fmuls31(dy, dy));
    l = Sgl28(std::sqrt(Fmadds(dz, dz, l)));
    auto rot = [&](u32 d) {
        u32 w = 0;
        for (u32 r = 0; r < 3; ++r)
            for (u32 k = 0; k < 12; k += 4) {
                w = load<u32>(b + 96 + 16 * r + k);
                store<u32>(d + 16 * r + k, w);
            }
        ctx.fpr[1] = LfsF(w);
    };
    if (l > k0 && l > step) {
        const f64 s = Sgl28(Lf(K + 4) / l);
        const u32 pos = sp + 112, rm = sp + 176, out = sp + 288;
        store<f32>(pos, static_cast<f32>(Fmadds(Fmuls31(s, dx), step, bx)));
        store<f32>(pos + 4, static_cast<f32>(Fmadds(Fmuls31(s, dy), step, by)));
        store<f32>(pos + 8, static_cast<f32>(Fmadds(Fmuls31(s, dz), step, bz)));
        rot(rm);
        call(ctx, Mat34FromRotPos33, {out, rm, pos});
        VCall(ctx, b, 84, {b, out});
        return;
    }
    const u32 rm = sp + 128, out = sp + 224;
    rot(rm);
    call(ctx, Mat34FromRotPos33, {out, rm, p});
    VCall(ctx, b, 84, {b, out});
    const u32 flag = load<u8>(o + 48);
    const f64 lo = Lf(o + 36), hi = Lf(o + 40);
    ctx.fpr[1] = hi;
    ctx.gpr[3] = flag;
    if (flag == 0) return;
    const s32 kind = static_cast<s32>(load<u32>(o + 24));
    if (kind > static_cast<s32>(0xE2E902C9u)) {
        if (kind == 0x026F1EA5) {
            const s32 n = static_cast<s32>(load<u32>(o + 56) + 1u);
            if (n < static_cast<s32>(load<u32>(o + 52))) store<u32>(o + 56, static_cast<u32>(n));
        }
    } else if (kind > static_cast<s32>(0xE2E902C8u)) {
        const s32 n = static_cast<s32>(load<u32>(o + 56) + 1u), m = static_cast<s32>(load<u32>(o + 52));
        store<u32>(o + 56, static_cast<u32>(n - Divw33(n, m) * m));
    }
    const f64 d = Sgl28(hi - lo);
    const f64 r = Sgl28(static_cast<f64>(static_cast<s64>(RandStep33())));
    const f64 w = Fmadds(d, Fmuls31(r, Lf(K + 8)), lo);
    store<f32>(o + 44, static_cast<f32>(w));
    ctx.fpr[1] = w;
    ctx.gpr[3] = 0x01160000u;
}

namespace {
constexpr u32 XmlDefFromRec33 = 0x00226A28u;  // XmlDef_FromRec226A28(o, src): the base copy
}  // namespace

// XmlDef copy constructor (o, src) (XmlDef_New245A30): the base part, the
// vtable 0x00B1BC00, +780 = 0, the fields +784..+1036 from src
// +720..+972 (offset -64; raw words / bytes in the original's order),
// the name +745 = String_CopyN(src +681, 32) and three u32[4] tables
// +824 / +840 / +856 from src +760 / +776 / +792.
DECOMP(00244980, XmlDef_Copy_00244980) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    call(ctx, XmlDefFromRec33, {o, s});
    // {dst, src} words (src 0 with dst 780 = clear; dst 0 = the vtable), then bytes marked by a set bit 0x10000
    static constexpr u32 kSeq[][2] = {
        {784, 720}, {800, 736}, {804, 740}, {808, 744}, {816, 752}, {820, 756}, {884, 820}, {888, 824},
        {892, 828}, {896, 832}, {900, 836}, {904, 840}, {908, 844}, {916, 852}, {920, 856}, {924, 860},
        {928, 864}, {932, 868}, {936, 872}, {940, 876}, {872, 808}, {944, 880}, {948, 884}, {0, 0},
        {876, 812}, {780, 0}, {952, 888}, {880, 816}, {956, 892}, {960, 896}, {912 | 0x10000, 848}, {964, 900},
        {968, 904}, {972, 908}, {976, 912}, {992, 928}, {996, 932}, {1000, 936}, {1008, 944}, {1020, 956},
        {1012, 948}, {1016 | 0x10000, 952}, {1028, 964}, {1024, 960}, {1032, 968}, {1036 | 0x10000, 972}};
    for (const auto& e : kSeq) {
        const u32 d = e[0] & 0xFFFF;
        if ((e[0] & 0x10000u) != 0) store<u8>(o + d, load<u8>(s + e[1]));
        else if (d == 0) store<u32>(o, 0x00B1BC00u);
        else if (d == 780) store<u32>(o + 780, 0);
        else store<u32>(o + d, load<u32>(s + e[1]));
    }
    call(ctx, StrCopyN33, {o + 745, s + 681, 32});
    for (u32 k = 0; k < 16; k += 4) {
        store<u32>(o + 824 + k, load<u32>(s + 760 + k));
        store<u32>(o + 840 + k, load<u32>(s + 776 + k));
        store<u32>(o + 856 + k, load<u32>(s + 792 + k));
    }
    ctx.fpr[1] = Lf(s + 772);  // f1 / r3 as left
    ctx.gpr[3] = s + 792;
}
