// Game code, medium sweep part 7: the spawn-query def readers of the
// 0x000CBE10 family. Each reads the four base fields (+220 f32, +224 /
// +228 hashes, +232 bool) and then walks the child elements: a child
// with bit 0x80 of its flags byte (+24) and the element hash 0xC3B709C0
// is read into the PtrVector at +180, one with 0xB921AB51 into +200
// (0x0049EF40). The factories make the def first (Mem_Alloc 240,
// SpawnQuery_Construct0CC188); quirk: when the allocation fails the reads
// still run, on def = 0.
#include "mid7.h"

using namespace sly4;
using namespace sly4::mid7;

namespace {

namespace qfn {
constexpr u32 SpawnQuery_Construct = 0x000CC188;  // (q, name, tmpl, a3, m, pos, u8, u8, stack 9th / 11th; f1) (SpawnQuery_Construct0CC188)
constexpr u32 QueryDef_ReadChild = 0x0049EF40;    // (node, PtrVector*) (name?)
}  // namespace qfn

constexpr u32 kIdentity = 0x01164BD0;  // f32[4][4]
constexpr u32 kVec4B30 = 0x01164B30;   // f32[4] passed as the position

// The base fields and the child list of the def at o, read from cursor c.
void QueryDefReadBody(PPUContext& ctx, u32 c, u32 o) {
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();
    XF32(ctx, c, 0xA7D74568, o + 220);
    XHash(ctx, c, 0xF89FDD94, o + 224);
    XHash(ctx, c, 0x7973EFA5, o + 228);
    XBoolVia(ctx, c, 0x2243757F, o + 232, tmp);
    for (u32 n = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {n}))) {
        if ((load<u8>(n + 24) & 0x80) == 0) continue;
        const u32 h = load<u32>(n + 20);
        if (h == 0xC3B709C0u) call(ctx, qfn::QueryDef_ReadChild, {n, o + 180});
        else if (h == 0xB921AB51u) call(ctx, qfn::QueryDef_ReadChild, {n, o + 200});
    }
}

// Mem_Alloc(240, 0, 16) and SpawnQuery_Construct0CC188(p, name, tmpl, 0,
// identity, kVec4B30, 1, 0, stack 9th = 0, 11th = 0; f1 = *f1def) when it
// succeeded (the 10th argument's stack slot is not written).
u32 QueryDefNew(PPUContext& ctx, u32 name, u32 tmpl, u32 f1def) {
    const u32 p = static_cast<u32>(call(ctx, m7fn::Mem_Alloc, {240, 0, 16}));
    if (p != 0)
        callf(ctx, qfn::SpawnQuery_Construct, {p, name, tmpl, 0, kIdentity, kVec4B30, 1, 0, 0, 0, 0}, {std::bit_cast<f64>(LfsBits(load<u32>(f1def)))});
    return p;
}

}  // namespace

// u32 XmlDef_ReadBase_000CBE10(a0, XmlCursor* c, Def* o): the base fields
// and the children; returns 0 (the last NextSiblingAny result).
DECOMP(000CBE10, XmlDef_ReadBase_000CBE10) {
    QueryDefReadBody(ctx, arg<u32>(ctx, 1), arg<u32>(ctx, 2));
    ret(ctx, 0);
}

// XmlDef_ReadVecs_000CBF88(a0, c, o): the base part, then the vec3 fields
// +240 and +256; +256..+264 are scaled by the f32 at 0x000CBF84. Returns
// the second ReadVec3 result (f1 = the scaled +264).
DECOMP(000CBF88, XmlDef_ReadVecs_000CBF88) {
    const u32 c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    QueryDefReadBody(ctx, c, o);
    XVec3(ctx, c, 0xADDC2CDD, o + 240);  // r7 = the hash is left live here (ReadVec3 reads r3-r6 only)
    const u32 r = XVec3(ctx, c, 0x0948D097, o + 256);
    mid6::ScaleVec3(ctx, o + 256, 0x000CBF84);
    ret(ctx, r);
}

// Def* XmlDef_New_000CC510(a0, c, name, tmpl): a 240-byte def with vtable
// 0x00B12880, +236 = 0, +4 |= 4 (read before), then read.
DECOMP(000CC510, XmlDef_New_000CC510) {
    const u32 c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    const u32 p = QueryDefNew(ctx, name, tmpl, 0x000CC50C);
    if (p != 0) {
        const u32 f4 = load<u32>(p + 4);
        store<u32>(p + 236, 0);
        store<u32>(p, 0x00B12880);
        store<u32>(p + 4, f4 | 4);
    }
    QueryDefReadBody(ctx, c, p);
    ret(ctx, p);
}

// Def* XmlDef_New_000CC728(a0, c, name, tmpl): a plain 240-byte
// SpawnQuery def (the constructor's vtable), then read.
DECOMP(000CC728, XmlDef_New_000CC728) {
    const u32 c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    const u32 p = QueryDefNew(ctx, name, tmpl, 0x000CC720);
    QueryDefReadBody(ctx, c, p);
    ret(ctx, p);
}
