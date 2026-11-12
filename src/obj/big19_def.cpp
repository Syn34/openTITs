// Game code, large-function sweep part 19: the def base initialiser (the
// callee of DefBase_Ctor_000C2E8C).
#include "big19.h"

namespace sly4::obj {

namespace {
constexpr u32 Def_Resolve34B0F8 = 0x0034B0F8;     // (u32 rec +56 hash, u32 owner +164) -> value (name?)
constexpr u32 Def_ReadParams490360 = 0x00490360;  // (p, rec +48) (name?)
constexpr u32 Def_FindByHash57CE08 = 0x0057CE08;  // (u32 hash, 0) -> def or 0
constexpr u32 WorldObj_FindByHash = 0x005801DC;   // (u32 hash, 0) -> object or 0
constexpr u32 SplineMap_Find = 0x007335C4;        // (map, const u32* hash) -> slot
constexpr u32 SortedMap_Find744AC4 = 0x00744AC4;  // (map, const u32* key) -> slot
constexpr u32 PtrVector_Construct = 0x005516E0;   // (vec)
constexpr u32 Array_Reserve = 0x00558F3C;         // (array, u32 n)
constexpr u32 Hash_ToString = 0x0056A6E0;         // (u32 hash) -> const char*
constexpr u32 String_HashNoCase = 0x0056A710;     // (const char*) -> u32
constexpr u32 String_CopyN = 0x00553638;          // (dst, src, u32 n)
constexpr u32 kDefBaseVt = 0x00B12230;   // the def base vtable
constexpr u32 kSplineHolder = 0x00F8F158;  // -> spline instance holder (+16: its map)
constexpr u32 kDefTable = 0x01013B04;    // {u16 cap, u16 count, u16 mark, u16 flags, {hash, def}* data}
constexpr u32 kComponents = 0x01056F04;  // the component registry map
constexpr u32 kZero = 0x000C26D0;        // 0.0f

struct DefRec {
    be_u32 w0, w4, w8;  // +0 .. +8
    be_u32 name12;      // +12
    be_u32 name16;      // +16
    be_u32 name20;      // +20
    u8 b24[8];          // +24 .. +31
    be_u32 w32;         // +32
    be_u32 w36;         // +36
    be_u32 w40;         // +40
    pad<4> _44;
    be_u32 w48;         // +48
    be_u32 w52;         // +52
    be_u32 w56;         // +56
    be_u32 w60;         // +60
    be_u32 list;        // +64: -> PtrVec of hashes
    pad<4> _68;
    be_f32 f72;         // +72
};
static_assert(offsetof(DefRec, w32) == 32 && offsetof(DefRec, list) == 64 && offsetof(DefRec, f72) == 72);
struct DefBase {
    be_u32 vt;          // +0
    be_u32 def;         // +4
    be_u32 obj;         // +8
    be_u32 w12;         // +12
    be_u32 vec;         // +16: PtrVec* of components
    be_u32 w20, w24, w28;  // +20 .. +28
    WordBytes flags;    // +32 (byte +32 its top byte)
    be_u32 w36;         // +36
    be_u32 w40;         // +40
    be_u32 h44;         // +44
    be_u32 h48;         // +48
    be_u32 h52;         // +52
    u8 name[64];        // +56
    be_u32 w120;        // +120
    be_u32 w124;        // +124
    be_u32 w128;        // +128
    be_u32 w132;        // +132
    be_u32 w136;        // +136
};
static_assert(offsetof(DefBase, flags) == 32 && offsetof(DefBase, name) == 56 && offsetof(DefBase, w136) == 136);
// The def table: {u16 cap, u16 count, u16 mark, u16 flags, {hash, def}* data}.
struct DefTable {
    be_u16 cap, count, mark, flags;  // +0 .. +6
    be_u32 data;                     // +8
};
}  // namespace

// void f(Def* p, const Record* rec) (no caller reads r3 or f1): the def base. The flag word +32 takes
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
// +72.
void DefBase_Init_000C26D8(DefBase* p, DefRec* rec) {
    GuestFrame frame(192);
    be_u32* keys = frame.as<be_u32>(112);  // sp+112 the component key, sp+116 the spline key
    u32 w = p->flags.w;
    w = (w & 0x7FFFFFFFu) | ((rec->b24[0] & 1u) << 31);
    p->flags.w = w;
    w = (w & ~0x40000000u) | ((rec->b24[1] & 1u) << 30);
    u32 b = w >> 24;
    p->flags.w = w;
    for (u32 k = 2; k < 8; ++k) b = (b & ~(0x80u >> k)) | ((rec->b24[k] & 1u) << (7 - k));
    w = (w & 0x00FFFFFFu) | (b << 24);
    p->flags.b[0] = static_cast<u8>(b);
    p->w12 = 0u;
    p->def = 0u;
    p->obj = 0u;
    p->vt = kDefBaseVt;
    p->vec = 0u;
    w = (w & ~0x00800000u) | ((rec->w32 & 1u) << 23);
    p->flags.w = w;
    p->w36 = rec->w36.get();
    const u32 name = rec->name16;
    p->w40 = rec->w40.get();
    const u32 h44 = call<u32>(String_HashNoCase, name);
    const u32 name2 = rec->name20;
    p->h44 = h44;
    const u32 h48 = call<u32>(String_HashNoCase, name2);
    p->w136 = 0u;
    p->w132 = rec->w60.get();
    p->w128 = 0u;
    p->w124 = rec->w56.get();
    p->w120 = rec->w52.get();
    p->h48 = h48;
    p->w20 = rec->w0.get();
    p->w24 = rec->w4.get();
    p->w28 = rec->w8.get();
    call(String_CopyN, &p->name[0], rec->name12.get(), 63u);
    p->h52 = call<u32>(String_HashNoCase, &p->name[0]);

    u32 def = p->def;
    if (const u32 h = p->h44; h != 0) {
        def = call<u32>(Def_FindByHash57CE08, h, 0u);
        p->def = def;
    }
    if (def == 0) {
        if (const u32 h2 = p->h48; h2 != 0) {
            const u32 o = call<u32>(WorldObj_FindByHash, h2, 0u);
            p->obj = o;
            if (o == 0) {
                const u32 map = gm<be_u32>(global<be_u32>(kSplineHolder) + 16);
                keys[1] = p->h48.get();
                p->obj = gm<be_u32>(call<u32>(SplineMap_Find, map, &keys[1])).get();
            }
        }
    }
    call(Def_ReadParams490360, p, rec->w48.get());

    if (rec->list != 0u) {
        for (u32 k = 0;; ++k) {
            const u32 list = gm<be_u32>(rec->list);
            const u32 n = list != 0 ? gm<PtrVec<u8>>(list).count.get() : 0u;
            if (k >= n) break;
            const u32 hk = gm<be_u32>(gm<be_u32>(list) + 4 * k);
            keys[0] = hk;
            const u32 c = gm<be_u32>(call<u32>(SortedMap_Find744AC4, kComponents, &keys[0]));
            if (c == 0) {
                call(Hash_ToString, hk);
                continue;
            }
            u32 v = p->vec;
            if (v == 0) {
                v = call<u32>(fn::SmallAlloc, 20u);
                p->vec = v;
                if (v != 0) {  // a failed allocation goes on with v = 0 (as the original)
                    call(PtrVector_Construct, v);
                    gm<be_u16>(v + 6) = 4;
                    v = p->vec;
                }
            }
            PtrVecPush(gm<PtrVec<u8>>(v), c);
        }
    }

    u32 own = 0;
    const u32 d4 = p->def;
    if (d4 != 0 && ((gm<be_u32>(d4 + 4) >> 10) & 0x3Fu) == 20) own = gm<be_u32>(d4 + 164);
    if (own == 0) {
        const u32 o8 = p->obj;
        if (o8 != 0 && ((gm<be_u32>(o8 + 4) >> 10) & 0x3Fu) == 8) own = gm<be_u32>(gm<be_u32>(o8 + 208) + 164);
    }
    if (own != 0) p->w128 = call<u32>(Def_Resolve34B0F8, p->w124.get(), own);

    // append {+52 hash, p} to the def table
    DefTable& t = global<DefTable>(kDefTable);
    const u32 hash = p->h52;
    u32 n = t.count;
    const u32 cap = t.cap;
    if (n == cap) {
        call(Array_Reserve, &t, (cap << 1) & 0xFFFEu);
        n = t.count;
    }
    const u32 data = t.data;
    gm<be_u32>(data + (n << 3)) = hash;
    gm<be_u32>(data + (static_cast<u32>(t.count) << 3) + 4) = guest(p);
    const u32 n1 = t.count + 1;
    const u32 hi = t.mark;
    const u32 fl = t.flags | 1u;
    t.count = static_cast<u16>(n1);
    t.mark = static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFFu) ? hi : n1);
    t.flags = static_cast<u16>(fl);

    const f64 zero = LfsW(global<be_u32>(kZero));
    const f64 f72 = LfsW(bits(rec->f72));
    if (f72 > zero) {
        const u32 o = p->obj;
        const u32 fr = gm<be_u32>(o + 208) + 48;
        if (LfsW(gm<be_u32>(fr + 12)) != zero) vcall(&gm<u8>(o), 64, f72);
    }
}

}  // namespace sly4::obj

NATIVE(000C26D8, sly4::obj::DefBase_Init_000C26D8)
