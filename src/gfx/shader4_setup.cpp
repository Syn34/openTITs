// SlyCharacter2_SetupDraw: the lit characters' transforms, fog, lights
// and layers (SlyCharacter2_Draw and SlyCharacterFakeFur_Draw call it).
#include "shader4.h"
#include "shader_draw.h"

namespace sly4::gfx {

namespace {

using namespace sdraw;
namespace s4 = sly4::render::shader4;

constexpr u32 kText = 0x005BC8C0;        // .text: f32 0.0; +16 vperm mask; +32 f32[4] {0, 0, 0, 1}; +48 f32 1/255
constexpr u32 kItemVariantLit = 1, kItemVariantFragLayers = 2;
constexpr u32 kLights = 0x010DD9F0;      // layers3::g::block_dd780 +624: 6 f32[4] light vectors (name?)
constexpr u32 kEye = 0x010DD8B0;         // layers3::g::block_dd780 +304: the camera position

// The character parameters (material +124) this reads.
struct CharParams {
    pad<28> _0;
    be_u32 ambient_w;      // +28: f32
    be_u32 ambient;        // +32: packed RGB
    be_f32 ambient_scale;  // +36
    pad<4> _40;
    be_u32 tint;           // +44: packed A8R8G8B8
};
static_assert(offsetof(CharParams, ambient_w) == 28 && offsetof(CharParams, tint) == 44);

// The byte at `shift` of a packed colour, through fcfid / frsp, times k.
f32 channel(u32 packed, u32 shift, f32 k) { return byte_scaled((packed >> shift) & 0xFF, k); }

}  // namespace

// void SlyCharacter2_SetupDraw(Shader* s, DrawItem* item)
// The item transform and the view fog; then, for a lit item (variant +69
// bit 0):
// - vertex constants 6..9: the item matrix's first three columns, then
//   {0, 0, 0, 1};
// - vertex constants 10..15: the 6 light vectors times the material's
//   tint (parameters +44, packed A8R8G8B8, / 255 as {R, G, B, A}; 0
//   without parameters);
// - the layers: as vertex constants 16.. (upload_layer_list, but without
//   the layered check), or as fragment constants 16.. when variant bit 1
//   is set;
// - fragment constant 3: the ambient colour (parameters +32, packed RGB,
//   / 255, times parameters +36) with w = parameters +28 (0 without
//   parameters);
// - fragment constant 1: the camera position in the item's frame
//   (Listener_LocalDirection), w = 1.
void SlyCharacter2_SetupDraw(ShaderPrograms*, DrawItem* item) {
    const f32 zero = global<be_f32>(kText), k255 = global<be_f32>(kText + 48);
    upload_item_matrix(item);
    apply_fog(item);
    if ((item->variant & kItemVariantLit) == 0) return;

    GuestFrame frame(64);
    be_u32* cols = frame.as<be_u32>();
    for (u32 r = 0; r < 3; ++r) {
        for (u32 c = 0; c < 4; ++c) cols[4 * r + c] = lvlx_word0(at<u8>(item, 16 * c + 4 * r));  // lvlx, vmrghw, vperm
    }
    raw4(cols + 12, host<u8>(kText + 32));
    const CharParams* params = host<CharParams>(mat_params(item_mat(item)));
    const u32 tint = params ? params->tint.get() : 0;
    const f32 r = channel(tint, 16, k255), g = channel(tint, 8, k255), b = channel(tint, 0, k255), a = channel(tint, 24, k255);
    vconsts(6, 16, cols);
    {
        GuestFrame lv(16);
        be_f32* v = lv.as<be_f32>();
        for (u32 n = 0; n < 6; ++n) {
            const be_f32* src = host<be_f32>(kLights + 16 * n);
            v[0] = src[0] * r;  // fmuls
            v[1] = src[1] * g;
            v[2] = src[2] * b;
            v[3] = src[3] * a;
            vconsts(10 + n, 4, v);
        }
    }
    if ((item->variant & kItemVariantFragLayers) == 0) {
        upload_layer_list(item_mat(item), item->layers);
    } else {
        upload_layers_fragment(item);
    }

    params = host<CharParams>(mat_params(item_mat(item)));
    const void* w = params ? static_cast<const void*>(&params->ambient_w) : host<u8>(kText);
    const f32 scale = params ? static_cast<f32>(params->ambient_scale) : zero;
    const u32 ambient = params ? params->ambient.get() : 0;
    GuestFrame fc(48);  // the original's sp+112 (constant 3), +128 (direction), +144 (constant 1)
    be_f32* c3 = fc.as<be_f32>(0);
    be_f32* dir = fc.as<be_f32>(16);
    be_f32* c1 = fc.as<be_f32>(32);
    copy_word(c3 + 3, w);
    c3[0] = channel(ambient, 16, k255) * scale;  // fmuls, fmuls
    c3[1] = channel(ambient, 8, k255) * scale;
    c3[2] = channel(ambient, 0, k255) * scale;
    fconst(3, c3);
    call(s4::fn::Listener_LocalDirection, dir, kEye, item);
    vec3_w1(c1, dir);
    fconst(1, c1);
}

}  // namespace sly4::gfx

NATIVE(005BC8F8, sly4::gfx::SlyCharacter2_SetupDraw)
