// Instance construction for the light / emitter / camera classes: each has
// its own free list (made on first use) behind one of the class map
// holders, and names itself after its definition.
#include "obj5.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj4;
using namespace sly4::obj5;

namespace {

// The ApeTListAllocator<T>::GetAllocator() + Alloc sequence: the list
// behind `holder` (a 60-byte ApeTFreeList made on first use, `elem`-byte
// entries, the header's +4 capacity), then its +12 vfunc. As in the
// original, a failed list allocation stores a null allocator and calls it.
u32 ListAlloc(PPUContext& ctx, u32 holder, u32 elem, u32 list_vtbl, u32 str) {
    const u32 header = load<u32>(holder);
    u32 list = load<u32>(header);
    if (list == 0) {
        const u32 obj = static_cast<u32>(call(ctx, o4fn::Mem_Alloc, {60, 0, 16}));
        if (obj != 0) {
            const u32 capacity = load<u32>(header + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
            const u16 h54 = load<u16>(obj + 54);
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
            store<u16>(obj + 52, 0);
            store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(obj, list_vtbl);
            store<u32>(obj + 56, str);
            // r8 / r9 = the vtable and the string here; FreeList_Init reads r3..r7 only.
            call(ctx, o4fn::FreeList_Init, {obj, elem, capacity, 0, 0});
        }
        list = obj;
        store<u32>(header, list);
    }
    return static_cast<u32>(Vcall0(ctx, list, 12));
}

// o->SetClass(def) (vfunc +44; r5 / r6 = OPD and entry).
inline void SetClass(PPUContext& ctx, u32 o, u32 def) {
    const u32 opd = load<u32>(load<u32>(o) + kVfnSetClass);
    call_ptr(ctx, opd, {o, def, opd, load<u32>(opd)});
}

// The light / emitter tail: SetClass, the name (and its hash) from the
// definition's vfunc +12, +292 from def +104 (the AglLight's +212 bit 0x80
// cleared), and an AglLight when +292 is set and there is none (then
// vfunc +36).
void NameAndLight(PPUContext& ctx, u32 o, u32 def) {
    SetClass(ctx, o, def);
    const u32 name = static_cast<u32>(Vcall0(ctx, def, kVfnGetName));
    call(ctx, fn::String_Copy, {o + kObjName, name, 32});
    const u32 h = static_cast<u32>(call(ctx, fn::String_HashNoCase, {name}));
    const u32 light = load<u32>(o + kObjLight);
    if (load<u32>(o + kObjNameHash) != h) store<u32>(o + kObjNameHash, h);
    store<u32>(o + kObj292, load<u32>(def + 104));
    if (light != 0) store<u8>(light + 212, static_cast<u8>(load<u8>(light + 212) & 0x7F));
    if (load<u32>(o + kObj292) != 0 && load<u32>(o + kObjLight) == 0) {
        store<u32>(o + kObjLight, static_cast<u32>(call(ctx, o5fn::Light_Alloc)));
        Vcall0(ctx, o, kVfnChanged);
    }
}

}  // namespace

// LightInstance* LightInstance_New(ApeLightDef* def)
// A kind-9 light (400 bytes) from the list behind *0x00F8F148 with the
// default light parameters (type 3, white, 1, 1.0, 127, {0, 1.0}, no
// record), then set to the definition and named after it. A failed
// allocation goes on with a null object, as in the original.
DECOMP(005A2BA8, LightInstance_New) {
    const u32 def = arg<u32>(ctx, 0);
    const u32 m = ListAlloc(ctx, kMapHolderC, kLightInstanceSize, kLightInstListVtbl, kStrLightInstList);
    u32 o = 0;
    if (m != 0) {
        call(ctx, o5fn::WorldObj_Construct, {m, 9});
        store<u32>(m + cls::kLightType, 3);
        store<u32>(m, cls::kVtblLightInstance);
        store<u16>(m + cls::kLightRec, 0xFFFF);
        for (u32 k = 0; k < 12; k += 4) store<u32>(m + cls::kLightColor + k, load<u32>(kOneVec3 + k));
        store<u32>(m + cls::kLight372, 1);
        store<u32>(m + cls::kLightRangeSq, cls::kOneF);
        store<u32>(m + cls::kLight384, 0);
        store<u8>(m + cls::kLight380, 127);
        store<u32>(m + cls::kLight384 + 4, cls::kOneF);
        o = m;
    }
    NameAndLight(ctx, o, def);
    ret(ctx, o);
}

// EmitterInstance* EmitterInstance_New(ApeEmitterDef* def, bool copy_def)
// A kind-10 emitter (352 bytes) from the list behind *0x00F8F14C. With
// copy_def, or a world-attached definition (data +42 bit 0x80), it gets
// its own copy of the definition (released at the end). Then set to the
// definition and named after it.
DECOMP(005A3FFC, EmitterInstance_New) {
    u32 def = arg<u32>(ctx, 0);
    const u32 copy = arg<u32>(ctx, 1) & 0xFF;
    const u32 m = ListAlloc(ctx, kMapHolderD, kEmitterInstanceSize, kEmitterInstListVtbl, kStrEmitterInstList);
    u32 o = 0;
    if (m != 0) {
        call(ctx, o5fn::WorldObj_Construct, {m, 10});
        store<u32>(m + kEmitter340, 0);
        store<u32>(m, kVtblEmitterInstance);
        o = m;
    }
    bool owns = false;
    if (def != 0 && (copy != 0 || (load<u8>(load<u32>(def + 112) + 42) & 0x80) != 0)) {
        def = static_cast<u32>(call(ctx, o5fn::EmitterDef_Copy, {def, 0}));
        owns = true;
    }
    NameAndLight(ctx, o, def);
    if (owns) Release(ctx, def);
    ret(ctx, o);
}

// CameraInstance* CameraInstance_New(ApeCameraDef* def)
// A kind-11 camera (352 bytes) from the list behind *0x00F8F150, set to
// the definition (no name).
DECOMP(005A59AC, CameraInstance_New) {
    const u32 def = arg<u32>(ctx, 0);
    const u32 m = ListAlloc(ctx, kMapHolderA, kCameraInstanceSize, kCameraInstListVtbl, kStrCameraInstList);
    u32 o = 0;
    if (m != 0) {
        call(ctx, o5fn::WorldObj_Construct, {m, 11});
        store<u32>(m, kVtblCameraInstance);
        o = m;
    }
    SetClass(ctx, o, def);
    ret(ctx, o);
}

// ApeEmitterDef* EmitterDef_Copy(const ApeEmitterDef* def, u32 flags)
// A new definition (0x0058E140(0, 0)) with def's box / sphere words
// (+16..+60), name (+64), +96..+104 and shared block (+108, reference
// counted; the old one freed with its last reference), a copy of the
// emitter data (+112, 0x005000C8 with flags bit 0) and a clone of the
// particle system (+116, its vfunc +20), the old ones released; then
// 0x00500714(system, data). Quirks kept: the data's reference count is
// dropped without a null check, and the name vfunc (+12) is fetched only
// when there is data.
DECOMP(0058E3E4, EmitterDef_Copy) {
    const u32 def = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 1);
    const u32 n = static_cast<u32>(call(ctx, o5fn::EmitterDef_New, {0, 0}));
    for (u32 off : {16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u, 60u}) store<u32>(n + off, load<u32>(def + off));
    call(ctx, obj2::o2fn::String_Assign, {n + obj2::kClassName, def + obj2::kClassName});
    const u32 shared = load<u32>(def + obj2::kClassShared);
    const u32 old = load<u32>(n + obj2::kClassShared);
    for (u32 off : {96u, 100u, 104u}) store<u32>(n + off, load<u32>(def + off));
    if (old != shared) {
        store<u32>(n + obj2::kClassShared, shared);
        if (shared != 0) store<u16>(shared, static_cast<u16>(load<u16>(shared) + 1));
        if (old != 0) {
            const u16 refs = static_cast<u16>(load<u16>(old) - 1);
            store<u16>(old, refs);
            if (static_cast<s16>(refs) <= 0) {
                const u32 data = load<u32>(old + 4);
                if (data != 0) {
                    call(ctx, cls::ifn::Mem_FreeFrom, {data - 16, 0});
                    store<u32>(old + 4, 0);
                }
                call(ctx, cls::ifn::Mem_FreeFrom, {old, 0});
            }
        }
    }
    const u32 d = static_cast<u32>(call(ctx, o5fn::EmitterData_Copy, {load<u32>(def + kEdData), (flags & 1) != 0 ? 1u : 0u}));
    const u32 od = load<u32>(n + kEdData);
    u32 cur = od;
    if (od != d) {
        store<u32>(n + kEdData, d);
        cur = d;
        if (d != 0) store<u16>(d, static_cast<u16>(load<u16>(d) + 1));
        if (od != 0) {
            call(ctx, o5fn::EmitterData_Release, {od});
            cur = load<u32>(n + kEdData);
        }
    }
    if (cur != 0) {
        Vcall0(ctx, n, kVfnGetName);
        cur = load<u32>(n + kEdData);
    }
    store<u16>(cur, static_cast<u16>(load<u16>(cur) - 1));  // no null check (as the original)
    const u32 sys = static_cast<u32>(Vcall0(ctx, load<u32>(def + kEdSystem), 20));
    const u32 os = load<u32>(n + kEdSystem);
    u32 s = os;
    if (os != sys) {
        store<u32>(n + kEdSystem, sys);
        s = sys;
        if (os != 0) {
            Vcall0(ctx, os, 24);
            s = load<u32>(n + kEdSystem);
        }
    }
    call(ctx, o5fn::EmitterSystem_Bind, {s, load<u32>(n + kEdData)});
    ret(ctx, n);
}

// void Kind13_Construct(Obj* o)  (vtable 0x00B249E8; class unidentified)
// The first half of the base constructor (kind 13, a new serial, the
// transform), then: the object takes the next entry of the list at
// 0x01194580 (its +8 array, when there is one, gets o; the u16 count at
// header +2 grows) and a fresh 96-byte record there (identity rows,
// default sphere, owner o at +88, +95 bits 0xC0 cleared); +284 is a 32-bit
// index here. Quirk kept: a full list gives index -1 and the record 96
// bytes before the records is written.
DECOMP(005A74A8, Kind13_Construct) {
    const u32 o = arg<u32>(ctx, 0);
    u32 w4 = load<u32>(o + obj2::kObjWord4);
    store<u16>(o + obj2::kObjWord4, 1);
    w4 = (w4 & 0x0000FFFFu) | 0x00010000u;
    w4 = (w4 & ~0x0000FC00u) | (kKind13 << 10);
    store<u32>(o, obj2::kVtblObjBase0);
    store<u32>(o + obj2::kObjWord4, w4);
    const u32 serial = load<u32>(obj2::kObjSerial) + 1;
    store<u32>(obj2::kObjSerial, serial);
    u32 w8 = load<u32>(o + obj2::kObjWord8);
    store<u8>(o + kObjName, 0);
    w8 = ((serial << 11) | (w8 & 0x7FFu)) & ~0x7C0u;
    store<u32>(o, obj2::kVtblObjBase1);
    store<u32>(o + 276, 0);
    store<u32>(o + 280, 0);
    store<u32>(o + 12, 0);
    store<u32>(o + obj2::kObjWord8, w8);
    call(ctx, obj2::o2fn::WorldObj_InitTransform, {o});
    store<u32>(o, kVtblKind13);
    store<u32>(o + obj2::kObjWord284, 0xFFFFFFFFu);
    u32 idx = 0xFFFFFFFFu;
    u32 h = load<u32>(kK13List + 4);
    const u32 n = load<u16>(h + 2);
    if (static_cast<s32>(n) < static_cast<s32>(load<u16>(h))) {
        const u32 arr = load<u32>(kK13List + 8);
        if (arr != 0) {
            store<u32>(arr + ((n << 2) & 0x0003FFFCu), o);
            h = load<u32>(kK13List + 4);
        }
        store<u16>(h + 2, static_cast<u16>(load<u16>(h + 2) + 1));
        idx = n;
    }
    const u32 rec = load<u32>(kK13List + 12) + idx * kK13RecSize;
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(rec + row + k, load<u32>(0x01164BD0 + row + k));
    for (u32 k = 0; k < 16; k += 4) store<u32>(rec + 64 + k, load<u32>(cls::kDefaultSphere + k));
    const u8 b95 = load<u8>(rec + 95);
    store<u32>(rec + 80, 0);
    store<u32>(rec + 84, 0);
    store<u32>(rec + 88, o);
    store<u16>(rec + 92, 0);
    store<u8>(rec + 94, 0);
    store<u8>(rec + 95, static_cast<u8>(b95 & 0x3F));
    store<u32>(o + obj2::kObjWord284, idx);
    ret(ctx, rec);
}

// bool PointInstance_Create(const void* src, Obj* parent)  (called through a pointer; ApePointInstance)
// A kind-12 ApePointInstance (288 bytes, vtable 0x00B24998, the base
// constructor's first half and the transform only) from the list behind
// *0x00F8F154. The holder's callback (+8, with +12) runs before (0) and
// after (1) 0x00569388(src, o) fills it in. Then it becomes parent's child
// (vfunc +20, tag -1) or, without a parent, is moved by the offset at
// 0x011944E0 and marked dirty; finally it goes into the holder's map (+16)
// under its name hash (the map then needs sorting) and gets +8 bit 0x400.
// Returns 1. A failed allocation goes on with a null object, as in the
// original.
DECOMP(005A6C48, PointInstance_Create) {
    const u32 src = arg<u32>(ctx, 0), parent = arg<u32>(ctx, 1);
    const u32 m = ListAlloc(ctx, kMapHolderPoint, kPointInstanceSize, kPointInstListVtbl, kStrPointInstList);
    u32 o = 0;
    if (m != 0) {
        u32 w4 = load<u32>(m + obj2::kObjWord4);
        w4 = (w4 & 0x0000FFFFu) | 0x00010000u;
        w4 = (w4 & ~0x0000FC00u) | (kKindPoint << 10);
        store<u16>(m + obj2::kObjWord4, 1);
        store<u32>(m + obj2::kObjWord4, w4);
        const u32 serial = load<u32>(obj2::kObjSerial) + 1;
        store<u32>(obj2::kObjSerial, serial);
        u32 w8 = load<u32>(m + obj2::kObjWord8);
        store<u32>(m, obj2::kVtblObjBase1);
        store<u8>(m + kObjName, 0);
        w8 = ((serial << 11) | (w8 & 0x7FFu)) & ~0x7C0u;
        store<u32>(m + 276, 0);
        store<u32>(m + 280, 0);
        store<u32>(m + 12, 0);
        store<u32>(m + obj2::kObjWord8, w8);
        call(ctx, obj2::o2fn::WorldObj_InitTransform, {m});
        store<u32>(m, kVtblPointInstance);
        o = m;
    }
    auto notify = [&](u32 phase) {
        const u32 h = load<u32>(kMapHolderPoint);
        const u32 cb = load<u32>(h + 8);
        if (cb != 0) call_ptr(ctx, cb, {o, load<u32>(h + 12), phase, cb, load<u32>(cb)});  // r6 / r7 = OPD and entry
    };
    notify(0);
    call(ctx, o5fn::PointInstance_Load, {src, o});
    notify(1);
    if (parent != 0) {
        const u32 opd = load<u32>(load<u32>(parent) + kVfnAddChild);
        call_ptr(ctx, opd, {parent, o, ~0ull, opd, load<u32>(opd)});  // tag -1; r6 / r7 = OPD and entry
    } else {
        const u32 t = o + kObjMatrix + 48;
        const f32 x = load<f32>(t) + load<f32>(kPointOffset);
        const f32 y = load<f32>(t + 4) + load<f32>(kPointOffset + 4);
        const f32 z = load<f32>(t + 8) + load<f32>(kPointOffset + 8);
        store<f32>(t + 4, y);
        store<f32>(t, x);
        store<f32>(t + 8, z);
        MarkDirtyInline(ctx, o);
    }
    const u32 map = load<u32>(load<u32>(kMapHolderPoint) + 16);
    const u32 hash = load<u32>(o + kObjNameHash);
    if (load<u16>(map + kMapCount) == load<u16>(map)) {
        call(ctx, o5fn::Array_Reserve, {map, (static_cast<u32>(load<u16>(map)) << 1) & 0xFFFEu});
    }
    const u32 data = load<u32>(map + kMapEntries);
    store<u32>(data + ((static_cast<u32>(load<u16>(map + kMapCount)) << 3) & ~7u), hash);
    store<u32>(data + ((static_cast<u32>(load<u16>(map + kMapCount)) << 3) & ~7u) + 4, o);
    const u32 n = load<u16>(map + kMapCount) + 1u;
    const u32 hi = load<u16>(map + 4);
    const u32 fl = load<u16>(map + kMapFlags);
    store<u16>(map + kMapCount, static_cast<u16>(n));
    store<u16>(map + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n));
    store<u16>(map + kMapFlags, static_cast<u16>(fl | 1));
    store<u32>(o + obj2::kObjWord8, load<u32>(o + obj2::kObjWord8) | 0x400);
    ret(ctx, 1u);
}
