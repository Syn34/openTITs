// Zone tests: the six-plane zone shapes and the zone mask lookup
// WorldObj_UpdateZoneMask uses.
#include <cmath>

#include "box.h"

using namespace sly4;
using namespace sly4::box;
using namespace sly4::obj4;

namespace {

// A plane's signed distance as the original computes it:
// ((p.y * y) + p.x * x (fused) + p.z * z (fused)) + p.d, in single precision.
inline f32 PlaneDist(u32 p, f32 x, f32 y, f32 z) {
    f32 d = static_cast<f32>(static_cast<f64>(load<f32>(p + 4)) * static_cast<f64>(y));
    d = static_cast<f32>(std::fma(static_cast<f64>(load<f32>(p)), static_cast<f64>(x), static_cast<f64>(d)));
    d = static_cast<f32>(std::fma(static_cast<f64>(load<f32>(p + 8)), static_cast<f64>(z), static_cast<f64>(d)));
    return static_cast<f32>(static_cast<f64>(d) + static_cast<f64>(load<f32>(p + 12)));
}

// 1 << b as slw computes it: shifts of 32-63 give 0.
inline u32 ZoneBit(u32 b) { return (b & 0x20) != 0 ? 0u : 1u << (b & 0x1F); }

}  // namespace

// bool Zone_Overlaps(const ZoneShape* shape, const Box* box)
// In fact a containment test: every corner of the box (bit 0 / 1 / 2 of
// the corner index pick min x / y / z, else max) lies on the inner side
// (distance >= 0; NaN passes) of all six planes.
DECOMP(00576E28, Zone_Overlaps) {
    const u32 shape = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f32 zero = load<f32>(kZoneShapeZero);
    for (u32 i = 0; i < kBoxCorners; ++i) {
        const f32 x = load<f32>(b + ((i & 1) != 0 ? 0 : 16));
        const f32 y = load<f32>(b + ((i & 2) != 0 ? 4 : 20));
        const f32 z = load<f32>(b + ((i & 4) != 0 ? 8 : 24));
        for (u32 j = 0; j < kShapePlanes; ++j)
            if (PlaneDist(shape + j * 16, x, y, z) < zero) return ret(ctx, 0u);
    }
    ret(ctx, 1u);
}

// u32 Zones_MaskFor(const Zones* zones, const Obj* o)
// The mask of the zones o lies in: the class box's corners and world box
// under the object's matrix (+80); a zone with +2 bit 0 needs all eight
// corners inside its six planes (+48), any other needs the world box inside
// its box (+16).
DECOMP(00577338, Zones_MaskFor) {
    const u32 zones = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 160);  // sp+112: the world box; sp+144: the 8 transformed corners
    const u32 wbox = frame.addr(), corners = frame.addr() + 32;
    const u32 m = o + obj::kObjMatrix + 64;
    call(ctx, bfn::Box_Corners, {load<u32>(o + kObjClassPtr) + kClassBoxMin, corners, m});
    // Only x/y/z are set; Box_Transform overwrites the whole box anyway.
    for (u32 k = 0; k < 12; k += 4) store<u32>(wbox + k, kFltMax);
    for (u32 k = 16; k < 28; k += 4) store<u32>(wbox + k, kFltMinNeg);
    call(ctx, o4fn::Box_Transform, {wbox, load<u32>(o + kObjClassPtr) + kClassBoxMin, m});
    u32 mask = 0;
    if (load<u8>(zones + 16) != 0) {
        const f32 zero = load<f32>(kZonesPlaneZero);
        u32 z = zones + 32;
        for (u32 i = 0; i < load<u8>(zones + 16); ++i, z += kZoneSize) {
            bool in = true;
            if ((load<u8>(z + 2) & 1) == 0) {
                in = static_cast<u32>(call(ctx, o4fn::Box_Overlaps, {z + 16, wbox})) != 0;
            } else {
                for (u32 c = 0; c < kBoxCorners && in; ++c) {
                    const u32 p = corners + c * 16;
                    const f32 x = load<f32>(p), y = load<f32>(p + 4), zz = load<f32>(p + 8);
                    for (u32 j = 0; j < kShapePlanes; ++j)
                        if (PlaneDist(z + 48 + j * 16, x, y, zz) < zero) {
                            in = false;
                            break;
                        }
                }
            }
            if (in) mask |= ZoneBit(load<u8>(z));
        }
    }
    ret(ctx, mask);
}
