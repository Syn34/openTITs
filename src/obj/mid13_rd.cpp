// Game code, medium sweep part 13: def readers called by the part-12
// factories (Damage_Read0C3910 first, then their own attributes).
#include "mid13.h"

using namespace sly4;
using namespace sly4::mid13;

// void XmlDef_ReadC12A8(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// (called by XmlDef_NewC1780) Damage_Read0C3910(a0, c, x, tmpl, o ? o + 4
// : 0), then a defaults record on the stack (vtable 0x00B12034, DefHeadInit
// with x->12 and "" and f 100.0, +4 |= 0x00200000; three zero vec3s +128 /
// +144 / +160, +176..+184 0, f32 +188 -1, bytes +192..+194 0) whose +128..
// +194 come from tmpl's +192..+262 when tmpl has flag 0x00200000 in +20
// (c's document path noted when tmpl lacks it). Reads three vec3s
// (0x63662D61, 0xD40C4BEC, 0xD40C4AFE) into o+128 / +144 / +160 (the last
// two scaled by the degrees-to-radians 0x000C12A0 when found), four floats
// into o+176..+188 and three bools into bytes o+192..+194.
DECOMP(000C12A8, XmlDef_ReadC12A8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m13fn::Damage_Read0C3910, {a0, c, x, tmpl, o != 0 ? o + 4 : 0u});
    Stack frame(ctx, 208);
    const u32 rec = frame.addr();  // the original's sp + 112
    const u32 vz = mid11::kVecZero;
    mid10::DefHeadInit(rec + 4, load<u32>(x + 12), kStrEmpty637C, kStrEmpty637C, 100.0);
    store<u32>(rec, 0x00B12034u);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x00200000u);
    for (u32 k = 128; k <= 160; k += 16)
        for (u32 j = 0; j < 12; j += 4) CopyW(rec + k + j, vz + j);
    for (u32 k = 176; k <= 184; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 188, 0xBF800000u);
    for (u32 k = 192; k < 195; ++k) store<u8>(rec + k, 0);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 20) & 0x00200000u) {
            for (u32 k = 0; k < 12; k += 4) {
                CopyW(rec + 128 + k, tmpl + 192 + k);
                CopyW(rec + 144 + k, tmpl + 208 + k);
                CopyW(rec + 160 + k, tmpl + 224 + k);
            }
            for (u32 k = 0; k < 16; k += 4) CopyW(rec + 176 + k, tmpl + 244 + k);
            for (u32 k = 0; k < 3; ++k) store<u8>(rec + 192 + k, load<u8>(tmpl + 260 + k));
        } else {
            call(ctx, m11fn::XmlCursor_DocPath, {a0});
        }
    }
    constexpr u32 kDegToRad = 0x000C12A0;
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x63662D61u, o + 128, rec + 128});
    if (call(ctx, xfn::XmlCursor_ReadVec3, {c, 0xD40C4BECu, o + 144, rec + 144}) == 0) ScaleVec3(o + 144, LoadF(kDegToRad));
    if (call(ctx, xfn::XmlCursor_ReadVec3, {c, 0xD40C4AFEu, o + 160, rec + 160}) == 0) ScaleVec3(o + 160, LoadF(kDegToRad));
    static constexpr u32 kF[4] = {0x1E0DB5B2u, 0x6A396AA4u, 0x21641C05u, 0x43B894DBu};
    for (u32 k = 0; k < 4; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kF[k], o + 176 + 4 * k}, {LoadF(rec + 176 + 4 * k)});
    static constexpr u32 kB[3] = {0xBA6158CDu, 0xA499B8DAu, 0xF016EED0u};
    for (u32 k = 0; k < 3; ++k) call(ctx, xfn::XmlCursor_ReadBool, {c, kB[k], o + 192 + k, load<u8>(rec + 192 + k)});
}

// u32 XmlDef_Read16A828(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// (called by 0016ADC4) Damage_Read0C3910 with the same arguments; an empty
// o->16 / o->20 name becomes "" (0x00AC8C70); o->124 = the "colObjName"
// attribute (or the default). Defaults: fourteen floats from the pool
// 0x0016A818 (-1 x4, 1, -1, 1, -1, 0, 0.3, -1, 1, -1, 1) and hash 0, the
// name default ""; when tmpl has flag 0x400 in +20 they come from tmpl
// (with a body at tmpl->244: the first three floats from
// Comp_GetF32_922918(body + 224) and body +140 / +144, the name default
// body->120 & ~1; then tmpl +192..+232 and hash +240), else c's document
// path is noted. Reads the floats into o+128..+180, a hash (0x3A15BFAB)
// into o+188; byte o+24 = 1. Returns 1.
DECOMP(0016A828, XmlDef_Read16A828) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m13fn::Damage_Read0C3910, {a0, c, x, tmpl, o});
    // (The original also builds a DefHeadInit record on its stack here; only
    // its two name words, both 0x00AC8C70, are read back.)
    constexpr u32 kPool = 0x0016A818, kName = 0x00AC8C70;
    const f64 m1 = LoadF(kPool), p1 = LoadF(kPool + 4);
    f64 f[14] = {m1, m1, m1, m1, p1, m1, p1, m1, LoadF(kPool + 8), LoadF(kPool + 12), m1, p1, m1, p1};
    u32 nameDef = kName, h = 0;
    if (tmpl != 0) {
        if (load<u32>(tmpl + 20) & 0x400u) {
            const u32 body = load<u32>(tmpl + 244);
            if (body != 0) {
                nameDef = load<u32>(body + 120) & 0xFFFFFFFEu;
                f[0] = callf(ctx, m13fn::Comp_GetF32_922918, {body + 224}, {});
                const u32 b2 = load<u32>(tmpl + 244);
                f[1] = LoadF(b2 + 140);
                f[2] = LoadF(b2 + 144);
            }
            for (u32 k = 0; k < 11; ++k) f[3 + k] = LoadF(tmpl + 192 + 4 * k);
            h = load<u32>(tmpl + 240);
        } else {
            call(ctx, m11fn::XmlCursor_DocPath, {a0});
        }
    }
    const u32 n16 = load<u32>(o + 16);
    if (n16 == 0 || load<u8>(n16) == 0) store<u32>(o + 16, kName);
    const u32 n20 = load<u32>(o + 20);
    if (n20 == 0 || load<u8>(n20) == 0) store<u32>(o + 20, kName);
    u32 name = static_cast<u32>(call(ctx, m11fn::XmlCursor_FindAttrByName, {c, 0x00ACB334u}));  // "colObjName"
    if (name == 0) name = nameDef;
    store<u32>(o + 124, name);
    static constexpr u32 kF[14] = {0x0023FEF4u, 0xD9BA76FAu, 0x5AF2884Au, 0x43B894DBu, 0x824F3B6Fu, 0x6B5A442Du, 0xAD6099F3u,
                                   0xA274B629u, 0x9AB17C61u, 0x9FF13A0Au, 0x4AD59CEFu, 0x825857B0u, 0x6C74B00Cu, 0x86D97BF8u};
    for (u32 k = 0; k < 14; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kF[k], o + 128 + 4 * k}, {f[k]});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x3A15BFABu, o + 188, h});
    store<u8>(o + 24, 1);
    ret(ctx, 1u);
}

// void XmlDef_Read203718(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// (called by 00203BE4) Damage_Read0C3910 with the same arguments, then the
// defaults (enum index 6, s32 1, floats from the pool 0x002036FC: -1, 0.15,
// 5, 0.9, 0.5, 1, 0, 0; hashes 0, 0), from tmpl when it has flag 0x1000 in
// +20 (byte +192, s16 +194, f32 +196; with a body at tmpl->200: *(body->16)
// +16, Comp_GetF32_922918(body + 224), body +144 / +140 and the three
// u16s body +510 / +404 / +406 as the high halves of floats; the first hash
// String_HashNoCase(ReturnK_004440E8(tmpl->204)), the second tmpl->208),
// else c's document path is noted. Reads an enum (0x0027873A, the six
// hashes at 0x00AD2C54) into o+124, an s32 (0xBF93119A) into o+128, eight
// floats into o+132..+160 and two hashes (0x5D1A6CBA, 0xA77EB5E9) into
// o+164 / o+168.
DECOMP(00203718, XmlDef_Read203718) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m13fn::Damage_Read0C3910, {a0, c, x, tmpl, o});
    // (The original also builds a DefHeadInit record on its stack here,
    // never read back.)
    constexpr u32 kPool = 0x002036FC;
    const f64 z = LoadF(kPool);
    f64 f[8] = {LoadF(kPool + 4), LoadF(kPool + 8), LoadF(kPool + 12), LoadF(kPool + 16), LoadF(kPool + 20), LoadF(kPool + 24), z, z};
    u32 en = 6, h1 = 0, h2 = 0;
    s32 s = 1;
    if (tmpl != 0) {
        if (load<u32>(tmpl + 20) & 0x1000u) {
            en = load<u8>(tmpl + 192);
            s = load<s16>(tmpl + 194);
            f[0] = LoadF(tmpl + 196);
            const u32 body = load<u32>(tmpl + 200);
            if (body != 0) {
                f[1] = LoadF(load<u32>(body + 16) + 16);
                f[2] = callf(ctx, m13fn::Comp_GetF32_922918, {body + 224}, {});
                const u32 b2 = load<u32>(tmpl + 200);
                f[4] = LoadF(b2 + 140);
                f[3] = LoadF(b2 + 144);
                // (The u16s go through the stack as words hi << 16 and back by lfs.)
                f[5] = LfsF(static_cast<u32>(load<u16>(b2 + 510)) << 16);
                f[6] = LfsF(static_cast<u32>(load<u16>(b2 + 404)) << 16);
                f[7] = LfsF(static_cast<u32>(load<u16>(b2 + 406)) << 16);
            }
            const u32 t = static_cast<u32>(call(ctx, m13fn::ReturnK_004440E8, {load<u32>(tmpl + 204)}));
            h1 = static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {t}));
            h2 = load<u32>(tmpl + 208);
        } else {
            call(ctx, m11fn::XmlCursor_DocPath, {a0});
        }
    }
    call(ctx, m13fn::Xml_ReadEnum, {c, 0x0027873Au, o + 124, 0x00AD2C54u, 6, en});
    call(ctx, xfn::XmlCursor_ReadS32, {c, 0xBF93119Au, o + 128, static_cast<u64>(static_cast<s64>(s))});
    static constexpr u32 kF[8] = {0x43B894DBu, 0x2060DDF2u, 0x0023FEF4u, 0x5AF2884Au, 0xD9BA76FAu, 0xCEDD379Au, 0xA1FDE21Du, 0x8933D0E0u};
    for (u32 k = 0; k < 8; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kF[k], o + 132 + 4 * k}, {f[k]});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x5D1A6CBAu, o + 164, h1});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xA77EB5E9u, o + 168, h2});
}
