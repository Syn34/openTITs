// The renderer range's next recompiled functions: the callees gfx12*.cpp
// left (the draw queue's job submit, ...) and the next smallest ones.
// Names carry the address where the role is unclear.
#include <atomic>
#include <bit>
#include <cmath>
#include <vector>

#include "fx.h"
#include "gfx13.h"

using namespace sly4;

namespace {

namespace g11 = sly4::render::gfx11;
namespace g12 = sly4::render::gfx12;
namespace g13 = sly4::render::gfx13;
namespace g10 = sly4::render::gfx10;
using g11::lfs;
using g11::sgl;

// The draw-record list (g12::g::draw_records) set up on first use, as
// DrawRecordList_Reset_0050F9A4 does: u8 +0 = 1, PtrVector +4 (u16 +10
// = 160), +32 = 0, +128 / +256 atomically 0.
void draw_records_init13(PPUContext& ctx) {
    constexpr u32 L = g12::g::draw_records;
    if (load<u8>(g12::g::draw_records_ready) != 0) return;
    store<u8>(L, 1);
    call(ctx, g12::fn::PtrVector_Construct, {L + 4});
    store<u16>(L + 10, 160);
    store<u32>(L + 32, 0);
    std::atomic_ref<u32>(ref<u32>(L + 128)).exchange(0);  // lwarx / stwcx.
    std::atomic_ref<u32>(ref<u32>(L + 256)).exchange(0);
    store<u8>(g12::g::draw_records_ready, 1);
}

// One inlined step of the game's random generator (game/update.h
// Random_Step): the state's mix ((rotl 4 + rotl 16) ^ kRandomMix) gives
// the next 16 bits; the state = rotl(mix, 16), then the last value = the
// 16 bits. Returns the previous last value as f32 (fcfid, frsp).
f64 random_step13() {
    const auto rotl = [](u32 v, int n) { return (v << n) | (v >> (32 - n)); };
    const u64 last = load<u64>(g13::g::random_last);
    const u32 x = load<u32>(g13::g::random_state);
    const u32 mix = (rotl(x, 4) + rotl(x, 16)) ^ g13::kRandomMix;
    store<u32>(g13::g::random_state, rotl(mix, 16));
    store<u64>(g13::g::random_last, mix & 0xFFFF);
    return sgl(static_cast<f64>(static_cast<s64>(last)));
}

// The point (x, y, z) through the 4x4 at m (lvx rows, address rounded
// down to 16) in vmaddfp order: m0 * x + 0, + m1 * y, + m2 * z, then
// (translate) m3 + that (vaddfp).
void through13(u32 m, const f32 in[3], bool translate, f32 out[4]) {
    m &= ~15u;
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(load<f32>(m + 4 * k), in[0], 0.0f);
        v = std::fmaf(load<f32>(m + 16 + 4 * k), in[1], v);
        v = std::fmaf(load<f32>(m + 32 + 4 * k), in[2], v);
        out[k] = translate ? load<f32>(m + 48 + 4 * k) + v : v;
    }
}

// ApeTListAllocator<T>::GetAllocator() inlined, as gfx12.cpp's
// get_list12: the free list at slot +0, built on first use as a 60-byte
// ApeTFreeList {vtable vt, +4..+52 cleared, bit 0x8000 of u16 +54
// cleared, +56 the signature name} and FreeList_Init(list, elem_size,
// slot +4 heap, 0, 0); a failed allocation leaves it null.
u32 get_list13(PPUContext& ctx, u32 slot, u32 vt, u32 name, u32 elem_size) {
    u32 fl = load<u32>(slot);
    if (fl == 0) {
        const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {60, 0, 16}));
        if (p != 0) {
            const u32 heap = load<u32>(slot + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(p + off, 0);
            const u32 h = load<u16>(p + 54) & ~0x8000u;
            store<u16>(p + 52, 0);
            store<u16>(p + 54, static_cast<u16>(h));
            store<u32>(p, vt);
            store<u32>(p + 56, name);
            call(ctx, g12::fn::FreeList_Init, {p, elem_size, heap, 0, 0});
        }
        fl = p;
        store<u32>(slot, fl);
    }
    return fl;
}

// The allocator holder's load callback (+8 OPD, +12 its argument), when
// set: (obj, argument, phase), r6 = the entry address as the sequence
// leaves it.
void list_callback13(PPUContext& ctx, u32 holder_ptr, u32 obj, u32 phase) {
    const u32 h = load<u32>(holder_ptr);
    if (const u32 cb = load<u32>(h + 8); cb != 0) call_ptr(ctx, cb, {obj, load<u32>(h + 12), phase, load<u32>(cb)});
}

}  // namespace

// void DrawQueue_Submit_0051669C(Batch* q)   (DrawQueue_Build_00516970; name?)
// Hands the batch's copies (q +12 first .. +16 last, *(q +20) of them)
// to a job: *(q +24) = the copy stride ((last - first) / (count - 1),
// 352 for one); a 224-byte job record from the frame stack: +0 the
// command buffer object (g13::g::command_buffer), +4..+12 a reserved
// call slot (CmdBuf_ReserveCall_00537DD4), +16..+76 the current view's
// matrix (draw::g::current_surface +672 +880, raw), +80..+207 the 128
// bytes the draw-record list's +28 points to (lvx / stvx: both addresses
// rounded down to 16), +208 the list's +256 counter; then
// WorkQueue_Submit56E710(q +12, q +20, q +24, job). The list is set up
// on first use (checked again before the copy, as the original reloads
// the guard). Leaves f1 as the last matrix word.
DECOMP(0051669C, DrawQueue_Submit_0051669C) {
    const u32 q = arg<u32>(ctx, 0);
    const u32 count = load<u32>(load<u32>(q + 20));
    u32 stride = g13::kDrawCopySize;
    if (count > 1) stride = (load<u32>(q + 16) - load<u32>(q + 12)) / (count - 1);  // divwu
    store<u32>(load<u32>(q + 24), stride);
    const u32 job = static_cast<u32>(call(ctx, g12::fn::MemStack_Alloc, {load<u32>(g12::g::frame_stack), 224}));
    store<u32>(job, load<u32>(g13::g::command_buffer));
    {
        Stack frame(ctx, 16);  // the original's sp+112..123
        const u32 slot = frame.addr();
        call(ctx, g13::fn::CmdBuf_ReserveCall_00537DD4, {slot});
        for (u32 k = 0; k < 12; k += 4) store<u32>(job + 4 + k, load<u32>(slot + k));
    }
    const u32 m = load<u32>(load<u32>(draw::g::current_surface) + 672) + 880;
    for (u32 k = 0; k < 64; k += 4) store<u32>(job + 16 + k, load<u32>(m + k));  // lfs / stfs: raw
    ctx.fpr[1] = lfs(m + 60);
    draw_records_init13(ctx);
    {
        const u32 src = load<u32>(g12::g::draw_records + 28) & ~15u, dst = (job + 80) & ~15u;
        for (u32 h = 0; h < 128; h += 64) {  // four lvx, then four stvx
            u32 w[16];
            for (u32 k = 0; k < 16; ++k) w[k] = load<u32>(src + h + 4 * k);
            for (u32 k = 0; k < 16; ++k) store<u32>(dst + h + 4 * k, w[k]);
        }
    }
    draw_records_init13(ctx);
    store<u32>(job + 208, g12::g::draw_records + 256);
    call(ctx, g13::fn::WorkQueue_Submit56E710, {load<u32>(q + 12), load<u32>(q + 20), load<u32>(q + 24), job});
}

// AglRenderTarget* -> the post-effect chain RenderTarget_Setup_005203A8
// creates (6848 bytes, 16-aligned, PostChain_Construct_0051F850, vtable
// vt_post_chain).
// void RenderTarget_Setup_005203A8(RenderTarget* rt)   (RenderTarget_Construct,
// RenderTarget_Init_00521110; name?)
// The render target's defaults after its base is built: +668 = 0;
// (ThreadId_005DB070, its result unused); u8 +49 = 1, +51 = 0, +50 &=
// 0x0F, word +4 bits 0xFC00 = 5 (<< 10); f32 +56 0.96 (0x3F75BE0B), +60
// max(1, 0.1) (fsel), +64 100, +68 / +72 1; u8 +52 bit 0x10 cleared and
// 0x80 set; two 64-byte records +128 / +192 {u8 +0 0, u8 +2..+5 255, 0,
// 255, 0, f32 +8 999, +12 1000, +16 / +24 255 / 255, +20 / +28 0};
// RenderTarget_SetMatrix(identity) and its rows' xyz copied to
// +688.. (16-byte stride, raw); u8 +52 bit 0x20 = !Matrix_NearlyEqual(
// +688, identity, 1e-5); the viewport {0, 0, surface +56, +60}
// (RenderTarget_SetViewport); +80 / +84 = 0, f32 +88 / +92 the surface
// size; f32 +112 = config +16 (raw), u8 +52 |= 0x80; name "RenderTarget"
// (+16, 32); the two callback lists' u8 +64 (+584 / +652) and +664 = 0;
// +656 a new post-effect chain (0 when Mem_Alloc fails), +660 its draw
// callback OPD (Call_0051A6A0), +256 = -1. Returns the chain.
DECOMP(005203A8, RenderTarget_Setup_005203A8) {
    const u32 rt = arg<u32>(ctx, 0);
    constexpr u32 C = g13::kRtSetupConsts;
    store<u32>(rt + 668, 0);
    ctx.gpr[3] = 0x00520000u;  // lis r3 the original leaves (ThreadId takes none)
    call(ctx, g13::fn::ThreadId_005DB070, {});
    u32 w48 = load<u32>(rt + 48);
    const u32 w4 = load<u32>(rt + 4);
    w48 = (w48 & ~0x00FF00FFu) | 0x00010000u;  // byte 49 = 1, byte 51 = 0
    store<u8>(rt + 49, 1);
    store<u8>(rt + 51, 0);
    w48 &= ~0x0000E000u;
    store<u32>(rt + 4, (w4 & ~0xFC00u) | (5u << 10));
    store<u32>(rt + 48, w48);
    store<u32>(rt + 56, 0x3F75BE0Bu);  // 0.96
    store<u8>(rt + 50, (w48 >> 8) & 0x0F);
    const f64 one = lfs(C), tenth = lfs(C + 4);
    const f64 d = sgl(one - tenth);
    const u8 b52 = load<u8>(rt + 52);
    store<u32>(rt + 64, 0x42C80000u);  // 100
    store<u8>(rt + 52, (b52 & ~0x10) | 0x80);
    store<u32>(rt + 68, 0x3F800000u);  // 1
    store<u32>(rt + 72, 0x3F800000u);
    store<u32>(rt + 60, g11::stfs_bits(d >= 0.0 ? one : tenth));  // fsel
    const f64 scale = lfs(C + 8);  // 1 / 255
    const f64 hi = sgl(255.0 * scale), lo = sgl(0.0 * scale);
    for (u32 i = 0; i < 2; ++i) {
        const u32 e = rt + 128 + 64 * i;
        store<u8>(e, 0);
        store<u8>(e + 2, 255);
        store<u8>(e + 3, 0);
        store<u8>(e + 4, 255);
        store<u8>(e + 5, 0);
        store<u32>(e + 16, g11::stfs_bits(hi));
        store<u32>(e + 20, g11::stfs_bits(lo));
        store<u32>(e + 24, g11::stfs_bits(hi));
        store<u32>(e + 28, g11::stfs_bits(lo));
        store<u32>(e + 8, 0x4479C000u);   // 999
        store<u32>(e + 12, 0x447A0000u);  // 1000
    }
    constexpr u32 I = g11::g::identity;
    call(ctx, g13::fn::RenderTarget_SetMatrix, {rt, I});
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(rt + 688 + 16 * r + k, load<u32>(I + 16 * r + k));  // lfs / stfs: raw
    ctx.gpr[5] = C;  // the r5 the original passes (unused)
    callf(ctx, g13::fn::Matrix_NearlyEqual, {rt + 688, I}, {lfs(C + 12)});
    const u32 eq = static_cast<u32>(ctx.gpr[3]);
    const u32 surf = load<u32>(rt + 512);
    store<u8>(rt + 52, (load<u8>(rt + 52) & ~0x20) | (eq == 0 ? 0x20 : 0));  // cntlzw & 0x20
    {
        Stack frame(ctx, 16);  // the ViewRect at the original's sp+112
        const u32 v = frame.addr();
        const u32 w = load<u32>(surf + 56), h = load<u32>(surf + 60);
        store<u16>(v, 0);
        store<u16>(v + 2, 0);
        store<u16>(v + 4, static_cast<u16>(w));
        store<u16>(v + 6, static_cast<u16>(h));
        call(ctx, sly4::render::gfx9::fn::RenderTarget_SetViewport, {rt, v});
    }
    const u32 surf2 = load<u32>(rt + 512);
    const s32 w = load<s32>(surf2 + 56), h = load<s32>(surf2 + 60);
    store<u32>(rt + 80, 0);
    store<u32>(rt + 84, 0);
    const u8 b = load<u8>(rt + 52);
    store<u32>(rt + 88, g11::stfs_bits(sgl(static_cast<f64>(w))));  // fcfid, frsp
    store<u32>(rt + 92, g11::stfs_bits(sgl(static_cast<f64>(h))));
    store<u8>(rt + 52, b | 0x80);
    store<u32>(rt + 112, load<u32>(sly4::g::config + 16));  // lfs / stfs: raw
    ctx.fpr[1] = lfs(sly4::g::config + 16);  // the f1 the original passes on
    call(ctx, fn::String_Copy, {rt + 16, g13::g::str_render_target, 32});
    store<u8>(rt + 520 + 64, 0);
    store<u8>(rt + 588 + 64, 0);
    store<u32>(rt + 664, 0);
    u32 chain = static_cast<u32>(call(ctx, fn::Mem_Alloc, {6848, 0, 16}));
    if (chain != 0) {
        call(ctx, g13::fn::PostChain_Construct_0051F850, {chain});
        store<u32>(chain, g13::vt_post_chain);
    }
    store<u32>(rt + 656, chain);
    store<u32>(rt + 660, g13::g::opd_post_chain_draw);
    store<u32>(rt + 256, 0xFFFFFFFFu);
    ctx.gpr[4] = g13::g::opd_post_chain_draw;  // as the original leaves them
    ctx.gpr[5] = 0xFFFFFFFFFFFFFFFFull;
    ret(ctx, chain);
}

// void Display_Prepare_005358A8(u32 msaa)   (RenderState_Invalidate_0051038C,
// View_Restore_00518574, f_00518E78; name?)
// When the display (draw::g::display) has +52 bits 22..24 above 1, u8
// +137 set and u8 +138 clear: PostProc_Init, anti-aliasing off (0, 0, 0,
// 0xFFFF), the display's targets bound (Display_GetTargets, then
// Draw_BindTargets(*depth, *colour, 0)), PostProc_ResolveMsaa(display,
// msaa), Draw_ResetState(1, 0), Display_ResolveTo_00534BAC(display, 0),
// the colour bound as texture 1; inline: blend, alpha test off, depth
// mask = render state u8 +93 bit 0, depth test = u8 +93, depth func
// ALWAYS, stencil and cull off; the quad drawn (Draw_PrepareQuad,
// Draw_RestoreDepth), then depth test u8 +93 again and depth func LEQUAL.
// Last the render state (fx::g::state) is reset: +4 / +16..+28 / +36..+88
// = 0, +8 / +216 = -1, u16 +32 = 0xFFFF, u8 +220 / +221 = 0, +92 bit 31
// cleared, the stencil defaults (+160 / +188 ALWAYS, ref 0, masks -1, ops
// KEEP), u8 +93 = 0, +96..+103 {1, 0, 8, 1, 0, 0, 0, 0}, +104..+116 = 0,
// +120..+132 = 64, u8 +136 = 2, +137 = 0, +140 = 0, +144 = 64, u16 +148 /
// +150 = 64, +152 / +154 = 0. Returns r3 as the original leaves it.
DECOMP(005358A8, Display_Prepare_005358A8) {
    const u32 msaa = arg<u32>(ctx, 0);
    const u32 d = load<u32>(draw::g::display);
    const s32 mode = static_cast<s32>((load<u32>(d + 52) >> 22) & 7);
    if (mode <= 1) return ret(ctx, static_cast<u32>(mode));
    if (load<u8>(d + 137) == 0) return ret(ctx, 0u);
    if (const u8 b = load<u8>(d + 138); b != 0) return ret(ctx, static_cast<u32>(b));
    constexpr u32 S = fx::g::state;
    call(ctx, post::fn::PostProc_Init, {0});  // r3 = the u8 +138 tested (0)
    call(ctx, g13::fn::cellGcmSetAntiAliasingControl, {load<u32>(S), 0, 0, 0, 0xFFFF});
    {
        Stack frame(ctx, 16);  // the original's sp+112 (colour) / +116 (depth)
        const u32 t = frame.addr();
        store<u32>(t, 0);
        store<u32>(t + 4, 0);
        call(ctx, fx::fn::Display_GetTargets, {t, t + 4});
        call(ctx, fx::fn::Draw_BindTargets, {load<u32>(load<u32>(t + 4)), load<u32>(load<u32>(t)), 0});
    }
    call(ctx, fx::fn::PostProc_ResolveMsaa, {d, msaa});
    call(ctx, draw::fn::Draw_ResetState, {1, 0});
    call(ctx, g13::fn::Display_ResolveTo_00534BAC, {d, 0});
    call(ctx, g13::fn::Display_BindAsTexture_00534DB4, {d, 1});
    constexpr u32 kAlways = 0x0207, kLequal = 0x0203, kKeep = 0x1E00, kAll = 0xFFFFFFFFu;
    effect::emit({fx::kBlendEnable, 0});
    effect::emit({fx::kAlphaTestEnable, 0});
    effect::emit({fx::kDepthMask, (load<u8>(S + 93) & 1u) != 0 ? 1u : 0u});
    effect::emit({fx::kDepthTestEnable, load<u8>(S + 93)});
    effect::emit({fx::kDepthFunc, kAlways});
    effect::emit({fx::kStencilTestEnable, 0});
    effect::emit({fx::kCullFaceEnable, 0});
    call(ctx, draw::fn::Draw_PrepareQuad);
    call(ctx, draw::fn::Draw_RestoreDepth);
    effect::emit({fx::kDepthTestEnable, load<u8>(S + 93)});
    effect::emit({fx::kDepthFunc, kLequal});
    store<u32>(S + 4, 0);
    store<u32>(S + 8, kAll);
    for (u32 off = 16; off <= 28; off += 4) store<u32>(S + off, 0);
    store<u16>(S + 32, 0xFFFF);
    store<u32>(S + 88, 0);
    for (u32 off = 52; off <= 80; off += 4) store<u32>(S + off, 0);
    store<u32>(S + 216, kAll);
    store<u8>(S + 220, 0);
    store<u8>(S + 221, 0);
    const u32 w92 = load<u32>(S + 92) & 0x7FFFFFFFu;
    store<u32>(S + 160, kAlways);
    store<u32>(S + 164, 0);
    store<u32>(S + 168, kAll);
    store<u32>(S + 92, w92);
    store<u32>(S + 172, kAll);
    for (u32 off = 176; off <= 184; off += 4) store<u32>(S + off, kKeep);
    store<u32>(S + 188, kAlways);
    store<u32>(S + 192, 0);
    store<u32>(S + 196, kAll);
    store<u32>(S + 200, kAll);
    for (u32 off = 204; off <= 212; off += 4) store<u32>(S + off, kKeep);
    for (u32 off = 36; off <= 48; off += 4) store<u32>(S + off, 0);
    store<u32>(S + 84, 0);
    static constexpr u8 kBytes96[8] = {1, 0, 8, 1, 0, 0, 0, 0};
    store<u8>(S + 93, 0);
    for (u32 k = 0; k < 8; ++k) store<u8>(S + 96 + k, kBytes96[k]);
    for (u32 off = 104; off <= 116; off += 4) store<u32>(S + off, 0);
    for (u32 off = 120; off <= 132; off += 4) store<u32>(S + off, 64);
    store<u8>(S + 136, 2);
    store<u8>(S + 137, 0);
    store<u32>(S + 140, 0);
    store<u32>(S + 144, 64);
    store<u16>(S + 148, 64);
    store<u16>(S + 150, 64);
    store<u16>(S + 152, 0);
    store<u16>(S + 154, 0);
    ret(ctx, 2u);  // the loop count li r3, 2 the original leaves
}

// void Surface_Bind_0053B668(RenderSurface* s, bool clear)
// (RenderState_Invalidate_0051038C, Bind_00520030, f_00522140,
// Call_0053BBE0; name?)
// Makes s the render target. For the display (draw::g::display), by its
// +52 bits 22..24 mode: 1 binds Display_GetTargets' pair; 2 and 4 bind
// that pair too while u8 +136 is set, +137 clear or +138 set, otherwise
// the resolve pair (+80 / +76, or +132 / +128 with config u8 +34) and
// anti-aliasing on (1, 0, 0, 0xFFFF); 0, 3 and above 4 bind nothing.
// Another surface turns anti-aliasing off (0, 0, 0, 0xFFFF) and binds
// (depth, colour) picked by +52 bit 25 (multisampled) and the buffer
// index i = +52 bits 30..31: plain {+24, +16 + 4i}; multisampled with
// config u8 +35 or +34 clear {+88, +84} when u8 +148 bit 7 is set, else
// {+72, +64 + 4i}; otherwise by the screen's +184 (1: {+120, +100 + 4i},
// else {+124, +108 + 4i}). Then inline the colour mask (+48), depth test
// (render state u8 +93) and LEQUAL; the viewport from
// Surface_DefaultRects_0051AE24's first rect {x, y, w, h} (scale {w / 2,
// -h / 2, 1, 0}, offset {x + w / 2, y + h / 2, 0, 0}, depth 0..1) and
// the scissor from the same rect. With clear and a non-zero +44 clear
// mask: clear colour 0xFF << 24 | u8 +32 << 16 | +33 << 8 | +34, depth /
// stencil 0xFFFFFF00, stencil mask 255, clear (+44), then the stencil
// mask back to the render state's +168. Returns r3 as the original
// leaves it (the gcm context, or the last stencil mask call's).
DECOMP(0053B668, Surface_Bind_0053B668) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 clear = arg<u32>(ctx, 1);
    constexpr u32 S = fx::g::state;
    constexpr u32 kAll = 0xFFFF;
    Stack frame(ctx, 64);  // the original's sp+128..183
    const u32 sp = frame.addr() - 128;
    auto bind_display_pair = [&](u32 t) {
        store<u32>(t, 0);
        store<u32>(t + 4, 0);
        call(ctx, fx::fn::Display_GetTargets, {t, t + 4});
        call(ctx, fx::fn::Draw_BindTargets, {load<u32>(load<u32>(t + 4)), load<u32>(load<u32>(t)), 0});
    };
    if (s == load<u32>(draw::g::display)) {
        const s32 mode = static_cast<s32>((load<u32>(s + 52) >> 22) & 7);
        if (mode == 1) {
            bind_display_pair(sp + 144);
        } else if (mode == 2 || mode == 4) {
            if (load<u8>(s + 136) != 0 || load<u8>(s + 137) == 0 || load<u8>(s + 138) != 0) {
                bind_display_pair(sp + 136);
            } else {
                const bool alt = load<u8>(sly4::g::config + 34) != 0;
                const u32 colour = alt ? s + 128 : s + 76, depth = alt ? s + 132 : s + 80;
                call(ctx, fx::fn::Draw_BindTargets, {load<u32>(depth), load<u32>(colour), 0});
                call(ctx, g13::fn::cellGcmSetAntiAliasingControl_005E2590, {load<u32>(S), 1, 0, 0, kAll});
            }
        }
    } else {
        call(ctx, g13::fn::cellGcmSetAntiAliasingControl, {load<u32>(S), 0, 0, 0, kAll});
        const u32 w = load<u32>(s + 52);
        const bool ms = ((w >> 25) & 1) != 0;
        const u32 i4 = ((w >> 30) & 3) * 4;
        u32 colour, depth;
        if (!ms) {
            colour = s + 16 + i4;
            depth = s + 24;
        } else if (load<u8>(sly4::g::config + 35) == 0 || load<u8>(sly4::g::config + 34) == 0) {
            const bool resolved = ((load<u8>(s + 148) >> 7) & 1) != 0;
            colour = resolved ? s + 84 : s + 64 + i4;
            depth = resolved ? s + 88 : s + 72;
        } else {
            const bool one = load<u32>(g12::g::screen + 184) == 1;
            colour = s + i4 + (one ? 100 : 108);
            depth = one ? s + 120 : s + 124;
        }
        call(ctx, fx::fn::Draw_BindTargets, {load<u32>(depth), load<u32>(colour), 0});
    }
    effect::emit({fx::kColorMask, load<u32>(s + 48)});
    effect::emit({fx::kDepthTestEnable, load<u8>(S + 93)});
    effect::emit({fx::kDepthFunc, 0x0203});  // LEQUAL
    const u32 rects = sp + 128;
    store<u32>(rects, 0);
    store<u32>(rects + 4, 0);
    call(ctx, g11::fn::Surface_DefaultRects_0051AE24, {s, rects, rects + 4});
    const u32 r = load<u32>(rects);
    const u32 w = load<u16>(r + 4), h = load<u16>(r + 6), x = load<u16>(r), y = load<u16>(r + 2);
    constexpr u32 K = g13::kSurfaceBindConsts;
    const f64 half = lfs(K);
    const f64 sx = sgl(sgl(static_cast<f64>(w)) * half);
    const f64 fh = sgl(static_cast<f64>(h));
    const u32 scale = sp + 152, offset = sp + 168;
    store<u32>(scale, g11::f32_bits(sx));
    store<u32>(scale + 4, g11::f32_bits(sgl(fh * lfs(K + 4))));
    store<u32>(scale + 8, 0x3F800000u);
    store<u32>(scale + 12, 0);
    store<u32>(offset + 8, 0);
    store<u32>(offset + 12, 0);
    store<u32>(offset, g11::f32_bits(sgl(sgl(static_cast<f64>(x)) + sx)));
    store<u32>(offset + 4, g11::f32_bits(g11::madds(fh, half, sgl(static_cast<f64>(y)))));
    // r8 / r9 are the float arguments' unused GPR slots (r8 holds y, as left).
    callf(ctx, g11::fn::Gcm_SetViewport, {load<u32>(S), x, y, w, h, y, ctx.gpr[9], scale, offset}, {lfs(K + 12), lfs(K + 8)});
    effect::emit({0x000808C0u /* SET_SCISSOR_HORIZONTAL, 2 */, (w << 16) | x, (h << 16) | y});
    u64 r3 = load<u32>(S);
    if ((clear & 0xFF) != 0) {
        if (const u32 mask = load<u32>(s + 44); mask != 0) {
            const u32 argb = 0xFF000000u | static_cast<u32>(load<u8>(s + 32)) << 16 | static_cast<u32>(load<u8>(s + 33)) << 8 | load<u8>(s + 34);
            call(ctx, fx::fn::cellGcmSetClearColor, {load<u32>(S), argb});
            call(ctx, fx::fn::cellGcmSetClearDepthStencil, {load<u32>(S), 0xFFFFFF00u});
            call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(S), 255});
            call(ctx, fx::fn::cellGcmSetClearSurface, {load<u32>(S), mask});
            r3 = call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(S), load<u32>(S + 168)});
        }
    }
    ret(ctx, r3);
}

// bool LoadParams_005AF298(ShaderType*, Material* m, Desc* d)   (gfx12.h: Shader_IsReady_005AF298)
// (Thunk_005AF4EC, f_005B6528, LoadParams_005D2F20, f_005D4198,
// f_005D5C80; name?)
// The shared start of these LoadParams: the texture named d+4
// (Tex_FindLoaded; none: returns 0) gets virtual slot 4 (+16) (t, 1, 1,
// 1) and is added to m (Material_AddTexture_004FF008); params 0..7 =
// d+88..+116 (raw). Params 8 / 9: with the desc version (u8 d+84) above
// 8, d+120 and d+124 (degrees) are first turned to radians in place
// (* kDegToRad) and then copied; otherwise 0. Returns 1.
DECOMP(005AF298, LoadParams_005AF298) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 4)}));
    if (t == 0) return ret(ctx, 0u);
    fx::vcall(ctx, t, 16, {t, 1, 1, 1});
    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t});
    auto slot = [&](u32 k) { return static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, k})); };
    for (u32 k = 0; k < 8; ++k) {
        const u32 v = load<u32>(d + 88 + 4 * k);  // lfs / stfs: raw
        store<u32>(slot(k), v);
    }
    if (static_cast<s32>(load<u8>(d + 84)) > 8) {
        const f64 c = lfs(g13::kDegToRad);
        for (u32 k = 0; k < 2; ++k) {
            const u32 p = d + 120 + 4 * k;
            const f64 v = sgl(lfs(p) * c);  // fmuls
            store<u32>(p, g11::stfs_bits(v));
            if (k == 1) ctx.fpr[1] = v;  // the f1 the original leaves
            const u32 w = load<u32>(p);  // lfs / stfs: raw
            store<u32>(slot(8 + k), w);
        }
    } else {
        store<u32>(slot(8), 0);
        store<u32>(slot(9), 0);
    }
    ret(ctx, 1u);
}

// bool Render_Shutdown_0050FB5C()   (Game_ShutdownSystems_005DB4BC; name?)
// Tears the renderer down: Call_005489CC, DrawRecordList_Reset_0050F9A4
// (and the list's second guard byte set); every object of the list at
// g13::g::object_list_f66fd0 (u8 +20 of them at +4..) gets virtual slot
// 1 (+4) and its slot is cleared, the count zeroed, the list itself
// gets slot 1 and the pointer is cleared; the stencil shadow pool's part
// +12 destructed; TopoShared_Release(*pool_vector); u8 block_ddb88 +32
// = 0; block_f66fb8 freed; Call520028_00546F3C; the frame block's
// (block_dd600) +12 / +16 freed (pool 0), +264 / +268 freed (pool 1)
// and cleared, +280 gets slot 1 and is cleared; alloc_ddc88 and the
// render-target allocator (g12::g::render_targets) freed and cleared;
// RenderJobs_Shutdown_0053B558. Returns 1.
DECOMP(0050FB5C, Render_Shutdown_0050FB5C) {
    namespace l3 = sly4::render::layers3;
    namespace g8 = sly4::render::gfx8;
    call(ctx, g13::fn::Call_005489CC);
    call(ctx, g13::fn::DrawRecordList_Reset_0050F9A4);
    if (load<u8>(g12::g::draw_records_guard2) == 0) store<u8>(g12::g::draw_records_guard2, 1);
    constexpr u32 H = g13::g::object_list_f66fd0;
    u32 h = load<u32>(H);
    if (static_cast<s32>(load<u8>(h + 20)) > 0) {
        u32 i = 0;
        do {
            if (const u32 e = load<u32>(h + 4 + 4 * i); e != 0) {
                fx::vcall(ctx, e, 4, {e});
                store<u32>(load<u32>(H) + 4 + 4 * i, 0);
                h = load<u32>(H);
            }
            i = (i + 1) & 0xFF;
        } while (static_cast<s32>(i) < static_cast<s32>(load<u8>(h + 20)));
    }
    store<u8>(h + 20, 0);
    if (const u32 l = load<u32>(H); l != 0) {
        fx::vcall(ctx, l, 4, {l});
        store<u32>(H, 0);
    }
    call(ctx, sly4::render::gfx9::fn::Pool_DestructPart, {l3::g::pool + 12});
    call(ctx, g13::fn::TopoShared_Release, {load<u32>(load<u32>(l3::g::pool_vector))});
    const u32 blk = load<u32>(g13::g::block_f66fb8);
    store<u8>(l3::g::block_ddb88 + 32, 0);
    call(ctx, g8::fn::Mem_FreeFrom, {blk, 0});
    call(ctx, g13::fn::Call520028_00546F3C);
    constexpr u32 F = l3::g::block_dd600;
    call(ctx, g8::fn::Mem_FreeFrom, {load<u32>(F + 12), 0});
    call(ctx, g8::fn::Mem_FreeFrom, {load<u32>(F + 16), 0});
    for (u32 off : {264u, 268u}) {
        if (const u32 p = load<u32>(F + off); p != 0) {
            call(ctx, g8::fn::Mem_FreeFrom, {p, 1});
            store<u32>(F + off, 0);
        }
    }
    if (const u32 o = load<u32>(F + 280); o != 0) {
        fx::vcall(ctx, o, 4, {o});
        store<u32>(F + 280, 0);
    }
    for (u32 a : {g13::g::alloc_ddc88, g12::g::render_targets}) {
        if (const u32 p = load<u32>(a); p != 0) {
            call(ctx, g8::fn::Mem_FreeFrom, {p, 0});
            store<u32>(a, 0);
        }
    }
    call(ctx, g13::fn::RenderJobs_Shutdown_0053B558);
    ret(ctx, 1u);
}

// void Skin_LimitWeights_004FCD78(Mesh* m, s32 bones)   (Iff_HandleSkinWeights_00525A24;
// gfx12.h: Mesh_SetFlag_004FCD78; name?)
// Cuts every vertex's skin record (m+40 of them, 8 bytes each from
// m+72: u8 bone indices +0..+3, u8 weights +4..+7) down to `bones`
// influences (nothing for 4): with fewer than 2 / 3 / 4 the index and
// weight of influence 1 / 2 / 3 are zeroed. Weights 1..3 below 4 are
// dropped, then all are renormalised to 255: w_k = w_k * 255 / sum
// (divwu: 0 for a zero sum), w0 = 255 - (w1 + w2 + w3). Returns r3 = m
// for 4, else 0.
DECOMP(004FCD78, Skin_LimitWeights_004FCD78) {
    const u32 m = arg<u32>(ctx, 0);
    const s32 bones = arg<s32>(ctx, 1);
    if (bones == 4) return ret(ctx, m);
    u32 p = load<u32>(m + 72);
    const u32 n = load<u32>(m + 40);
    for (u32 i = 0; i < n; ++i, p += 8) {
        if (bones < 2) {
            store<u8>(p + 1, 0);
            store<u8>(p + 5, 0);
        }
        if (bones < 3) {
            store<u8>(p + 2, 0);
            store<u8>(p + 6, 0);
        }
        if (bones < 4) {
            store<u8>(p + 3, 0);
            store<u8>(p + 7, 0);
        }
        u32 w[4];
        for (u32 k = 3; k >= 1; --k) {
            w[k] = load<u8>(p + 4 + k);
            if (w[k] < 4) w[k] = 0;
        }
        w[0] = load<u8>(p + 4);
        const u32 sum = w[0] + w[1] + w[2] + w[3];
        u32 q[4];
        for (u32 k = 1; k < 4; ++k) q[k] = sum != 0 ? (w[k] * 255) / sum : 0;  // divwu
        for (u32 k = 1; k < 4; ++k) store<u8>(p + 4 + k, static_cast<u8>(q[k]));
        store<u8>(p + 4, static_cast<u8>(255 - (q[1] + q[2] + q[3])));
    }
    ret(ctx, 0u);
}

// void Emitter_Advance_00500908(Emitter* e; f32 dt)   (f_00506340,
// Emitter_Update_007A0C10; name?)
// Steps an emitter (props p = e+4) by t = dt * e+12: e+8 += t; u8 +64
// bit 0x40 cleared. Its phase (+64 bits 3..4) runs with the timer +20:
// 1 (on): +20 -= t; when it runs out the phase becomes 2 and +20 = p+24
// * (1 - p+28 * rnd); 2 (off): +20 -= t; when it runs out the phase
// becomes 1 (bit 0x20 set: just started) and +20 = p+32 * (1 - p+36 *
// rnd). The emission count +16 grows by t * p+16 * (1 - p+20 * rnd)
// while emitting (p u8 +42 bit 4) in phases 0 / 3 and 1 (a phase 1
// that ran out and had bit 0x20 set adds the old timer * p+16 * (...)
// instead; bit 0x20 is cleared in phase 1). rnd = the generator's last
// value / 65535 (random_step13). Then, with a target (+24): +48..+56 =
// the velocity (target +48 xyz - +32 xyz) / t when virtual slot 7 (+28)
// returns non-zero, else 0, and +32..+40 = the target's +48 xyz. Returns
// r3 / f1 as the original leaves them.
DECOMP(00500908, Emitter_Advance_00500908) {
    const u32 e = arg<u32>(ctx, 0);
    constexpr u32 K = g13::kEmitterConsts;
    const f64 t = sgl(farg(ctx, 0) * lfs(e + 12));
    const u32 p = load<u32>(e + 4);
    u32 b = load<u8>(e + 64) & ~0x40u;
    store<u8>(e + 64, static_cast<u8>(b));
    store<u32>(e + 8, g11::stfs_bits(sgl(lfs(e + 8) + t)));
    const f64 zero = lfs(K);
    const s32 phase = static_cast<s32>((b >> 3) & 3);
    auto rnd = [&] { return sgl(random_step13() * lfs(K + 4)); };
    auto emitting = [&] { return ((load<u8>(p + 42) >> 4) & 1) != 0; };
    auto rate = [&](f64 scale) {  // scale * p+16 * (1 - p+20 * rnd)
        const f64 a = sgl(scale * lfs(p + 16));
        const f64 r = rnd();
        return sgl(a * g11::nmsubs(lfs(p + 20), r, lfs(K + 8)));
    };
    f64 add = zero;
    if (phase == 1) {
        const f64 left = lfs(e + 20);
        const bool started = ((b >> 5) & 1) != 0;
        b &= ~0x20u;
        const f64 rest = sgl(left - t);
        if (!(rest <= zero)) {  // fcmpu, cror lt | eq (NaN: still running)
            store<u32>(e + 20, g11::stfs_bits(rest));
            store<u8>(e + 64, static_cast<u8>(b));
            if (emitting()) add = rate(t);
        } else {
            b = (b & ~0x18u) | (2u << 3);
            store<u8>(e + 64, static_cast<u8>(b));
            const f64 r = rnd();
            store<u32>(e + 20, g11::stfs_bits(sgl(lfs(p + 24) * g11::nmsubs(lfs(p + 28), r, lfs(K + 8)))));
            if (started && emitting()) {
                const f64 a = sgl(left * lfs(p + 16));  // the timer before the step
                const f64 r2 = rnd();
                add = sgl(a * g11::nmsubs(lfs(p + 20), r2, lfs(K + 8)));
            }
        }
    } else if (phase == 2) {
        const f64 rest = sgl(lfs(e + 20) - t);
        store<u32>(e + 20, g11::stfs_bits(rest));
        if (rest <= zero) {  // cror lt | eq (NaN: still running)
            b = (b & ~0x18u) | (1u << 3) | 0x20u;
            store<u8>(e + 64, static_cast<u8>(b));
            const f64 r = rnd();
            store<u32>(e + 20, g11::stfs_bits(sgl(lfs(p + 32) * g11::nmsubs(lfs(p + 36), r, lfs(K + 8)))));
        }
    } else if (emitting()) {
        add = rate(t);
    }
    const f64 count = sgl(lfs(e + 16) + add);
    store<u32>(e + 16, g11::stfs_bits(count));
    if (load<u32>(e + 24) == 0) {
        ctx.fpr[1] = count;
        return ret(ctx, 0u);
    }
    f64 vx = zero, vy = zero, vz = zero;
    if (static_cast<u32>(fx::vcall(ctx, e, 28, {e})) != 0) {
        const f64 inv = sgl(lfs(K + 8) / t);  // fdivs
        const u32 v = load<u32>(e + 24) + 48;
        vx = sgl(sgl(lfs(v) - lfs(e + 32)) * inv);
        vy = sgl(sgl(lfs(v + 4) - lfs(e + 36)) * inv);
        vz = sgl(sgl(lfs(v + 8) - lfs(e + 40)) * inv);
    }
    const u32 v = load<u32>(e + 24) + 48;
    store<u32>(e + 48, g11::stfs_bits(vx));
    store<u32>(e + 52, g11::stfs_bits(vy));
    store<u32>(e + 56, g11::stfs_bits(vz));
    for (u32 k = 0; k < 12; k += 4) store<u32>(e + 32 + k, load<u32>(v + k));  // lfs / stfs: raw
    ctx.fpr[1] = lfs(v + 8);
    ret(ctx, v);
}

// Particle* Emitter_Emit_0079D280(Emitter* e; f32 t)   (f_00506340,
// Emitter_Update_007A0C10; name?)
// Takes the next 36-byte particle from the pool e+72 ({+0 start, +4 end,
// +8 next, +16 capacity, +20 count}: none when count == capacity; next
// wraps to the start at the end) and starts it: the props' (p = e+4)
// shape (p+108) virtual slot 4 (+16) gives a position and direction;
// with a transform (e+24) both go through it (the position translated,
// then moved back by the emitter's velocity e+48.. * t; the direction
// gains the velocity * p+64). Particle: +0 position, +12 = e+8, +16
// direction, then from the random generator (rnd = last / 65535):
// u8 +28 = 127 / (1 - p+12 * rnd1), +29 = 255 * (1 - (p+116)->+12 *
// rnd2) (1 without p+116), +30 = 127 * (1 - (p+124)->+32 * rnd3), times
// a random sign (raw last >= 32768: -1) when u8 (p+124)->+177 is set (1
// without p+124), +31 = 127 * (p+124)->+172 * rnd5 (0 without); +32 the
// generator's state after one more step. Returns the particle (0 when
// none).
DECOMP(0079D280, Emitter_Emit_0079D280) {
    const u32 e = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    const u32 pool = load<u32>(e + 72);
    const u32 count = load<u32>(pool + 20);
    if (count == load<u32>(pool + 16)) return ret(ctx, 0u);
    const u32 cur = load<u32>(pool + 8);
    const u32 next = cur + g13::kParticleSize;
    store<u32>(pool + 20, count + 1);
    store<u32>(pool + 8, static_cast<s32>(next - load<u32>(pool + 4)) < 0 ? next : load<u32>(pool));
    const u32 q = cur;
    if (q == 0) return ret(ctx, 0u);
    Stack frame(ctx, 32);  // the original's sp+112 (position) / +128 (direction)
    const u32 pos = frame.addr(), dir = pos + 16;
    {
        const u32 shape = load<u32>(load<u32>(e + 4) + 108);
        fx::vcall(ctx, shape, 16, {shape, pos, dir});
    }
    const u32 p = load<u32>(e + 4);
    const u32 m = load<u32>(e + 24);
    const f64 age = lfs(e + 8);
    f64 P[3], D[3];
    for (u32 k = 0; k < 3; ++k) P[k] = lfs(pos + 4 * k), D[k] = lfs(dir + 4 * k);
    const f64 spread = lfs(p + 12);
    const u32 tint = load<u32>(p + 116), vary = load<u32>(p + 124);
    if (m != 0) {
        f32 in[3], out[4];
        for (u32 k = 0; k < 3; ++k) in[k] = static_cast<f32>(P[k]);
        through13(m, in, true, out);
        for (u32 k = 0; k < 3; ++k) P[k] = g11::nmsubs(lfs(e + 48 + 4 * k), t, out[k]);
        for (u32 k = 0; k < 3; ++k) in[k] = static_cast<f32>(D[k]);
        through13(m, in, false, out);
        const f64 boost = lfs(p + 64);
        for (u32 k = 0; k < 3; ++k) D[k] = g11::madds(lfs(e + 48 + 4 * k), boost, out[k]);
    }
    constexpr u32 K = g13::kEmitConsts;
    const f64 inv = lfs(K), one = lfs(K + 4);
    auto rnd = [&] { return sgl(random_step13() * inv); };
    const f64 r1 = rnd();
    f64 a = one, b = one, c = lfs(K + 8);
    if (tint != 0) {
        const f64 r = rnd();
        a = g11::nmsubs(lfs(tint + 12), r, one);
    }
    if (vary != 0) {
        const f64 r = rnd();
        b = g11::nmsubs(lfs(vary + 32), r, one);
        if (load<u8>(vary + 177) != 0) {
            const f64 half = lfs(K + 20), neg = lfs(K + 24);
            const f64 raw = random_step13();
            b = sgl(b * (sgl(raw - half) >= 0.0 ? neg : one));  // fsel
        }
        const f64 r5 = rnd();
        c = sgl(r5 * lfs(vary + 172));
    }
    const f64 d = sgl(one / g11::nmsubs(spread, r1, one));  // fdivs
    for (u32 k = 0; k < 3; ++k) store<u32>(q + 4 * k, g11::stfs_bits(P[k]));
    store<u32>(q + 12, g11::stfs_bits(age));
    for (u32 k = 0; k < 3; ++k) store<u32>(q + 16 + 4 * k, g11::stfs_bits(D[k]));
    const f64 k127 = lfs(K + 12), k255 = lfs(K + 16);
    const u64 i28 = rt::fctiw(sgl(d * k127), true);
    store<u8>(q + 28, static_cast<u8>(i28));
    const u64 i29 = rt::fctiw(sgl(a * k255), true);
    store<u8>(q + 29, static_cast<u8>(i29));
    store<u8>(q + 30, static_cast<u8>(rt::fctiw(sgl(b * k127), true)));
    store<u8>(q + 31, static_cast<u8>(rt::fctiw(sgl(c * k127), true)));
    {
        const auto rotl = [](u32 v, int n) { return (v << n) | (v >> (32 - n)); };
        const u32 x = load<u32>(g13::g::random_state);
        const u32 mix = (rotl(x, 4) + rotl(x, 16)) ^ g13::kRandomMix;
        store<u32>(g13::g::random_state, rotl(mix, 16));
        store<u64>(g13::g::random_last, mix & 0xFFFF);
        store<u32>(q + 32, rotl(mix, 16));
    }
    ctx.fpr[1] = std::bit_cast<f64>(i29);  // the fctiwz result left in f1
    ret(ctx, q);
}

// void RenderTarget_Sync_005109B0(RenderTarget* rt)   (f_000AC698,
// RenderTargets_Update_00511F8C; gfx12.h: RenderTarget_Draw_005109B0;
// name?)
// Copies the render target into its render-side copy s (rt+672) field
// by field, as the compiled member-wise assignment does (bit fields one
// by one, each read again after the previous store; lfs / stfs pairs
// raw): u16 +4, word +4 bits 0xFC00, +8 bits 0xFFFFF800 / 0x400 /
// 0x3C0; the reference block +12 (the new one's u16 +0 count up, the old
// one's down: at zero its four refs +16 / +12 / +8 / +4 are released,
// RefBlock_Release, and it is freed); the name +16..+47; u8 +48 / +49,
// +50 bits 0xE0 / 0x10, +51, +52 bits 0x80..0x04; +56..+72, +80..+92,
// u16 +96..+110, +112; the two 64-byte records +128 / +192 (bytes 0..5,
// words 8..60); +256, +272..+495, +496..+516; the two 68-byte callback
// lists +520 / +588; +656..+672; the matrices +688.. (12 rows' xyz),
// +880..+1080, +1088.. (11 rows' xyz), +1264..+1388; +1392..+1775 (lvx
// / stvx: 16-byte aligned); u8 +1776 bits 0x80..0x10; +1784..+1823 (ld /
// std); +1824 / +1828. Last the main-thread-id static is set up if its
// guard is clear. Leaves r3 / f1 as the original does.
DECOMP(005109B0, RenderTarget_Sync_005109B0) {
    const u32 rt = arg<u32>(ctx, 0);
    const u32 s = load<u32>(rt + 672);
    auto raw = [&](u32 off) { store<u32>(s + off, load<u32>(rt + off)); };  // lfs / stfs, lwz / stw
    auto bits8 = [&](u32 off, u32 v, u32 mask) {  // u8 bit field from rt
        v = (v & ~mask) | (load<u8>(rt + off) & mask);
        store<u8>(s + off, static_cast<u8>(v));
        return v;
    };
    store<u16>(s + 4, load<u16>(rt != 0 ? rt + 4 : 0u));  // the base-class adjustment: reads address 0 for a null rt
    store<u32>(s + 4, (load<u32>(s + 4) & ~0xFC00u) | (load<u32>(rt + 4) & 0xFC00u));
    u32 w8 = (load<u32>(s + 8) & 0x7FFu) | (load<u32>(rt + 8) & ~0x7FFu);
    store<u32>(s + 8, w8);
    w8 = (w8 & ~0x400u) | (load<u32>(rt + 8) & 0x400u);
    store<u32>(s + 8, w8);
    w8 = (w8 & ~0x3C0u) | (load<u32>(rt + 8) & 0x3C0u);
    store<u32>(s + 8, w8);
    {
        const u32 old = load<u32>(s + 12), nb = load<u32>(rt + 12);
        if (old != nb) {
            store<u32>(s + 12, nb);
            if (nb != 0) store<u16>(nb, load<u16>(nb) + 1);
            if (old != 0) {
                const u16 n = static_cast<u16>(load<u16>(old) - 1);
                store<u16>(old, n);
                if (static_cast<s16>(n) <= 0) {
                    for (u32 k = 4; k >= 1; --k)
                        if (const u32 r = load<u32>(old + 4 * k); r != 0) call(ctx, g13::fn::RefBlock_Release, {r});
                    call(ctx, sly4::render::gfx8::fn::Mem_FreeFrom, {old, 0});
                }
            }
        }
    }
    for (u32 k = 16; k < 48; ++k) store<u8>(s + k, load<u8>(rt + k));
    {
        u32 w = load<u32>(s + 48);
        const u32 b48 = load<u8>(rt + 48);
        store<u8>(s + 48, static_cast<u8>(b48));
        w = (w & 0x00FFFFFFu) | (b48 << 24);
        const u32 b49 = load<u8>(rt + 49);
        w = (w & 0xFF00FFFFu) | (b49 << 16);
        store<u8>(s + 49, static_cast<u8>(b49));
        w = (w & ~0xE000u) | (load<u32>(rt + 48) & 0xE000u);
        u32 b50 = (w >> 8) & 0xFF;
        store<u32>(s + 48, w);
        b50 = (b50 & ~0x10u) | (load<u8>(rt + 50) & 0x10u);
        u32 b52 = load<u8>(s + 52);
        store<u8>(s + 50, static_cast<u8>(b50));
        store<u8>(s + 51, load<u8>(rt + 51));
        for (u32 m = 0x80; m >= 0x04; m >>= 1) b52 = bits8(52, b52, m);
    }
    for (u32 off : {56u, 60u, 64u, 68u, 72u, 80u, 84u, 88u, 92u}) raw(off);
    for (u32 off = 96; off < 112; off += 2) store<u16>(s + off, load<u16>(rt + off));
    raw(112);
    for (u32 i = 0; i < 2; ++i) {
        const u32 a = rt + 128 + 64 * i, b = s + 128 + 64 * i;
        for (u32 k = 0; k < 6; ++k) store<u8>(b + k, load<u8>(a + k));
        for (u32 k = 8; k < 64; k += 4) store<u32>(b + k, load<u32>(a + k));
    }
    for (u32 off = 256; off < 260; off += 4) raw(off);
    for (u32 off = 272; off < 496; off += 4) raw(off);
    for (u32 off = 496; off < 520; off += 4) raw(off);
    for (u32 i = 0; i < 2; ++i) {
        const u32 a = rt + 520 + 68 * i, b = s + 520 + 68 * i;
        for (u32 k = 0; k < 64; k += 8) {  // lwz pairs, then the stores
            const u32 x = load<u32>(a + k), y = load<u32>(a + k + 4);
            store<u32>(b + k, x);
            store<u32>(b + k + 4, y);
        }
        store<u32>(b + 64, load<u32>(a + 64));
    }
    for (u32 off = 656; off <= 672; off += 4) raw(off);
    for (u32 r = 0; r < 12; ++r)
        for (u32 k = 0; k < 12; k += 4) raw(688 + 16 * r + k);
    for (u32 off = 880; off <= 1080; off += 4) raw(off);
    for (u32 r = 0; r < 11; ++r)
        for (u32 k = 0; k < 12; k += 4) raw(1088 + 16 * r + k);
    for (u32 off = 1264; off <= 1388; off += 4) raw(off);
    for (u32 base : {1392u, 1520u, 1648u}) {  // lvx / stvx, 64 bytes at a time
        const u32 a = (rt + base) & ~15u, b = (s + base) & ~15u;
        for (u32 h = 0; h < 128; h += 64) {
            u32 w[16];
            for (u32 k = 0; k < 16; ++k) w[k] = load<u32>(a + h + 4 * k);
            for (u32 k = 0; k < 16; ++k) store<u32>(b + h + 4 * k, w[k]);
        }
    }
    {
        u32 b = load<u8>(s + 1776);
        for (u32 m = 0x80; m >= 0x10; m >>= 1) b = bits8(1776, b, m);
    }
    {
        u64 d[4];
        for (u32 k = 0; k < 4; ++k) d[k] = load<u64>(rt + 1784 + 8 * k);
        for (u32 k = 0; k < 4; ++k) store<u64>(s + 1784 + 8 * k, d[k]);
        store<u64>(s + 1816, load<u64>(rt + 1816));
    }
    raw(1824);
    raw(1828);
    ctx.fpr[1] = lfs(rt + 1828);
    u32 r3 = load<u8>(g13::g::main_thread_guard);
    if (r3 == 0) {
        store<u64>(g13::g::main_thread, 0);
        store<u64>(g13::g::main_thread + 8, 0);
        store<u8>(g13::g::main_thread_guard, 1);
        r3 = 0x011E0000u;  // lis r3 the original leaves
    }
    ret(ctx, r3);
}

// void EffectSlots_Cull_005AC280(View* v)   (World_SubmitFrame; name?)
// The current effect_pair count (effect_pair[effect_index]) = 0; when
// effects are on (u8 effects_on): View_Update(v, 0), then each effect
// slot (g11::g::effect_slots, 200 bytes, count re-read each step) whose
// object (+8) lies within the slot's radius (+20) of the view's camera
// (v+864; the distance against the object +240) and whose box (the
// object's +208 -> +16 min / +32 max: centre +240, half extents (max -
// min) * 0.5, axes +80) passes Sphere_FrustumFine(v+272, ...): with an
// owner (+0) only u8 effect_global_seen is set, else
// EffectSlot_Submit_005AB908(v, slot). Leaves r3 / f1 as the original
// does.
DECOMP(005AC280, EffectSlots_Cull_005AC280) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 i4 = load<u32>(g13::g::effect_index) << 2;
    store<u32>(g11::g::effect_pair + i4, 0);
    if (load<u8>(g13::g::effects_on) == 0) return ret(ctx, i4);
    call(ctx, g12::fn::View_Update, {v, 0});
    if (load<u32>(g11::g::effect_count) == 0) return ret(ctx, 0u);
    const f64 half = lfs(g13::kHalf5AC278);
    const u32 cam = v + 864;
    Stack frame(ctx, 16);  // the original's sp+112: the half extents
    const u32 ext = frame.addr();
    for (u32 k = 0; k < load<u32>(g11::g::effect_count); ++k) {  // cmplw
        const u32 slot = g11::g::effect_slots + 200 * k;
        const u32 o = load<u32>(slot + 8);
        const u32 c = o + 240;
        const f64 dy = sgl(lfs(cam + 4) - lfs(c + 4)), dx = sgl(lfs(cam) - lfs(c)), dz = sgl(lfs(cam + 8) - lfs(c + 8));
        const f64 r = lfs(slot + 20);
        const f64 r2 = sgl(r * r);
        const f64 d2 = g11::madds(dz, dz, g11::madds(dx, dx, sgl(dy * dy)));
        ctx.fpr[1] = d2;
        if (d2 > r2) continue;  // fcmpu gt (NaN: tested)
        const u32 lo = load<u32>(o + 208) + 16, hi = lo + 16;
        const f64 ex = sgl(sgl(lfs(hi) - lfs(lo)) * half);
        const f64 ey = sgl(sgl(lfs(hi + 4) - lfs(lo + 4)) * half);
        const f64 ez = sgl(sgl(lfs(hi + 8) - lfs(lo + 8)) * half);
        store<u32>(ext, g11::stfs_bits(ex));
        store<u32>(ext + 4, g11::stfs_bits(ey));
        store<u32>(ext + 8, g11::stfs_bits(ez));
        ctx.fpr[1] = ex;
        if (static_cast<u32>(call(ctx, g13::fn::Sphere_FrustumFine, {v + 272, c, ext, o + 80})) == 0) continue;
        if (load<u32>(slot) != 0) {
            if (load<u8>(g13::g::effect_global_seen) == 0) store<u8>(g13::g::effect_global_seen, 1);
        } else {
            call(ctx, g13::fn::EffectSlot_Submit_005AB908, {v, slot});
        }
    }
    ret(ctx, load<u32>(g11::g::effect_count));
}

// u8 Stream_NextSlot_004FCE70(Streams* o)   (Streams_Bind_00538950)
// Returns the u8 o+109 and counts it up (r4 = o+109, r5 = the new
// value, as left).
DECOMP(004FCE70, Stream_NextSlot_004FCE70) {
    const u32 p = arg<u32>(ctx, 0) + 109;
    const u32 n = load<u8>(p);
    store<u8>(p, static_cast<u8>(n + 1));
    ctx.gpr[4] = p;
    ctx.gpr[5] = n + 1;
    ret(ctx, n);
}

namespace {

// One stream record {+0 address, +4 the RSX offset << 1, +8 count << 4 |
// low nibble kept | 1 (set after the offset), +12 top nibble = 2} for
// the bank's data at `at`; the stores in each compiled sequence's order
// (`late12`: +12 after +8). Returns the offset word.
u32 bind_stream13(PPUContext& ctx, u32 rec, u32 addr, u32 count, bool late12, u32 tmp) {
    const u32 w8 = (load<u32>(rec + 8) & 0xFu) | (count << 4);
    const u32 w12 = (load<u32>(rec + 12) & 0x0FFFFFFFu) | (2u << 28);
    store<u32>(rec, addr);
    if (!late12) store<u32>(rec + 12, w12);
    store<u32>(rec + 8, w8);
    if (late12) store<u32>(rec + 12, w12);
    store<u32>(tmp, 0);
    call(ctx, g11::fn::cellGcmAddressToOffset, {addr, tmp});
    const u32 off = load<u32>(tmp) << 1;
    store<u32>(rec + 4, off);
    store<u32>(rec + 8, load<u32>(rec + 8) | 1);
    return off;
}

}  // namespace

// void Streams_Bind_00538950(Streams* o, const MeshData* m)   (f_00524B68;
// name?)
// With the bank (g13::g::bank_state +4) and m+28 elements: stream slot
// Stream_NextSlot_004FCE70(o) (o+24 + 16 * slot) = the bank's data +
// m+24, m+28 elements; again with m+60 elements when that is set (the
// same data); and the record o+44 = the data + m+48, m+52 elements when
// that is set. Leaves r3 as the original does.
DECOMP(00538950, Streams_Bind_00538950) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 b = load<u32>(g13::g::bank_state + 4);
    if (b == 0) return ret(ctx, o);
    if (load<u32>(m + 28) == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);  // the original's sp+112 / +116 / +120
    const u32 sp = frame.addr();
    for (u32 cnt_off : {28u, 60u}) {
        if (cnt_off == 60 && load<u32>(m + 60) == 0) break;
        const u32 slot = static_cast<u32>(call(ctx, g13::fn::Stream_NextSlot_004FCE70, {o}));
        const u32 rec = o + 24 + (slot << 4);
        const u32 addr = load<u32>(b + 8) + load<u32>(m + 24);
        bind_stream13(ctx, rec, addr, load<u32>(m + cnt_off), false, sp + (cnt_off == 28 ? 8 : 4));
    }
    const u32 n = load<u32>(m + 52);
    if (n == 0) return ret(ctx, 0u);
    ret(ctx, bind_stream13(ctx, o + 44, load<u32>(b + 8) + load<u32>(m + 48), n, true, sp));
}

// bool Iff_HandleEmitProps_005284C0(IffReader* r, Obj* owner)   [an IFF
// chunk handler] (name?)
// A new AglEmitProperties from its ApeTListAllocator (the free list
// *g13::g::emit_props_list, built on first use: FreeList_Init(list, 140,
// heap, 0, 0)) through virtual slot 3 (+12), EmitProps_Construct; the
// owner's virtual slot 3 (+12); the allocator's load callback (phase 0),
// the chunks (Iff_ReadChunks), the callback again (phase 1). Returns 1.
// Quirk: no list / entry carries on with 0.
DECOMP(005284C0, Iff_HandleEmitProps_005284C0) {
    const u32 r = arg<u32>(ctx, 0), owner = arg<u32>(ctx, 1);
    constexpr u32 H = g13::g::emit_props_list;
    const u32 fl = get_list13(ctx, load<u32>(H), g13::g::vt_emit_props_list, g13::g::str_emit_props_list, 140);
    u32 o = 0;
    if (const u32 e = static_cast<u32>(fx::vcall(ctx, fl, 12, {fl})); e != 0) {
        call(ctx, g13::fn::EmitProps_Construct, {e});
        o = e;
    }
    fx::vcall(ctx, owner, 12, {owner});
    list_callback13(ctx, H, o, 0);
    call(ctx, g12::fn::Iff_ReadChunks, {r, o});
    list_callback13(ctx, H, o, 1);
    ret(ctx, 1u);
}

// ScreenWarp* ScreenWarp_Construct_0051F1C0(ScreenWarp* w)
// (PostChain_Construct_0051F850; name?)
// The post effect base (u8 +4 / +5 / +12, +8, +20 = 0, vtable
// vt_screen_warp), the colour +5736 = g13::g::white, u8 +5740 = 1,
// +5744 / +5748 / +5756 / +5768 = 0, f32 +5752 / +5760 the screen width
// and +5764 / +5772 the height (config u16 +4 / +6), the grid size u8
// +5713 / +5714 = 8; then the index list (u16 +5144.., count u16 +5716)
// for u8 +5714 strips of the grid: per strip, while the column counter
// (from 1) is at most u8 +5713 (signed; at least once), the pair (b,
// a) with b counting from 9 and a from 0, then the degenerate pair (a -
// 1, b) (each count and limit read again after every store); the count
// then drops by 2. u8 +5712 = 0, +5720 / +5724 = 0, +5728 / +5732 =
// 1.0, the name "ScreenWarp" +8 and its hash +16 (String_HashNoCase).
// Returns the hash.
DECOMP(0051F1C0, ScreenWarp_Construct_0051F1C0) {
    const u32 w = arg<u32>(ctx, 0);
    store<u8>(w + 4, 0);
    store<u8>(w + 5, 0);
    store<u32>(w + 8, 0);
    store<u8>(w + 12, 0);
    store<u32>(w, g13::vt_screen_warp);
    store<u32>(w + 20, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(w + 5736 + k, load<u8>(g13::g::white + k));
    store<u8>(w + 5740, 1);
    store<u32>(w + 5744, 0);
    store<u32>(w + 5748, 0);
    const f64 sw = sgl(static_cast<f64>(load<u16>(sly4::g::config + 4)));
    store<u32>(w + 5756, 0);
    store<u32>(w + 5752, g11::stfs_bits(sw));
    const f64 sh = sgl(static_cast<f64>(load<u16>(sly4::g::config + 6)));
    store<u32>(w + 5760, g11::stfs_bits(sw));
    store<u32>(w + 5768, 0);
    store<u8>(w + 5713, 8);
    store<u8>(w + 5714, 8);
    store<u16>(w + 5716, 0);
    store<u32>(w + 5764, g11::stfs_bits(sh));
    store<u32>(w + 5772, g11::stfs_bits(sh));
    const u32 cnt = w + 5716, idx = w + 5144;
    auto push = [&](u32 v) {
        const u32 n = load<u16>(cnt);
        store<u16>(cnt, static_cast<u16>(n + 1));
        store<u16>(idx + 2 * n, static_cast<u16>(v));
    };
    u32 a = 0, b = 9, row = 0;
    s32 cols = 8;  // li r9, 8 before the first strip, then u8 +5713
    do {
        if (cols >= 0) {
            u32 c = 0;
            do {
                c = (c + 1) & 0xFF;
                push(b++);
                push(a);
                a = (a + 1) & 0xFFFF;
            } while (static_cast<s32>(c) <= static_cast<s32>(load<u8>(w + 5713)));
        }
        row = (row + 1) & 0xFF;
        push(a - 1);
        push(b);
        cols = static_cast<s32>(load<u8>(w + 5713));
    } while (static_cast<s32>(row) < static_cast<s32>(load<u8>(w + 5714)));
    const u32 n = load<u16>(cnt);
    store<u8>(w + 5712, 0);
    store<u32>(w + 5724, 0);
    store<u32>(w + 5720, 0);
    store<u16>(cnt, static_cast<u16>(n - 2));
    store<u32>(w + 5732, 0x3F800000u);
    store<u32>(w + 5728, 0x3F800000u);
    store<u32>(w + 8, g13::g::str_screen_warp);
    ctx.fpr[1] = sh;  // the f1 the original passes on
    const u32 h = static_cast<u32>(call(ctx, sly4::post::fn::String_HashNoCase, {g13::g::str_screen_warp}));
    store<u32>(w + 16, h);
    ret(ctx, h);
}

// void Display_BindResolve_00534BAC(RenderSurface* d, u32 unit)
// (Display_Prepare_005358A8; gfx13.h: Display_ResolveTo_00534BAC; name?)
// As Display_BindAsTexture_00534DB4 but for the display's resolve
// target (+128 with config +34 (stereo), else +76) and its own sampler:
// filter (0, 7, 4, 1) when the display's +52 bits 22..24 are 2, else
// (0, 2, 2, 1); control (on, lod 0..15 << 8, no aniso); address (3, 3,
// 3, 0, 1, 0); the CellGcmTexture {0xA5, 1 mipmap, 2D, remap 0xAAE4,
// width config +4 * 2, height config +6 (* 2 unless mode 2), depth 1,
// location / pitch / offset from the target}; then render state +52 + 4
// * unit = 0. Leaves r3 = &render state +52.
DECOMP(00534BAC, Display_BindResolve_00534BAC) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 unit = arg<u32>(ctx, 1), idx = unit & 0xFF;
    constexpr u32 S = fx::g::state, kCfg = sly4::g::config;
    if (((load<u32>(d + 52) >> 22) & 7) == 2) call(ctx, g12::fn::cellGcmSetTextureFilter, {load<u32>(S), idx, 0, 7, 4, 1});
    else call(ctx, g12::fn::cellGcmSetTextureFilter, {load<u32>(S), idx, 0, 2, 2, 1});
    call(ctx, g12::fn::cellGcmSetTextureControl, {load<u32>(S), idx, 1, 0, 3840, 0});
    call(ctx, g12::fn::cellGcmSetTextureAddress, {load<u32>(S), idx, 3, 3, 3, 0, 1, 0});
    Stack frame(ctx, 64);  // the original's sp+112..175
    const u32 tex = frame.addr();
    store<u8>(tex, 0xA5);
    store<u8>(tex + 1, 1);
    store<u8>(tex + 2, 2);
    store<u8>(tex + 3, 0);
    store<u32>(tex + 4, 0xAAE4);
    const u32 mode = (load<u32>(d + 52) >> 22) & 7;
    const u32 c = load<u32>(S);
    const u8 stereo = load<u8>(kCfg + 34);
    if (mode == 2) {
        const u32 w = load<u16>(kCfg + 4), h = load<u16>(kCfg + 6);
        store<u16>(tex + 10, static_cast<u16>(h));
        store<u16>(tex + 8, static_cast<u16>(w << 1));
    } else {
        const u32 w = load<u16>(kCfg + 4), h = load<u16>(kCfg + 6);
        store<u16>(tex + 8, static_cast<u16>(w << 1));
        store<u16>(tex + 10, static_cast<u16>(h << 1));
    }
    store<u16>(tex + 12, 1);
    const u32 target = d + (stereo != 0 ? 128 : 76);
    store<u32>(tex + 16, load<u32>(load<u32>(target) + 32));
    store<u8>(tex + 14, static_cast<u8>(load<u32>(load<u32>(target) + 4) & 1));
    store<u8>(tex + 15, 0);
    store<u32>(tex + 20, load<u32>(load<u32>(target) + 4) >> 1);
    call(ctx, g12::fn::cellGcmSetTexture, {c, idx, tex});
    store<u32>(S + 52 + static_cast<u32>(static_cast<s32>(unit << 2)), 0);  // rldicr / extsw: the full unit
    ret(ctx, S + 52);
}

namespace {

// A LoadParams texture: added to m (Material_AddTexture_004FF008), then
// sized to m+20's u16 +122 / +124 through its virtual slot 4 (+16):
// (t, w, h, 1). `sizes_after`: the sequence leaves r7 = the OPD, r8 / r9
// = the sizes (else r7 = the OPD, r8 = its entry).
void add_sized_texture13(PPUContext& ctx, u32 m, u32 t, bool sizes_after) {
    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t});
    const u32 sz = load<u32>(m + 20);
    const u32 w = load<u16>(sz + 122), h = load<u16>(sz + 124);
    const u32 opd = load<u32>(load<u32>(t) + 16);
    if (sizes_after) call_ptr(ctx, opd, {t, w, h, 1, opd, w, h});
    else call_ptr(ctx, opd, {t, w, h, 1, opd, load<u32>(opd)});
}

}  // namespace

// bool LoadParams_005CD100(ShaderType*, Material* m, const Desc* d)
// [a shader type's LoadParams] (name?)
// Unless config +152 bit 7, with a desc version (u8 d+84): the texture
// named d+8 (Tex_FindLoaded; none: done) added and sized; params 0..2 =
// d+88 / +92 / +96, param 3 = d+100 from version 4 on (else the f32 at
// kParam3Default), param 4 = 5.0; then unless config +152 bit 6 the
// texture named d+12 added and sized. Returns 1.
DECOMP(005CD100, LoadParams_005CD100) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    constexpr u32 kCfg = sly4::g::config;
    if ((load<u8>(kCfg + 152) & 0x80) != 0) return ret(ctx, 1u);
    if (static_cast<s32>(load<u8>(d + 84)) <= 0) return ret(ctx, 1u);
    const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 8)}));
    if (t == 0) return ret(ctx, 1u);
    add_sized_texture13(ctx, m, t, true);
    auto param = [&](u32 k, u32 v) { store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, k})), v); };
    for (u32 k = 0; k < 3; ++k) {
        const u32 v = load<u32>(d + 88 + 4 * k);  // lfs / stfs: raw
        param(k, v);
    }
    const u32 v3 = static_cast<s32>(load<u8>(d + 84)) > 3 ? load<u32>(d + 100) : load<u32>(g13::kParam3Default);
    param(3, v3);
    param(4, 0x40A00000u);  // 5.0
    if ((load<u8>(kCfg + 152) & 0x40) == 0) {
        const u32 t2 = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 12)}));
        if (t2 != 0) add_sized_texture13(ctx, m, t2, false);
    }
    ret(ctx, 1u);
}

// bool Iff_HandlePacked10_005241F0(IffReader* r, Obj* o)   [an IFF chunk
// handler] (name?)
// Reads the chunk (Iff_ChunkSize bytes, FileReader_Read) into a
// temporary block (Mem_Alloc((u16 o+8 + 15) * 8 rounded to 128, 0, 16))
// of u16 o+8 entries of four u16 (byte-swapped first when the reader's
// u8 +12 bit 1 is set), and packs each entry into 5 bytes at o+12: bytes
// 0..3 = value >> 2, byte 4 = the four low bit pairs (v0 | v1 << 2 | v2
// << 4 | v3 << 6, merged in four stores as compiled). The block is then
// freed. Returns 1. Quirk: a failed allocation (0) is still read into
// and unpacked from; only the free is skipped.
DECOMP(005241F0, Iff_HandlePacked10_005241F0) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 size = static_cast<u32>(call(ctx, g12::fn::Iff_ChunkSize, {r}));
    const u32 bytes = ((load<u16>(o + 8) + 15u) << 3) & 0x7FF80u;
    const u32 buf = static_cast<u32>(call(ctx, fn::Mem_Alloc, {bytes, 0, 16}));
    call(ctx, g12::fn::FileReader_Read, {r, buf, size});
    if ((load<u8>(r + 12) & 2) != 0 && load<u16>(o + 8) != 0) {
        s32 n = load<u16>(o + 8);
        u32 p = buf;
        do {
            for (u32 k = 0; k < 8; k += 2) store<u16>(p + k, static_cast<u16>(load<u8>(p + k + 1) << 8 | load<u8>(p + k)));
            p += 8;
        } while (--n > 0);
    }
    u32 dst = load<u32>(o + 12);
    if (static_cast<s32>(load<u16>(o + 8)) > 0) {
        u32 src = buf;
        s32 i = 0;
        do {
            u32 b4 = load<u8>(dst + 4) & 0xFCu;
            const u32 v0 = load<u16>(src);
            b4 |= v0 & 3;
            store<u8>(dst, static_cast<u8>(v0 >> 2));
            store<u8>(dst + 4, static_cast<u8>(b4));
            b4 &= 0xF3;
            const u32 v1 = load<u16>(src + 2);
            b4 |= (v1 << 2) & 0x0C;
            store<u8>(dst + 1, static_cast<u8>(v1 >> 2));
            store<u8>(dst + 4, static_cast<u8>(b4));
            b4 &= 0xCF;
            const u32 v2 = load<u16>(src + 4);
            b4 |= (v2 << 4) & 0x30;
            store<u8>(dst + 2, static_cast<u8>(v2 >> 2));
            store<u8>(dst + 4, static_cast<u8>(b4));
            b4 &= 0x3F;
            const u32 v3 = load<u16>(src + 6);
            b4 |= (v3 << 6) & 0xC0;
            store<u8>(dst + 3, static_cast<u8>(v3 >> 2));
            store<u8>(dst + 4, static_cast<u8>(b4));
            dst += 5;
            src += 8;
            ++i;
        } while (i < static_cast<s32>(load<u16>(o + 8)));
    }
    if (buf != 0) call(ctx, sly4::render::gfx8::fn::Mem_FreeFrom, {buf, 0});
    ret(ctx, 1u);
}

// void Emitter_UpdateSeen_00506340(Emitter* o; f1 dt)   [called through
// a pointer] (name?)
// Emitter_Update_007A0C10 for emitters that may sleep: only when it is
// on (u8 +64 bit 7) and either always updates (desc +4's u8 +42 bit 5)
// or was seen this frame (+28 >= layers4::g::anim_stamp, unsigned).
// Then the same steps: prewarming (+64 bit 6 and desc +42 bit 6: the
// time +8 stepped back one interval, desc +4 / u16 +40, then emits one
// interval apart until the ring +72 is full), Emitter_Advance_00500908
// (dt), the ring's oldest particles dropped once their age (time - +12
// + 0.01) reaches the lifetime desc +4, and one particle per unit of the
// rate +16 (time offsets dt / +16 apart). Leaves r3 / f1 as the
// original does.
DECOMP(00506340, Emitter_UpdateSeen_00506340) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kK = g13::kEmitterUpdateConsts;  // f32 0.01, 0.0, 1.0
    const u32 b = load<u8>(o + 64);
    const f64 time0 = lfs(o + 8);
    auto sleep = [&] {
        ctx.fpr[1] = time0;
        ret(ctx, b);
    };
    if ((b & 0x80) == 0) return sleep();
    const u32 pb = load<u8>(load<u32>(o + 4) + 42);
    if ((pb & 0x20) == 0 && load<u32>(o + 28) < load<u32>(sly4::render::layers4::g::anim_stamp)) return sleep();
    if ((b & 0x40) != 0 && (pb & 0x40) != 0) {
        const u32 d = load<u32>(o + 4);
        const f64 step = sgl(lfs(d + 4) / sgl(static_cast<f64>(load<u16>(d + 40))));
        store<u32>(o + 8, g11::stfs_bits(sgl(time0 - step)));
        auto full = [&] {
            const u32 r = load<u32>(o + 72);
            return load<u32>(r + 20) == load<u32>(r + 16);
        };
        if (!full()) {
            const f64 zero = lfs(kK + 4);
            do {
                store<u32>(o + 8, g11::stfs_bits(sgl(lfs(o + 8) + step)));
                callf(ctx, g12::fn::Emitter_Emit_0079D280, {o}, {zero});
            } while (!full());
        }
    }
    callf(ctx, g12::fn::Emitter_Advance_00500908, {o}, {dt});
    const u32 ring = load<u32>(o + 72);
    const f64 now = lfs(o + 8);
    const u32 d = load<u32>(o + 4);
    f64 rate = lfs(o + 16);
    const f64 k0 = lfs(kK);
    for (u32 count = load<u32>(ring + 20);;) {
        const u32 p = count != 0 ? load<u32>(ring + 12) : 0u;
        if (p == 0) break;
        if (!(sgl(sgl(now - lfs(p + 12)) + k0) >= lfs(d + 4))) break;
        const u32 next = load<u32>(ring + 12) + g13::kParticleSize, end = load<u32>(ring + 4), base = load<u32>(ring);
        store<u32>(ring + 20, count - 1);
        store<u32>(ring + 12, static_cast<s32>(next - end) < 0 ? next : base);  // wrap at the end
        count -= 1;
    }
    ctx.fpr[1] = now;
    const f64 one = lfs(kK + 8);
    if (rate < one) return ret(ctx, kK);
    rate = sgl(dt / rate);
    f64 t = lfs(kK + 4);
    u64 r3 = 0;
    f64 left = 0;
    do {
        callf(ctx, g12::fn::Emitter_Emit_0079D280, {o}, {t});
        r3 = ctx.gpr[3];
        t = sgl(t + rate);
        left = sgl(lfs(o + 16) - one);
        store<u32>(o + 16, g11::stfs_bits(left));
    } while (left >= one);
    ctx.fpr[1] = left;
    ret(ctx, r3);
}

// u64 RenderThread_Create_005129BC(u32 a, u32 b)   (main; name?)
// The 56-byte render thread record (Mem_Alloc): +0 "AglRenderThread", +8
// u64 0 (the thread id), two event flags (sys_event_flag_create:
// protocol 2 (priority), pshared 0x200, key 0, flags 0, type 0x20000,
// named from +16 "WaitRenderStart" / +24 "WaitRenderComplete", ids +20 /
// +28), +32 a, +36 b, +40 / +44 / u8 +48 = 0; Thread_Create(t,
// opd_render_thread_main, {1536, 0, 1}). Then render_thread = t, and its
// thread id (+8) goes to the main-thread-id static's second slot (set up
// first if its guard is clear) and the screen's +152. Returns the id.
// Quirk: a failed allocation reads the id from address 8.
DECOMP(005129BC, RenderThread_Create_005129BC) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 t = static_cast<u32>(call(ctx, fn::Mem_Alloc, {56, 0, 16}));
    if (t != 0) {
        Stack frame(ctx, 80);  // the original's sp+112..191
        const u32 sp = frame.addr() - 112;
        auto event_flag = [&](u32 attr, u32 name_field, u32 name, u32 id_field) {
            store<u32>(t + name_field, name);
            if (name_field == 16) {
                store<u32>(t, g13::g::str_agl_render_thread);
                store<u64>(t + 8, 0);
            }
            store<u32>(attr, 2);          // SYS_SYNC_PRIORITY
            store<u32>(attr + 4, 0x200);  // SYS_SYNC_NOT_PROCESS_SHARED
            store<u64>(attr + 8, 0);      // ipc key
            store<u32>(attr + 16, 0);     // flags
            store<u32>(attr + 20, 0x20000);  // SYS_SYNC_WAITER_MULTIPLE
            store<u8>(attr + 24, 0);
            call(ctx, g12::fn::String_Copy, {attr + 24, load<u32>(t + name_field), 8});
            syscall(ctx, g13::kSysEventFlagCreate, {t + id_field, attr, 0});
        };
        event_flag(sp + 144, 16, g13::g::str_wait_render_start, 20);
        event_flag(sp + 112, 24, g13::g::str_wait_render_complete, 28);
        store<u32>(t + 32, a);
        store<u32>(t + 36, b);
        store<u32>(t + 40, 0);
        store<u32>(t + 44, 0);
        store<u8>(t + 48, 0);
        const u32 params = sp + 176;
        store<u32>(params + 4, 0);
        store<u32>(params, 1536);  // priority
        store<u32>(params + 8, 1);
        call(ctx, g13::fn::Thread_Create, {t, g13::g::opd_render_thread_main, params});
    }
    store<u32>(g13::g::render_thread, t);
    const u64 id = load<u64>(t + 8);
    if (load<u8>(g13::g::main_thread_guard) == 0) {
        store<u64>(g13::g::main_thread, 0);
        store<u64>(g13::g::main_thread + 8, 0);
        store<u8>(g13::g::main_thread_guard, 1);
    }
    store<u64>(g13::g::main_thread + 8, id);
    store<u64>(g12::g::screen + 152, id);
    ret(ctx, id);
}

// void EmitterDraw_Layered_005052FC(Emitter* o, ParticleList* list, x)
// (f_00505518, f_00505740; name?)
// Draws list's particles layer by layer: layer 0 is bound
// (EmitterDraw_BindLayer_00504D40(o, 0, 0, 0)) and the per-layer
// counts (g13::g::bucket_counts) cleared; each particle i from list+124
// - 1 down to 0 gets the layer q = (particle_layer[i] * u8 (o+4)->+88)
// >> 8: 0 draws it now (EmitterDraw_Particle_00504F68(o, list, 0, i,
// x)), else i is appended to row q of bucket_rows (count q - 1). Then
// layer 1 is bound and for each layer b from 0 below the layer count -
// 1 its particles are drawn with layer b + 1, after which layer b + 2
// is bound. Returns r3 as the last bind leaves it. Quirk: a layer count
// of 0 walks 2^32 - 1 layers.
DECOMP(005052FC, EmitterDraw_Layered_005052FC) {
    const u32 o = arg<u32>(ctx, 0), list = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    const u32 layers = load<u8>(load<u32>(o + 4) + 88);
    auto bind = [&](u32 layer) { return call(ctx, g13::fn::EmitterDraw_BindLayer_00504D40, {o, layer, 0, 0}); };
    bind(0);
    constexpr u32 C = g13::g::bucket_counts;
    for (u32 k = 0; k < 16; k += 4) store<u32>(C + k, 0);  // stvx of zero
    const u32 n = load<u32>(list + 124);
    if (n != 0) {
        for (u32 i = n - 1;; --i) {
            const u32 prod = load<u8>(load<u32>(o + 4) + 88) * load<u8>(g13::g::particle_layer + static_cast<u32>(static_cast<s32>(i)));
            const u32 q = prod >> 8;
            if (q == 0) {
                call(ctx, g13::fn::EmitterDraw_Particle_00504F68, {o, list, 0, i, x});
            } else {
                const u32 at = C + ((q - 1) << 2);
                const u32 c = load<u32>(at);
                store<u32>(at, c + 1);
                store<u32>(g13::g::bucket_rows + ((prod << 3) & 0xFFFFF800u) + (c << 2), i);
            }
            if (i == 0) break;
        }
    }
    u64 r3 = bind(1);
    const u32 count = layers - 1;
    for (u32 b = 0; b < count; ++b) {  // cmplw
        const u32 at = C + 4 * b;
        if (load<u32>(at) != 0) {
            u32 k = 0;
            do {
                call(ctx, g13::fn::EmitterDraw_Particle_00504F68,
                     {o, list, b + 1, load<u32>(g13::g::bucket_rows + 2048 * (b + 1) + 4 * k), x});
                ++k;
            } while (k < load<u32>(at));
        }
        r3 = bind(b + 2);
    }
    ret(ctx, r3);
}

// void EmitterDraw_Matrices_00505518(Emitter* o, ParticleList* list, x)
// (f_00507B18; name?)
// Builds each particle's matrix (g13::g::particle_mtx, from list+124 - 1
// down to 0): its rows' xyz scaled by the particle's size
// (Matrix_ScaleAxes with {s, s, s}), row 3 xyz = its position (raw);
// unless o+24 is set every row then goes through the list's 4x4 (list+0
// rows, lvx: rows 0..2 without, row 3 with its translation, vmaddfp
// order). Then EmitterDraw_Layered_005052FC(o, list, x), whose r3 it
// leaves.
DECOMP(00505518, EmitterDraw_Matrices_00505518) {
    const u32 o = arg<u32>(ctx, 0), list = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    const u32 n = load<u32>(list + 124);
    if (n != 0) {
        Stack frame(ctx, 16);  // the original's sp+112: the scale
        const u32 sv = frame.addr();
        for (u32 i = n - 1;; --i) {
            const u32 m = g13::g::particle_mtx + 64 * i;
            const u32 s = load<u32>(g13::g::particle_scale + 4 * i);  // lfsx / stfs: raw
            for (u32 k = 0; k < 12; k += 4) store<u32>(sv + k, s);
            ctx.fpr[1] = lfs(g13::g::particle_scale + 4 * i);
            call(ctx, g13::fn::Matrix_ScaleAxes, {m, sv});
            const u32 p = g13::g::particle_pos + 16 * i;
            for (u32 k = 0; k < 12; k += 4) store<u32>(m + 48 + k, load<u32>(p + k));  // lfs / stfs: raw
            ctx.fpr[1] = lfs(p + 8);
            if (load<u32>(o + 24) == 0) {
                f32 r[4][4];
                for (u32 row = 0; row < 4; ++row) {
                    f32 in[3];
                    for (u32 k = 0; k < 3; ++k) in[k] = load<f32>(((m + 16 * row) & ~15u) + 4 * k);
                    through13(list, in, row == 3, r[row]);
                }
                for (u32 row = 0; row < 4; ++row)
                    for (u32 k = 0; k < 4; ++k) store<f32>(((m + 16 * row) & ~15u) + 4 * k, r[row][k]);
            }
            if (i == 0) break;
        }
    }
    ret(ctx, call(ctx, g13::fn::EmitterDraw_Layered_005052FC, {o, list, x}));
}

// Emitter* EmitterDraw_BindLayer_00504D40(Emitter* o, u32 layer)
// (EmitterDraw_Layered_005052FC; name?)
// For a layer below u8 (o+4)->+88: its mesh L = (o+4)->+72[layer]
// binds its vertex arrays as Draw_BindVertexArrays_0079E0C0 does, inline
// (render state +16 = L's format +68, +8 = -1 for a new format; unless
// +8 already equals the address L+28 >> 1: per used, enabled attribute
// SET_VERTEX_DATA_ARRAY_FORMAT (stride u16 +120) / _OFFSET (address + u8
// +8 | (L+28 bit 0) << 31), unused arrays disabled highest first, +32 =
// the used mask); then with L+64 set, render state +4 = L+44. Returns r3
// = the layer count when out of range, else the render state.
DECOMP(00504D40, EmitterDraw_BindLayer_00504D40) {
    const u32 o = arg<u32>(ctx, 0), layer = arg<u32>(ctx, 1);
    const u32 d = load<u32>(o + 4);
    const u32 n = load<u8>(d + 88);
    if (!(layer < n)) return ret(ctx, n);  // cmplw
    constexpr u32 S = fx::g::state;
    const u32 L = load<u32>(d + 72 + 4 * layer);
    u32 cur_addr = load<u32>(S + 8);
    const u32 f = load<u32>(L + 68);
    if (load<u32>(S + 16) != f) {
        cur_addr = 0xFFFFFFFFu;
        store<u32>(S + 8, cur_addr);
    }
    store<u32>(S + 16, f);
    const u32 w28 = load<u32>(L + 28);
    const u32 stride = load<u16>(f + 120);
    const u32 addr = w28 >> 1;
    if (cur_addr != addr) {
        store<u32>(S + 8, addr);
        const u32 used = load<u16>(load<u32>(S + 20) + 10) & load<u16>(f + 126);
        if (load<u32>(f + 4) != 0) {
            const u32 st = (stride << 8) & 0xFF00, loc = (w28 << 31) & 0x80000000u;
            for (u32 a = f; load<u32>(a + 4) != 0; a += 12) {
                const u32 reg = load<u32>(a) + load<u8>(a + 9);
                const u32 sh = reg & 63, bit = sh < 32 ? 1u << sh : 0u;  // slw
                if (load<u8>(a + 10) != 0 || (bit & used) == 0) continue;
                const u32 type = load<u32>(a + 4);
                const u32 r4 = (reg << 2) & 0x3FC;
                const u32 c = load<u32>(S);
                const u32 fmt = ((static_cast<u32>(static_cast<s32>(type) >> 8) << 4) & 0xFF0) | (type & 0xFF);
                const u32 off = load<u8>(a + 8) + addr;
                const u32 cur = load<u32>(c + 8);
                store<u32>(cur, (r4 + 0x1740) | 0x00040000u);  // SET_VERTEX_DATA_ARRAY_FORMAT + 4 * reg
                store<u32>(cur + 4, fmt | st);
                store<u32>(cur + 8, (r4 + 0x1680) | 0x00040000u);  // SET_VERTEX_DATA_ARRAY_OFFSET + 4 * reg
                store<u32>(cur + 12, off | loc);
                store<u32>(c + 8, cur + 16);
            }
        }
        u32 off_mask = (load<u16>(S + 32) | used) ^ used;
        if (off_mask != 0) {
            for (s32 i = 15; i >= 0; --i) {
                const u32 bit = (1u << i) & 0xFFFF;
                if ((off_mask & bit) != 0) {
                    const u32 en = load<u16>(S + 32);
                    off_mask = (off_mask & ~bit) & 0xFFFF;
                    const u32 c = load<u32>(S);
                    store<u16>(S + 32, static_cast<u16>(en & ~bit));
                    const u32 cur = load<u32>(c + 8);
                    store<u32>(cur, ((static_cast<u32>(i) << 2) & 0x3FC) + 0x1740 + 0x00040000u);
                    store<u32>(cur + 4, 2);  // disabled
                    store<u32>(c + 8, cur + 8);
                }
                if (off_mask == 0) break;
            }
        }
        store<u16>(S + 32, static_cast<u16>(used));
    }
    if (load<u32>(L + 64) != 0) store<u32>(S + 4, L + 44);
    ret(ctx, S);
}

// void EmitterDraw_Particle_00504F68(Emitter* o, ParticleList*, u32 layer,
// u32 index, const f32 x[4][4])   (EmitterDraw_Layered_005052FC; name?)
// One particle: vertex constants 0..3 = (its matrix particle_mtx[index]
// rows' xyz through x: row i = x0 * m_i.x (+ x3 for row 3, first) + x1 *
// m_i.y + x2 * m_i.z, vmaddfp order) transposed; constant 4 = its colour
// (particle_colour[index] bytes / 255). Then the layer's mesh L =
// (o+4)->+72[layer] is drawn: a changed fragment program (render state
// +92 bit 31: cleared, cellGcmSetFragmentProgram(+28 +1136, 1)) or
// pending constants (+28's u64 +16) are uploaded (FProgram_Upload), then
// with an index count (L+64) cellGcmSetDrawIndexArray(u8 L+76 mode,
// count, the index buffer (render state +4)'s u8 +16 type, +4 bit 0
// location, +4 >> 1 offset), else cellGcmSetDrawArrays(u8 L+76, 0, L+40).
// Returns that call's r3.
DECOMP(00504F68, EmitterDraw_Particle_00504F68) {
    const u32 o = arg<u32>(ctx, 0), layer = arg<u32>(ctx, 2), index = arg<u32>(ctx, 3), x = arg<u32>(ctx, 4);
    constexpr u32 S = fx::g::state;
    const u32 m = (g13::g::particle_mtx + (index << 6)) & ~15u, xa = x & ~15u;
    auto lv = [](u32 a, u32 k) { return load<f32>(a + 4 * k); };
    f32 r[4][4];
    for (u32 i = 0; i < 4; ++i) {
        const u32 row = m + 16 * i;
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(lv(xa, k), lv(row, 0), i == 3 ? lv(xa + 48, k) : 0.0f);
            v = std::fmaf(lv(xa + 16, k), lv(row, 1), v);
            r[i][k] = std::fmaf(lv(xa + 32, k), lv(row, 2), v);
        }
    }
    Stack frame(ctx, 80);  // the original's sp+112 (colour) / +128 (matrix)
    const u32 col = frame.addr(), mt = col + 16;
    for (u32 k = 0; k < 4; ++k)
        for (u32 i = 0; i < 4; ++i) store<f32>(mt + 16 * k + 4 * i, r[i][k]);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(S), 0, 16, mt});
    const f64 inv = lfs(g13::kInv255_504F60);
    const u32 c = g13::g::particle_colour + (index << 2);
    for (u32 k = 0; k < 4; ++k) store<u32>(col + 4 * k, g11::stfs_bits(sgl(sgl(static_cast<f64>(load<u8>(c + k))) * inv)));
    ctx.fpr[1] = sgl(sgl(static_cast<f64>(load<u8>(c + 3))) * inv);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(S), 4, 4, col});
    const u32 L = load<u32>(load<u32>(o + 4) + 72 + (layer << 2));
    const u32 n = load<u32>(L + 64);
    auto upload = [&] {
        const u32 w92 = load<u32>(S + 92);
        if ((w92 >> 31) != 0) {
            store<u32>(S + 92, w92 & 0x7FFFFFFFu);
            call(ctx, draw::fn::cellGcmSetFragmentProgram, {load<u32>(S), load<u32>(S + 28) + 1136, 1});
            call(ctx, fx::fn::FProgram_Upload, {S + 28});
        } else if (const u32 fp = load<u32>(S + 28); fp != 0 && load<u64>(fp + 16) != 0) {
            call(ctx, fx::fn::FProgram_Upload, {S + 28});
        }
    };
    if (n != 0) {
        const u32 mode = load<u32>(L + 76);
        upload();
        const u32 ib = load<u32>(S + 4);
        const u32 w4 = load<u32>(ib + 4);
        ret(ctx, call(ctx, sly4::render::pool::fn::cellGcmSetDrawIndexArray, {load<u32>(S), mode & 0xFF, n, load<u32>(ib + 16) & 0xFF, w4 & 1, w4 >> 1}));
    } else {
        const u32 mode = load<u32>(L + 76), count = load<u32>(L + 40);
        upload();
        ret(ctx, call(ctx, sly4::render::pool::fn::cellGcmSetDrawArrays, {load<u32>(S), mode & 0xFF, 0, count}));
    }
}

// bool LoadParams_005CBE90(ShaderType*, Material* m, Desc* d)
// [a shader type's LoadParams] (name?)
// A desc older than version 4 (u8 d+84 <= 3) or without d+100 is
// upgraded instead and the load fails (0): d+100 = d+104 (version 5 and
// up) or the f32 at kParam100Default, d+104 = 5.0, version 4, d+80 =
// 0x2B5CD9F5. Otherwise, unless config +152 bit 7: the texture named d+8
// (Tex_FindLoaded) added and sized, params 0..2 = d+88 / +92 / +96, and
// unless config +152 bit 6 the texture named d+12 added and sized.
// Returns 1.
DECOMP(005CBE90, LoadParams_005CBE90) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    constexpr u32 kCfg = sly4::g::config;
    const s32 v = static_cast<s32>(load<u8>(d + 84));
    const u32 name = load<u32>(d + 8);
    if (!(v > 3 && load<u32>(d + 100) != 0)) {
        const u32 w = v > 4 ? load<u32>(d + 104) : load<u32>(g13::kParam100Default);  // lfs / stfs: raw
        ctx.fpr[1] = v > 4 ? lfs(d + 104) : lfs(g13::kParam100Default);
        store<u32>(d + 100, w);
        store<u32>(d + 104, 0x40A00000u);  // 5.0
        store<u8>(d + 84, 4);
        store<u32>(d + 80, 0x2B5CD9F5u);
        return ret(ctx, 0u);
    }
    if ((load<u8>(kCfg + 152) & 0x80) != 0) return ret(ctx, 1u);
    if (v <= 0) return ret(ctx, 1u);
    const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {name}));
    if (t == 0) return ret(ctx, 1u);
    add_sized_texture13(ctx, m, t, true);
    for (u32 k = 0; k < 3; ++k) {
        const u32 val = load<u32>(d + 88 + 4 * k);  // lfs / stfs: raw
        store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, k})), val);
    }
    if ((load<u8>(kCfg + 152) & 0x40) == 0) {
        const u32 t2 = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 12)}));
        if (t2 != 0) add_sized_texture13(ctx, m, t2, false);
    }
    ret(ctx, 1u);
}

// const void* Pack_Entry_007A4874(Pack* o, s32 i, u32* size)   (f_00563368;
// name?)
// An entry of a packed table at o+4 (base), its words byte-swapped when
// u8 o+12 is set: base+0 the entry count, base+8 + 8 * i the offset of
// entry i's header {+0 u32: 0 = empty, +8 its data offset}. An empty
// entry gives *size = 0 and returns 0; otherwise *size = the next
// entry's data offset (o+8 for the last) - this one's and it returns
// base + its data offset (both read again after the size is stored).
DECOMP(007A4874, Pack_Entry_007A4874) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    const s32 i = arg<s32>(ctx, 1);
    const u32 off_i = static_cast<u32>(static_cast<s32>(static_cast<u32>(i) << 3));
    u32 base = load<u32>(o + 4);
    bool swap = load<u8>(o + 12) != 0;
    const u32 end = load<u32>(o + 8);
    auto sw = [&](u32 v) { return swap ? __builtin_bswap32(v) : v; };
    const u32 hdr = sw(load<u32>(base + 8 + off_i));
    if (sw(load<u32>(base + hdr)) == 0) {
        store<u32>(out, 0);
        return ret(ctx, 0u);
    }
    const u32 start = sw(load<u32>(base + hdr + 8));
    u32 stop = end;
    if (i != static_cast<s32>(sw(load<u32>(base)) - 1)) {  // cmpw
        const u32 next = sw(load<u32>(base + 8 + static_cast<u32>(static_cast<s32>(static_cast<u32>(i + 1) << 3))));
        stop = sw(load<u32>(base + next + 8));
    }
    store<u32>(out, stop - start);
    base = load<u32>(o + 4);
    swap = load<u8>(o + 12) != 0;
    const u32 h2 = sw(load<u32>(base + 8 + off_i));
    ret(ctx, base + sw(load<u32>(base + h2 + 8)));
}

namespace {

// out_i = row i of a (lvx, rounded down to 16) through b (lvx rows):
// b0 * a_i.x (+ b3 for row 3, added first) + b1 * a_i.y + b2 * a_i.z,
// vmaddfp order.
void rows_through13(u32 a, u32 b, f32 out[4][4]) {
    a &= ~15u;
    b &= ~15u;
    for (u32 i = 0; i < 4; ++i) {
        const u32 row = a + 16 * i;
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(b + 4 * k), load<f32>(row), i == 3 ? load<f32>(b + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(b + 16 + 4 * k), load<f32>(row + 4), v);
            out[i][k] = std::fmaf(load<f32>(b + 32 + 4 * k), load<f32>(row + 8), v);
        }
    }
}

}  // namespace

// void View_SetOrtho_00522A64(View* v; f32 l, f32 r, f32 b, f32 t)
// [called through a pointer] (name?)
// An orthographic projection (Mat4_Ortho(l, r, b, t, v+60 near, v+64
// far)) into v+1008 (raw); v+880 = the view matrix v+752 (rows) times it
// (rows_through13), v+944 its transpose (stvx, then reloaded); the
// frustum planes v+272 from v+880 (Frustum_FromMatrix), then
// View_SetConstants(v), whose r3 it leaves.
DECOMP(00522A64, View_SetOrtho_00522A64) {
    const u32 v = arg<u32>(ctx, 0);
    Stack frame(ctx, 64);  // the original's sp+112..175
    const u32 pm = frame.addr();
    callf(ctx, g11::fn::Mat4_Ortho, {pm}, {farg(ctx, 0), farg(ctx, 1), farg(ctx, 2), farg(ctx, 3), lfs(v + 60), lfs(v + 64)});
    for (u32 k = 0; k < 64; k += 4) store<u32>(v + 1008 + k, load<u32>(pm + k));  // lfs / stfs: raw
    ctx.fpr[1] = lfs(pm + 60);
    f32 r[4][4];
    rows_through13(v + 752, v + 1008, r);
    const u32 vp = (v + 880) & ~15u, tp = (v + 944) & ~15u;
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) store<f32>(vp + 16 * i + 4 * k, r[i][k]);
    f32 q[4][4];
    for (u32 i = 0; i < 3; ++i)
        for (u32 k = 0; k < 4; ++k) q[i][k] = load<f32>(vp + 16 * i + 4 * k);  // rows 0..2 reloaded
    for (u32 k = 0; k < 4; ++k) q[3][k] = r[3][k];
    for (u32 k = 0; k < 4; ++k)
        for (u32 i = 0; i < 4; ++i) store<f32>(tp + 16 * k + 4 * i, q[i][k]);
    call(ctx, g11::fn::Matrix_Copy_004FDEA0, {v + 272, v + 880});  // Frustum_FromMatrix
    ret(ctx, call(ctx, sly4::render::shader5::fn::View_SetConstants, {v}));
}

// void Poly_ClipPlane_005A9B48(const f32 p[4], const f32 in[][4], u32 n,
// f32 out[][4], u32* count)   (f_005AA998; name?)
// Clips the polygon in (n points, 16-byte stride) against the plane p
// (kept side: p.x * x + p.y * y + p.z * z + p.w >= 0, NaN dropped):
// each kept point i is copied (raw), and where the side changes from i
// to the next point j the crossing in_i - (d_i / (p . (in_i - in_j))) *
// (in_i - in_j) is added unless that dot is below 1e-5 in size. *count
// = the output points (counted before each one's xyz is stored). Leaves
// r3 / f1 as the original does. Quirk: the original keeps the distances
// in its own frame (sp+48 + 4 * i), so n above 10 overwrites its saved
// registers; here they live in a separate block.
DECOMP(005A9B48, Poly_ClipPlane_005A9B48) {
    const u32 p = arg<u32>(ctx, 0), in = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3), cnt = arg<u32>(ctx, 4);
    std::vector<f64> d;
    if (n != 0) {
        const f64 w = lfs(p + 12);
        ctx.fpr[1] = w;
        s32 left = static_cast<s32>(n);
        u32 v = in;
        do {
            f64 s = g11::madds(lfs(p), lfs(v), w);
            s = g11::madds(lfs(p + 8), lfs(v + 8), s);
            s = g11::madds(lfs(p + 4), lfs(v + 4), s);
            d.push_back(s);
            v += 16;
        } while (--left > 0);
    }
    store<u32>(cnt, 0);
    if (n == 0) return;
    const f64 zero = lfs(g13::kClipConsts), eps = lfs(g13::kClipConsts + 4);
    auto dist = [&](u32 k) { return k < d.size() ? d[k] : 0.0; };
    for (u32 i = 0; i < n; ++i) {  // cmplw
        const u32 j = (i + 1) % n;
        const f64 di = dist(i);
        const bool a = di >= zero, b = dist(j) >= zero;
        ctx.fpr[1] = dist(j);
        const u32 vi = in + 16 * i;
        if (a) {
            const u32 k = load<u32>(cnt);
            store<u32>(cnt, k + 1);
            const u32 o = out + (k << 4);
            for (u32 c = 0; c < 12; c += 4) store<u32>(o + c, load<u32>(vi + c));  // lfs / stfs: raw
            ctx.fpr[1] = lfs(vi + 8);
        }
        if (a == b) continue;
        const u32 vj = in + 16 * j;
        const f64 x = lfs(vi), y = lfs(vi + 4), z = lfs(vi + 8);
        const f64 ey = sgl(y - lfs(vj + 4)), ex = sgl(x - lfs(vj)), ez = sgl(z - lfs(vj + 8));
        ctx.fpr[1] = ex;
        f64 dot = sgl(lfs(p + 4) * ey);
        dot = g11::madds(lfs(p), ex, dot);
        dot = g11::madds(lfs(p + 8), ez, dot);
        if (std::fabs(dot) < eps) continue;
        const f64 t = sgl(di / dot);
        const u32 k = load<u32>(cnt);
        const u32 o = out + (k << 4);
        store<u32>(cnt, k + 1);
        const f64 rx = g11::nmsubs(t, ex, x);
        store<u32>(o, g11::stfs_bits(rx));
        store<u32>(o + 4, g11::stfs_bits(g11::nmsubs(t, ey, y)));
        store<u32>(o + 8, g11::stfs_bits(g11::nmsubs(t, ez, z)));
        ctx.fpr[1] = rx;
    }
}

namespace {

// A 128-byte vertex format at f from the attribute templates at tpl: u16
// +120 stride 24, +126 mask 0, +124 |= 0x8000; the templates copied up
// to and including the one with +4 == 0, each used one adding bit (+0 +
// u8 +9, slw) to the mask (read again each time).
void vertex_format13(u32 f, u32 tpl) {
    const u32 flags = load<u16>(f + 124);
    store<u16>(f + 120, 24);
    store<u16>(f + 126, 0);
    store<u16>(f + 124, static_cast<u16>(flags | 0x8000));
    for (u32 dst = f;; dst += 12, tpl += 12) {
        const u32 type = load<u32>(tpl + 4), reg = load<u32>(tpl);
        store<u32>(dst + 4, type);
        store<u32>(dst, reg);
        store<u8>(dst + 8, load<u8>(tpl + 8));
        const u32 sub = load<u8>(tpl + 9);
        store<u8>(dst + 9, static_cast<u8>(sub));
        store<u8>(dst + 10, load<u8>(tpl + 10));
        if (type == 0) break;
        const u32 sh = (reg + sub) & 63;
        const u32 bit = sh < 32 ? 1u << sh : 0u;
        store<u16>(f + 126, static_cast<u16>(load<u16>(f + 126) | bit));
    }
}

}  // namespace

// bool Graphics_InitFormats_0050ECD4()   (Graphics_Init; name?)
// The two built-in vertex formats (render::g::vertex_formats, +0 / +128,
// from vertex_format_tpl0 / tpl1) and the draw ring (224 bytes,
// DrawRing_Init_0050EB08(ring, config +336 (0: 0x20000, negative: 0),
// config +332 (0: 24576, negative: 0), 2, 0)) into render::g::state_cache
// (0 when Mem_Alloc fails). Returns 1.
DECOMP(0050ECD4, Graphics_InitFormats_0050ECD4) {
    vertex_format13(sly4::render::g::vertex_formats, g13::g::vertex_format_tpl0);
    vertex_format13(sly4::render::g::vertex_formats + 128, g13::g::vertex_format_tpl1);
    auto pick = [](u32 v, u32 dflt) { return v == 0 ? dflt : static_cast<s32>(v) > 0 ? v : 0u; };
    const u32 size = pick(load<u32>(sly4::g::config + 336), 0x20000);
    const u32 count = pick(load<u32>(sly4::g::config + 332), 24576);
    u32 ring = static_cast<u32>(call(ctx, fn::Mem_Alloc, {224, 0, 16}));
    if (ring != 0) call(ctx, g13::fn::DrawRing_Init_0050EB08, {ring, size, count, 2, 0});
    store<u32>(sly4::render::g::state_cache, ring);
    ret(ctx, 1u);
}

// bool LoadParams_005B6528(ShaderType*, Material* m, const Desc* d)
// [a shader type's LoadParams] (name?)
// Unless LoadParams_005AF298(g12::g::sho_texture_warp_type, m, d)
// fails (then 0): param 10 = d+128 (desc version u8 d+84 at least 10)
// or 1; params 11..20 = d+132.. (version at least the index) or the f32
// at kParams11Default (raw). Returns 1.
DECOMP(005B6528, LoadParams_005B6528) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if (static_cast<u32>(call(ctx, g12::fn::Shader_IsReady_005AF298, {g12::g::sho_texture_warp_type, m, d})) == 0) return ret(ctx, 0u);
    auto ver = [&] { return static_cast<s32>(load<u8>(d + 84)); };
    auto param = [&](u32 k, u32 v) { store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, k})), v); };
    param(10, ver() < 10 ? 1u : load<u32>(d + 128));
    const u32 dflt = load<u32>(g13::kParams11Default);
    for (u32 k = 11; k <= 20; ++k) param(k, ver() < static_cast<s32>(k) ? dflt : load<u32>(d + 128 + 4 * (k - 10)));  // lfs / stfs: raw
    ret(ctx, 1u);
}

// void Material_SetMatrixParams_005AA718(Material* m, u32 v, const f32 src[16])
// (EffectSlot_Submit_005AB908; name?)
// Obj_SetSlot20(m, v, 1), then params 3..18 = src[0..15] (raw). Leaves
// r3 = the last param's slot.
DECOMP(005AA718, Material_SetMatrixParams_005AA718) {
    const u32 m = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1), src = arg<u32>(ctx, 2);
    call(ctx, g11::fn::Obj_SetSlot20, {m, v, 1});
    u32 slot = 0;
    for (u32 k = 0; k < 16; ++k) {
        const u32 w = load<u32>(src + 4 * k);  // lfs / stfs: raw
        slot = static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, 3 + k}));
        store<u32>(slot, w);
    }
    ret(ctx, slot);
}

// bool Iff_HandleScaleCurve_00529D80(IffReader* r, EmitProps* o)   [an IFF
// chunk handler] (name?)
// A version byte (Iff_ReadU8), the "variance" (FileReader_ReadF32Swapped,
// default 0), then a new 80-byte scale curve (Mem_Alloc; vtable
// vt_scale_curve, u16 +4 refs 1, +8 / +12 = 0) referenced from o+116
// (the new one's refs up, the old one's down: at zero its virtual slot 1
// (+4)), its creation reference dropped; +12 = the variance clamped to
// [0, 1] (fcmpu: NaN gives 1); then the chunk's remaining words as
// "Scale" keys +16.. (count +8; the first is also written to the second
// slot). Returns 1.
DECOMP(00529D80, Iff_HandleScaleCurve_00529D80) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);  // the original's sp+112 (variance), +116 (key), +120 (version)
    const u32 sp = frame.addr();
    store<u32>(sp, 0);
    call(ctx, g10::fn::Iff_ReadU8, {r, sp + 8, 0});
    call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp, g13::g::str_variance});
    const u32 p = static_cast<u32>(call(ctx, fn::Mem_Alloc, {80, 0, 16}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p, g13::vt_scale_curve);
        store<u32>(p + 8, 0);
        store<u32>(p + 12, 0);
    }
    if (const u32 old = load<u32>(o + 116); old != p) {
        store<u32>(o + 116, p);
        if (p != 0) store<u16>(p + 4, load<u16>(p + 4) + 1);
        if (old != 0) {
            const u16 n = static_cast<u16>(load<u16>(old + 4) - 1);
            store<u16>(old + 4, n);
            if (static_cast<s16>(n) <= 0) fx::vcall(ctx, old, 4, {old});
        }
    }
    {
        const u32 q = load<u32>(o + 116);
        store<u16>(q + 4, static_cast<u16>(load<u16>(q + 4) - 1));
    }
    const f64 hi = lfs(g13::kUnitRange), lo = lfs(g13::kUnitRange + 4), var = lfs(sp);
    const u32 c = load<u32>(o + 116);
    const f64 m = var < hi ? var : hi;
    const f64 v = m > lo ? (var < hi ? var : hi) : lo;
    store<u32>(c + 12, g11::stfs_bits(v));
    ctx.fpr[1] = v;
    const u32 n = static_cast<u32>(call(ctx, g13::fn::ChunkReader_Remaining, {r})) >> 2;
    for (u32 i = 0; i < n; ++i) {  // cmplw
        call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp + 4, g13::g::str_scale});
        const u32 q = load<u32>(o + 116);
        const u32 key = load<u32>(sp + 4);  // lfs / stfs: raw
        const u32 k = load<u32>(q + 8);
        store<u32>(q + 8, k + 1);
        store<u32>(q + 16 + 4 * k, key);
        if (const u32 k2 = load<u32>(q + 8); k2 == 1) store<u32>(q + 16 + 4 * k2, key);
        ctx.fpr[1] = lfs(sp + 4);
    }
    ret(ctx, 1u);
}

// bool LoadParams_005B44B4(ShaderType*, Material* m, const Desc* d)
// [a shader type's LoadParams] (name?)
// The texture named d+4 (Tex_FindLoaded; none: returns 0) gets virtual
// slot 4 (+16) (t, 1, 3, 1) and is added to m; m+120's top nibble = 2
// when u8 d+76 is 2, else 3 (bits 30..31 cleared). Then as
// LoadParams_005CD100's start: unless config +152 bit 7, with a desc
// version, the texture named d+8 added and sized, params 0..2 = d+88 /
// +92 / +96, unless config +152 bit 6 the texture named d+12 added and
// sized. Returns 1.
DECOMP(005B44B4, LoadParams_005B44B4) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    constexpr u32 kCfg = sly4::g::config;
    const u32 t0 = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 4)}));
    if (t0 == 0) return ret(ctx, 0u);
    fx::vcall(ctx, t0, 16, {t0, 1, 3, 1});
    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t0});
    const u32 mode = load<u8>(d + 76) == 2 ? 2u : 3u;
    store<u32>(m + 120, (load<u32>(m + 120) & 0x0FFFFFFFu) | ((mode << 28) & 0x30000000u));
    if ((load<u8>(kCfg + 152) & 0x80) != 0) return ret(ctx, 1u);
    if (static_cast<s32>(load<u8>(d + 84)) <= 0) return ret(ctx, 1u);
    const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 8)}));
    if (t == 0) return ret(ctx, 1u);
    add_sized_texture13(ctx, m, t, true);
    for (u32 k = 0; k < 3; ++k) {
        const u32 val = load<u32>(d + 88 + 4 * k);  // lfs / stfs: raw
        store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, k})), val);
    }
    if ((load<u8>(kCfg + 152) & 0x40) == 0) {
        const u32 t2 = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 12)}));
        if (t2 != 0) add_sized_texture13(ctx, m, t2, false);
    }
    ret(ctx, 1u);
}
