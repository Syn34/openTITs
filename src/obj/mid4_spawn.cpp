// Game code, medium sweep part 4: four inlined copies of a "spawn from a
// query" method (two pairs). For an owner o and a target t (unless t is
// flagged as excluded): o->vfn[8](o, t) gives a value that a local query
// (168 bytes at r1+224 in the original) is initialized with
// (Obj_Init22(q, 1, 0, value, o, 0, 0x00B12000, 0x01164BD0, 0x01164B30, 0,
// 0, 1, 0, 0, 1, 0, 0, 0, -1, -, -, 0; -1.0f, -1.0f)); the query gets o's
// four bytes +172.. (0 without o) and its flags; t->vfn[2](t, q); a new
// object is SmallAlloc'ed and constructed from q, then its vfn[2]() is
// called (on a null object too, as the original); the query's PtrVector
// is freed. Returns the object (0 when excluded or out of memory).
#include "mid4.h"

using namespace sly4;
using namespace sly4::mid4;

namespace {

namespace sfn {
constexpr u32 Obj_Init22 = 0x000CDFB0;     // 22 arguments (14 on the stack), f32 x2
constexpr u32 SmallAlloc = 0x0055238C;     // (size) -> p
constexpr u32 Ctor_0020C868 = 0x0020C868;  // (p, query, 32, 1)
constexpr u32 kTvDisable = 0x010B099C;     // TuningVar (u8 +72): spawning off (name?)
}  // namespace sfn

struct SpawnCfg {
    u32 xword;     // t's flag word holding the "keep" bit (+4 / +8; 0: no exclusion)
    u32 xmask;     // that bit
    bool tv;       // returns 0 while kTvDisable is set
    u32 m1;        // the address of the original's -1.0f (the words before it)
    u32 or4, or8;  // the flags or'ed into the query's +4 / +8
    u32 size;      // SmallAlloc size
    u32 ctor;      // ctor(p, q) (0: Ctor_0020C868(p, q, 32, 1), then vt)
    u32 vt;
    u32 byte0 = 0;  // the query's +150 without o (the others 0)
};

void QuerySpawn(PPUContext& ctx, const SpawnCfg& c) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = arg<u32>(ctx, 1);
    // excluded: t's +4 bit 0x00400000 and +184 bit 2 set, the keep bit clear
    if (c.xword != 0 && (load<u32>(t + 4) & 0x00400000u) != 0 && (load<u8>(t + 184) & 4) != 0 && (load<u32>(t + c.xword) & c.xmask) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (c.tv && load<u8>(sfn::kTvDisable + 72) != 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 168);
    const u32 q = frame.addr();
    const f64 m1 = std::bit_cast<f64>(LfsBits(load<u32>(c.m1)));
    // the stack slots of arguments 19 / 20 are left unwritten by the original
    callf(ctx, sfn::Obj_Init22,
          {q, 1, 0, v, o, 0, 0x00B12000, 0x01164BD0, 0x01164B30, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0xFFFFFFFFull, 0, 0, 0},
          {m1, m1});
    store<u32>(q + 144, 0);
    store<u8>(q + 148, 1);
    store<u8>(q + 149, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(q + 150 + k, o != 0 ? load<u8>(o + 172 + k) : (k == 0 ? c.byte0 : 0));
    store<u32>(q + 4, load<u32>(q + 4) | c.or4);
    if (c.or8 != 0) store<u32>(q + 8, load<u32>(q + 8) | c.or8);
    VCall(ctx, t, 8, {t, q});
    u32 p = static_cast<u32>(call(ctx, sfn::SmallAlloc, {c.size}));
    if (p != 0) {
        if (c.ctor != 0) {
            call(ctx, c.ctor, {p, q});
        } else {
            call(ctx, sfn::Ctor_0020C868, {p, q, 32, 1});
            store<u32>(p, c.vt);
        }
    }
    VCall(ctx, p, 8, {p});  // also for p == 0, as the original
    if (const u32 vec = load<u32>(q); vec != 0) {
        VecDtor(ctx, vec);
        call(ctx, m4fn::SmallFree, {load<u32>(q), 20});
    }
    ret(ctx, p);
}

}  // namespace

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +4 0x02000000, query +4
// |= 0x02400000, a 624-byte object, ctor 0x00210478).
DECOMP(002105F8, QuerySpawn_002105F8) {
    QuerySpawn(ctx, {4, 0x02000000, true, 0x002105F0, 0x02400000, 0, 624, 0x00210478, 0});
}

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +4 0x04000000, query +4
// |= 0x04400000, a 624-byte object, ctor 0x00212FC8).
DECOMP(002130D0, QuerySpawn_002130D0) {
    QuerySpawn(ctx, {4, 0x04000000, true, 0x002130CC, 0x04400000, 0, 624, 0x00212FC8, 0});
}

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +8 0x8, query +4 |=
// 0x00400000 / +8 |= 0x8, a 432-byte object of vtable 0x00B19B58).
DECOMP(00212828, QuerySpawn_00212828) {
    QuerySpawn(ctx, {8, 0x8, false, 0x00212820, 0x00400000, 0x8, 432, 0, 0x00B19B58});
}

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +8 0x4, query +4 |=
// 0x00400000 / +8 |= 0x4, a 432-byte object of vtable 0x00B1A150).
DECOMP(00219A10, QuerySpawn_00219A10) {
    QuerySpawn(ctx, {8, 0x4, false, 0x00219A0C, 0x00400000, 0x4, 432, 0, 0x00B1A150});
}

// Obj* f(Owner* o, Obj* t): QuerySpawn without the exclusion (query +4 |=
// 0x00400000, a 432-byte object of vtable 0x00B19698; quirk kept: the
// query's +150 is 1 without o, a register left from the call setup).
DECOMP(0020CEA0, QuerySpawn_0020CEA0) {
    QuerySpawn(ctx, {0, 0, false, 0x0020CE9C, 0x00400000, 0, 432, 0, 0x00B19698, 1});
}

// Obj* f(Owner* o, Obj* t): QuerySpawn (keep bit +4 0x01000000, query +4
// |= 0x01400000, a 432-byte object of vtable 0x00B19FE0).
DECOMP(00218980, QuerySpawn_00218980) {
    QuerySpawn(ctx, {4, 0x01000000, false, 0x0021897C, 0x01400000, 0, 432, 0, 0x00B19FE0});
}

// ---------------------------------------------------------------- QueryEx variant

namespace {

namespace sfn2 {
constexpr u32 QueryEx_Construct = 0x000DFDF8;  // (QueryEx* q, ... 19 arguments; r7 = Def*)
constexpr u32 kCurrentWorld = 0x00F65CA0;      // World*
}  // namespace sfn2

// Obj* f(Owner* o, Obj* t): as QuerySpawn with a 208-byte QueryEx (r1+208
// in the original): QueryEx_Construct(q, 4, 128, o->vfn[8](o, t), o, 0,
// 0x00B1B4F0, 0x01164BD0, 0x01164B30, bit, 0, 0, 1, 0, 1, 0, 0, 0, -1)
// (bit: o->+728 bit 31; the stack slots of arguments 19 / 20 are left
// as they are), q->+12 |= or12, q->+200 = 0, t->vfn[2](t, q); then the
// world's counter (World +0x1281D4 -> +4 -> +116 -> +48) is incremented,
// copied to q->+200 and passed to number(o, n); a SmallAlloc'ed object is
// constructed from q (ctor(p, q)) and its vfn[2]() called (on null too, as
// the original); q's two record vectors (+164, +144) and its PtrVector
// are freed. Returns the object.
void QueryExSpawn(PPUContext& ctx, u32 or12, u32 number, u32 size, u32 ctor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    const u32 bit = load<u32>(o + 728) >> 31;
    Stack frame(ctx, 208);
    const u32 q = frame.addr();
    call(ctx, sfn2::QueryEx_Construct,
         {q, 4, 128, v, o, 0, 0x00B1B4F0, 0x01164BD0, 0x01164B30, bit, 0, 0, 1, 0, 1, 0, 0, 0, 0xFFFFFFFFull});
    store<u32>(q + 200, 0);
    store<u32>(q + 12, load<u32>(q + 12) | or12);
    VCall(ctx, t, 8, {t, q});
    const u32 y = load<u32>(load<u32>(load<u32>(load<u32>(sfn2::kCurrentWorld) + 0x1281D4) + 4) + 116);
    const u32 n = load<u32>(y + 48) + 1;
    store<u32>(y + 48, n);
    store<u32>(q + 200, n);
    call(ctx, number, {o, n});
    u32 p = static_cast<u32>(call(ctx, sfn::SmallAlloc, {size}));
    if (p != 0) call(ctx, ctor, {p, q});
    VCall(ctx, p, 8, {p});  // also for p == 0, as the original
    VecDtor(ctx, q + 164);
    VecDtor(ctx, q + 144);
    if (const u32 vec = load<u32>(q); vec != 0) {
        VecDtor(ctx, vec);
        call(ctx, m4fn::SmallFree, {load<u32>(q), 20});
    }
    ret(ctx, p);
}

}  // namespace

// Obj* f(Owner* o, Obj* t): QueryExSpawn (q->+12 |= 0xC00, numbered by
// 0x0022FAA0, a 1104-byte object, ctor 0x00233728).
DECOMP(00234324, QueryExSpawn_00234324) { QueryExSpawn(ctx, 0xC00, 0x0022FAA0, 1104, 0x00233728); }

// Obj* f(Owner* o, Obj* t): QueryExSpawn (q->+12 |= 0x1400, numbered by
// 0x00240AC0, a 640-byte object, ctor 0x002404F0).
DECOMP(00240C64, QueryExSpawn_00240C64) { QueryExSpawn(ctx, 0x1400, 0x00240AC0, 640, 0x002404F0); }
