// Game code, medium sweep part 3: XML element readers.
//  - the level name / id list readers: for an element of the level being
//    loaded, its "name" is copied into the level string pool and its "id"
//    hashed, onto one of three pairs of PtrVectors (the six at 0x010B7868,
//    emptied by World_UnloadLevel47E578);
//  - element constructors / readers: the fields cleared, then read
//    attribute by attribute (XmlCursor_Read* write the value or the
//    default into the field, so each default is the field's value or 0).
#include "mid3.h"

using namespace sly4;
using namespace sly4::mid3;

namespace {

constexpr u32 kGameLevelName = 0x010C2C28;  // const char*: the level being loaded (name?)
constexpr u32 kCurrentWorld = 0x00F65CA0;   // World*
constexpr u32 kWorldLevelHash = 0x128114;   // World +: u32 the current level's hash
constexpr u32 kStrPool = 0x010B7834;        // char* the level string pool (0x8000 bytes)
constexpr u32 kStrPoolCursor = 0x010B7838;  // char* its first free byte
constexpr u32 kStrPoolSize = 0x8000;
constexpr u32 kStrName = 0x00AE916C;        // "name"
constexpr u32 kStrId = 0x00AE9174;          // "id"

// u32 f(XmlCursor* c) for the list pair (names, ids): only when the level
// being loaded is the world's current one and the element has a name.
// The name goes into the pool when it fits (pool cursor + strlen + 1 below
// the pool's end) and onto names when it was copied to a non-null place;
// the id's hash (0 without one) goes onto ids either way. Returns 1.
void ReadNameId(PPUContext& ctx, u32 names, u32 ids) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, m3fn::String_HashNoCase, {load<u32>(kGameLevelName)}));
    if (h == load<u32>(load<u32>(kCurrentWorld) + kWorldLevelHash)) {
        if (const u32 name = static_cast<u32>(call(ctx, m3fn::XmlCursor_FindAttrByName, {c, kStrName})); name != 0) {
            const u32 len = static_cast<u32>(call(ctx, m3fn::String_Length, {name}));
            const u32 dst = load<u32>(kStrPoolCursor);
            if (dst + len + 1 < load<u32>(kStrPool) + kStrPoolSize) {
                call(ctx, m3fn::String_Assign, {dst, name});
                store<u32>(kStrPoolCursor, load<u32>(kStrPoolCursor) + len + 1);
                if (dst != 0) mid1::PtrVecPush(ctx, names, dst);
            }
            u32 id = 0;
            if (const u32 s = static_cast<u32>(call(ctx, m3fn::XmlCursor_FindAttrByName, {c, kStrId})); s != 0)
                id = static_cast<u32>(call(ctx, m3fn::String_HashNoCase, {s}));
            mid1::PtrVecPush(ctx, ids, id);
        }
    }
    ret(ctx, 1u);
}

namespace xfn {
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;   // (c, hash, u32* out, default) -> 0 when found
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;    // (c, hash, s32* out, default)
constexpr u32 XmlCursor_ReadBool = 0x004F986C;   // (c, hash, u8* out, default) -> 0 when found
constexpr u32 XmlCursor_ReadColor = 0x004F9F9C;  // (c, hash, u8[4]* out, const u8[4]* default)
constexpr u32 XmlCursor_FindAttr = 0x004F9534;   // (c, hash) -> const char* or 0
constexpr u32 Obj_InitVt_B21C98 = 0x004BB3B0;    // (o): +4 = 1, the base vtable
}  // namespace xfn

u32 ReadHash(PPUContext& ctx, u32 c, u32 hash, u32 out, u32 def) {
    return static_cast<u32>(call(ctx, xfn::XmlCursor_ReadHash, {c, hash, out, def}));
}
u32 ReadBool(PPUContext& ctx, u32 c, u32 hash, u32 out, u32 def) {
    return static_cast<u32>(call(ctx, xfn::XmlCursor_ReadBool, {c, hash, out, def}));
}
// The hash fields {attribute, offset} of o, each with default 0 or (keep)
// its current value.
void ReadHashes(PPUContext& ctx, u32 c, u32 o, std::initializer_list<std::pair<u32, u32>> f, bool keep) {
    for (const auto& [hash, off] : f) ReadHash(ctx, c, hash, o + off, keep ? load<u32>(o + off) : 0u);
}
// The u8 bool fields {attribute, offset} of o, each defaulting to its value.
void ReadBools(PPUContext& ctx, u32 c, u32 o, std::initializer_list<std::pair<u32, u32>> f) {
    for (const auto& [hash, off] : f) ReadBool(ctx, c, hash, o + off, load<u8>(o + off));
}
// A bool attribute into the bit `mask` of the T at p, through the byte
// tmp: the default is the bit, the bit is then bit 0 of tmp.
template <typename T>
void ReadBit(PPUContext& ctx, u32 c, u32 hash, u32 p, T mask, u32 tmp) {
    ReadBool(ctx, c, hash, tmp, (load<T>(p) & mask) != 0 ? 1u : 0u);
    const T v = load<T>(p);
    store<T>(p, static_cast<T>((v & static_cast<T>(~mask)) | ((load<u8>(tmp) & 1u) != 0 ? mask : 0)));
}
// An s32 field read as a hash first (an enum name) and, when that attribute
// is missing or not a name, as an int (both defaulting to 0).
void ReadEnumOrInt(PPUContext& ctx, u32 c, u32 hash_name, u32 hash_int, u32 out) {
    if (ReadHash(ctx, c, hash_name, out, 0) != 0) call(ctx, xfn::XmlCursor_ReadS32, {c, hash_int, out, 0});
}

}  // namespace

// u32 f(XmlCursor*): a level name / id element into list pair 0.
DECOMP(00483B88, LevelNames_Read0_00483B88) { ReadNameId(ctx, 0x010B7868, 0x010B787C); }
// u32 f(XmlCursor*): a level name / id element into list pair 1.
DECOMP(00483D74, LevelNames_Read1_00483D74) { ReadNameId(ctx, 0x010B7890, 0x010B78A4); }
// u32 f(XmlCursor*): a level name / id element into list pair 2.
DECOMP(00483F60, LevelNames_Read2_00483F60) { ReadNameId(ctx, 0x010B78B8, 0x010B78CC); }

// u32 T::Read(T* o, XmlCursor* c) (vtable 0x00B22380, 52 bytes): +4 = 1,
// +12..+20 = 0, +24 = {255, 255, 255, 255}, +28 / +48 = 0 (u8), +32..+44
// = 0; then +16 / +20 (each a hash, else an int), the bool +28, the colour
// +24 (default 0x01164CA0) and the hashes +32..+44. Returns the last read's.
DECOMP(004C96E4, ReaderB22380_Read_004C96E4) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    store<u16>(o + 4, 1);
    store<u32>(o + 16, 0);
    store<u32>(o + 0, 0x00B22380);
    store<u32>(o + 20, 0);
    store<u32>(o + 12, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 24 + k, 0xFF);
    store<u8>(o + 28, 0);
    for (const u32 off : {32u, 36u, 40u, 44u}) store<u32>(o + off, 0);
    store<u8>(o + 48, 0);
    ReadEnumOrInt(ctx, c, 0x8A93973F, 0x5C8998CD, o + 16);
    ReadEnumOrInt(ctx, c, 0x40795527, 0x0C0E22B5, o + 20);
    ReadBool(ctx, c, 0x0000FC81, o + 28, load<u8>(o + 28));
    call(ctx, xfn::XmlCursor_ReadColor, {c, 0x03D53743, o + 24, mid2::kSlotColorA0});
    ReadHashes(ctx, c, o, {{0x0028162B, 32}, {0x00281620, 36}, {0x0028161B, 40}, {0x0028161A, 44}}, false);
}

// u32 T::Read(T* o, XmlCursor* c) (vtable 0x00B0FD60): the base init
// Obj_InitVt_B21C98(o) (r4 = c, as passed), +12..+48 = 0, then eleven hashes
// +12..+52 with default 0. Returns the last read's.
DECOMP(00080D68, ReaderB0FD60_Read_00080D68) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, xfn::Obj_InitVt_B21C98, {o, c});
    store<u32>(o + 12, 0);
    store<u32>(o + 0, 0x00B0FD60);
    for (u32 off = 16; off <= 48; off += 4) store<u32>(o + off, 0);
    ReadHashes(ctx, c, o, {{0x4D4EF768, 12}, {0xCC50FEFA, 16}, {0x7A30D7E3, 20}, {0xEB9EE198, 24},
                           {0xB2E59948, 28}, {0xE89DF217, 32}, {0x044DC6AB, 36}, {0xAB27550F, 40},
                           {0xAB39AA94, 44}, {0x7FAAD4A0, 48}, {0x03D1FCFC, 52}}, false);
}

// u32 T::Read(T* o, XmlCursor* c) (vtable 0x00B11850): the base init
// Obj_InitVt_B21C98(o) (r4 = c), +12..+28 = 0, the bytes +32..+40 = 0;
// then the hashes +12 (default 0) and +16..+28 and the bools +32..+40
// (defaults: the fields). Returns the last read's.
DECOMP(000A48F0, ReaderB11850_Read_000A48F0) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, xfn::Obj_InitVt_B21C98, {o, c});
    store<u32>(o + 12, 0);
    store<u32>(o + 0, 0x00B11850);
    for (u32 off = 16; off <= 28; off += 4) store<u32>(o + off, 0);
    for (u32 off = 32; off <= 40; ++off) store<u8>(o + off, 0);
    ReadHash(ctx, c, 0x0024728B, o + 12, 0);
    ReadHashes(ctx, c, o, {{0x8C3DFCAA, 16}, {0x00014B07, 20}, {0xE4F7C357, 24}, {0x12E3611B, 28}}, true);
    ReadBools(ctx, c, o, {{0x3D1FC26C, 32}, {0x2CC1B490, 33}, {0x5D4879B3, 34}, {0x0ADE6CD2, 35}, {0xF1559E55, 36},
                          {0x466FBEF5, 37}, {0x9304C8C5, 38}, {0x05D5430F, 39}, {0x971C59D9, 40}});
}

// u32 T::Read(T* o, XmlCursor* c) (vtable 0x00B220B0): +4 = 1, +12..+36 =
// 0, the flags word +40: bits 31..27 cleared, bits 26..25 = 2; then
// +12 / +16 / +20 / +28 (each a hash, else an int), the hash +24, the five
// flag bits (through a stack byte, defaults: the bits) and the 2-bit field
// from the last bool when it was found (default 0). Returns the flags word,
// or that read's nonzero result.
DECOMP(004C1FE4, ReaderB220B0_Read_004C1FE4) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 40);
    store<u16>(o + 4, 1);
    store<u32>(o + 12, 0);
    store<u32>(o + 0, 0x00B220B0);
    for (u32 off = 16; off <= 36; off += 4) store<u32>(o + off, 0);
    store<u32>(o + 40, (w & 0x01FFFFFFu) | 0x04000000u);
    ReadEnumOrInt(ctx, c, 0x000107E5, 0x86E24673, o + 12);
    ReadEnumOrInt(ctx, c, 0x8A93973F, 0x5C8998CD, o + 16);
    ReadEnumOrInt(ctx, c, 0x93116591, 0x39F3EE1F, o + 20);
    ReadHash(ctx, c, 0x00255BF3, o + 24, 0);
    ReadEnumOrInt(ctx, c, 0x3EF76876, 0xABA3F684, o + 28);
    Stack frame(ctx, 16);
    const u32 t = frame.addr();
    ReadBit<u32>(ctx, c, 0x6F82BD88, o + 40, 0x80000000u, t);
    ReadBit<u32>(ctx, c, 0x61311363, o + 40, 0x40000000u, t);
    ReadBit<u32>(ctx, c, 0xE48AEDBD, o + 40, 0x20000000u, t);
    ReadBit<u32>(ctx, c, 0x2A3FC52F, o + 40, 0x10000000u, t);
    ReadBit<u32>(ctx, c, 0x11605B48, o + 40, 0x08000000u, t);
    const u32 r = ReadBool(ctx, c, 0x469D7C52, t, 0);
    if (r != 0) return;
    const u32 v = (load<u32>(o + 40) & ~0x06000000u) | ((static_cast<u32>(load<u8>(t)) << 25) & 0x06000000u);
    store<u32>(o + 40, v);
    ret(ctx, v);
}

// u32 f(?, XmlCursor* c, T* o): the bits 0x20 / 0x10 / 0x08 / 0x02 of the
// u8 +184 (through a stack byte, zeroed first; defaults: the bits), the
// hashes +188 / +192, the bools +196 / +197, the hash +200 (defaults: the
// fields), then the string +204 (String_CopyN 256, +459 = 0) when present.
// Returns 0.
DECOMP(0022083C, Reader_Read_0022083C) {
    const u32 c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 t = frame.addr();
    store<u8>(t, 0);
    ReadBit<u8>(ctx, c, 0xBEF25C9F, o + 184, 0x20, t);
    ReadBit<u8>(ctx, c, 0x1EF4F9F5, o + 184, 0x10, t);
    ReadBit<u8>(ctx, c, 0x2D407057, o + 184, 0x08, t);
    ReadBit<u8>(ctx, c, 0x778A1C92, o + 184, 0x02, t);
    ReadHashes(ctx, c, o, {{0xD5227BA8, 188}, {0x27365255, 192}}, true);
    ReadBools(ctx, c, o, {{0x95098150, 196}, {0x9446A097, 197}});
    ReadHash(ctx, c, 0x45BF4CC0, o + 200, load<u32>(o + 200));
    const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0xF735351B}));
    if (s != 0) {
        call(ctx, m3fn::String_CopyN, {o + 204, s, 256});
        store<u8>(o + 459, 0);
    }
    ret(ctx, 0u);
}
