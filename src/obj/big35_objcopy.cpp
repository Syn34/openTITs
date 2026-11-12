// Game code, large-function sweep part 35: the constructor of the
// 3280-byte object (vtable 0x00B1E3E0) built by 0x003292C8.
#include "native/objtypes.h"

namespace sly4::obj {

struct Elem12 {
    be_u32 w[3];
};
// A vector of 12-byte elements {+0 data, +4 u16 capacity, +6 u16 element
// size, +8 u16 count, +10 u16 high-water mark}.
struct Vec12 {
    gp<Elem12> data;  // +0
    be_u16 cap;       // +4
    be_u16 step;      // +6
    be_u16 count;     // +8
    be_u16 mark;      // +10
};
struct Elem12List {
    gp<Elem12> data;  // +0
    pad<4> _4;
    be_u16 count;     // +8
};
// The creation record (its layout is copied word for word).
struct ObjB1E3E0Desc {
    be_u32 w[162];    // +0 .. +644
};
struct ObjB1E3E0 {
    be_u32 vtable;    // +0
    pad<2568> _4;
    be_u32 w2572;     // +2572
    be_u32 flags;     // +2576: top two bits cleared
    pad<12> _2580;
    be_u32 cfg[164];  // +2592 .. +3247: copied from the Desc
    Vec12 elems;      // +3248
};
static_assert(offsetof(ObjB1E3E0, cfg) == 2592 && offsetof(ObjB1E3E0, elems) == 3248);

namespace {
constexpr u32 kVtable = 0x00B1E3E0;
constexpr u32 Obj_Construct_00769BD8 = 0x00769BD8;  // (Obj*, const Def*): the base class
constexpr u32 PtrVector_Construct = 0x005516E0;     // (vec)
constexpr u32 PtrVector_Init = 0x00551598;          // (vec, capacity, a, b)
}  // namespace

// void ObjB1E3E0_Construct328D24(Obj* o, const Def* d, const Desc* s,
// const List* list) (from 0x003292C8, on a 3280-byte SmallAlloc block):
// the base Obj_Construct_00769BD8(o, d); +2572 0, +0 the vtable, +2576 its
// top two bits cleared. Then raw word copies from s (the lfs / stfs pairs
// too): s+0..+180 to +2592; three 96-byte records s+192.. to +2784..
// (words +0.. +84 except +12); s+480 / +484 to +3072 / +3076; s+488..+500
// to +3080; s+512..+520 to +3104, +528..+536 to +3120, +544..+552 to
// +3136, +560..+568 to +3152 (with +576 / +580 to +3168 / +3172 and +584
// to +3176 interleaved as in the original); s+592..+640 to +3184. Then the
// vector +3248 (Construct, step 12, Init(16, 0, 1)) gets a copy of each
// 12-byte element of list (+0 data, +8 u16 count).
void ObjB1E3E0_Construct328D24(ObjB1E3E0* o, u32 d, const ObjB1E3E0Desc* s, const Elem12List* list) {
    call(Obj_Construct_00769BD8, o, d);
    const u32 flags = o->flags & 0x3FFFFFFFu;
    o->w2572 = 0u;
    o->vtable = kVtable;
    o->flags = flags;
    // dst / src byte offsets (dst relative to +2592)
    auto copy = [&](u32 dst, u32 src, u32 bytes) {
        for (u32 k = 0; k < bytes; k += 4) o->cfg[(dst - 2592 + k) / 4] = s->w[(src + k) / 4];
    };
    copy(2592, 0, 180);
    for (u32 r = 0; r < 3; ++r) {
        copy(2784 + 96 * r, 192 + 96 * r, 12);
        copy(2784 + 96 * r + 16, 192 + 96 * r + 16, 72);
    }
    copy(3072, 480, 8);
    copy(3080, 488, 16);
    copy(3104, 512, 12);
    copy(3120, 528, 12);
    copy(3136, 544, 12);
    copy(3152, 560, 4);
    copy(3156, 564, 4);
    copy(3168, 576, 4);
    copy(3160, 568, 4);
    copy(3172, 580, 4);
    copy(3176, 584, 4);
    copy(3184, 592, 52);

    Vec12* v = &o->elems;
    call(PtrVector_Construct, v);
    v->step = 12;
    call(PtrVector_Init, v, 16u, 0u, 1u);
    for (u32 i = 0;; ++i) {
        const u32 n = list->count;
        if (i >= n) break;
        u32 cnt = v->count;
        const Elem12* src = list->data.get() + i;
        const u32 cap = v->cap;
        if (cnt == cap) {
            call(fn::PtrVector_Reserve, v, (cap << 1) & 0x1FFFEu);
            cnt = v->count;
        }
        Elem12* dst = v->data.get() + cnt;
        for (u32 k = 0; k < 3; ++k) dst->w[k] = src->w[k].get();
        const u32 c1 = v->count + 1u;
        const u32 hi = v->mark;
        v->count = static_cast<u16>(c1);
        v->mark = static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFF) ? hi : c1);
    }
}

}  // namespace sly4::obj

NATIVE(00328D24, sly4::obj::ObjB1E3E0_Construct328D24)
