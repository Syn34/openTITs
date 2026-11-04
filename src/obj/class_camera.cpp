// ApeCameraInstance: the camera objects (kind 11, vtable 0x00B24948). The
// slots it shares with ApeGeomInstance are in obj2_vt.cpp / obj3.cpp /
// obj4.cpp.
#include "classes.h"

using namespace sly4;
using namespace sly4::cls;
using namespace sly4::obj4;

namespace {

constexpr u32 kCameraInstance_New = 0x005A59AC;  // (ApeCameraDef*) -> instance (name?)
constexpr u32 kVtblCameraInstance = 0x00B24948;
constexpr u32 kCameraDestroyOpd = 0x00F3C978;
constexpr u32 kCameraFov = 340;        // f32 (def +176) (name?)
constexpr u32 kCameraDefMatrix = 112;  // def +112: f32[4][4] local transform rows

}  // namespace

// void CameraInstance_Destruct(CameraInstance* o)  (slot +0)
DECOMP(007A8D14, CameraInstance_Destruct) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, kVtblCameraInstance);
    call(ctx, ifn::WorldObj_DestructBase, {o});
}

// void CameraInstance_DeletingDestruct(CameraInstance* o)  (slot +4)
// The destructor is inlined here.
DECOMP(007A9EE4, CameraInstance_DeletingDestruct) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, kVtblCameraInstance);
    call(ctx, ifn::WorldObj_DestructBase, {o});
    call(ctx, ifn::Mem_FreeFrom, {o, 0});
}

// void CameraInstance_Release(CameraInstance* o)  (slot +8)
// As WorldObj_Release, with the camera's deferred destructor (0x00F3C978).
DECOMP(005A58C8, CameraInstance_Release) {
    const u32 o = arg<u32>(ctx, 0);
    const u16 refs = static_cast<u16>(load<u16>(o + kObjRefs) - 1);
    store<u16>(o + kObjRefs, refs);
    if (static_cast<s16>(refs) > 0) return;
    call(ctx, ifn::WorldObj_Unlink, {o});
    RemoveChildren(ctx, o);
    call(ctx, ifn::WorldObj_ReleaseRes, {o});
    call(ctx, ifn::DeferredDestroy, {o, kCameraDestroyOpd});
}

// void CameraInstance_ShowAndNotify(CameraInstance* o, bool)  (slot +16)
// Cameras ignore it.
DECOMP(007A9EE0, CameraInstance_ShowAndNotify) {}

// void CameraInstance_Changed(CameraInstance* o)  (slot +36)
// Only the peer copy: an object with a peer record copies its +240 vec4 to
// the record's vec4. Leaves r3 = &o->+240, r4 = the destination and f1 as
// the original does.
DECOMP(007A9964, CameraInstance_Changed) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 idx = load<u16>(o + obj::kObjFlags);
    if (idx == 0xFFFF) return;
    const u32 peers = load<u32>(load<u32>(obj3::kSlotPeers));
    const u32 base = load<u32>(peers);
    const s32 i = static_cast<s32>(base + idx * kPeerSize - base) / static_cast<s32>(kPeerSize);
    const u32 dst = load<u32>(peers + 4) + (static_cast<u32>(i) << 5);
    CopyVec4(dst, o + kObjAnchor);
    ctx.fpr[1] = static_cast<f64>(load<f32>(o + kObjAnchor + 12));
    ctx.gpr[4] = dst;
    ret(ctx, o + kObjAnchor);
}

// void CameraInstance_SetClass(CameraInstance* o, ApeCameraDef* def)  (slot +44)
// The base swap; with a definition, its field of view (+176) and local
// transform (+112, four rows of three) are copied and the object marked
// dirty.
DECOMP(005A5C28, CameraInstance_SetClass) {
    const u32 o = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1);
    if (def == load<u32>(o + kObjClassPtr)) return;
    call(ctx, ifn::ApeObj_SetClass, {o, def});
    const u32 cls = load<u32>(o + kObjClassPtr);
    if (cls == 0) return;
    store<u32>(o + kCameraFov, load<u32>(cls + 176));
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 12; c += 4)
            store<u32>(o + obj::kObjMatrix + 16 * r + c, load<u32>(cls + kCameraDefMatrix + 16 * r + c));
    MarkDirtyInline(ctx, o);
}

// CameraInstance* CameraInstance_Clone(const CameraInstance* o, u32 flags)  (slot +56)
// A new camera of o's definition with o's state; children cloned with
// kCloneChildren. Cameras are never added to the world here.
DECOMP(005A5B28, CameraInstance_Clone) {
    const u32 o = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 1);
    const u32 n = static_cast<u32>(call(ctx, kCameraInstance_New, {load<u32>(o + kObjClassPtr)}));
    call(ctx, ifn::WorldObj_CopyFrom, {n, o, 0});
    if ((flags & kCloneChildren) != 0) CloneChildren(ctx, o, n, flags);
    ret(ctx, n);
}

// void CameraInstance_Dump(CameraInstance* o, s32 depth, u32 flags)  (slot +60)
// The tree at the same depth (flags | 1), then the skeleton one level down
// while depth > 0.
DECOMP(005A5DC0, CameraInstance_Dump) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 depth = ctx.gpr[4];
    call(ctx, ifn::WorldObj_DumpTree, {o, depth, ctx.gpr[5] | 1});
    if (static_cast<s32>(depth) <= 0) return;
    if (static_cast<u32>(Vcall0(ctx, o, kVfnGetSkeleton)) == 0) return;
    const u32 sk = static_cast<u32>(Vcall0(ctx, o, kVfnGetSkeleton));
    call(ctx, ifn::Skeleton_DumpTree, {sk, static_cast<u64>(static_cast<s64>(static_cast<s32>(depth - 1)))});
}
