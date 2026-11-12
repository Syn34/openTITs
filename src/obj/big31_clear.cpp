// Game code, large-function sweep part 31: whether the segment between two
// points is free of geometry (Havok ray cast, two objects' shapes ignored).
#include "../sly4.h"
#include "big31.h"

using namespace sly4;
using namespace sly4::big31;

namespace {
constexpr u32 kClearMasks = 0x00235100;      // .text: lane masks y / z / w
constexpr u32 kGame = 0x00F65CA0;       // Game* (+0x1281D4 the physics holder, +4 its world)
constexpr u32 kVtRayCol = 0x00F1DE80;   // all-hits ray collector vtable (80-byte hits, 8 inline)
constexpr u32 kHkHeap = 0x011DF31C;     // Havok heap allocator (vfn[20] (p, bytes) frees)
constexpr u32 kPropKind = 0xA734004D;   // shape property: the shape's kind
constexpr u32 kKindOwned = 0x7A30D7E3;  // that kind: the shape belongs to the object in kPropOwner
constexpr u32 kPropOwner = 0x91C7CF5B;  // shape property: the owning object
constexpr u32 HkWorld_CastRay92E510 = 0x0092E510;  // (hkpWorld*, const RayInput*, Collector*)

// The low word of property `key` in the shape's list (+124 the 16-byte
// entries {key, -, u64 value}, +128 the s32 count), 0 when absent.
u32 PropLo(u32 props, u32 key) {
    for (s32 j = 0; j < load<s32>(props + 128); ++j) {
        const u32 e = load<u32>(props + 124) + 16 * static_cast<u32>(j);
        if (load<u32>(e) == key) return load<u32>(e + 12);
    }
    return 0;
}
}  // namespace

// bool f(const f32 a[3], const f32 b[3], Obj* x, Obj* y) (called by
// f_000EBEF8 and f_00235688). Casts a ray from a to b (RayInput at sp +
// 112: from, to, +32 / +36 / +40 = 0; the collector at sp + 160 with eight
// inline 80-byte hits at sp + 192; the hits are not sorted). Hits whose
// shape (+68) type (+28 & 0x1F) is 5, 6 or >= 16 are ignored. A shape with
// byte +24 == 1 or 2 has a property list at shape + s8 +16; when its
// kPropKind is kKindOwned its owner is the kPropOwner property. Hits owned
// by x or y are ignored; any other hit (with no owner or another one)
// blocks: r3 = 0. No blocking hit: r3 = 1. A heap hit array is freed first
// through kHkHeap->vfn[20] (entry in r6, OPD in r7).
DECOMP(00235130, Ray_ClearBetween_00235130) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), y = arg<u32>(ctx, 3);
    Stack frame(ctx, 720);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    const u32 ray = sp + 112, coll = sp + 160, hits = sp + 192;
    store<u8>(sp + 144, 0);
    store<u32>(sp + 152, 0);
    const auto mk = LoadMasks3(kClearMasks);
    Stvx(ray, RowSelW(a, a + 4, a + 8, mk));
    Stvx(ray + 16, RowSelW(b, b + 4, b + 8, mk));
    store<u32>(sp + 148, 0);
    const u32 world = load<u32>(load<u32>(load<u32>(kGame) + 0x1281D4) + 4);
    store<u32>(coll + 4, 0x3F800000u);
    store<u32>(coll, kVtRayCol);
    store<u32>(coll + 16, hits);
    store<u32>(coll + 20, 0);
    store<u32>(coll + 24, 0x80000008u);
    for (u32 j = 0; j < 8; ++j) {
        const u32 h = hits + 80 * j;
        store<u32>(h + 16, 0x3F800000u);
        store<u32>(h + 20, 0xFFFFFFFFu);
        store<u32>(h + 64, 0);
        store<u32>(h + 32, 0xFFFFFFFFu);
        store<u32>(h + 68, 0);
    }
    store<u32>(coll + 4, 0x3F800000u);
    store<u32>(coll + 20, 0);
    call(ctx, HkWorld_CastRay92E510, {world, ray, coll});
    const s32 n = load<s32>(coll + 20);
    const u32 hp = load<u32>(coll + 16);
    const u32 cw = load<u32>(coll + 24);
    u32 clear = 1;
    for (s32 i = 0; i < n; ++i) {
        const u32 sh = load<u32>(hp + 80 * static_cast<u32>(i) + 68);
        const u32 ty = load<u32>(sh + 28) & 0x1F;
        if (ty >= 16 || ty == 6 || ty == 5) continue;
        const s8 kind = static_cast<s8>(load<u8>(sh + 24));
        const u32 rel = sh + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(sh + 16))));
        const u32 props = kind == 1 ? rel : (kind == 2 ? rel : 0);
        u32 owner = 0;
        if (props != 0 && PropLo(props, kPropKind) == kKindOwned)
            owner = load<s32>(props + 128) > 0 ? PropLo(props, kPropOwner) : 0;
        if (owner != 0 && (owner == x || owner == y)) continue;
        clear = 0;
        break;
    }
    if ((cw & 0x80000000u) == 0) {
        store<u32>(coll + 20, 0);
        store<u32>(coll, kVtRayCol);
        const u32 opd = load<u32>(load<u32>(kHkHeap) + 20);
        const u64 size = static_cast<u64>(static_cast<s64>(static_cast<s32>(80u * (cw & 0x3FFFFFFFu))));
        call_ptr(ctx, opd, {kHkHeap, hp, size, load<u32>(opd), opd});  // entry in r6, OPD in r7
    }
    ret(ctx, clear);
}
