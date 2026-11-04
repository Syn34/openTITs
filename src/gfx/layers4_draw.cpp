// The derived render layers' item passes: ZTranslucent's depth pre-pass
// (ZLayer_DrawBody) and WaterResolve / ScreenWarp's two passes, plus the
// scene-copy helpers the second resolve pass uses.
#include "layers4.h"

#include <cmath>

#include "shadergroup.h"

using namespace sly4;

namespace {

namespace rl = sly4::render::layer;
namespace l2 = sly4::render::layer2;
namespace l4 = sly4::render::layers4;
namespace shg = sly4::shgroup;

using fx::emit;

constexpr u32 kStats = sly4::g::mem_stats, kConfig = sly4::g::config;
constexpr u32 kFormatTable = 0x00F67254;  // 24-byte entries, +8 bits per pixel (tgt::g::format_table)

// The Shader for (sg, item): the group's picker (+16) slot 16, as
// DrawItem_Draw picks it (no view override here).
u32 pick_shader(PPUContext& ctx, u32 sg, u32 item) {
    const u32 picker = load<u32>(sg + 16);
    return static_cast<u32>(fx::vcall(ctx, picker, l2::kPickShader, {picker, sg, item}));
}

// Uploads transpose(item rows * view_proj) as vertex constants 0..3 (one
// SET_TRANSFORM_CONSTANT_LOAD of 17 words). Row i of the product is
// item[i].x * M0 + item[i].y * M1 + item[i].z * M2 (+ M3 for row 3),
// fused per lane in vmaddfp order. The rows are read with lvx (address
// rounded down to 16).
void upload_matrix(u32 item) {
    const u32 m = l4::g::view_proj;
    f32 r[4][4];
    for (u32 i = 0; i < 4; ++i) {
        const u32 row = (item & ~15u) + 16 * i;
        const f32 x = load<f32>(row), y = load<f32>(row + 4), z = load<f32>(row + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(m + 4 * k), x, i == 3 ? load<f32>(m + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(m + 16 + 4 * k), y, v);
            r[i][k] = std::fmaf(load<f32>(m + 32 + 4 * k), z, v);
        }
    }
    const u32 c = load<u32>(fx::g::state);
    const u32 cur = load<u32>(c + 8);
    store<u32>(c + 8, cur + 72);
    store<u32>(cur, l4::kVertexConst17);
    store<u32>(cur + 4, 0);  // first constant
    for (u32 k = 0; k < 4; ++k) {
        for (u32 i = 0; i < 4; ++i) store<f32>(cur + 8 + 16 * k + 4 * i, r[i][k]);
    }
}

// Binds the shader's vertex (+4) and fragment (+8) programs as each draw
// inlines it.
void bind_programs(PPUContext& ctx, u32 shader) {
    fx::bind_vertex(ctx, shader + 4);
    fx::bind_fragment(shader + 8);
}

}  // namespace

// void ZLayer_DrawBody(RenderLayer* l)   (ZTranslucent's item pass; name?)
// For each run of entries with the same key: a depth-only pre-pass
// (colour writes off, depth test as the render state has it, LEQUAL,
// depth write iff the state's depth bit, cull on, CCW front face, alpha
// test and blending off) drawing every item with shader group 2's shader
// for it, its matrix in vertex constants 0..3 and DrawItem_DrawDepth;
// then colour writes back on, the render state cache invalidated and the
// run drawn normally (DrawItem_Draw), the bound shader ended. Returns 0
// (the null item that ends the list).
DECOMP(00513B78, ZLayer_DrawBody) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 st = fx::g::state;
    u32 group = load<u32>(l + rl::kBase);
    if (load<u32>(group + 4) != 0) {
        for (;;) {
            const u32 key = load<u32>(group);
            u32 end = group;
            do end += 8;
            while (load<u32>(end + 4) != 0 && load<u32>(end) == key);

            emit({l4::kColorMask, 0});
            emit({l4::kDepthTest, ref<u8>(st + 93)});
            emit({l4::kDepthFunc, l4::kLequal});
            emit({l4::kDepthMask, (ref<u8>(st + 93) & 1) != 0 ? 1u : 0u});
            emit({l4::kCullEnable, 1});
            emit({l4::kFrontFace, l4::kCcw});
            emit({l4::kAlphaTest, 0});
            emit({l4::kBlendEnable, 0});
            const u32 sg = static_cast<u32>(call(ctx, shg::fn::Shader_LoadGroup, {2}));
            for (u32 e = group; e < end; e += 8) {
                const u32 item = load<u32>(e + 4);
                const u32 mat = load<u32>(item + 12);
                const u32 shader = pick_shader(ctx, sg, item);
                bind_programs(ctx, shader);
                upload_matrix(item);
                call(ctx, l4::fn::DrawItem_DrawDepth, {mat, item});
            }
            emit({l4::kColorMask, 0x01010101});
            call(ctx, render::fn::Draw_InvalidateState);
            for (u32 e = group; e < end; e += 8) call(ctx, rl::DrawItem_Draw, {load<u32>(e + 4)});

            const u32 b = l2::g::binding;
            if (const u32 shader = load<u32>(b + 4); shader != 0) {
                fx::vcall(ctx, shader, l2::kShaderEnd, {shader, 0});
                store<u32>(b + 4, 0);
                store<u32>(b + 8, 0);
                store<u32>(b, 0);
            }
            group = end;
            if (load<u32>(group + 4) == 0) break;
        }
    }
    ctx.gpr[3] = 0;
}

// void ResolveLayer_Pass1(RenderLayer* l)   (name?)
// Clears alpha (Draw_Clear(CLEAR_A, 0, 1.0, 0, 255)), then with only
// alpha writes on, blending and depth writes off and the layer's four
// floats (+176) in vertex constant 4, draws every item with shader group
// 0's shader for it, its matrix in vertex constants 0..3 and the item's
// material (+12) slot 16. Colour writes back on, state invalidated;
// returns what Draw_InvalidateState returns.
DECOMP(00514200, ResolveLayer_Pass1) {
    const u32 l = arg<u32>(ctx, 0);
    constexpr u32 kClearA = 0x80;
    // r5 holds the constant's address in the original (Draw_Clear doesn't read it).
    callf(ctx, l4::fn::Draw_Clear, {kClearA, 0, l4::kPass1ClearDepth, 0, 255},
          {static_cast<f64>(load<f32>(l4::kPass1ClearDepth))});
    emit({l4::kColorMask, 0x01000000});
    emit({l4::kBlendEnable, 0});
    emit({l4::kDepthMask, 0});
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 4, 4, l + 176});
    const u32 sg = static_cast<u32>(call(ctx, shg::fn::Shader_LoadGroup, {0}));
    for (u32 e = load<u32>(l + rl::kBase); load<u32>(e + 4) != 0; e += 8) {
        const u32 item = load<u32>(e + 4);
        const u32 mat = load<u32>(item + 12);
        const u32 shader = pick_shader(ctx, sg, item);
        bind_programs(ctx, shader);
        upload_matrix(item);
        fx::vcall(ctx, mat, 16, {mat, item});
    }
    emit({l4::kColorMask, 0x01010101});
    ctx.gpr[3] = call(ctx, render::fn::Draw_InvalidateState);
}

// void ResolveLayer_Pass2(RenderLayer* l)   (name?)
// Counts one more layer (g::config +76), gathers up to 96 non-empty screen
// rects of the items (ResolveLayer_ItemRect; a 97th non-empty rect stops
// the gathering) and their total area. When the area is
// more than 0.9 of the screen (g::config +4 * +6), the whole colour target
// is copied (Draw_CopyScreen, once per layer count); otherwise each rect
// is copied from the scene's render target into the resolve texture
// (Draw_CopyRect). Then the items are drawn normally and the layer ended;
// returns what RenderLayer_End returns.
DECOMP(00514838, ResolveLayer_Pass2) {
    const u32 l = arg<u32>(ctx, 0);
    Stack frame(ctx, 1760 - 112);  // the original's frame
    const u32 sp = frame.addr() - 112;
    const u32 out = sp + 112, rects = sp + 128, temp = sp + 1664;  // f32[4] out, {s32 x0, y0, x1, y1}[96], f64 temp

    const u32 first = load<u32>(l + rl::kBase);
    store<u32>(kConfig + 76, load<u32>(kConfig + 76) + 1);
    for (u32 i = 0; i < 16 * l4::kMaxRects; i += 4) store<u32>(rects + i, 0);
    const u32 dst = static_cast<u32>(call(ctx, l4::fn::Stats_ResolveTexture, {kStats}));
    const u32 src = static_cast<u32>(call(ctx, l4::fn::Display_SelectTarget, {kStats}));
    f32 area = load<f32>(l4::kPass2Consts);
    u32 n = 0;
    if (load<u32>(first + 4) != 0) {
        for (u32 e = first; load<u32>(e + 4) != 0; e += 8) {
            call(ctx, l4::fn::ResolveLayer_ItemRect, {l, load<u32>(e + 4), out, out + 4, out + 8, out + 12});
            const f32 x0 = load<f32>(out), y0 = load<f32>(out + 4), x1 = load<f32>(out + 8), y1 = load<f32>(out + 12);
            if (!(x1 > x0) || !(y1 > y0)) continue;
            if (n == l4::kMaxRects) break;
            const u32 r = rects + 16 * n;
            const f32 w = static_cast<f32>(static_cast<f64>(x1) - static_cast<f64>(x0));
            const f32 h = static_cast<f32>(static_cast<f64>(y1) - static_cast<f64>(y0));
            ++n;
            store<u32>(r, static_cast<u32>(rt::fctid(x0, true)));
            area = static_cast<f32>(std::fma(static_cast<f64>(w), static_cast<f64>(h), static_cast<f64>(area)));
            store<u32>(r + 4, static_cast<u32>(rt::fctid(y0, true)));
            store<u32>(r + 8, static_cast<u32>(rt::fctid(x1, true)));
            store<u32>(r + 12, static_cast<u32>(rt::fctid(y1, true)));
        }
    }

    const u32 pixels = static_cast<u32>(load<u16>(kConfig + 4)) * load<u16>(kConfig + 6);  // mullw
    const s64 pixels64 = static_cast<s32>(pixels);                                         // extsw
    store<u64>(temp, static_cast<u64>(pixels64));  // the fcfid temp (the rect loop's temps end overwritten by it)
    const f64 limit = rt::fcfid(static_cast<u64>(pixels64)) * load<f64>(l4::kPass2Consts + 8);
    if (static_cast<f64>(area) > limit) {
        const u32 stamp = load<u32>(kConfig + 76);
        if (load<u32>(l4::g::copy_stamp) != stamp) {
            store<u32>(l4::g::copy_stamp, stamp);
            // r3 still holds the high half of copy_stamp's address (Draw_CopyScreen takes no arguments).
            store<u32>(l4::g::copy_texture, static_cast<u32>(call(ctx, l4::fn::Draw_CopyScreen, {l4::g::copy_stamp & 0xFFFF0000u})));
        }
    } else {
        for (u32 i = 0; i < n; ++i) {
            const u32 r = rects + 16 * i;
            const u32 x0 = load<u32>(r), y0 = load<u32>(r + 4);
            call(ctx, l4::fn::Draw_CopyRect,
                 {dst, src, x0, y0, x0, y0, load<u32>(r + 8) - x0, load<u32>(r + 12) - y0});
        }
    }

    for (u32 e = load<u32>(l + rl::kBase); load<u32>(e + 4) != 0; e += 8) call(ctx, rl::DrawItem_Draw, {load<u32>(e + 4)});
    ctx.gpr[3] = call(ctx, rl::RenderLayer_End);
}

// Texture* Stats_ResolveTexture(MemStats* s)   (name?)
// s +100 when g::config +34 is set (per-eye buffers), else s +84.
DECOMP(00536D28, Stats_ResolveTexture) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 per_eye = ref<u8>(kConfig + 34);
    ctx.gpr[4] = per_eye;  // left in r4
    ret(ctx, load<u32>(s + (per_eye != 0 ? 100 : 84)));
}

// Target* Display_SelectTarget(Texture* const* table)   (name?)
// The +40 render target of the table entry Display_SelectBuffer picks
// (+128 for the alternate buffer set, else +92 / +112 / +120 per eye).
DECOMP(00537CB8, Display_SelectTarget) {
    const u32 table = arg<u32>(ctx, 0);
    const u32 disp = load<u32>(draw::g::display);
    u32 slot;
    if ((ref<u8>(disp + 148) >> 7) != 0) {
        slot = table + 128;
    } else {
        const u32 k = (load<u32>(disp + 52) >> 28) & 0xC;  // rlwinm 4,28,29: bits 30..31 * 4
        if (ref<u8>(kConfig + 34) != 0) {
            slot = table + k + (load<u32>(kStats + 184) == 1 ? 112 : 120);
        } else {
            slot = table + k + 92;
        }
    }
    ret(ctx, load<u32>(load<u32>(slot) + 40));
}

// void Draw_CopyRect(Texture* dst, Target* src, u32 sx, u32 sy, u32 dx, u32 dy, u32 w, u32 h)   (name?)
// cellGcmSetTransferImage (local to local) of a w x h rect from src (+4
// offset << 1 | location, +32 pitch, +42 first visible row) at (sx, sy)
// to dst (+32 pitch, +36 offset, +44 format) at (dx, dy). Returns what
// the transfer returns.
DECOMP(00536C3C, Draw_CopyRect) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u64 sx = ctx.gpr[5], sy = ctx.gpr[6], dx = ctx.gpr[7], dy = ctx.gpr[8], w = ctx.gpr[9], h = ctx.gpr[10];
    const s32 bpp = static_cast<s32>(load<u32>(kFormatTable + 24 * load<u32>(dst + 44) + 8));
    const u32 bytes = static_cast<u32>(bpp / 8) & 0xFF;  // srawi + addze
    const u32 pitch = load<u32>(src + 32);
    const u32 offset = (load<u32>(src + 4) >> 1) + pitch * load<u16>(src + 42);
    ctx.gpr[3] = call(ctx, l4::fn::cellGcmSetTransferImage,
                      {load<u32>(fx::g::state), 0, load<u32>(dst + 36), load<u32>(dst + 32), dx, dy, offset, pitch,
                       sx, sy, w, h, bytes});
}

// Texture* Draw_CopyScreen()   (name?)
// Copies the whole colour target into Stats_ResolveTexture's texture (as
// fx::copy_colour_to does) and returns that texture.
DECOMP(00536D4C, Draw_CopyScreen) {
    const u32 tex = load<u32>(kStats + (ref<u8>(kConfig + 34) != 0 ? 100 : 84));
    fx::copy_colour_to(ctx, tex);
    ctx.gpr[3] = tex;
}

// void ResolveLayer_ItemRect(RenderLayer* l, DrawItem* item, f32* x0, f32* y0, f32* x1, f32* y1)   (name?)
// The item's screen rect in pixels: the material's bounding sphere
// (+92: centre, radius) through the item matrix (vmaddfp per lane, + row
// 3), the radius scaled by the largest of the three scales
// DrawItem_GetScale writes; a zero radius gives an empty rect (all 0).
// Otherwise View_ProjectSphere(sphere, view_matrix, view +1008, outs)
// and the result scaled by the screen size (g::config +4 / +6). Returns
// the r3 the original leaves (0, or the screen height).
DECOMP(005145C0, ResolveLayer_ItemRect) {
    namespace l3 = sly4::render::layers3;
    const u32 item = arg<u32>(ctx, 1);
    const u32 px0 = arg<u32>(ctx, 2), py0 = arg<u32>(ctx, 3), px1 = arg<u32>(ctx, 4), py1 = arg<u32>(ctx, 5);
    Stack frame(ctx, 272 - 112);  // the original's frame
    const u32 sp = frame.addr() - 112;
    const u32 centre = sp + 112, scales = sp + 128, sphere = sp + 144, temp = sp + 160;

    const u32 bound = load<u32>(item + 12) + 92;
    const f32 bx = load<f32>(bound), by = load<f32>(bound + 4), bz = load<f32>(bound + 8), br = load<f32>(bound + 12);
    const u32 rows = item & ~15u;  // lvx
    f32 c[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(load<f32>(rows + 4 * k), bx, 0.0f);
        v = std::fmaf(load<f32>(rows + 16 + 4 * k), by, v);
        v = std::fmaf(load<f32>(rows + 32 + 4 * k), bz, v);
        c[k] = load<f32>(rows + 48 + 4 * k) + v;  // vaddfp
    }
    call(ctx, l4::fn::DrawItem_GetScale, {item, scales});
    const f32 sx = load<f32>(scales), sy = load<f32>(scales + 4), sz = load<f32>(scales + 8);
    f32 m = static_cast<f32>(static_cast<f64>(sx) - static_cast<f64>(sy)) >= 0.0f ? sx : sy;  // fsubs + fsel
    m = static_cast<f32>(static_cast<f64>(m) - static_cast<f64>(sz)) >= 0.0f ? m : sz;
    const f32 radius = static_cast<f32>(static_cast<f64>(m) * static_cast<f64>(br));
    if (radius == load<f32>(l4::kItemRectZero)) {
        for (const u32 p : {px0, py0, px1, py1}) store<u32>(p, 0);
        ctx.gpr[3] = 0;
        return;
    }
    for (u32 k = 0; k < 4; ++k) store<f32>(centre + 4 * k, c[k]);  // stvx (over the local centre's stfs)
    store<f32>(sphere + 12, radius);
    store<f32>(sphere + 4, c[1]);
    store<f32>(sphere, c[0]);
    store<f32>(sphere + 8, c[2]);
    const u32 view = load<u32>(load<u32>(draw::g::current_surface) + 672);
    call(ctx, l4::fn::View_ProjectSphere, {sphere, l3::g::view_matrix, view + 1008, px0, py0, px1, py1});
    const u32 w = load<u16>(kConfig + 4), h = load<u16>(kConfig + 6);
    const auto scale = [](u32 p, u32 n) {
        store<f32>(p, static_cast<f32>(static_cast<f64>(load<f32>(p)) * static_cast<f64>(static_cast<f32>(n))));
    };
    scale(px0, w);
    scale(py0, h);
    scale(px1, w);
    scale(py1, h);
    store<u64>(temp, h);  // the last fcfid temp
    ctx.gpr[3] = h;
}
