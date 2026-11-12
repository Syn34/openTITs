// Native types and helpers shared by the game-object code in src/obj.
//
// The structures describe guest memory (big endian, 32-bit pointers); see
// docs/NATIVE.md. Float helpers reproduce the PPU's single-precision rules
// the way the verified code does: values travel as the double an FPR holds,
// each single-precision operation rounds once (`S`), fused multiply-adds
// round once after the fma, and `lf` loads a word bit for bit (NaN payloads
// kept, not quieted, as lfs does).
#pragma once

#include <bit>
#include <cmath>
#include <cstddef>

#include "opentit/native.h"

namespace sly4::obj {
using namespace opentit;
using namespace opentit::native;

// ---------------------------------------------------------------- guest memory
// The object at guest address ea, also for ea == 0 (host<T> maps 0 to
// nullptr; this is for the original's unchecked accesses near address 0).
template <typename T>
T& gm(u32 ea) {
    return *reinterpret_cast<T*>(mem::ptr(ea));
}
// The raw 32-bit word of a big-endian value (e.g. the bits of a be_f32).
template <typename T>
u32 bits(const be<T>& v) {
    static_assert(sizeof(T) == 4);
    return (u32(v.raw[0]) << 24) | (u32(v.raw[1]) << 16) | (u32(v.raw[2]) << 8) | u32(v.raw[3]);
}
template <typename T>
void set_bits(be<T>& v, u32 w) {
    static_assert(sizeof(T) == 4);
    v.raw[0] = u8(w >> 24);
    v.raw[1] = u8(w >> 16);
    v.raw[2] = u8(w >> 8);
    v.raw[3] = u8(w);
}

// The function descriptor (OPD) in obj's vtable at byte offset `slot`,
// for call_ptr when the object is not the first argument.
inline u32 vfn(const void* obj, u32 slot) { return gm<be_u32>(gm<be_u32>(guest(obj)) + slot); }

// ---------------------------------------------------------------- PPU float rules
// lfs: the double a single-precision word loads as, bit for bit.
inline u64 LfsBits(u32 w) {
    if ((w & 0x7F800000u) == 0x7F800000u && (w & 0x007FFFFFu) != 0)
        return (static_cast<u64>(w >> 31) << 63) | (0x7FFull << 52) | (static_cast<u64>(w & 0x007FFFFFu) << 29);
    return std::bit_cast<u64>(static_cast<f64>(std::bit_cast<f32>(w)));
}
inline f64 LfsW(u32 w) { return std::bit_cast<f64>(LfsBits(w)); }
inline f64 lf(const be_f32& v) { return LfsW(bits(v)); }
// lfd
inline f64 ld(const be_f64& v) { return v.get(); }
// Round to single (frsp / the rounding of every single-precision op).
inline f64 S(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }
inline f64 Adds(f64 a, f64 b) { return S(a + b); }
inline f64 Subs(f64 a, f64 b) { return S(a - b); }
inline f64 Muls(f64 a, f64 b) { return S(a * b); }
inline f64 Divs(f64 a, f64 b) { return S(a / b); }
inline f64 Sqrts(f64 a) { return S(std::sqrt(a)); }
inline f64 Fmadds(f64 a, f64 c, f64 b) { return S(std::fma(a, c, b)); }
inline f64 Fmsubs(f64 a, f64 c, f64 b) { return S(std::fma(a, c, -b)); }
inline f64 Fnmadds(f64 a, f64 c, f64 b) { return S(-std::fma(a, c, b)); }
inline f64 Fnmsubs(f64 a, f64 c, f64 b) { return S(-std::fma(a, c, -b)); }
// fsel d, a, c, b: a >= 0 (false for NaN) ? c : b.
inline f64 Fsel(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }
// fctiwz / fctidz then fcfid (the compiler's truncation idiom).
inline u64 Fctidz(f64 x) { return rt::fctid(x, true); }
inline f64 Fcfid(u64 v) { return rt::fcfid(v); }

// ---------------------------------------------------------------- common shapes
struct Vec3 {
    be_f32 x, y, z;  // +0, +4, +8
};
static_assert(sizeof(Vec3) == 12);
struct Vec4 {
    be_f32 x, y, z, w;  // +0 .. +12 (a quadword)
};
static_assert(sizeof(Vec4) == 16);
// A quadword as four raw words (lvx / stvx copies without float conversion).
struct Quad {
    be_u32 w[4];
};
static_assert(sizeof(Quad) == 16);

// CellGcmTexture (24 bytes; the same layout as sly4::gfx::GcmTexture).
struct GcmTexture {
    u8 format;        // +0
    u8 mipmap;        // +1
    u8 dimension;     // +2
    u8 cubemap;       // +3
    be_u32 remap;     // +4
    be_u16 width;     // +8
    be_u16 height;    // +10
    be_u16 depth;     // +12
    u8 location;      // +14
    pad<1> _15;
    be_u32 pitch;     // +16
    be_u32 offset;    // +20
};
static_assert(sizeof(GcmTexture) == 24);

constexpr u32 kZeroVec = 0x01164B40;  // Vec3: the global zero vector
constexpr u32 kWorld = 0x00F65CA0;    // World*: the game world

// A pointer vector {+0 data, +4 u16 capacity, +8 u16 count, +10 u16
// high-water mark} (PtrVector_Reserve 0x00551718).
template <typename T>
struct PtrVec {
    gp<gp<T>> data;  // +0
    be_u16 cap;      // +4
    pad<2> _6;
    be_u16 count;    // +8
    be_u16 mark;     // +10
};
static_assert(sizeof(PtrVec<int>) == 12);

// Havok's hkArray {+0 data, +4 s32 size, +8 capacity (low 30 bits) | flags}.
template <typename T>
struct HkArray {
    gp<T> data;        // +0
    be_s32 size;       // +4
    be_u32 capflags;   // +8
};
static_assert(sizeof(HkArray<int>) == 12);

// AVL tree nodes: {+0 s16 balance, +2 s16 height, +4 key, +8 value, +12
// left, +16 right}.
template <typename K, typename V>
struct AvlNode {
    be_s16 balance;             // +0
    be_s16 height;              // +2
    K key;                      // +4
    V value;                    // +8
    gp<AvlNode> left;           // +12
    gp<AvlNode> right;          // +16
};
static_assert(sizeof(AvlNode<be_u32, be_u32>) == 20);

// An AVL tree's root object: +8 u8 the "rebalance" flag.
struct AvlTree {
    pad<8> _0;
    u8 balanced;  // +8
};

// Common callees.
namespace fn {
constexpr u32 Mem_Alloc = 0x0054ED38;          // (size, pool, align) -> pointer or 0
constexpr u32 FreeList_Init = 0x00551A58;      // (list, elem_size, count, heap, bool)
constexpr u32 FreeList_Free = 0x00556D18;      // (list, p)
constexpr u32 SmallAlloc = 0x0055238C;         // (size) -> p or 0
constexpr u32 SmallFree = 0x00552420;          // (p, size)
constexpr u32 PtrVector_Reserve = 0x00551718;  // (v, n)
constexpr u32 String_Format = 0x005534D0;      // (fmt, ...) -> char*
constexpr u32 Memmove = 0x0071C3F0;            // (dst, src, n)
constexpr u32 Effect_SetHash = 0x004CF988;     // (set, hash)
constexpr u32 HkArray_Grow = 0x007CCD60;       // (allocator, hkArray*, elem size)
constexpr u32 Ui_Find = 0x004D5544;           // (const char* path, UiElem* parent) -> UiElem* or 0
}  // namespace fn
constexpr u32 kHkArrayAlloc = 0x011DF31C;  // the allocator HkArray_Grow is given

// A UI element (widget): vt slot 12 its type / state (3: a text choice),
// slot 188 sets its text (e, s, 0). Found with fn::Ui_Find.
struct UiElem;
inline u32 UiElem_State(UiElem* e) { return vcall<u32>(e, 12); }
inline void UiElem_SetText(UiElem* e, u32 s) { vcall(e, 188, s, 0u); }

// ---------------------------------------------------------------- Havok memory
// The thread's Havok memory router: *(r13 - 28332) +44 the allocator, vt +8
// alloc(a, size). (r13 is the PPU thread pointer, read from the context:
// fixed per thread, not a leftover register.) The u16 size goes to +4 of
// the result, also when the allocation failed (a store to address 4, as the
// original does).
inline u32 HkAlloc(u32 size) {
    const u32 router = gm<be_u32>(static_cast<u32>(ctx().gpr[13]) - 28332u);
    u8* a = host<u8>(gm<be_u32>(router + 44));
    const u32 p = vcall<u32>(a, 8, size);
    gm<be_u16>(p + 4) = static_cast<u16>(size);
    return p;
}

// A Havok entity's property block: 16-byte entries {u32 key, ..., u64
// value at +8} at +124, s32 count +128 (both re-read every step).
struct HkPropEntry {
    be_u32 key;   // +0
    pad<4> _4;
    be_u32 hi;    // +8
    be_u32 lo;    // +12
};
static_assert(sizeof(HkPropEntry) == 16);
struct HkProps {
    pad<124> _0;
    gp<HkPropEntry> table;  // +124
    be_s32 count;           // +128
};
static_assert(offsetof(HkProps, count) == 128);
// The low word of property `key` of the block at guest address props, 0
// when absent (count and table re-read every step; unchecked).
inline u32 HkPropLo(u32 props, u32 key) {
    const HkProps& p = gm<HkProps>(props);
    for (s32 j = 0; j < p.count; ++j) {
        const HkPropEntry& e = gm<HkPropEntry>(p.table.ea + 16 * static_cast<u32>(j));
        if (e.key == key) return e.lo;
    }
    return 0;
}
// A collidable (hkpCdBody / shape holder) as the ray and cast filters read
// it: +16 the s8 offset to its owner block, +24 the s8 kind (1: an entity
// with a property block, 2: a phantom), +28 the shape type (low 5 bits).
struct HkCollidable {
    pad<16> _0;
    s8 owner_off;  // +16
    pad<7> _17;
    s8 kind;       // +24
    pad<3> _25;
    be_u32 type;   // +28
};
static_assert(offsetof(HkCollidable, kind) == 24 && offsetof(HkCollidable, type) == 28);

// ---------------------------------------------------------------- AVL helpers
// The node's height and balance fixed from its children's heights (a null
// child counts 0): height = max + 1, balance = right - left.
template <typename N>
void AvlFix(N* x) {
    s32 hl = 0, hr = 0;
    if (N* l = x->left) hl = l->height;
    if (N* r = x->right) hr = r->height;
    const s32 m = hl > hr ? hl : hr;
    x->height = static_cast<s16>(m + 1);
    x->balance = static_cast<s16>(hr - hl);
}

// A per-type free list of AVL nodes, made on first use: Mem_Alloc(60, 0,
// 16), its fields cleared, vtable / name set, FreeList_Init(p, 20, 64, 0,
// 0). The global is stored even when the allocation failed (0, then used),
// as the original does.
struct FreeList {
    be_u32 vtable;      // +0
    be_u32 w[12];       // +4 .. +48
    be_u16 h52;         // +52
    be_u16 flags;       // +54: bit 15 cleared on construction
    be_u32 name;        // +56
};
static_assert(sizeof(FreeList) == 60);
inline FreeList* AvlPool(u32 pool_ea, u32 pool_vt, u32 pool_name) {
    gp<FreeList>& slot = global<gp<FreeList>>(pool_ea);
    FreeList* pool = slot;
    if (pool == nullptr) {
        FreeList* p = call<FreeList*>(fn::Mem_Alloc, 60u, 0u, 16u);
        if (p != nullptr) {
            for (u32 k : {1u, 0u, 2u, 4u, 3u, 5u}) p->w[k] = 0u;  // +8 +4 +12 +20 +16 +24
            const u16 f = p->flags;
            for (u32 k : {7u, 6u, 8u, 9u, 10u, 11u}) p->w[k] = 0u;  // +32 +28 +36 +40 +44 +48
            p->h52 = 0;
            p->flags = static_cast<u16>(f & 0x7FFFu);
            p->vtable = pool_vt;
            p->name = pool_name;
            call(fn::FreeList_Init, p, 20u, 64u, 0u, 0u);
            pool = p;
        }
        slot = pool;
    }
    return pool;
}

// ---------------------------------------------------------------- script nodes
// A script node's parameter k: the node's s16 indices +4 (a) and +2 (b)
// pick 128-byte rows of the table at *0x00F65038 (+24; -1: no row, address
// 0). Row b's selector bytes sit at +*(u16*)0x0106CF7E when its byte +2 is
// non-zero (else at address 0); selector k, times 4, plus the u16 at
// 0x0106CF70 indexes row a. Returns the parameter's guest address.
struct ScriptNodeHead {
    pad<2> _0;
    be_s16 b;  // +2
    be_s16 a;  // +4
};
inline u32 NodeParamAddr(u32 node, u32 k) {
    const u32 t = global<be_u32>(0x00F65038);
    const ScriptNodeHead& nh = gm<ScriptNodeHead>(node);
    const s32 a = nh.a, b = nh.b;
    const u32 rows = gm<be_u32>(t + 24);
    const u32 rowA = a != -1 ? gm<be_u32>(t + 24) + (static_cast<u32>(a) << 7) : 0u;
    const u32 rowB = b != -1 ? rows + (static_cast<u32>(b) << 7) : 0u;
    const u32 sel = gm<u8>(rowB + 2) != 0 ? rowB + global<be_u16>(0x0106CF7E) : 0u;
    return rowA + global<be_u16>(0x0106CF70) + (static_cast<u32>(gm<u8>(sel + k)) << 2);
}

}  // namespace sly4::obj
