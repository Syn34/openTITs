// Game code, medium sweep part 4: helpers shared by mid4_*.cpp (still
// recompiled game functions of 120-400 instructions, in coherent groups:
// the last tuning-variable statics, more action-script node factories,
// XML readers and pairs / quads of template instances).
#pragma once

#include <bit>
#include <cmath>

#include "mid3.h"

namespace sly4 {
namespace mid4 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using mid3::PoolArrayDtor;
using mid3::TvBool;
using mid3::TvColor;
using mid3::TvDtor;
using mid3::TvFloat;
using mid3::TvHash;
using mid3::TvInt;
using mid3::TvNew;
using mid3::TvString;
using mid3::TvVec3;
using mid3::kVtBool;
using mid3::kVtColor;
using mid3::kVtFloat;
using mid3::kVtHash;
using mid3::kVtInt;
using mid3::kVtString;
using mid3::kVtVec2;
using mid3::kVtVec3;
using obj19::VCall;
using obj19::VecDtor;

namespace m4fn {
constexpr u32 Mem_Alloc = 0x0054ED38;            // (size, pool, align)
constexpr u32 memset = 0x00726E54;               // (p, c, n) -> p
constexpr u32 String_CopyN = 0x00553638;         // (dst, src, n)
constexpr u32 String_HashNoCase = 0x0056A710;    // (s) -> u32
constexpr u32 PtrVector_Clear = 0x0054EC1C;      // (vec)
constexpr u32 Nop = 0x0054EC18;                  // (p)
constexpr u32 SmallFree = 0x00552420;            // (p, size)
}  // namespace m4fn

// The inlined ~T[n]() of an array of n tuning vars of `stride` bytes at
// base: from the last element down (the vtable back, then the base dtor).
inline void TvArrayDtor(PPUContext& ctx, u32 base, u32 n, u32 stride, u32 vt) {
    for (u32 k = n; k-- > 0;) TvDtor(ctx, base + k * stride, vt);
}

// An f32 global copied with lfs f1 / stfs (the word as is): f1 is left
// holding it, as in the original.
inline void CopyF32(PPUContext& ctx, u32 dst, u32 src) {
    const u32 w = load<u32>(src);
    store<u32>(dst, w);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w));
}

// The inlined constructor of a sorted pool array global {+0 u16 count, +2
// / +4 u16 0, +6 u16 (as PoolArrayDtor: the element size << 1), +8 data,
// +12 allocator, +16 0, +20 the compare function's OPD}: the default
// allocator; for a non-zero count the data from its vfn[0](a, count *
// ((+6 >> 1) & 0x7FFF), 0, 16); then Array_Init. The order of the +16 /
// +20 stores and of the OPD / entry left in r7 / r8 differ between the
// inlined copies (cmp_first: +20 first; opd_r8: r7 = entry, r8 = OPD).
inline void SortedArrayCtor(PPUContext& ctx, u32 a, u16 n, u16 esz, u32 cmp, bool cmp_first, bool opd_r8) {
    store<u16>(a, n);
    store<u16>(a + 2, 0);
    store<u16>(a + 4, 0);
    store<u16>(a + 6, esz);
    store<u32>(a + 8, 0);
    const u32 al = static_cast<u32>(call(ctx, mid3::m3fn::Mem_DefaultAllocator, {}));
    const u32 cnt = load<u16>(a);
    store<u32>(a + 12, al);
    if (cnt != 0) {
        const u32 bytes = cnt * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
        const u32 opd = load<u32>(load<u32>(al));
        const u32 entry = load<u32>(opd);
        const u64 p = opd_r8 ? call_ptr(ctx, opd, {al, bytes, 0, 16, entry, opd}) : call_ptr(ctx, opd, {al, bytes, 0, 16, opd, entry});
        store<u32>(a + 8, static_cast<u32>(p));
    }
    call(ctx, mid3::m3fn::Array_Init, {a});
    if (cmp_first) {
        store<u32>(a + 20, cmp);
        store<u32>(a + 16, 0);
    } else {
        store<u32>(a + 16, 0);
        store<u32>(a + 20, cmp);
    }
}

// The inlined ~PtrVector[n]() of n 20-byte vectors at base, last first.
inline void VecArrayDtor(PPUContext& ctx, u32 base, u32 n) {
    for (u32 k = n; k-- > 0;) VecDtor(ctx, base + 20 * k);
}

// ---------------------------------------------------------------- action-script nodes

// As mid3_node.cpp's NewNamedNode (the event / condition node factories),
// with up to six default names per enum parameter.
namespace nfn {
constexpr u32 Node_Init = 0x00391694;        // (p, s16 id, const char* name)
constexpr u32 Cond_Init = 0x0038E5DC;        // (p, s16 id, const char* name, 1)
constexpr u32 Node_Bind = 0x004D3088;        // (owner, const char* name, kind, k6, -1, 0) -> binding
constexpr u32 Binding_Reserve = 0x004D2BB8;  // (b, n): +72 = n, +68 = the name array
constexpr u32 kNodeTable = 0x00F65038;       // -> +24 the 128-byte records
constexpr u32 kFlagOffset = 0x0106CF82;      // u16 added to the node's +20
constexpr u32 kCondFlagOffset = 0x0106CF7A;  // u16: the same for condition nodes
}  // namespace nfn

// A parameter: its name, the Node_Bind kind / k6, the default value word
// stored in the binding's +76, its slot in the node's flag bytes, and for
// an enum parameter the size of its name list and its default names.
struct NodeParam {
    u32 name, kind, k6, init, slot;
    u32 reserve = 0;
    u32 strs[6] = {};
};

// A name appended to binding b's list {+0 u32 count, +68 char*[], +72 u32
// capacity} when there is room: a 100-byte Mem_Alloc'd copy (String_CopyN).
inline void NodeBindingAddName(PPUContext& ctx, u32 b, u32 s) {
    if (!(load<u32>(b) < load<u32>(b + 72))) return;
    const u32 m = static_cast<u32>(call(ctx, m4fn::Mem_Alloc, {100, 0, 16}));
    store<u32>(load<u32>(b + 68) + ((load<u32>(b) << 2) & 0xFFFFFFFCu), m);
    call(ctx, m4fn::String_CopyN, {load<u32>(load<u32>(b + 68) + ((load<u32>(b) << 2) & 0xFFFFFFFCu)), s, 100});
    store<u32>(b, load<u32>(b) + 1);
}

// Node* New(s16 id): Mem_Alloc(24, 0, 16) (0 on failure); Node_Init(p, id,
// name) (cond: Cond_Init(p, id, name, 1)); the u16 rec stored at the
// node's record (kNodeTable->+24 + index * 128, the s16 index at +16; at
// address 0 for index -1, as the original); then per parameter
// Node_Bind(p->+12, name, kind, k6, -1, 0), the binding's +76 = init and
// its +64 copied as a byte to (p->+20 + flag offset)[slot]; for an enum
// parameter Binding_Reserve(b, reserve) and its default names. Returns p
// (r4 = the last +64, as left).
inline void NewNamedNode(PPUContext& ctx, u32 name, u16 rec, std::initializer_list<NodeParam> params, bool cond = false) {
    const u64 id = ctx.gpr[3];
    const u32 p = static_cast<u32>(call(ctx, m4fn::Mem_Alloc, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u64 sid = static_cast<u64>(static_cast<s64>(static_cast<s16>(id)));
    if (cond) call(ctx, nfn::Cond_Init, {p, sid, name, 1});
    else call(ctx, nfn::Node_Init, {p, sid, name});
    const s32 idx = static_cast<s16>(load<u16>(p + 16));
    const u32 tbl = load<u32>(nfn::kNodeTable);
    const u32 r = idx == -1 ? 0 : load<u32>(tbl + 24) + (static_cast<u32>(idx) << 7);
    store<u16>(r, rec);
    const u32 flags = load<u32>(p + 20) + load<u16>(cond ? nfn::kCondFlagOffset : nfn::kFlagOffset);
    for (const NodeParam& q : params) {
        const u32 b = static_cast<u32>(call(ctx, nfn::Node_Bind, {load<u32>(p + 12), q.name, q.kind, q.k6, ~0ull, 0}));
        const u32 v = load<u32>(b + 64);
        store<u32>(b + 76, q.init);
        store<u8>(flags + q.slot, static_cast<u8>(v));
        ctx.gpr[4] = v;
        if (q.reserve == 0) continue;
        call(ctx, nfn::Binding_Reserve, {b, q.reserve});
        for (const u32 str : q.strs)
            if (str != 0) NodeBindingAddName(ctx, b, str);
    }
    ret(ctx, p);
}

}  // namespace mid4
}  // namespace sly4
