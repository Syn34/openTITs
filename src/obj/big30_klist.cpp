// Game code, large-function sweep part 30: adding a KList definition from
// its XML node to the KList definition library (a handle pool plus a
// name-hash map).
#include "../sly4.h"
#include "big30.h"

using namespace sly4;
using namespace sly4::big30;

namespace {
constexpr u32 Knowledge_Load = 0x0038DB04;       // (KList*, XmlNode*)
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (cursor, const char*) -> const char*
constexpr u32 String_Format = 0x005534D0;        // (fmt, ...) (debug message)
constexpr u32 Array_Reserve = 0x00558F3C;        // (array, u32 n)
constexpr u32 String_HashNoCase = 0x0056A710;    // (const char*) -> u32
constexpr u32 HandlePool_Resize = 0x00773E8C;    // (pool, s32 n)
constexpr u32 SortedMap_Find774154 = 0x00774154; // (map, const u32* key) -> value slot
constexpr u32 kName = 0x00ADD4CC;                // "Name"
constexpr u32 kDupFmt = 0x00ADD88C;              // "Fail to add KList (%s) to the KList Def Lib, duplicate KList def found!"
constexpr u32 kSerial = 0x010C29DC;              // the global handle generation

// The handle pool at the library's +0: +0 / +4 the free list's head /
// tail, +8 / +12 the used list's head / tail, +16 used, +20 free, +24
// capacity, +28 the 16-byte entries {+0 prev, +4 next, +8 value, +12 bit
// 31 free and bits 30..15 the generation}, +32 u8 growable.
inline u32 Ent(u32 pool, u32 i) { return load<u32>(pool + 28) + (i << 4); }

// The next generation (wrapping 0xFFFF -> 1) stamped into entry i; returns
// the handle i << 16 | generation.
u32 Stamp(u32 pool, u32 i) {
    u32 g = load<u32>(kSerial) + 1;
    store<u32>(kSerial, g);
    if (g > 0xFFFFu) {
        g = 1;
        store<u32>(kSerial, g);
    }
    const u32 e = Ent(pool, i) + 12;
    store<u32>(e, (load<u32>(e) & ~0x7FFF8000u) | ((g << 15) & 0x7FFF8000u));
    return ((i & 0xFFFFu) << 16) | (g & 0xFFFFu);
}

// The free head becomes the only used entry.
u32 AllocFirst(u32 pool) {
    if (load<u32>(pool + 24) != 1) {
        const u32 next = load<u32>(Ent(pool, load<u32>(pool + 0)) + 4);
        store<u32>(Ent(pool, next) + 0, 0xFFFFFFFFu);
    }
    const u32 head = load<u32>(pool + 0);
    const u32 e = load<u32>(pool + 28) + (head << 4);
    store<u32>(pool + 8, head);
    store<u32>(pool + 12, head);
    store<u32>(pool + 0, load<u32>(e + 4));
    store<u32>(e + 4, 0xFFFFFFFFu);
    store<u32>(e + 0, 0xFFFFFFFFu);
    store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) & 0x7F));
    const u32 used = load<u32>(pool + 16), nf = load<u32>(pool + 20);
    store<u32>(pool + 16, used + 1);
    store<u32>(pool + 20, nf - 1);
    return Stamp(pool, load<u32>(pool + 8));
}

// The free head goes after the entry of handle hint; 0 when hint is not a
// live entry (generation 0, index out of range, stale or free).
u32 AllocAfter(u32 pool, u32 hint) {
    const u32 gen = hint & 0xFFFFu;
    if (gen == 0) return 0;
    const u32 h = hint >> 16;
    if (!(static_cast<s32>(h) < static_cast<s32>(load<u32>(pool + 24)))) return 0;
    const u32 he = load<u32>(pool + 28) + ((hint >> 12) & 0x000FFFF0u);
    if (((load<u32>(he + 12) >> 15) & 0xFFFFu) != gen || (load<u8>(he + 12) & 0x80) != 0) return 0;
    const u32 head = load<u32>(pool + 0);
    u32 nfree;
    if (load<u32>(pool + 20) == 1) {
        nfree = 0xFFFFFFFFu;
        store<u32>(pool + 4, nfree);
    } else {
        nfree = load<u32>(Ent(pool, head) + 4);
    }
    store<u32>(pool + 0, nfree);
    const u32 e = Ent(pool, head);
    store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) & 0x7F));
    store<u32>(Ent(pool, head) + 0, h);
    const bool tail = h == load<u32>(pool + 12);
    const u32 used = load<u32>(pool + 16), nf = load<u32>(pool + 20);  // read before the links, as the original
    const u32 d = load<u32>(pool + 28), hi = (hint >> 12) & 0x000FFFF0u;
    store<u32>(d + (head << 4) + 4, tail ? 0xFFFFFFFFu : load<u32>(d + hi + 4));
    if (!tail) store<u32>(d + (load<u32>(d + hi + 4) << 4), head);
    store<u32>(d + hi + 4, head);
    if (tail) store<u32>(pool + 12, head);
    store<u32>(pool + 16, used + 1);
    store<u32>(pool + 20, nf - 1);
    return Stamp(pool, head);
}
}  // namespace

// KList* f(KListLib* lib, XmlNode* x, u32* handle) (called by f_000650B0,
// Knowledge_Clone_00065290, World_LoadZipXml). 0 when the pool is full
// (used >= capacity, signed). With a cursor x->+12, the "Name" attribute's
// no-case hash is looked up in the map at lib+36; a hit prints the
// duplicate message and returns 0. A pool entry is taken (the first used
// one, or after the used tail; the handle is 0 when the tail fails
// validation) and *handle gets its handle. The "pool full" branch (grow
// by HandlePool_Resize(lib, 2 * capacity) when +32 is set, else handle 0)
// cannot run after the entry check unless a callee changed the counts; it
// is kept. The entry's value k (entry 0 for a 0 handle, as the original)
// gets Knowledge_Load(k, x); when the u16 +6 of (k->+0 - 152) has bit 7,
// {key (k->+0 - 152)->+0, k} is appended to the map's array (u16 capacity
// +36, count +38, high-water +40, flags +42 |= 1 = unsorted, data +44;
// Array_Reserve(lib+36, 2 * capacity) when full). Returns k.
DECOMP(00390DC8, KListLib_Add_00390DC8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    if (!(static_cast<s32>(load<u32>(o + 16)) < static_cast<s32>(load<u32>(o + 24)))) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 cur = load<u32>(x + 12);
    if (cur != 0) {
        const u32 name = static_cast<u32>(call(ctx, XmlCursor_FindAttrByName, {cur, kName}));
        const u32 h = static_cast<u32>(call(ctx, String_HashNoCase, {name}));
        Stack frame(ctx, 16);  // the original's key at 112(r1)
        store<u32>(frame.addr(), h);
        const u32 slot = static_cast<u32>(call(ctx, SortedMap_Find774154, {o + 36, frame.addr()}));
        if (load<u32>(slot) != 0) {
            call(ctx, String_Format, {kDupFmt, name});
            ctx.gpr[3] = 0;
            return;
        }
    }
    u32 used = load<u32>(o + 16), cap = load<u32>(o + 24);
    const bool full = used == cap;
    u32 hint = 0;
    if (used != 0) {
        const u32 t = load<u32>(o + 12);
        hint = ((t & 0xFFFFu) << 16) | ((load<u32>(load<u32>(o + 28) + (t << 4) + 12) >> 15) & 0xFFFFu);
    }
    u32 handle;
    bool ok = true;
    if (full) {
        if (load<u8>(o + 32) == 0) {
            ok = false;
        } else {
            call(ctx, HandlePool_Resize, {o, static_cast<u64>(static_cast<s64>(static_cast<s32>(cap << 1)))});
            cap = load<u32>(o + 24);
            used = load<u32>(o + 16);
        }
    }
    if (!ok) handle = 0;
    else if (used == 0) handle = AllocFirst(o);
    else handle = AllocAfter(o, hint);
    store<u32>(out, handle);
    const u32 k = load<u32>(load<u32>(o + 28) + (static_cast<u32>(load<u16>(out)) << 4) + 8);
    call(ctx, Knowledge_Load, {k, x});
    const u32 d = load<u32>(k) - 152;
    if (load<u16>(d + 6) & 0x80) {
        u32 n = load<u16>(o + 38);
        if (n == load<u16>(o + 36)) {
            call(ctx, Array_Reserve, {o + 36, (static_cast<u32>(load<u16>(o + 36)) << 1) & 0xFFFEu});
            n = load<u16>(o + 38);
        }
        const u32 data = load<u32>(o + 44);
        store<u32>(data + ((n << 3) & 0x7FFF8u), load<u32>(d));
        store<u32>(data + (static_cast<u32>(load<u16>(o + 38)) << 3) + 4, k);
        const u32 n1 = load<u16>(o + 38) + 1;
        const u32 hw = load<u16>(o + 40), fl = load<u16>(o + 42);
        store<u16>(o + 38, static_cast<u16>(n1));
        store<u16>(o + 40, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
        store<u16>(o + 42, static_cast<u16>(fl | 1));
    }
    ctx.gpr[3] = k;
}
