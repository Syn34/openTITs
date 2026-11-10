// Game code, medium sweep part 5: more inlined copies of the "spawn from
// a query" method (as mid4_spawn.cpp): for an owner o and a target t
// (unless t is flagged as excluded: t->+4 bit 0x00400000 and t->+184 bit
// 2 set, the class's keep bit clear), o->vfn[8](o, t) gives a value that
// a local query is initialized with (Obj_Init22(q, 1, 0, value, o, 0,
// 0x00B12000, 0x01164BD0, 0x01164B30, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, -,
// -, 0; m1, m1) with m1 the f32 just before the function); the query
// gets o's four bytes +172.. (0 without o), its flags and the class's
// extra fields; t->vfn[2](t, q); a SmallAlloc'ed object is built on
// QuerySpawn_BaseCtor_0020C868(p, q, n, 1) and the class's inlined
// constructor, then its vfn[2]() is called (on a null object too, as the
// original); the query's PtrVector is freed. Returns the object (0 when
// excluded or out of memory).
#include "mid5.h"

using namespace sly4;
using namespace sly4::mid5;

namespace {

namespace qfn {
constexpr u32 Obj_Init22 = 0x000CDFB0;           // 22 arguments (14 on the stack), f32 x2
constexpr u32 SmallAlloc = 0x0055238C;           // (size) -> p
constexpr u32 QuerySpawn_BaseCtor = 0x0020C868;  // (p, Query* q, n, 1)
constexpr u32 memset = 0x00726E54;               // (p, c, n) -> p
constexpr u32 Name_Resolve_002E1220 = 0x002E1220;  // (x, u32 hash) -> u32 (name?)
constexpr u32 SplineMap_Find = 0x007335C4;       // (map, const u32* key) -> value*
}  // namespace qfn

constexpr u32 kResolveArg = 0x00F64C20;  // u32 (name?)
constexpr u32 kSplineHolder = 0x00F8F158;  // -> +16 the spline map (name?)

struct QsCfg {
    u32 keep_word, keep_mask;  // t's keep bit
    u32 m1;                    // the f32 passed twice to Obj_Init22
    u32 or4, or8;              // or'ed into the query's +4 / +8
    u32 size;                  // SmallAlloc size
    u32 n;                     // the base ctor's third argument
    u32 qsize;                 // the query's stack extent
};

template <typename QInit, typename Ctor>
void QuerySpawn5(PPUContext& ctx, const QsCfg& c, QInit qinit, Ctor ctor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = arg<u32>(ctx, 1);
    if ((load<u32>(t + 4) & 0x00400000u) != 0 && (load<u8>(t + 184) & 4) != 0 && (load<u32>(t + c.keep_word) & c.keep_mask) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, c.qsize);
    const u32 q = frame.addr();
    const f64 m1 = std::bit_cast<f64>(LfsBits(load<u32>(c.m1)));
    // the stack slots of arguments 19 / 20 are left unwritten by the original
    callf(ctx, qfn::Obj_Init22,
          {q, 1, 0, v, o, 0, 0x00B12000, 0x01164BD0, 0x01164B30, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0xFFFFFFFFull, 0, 0, 0},
          {m1, m1});
    store<u32>(q + 144, 0);
    store<u8>(q + 148, 1);
    store<u8>(q + 149, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(q + 150 + k, o != 0 ? load<u8>(o + 172 + k) : 0);
    store<u32>(q + 4, load<u32>(q + 4) | c.or4);
    if (c.or8 != 0) store<u32>(q + 8, load<u32>(q + 8) | c.or8);
    qinit(q, t);
    VCall(ctx, t, 8, {t, q});
    const u32 p = static_cast<u32>(call(ctx, qfn::SmallAlloc, {c.size}));
    if (p != 0) {
        call(ctx, qfn::QuerySpawn_BaseCtor, {p, q, c.n, 1});
        ctor(p, q);
    }
    VCall(ctx, p, 8, {p});  // also for p == 0, as the original
    mid1::PtrVecDeleteSmall(ctx, q);
    ret(ctx, p);
}

}  // namespace

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +8 0x10, query +4 |=
// 0x00400000 / +8 |= 0x10, +156 = -1; a 432-byte object of vtable
// 0x00B1A5E8 whose +428 is the query's +156).
DECOMP(0021D5B0, QuerySpawn_0021D5B0) {
    QuerySpawn5(
        ctx, {8, 0x10, 0x0021D5AC, 0x00400000, 0x10, 432, 32, 168},
        [&](u32 q, u32) { store<u32>(q + 156, 0xFFFFFFFFu); },
        [&](u32 p, u32 q) {
            store<u32>(p, 0x00B1A5E8);
            store<u32>(p + 428, load<u32>(q + 156));
        });
}

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +8 0x80, query +4 |=
// 0x00400000 / +8 |= 0x80; a 464-byte object (base n = 256) of vtable
// 0x00B1B180: +428 = 0, +432 a PtrVector of 8-byte elements, capacity 2).
DECOMP(00225348, QuerySpawn_00225348) {
    QuerySpawn5(
        ctx, {8, 0x80, 0x00225340, 0x00400000, 0x80, 464, 256, 168}, [&](u32, u32) {},
        [&](u32 p, u32) {
            store<u32>(p + 428, 0);
            store<u32>(p, 0x00B1B180);
            mid1::PtrVecNew(ctx, p + 432, 2, 8, 1);
        });
}

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +4 0x20000000, query +4
// |= 0x20400000; a 688-byte object of vtable 0x00B1AEA0: +656 / +660 /
// +664 / +672 cleared, the u32 +44 of its +416 = 17, the 108-byte blocks
// +428 / +536 zeroed).
DECOMP(002222F0, QuerySpawn_002222F0) {
    QuerySpawn5(
        ctx, {4, 0x20000000, 0x002222EC, 0x20400000, 0, 688, 32, 168}, [&](u32, u32) {},
        [&](u32 p, u32) {
            const u32 x = load<u32>(p + 416);
            store<u32>(p + 656, 0);
            store<u32>(p + 660, 0);
            store<u32>(p + 664, 0);
            store<u32>(p, 0x00B1AEA0);
            store<u8>(p + 672, 0);
            store<u32>(x + 44, 17);
            call(ctx, qfn::memset, {p + 428, 0, 108});
            call(ctx, qfn::memset, {p + 536, 0, 108});
        });
}

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +4 0x40000000, query +4
// |= 0x40400000 and its extra fields cleared: +156 u32 hash, +160 u32,
// +164 / +165 u8, +168 u32, +172 char[256]; a 704-byte object of vtable
// 0x00B1AA38 taking them: +428 = Name_Resolve_002E1220(*kResolveArg,
// hash), +432 / +436 / +437 / +440 copied, +444 the string).
DECOMP(00220560, QuerySpawn_00220560) {
    QuerySpawn5(
        ctx, {4, 0x40000000, 0x00220558, 0x40400000, 0, 704, 32, 432},
        [&](u32 q, u32) {
            store<u32>(q + 156, 0);
            store<u32>(q + 160, 0);
            store<u8>(q + 164, 0);
            store<u8>(q + 165, 0);
            store<u32>(q + 168, 0);
            store<u8>(q + 172, 0);
        },
        [&](u32 p, u32 q) {
            store<u32>(p + 428, 0);
            store<u32>(p, 0x00B1AA38);
            store<u32>(p + 432, load<u32>(q + 160));
            store<u8>(p + 436, load<u8>(q + 164));
            store<u8>(p + 437, load<u8>(q + 165));
            store<u32>(p + 440, load<u32>(q + 168));
            store<u32>(p + 428, static_cast<u32>(call(ctx, qfn::Name_Resolve_002E1220, {load<u32>(kResolveArg), load<u32>(q + 156)})));
            call(ctx, m5fn::String_CopyN, {p + 444, q + 172, 256});
        });
}

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +8 0x20, query +4 |=
// 0x00400000 / +8 |= 0x20; the query's +156 = a hash: t's +188 when t's
// +8 bit 0x20 is set, else String_HashNoCase(String_Format(0x00AD3854,
// name after its first '_')) of t's name (+140, or when empty the
// string at (t->+180->+120 & ~1) when t has a +180); a 432-byte object of
// vtable 0x00B1ABB0 whose +428 is the spline map's value for the hash).
DECOMP(00221238, QuerySpawn_00221238) {
    QuerySpawn5(
        ctx, {8, 0x20, 0x00221234, 0x00400000, 0x20, 432, 32, 168},
        [&](u32 q, u32 t) {
            store<u32>(q + 156, 0);
            const u32 t8 = load<u32>(t + 8);
            u32 s = t + 140;
            if (s == 0 || load<u8>(s) == 0)
                if (const u32 n = load<u32>(t + 180); n != 0) s = load<u32>(n + 120) & 0xFFFFFFFEu;
            u32 h;
            if ((t8 & 0x20) != 0) {
                h = load<u32>(t + 188);
            } else {
                if (load<u8>(s) != 0) {
                    for (;;) {
                        const u32 ch = load<u8>(s);
                        ++s;
                        if (ch == '_' || load<u8>(s) == 0) break;
                    }
                }
                const u32 f = static_cast<u32>(call(ctx, m5fn::String_Format, {0x00AD3854, s}));
                h = static_cast<u32>(call(ctx, m5fn::String_HashNoCase, {f}));
            }
            store<u32>(q + 156, h);
        },
        [&](u32 p, u32 q) {
            store<u32>(p + 428, 0);
            store<u32>(p, 0x00B1ABB0);
            store<u32>(q + 160, load<u32>(q + 156));  // the key, in the query's spare word
            const u32 v = static_cast<u32>(call(ctx, qfn::SplineMap_Find, {load<u32>(load<u32>(kSplineHolder) + 16), q + 160}));
            store<u32>(p + 428, load<u32>(v));
        });
}
