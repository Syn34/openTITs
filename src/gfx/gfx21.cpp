// The renderer range's next recompiled functions (gfx21). Names carry the
// address where the role is unclear.
#include <atomic>
#include <bit>
#include <cmath>

#include "fx.h"
#include "gfx21.h"
#include "../game/meta.h"

using namespace sly4;

namespace {

namespace g21 = sly4::render::gfx21;
using sly4::fx::vcall;

inline f32 f32_at(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }
inline void put_f32(u32 a, f32 v) { store<u32>(a, std::bit_cast<u32>(v)); }
inline void atomic_put(u32 ea, u32 v) {  // lwarx / stwcx. loop in the original
    std::atomic_ref<u32>(ref<u32>(ea)).exchange(__builtin_bswap32(v));
}

// The tuning variable table's removal, as the base destructors inline it:
// the vtable stored, then a linear search for `v`; when found, the
// entries after it move down one, the count drops and the old last slot
// is cleared. Returns whether it was found (r3 is the new count then).
bool tuning_unregister(PPUContext& ctx, u32 v) {
    store<u32>(v, g21::kTuningVarBaseVtbl);
    const u32 n = load<u32>(g21::kTuningVarCount);
    ctx.gpr[4] = g21::kTuningVarBaseVtbl;
    ctx.gpr[5] = 0;
    if (n == 0) return false;
    ctx.gpr[4] = g21::kTuningVars;
    u32 i = 0;
    while (load<u32>(g21::kTuningVars + 4 * i) != v) {
        if (n <= ++i) return false;
    }
    const u32 last = n - 1;
    for (; i < last; ++i) store<u32>(g21::kTuningVars + 4 * i, load<u32>(g21::kTuningVars + 4 * (i + 1)));
    store<u32>(g21::kTuningVarCount, last);
    store<u32>(g21::kTuningVars + 4 * last, 0);
    return true;
}

// The sphere / name part of the geometry def constructors, in the
// original's store order, then the inlined release of the shared block at
// +108 (refcount u16 +0, data +4 allocated 16 bytes in).
void def_reset_sphere(PPUContext& ctx, u32 g) {
    const u32 shared = load<u32>(g + g21::kDefShared);
    store<u32>(g + 48, load<u32>(g21::kDefaultSphere));
    store<u32>(g + 52, load<u32>(g21::kDefaultSphere + 4));
    store<u32>(g + 56, load<u32>(g21::kDefaultSphere + 8));
    store<u32>(g + 96, 0);
    store<u32>(g + 60, load<u32>(g21::kDefaultSphere + 12));
    store<u32>(g + 100, 0);
    store<u8>(g + 64, 0);
    store<u32>(g + 104, 0);
    ctx.fpr[1] = sly4::render::gfx11::lfs(g21::kDefaultSphere + 12);  // f1 as the lfs / stfs copy leaves it
    if (shared == 0) return;
    store<u32>(g + g21::kDefShared, 0);
    const u16 refs = static_cast<u16>(load<u16>(shared) - 1);
    store<u16>(shared, refs);
    if (static_cast<s16>(refs) > 0) return;
    const u32 data = load<u32>(shared + 4);
    if (data != 0) {
        call(ctx, g21::fn::Mem_FreeFrom, {data - 16, 0});
        store<u32>(shared + 4, 0);
    }
    call(ctx, g21::fn::Mem_FreeFrom, {shared, 0});
}

// ApeTListAllocator<T>::GetAllocator() and the allocation: the class's
// free list, made on first use (when Mem_Alloc fails the original stores 0
// and calls through it), then its virtual Alloc (slot +12; r4 / r5 = the
// OPD and its entry).
u32 freelist_make(PPUContext& ctx, u32 vtbl, u32 name, u32 size, u32 r5);


// ApeTListAllocator<T>::GetAllocator(): the class's free list (header
// {+0 list, +4 heap} at *global), made on first use (0 stored when
// Mem_Alloc fails; the original then goes on with it).
u32 list_get(PPUContext& ctx, u32 global, u32 vtbl, u32 name, u32 size) {
    const u32 header = load<u32>(global);
    u32 list = load<u32>(header);
    if (list == 0) {
        list = freelist_make(ctx, vtbl, name, size, load<u32>(header + 4));
        store<u32>(header, list);
    }
    return list;
}

u32 def_alloc(PPUContext& ctx, const g21::DefClass& c) {
    const u32 list = list_get(ctx, c.global, c.list_vtbl, c.list_name, c.size);
    const u32 opd = load<u32>(load<u32>(list) + 12);
    return static_cast<u32>(call_ptr(ctx, opd, {list, opd, load<u32>(opd)}));
}

// A new ApeTFreeList<T> (60 bytes, Mem_Alloc(60, 0, 16)): zeroed, +54 bit
// 15 cleared, its vtable and GetAllocator() name, then FreeList_Init(l,
// size, r5, 0, 0) (r5: the heap, or the item count of an ApeTMemClassFL;
// r8 / r9 = the vtable and the name, FreeList_Init reads r3..r7). 0 when
// Mem_Alloc fails.
u32 freelist_make(PPUContext& ctx, u32 vtbl, u32 name, u32 size, u32 r5) {
    const u32 obj = static_cast<u32>(call(ctx, g21::fn::Mem_Alloc, {60, 0, 16}));
    if (obj == 0) return 0;
    for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
    const u16 h54 = load<u16>(obj + 54);
    store<u16>(obj + 52, 0);
    store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
    store<u32>(obj, vtbl);
    store<u32>(obj + 56, name);
    call(ctx, g21::fn::FreeList_Init, {obj, size, r5, 0, 0});
    return obj;
}

// The root header and geometry base constructor (vtable 0x00B246B0).
void def_construct_base(PPUContext& ctx, u32 g, u32 kind) {
    const u32 w4 = (load<u32>(g + 4) & 0x3FFu) | 0x00010000u | (kind << 10);
    store<u16>(g + 4, 1);
    store<u32>(g + 4, w4);
    const u32 serial = load<u32>(g21::kObjSerial) + 1;
    store<u32>(g21::kObjSerial, serial);
    const u32 w8 = load<u32>(g + 8);
    store<u32>(g + 32, g21::kFltMinNeg);
    store<u32>(g, g21::kGeomBaseVtbl);
    store<u32>(g + 36, g21::kFltMinNeg);
    store<u32>(g + 16, g21::kFltMax);
    store<u32>(g + 20, g21::kFltMax);
    store<u32>(g + 24, g21::kFltMax);
    store<u32>(g + 40, g21::kFltMinNeg);
    store<u32>(g + g21::kDefShared, 0);
    store<u32>(g + 12, 0);
    store<u32>(g + 8, ((w8 & 0x7FFu) | (serial << 11)) & ~0x7C0u);  // flags 0x400 and 0x3C0 cleared
    call(ctx, g21::fn::Box_SetPoint, {g + 16, g21::kOrigin});
    def_reset_sphere(ctx, g);
}

// The shared tail: the class's load hook (phase 0), the chunks read into
// the def, the hook again (phase 1), then the def in the class's named
// map under its +96 name (doubling the map when full) with flag 0x400.
// A failed allocation goes on with g = 0 like the original. Returns 1.
void def_read_chunks(PPUContext& ctx, const g21::DefClass& c, u32 r, u32 g) {
    auto hook = [&](u32 phase) {
        const u32 cls = load<u32>(c.global);
        const u32 opd = load<u32>(cls + 8);
        if (opd != 0) call_ptr(ctx, opd, {g, load<u32>(cls + 12), phase, opd, load<u32>(opd)});
    };
    hook(0);
    call(ctx, g21::fn::Iff_ReadChunks, {r, g});
    hook(1);
    const u32 map = load<u32>(load<u32>(c.global) + 16);
    const u32 name = load<u32>(g + g21::kDefName);
    if (load<u16>(map + 2) == load<u16>(map)) call(ctx, g21::fn::Array_Reserve, {map, (load<u16>(map) << 1) & 0xFFFEu});
    const u32 data = load<u32>(map + 8);
    store<u32>(data + 8 * load<u16>(map + 2), name);
    store<u32>(data + 8 * load<u16>(map + 2) + 4, g);
    const u32 n1 = load<u16>(map + 2) + 1, peak = load<u16>(map + 4), flags = load<u16>(map + 6);
    store<u16>(map + 2, static_cast<u16>(n1));
    store<u16>(map + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n1 & 0xFFFF) ? peak : n1));
    store<u16>(map + 6, static_cast<u16>(flags | 1));
    store<u32>(g + 8, load<u32>(g + 8) | g21::kDefRegistered);
    ret(ctx, 1u);
}

}  // namespace

// void TuningVar_ConstructBase_004FBFA8(TuningVar* v, const char* name)
// The base constructor of every tuning variable class (vtables 0x00B0D9xx
// / 0x00B13xxx): base vtable, the name copied into the String at +4 and
// its String_HashNoCase at +68, then v appended to the global table (no
// bounds check). Leaves r3 = 0x010C0000 (the table's high half).
DECOMP(004FBFA8, TuningVar_ConstructBase_004FBFA8) {
    const u32 v = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    store<u32>(v, g21::kTuningVarBaseVtbl);
    call(ctx, g21::fn::String_Assign, {v + g21::kTvName, name, g21::kTuningVarBaseVtbl});
    store<u32>(v + g21::kTvNameHash, static_cast<u32>(call(ctx, g21::fn::String_HashNoCase, {name})));
    const u32 n = load<u32>(g21::kTuningVarCount);
    store<u32>(g21::kTuningVarCount, n + 1);
    store<u32>(g21::kTuningVars + 4 * n, v);
    ctx.gpr[3] = g21::kTuningVarCount & 0xFFFF0000u;
}

// void TuningVar_DestructBase_004FC028(TuningVar* v)
// ~TuningVar() base: removes v from the global table (see
// tuning_unregister). Leaves r3 = the new count when found, else v.
DECOMP(004FC028, TuningVar_DestructBase_004FC028) {
    const u32 v = arg<u32>(ctx, 0);
    if (tuning_unregister(ctx, v)) ctx.gpr[3] = load<u32>(g21::kTuningVarCount);
}

// void TuningVar_DestructBase_004FC1A0(TuningVar* v)
// An identical copy of TuningVar_DestructBase_004FC028 (the destructors
// of the 0x00B0D9xx classes call this one).
DECOMP(004FC1A0, TuningVar_DestructBase_004FC1A0) {
    const u32 v = arg<u32>(ctx, 0);
    if (tuning_unregister(ctx, v)) ctx.gpr[3] = load<u32>(g21::kTuningVarCount);
}

// void TuningVar_Delete_004FC0CC(TuningVar* v)
// The deleting destructor: nothing for a null v, else the removal from
// the table and Mem_FreeFrom(v, 0).
DECOMP(004FC0CC, TuningVar_Delete_004FC0CC) {
    const u32 v = arg<u32>(ctx, 0);
    if (v == 0) return;
    tuning_unregister(ctx, v);
    call(ctx, g21::fn::Mem_FreeFrom, {v, 0, 0});
}

// void Trophy_Shutdown_004FB388(Trophy* t)   [called by World_Destruct]
// Stops the trophy thread (+92 = 1, join +88), destroys the two sync
// objects at +60 / +112, clears +8, destroys the NP trophy handle and
// context and terminates the library, clears the global 0x00F668E8, frees
// the entry array at +1376 through its allocator (capacity * element
// size) and destructs it, then frees the buffer at +52.
DECOMP(004FB388, Trophy_Shutdown_004FB388) {
    namespace m = sly4::meta;
    const u32 t = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);  // the thread's exit value (4 bytes at r1 + 112)
    store<u8>(t + 92, 1);
    call(ctx, g21::fn::Thread_Join, {load<u32>(t + m::kTrThread), frame.addr()});
    call(ctx, g21::fn::Event_Destroy, {t + m::kTrEvent60});
    call(ctx, g21::fn::Event_Destroy, {t + m::kTrEvent112});
    store<u8>(t + 8, 0);
    call(ctx, g21::imp::sceNpTrophyDestroyHandle, {load<u32>(t + m::kTrHandle)});
    call(ctx, g21::imp::sceNpTrophyDestroyContext, {load<u32>(t + m::kTrContext)});
    call(ctx, g21::imp::sceNpTrophyTerm);
    store<u32>(m::kTrophy, 0);
    const u32 arr = t + m::kTrList;
    const u32 bytes = static_cast<u32>(load<u16>(arr + m::kArrCapacity)) * (load<u16>(arr + m::kArrFlags) >> 1);
    const u32 data = load<u32>(arr + m::kArrData);
    if (data != 0) {
        const u32 alloc = load<u32>(arr + m::kArrAllocator);
        vcall(ctx, alloc, m::kAllocVfnFree, {alloc, data, bytes});
    }
    call(ctx, g21::fn::Array_Destruct, {arr});
    const u32 buf = load<u32>(t + 52);
    if (buf != 0) {
        call(ctx, g21::fn::Mem_FreeFrom, {buf, 0});
        store<u32>(t + 52, 0);
    }
    ctx.gpr[3] = 0;
}

// void SubMesh_DestructBase_004FD008(SubMesh* m)
// The SubMesh base destructor (vtable 0x00B23148): the pool part at +24
// destructed and reset (0, 1, 2, flags & 0x0FFFFFFF), the part at +44
// destructed, the buffers at +72 and +84 freed and cleared, both parts
// destructed again, the object at +16 deleted (vtable slot +8), then
// ApeRoot_Destruct.
DECOMP(004FD008, SubMesh_DestructBase_004FD008) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 p24 = m + g21::kSmPart24, p44 = m + g21::kSmPart44;
    store<u32>(m, g21::kSubMeshBaseVtbl);
    call(ctx, g21::fn::Pool_DestructPart, {p24, p24, g21::kSubMeshBaseVtbl});
    const u32 w12 = load<u32>(p24 + 12);
    store<u32>(p24 + 4, 1);  // (w4 & 1) | 1
    store<u32>(p24 + 0, 0);
    store<u32>(p24 + 8, 2);  // (w8 & 0xE) with 2 inserted into bits 1-3
    store<u32>(p24 + 12, w12 & 0x0FFFFFFFu);
    call(ctx, g21::fn::Pool_DestructPart, {p44});
    if (load<u32>(m + g21::kSmBuf72) != 0) {
        call(ctx, g21::fn::Mem_FreeFrom, {load<u32>(m + g21::kSmBuf72), 0});
        store<u32>(m + g21::kSmBuf72, 0);
    }
    if (load<u32>(m + g21::kSmBuf84) != 0) {
        call(ctx, g21::fn::Mem_FreeFrom, {load<u32>(m + g21::kSmBuf84), 0});
        store<u32>(m + g21::kSmBuf84, 0);
    }
    call(ctx, g21::fn::Pool_DestructPart, {p44});
    call(ctx, g21::fn::Pool_DestructPart, {p24});
    const u32 owned = load<u32>(m + g21::kSmOwned);
    if (owned != 0) vcall(ctx, owned, 8, {owned});
    call(ctx, g21::fn::ApeRoot_Destruct, {m});
}

// void PadDevice_UpdateRumble_0056F9D8(PadDevice* d)   (vtable slot)
// InputDevice_IntegrateAxes, then while active (+148): the small motor's
// accumulator +292 gains axis 0 (+132); once it reaches 1 it drops by 1
// and the small motor is on for this frame when axis 0 > 0, the large
// motor gets (s32)(axis 1 * 255) (else both 0). The two motor bytes go to
// cellPadSetActDirect when the port has an actuator.
DECOMP(0056F9D8, PadDevice_UpdateRumble_0056F9D8) {
    const u32 d = arg<u32>(ctx, 0);
    call(ctx, g21::fn::InputDevice_IntegrateAxes, {d});
    const u8 active = load<u8>(d + g21::kPadActive);
    const f32 axis1 = f32_at(d + g21::kPadAxis1);
    ctx.fpr[1] = sly4::render::gfx11::lfs(d + g21::kPadAxis1);
    ctx.gpr[3] = active;
    if (active == 0) return;
    const f32 axis0 = f32_at(d + g21::kPadAxis0);
    const f32 acc = static_cast<f32>(static_cast<f64>(f32_at(d + g21::kPadAccum)) + axis0);
    put_f32(d + g21::kPadAccum, acc);
    const f32 one = f32_at(g21::kPadRumbleConsts), zero = f32_at(g21::kPadRumbleConsts + 4);
    const f32 scale = f32_at(g21::kPadRumbleConsts + 8);
    u8 small = 0;
    u32 large = 0;
    if (acc >= one) {
        small = axis0 > zero ? 1 : 0;
        const f32 left = static_cast<f32>(static_cast<f64>(acc) - one);
        const f64 big = static_cast<f32>(static_cast<f64>(axis1) * scale);
        const u64 bits = rt::fctiw(big, true);
        ctx.set_fpr_bits(1, bits);
        large = static_cast<u32>(bits);
        put_f32(d + g21::kPadAccum, left);
    }
    store<u8>(d + g21::kPadAct, small);
    const u8 port = load<u8>(d + g21::kPadPort);
    store<u8>(d + g21::kPadAct + 1, static_cast<u8>(large));
    const u32 cap = load<u32>(g21::kPadCapability + 4 * port) & g21::kPadCapActuator;
    ctx.gpr[3] = cap;
    if (cap == 0) return;
    call(ctx, g21::imp::cellPadSetActDirect, {port, d + g21::kPadAct});
}

// bool LightDef_ReadChunks_0059BEE4(IffReader* r)   (chunk handler)
// A new ApeLightDef (kind 21, 176 bytes): the geometry base, then its
// vtable 0x00B24700 and GeomDef_Reset; read and registered as the class
// 0x00F8F130 does it (def_read_chunks).
DECOMP(0059BEE4, LightDef_ReadChunks_0059BEE4) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 g = def_alloc(ctx, g21::kLightDef);
    if (g != 0) {
        def_construct_base(ctx, g, g21::kLightDef.kind);
        store<u32>(g, g21::kLightDef.vtbl);
        call(ctx, g21::fn::GeomDef_Reset, {g, g21::kLightDef.vtbl});
    }
    def_read_chunks(ctx, g21::kLightDef, r, g);
}

// bool CameraDef_ReadChunks_0059CACC(IffReader* r)   (chunk handler)
// A new ApeCameraDef (kind 23, 192 bytes): the geometry base, its vtable
// 0x00B247B8, the box and sphere reset again, and the 3x4 frame at +112
// from the identity at 0x01164BD0 (+176 = 0); class 0x00F8F134.
DECOMP(0059CACC, CameraDef_ReadChunks_0059CACC) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 g = def_alloc(ctx, g21::kCameraDef);
    if (g != 0) {
        def_construct_base(ctx, g, g21::kCameraDef.kind);
        store<u32>(g, g21::kCameraDef.vtbl);
        call(ctx, g21::fn::Box_SetPoint, {g + 16, g21::kOrigin});
        def_reset_sphere(ctx, g);
        for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u})
            store<u32>(g + 112 + off, load<u32>(g21::kIdentity3x4 + off));
        store<u32>(g + 176, 0);
        store<u32>(g + 168, load<u32>(g21::kIdentity3x4 + 56));
        ctx.fpr[1] = sly4::render::gfx11::lfs(g21::kIdentity3x4 + 56);
    }
    def_read_chunks(ctx, g21::kCameraDef, r, g);
}

// bool PointDef_ReadChunks_0059E208(IffReader* r)   (chunk handler)
// A new ApePointDef (kind 24, 112 bytes): the geometry base, its vtable
// 0x00B24808, the box and sphere reset again; class 0x00F8F13C.
DECOMP(0059E208, PointDef_ReadChunks_0059E208) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 g = def_alloc(ctx, g21::kPointDef);
    if (g != 0) {
        def_construct_base(ctx, g, g21::kPointDef.kind);
        store<u32>(g, g21::kPointDef.vtbl);
        call(ctx, g21::fn::Box_SetPoint, {g + 16, g21::kOrigin});
        def_reset_sphere(ctx, g);
    }
    def_read_chunks(ctx, g21::kPointDef, r, g);
}

// Job* SkinBankJob_Build_0059A134(-, u32 per, u32 index)   (vtable slot)
// Bank_BuildViewJob's job plus the skinning job's inputs: the items array
// + 128 (+288), the arena's u8 +4 / +256 / +20 (+400 / +404 / +408), the
// skin matrix and job buffers of this frame with their counters and
// capacities (+412 .. +432), the skin block, SPU image and its size in
// quads (+436 .. +444) and the word at 0x010F0E08 (+368). Then for each
// item index i from (index & 0xFF) to min(index + 1, count) - 1 (signed;
// k = i - (index & 0xFFFF)): its 16 floats +48 at +304 + 64k, the lock
// word 0x01192C00 + 128i at +372 (atomically set to 1), the slot words
// item +768 / item +896 / 0x01192A00 + 128i / 0x01192800 + 128i at +376 /
// +388 / +380 / +384 + 4k (each atomically cleared), a 32 KiB frame
// buffer at item +1024 and +1152 with its end and the item's +1152 at
// +392 / +396 + 4k, and item + 128 * u8 +652 + 1280 at +448 + 4k.
// As in the original, i is a u8: a last index of 255 or more (a zero
// count, or a large index) never ends the loop.
DECOMP(0059A134, SkinBankJob_Build_0059A134) {
    namespace L = g21;
    const u32 index = arg<u32>(ctx, 2);
    const u32 job = static_cast<u32>(call(ctx, L::fn::Bank_BuildViewJob, {arg<u32>(ctx, 0), arg<u32>(ctx, 1), index}));
    const u32 items = load<u32>(L::kVjWork + 8);
    const u32 mframe = load<u32>(L::kSkinMatrixFrame);
    store<u32>(job + 288, items + 128);
    store<u32>(job + 420, L::kSkinEntries);
    store<u32>(job + 404, load<u32>(L::kArena + 256));
    store<u8>(job + 400, load<u8>(L::kArena + 4));
    store<u32>(job + 408, load<u32>(L::kArena + 20));
    store<u32>(job + 412, L::kSkinMatrices + mframe * 0x9000u);
    store<u32>(job + 416, L::kSkinMatrixCount);
    store<u32>(job + 432, L::kSkinEntries);
    store<u32>(job + 428, L::kSkinJobCount);
    store<u32>(job + 424, L::kSkinJobs + load<u32>(L::kSkinJobFrame) * 0x30000u);
    store<u32>(job + 436, static_cast<u32>(call(ctx, L::fn::Plus12080_005382A8, {L::kSkinMatrices})));
    store<u32>(job + 440, L::kSkinImage);
    store<u32>(job + 368, load<u32>(L::kSkinWord368));
    store<u32>(job + 444, L::kSkinImageQuads);
    const u32 first = index & 0xFFFF;
    u32 last = (first + 1) & 0xFFFF;
    const u32 count = load<u16>(L::kVjWork + 12);
    if (!(static_cast<s32>(last) < static_cast<s32>(count))) last = count;
    last = (last - 1) & 0xFFFF;
    for (u32 i = index & 0xFF; static_cast<s32>(i) <= static_cast<s32>(last); i = (i + 1) & 0xFF) {
        const u32 item = load<u32>(items + 4 + 4 * i);
        const u32 k4 = 4 * (i - first);
        const u32 d = job + 304 + 16 * k4;
        for (u32 o = 0; o < 64; o += 4) store<u32>(d + o, load<u32>(item + 48 + o));
        ctx.fpr[1] = sly4::render::gfx11::lfs(item + 108);
        store<u32>(job + 372, L::kSkinLocks + 128 * i);
        store<u32>(job + 376 + k4, item + 768);
        store<u32>(job + 388 + k4, item + 896);
        store<u32>(job + 380 + k4, L::kSkinSlotsA + 128 * i);
        store<u32>(job + 384 + k4, L::kSkinSlotsB + 128 * i);
        atomic_put(load<u32>(job + 372), 1);
        atomic_put(load<u32>(job + 376 + k4), 0);
        atomic_put(load<u32>(job + 388 + k4), 0);
        atomic_put(load<u32>(job + 384 + k4), 0);
        atomic_put(load<u32>(job + 380 + k4), 0);
        const u32 buf = static_cast<u32>(call(ctx, L::fn::Render_FrameAlloc, {0x8000}));
        store<u32>(item + 1024, buf);
        store<u32>(item + 1152, buf);
        store<u32>(job + 396 + k4, item + 1152);
        store<u32>(job + 392 + k4, buf + 0x8000);
        store<u32>(job + 448 + k4, item + (static_cast<u32>(load<u8>(item + 652)) << 7) + 1280);
    }
    ret(ctx, job);
}

// void Item_Submit_0058C53C(Item* it)   [Bank_PreUpdate, EmitterInstance_Visit1] (name?)
// Builds the item's 96-byte draw packet on the stack: rows 0-2 of its
// matrix (animated: WorldObj_AnimMatrix; else the slot matrix +284 or its
// own +80) and the translation, +288, the flag bytes, the colour +316 and
// +296; the texture from vfunc +28 (DrawPacket_SetTexture). An item whose
// bound sphere (+240, r +252) lies inside one of its occluder boxes (the
// centre through each box's matrix, |p| <= extent - r on x, y, z; vector
// lanes, NaN fails) stops there; otherwise the packet goes to the sink
// +208 (vfunc +20), then vfunc +32(it, *0x010DDA78) and, with +260 set,
// the same on the object at +256. View_GetCurrent is called before the
// occluder loop and its result unused, as in the original.
DECOMP(0058C53C, Item_Submit_0058C53C) {
    namespace L = g21;
    const u32 it = arg<u32>(ctx, 0);
    u32 flags = load<u32>(it + 164);
    u32 m;
    if (((flags >> 6) & 3) != 0) {
        m = static_cast<u32>(call(ctx, L::fn::WorldObj_AnimMatrix, {it}));
        flags = load<u32>(it + 164);
    } else {
        const u32 slot = load<u16>(it + 284);
        m = slot == 0xFFFF ? it + 80 : load<u32>(L::kSlotMatrices) + (slot << 7);
    }
    Stack frame(ctx, 128);
    const u32 pk = frame.addr();
    for (u32 row = 0; row < 4; ++row)
        for (u32 c = 0; c < 12; c += 4) store<u32>(pk + 16 * row + c, load<u32>(m + 16 * row + c));
    ctx.fpr[1] = sly4::render::gfx11::lfs(m + 56);
    store<u32>(pk + 64, load<u32>(it + 288));
    for (u32 off : {68u, 80u, 84u}) store<u32>(pk + off, 0);
    store<u8>(pk + 88, 0);
    store<u8>(pk + 89, 0);
    store<u32>(pk + 92, 0);
    store<u8>(pk + 68, load<u8>(it + 320));
    store<u8>(pk + 69, load<u8>(it + 168));
    store<u8>(pk + 70, static_cast<u8>((load<u32>(it + 320) >> 18) & 0x3F));
    const u32 bits = (((flags >> 14) & 1) << 7) | ((load<u8>(it + 321) & 1u) << 6) | (((load<u8>(it + 322) >> 7) & 1u) << 4);
    store<u8>(pk + 71, static_cast<u8>(bits));
    for (u32 k = 0; k < 4; ++k) store<u8>(pk + 72 + k, load<u8>(it + 316 + k));
    store<u32>(pk + 76, load<u32>(it + 296));
    u64 r3 = 0;
    const u32 tex = static_cast<u32>(vcall(ctx, it, 28, {it}));
    if (tex != 0) r3 = call(ctx, L::fn::DrawPacket_SetTexture, {pk, load<u32>(tex + 44), load<u16>(tex + 28)});
    const u32 occ = load<u32>(it + 340);
    if (occ != 0) {
        const u32 set = occ + 8;
        const u32 n = load<u16>(set + 4);
        if (n != 0) {
            r3 = call(ctx, L::fn::View_GetCurrent);
            u32 e = load<u32>(set);
            const u32 end = e + 80 * n;
            const f32 cx = f32_at(it + 240), cy = f32_at(it + 244), cz = f32_at(it + 248), rad = f32_at(it + 252);
            auto lane = [](u32 a, u32 k) { return f32_at((a & ~15u) + 4 * k); };  // lvx
            auto fma32 = [](f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); };
            do {
                bool inside = true;
                for (u32 k = 0; k < 3; ++k) {
                    f32 p = fma32(lane(e + 16, k), cx, 0.0f);
                    p = fma32(lane(e + 32, k), cy, p);
                    p = fma32(lane(e + 48, k), cz, p);
                    p = static_cast<f32>(static_cast<f64>(lane(e + 64, k)) + p);
                    const f32 lo = static_cast<f32>(static_cast<f64>(lane(e, k)) - rad);
                    if (!(lo >= std::fabs(p))) inside = false;  // vandc sign, vcmpgefp.
                }
                e += 80;
                r3 = e;
                if (inside) return ret(ctx, e);
            } while (e < end);
        }
    }
    const u32 sink = load<u32>(it + 208);
    if (sink == 0) return ret(ctx, r3);
    vcall(ctx, sink, 20, {sink, pk});
    vcall(ctx, it, 32, {it, load<u32>(L::kItemPassArg)});
    const u32 on = load<u32>(it + 260);
    if (on == 0) return ret(ctx, 0u);
    const u32 other = load<u32>(it + 256);
    r3 = vcall(ctx, other, 32, {other, load<u32>(L::kItemPassArg)});
    ret(ctx, r3);
}

// void Bank_DrawItem(-, Bank* b)   [Bank_DrawViewports] (name?)
// For each of the bank's queued draws (at most 256): fade = (u8 obj +168
// - 128) / 127; when |fade| >= 1e-5, a packet from the object's animated
// matrix (rows 0-2 and the translation; +68 = 0, +69 = 255) with its
// texture (vfunc +28, DrawPacket_SetTexture), scaled by fade * the
// draw's scale (Matrix_Scale3); the packet times the bank's matrix +48,
// transposed, goes to the vertex program constants 0-3; then a DrawItem
// for the model's first mesh and, from the last mesh down, every mesh
// with flag bit 5 is announced to the bank (vfunc +20) and drawn
// (DrawItem_DrawBank_005393FC, the item's +12 = the mesh). As in the
// original a model with no meshes makes the inner loop run on.
DECOMP(0058D088, Bank_DrawItem) {
    namespace L = g21;
    using sly4::render::gfx11::lfs;
    const u32 b = arg<u32>(ctx, 1);
    u32 e = load<u32>(b + L::kBankDrawList);
    const u32 cur = load<u32>(b + L::kBankDrawCursor);
    u32 n = (cur - e) >> 4;
    u64 r3 = cur;
    if (n > 256) n = 256;
    else if (n == 0) return ret(ctx, r3);
    const u32 end = e + 16 * n;
    Stack frame(ctx, 160);
    const u32 cb = frame.addr(), pk = cb + 64;  // r1 + 112 / r1 + 176
    auto fma32 = [](f32 a, f32 c, f32 bb) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(bb))); };
    auto lane = [](u32 a, u32 k) { return f32_at((a & ~15u) + 4 * k); };  // lvx
    do {
        const u32 obj = load<u32>(e);
        const f64 v = static_cast<f64>(static_cast<s32>(load<u8>(obj + 168)) - 128);
        const f32 fade = static_cast<f32>(static_cast<f64>(static_cast<f32>(v)) / static_cast<f64>(f32_at(L::kBankDrawConsts)));
        const f32 scale = f32_at(e + 8);
        const u32 model = load<u32>(e + 4);
        ctx.fpr[1] = lfs(L::kBankDrawConsts + 4);
        r3 = L::kBankDrawConsts;
        if (!(std::fabs(fade) < f32_at(L::kBankDrawConsts + 4))) {
            const u32 m = static_cast<u32>(call(ctx, L::fn::WorldObj_AnimMatrix, {obj}));
            for (u32 row = 0; row < 4; ++row)
                for (u32 c = 0; c < 12; c += 4) store<u32>(pk + 16 * row + c, load<u32>(m + 16 * row + c));
            for (u32 off : {68u, 64u, 76u, 80u, 84u}) store<u32>(pk + off, 0);
            store<u8>(pk + 88, 0);
            store<u8>(pk + 89, 0);
            store<u32>(pk + 92, 0);
            store<u8>(pk + 68, 0);
            store<u8>(pk + 69, 255);
            const u32 tex = static_cast<u32>(vcall(ctx, obj, 28, {obj}));
            if (tex != 0) call(ctx, L::fn::DrawPacket_SetTexture, {pk, load<u32>(tex + 44), load<u16>(tex + 28)});
            const f32 s = static_cast<f32>(static_cast<f64>(fade) * scale);
            callf(ctx, L::fn::Matrix_Scale3, {pk}, {s});
            ctx.fpr[1] = s;
            // R[i] = sum_k pk[i][k] * bank[k] (+ bank[3] for row 3), then transposed.
            f32 rm[4][4];
            const u32 bm = b + L::kBankMatrix;
            for (u32 i = 0; i < 4; ++i) {
                const u32 rowp = pk + 16 * i;
                for (u32 j = 0; j < 4; ++j) {
                    f32 acc = fma32(lane(bm, j), f32_at(rowp), i == 3 ? lane(bm + 48, j) : 0.0f);
                    acc = fma32(lane(bm + 16, j), f32_at(rowp + 4), acc);
                    rm[i][j] = fma32(lane(bm + 32, j), f32_at(rowp + 8), acc);
                }
            }
            for (u32 j = 0; j < 4; ++j)
                for (u32 i = 0; i < 4; ++i) put_f32(cb + 16 * j + 4 * i, rm[i][j]);  // stvx
            call(ctx, L::fn::cellGcmSetVertexProgramConstants, {load<u32>(L::kGcmContext), 0, 16, cb});
            const u32 di = static_cast<u32>(call(ctx, L::fn::DrawItem_Create, {load<u32>(load<u32>(model + 48)), 0, pk}));
            u64 k = static_cast<u64>(static_cast<s64>(load<u16>(model + 52)) - 1);
            for (;;) {
                const u32 ent = load<u32>(model + 48) + static_cast<u32>(k << 3);
                if (((load<u8>(load<u32>(ent + 4) + 100) >> 5) & 1) != 0) {
                    const u32 mesh = load<u32>(ent);
                    vcall(ctx, b, 20, {b, mesh});
                    store<u32>(di + 12, mesh);
                    call(ctx, L::fn::DrawItem_DrawBank_005393FC, {mesh, di, b});
                }
                r3 = k;
                if (static_cast<u32>(k) == 0) break;
                --k;
            }
        }
        e += 16;
    } while (e < end);
    ret(ctx, r3);
}

// void Bank_Stop596A8C(s32 mode)   [main, Bank_Destroy(2)] (name?)
// Modes 0 and 2: when the group stamp changed, the group levels are redone
// (GroupList_SetLevels on {level, count, items}; with more than 512
// objects the first count / 2 - 32 go to event slot 0's worker (OPD
// 0x00F3C618 with the block) and the rest are done here before waiting
// for it); every changed object gets its slot (SlotEntry_Alloc when it has
// none and +164 bit 15 is set) updated (SlotEntry_Update) and +164 bit 9
// cleared, the changed list is emptied (+8 = +12); the watched skeletons
// are composed (+31 bit 5 cleared) and the watch list emptied; the
// flagged objects (+164 bit 15, from the last) are pushed onto the output
// vector after emptying it. Modes 1 and 2 then bump the frame counter.
DECOMP(00596A8C, Bank_Stop596A8C) {
    namespace L = g21;
    const s32 mode = arg<s32>(ctx, 0);
    u64 r3 = static_cast<u32>(mode);
    if (mode == 0 || mode == 2) {
        if (load<u32>(L::kGroupStampSeen) != load<u32>(L::kGroupStamp)) {
            store<u32>(L::kGroupStampSeen, load<u32>(L::kGroupStamp));
            const u32 holder = load<u32>(L::kGroupHolder);
            const u32 level = load<u32>(L::kGroupLevel);  // lfs / stfs: raw word
            const u32 n = load<u16>(load<u32>(holder + 4) + 2);
            const u32 items = load<u32>(holder + 12);
            Stack frame(ctx, 48);
            const u32 blk = frame.addr();  // r1 + 112; the two halves at r1 + 124 / + 136
            auto block = [&](u32 a, u32 count, u32 list) {
                store<u32>(a, level);
                store<u32>(a + 4, count);
                store<u32>(a + 8, list);
            };
            if (n <= L::kGroupSplit) {
                block(blk, n, items);
                call(ctx, L::fn::GroupList_SetLevels, {blk});
            } else {
                const u32 first = (n >> 1) - 32;
                block(blk + 12, first, items);
                call(ctx, L::fn::EventSlot_Set, {0, (static_cast<u64>(L::kGroupJobOpd) << 32) | (blk + 12)});
                block(blk + 24, n - first, items + 4 * first);
                call(ctx, L::fn::GroupList_SetLevels, {blk + 24});
                call(ctx, L::fn::EventSlot_Wait, {0});
            }
        }
        const u32 cl = L::kChangedList;
        for (u32 i = 0; i < load<u32>(cl + 4); ++i) {
            const u32 e = load<u32>(load<u32>(cl) + 4 * i);
            u32 slot = load<u16>(e + 284);
            if (slot == 0xFFFF && ((load<u32>(e + 164) >> 15) & 1) != 0) {
                call(ctx, L::fn::SlotEntry_Alloc, {e});
                slot = load<u16>(e + 284);
            }
            if (slot != 0xFFFF) call(ctx, L::fn::SlotEntry_Update, {load<u32>(L::kSlotMatrices) + (slot << 7), e});
            store<u32>(e + 164, load<u32>(e + 164) & ~0x200u);
        }
        store<u32>(cl + 4, 0);
        store<u32>(cl + 8, load<u32>(cl + 12));
        const u32 sw = load<u32>(L::kSkeletonWatch);
        u32 cnt = load<u32>(sw + 4);
        for (u32 i = 0; i < cnt; ++i) {
            const u32 sk = load<u32>(load<u32>(sw) + 4 * i);
            if (sk == 0) continue;
            call(ctx, L::fn::Skeleton_Compose, {sk, load<u32>(sk + 44)});
            store<u8>(sk + 31, load<u8>(sk + 31) & ~0x20u);
            cnt = load<u32>(sw + 4);
        }
        const u32 out = load<u32>(L::kFlaggedOut);
        store<u32>(sw + 4, 0);
        store<u16>(out + 8, 0);
        const u32 nf = load<u16>(L::kFlaggedList + 8);
        r3 = L::kFlaggedList;
        if (nf != 0) {
            for (u32 k = nf - 1;; --k) {
                const u32 e = load<u32>(load<u32>(L::kFlaggedList) + 4 * k);
                if (((load<u32>(e + 164) >> 15) & 1) != 0) {
                    const u32 v = load<u32>(L::kFlaggedOut);
                    if (load<u16>(v + 8) == load<u16>(v + 4)) call(ctx, L::fn::PtrVector_Reserve, {v, static_cast<u32>(load<u16>(v + 4)) << 1});
                    const u32 c = load<u16>(v + 8);
                    store<u32>(load<u32>(v) + 4 * c, e);
                    const u32 c1 = c + 1, peak = load<u16>(v + 10);
                    store<u16>(v + 8, static_cast<u16>(c1));
                    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(c1 & 0xFFFF) ? peak : c1));
                }
                if (k == 0) break;
            }
            r3 = 0;
        }
    }
    if (mode == 1 || mode == 2) {
        store<u32>(L::kFrameCounter, load<u32>(L::kFrameCounter) + 1);
        r3 = L::kFrameCounter - 96;
    }
    ret(ctx, r3);
}

// void Boxes_PickNearest_005843C0(BoxSet* s, View* v)   [Bank_PreUpdate] (name?)
// Picks the (at most 2) boxes nearest the view's eye (+864): for each box
// the squared distance from the eye to it (the eye in the box's axes,
// clamped to its extents), kept when within the radius squared and the
// current cut-off (NaN passes both) and Sphere_FrustumFine(view planes
// +272, centre, extents, axes) says visible; insertion-sorted into a
// frame buffer of {box, d}, the cut-off becoming the last one's d once
// full. As in the original the count grows to 2 even with room for 1
// (writing past the buffer). Then each picked box goes to the output: its
// extents, its axes transposed (columns of [3][4]) and the translation
// -centre times them (vector lanes).
DECOMP(005843C0, Boxes_PickNearest_005843C0) {
    namespace L = g21;
    using sly4::render::gfx11::lfs;
    const u32 s = arg<u32>(ctx, 0), view = arg<u32>(ctx, 1);
    const u32 c = load<u16>(s + 4);
    store<u16>(s + 12, 0);
    u32 max = L::kPickMax;
    if (static_cast<s32>(c) < 2) {
        max = c;
        if (c == 0) return ret(ctx, 0u);
    }
    const u32 planes = view + 272, eye = view + 864;
    const f32 rad = f32_at(s + 16);
    const f32 limit = static_cast<f32>(static_cast<f64>(rad) * rad);
    f32 cutoff = f32_at(L::kPickConsts);
    const u32 buf = static_cast<u32>(call(ctx, L::fn::Render_FrameAlloc, {max * 8}));
    u32 e = load<u32>(s);
    const u32 end = e + 80 * load<u16>(s + 4);
    u32 picked = 0;
    u64 r3 = 0;
    f64 f1 = 0;
    auto fmadds = [](f32 a, f32 c2, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c2), static_cast<f64>(b))); };
    do {
        const u32 box = e;
        e += 80;
        const f32 dx = static_cast<f32>(static_cast<f64>(f32_at(eye)) - f32_at(box + 64));
        const f32 dy = static_cast<f32>(static_cast<f64>(f32_at(eye + 4)) - f32_at(box + 68));
        const f32 dz = static_cast<f32>(static_cast<f64>(f32_at(eye + 8)) - f32_at(box + 72));
        f1 = dx;
        f32 d2 = f32_at(L::kPickConsts + 4);
        for (u32 j = 0; j < 3; ++j) {
            const u32 ax = box + 16 + 16 * j;
            f32 d = static_cast<f32>(static_cast<f64>(f32_at(ax + 4)) * dy);
            d = fmadds(f32_at(ax), dx, d);
            d = fmadds(f32_at(ax + 8), dz, d);
            const f32 ext = f32_at(box + 4 * j);
            if (d < -ext) {
                const f32 o = static_cast<f32>(static_cast<f64>(d) + ext);
                d2 = fmadds(o, o, d2);
            } else if (d > ext) {
                const f32 o = static_cast<f32>(static_cast<f64>(d) - ext);
                d2 = fmadds(o, o, d2);
            }
        }
        r3 = box + 64;
        if (limit < d2 || d2 > cutoff) continue;
        r3 = call(ctx, L::fn::Sphere_FrustumFine, {planes, box + 64, box, box + 16});
        if (static_cast<u32>(r3) == 0) continue;
        s32 i = static_cast<s16>(picked - 1);
        while (i >= 0) {
            f1 = lfs(buf + 8 * i + 4);
            if (d2 > f32_at(buf + 8 * i + 4)) break;
            i = static_cast<s16>(i - 1);
        }
        r3 = static_cast<u64>(static_cast<s64>(i));
        const s32 pos = i + 1;
        if (!(pos < static_cast<s32>(max))) continue;
        if (picked < 2) ++picked;
        for (s32 k = static_cast<s16>(picked - 2); k >= pos; k = static_cast<s16>(k - 1)) {
            store<u32>(buf + 8 * (k + 1), load<u32>(buf + 8 * k));
            f1 = lfs(buf + 8 * k + 4);
            store<u32>(buf + 8 * (k + 1) + 4, load<u32>(buf + 8 * k + 4));
        }
        put_f32(buf + 8 * pos + 4, d2);
        store<u32>(buf + 8 * pos, box);
        r3 = buf + 8 * pos;
        if (picked == max) cutoff = f32_at(buf + 8 * max - 4);
    } while (e < end);
    store<u16>(s + 12, static_cast<u16>(picked));
    ctx.fpr[1] = f1;
    if (picked == 0) return ret(ctx, r3);
    auto lane = [](u32 a, u32 k) { return f32_at((a & ~15u) + 4 * k); };  // lvx
    for (u32 k = 0; k < picked; ++k) {
        const u32 box = load<u32>(buf + 8 * k);
        const u32 out = load<u32>(s + 8) + 80 * k;
        f32 col[3][4], nc[3];
        for (u32 j = 0; j < 3; ++j) nc[j] = static_cast<f32>(0.0 - static_cast<f64>(lane(box + 64, j)));  // vsubfp 0 - c
        for (u32 j = 0; j < 3; ++j)
            for (u32 r = 0; r < 3; ++r) col[j][r] = lane(box + 16 + 16 * r, j);
        f32 t[3];
        for (u32 r = 0; r < 3; ++r) {
            f32 acc = fmadds(col[0][r], nc[0], 0.0f);
            acc = fmadds(col[1][r], nc[1], acc);
            t[r] = fmadds(col[2][r], nc[2], acc);
        }
        for (u32 j = 0; j < 3; ++j) store<u32>(out + 4 * j, load<u32>(box + 4 * j));
        for (u32 j = 0; j < 3; ++j)  // stvx, then lfs / stfs: raw words
            for (u32 r = 0; r < 3; ++r) store<u32>(out + 16 + 16 * j + 4 * r, load<u32>(((box + 16 + 16 * r) & ~15u) + 4 * j));
        for (u32 r = 0; r < 3; ++r) put_f32(out + 64 + 4 * r, t[r]);
        ctx.fpr[1] = t[2];
    }
    ret(ctx, 8 * picked);
}

// void PackEntry_Unpack_00563368(Pack* p, s32 i, Unpacked* out, void** buf,
// u32* size, bool swap_out)   [f_00563808] (name?)
// Entry i of a packed table (base p +4, words byte-swapped when u8 p +12
// is set): its key (base +4 + 8i) and header (offset base +8 + 8i: {u32
// n, -, u32 first key, then 12-byte records {-, u32 a, u32 key}}). Lays
// out one buffer from the memory stack at 0x0116489C: the n words a, the
// n u16 (key - first key) (each byte-swapped when swap_out), then the
// entry's data (Pack_Entry_007A4874); out gets {key, 0, u16 n, 0, offsets
// 4n and 4n + align4(2n + 3)}, *size the total and *buf the buffer (0
// when empty). The two arrays are built on the stack first (256 words /
// 260 halves; a larger n runs past them, as in the original).
DECOMP(00563368, PackEntry_Unpack_00563368) {
    namespace L = g21;
    const u32 p = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 bufp = arg<u32>(ctx, 3), sizep = arg<u32>(ctx, 4);
    const bool swap_out = (arg<u32>(ctx, 5) & 0xFF) != 0;
    const u32 i8 = i << 3;
    u32 base = load<u32>(p + 4);
    bool swap = load<u8>(p + 12) != 0;
    auto rd = [&](u32 a) { const u32 w = load<u32>(a); return swap ? __builtin_bswap32(w) : w; };
    const u32 key = rd(base + 4 + i8);
    const u32 n = rd(base + rd(base + 8 + i8));
    Stack frame(ctx, 1552);
    const u32 loc = frame.addr();  // r1 + 112: the data size, then the arrays at +4 / +1028
    store<u32>(loc, 0);
    const u32 data = static_cast<u32>(call(ctx, L::fn::Pack_Entry_007A4874, {p, i, loc, 0}));
    swap = load<u8>(p + 12) != 0;
    base = load<u32>(p + 4);
    const u32 dsize = load<u32>(loc);
    const u32 first = rd(base + rd(base + 8 + i8) + 8);
    for (u32 k = 0; k < n; ++k) {
        const u32 a = rd(base + rd(base + 8 + i8) + 12 * k + 4);
        store<u32>(loc + 4 + 4 * k, a);
        const u32 d = rd(base + rd(base + 8 + i8) + 12 * k + 8) - first;
        store<u16>(loc + 1028 + 2 * k, static_cast<u16>(d));
        if (swap_out) {
            store<u32>(loc + 4 + 4 * k, __builtin_bswap32(load<u32>(loc + 4 + 4 * k)));
            store<u16>(loc + 1028 + 2 * k, __builtin_bswap16(static_cast<u16>(d)));
        }
    }
    const u32 wbytes = (4 * n + 3) & ~3u, hbytes = (2 * n + 3) & ~3u;
    const u32 dofs = hbytes + wbytes, total = dofs + dsize;
    store<u32>(out + 0, key);
    store<u32>(out + 4, 0);
    store<u16>(out + 8, static_cast<u16>(n));
    store<u16>(out + 10, 0);
    store<u16>(out + 12, static_cast<u16>(wbytes));
    store<u16>(out + 14, static_cast<u16>(dofs));
    store<u16>(out + 16, 0);
    store<u32>(out + 20, 0);
    store<u16>(out + 18, 0);
    store<u32>(out + 24, 0);
    store<u32>(sizep, total);
    store<u32>(bufp, 0);
    if (total == 0) return ret(ctx, 0u);
    const u32 buf = static_cast<u32>(call(ctx, L::fn::MemStack_Alloc, {load<u32>(L::kUnpackStack), total}));
    const u32 words_at = load<u16>(out + 10);
    store<u32>(bufp, buf);
    call(ctx, L::fn::memmove, {buf + words_at, loc + 4, wbytes});
    call(ctx, L::fn::memmove, {load<u32>(bufp) + load<u16>(out + 12), loc + 1028, hbytes});
    call(ctx, L::fn::memmove, {load<u32>(bufp) + load<u16>(out + 14), data, load<u32>(loc)});
}

// void Constants_Init_0056B93C()   (static initializer)
// Fills the engine's constant vectors, matrices and colours: Vec2 one /
// zero / +-x / +-y, Vec3 (16-byte slots) one / zero / +-x / +-y / +-z,
// Vec4 one / zero, the identity as 3x4 rows, 4x4 and 3x3 rows, a zero
// Vec4 and the identity quaternion, twelve RGBA colours; clears a few
// words, sets up the chunk vector (PtrVector_Init(v, 0, 0, 0), element
// size 4) and the chunk map (capacity 256, 8-byte entries, its 2 KiB
// from the default allocator (made on first use), default compare).
// The original interleaves the stores (checked with the stores sorted).
DECOMP(0056B93C, Constants_Init_0056B93C) {
    namespace K = g21::k;
    constexpr u32 one = 0x3F800000u, neg = 0xBF800000u;
    auto words = [](u32 a, std::initializer_list<u32> v) {
        for (u32 w : v) { store<u32>(a, w); a += 4; }
    };
    store<u32>(K::kWord4CD4, 0);
    words(K::kVec2One, {one, one, 0, 0, one, 0, 0, one, neg, 0, 0, neg});
    const u32 vec3[8][3] = {{one, one, one}, {0, 0, 0}, {one, 0, 0}, {0, one, 0}, {0, 0, one}, {neg, 0, 0}, {0, neg, 0}, {0, 0, neg}};
    for (u32 k = 0; k < 8; ++k) words(K::kVec3One + 16 * k, {vec3[k][0], vec3[k][1], vec3[k][2]});
    words(K::kVec4One, {one, one, one, one});
    words(K::kVec4Zero, {0, 0, 0, 0});
    for (u32 r = 0; r < 4; ++r) words(K::kMatrix3x4 + 16 * r, {r == 0 ? one : 0, r == 1 ? one : 0, r == 2 ? one : 0});
    for (u32 r = 0; r < 4; ++r) words(K::kMatrix4x4 + 16 * r, {r == 0 ? one : 0, r == 1 ? one : 0, r == 2 ? one : 0, r == 3 ? one : 0});
    for (u32 r = 0; r < 3; ++r) words(K::kMatrix3x3 + 16 * r, {r == 0 ? one : 0, r == 1 ? one : 0, r == 2 ? one : 0});
    words(K::kVec4Zero2, {0, 0, 0, 0});
    words(K::kQuatIdentity, {0, 0, 0, one});
    store<u64>(K::kZero64, 0);
    store<u32>(K::kZero64 + 8, 0);
    static constexpr u8 kRgba[12][4] = {
        {255, 255, 255, 255}, {0, 0, 0, 0}, {0, 0, 0, 255}, {255, 0, 0, 255},
        {0, 255, 0, 255}, {0, 0, 255, 255}, {160, 32, 240, 255}, {255, 165, 0, 255},
        {64, 64, 64, 255}, {255, 255, 0, 255}, {255, 160, 215, 255}, {0, 255, 255, 255}};
    for (u32 c = 0; c < 12; ++c)
        for (u32 b = 0; b < 4; ++b) store<u8>(K::kColours + 4 * c + b, kRgba[c][b]);
    // The chunk vector: empty, element size 4, flags +12 bits 0-2 cleared
    // (the other bits too: the original masks with 7 first).
    const u32 v = K::kChunkVector;
    store<u32>(v, 0);
    store<u16>(v + 4, 0);
    store<u16>(v + 8, 0);
    store<u16>(v + 10, 0);
    store<u32>(v + 16, 0);
    store<u16>(v + 6, 4);
    store<u32>(v + 12, 0);
    call(ctx, g21::fn::PtrVector_Init, {v, 0, 0, 0});
    store<u32>(K::kChunkPair, 0);
    store<u32>(K::kChunkPair + 4, 256);
    const u32 m = K::kChunkMap;
    store<u16>(m + 0, 256);  // capacity
    store<u16>(m + 2, 0);
    store<u16>(m + 4, 0);
    store<u16>(m + 6, 16);   // entry size 8 << 1
    store<u32>(m + 8, 0);
    u32 alloc = load<u32>(K::kDefaultAllocatorPtr);
    if (alloc == 0) {
        if (load<u8>(K::kDefaultAllocatorGuard) == 0) {
            store<u32>(K::kDefaultAllocator, K::kDefaultAllocatorVtable);
            store<u8>(K::kDefaultAllocatorGuard, 1);
        }
        store<u32>(K::kDefaultAllocator, K::kDefaultAllocatorVtable);  // the placement new again
        alloc = K::kDefaultAllocator;
        store<u32>(K::kDefaultAllocatorPtr, alloc);
    }
    const u32 opd = load<u32>(load<u32>(alloc));  // slot 0: Alloc(this, size, 0, align); r7 / r8 = entry / OPD
    store<u32>(m + 12, alloc);
    const u32 data = static_cast<u32>(call_ptr(ctx, opd, {alloc, 2048, 0, 16, load<u32>(opd), opd}));
    store<u32>(m + 8, data);
    store<u32>(m + 20, K::kDefaultCompare);
    store<u32>(m + 16, 0);
    ret(ctx, data);
}

// void SimpleEmitter_SetData_0059D6BC(EmitterData* d, Emitter* e, s32 mode)   (vtable slot) (name?)
// Mode 1 only: e +112 = d (ref counted: d's u16 +0 bumped, the old data
// released), the emitter's vfunc +12 when it has data, then d released
// (the by-value reference). A new 80-byte instance (SmallAlloc; base
// Thunk_005008D8(inst, data), vtable 0x00B23308 then 0x00B23338) with a
// ring buffer of min(count, 256) 36-byte particles from the
// ApeTMemClassFL<ApeTRingBuf<AglSimplePtl>, 1024> list (0x011DE4B4, made
// on first use) at +72 becomes e +116 (the old one deleted through
// vfunc +24). With d +43 == 1 the instance's +64 bit 7 is cleared and d
// +42 gets 0x40 (a failed SmallAlloc then writes through 0, as in the
// original).
DECOMP(0059D6BC, SimpleEmitter_SetData_0059D6BC) {
    namespace L = g21;
    const u32 d = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    if (arg<s32>(ctx, 2) != 1) return;
    const u32 old = load<u32>(e + 112);
    if (old != d) {
        store<u32>(e + 112, d);
        if (d != 0) store<u16>(d, static_cast<u16>(load<u16>(d) + 1));
        if (old != 0) call(ctx, L::fn::EmitterData_Release, {old});
    }
    if (load<u32>(e + 112) != 0) vcall(ctx, e, 12, {e});
    call(ctx, L::fn::EmitterData_Release, {d});
    const bool flagged = load<u8>(d + 43) == 1;
    u32 inst = static_cast<u32>(call(ctx, L::fn::SmallAlloc, {80}));
    if (inst != 0) {
        const u32 data = load<u32>(e + 112);
        call(ctx, L::fn::Thunk_005008D8, {inst, data});
        store<u32>(inst, L::kSimpleEmitterBaseVtbl);
        u32 fl = load<u32>(L::kRingFreeList);
        if (fl == 0) {
            fl = freelist_make(ctx, L::kRingFreeListVtbl, L::kRingFreeListName, 24, 1024);
            store<u32>(L::kRingFreeList, fl);  // 0 if Mem_Alloc failed: the original then calls through 0
        }
        const u32 opd = load<u32>(load<u32>(fl) + 12);
        u32 ring = static_cast<u32>(call_ptr(ctx, opd, {fl, opd, load<u32>(opd)}));
        if (ring != 0) {
            const u32 n = static_cast<u32>(call(ctx, L::fn::Min256_00500534, {data}));
            const u32 buf = static_cast<u32>(call(ctx, L::fn::SmallAlloc, {36 * n}));
            store<u32>(ring + 0, buf);
            store<u32>(ring + 8, buf);
            store<u32>(ring + 12, buf);
            store<u32>(ring + 16, n);
            store<u32>(ring + 20, 0);
            store<u32>(ring + 4, buf + 36 * n);
        }
        store<u32>(inst + 72, ring);
        store<u32>(inst, L::kSimpleEmitterVtbl);
    }
    if (flagged) {
        store<u8>(inst + 64, load<u8>(inst + 64) & 0x7F);
        store<u8>(d + 42, load<u8>(d + 42) | 0x40);
    }
    const u32 cur = load<u32>(e + 116);
    u64 r3 = cur;
    if (cur != inst) {
        store<u32>(e + 116, inst);
        if (cur != 0) r3 = vcall(ctx, cur, 24, {cur});
    }
    ret(ctx, r3);
}

// void GeomLod_Update_005908A0()   [Bank_Start] (name?)
// With the thread's LOD parameters (GetForThread_00522C98: +64 f32 max
// distance, +68 / +72 scales): the defs of up to 32 kind-1 instances
// (+320 bits 18-23) are collected; every other ApeGeomDef gets its +120
// scaled by +72 and clamped to [1e-5, 100], and its LOD distances (+136
// + 8k, k < u8 +112) scaled by +68 and capped at the max (fsel). Then
// every instance's +312 = (its def's +60 + the last LOD's distance)^2 and
// its vfunc +36 is called. The instance / def lists come from the classes'
// free lists (made on first use); items sit 16 bytes into their nodes.
DECOMP(005908A0, GeomLod_Update_005908A0) {
    namespace L = g21;
    using sly4::render::gfx11::lfs;
    const u32 prm = static_cast<u32>(call(ctx, L::fn::GetForThread_00522C98));
    auto first = [&](u32 list) {
        const u32 node = static_cast<u32>(call(ctx, L::fn::Obj_GetField4, {list}));
        return node != 0 ? node + 16 : 0u;
    };
    auto next = [](u32 item) {
        const u32 node = load<u32>(item - 16);
        return node != 0 ? node + 16 : 0u;
    };
    Stack frame(ctx, 128);
    const u32 kept = frame.addr();  // r1 + 112: GeomDef*[32]
    u32 nkept = 0;
    for (u32 it = first(list_get(ctx, L::kGeomInstClass, L::kGeomInstListVtbl, L::kGeomInstListName, 352)); it != 0; it = next(it)) {
        if (((load<u32>(it + 320) >> 18) & 0x3F) == 1 && nkept != 32) store<u32>(kept + 4 * nkept++, load<u32>(it + 208));
    }
    const f32 maxd = f32_at(prm + 64);
    f64 f1 = ctx.fpr[1];
    u32 g = first(list_get(ctx, L::kGeomDefClass, L::kGeomDefListVtbl, L::kGeomDefListName, 176));
    if (g != 0) {
        const f32 hi = f32_at(L::kLodConsts), lo = f32_at(L::kLodConsts + 4);
        for (; g != 0; g = next(g)) {
            bool found = false;
            for (u32 k = 0; k < nkept; ++k)
                if (load<u32>(kept + 4 * k) == g) { found = true; break; }
            if (found) continue;
            const f32 v = static_cast<f32>(static_cast<f64>(f32_at(g + 120)) * f32_at(prm + 72));
            f1 = v;
            const f32 m = v < hi ? v : hi;
            put_f32(g + 120, m > lo ? m : lo);
            for (u32 k = 0; k < load<u8>(g + 112); ++k) {
                const u32 p = g + 132 + 8 * k + 4;
                const f32 d = static_cast<f32>(static_cast<f64>(f32_at(p)) * f32_at(prm + 68));
                const f32 over = static_cast<f32>(static_cast<f64>(d) - maxd);
                const f32 r = over >= 0.0f ? maxd : d;  // fsel
                f1 = r;
                put_f32(p, r);
            }
        }
    }
    for (u32 it = first(list_get(ctx, L::kGeomInstClass, L::kGeomInstListVtbl, L::kGeomInstListName, 352)); it != 0; it = next(it)) {
        const u32 d = load<u32>(it + 208);
        const f32 r = static_cast<f32>(static_cast<f64>(f32_at(d + 8 * load<u8>(d + 112) + 128)) + f32_at(d + 60));
        const f32 r2 = static_cast<f32>(static_cast<f64>(r) * r);
        f1 = r2;
        put_f32(it + 312, r2);
        vcall(ctx, it, 36, {it});
    }
    ctx.fpr[1] = f1;
    ret(ctx, 0u);
}

// bool Tri_OverlapsBox_00577788(const f32 c[3], const f32 h[3], const f32 tri[3][4])   [Quad_InFrustum] (name?)
// The separating-axis triangle / box test (Akenine-Moller): the triangle
// relative to the box centre, the nine edge x axis tests (projections
// with fmuls / fmsubs, radius fmuls / fmadds), the box's own three axes,
// then Plane_StraddlesBox with the triangle's normal (e0 x e1), v0 and
// the half extents. f1 is left as the original leaves it: v0.x through
// the edge tests, the max x / y / z for the axis tests, the normal's x
// after.
DECOMP(00577788, Tri_OverlapsBox_00577788) {
    namespace L = g21;
    const u32 c = arg<u32>(ctx, 0), hp = arg<u32>(ctx, 1), t = arg<u32>(ctx, 2);
    auto sub = [](f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - b); };
    auto mul = [](f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * b); };
    auto fmadds = [](f32 a, f32 b, f32 add) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(b), static_cast<f64>(add))); };
    auto fmsubs = [](f32 a, f32 b, f32 s2) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(b), -static_cast<f64>(s2))); };
    f32 v[3][3];
    for (u32 k = 0; k < 3; ++k)
        for (u32 j = 0; j < 3; ++j) v[k][j] = sub(f32_at(t + 16 * k + 4 * j), f32_at(c + 4 * j));
    Stack frame(ctx, 32);
    const u32 v0p = frame.addr(), np = v0p + 16;  // r1 + 112 / + 128
    for (u32 j = 0; j < 3; ++j) put_f32(v0p + 4 * j, v[0][j]);
    ctx.fpr[1] = v[0][0];
    f32 e[3][3];  // e0 = v1 - v0, e1 = v2 - v1, e2 = v0 - v2
    for (u32 j = 0; j < 3; ++j) {
        e[0][j] = sub(v[1][j], v[0][j]);
        e[1][j] = sub(v[2][j], v[1][j]);
        e[2][j] = sub(v[0][j], v[2][j]);
    }
    const f32 hx = f32_at(hp), hy = f32_at(hp + 4), hz = f32_at(hp + 8);
    // Projections on the axes X x e, Y x e, Z x e.
    auto px = [&](const f32* ed, const f32* p) { return fmsubs(ed[2], p[1], mul(ed[1], p[2])); };
    auto py = [&](const f32* ed, const f32* p) { return fmsubs(ed[0], p[2], mul(ed[2], p[0])); };
    auto pz = [&](const f32* ed, const f32* p) { return fmsubs(ed[1], p[0], mul(ed[0], p[1])); };
    // min / max as the original orders them (a < b ? a, b : b, a), then
    // the radius test: separated when min > rad or max < -rad.
    auto sep = [&](f32 a, f32 b, f32 rad) {
        const f32 lo = a < b ? a : b, hi = a < b ? b : a;
        return lo > rad || hi < -rad;
    };
    for (u32 k = 0; k < 3; ++k) {
        const f32* ed = e[k];
        const f32 ax = std::fabs(ed[0]), ay = std::fabs(ed[1]), az = std::fabs(ed[2]);
        // X x e: e0 / e1 use v0 and v2, e2 v0 and v1.
        const u32 o = k == 2 ? 1 : 2;
        if (sep(px(ed, v[0]), px(ed, v[o]), fmadds(az, hy, mul(ay, hz)))) return ret(ctx, 0u);
        if (sep(py(ed, v[0]), py(ed, v[o]), fmadds(az, hx, mul(ax, hz)))) return ret(ctx, 0u);
        // Z x e: e0 / e2 use v2 and v1 (in that order), e1 v0 and v1.
        const f32 rz = fmadds(ay, hx, mul(ax, hy));
        if (k == 1 ? sep(pz(ed, v[0]), pz(ed, v[1]), rz) : sep(pz(ed, v[2]), pz(ed, v[1]), rz)) return ret(ctx, 0u);
    }
    const f32 hv[3] = {hx, hy, hz};
    for (u32 j = 0; j < 3; ++j) {
        f32 lo = v[0][j], hi = v[0][j];
        for (u32 k = 1; k < 3; ++k) {
            if (v[k][j] < lo) lo = v[k][j];
            if (v[k][j] > hi) hi = v[k][j];
        }
        ctx.fpr[1] = hi;
        if (lo > hv[j] || hi < -hv[j]) return ret(ctx, 0u);
    }
    const f32 n[3] = {fmsubs(e[0][1], e[1][2], mul(e[0][2], e[1][1])), fmsubs(e[0][2], e[1][0], mul(e[0][0], e[1][2])),
                      fmsubs(e[0][0], e[1][1], mul(e[0][1], e[1][0]))};
    for (u32 j = 0; j < 3; ++j) put_f32(np + 4 * j, n[j]);
    ctx.fpr[1] = n[0];
    const u32 r = static_cast<u32>(call(ctx, L::fn::Plane_StraddlesBox, {np, v0p, hp}));
    ret(ctx, r != 0 ? 1u : 0u);
}

// void GeomJobs_Submit_00599864()   (vtable slot) (name?)
// Splits the queued geometry (16-byte entries) into SPU jobs of 32 - 80
// entries (span / 512 rounded up to 32): a 144-byte job block (config
// +48 / +52, the pass and arena words, the output lists: the done flag,
// LOD output {0x0118E0E0, 64}, records {0x0118E580, 128}, the record
// list's enable byte) with a zeroed 240-byte buffer; each stream (the
// pass's +28[]) is padded to 16 bytes with markers and its cursor handed
// to the job (+32 + 4k, flag 0x40 on the stream while it runs). After
// WorkQueue_SubmitJobDesc per chunk, waits (sys_timer_usleep(300)) while
// the done flag is set, takes the streams' cursors back, submits the
// draw records the jobs wrote and draws the LOD outputs, then flushes the
// layer 0 marker and submits the emitter jobs.
DECOMP(00599864, GeomJobs_Submit_00599864) {
    namespace J = g21::gj;
    namespace L = g21;
    const u32 qp = load<u32>(J::kQueuePtr);
    const u32 span = load<u32>(qp) - J::kQueue;
    u32 count = span >> 4;
    if (count == 0) return ret(ctx, qp);
    const u32 scratch = static_cast<u32>(call(ctx, L::fn::Render_FrameAlloc, {240}));
    call(ctx, L::fn::memset, {scratch, 0, 240});
    const u32 job = static_cast<u32>(call(ctx, L::fn::Render_FrameAlloc, {144}));
    store<u32>(job + 16, scratch);
    store<u32>(job + 0, load<u32>(J::kConfig + 52));
    store<u32>(job + 12, J::kJobBlock);
    store<u32>(job + 4, load<u32>(J::kConfig + 48));  // lfs / stfs: raw
    ctx.fpr[1] = sly4::render::gfx11::lfs(J::kConfig + 48);
    store<u32>(job + 8, load<u32>(J::kPassInfo + 16));
    store<u32>(job + 24, load<u32>(L::kArena + 256));
    store<u8>(job + 20, load<u8>(L::kArena + 4));
    store<u32>(job + 104, J::kDone);
    store<u32>(job + 28, load<u32>(L::kArena + 20));
    store<u32>(job + 108, J::kLodOut);
    store<u32>(job + 112, 64);
    store<u32>(job + 116, J::kLodOutEnd);
    store<u32>(job + 120, J::kRecords);
    store<u32>(job + 124, 128);
    store<u32>(job + 128, J::kRecordsEnd);
    if (load<u8>(J::kRecListGuard) == 0) {
        call(ctx, L::fn::DrawRecordList_Init, {J::kRecList});
        store<u8>(J::kRecListGuard, 1);
    }
    store<u8>(job + 132, load<u8>(J::kRecList));
    store<u32>(job + 100, load<u32>(J::kStreamCount));
    atomic_put(J::kDone, 0);
    atomic_put(J::kLodOutEnd, J::kLodOut);    // the output lists' ends back to their starts
    atomic_put(J::kRecordsEnd, J::kRecords);
    for (u32 k = 0; k < load<u32>(job + 100); ++k) {
        u32 cur = load<u32>(load<u32>(J::kStreams + 4 * k) + 8);
        while (((cur + 15) & ~15u) != cur) {
            call(ctx, L::fn::PushMarker_0051300C, {load<u32>(J::kStreams + 4 * k)});
            cur = load<u32>(load<u32>(J::kStreams + 4 * k) + 8);
        }
        atomic_put(J::kCursors + 128 * k, cur);
        store<u32>(job + 32 + 4 * k, J::kCursors + 128 * k);
        const u32 st = load<u32>(J::kStreams + 4 * k);
        store<u8>(st + 12, load<u8>(st + 12) | 0x40);
    }
    u32 per = ((span >> 9) + 31) & ~31u;
    if (per <= J::kMinPer) per = J::kMinPer;
    if (!(per < J::kMaxPer)) per = J::kMaxPer;
    u32 desc = static_cast<u32>(call(ctx, L::fn::Render_FrameAlloc, {((per + count - 1) / per) << 4}));
    u32 entries = J::kQueue;
    while (count != 0) {
        const u32 chunk = per < count ? per : count;
        per = chunk;
        call(ctx, L::fn::WorkQueue_SubmitJobDesc, {job, desc, entries, chunk});
        count -= chunk;
        desc += 16;
        entries += chunk * 16;
    }
    while (load<u32>(J::kDone) != 0) syscall(ctx, J::kSysTimerUsleep, {J::kSleepUs});
    for (u32 k = 0; k < load<u32>(job + 100); ++k) {
        const u32 st = load<u32>(J::kStreams + 4 * k);
        store<u32>(st + 8, load<u32>(J::kCursors + 128 * k));
        store<u8>(st + 12, load<u8>(st + 12) & ~0x40u);
    }
    {
        const u32 d = load<u32>(J::kRecordsEnd) - J::kRecords;
        const u32 q = static_cast<u32>((static_cast<u64>(0x24924925u) * d) >> 32);  // mulhwu: d / 112
        const u32 n = ((((d - q) >> 1) + q) >> 6);
        for (u32 k = 0; k < n; ++k) {
            const u32 rec = J::kRecords + 112 * k;
            call(ctx, L::fn::DrawRecord_Submit, {load<u32>(rec + 96), rec});
        }
    }
    const u32 d = load<u32>(J::kLodOutEnd) - J::kLodOut;
    if ((d >> 4) != 0) {
        const u32 end = J::kLodOut + (d & ~15u);
        u32 e = J::kLodOut;
        do {
            const u32 o = load<u32>(e);
            if (o != 0) call(ctx, L::fn::GeomInstance_DrawLod, {o, load<u8>(e + 8), load<u8>(e + 9), (load<u32>(o + 320) >> 18) & 0x3F});
            e += 16;
        } while (e < end);
    }
    call(ctx, L::fn::Layer0_FlushMarker_0051A6D4);
    call(ctx, L::fn::EmitterJobs_Submit_00528130);
}

// void SlotsWork_Prepare_005894F8()   [f_00599154] (name?)
// Fills the slots job's work record: the view's six planes and two
// matrices (+0 / +96), rows of the view block, bank and config words, the
// eye (+2976), the output rings (reset), the counter at 0x0117E200
// cleared; flags +3093 (bit 7 cleared, bit 6 = config +34 bit 0). With
// the view's +256 bit 1 and +52 bit 2 set, each light copy from
// 0x011957E2 (count 0x011957E4) gets a 2 KiB frame buffer (+132) and a
// 128-byte block (+136, its first word the buffer), and its frustum from
// its look-at frame (w = 0 / 1) times its projection (orthographic +-w/2,
// +-h/2 when +4 is set, else Matrix_PerspectiveWH) goes to +224 + 224k,
// the block / buffer end to +3156 / +3108 + 4k; else the count is 0.
DECOMP(005894F8, SlotsWork_Prepare_005894F8) {
    namespace L = g21;
    namespace W = g21::sw;
    using sly4::render::gfx11::lfs;
    const u32 w = W::kWork;
    auto copy = [](u32 dst, u32 src, u32 n) { for (u32 k = 0; k < n; k += 4) store<u32>(dst + k, load<u32>(src + k)); };
    store<u32>(w + 2992, load<u32>(W::kPassArg));
    u32 v = static_cast<u32>(call(ctx, L::fn::View_GetCurrent));
    copy(w, v + 272, 96);
    copy(w + 96, v + 368, 128);
    for (u32 r = 0; r < 4; ++r) copy(w + 2912 + 16 * r, W::kViewBlock + 256 + 16 * r, 12);
    store<u32>(w + 2996, load<u32>(load<u32>(W::kBank + 36) + 20));
    store<u32>(w + 3000, load<u32>(W::kViewBlock + 612));
    store<u32>(w + 3004, load<u32>(W::kGrid));
    store<u32>(w + 3008, load<u16>(W::kGrid + 4));
    store<u32>(w + 3012, load<u32>(W::kConfig + 100));
    store<u32>(w + 3016, load<u32>(W::kConfig + 104));
    store<u32>(w + 3020, load<u32>(W::kConfig + 108));
    store<u8>(w + 3092, load<u8>(W::kConfig + 153));
    store<u32>(w + 3024, load<u32>(W::kConfig + 96));
    v = static_cast<u32>(call(ctx, L::fn::View_GetCurrent));
    copy(w + 2976, v + 864, 12);
    store<u32>(w + 3028, load<u32>(W::kConfig + 128));
    store<u32>(w + 3036, W::kGrid + 16);
    store<u32>(w + 3032, load<u32>(W::kConfig + 132));
    store<u32>(w + 3088, W::kGrid + 272);
    copy(w + 3040, W::kBank + 5264, 12);
    copy(w + 3056, W::kBank + 5280, 12);
    store<u32>(w + 3072, load<u32>(W::kBank + 5312));
    store<u32>(w + 3076, load<u32>(W::kBank + 5316));
    store<u8>(w + 3093, load<u8>(w + 3093) & 0x7F);
    store<u32>(w + 3080, load<u32>(W::kBank + 5320));
    ctx.fpr[1] = lfs(W::kBank + 5320);
    store<u32>(w + 3096, W::kOutA);
    store<u32>(w + 3100, W::kOutB);
    store<u32>(w + 3104, 4096);
    store<u32>(w + 3216, W::kRing);
    store<u32>(w + 3220, W::kRingStart);
    store<u32>(w + 3224, W::kRingStart + W::kRingSize);
    store<u32>(W::kRing, W::kRingStart);
    store<u32>(W::kOutB, W::kOutBInit);
    store<u32>(w + 3212, W::kCounter);
    atomic_put(W::kCounter, 0);
    store<u8>(w + 3093, static_cast<u8>((load<u8>(w + 3093) & ~0x40u) | ((load<u8>(W::kConfig + 34) & 1u) << 6)));
    v = static_cast<u32>(call(ctx, L::fn::View_GetCurrent));
    const bool lit = (load<u32>(v + 256) & 2) != 0 && ((load<u8>(v + 52) >> 2) & 1) != 0;
    if (!lit) {
        store<u32>(w + 3208, 0);
        return ret(ctx, 0u);
    }
    const u32 n = load<u16>(W::kLightCount);
    const u32 lights = W::kLights + 144 * load<u16>(W::kLightFirst);
    store<u32>(w + 3208, n);
    store<u32>(w + 3204, 512);
    u64 r3 = lights - W::kLights;
    Stack frame(ctx, 384);
    const u32 m2 = frame.addr(), proj = m2 + 64, prod = m2 + 128;  // r1 + 112 / + 176 / + 240
    const u32 persp = m2 + 192, ortho = m2 + 256, lk = m2 + 320;   // r1 + 304 / + 368 / + 432
    for (u32 k = 0; static_cast<s32>(k) < static_cast<s32>(n); k = (k + 1) & 0xFFFF) {
        const u32 l = lights + 144 * k;
        store<u32>(l + 132, static_cast<u32>(call(ctx, L::fn::Render_FrameAlloc, {W::kLightBuf})));
        const u32 blk = static_cast<u32>(call(ctx, L::fn::Render_FrameAlloc, {W::kLightBlock}));
        const u32 buf = load<u32>(l + 132);
        store<u32>(l + 136, blk);
        atomic_put(blk, buf);
        const u32 out = w + 224 + 224 * k;
        call(ctx, L::fn::Matrix_LookAt, {lk, l + 16, l + 32, l + 48});
        for (u32 r = 0; r < 4; ++r) {
            copy(m2 + 16 * r, lk + 16 * r, 12);
            store<u32>(m2 + 16 * r + 12, r == 3 ? 0x3F800000u : 0u);
        }
        u32 p;
        if (load<u32>(l + 4) != 0) {
            const f32 hw = f32_at(l + 88), hh = f32_at(l + 92);
            const f32 a = f32_at(W::kOrthoHalf), b = f32_at(W::kOrthoHalf + 4);
            auto mul = [](f32 x, f32 y) { return static_cast<f64>(static_cast<f32>(static_cast<f64>(x) * y)); };
            callf(ctx, L::fn::Mat4_Ortho, {ortho}, {mul(hw, a), mul(hw, b), mul(hh, a), mul(hh, b), lfs(l + 80), lfs(l + 84)});
            p = ortho;
        } else {
            callf(ctx, L::fn::Matrix_PerspectiveWH, {persp}, {lfs(l + 88), lfs(l + 92), lfs(l + 80), lfs(l + 84)});
            p = persp;
        }
        copy(proj, p, 64);
        ctx.fpr[1] = lfs(p);
        call(ctx, L::fn::Matrix_MultiplyTo, {prod, m2, proj});
        call(ctx, L::fn::Frustum_FromMatrix, {out, prod});
        store<u32>(w + 3156 + 4 * k, load<u32>(l + 136));
        r3 = load<u32>(l + 132) + W::kLightBuf;
        store<u32>(w + 3108 + 4 * k, static_cast<u32>(r3));
    }
    ret(ctx, r3);
}
