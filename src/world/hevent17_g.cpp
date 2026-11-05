// World_HandleEvent callees, part 17g: the camera target constructor.
#include "hevent17.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent17;

namespace {
// Target (688 bytes): +0 vtable, +4 Camera*, +8 u8 priority, +12 Object*,
// +16 s32 bone, +32 vec3, +48 f32[4][3] frame (rows of 16), +112 f32, +128
// vec3, +144 / +148 u32, +152 f32, +160 f32[4], +176 u32, +180 u16, +182
// u16 flags (bits 13..15 cleared, 10..12 from a tuning word), +184 f32
// fov, +188 / +192 u32 (+192: the targeting component), +208 6 records of
// 80 bytes.
constexpr u32 kTargetVtable = 0x00B13DA0;
constexpr u32 kTCam = 4, kTPrio = 8, kTObj = 12, kTBone = 16, kTVec = 32, kTFrame = 48, kTPos = 96;
constexpr u32 kTRows = 64, kTFwd = 80, kTFlags = 182, kTComp = 192, kTRecs = 208, kTRecSize = 80, kTNumRecs = 6;
constexpr u32 kZeroVec = 0x01164B40, kIdentity = 0x01164BD0, kUpVec = 0x01164B60, kVec4C90 = 0x01164C90;
constexpr u32 kTune112 = 0x01035900 + 72, kTuneFlags = 0x01035B80 + 72;
constexpr u32 kTuneUp = 0x01035698 + 72, kTuneBack = 0x010356E8 + 72;
constexpr u32 kF25 = 0x41C80000, kF1 = 0x3F800000;
constexpr u32 kCamFov = 356, kCamMatrix = 96, kLookMatrix = 80, kObjPos = 144;
constexpr u32 kBoneFind0D14DC = 0x000D14DC;  // (Object*, u32 hash) -> s32 bone or -1 (name?)
constexpr u32 kBoneMatrixCE688 = 0x000CE688; // (f32 out[4][4], Object*, s32 bone, 1, 1) (name?)
constexpr u32 kBoneHash = 0x0021C5E0;
constexpr u32 kMatrixNormalize = 0x0056D374;  // Matrix_NormalizeInPlace (f32 m[4][4]) (symbols.txt)
// The targeting component template (on the stack): +0 vtable, +4 u32 name
// hash, +8 Object*, +12 u32 1, +16 u8, +20 6 limits of 24 bytes (then the
// first's +0 / +4 / +8 overridden), +164..+166 u8, +168 u32, +176 / +192
// vec3.
constexpr u32 kTmplVtBuild = 0x00B0DAAC, kTmplVtFinal = 0x00B13C74;
constexpr u32 kTmplSize = 208, kTmplLimits = 20, kLimitSize = 24;
constexpr u32 kStrBinocucom = 0x00AC6DF4;  // "CompTargeting_Binocucom"
// The template registry: an array {+0 u16 capacity, +2 u16 count, +4 u16
// high-water mark, +6 u16 flags (bit 0: unsorted), +8 {u32 hash, Tmpl*}[]}.
constexpr u32 kTmplRegistry = 0x01056F04;
constexpr u32 kArrayReserve = 0x00558F3C;     // Array_Reserve (Array*, u16 capacity) (symbols.txt)
constexpr u32 kSortedMapRemove = 0x007330C0;  // SortedMap_Remove7330C0 (map, const u32* key) (symbols.txt)
constexpr u32 kObjAttach = 0x000D135C;        // Obj_Attach0D135C (hevent17_a.cpp)

using hevent7::lfs_f64;

inline f32 sf(u32 ea) { return static_cast<f32>(lfs_f64(ea)); }
inline f64 ld(u32 ea) { return lfs_f64(ea); }
// fmsubs a*c - b.
inline f32 fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }
}  // namespace

// u32 Target_Construct10BD88(Target* t, Camera* cam, Object* o, Object*
// look) (name?): initialises a camera target on o (frame from the look
// object, else the camera; positioned on o's bone 0x0021C5E0 with an up /
// forward basis, or offset from o's position without one) and attaches a
// "CompTargeting_Binocucom" targeting component to o (registered in the
// template registry while it is built). Returns SortedMap_Remove's result.
DECOMP(0010BD88, Target_Construct10BD88) {
    const u32 t = arg<u32>(ctx, 0), cam = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2), look = arg<u32>(ctx, 3);
    store<u32>(t + kTCam, cam);
    store<u8>(t + kTPrio, 0);
    for (u32 k = 0; k < 12; k += 4) store<u32>(t + kTVec + k, load<u32>(kZeroVec + k));
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 12; c += 4) {
            store<u32>(t + kTFrame + 16 * r + c, load<u32>(kIdentity + 16 * r + c));
            if (r == 3 && c == 0) store<u32>(t + kTBone, 0xFFFFFFFFu);
            if (r == 3 && c == 4) store<u32>(t + kTObj, o);
        }
    store<u32>(t, kTargetVtable);
    store<u32>(t + 112, load<u32>(kTune112));
    store<u32>(t + 128, load<u32>(kZeroVec));
    store<u32>(t + 132, load<u32>(kZeroVec + 4));
    store<u32>(t + 148, 0);
    store<u32>(t + 136, load<u32>(kZeroVec + 8));
    store<u32>(t + 152, kF25);
    u32 fl = load<u16>(t + kTFlags);
    store<u32>(t + 160, load<u32>(kVec4C90));
    store<u32>(t + 144, 0);
    store<u32>(t + 164, load<u32>(kVec4C90 + 4));
    store<u32>(t + 168, load<u32>(kVec4C90 + 8));
    fl &= ~0xE000u;
    store<u16>(t + kTFlags, static_cast<u16>(fl));
    store<u32>(t + 172, load<u32>(kVec4C90 + 12));
    store<u32>(t + 176, 0);
    const u32 tf = load<u32>(kTuneFlags);
    store<u16>(t + 180, 0);
    store<u16>(t + kTFlags, static_cast<u16>((fl & ~0x1C00u) | ((tf & 7u) << 10)));
    store<u32>(t + 188, 0);
    store<u32>(t + 184, load<u32>(cam + kCamFov));
    store<u32>(t + kTComp, 0);
    for (u32 k = 0; k < kTNumRecs; ++k) {
        const u32 r = t + kTRecs + kTRecSize * k;
        store<u32>(r + 16, kF1);
        store<u32>(r + 20, 0xFFFFFFFFu);
        store<u32>(r + 64, 0);
        store<u32>(r + 32, 0xFFFFFFFFu);
        store<u32>(r + 68, 0);
    }
    const u32 obj = load<u32>(t + kTObj);
    const u32 src = look != 0 ? look + kLookMatrix : cam + kCamMatrix;
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 12; c += 4) store<u32>(t + kTFrame + 16 * r + c, load<u32>(src + 16 * r + c));
    const u32 bone = static_cast<u32>(call(ctx, kBoneFind0D14DC, {obj, kBoneHash}));
    store<u32>(t + kTBone, bone);
    Stack frame(ctx, 64 + kTmplSize);  // bone matrix (r1 +112), template (r1 +176)
    const u32 m = frame.addr(), s = frame.addr() + 64;
    const u32 row0 = t + kTFrame, up = t + kTRows, fwd = t + kTFwd, pos = t + kTPos;
    if (static_cast<s32>(bone) >= 0) {
        call(ctx, kBoneMatrixCE688, {m, load<u32>(t + kTObj), bone, 1, 1});
        for (u32 k = 0; k < 12; k += 4) store<u32>(pos + k, load<u32>(m + 48 + k));
        for (u32 k = 0; k < 12; k += 4) store<u32>(up + k, load<u32>(kUpVec + k));
        // row0 = up x fwd, then fwd = row0 x up (fmuls / fmsubs pairs).
        store<f32>(row0, fmsubs(ld(up + 4), ld(fwd + 8), hevent::fmuls(ld(up + 8), ld(fwd + 4))));
        store<f32>(row0 + 4, fmsubs(ld(up + 8), ld(fwd), hevent::fmuls(ld(up), ld(fwd + 8))));
        store<f32>(row0 + 8, fmsubs(ld(up), ld(fwd + 4), hevent::fmuls(ld(up + 4), ld(fwd))));
        store<f32>(fwd, fmsubs(ld(row0 + 4), ld(up + 8), hevent::fmuls(ld(row0 + 8), ld(up + 4))));
        store<f32>(fwd + 4, fmsubs(ld(row0 + 8), ld(up), hevent::fmuls(ld(row0), ld(up + 8))));
        const f32 z = fmsubs(ld(row0), ld(up + 4), hevent::fmuls(ld(row0 + 4), ld(up)));
        store<f32>(fwd + 8, z);
        ctx.fpr[1] = z;
        call(ctx, kMatrixNormalize, {row0});
    } else {
        const u32 p = load<u32>(t + kTObj) + kObjPos;
        f64 v[3];
        for (u32 k = 0; k < 3; ++k) {
            store<u32>(pos + 4 * k, load<u32>(p + 4 * k));
            v[k] = ld(p + 4 * k);
        }
        const f64 ku = ld(kTuneUp);
        for (u32 k = 0; k < 3; ++k) {
            v[k] = static_cast<f32>(std::fma(ld(up + 4 * k), ku, v[k]));  // fmadds
            store<f32>(pos + 4 * k, static_cast<f32>(v[k]));
        }
        const f64 kb = -ld(kTuneBack);
        for (u32 k = 0; k < 3; ++k) {
            v[k] = static_cast<f32>(std::fma(ld(fwd + 4 * k), kb, v[k]));
            store<f32>(pos + 4 * k, static_cast<f32>(v[k]));
        }
        ctx.fpr[1] = v[2];
    }
    // The component template, registered by name while it is attached.
    store<u32>(s, kTmplVtBuild);
    const u32 h = static_cast<u32>(call(ctx, fn::String_HashNoCase, {kStrBinocucom}));
    store<u32>(s + 8, 0);
    store<u8>(s + 16, 0);
    store<u32>(s + 4, h);
    store<u32>(s + 12, 1);
    const u32 arr = kTmplRegistry;
    u32 n = load<u16>(arr + 2);
    const u32 cap = load<u16>(arr);
    if (n == cap) {
        call(ctx, kArrayReserve, {arr, (cap << 1) & 0xFFFEu});
        n = load<u16>(arr + 2);
    }
    const u32 data = load<u32>(arr + 8);
    store<u32>(data + ((n << 3) & 0x7FFF8u), load<u32>(s + 4));
    store<u32>(data + (static_cast<u32>(load<u16>(arr + 2)) << 3) + 4, s);
    const u32 n2 = load<u16>(arr + 2) + 1;
    const u32 hi = load<u16>(arr + 4), af = load<u16>(arr + 6);
    store<u16>(arr + 2, static_cast<u16>(n2));
    store<u16>(arr + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n2 & 0xFFFF) ? hi : n2));
    store<u16>(arr + 6, static_cast<u16>(af | 1));
    store<u32>(s, kTmplVtFinal);
    for (u32 k = 0; k < 6; ++k) {
        const u32 l = s + kTmplLimits + kLimitSize * k;
        store<u32>(l, 0);
        store<u32>(l + 4, 0);
        store<u32>(l + 8, 0x42B40000);   // 90.0
        store<u32>(l + 12, 0xFF7FFFFF);  // -FLT_MAX
        store<u32>(l + 16, 0x7F7FFFFF);  // FLT_MAX
        store<u32>(l + 20, 0x3F000000);  // 0.5
    }
    for (u32 k = 0; k < 12; k += 4) store<u32>(s + 176 + k, load<u32>(kZeroVec + k));
    store<u8>(s + 164, 0);
    store<u8>(s + 166, 0);
    store<u8>(s + 165, 0);
    store<u32>(s + 168, 0);
    for (u32 k = 0; k < 12; k += 4) store<u32>(s + 192 + k, load<u32>(kZeroVec + k));
    store<u8>(s + 166, 1);
    store<u32>(s + 8, o);
    store<u32>(s + 28, 0x43340000);  // 180.0
    store<u32>(s + 20, 32);
    store<u32>(s + 24, 0x43480000);  // 200.0
    ctx.fpr[1] = ld(kZeroVec + 8);
    const u32 c = static_cast<u32>(call(ctx, kObjAttach, {o, s, 1}));
    store<u32>(t + kTComp, c);
    store<u32>(s, kTmplVtBuild);
    call(ctx, kSortedMapRemove, {arr, s + 4});
}
