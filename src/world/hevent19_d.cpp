// World_HandleEvent callees, part 19d: a speaker line node's skeleton
// binding (the pose and animated skeleton it rebuilds, with Havok's inlined
// hkArray code).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
// hkArray {+0 data, +4 s32 size, +8 u32 capacity | flags}: bit 31 means
// the storage is not owned; the low 30 bits are the capacity.
constexpr u32 kArrNotOwned = 0x80000000u, kArrCapMask = 0x3FFFFFFFu;
constexpr u32 kHkMem = 0x011DF31C;       // Havok container allocator (vt +20: free(p, bytes)) (as hevent10_i.cpp)
constexpr u32 kHkMemVtFree = 20;
constexpr u32 kArrayReserve = 0x007CCE88;  // (allocator, hkArray*, s32 n, s32 elem size) (name?)
// The thread's allocator: *(r13 - 28332) +44, vt +8 alloc(size), +12 free(p, size).
constexpr s32 kTlsAllocHolder = -28332;
constexpr u32 kHolderAllocator = 44, kAllocVtAlloc = 8, kAllocVtFree = 12;

// Node: +4 Speaker*, +24 animated skeleton (48 bytes), +28 pose (56 bytes).
constexpr u32 kNodeSpeaker = 4, kNodeAnimSkel = 24, kNodePose = 28, kNode32 = 32;
constexpr u32 kAnimSkelSize = 48, kAnimSkelRate = 32, kRate001 = 0x3C23D70Au;  // f32 0.01
constexpr u32 kAnimSkelRelease = 0x007D4EA8;    // (AnimSkel*, 0) (name?)
constexpr u32 kAnimSkelConstruct = 0x00A45750;  // (AnimSkel*, Skeleton*) (name?)
constexpr u32 kPoseSetReference = 0x00A56E68;   // (Pose* or 0) (name?)
constexpr u32 kSpeakerSetLine = 0x0034F690;     // (Speaker*, Skeleton*, 0) (symbols.txt)
// Pose (56 bytes): +0 Skeleton*, +4 / +16 hkArray of 48-byte transforms
// (model, local), +28 hkArray<u32> bone flags, +40 / +41 u8, +44
// hkArray<f32> float slots. Skeleton: +28 s32 bone count, +64 s32 float
// slot count.
constexpr u32 kPoseSize = 56, kSkelBones = 28, kSkelFloats = 64;

inline u32 tls_allocator(PPUContext& ctx) {
    return load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + kTlsAllocHolder) + kHolderAllocator);
}
inline void hk_free(PPUContext& ctx, u32 ptr, u32 bytes) {
    vcall(ctx, kHkMem, kHkMemVtFree, {kHkMem, ptr, static_cast<u64>(static_cast<s64>(static_cast<s32>(bytes)))});
}
inline void reserve(PPUContext& ctx, u32 arr, s32 n, u32 elem) {
    call(ctx, kArrayReserve, {kHkMem, arr, static_cast<u64>(static_cast<s64>(n)), elem});
}
// The inlined setSize growth: capacity below n reserves max(2 x capacity, n).
inline void grow(PPUContext& ctx, u32 arr, s32 n, u32 elem) {
    const u32 cap = load<u32>(arr + 8);
    if (static_cast<s32>(cap & kArrCapMask) >= n) return;
    const s32 dbl = static_cast<s32>((cap << 1) & 0x7FFFFFFEu);
    reserve(ctx, arr, dbl > n ? dbl : n, elem);
}
}  // namespace

// void Node_Init350178(Node* n, Skeleton* skel, u8 force) (name?): binds
// the node to skel (unless already bound and not forced): destroys the old
// pose, releases and recreates the animated skeleton (rate 0.01), builds a
// pose for the skeleton (arrays sized to the bone and float slot counts,
// new float slots zeroed), sets it to the reference pose and refreshes the
// speaker's line. A failed animated skeleton allocation still writes +4 /
// +32 of address 0; a failed pose allocation leaves +28 = 0 and the
// reference pose call gets 0 (all as the original).
DECOMP(00350178, Node_Init350178) {
    const u32 n = arg<u32>(ctx, 0), skel = arg<u32>(ctx, 1);
    const u32 force = arg<u8>(ctx, 2);
    if (load<u32>(n + kNodeAnimSkel) != 0 && force == 0) return ret(ctx, 0u);
    if (skel == 0) return ret(ctx, load<u32>(n + kNodeAnimSkel) == 0 ? 0u : force);
    // Destroy the old pose (arrays in reverse order, then the block).
    const u32 old = load<u32>(n + kNodePose);
    if (old != 0) {
        u32 cap = load<u32>(old + 52);
        store<u32>(old + 48, 0);
        if ((cap & kArrNotOwned) == 0) hk_free(ctx, load<u32>(old + 44), cap << 2);
        cap = load<u32>(old + 36);
        store<u32>(old + 44, 0);
        store<u32>(old + 52, kArrNotOwned);
        store<u32>(old + 32, 0);
        if ((cap & kArrNotOwned) == 0) hk_free(ctx, load<u32>(old + 28), cap << 2);
        cap = load<u32>(old + 24);
        store<u32>(old + 28, 0);
        store<u32>(old + 36, kArrNotOwned);
        store<u32>(old + 20, 0);
        if ((cap & kArrNotOwned) == 0) hk_free(ctx, load<u32>(old + 16), (cap & kArrCapMask) * 48);
        cap = load<u32>(old + 12);
        store<u32>(old + 16, 0);
        store<u32>(old + 24, kArrNotOwned);
        store<u32>(old + 8, 0);
        if ((cap & kArrNotOwned) == 0) hk_free(ctx, load<u32>(old + 4), (cap & kArrCapMask) * 48);
        store<u32>(old + 4, 0);
        store<u32>(old + 12, kArrNotOwned);
        const u32 al = tls_allocator(ctx);
        vcall(ctx, al, kAllocVtFree, {al, old, kPoseSize});
        store<u32>(n + kNodePose, 0);
    }
    const u32 as_old = load<u32>(n + kNodeAnimSkel);
    if (as_old != 0) {
        store<u32>(n + kNode32, 0);
        call(ctx, kAnimSkelRelease, {as_old, 0});
    }
    u32 al = tls_allocator(ctx);
    const u32 as = static_cast<u32>(vcall(ctx, al, kAllocVtAlloc, {al, kAnimSkelSize}));
    store<u16>(as + 4, kAnimSkelSize);  // written even when the allocation failed
    if (as != 0) call(ctx, kAnimSkelConstruct, {as, skel});
    store<u32>(n + kNodeAnimSkel, as);
    store<u32>(as + kAnimSkelRate, kRate001);
    al = tls_allocator(ctx);
    const u32 p = static_cast<u32>(vcall(ctx, al, kAllocVtAlloc, {al, kPoseSize}));
    if (p != 0) {
        store<u32>(p + 0, skel);
        store<u32>(p + 4, 0);
        store<u32>(p + 8, 0);
        store<u32>(p + 12, kArrNotOwned);
        store<u32>(p + 16, 0);
        store<u32>(p + 20, 0);
        store<u32>(p + 24, kArrNotOwned);
        store<u32>(p + 28, 0);
        store<u32>(p + 32, 0);
        store<u32>(p + 36, kArrNotOwned);
        store<u8>(p + 40, 0);
        store<u8>(p + 41, 0);
        store<u32>(p + 44, 0);
        store<u32>(p + 48, 0);
        store<u32>(p + 52, kArrNotOwned);
        const s32 bones = static_cast<s32>(load<u32>(skel + kSkelBones));
        const s32 rounded = static_cast<s32>(static_cast<u32>(bones + 3) & ~3u);
        if (rounded > 0) reserve(ctx, p + 16, rounded, 48);
        if (static_cast<s32>(load<u32>(p + 12) & kArrCapMask) < rounded) reserve(ctx, p + 4, rounded, 48);
        if (static_cast<s32>(load<u32>(p + 36) & kArrCapMask) < rounded) reserve(ctx, p + 28, rounded, 4);
        grow(ctx, p + 16, bones, 48);
        store<u32>(p + 20, static_cast<u32>(bones));
        grow(ctx, p + 4, bones, 48);
        store<u32>(p + 8, static_cast<u32>(bones));
        grow(ctx, p + 28, bones, 4);
        store<u32>(p + 32, static_cast<u32>(bones));
        const s32 floats = static_cast<s32>(load<u32>(load<u32>(p + 0) + kSkelFloats));
        const s32 f4 = static_cast<s32>(static_cast<u32>(floats + 3) & ~3u);
        if (static_cast<s32>(load<u32>(p + 52) & kArrCapMask) < f4) reserve(ctx, p + 44, f4, 4);
        grow(ctx, p + 44, floats, 4);
        const s32 have = static_cast<s32>(load<u32>(p + 48));
        const u32 base = load<u32>(p + 44) + static_cast<u32>(have) * 4;
        for (s32 k = 0; k < floats - have; ++k) store<u32>(base + 4 * static_cast<u32>(k), 0);
        store<u32>(p + 48, static_cast<u32>(floats));
    }
    store<u32>(n + kNodePose, p);
    call(ctx, kPoseSetReference, {p});
    const u32 sp = load<u32>(n + kNodeSpeaker);
    if (sp != 0) call(ctx, kSpeakerSetLine, {sp, skel, 0});
}
