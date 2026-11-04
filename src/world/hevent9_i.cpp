// World_HandleEvent callees, part 9i: the per-frame object update.
#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9i {
namespace ifn9 {
constexpr u32 SortedMap_Remove = 0x00771DD0;       // (map, const u32* key)
constexpr u32 Hashed_Make = 0x003651F0;            // (u32 hash) -> Object*
constexpr u32 Objects_Flush365360 = 0x00365360;    // () (name?)
constexpr u32 Objects_Flush364980 = 0x00364980;    // () (name?)
constexpr u32 Highlight_Update363B78 = 0x00363B78; // (Highlight*; f32 dt) (name?)
constexpr u32 Obj_Prepare3629D8 = 0x003629D8;      // (Object*): leaves its kind list
}  // namespace ifn9

// The objects freed at the next update: a PtrVector {+0 Object**, +8 u16
// count} and a map {+2 u16 count, +8 {key, Object*}[]} (as hevent9_b.cpp).
constexpr u32 kDoomed = 0x0106C294, kDoomedMap = 0x0106C240;
constexpr u32 kHashedObj = 0x00F64CBC;       // Object* made by Hashed_Make (as hevent3_b.cpp)
constexpr u32 kFlush365360 = 0x0106C270;     // u32: run 0x00365360 (name?)
constexpr u32 kFlag_F64CB4 = 0x00F64CB4;     // u32 cleared with it (name?)
constexpr u32 kFlush364980 = 0x0106C27C;     // u32: run 0x00364980 when the world has no layers (name?)
constexpr u32 kHighlight = 0x0106C330;
constexpr u32 kWorldLayers = 0x128214;       // u16
constexpr u32 kWorldVState = 32;             // World vtable: (World*, 1) -> state (12: busy) (name?)
// Object: +8 def (+68 handle), +12 handle, +24 parent (+164 bit 15: keeps
// its children listed), +160 flags (bit 0: updated).
constexpr u32 kObjDef = 8, kObjHandle = 12, kObjParent = 24, kObjFlags160 = 160, kParentFlags = 164;
constexpr u32 kDefHandle = 68;
constexpr u32 kVRelease = 4, kVUpdate = 8;
constexpr u32 kKinds = 4;
}  // namespace hevent9i
}  // namespace sly4

using namespace sly4::hevent9i;

namespace {
// An object's handle: its own (+12), else its def's (+68), else 0; null
// objects are read through as in the original.
u32 handle_of(u32 o) {
    const u32 h = load<u32>(o + kObjHandle);
    if (h != 0) return h;
    const u32 def = load<u32>(o + kObjDef);
    return def != 0 ? load<u32>(def + kDefHandle) : 0;
}

void map_remove(PPUContext& ctx, u32 slot, u32 key) {
    store<u32>(slot, key);
    call(ctx, ifn9::SortedMap_Remove, {kHandleMap, slot});
}
}  // namespace

// void Objects_Update3655D0(f32 dt) (name?): frees the doomed objects (both
// sets: out of the handle map, vtable +4), keeps the hashed-name object in
// step (when one was asked for, the world has no layers and its state is not
// 12: replaced when its handle differs, then the request is cleared), runs
// the two pending flushes, updates the highlight and every handle-map object
// with +160 bit 0 (vtable +8 with dt), and takes out of the four kind lists
// each object whose parent is missing or lacks +164 bit 15. r3 as the
// original leaves it (the last list's count).
DECOMP(003655D0, Objects_Update3655D0) {
    const f64 dt = farg(ctx, 0);
    Stack frame(ctx, 16);  // the keys at r1+112, +116, +120
    const u32 key = frame.addr();
    for (u32 i = 0, off = 0; i < load<u16>(kDoomed + 8); ++i, off += 4) {
        map_remove(ctx, key + 0, handle_of(load<u32>(load<u32>(kDoomed) + off)));
        const u32 o = load<u32>(load<u32>(kDoomed) + off);
        if (o != 0) {
            vcall(ctx, o, kVRelease, {o});
            store<u32>(load<u32>(kDoomed) + off, 0);
        }
    }
    store<u16>(kDoomed + 8, 0);
    for (s32 i = 0; i < static_cast<s32>(load<u16>(kDoomedMap + 2)); ++i) {
        const u32 o = load<u32>(load<u32>(kDoomedMap + 8) + static_cast<u32>(i) * 8 + 4);
        map_remove(ctx, key + 4, handle_of(o));
        if (o != 0) vcall(ctx, o, kVRelease, {o});
    }
    const u32 want = load<u8>(kHashedNameSet);
    store<u16>(kDoomedMap + 2, 0);

    if (want != 0) {
        const u32 w = load<u32>(kCurrentWorld);
        if (load<u16>(w + kWorldLayers) == 0 && static_cast<u32>(vcall(ctx, w, kWorldVState, {w, 1})) - 12 != 0) {
            // Hashed_Refresh (0x003654C0) inlined.
            u32 h = load<u32>(kHashedObj);
            bool make = h == 0;
            if (!make && handle_of(h) != load<u32>(kHashedName)) {
                map_remove(ctx, key + 8, handle_of(h));
                h = load<u32>(kHashedObj);
                if (h != 0) {
                    vcall(ctx, h, kVRelease, {h});
                    store<u32>(kHashedObj, 0);
                }
                make = true;
            }
            if (make) store<u32>(kHashedObj, static_cast<u32>(call(ctx, ifn9::Hashed_Make, {load<u32>(kHashedName)})));
            store<u8>(kHashedNameSet, 0);
            store<u32>(kHashedName, 0);
        }
    }
    if (load<u32>(kFlush365360) != 0) {
        call(ctx, ifn9::Objects_Flush365360);
        store<u32>(kFlush365360, 0);
        store<u32>(kFlag_F64CB4, 0);
    }
    if (load<u32>(kFlush364980) != 0 && load<u16>(load<u32>(kCurrentWorld) + kWorldLayers) == 0) {
        call(ctx, ifn9::Objects_Flush364980);
        store<u32>(kFlush364980, 0);
    }
    callf(ctx, ifn9::Highlight_Update363B78, {kHighlight}, {dt});
    for (s32 i = 0; i < static_cast<s32>(load<u16>(kHandleMap + 2)); ++i) {
        const u32 o = load<u32>(load<u32>(kHandleMap + 8) + static_cast<u32>(i) * 8 + 4);
        if ((load<u32>(o + kObjFlags160) & 1u) == 0) continue;
        ctx.fpr[1] = dt;
        vcall(ctx, o, kVUpdate, {o});
    }
    u32 n = 0;
    for (u32 k = 0; k < kKinds; ++k) {
        const u32 list = kObjLists + k * kObjListStride;
        for (s32 j = 0; j < static_cast<s32>(n = load<u16>(list + 8)); ++j) {
            const u32 o = load<u32>(load<u32>(list) + static_cast<u32>(j) * 4);
            const u32 p = load<u32>(o + kObjParent);
            if (p != 0 && (load<u32>(p + kParentFlags) & 0x8000u) != 0) continue;
            call(ctx, ifn9::Obj_Prepare3629D8, {o});
            --j;
        }
    }
    ctx.gpr[3] = n;
}
