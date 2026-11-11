// Game code, medium sweep part 14: choice entry / choice set assignment.
#include "mid14.h"

using namespace sly4;
using namespace sly4::mid14;

namespace {
constexpr u32 VectorAssign64_22A610 = 0x0022A610;   // (dst vec, src vec, 0): vector of 64-byte records (name?)
constexpr u32 ChoiceEntry_Construct = 0x0022AB00;   // decompiled in mid14_misc.cpp
constexpr u32 ChoiceEntry_Assign = 0x0022C01C;      // below
constexpr u32 Vector_Push352 = 0x0075C8F4;          // decompiled in mid14_misc.cpp
}  // namespace

// ChoiceEntry* ChoiceEntry_Assign22C01C(ChoiceEntry* d, const ChoiceEntry* s)
// (called by ChoiceSet_Assign22C4A8) Nothing when d == s. Otherwise the
// plain fields as Vector_Push352_75C8F4 copies them (+0..+236), then the
// five record vectors +240..+320 through 0x0022A610 (dst, src, 0). The
// original tests d != s a second time before those calls (always true).
// Returns d when d == s, else what the last vector assignment returns.
DECOMP(0022C01C, ChoiceEntry_Assign22C01C) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if (d == s) {
        ret(ctx, d);
        return;
    }
    for (u32 k : {0u, 4u, 8u, 16u, 32u, 20u, 24u, 36u}) Cw(d + k, s + k);
    for (u32 k = 40; k <= 73; ++k) store<u8>(d + k, load<u8>(s + k));
    for (u32 k : {80u, 76u, 84u}) Cw(d + k, s + k);
    for (u32 k = 96; k <= 160; k += 4) Cw(d + k, s + k);
    store<u8>(d + 164, load<u8>(s + 164));
    for (u32 k = 168; k <= 176; k += 4) Cw(d + k, s + k);
    store<u8>(d + 180, load<u8>(s + 180));
    for (u32 k : {192u, 196u, 200u, 208u, 212u, 216u, 224u, 228u, 232u, 236u}) Cw(d + k, s + k);
    SetF1(ctx, s + 236);  // leftover: the last lfs
    u64 r = 0;
    for (u32 k = 240; k <= 320; k += 20) r = call(ctx, VectorAssign64_22A610, {d + k, s + k, 0});
    ret(ctx, r);
}

// u32 ChoiceSet_Assign22C4A8(ChoiceSet* d, const ChoiceSet* s)
// (called by XmlDef_FromRec22D44C, XmlDef_Read22DFD8) Nothing when d == s
// (returns d). Otherwise copies the words +0..+160 (not +12 / +28 / +44 /
// +108 / +124 / +132..+148) and then, for each 32-byte group of s's vector
// +132 (count u16 +140): pushes an empty group onto d's vector (a stack
// group {PtrVector of 352-byte entries, 0, 0, byte 0} memmoved in, 29
// bytes, growing to twice the capacity when full, high-water +142), frees
// the stack group's vector and, unless the new slot is s's group itself,
// copies the group's +20 / +24 / byte +28 and appends a copy of each of its
// entries (a constructed stack entry pushed with Vector_Push352_75C8F4, the
// stack entry's five vectors freed, then ChoiceEntry_Assign22C01C onto the
// pushed copy). Returns s's group count.
DECOMP(0022C4A8, ChoiceSet_Assign22C4A8) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if (d == s) {
        ret(ctx, d);
        return;
    }
    for (u32 k : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u}) Cw(d + k, s + k);
    for (u32 k = 48; k <= 104; k += 4) Cw(d + k, s + k);
    for (u32 k : {112u, 116u, 120u}) Cw(d + k, s + k);
    const u32 w128 = load<u32>(s + 128), w152 = load<u32>(s + 152);
    store<u32>(d + 128, w128);
    const u32 w156 = load<u32>(s + 156);
    store<u32>(d + 152, w152);
    store<u32>(d + 156, w156);
    Cw(d + 160, s + 160);
    SetF1(ctx, s + 160);  // leftover: the last lfs
    Stack frame(ctx, 384);
    const u32 ent = frame.addr();  // the original's sp + 112: a 352-byte entry
    const u32 grp = ent + 352;     // sp + 464: a 32-byte group
    for (u32 i = 0; i < load<u16>(s + 140); ++i) {
        const u32 sg = load<u32>(s + 132) + i * 32;
        call(ctx, m1fn::PtrVector_Construct, {grp});
        store<u32>(grp + 24, 0);
        store<u8>(grp + 28, 0);
        store<u32>(grp + 20, 0);
        store<u16>(grp + 6, 352);
        u32 n = load<u16>(d + 140);
        const u32 cap = load<u16>(d + 136);
        if (n == cap) {
            call(ctx, m1fn::PtrVector_Reserve, {d + 132, (cap << 1) & 0x0001FFFEu});
            n = load<u16>(d + 140);
        }
        call(ctx, mid12::m12fn::Memmove, {load<u32>(d + 132) + (n << 5), grp, 29});
        const u32 c = load<u16>(d + 140) + 1;
        const u32 hw = load<u16>(d + 142);
        store<u16>(d + 140, static_cast<u16>(c));
        store<u16>(d + 142, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFFu) ? hw : c));
        call(ctx, m1fn::PtrVector_Clear, {grp});
        call(ctx, m1fn::Nop, {grp});
        const u32 dg = load<u32>(d + 132) + ((load<u16>(d + 140) - 1u) << 5);
        if (dg == sg) continue;
        Cw(dg + 20, sg + 20);
        Cw(dg + 24, sg + 24);
        store<u8>(dg + 28, load<u8>(sg + 28));
        for (u32 j = 0; j < load<u16>(sg + 8); ++j) {
            const u32 se = load<u32>(sg) + j * 352;
            call(ctx, ChoiceEntry_Construct, {ent});
            call(ctx, Vector_Push352, {dg, ent});
            for (u32 k = 320; k >= 240; k -= 20) {
                call(ctx, m1fn::PtrVector_Clear, {ent + k});
                call(ctx, m1fn::Nop, {ent + k});
            }
            call(ctx, ChoiceEntry_Assign, {load<u32>(dg) + (load<u16>(dg + 8) - 1u) * 352, se});
        }
    }
    ret(ctx, static_cast<u32>(load<u16>(s + 140)));
}
