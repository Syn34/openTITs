// Game code, large-function sweep part 30: adding a KList definition from
// its XML node to the KList definition library (a handle pool plus a
// name-hash map).
#include "big30.h"

namespace sly4::obj {

namespace {
constexpr u32 Knowledge_Load = 0x0038DB04;       // (KList*, XmlNode*)
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (cursor, const char*) -> const char*
constexpr u32 Array_Reserve = 0x00558F3C;        // (array, u32 n)
constexpr u32 String_HashNoCase = 0x0056A710;    // (const char*) -> u32
constexpr u32 HandlePool_Resize = 0x00773E8C;    // (pool, s32 n)
constexpr u32 SortedMap_Find774154 = 0x00774154; // (map, const u32* key) -> value slot
constexpr u32 kName = 0x00ADD4CC;                // "Name"
constexpr u32 kDupFmt = 0x00ADD88C;              // "Fail to add KList (%s) to the KList Def Lib, duplicate KList def found!"
constexpr u32 kSerial = 0x010C29DC;              // the global handle generation

// A handle pool entry: prev, next, value, and +12 bit 31 free (the top bit
// of the u8 +12) and bits 30..15 the generation.
struct HEntry {
    be_u32 prev;   // +0
    be_u32 next;   // +4
    be_u32 value;  // +8
    WordBytes gen; // +12
};
static_assert(sizeof(HEntry) == 16);
// The library: a handle pool at +0 (free list head / tail, used list head
// / tail, used, free, capacity, entries, growable) and at +36 a sorted
// array of {u32 key, KList*} (u16 capacity, count, high-water mark, flags
// bit 0 = unsorted, data +44).
struct KListLib {
    be_u32 free_head;  // +0
    be_u32 free_tail;  // +4
    be_u32 used_head;  // +8
    be_u32 used_tail;  // +12
    be_u32 used;       // +16
    be_u32 nfree;      // +20
    be_u32 cap;        // +24
    be_u32 ents;       // +28: HEntry[]
    u8 growable;       // +32
    pad<3> _33;
    be_u16 map_cap;    // +36
    be_u16 map_count;  // +38
    be_u16 map_mark;   // +40
    be_u16 map_flags;  // +42
    be_u32 map_data;   // +44
};
static_assert(offsetof(KListLib, growable) == 32 && offsetof(KListLib, map_data) == 44);
// The XML node: +12 its attribute cursor.
struct KListXml {
    pad<12> _0;
    be_u32 cursor;  // +12
};
// A loaded KList: +0 points 152 bytes past its definition {+0 key, u16
// +6 flags (bit 7: named)}.
struct KListDef {
    be_u32 key;     // +0
    pad<2> _4;
    be_u16 flags;   // +6
};

HEntry& Ent(const KListLib* p, u32 i) { return gm<HEntry>(p->ents + (i << 4)); }

// The next generation (wrapping 0xFFFF -> 1) stamped into entry i; returns
// the handle i << 16 | generation.
u32 Stamp(KListLib* p, u32 i) {
    be_u32& serial = global<be_u32>(kSerial);
    u32 g = serial + 1;
    serial = g;
    if (g > 0xFFFFu) {
        g = 1;
        serial = g;
    }
    WordBytes& e = Ent(p, i).gen;
    e.w = (e.w & ~0x7FFF8000u) | ((g << 15) & 0x7FFF8000u);
    return ((i & 0xFFFFu) << 16) | (g & 0xFFFFu);
}

// The free head becomes the only used entry.
u32 AllocFirst(KListLib* p) {
    if (p->cap != 1u) {
        const u32 next = Ent(p, p->free_head).next;
        Ent(p, next).prev = 0xFFFFFFFFu;
    }
    const u32 head = p->free_head;
    HEntry& e = Ent(p, head);
    p->used_head = head;
    p->used_tail = head;
    p->free_head = e.next;
    e.next = 0xFFFFFFFFu;
    e.prev = 0xFFFFFFFFu;
    e.gen.b[0] = static_cast<u8>(e.gen.b[0] & 0x7F);
    const u32 used = p->used, nf = p->nfree;
    p->used = used + 1;
    p->nfree = nf - 1;
    return Stamp(p, p->used_head);
}

// The free head goes after the entry of handle hint; 0 when hint is not a
// live entry (generation 0, index out of range, stale or free).
u32 AllocAfter(KListLib* p, u32 hint) {
    const u32 gen = hint & 0xFFFFu;
    if (gen == 0) return 0;
    const u32 h = hint >> 16;
    if (!(static_cast<s32>(h) < static_cast<s32>(p->cap.get()))) return 0;
    const u32 hi = (hint >> 12) & 0x000FFFF0u;
    const HEntry& he = gm<HEntry>(p->ents + hi);
    if (((he.gen.w >> 15) & 0xFFFFu) != gen || (he.gen.b[0] & 0x80) != 0) return 0;
    const u32 head = p->free_head;
    u32 nfree;
    if (p->nfree == 1u) {
        nfree = 0xFFFFFFFFu;
        p->free_tail = nfree;
    } else {
        nfree = Ent(p, head).next;
    }
    p->free_head = nfree;
    HEntry& e = Ent(p, head);
    e.gen.b[0] = static_cast<u8>(e.gen.b[0] & 0x7F);
    Ent(p, head).prev = h;
    const bool tail = h == p->used_tail;
    const u32 used = p->used, nf = p->nfree;  // read before the links, as the original
    const u32 d = p->ents;
    gm<HEntry>(d + (head << 4)).next = tail ? 0xFFFFFFFFu : gm<HEntry>(d + hi).next.get();
    if (!tail) gm<HEntry>(d + (gm<HEntry>(d + hi).next << 4)).prev = head;
    gm<HEntry>(d + hi).next = head;
    if (tail) p->used_tail = head;
    p->used = used + 1;
    p->nfree = nf - 1;
    return Stamp(p, head);
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
// {key (k->+0 - 152)->+0, k} is appended to the map's array (Array_Reserve
// (lib+36, 2 * capacity) when full). Returns k.
u32 KListLib_Add_00390DC8(KListLib* o, KListXml* x, be_u32* out) {
    if (!(static_cast<s32>(o->used.get()) < static_cast<s32>(o->cap.get()))) return 0;
    const u32 cur = x->cursor;
    if (cur != 0) {
        const u32 name = call<u32>(XmlCursor_FindAttrByName, cur, kName);
        const u32 h = call<u32>(String_HashNoCase, name);
        GuestFrame frame(16);  // the original's key at 112(r1)
        be_u32* key = frame.as<be_u32>();
        *key = h;
        const u32 slot = call<u32>(SortedMap_Find774154, guest(o) + 36, key);
        if (gm<be_u32>(slot) != 0u) {
            call(fn::String_Format, kDupFmt, name);
            return 0;
        }
    }
    u32 used = o->used, cap = o->cap;
    const bool full = used == cap;
    u32 hint = 0;
    if (used != 0) {
        const u32 t = o->used_tail;
        hint = ((t & 0xFFFFu) << 16) | ((Ent(o, t).gen.w >> 15) & 0xFFFFu);
    }
    u32 handle;
    bool ok = true;
    if (full) {
        if (o->growable == 0) {
            ok = false;
        } else {
            call(HandlePool_Resize, o, static_cast<s32>(cap << 1));
            cap = o->cap;
            used = o->used;
        }
    }
    if (!ok) handle = 0;
    else if (used == 0) handle = AllocFirst(o);
    else handle = AllocAfter(o, hint);
    *out = handle;
    const u32 k = Ent(o, gm<be_u16>(guest(out))).value;
    call(Knowledge_Load, k, x);
    const u32 d = gm<be_u32>(k) - 152u;
    if (gm<KListDef>(d).flags & 0x80) {
        u32 n = o->map_count;
        if (n == o->map_cap) {
            call(Array_Reserve, guest(o) + 36, (static_cast<u32>(o->map_cap) << 1) & 0xFFFEu);
            n = o->map_count;
        }
        const u32 data = o->map_data;
        gm<be_u32>(data + ((n << 3) & 0x7FFF8u)) = gm<KListDef>(d).key.get();
        gm<be_u32>(data + (static_cast<u32>(o->map_count) << 3) + 4) = k;
        const u32 n1 = o->map_count + 1u;
        const u32 hw = o->map_mark, fl = o->map_flags;
        o->map_count = static_cast<u16>(n1);
        o->map_mark = static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1);
        o->map_flags = static_cast<u16>(fl | 1);
    }
    return k;
}

}  // namespace sly4::obj

NATIVE(00390DC8, sly4::obj::KListLib_Add_00390DC8)
