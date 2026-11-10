// Game code, medium sweep part 4: a group of three inlined copies of the
// same "flush" method (three classes, the members at different offsets):
// when its pending flag is set, it notifies handle 0x2508CAD1, calls
// vfn[5](obj, 1) on each object of its PtrVector (emptied), sets the
// world's record at +0x1283A8 from three tuning values and fills the
// world's 28-byte slot (+0x128334 + 28 * index) from more of them.
#include "mid4.h"

using namespace sly4;
using namespace sly4::mid4;

namespace {

namespace mfn {
constexpr u32 Handle_Notify36742C = 0x0036742C;  // (handle, a, b, c)
constexpr u32 Rec_Set_00470560 = 0x00470560;     // (Rec*, f32 x, f32 y, f32 z, ?, ?, ?, u32 w, bool b)
constexpr u32 kCurrentWorld = 0x00F65CA0;        // World*
constexpr u32 kTvX = 0x010B9B60;                 // TuningVar (f32 +72)
constexpr u32 kTvW = 0x010B9BAC;                 // TuningVar (s32 +72)
constexpr u32 kTvZ = 0x010B9BF8;                 // TuningVar (f32 +72)
constexpr u32 kTvColor = 0x010B9B14;             // TuningVar (u8[4] +72)
constexpr u32 kColor2 = 0x01164CA0;              // u8[4]
}  // namespace mfn

f64 F32At(u32 ea) { return std::bit_cast<f64>(LfsBits(load<u32>(ea))); }

// void f(T* o): flag / vec / count / index: o's u8 pending flag, its
// PtrVector's data (the u16 count at +8 past it), the u8 slot index; zero:
// the address of the 0.0f the original reads from its own text (the word
// before the function), hi: the lis half of its address, left in r6.
void Flush(PPUContext& ctx, u32 flag, u32 vec, u32 index, u32 zero, u32 hi) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + flag) != 0) {
        call(ctx, mfn::Handle_Notify36742C, {0x2508CAD1, 0, 1, 0});
        for (u32 k = 0; k < load<u16>(o + vec + 8); ++k) {
            const u32 obj = load<u32>(load<u32>(o + vec) + 4 * k);
            VCall(ctx, obj, 20, {obj, 1});
        }
        store<u16>(o + vec + 8, 0);
        const u32 w = load<u32>(mfn::kCurrentWorld);
        // r4-r6 as the original leaves them (the callee reads r7 / r8)
        callf(ctx, mfn::Rec_Set_00470560,
              {w + 0x1283A8, zero, w, hi, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(mfn::kTvW + 72)))), 1},
              {F32At(mfn::kTvX + 72), F32At(zero), F32At(mfn::kTvZ + 72)});
        // u8 index compared signed with 0: the original's world counter
        // path (+0x1283A4 ++) for a negative one is never taken.
        const u32 i = load<u8>(o + index);
        const u32 w2 = load<u32>(mfn::kCurrentWorld);
        const u32 z = load<u32>(mfn::kTvZ + 72);
        const u32 e = w2 + 28 * i;
        store<u32>(e + 0x128334, (load<u32>(e + 0x128334) & 0x007FFFFFu) | 0x20800000u);
        for (u32 k = 0; k < 4; ++k) store<u8>(e + 0x128336 + k, load<u8>(mfn::kTvColor + 72 + k));
        for (u32 k = 0; k < 4; ++k) store<u8>(e + 0x12833A + k, load<u8>(mfn::kColor2 + k));
        store<u32>(e + 0x128340, 0x3F800000);
        store<u32>(e + 0x128344, 0);
        store<u32>(e + 0x128348, z);
        store<u32>(e + 0x12834C, 0);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(z));
    }
    store<u8>(o + flag, 0);
    ctx.gpr[3] = 0;
}

}  // namespace

// void f(T* o): Flush (flag +117, PtrVector +120, index +252).
DECOMP(003DEBF8, Flush_003DEBF8) { Flush(ctx, 117, 120, 252, 0x003DEBF4, 0x003E0000); }

// void f(T* o): Flush (flag +2325, PtrVector +2340, index +2368).
DECOMP(003E5DF8, Flush_003E5DF8) { Flush(ctx, 2325, 2340, 2368, 0x003E5DF4, 0x003E0000); }

// void f(T* o): Flush (flag +181, PtrVector +232, index +260).
DECOMP(003F2718, Flush_003F2718) { Flush(ctx, 181, 232, 260, 0x003F2710, 0x003F0000); }
