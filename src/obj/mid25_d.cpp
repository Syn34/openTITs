// Game code, medium sweep part 25: parameter registration lists
// (Obj_RegisterParam over static descriptors, enum parameters getting their
// default name lists) of the spawner and the laser anchor / spline objects.
#include "mid25.h"

using namespace sly4;
using namespace sly4::mid25;

namespace {
constexpr u32 kRegisterParam = 0x004F7128u;  // Obj_RegisterParam(o, const ParamDesc*)
constexpr u32 kBindingReserve = 0x004D2BB8u;  // Binding_Reserve(b, n)

// Obj_RegisterParam(o, d) for each descriptor.
void Reg(PPUContext& ctx, u32 o, std::initializer_list<u32> ds) {
    for (u32 d : ds) call(ctx, kRegisterParam, {o, d});
}

// An enum parameter: Obj_RegisterParam(o, d), Binding_Reserve(d->+16, n)
// and the names (each appended while the binding's list has room; the
// binding re-read from d->+16 each time).
void RegEnum(PPUContext& ctx, u32 o, u32 d, std::initializer_list<u32> names) {
    call(ctx, kRegisterParam, {o, d});
    call(ctx, kBindingReserve, {load<u32>(d + 16), static_cast<u32>(names.size())});
    for (u32 s : names) mid4::NodeBindingAddName(ctx, load<u32>(d + 16), s);
}

// As RegEnum with the n names taken from the u32 table at tbl.
void RegEnumTable(PPUContext& ctx, u32 o, u32 d, u32 tbl, u32 n) {
    call(ctx, kRegisterParam, {o, d});
    call(ctx, kBindingReserve, {load<u32>(d + 16), n});
    for (u32 k = 0; k < n; ++k) mid4::NodeBindingAddName(ctx, load<u32>(d + 16), load<u32>(tbl + 4 * k));
}

// The class name tail of the laser parameter lists: o->+8 (the class
// record) gets its name +44 = prefix and bit 0x80 of the byte +40, and
// o->+88 = 0x00F38258. r3 = o->+8.
void LaserClassTail(PPUContext& ctx, u32 o, u32 prefix) {
    const u32 c = load<u32>(o + 8);
    store<u32>(c + 44, prefix);
    store<u8>(c + 40, static_cast<u8>(load<u8>(c + 40) | 0x80u));
    store<u32>(o + 88, 0x00F38258u);
    ctx.gpr[3] = c;
}
}  // namespace

// void f(Object* o) (Obj_InitParams_0043C070, 0x0043C1B0): the spawner
// parameters: the 33 descriptors 0x010AAFF8.. (24 bytes apart) registered,
// the tenth (0x010AB0D0) an enum with the names "SpawnAtBegining",
// "SpawnBaseOnTrigger", "SpawnBaseOnScript". r3 = the last call's.
DECOMP(0043BC58, Spawner_RegisterParams_0043BC58) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 d = 0x010AAFF8u; d < 0x010AB0D0u; d += 24) Reg(ctx, o, {d});
    RegEnum(ctx, o, 0x010AB0D0u, {0x00AE36B0u, 0x00AE36C0u, 0x00AE36D8u});
    for (u32 d = 0x010AB0E8u; d <= 0x010AB2F8u; d += 24) Reg(ctx, o, {d});
}

// void f(Object* o) (no direct caller): the SLaserAnchor_ parameters: enum
// descriptors 0x010AB6F8 / 0x010AB728 (2 names from the tables 0x00F657CC /
// 0x00F657EC), 0x010AB7E8 / 0x010AB7D0 (3 names from 0x00F657B4 /
// 0x00F6579C) among the plain ones; then LaserClassTail("SLaserAnchor_").
DECOMP(0043CA48, LaserAnchor_RegisterParams_0043CA48) {
    const u32 o = arg<u32>(ctx, 0);
    Reg(ctx, o, {0x010AB6E0u});
    RegEnumTable(ctx, o, 0x010AB6F8u, 0x00F657CCu, 2);
    Reg(ctx, o, {0x010AB710u});
    RegEnumTable(ctx, o, 0x010AB728u, 0x00F657ECu, 2);
    Reg(ctx, o, {0x010AB740u, 0x010AB758u, 0x010AB770u, 0x010AB788u, 0x010AB7A0u, 0x010AB7B8u});
    RegEnumTable(ctx, o, 0x010AB7E8u, 0x00F657B4u, 3);
    Reg(ctx, o, {0x010AB800u, 0x010AB818u, 0x010AB830u, 0x010AB848u});
    RegEnumTable(ctx, o, 0x010AB7D0u, 0x00F6579Cu, 3);
    Reg(ctx, o, {0x010AB860u});
    LaserClassTail(ctx, o, 0x00AE3740u);  // "SLaserAnchor_"
}

// void f(Object* o) (no direct caller): the SLaserSpline_ parameters: the
// two 2-name enums as the anchor's, two plain descriptors, then for both
// ends (0, 1) a block of plain descriptors and two 3-name enums (tables
// 0x00F657B4 / 0x00F6579C), the second end with one extra descriptor before
// each enum group; then LaserClassTail("SLaserSpline_").
DECOMP(0043CF48, LaserSpline_RegisterParams_0043CF48) {
    const u32 o = arg<u32>(ctx, 0);
    Reg(ctx, o, {0x010AB878u});
    RegEnumTable(ctx, o, 0x010AB890u, 0x00F657CCu, 2);
    Reg(ctx, o, {0x010AB8A8u});
    RegEnumTable(ctx, o, 0x010AB8C0u, 0x00F657ECu, 2);
    Reg(ctx, o, {0x010AB8D8u, 0x010AB8F0u});
    for (u32 side = 0; side < 2; ++side) {
        const bool b = side != 0;
        Reg(ctx, o, {b ? 0x010AB9C8u : 0x010AB908u, b ? 0x010AB9E0u : 0x010AB920u});
        if (b) Reg(ctx, o, {0x010AB9F8u});
        RegEnumTable(ctx, o, b ? 0x010ABA28u : 0x010AB950u, 0x00F657B4u, 3);
        Reg(ctx, o, {b ? 0x010ABA40u : 0x010AB968u, b ? 0x010ABA58u : 0x010AB980u});
        if (b) Reg(ctx, o, {0x010ABA70u});
        Reg(ctx, o, {b ? 0x010ABA88u : 0x010AB998u, b ? 0x010ABAA0u : 0x010AB9B0u});
        RegEnumTable(ctx, o, b ? 0x010ABA10u : 0x010AB938u, 0x00F6579Cu, 3);
    }
    Reg(ctx, o, {0x010ABAB8u});
    LaserClassTail(ctx, o, 0x00AE3750u);  // "SLaserSpline_"
}
