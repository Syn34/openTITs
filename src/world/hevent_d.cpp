// World_HandleEvent callees, 33..38 instructions: list walks.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
namespace dfn {
constexpr u32 SmallFree = 0x00552420;     // (p, size)
constexpr u32 sub_000CF534 = 0x000CF534;  // (Object*, f32 dt)
constexpr u32 sub_002A9190 = 0x002A9190;  // (Node*, f32 dt)
constexpr u32 sub_000CF0C0 = 0x000CF0C0;  // (Object*, f32 dt)
constexpr u32 sub_004BB4B4 = 0x004BB4B4;  // (a, b) -> Object* (name?)
}  // namespace dfn

constexpr u32 kRefListHead = 0x00F65B7C;   // u32 first RefNode (24 bytes: +4 next, +8 prev, +20 refs) (name?)
constexpr u32 kNodeList = 0x00F64A58;      // u32 first node (+2568 next, +72 two Object* 24 apart) (name?)
constexpr u32 kNodeNext = 2568;
}  // namespace

// void Obj_CollectChain(Object* o): walks the chain from *(o->+12) through
// +12 links; every node but the last is appended to the u32 array at +16
// (u16 count at +96), the last goes to +100 (0 for an empty chain).
DECOMP(007879A8, Obj_CollectChain) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 96, 0);
    const u32 head = load<u32>(load<u32>(o + 12));
    ctx.gpr[5] = load<u32>(o + 12);
    u32 last = 0;
    if (head != 0) {
        ctx.gpr[6] = o + 16;
        ctx.gpr[7] = o + 96;
        u32 p = head;
        u32 next = 0;
        do {
            last = p;
            next = load<u32>(p + 12);
            if (next != 0) {
                const s32 idx = static_cast<s16>(load<u16>(o + 96));
                const u32 slot = o + 16 + static_cast<u32>(idx) * 4;
                store<u16>(o + 96, static_cast<u16>(idx + 1));
                store<u32>(slot, p);
                ctx.gpr[9] = static_cast<u64>(static_cast<s64>(idx) + 1);
                ctx.gpr[10] = slot;
            }
            p = next;
        } while (p != 0);
        ctx.gpr[5] = 0;
        ctx.gpr[8] = 0;
    }
    ctx.gpr[4] = last;
    store<u32>(o + 100, last);
}

// s32 Obj_SumChildSizes(Object* o): the sum of elem->+12 over every element of
// every list in o->+20[0 .. +24) (each list: *entry -> {+12 data, +16 count}).
DECOMP(00346AF8, Obj_SumChildSizes) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 n = static_cast<s32>(load<u32>(o + 24));
    u32 arr = load<u32>(o + 20);
    u32 sum = 0;
    s32 i = 0;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(n));
    ctx.gpr[5] = 0;
    if (n > 0) {
        do {
            const u32 list = load<u32>(arr) + 12;
            s32 k = 0;
            u32 off = 0;
            for (; k < static_cast<s32>(load<u32>(list + 4)); ++k, off += 4)
                sum += load<u32>(load<u32>(load<u32>(list) + off) + 12);
            ctx.gpr[8] = list;
            ctx.gpr[9] = off;
            ctx.gpr[10] = static_cast<u64>(static_cast<s64>(k));
            ctx.gpr[11] = load<u32>(list + 4);
            ++i;
            arr += 4;
        } while (i < n);
    }
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(i));
    ctx.gpr[7] = sum;
    ret(ctx, sum);
}

// void RefNode_Release(RefNode* n): drops a reference; at 0 the node is
// unlinked from the list at 0x00F65B7C and freed (24 bytes) (name?)
DECOMP(004432DC, RefNode_Release) {
    const u32 n = arg<u32>(ctx, 0);
    if (n == 0) return;
    s32 refs = static_cast<s32>(load<u32>(n + 20));
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(refs));
    if (refs <= 0) return;
    --refs;
    store<u32>(n + 20, static_cast<u32>(refs));
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(refs));
    if (refs != 0) return;
    const u32 prev = load<u32>(n + 8);
    const u32 next = load<u32>(n + 4);
    if (prev != 0) {
        store<u32>(prev + 4, next);
    } else {
        store<u32>(kRefListHead, next);
    }
    if (load<u32>(n + 4) != 0) store<u32>(load<u32>(n + 4) + 8, load<u32>(n + 8));
    call(ctx, dfn::SmallFree, {n, 24});
}

// void Nodes_Tick0CF534(f32 dt): sub_000CF534(obj, dt) for both objects of every node of 0x00F64A58 (name?)
DECOMP(002AA938, Nodes_Tick0CF534) {
    const f64 dt = farg(ctx, 0);
    for (u32 n = load<u32>(kNodeList); n != 0; n = load<u32>(n + kNodeNext))
        for (u32 k = 0; k < 2; ++k) callf(ctx, dfn::sub_000CF534, {load<u32>(n + 72 + k * 24)}, {dt});
}

// void Nodes_Tick2A9190(f32 dt): per node sub_002A9190(node, dt), then
// sub_000CF0C0(obj, dt) for its two objects (name?)
DECOMP(002AA8A0, Nodes_Tick2A9190) {
    const f64 dt = farg(ctx, 0);
    for (u32 n = load<u32>(kNodeList); n != 0; n = load<u32>(n + kNodeNext)) {
        callf(ctx, dfn::sub_002A9190, {n}, {dt});
        for (u32 k = 0; k < 2; ++k) callf(ctx, dfn::sub_000CF0C0, {load<u32>(n + 72 + k * 24)}, {dt});
    }
}

// void HandleMap_ForEachVfn16(): vfunc +16(obj) for every object in the handle map.
DECOMP(0035C070, HandleMap_ForEachVfn16) {
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(kHandleMap + 2)); ++i) {
        const u32 obj = load<u32>(load<u32>(kHandleMap + 8) + i * 8 + 4);
        vcall(ctx, obj, 16, {obj});
    }
}

// bool Obj_SetIfKind2(a, b, u32 v): x = sub_004BB4B4(a, b); when x's vfunc +32
// (kind?) returns 2, x->+20 = v and 1; else 0.
DECOMP(004A674C, Obj_SetIfKind2) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    if (a == 0) return ret(ctx, 0u);
    const u32 x = static_cast<u32>(call(ctx, dfn::sub_004BB4B4, {a, b, ctx.gpr[5], a}));
    if (x == 0) return ret(ctx, 0u);
    if (static_cast<s32>(vcall(ctx, x, 32, {x})) != 2) return ret(ctx, 0u);
    store<u32>(x + 20, v);
    ret(ctx, 1u);
}

// u32 Obj_CountFlagged(Object* o): how many of the +24 entries (860 bytes from
// +20) have a set +812 and bit 0x20 in the current world's save flags
// (u16 at save slot (user block byte 0) + 416 + entry byte 822 * 4).
DECOMP(002E2A98, Obj_CountFlagged) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 24);
    ctx.gpr[6] = n;
    ctx.gpr[7] = 0;
    ctx.gpr[8] = 0;
    if (n == 0) return ret(ctx, 0u);
    const u32 w = load<u32>(kCurrentWorld);
    u32 e = load<u32>(o + 20);
    const u32 s = ref<u8>(w + kUserBlocks) * kSaveSlotSize;
    u32 count = 0, i = 0;
    do {
        const u32 base = s + w;
        u32 r9 = load<u16>(base + ((static_cast<u32>(ref<u8>(e + 822)) << 2)) + 496) & 0x20u;
        if (r9 != 0) {
            r9 = load<u32>(e + 812);
            if (r9 != 0) ++count;
        }
        ctx.gpr[9] = r9;
        ctx.gpr[10] = base;
        ++i;
        e += 860;
    } while (i < n);
    ctx.gpr[4] = e;
    ctx.gpr[5] = w;
    ctx.gpr[7] = i;
    ctx.gpr[8] = count;
    ret(ctx, count);
}
