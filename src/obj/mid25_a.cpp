// Game code, medium sweep part 25: the world-object destructor family whose
// base part sets vtable 0x00B1F678 (the 0x0036xxxx-0x0038xxxx scripted
// objects): each unregisters o from its world lists, resets its components
// and frees its own arrays, then runs the shared base tail.
#include "mid25.h"

using namespace sly4;
using namespace sly4::mid25;

namespace {

// World_ListRemove_0046B7A4(*0x00F65CA0, kind, tbl, o, 0).
void Unlist(PPUContext& ctx, u32 o, u32 kind, u32 tbl) {
    call(ctx, m25fn::World_ListRemove, {load<u32>(kWorld25), kind, tbl, o, 0});
}

// The component o->+24 (when set) gets c->vt[40](c, 0, 1).
void Reset24(PPUContext& ctx, u32 o) {
    const u32 c = load<u32>(o + 24);
    if (c != 0) VCall(ctx, c, 40, {c, 0, 1});
}

// For i < *cnt (re-read after each call): e = the word at
// *arr + i * stride + off; a set e gets e->vt[88](e, 0).
void Notify88(PPUContext& ctx, u32 cnt, u32 arr, u32 stride, u32 off) {
    u32 n = load<u32>(cnt);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(arr) + i * stride + off);
        if (e == 0) continue;
        VCall(ctx, e, 88, {e, 0});
        n = load<u32>(cnt);
    }
}

// Mem_FreeFrom(*slot, 0) and *slot = 0 when the slot is set.
void FreeSlot(PPUContext& ctx, u32 slot) {
    const u32 p = load<u32>(slot);
    if (p == 0) return;
    call(ctx, m25fn::Mem_FreeFrom, {p, 0});
    store<u32>(slot, 0);
}

// PtrVector_Clear(v); Nop(v).
void ClearVec(PPUContext& ctx, u32 v) {
    call(ctx, m25fn::PtrVector_Clear, {v});
    call(ctx, m25fn::Nop, {v});
}

// The six words o + base .. o + base + 20: a set one gets e->vt[slot](...)
// (slot 8: (e); slot 40: (e, 0, 1)), and is cleared afterwards with `clear`.
void Each6(PPUContext& ctx, u32 o, u32 base, u32 slot, bool clear) {
    for (u32 i = 0; i < 6; ++i) {
        const u32 a = o + base + 4 * i;
        const u32 e = load<u32>(a);
        if (e == 0) continue;
        if (slot == 40) VCall(ctx, e, 40, {e, 0, 1});
        else VCall(ctx, e, slot, {e});
        if (clear) store<u32>(a, 0);
    }
}

// The base destructor tail: vtable 0x00B1F678; the effect o->+20 gets
// Effect_Release(e, 0x00010CBB) and bit 0x200 of its +136; the component
// c = o->+24 (its +276 / +280 cleared when +276 is 0x7DF841DC) gets
// c->vt[40](c, 0, 1) and o->+24->vt[8](); q = o->+28 gets q->vt[4](q) and
// the word is cleared; then VtB21B58_DtorTrivial_004A0F70(o + 48) (r3 = its).
void BaseTail(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B1F678u);
    if (load<u32>(o + 20) != 0) {
        call(ctx, m25fn::Effect_Release, {load<u32>(o + 20), 0x00010CBBu});
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
    call(ctx, m25fn::DtorTrivial_B21B58, {o + 48});
}

}  // namespace

// The base destructor (tail of 0x00381320 and of the vtables 0x00B1F6E0 /
// 0x00B1FA48), void f(T* o): vtable 0x00B1F6B0, unlisted (8, 0x00F37878);
// o->+24->vt[40](.., 0, 1); with def = o->+8 and n = def->+152 entries, the
// 16-byte records of o->+328 notify their object (+4) by vt[88](e, 0);
// Effect_SetHash4CF988(o->+20, 0x90C77BBD) when the effect is set; then
// Obj_Set4E3168("Scale", e) for every record object and, for the entries
// with bit 0x80000000 of def->+148 [32 * i + 28], the record object gets
// e->vt[4](e); o->+328 is freed (Mem_FreeFrom, cleared); base tail.
DECOMP(0036ABF8, Obj_BaseDtor_0036ABF8) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F6B0u);
    Unlist(ctx, o, 8, 0x00F37878u);
    Reset24(ctx, o);
    const u32 def = load<u32>(o + 8);
    Notify88(ctx, def + 152, o + 328, 16, 4);
    if (const u32 e = load<u32>(o + 20); e != 0) call(ctx, m25fn::Effect_SetHash, {e, 0x90C77BBDu});
    const u32 d = load<u32>(o + 8);
    u32 n = load<u32>(d + 152);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(o + 328) + 16 * i + 4);
        if (e == 0) continue;
        call(ctx, m25fn::Obj_Set4E3168, {0x00ADCAA8u, e});  // "Scale"
        n = load<u32>(d + 152);
    }
    for (u32 i = 0; i < n; ++i) {
        if ((load<u32>(load<u32>(d + 148) + 32 * i + 28) & 0x80000000u) == 0) continue;
        const u32 e = load<u32>(load<u32>(o + 328) + 16 * i + 4);
        if (e == 0) continue;
        VCall(ctx, e, 4, {e});
        n = load<u32>(d + 152);
    }
    FreeSlot(ctx, o + 328);
    BaseTail(ctx, o);
}

// T::~T(o) (Delete_00370498, 224 bytes): vtable 0x00B1F838, unlisted
// (7, 0x00F379E0) and (8, 0x00F379E8); o->+24->vt[40](.., 0, 1); the
// def's 16-byte entries (o->+8: array +148, count +152) notify their
// object (+4) by vt[88](e, 0); o->+200 freed; the vector o + 164 cleared;
// base tail.
DECOMP(00370220, Obj_Dtor_B1F838_00370220) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F838u);
    Unlist(ctx, o, 7, 0x00F379E0u);
    Unlist(ctx, o, 8, 0x00F379E8u);
    Reset24(ctx, o);
    const u32 def = load<u32>(o + 8);
    Notify88(ctx, def + 152, def + 148, 16, 4);
    FreeSlot(ctx, o + 200);
    ClearVec(ctx, o + 164);
    BaseTail(ctx, o);
}

// T::~T(o) (Delete_00371304, 624 bytes): vtable 0x00B1F870; o->+344 gets
// vt[4]() and is cleared; unlisted (6, 0x00F37A28), (8, 0x00F37A30),
// (0, 0x00F37A20); o->+24->vt[40](.., 0, 1); when the u16 o->+356 is below
// def->+152 (def = o->+8), the def's 16-byte entry of that index (+148)
// holds a list (+4) of +8 16-byte records whose object (+4) gets
// vt[88](e, 0); then for each of the def->+152 words of o->+164 a set one
// is freed (Mem_FreeFrom, cleared), o->+164 is freed, o->+172 gets
// vt[8]() and is cleared; base tail.
DECOMP(00370F70, Obj_Dtor_B1F870_00370F70) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F870u);
    if (const u32 a = load<u32>(o + 344); a != 0) {
        VCall(ctx, a, 4, {a});
        store<u32>(o + 344, 0);
    }
    Unlist(ctx, o, 6, 0x00F37A28u);
    Unlist(ctx, o, 8, 0x00F37A30u);
    Unlist(ctx, o, 0, 0x00F37A20u);
    Reset24(ctx, o);
    const u32 def = load<u32>(o + 8);
    if (load<u16>(o + 356) < load<u32>(def + 152)) {
        auto ent = [&] { return load<u32>(def + 148) + 16 * load<u16>(o + 356); };
        u32 en = ent();
        for (u32 j = 0; j < load<u32>(en + 8); ++j) {
            const u32 e = load<u32>(load<u32>(en + 4) + 16 * j + 4);
            if (e != 0) {
                VCall(ctx, e, 88, {e, 0});
                en = ent();
            }
        }
    }
    const u32 d = load<u32>(o + 8);
    u32 n = load<u32>(d + 152);
    for (u32 i = 0; i < n; ++i) {
        const u32 s = load<u32>(o + 164) + 4 * i;
        const u32 p = load<u32>(s);
        if (p == 0) continue;
        call(ctx, m25fn::Mem_FreeFrom, {p, 0});
        store<u32>(s, 0);
        n = load<u32>(d + 152);
    }
    FreeSlot(ctx, o + 164);
    if (const u32 b = load<u32>(o + 172); b != 0) {
        VCall(ctx, b, 8, {b});
        store<u32>(o + 172, 0);
    }
    BaseTail(ctx, o);
}

// T::~T(o) (Delete_00374A8C, 240 bytes): vtable 0x00B1F8A8; o->+220 freed;
// unlisted (4, 0x00F37A70), (0, 0x00F37A78), (8, 0x00F37A68);
// o->+24->vt[40](.., 0, 1); the def's 28-byte entries (o->+8: array +196,
// count +200) notify their object (+12) by vt[88](e, 0); o->+208 freed;
// base tail.
DECOMP(003747F4, Obj_Dtor_B1F8A8_003747F4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F8A8u);
    FreeSlot(ctx, o + 220);
    Unlist(ctx, o, 4, 0x00F37A70u);
    Unlist(ctx, o, 0, 0x00F37A78u);
    Unlist(ctx, o, 8, 0x00F37A68u);
    Reset24(ctx, o);
    const u32 def = load<u32>(o + 8);
    Notify88(ctx, def + 200, def + 196, 28, 12);
    FreeSlot(ctx, o + 208);
    BaseTail(ctx, o);
}

// T::~T(o) (Delete_0037726C, 432 bytes): vtable 0x00B1F8E0, unlisted
// (8, 0x00F37AB0) and (3, 0x00F37AB8); 0x00376C98(o, 0); o->+232 freed;
// the u16 o->+296 words of o->+288 and the six words o + 328.. get
// vt[8]() and are cleared; the vectors o + 400, o + 308, o + 288 cleared;
// base tail (no o->+24 reset before it in this one).
DECOMP(00376F88, Obj_Dtor_B1F8E0_00376F88) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F8E0u);
    Unlist(ctx, o, 8, 0x00F37AB0u);
    Unlist(ctx, o, 3, 0x00F37AB8u);
    call(ctx, 0x00376C98u, {o, 0});
    FreeSlot(ctx, o + 232);
    for (u32 i = 0; i < load<u16>(o + 296); ++i) {
        const u32 e = load<u32>(load<u32>(o + 288) + 4 * i);
        if (e == 0) continue;
        VCall(ctx, e, 8, {e});
        store<u32>(load<u32>(o + 288) + 4 * i, 0);
    }
    Each6(ctx, o, 328, 8, true);
    ClearVec(ctx, o + 400);
    ClearVec(ctx, o + 308);
    ClearVec(ctx, o + 288);
    BaseTail(ctx, o);
}

// T::~T(o) (Delete_0037F150, 336 bytes): vtable 0x00B1F950, unlisted
// (8, 0x00F37B30) and (3, 0x00F37B38); o->+24->vt[40](.., 0, 1); the def's
// 28-byte entries (o->+8: array +180, count +184) notify their object
// (+12) by vt[88](e, 0); the six words o + 280.. get vt[40](e, 0, 1);
// o->+232 freed; the six words get vt[8]() (not cleared); the vector
// o + 304 cleared; base tail.
DECOMP(0037EE20, Obj_Dtor_B1F950_0037EE20) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F950u);
    Unlist(ctx, o, 8, 0x00F37B30u);
    Unlist(ctx, o, 3, 0x00F37B38u);
    Reset24(ctx, o);
    const u32 def = load<u32>(o + 8);
    Notify88(ctx, def + 184, def + 180, 28, 12);
    Each6(ctx, o, 280, 40, false);
    FreeSlot(ctx, o + 232);
    Each6(ctx, o, 280, 8, false);
    ClearVec(ctx, o + 304);
    BaseTail(ctx, o);
}
