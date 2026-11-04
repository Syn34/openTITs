// The renderer range's remaining functions of ~80 to ~450 instructions:
// the callees gfx10*.cpp left recompiled (the GPU timer behind the shadow
// pass, the stencil shadow quad format, ...) and the next smallest ones.
// Names carry the address where the role is unclear.
#include <algorithm>
#include <atomic>
#include <cmath>

#include "fx.h"
#include "gfx11.h"
#include "pool.h"

using namespace sly4;

namespace {

namespace g11 = sly4::render::gfx11;

constexpr u32 kShadowPool = 0x010DF8B0;  // layers3::g::pool (the StencilShadow renderer)

}  // namespace

// void (GpuTimer* t)   (ShadowPass_Begin_00518490 passes 0x010DDB88; name?)
// Adaptive on / off switch from a GPU time. GpuTimer: +0 / +4 u32 report
// indices (2044 at +0: none, the frame counter is used instead), +8 f32
// on threshold, +12 f32 off threshold, +16 u32 countdown reload, +24 u64
// countdown, +32 u8 on, +36 f32 the frame counter (0x010F0F9C) as a float.
// Each call counts +24 down; once it reaches 0 the time between the two
// timestamps (ns, absolute) / 1e6 (over 30 ms counts as 0) switches +32:
// on at >= +8, off at <= +12, the countdown reloading on each. Config
// byte +34 forces +32 off. Leaves f1 = the value compared, and r3 as the
// original does (t on the early out, else 0 or the reload).
DECOMP(0053CA20, GpuTimer_Update_0053CA20) {
    const u32 t = arg<u32>(ctx, 0);
    const f64 frame = static_cast<f32>(static_cast<f64>(load<u32>(g11::g::frame_counter)));
    const u64 left = load<u64>(t + 24) - 1;
    store<u64>(t + 24, left);
    store<u32>(t + 36, std::bit_cast<u32>(static_cast<f32>(frame)));
    ctx.fpr[1] = frame;
    if (static_cast<s64>(left) > 0) {
        ret(ctx, t);
        return;
    }
    constexpr u32 kK = 0x0053CA10;  // f32 30.0 (limit), 0.0, 1e6 (ns per ms)
    f64 v;
    if (load<u32>(t) == 2044) {
        v = static_cast<f32>(static_cast<f64>(load<u32>(g11::g::frame_counter)));
    } else {
        const u64 a = call(ctx, g11::fn::cellGcmGetTimeStamp, {load<u32>(t + 4)});
        const u64 b = call(ctx, g11::fn::cellGcmGetTimeStamp, {load<u32>(t)});
        const u64 d = a > b ? a - b : b - a;
        const f64 two32 = g11::lfd(0x00AF1DD8);  // 2^32
        const f32 ns = static_cast<f32>(std::fma(two32, static_cast<f64>(d >> 32), static_cast<f64>(d & 0xFFFFFFFFu)));
        v = static_cast<f32>(static_cast<f64>(ns) / g11::lfs(kK + 8));
        if (v > g11::lfs(kK)) v = g11::lfs(kK + 4);
    }
    ctx.fpr[1] = v;
    const u8 forced_off = load<u8>(sly4::g::config + 34);
    if (forced_off != 0) {
        store<u8>(t + 32, 0);
        ret(ctx, 0u);
        return;
    }
    u64 r3 = forced_off;
    if (v >= g11::lfs(t + 8)) {
        store<u8>(t + 32, 1);
        r3 = load<u32>(t + 16);
        store<u64>(t + 24, r3);
    }
    if (v <= g11::lfs(t + 12)) {
        store<u8>(t + 32, 0);
        r3 = load<u32>(t + 16);
        store<u64>(t + 24, r3);
    }
    ret(ctx, r3);
}

// void ()   (Records_Init_00527484; name?)
// The stencil shadow renderer's quad: vertex format +176 from the template
// 0x00F671BC (12-byte attributes up to the one with +4 == 0, copied too;
// +302 mask |= 1 << (register base + register) per attribute), stride
// +296 = 12, +300 |= 0x8000, +302 cleared first; then the buffer +12 gets
// 4 * stride bytes (Buffer_Alloc52F198(+12, .., 128, 0, 2)), its +8 bits
// 28..30 = 2, and the 4 xy corners (-1, 1) (-1, -1) (1, 1) (1, -1) when
// it got memory (z left).
DECOMP(0052730C, StencilShadow_InitQuad_0052730C) {
    constexpr u32 p = kShadowPool, kTemplate = 0x00F671BC;
    constexpr u32 fmt = p + 176;
    store<u16>(p + 296, 12);
    store<u16>(p + 302, 0);
    store<u16>(p + 300, static_cast<u16>(load<u16>(p + 300) | 0x8000));
    u32 src = kTemplate, dst = fmt;
    for (;;) {
        const u32 type = load<u32>(src + 4);
        store<u32>(dst + 4, type);
        store<u32>(dst, load<u32>(src));
        store<u8>(dst + 8, load<u8>(src + 8));
        store<u8>(dst + 9, load<u8>(src + 9));
        store<u8>(dst + 10, load<u8>(src + 10));
        if (type == 0) break;
        const u32 sh = (load<u32>(src) + load<u8>(src + 9)) & 0x3F;  // slw
        store<u16>(p + 302, static_cast<u16>(load<u16>(p + 302) | (sh < 32 ? 1u << sh : 0u)));
        src += 12;
        dst += 12;
    }
    call(ctx, g11::fn::Buffer_Alloc52F198, {p + 12, static_cast<u32>(load<u16>(p + 296)) << 2, 128, 0, 2});
    const u32 buf = p + 12;
    store<u32>(buf + 8, (load<u32>(buf + 8) & ~0xEu) | (2u << 1));
    const u32 v = load<u32>(buf);
    if (v != 0) {
        constexpr u32 kMinus1 = 0xBF800000u, kOne = 0x3F800000u;
        store<u32>(v + 0, kMinus1);
        store<u32>(v + 4, kOne);
        store<u32>(v + 12, kMinus1);
        store<u32>(v + 16, kMinus1);
        store<u32>(v + 24, kOne);
        store<u32>(v + 28, kOne);
        store<u32>(v + 36, kOne);
        store<u32>(v + 40, kMinus1);
        ret(ctx, kMinus1);  // r3 as the original leaves it
    } else {
        ret(ctx, load<u32>(buf + 4) & 1);
    }
}

// void (Warp* w)   (Warp_Update_00545A38, the first time; name?)
// Once (u8 +5776): two vertex buffer parts +5796 / +5812 of n * 24 bytes
// (n = u8 +5713 * u8 +5714, kept at +5792; pool 5, the RSX offset << 1 |
// 1 at +4) and the u16 index buffer part +5828 (pool 4, count +5788 =
// 2 * cols * rows + 2 * rows - 2 * cols - 4, +8 bits 28..30 = 2), then
// the strip indices: per row of u8 +5785 rows the (cols + k, k) pairs for
// each of the cols columns, and between rows the join (last - 1, next).
// Parts: +8 size << 4 | flags, +12 tag in bits 28..31 (2). Leaves r3 =
// u8 +5713 (or u8 +5776 on the early out).
DECOMP(00544B4C, Warp_BuildGrid_00544B4C) {
    const u32 w = arg<u32>(ctx, 0);
    const u8 built = load<u8>(w + 5776);
    if (built != 0) {
        ret(ctx, built);
        return;
    }
    const u32 n = static_cast<u32>(load<u8>(w + 5713)) * load<u8>(w + 5714);
    const u32 bytes = n * 24;
    Stack frame(ctx, 16);
    const u32 off = frame.addr();  // the three offsets (120, 116, 112 in the original)
    store<u32>(w + 5792, n);
    store<u8>(w + 5776, 1);
    // Part fields: size << 4 in bits 7..27 of +8, tag 2 in bits 28..31 of +12.
    const auto part_size = [&](u32 part) { return (load<u32>(part + 8) & 0xFu) | ((bytes << 4) & 0x01FFFFF0u); };
    const auto part_tag = [&](u32 part) { return (load<u32>(part + 12) & 0x0FFFFFFFu) | (2u << 28); };
    // Vertex buffer 0.
    const u32 size0 = part_size(w + 5796), tag0 = part_tag(w + 5796);
    store<u32>(w + 5804, size0);
    store<u32>(w + 5808, tag0);
    u32 p = static_cast<u32>(call(ctx, g11::fn::Mem_AllocPool, {128, size0 >> 4, 5, 2}));
    store<u32>(w + 5796, p);
    store<u32>(off + 8, 0);
    call(ctx, g11::fn::cellGcmAddressToOffset, {p, off + 8});
    // Vertex buffer 1 (its fields written after buffer 0's offset).
    const u32 size1 = part_size(w + 5812), tag1 = part_tag(w + 5812);
    store<u32>(w + 5800, (load<u32>(off + 8) << 1) | 1);
    store<u32>(w + 5820, size1);
    store<u32>(w + 5824, tag1);
    p = static_cast<u32>(call(ctx, g11::fn::Mem_AllocPool, {128, size1 >> 4, 5, 2}));
    store<u32>(w + 5812, p);
    store<u32>(off + 4, 0);
    call(ctx, g11::fn::cellGcmAddressToOffset, {p, off + 4});
    // Index buffer.
    const u32 ib = w + 5828;
    {
        const u32 cols2 = static_cast<u32>(load<u8>(w + 5713)) << 1, rows = load<u8>(w + 5714);
        store<u32>(w + 5844, 1);
        const u32 count = cols2 * rows + (rows << 1) - (cols2 + 2) - 2;
        store<u32>(w + 5816, (load<u32>(off + 4) << 1) | 1);
        store<u32>(w + 5788, count);
        const u32 size = (load<u32>(ib + 8) & 0xFu) | (count << 5);
        const u32 tag = (load<u32>(ib + 12) & 0x0FFFFFFFu) | (2u << 28);
        store<u32>(ib + 12, tag);
        store<u32>(ib + 8, size);
        p = static_cast<u32>(call(ctx, g11::fn::Mem_AllocPool, {128, size >> 4, 4, 2}));
        store<u32>(ib, p);
        store<u32>(off, 0);
        call(ctx, g11::fn::cellGcmAddressToOffset, {p, off});
        store<u32>(ib + 4, load<u32>(off) << 1);
        store<u32>(ib + 8, (load<u32>(ib + 8) & ~0xEu) | (2u << 1));
    }
    // The strip indices (the sizes re-read as the original does).
    u32 cols = load<u8>(w + 5713), strips = load<u8>(w + 5785);
    u32 dst = load<u32>(ib);
    u32 next = cols, cur = 0;  // r5 / r8
    if (strips != 0) {
        u32 row = 0;
        do {
            if (cols != 0) {
                u32 k = 0;
                do {
                    store<u16>(dst, static_cast<u16>(next));
                    store<u16>(dst + 2, static_cast<u16>(cur));
                    dst += 4;
                    ++next;
                    ++cur;
                    ++k;
                    cols = load<u8>(w + 5713);
                } while (k < cols);
                strips = load<u8>(w + 5785);
            }
            if (static_cast<s32>(row) != static_cast<s32>(strips - 1)) {
                store<u16>(dst, static_cast<u16>(cur - 1));
                store<u16>(dst + 2, static_cast<u16>(next));
                dst += 4;
                strips = load<u8>(w + 5785);
                cols = load<u8>(w + 5713);
            }
            ++row;
        } while (row < strips);
    }
    ret(ctx, cols);
}

// void ()   (ShadowPass_Begin_00518490; name?)
// Copies the scene depth (Display_GetDepthTexture) into the depth target
// *0x011640D8 with PostProc_DownSampleDepth: vertex constant 1 = the
// (lazily zeroed) vec4 at 0x010F0C90; blending and alpha test off, depth
// mask = bit 0 / depth test = u8 render state +93, depth func ALWAYS,
// stencil and culling off, colour writes off; texture unit 0, the quad
// (the Draw_PrepareQuad / Draw_RestoreDepth thunks), the state cache
// reset and colour writes back on. Leaves r3 = the gcm context.
DECOMP(005181E0, ShadowPass_CopyDepth_005181E0) {
    namespace g11g = g11::g;
    const u32 st = fx::g::state;
    const u32 depth = static_cast<u32>(call(ctx, render::shader2::fn::Display_GetDepthTexture, {sly4::g::mem_stats}));
    if (load<u8>(g11g::zero_c90_ready) == 0) {
        for (u32 k = 0; k < 16; k += 4) store<u32>(g11g::zero_c90 + k, 0);
        store<u8>(g11g::zero_c90_ready, 1);
    }
    call(ctx, fx::fn::Draw_BindTargets, {load<u32>(g11g::shadow_depth), 0, 0});
    fx::bind_vertex(ctx, fx::g::vp_down_depth);
    fx::bind_fragment(fx::g::fp_down_depth);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 1, 4, g11g::zero_c90});
    fx::emit({fx::kBlendEnable, 0});
    fx::emit({fx::kAlphaTestEnable, 0});
    fx::emit({fx::kDepthMask, load<u8>(st + 93) & 1u});
    fx::emit({fx::kDepthTestEnable, load<u8>(st + 93)});
    fx::emit({fx::kDepthFunc, 0x207 /* ALWAYS */});
    fx::emit({fx::kStencilTestEnable, 0});
    fx::emit({fx::kCullFaceEnable, 0});
    fx::emit({fx::kColorMask, 0});
    call(ctx, fx::fn::Draw_SetTexture0Call, {depth, 0, 0});
    call(ctx, g11::fn::Thunk_PrepareQuad_0053C850);
    call(ctx, g11::fn::Thunk_RestoreDepth_0053C830);
    call(ctx, render::fn::Draw_InvalidateState);
    fx::emit({fx::kColorMask, 0x01010101u});
    ret(ctx, load<u32>(st));
}

// void (DrawQueue* q)   (DrawQueue_Begin_00518610, queue 0x011DE300; name?)
// Unless the list q+24 is empty (u16 +8): binds the programs
// ****(q+32)+16 +12 holds (+4 vertex, +8 fragment), vertex constant 6 =
// {0.05, 0, 1 / near, (near - far) / (near * far)} of the current view
// (*(*0x010DDA74 + 672) +60 / +64), uploads the fragment program, sets
// the vertex format q+28 (all its attributes, data offset 0 in main
// memory) unless the render state's +8 cache is already 0, atomically
// clears q+256, then builds the batch {list, 0x24000, 0...} on the stack
// with f_00516970 and resets the state cache.
DECOMP(00516CD0, DrawQueue_Run_00516CD0) {
    namespace pl = sly4::render::pool;
    const u32 q = arg<u32>(ctx, 0);
    const u16 count = load<u16>(load<u32>(q + 24) + 8);
    if (count == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 st = fx::g::state;
    const u32 progs = load<u32>(load<u32>(load<u32>(load<u32>(q + 32) + 16) + 12));
    fx::bind_vertex(ctx, progs + 4);
    fx::bind_fragment(progs + 8);

    const u32 view = load<u32>(load<u32>(sly4::draw::g::current_surface) + 672);
    const f64 n = g11::lfs(view + 60), f = g11::lfs(view + 64);
    const f64 nf = static_cast<f32>(f * n);
    const f64 inv_n = static_cast<f32>(g11::lfs(0x00516CCC) / n);  // 1.0 / near
    const f64 range = static_cast<f32>(static_cast<f64>(static_cast<f32>(n - f)) / nf);
    Stack frame(ctx, 48);
    const u32 vc = frame.addr(), batch = frame.addr() + 16;
    store<u32>(vc, 0x3D4CCCCDu);  // 0.05f
    store<u32>(vc + 4, 0);
    store<u32>(vc + 8, g11::f32_bits(inv_n));
    store<u32>(vc + 12, g11::f32_bits(range));
    ctx.fpr[1] = inv_n;  // f1 / f2 as the original leaves them
    ctx.fpr[2] = range;
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 6, 4, vc});
    pl::upload_fragment(ctx);

    const u32 vf = load<u32>(q + 28);
    if (load<u32>(st + 8) != 0) {
        store<u32>(st + 8, 0);
        pl::send_attributes(vf, load<u16>(vf + 120), load<u16>(vf + 126), 0, 0x80000000u);
    }
    std::atomic_ref<u32>(ref<u32>(q + 256)).store(0);  // lwarx / stwcx.
    store<u32>(batch, load<u32>(q + 24));
    store<u32>(batch + 4, 0x00024000u);
    for (u32 k = 8; k < 28; k += 4) store<u32>(batch + k, 0);
    call(ctx, g11::fn::DrawQueue_Build_00516970, {batch});
    ret(ctx, call(ctx, render::fn::Draw_InvalidateState));
}

// bool (ShaderType*, Material* m, const Desc* d)   (LoadParams_005C25B8
// passes 0x01196EC0; name?)
// The shared material setup behind the lit shader types, for a desc with
// u8 +84 (its field count) set: the base texture d+16 (or default 3) with
// virtual slot 4 (+16) (tex, 3, 3, 1); unless config +152 bit 0x80, the
// texture d+8 (sized as the surface m+20 +122 / +124) and params 0..2 =
// d+88..+96; unless bit 0x40 as well, the texture d+12 the same way. Then
// the params the count covers, else defaults (0x005BDD10): 4 = d+104
// (> 4; 15.0) clamped to 0 when not positive, 7 = d+116 (> 7; 2.5), 9 =
// d+124 (> 9; 2.0), 10 = d+128 (> 10; 1.0), 12 = d+136 (> 12; 1.0), and
// the colours 8 = d+120 (> 8) / 11 = d+132 (> 11), -1 by default, with
// bytes 1 and 3 swapped. Returns 1; f1 = param 4's value unrounded.
DECOMP(005BDD30, Material_SetupLit_005BDD30) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const auto param = [&](u32 k) { return static_cast<u32>(call(ctx, render::shader5::fn::Material_ParamSlot, {m, k})); };
    const u8 count = load<u8>(d + 84);
    if (count == 0) return ret(ctx, 1u);
    u32 tex = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 16)}));
    if (tex == 0) tex = static_cast<u32>(call(ctx, g11::fn::Tex_GetDefault, {3}));
    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, tex});
    fx::vcall(ctx, tex, 16, {tex, 3, 3, 1});
    const u32 flags = sly4::g::config + 152;
    if ((load<u8>(flags) & 0x80) == 0) {
        const u32 t2 = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 8)}));
        if (t2 != 0) {
            call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t2});
            const u32 surf = load<u32>(m + 20);
            const u32 w = load<u16>(surf + 122), h = load<u16>(surf + 124);
            const u32 opd = load<u32>(load<u32>(t2) + 16);
            call_ptr(ctx, opd, {t2, w, h, 1, h, w});  // r7 / r8 left as the original's shuffle
            for (u32 k = 0; k < 3; ++k) {
                const u32 v = load<u32>(d + 88 + 4 * k);  // lfs / stfs: raw
                store<u32>(param(k), v);
            }
            if ((load<u8>(flags) & 0x40) == 0) {
                const u32 t3 = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 12)}));
                if (t3 != 0) {
                    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t3});
                    const u32 s3 = load<u32>(m + 20);
                    fx::vcall(ctx, t3, 16, {t3, load<u16>(s3 + 122), load<u16>(s3 + 124), 1});
                }
            }
        }
    }
    constexpr u32 kK = 0x005BDD10;  // f32 15.0, 0, f64 0.0, f32 2.5, 2.0, 1.0
    // Param 4: fsel(-x, 0.0, x), then frsp (a NaN comes out quieted).
    const u32 x = load<u8>(d + 84) > 4 ? load<u32>(d + 104) : load<u32>(kK);
    const f64 xv = render::gfx10::lfs_value(x);
    const f64 sel = -xv >= 0.0 ? g11::lfd(kK + 8) : xv;
    const u32 p4 = std::isnan(sel) ? (x | 0x00400000u) : g11::f32_bits(sel);
    store<u32>(param(4), p4);
    const u8 c = load<u8>(d + 84);
    const u32 v7 = c > 7 ? load<u32>(d + 116) : load<u32>(kK + 16);
    const u32 v9 = c > 9 ? load<u32>(d + 124) : load<u32>(kK + 20);
    const u32 v10 = c > 10 ? load<u32>(d + 128) : load<u32>(kK + 24);
    const u32 v12 = c > 12 ? load<u32>(d + 136) : load<u32>(kK + 24);
    store<u32>(param(7), v7);
    store<u32>(param(9), v9);
    store<u32>(param(10), v10);
    store<u32>(param(12), v12);
    const u8 c2 = load<u8>(d + 84);
    const u32 col8 = c2 > 8 ? load<u32>(d + 120) : 0xFFFFFFFFu;
    const u32 col11 = c2 > 11 ? load<u32>(d + 132) : 0xFFFFFFFFu;
    const auto swap13 = [](u32 v) { return (v & 0xFF00FF00u) | ((v >> 16) & 0xFFu) | ((v & 0xFFu) << 16); };
    store<u32>(param(8), swap13(col8));
    store<u32>(param(11), swap13(col11));
    ctx.fpr[1] = sel;
    ret(ctx, 1u);
}

namespace {

// The parabola sine of the warp grid (constants at 0x00544DD8: f32 pi,
// 0.0, f64 pi, -pi, f32 2pi, 4/pi, -4/pi^2, 0.225, pi/2): x wrapped into
// [-pi, pi] first by the caller.
f64 warp_sine(f64 x, f64 b, f64 c, f64 p) {
    namespace g11 = sly4::render::gfx11;
    const f64 y = g11::madds(x, b, g11::sgl(g11::sgl(x * c) * std::fabs(x)));
    const f64 t = g11::msubs(y, std::fabs(y), y);
    return g11::madds(t, p, y);
}

}  // namespace

// void (Warp* w)   (Warp_Update_00545A38, before Warp_Draw; name?)
// Fills the current vertex buffer (+5796 + 16 * u32 +5780; +8 bits 28..30
// = 2) with the warp grid: u8 +5713 columns by u8 +5714 rows of 24-byte
// vertices {xyz, u8[4] colour +5736, uv}. Position: +5744 / +5748 (x0,
// y0) to +5752 / +5772 over (+5784, +5785) steps, z = -near of the
// current view. UV: the same corners over the screen size (config +4 /
// +6), offset inside the border by a wave: amplitude +5720 / +5724 per
// cell, phase (u * pi + v) * time (0x010DD5B0) * speed (+5728 / +5732),
// sine for x and cosine for y. Leaves r3 = u8 +5713, f1 = -near.
DECOMP(00544E08, Warp_FillGrid_00544E08) {
    namespace g11 = sly4::render::gfx11;
    using g11::sgl;
    const u32 w = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x00544DD8;
    const f64 time = g11::lfs(0x010DD5B0);
    const u32 buf = w + 5796 + (load<u32>(w + 5780) << 4);
    store<u32>(buf + 8, (load<u32>(buf + 8) & ~0xEu) | (2u << 1));
    const f64 x0 = g11::lfs(w + 5744), x1 = g11::lfs(w + 5752), y0 = g11::lfs(w + 5748), y1 = g11::lfs(w + 5772);
    const u32 last_col = load<u8>(w + 5784), last_row = load<u8>(w + 5785);
    u32 v = load<u32>(buf);
    const f64 sw = sgl(static_cast<f64>(load<u16>(sly4::g::config + 4)));
    const f64 sh = sgl(static_cast<f64>(load<u16>(sly4::g::config + 6)));
    const f64 u0 = sgl(x0 / sw), u1 = sgl(x1 / sw), v0 = sgl(y0 / sh), v1 = sgl(y1 / sh);
    const f64 nc = sgl(static_cast<f64>(last_col)), nr = sgl(static_cast<f64>(last_row));
    const f64 dx = sgl(sgl(x1 - x0) / nc), dy = sgl(sgl(y1 - y0) / nr);
    const f64 du = sgl(sgl(u1 - u0) / nc), dv = sgl(sgl(v1 - v0) / nr);

    const u32 view = static_cast<u32>(call(ctx, g11::fn::View_GetCurrent));
    u32 cols = load<u8>(w + 5713), rows = load<u8>(w + 5714);
    const u32 z_bits = load<u32>(view + 60) ^ 0x80000000u;  // fneg; stfs: raw
    const f64 z = render::gfx10::lfs_value(z_bits);
    const f64 amp_x = sgl(g11::lfs(w + 5720) / sgl(static_cast<f64>(cols)));
    const f64 amp_y = sgl(g11::lfs(w + 5724) / sgl(static_cast<f64>(rows)));
    ctx.fpr[1] = z;
    ctx.fpr[2] = amp_x;
    ctx.fpr[3] = amp_y;
    if (rows == 0) return ret(ctx, cols);

    const f64 pi = g11::lfs(kK), zero = g11::lfs(kK + 4), pi_d = g11::lfd(kK + 8), mpi_d = g11::lfd(kK + 16);
    const f64 two_pi = g11::lfs(kK + 24), b = g11::lfs(kK + 28), c = g11::lfs(kK + 32), p = g11::lfs(kK + 36),
              half_pi = g11::lfs(kK + 40);
    // x wrapped into [-pi, pi]: (x + s) - trunc((x + s) / 2pi) * 2pi - s, s = +-pi.
    const auto wrap = [&](f64 x, f64& out_s) {
        const f64 s = sgl(x >= 0.0 ? pi_d : mpi_d);
        const f64 a = sgl(s + x);
        out_s = s;
        return a;
    };
    for (u32 j = 0; j < rows; ++j) {
        if (cols == 0) continue;
        const f64 fj = sgl(static_cast<f64>(j));
        const f64 y = g11::madds(fj, dy, y0);
        const f64 vv = g11::madds(fj, dv, v0);
        u32 i = 0;
        do {
            const f64 fi = sgl(static_cast<f64>(i));
            store<u32>(v + 4, g11::f32_bits(y));
            store<u32>(v + 8, z_bits);
            store<u32>(v + 0, g11::f32_bits(g11::madds(fi, dx, x0)));
            f64 ox, oy;
            const u32 lc = load<u8>(w + 5784);
            u32 lr = 0;
            bool edge = i == 0 || i == lc || j == 0;
            if (!edge) {
                lr = load<u8>(w + 5785);
                edge = j == lr;
            }
            if (edge) {
                ox = oy = zero;
            } else {
                const f64 fu = sgl(fi / sgl(static_cast<f64>(lc)));
                const f64 speed_x = g11::lfs(w + 5728), speed_y = g11::lfs(w + 5732);
                const f64 fv = sgl(fj / sgl(static_cast<f64>(lr)));
                const f64 px = sgl(sgl(sgl(g11::madds(fv, pi, fu)) * time) * speed_x);
                const f64 py = sgl(sgl(sgl(g11::madds(fu, pi, fv)) * time) * speed_y);
                f64 sx, sy;
                const f64 ax = wrap(px, sx), ay = wrap(py, sy);
                const f64 qx = sgl(g11::trunc_d(sgl(ax / two_pi)));
                const f64 qy = sgl(g11::trunc_d(sgl(ay / two_pi)));
                const f64 wx = sgl(g11::nmsubs(qx, two_pi, ax) - sx);
                const f64 wy = sgl(g11::nmsubs(qy, two_pi, ay) - sy);
                ox = sgl(amp_x * warp_sine(wx, b, c, p));
                f64 cy = sgl(wy + half_pi);  // cosine: sine a quarter turn on
                if (cy > pi) cy = sgl(cy - two_pi);
                oy = sgl(amp_y * warp_sine(cy, b, c, p));
            }
            const f64 u = g11::madds(fi, du, u0);
            store<u32>(v + 20, g11::f32_bits(sgl(oy + vv)));
            store<u32>(v + 16, g11::f32_bits(sgl(u + ox)));
            for (u32 k = 0; k < 4; ++k) store<u8>(v + 12 + k, load<u8>(w + 5736 + k));
            v += 24;
            ++i;
            cols = load<u8>(w + 5713);
        } while (i < cols);
        rows = load<u8>(w + 5714);
    }
    ret(ctx, cols);
}

// void (ShadowProj* o, const f32 dir[3], s32 sphere_fit, Camera* cam,
// const f32 range[8])   (Proj_Update_0052BB60; name?)
// The shadow projection: a look-at from the origin along dir (up =
// 0x01164B60, or 0x01164B50 when | |dir . up| - 1 | < 1e-5), the camera
// frustum slice o+12 / o+16 (f_0052B4B8); its light-space bounds lo / hi,
// either from the corners (f_0052B2EC) or, with sphere_fit, the bounding
// sphere of the slice (Frustum_SliceSphere, Sphere_Transform by cam+128,
// through the look-at: centre +- radius), snapped to the texel grid u16
// o+32 / +34 (f_0052AF78); then the depth range (f_0052AA98, from range
// with cam+196 + o+20 / +24), the ortho box Mat4_Ortho(lo.x, hi.x, lo.y,
// hi.y, near, far), o+48.. = ortho * look-at (VMX), o+336 = |far - near|,
// f_004FDEA0(o+112, o+48) and virtual slot 7 (+28) (o, corners, cam+128,
// cam+64). r3 as that call leaves it.
DECOMP(0052B688, ShadowProj_Build_0052B688) {
    namespace g11 = sly4::render::gfx11;
    using g11::sgl;
    const u32 o = arg<u32>(ctx, 0), dir = arg<u32>(ctx, 1), cam = arg<u32>(ctx, 3), range = arg<u32>(ctx, 4);
    const u32 sphere_fit = arg<u32>(ctx, 2);
    Stack frame(ctx, 480);
    const u32 sp = frame.addr() - 112;  // the original's r1: offsets as there (112..591)
    constexpr u32 kFltMax = 0x7F7FFFFFu, kFltMin = 0xFF7FFFFFu;
    // Bounds {lo[3], -, hi[3]} at 192 (dead: overwritten before use).
    store<u32>(sp + 208, kFltMin);
    store<u32>(sp + 192, kFltMax);
    store<u32>(sp + 196, kFltMax);
    store<u32>(sp + 200, kFltMax);
    store<u32>(sp + 212, kFltMin);
    store<u32>(sp + 216, kFltMin);
    u32 up = g11::g::axis_b60;
    {
        constexpr u32 kK = 0x0052B67C;  // f32 1.0, 1e-5
        f64 d = sgl(g11::lfs(dir + 4) * g11::lfs(up + 4));
        d = g11::madds(g11::lfs(dir), g11::lfs(up), d);
        d = g11::madds(g11::lfs(dir + 8), g11::lfs(up + 8), d);
        d = std::fabs(sgl(std::fabs(d) - g11::lfs(kK)));
        if (d < g11::lfs(kK + 4)) up = g11::g::axis_b50;
    }
    for (u32 k = 0; k < 12; k += 4) store<u32>(sp + 144 + k, load<u32>(up + k));  // lfs / stfs: raw
    const u32 look = sp + 256, corners = sp + 448, lo = sp + 112, hi = sp + 128;
    call(ctx, sly4::render::shader5::fn::Matrix_LookAt, {look, g11::g::origin, dir, sp + 144});
    callf(ctx, g11::fn::Shadow_FrustumCorners_0052B4B8, {cam, ctx.gpr[4], ctx.gpr[5], corners},  // r4 / r5 not read
          {g11::lfs(o + 12), g11::lfs(o + 16)});
    const u32 view = cam + 128;
    if (sphere_fit == 0) {
        call(ctx, g11::fn::Shadow_Bounds_0052B2EC, {lo, hi, corners, view, look});
        call(ctx, g11::fn::Shadow_SnapBounds_0052AF78, {lo, hi, load<u16>(o + 32), load<u16>(o + 34)});
    } else {
        // The corners' 3x4 (32-byte rows) packed into 16-byte rows at 384.
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 12; k += 4) store<u32>(sp + 384 + 16 * r + k, load<u32>(corners + 32 * r + k));
        const u32 sphere = sp + 160;
        callf(ctx, g11::fn::Frustum_SliceSphere_0052B210, {sphere, sp + 384}, {g11::lfs(o + 12), g11::lfs(o + 16)});
        call(ctx, g11::fn::Sphere_Transform, {sphere, view, 0});
        // centre (w = 1) through the look-at: L0 * x + 0, + L1 * y, + L2 * z, + L3.
        const u32 s0 = load<u32>(sphere), s1 = load<u32>(sphere + 4), s2 = load<u32>(sphere + 8);
        store<u32>(sp + 176, s0);  // dead stores (the stvx below overwrites them)
        store<u32>(sp + 180, s1);
        store<u32>(sp + 184, s2);
        const f32 x = std::bit_cast<f32>(s0), y = std::bit_cast<f32>(s1), z = std::bit_cast<f32>(s2);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(look + 4 * k), x, 0.0f);
            v = std::fmaf(load<f32>(look + 16 + 4 * k), y, v);
            v = std::fmaf(load<f32>(look + 32 + 4 * k), z, v);
            store<f32>(sp + 176 + 4 * k, load<f32>(look + 48 + 4 * k) + v);
        }
        const f64 r = g11::lfs(sphere + 12), nr = -r;
        const f64 cz = -g11::lfs(sp + 184), cx = g11::lfs(sp + 176), cy = g11::lfs(sp + 180);
        store<u32>(lo, g11::f32_bits(sgl(nr + cx)));
        store<u32>(lo + 4, g11::f32_bits(sgl(nr + cy)));
        store<u32>(hi, g11::f32_bits(sgl(r + cx)));
        store<u32>(hi + 4, g11::f32_bits(sgl(r + cy)));
        store<u32>(lo + 8, g11::f32_bits(sgl(nr + cz)));
        store<u32>(hi + 8, g11::f32_bits(sgl(r + cz)));
        call(ctx, g11::fn::Shadow_SnapBounds_0052AF78, {lo, hi, load<u16>(o + 32), load<u16>(o + 34)});
    }
    // The depth range record at 224: range[0], near, range[2], -, range[4], far, range[6].
    const u32 rec = sp + 224, bounds = sp + 192;
    const f64 base = g11::lfs(cam + 196);
    const f64 dn = sgl(base + g11::lfs(o + 20)), df = sgl(base + g11::lfs(o + 24));
    store<u32>(rec, load<u32>(range));
    store<u32>(rec + 8, load<u32>(range + 8));
    store<u32>(rec + 16, load<u32>(range + 16));
    store<u32>(rec + 24, load<u32>(range + 24));
    store<u32>(rec + 4, g11::f32_bits(dn));
    store<u32>(rec + 20, g11::f32_bits(df));
    for (u32 k = 0; k < 12; k += 4) {
        store<u32>(bounds + k, load<u32>(lo + k));
        store<u32>(bounds + 16 + k, load<u32>(hi + k));
    }
    ctx.fpr[1] = dn;  // left in f1 / f2 (not read)
    ctx.fpr[2] = df;
    const u32 zn = sp + 576, zf = sp + 580;
    call(ctx, g11::fn::Shadow_DepthRange_0052AA98, {zn, zf, bounds, look, dir, rec});
    const u32 ortho = sp + 320;
    callf(ctx, g11::fn::Mat4_Ortho, {ortho},
          {g11::lfs(lo), g11::lfs(hi), g11::lfs(lo + 4), g11::lfs(hi + 4), g11::lfs(zn), g11::lfs(zf)});
    // o+48.. = ortho * look-at, per row k: O0 * L[k].x + 0, + O1 * L[k].y, + O2 * L[k].z (+ O3 for row 3).
    f32 out[4][4];
    for (u32 row = 0; row < 4; ++row) {
        const u32 l = look + 16 * row;
        const f32 lx = load<f32>(l), ly = load<f32>(l + 4), lz = load<f32>(l + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(ortho + 4 * k), lx, row == 3 ? load<f32>(ortho + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(ortho + 16 + 4 * k), ly, v);
            out[row][k] = std::fmaf(load<f32>(ortho + 32 + 4 * k), lz, v);
        }
    }
    const f64 depth = std::fabs(sgl(g11::lfs(zf) - g11::lfs(zn)));
    store<u32>(o + 336, g11::f32_bits(depth));
    const u32 m = (o + 48) & ~15u;  // stvx
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 4; ++k) store<f32>(m + 16 * row + 4 * k, out[row][k]);
    ctx.fpr[1] = depth;
    call(ctx, g11::fn::Matrix_Copy_004FDEA0, {o + 112, o + 48});
    ctx.gpr[3] = fx::vcall(ctx, o, 28, {o, corners, view, cam + 64});
}

namespace {

// In-place transpose of the 4x4 at m (lvx / stvx: 16-byte aligned).
void transpose_in_place11(u32 m) {
    m &= ~15u;
    u32 w[4][4];
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) w[i][k] = load<u32>(m + 16 * i + 4 * k);
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) store<u32>(m + 16 * i + 4 * k, w[k][i]);
}

// out = a * b (row-major 4x4 at a / b, f32 words), each element
// a[i][1] * b[1][j] rounded, then + a[i][0] * b[0][j], + a[i][2] * b[2][j],
// + a[i][3] * b[3][j] fused, stored as it is computed. Returns the last.
f64 mul4_11(u32 out, u32 a, u32 b) {
    namespace g11 = sly4::render::gfx11;
    f64 v = 0;
    for (u32 i = 0; i < 4; ++i)
        for (u32 j = 0; j < 4; ++j) {
            const u32 r = a + 16 * i;
            v = g11::sgl(g11::lfs(r + 4) * g11::lfs(b + 16 + 4 * j));
            v = g11::madds(g11::lfs(r), g11::lfs(b + 4 * j), v);
            v = g11::madds(g11::lfs(r + 8), g11::lfs(b + 32 + 4 * j), v);
            v = g11::madds(g11::lfs(r + 12), g11::lfs(b + 48 + 4 * j), v);
            store<u32>(out + 16 * i + 4 * j, g11::f32_bits(v));
        }
    return v;
}

}  // namespace

// void (const f32 src[16], f32 dst[16], -, -, s32 eye; f1 depth, f2
// strength)   (View_StereoMatrix_005483EC; name?)
// The stereo projection: eye 0 copies src. Otherwise src must be a plain
// perspective matrix (the off-axis terms 0, f2 in [0, 1], f1 >= 0; else
// "ParameterCheckError!" and dst = src for eye 1 / 2). With the screen
// (0x010F0E20 +160 size, +164 eye separation): s = +164 * f2 /
// ((+160 / 1.14734) * 2.54) and o = |s * f1 * src[2][3] / src[0][0]|;
// eye 1 (left) dst = T(+-s) * src^T * T(-+o) transposed, eye 2 (right)
// the mirror (T(x): identity with x at [0][3]). Any other eye only
// transposes dst in place. Leaves r3 = src (eye 0) or dst + 16.
DECOMP(00536290, Stereo_ProjectionMatrix_00536290) {
    namespace g11 = sly4::render::gfx11;
    using g11::sgl;
    const u32 src = arg<u32>(ctx, 0), dst = arg<u32>(ctx, 1);
    const s32 eye = arg<s32>(ctx, 4);
    if (eye == 0) {
        for (u32 k = 0; k < 64; k += 4) store<u32>(dst + k, load<u32>(src + k));  // lfs / stfs: raw
        ctx.fpr[1] = g11::lfs(src + 60);
        return ret(ctx, src);
    }
    Stack frame(ctx, 256);
    const u32 sp = frame.addr() - 112;  // the original's offsets 112..367
    const u32 t = sp + 112, a = sp + 176, b = sp + 240, c = sp + 304;
    const u32 sa = src & ~15u;  // lvx
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) store<u32>(t + 16 * i + 4 * k, load<u32>(sa + 16 * k + 4 * i));
    const auto el = [&](u32 i, u32 j) { return g11::lfs(t + 16 * i + 4 * j); };  // src^T [i][j]
    const u32 left = eye == 1 ? dst : 0, right = eye == 2 ? dst : 0;
    constexpr u32 kK = 0x0053627C;  // f32 1.14734, 2.54, 0.0, 1.0
    constexpr u32 kScreen = sly4::g::mem_stats;
    const f64 inches = sgl(sgl(g11::lfs(kScreen + 160) / g11::lfs(kK)) * g11::lfs(kK + 4));
    const f64 zero = g11::lfs(kK + 8);
    const f64 f1 = ctx.fpr[1], f2 = ctx.fpr[2];
    // T[i][j] = src[j][i]: the entries checked are src[1][0], [2][0], [3][0], [0][1], [2][1],
    // [3][1], [0][2], [1][2], [0][3], [1][3], [3][3].
    const bool ok = el(0, 1) == zero && el(0, 2) == zero && el(0, 3) == zero && el(1, 0) == zero &&
                    el(1, 2) == zero && el(1, 3) == zero && el(2, 0) == zero && el(2, 1) == zero &&
                    el(3, 0) == zero && el(3, 1) == zero && el(3, 3) == zero && !(f2 < zero) &&
                    !(f2 > g11::lfs(kK + 12)) && !(f1 < zero);
    if (!ok) {
        call(ctx, render::gfx10::fn::Libc_printf, {0x00AF3410u /* "ParameterCheckError!\n" */});
        for (const u32 out : {left, right}) {
            if (out == 0) continue;
            const u32 m = out & ~15u;  // stvx of src^T's rows
            for (u32 k = 0; k < 64; k += 4) store<u32>(m + k, load<u32>(t + k));
        }
    } else {
        const f64 s = sgl(sgl(g11::lfs(kScreen + 164) * f2) / inches);
        f64 o = sgl(-sgl(sgl(s * f1) * el(3, 2)) / el(0, 0));
        if (o < zero) o = -o;
        // A and B: identities.
        for (u32 k = 0; k < 128; k += 4) store<u32>(a + k, 0);
        for (u32 k = 0; k < 4; ++k) {
            store<u32>(a + 20 * k, 0x3F800000u);
            store<u32>(b + 20 * k, 0x3F800000u);
        }
        f64 last = o;
        if (left != 0) {
            store<u32>(b + 12, g11::f32_bits(o));
            store<u32>(a + 12, g11::f32_bits(-s));
            mul4_11(c, t, b);
            ctx.fpr[2] = mul4_11(left, a, c);
        }
        if (right != 0) {
            store<u32>(a + 12, g11::f32_bits(s));
            store<u32>(b + 12, g11::f32_bits(-o));
            mul4_11(c, t, b);
            last = mul4_11(right, a, c);
        } else if (left == 0) {
            last = o;
        }
        ctx.fpr[1] = last;
    }
    transpose_in_place11(dst);
    ret(ctx, dst + 16);
}

// bool (IffReader* r)   [an IFF chunk handler] (name?)
// Reads a texture's chunks (Iff_ReadChunks into a local Texture*). A
// reader with u8 +12 bit 0x04 stops there; otherwise a loaded texture
// (Iff_ReadChunks == 1 and +56 set) goes to the AglTexture allocator's
// load hook (*0x00F8F118: +8 OPD, called (tex, +12, 1) when set) and onto the list *0x010DDAF4 as
// {tex +48 hash, tex} (Array_Reserve to twice the capacity when full,
// peak +4, flags +6 |= 1). Returns 1.
DECOMP(0051D210, Iff_HandleTexture_0051D210) {
    const u32 r = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 out = frame.addr();
    store<u32>(out, 0);
    if ((load<u8>(r + 12) & 0x04) != 0) {
        call(ctx, render::gfx9::fn::Iff_ReadChunks, {r, out});
        return ret(ctx, 1u);
    }
    if (static_cast<u32>(call(ctx, render::gfx9::fn::Iff_ReadChunks, {r, out})) != 1) return ret(ctx, 1u);
    if (static_cast<u32>(call(ctx, g11::fn::Has56_0053A9C4, {load<u32>(out)})) == 0) return ret(ctx, 1u);
    const u32 hook = load<u32>(g11::g::texture_alloc);
    u32 tex = load<u32>(out);
    const u32 cb = load<u32>(hook + 8);
    if (cb != 0) {
        call_ptr(ctx, cb, {tex, load<u32>(hook + 12), 1, cb, load<u32>(cb)});  // r6 / r7: the OPD and entry
        tex = load<u32>(out);
    }
    const u32 hash = load<u32>(tex + 48);
    const u32 list = load<u32>(g11::g::texture_list);
    u32 count = load<u16>(list + 2);
    const u32 cap = load<u16>(list);
    if (static_cast<s32>(count) == static_cast<s32>(cap)) {
        call(ctx, post::fn::Array_Reserve, {list, (cap << 1) & 0xFFFE});
        count = load<u16>(list + 2);
    }
    store<u32>(load<u32>(list + 8) + (count << 3), hash);
    const u32 data = load<u32>(list + 8);
    const u32 n = load<u16>(list + 2);
    const u32 next = n + 1;
    store<u16>(list + 2, static_cast<u16>(next));
    store<u32>(data + (n << 3) + 4, tex);
    const u32 peak = load<u16>(list + 4);
    const u32 flags = load<u16>(list + 6);
    store<u16>(list + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(next & 0xFFFF) ? peak : next));
    store<u16>(list + 6, static_cast<u16>(flags | 1));
    ret(ctx, 1u);
}

// void (RenderTarget* t, u32 mask)   (name?)
// With u8 0x00F66FC0 set and config +152 bit 0x10 clear: the object
// t+656 gets virtual slot 0 (+0), each of its passes +4.. picked by mask
// bits 0..9 slot 4 (+16) when slot 3 (+12) says so, then the object slot
// 1 (+4). Ends with the state cache reset (r3 as it leaves it).
DECOMP(00522D18, RenderTarget_RunPasses_00522D18) {
    const u32 t = arg<u32>(ctx, 0), mask = arg<u32>(ctx, 1);
    const u32 o = load<u32>(t + 656);
    if (load<u8>(g11::g::skip_flag_66fc0) != 0 && (load<u8>(sly4::g::config + 152) & 0x10) == 0) {  // rlwinm 28,31,31
        fx::vcall(ctx, o, 0, {o});
        for (u32 i = 0; i < 10; ++i) {
            if ((mask & (1u << i)) == 0) continue;
            const u32 pass = load<u32>(o + 4 + 4 * i);
            if (static_cast<u32>(fx::vcall(ctx, pass, 12, {pass})) != 0) fx::vcall(ctx, pass, 16, {pass});
        }
        fx::vcall(ctx, o, 4, {o});
    }
    ret(ctx, call(ctx, render::fn::Draw_InvalidateState));
}

namespace {

// r = a * b with VMX: row i = b0 a[i].x + b1 a[i].y + b2 a[i].z (+ b3
// for row 3), fused per lane in vmaddfp order. Addresses rounded down to
// 16 (lvx).
void mul_affine11(u32 a, u32 b, f32 r[4][4]) {
    a &= ~15u;
    b &= ~15u;
    for (u32 i = 0; i < 4; ++i) {
        const f32 x = load<f32>(a + 16 * i), y = load<f32>(a + 16 * i + 4), z = load<f32>(a + 16 * i + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(b + 4 * k), x, i == 3 ? load<f32>(b + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(b + 16 + 4 * k), y, v);
            r[i][k] = std::fmaf(load<f32>(b + 32 + 4 * k), z, v);
        }
    }
}

// r transposed into a 64-byte stack block, then vertex constants 0..3.
u64 upload_mvp11(PPUContext& ctx, const f32 r[4][4]) {
    Stack frame(ctx, 64);
    const u32 t = frame.addr();
    for (u32 k = 0; k < 4; ++k)
        for (u32 i = 0; i < 4; ++i) store<f32>(t + 16 * k + 4 * i, r[i][k]);
    return call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 0, 16, t});
}

}  // namespace

// void (Obj* a, const f32 m[16])   (name?)
// Vertex constants 0..3 = (m, or the identity when a+24 is set) * the
// current view's matrix (+880), transposed.
DECOMP(005012A4, VpConst_SetModelView_005012A4) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 m = load<u32>(a + 24) != 0 ? g11::g::identity : arg<u32>(ctx, 1);
    const u32 view = load<u32>(load<u32>(sly4::draw::g::current_surface) + 672);
    f32 r[4][4];
    mul_affine11(m, view + 880, r);
    ret(ctx, upload_mvp11(ctx, r));
}

// bool (IffReader* r, Obj* o)   [an IFF chunk handler] (name?)
// A 20-byte record {0, 15.0, 1.0, u16 0, u16 0, u8 bit 7 cleared}
// (Mem_Alloc; 0 on failure) appended to the list of o's holder o+92
// (created by f_004FD8D4 on first use); when the type of the entry
// o+48 [count - 1] +16 is 0xA359CDE9 or 0xC7DFEF69, its +4 = that
// entry's +124 +16 (or 0.0) (written even for a failed allocation:
// address 4). Then the record's chunks. Returns 1.
DECOMP(00523C98, Iff_HandleRecord_00523C98) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    u32 holder = load<u32>(o + 92);
    if (holder == 0) {
        holder = static_cast<u32>(call(ctx, g11::fn::Owner_Create_004FD8D4, {o}));
        store<u32>(o + 92, holder);
    }
    const u32 list = load<u32>(holder);
    const u32 rec = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {20, 0, 16}));
    if (rec != 0) {
        const u8 flags = load<u8>(rec + 16);
        store<u32>(rec, 0);
        store<u32>(rec + 4, 0x41700000u);  // 15.0f
        store<u16>(rec + 12, 0);
        store<u32>(rec + 8, 0x3F800000u);  // 1.0f
        store<u16>(rec + 14, 0);
        store<u8>(rec + 16, static_cast<u8>(flags & 0x7F));
    }
    store<u32>(load<u32>(list + 8) + (load<u32>(list + 4) << 2), rec);
    store<u32>(list + 4, load<u32>(list + 4) + 1);
    const u32 idx = (load<u32>(load<u32>(holder) + 4) & 0xFFFF) - 1;
    const u32 e = load<u32>(load<u32>(load<u32>(o + 48) + (idx << 3)) + 16);
    const u32 type = load<u32>(load<u32>(e + 16) + 8);
    if (type == 0xA359CDE9u || type == 0xC7DFEF69u) {
        const u32 src = load<u32>(e + 124);
        const u32 v = src != 0 ? load<u32>(src + 16) : load<u32>(0x00523C90u);
        store<u32>(rec + 4, v);  // lfs / stfs: raw
        ctx.fpr[1] = render::gfx10::lfs_value(v);  // left in f1
    }
    call(ctx, render::gfx9::fn::Iff_ReadChunks, {r, rec});
    ret(ctx, 1u);
}

namespace {

// ApeTListAllocator<T>::GetAllocator() inlined (as gfx10's AglRenderSurface
// delete): the free list *slot, built on first use as a 60-byte
// ApeTFreeList {vtable vt, +4..+52 cleared, bit 0x8000 of u16 +54
// cleared, +56 the signature name} and FreeList_Init(list, elem_size,
// slot+4 heap, 0, 0); a failed allocation leaves it null.
u32 get_list11(PPUContext& ctx, u32 slot, u32 vt, u32 name, u32 elem_size) {
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
            call(ctx, render::gfx9::fn::FreeList_Init, {p, elem_size, heap, 0, 0});
        }
        fl = p;
        store<u32>(slot, fl);
    }
    return fl;
}

// The 16-byte buffer part {+0 p, +4 bit 0: pool 5, +8 bit 0: not owned,
// +12 tag in bits 28..31} freed as Pool_DestructPart does, inlined.
void free_part11(PPUContext& ctx, u32 part) {
    const u32 p = load<u32>(part);
    if (p == 0 || (load<u32>(part + 8) & 1) != 0) return;
    const u32 pool = (load<u32>(part + 4) & 1) != 0 ? 5u : 4u;
    call(ctx, render::gfx8::fn::Mem_FreePool, {p, pool, load<u32>(part + 12) >> 28});
    store<u32>(part, 0);
}

}  // namespace

// void (AglTopology* t)   [called through a pointer] (name?)
// ApeTListAllocator<AglTopology> delete: t's key +80 out of the map
// *0x010DDC80 (SortedMap_Remove79E7B0), the destructor (virtual slot 0),
// then t back to the free list *0x00F8F124 (112-byte entries; built on
// first use) through its virtual slot 4 (+16).
DECOMP(005261A0, AglTopology_Delete_005261A0) {
    const u32 t = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), load<u32>(t + 80));
    call(ctx, g11::fn::SortedMap_Remove79E7B0, {load<u32>(g11::g::topology_map), frame.addr()});
    fx::vcall(ctx, t, 0, {t});
    const u32 fl = get_list11(ctx, load<u32>(0x00F8F124u), 0x00B23E00u, 0x00AF12C9u, 112);
    ctx.gpr[3] = fx::vcall(ctx, fl, 16, {fl, t});
}

// void ()   (name?)
// Clears the effect slot table (0x01194ED8, 200-byte entries, count
// 0x01194FA0): per slot Obj_SetSlot20(+12, 0, 1), each of the +40
// objects +28.. virtual slot 2 (+8) and cleared, the same for +24; then
// the count and the pair 0x01195090 = 0.
DECOMP(005AC64C, EffectSlots_Clear_005AC64C) {
    namespace g11g = g11::g;
    for (u32 k = 0; k < load<u32>(g11g::effect_count); ++k) {
        const u32 e = g11g::effect_slots + 200 * k;
        call(ctx, g11::fn::Obj_SetSlot20, {load<u32>(e + 12), 0, 1});
        u32 n = load<u32>(e + 40);
        for (u32 j = 0; j < n; ++j) {
            const u32 at = e + 28 + 4 * j;
            const u32 p = load<u32>(at);
            if (p == 0) continue;
            fx::vcall(ctx, p, 8, {p});
            store<u32>(at, 0);
            n = load<u32>(e + 40);
        }
        const u32 q = load<u32>(e + 24);
        if (q != 0) {
            fx::vcall(ctx, q, 8, {q});
            store<u32>(e + 24, 0);
        }
    }
    store<u32>(g11g::effect_count, 0);
    store<u32>(g11g::effect_pair, 0);
    store<u32>(g11g::effect_pair + 4, 0);
    ret(ctx, 0x01190000u);  // r3 as left (lis)
}

// void (Warp* w)   [vtable 0x00B24070, the deleting destructor] (name?)
// For a non-null w: vtable 0x00B24070, the buffer parts +5828, +5812,
// +5796 freed (Pool_DestructPart inlined), the destructor body
// (Obj23650_DestructInline) and Mem_FreeFrom(w, 0).
DECOMP(007A3700, Warp_Delete_007A3700) {
    const u32 w = arg<u32>(ctx, 0);
    if (w == 0) return;
    store<u32>(w, 0x00B24070u);
    free_part11(ctx, w + 5828);
    free_part11(ctx, w + 5812);
    free_part11(ctx, w + 5796);
    call(ctx, g11::fn::Obj23650_DestructInline, {w});
    ret(ctx, call(ctx, render::gfx8::fn::Mem_FreeFrom, {w, 0}));
}

// void (AglTexture* t)   [called through a pointer] (name?)
// ApeTListAllocator<AglTexture> delete: t's hash +48 out of the texture
// map *0x010DDAF4 (SortedMap_Remove79E4C8), the destructor (virtual slot
// 0), then t back to the free list of *0x00F8F118 (132-byte entries;
// built on first use) through its virtual slot 4 (+16).
DECOMP(0051E1D8, AglTexture_Delete_0051E1D8) {
    const u32 t = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), load<u32>(t + 48));
    call(ctx, g11::fn::SortedMap_Remove79E4C8, {load<u32>(g11::g::texture_list), frame.addr()});
    fx::vcall(ctx, t, 0, {t});
    const u32 fl = get_list11(ctx, load<u32>(g11::g::texture_alloc), 0x00B23DA0u, 0x00AF11E0u, 132);
    ctx.gpr[3] = fx::vcall(ctx, fl, 16, {fl, t});
}

// Shader* (ShaderType*, Material* m, DrawItem* item)   [called through a
// pointer] (name?)
// SHO_DefaultType_SelectShader with distance LODs: with the item's
// material (+28) u8 +100 bit 0x80 and item+44 set, sel = *item+44 + 4
// (item u8 +69 = 1) when u8 m+44 is 1 or the item (+48) is farther from
// the camera (0x010DD8B0) than m's LOD distance (m+124 +12, else 0.0);
// else + 8 (+69 = 3) for u8 m+44 == 2, + 12 (+69 = 7) otherwise, and
// +69 |= 8 when (distance - m+124 +16)^2 < the squared distance (the
// fade band). Returns m+16 +12 [2 * sel + (m+128 != 0)]; r4 / r5 / f1 as
// left.
DECOMP(005CD310, Lod_SelectShader_005CD310) {
    namespace g11 = sly4::render::gfx11;
    using g11::sgl;
    const u32 m = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 mat = load<u32>(item + 28);
    store<u8>(item + 69, 0);
    u32 sel = 0;
    const u32 p = load<u32>(item + 44);
    if ((load<u8>(mat + 100) & 0x80) != 0 && p != 0) {
        const u8 kind = load<u8>(m + 44);
        if (kind == 1) {
            store<u8>(item + 69, 1);
            sel = load<u32>(p) + 4;
        } else {
            constexpr u32 kCam = render::g::camera_pos;
            const u32 pos = item + 48;
            const f64 dy = sgl(g11::lfs(kCam + 4) - g11::lfs(pos + 4));
            const f64 dx = sgl(g11::lfs(kCam) - g11::lfs(pos));
            const f64 dz = sgl(g11::lfs(kCam + 8) - g11::lfs(pos + 8));
            const f64 zero = g11::lfs(0x005CD308u);  // 0.0
            const u32 lod = load<u32>(m + 124);
            const f64 d2 = g11::madds(dz, dz, g11::madds(dx, dx, sgl(dy * dy)));
            const f64 near = lod != 0 ? g11::lfs(lod + 12) : zero;
            f64 f1 = zero;
            if (sgl(near * near) < d2) {
                store<u8>(item + 69, 1);
                sel = load<u32>(p) + 4;
            } else {
                u32 flags;
                if (kind == 2) {
                    flags = 3;
                    sel = load<u32>(p) + 8;
                } else {
                    flags = 7;
                    sel = load<u32>(p) + 12;
                }
                store<u8>(item + 69, static_cast<u8>(flags));
                const f64 band = lod != 0 ? g11::lfs(lod + 16) : zero;
                f1 = sgl(sgl(near - band) * sgl(near - band));
                if (f1 < d2) store<u8>(item + 69, static_cast<u8>(flags | 8));
            }
            ctx.fpr[1] = f1;
        }
    }
    const u32 idx = (sel << 1) | (load<u32>(m + 128) != 0 ? 1u : 0u);  // rlwimi
    const u32 table = load<u32>(load<u32>(m + 16) + 12);
    ctx.gpr[4] = table;  // left as the original leaves them
    ctx.gpr[5] = idx;
    ret(ctx, load<u32>(table + 4 * idx));
}

// void (s32 phase)   (name?)
// The frame phases (2 runs both halves): 0 / 2 flush the pending objects
// 0x010E05D4 (Call_00548938 each, count 0x010E0614 cleared); 1 / 2
// f_00548A10; 0 / 2 f_00511F8C + Debug_SwapPrimPools; 1 / 2 kick the job
// chain, f_00512148, Debug_ReleasePrimPool and rotate the surface words
// at 0x010DDA74 (+8 = +12, +12 = +4, +20 = +16, +16 ^= 1); 3 f_00511F8C,
// Debug_SwapPrimPools and Debug_ReleasePrimPool. r3 as left.
DECOMP(005124F4, Render_Phase_005124F4) {
    namespace f11 = g11::fn;
    const s32 n = arg<s32>(ctx, 0);
    u64 r3 = static_cast<u32>(n);
    const bool first = n == 0 || n == 2, second = n == 1 || n == 2;
    if (first) {
        u32 count = load<u32>(g11::g::pending_count);
        if (count != 0) {
            for (u32 i = 0; i < count; ++i) {
                call(ctx, f11::Call_00548938, {load<u32>(g11::g::pending_list + 4 * i)});
                count = load<u32>(g11::g::pending_count);
            }
            store<u32>(g11::g::pending_count, 0);
            r3 = count;
        }
    }
    if (second) r3 = call(ctx, f11::Fn_00548A10);
    if (first) {
        call(ctx, f11::Fn_00511F8C);
        r3 = call(ctx, f11::Debug_SwapPrimPools);
    }
    if (second) {
        const u32 q = static_cast<u32>(call(ctx, f11::JobQueue_Get));
        call(ctx, f11::JobChain_Kick, {q});
        call(ctx, f11::Fn_00512148);
        call(ctx, f11::Debug_ReleasePrimPool);
        constexpr u32 s = sly4::draw::g::current_surface;
        const u32 a = load<u32>(s + 16), b = load<u32>(s + 12), c = load<u32>(s + 4);
        store<u32>(s + 8, b);
        store<u32>(s + 12, c);
        store<u32>(s + 20, a);
        store<u32>(s + 16, a ^ 1);
        r3 = s;
    }
    if (n == 3) {
        call(ctx, f11::Fn_00511F8C);
        call(ctx, f11::Debug_SwapPrimPools);
        r3 = call(ctx, f11::Debug_ReleasePrimPool);
    }
    ret(ctx, r3);
}

// void (RecordStream* s)   [a record handler] (name?)
// Reads one word {u8 on, u8 ref, u16 func} from the stream (+0 start, +4
// end, +12 cursor; 4-aligned, wrapping to the start when a 16-byte record
// would pass the end, the word at +12 of it): on: stencil test on, func
// = the u16 (render state +160) with the cached ref +164 / mask +172,
// then ref = the u8 (+164) sent again; off: stencil test off, func
// ALWAYS (519). Leaves r3 = the gcm context, f1 = the word as a float.
DECOMP(005151E8, Record_Stencil_005151E8) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 start = load<u32>(s), end = load<u32>(s + 4);
    u32 c = load<u32>(s + 12);
    if (c + 15 > end) c = start;  // cmplw
    c = (c + 3) & ~3u;
    u32 at = c + 19 > end ? start : c + 12;
    at = (at + 3) & ~3u;
    store<u32>(s + 12, at + 4);
    const u32 w = load<u32>(at);  // lfs / stfs through the stack: raw
    ctx.fpr[1] = render::gfx10::lfs_value(w);  // left in f1
    const u32 on = w >> 24, ref = (w >> 16) & 0xFF, func = w & 0xFFFF;
    const u32 st = fx::g::state;
    constexpr u32 kStencilFunc = render::pool::kStencilFunc;
    if (on != 0) {
        fx::emit({fx::kStencilTestEnable, 1});
        store<u32>(st + 160, func);
        fx::emit({kStencilFunc, func, load<u32>(st + 164), load<u32>(st + 172)});
        store<u32>(st + 164, ref);
        const u32 mask = load<u32>(st + 172);
        fx::emit({kStencilFunc, load<u32>(st + 160), ref, mask});
    } else {
        fx::emit({fx::kStencilTestEnable, 0});
        store<u32>(st + 160, 519);
        fx::emit({kStencilFunc, 519, load<u32>(st + 164), load<u32>(st + 172)});
    }
    ret(ctx, load<u32>(st));
}

// void (Surface* s, ViewRect* viewport, ViewRect* scissor)   (name?)
// Surface_DefaultRects fills in null rects; then the viewport {x, y, w,
// h} (u16) with scale {w / 2, -h / 2, 1, 0}, offset {x + w / 2, y + h /
// 2, 0, 0} and depth 0..1 (Gcm_SetViewport), and the scissor from the
// same rect (SET_SCISSOR_HORIZONTAL / VERTICAL inline). Leaves r3 = the
// gcm context.
DECOMP(00532450, Draw_SetViewport_00532450) {
    namespace g11 = sly4::render::gfx11;
    using g11::sgl;
    Stack frame(ctx, 112);
    const u32 sp = frame.addr() - 112;  // the original's offsets 112..167
    const u32 rects = sp + 128;
    store<u32>(rects, arg<u32>(ctx, 1));
    store<u32>(rects + 4, arg<u32>(ctx, 2));
    call(ctx, g11::fn::Surface_DefaultRects_0051AE24, {arg<u32>(ctx, 0), rects, rects + 4});
    const u32 r = load<u32>(rects);
    const u32 w = load<u16>(r + 4), h = load<u16>(r + 6), x = load<u16>(r), y = load<u16>(r + 2);
    constexpr u32 kK = 0x0053243C;  // f32 0.5, -0.5, 1.0, 0.0
    const f64 half = g11::lfs(kK);
    const f64 fh = sgl(static_cast<f64>(h));
    const f64 sx = sgl(sgl(static_cast<f64>(w)) * half);
    const u32 scale = sp + 136, offset = sp + 152;
    store<u32>(scale, g11::f32_bits(sx));
    store<u32>(scale + 4, g11::f32_bits(sgl(fh * g11::lfs(kK + 4))));
    store<u32>(scale + 8, 0x3F800000u);
    store<u32>(scale + 12, 0);
    store<u32>(offset + 8, 0);
    store<u32>(offset + 12, 0);
    store<u32>(offset, g11::f32_bits(sgl(sgl(static_cast<f64>(x)) + sx)));
    store<u32>(offset + 4, g11::f32_bits(g11::madds(fh, half, sgl(static_cast<f64>(y)))));
    const u32 gcm = load<u32>(fx::g::state);
    // r8 / r9 are the float arguments' unused GPR slots (r8 holds y, as left).
    callf(ctx, g11::fn::Gcm_SetViewport, {gcm, x, y, w, h, y, ctx.gpr[9], scale, offset},
          {g11::lfs(kK + 12), g11::lfs(kK + 8)});
    fx::emit({0x000808C0u /* SET_SCISSOR_HORIZONTAL, 2 */, (w << 16) | x, (h << 16) | y});
    ret(ctx, load<u32>(fx::g::state));
}

// void (f32 zmin, f32 zmax)   (name?)
// Depth bounds test: off (0) when the range is 0..0, the depth test is
// off (render state u8 +93) or there is no depth target (+84); otherwise
// zmax scaled to the target format (+24: 4 / 6 * 65536, 5 / 7 * 2^24; any
// other format warns once (u8 0x011DE2C4) and turns it off) and
// DEPTH_BOUNDS_TEST_ENABLE 1, DEPTH_BOUNDS {zmin, zmax}. r3 = 0 when off,
// else the gcm context.
DECOMP(0079E2B8, Draw_SetDepthBounds_0079E2B8) {
    namespace g11 = sly4::render::gfx11;
    constexpr u32 kK = 0x0079E2A8;  // f32 0.0, 65536.0, 16777216.0
    const f64 zmin = ctx.fpr[1];
    f64 zmax = ctx.fpr[2];
    const f64 zero = g11::lfs(kK);
    ctx.fpr[2] = zero;
    const u32 st = fx::g::state;
    const u32 depth = load<u32>(st + 84);
    bool off = (zmin == zero && zmax == zero) || load<u8>(st + 93) == 0 || depth == 0;
    if (!off) {
        const s32 fmt = load<s32>(depth + 24);
        if (fmt == 4 || fmt == 6) {
            ctx.fpr[2] = g11::lfs(kK + 4);
            zmax = g11::sgl(zmax * ctx.fpr[2]);
        } else if (fmt == 5 || fmt == 7) {
            ctx.fpr[2] = g11::lfs(kK + 8);
            zmax = g11::sgl(zmax * ctx.fpr[2]);
        } else {
            if (load<u8>(g11::g::depth_bounds_warned) == 0) store<u8>(g11::g::depth_bounds_warned, 1);
            off = true;
        }
    }
    ctx.fpr[3] = zmax;
    constexpr u32 kEnable = 0x00040A68, kBounds = 0x00080A78;  // DEPTH_BOUNDS_TEST_ENABLE, DEPTH_BOUNDS (2)
    if (off) {
        fx::emit({kEnable, 0});
        return ret(ctx, 0u);
    }
    const u32 c = load<u32>(st);
    const u32 cur = load<u32>(c + 8);
    store<u32>(cur, kEnable);
    store<u32>(c + 8, cur + 8);  // the pointer advanced before the value is written
    store<u32>(cur + 4, 1);
    const u32 c2 = load<u32>(st);
    const u32 at = load<u32>(c2 + 8);
    store<u32>(at, kBounds);
    store<u32>(at + 4, g11::stfs_bits(zmin));
    store<u32>(at + 8, g11::stfs_bits(zmax));
    store<u32>(c2 + 8, at + 12);
    ret(ctx, c2);
}

// void (const char* layer_name, u32 a, u32 b, u32 c, s8 order)   (name?)
// Adds the pre-draw callback {a, b, c, order} to the render layer named
// layer_name (g::layers 0x010DDA8C, String_CompareNoCase on +16): the
// 16-byte records +20.. (u8 count +84) stay sorted by order, later
// records moved up one (memmove 13 bytes) while their order is greater.
// No bound check (the original's). Leaves r3 as the original does.
DECOMP(0051A79C, RenderLayers_AddPreDraw_0051A79C) {
    const u32 name = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 order_arg = arg<u32>(ctx, 4);
    constexpr u32 kLayers = render::g::layers;
    if (load<u32>(kLayers) == 0) return ret(ctx, name);
    u32 k = 0;
    for (;;) {
        const u32 l = load<u32>(kLayers + 4 + 4 * k);
        if (static_cast<u32>(call(ctx, g11::fn::String_CompareNoCase, {load<u32>(l + 16), name})) == 0) break;
        ++k;
        const u32 n = load<u32>(kLayers);
        if (k >= n) return ret(ctx, n);
    }
    const u32 at = kLayers + 4 + 4 * k;
    const u32 l = load<u32>(at);
    if (l == 0) return ret(ctx, at);
    const s32 order = static_cast<s8>(order_arg);
    s32 i = static_cast<s8>(load<u8>(l + 84) - 1);
    while (i >= 0 && static_cast<s8>(load<u8>(l + 32 + 16 * i)) > order) {
        call(ctx, g11::fn::memmove, {l + 36 + 16 * i, l + 20 + 16 * i, 13});
        i = static_cast<s8>(i - 1);
    }
    const u32 rec = l + 36 + 16 * static_cast<u32>(i);
    store<u8>(l + 84, static_cast<u8>(load<u8>(l + 84) + 1));
    store<u32>(rec, a);
    store<u32>(rec + 4, b);
    store<u32>(rec + 8, c);
    store<u8>(rec + 12, static_cast<u8>(order_arg));
    ret(ctx, rec);
}

// Trail* (Trail* o)   [constructor of vtable 0x00B23630] (name?)
// Fields to their defaults (constants at 0x0051EF08): +0 vtable, +4 / +5
// / +12 bytes and +8 / +32 / +80 / +84 0, +24 0.016, +28 0.1, +48.. and
// +64.. the origin, u8 +88 bits 0x80 / 0x40 cleared, +96..+104 1.0,
// +108 0.75 (twice), +112.. the vec4 0x01164BB0, +128 0.55, +132 1.0,
// +136 4.9e-5, +20 = 0.016 clamped into [0.016, 0.1] (fsel), and +80 =
// +20 * the steps for 0.75^n to fall under 0.05 (at most 60). f1 = +80.
DECOMP(0051EF28, Trail_Construct_0051EF28) {
    namespace g11 = sly4::render::gfx11;
    using g11::sgl;
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x0051EF08;  // f32 0.016, 0.1, 0.75, 0, f64 0.016, f32 1.0, 0.05
    store<u8>(o + 4, 0);
    store<u8>(o + 5, 0);
    store<u32>(o + 8, 0);
    store<u8>(o + 12, 0);
    store<u32>(o, 0x00B23630u);
    const f64 lo = g11::lfs(kK), hi = g11::lfs(kK + 4);
    store<u32>(o + 24, 0x3C83126Fu);
    store<u32>(o + 28, 0x3DCCCCCDu);
    store<u32>(o + 32, 0);
    const f64 span = sgl(hi - lo);
    constexpr u32 kOrigin = g11::g::origin;
    store<u32>(o + 48, load<u32>(kOrigin));  // lfs / stfs: raw
    store<u32>(o + 52, load<u32>(kOrigin + 4));
    store<u32>(o + 56, load<u32>(kOrigin + 8));
    const u8 flags = load<u8>(o + 88);
    store<u32>(o + 64, load<u32>(kOrigin));
    store<u32>(o + 68, load<u32>(kOrigin + 4));
    store<u32>(o + 80, 0);
    store<u32>(o + 72, load<u32>(kOrigin + 8));
    store<u32>(o + 84, 0);
    store<u8>(o + 88, static_cast<u8>(flags & ~0xC0u));
    store<u32>(o + 96, 0x3F800000u);
    store<u32>(o + 100, 0x3F800000u);
    store<u32>(o + 104, 0x3F800000u);
    store<u32>(o + 108, 0x3F400000u);
    const f64 step = g11::lfs(kK + 8);
    constexpr u32 kV = g11::g::vec_bb0;
    store<u32>(o + 112, load<u32>(kV));
    store<u32>(o + 116, load<u32>(kV + 4));
    store<u32>(o + 120, load<u32>(kV + 8));
    const u32 w = load<u32>(kV + 12);
    const f64 sel = sgl(span >= 0.0 ? g11::lfd(kK + 16) : hi);  // fsel + frsp
    const f64 floor = g11::lfs(kK + 28);
    store<u32>(o + 132, 0x3F800000u);
    store<u32>(o + 128, 0x3F0CCCCDu);
    store<u32>(o + 124, w);
    const f64 dt = sgl(sel - lo) >= 0.0 ? sel : lo;  // fsel
    store<u32>(o + 20, g11::stfs_bits(dt));
    store<u32>(o + 108, 0x3F400000u);
    const f64 t = g11::lfs(o + 20);
    f64 v = g11::lfs(kK + 24);
    u32 n = 0;
    for (;;) {
        v = sgl(v * step);
        if (v < floor) break;
        n = (n + 1) & 0xFFFF;
        if (static_cast<s32>(n) >= 60) break;
    }
    store<u32>(o + 136, 0x384D855Au);
    const f64 total = sgl(sgl(static_cast<f64>(n)) * t);
    store<u32>(o + 80, g11::f32_bits(total));
    ctx.fpr[1] = total;
    ret(ctx, o);
}

namespace {

// The destructor body of the 0x00B23690 effect (two identical copies):
// each member's vtable set to its own then to the base 0x00B234C8 (+6736,
// +6560, +6336, +6176), the Warp at +316 (vtable 0x00B24070; its parts
// +6144 / +6128 / +6112 through Pool_DestructPart), its child +336
// (vtable 0x00B23650 on +316 first) virtual slot 2 (+8), then +316,
// +284, +256, +192, +112, +48 to the base. Returns o+48.
u32 effect_destruct_body11(PPUContext& ctx, u32 o) {
    constexpr u32 kBase = 0x00B234C8;
    store<u32>(o, 0x00B23690u);
    store<u32>(o + 6560, 0x00B23FC0u);
    store<u32>(o + 6736, kBase);
    store<u32>(o + 6560, kBase);
    store<u32>(o + 6336, 0x00B24038u);
    store<u32>(o + 6336, kBase);
    store<u32>(o + 6176, 0x00B23FF0u);
    store<u32>(o + 6176, kBase);
    store<u32>(o + 316, 0x00B24070u);
    for (u32 part : {6144u, 6128u, 6112u}) call(ctx, render::gfx9::fn::Pool_DestructPart, {o + part});
    const u32 child = load<u32>(o + 336);
    if (child != 0) {
        store<u32>(o + 316, 0x00B23650u);
        fx::vcall(ctx, child, 8, {child});
    }
    store<u32>(o + 316, kBase);
    store<u32>(o + 284, kBase);
    store<u32>(o + 256, kBase);
    store<u32>(o + 192, 0x00B240B0u);
    store<u32>(o + 192, kBase);
    store<u32>(o + 112, 0x00B23F90u);
    store<u32>(o + 112, kBase);
    store<u32>(o + 48, 0x00B23F38u);
    store<u32>(o + 48, kBase);
    return o + 48;
}

void effect_destruct11(PPUContext& ctx) { ret(ctx, effect_destruct_body11(ctx, arg<u32>(ctx, 0))); }

// The deleting destructor: for a non-null o, the body then Mem_FreeFrom(o, 0).
void effect_delete11(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    effect_destruct_body11(ctx, o);
    ret(ctx, call(ctx, render::gfx8::fn::Mem_FreeFrom, {o, 0}));
}

}  // namespace

// void (Effect* o)   [destructor of vtable 0x00B23690] (name?)
DECOMP(007A1B48, WarpEffect_Destruct_007A1B48) { effect_destruct11(ctx); }

// void (Effect* o)   [the same destructor body, a second copy] (name?)
DECOMP(007A1E4C, WarpEffect_Destruct_007A1E4C) { effect_destruct11(ctx); }

// void (Effect* o)   [deleting destructor of vtable 0x00B23690] (name?)
DECOMP(007A1CBC, WarpEffect_Delete_007A1CBC) { effect_delete11(ctx); }

// void (Effect* o)   [the same deleting destructor, a second copy] (name?)
DECOMP(007A1FC0, WarpEffect_Delete_007A1FC0) { effect_delete11(ctx); }

// Texture* (RenderTarget* rt, u32 depth)   (name?)
// A texture over the render target's colour buffer: Texture_Alloc, name
// "Color-RenderTarget" (+16, 32 bytes, +47 = 0; hash +48), size u16 +52
// / +54 from rt+40's +36 / +38, the 24-byte CellGcmTexture rt+16 at
// +72, +56 / +60 = rt+0 / +4, +64 = rt+8's size bits and flags with bit 0
// set, +68 tag = rt+12's top bits, +96 = 0; then +108 =
// Target_Bind51B4C4(rt+40, depth, tex). Returns the texture.
DECOMP(0051E514, RenderTarget_ColourTexture_0051E514) {
    const u32 rt = arg<u32>(ctx, 0), depth = arg<u32>(ctx, 1);
    const u32 surf = load<u32>(rt + 40);
    const u32 w = load<u16>(surf + 36), h = load<u16>(surf + 38);
    const u32 tex = static_cast<u32>(call(ctx, render::gfx10::fn::Texture_Alloc));
    const u32 name = tex + 16;
    call(ctx, render::gfx10::fn::String_CopyN, {name, 0x00AF1B30u /* "Color-RenderTarget" */, 32});
    store<u8>(tex + 47, 0);
    store<u32>(tex + 48, static_cast<u32>(call(ctx, post::fn::String_HashNoCase, {name})));
    store<u16>(tex + 52, static_cast<u16>(w));
    store<u16>(tex + 54, static_cast<u16>(h));
    for (u32 k = 0; k < 24; k += 4) store<u32>(tex + 72 + k, load<u32>(rt + 16 + k));
    store<u32>(tex + 56, load<u32>(rt));
    store<u32>(tex + 60, load<u32>(rt + 4));
    u32 v = (load<u32>(tex + 64) & 0xFu) | (load<u32>(rt + 8) & ~0xFu);
    const u32 tag = load<u32>(tex + 68) & 0x0FFFFFFFu;
    store<u32>(tex + 64, v);
    v = (v & ~0xEu) | (load<u32>(rt + 8) & 0xEu);
    store<u32>(tex + 64, v);
    const u32 top = load<u32>(rt + 12) & 0xF0000000u;
    store<u32>(tex + 64, v | 1);
    store<u32>(tex + 96, 0);
    store<u32>(tex + 68, tag | top);
    store<u32>(tex + 108, static_cast<u32>(call(ctx, g11::fn::Target_Bind51B4C4, {surf, depth, tex})));
    ret(ctx, tex);
}

// void ()   (name?)
// With u8 0x00F66FD5 set: for each active (u8 +4) of the 16 query slots
// (layers3 slots 0x010DFAD0, 176 bytes), each of its 4 pending records
// (+80 + 20 * j: +0 / +4 results, +8 / +12 report indices, u8 +16 bit 0
// issued, bits 1 / 2 results in) takes the result of each report whose
// status word (*0x011640E8 +12 + 16 * index) is 0 (the value at +8);
// then the ring index u16 +160 steps on (mod 4) and that record's flags
// are cleared.
DECOMP(00548A10, Query_PollResults_00548A10) {
    if (load<u8>(g11::g::queries_on) == 0) return ret(ctx, 0u);
    const u32 table = load<u32>(render::pool::query::g::report_base);
    const u32 values = table + 8, status = table + 12;
    for (u32 k = 0; k < 16; ++k) {
        const u32 e = render::layers3::g::slots + 176 * k;
        if (load<u8>(e + 4) == 0) continue;
        for (u32 j = 0; j < 4; ++j) {
            const u32 s = e + 80 + 20 * j;
            const u8 flags = load<u8>(s + 16);
            if ((flags & 1) == 0) continue;
            const u32 a = load<u32>(s + 8), b = load<u32>(s + 12);
            u8 f = flags;
            if ((flags & 2) == 0 && load<u32>(status + (a << 4)) == 0) {
                f = static_cast<u8>(flags | 2);
                const u32 v = load<u32>(values + (a << 4));
                store<u8>(s + 16, f);
                store<u32>(s, v);
            }
            if ((flags & 4) == 0 && load<u32>(status + (b << 4)) == 0) {
                f = static_cast<u8>(f | 4);
                const u32 v = load<u32>(values + (b << 4));
                store<u8>(s + 16, f);
                store<u32>(s + 4, v);
            }
        }
        const s32 c = static_cast<s32>(load<u16>(e + 160)) + 1;
        const u32 next = static_cast<u32>(c - (c / 4) * 4);
        store<u16>(e + 160, static_cast<u16>(next));
        store<u8>(e + 96 + 20 * (next & 0xFFFF), 0);
    }
    ret(ctx, status);  // r3 as left
}

namespace {

constexpr u32 kDepthBoundsEnable = 0x00040A68;  // NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE

// The inlined depth-bounds setter (Draw_SetDepthBounds_0079E2B8) for the
// range 0..0 its callers pass as constants: always DEPTH_BOUNDS_TEST_ENABLE
// 0 (the depth-format paths the original keeps are unreachable).
void depth_bounds_off11() { fx::emit({kDepthBoundsEnable, 0}); }

}  // namespace

// void ()   (f_00549F30; name?)
// Colour writes on (COLOR_MASK 0x01010101), then the depth bounds test
// off. Leaves r3 = the gcm context, f1 = f2 = 0.0 (0x00549854).
DECOMP(00549860, Draw_ColourOnBoundsOff_00549860) {
    fx::emit({fx::kColorMask, 0x01010101u});
    depth_bounds_off11();
    ctx.fpr[1] = ctx.fpr[2] = g11::lfs(0x00549854u);
    ret(ctx, load<u32>(fx::g::state));
}

// void (ShaderType*, Material* m)   [called through a pointer] (name?)
// When the material's depth-bounds parameter (m+124 +16; none: 0.0 from
// 0x005B0AE0) is not 0.0: the depth bounds test off (the inlined setter
// with the constant range 0..0). r3 = m+124 on the early out, else 0.
DECOMP(005B0AF0, Material_DepthBounds_005B0AF0) {
    const u32 m = arg<u32>(ctx, 1);
    const f64 zero = g11::lfs(0x005B0AE0u);
    const u32 p = load<u32>(m + 124);
    const f64 v = p != 0 ? g11::lfs(p + 16) : zero;
    ctx.fpr[1] = zero;
    if (v == zero) {
        ctx.fpr[2] = v;
        return ret(ctx, p);
    }
    ctx.fpr[2] = zero;
    depth_bounds_off11();
    ret(ctx, 0u);
}

// bool (IffReader* r, Obj* o)   [an IFF chunk handler] (name?)
// Iff_HandleForce_00528F20 with a leading u8 (read, unused) and
// "noForceTime": vec3 "force" and f32 "noForceTime" (both 0 unless
// read) into a new 48-byte force record (vtable 0x00B23260, u16
// refcount +4 = 1; null when the allocation fails) that replaces o+120
// (new one's count up, the old one's down and released through its
// virtual slot 1 (+4) at zero), the creation reference dropped; +16..
// +24 = force, +32 = noForceTime (written through a null record when the
// allocation failed). Returns 1; f1 = noForceTime, f2 / f3 = force y / z.
DECOMP(0052906C, Iff_HandleForceTime_0052906C) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 32);
    const u32 b = frame.addr();  // force xyz, noForceTime (zeroed: stvx), +16 the u8
    for (u32 k = 0; k < 16; k += 4) store<u32>(b + k, 0);
    call(ctx, render::gfx10::fn::Iff_ReadU8, {r, b + 16, 0});
    call(ctx, render::gfx10::fn::FileReader_ReadVec3Swapped, {r, b, 0x00B23B8Du});  // "force"
    call(ctx, render::gfx10::fn::FileReader_ReadF32Swapped, {r, b + 12, 0x00B23B93u});  // "noForceTime"
    const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {48, 0, 16}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p + 24, 0);
        store<u32>(p, 0x00B23260u);
        store<u32>(p + 20, 0);
        store<u32>(p + 16, 0);
        store<u32>(p + 32, 0);
    }
    const u32 old = load<u32>(o + 120);
    if (old != p) {
        store<u32>(o + 120, p);
        if (p != 0) store<u16>(p + 4, static_cast<u16>(load<u16>(p + 4) + 1));
        if (old != 0) {
            const u32 n = load<u16>(old + 4) - 1u;
            store<u16>(old + 4, static_cast<u16>(n));
            if (static_cast<s16>(n) <= 0) fx::vcall(ctx, old, 4, {old});
        }
    }
    u32 q = load<u32>(o + 120);
    store<u16>(q + 4, static_cast<u16>(load<u16>(q + 4) - 1));
    q = load<u32>(o + 120);
    for (u32 k = 0; k < 12; k += 4) store<u32>(q + 16 + k, load<u32>(b + k));  // lfs / stfs: raw
    store<u32>(load<u32>(o + 120) + 32, load<u32>(b + 12));
    ctx.fpr[1] = render::gfx10::lfs_value(load<u32>(b + 12));
    ctx.fpr[2] = render::gfx10::lfs_value(load<u32>(b + 4));
    ctx.fpr[3] = render::gfx10::lfs_value(load<u32>(b + 8));
    ret(ctx, 1u);
}

// bool (ShaderType*, Material* m, const Desc* d)   [a ShaderType's slot 1] (name?)
// Material parameters from a desc of u8 +84 fields: 0 = d+88 as an
// integer clamped to 255 (0 without fields), 1..6 = d+92..+112 where the
// count covers them, else 0x005CE360's float. Returns 1.
DECOMP(005CE368, LoadParams_005CE368) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const auto param = [&](u32 k) { return static_cast<u32>(call(ctx, render::shader5::fn::Material_ParamSlot, {m, k})); };
    u32 v0 = 0;
    if (load<u8>(d + 84) != 0) v0 = std::min<u32>(load<u32>(d + 88), 255);
    store<u32>(param(0), v0);
    const u32 def = load<u32>(0x005CE360u);
    for (u32 k = 1; k <= 6; ++k) {
        const u32 v = load<u8>(d + 84) > k ? load<u32>(d + 88 + 4 * k) : def;  // lfs / stfs: raw
        store<u32>(param(k), v);
    }
    ret(ctx, 1u);
}

// void (View* v)   (name?)
// Records the view's state into history entry u8 +652 (128 bytes at
// +1280): the u16 rect +28.. {x, y, w, h} at +0, +8 = 0, +12 = 1.0, the
// matrix +48.. transposed (VMX) at +16.., and the viewport scale {w / 2,
// -h / 2, 1, 0} at +80 / offset {x + w / 2, y + h / 2, 0, 0} at +96 (as
// Draw_SetViewport_00532450 computes them), u8 +112 = 0. Leaves r3 = h,
// f1 = the offset's y.
DECOMP(00549410, View_RecordState_00549410) {
    namespace g11 = sly4::render::gfx11;
    using g11::sgl;
    const u32 v = arg<u32>(ctx, 0);
    const u32 e = v + 1280 + (static_cast<u32>(load<u8>(v + 652)) << 7);
    const u32 r = v + 28;
    store<u16>(e, load<u16>(r));
    store<u16>(e + 2, load<u16>(r + 2));
    store<u32>(e + 8, 0);
    store<u16>(e + 4, load<u16>(r + 4));
    store<u32>(e + 12, 0x3F800000u);
    store<u16>(e + 6, load<u16>(r + 6));
    {
        const u32 src = (v + 48) & ~15u, dst = (e + 16) & ~15u;  // lvx / stvx
        u32 w[4][4];
        for (u32 i = 0; i < 4; ++i)
            for (u32 k = 0; k < 4; ++k) w[i][k] = load<u32>(src + 16 * i + 4 * k);
        for (u32 i = 0; i < 4; ++i)
            for (u32 k = 0; k < 4; ++k) store<u32>(dst + 16 * i + 4 * k, w[k][i]);
    }
    const u32 w = load<u16>(r + 4), h = load<u16>(r + 6), x = load<u16>(r), y = load<u16>(r + 2);
    constexpr u32 kK = 0x00549404;  // f32 0.5, -0.5
    const f64 half = g11::lfs(kK);
    const f64 sx = sgl(sgl(static_cast<f64>(w)) * half);
    const f64 fh = sgl(static_cast<f64>(h)), fy = sgl(static_cast<f64>(y));
    store<u32>(e + 80, g11::f32_bits(sx));
    store<u32>(e + 88, 0x3F800000u);
    store<u32>(e + 92, 0);
    store<u32>(e + 84, g11::f32_bits(sgl(fh * g11::lfs(kK + 4))));
    store<u32>(e + 96, g11::f32_bits(sgl(sgl(static_cast<f64>(x)) + sx)));
    store<u32>(e + 104, 0);
    store<u32>(e + 108, 0);
    store<u8>(e + 112, 0);
    const f64 oy = g11::madds(fh, half, fy);
    store<u32>(e + 100, g11::f32_bits(oy));
    ctx.fpr[1] = oy;
    ctx.fpr[2] = fy;
    ctx.fpr[3] = fh;
    ret(ctx, h);
}

namespace {

// The 0x00B23308 emitter's destructor body (two copies): with a ring
// buffer o+72: vtable 0x00B23308, its data freed (SmallFree(+0, count
// +16 * 36)) and the buffer back to the ApeTMemClassFL<ApeTRingBuf<
// AglSimplePtl>, 1024> free list *0x011DE4B4 (built on first use: 60
// bytes, vtable 0x00B0EF68, FreeList_Init(list, 24, 1024, 0, 0)) through
// its virtual slot 4 (+16). Then vtable 0x00B232D8, EmitPropsRef_Set(o+4,
// 0) and EmitPropsRef_Release(o+4).
u64 emitter_destruct_body11(PPUContext& ctx, u32 o) {
    const u32 ring = load<u32>(o + 72);
    if (ring != 0) {
        const u32 n = load<u32>(ring + 16);
        const u32 data = load<u32>(ring);
        store<u32>(o, 0x00B23308u);
        call(ctx, render::gfx9::fn::SmallFree, {data, n * 36});
        constexpr u32 kSlot = 0x011DE4B4;
        u32 fl = load<u32>(kSlot);
        if (fl == 0) {
            const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {60, 0, 16}));
            if (p != 0) {
                for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(p + off, 0);
                const u32 h = load<u16>(p + 54) & ~0x8000u;
                store<u16>(p + 52, 0);
                store<u16>(p + 54, static_cast<u16>(h));
                store<u32>(p, 0x00B0EF68u);
                store<u32>(p + 56, 0x00AF1066u);  // the GetAllocator() signature
                call(ctx, render::gfx9::fn::FreeList_Init, {p, 24, 1024, 0, 0});
            }
            fl = p;
            store<u32>(kSlot, fl);
        }
        fx::vcall(ctx, fl, 16, {fl, ring});
    }
    const u32 ref = o + 4;
    store<u32>(o, 0x00B232D8u);
    call(ctx, render::gfx8::fn::EmitPropsRef_Set, {ref, 0, 0x00B232D8u /* r5 as left */});
    return call(ctx, 0x0079CA4Cu /* EmitPropsRef_Release_0079CA4C */, {ref});
}

void emitter_destruct11(PPUContext& ctx) { ret(ctx, emitter_destruct_body11(ctx, arg<u32>(ctx, 0))); }

}  // namespace

// void (Emitter* o)   [destructor of vtable 0x00B23308] (name?)
DECOMP(00500FD0, Emitter_Destruct_00500FD0) { emitter_destruct11(ctx); }

// void (Emitter* o)   [the same destructor body, a second copy] (name?)
DECOMP(007A08CC, Emitter_Destruct_007A08CC) { emitter_destruct11(ctx); }

// void (Emitter* o)   [deleting destructor of vtable 0x00B23308] (name?)
// For a non-null o: the destructor body, then Mem_FreeFrom(o, 0).
DECOMP(007A0A58, Emitter_Delete_007A0A58) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    emitter_destruct_body11(ctx, o);
    ret(ctx, call(ctx, render::gfx8::fn::Mem_FreeFrom, {o, 0}));
}

// AglRenderTarget* (u32 arg)   (name?)
// ApeTListAllocator<AglRenderTarget>: an entry from the free list
// *0x00F8F11C (1840-byte entries; built on first use) through its virtual
// slot 3 (+12); when it gives one: f_00521110(rt, arg), vtable
// 0x00B240D0, u8 +1816 = 0, u64 +1808 = 0, +1824 = 4.0, +1828 = 0.01,
// u8 +1776 bits 0x80..0x10 cleared. Returns it (0 when the list had none).
DECOMP(0052317C, AglRenderTarget_New_0052317C) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 fl = get_list11(ctx, load<u32>(0x00F8F11Cu), 0x00B23DC0u, 0x00AF122Cu, 1840);
    const u32 rt = static_cast<u32>(fx::vcall(ctx, fl, 12, {fl}));
    if (rt == 0) return ret(ctx, 0u);
    call(ctx, g11::fn::RenderTarget_Init_00521110, {rt, a});
    const u8 flags = load<u8>(rt + 1776);
    store<u8>(rt + 1816, 0);
    store<u32>(rt, 0x00B240D0u);
    store<u64>(rt + 1808, 0);
    store<u32>(rt + 1824, 0x40800000u);  // 4.0f
    store<u8>(rt + 1776, static_cast<u8>(flags & 0x0F));
    store<u32>(rt + 1828, 0x3C23D70Au);  // 0.01f
    ret(ctx, rt);
}

// bool (IffReader* r, ShadowMesh* o)   [an IFF chunk handler] (name?)
// A u8 (read, unused), u16 "numVerts", u16 "numEdges", u8 "isSkinned";
// with both counts: o+0 = verts, o+4 = Mem_AllocPool(128, 6 * verts
// rounded up to 8, 0, 0) (the peak count u16 0x010DD590 kept), skinned:
// o+16 / +20 = two Mem_Alloc(4 * (verts + 7) & 0x3FFE0, 0, 16); o+8 =
// edges, o+12 = Mem_Alloc(5 * edges rounded up to 16, 0, 16). Returns 1.
DECOMP(00523F74, Iff_HandleShadowMesh_00523F74) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 b = frame.addr();  // +0 verts, +2 edges, +4 skinned, +6 the unused u8
    store<u16>(b, 0);
    store<u16>(b + 2, 0);
    store<u16>(b + 4, 0);
    call(ctx, render::gfx10::fn::Iff_ReadU8, {r, b + 6, 0});
    call(ctx, g11::fn::Iff_ReadU16, {r, b, 0x00B238FAu});      // "numVerts"
    call(ctx, g11::fn::Iff_ReadU16, {r, b + 2, 0x00B23903u});  // "numEdges"
    call(ctx, render::gfx10::fn::Iff_ReadU8, {r, b + 4, 0x00B2390Cu});  // "isSkinned"
    const u32 verts = load<u16>(b);
    if (verts == 0 || load<u16>(b + 2) == 0) return ret(ctx, 1u);
    const u32 s = (verts + 7) & 0xFFF8u;
    store<u16>(o, static_cast<u16>(verts));
    store<u32>(o + 4, static_cast<u32>(call(ctx, g11::fn::Mem_AllocPool, {128, (s * 3) << 1, 0, 0})));
    const u32 n = load<u16>(o);
    if (static_cast<s32>(n) > static_cast<s32>(load<u16>(g11::g::shadow_verts_peak)))
        store<u16>(g11::g::shadow_verts_peak, static_cast<u16>(n));
    if (load<u8>(b + 4) != 0) {
        const u32 bytes = ((load<u16>(b) + 7u) << 2) & 0x0003FFE0u;
        store<u32>(o + 16, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {bytes, 0, 16})));
        store<u32>(o + 20, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {bytes, 0, 16})));
    }
    const u32 edges = load<u16>(b + 2);
    store<u16>(o + 8, static_cast<u16>(edges));
    const u32 t = (edges + 15) & 0xFFF0u;
    store<u32>(o + 12, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {t + (t << 2), 0, 16})));
    ret(ctx, 1u);
}

// void (ThermalVision* o)   (name?)
// When the effect is on (virtual slot 3 (+12)): once (u8 0x00F67A02) the
// empty textures +20 / +24 / +28 become Tex_GetDefault(0); the scene
// texture over the colour target (mem_stats +100 with config +34 set,
// else +84) is made once and kept (0x00F67504 / 0x00F67500, via
// RenderTarget_ColourTexture_0051E514) into +20; with one: the random
// pair, ThermalVision_Draw, layer 16's draw (virtual slot 4) and the
// state cache reset. r3 as left.
DECOMP(0054664C, ThermalVision_Update_0054664C) {
    namespace g11g = g11::g;
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(fx::vcall(ctx, o, 12, {o})) == 0) return ret(ctx, 0u);
    if (load<u8>(g11g::thermal_defaults_set) == 0) {
        store<u8>(g11g::thermal_defaults_set, 1);
        const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_GetDefault, {0}));
        const u32 a = load<u32>(o + 20), b = load<u32>(o + 24), c = load<u32>(o + 28);
        if (a == 0) store<u32>(o + 20, t);
        if (b == 0) store<u32>(o + 24, t);
        if (c == 0) store<u32>(o + 28, t);
    }
    const bool eye = load<u8>(sly4::g::config + 34) != 0;
    const u32 slot = eye ? g11g::thermal_tex_eye : g11g::thermal_tex_alt;
    u32 tex = load<u32>(slot);
    if (tex == 0) {
        const u32 target = load<u32>(sly4::g::mem_stats + (eye ? 100 : 84));
        if (target != 0) {
            tex = static_cast<u32>(call(ctx, g11::fn::RenderTarget_ColourTexture_0051E514, {target, 0}));
            store<u32>(slot, tex);
        }
    }
    store<u32>(o + 20, tex);
    if (tex == 0) return ret(ctx, 0u);
    call(ctx, g11::fn::Random_Pair_0051F660, {o});
    call(ctx, g11::fn::ThermalVision_Draw, {o});
    const u32 layer = load<u32>(render::g::layers + 68);
    fx::vcall(ctx, layer, 16, {layer});
    ret(ctx, call(ctx, render::fn::Draw_InvalidateState));
}

// void (Trail* o)   (name?)
// f_0051E8E0(o, +48, +64, +80) and f_0051EB20(o) (its f1 the target);
// with u8 +20 bit 0x40 and a target over 0.0: target = target * 0.5 +
// 0.5 and bit 0x80 set, else target 0.0 and bit 0x80 clear (+20 written
// only when that bit changes); +88 = target. Then +84 eases toward it by
// at most the SystemState's +4 (0x011DE094, built on first use) * 0.7
// going up / * 2.0 going down. Leaves r3 = the constants' address
// (SystemState when +84 is already there), f1 = the new +84.
DECOMP(0051ED00, Trail_UpdateFade_0051ED00) {
    namespace g11 = sly4::render::gfx11;
    using g11::sgl;
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, g11::fn::Trail_Fn_0051E8E0, {o, o + 48, o + 64, o + 80});
    call(ctx, g11::fn::Trail_Fn_0051EB20, {o});
    f64 t = ctx.fpr[1];
    constexpr u32 kK = 0x0051ECEC;  // f32 0.0, 0.5, 0.7, 2.0
    const u8 flags = load<u8>(o + 20);
    u32 on = 0;
    if ((flags & 0x40) != 0 && t > g11::lfs(kK)) {
        on = 1;
        t = g11::madds(t, g11::lfs(kK + 4), g11::lfs(kK + 4));
    } else {
        t = g11::lfs(kK);
    }
    if (on != ((flags >> 7) & 1u)) store<u8>(o + 20, static_cast<u8>((flags & 0x7F) | (on << 7)));
    store<u32>(o + 88, g11::stfs_bits(t));
    namespace ss = sly4::system_state;
    constexpr u32 st = ss::object;
    if (load<u8>(ss::guard) == 0) {  // system_state::get inlined (built on first use)
        const u8 b = load<u8>(st + 32);
        store<u32>(st + 20, 0);
        store<u32>(st + 24, 0);
        store<u8>(st + 32, static_cast<u8>(b & 0x3F));
        call(ctx, sly4::fn::memset, {st + 92, 0, 400});
        t = g11::lfs(o + 88);
        store<u32>(ss::registry_slot, st);
        store<u8>(ss::guard, 1);
    }
    const f64 cur = g11::lfs(o + 84), rate = g11::lfs(st + 4);
    if (cur < t) {
        const f64 d = sgl(t - cur), lim = sgl(rate * g11::lfs(kK + 12));
        const f64 step = sgl(d - lim) >= 0.0 ? lim : d;  // fsel
        const f64 v = sgl(cur + step);
        store<u32>(o + 84, g11::f32_bits(v));
        ctx.fpr[1] = v;
        return ret(ctx, kK);  // r3 as left
    } else if (cur > t) {
        const f64 d = sgl(cur - t), lim = sgl(rate * g11::lfs(kK + 8));
        const f64 step = sgl(d - lim) >= 0.0 ? lim : d;  // fsel
        const f64 v = sgl(cur - step);
        store<u32>(o + 84, g11::f32_bits(v));
        ctx.fpr[1] = v;
        return ret(ctx, kK);
    } else {
        ctx.fpr[1] = t;
    }
    ret(ctx, st);
}

// void (FrameBuffer* fb, const u16 rect[4])   (name?)
// The scissor: fb's rect (+202 x, +204 y, +206 w, +208 h), each edge
// clamped into rect when one is given; unless it equals the cached one
// (+164..+170), cached and sent as a type-9 record {9 << 24, the u16 the
// failing compare loaded (the original's leftover), 0} then {x, y, w,
// h} into the record stream (+0 start, +4 end, +16 cursor; 4-aligned,
// wrapping to the start). Returns fb.
DECOMP(0052173C, FrameBuffer_SetScissor_0052173C) {
    const u32 fb = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    s32 x0 = load<u16>(fb + 202), y0 = load<u16>(fb + 204);
    s32 x1 = load<u16>(fb + 206) + x0, y1 = load<u16>(fb + 208) + y0;
    if (r != 0) {
        const s32 rx = load<u16>(r), rx1 = rx + load<u16>(r + 4);
        if (x0 < rx) x0 = rx; else if (rx1 < x0) x0 = rx1;
        if (x1 < rx) x1 = rx; else if (rx1 < x1) x1 = rx1;
        const s32 ry = load<u16>(r + 2), ry1 = ry + load<u16>(r + 6);
        if (y0 < ry) y0 = ry; else if (ry1 < y0) y0 = ry1;
        if (y1 < ry) y1 = ry; else if (ry1 < y1) y1 = ry1;
    }
    const u32 w = static_cast<u32>(x1 - x0), h = static_cast<u32>(y1 - y0);
    u32 stale;  // r9: the value the failing compare loaded
    if ((static_cast<u32>(x0) & 0xFFFF) != load<u16>(fb + 164)) stale = static_cast<u32>(x0) & 0xFFFF;
    else if ((stale = load<u16>(fb + 166)) != (static_cast<u32>(y0) & 0xFFFF)) {}
    else if ((stale = load<u16>(fb + 168)) != (w & 0xFFFF)) {}
    else if ((stale = load<u16>(fb + 170)) != (h & 0xFFFF)) {}
    else return ret(ctx, fb);
    store<u16>(fb + 164, static_cast<u16>(x0));
    store<u16>(fb + 166, static_cast<u16>(y0));
    store<u16>(fb + 168, static_cast<u16>(w));
    store<u16>(fb + 170, static_cast<u16>(h));
    const u32 start = load<u32>(fb), end = 4;
    u32 p = load<u32>(fb + 16);
    if (p + 15 > load<u32>(fb + end)) p = start;  // cmplw
    p = (p + 3) & ~3u;
    store<u32>(fb + 16, p);
    store<u32>(p + 8, 0);
    store<u32>(p, 0x09000000u | (stale & 0x00FF0000u));
    store<u32>(p + 4, stale & 0xFFFFu);
    p = load<u32>(fb + 16);
    u32 q = p + 23 > load<u32>(fb + end) ? load<u32>(fb) : p + 12;
    q = (q + 3) & ~3u;
    store<u32>(fb + 16, q);
    store<u16>(q, static_cast<u16>(x0));
    store<u16>(q + 2, static_cast<u16>(y0));
    store<u16>(q + 4, static_cast<u16>(w));
    store<u16>(q + 6, static_cast<u16>(h));
    store<u32>(fb + 16, load<u32>(fb + 16) + 8);
    ret(ctx, fb);
}

// void (Sprite* a, const f32 m[16])   (name?)
// Binds the material (a+4 +72; its shader +16 +12 +0) and, by the
// shader's u8 +44 mode: 1..3 / 5 the model-view of m
// (VpConst_SetModelView_005012A4); 4 billboarded: the rotation 0x010DD940
// (Matrix_MulRot of the identity by it) with m's translation (the
// identity's when a+24 is set); others the identity (f_004FCC34). Then
// Stencil_SetEqual32_005011A0(a).
DECOMP(00501820, Sprite_Bind_00501820) {
    namespace g11 = sly4::render::gfx11;
    const u32 a = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 mat = load<u32>(load<u32>(a + 4) + 72);
    call(ctx, g11::fn::Material_Bind, {mat, load<u32>(load<u32>(load<u32>(mat + 16) + 12))});
    const s32 mode = load<u8>(load<u32>(a + 4) + 44);
    constexpr u32 kId = g11::g::identity, kRot = g11::g::billboard_rot;
    if (mode <= 0 || mode > 5) {
        call(ctx, g11::fn::VpConst_Set_004FCC34, {kId});
    } else if (mode != 4) {
        call(ctx, g11::fn::VpConst_SetModelView_005012A4, {a, m});
    } else {
        Stack frame(ctx, 128);
        const u32 sp = frame.addr() - 112;  // the original's offsets 112..239
        const u32 out = sp + 112;
        u32 src;  // the translation row copied into out's row 3
        if (load<u32>(a + 24) != 0) {
            call(ctx, g11::fn::Matrix_MulRot, {out, kId, kRot});
            src = kId + 48;
        } else {
            const u32 t = sp + 176;  // the identity's rotation rows (xyz) and m's translation (+48)
            for (u32 r = 0; r < 3; ++r)
                for (u32 k = 0; k < 12; k += 4) store<u32>(t + 16 * r + k, load<u32>(kId + 16 * r + k));  // lfs / stfs: raw
            for (u32 k = 0; k < 12; k += 4) store<u32>(t + 48 + k, load<u32>(m + 48 + k));
            call(ctx, g11::fn::Matrix_MulRot, {out, t, kRot});
            src = t + 48;
        }
        for (u32 k = 0; k < 12; k += 4) store<u32>(out + 48 + k, load<u32>(src + k));
        ctx.fpr[1] = render::gfx10::lfs_value(load<u32>(src + 8));
        call(ctx, g11::fn::VpConst_SetModelView_005012A4, {a, out});
    }
    ret(ctx, call(ctx, g11::fn::Stencil_SetEqual32_005011A0, {a}));
}
