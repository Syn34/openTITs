// World_HandleEvent callees, part 29f: the level's geometry instance
// "styles": up to four styled instances per definition, released and
// rebuilt from the level's geometry list.
#include "hevent29.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;
using namespace sly4::hevent29;

namespace {
namespace fn29f {
constexpr u32 Style_Apply = 0x000D6D28;         // (u32 key, Target*) -> instance (Style_Apply0D6D28)
constexpr u32 Nop = 0x0054EC18;
constexpr u32 PtrVector_Clear = 0x0054EC1C;     // (vec)
constexpr u32 SmallFree = 0x00552420;           // (p, size)
constexpr u32 String_Copy = 0x00553684;         // (dst, src, size)
constexpr u32 String_HashNoCase = 0x0056A710;   // (const char*) -> u32
constexpr u32 WorldObj_Set292 = 0x00580AF0;     // (obj, u32)
constexpr u32 PointMap_Find = 0x0073343C;       // (map, const u32* key) -> value slot
constexpr u32 CameraDefMap_Find = 0x0073374C;   // (map, const u32* key) -> value slot
}  // namespace fn29f

// World fields: u8 count at +0x128610, then 36-byte records.
constexpr u32 kStyleCount = 0x128610;
constexpr u32 kStyles = 0x128614, kStyleSize = 36;
constexpr u32 kStGeom = 0, kStMaterial = 4, kStCamDef = 8, kStColorA = 12, kStColorB = 16, kStInst = 20;  // +20 Instance*[4]
constexpr u32 kWLevelGeom = 0x128164;  // Level*: +5448 -> {+0 Def[84], u16 +8 count}
constexpr u32 kGeomList = 5448;
constexpr u32 kDefSize = 84;
// Definition fields: +0 geometry name hash, +4 material key, +8 camera def
// key, +12 / +16 u8[4], then four 16-byte entries at +16: +8 style key
// (entry +24), +4 name (+20), +12 tint key (+28), +16 point key (+32).
constexpr u32 kDefEntry = 16;
// Instance fields.
constexpr u32 kInstFlags = 172;   // bit 0x00040000: vt +40 (1) on release
constexpr u32 kInstRefs = 178;    // u16
constexpr u32 kInstLight = 160;   // WorldObj* (kind 8: WorldObj_Set292 0xD000)
constexpr u32 kInstNext = 164;    // u16 +40 bit 0x400 cleared
// Allocators (ApeTListAllocator<T> slots).
constexpr u32 kGeomAllocSlot = 0x00F8F144, kMatAllocSlot = 0x00F8F114;
constexpr u32 kVtGeomFreeList = 0x00B0F008, kVtMatFreeList = 0x00B10ED0;
constexpr u32 kStrGeomFreeList = 0x00AE54E6;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeGeomInstance]"
constexpr u32 kStrMatFreeList = 0x00AE5499;   // "... [with T = AglMaterial]"
constexpr u32 kGeomNodeSize = 352, kMatNodeSize = 144, kNodeBias = 16;
constexpr u32 kPointDefSlot = 0x00F8F154;     // +16 point map
// The style descriptor handed to Style_Apply0D6D28 (on the stack).
constexpr u32 kVtStyle = 0x00B12260;
constexpr u32 kIdentityRot = 0x01164BD0, kZeroPos = 0x01164B30;
constexpr u32 kStrEmpty = 0x00AE65E4;  // ""
constexpr u32 kTint = 0x0046B9E8;      // f32 1, 0.25, 0
constexpr u32 kSetLight = 0xD000;

// ApeTListAllocator<T>::GetAllocator() (inlined, as hevent28.h
// camdef_allocator): a failed allocation stores 0 and is returned.
u32 list_allocator(PPUContext& ctx, u32 slot_ea, u32 vt, u32 node_size, u32 name) {
    const u32 slot = load<u32>(slot_ea);
    const u32 alloc = load<u32>(slot);
    if (alloc != 0) return alloc;
    const u32 fl = static_cast<u32>(call(ctx, h27fn::Mem_Alloc, {60, 0, 16}));
    if (fl != 0) {
        const u32 per_block = load<u32>(slot + 4);
        store<u32>(fl + 8, 0);
        store<u32>(fl + 4, 0);
        store<u32>(fl + 12, 0);
        store<u32>(fl + 20, 0);
        store<u32>(fl + 16, 0);
        const u16 flags = load<u16>(fl + 54);
        store<u32>(fl + 24, 0);
        store<u32>(fl + 32, 0);
        store<u32>(fl + 28, 0);
        for (u32 k = 36; k <= 48; k += 4) store<u32>(fl + k, 0);
        store<u16>(fl + 52, 0);
        store<u16>(fl + 54, static_cast<u16>(flags & 0x7FFF));
        store<u32>(fl + 0, vt);
        store<u32>(fl + 56, name);
        call(ctx, h27fn::FreeList_Init, {fl, node_size, per_block, 0, 0});
    }
    store<u32>(slot, fl);
    return fl;
}

// The first live node of an allocator (Obj_GetField4), as its object.
u32 first_obj(PPUContext& ctx, u32 alloc) {
    const u32 n = static_cast<u32>(call(ctx, h27fn::Obj_GetField4, {alloc}));
    return n != 0 ? n + kNodeBias : 0;
}
u32 next_obj(u32 o) {
    const u32 n = load<u32>(o - kNodeBias);
    return n != 0 ? n + kNodeBias : 0;
}
}  // namespace

// u32 World_BuildStyles46B9F8(World* w) (name?): the styled instances of
// every record (+0x128614, count +0x128610) released (vt +88 (0, 1, 1,
// 1), vt +40 (0), and (1) with +172 bit 0x40000; the first null slot ends
// a record) and the count cleared; then, from the level's geometry list
// (+0x128164 +5448), each record gets the live geometry instance whose
// name (vt +12) hashes to the def's +0, the material with the def's key
// (+40) and a positive +4, the camera def and two colours, and up to four
// instances made by Style_Apply0D6D28 from a stack style (identity
// matrix, or the point's, a name) - each referenced (+178), vt +96 (1),
// vt +88 (0, 1, 1, 1), its kind-8 light set (0xD000, +322 bit 0x80 -> vt
// +36), tinted (vt +200 (key, 0, 1; 1, 0.25, 0)) and its +164 +40 bit
// 0x400 cleared. r3 is what the original leaves (see the returns).
DECOMP(0046B9F8, World_BuildStyles46B9F8) {
    const u32 w = arg<u32>(ctx, 0);
    Stack frame(ctx, 208);  // +0 style (180 bytes, name at +140), +192 / +196 keys
    const u32 style = frame.addr(), name = style + 140, pkey = style + 192, ckey = style + 196;

    for (u32 i = 0; i < load<u8>(w + kStyleCount); ++i) {
        for (u32 j = 0; j < 4; ++j) {
            const u32 slot = w + kStyles + kStyleSize * i + kStInst + 4 * j;
            const u32 o = load<u32>(slot);
            if (o == 0) break;
            vcall(ctx, o, 88, {o, 0, 1, 1, 1});
            vcall(ctx, o, 40, {o, 0});
            if ((load<u32>(o + kInstFlags) & 0x00040000u) != 0) vcall(ctx, o, 40, {o, 1});
            store<u32>(slot, 0);
        }
    }
    store<u8>(w + kStyleCount, 0);

    const u32 lvl = load<u32>(w + kWLevelGeom);
    if (lvl == 0) {
        ctx.gpr[3] = 0x00120000;  // (the offset's high half, as left)
        return;
    }
    const u32 list = load<u32>(lvl + kGeomList);
    if (list == 0 || load<u16>(list + 8) == 0) {
        ctx.gpr[3] = lvl + kGeomList;
        return;
    }
    const u16 n = load<u16>(list + 8);
    store<u8>(w + kStyleCount, static_cast<u8>(n));
    if ((n & 0xFF) == 0) {
        ctx.gpr[3] = n;
        return;
    }

    for (u32 i = 0; i < load<u8>(w + kStyleCount); ++i) {
        const u32 rec = w + kStyles + kStyleSize * i;
        const u32 def = load<u32>(load<u32>(load<u32>(w + kWLevelGeom) + kGeomList)) + kDefSize * i;

        // The geometry instance with the def's name.
        u32 g = first_obj(ctx, list_allocator(ctx, kGeomAllocSlot, kVtGeomFreeList, kGeomNodeSize, kStrGeomFreeList));
        for (; g != 0; g = next_obj(g)) {
            const u32 nm = static_cast<u32>(vcall(ctx, g, 12, {g}));
            if (static_cast<u32>(call(ctx, fn29f::String_HashNoCase, {nm})) == load<u32>(def + 0)) break;
        }
        store<u32>(rec + kStGeom, g);

        // The material.
        u32 mat = 0;
        const u32 mkey = load<u32>(def + 4);
        if (mkey != 0) {
            u32 m = first_obj(ctx, list_allocator(ctx, kMatAllocSlot, kVtMatFreeList, kMatNodeSize, kStrMatFreeList));
            for (; m != 0; m = next_obj(m))
                if (load<u32>(m + 40) == mkey) break;
            mat = m;
            if (mat != 0 && static_cast<s16>(load<u16>(mat + 4)) <= 0) mat = 0;
        }
        store<u32>(rec + kStMaterial, mat);

        store<u32>(ckey, load<u32>(def + 8));
        const u32 cd = load<u32>(static_cast<u32>(call(ctx, fn29f::CameraDefMap_Find, {load<u32>(load<u32>(hevent28::kCamDefs) + 16), ckey})));
        store<u32>(rec + kStCamDef, cd);
        for (u32 k = 0; k < 4; ++k) store<u8>(rec + kStColorA + k, load<u8>(def + 12 + k));
        for (u32 k = 0; k < 4; ++k) store<u8>(rec + kStColorB + k, load<u8>(def + 16 + k));
        const f64 t1 = lfs_at(kTint), t2 = lfs_at(kTint + 4), t3 = lfs_at(kTint + 8);

        for (u32 j = 0; j < 4; ++j) {
            const u32 e = def + kDefEntry * j;  // entry fields at e +20 .. +32
            u32 inst = 0;
            if (load<u32>(e + 24) != 0) {
                // The style: vtable, matrix rows (identity / the point's),
                // position, flags (the stack word's low 21 bits kept, as
                // the original reads it uninitialised), name.
                store<u32>(style + 0, kVtStyle);
                store<u32>(style + 16, 0);
                for (u32 r = 0; r < 4; ++r)
                    for (u32 k = 0; k < 12; k += 4) store<u32>(style + 32 + 16 * r + k, load<u32>(kIdentityRot + 16 * r + k));
                const u32 old = load<u32>(style + 132);
                for (u32 k = 0; k < 12; k += 4) store<u32>(style + 96 + k, load<u32>(kZeroPos + k));
                store<u32>(style + 112, 0);
                store<u32>(style + 116, 0);
                store<u32>(style + 120, 0);
                store<u32>(style + 124, 0xBF800000u);
                store<u32>(style + 128, 0xBF800000u);
                store<u32>(style + 132, ((old | 0x80000000u) & ~0x60000000u) | 0x1FE00000u);
                store<u32>(style + 136, 0);
                store<u32>(style + 4, 0);
                store<u32>(style + 8, 0);
                store<u32>(style + 12, 0);
                store<u8>(name, 0);
                ctx.fpr[2] = lfs_at(kZeroPos + 4);  // f1 / f2 as the copies leave them
                ctx.fpr[1] = lfs_at(kZeroPos + 8);
                call(ctx, fn29f::String_Copy, {name, kStrEmpty, 40});
                const u32 nm = load<u32>(e + 20);
                if (nm != 0) call(ctx, fn29f::String_Copy, {name, nm, 39});
                store<u32>(pkey, load<u32>(e + 32));
                const u32 pt = load<u32>(static_cast<u32>(call(ctx, fn29f::PointMap_Find, {load<u32>(load<u32>(kPointDefSlot) + 16), pkey})));
                if (pt != 0) {
                    for (u32 r = 0; r < 4; ++r)
                        for (u32 k = 0; k < 12; k += 4) store<u32>(style + 32 + 16 * r + k, load<u32>(pt + 80 + 16 * r + k));
                    ctx.fpr[1] = lfs_at(pt + 80 + 56);  // f1: the last word copied
                }
                inst = static_cast<u32>(call(ctx, fn29f::Style_Apply, {load<u32>(e + 24), style}));
                const u32 vec = load<u32>(style + 16);
                store<u32>(style + 0, kVtStyle);
                if (vec != 0) {
                    call(ctx, fn29f::PtrVector_Clear, {vec});
                    call(ctx, fn29f::Nop, {vec});
                    call(ctx, fn29f::SmallFree, {load<u32>(style + 16), 20});
                }
            }
            store<u32>(rec + kStInst + 4 * j, inst);
            if (inst == 0) continue;

            store<u16>(inst + kInstRefs, static_cast<u16>(load<u16>(inst + kInstRefs) + 1));
            vcall(ctx, inst, 96, {inst, 1});
            vcall(ctx, inst, 88, {inst, 0, 1, 1, 1});
            const u32 light = load<u32>(inst + kInstLight);
            if (light != 0 && ((load<u32>(light + 4) >> 10) & 63) == 8) {
                call(ctx, fn29f::WorldObj_Set292, {light, kSetLight});
                const u8 b = load<u8>(light + 322);
                if ((b & 0x80) != 0) {
                    store<u8>(light + 322, static_cast<u8>(b & ~0x80));
                    vcall(ctx, light, 36, {light});
                }
            }
            const u32 tint = load<u32>(e + 28);
            if (tint != 0) {
                // vt +200 (inst, key, 0, 1, -, -, 0, r10 as left, stack 0; 1, 0.25, 0):
                // r7 / r8 hold the OPD and its entry, as the call sequence leaves them.
                const u32 opd = load<u32>(load<u32>(inst) + 200);
                ctx.fpr[1] = t1;
                ctx.fpr[2] = t2;
                ctx.fpr[3] = t3;
                call_ptr(ctx, opd, {inst, tint, 0, 1, opd, load<u32>(opd), 0, ctx.gpr[10], 0});
            }
            const u32 nx = load<u32>(inst + kInstNext);
            if (nx != 0) store<u16>(nx + 40, static_cast<u16>(load<u16>(nx + 40) & ~0x400u));
        }
    }
    ctx.gpr[3] = load<u8>(w + kStyleCount);
}
