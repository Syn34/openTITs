// World_HandleEvent callees, part 18a: small leaves under the hevent17
// functions (player component, flag lookup, main object pause, bone lookup,
// object colour, eased table lookup).
#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
constexpr u32 kCompPlayer = 17;  // component type looked up by Player_Get0D9474

// Object +152: pause target (vt +20 pause(t, u8)).
constexpr u32 kObjPauseTarget = 152, kPauseVt = 20;

// Object +160 WorldObj (vt +28 skeleton getter).
constexpr u32 kWobjVtSkeleton = 28;

// Object +4 u32: bits 10..15 kind (8: coloured mesh); +208 Mesh* (+112 u8
// part count, 8-byte parts from +132 {Part*}); Part +92 Material* (+8 u8
// rgba[4], +12 f32 intensity).
constexpr u32 kObjKindWord = 4, kObjKindShift = 10, kObjKindMask = 0x3F, kKindColoured = 8;
constexpr u32 kObjMesh = 208, kMeshPartCount = 112, kMeshParts = 132, kMeshPartSize = 8;
constexpr u32 kPartMaterial = 92, kMatRgba = 8, kMatIntensity = 12;

// Ease4A8320 constants (f32[5] at 0x004A830C: x range lo, hi; t scale,
// bias; x scale) and its f32 table (rows of 6) at 0x00AEB6D4.
constexpr u32 kEaseConsts = 0x004A830C, kEaseTable = 0x00AEB6D4;
}  // namespace

// Object* Player_Get0D9474(Player* p) (name?): the object's component 17.
DECOMP(000D9474, Player_Get0D9474) {
    ret(ctx, call(ctx, hevent12::h12fn::Obj_FindComponent, {arg<u32>(ctx, 0), kCompPlayer, 0}));
}

// bool Flag_Get48FC68(u32 hash) (name?): the value in the flag map at
// 0x010BB738 is non-zero (the slot is never null-checked).
DECOMP(0048FC68, Flag_Get48FC68) {
    Stack frame(ctx, 16);  // the key word (r1 +112 in the original frame)
    store<u32>(frame.addr(), arg<u32>(ctx, 0));
    const u32 slot = static_cast<u32>(call(ctx, h18fn::SortedMap_Find786C0C, {kFlagMap, frame.addr()}));
    ret(ctx, load<u32>(slot) != 0 ? 1u : 0u);
}

// void Main_Pause426AD8(Object* o, u8 paused) (name?): forwards to the
// object's pause target (+152), when it has one. Returns r3 = 0 without one.
DECOMP(00426AD8, Main_Pause426AD8) {
    const u32 t = load<u32>(arg<u32>(ctx, 0) + kObjPauseTarget);
    const u32 paused = arg<u8>(ctx, 1);
    if (t == 0) {
        ctx.gpr[4] = paused;
        ret(ctx, 0u);
        return;
    }
    vcall(ctx, t, kPauseVt, {t, paused});
}

// s32 Obj_FindBone0D14DC(Object* o, u32 hash) (name?): the bone index of
// hash in the WorldObj's skeleton, or -1 (hash 0, no WorldObj or skeleton).
DECOMP(000D14DC, Obj_FindBone0D14DC) {
    const u32 o = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1);
    if (hash == 0) {
        ret(ctx, -1);
        return;
    }
    const u32 wobj = load<u32>(o + hevent10::kObjWorldObj);
    if (wobj != 0) {
        const u32 skel = static_cast<u32>(vcall(ctx, wobj, kWobjVtSkeleton, {wobj}));
        if (skel != 0) {
            ret(ctx, call(ctx, hevent9::h9fn::Skeleton_FindBone, {skel, hash}));
            return;
        }
    }
    ret(ctx, -1);
}

// void Obj_SetColourD6F90(Object* o, const u8 rgba[4]; f32 intensity)
// (name?): sets the colour and intensity of every material of a kind-8
// object's mesh parts.
DECOMP(000D6F90, Obj_SetColourD6F90) {
    // r3 is left as the original leaves it (o, the mesh, or past its parts).
    const u32 o = arg<u32>(ctx, 0), rgba = arg<u32>(ctx, 1);
    if (o == 0) return;
    if (((load<u32>(o + kObjKindWord) >> kObjKindShift) & kObjKindMask) != kKindColoured) return;
    const u32 mesh = load<u32>(o + kObjMesh);
    const u32 n = load<u8>(mesh + kMeshPartCount);
    ctx.gpr[3] = mesh + (n != 0 ? kMeshPartSize * n : 0);
    for (u32 i = 0; i < n; ++i) {
        const u32 part = load<u32>(mesh + kMeshPartSize * i + kMeshParts);
        if (part == 0) continue;
        const u32 mat = load<u32>(part + kPartMaterial);
        if (mat == 0) continue;
        for (u32 k = 0; k < 4; ++k) store<u8>(mat + kMatRgba + k, load<u8>(rgba + k));
        store<f32>(mat + kMatIntensity, static_cast<f32>(farg(ctx, 0)));
    }
}

// f32 Ease4A8320(f32 x, f32 t) (name?): x outside [lo, hi] is returned as
// it is; otherwise the table entry at row (s32)(x * scale), column
// clamp((s32)(t * a + b), 1, ..) - 1 (capped at 5 once it reaches 6). The
// row is not range-checked.
DECOMP(004A8320, Ease4A8320) {
    const f64 x = farg(ctx, 0), t = farg(ctx, 1);
    ctx.gpr[3] = kEaseConsts;  // r3 left as the original leaves it
    if (x < hevent7::lfs_f64(kEaseConsts) || x > hevent7::lfs_f64(kEaseConsts + 4)) return;  // f1 = x
    const f32 ct = static_cast<f32>(std::fma(t, hevent7::lfs_f64(kEaseConsts + 8), hevent7::lfs_f64(kEaseConsts + 12)));
    s32 col = static_cast<s32>(static_cast<u32>(rt::fctiw(ct, true)));
    if (col < 1) col = 1;
    col -= 1;
    if (col >= 6) col = 5;
    const f32 rx = fmuls(x, hevent7::lfs_f64(kEaseConsts + 16));
    const s32 row = static_cast<s32>(static_cast<u32>(rt::fctiw(rx, true)));
    const u32 idx = static_cast<u32>(row) * 6u + static_cast<u32>(col);
    ctx.gpr[3] = kEaseTable;
    ctx.fpr[1] = hevent7::lfs_f64(kEaseTable + idx * 4u);
}
