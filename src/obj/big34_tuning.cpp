// Game code, large-function sweep part 34: a settings block filled from its
// global tuning variables.
#include "native/objtypes.h"

namespace sly4::obj {

struct TuningBlock {
    pad<4> _0;
    u8 b4, b5;       // +4, +5
    pad<2> _6;
    be_u32 w[118];   // +8 .. +476
};
static_assert(sizeof(TuningBlock) == 480);

namespace {
// One copied value: the destination offset, the source address (a tuning
// variable's value: +72 for a scalar, +80.. for a vector) and its size.
struct Copy {
    u16 dst;
    u32 src;
    u8 size;
};
// In the original's order (the bytes +4 / +5 and the word +132 come in the middle).
constexpr Copy kCopies[] = {
    {8, 0x01036B64, 4}, {12, 0x01036BB0, 4}, {16, 0x01036BFC, 4}, {20, 0x01036C48, 4},
    {24, 0x01036C94, 4}, {28, 0x01036CE0, 4}, {32, 0x01036D2C, 4}, {36, 0x01036D78, 4},
    {40, 0x01036DC4, 4}, {44, 0x01036E10, 4}, {48, 0x01036E5C, 4}, {52, 0x01036EA8, 4},
    {56, 0x01036EF4, 4}, {60, 0x01036F40, 4}, {64, 0x01036F8C, 4}, {80, 0x01036FE0, 4},
    {84, 0x01036FE4, 4}, {88, 0x01036FE8, 4}, {96, 0x01037038, 4}, {100, 0x01037084, 4},
    {104, 0x010370D0, 4}, {108, 0x0103711C, 4}, {112, 0x01037168, 4}, {116, 0x010371B4, 4},
    {120, 0x01037200, 4}, {124, 0x0103724C, 4}, {128, 0x01037298, 4}, {144, 0x01037340, 4},
    {148, 0x01037344, 4}, {152, 0x01037348, 4}, {160, 0x010373A0, 4}, {164, 0x010373A4, 4},
    {168, 0x010373A8, 4}, {176, 0x01037400, 4}, {180, 0x01037404, 4}, {184, 0x01037408, 4},
    {192, 0x01037458, 4}, {196, 0x010374A4, 4}, {208, 0x01037500, 4}, {212, 0x01037504, 4},
    {216, 0x01037508, 4}, {224, 0x01037558, 4}, {4, 0x01036ACC, 1}, {228, 0x010375F0, 4},
    {232, 0x010375A4, 4}, {5, 0x01036B18, 1}, {236, 0x0103763C, 4}, {240, 0x01037688, 4},
    {244, 0x010376D4, 4}, {248, 0x01037720, 4}, {252, 0x0103776C, 4}, {256, 0x010377B8, 4},
    {260, 0x01037804, 4}, {264, 0x01037850, 4}, {268, 0x0103789C, 4}, {272, 0x010378E8, 4},
    {276, 0x01037934, 4}, {280, 0x01037980, 4}, {284, 0x010379CC, 4}, {132, 0x010372E4, 4},
    {288, 0x01037A18, 4}, {292, 0x01037A64, 4}, {296, 0x01037AB0, 4}, {300, 0x01037AFC, 4},
    {304, 0x01037B48, 4}, {308, 0x01037B94, 4}, {312, 0x01037BE0, 4}, {316, 0x01037C2C, 4},
    {320, 0x01037C78, 4}, {324, 0x01037CC4, 4}, {336, 0x01037D20, 4}, {340, 0x01037D24, 4},
    {344, 0x01037D28, 4}, {352, 0x01037D78, 4}, {356, 0x01037DC4, 4}, {368, 0x01037E20, 4},
    {372, 0x01037E24, 4}, {376, 0x01037E28, 4}, {384, 0x01037E80, 4}, {388, 0x01037E84, 4},
    {392, 0x01037E88, 4}, {400, 0x01037ED8, 4}, {404, 0x01037F24, 4}, {408, 0x01037F70, 4},
    {412, 0x01037FBC, 4},
};
constexpr u32 kArrayVars = 0x01037FC0;  // 14 scalar tuning variables, 76 bytes apart
constexpr u32 kTailA = 0x01038430, kTailB = 0x0103847C;
}  // namespace

// void T::LoadTuning(T* o) (called through a pointer). Copies the global
// tuning values into o as raw words (bytes for +4 / +5): the kCopies list,
// then the 14-entry array +416..+468 from the variables at kArrayVars, then
// +472 / +476.
void Tuning_CopyDefaults_007379FC(TuningBlock* o) {
    for (const Copy& c : kCopies) {
        if (c.size == 1) (c.dst == 4 ? o->b4 : o->b5) = global<u8>(c.src);
        else o->w[(c.dst - 8) / 4] = global<be_u32>(c.src).get();
    }
    for (u32 k = 0; k < 14; ++k) o->w[(416 - 8) / 4 + k] = global<be_u32>(kArrayVars + 76 * k + 72).get();
    o->w[(472 - 8) / 4] = global<be_u32>(kTailA).get();
    o->w[(476 - 8) / 4] = global<be_u32>(kTailB).get();
}

}  // namespace sly4::obj

NATIVE(007379FC, sly4::obj::Tuning_CopyDefaults_007379FC)
