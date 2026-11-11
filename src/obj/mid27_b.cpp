// Game code, medium sweep part 27: the section reset of the
// Obj_Dtor_B1EFD0 objects and the two pair fillers of the bank loaders
// (Bank_LoadPairs_0044F0C0 / _0044F3BC in mid26_c.cpp).
#include "mid27.h"

using namespace sly4;
using namespace sly4::mid27;

namespace {
using namespace m27fn;
constexpr u32 kSetModeBase = 0x003260F0u;   // Section_SetModeBase_003260F0(o, mode, u8)
constexpr u32 kRegUnlink = 0x0048F298u;     // Registry_Unlink_0048F298(o, 0)
constexpr u32 kSetFlagHash = 0x0048FA54u;   // Obj_SetFlagHash_0048FA54(o, hash, 0)

// SmallFree(e, 144) for each entry of the vector +2772 (u16 count +2780,
// re-read), then the count = 0. Returns the count read last.
u32 FreeRecs(PPUContext& ctx, u32 o) {
    u32 i = 0, n;
    while (i < (n = load<u16>(o + 2780))) {
        call(ctx, SmallFree27, {load<u32>(load<u32>(o + 2772) + 4 * i), 144});
        ++i;
    }
    store<u16>(o + 2780, 0);
    return n;
}

inline f64 Sgl(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }
inline f64 Fsel(f64 a, f64 b, f64 c) { return a >= 0.0 ? b : c; }
}  // namespace

// u32 f(T* o, s32 mode, u8 c): Section_SetModeBase_003260F0(o, mode, c).
// Mode 4: +2612 / +2616 = 0, every record of the vector +2572 (u16 count
// +2580, re-read) goes through f_0033C5B4(rec), Registry_Unlink_0048F298
// (o, 0), Obj_SetFlagHash_0048FA54(o, 0xA8B960FF, 0), the +2772 records
// freed (r3 = 0). Mode 5: each record's group (+68) gets its +56 / +60 /
// +48 / +52 cleared, Group_Release_004563A0(g, 1) and
// Obj_ForEachSet_004562F4(g, 0); Registry_Unlink_0048F298(o, 0), the +2772
// records freed (r3 = the count read last). Other modes: r3 = the base's.
DECOMP(0033CE20, Section_Reset_0033CE20) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 mode = arg<u32>(ctx, 1);
    call(ctx, kSetModeBase, {o, mode, arg<u32>(ctx, 2) & 0xFFu});
    if (mode == 4) {
        store<u32>(o + 2612, 0);
        store<u32>(o + 2616, 0);
        for (u32 i = 0; i < load<u16>(o + 2580); ++i) call(ctx, 0x0033C5B4u, {load<u32>(load<u32>(o + 2572) + 4 * i)});
        call(ctx, kRegUnlink, {o, 0});
        call(ctx, kSetFlagHash, {o, 0xA8B960FFu, 0});
        FreeRecs(ctx, o);
        ctx.gpr[3] = 0;
        return;
    }
    if (mode != 5) return;
    for (u32 i = 0; i < load<u16>(o + 2580); ++i) {
        const u32 slot = load<u32>(o + 2572) + 4 * i;
        if (const u32 g = load<u32>(load<u32>(slot) + 68); g != 0) {
            store<u32>(g + 56, 0);
            store<u32>(g + 60, 0);
            store<u32>(g + 48, 0);
            store<u32>(g + 52, 0);
        }
        const u32 rec = load<u32>(slot);
        if (const u32 g = load<u32>(rec + 68); g != 0) {
            call(ctx, 0x004563A0u, {g, 1});
            call(ctx, 0x004562F4u, {load<u32>(rec + 68), 0});
        }
    }
    call(ctx, kRegUnlink, {o, 0});
    ctx.gpr[3] = FreeRecs(ctx, o);
}

// u32 f(Array* a, IffReader* r): n = Iff_ReadU32("iSize"); n times a
// 112-byte record is allocated (Mem_Alloc(112, 0, 16)) and initialised
// (scales 1.0, range 24000.0, the two vectors at +48 / +64 from the
// global 0x01164B40, +40 = their larger length clamped below by 0.0,
// +84 = -1), read by f_00440CD8(a, rec, r, *0x00F65B3C) and appended to a
// as {rec->+0, rec} (a failed allocation still reads the null record's
// first word). Returns n.
DECOMP(00441178, Bank_FillPairs_00441178) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 r = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);  // the count at 112(r1)
    const u32 cnt = frame.addr();
    store<u32>(cnt, 0);
    call(ctx, IffReadU32, {r, cnt, 0x00AE4600u});
    constexpr u32 kV = 0x01164B40u;
    const f64 zero = Lf(0x00441170u);
    for (u32 i = 0; i < load<u32>(cnt); ++i) {
        const u32 p = static_cast<u32>(call(ctx, MemAlloc, {112, 0, 16}));
        const u32 g = load<u32>(0x00F65B3Cu);
        if (p != 0) {
            store<u32>(p + 8, 0);
            store<u32>(p + 12, 0);
            store<u32>(p + 16, 0x3F800000u);
            store<u32>(p + 20, 0x3F800000u);
            store<u32>(p + 24, 0x3F800000u);
            store<u32>(p + 28, 0x46BB7E00u);
            store<u32>(p + 32, 0x3F800000u);
            store<u32>(p + 36, 0);
            const f64 x1 = Lf(kV), y1 = Lf(kV + 4), z1 = Lf(kV + 8);
            store<u32>(p + 48, load<u32>(kV));
            store<u32>(p + 52, load<u32>(kV + 4));
            store<u32>(p + 56, load<u32>(kV + 8));
            const f64 x2 = Lf(kV), y2 = Lf(kV + 4);
            store<u32>(p + 64, load<u32>(kV));
            store<u32>(p + 68, load<u32>(kV + 4));
            const u32 z2w = load<u32>(kV + 8);
            const f64 z2 = Lf(kV + 8);
            store<u32>(p + 0, 0);
            store<u32>(p + 4, 0);
            store<u8>(p + 80, 0);
            store<u8>(p + 81, 0);
            store<u8>(p + 82, 0);
            store<u32>(p + 84, 0xFFFFFFFFu);
            store<u32>(p + 88, 0);
            store<u32>(p + 92, 0);
            store<u32>(p + 72, z2w);
            f64 s1 = Sgl(y1 * y1);
            s1 = Sgl(std::fma(x1, x1, s1));
            s1 = Sgl(std::fma(z1, z1, s1));
            f64 s2 = Sgl(y2 * y2);
            s2 = Sgl(std::fma(x2, x2, s2));
            s2 = Sgl(std::fma(z2, z2, s2));
            const f64 l1 = Sgl(std::sqrt(s1)), l2 = Sgl(std::sqrt(s2));
            f64 m = Fsel(Sgl(l1 - l2), l1, l2);
            m = Fsel(Sgl(zero - m), zero, m);
            store<u32>(p + 40, std::bit_cast<u32>(static_cast<f32>(m)));
            // The original calls on with f1 = the clamped length (f1 is not
            // written again before the return).
            ctx.fpr[1] = m;
        }
        call(ctx, 0x00440CD8u, {a, p, r, static_cast<u64>(static_cast<s64>(static_cast<s32>(g)))});
        PairPush27(ctx, a, load<u32>(p), p, false);
    }
    ctx.gpr[3] = load<u32>(cnt);
}

// u32 f(Array* a, IffReader* r): n = Iff_ReadU32("iSize"); n times a
// 252-byte record is allocated (Mem_Alloc(252, 0, 16), not checked), its
// key read (Iff_ReadU32 "iSoundHash") and its 3 x 3 x 3 {s32, f32} entries
// (FileReader_ReadByte "iTargetRegisterValue", sign-extended, then
// FileReader_ReadF32Swapped "fFadeDuration"); appended to a as {key, rec}.
// Returns n; f1 is left as the float read last.
DECOMP(004413A4, Bank_FillPairs_004413A4) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 r = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);  // count 112(r1), key 116, f32 120, byte 124
    const u32 cnt = frame.addr(), key = cnt + 4, fv = cnt + 8, bv = cnt + 12;
    store<u32>(cnt, 0);
    call(ctx, IffReadU32, {r, cnt, 0x00AE4600u});
    for (u32 i = 0; i < load<u32>(cnt); ++i) {
        store<u32>(key, 0);
        const u32 p = static_cast<u32>(call(ctx, MemAlloc, {252, 0, 16}));
        call(ctx, IffReadU32, {r, key, 0x00AE4630u});
        for (u32 x = 0; x < 3; ++x)
            for (u32 y = 0; y < 3; ++y)
                for (u32 z = 0; z < 3; ++z) {
                    const u32 e = p + 84 * x + 28 * y + 8 * z;
                    store<u8>(bv, 0);
                    store<u32>(fv, 0);
                    call(ctx, ReadByte, {r, bv, 0x00AE4608u});
                    call(ctx, ReadF32, {r, fv, 0x00AE4620u});
                    store<u32>(e, static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(bv)))));
                    ctx.fpr[1] = Lf(fv);
                    store<u32>(e + 4, load<u32>(fv));
                }
        PairPush27(ctx, a, load<u32>(key), p, true);
    }
    ctx.gpr[3] = load<u32>(cnt);
}
