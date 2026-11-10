// Game code, large-function sweep part 2: the damage / collision def
// reader 000C85A8 (18 callers: the attack and projectile def readers),
// built on Damage_Read0C3910 (big1_xml.cpp).
#include "big2.h"

using namespace sly4;
using namespace sly4::big2;

namespace b2d {
constexpr u32 Damage_Read0C3910 = 0x000C3910;      // (x, XmlCursor*, const Damage* tmpl, const Def* parent, Damage* out) -> u32
constexpr u32 XmlCursor_DocPath = 0x004F8944;      // (x) -> the cursor's document path (the original's error report)
constexpr u32 DamageDef_Construct = 0x00745D88;    // (DamageDef*, u32, const char*, const char*, ..., f1..f13 and 39 parameter slots)
constexpr u32 ColVec_Push = 0x00745564;            // (PtrVector* of 160-byte ColRecords, const ColRecord*): copy the fields to a new element
constexpr u32 sscanf = 0x0071EC78;                 // (s, fmt, ...) -> fields converted
constexpr u32 SmallAlloc = 0x0055238C;             // (size)
constexpr u32 SmallFree = 0x00552420;              // (p, size)
constexpr u32 PtrVector_Construct = 0x005516E0;    // (vec)
constexpr u32 PtrVector_Clear = 0x0054EC1C;        // (vec)
constexpr u32 Nop = 0x0054EC18;                    // (p)
constexpr u32 kStrEmpty = 0x00AC6790;              // ""
constexpr u32 kStrColObjName = 0x00AC67C4;         // "colObjName"
constexpr u32 kStrTargetingFlags = 0x00AC6868;     // "targetingFlags"
constexpr u32 kFmtTargetingFlags = 0x00AC6878;     // "%d,%d,%d,%d,%d,%d"
constexpr u32 kColDefault112 = 0x01013B00;         // f32: a ColRecord's default +112
constexpr u32 kTagCol = 0x6364B153u;               // the collision record children
constexpr u32 kMaxRecords = 32;                    // a vector holding 32 records takes no more
constexpr u32 kRecBytes = 160;
// The constants DamageDef_Construct is called with (the .text table at
// 0x000C8590: 100.0, 0.0, 0.3, 1.0, -1.0).
constexpr u32 kF100 = 0x42C80000u, kF0 = 0x00000000u, kF03 = 0x3E99999Au, kF1 = 0x3F800000u, kFm1 = 0xBF800000u;
}  // namespace b2d

namespace {

using mid2::kSlotDefaultVec;

inline f64 F(u32 bits) { return std::bit_cast<f64>(LfsBits(bits)); }
inline void Copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// XmlCursor_ReadF32 into p with the default read from dflt.
void ReadF32From(PPUContext& ctx, u32 cur, u32 hash, u32 p, u32 dflt) {
    callf(ctx, b2fn::XmlCursor_ReadF32, {cur, hash, p}, {F(load<u32>(dflt))});
}

// The inlined ColRecord default constructor (160 bytes): the vec3s +0 /
// +16 / +48 / +128 from kSlotDefaultVec, +64 / +84 / +116 = 1.0, +72..+80 /
// +88 / +96..+108 = -1.0, +112 from kColDefault112, the hashes / pointers
// +32 / +36 / +68 / +120 / +144 cleared and the four flag bits (+124 bits
// 31..28) cleared; the padding words (+12, +28, +40, +44, +60, +92, +140,
// +148..+156) and +124's low bits keep what the buffer held, as the
// original (ColVec_Push never copies them). f1 = vec[0], as the original
// leaves it.
void RecDefaults(PPUContext& ctx, u32 r) {
    for (const u32 o : {0u, 16u, 48u, 128u})
        for (u32 k = 0; k < 3; ++k) Copy32(r + o + 4 * k, kSlotDefaultVec + 4 * k);
    for (const u32 o : {32u, 36u, 68u, 120u, 144u}) store<u32>(r + o, 0);
    for (const u32 o : {64u, 84u, 116u}) store<u32>(r + o, b2d::kF1);
    for (const u32 o : {72u, 76u, 80u, 88u, 96u, 100u, 104u, 108u}) store<u32>(r + o, b2d::kFm1);
    Copy32(r + 112, b2d::kColDefault112);
    store<u32>(r + 124, load<u32>(r + 124) & 0x0FFFFFFFu);
    ctx.set_fpr_bits(1, LfsBits(load<u32>(kSlotDefaultVec)));
}

// The fields of a ColRecord e both copy loops take: +0 / +16 / +48
// vec3s, +64..+88, +96..+120 and the four flag bits of +124.
void RecCopyCommon(u32 r, u32 e) {
    for (const u32 o : {0u, 4u, 8u, 16u, 20u, 24u, 48u, 52u, 56u, 64u, 68u, 72u, 76u, 80u, 84u, 88u, 96u, 100u, 104u, 108u, 112u, 116u, 120u})
        Copy32(r + o, e + o);
    store<u32>(r + 124, (load<u32>(r + 124) & 0x0FFFFFFFu) | (load<u32>(e + 124) & 0xF0000000u));
}

// Whether a ColRecord at e is worth keeping: a non-empty name (+32), or
// a position (+0) that differs bitwise from kSlotDefaultVec.
bool RecUsed(u32 e) {
    const u32 name = load<u32>(e + 32);
    if (name != 0 && load<u8>(name) != 0) return true;
    return load<u64>(e) != load<u64>(kSlotDefaultVec) || load<u32>(e + 8) != load<u32>(kSlotDefaultVec + 8);
}

// The PtrVector at *slot, made on first use (SmallAlloc(20) stored to
// the slot even when 0; constructed with 160-byte elements), re-read.
u32 RecVector(PPUContext& ctx, u32 slot) {
    u32 v = load<u32>(slot);
    if (v != 0) return v;
    v = static_cast<u32>(call(ctx, b2d::SmallAlloc, {20}));
    store<u32>(slot, v);
    if (v == 0) return 0;
    call(ctx, b2d::PtrVector_Construct, {v});
    store<u16>(v + 6, static_cast<u16>(b2d::kRecBytes));
    return load<u32>(slot);
}

// A vector already holding kMaxRecords records (or more) takes no more.
bool RecVectorFull(u32 v) { return v != 0 && load<u16>(v + 8) >= b2d::kMaxRecords; }

// XmlCursor_ReadBool into bit (31 - k) of the word at p (through the
// stack byte tb; the default is the current bit of def).
void ReadBit(PPUContext& ctx, u32 cur, u32 hash, u32 tb, u32 def, u32 k, u32 p) {
    call(ctx, b2fn::XmlCursor_ReadBool, {cur, hash, tb, (def >> (31 - k)) & 1u});
    const u32 m = 0x80000000u >> k;
    store<u32>(p, (load<u32>(p) & ~m) | ((load<u8>(tb) & 1u) ? m : 0u));
}

// The original's frame offsets (sp + o) mapped onto ours.
struct Frame {
    u32 base;
    u32 at(u32 o) const { return base + o - 112; }
};

}  // namespace

// u32 DamageColDef_Read_000C85A8(x, XmlCursor* cur, const DamageDef* tmpl,
// const Def* parent, DamageDef* out): Damage_Read0C3910 with the same
// arguments, then a default DamageDef d built on the stack
// (DamageDef_Construct); when the parent's +20 has bit 0x4, d takes the
// parent's values (+136..+216, the six bytes +128, the flag bits 31..22 of
// +220) and a copy of each of the parent's 160-byte ColRecords (+192, a
// PtrVector) in d's own vector (d +124), otherwise a set parent is
// reported (XmlCursor_DocPath). Then out: the names +16 / +20 from d when
// empty, the float / hash / vec3 / floats +136..+216 (d's values as the
// defaults; +172 = 0 unless +168 > 0), the bools into bits 31..22 of +220,
// the six bytes +128 from d or from "targetingFlags" ("%d,%d,%d,%d,%d,%d");
// then one ColRecord per 0x6364B153 child into out +124 (up to 32; children
// with neither a name ("colObjName") nor a position away from
// kSlotDefaultVec are skipped); finally d's records are appended to out's
// (unless the bool 0xFE139BBC is false and out already has some) and d's
// vector is freed. The original's r3 (ignored by every caller): SmallFree's
// result, or the bool when d had no vector.
DECOMP(000C85A8, DamageColDef_Read_000C85A8) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1), tmpl = arg<u32>(ctx, 2);
    const u32 parent = arg<u32>(ctx, 3), out = arg<u32>(ctx, 4);
    Stack stack(ctx, 1160 - 112);
    const Frame sp{stack.addr()};
    for (u32 k = 0; k < 1160 - 112; k += 8) store<u64>(stack.addr() + k, 0);  // (the original's buffers hold stale stack)
    const u32 d = sp.at(848);       // the default DamageDef (224 bytes)
    const u32 tb = sp.at(1096), tb2 = sp.at(1097);
    const u32 dvec = d + 124;       // d's ColRecord vector

    call(ctx, b2d::Damage_Read0C3910, {x, cur, tmpl, parent, out});
    // DamageDef_Construct(d, tmpl->+12, "", "", (r7), (r8), 0x9260C26E,
    // kSlotDefaultVec, f1..f13 = 100, -1, 1, 1, 1, 1, 0, 0.3, -1, 1, 1, 1, 1;
    // slots 19..22 = 1.0, 23..30 = 0, 31 = 1, 32..35 = 0, 36 / 37 = -1.0,
    // 38 = 0). r7 = 1 and r8 = what Damage_Read0C3910 left, as the
    // original's registers hold them (neither is read).
    callf(ctx, b2d::DamageDef_Construct,
          {d, load<u32>(tmpl + 12), b2d::kStrEmpty, b2d::kStrEmpty, 1, ctx.gpr[8], 0x9260C26Eu, kSlotDefaultVec,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   // slots 8..18 (f1..f11's, unwritten)
           b2d::kF1, b2d::kF1, b2d::kF1, b2d::kF1,            // 19..22
           0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,             // 23..35
           b2d::kFm1, b2d::kFm1, 0},                          // 36..38
          {F(b2d::kF100), F(b2d::kFm1), F(b2d::kF1), F(b2d::kF1), F(b2d::kF1), F(b2d::kF1), F(b2d::kF0), F(b2d::kF03),
           F(b2d::kFm1), F(b2d::kF1), F(b2d::kF1), F(b2d::kF1), F(b2d::kF1)});

    if (parent != 0) {
        if ((load<u32>(parent + 20) & 0x4u) == 0) {
            call(ctx, b2d::XmlCursor_DocPath, {x});
        } else {
            store<u32>(d + 220, (load<u32>(d + 220) & 0x003FFFFFu) | (load<u32>(parent + 300) & 0xFFC00000u));
            Copy32(d + 136, parent + 204);
            Copy32(d + 140, parent + 208);
            for (u32 k = 0; k < 3; ++k) Copy32(d + 144 + 4 * k, parent + 224 + 4 * k);
            for (u32 k = 0; k < 15; ++k) Copy32(d + 160 + 4 * k, parent + 240 + 4 * k);
            for (u32 k = 0; k < 6; ++k) store<u8>(d + 128 + k, load<u8>(parent + 196 + k));
            ctx.set_fpr_bits(1, LfsBits(load<u32>(parent + 204)));
            const u32 p = sp.at(368);
            u32 pv = load<u32>(parent + 192);
            for (u32 i = 0, off = 0; i < (pv != 0 ? load<u16>(pv + 8) : 0u); ++i, off += b2d::kRecBytes) {
                RecDefaults(ctx, p);
                const u32 e = load<u32>(pv) + off;
                RecCopyCommon(p, e);
                for (const u32 o : {36u, 128u, 132u, 136u, 144u}) Copy32(p + o, e + o);
                ctx.set_fpr_bits(1, LfsBits(load<u32>(e + 24)));
                const u32 obj = load<u32>(e + 32);
                store<u32>(p + 32, obj != 0 ? load<u32>(obj + 120) & ~1u : 0u);
                call(ctx, b2d::ColVec_Push, {RecVector(ctx, dvec), p});
                pv = load<u32>(parent + 192);
            }
        }
    }

    store<u8>(tb, 0);
    for (const u32 o : {16u, 20u}) {
        const u32 s = load<u32>(out + o);
        if (s == 0 || load<u8>(s) == 0) store<u32>(out + o, load<u32>(d + o));
    }
    ReadF32From(ctx, cur, 0x43B894DBu, out + 136, d + 136);
    call(ctx, b2fn::XmlCursor_ReadHash, {cur, 0x5B1E4A49u, out + 140, load<u32>(d + 140)});
    call(ctx, b2fn::XmlCursor_ReadVec3, {cur, 0x354C5B93u, out + 144, d + 144});
    ReadF32From(ctx, cur, 0x824F3B6Fu, out + 160, d + 160);
    ReadF32From(ctx, cur, 0x6B5A442Du, out + 164, d + 164);
    ReadF32From(ctx, cur, 0xAD6099F3u, out + 168, d + 168);
    {
        const f64 v = F(load<u32>(out + 168));
        ctx.fpr[1] = v;
        if (v > 0.0) ReadF32From(ctx, cur, 0xA274B629u, out + 172, d + 172);
        else store<u32>(out + 172, 0);
    }
    static constexpr u32 kF[11] = {0x9AB17C61u, 0x9FF13A0Au, 0x4AD59CEFu, 0x825857B0u, 0x6C74B00Cu, 0x86D97BF8u,
                                   0x30003446u, 0x3E44F9F5u, 0xFDA86B42u, 0x0D5F7C08u, 0x55B0EB25u};
    for (u32 k = 0; k < 11; ++k) ReadF32From(ctx, cur, kF[k], out + 176 + 4 * k, d + 176 + 4 * k);
    static constexpr u32 kB[10] = {0xBA6158CDu, 0xA499B8DAu, 0xB0D191A7u, 0xF03E41A4u, 0xBD23F15Du,
                                   0x8CB95A92u, 0xA78B0427u, 0x0883B9B7u, 0x1667C898u, 0xB2B78B0Fu};
    for (u32 k = 0; k < 10; ++k) ReadBit(ctx, cur, kB[k], tb, load<u32>(d + 220), k, out + 220);
    for (u32 k = 0; k < 6; ++k) store<u8>(out + 128 + k, load<u8>(d + 128 + k));
    const u32 tf = static_cast<u32>(call(ctx, b2fn::XmlCursor_FindAttrByName, {cur, b2d::kStrTargetingFlags}));
    if (tf != 0) {
        const u32 v = sp.at(1072);
        if (static_cast<u32>(call(ctx, b2d::sscanf, {tf, b2d::kFmtTargetingFlags, v, v + 4, v + 8, v + 12, v + 16, v + 20})) == 6) {
            for (u32 k = 0; k < 6; ++k) store<u8>(out + 128 + k, static_cast<u8>(load<u32>(v + 4 * k)));
        } else {
            call(ctx, b2d::XmlCursor_DocPath, {x});
        }
    }

    // The ColRecord children.
    const u32 r = sp.at(528);
    for (u32 c = static_cast<u32>(call(ctx, b2fn::XmlCursor_FirstChild, {cur, b2d::kTagCol})); c != 0;
         c = static_cast<u32>(call(ctx, b2fn::XmlCursor_NextSibling, {c, b2d::kTagCol}))) {
        if (RecVectorFull(load<u32>(out + 124))) {
            call(ctx, b2d::XmlCursor_DocPath, {x});
            continue;
        }
        RecDefaults(ctx, r);
        call(ctx, b2fn::XmlCursor_ReadVec3, {c, 0x63662D61u, r, r});
        store<u32>(r + 32, static_cast<u32>(call(ctx, b2fn::XmlCursor_FindAttrByName, {c, b2d::kStrColObjName})));
        if (!RecUsed(r)) continue;
        call(ctx, b2fn::XmlCursor_ReadVec3, {c, 0x1B89D5F3u, r + 16, r + 16});
        ReadHash(ctx, c, 0x75849CFEu, r + 36);
        static constexpr u32 kA[6] = {0x29F45D46u, 0xA1FDE21Du, 0xE2154ADBu, 0x8933D0E0u, 0x0023FEF4u, 0xB89310FDu};
        for (u32 k = 0; k < 6; ++k) ReadF32(ctx, c, kA[k], r + 96 + 4 * k);
        ReadHash(ctx, c, 0x725B9AF2u, r + 120);
        static constexpr u32 kRB[4] = {0xF8E455ACu, 0xBC07DCA3u, 0x47C04D53u, 0x306B6E24u};
        for (u32 k = 0; k < 4; ++k) ReadBit(ctx, c, kRB[k], tb, load<u32>(r + 124), k, r + 124);
        if (load<u8>(tb) & 1u) {
            call(ctx, b2fn::XmlCursor_ReadVec3, {c, 0x07A80994u, r + 48, r + 48});
            static constexpr u32 kV[7] = {0x46206B53u, 0xE3E14C49u, 0x471281EFu, 0x9538DE99u, 0x9533AB5Au, 0x040C1D75u, 0x69FE7274u};
            for (u32 k = 0; k < 7; ++k) ReadF32(ctx, c, kV[k], r + 64 + 4 * k);
        }
        call(ctx, b2fn::XmlCursor_ReadVec3, {c, 0xCC0D6864u, r + 128, r + 128});
        ReadHash(ctx, c, 0x5B1E4A49u, r + 144);
        call(ctx, b2d::ColVec_Push, {RecVector(ctx, out + 124), r});
    }

    store<u8>(tb2, 0);
    call(ctx, b2fn::XmlCursor_ReadBool, {cur, 0xFE139BBCu, tb2, 0});
    u32 dv = load<u32>(dvec);
    if (dv != 0 && load<u16>(dv + 8) != 0) {
        const u32 ov = load<u32>(out + 124);
        if (load<u8>(tb2) != 0 || ov == 0 || load<u16>(ov + 8) == 0) {
            const u32 q = sp.at(688);
            for (u32 i = 0, off = 0;; ++i, off += b2d::kRecBytes) {
                dv = load<u32>(dvec);
                if (i >= (dv != 0 ? load<u16>(dv + 8) : 0u)) break;
                const u32 v = load<u32>(out + 124);
                if (RecVectorFull(v)) {
                    call(ctx, b2d::XmlCursor_DocPath, {x});
                    continue;
                }
                RecDefaults(ctx, q);
                const u32 e = load<u32>(dv) + off;
                if (!RecUsed(e)) continue;
                RecCopyCommon(q, e);
                Copy32(q + 32, e + 32);
                Copy32(q + 36, e + 36);
                ctx.set_fpr_bits(1, LfsBits(load<u32>(e + 88)));
                call(ctx, b2d::ColVec_Push, {v != 0 ? v : RecVector(ctx, out + 124), q});
            }
        }
    }
    if (dv == 0) {
        ret(ctx, load<u8>(tb2));
        return;
    }
    call(ctx, b2d::PtrVector_Clear, {dv});
    call(ctx, b2d::Nop, {dv});
    call(ctx, b2d::SmallFree, {load<u32>(dvec), 20});
}
