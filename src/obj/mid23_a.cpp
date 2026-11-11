// Game code, medium sweep part 23: the target-list sync and part-chain
// factory part 22 left as callees, then more still-recompiled game functions.
#include "mid23.h"

using namespace sly4;
using namespace sly4::mid23;

namespace {
constexpr u32 kPtrVectorReserve = 0x00551718u;    // PtrVector_Reserve(v, n)
constexpr u32 kStringFormat = 0x005534D0u;        // String_Format(fmt, ...)
constexpr u32 kStringCopy = 0x00553684u;          // String_Copy(dst, src, size)
constexpr u32 kSmallAlloc = 0x0055238Cu;          // SmallAlloc(size)
constexpr u32 kSmallFree = 0x00552420u;           // SmallFree(p, size)
constexpr u32 kPtrVectorConstruct = 0x005516E0u;  // PtrVector_Construct(v)
constexpr u32 kPtrVectorClear = 0x0054EC1Cu;      // PtrVector_Clear(v)
constexpr u32 kNop = 0x0054EC18u;                 // Nop(...)
constexpr u32 kVectorAdd = 0x00733964u;           // Vector_Add733964(v, rec192)
constexpr u32 kMemAlloc = 0x0054ED38u;            // Mem_Alloc(size, heap, align)
constexpr u32 kQueryExConstruct = 0x000DFDF8u;    // QueryEx_Construct_000DFDF8(q, 18 more)
constexpr u32 kQueryConstruct = 0x0017EAD4u;      // Query_Construct17EAD4(q, ...; f1..f7)
constexpr u32 kWorld = 0x00F65CA0u;               // World* g_World
constexpr u32 kObjInit22 = 0x000CDFB0u;           // Obj_Init22(q, 21 more; f1, f2)
constexpr u32 kRecordInit = 0x000E44E0u;          // Record_Init_000E44E0(q, v, o, ...)
constexpr u32 kPtrVectorInit = 0x00551598u;       // PtrVector_Init(v, n, a, b)
constexpr u32 kCallK2 = 0x000D94F4u;              // CallK2_000D94F4(o)
constexpr u32 kVec4B40 = 0x01164B40u;             // f32[3] default vector copied into spawn queries

// Clears and frees q's lazily allocated 20-byte PtrVector (+0).
void QueryFreeVec(PPUContext& ctx, u32 q) {
    if (const u32 v = load<u32>(q); v != 0) {
        call(ctx, kPtrVectorClear, {v});
        call(ctx, kNop, {v});
        call(ctx, kSmallFree, {load<u32>(q), 20});
    }
}

// The spawn-query epilogue: q's two record vectors (+164, +144) and its
// lazily allocated 20-byte PtrVector (+0) are cleared and freed.
void QueryFree(PPUContext& ctx, u32 q) {
    call(ctx, kPtrVectorClear, {q + 164});
    call(ctx, kNop, {q + 164});
    call(ctx, kPtrVectorClear, {q + 144});
    call(ctx, kNop, {q + 144});
    QueryFreeVec(ctx, q);
}

// QueryEx_Construct_000DFDF8(q, a, b, v, o, 0, vt, 0x01164BD0, 0x01164B30, s9,
// s10, 0, 1, 0, 1, 0, 0, 0, 0xFFFFFFFF) as the spawners below build it (the
// last one zero-extended).
void QueryExInit(PPUContext& ctx, u32 q, u32 a, u32 b, u32 v, u32 o, u32 vt, u32 s9, u32 s10) {
    call(ctx, kQueryExConstruct, {q, a, b, v, o, 0, vt, 0x01164BD0u, 0x01164B30u, s9, s10, 0, 1, 0, 1, 0, 0, 0, 0xFFFFFFFFu});
}

// Two copies of the default vector 0x01164B40 at a and a+16 (q+208 in the
// QueryEx spawners, q+144 in the Obj_Init22 ones).
void DefVecs(u32 a) {
    for (u32 k = 0; k < 3; ++k) {
        Cw(a + 4 * k, kVec4B40 + 4 * k);
        Cw(a + 16 + 4 * k, kVec4B40 + 4 * k);
    }
}

// Obj_Init22(q, a, b, v, o, 0, vt, 0x01164BD0, 0x01164B30, s9, s10, s11, s12,
// 0, s14, 0, 0, 0, 0xFFFFFFFF, -, -, 0; f1 = f2 = k) as the spawners below
// build it (stack slots 19 / 20 left as they are).
void ObjInit22(PPUContext& ctx, u32 q, u32 a, u32 b, u32 v, u32 o, u32 vt, u32 s9, u32 s10, u32 s11, u32 s12, u32 s14,
               f64 k) {
    callf(ctx, kObjInit22, {q, a, b, v, o, 0, vt, 0x01164BD0u, 0x01164B30u, s9, s10, s11, s12, 0, s14, 0, 0, 0, 0xFFFFFFFFu, 0, 0, 0},
          {k, k});
}

// Record_Init_000E44E0(q, v, o, 0, 0x01164BD0, 0x01164B30, 1, 0, 0, 0, 0, 0).
void RecordInit(PPUContext& ctx, u32 q, u32 v, u32 o) {
    call(ctx, kRecordInit, {q, v, o, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0, 0});
}

// ++World->0x1281D4->4->116->48, returned.
u32 WorldNextId() {
    const u32 y = load<u32>(load<u32>(load<u32>(load<u32>(kWorld) + 0x1281D4u) + 4) + 116);
    const u32 n = load<u32>(y + 48) + 1;
    store<u32>(y + 48, n);
    return n;
}
}  // namespace

// u32 Targets_Sync26FAD4(Query* o, Pair* table, u32 n): o->16 is a vector of
// 8-byte {Obj*, f32} pairs (u16 count +24, capacity +20, high-water +26) whose
// previous size is o->36; self = o->4->8. Every old entry missing from the
// new table (by Obj*) that is non-null gets x->vfn[312](x, self, o->8); then
// every new entry missing from the old list gets x->vfn[308](x, self, o->8)
// (no null check). The vector is then refilled with the first o->12->0
// table entries (bounds re-read each pass; grown with PtrVector_Reserve(v,
// 2 * capacity) when full), +26 = max(+26, count), and o->36 = n. r3 = the
// final count as the original leaves it; f1 = the last copied f32.
DECOMP(0026FAD4, Targets_Sync26FAD4) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 2);
    u32 tab = arg<u32>(ctx, 1);
    const u32 self = load<u32>(load<u32>(o + 4) + 8);
    u32 cur = load<u32>(o + 36);
    if (cur != 0) {
        u32 i = 0;
        do {
            u32 j = 0;
            while (j < n && load<u32>(load<u32>(o + 16) + 8 * i) != load<u32>(tab + 8 * j)) ++j;
            if (j >= n) {
                const u32 x = load<u32>(load<u32>(o + 16) + 8 * i);
                if (x != 0) {
                    VCall(ctx, x, 312, {x, self, load<u32>(o + 8)});
                    cur = load<u32>(o + 36);
                }
            }
            ++i;
        } while (i < cur);
    }
    if (n != 0) {
        u32 i = 0;
        do {
            u32 j = 0;
            while (j < cur && load<u32>(load<u32>(o + 16) + 8 * j) != load<u32>(tab + 8 * i)) ++j;
            if (j >= cur) {
                const u32 x = load<u32>(tab + 8 * i);  // quirk: not null-checked
                VCall(ctx, x, 308, {x, self, load<u32>(o + 8)});
                cur = load<u32>(o + 36);
            }
            ++i;
        } while (i < n);
    }
    store<u16>(o + 24, 0);
    u32 r3 = 0;
    if (load<u32>(load<u32>(o + 12)) != 0) {
        u32 i = 0, cnt = 0;
        u32 lim;
        do {
            if ((r3 & 0xFFFFu) == load<u16>(o + 20)) {
                call(ctx, kPtrVectorReserve, {o + 16, (static_cast<u32>(load<u16>(o + 20)) << 1) & 0x1FFFEu});
                cnt = load<u16>(o + 24);
            }
            const u32 dst = load<u32>(o + 16) + ((cnt << 3) & 0x7FFF8u);
            Cw(dst, tab);
            Cw(dst + 4, tab + 4);
            ctx.fpr[1] = Lf(tab + 4);
            const u32 c1 = static_cast<u32>(load<u16>(o + 24)) + 1;
            const u32 hi = load<u16>(o + 26);
            store<u16>(o + 24, static_cast<u16>(c1));
            lim = load<u32>(load<u32>(o + 12));
            u32 m;
            if (static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFFu)) {
                m = hi;
                r3 = c1 & 0xFFFFu;
            } else {
                m = c1;
                r3 = c1;
            }
            store<u16>(o + 26, static_cast<u16>(m));
            cnt = c1;
            ++i;
            tab += 8;
        } while (i < lim);
    }
    store<u32>(o + 36, n);
    ctx.gpr[3] = r3;
}

// Chain* Part_CreateChain1EFDE8(Obj* o, Src* src, b, f32 rows[4][4], x, y,
// f32 v1[3], f32 v2[3]): fills a chain-creation record on the stack (sp+112,
// vtable 0x00B162B0 at the call, base 0x00B12260): name String_Format(
// 0x00AD2050, o->vfn[76](o)) copied into +252 (40 bytes), the four rows' xyz
// at +144.., the vector 0x01164B30 at +208, y +224, src +232, src->36 / ->40
// (or *0x001EFDE0) at +236 / +240 (+236 then replaced by *0x01021C84 *
// *0x001EFDE4), flags word +244 (bit 31 set, bit 29 = src->32 bit 0x04,
// 0x1FE00000 set, the low bits as the uninitialised stack word holds them),
// the 192-byte records of src->136 (u16 count +8) collected into a lazily
// allocated 20-byte vector at +128 (element size 192), v1 +304, v2 +320, x
// +336, b +340, u8 +344 = bit 0x40000 of o->980, bit 0x1000 of +120. r3 =
// o->804->vfn[28](o->804, &record); the vector is then cleared and freed.
DECOMP(001EFDE8, Part_CreateChain1EFDE8) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), rows = arg<u32>(ctx, 3);
    const u32 x = arg<u32>(ctx, 4), y = arg<u32>(ctx, 5), v1 = arg<u32>(ctx, 6), v2 = arg<u32>(ctx, 7);
    Stack frame(ctx, 464);
    const u32 sp = frame.addr();
    const u32 name = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 str = static_cast<u32>(call(ctx, kStringFormat, {0x00AD2050u, name}));
    const u32 flag = (load<u32>(o + 980) >> 18) & 1u;
    store<u32>(sp + 112, 0x00B12260u);
    store<u32>(sp + 128, 0);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cw(sp + 144 + 16 * r + 4 * k, rows + 16 * r + 4 * k);
    for (u32 k = 0; k < 3; ++k) Cw(sp + 208 + 4 * k, 0x01164B30u + 4 * k);
    store<u32>(sp + 224, y);
    store<u32>(sp + 228, 0);
    store<u32>(sp + 232, src);
    const u32 dflt = load<u32>(0x001EFDE0u);
    store<u32>(sp + 236, src != 0 ? load<u32>(src + 36) : dflt);
    store<u32>(sp + 240, src != 0 ? load<u32>(src + 40) : dflt);
    // quirk: the flags word is merged into whatever the stack held there
    const u32 bit = src != 0 ? (load<u8>(src + 32) >> 2) & 1u : 0u;
    store<u32>(sp + 244, (load<u32>(sp + 244) & 0x1FFFFFFFu) | 0x80000000u | (bit << 29) | 0x1FE00000u);
    store<u32>(sp + 248, 0);
    store<u32>(sp + 116, 0);
    store<u32>(sp + 120, 0);
    store<u32>(sp + 124, 0);
    store<u8>(sp + 252, 0);
    if (str != 0) call(ctx, kStringCopy, {sp + 252, str, 40});
    if (src != 0) {
        u32 v = load<u32>(src + 136);
        if (v != 0 && load<u16>(v + 8) != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 192) {
                const u32 lim = v != 0 ? load<u16>(v + 8) : 0u;
                if (!(i < lim)) break;
                const u32 elem = load<u32>(v) + off;
                u32 vec = load<u32>(sp + 128);
                if (vec == 0) {
                    vec = static_cast<u32>(call(ctx, kSmallAlloc, {20}));
                    store<u32>(sp + 128, vec);
                    if (vec != 0) {
                        call(ctx, kPtrVectorConstruct, {vec});
                        store<u16>(vec + 6, 192);
                        vec = load<u32>(sp + 128);
                    }
                }
                call(ctx, kVectorAdd, {vec, elem});  // quirk: a failed allocation passes 0
                v = load<u32>(src + 136);
            }
        }
    }
    store<u32>(sp + 112, 0x00B162B0u);
    for (u32 k = 0; k < 3; ++k) {
        Cw(sp + 304 + 4 * k, v1 + 4 * k);
        Cw(sp + 320 + 4 * k, v2 + 4 * k);
    }
    store<u32>(sp + 336, x);
    store<u32>(sp + 340, b);
    store<u8>(sp + 344, static_cast<u8>(flag));
    store<u32>(sp + 120, load<u32>(sp + 120) | 0x1000u);
    ctx.fpr[1] = S(Lf(0x01021C84u) * Lf(0x001EFDE4u));
    Sf(sp + 236, ctx.fpr[1]);
    const u32 c = load<u32>(o + 804);
    const u32 r = static_cast<u32>(VCall(ctx, c, 28, {c, sp + 112}));
    const u32 vec = load<u32>(sp + 128);
    if (vec != 0) {
        store<u32>(sp + 112, 0x00B12260u);
        call(ctx, kPtrVectorClear, {vec});
        call(ctx, kNop, {vec});
        call(ctx, kSmallFree, {load<u32>(sp + 128), 20});
    }
    ctx.gpr[3] = r;
}

// ---------------------------------------------------------------- spawners
// Obj* f(Owner* o, Obj* t) in each: v = o->vfn[32](o, t); a query q on the
// stack is built from v and o, filled, handed to t->vfn[8](t, q); an object
// p is allocated and constructed from q, then p->vfn[8](p) (on null too
// unless noted, as the original); QueryFree(q); r3 = p.

// Spawner17ED58: Query_Construct17EAD4(q, v, o, 0, 0x01164BD0, 0x01164B30,
// 0x01164B40, 0x01164B40; f1..f7 = K0, K8, K4, K4, K8, K8, K8 of the floats
// 0x0017ED48; stack slot 15 = 0, 17..21 = 0, 22 = 1, 23..25 = 0, 26 =
// 0xFFFFFFFF, the rest left as they are), a 1728-byte SmallAlloc object
// constructed by 0x0017DD90(p, q).
DECOMP(0017ED58, Spawner17ED58) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 336);
    const u32 q = frame.addr();
    constexpr u32 K = 0x0017ED48u;
    const f64 k0 = Lf(K), k4 = Lf(K + 4), k8 = Lf(K + 8);
    callf(ctx, kQueryConstruct,
          {q, v, o, 0, 0x01164BD0u, 0x01164B30u, kVec4B40, kVec4B40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0xFFFFFFFFu},
          {k0, k8, k4, k4, k8, k8, k8});
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {1728})); p != 0) {
        call(ctx, 0x0017DD90u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1C984: QueryExInit(q, 4, 128, v, o, vt 0x00B0DBAC, 1, 0), q->4 |=
// 0x18, a 1360-byte Mem_Alloc(1360, 0, 16) object: Thunk_00121BB8(p, q),
// vtable 0x00B0DE40.
DECOMP(0001C984, Spawner1C984) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 240);
    const u32 q = frame.addr();
    QueryExInit(ctx, q, 4, 128, v, o, 0x00B0DBACu, 1, 0);
    store<u32>(q + 4, load<u32>(q + 4) | 0x18u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {1360, 0, 16})); p != 0) {
        call(ctx, 0x00121BB8u, {p, q});
        store<u32>(p, 0x00B0DE40u);
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner16C9EC: QueryExInit(q, 0, 0, v, o, vt 0x00B145D0, 1, 1), q->200 =
// 0, q->4 |= 0x400, an 800-byte object from 0x0016BF00(p, q); then
// Obj_PlayCue_002971BC(CallK2_000D94F4(p), 0).
DECOMP(0016C9EC, Spawner16C9EC) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 240);
    const u32 q = frame.addr();
    QueryExInit(ctx, q, 0, 0, v, o, 0x00B145D0u, 1, 1);
    store<u32>(q + 200, 0);
    store<u32>(q + 4, load<u32>(q + 4) | 0x400u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {800})); p != 0) {
        call(ctx, 0x0016BF00u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    const u32 c = static_cast<u32>(call(ctx, 0x000D94F4u, {r}));
    call(ctx, 0x002971BCu, {c, 0});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner204660: QueryExInit(q, 4, 32, v, o, vt 0x00B19188, 1, 0),
// DefVecs(q+208), q->4 |= 0x1000, q->240 = q->244 = 0, u8 q->248..250 = 0, a
// 464-byte object from 0x002040A0(p, q).
DECOMP(00204660, Spawner204660) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 256);
    const u32 q = frame.addr();
    QueryExInit(ctx, q, 4, 32, v, o, 0x00B19188u, 1, 0);
    DefVecs(q + 208);
    ctx.fpr[1] = Lf(kVec4B40 + 8);  // f1 as the copies leave it
    store<u32>(q + 4, load<u32>(q + 4) | 0x1000u);
    store<u32>(q + 240, 0);
    store<u32>(q + 244, 0);
    store<u8>(q + 248, 0);
    store<u8>(q + 249, 0);
    store<u8>(q + 250, 0);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {464})); p != 0) {
        call(ctx, 0x002040A0u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner247EE4: QueryExInit(q, 4, 128, v, o, vt 0x00B1B4F0, o->728 bit 31,
// 0), q->200 = 0, q->12 |= 0x2400; after t->vfn[8] the world id
// (WorldNextId) goes to q->200 and 0x00245DD0(o, id); a 1440-byte object
// from 0x00247AB8(p, q), p->vfn[8](p) only when allocated.
DECOMP(00247EE4, Spawner247EE4) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 240);
    const u32 q = frame.addr();
    QueryExInit(ctx, q, 4, 128, v, o, 0x00B1B4F0u, load<u32>(o + 728) >> 31, 0);
    store<u32>(q + 200, 0);
    store<u32>(q + 12, load<u32>(q + 12) | 0x2400u);
    VCall(ctx, t, 8, {t, q});
    const u32 n = WorldNextId();
    store<u32>(q + 200, n);
    call(ctx, 0x00245DD0u, {o, n});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {1440})); p != 0) {
        call(ctx, 0x00247AB8u, {p, q});
        r = p;
        VCall(ctx, r, 8, {r});
    }
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner167D18: QueryExInit(q, 0, 0, v, o, vt 0x00AC8C70, 0, 0), q->200 =
// 0, q->4 |= 0x80, DefVecs(q+208), q->240 = WorldNextId(), a 480-byte object
// from 0x001676F8(p, q).
DECOMP(00167D18, Spawner167D18) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 256);
    const u32 q = frame.addr();
    QueryExInit(ctx, q, 0, 0, v, o, 0x00AC8C70u, 0, 0);
    store<u32>(q + 200, 0);
    store<u32>(q + 4, load<u32>(q + 4) | 0x80u);
    DefVecs(q + 208);
    ctx.fpr[1] = Lf(kVec4B40);  // f1 as the copies leave it
    store<u32>(q + 240, 0);
    store<u32>(q + 240, WorldNextId());
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {480})); p != 0) {
        call(ctx, 0x001676F8u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1909CC: QueryExInit(q, 5, 32, v, o, vt 0x00AC8C70, 0, 0), q->200 =
// 0, q->4 |= 0x200, DefVecs(q+208), then the parameters +240.. (f32 1, 10, 1,
// 40, u8 0, 0, 0, 0, f32 1, 0, 0, 5, u8 1, 0), a 704-byte object from
// 0x00190158(p, q).
DECOMP(001909CC, Spawner1909CC) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 320);
    const u32 q = frame.addr();
    QueryExInit(ctx, q, 5, 32, v, o, 0x00AC8C70u, 0, 0);
    store<u32>(q + 200, 0);
    store<u32>(q + 4, load<u32>(q + 4) | 0x200u);
    DefVecs(q + 208);
    ctx.fpr[1] = Lf(kVec4B40);  // f1 as the copies leave it
    store<u32>(q + 292, 0);
    store<u8>(q + 288, 1);
    store<u32>(q + 284, 0x40A00000u);
    store<u32>(q + 280, 0);
    store<u32>(q + 276, 0);
    store<u32>(q + 272, 0x3F800000u);
    store<u32>(q + 268, 0);
    store<u32>(q + 264, 0);
    store<u32>(q + 260, 0);
    store<u8>(q + 256, 0);
    store<u32>(q + 252, 0x42200000u);
    store<u32>(q + 248, 0x3F800000u);
    store<u32>(q + 244, 0x41200000u);
    store<u32>(q + 240, 0x3F800000u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {704})); p != 0) {
        call(ctx, 0x00190158u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1FA8C4: QueryExInit(q, 0, 0, v, o, vt 0x00AD2B74, u8 o->301 bit
// 0x80, 0), DefVecs(q+208), q->4 |= 0x20, q->240 = 0, +244 = -1.0f, +248 =
// FLT_MAX, u8 +252 = 0; a 448-byte PhysObj_Ctor_001FA238(p, q) with the
// vtables of 0x00B18D28 (+8 -> p+0, +416 -> p+380, +456 -> p+384).
DECOMP(001FA8C4, Spawner1FA8C4) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 256);
    const u32 q = frame.addr();
    QueryExInit(ctx, q, 0, 0, v, o, 0x00AD2B74u, (load<u8>(o + 301) >> 7) & 1u, 0);
    DefVecs(q + 208);
    ctx.fpr[1] = Lf(kVec4B40 + 8);  // f1 as the copies leave it
    store<u32>(q + 4, load<u32>(q + 4) | 0x20u);
    store<u32>(q + 240, 0);
    store<u32>(q + 244, 0xBF800000u);
    store<u32>(q + 248, 0x7F7FFFFFu);
    store<u8>(q + 252, 0);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {448})); p != 0) {
        call(ctx, 0x001FA238u, {p, q});
        store<u32>(p, 0x00B18D30u);
        store<u32>(p + 380, 0x00B18EC8u);
        store<u32>(p + 384, 0x00B18EF0u);
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1FF16C: QueryExInit(q, 0, 0, v, o, vt 0x00AD2B74, u8 o->301 bit
// 0x80, 0), DefVecs(q+208), q->240 = 0, +244 = -1.0f, +248 = FLT_MAX, the
// word +252: top byte 0, bit 0x00800000 = bit 0x20000000 of o->452 (0 for a
// null o), the rest as the stack held it; q->4 |= 0x60. After t->vfn[8]
// q+224 is copied again; a 608-byte PhysObj_Ctor_001FEF38(p, q).
DECOMP(001FF16C, Spawner1FF16C) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 256);
    const u32 q = frame.addr();
    QueryExInit(ctx, q, 0, 0, v, o, 0x00AD2B74u, (load<u8>(o + 301) >> 7) & 1u, 0);
    DefVecs(q + 208);
    store<u32>(q + 240, 0);
    store<u32>(q + 244, 0xBF800000u);
    store<u32>(q + 248, 0x7F7FFFFFu);
    const u32 old = load<u32>(q + 252);  // quirk: merged into the uninitialised word
    const u32 b = o != 0 ? (load<u32>(o + 452) >> 29) & 1u : 0u;
    store<u32>(q + 252, (old & 0x007FFFFFu) | (b << 23));
    store<u32>(q + 4, load<u32>(q + 4) | 0x60u);
    VCall(ctx, t, 8, {t, q});
    for (u32 k = 0; k < 3; ++k) Cw(q + 224 + 4 * k, kVec4B40 + 4 * k);
    ctx.fpr[1] = Lf(kVec4B40);  // f1 as the copies leave it
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {608})); p != 0) {
        call(ctx, 0x001FEF38u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// SpawnerC0FE8: QueryExInit-like QueryEx_Construct(q, 4, 128, v, o, 0,
// 0x00B12070, ..., u8 o->260, u8 o->261 in slots 9 / 10), DefVecs(q+208),
// q->240..248 = o->244..252 (K = *0x000C0FE0 for a null o), q->252 = o->256
// (or *0x000C0FE4), q->4 |= 0x00200000; a 448-byte object from 0x000B2E18.
DECOMP(000C0FE8, SpawnerC0FE8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 272);
    const u32 q = frame.addr();
    const u32 a = o != 0 ? load<u8>(o + 260) : 0u, b = o != 0 ? load<u8>(o + 261) : 0u;
    call(ctx, kQueryExConstruct, {q, 4, 128, v, o, 0, 0x00B12070u, 0x01164BD0u, 0x01164B30u, a, b, 0, 1, 0, 1, 0, 0, 0, 0xFFFFFFFFu});
    DefVecs(q + 208);
    constexpr u32 K = 0x000C0FE0u;
    for (u32 k = 0; k < 3; ++k) Cw(q + 240 + 4 * k, o != 0 ? o + 244 + 4 * k : K);
    Cw(q + 252, o != 0 ? o + 256 : K + 4);
    ctx.fpr[1] = Lf(o != 0 ? o + 256 : K + 4);
    store<u32>(q + 4, load<u32>(q + 4) | 0x00200000u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {448})); p != 0) {
        call(ctx, 0x000B2E18u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner177AD0: QueryExInit(q, 0, 0, v, o, vt 0x00B14958, u8 o->256 bit
// 0x04, 0), DefVecs(q+208), q->4 |= 0x800, q->240 = q->244 = 0, +248 =
// -1.0f, +252 = FLT_MAX; a Mem_Alloc(448, 0, 16) object from 0x001774D8;
// when CallK2_000D94F4(p) is non-null, CallK2_000D94F4(p)->1240 =
// p->vfn[100](p).
DECOMP(00177AD0, Spawner177AD0) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 272);
    const u32 q = frame.addr();
    QueryExInit(ctx, q, 0, 0, v, o, 0x00B14958u, (load<u8>(o + 256) >> 2) & 1u, 0);
    DefVecs(q + 208);
    ctx.fpr[1] = Lf(kVec4B40 + 8);  // f1 as the copies leave it
    store<u32>(q + 4, load<u32>(q + 4) | 0x800u);
    store<u32>(q + 240, 0);
    store<u32>(q + 244, 0);
    store<u32>(q + 248, 0xBF800000u);
    store<u32>(q + 252, 0x7F7FFFFFu);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {448, 0, 16})); p != 0) {
        call(ctx, 0x001774D8u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    if (call(ctx, kCallK2, {r}) != 0) {
        const u32 c = static_cast<u32>(call(ctx, kCallK2, {r}));
        store<u32>(c + 1240, static_cast<u32>(VCall(ctx, r, 100, {r})));
    }
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1EC5C8: RecordInit(q, v, o), q->12 |= 8, the f32 pair 0x01164AFC
// twice at q+208, q+224..240 = 0, q+244 = o->420, q+248..272 = 0, u8
// q+276..283 = 0, 1, 0, 1, 1, 0, 0, 0; a 992-byte object from 0x001EBDF0.
DECOMP(001EC5C8, Spawner1EC5C8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 288);
    const u32 q = frame.addr();
    RecordInit(ctx, q, v, o);
    constexpr u32 K = 0x01164AFCu;
    Cw(q + 208, K);
    Cw(q + 212, K + 4);
    Cw(q + 216, K);
    Cw(q + 220, K + 4);
    ctx.fpr[1] = Lf(K);
    for (u32 k = 0; k < 5; ++k) store<u32>(q + 224 + 4 * k, 0);
    store<u32>(q + 244, load<u32>(o + 420));
    for (u32 k = 0; k < 7; ++k) store<u32>(q + 248 + 4 * k, 0);
    static constexpr u8 kBytes[8] = {0, 1, 0, 1, 1, 0, 0, 0};
    for (u32 k = 0; k < 8; ++k) store<u8>(q + 276 + k, kBytes[k]);
    store<u32>(q + 12, load<u32>(q + 12) | 8u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {992})); p != 0) {
        call(ctx, 0x001EBDF0u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1BADD8: ObjInit22(q, 0, 0, v, o, vt 0x00B13880, 0, 0, 1, 1, 0; f =
// *0x001BADD0), DefVecs(q+144), q->8 |= 0x1000, q->176 = 0, +180 = -1, u8
// +184..188 = 0, q->20 |= 0x80; a 416-byte object from 0x001B9EB8; only
// q's PtrVector is freed.
DECOMP(001BADD8, Spawner1BADD8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 192);
    const u32 q = frame.addr();
    ObjInit22(ctx, q, 0, 0, v, o, 0x00B13880u, 0, 0, 1, 1, 0, Lf(0x001BADD0u));
    DefVecs(q + 144);
    ctx.fpr[1] = Lf(kVec4B40 + 8);  // f1 as the copies leave it
    store<u32>(q + 8, load<u32>(q + 8) | 0x1000u);
    store<u32>(q + 176, 0);
    store<u32>(q + 180, 0xFFFFFFFFu);
    for (u32 k = 0; k < 5; ++k) store<u8>(q + 184 + k, 0);
    store<u32>(q + 20, load<u32>(q + 20) | 0x80u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {416})); p != 0) {
        call(ctx, 0x001B9EB8u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFreeVec(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner224720: r3 = 0 when t->4 bit 0x00400000, u8 t->184 bit 0x04 and
// not t->8 bit 0x02; else ObjInit22(q, 1, 0, v, o, vt 0x00B12000, 0, 0, 1,
// 0, 1; f = *0x00224718), q->144 = 0, u8 q->148 = 1, q->149 = 0, q->150..153
// = u8 o->172..175 (0 for a null o), q->4 |= 0x00400000, q->156..176 = 0,
// q->8 |= 2; a 448-byte LightObj_Ctor_00224448(p, q); only q's PtrVector is
// freed.
DECOMP(00224720, Spawner224720) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    if ((load<u32>(t + 4) & 0x00400000u) != 0 && ((load<u8>(t + 184) >> 2) & 1u) != 0 && (load<u32>(t + 8) & 2u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 208);
    const u32 q = frame.addr();
    ctx.fpr[1] = Lf(0x00224718u);
    ObjInit22(ctx, q, 1, 0, v, o, 0x00B12000u, 0, 0, 1, 0, 1, ctx.fpr[1]);
    store<u32>(q + 144, 0);
    store<u8>(q + 148, 1);
    store<u8>(q + 149, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(q + 150 + k, o != 0 ? load<u8>(o + 172 + k) : 0);
    store<u32>(q + 4, load<u32>(q + 4) | 0x00400000u);
    for (u32 k = 0; k < 6; ++k) store<u32>(q + 156 + 4 * k, 0);
    store<u32>(q + 8, load<u32>(q + 8) | 2u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {448})); p != 0) {
        call(ctx, 0x00224448u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFreeVec(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1A51E8: ObjInit22(q, 4, 128, v, o, vt 0x00B15018, 0, 0, 0, 1, 0; f
// = *0x001A51E0), a 12-byte-element PtrVector at q+144 (PtrVector_Init(v,
// 16, 0, 1)) filled with the u16 o->148 vec3s of o->140 (grown with
// PtrVector_Reserve(v, 2 * capacity) when full; high-water +154), q->8 |=
// 0x400; a 384-byte object from 0x001A4680; then the vector and q's
// PtrVector are freed.
DECOMP(001A51E8, Spawner1A51E8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 176);
    const u32 q = frame.addr();
    ctx.fpr[1] = Lf(0x001A51E0u);
    ObjInit22(ctx, q, 4, 128, v, o, 0x00B15018u, 0, 0, 0, 1, 0, ctx.fpr[1]);
    const u32 vec = q + 144;
    call(ctx, kPtrVectorConstruct, {vec});
    store<u16>(vec + 6, 12);
    call(ctx, kPtrVectorInit, {vec, 16, 0, 1});
    store<u32>(q + 8, load<u32>(q + 8) | 0x400u);
    u32 cnt = load<u16>(vec + 8);
    for (u32 i = 0, off = 0; i < load<u16>(o + 148); ++i, off += 12) {
        const u32 src = load<u32>(o + 140) + off;
        if ((cnt & 0xFFFFu) == load<u16>(vec + 4)) {
            call(ctx, kPtrVectorReserve, {vec, (static_cast<u32>(load<u16>(vec + 4)) << 1) & 0x1FFFEu});
            cnt = load<u16>(vec + 8);
        }
        const u32 dst = load<u32>(vec) + 12 * (cnt & 0xFFFFu);
        Cw(dst, src);
        Cw(dst + 4, src + 4);
        Cw(dst + 8, src + 8);
        ctx.fpr[1] = Lf(src + 8);
        const u32 c1 = static_cast<u32>(load<u16>(vec + 8)) + 1;
        const u32 hi = load<u16>(vec + 10);
        store<u16>(vec + 8, static_cast<u16>(c1));
        if (static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFFu)) {
            store<u16>(vec + 10, static_cast<u16>(hi));
            cnt = c1 & 0xFFFFu;
        } else {
            store<u16>(vec + 10, static_cast<u16>(c1));
            cnt = c1;
        }
    }
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {384})); p != 0) {
        call(ctx, 0x001A4680u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    call(ctx, kPtrVectorClear, {vec});
    call(ctx, kNop, {vec});
    QueryFreeVec(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1BEFE8: RecordInit(q, v, o), q->236 = 0, q->8 |= 0x00080000, the
// seven f32 o->308 + 8k at q+208.. (*0x001BEFDC for a null o), q->240 =
// o->400 (0), q->244 = o->404 (*0x001BEFE0), u8 q->248 / 249 = o->408 / 409
// (0), q->8 |= 0x00100000; a 576-byte Owner_Construct1ADD08(p, q) with the
// vtables of 0x00B16670 (+8 -> p+0, +476 -> p+380, +516 -> p+384), p->560 =
// -1, p->556 = q->240, p->568 = 0, p->564 = q->244, flags u8 p->572: 0x80 =
// q->248 bit 0, 0x40 / 0x20 clear, then 0x10 = q->249 bit 0, 0x08 clear.
DECOMP(001BEFE8, Spawner1BEFE8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 256);
    const u32 q = frame.addr();
    RecordInit(ctx, q, v, o);
    constexpr u32 K = 0x001BEFDCu;
    store<u32>(q + 236, 0);
    store<u32>(q + 8, load<u32>(q + 8) | 0x00080000u);
    for (u32 k = 0; k < 7; ++k) Cw(q + 208 + 4 * k, o != 0 ? o + 308 + 8 * k : K);
    store<u32>(q + 240, o != 0 ? load<u32>(o + 400) : 0u);
    Cw(q + 244, o != 0 ? o + 404 : K + 4);
    ctx.fpr[1] = Lf(q + 244);
    store<u8>(q + 248, o != 0 ? load<u8>(o + 408) : 0);
    store<u8>(q + 249, o != 0 ? load<u8>(o + 409) : 0);
    store<u32>(q + 8, load<u32>(q + 8) | 0x00100000u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {576})); p != 0) {
        call(ctx, 0x001ADD08u, {p, q});
        store<u32>(p, 0x00B16678u);
        store<u32>(p + 380, 0x00B1684Cu);
        store<u32>(p + 384, 0x00B16874u);
        store<u32>(p + 560, 0xFFFFFFFFu);
        store<u32>(p + 556, load<u32>(q + 240));
        store<u32>(p + 568, 0);
        Cw(p + 564, q + 244);
        u8 f = load<u8>(p + 572);
        f = static_cast<u8>((f & ~0xE0u) | ((load<u8>(q + 248) & 1u) << 7));
        store<u8>(p + 572, f);
        f = static_cast<u8>((f & ~0x18u) | ((load<u8>(q + 249) & 1u) << 4));
        store<u8>(p + 572, f);
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// SpawnerDE208: Query_Init0CDD28(q, 0, 0, v, o, 0, 0x00AC6790, 0x01164BD0,
// 0x01164B30, o->232 bit 30, o->232 bit 29, 0, 1, 0, 0, 0, 0, 0, 0xFFFFFFFF,
// -, -, 0; f1 = f2 = *0x000DE204), a 76-byte-element PtrVector at q+144
// (PtrVector_Init(v, 16, 0, 1)), q->164 = o->180 (0 for a null o), q->12 |=
// 0x40; a 384-byte object from 0x000DDCA8; the vector and q's PtrVector
// are freed.
DECOMP(000DE208, SpawnerDE208) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 176);
    const u32 q = frame.addr();
    const u32 w = load<u32>(o + 232);
    ctx.fpr[1] = Lf(0x000DE204u);
    callf(ctx, 0x000CDD28u,
          {q, 0, 0, v, o, 0, 0x00AC6790u, 0x01164BD0u, 0x01164B30u, (w >> 30) & 1u, (w >> 29) & 1u, 0, 1, 0, 0, 0, 0, 0, 0xFFFFFFFFu, 0, 0, 0},
          {ctx.fpr[1], ctx.fpr[1]});
    const u32 vec = q + 144;
    call(ctx, kPtrVectorConstruct, {vec});
    store<u16>(vec + 6, 76);
    call(ctx, kPtrVectorInit, {vec, 16, 0, 1});
    store<u32>(q + 164, o != 0 ? load<u32>(o + 180) : 0u);
    store<u32>(q + 12, load<u32>(q + 12) | 0x40u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {384})); p != 0) {
        call(ctx, 0x000DDCA8u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    call(ctx, kPtrVectorClear, {vec});
    call(ctx, kNop, {vec});
    QueryFreeVec(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1D1B18: RecordInit(q, v, o), q->236 = 0, q->8 |= 0x00080000, the
// seven f32 o->308 + 8k at q+208.. (*0x001D1B10 for a null o), a 28-byte-
// element PtrVector at q+240 (PtrVector_Init(v, 16, 0, 1)), q->260 / 264 =
// 0, q->268 / 272 = -1.0f, q->276..292 = 0, q->296 = -1, the word q->300
// with its two top bits cleared (the rest as the stack held it), q->8 |=
// 0x08000000; a 976-byte object from 0x001D1170; the vector and q's three
// are freed.
DECOMP(001D1B18, Spawner1D1B18) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 320);
    const u32 q = frame.addr();
    RecordInit(ctx, q, v, o);
    constexpr u32 K = 0x001D1B10u;
    store<u32>(q + 236, 0);
    store<u32>(q + 8, load<u32>(q + 8) | 0x00080000u);
    for (u32 k = 0; k < 7; ++k) Cw(q + 208 + 4 * k, o != 0 ? o + 308 + 8 * k : K);
    ctx.fpr[1] = Lf(o != 0 ? o + 356 : K);
    const u32 vec = q + 240;
    call(ctx, kPtrVectorConstruct, {vec});
    store<u16>(vec + 6, 28);
    call(ctx, kPtrVectorInit, {vec, 16, 0, 1});
    store<u32>(q + 260, 0);
    store<u32>(q + 264, 0);
    store<u32>(q + 268, 0xBF800000u);
    store<u32>(q + 272, 0xBF800000u);
    for (u32 k = 0; k < 5; ++k) store<u32>(q + 276 + 4 * k, 0);
    store<u32>(q + 296, 0xFFFFFFFFu);
    store<u32>(q + 300, load<u32>(q + 300) & 0x3FFFFFFFu);  // quirk: uninitialised low bits kept
    store<u32>(q + 8, load<u32>(q + 8) | 0x08000000u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {976})); p != 0) {
        call(ctx, 0x001D1170u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    call(ctx, kPtrVectorClear, {vec});
    call(ctx, kNop, {vec});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1BDAE8: RecordInit(q, v, o), the vector 0x01164B40 at q+208,
// q->224 / 228 = -1.0f, u8 q->232..234 = 0, q->8 |= 0x200, q->20 |=
// 0x10000; a 464-byte Thunk_000E40B4(p, q) object with the vtables of
// 0x00B16430 (+8 -> p+0, +436 -> p+380, +476 -> p+384), p->432..440 =
// q->208..216, p->448 / 452 = q->224 / 228, u8 p->456..458 = q->232..234;
// p->448 = p->216->304 when q->224 < *0x001BDAE4; p->432..440 = p->216->
// 320..328 when p->432..440 still equal the default vector.
DECOMP(001BDAE8, Spawner1BDAE8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 256);
    const u32 q = frame.addr();
    RecordInit(ctx, q, v, o);
    for (u32 k = 0; k < 3; ++k) Cw(q + 208 + 4 * k, kVec4B40 + 4 * k);
    ctx.fpr[1] = Lf(kVec4B40 + 8);
    store<u32>(q + 224, 0xBF800000u);
    store<u32>(q + 228, 0xBF800000u);
    store<u8>(q + 232, 0);
    store<u8>(q + 233, 0);
    store<u8>(q + 234, 0);
    store<u32>(q + 8, load<u32>(q + 8) | 0x200u);
    store<u32>(q + 20, load<u32>(q + 20) | 0x10000u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {464})); p != 0) {
        call(ctx, 0x000E40B4u, {p, q});
        store<u32>(p, 0x00B16438u);
        store<u32>(p + 380, 0x00B165E4u);
        store<u32>(p + 384, 0x00B1660Cu);
        Cw(p + 432, q + 208);
        Cw(p + 436, q + 212);
        Cw(p + 440, q + 216);
        Cw(p + 448, q + 224);
        Cw(p + 452, q + 228);
        store<u8>(p + 456, load<u8>(q + 232));
        store<u8>(p + 457, load<u8>(q + 233));
        store<u8>(p + 458, load<u8>(q + 234));
        ctx.fpr[1] = Lf(q + 224);
        const u32 d = load<u32>(p + 216);
        if (ctx.fpr[1] < Lf(0x001BDAE4u)) {
            Cw(p + 448, d + 304);
            ctx.fpr[1] = Lf(d + 304);
        }
        if (load<u64>(p + 432) == load<u64>(kVec4B40) && load<u32>(p + 440) == load<u32>(kVec4B40 + 8)) {
            Cw(p + 432, d + 320);
            Cw(p + 436, d + 324);
            Cw(p + 440, d + 328);
            ctx.fpr[1] = Lf(d + 328);
        }
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1F5660: RecordInit(q, v, o), q->208 = o->304, q+224 = the vec3
// o+320 (or the equal default 0x01164B40), q+240 = the vec4 o+336 (or the
// equal 0x01164C80), q->256..264 = o->352..360 each replaced by
// *0x001F5658 when it compares equal to it, q->268 = o->364, u8 q->272 = 0,
// q->12 |= 0x10; a 576-byte object from 0x001F54E8.
DECOMP(001F5660, Spawner1F5660) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 304);
    const u32 q = frame.addr();
    RecordInit(ctx, q, v, o);
    store<u32>(q + 208, load<u32>(o + 304));
    u32 s3 = o + 320;
    if (load<u64>(s3) == load<u64>(kVec4B40) && load<u32>(s3 + 8) == load<u32>(kVec4B40 + 8)) s3 = kVec4B40;
    for (u32 k = 0; k < 3; ++k) Cw(q + 224 + 4 * k, s3 + 4 * k);
    constexpr u32 kVec4C80 = 0x01164C80u;
    u32 s4 = o + 336;
    if (load<u64>(s4) == load<u64>(kVec4C80) && load<u64>(s4 + 8) == load<u64>(kVec4C80 + 8)) s4 = kVec4C80;
    for (u32 k = 0; k < 4; ++k) Cw(q + 240 + 4 * k, s4 + 4 * k);
    constexpr u32 K = 0x001F5658u;
    const f64 kf = Lf(K);
    for (u32 k = 0; k < 3; ++k) Cw(q + 256 + 4 * k, Lf(o + 352 + 4 * k) == kf ? K : o + 352 + 4 * k);
    ctx.fpr[1] = Lf(o + 360);
    store<u32>(q + 268, load<u32>(o + 364));
    store<u8>(q + 272, 0);
    store<u32>(q + 12, load<u32>(q + 12) | 0x10u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {576})); p != 0) {
        call(ctx, 0x001F54E8u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner21BD90: r3 = 0 when t->4 bit 0x00400000, u8 t->184 bit 0x04 and
// not t->4 bit 0x10000000; else ObjInit22(q, 1, 0, v, o, vt 0x00B12000, 0,
// 0, 1, 0, 1; f = *0x0021BD84), q->144 = 0, u8 q->148 = 1, q->149 = 0,
// q->150..153 = u8 o->172..175, q->156 = o->176, q->160 = o->188 (-1),
// q->164 = o->192 (*0x0021BD88), the word q->168: bit 31 clear, bit 30 =
// o->196 bit 31, bits 29 / 28 clear, bit 27 = o->196 bit 30, the rest as
// the stack held it; q->4 |= 0x10400000; a 464-byte object from 0x0021BBE4;
// only q's PtrVector is freed. (A null o reads as zeros where noted.)
DECOMP(0021BD90, Spawner21BD90) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 tw = load<u32>(t + 4);
    if ((tw & 0x00400000u) != 0 && ((load<u8>(t + 184) >> 2) & 1u) != 0 && (tw & 0x10000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 192);
    const u32 q = frame.addr();
    constexpr u32 K = 0x0021BD84u;
    ObjInit22(ctx, q, 1, 0, v, o, 0x00B12000u, 0, 0, 1, 0, 1, Lf(K));
    store<u32>(q + 144, 0);
    store<u8>(q + 148, 1);
    store<u8>(q + 149, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(q + 150 + k, o != 0 ? load<u8>(o + 172 + k) : 0);
    store<u32>(q + 156, o != 0 ? load<u32>(o + 176) : 0u);
    store<u32>(q + 160, o != 0 ? load<u32>(o + 188) : 0xFFFFFFFFu);
    Cw(q + 164, o != 0 ? o + 192 : K + 4);
    ctx.fpr[1] = Lf(o != 0 ? o + 192 : K + 4);
    const u32 ow = o != 0 ? load<u32>(o + 196) : 0u;
    // quirk: merged into whatever the stack held there
    store<u32>(q + 168, (load<u32>(q + 168) & 0x07FFFFFFu) | ((ow >> 31) << 30) | (((ow >> 30) & 1u) << 27));
    store<u32>(q + 4, load<u32>(q + 4) | 0x10400000u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {464})); p != 0) {
        call(ctx, 0x0021BBE4u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFreeVec(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1CB448: RecordInit(q, v, o), q->236 = 0, q->8 |= 0x00080000, the
// seven f32 o->308 + 8k at q+208.. (K = 0x001CB43C: K[1] for a null o),
// q->240 = o->436 (0), q->244..268 = o->440..464 (K[0], the last K[2]),
// q->272 = o->480 (-1), u8 q->276 / 277 = u8 o->484 / 485 != 0 (1), q->8 |=
// 0x02000000; an 864-byte object from 0x001CB000.
DECOMP(001CB448, Spawner1CB448) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 288);
    const u32 q = frame.addr();
    RecordInit(ctx, q, v, o);
    constexpr u32 K = 0x001CB43Cu;
    store<u32>(q + 236, 0);
    store<u32>(q + 8, load<u32>(q + 8) | 0x00080000u);
    for (u32 k = 0; k < 7; ++k) Cw(q + 208 + 4 * k, o != 0 ? o + 308 + 8 * k : K + 4);
    store<u32>(q + 240, o != 0 ? load<u32>(o + 436) : 0u);
    for (u32 k = 0; k < 6; ++k) Cw(q + 244 + 4 * k, o != 0 ? o + 440 + 4 * k : K);
    Cw(q + 268, o != 0 ? o + 464 : K + 8);
    ctx.fpr[1] = Lf(o != 0 ? o + 464 : K + 8);
    store<u32>(q + 272, o != 0 ? load<u32>(o + 480) : 0xFFFFFFFFu);
    store<u8>(q + 276, o != 0 ? (load<u8>(o + 484) != 0 ? 1 : 0) : 1);
    store<u8>(q + 277, o != 0 ? (load<u8>(o + 485) != 0 ? 1 : 0) : 1);
    store<u32>(q + 8, load<u32>(q + 8) | 0x02000000u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {864})); p != 0) {
        call(ctx, 0x001CB000u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner1C6528: RecordInit(q, v, o), q->236 = 0, q->8 |= 0x00080000, the
// seven f32 o->308 + 8k at q+208.. (K = 0x001C6520: K[0] for a null o),
// q->240 / 244 = o->428 / 432 (0), q->248 / 252 = o->436 / 440 (K[1]); six
// 80-byte entries at q+256 (+0 = 0, the default vector at +16 and +32, the
// vec4 0x01164C90 at +48, +64 = 0, u8 +68 = 1, +69 = 0); the word q->736:
// bit 31 clear, bit 30 = o->444 bit 22 (0), the rest as the stack held it;
// q->8 |= 0x00400000; a 704-byte object from 0x001C5A80.
DECOMP(001C6528, Spawner1C6528) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 752);
    const u32 q = frame.addr();
    RecordInit(ctx, q, v, o);
    constexpr u32 K = 0x001C6520u, kVec4C90 = 0x01164C90u;
    store<u32>(q + 236, 0);
    store<u32>(q + 8, load<u32>(q + 8) | 0x00080000u);
    for (u32 k = 0; k < 7; ++k) Cw(q + 208 + 4 * k, o != 0 ? o + 308 + 8 * k : K);
    store<u32>(q + 240, o != 0 ? load<u32>(o + 428) : 0u);
    store<u32>(q + 244, o != 0 ? load<u32>(o + 432) : 0u);
    Cw(q + 248, o != 0 ? o + 436 : K + 4);
    Cw(q + 252, o != 0 ? o + 440 : K + 4);
    for (u32 j = 0; j < 6; ++j) {
        const u32 e = q + 256 + 80 * j;
        store<u32>(e, 0);
        DefVecs(e + 16);
        for (u32 k = 0; k < 4; ++k) Cw(e + 48 + 4 * k, kVec4C90 + 4 * k);
        store<u32>(e + 64, 0);
        store<u8>(e + 68, 1);
        store<u8>(e + 69, 0);
    }
    ctx.fpr[1] = Lf(kVec4C90 + 12);
    const u32 b = o != 0 ? (load<u32>(o + 444) >> 22) & 1u : 0u;
    store<u32>(q + 736, (load<u32>(q + 736) & 0x3FFFFFFFu) | (b << 30));  // quirk: uninitialised low bits kept
    store<u32>(q + 8, load<u32>(q + 8) | 0x00400000u);
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {704})); p != 0) {
        call(ctx, 0x001C5A80u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFree(ctx, q);
    ctx.gpr[3] = r;
}

// Spawner21A6E8: r3 = 0 when t->4 bit 0x00400000, u8 t->184 bit 0x04 and
// not t->4 bit 0x08000000; else ObjInit22(q, 1, 0, v, o, vt 0x00B12000, 0,
// 0, 1, 0, 1; f = *0x0021A6E0), q->144 = 0, u8 q->148 = 1, q->149 = 0,
// q->150..153 = u8 o->172..175, q+160 = the vec3 o+176 (0x01164B40),
// q->176 = o->192 (0), q->180..188 = o->196..204 (*0x0021A6E4), u8
// q->192 / 193 / 195 / 196 = u8 o->208 / 209 / 210 / 211 != 0, q->194 = 0,
// q->4 |= 0x08400000; a 480-byte object from 0x0021A518; only q's
// PtrVector is freed. (A null o reads as zeros where noted.)
DECOMP(0021A6E8, Spawner21A6E8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 tw = load<u32>(t + 4);
    if ((tw & 0x00400000u) != 0 && ((load<u8>(t + 184) >> 2) & 1u) != 0 && (tw & 0x08000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 224);
    const u32 q = frame.addr();
    constexpr u32 K = 0x0021A6E0u;
    ObjInit22(ctx, q, 1, 0, v, o, 0x00B12000u, 0, 0, 1, 0, 1, Lf(K));
    store<u32>(q + 144, 0);
    store<u8>(q + 148, 1);
    store<u8>(q + 149, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(q + 150 + k, o != 0 ? load<u8>(o + 172 + k) : 0);
    const u32 w4 = load<u32>(q + 4);
    store<u32>(q + 4, w4 | 0x00400000u);
    const u32 src = o != 0 ? o + 176 : kVec4B40;
    for (u32 k = 0; k < 3; ++k) Cw(q + 160 + 4 * k, src + 4 * k);
    store<u32>(q + 176, o != 0 ? load<u32>(o + 192) : 0u);
    for (u32 k = 0; k < 3; ++k) Cw(q + 180 + 4 * k, o != 0 ? o + 196 + 4 * k : K + 4);
    ctx.fpr[1] = Lf(o != 0 ? o + 200 : K + 4);
    auto nz = [&](u32 off) -> u8 { return o != 0 && load<u8>(o + off) != 0 ? 1 : 0; };
    store<u8>(q + 192, nz(208));
    store<u8>(q + 194, 0);
    store<u8>(q + 193, nz(209));
    store<u8>(q + 195, nz(210));
    store<u32>(q + 4, w4 | 0x08400000u);
    store<u8>(q + 196, nz(211));
    VCall(ctx, t, 8, {t, q});
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {480})); p != 0) {
        call(ctx, 0x0021A518u, {p, q});
        r = p;
    }
    VCall(ctx, r, 8, {r});
    QueryFreeVec(ctx, q);
    ctx.gpr[3] = r;
}
