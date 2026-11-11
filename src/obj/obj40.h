// Game code, part 40: helpers shared by obj40*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the 83
// instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj39.h"

namespace sly4 {
namespace obj40 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj39;

// Single precision rounding of a double (the s-forms).
inline f64 F40(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }

// Sign extension of a 32-bit value to the 64-bit register (extsw).
inline u64 Sx40(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }

// The f32 at ea widened to the register (lfs).
inline f64 Lfs40(u32 ea) { return static_cast<f64>(load<f32>(ea)); }

// The deleting destructor of 0x002808B4 / 0x0028F3E4 (o = r3): r3 = 0 for
// o == 0; else vtable vt, World_ListRemove_0046B7A4(*0x00F65CA0, kind,
// list, o, o->+8 (read before the vtable store)), vtable 0x00B1BE30,
// Unlist1056F1C39(o), List1013B5C_Remove(o) and SmallFree(o, 272) (r3 =
// its).
inline void DeleteWorld272_40(PPUContext& ctx, u32 vt, u32 kind, u32 list) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = load<u32>(o + 8);
    store<u32>(o, vt);
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), kind, list, o, w});
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C39(o);
    call(ctx, 0x000D6940u, {o});
    call(ctx, 0x00552420u, {o, 272});
}

// The destructor of 0x0048F3F8 / 0x0048F6B0 (o = r3, two identical
// copies): vtable 0x00B21A30; when the u8 o->+4 is set, o is unlinked from
// the node chains of the registry 0x010BB738 (u16 count +2, 8-byte {key,
// holder} entries +8; each holder points at a chain of {obj, next} nodes;
// matched nodes get obj = 0); a holder left empty is dropped
// (SortedMap_Remove786A18(reg, &key), Mem_FreeFrom(holder, 0)) and the
// same index looked at again; then u8 o->+4 = 0. r3 as the original
// leaves it (o, or the final count).
inline void RegistryUnlink40(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B21A30u);
    if (load<u8>(o + 4) == 0) return;
    constexpr u32 R = 0x010BB738u;
    for (s32 i = 0; i < static_cast<s32>(load<u16>(R + 2)); ++i) {
        const u32 d = load<u32>(R + 8);
        const u32 hold = load<u32>(d + 8 * static_cast<u32>(i) + 4);
        u32 head = load<u32>(hold);
        if (load<u32>(head) == o) {
            const u32 nx = load<u32>(head + 4);
            store<u32>(head, 0);
            head = nx;
            store<u32>(hold, nx);
        } else {
            u32 prev = head;
            for (u32 q = load<u32>(head + 4); q != 0; prev = q, q = load<u32>(prev + 4)) {
                if (load<u32>(q) != o) continue;
                const u32 nx = load<u32>(q + 4);
                store<u32>(q, 0);
                store<u32>(prev + 4, nx);
                break;
            }
        }
        if (head == 0) {
            Stack fr(ctx, 16);  // the key at sp+112
            store<u32>(fr.addr(), load<u32>(d + 8 * static_cast<u32>(i)));
            call(ctx, 0x00786A18u, {R, fr.addr()});
            call(ctx, 0x0054EE6Cu, {hold, 0});
            --i;
        }
    }
    ctx.gpr[3] = load<u16>(R + 2);
    store<u8>(o + 4, 0);
}

// The destructor body of 0x0075A3AC / 0x0075AF24 (o = r3): vtable vt and
// the base part base(o); vtable 0x00B19558, Volume_Notify112D8(*0x00F557EC,
// o + 424), Shape_Release638498(o + 424); the two arrays o + 388 / o + 364
// (u16 count +0, u16 element size << 1 at +6, data +8, allocator +12) free
// their data through alloc->vt[8](alloc, data, count * size) when set and
// are destructed (Array_Destruct); PtrVector_Clear / Nop(o + 344) and the
// base Thunk_000D729C(o) (r3 = its).
inline void DtorShapeArrays40(PPUContext& ctx, u32 vt, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, base, {o, vt});
    store<u32>(o, 0x00B19558u);
    call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), o + 424});
    call(ctx, 0x00638498u, {o + 424});
    for (u32 a : {o + 388, o + 364}) {
        const u32 bytes = static_cast<u32>(load<u16>(a)) * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
        const u32 d = load<u32>(a + 8);
        if (d != 0) {
            const u32 al = load<u32>(a + 12);
            VCall(ctx, al, 8, {al, d, bytes});
        }
        call(ctx, 0x0054EC94u, {a});
    }
    call(ctx, 0x0054EC1Cu, {o + 344});
    call(ctx, 0x0054EC18u, {o + 344});
    call(ctx, 0x000D729Cu, {o});
}

// The template attach of 0x000D99D8 / 0x000D9B30 / 0x000D9C88 (Obj* o,
// u32 key): the template t = *SortedMap_Find744AC4(0x01056F04, &key) of
// kind `kind` (t->+12) gets owner t->+8 = o while the old component of
// that kind (Obj_FindComponent(o, kind, 0)) is removed
// (List_RemoveAll_000D91BC) and Comp_Create2635E8(o, t) made; a new one is
// appended to o's component vector +248 (20 bytes, made on first use by
// SmallAlloc(20) with grow step u16 +6 = 4; an allocation failure reads
// through null, a quirk). t->+8 = 0 and r3 = 0 at the end (the find result
// / t->+12 on the early returns).
inline void ObjAttachTmpl40(PPUContext& ctx, u32 kind) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    u32 t;
    {
        Stack fr(ctx, 16);  // the key at sp+112
        store<u32>(fr.addr(), key);
        const u32 r = static_cast<u32>(call(ctx, 0x00744AC4u, {0x01056F04u, fr.addr()}));
        t = load<u32>(r);
    }
    if (t == 0) return;
    const u32 k = load<u32>(t + 12);
    ctx.gpr[3] = k;
    if (k != kind) return;
    store<u32>(t + 8, o);
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {o, kind, 0}));
    if (c != 0) call(ctx, 0x000D91BCu, {o, c});
    store<u32>(t + 8, o);
    const u32 comp = static_cast<u32>(call(ctx, 0x002635E8u, {o, t}));
    if (comp != 0) {
        u32 v = load<u32>(o + 248);
        if (v == 0) {
            v = static_cast<u32>(call(ctx, 0x0055238Cu, {20}));
            store<u32>(o + 248, v);
            if (v != 0) {
                call(ctx, 0x005516E0u, {v});
                store<u16>(v + 6, 4);
                v = load<u32>(o + 248);
            }
        }
        u32 n = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (n == cap) {
            call(ctx, 0x00551718u, {v, (cap << 1) & 0xFFFFFFFEu});
            n = load<u16>(v + 8);
        }
        store<u32>(load<u32>(v) + ((n << 2) & 0xFFFFFFFCu), comp);
        const u32 n1 = load<u16>(v + 8) + 1;
        const u32 hw = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(n1));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    }
    store<u32>(t + 8, 0);
    ret(ctx, 0u);
}

// The "name" string push of 0x00483A34 / 0x0048414C (XmlCursor* x; vec =
// the pointer vector, data +0, u16 capacity +4, count +8, high-water +10):
// when String_HashNoCase(*0x010C2C28) is the current level's hash
// (*(*0x00F65CA0 + 0x00128114)), x's attribute "name" (0x00AE916C,
// XmlCursor_FindAttrByName) is copied into the string pool (cursor
// *0x010B7838 below *0x010B7834 + 0x8000, String_Assign; the cursor moves
// past it and its 0) and its copy appended to vec (grown to twice the
// capacity when full, PtrVector_Reserve). r3 = 1.
inline void StrPoolPush40(PPUContext& ctx, u32 vec) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {load<u32>(0x010C2C28u)}));
    if (h == load<u32>(load<u32>(0x00F65CA0u) + 0x00128114u)) {
        const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AE916Cu}));
        if (s != 0) {
            const u32 len = static_cast<u32>(call(ctx, 0x00555FA4u, {s}));
            const u32 p = load<u32>(0x010B7838u);
            if (p + len + 1 < load<u32>(0x010B7834u) + 0x8000u) {
                call(ctx, 0x00555F80u, {p, s});
                store<u32>(0x010B7838u, load<u32>(0x010B7838u) + len + 1);
                if (p != 0) {
                    u32 n = load<u16>(vec + 8);
                    const u32 cap = load<u16>(vec + 4);
                    if (n == cap) {
                        call(ctx, 0x00551718u, {vec, (cap << 1) & 0x1FFFEu});
                        n = load<u16>(vec + 8);
                    }
                    const u32 d = load<u32>(vec);
                    const u32 n6 = load<u16>(vec + 8);
                    const u32 hw = load<u16>(vec + 10);
                    store<u32>(d + ((n << 2) & 0x3FFFCu), p);
                    const u32 n1 = n6 + 1;
                    store<u16>(vec + 8, static_cast<u16>(n1));
                    store<u16>(vec + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
                }
            }
        }
    }
    ret(ctx, 1u);
}

}  // namespace obj40
}  // namespace sly4
