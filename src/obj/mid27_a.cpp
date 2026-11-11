// Game code, medium sweep part 27: destructors of registered objects (the
// 0x00B1BE30 base part: registry 0x01056F1C, List1013B5C_Remove), the
// non-deleting halves of the template Delete_* thunks in obj19_tmpl.cpp.
#include "mid27.h"

using namespace sly4;
using namespace sly4::mid27;

namespace {
using namespace m27fn;
constexpr u32 kHandleOwner = 0x00F64C38u;  // owner passed to Handle_Release_002F20A4
constexpr u32 kAnimListTbl = 0x00F35850u;  // World_ListRemove table of 0x002BC728
constexpr u32 kHandleRelease = 0x002F20A4u;
constexpr u32 kAnimSet = 0x002549F8u;       // Anim_Set2549F8(c, u8 full)
constexpr u32 kSkelRelease27 = 0x007D4EA8u; // AnimSkel_Release7D4EA8(p)
constexpr u32 kRemoveEntity27 = 0x0092FFC8u; // Hk_RemoveEntity92FFC8(result*, world, entity)
}  // namespace

// T::~T(o) (vtable 0x00B1CFD8; Delete_002BCA1C, 400 bytes): the two
// 36-byte slots +44 / +80, when set, are detached from their owner
// (e->+260 vt[24](owner, e)), stopped (vt[40](e, 0, 0)) and released
// (vt[8]; the slot is not cleared). The effect +324 gets Effect_Release
// (0x10CBB) and bit 0x200 of its +136. With the body +320's entity (+8)
// it leaves the world (result on the stack) and bit 0x20000 of the def
// (+8) doubleword +184 is cleared; the body is released (no null check).
// The handles +56 / +92 go to Handle_Release_002F20A4(*0x00F64C38, h);
// World_ListRemove(*0x00F65CA0, 0, 0x00F35850, o, o->+8);
// Anim_Set2549F8(o, 1); +368 = 0, the 8-bit field at bit 23 of +384 = 6;
// the PtrVector +328 destroyed; base part.
DECOMP(002BC728, Obj_Dtor_B1CFD8_002BC728) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);  // Hk_RemoveEntity result at 112(r1)
    store<u32>(o, 0x00B1CFD8u);
    for (u32 i = 0; i < 2; ++i) {
        const u32 slot = o + 44 + 36 * i;
        const u32 e = load<u32>(slot);
        if (e == 0) continue;
        const u32 own = load<u32>(e + 260);
        VCall(ctx, own, 24, {own, e});
        const u32 e2 = load<u32>(slot);
        VCall(ctx, e2, 40, {e2, 0, 0});
        const u32 e3 = load<u32>(slot);
        VCall(ctx, e3, 8, {e3});
    }
    if (const u32 fx = load<u32>(o + 324); fx != 0) {
        call(ctx, EffRelease, {fx, 0x00010CBBu});
        const u32 f2 = load<u32>(o + 324);
        store<u32>(f2 + 136, load<u32>(f2 + 136) | 0x200u);
    }
    u32 body = load<u32>(o + 320);
    if (const u32 ent = load<u32>(body + 8); ent != 0) {
        call(ctx, kRemoveEntity27, {frame.addr(), ent, body});
        const u32 d = load<u32>(o + 8);
        store<u64>(d + 184, load<u64>(d + 184) & ~0x20000ull);
        body = load<u32>(o + 320);
    }
    call(ctx, kSkelRelease27, {body});
    for (u32 i = 0; i < 2; ++i) call(ctx, kHandleRelease, {load<u32>(kHandleOwner), load<u32>(o + 56 + 36 * i)});
    call(ctx, World_ListRemove, {load<u32>(mid25::kWorld25), 0, kAnimListTbl, o, load<u32>(o + 8)});
    call(ctx, kAnimSet, {o, 1});
    const u32 f = load<u32>(o + 384);
    store<u32>(o + 368, 0);
    store<u32>(o + 384, (f & 0x807FFFFFu) | 0x03000000u);
    PtrVecDtor27(ctx, o + 328);
    RegBase27(ctx, o);
}

// T::~T(o) (vtable 0x00B1C8D8; Delete_0029A3C0, 2352 bytes):
// RefNode_Release(+2256) and +2256 = 0; +2268 vt[8] (cleared); the
// effect +2276 gets bit 0x200 of its +136 (cleared); the component +2176,
// when its +172 has bit 0x40000, gets vt[40](c, 1) and is cleared; every
// entry of the PtrVector +64 (u16 count +72, re-read) goes through
// f_00299F74(o, e); the counts +72 / +52 / +92 = 0; the nav query +2328
// finished and freed; the PtrVectors +84 / +64 / +44 / +24 destroyed;
// base part.
DECOMP(0029A108, Obj_Dtor_B1C8D8_0029A108) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1C8D8u);
    call(ctx, RefNodeRelease, {load<u32>(o + 2256)});
    store<u32>(o + 2256, 0);
    Vt8Slot(ctx, o + 2268, true);
    if (const u32 fx = load<u32>(o + 2276); fx != 0) {
        store<u32>(fx + 136, load<u32>(fx + 136) | 0x200u);
        store<u32>(o + 2276, 0);
    }
    if (const u32 c = load<u32>(o + 2176); c != 0 && (load<u32>(c + 172) & 0x40000u) != 0) {
        VCall(ctx, c, 40, {c, 1});
        store<u32>(o + 2176, 0);
    }
    for (u32 i = 0; i < load<u16>(o + 72); ++i) call(ctx, 0x00299F74u, {o, load<u32>(load<u32>(o + 64) + 4 * i)});
    store<u16>(o + 72, 0);
    store<u16>(o + 52, 0);
    store<u16>(o + 92, 0);
    call(ctx, 0x0062E094u, {o + 2328});  // NavQuery_Finish62E094
    call(ctx, 0x0062D08Cu, {o + 2328});  // NavQuery_Free62D08C
    for (u32 off : {84u, 64u, 44u, 24u}) PtrVecDtor27(ctx, o + off);
    RegBase27(ctx, o);
}

// T::~T(o) (vtable 0x00B1C680; Delete_00289D7C, 656 bytes): o leaves the
// global list 0x01057770 (u32 count *0x01057870; swapped with the last
// entry); unless +404 is 3, PairBuffer_PushData(*0x00F64A54, 0x00F35378,
// o + 244, 64); the 64 bytes +244 zeroed; TableEntry_Release_005AD0A0
// (o + 240); with o->+8 and its +164, Obj_FindPair_0034F580(+164,
// 0x00F35358, o); the pool parts +568, +552, +536, +280, +264, +248
// destructed; base part.
DECOMP(00289AA8, Obj_Dtor_B1C680_00289AA8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kCnt = 0x01057870u, kArr = 0x01057770u;
    store<u32>(o, 0x00B1C680u);
    if (const u32 n = load<u32>(kCnt); n != 0) {
        for (u32 i = 0; i < n; ++i) {
            if (load<u32>(kArr + 4 * i) != o) continue;
            const u32 last = n - 1;
            const u32 a = load<u32>(kArr + 4 * i), b = load<u32>(kArr + 4 * last);
            store<u32>(kArr + 4 * last, a);
            store<u32>(kArr + 4 * i, b);
            store<u32>(kCnt, last);
            break;
        }
    }
    if (load<u32>(o + 404) != 3) call(ctx, 0x00565044u, {load<u32>(0x00F64A54u), 0x00F35378u, o + 244, 64});
    for (u32 k = 0; k < 16; ++k) store<u32>(o + 244 + 4 * k, 0);
    call(ctx, 0x005AD0A0u, {o + 240});
    if (const u32 d = load<u32>(o + 8); d != 0) {
        if (const u32 p = load<u32>(d + 164); p != 0) call(ctx, 0x0034F580u, {p, 0x00F35358u, o});
    }
    for (u32 off : {568u, 552u, 536u, 280u, 264u, 248u}) call(ctx, PoolDestructPart, {o + off});
    RegBase27(ctx, o);
}
