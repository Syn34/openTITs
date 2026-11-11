// Game code, medium sweep part 24: point-instance and teleporter-setting
// spawners (Query_Init0CDD28 / Record_Init_000E44E0 users found by caller
// search).
#include "mid24.h"

using namespace sly4;
using namespace sly4::mid24;

namespace {
constexpr u32 kSmallAlloc = 0x0055238Cu;       // SmallAlloc(size)
constexpr u32 kSmallFree = 0x00552420u;        // SmallFree(p, size)
constexpr u32 kPtrVectorClear = 0x0054EC1Cu;   // PtrVector_Clear(v)
constexpr u32 kNop = 0x0054EC18u;              // Nop(...)
constexpr u32 kMemAlloc = 0x0054ED38u;         // Mem_Alloc(size, heap, align)
constexpr u32 kQueryInitCDD28 = 0x000CDD28u;   // Query_Init0CDD28(q, 21 more; f1, f2)
constexpr u32 kRecordInit = 0x000E44E0u;       // Record_Init_000E44E0(q, v, o, ...)
constexpr u32 kStringFormat = 0x005534D0u;     // String_Format(fmt, ...)
constexpr u32 kStringLength = 0x00555FA4u;     // String_Length(s)
constexpr u32 kStringCompareN = 0x00555EB0u;   // String_CompareN(a, b, n)
constexpr u32 kHashNoCase = 0x0056A710u;       // String_HashNoCase(s)
constexpr u32 kHashToString = 0x0056A6E0u;     // Hash_ToString(h)
constexpr u32 kTargetGetDef = 0x000D951Cu;     // Target_GetDef0D951C(target)
constexpr u32 kPointHolderSlot = 0x00F8F154u;  // u32*: the point-instance allocator / holder (+16 PointMap)
constexpr u32 kFromTarget = 0x00AC69D4u;       // "FromTarget"
constexpr u64 kMinus1 = ~0ull;                 // lis -1 / ori: the sign-extended -1 the stack slot gets

// Clears and frees q's lazily allocated 20-byte PtrVector (+0).
void FreeVec0(PPUContext& ctx, u32 q) {
    if (const u32 v = load<u32>(q); v != 0) {
        call(ctx, kPtrVectorClear, {v});
        call(ctx, kNop, {v});
        call(ctx, kSmallFree, {load<u32>(q), 20});
    }
}

// Entity parameter k of o: o->12 is an array of per-parameter words indexed
// by the registered parameter descriptor D's D->16->64.
u32 ParamW(u32 o, u32 D) { return load<u32>(load<u32>(o + 12) + (load<u32>(load<u32>(D + 16) + 64) << 2)); }
u8 ParamB(u32 o, u32 D) { return load<u8>(load<u32>(o + 12) + (load<u32>(load<u32>(D + 16) + 64) << 2)); }
}  // namespace

// void PointInst_SpawnFromTarget0D3EF0(): gets the ApePointInstance list
// allocator *0x00F8F154 (on first use a 60-byte Mem_Alloc(60, 0, 16)
// free list: fields zeroed, u16 +54 bit 0x8000 cleared, vtable 0x00B13900,
// +56 = 0x00AC6621, FreeList_Init(l, 288, slot[1], 0, 0); stored even when
// null) and walks its instances (Obj_GetField4(list) + 16, then each
// x[-4] + 16). For every instance whose x->vfn[12](x) name starts with
// "FromTarget" (String_CompareN against the static String_Length of it,
// computed once under the guard byte 0x010321B8 into 0x010321C0) it builds
// Query_Init0CDD28(q, 1, 0, name, 0, x, vt 0x00AC6790, m, 0x01164B30, 0, 0,
// 1, 1, 0, 1, 1, 0, 0, -1, -, -, 0; f = *0x000D3EEC), where m =
// Mat34_FromRotPos553B4C(0x01164C50, 0x01164B40); q->144 = 0, q->148 =
// -1.0f, q->12 |= 0x200; a 368-byte object from 0x000D3BB4(p, q), p->vfn[8]
// (on null too); q's PtrVector freed. r3 as the last call / load leaves it.
DECOMP(000D3EF0, PointInst_SpawnFromTarget0D3EF0) {
    const u32 slot = load<u32>(kPointHolderSlot);
    u32 al = load<u32>(slot);
    if (al == 0) {
        const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {60, 0, 16}));
        if (p != 0) {
            const u32 n = load<u32>(slot + 4);
            store<u32>(p + 8, 0);
            store<u32>(p + 4, 0);
            store<u32>(p + 12, 0);
            store<u32>(p + 20, 0);
            store<u32>(p + 16, 0);
            store<u32>(p + 24, 0);
            const u16 h = load<u16>(p + 54);
            store<u32>(p + 32, 0);
            store<u32>(p + 28, 0);
            store<u32>(p + 36, 0);
            store<u32>(p + 40, 0);
            store<u32>(p + 44, 0);
            store<u32>(p + 48, 0);
            store<u16>(p + 52, 0);
            store<u16>(p + 54, static_cast<u16>(h & 0x7FFFu));
            store<u32>(p, 0x00B13900u);
            store<u32>(p + 56, 0x00AC6621u);
            call(ctx, 0x00551A58u, {p, 288, n, 0, 0});
            al = p;
        }
        store<u32>(slot, al);
    }
    const u32 f = static_cast<u32>(call(ctx, 0x00556BF0u, {al}));
    u32 x = f != 0 ? f + 16 : 0;
    if (x == 0) return;
    const f64 k = Lf(0x000D3EECu);
    Stack frame(ctx, 240);
    const u32 m = frame.addr(), q = m + 64;
    do {
        if (load<u8>(0x010321B8u) == 0) {
            store<u32>(0x010321C0u, static_cast<u32>(call(ctx, kStringLength, {kFromTarget})));
            store<u8>(0x010321B8u, 1);
        }
        const u32 nm = static_cast<u32>(VCall(ctx, x, 12, {x}));
        const u64 len = static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(0x010321C0u))));
        if (static_cast<u32>(call(ctx, kStringCompareN, {nm, kFromTarget, len})) == 0) {
            const u32 nm2 = static_cast<u32>(VCall(ctx, x, 12, {x}));
            call(ctx, 0x00553B4Cu, {m, 0x01164C50u, 0x01164B40u});
            callf(ctx, kQueryInitCDD28, {q, 1, 0, nm2, 0, x, 0x00AC6790u, m, 0x01164B30u, 0, 0, 1, 1, 0, 1, 1, 0, 0, kMinus1, 0, 0, 0},
                  {k, k});
            const u32 w = load<u32>(q + 12);
            store<u32>(q + 144, 0);
            store<u32>(q + 148, 0xBF800000u);
            store<u32>(q + 12, w | 0x200u);
            u32 r = 0;
            if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {368})); p != 0) {
                call(ctx, 0x000D3BB4u, {p, q});
                r = p;
            }
            VCall(ctx, r, 8, {r});
            FreeVec0(ctx, q);
        }
        const u32 nx = load<u32>(x - 16);
        ctx.gpr[3] = nx;
        x = nx != 0 ? nx + 16 : 0;
    } while (x != 0);
}

// void TeleportSetting_Spawn1EBA20(Entity* o, u32 target): x = the point
// instance PointMap_Find(holder->16, &target) names; none: String_Format(
// "Cannot find the teleporter object instance (%s) to associate with Entity
// Teleporter Setting (%s)", Hash_ToString(target), o->4). Else s =
// Save_Profile0C9FC8(HashNoCase(param 0x010ABDD0)); nothing more when s is
// null or s->28 bit 0x02 is clear. Otherwise Record_Init_000E44E0(q,
// x->vfn[12](x), s, 0, x + 16, 0x01164B30, 1, 0, 0, 0, 0, 0), q->208 =
// param 0x010ABDE8, q->212..220 = HashNoCase(params 0x010ABE00 / 18 / 30),
// u8 q->224 / 225 = params 0x010ABE60 / 78, q->12 |= 2; a 512-byte object
// p from 0x001EB478(p, q); d = Target_GetDef0D951C(p): d->124 bit 0x80000 =
// param 0x010ABE90 bit 0. Unless u8 param 0x010ABE48: p->160->vfn[40](c, 0,
// 0) when set, and for d = Target_GetDef0D951C(p): d->vfn[44](d, 0),
// Obj_EffectToggle_002B81FC(d, 0). q's vectors are freed. r3 as the last
// call leaves it (0 on the early s->28 exit).
DECOMP(001EBA20, TeleportSetting_Spawn1EBA20) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    Stack frame(ctx, 256);
    const u32 q = frame.addr(), key = q + 240;
    store<u32>(key, t);
    const u32 e = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(kPointHolderSlot) + 16), key}));
    const u32 x = load<u32>(e);
    if (x == 0) {
        const u32 h = static_cast<u32>(call(ctx, kHashToString, {t}));
        call(ctx, kStringFormat, {0x00AD1F90u, h, load<u32>(o + 4)});
        return;
    }
    const u32 hs = static_cast<u32>(call(ctx, kHashNoCase, {ParamW(o, 0x010ABDD0u)}));
    const u32 s = static_cast<u32>(call(ctx, 0x000C9FC8u, {hs}));
    if (s == 0) return;
    if ((load<u32>(s + 28) & 2u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 nm = static_cast<u32>(VCall(ctx, x, 12, {x}));
    call(ctx, kRecordInit, {q, nm, s, 0, x + 16, 0x01164B30u, 1, 0, 0, 0, 0, 0});
    const u32 w = load<u32>(q + 12);
    store<u32>(q + 212, 0);
    store<u32>(q + 216, 0);
    store<u32>(q + 220, 0);
    store<u8>(q + 224, 0);
    store<u32>(q + 12, w | 2u);
    store<u8>(q + 225, 0);
    store<u32>(q + 208, ParamW(o, 0x010ABDE8u));
    store<u32>(q + 212, static_cast<u32>(call(ctx, kHashNoCase, {ParamW(o, 0x010ABE00u)})));
    store<u32>(q + 216, static_cast<u32>(call(ctx, kHashNoCase, {ParamW(o, 0x010ABE18u)})));
    store<u32>(q + 220, static_cast<u32>(call(ctx, kHashNoCase, {ParamW(o, 0x010ABE30u)})));
    store<u8>(q + 224, ParamB(o, 0x010ABE60u));
    store<u8>(q + 225, ParamB(o, 0x010ABE78u));
    u32 p = 0;
    if (const u32 a = static_cast<u32>(call(ctx, kSmallAlloc, {512})); a != 0) {
        call(ctx, 0x001EB478u, {a, q});
        p = a;
    }
    if (const u32 d = static_cast<u32>(call(ctx, kTargetGetDef, {p})); d != 0) {
        const u32 b = ParamB(o, 0x010ABE90u);
        store<u32>(d + 124, (load<u32>(d + 124) & ~0x00080000u) | ((b & 1u) << 19));
    }
    if (ParamB(o, 0x010ABE48u) == 0) {
        if (const u32 c = load<u32>(p + 160); c != 0) VCall(ctx, c, 40, {c, 0, 0});
        if (const u32 d = static_cast<u32>(call(ctx, kTargetGetDef, {p})); d != 0) {
            VCall(ctx, d, 44, {d, 0});
            call(ctx, 0x002B81FCu, {d, 0});
        }
    }
    call(ctx, kPtrVectorClear, {q + 164});
    call(ctx, kNop, {q + 164});
    call(ctx, kPtrVectorClear, {q + 144});
    call(ctx, kNop, {q + 144});
    FreeVec0(ctx, q);
}
