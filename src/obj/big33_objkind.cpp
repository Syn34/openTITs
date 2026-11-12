// Game code, large-function sweep part 33: an object constructed from its
// descriptor, with a kind-specific component (chosen by the descriptor's
// kind hash) allocated into +24.
#include "big33.h"

namespace sly4::obj {

struct ObjKindDesc {
    pad<20> _0;
    be_u32 key;   // +20: looked up in the object system's map
    be_u32 kind;  // +24: the component kind hash
};
// What an unknown kind asks (vt slot 20): three keys and their values.
struct KindQuery {
    pad<104> _0;
    be_u32 keys[3];    // +104
    pad<4> _116;
    be_u32 values[3];  // +120
};
static_assert(offsetof(KindQuery, values) == 120);
struct KindKeyHolder {
    pad<52> _0;
    be_u32 key;  // +52
};
struct ObjKindOwner {
    pad<216> _0;
    gp<KindKeyHolder> holder;  // +216
    gp<KindQuery> query;       // +220
};
struct ObjKind {
    be_u32 vtable;            // +0
    pad<4> _4;
    gp<ObjKindOwner> owner;   // +8
    pad<12> _12;
    be_u32 comp;              // +24: the component
    be_u32 value;             // +28
};
static_assert(offsetof(ObjKind, comp) == 24);
// The component: the common head and the fields some kinds clear.
struct KindComp {
    be_u32 vtable;        // +0
    pad<56> _4;
    be_u32 f60;           // +60 (kind 0x8CD18EE1)
    pad<152> _64;
    pad<192> member;      // +216: Comp_InitMember
    be_u32 f408, f412;    // +408, +412 (kind 0x685B0BE7)
    pad<5840> _416;
    u8 f6256;             // +6256 (kind 0x0EEBEFAA)
};
static_assert(offsetof(KindComp, member) == 216 && offsetof(KindComp, f408) == 408 && offsetof(KindComp, f6256) == 6256);
struct ObjSystem {
    pad<32> _0;
    pad<4> map;  // +32: a sorted map
};

namespace {
constexpr u32 kVtObjKind = 0x00B0DDC0;   // this object's vtable
constexpr u32 kVtCompBase = 0x00B0DC70;  // component base vtable (set before its member init)
constexpr u32 kObjSystem = 0x00F658D4;   // ObjSystem*
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
// f_00063E24; neither reads the result). Obj_Register(this, d), vtable
// kVtObjKind; value = the value mapped to d->key in the map at the object
// system's +32 (0 when the key is 0). Then by d->kind a component is
// allocated (Mem_Alloc(size, 0, 16)) into comp (0 when the allocation
// fails): base vtable, Comp_InitMember(c + 216), kind-specific zeroed
// fields, final vtable. Kind 0xC83BCA3D instead runs
// Comp_C83BCA3D_Construct. An unknown kind first asks owner->query (vt slot
// 20) and, when that is non-zero, sets value to its entry whose key is
// owner->holder->key (else 0).
void ObjKind_Construct_000179C0(ObjKind* o, const ObjKindDesc* d) {
    GuestFrame frame(16);  // sp+112: the map key
    be_u32* key_slot = frame.as<be_u32>();
    call(Obj_Register, o, d);
    const u32 key = d->key;
    *key_slot = key;
    o->vtable = kVtObjKind;
    ObjSystem& sys = gm<ObjSystem>(global<be_u32>(kObjSystem));  // unchecked
    u32 v = 0;
    if (key != 0) v = gm<be_u32>(call<u32>(SortedMap_Find7332B4, &sys.map, key_slot));
    const u32 kind = d->kind;
    o->value = v;

    if (kind == 0xC83BCA3D) {
        const u32 c = call<u32>(fn::Mem_Alloc, 784u, 0u, 16u);
        if (c != 0) call(Comp_C83BCA3D_Construct, c);
        o->comp = c;
        return;
    }
    const Kind* k = &kDefaultKind;
    for (const Kind& e : kKinds)
        if (e.hash == kind) k = &e;
    if (k == &kDefaultKind) {
        const ObjKindOwner& ow = gm<ObjKindOwner>(o->owner.ea);  // unchecked
        KindQuery* q = ow.query;
        if (q != nullptr && vcall<u32>(q, 20) != 0) {
            const KindKeyHolder* p = ow.holder;
            const u32 want = p != nullptr ? p->key.get() : 0u;
            u32 val = 0;
            if (q->keys[0] == want) val = q->values[0];
            else if (q->keys[1] == want) val = q->values[1];
            else if (q->keys[2] == want) val = q->values[2];
            o->value = val;
        }
    }
    KindComp* c = call<KindComp*>(fn::Mem_Alloc, k->size, 0u, 16u);
    if (c == nullptr) {
        o->comp = 0u;
        return;
    }
    c->vtable = kVtCompBase;
    call(Comp_InitMember, &c->member);
    if (kind == 0x685B0BE7) {
        c->f408 = 0u;
        c->f412 = 0u;
    } else if (kind == 0x8CD18EE1) {
        c->f60 = 0u;
    } else if (kind == 0x0EEBEFAA) {
        c->f6256 = 0;
    }
    c->vtable = k->vt;
    o->comp = guest(c);
}

}  // namespace sly4::obj

NATIVE(000179C0, sly4::obj::ObjKind_Construct_000179C0)
