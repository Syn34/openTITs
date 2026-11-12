// Small container helpers: collecting objects by type, clearing a pointer
// vector, reserving array space, and the shared empty function.
#include "more.h"

namespace sly4::audio {
using namespace opentit::native;
using container::PtrVector;

namespace {

constexpr u32 kObjects = 0x01013B34;  // all objects: +0 Obj** data, +8 u16 count (name?)
constexpr u32 kAllocVfnAlloc = 0, kAllocVfnFree = 8;

struct ObjList {
    gp<gp<TypedObj>> data;  // +0
    pad<4> _4;
    be_u16 count;           // +8
};

}  // namespace

// Appends every object whose type mask has `type`, growing `out` (x2).  (name?)
void Obj_CollectByType(u32 type, PtrVector* out) {
    const u32 bit = 1u << (type & 31);
    const u32 word = (type >> 3) & 0x1FFFFFFCu;
    ObjList& objects = global<ObjList>(kObjects);
    for (u32 i = 0; i < objects.count; ++i) {
        gp<TypedObj>& slot = objects.data.get()[i];
        TypedObj* obj = slot;
        if ((gref<be_u32>(guest(obj) + kObjTypeMask + word) & bit) == 0) continue;
        u32 count = out->count;
        const u32 cap = out->capacity;
        if (count == cap) {
            container::PtrVector_Reserve(out, (cap << 1) & 0x1FFFE);
            count = out->count;
            obj = slot;
        }
        gref<be_u32>(out->data.ea + ((count << 2) & 0x3FFFC)) = guest(obj);
        const u32 n = static_cast<u32>(out->count) + 1;
        const u32 high = out->high_water;
        out->count = static_cast<u16>(n);
        out->high_water = static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : n);
    }
}

// Frees the data through the allocator at +16 (vtable slot 8, with the
// size word at +12 >> 3) and zeroes data, capacity and count. Returns 0.  (name?)
u32 PtrVector_Clear(PtrVector* v) {
    const u32 data = v->data.ea;
    if (data == 0) return 0;
    vcall(v->allocator.get(), kAllocVfnFree, data, v->bits >> 3);
    v->capacity = 0;
    v->count = 0;
    v->data = nullptr;
    return 0;
}

// A bare blr shared by many callers (it was named PtrVector_Free); touches no register.
void Nop() {}

// Grows to max(capacity, old + 16) elements: allocates through vtable slot 0
// (size, 0, align 16), copies the old elements and frees them (slot 8).  (name?)
void Array_Reserve(Array* a, u16 want) {
    const u32 old_cap = a->capacity;
    const u32 esize = (static_cast<u32>(a->flags) >> 1) & 0x7FFF;
    u32 cap = (old_cap + 16) & 0xFFFF;
    const u32 old_bytes = old_cap * esize;
    if (static_cast<s32>(want) > static_cast<s32>(cap)) cap = want;
    const u32 mem = vcall<u32>(a->allocator.get(), kAllocVfnAlloc, cap * esize, 0u, 16u);
    if (const u32 old = a->data) {
        call(mfn::memmove, mem, old, old_bytes);
        vcall(a->allocator.get(), kAllocVfnFree, a->data.get(), old_bytes);
    }
    a->data = mem;
    a->capacity = static_cast<u16>(cap);
}

}  // namespace sly4::audio

NATIVE(000DD258, sly4::audio::Obj_CollectByType)
NATIVE(0054EC1C, sly4::audio::PtrVector_Clear)
NATIVE(0054EC18, sly4::audio::Nop)
NATIVE(00558F3C, sly4::audio::Array_Reserve)
