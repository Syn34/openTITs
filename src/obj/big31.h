// Game code, large-function sweep part 31: helpers shared by big31_*.cpp
// (the Havok ray casts with an all-hits collector on the stack).
#pragma once

#include "native/vec.h"

namespace sly4::obj {

namespace b31fn {
constexpr u32 Text_Lookup34A398 = 0x0034A398;   // (u32 id) -> text entry or 0
constexpr u32 Speaker_Say3514AC = 0x003514AC;   // (Speaker*, text, u8, -, -, -, r9, r10, voice; f1-f3)
constexpr u32 Obj_NotifyA362F1C = 0x00362F1C;   // (Obj*, u8 a, u8 b)
constexpr u32 SortedMap_Find3 = 0x0076611C;     // (map, const u32* key) -> value slot
constexpr u32 kHandleMap = 0x0106C228;          // SortedMap: handle -> Obj*
constexpr u32 HkWorld_CastRay92E510 = 0x0092E510;  // (hkpWorld*, const RayInput*, Collector*)
constexpr u32 HkAllHits_Sort887E28 = 0x00887E28;   // (Collector*)
}  // namespace b31fn

constexpr u32 kVtRayCol = 0x00F1DE80;  // all-hits ray collector vtable (80-byte hits, 8 inline)
constexpr u32 kHkHeap = 0x011DF31C;    // Havok heap allocator object (vt 20 frees (heap, p, bytes))
constexpr u32 kPropKind = 0xA734004D;   // shape property: the shape's kind
constexpr u32 kKindOwned = 0x7A30D7E3;  // that kind: the shape belongs to the object in kPropOwner
constexpr u32 kPropOwner = 0x91C7CF5B;  // shape property: the owning object

// hkpWorldRayCastInput (48 bytes): from, to, +32 u8, +36, +40.
struct RayInput {
    Vec4 from;     // +0
    Vec4 to;       // +16
    u8 b32;        // +32
    pad<3> _33;
    be_u32 w36;    // +36
    be_u32 w40;    // +40
    pad<4> _44;
};
static_assert(sizeof(RayInput) == 48);
// One 80-byte ray hit: +16 the fraction, +68 the collidable.
struct RayHit {
    pad<16> _0;
    be_f32 fraction;  // +16
    be_u32 w20;       // +20
    pad<8> _24;
    be_u32 w32;       // +32
    pad<28> _36;
    be_u32 w64;       // +64
    be_u32 shape;     // +68: an HkCollidable
    pad<8> _72;
};
static_assert(sizeof(RayHit) == 80 && offsetof(RayHit, shape) == 68);
// The all-hits ray collector with eight inline hits (an hkArray at +16).
struct RayCollector {
    be_u32 vtable;     // +0
    be_f32 early_out;  // +4
    pad<8> _8;
    be_u32 hits;       // +16: RayHit*
    be_s32 count;      // +20
    be_u32 capflags;   // +24: capacity | 0x80000000 (inline storage)
    pad<4> _28;
    RayHit inline_hits[8];  // +32
};
static_assert(sizeof(RayCollector) == 672);

// The game's physics world: the Game's +0x1281D4 holder, its +4.
inline u32 PhysWorld() { return gm<be_u32>(gm<be_u32>(global<be_u32>(kWorld) + 0x1281D4) + 4); }

// The collector set up as the compiled code does (early-out 1.0, the
// eight inline hits reset; count and early-out stored again at the end).
inline void RayCollector_Init(RayCollector* c) {
    c->early_out = 1.0f;
    c->vtable = kVtRayCol;
    c->hits = guest(&c->inline_hits[0]);
    c->count = 0;
    c->capflags = 0x80000008u;
    for (RayHit& h : c->inline_hits) {
        set_bits(h.fraction, 0x3F800000u);
        h.w20 = 0xFFFFFFFFu;
        h.w64 = 0u;
        h.w32 = 0xFFFFFFFFu;
        h.shape = 0u;
    }
    c->count = 0;
    c->early_out = 1.0f;
}

// A heap hit array freed (count / vtable reset first) through kHkHeap's vt
// 20 (heap, p, 80 * capacity).
inline void RayCollector_Free(RayCollector* c, u32 hits, u32 cw) {
    if ((cw & 0x80000000u) != 0) return;
    const s32 size = static_cast<s32>(80u * (cw & 0x3FFFFFFFu));
    c->count = 0;
    c->vtable = kVtRayCol;
    vcall(host<u8>(kHkHeap), 20, hits, size);
}

}  // namespace sly4::obj
