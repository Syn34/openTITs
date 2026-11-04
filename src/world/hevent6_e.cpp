// World_HandleEvent callees, part 6: attachment removal, the handle pool's
// remove, the treasure reset and the object range test (127..138
// instructions).
#include <cmath>

#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent6e {
namespace efn {
constexpr u32 Effect_FindComponent = 0x004BB4B4;  // (Effect*, u32 type hash) -> component or 0
constexpr u32 Vector_RemoveAt74534C = 0x0074534C; // (Vector*, s64 index) (name?)
}  // namespace efn

// Attachment slots: holder +340 -> {+0 slots (240 bytes each), +8 u16 count};
// slot +176 flags (0x04000000 keep pose, 0x02000000 restore link),
// +192 Object*, +220 u32 link, +224 f32.
constexpr u32 kSlots = 340, kSlotSize = 240;
constexpr u32 kSlotKeepPose = 0x04000000, kSlotRestoreLink = 0x02000000;
constexpr u32 kOwnerRefType = 0x8C3DFCAA;       // component type (name?)
constexpr u32 kTreasureGlowType = 0x811B12AF;   // component type (name?)
constexpr u32 kTreasureSparkType = 0xCBC95E9E;  // component type (name?)
constexpr u32 kRefStateBound = 5;               // component vfunc +32 state with a +28 reference

constexpr u32 kIdentityQuat = 0x01164C90;       // 4 f32 (name?)
constexpr u32 kOneVec3 = 0x01164B30;            // as uielem/build6.h

// Object range test: per-object float / byte attributes indexed by the
// attribute descriptors' +16 -> +64 index (object +48 -> +12 values).
constexpr u32 kAttrOuter = 0x010AB160, kAttrInner = 0x010AB178;  // f32 radii (name?)
constexpr u32 kAttrUseOwner = 0x010AB0E8, kAttrUseOwner2 = 0x010AB118;
constexpr u32 kZero453348 = 0x00453348;         // f32 0.0
constexpr u32 kDefaultRange = 0x010B1710;       // f32 tuning (0x010B16C8 +72)
constexpr u32 kPlayerSlot = 4208 + 8;           // World +4216: player object (+144 position)
}  // namespace hevent6e
}  // namespace sly4

using namespace sly4::hevent6e;

namespace {
// Clears an effect component's +28 reference (release, vfunc +8) when the
// component is in state 5. Returns r3 as the original leaves it.
u64 drop_ref(PPUContext& ctx, u32 c, bool clear36) {
    const u64 st = vcall(ctx, c, 32, {c});
    if (static_cast<s32>(st) != static_cast<s32>(kRefStateBound)) return st;
    const u32 o = load<u32>(c + 28);
    if (o != 0) vcall(ctx, o, 8, {o});
    store<u32>(c + 28, 0);
    if (clear36) store<u32>(c + 36, 0);
    return 0;
}

inline u32 attr_index(u32 desc) { return load<u32>(load<u32>(desc + 16) + 64) << 2; }
}  // namespace

// void Obj_Detach0D72C0(Holder* h, Object* o) (name?): finds o's attachment
// slot; unless the slot keeps the pose, o gets vfunc +112(slot +224) and, with
// the restore flag, the slot object vfunc +104(slot +220). Then o vfunc
// +184(0); o's vfunc +272 effect has its 0x8C3DFCAA component's reference
// dropped (+28 / +36 = 0); the slot is removed (0x0074534C).
DECOMP(000D72C0, Obj_Detach0D72C0) {
    const u32 h = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    if (o == 0) return;
    const u32 v = load<u32>(h + kSlots);
    u32 i = 0, slot = 0;
    for (;; ++i) {
        if (i >= (v != 0 ? load<u16>(v + 8) : 0u)) {
            ctx.gpr[3] = v;  // r3 as the original leaves it
            return;
        }
        slot = load<u32>(v) + i * kSlotSize;
        if (load<u32>(slot + 192) == o) break;
    }
    if ((load<u32>(slot + 176) & kSlotKeepPose) == 0) {
        ctx.fpr[1] = load<f32>(slot + 224);
        vcall(ctx, o, 112, {o});
        if ((load<u32>(slot + 176) & kSlotRestoreLink) != 0) {
            const u32 so = load<u32>(slot + 192);
            vcall(ctx, so, 104, {so, load<u32>(slot + 220)});
        }
    }
    vcall(ctx, o, 184, {o, 0});
    const u32 e = static_cast<u32>(vcall(ctx, o, 272, {o}));
    if (e != 0) {
        const u32 c = static_cast<u32>(call(ctx, efn::Effect_FindComponent, {e, kOwnerRefType}));
        if (c != 0) drop_ref(ctx, c, true);
    }
    call(ctx, efn::Vector_RemoveAt74534C, {load<u32>(h + kSlots), static_cast<u64>(static_cast<s64>(static_cast<s32>(i)))});
}

// void Pool_Remove784DE4(Handle* out, Pool* p, Handle it) (name?): removes
// the element of handle it (index << 16 | serial) from the pool's in-use list
// and pushes it on the free list; *out = the previous element's handle
// (0xFFFF0000 at the front), 0 for a stale / free / out-of-range handle.
// Pool: +0 free tail, +4 free head (-1 none), +8 last, +12 first, +16 used,
// +20 free count, +24 capacity, +28 entries (16 bytes: +0 next, +4 prev,
// +12 flags: 0x80000000 free, bits 15..30 serial).
DECOMP(00784DE4, Pool_Remove784DE4) {
    const u32 out = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), it = arg<u32>(ctx, 2);
    const u32 used = load<u32>(p + 16);
    const s32 free_head = load<s32>(p + 4);
    const s32 first = load<s32>(p + 12);
    u32 result = 0;
    const u32 serial = it & 0xFFFF;
    const s32 index = static_cast<s32>(it >> 16);
    const s32 cap = used != 0 && serial != 0 ? load<s32>(p + 24) : 0;
    if (used != 0 && serial != 0 && index < cap) {
        const u32 off = static_cast<u32>(index) << 4;
        const u32 ents = load<u32>(p + 28);
        const u32 e = ents + off;
        const u32 flags = load<u32>(e + 12);
        if (((flags >> 15) & 0xFFFF) == serial && (load<u8>(e + 12) & 0x80) == 0) {
            // Unlink from the in-use list.
            if (index == load<s32>(p + 8)) {
                store<u32>(p + 8, load<u32>(e + 4));
            } else {
                const u32 next = load<u32>(ents + off);
                store<u32>(ents + (next << 4) + 4, load<u32>(e + 4));
            }
            if (index == first) {
                store<u32>(p + 12, load<u32>(ents + off));
                result = 0xFFFF0000u;
            } else {
                const u32 prev = load<u32>(e + 4);
                store<u32>(ents + static_cast<u32>(static_cast<s32>(prev << 4)), load<u32>(ents + off));
                const u32 prev2 = load<u32>(e + 4);
                result = (prev2 << 16) | ((load<u32>(ents + ((prev2 << 4) & 0xFFFF0u) + 12) >> 15) & 0xFFFF);
            }
            // Push on the free list (prev = -1, free flag set, serial cleared).
            auto mark_free = [&] {
                const u32 fe = load<u32>(p + 28) + off;
                const u8 b = static_cast<u8>(load<u8>(fe + 12) | 0x80);
                const u32 w = ((load<u32>(fe + 12) & 0x00FFFFFFu) | (u32(b) << 24)) & ~0x7FFF8000u;
                store<u8>(fe + 12, b);
                store<u32>(fe + 12, w);
            };
            if (free_head == -1) {
                store<u32>(ents + off, 0xFFFFFFFFu);
                store<u32>(e + 4, 0xFFFFFFFFu);
                mark_free();
                store<u32>(p + 4, static_cast<u32>(index));
                store<u32>(p + 0, static_cast<u32>(index));
            } else {
                store<u32>(ents + (static_cast<u32>(free_head) << 4) + 4, static_cast<u32>(index));
                store<u32>(ents + off, static_cast<u32>(free_head));
                store<u32>(load<u32>(p + 28) + off + 4, 0xFFFFFFFFu);
                mark_free();
                store<u32>(p + 4, static_cast<u32>(index));
                if (cap == 1) store<u32>(p + 0, static_cast<u32>(index));
            }
            const u32 n = used - 1;
            const u32 nfree = load<u32>(p + 20);
            store<u32>(p + 16, n);
            store<u32>(p + 20, nfree + 1);
            if (n == 0) {
                store<u32>(p + 12, 0xFFFFFFFFu);
                store<u32>(p + 8, 0xFFFFFFFFu);
            }
        }
    }
    store<u32>(out, result);
}

// void Treasure_Finish425980(Treasure* t) (name?): clears +0..63 (stvx, t is
// 16-byte aligned), +64 / +80 = the 0x01164C90 quaternion, +96 / +112 = the
// one vector, +128..+152 / +160 = 0, +164 bits 30 / 31 cleared; drops the
// references of the +156 effect's 0x811B12AF and 0xCBC95E9E components.
DECOMP(00425980, Treasure_Finish425980) {
    const u32 t = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 4; ++k) {
        const u32 a = (t + 16 * k) & ~15u;
        store<u64>(a, 0);
        store<u64>(a + 8, 0);
    }
    for (u32 base : {64u, 80u})
        for (u32 k = 0; k < 4; ++k) store<u32>(t + base + 4 * k, load<u32>(kIdentityQuat + 4 * k));
    for (u32 k = 0; k < 3; ++k) store<u32>(t + 96 + 4 * k, load<u32>(kOneVec3 + 4 * k));
    store<u32>(t + 112, load<u32>(kOneVec3));
    store<u32>(t + 116, load<u32>(kOneVec3 + 4));
    const u32 f164 = load<u32>(t + 164);
    store<u32>(t + 128, 0);
    store<u32>(t + 120, load<u32>(kOneVec3 + 8));
    for (u32 o : {132u, 136u, 140u, 144u, 148u, 152u, 160u}) store<u32>(t + o, 0);
    const u32 e = load<u32>(t + 156);
    store<u32>(t + 164, f164 & 0x3FFFFFFFu);
    ctx.fpr[1] = load<f32>(kOneVec3 + 8);  // f1 as the original leaves it
    if (e == 0) return;
    const u32 c = static_cast<u32>(call(ctx, efn::Effect_FindComponent, {e, kTreasureGlowType}));
    if (c != 0) drop_ref(ctx, c, false);
    const u32 c2 = static_cast<u32>(call(ctx, efn::Effect_FindComponent, {load<u32>(t + 156), kTreasureSparkType}));
    if (c2 != 0) ctx.gpr[3] = drop_ref(ctx, c2, false);
}

// bool Obj_Ready453350(Object* o) (name?): true when o has no range (outer
// and inner radius attributes <= 0; an inner radius of 0 on an object whose
// vfunc +20 is set and +36 is not 1 takes the tuning default), false without
// a player; else whether the player (World +4216, +144) is inside the outer
// radius and outside the inner one, measured to o's anchor (desc +40 when
// both owner attributes are set, else +60; position +128).
DECOMP(00453350, Obj_Ready453350) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 vals = load<u32>(load<u32>(o + 48) + 12);
    const f64 zero = load<f32>(kZero453348);
    f64 inner = load<f32>(vals + attr_index(kAttrInner));
    const f64 outer = load<f32>(vals + attr_index(kAttrOuter));
    if (inner == zero && vcall(ctx, o, 20, {o}) != 0 && load<s32>(o + 36) != 1) inner = load<f32>(kDefaultRange);
    if (outer <= zero && inner <= zero) {
        ret(ctx, 1u);
        return;
    }
    const u32 player = load<u32>(load<u32>(kCurrentWorld) + kPlayerSlot);
    if (player == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 pos = player + 144;
    const u32 bytes = load<u32>(load<u32>(o + 48) + 12);
    u32 anchor = 0;
    if (load<u8>(bytes + attr_index(kAttrUseOwner)) != 0 && load<u8>(bytes + attr_index(kAttrUseOwner2)) != 0) {
        const u32 d = load<u32>(o + 8);
        if (d != 0) anchor = load<u32>(d + 40);
    }
    if (anchor == 0) anchor = load<u32>(o + 60);
    const u32 q = anchor + 128;
    const f64 dy = static_cast<f32>(static_cast<f64>(load<f32>(pos + 4)) - load<f32>(q + 4));
    const f64 dx = static_cast<f32>(static_cast<f64>(load<f32>(pos + 0)) - load<f32>(q + 0));
    const f64 dz = static_cast<f32>(static_cast<f64>(load<f32>(pos + 8)) - load<f32>(q + 8));
    f64 d2 = fmuls(dy, dy);
    d2 = static_cast<f32>(std::fma(dx, dx, d2));
    d2 = static_cast<f32>(std::fma(dz, dz, d2));
    ctx.fpr[1] = d2;  // f1 as the original leaves it
    if (outer > zero && d2 >= static_cast<f64>(fmuls(outer, outer))) {
        ret(ctx, 0u);
        return;
    }
    if (inner > zero && d2 <= static_cast<f64>(fmuls(inner, inner))) {
        ret(ctx, 0u);
        return;
    }
    ret(ctx, 1u);
}
