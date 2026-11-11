// Game code, medium sweep part 26: destructors of assorted game objects
// (pointer vectors, owned records and buffers released, then their base
// destructor), and the world's level-data teardown.
#include "mid26.h"

using namespace sly4;
using namespace sly4::mid26;

namespace {
using namespace m26fn;
constexpr u32 kEffectDestruct = 0x004C8E80u;   // Effect_Destruct(effect)
constexpr u32 kRefNodeRelease = 0x004432DCu;   // RefNode_Release(node)
constexpr u32 kEmitterStop = 0x00443424u;      // Emitter_Stop(handle, fade)
constexpr u32 kBaseDtorB12550 = 0x000DF934u;   // VtB12550_Dtor_000DF934(o)
constexpr u32 kRegistryUnlink2 = 0x0048F6B0u;  // Registry_Unlink2_0048F6B0(o)

// PtrVector_Clear(v); Nop(v).
void ClearVec26(PPUContext& ctx, u32 v) {
    call(ctx, PtrVector_Clear, {v});
    call(ctx, Nop, {v});
}

// A heap pointer vector p (20 bytes) at *slot: cleared and freed
// (SmallFree(*slot, 20), the slot re-read) when set.
void FreePtrVec(PPUContext& ctx, u32 slot) {
    const u32 p = load<u32>(slot);
    if (p == 0) return;
    ClearVec26(ctx, p);
    call(ctx, SmallFree, {load<u32>(slot), 20});
}

// The shared tail of the 0x00B12550-based objects: the vtables of the two
// secondary bases at o + a / o + b and of o itself are reset (0x00B12528,
// 0x00B1200C, 0x00B126B8), then VtB12550_Dtor_000DF934(o) (r3 = its).
void TailB12550(PPUContext& ctx, u32 o, u32 a, u32 b) {
    store<u32>(o + a, 0x00B12528u);
    store<u32>(o + b, 0x00B1200Cu);
    store<u32>(o, 0x00B126B8u);
    call(ctx, kBaseDtorB12550, {o});
}

// The "u16 count +2, {key, T*} +8" arrays: every second word is freed
// (Mem_FreeFrom(p, 0); the count re-read, signed), then the count is 0.
void FreePairArray(PPUContext& ctx, u32 ar) {
    for (s32 i = 0; i < static_cast<s32>(load<u16>(ar + 2)); ++i) call(ctx, Mem_FreeFrom, {load<u32>(load<u32>(ar + 8) + 8 * i + 4), 0});
    store<u16>(ar + 2, 0);
}

// The 0x00B1E1F8 base part (r3 = Registry_Unlink2_0048F6B0(o)): vtable
// 0x00B1E1F8; the counted array +2544 (count at -4, 8-byte entries) has its
// entries freed from the last (inline, or by Ptr_FreeAndClear_00321240(e)
// with `viaHelper`) and is freed itself (cleared); for the five slots +2232
// (stride 8) each vector of vectors has its element vectors cleared and
// freed (the slot re-read after each), its inner vector cleared and freed
// (SmallFree 20) and is freed (cleared); +2524 / +2512 / +2500 / +2488
// freed (cleared); the vector +2272 cleared and freed.
void TailB1E1F8(PPUContext& ctx, u32 o, bool viaHelper) {
    const u32 arr = load<u32>(o + 2544);
    store<u32>(o, 0x00B1E1F8u);
    if (arr != 0) {
        const u32 hdr = arr - 4;
        for (s32 i = static_cast<s32>(load<u32>(hdr)) - 1; i >= 0; --i) {
            const u32 e = arr + 8 * static_cast<u32>(i);
            if (viaHelper) {
                call(ctx, 0x00321240u, {e});  // Ptr_FreeAndClear_00321240(e)
            } else if (const u32 q = load<u32>(e); q != 0) {
                call(ctx, Mem_FreeFrom, {q, 0});
                store<u32>(e, 0);
            }
        }
        call(ctx, Mem_FreeFrom, {hdr, 0});
        store<u32>(o + 2544, 0);
    }
    for (u32 k = 0; k < 5; ++k) {
        const u32 slot = o + 2232 + 8 * k;
        u32 v = load<u32>(slot);
        if (v == 0) continue;
        for (u32 j = 0;; ++j) {
            const u32 pv = load<u32>(v);
            if (j >= (pv != 0 ? load<u16>(pv + 8) : 0u)) break;
            const u32 e = load<u32>(load<u32>(pv) + 4 * j);
            if (e == 0) continue;
            ClearVec26(ctx, e);
            call(ctx, Mem_FreeFrom, {e, 0});
            v = load<u32>(slot);
        }
        if (v == 0) continue;
        FreePtrVec(ctx, v);
        call(ctx, Mem_FreeFrom, {v, 0});
        store<u32>(slot, 0);
    }
    FreeSlot26(ctx, o + 2524);
    FreeSlot26(ctx, o + 2512);
    FreeSlot26(ctx, o + 2500);
    FreeSlot26(ctx, o + 2488);
    FreePtrVec(ctx, o + 2272);
    call(ctx, kRegistryUnlink2, {o});
}
}  // namespace

// T::~T(o) (vtables 0x00B1E278 -> 0x00B1E1F8; Delete_003269F0 and the
// 0x00B1E2F0 thunks): the chain from +2576 (next +2564) gets vt[4]; then
// the 0x00B1E1F8 base part (TailB1E1F8, entries freed inline).
DECOMP(003266E4, Obj_Dtor_B1E278_003266E4) {
    const u32 o = arg<u32>(ctx, 0);
    u32 p = load<u32>(o + 2576);
    store<u32>(o, 0x00B1E278u);
    while (p != 0) {
        const u32 next = load<u32>(p + 2564);
        VCall(ctx, p, 4, {p});
        p = next;
    }
    TailB1E1F8(ctx, o, false);
}

// T::~T(o) (vtable 0x00B1D7C0, secondary bases +380 / +384; Delete_
// 002FE8E8 and adjustor thunks): the seven vectors +1100..+964 cleared;
// the effects +880 / +808 / +736 / +664 / +592 destroyed and freed
// (SmallFree 140, cleared); with the vector +388: vtables 0x00B12898 /
// 0x00B12A44 / 0x00B12A6C and it is cleared and freed; base tail.
DECOMP(002FE63C, Obj_Dtor_B1D7C0_002FE63C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1D7C0u);
    store<u32>(o + 380, 0x00B1D96Cu);
    store<u32>(o + 384, 0x00B1D994u);
    for (u32 off : {1100u, 1072u, 1052u, 1032u, 1004u, 984u, 964u}) ClearVec26(ctx, o + off);
    for (u32 off : {880u, 808u, 736u, 664u, 592u}) {
        const u32 e = load<u32>(o + off);
        if (e == 0) continue;
        call(ctx, kEffectDestruct, {e});
        call(ctx, SmallFree, {e, 140});
        store<u32>(o + off, 0);
    }
    if (const u32 v = load<u32>(o + 388); v != 0) {
        store<u32>(o + 384, 0x00B12A6Cu);
        store<u32>(o + 380, 0x00B12A44u);
        store<u32>(o, 0x00B12898u);
        ClearVec26(ctx, v);
        call(ctx, SmallFree, {load<u32>(o + 388), 20});
    }
    TailB12550(ctx, o, 384, 380);
}

// T::~T(o) (vtable 0x00B1DA78; Delete_00303CEC): the buffers of the
// vector +304 freed (cleared, count re-read); each 56-byte record of +324
// has the objects of its vector +36 released (vt[4], cleared), its vector
// cleared and is freed (cleared); the 56-byte records of +344 likewise
// without the release; each count zeroed after its loop; the three vectors
// cleared; Thunk_000E2A34(o) (r3 = its).
DECOMP(00303A4C, Obj_Dtor_B1DA78_00303A4C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1DA78u);
    for (u32 i = 0; i < load<u16>(o + 312); ++i) {
        const u32 e = load<u32>(load<u32>(o + 304) + 4 * i);
        if (e == 0) continue;
        call(ctx, Mem_FreeFrom, {e, 0});
        store<u32>(load<u32>(o + 304) + 4 * i, 0);
    }
    store<u16>(o + 312, 0);
    for (u32 i = 0; i < load<u16>(o + 332); ++i) {
        const u32 r = load<u32>(load<u32>(o + 324) + 4 * i);
        if (r == 0) continue;
        for (u32 j = 0; j < load<u16>(r + 44); ++j) {
            const u32 q = load<u32>(load<u32>(r + 36) + 4 * j);
            if (q == 0) continue;
            VCall(ctx, q, 4, {q});
            store<u32>(load<u32>(r + 36) + 4 * j, 0);
        }
        store<u16>(r + 44, 0);
        ClearVec26(ctx, r + 36);
        call(ctx, SmallFree, {r, 56});
        store<u32>(load<u32>(o + 324) + 4 * i, 0);
    }
    store<u16>(o + 332, 0);
    for (u32 i = 0; i < load<u16>(o + 352); ++i) {
        const u32 r = load<u32>(load<u32>(o + 344) + 4 * i);
        if (r == 0) continue;
        ClearVec26(ctx, r + 36);
        call(ctx, SmallFree, {r, 56});
        store<u32>(load<u32>(o + 344) + 4 * i, 0);
    }
    store<u16>(o + 352, 0);
    ClearVec26(ctx, o + 344);
    ClearVec26(ctx, o + 324);
    ClearVec26(ctx, o + 304);
    call(ctx, 0x000E2A34u, {o});
}

// T::~T(o) (vtable 0x00B1B740, secondary bases +388 / +392; Delete_002348B0
// and adjustor thunks): Emitter_Stop(+864 / +868 / +872, 1);
// RefNode_Release of +896, +916..+928 and +900..+912; the vectors +716 /
// +696 / +676 cleared; the buffers +488 (size +504 * 4), +464 / +440 /
// +416 (sizes +480 / +456 / +432 * 16) go back to SmallFree; base tail.
DECOMP(002346A4, Obj_Dtor_B1B740_002346A4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1B740u);
    store<u32>(o + 388, 0x00B1B8F4u);
    store<u32>(o + 392, 0x00B1B91Cu);
    for (u32 off : {864u, 868u, 872u}) call(ctx, kEmitterStop, {load<u32>(o + off), 1});
    for (u32 off : {896u, 916u, 920u, 924u, 928u, 900u, 904u, 908u, 912u}) call(ctx, kRefNodeRelease, {load<u32>(o + off)});
    for (u32 off : {716u, 696u, 676u}) ClearVec26(ctx, o + off);
    call(ctx, SmallFree, {load<u32>(o + 488), load<u32>(o + 504) << 2});
    call(ctx, SmallFree, {load<u32>(o + 464), load<u32>(o + 480) << 4});
    call(ctx, SmallFree, {load<u32>(o + 440), load<u32>(o + 456) << 4});
    call(ctx, SmallFree, {load<u32>(o + 416), load<u32>(o + 432) << 4});
    TailB12550(ctx, o, 392, 388);
}

// T::~T(o) (vtable 0x00B14168; Delete_00121E24): StateMachine_Shutdown(o);
// the references +392 (unless it is +384), +388 (unless it is +384) and
// +384 get vt[4] and are cleared; the eight effects +796 (stride 16) get
// bit 0x200 of +136 and are cleared; +976 freed (cleared); RefNode_Release
// of +948, +940, +944, +952..+964; the vector +1328 cleared; the pool array
// +916 returns its data (allocator +928 vt[8]) and is destroyed; vtables
// 0x00B0DB3C (+492) / 0x00B126B8; VtB12550_Dtor_000DF934(o) (r3 = its).
DECOMP(00121BDC, Obj_Dtor_B14168_00121BDC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B14168u);
    call(ctx, 0x00464B90u, {o});
    u32 a = load<u32>(o + 384);
    if (const u32 b = load<u32>(o + 392); b != a && b != 0) {
        VCall(ctx, b, 4, {b});
        a = load<u32>(o + 384);
    }
    const u32 c = load<u32>(o + 388);
    store<u32>(o + 392, 0);
    if (c != 0 && c != a) {
        VCall(ctx, c, 4, {c});
        a = load<u32>(o + 384);
    }
    store<u32>(o + 388, 0);
    if (a != 0) {
        VCall(ctx, a, 4, {a});
        store<u32>(o + 384, 0);
    }
    for (u32 k = 0; k < 8; ++k) {
        const u32 slot = o + 796 + 16 * k;
        const u32 e = load<u32>(slot);
        if (e == 0) continue;
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        store<u32>(slot, 0);
    }
    FreeSlot26(ctx, o + 976);
    for (u32 off : {948u, 940u, 944u, 952u, 956u, 960u, 964u}) call(ctx, kRefNodeRelease, {load<u32>(o + off)});
    ClearVec26(ctx, o + 1328);
    const u32 sz = load<u16>(o + 916) * ((static_cast<u32>(load<u16>(o + 922)) >> 1) & 0x7FFFu);
    if (const u32 d = load<u32>(o + 924); d != 0) {
        const u32 al = load<u32>(o + 928);
        VCall(ctx, al, 8, {al, d, sz});
    }
    call(ctx, 0x0054EC94u, {o + 916});  // Array_Destruct
    store<u32>(o + 492, 0x00B0DB3Cu);
    store<u32>(o, 0x00B126B8u);
    call(ctx, kBaseDtorB12550, {o});
}

// T::~T(o) (vtable 0x00B1BF00; Delete_00266BF4): for each group of the
// vector +28 (u16 count +36; the group pointer is used unchecked) the
// objects of its 8-byte list +8 (count +16) and its object +52 that have
// bit 0x40000 of +172 get vt[40](e, 1); the list count is zeroed and the
// group (when set) has its vectors +28 / +8 cleared and is freed
// (SmallFree 64); the count +36 zeroed and the vector cleared; vtable
// 0x00B1BE30, o removed from the registry 0x01056F1C;
// List1013B5C_Remove(o) (r3 = its).
DECOMP(0026694C, Obj_Dtor_B1BF00_0026694C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1BF00u);
    for (u32 i = 0; i < load<u16>(o + 36); ++i) {
        const u32 g = load<u32>(load<u32>(o + 28) + 4 * i);
        for (u32 j = 0; j < load<u16>(g + 16); ++j) {
            const u32 e = load<u32>(load<u32>(g + 8) + 8 * j);
            if (e != 0 && (load<u32>(e + 172) & 0x40000u) != 0) VCall(ctx, e, 40, {e, 1});
        }
        store<u16>(g + 16, 0);
        if (const u32 h = load<u32>(g + 52); h != 0 && (load<u32>(h + 172) & 0x40000u) != 0) VCall(ctx, h, 40, {h, 1});
        if (g == 0) continue;  // checked only here (quirk)
        ClearVec26(ctx, g + 28);
        ClearVec26(ctx, g + 8);
        call(ctx, SmallFree, {g, 64});
    }
    store<u16>(o + 36, 0);
    ClearVec26(ctx, o + 28);
    store<u32>(o, 0x00B1BE30u);
    RegistryRemove26(o);
    call(ctx, 0x000D6940u, {o});  // List1013B5C_Remove
}

// void f(LevelData* o) (called by World_Destruct): the session pointer
// 0x00F64C20 is cleared; the five pair arrays 0x01068184..0x010681E4
// (stride 24) free their objects, the vector 0x010681FC its buffers (each
// count zeroed); +20 / +28 freed (cleared), +24 / +32 cleared;
// Level_Release2D9A14(+8) when set; +0..+16 zeroed. r3 = 0.
DECOMP(002DD604, Level_ReleaseData_002DD604) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(kSession26, 0);
    for (u32 ar = 0x01068184u; ar <= 0x010681E4u; ar += 24) FreePairArray(ctx, ar);
    const u32 v = 0x010681FCu;
    for (u32 i = 0; i < load<u16>(v + 8); ++i) call(ctx, Mem_FreeFrom, {load<u32>(load<u32>(v) + 4 * i), 0});
    store<u16>(v + 8, 0);
    FreeSlot26(ctx, o + 20);
    FreeSlot26(ctx, o + 28);
    const u32 lv = load<u32>(o + 8);
    store<u32>(o + 24, 0);
    store<u32>(o + 32, 0);
    if (lv != 0) call(ctx, 0x002D9A14u, {lv});
    for (u32 off = 0; off <= 16; off += 4) store<u32>(o + off, 0);
    ctx.gpr[3] = 0;
}

// T::~T(o) (vtable 0x00B1EFD0; Delete_0033D49C): 0x0033CE20(o, 0, 1); each
// 156-byte record of the vector +2572 (u16 count +2580) is shut down
// (0x0033C03C), its vectors +132 / +112 / +92 / +72 cleared and it is freed
// (SmallFree, not cleared); the records of +2592 (36 bytes; count +2600)
// and +2772 (144 bytes; count +2780, then zeroed) freed unchecked;
// Obj_Flag13Off_0048F298(o, 0); the vectors +2792 / +2772 / +2592 / +2572
// cleared; then the 0x00B1E1F8 base part (TailB1E1F8, entries freed by
// Ptr_FreeAndClear_00321240).
DECOMP(0033D008, Obj_Dtor_B1EFD0_0033D008) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1EFD0u);
    call(ctx, 0x0033CE20u, {o, 0, 1});
    for (u32 i = 0; i < load<u16>(o + 2580); ++i) {
        const u32 r = load<u32>(load<u32>(o + 2572) + 4 * i);
        if (r == 0) continue;
        call(ctx, 0x0033C03Cu, {r});
        for (u32 off : {132u, 112u, 92u, 72u}) ClearVec26(ctx, r + off);
        call(ctx, SmallFree, {r, 156});
    }
    for (u32 i = 0; i < load<u16>(o + 2600); ++i) call(ctx, SmallFree, {load<u32>(load<u32>(o + 2592) + 4 * i), 36});
    for (u32 i = 0; i < load<u16>(o + 2780); ++i) call(ctx, SmallFree, {load<u32>(load<u32>(o + 2772) + 4 * i), 144});
    store<u16>(o + 2780, 0);
    call(ctx, 0x0048F298u, {o, 0});
    for (u32 off : {2792u, 2772u, 2592u, 2572u}) ClearVec26(ctx, o + off);
    TailB1E1F8(ctx, o, true);
}
