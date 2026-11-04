// Post-effect initialisers: each loads its program pairs (and extra
// fragment variants) by name, then raises some fragment programs'
// register counts. Names come either from string literals or from the
// effect's name table (read from memory at each use, as the original does).
#include "effect_init.h"

#include "prog_load.h"

using namespace sly4;

namespace {

namespace efn = sly4::effect::fn;
using progload::load_fragment;
using progload::load_vertex;
using progload::min_registers;

}  // namespace

// void ThermalVision_Init()
// PostProc_ThermalVision (vertex + fragment) and the _MN fragment variant
// (names at 0x00F67800 / +4), registers the thermal shader object, and
// gives both fragment programs at least 32 registers.
DECOMP(00546334, ThermalVision_Init) {
    constexpr u32 kNames = 0x00F67800, kBindings = 0x00F67808;
    load_vertex(ctx, 0x01164178, load<u32>(kNames));
    load_fragment(ctx, 0x01164180, load<u32>(kNames), kBindings);
    load_fragment(ctx, 0x01164184, load<u32>(kNames + 4), kBindings);
    call(ctx, efn::Shader_Register, {effect::g::thermal_shader});
    min_registers(ctx, 0x01164180, 32);
    min_registers(ctx, 0x01164184, 32);
}

// void Bloom_Init()
// The bloom threshold copy, apply, 4x down- and up-sample program pairs.
DECOMP(0053D800, Bloom_Init) {
    load_vertex(ctx, 0x01164118, 0x00AF2324);  // PostProc_BloomCopyThreshold
    load_fragment(ctx, 0x0116411C, 0x00AF2324, 0x00F676E0);
    load_vertex(ctx, 0x01164120, 0x00AF2354);  // PostProc_BloomApply
    load_fragment(ctx, 0x01164124, 0x00AF2354, 0x00F676F0);
    load_vertex(ctx, 0x01164128, 0x00AF236C);  // PostProc_BloomDownSample4x
    load_fragment(ctx, 0x0116412C, 0x00AF236C, 0x00F67700);
    load_vertex(ctx, 0x01164130, 0x00AF238C);  // PostProc_BloomUpSample4x
    load_fragment(ctx, 0x01164134, 0x00AF238C, 0x00F67710);
    min_registers(ctx, 0x0116411C, 32);
    min_registers(ctx, 0x01164124, 32);
    min_registers(ctx, 0x0116412C, 32);
    min_registers(ctx, 0x01164134, 8);
}

// void Dof_Init()
// PostProc_DofApply (vertex + fragment) and three more fragment variants
// (names at 0x00F67728 +0..+12), all at least 32 registers.
DECOMP(00540824, Dof_Init) {
    constexpr u32 kNames = 0x00F67728, kBindings = 0x00F67738;
    load_vertex(ctx, 0x01164148, load<u32>(kNames));
    for (u32 i = 0; i < 4; ++i) load_fragment(ctx, 0x01164150 + 4 * i, load<u32>(kNames + 4 * i), kBindings);
    for (u32 i = 0; i < 4; ++i) min_registers(ctx, 0x01164150 + 4 * i, 32);
}

// void Effect_InitNone() — empty.
DECOMP(0053EC4C, Effect_InitNone) {}

// void Monochrome_Init()
// PostProc_Monochrome and PostProc_InvMonochrome; the first fragment
// program gets at least 32 registers.
DECOMP(0053ECE0, Monochrome_Init) {
    constexpr u32 kBindings = 0x00F67720;
    load_vertex(ctx, 0x01164138, 0x00AF23AC);  // PostProc_Monochrome
    load_fragment(ctx, 0x0116413C, 0x00AF23AC, kBindings);
    load_vertex(ctx, 0x01164140, 0x00AF23C4);  // PostProc_InvMonochrome
    load_fragment(ctx, 0x01164144, 0x00AF23C4, kBindings);
    min_registers(ctx, 0x0116413C, 32);
}

// void Warp_Init() — the PostProc_Warp pair.
DECOMP(0054588C, Warp_Init) {
    load_vertex(ctx, 0x01151A04, 0x00AF3534);  // PostProc_Warp
    ctx.gpr[3] = load_fragment(ctx, 0x01151A08, 0x00AF3534, 0);
}

// void GodRays_Init() — GodRays_Load, then at least 32 registers.
DECOMP(00543774, GodRays_Init) {
    call(ctx, efn::GodRays_Load);
    min_registers(ctx, 0x010F0B44, 32);
}

// void MotionTrail_Init() — MotionTrail_Load, then at least 32 registers.
DECOMP(005447F4, MotionTrail_Init) {
    call(ctx, efn::MotionTrail_Load);
    min_registers(ctx, 0x010F0B4C, 32);
}

// void BloomNoise_Init()
// PostProc_BloomDownSample4x again (into the noise effect's globals),
// PostProc_BloomNoise's vertex program and the _NoBloom and plain
// fragment variants (names at 0x00F67788 / +4); three fragment programs
// at least 32 registers.
DECOMP(005420CC, BloomNoise_Init) {
    constexpr u32 kNames = 0x00F67788, kBindings = 0x00F67790;
    load_vertex(ctx, 0x01164160, 0x00AF236C);  // PostProc_BloomDownSample4x
    load_fragment(ctx, 0x01164164, 0x00AF236C, 0x00F67778);
    load_vertex(ctx, 0x01164168, load<u32>(kNames + 4));
    load_fragment(ctx, 0x01164170, load<u32>(kNames), kBindings);
    load_fragment(ctx, 0x01164174, load<u32>(kNames + 4), kBindings);
    min_registers(ctx, 0x01164164, 32);
    min_registers(ctx, 0x01164170, 32);
    min_registers(ctx, 0x01164174, 32);
}

// void GodRays_Load() — PostProc_GodRaysScatter (name at 0x00F67078).
DECOMP(0051EE94, GodRays_Load) {
    // r5 = 0x00F6707C is passed to VProgram_Load, which ignores it.
    ctx.gpr[3] = call(ctx, progload::fn::VProgram_Load, {0x010F0B40, load<u32>(0x00F67078), 0x00F6707C});
    ctx.gpr[3] = call(ctx, progload::fn::FProgram_Load, {0x010F0B44, load<u32>(0x00F67078), 0x00F6708C});
}

// void MotionTrail_Load() — PostProc_MotionTrailSmear (name at 0x00F670AC).
DECOMP(0051F0FC, MotionTrail_Load) {
    call(ctx, progload::fn::VProgram_Load, {0x010F0B48, load<u32>(0x00F670AC), 0});
    ctx.gpr[3] = call(ctx, progload::fn::FProgram_Load, {0x010F0B4C, load<u32>(0x00F670AC), 0x00F670B0});
}
