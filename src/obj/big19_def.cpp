// Game code, large-function sweep part 19: the def base initialiser (the
// callee of DefBase_Ctor_000C2E8C).
#include "big19.h"

using namespace sly4;
using namespace sly4::big19;

namespace {
namespace hd {
constexpr u32 Def_Resolve34B0F8 = 0x0034B0F8;     // (u32 rec +56 hash, u32 owner +164) -> value (name?)
constexpr u32 Def_ReadParams490360 = 0x00490360;  // (p, rec +48) (name?)
constexpr u32 Def_FindByHash57CE08 = 0x0057CE08;  // (u32 hash, 0) -> def or 0
constexpr u32 WorldObj_FindByHash = 0x005801DC;   // (u32 hash, 0) -> object or 0
constexpr u32 SplineMap_Find = 0x007335C4;        // (map, const u32* hash) -> slot
constexpr u32 SortedMap_Find744AC4 = 0x00744AC4;  // (map, const u32* key) -> slot

constexpr u32 kDefBaseVt = 0x00B12230;   // the def base vtable
constexpr u32 kSplineHolder = 0x00F8F158;  // -> spline instance holder (+16: its map)
constexpr u32 kDefTable = 0x01013B04;    // {u16 cap, u16 count, u16 mark, u16 flags, {hash, def}* data}
constexpr u32 kComponents = 0x01056F04;  // the component registry map
constexpr u32 kZero = 0x000C26D0;        // 0.0f
}  // namespace hd
}  // namespace

// u32 f(Def* p, const Record* rec): the def base. The flag word +32 takes
// bit 31 / 30 from the bytes rec +24 / +25 (word stores), bits 29..24 from
// rec +26..+31 (byte stores to +32, bit 0 of each), bit 23 from rec +32;
// +36 / +40 = rec +36 / +40, +44 = hash(rec +16 name), +48 = hash(rec +20),
// +4 / +8 / +12 / +16 = 0, vtable, +120 / +124 / +132 = rec +52 / +56 /
// +60, +128 / +136 = 0, +20..+28 = rec +0..+8, the name rec +12 copied to
// +56 (63 chars) and hashed into +52. +4 = the def named by +44 (when set);
// without one, +8 = the world object named by +48 (or, missing, the spline
// with that hash). Def_ReadParams490360(p, rec +48). Each hash of the list
// at **(rec +64) found in the component registry is appended to the
// PtrVector +16 (made on first use, capacity 4; an unknown hash is turned
// into its string and skipped). An owner (+4 of type 20, or +8 of type 8
// through its +208) with a +164 gives +128 = Def_Resolve34B0F8(+124, it).
// {+52, p} is appended to the def table; then, when rec +72 > 0 and the +8
// object's frame (+208 +60) is not 0, its vfunc 64 is called with f1 = rec
// +72. f1 is left as rec +72; r3 as the vfunc's result, else the frame +48
// (frame +60 == 0) or the table flags | 1.
DECOMP(000C26D8, DefBase_Init_000C26D8) {
    using namespace hd;
    using namespace b19fn;
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    Stack frame(ctx, 192);
    u32 w = load<u32>(p + 32);
    w = (w & 0x7FFFFFFFu) | ((load<u8>(rec + 24) & 1u) << 31);
    store<u32>(p + 32, w);
    w = (w & ~0x40000000u) | ((load<u8>(rec + 25) & 1u) << 30);
    u32 b = w >> 24;
    store<u32>(p + 32, w);
    b = (b & ~0x20u) | ((load<u8>(rec + 26) & 1u) << 5);
    store<u8>(p + 32, static_cast<u8>(b));
    b = (b & ~0x10u) | ((load<u8>(rec + 27) & 1u) << 4);
    store<u8>(p + 32, static_cast<u8>(b));
    const u32 b28 = load<u8>(rec + 28);
    store<u32>(p + 12, 0);
    b = (b & ~0x08u) | ((b28 & 1u) << 3);
    store<u32>(p + 4, 0);
    store<u8>(p + 32, static_cast<u8>(b));
    const u32 b29 = load<u8>(rec + 29);
    store<u32>(p + 8, 0);
    b = (b & ~0x04u) | ((b29 & 1u) << 2);
    store<u8>(p + 32, static_cast<u8>(b));
    const u32 b30 = load<u8>(rec + 30);
    store<u32>(p + 0, kDefBaseVt);
    b = (b & ~0x02u) | ((b30 & 1u) << 1);
    store<u32>(p + 16, 0);
    store<u8>(p + 32, static_cast<u8>(b));
    b = (b & 0xFEu) | (load<u8>(rec + 31) & 1u);
    w = (w & 0x00FFFFFFu) | (b << 24);
    store<u8>(p + 32, static_cast<u8>(b));
    w = (w & ~0x00800000u) | ((load<u32>(rec + 32) & 1u) << 23);
    store<u32>(p + 32, w);
    store<u32>(p + 36, load<u32>(rec + 36));
    const u32 name = load<u32>(rec + 16);
    store<u32>(p + 40, load<u32>(rec + 40));
    const u32 h44 = static_cast<u32>(call(ctx, String_HashNoCase, {name}));
    const u32 name2 = load<u32>(rec + 20);
    store<u32>(p + 44, h44);
    const u32 h48 = static_cast<u32>(call(ctx, String_HashNoCase, {name2}));
    const u32 r52 = load<u32>(rec + 52), r56 = load<u32>(rec + 56), r60 = load<u32>(rec + 60);
    const u32 r0 = load<u32>(rec + 0), r12 = load<u32>(rec + 12);
    store<u32>(p + 136, 0);
    store<u32>(p + 132, r60);
    store<u32>(p + 128, 0);
    store<u32>(p + 124, r56);
    store<u32>(p + 120, r52);
    store<u32>(p + 48, h48);
    store<u32>(p + 20, r0);
    store<u32>(p + 24, load<u32>(rec + 4));
    store<u32>(p + 28, load<u32>(rec + 8));
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(r60));
    call(ctx, String_CopyN, {p + 56, r12, 63});
    store<u32>(p + 52, static_cast<u32>(call(ctx, String_HashNoCase, {p + 56})));

    u32 def = load<u32>(p + 4);
    const u32 h = load<u32>(p + 44);
    if (h != 0) {
        def = static_cast<u32>(call(ctx, Def_FindByHash57CE08, {h, 0}));
        store<u32>(p + 4, def);
    }
    if (def == 0) {
        const u32 h2 = load<u32>(p + 48);
        if (h2 != 0) {
            const u32 o = static_cast<u32>(call(ctx, WorldObj_FindByHash, {h2, 0}));
            store<u32>(p + 8, o);
            if (o == 0) {
                const u32 key = frame.addr() + 116;
                const u32 map = load<u32>(load<u32>(kSplineHolder) + 16);
                store<u32>(key, load<u32>(p + 48));
                const u32 slot = static_cast<u32>(call(ctx, SplineMap_Find, {map, key}));
                store<u32>(p + 8, load<u32>(slot));
            }
        }
    }
    call(ctx, Def_ReadParams490360, {p, load<u32>(rec + 48)});

    if (load<u32>(rec + 64) != 0) {
        const u32 key = frame.addr() + 112;
        for (u32 k = 0;; ++k) {
            const u32 list = load<u32>(load<u32>(rec + 64));
            const u32 n = list != 0 ? load<u16>(list + 8) : 0;
            if (k >= n) break;
            const u32 hk = load<u32>(load<u32>(list) + 4 * k);
            store<u32>(key, hk);
            const u32 c = load<u32>(static_cast<u32>(call(ctx, SortedMap_Find744AC4, {kComponents, key})));
            if (c == 0) {
                call(ctx, Hash_ToString, {hk});
                continue;
            }
            u32 v = load<u32>(p + 16);
            if (v == 0) {
                v = static_cast<u32>(call(ctx, SmallAlloc, {20}));
                store<u32>(p + 16, v);
                if (v != 0) {  // a failed allocation goes on with v = 0 (as the original)
                    call(ctx, PtrVector_Construct, {v});
                    store<u16>(v + 6, 4);
                    v = load<u32>(p + 16);
                }
            }
            PtrVectorPush(ctx, v, c);
        }
    }

    u32 own = 0;
    const u32 d4 = load<u32>(p + 4);
    if (d4 != 0 && ((load<u32>(d4 + 4) >> 10) & 0x3Fu) == 20) own = load<u32>(d4 + 164);
    if (own == 0) {
        const u32 o8 = load<u32>(p + 8);
        if (o8 != 0 && ((load<u32>(o8 + 4) >> 10) & 0x3Fu) == 8) own = load<u32>(load<u32>(o8 + 208) + 164);
    }
    if (own != 0) store<u32>(p + 128, static_cast<u32>(call(ctx, Def_Resolve34B0F8, {load<u32>(p + 124), own})));

    // append {+52 hash, p} to the def table
    const u32 t = kDefTable;
    const u32 hash = load<u32>(p + 52);
    u32 n = load<u16>(t + 2);
    const u32 cap = load<u16>(t + 0);
    if (n == cap) {
        call(ctx, Array_Reserve, {t, (cap << 1) & 0xFFFEu});
        n = load<u16>(t + 2);
    }
    const u32 data = load<u32>(t + 8);
    store<u32>(data + (n << 3), hash);
    store<u32>(data + (load<u16>(t + 2) << 3) + 4, p);
    const u32 n1 = load<u16>(t + 2) + 1;
    const u32 hi = load<u16>(t + 4);
    const u32 fl = load<u16>(t + 6) | 1u;
    store<u16>(t + 2, static_cast<u16>(n1));
    store<u16>(t + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFFu) ? hi : n1));
    store<u16>(t + 6, static_cast<u16>(fl));

    const f64 zero = LfsX(kZero);
    ctx.fpr[1] = LfsX(rec + 72);
    u64 r = fl;
    if (ctx.fpr[1] > zero) {
        const u32 o = load<u32>(p + 8);
        const u32 fr = load<u32>(o + 208) + 48;
        r = fr;
        if (LfsX(fr + 12) != zero) r = VCall(ctx, o, 64, {o});
    }
    ret(ctx, r);
}
