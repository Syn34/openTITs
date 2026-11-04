// The base ApeObj slots (vtable 0x00B24610) the instance vtables share,
// and the ApePointInstance (kind 12, vtable 0x00B24998) and
// ApeSplineInstance (kind 13, vtable 0x00B249E8) slots.
#include <bit>

#include "obj7.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj7;

// ---------------------------------------------------------------- shared helpers

void sly4::obj7::ApeObjDestructBody(PPUContext& ctx, u32 o) {
    store<u32>(o, obj3::kVtblBase1);
    if ((load<u32>(o + obj::kObjFlags) & obj::kObjQueuedFlag) != 0) {
        const u32 v = load<u32>(obj::kDirtyList);
        const u32 n = load<u16>(v + 8);
        const u32 last = n - 1;
        if (n != 0) {
            for (s32 i = static_cast<s32>(last);; --i) {
                const u32 data = load<u32>(v);
                if (load<u32>(data + static_cast<u32>(i) * 4) == o) {
                    store<u16>(v + 8, static_cast<u16>(last));
                    if (i != static_cast<s32>(last & 0xFFFF))
                        store<u32>(data + static_cast<u32>(i) * 4, load<u32>(data + ((last << 2) & 0x3FFFC)));
                    store<u32>(o + obj::kObjFlags, load<u32>(o + obj::kObjFlags) & ~obj::kObjQueuedFlag);
                    break;
                }
                if (i == 0) break;
            }
        }
    }
    call(ctx, o7fn::WorldObj_ReleasePeers, {o});
    call(ctx, cls::ifn::ApeObj_SetClass, {o, 0});
    call(ctx, o7fn::ApeRoot_Destruct, {o});
}

void sly4::obj7::InstanceRelease(PPUContext& ctx, u32 o, u32 destroy_opd) {
    const u16 refs = static_cast<u16>(load<u16>(o + kObjRefs) - 1);
    store<u16>(o + kObjRefs, refs);
    if (static_cast<s16>(refs) > 0) return;
    call(ctx, cls::ifn::WorldObj_Unlink, {o});
    cls::RemoveChildren(ctx, o);
    call(ctx, cls::ifn::WorldObj_ReleaseRes, {o});
    call(ctx, cls::ifn::DeferredDestroy, {o, destroy_opd});
}

u32 sly4::obj7::CopyAnchor(u32 o) {
    const u32 idx = load<u16>(o + obj::kObjFlags);
    if (idx == 0xFFFF) return 0;
    const u32 peers = load<u32>(load<u32>(obj3::kSlotPeers));
    const u32 base = load<u32>(peers);
    // The record's index, recomputed as (record - base) / 160 (signed).
    const s32 i = static_cast<s32>(base + idx * kPeerSize - base) / static_cast<s32>(kPeerSize);
    const u32 dst = load<u32>(peers + 4) + (static_cast<u32>(i) << 5);
    CopyVec4(dst, o + kObjAnchor);
    return dst;
}

// ---------------------------------------------------------------- ApeObj (0x00B24610)

// ApeSkeleton* ApeObj_GetSkeleton(Obj* o)  (slot +28)
// The base objects have none.
DECOMP(007A9918, ApeObj_GetSkeleton) { ret(ctx, 0u); }

// void ApeObj_SetPerView(Obj* o, u32 v)  (slot +32)
// The base ignores it.
DECOMP(007A9920, ApeObj_SetPerView) {}

// void ApeObj_Changed(Obj* o)  (slot +36)
// Only the peer copy. Leaves r3 = &o->+240, r4 = the destination and f1
// (or r4 = 0xFFFF without a peer record), as the original does.
DECOMP(0057FC40, ApeObj_Changed) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 dst = CopyAnchor(o);
    if (dst == 0) {
        ctx.gpr[4] = 0xFFFF;
        return;
    }
    ctx.fpr[1] = static_cast<f64>(load<f32>(o + kObjAnchor + 12));
    ctx.gpr[4] = dst;
    ret(ctx, o + kObjAnchor);
}

// void ApeObj_SetHidden(Obj* o, u8 visible, bool recurse)  (slot +40)
// As WorldObj_SetHidden without the group-4 exception: bit 0x8000 of +164
// from bit 0 of `visible`, optionally the children's (their +40), then
// vfunc +36 when the old bit differed from `visible`.
DECOMP(0057FCD4, ApeObj_SetHidden) {
    const u32 o = arg<u32>(ctx, 0), v_arg = arg<u32>(ctx, 1);
    const u32 v = v_arg & 0xFF, recurse = arg<u32>(ctx, 2) & 0xFF;
    const u32 w = load<u32>(o + obj::kObjFlags);
    const u32 old = (w >> 15) & 1;
    store<u32>(o + obj::kObjFlags, (w & ~kObjFlagVisible) | ((v_arg << 15) & kObjFlagVisible));
    if (recurse != 0)
        for (u32 c = load<u32>(o + kObjFirst); c != 0; c = load<u32>(c + kObjNext)) {
            const u32 opd = load<u32>(load<u32>(c) + 40);
            call_ptr(ctx, opd, {c, v, recurse, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
        }
    if (old != v) Vcall0(ctx, o, kVfnChanged);
}

// void ApeObj_SetByte320(Obj* o, u8 v, bool recurse)  (slot +48)
// The base has no +320: only the children's (r4 passed on unmasked).
DECOMP(0057FE40, ApeObj_SetByte320) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 v = ctx.gpr[4];
    const u32 recurse = arg<u32>(ctx, 2) & 0xFF;
    if (recurse == 0) return;
    for (u32 c = load<u32>(o + kObjFirst); c != 0; c = load<u32>(c + kObjNext)) {
        const u32 opd = load<u32>(load<u32>(c) + 48);
        call_ptr(ctx, opd, {c, v, recurse, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
    }
}

// void ApeObj_SetByte168(Obj* o, u8 v, bool recurse)  (slot +52)
// Likewise: only the children's.
DECOMP(0057FEC8, ApeObj_SetByte168) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = arg<u32>(ctx, 1) & 0xFF, recurse = arg<u32>(ctx, 2) & 0xFF;
    if (recurse == 0) return;
    for (u32 c = load<u32>(o + kObjFirst); c != 0; c = load<u32>(c + kObjNext)) {
        const u32 opd = load<u32>(load<u32>(c) + 52);
        call_ptr(ctx, opd, {c, v, recurse, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
    }
}

// ---------------------------------------------------------------- ApePointInstance (0x00B24998)

// void PointInstance_Destruct(PointInstance* o)  (slot +0)
DECOMP(007A9F38, PointInstance_Destruct) { ApeObjDestructBody(ctx, arg<u32>(ctx, 0)); }

// void PointInstance_DeletingDestruct(PointInstance* o)  (slot +4)
DECOMP(007AA024, PointInstance_DeletingDestruct) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    ApeObjDestructBody(ctx, o);
    call(ctx, cls::ifn::Mem_FreeFrom, {o, 0});
}

// void PointInstance_Release(PointInstance* o)  (slot +8)
DECOMP(005A6808, PointInstance_Release) { InstanceRelease(ctx, arg<u32>(ctx, 0), kPointDestroyOpd); }

// void PointInstance_ShowAndNotify(PointInstance* o, bool)  (slot +16)
// Points ignore it.
DECOMP(007AA128, PointInstance_ShowAndNotify) {}

// PointInstance* PointInstance_Clone(const PointInstance* o, u32 flags)  (slot +56)
// A new point of o's definition with o's base state; children cloned with
// kCloneChildren.
DECOMP(005A6B28, PointInstance_Clone) {
    const u32 o = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 1);
    const u32 n = static_cast<u32>(call(ctx, o7fn::PointInstance_New, {load<u32>(o + kObjClassPtr)}));
    call(ctx, obj5::o5fn::WorldObj_CopyBase, {n, o, 0});
    if ((flags & kCloneChildren) != 0) cls::CloneChildren(ctx, o, n, flags);
    ret(ctx, n);
}

// void PointInstance_Dump(PointInstance* o, s32 depth, u32 flags)  (slot +60)
DECOMP(005A6C28, PointInstance_Dump) {
    call(ctx, cls::ifn::WorldObj_DumpTree, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5]});
}

// ---------------------------------------------------------------- ApeSplineInstance (0x00B249E8)

// void SplineInstance_Destruct(SplineInstance* o)  (slot +0)
// Gives its record back (0x007A9128, the index sign-extended; +284 = -1),
// then the ApeObj destructor body.
DECOMP(005A7680, SplineInstance_Destruct) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 idx = load<u32>(o + kObjSplineIndex);
    if (idx != 0xFFFFFFFFu) {
        store<u32>(o, obj5::kVtblKind13);
        call(ctx, o7fn::SplineList_Remove, {kSplineList, static_cast<u64>(static_cast<s64>(static_cast<s32>(idx)))});
        store<u32>(o + kObjSplineIndex, 0xFFFFFFFFu);
    }
    ApeObjDestructBody(ctx, o);
}

// void SplineInstance_DeletingDestruct(SplineInstance* o)  (slot +4)
DECOMP(005A77A4, SplineInstance_DeletingDestruct) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    call(ctx, o7fn::SplineInstance_Destruct, {o});
    call(ctx, cls::ifn::Mem_FreeFrom, {o, 0});
}

// void SplineInstance_Release(SplineInstance* o)  (slot +8)
DECOMP(005A7A54, SplineInstance_Release) { InstanceRelease(ctx, arg<u32>(ctx, 0), kSplineDestroyOpd); }

// void SplineInstance_ShowAndNotify(SplineInstance* o, bool)  (slot +16)
// Only vfunc +36.
DECOMP(005A77F0, SplineInstance_ShowAndNotify) { Vcall0(ctx, arg<u32>(ctx, 0), kVfnChanged); }

// void SplineInstance_SyncRecord(SplineInstance* o)
// Refreshes o's record from the record's owner (+88; o itself normally):
// the owner's world matrix rows (+80, three floats each) and +240 vec4,
// the class's spline data (+112) when it has a class, and with data its
// +4 and count. The record is active when the owner is visible and has no
// parent, a visible parent that is not a spline, or a spline parent while
// +95 bit 0x40 is set. Leaves r3 = the new +95, r4 = the record, r5 = the
// active bit and f1 = the last float copied, as the original does.
DECOMP(005A782C, SplineInstance_SyncRecord) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 idx = load<u32>(o + kObjSplineIndex);
    if (idx == 0xFFFFFFFFu) {
        ctx.gpr[4] = idx;
        return;
    }
    const u32 rec = load<u32>(kSplineList + 12) + idx * obj5::kK13RecSize;
    const u32 ow = load<u32>(rec + kSrOwner);
    const u32 w = ow + obj6::kObjWorld;
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(rec + row + k, load<u32>(w + row + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(rec + 64 + k, load<u32>(ow + kObjAnchor + k));
    const u32 a3 = load<u32>(ow + kObjAnchor + 12);
    u32 data = load<u32>(rec + kSrData);
    store<u32>(rec + 76, a3);
    const u32 cl = load<u32>(ow + kObjClassPtr);
    if (cl != 0) {
        data = cl + kSplineDefData;
        store<u32>(rec + kSrData, data);
    }
    if (data != 0) {
        store<u32>(rec + kSrData4, load<u32>(data + 4));
        store<u16>(rec + kSrCount, static_cast<u16>(load<u32>(data) & 0x7FFF));
    }
    u32 active = 0;
    if ((load<u32>(ow + obj::kObjFlags) & kObjFlagVisible) != 0) {
        const u32 p = load<u32>(ow + kObjParentLink);
        if (p == 0) {
            active = 1;
        } else if (Kind(p) == obj5::kKind13) {
            active = (load<u8>(rec + kSrBits) & kSrFollowParent) != 0 ? 1 : 0;
        } else {
            active = (load<u32>(p + obj::kObjFlags) & kObjFlagVisible) != 0 ? 1 : 0;
        }
    }
    const u32 b = (load<u8>(rec + kSrBits) & ~kSrActive & 0xFF) | (active << 7);
    store<u8>(rec + kSrBits, static_cast<u8>(b));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(a3));
    ctx.gpr[5] = active;
    ctx.gpr[4] = rec;
    ret(ctx, b);
}

// void SplineInstance_Changed(SplineInstance* o)  (slot +36)
// The peer copy, then SplineInstance_SyncRecord.
DECOMP(005A79A4, SplineInstance_Changed) {
    const u32 o = arg<u32>(ctx, 0);
    CopyAnchor(o);
    call(ctx, o7fn::SplineInstance_SyncRecord, {o});
}

// SplineInstance* SplineInstance_Clone(const SplineInstance* o, u32 flags)  (slot +56)
// kCloneOwnClass: a new instance of a copy of the class (the copy's
// reference released; base state copied without the class), otherwise of
// the same class. The record's mode (+94) goes to 0x005A7B38; children
// cloned with kCloneChildren; an unparented clone is added to the world
// unless kCloneNoWorld.
DECOMP(005A7E80, SplineInstance_Clone) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 flags = ctx.gpr[4];
    const u32 cl = load<u32>(o + kObjClassPtr);
    u32 n;
    if ((flags & kCloneOwnClass) != 0) {
        const u32 copy = static_cast<u32>(call(ctx, o7fn::SplineDef_Copy, {cl}));
        n = static_cast<u32>(call(ctx, o7fn::SplineInstance_New, {copy}));
        Release(ctx, copy);
        call(ctx, obj5::o5fn::WorldObj_CopyBase, {n, o, 1});
    } else {
        n = static_cast<u32>(call(ctx, o7fn::SplineInstance_New, {cl}));
        call(ctx, obj5::o5fn::WorldObj_CopyBase, {n, o, 0});
    }
    const u32 rec = load<u32>(kSplineList + 12) + load<u32>(o + kObjSplineIndex) * obj5::kK13RecSize;
    call(ctx, o7fn::SplineInstance_SetMode, {n, load<u8>(rec + kSrMode)});
    if ((flags & kCloneChildren) != 0) cls::CloneChildren(ctx, o, n, static_cast<u32>(flags));
    if (load<u32>(o + kObjParentLink) == 0 && (flags & kCloneNoWorld) == 0)
        call(ctx, cls::ifn::WorldObj_AddToWorld, {n});
    ret(ctx, n);
}

// void SplineInstance_Dump(SplineInstance* o, s32 depth, u32 flags)  (slot +60)
DECOMP(005A8388, SplineInstance_Dump) {
    call(ctx, cls::ifn::WorldObj_DumpTree, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5]});
}
