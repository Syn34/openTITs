// World objects, part 4: the resources the object destructor gives back
// (lights, skeletons, the root's shared block) and the texture root
// constructor.
#include "obj4.h"

using namespace sly4;
using namespace sly4::obj4;

namespace {

// The ApeTFreeList header every lazily made free list starts with, then
// FreeList_Init(list, size, count, 0, 0). 0 if Mem_Alloc fails.
u32 NewFreeList(PPUContext& ctx, u32 vtbl, u32 name, u32 elem_size, u32 count) {
    const u32 obj = static_cast<u32>(call(ctx, o4fn::Mem_Alloc, {60, 0, 16}));
    if (obj == 0) return 0;
    for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(obj + off, 0);
    const u16 h54 = load<u16>(obj + 54);
    for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
    store<u16>(obj + 52, 0);
    store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
    store<u32>(obj, vtbl);
    store<u32>(obj + 56, name);
    // r8 = the name here in the original; FreeList_Init reads r3..r7 only.
    call(ctx, o4fn::FreeList_Init, {obj, elem_size, count, 0, 0});
    return obj;
}

// alloc->Free(p) (slot +16): r5 / r6 = the OPD and entry.
void FreeListFree(PPUContext& ctx, u32 alloc, u32 p) {
    const u32 opd = load<u32>(load<u32>(alloc) + 16);
    call_ptr(ctx, opd, {alloc, p, opd, load<u32>(opd)});
}

}  // namespace

// void Light_Free(AglLight* l)  ("Texture_ReleaseRef" in symbols.txt)
// Gives an AglLight (object +288) back to its free list at *0x010DDC84,
// making the list on first use. As in the original, a failed Mem_Alloc
// leaves 0 stored and the call goes through address 0.
DECOMP(0050CAE8, Light_Free) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 header = load<u32>(obj2::kLightList);
    u32 alloc = load<u32>(header);
    if (alloc == 0) {
        alloc = NewFreeList(ctx, obj2::kLightListVtbl, obj2::kStrLightList, obj2::kLightSize, load<u32>(header + 4));
        store<u32>(header, alloc);
    }
    FreeListFree(ctx, alloc, l);
}

// void Skeleton_Free(ApeSkeleton* s)  ("WorldObj_FreeExtra300" in symbols.txt)
// Clears the skeleton from the watch list (0x01194590) if it is flagged
// there (+31 bit 0x20), frees its +20 block (size +24) and gives it back
// to the ApeSkeleton free list (made on first use).
DECOMP(005807D8, Skeleton_Free) {
    const u32 s = arg<u32>(ctx, 0);
    if (s == 0) return;
    if ((load<u8>(s + 31) & 0x20) != 0) {
        const u32 list = load<u32>(kSkeletonWatch);
        const u32 n = load<u32>(list + 4);
        const u32 data = load<u32>(list);
        for (u32 i = 0; i < n; ++i)
            if (load<u32>(data + i * 4) == s) {
                store<u32>(data + ((i << 2) & 0xFFFFFFFCu), 0);
                break;
            }
    }
    const u32 block = load<u32>(s + 20);
    if (block != 0) {
        call(ctx, o4fn::SmallFree, {block, load<u32>(s + 24)});
        store<u32>(s + 20, 0);
    }
    u32 alloc = load<u32>(kSkeletonList);
    if (alloc == 0) {
        alloc = NewFreeList(ctx, kSkeletonListVtbl, kStrSkeletonList, 64, 128);
        store<u32>(kSkeletonList, alloc);  // 0 if Mem_Alloc failed: the original then calls through 0
    }
    FreeListFree(ctx, alloc, s);
}

// void ApeRoot_Destruct(ApeRoot* o)  ("WorldObj_DestructRoot" in symbols.txt)
// The root destructor every object ends in (vtable 0x00B242B8): calls the
// destroy hook (0x01164CD4) for objects with +8 bits 6-9 set, then drops a
// reference to the shared block at +12. On its last reference the block's
// four sub-blocks (+16, +12, +8, +4) are dropped too (each freed with its
// +28 buffer on its last reference) and the block is freed.
DECOMP(00558CE8, ApeRoot_Destruct) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w8 = load<u32>(o + 8);
    store<u32>(o, kVtblRoot);
    if (((w8 >> 6) & 0xF) != 0) {
        const u32 hook = load<u32>(kRootDestroyHook);
        if (hook != 0) call_ptr(ctx, hook, {o, hook, load<u32>(hook)});  // r4 / r5 = OPD and entry
    }
    const u32 shared = load<u32>(o + 12);
    if (shared == 0) return;
    const u16 refs = static_cast<u16>(load<u16>(shared) - 1);
    store<u16>(shared, refs);
    if (static_cast<s16>(refs) > 0) return;
    for (u32 off = 16; off >= 4; off -= 4) {
        const u32 sub = load<u32>(shared + off);
        if (sub == 0) continue;
        const u16 r = static_cast<u16>(load<u16>(sub) - 1);
        store<u16>(sub, r);
        if (static_cast<s16>(r) > 0) continue;
        call(ctx, o4fn::Mem_FreePool, {load<u32>(sub + 28), 0, 0});
        call(ctx, o4fn::Mem_FreePool, {sub, 0, 0});
    }
    call(ctx, o4fn::Mem_FreePool, {shared, 0, 0});
}

// void Texture_ConstructRoot(Texture* t)  (name?)
// The root constructor (vtable 0x00B242B8) as kind 1: +4 = {u16 1, kind 1
// in bits 10-15}, a new object serial (0x01164CD0) in bits 11-31 of +8
// (0x400 / 0x3C0 cleared), then vtable 0x00B23380 and the defaults: +60
// bit 0 set, +64 = 2, +68 top nibble cleared, u16 1/1/1 at +116, 3/3/3 at
// +122, 7 at +128, and the next texture serial (0x010DD594) at +112.
// Leaves r4..r10 as the original does.
DECOMP(0050A2F0, Texture_ConstructRoot) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 w4 = (load<u32>(t + 4) & 0x000003FFu) | 0x00010000u | (1u << 10);
    store<u16>(t + 4, 1);
    store<u32>(t + 4, w4);
    const u32 serial = load<u32>(obj2::kObjSerial) + 1;
    store<u32>(obj2::kObjSerial, serial);
    u32 w8 = (load<u32>(t + 8) & 0x7FFu) | ((serial << 11) & 0xFFFFF800u);
    const u32 w60 = load<u32>(t + 60);
    store<u32>(t, kVtblRoot);
    w8 &= ~0x400u;
    const u32 w68 = load<u32>(t + 68) & 0x0FFFFFFFu;
    store<u32>(t + 48, 0);
    store<u32>(t + 68, w68);
    w8 &= ~0x3C0u;
    store<u32>(t + 60, (w60 & 1u) | 1u);
    store<u32>(t, kVtblTexture);
    store<u32>(t + 64, 2);
    store<u32>(t + 56, 0);
    store<u32>(t + 96, 0);
    store<u8>(t + 16, 0);
    store<u32>(t + 108, 0);
    store<u16>(t + 52, 0);
    store<u16>(t + 54, 0);
    store<u16>(t + 116, 1);
    store<u32>(t + 8, w8);
    store<u16>(t + 118, 1);
    store<u16>(t + 120, 1);
    const u32 tex_serial = load<u32>(kTextureSerial);
    store<u16>(t + 122, 3);
    store<u16>(t + 124, 3);
    store<u16>(t + 126, 3);
    store<u16>(t + 128, 7);
    store<u32>(t + 12, 0);
    store<u32>(kTextureSerial, tex_serial + 1);
    store<u32>(t + 112, tex_serial);
    ctx.gpr[4] = tex_serial;
    ctx.gpr[5] = w68;
    ctx.gpr[6] = 3;
    ctx.gpr[7] = 0;
    ctx.gpr[8] = static_cast<u64>(tex_serial) + 1;  // addic: a 64-bit sum
    ctx.gpr[9] = 7;
    ctx.gpr[10] = 2;
}
