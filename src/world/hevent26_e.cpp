// World_HandleEvent callees, part 26e: camera controller constructors
// picked by World_SetCameraTarget484858.
#include <bit>

#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kAtan2f = 0x005E665C;           // (f32 y, f32 x) -> f32 (symbols.txt atan2f)
constexpr u32 kCamCtrl3SetTarget = 0x001134D8;  // (CamCtrl*) re-target after a new object (name?)
constexpr u32 kZeroVec = 0x01164B40, kVec2 = 0x01164AFC;
// Camera tuning in 0x01036540..: per-slot 76-byte records at 0x01036540 /
// 0x010365D8 (f32 at +72), a vec3 at 0x010366C0 (degrees) and 96-byte
// records at 0x01036730 (vec3 at +80).
constexpr u32 kTuneA = 0x01036540, kTuneB = 0x010365D8, kTuneAngles = 0x010366C0, kTuneOffsets = 0x01036730;
// Constants at 0x0011428C: f32 0.0, f32 pi / 180.
constexpr u32 kConsts114298 = 0x0011428C;
constexpr u32 kVtCamCtrl3 = 0x00B13F70, kVtCamCtrl4 = 0x00B14020;
// Objects count camera references in a u16 at +178; a held reference is
// released through vt +40 (ref, 0).
constexpr u32 kObjCamRefs = 178, kVtRelease = 40;

inline f32 lf(u32 ea) { return std::bit_cast<f32>(load<u32>(ea)); }
void copy_words(u32 dst, u32 src, u32 n) {
    for (u32 k = 0; k < n; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
}
}  // namespace

// ---------------------------------------------------------------- 0x00113608
// u32 CamCtrl3_Construct113608(CamCtrl3* c, Camera* cam, Object* obj; f32
// a) (name?): vtable 0x00B13F70, +4 = cam, +8 = 0, +80 = a, +96 = 0, +100
// = 2.0f, +104 / +112 / +116 = 0, +120 = 1.0f; the +84 target is released
// (vt +40 (0)) and replaced by obj, which gets a camera reference (+178)
// and CamCtrl3 re-targeting (0x001134D8).
DECOMP(00113608, CamCtrl3_Construct113608) {
    const u32 c = arg<u32>(ctx, 0), cam = arg<u32>(ctx, 1), obj = arg<u32>(ctx, 2);
    store<u32>(c + 4, cam);
    store<f32>(c + 80, static_cast<f32>(farg(ctx, 0)));
    store<u8>(c + 8, 0);
    store<u32>(c + 96, 0);
    store<u32>(c + 0, kVtCamCtrl3);
    store<u32>(c + 100, 0x40000000u);  // 2.0f
    store<u32>(c + 104, 0);
    const u32 old = load<u32>(c + 84);
    store<u32>(c + 112, 0);
    store<u32>(c + 116, 0);
    store<u32>(c + 120, 0x3F800000u);  // 1.0f
    u64 r3 = old;
    if (old != 0) r3 = vcall(ctx, old, kVtRelease, {old, 0});
    store<u32>(c + 84, obj);
    if (obj == 0) return ret(ctx, r3);
    store<u16>(obj + kObjCamRefs, static_cast<u16>(load<u16>(obj + kObjCamRefs) + 1));
    call(ctx, kCamCtrl3SetTarget, {c});  // r3 as it leaves it
}

// ---------------------------------------------------------------- 0x00114298
// u32 CamCtrl4_Construct114298(CamCtrl4* c, Camera* cam, Object* obj, s32
// slot) (name?): vtable 0x00B14020, +4 = cam, +8 = 0, +12 / +16 / +64 /
// +68 = 0, +32 = the zero vec3, +48 / +56 / +120 / +128 = the vec2 at
// 0x01164AFC; +96 = the 0x010366C0 angles in radians, +112 = obj's heading
// atan2(obj +128, obj +136) (0 without obj), +116 = +104; +144 = the slot's
// offset (0x01036730 +96*slot +80), +160 / +164 = the slot's 0x01036540 /
// 0x010365D8 +76*slot +72; +168 bit 31 set, bits 29..30 = slot & 3, bits
// 26..28 cleared. The previous +12 reference (none after the clear above
// unless obj aliases it) is released and obj referenced (+178 += 1).
DECOMP(00114298, CamCtrl4_Construct114298) {
    const u32 c = arg<u32>(ctx, 0), cam = arg<u32>(ctx, 1), obj = arg<u32>(ctx, 2), slot = arg<u32>(ctx, 3);
    const u32 K = kConsts114298;
    store<u32>(c + 4, cam);
    store<u8>(c + 8, 0);
    store<u32>(c + 0, kVtCamCtrl4);
    store<u32>(c + 12, 0);
    store<u32>(c + 16, 0);
    copy_words(c + 32, kZeroVec, 3);
    store<u32>(c + 48, load<u32>(kVec2));
    store<u32>(c + 52, load<u32>(kVec2 + 4));
    store<u32>(c + 56, load<u32>(kVec2));
    store<u32>(c + 64, 0);
    store<u32>(c + 60, load<u32>(kVec2 + 4));
    store<u32>(c + 68, 0);
    const f64 rad = lf(K + 4);
    const f32 az = fmuls(lf(kTuneAngles + 8), rad);
    store<f32>(c + 96, fmuls(lf(kTuneAngles + 0), rad));
    store<f32>(c + 100, fmuls(lf(kTuneAngles + 4), rad));
    store<f32>(c + 104, az);
    u32 heading = load<u32>(K + 0);
    u32 f1 = std::bit_cast<u32>(az);
    u32 old = 0;
    if (obj != 0) {
        const f64 h = callf(ctx, kAtan2f, {}, {lf(obj + 128), lf(obj + 136)});
        heading = std::bit_cast<u32>(static_cast<f32>(h));
        old = load<u32>(c + 12);
        f1 = load<u32>(c + 104);
    }
    store<u32>(c + 112, heading);
    store<u32>(c + 116, f1);
    store<u32>(c + 120, load<u32>(kVec2));
    store<u32>(c + 124, load<u32>(kVec2 + 4));
    store<u32>(c + 128, load<u32>(kVec2));
    store<u32>(c + 132, load<u32>(kVec2 + 4));
    copy_words(c + 144, kTuneOffsets + 96 * slot + 80, 3);
    const u32 w = load<u32>(c + 168);
    store<u32>(c + 160, load<u32>(kTuneA + 76 * slot + 72));
    store<u32>(c + 164, load<u32>(kTuneB + 76 * slot + 72));
    store<u32>(c + 168, (w & ~0x7C000000u) | 0x80000000u | ((slot & 3u) << 29));
    ctx.fpr[1] = lf(kTuneB + 76 * slot + 72);  // the last lfs
    u64 r3 = 0;
    if (old != 0) r3 = vcall(ctx, old, kVtRelease, {old, 0});
    store<u32>(c + 12, obj);
    if (obj != 0) {
        store<u16>(obj + kObjCamRefs, static_cast<u16>(load<u16>(obj + kObjCamRefs) + 1));
        r3 = obj + kObjCamRefs;
    }
    ret(ctx, r3);
}

// ---------------------------------------------------------------- 0x004845F0
namespace {
constexpr u32 kWorldCamera = 0x1281E0;        // World +0x1281E0: Camera* (layer stack: u8 +345 depth, +368.. CamCtrl*[])
constexpr u32 kWorldCamTarget = 0x128584;     // World +0x128584: CamCtrl* of the current target (as World_SetCameraTarget484858)
constexpr u32 kCamLayers = 364, kCamDepth = 345, kCamFocus = 432;
constexpr u32 kPopBlend = 0x004845EC;         // f32 -1.0 (blend time given to Obj_PopLayer)
constexpr u32 kObjPopLayer = 0x000FBB88;      // (Camera*, CamCtrl*; f32) (symbols.txt Obj_PopLayer)
// Controllers drop their object reference through a kind-specific setter
// (vt +8 kind): 1 / 11 / 2 / 7 / 9.
constexpr u32 kSetRefKind1 = 0x001088E4, kSetRefKind11 = 0x000FEEBC, kSetRefKind2 = 0x00108A34,
              kSetRefKind7 = 0x0010DD60, kSetRefKind9 = 0x0011135C;
}  // namespace

// void World_PopCameraLayers4845F0(World* w) (name?): while the world
// camera's layer stack (u8 +345 depth, entries at +364 + 4 * depth) has a
// top controller: its object reference is dropped by kind (vt +8, asked
// again for each test), the camera's +432 focus cleared when it is the
// one, Obj_PopLayer(cam, ctrl; -1.0f), and a matching current target
// (w +0x128584) gets vt +4 and is cleared.
DECOMP(004845F0, World_PopCameraLayers4845F0) {
    const u32 w = arg<u32>(ctx, 0);
    if (load<u32>(w + kWorldCamera) == 0) return ret(ctx, 0u);
    const f64 blend = lf(kPopBlend);
    for (;;) {
        const u32 cam = load<u32>(w + kWorldCamera);
        const u32 n = load<u8>(cam + kCamDepth);
        u32 top = 0;
        if (n != 0) top = load<u32>(cam + 4 * n + kCamLayers);
        if (top == 0) return ret(ctx, n != 0 ? cam + 4 * n + kCamLayers : 0u);
        auto kind = [&] { return static_cast<s32>(vcall(ctx, top, 8, {top})); };
        if (kind() == 1) call(ctx, kSetRefKind1, {top, 0});
        else if (kind() == 11) call(ctx, kSetRefKind11, {top, 0});
        else if (kind() == 2) call(ctx, kSetRefKind2, {top, 0});
        else if (kind() == 7) call(ctx, kSetRefKind7, {top, 0});
        else if (kind() == 9) call(ctx, kSetRefKind9, {top, 0});
        u32 c = load<u32>(w + kWorldCamera);
        if (load<u32>(c + kCamFocus) == top) {
            store<u32>(c + kCamFocus, 0);
            c = load<u32>(w + kWorldCamera);
        }
        callf(ctx, kObjPopLayer, {c, top}, {blend});
        const u32 cur = load<u32>(w + kWorldCamTarget);
        if (cur == top && cur != 0) {
            vcall(ctx, cur, 4, {cur});
            store<u32>(w + kWorldCamTarget, 0);
        }
    }
}

// ---------------------------------------------------------------- 0x00111410
namespace {
constexpr u32 kVtCamCtrl2 = 0x00B13E58;
constexpr u32 kShapeConstruct = 0x009F6BB0;   // (Shape*) in-place constructor of the +16 collision shape (Havok side, name?)
constexpr u32 kShapeDescConstruct = 0x009F6A74;  // (Desc*) (name?)
constexpr u32 kShapeInit = 0x009F63EC;        // (Shape*, const Desc*) (name?)
constexpr u32 kMatIdentity = 0x01164BD0;      // 3x4 identity (rows at +0 / +16 / +32 / +48)
constexpr u32 kDescAxisA = 0x01164B60, kDescAxisB = 0x01164B70;  // vec3 constants
constexpr u32 kObjCamFlags = 200, kObjCamShaped = 0x400, kObjMatrix = 96, kObjShapeSrc = 216;

void copy_rows(u32 dst, u32 src) {  // 3x4 matrix, xyz of each row
    for (u32 r = 0; r < 4; ++r) copy_words(dst + 16 * r, src + 16 * r, 3);
}
void xyz0(u32 dst, u32 src) {  // (x, y, z, 0): splats merged by lane masks (0x001113E0)
    copy_words(dst, src, 3);
    store<u32>(dst + 12, 0);
}
}  // namespace

// u32 CamCtrl2_Construct111410(CamCtrl2* c, Camera* cam, Object* obj)
// (name?): vtable 0x00B13E58, +4 = cam, +8 = 0, +12 = 0, collision shape
// at +16 constructed, +208 = identity. For an obj with +200 bit 10: a
// shape descriptor (stack: +0 = 0, +4 = 1.0f, two constant axes, rows
// from the obj +216 source at +560 / +608 / +576 / +624, words +592 /
// +640) initialises the shape and +208 = obj +96 matrix; other objects
// get vt +76. The +12 reference is replaced by obj (+178 += 1).
DECOMP(00111410, CamCtrl2_Construct111410) {
    const u32 c = arg<u32>(ctx, 0), cam = arg<u32>(ctx, 1), obj = arg<u32>(ctx, 2);
    store<u32>(c + 4, cam);
    store<u8>(c + 8, 0);
    store<u32>(c + 12, 0);
    store<u32>(c + 0, kVtCamCtrl2);
    call(ctx, kShapeConstruct, {c + 16});
    copy_rows(c + 208, kMatIdentity);
    ctx.fpr[1] = lf(kMatIdentity + 56);  // the last lfs
    if (obj != 0 && (load<u32>(obj + kObjCamFlags) & kObjCamShaped) != 0) {
        const u32 src = load<u32>(obj + kObjShapeSrc);
        // Descriptor as the original's at sp+112 (144 bytes).
        Stack frame(ctx, 144);
        const u32 ds = frame.addr();
        call(ctx, kShapeDescConstruct, {ds});
        store<u32>(ds + 0, 0);
        store<u32>(ds + 4, 0x3F800000u);  // 1.0f
        xyz0(ds + 16, kDescAxisA);
        xyz0(ds + 32, kDescAxisB);
        xyz0(ds + 48, src + 560);
        xyz0(ds + 96, src + 608);
        xyz0(ds + 64, src + 576);
        xyz0(ds + 112, src + 624);
        for (u32 k = 0; k < 16; k += 4) {
            store<u32>(ds + 80 + k, load<u32>(src + 592 + k));
            store<u32>(ds + 128 + k, load<u32>(src + 640 + k));
        }
        call(ctx, kShapeInit, {c + 16, ds});
        copy_rows(c + 208, obj + kObjMatrix);
        ctx.fpr[1] = lf(obj + kObjMatrix + 56);
    } else if (obj != 0) {
        vcall(ctx, obj, 76, {obj});
    }
    const u32 old = load<u32>(c + 12);
    u64 r3 = old;
    if (old != 0) r3 = vcall(ctx, old, kVtRelease, {old, 0});
    store<u32>(c + 12, obj);
    if (obj != 0) {
        store<u16>(obj + kObjCamRefs, static_cast<u16>(load<u16>(obj + kObjCamRefs) + 1));
        r3 = obj + kObjCamRefs;
    }
    ret(ctx, r3);
}
