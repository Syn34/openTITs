// Game code, medium sweep part 27: more destructors (registered objects on
// the 0x00B1BE30 base, the 0x00B1E1F8 section base, Havok-array owners)
// and an object shutdown method.
#include "mid27.h"

using namespace sly4;
using namespace sly4::mid27;

namespace {
using namespace m27fn;
constexpr u32 kDispatch362 = 0x0036325Cu;   // Obj_Dispatch362(o, a, b, c)
constexpr u32 kMapRegister = 0x00362BECu;   // Map_Register_00362BEC(o)
constexpr u32 kEffDelete = 0x004D0280u;     // Effect_Delete(e)
constexpr u32 kDtorB219D0 = 0x00464AD0u;    // Dtor_B219D0_00464AD0(p)
constexpr u32 kSkelRel = 0x007D4EA8u;       // AnimSkel_Release7D4EA8(p)
constexpr u32 kMemFreeFrom = 0x0054EE6Cu;   // Mem_FreeFrom(p, 0)
constexpr u32 kRemoveEntity = 0x0092FFC8u;  // Hk_RemoveEntity92FFC8(result*, world, entity)
constexpr u32 kRemoveProperty = 0x0093FED8u; // Hk_RemoveProperty93FED8(result*, entity, key)

// The PtrVector at *slot (20 bytes): destroyed and freed (SmallFree(*slot,
// 20), the slot re-read) when set.
void FreePtrVec27(PPUContext& ctx, u32 slot) {
    const u32 p = load<u32>(slot);
    if (p == 0) return;
    PtrVecDtor27(ctx, p);
    call(ctx, SmallFree27, {load<u32>(slot), 20});
}

// The 0x00B1E1F8 section base part (as TailB1E1F8 in mid26_d.cpp, the
// entries going through Ptr_FreeAndClear_00321240): vtable 0x00B1E1F8; the
// counted array +2544 (count at -4, 8-byte entries, from the last) and the
// five vectors of vectors +2232 (stride 8) freed; +2524 / +2512 / +2500 /
// +2488 freed; the vector +2272 destroyed and freed;
// Registry_Unlink2_0048F6B0(o) (r3 = its).
void SectionBase27(PPUContext& ctx, u32 o) {
    const u32 arr = load<u32>(o + 2544);
    store<u32>(o, 0x00B1E1F8u);
    if (arr != 0) {
        const u32 hdr = arr - 4;
        for (u32 i = load<u32>(hdr) - 1; static_cast<s32>(i) >= 0; --i) call(ctx, 0x00321240u, {arr + 8 * i});
        call(ctx, kMemFreeFrom, {hdr, 0});
        store<u32>(o + 2544, 0);
    }
    for (u32 k = 0; k < 5; ++k) {
        const u32 slot = o + 2232 + 8 * k;
        u32 v = load<u32>(slot);
        if (v == 0) continue;
        for (u32 j = 0;; ++j) {
            const u32 pv = load<u32>(v);  // read even when the slot became 0 (quirk)
            if (j >= (pv != 0 ? static_cast<u32>(load<u16>(pv + 8)) : 0u)) break;
            const u32 e = load<u32>(load<u32>(load<u32>(v)) + 4 * j);
            if (e == 0) continue;
            PtrVecDtor27(ctx, e);
            call(ctx, kMemFreeFrom, {e, 0});
            v = load<u32>(slot);
        }
        if (v == 0) continue;
        FreePtrVec27(ctx, v);
        call(ctx, kMemFreeFrom, {v, 0});
        store<u32>(slot, 0);
    }
    for (u32 off : {2524u, 2512u, 2500u, 2488u}) FreeSlot26(ctx, o + off);
    FreePtrVec27(ctx, o + 2272);
    call(ctx, 0x0048F6B0u, {o});  // Registry_Unlink2_0048F6B0
}

// The slot's object x: Map_Register_00362BEC(x) when x->+160 >> 1 is 0 or
// o->+52 has bit 0x08000000, else Obj_Dispatch362(x, 0, 1, 1); then
// x->vt[20](x, 0).
void DropMapped(PPUContext& ctx, u32 o, u32 x) {
    if ((load<u32>(x + 160) >> 1) == 0 || (load<u32>(o + 52) & 0x08000000u) != 0) call(ctx, kMapRegister, {x});
    else call(ctx, kDispatch362, {x, 0, 1, 1});
    VCall(ctx, x, 20, {x, 0});
}
}  // namespace

// T::~T(o) (vtable 0x00B1C788; Delete_00291480): Obj_SetField20_00276558
// (o, 0), f_00275110(o), List1013B70_Remove(o); the members +60 / +76 /
// +92 / +108 / +124 / +140 / +40 get vt[8] (not cleared); base part (the
// registry search re-reading count and data).
DECOMP(00291228, Obj_Dtor_B1C788_00291228) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1C788u);
    call(ctx, 0x00276558u, {o, 0});
    call(ctx, 0x00275110u, {o});
    call(ctx, 0x000D69B4u, {o});
    for (u32 off : {60u, 76u, 92u, 108u, 124u, 140u, 40u}) Vt8Slot(ctx, o + off, false);
    RegBase27(ctx, o, true);
}

// T::~T(o) (vtable 0x00B1CF28; Delete_002BABC4): the mapped objects +36 /
// +40 are dropped (DropMapped; +36 read before the vtable store) and both
// cleared; +44 / +48 get vt[40](e, 0) (cleared); World_ListRemove(
// *0x00F65CA0, 6, 0x00F357E0, o + 64 / o + 72, 0); +68 / +76 vt[8]
// (cleared); base part (the registry search re-reading count and data).
DECOMP(002BA884, Obj_Dtor_B1CF28_002BA884) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 36);
    store<u32>(o, 0x00B1CF28u);
    if (a != 0) DropMapped(ctx, o, a);
    if (const u32 b = load<u32>(o + 40); b != 0) DropMapped(ctx, o, b);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    for (u32 off : {44u, 48u}) {
        if (const u32 e = load<u32>(o + off); e != 0) VCall(ctx, e, 40, {e, 0});
        store<u32>(o + off, 0);
    }
    for (u32 off : {64u, 72u}) call(ctx, World_ListRemove, {load<u32>(mid25::kWorld25), 6, 0x00F357E0u, o + off, 0});
    Vt8Slot(ctx, o + 68, true);
    Vt8Slot(ctx, o + 76, true);
    RegBase27(ctx, o, true);
}

// T::~T(o) (vtables 0x00B1D070 / +24 0x00B1D0BC; Delete_002C9C50 and the
// adjustor thunks): List1013B5C_Remove(o); the effect +328 released
// (0x10CBB, bit 0x200 of its +136, cleared); +284 Effect_Delete (cleared);
// when u8 +336: f_002C5C34(o) and f_002C96A8(o; f1 = *0x002C9904); +296
// vt[40](e, 0), cleared with u8 +288 = 0; +292 leaves the list +276
// (List_RemoveAll_000D91BC, cleared); the animator +280 popped off the
// world's layer stack (f1 = *0x002C9908) and released (vt[4], cleared);
// +440 = 0, +436 / +424 get Obj_Dispatch362(p, 1, 1, 0) and are cleared;
// +276 = 0; the members +624 (vtable 0x00B1D040) and +24 destroyed
// (Dtor_B219D0_00464AD0), the PtrVector +356 between; base part.
DECOMP(002C9910, Obj_Dtor_B1D070_002C9910) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1D070u);
    store<u32>(o + 24, 0x00B1D0BCu);
    call(ctx, ListRemove, {o});
    if (const u32 fx = load<u32>(o + 328); fx != 0) {
        call(ctx, EffRelease, {fx, 0x00010CBBu});
        const u32 f2 = load<u32>(o + 328);
        store<u32>(f2 + 136, load<u32>(f2 + 136) | 0x200u);
        store<u32>(o + 328, 0);
    }
    if (const u32 e = load<u32>(o + 284); e != 0) {
        call(ctx, kEffDelete, {e});
        store<u32>(o + 284, 0);
    }
    if (load<u8>(o + 336) != 0) {
        call(ctx, 0x002C5C34u, {o});
        callf(ctx, 0x002C96A8u, {o}, {Lf(0x002C9904u)});
    }
    if (const u32 e = load<u32>(o + 296); e != 0) {
        VCall(ctx, e, 40, {e, 0});
        store<u32>(o + 296, 0);
        store<u8>(o + 288, 0);  // (cntlzw(0) >> 5) ^ 1 of the word just cleared
    }
    if (const u32 e = load<u32>(o + 292); e != 0) {
        call(ctx, 0x000D91BCu, {load<u32>(o + 276), e});
        store<u32>(o + 292, 0);
    }
    if (u32 l = load<u32>(o + 280); l != 0) {
        if (const u32 stack = load<u32>(load<u32>(mid25::kWorld25) + 0x001281E0u); stack != 0) {
            callf(ctx, Obj_PopLayer, {stack, l}, {Lf(0x002C9908u)});
            l = load<u32>(o + 280);
        }
        if (l != 0) VCall(ctx, l, 4, {l});
        store<u32>(o + 280, 0);
    }
    store<u32>(o + 440, 0);
    if (const u32 p = load<u32>(o + 436); p != 0) call(ctx, kDispatch362, {p, 1, 1, 0});
    store<u32>(o + 436, 0);
    if (const u32 p = load<u32>(o + 424); p != 0) call(ctx, kDispatch362, {p, 1, 1, 0});
    store<u32>(o + 424, 0);
    store<u32>(o + 276, 0);
    store<u32>(o + 624, 0x00B1D040u);
    call(ctx, kDtorB219D0, {o + 624});
    PtrVecDtor27(ctx, o + 356);
    call(ctx, kDtorB219D0, {o + 24});
    RegBase27(ctx, o);
}

// T::~T(o) (vtable 0x00B162C8; Delete_001BBBC8): with the entry table
// +400: +404 and +400 get vt[8] (no null checks, both cleared), each of
// the u16 +392 96-byte records at +388 has its +80 released (vt[8], no
// check) and cleared, and +388 is freed (Mem_FreeFrom); the attachment
// +384 is detached from its owner (+260 vt[24](owner, c)) unless u16 +394
// has bit 0x800, has its +276 / +280 reset to 0x02235C33 / 0 when +280
// names o, and is released (vt[8]). World_ListRemove(*0x00F65CA0, 0,
// 0x00F332F8, o, o); Thunk_000D729C(o) (r3 = its).
DECOMP(001BB9B0, Obj_Dtor_B162C8_001BB9B0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 tbl = load<u32>(o + 400);
    store<u32>(o, 0x00B162C8u);
    if (tbl != 0) {
        const u32 a = load<u32>(o + 404);
        VCall(ctx, a, 8, {a});
        store<u32>(o + 404, 0);
        const u32 b = load<u32>(o + 400);
        VCall(ctx, b, 8, {b});
        store<u32>(o + 400, 0);
        if (load<u16>(o + 392) != 0) {
            u32 i = 0;
            do {
                const u32 e = load<u32>(load<u32>(o + 388) + 96 * i + 80);
                VCall(ctx, e, 8, {e});
                store<u32>(load<u32>(o + 388) + 96 * i + 80, 0);
                ++i;
            } while (i < load<u16>(o + 392));
        }
        call(ctx, kMemFreeFrom, {load<u32>(o + 388), 0});
        u32 c = load<u32>(o + 384);
        if (c != 0) {
            if ((load<u16>(o + 394) & 0x800u) == 0) {
                if (const u32 own = load<u32>(c + 260); own != 0) {
                    VCall(ctx, own, 24, {own, c});
                    c = load<u32>(o + 384);
                }
                const u32 who = load<u32>(c + 276) == 0x02235C33u ? load<u32>(c + 280) : 0u;
                if (who == o) {
                    store<u32>(c + 280, 0);
                    store<u32>(c + 276, 0x02235C33u);
                }
            }
            VCall(ctx, c, 8, {c});
        }
    }
    call(ctx, World_ListRemove, {load<u32>(mid25::kWorld25), 0, 0x00F332F8u, o, o});
    call(ctx, 0x000D729Cu, {o});
}

// T::~T(o) (vtable 0x00B1EDE8; Delete_00336A80): the five members +2608
// get vt[40](e, 1) and are cleared; Registry_Unlink_0048F298(o, 0); when
// u8 +2980: the world's (+4216) item gets 0x000D949C, Obj_SetBit27Notify_
// 002E6E54(session, 1), Game_SetPaused(1, 0), Flagged_SetOn_0029F428(r, 0)
// with the item's result, u8 +2980 = 0; the section base part.
DECOMP(003366F0, Obj_Dtor_B1EDE8_003366F0) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1EDE8u);
    for (u32 k = 0; k < 5; ++k) {
        const u32 slot = o + 2608 + 4 * k;
        if (const u32 e = load<u32>(slot); e != 0) VCall(ctx, e, 40, {e, 1});
        store<u32>(slot, 0);
    }
    call(ctx, 0x0048F298u, {o, 0});
    if (load<u8>(o + 2980) != 0) {
        u32 r = 0;
        if (const u32 p = load<u32>(load<u32>(mid25::kWorld25) + 4208 + 8); p != 0) r = static_cast<u32>(call(ctx, 0x000D949Cu, {p}));
        call(ctx, 0x002E6E54u, {load<u32>(mid26::kSession26), 1});
        call(ctx, 0x00365A54u, {1, 0});  // Game_SetPaused
        if (r != 0) call(ctx, 0x0029F428u, {r, 0});
        store<u8>(o + 2980, 0);
    }
    SectionBase27(ctx, o);
}

// T::~T(o) (vtable 0x00B1F328; Delete_003448C0): *0x00F64C84 = 0; with
// the skeleton +4: it is released (cleared), every element of the
// nested arrays +20 (count +24) -> +12 (count +16) -> +8 (count +12) gets
// vt[0], and the +8 entries (count +12) are released from the last. The
// Havok arrays {+8, +20, +32 (pointers), +44 (Mem_FreeFrom'd entries,
// count +48), +56 (64-byte)} are emptied and their storage freed through
// the thread allocator (*0x00F64C88 = 0 between), then the inlined base
// destructors empty +44..+8 again. r3 = 0x80000000.
DECOMP(00344398, Obj_Dtor_B1F328_00344398) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 skel = load<u32>(o + 4);
    store<u32>(o, 0x00B1F328u);
    store<u32>(0x00F64C84u, 0);
    if (skel != 0) {
        call(ctx, kSkelRel, {skel});
        store<u32>(o + 4, 0);
        for (s32 i = 0; i < static_cast<s32>(load<u32>(o + 24)); ++i) {
            for (s32 j = 0;; ++j) {
                const u32 a = load<u32>(load<u32>(o + 20) + 4 * static_cast<u32>(i));
                if (!(j < static_cast<s32>(load<u32>(a + 16)))) break;
                const u32 b = load<u32>(load<u32>(a + 12) + 4 * static_cast<u32>(j));
                for (s32 k = 0; k < static_cast<s32>(load<u32>(b + 12)); ++k) {
                    const u32 e = load<u32>(load<u32>(b + 8) + 4 * static_cast<u32>(k));
                    VCall(ctx, e, 0, {e});
                }
            }
        }
        for (u32 n = load<u32>(o + 12); static_cast<s32>(n) > 0; --n) call(ctx, kSkelRel, {load<u32>(load<u32>(o + 8) + 4 * (n - 1))});
    }
    constexpr u32 kF = 0x80000000u;
    store<u32>(o + 12, 0);
    HkArrFree27(ctx, o, 8, 16, 4, true);
    store<u32>(o + 8, 0);
    store<u32>(o + 16, kF);
    store<u32>(o + 24, 0);
    HkArrFree27(ctx, o, 20, 28, 4, false);
    store<u32>(o + 20, 0);
    store<u32>(o + 28, kF);
    store<u32>(o + 36, 0);
    HkArrFree27(ctx, o, 32, 40, 4, true);
    store<u32>(o + 32, 0);
    store<u32>(o + 40, kF);
    for (s32 i = 0; i < static_cast<s32>(load<u32>(o + 48)); ++i) call(ctx, kMemFreeFrom, {load<u32>(load<u32>(o + 44) + 4 * static_cast<u32>(i)), 0});
    store<u32>(o + 48, 0);
    HkArrFree27(ctx, o, 44, 52, 4, true);
    store<u32>(o + 44, 0);
    store<u32>(o + 52, kF);
    store<u32>(0x00F64C88u, 0);
    if ((load<u32>(o + 64) & kF) == 0) {
        store<u32>(o + 60, 0);
        HkArrFree27(ctx, o, 56, 64, 64, false);
    }
    store<u32>(o + 60, 0);
    store<u32>(o + 56, 0);
    store<u32>(o + 64, kF);
    // The base destructors' copies: their arrays were all emptied above
    // (flag set), so the frees below find nothing to free.
    store<u32>(o + 48, 0);
    HkArrFree27(ctx, o, 44, 52, 4, false);
    store<u32>(o + 44, 0);
    store<u32>(o + 52, kF);
    store<u32>(o + 36, 0);
    HkArrFree27(ctx, o, 32, 40, 4, true);
    store<u32>(o + 32, 0);
    store<u32>(o + 40, kF);
    store<u32>(o + 24, 0);
    HkArrFree27(ctx, o, 20, 28, 4, true);
    store<u32>(o + 20, 0);
    store<u32>(o + 28, kF);
    store<u32>(o + 12, 0);
    HkArrFree27(ctx, o, 8, 16, 4, false);
    store<u32>(o + 8, 0);
    store<u32>(o + 16, kF);
    ctx.gpr[3] = kF;
}

// T::~T(o) (vtable 0x00B1F558; Delete_0035357C and the 0x00B1F570
// destructors): the owned Havok object +28 has its arrays +44 / +28 (4-byte)
// and +16 / +4 (48-byte) emptied and freed through the thread allocator,
// then goes back to the thread's memory router (*(r13 - 28332) +44
// vt[12](a, p, 56)) and +28 = 0; the skeleton +24 is released.
// r3: the release's, else 0 (o without +28).
DECOMP(00353334, Obj_Dtor_B1F558_00353334) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 28);
    constexpr u32 kF = 0x80000000u;
    store<u32>(o, 0x00B1F558u);
    if (p != 0) {
        store<u32>(p + 48, 0);
        HkArrFree27(ctx, p, 44, 52, 4, true);
        store<u32>(p + 44, 0);
        store<u32>(p + 52, kF);
        store<u32>(p + 32, 0);
        HkArrFree27(ctx, p, 28, 36, 4, false);
        store<u32>(p + 28, 0);
        store<u32>(p + 36, kF);
        store<u32>(p + 20, 0);
        HkArrFree27(ctx, p, 16, 24, 48, true);
        store<u32>(p + 16, 0);
        store<u32>(p + 24, kF);
        store<u32>(p + 8, 0);
        HkArrFree27(ctx, p, 4, 12, 48, true);
        store<u32>(p + 4, 0);
        store<u32>(p + 12, kF);
        const u32 a = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) - 28332u) + 44);
        VCall(ctx, a, 12, {a, p, 56});
        store<u32>(o + 28, 0);
        ctx.gpr[3] = 0;
    } else {
        ctx.gpr[3] = o;
    }
    if (const u32 q = load<u32>(o + 24); q != 0) call(ctx, kSkelRel, {q});
}

// void f(T* o, u8 on) (a shutdown method; no direct callers): when on, the
// effect +352 gets bit 0x100 of its +136, Effect_Release(e, 0x879D1B7A),
// bit 0x200 and is cleared; the skeleton +348 leaves the world (its entity
// +8, result on the stack), each bone (Skel_BoneAt_0091A7D8, count
// Skel_BoneCount_0091A2CC re-read) is released, the properties 0xA734004D /
// 0x91C7CF5B are removed when present in its 16-byte table (+124, count
// +128) and it is released. Then Obj_ShutdownBase_000D74C0(o, on).
DECOMP(000DE6DC, Obj_Shutdown_000DE6DC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    Stack frame(ctx, 32);  // results at 112 / 120 / 128 in the original frame
    const u32 res = frame.addr();
    if (on != 0) {
        if (const u32 fx = load<u32>(o + 352); fx != 0) {
            store<u32>(fx + 136, load<u32>(fx + 136) | 0x100u);
            call(ctx, EffRelease, {load<u32>(o + 352), 0x879D1B7Au});
            const u32 f2 = load<u32>(o + 352);
            store<u32>(f2 + 136, load<u32>(f2 + 136) | 0x200u);
            store<u32>(o + 352, 0);
        }
        if (u32 s = load<u32>(o + 348); s != 0) {
            if (const u32 ent = load<u32>(s + 8); ent != 0) call(ctx, kRemoveEntity, {res + 16, ent, s});
            for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(call(ctx, 0x0091A2CCu, {load<u32>(o + 348)})); ++i) {
                const u32 b = static_cast<u32>(call(ctx, 0x0091A7D8u, {load<u32>(o + 348), i}));
                call(ctx, kSkelRel, {b});
            }
            s = load<u32>(o + 348);
            const s32 n = static_cast<s32>(load<u32>(s + 128));
            for (s32 k = 0; k < n; ++k) {
                if (load<u32>(load<u32>(s + 124) + 16 * static_cast<u32>(k)) != 0xA734004Du) continue;
                call(ctx, kRemoveProperty, {res, s, 0xA734004Du});
                s = load<u32>(o + 348);
                break;
            }
            u32 rel = s;
            for (s32 k = 0; k < static_cast<s32>(load<u32>(s + 128)); ++k) {
                if (load<u32>(load<u32>(s + 124) + 16 * static_cast<u32>(k)) != 0x91C7CF5Bu) continue;
                call(ctx, kRemoveProperty, {res + 8, s, 0x91C7CF5Bu});
                rel = load<u32>(o + 348);
                break;
            }
            call(ctx, kSkelRel, {rel});
        }
    }
    call(ctx, 0x000D74C0u, {o, on});
}
