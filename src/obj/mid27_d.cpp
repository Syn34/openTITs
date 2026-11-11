// Game code, medium sweep part 27: destructors of two camera / gadget
// objects (the photo binocucom and the 0x00B14B90 gadget on the
// 0x00B12550 base).
#include "mid27.h"

using namespace sly4;
using namespace sly4::mid27;

namespace {
using namespace m27fn;
inline f32 Fs(f64 x) { return static_cast<f32>(x); }
inline f32 Mul(f32 a, f32 b) { return Fs(static_cast<f64>(a) * b); }
inline f32 Msub(f32 a, f32 c, f32 b) { return Fs(std::fma(static_cast<f64>(a), c, -static_cast<f64>(b))); }
inline f32 Madd(f32 a, f32 c, f32 b) { return Fs(std::fma(static_cast<f64>(a), c, static_cast<f64>(b))); }
inline void Sf(u32 ea, f32 v) { store<u32>(ea, std::bit_cast<u32>(v)); }
}  // namespace

// T::~T(o) (vtable 0x00B13DA0 -> 0x00B0DAEC; Delete_0010C60C): +192 leaves
// the list o->+12 (List_RemoveAll_000D91BC, cleared); the UI node
// ".InGame.BinocucomPhoto" (Ui_Find) gets vt[88](u, 0). Unless the camera
// o->+12 reports 3 for vt[32](c, 0) and not 1 for vt[32](c, 1), its
// matrix (+96, rows at 16-byte stride) is rebuilt around the up vector
// o+80: x = normalize(row1 x up), z = normalize(x x row1), and passed to
// vt[84](c, m) (stack copy, rows 1 and 3 unchanged).
// r3 = 0x00B0DAEC.
DECOMP(0010C3C8, Binocucom_Dtor_0010C3C8) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 64);  // the 4x4 matrix at 112(r1)
    const u32 m = frame.addr();
    store<u32>(o, 0x00B13DA0u);
    if (const u32 e = load<u32>(o + 192); e != 0) {
        call(ctx, 0x000D91BCu, {load<u32>(o + 12), e});
        store<u32>(o + 192, 0);
    }
    if (const u32 u = static_cast<u32>(call(ctx, 0x004D5544u, {0x00AC6E0Cu, 0})); u != 0) VCall(ctx, u, 88, {u, 0});
    u32 c = load<u32>(o + 12);
    bool rebuild = static_cast<u32>(VCall(ctx, c, 32, {c, 0})) != 3;
    if (!rebuild) {
        c = load<u32>(o + 12);
        rebuild = static_cast<u32>(VCall(ctx, c, 32, {c, 1})) == 1;
    }
    if (rebuild) {
        c = load<u32>(o + 12);
        const u32 p = c + 96;
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k) store<u32>(m + 16 * row + 4 * k, load<u32>(p + 16 * row + 4 * k));
        const f32 r1x = F32(load<u32>(p + 16)), r1y = F32(load<u32>(p + 20)), r1z = F32(load<u32>(p + 24));
        const f32 vz = F32(load<u32>(o + 88)), vy = F32(load<u32>(o + 84)), vx = F32(load<u32>(o + 80));
        const f32 one = F32(load<u32>(0x0010C3C0u));
        f32 ay = Msub(r1z, vx, Mul(r1x, vz));
        f32 ax = Msub(r1y, vz, Mul(r1z, vy));
        f32 az = Msub(r1x, vy, Mul(r1y, vx));
        f32 inv = Fs(static_cast<f64>(one) / Fs(std::sqrt(static_cast<f64>(Madd(az, az, Madd(ax, ax, Mul(ay, ay)))))));
        ax = Mul(ax, inv);
        az = Mul(az, inv);
        ay = Mul(ay, inv);
        const f32 t7 = Mul(ax, r1z), t9 = Mul(az, r1y), t10 = Mul(ay, r1x);
        Sf(m + 0, ax);
        f32 zy = Msub(az, r1x, t7);
        f32 zx = Msub(ay, r1z, t9);
        f32 zz = Msub(ax, r1y, t10);
        Sf(m + 4, ay);
        Sf(m + 8, az);
        const f32 len = Fs(std::sqrt(static_cast<f64>(Madd(zz, zz, Madd(zx, zx, Mul(zy, zy))))));
        const f32 inv2 = Fs(static_cast<f64>(one) / len);
        zx = Mul(zx, inv2);
        zy = Mul(zy, inv2);
        zz = Mul(zz, inv2);
        Sf(m + 32, zx);
        Sf(m + 36, zy);
        Sf(m + 40, zz);
        // f1-f10 as the original leaves them for the call.
        ctx.fpr[1] = zy; ctx.fpr[2] = zz; ctx.fpr[3] = zx; ctx.fpr[4] = inv2;
        ctx.fpr[5] = az; ctx.fpr[6] = ay; ctx.fpr[7] = len; ctx.fpr[8] = one;
        ctx.fpr[9] = t9; ctx.fpr[10] = t10;
        VCall(ctx, c, 84, {c, m});
    }
    store<u32>(o, 0x00B0DAECu);
    ctx.gpr[3] = 0x00B0DAECu;  // the vtable word just stored
}

// T::~T(o) (vtables 0x00B14B98 / +380 0x00B14D48 / +384 0x00B14D70;
// Delete_00184700 and the adjustor thunks): when o->+348 > *0x00184278
// (0.0) and the player (world +0x1281F4) has bit 0x40000000 of +52,
// Track_SelectKey_003DE47C(player, o->+160->+208->+100; f1 = o->+348).
// List1013B48_Remove(o); +1680 = 0. With a player and the attached camera
// +1312 having bit 0x40000 of +172 (player flag as above): the camera's
// rotation (+96, three rows) and o's position +144 make a matrix
// (Mat34_FromRotPos553B4C) passed to its vt[84], its +664 / +624 cleared
// and Msg_Post(o, 0x0C8B8AB8, 0x00531BA0, 0). The camera's +664 and +1312
// are cleared. The six globals 0x00F6498C..0x00F649A0 get vt[8] (and are
// cleared when the first one's s16 +4 is 1); +1328 vt[8]; bit 0x200 of
// +1316's +136 cleared; the Havok array +1676 (+1684) freed; the PtrVector
// +1656 destroyed; vtables 0x00B12528 / 0x00B1200C / 0x00B126B8 and
// VtB12550_Dtor_000DF934(o) (r3 = its).
DECOMP(00184280, Gadget_Dtor_B14B90_00184280) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 144);  // pos 112, rows 128, matrix 176 in the original frame
    const u32 pos = frame.addr(), rot = pos + 16, mtx = pos + 64;
    constexpr u32 kPlayerSlot = 0x001281F4u;
    store<u32>(o, 0x00B14B98u);
    store<u32>(o + 380, 0x00B14D48u);
    store<u32>(o + 384, 0x00B14D70u);
    const f64 zero = Lf(0x00184278u);
    const f64 t = Lf(o + 348);
    ctx.fpr[1] = t;
    ctx.fpr[2] = zero;
    const u32 pl = load<u32>(load<u32>(mid25::kWorld25) + kPlayerSlot);
    if (t > zero && pl != 0 && (load<u32>(pl + 52) & 0x40000000u) != 0)
        callf(ctx, 0x003DE47Cu, {pl, load<u32>(load<u32>(load<u32>(o + 160) + 208) + 100)}, {t});
    call(ctx, 0x000D68CCu, {o});
    store<u32>(o + 1680, 0);
    if (load<u32>(load<u32>(mid25::kWorld25) + kPlayerSlot) != 0) {
        const u32 c = load<u32>(o + 1312);
        if (c != 0 && (load<u32>(pl + 52) & 0x40000000u) != 0 && (load<u32>(c + 172) & 0x40000u) != 0) {
            for (u32 k = 0; k < 3; ++k) store<u32>(pos + 4 * k, load<u32>(o + 144 + 4 * k));
            for (u32 row = 0; row < 3; ++row)
                for (u32 k = 0; k < 3; ++k) store<u32>(rot + 16 * row + 4 * k, load<u32>(c + 96 + 16 * row + 4 * k));
            ctx.fpr[1] = Lf(c + 136);  // left over from the copy
            call(ctx, 0x00553B4Cu, {mtx, rot, pos});
            VCall(ctx, c, 84, {c, mtx});
            store<u32>(c + 664, 0);
            store<u32>(c + 624, 0);
            call(ctx, 0x00470660u, {o, 0x0C8B8AB8u, 0x00531BA0u, 0});  // Msg_Post
        }
    }
    if (const u32 c = load<u32>(o + 1312); c != 0) {
        store<u32>(c + 664, 0);
        store<u32>(o + 1312, 0);
    }
    constexpr u32 kG = 0x00F6498Cu;
    bool clear = false;
    if (const u32 g = load<u32>(kG); g != 0) {
        clear = static_cast<s16>(load<u16>(g + 4)) == 1;
        VCall(ctx, g, 8, {g});
    }
    for (u32 off : {4u, 8u, 12u, 16u, 20u}) Vt8Slot(ctx, kG + off, false);
    if (clear)
        for (u32 off : {0u, 4u, 8u, 12u, 16u, 20u}) store<u32>(kG + off, 0);
    Vt8Slot(ctx, o + 1328, false);
    if (const u32 x = load<u32>(o + 1316); x != 0) store<u32>(x + 136, load<u32>(x + 136) & ~0x200u);
    store<u32>(o + 1680, 0);
    HkArrFree27(ctx, o, 1676, 1684, 4, false);
    store<u32>(o + 1676, 0);
    store<u32>(o + 1684, 0x80000000u);
    PtrVecDtor27(ctx, o + 1656);
    store<u32>(o + 384, 0x00B12528u);
    store<u32>(o + 380, 0x00B1200Cu);
    store<u32>(o, 0x00B126B8u);
    call(ctx, 0x000DF934u, {o});
}
