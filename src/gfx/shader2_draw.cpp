// The shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(Shader* s, DrawItem* item)
//
// The simple ones upload the item's transform (its 4 x 4 rows times
// layers4::g::view_proj, transposed, as vertex constants 0..3) and hand
// the item to its mesh (Mesh vtable slot 16 draws (mesh, item)).
#include "shader2.h"

#include <cmath>

using namespace sly4;

namespace {

namespace d3 = sly4::render::draw3;
namespace l4 = sly4::render::layers4;
namespace s2 = sly4::render::shader2;

constexpr u32 kMeshDraw = 16;  // Mesh vtable slot 16: Draw(mesh, item)

f64 as_f64(u32 word) { return static_cast<f64>(std::bit_cast<f32>(word)); }

// transpose(item rows * view_proj) as vertex constants 0..3 (one
// SET_TRANSFORM_CONSTANT_LOAD of 17 words): rows read with lvx (address
// rounded down to 16), fused per lane in vmaddfp order, row 3 adding
// view_proj's row 3. (draw3.cpp's Slot_DrawProxy inlines the same.)
void upload_item_transform(u32 item) {
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
        for (u32 i = 0; i < 4; ++i) store<f32>(cur + 8 + 16 * k + 4 * i, r[i][k]);  // stvlx / stvrx pairs
    }
}

// The mesh's draw (item +12, slot 16); r3 is left as it returns.
void draw_mesh(PPUContext& ctx, u32 item) {
    const u32 mesh = load<u32>(item + d3::kItemMesh);
    ctx.gpr[3] = fx::vcall(ctx, mesh, kMeshDraw, {mesh, item});
}

}  // namespace

// The item transform, then the mesh's draw.
DECOMP(005ADD54, PrimTxr_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    draw_mesh(ctx, item);
}
DECOMP(005B1830, ProjTxr_Deferred_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    draw_mesh(ctx, item);
}
DECOMP(005B1AE8, ZPass_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    draw_mesh(ctx, item);
}
DECOMP(005B095C, SprParticle_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    draw_mesh(ctx, item);
}

// SimpleColorTxr: the item transform, the item's material +64 (a vec4
// colour) as vertex constant 4, then the mesh's draw.
DECOMP(005ADEE4, SimpleColorTxr_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants,
         {load<u32>(fx::g::state), 4, 4, load<u32>(item + d3::kItemMaterial) + 64});
    draw_mesh(ctx, item);
}

// GeoParticle: nothing without a colour (item +64, u8[4]); else the item
// transform, the colour * k (1/255) as vertex constant 4, then the mesh's
// draw.
DECOMP(005B0440, GeoParticle_Draw) {
    constexpr u32 k = 0x005B043C;  // f32 in .text
    const u32 item = arg<u32>(ctx, 1);
    const u32 colour = load<u32>(item + 64);
    if (colour == 0) {
        ctx.gpr[3] = 0;  // left in r3
        return;
    }
    upload_item_transform(item);
    const f64 scale = static_cast<f64>(load<f32>(k));
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    for (u32 i = 0; i < 4; ++i) {
        store<f32>(v + 4 * i, static_cast<f32>(static_cast<f64>(ref<u8>(load<u32>(item + 64) + i)) * scale));  // fcfid, frsp, fmuls
    }
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 4, 4, v});
    draw_mesh(ctx, item);
}

// Debug: the item transform; vertex constant 4 = the item's colour (item
// +64, u8[4] / 255; {1, 1, 0, 0.5} without one), with alpha blending on
// (SRC_ALPHA, ONE_MINUS_SRC_ALPHA, FUNC_ADD) when its alpha is below 1;
// then the mesh's draw.
DECOMP(005ADA18, Debug_Draw) {
    constexpr u32 k = 0x005ADA0C;  // f32 pair in .text (1.0, 1/255)
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    store<u32>(v, s2::kOne);
    store<u32>(v + 4, s2::kOne);
    store<u32>(v + 8, 0);
    store<u32>(v + 12, 0x3F000000);  // 0.5
    if (const u32 colour = load<u32>(item + 64); colour != 0) {
        const f64 scale = static_cast<f64>(load<f32>(k + 4));
        for (u32 i = 0; i < 4; ++i) store<f32>(v + 4 * i, static_cast<f32>(static_cast<f64>(ref<u8>(colour + i)) * scale));  // fcfid, frsp, fmuls
        if (load<f32>(v + 12) < load<f32>(k)) fx::blend_on(fx::kSrcAlpha2, fx::kOneMinusSrcAlpha2);  // fcmpu / blt
    }
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 4, 4, v});
    draw_mesh(ctx, item);
}

// Outline: nothing for an item farther (squared, from the camera at
// layers3::g::block_dd780 +304) than its material's parameters +0 (0.0
// without parameters); else the item transform, vertex constant 4 = {0,
// 0, 0, width} (the colour record's f32 +4; 1.0 without one), fragment
// constant 0 = {the colour (item +64, u8[4] / 255; 0 without), item +71 /
// 255 as alpha}, then the mesh's draw.
DECOMP(005AFBF8, Outline_Draw) {
    namespace sp = render::pool;
    constexpr u32 k = 0x005AFBEC;  // f32[3] in .text (0.0, 1/255, 255.0)
    const u32 cam = render::layers3::g::block_dd780;
    const u32 item = arg<u32>(ctx, 1);
    const u32 k0 = load<u32>(k);
    const f64 limit = as_f64(s2::param_or(load<u32>(load<u32>(item + d3::kItemMaterial) + d3::kMatParams), 0, k0));
    const f64 dy = sp::sgl(sp::lfs(cam + 308) - sp::lfs(item + 52));  // fsubs
    const f64 dx = sp::sgl(sp::lfs(cam + 304) - sp::lfs(item + 48));
    const f64 dz = sp::sgl(sp::lfs(cam + 312) - sp::lfs(item + 56));
    const f64 d2 = sp::fmadds(dz, dz, sp::fmadds(dx, dx, sp::sgl(dy * dy)));
    if (limit < d2) {  // fcmpu / blt
        ctx.gpr[3] = cam;  // left in r3
        return;
    }
    upload_item_transform(item);
    // The original's sp+112 (fragment constant 0) and sp+128 (vertex constant 4).
    Stack frame(ctx, 32);
    const u32 f = frame.addr(), v = f + 16;
    for (u32 i = 0; i < 12; i += 4) {
        store<u32>(f + i, k0);
        store<u32>(v + i, k0);
    }
    store<u32>(v + 12, s2::kOne);
    if (const u32 colour = load<u32>(item + 64); colour != 0) {
        const f64 scale = sp::lfs(k + 4);
        for (u32 i = 0; i < 4; ++i) store<f32>(f + 4 * i, static_cast<f32>(static_cast<f64>(ref<u8>(colour + i)) * scale));  // fcfid, frsp, fmuls
        store<u32>(v + 12, load<u32>(colour + 4));
    }
    sp::stfs(f + 12, sp::sgl(static_cast<f64>(ref<u8>(item + 71)) / sp::lfs(k + 8)));  // fdivs
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 4, 4, v});
    call(ctx, d3::fn::FProgram_SetConstant, {0, f});
    draw_mesh(ctx, item);
}

namespace {

// Decal_Draw's and ScreenWarp_Draw's body: the item transform,
// DrawItem_SetParticleState(item, 32); for a layered item (material +100
// bit 7 and a layer list at item +44) the layer list as vertex constants
// 16..: {+16 * colour, +32 * colour, +48} and per layer entry (+0 u32
// count, 48-byte entries from +64) {+0, +16 * colour, +32}, with colour
// the material's f32[4] at +64; then the mesh's draw.
void draw_layered(PPUContext& ctx, u32 item) {
    namespace sp = render::pool;
    upload_item_transform(item);
    call(ctx, s2::fn::DrawItem_SetParticleState, {item, 32});
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    if (((ref<u8>(mat + d3::kMatFlags) >> 7) & 1) != 0) {  // rlwinm 25,31,31
        if (const u32 list = load<u32>(item + d3::kItemLayers); list != 0) {
            const u32 colour = mat + 64;
            const auto tinted = [&](u32 dst, u32 src) {
                for (u32 i = 0; i < 16; i += 4) sp::stfs(dst + i, sp::sgl(sp::lfs(src + i) * sp::lfs(colour + i)));  // fmuls
            };
            const auto raw = [](u32 dst, u32 src) {
                for (u32 i = 0; i < 16; i += 4) sp::copy_word(dst + i, src + i);
            };
            // The original builds this at sp+112 of a 320-byte frame
            // without a bound: more than 3 entries overrun its saved
            // registers. Here the frame is sized for the entry count.
            Stack frame(ctx, 48 + 48 * load<u32>(list));
            const u32 v = frame.addr();
            tinted(v, list + 16);
            tinted(v + 16, list + 32);
            raw(v + 32, list + 48);
            u32 dst = v + 48;
            if (load<u32>(list) != 0) {
                u32 src = list + 64;
                for (u32 n = 0; n < load<u32>(list); ++n, src += 48, dst += 48) {  // cmplw
                    raw(dst, src);
                    tinted(dst + 16, src + 16);
                    raw(dst + 32, src + 32);
                }
            }
            const u32 floats = static_cast<u32>(static_cast<s32>(dst - v) / 16) << 2;  // srawi / addze, rlwinm 2,0,29
            call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 16, floats, v});
        }
    }
    draw_mesh(ctx, item);
}

}  // namespace

DECOMP(005B3148, Decal_Draw) { draw_layered(ctx, arg<u32>(ctx, 1)); }

// ScreenWarp: nothing when g::config +152 bit 2 is set (as its Bind);
// else Decal_Draw's body.
DECOMP(005AF730, ScreenWarp_Draw) {
    if (((ref<u8>(sly4::g::config + 152) >> 2) & 1) != 0) {  // rlwinm 30,31,31
        ctx.gpr[3] = 1;  // left in r3
        return;
    }
    draw_layered(ctx, arg<u32>(ctx, 1));
}
