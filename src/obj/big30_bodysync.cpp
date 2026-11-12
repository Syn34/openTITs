// Game code, large-function sweep part 30: moving an object and pushing
// the new attachment matrices to the physics bodies of its slots.
#include "../sly4.h"
#include "big30.h"

using namespace sly4;
using namespace sly4::big30;
using big5::Stvx;
using vmx2::LoadMasks3;
using vmx2::Masks3;
using vmx2::RowSelW;

namespace {
constexpr u32 Obj_AttachMatrix = 0x000E3534;  // (Obj*, Mtx* out, Slot*, const Mtx* frame)
constexpr u32 Hk_SetTransform = 0x0091EAB8;   // (body, const hkTransform*)
constexpr u32 Hk_Activate = 0x0091A4C8;       // (body)
constexpr u32 kRowMasks = 0x000E5EC0;         // .rodata: the three row-select masks
}  // namespace

// void f(Obj* o, const Mtx* m) (called by Obj_RefreshSlots_001C7C5C,
// Obj_UpdateSlots_001C9C18, f_001CC440). The x/y/z words of m's four rows
// are copied to o+96 (raw). Then for each 48-byte slot e of o->+388 (u16
// count +8, re-read every pass) with a body b = e->+28 whose +8 is set:
// Obj_AttachMatrix(o, &L, e, m) builds L; when any x/y/z word of L
// differs (bitwise) from b's transform at +240, the rows of L with w
// cleared go to Hk_SetTransform(b, &rows) and Hk_Activate(b). No caller
// reads r3 or other leftovers.
DECOMP(000E5EF0, Obj_SyncBodies_000E5EF0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 r = 0; r < 4; ++r)
        for (u32 j = 0; j < 3; ++j) store<u32>(o + 96 + 16 * r + 4 * j, load<u32>(m + 16 * r + 4 * j));
    const u32 a0 = load<u32>(o + 388);
    if (a0 == 0 || load<u16>(a0 + 8) == 0) return;
    Stack frame(ctx, 224);  // the original's locals at 112..335(r1)
    const u32 L = frame.addr(), B = L + 64, T = L + 128;
    const Masks3 k = LoadMasks3(kRowMasks);
    for (u32 i = 0, off = 0;; ++i, off += 48) {
        const u32 a = load<u32>(o + 388);
        const u32 n = a != 0 ? load<u16>(a + 8) : 0;
        if (i >= n) return;
        const u32 e = load<u32>(a) + off;
        const u32 b = load<u32>(e + 28);
        if (b == 0 || load<u32>(b + 8) == 0) continue;
        call(ctx, Obj_AttachMatrix, {o, L, e, m});
        for (u32 r = 0; r < 4; ++r)
            for (u32 j = 0; j < 3; ++j) store<u32>(B + 16 * r + 4 * j, load<u32>(b + 240 + 16 * r + 4 * j));
        bool diff = false;
        for (u32 r = 0; r < 4; ++r)
            for (u32 j = 0; j < 3; ++j) diff |= load<u32>(B + 16 * r + 4 * j) != load<u32>(L + 16 * r + 4 * j);
        if (!diff) continue;
        for (u32 r = 0; r < 4; ++r) {
            const u32 p = L + 16 * r;
            Stvx(T + 16 * r, RowSelW(p, p + 4, p + 8, k));
        }
        call(ctx, Hk_SetTransform, {b, T});
        call(ctx, Hk_Activate, {b});
    }
}
