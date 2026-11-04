// World_HandleEvent callees, 22..26 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
namespace bfn {
constexpr u32 sub_00447B70 = 0x00447B70;   // () after a push (name?)
constexpr u32 sub_00484BE0 = 0x00484BE0;   // (UserBlock*, ...) 9 arguments (name?)
constexpr u32 Set_Find = 0x004BCE90;       // (u32 name) -> Set*
constexpr u32 sub_004BD1CC = 0x004BD1CC;   // (Set*)
constexpr u32 sub_002DD898 = 0x002DD898;   // (Session*)
constexpr u32 Effect_Release = 0x004CF3C4; // (Effect*, 0x10CBB)
constexpr u32 sub_000190B4 = 0x000190B4;   // (u32)
constexpr u32 sub_00484858 = 0x00484858;   // (World*, UserBlock*)
constexpr u32 sub_004D22B0 = 0x004D22B0;   // (Object*)
constexpr u32 sub_0056DFC8 = 0x0056DFC8;   // (0x01163FC8)
constexpr u32 Nop = 0x0054EC18;
constexpr u32 SmallFree = 0x00552420;      // (p, size)
constexpr u32 Ui_Find = 0x004D5544;        // (path, parent)
constexpr u32 memset = 0x00726E54;
constexpr u32 PostFx_UpdateTint = 0x004691E8;  // (s, f32 dt)
}  // namespace bfn

constexpr u32 kPushCount = 0x010AFE90;      // u32 count of the 4-entry stack at 0x010AFB84 (name?)
constexpr u32 kPushStack = 0x010AFB84;
constexpr u32 kSetName7719A548 = 0x7719A548;   // hashed Set name (name?)
constexpr u32 kEffectReleaseTag = 0x10CBB;
constexpr u32 kGlobalF557F4 = 0x00F557F4;
constexpr u32 kTune106BA3C = 0x0106BA3C;       // tuning object, value at +72 (name?)
constexpr u32 kConst191A8 = 0x000191A8;        // f32 {-0.5, 20.0}
constexpr u32 kPadsReady = 0x00F65C84;         // u8 (name?)
constexpr u32 kPadStride = 11968;              // two pad blocks from the Game object (+176) (name?)
constexpr u32 kGlobal1163FC8 = 0x01163FC8;
constexpr u32 kFlag1163F80 = 0x01163F80;       // u8 (name?)
constexpr u32 kLetterboxPath = 0x00AE8AF8;     // "InGame.Cutscene.Letterbox"
constexpr u32 kUiVfnShow = 88;                 // (ui, bool) (name?)
constexpr u32 kFadeUi = 0x00F65C88;            // u32 Ui* of "Common.Fade"
constexpr u32 kZero787DC0 = 0x00787DC0;        // f32 0.0

// The World's callback PtrVectors (+0x128480, 20-byte stride); 5..11 are reset together.
constexpr u32 kWorldLists = 0x128480, kWorldListStride = 20;

constexpr u32 kWorldUserActive = 0x1281E0;     // u32 (name?)
constexpr u32 kWorldTimeScale = 0x1282A4;      // ScaleBlend: +8 f32 value, +24 bit 31 on
constexpr u32 kWorldSlowMo = 0x128588;         // f32 pair (+0, +12) (name?)
constexpr u32 kWorldFilter = 0x1283A8;         // f32 a, b, c, u32 d, u32 = 0, u32 flags
}  // namespace

// void PushStack_Push(u32 v): appends v to the 4-entry array at 0x010AFB84
// when there is room and v != 0, then sub_00447B70() (name?). Leaves r4 = the old count otherwise.
DECOMP(0044D5BC, PushStack_Push) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 n = load<u32>(kPushCount);
    ctx.gpr[4] = n;
    if (n >= 4 || v == 0) return;
    store<u32>(kPushStack + ((n << 2) & 0xFFFFFFFCu), v);
    store<u32>(kPushCount, n + 1);
    call(ctx, bfn::sub_00447B70, {v, n + 1, kPushStack, (n << 2) & 0xFFFFFFFCu, 0x010B0000u});
}

// void World_UserCall484BE0(World* w, u32 user, a, b, u8 c, u8 d, u8 e, u8 f, u8 g):
// sub_00484BE0(the user's block, a, b, c, d, e, f, g); g is the stack argument.
DECOMP(00488118, World_UserCall484BE0) {
    const u32 w = arg<u32>(ctx, 0), user = arg<u32>(ctx, 1);
    const u32 g9 = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116) & 0xFF;  // 9th argument (low word)
    ctx.gpr[11] = g9;  // the original loads the stack argument into r11
    ctx.gpr[12] = 0;
    call(ctx, bfn::sub_00484BE0,
         {w + user * kUserBlockSize + kUserBlocks, ctx.gpr[5], ctx.gpr[6], arg<u32>(ctx, 4) & 0xFF,
          arg<u32>(ctx, 5) & 0xFF, arg<u32>(ctx, 6) & 0xFF, arg<u32>(ctx, 7) & 0xFF, g9});
}

// u32 Session_ReleaseSet(Session* s): when s->+40 is set and the Set 0x7719A548
// exists, sub_004BD1CC(set) then sub_002DD898(s). r4 is passed through to Set_Find as is.
DECOMP(002DE1F4, Session_ReleaseSet) {
    const u32 s = arg<u32>(ctx, 0);
    if (load<u32>(s + 40) == 0) return ret(ctx, 0u);
    const u32 set = static_cast<u32>(call(ctx, bfn::Set_Find, {kSetName7719A548, ctx.gpr[4]}));
    if (set == 0) return ret(ctx, 0u);
    call(ctx, bfn::sub_004BD1CC, {set});
    call(ctx, bfn::sub_002DD898, {s});
}

// void Obj_ReleaseEffect156(Object* o): Effect_Release(o->+156, 0x10CBB), sets its
// +136 bit 0x200 and clears o->+156. Leaves r3 = the effect, r4 = 0, r5 = its flags.
DECOMP(00424F24, Obj_ReleaseEffect156) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 156) == 0) return ret(ctx, 0u);
    call(ctx, bfn::Effect_Release, {load<u32>(o + 156), kEffectReleaseTag});
    const u32 e = load<u32>(o + 156);
    const u32 f = load<u32>(e + 136) | 0x200u;
    store<u32>(e + 136, f);
    store<u32>(o + 156, 0);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = f;
    ret(ctx, e);
}

// void World_ResetLists5to11(World* w): count = 0 for the callback lists 5..11.
// Leaves r4..r11 as the original's offsets.
DECOMP(00458B50, World_ResetLists5to11) {
    const u32 w = arg<u32>(ctx, 0);
    for (u32 k = 5; k <= 11; ++k) store<u16>(w + kWorldLists + k * kWorldListStride + 8, 0);
    ctx.gpr[4] = 0x128514;
    ctx.gpr[5] = 0x128550;
    ctx.gpr[6] = 0x128500;
    ctx.gpr[7] = 0x128564;
    ctx.gpr[8] = 0;
    ctx.gpr[9] = ctx.gpr[10] = ctx.gpr[11] = 0x120000;
}

// f32 World_GetTimeScale(World* w): 1.0 * slow-mo +0 * slow-mo +12, times the
// time-scale blend's value while it is on (name?). Leaves r4/r5 and f2/f3 as the original.
DECOMP(007877D0, World_GetTimeScale) {
    const u32 w = arg<u32>(ctx, 0);
    const f32 one = load<f32>(0x007877C8);
    const f32 a = load<f32>(w + kWorldSlowMo), b = load<f32>(w + kWorldSlowMo + 12);
    f32 r = fmuls(fmuls(one, a), b);
    const u32 on = (load<u32>(w + kWorldTimeScale + 24) >> 31) & 1;
    ctx.gpr[5] = 0x1282BC;
    ctx.gpr[4] = on;
    ctx.fpr[2] = a;
    ctx.fpr[3] = b;
    if (on != 0) {
        const f32 s = load<f32>(w + kWorldTimeScale + 8);
        ctx.gpr[4] = 0x1282AC;
        ctx.fpr[2] = s;
        r = fmuls(r, s);
    }
    ret(ctx, r);
}

// void Obj_SetValue32FromTuning(Object* o): sub_000190B4(*0x00F557F4), then
// o->f32 = tuning 0x0106BA3C * -0.5 + 20.0 (name?)
DECOMP(000191B0, Obj_SetValue32FromTuning) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, bfn::sub_000190B4, {load<u32>(kGlobalF557F4), 0x20000u, 0x00F50000u});
    const f32 c0 = load<f32>(kConst191A8), c1 = load<f32>(kConst191A8 + 4);
    const f32 t = load<f32>(kTune106BA3C + 72);
    const f32 v = static_cast<f32>(std::fma(static_cast<f64>(t), static_cast<f64>(c0), static_cast<f64>(c1)));
    store<f32>(o + 32, v);
    ctx.fpr[1] = v;
    ctx.fpr[2] = c1;
    ctx.fpr[3] = t;
    ret(ctx, kTune106BA3C);
}

// void Obj_DestroyChild8(Object* o): o->p8->vfunc+40(p8, 0), then o->p8 = 0. r3 = 0.
DECOMP(00465C50, Obj_DestroyChild8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 8);
    if (p != 0) {
        vcall(ctx, p, 40, {p, 0});
        store<u32>(o + 8, 0);
    }
    ret(ctx, 0u);
}

// void World_UserCall484858(World* w, u32 user): sub_00484858(w, the user's block)
// while w->+0x1281E0 is set. r3 = user, r4 = w, r5 = 0, r6 = user, r7 = 0x1281E0 otherwise.
DECOMP(00488A78, World_UserCall484858) {
    const u32 w = arg<u32>(ctx, 0), user = arg<u32>(ctx, 1);
    ctx.gpr[6] = user;
    ctx.gpr[7] = kWorldUserActive;
    if (load<u32>(w + kWorldUserActive) == 0) {
        ctx.gpr[5] = 0;
        ctx.gpr[4] = w;
        return ret(ctx, user);
    }
    const u32 base = w + user * kUserBlockSize;
    call(ctx, bfn::sub_00484858, {w, base + kUserBlocks, base});
}

// bool Pads_AnyBit40(): byte 0x00F65C84 set and, for either of the two pad blocks
// (Game + 176, 11968 apart), (+8 & +12) has bit 0x40 (name?)
DECOMP(0048BEC8, Pads_AnyBit40) {
    if (ref<u8>(kPadsReady) == 0) return ret(ctx, 0u);
    u32 base = g::game;
    for (u32 k = 0; k < 2; ++k, base += kPadStride) {
        const u32 e = base + 176;
        const u32 a = load<u32>(e + 8);
        const u32 m = (a & load<u32>(e + 12)) & 0x40u;
        ctx.gpr[4] = k;
        ctx.gpr[5] = m;
        ctx.gpr[6] = a;
        if (m != 0) return ret(ctx, 1u);
    }
    ctx.gpr[4] = 2;
    ret(ctx, 0u);
}

// void World_SetFilter(World* w, f32 a, f32 b, f32 c, u32 d, bool on): the
// filter at +0x1283A8 = {a, b, c, d, 0}; its flags word gets bit 31 = on and bit 30 set (name?)
DECOMP(00787CCC, World_SetFilter) {
    const u32 w = arg<u32>(ctx, 0) + kWorldFilter;
    store<f32>(w, static_cast<f32>(farg(ctx, 0)));
    store<u32>(w + 12, arg<u32>(ctx, 4));
    store<f32>(w + 4, static_cast<f32>(farg(ctx, 1)));
    store<f32>(w + 8, static_cast<f32>(farg(ctx, 2)));
    store<u32>(w + 16, 0);
    const u32 f = (load<u32>(w + 20) & 0x7FFFFFFFu) | ((arg<u32>(ctx, 5) & 1) << 31) | 0x40000000u;
    store<u32>(w + 20, f);
    ctx.gpr[4] = f;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = 0x1283BC;
    ctx.gpr[9] = 0x1283B8;
    ctx.gpr[10] = ctx.gpr[11] = 0x120000;
}

// void Ui_Release4D22B0(?, Object* o, u8 keep): sub_004D22B0(o) when o is set,
// sub_0056DFC8(0x01163FC8), and byte 0x01163F80 = 1 unless keep (r3 is not read) (name?)
DECOMP(0046617C, Ui_Release4D22B0) {
    const u32 o = arg<u32>(ctx, 1);
    const u32 keep = arg<u32>(ctx, 2);
    if (o != 0) call(ctx, bfn::sub_004D22B0, {o});
    call(ctx, bfn::sub_0056DFC8, {kGlobal1163FC8});
    const u32 k = keep & 0xFF;
    if (k != 0) return ret(ctx, k);
    ref<u8>(kFlag1163F80) = 1;
    ctx.gpr[4] = 1;
    ret(ctx, 0x01160000u);
}

// void PtrVectorBox_Free(PtrVector** pp): PtrVector_Clear(*pp), Nop(*pp),
// SmallFree(*pp, 20); *pp is not cleared. r3 = pp without one.
DECOMP(0078756C, PtrVectorBox_Free) {
    const u32 pp = arg<u32>(ctx, 0);
    const u32 v = load<u32>(pp);
    if (v == 0) return;
    call(ctx, hfn::PtrVector_Clear, {v});
    call(ctx, bfn::Nop, {v});
    call(ctx, bfn::SmallFree, {load<u32>(pp), 20});
}

// void Letterbox_Show(?, u8 on): Ui_Find("InGame.Cutscene.Letterbox", 0)->vfunc+88(ui, on).
DECOMP(00470590, Letterbox_Show) {
    const u32 on = arg<u32>(ctx, 1) & 0xFF;
    const u32 ui = static_cast<u32>(call(ctx, bfn::Ui_Find, {kLetterboxPath, 0}));
    if (ui == 0) return;
    vcall(ctx, ui, kUiVfnShow, {ui, on});
}

// void World_ClearBusyHideFade(World* w): clears +0x1282A0 bit 0x40000000 and
// hides the "Common.Fade" Ui (vfunc +88 (ui, 0)). Leaves r3 = 0 without the Ui.
DECOMP(0078749C, World_ClearBusyHideFade) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 v = load<u32>(w + wevent::kWorldBusy) & ~0x40000000u;
    store<u32>(w + wevent::kWorldBusy, v);
    const u32 ui = load<u32>(kFadeUi);
    ctx.gpr[4] = wevent::kWorldBusy;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = 0x00F60000u;
    ctx.gpr[7] = v;
    if (ui == 0) return ret(ctx, 0u);
    vcall(ctx, ui, kUiVfnShow, {ui, 0});
}

// void PostFx_ResetTint(PostFx* p): zeroes 112 bytes, sets +0 bit 0x00800000 and
// +112 = 1, then PostFx_UpdateTint(p, 0.0).
DECOMP(00787DC8, PostFx_ResetTint) {
    const u32 p = arg<u32>(ctx, 0);
    call(ctx, bfn::memset, {p, 0, 112});
    const u32 f = load<u32>(p) | 0x00800000u;
    store<u32>(p + 112, 1);
    store<u32>(p, f);
    callf(ctx, bfn::PostFx_UpdateTint, {p, 1, f}, {static_cast<f64>(load<f32>(kZero787DC0))});
}
