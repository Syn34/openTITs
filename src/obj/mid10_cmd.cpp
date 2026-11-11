// Game code, medium sweep part 10: command object constructors.
#include "mid10.h"

using namespace sly4;
using namespace sly4::mid10;

namespace {

constexpr u32 Ctor_Base42C9D0 = 0x0042C9D0;  // (o, u8 on): the command base constructor
constexpr u32 String_Copy = 0x00553684;      // (dst, src, size): strncpy(dst, src, size - 1) + terminate

}  // namespace

// u32 VtB210D0_Ctor_00433B2C(Cmd* o, const char names[][128], s32 n, const
//     f32 a[2], const f32 b[2], u32 w, u8 on, bool f20, stack: bools s116,
//     s124, s132, s140, const u8 colA[4] s148, const u8 colB[4] s156, bool
//     f01 s164)  (called by 000A2098)
// Ctor_Base42C9D0(o, on), vtable 0x00B210D0, +524 n, +528 a, +536 b, +544
// w, the bytes +548 / +552 = colA / colB, the flags byte +556 (0x80 / 0x40
// cleared, 0x20 f20, 0x10 / 0x08 / 0x04 / 0x02 s116..s140, 0x01 f01), then
// the n names copied (127 chars) to o+9 + 128 k. Leaves r3 = the flags byte
// (or the count when names were copied) and f1 = b[1].
DECOMP(00433B2C, VtB210D0_Ctor_00433B2C) {
    const u32 o = arg<u32>(ctx, 0), names = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3), b = arg<u32>(ctx, 4);
    const u32 w = arg<u32>(ctx, 5), f20 = arg<u32>(ctx, 7);
    const u32 s116 = SArg(ctx, 116), s124 = SArg(ctx, 124), s132 = SArg(ctx, 132), s140 = SArg(ctx, 140);
    const u32 s164 = SArg(ctx, 164), colA = SArg(ctx, 148), colB = SArg(ctx, 156);
    call(ctx, Ctor_Base42C9D0, {o, arg<u32>(ctx, 6) & 0xFFu});
    CopyW(o + 528, a);
    store<u32>(o + 524, n);
    CopyW(o + 532, a + 4);
    store<u32>(o, 0x00B210D0u);
    CopyW(o + 536, b);
    store<u32>(o + 544, w);
    CopyW(o + 540, b + 4);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 548 + k, load<u8>(colA + k));
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 552 + k, load<u8>(colB + k));
    u32 fl = load<u8>(o + 556) & ~0xC0u;
    fl = (fl & ~0x20u) | ((f20 & 1u) << 5);
    fl = (fl & ~0x10u) | ((s116 & 1u) << 4);
    fl = (fl & ~0x08u) | ((s124 & 1u) << 3);
    fl = (fl & ~0x04u) | ((s132 & 1u) << 2);
    fl = (fl & ~0x02u) | ((s140 & 1u) << 1);
    fl = (fl & 0xFEu) | (s164 & 1u);
    store<u8>(o + 556, static_cast<u8>(fl));
    ctx.fpr[1] = LoadF(b + 4);
    if (load<s32>(o + 524) <= 0) {
        ret(ctx, fl);
        return;
    }
    s32 k = 0;
    do {
        call(ctx, String_Copy, {o + 9 + 128 * static_cast<u32>(k), names + 128 * static_cast<u32>(k), 127});
        ++k;
    } while (k < load<s32>(o + 524));
    ret(ctx, load<u32>(o + 524));
}

namespace {

constexpr u32 SubMachine_Construct = 0x0047C5D8;  // (o, a, b, c, d): the base constructor (reads r3..r7)
constexpr u32 kCurB204E8 = 0x00F65844;      // the instance last constructed

// The store program of VtB204E8_Ctor_003C6CC0 in the original order:
// {offset, kind, value}: kind 4 / 1 = a word / byte constant, 'V' = the
// word value of the zero vec3 0x01164B40 (+value), 'O' = o + value.
struct CtorStore {
    u16 off;
    char kind;
    u32 v;
};
constexpr CtorStore kB204E8Stores[] = {
    {68, 4, 0}, {72, 4, 0}, {120, 4, 0x3F07AE14u}, {124, 4, 0x3F800000u}, {208, 4, 0}, {212, 4, 0}, {216, 4, 0x3F000000u},
    {264, 4, 0x3F07AE14u}, {268, 4, 0x3F800000u}, {352, 4, 0}, {356, 4, 0}, {360, 4, 0x3F000000u}, {416, 'V', 0}, {56, 4, 0},
    {420, 'V', 4}, {0, 4, 0x00B204E8u}, {60, 4, 0}, {64, 4, 0}, {424, 'V', 8}, {112, 4, 0}, {76, 4, 0}, {116, 4, 0},
    {432, 'V', 0}, {176, 4, 0}, {180, 4, 0}, {184, 4, 0}, {436, 'V', 4}, {188, 4, 0}, {192, 4, 0}, {204, 1, 0}, {205, 1, 0},
    {440, 'V', 8}, {220, 4, 0}, {228, 1, 0}, {232, 4, 0}, {236, 1, 0}, {256, 4, 0}, {240, 4, 0}, {448, 'V', 0}, {260, 4, 0},
    {320, 4, 0}, {324, 4, 0}, {452, 'V', 4}, {328, 4, 0}, {332, 4, 0}, {336, 4, 0}, {348, 1, 0}, {349, 1, 0}, {456, 'V', 8},
    {364, 4, 0}, {372, 1, 0}, {376, 4, 0}, {380, 1, 0}, {384, 4, 0}, {464, 'V', 0}, {400, 4, 0}, {468, 'V', 4}, {472, 'V', 8},
    {480, 'V', 0}, {484, 'V', 4}, {496, 4, 0}, {488, 'V', 8}, {500, 4, 0x3F800000u}, {504, 4, 0x3F800000u}, {508, 4, 0},
    {512, 4, 0}, {516, 4, 0}, {520, 4, 0}, {524, 4, 0}, {528, 4, 0}, {576, 4, 0}, {580, 1, 0}, {581, 1, 0}, {582, 1, 0},
    {112, 'O', 0}, {256, 'O', 0}, {80, 4, 0}, {84, 4, 0}, {104, 'O', 112}, {108, 'O', 256},
};

}  // namespace

// u32 VtB204E8_Ctor_003C6CC0(T* o, a, b, c, d)  (called by 003C6EA8)
// SubMachine_Construct(o, a, b, c, d) (r4..r7 passed through), vtable
// 0x00B204E8, the fields as kB204E8Stores lists them (three blend slots at
// +112 / +256 / +400 with 0.53 / 1.0 / 0.5 weights, six zero vec3s at
// +416..+488, two self-linked lists +104 / +108), then the global
// 0x00F65844 = o. Returns 0 (r3 as the stores leave it), f1 = the vec3's z.
DECOMP(003C6CC0, VtB204E8_Ctor_003C6CC0) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, SubMachine_Construct, {o, arg<u32>(ctx, 1), arg<u32>(ctx, 2), arg<u32>(ctx, 3), arg<u32>(ctx, 4)});
    constexpr u32 kVec = 0x01164B40;
    for (const CtorStore& s : kB204E8Stores) {
        const u32 ea = o + s.off;
        if (s.kind == 4) store<u32>(ea, s.v);
        else if (s.kind == 1) store<u8>(ea, static_cast<u8>(s.v));
        else if (s.kind == 'V') CopyW(ea, kVec + s.v);
        else store<u32>(ea, o + s.v);
    }
    store<u32>(kCurB204E8, o);
    ctx.fpr[1] = LoadF(kVec + 8);
    ret(ctx, 0u);
}
