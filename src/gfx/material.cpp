// AglMaterial construction (Material_Construct) and adding a shader to the
// shader registry (Shader_Register).
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 PtrVector_Init = 0x00551598;      // (vector, capacity, 0, 0) as used here
constexpr u32 PtrVector_Reserve = 0x00551718;   // (vector, capacity)
constexpr u32 kVtBase0 = 0x00B242B8;            // written first, as by RenderSurface_Construct
constexpr u32 kVtMaterialBase = 0x00B233B0;
constexpr u32 kSerial = 0x01164CD0;             // u32: shared with RenderSurface_Construct
constexpr u32 kMaterialCount = 0x010DD5C0;      // u32: materials made (id in +46)
constexpr u32 kDefaultColour = 0x01164BB0;      // 4 x f32, copied to +64 and +80
constexpr u32 kRegistryMade = 0x010F0BA0;       // u8  (see shadergroup.h)
constexpr u32 kRegistry = 0x010F0BA8;           // PtrVector* of shaders

}  // namespace

// void Material_Construct(AglMaterial* m)
// Base object part (serial << 11 in +8), no textures (+20..+36, released
// if set), blend words at +104 / +112, render state +116 = 0x900, a new
// material id at +46, the default colour twice (+64, +80), alpha 1.0 at
// +96. (The original leaves 1.0f in r3, not m.)
void Material_Construct(Material* m) {
    const u32 w4 = (m->w4 & 0x03FF) | 0x00011000;
    *at<be_u16>(m, 4) = 1;
    m->w4 = w4;
    be_u32& serial_ref = global<be_u32>(kSerial);
    const u32 serial = serial_ref + 1;
    m->vtable = kVtBase0;
    serial_ref = serial;
    const u32 w8 = m->serial;
    m->w12 = 0u;
    m->vtable = kVtMaterialBase;
    m->serial = (serial << 11) | (w8 & 0x3F);
    for (u32 k = 0; k < 5; ++k) m->textures[k] = nullptr;
    const u32 w104 = m->blend, w112 = m->blend2;
    m->blend_eq = 0x8006;
    m->blend_dst = 771;
    m->blend = (w104 & 0x00007FFF) | 0x01810000;
    m->state = 2304;
    m->w16 = 0u;
    m->blend2 = (w112 & 0x00007FFF) | 0x80000000u | 0x01018000;
    for (u32 k = 0; k < 5; ++k) {
        u8* t = m->textures[k];
        if (!t) continue;
        m->textures[k] = nullptr;
        vcall(t, 8);  // release
    }
    m->b44 = 0;
    be_u32& count = global<be_u32>(kMaterialCount);
    const u32 id = count;
    count = id + 1;
    const be_u32* c = host<be_u32>(kDefaultColour);
    m->colour[0] = c[0].get();
    m->colour[1] = c[1].get();
    m->colour[2] = c[2].get();
    m->id = static_cast<u16>(id);
    m->colour[3] = c[3].get();
    m->colour2[0] = c[0].get();
    m->colour2[1] = c[1].get();
    const u8 b100 = m->b100;
    m->colour2[2] = c[2].get();
    const u32 w120 = m->w120;
    m->colour2[3] = c[3].get();
    m->alpha = 0x3F800000u;  // 1.0f
    m->w128 = 0u;
    m->b100 = static_cast<u8>((b100 | 0xE0) & ~0x08);
    m->w120 = (w120 & 0x0FFFFFFF) | 0x20000000;
    m->w124 = 0u;
    *at<u8>(m, 121) = 0;
}

// bool Shader_Register(Shader* s)
// Calls the shader's first virtual function, appends it to the shader
// registry (made on first use) and sets flag 2 in +24. Returns 1.
bool Shader_Register(Shader* s) {
    vcall(s, 0);
    u8& made = global<u8>(kRegistryMade);
    if (made == 0) {
        PtrVector* v = call<PtrVector*>(sly4::fn::Mem_Alloc, 20u, 0u, 16u);
        if (v) {
            call(PtrVector_Construct, v);
            v->elem_size = 4;
            call(PtrVector_Init, v, 512u, 0u, 0u);
        }
        global<gp<PtrVector>>(kRegistry) = v;
        made = 1;
    }
    PtrVector* v = or_zero_page(global<gp<PtrVector>>(kRegistry).get());
    u32 count = v->count;
    if (count == v->capacity) {
        call(PtrVector_Reserve, v, static_cast<u32>(v->capacity << 1));
        count = v->count;
    }
    *at<be_u32>(v->data.get(), 4 * count) = guest(s);
    const u32 n = v->count + 1u;
    const u16 high = v->high_water;
    v->count = static_cast<u16>(n);
    v->high_water = static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : static_cast<u16>(n);
    s->flags = s->flags | 2;
    return true;
}

}  // namespace sly4::gfx

NATIVE(004FF0D8, sly4::gfx::Material_Construct)
NATIVE(0050AFEC, sly4::gfx::Shader_Register)
