// Game code, large-function sweep part 30: moving an object and pushing
// the new attachment matrices to the physics bodies of its slots.
#include "native/vec.h"

namespace sly4::obj {

struct Mtx4x4 {
    Vec4 r[4];  // rows 0..2, translation
};
struct SyncBody {
    pad<8> _0;
    be_u32 w8;         // +8
    pad<228> _12;
    Mtx4x4 xf;         // +240: its current transform
};
static_assert(offsetof(SyncBody, xf) == 240);
// A 48-byte slot: +28 its body.
struct SyncSlot {
    pad<28> _0;
    gp<SyncBody> body;  // +28
    pad<16> _32;
};
static_assert(sizeof(SyncSlot) == 48);
struct SyncSlots {
    be_u32 data;   // +0: SyncSlot*
    pad<4> _4;
    be_u16 count;  // +8
};
struct SyncObj {
    pad<96> _0;
    Mtx4x4 m;                // +96
    pad<228> _160;
    gp<SyncSlots> slots;     // +388
};
static_assert(offsetof(SyncObj, slots) == 388);

namespace {
constexpr u32 Obj_AttachMatrix = 0x000E3534;  // (Obj*, Mtx* out, Slot*, const Mtx* frame)
constexpr u32 Hk_SetTransform = 0x0091EAB8;   // (body, const hkTransform*)
constexpr u32 Hk_Activate = 0x0091A4C8;       // (body)
constexpr u32 kRowMasks = 0x000E5EC0;         // .rodata: the three row-select masks

// The x / y / z words of the four rows (raw).
void Copy4x3(Mtx4x4& d, const Mtx4x4& s) {
    for (u32 r = 0; r < 4; ++r) {
        set_bits(d.r[r].x, bits(s.r[r].x));
        set_bits(d.r[r].y, bits(s.r[r].y));
        set_bits(d.r[r].z, bits(s.r[r].z));
    }
}
}  // namespace

// void f(Obj* o, const Mtx* m) (called by Obj_RefreshSlots_001C7C5C,
// Obj_UpdateSlots_001C9C18, f_001CC440; none reads r3). The x / y / z
// words of m's four rows are copied to o->m (raw). Then for each slot e of
// o->slots (count re-read every pass) with a body b whose w8 is set:
// Obj_AttachMatrix(o, &L, e, m) builds L; when any x / y / z word of L
// differs (bitwise) from b's transform, the rows of L with w cleared go to
// Hk_SetTransform(b, &rows) and Hk_Activate(b).
void Obj_SyncBodies_000E5EF0(SyncObj* o, const Mtx4x4* m) {
    Copy4x3(o->m, *m);
    const SyncSlots* a0 = o->slots;
    if (a0 == nullptr || a0->count == 0) return;
    GuestFrame frame(224);  // the original's locals at 112..335(r1)
    Mtx4x4* L = frame.as<Mtx4x4>(0);
    Mtx4x4* B = frame.as<Mtx4x4>(64);
    Mtx4x4* T = frame.as<Mtx4x4>(128);
    const Masks3 k = LoadMasks3(kRowMasks);
    for (u32 i = 0, off = 0;; ++i, off += 48) {
        const SyncSlots* a = o->slots;
        const u32 n = a != nullptr ? a->count : 0u;
        if (i >= n) return;
        SyncSlot& e = gm<SyncSlot>(a->data + off);
        SyncBody* b = e.body;
        if (b == nullptr || b->w8 == 0) continue;
        call(Obj_AttachMatrix, o, L, &e, m);
        Copy4x3(*B, b->xf);
        bool diff = false;
        for (u32 r = 0; r < 4; ++r)
            diff |= bits(B->r[r].x) != bits(L->r[r].x) || bits(B->r[r].y) != bits(L->r[r].y) || bits(B->r[r].z) != bits(L->r[r].z);
        if (!diff) continue;
        for (u32 r = 0; r < 4; ++r) Stvx(&T->r[r], RowSelW(&L->r[r].x, &L->r[r].y, &L->r[r].z, k));
        call(Hk_SetTransform, b, T);
        call(Hk_Activate, b);
    }
}

}  // namespace sly4::obj

NATIVE(000E5EF0, sly4::obj::Obj_SyncBodies_000E5EF0)
