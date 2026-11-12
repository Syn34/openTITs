// Game code, large-function sweep part 24: a collision-object def reader
// (the 256-byte record of Rec_Init7576D8).
#include "big24.h"

namespace sly4::obj {

namespace {
constexpr u32 kDamageRead = 0x000C3910;       // Damage_Read0C3910 (x, XmlCursor*, tmpl, parent, out)
constexpr u32 kRecInit = 0x007576D8;          // Rec_Init7576D8 (Rec*, a, b, c, d, f1..f13, stack slots 18..46)
constexpr u32 kDocPath = 0x004F8944;          // XmlCursor_DocPath (x): the error report
constexpr u32 kFindAttr = 0x004F9628;         // XmlCursor_FindAttrByName (cur, const char*) -> const char*
constexpr u32 kReadBool = 0x004F986C;         // XmlCursor_ReadBool (cur, hash, u8* out, u8 default)
constexpr u32 kReadF32 = 0x004F9B20;          // XmlCursor_ReadF32 (cur, hash, f32* out; f1 default)
constexpr u32 kReadVec3 = 0x004F9E0C;         // XmlCursor_ReadVec3 (cur, hash, Vec3* out, const Vec3* default)
constexpr u32 kReadHash = 0x004FA14C;         // XmlCursor_ReadHash (cur, hash, u32* out, u32 default)
constexpr u32 kSscanf = 0x0071EC78;           // sscanf_0071EC78
constexpr u32 kCompGetF32 = 0x00922918;       // Comp_GetF32_922918 (body + 224) -> f1
constexpr u32 kStrEmpty = 0x00AD2B74;         // ""
constexpr u32 kStrColObjName = 0x00AD2B80;    // "colObjName"
constexpr u32 kStrTargetingFlags = 0x00AD2B90;  // "targetingFlags"
constexpr u32 kFmtSix = 0x00AD2BA0;           // "%d,%d,%d,%d,%d,%d"
constexpr u32 kRecArg26 = 0x0024A738;         // the word passed in slot 26 (record +244)
constexpr u32 kConsts = 0x001F9064;           // .text f32 100, 0.3, 0, 1, -1
constexpr u32 kTagSize = 0x63662D61u, kTagOffset = 0x1B89D5F3u;
constexpr u32 kTagRadius = 0xA274B629u;       // read only when +200 > 0

// The 256-byte collision def record (out, the stack default d, tmpl).
struct ColDef {
    pad<12> _0;
    be_u32 w12;         // +12
    be_u32 name16;      // +16
    be_u32 name20;      // +20
    pad<104> _24;
    Vec4 size;          // +128
    Vec4 offset;        // +144
    be_u32 name160;     // +160
    be_f32 f[20];       // +164 .. +240
    be_u32 hash;        // +244
    u8 b248[4];         // +248 .. +251
    WordBytes flags;    // +252: bits 0x8000 >> k; its top two bytes are bytes 4 / 5 of +248
};
static_assert(sizeof(ColDef) == 256 && offsetof(ColDef, name160) == 160 && offsetof(ColDef, hash) == 244);
// Byte k (0..5) of the six bytes at +248.
u8& Tail(ColDef& c, u32 k) { return k < 4 ? c.b248[k] : c.flags.b[k - 4]; }

// The parent def.
struct ColParent {
    pad<20> _0;
    be_u32 flags;       // +20: 0x20 inherit
    pad<168> _24;
    Vec3 v192;          // +192
    pad<20> _204;
    be_u32 w224[15];    // +224 .. +280
    be_u32 w284;        // +284
    u8 b288[6];         // +288 .. +293
    pad<2> _294;
    be_u32 body;        // +296: ColBody*
    WordBytes w300;     // +300 (byte +301 its second)
};
static_assert(offsetof(ColParent, v192) == 192 && offsetof(ColParent, w224) == 224 && offsetof(ColParent, body) == 296);
struct ColBody {
    pad<120> _0;
    be_u32 w120;        // +120
    pad<16> _124;
    be_u32 w140;        // +140
    be_u32 w144;        // +144
    pad<76> _148;
    pad<4> comp;        // +224: Comp_GetF32_922918's argument
    pad<176> _228;
    be_u16 h404;        // +404
    be_u16 h406;        // +406
};
static_assert(offsetof(ColBody, w140) == 140 && offsetof(ColBody, comp) == 224 && offsetof(ColBody, h404) == 404);
// The original's locals (sp offsets).
struct ColLocals {
    ColDef d;           // sp+432: the default record
    be_s32 vals[6];     // sp+688: sscanf results
    u8 tb;              // sp+712: ReadBool result
};
static_assert(offsetof(ColLocals, vals) == 256 && offsetof(ColLocals, tb) == 280);

// The f32 at kConsts + off, as bits / as the double an lfs leaves.
u32 Cb(u32 off) { return global<be_u32>(kConsts + off); }
f64 Cf(u32 off) { return lf(global<be_f32>(kConsts + off)); }
}  // namespace

// void f(x, XmlCursor* cur, const Def* tmpl, const Def* parent, Def* out)
// (called by 001F9BA0, 001FE170; neither reads r3): Damage_Read0C3910 with
// the same arguments, then a default 256-byte record d (Rec_Init7576D8(d,
// tmpl->+12, "", "", ""; f1-f5 -1, f6 1, f7 100, f8 -1, f9-f12 1, f13 0;
// slots 18-25 0.3, -1, 1, 1, 1, 0, 1, 1; 26 0x0024A738; 42/43 -1; 45/46
// 0x01164B40; the rest 0)). With a parent: when parent->+20 has 0x20, d
// takes the parent's values (+128 vec3 from +192; with a body b =
// parent->+296: +160 = b->+120 & ~1, +164 = Comp_GetF32_922918(b + 224),
// +168/+172 from b->+140/+144, +176/+180 the u16s b->+404/+406 << 16;
// +184..+240 from +224..+280, +244 from +284, flag bits 0xFF00 / 0xF8 of
// +252 from the parent's +300 / +301, bytes +248..+253 from +288), else
// XmlCursor_DocPath(x). Then out's empty name strings +16 / +20 take d's,
// the vec3s +128 (size) / +144, +160 = the "colObjName" attribute (else
// d's), 24 floats +164..+240 (+204 only when +200 > 0, else 0), hash +244,
// 13 bools into bits 0x8000 >> k of +252 (defaults from d's bits), bytes
// +248..+253 from d (the last two overwrite +252's high half), and
// "targetingFlags" ("%d,%d,%d,%d,%d,%d" into +248..+253; a bad string is
// reported with XmlCursor_DocPath).
void ColDef_Read_001F9078(u32 x, u32 cur, ColDef* tmpl, ColParent* parent, ColDef* out) {
    GuestFrame frame(784);
    ColLocals& L = *frame.as<ColLocals>(432);
    ColDef& d = L.d;
    call(kDamageRead, x, cur, tmpl, parent, out);
    const u32 m1 = Cb(16), one = Cb(12), zero = Cb(8);
    const f64 fm1 = Cf(16), f1 = Cf(12), f100 = Cf(0), f0 = Cf(8);
    call(kRecInit, &d, tmpl->w12.get(), kStrEmpty, kStrEmpty, kStrEmpty,
         fm1, fm1, fm1, fm1, fm1, f1, f100, fm1, f1, f1, f1, f1, f0,  // f1..f13 (slots 5..17)
         Cb(4), m1, one, one, one, zero, one, one,                    // 18..25
         kRecArg26, 0u,                                               // 26, 27
         0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,      // 28..41
         m1, m1, 0u, kZeroVec, kZeroVec);                             // 42..46
    if (parent != nullptr) {
        if ((parent->flags & 0x20u) != 0) {
            CopyXYZ(d.size, parent->v192);
            if (const u32 b = parent->body) {
                d.name160 = gm<ColBody>(b).w120 & ~1u;
                d.f[0] = static_cast<f32>(call<f64>(kCompGetF32, &gm<ColBody>(b).comp));
                const ColBody& b2 = gm<ColBody>(parent->body);
                set_bits(d.f[1], b2.w140);
                set_bits(d.f[2], b2.w144);
                set_bits(d.f[3], static_cast<u32>(b2.h404) << 16);
                set_bits(d.f[4], static_cast<u32>(b2.h406) << 16);
            }
            for (u32 k = 0; k < 15; ++k) set_bits(d.f[5 + k], parent->w224[k]);
            d.hash = parent->w284.get();
            const u32 pf = parent->w300.w;
            const u32 pb = parent->w300.b[1];
            u32 f = d.flags.w;
            f = (f & ~0xFF00u) | ((pf >> 16) & 0xFF00u);
            f = (f & ~0xF8u) | (pb & 0xF8u);
            d.flags.w = f;
            for (u32 k = 0; k < 6; ++k) Tail(d, k) = parent->b288[k];
        } else {
            call(kDocPath, x);
        }
    }
    for (be_u32 ColDef::*m : {&ColDef::name16, &ColDef::name20}) {
        const u32 s = out->*m;
        if (s == 0 || gm<u8>(s) == 0) out->*m = (d.*m).get();
    }
    call(kReadVec3, cur, kTagSize, &out->size, &d.size);
    call(kReadVec3, cur, kTagOffset, &out->offset, &d.offset);
    u32 name = call<u32>(kFindAttr, cur, kStrColObjName);
    if (name == 0) name = d.name160;
    out->name160 = name;
    // out->f[k] from the attribute `hash` (default d's).
    auto rf = [&](u32 hash, u32 k) { call(kReadF32, cur, hash, &out->f[k], lf(d.f[k])); };
    static constexpr u32 kA[10] = {0x0023FEF4u, 0xD9BA76FAu, 0x5AF2884Au, 0xA1FDE21Du, 0x8933D0E0u,
                                   0xCEDD379Au, 0x43B894DBu, 0x824F3B6Fu, 0x6B5A442Du, 0xAD6099F3u};
    for (u32 k = 0; k < 10; ++k) rf(kA[k], k);                     // +164 .. +200
    if (lf(out->f[9]) > Cf(8)) rf(kTagRadius, 10);                 // +204
    else set_bits(out->f[10], 0u);
    static constexpr u32 kB[9] = {0x9AB17C61u, 0x9FF13A0Au, 0x4AD59CEFu, 0x825857B0u, 0x6C74B00Cu,
                                  0x86D97BF8u, 0x07EC64FFu, 0xBC97EF2Bu, 0x55B0EB25u};
    for (u32 k = 0; k < 9; ++k) rf(kB[k], 11 + k);                 // +208 .. +240
    call(kReadHash, cur, 0x5B1E4A49u, &out->hash, d.hash.get());
    static constexpr u32 kBools[13] = {0x9EB9A73Du, 0xB5D9033Cu, 0x88D0410Du, 0xAB0BE3EDu, 0x926425E9u, 0x7A3FBFFBu, 0xB0D191A7u,
                                       0x0883B9B7u, 0xBA6158CDu, 0x8CB95A92u, 0xA8C3D7DAu, 0x1667C898u, 0xD13F28E4u};
    for (u32 k = 0; k < 13; ++k) {
        const u32 bit = 0x8000u >> k;
        call(kReadBool, cur, kBools[k], &L.tb, (d.flags.w & bit) != 0 ? 1u : 0u);
        out->flags.w = (out->flags.w & ~bit) | ((L.tb & 1u) != 0 ? bit : 0u);
    }
    for (u32 k = 0; k < 6; ++k) Tail(*out, k) = Tail(d, k);
    const u32 tf = call<u32>(kFindAttr, cur, kStrTargetingFlags);
    if (tf == 0) return;
    be_s32* v = L.vals;
    if (call<s32>(kSscanf, tf, kFmtSix, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) {
        call(kDocPath, x);
        return;
    }
    for (u32 k = 0; k < 6; ++k) Tail(*out, k) = static_cast<u8>(v[k].get());
}

}  // namespace sly4::obj

NATIVE(001F9078, sly4::obj::ColDef_Read_001F9078)
