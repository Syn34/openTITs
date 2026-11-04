// World_HandleEvent callees, part 14a: small leaves under the hevent13
// functions (profile item, context init, save flags, entry spawn index,
// effect args, empty XML document, container init).
#include "hevent14.h"

using namespace sly4;
using namespace sly4::hevent14;

namespace {
// Profile +1420: u32[4] item hashes.
constexpr u32 kProfileItems = 1420, kProfileItemCount = 4;

// Ctx (as Session_UpdateEntries2E4040): +0 kind (6), +4 f32 time, +8/+12/+16
// 0, +20 Object*, +24 f32 dt. World +12: the clock (+0 f32 time).
constexpr u32 kCtxKindPlayer = 6;
constexpr u32 kWorldClock = 12;

// SaveSlot +348 flags: bit 15 and bit 16 (IBM bits 16 / 15).
constexpr u32 kSaveFlags = 348, kSaveBit15 = 0x8000, kSaveBit16 = 0x10000;

// Event argument vector (as hevent8.h): +0 76-byte records, +8 u16 count.
constexpr u32 kArgVecData = 0, kArgVecCount = 8;

// XmlDoc: +0 vtable, 64 32-byte nodes at +192 (vtable 0x00B23090), +176
// four words, +32 packed flags.
constexpr u32 kXmlDocVtable = 0x00B230D0, kXmlNodeVtable = 0x00B23090;
constexpr u32 kXmlNodes = 192, kXmlNodeSize = 32, kXmlNodeCount = 64;
constexpr u8 kXmlNodeFlag24 = 0x80;  // byte +24 bit 7 (cleared)

// Container (r3): +0 / +4 block, +8 index table (with an index), +12 data.
// Block header (16 bytes): +0 u16 capacity, +2 u16 0, +4 u32 size (0 then
// the allocation size), +8 u32 indexed, +12 Allocator*. Allocator vt +0:
// alloc(Allocator*, size, 0, 16).
constexpr u32 kAllocVtAlloc = 0, kAllocAlign = 16, kBlockHeader = 16;
}  // namespace

// u32 Profile_Item121B40(Profile* p, u32 k) (name?): p +1420 [k] for k < 4,
// else 0.
DECOMP(00121B40, Profile_Item121B40) {
    const u32 p = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    ret(ctx, k < kProfileItemCount ? load<u32>(p + kProfileItems + k * 4) : 0u);
}

// void Ctx_Init4651A0(Ctx* c, Object* player; f32 dt) (name?): an update
// context of kind 6 at the current world time.
DECOMP(004651A0, Ctx_Init4651A0) {
    const u32 c = arg<u32>(ctx, 0), player = arg<u32>(ctx, 1);
    store<u32>(c + 20, player);
    store<u32>(c + 0, kCtxKindPlayer);
    const u32 clock = load<u32>(load<u32>(hevent::kCurrentWorld) + kWorldClock);
    const u32 t = load<u32>(clock);  // lfs / stfs pair
    store<f32>(c + 24, static_cast<f32>(farg(ctx, 0)));
    store<u32>(c + 4, t);
    store<u32>(c + 8, 0);
    store<u32>(c + 12, 0);
    store<u32>(c + 16, 0);
}

// u8 Save_Mark0AE418(SaveSlot* s, u8 out[2]) (name?): out[0] = save +348 bit
// 15, out[1] = bit 16. Returns out[1].
DECOMP(000AE418, Save_Mark0AE418) {
    const u32 s = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 f = load<u32>(s + kSaveFlags);
    store<u8>(out + 0, (f & kSaveBit15) != 0 ? 1 : 0);
    const u8 b = (f & kSaveBit16) != 0 ? 1 : 0;
    store<u8>(out + 1, b);
    ret(ctx, static_cast<u32>(b));
}

// s32 Entry_Index325E70(Entry* e, u32 spawn) (name?): the user index whose
// spawn hash (e +2416 [9]) is spawn, or -1 (also for spawn 0).
DECOMP(00325E70, Entry_Index325E70) {
    const u32 e = arg<u32>(ctx, 0), spawn = arg<u32>(ctx, 1);
    s32 r = -1;
    if (spawn != 0) {
        for (s32 i = 0; i < static_cast<s32>(kNumSpawns); ++i) {
            if (load<u32>(e + kEntrySpawns + static_cast<u32>(i) * 4) == spawn) {
                r = i;
                break;
            }
        }
    }
    ret(ctx, r);
}

// s32 Entry_Index325EBC(Entry* e, Object* o) (name?): the user index whose
// spawn hash matches o's owner point (owner +52, 0 without an owner), or
// for the "any point" hash 0x14120: o +188 bit 5, or o +192 bit 3 with the
// owner's +192 hash being 0x14120. -1 when none (or o is 0).
DECOMP(00325EBC, Entry_Index325EBC) {
    const u32 e = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    s32 r = -1;
    if (o != 0) {
        const bool any188 = (load<u32>(o + kObjFlags188) & kObjFlag188Bit5) != 0;
        const u32 owner = load<u32>(o + kObjOwner);
        const bool alt192 = (load<u32>(o + kObjFlags192) & kObjFlag192Bit3) != 0;
        for (s32 i = 0; i < static_cast<s32>(kNumSpawns); ++i) {
            const u32 s = load<u32>(e + kEntrySpawns + static_cast<u32>(i) * 4);
            const u32 h = owner != 0 ? load<u32>(owner + kOwnerHash) : 0;
            if (s == h) { r = i; break; }
            if (s != kAnyPoint) continue;
            if (any188) { r = i; break; }
            if (!alt192) continue;
            // Quirk: without an owner this reads guest address 192.
            if (load<u32>(owner + kOwnerAltHash) == s) { r = i; break; }
        }
    }
    ret(ctx, r);
}

// u32 Effect_AddArgs49FCC0(Effect* fx, ArgVector* v, e, u8 a, u8 b) (name?):
// adds every argument record of v to fx through 0x0049F080. Returns the
// count last read (0 for no effect).
DECOMP(0049FCC0, Effect_AddArgs49FCC0) {
    const u32 fx = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1), e = arg<u32>(ctx, 2);
    const u8 a = arg<u8>(ctx, 3), b = arg<u8>(ctx, 4);
    if (fx == 0) return ret(ctx, 0u);
    u32 n = load<u16>(v + kArgVecCount);
    for (u32 i = 0, off = 0; i < n; ++i, off += hevent8::kArgSize) {
        call(ctx, h14fn::Effect_AddArg49F080, {fx, load<u32>(v + kArgVecData) + off, e, a, b});
        n = load<u16>(v + kArgVecCount);
    }
    ret(ctx, n);
}

// void XmlDoc_Init4FA7A4(XmlDoc* d) (name?): an empty document: header, 64
// free nodes, flags and the root fields.
DECOMP(004FA7A4, XmlDoc_Init4FA7A4) {
    const u32 d = arg<u32>(ctx, 0);
    const u8 f24 = static_cast<u8>(load<u8>(d + 24) & ~kXmlNodeFlag24);
    store<u32>(d + 12, 0xFFFFFFFFu);
    store<u32>(d + 8, 0xFFFFFFFFu);
    store<u32>(d + 16, 0);
    store<u32>(d + 4, 0);
    store<u32>(d + 20, 0);
    store<u8>(d + 24, f24);
    store<u32>(d + 28, 0);
    store<u8>(d + 25, 0);
    store<u16>(d + 26, 0);
    store<u32>(d + 0, kXmlDocVtable);
    store<u32>(d + 40, 0xFFFFFFFFu);
    store<u32>(d + 36, 0xFFFFFFFFu);
    for (u32 k = 0; k < kXmlNodeCount; ++k) {
        const u32 n = d + kXmlNodes + k * kXmlNodeSize;
        const u8 g24 = static_cast<u8>(load<u8>(n + 24) & ~kXmlNodeFlag24);
        store<u32>(n + 12, 0xFFFFFFFFu);
        store<u32>(n + 8, 0xFFFFFFFFu);
        store<u32>(n + 16, 0);
        store<u32>(n + 4, 1);
        store<u32>(n + 20, 0);
        store<u8>(n + 24, g24);
        store<u32>(n + 28, 0);
        store<u8>(n + 25, 0);
        store<u16>(n + 26, 0);
        store<u32>(n + 0, kXmlNodeVtable);
    }
    const u32 f32w = load<u32>(d + 32);
    store<u32>(d + 44, 0);
    store<u32>(d + 40, 0);
    store<u32>(d + 36, 0);
    // rlwimi 4,14,10,17 then clear the top ten bits: bits 14..21 = 4, bits 22..31 = 0.
    store<u32>(d + 32, ((f32w & ~0x003FC000u) | 0x00010000u) & 0x003FFFFFu);
    for (u32 o : {176u, 180u, 184u, 188u}) store<u32>(d + o, 0);
    store<u8>(d + 48, 0);
}

// u32 Container_Init76EBA0(Container* c, u32 n, u32 indexed, Allocator* a)
// (name?): allocates one block for n 8-byte items (plus n u32 indices when
// indexed) after a 16-byte header. Nothing for n = 0. Returns c +12.
DECOMP(0076EBA0, Container_Init76EBA0) {
    const u32 c = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), indexed = arg<u32>(ctx, 2), al = arg<u32>(ctx, 3);
    if (n == 0) return;  // r3 stays c
    const u32 size = (n * 8 + (indexed != 0 ? n * 4 : 0) + 31) & ~15u;
    const u32 p = static_cast<u32>(hevent::vcall(ctx, al, kAllocVtAlloc, {al, size, 0, kAllocAlign}));
    store<u32>(c + 0, p);
    store<u32>(c + 4, p);
    if (p != 0) {
        store<u16>(p + 2, 0);
        store<u32>(p + 4, 0);
        store<u32>(p + 8, 0);
    }
    store<u16>(p + 0, static_cast<u16>(n));  // quirk: also when the allocation failed
    const u32 h = load<u32>(c + 4);
    store<u32>(h + 8, indexed);
    store<u32>(h + 4, size);
    store<u32>(h + 12, al);
    const u32 base = load<u32>(c + 0);
    const u32 idx = indexed != 0 ? p + kBlockHeader : 0;
    store<u32>(c + 8, idx);
    const u32 data = idx != 0 ? base + load<u16>(h + 0) * 4 + kBlockHeader : base + kBlockHeader;
    store<u32>(c + 12, data);
    ret(ctx, data);
}
