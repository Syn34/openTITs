// World_HandleEvent callees, part 23d: the closest point on a segment and
// an object's type index (from its type component or its name).
#include "hevent22.h"

#include <bit>
#include <cmath>

using namespace sly4;
using namespace sly4::hevent;

namespace {
// Segment_ClosestPoint19248's constants at 0x00019238: 1e-5 (shortest
// segment), 1.0, 0.0.
constexpr u32 kSegConsts = 0x00019238;

f64 lfs(u32 ea) { return static_cast<f64>(std::bit_cast<f32>(load<u32>(ea))); }
f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }

// Obj_TypeIndex4A1244: component +8 type hash, +12 u16 property count, +16
// sorted u32 property hashes, +20 u16 value offsets, +24 value block. The
// "Type" property's u32 value maps through the byte table at 0x00AEB576;
// names map through the 20 hashes at 0x00AEB630.
constexpr u32 kObjComponents = 12, kNumComponentSlots = 4;
constexpr u32 kTypeComponent = 0x922D3BD1;
constexpr u32 kHashType = 0x0027873A;  // "Type"
constexpr u32 kCompHash = 8, kCompCount = 12, kCompKeys = 16, kCompOffsets = 20, kCompValues = 24;
constexpr u32 kTypeIndexByValue = 0x00AEB576;  // u8[]
constexpr u32 kTypeNameHashes = 0x00AEB630, kNumTypeNames = 20;
constexpr u32 kSlotName = 12;  // vtable: () -> const char* name
constexpr u32 kStringHashNoCase = 0x0056A710;
constexpr u32 kNameBuf = 48;  // the original's buffer (r1 + 112 in a 160-byte frame)
}  // namespace

// void Segment_ClosestPoint19248(vec3* out, -, const vec3* a, const vec3*
// b, const vec3* p) (name?): out = the point of segment a-b closest to p,
// in single precision: a when the segment is shorter than 1e-5 (or NaN) or
// p projects before a, b when it projects past b, else b + d * dot(d, p -
// b) with d the unit direction. Copies are raw words. Leaves r4 =
// 0x00019238 and f1 as the original does.
DECOMP(00019248, Segment_ClosestPoint19248) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3), p = arg<u32>(ctx, 4);
    ctx.gpr[4] = kSegConsts;
    const f64 by = lfs(b + 4), ay = lfs(a + 4), bx = lfs(b + 0), ax = lfs(a + 0), bz = lfs(b + 8), az = lfs(a + 8);
    const u32 ax_bits = load<u32>(a + 0), bx_bits = load<u32>(b + 0);
    f32 dy = fsubs(by, ay), dx = fsubs(bx, ax), dz = fsubs(bz, az);
    f32 len = static_cast<f32>(static_cast<f64>(dy) * dy);
    len = fmadds(dx, dx, len);
    len = fmadds(dz, dz, len);
    len = static_cast<f32>(std::sqrt(static_cast<f64>(len)));
    auto copy_a = [&] {
        store<u32>(out + 0, ax_bits);
        store<u32>(out + 4, load<u32>(a + 4));
        const u32 z = load<u32>(a + 8);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
        store<u32>(out + 8, z);
    };
    if (!(len > lfs(kSegConsts) || len == lfs(kSegConsts))) return copy_a();
    const f32 inv = static_cast<f32>(lfs(kSegConsts + 4) / len);
    const f64 py = lfs(p + 4), px = lfs(p + 0), pz = lfs(p + 8);
    const f64 zero = lfs(kSegConsts + 8);
    ctx.fpr[1] = zero;
    dy = static_cast<f32>(static_cast<f64>(dy) * inv);
    dx = static_cast<f32>(static_cast<f64>(dx) * inv);
    dz = static_cast<f32>(static_cast<f64>(dz) * inv);
    f32 t = static_cast<f32>(static_cast<f64>(dy) * fsubs(py, ay));
    t = fmadds(dx, fsubs(px, ax), t);
    t = fmadds(dz, fsubs(pz, az), t);
    if (!(t > zero)) return copy_a();
    f32 u = static_cast<f32>(static_cast<f64>(dy) * fsubs(py, by));
    u = fmadds(dx, fsubs(px, bx), u);
    u = fmadds(dz, fsubs(pz, bz), u);
    if (u < zero) {
        const f32 x = fmadds(dx, u, bx), y = fmadds(dy, u, by), z = fmadds(dz, u, bz);
        ctx.fpr[1] = x;
        store<f32>(out + 0, x);
        store<f32>(out + 4, y);
        store<f32>(out + 8, z);
        return;
    }
    store<u32>(out + 0, bx_bits);
    store<u32>(out + 4, load<u32>(b + 4));
    const u32 z = load<u32>(b + 8);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
    store<u32>(out + 8, z);
}

// s32 Obj_TypeIndex4A1244(Object* o) (name?): with a type component
// (0x922D3BD1) among the first 4 of +12 (stopping at a null slot) whose
// sorted property list has "Type", the byte at 0x00AEB576 + that value.
// Otherwise the name (vtable +12) between its first and second '_' is
// hashed (String_HashNoCase) and its index among the 20 hashes at
// 0x00AEB630 returned, or -1 (as an unsigned 32-bit value).
DECOMP(004A1244, Obj_TypeIndex4A1244) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 comps = load<u32>(o + kObjComponents);
    if (comps != 0) {
        for (u32 i = 0; i < kNumComponentSlots; ++i) {
            const u32 c = load<u32>(comps + 4 + 4 * i);
            if (c == 0) break;
            if (load<u32>(c + kCompHash) != kTypeComponent) continue;
            const u32 n = load<u16>(c + kCompCount);
            u32 keys = load<u32>(c + kCompKeys);
            for (s32 j = 0; j < static_cast<s32>(n); ++j, keys += 4) {
                const u32 k = load<u32>(keys);
                if (k == kHashType) {
                    const u32 off = load<u16>(load<u32>(c + kCompOffsets) + 2u * static_cast<u32>(j));
                    const u32 v = load<u32>(load<u32>(c + kCompValues) + off);
                    return ret(ctx, static_cast<u32>(load<u8>(kTypeIndexByValue + v)));
                }
                if (k > kHashType) break;
            }
            break;
        }
    }
    const u32 name = static_cast<u32>(vcall(ctx, o, kSlotName, {o}));
    // Unbounded copy, as in the original (which overruns into its saved
    // registers past 32 characters).
    Stack frame(ctx, kNameBuf + 64);
    const u32 buf = frame.addr();
    u32 d = buf;
    for (u32 s = name; load<u8>(s) != 0;) {
        const u8 ch = load<u8>(s);
        s += 1;
        if (ch == '_') {
            for (u8 c; (c = load<u8>(s)) != 0 && c != '_'; ++s, ++d) store<u8>(d, c);
            break;
        }
    }
    store<u8>(d, 0);
    const u32 h = static_cast<u32>(call(ctx, kStringHashNoCase, {buf}));
    for (u32 i = 0; i < kNumTypeNames; ++i)
        if (h == load<u32>(kTypeNameHashes + 4 * i)) return ret(ctx, i);
    ret(ctx, 0xFFFFFFFFu);
}
