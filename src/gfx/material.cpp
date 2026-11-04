// AglMaterial construction (Material_Construct) and adding a shader to the
// shader registry (Shader_Register).
#include "material.h"

using namespace sly4;

namespace {

namespace mfn = sly4::material::fn;
namespace mg = sly4::material::g;

}  // namespace

// AglMaterial* Material_Construct(AglMaterial* m)
// Base object part (serial << 11 in +8), no textures (+20..+36, released
// if set), blend words at +104 / +112, render state +116 = 0x900, a new
// material id at +46, the default colour twice (+64, +80), alpha 1.0 at
// +96. Leaves r3 = 0x3F800000, as the original does.
DECOMP(004FF0D8, Material_Construct) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 w4 = (load<u32>(m + 4) & 0x03FF) | 0x00011000;
    store<u16>(m + 4, 1);
    store<u32>(m + 4, w4);
    const u32 serial = load<u32>(mg::serial) + 1;
    store<u32>(m, mg::vt_base0);
    store<u32>(mg::serial, serial);
    const u32 w8 = load<u32>(m + 8);
    store<u32>(m + 12, 0);
    store<u32>(m, mg::vt_material_base);
    store<u32>(m + 8, (serial << 11) | (w8 & 0x3F));
    for (u32 off : {20u, 24u, 28u, 32u, 36u}) store<u32>(m + off, 0);
    const u32 w104 = load<u32>(m + 104), w112 = load<u32>(m + 112);
    store<u16>(m + 108, 0x8006);
    store<u16>(m + 110, 771);
    store<u32>(m + 104, (w104 & 0x00007FFF) | 0x01810000);
    store<u16>(m + 116, 2304);
    store<u32>(m + 16, 0);
    store<u32>(m + 112, (w112 & 0x00007FFF) | 0x80000000u | 0x01018000);
    for (u32 k = 0; k < 5; ++k) {
        const u32 slot = m + 20 + 4 * k;
        const u32 t = load<u32>(slot);
        if (t == 0) continue;
        store<u32>(slot, 0);
        call_ptr(ctx, load<u32>(load<u32>(t) + 8), {t});  // release
    }
    ref<u8>(m + 44) = 0;
    const u32 id = load<u32>(mg::material_count);
    store<u32>(mg::material_count, id + 1);
    const u32 c = mg::default_colour;
    store<u32>(m + 64, load<u32>(c));
    store<u32>(m + 68, load<u32>(c + 4));
    store<u32>(m + 72, load<u32>(c + 8));
    store<u16>(m + 46, static_cast<u16>(id));
    store<u32>(m + 76, load<u32>(c + 12));
    store<u32>(m + 80, load<u32>(c));
    store<u32>(m + 84, load<u32>(c + 4));
    const u8 b100 = ref<u8>(m + 100);
    store<u32>(m + 88, load<u32>(c + 8));
    const u32 w120 = load<u32>(m + 120);
    store<u32>(m + 92, load<u32>(c + 12));
    store<u32>(m + 96, 0x3F800000);  // 1.0f
    store<u32>(m + 128, 0);
    ref<u8>(m + 100) = static_cast<u8>((b100 | 0xE0) & ~0x08);
    store<u32>(m + 120, (w120 & 0x0FFFFFFF) | 0x20000000);
    store<u32>(m + 124, 0);
    ref<u8>(m + 121) = 0;
    ctx.gpr[3] = 0x3F800000;
}

// bool Shader_Register(Shader* s)
// Calls the shader's first virtual function, appends it to the shader
// registry (made on first use) and sets flag 2 in +24. Returns 1.
DECOMP(0050AFEC, Shader_Register) {
    const u32 s = arg<u32>(ctx, 0);
    call_ptr(ctx, load<u32>(load<u32>(s)), {s});
    if (ref<u8>(mg::registry_made) == 0) {
        u32 v = static_cast<u32>(call(ctx, fn::Mem_Alloc, {20, 0, 16}));
        if (v != 0) {
            call(ctx, mfn::PtrVector_Construct, {v});
            store<u16>(v + 6, 4);
            call(ctx, mfn::PtrVector_Init, {v, 512, 0, 0});
        }
        store<u32>(mg::registry, v);
        ref<u8>(mg::registry_made) = 1;
    }
    const u32 v = load<u32>(mg::registry);
    u32 count = load<u16>(v + 8);
    if (count == load<u16>(v + 4)) {
        call(ctx, mfn::PtrVector_Reserve, {v, static_cast<u32>(load<u16>(v + 4) << 1)});
        count = load<u16>(v + 8);
    }
    store<u32>(load<u32>(v) + 4 * count, s);
    const u32 n = load<u16>(v + 8) + 1u;
    const u16 high = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : static_cast<u16>(n));
    store<u32>(s + 24, load<u32>(s + 24) | 2);
    ctx.gpr[3] = 1;
}
