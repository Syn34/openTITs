// World_HandleEvent callees, 42..45 instructions: component lookup and the
// per-frame job launchers over the object list at 0x01013B34.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
namespace ffn {
constexpr u32 MemStack_Alloc = 0x0055260C;  // (MemStack*, size)
constexpr u32 Job_Run456A60 = 0x00456A60;   // (JobDesc*) (name?)
constexpr u32 Job_Run456BA8 = 0x00456BA8;   // (JobDesc*) (name?)
constexpr u32 Job_Run456CF0 = 0x00456CF0;   // (JobDesc*) (name?)
constexpr u32 sub_0034D1A8 = 0x0034D1A8;    // (Object*, f32 dt)
constexpr u32 sub_00371360 = 0x00371360;    // (Object*)
}  // namespace ffn

// The object list the jobs run over: {u32 data, ..., u16 count at +8} (name?)
constexpr u32 kJobObjects = 0x01013B34;
constexpr u32 kGameFrameStack = 160;        // Game +: MemStack* for per-frame allocations (name?)

// The three jobs: {descriptor +0, counter +4 (cleared first by two of them), +16}.
struct JobKind {
    u32 desc, counter, extra, run;
    bool clear;
};
constexpr JobKind kJobCF97C{0x01013BB0, 0x01013BA0, 0x01021D80, ffn::Job_Run456A60, false};
constexpr JobKind kJobCE5D4{0x01019BE0, 0x01019BD0, 0x01021E80, ffn::Job_Run456BA8, true};
constexpr JobKind kJobCF480{0x0101DBF0, 0x0101DBE0, 0x01021F00, ffn::Job_Run456CF0, true};

// Builds a 32-byte job record on the frame stack {desc, counter, first, count,
// extra} for the object list and runs it, when the list is not empty.
void RunObjectJob(PPUContext& ctx, const JobKind& k) {
    if (k.clear) {
        store<u32>(k.counter, 0);
        ctx.gpr[4] = 0;
    }
    const u32 n0 = load<u16>(kJobObjects + 8);
    if (n0 == 0) return ret(ctx, 0u);
    const u32 m = static_cast<u32>(call(ctx, ffn::MemStack_Alloc, {load<u32>(g::game + kGameFrameStack), 32}));
    const u32 first = static_cast<s32>(load<u16>(kJobObjects + 8)) > 0 ? load<u32>(kJobObjects) : 0u;
    const u32 n = load<u16>(kJobObjects + 8);
    store<u32>(m + 8, first);
    store<u32>(m + 12, n);
    store<u32>(m + 0, k.desc);
    store<u32>(m + 4, k.counter);
    store<u32>(m + 16, k.extra);
    call(ctx, k.run, {m, m, k.desc, k.counter, k.extra, n});
}
}  // namespace

// void Objects_RunJobA() / B / C: the three per-frame jobs over the object list (name?)
DECOMP(000CF97C, Objects_RunJobA) { RunObjectJob(ctx, kJobCF97C); }
DECOMP(000CE5D4, Objects_RunJobB) { RunObjectJob(ctx, kJobCE5D4); }
DECOMP(000CF480, Objects_RunJobC) { RunObjectJob(ctx, kJobCF480); }

// void Objects_Tick34D1A8(f32 dt): while byte 0x0106BBE4 is set, sub_0034D1A8(obj, dt)
// for every object of the list at 0x0106BC70 whose u16 +40 has bit 0x100 (name?)
DECOMP(0034E2BC, Objects_Tick34D1A8) {
    constexpr u32 kList = 0x0106BC70, kEnabled = 0x0106BBE4;
    const f64 dt = farg(ctx, 0);
    if (ref<u8>(kEnabled) == 0) return;
    for (u32 i = 0; i < load<u16>(kList + 8); ++i) {
        const u32 obj = load<u32>(load<u32>(kList) + i * 4);
        if (((load<u16>(obj + 40) >> 8) & 1) != 0) callf(ctx, ffn::sub_0034D1A8, {obj, 1}, {dt});
    }
}

// void World_SetChild1281F4(World* w, Object* p): releases the current one
// (vfunc +12, then the deleting destructor +4 with r4 = its OPD) and stores p.
DECOMP(0047C530, World_SetChild1281F4) {
    constexpr u32 kChild = 0x1281F4;
    const u32 w = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 old = load<u32>(w + kChild);
    if (old != 0) {
        vcall(ctx, old, 12, {old});
        const u32 q = load<u32>(w + kChild);
        if (q != 0) vcall(ctx, q, 4, {q});
    }
    store<u32>(w + kChild, p);
    ret(ctx, kChild);
}

// Component* Obj_FindComponent(Object* o, s32 type, Component* after): the
// next entry of o's component vector (+248; u32 data, u16 count +8) whose +12
// is `type`, searching from just past `after` (from the start when after is 0
// or not in the vector). Registers as the original's two loops.
DECOMP(000D1288, Obj_FindComponent) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 type = static_cast<s32>(arg<u32>(ctx, 1));
    const u32 after = arg<u32>(ctx, 2);
    const u32 vec = load<u32>(o + 248);
    ctx.gpr[6] = 0;
    u32 start = 0;
    if (after != 0) {
        u32 i = 0, off = 0;
        for (;;) {
            const u32 n = vec != 0 ? load<u16>(vec + 8) : 0u;
            ctx.gpr[10] = n;
            if (i >= n) break;
            const u32 e = load<u32>(load<u32>(vec) + off);
            ++i;
            ctx.gpr[10] = e;
            if (static_cast<s32>(e) == static_cast<s32>(after)) {
                start = i;
                break;
            }
            off += 4;
        }
        ctx.gpr[8] = off;
        ctx.gpr[9] = i;
    }
    for (u32 j = start;; ++j) {
        const u32 n = vec != 0 ? load<u16>(vec + 8) : 0u;
        ctx.gpr[7] = j;
        ctx.gpr[5] = n;
        if (j >= n) return ret(ctx, 0u);
        const u32 c = load<u32>(load<u32>(vec) + j * 4);
        const u32 t = load<u32>(c + 12);
        ctx.gpr[5] = c;
        ctx.gpr[8] = t;
        if (static_cast<s32>(t) == type) return ret(ctx, c);
    }
}

// void Obj_ArmTarget(Object* o, u32 v): when +360 has bit 31 but not 0x08000000
// and the state (+160 >> 1) is 5: +360 bit 0x40000000 = v & 1,
// sub_00371360(o), then +360 |= 0x01000000. r3 = v otherwise. (The original
// also has a vfunc +12 branch for state 0 that the state-5 test makes unreachable.)
DECOMP(00371AE8, Obj_ArmTarget) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 360);
    ctx.gpr[3] = v;
    ctx.gpr[4] = w;
    ctx.gpr[5] = (w >> 27) & 1;
    if (((w >> 27) & 1) != 0) return;
    ctx.gpr[5] = w >> 31;
    if ((w >> 31) == 0) return;
    const u32 s = load<u32>(o + 160) >> 1;
    ctx.gpr[5] = s;
    if (s != 5) return;
    const u32 nw = (w & ~0x40000000u) | ((v & 1) << 30);
    store<u32>(o + 360, nw);
    call(ctx, ffn::sub_00371360, {o, nw, s});
    const u32 f = load<u32>(o + 360) | 0x01000000u;
    store<u32>(o + 360, f);
    ret(ctx, f);
}
