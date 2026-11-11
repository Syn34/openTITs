// Game code, medium sweep part 26: helpers shared by mid26_*.cpp (more
// still-recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid25.h"

namespace sly4 {
namespace mid26 {

using namespace opentit;
using namespace opentit::decomp;
using mid14::Lf;
using mid6::F32;
using mid6::W32;
using obj19::VCall;

namespace m26fn {
using namespace mid25::m25fn;
constexpr u32 Obj_FindByHash = 0x000DB290u;     // Obj_FindByHash_000DB290(o, hash) -> child or 0
constexpr u32 Obj_PopLayer = 0x000FBB88u;       // Obj_PopLayer(o, animator; f1 t)
constexpr u32 SlotRec_Flag40 = 0x002DA89Cu;     // SlotRec_Flag40_002DA89C(obj) -> bool
constexpr u32 CurPlayerKey = 0x002E23ACu;       // f_002E23AC(session) -> the current player key
constexpr u32 LevelKeyByte = 0x002E1B1Cu;       // Player_LevelKeyByte_002E1B1C(session, hash) -> u8
constexpr u32 HandleNotify = 0x0036742Cu;       // Handle_Notify36742C(handle, a, b, c)
constexpr u32 SyncBindings = 0x003BB8ACu;       // Obj_SyncBindingPoses(o, mode, skip)
constexpr u32 AnimRestart = 0x002775A8u;        // Anim_Restart2775A8
constexpr u32 HashNoCase = 0x0056A710u;         // String_HashNoCase(s)
constexpr u32 SortedMap_Remove7330C0 = 0x007330C0u;  // (map, key ptr)
}  // namespace m26fn

constexpr u32 kSession26 = 0x00F64C20u;  // Session*

// ---- world-object destructor family (base vtable 0x00B1F678), as mid25_a.cpp

// World_ListRemove_0046B7A4(*0x00F65CA0, kind, tbl, o, 0).
inline void Unlist26(PPUContext& ctx, u32 o, u32 kind, u32 tbl) {
    call(ctx, m26fn::World_ListRemove, {load<u32>(mid25::kWorld25), kind, tbl, o, 0});
}

// e->vt[40](e, on, 1) when e is set. Returns r3 as the original leaves it
// (the call's result, or 0 = the word tested).
inline u32 Vt40(PPUContext& ctx, u32 e, u32 on) { return e != 0 ? static_cast<u32>(VCall(ctx, e, 40, {e, on, 1})) : 0; }

// For i < n and each base in turn: Vt40(o + base + 4 * i, on). Returns the last r3.
inline u32 Vt40Each(PPUContext& ctx, u32 o, std::initializer_list<u32> bases, u32 n, u32 on) {
    u32 r = 0;
    for (u32 i = 0; i < n; ++i)
        for (u32 b : bases) r = Vt40(ctx, load<u32>(o + b + 4 * i), on);
    return r;
}

// e = *slot: e->vt[8](e) when set and, with `clear`, *slot = 0.
inline void Vt8Slot(PPUContext& ctx, u32 slot, bool clear) {
    const u32 e = load<u32>(slot);
    if (e == 0) return;
    VCall(ctx, e, 8, {e});
    if (clear) store<u32>(slot, 0);
}

// For i < n and each base in turn: Vt8Slot(o + base + 4 * i, clear).
inline void Vt8Each(PPUContext& ctx, u32 o, std::initializer_list<u32> bases, u32 n, bool clear) {
    for (u32 i = 0; i < n; ++i)
        for (u32 b : bases) Vt8Slot(ctx, o + b + 4 * i, clear);
}

// For i < *cnt (re-read after each call): e = the word at *arr + i * stride
// + off; a set e gets e->vt[88](e, on).
inline void Notify88On(PPUContext& ctx, u32 cnt, u32 arr, u32 stride, u32 off, u32 on) {
    u32 n = load<u32>(cnt);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(arr) + i * stride + off);
        if (e == 0) continue;
        VCall(ctx, e, 88, {e, on});
        n = load<u32>(cnt);
    }
}

// Mem_FreeFrom(*slot, 0) and *slot = 0 when the slot is set.
inline void FreeSlot26(PPUContext& ctx, u32 slot) {
    const u32 p = load<u32>(slot);
    if (p == 0) return;
    call(ctx, m26fn::Mem_FreeFrom, {p, 0});
    store<u32>(slot, 0);
}

// The base destructor tail: vtable 0x00B1F678; the effect o->+20 gets
// Effect_Release(e, 0x00010CBB) and bit 0x200 of its +136; the component
// c = o->+24 (its +276 / +280 cleared when +276 is 0x7DF841DC) gets
// c->vt[40](c, 0, 1) and o->+24->vt[8](); q = o->+28 gets q->vt[4](q) and
// the word is cleared; then VtB21B58_DtorTrivial_004A0F70(o + 48) (r3 = its).
inline void BaseTail26(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B1F678u);
    if (load<u32>(o + 20) != 0) {
        call(ctx, m26fn::Effect_Release, {load<u32>(o + 20), 0x00010CBBu});
        const u32 e = load<u32>(o + 20);
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
    }
    if (const u32 c = load<u32>(o + 24); c != 0) {
        if (load<u32>(c + 276) == 0x7DF841DCu) {
            store<u32>(c + 276, 0);
            store<u32>(c + 280, 0);
        }
        VCall(ctx, c, 40, {c, 0, 1});
        const u32 c2 = load<u32>(o + 24);
        VCall(ctx, c2, 8, {c2});
    }
    if (const u32 q = load<u32>(o + 28); q != 0) {
        VCall(ctx, q, 4, {q});
        store<u32>(o + 28, 0);
    }
    call(ctx, m26fn::DtorTrivial_B21B58, {o + 48});
}

// Removes o from the registry array 0x01056F1C ({key, T*} pairs, the later
// ones moved down; the count and data read once for the search, the count
// re-read after each move).
inline void RegistryRemove26(u32 o) {
    const u32 ar = 0x01056F1Cu;
    const u32 cnt = load<u16>(ar + 2);
    const u32 data = load<u32>(ar + 8);
    for (u32 i = 0; i < cnt; ++i) {
        if (load<u32>(data + 8 * i + 4) != o) continue;
        u32 c = cnt;
        if (static_cast<s32>(c) >= 2 && static_cast<s32>(i) < static_cast<s32>(c - 1)) {
            for (u32 j = i;;) {
                const u32 e = data + 8 * j;
                ++j;
                store<u32>(e, load<u32>(e + 8));
                store<u32>(e + 4, load<u32>(e + 12));
                c = load<u16>(ar + 2);
                if (!(static_cast<s32>(j) < static_cast<s32>(c - 1))) break;
            }
        }
        store<u16>(ar + 2, static_cast<u16>(c - 1));
        return;
    }
}

// As RegistryRemove26, but the search re-reads the count and the data
// pointer each step (the moves use the data pointer read last).
inline void RegistryRemoveB26(u32 o) {
    const u32 ar = 0x01056F1Cu;
    for (u32 i = 0;; ++i) {
        const u32 cnt = load<u16>(ar + 2);
        if (i >= cnt) return;
        const u32 data = load<u32>(ar + 8);
        if (load<u32>(data + 8 * i + 4) != o) continue;
        u32 c = cnt;
        if (static_cast<s32>(c) >= 2 && static_cast<s32>(i) < static_cast<s32>(c - 1)) {
            for (u32 j = i;;) {
                const u32 e = data + 8 * j;
                ++j;
                store<u32>(e, load<u32>(e + 8));
                store<u32>(e + 4, load<u32>(e + 12));
                c = load<u16>(ar + 2);
                if (!(static_cast<s32>(j) < static_cast<s32>(c - 1))) break;
            }
        }
        store<u16>(ar + 2, static_cast<u16>(c - 1));
        return;
    }
}

}  // namespace mid26
}  // namespace sly4
