// Game code, VMX sweep part 6 (see vmx6.h): spline instances.
#include "../sly4.h"
#include "vmx6.h"

using namespace sly4;
using namespace sly4::vmx6;

namespace {
constexpr u32 kPool3E7030 = 0x003E7020;  // f32 {FLT_MAX, 0.0, 1.0}
constexpr u32 kAllocSlot3E7030 = 0x00F8F158;  // -> {free list*, heap} (ApeTListAllocator<ApeSplineInstance>)
constexpr u32 kFreeListVt3E7030 = 0x00B0EFE8;
constexpr u32 kFreeListName3E7030 = 0x00ADEFB6;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeSplineInstance]"
constexpr u32 kLenScale3E7030 = 0x0108562C;  // f32 divisor of length * (1 - t)
constexpr u32 kLenMin3E7030 = 0x01085C68;  // f32 minimum of that
}  // namespace

// SplineInstance* f(Obj* self, const f32 p[3], ?, f32* t, f32* dist; f32
// maxDist) (called by 003E7D78): the nearest spline instance to p. The
// instance allocator's free list is created on first use (Mem_Alloc(60,
// 0, 16), its fields cleared, FreeList_Init(list, 288, heap, 0, 0); the
// slot is written even when the allocation failed). Over the used list
// (Obj_GetField4(list), +16 per node, links at node-16), skipping
// self->+56: an instance with mode (SplineInstance_GetMode) 2..15, flag
// 0x8000 of +164 (and of its parent +260's when set), whose def (+208)
// bounding sphere (+48, radius +60) through the instance rows +80 is
// within maxDist of p, SplineInstance_Nearest(inst, p, 0, &d) = t with
// def->+120 * (1 - t) / scale >= min, and the smallest d wins. *t / *dist
// (when non-null) get the winner's t / d (0.0 / FLT_MAX without one).
// f1 is what the original leaves.
DECOMP(003E7030, Spline_FindNearestInstance_003E7030) {
    const u32 self = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), outT = arg<u32>(ctx, 3), outD = arg<u32>(ctx, 4);
    const f64 maxd = ctx.fpr[1];
    Stack frame(ctx, 240);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 slot = load<u32>(kAllocSlot3E7030);
    f64 bestD = Lfs4(kPool3E7030), bestT = Lfs4(kPool3E7030 + 4);
    u32 list = load<u32>(slot);
    if (list == 0) {
        const u32 l = static_cast<u32>(call(ctx, 0x0054ED38, {60, 0, 16}));  // Mem_Alloc
        if (l != 0) {
            const u32 heap = load<u32>(slot + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(l + off, 0);
            store<u16>(l + 52, 0);
            store<u16>(l + 54, static_cast<u16>(load<u16>(l + 54) & ~0x8000u));
            store<u32>(l, kFreeListVt3E7030);
            store<u32>(l + 56, kFreeListName3E7030);
            call(ctx, 0x00551A58, {l, 288, heap, 0, 0});  // FreeList_Init
            list = l;
        }
        store<u32>(slot, list);  // also when the allocation failed (quirk)
    }
    const u32 first = static_cast<u32>(call(ctx, 0x00556BF0, {list}));  // Obj_GetField4
    u32 inst = first != 0 ? first + 16 : 0;
    u32 best = 0;
    if (inst != 0) {
        const f64 one = Lfs4(kPool3E7030 + 8);
        for (; inst != 0;) {
            if (inst != load<u32>(self + 56)) {
                const u32 mode = static_cast<u32>(call(ctx, 0x005A8BA8, {inst}));  // SplineInstance_GetMode
                const u32 par = load<u32>(inst + 260);
                if (mode >= 2 && mode <= 15 && (load<u32>(inst + 164) & 0x8000u) != 0 &&
                    (par == 0 || (load<u32>(par + 164) & 0x8000u) != 0)) {
                    const u32 c = load<u32>(inst + 208) + 48;
                    const u32 R = inst + 80;
                    Stvx(sp + 112, XformPoint(Lvx(R), Lvx(R + 16), Lvx(R + 32), Lvx(R + 48), W3(c)));
                    const f32 dy = Subs(Lfs4(sp + 116), Lfs4(p + 4)), dx = Subs(Lfs4(sp + 112), Lfs4(p)),
                              dz = Subs(Lfs4(sp + 120), Lfs4(p + 8));
                    const f32 l2 = Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)));
                    const f32 ds = Subs(static_cast<f32>(std::sqrt(static_cast<f64>(l2))), Lfs4(c + 12));
                    ctx.fpr[1] = ds;
                    if (!(ds > maxd)) {
                        const f64 t = callf(ctx, 0x005927FC, {inst, p, 0, sp + 128}, {});  // SplineInstance_Nearest
                        const f32 rem = Divs(Muls(Lfs4(load<u32>(inst + 208) + 120), Subs(one, t)), Lfs4(kLenScale3E7030));
                        ctx.fpr[1] = rem;
                        if (!(rem < Lfs4(kLenMin3E7030))) {
                            const f64 d = Lfs4(sp + 128);
                            ctx.fpr[1] = d;
                            if (d < bestD) {
                                best = inst;
                                bestD = d;
                                bestT = t;
                            }
                        }
                    }
                }
            }
            const u32 nx = load<u32>(inst - 16);
            inst = nx != 0 ? nx + 16 : 0;
        }
    }
    if (outT != 0) store<f32>(outT, static_cast<f32>(bestT));
    if (outD != 0) store<f32>(outD, static_cast<f32>(bestD));
    ctx.gpr[3] = best;
}
