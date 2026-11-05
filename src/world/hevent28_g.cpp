// World_HandleEvent callees, part 28g: instantiating the level's objects
// from their definitions (Objects_CreateFromDefs35AC7C).
#include "hevent28.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent28;

namespace {
namespace ofn {
constexpr u32 Array_Reserve = 0x00558F3C;          // (array, new capacity)
constexpr u32 SortedMap_Find3 = 0x0076611C;        // (map, const u32* key) -> value slot
constexpr u32 UiObj_Construct = 0x0035A868;        // (Object*, Def*)
constexpr u32 Obj_Construct35A5B0 = 0x0035A5B0;
constexpr u32 Obj_Construct35A3A0 = 0x0035A3A0;
constexpr u32 Obj_Construct35A258 = 0x0035A258;
constexpr u32 PathObj_Construct = 0x00359E08;
constexpr u32 Obj_Construct359CDC = 0x00359CDC;
constexpr u32 Obj_Construct3599C4 = 0x003599C4;
constexpr u32 ScrollerC_Construct = 0x00359284;
constexpr u32 ScrollerB_Construct = 0x00358B64;
constexpr u32 Scroller_Construct = 0x00358760;
constexpr u32 Obj_Construct357DC4 = 0x00357DC4;    // (Object* 544 bytes, Def*) (name?)
constexpr u32 Obj_Construct356BA0 = 0x00356BA0;    // (Object* 976 bytes, Def*) (name?)
}  // namespace ofn

// {key, Def*} array of object definitions, and the two {id, Object*} maps
// filled here (u16 +0 capacity, +2 count, +4 high-water mark, +6 flags,
// +8 8-byte entries; as hevent22.h map_append).
constexpr u32 kDefs = 0x0106C210, kObjects = 0x0106C228, kObjects4 = 0x0106C258;
constexpr u32 kDefFlags = 80, kDefId = 68;
// UI object variants (UiObj_Construct, then vtable / fields / +4 flag).
constexpr u32 kUiVtA = 0x00B1F988, kUiVtB = 0x00B1FA48, kUiVtC = 0x00B1F6E0;
// Globals resolved by id afterwards.
constexpr u32 kIdA = 0x00F64D28, kIdB = 0x00F64CDC;
constexpr u32 kResolved = 0x0106C330;  // +100 / +104 the objects for kIdA / kIdB (live: +4 bit 0)
constexpr u32 kFlagC3C0 = 0x0106C3C0;  // u8 cleared (name?)
constexpr u32 kWordCC8 = 0x00F64CC8;   // u32 cleared (name?)
// The original's 224-byte frame: the two lookup keys at r1 +112 / +116.
constexpr u32 kFrame = 224, kKeyBAt = 112, kKeyAAt = 116;

// The inlined {key, value} push (store order as the original: key, count,
// value, high-water mark, flags).
void push(PPUContext& ctx, u32 m, u32 key, u32 value) {
    if (load<u16>(m + 2) == load<u16>(m + 0)) call(ctx, ofn::Array_Reserve, {m, (static_cast<u32>(load<u16>(m + 0)) << 1) & 0xFFFEu});
    const u32 data = load<u32>(m + 8);
    const u32 n = load<u16>(m + 2);
    store<u32>(data + ((n << 3) & 0x7FFF8u), key);
    store<u16>(m + 2, static_cast<u16>(n + 1));
    store<u32>(data + (n << 3) + 4, value);
    const u32 hi = load<u16>(m + 4);
    const u32 fl = load<u16>(m + 6);
    store<u16>(m + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>((n + 1) & 0xFFFF) ? hi : n + 1));
    store<u16>(m + 6, static_cast<u16>(fl | 1u));
}

// Allocates size bytes (SmallAlloc, or Mem_Alloc(size, 0, 16) when big)
// and constructs the object for def; 0 when out of memory.
u32 make(PPUContext& ctx, u32 size, bool big, u32 ctor, u32 def) {
    const u32 o = static_cast<u32>(big ? call(ctx, hevent27::h27fn::Mem_Alloc, {size, 0, 16}) : call(ctx, h28fn::SmallAlloc, {size}));
    if (o != 0) call(ctx, ctor, {o, def});
    return o;
}

// The object registered as live under the id stored at key_ea, or 0.
u32 resolve(PPUContext& ctx, u32 key_ea) {
    const u32 o = load<u32>(static_cast<u32>(call(ctx, ofn::SortedMap_Find3, {kObjects, key_ea})));
    return o != 0 && (load<u32>(o + 4) & 1u) != 0 ? o : 0;
}
}  // namespace

// u32 Objects_CreateFromDefs35AC7C() (name?): for each object definition
// (0x0106C210), the object its +80 flags select is allocated and
// constructed and registered by the def's id (+68) in 0x0106C228; type 4
// objects (+80 bit 2) are also registered in 0x0106C258 (0 when not live
// or not allocated). Then the objects for the ids at 0x00F64D28 /
// 0x00F64CDC are resolved into 0x0106C330 +100 / +104. Returns r3 =
// 0x01070000 as the original leaves it.
DECOMP(0035AC7C, Objects_CreateFromDefs35AC7C) {
    Stack frame(ctx, kFrame);
    for (s32 i = 0; i < static_cast<s32>(load<u16>(kDefs + 2)); ++i) {
        const u32 def = load<u32>(load<u32>(kDefs + 8) + 8 * static_cast<u32>(i) + 4);
        if (def == 0) continue;
        u32 fl = load<u32>(def + kDefFlags);
        u32 obj = 0;
        if ((fl & 0x1u) != 0) {
            if ((fl & 0x4000u) != 0) {
                obj = static_cast<u32>(call(ctx, h28fn::SmallAlloc, {416}));
                if (obj != 0) {
                    call(ctx, ofn::UiObj_Construct, {obj, def});
                    store<u32>(obj + 0, kUiVtA);
                    store<u32>(obj + 400, 0);
                    store<u32>(obj + 404, 0);
                    store<u32>(obj + 408, 0xFFFFFFFFu);
                    store<u32>(obj + 4, load<u32>(obj + 4) | 0x4000u);
                }
            } else if ((fl & 0x20000u) != 0) {
                obj = static_cast<u32>(call(ctx, h28fn::SmallAlloc, {432}));
                if (obj != 0) {
                    call(ctx, ofn::UiObj_Construct, {obj, def});
                    store<u32>(obj + 0, kUiVtB);
                    for (u32 o = 400; o <= 416; o += 4) store<u32>(obj + o, 0);
                    const u32 w = load<u32>(obj + 4);
                    store<u8>(obj + 420, 1);
                    store<u8>(obj + 421, 1);
                    store<u32>(obj + 4, w | 0x20000u);
                }
            } else {
                obj = static_cast<u32>(call(ctx, h28fn::SmallAlloc, {400}));
                if (obj != 0) {
                    call(ctx, ofn::UiObj_Construct, {obj, def});
                    store<u32>(obj + 0, kUiVtC);
                    store<u32>(obj + 4, load<u32>(obj + 4) | 0x1u);
                }
            }
        } else if ((fl & 0x2u) != 0) {
            obj = make(ctx, 224, false, ofn::Obj_Construct35A5B0, def);
        } else if (((fl = load<u32>(def + kDefFlags)) & 0x4u) != 0) {
            obj = make(ctx, 224, false, ofn::Obj_Construct35A3A0, def);
            const u32 live = obj != 0 && (load<u32>(obj + 4) & 0x4u) != 0 ? obj : 0;
            push(ctx, kObjects4, load<u32>(def + kDefId), live);  // also when not allocated
        } else if ((fl & 0x40u) != 0) {
            obj = make(ctx, 208, false, ofn::Obj_Construct35A258, def);
        } else if (((fl = load<u32>(def + kDefFlags)) & 0x200u) != 0) {
            obj = make(ctx, 624, false, ofn::PathObj_Construct, def);
        } else if ((fl & 0x20u) != 0) {
            obj = make(ctx, 192, false, ofn::Obj_Construct359CDC, def);
        } else if (((fl = load<u32>(def + kDefFlags)) & 0x400u) != 0) {
            obj = make(ctx, 240, false, ofn::Obj_Construct3599C4, def);
        } else if ((fl & 0x800u) != 0) {
            obj = make(ctx, 432, false, ofn::ScrollerC_Construct, def);
        } else if (((fl = load<u32>(def + kDefFlags)) & 0x1000u) != 0) {
            obj = make(ctx, 480, false, ofn::ScrollerB_Construct, def);
        } else if ((fl & 0x2000u) != 0) {
            obj = make(ctx, 336, false, ofn::Scroller_Construct, def);
        } else if (((fl = load<u32>(def + kDefFlags)) & 0x8000u) != 0) {
            obj = make(ctx, 544, true, ofn::Obj_Construct357DC4, def);
        } else if ((fl & 0x10000u) != 0) {
            obj = make(ctx, 976, true, ofn::Obj_Construct356BA0, def);
        } else {
            continue;
        }
        if (obj != 0) push(ctx, kObjects, load<u32>(def + kDefId), obj);
    }

    const u32 ka = load<u32>(kIdA);
    store<u32>(frame.addr() + kKeyAAt, ka);
    store<u32>(kResolved + 100, ka != 0 ? resolve(ctx, frame.addr() + kKeyAAt) : 0);
    const u32 kb = load<u32>(kIdB);
    store<u32>(frame.addr() + kKeyBAt, kb);
    store<u32>(kResolved + 104, kb != 0 ? resolve(ctx, frame.addr() + kKeyBAt) : 0);
    store<u8>(kFlagC3C0, 0);
    store<u32>(kWordCC8, 0);
    ctx.gpr[3] = 0x01070000u;  // as the original's last lis
}
