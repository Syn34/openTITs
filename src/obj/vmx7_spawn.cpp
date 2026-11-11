// Game code, VMX sweep part 7 (see vmx7.h): "spawn from a query" methods
// (the pattern of mid4_spawn.cpp / mid5_spawn.cpp) whose inlined query
// set-up uses VMX.
#include "../sly4.h"
#include "mid1.h"
#include "vmx7.h"

using namespace sly4;
using namespace sly4::vmx7;

namespace {
constexpr u32 kObj_Init22 = 0x000CDFB0;  // 22 arguments (14 on the stack), f32 x2
constexpr u32 kSplineCam_Ctor = 0x00214FAC;  // (p, Query* q) (name?)
constexpr u32 kSplineCamM1 = 0x00215120;  // f32 passed twice to Obj_Init22
constexpr u32 kFmtSplinePos = 0x00AD371C;  // "Spline_Camera_Pos%s"
constexpr u32 kFmtSplineTgt = 0x00AD3734;  // "Spline_Camera_Tgt%s"
constexpr u32 kSplineCamKind = 0x922D3BD1;  // the query's +164 kind that wants the camera names
}  // namespace

// Obj* f(Owner* o, Obj* t): QuerySpawn of a spline camera. Returns 0 when t
// is excluded (t->+4 bit 0x00400000 and t->+184 bit 2 set, bit 0x00800000
// clear). Else the query q (stack +224) is Obj_Init22(q, 1, 0, o->vfn[8](o,
// t), o, 0, 0x00B12000, 0x01164BD0, 0x01164B30, 0, 0, 1, 0, 0, 1, 0, 0, 0,
// -1, -, -, 0; m1, m1); q+144 = 0, +148 = 1, +149 = 0, +150 / +152 / +153
// = o's bytes +172 / +174 / +175 (0 without o), +156 = 0, +164 = the kind,
// +168 = +172 = -1.0, +176..+223 zeroed, +151 = 1, +4 |= 0x00C00000, +160 /
// +162 = o's u16 +176 / +178, +184 = o->+180; with t's bit 0x00800000:
// +156 = t->+188, +160 = t's u16 +194 unless its +192 is 0xFFFF, +164 =
// t->+196 and the 48 bytes t+208.. (lvx) over +176... Then t->vfn[2](t, q);
// when q's kind is still the camera kind, the name (t+140, or (t->+180)->
// +120 & ~1 when that string is empty and t->+180 is set) past its first
// '_' gives +176 = hash("Spline_Camera_Pos<name>") and +180 =
// hash("Spline_Camera_Tgt<name>") where they are 0. p = SmallAlloc(480)
// built by kSplineCam_Ctor(p, q); p->vfn[2](p) (also for a null p, as the
// original); q's PtrVector freed. Returns p.
DECOMP(00215128, QuerySpawn_SplineCam_00215128) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 tw = load<u32>(t + 4);
    if ((tw & 0x00400000u) != 0 && (load<u8>(t + 184) & 4) != 0 && (tw & 0x00800000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 v = static_cast<u32>(VCall(ctx, o, 32, {o, t}));
    Stack frame(ctx, 496);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 q = sp + 224;
    const f64 m1 = Lfs4(kSplineCamM1);
    // the stack slots of arguments 20 / 21 (+200 / +208) are left unwritten by the original
    callf(ctx, kObj_Init22,
          {q, 1, 0, v, o, 0, 0x00B12000, 0x01164BD0, 0x01164B30, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0xFFFFFFFFull, 0, 0, 0},
          {m1, m1});
    store<u32>(q + 144, 0);
    store<u8>(q + 148, 1);
    store<u8>(q + 149, 0);
    const u16 o176 = load<u16>(o + 176), o178 = load<u16>(o + 178);
    const u32 o180 = load<u32>(o + 180), t4 = load<u32>(t + 4);
    const u32 q4 = load<u32>(q + 4);
    store<u8>(q + 150, o != 0 ? load<u8>(o + 172) : 0);
    store<u8>(q + 152, o != 0 ? load<u8>(o + 174) : 0);
    store<u8>(q + 153, o != 0 ? load<u8>(o + 175) : 0);
    store<u32>(q + 156, 0);
    store<u32>(q + 164, kSplineCamKind);
    store<u32>(q + 168, 0xBF800000);
    store<u32>(q + 172, 0xBF800000);
    for (u32 k = 0; k < 3; ++k) Stvx(q + 176 + 16 * k, Zero4());
    store<u8>(q + 151, 1);
    store<u32>(q + 4, q4 | 0x00C00000);
    u32 name = t + 140;
    if (name == 0 || load<u8>(name) == 0) {
        const u32 x = load<u32>(t + 180);
        if (x != 0) name = load<u32>(x + 120) & ~1u;
    }
    store<u16>(q + 160, o176);
    store<u16>(q + 162, o178);
    store<u32>(q + 184, o180);
    if ((t4 & 0x00800000u) != 0) {
        store<u32>(q + 156, load<u32>(t + 188));
        if (load<u16>(t + 192) != 0xFFFF) store<u16>(q + 160, load<u16>(t + 194));
        store<u32>(q + 164, load<u32>(t + 196));
        const V4 a = Lvx(t + 208), b = Lvx(t + 224), c = Lvx(t + 240);
        Stvx(q + 176, a);
        Stvx(q + 192, b);
        Stvx(q + 208, c);
    }
    VCall(ctx, t, 8, {t, q});
    if (load<u32>(q + 164) == kSplineCamKind) {
        if (load<u8>(name) != 0) {
            for (;;) {
                const u8 ch = load<u8>(name);
                ++name;
                if (ch == '_' || load<u8>(name) == 0) break;
            }
        }
        if (load<u32>(q + 176) == 0) {
            const u32 s = static_cast<u32>(call(ctx, v4fn::String_Format, {kFmtSplinePos, name}));
            store<u32>(q + 176, static_cast<u32>(call(ctx, fn::String_HashNoCase, {s})));
        }
        if (load<u32>(q + 180) == 0) {
            const u32 s = static_cast<u32>(call(ctx, v4fn::String_Format, {kFmtSplineTgt, name}));
            store<u32>(q + 180, static_cast<u32>(call(ctx, fn::String_HashNoCase, {s})));
        }
    }
    const u32 p = static_cast<u32>(call(ctx, v5fn::SmallAlloc, {480}));
    if (p != 0) call(ctx, kSplineCam_Ctor, {p, q});
    VCall(ctx, p, 8, {p});  // also for p == 0, as the original
    mid1::PtrVecDeleteSmall(ctx, q);
    ret(ctx, p);
}
