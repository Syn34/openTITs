// Game code, large-function sweep part 13: the player's rig-part tuning
// helpers (callees of Player_Setup232F00 / Player_Setup231470).
#include "big13.h"
#include "big14.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
inline be_u32& W(u32 base, u32 off) { return gm<be_u32>(base + off); }
inline f64 F(u32 ea) { return LfsW(gm<be_u32>(ea)); }
inline void St(u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); }
inline void CopyW(u32 dst, u32 src) { gm<be_u32>(dst) = gm<be_u32>(src).get(); }
constexpr u32 HkArray_Reserve13 = 0x007CCE88;   // (allocator, hkArray*, s32 n, s32 elem size)
constexpr u32 MapE_Find = 0x0075C1DC;           // (map, const u32* key) -> slot*
constexpr u32 EmitterInstance_New = 0x005A3FFC; // (def, bool) -> instance
constexpr u32 Def_SetName = 0x00584BDC;         // (def, name, bool hash)
constexpr u32 Hk_Curve9FDF0C = 0x009FDF0C;      // (f1..f4) -> f1 (reads only f1..f4) (name?)
// The AltiVec / PPC NaN rules: an operand NaN comes out quieted (a, else
// b, else c), a NaN the operation itself makes is the positive default
// NaN (x86 makes a negative one).
inline f32 QNaN32(f32 x) { return std::bit_cast<f32>(std::bit_cast<u32>(x) | 0x00400000u); }
inline f32 VmaddP(f32 a, f32 c, f32 b) {
    if (a != a) return QNaN32(a);
    if (b != b) return QNaN32(b);
    if (c != c) return QNaN32(c);
    const f32 r = std::fma(a, c, b);
    return r != r ? std::bit_cast<f32>(0x7FC00000u) : r;
}
inline V4 Row3P(const V4& c0, const V4& c1, const V4& c2, f32 s0, f32 s1, f32 s2) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = VmaddP(c2.e[k], s2, VmaddP(c1.e[k], s1, VmaddP(c0.e[k], s0, 0.0f)));
    return o;
}
inline V4 Add4P(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        const f32 x = a.e[k], y = b.e[k];
        const f32 r = x + y;
        o.e[k] = x != x ? QNaN32(x) : y != y ? QNaN32(y) : r != r ? std::bit_cast<f32>(0x7FC00000u) : r;
    }
    return o;
}
// fadds with the PPC default NaN for a NaN the add makes.
inline f64 AddsP(f64 a, f64 b) {
    const f64 r = Adds(a, b);
    return (r != r && a == a && b == b) ? std::bit_cast<f64>(0x7FF8000000000000ull) : r;
}
// hkArray {+0 data, +4 s32 size, +8 u32 capacity | flags (top two bits)}:
// grows to max(2 * capacity, n) when the capacity (low 30 bits) is below n
// (signed compares; a negative n never grows). The size is not written.
inline void HkReserve(u32 arr, s32 n, u32 elem) {
    const u32 cap = gm<be_u32>(arr + 8);
    if (static_cast<s32>(cap & 0x3FFFFFFFu) >= n) return;
    const s32 dbl = static_cast<s32>((cap << 1) & 0x7FFFFFFEu);
    call(HkArray_Reserve13, kHkArrayAlloc, arr, dbl > n ? dbl : n, elem);
}
// The player tuning's link groups (tuning +1072: +132 the 32-byte groups,
// +140 the u16 group count; each group {+0 the 352-byte items, +8 u16
// count}): f(g, i, item) for every item j of every group g, with i = g *
// group count + j (the rig parts keep a square table, one row per group).
// The counts and pointers are re-read as the original does.
template <typename Fn>
inline void ForLinkItems(u32 links, Fn f) {
    const u32 ng = gm<be_u16>(links + 140);
    for (u32 g = 0; g < ng; ++g) {
        const u32 grp = gm<be_u32>(links + 132) + 32 * g;
        for (u32 j = 0; j < gm<be_u16>(grp + 8); ++j) f(g, g * ng + j, gm<be_u32>(grp) + 352 * j);
    }
}
// {w(a), w(b), w(c), 0} stored as the original's vsel / vandc merge of
// three splatted words to the 16-byte block holding dst.
inline void StoreVec3W(u32 dst, u32 wa, u32 wb, u32 wc) {
    Quad& q = gm<Quad>(dst & ~15u);
    q.w[0] = wa;
    q.w[1] = wb;
    q.w[2] = wc;
    q.w[3] = 0u;
}
}  // namespace

namespace {
constexpr u32 kTuneLinks = 1072;   // tuning (p->396) +: the link groups (see ForLinkItems)
}  // namespace

// u32 Player_TunePart232B60(Player* p, const Tier* a, Part* part) (a = the
// world tier block +4; name?): copies a's 16 bytes at +16 to part +16, the
// tuning vectors {+1088, +1092, +1096} / {+1104, +1108, +1112} and the
// negated {+1072, +1076, +1080} to part +48 / +64 / +80, and eleven more
// tuning floats and a +32 byte. part's 40-byte array +140 gets (s8) a->32
// entries and its byte array +152 one count per link group; every link
// item fills a 40-byte entry (row = group) and the group's item count goes
// to the byte array. Returns the group count (or, with none, part +160).
void Player_TunePart232B60(u32 p, u32 a, u32 part) {
    const u32 u = gm<be_u32>(p + 396) + kTuneLinks;
    {   // lvx / stvx: the aligned 16-byte blocks
        const u32 s = (a + 16) & ~15u, d = (part + 16) & ~15u;
        for (u32 k = 0; k < 16; k += 4) gm<be_u32>(d + k) = gm<be_u32>(s + k);
    }
    const u32 nx = gm<be_u32>(u + 0) ^ 0x80000000u, ny = gm<be_u32>(u + 4) ^ 0x80000000u, nz = gm<be_u32>(u + 8) ^ 0x80000000u;  // fneg
    const u32 w152 = gm<be_u32>(u + 152);
    StoreVec3W(part + 48, gm<be_u32>(u + 16), gm<be_u32>(u + 20), gm<be_u32>(u + 24));
    StoreVec3W(part + 64, gm<be_u32>(u + 32), gm<be_u32>(u + 36), gm<be_u32>(u + 40));
    StoreVec3W(part + 80, nx, ny, nz);
    CopyW(part + 128, u + 88);
    CopyW(part + 96, u + 48);
    CopyW(part + 100, u + 52);
    CopyW(part + 104, u + 56);
    CopyW(part + 116, u + 76);
    CopyW(part + 120, u + 80);
    CopyW(part + 124, u + 84);
    CopyW(part + 108, u + 68);
    CopyW(part + 112, u + 72);
    CopyW(part + 136, u + 96);
    const u32 w92 = gm<be_u32>(u + 92);
    gm<u8>(part + 32) = static_cast<u8>(w152);
    gm<be_u32>(part + 132) = w92;

    const s32 n = static_cast<s8>(w152);
    HkReserve(part + 140, n, 40);
    gm<be_u32>(part + 144) = static_cast<u32>(n);
    const s32 ng = gm<be_u16>(u + 140);
    HkReserve(part + 152, ng, 1);
    gm<be_u32>(part + 156) = static_cast<u32>(ng);
    if (ng > 0) {
        for (s32 g = 0; g < ng; ++g) {
            const u32 grp = gm<be_u32>(u + 132) + 32 * static_cast<u32>(g);
            const u32 c = gm<be_u16>(grp + 8);
            for (u32 j = 0; j < c; ++j) {
                const u32 it = gm<be_u32>(grp) + 352 * j;
                const u32 e = gm<be_u32>(part + 140) + 40 * (static_cast<u32>(g * ng) + j);
                gm<u8>(e + 36) = static_cast<u8>(g);
                CopyW(e + 12, it + 108);
                CopyW(e + 24, it + 124);
                CopyW(e + 0, it + 96);
                CopyW(e + 8, it + 104);
                CopyW(e + 4, it + 100);
                CopyW(e + 16, it + 116);
                CopyW(e + 20, it + 120);
                CopyW(e + 28, it + 128);
                const u32 w = gm<be_u32>(it + 132);
                gm<be_u32>(e + 32) = w;
            }
            gm<u8>(gm<be_u32>(part + 152) + static_cast<u32>(g)) = static_cast<u8>(c);
        }
    }
}

// u32 Player_TunePart2329DC(Player* p, const Tier* a, Part* part) (name?):
// part's byte array +16 gets (s8) a->32 entries; tuning +1264 / +1268 to
// part +8 / +12; each link item's +164 byte to the square table. Returns
// the group count.
void Player_TunePart2329DC(u32 p, u32 a, u32 part) {
    const u32 t = gm<be_u32>(p + 396);
    const s32 n = static_cast<s8>(gm<u8>(a + 32));
    HkReserve(part + 16, n, 1);
    gm<be_u32>(part + 20) = static_cast<u32>(n);
    CopyW(part + 8, t + 1264);
    const u32 w = gm<be_u32>(t + 1268);
    gm<be_u32>(part + 12) = w;
    ForLinkItems(t + kTuneLinks, [&](u32, u32 i, u32 it) {
        gm<u8>(gm<be_u32>(part + 16) + i) = gm<u8>(it + 164);
    });
}

// u32 Player_TunePart2327A8(Player* p, const Tier* a, Part* part) (name?):
// tuning +1376..+1388 to part +8/+12/+20/+24; part's float array +28 gets
// the tuning's u16 +1400 keys (from the array at +1392) and its array +40
// (s8) a->32 entries; part +16 = 0x009FDF0C(+1324, +1228, +1280, the last
// key (unchecked: no keys reads the word before the array)); each link
// item's +168 float to the square table at +40. Returns the group count.
void Player_TunePart2327A8(u32 p, u32 a, u32 part) {
    const u32 t = gm<be_u32>(p + 396);
    const u32 k = t + 1376;
    CopyW(part + 8, k + 0);
    CopyW(part + 12, k + 4);
    CopyW(part + 20, k + 8);
    const u32 nk = gm<be_u16>(k + 24);
    const u32 w12 = gm<be_u32>(k + 12);
    gm<be_u32>(part + 24) = w12;
    HkReserve(part + 28, static_cast<s32>(nk), 4);
    gm<be_u32>(part + 32) = nk;
    const s32 n = static_cast<s8>(gm<u8>(a + 32));
    HkReserve(part + 40, n, 4);
    gm<be_u32>(part + 44) = static_cast<u32>(n);
    for (u32 j = 0; j < nk; ++j) {
        const u32 w = gm<be_u32>(gm<be_u32>(k + 16) + 4 * j);
        gm<be_u32>(gm<be_u32>(part + 28) + 4 * j) = w;
    }
    const u32 last = gm<be_u32>(part + 28) + 4 * (nk - 1);
    const f64 r = call<f64>(Hk_Curve9FDF0C, F(t + 1272 + 52), F(t + kTuneLinks + 156), F(t + 1272 + 8), F(last));
    St(part + 16, r);
    ForLinkItems(t + kTuneLinks, [&](u32, u32 i, u32 it) {
        const u32 w = gm<be_u32>(it + 168);
        gm<be_u32>(gm<be_u32>(part + 40) + 4 * i) = w;
    });
}

// u32* Player_TunePart232650(Player* p, const Tier* a, Part* part) (name?):
// tuning +1412 to part +20; part's 12-byte array +8 gets (s8) a->32
// entries, each link item's {+172, +176 floats, +180 byte} in the square
// table. Returns the array's data.
void Player_TunePart232650(u32 p, u32 a, u32 part) {
    const u32 t = gm<be_u32>(p + 396);
    const u32 w = gm<be_u32>(t + 1412);
    gm<be_u32>(part + 20) = w;
    const s32 n = static_cast<s8>(gm<u8>(a + 32));
    HkReserve(part + 8, n, 12);
    const u32 data = gm<be_u32>(part + 8);
    gm<be_u32>(part + 12) = static_cast<u32>(n);
    ForLinkItems(t + kTuneLinks, [&](u32, u32 i, u32 it) {
        const u32 d = data + 12 * i;
        CopyW(d + 0, it + 172);
        gm<u8>(d + 8) = gm<u8>(it + 180);
        const u32 x = gm<be_u32>(it + 176);
        gm<be_u32>(d + 4) = x;
    });
}

// u32* Player_TunePart232360(Player* p, const Tier* a, Part* part) (name?):
// part's 48-byte array +8 and 12-byte array +20 each get (s8) a->32
// entries; each link item fills the square tables: {+192, +196, +200, 0},
// {+208, +212, +216, 0} and +224 in the 48-byte entry, +228..+236 in the
// 12-byte one. Returns the 48-byte array's data.
void Player_TunePart232360(u32 p, u32 a, u32 part) {
    const u32 t = gm<be_u32>(p + 396);
    s32 n = static_cast<s8>(gm<u8>(a + 32));
    HkReserve(part + 8, n, 48);
    gm<be_u32>(part + 12) = static_cast<u32>(n);
    n = static_cast<s8>(gm<u8>(a + 32));
    HkReserve(part + 20, n, 12);
    const u32 d48 = gm<be_u32>(part + 8);
    const u32 d12 = gm<be_u32>(part + 20);
    gm<be_u32>(part + 24) = static_cast<u32>(n);
    ForLinkItems(t + kTuneLinks, [&](u32, u32 i, u32 it) {
        const u32 e = d48 + 48 * i, f = d12 + 12 * i;
        CopyW(e + 32, it + 224);
        CopyW(f + 0, it + 228);
        CopyW(f + 4, it + 232);
        const u32 w = gm<be_u32>(it + 236);
        gm<be_u32>(f + 8) = w;
        StoreVec3W(e + 0, gm<be_u32>(it + 192), gm<be_u32>(it + 196), gm<be_u32>(it + 200));
        StoreVec3W(e + 16, gm<be_u32>(it + 208), gm<be_u32>(it + 212), gm<be_u32>(it + 216));
    });
}

// u32 Player_InitLinkPart231208(-, PtrVector* out, const Src* src) (r3
// unread; name?): for each 64-byte entry e of src (+0 data, +8 u16 count)
// looks e +32 up in the emitter-def map; when found, a clone instance
// (EmitterInstance_New(def, 1)) whose def is renamed "<name>_clone" and
// started (vfunc +40 (1, 1)); a def with a +112 controller takes e +56 /
// +60 when not below the .text f32 at 0x00231204 (else the controller's
// +16 / its +108's +8) and those two are zeroed. Appends {e, def, inst,
// f0, f1} (20 bytes) to out, raising its u16 high-water mark +10; a miss
// appends {e, 0, 0, k, k} with k the .text f32 at 0x00231200. Returns
// src's count.
void Player_InitLinkPart231208(u32, u32 out, u32 src) {
    constexpr u32 kDefault = 0x00231200u, kMin = 0x00231204u;
    constexpr u32 kFmtClone = 0x00AD3CF0u;   // "%s_clone"
    constexpr u32 kEmitterDefs = 0x00F8F138u;   // holder* (+16 the name map)
    const u32 wDefault = gm<be_u32>(kDefault);
    const f64 fMin = F(kMin);
    GuestFrame frame(16);
    const u32 key = frame.addr();   // the original's sp + 112
    for (u32 i = 0; i < gm<be_u16>(src + 8); ++i) {
        const u32 e = gm<be_u32>(src) + 64 * i;
        u32 def = 0, inst = 0, w0 = wDefault, w1 = wDefault;
        gm<be_u32>(key) = gm<be_u32>(e + 32);
        const u32 slot = call<u32>(MapE_Find, gm<be_u32>(gm<be_u32>(kEmitterDefs) + 16), key);
        const u32 found = gm<be_u32>(slot);
        if (found != 0) {
            inst = call<u32>(EmitterInstance_New, found, 1);
            def = gm<be_u32>(inst + 208);
            const u32 name = vcall<u32>(host<u8>(def), 12);
            const u32 s = call<u32>(fn::String_Format, kFmtClone, name);
            call(Def_SetName, def, s, 1);
            vcall(host<u8>(inst), 40, 1, 1);
            const u32 c = gm<be_u32>(def + 112);
            if (c != 0) {
                w0 = gm<be_u32>(e + 56);
                if (!(F(e + 56) >= fMin)) w0 = gm<be_u32>(c + 16);
                gm<be_u32>(c + 16) = 0;
                w1 = gm<be_u32>(e + 60);
                if (!(F(e + 60) >= fMin)) w1 = gm<be_u32>(gm<be_u32>(c + 108) + 8);
                gm<be_u32>(gm<be_u32>(c + 108) + 8) = 0;
            }
        }
        u32 n = gm<be_u16>(out + 8);
        if (n == gm<be_u16>(out + 4)) {
            call(fn::PtrVector_Reserve, out, (n << 1) & 0x1FFFEu);
            n = gm<be_u16>(out + 8);
        }
        const u32 d = gm<be_u32>(out) + 20 * n;
        const u32 hi = gm<be_u16>(out + 10);
        gm<be_u32>(d + 0) = e;
        gm<be_u32>(d + 4) = def;
        gm<be_u32>(d + 8) = inst;
        gm<be_u32>(d + 12) = w0;
        gm<be_u32>(d + 16) = w1;
        const u32 n1 = gm<be_u16>(out + 8) + 1;
        gm<be_u16>(out + 8) = static_cast<u16>(n1);
        gm<be_u16>(out + 10) = static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF) ? hi : n1);
    }
}

namespace {
namespace k371 {
constexpr u32 kConst = 0x002371E4u;   // .text f32: +0 1e-5, +4 0.5, +8 1.0, +12 0.0, +16 pi/2
constexpr u32 kZero = 0x01164B40u;    // vec3 (0, 0, 0)
constexpr u32 kOne = 0x01164B30u;     // vec3 (1, 1, 1)
constexpr u32 kUp = 0x01164B60u;      // vec3 up axis
constexpr u32 kLinkCallback = 0x00F34C30u;   // OPD passed to World_AddTimedCallback
constexpr u32 Vec3_Copy = 0x00734D90;
constexpr u32 Mat43_Copy = 0x002370D0;
constexpr u32 WorldObj_UpdateLod = 0x0057EE00;
constexpr u32 WorldObj_MarkDirty = 0x0057D000;
constexpr u32 Hk_ToMatrix7CEBD8 = 0x007CEBD8;       // (Mat44* out, const in[16 bytes]) (name?)
constexpr u32 Mat44_MulInPlace75D154 = 0x0075D154;  // (Mat44* m, const Mat44* by) (name?)
constexpr u32 Mat34_FromRotPos = 0x00553B4C;
constexpr u32 Quat_Multiply = 0x005556A4;
constexpr u32 Quat_ToMatrix = 0x00553A90;
constexpr u32 Math_Sinf = 0x007345F0;
constexpr u32 Math_Cosf = 0x007344E0;
constexpr u32 World_AddTimedCallback = 0x0046B5F8;
}  // namespace k371

inline f32 Fsqrts13(f32 x) { return static_cast<f32>(std::sqrt(static_cast<f64>(x))); }
inline f32 WordF13(u32 w) { return std::bit_cast<f32>(w); }

// The quaternion {axis * sin(angle), cos(angle)} of an axis normalised by
// 1 / sqrt(y*y + x*x + z*z) (fused, single precision), written to q;
// sin / cos are the game's Math_Sinf / Math_Cosf (f1 is what Math_Cosf
// leaves).
void AxisQuat(u32 q, f32 x, f32 y, f32 z, f32 one, f32 angle) {
    const f32 inv = Divs(one, Fsqrts13(Fmadds(z, z, Fmadds(x, x, Muls(y, y)))));
    const f32 ax = Muls(x, inv), ay = Muls(y, inv), az = Muls(z, inv);
    const f64 s = call<f64>(k371::Math_Sinf, static_cast<f64>(angle));
    const f32 qx = Muls(s, ax), qy = Muls(s, ay), qz = Muls(s, az);
    const f64 c = call<f64>(k371::Math_Cosf, static_cast<f64>(angle));
    gm<be_f32>(q + 0) = qx;
    gm<be_f32>(q + 4) = qy;
    gm<be_f32>(q + 8) = qz;
    St(q + 12, c);
}
}  // namespace

// u32 Player_SetRate2371F8(Player* p; f32 dt) (name?: it poses the
// player's link chains). Nothing without the rig p->612. Copies the rig
// body's matrix (+24 +240) to p +96 and its position (+432) to p +560 (with
// dt above 1e-5 and neither flag 0x40 nor 0x20 of p->172, the lag between
// the old and new position goes to the offset (sp +112) and p +144); with
// p->736 that object gets p's matrix moved to the tuning's +944 point;
// drives the main link (p +716) at the rig's rate (vfunc +60) / tuning
// +1324; tells p->740 (vfunc +40) whether the rig is held (rig->156 +16 /
// +17) and stops when vfunc +32 (p, 0) returns 1. Then, per link group
// (tuning +1072): each item's 128-byte link entry (p->676) is posed from
// the rig's bone transform (vfunc +48), rotated by the item's three
// half-angles (+0/+4/+8, optionally a quarter-turn twist with +40) and
// offset (+16), into its object (+100) and matrix (+120; marks the chain
// dirty); its five sub-links are driven by the bone's +192 / +216
// weights. The group's object (p->696) is placed between the first and
// last items (a quarter-turn twist with group +28). A dirty chain queues
// the world callback 0x00F34C30(p). Returns 0, 1 (stopped) or the
// callback queue's result.
void Player_SetRate2371F8(u32 p, f64 dt) {
    using namespace k371;
    if (gm<be_u32>(p + 612) == 0) return;
    const f32 kHalf = F(kConst + 4), kOneF = F(kConst + 8), kZeroF = F(kConst + 12), kQuarter = F(kConst + 16);
    GuestFrame frame(1392);
    const u32 sp = frame.addr();   // the original's frame, same offsets
    call(Vec3_Copy, sp + 112, kZero);
    bool dirty = false;
    if (gm<be_u32>(gm<be_u32>(p + 612) + 24) != 0) {
        call(Mat43_Copy, sp + 352, gm<be_u32>(gm<be_u32>(p + 612) + 24) + 240);
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(p + 96 + 16 * r + 4 * k, sp + 352 + 16 * r + 4 * k);
        const u32 b = gm<be_u32>(gm<be_u32>(p + 612) + 24);
        const u32 w1 = gm<be_u32>(b + 432), w2 = gm<be_u32>(b + 436), w3 = gm<be_u32>(b + 440);
        const u32 fl = gm<be_u32>(p + 172);
        if (dt > static_cast<f64>(F(kConst)) && (fl & 0x40u) == 0 && (fl & 0x20u) == 0) {
            // The lag: (pos + 0.5 * (pos - old)) * dt + p +544, less the body's +288.
            const f32 x = WordF13(w1), y = WordF13(w2), z = WordF13(w3);
            f32 dx = Subs(x, F(p + 560)), dy = Subs(y, F(p + 564)), dz = Subs(z, F(p + 568));
            dx = Fmadds(kHalf, dx, x);
            dy = Fmadds(kHalf, dy, y);
            dz = Fmadds(kHalf, dz, z);
            dx = Subs(Fmadds(dx, dt, F(p + 544)), F(b + 288));
            dy = Subs(Fmadds(dy, dt, F(p + 548)), F(b + 292));
            dz = Subs(Fmadds(dz, dt, F(p + 552)), F(b + 296));
            gm<be_f32>(sp + 112) = dx;
            gm<be_f32>(sp + 116) = dy;
            gm<be_f32>(sp + 120) = dz;
            gm<be_f32>(p + 144) = AddsP(F(p + 144), dx);
            gm<be_f32>(p + 148) = AddsP(F(p + 148), dy);
            gm<be_f32>(p + 152) = AddsP(F(p + 152), dz);
        }
        gm<be_u32>(p + 560) = w1;
        gm<be_u32>(p + 564) = w2;
        gm<be_u32>(p + 568) = w3;
        const u32 lod = gm<be_u32>(p + 160);
        if (lod != 0) call(WorldObj_UpdateLod, lod, 0, 0);
    }

    const u32 t = gm<be_u32>(p + 396);
    const u32 links = t + 1072;
    if (gm<be_u32>(p + 736) != 0) {
        // p's matrix with its translation moved to the tuning's +944 point.
        const u32 m = t + 944;
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(sp + 416 + 16 * r + 4 * k, p + 96 + 16 * r + 4 * k);
        CopyW(sp + 128, m + 48);
        CopyW(sp + 132, m + 52);
        CopyW(sp + 136, m + 56);
        const V4 v = Add4P(Lvx(host<u8>(sp + 464)), Row3P(Lvx(host<u8>(sp + 416)), Lvx(host<u8>(sp + 432)), Lvx(host<u8>(sp + 448)), F(m + 48), F(m + 52), F(m + 56)));
        Stvx(host<u8>(sp + 128), v);
        CopyW(sp + 464, sp + 128);
        CopyW(sp + 468, sp + 132);
        const u32 wz = gm<be_u32>(sp + 136);
        gm<be_u32>(sp + 472) = wz;
        Obj_SetMatrixD87CC(gm<be_u32>(p + 736), sp + 416);
    }
    u32 rig = gm<be_u32>(p + 612);
    const f32 rate = Divs(vcall<f64>(host<u8>(rig), 60), F(t + 1272 + 52));
    Player_DriveLink236DB8(p, p + 716, p + 96, 0, 1, rate);
    const u32 st = gm<be_u32>(gm<be_u32>(p + 612) + 156);
    const u32 held = (gm<u8>(st + 17) != 0 || gm<u8>(st + 16) != 0) ? 1u : 0u;
    const u32 o740 = gm<be_u32>(p + 740);
    if (o740 != 0) vcall(host<u8>(o740), 40, held, 1);
    if (vcall<u32>(host<u8>(p), 32, 0u) == 1) return;
    const u32 ng = gm<be_u16>(links + 140);
    if (ng == 0) return;

    for (u32 g = 0; g < ng; ++g) {
        // B = the first item's bone position, A = the last one's.
        u32 bx = gm<be_u32>(kZero), by = gm<be_u32>(kZero + 4), bz = gm<be_u32>(kZero + 8);
        u32 ax = gm<be_u32>(kOne), ay = gm<be_u32>(kOne + 4), az = gm<be_u32>(kOne + 8);
        const u32 grp = gm<be_u32>(links + 132) + 32 * g;
        const u32 n = gm<be_u16>(grp + 8);
        for (u32 j = 0; j < n; ++j) {
            const u32 idx = g * ng + j;
            const u32 e = gm<be_u32>(p + 676) + 128 * idx;
            const u32 it = gm<be_u32>(grp) + 352 * j;
            const u32 o2 = gm<be_u32>(e + 100);
            if (o2 == 0 && gm<be_u32>(e + 120) == 0) continue;
            rig = gm<be_u32>(p + 612);
            vcall(host<u8>(rig), 48, gm<be_u32>(rig + 24), gm<be_u32>(rig + 52), static_cast<u64>(static_cast<s64>(static_cast<s32>(idx))), sp + 144, sp + 160);
            for (u32 k = 0; k < 16; k += 4) CopyW(sp + 528 + k, sp + 144 + k);
            call(Hk_ToMatrix7CEBD8, sp + 480, sp + 160);
            call(Mat43_Copy, sp + 544, sp + 480);

            // M (sp +608): row 0 the bone's x axis, row 2 = x axis x up
            // (normalised when not zero), row 1 = row 2 x row 0, row 3 the
            // bone's position.
            f32 x0 = F(sp + 544), y0 = F(sp + 548), z0 = F(sp + 552);
            const f32 ux = F(kUp), uy = F(kUp + 4), uz = F(kUp + 8);
            f32 cx = Fmsubs(y0, uz, Muls(z0, uy));
            f32 cy = Fmsubs(z0, ux, Muls(x0, uz));
            f32 cz = Fmsubs(x0, uy, Muls(y0, ux));
            CopyW(sp + 608, sp + 544);
            CopyW(sp + 612, sp + 548);
            CopyW(sp + 616, sp + 552);
            CopyW(sp + 656, sp + 592);
            CopyW(sp + 660, sp + 596);
            CopyW(sp + 664, sp + 600);
            const f32 c2 = Fmadds(cz, cz, Fmadds(cx, cx, Muls(cy, cy)));
            if (c2 > kZeroF) {
                const f32 inv = Divs(kOneF, Fsqrts13(c2));
                cx = Muls(cx, inv);
                cy = Muls(cy, inv);
                cz = Muls(cz, inv);
            }
            gm<be_f32>(sp + 640) = cx;
            gm<be_f32>(sp + 644) = cy;
            gm<be_f32>(sp + 648) = cz;
            const f32 nx = Fmsubs(cy, z0, Muls(cz, y0));
            const f32 ny = Fmsubs(cz, x0, Muls(cx, z0));
            const f32 nz = Fmsubs(cx, y0, Muls(cy, x0));
            gm<be_f32>(sp + 624) = nx;
            gm<be_f32>(sp + 628) = ny;
            gm<be_f32>(sp + 632) = nz;
            if (gm<u8>(it + 40) != 0) {   // a quarter-turn twist about row 1
                AxisQuat(sp + 272, nx, ny, nz, kOneF, kQuarter);
                call(Quat_ToMatrix, sp + 800, sp + 272, kZero);
                call(Mat44_MulInPlace75D154, sp + 608, sp + 800);
                x0 = F(sp + 608);
                y0 = F(sp + 612);
                z0 = F(sp + 616);
            }
            // Rotations by the item's half-angles about M's rows 0, 1, 2.
            AxisQuat(sp + 208, x0, y0, z0, kOneF, Muls(F(it + 0), kHalf));
            AxisQuat(sp + 224, F(sp + 624), F(sp + 628), F(sp + 632), kOneF, Muls(F(it + 4), kHalf));
            AxisQuat(sp + 240, F(sp + 640), F(sp + 644), F(sp + 648), kOneF, Muls(F(it + 8), kHalf));
            // The item's offset (+16) through p's rotation (rows +96/+112/+128).
            CopyW(sp + 256, it + 16);
            CopyW(sp + 260, it + 20);
            CopyW(sp + 264, it + 24);
            Stvx(host<u8>(sp + 256), Row3P(Lvx(host<u8>(p + 96)), Lvx(host<u8>(p + 112)), Lvx(host<u8>(p + 128)), F(it + 16), F(it + 20), F(it + 24)));
            for (u32 r = 0; r < 3; ++r)
                for (u32 k = 0; k < 3; ++k) CopyW(sp + 304 + 16 * r + 4 * k, sp + 544 + 16 * r + 4 * k);
            call(Mat34_FromRotPos, sp + 736, sp + 304, kZero);
            call(Quat_Multiply, sp + 192, sp + 208, sp + 224);
            call(Quat_Multiply, sp + 176, sp + 192, sp + 240);
            call(Quat_ToMatrix, sp + 672, sp + 176, kZero);
            call(Mat44_MulInPlace75D154, sp + 736, sp + 672);
            f32 px = AddsP(F(sp + 592), F(sp + 256));
            f32 py = AddsP(F(sp + 596), F(sp + 260));
            f32 pz = AddsP(F(sp + 600), F(sp + 264));
            u32 rot[9];
            for (u32 r = 0; r < 3; ++r)
                for (u32 k = 0; k < 3; ++k) rot[3 * r + k] = gm<be_u32>(sp + 736 + 16 * r + 4 * k);
            gm<be_f32>(sp + 784) = px;
            gm<be_f32>(sp + 788) = py;
            gm<be_f32>(sp + 792) = pz;
            px = AddsP(px, F(sp + 112));
            py = AddsP(py, F(sp + 116));
            pz = AddsP(pz, F(sp + 120));
            if (o2 != 0) {
                for (u32 r = 0; r < 3; ++r)
                    for (u32 k = 0; k < 3; ++k) gm<be_u32>(o2 + 16 + 16 * r + 4 * k) = rot[3 * r + k];
                gm<be_f32>(o2 + 64) = px;
                gm<be_f32>(o2 + 68) = py;
                gm<be_f32>(o2 + 72) = pz;
                call(WorldObj_MarkDirty, o2);
            }
            const u32 m120 = gm<be_u32>(e + 120);
            if (m120 != 0) {
                for (u32 r = 0; r < 3; ++r)
                    for (u32 k = 0; k < 3; ++k) gm<be_u32>(m120 + 16 * r + 4 * k) = rot[3 * r + k];
                gm<be_f32>(m120 + 48) = px;
                gm<be_f32>(m120 + 52) = py;
                gm<be_f32>(m120 + 56) = pz;
                dirty = true;
            }
            if (j == 0) {
                bx = gm<be_u32>(sp + 144);
                bz = gm<be_u32>(sp + 152);
                by = gm<be_u32>(sp + 148);
            } else if (j == n - 1) {
                ax = gm<be_u32>(sp + 144);
                ay = gm<be_u32>(sp + 148);
                az = gm<be_u32>(sp + 152);
            }

            // The five sub-links (+0, +20, +40 at weight |+192|, +60 / +80
            // at |+216|, each clamped to the link's +124 / the tuning's
            // +1520 and then divided by it); a sub-link's flag says whether
            // the bone's +36 is set and its weight is negative (or not).
            const u32 x = gm<be_u32>(gm<be_u32>(p + 612) + 72) + 224 * idx;
            const f32 w192 = F(x + 192), w216 = F(x + 216);
            const bool neg192 = w192 < kZeroF, neg216 = w216 < kZeroF;
            const f32 a192 = std::fabs(w192), a216 = std::fabs(w216);
            const bool on = gm<be_u32>(x + 36) != 0;
            const f32 lim1 = F(e + 124);
            f32 s1 = a192 < lim1 ? a192 : lim1;
            if (!(s1 > kZeroF)) s1 = kZeroF;
            const f32 lim2 = F(t + 1500 + 20);
            f32 s2 = a216 < lim2 ? a216 : lim2;
            if (!(s2 > kZeroF)) s2 = kZeroF;
            const f32 r1 = Divs(s1, lim1), r2 = Divs(s2, lim2);
            const u32 mm = sp + 608;
            Player_DriveLink236DB8(p, e + 0, mm, 0, on ? 1u : 0u, r1);
            Player_DriveLink236DB8(p, e + 20, mm, 0, (on && !neg192) ? 1u : 0u, r1);
            Player_DriveLink236DB8(p, e + 40, mm, 0, (on && neg192) ? 1u : 0u, r1);
            Player_DriveLink236DB8(p, e + 60, mm, 0, (on && neg216) ? 1u : 0u, r2);
            Player_DriveLink236DB8(p, e + 80, mm, 0, (on && !neg216) ? 1u : 0u, r2);
        }

        // The group's object between A and B: row 0 the direction A -> B,
        // row 1 = up (p +128) x row 0 (optionally a quarter turn about it),
        // row 2 = up, row 3 the midpoint.
        const u32 obj = gm<be_u32>(gm<be_u32>(p + 696) + 4 * g);
        if (obj == 0 || n <= 1) continue;
        f32 dy = Subs(WordF13(by), WordF13(ay));
        f32 dx = Subs(WordF13(bx), WordF13(ax));
        f32 dz = Subs(WordF13(bz), WordF13(az));
        f32 d2 = Fmadds(dx, dx, Muls(dy, dy));
        const f32 mx = Fmadds(dx, kHalf, WordF13(ax));
        const f32 my = Fmadds(dy, kHalf, WordF13(ay));
        d2 = Fmadds(dz, dz, d2);
        const f32 vx = F(p + 128), vy = F(p + 132), vz = F(p + 136);
        const f32 mz = Fmadds(dz, kHalf, WordF13(az));
        CopyW(sp + 900, p + 132);
        CopyW(sp + 896, p + 128);
        CopyW(sp + 904, p + 136);
        const f32 inv = Divs(kOneF, Fsqrts13(d2));
        CopyW(sp + 912, kZero);
        CopyW(sp + 916, kZero + 4);
        CopyW(sp + 920, kZero + 8);
        dx = Muls(inv, dx);
        dy = Muls(inv, dy);
        dz = Muls(inv, dz);
        gm<be_f32>(sp + 864) = dx;
        gm<be_f32>(sp + 868) = dy;
        gm<be_f32>(sp + 872) = dz;
        const f32 qz = Fmsubs(vx, dy, Muls(vy, dx));
        const f32 qx = Fmsubs(vy, dz, Muls(vz, dy));
        const f32 qy = Fmsubs(vz, dx, Muls(vx, dz));
        gm<be_f32>(sp + 888) = qz;
        gm<be_f32>(sp + 880) = qx;
        gm<be_f32>(sp + 884) = qy;
        u32 w0 = std::bit_cast<u32>(dx);
        if (gm<u8>(grp + 28) != 0) {
            AxisQuat(sp + 288, qx, qy, qz, kOneF, kQuarter);
            call(Quat_ToMatrix, sp + 928, sp + 288, kZero);
            call(Mat44_MulInPlace75D154, sp + 864, sp + 928);
            w0 = gm<be_u32>(sp + 864);
        }
        gm<be_f32>(sp + 912) = mx;
        gm<be_f32>(sp + 916) = my;
        gm<be_f32>(sp + 920) = mz;
        gm<be_u32>(obj + 16) = w0;
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = (r == 0 ? 1 : 0); k < 3; ++k) CopyW(obj + 16 + 16 * r + 4 * k, sp + 864 + 16 * r + 4 * k);
        call(WorldObj_MarkDirty, obj);
    }
    if (!dirty) return;
    call(World_AddTimedCallback, gm<be_u32>(kWorld), 1u, kLinkCallback, p, p);
}

}  // namespace sly4::obj

NATIVE(00232B60, sly4::obj::Player_TunePart232B60)
NATIVE(002329DC, sly4::obj::Player_TunePart2329DC)
NATIVE(002327A8, sly4::obj::Player_TunePart2327A8)
NATIVE(00232650, sly4::obj::Player_TunePart232650)
NATIVE(00232360, sly4::obj::Player_TunePart232360)
NATIVE(00231208, sly4::obj::Player_InitLinkPart231208)
NATIVE(002371F8, sly4::obj::Player_SetRate2371F8)
