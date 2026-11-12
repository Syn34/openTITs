// Game code, large-function sweep part 16: damage / collision def readers
// (the Def_Construct74FD80 family).
#include "big16.h"

namespace sly4::obj {

namespace {
constexpr u32 kPoolD3 = 0x001DD3C8;      // f32 {100, 0, 0.3, 1, -1, pi/180, 0.5}, f64 {0, 1} at +32 / +40
constexpr u32 kKinds = 0x00AD1ED8;       // u32[2] {0x0024A738, 0xE01ED3CB}
constexpr u32 sPlayerName = 0x00AD1EE4;  // "playerName"
constexpr u32 kStrAD1E98 = 0x00AD1E98;   // the damage def name (both string arguments)
constexpr u32 kQuatIdent = 0x01164C80;   // f32[4], the identity quaternion
constexpr u32 kVec2_4AFC = 0x01164AFC;   // f32[2]
}  // namespace

// void DamageColDef_ReadDD3F8(a0, XmlCursor* c, x, const Def* tmpl, Rec* o)
// (no caller reads r3 or f1): DamageColDef_Read_000C85A8 with the same
// arguments, then a defaults record on the stack (DmgDef_Ctor: x->12, the
// name 0x00AD1E98 twice, pool 0x001DD3C8) with zero vec3s +240 / +272 /
// +304 / +352, identity quaternions +320 / +368, f32 limits +440..+452 =
// -90 / 90 / -180 / 180, +460..+472 1.0, +484..+492 / +540..+548 -1,
// +496..+508 0x3B8EFA35 / 0x3D0EFA35 x2, +552 / +556 the two floats at
// 0x01164AFC, byte +432 1, the rest of +224..+538 0 (+516 / +520 an empty
// string {length, buffer}); +4 |= 0x40000000. A tmpl with 0x40000000 in
// +24 supplies them instead (rec +k = tmpl +k + 80; the string duplicated
// with String_Alloc / String_CopyN when its length is non-zero); a tmpl
// without it notes c's document path. o is then read from c: hash
// 0x0A2E921E +224, vec3 0x81785FF8 +240, hash 0x7242FC8F +256, vec3
// 0xCC0D6864 +272, hashes 0xFC8AFCD5 +296 / 0xA7E293B8 +300, vec3
// 0x3818B318 +304, the euler angles 0x898B7952 (degrees, default 0) as a
// quaternion into +320, bools +336 / +337, hash +340, vec3 +352, the euler
// angles 0x66E10869 as a quaternion into +368 (default 0, not the record's
// +368), f32s +384..+396 (+384 / +392 / +396 at least 0, +388 at least
// +384), hash +400, f32s +404..+508 (+496..+508 degrees to radians when
// present), the hash 0x79545A55 whose index in {0x0024A738, 0xE01ED3CB}
// goes to +512 when found, the "playerName" attribute duplicated into the
// string +516 / +520 (the old buffer freed first), hashes +524..+532,
// bools +536..+538, f32s +540..+548 and the vec2 0xBF3F0DB1 +552 (degrees
// to radians when present); then f32s +456..+480 are clamped to [0, 1].
// The record's string buffer is freed when set and its vector destroyed.
void DamageColDef_ReadDD3F8(u32 a0, u32 c, u32 x, u32 tmpl, u32 o) {
    GuestFrame frame(1152);
    const u32 sp = frame.addr(), rec = sp + 400, e1 = sp + 368, e2 = sp + 384, kh = sp + 960;
    auto W = [](u32 base, u32 off) -> be_u32& { return gm<be_u32>(base + off); };
    auto B = [](u32 base, u32 off) -> u8& { return gm<u8>(base + off); };
    auto F = [](u32 ea) { return LfsW(gm<be_u32>(ea)); };
    auto St = [](u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); };
    call(fn::DamageColDef_Read_000C85A8, a0, c, x, tmpl, o);
    DmgDef_Ctor(rec, W(x, 12), kStrAD1E98, kStrAD1E98, kPoolD3);
    for (u32 v : {240u, 272u, 304u, 352u})
        for (u32 k = 0; k < 3; ++k) W(rec, v + 4 * k) = W(kZeroVec, 4 * k);
    for (u32 q : {320u, 368u})
        for (u32 k = 0; k < 4; ++k) W(rec, q + 4 * k) = W(kQuatIdent, 4 * k);
    for (u32 off : {224u, 256u, 296u, 300u, 340u, 384u, 388u, 392u, 396u, 400u, 436u, 456u, 476u, 480u, 512u, 516u, 520u, 524u,
                    528u, 532u})
        W(rec, off) = 0u;
    for (u32 k = 0; k < 7; ++k) W(rec, 404 + 4 * k) = 0u;
    B(rec, 336) = 0;
    B(rec, 337) = 0;
    W(rec, 440) = 0xC2B40000u;
    W(rec, 444) = 0x42B40000u;
    W(rec, 448) = 0xC3340000u;
    W(rec, 452) = 0x43340000u;
    B(rec, 432) = 1;
    B(rec, 433) = 0;
    for (u32 k = 0; k < 4; ++k) W(rec, 460 + 4 * k) = 0x3F800000u;
    for (u32 off : {484u, 488u, 492u, 540u, 544u, 548u}) W(rec, off) = 0xBF800000u;
    for (u32 k = 0; k < 2; ++k) {
        W(rec, 496 + 8 * k) = 0x3B8EFA35u;
        W(rec, 500 + 8 * k) = 0x3D0EFA35u;
    }
    for (u32 k = 0; k < 3; ++k) B(rec, 536 + k) = 0;
    W(rec, 552) = W(kVec2_4AFC, 0);
    W(rec, 556) = W(kVec2_4AFC, 4);
    W(rec, 4) = W(rec, 4) | 0x40000000u;
    if (tmpl != 0) {
        if (W(tmpl, 24) & 0x40000000u) {
            for (u32 k : {224u, 240u, 244u, 248u, 256u, 272u, 276u, 280u, 296u, 300u, 304u, 308u, 312u, 320u, 324u, 328u, 332u})
                W(rec, k) = W(tmpl, k + 80);
            B(rec, 336) = B(tmpl, 416);
            B(rec, 337) = B(tmpl, 417);
            W(rec, 340) = W(tmpl, 420);
            for (u32 k = 352; k <= 360; k += 4) W(rec, k) = W(tmpl, k + 80);
            for (u32 k = 368; k <= 428; k += 4) W(rec, k) = W(tmpl, k + 80);
            B(rec, 432) = B(tmpl, 512);
            B(rec, 433) = B(tmpl, 513);
            for (u32 k = 436; k <= 512; k += 4) W(rec, k) = W(tmpl, k + 80);
            const u32 len = W(tmpl, 596);
            W(rec, 520) = 0u;
            W(rec, 516) = len;
            if (len != 0) {
                W(rec, 520) = call<u32>(fn::String_Alloc, len);
                call(fn::String_CopyN, W(rec, 520).get(), W(tmpl, 600).get(), static_cast<s32>(W(rec, 516).get()));
            }
            for (u32 k = 524; k <= 532; k += 4) W(rec, k) = W(tmpl, k + 80);
            for (u32 k = 536; k <= 538; ++k) B(rec, k) = B(tmpl, k + 80);
            for (u32 k = 540; k <= 556; k += 4) W(rec, k) = W(tmpl, k + 80);
        } else {
            call(fn::XmlCursor_DocPath, a0);
        }
    }
    auto hash = [&](u32 h, u32 off) { call(fn::XmlCursor_ReadHash, c, h, o + off, W(rec, off).get()); };
    auto boolr = [&](u32 h, u32 off) { call(fn::XmlCursor_ReadBool, c, h, o + off, B(rec, off)); };
    auto vec3 = [&](u32 h, u32 off) { call(fn::XmlCursor_ReadVec3, c, h, o + off, rec + off); };
    auto f32r = [&](u32 h, u32 off) { return DmgReadF32(c, h, o + off, rec + off); };
    const f64 rad = F(kPoolD3 + 20), half = F(kPoolD3 + 24);
    const f64 zero64 = gm<be_f64>(kPoolD3 + 32).get(), one64 = gm<be_f64>(kPoolD3 + 40).get();
    // The euler angles (degrees) read into the local e (scaled in place) as
    // the quaternion q.
    auto euler = [&](u32 h, u32 e, u32 q) {
        call(fn::XmlCursor_ReadVec3, c, h, e, kZeroVec);
        const f64 ex = Muls(F(e), rad), ey = Muls(F(e + 4), rad), ez = Muls(F(e + 8), rad);
        St(e, ex);
        St(e + 4, ey);
        St(e + 8, ez);
        DmgEulerQuat(q, ex, ey, ez, half);
    };
    // Clamp of the f32 at ea below at 0 (fsel against the f64 0 of the pool;
    // NaN -> 0), stored back as the frsp of the result.
    auto clampLo0 = [&](u32 ea) {
        const f64 v = F(ea);
        St(ea, S(Fsel(v, v, zero64)));
    };
    hash(0x0A2E921Eu, 224);
    vec3(0x81785FF8u, 240);
    hash(0x7242FC8Fu, 256);
    vec3(0xCC0D6864u, 272);
    hash(0xFC8AFCD5u, 296);
    hash(0xA7E293B8u, 300);
    vec3(0x3818B318u, 304);
    euler(0x898B7952u, e1, o + 320);
    boolr(0x2A654FC3u, 336);
    boolr(0xDA165A53u, 337);
    hash(0x753EE781u, 340);
    vec3(0x1FAF38A1u, 352);
    euler(0x66E10869u, e2, o + 368);
    f32r(0x2C50D8F5u, 384);
    clampLo0(o + 384);
    f32r(0xA35DB66Eu, 388);
    {
        const f64 b = F(o + 388), a = F(o + 384);
        St(o + 388, Fsel(Subs(b, a), b, a));
    }
    f32r(0xA21AA8A9u, 392);
    clampLo0(o + 392);
    f32r(0x295E99EEu, 396);
    clampLo0(o + 396);
    hash(0x4205E7F3u, 400);
    constexpr u32 kF404[7] = {0xA771B991u, 0xD12B2C33u, 0xA5972CE6u, 0xB09C9890u, 0x4314A942u, 0x4CEF43B4u, 0x471C7A06u};
    for (u32 k = 0; k < 7; ++k) f32r(kF404[k], 404 + 4 * k);
    boolr(0xEC4A7369u, 432);
    boolr(0x6661DA37u, 433);
    constexpr u32 kF436[15] = {0xEADC4688u, 0xDE05302Eu, 0x47E3CC5Cu, 0x878EA6DDu, 0x8722768Bu, 0x4CB13E10u, 0x824119DCu, 0x2CA6B8AFu,
                               0x92AC6682u, 0x24EC3F49u, 0x093E0E77u, 0x183B44B4u, 0x99479EADu, 0x172CC986u, 0x810B65B4u};
    for (u32 k = 0; k < 15; ++k) f32r(kF436[k], 436 + 4 * k);
    constexpr u32 kF496[4] = {0x64AB749Eu, 0xD617D519u, 0xC209726Fu, 0x99F8C3AAu};
    for (u32 k = 0; k < 4; ++k)
        if (f32r(kF496[k], 496 + 4 * k) == 0) St(o + 496 + 4 * k, Muls(F(o + 496 + 4 * k), rad));
    W(kh, 0) = 0u;
    if (call<u32>(fn::XmlCursor_ReadHash, c, 0x79545A55u, kh, 0u) == 0) {
        const u32 v = W(kh, 0);
        for (u32 i = 0; i < 2; ++i)
            if (W(kKinds, 4 * i) == v) {
                W(o, 512) = i;
                break;
            }
    }
    const u32 name = call<u32>(fn::XmlCursor_FindAttrByName, c, sPlayerName);
    if (name != 0) {
        if (W(o, 520) != 0u) call(fn::Mem_FreeIfSet, W(o, 520).get(), W(o, 516).get());
        const u32 len = call<u32>(fn::String_Length, name) + 1u;
        W(o, 516) = len;
        W(o, 520) = call<u32>(fn::String_Alloc, len);
        call(fn::String_CopyN, W(o, 520).get(), name, static_cast<s32>(W(o, 516).get()));
    }
    hash(0x6350BDB4u, 524);
    hash(0xA2E0E877u, 528);
    hash(0x6A024DD2u, 532);
    boolr(0x8F16DBE7u, 536);
    boolr(0x3FF0E768u, 537);
    boolr(0xF29E7002u, 538);
    f32r(0x1F8A5F77u, 540);
    f32r(0x17C27FD0u, 544);
    f32r(0x81A11BFEu, 548);
    const u32 st = call<u32>(fn::XmlCursor_ReadVec2, c, 0xBF3F0DB1u, o + 552, rec + 552);
    f64 v[7];
    for (u32 k = 0; k < 7; ++k) v[k] = F(o + 456 + 4 * k);
    if (st == 0) {
        const f64 a = Muls(F(o + 552), rad), b = Muls(F(o + 556), rad);
        St(o + 552, a);
        St(o + 556, b);
    }
    const f64 one = F(kPoolD3 + 12);
    for (u32 k = 0; k < 7; ++k) {
        const f64 t = S(Fsel(Subs(one, v[k]), v[k], one64));
        v[k] = S(Fsel(t, t, zero64));
        St(o + 456 + 4 * k, v[k]);
    }
    const u32 buf = W(rec, 520);
    if (buf != 0) call(fn::Mem_FreeIfSet, buf, W(rec, 516).get());
    DmgDef_Destroy(rec);
}

// u32 DamageColDef_KindFromHashD6164(s32 hash): the kind bit of a damage
// kind name hash: 0x788F788A 1, 0x928BCAD1 2, 0x928F4636 4, 0xBFAFE1AD 8,
// 0x87EBFA08 16, 0x923F4D1D 32, 0x03D6A020 128, 2405 256, 2591 512, 2406
// 1024, 2592 2048, 2407 4096, 2593 8192, anything else (0x767FB0D0
// included) 64.
u32 DamageColDef_KindFromHashD6164(s32 h) {
    struct Kv {
        u32 h, v;
    };
    static constexpr Kv kLow[5] = {{0xBFAFE1ADu, 8}, {0x928F4636u, 4}, {0x928BCAD1u, 2}, {0x923F4D1Du, 32}, {0x87EBFA08u, 16}};
    static constexpr Kv kHigh[3] = {{0x788F788Au, 1}, {0x767FB0D0u, 64}, {0x03D6A020u, 128}};
    if (h == 2405) return 256u;
    const bool high = h > 2405;
    const Kv* t = high ? kHigh : kLow;
    const u32 n = high ? 3 : 5;
    for (u32 k = 0; k < n; ++k)
        if (static_cast<u32>(h) == t[k].h) return t[k].v;
    if (high) {
        switch (h) {
            case 2593: return 8192;
            case 2592: return 2048;
            case 2591: return 512;
            case 2407: return 4096;
            case 2406: return 1024;
            default: break;
        }
    }
    return 64;
}

}  // namespace sly4::obj

NATIVE(001DD3F8, sly4::obj::DamageColDef_ReadDD3F8)
NATIVE(001D6164, sly4::obj::DamageColDef_KindFromHashD6164)
