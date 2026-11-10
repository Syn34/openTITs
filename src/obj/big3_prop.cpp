// Game code, large-function sweep part 3: the property-set flag reader
// 004A1440 (13 callers, among them the character / gadget setup
// functions 0012EB80..0015E2D8).
#include "big3.h"

using namespace sly4;
using namespace sly4::big3;

namespace b3p {
constexpr u32 kSetFull = 0x922D3BD1u;   // the property set with every field
constexpr u32 kSetShort = 0xBDE4F0E8u;  // the set with the bytes and four of the flags
// Property name hashes.
constexpr u32 kByte0 = 0x0027873Au, kByte1 = 0xBC55788Du;
constexpr u32 kFlag8000 = 0x89962D6Fu, kFlag4000 = 0xA8EA59B1u, kFlag2000 = 0xA8EA59B2u, kFlag1000 = 0x1DFF78B4u,
              kFlag0800 = 0x29FAC943u, kFlag0400 = 0xD4069C65u, kFlag0200 = 0xA78B0427u, kFlag0100 = 0x00011A6Bu,
              kFlag0080 = 0x8B3AAEC9u, kFlag0040 = 0x65AF9A49u, kFlag0020 = 0x9B828ACAu;
constexpr u32 kF4 = 0x1D6C16D5u, kF8 = 0xA981483Bu, kW12 = 0xD20A263Du;
}  // namespace b3p

namespace {

// The u32 property `hash` of set s, or dflt.
u32 PropWord(u32 s, u32 hash, u32 dflt) {
    const u32 p = PropFind(s, hash);
    return p != 0 ? load<u32>(p) : dflt;
}
// The u8 property `hash` of set s, or 0.
u32 PropByte(u32 s, u32 hash) {
    const u32 p = PropFind(s, hash);
    return p != 0 ? load<u8>(p) : 0u;
}
inline u32 Bit(u32 w, u32 mask, u32 v) { return (w & ~mask) | ((v & 1u) != 0 ? mask : 0u); }

}  // namespace

// bool PropSet_ReadFlags(Owner* o, Flags* out): out->+0 / +1 two bytes
// (default -1 / 0), the flag bits 0x8000..0x0020 of the word at +0 (each
// the low bit of its property, default 0), and in the full set the f32s
// +4 / +8 and the u32 +12 (left alone when absent). The flag word is
// stored again before each further property is looked up, as the
// original does. Returns 0 when o->+12 is null or neither set exists
// (the full set is preferred). The original's "set index >= 0" checks
// before each lookup are always true once a set is found.
DECOMP(004A1440, PropSet_ReadFlags) {
    using namespace b3p;
    const u32 block = load<u32>(arg<u32>(ctx, 0) + 12);
    const u32 out = arg<u32>(ctx, 1);
    if (block == 0) { ret(ctx, 0u); return; }
    s32 k = PropSetIndex(block, kSetFull);
    const bool full = k >= 0;
    if (!full) k = PropSetIndex(block, kSetShort);
    if (k < 0) { ret(ctx, 0u); return; }
    const u32 s = load<u32>(block + 4 + 4 * static_cast<u32>(k));
    store<u8>(out + 0, static_cast<u8>(PropWord(s, kByte0, 0xFFFFFFFFu)));
    store<u8>(out + 1, static_cast<u8>(PropWord(s, kByte1, 0)));
    u32 w = load<u32>(out);
    if (full) {
        // Every flag of the full set is a u8 property.
        w = Bit(w, 0x8000u, PropByte(s, kFlag8000));
        for (const auto& [h, m] : {std::pair{kFlag4000, 0x4000u}, {kFlag2000, 0x2000u}, {kFlag1000, 0x1000u}, {kFlag0800, 0x0800u},
                                   {kFlag0400, 0x0400u}, {kFlag0200, 0x0200u}, {kFlag0100, 0x0100u}, {kFlag0080, 0x0080u},
                                   {kFlag0040, 0x0040u}, {kFlag0020, 0x0020u}}) {
            store<u32>(out, w);
            w = Bit(w, m, PropByte(s, h));
        }
        store<u32>(out, w);
        if (const u32 p = PropFind(s, kF4)) {
            const u32 bits = load<u32>(p);
            ctx.set_fpr_bits(1, LfsBits(bits));  // (lfs f1 / stfs f1: f1 keeps the value)
            store<u32>(out + 4, bits);
        }
        if (const u32 p = PropFind(s, kF8)) {
            const u32 bits = load<u32>(p);
            ctx.set_fpr_bits(1, LfsBits(bits));
            store<u32>(out + 8, bits);
        }
        if (const u32 p = PropFind(s, kW12)) store<u32>(out + 12, load<u32>(p));
    } else {
        // The short set's flags are u32 properties.
        w = Bit(w, 0x8000u, PropWord(s, kFlag8000, 0));
        for (const auto& [h, m] : {std::pair{kFlag0400, 0x0400u}, {kFlag0200, 0x0200u}, {kFlag0040, 0x0040u}}) {
            store<u32>(out, w);
            w = Bit(w, m, PropWord(s, h, 0));
        }
        store<u32>(out, w);
    }
    ret(ctx, 1u);
}
