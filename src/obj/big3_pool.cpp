// Game code, large-function sweep part 3: the handle-pool object factory
// 002F0E0C (called by Obj_Call2F0E0C_0042F92C): five object kinds, each
// with the pool allocation inlined.
#include "big3.h"

using namespace sly4;
using namespace sly4::big3;

namespace b3h {
constexpr u32 SmallAlloc = 0x0055238C;      // (size) -> p or 0
constexpr u32 kHandleSerial = 0x010C29DC;   // u32: the generation counter (1..0xFFFF)
// The kinds: the desc's type hash, the object size and its constructor
// (obj, desc).
struct Kind {
    u32 type, size, ctor;
};
constexpr Kind kKinds[5] = {
    {0x489EBD81u, 132, 0x002EF938},
    {0xEB2F2138u, 256, 0x002F0058},
    {0x996564D0u, 20, 0x002F0CD8},
    {0x8733FD75u, 156, 0x002ED6F4},
    {0xD0E940EFu, 208, 0x002EC920},
};
}  // namespace b3h

namespace {

// The handle pool: +0 / +4 the free list's head / tail, +8 / +12 the used
// list's head / tail, +16 the used count, +20 the free count, +24 the
// capacity, +28 the entries, +32 u8 growable. An entry (16 bytes): +0
// prev, +4 next, +8 the object, +12 bit 31 free and bits 30..15 the
// generation. A handle is index << 16 | generation.
inline u32 Ent(u32 pool, u32 i) { return load<u32>(pool + 28) + (i << 4); }

// The next generation (the global serial, wrapping 0xFFFF -> 1) stamped
// into entry i; returns the handle.
u32 Stamp(u32 pool, u32 i) {
    u32 g = load<u32>(b3h::kHandleSerial) + 1;
    store<u32>(b3h::kHandleSerial, g);
    if (g > 0xFFFFu) {
        g = 1;
        store<u32>(b3h::kHandleSerial, g);
    }
    const u32 e = Ent(pool, i) + 12;
    store<u32>(e, (load<u32>(e) & ~0x7FFF8000u) | ((g << 15) & 0x7FFF8000u));
    return ((i & 0xFFFFu) << 16) | (g & 0xFFFFu);
}

// Takes the free head into the empty used list.
u32 AllocFirst(u32 pool) {
    if (load<u32>(pool + 24) != 1) {
        const u32 next = load<u32>(Ent(pool, load<u32>(pool + 0)) + 4);
        store<u32>(Ent(pool, next) + 0, 0xFFFFFFFFu);
    }
    const u32 head = load<u32>(pool + 0);
    const u32 data = load<u32>(pool + 28);
    store<u32>(pool + 8, head);
    store<u32>(pool + 12, head);
    const u32 e = data + (head << 4);
    store<u32>(pool + 0, load<u32>(e + 4));
    store<u32>(e + 4, 0xFFFFFFFFu);
    store<u32>(e + 0, 0xFFFFFFFFu);
    store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) & 0x7F));
    store<u32>(pool + 16, load<u32>(pool + 16) + 1);
    store<u32>(pool + 20, load<u32>(pool + 20) - 1);
    return Stamp(pool, load<u32>(pool + 8));
}

// Takes the free head into the used list after the entry of handle
// `hint` (the used tail); 0 when the hint is not a live entry.
u32 AllocAfter(u32 pool, u32 hint) {
    const u32 gen = hint & 0xFFFFu;
    if (gen == 0) return 0;
    const u32 h = hint >> 16;
    if (!(static_cast<s32>(h) < load<s32>(pool + 24))) return 0;
    const u32 he = Ent(pool, h);
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
    const u32 used = load<u32>(pool + 16), nf = load<u32>(pool + 20);  // (read before the links, as the original)
    store<u32>(Ent(pool, head) + 4, tail ? 0xFFFFFFFFu : load<u32>(Ent(pool, h) + 4));
    if (!tail) store<u32>(Ent(pool, load<u32>(Ent(pool, h) + 4)) + 0, head);
    store<u32>(Ent(pool, h) + 4, head);
    if (tail) store<u32>(pool + 12, head);
    store<u32>(pool + 16, used + 1);
    store<u32>(pool + 20, nf - 1);
    return Stamp(pool, head);
}

}  // namespace

// Obj* ObjPool_Create(HandlePool* pool, const Desc* desc, u32* handle): the
// object of desc's kind (its type hash at +0) in a new pool entry: 0 when
// the pool is full (used count >= capacity) or the kind is unknown, with
// *handle untouched. The entry goes after the used tail; *handle gets its
// handle (0 when the tail's handle fails validation, and the object then
// still goes into entry 0, as the original). Then the object is
// allocated, constructed with desc and stored in the entry (0 when the
// allocation fails). The inlined allocator's "grow when full" branch
// cannot run after the full check and is left out.
DECOMP(002F0E0C, ObjPool_Create) {
    const u32 pool = arg<u32>(ctx, 0), desc = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const s32 n = load<s32>(pool + 16);
    if (!(n < load<s32>(pool + 24))) { ret(ctx, 0u); return; }
    const u32 type = load<u32>(desc);
    const b3h::Kind* kind = nullptr;
    for (const auto& k : b3h::kKinds)
        if (k.type == type) kind = &k;
    if (kind == nullptr) { ret(ctx, 0u); return; }
    u32 handle;
    if (n == 0) {
        handle = AllocFirst(pool);
    } else {
        const u32 last = load<u32>(pool + 12);
        const u32 hint = ((last & 0xFFFFu) << 16) | ((load<u32>(Ent(pool, last) + 12) >> 15) & 0xFFFFu);
        handle = AllocAfter(pool, hint);
    }
    store<u32>(out, handle);
    const u32 slot = Ent(pool, load<u16>(out)) + 8;
    u32 obj = static_cast<u32>(call(ctx, b3h::SmallAlloc, {kind->size}));
    if (obj != 0) call(ctx, kind->ctor, {obj, desc});
    store<u32>(slot, obj);
    ret(ctx, obj);
}
