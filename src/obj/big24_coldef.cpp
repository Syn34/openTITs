// Game code, large-function sweep part 24: a collision-object def reader
// (the 256-byte record of Rec_Init7576D8).
#include "../sly4.h"
#include "big24.h"

using namespace sly4;
using namespace sly4::big24;

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
constexpr u32 kZeroVec = 0x01164B40;          // Vec3 (the record's two default vectors)
constexpr u32 kRecArg26 = 0x0024A738;         // the word passed in slot 26 (record +244)
constexpr u32 kConsts = 0x001F9064;           // .text f32 100, 0.3, 0, 1, -1
constexpr u32 kTagSize = 0x63662D61u, kTagOffset = 0x1B89D5F3u;
constexpr u32 kTagRadius = 0xA274B629u;       // read only when +200 > 0

// The f32 at kConsts + off, as bits / as the double an lfs leaves.
u32 Cb(u32 off) { return load<u32>(kConsts + off); }
f64 Cf(u32 off) { return Lf(kConsts + off); }
}  // namespace

// u32 f(x, XmlCursor* cur, const Def* tmpl, const Def* parent, Def* out)
// (called by 001F9BA0, 001FE170): Damage_Read0C3910 with the same
// arguments, then a default 256-byte record d (Rec_Init7576D8(d,
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
// reported with XmlCursor_DocPath). r3: what the original leaves (the
// attribute 0 / the DocPath result / out + 6).
DECOMP(001F9078, ColDef_Read_001F9078) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1), tmpl = arg<u32>(ctx, 2);
    const u32 parent = arg<u32>(ctx, 3), out = arg<u32>(ctx, 4);
    Stack frame(ctx, 784);
    const u32 sp = frame.addr();
    const u32 d = sp + 432;      // the default record (256 bytes)
    const u32 vals = sp + 688;   // s32[6] sscanf results
    const u32 tb = sp + 712;     // u8 ReadBool result
    call(ctx, kDamageRead, {x, cur, tmpl, parent, out});
    const u32 m1 = Cb(16), one = Cb(12), zero = Cb(8);
    callf(ctx, kRecInit,
          {d, load<u32>(tmpl + 12), kStrEmpty, kStrEmpty, kStrEmpty, 0, kZeroVec, ctx.gpr[10],  // r8 0, r9, r10 (unread)
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                // slots 8..17 (the FPR arguments')
           Cb(4), m1, one, one, one, zero, one, one,                   // 18..25
           kRecArg26, 0,                                               // 26, 27
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   // 28..41
           m1, m1, 0, kZeroVec, kZeroVec},                             // 42..46
          {Cf(16), Cf(16), Cf(16), Cf(16), Cf(16), Cf(12), Cf(0), Cf(16), Cf(12), Cf(12), Cf(12), Cf(12), Cf(8)});
    if (parent != 0) {
        if ((load<u32>(parent + 20) & 0x20u) != 0) {
            for (u32 k = 0; k < 3; ++k) Cw(d + 128 + 4 * k, parent + 192 + 4 * k);
            const u32 b = load<u32>(parent + 296);
            if (b != 0) {
                store<u32>(d + 160, load<u32>(b + 120) & ~1u);
                Sf(d + 164, callf(ctx, kCompGetF32, {b + 224}, {}));
                const u32 b2 = load<u32>(parent + 296);
                Cw(d + 168, b2 + 140);
                Cw(d + 172, b2 + 144);
                store<u32>(d + 176, static_cast<u32>(load<u16>(b2 + 404)) << 16);
                store<u32>(d + 180, static_cast<u32>(load<u16>(b2 + 406)) << 16);
            }
            for (u32 k = 0; k < 15; ++k) Cw(d + 184 + 4 * k, parent + 224 + 4 * k);
            Cw(d + 244, parent + 284);
            const u32 pf = load<u32>(parent + 300);
            const u32 pb = load<u8>(parent + 301);
            u32 f = load<u32>(d + 252);
            f = (f & ~0xFF00u) | ((pf >> 16) & 0xFF00u);
            f = (f & ~0xF8u) | (pb & 0xF8u);
            store<u32>(d + 252, f);
            for (u32 k = 0; k < 6; ++k) store<u8>(d + 248 + k, load<u8>(parent + 288 + k));
        } else {
            call(ctx, kDocPath, {x});
        }
    }
    for (const u32 o : {16u, 20u}) {
        const u32 s = load<u32>(out + o);
        if (s == 0 || load<u8>(s) == 0) store<u32>(out + o, load<u32>(d + o));
    }
    call(ctx, kReadVec3, {cur, kTagSize, out + 128, d + 128});
    call(ctx, kReadVec3, {cur, kTagOffset, out + 144, d + 144});
    u32 name = static_cast<u32>(call(ctx, kFindAttr, {cur, kStrColObjName}));
    if (name == 0) name = load<u32>(d + 160);
    store<u32>(out + 160, name);
    auto rf = [&](u32 hash, u32 off) { callf(ctx, kReadF32, {cur, hash, out + off}, {Lf(d + off)}); };
    static constexpr u32 kA[10] = {0x0023FEF4u, 0xD9BA76FAu, 0x5AF2884Au, 0xA1FDE21Du, 0x8933D0E0u,
                                   0xCEDD379Au, 0x43B894DBu, 0x824F3B6Fu, 0x6B5A442Du, 0xAD6099F3u};
    for (u32 k = 0; k < 10; ++k) rf(kA[k], 164 + 4 * k);
    if (Lf(out + 200) > Cf(8)) rf(kTagRadius, 204);
    else store<u32>(out + 204, 0);
    static constexpr u32 kB[9] = {0x9AB17C61u, 0x9FF13A0Au, 0x4AD59CEFu, 0x825857B0u, 0x6C74B00Cu,
                                  0x86D97BF8u, 0x07EC64FFu, 0xBC97EF2Bu, 0x55B0EB25u};
    for (u32 k = 0; k < 9; ++k) rf(kB[k], 208 + 4 * k);
    call(ctx, kReadHash, {cur, 0x5B1E4A49u, out + 244, load<u32>(d + 244)});
    static constexpr u32 kBools[13] = {0x9EB9A73Du, 0xB5D9033Cu, 0x88D0410Du, 0xAB0BE3EDu, 0x926425E9u, 0x7A3FBFFBu, 0xB0D191A7u,
                                   0x0883B9B7u, 0xBA6158CDu, 0x8CB95A92u, 0xA8C3D7DAu, 0x1667C898u, 0xD13F28E4u};
    for (u32 k = 0; k < 13; ++k) {
        const u32 bit = 0x8000u >> k;
        call(ctx, kReadBool, {cur, kBools[k], tb, (load<u32>(d + 252) & bit) != 0 ? 1u : 0u});
        store<u32>(out + 252, (load<u32>(out + 252) & ~bit) | ((load<u8>(tb) & 1u) != 0 ? bit : 0u));
    }
    for (u32 k = 0; k < 6; ++k) store<u8>(out + 248 + k, load<u8>(d + 248 + k));
    const u32 tf = static_cast<u32>(call(ctx, kFindAttr, {cur, kStrTargetingFlags}));
    if (tf == 0) return;  // r3 = 0
    if (static_cast<s32>(call(ctx, kSscanf, {tf, kFmtSix, vals, vals + 4, vals + 8, vals + 12, vals + 16, vals + 20})) != 6) {
        call(ctx, kDocPath, {x});  // r3 = its result
        return;
    }
    for (u32 k = 0; k < 6; ++k) store<u8>(out + 248 + k, static_cast<u8>(load<u32>(vals + 4 * k)));
    ctx.gpr[3] = out + 6;  // leftover: the loop's byte pointer
}
