// World_HandleEvent callees, part 18g: animator component reset and the
// per-layer animation stop.
#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
// Anim component: +8 Object* (vt +60 animator getter; +752 u8 flags, bytes
// +658..+710 bit 6 0x40 cleared), +300 / +304 u32, +328 Entry*[] with +336
// u16 count, +372 Entry*, +376 u32, +384 u32 flags (bit 0: "owns the
// object's flags", bits 3..6 cleared). Entry vt +40 enable(e, u8), +172 bit
// 18 0x40000 re-enable.
constexpr u32 kCompObj = 8, kComp300 = 300, kComp304 = 304, kCompEntries = 328, kCompEntryCount = 336;
constexpr u32 kCompEntry = 372, kComp376 = 376, kCompFlags = 384;
constexpr u32 kCompOwns = 0x1, kCompFlagsClear = 0x78;
constexpr u32 kObjVtAnimator = 60, kObjFlags752 = 752, kObjF752A = 0x80, kObjF752B = 0x40, kObjByteBit = 0x40;
constexpr u32 kEntVtEnable = 40, kEntFlags172 = 172, kEntReenable = 0x40000;
constexpr u32 kStopKeyBB5106E1 = 0xBB5106E1u;
constexpr u32 kAnimStop254814 = 0x00254814;  // (Animator*, key) (hevent17_b.cpp)
constexpr u32 kComp254168 = 0x00254168;      // (Component*, 0) (name?)

// Animator: +32 u32 handle, 8 layers of 40 bytes from +40 (+0 key, +4
// Layer* (+54 u16 track count), +20 u32 track id).
constexpr u32 kAnimHandle = 32, kAnimLayers = 40, kLayerSize = 40, kNumLayers = 8;
constexpr u32 kLayerPtr = 4, kLayerTrackId = 20, kLayerTracks = 54;
// The handle pool *0x00F557F8: +256 s32 count, +260 16-byte entries (+8
// object, +12 u32: bit 31 free, bits 15..30 serial). A handle is (index <<
// 16) | serial; serial 0 is null. Pool objects: +0 Def* (+0 parent Def*,
// +24 u32, +36 u32 track set).
constexpr u32 kPool = 0x00F557F8, kPoolCount = 256, kPoolEntries = 260, kPoolEntrySize = 16;
constexpr u32 kPeObject = 8, kPeSerial = 12;
constexpr u32 kDefParent = 0, kDef24 = 24, kDefTracks = 36;
// Track: +28 s32 kind (5: stoppable), +32 u32 id (0: any).
constexpr u32 kTrackKind = 28, kTrackId = 32, kTrackStoppable = 5;
constexpr u32 kTrackFind4B3624 = 0x004B3624;   // (Layer*, s16 index, track set) -> Track* (name?)
constexpr u32 kAnimHandle254438 = 0x00254438;  // (u32* out handle, Animator*, u32) (name?)
constexpr u32 kTrackStop254210 = 0x00254210;   // (Animator*, Object*, Track*) (name?)

// The pool object of a handle, or 0.
u32 resolve(u32 pool, u32 h) {
    const u32 serial = h & 0xFFFF;
    if (serial == 0) return 0;
    if (static_cast<s32>(h >> 16) >= load<s32>(pool + kPoolCount)) return 0;
    const u32 e = load<u32>(pool + kPoolEntries) + (h >> 16) * kPoolEntrySize;
    if (((load<u32>(e + kPeSerial) >> 15) & 0xFFFF) != serial) return 0;
    if (load<u8>(e + kPeSerial) & 0x80) return 0;
    return load<u32>(e + kPeObject);
}

// Disables an entry, re-enabling it when its +172 bit 18 is set.
void reenable(PPUContext& ctx, u32 e) {
    vcall(ctx, e, kEntVtEnable, {e, 0});
    if (load<u32>(e + kEntFlags172) & kEntReenable) vcall(ctx, e, kEntVtEnable, {e, 1});
}
}  // namespace

// void Anim_Set2549F8(Component* c, u8 full) (name?): resets the anim
// component: stops the animator's 0xBB5106E1 animations, 0x00254168 (c, 0),
// re-enables (full: all +328 entries, emptying the list) and drops the +372
// entry, clears flag bits 3..6 and, when the component owns the object's
// flags, clears bit 6 of its flag bytes.
DECOMP(002549F8, Anim_Set2549F8) {
    const u32 c = arg<u32>(ctx, 0), full = arg<u8>(ctx, 1);
    const u32 o = load<u32>(c + kCompObj);
    store<u32>(c + kComp304, 0);
    store<u32>(c + kComp300, 0);
    const u32 a = static_cast<u32>(vcall(ctx, o, kObjVtAnimator, {o}));
    if (a != 0) call(ctx, kAnimStop254814, {a, kStopKeyBB5106E1});
    call(ctx, kComp254168, {c, 0});
    if (full != 0) {
        for (u32 i = 0; i < load<u16>(c + kCompEntryCount); ++i)
            reenable(ctx, load<u32>(load<u32>(c + kCompEntries) + 4 * i));
        store<u16>(c + kCompEntryCount, 0);
    }
    if (load<u32>(c + kCompEntry) != 0) {
        const u32 e = load<u32>(c + kCompEntry);
        vcall(ctx, e, kEntVtEnable, {e, 0});
        const u32 e2 = load<u32>(c + kCompEntry);
        if (load<u32>(e2 + kEntFlags172) & kEntReenable) vcall(ctx, e2, kEntVtEnable, {e2, 1});
        store<u32>(c + kCompEntry, 0);
    }
    const u32 f = load<u32>(c + kCompFlags);
    store<u32>(c + kComp376, 0);
    store<u32>(c + kCompFlags, f & ~kCompFlagsClear);
    if ((f & kCompOwns) == 0) {
        ret(ctx, 0u);  // r3 left as the original leaves it
        return;
    }
    const u32 b = load<u8>(o + kObjFlags752);
    if ((b & kObjF752A) == 0 || (b & kObjF752B) != 0) {
        const u32 x = load<u8>(o + 658), y = load<u8>(o + 660);
        store<u8>(o + 658, static_cast<u8>(x & ~kObjByteBit));
        store<u8>(o + 660, static_cast<u8>(y & ~kObjByteBit));
    }
    constexpr u32 kBytes[4] = {659, 702, 671, 710};
    u32 v[4];
    for (u32 k = 0; k < 4; ++k) v[k] = load<u8>(o + kBytes[k]);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + kBytes[k], static_cast<u8>(v[k] & ~kObjByteBit));
    const u32 f2 = load<u32>(c + kCompFlags) & ~kCompOwns;
    store<u32>(c + kCompFlags, f2);
    ret(ctx, f2);
}

// void Anim_Stop254564(Animator* a, u32 key) (name?): for the layer with
// key, walks the animator's object (and, while no track is found, its
// parent defs) for each of the layer's tracks, and stops the stoppable
// track (kind 5, id 0 or the layer's) it finds (0x00254210).
DECOMP(00254564, Anim_Stop254564) {
    const u32 a = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    u32 i = 0;
    while (load<u32>(a + kAnimLayers + kLayerSize * i) != key)
        if (++i >= kNumLayers) return;
    const u32 l = a + kLayerSize * i + kAnimLayers;
    const u32 layer = load<u32>(l + kLayerPtr);
    if (layer == 0) return;
    u32 obj = resolve(load<u32>(kPool), load<u32>(a + kAnimHandle));
    if (obj == 0) return;
    const u32 n = load<u16>(layer + kLayerTracks);
    if (n == 0) return;
    Stack frame(ctx, 16);  // the two handle words (r1 +112 / +116 in the original frame)
    const u32 h1 = frame.addr(), h2 = frame.addr() + 4;
    for (u32 j = 0; j < n; ++j) {
        const u64 idx = static_cast<u64>(static_cast<s64>(static_cast<s16>(j)));
        u32 t = static_cast<u32>(call(ctx, kTrackFind4B3624, {layer, idx, load<u32>(load<u32>(obj) + kDefTracks)}));
        while (t == 0) {
            const u32 parent = load<u32>(load<u32>(obj) + kDefParent);
            if (parent == 0) break;
            t = static_cast<u32>(call(ctx, kTrackFind4B3624, {layer, idx, load<u32>(parent + kDefTracks)}));
            call(ctx, kAnimHandle254438, {h1, a, load<u32>(load<u32>(load<u32>(obj)) + kDef24)});
            if ((load<u32>(h1) & 0xFFFF) == 0) break;
            const u32 pool = load<u32>(kPool);
            call(ctx, kAnimHandle254438, {h2, a, load<u32>(load<u32>(load<u32>(obj)) + kDef24)});
            obj = resolve(pool, load<u32>(h2));  // a null object is still walked (as the original)
        }
        if (t == 0 || load<s32>(t + kTrackKind) != static_cast<s32>(kTrackStoppable)) continue;
        const u32 id = load<u32>(t + kTrackId);
        if (id != 0 && id != load<u32>(l + kLayerTrackId)) continue;
        call(ctx, kTrackStop254210, {a, obj, t});
    }
}

namespace {
// Effect components set up for an object (types as Effect_FindComponent):
// 0x0027469E kind 5 bound (+28 WorldObj ref, +36 u32), 0x77EB3A29 kind 12
// (+20 u32), 0x001E8ECC kind 4 placed (+32 3x4 matrix).
constexpr u32 kEffSize = 140, kEffFlags = 136, kEffDetached = 0x200;
constexpr u32 kCompVtKind = 32, kCompBound = 0x0027469E, kKindBound = 5, kBoundRef = 28, kBoundValue = 36;
constexpr u32 kCompTagged = 0x77EB3A29, kKindTagged = 12, kTaggedValue = 20;
constexpr u32 kCompPlaced = 0x001E8ECC, kKindPlaced = 4, kPlacedMatrix = 32;
constexpr u32 kRefVtRelease = 8, kRefCount = 4;
// Object: +160 WorldObj (vt +28 skeleton), +164 / +168 u32.
constexpr u32 kObj164 = 164, kObj168 = 168, kWobjVtSkeleton = 28;
constexpr u32 kWorldObjUpdateLod = 0x0057EE00;  // (WorldObj*, 0, u8) (symbols.txt)
constexpr u32 kObjBoneMatrixCE688 = 0x000CE688;  // (f32 out[4][4], Object*, s32 bone, 1, 1) (name?)

s32 kind_of(PPUContext& ctx, u32 comp) { return static_cast<s32>(vcall(ctx, comp, kCompVtKind, {comp})); }
}  // namespace

// Effect* Obj_CreateEffectDCA20(Object* o, Def* def, u32 bone, u8 start)
// (name?): creates an effect for def bound to the object's WorldObj
// (reference counted) and its +164 / +168 values, placed at the bone's
// matrix (bone hash 0: the object's), and starts it when asked. Returns
// the effect (0 without a def). A failed allocation still looks components
// up and flags / starts address 0 (as the original).
DECOMP(000DCA20, Obj_CreateEffectDCA20) {
    const u32 o = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1), bone = arg<u32>(ctx, 2), start = arg<u8>(ctx, 3);
    if (def == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 e = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kEffSize}));
    if (e != 0) call(ctx, hevent7::h7fn::Effect_Construct, {e, def});
    if (const u32 c = static_cast<u32>(call(ctx, hevent7::h7fn::Effect_FindComponent, {e, kCompBound}));
        c != 0 && kind_of(ctx, c) == static_cast<s32>(kKindBound)) {
        const u32 old = load<u32>(c + kBoundRef);
        const u32 wobj = load<u32>(o + hevent10::kObjWorldObj);
        if (old != 0) vcall(ctx, old, kRefVtRelease, {old});
        store<u32>(c + kBoundRef, wobj);
        if (wobj != 0) store<u16>(wobj + kRefCount, static_cast<u16>(load<u16>(wobj + kRefCount) + 1));
        store<u32>(c + kBoundValue, load<u32>(o + kObj164));
    }
    if (const u32 c = static_cast<u32>(call(ctx, hevent7::h7fn::Effect_FindComponent, {e, kCompTagged}));
        c != 0 && kind_of(ctx, c) == static_cast<s32>(kKindTagged))
        store<u32>(c + kTaggedValue, load<u32>(o + kObj168));
    u32 w = load<u32>(o + hevent10::kObjWorldObj);
    if (w != 0 && bone != 0) {
        call(ctx, kWorldObjUpdateLod, {w, 0, 0});
        w = load<u32>(o + hevent10::kObjWorldObj);
    }
    u32 index = 0xFFFFFFFFu;
    if (bone != 0 && w != 0) {
        const u32 skel = static_cast<u32>(vcall(ctx, w, kWobjVtSkeleton, {w}));
        if (skel != 0) index = static_cast<u32>(call(ctx, hevent9::h9fn::Skeleton_FindBone, {skel, bone}));
    }
    Stack frame(ctx, 80);  // the bone matrix (r1 +112 in the original frame)
    const u32 m = frame.addr();
    call(ctx, kObjBoneMatrixCE688, {m, o, index, 1, 1});
    if (const u32 c = static_cast<u32>(call(ctx, hevent7::h7fn::Effect_FindComponent, {e, kCompPlaced}));
        c != 0 && kind_of(ctx, c) == static_cast<s32>(kKindPlaced))
        hevent13::copy_mat(ctx, c + kPlacedMatrix, m);
    if (start != 0) {
        store<u32>(e + kEffFlags, load<u32>(e + kEffFlags) | kEffDetached);
        call(ctx, hevent7::h7fn::Effect_Start, {e});
    }
    ret(ctx, e);
}
