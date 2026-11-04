// Small container helpers: collecting objects by type, clearing a pointer
// vector, reserving array space, and the shared empty function.
#include "more.h"

using namespace sly4;

namespace {

constexpr u32 kObjects = 0x01013B34;  // all objects: +0 Obj** data, +8 u16 count (name?)
constexpr u32 kObjTypeMask = 192;     // Obj: u32[] bit per type it is (name?)

u32 vfunc(u32 obj, u32 offset) { return load<u32>(load<u32>(obj) + offset); }

// Indirect call through an OPD, leaving r6 = the OPD and r7 = its entry as
// the original's call sequence does (in case the target reads them).
u64 call_opd(PPUContext& ctx, u32 opd, std::initializer_list<u64> args) {
    ctx.gpr[6] = opd;
    ctx.gpr[7] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}

}  // namespace

// void Obj_CollectByType(s32 type, PtrVector* out)  (name?)
// PtrVector: +0 data, +4 u16 capacity, +8 u16 count, +10 u16 high water.
// Appends every object whose type mask has `type`, growing `out` (x2).
DECOMP(000DD258, Obj_CollectByType) {
    const u32 type = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 bit = 1u << (type & 31);
    const u32 word = (type >> 3) & 0x1FFFFFFCu;
    for (u32 i = 0; i < load<u16>(kObjects + 8); ++i) {
        const u32 slot = load<u32>(kObjects) + i * 4;
        u32 obj = load<u32>(slot);
        if ((load<u32>(obj + kObjTypeMask + word) & bit) == 0) continue;
        u32 count = load<u16>(out + 8);
        const u32 cap = load<u16>(out + 4);
        if (count == cap) {
            call(ctx, mfn::PtrVector_Reserve, {out, (cap << 1) & 0x1FFFE});
            count = load<u16>(out + 8);
            obj = load<u32>(slot);
        }
        store<u32>(load<u32>(out) + ((count << 2) & 0x3FFFC), obj);
        const u32 n = static_cast<u32>(load<u16>(out + 8)) + 1;
        const u32 high = load<u16>(out + 10);
        store<u16>(out + 8, static_cast<u16>(n));
        store<u16>(out + 10, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : n));
    }
}

// void PtrVector_Clear(PtrVector* v)  (name?)
// Frees the data through the allocator at +16 (vtable slot 8, with the
// size word at +12 >> 3) and zeroes data, capacity and count. Returns 0 in r3.
DECOMP(0054EC1C, PtrVector_Clear) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 data = load<u32>(v);
    if (data == 0) return ret(ctx, 0u);
    const u32 alloc = load<u32>(v + 16);
    call_opd(ctx, vfunc(alloc, 8), {alloc, data, load<u32>(v + 12) >> 3});
    store<u16>(v + 4, 0);
    store<u16>(v + 8, 0);
    store<u32>(v, 0);
    ret(ctx, 0u);
}

// void Nop()
// A bare blr shared by many callers (it was named PtrVector_Free); touches no register.
DECOMP(0054EC18, Nop) { (void)ctx; }

// void Array_Reserve(Array* a, u16 capacity)  (name?)
// Array: +0 u16 capacity, +6 u16 element size << 1, +8 data, +12 allocator.
// Grows to max(capacity, old + 16) elements: allocates through vtable slot 0
// (size, 0, align 16), copies the old elements and frees them (slot 8).
DECOMP(00558F3C, Array_Reserve) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 want = arg<u32>(ctx, 1) & 0xFFFF;
    const u32 old_cap = load<u16>(a);
    const u32 alloc = load<u32>(a + 12);
    const u32 esize = (static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFF;
    u32 cap = (old_cap + 16) & 0xFFFF;
    const u32 alloc_fn = load<u32>(load<u32>(alloc));
    const u32 old_bytes = old_cap * esize;
    if (static_cast<s32>(want) > static_cast<s32>(cap)) cap = want;
    const u32 mem = static_cast<u32>(call_opd(ctx, alloc_fn, {alloc, cap * esize, 0, 16}));
    const u32 old = load<u32>(a + 8);
    if (old != 0) {
        call(ctx, mfn::memmove, {mem, old, old_bytes});
        const u32 al = load<u32>(a + 12);
        call_opd(ctx, vfunc(al, 8), {al, load<u32>(a + 8), old_bytes});
    }
    store<u32>(a + 8, mem);
    store<u16>(a, static_cast<u16>(cap));
}
