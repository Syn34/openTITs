// World-object entry points used by the UI's type-8 element.
#include "obj.h"

using namespace sly4;
using namespace sly4::obj;

namespace {

inline f64 lfs(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
inline f64 fsubs(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a - b)); }
inline f64 fmuls(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a * b)); }

inline void Vcall(PPUContext& ctx, u32 o, u32 slot) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    call_ptr(ctx, opd, {o, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
}

// The value in a class map (holder +16) for `hash`.
u32 FindIn(PPUContext& ctx, u32 holder, u32 finder, u32 keybuf, u32 hash) {
    store<u32>(keybuf, hash);
    return load<u32>(static_cast<u32>(call(ctx, finder, {load<u32>(load<u32>(holder) + 16), keybuf})));
}

}  // namespace

// void WorldObj_SetGroup(Obj* o, u32 group)  (name?)
// Sets bits 18-23 of +320 and tells the object (vfunc +36) when it changes.
DECOMP(00583E54, WorldObj_SetGroup) {
    const u32 o = arg<u32>(ctx, 0), group = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + kObjGroupWord);
    if (static_cast<s32>((w >> 18) & 0x3F) == static_cast<s32>(group)) return ret(ctx, group);  // r3 as the original
    store<u32>(o + kObjGroupWord, (w & ~0x00FC0000u) | ((group << 18) & 0x00FC0000u));
    Vcall(ctx, o, kObjVfnChanged);
}

// void WorldObj_SetName(Obj* o, const char* name, bool rehash)
DECOMP(0057D368, WorldObj_SetName) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    const u8 rehash = static_cast<u8>(arg<u32>(ctx, 2));
    call(ctx, ofn::String_Copy, {o + kObjName, name, 32});
    if (rehash == 0) return;
    const u32 h = static_cast<u32>(call(ctx, ofn::String_HashNoCase, {name}));
    if (load<u32>(o + kObjNameHash) != h) store<u32>(o + kObjNameHash, h);
}

// Obj* WorldObj_FindByHash(u32 hash, s32 kind)
// The object named `hash` in the class map for `kind` (11; 20 / 8; 21 /
// 9; 22 / 10), or in the first of the four maps that has it.
DECOMP(005801DC, WorldObj_FindByHash) {
    const u32 hash = arg<u32>(ctx, 0);
    const s32 kind = arg<s32>(ctx, 1);
    if (hash == 0) return ret(ctx, 0u);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 160 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    switch (kind) {
        case 22: case 10: return ret(ctx, FindIn(ctx, kMapHolderD, ofn::MapD_Find, sp + 132, hash));
        case 21: case 9: return ret(ctx, FindIn(ctx, kMapHolderC, ofn::MapC_Find, sp + 136, hash));
        case 20: case 8: return ret(ctx, FindIn(ctx, kMapHolderB, ofn::MapB_Find, sp + 140, hash));
        case 11: return ret(ctx, FindIn(ctx, kMapHolderA, ofn::MapA_Find, sp + 128, hash));
        default: break;
    }
    u32 o = FindIn(ctx, kMapHolderA, ofn::MapA_Find, sp + 124, hash);
    if (o == 0) o = FindIn(ctx, kMapHolderB, ofn::MapB_Find, sp + 120, hash);
    if (o == 0) o = FindIn(ctx, kMapHolderC, ofn::MapC_Find, sp + 116, hash);
    if (o == 0) o = FindIn(ctx, kMapHolderD, ofn::MapD_Find, sp + 112, hash);
    ret(ctx, o);
}

// void WorldObj_SetScale(Obj* o, const f32 scale[3], bool recurse)
// Does nothing when every component is within 1e-5 of the current one.
// Otherwise stores it, flags a non-unit scale (0x2000), marks the object
// and its parent dirty (queueing the parent on the dirty list once),
// optionally recurses into the children, and calls vfunc +36.
DECOMP(0057D100, WorldObj_SetScale) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u8 recurse = static_cast<u8>(arg<u32>(ctx, 2));
    const f64 eps = lfs(kScaleEpsilon);
    bool same = true;
    for (u32 k = 0; k < 12 && same; k += 4) same = std::fabs(fsubs(lfs(o + kObjScale + k), lfs(s + k))) < eps;
    if (same) return;

    for (u32 k = 0; k < 12; k += 4) store<u32>(o + kObjScale + k, load<u32>(s + k));  // lfs / stfs pairs
    const f64 s0 = lfs(o + kObjScale), s1 = lfs(s + 4), s2 = lfs(s + 8);  // as the original keeps them
    const bool unit = std::fabs(fsubs(s0, lfs(kOneVec3))) < eps && std::fabs(fsubs(s1, lfs(kOneVec3 + 4))) < eps &&
                      std::fabs(fsubs(s2, lfs(kOneVec3 + 8))) < eps;
    u32 w = (load<u32>(o + kObjFlags) & ~kObjNonUnitScale) | (unit ? 0 : kObjNonUnitScale);
    store<u32>(o + kObjFlags, w);
    if ((w & kObjDirty) == 0) {
        const u32 p = load<u32>(o + kObjParent);  // no null check, as in the original
        u32 pw = load<u32>(p + kObjFlags);
        if ((pw & kObjDirty) == 0) {
            if ((pw & kObjQueuedFlag) == 0 && load<u32>(p + kObjQueued) == 0 && load<u8>(kDirtyListOff) == 0) {
                const u32 v = load<u32>(kDirtyList);
                if (load<u16>(v + 8) == load<u16>(v + 4))
                    call(ctx, ofn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + 4)) << 1) & ~1u});
                store<u32>(load<u32>(v) + load<u16>(v + 8) * 4u, p);
                const u32 n = load<u16>(v + 8) + 1u;
                const u32 hi = load<u16>(v + 10);
                store<u16>(v + 8, static_cast<u16>(n));
                store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n));
                pw = load<u32>(p + kObjFlags) | kObjQueuedFlag;
            }
            store<u32>(p + kObjFlags, pw | kObjDirty);
            w = load<u32>(o + kObjFlags);
        }
        store<u32>(o + kObjFlags, w | kObjDirty);
    }
    if (recurse != 0)
        for (u32 c = load<u32>(o + kObjFirstChild); c != 0; c = load<u32>(c + kObjNextSibling))
            call(ctx, ofn::WorldObj_SetScale, {c, s, recurse});
    Vcall(ctx, o, kObjVfnChanged);
}

// void UiElem8_Attach(Element8* e, Obj* o, bool recurse)
// Gives each child of `o` the element's object's scale, group 12 and its
// translation scaled by that scale (z negated), then updates its matrix;
// optionally the same for their children.
DECOMP(004D7CF8, UiElem8_Attach) {
    const u32 e = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u8 recurse = static_cast<u8>(arg<u32>(ctx, 2));
    for (u32 c = load<u32>(o + kObjFirstChild); c != 0; c = load<u32>(c + kObjNextSibling)) {
        call(ctx, ofn::WorldObj_SetScale, {c, load<u32>(e + 168) + kObjScale, 0});
        {
            const u32 opd = load<u32>(load<u32>(c) + kObjVfnSet40);
            call_ptr(ctx, opd, {c, 0, 0, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
        }
        call(ctx, ofn::WorldObj_SetGroup, {c, 12});
        const u32 root = load<u32>(e + 168);
        const f64 x = fmuls(lfs(c + 64), lfs(root + kObjScale));
        const f64 y = fmuls(lfs(c + 68), lfs(root + kObjScale + 4));
        const f64 z = -fmuls(lfs(c + 72), lfs(root + kObjScale + 8));
        // The original also writes the three matrix rows back unchanged (lfs / stfs pairs).
        for (u32 off : {16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u}) store<u32>(c + off, load<u32>(c + off));
        store<f32>(c + 64, static_cast<f32>(x));
        store<f32>(c + 68, static_cast<f32>(y));
        store<f32>(c + 72, static_cast<f32>(z));
        // r4..r6 = root +148 / +152 / +144 here in the original; the callee reads only r3.
        call(ctx, ofn::WorldObj_MarkDirty, {c});
        if (recurse != 0) call(ctx, ofn::UiElem8_Attach, {e, c, recurse});
    }
}

namespace {

namespace ifn {
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 FreeList_Init = 0x00551A58;
constexpr u32 WorldObj_Construct = 0x00580708;  // (obj, s32 kind) (name?)
constexpr u32 WorldClass_Copy = 0x005858D4;     // (cls) -> a copy to instance from (name?)
constexpr u32 Light_Alloc = 0x00526494;         // () -> AglLight for +288
}  // namespace ifn

constexpr u32 kObjListVtbl = 0x00B0F008;     // ApeTFreeList<...> of 352-byte objects
constexpr u32 kStrObjList = 0x00AF4B7E;      // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = Ape..."
constexpr u32 kInstanceVtbl = 0x00B24858;
constexpr u32 kInstanceSize = 352;
constexpr u32 kObjExtra = 288, kObjClassData = 292;
constexpr u32 kVfnSetClass = 44;   // (cls)
constexpr u32 kVfnName = 12;       // () -> const char*
constexpr u32 kVfnRelease = 8;

}  // namespace

// Obj* WorldObj_Instance(Class* cls, bool copy_class)
// A new object of kind 8 (vtable 0x00B24858) from the free list of class
// map holder B (made on first use), set to the class (a copy of it when
// `copy_class`, released afterwards) and named after it. No null check on
// the allocation, as in the original.
DECOMP(005A05E8, WorldObj_Instance) {
    u32 cls = arg<u32>(ctx, 0);
    const u8 copy_class = static_cast<u8>(arg<u32>(ctx, 1));
    const u32 list = load<u32>(kMapHolderB);
    u32 alloc = load<u32>(list);
    if (alloc == 0) {
        const u32 obj = static_cast<u32>(call(ctx, ifn::Mem_Alloc, {60, 0, 16}));
        if (obj != 0) {
            const u32 capacity = load<u32>(list + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
            const u16 h54 = load<u16>(obj + 54);
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
            store<u16>(obj + 52, 0);
            store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(obj, kObjListVtbl);
            store<u32>(obj + 56, kStrObjList);
            call(ctx, ifn::FreeList_Init, {obj, kInstanceSize, capacity, 0, 0});
        }
        alloc = obj;
        store<u32>(list, alloc);
    }
    u32 o = 0;
    {
        const u32 opd = load<u32>(load<u32>(alloc) + 12);
        const u32 m = static_cast<u32>(call_ptr(ctx, opd, {alloc, opd, load<u32>(opd)}));  // r4 / r5 = OPD and entry
        if (m != 0) {
            call(ctx, ifn::WorldObj_Construct, {m, 8});
            store<u32>(m, kInstanceVtbl);
            o = m;
        }
    }
    bool owns = false;
    if (cls != 0 && copy_class != 0) {
        cls = static_cast<u32>(call(ctx, ifn::WorldClass_Copy, {cls}));
        owns = true;
    }
    {
        const u32 opd = load<u32>(load<u32>(o) + kVfnSetClass);
        call_ptr(ctx, opd, {o, cls, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
    }
    const u32 name = static_cast<u32>(call_ptr(ctx, load<u32>(load<u32>(cls) + kVfnName),
                                               {cls, load<u32>(load<u32>(cls) + kVfnName),
                                                load<u32>(load<u32>(load<u32>(cls) + kVfnName))}));
    call(ctx, ofn::String_Copy, {o + kObjName, name, 32});
    const u32 h = static_cast<u32>(call(ctx, ofn::String_HashNoCase, {name}));
    const u32 extra = load<u32>(o + kObjExtra);
    if (load<u32>(o + kObjNameHash) != h) store<u32>(o + kObjNameHash, h);
    store<u32>(o + kObjClassData, load<u32>(cls + 104));
    if (extra != 0) store<u8>(extra + 212, static_cast<u8>(load<u8>(extra + 212) & 0x7F));
    if (load<u32>(o + kObjClassData) != 0 && load<u32>(o + kObjExtra) == 0) {
        store<u32>(o + kObjExtra, static_cast<u32>(call(ctx, ifn::Light_Alloc)));
        Vcall(ctx, o, kObjVfnChanged);
    }
    if (owns) Vcall(ctx, cls, kVfnRelease);
    ret(ctx, o);
}
