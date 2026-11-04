// World objects, part 8: the point / spline constructors, the spline
// definition copy, the spline record list, the light animation set-up,
// the emitter definition reset and the base ApeObj destructor.
#include <bit>

#include "obj8.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj4;
using namespace sly4::obj8;

namespace {

// The ApeTListAllocator<T>::GetAllocator() + Alloc sequence: the list
// behind `holder` (made on first use), then its Alloc (vfunc +12).
u32 ListNew(PPUContext& ctx, u32 holder, u32 vtbl, u32 name, u32 size) {
    const u32 list = obj6::ListAllocator(ctx, holder, vtbl, name, size);
    return static_cast<u32>(Vcall0(ctx, list, kVfnListAlloc));
}

// The point / spline constructors' tail: o->SetClass(def) (vfunc +44;
// r5 / r6 = OPD and entry), then the name (and its hash) from the
// definition's vfunc +12. o is null after a failed allocation, as in the
// original (which then reads through it).
void SetClassAndName(PPUContext& ctx, u32 o, u32 def) {
    const u32 opd = load<u32>(load<u32>(o) + obj5::kVfnSetClass);
    call_ptr(ctx, opd, {o, def, opd, load<u32>(opd)});
    const u32 name = static_cast<u32>(Vcall0(ctx, def, kVfnGetName));
    call(ctx, fn::String_Copy, {o + kObjName, name, 32});
    const u32 h = static_cast<u32>(call(ctx, fn::String_HashNoCase, {name}));
    if (load<u32>(o + kObjNameHash) != h) store<u32>(o + kObjNameHash, h);
}

// Drops a reference to a {u16 refs, u16, data (allocated 16 bytes in)}
// shared block; the last one frees the data and the block.
void ReleaseShared(PPUContext& ctx, u32 s) {
    const u16 refs = static_cast<u16>(load<u16>(s) - 1);
    store<u16>(s, refs);
    if (static_cast<s16>(refs) > 0) return;
    const u32 data = load<u32>(s + 4);
    if (data != 0) {
        call(ctx, cls::ifn::Mem_FreeFrom, {data - 16, 0});
        store<u32>(s + 4, 0);
    }
    call(ctx, cls::ifn::Mem_FreeFrom, {s, 0});
}

inline f32 F(u32 bits) { return std::bit_cast<f32>(bits); }
inline u32 B(f32 v) { return std::bit_cast<u32>(v); }
inline f32 Subs(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - static_cast<f64>(b)); }
inline f32 Muls(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * static_cast<f64>(b)); }

// The game's random generator step (inlined): returns the previous
// 0x01164888 (s64 -> double -> float) and steps the state.
f32 RandomPrev() {
    const auto rotl = [](u32 v, int n) { return (v << n) | (v >> (32 - n)); };
    const u64 last = load<u64>(obj6::kRandomLast);
    const u32 s = load<u32>(obj6::kRandomState);
    const u32 mix = (rotl(s, 4) + rotl(s, 16)) ^ obj6::kRandomMix;
    store<u32>(obj6::kRandomState, rotl(mix, 16));
    store<u64>(obj6::kRandomLast, mix & 0xFFFFu);
    return static_cast<f32>(static_cast<f64>(static_cast<s64>(last)));
}

}  // namespace

// The linear search the property lookups inline: keys are sorted, so the
// search stops at the first larger one (unsigned).
u32 sly4::obj8::PropFind(u32 table, u32 key) {
    const u32 keys = load<u32>(table + kPtKeys);
    const s32 n = load<u16>(table + kPtCount);
    for (s32 i = 0; i < n; ++i) {
        const u32 k = load<u32>(keys + static_cast<u32>(i) * 4);
        if (k == key) {
            const u32 off = load<u16>(load<u32>(table + kPtOffsets) + static_cast<u32>(i) * 2);
            return load<u32>(table + kPtData) + off;
        }
        if (k > key) break;
    }
    return 0;
}

// void ApeObj_Destruct(ApeObj* o)  (the base vtable 0x00B24610, slot +0)
// Off the dirty list when queued (the last entry moved into its place),
// then the peers, the class and the root destructor.
DECOMP(0057DCF0, ApeObj_Destruct) { obj7::ApeObjDestructBody(ctx, arg<u32>(ctx, 0)); }

// ApePointInstance* PointInstance_New(ApePointDef* def)
// A kind-12 point (288 bytes) from the list behind *0x00F8F154 (the
// WorldObj base constructor inlined: one reference, kind 12, a new serial,
// the transform), then set to the definition and named after it. A failed
// allocation goes on with a null object, as in the original.
DECOMP(005A68EC, PointInstance_New) {
    const u32 def = arg<u32>(ctx, 0);
    const u32 m = ListNew(ctx, obj5::kMapHolderPoint, obj5::kPointInstListVtbl, obj5::kStrPointInstList,
                          obj5::kPointInstanceSize);
    u32 o = 0;
    if (m != 0) {
        const u32 w4 = load<u32>(m + obj2::kObjWord4);
        store<u16>(m + obj2::kObjWord4, 1);
        store<u32>(m + obj2::kObjWord4, (w4 & 0x3FFu) | 0x00010000u | (obj5::kKindPoint << 10));
        const u32 serial = load<u32>(obj2::kObjSerial) + 1;
        store<u32>(obj2::kObjSerial, serial);
        u32 w8 = load<u32>(m + obj2::kObjWord8);
        store<u32>(m, obj3::kVtblBase1);
        w8 = (serial << 11) | (w8 & 0x3Fu);  // bits 0x400 / 0x3C0 cleared
        store<u8>(m + kObjName, 0);
        store<u32>(m + 276, 0);
        store<u32>(m + 280, 0);
        store<u32>(m + obj6::kObjComps, 0);
        store<u32>(m + obj2::kObjWord8, w8);
        call(ctx, obj2::o2fn::WorldObj_InitTransform, {m, w8});  // r4 as in the original (not read)
        store<u32>(m, obj5::kVtblPointInstance);
        o = m;
    }
    SetClassAndName(ctx, o, def);
    ret(ctx, o);
}

// ApeSplineInstance* SplineInstance_New(ApeSplineDef* def)
// A kind-13 spline (288 bytes) from the list behind *0x00F8F158
// (Kind13_Construct), then set to the definition and named after it.
DECOMP(005A7CB4, SplineInstance_New) {
    const u32 def = arg<u32>(ctx, 0);
    const u32 m = ListNew(ctx, kSplineInstList, kSplineInstListVtbl, kStrSplineInstList, kSplineInstanceSize);
    u32 o = 0;
    if (m != 0) {
        call(ctx, o8fn::Kind13_Construct, {m});
        o = m;
    }
    SetClassAndName(ctx, o, def);
    ret(ctx, o);
}

// ApeSplineDef* SplineDef_Copy(const ApeSplineDef* src)
// A new definition (128 bytes from the list behind *0x00F8F140) with
// src's box / sphere words, name, +96..+104, shared block (+108, reference
// counted) and points (+112 / +116: a fresh array of the same size, the
// points copied); +124 cleared and +120 copied. A failed allocation goes
// on with a null definition, as in the original.
DECOMP(0059F248, SplineDef_Copy) {
    const u32 src = arg<u32>(ctx, 0);
    const u32 m = ListNew(ctx, kSplineDefList, kSplineDefListVtbl, kStrSplineDefList, kSplineDefSize);
    u32 d = 0;
    if (m != 0) {
        call(ctx, o8fn::SplineDef_Construct, {m});
        d = m;
    }
    for (u32 off : {16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u, 60u}) store<u32>(d + off, load<u32>(src + off));
    call(ctx, o8fn::String_Assign, {d + kSdName, src + kSdName});
    for (u32 off : {96u, 100u, 104u}) store<u32>(d + off, load<u32>(src + off));
    const u32 old = load<u32>(d + kSdShared);
    const u32 nw = load<u32>(src + kSdShared);
    if (old != nw) {
        store<u32>(d + kSdShared, nw);
        if (nw != 0) store<u16>(nw, static_cast<u16>(load<u16>(nw) + 1));
        if (old != 0) ReleaseShared(ctx, old);
    }
    if (d + kSdData != src + kSdData) {
        u32 w = load<u32>(src + kSdData);
        const u32 top = w >> 31;
        const u32 count = ((w >> 15) & 0x7FFF) - top;  // one fewer with bit 31
        call(ctx, o8fn::SplineArray_Init, {d + kSdData, count, top, (w >> 30) & 1});
        w = load<u32>(src + kSdData);
        if (((w >> 15) & 0x7FFF) != 0) {
            u32 i = 0, off = 0;
            do {
                const u32 dp = load<u32>(d + kSdPoints) + off, sp = load<u32>(src + kSdPoints) + off;
                for (u32 k : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u}) store<u32>(dp + k, load<u32>(sp + k));
                ++i;
                off += kSdPointSize;
                w = load<u32>(src + kSdData);
            } while (i < ((w >> 15) & 0x7FFF));
        }
        // The low 15 bits are taken from src (not the count bits).
        store<u32>(d + kSdData, (load<u32>(d + kSdData) & 0xFFFF8000u) | (w & 0x7FFFu));
        const u32 f120 = load<u32>(src + 120);
        store<u32>(d + 124, 0);
        store<u32>(d + 120, f120);
    }
    ret(ctx, d);
}

// u8 SplineInstance_SetMode(ApeSplineInstance* o, u8 mode)
// Refreshes o's record (+284, not checked against -1) from the record's
// owner like SplineInstance_SyncRecord, then stores the mode at +94.
// Leaves r5 = the record, r6 = mode, r7 = the active bit and f1 = the
// last float copied, as the original does.
DECOMP(005A7B38, SplineInstance_SetMode) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 mode = ctx.gpr[4];
    const u32 rec = load<u32>(obj7::kSplineList + kSlRecs) + load<u32>(o + obj7::kObjSplineIndex) * obj5::kK13RecSize;
    const u32 ow = load<u32>(rec + obj7::kSrOwner);
    const u32 w = ow + obj6::kObjWorld;
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(rec + row + k, load<u32>(w + row + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(rec + 64 + k, load<u32>(ow + kObjAnchor + k));
    const u32 a3 = load<u32>(ow + kObjAnchor + 12);
    u32 data = load<u32>(rec + obj7::kSrData);
    store<u32>(rec + 76, a3);
    const u32 cl = load<u32>(ow + kObjClassPtr);
    u32 bits = load<u8>(rec + obj7::kSrBits);
    if (cl != 0) {
        data = cl + obj7::kSplineDefData;
        store<u32>(rec + obj7::kSrData, data);
    }
    if (data != 0) {
        store<u32>(rec + obj7::kSrData4, load<u32>(data + 4));
        store<u16>(rec + obj7::kSrCount, static_cast<u16>(load<u32>(data) & 0x7FFF));
    }
    u32 active = 0;
    if ((load<u32>(ow + kObjFlags) & kObjFlagVisible) != 0) {
        const u32 p = load<u32>(ow + kObjParentLink);
        if (p == 0) {
            active = 1;
        } else if (Kind(p) == obj5::kKind13) {
            active = (bits & obj7::kSrFollowParent) != 0 ? 1 : 0;
        } else {
            active = (load<u32>(p + kObjFlags) & kObjFlagVisible) != 0 ? 1 : 0;
        }
    }
    bits = (bits & ~obj7::kSrActive & 0xFF) | (active << 7);
    store<u8>(rec + obj7::kSrBits, static_cast<u8>(bits));
    store<u8>(rec + obj7::kSrMode, static_cast<u8>(mode));
    ctx.fpr[1] = static_cast<f64>(F(a3));
    ctx.gpr[5] = rec;
    ctx.gpr[6] = mode;
    ctx.gpr[7] = active;
    ret(ctx, bits);
}

// u16 SplineList_Remove(SplineList* list, s32 idx)
// Removes record idx: the last record moves into its place (with its
// object pointer when the list keeps them), the list's moved callback
// (header +8, when set) runs for the object now at idx, and the count
// drops. With one record or none the count is just cleared.
DECOMP(007A9128, SplineList_Remove) {
    const u32 list = arg<u32>(ctx, 0);
    const u64 idx = ctx.gpr[4];
    const u32 i = static_cast<u32>(idx);
    u32 hdr = load<u32>(list + kSlHeader);
    const u32 count = load<u16>(hdr + 2);
    if (static_cast<s32>(count) <= 1) {
        store<u16>(hdr + 2, 0);
        ret(ctx, 0u);
        return;
    }
    const u32 last = count - 1;
    const u32 recs = load<u32>(list + kSlRecs);
    const u32 d = recs + i * obj5::kK13RecSize, s = recs + last * obj5::kK13RecSize;
    for (u32 k = 0; k < 80; k += 4)
        if ((k & 15) != 12 || k >= 64) store<u32>(d + k, load<u32>(s + k));
    for (u32 k : {80u, 84u, 88u}) store<u32>(d + k, load<u32>(s + k));
    u32 b = load<u8>(d + 95);
    store<u16>(d + 92, load<u16>(s + 92));
    store<u8>(d + 94, load<u8>(s + 94));
    b = (b & ~0x80u) | (load<u8>(s + 95) & 0x80u);
    store<u8>(d + 95, static_cast<u8>(b));
    b = (b & ~0x40u) | (load<u8>(s + 95) & 0x40u);
    store<u8>(d + 95, static_cast<u8>(b));
    u32 ptrs = load<u32>(list + kSlPtrs);
    if (ptrs != 0) {
        store<u32>(ptrs + i * 4, load<u32>(ptrs + last * 4));
        ptrs = load<u32>(list + kSlPtrs);
    }
    hdr = load<u32>(list + kSlHeader);
    const u32 cb = load<u32>(hdr + 8);
    if (ptrs != 0 && cb != 0) {
        const u32 obj = load<u32>(ptrs + i * 4);
        if (obj != 0) {
            call_ptr(ctx, cb, {obj, idx, load<u32>(cb), cb});  // r5 = entry, r6 = OPD
            hdr = load<u32>(list + kSlHeader);
        }
    }
    const u64 n = static_cast<u64>(static_cast<s64>(load<u16>(hdr + 2)) - 1);
    store<u16>(hdr + 2, static_cast<u16>(n));
    ctx.gpr[3] = n;
}

// void LightAnim_Setup(LightAnimRecord* rec, Comps* comps, s32 slot, s32 light_type)
// Fills an animation record from the properties of component `slot`: the
// type, base, peak (stored as peak - base), period (a zero period zeroes
// the base), noise, and a random start phase (rand * 1/65535 * period).
// Then the range terms at +0 / +4 / +8: (10000, 10000^2, 10000^2 - 1) for
// light types 0 / 1; otherwise r = the range property (25 by default, the
// lookup skipped for a negative slot): (r, r^2, (r - 5)^2).
DECOMP(00591410, LightAnim_Setup) {
    const u32 rec = arg<u32>(ctx, 0), comps = arg<u32>(ctx, 1);
    const s32 slot = arg<s32>(ctx, 2), lt = arg<s32>(ctx, 3);
    const u32 table = load<u32>(comps + static_cast<u32>(slot) * 4 + 4);
    const u32 a = rec + 32;
    u32 v;
    if ((v = PropFind(table, kKeyAnimType)) != 0) store<u32>(a + 8, load<u32>(v));
    if ((v = PropFind(table, kKeyAnimBase)) != 0) store<u32>(a + 28, load<u32>(v));
    if ((v = PropFind(table, kKeyAnimPeak)) != 0) store<f32>(a + 32, Subs(load<f32>(v), load<f32>(a + 28)));
    if ((v = PropFind(table, kKeyAnimPeriod)) != 0) {
        const u32 p = load<u32>(v);
        store<u32>(a + 4, p);
        if (F(p) == 0.0f) store<u32>(a + 28, 0);
    }
    if ((v = PropFind(table, kKeyAnimNoise)) != 0) store<u32>(a + 16, load<u32>(v));
    if ((v = PropFind(table, kKeyAnimRandomPhase)) != 0 && load<u8>(v) != 0) {
        const f32 c = load<f32>(kLsConsts + 20);
        const f32 period = load<f32>(a + 4);
        const f32 r = RandomPrev();
        store<f32>(a, Muls(Muls(r, c), period));
    }
    if (lt == 1 || lt == 0) {
        const f32 r = load<f32>(kLsConsts + 12);
        const f32 r2 = Muls(r, r);
        store<f32>(rec, r);
        store<f32>(rec + 4, r2);
        store<f32>(rec + 8, Subs(r2, load<f32>(kLsConsts + 16)));
        return;
    }
    u32 r = load<u32>(kLsConsts);
    if (slot >= 0 && (v = PropFind(table, kKeyLightRange)) != 0) r = load<u32>(v);
    const f32 d = Subs(F(r), load<f32>(kLsConsts + 4));
    store<u32>(rec, r);
    store<f32>(rec + 4, Muls(F(r), F(r)));
    store<f32>(rec + 8, Muls(d, d));
}

// void EmitterDef_Init(ApeEmitterDef* def)
// Resets a definition: the box to the origin point, the default sphere,
// +96..+104 and +64 cleared, the shared block (+108), the emitter data
// (+112) and the particle system (+116, vfunc +24) released, +120 cleared.
DECOMP(0058DE4C, EmitterDef_Init) {
    const u32 d = arg<u32>(ctx, 0);
    call(ctx, cls::cfn::Box_SetPoint, {d + 16, cls::kOrigin});
    store<u32>(d + 48, load<u32>(cls::kDefaultSphere));
    store<u32>(d + 52, load<u32>(cls::kDefaultSphere + 4));
    store<u32>(d + 56, load<u32>(cls::kDefaultSphere + 8));
    const u32 sh = load<u32>(d + kSdShared);
    store<u32>(d + 96, 0);
    store<u32>(d + 60, load<u32>(cls::kDefaultSphere + 12));
    store<u32>(d + 100, 0);
    store<u8>(d + 64, 0);
    store<u32>(d + 104, 0);
    if (sh != 0) {
        store<u32>(d + kSdShared, 0);
        ReleaseShared(ctx, sh);
    }
    const u32 data = load<u32>(d + obj5::kEdData);
    if (data != 0) {
        store<u32>(d + obj5::kEdData, 0);
        call(ctx, obj5::o5fn::EmitterData_Release, {data});
    }
    const u32 sys = load<u32>(d + obj5::kEdSystem);
    if (sys != 0) {
        store<u32>(d + obj5::kEdSystem, 0);
        Vcall0(ctx, sys, obj6::kVfnSystemRelease);
    }
    store<u8>(d + 120, 0);
    ret(ctx, 0u);
}
