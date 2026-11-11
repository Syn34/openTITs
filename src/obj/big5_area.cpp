// Game code, large-function sweep part 5: an area's animation settings
// pushed onto the player's animator layers (00218DD8).
#include "big5.h"

using namespace sly4;
using namespace sly4::big5;

namespace {
constexpr u32 kWorldPtr = 0x00F65CA0u;   // u32: the world
constexpr u32 kWorldPlayer = 0x001281E0u;  // World: the player
constexpr u32 kAreaDef = 216;            // Area: its def
constexpr u32 kDefKind = 176;            // def: the settings kind (hash)
constexpr u32 kPlLayerTop = 345;         // player: u8 index of the top layer
constexpr u32 kPlLayers = 364;           // player: Animator* [], indexed by +345
constexpr u32 kEntFlags192 = 192;        // entity: 0x8 / +200: 0x400 = the kinds it accepts
constexpr u32 kEntFlags200 = 200;
constexpr u32 kObjLayerBelow = 0x000FCBD0u;  // (player, layer) -> the layer below or 0
constexpr u32 kLayerApply = 0x000FEF54u;     // (layer, settings, 0) (name?)
constexpr u32 kFltMax = 0x7F7FFFFFu;         // 0x00218DCC
constexpr u32 kUnset = 0x60000000u;          // 0x00218DD0: the "not set" value of a settings field
// The settings kinds (def +176) and the layer types (vfn[8]) they apply to.
constexpr u32 kKindA = 0x86DF6221u;  // layer type 11, settings def +180 (applied by kLayerApply)
constexpr u32 kKindB = 0x04C5F92Bu;  // layer type 15, def +192
constexpr u32 kKindC = 0x3F2F1B0Cu;  // layer type 7, def +188
constexpr u32 kKindD = 0x7BF02FB1u;  // layer type 1, def +184

f32 F(u32 a) { return load<f32>(a); }

// The last layer, walking down from the top one, whose vfn[8] type is
// `type`; r3 gets what the original's walk leaves there (0, or the top
// slot's address when the top slot is empty).
u32 FindLayer(PPUContext& ctx, u32 pl, s32 type, u32& r3) {
    const u32 n = load<u8>(pl + kPlLayerTop);
    u32 cur = 0, found = 0;
    r3 = 0;
    if (n != 0) {
        r3 = pl + 4 * n + kPlLayers;
        cur = load<u32>(r3);
    }
    while (cur != 0) {
        if (static_cast<s32>(VCall(ctx, cur, 8, {cur})) == type) found = cur;
        cur = static_cast<u32>(call(ctx, kObjLayerBelow, {pl, cur}));
        r3 = cur;
    }
    return found;
}
}  // namespace

// void Area_ApplyLayerSettings(Area* o, Entity* e)
// For an entity that accepts the area's kind of settings (flag 0x8 of
// e+192 for kinds A / B / D, 0x400 of e+200 for C; A takes either), finds
// the player's matching animator layer and applies the def's settings
// block: A through 0x000FEF54(layer, block, 0); B copies the fields that
// are set (+0 only when >= FLT_MAX, the +16 and +32 vectors unless they
// hold the "unset" value 0x60000000); C copies 26 words (dst = src + 16);
// D copies 36 words to +360.., the word at +84 to +444, sets +12 to
// 0xFFFFFFFF, ORs 0x1F into +16 and takes bit 29 of +208 from bit 0 of
// block +148. r3 / f1 at exit are what the original leaves there.
DECOMP(00218DD8, Area_ApplyLayerSettings) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 pl = load<u32>(load<u32>(kWorldPtr) + kWorldPlayer);
    const u32 def = load<u32>(o + kAreaDef);
    if (pl == 0) return ret(ctx, o);
    const u32 kind = load<u32>(def + kDefKind);
    u32 r3 = 0;
    const auto accepts8 = [e]() { return (load<u32>(e + kEntFlags192) & 0x8u) != 0; };
    const auto accepts400 = [e]() { return (load<u32>(e + kEntFlags200) & 0x400u) != 0; };
    if (kind == kKindA) {
        if (e == 0) return ret(ctx, kind);
        if (!accepts8() && !accepts400()) return ret(ctx, 0u);
        const u32 blk = load<u32>(def + 180);
        const u32 layer = FindLayer(ctx, pl, 11, r3);
        if (layer == 0 || blk == 0) return ret(ctx, r3);
        call(ctx, kLayerApply, {layer, blk, 0});
        return;
    }
    if (kind == kKindB) {
        if (e == 0) return ret(ctx, kind);
        if (!accepts8()) return ret(ctx, 0u);
        const u32 blk = load<u32>(def + 192);
        const u32 layer = FindLayer(ctx, pl, 15, r3);
        if (layer == 0 || blk == 0) return ret(ctx, r3);
        u32 last = load<u32>(blk);
        if (F(blk) >= std::bit_cast<f32>(kFltMax)) CopyW(layer + 128, blk);
        last = load<u32>(blk + 16);
        if (!(F(blk + 16) == std::bit_cast<f32>(kUnset))) {
            CopyW(layer + 80, blk + 16);
            CopyW(layer + 84, blk + 20);
            last = load<u32>(blk + 24);
            CopyW(layer + 88, blk + 24);
        }
        last = load<u32>(blk + 32);
        if (!(F(blk + 32) == std::bit_cast<f32>(kUnset))) {
            CopyW(layer + 112, blk + 32);
            last = load<u32>(blk + 36);
            CopyW(layer + 116, blk + 36);
        }
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(last));
        return ret(ctx, 0x00218DCCu);  // (the constants' address)
    }
    if (kind == kKindC) {
        if (e == 0) return ret(ctx, kind);
        if (!accepts400()) return ret(ctx, 0u);
        const u32 blk = load<u32>(def + 188);
        const u32 layer = FindLayer(ctx, pl, 7, r3);
        if (layer == 0 || blk == 0) return ret(ctx, r3);
        CopyW(layer + 16, blk);
        CopyWords(layer + 32, blk + 16, 3);
        CopyWords(layer + 48, blk + 32, 9);
        CopyWords(layer + 96, blk + 80, 3);
        CopyWords(layer + 112, blk + 96, 10);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(blk + 132)));
        return ret(ctx, r3);
    }
    if (kind == kKindD) {
        if (e == 0) return ret(ctx, kind);
        if (!accepts8()) return ret(ctx, 0u);
        const u32 blk = load<u32>(def + 184);
        const u32 layer = FindLayer(ctx, pl, 1, r3);
        if (layer == 0 || blk == 0) return ret(ctx, r3);
        CopyWords(layer + 360, blk, 20);
        const u32 w12 = load<u32>(layer + 12);
        CopyW(layer + 440, blk + 80);
        CopyWords(layer + 448, blk + 88, 6);  // +448..+468 <- +88..+108
        const u32 b84 = load<u32>(blk + 84);
        CopyW(layer + 472, blk + 112);
        store<u32>(layer + 444, b84);
        CopyWords(layer + 476, blk + 116, 3);  // +476..+484
        const u32 w16 = load<u32>(layer + 16);
        store<u32>(layer + 12, w12 | 0xFFFFFFFFu);
        CopyWords(layer + 488, blk + 128, 2);  // +488, +492
        store<u32>(layer + 16, w16 | 0x1Fu);
        CopyWords(layer + 496, blk + 136, 2);  // +496, +500
        const u32 b148 = load<u32>(blk + 148);
        const u32 w208 = load<u32>(layer + 208);
        CopyW(layer + 504, blk + 144);
        store<u32>(layer + 208, (w208 & ~0x20000000u) | ((b148 & 1u) << 29));
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(blk + 144)));
        return ret(ctx, b148);
    }
    ret(ctx, kind);
}

namespace {
constexpr u32 kAreaDefBaseCtor = 0x000C2B90u;  // (o, src): the base part (template instance)
constexpr u32 kStringCopyN = 0x00553638u;      // (dst, src, n)
constexpr u32 kLayerSettingsCtor = 0x000FD7ACu;  // (block): kind A's 128-byte block (name?)
constexpr u32 kVtAreaDefBase = 0x00B19510u;
constexpr u32 kVtAreaDef = 0x00B19FB0u;
// The defaults of kind D's / kind C's blocks: the +72 value of 76-byte
// property descriptors.
constexpr u32 kDefaultsD = 0x010334D0u;  // 38 words, 76 bytes apart
struct Pair {
    u16 src, dst;
};
constexpr u32 kDefaultsC = 0x01034574u;  // + src
constexpr Pair kDefaultsCMap[] = {
    {0x000, 0},   {0x05C, 16},  {0x060, 20},  {0x064, 24},  {0x0B4, 32},  {0x100, 36},  {0x14C, 40},
    {0x198, 44},  {0x1E4, 48},  {0x230, 52},  {0x27C, 56},  {0x2C8, 60},  {0x314, 64},  {0x36C, 80},
    {0x370, 84},  {0x374, 88},  {0x3C4, 96},  {0x410, 100}, {0x45C, 104}, {0x4A8, 108}, {0x4F4, 112},
    {0x540, 116}, {0x58C, 120}, {0x5D8, 124}, {0x624, 128}, {0x670, 132},
};
// Words copied from the source (dst offsets; kind C: src = 384 + dst, kind
// B: src = 528 + dst).
constexpr u8 kBlockCWords[] = {0, 16, 20, 24, 32, 36, 40, 44, 48, 52, 56, 60, 64,
                               80, 84, 88, 96, 100, 104, 108, 112, 116, 120, 124, 128, 132};
constexpr u8 kBlockBWords[] = {0, 16, 20, 24, 32, 36};
}  // namespace

// void AreaDef_Construct(AreaDef* o, const AreaDefSrc* src)
// The base part, then the color bytes +172..175 from src +80, the name
// (31 chars) at +140 from src->76, the vtable, the settings kind at +176
// (src +84) and +180..192 cleared; then the kind's settings block (see
// Area_ApplyLayerSettings): A: 128 bytes at +180 (0x000FD7AC, then src
// +96..+220 as below); D: 152 bytes at +184, the descriptor defaults
// then 19 word pairs from src +224; C: 144 bytes at +188, defaults then
// src +384..; B: 48 bytes at +192 (-1 and FLT_MAX defaults) then src
// +528... A failed allocation still gets the source words written at
// address 0 (as in the original). r3 / f1 are what the original leaves.
DECOMP(00216AB0, AreaDef_Construct) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    call(ctx, kAreaDefBaseCtor, {o, src});
    store<u8>(o + 172, load<u8>(src + 80));
    store<u8>(o + 173, load<u8>(src + 81));
    store<u32>(o, kVtAreaDefBase);
    store<u8>(o + 174, load<u8>(src + 82));
    store<u8>(o + 175, load<u8>(src + 83));
    call(ctx, kStringCopyN, {o + 140, load<u32>(src + 76), 31});
    store<u32>(o, kVtAreaDef);
    const u32 kind = load<u32>(src + 84);
    store<u32>(o + 180, 0);
    store<u32>(o + kDefKind, kind);
    store<u32>(o + 184, 0);
    store<u32>(o + 188, 0);
    store<u32>(o + 192, 0);
    if (kind == kKindD) {
        u32 blk = static_cast<u32>(call(ctx, b5fn::kMemAlloc, {152, 0, 16}));
        if (blk != 0) {
            for (u32 k = 0; k < 38; ++k) CopyW(blk + 4 * k, kDefaultsD + 76 * k);
            ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kDefaultsD + 76 * 36)));
        }
        store<u32>(o + 184, blk);
        for (u32 k = 0; k < 19; ++k) {
            const u32 w0 = load<u32>(src + 224 + 8 * k), w1 = load<u32>(src + 228 + 8 * k);
            store<u32>(blk + 8 * k, w0);
            store<u32>(blk + 8 * k + 4, w1);
        }
        return ret(ctx, blk);
    }
    if (kind == kKindC) {
        const u32 blk = static_cast<u32>(call(ctx, b5fn::kMemAlloc, {144, 0, 16}));
        if (blk != 0)
            for (const Pair& p : kDefaultsCMap) CopyW(blk + p.dst, kDefaultsC + p.src);
        const u32 first = load<u32>(src + 384);
        store<u32>(o + 188, blk);
        store<u32>(blk, first);
        for (u32 k = 1; k < sizeof(kBlockCWords); ++k) CopyW(blk + kBlockCWords[k], src + 384 + kBlockCWords[k]);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(src + 516)));
        return ret(ctx, blk);
    }
    if (kind == kKindB) {
        const u32 blk = static_cast<u32>(call(ctx, b5fn::kMemAlloc, {48, 0, 16}));
        if (blk != 0) {
            store<u32>(blk, 0xBF800000u);  // -1.0f
            for (u32 off : {16u, 20u, 24u, 32u, 36u}) store<u32>(blk + off, kFltMax);
        }
        const u32 first = load<u32>(src + 528);
        store<u32>(o + 192, blk);
        store<u32>(blk, first);
        for (u32 k = 1; k < sizeof(kBlockBWords); ++k) CopyW(blk + kBlockBWords[k], src + 528 + kBlockBWords[k]);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(src + 564)));
        return ret(ctx, blk);
    }
    if (kind == kKindA) {
        u32 blk = static_cast<u32>(call(ctx, b5fn::kMemAlloc, {128, 0, 16}));
        if (blk != 0) call(ctx, kLayerSettingsCtor, {blk});
        const u32 first = load<u32>(src + 96);
        store<u32>(o + 180, blk);
        store<u32>(blk, first);
        CopyWords(blk + 4, src + 100, 2);     // +4, +8
        CopyWords(blk + 16, src + 112, 3);    // +16..+24
        CopyWords(blk + 32, src + 128, 16);   // +32..+92
        CopyWords(blk + 96, src + 192, 6);    // +96 (an int), +100..+116
        store<u8>(blk + 120, load<u8>(src + 216));
        CopyW(blk + 124, src + 220);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(src + 212)));
        return ret(ctx, blk);
    }
    ret(ctx, kind);
}
