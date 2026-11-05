// World_HandleEvent callees, part 23c: the Havok skeleton file registration
// (named by hevent22_e.cpp Level_LoadFile34AF14).
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
// Havok class lookups (2-instruction helpers: a TOC constant, then a load).
constexpr u32 kHkClassSlot7FECB8 = 0x007FECB8;  // () -> 0x011E2E80 (the root level container class slot) (name?)
constexpr u32 kHkLoadWord7D53AC = 0x007D53AC;   // (u32* slot) -> *slot (name?)
constexpr u32 kHkAddRef7D48C4 = 0x007D48C4;     // (hkResource*) (name?)
constexpr u32 kHkFindObject802B08 = 0x00802B08; // (container, const hkClass*, 0) -> object (name?)
constexpr u32 kHkRegistry = 0x011E2EE0;         // object whose vtable +16 gives the class registry (name?)
constexpr u32 kHkSkeletonClassSlot = 0x011F842C;
constexpr u32 kSlotRegistry = 16, kSlotContents = 28;
constexpr u32 kSortedMapFind76DD98 = 0x0076DD98;  // (map, const u32* key) -> value slot (symbols.txt)
constexpr u32 kHashToString = 0x0056A6E0, kStringFormat = 0x005534D0;
constexpr u32 kStringFindChar = 0x00556014;     // String_Thunk71EDAC: (s, c) -> last c in s or 0 (strrchr?)
constexpr u32 kStringLength = 0x00555FA4, kMemCompare = 0x00555DB8;
// The loaded resources (PtrVector, hevent18 layout) and the skeleton map
// ({key, skeleton} entries, hevent22::map_append layout).
constexpr u32 kResources = 0x0106BC58;
constexpr u32 kSkeletonMap = 0x0106BC40;
constexpr u32 kStrRoot = 0x00ADC02C, kStrAttachRoot = 0x00ADC034;
constexpr u32 kStrDuplicate = 0x00ADC044;  // "Duplicate Havok skeleton map in table: %s"
// Skeleton: +16 bone name table holder (+24: 8-byte entries, name pointer
// with bit 0 as a flag), +24 bones (64 bytes: +0 s16 name index, +48 a
// 16-byte scale vector), +28 s32 bone count.
constexpr u32 kSkNames = 16, kNamesTable = 24, kSkBones = 24, kSkBoneCount = 28;
constexpr u32 kBoneSize = 64, kBoneScale = 48;
constexpr char kColon = ':';
}  // namespace

// bool Level_Load34AB90(u32 key, Object* resource) (name?): registers a
// loaded Havok skeleton file: the resource (vtable +28 (res, root level
// container class, registry from 0x011E2EE0 vtable +16)) gives the
// container (0 returns 0); the resource gets 0x007D48C4 and is pushed onto
// the resources vector (0x0106BC58); the container's skeleton
// (0x00802B08) is added to the map at 0x0106BC40 under key unless already
// there ("Duplicate Havok skeleton map in table: %s" is reported then and
// nothing else is done). Every bone whose name (after the last ':' when
// one is followed by more) starts with "Root" or is "AttachRoot" gets its
// scale (+48) set to {1, 1, 1, 0}. Returns 1.
DECOMP(0034AB90, Level_Load34AB90) {
    const u32 key = arg<u32>(ctx, 1), res = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);
    const u32 slot = static_cast<u32>(call(ctx, kHkClassSlot7FECB8, {}));
    const u32 cls = static_cast<u32>(call(ctx, kHkLoadWord7D53AC, {slot}));
    const u32 reg_obj = load<u32>(kHkRegistry);
    const u32 reg = static_cast<u32>(vcall(ctx, reg_obj, kSlotRegistry, {reg_obj}));
    const u32 container = static_cast<u32>(vcall(ctx, res, kSlotContents, {res, cls, reg}));
    if (container == 0) return ret(ctx, 0u);
    call(ctx, kHkAddRef7D48C4, {res});
    sly4::hevent18::pv_push(ctx, kResources, res);
    const u32 skcls = static_cast<u32>(call(ctx, kHkLoadWord7D53AC, {kHkSkeletonClassSlot}));
    const u32 sk = static_cast<u32>(call(ctx, kHkFindObject802B08, {container, skcls, 0}));
    const u32 found = static_cast<u32>(call(ctx, kSortedMapFind76DD98, {kSkeletonMap, frame.addr()}));
    if (load<u32>(found) != 0) {
        const u32 s = static_cast<u32>(call(ctx, kHashToString, {load<u32>(frame.addr())}));
        call(ctx, kStringFormat, {kStrDuplicate, s});
        return ret(ctx, 1u);
    }
    sly4::hevent22::map_append(ctx, kSkeletonMap, load<u32>(frame.addr()), sk);
    for (u32 i = 0, off = 0; static_cast<s32>(i) < static_cast<s32>(load<u32>(sk + kSkBoneCount)); ++i, off += kBoneSize) {
        const s32 ni = static_cast<s16>(load<u16>(load<u32>(sk + kSkBones) + off));
        const u32 name = load<u32>(load<u32>(load<u32>(sk + kSkNames) + kNamesTable) + static_cast<u32>(ni) * 8) & ~1u;
        u32 s = static_cast<u32>(call(ctx, kStringFindChar, {name, static_cast<u32>(kColon)}));
        if (s != 0 && static_cast<s32>(call(ctx, kStringLength, {s})) > 1) s = s + 1;
        else s = name;
        if (static_cast<s32>(call(ctx, kMemCompare, {s, kStrRoot, 4})) != 0 &&
            static_cast<s32>(call(ctx, kMemCompare, {s, kStrAttachRoot, 10})) != 0)
            continue;
        // stvx of the {1.0f, 1.0f, 1.0f, 0} vector built from 0x0034AB50.
        const u32 v = (load<u32>(sk + kSkBones) + off + kBoneScale) & ~15u;
        store<u32>(v + 0, 0x3F800000u);
        store<u32>(v + 4, 0x3F800000u);
        store<u32>(v + 8, 0x3F800000u);
        store<u32>(v + 12, 0);
    }
    ret(ctx, 1u);
}
