// SlyCharacter2_SetupDraw: the lit characters' transforms, fog, lights
// and layers (SlyCharacter2_Draw and SlyCharacterFakeFur_Draw call it).
#include "shader4_util.h"

using namespace sly4;
using namespace sly4::render::shader4::util;

namespace {

constexpr u32 kText = 0x005BC8C0;        // .text: f32 0.0; +16 vperm mask; +32 f32[4] {0, 0, 0, 1}; +48 f32 1/255
constexpr u32 kItemVariantLit = 1, kItemVariantFragLayers = 2;
constexpr u32 kLights = 0x010DD9F0;         // layers3::g::block_dd780 +624: 6 f32[4] light vectors (name?)
constexpr u32 kEye = 0x010DD8B0;            // layers3::g::block_dd780 +304: the camera position
constexpr u32 kParamsTint = 44, kParamsAmbientW = 28, kParamsAmbientScale = 36, kParamsAmbient = 32;

// The byte at `shift` of a packed colour, through fcfid / frsp, times k.
f64 channel(u32 packed, u32 shift, f64 k) { return byte_scaled((packed >> shift) & 0xFF, k); }

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
DECOMP(005BC8F8, SlyCharacter2_SetupDraw) {
    const u32 item = arg<u32>(ctx, 1);
    const f64 zero = sp::lfs(kText), k255 = sp::lfs(kText + 48);
    upload_item_transform(item);
    apply_fog(ctx, item);
    if ((ref<u8>(item + d3::kItemVariant) & kItemVariantLit) == 0) {
        ctx.gpr[3] = 0;  // r3 as the original leaves it (the tested bit)
        return;
    }

    Stack frame(ctx, 64);
    const u32 cols = frame.addr();
    for (u32 r = 0; r < 3; ++r) {
        for (u32 c = 0; c < 4; ++c) store<u32>(cols + 16 * r + 4 * c, lvlx_word0(item + 16 * c + 4 * r));  // lvlx, vmrghw, vperm
    }
    raw4(cols + 48, kText + 32);
    u32 params = load<u32>(load<u32>(item + d3::kItemMaterial) + d3::kMatParams);
    const u32 tint = params != 0 ? load<u32>(params + kParamsTint) : 0;
    const f64 r = channel(tint, 16, k255), g = channel(tint, 8, k255), b = channel(tint, 0, k255), a = channel(tint, 24, k255);
    set_vconsts(ctx, 6, 16, cols);
    {
        Stack lv(ctx, 16);
        const u32 v = lv.addr();
        for (u32 n = 0; n < 6; ++n) {
            const u32 src = kLights + 16 * n;
            sp::stfs(v, sp::sgl(sp::lfs(src) * r));  // fmuls
            sp::stfs(v + 4, sp::sgl(sp::lfs(src + 4) * g));
            sp::stfs(v + 8, sp::sgl(sp::lfs(src + 8) * b));
            sp::stfs(v + 12, sp::sgl(sp::lfs(src + 12) * a));
            set_vconsts(ctx, 10 + n, 4, v);
        }
    }
    if ((ref<u8>(item + d3::kItemVariant) & kItemVariantFragLayers) == 0) {
        upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
    } else {
        upload_layers_fragment(ctx, item);
    }

    params = load<u32>(load<u32>(item + d3::kItemMaterial) + d3::kMatParams);
    const u32 w = params != 0 ? params + kParamsAmbientW : kText;
    const f64 scale = params != 0 ? sp::lfs(params + kParamsAmbientScale) : zero;
    const u32 ambient = params != 0 ? load<u32>(params + kParamsAmbient) : 0;
    Stack fc(ctx, 48);  // the original's sp+112 (constant 3), +128 (direction), +144 (constant 1)
    const u32 c3 = fc.addr(), dir = c3 + 16, c1 = c3 + 32;
    sp::copy_word(c3 + 12, w);
    sp::stfs(c3, sp::sgl(channel(ambient, 16, k255) * scale));  // fmuls, fmuls
    sp::stfs(c3 + 4, sp::sgl(channel(ambient, 8, k255) * scale));
    sp::stfs(c3 + 8, sp::sgl(channel(ambient, 0, k255) * scale));
    call(ctx, d3::fn::FProgram_SetConstant, {3, c3});
    call(ctx, sly4::render::shader4::fn::Listener_LocalDirection, {dir, kEye, item});
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(c1 + i, dir + i);
    store<u32>(c1 + 12, s2::kOne);
    call(ctx, d3::fn::FProgram_SetConstant, {1, c1});
}
