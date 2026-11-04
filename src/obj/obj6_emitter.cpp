// World objects, part 6: emitter properties (AglEmitProperties, the
// ref-counted data an ApeEmitterDef shares) and new emitter defs.
#include "obj6.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj6;

namespace {

// The inlined release of an AglEmitProperties: drop a reference (u16 at
// +0); at zero destroy it and give it back to its free list.
void PropsRelease(PPUContext& ctx, u32 p) {
    const u16 refs = static_cast<u16>(load<u16>(p) - 1);
    store<u16>(p, refs);
    if (static_cast<s16>(refs) > 0) return;
    call(ctx, o6fn::EmitProps_Destruct, {p});
    const u32 alloc = ListAllocator(ctx, kEmitPropsList, kEmitPropsListVtbl, kStrEmitPropsList, kEmitPropsSize);
    ListFree(ctx, alloc, p);
}

// Points *slot at v, a sub-object with a u16 reference count at +4 (a
// reference taken; the old one dropped and destroyed through its vfunc +4
// at zero). Returns what *slot holds afterwards, as the original reads it.
u32 SubRefSet(PPUContext& ctx, u32 slot, u32 v) {
    const u32 old = load<u32>(slot);
    if (old == v) return old;
    store<u32>(slot, v);
    if (v != 0) store<u16>(v + kSubRefs, static_cast<u16>(load<u16>(v + kSubRefs) + 1));
    if (old == 0) return v;
    const u16 refs = static_cast<u16>(load<u16>(old + kSubRefs) - 1);
    store<u16>(old + kSubRefs, refs);
    if (static_cast<s16>(refs) > 0) return v;
    const u32 opd = load<u32>(load<u32>(old) + kVfnSubDestroy);
    call_ptr(ctx, opd, {old, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
    return load<u32>(slot);
}

}  // namespace

// void EmitterData_Release(AglEmitProperties* p)
// Drops a reference; the last one destroys p (0x004FFA98) and gives it
// back to the AglEmitProperties free list (*0x00F8F128, made on first use).
DECOMP(005003F4, EmitterData_Release) {
    PropsRelease(ctx, arg<u32>(ctx, 0));
}

// AglEmitProperties** EmitPropsRef_Set(AglEmitProperties** slot, AglEmitProperties* p)
// The ref-counted pointer assignment: nothing when *slot is already p;
// otherwise *slot = p (a reference taken) and the old one released.
// Returns slot.
DECOMP(0079CB94, EmitPropsRef_Set) {
    const u32 slot = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 old = load<u32>(slot);
    if (old != p) {
        store<u32>(slot, p);
        if (p != 0) store<u16>(p, static_cast<u16>(load<u16>(p) + 1));
        if (old != 0) PropsRelease(ctx, old);
    }
    ret(ctx, slot);
}

// AglEmitProperties* EmitterData_Copy(AglEmitProperties* src, bool copy_curve)
// A new AglEmitProperties from the free list, constructed (0x004FFFF0, not
// when the list is empty) and assigned from src (0x004FE3DC, even with a
// null copy). The *source* then gets private sub-objects: its +108 is
// replaced by that object's clone (vfunc +12), and with copy_curve its
// +112 by a fresh 84-byte copy (vtable 0x00B23200, +6 / +8 / +12 / +16 and
// the 64 bytes at +20). Each new sub-object came with one reference and
// gained one when stored, so one is dropped (no null checks: a failed
// SmallAlloc copies to address 20). Returns the new properties.
DECOMP(005000C8, EmitterData_Copy) {
    const u32 src = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 1);
    const u32 alloc = ListAllocator(ctx, kEmitPropsList, kEmitPropsListVtbl, kStrEmitPropsList, kEmitPropsSize);
    const u32 got = static_cast<u32>(Vcall0(ctx, alloc, 12));
    u32 n = 0;
    if (got != 0) {
        call(ctx, o6fn::EmitProps_Construct, {got});
        n = got;
    }
    call(ctx, o6fn::EmitProps_CopyFrom, {n, src});
    const u32 clone = static_cast<u32>(Vcall0(ctx, load<u32>(src + kEpShape), kVfnSubClone));
    const u32 shape = SubRefSet(ctx, src + kEpShape, clone);
    store<u16>(shape + kSubRefs, static_cast<u16>(load<u16>(shape + kSubRefs) - 1));
    if ((flags & 1) == 0) return ret(ctx, n);
    const u32 from = load<u32>(src + kEpCurve);
    u32 c = static_cast<u32>(call(ctx, o6fn::SmallAlloc, {kEmitCurveSize}));
    if (c != 0) {
        store<u16>(c + kSubRefs, 1);
        store<u32>(c, kVtblEmitCurve);
        store<u8>(c + 6, 0);
        store<u16>(c + 8, 0);
        store<u32>(c + 12, kCurveBig);
        store<u32>(c + 16, kCurveBig);
    }
    call(ctx, o6fn::memmove, {c + 20, from + 20, 64});
    store<u8>(c + 6, load<u8>(from + 6));
    store<u32>(c + 12, load<u32>(from + 12));
    store<u32>(c + 16, load<u32>(from + 16));
    store<u16>(c + 8, load<u16>(from + 8));
    SubRefSet(ctx, src + kEpCurve, c);
    const u32 cur = load<u32>(src + kEpCurve);
    store<u16>(cur + kSubRefs, static_cast<u16>(load<u16>(cur + kSubRefs) - 1));
    ret(ctx, n);
}

// ApeEmitterDef* EmitterDef_New(AglEmitProperties* data, EmitterSystem* system)
// A new ApeEmitterDef from its free list (*0x00F8F138, made on first use;
// 0x0058DF94 constructs it) with +112 = data (a reference taken, the old
// one released) and +116 = system (no reference; the old one's vfunc +24),
// added to the def table (*0x0119457C, grown by 128 when full): its count
// grows, the optional +8 array and the +12 array get the def, and the def
// keeps its index at +122. Quirks kept: a failed list allocation leaves a
// null def that is still written to, and a table that stays full gives
// index -1 (the +12 array's word before the first is written).
DECOMP(0058E140, EmitterDef_New) {
    const u32 data = arg<u32>(ctx, 0), sys = arg<u32>(ctx, 1);
    const u32 alloc = ListAllocator(ctx, kEmitterDefList, kEmitterDefListVtbl, kStrEmitterDefList, kEmitterDefSize);
    const u32 got = static_cast<u32>(Vcall0(ctx, alloc, 12));
    u32 d = 0;
    if (got != 0) {
        call(ctx, o6fn::EmitterDef_Construct, {got});
        d = got;
    }
    const u32 od = load<u32>(d + obj5::kEdData);
    if (od != data) {
        store<u32>(d + obj5::kEdData, data);
        if (data != 0) store<u16>(data, static_cast<u16>(load<u16>(data) + 1));
        if (od != 0) call(ctx, obj5::o5fn::EmitterData_Release, {od});
    }
    const u32 os = load<u32>(d + obj5::kEdSystem);
    if (os != sys) {
        store<u32>(d + obj5::kEdSystem, sys);
        if (os != 0) Vcall0(ctx, os, kVfnSystemRelease);
    }
    // Takes the next index when the table has room: the +8 array (if any)
    // gets d, the count grows. -1 when full.
    auto take = [&]() -> u32 {
        u32 t = load<u32>(kEmitterDefTable);
        u32 h = load<u32>(t + 4);
        const u32 n = load<u16>(h + 2);
        if (static_cast<s32>(n) >= static_cast<s32>(load<u16>(h))) return 0xFFFFFFFFu;
        const u32 arr = load<u32>(t + 8);
        if (arr != 0) {
            store<u32>(arr + ((n << 2) & 0x0003FFFCu), d);
            h = load<u32>(t + 4);
        }
        store<u16>(h + 2, static_cast<u16>(load<u16>(h + 2) + 1));
        return n;
    };
    u32 idx = take();
    if (static_cast<s32>(idx) < 0) {
        call(ctx, o6fn::PtrTable_Grow, {load<u32>(kEmitterDefTable), kTableGrowBy});
        idx = take();
    }
    const u32 t = load<u32>(kEmitterDefTable);
    store<u32>(load<u32>(t + 12) + idx * 4, d);
    store<u16>(d + kEdIndex, static_cast<u16>(idx));
    ret(ctx, d);
}
