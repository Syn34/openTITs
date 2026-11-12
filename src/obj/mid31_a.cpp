// Game code, medium sweep part 31: a spline-node attach update and a
// ragdoll / child teardown.
#include "mid31.h"

using namespace sly4;
using namespace sly4::mid31;

namespace {
using namespace m31fn;

constexpr u32 kHkWorld31 = 0x00F557ECu;   // the volume listener / obstacle system pointer
constexpr u32 kZero3_31 = 0x01164B40u;    // a 12-byte zero vector (BSS)
constexpr u32 kBoxTag31 = 0xA970EC1Fu;    // the box shape kind hash

// The shared loop of the two volume shape rebuilds (000E6D18 with `notify`,
// 000E7180 without). o's list +388 (48-byte rows, u16 count +8, re-read
// every row) is walked beside the def rows ((+216)->+192->0, 160 bytes
// each); a row with a shape +28, a def +208 == the box tag and a row tag
// +144 of 0 or the box tag has its volume +32 rebuilt: with `notify`
// Volume_Notify112D8(*kHkWorld31, vol) first. With the def extent +224 all
// zero: Shape_InitBody(tmp, sys, shape, 0, o->vslot 76(o), mask), else
// Shape_Init110EC(tmp, sys, m, &extent, o->vslot 76(o), mask) with m o's
// matrix +96 (rows of three; y of the translation raised by half the extent
// +228; the w words left unwritten); then Volume_SetShape637D14(vol, tmp)
// and Shape_Release638498(tmp). mask = 0xFFFF with def +300 bit 0x01000000,
// else 0xFFFE.
// r3 is left as the original leaves it: 1 (flag set), 0 (no rows), or the
// row count the loop ended on.
void VolShapes31(PPUContext& ctx, u32 o, bool notify) {
    if ((load<u32>(o + 172) & 0x80u) != 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 d = load<u32>(o + 216);
    const u32 mask = (load<u32>(d + 300) & 0x01000000u) != 0 ? 0xFFFFu : 0xFFFEu;
    {
        const u32 v = load<u32>(o + 388);
        if (v == 0 || load<u16>(v + 8) == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    const f64 half = Lf(notify ? 0x000E6D10u : 0x000E7178u);  // 0.5f
    Stack frame(ctx, 96);
    const u32 m = frame.addr(), tmp = m + 64;  // the original: m at 112(r1), tmp at 176 / 180(r1)
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(o + 388);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0;
        if (i >= n) {
            ret(ctx, n);
            return;
        }
        const u32 row = load<u32>(v) + 48 * i;
        const u32 drow = load<u32>(load<u32>(d + 192)) + 160 * i;
        const u32 shape = load<u32>(row + 28);
        if (shape == 0 || load<u32>(d + 208) != kBoxTag31) continue;
        const u32 t = load<u32>(drow + 144);
        if (t != 0 && t != kBoxTag31) continue;
        const u32 vol = row + 32;
        if (notify) call(ctx, 0x000112D8u, {load<u32>(kHkWorld31), vol});  // Volume_Notify112D8
        const u32 sys = load<u32>(kHkWorld31);
        const bool zero = load<u64>(d + 224) == load<u64>(kZero3_31) && load<u32>(d + 232) == load<u32>(kZero3_31 + 8);
        const u32 mx = o + 96;
        static constexpr u32 kOff[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
        u32 out;
        if (zero) {
            if (notify)  // f1..f12 hold the matrix words at the virtual call (leftovers)
                for (u32 k = 0; k < 12; ++k) ctx.fpr[1 + k] = Lf(mx + kOff[k]);
            const u32 r = static_cast<u32>(VCall(ctx, o, 76, {o}));
            out = tmp;
            call(ctx, 0x00011410u, {out, sys, shape, 0, r, mask});  // Shape_InitBody
        } else {
            for (u32 k = 0; k < 12; ++k)
                if (k != 10) store<u32>(m + kOff[k], load<u32>(mx + kOff[k]));
            const f64 y = Fmadds(Lf(d + 228), half, Lf(mx + 52));
            store<u32>(m + 52, W32(static_cast<f32>(y)));
            if (notify)
                for (u32 k = 1; k < 12; ++k) ctx.fpr[1 + k] = Lf(mx + kOff[k]);
            else
                ctx.fpr[2] = Lf(mx + 52);
            ctx.fpr[1] = y;
            const u32 r = static_cast<u32>(VCall(ctx, o, 76, {o}));
            out = tmp + 4;
            call(ctx, 0x000110ECu, {out, sys, m, d + 224, r, mask});  // Shape_Init110EC
        }
        call(ctx, 0x00637D14u, {vol, out});  // Volume_SetShape637D14
        call(ctx, 0x00638498u, {out});       // Shape_Release638498
    }
}

constexpr u32 kIdent31 = 0x01164BD0u;  // the identity Mat44 (BSS copy)

// Copies the 12 xyz words of the Mat44 at src to dst (rows of 16 bytes; the
// w words untouched), as the original's lfs / stfs pairs do.
void Mat3x4Copy31(u32 dst, u32 src) {
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) store<u32>(dst + 16 * r + 4 * c, load<u32>(src + 16 * r + 4 * c));
}

// The shared body of 000D8954 / 000E7810: with an anim owner +164, o's
// vslot 208(o, m, -, 0) fills m (an identity copy); unless m comes back as
// the identity, o's matrix +96 times m is either handed to o's vslot 84
// (`toVslot`) or copied back into +96, then Obj_SetMatrixD87CC(o, product).
// Then, with an owner whose component (Obj_FindComp34CC34(+164, 0)) is not
// Anim_IsDone353EB0: r3 = 0; else r3 = 1 when the component's +12 has bit
// 0x08000000, otherwise flag 0x2000 of +172 is cleared and r3 = 0.
void AnimMatrix31(PPUContext& ctx, u32 o, bool toVslot) {
    if (load<u32>(o + 164) != 0) {
        Stack frame(ctx, 128);
        const u32 m = frame.addr(), p = m + 64;  // 112(r1) / 176(r1) in the original
        Mat3x4Copy31(m, kIdent31);
        ctx.fpr[2] = Lf(kIdent31 + 52);  // f2 / f3 left by the copy
        ctx.fpr[3] = Lf(kIdent31 + 56);
        const u32 opd = load<u32>(load<u32>(o) + 208);
        call_ptr(ctx, opd, {o, m, opd, 0, load<u32>(opd)});
        bool same = true;
        for (u32 r = 0; r < 4; ++r)
            if (load<u64>(m + 16 * r) != load<u64>(kIdent31 + 16 * r) || load<u32>(m + 16 * r + 8) != load<u32>(kIdent31 + 16 * r + 8)) same = false;
        if (!same) {
            Mat3x4Copy31(p, o + 96);
            ctx.fpr[1] = Lf(o + 96 + 56);
            call(ctx, 0x005541F0u, {p, m});  // Matrix_Multiply(io, m) (r5 = o + 96 left over)
            if (toVslot) {
                VCall(ctx, o, 84, {o, p});
            } else {
                Mat3x4Copy31(o + 96, p);
                ctx.fpr[1] = Lf(p + 56);
            }
            call(ctx, 0x000D87CCu, {o, p});  // Obj_SetMatrixD87CC
        }
    }
    u32 a = load<u32>(o + 164);
    if (a != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x0034CC34u, {a, 0}));  // Obj_FindComp34CC34
        if (c != 0 && static_cast<u32>(call(ctx, 0x00353EB0u, {c, 0})) == 0) {  // Anim_IsDone353EB0
            ret(ctx, 0u);
            return;
        }
    }
    a = load<u32>(o + 164);
    if (a != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x0034CC34u, {a, 0}));
        if (c != 0 && (load<u32>(c + 12) & 0x08000000u) != 0) {
            ret(ctx, 1u);
            return;
        }
    }
    store<u32>(o + 172, load<u32>(o + 172) & ~0x2000u);
    ret(ctx, 0u);
}
}  // namespace

// bool f(T* o; f32 dt): 0 unless Obj_Check_001AEA50(o) (it reads the
// caller's f1, passed through). With flag 0x08000000 of +580 pending: the
// node +564 is looked up as the component +568 of the effect +356 when its
// vslot 32 reports kind 5 and its +28 is a kind-13 node, else by
// Node_FindKind13_004A21F8(+160, +568), else from the spline map (the
// entry's first word, stored even when 0); the flag is then cleared. Unless
// 0x40000000 is set: +556 = Obj_FindById(+560) when it has +192 bit
// 0x00400000 (its u16 +178 is bumped), else 0, and 0x40000000 is set.
// With +172 bit 0x200, each of the six slots i with u8 +600 + 20i and an
// object +588 + 20i gets Obj_SlotMatrixB_001C5738(m, o, i) passed to its
// vslot 84. Then 0x001C7348(o); r3 = 1.
DECOMP(001C78C8, SplineNode_Attach_001C78C8) {
    const u32 o = arg<u32>(ctx, 0);
    callf(ctx, ObjCheck31, {o}, {farg(ctx, 0)});
    if (static_cast<s32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 128);
    const u32 sp = frame.addr();
    u32 fl = load<u32>(o + 580);
    if ((fl & 0x08000000u) != 0) {
        u32 c = load<u32>(o + 356);
        if (c != 0) {
            c = static_cast<u32>(call(ctx, FindComp31, {c, load<u32>(o + 568)}));
            if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
                const u32 n = load<u32>(c + 28);
                if (n != 0 && ((load<u32>(n + 4) >> 10) & 63) == 13) store<u32>(o + 564, n);
            }
        }
        u32 r = load<u32>(o + 564);
        if (r == 0) {
            r = static_cast<u32>(call(ctx, 0x004A21F8u, {load<u32>(o + 160), load<u32>(o + 568)}));  // Node_FindKind13_004A21F8
            store<u32>(o + 564, r);
            if (r == 0) {
                store<u32>(sp + 64, load<u32>(o + 568));
                const u32 e = static_cast<u32>(call(ctx, SplMapFind31, {load<u32>(load<u32>(kSplineSlot31) + 16), sp + 64}));
                store<u32>(o + 564, load<u32>(e));
            }
        }
        fl = load<u32>(o + 580) & ~0x08000000u;
        store<u32>(o + 580, fl);
    }
    if ((fl & 0x40000000u) == 0) {
        const u32 x = static_cast<u32>(call(ctx, ObjById31, {load<u32>(o + 560)}));
        u32 r = 0;
        if (x != 0 && (load<u32>(x + 192) & 0x00400000u) != 0) r = x;
        store<u32>(o + 556, r);
        if (r != 0) store<u16>(r + 178, static_cast<u16>(load<u16>(r + 178) + 1));
        store<u32>(o + 580, load<u32>(o + 580) | 0x40000000u);
    }
    if ((load<u32>(o + 172) & 0x200u) != 0) {
        for (u32 i = 0; i < 6; ++i) {
            if (load<u8>(o + 600 + 20 * i) == 0) continue;
            const u32 p = load<u32>(o + 588 + 20 * i);
            if (p == 0) continue;
            call(ctx, 0x001C5738u, {sp, o, i});  // Obj_SlotMatrixB_001C5738(f32 m[16], o, i)
            VCall(ctx, p, 84, {p, sp});
        }
    }
    call(ctx, 0x001C7348u, {o});
    ret(ctx, 1u);
}

// u32 f(T* o, u8 on): with on, the teardown: 0x001EDD84(o, 0); the
// objects +820 / +824 get vslot 40(x, 1) and are cleared; flag 0x10000000
// of +980 is cleared; Obj_ReleaseChild_001ED85C(o, +812); the two slots
// +828 / +832 get vslot 40(x, 1) and are cleared; +872 gets vslot 40(x, 0)
// and is cleared; while +816 is set: its +160's effect +260 gets
// vslot 24(e, +160), then +816 gets vslot 88(x, 0, 1, 1, 1), vslot 40(x, 0)
// and vslot 40(x, 1), and is cleared. The skeleton +852 (if any) leaves its
// body world (+8, Body_Remove931F00) and is released; each 8-byte row
// {child, obj} of the u16-counted list +640 has its obj (if any) given
// vslot 40(obj, 0) and its child released; the count is zeroed. Always
// tail-calls Obj_SetRagdoll_000E4544(o, on).
DECOMP(001EE0CC, Obj_Teardown_001EE0CC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFF;
    if (on != 0) {
        call(ctx, 0x001EDD84u, {o, 0});
        for (u32 off : {820u, 824u}) {
            const u32 x = load<u32>(o + off);
            if (x == 0) continue;
            VCall(ctx, x, 40, {x, 1});
            store<u32>(o + off, 0);
        }
        store<u32>(o + 980, load<u32>(o + 980) & ~0x10000000u);
        const u32 c812 = load<u32>(o + 812);
        call(ctx, RelChild31, {o, c812});
        for (u32 i = 0; i < 2; ++i) {
            const u32 x = load<u32>(o + 828 + 4 * i);
            if (x == 0) continue;
            VCall(ctx, x, 40, {x, 1});
            store<u32>(o + 828 + 4 * i, 0);
        }
        const u32 x872 = load<u32>(o + 872);
        if (x872 != 0) VCall(ctx, x872, 40, {x872, 0});
        const u32 p = load<u32>(o + 816);
        store<u32>(o + 872, 0);
        if (p != 0) {
            const u32 c = load<u32>(p + 160);
            if (c != 0) {
                const u32 e = load<u32>(c + 260);
                if (e != 0) VCall(ctx, e, 24, {e, c});
            }
            const u32 q = load<u32>(o + 816);
            VCall(ctx, q, 88, {q, 0, 1, 1, 1});
            const u32 q2 = load<u32>(o + 816);
            VCall(ctx, q2, 40, {q2, 0});
            const u32 q3 = load<u32>(o + 816);
            VCall(ctx, q3, 40, {q3, 1});
            store<u32>(o + 816, 0);
        }
        const u32 s = load<u32>(o + 852);
        if (s != 0) {
            const u32 w = load<u32>(s + 8);
            if (w != 0) call(ctx, 0x00931F00u, {w, s});  // Body_Remove931F00(world, body)
            const u32 s2 = load<u32>(o + 852);
            call(ctx, 0x007D4EA8u, {s2});  // AnimSkel_Release7D4EA8
        }
        for (u32 i = 0; i < load<u16>(o + 648); ++i) {
            const u32 row = load<u32>(o + 640) + 8 * i;
            const u32 x = load<u32>(row + 4);
            if (x != 0) VCall(ctx, x, 40, {x, 0});
            call(ctx, RelChild31, {o, load<u32>(row)});
        }
        store<u16>(o + 648, 0);
    }
    ret(ctx, call(ctx, 0x000E4544u, {o, on}));  // Obj_SetRagdoll_000E4544
}

// void f(T* o): o's box volumes re-shaped after a move, each one first
// unregistered from the listener (VolShapes31 with the notify).
DECOMP(000E6D18, Volume_Reshape_000E6D18) {
    VolShapes31(ctx, arg<u32>(ctx, 0), true);
}

// void f(T* o): the same rebuild without the listener notify.
DECOMP(000E7180, Volume_Rebuild_000E7180) {
    VolShapes31(ctx, arg<u32>(ctx, 0), false);
}

// bool f(T* o): the anim-driven matrix update handing the product to
// o's vslot 84 (AnimMatrix31).
DECOMP(000D8954, Obj_AnimMatrixV_000D8954) {
    AnimMatrix31(ctx, arg<u32>(ctx, 0), true);
}

// bool f(T* o): the same, writing the product into o's matrix +96.
DECOMP(000E7810, Obj_AnimMatrix_000E7810) {
    AnimMatrix31(ctx, arg<u32>(ctx, 0), false);
}

namespace {
// The shared physics tail of the two rigid-body defs' constructors (o, def
// d, entity slot o + slot, the f32 0 at kZero): f32 +220 below 0 takes
// d +76; the named entity d +124 (Obj_FindName_0034654C in the game's
// named-object table) leaves its world (+8, Hk_RemoveEntity92FFC8 into a
// stack result) and is replaced by its vslot 32 result, stored in the slot.
// With d +128 >= 0 and an entity motion kind (u8 +232) other than 4 / 5:
// Hk_SetMass_0091D618 (f1 = d +128) when it is non-zero, else
// Hk_SetMotion_0091D470(e, 5, 1, 0). d +132 / +136 (when >= 0) go to the
// entity's +140 / +144; its f32 fields +411 / +410 get 5 / 100
// (Hk_SetF32At_007EE08C) and +44 the quality 2 (kind 5), 3 (kind 4) or 4.
// Returns the entity (0 when not found); f1 as the original leaves it.
u32 PhysTail31(PPUContext& ctx, u32 o, u32 d, u32 slot, u32 kZero) {
    const f64 z = Lf(kZero);
    if (Lf(o + 220) < z) store<u32>(o + 220, load<u32>(d + 76));
    ctx.fpr[1] = Lf(d + 76);
    const u32 tbl = load<u32>(load<u32>(0x00F65CA0u) + 0x001281D4u);
    u32 e = static_cast<u32>(call(ctx, 0x0034654Cu, {tbl, load<u32>(d + 124)}));  // Obj_FindName_0034654C
    if (e == 0) return 0;
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();
    const u32 w = load<u32>(e + 8);
    if (w != 0) call(ctx, 0x0092FFC8u, {sp + 8, w, e});  // Hk_RemoveEntity92FFC8(result*, world, entity)
    e = static_cast<u32>(VCall(ctx, e, 32, {e}));
    const f64 mass = Lf(d + 128);
    ctx.fpr[1] = mass;
    store<u32>(o + slot, e);
    if (mass >= z) {
        const u8 k = load<u8>(e + 232);
        if (k != 5 && k != 4) {
            if (mass != z) call(ctx, 0x0091D618u, {e});  // Hk_SetMass_0091D618
            else call(ctx, 0x0091D470u, {e, 5, 1, 0});   // Hk_SetMotion_0091D470
        }
    }
    ctx.fpr[1] = Lf(d + 132);
    if (Lf(d + 132) >= z) store<u32>(load<u32>(o + slot) + 140, load<u32>(d + 132));
    ctx.fpr[1] = Lf(d + 136);
    if (Lf(d + 136) >= z) store<u32>(load<u32>(o + slot) + 144, load<u32>(d + 136));
    e = load<u32>(o + slot);
    u32 q = 4;
    if (load<u8>(e + 232) == 5) q = 2;
    else if (load<u8>(e + 232) == 4) q = 3;
    store<u32>(sp + 4, 0x40A00000u);  // 5.0f
    call(ctx, 0x007EE08Cu, {e + 411, sp + 4});  // Hk_SetF32At_007EE08C
    e = load<u32>(o + slot);
    store<u32>(sp, 0x42C80000u);  // 100.0f
    call(ctx, 0x007EE08Cu, {e + 410, sp});
    e = load<u32>(o + slot);
    store<u32>(e + 44, q);
    return e;
}
}  // namespace

// void f(T* o, Def* d): a rigid-body object's constructor: base
// Thunk_000C3078(o, d), vtable 0x00B14360, entity slot +268 = 0; d's
// f32s +140..+180 and words +184..+200 copied to +192..+252, bytes
// +208..+213 to +260..+265; then PhysTail31 (slot +268) and +272 = d +216
// (also r3).
DECOMP(001664C8, PhysDefA_Construct_001664C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = arg<u32>(ctx, 1);
    call(ctx, 0x000C3078u, {o, d});  // Thunk_000C3078
    u32 v[16];
    for (u32 k = 0; k < 16; ++k) v[k] = load<u32>(d + 140 + 4 * k);
    store<u32>(o + 268, 0);
    for (u32 k = 16; k-- > 0;) store<u32>(o + 192 + 4 * k, v[k]);
    store<u32>(o + 0, 0x00B14360u);
    for (u32 k = 0; k < 6; ++k) store<u8>(o + 260 + k, load<u8>(d + 208 + k));
    PhysTail31(ctx, o, d, 268, 0x001664C4u);
    const u32 r = load<u32>(d + 216);
    store<u32>(o + 272, r);
    ret(ctx, r);
}

// void f(T* o, Def* d): the second rigid-body constructor: vtable
// 0x00B14B50, entity slot +288 = 0; d's +140..+228 copied to +192..+280
// (u8 +216 / +228 as bytes), bytes +229..+234 to +281..+286; then
// PhysTail31 (slot +288). r3 = the entity (0 when not found).
DECOMP(0017C918, PhysDefB_Construct_0017C918) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = arg<u32>(ctx, 1);
    call(ctx, 0x000C3078u, {o, d});  // Thunk_000C3078
    u32 v[19];
    for (u32 k = 0; k < 19; ++k) v[k] = load<u32>(d + 140 + 4 * k);
    const u8 b216 = load<u8>(d + 216), b228 = load<u8>(d + 228);
    const u32 w220 = load<u32>(d + 220), w224 = load<u32>(d + 224);
    store<u32>(o + 288, 0);
    store<u8>(o + 280, b228);
    store<u32>(o + 276, w224);
    store<u32>(o + 272, w220);
    store<u8>(o + 268, b216);
    for (u32 k = 19; k-- > 0;) store<u32>(o + 192 + 4 * k, v[k]);  // +264 (f32), +260 (word), +256..+192
    store<u32>(o + 0, 0x00B14B50u);
    for (u32 k = 0; k < 6; ++k) store<u8>(o + 281 + k, load<u8>(d + 229 + k));
    ret(ctx, PhysTail31(ctx, o, d, 288, 0x0017C914u));
}

// void f(T* o, Def* d): a spline-body object's constructor: base
// Thunk_000C3078(o, d), vtable 0x00B16058; the 8-byte point vector +192
// (PtrVector_Construct, element size u16 +198 = 8, PtrVector_Init(v,
// d u16 +132, 0, 1)) gets the d u16 +132 points of d +124 appended one by
// one (doubling PtrVector_Reserve when the count u16 +200 reaches the
// capacity u16 +196; high-water u16 +202); +232 / +240 / +236 = d +164 /
// +172 / +168, entity +228 = 0, flags +244 take bits 0x80000000 /
// 0x40000000 of d +176 and (by a byte store) 0x20000000; +212.. = the 16
// bytes at d +144. The named entity d +160 (Obj_FindName_0034654C) leaves
// its world (Hk_RemoveEntity92FFC8 into a stack result), is replaced by its
// vslot 32 result (+228), keyframed (Hk_SetMotion_0091D470(e, 4, 1, 0)),
// u8 +42 = 9, 0x0091D694(e; f1 = 255), u16 +406 = 0x437F, f32 +411 = 0
// (Hk_SetF32At_007EE08C) and +44 = 28. r3 = 28, or 0 without the entity.
DECOMP(001B37E8, SplineBody_Construct_001B37E8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = arg<u32>(ctx, 1);
    call(ctx, 0x000C3078u, {o, d});  // Thunk_000C3078
    const u32 v = o + 192;
    const u32 n0 = load<u16>(d + 132);
    store<u32>(o + 0, 0x00B16058u);
    call(ctx, 0x005516E0u, {v});  // PtrVector_Construct
    store<u16>(o + 198, 8);
    call(ctx, 0x00551598u, {v, n0, 0, 1});  // PtrVector_Init
    const u32 f164 = load<u32>(d + 164), w172 = load<u32>(d + 172);
    store<u32>(o + 232, f164);
    store<u32>(o + 240, w172);
    store<u32>(o + 228, 0);
    store<u32>(o + 236, load<u32>(d + 168));
    u32 fl = (load<u32>(o + 244) & 0x7FFFFFFFu) | (load<u32>(d + 176) & 0x80000000u);
    store<u32>(o + 244, fl);
    fl = (fl & ~0x40000000u) | (load<u32>(d + 176) & 0x40000000u);
    store<u32>(o + 244, fl);
    const u8 top = static_cast<u8>(((fl >> 24) & ~0x20u) | ((load<u32>(d + 176) & 0x20000000u) != 0 ? 0x20u : 0u));
    ctx.fpr[1] = Lf(d + 168);
    u32 cur = load<u16>(o + 200);  // r26 in the original (the count as last computed)
    u32 idx = cur;                 // r3
    store<u8>(o + 244, top);
    for (u32 i = 0; i < load<u16>(d + 132); ++i) {
        const u32 src = load<u32>(d + 124) + 8 * i;
        const u32 cap = load<u16>(o + 196);
        if ((cur & 0xFFFF) == cap) {
            call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});  // PtrVector_Reserve
            idx = load<u16>(o + 200);
        }
        const u32 dst = load<u32>(o + 192) + ((idx << 3) & 0x7FFF8u);
        store<u32>(dst + 0, load<u32>(src + 0));
        store<u32>(dst + 4, load<u32>(src + 4));
        ctx.fpr[1] = Lf(src + 4);  // the points are copied through f1
        idx = load<u16>(o + 200) + 1u;
        cur = idx & 0xFFFF;
        store<u16>(o + 200, static_cast<u16>(idx));
        u32 hw = load<u16>(o + 202);
        if (static_cast<s32>(hw) <= static_cast<s32>(cur)) {
            hw = idx;
            cur = idx;
        }
        store<u16>(o + 202, static_cast<u16>(hw));
    }
    u32 q[4];
    for (u32 k = 0; k < 4; ++k) q[k] = load<u32>(d + 144 + 4 * k);
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 212 + 4 * k, q[k]);
    const u32 tbl = load<u32>(load<u32>(0x00F65CA0u) + 0x001281D4u);
    u32 e = static_cast<u32>(call(ctx, 0x0034654Cu, {tbl, load<u32>(d + 160)}));  // Obj_FindName_0034654C
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();
    const u32 w = load<u32>(e + 8);
    if (w != 0) call(ctx, 0x0092FFC8u, {sp + 4, w, e});  // Hk_RemoveEntity92FFC8(result*, world, entity)
    e = static_cast<u32>(VCall(ctx, e, 32, {e}));
    store<u32>(o + 228, e);
    call(ctx, 0x0091D470u, {e, 4, 1, 0});  // Hk_SetMotion_0091D470
    e = load<u32>(o + 228);
    store<u8>(e + 42, 9);
    callf(ctx, 0x0091D694u, {e}, {Lf(0x001B37E4u)});  // f1 = 255
    e = load<u32>(o + 228);
    store<u16>(e + 406, 0x437F);
    store<u32>(sp, 0);
    call(ctx, 0x007EE08Cu, {e + 411, sp});  // Hk_SetF32At_007EE08C
    store<u32>(load<u32>(o + 228) + 44, 28);
    ret(ctx, 28u);
}

// void f(T* o, Def* d): a dynamic-body object's constructor: base
// Thunk_000C3078(o, d), vtable 0x00B14590; d's f32s +140..+180 copied to
// +192..+232 (+220 replaced by d +76 when below 0), +240 = d +188, body
// +244 = 0, +256..+264 = the zero vector. The named entity d +124
// (Obj_FindName_0034654C) is taken out of its world twice over
// (Hk_RemoveEntity92FFC8 into two stack result bytes, the world re-read)
// and a stack rigid-body cinfo built from it (0x0091FB50 constructor,
// Hk_CinfoFromBody91D208); d +132 / +136 (when >= 0) go to cinfo +160 /
// +168, +156 = +176 = 0, u8 +180 = 1, +200 = 8, +204 = 1. A 544-byte body
// from the thread's Havok router (u16 +4 = 544, stored even when the
// allocation failed) is constructed from it (Hk_RigidBodyCtor_0091E100)
// into +244; HkBody_SetName7CE950(body +120, entity +120 & ~1); mass 100
// (Hk_SetMass_0091D618, f1); u8 +42 = 8; Hk_SetMotion_0091D470(b, 3, 1,
// 0); d +132 / +136 (when >= 0) to body +140 / +144; +44 = 8. r3 = 8, or
// 0 without the entity.
DECOMP(0016A448, DynBody_Construct_0016A448) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = arg<u32>(ctx, 1);
    call(ctx, 0x000C3078u, {o, d});  // Thunk_000C3078
    for (u32 k = 0; k < 8; ++k) store<u32>(o + 192 + 4 * k, load<u32>(d + 140 + 4 * k));
    const u32 f172 = load<u32>(d + 172);
    store<u32>(o + 0, 0x00B14590u);
    store<u32>(o + 224, f172);
    store<u32>(o + 228, load<u32>(d + 176));
    store<u32>(o + 232, load<u32>(d + 180));
    store<u32>(o + 256, load<u32>(kZero3_31 + 0));
    const u32 w188 = load<u32>(d + 188);
    store<u32>(o + 240, w188);
    store<u32>(o + 244, 0);
    store<u32>(o + 260, load<u32>(kZero3_31 + 4));
    store<u32>(o + 264, load<u32>(kZero3_31 + 8));
    const f64 z = Lf(0x0016A440u);  // 0.0f
    ctx.fpr[1] = Lf(d + 168);
    if (Lf(d + 168) < z) {
        store<u32>(o + 220, load<u32>(d + 76));
        ctx.fpr[1] = Lf(d + 76);
    }
    const u32 tbl = load<u32>(load<u32>(0x00F65CA0u) + 0x001281D4u);
    const u32 e = static_cast<u32>(call(ctx, 0x0034654Cu, {tbl, load<u32>(d + 124)}));  // Obj_FindName_0034654C
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 224);
    const u32 ci = frame.addr(), res = ci + 208;  // 112(r1) / 320(r1) in the original
    if (load<u32>(e + 8) != 0) call(ctx, 0x0092FFC8u, {res, load<u32>(e + 8), e});      // Hk_RemoveEntity92FFC8
    if (load<u32>(e + 8) != 0) call(ctx, 0x0092FFC8u, {res + 1, load<u32>(e + 8), e});
    call(ctx, 0x0091FB50u, {ci});     // the rigid-body cinfo constructor
    call(ctx, 0x0091D208u, {e, ci});  // Hk_CinfoFromBody91D208
    {
        const f64 a = Lf(d + 132), b = Lf(d + 136);
        if (a >= z) store<u32>(ci + 160, load<u32>(d + 132));
        if (b >= z) store<u32>(ci + 168, load<u32>(d + 136));
        ctx.fpr[1] = b;
        ctx.fpr[2] = a;
    }
    store<u32>(ci + 156, 0);
    store<u8>(ci + 180, 1);
    store<u8>(ci + 200, 8);
    store<u32>(ci + 176, 0);
    store<u8>(ci + 204, 1);
    const u32 rt = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) - 28332u) + 44);  // the thread's Havok router
    const u32 p = static_cast<u32>(VCall(ctx, rt, 8, {rt, 544}));
    store<u16>(p + 4, 544);  // also when the allocation failed (address 4)
    u32 body = 0;
    if (p != 0) {
        call(ctx, 0x0091E100u, {p, ci});  // Hk_RigidBodyCtor_0091E100
        body = p;
    }
    store<u32>(o + 244, body);
    call(ctx, 0x007CE950u, {body + 120, load<u32>(e + 120) & ~1u});  // HkBody_SetName7CE950
    ctx.fpr[1] = Lf(0x0016A444u);  // 100.0f
    call(ctx, 0x0091D618u, {load<u32>(o + 244)});  // Hk_SetMass_0091D618
    const u32 b2 = load<u32>(o + 244);
    store<u8>(b2 + 42, 8);
    call(ctx, 0x0091D470u, {b2, 3, 1, 0});  // Hk_SetMotion_0091D470
    const f64 a = Lf(d + 132), b = Lf(d + 136);
    ctx.fpr[1] = b;
    ctx.fpr[2] = a;
    const u32 bb = load<u32>(o + 244);
    if (a >= z) store<u32>(bb + 140, load<u32>(d + 132));
    if (b >= z) store<u32>(bb + 144, load<u32>(d + 136));
    store<u32>(bb + 44, 8);
    ret(ctx, 8u);
}
