// Game code, medium sweep part 3: level sections.
//  - the SetMode override of the section class with vtable 0x00B1E5D0
//    (VtB1E5D0_Dtor calls it with mode 0): sections entering mode 4 join
//    the active-section list 0x00F64C3C (linked through +2592), leaving it
//    they are unlinked, and the world object's highlight bits follow;
//  - the direct destructor of the 0x00B20218 class (its thunks are in
//    mid2_dtor.cpp).
#include "mid3.h"

using namespace sly4;
using namespace sly4::mid3;

namespace {

namespace sfn {
constexpr u32 Section_SetMode = 0x003260F0;   // (o, s32 mode, u8 flag): the base SetMode
constexpr u32 PushGlobal_00527A48 = 0x00527A48;  // ()
constexpr u32 Obj_Notify26 = 0x000D94CC;      // (obj) -> its world object or 0
constexpr u32 PointMap_Find = 0x0073343C;     // (map, const u32* key) -> value*
constexpr u32 Hash_ToString = 0x0056A6E0;     // (hash)
}  // namespace sfn

constexpr u32 kActiveSections = 0x00F64C3C;  // Section* head (next at +2592)
constexpr u32 kCurrentSection = 0x00F64C40;  // Section*
constexpr u32 kSectionPending = 0x00F64C44;  // u32: PushGlobal_00527A48 owed
constexpr u32 kSectionWord_0106B544 = 0x0106B544;  // cleared when the list starts (name?)
constexpr u32 kPointMapOwner = 0x00F8F154;   // -> +16 the hash -> object map

constexpr u32 kMode = 18;        // u8 the current mode
constexpr u32 kNext = 2592;      // Section* next in kActiveSections
constexpr u32 kCount = 2580;     // u16
constexpr u32 kTargetName = 2596;  // u32 hash
constexpr u32 kFlags = 2600;     // 0x80000000 highlight the world object, 0x20000000 mark its draw flag

// The section's world object: o->+12->+8.
u32 WorldObj(u32 o) { return load<u32>(load<u32>(o + 12) + 8); }

// The pending PushGlobal_00527A48 run and cleared.
void FlushPending(PPUContext& ctx) {
    if (load<u32>(kSectionPending) == 0) return;
    call(ctx, sfn::PushGlobal_00527A48);
    store<u32>(kSectionPending, 0);
}

// Bit 0x40 of the u8 +711 of the section's world object set / cleared,
// when it has one with bit 8 of +192.
void MarkDraw(u32 o, bool on) {
    const u32 x = WorldObj(o);
    if (x == 0 || (load<u32>(x + 192) & 8u) == 0) return;
    const u8 b = load<u8>(x + 711);
    store<u8>(x + 711, static_cast<u8>(on ? (b | 0x40) : (b & ~0x40)));
}

}  // namespace

// u32 Section::SetMode(Section* o, s32 mode, u8 flag) (vtable 0x00B1E5D0's
// override of 0x003260F0).
//  - flag 0 and mode == o->+18: just the base SetMode;
//  - mode 4: o joins the active list (first: it becomes the current
//    section too, word 0x0106B544 = 0, the pending push flushed); with a
//    zero u16 +2580 the base SetMode and o->vfn[104](o, 5, 0) follow, else
//    the world object takes its target (+2596 looked up in the point map
//    into +252, or Hash_ToString on a miss) and the highlight (+244 |=
//    0x800), the draw mark (+711 |= 0x40), then the base SetMode;
//  - leaving mode 4: o is unlinked (when o is not in the list, the last
//    node's link is overwritten with o's, as the original), the current
//    section moves on, the highlight is cleared (and set on the
//    o->+380 vfn[8] result's word), the draw mark cleared, then the base.
DECOMP(0032AC78, Section_SetMode_0032AC78) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 mode = ctx.gpr[4];
    const u32 flag = arg<u32>(ctx, 2) & 0xFFu;
    const s32 m = static_cast<s32>(mode);
    const auto base = [&] { ret(ctx, call(ctx, sfn::Section_SetMode, {o, mode, flag})); };
    if (flag == 0 && m == static_cast<s32>(load<u8>(o + kMode))) return base();
    if (m == 4) {
        const u32 head = load<u32>(kActiveSections);
        if (head == 0) {
            store<u32>(kActiveSections, o);
            store<u32>(kCurrentSection, o);
            const u32 pending = load<u32>(kSectionPending);
            store<u32>(kSectionWord_0106B544, 0);
            if (pending != 0) {
                call(ctx, sfn::PushGlobal_00527A48);
                store<u32>(kSectionPending, 0);
            }
        } else {
            u32 p = head;
            while (p != 0 && p != o) p = load<u32>(p + kNext);
            if (p == 0) {
                store<u32>(o + kNext, head);
                store<u32>(kActiveSections, o);
            }
        }
        if (load<u16>(o + kCount) == 0) {
            call(ctx, sfn::Section_SetMode, {o, mode, flag});
            ret(ctx, VCall(ctx, o, 104, {o, 5, 0}));
            return;
        }
        if (const u32 x = WorldObj(o); x != 0) {
            if (const u32 w = static_cast<u32>(call(ctx, sfn::Obj_Notify26, {x})); w != 0) {
                if (load<u32>(o + kTargetName) != 0) {
                    Stack frame(ctx, 16);
                    store<u32>(frame.addr(), load<u32>(o + kTargetName));
                    const u32 map = load<u32>(load<u32>(kPointMapOwner) + 16);
                    const u32 t = load<u32>(static_cast<u32>(call(ctx, sfn::PointMap_Find, {map, frame.addr()})));
                    if (t != 0) store<u32>(w + 252, t);
                    else call(ctx, sfn::Hash_ToString, {load<u32>(o + kTargetName)});
                }
                if (load<u32>(o + kFlags) & 0x80000000u) store<u32>(w + 244, load<u32>(w + 244) | 0x800u);
            }
        }
        if (load<u32>(o + kFlags) & 0x20000000u) MarkDraw(o, true);
        return base();
    }
    if (load<u8>(o + kMode) != 4) return base();
    u32 cur;
    if (load<u32>(kActiveSections) == o) {
        cur = load<u32>(o + kNext);
        store<u32>(kActiveSections, cur);
    } else {
        u32 p = load<u32>(kActiveSections);
        for (;;) {
            const u32 n = load<u32>(p + kNext);
            if (n == 0 || n == o) break;
            p = n;
        }
        store<u32>(p + kNext, load<u32>(o + kNext));
        cur = load<u32>(kActiveSections);
    }
    if (load<u32>(kCurrentSection) == o) {
        store<u32>(kCurrentSection, cur);
        FlushPending(ctx);
    }
    if (load<u32>(o + kFlags) & 0x80000000u) {
        if (const u32 x = WorldObj(o); x != 0) {
            if (const u32 w = static_cast<u32>(call(ctx, sfn::Obj_Notify26, {x})); w != 0) {
                store<u32>(w + 244, load<u32>(w + 244) & ~0x800u);
                const u32 x2 = WorldObj(o);
                if (x2 != 0 && (load<u32>(x2 + 192) & 8u) != 0) {
                    if (const u32 z = load<u32>(x2 + 380); z != 0) {
                        if (const u32 r = static_cast<u32>(VCall(ctx, z, 8, {z})); r != 0) store<u32>(r, load<u32>(r) | 0x800u);
                    }
                }
            }
        }
    }
    if (load<u32>(o + kFlags) & 0x20000000u) MarkDraw(o, false);
    base();
}

// u32 ~T(T* o) of the 0x00B20218 class (448 bytes; see mid2_dtor.cpp
// B20218Dtor, whose thunks enter it through the bases): vtables 0x00B20220
// (+0) / 0x00B2028C (+56) / 0x00B202B4 (+60) / 0x00B202D8 (+64); the block
// +256 freed (SmallFree, +272 * 8 bytes); the PtrVectors +192 / +172 /
// +140, the pool array +116, the PtrVector +96 and the pool array +72
// destroyed; Obj_BaseDtor_0048F6B0(o + 64); the +60 / +56 vtables back to
// 0x00B12528 / 0x00B1200C; then Dtor_B219D0_00490B34(o).
DECOMP(0077D934, VtB20220_Dtor_0077D934) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 vt = 0x00B20218;
    store<u32>(o + 0, vt + 8);
    store<u32>(o + 56, vt + 116);
    store<u32>(o + 60, vt + 156);
    store<u32>(o + 64, vt + 192);
    call(ctx, m3fn::SmallFree, {load<u32>(o + 256), (load<u32>(o + 272) << 3) & 0xFFFFFFF8u});
    VecDtor(ctx, o + 192);
    VecDtor(ctx, o + 172);
    VecDtor(ctx, o + 140);
    PoolArrayDtor(ctx, o + 116);
    VecDtor(ctx, o + 96);
    PoolArrayDtor(ctx, o + 72);
    call(ctx, m3fn::Obj_BaseDtor_0048F6B0, {o + 64});
    store<u32>(o + 60, 0x00B12528);
    store<u32>(o + 56, 0x00B1200C);
    ret(ctx, call(ctx, m3fn::Dtor_B219D0_00490B34, {o}));
}
