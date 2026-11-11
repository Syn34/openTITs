// Game code, VMX sweep part 7 (see vmx7.h): the loading screen.
#include "../sly4.h"
#include "vmx7.h"

using namespace sly4;
using namespace sly4::vmx7;

namespace {
constexpr u32 kLoad_Fn44D9E0 = 0x0044D9E0;  // T* (T* x) (reads r3 only) (name?)
constexpr u32 kShader_LoadGroup = 0x0050F0F0;  // (u32 group) -> ShaderGroup*
constexpr u32 kMat4_Ortho = 0x0056A358;  // (f32 out[16]; f1 l, f2 r, f3 b, f4 t, f5 n, f6 f)
constexpr u32 kSysEventFlagCreate7 = 82;  // sys_event_flag_create(u32* id, const sys_event_flag_attribute_t*, u64 init)

constexpr u32 kLoadInitDone = 0x00F9CDE0;  // u8: the screen's widgets / state set up
constexpr u32 kLoadGfxDone = 0x00F9CDE1;  // u8: the graphics part set up
constexpr u32 kLoadOpt = 0x00F9D090;  // +72 u8: also run kLoad_Fn44D9E0
constexpr u32 kLoadOptArg = 0x00F9D0D8;
constexpr u32 kLoadOptOut = 0x00F5590C;
constexpr u32 kLoadState = 0x00F9CED0;  // 220 bytes: +0 size 220, +16 f32 1.0
constexpr u32 kLoadWidgets = 0x00F558E8;  // Widget* {Common.Loading, its hint text, its top text}
constexpr u32 kStrCommonLoading = 0x00AC5DC0;  // "Common.Loading"
constexpr u32 kStrHintText = 0x00AC5DD0;  // "HintText.Clip.text"
constexpr u32 kStrTopText = 0x00AC5DE8;  // "TopText.Clip.text"
constexpr u32 kLoadFlags = 0x00F558F4;  // u32* {start, continue} event flag records (8 bytes: name, id)
constexpr u32 kStrStartLoading = 0x00AC5E0C;  // "StartLoadingScreen"
constexpr u32 kStrContinueThread = 0x00AC5E24;  // "ContinueThread"
constexpr u32 kLoadShaderPtrs = 0x00F558FC;  // u32 {group data + 4, + 8}
constexpr u32 kLoadOrthoK = 0x000ABBD0;  // f32 {-1, 0, 1}
constexpr u32 kLoadMatrix = 0x00F9CE80;  // the screen matrix (16-byte rows)
constexpr u32 kLoadDescs = 0x00F9CE00;  // 12-byte records; +120 u16 24, +124 u16 |= 0x8000, +126 u16 mask
constexpr u32 kLoadDescSrc = 0x00F64464;  // 12-byte records {u32 a, u32 b (0 ends), u8 c, u8 d, u8 e}

// An event flag record p = Mem_Alloc(8, 0, 16) named name: attr at a (32
// bytes: protocol 2, pshared 0x200, ipc_key 0, flags 0, type 0x20000, name
// = String_Copy(a + 24, name, 8)), sys_event_flag_create(p + 4, a, 0).
// Returns p (0 when the allocation fails).
u32 LoadNewEventFlag(PPUContext& ctx, u32 a, u32 name) {
    const u32 p = static_cast<u32>(call(ctx, fn::Mem_Alloc, {8, 0, 16}));
    if (p == 0) return 0;
    store<u32>(p, name);
    store<u32>(a, 2);
    store<u32>(a + 4, 512);
    store<u64>(a + 8, 0);
    store<u8>(a + 24, 0);
    store<u32>(a + 16, 0);
    store<u32>(a + 20, 0x20000);
    call(ctx, fn::String_Copy, {a + 24, load<u32>(p), 8});
    syscall(ctx, kSysEventFlagCreate7, {p + 4, a, 0});
    return p;
}
}  // namespace

// void f(u8 gfx) (from 0x000AC0F0 / 0x000AC698): loading screen set-up.
// With gfx (once, kLoadGfxDone): the descriptor list kLoadDescSrc is copied
// to kLoadDescs up to and including its terminator (b == 0), the u16 +126
// mask getting bit (a + d) for each copied entry, +120 = 24, +124 |=
// 0x8000; the shader group 4's data pointer (+16 ->+12 ->+0) + 4 / + 8 is
// kept; the screen matrix is an ortho (-1..1, 0..1) times the global
// matrix at 0x01164BD0 (row by row, vmaddfp); and the two event flags
// "StartLoadingScreen" / "ContinueThread" are made. Without gfx (once,
// kLoadInitDone): optionally kLoad_Fn44D9E0, the 220-byte state cleared
// (+0 = 220, +16 = 1.0), and the three widgets looked up. r3 is left as
// the original leaves it (the last result, or the guard byte).
DECOMP(000ABBE0, Loading_Init_000ABBE0) {
    const u8 gfx = static_cast<u8>(arg<u32>(ctx, 0));
    Stack frame(ctx, 256);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    if (gfx != 0) {
        const u8 done = load<u8>(kLoadGfxDone);
        ctx.gpr[3] = done;  // what the original leaves in r3 when it returns here
        if (done != 0) return;
        const u32 D = kLoadDescs;
        const u16 f124 = load<u16>(D + 124);
        store<u8>(kLoadGfxDone, 1);
        store<u16>(D + 120, 24);
        store<u16>(D + 126, 0);
        store<u16>(D + 124, static_cast<u16>(f124 | 0x8000));
        u32 src = kLoadDescSrc, dst = D;
        for (;;) {
            const u32 b = load<u32>(src + 4);
            if (b == 0) break;
            const u32 a = load<u32>(src);
            store<u32>(dst + 4, b);
            store<u32>(dst, a);
            store<u8>(dst + 8, load<u8>(src + 8));
            const u8 d = load<u8>(src + 9);
            store<u8>(dst + 9, d);
            store<u8>(dst + 10, load<u8>(src + 10));
            const u32 sh = (a + d) & 63;  // slw
            store<u16>(D + 126, static_cast<u16>(load<u16>(D + 126) | (sh < 32 ? 1u << sh : 0u)));
            src += 12;
            dst += 12;
        }
        const u32 a = load<u32>(src);
        store<u32>(dst + 4, 0);
        store<u32>(dst, a);
        store<u8>(dst + 8, load<u8>(src + 8));
        store<u8>(dst + 9, load<u8>(src + 9));
        store<u8>(dst + 10, load<u8>(src + 10));
        const u32 g = static_cast<u32>(call(ctx, kShader_LoadGroup, {4}));
        const u32 data = load<u32>(load<u32>(load<u32>(g + 16) + 12));
        store<u32>(kLoadShaderPtrs, data + 4);
        store<u32>(kLoadShaderPtrs + 4, data + 8);
        const f64 lo = Lfs4(kLoadOrthoK), z = Lfs4(kLoadOrthoK + 4), hi = Lfs4(kLoadOrthoK + 8);
        callf(ctx, kMat4_Ortho, {sp + 112}, {lo, hi, lo, hi, z, hi});
        const u32 G = 0x01164BD0;
        const V4 m0 = Lvx(sp + 112), m1 = Lvx(sp + 128), m2 = Lvx(sp + 144), m3 = Lvx(sp + 160);
        for (u32 r = 0; r < 4; ++r) {
            const V4 g4 = Lvx(G + 16 * r);
            V4 v;
            for (u32 k = 0; k < 4; ++k) {
                f32 x = Vmadd(m0.e[k], g4.e[0], r == 3 ? m3.e[k] : 0.0f);
                x = Vmadd(m1.e[k], g4.e[1], x);
                v.e[k] = Vmadd(m2.e[k], g4.e[2], x);
            }
            Stvx(kLoadMatrix + 16 * r, v);
        }
        store<u32>(kLoadFlags, LoadNewEventFlag(ctx, sp + 208, kStrStartLoading));
        const u32 p2 = LoadNewEventFlag(ctx, sp + 176, kStrContinueThread);
        store<u32>(kLoadFlags + 4, p2);
        ctx.gpr[3] = p2;
        return;
    }
    const u8 done = load<u8>(kLoadInitDone);
    if (done != 0) {
        ctx.gpr[3] = done;
        return;
    }
    store<u8>(kLoadInitDone, 1);
    if (load<u8>(kLoadOpt + 72) != 0) store<u32>(kLoadOptOut, static_cast<u32>(call(ctx, kLoad_Fn44D9E0, {kLoadOptArg})));
    call(ctx, fn::memset, {kLoadState, 0, 220});
    store<u32>(kLoadState, 220);
    store<u32>(kLoadState + 16, 0x3F800000);
    const u32 w = static_cast<u32>(call(ctx, v5fn::Ui_Find, {kStrCommonLoading, 0}));
    store<u32>(kLoadWidgets, w);
    store<u32>(kLoadWidgets + 4, static_cast<u32>(call(ctx, v5fn::Ui_Find, {kStrHintText, w})));
    const u32 top = static_cast<u32>(call(ctx, v5fn::Ui_Find, {kStrTopText, load<u32>(kLoadWidgets)}));
    store<u32>(kLoadWidgets + 8, top);
    ctx.gpr[3] = top;
}
