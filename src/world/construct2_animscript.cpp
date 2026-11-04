// The anim-script command constructors World_Construct registers that the
// straight-line generator (construct_animscript*.cpp) left out: they call a
// member constructor, read back a field they or the base wrote, or loop.
// Names in quotes are strings in the EBOOT whose hash matches the key.
#include "construct.h"

using namespace sly4;
using namespace sly4::wcons;

namespace {
constexpr u32 kMember49E860_Construct = 0x0049E860;  // (obj + 12): an embedded member (name?)
constexpr u32 kSetPos_Init = 0x0008DD48;             // (cmd) (name?)
constexpr u32 kCmd9F8D96A1_Init = 0x000A1C28;        // (cmd) (name?)

// A sub-record of AnimCmd F96A5E9C (108 bytes): {0, 0, u8 1, self, ..., u16 +96 = 0, +100 = 0}.
void InitSubRecord(u32 r) {
    store<u32>(r + 12, r);
    store<u32>(r + 0, 0);
    store<u32>(r + 4, 0);
    store<u16>(r + 96, 0);
    store<u32>(r + 100, 0);
    ref<u8>(r + 8) = 1;
}
}  // namespace

// AnimCmd (hash 0xF96A5E9C), registered at 0045DE9C. The original inlines a
// walk of the chain at +12 (collecting into +28 / count +108, tail at +112),
// but +12 was just cleared, so it always stores a 0 tail.
DECOMP(0007020C, AnimCmd_F96A5E9C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F3B0u);
    store<u32>(obj + 24, obj + 12);
    ref<u8>(obj + 20) = 1;
    store<u16>(obj + 108, 0);
    const u32 head = load<u32>(obj + 12);  // always 0 here
    u32 tail = 0;
    for (u32 p = head; p != 0;) {
        tail = p;
        const u32 next = load<u32>(p + 12);
        if (next != 0) {
            const s32 idx = static_cast<s16>(load<u16>(obj + 108));
            store<u16>(obj + 108, static_cast<u16>(idx + 1));
            store<u32>(obj + 28 + static_cast<u32>(idx) * 4, p);
        }
        p = next;
    }
    store<u32>(obj + 112, tail);
    ref<u8>(obj + 20) = 1;
    for (u32 k = 0; k < 2; ++k) {
        InitSubRecord(obj + 116 + k * 216);
        InitSubRecord(obj + 224 + k * 216);
    }
    for (u32 off = 548; off <= 564; off += 4) store<u32>(obj + off, 0);
    ref<u8>(obj + 568) = 255;
    ref<u8>(obj + 569) = 255;
    ref<u8>(obj + 570) = 0;
    ref<u8>(obj + 571) = 1;
    ref<u8>(obj + 572) = 0;
    ref<u8>(obj + 573) = 0;
    ctx.gpr[3] = 1;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 255;
    ctx.gpr[6] = obj + 332;
    ctx.gpr[7] = obj + 440;
}

// AnimCmd (hash 0xF77517AF), registered at 0045EA2C.
DECOMP(0007AD08, AnimCmd_F77517AF_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0F928u);
    call(ctx, kMember49E860_Construct, {obj + 12, 0x00B0F928u});
    ref<u8>(obj + 88) = 0;
    ctx.gpr[3] = 0;
}

// AnimCmd "GamePlayAnim" (hash 0xFF86B537), registered at 0045F35C. The
// flags word +84 keeps its other bits: bit 31 set, bit 30 cleared.
DECOMP(0008202C, AnimCmd_GamePlayAnim_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 old = load<u32>(obj + 84);
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FE50u);
    for (u32 off : {16u, 20u, 24u, 28u, 36u}) store<u32>(obj + off, 0);
    ref<u8>(obj + 40) = 0;
    store<u32>(obj + 44, 0);
    store<u32>(obj + 48, 0);
    ref<u8>(obj + 52) = 0;
    store<u32>(obj + 56, 0);
    store<u32>(obj + 60, 0x3F800000u);  // 1.0f
    for (u32 off = 64; off <= 80; off += 4) store<u32>(obj + off, 0);
    const u32 f = (old | 0x80000000u) & ~0x40000000u;
    store<u32>(obj + 84, f);
    ctx.gpr[3] = f;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = old;
    ctx.gpr[6] = 0x3F800000u;
    ctx.gpr[7] = 0x80000000u;
}

// AnimCmd (hash 0xBF6B7FBB), registered at 0045FD74.
DECOMP(00088EE0, AnimCmd_BF6B7FBB_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B10350u);
    call(ctx, kMember49E860_Construct, {obj + 12, 0x00B10350u});
    ref<u8>(obj + 88) = 0;
    ctx.gpr[3] = 0;
}

// AnimCmd (hash 0x06F47B91), registered at 0045FFE8. +24 is cleared as one doubleword.
DECOMP(0008A30C, AnimCmd_06F47B91_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B10490u);
    store<u64>(obj + 24, 0);
    ref<u8>(obj + 32) = 0;
    ctx.gpr[3] = 0x00B10490u;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0;
}

// AnimCmd "SetPos" (hash 0x9195F6B2), registered at 004606F4.
DECOMP(0008DE44, AnimCmd_SetPos_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B10828u);
    call(ctx, kSetPos_Init, {obj, 0x00B10828u});
}

// AnimCmd "SwitchStatementEntity" (hash 0xE384A03E), registered at 00461104:
// four PtrVectors at +12 (28 bytes apart, +6 = 4, +20 / +24 = 0) and one at +136.
DECOMP(00098044, AnimCmd_SwitchStatementEntity_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B10D50u);
    for (u32 k = 0; k < 4; ++k) {
        const u32 v = obj + 12 + k * 28;
        call(ctx, kfn::PtrVector_Construct, {v});
        store<u16>(v + 6, 4);
        store<u32>(v + 20, 0);
        store<u32>(v + 24, 0);
    }
    call(ctx, kfn::PtrVector_Construct, {obj + 136});
    store<u16>(obj + 142, 4);
    store<u32>(obj + 128, 0);
    store<u32>(obj + 132, 0);
    store<u32>(obj + 124, 0);
}

// AnimCmd (hash 0x93C95D39), registered at 00461E88: +12 gets bit 31 set.
DECOMP(000A0FAC, AnimCmd_93C95D39_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 f = load<u32>(obj + 12) | 0x80000000u;
    store<u32>(obj, 0x00B11530u);
    store<u32>(obj + 12, f);
    ctx.gpr[3] = 0x00B11530u;
    ctx.gpr[4] = f;
    ctx.gpr[5] = 0x80000000u;
}

// AnimCmd (hash 0x661993C7), registered at 00461ED4: a PtrVector at +144
// (12-byte elements, 6 reserved), +164..+184 = 0, flags +188 bit 31 set and
// bits 30 / 29 cleared, bytes +12 / +77 = 0.
DECOMP(000A116C, AnimCmd_661993C7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v = obj + 144;
    store<u32>(obj, 0x00B11558u);
    call(ctx, kfn::PtrVector_Construct, {v, 0x00B11558u});
    store<u16>(obj + 150, 12);
    call(ctx, kfn::PtrVector_Init, {v, 6, 0, 1});
    const u32 f = (load<u32>(obj + 188) | 0x80000000u) & ~0x60000000u;
    for (u32 off = 164; off <= 184; off += 4) store<u32>(obj + off, 0);
    store<u32>(obj + 188, f);
    ref<u8>(obj + 12) = 0;
    ref<u8>(obj + 77) = 0;
    ctx.gpr[3] = 0;
    ctx.gpr[4] = f;
    ctx.gpr[5] = 0x80000000u;
}

// AnimCmd (hash 0x9F8D96A1), registered at 00461F70.
DECOMP(000A1CEC, AnimCmd_9F8D96A1_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B115A8u);
    call(ctx, kCmd9F8D96A1_Init, {obj, 0x00B115A8u});
}
