// Game code, large-function sweep part 16: damage / collision def readers
// (the Def_Construct74FD80 family).
#include "big16.h"

using namespace sly4;
using namespace sly4::big16;

namespace {
namespace hdd3 {
constexpr u32 kPool = 0x001DD3C8;        // f32 {100, 0, 0.3, 1, -1, pi/180, 0.5}, f64 {0, 1} at +32 / +40
constexpr u32 kKinds = 0x00AD1ED8;       // u32[2] {0x0024A738, 0xE01ED3CB}
constexpr u32 sPlayerName = 0x00AD1EE4;  // "playerName"

// Clamp of the f32 at ea below at 0 (fsel against the f64 0 of the pool;
// NaN -> 0), stored back as the frsp of the result.
inline void ClampLo0(u32 ea) {
    const f64 v = LfsX(ea);
    Stfs(ea, Frsp(Fsel(v, v, Lfd(kPool + 32))));
}
}  // namespace hdd3
}  // namespace

// void DamageColDef_ReadDD3F8(a0, XmlCursor* c, x, const Def* tmpl, Rec* o)
// DamageColDef_Read_000C85A8 with the same arguments, then a defaults record
// on the stack (DmgCtor: x->12, the name 0x00AD1E98 twice, r7 / r8 as the
// call leaves them, pool 0x001DD3C8) with zero vec3s +240 / +272 / +304 /
// +352, identity quaternions +320 / +368, f32 limits +440..+452 = -90 / 90
// / -180 / 180, +460..+472 1.0, +484..+492 / +540..+548 -1, +496..+508
// 0x3B8EFA35 / 0x3D0EFA35 x2, +552 / +556 the two floats at 0x01164AFC,
// byte +432 1, the rest of +224..+538 0 (+516 / +520 an empty string
// {length, buffer}); +4 |= 0x40000000. A tmpl with 0x40000000 in +24
// supplies them instead (rec +k = tmpl +k + 80; the string duplicated with
// String_Alloc / String_CopyN when its length is non-zero); a tmpl without
// it notes c's document path. o is then read from c: hash 0x0A2E921E +224,
// vec3 0x81785FF8 +240, hash 0x7242FC8F +256, vec3 0xCC0D6864 +272, hashes
// 0xFC8AFCD5 +296 / 0xA7E293B8 +300, vec3 0x3818B318 +304, the euler
// angles 0x898B7952 (degrees, default 0) as a quaternion into +320, bools
// +336 / +337, hash +340, vec3 +352, the euler angles 0x66E10869 as a
// quaternion into +368 (default 0, not the record's +368), f32s +384..+396
// (+384 / +392 / +396 at least 0, +388 at least +384), hash +400, f32s
// +404..+508 (+496..+508 degrees to radians when present), the hash
// 0x79545A55 whose index in {0x0024A738, 0xE01ED3CB} goes to +512 when
// found, the "playerName" attribute duplicated into the string +516 / +520
// (the old buffer freed first), hashes +524..+532, bools +536..+538, f32s
// +540..+548 and the vec2 0xBF3F0DB1 +552 (degrees to radians when
// present); then f32s +456..+480 are clamped to [0, 1]. The record's string
// buffer is freed when set and its vector destroyed. r3 / f1 as the last
// call leaves them (r3 the record's string buffer and f1 the clamped +480
// when no call follows the clamps).
DECOMP(001DD3F8, DamageColDef_ReadDD3F8) {
    using namespace b16fn;
    using namespace hdd3;
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 1152);
    const u32 sp = frame.addr(), rec = sp + 400, e1 = sp + 368, e2 = sp + 384, kh = sp + 960;
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    DmgCtor(ctx, rec, load<u32>(x + 12), mid11::kStrAD1E98, mid11::kStrAD1E98, ctx.gpr[7], ctx.gpr[8], kPool);
    for (u32 v : {240u, 272u, 304u, 352u})
        for (u32 k = 0; k < 3; ++k) CopyW(rec + v + 4 * k, kVecZero + 4 * k);
    for (u32 q : {320u, 368u})
        for (u32 k = 0; k < 4; ++k) CopyW(rec + q + 4 * k, mid11::kQuatIdent + 4 * k);
    for (u32 off : {224u, 256u, 296u, 300u, 340u, 384u, 388u, 392u, 396u, 400u, 436u, 456u, 476u, 480u, 512u, 516u, 520u, 524u,
                    528u, 532u})
        store<u32>(rec + off, 0);
    for (u32 k = 0; k < 7; ++k) store<u32>(rec + 404 + 4 * k, 0);
    store<u8>(rec + 336, 0);
    store<u8>(rec + 337, 0);
    store<u32>(rec + 440, 0xC2B40000u);
    store<u32>(rec + 444, 0x42B40000u);
    store<u32>(rec + 448, 0xC3340000u);
    store<u32>(rec + 452, 0x43340000u);
    store<u8>(rec + 432, 1);
    store<u8>(rec + 433, 0);
    for (u32 k = 0; k < 4; ++k) store<u32>(rec + 460 + 4 * k, 0x3F800000u);
    for (u32 off : {484u, 488u, 492u, 540u, 544u, 548u}) store<u32>(rec + off, 0xBF800000u);
    for (u32 k = 0; k < 2; ++k) {
        store<u32>(rec + 496 + 8 * k, 0x3B8EFA35u);
        store<u32>(rec + 500 + 8 * k, 0x3D0EFA35u);
    }
    for (u32 k = 0; k < 3; ++k) store<u8>(rec + 536 + k, 0);
    CopyW(rec + 552, mid11::kVec2_4AFC);
    CopyW(rec + 556, mid11::kVec2_4AFC + 4);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x40000000u);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x40000000u) {
            for (u32 k : {224u, 240u, 244u, 248u, 256u, 272u, 276u, 280u, 296u, 300u, 304u, 308u, 312u, 320u, 324u, 328u, 332u})
                CopyW(rec + k, tmpl + k + 80);
            store<u8>(rec + 336, load<u8>(tmpl + 416));
            store<u8>(rec + 337, load<u8>(tmpl + 417));
            CopyW(rec + 340, tmpl + 420);
            for (u32 k = 352; k <= 360; k += 4) CopyW(rec + k, tmpl + k + 80);
            for (u32 k = 368; k <= 428; k += 4) CopyW(rec + k, tmpl + k + 80);
            store<u8>(rec + 432, load<u8>(tmpl + 512));
            store<u8>(rec + 433, load<u8>(tmpl + 513));
            for (u32 k = 436; k <= 512; k += 4) CopyW(rec + k, tmpl + k + 80);
            const u32 len = load<u32>(tmpl + 596);
            store<u32>(rec + 520, 0);
            store<u32>(rec + 516, len);
            if (len != 0) {
                store<u32>(rec + 520, static_cast<u32>(call(ctx, String_Alloc, {len})));
                call(ctx, String_CopyN, {load<u32>(rec + 520), load<u32>(tmpl + 600), Sx64(load<u32>(rec + 516))});
            }
            for (u32 k = 524; k <= 532; k += 4) CopyW(rec + k, tmpl + k + 80);
            for (u32 k = 536; k <= 538; ++k) store<u8>(rec + k, load<u8>(tmpl + k + 80));
            for (u32 k = 540; k <= 556; k += 4) CopyW(rec + k, tmpl + k + 80);
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    auto hash = [&](u32 h, u32 off) { call(ctx, XmlCursor_ReadHash, {c, h, o + off, load<u32>(rec + off)}); };
    auto boolr = [&](u32 h, u32 off) { call(ctx, XmlCursor_ReadBool, {c, h, o + off, load<u8>(rec + off)}); };
    auto vec3 = [&](u32 h, u32 off) { call(ctx, XmlCursor_ReadVec3, {c, h, o + off, rec + off}); };
    auto f32r = [&](u32 h, u32 off) { return RdF32(ctx, c, h, o + off, rec + off); };
    const f64 rad = LfsX(kPool + 20), half = LfsX(kPool + 24);
    // The euler angles (degrees) read into the local e (scaled in place) as
    // the quaternion q.
    auto euler = [&](u32 h, u32 e, u32 q) {
        call(ctx, XmlCursor_ReadVec3, {c, h, e, kVecZero});
        const f64 ex = Ms(LfsX(e), rad), ey = Ms(LfsX(e + 4), rad), ez = Ms(LfsX(e + 8), rad);
        Stfs(e, ex);
        Stfs(e + 4, ey);
        Stfs(e + 8, ez);
        mid12::EulerQuatAt(ctx, q, ex, ey, ez, half);
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
    ClampLo0(o + 384);
    f32r(0xA35DB66Eu, 388);
    {
        const f64 b = LfsX(o + 388), a = LfsX(o + 384);
        Stfs(o + 388, Fsel(Ss(b, a), b, a));
    }
    f32r(0xA21AA8A9u, 392);
    ClampLo0(o + 392);
    f32r(0x295E99EEu, 396);
    ClampLo0(o + 396);
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
        if (f32r(kF496[k], 496 + 4 * k) == 0) ScaleAt(o + 496 + 4 * k, rad);
    store<u32>(kh, 0);
    if (call(ctx, XmlCursor_ReadHash, {c, 0x79545A55u, kh, 0}) == 0) {
        const u32 v = load<u32>(kh);
        for (u32 i = 0; i < 2; ++i)
            if (load<u32>(kKinds + 4 * i) == v) {
                store<u32>(o + 512, i);
                break;
            }
    }
    const u32 name = static_cast<u32>(call(ctx, XmlCursor_FindAttrByName, {c, sPlayerName}));
    if (name != 0) {
        if (load<u32>(o + 520) != 0) call(ctx, Mem_FreeIfSet, {load<u32>(o + 520), load<u32>(o + 516)});
        const u32 len = static_cast<u32>(call(ctx, String_Length, {name})) + 1u;
        store<u32>(o + 516, len);
        store<u32>(o + 520, static_cast<u32>(call(ctx, String_Alloc, {len})));
        call(ctx, String_CopyN, {load<u32>(o + 520), name, Sx64(load<u32>(o + 516))});
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
    const u32 st = static_cast<u32>(call(ctx, XmlCursor_ReadVec2, {c, 0xBF3F0DB1u, o + 552, rec + 552}));
    f64 v[7];
    for (u32 k = 0; k < 7; ++k) v[k] = LfsX(o + 456 + 4 * k);
    if (st == 0) {
        const f64 a = Ms(LfsX(o + 552), rad), b = Ms(LfsX(o + 556), rad);
        Stfs(o + 552, a);
        Stfs(o + 556, b);
    }
    const f64 one = LfsX(kPool + 12), one64 = Lfd(kPool + 40), zero64 = Lfd(kPool + 32);
    for (u32 k = 0; k < 7; ++k) {
        const f64 t = Frsp(Fsel(Ss(one, v[k]), v[k], one64));
        v[k] = Frsp(Fsel(t, t, zero64));
        Stfs(o + 456 + 4 * k, v[k]);
    }
    ctx.fpr[1] = v[6];
    const u32 buf = load<u32>(rec + 520);
    ctx.gpr[3] = buf;
    if (buf != 0) call(ctx, Mem_FreeIfSet, {buf, load<u32>(rec + 516)});
    DmgDestroy(ctx, rec);
}

// u32 DamageColDef_KindFromHashD6164(s32 hash): the kind bit of a damage
// kind name hash: 0x788F788A 1, 0x928BCAD1 2, 0x928F4636 4, 0xBFAFE1AD 8,
// 0x87EBFA08 16, 0x923F4D1D 32, 0x03D6A020 128, 2405 256, 2591 512, 2406
// 1024, 2592 2048, 2407 4096, 2593 8192, anything else (0x767FB0D0
// included) 64. The compares are signed; r4 is left as the last constant
// the original loads (not read by either caller).
DECOMP(001D6164, DamageColDef_KindFromHashD6164) {
    struct Kv {
        u32 h, v;
    };
    static constexpr Kv kLow[5] = {{0xBFAFE1ADu, 8}, {0x928F4636u, 4}, {0x928BCAD1u, 2}, {0x923F4D1Du, 32}, {0x87EBFA08u, 16}};
    static constexpr Kv kHigh[3] = {{0x788F788Au, 1}, {0x767FB0D0u, 64}, {0x03D6A020u, 128}};
    const s32 h = arg<s32>(ctx, 0);
    if (h == 2405) {
        ret(ctx, 256u);
        return;
    }
    const bool high = h > 2405;
    const Kv* t = high ? kHigh : kLow;
    const u32 n = high ? 3 : 5;
    for (u32 k = 0; k < n; ++k) {
        ctx.gpr[4] = Sx64(t[k].h);
        if (static_cast<u32>(h) == t[k].h) {
            ret(ctx, t[k].v);
            return;
        }
    }
    u32 v = 64;
    if (high) {
        switch (h) {
            case 2593: v = 8192; break;
            case 2592: v = 2048; break;
            case 2591: v = 512; break;
            case 2407: v = 4096; break;
            case 2406: v = 1024; break;
            default: break;
        }
    }
    ret(ctx, v);
}
