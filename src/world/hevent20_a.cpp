// World_HandleEvent callees, part 20a: small leaves under the World event
// handler's level and object paths (a flag bit, a free, the level set stop
// / refresh, a request forwarder, a layer stack reset, a list notifier and
// the object slot accessors).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kMemFreeFrom = 0x0054EE6C;        // (p, pool) (symbols.txt Mem_FreeFrom)
constexpr u32 kSetFind = 0x004BCE90;            // (u32 hash) -> Set* or 0 (symbols.txt Set_Find)
constexpr u32 kSetStop = 0x004BD1CC;            // (Set*) (symbols.txt Set_Stop4BD1CC)
constexpr u32 kLoaderStop = 0x002DD898;         // (Loader*) (symbols.txt Loader_Stop2DD898)
constexpr u32 kSessionRefresh = 0x002DDDCC;     // (Session*) (symbols.txt Session_Refresh2DDDCC)
constexpr u32 kSetRestart4BE420 = 0x004BE420;   // (Set*) (name?)
constexpr u32 kRequest3690F0 = 0x003690F0;      // (a, b, c, d, u8 e) (name?)
constexpr u32 kLayerStackClear = 0x0042B0C8;    // () (symbols.txt LayerStack_Clear)
constexpr u32 kLevelSet = 0x7719A548;           // hash of the level set looked up by the loader
constexpr u32 kLayerBlock = 0x00F65858;         // u32 block freed by LayerStack_Reset42B240
constexpr u32 kLayerWord0 = 0x010AA690, kLayerWord1 = 0x00F6585C;  // u32 words cleared with it
constexpr u32 kDefaultPos = 0x01164B40;         // f32[3] returned for an empty / out-of-range slot

// The object's definition (+8) slot count (+152), compared unsigned.
inline bool slot_in_range(u32 o, u32 i) { return i < load<u32>(load<u32>(o + 8) + 152); }

// Copy the three f32 words at src to dst as raw words; f1 is left holding
// the last one as lfs loads it.
inline void copy_vec3(PPUContext& ctx, u32 dst, u32 src) {
    for (u32 k = 0; k < 12; k += 4) store<u32>(dst + k, load<u32>(src + k));
    ctx.fpr[1] = hevent7::lfs_f64(src + 8);
}

// The slot accessors' tail: slot pointer p (+128 vec3) or the default.
inline void slot_pos(PPUContext& ctx, u32 out, u32 o, u32 i, u32 table, u32 shift) {
    if (slot_in_range(o, i)) {
        const u32 p = load<u32>(load<u32>(o + table) + (i << shift));
        if (p != 0) {
            copy_vec3(ctx, out, p + 128);
            ctx.gpr[4] = p + 128;
            ctx.gpr[5] = i << shift;
            ctx.gpr[3] = out;
            return;
        }
    }
    copy_vec3(ctx, out, kDefaultPos);
    ctx.gpr[4] = kDefaultPos & 0xFFFF0000u;
    ctx.gpr[5] = kDefaultPos;
    ctx.gpr[3] = out;
}

// The slot notifiers' tail: slot pointer p's vt +40 (p, a, 1).
inline void slot_notify(PPUContext& ctx, u32 o, u32 a, u32 i, u32 table, u32 shift) {
    if (!slot_in_range(o, i)) {
        ret(ctx, o);
        return;
    }
    const u32 p = load<u32>(load<u32>(o + table) + (i << shift));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    vcall(ctx, p, 40, {p, a & 0xFF, 1});
}
}  // namespace

// void Flag_Set787630(u8* base, u32 off, u32 v) (name?): byte (base + off)
// +656 bit 0x80 = v & 1. r3 is left at base + off, r4 at the new byte.
DECOMP(00787630, Flag_Set787630) {
    const u32 p = static_cast<u32>(arg<u64>(ctx, 0) + arg<u64>(ctx, 1));
    const u32 b = (load<u8>(p + 656) & ~0x80u) | ((arg<u32>(ctx, 2) << 7) & 0x80u);
    store<u8>(p + 656, static_cast<u8>(b));
    ctx.gpr[4] = b;
    ctx.gpr[3] = p;
}

// void Mem_Free735F54(void* p) (name?): Mem_FreeFrom(p, 0).
DECOMP(00735F54, Mem_Free735F54) {
    call(ctx, kMemFreeFrom, {arg<u32>(ctx, 0), 0});
}

// void Loader_StopLevel2DDA54(Loader* l) (name?): when l +40 is set and the
// level set exists: Set_Stop4BD1CC(set), then Loader_Stop2DD898(l).
DECOMP(002DDA54, Loader_StopLevel2DDA54) {
    const u32 l = arg<u32>(ctx, 0);
    if (load<u32>(l + 40) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 set = static_cast<u32>(call(ctx, kSetFind, {kLevelSet}));
    if (set == 0) return;
    call(ctx, kSetStop, {set});
    call(ctx, kLoaderStop, {l});
}

// void Loader_Refresh2DE198(Loader* l) (name?): when l +40 is set and the
// level set exists, 0x004BE420(set); then Session_Refresh2DDDCC(l).
DECOMP(002DE198, Loader_Refresh2DE198) {
    const u32 l = arg<u32>(ctx, 0);
    if (load<u32>(l + 40) != 0) {
        const u32 set = static_cast<u32>(call(ctx, kSetFind, {kLevelSet}));
        if (set != 0) call(ctx, kSetRestart4BE420, {set});
    }
    call(ctx, kSessionRefresh, {l});
}

// void Request_Forward3694A0(a, b, c, d, u8 e) (name?): 0x003690F0(a, b, c,
// d, e) when a is set. The original shuffles its arguments through r8 / r9
// and leaves them (and the zero branch's swapped r3..r6) behind.
DECOMP(003694A0, Request_Forward3694A0) {
    const u64 a = ctx.gpr[3], b = ctx.gpr[4], c = ctx.gpr[5], d = ctx.gpr[6];
    const u32 e = arg<u32>(ctx, 4) & 0xFF;
    if (static_cast<s32>(a) == 0) {
        ctx.gpr[3] = d;
        ctx.gpr[4] = c;
        ctx.gpr[5] = b;
        ctx.gpr[6] = a;
        ctx.gpr[7] = e;
        ctx.gpr[8] = d;
        ctx.gpr[9] = c;
        return;
    }
    ctx.gpr[8] = c;  // left by the original's shuffle (0x003690F0 does not read them)
    ctx.gpr[9] = d;
    call(ctx, kRequest3690F0, {a, b, c, d, e});
}

// void LayerStack_Reset42B240() (name?): LayerStack_Clear(), then frees the
// block at 0x00F65858 (cleared), and clears 0x010AA690 and 0x00F6585C.
DECOMP(0042B240, LayerStack_Reset42B240) {
    call(ctx, kLayerStackClear, {ctx.gpr[3]});
    const u32 blk = load<u32>(kLayerBlock);
    if (blk != 0) {
        call(ctx, kMemFreeFrom, {blk, 0});
        store<u32>(kLayerBlock, 0);
    }
    store<u32>(kLayerWord0, 0);
    store<u32>(kLayerWord1, 0);
    ctx.gpr[3] = 0x010B0000u;  // lis r3, 0x10B (left)
    ctx.gpr[4] = 0;
    ctx.gpr[5] = kLayerWord1 & 0xFFFF0000u;
}

// void List_Notify456500(Owner* o, s32 key) (name?): the first node of the
// list o +32 (next +72) whose +76 == key gets its vt +24 (node).
DECOMP(00456500, List_Notify456500) {
    const u32 key = arg<u32>(ctx, 1);
    for (u32 n = load<u32>(arg<u32>(ctx, 0) + 32); n != 0; n = load<u32>(n + 72)) {
        const u32 k = load<u32>(n + 76);
        ctx.gpr[5] = k;
        if (static_cast<s32>(k) == static_cast<s32>(key)) {
            vcall(ctx, n, 24, {n});
            return;
        }
    }
    ret(ctx, 0);
}

// void Obj_SlotNotify36BFAC(Obj* o, u8 a, u32 i) (name?): slot i of the
// 16-byte table o +328 (count def +152): vt +40 (slot, a, 1).
DECOMP(0036BFAC, Obj_SlotNotify36BFAC) {
    slot_notify(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2), 328, 4);
}

// void Obj_SlotNotify370938(Obj* o, u8 a, u32 i) (name?): as 0x0036BFAC
// with the 4-byte table o +200.
DECOMP(00370938, Obj_SlotNotify370938) {
    slot_notify(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2), 200, 2);
}

// f32* Obj_SlotPos36C06C(f32 out[3], Obj* o, u32 i) (name?): slot i of the
// 16-byte table o +328: its +128 vec3, or the default at 0x01164B40.
DECOMP(0036C06C, Obj_SlotPos36C06C) {
    slot_pos(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2), 328, 4);
}

// f32* Obj_SlotPos3709A0(f32 out[3], Obj* o, u32 i) (name?): as 0x0036C06C
// with the 4-byte table o +200.
DECOMP(003709A0, Obj_SlotPos3709A0) {
    slot_pos(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2), 200, 2);
}
