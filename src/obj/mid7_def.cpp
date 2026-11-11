// Game code, medium sweep part 7: more def factories of the 0x000CBE10
// family (as part 6's mid6_def.cpp). Each makes the def with SmallAlloc,
// initializes it through Wrapper_0CC4D4 (the spawn-query init) with its
// name and template, sets its vtable and defaults (taken from the template
// when there is one), then reads it with its own XmlDef_Read_* (a0, c, p).
// Quirk: when the allocation fails the read still runs, on p = 0. The FPR
// writes at the end mirror the f1 the original leaves before the read.
#include "mid7.h"

using namespace sly4;
using namespace sly4::mid7;

namespace {

namespace dfn {
constexpr u32 SpawnQuery_Init = 0x000CC4D4;      // (q, name, tmpl, a3, m, pos, u8, u8, stack 9th / 11th; f1) (Wrapper_0CC4D4)
constexpr u32 XmlDef_Read_001BF2A0 = 0x001BF2A0;  // (a0, c, def)
constexpr u32 XmlDef_Read_001CB740 = 0x001CB740;  // (a0, c, def)
constexpr u32 XmlDef_Read_001C6838 = 0x001C6838;  // (a0, c, def) (name?)
constexpr u32 XmlDef_Read_001D7470 = 0x001D7470;  // (a0, c, def) (name?)
}  // namespace dfn

constexpr u32 kIdentity = 0x01164BD0;  // f32[4][4]
constexpr u32 kVec4B30 = 0x01164B30;   // f32[4] passed as the init's position
constexpr u32 kZeroVec = 0x01164B40;   // f32[3] 0
constexpr u32 kVec4C90 = 0x01164C90;   // f32[4]
constexpr u32 kNegMax = 0xFF7FFFFFu;   // -FLT_MAX

// Wrapper_0CC4D4(p, name, tmpl, 0, identity, kVec4B30, 1, 0, 0, -, 0) with
// f1 = *f1def (the 10th argument's stack slot is not written).
void DefInit(PPUContext& ctx, u32 p, u32 name, u32 tmpl, u32 f1def) {
    callf(ctx, dfn::SpawnQuery_Init, {p, name, tmpl, 0, kIdentity, kVec4B30, 1, 0, 0, 0, 0}, {std::bit_cast<f64>(LfsBits(load<u32>(f1def)))});
}
u32 Small(PPUContext& ctx, u32 size) { return static_cast<u32>(call(ctx, m7fn::SmallAlloc, {size})); }
// The template's word at off, or the word at def without one.
u32 T(u32 tmpl, u32 off, u32 def) { return tmpl != 0 ? load<u32>(tmpl + off) : load<u32>(def); }
// The template's byte at off as a bool, or dflt without one.
u8 TB(u32 tmpl, u32 off, u32 dflt) { return (tmpl != 0 ? load<u8>(tmpl + off) : dflt) != 0 ? 1 : 0; }
// f1 = the f32 word w (as lfs leaves it).
void SetF1(PPUContext& ctx, u32 w) { ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w)); }
// +8 |= 0x80000 (read before), +268 / +236 = 0, +4 |= 4, +240..+264 = -FLT_MAX.
void NegMaxDefaults(u32 p) {
    const u32 f8 = load<u32>(p + 8), f4 = load<u32>(p + 4);
    store<u32>(p + 8, f8 | 0x80000);
    store<u32>(p + 268, 0);
    store<u32>(p + 4, f4 | 4);
    store<u32>(p + 236, 0);
    for (u32 k = 0; k < 7; ++k) store<u32>(p + 240 + 4 * k, kNegMax);
}

}  // namespace

// Def* XmlDef_New_001BF4E0(a0, c, name, tmpl): 288 bytes, vtable
// 0x00B16660; +272 / +276 and bools +280 / +281 from the template,
// +8 |= 0x100000.
DECOMP(001BF4E0, XmlDef_New_001BF4E0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001BF4D8;  // f32 {init f1, default}
    const u32 p = Small(ctx, 288);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K);
        NegMaxDefaults(p);
        const u32 f8 = load<u32>(p + 8);
        store<u32>(p, 0x00B16660);
        store<u32>(p + 272, tmpl != 0 ? load<u32>(tmpl + 400) : 0);
        const u32 f = T(tmpl, 404, K + 4);
        store<u32>(p + 276, f);
        store<u8>(p + 280, TB(tmpl, 408, 0));
        const u8 b = TB(tmpl, 409, 0);
        store<u32>(p + 8, f8 | 0x100000);
        store<u8>(p + 281, b);
        SetF1(ctx, f);
    }
    call(ctx, dfn::XmlDef_Read_001BF2A0, {a0, c, p});
    ret(ctx, p);
}

// Def* XmlDef_New_001CBA90(a0, c, name, tmpl): 320 bytes, vtable
// 0x00B17258; +272..+304 and bools +308 / +309 from the template (seven
// f32 defaults, -1, true, true), +8 |= 0x2000000.
DECOMP(001CBA90, XmlDef_New_001CBA90) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001CBA84;  // f32 {init f1, default 276..296, default 300}
    const u32 p = Small(ctx, 320);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K);
        NegMaxDefaults(p);
        const u32 f8 = load<u32>(p + 8);
        store<u32>(p, 0x00B17258);
        store<u32>(p + 272, tmpl != 0 ? load<u32>(tmpl + 436) : 0);
        for (u32 k = 0; k < 6; ++k) store<u32>(p + 276 + 4 * k, T(tmpl, 440 + 4 * k, K + 4));
        const u32 f = T(tmpl, 464, K + 8);
        store<u32>(p + 300, f);
        store<u32>(p + 304, tmpl != 0 ? load<u32>(tmpl + 480) : 0xFFFFFFFFu);
        store<u8>(p + 308, TB(tmpl, 484, 1));
        const u8 b = TB(tmpl, 485, 1);
        store<u32>(p + 8, f8 | 0x2000000);
        store<u8>(p + 309, b);
        SetF1(ctx, f);
    }
    call(ctx, dfn::XmlDef_Read_001CB740, {a0, c, p});
    ret(ctx, p);
}

// Def* XmlDef_New_001C6D30(a0, c, name, tmpl): 784 bytes, vtable
// 0x00B16D98; +272..+284 from the template, six 80-byte records at +288
// ({+0 0, +16 / +32 zero vec3, +48 kVec4C90, +64 0, +68 1, +69 0}), bit 30
// of +768 = bit 0x400000 of the template's +444 (bit 31 cleared),
// +8 |= 0x400000.
DECOMP(001C6D30, XmlDef_New_001C6D30) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001C6D28;  // f32 {init f1, default}
    const u32 p = Small(ctx, 784);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K);
        NegMaxDefaults(p);
        store<u32>(p, 0x00B16D98);
        store<u32>(p + 272, tmpl != 0 ? load<u32>(tmpl + 428) : 0);
        store<u32>(p + 276, tmpl != 0 ? load<u32>(tmpl + 432) : 0);
        store<u32>(p + 280, T(tmpl, 436, K + 4));
        store<u32>(p + 284, T(tmpl, 440, K + 4));
        for (u32 i = 0; i < 6; ++i) {
            const u32 r = p + 288 + 80 * i;
            store<u32>(r, 0);
            for (u32 k = 0; k < 3; ++k) store<u32>(r + 16 + 4 * k, load<u32>(kZeroVec + 4 * k));
            for (u32 k = 0; k < 3; ++k) store<u32>(r + 32 + 4 * k, load<u32>(kZeroVec + 4 * k));
            for (u32 k = 0; k < 3; ++k) store<u32>(r + 48 + 4 * k, load<u32>(kVec4C90 + 4 * k));
            store<u32>(r + 64, 0);
            store<u32>(r + 60, load<u32>(kVec4C90 + 12));
            store<u8>(r + 68, 1);
            store<u8>(r + 69, 0);
        }
        const u32 w = load<u32>(p + 768) & 0x7FFFFFFFu;
        u32 b = 0;
        if (tmpl != 0) {
            store<u32>(p + 768, w);
            b = (load<u32>(tmpl + 444) >> 22) & 1;
        }
        const u32 f8 = load<u32>(p + 8);
        store<u32>(p + 768, (w & ~0x40000000u) | (b << 30));
        store<u32>(p + 8, f8 | 0x400000);
        SetF1(ctx, load<u32>(kVec4C90 + 12));
    }
    call(ctx, dfn::XmlDef_Read_001C6838, {a0, c, p});
    ret(ctx, p);
}

// Def* XmlDef_New_001D7D98(a0, c, name, tmpl): 368 bytes, vtable
// 0x00B17978, +236 = 0, +4 |= 4; vec3s +240 / +256, f32 +272..+284 and
// the 17 words +288 (default {0, 0, 0, 1, 0...}) from the template, bits
// 30-31 of +356 = bits 27-28 of the template's +492, +12 |= 1.
DECOMP(001D7D98, XmlDef_New_001D7D98) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001D7D88;  // f32 {init f1, default 272, default 276..284}
    const u32 p = Small(ctx, 368);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K);
        const u32 f4 = load<u32>(p + 4);
        store<u32>(p + 236, 0);
        store<u32>(p + 4, f4 | 4);
        store<u32>(p, 0x00B17978);
        u32 src = tmpl != 0 ? tmpl + 368 : kZeroVec;
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 240 + 4 * k, load<u32>(src + 4 * k));
        src = tmpl != 0 ? tmpl + 384 : kZeroVec;
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 256 + 4 * k, load<u32>(src + 4 * k));
        store<u32>(p + 272, T(tmpl, 400, K + 4));
        store<u32>(p + 276, T(tmpl, 404, K + 8));
        store<u32>(p + 280, T(tmpl, 408, K + 8));
        const u32 f = T(tmpl, 412, K + 8);
        store<u32>(p + 284, f);
        for (u32 k = 0; k < 17; ++k) store<u32>(p + 288 + 4 * k, tmpl != 0 ? load<u32>(tmpl + 420 + 4 * k) : (k == 3 ? 1u : 0u));
        const u32 bits = tmpl != 0 ? (load<u32>(tmpl + 492) >> 27) & 3 : 0;
        const u32 f12 = load<u32>(p + 12);
        store<u32>(p + 356, (load<u32>(p + 356) & 0x3FFFFFFFu) | (bits << 30));
        store<u32>(p + 12, f12 | 1);
        SetF1(ctx, f);
    }
    call(ctx, dfn::XmlDef_Read_001D7470, {a0, c, p});
    ret(ctx, p);
}
