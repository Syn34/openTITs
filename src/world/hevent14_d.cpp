// World_HandleEvent callees, part 14d: Player_Respawn12C250, putting the
// player back at the current entry's point (grind coach check, physics
// reset, entry transform, the camera blends).
#include "hevent14.h"

using namespace sly4;
using namespace sly4::hevent13;
using namespace sly4::hevent14;

namespace {
// Player object: vt +16 (owner) reset, vt +28 set slot (o, u32 k, u8 v), vt
// +32 state (o, 0), vt +64 body, vt +84 set transform (o, Mat*); +20 spline
// mover (+48 spline, +52, +56 f32, +84 u8 mode), +172 s8 user, +184 u64
// flags (+188 bit 5, bit 23 cleared at the end), +436 f32, +576 Mat (80
// bytes, cleared first), +640 / +641 u8 slot values, +644 spline, +648
// f32, +656 u8 per-move flags.
constexpr u32 kOwnVtReset = 16;
constexpr u32 kPlVtSetSlot = 28, kPlVtState = 32, kPlVtBody = 64, kPlVtSetTransform = 84;
constexpr u32 kPlMover = 20, kPlUser = 172, kPlFlags64 = 184, kPlF436 = 436;
constexpr u32 kPlMat = 576, kPlMatClear = 80, kPlSlot0 = 640, kPlSlot1 = 641;
constexpr u32 kPlSpline = 644, kPlSplineT = 648, kPlMoveFlags = 656;
constexpr u8 kSlotGrind = 3;
constexpr u64 kPlFlagBit23 = 1ull << 23;  // rldimi 23,40: word +188 bit 23
constexpr u32 kMvSpline = 48, kMvKnot = 52, kMvT = 56, kMvMode = 84;
constexpr u32 kStateDead = 5;

// Entry (World current, 0x00F64C20 +72): vt +76 point hash for a user;
// +2328 per-user anchor objects, +2364 per-user f32 height offsets, +2488 /
// +2500 per-user bit sets (use point / release).
constexpr u32 kSessionPtr = 0x00F64C20, kSessionEntry = 72;
constexpr u32 kEnVtPoint = 76;
constexpr u32 kEnAnchors = 2328, kEnHeights = 2364, kEnUsePointBits = 2488, kEnReleaseBits = 2500;
constexpr u32 kAnchorMat = 96;  // 0x000D6A28's result +96: the Mat

// World +0x1281E0: the camera (+345 u8 count, +364 Target*[], +596 f32).
constexpr u32 kWCamera = 0x1281E0, kCamCount = 345, kCamTargets = 364, kCamBlendTime = 596;
constexpr u32 kTgtVtKind = 8, kTgtKindFixed = 11;
constexpr u32 kCamBlend01 = 0x3DCCCCCD;  // 0.1f

constexpr u32 kF436Src = 0x01038C80;       // f32 copied to +436
constexpr u32 kMinusOne = 0x0012C248;      // f32 -1.0
constexpr u32 kZero = 0x0012C24C;          // f32 0.0
constexpr u32 kMoveFlagIndex = 0x00ACAB74; // s32[] index of +656 flags by +641
constexpr u8 kMoveAllowed = 0x80, kMoveBlocked = 0x40;
constexpr u32 kUbFlags = 0, kUbFlagRespawned = 0x40, kUb108 = 108;

// Callees.
constexpr u32 kObjOwner0D93FC = 0x000D93FC;      // Obj_Owner0D93FC
constexpr u32 kSetPhysicsActive = 0x00260C14;
constexpr u32 kBodyReset261328 = 0x00261328;  // (body, u32 mode) (name?)
constexpr u32 kAnchor_Get0D6A28 = 0x000D6A28; // (anchor) -> object with a Mat at +96, or 0 (name?)
constexpr u32 kSaveProfile = 0x000C9FC8;      // Save_Profile0C9FC8
constexpr u32 kPlayer_SetSave124D58 = 0x00124D58;  // (o, profile, 0) (name?)
constexpr u32 kSpline_Knot4A22A4 = 0x004A22A4;     // (spline) -> u32 (name?)
constexpr u32 kSplineGetMode = 0x005A8BA8;
constexpr u32 kBlendPose = 0x000FA660;        // Obj_BlendPose0FA660 (cam; f32)
constexpr u32 kCam_Next0FCBD0 = 0x000FCBD0;   // (cam, target) -> next target (name?)
constexpr u32 kCam_Blend0FBB88 = 0x000FBB88;  // (cam, target; f32) (name?)
constexpr u32 kTarget_Reset0FEACC = 0x000FEACC;  // (target) (name?)

// The vt +28 call: the original leaves the entry address in r6 and the OPD
// in r7 (the third argument is loaded between them).
void set_slot(PPUContext& ctx, u32 o, u32 k, u32 v) {
    const u32 opd = load<u32>(load<u32>(o) + kPlVtSetSlot);
    ctx.gpr[6] = load<u32>(opd);
    ctx.gpr[7] = opd;
    call_ptr(ctx, opd, {o, k, v});
}
u32 body(PPUContext& ctx, u32 o) { return static_cast<u32>(hevent::vcall(ctx, o, kPlVtBody, {o})); }
// stvx of zero at +576 .. +655 (16-byte aligned, as stvx).
void clear_mat(u32 o) {
    const u32 base = (o + kPlMat) & ~15u;
    for (u32 k = 0; k < kPlMatClear; k += 4) store<u32>(base + k, 0);
}
}  // namespace

// u32 Player_Respawn12C250(Object* o) (name?): resets the owner (vt +16),
// sets +436, and (grinding without a blocked move) puts the save profile
// back and flags the user block. Restarts the body's physics, takes the
// transform from the entry's anchor (plus height offset) or named point for
// this user, applies it and the two slots, releases the player (entry bit)
// or restores its grind spline, then blends the camera's targets. Clears
// flag bit 23. Returns 0.
DECOMP(0012C250, Player_Respawn12C250) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, kObjOwner0D93FC, {o})) != 0) {
        const u32 own = static_cast<u32>(call(ctx, kObjOwner0D93FC, {o}));
        hevent::vcall(ctx, own, kOwnVtReset, {own});
    }
    store<u32>(o + kPlF436, load<u32>(kF436Src));
    ctx.fpr[1] = hevent7::lfs_f64(kF436Src);

    if ((load<u32>(o + kObjFlags188) & kObjFlag188Bit5) != 0 && load<u8>(o + kPlSlot0) == kSlotGrind) {
        const s32 fi = static_cast<s32>(load<u32>(kMoveFlagIndex + load<u8>(o + kPlSlot1) * 4u));
        const u8 mf = load<u8>(o + kPlMoveFlags + static_cast<u32>(fi));
        if (!((mf & kMoveAllowed) != 0 && (mf & kMoveBlocked) == 0) &&
            static_cast<u32>(hevent::vcall(ctx, o, kPlVtState, {o, 0})) != kStateDead &&
            (load<u32>(o + kObjFlags188) & kObjFlag188Bit5) != 0) {
            const u32 prof = static_cast<u32>(call(ctx, kSaveProfile, {kAnyPoint}));
            call(ctx, kPlayer_SetSave124D58, {o, prof, 0});
            const s32 user = static_cast<s8>(load<u8>(o + kPlUser));
            const u32 ub = load<u32>(hevent::kCurrentWorld) + static_cast<u32>(user) * hevent::kUserBlockSize + hevent::kUserBlocks;
            const u32 f = load<u32>(ub + kUbFlags);
            store<u32>(ub + kUb108, 0);
            store<u32>(ub + kUbFlags, f | kUbFlagRespawned);
        }
    }

    if (body(ctx, o) != 0) {
        call(ctx, kSetPhysicsActive, {body(ctx, o), 0});
        call(ctx, kSetPhysicsActive, {body(ctx, o), 1});
    }

    const u32 e = load<u32>(load<u32>(kSessionPtr) + kSessionEntry);
    const s32 user = static_cast<s8>(load<u8>(o + kPlUser));
    const u32 ub = load<u32>(hevent::kCurrentWorld) + static_cast<u32>(user) * hevent::kUserBlockSize + hevent::kUserBlocks;
    constexpr u32 kUbIndex = 2, kUbPlayer = 8;
    bool release = false;
    if (e != 0) {
        s32 idx = static_cast<s32>(call(ctx, h14fn::Entry_Index325EBC, {e, load<u32>(ub + kUbPlayer)}));
        if (idx == -1) idx = load<u8>(ub + kUbIndex);
        const u32 ui = static_cast<u32>(idx);
        const u32 byte = ui >> 3, bit = 1u << (ui & 7);
        const u32 anchor = load<u32>(e + kEnAnchors + ui * 4);
        if (anchor != 0) {
            const u32 a = static_cast<u32>(call(ctx, kAnchor_Get0D6A28, {anchor}));
            if (a != 0) {
                clear_mat(o);
                copy_mat(ctx, o + kPlMat, a + kAnchorMat);
                // +628 (the y of the position) += the user's height offset (fadds).
                const f32 y = static_cast<f32>(hevent7::lfs_f64(o + kPlMat + 52) + hevent7::lfs_f64(e + kEnHeights + ui * 4));
                store<f32>(o + kPlMat + 52, y);
                ctx.fpr[1] = y;
                if ((load<u8>(load<u32>(e + kEnReleaseBits) + byte) & bit) != 0) release = true;
            }
        } else if ((load<u8>(load<u32>(e + kEnUsePointBits) + byte) & bit) != 0) {
            const u32 h = static_cast<u32>(hevent::vcall(ctx, e, kEnVtPoint, {e, ui}));
            if (h != 0) {
                Stack frame(ctx, 16);  // r1+112: the key
                const u32 pt = find_point(ctx, frame.addr(), h);
                if (pt != 0) {
                    clear_mat(o);
                    copy_mat(ctx, o + kPlMat, pt + kPointMatrix);
                    if ((load<u8>(load<u32>(e + kEnReleaseBits) + byte) & bit) != 0) release = true;
                }
            }
        }
    }

    call(ctx, kBodyReset261328, {body(ctx, o), 1});
    hevent::vcall(ctx, o, kPlVtSetTransform, {o, o + kPlMat});
    set_slot(ctx, o, 0, load<u8>(o + kPlSlot0));
    set_slot(ctx, o, 1, load<u8>(o + kPlSlot1));
    if (release) {
        call(ctx, h14fn::Obj_Release12BF70, {o});
    } else if (load<u8>(o + kPlSlot0) == kSlotGrind) {
        const u32 mv = load<u32>(o + kPlMover);
        const u32 spline = load<u32>(o + kPlSpline);
        store<u32>(mv + kMvSpline, spline);
        store<u32>(mv + kMvT, load<u32>(o + kPlSplineT));  // lfs / stfs pair
        ctx.fpr[1] = hevent7::lfs_f64(o + kPlSplineT);
        store<u32>(mv + kMvKnot, static_cast<u32>(call(ctx, kSpline_Knot4A22A4, {spline})));
        store<u8>(mv + kMvMode, static_cast<u8>(call(ctx, kSplineGetMode, {load<u32>(mv + kMvSpline)})));
        call(ctx, kBodyReset261328, {body(ctx, o), 3});
    }

    const u32 cam = load<u32>(load<u32>(hevent::kCurrentWorld) + kWCamera);
    if (cam != 0) {
        callf(ctx, kBlendPose, {cam}, {hevent7::lfs_f64(kMinusOne)});
        const u8 n = load<u8>(cam + kCamCount);
        store<u32>(cam + kCamBlendTime, kCamBlend01);
        u32 t = n != 0 ? load<u32>(cam + kCamTargets + n * 4u) : 0;
        if (t != 0) {
            const f64 zero = hevent7::lfs_f64(kZero);
            do {
                const u32 next = static_cast<u32>(call(ctx, kCam_Next0FCBD0, {cam, t}));
                if (static_cast<u32>(hevent::vcall(ctx, t, kTgtVtKind, {t})) != kTgtKindFixed)
                    callf(ctx, kCam_Blend0FBB88, {cam, t}, {zero});
                else
                    call(ctx, kTarget_Reset0FEACC, {t});
                t = next;
            } while (t != 0);
        }
    }
    store<u64>(o + kPlFlags64, load<u64>(o + kPlFlags64) & ~kPlFlagBit23);
    ret(ctx, 0u);
}
