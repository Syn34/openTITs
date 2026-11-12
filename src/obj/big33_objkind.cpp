// Game code, large-function sweep part 33: an object constructed from its
// descriptor, with a kind-specific component (chosen by the descriptor's
// kind hash) allocated into +24.
#include "../sly4.h"
#include "big33.h"

using namespace sly4;
using namespace sly4::big33;

namespace {
constexpr u32 kVtObjKind = 0x00B0DDC0;   // this object's vtable
constexpr u32 kVtCompBase = 0x00B0DC70;  // component base vtable (set before its member init)
constexpr u32 kObjSystem = 0x00F658D4;   // u32: the system holding the map (+32) of descriptor +20 keys
constexpr u32 Obj_Register = 0x00250298;    // (this, desc)
constexpr u32 SortedMap_Find7332B4 = 0x007332B4;  // (map, const u32* key) -> u32*
constexpr u32 Comp_InitMember = 0x006279E8;  // (component + 216)
constexpr u32 Comp_C83BCA3D_Construct = 0x000175FC;  // (component): the 784-byte kind's constructor

// One component kind: its hash, Mem_Alloc size and final vtable.
struct Kind {
    u32 hash, size, vt;
};
constexpr Kind kKinds[] = {
    {0xCCFB1F40, 368, 0x00B0E428},  {0x7B70E717, 496, 0x00B0EDA0}, {0x685B0BE7, 416, 0x00B0E028},
    {0x49CC0E00, 384, 0x00B0E2F0},  {0x4908BDCF, 336, 0x00B0E750}, {0x0EEBEFAA, 6288, 0x00B0EAF8},
    {0xBC6EBE7B, 528, 0x00B0EC30},  {0xA1519173, 2336, 0x00B0E9C0}, {0x8CD18EE1, 432, 0x00B0E1B8},
};
constexpr Kind kDefaultKind = {0, 480, 0x00B0E5B8};
}  // namespace

// T* T::T(T* this, const Desc* d) (called by Thunk_00017F64 and
// f_00063E24). Obj_Register(this, d), vtable kVtObjKind; +28 = the value
// mapped to d+20 in the map at kObjSystem->+32 (0 when d+20 is 0). Then by
// the kind hash d+24 a component is allocated (Mem_Alloc(size, 0, 16)) into
// +24 (0 when the allocation fails): base vtable, Comp_InitMember(c + 216),
// kind-specific zeroed fields, final vtable (r3 is left as that vtable, or 0).
// Kind 0xC83BCA3D instead runs Comp_C83BCA3D_Construct (r3 left as its
// result). An unknown kind first asks this+8 ->+220 (vt slot 20) and, when
// that is non-zero, sets +28 to its +120 / +124 / +128 entry whose key
// (+104 / +108 / +112) is this+8 ->+216 ->+52 (else 0).
DECOMP(000179C0, ObjKind_Construct_000179C0) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);  // sp+112: the map key
    const u32 sp = frame.addr();
    call(ctx, Obj_Register, {o, d});
    const u32 key = load<u32>(d + 20);
    store<u32>(sp, key);
    store<u32>(o, kVtObjKind);
    const u32 sys = load<u32>(kObjSystem);
    u32 v = 0;
    if (key != 0) v = load<u32>(static_cast<u32>(call(ctx, SortedMap_Find7332B4, {sys + 32, sp})));
    const u32 kind = load<u32>(d + 24);
    store<u32>(o + 28, v);

    if (kind == 0xC83BCA3D) {
        const u32 c = static_cast<u32>(call(ctx, fn::Mem_Alloc, {784u, 0u, 16u}));
        if (c != 0) call(ctx, Comp_C83BCA3D_Construct, {c});
        store<u32>(o + 24, c);
        return;  // r3: 0 or the constructor's result
    }
    const Kind* k = &kDefaultKind;
    for (const Kind& e : kKinds)
        if (e.hash == kind) k = &e;
    if (k == &kDefaultKind) {
        const u32 q = load<u32>(load<u32>(o + 8) + 220);
        if (q != 0 && static_cast<u32>(VCall(ctx, q, 20, {q})) != 0) {
            const u32 p = load<u32>(load<u32>(o + 8) + 216);
            const u32 want = p != 0 ? load<u32>(p + 52) : 0;
            u32 val = 0;
            if (load<u32>(q + 104) == want) val = load<u32>(q + 120);
            else if (load<u32>(q + 108) == want) val = load<u32>(q + 124);
            else if (load<u32>(q + 112) == want) val = load<u32>(q + 128);
            store<u32>(o + 28, val);
        }
    }
    const u32 c = static_cast<u32>(call(ctx, fn::Mem_Alloc, {k->size, 0u, 16u}));
    if (c == 0) {
        store<u32>(o + 24, 0);
        return ret(ctx, 0u);
    }
    store<u32>(c, kVtCompBase);
    call(ctx, Comp_InitMember, {c + 216});
    if (kind == 0x685B0BE7) {
        store<u32>(c + 408, 0);
        store<u32>(c + 412, 0);
    } else if (kind == 0x8CD18EE1) {
        store<u32>(c + 60, 0);
    } else if (kind == 0x0EEBEFAA) {
        store<u8>(c + 6256, 0);
    }
    store<u32>(c, k->vt);
    store<u32>(o + 24, c);
    ret(ctx, k->vt);
}
