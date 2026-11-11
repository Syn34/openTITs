// Game code, medium sweep part 14: constructors and record copies.
#include "mid14.h"

using namespace sly4;
using namespace sly4::mid14;

namespace {
constexpr u32 kVecZero = 0x01164B40;   // f32[3] 0
constexpr u32 kVec4B90 = 0x01164B90;   // f32[3]
constexpr u32 kK22AAF8 = 0x0022AAF8;   // f32 pair multiplied into +120
}  // namespace

// void ChoiceEntry_Construct22AB00(ChoiceEntry* p)
// (called by ChoiceEntry_Read22AD08, ChoiceList_Read22C2B0, 0022C4A8) The
// 352-byte choice entry: five PtrVectors at +240..+320 (element size 64 at
// +6 of each), then the defaults: +0 / +16 vec3 (0x01164B40), +32 / +36 0,
// bytes +40 / +41 0, +76..+84 0, floats +96..+116, +120 = k0 * k1
// (0x0022AAF8, fmuls), +124 0, floats +128..+176, bytes +164 / +180 0,
// +192 vec3 (0x01164B40), +208 vec3 (0x01164B90), floats +224..+236.
DECOMP(0022AB00, ChoiceEntry_Construct22AB00) {
    const u32 p = arg<u32>(ctx, 0);
    for (u32 k = 240; k <= 320; k += 20) {
        call(ctx, m1fn::PtrVector_Construct, {p + k});
        store<u16>(p + k + 6, 64);
    }
    Cw(p + 0, kVecZero);
    Cw(p + 4, kVecZero + 4);
    Cw(p + 8, kVecZero + 8);
    Cw(p + 16, kVecZero);
    store<u32>(p + 32, 0);
    Cw(p + 20, kVecZero + 4);
    store<u32>(p + 36, 0);
    Cw(p + 24, kVecZero + 8);
    store<u8>(p + 40, 0);
    store<u8>(p + 41, 0);
    store<u32>(p + 76, 0);
    store<u32>(p + 80, 0);
    store<u32>(p + 84, 0);
    store<u32>(p + 96, 0x3ECCCCCDu);   // 0.4
    store<u32>(p + 100, 0x41200000u);  // 10
    store<u32>(p + 104, 0x3E4CCCCDu);  // 0.2
    const f64 k0 = LfsF(load<u32>(kK22AAF8));
    store<u32>(p + 108, 0x3FC00000u);  // 1.5
    store<u32>(p + 112, 0xBF800000u);  // -1
    store<u32>(p + 116, 0x3E800000u);  // 0.25
    const f64 k1 = LfsF(load<u32>(kK22AAF8 + 4));
    store<u32>(p + 124, 0);
    const f64 m = MulS(k0, k1);
    store<u32>(p + 128, 0x3DCCCCCDu);  // 0.1
    store<u32>(p + 132, 0x419CCCCDu);  // 19.6
    store<u32>(p + 136, 0x3F800000u);  // 1
    for (u32 k = 140; k <= 160; k += 4) store<u32>(p + k, 0xBF800000u);
    store<u32>(p + 168, 0x3E4CCCCDu);
    StoreF(p + 120, m);
    store<u32>(p + 172, 0x44BB8000u);  // 1500
    store<u32>(p + 176, 0x3F666666u);  // 0.9
    Cw(p + 192, kVecZero);
    store<u8>(p + 164, 0);
    Cw(p + 196, kVecZero + 4);
    store<u8>(p + 180, 0);
    Cw(p + 200, kVecZero + 8);
    Cw(p + 208, kVec4B90);
    Cw(p + 212, kVec4B90 + 4);
    store<u32>(p + 224, 0x3EB33333u);  // 0.35
    Cw(p + 216, kVec4B90 + 8);
    store<u32>(p + 228, 0x42480000u);  // 50
    store<u32>(p + 232, 0x40400000u);  // 3
    store<u32>(p + 236, 0x40400000u);
    // Leftovers: f1 = the last word loaded (0x01164B98), r3 = 0x42480000.
    SetF1(ctx, kVec4B90 + 8);
    ret(ctx, 0x42480000u);
}

// u32 Vector_Push352_75C8F4(Vector* v, const ChoiceEntry* src)
// (called by ChoiceList_Read22C2B0, 0022C4A8) Appends a 352-byte element
// {+0 data, +4 u16 capacity, +8 u16 count, +10 u16 high-water}: grows it
// (PtrVector_Reserve to twice the capacity, masked 0x1FFFE) when full, then
// copies the element field by field: words +0..+36, bytes +40..+73, words
// +76..+84, +96..+163, byte +164, words +168..+176, byte +180, the floats
// +192..+236 (not +204 / +220), and the five PtrVector headers +240..+339
// as plain words (a shallow copy). Bumps the count, raises the high-water
// mark to it and returns the new element's index (count - 1).
DECOMP(0075C8F4, Vector_Push352_75C8F4) {
    const u32 v = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x0001FFFEu});
        n = load<u16>(v + 8);
    }
    const u32 d = load<u32>(v) + n * 352;
    for (u32 k : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u}) Cw(d + k, s + k);
    for (u32 k = 40; k <= 73; ++k) store<u8>(d + k, load<u8>(s + k));
    for (u32 k = 76; k <= 84; k += 4) Cw(d + k, s + k);
    for (u32 k = 96; k <= 160; k += 4) Cw(d + k, s + k);
    store<u8>(d + 164, load<u8>(s + 164));
    for (u32 k = 168; k <= 176; k += 4) Cw(d + k, s + k);
    store<u8>(d + 180, load<u8>(s + 180));
    for (u32 k : {192u, 196u, 200u, 208u, 212u, 216u, 224u, 228u, 232u, 236u}) Cw(d + k, s + k);
    SetF1(ctx, s + 236);  // leftover: the last lfs
    for (u32 k = 240; k <= 336; k += 4) Cw(d + k, s + k);
    const u32 c = (load<u16>(v + 8) + 1) & 0xFFFFu;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(c));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c) ? hw : c));
    ret(ctx, c - 1);
}

// Def* XmlDef_Construct18EA50(Def* p, const Record* rec)
// (called by XmlDef_New18F8E8) Thunk_000C3078 (the def base), vtable
// 0x00B14D98, the record's +140..+232 copied to +192..+284 (with gaps),
// u32 +292 0, the PtrVector +296 (element size 4), bytes +285..+290 from
// the record's +233..+238. +220 is raised to the record's +76 when below
// k (0x0018EA4C). Then the object named by the record's +124 is looked up
// in the game's table (0x00F65CA0 -> +0x1281D4); when found it is removed
// from its world (Havok removeEntity on its +8), its slot-32 virtual result
// stored in +292, and the record's +128 / +132 / +136 (each when >= k)
// applied: a motion change (0x0091D470 for == k, 0x0091D618 for > k)
// unless that body's kind byte +232 is 4 or 5, and floats into its +140 /
// +144. Last its +411 / +410 get 5.0 / 100.0 (0x007EE08C) and +44 the
// kind's quality (2 for kind 5, 3 for 4, else 4). Returns +292 (0 when
// the object is not found).
DECOMP(0018EA50, XmlDef_Construct18EA50) {
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    call(ctx, m14fn::Thunk_000C3078, {p, rec});
    for (u32 k = 0; k <= 44; k += 4) Cw(p + 192 + k, rec + 140 + k);
    store<u32>(p, 0x00B14D98u);
    Cw(p + 240, rec + 188);
    Cw(p + 244, rec + 192);
    Cw(p + 248, rec + 196);
    const u8 b200 = load<u8>(rec + 200);
    Cw(p + 264, rec + 212);
    const u32 w204 = load<u32>(rec + 204);
    Cw(p + 268, rec + 216);
    store<u8>(p + 252, b200);
    const u32 w220 = load<u32>(rec + 220);
    store<u32>(p + 256, w204);
    const u32 w208 = load<u32>(rec + 208);
    store<u32>(p + 272, w220);
    const u32 w224 = load<u32>(rec + 224);
    store<u32>(p + 260, w208);
    store<u32>(p + 276, w224);
    const u32 w228 = load<u32>(rec + 228);
    const u8 b232 = load<u8>(rec + 232);
    store<u32>(p + 280, w228);
    store<u8>(p + 284, b232);
    store<u32>(p + 292, 0);
    call(ctx, m1fn::PtrVector_Construct, {p + 296});
    const u32 w76 = load<u32>(rec + 76);
    const u32 name = load<u32>(rec + 124);
    const u8 c0 = load<u8>(rec + 233);
    store<u16>(p + 302, 4);
    store<u8>(p + 285, c0);
    for (u32 k = 1; k < 6; ++k) store<u8>(p + 285 + k, load<u8>(rec + 233 + k));
    const f64 lim = LfsF(load<u32>(0x0018EA4Cu));
    if (LoadF(p + 220) < lim) store<u32>(p + 220, w76);
    ctx.fpr[1] = LfsF(w76);  // leftover f1 when the lookup fails
    const u32 tbl = load<u32>(load<u32>(kGlobF65CA0) + 0x001281D4u);
    u32 o = static_cast<u32>(call(ctx, m14fn::Obj_FindName_0034654C, {tbl, name}));
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();  // the original's sp + 112
    if (const u32 w = load<u32>(o + 8); w != 0) call(ctx, m14fn::Hk_RemoveEntity92FFC8, {sp + 8, w, o});
    o = static_cast<u32>(VCall(ctx, o, 32, {o}));
    const f64 f128 = LfsF(load<u32>(rec + 128));
    store<u32>(p + 292, o);
    if (f128 > lim || f128 == lim) {
        const u8 kind = load<u8>(o + 232);
        if (kind != 5 && kind != 4) {
            if (f128 != lim) call(ctx, m14fn::Hk_SetMass_0091D618, {o});
            else call(ctx, m14fn::Hk_SetMotion_0091D470, {o, 5, 1, 0});
        }
    }
    const u32 w132 = load<u32>(rec + 132);
    if (const f64 f = LfsF(w132); f > lim || f == lim) store<u32>(load<u32>(p + 292) + 140, w132);
    const u32 w136 = load<u32>(rec + 136);
    if (const f64 f = LfsF(w136); f > lim || f == lim) store<u32>(load<u32>(p + 292) + 144, w136);
    const u32 b = load<u32>(p + 292);
    u32 q = 2;
    if (load<u8>(b + 232) != 5) q = load<u8>(b + 232) == 4 ? 3 : 4;
    ctx.fpr[1] = LfsF(w136);  // leftover f1 (the last lfs)
    store<u32>(sp + 4, 0x40A00000u);  // 5.0
    call(ctx, m14fn::Hk_SetF32At_007EE08C, {b + 411, sp + 4});
    store<u32>(sp, 0x42C80000u);  // 100.0
    call(ctx, m14fn::Hk_SetF32At_007EE08C, {load<u32>(p + 292) + 410, sp});
    const u32 r = load<u32>(p + 292);
    store<u32>(r + 44, q);
    ret(ctx, r);
}

// u32 SetSoundRegister_Init91178(Action* o, XmlCursor* c)
// (called by New_00091698) The "SetSoundRegister" action: base
// constructor (0x004BB3B0), vtable 0x00B10968, +12..+20 0, bytes +24..+28
// 0, +29 -128, +30 0, +31 127. Reads hash 0x3404783C -> +12, s32
// 0x012C69E3 -> +16 (the register) and bools 0x688367CA / 0x7D7BF7A3 /
// 0x0464F605 -> bytes +24 / +25 (global) / +26. The register is checked
// (String_Format warnings) and clamped to 1..64 (global) or 0..3 (sound;
// stored one up at the end). When hash 0x82A06C8A -> +20 is not found the
// value byte +27 comes from float 0x5B5AF6FF (0..1, scaled to min..127,
// min = -128 or 0 for byte +24) or else s32 0x82A03D52 (min..127). Then
// s32 0x4C5F680C / 0x2537A9BA (when found: flag bytes +28 / +30) clamped
// to -128..127 into bytes +29 / +31. Returns byte +25, or the stored
// register when that is 0 (registers left as the original leaves them).
DECOMP(00091178, SetSoundRegister_Init91178) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, m14fn::Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B10968u);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    for (u32 k = 24; k <= 28; ++k) store<u8>(o + k, 0);
    store<u8>(o + 29, 0x80);
    store<u8>(o + 30, 0);
    store<u8>(o + 31, 0x7F);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x3404783Cu, o + 12, 0});
    call(ctx, xfn::XmlCursor_ReadS32, {c, 0x012C69E3u, o + 16, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x688367CAu, o + 24, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x7D7BF7A3u, o + 25, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x0464F605u, o + 26, 0});
    s32 r;
    if (load<u8>(o + 25) != 0) {
        s32 g = static_cast<s32>(load<u32>(o + 16));
        if (g < 1 || g > 64) {
            call(ctx, m1fn::String_Format, {0x00AC5494u, static_cast<u64>(static_cast<s64>(g)), 1, 64});
            g = static_cast<s32>(load<u32>(o + 16));
        }
        r = ClampS(g, 1, 64);
    } else {
        s32 g = static_cast<s32>(load<u32>(o + 16));
        if (g < 0 || g >= 4) {
            call(ctx, m1fn::String_Format, {0x00AC54ECu, static_cast<u64>(static_cast<s64>(g)), 0, 3});
            g = static_cast<s32>(load<u32>(o + 16));
        }
        r = ClampS(g, 0, 3);
    }
    const s32 lo = load<u8>(o + 24) != 0 ? 0 : -128;
    store<u32>(o + 16, static_cast<u32>(r));
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();  // the original's sp + 112 (+0 s32, +4 s32, +8 f32)
    if (static_cast<u32>(call(ctx, xfn::XmlCursor_ReadHash, {c, 0x82A06C8Au, o + 20, 0})) != 0) {
        const u32 kk = 0x0009116Cu;  // f32 0 / 1
        const f64 k0 = LfsF(load<u32>(kk));
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x5B5AF6FFu, sp + 8}, {k0});
        if (static_cast<u32>(ctx.gpr[3]) == 0) {
            f64 f = LfsF(load<u32>(sp + 8));
            if (f < k0 || f > LfsF(load<u32>(kk + 4))) {
                ctx.fpr[1] = f;  // the %f vararg goes in f1 and r4
                call(ctx, m1fn::String_Format, {0x00AC5544u, std::bit_cast<u64>(f)});
                f = LfsF(load<u32>(sp + 8));
            }
            const f64 span = static_cast<f64>(static_cast<f32>(static_cast<f64>(static_cast<s64>(127 - lo))));
            const u64 t = rt::fctiw(MulS(f, span), true);
            ctx.fpr[1] = std::bit_cast<f64>(t);  // leftover f1
            const s32 x = lo + static_cast<s32>(static_cast<u32>(t));
            store<u32>(sp + 4, static_cast<u32>(x));
            store<u8>(o + 27, static_cast<u8>(ClampS(x, lo, 127)));
        } else if (static_cast<u32>(call(ctx, xfn::XmlCursor_ReadS32, {c, 0x82A03D52u, sp + 4, 0})) == 0) {
            s32 x = static_cast<s32>(load<u32>(sp + 4));
            if (lo > x || x > 127) {
                call(ctx, m1fn::String_Format, {0x00AC559Cu, static_cast<u64>(static_cast<s64>(x)), static_cast<u64>(static_cast<s64>(lo)), 127});
                x = static_cast<s32>(load<u32>(sp + 4));
            }
            store<u8>(o + 27, static_cast<u8>(ClampS(x, lo, 127)));
        }
    }
    store<u32>(sp, 0);
    if (static_cast<u32>(call(ctx, xfn::XmlCursor_ReadS32, {c, 0x4C5F680Cu, sp, 0})) == 0) {
        store<u8>(o + 28, 1);
        store<u8>(o + 29, static_cast<u8>(ClampS(static_cast<s32>(load<u32>(sp)), -128, 127)));
    }
    if (static_cast<u32>(call(ctx, xfn::XmlCursor_ReadS32, {c, 0x2537A9BAu, sp, 0})) == 0) {
        store<u8>(o + 30, 1);
        store<u8>(o + 31, static_cast<u8>(ClampS(static_cast<s32>(load<u32>(sp)), -128, 127)));
    }
    u32 res = load<u8>(o + 25);
    if (res == 0) {
        res = load<u32>(o + 16) + 1;
        store<u32>(o + 16, res);
    }
    ret(ctx, res);
}
