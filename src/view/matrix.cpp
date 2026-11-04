// Matrix_FromAxisAngle (0x0056A410) and HashMap_Find (0x007948E4).
#include "matrix.h"

using namespace sly4;
using namespace sly4::view;

namespace {

// Constants at 0x0056A3E8.
constexpr f64 kPi = 3.1415927410125732, kMinusPi = -3.1415927410125732;  // doubles
constexpr f32 kTwoPi = 6.28318548f;        // 0x40C90FDB
constexpr f32 kFourOverPi = 1.27323949f;   // 0x3FA2F983
constexpr f32 kMinusFourOverPiSq = -0.405284703f;  // 0xBECF817A
constexpr f32 kRefine = 0.224999994f;      // 0x3E666666
constexpr f32 kHalfPi = 1.57079637f;       // 0x3FC90FDB
constexpr f32 kPiF = 3.14159274f;          // 0x40490FDB

// The parabolic sine the original uses, for x in [-pi, pi].
f64 FastSin(f64 x) {
    const f64 y = fmadds(x, kFourOverPi, fmuls(fmuls(x, kMinusFourOverPiSq), std::fabs(x)));
    return fmadds(fmsubs(y, std::fabs(y), y), kRefine, y);
}

}  // namespace

// void Matrix_FromAxisAngle(f32 out[16], const f32 axis[3]; f1 = radians)
// A rotation about `axis` (assumed unit length) from a fast sine/cosine;
// rows' xyz at out+0/16/32, out+48..56 set to 0 (w lanes untouched).
DECOMP(0056A410, Matrix_FromAxisAngle) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 axis = arg<u32>(ctx, 1);
    const f64 angle = farg(ctx, 0);

    const f64 pi = sgl(angle >= 0.0 ? kPi : kMinusPi);  // fsel + frsp
    const f64 ax = load<f32>(axis), ay = load<f32>(axis + 4), az = load<f32>(axis + 8);
    f64 x = fadds(pi, angle);
    const f64 turns = sgl(rt::fcfid(rt::fctid(fdivs(x, kTwoPi), true)));
    x = fnmsubs(turns, kTwoPi, x);
    x = fsubs(x, pi);
    const f64 s = FastSin(x);
    f64 xc = fadds(x, kHalfPi);
    if (xc > static_cast<f64>(kPiF)) xc = fsubs(xc, kTwoPi);
    const f64 c = FastSin(xc);

    const f64 sx = fmuls(ax, s), sy = fmuls(ay, s), sz = fmuls(az, s);
    const f64 t0 = fnmsubs(ax, c, ax), t1 = fnmsubs(ay, c, ay), t2 = fnmsubs(az, c, az);
    const f64 ax2 = load<f32>(axis), ay2 = load<f32>(axis + 4), az2 = load<f32>(axis + 8);
    store<f32>(out + 0, static_cast<f32>(fmadds(ax2, t0, c)));
    store<f32>(out + 4, static_cast<f32>(fmadds(ax2, t1, sz)));
    store<f32>(out + 8, static_cast<f32>(fmadds(ax2, t2, -sy)));
    store<f32>(out + 16, static_cast<f32>(fmadds(ay2, t0, -sz)));
    store<f32>(out + 20, static_cast<f32>(fmadds(ay2, t1, c)));
    store<f32>(out + 24, static_cast<f32>(fmadds(ay2, t2, sx)));
    store<u32>(out + 56, 0);
    store<u32>(out + 52, 0);
    store<u32>(out + 48, 0);
    store<f32>(out + 32, static_cast<f32>(fmadds(az2, t0, sy)));
    store<f32>(out + 36, static_cast<f32>(fmadds(az2, t1, -sx)));
    store<f32>(out + 40, static_cast<f32>(fmadds(az2, t2, c)));
}

// u32* HashMap_Find(SortedMap* map, const u32* key)
// Sorts the entries first if they were changed (qsort), then binary
// searches them; returns the entry's value slot, or map+16 when there is
// none.
DECOMP(007948E4, HashMap_Find) {
    const u32 map = arg<u32>(ctx, 0);
    const u32 key = arg<u32>(ctx, 1);

    const u16 flags = field<u16>(map, kMapFlags);
    if (flags & kMapUnsorted) {
        field<u16>(map, kMapFlags) = static_cast<u16>(flags & 0xFFFE);
        if (field<u16>(map, kMapCount) != 0)  // cmpwi of a zero-extended u16: > 0
            call(ctx, mfn::qsort, {field<u32>(map, kMapEntries), field<u16>(map, kMapCount).get(), kMapEntrySize,
                                   field<u32>(map, kMapCompare)});
    }

    s32 hi = static_cast<s32>(field<u16>(map, kMapCount).get()) - 1, lo = 0;
    u32 entries = field<u32>(map, kMapEntries);
    s32 found = -1;
    if (field<u32>(map, kMapCompare) == kMapCompareU32) {
        if (hi < 0) return ret(ctx, map + kMapMiss);
        do {
            const s32 mid = (hi + lo) >> 1;
            const u32 k = load<u32>(key);
            const u32 e = load<u32>(entries + static_cast<u32>(mid) * kMapEntrySize);
            if (e > k) {
                hi = mid - 1;
            } else if (e < k) {
                lo = mid + 1;
            } else {
                found = mid;
                break;
            }
        } while (hi >= lo);
    } else {
        Stack frame(ctx, 8);
        store<u32>(frame.addr(), load<u32>(key));
        if (hi < 0) return ret(ctx, map + kMapMiss);
        do {
            const s32 mid = (hi + lo) >> 1;
            const s32 r = static_cast<s32>(call_ptr(ctx, field<u32>(map, kMapCompare),
                                                    {entries + static_cast<u32>(mid) * kMapEntrySize, frame.addr()}));
            if (r > 0) {
                hi = mid - 1;
            } else if (r < 0) {
                lo = mid + 1;
            } else {
                entries = field<u32>(map, kMapEntries);
                found = mid;
                break;
            }
        } while (hi >= lo);
    }
    if (found < 0) return ret(ctx, map + kMapMiss);
    ret(ctx, entries + static_cast<u32>(found) * kMapEntrySize + 4);
}
