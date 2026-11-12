// Matrix_FromAxisAngle (0x0056A410) and HashMap_Find (0x007948E4).
#include "matrix.h"

#include "../resource/tags.h"

namespace sly4::view {
using namespace opentit::native;

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

// A rotation about `axis` (assumed unit length) from a fast sine/cosine;
// rows' xyz at out+0/16/32, out+48..56 set to 0 (w lanes untouched).
void Matrix_FromAxisAngle(be_f32* out, const be_f32* axis, f64 angle) {
    const f64 pi = sgl(angle >= 0.0 ? kPi : kMinusPi);  // fsel + frsp
    const f64 ax = axis[0], ay = axis[1], az = axis[2];
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
    const f64 ax2 = axis[0], ay2 = axis[1], az2 = axis[2];
    out[0] = static_cast<f32>(fmadds(ax2, t0, c));
    out[1] = static_cast<f32>(fmadds(ax2, t1, sz));
    out[2] = static_cast<f32>(fmadds(ax2, t2, -sy));
    out[4] = static_cast<f32>(fmadds(ay2, t0, -sz));
    out[5] = static_cast<f32>(fmadds(ay2, t1, c));
    out[6] = static_cast<f32>(fmadds(ay2, t2, sx));
    be_u32* w = reinterpret_cast<be_u32*>(out);
    w[14] = 0u;
    w[13] = 0u;
    w[12] = 0u;
    out[8] = static_cast<f32>(fmadds(az2, t0, sy));
    out[9] = static_cast<f32>(fmadds(az2, t1, -sx));
    out[10] = static_cast<f32>(fmadds(az2, t2, c));
}

// Sorts the entries first if they were changed (qsort), then binary
// searches them; returns the entry's value slot, or map+16 when there is
// none.
be_u32* HashMap_Find(tags::SortedMap* map, const be_u32* key) {
    const u16 flags = map->flags;
    if (flags & kMapUnsorted) {
        map->flags = static_cast<u16>(flags & 0xFFFE);
        if (map->count != 0)  // cmpwi of a zero-extended u16: > 0
            call(mfn::qsort, map->entries.get(), static_cast<u32>(map->count), kMapEntrySize, map->compare.get());
    }
    s32 hi = static_cast<s32>(map->count) - 1, lo = 0;
    u32 entries = map->entries;
    s32 found = -1;
    if (map->compare == kMapCompareU32) {
        if (hi < 0) return &map->null_value;
        do {
            const s32 mid = (hi + lo) >> 1;
            const u32 k = *key;
            const u32 e = gref<be_u32>(entries + static_cast<u32>(mid) * kMapEntrySize);
            if (e > k) hi = mid - 1;
            else if (e < k) lo = mid + 1;
            else {
                found = mid;
                break;
            }
        } while (hi >= lo);
    } else {
        GuestFrame frame(8);
        be_u32* copy = frame.as<be_u32>();
        *copy = key->get();
        if (hi < 0) return &map->null_value;
        do {
            const s32 mid = (hi + lo) >> 1;
            const s32 r = call_ptr<s32>(map->compare, entries + static_cast<u32>(mid) * kMapEntrySize, copy);
            if (r > 0) hi = mid - 1;
            else if (r < 0) lo = mid + 1;
            else {
                entries = map->entries;
                found = mid;
                break;
            }
        } while (hi >= lo);
    }
    if (found < 0) return &map->null_value;
    return &gref<be_u32>(entries + static_cast<u32>(found) * kMapEntrySize + 4);
}

}  // namespace sly4::view

NATIVE(0056A410, sly4::view::Matrix_FromAxisAngle)
NATIVE(007948E4, sly4::view::HashMap_Find)
