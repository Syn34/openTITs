// World_HandleEvent callees, part 15d: Effect_AddArg49F080, one event
// argument record bound to the effect parameter of the same name.
#include "hevent15.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent15;

namespace {
namespace dfn {
constexpr u32 Effect_FindComponent = 0x004BB4B4;   // (Effect*, u32 hash) -> component or 0 (symbols.txt)
constexpr u32 Hash_ToString = 0x0056A6E0;          // (u32) -> "0x%x" (symbols.txt)
constexpr u32 SortedMap_Find734750 = 0x00734750;   // (map, const u32* key) -> slot (symbols.txt)
constexpr u32 SortedMap_Find73B608 = 0x0073B608;   // (map, const u32* key) -> slot (symbols.txt)
constexpr u32 SortedMap_Find733F8C = 0x00733F8C;   // (map, const u32* key) -> slot (symbols.txt)
constexpr u32 SortedMap_Find3 = 0x0076611C;        // (map, const u32* key) -> slot (symbols.txt)
constexpr u32 Item_Hash39FEBC = 0x0039FEBC;        // (368-byte record) -> u32 hash (name?)
constexpr u32 Obj_FindByHandle42C2B0 = 0x0042C2B0; // (u32 handle, u8) -> Object* (name?)
constexpr u32 Obj_FindById = 0x000D6A28;       // (u32 id) -> Object* (hevent15_a.cpp)
constexpr u32 Param_SetMatrix0A78D4 = 0x000A78D4;  // (Param*, const void*) (name?)
}  // namespace dfn

// Argument record (hevent8.h kArgSize): +0 value, +4 alternative id, +68
// name hash, +72 u8 type.
constexpr u32 kArgId = 4;
// Effect parameter: vt +32 kind(), vt +24 get int, vt +20 get f32 (f1), vt
// +8 release; +20 value, +28 counted reference (+4 u16 count), +32 vec3 /
// 4x3 matrix, +36 reference extra word.
constexpr u32 kVKind = 32, kVGetInt = 24, kVGetFloat = 20, kVRelease = 8;
constexpr u32 kParValue = 20, kParRef = 28, kParRefExtra = 36, kParVec = 32, kRefCount = 4;

// Tables: (*0x00F657FC) items: +4 header (+2 u16 count), +12 112-byte
// records (+76 map hash -> s16 index), +28 336-byte records; +36 header (+2
// u16 count), +44 368-byte records; +68 map hash -> s16 index, +12 112-byte
// records. (*0x00F65BE8): +4 / +28 maps hash -> object.
constexpr u32 kItems = 0x00F657FC, kLevelInfo = 0x00F65BE8;

// Object +192 bit 22; +160 / +164 the reference and its extra word; +168
// name hash.
constexpr u32 kObjFlag192Bit22 = 0x00400000, kObjRef = 160, kObjRefExtra = 164, kObjName = 168;
}  // namespace

// u32 Effect_AddArg49F080(Effect* fx, Arg* a, Effect* src, u8 by_id, u8 quiet)
// (name?): finds fx's parameter named by a +68 and, when its kind (vt +32)
// fits a's type (+72), sets it from a's value: 1 copies another effect's
// (src) parameter of the same kind, 2 / 3 / 9 take objects (or object
// references), 4 .. 8 look the hash up in the item / level tables, 10 .. 16
// copy ints, floats, vectors or matrices. A failed lookup reports the hash
// (Hash_ToString) unless quiet. r3 as the original leaves it.
DECOMP(0049F080, Effect_AddArg49F080) {
    const u32 fx = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), src = arg<u32>(ctx, 2);
    const u8 by_id = arg<u8>(ctx, 3), quiet = arg<u8>(ctx, 4);
    u64 r3 = fx;
    auto done = [&]() { ctx.gpr[3] = r3; };
    if (fx == 0) return done();
    r3 = load<u8>(a + hevent8::kArgType);
    if (r3 == 0) return done();
    const u32 p = static_cast<u32>(r3 = call(ctx, dfn::Effect_FindComponent, {fx, load<u32>(a + hevent8::kArgHash)}));
    if (p == 0) return done();
    const u32 type = load<u8>(a + hevent8::kArgType);
    r3 = type;
    auto kind = [&](u32 x) { return static_cast<u32>(r3 = vcall(ctx, x, kVKind, {x})); };
    auto report = [&](u32 h) { r3 = call(ctx, dfn::Hash_ToString, {h}); };
    // The object's name hash, else the record's id.
    auto obj_name = [&]() {
        const u32 o = load<u32>(a + hevent8::kArgValue);
        return o != 0 ? load<u32>(o + kObjName) : load<u32>(a + kArgId);
    };
    // p +28 = ref (counted), p +36 = extra.
    auto set_ref = [&](u32 ref, u32 extra_ea) {
        const u32 old = load<u32>(p + kParRef);
        r3 = old;
        if (old != 0) r3 = vcall(ctx, old, kVRelease, {old});
        store<u32>(p + kParRef, ref);
        const u32 extra = load<u32>(extra_ea);
        if (ref != 0) {
            store<u16>(ref + kRefCount, static_cast<u16>(load<u16>(ref + kRefCount) + 1));
            r3 = ref + kRefCount;
        }
        store<u32>(p + kParRefExtra, extra);
    };
    // Kind 2 / 3 objects: the record's object, else looked up by id.
    auto find_obj = [&](u32 hint) {
        u32 o = load<u32>(a + hevent8::kArgValue);
        if (o == 0) {
            if (by_id == 0) o = static_cast<u32>(r3 = call(ctx, dfn::Obj_FindById, {load<u32>(a + kArgId)}));
            else o = static_cast<u32>(r3 = call(ctx, dfn::Obj_FindByHandle42C2B0, {load<u32>(a + kArgId), hint}));
        }
        return o;
    };
    switch (type) {
    case 1: {  // the same-named parameter of src, of the same kind
        if (src == 0) return done();
        const u32 q = static_cast<u32>(r3 = call(ctx, dfn::Effect_FindComponent, {src, load<u32>(a + hevent8::kArgValue)}));
        if (q == 0) return done();
        if (kind(q) == 5 && kind(p) == 5) {
            const u32 ref = load<u32>(q + kParRef);
            const u32 old = load<u32>(p + kParRef);
            r3 = old;
            if (old != 0) r3 = vcall(ctx, old, kVRelease, {old});
            store<u32>(p + kParRef, ref);
            const u32 extra = load<u32>(q + kParRefExtra);
            if (ref != 0) {
                store<u16>(ref + kRefCount, static_cast<u16>(load<u16>(ref + kRefCount) + 1));
                r3 = ref + kRefCount;
            }
            store<u32>(p + kParRefExtra, extra);
            return done();
        }
        if (kind(q) == 2 && kind(p) == 2) {
            r3 = vcall(ctx, q, kVGetInt, {q});
            store<u32>(p + kParValue, static_cast<u32>(r3));
            return done();
        }
        if (kind(q) == 1 && kind(p) == 1) {
            r3 = vcall(ctx, q, kVGetFloat, {q});
            store<f32>(p + kParValue, static_cast<f32>(ctx.fpr[1]));
            return done();
        }
        if (kind(q) == 16 && kind(p) == 16) r3 = call(ctx, dfn::Param_SetMatrix0A78D4, {p, q + kParValue});
        return done();
    }
    case 2: {  // an object's reference (+160 / +164)
        const u32 o = find_obj(quiet);
        if (o != 0 && kind(p) == 5) {
            set_ref(load<u32>(o + kObjRef), o + kObjRefExtra);
            r3 = load<u32>(o + kObjRefExtra);
            return done();
        }
        if (quiet != 0) return done();
        if (o == 0) report(obj_name());
        if (kind(p) == 5) return done();
        report(obj_name());
        return done();
    }
    case 3: {  // an object with +192 bit 22
        const u32 o = find_obj(0);
        if (o != 0 && kind(p) == 15) {
            r3 = load<u32>(o + hevent14::kObjFlags192) & kObjFlag192Bit22;
            if (r3 != 0) {
                store<u32>(p + kParValue, o);
                return done();
            }
        }
        if (quiet != 0) return done();
        report(obj_name());
        return done();
    }
    case 4:
    case 5: {  // level object by hash
        Stack frame(ctx, 32);  // r1+124 (5) / r1+128 (4): the key
        const bool five = type == 5;
        const u32 key = frame.addr() + (five ? 12 : 16);
        store<u32>(key, load<u32>(a + hevent8::kArgValue));
        const u32 map = load<u32>(kLevelInfo) + (five ? 28 : 4);
        const u32 slot = static_cast<u32>(r3 = call(ctx, five ? dfn::SortedMap_Find73B608 : dfn::SortedMap_Find733F8C, {map, key}));
        const u32 o = load<u32>(slot);
        if (o != 0 && kind(p) == (five ? 18u : 17u)) {
            store<u32>(p + kParValue, o);
            return done();
        }
        if (quiet == 0) report(load<u32>(a + hevent8::kArgValue));
        return done();
    }
    case 6: {  // 336-byte item record by a hash in one of the 112-byte records' maps
        const u32 g = load<u32>(kItems);
        const u32 v = load<u32>(a + hevent8::kArgValue);
        if (v != 0) {
            r3 = load<u16>(load<u32>(g + 4) + 2);
            for (u32 k = 0, off = 0; k < static_cast<u32>(r3); ++k, off += 112) {
                const u32 rec = load<u32>(g + 12) + off;
                if (rec != 0) {
                    Stack frame(ctx, 16);  // r1+120: the key
                    store<u32>(frame.addr() + 8, v);
                    const u32 slot = static_cast<u32>(call(ctx, dfn::SortedMap_Find734750, {rec + 76, frame.addr() + 8}));
                    const s32 idx = static_cast<s16>(load<u16>(slot));
                    r3 = static_cast<u64>(static_cast<s64>(idx));
                    if (idx != -1) {
                        const u32 item = load<u32>(g + 28) + static_cast<u32>(idx) * 336;
                        r3 = static_cast<u32>(idx) * 336;
                        if (item != 0 && kind(p) == 19) {
                            store<u32>(p + kParValue, item);
                            return done();
                        }
                        break;
                    }
                }
                r3 = load<u16>(load<u32>(g + 4) + 2);
            }
        }
        if (quiet == 0) report(load<u32>(a + hevent8::kArgValue));
        return done();
    }
    case 7: {  // 368-byte record whose hash (0x0039FEBC) is the value
        const u32 g = load<u32>(kItems);
        const u32 v = load<u32>(a + hevent8::kArgValue);
        if (v != 0) {
            r3 = load<u16>(load<u32>(g + 36) + 2);
            for (u32 k = 0, off = 0; k < static_cast<u32>(r3); ++k, off += 368) {
                const u32 rec = load<u32>(g + 44) + off;
                if (rec != 0) {
                    r3 = call(ctx, dfn::Item_Hash39FEBC, {rec});
                    if (static_cast<u32>(r3) == v) {
                        if (kind(p) == 20) {
                            store<u32>(p + kParValue, rec);
                            return done();
                        }
                        break;
                    }
                }
                r3 = load<u16>(load<u32>(g + 36) + 2);
            }
        }
        if (quiet == 0) report(load<u32>(a + hevent8::kArgValue));
        return done();
    }
    case 8: {  // 112-byte record by hash in the items' +68 map
        const u32 g = load<u32>(kItems);
        const u32 v = load<u32>(a + hevent8::kArgValue);
        Stack frame(ctx, 16);  // r1+116: the key
        store<u32>(frame.addr() + 4, v);
        r3 = v;
        if (v != 0) {
            const u32 slot = static_cast<u32>(call(ctx, dfn::SortedMap_Find734750, {g + 68, frame.addr() + 4}));
            const s32 idx = static_cast<s16>(load<u16>(slot));
            r3 = static_cast<u64>(static_cast<s64>(idx));
            if (idx != -1) {
                const u32 rec = load<u32>(g + 12) + static_cast<u32>(idx) * 112;
                r3 = static_cast<u32>(idx) * 112;
                if (rec != 0 && kind(p) == 21) {
                    store<u32>(p + kParValue, rec);
                    return done();
                }
            }
        }
        if (quiet == 0) report(load<u32>(a + hevent8::kArgValue));
        return done();
    }
    case 9: {  // an object by handle: its +24 reference, +28 extra
        const u32 h = load<u32>(a + hevent8::kArgValue);
        if (h == 0) return done();
        Stack frame(ctx, 16);  // r1+112: the key
        store<u32>(frame.addr(), h);
        const u32 o = load<u32>(static_cast<u32>(r3 = call(ctx, dfn::SortedMap_Find3, {kHandleMap, frame.addr()})));
        if (o == 0 || kind(p) != 5) return done();
        set_ref(load<u32>(o + 24), o + 28);
        r3 = load<u32>(o + 28);
        return done();
    }
    case 10:
    case 13:
        if (kind(p) == 2) store<u32>(p + kParValue, static_cast<u32>(r3 = load<u32>(a + hevent8::kArgValue)));
        return done();
    case 11:
        if (kind(p) == 16) r3 = call(ctx, dfn::Param_SetMatrix0A78D4, {p, a});
        return done();
    case 12:
        if (kind(p) == 1) {
            store<u32>(p + kParValue, load<u32>(a));
            ctx.fpr[1] = lfs_f64(a);
        }
        return done();
    case 14:
        if (kind(p) == 2) store<u32>(p + kParValue, static_cast<u32>(r3 = load<u8>(a + hevent8::kArgValue)));
        return done();
    case 15:
        if (kind(p) == 3) {
            for (u32 k = 0; k < 12; k += 4) store<u32>(p + kParVec + k, load<u32>(a + k));
            for (u32 k = 0; k < 3; ++k) ctx.fpr[1 + k] = lfs_f64(a + 4 * k);
        }
        return done();
    case 16:
        if (kind(p) == 4) {
            u32 w[12];
            u32 n = 0;
            for (u32 row = 0; row < 4; ++row)
                for (u32 k = 0; k < 12; k += 4) w[n++] = load<u32>(a + 16 * row + k);
            n = 0;
            for (u32 row = 0; row < 4; ++row)
                for (u32 k = 0; k < 12; k += 4) store<u32>(p + kParVec + 16 * row + k, w[n++]);
            n = 0;
            for (u32 row = 0; row < 4; ++row)
                for (u32 k = 0; k < 12; k += 4) ctx.fpr[1 + n++] = lfs_f64(a + 16 * row + k);
        }
        return done();
    default:
        return done();
    }
}
