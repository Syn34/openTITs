// The shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(ShaderPrograms* s, DrawItem* item)
//
// The simple ones upload the item's transform (its 4 x 4 rows times
// layers4::g::view_proj, transposed, as vertex constants 0..3) and hand
// the item to its mesh (Mesh vtable slot 16 draws (mesh, item)).
#include "shader2.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace d3 = sly4::render::draw3;
namespace s2 = sly4::render::shader2;

constexpr u32 kMeshDraw = 16;  // Mesh vtable slot 16: Draw(mesh, item)
constexpr u32 kCamera = 0x010DD780 + 304;  // f32[3]: layers3::g::block_dd780 +304

// The mesh's draw (item +12, slot 16).
void draw_mesh(DrawItem* item) { vcall(or_zero_page(item->mesh.get()), kMeshDraw, item); }

// The item's colour (+64: a pointer to u8[4]; +4 of it an f32 width for Outline).
const u8* item_colour(const DrawItem* item) { return host<u8>(item->w64); }

}  // namespace

// The item transform, then the mesh's draw.
void PrimTxr_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    draw_mesh(item);
}
void ProjTxr_Deferred_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    draw_mesh(item);
}
void ZPass_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    draw_mesh(item);
}
void SprParticle_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    draw_mesh(item);
}

// SimpleColorTxr: the item transform, the item's material +64 (a vec4
// colour) as vertex constant 4, then the mesh's draw.
void SimpleColorTxr_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 4u, 4u, at<u8>(item->mat.get(), 64));
    draw_mesh(item);
}

// GeoParticle: nothing without a colour (item +64, u8[4]); else the item
// transform, the colour * k (1/255) as vertex constant 4, then the mesh's
// draw.
void GeoParticle_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005B043C;  // f32 in .text
    if (item->w64 == 0) return;
    upload_item_matrix(item);
    const f32 scale = global<be_f32>(k);
    GuestFrame f(16);
    be_f32* v = f.as<be_f32>();
    for (u32 i = 0; i < 4; ++i) v[i] = static_cast<f32>(*at<u8>(item_colour(item), i)) * scale;  // fcfid, frsp, fmuls
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 4u, 4u, v);
    draw_mesh(item);
}

// Debug: the item transform; vertex constant 4 = the item's colour (item
// +64, u8[4] / 255; {1, 1, 0, 0.5} without one), with alpha blending on
// (SRC_ALPHA, ONE_MINUS_SRC_ALPHA, FUNC_ADD) when its alpha is below 1;
// then the mesh's draw.
void Debug_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005ADA0C;  // f32 pair in .text (1.0, 1/255)
    upload_item_matrix(item);
    GuestFrame f(16);
    be_f32* v = f.as<be_f32>();
    be_u32* w = f.as<be_u32>();
    w[0] = s2::kOne;
    w[1] = s2::kOne;
    w[2] = 0u;
    w[3] = 0x3F000000u;  // 0.5
    if (const u8* colour = item_colour(item)) {
        const f32 scale = *host<be_f32>(k + 4);
        for (u32 i = 0; i < 4; ++i) v[i] = static_cast<f32>(*at<u8>(colour, i)) * scale;  // fcfid, frsp, fmuls
        if (v[3] < global<be_f32>(k)) blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);  // fcmpu / blt
    }
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 4u, 4u, v);
    draw_mesh(item);
}

// Outline: nothing for an item farther (squared, from the camera at
// layers3::g::block_dd780 +304) than its material's parameters +0 (0.0
// without parameters); else the item transform, vertex constant 4 = {0,
// 0, 0, width} (the colour record's f32 +4; 1.0 without one), fragment
// constant 0 = {the colour (item +64, u8[4] / 255; 0 without), item +71 /
// 255 as alpha}, then the mesh's draw.
void Outline_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005AFBEC;  // f32[3] in .text (0.0, 1/255, 255.0)
    const be_f32* kc = host<be_f32>(k);
    const be_f32* cam = host<be_f32>(kCamera);
    const u32 k0 = *reinterpret_cast<const be_u32*>(kc);
    const be_u32* params = host<be_u32>(or_zero_page(item->mat.get())->w124);
    const f32 limit = as_f32(params ? params[0].get() : k0);
    const f32 dy = cam[1] - item->pos[1];  // fsubs
    const f32 dx = cam[0] - item->pos[0];
    const f32 dz = cam[2] - item->pos[2];
    const f32 d2 = fmadds(dz, dz, fmadds(dx, dx, dy * dy));
    if (limit < d2) return;  // fcmpu / blt
    upload_item_matrix(item);
    // The original's sp+112 (fragment constant 0) and sp+128 (vertex constant 4).
    GuestFrame frame(32);
    be_u32* fw = frame.as<be_u32>(0);
    be_u32* vw = frame.as<be_u32>(16);
    be_f32* fc = frame.as<be_f32>(0);
    for (u32 i = 0; i < 3; ++i) {
        fw[i] = k0;
        vw[i] = k0;
    }
    vw[3] = s2::kOne;
    if (const u8* colour = item_colour(item)) {
        const f32 scale = kc[1];
        for (u32 i = 0; i < 4; ++i) fc[i] = static_cast<f32>(*at<u8>(colour, i)) * scale;  // fcfid, frsp, fmuls
        vw[3] = at<be_u32>(colour, 4)->get();
    }
    fc[3] = static_cast<f32>(*at<u8>(item, 71)) / kc[2];  // fdivs
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 4u, 4u, vw);
    FProgram_SetConstant(0, reinterpret_cast<const be_u32*>(fc));
    draw_mesh(item);
}

namespace {

// Decal_Draw's and ScreenWarp_Draw's body: the item transform,
// DrawItem_SetParticleState(item, 32); for a layered item (material +100
// bit 7 and a layer list at item +44) the layer list as vertex constants
// 16..: {+16 * colour, +32 * colour, +48} and per layer entry (+0 u32
// count, 48-byte entries from +64) {+0, +16 * colour, +32}, with colour
// the material's f32[4] at +64; then the mesh's draw.
void draw_layered(DrawItem* item) {
    upload_item_matrix(item);
    DrawItem_SetParticleState(item, 32);
    Material* mat = or_zero_page(item->mat.get());
    if (((mat->b100 >> 7) & 1) != 0) {  // rlwinm 25,31,31
        if (const be_u32* list = item->layers) {
            const be_f32* colour = at<be_f32>(mat, 64);
            const auto tinted = [&](be_f32* dst, const be_f32* src) {
                for (u32 i = 0; i < 4; ++i) dst[i] = src[i] * colour[i];  // fmuls
            };
            const auto raw = [](void* dst, const void* src) {
                for (u32 i = 0; i < 4; ++i) copy_word(static_cast<be_u32*>(dst) + i, static_cast<const be_u32*>(src) + i);
            };
            const auto field = [&](u32 off) { return at<be_f32>(list, off); };
            // The original builds this at sp+112 of a 320-byte frame
            // without a bound: more than 3 entries overrun its saved
            // registers. Here the frame is sized for the entry count.
            GuestFrame f(48 + 48 * list[0]);
            be_f32* v = f.as<be_f32>();
            tinted(v, field(16));
            tinted(v + 4, field(32));
            raw(v + 8, field(48));
            be_f32* dst = v + 12;
            if (list[0] != 0) {
                u32 src = 64;
                for (u32 n = 0; n < list[0]; ++n, src += 48, dst += 12) {  // cmplw
                    raw(dst, field(src));
                    tinted(dst + 4, field(src + 16));
                    raw(dst + 8, field(src + 32));
                }
            }
            const u32 floats = static_cast<u32>(static_cast<s32>(guest(dst) - guest(v)) / 16) << 2;  // srawi / addze, rlwinm 2,0,29
            call(fn::cellGcmSetVertexProgramConstants, gcm(), 16u, floats, v);
        }
    }
    draw_mesh(item);
}

}  // namespace

void Decal_Draw(ShaderPrograms*, DrawItem* item) { draw_layered(item); }

// ScreenWarp: nothing when g::config +152 bit 2 is set (as its Bind);
// else Decal_Draw's body.
void ScreenWarp_Draw(ShaderPrograms*, DrawItem* item) {
    if (((config_u8(152) >> 2) & 1) != 0) return;  // rlwinm 30,31,31
    draw_layered(item);
}

}  // namespace sly4::gfx

NATIVE(005ADD54, sly4::gfx::PrimTxr_Draw)
NATIVE(005B1830, sly4::gfx::ProjTxr_Deferred_Draw)
NATIVE(005B1AE8, sly4::gfx::ZPass_Draw)
NATIVE(005B095C, sly4::gfx::SprParticle_Draw)
NATIVE(005ADEE4, sly4::gfx::SimpleColorTxr_Draw)
NATIVE(005B0440, sly4::gfx::GeoParticle_Draw)
NATIVE(005ADA18, sly4::gfx::Debug_Draw)
NATIVE(005AFBF8, sly4::gfx::Outline_Draw)
NATIVE(005B3148, sly4::gfx::Decal_Draw)
NATIVE(005AF730, sly4::gfx::ScreenWarp_Draw)
