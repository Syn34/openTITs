// Post-effect initialisers: each loads its program pairs (and extra
// fragment variants) by name, then raises some fragment programs'
// register counts. Names come either from string literals or from the
// effect's name table (read from memory at each use, as the original
// does). The original inlines VProgram_Load / FProgram_Load here.
#include "effect_init.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

const char* name_at(u32 table) { return host<const char>(global<be_u32>(table)); }
const char* str(u32 ea) { return host<const char>(ea); }
void vertex(u32 out, const char* name) { VProgram_Load(host<gp<VProgram>>(out), name, 0); }
void fragment(u32 out, const char* name, u32 bindings) {
    FProgram_Load(host<gp<FProgram>>(out), name, host<Binding>(bindings));
}
void min_regs(u32 out, u8 min) { min_registers(host<gp<FProgram>>(out), min); }

}  // namespace

// void ThermalVision_Init()
// PostProc_ThermalVision (vertex + fragment) and the _MN fragment variant
// (names at 0x00F67800 / +4), registers the thermal shader object, and
// gives both fragment programs at least 32 registers.
void ThermalVision_Init() {
    constexpr u32 kNames = 0x00F67800, kBindings = 0x00F67808;
    vertex(0x01164178, name_at(kNames));
    fragment(0x01164180, name_at(kNames), kBindings);
    fragment(0x01164184, name_at(kNames + 4), kBindings);
    Shader_Register(host<Shader>(effect::g::thermal_shader));
    min_regs(0x01164180, 32);
    min_regs(0x01164184, 32);
}

// void Bloom_Init()
// The bloom threshold copy, apply, 4x down- and up-sample program pairs.
void Bloom_Init() {
    vertex(0x01164118, str(0x00AF2324));  // PostProc_BloomCopyThreshold
    fragment(0x0116411C, str(0x00AF2324), 0x00F676E0);
    vertex(0x01164120, str(0x00AF2354));  // PostProc_BloomApply
    fragment(0x01164124, str(0x00AF2354), 0x00F676F0);
    vertex(0x01164128, str(0x00AF236C));  // PostProc_BloomDownSample4x
    fragment(0x0116412C, str(0x00AF236C), 0x00F67700);
    vertex(0x01164130, str(0x00AF238C));  // PostProc_BloomUpSample4x
    fragment(0x01164134, str(0x00AF238C), 0x00F67710);
    min_regs(0x0116411C, 32);
    min_regs(0x01164124, 32);
    min_regs(0x0116412C, 32);
    min_regs(0x01164134, 8);
}

// void Dof_Init()
// PostProc_DofApply (vertex + fragment) and three more fragment variants
// (names at 0x00F67728 +0..+12), all at least 32 registers.
void Dof_Init() {
    constexpr u32 kNames = 0x00F67728, kBindings = 0x00F67738;
    vertex(0x01164148, name_at(kNames));
    for (u32 i = 0; i < 4; ++i) fragment(0x01164150 + 4 * i, name_at(kNames + 4 * i), kBindings);
    for (u32 i = 0; i < 4; ++i) min_regs(0x01164150 + 4 * i, 32);
}

// void Effect_InitNone() -- empty.
void Effect_InitNone() {}

// void Monochrome_Init()
// PostProc_Monochrome and PostProc_InvMonochrome; the first fragment
// program gets at least 32 registers.
void Monochrome_Init() {
    constexpr u32 kBindings = 0x00F67720;
    vertex(0x01164138, str(0x00AF23AC));  // PostProc_Monochrome
    fragment(0x0116413C, str(0x00AF23AC), kBindings);
    vertex(0x01164140, str(0x00AF23C4));  // PostProc_InvMonochrome
    fragment(0x01164144, str(0x00AF23C4), kBindings);
    min_regs(0x0116413C, 32);
}

// void Warp_Init() -- the PostProc_Warp pair.
void Warp_Init() {
    vertex(0x01151A04, str(0x00AF3534));  // PostProc_Warp
    fragment(0x01151A08, str(0x00AF3534), 0);
}

// void GodRays_Load() -- PostProc_GodRaysScatter (name at 0x00F67078).
void GodRays_Load() {
    vertex(0x010F0B40, name_at(0x00F67078));
    fragment(0x010F0B44, name_at(0x00F67078), 0x00F6708C);
}

// void MotionTrail_Load() -- PostProc_MotionTrailSmear (name at 0x00F670AC).
void MotionTrail_Load() {
    vertex(0x010F0B48, name_at(0x00F670AC));
    fragment(0x010F0B4C, name_at(0x00F670AC), 0x00F670B0);
}

// void GodRays_Init() -- GodRays_Load, then at least 32 registers.
void GodRays_Init() {
    GodRays_Load();
    min_regs(0x010F0B44, 32);
}

// void MotionTrail_Init() -- MotionTrail_Load, then at least 32 registers.
void MotionTrail_Init() {
    MotionTrail_Load();
    min_regs(0x010F0B4C, 32);
}

// void BloomNoise_Init()
// PostProc_BloomDownSample4x again (into the noise effect's globals),
// PostProc_BloomNoise's vertex program and the _NoBloom and plain
// fragment variants (names at 0x00F67788 / +4); three fragment programs
// at least 32 registers.
void BloomNoise_Init() {
    constexpr u32 kNames = 0x00F67788, kBindings = 0x00F67790;
    vertex(0x01164160, str(0x00AF236C));  // PostProc_BloomDownSample4x
    fragment(0x01164164, str(0x00AF236C), 0x00F67778);
    vertex(0x01164168, name_at(kNames + 4));
    fragment(0x01164170, name_at(kNames), kBindings);
    fragment(0x01164174, name_at(kNames + 4), kBindings);
    min_regs(0x01164164, 32);
    min_regs(0x01164170, 32);
    min_regs(0x01164174, 32);
}

}  // namespace sly4::gfx

NATIVE(00546334, sly4::gfx::ThermalVision_Init)
NATIVE(0053D800, sly4::gfx::Bloom_Init)
NATIVE(00540824, sly4::gfx::Dof_Init)
NATIVE(0053EC4C, sly4::gfx::Effect_InitNone)
NATIVE(0053ECE0, sly4::gfx::Monochrome_Init)
NATIVE(0054588C, sly4::gfx::Warp_Init)
NATIVE(00543774, sly4::gfx::GodRays_Init)
NATIVE(005447F4, sly4::gfx::MotionTrail_Init)
NATIVE(005420CC, sly4::gfx::BloomNoise_Init)
NATIVE(0051EE94, sly4::gfx::GodRays_Load)
NATIVE(0051F0FC, sly4::gfx::MotionTrail_Load)
