// World_HandleEvent callees, part 26d: the level-entry state machine
// constructor (vtable 0x00B20488).
#include <bit>

#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kSubMachineConstruct = 0x0047C5D8;  // (SubMachine*, a1, 0, u32) (symbols.txt SubMachine_Construct)
constexpr u32 kPtrVectorConstruct = 0x005516E0;   // (PtrVector*) (symbols.txt)
constexpr u32 kPtrVectorInit = 0x00551598;        // (PtrVector*, n, 0, 1) (symbols.txt)
constexpr u32 kMemset = 0x00726E54;
constexpr u32 kVtable3B835C = 0x00B20488;         // (symbols.txt VtB20488_Dtor / _Delete)
// Shared constants: vec3 zero, vec2 at 0x01164AFC, quaternion identity
// (4 words) and a 3x4 identity matrix (rows at +0 / +16 / +32 / +48).
constexpr u32 kZeroVec = 0x01164B40, kVec2 = 0x01164AFC, kQuatIdentity = 0x01164C80, kMatIdentity = 0x01164BD0;
constexpr u32 kOne = 0x3F800000u, kQuarter = 0x3E800000u;

void copy_words(u32 dst, u32 src, u32 n) {
    for (u32 k = 0; k < n; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
}
void zero_words(u32 dst, u32 n) {
    for (u32 k = 0; k < n; ++k) store<u32>(dst + 4 * k, 0);
}
void vec3_zero(u32 dst) { copy_words(dst, kZeroVec, 3); }
void quat_identity(u32 dst) { copy_words(dst, kQuatIdentity, 4); }
void mat34_identity(u32 dst) {
    for (u32 r = 0; r < 4; ++r) copy_words(dst + 16 * r, kMatIdentity + 16 * r, 3);
}
}  // namespace

// ---------------------------------------------------------------- 0x003B835C
// LevelMachine* LevelMachine_Construct3B835C(LevelMachine* m, a1, u32 a2,
// Level* lv) (name?): SubMachine_Construct(m, a1, 0, lv +4), +56 = a2, +60
// = lv, vtable 0x00B20488, PtrVector +104 (+110 = 128, Init(10, 0, 1)).
// Fills the state blocks with zero vectors, identity quaternions and
// matrices (five 48-byte quaternion pairs at +272, matrices at +608 /
// +1232 / +1376), clears +840..+1159, +1560 bits 25..31 cleared and bit 24
// set, +28 |= 2, and links the sub-state pointers +64..+100 (into lv and
// m). Returns lv + 5072.
DECOMP(003B835C, LevelMachine_Construct3B835C) {
    const u32 m = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2), lv = arg<u32>(ctx, 3);
    call(ctx, kSubMachineConstruct, {m, a1, 0, load<u32>(lv + 4)});
    store<u32>(m + 56, a2);
    store<u32>(m + 60, lv);
    store<u32>(m, kVtable3B835C);
    call(ctx, kPtrVectorConstruct, {m + 104});
    store<u16>(m + 110, 128);
    call(ctx, kPtrVectorInit, {m + 104, 10, 0, 1});

    zero_words(m + 128, 3);
    vec3_zero(m + 144);
    vec3_zero(m + 160);
    zero_words(m + 176, 3);
    zero_words(m + 192, 2);
    vec3_zero(m + 208);
    vec3_zero(m + 224);
    store<u32>(m + 240, kOne);
    store<u32>(m + 244, kOne);
    store<u32>(m + 248, 0);
    store<u32>(m + 252, kQuarter);
    copy_words(m + 256, kVec2, 2);
    for (u32 k = 0; k < 5; ++k) {  // 48-byte blocks: two quaternions, then +32..+40 zeroed
        const u32 b = m + 272 + 48 * k;
        quat_identity(b);
        quat_identity(b + 16);
        store<u32>(b + 36, 0);
        store<u32>(b + 32, 0);
        store<u16>(b + 38, 0);
        store<u32>(b + 40, 0);
    }
    zero_words(m + 512, 2);
    zero_words(m + 528, 2);
    vec3_zero(m + 544);
    vec3_zero(m + 560);
    store<u32>(m + 576, 0);
    zero_words(m + 592, 3);
    mat34_identity(m + 608);
    vec3_zero(m + 736);
    zero_words(m + 752, 3);
    vec3_zero(m + 768);
    vec3_zero(m + 784);
    store<u32>(m + 800, 0);
    store<u8>(m + 804, 0);
    zero_words(m + 816, 3);
    store<u8>(m + 828, 0);
    zero_words(m + 832, 2);
    store<u8>(m + 1160, 0);
    vec3_zero(m + 1168);
    vec3_zero(m + 1184);
    zero_words(m + 1200, 5);
    mat34_identity(m + 1232);
    vec3_zero(m + 1296);
    store<u32>(m + 1312, 0);
    quat_identity(m + 1328);
    store<u32>(m + 1344, 0);
    store<u32>(m + 1348, kOne);
    zero_words(m + 1352, 3);
    copy_words(m + 1364, kVec2, 2);
    mat34_identity(m + 1376);
    vec3_zero(m + 1440);
    vec3_zero(m + 1456);
    quat_identity(m + 1472);
    quat_identity(m + 1488);
    store<u32>(m + 1504, 0);
    store<u32>(m + 1512, 0);  // +1508 is left as it was
    store<u32>(m + 1516, 0);
    call(ctx, kMemset, {m + 840, 0, 320});

    for (u32 k = 1520; k <= 1543; ++k)  // byte flags; +1527 and +1539 are left as they were
        if (k != 1527 && k != 1539) store<u8>(m + k, 0);
    zero_words(m + 1544, 4);
    store<u32>(m + 1560, (load<u32>(m + 1560) & 0x01FFFFFFu) | 0x01000000u);
    zero_words(m + 1580, 10);
    store<u32>(m + 28, load<u32>(m + 28) | 2u);
    const u32 l2 = load<u32>(m + 60);
    store<u32>(m + 84, m + 128);
    store<u32>(m + 88, m + 192);
    store<u32>(m + 92, m + 528);
    store<u32>(m + 64, l2 + 2580);
    store<u32>(m + 72, l2 + 4944);
    store<u32>(m + 68, l2 + 2620);
    store<u32>(m + 96, m + 592);
    store<u32>(m + 76, l2 + 5000);
    store<u32>(m + 100, m + 816);
    store<u32>(m + 80, l2 + 5072);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(load<u32>(kQuatIdentity + 12)));  // the last lfs
    ret(ctx, l2 + 5072);
}
