// World_HandleEvent callees, part 19c: the animator's bone-constraint
// records built by Obj_Reset293264 (the 224-byte look-at records and the
// 320-byte scale records).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
using hevent7::lfs_f64;

constexpr u32 kRig = 8, kRigSkeleton = 28;
constexpr u32 kZeroVec = 0x01164B40;   // f32[3] {0, 0, 0}
constexpr u32 kZeroVec2 = 0x01164B60;  // f32[3] (as kZeroVec, another copy)
constexpr u32 kVecZero = 0x00B2C1D0;   // v128 {0, 0, 0, 0}
constexpr u32 kVecW1 = 0x00B2C390;     // v128 {0, 0, 0, 1.0}
constexpr u32 kOneF = 0x3F800000u, kMinusOneF = 0xBF800000u;

// Copies a 16-byte vector constant (lvx / stvx).
inline void copy16(u32 dst, u32 src) {
    for (u32 k = 0; k < 16; k += 4) store<u32>(dst + k, load<u32>(src + k));
}
// Stores (x, y, z, 0) of the singles at src (the vsel / vandc sequence).
inline void store_xyz0(u32 dst, u32 src) {
    store<u32>(dst + 0, load<u32>(src));
    store<u32>(dst + 4, load<u32>(src + 4));
    store<u32>(dst + 8, load<u32>(src + 8));
    store<u32>(dst + 12, 0);
}
inline void stfs(u32 ea, f64 v) { store<f32>(ea, static_cast<f32>(v)); }

// Look-at records: Animator +96 PtrVector* (20 bytes, grow step 224),
// pushed with 0x0076E400 (vec, const Rec*). Rec (224 bytes):
//   +0 u32 name hash, +16 s16 bone[3] (+22, +24 stay -1), +32 f32 axis[4]
//   (x, y, z, 0), +48 f32 c, b, a, a, +64 f32 a, +80 / +112 v128 0,
//   +96 / +128 / +176 v128 {0, 0, 0, 1}, +144 u8 1, +145 u8 (mode == 1),
//   +160 u32 bone 3 lookup, +192 u32 extra, +196 u32 mode, +200 / +204 f32
//   a, +208 f32[3] zero. Bytes the original never writes keep the stack's
//   contents.
constexpr u32 kLookAts = 96, kLookAtVecSize = 20, kLookAtStep = 224, kLookAtSize = 224;
constexpr u32 kVectorPush76E400 = 0x0076E400;  // (vec, const Rec*) (name?)

// Scale records: Animator +108 PtrVector* (20 bytes, grow step 320),
// pushed with 0x0076E704. Rec (320 bytes): +0 u32 name hash, +16 object A
// (0x00A3D4C0; +16 Skeleton*, +20 s16 bone[3]), +32 / +48 f32 (x, y, z, 0)
// of v0 / v1, +64 / +80 zero, +100 / +104 / +108 / +112 / +124 / +128
// f32 f1, f2, f4, f3, f8, f7, +144 u32 copy of A (240 bytes, 0 when the
// allocation failed), +160 object B (0x00A3D484), +256 u8 0, +260 u32
// 0x0106BF9C, +304 u32 root bone, +308 u32 extra.
constexpr u32 kScales = 108, kScaleVecSize = 20, kScaleStep = 320, kScaleSize = 320;
constexpr u32 kVectorPush76E704 = 0x0076E704;  // (vec, const Rec*) (name?)
constexpr u32 kObjAInit = 0x00A3D4C0, kObjBInit = 0x00A3D484, kObjACopy = 0x00A3D570;  // (name?)
constexpr u32 kRootName = 0x00ADC02C, kRootName2 = 0x00ADC0A8;  // "Root", "c:b:sk:Root"
constexpr u32 kScaleTag = 0x0106BF9C;
// The allocator: *(r13 - 28332) +44, vt +8 alloc(size).
constexpr s32 kTlsAllocHolder = -28332;
constexpr u32 kHolderAllocator = 44, kAllocVt = 8, kCopySize = 240;
// Animator +4: the owner, vt +12 called when the root bone is missing.
constexpr u32 kAnimOwner4 = 4, kOwnerVtFail = 12;

// Creates the record vector at a + off on first use.
inline u32 record_vector(PPUContext& ctx, u32 a, u32 off, u32 step) {
    u32 v = load<u32>(a + off);
    if (v != 0) return v;
    v = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {20}));
    store<u32>(a + off, v);
    if (v == 0) return 0;
    call(ctx, hevent9::h9fn::PtrVector_Construct, {v});
    store<u16>(v + hevent18::kPvStep, static_cast<u16>(step));
    return load<u32>(a + off);
}
}  // namespace

// void Anim_A351B60(Animator* a, const char* name, s32 mode, const char*
// bone0, bone1, bone2, bone3, const f32 axis[3]; f32 a, b, c; u32 extra
// (11th argument)) (name?): appends a look-at record when bones 0..2 are
// found (and bone 3 too in mode 1).
DECOMP(00351B60, Anim_A351B60) {
    const u32 a = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), mode = arg<u32>(ctx, 2);
    const u32 bn[4] = {arg<u32>(ctx, 3), arg<u32>(ctx, 4), arg<u32>(ctx, 5), arg<u32>(ctx, 6)};
    const u32 axis = arg<u32>(ctx, 7);
    const u32 extra = load<u32>(static_cast<u32>(ctx.gpr[1]) + 140);  // 11th argument
    const f64 fa = farg(ctx, 0), fb = farg(ctx, 1), fc = farg(ctx, 2);
    const u32 rig = load<u32>(a + kRig);
    if (rig == 0) return ret(ctx, name);
    const u32 skel = load<u32>(rig + kRigSkeleton);
    if (skel == 0) return ret(ctx, name);
    Stack frame(ctx, kLookAtSize);  // r1 +112 .. +335
    const u32 r = frame.addr();
    store<u32>(r + 48, kMinusOneF);
    copy16(r + 32, kVecZero);
    store<u32>(r + 52, kOneF);
    store<u32>(r + 0, 0);
    store<u32>(r + 56, kOneF);
    store<u32>(r + 60, kOneF);
    for (u32 k = 0; k < 5; ++k) store<u16>(r + 16 + 2 * k, 0xFFFF);
    store<u32>(r + 64, kOneF);
    copy16(r + 80, kVecZero);
    copy16(r + 96, kVecW1);
    copy16(r + 112, kVecZero);
    copy16(r + 128, kVecW1);
    store<u8>(r + 144, 1);
    store<u8>(r + 145, 0);
    store<u32>(r + 160, 0xFFFFFFFFu);
    store<u32>(r + 176, 0); store<u32>(r + 180, 0); store<u32>(r + 184, 0); store<u32>(r + 188, kOneF);
    store<u32>(r + 192, 0);
    store<u32>(r + 196, 0);
    store<u32>(r + 200, kOneF);
    store<u32>(r + 204, kOneF);
    for (u32 k = 0; k < 12; k += 4) store<u32>(r + 208 + k, load<u32>(kZeroVec + k));
    ctx.fpr[1] = lfs_f64(kZeroVec + 8);  // f1 / f2 as the copy leaves them
    ctx.fpr[2] = lfs_f64(kZeroVec + 4);
    const u32 hash = static_cast<u32>(call(ctx, hevent::hfn::String_HashNoCase, {name}));
    stfs(r + 200, fa);
    stfs(r + 204, fa);
    copy16(r + 176, kVecW1);
    store_xyz0(r + 32, axis);
    store<u32>(r + 0, hash);
    store<u32>(r + 196, mode);
    for (u32 k = 0; k < 3; ++k) store<u16>(r + 16 + 2 * k, static_cast<u16>(call(ctx, h19fn::Rig_FindBoneA59468, {skel, bn[k], 0})));
    stfs(r + 56, fa);
    stfs(r + 60, fa);
    stfs(r + 64, fa);
    stfs(r + 52, fb);
    stfs(r + 48, fc);
    store<u8>(r + 145, mode == 1 ? 1 : 0);
    const u64 b3 = call(ctx, h19fn::Rig_FindBoneA59468, {skel, bn[3], 0});
    store<u32>(r + 192, extra);
    store<u32>(r + 160, static_cast<u32>(b3));
    for (u32 k = 0; k < 3; ++k)
        if (static_cast<s16>(load<u16>(r + 16 + 2 * k)) == -1) return;  // r3 = bone 3's lookup
    if (load<u32>(r + 196) == 1 && static_cast<s32>(b3) == -1) return;
    const u32 v = record_vector(ctx, a, kLookAts, kLookAtStep);
    call(ctx, kVectorPush76E400, {v, r});
}

// void Anim_C352190(Animator* a, const char* name, const char* bone0,
// bone1, bone2, const f32 v0[3], const f32 v1[3]; f32 x1..x4, -, -, x7,
// x8; u32 extra (15th argument)) (name?): appends a scale record for the
// three bones under the skeleton's root ("Root", else "c:b:sk:Root"); a
// missing root calls the owner's vt +12, a missing bone does the same.
DECOMP(00352190, Anim_C352190) {
    const u32 a = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    const u32 bn[3] = {arg<u32>(ctx, 2), arg<u32>(ctx, 3), arg<u32>(ctx, 4)};
    const u32 v0 = arg<u32>(ctx, 5), v1 = arg<u32>(ctx, 6);
    const u32 extra = load<u32>(static_cast<u32>(ctx.gpr[1]) + 172);  // 15th argument
    const f64 f1 = farg(ctx, 0), f2 = farg(ctx, 1), f3 = farg(ctx, 2), f4 = farg(ctx, 3), f7 = farg(ctx, 6), f8 = farg(ctx, 7);
    const u32 rig = load<u32>(a + kRig);
    if (rig == 0) return;  // r3 = a
    const u32 skel = load<u32>(rig + kRigSkeleton);
    if (skel == 0) return;
    Stack frame(ctx, kScaleSize);  // r1 +112 .. +431
    const u32 r = frame.addr();
    store<u32>(r + 0, 0);
    call(ctx, kObjAInit, {r + 16});
    store<u32>(r + 144, 0);
    call(ctx, kObjBInit, {r + 160});
    store<u32>(r + 260, kScaleTag);
    const u32 hash = static_cast<u32>(call(ctx, hevent::hfn::String_HashNoCase, {name}));
    store<u32>(r + 0, hash);
    store<u32>(r + 16, skel);
    auto fail = [&] {
        const u32 owner = load<u32>(a + kAnimOwner4);
        hevent::vcall(ctx, owner, kOwnerVtFail, {owner});
    };
    u64 root = call(ctx, h19fn::Rig_FindBoneA59468, {skel, kRootName, 0});
    store<u32>(r + 304, static_cast<u32>(root));
    if (static_cast<s32>(root) == -1) {
        root = call(ctx, h19fn::Rig_FindBoneA59468, {skel, kRootName2, 0});
        store<u32>(r + 304, static_cast<u32>(root));
        if (static_cast<s32>(root) == -1) return fail();
    }
    for (u32 k = 0; k < 3; ++k) {
        const u64 b = call(ctx, h19fn::Rig_FindBoneA59468, {skel, bn[k], 0});
        store<u16>(r + 20 + 2 * k, static_cast<u16>(b));
        if (static_cast<s16>(static_cast<u16>(b)) == -1) return fail();
    }
    store_xyz0(r + 32, v0);
    store_xyz0(r + 48, v1);
    stfs(r + 100, f1);
    stfs(r + 104, f2);
    stfs(r + 112, f3);
    stfs(r + 108, f4);
    stfs(r + 124, f8);
    stfs(r + 128, f7);
    store_xyz0(r + 64, kZeroVec2);
    store_xyz0(r + 80, kZeroVec2);
    store<u8>(r + 256, 0);
    const u32 holder = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + kTlsAllocHolder) + kHolderAllocator);
    const u32 copy = static_cast<u32>(hevent::vcall(ctx, holder, kAllocVt, {holder, kCopySize}));
    store<u16>(copy + 4, kCopySize);  // written even when the allocation failed
    if (copy != 0) call(ctx, kObjACopy, {copy, r + 16});
    store<u32>(r + 308, extra);
    store<u32>(r + 144, copy);
    const u32 v = record_vector(ctx, a, kScales, kScaleStep);
    call(ctx, kVectorPush76E704, {v, r});
}
