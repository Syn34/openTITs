// Game code, medium sweep part 5: helpers shared by mid5_*.cpp (still
// recompiled game functions of 120-400 instructions: action-script node
// binders and timer conditions, high-score tables, XML element readers and
// other template families).
#pragma once

#include <bit>
#include <cmath>

#include "mid4.h"

namespace sly4 {
namespace mid5 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using mid4::NodeBindingAddName;
using obj19::VCall;
using obj19::VecDtor;

namespace m5fn {
constexpr u32 Mem_Alloc = 0x0054ED38;          // (size, pool, align)
constexpr u32 String_CopyN = 0x00553638;       // (dst, src, n)
constexpr u32 String_Format = 0x005534D0;      // (fmt, ...) -> static buffer
constexpr u32 String_HashNoCase = 0x0056A710;  // (s) -> u32
constexpr u32 Node_Bind = 0x004D3088;          // (owner, const char* name, kind, k6, -1, 0) -> binding
constexpr u32 Binding_Reserve = 0x004D2BB8;    // (b, n): +72 = n, +68 = the name array
constexpr u32 Owner_AddName = 0x004D3378;      // (owner, const char* name, s32 kind) -> 1
}  // namespace m5fn

constexpr u32 kGame = 0x00F65F84;  // Game*: +268 s32 sync-type count, +272 {const char* name, u32}[], +276 u32 their capacity (name?)

// Binding* f(owner, name, kind, k6): Node_Bind(owner, name, kind, k6, -1, 0).
inline u32 Bind(PPUContext& ctx, u32 owner, u32 name, u32 kind, u32 k6) {
    return static_cast<u32>(call(ctx, m5fn::Node_Bind, {owner, name, kind, k6, ~0ull, 0}));
}

// ---------------------------------------------------------------- XML element readers

namespace xfn {
constexpr u32 XmlCursor_ReadBool = 0x004F986C;   // (c, hash, u8* out, u8 default) -> 0 when found
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;    // (c, hash, s32* out, s32 default)
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;    // (c, hash, f32* out), default in f1
constexpr u32 XmlCursor_ReadVec2 = 0x004F9C9C;   // (c, hash, f32 out[2], const f32 default[2])
constexpr u32 XmlCursor_ReadVec3 = 0x004F9E0C;   // (c, hash, f32 out[3], const f32 default[3])
constexpr u32 XmlCursor_ReadColor = 0x004F9F9C;  // (c, hash, u8 out[4], const u8 default[4])
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;   // (c, hash, u32* out, u32 default)
constexpr u32 XmlCursor_FindAttr = 0x004F9534;   // (c, hash) -> const char* or 0
}  // namespace xfn

// XmlCursor_Read*(c, hash, out, def): the result (0 when found).
inline u32 XRead(PPUContext& ctx, u32 fn, u32 c, u32 hash, u32 out, u32 def) {
    return static_cast<u32>(call(ctx, fn, {c, hash, out, def}));
}
// The u32 hash field at p, defaulting to its value.
inline u32 XHash(PPUContext& ctx, u32 c, u32 hash, u32 p) { return XRead(ctx, xfn::XmlCursor_ReadHash, c, hash, p, load<u32>(p)); }
// The s32 field at p, defaulting to its value.
inline u32 XS32(PPUContext& ctx, u32 c, u32 hash, u32 p) {
    return static_cast<u32>(call(ctx, xfn::XmlCursor_ReadS32, {c, hash, p, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(p))))}));
}
// The u8 bool field at p, defaulting to its value.
inline u32 XBool(PPUContext& ctx, u32 c, u32 hash, u32 p) { return XRead(ctx, xfn::XmlCursor_ReadBool, c, hash, p, load<u8>(p)); }
// The f32 field at p, defaulting to its value (passed in f1).
inline u32 XF32(PPUContext& ctx, u32 c, u32 hash, u32 p) {
    callf(ctx, xfn::XmlCursor_ReadF32, {c, hash, p}, {std::bit_cast<f64>(LfsBits(load<u32>(p)))});
    return static_cast<u32>(ctx.gpr[3]);
}
// A bool attribute into the bit `mask` of the T at p, through the byte
// tmp: the default is the bit, the bit is then bit 0 of tmp. Returns the
// T stored.
template <typename T>
inline T XBit(PPUContext& ctx, u32 c, u32 hash, u32 p, T mask, u32 tmp) {
    XRead(ctx, xfn::XmlCursor_ReadBool, c, hash, tmp, (load<T>(p) & mask) != 0 ? 1u : 0u);
    const T v = static_cast<T>((load<T>(p) & static_cast<T>(~mask)) | ((load<u8>(tmp) & 1u) != 0 ? mask : 0));
    store<T>(p, v);
    return v;
}

// A field read in a list: kind 'H' hash, 'F' f32, 'B' u8 bool, 'S' s32,
// each defaulting to the field's value.
struct XField {
    char kind;
    u32 hash, off;
};
inline u32 XFields(PPUContext& ctx, u32 c, u32 o, std::initializer_list<XField> fs) {
    u32 r = 0;
    for (const XField& f : fs) {
        switch (f.kind) {
        case 'H': r = XHash(ctx, c, f.hash, o + f.off); break;
        case 'F': r = XF32(ctx, c, f.hash, o + f.off); break;
        case 'B': r = XBool(ctx, c, f.hash, o + f.off); break;
        default: r = XS32(ctx, c, f.hash, o + f.off); break;
        }
    }
    return r;
}

}  // namespace mid5
}  // namespace sly4
