// Game code, large-function sweep part 13: the player's rig-part tuning
// helpers (callees of Player_Setup232F00 / Player_Setup231470).
#include "big13.h"

using namespace sly4;
using namespace sly4::big13;

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
DECOMP(00232B60, Player_TunePart232B60) {
    const u32 a = arg<u32>(ctx, 1);
    const u32 part = arg<u32>(ctx, 2);
    const u32 u = load<u32>(arg<u32>(ctx, 0) + 396) + kTuneLinks;
    {   // lvx / stvx: the aligned 16-byte blocks
        const u32 s = (a + 16) & ~15u, d = (part + 16) & ~15u;
        for (u32 k = 0; k < 16; k += 4) store<u32>(d + k, load<u32>(s + k));
    }
    const u32 nx = load<u32>(u + 0) ^ 0x80000000u, ny = load<u32>(u + 4) ^ 0x80000000u, nz = load<u32>(u + 8) ^ 0x80000000u;  // fneg
    const u32 w152 = load<u32>(u + 152);
    StoreVec3W(part + 48, load<u32>(u + 16), load<u32>(u + 20), load<u32>(u + 24));
    StoreVec3W(part + 64, load<u32>(u + 32), load<u32>(u + 36), load<u32>(u + 40));
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
    const u32 w92 = load<u32>(u + 92);
    store<u8>(part + 32, static_cast<u8>(w152));
    store<u32>(part + 132, w92);
    SetF1Bits(ctx, w92);   // the last lfs (the reserves leave f1 as they do)

    const s32 n = static_cast<s8>(w152);
    HkReserve(ctx, part + 140, n, 40);
    store<u32>(part + 144, static_cast<u32>(n));
    u32 r3 = load<u32>(part + 160);
    const s32 ng = load<u16>(u + 140);
    if (HkReserve(ctx, part + 152, ng, 1)) r3 = static_cast<u32>(ctx.gpr[3]);
    store<u32>(part + 156, static_cast<u32>(ng));
    if (ng > 0) {
        for (s32 g = 0; g < ng; ++g) {
            const u32 grp = load<u32>(u + 132) + 32 * static_cast<u32>(g);
            const u32 c = load<u16>(grp + 8);
            for (u32 j = 0; j < c; ++j) {
                const u32 it = load<u32>(grp) + 352 * j;
                const u32 e = load<u32>(part + 140) + 40 * (static_cast<u32>(g * ng) + j);
                store<u8>(e + 36, static_cast<u8>(g));
                CopyW(e + 12, it + 108);
                CopyW(e + 24, it + 124);
                CopyW(e + 0, it + 96);
                CopyW(e + 8, it + 104);
                CopyW(e + 4, it + 100);
                CopyW(e + 16, it + 116);
                CopyW(e + 20, it + 120);
                CopyW(e + 28, it + 128);
                const u32 w = load<u32>(it + 132);
                store<u32>(e + 32, w);
                SetF1Bits(ctx, w);
            }
            store<u8>(load<u32>(part + 152) + static_cast<u32>(g), static_cast<u8>(c));
        }
        r3 = static_cast<u32>(ng);
    }
    ctx.gpr[3] = r3;
}

// u32 Player_TunePart2329DC(Player* p, const Tier* a, Part* part) (name?):
// part's byte array +16 gets (s8) a->32 entries; tuning +1264 / +1268 to
// part +8 / +12; each link item's +164 byte to the square table. Returns
// the group count.
DECOMP(002329DC, Player_TunePart2329DC) {
    const u32 a = arg<u32>(ctx, 1);
    const u32 part = arg<u32>(ctx, 2);
    const u32 t = load<u32>(arg<u32>(ctx, 0) + 396);
    const s32 n = static_cast<s8>(load<u8>(a + 32));
    HkReserve(ctx, part + 16, n, 1);
    store<u32>(part + 20, static_cast<u32>(n));
    CopyW(part + 8, t + 1264);
    const u32 w = load<u32>(t + 1268);
    store<u32>(part + 12, w);
    SetF1Bits(ctx, w);
    ctx.gpr[3] = ForLinkItems(t + kTuneLinks, [&](u32, u32 i, u32 it) {
        store<u8>(load<u32>(part + 16) + i, load<u8>(it + 164));
    });
}

// u32 Player_TunePart2327A8(Player* p, const Tier* a, Part* part) (name?):
// tuning +1376..+1388 to part +8/+12/+20/+24; part's float array +28 gets
// the tuning's u16 +1400 keys (from the array at +1392) and its array +40
// (s8) a->32 entries; part +16 = 0x009FDF0C(+1324, +1228, +1280, the last
// key (unchecked: no keys reads the word before the array)); each link
// item's +168 float to the square table at +40. Returns the group count.
DECOMP(002327A8, Player_TunePart2327A8) {
    const u32 a = arg<u32>(ctx, 1);
    const u32 part = arg<u32>(ctx, 2);
    const u32 t = load<u32>(arg<u32>(ctx, 0) + 396);
    const u32 k = t + 1376;
    CopyW(part + 8, k + 0);
    CopyW(part + 12, k + 4);
    CopyW(part + 20, k + 8);
    const u32 nk = load<u16>(k + 24);
    const u32 w12 = load<u32>(k + 12);
    store<u32>(part + 24, w12);
    SetF1Bits(ctx, w12);
    HkReserve(ctx, part + 28, static_cast<s32>(nk), 4);
    store<u32>(part + 32, nk);
    const s32 n = static_cast<s8>(load<u8>(a + 32));
    HkReserve(ctx, part + 40, n, 4);
    store<u32>(part + 44, static_cast<u32>(n));
    for (u32 j = 0; j < nk; ++j) {
        const u32 w = load<u32>(load<u32>(k + 16) + 4 * j);
        store<u32>(load<u32>(part + 28) + 4 * j, w);
        SetF1Bits(ctx, w);
    }
    const u32 keys = load<u32>(part + 28);
    const u32 last = keys + 4 * (nk - 1);
    const f64 r = callf(ctx, b13fn::Hk_Curve9FDF0C, {last, keys},
                        {LfsX(t + 1272 + 52), LfsX(t + kTuneLinks + 156), LfsX(t + 1272 + 8), LfsX(last)});
    Stfs(part + 16, r);
    ctx.gpr[3] = ForLinkItems(t + kTuneLinks, [&](u32, u32 i, u32 it) {
        const u32 w = load<u32>(it + 168);
        store<u32>(load<u32>(part + 40) + 4 * i, w);
        SetF1Bits(ctx, w);
    });
}

// u32* Player_TunePart232650(Player* p, const Tier* a, Part* part) (name?):
// tuning +1412 to part +20; part's 12-byte array +8 gets (s8) a->32
// entries, each link item's {+172, +176 floats, +180 byte} in the square
// table. Returns the array's data.
DECOMP(00232650, Player_TunePart232650) {
    const u32 a = arg<u32>(ctx, 1);
    const u32 part = arg<u32>(ctx, 2);
    const u32 t = load<u32>(arg<u32>(ctx, 0) + 396);
    const u32 w = load<u32>(t + 1412);
    store<u32>(part + 20, w);
    SetF1Bits(ctx, w);
    const s32 n = static_cast<s8>(load<u8>(a + 32));
    HkReserve(ctx, part + 8, n, 12);
    const u32 data = load<u32>(part + 8);
    store<u32>(part + 12, static_cast<u32>(n));
    ForLinkItems(t + kTuneLinks, [&](u32, u32 i, u32 it) {
        const u32 d = data + 12 * i;
        CopyW(d + 0, it + 172);
        store<u8>(d + 8, load<u8>(it + 180));
        const u32 x = load<u32>(it + 176);
        store<u32>(d + 4, x);
        SetF1Bits(ctx, x);
    });
    ctx.gpr[3] = data;
}

// u32* Player_TunePart232360(Player* p, const Tier* a, Part* part) (name?):
// part's 48-byte array +8 and 12-byte array +20 each get (s8) a->32
// entries; each link item fills the square tables: {+192, +196, +200, 0},
// {+208, +212, +216, 0} and +224 in the 48-byte entry, +228..+236 in the
// 12-byte one. Returns the 48-byte array's data.
DECOMP(00232360, Player_TunePart232360) {
    const u32 a = arg<u32>(ctx, 1);
    const u32 part = arg<u32>(ctx, 2);
    const u32 t = load<u32>(arg<u32>(ctx, 0) + 396);
    s32 n = static_cast<s8>(load<u8>(a + 32));
    HkReserve(ctx, part + 8, n, 48);
    store<u32>(part + 12, static_cast<u32>(n));
    n = static_cast<s8>(load<u8>(a + 32));
    HkReserve(ctx, part + 20, n, 12);
    const u32 d48 = load<u32>(part + 8);
    const u32 d12 = load<u32>(part + 20);
    store<u32>(part + 24, static_cast<u32>(n));
    ForLinkItems(t + kTuneLinks, [&](u32, u32 i, u32 it) {
        const u32 e = d48 + 48 * i, f = d12 + 12 * i;
        CopyW(e + 32, it + 224);
        CopyW(f + 0, it + 228);
        CopyW(f + 4, it + 232);
        const u32 w = load<u32>(it + 236);
        store<u32>(f + 8, w);
        SetF1Bits(ctx, w);
        StoreVec3W(e + 0, load<u32>(it + 192), load<u32>(it + 196), load<u32>(it + 200));
        StoreVec3W(e + 16, load<u32>(it + 208), load<u32>(it + 212), load<u32>(it + 216));
    });
    ctx.gpr[3] = d48;
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
DECOMP(00231208, Player_InitLinkPart231208) {
    const u32 out = arg<u32>(ctx, 1);
    const u32 src = arg<u32>(ctx, 2);
    constexpr u32 kDefault = 0x00231200u, kMin = 0x00231204u;
    constexpr u32 kFmtClone = 0x00AD3CF0u;   // "%s_clone"
    constexpr u32 kEmitterDefs = 0x00F8F138u;   // holder* (+16 the name map)
    const u32 wDefault = load<u32>(kDefault);
    const f64 fMin = LfsAt(kMin);
    Stack frame(ctx, 16);
    const u32 key = frame.addr();   // the original's sp + 112
    u32 cnt;
    for (u32 i = 0; i < (cnt = load<u16>(src + 8)); ++i) {
        const u32 e = load<u32>(src) + 64 * i;
        u32 def = 0, inst = 0, w0 = wDefault, w1 = wDefault;
        store<u32>(key, load<u32>(e + 32));
        const u32 slot = static_cast<u32>(call(ctx, b13fn::MapE_Find, {load<u32>(load<u32>(kEmitterDefs) + 16), key}));
        const u32 found = load<u32>(slot);
        if (found != 0) {
            inst = static_cast<u32>(call(ctx, b13fn::EmitterInstance_New, {found, 1}));
            def = load<u32>(inst + 208);
            const u32 name = static_cast<u32>(VCall(ctx, def, 12, {def}));
            const u32 s = static_cast<u32>(call(ctx, b13fn::String_Format, {kFmtClone, name}));
            call(ctx, b13fn::Def_SetName, {def, s, 1});
            VCall(ctx, inst, 40, {inst, 1, 1});
            const u32 c = load<u32>(def + 112);
            if (c != 0) {
                w0 = load<u32>(e + 56);
                if (!(LfsAt(e + 56) >= fMin)) w0 = load<u32>(c + 16);
                store<u32>(c + 16, 0);
                w1 = load<u32>(e + 60);
                if (!(LfsAt(e + 60) >= fMin)) w1 = load<u32>(load<u32>(c + 108) + 8);
                store<u32>(load<u32>(c + 108) + 8, 0);
            }
        }
        u32 n = load<u16>(out + 8);
        if (n == load<u16>(out + 4)) {
            call(ctx, b13fn::PtrVector_Reserve, {out, (n << 1) & 0x1FFFEu});
            n = load<u16>(out + 8);
        }
        const u32 d = load<u32>(out) + 20 * n;
        const u32 hi = load<u16>(out + 10);
        store<u32>(d + 0, e);
        store<u32>(d + 4, def);
        store<u32>(d + 8, inst);
        store<u32>(d + 12, w0);
        store<u32>(d + 16, w1);
        const u32 n1 = load<u16>(out + 8) + 1;
        store<u16>(out + 8, static_cast<u16>(n1));
        store<u16>(out + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF) ? hi : n1));
    }
    ctx.gpr[3] = cnt;
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
constexpr u32 Obj_SetMatrixD87CC = 0x000D87CC;      // (obj, const Mat44*) (name?)
constexpr u32 Player_DriveLink236DB8 = 0x00236DB8;  // (Player*, link, const Mat44*, -, bool; f32 rate) (name?)
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
void AxisQuat(PPUContext& ctx, u32 q, f32 x, f32 y, f32 z, f32 one, f32 angle) {
    const f32 inv = Divs(one, Fsqrts13(Fmadds(z, z, Fmadds(x, x, Muls(y, y)))));
    const f32 ax = Muls(x, inv), ay = Muls(y, inv), az = Muls(z, inv);
    const f64 s = callf(ctx, k371::Math_Sinf, {}, {angle});
    const f32 qx = Muls(s, ax), qy = Muls(s, ay), qz = Muls(s, az);
    const f64 c = callf(ctx, k371::Math_Cosf, {}, {angle});
    store<f32>(q + 0, qx);
    store<f32>(q + 4, qy);
    store<f32>(q + 8, qz);
    Stfs(q + 12, c);
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
DECOMP(002371F8, Player_SetRate2371F8) {
    using namespace k371;
    const u32 p = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (load<u32>(p + 612) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const f32 kHalf = F(kConst + 4), kOneF = F(kConst + 8), kZeroF = F(kConst + 12), kQuarter = F(kConst + 16);
    Stack frame(ctx, 1392);
    const u32 sp = frame.addr();   // the original's frame, same offsets
    call(ctx, Vec3_Copy, {sp + 112, kZero});
    bool dirty = false;
    if (load<u32>(load<u32>(p + 612) + 24) != 0) {
        call(ctx, Mat43_Copy, {sp + 352, load<u32>(load<u32>(p + 612) + 24) + 240});
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(p + 96 + 16 * r + 4 * k, sp + 352 + 16 * r + 4 * k);
        const u32 b = load<u32>(load<u32>(p + 612) + 24);
        const u32 w1 = load<u32>(b + 432), w2 = load<u32>(b + 436), w3 = load<u32>(b + 440);
        const u32 fl = load<u32>(p + 172);
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
            store<f32>(sp + 112, dx);
            store<f32>(sp + 116, dy);
            store<f32>(sp + 120, dz);
            store<f32>(p + 144, Adds(F(p + 144), dx));
            store<f32>(p + 148, Adds(F(p + 148), dy));
            store<f32>(p + 152, Adds(F(p + 152), dz));
        }
        store<u32>(p + 560, w1);
        store<u32>(p + 564, w2);
        store<u32>(p + 568, w3);
        SetF1Bits(ctx, w1);   // f1 as the lfs of +432 leaves it
        const u32 lod = load<u32>(p + 160);
        if (lod != 0) call(ctx, WorldObj_UpdateLod, {lod, 0, 0});
    }

    const u32 t = load<u32>(p + 396);
    const u32 links = t + 1072;
    if (load<u32>(p + 736) != 0) {
        // p's matrix with its translation moved to the tuning's +944 point.
        const u32 m = t + 944;
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(sp + 416 + 16 * r + 4 * k, p + 96 + 16 * r + 4 * k);
        CopyW(sp + 128, m + 48);
        CopyW(sp + 132, m + 52);
        CopyW(sp + 136, m + 56);
        const big5::V4 v = big5::Add4(big5::Lvx(sp + 464), big5::Row3(big5::Lvx(sp + 416), big5::Lvx(sp + 432), big5::Lvx(sp + 448), F(m + 48), F(m + 52), F(m + 56)));
        big5::Stvx(sp + 128, v);
        CopyW(sp + 464, sp + 128);
        CopyW(sp + 468, sp + 132);
        const u32 wz = load<u32>(sp + 136);
        store<u32>(sp + 472, wz);
        SetF1Bits(ctx, wz);
        call(ctx, Obj_SetMatrixD87CC, {load<u32>(p + 736), sp + 416});
    }
    u32 rig = load<u32>(p + 612);
    VCall(ctx, rig, 60, {rig});
    const f32 rate = Divs(ctx.fpr[1], F(t + 1272 + 52));
    callf(ctx, Player_DriveLink236DB8, {p, p + 716, p + 96, ctx.gpr[6], 1}, {rate});   // r6 unread (leftover)
    const u32 st = load<u32>(load<u32>(p + 612) + 156);
    const u32 held = (load<u8>(st + 17) != 0 || load<u8>(st + 16) != 0) ? 1u : 0u;
    const u32 o740 = load<u32>(p + 740);
    if (o740 != 0) VCall(ctx, o740, 40, {o740, held, 1});
    if (static_cast<u32>(VCall(ctx, p, 32, {p, 0})) == 1) return;   // r3 = 1
    const u32 ng = load<u16>(links + 140);
    if (ng == 0) {
        ctx.gpr[3] = 0;
        return;
    }

    for (u32 g = 0; g < ng; ++g) {
        // B = the first item's bone position, A = the last one's.
        u32 bx = load<u32>(kZero), by = load<u32>(kZero + 4), bz = load<u32>(kZero + 8);
        u32 ax = load<u32>(kOne), ay = load<u32>(kOne + 4), az = load<u32>(kOne + 8);
        SetF1Bits(ctx, ax);
        const u32 grp = load<u32>(links + 132) + 32 * g;
        const u32 n = load<u16>(grp + 8);
        for (u32 j = 0; j < n; ++j) {
            const u32 idx = g * ng + j;
            const u32 e = load<u32>(p + 676) + 128 * idx;
            const u32 it = load<u32>(grp) + 352 * j;
            const u32 o2 = load<u32>(e + 100);
            if (o2 == 0 && load<u32>(e + 120) == 0) continue;
            rig = load<u32>(p + 612);
            VCall(ctx, rig, 48, {rig, load<u32>(rig + 24), load<u32>(rig + 52), static_cast<u64>(static_cast<s64>(static_cast<s32>(idx))), sp + 144, sp + 160});
            for (u32 k = 0; k < 16; k += 4) CopyW(sp + 528 + k, sp + 144 + k);
            call(ctx, Hk_ToMatrix7CEBD8, {sp + 480, sp + 160});
            call(ctx, Mat43_Copy, {sp + 544, sp + 480});

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
            store<f32>(sp + 640, cx);
            store<f32>(sp + 644, cy);
            store<f32>(sp + 648, cz);
            const f32 nx = Fmsubs(cy, z0, Muls(cz, y0));
            const f32 ny = Fmsubs(cz, x0, Muls(cx, z0));
            const f32 nz = Fmsubs(cx, y0, Muls(cy, x0));
            store<f32>(sp + 624, nx);
            store<f32>(sp + 628, ny);
            store<f32>(sp + 632, nz);
            if (load<u8>(it + 40) != 0) {   // a quarter-turn twist about row 1
                AxisQuat(ctx, sp + 272, nx, ny, nz, kOneF, kQuarter);
                call(ctx, Quat_ToMatrix, {sp + 800, sp + 272, kZero});
                call(ctx, Mat44_MulInPlace75D154, {sp + 608, sp + 800});
                x0 = F(sp + 608);
                y0 = F(sp + 612);
                z0 = F(sp + 616);
            }
            // Rotations by the item's half-angles about M's rows 0, 1, 2.
            AxisQuat(ctx, sp + 208, x0, y0, z0, kOneF, Muls(F(it + 0), kHalf));
            AxisQuat(ctx, sp + 224, F(sp + 624), F(sp + 628), F(sp + 632), kOneF, Muls(F(it + 4), kHalf));
            AxisQuat(ctx, sp + 240, F(sp + 640), F(sp + 644), F(sp + 648), kOneF, Muls(F(it + 8), kHalf));
            // The item's offset (+16) through p's rotation (rows +96/+112/+128).
            CopyW(sp + 256, it + 16);
            CopyW(sp + 260, it + 20);
            CopyW(sp + 264, it + 24);
            big5::Stvx(sp + 256, big5::Row3(big5::Lvx(p + 96), big5::Lvx(p + 112), big5::Lvx(p + 128), F(it + 16), F(it + 20), F(it + 24)));
            for (u32 r = 0; r < 3; ++r)
                for (u32 k = 0; k < 3; ++k) CopyW(sp + 304 + 16 * r + 4 * k, sp + 544 + 16 * r + 4 * k);
            SetF1Bits(ctx, load<u32>(sp + 584));
            call(ctx, Mat34_FromRotPos, {sp + 736, sp + 304, kZero});
            call(ctx, Quat_Multiply, {sp + 192, sp + 208, sp + 224});
            call(ctx, Quat_Multiply, {sp + 176, sp + 192, sp + 240});
            call(ctx, Quat_ToMatrix, {sp + 672, sp + 176, kZero});
            call(ctx, Mat44_MulInPlace75D154, {sp + 736, sp + 672});
            f32 px = Adds(F(sp + 592), F(sp + 256));
            f32 py = Adds(F(sp + 596), F(sp + 260));
            f32 pz = Adds(F(sp + 600), F(sp + 264));
            u32 rot[9];
            for (u32 r = 0; r < 3; ++r)
                for (u32 k = 0; k < 3; ++k) rot[3 * r + k] = load<u32>(sp + 736 + 16 * r + 4 * k);
            store<f32>(sp + 784, px);
            store<f32>(sp + 788, py);
            store<f32>(sp + 792, pz);
            px = Adds(px, F(sp + 112));
            py = Adds(py, F(sp + 116));
            pz = Adds(pz, F(sp + 120));
            SetF1Bits(ctx, load<u32>(sp + 116));
            if (o2 != 0) {
                for (u32 r = 0; r < 3; ++r)
                    for (u32 k = 0; k < 3; ++k) store<u32>(o2 + 16 + 16 * r + 4 * k, rot[3 * r + k]);
                store<f32>(o2 + 64, px);
                store<f32>(o2 + 68, py);
                store<f32>(o2 + 72, pz);
                call(ctx, WorldObj_MarkDirty, {o2});
            }
            const u32 m120 = load<u32>(e + 120);
            if (m120 != 0) {
                for (u32 r = 0; r < 3; ++r)
                    for (u32 k = 0; k < 3; ++k) store<u32>(m120 + 16 * r + 4 * k, rot[3 * r + k]);
                store<f32>(m120 + 48, px);
                store<f32>(m120 + 52, py);
                store<f32>(m120 + 56, pz);
                dirty = true;
            }
            if (j == 0) {
                bx = load<u32>(sp + 144);
                bz = load<u32>(sp + 152);
                by = load<u32>(sp + 148);
            } else if (j == n - 1) {
                ax = load<u32>(sp + 144);
                ay = load<u32>(sp + 148);
                az = load<u32>(sp + 152);
            }

            // The five sub-links (+0, +20, +40 at weight |+192|, +60 / +80
            // at |+216|, each clamped to the link's +124 / the tuning's
            // +1520 and then divided by it); a sub-link's flag says whether
            // the bone's +36 is set and its weight is negative (or not).
            const u32 x = load<u32>(load<u32>(p + 612) + 72) + 224 * idx;
            const f32 w192 = F(x + 192), w216 = F(x + 216);
            const bool neg192 = w192 < kZeroF, neg216 = w216 < kZeroF;
            const f32 a192 = std::fabs(w192), a216 = std::fabs(w216);
            const bool on = load<u32>(x + 36) != 0;
            const f32 lim1 = F(e + 124);
            f32 s1 = a192 < lim1 ? a192 : lim1;
            if (!(s1 > kZeroF)) s1 = kZeroF;
            const f32 lim2 = F(t + 1500 + 20);
            f32 s2 = a216 < lim2 ? a216 : lim2;
            if (!(s2 > kZeroF)) s2 = kZeroF;
            const f32 r1 = Divs(s1, lim1), r2 = Divs(s2, lim2);
            const u32 mm = sp + 608;
            callf(ctx, Player_DriveLink236DB8, {p, e + 0, mm, ctx.gpr[6], on ? 1u : 0u}, {r1});
            callf(ctx, Player_DriveLink236DB8, {p, e + 20, mm, ctx.gpr[6], (on && !neg192) ? 1u : 0u}, {r1});
            callf(ctx, Player_DriveLink236DB8, {p, e + 40, mm, ctx.gpr[6], (on && neg192) ? 1u : 0u}, {r1});
            callf(ctx, Player_DriveLink236DB8, {p, e + 60, mm, ctx.gpr[6], (on && neg216) ? 1u : 0u}, {r2});
            callf(ctx, Player_DriveLink236DB8, {p, e + 80, mm, ctx.gpr[6], (on && !neg216) ? 1u : 0u}, {r2});
        }

        // The group's object between A and B: row 0 the direction A -> B,
        // row 1 = up (p +128) x row 0 (optionally a quarter turn about it),
        // row 2 = up, row 3 the midpoint.
        const u32 obj = load<u32>(load<u32>(p + 696) + 4 * g);
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
        store<f32>(sp + 864, dx);
        store<f32>(sp + 868, dy);
        store<f32>(sp + 872, dz);
        const f32 qz = Fmsubs(vx, dy, Muls(vy, dx));
        const f32 qx = Fmsubs(vy, dz, Muls(vz, dy));
        const f32 qy = Fmsubs(vz, dx, Muls(vx, dz));
        store<f32>(sp + 888, qz);
        store<f32>(sp + 880, qx);
        store<f32>(sp + 884, qy);
        u32 w0 = std::bit_cast<u32>(dx);
        if (load<u8>(grp + 28) != 0) {
            AxisQuat(ctx, sp + 288, qx, qy, qz, kOneF, kQuarter);
            call(ctx, Quat_ToMatrix, {sp + 928, sp + 288, kZero});
            call(ctx, Mat44_MulInPlace75D154, {sp + 864, sp + 928});
            w0 = load<u32>(sp + 864);
        }
        store<f32>(sp + 912, mx);
        store<f32>(sp + 916, my);
        store<f32>(sp + 920, mz);
        store<u32>(obj + 16, w0);
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = (r == 0 ? 1 : 0); k < 3; ++k) CopyW(obj + 16 + 16 * r + 4 * k, sp + 864 + 16 * r + 4 * k);
        SetF1Bits(ctx, load<u32>(sp + 920));
        call(ctx, WorldObj_MarkDirty, {obj});
    }
    if (!dirty) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, World_AddTimedCallback, {load<u32>(big12::kWorld), 1, kLinkCallback, p, p});
}
