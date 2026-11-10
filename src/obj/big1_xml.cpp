// Game code, large-function sweep part 1: the damage / targeting
// description readers shared by many object XML loaders ("FromTarget"
// family): a description copied from a template, then overridden by the
// element's attributes.
#include "big1.h"

using namespace sly4;
using namespace sly4::big1;

namespace b1x {
constexpr u32 XmlCursor_DocPath = 0x004F8944;          // (cursor) -> path string (debug)
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;          // (cursor, hash, s32* out, s32 default)
constexpr u32 XmlCursor_ReadF32ByName = 0x004FA5C4;    // (cursor, const char* name, f32* out; f1 default)
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;       // (cursor, tag hash) -> child or 0
constexpr u32 kFmtThreshold = 0x00AC67A0;  // "damageStateThreshold%d"
constexpr u32 kDamageFlagHashes = 0x00AC66CC;  // u32[13]: the bools of the 0xB2892E4A child
constexpr u32 kTagDamageFlags = 0xB2892E4A;
constexpr u32 kHundredF = 0x42C80000;  // 100.0f
}  // namespace b1x

namespace {

// The defaults the damage attributes fall back to (the original builds a
// whole description on the stack; only these fields are read): from the
// parent description when it has flag +20 bit 1, else constants.
struct DamageDefaults {
    u32 health;          // f32: 100.0, or parent +140
    u32 hash1, hash2;    // parent +148 / +152
    u32 threshold[5];    // f32: 0, -1, -2, -3, -4, or parent +156..
    u32 radius;          // f32: parent +176
    u32 hash3, hash4;    // parent +180 / +184
    u32 flags;           // u16: parent +188 (bits 4..15), else 0x8200
};

// The damage description (part of a larger def; offsets as the readers use
// them): +24..+75 copied from the template, +76 f32, +80 / +84 hashes,
// +88 f32[5] thresholds, +108 f32, +112 / +116 hashes, +120 u16 flags
// (bit 15, bits 14..8 bools, bits 4..7 a level), +122 u16 the 13 damage
// type bits.
void CopyTemplate(u32 out, u32 src) {
    for (u32 k = 24; k <= 30; ++k) store<u8>(out + k, load<u8>(src + k));
    store<u32>(out + 36, load<u32>(src + 36));  // lfs / stfs
    store<u8>(out + 31, load<u8>(src + 31));
    store<u32>(out + 32, load<u32>(src + 32));
    store<u32>(out + 40, load<u32>(src + 40));
    store<u8>(out + 44, load<u8>(src + 44));
    store<u32>(out + 48, load<u32>(src + 48));
    store<u32>(out + 64, load<u32>(src + 64));
    store<u32>(out + 52, load<u32>(src + 52));
    store<u32>(out + 60, load<u32>(src + 60));
    store<u32>(out + 56, load<u32>(src + 56));
    store<u32>(out + 72, load<u32>(src + 72));
}

// One bool attribute into bit `bit` of the u16 at p (default: bit `dbit`
// of the defaults' flags).
void ReadFlag(PPUContext& ctx, u32 cur, u32 hash, u32 tb, u32 dflags, u32 dbit, u32 p, u32 bit) {
    call(ctx, b1fn::XmlCursor_ReadBool, {cur, hash, tb, (dflags >> dbit) & 1u});
    const u32 h = load<u16>(p);
    store<u16>(p, static_cast<u16>((h & ~(1u << bit)) | ((load<u8>(tb) & 1u) << bit)));
}

}  // namespace

// u32 Damage_Read0C3910(x, XmlCursor* cur, const Damage* tmpl, const Def*
// parent, Damage* out): out's +24..+75 from tmpl; then the attributes,
// defaulting to the parent's damage fields (when parent +20 has bit 1; a
// parent without it only gets XmlCursor_DocPath(x), a leftover debug call)
// or to constants; the five "damageStateThreshold%d" floats; and the 13
// damage type bits of the 0xB2892E4A child. Returns what the original
// leaves in r3: 0 without that child, else the last flag read's result
// (the bool reader's, or the new +122 value when it was found).
DECOMP(000C3910, Damage_Read0C3910) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1), tmpl = arg<u32>(ctx, 2);
    const u32 parent = arg<u32>(ctx, 3), out = arg<u32>(ctx, 4);
    Stack frame(ctx, 144);
    const u32 tmp = frame.addr();  // the original's sp + 112
    const u32 tb = tmp + 128;      // sp + 240
    const u32 tb2 = tmp + 129;     // sp + 241
    CopyTemplate(out, tmpl);
    DamageDefaults d{b1x::kHundredF, 0, 0, {0, 0xBF800000u, 0xC0000000u, 0xC0400000u, 0xC0800000u}, 0, 0, 0, 0x8200};
    if (parent != 0) {
        if (load<u32>(parent + 20) & 2u) {
            d.health = load<u32>(parent + 140);
            d.hash1 = load<u32>(parent + 148);
            d.hash2 = load<u32>(parent + 152);
            d.radius = load<u32>(parent + 176);
            d.hash3 = load<u32>(parent + 180);
            d.hash4 = load<u32>(parent + 184);
            d.flags = load<u16>(parent + 188) & 0xFFF0u;  // bits 0..3 are never read
            for (u32 k = 0; k < 5; ++k) d.threshold[k] = load<u32>(parent + 156 + 4 * k);
        } else {
            call(ctx, b1x::XmlCursor_DocPath, {x});
        }
    }
    callf(ctx, b1fn::XmlCursor_ReadF32, {cur, 0xA7D74568u, out + 76}, {std::bit_cast<f64>(LfsBits(d.health))});
    call(ctx, b1fn::XmlCursor_ReadHash, {cur, 0xF89FDD94u, out + 80, d.hash1});
    call(ctx, b1fn::XmlCursor_ReadHash, {cur, 0x7973EFA5u, out + 84, d.hash2});
    const u32 fl = out + 120;
    ReadFlag(ctx, cur, 0x2243757Fu, tb, d.flags, 15, fl, 15);
    call(ctx, b1x::XmlCursor_ReadS32, {cur, 0xB981F897u, tmp, (d.flags >> 4) & 0xFu});
    store<u16>(fl, static_cast<u16>((load<u16>(fl) & ~0xF0u) | ((load<u32>(tmp) << 4) & 0xF0u)));
    callf(ctx, b1fn::XmlCursor_ReadF32, {cur, 0xC7348451u, out + 108}, {std::bit_cast<f64>(LfsBits(d.radius))});
    call(ctx, b1fn::XmlCursor_ReadHash, {cur, 0xC9796122u, out + 112, d.hash3});
    call(ctx, b1fn::XmlCursor_ReadHash, {cur, 0xDB88854Eu, out + 116, d.hash4});
    static constexpr u32 kBools[] = {0xEB08E801u, 0xC2F5D6F9u, 0x93CCEEE2u, 0xF95AF325u, 0x9366A555u, 0xFD95C66Au, 0xC3BD9B44u};
    for (u32 k = 0; k < 7; ++k) ReadFlag(ctx, cur, kBools[k], tb, d.flags, 14 - k, fl, 14 - k);
    for (u32 k = 0; k < 5; ++k) {
        const u32 name = static_cast<u32>(call(ctx, b1fn::String_Format, {b1x::kFmtThreshold, k + 1}));
        callf(ctx, b1x::XmlCursor_ReadF32ByName, {cur, name, out + 88 + 4 * k}, {std::bit_cast<f64>(LfsBits(d.threshold[k]))});
    }
    const u32 c = static_cast<u32>(call(ctx, b1x::XmlCursor_FirstChild, {cur, b1x::kTagDamageFlags}));
    u32 r = c;
    if (c != 0) {
        for (u32 k = 0; k < 13; ++k) {
            r = static_cast<u32>(call(ctx, b1fn::XmlCursor_ReadBool, {c, load<u32>(b1x::kDamageFlagHashes + 4 * k), tb2, 0}));
            if (r != 0) continue;
            const u32 bit = 1u << k, h = load<u16>(out + 122);
            r = load<u8>(tb2) != 0 ? (h | bit) : (h & ~bit);
            store<u16>(out + 122, static_cast<u16>(r));
        }
    }
    ret(ctx, r);
}
