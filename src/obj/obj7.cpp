// World objects, part 7: the small callees parts 4-6 left recompiled
// (component / light checks, construction of topologies, emitter
// definitions and emitter properties, the IFF chunk reader, the pointer
// table growth and the topology record releases).
#include "obj7.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj6;
using namespace sly4::obj7;

namespace {

constexpr u32 kVtblTopology = 0x00B23368;      // AglTopology
constexpr u32 kStrNotSet = 0x00AF1870;         // "NOT SET"
constexpr u32 kTopoSharedList = 0x010DFA24;    // {Obj** data, ..., u16 count at +8}: shared blocks not yet freed (name?)
constexpr u32 kTopoName = 96;                  // const char*
constexpr u32 kTopoSharedNoList = 12;          // u8 in the shared block: nonzero = not on kTopoSharedList (name?)
constexpr u32 kTopoSharedChain = 8;            // the shared block's chain of {data*, ..., next at +8} nodes
constexpr u32 kKindTopology = 3, kKindEmitterDef = 22;
constexpr u32 kVtblEmitterDef = 0x00B247E0;
constexpr u32 kEmitterDef_Init = 0x0058DE4C;   // (ApeEmitterDef*): the remaining fields (name?)
constexpr u32 kLightAnim_Setup = 0x00591410;   // (record, comps, s32 slot, s32 light type) (name?)
constexpr u32 kLightAnimDefault = 0x00591818;  // f32 {0, 10000}: the vec4 (0, 0, 0, 10000) for light types 0 / 1
constexpr u32 kVfnStreamRead = 12;             // (stream, buf, size, 0) -> bytes read
constexpr u32 kVfnAllocAlloc = 0, kVfnAllocFree = 8;  // the pointer table's allocator: (a, size, 0, 16) / (a, p, size)

// The base-header constructor steps the constructors inline: +4 = refs 1
// and `kind` (bits 10-15; bits 0-9 kept), the root vtable, a new object
// serial in +8 bits 11-31 (0x7C0 cleared, the low 6 bits kept).
void ConstructHeader(u32 o, u32 kind) {
    u32 w4 = load<u32>(o + obj2::kObjWord4);
    store<u16>(o + obj2::kObjWord4, 1);
    w4 = (w4 & 0x0000FFFFu) | 0x00010000u;
    w4 = (w4 & ~0x0000FC00u) | (kind << 10);
    store<u32>(o + obj2::kObjWord4, w4);
    const u32 serial = load<u32>(obj2::kObjSerial) + 1;
    store<u32>(o, obj2::kVtblObjBase0);
    store<u32>(obj2::kObjSerial, serial);
}
u32 SerialWord8(u32 o) {
    return ((load<u32>(obj2::kObjSerial) << 11) | (load<u32>(o + obj2::kObjWord8) & 0x7FFu)) & ~0x7C0u;
}

// stvx of a zero vector: the 16 bytes at ea & ~15.
void ZeroQuad(u32 ea) {
    ea &= ~15u;
    for (u32 k = 0; k < 16; k += 4) store<u32>(ea + k, 0);
}

// The index of the first of comps' four component slots (+4..+16) whose
// type hash (+8) is `type`; -1 when a null slot or the end comes first.
s32 FindComp(u32 comps, u32 type) {
    for (u32 i = 0; i < kCompSlots; ++i) {
        const u32 c = load<u32>(comps + 4 + i * 4);
        if (c == 0) return -1;
        if (load<u32>(c + 8) == type) return static_cast<s32>(i);
    }
    return -1;
}

}  // namespace

// bool Light_Accepts(const LightInstance* l, bool visible_only)
// As Collector_Accepts for lights: never a +320 type-1 object, nor a
// hidden one when visible_only, nor one without components; otherwise
// whether one of its first four components has the light type hash
// (0x00F88954).
DECOMP(00591948, Light_Accepts) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 comps = load<u32>(o + kObjComps);
    if (((load<u32>(o + obj5::kObjBits320) >> 9) & 0xF) == kObjType320Skip) return ret(ctx, 0u);
    if ((arg<u32>(ctx, 1) & 0xFF) != 0 && (load<u32>(o + obj::kObjFlags) & kObjFlagVisible) == 0) return ret(ctx, 0u);
    if (comps == 0) return ret(ctx, 0u);
    ret(ctx, FindComp(comps, load<u32>(kCompTypeLight)) >= 0 ? 1u : 0u);
}

// void LightAnim_Init(LightAnim* rec, LightInstance* l)
// Points the record at the light (+68) and its class (+72), sets +12 bit
// 0x80 from the light's +321 bit 0x02, copies its +240 vec4 to +16 (types
// 0 / 1 (+368): (0, 0, 0, 10000)), then 0x00591410(rec, comps, the light
// component's slot or -1, type). As in the original the components are
// read without a null check (the callers check them first).
DECOMP(00591820, LightAnim_Init) {
    const u32 rec = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1);
    const u32 comps = load<u32>(l + kObjComps);
    const u32 type = load<u32>(kCompTypeLight);
    const u32 cl = load<u32>(l + kObjClassPtr);
    const u32 b12 = load<u8>(rec + kLaFlags);
    const s32 slot = FindComp(comps, type);
    store<u32>(rec + kLaOwner, l);
    store<u32>(rec + kLaOwner + 4, cl);
    u32 flag = 0;
    if (l != 0 && ((load<u8>(l + obj3::kObjByte321) >> 1) & 1) == 1) flag = 1;
    store<u8>(rec + kLaFlags, static_cast<u8>((b12 & 0x7F) | (flag << 7)));
    const s32 lt = static_cast<s32>(load<u32>(l + cls::kLightType));
    u32 v[4];
    if (lt == 0 || lt == 1) {
        const u32 zero = load<u32>(kLightAnimDefault), far = load<u32>(kLightAnimDefault + 4);
        v[0] = v[1] = v[2] = zero;
        v[3] = far;
    } else {
        for (u32 k = 0; k < 4; ++k) v[k] = load<u32>(l + kObjAnchor + k * 4);
    }
    for (u32 k = 0; k < 4; ++k) store<u32>(rec + 16 + k * 4, v[k]);
    call(ctx, kLightAnim_Setup, {rec, comps, static_cast<u64>(static_cast<s64>(slot)), static_cast<u64>(static_cast<s64>(lt))});
}

// void ObjComps_Assign(Obj* dst, const Obj* src)
// dst->+12 = src->+12 with reference counts (u16 at +0): the new set
// gains one; the old one loses one and at zero frees its four components
// (from the last slot to the first: each loses a reference and at zero
// frees its +28 block and itself) and then itself.
DECOMP(00558E24, ObjComps_Assign) {
    const u32 dst = arg<u32>(ctx, 0);
    const u32 nw = load<u32>(arg<u32>(ctx, 1) + kObjComps);
    const u32 old = load<u32>(dst + kObjComps);
    if (old == nw) return;
    store<u32>(dst + kObjComps, nw);
    if (nw != 0) store<u16>(nw, static_cast<u16>(load<u16>(nw) + 1));
    if (old == 0) return;
    const u16 refs = static_cast<u16>(load<u16>(old) - 1);
    store<u16>(old, refs);
    if (static_cast<s16>(refs) > 0) return;
    for (s32 i = static_cast<s32>(kCompSlots) - 1; i >= 0; --i) {
        const u32 c = load<u32>(old + 4 + static_cast<u32>(i) * 4);
        if (c == 0) continue;
        const u16 cr = static_cast<u16>(load<u16>(c) - 1);
        store<u16>(c, cr);
        if (static_cast<s16>(cr) > 0) continue;
        call(ctx, o4fn::Mem_FreePool, {load<u32>(c + 28), 0, 0});
        call(ctx, o4fn::Mem_FreePool, {c, 0, 0});
    }
    call(ctx, o4fn::Mem_FreePool, {old, 0, 0});
}

// void EmitProps_Construct(AglEmitProperties* p)
// The defaults: one reference, everything zero except +16 / +64 / +92 /
// +96 = 1.0, +48 = -3.0, +52 / +56 = -50.0, +60 = 5.0 and +42 bit 0x10
// (0xE0 cleared). Leaves r3..r9 as the original does.
DECOMP(004FFFF0, EmitProps_Construct) {
    const u32 p = arg<u32>(ctx, 0);
    constexpr u32 kOne = 0x3F800000, kNeg3 = 0xC0400000, kNeg50 = 0xC2480000, kFive = 0x40A00000;
    store<u32>(p + 72, 0);
    store<u16>(p + 40, 0);
    for (u32 off : {36u, 32u, 28u, 24u, 20u}) store<u32>(p + off, 0);
    store<u32>(p + 16, kOne);
    for (u32 off : {12u, 8u, 4u, 136u, 132u, 128u, 124u, 120u, 116u, 112u, 108u}) store<u32>(p + off, 0);
    store<u16>(p, 1);
    for (u32 off : {72u, 76u, 80u, 84u}) store<u32>(p + off, 0);
    u32 b42 = load<u8>(p + 42);
    store<u8>(p + 88, 0);
    store<u16>(p + 90, 0);
    store<u8>(p + 68, 0);
    store<u8>(p + 69, 0);
    store<u32>(p + 64, kOne);
    b42 = (b42 & ~0xE0u) | 0x10;
    store<u8>(p + 43, 0);
    store<u8>(p + 44, 0);
    store<u32>(p + 92, kOne);
    store<u8>(p + 42, static_cast<u8>(b42));
    store<u32>(p + 96, kOne);
    store<u32>(p + 100, 0);
    store<u32>(p + 104, 0);
    store<u32>(p + 48, kNeg3);
    store<u32>(p + 52, kNeg50);
    store<u32>(p + 56, kNeg50);
    store<u32>(p + 60, kFive);
    ctx.gpr[4] = b42;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = 0xFFFFFFFF00000000ull | kNeg3;  // lis sign-extends
    ctx.gpr[7] = kOne;
    ctx.gpr[8] = 0xFFFFFFFF00000000ull | kNeg50;
    ctx.gpr[9] = kFive;
}

// void WorldObj_UpdateKind8(Obj* o)
// An object with a slot entry (+284 != 0xFFFF, entry = *0x01193004 + slot
// * 128) copies its world matrix (+80: four rows of three) to the entry.
// Leaves r3 = &o->+136 and f1 = the last float copied.
DECOMP(00582F3C, WorldObj_UpdateKind8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 slot = load<u16>(o + obj2::kObjWord284);
    if (slot == 0xFFFF) return;
    const u32 e = load<u32>(kSlotTableBase) + ((slot << 7) & 0xFFFFFF80u);
    if (e == 0) return;
    const u32 w = o + kObjWorld;
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(e + row + k, load<u32>(w + row + k));
    ctx.fpr[1] = static_cast<f64>(load<f32>(w + 56));
    ret(ctx, w + 56);
}

// void Topology_Construct(AglTopology* t)
// The base header (kind 3, the root vtable then 0x00B23368, a new
// serial), zeroed draw lists and fields, +48 = &t->+16, +76 = 1.0, +84 =
// t, +54 bits: 0x10 / 0x20 set, 0xC0 cleared, name "NOT SET". The +88
// shared block (just zeroed) would be released first, as in the original.
// Leaves r3 = the name and v2 = 0.
DECOMP(00508690, Topology_Construct) {
    const u32 t = arg<u32>(ctx, 0);
    ConstructHeader(t, kKindTopology);
    const u32 w8 = SerialWord8(t);
    store<u32>(t, kVtblTopology);
    store<u32>(t + 12, 0);
    store<u32>(t + 8, w8 & ~0x400u);
    for (u32 off : {16u, 20u, 28u, 24u, 36u, 32u, 44u, 40u}) store<u32>(t + off, 0);
    const u32 b54 = load<u8>(t + kTopoFlags);
    store<u32>(t + kTopoShared, 0);
    store<u8>(t + kTopoFlags, static_cast<u8>(b54 | 0x10));
    store<u32>(t + kTopoDraws, t + 16);
    ZeroQuad(t + 16);
    ZeroQuad(t + 32);
    ctx.vr[2] = v128{};
    store<u16>(t + kTopoDrawCount, 0);
    store<u32>(t + 64, 0);
    store<u32>(t + 68, 0);
    store<u32>(t + 72, 0);
    store<u32>(t + 76, cls::kOneF);
    store<u32>(t + 80, 0);
    store<u32>(t + 84, t);
    const u32 shared = load<u32>(t + kTopoShared);
    const u32 b = load<u8>(t + kTopoFlags);
    store<u8>(t + kTopoFlags, static_cast<u8>((b & 0x3F) | 0x20));
    if (shared != 0) {
        store<u32>(t + kTopoShared, 0);
        call(ctx, o6fn::TopoShared_Release, {shared});
    }
    store<u32>(t + kTopoRecord, 0);
    store<u32>(t + kTopoName, kStrNotSet);
    ret(ctx, kStrNotSet);
}

// ChunkEntry* Iff_NextChunk(IffReader* r)
// Reads the next 8-byte chunk header {fourcc, size} (little endian) from
// the stream (vfunc +12) and advances the position by what was read.
// Nothing read: 0. Otherwise looks the fourcc up in the chunk map (only
// for its side effects: the result is unused) and pushes {fourcc, stream
// position before the header's payload (the low word of stream +264),
// size - 4} on the chunk stack unless it is full; returns the top entry
// (0 when the stack is empty).
DECOMP(00569108, Iff_NextChunk) {
    const u32 r = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 buf = frame.addr();  // sp+112: +0 the map key, +4 the header read
    const u32 stream = load<u32>(r + kIrStream);
    const u32 opd = load<u32>(load<u32>(stream) + kVfnStreamRead);
    const u32 n = static_cast<u32>(call_ptr(ctx, opd, {stream, buf + 4, 8, 0, opd, load<u32>(opd)}));  // r7 / r8 = OPD and entry
    store<u32>(r + kIrPos, load<u32>(r + kIrPos) + n);
    const u32 id = __builtin_bswap32(load<u32>(buf + 4));
    const u32 size = __builtin_bswap32(load<u32>(buf + 8));
    if (n == 0) return ret(ctx, 0u);
    const u64 pos = load<u64>(load<u32>(r + kIrStream) + kStreamPos);
    store<u32>(buf, id);
    store<u32>(buf + 8, size);
    store<u32>(buf + 4, id);
    call(ctx, o6fn::SortedMap_Find, {kChunkMap, buf});
    const u32 st = load<u32>(r + kIrStack);
    s32 top = static_cast<s32>(load<u32>(st + kIsTop));
    if (top < static_cast<s32>(load<u32>(st))) {
        ++top;
        store<u32>(st + kIsTop, static_cast<u32>(top));
        const u32 e = st + kIsEntries + static_cast<u32>(top) * kIsEntrySize;
        store<u32>(e, id);
        store<u32>(e + kIeStart, static_cast<u32>(pos));
        store<u32>(e + kIeSize, size - 4);
        top = static_cast<s32>(load<u32>(st + kIsTop));
    }
    ret(ctx, top > -1 ? st + kIsEntries + static_cast<u32>(top) * kIsEntrySize : 0u);
}

// void EmitterDef_Construct(ApeEmitterDef* d)
// The geometry-definition base (kind 22, a new serial, the empty box
// collapsed to the origin, the default sphere, zero +96..+108; a +108
// shared block, just zeroed, would be released as in the original), then
// the emitter definition's vtable, no data / system (+112 / +116), index
// 0xFFFF and 0x0058DE4C for the rest.
DECOMP(0058DF94, EmitterDef_Construct) {
    const u32 d = arg<u32>(ctx, 0);
    ConstructHeader(d, kKindEmitterDef);
    const u32 w8 = SerialWord8(d);
    store<u32>(d + 32, kFltMinNeg);
    store<u32>(d, cls::kVtblGeomBase);
    store<u32>(d + 36, kFltMinNeg);
    store<u32>(d + 16, kFltMax);
    store<u32>(d + 20, kFltMax);
    store<u32>(d + 24, kFltMax);
    store<u32>(d + 40, kFltMinNeg);
    store<u32>(d + cls::kGeomShared, 0);
    store<u32>(d + 12, 0);
    store<u32>(d + 8, w8 & ~0x400u);
    call(ctx, cls::cfn::Box_SetPoint, {d + 16, cls::kOrigin});
    for (u32 k = 0; k < 12; k += 4) store<u32>(d + 48 + k, load<u32>(cls::kDefaultSphere + k));
    const u32 sh = load<u32>(d + cls::kGeomShared);
    store<u32>(d + 96, 0);
    store<u32>(d + 60, load<u32>(cls::kDefaultSphere + 12));
    store<u32>(d + 100, 0);
    store<u8>(d + 64, 0);
    store<u32>(d + 104, 0);
    if (sh != 0) {
        store<u32>(d + cls::kGeomShared, 0);
        const u16 refs = static_cast<u16>(load<u16>(sh) - 1);
        store<u16>(sh, refs);
        if (static_cast<s16>(refs) <= 0) {
            const u32 data = load<u32>(sh + 4);
            if (data != 0) {
                call(ctx, cls::ifn::Mem_FreeFrom, {data - 16, 0});
                store<u32>(sh + 4, 0);
            }
            call(ctx, cls::ifn::Mem_FreeFrom, {sh, 0});
        }
    }
    store<u32>(d + obj5::kEdData, 0);
    store<u32>(d, kVtblEmitterDef);
    store<u32>(d + obj5::kEdSystem, 0);
    store<u16>(d + kEdIndex, 0xFFFF);
    call(ctx, kEmitterDef_Init, {d});
}

// void PtrTable_Grow(PtrTable* t, u32 by)
// Reallocates the table's block (header {u16 capacity, u16 count, u32
// size, +8 / +12 copied from the old header}, then the optional array
// when the old header's +8 is set, then the main array) for capacity +
// by entries through the header's allocator (+12, vfunc +0 with (size,
// 0, 16)), moves both arrays over, and frees the old block (vfunc +8 with
// the old size). As in the original a failed allocation is not checked.
DECOMP(007A78C4, PtrTable_Grow) {
    const u32 t = arg<u32>(ctx, 0);
    u32 h = load<u32>(t + 4);
    const u32 alloc = load<u32>(h + 12);
    const u32 cap = static_cast<u32>(load<u16>(h) + ctx.gpr[4]);
    const u32 bytes = (cap << 2) & ~3u;
    const u32 opt = load<u32>(h + 8);
    const u32 size = (bytes + (opt != 0 ? bytes : 0) + 31) & ~15u;
    u32 opd = load<u32>(load<u32>(alloc) + kVfnAllocAlloc);
    const u32 blk = static_cast<u32>(call_ptr(ctx, opd, {alloc, size, 0, 16, load<u32>(opd)}));  // r7 = entry
    h = load<u32>(t + 4);
    u32 opt_arr = 0, main_arr;
    if (load<u32>(h + 8) != 0) opt_arr = blk + 16;
    main_arr = opt_arr != 0 ? blk + bytes + 16 : blk + 16;
    if (opt_arr != 0) {
        call(ctx, o6fn::memmove, {opt_arr, load<u32>(t + 8), (static_cast<u32>(load<u16>(h)) << 2)});
        h = load<u32>(t + 4);
    }
    call(ctx, o6fn::memmove, {main_arr, load<u32>(t + 12), (static_cast<u32>(load<u16>(h)) << 2)});
    h = load<u32>(t + 4);
    const u32 old = load<u32>(t);
    const u32 old_size = load<u32>(h + 4);
    if (blk != 0) {
        store<u16>(blk + 2, 0);
        store<u32>(blk + 4, 0);
        store<u32>(blk + 8, 0);
    }
    const u32 oh = load<u32>(t + 4);
    store<u32>(blk + 8, load<u32>(oh + 8));
    store<u32>(blk + 12, load<u32>(load<u32>(t + 4) + 12));
    store<u32>(t, blk);
    store<u32>(t + 4, blk);
    store<u32>(t + 8, opt_arr);
    store<u32>(t + 12, main_arr);
    store<u16>(blk, static_cast<u16>(cap));
    store<u32>(blk + 4, size);
    store<u16>(blk + 2, load<u16>(oh + 2));
    const u32 a2 = load<u32>(blk + 12);
    opd = load<u32>(load<u32>(a2) + kVfnAllocFree);
    call_ptr(ctx, opd, {a2, old, old_size, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
}

// void TopoShared_Release(TopoShared* s)
// Drops a reference (u16 at +0); at zero takes s off the pending list at
// 0x010DFA24 when +12 is clear (the list's last entry moved into its
// place; a u8 index, as in the original), frees the chain at +8 (each
// node's data block with its four buffers at +0x8004 / +0x800C / +0x8010
// / +0x8014, then the node) and s.
DECOMP(005084CC, TopoShared_Release) {
    const u32 s = arg<u32>(ctx, 0);
    const u16 refs = static_cast<u16>(load<u16>(s) - 1);
    store<u16>(s, refs);
    if (static_cast<s16>(refs) > 0) return;
    if (load<u8>(s + kTopoSharedNoList) == 0) {
        const u32 arr = load<u32>(kTopoSharedList);
        const u32 n = load<u16>(kTopoSharedList + 8);
        if (n != 0) {
            u32 i = 0;
            do {
                if (load<u32>(arr + i * 4) == s) {
                    const u32 last = n - 1;
                    store<u16>(kTopoSharedList + 8, static_cast<u16>(last));
                    if (i != (last & 0xFFFF)) store<u32>(arr + i * 4, load<u32>(arr + ((last << 2) & 0x3FFFC)));
                    break;
                }
                i = (i + 1) & 0xFF;
            } while (i < n);
        }
    }
    if (s == 0) return;
    for (u32 p = load<u32>(s + kTopoSharedChain); p != 0; p = load<u32>(s + kTopoSharedChain)) {
        store<u32>(s + kTopoSharedChain, load<u32>(p + 8));
        const u32 q = load<u32>(p);
        if (q != 0) {
            call(ctx, cls::ifn::Mem_FreeFrom, {load<u32>(q + 0x800C), 0});
            call(ctx, cls::ifn::Mem_FreeFrom, {load<u32>(q + 0x8014), 0});
            if (load<u32>(q + 0x8004) != 0) call(ctx, cls::ifn::Mem_FreeFrom, {load<u32>(q + 0x8004), 0});
            if (load<u32>(q + 0x8010) != 0) call(ctx, cls::ifn::Mem_FreeFrom, {load<u32>(q + 0x8010), 0});
            call(ctx, cls::ifn::Mem_FreeFrom, {q, 0});
            store<u32>(p, 0);
        }
        call(ctx, cls::ifn::Mem_FreeFrom, {p, 0});
    }
    call(ctx, cls::ifn::Mem_FreeFrom, {s, 0});
}

// TopoRecordRef* TopoRecord_Set(TopoRecordRef* ref, TopoRecord* r)
// ref->+0 = r with reference counts (u16 at +0). The old record, at zero
// references, frees each of its entries (+8 array, u32 count at +4): an
// entry's 24-byte items (+0, u16 count at +14) give back their +4 pool
// block and free their +12 / +16 / +20 blocks (zeroed), then the items,
// the entry (its slot zeroed), the array and the record. Returns ref.
DECOMP(0079C668, TopoRecord_Set) {
    const u32 ref = arg<u32>(ctx, 0), nr = arg<u32>(ctx, 1);
    const u32 old = load<u32>(ref);
    ret(ctx, ref);
    if (old == nr) return;
    store<u32>(ref, nr);
    if (nr != 0) store<u16>(nr, static_cast<u16>(load<u16>(nr) + 1));
    if (old == 0) return;
    const u16 refs = static_cast<u16>(load<u16>(old) - 1);
    store<u16>(old, refs);
    if (static_cast<s16>(refs) > 0) return;
    if (load<u32>(old + 4) != 0) {
        u32 i = 0;
        do {
            const u32 e = load<u32>(load<u32>(old + 8) + i * 4);
            if (e != 0) {
                if (load<u32>(e) != 0) {
                    for (u32 j = 0, off = 0; j < load<u16>(e + 14); ++j, off += 24) {
                        const u32 it = load<u32>(e) + off;
                        if (load<u32>(it + 4) != 0) call(ctx, o4fn::Mem_FreePool, {load<u32>(it + 4), 0, 0});
                        for (u32 k : {12u, 16u, 20u}) {
                            if (load<u32>(it + k) == 0) continue;
                            call(ctx, cls::ifn::Mem_FreeFrom, {load<u32>(it + k), 0});
                            store<u32>(it + k, 0);
                        }
                    }
                    call(ctx, o4fn::Mem_FreePool, {load<u32>(e), 0, 0});
                }
                call(ctx, cls::ifn::Mem_FreeFrom, {e, 0});
                store<u32>(load<u32>(old + 8) + i * 4, 0);
            }
            i = (i + 1) & 0xFFFF;
        } while (i < load<u32>(old + 4));
    }
    if (load<u32>(old + 8) != 0) {
        call(ctx, cls::ifn::Mem_FreeFrom, {load<u32>(old + 8), 0});
        store<u32>(old + 8, 0);
    }
    call(ctx, cls::ifn::Mem_FreeFrom, {old, 0});
    ret(ctx, ref);
}

// void EmitProps_Destruct(AglEmitProperties* p)
// Resets the seven reference-counted members +108..+132 (each cleared;
// u16 references at +4; at zero vfunc +4 destroys it), then by the
// material mode (+68): 0 releases the +72 material (vfunc +8, cleared), 1
// the +72 array of +88 materials (each cleared), others nothing. Then the
// members' own destructors run from +136 down to +108 (+136 is only
// released here; the others were cleared above).
DECOMP(004FFA98, EmitProps_Destruct) {
    const u32 p = arg<u32>(ctx, 0);
    auto drop = [&](u32 x) {
        const u16 refs = static_cast<u16>(load<u16>(x + kSubRefs) - 1);
        store<u16>(x + kSubRefs, refs);
        if (static_cast<s16>(refs) <= 0) Vcall0(ctx, x, kVfnSubDestroy);
    };
    for (u32 off = 108; off <= 132; off += 4) {
        const u32 x = load<u32>(p + off);
        if (x == 0) continue;
        store<u32>(p + off, 0);
        drop(x);
    }
    const u32 mode = load<u8>(p + kEpStatic);
    if (mode == 0) {
        const u32 m = load<u32>(p + kEpMaterial);
        if (m != 0) {
            Release(ctx, m);
            store<u32>(p + kEpMaterial, 0);
        }
    } else if (mode == 1) {
        for (u32 i = 0; i < load<u8>(p + 88); ++i) {
            const u32 slot = p + kEpMaterial + i * 4;
            const u32 m = load<u32>(slot);
            if (m == 0) continue;
            Release(ctx, m);
            store<u32>(slot, 0);
        }
    }
    for (u32 off = 136; off >= 108; off -= 4) {
        const u32 x = load<u32>(p + off);
        if (x != 0) drop(x);
    }
}

// void EmitProps_CopyFrom(AglEmitProperties* dst, const AglEmitProperties* src)
// Copies the parameters (+4..+64, the +42 bits 0xF0, +43 / +44, +88 /
// +90, the material mode +68 and +69, +92..+104), the material(s) at +72
// by mode (0: one, 1: +88 of them; others none: new references only, the
// old ones are not released and nulls are not checked, as in the
// original) and the reference-counted members +108..+136 (references at
// +4; an old member dropping to zero is destroyed by its vfunc +4).
DECOMP(004FE3DC, EmitProps_CopyFrom) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    for (u32 off = 4; off <= 28; off += 4) store<u32>(dst + off, load<u32>(src + off));
    const u16 h40 = load<u16>(src + 40);
    store<u32>(dst + 32, load<u32>(src + 32));
    const u32 f36 = load<u32>(src + 36);
    store<u16>(dst + 40, h40);
    store<u32>(dst + 36, f36);
    u32 b = load<u8>(dst + 42);
    for (u32 bit : {0x80u, 0x40u, 0x20u, 0x10u}) {
        b = (b & ~bit) | (load<u8>(src + 42) & bit);
        store<u8>(dst + 42, static_cast<u8>(b));
        if (bit == 0x80u) store<u32>(dst + 64, load<u32>(src + 64));
    }
    store<u8>(dst + 43, load<u8>(src + 43));
    store<u8>(dst + 44, load<u8>(src + 44));
    for (u32 off = 48; off <= 64; off += 4) store<u32>(dst + off, load<u32>(src + off));
    store<u8>(dst + 88, load<u8>(src + 88));
    store<u16>(dst + 90, load<u16>(src + 90));
    const u32 mode = load<u8>(src + kEpStatic);
    store<u8>(dst + kEpStatic, static_cast<u8>(mode));
    auto addref = [](u32 x) { store<u16>(x + kSubRefs, static_cast<u16>(load<u16>(x + kSubRefs) + 1)); };
    if (mode == 0) {
        const u32 m = load<u32>(src + kEpMaterial);
        store<u32>(dst + kEpMaterial, m);
        addref(m);
    } else if (mode == 1 && load<u8>(src + 88) != 0) {
        u32 i = 0;
        do {
            const u32 m = load<u32>(src + kEpMaterial + i * 4);
            store<u32>(dst + kEpMaterial + i * 4, m);
            addref(m);
            ++i;
        } while (i < load<u8>(dst + 88));
    }
    store<u8>(dst + 69, load<u8>(src + 69));
    for (u32 off = 92; off <= 104; off += 4) store<u32>(dst + off, load<u32>(src + off));
    for (u32 off = 108; off <= 136; off += 4) {
        const u32 v = load<u32>(src + off), old = load<u32>(dst + off);
        if (old == v) continue;
        store<u32>(dst + off, v);
        if (v != 0) addref(v);
        if (old == 0) continue;
        const u16 refs = static_cast<u16>(load<u16>(old + kSubRefs) - 1);
        store<u16>(old + kSubRefs, refs);
        if (static_cast<s16>(refs) <= 0) Vcall0(ctx, old, kVfnSubDestroy);
    }
}
