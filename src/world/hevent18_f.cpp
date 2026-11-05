// World_HandleEvent callees, part 18f: the 192-byte record vector push and
// the animator component reset.
#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
// 192-byte record: +0..+127 bytes, +128..+136 / +144..+156 f32, +160..+172
// u32, +176 u32 flags (bits 0..7 copied, 8..31 kept), +180 u32. +140 and
// +184..+191 are not copied.
constexpr u32 kRecSize = 192, kRecFlags = 176, kRecLast = 180;

// Animator component: +8 Owner* (+164 Animator*, vt +64 rig getter (+48
// u32)), +24 u32 a[8], +56 u32 b[8], +88 u32 c[2], +96 base pose.
constexpr u32 kCompOwner = 8, kOwnerAnimator = 164, kOwnerVtRig = 64, kRigArg = 48;
constexpr u32 kCompA = 24, kCompB = 56, kCompC = 88, kCompBase = 96;
constexpr u32 kAnimBase3525A0 = 0x003525A0;  // (Animator*, 4 vec*; f32 x5) (name?)
constexpr u32 kAnimC352190 = 0x00352190;     // (Animator*, 6 ptrs; f32 x8; stack u32) (name?)
constexpr u32 kAnimA351B60 = 0x00351B60;     // (Animator*, q, u32, 5 ptrs; f32 x3; stack u32) (name?)
constexpr u32 kAnimB351F30 = 0x00351F30;     // (Animator*, q, u32, 5 ptrs) (name?)

void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
}  // namespace

// u32 Vector_Add733964(Vector* v, const Rec* src) (name?): appends a copy
// of the 192-byte record (grows a full vector to twice its capacity) and
// returns its index.
DECOMP(00733964, Vector_Add733964) {
    const u32 v = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    if (load<u16>(v + kPvCount) == load<u16>(v + kPvCap))
        call(ctx, hevent8::h8fn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + kPvCap)) << 1) & 0x1FFFEu});
    const u32 d = load<u32>(v + kPvData) + load<u16>(v + kPvCount) * kRecSize;
    for (u32 k = 0; k < 128; ++k) store<u8>(d + k, load<u8>(src + k));
    for (u32 k = 128; k <= 136; k += 4) copy32(d + k, src + k);  // lfs / stfs pairs
    for (u32 k = 144; k <= 156; k += 4) copy32(d + k, src + k);
    const u32 old = load<u32>(d + kRecFlags);
    for (u32 k = 160; k <= 172; k += 4) copy32(d + k, src + k);
    // The flag bits are inserted one at a time (bit 31 first), each stored.
    u32 f = old & 0x7FFFFFFFu;
    for (u32 b = 0; b < 8; ++b) {
        const u32 m = 0x80000000u >> b;
        f = (f & ~m) | (load<u32>(src + kRecFlags) & m);
        store<u32>(d + kRecFlags, f);
    }
    copy32(d + kRecLast, src + kRecLast);
    const u32 n1 = load<u16>(v + kPvCount) + 1;
    const u32 hi = load<u16>(v + kPvHigh);
    store<u16>(v + kPvCount, static_cast<u16>(n1));
    store<u16>(v + kPvHigh, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF) ? hi : n1));
    ctx.fpr[1] = hevent7::lfs_f64(src + 156);  // f1 left as the copies leave it
    ret(ctx, static_cast<s32>(n1 & 0xFFFF) - 1);
}

// void Obj_Reset293264(Component* c) (name?): re-applies the component's
// poses to its owner's animator: the base pose (+96), the two +88 poses
// (with the rig's +48), then the eight +24 and +56 entries.
DECOMP(00293264, Obj_Reset293264) {
    const u32 c = arg<u32>(ctx, 0);
    u32 owner = load<u32>(c + kCompOwner);
    const u32 an = load<u32>(owner + kOwnerAnimator);
    if (an == 0) return;
    if (const u32 x = load<u32>(c + kCompBase); x != 0) {
        auto f = [&](u32 k) { return hevent7::lfs_f64(x + 128 + 4 * k); };
        callf(ctx, kAnimBase3525A0, {an, x + 36, x + 80, x + 96, x + 112}, {f(0), f(1), f(2), f(3), f(4)});
        owner = load<u32>(c + kCompOwner);
    }
    const u32 rig = static_cast<u32>(vcall(ctx, owner, kOwnerVtRig, {owner}));
    const u32 rarg = rig != 0 ? load<u32>(rig + kRigArg) : 0u;
    for (u32 i = 0; i < 2; ++i) {
        const u32 p = load<u32>(c + kCompC + 4 * i);
        if (p == 0) continue;
        auto f = [&](u32 k) { return hevent7::lfs_f64(p + 160 + 4 * k); };
        // Seven register arguments (r10 = p, unread), eight float slots,
        // then the rig word in parameter slot 15 (r1 +168).
        callf(ctx, kAnimC352190, {an, p, p + 32, p + 64, p + 96, p + 128, p + 144, p, 0, 0, 0, 0, 0, 0, 0, rarg},
              {f(0), f(1), f(2), f(3), f(4), f(5), f(6), f(7)});
    }
    for (u32 j = 0; j < 8; ++j) {
        if (const u32 q = load<u32>(c + kCompA + 4 * j); q != 0) {
            auto f = [&](u32 k) { return hevent7::lfs_f64(q + 192 + 4 * k); };
            const u32 w36 = load<u32>(q + 36);
            ctx.gpr[11] = w36;  // the original loads the stack word into r11
            // Eight register arguments, three float slots, then +36 in slot 11 (r1 +136).
            callf(ctx, kAnimA351B60, {an, q, load<u32>(q + 32), q + 40, q + 72, q + 104, q + 136, q + 176, 0, 0, 0, w36},
                  {f(0), f(1), f(2)});
        }
        if (const u32 q = load<u32>(c + kCompB + 4 * j); q != 0) {
            ctx.gpr[11] = q + 176;  // r11 left as the original's
            call(ctx, kAnimB351F30, {an, q, load<u32>(q + 32), q + 40, q + 72, q + 104, q + 136, q + 176});
        } else if (j == 7) {
            ctx.gpr[3] = 0;  // r3 left as the original leaves it
        }
    }
}
