// World_HandleEvent callees, part 25b: callees named by hevent24 (speaker
// text widgets, the game completion percentage, a clip's array release).
#include <bit>
#include <cmath>

#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kEpisodePercent = 0x002DB040;    // (Episode*, Save*) -> f32 (name?)
constexpr u32 kArrayDestruct = 0x0054EC94;     // (Array*) (symbols.txt Array_Destruct)
constexpr u32 kMemFreeFrom = 0x0054EE6C;       // (block, pool)
// Constants in the code segment before 0x002E3188: 0.0, 100.0, 0.01.
constexpr u32 kPercentConsts = 0x002E317C;
}  // namespace

// void Spk_SetText384EE4(SpeakerObj* o, const char* text, u32 field) (name?):
// the def's (+8) widget entries (+152 count, 36 bytes each from +148) are
// searched for +32 == field; that entry's +16 widget gets the text (vt +188
// (w, text, 0)) and is refreshed (vt +148). Nothing happens for a null text
// or field. r3 is the text unless both calls were made.
DECOMP(00384EE4, Spk_SetText384EE4) {
    const u32 o = arg<u32>(ctx, 0), text = arg<u32>(ctx, 1), field = arg<u32>(ctx, 2);
    const u32 d = load<u32>(o + 8);
    ret(ctx, text);
    if (text == 0 || field == 0) return;
    const u32 n = load<u32>(d + 152);
    if (n == 0) return;
    const u32 tab = load<u32>(d + 148);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(tab + 36 * i + 32) != field) continue;
        const u32 w = load<u32>(tab + 36 * i + 16);
        if (w == 0) return;
        vcall(ctx, w, 188, {w, text, 0});
        const u32 w2 = load<u32>(load<u32>(d + 148) + 36 * i + 16);
        vcall(ctx, w2, 148, {w2});
        return;
    }
}

// f32 Game_CompletePercent2E3188(Game* g, Save* save) (name?): the weighted
// mean of the episodes' completion (0x002DB040 per 860-byte episode at
// g +20, count g +24, weight f32 at +832) as a percentage: sum(pct * w *
// 0.01) / sum(w) * 100, or 0 when the weights sum to 0 or less. save
// defaults to the current user's save slot of the current world.
DECOMP(002E3188, Game_CompletePercent2E3188) {
    const u32 g = arg<u32>(ctx, 0);
    u32 save = arg<u32>(ctx, 1);
    const u32 w = load<u32>(kCurrentWorld);
    if (save == 0) save = w + load<u8>(w + kUserBlocks) * kSaveSlotSize + kSaveData;
    const f32 zero = load<f32>(kPercentConsts);
    f32 total = zero, acc = zero;
    if (load<u32>(g + 24) != 0) {
        const f32 scale = load<f32>(kPercentConsts + 8);
        for (u32 i = 0; i < load<u32>(g + 24); ++i) {
            const u32 ep = load<u32>(g + 20) + 860 * i;
            total = static_cast<f32>(static_cast<f64>(total) + load<f32>(ep + 832));
            call(ctx, kEpisodePercent, {ep, save});  // (f1 holds the weight; not read)
            const f64 pct = ctx.fpr[1];
            const f32 p = fmuls(pct, load<f32>(load<u32>(g + 20) + 860 * i + 832));
            acc = static_cast<f32>(std::fma(static_cast<f64>(p), static_cast<f64>(scale), static_cast<f64>(acc)));
        }
    }
    // r3 as the original leaves it: the episode table, or the constants.
    ctx.gpr[3] = load<u32>(g + 20);
    if (!(total > zero)) return ret(ctx, zero);
    ctx.gpr[3] = kPercentConsts;
    const f32 q = static_cast<f32>(static_cast<f64>(acc) / static_cast<f64>(total));
    ret(ctx, fmuls(q, load<f32>(kPercentConsts + 4)));
}

namespace {
// Frees a clip array at slot (u16 +0 count, +6 stride * 2, +8 data, +12
// allocator): data back to the allocator (vt +8), then the array itself.
bool free_array(PPUContext& ctx, u32 slot) {
    const u32 a = load<u32>(slot);
    if (a == 0) return false;
    const u32 size = static_cast<u32>(load<u16>(a + 0)) * (static_cast<u32>(load<u16>(a + 6)) >> 1);
    if (const u32 data = load<u32>(a + 8); data != 0) {
        const u32 alloc = load<u32>(a + 12);
        vcall(ctx, alloc, 8, {alloc, data, size});
    }
    call(ctx, kArrayDestruct, {a});
    call(ctx, kMemFreeFrom, {a, 0});
    store<u32>(slot, 0);
    return true;
}
}  // namespace

// u32 Clip_Release4B0C70(Clip* c) (name?): frees the arrays at +8, +12 and
// +16, then the +36 block {+0 data, +4 owner (+4 size, +12 allocator)}
// (its data first, through the owner's allocator vt +8). r3 is 0, or c
// when nothing was held.
DECOMP(004B0C70, Clip_Release4B0C70) {
    const u32 c = arg<u32>(ctx, 0);
    bool any = false;
    for (u32 off : {8u, 12u, 16u}) any |= free_array(ctx, c + off);
    if (const u32 b = load<u32>(c + 36); b != 0) {
        if (const u32 data = load<u32>(b + 0); data != 0) {
            const u32 owner = load<u32>(b + 4);
            const u32 size = load<u32>(owner + 4), alloc = load<u32>(owner + 12);
            vcall(ctx, alloc, 8, {alloc, data, size});
        }
        call(ctx, kMemFreeFrom, {b, 0});
        store<u32>(c + 36, 0);
        any = true;
    }
    ret(ctx, any ? 0u : c);
}

// ---------------------------------------------------------------- 0x0031E1D8
namespace {
constexpr u32 kPointMapFind = 0x0073343C;      // (map, const u32* key) -> value slot (symbols.txt PointMap_Find)
constexpr u32 kPointDefs = 0x00F8F154;         // u32 -> object whose +16 is the named point map
constexpr u32 kHashToString = 0x0056A6E0;      // (u32) -> "0x%x" (debug name of a missing hash)
constexpr u32 kDirToMatrix = 0x004A54F8;       // (const vec3* dir, Matrix* m) (name?)
constexpr u32 kMatrixNormalize = 0x0056D374;   // (Matrix*) (symbols.txt Matrix_NormalizeInPlace)
constexpr u32 kQuatToMatrix = 0x00553A90;      // (Matrix* out, const quat*, const vec4* pos) (symbols.txt Quat_ToMatrix)
constexpr u32 kMatrixMultiply = 0x005541F0;    // (Matrix* a, const Matrix* b) (symbols.txt)
constexpr u32 kGeomFind = 0x005801A0;          // (u32 hash) -> GeomInstance* (symbols.txt GeomInstance_FindByHash)
constexpr u32 kSmallAlloc = 0x0055238C;
constexpr u32 kPtrVectorConstruct = 0x005516E0, kPtrVectorInit = 0x00551598, kPtrVectorReserve = 0x00551718;
constexpr u32 kSetName = 0x0057D368;           // (WorldObj*, const char*, bool) (symbols.txt WorldObj_SetName)
constexpr u32 kMarkDirty = 0x0057D000;         // (WorldObj*) (symbols.txt WorldObj_MarkDirty)
constexpr u32 kZeroVec4 = 0x01164B40;          // 16 zero bytes (also the Quat_ToMatrix position)
constexpr u32 kDirConsts = 0x0031E1D0;         // f32 1e-5, 1.0 (code segment)
constexpr u32 kScrollGroupName = 0x00ADB5B8;   // "DummyScrollGroupName"

// Point objects carry their position at +128.
constexpr u32 kPointPos = 128;

// The named point (by hash at def +off), or 0 (its hash name formatted for
// the debug log when missing).
u32 find_point(PPUContext& ctx, u32 a, u32 off, u32 key_slot) {
    const u32 key = load<u32>(load<u32>(a + 132) + off);
    if (key != 0) {
        store<u32>(key_slot, key);
        const u32 p = load<u32>(static_cast<u32>(call(ctx, kPointMapFind, {load<u32>(load<u32>(kPointDefs) + 16), key_slot})));
        if (p != 0) return p;
    }
    if (const u32 k = load<u32>(load<u32>(a + 132) + off); k != 0) call(ctx, kHashToString, {k});
    return 0;
}

// The engine's inlined PtrVector push (u16 +4 capacity, +8 count, +10
// high-water mark); grow_mask is the doubling's mask as compiled.
void vec_push(PPUContext& ctx, u32 v, u32 value, u32 grow_mask, u32 index_mask) {
    if (load<u16>(v + 8) == load<u16>(v + 4)) call(ctx, kPtrVectorReserve, {v, (static_cast<u32>(load<u16>(v + 4)) << 1) & grow_mask});
    store<u32>(load<u32>(v) + ((static_cast<u32>(load<u16>(v + 8)) << 2) & index_mask), value);
    const u32 n = static_cast<u32>(load<u16>(v + 8)) + 1;
    const u32 hi = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n));
}

// Copies the 3x4 part (rows 16 bytes apart) of a matrix as raw words.
void copy_mat34(u32 dst, u32 src) {
    for (u32 row = 0; row < 4; ++row)
        for (u32 c = 0; c < 3; ++c) store<u32>(dst + 16 * row + 4 * c, load<u32>(src + 16 * row + 4 * c));
}
}  // namespace

// u32 Attach_Init31E1D8(Attach* a) (name?): with a +136 set, aims the
// attachment from the def's (+132) +36 point to its +40 point (both looked
// up in the named point map): +112 the unit direction (+128 its length;
// normalised only above 1e-5), and unless it is exactly zero, +48 a matrix
// along it (0x004A54F8, normalised) placed at the first point. The world
// matrix (+48 times the def's +16 rotation) is then given to one dummy
// scroll-group object per (def +52) slot of every def geometry instance
// (def +0 hash list, vt +40 (0, 1) on each): slot j's child holder (44
// bytes: PtrVectors +0 step 4, +20 step 32, +40 = a) comes from a's vector
// or is created and pushed (also into a's ring buffer +20..+40), and the
// object from geometry vt +56 (5) is named, shown (vt +40 (0, 1)),
// placed, marked dirty and pushed into the holder. Returns the hash list
// count (0 without +136).
DECOMP(0031E1D8, Attach_Init31E1D8) {
    const u32 a = arg<u32>(ctx, 0);
    if (load<u32>(a + 136) == 0) return ret(ctx, 0u);
    Stack frame(ctx, 144);
    const u32 rot = frame.addr(), world = frame.addr() + 64;  // the original's r1 +112 / +176
    const u32 keyB = frame.addr() + 128, keyA = frame.addr() + 132;  // r1 +240 / +244
    const u32 pa = find_point(ctx, a, 36, keyA);
    const u32 pb = find_point(ctx, a, 40, keyB);
    if (pa != 0 && pb != 0) {
        const u32 ax = load<u32>(pa + kPointPos), ay = load<u32>(pa + kPointPos + 4), az = load<u32>(pa + kPointPos + 8);
        const auto f = [](u32 bits) { return static_cast<f64>(std::bit_cast<f32>(bits)); };
        f32 dy = static_cast<f32>(static_cast<f64>(load<f32>(pb + kPointPos + 4)) - f(ay));
        f32 dx = static_cast<f32>(static_cast<f64>(load<f32>(pb + kPointPos)) - f(ax));
        f32 dz = static_cast<f32>(static_cast<f64>(load<f32>(pb + kPointPos + 8)) - f(az));
        f32 len2 = fmuls(dy, dy);
        store<f32>(a + 116, dy);
        store<f32>(a + 112, dx);
        store<f32>(a + 120, dz);
        len2 = static_cast<f32>(std::fma(static_cast<f64>(dx), static_cast<f64>(dx), static_cast<f64>(len2)));
        len2 = static_cast<f32>(std::fma(static_cast<f64>(dz), static_cast<f64>(dz), static_cast<f64>(len2)));
        store<f32>(a + 128, len2);
        if (len2 > load<f32>(kDirConsts)) {
            const f32 len = static_cast<f32>(std::sqrt(static_cast<f64>(len2)));
            store<f32>(a + 128, len);
            const f32 inv = static_cast<f32>(static_cast<f64>(load<f32>(kDirConsts + 4)) / static_cast<f64>(len));
            dx = fmuls(dx, inv);
            dy = fmuls(dy, inv);
            dz = fmuls(dz, inv);
            store<f32>(a + 112, dx);
            store<f32>(a + 116, dy);
            store<f32>(a + 120, dz);
        }
        const bool zero = load<u64>(a + 112) == load<u64>(kZeroVec4) && load<u32>(a + 120) == load<u32>(kZeroVec4 + 8);
        if (!zero) {
            call(ctx, kDirToMatrix, {a + 112, a + 48});
            call(ctx, kMatrixNormalize, {a + 48});
            store<u32>(a + 96, ax);
            store<u32>(a + 100, ay);
            store<u32>(a + 104, az);
        } else {
            call(ctx, kHashToString, {load<u32>(load<u32>(a + 132) + 36)});
            call(ctx, kHashToString, {load<u32>(load<u32>(a + 132) + 40)});
        }
    }
    call(ctx, kQuatToMatrix, {rot, load<u32>(a + 132) + 16, kZeroVec4});
    copy_mat34(world, a + 48);
    call(ctx, kMatrixMultiply, {world, rot});
    call(ctx, kMatrixNormalize, {world});
    for (u32 i = 0;; ++i) {
        const u32 list = load<u32>(load<u32>(a + 132));
        const u32 n = list != 0 ? static_cast<u32>(load<u16>(list + 8)) : 0u;
        if (!(i < n)) return ret(ctx, n);
        const u32 hash = load<u32>(load<u32>(list) + 4 * i);
        if (hash == 0) continue;
        const u32 geom = static_cast<u32>(call(ctx, kGeomFind, {hash}));
        if (geom == 0) {
            call(ctx, kHashToString, {hash});
            continue;
        }
        vcall(ctx, geom, 40, {geom, 0, 1});
        if (load<u32>(load<u32>(a + 132) + 52) == 0) continue;
        for (u32 j = 0;;) {
            u32 holder;
            if (j < load<u16>(a + 8)) {
                holder = load<u32>(load<u32>(a) + 4 * j);
            } else {
                holder = 0;
                const u32 s = static_cast<u32>(call(ctx, kSmallAlloc, {44}));
                if (s != 0) {
                    const u32 l2 = load<u32>(load<u32>(a + 132));
                    const u32 cnt = l2 != 0 ? static_cast<u32>(load<u16>(l2 + 8)) : 0u;
                    call(ctx, kPtrVectorConstruct, {s});
                    store<u16>(s + 6, 4);
                    call(ctx, kPtrVectorInit, {s, cnt, 0, 1});
                    call(ctx, kPtrVectorConstruct, {s + 20});
                    store<u16>(s + 26, 32);
                    call(ctx, kPtrVectorInit, {s + 20, cnt, 0, 1});
                    store<u32>(s + 40, 0);
                    holder = s;
                }
                vec_push(ctx, a, holder, 0x1FFFEu, 0x3FFFCu);
                store<u32>(holder + 40, a);  // quirk: holder 0 writes guest address 40
                // a's ring buffer of holders: +20 begin, +24 end, +28 write
                // slot, +36 capacity, +40 count.
                if (const u32 count = load<u32>(a + 40); count != load<u32>(a + 36)) {
                    const u32 wp = load<u32>(a + 28);
                    const u32 next = wp + 4;
                    const u32 end = load<u32>(a + 24), begin = load<u32>(a + 20);
                    store<u32>(a + 40, count + 1);
                    store<u32>(a + 28, static_cast<s32>(next - end) < 0 ? next : begin);
                    if (wp != 0) store<u32>(wp, holder);
                }
            }
            const u32 obj = static_cast<u32>(vcall(ctx, geom, 56, {geom, 5}));
            if (obj != 0) {
                call(ctx, kSetName, {obj, kScrollGroupName, 1});
                vcall(ctx, obj, 40, {obj, 0, 1});
                copy_mat34(obj + 16, world);
                call(ctx, kMarkDirty, {obj});
                vec_push(ctx, holder, obj, 0xFFFFFFFEu, 0xFFFFFFFCu);
            }
            ++j;
            if (!(j < load<u32>(load<u32>(a + 132) + 52))) break;
        }
    }
}

// ---------------------------------------------------------------- 0x0031FB90
namespace {
constexpr u32 kMapFindRec = 0x00768AC0;        // (map, const u32* key) -> value slot (symbols.txt SortedMap_Find768AC0)
constexpr u32 kKeysFind = 0x00768C48;          // (map, const u32* key) -> value slot (symbols.txt Keys_Find768C48)
constexpr u32 kArrayReserve = 0x00558F3C;      // (array, new capacity)
constexpr u32 kAttachBuild = 0x0031F44C;       // (PtrVector* out, Model*) (symbols.txt Attach_Build31F44C)
constexpr u32 kModelReset = 0x0031F968;        // (Model*, 0, 0) (symbols.txt Model_Reset31F968)

// The first half of the engine's inlined {key, value} map push (u16 +0
// capacity, +2 count, +4 high-water mark, +6 flags, +8 data): the key and
// count are stored, the new count and the old high-water mark are returned
// and the caller stores the value and the two halves in its own order.
struct MapPush {
    u32 n1, hi;
};
MapPush map_push_key(PPUContext& ctx, u32 m, u32 key_ea, u32 value) {
    u32 n = load<u16>(m + 2);
    if (n == load<u16>(m + 0)) {
        call(ctx, kArrayReserve, {m, (static_cast<u32>(load<u16>(m + 0)) << 1) & 0xFFFEu});
        n = load<u16>(m + 2);
    }
    const u32 data = load<u32>(m + 8);
    const u32 n2 = load<u16>(m + 2);
    store<u32>(data + ((n << 3) & 0x7FFF8u), load<u32>(key_ea));
    const u32 hi = load<u16>(m + 4);
    store<u16>(m + 2, static_cast<u16>(n2 + 1));
    store<u32>(data + (n2 << 3) + 4, value);
    return {n2 + 1, hi};
}
u16 high_water(const MapPush& p) { return static_cast<u16>(static_cast<s32>(p.hi) > static_cast<s32>(p.n1 & 0xFFFF) ? p.hi : p.n1); }
}  // namespace

// u32 Model_Build31FB90(Model* m) (name?): from the def (+48), each +0
// list record (16 bytes {key, -, u32 n, u8 flag}) not yet in the map at +0
// gets a 32-byte slot table (SmallAlloc: +0 / +8 / +12 a buffer of n
// words, +4 its end, +16 n, +20 0, +24 the record, +28 bit 31 the record's
// flag) keyed by the record key; each +4 list record (8 bytes {PtrVector*
// models, key}) not yet in the map at +24 gets a 24-byte attachment vector
// (PtrVector step 4 sized to the models, +20 the record) filled by
// Attach_Build31F44C. Duplicates have their key formatted for the debug
// log. Ends with Model_Reset31F968(m, 0, 0) and returns its result. A
// failed slot allocation is still added (as 0) and its +28 updated.
DECOMP(0031FB90, Model_Build31FB90) {
    const u32 m = arg<u32>(ctx, 0);
    for (u32 i = 0;; ++i) {
        const u32 list = load<u32>(load<u32>(m + 48));
        if (!(i < (list != 0 ? static_cast<u32>(load<u16>(list + 8)) : 0u))) break;
        const u32 rec = load<u32>(list) + 16 * i;
        if (load<u32>(static_cast<u32>(call(ctx, kMapFindRec, {m, rec}))) != 0) {
            call(ctx, kHashToString, {load<u32>(rec)});
            continue;
        }
        u32 item = 0;
        if (const u32 s = static_cast<u32>(call(ctx, kSmallAlloc, {32})); s != 0) {
            const u32 n = load<u32>(rec + 8);
            const u32 bytes = n << 2;
            const u32 buf = static_cast<u32>(call(ctx, kSmallAlloc, {bytes}));
            const u32 w = load<u32>(s + 28) & 0x7FFFFFFFu;
            store<u32>(s + 0, buf);
            store<u32>(s + 8, buf);
            const u32 flag = load<u8>(rec + 12);
            store<u32>(s + 12, buf);
            store<u32>(s + 4, buf + bytes);
            store<u32>(s + 16, n);
            store<u32>(s + 20, 0);
            store<u32>(s + 24, rec);
            store<u32>(s + 28, w | ((flag & 1) << 31));
            item = s;
        }
        const MapPush p = map_push_key(ctx, m, rec, item);
        store<u16>(m + 6, static_cast<u16>(load<u16>(m + 6) | 1));
        store<u16>(m + 4, high_water(p));
        // Recomputed from the record (also through a null item).
        const u32 r = load<u32>(item + 24);
        const u32 w = load<u32>(item + 28) & 0x7FFFFFFFu;
        store<u32>(item + 28, w | ((static_cast<u32>(load<u8>(r + 12)) & 1) << 31));
    }
    for (u32 i = 0;; ++i) {
        const u32 list = load<u32>(load<u32>(m + 48) + 4);
        if (!(i < (list != 0 ? static_cast<u32>(load<u16>(list + 8)) : 0u))) break;
        const u32 rec = load<u32>(list) + 8 * i;
        if (load<u32>(static_cast<u32>(call(ctx, kKeysFind, {m + 24, rec + 4}))) != 0) {
            call(ctx, kHashToString, {load<u32>(rec + 4)});
            continue;
        }
        u32 item = 0;
        if (const u32 s = static_cast<u32>(call(ctx, kSmallAlloc, {24})); s != 0) {
            const u32 models = load<u32>(rec);
            const u32 cnt = models != 0 ? static_cast<u32>(load<u16>(models + 8)) : 0u;
            call(ctx, kPtrVectorConstruct, {s});
            store<u16>(s + 6, 4);
            call(ctx, kPtrVectorInit, {s, cnt, 0, 1});
            store<u32>(s + 20, rec);
            item = s;
        }
        const MapPush p = map_push_key(ctx, m + 24, rec + 4, item);
        const u32 fl = load<u16>(m + 30);
        store<u16>(m + 28, high_water(p));
        store<u16>(m + 30, static_cast<u16>(fl | 1));
        call(ctx, kAttachBuild, {item, m});
    }
    call(ctx, kModelReset, {m, 0, 0});
}
