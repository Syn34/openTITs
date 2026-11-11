// Game code, medium sweep part 26: destructors of physics-backed registered
// objects (base vtable 0x00B1BE30: removed from the registry 0x01056F1C and
// from the list 0x01013B5C last). Their Havok bodies are detached and
// released first.
#include "mid26.h"

using namespace sly4;
using namespace sly4::mid26;

namespace {
using namespace m26fn;
constexpr u32 kRegBaseVt = 0x00B1BE30u;        // registered-object base vtable
constexpr u32 kListRemove = 0x000D6940u;       // List1013B5C_Remove(o)
constexpr u32 kSkelRelease = 0x007D4EA8u;      // AnimSkel_Release7D4EA8(p)
constexpr u32 kGetBody = 0x009D19C8u;          // Hk_GetBody9D19C8(x) -> body
constexpr u32 kBodySetOwner = 0x0091A228u;     // HkBody_SetMotion91A228(body, o + 24)
constexpr u32 kBodyRemove = 0x00931F00u;       // Body_Remove931F00(world, body)
constexpr u32 kRemoveEntity = 0x0092FFC8u;     // Hk_RemoveEntity92FFC8(result*, world, entity)
constexpr u32 kRemoveProperty = 0x0093FED8u;   // Hk_RemoveProperty93FED8(result*, entity, key)
constexpr u32 kRefNodeRelease = 0x004432DCu;   // RefNode_Release(node)

// The base part: o removed from the registry (variant B / A) and
// List1013B5C_Remove(o) (r3 = its).
void RegBaseTail(PPUContext& ctx, u32 o, bool reread) {
    if (reread) RegistryRemoveB26(o);
    else RegistryRemove26(o);
    call(ctx, kListRemove, {o});
}
}  // namespace

// T::~T(o) (vtable 0x00B1CE20; Delete_002B7074): the attachment +64 is
// first reset (0x0012D290) when its +192 has bit 8, then gets vt[40](a, 0)
// and is cleared; +128 freed (cleared); with the rigid body +92: its entity
// (+8) leaves the world and the properties 0xA734004D / 0x91C7CF5B are
// removed (results on the stack), then it is released; +112 vt[8]
// (cleared); the effect +108 deleted (0x004D0280, cleared); base part.
DECOMP(002B6E2C, Obj_Dtor_B1CE20_002B6E2C) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 32);  // three result words at 112 / 120 / 128 in the original frame
    const u32 res = frame.addr();
    u32 a = load<u32>(o + 64);
    store<u32>(o, 0x00B1CE20u);
    if (a != 0 && (load<u32>(a + 192) & 8u) != 0) {
        call(ctx, 0x0012D290u, {a});
        a = load<u32>(o + 64);
    }
    if (a != 0) VCall(ctx, a, 40, {a, 0});
    store<u32>(o + 64, 0);
    FreeSlot26(ctx, o + 128);
    if (const u32 rb = load<u32>(o + 92); rb != 0) {
        call(ctx, kRemoveEntity, {res + 16, load<u32>(rb + 8), rb});
        call(ctx, kRemoveProperty, {res, load<u32>(o + 92), 0xA734004Du});
        call(ctx, kRemoveProperty, {res + 8, load<u32>(o + 92), 0x91C7CF5Bu});
        call(ctx, kSkelRelease, {load<u32>(o + 92)});
    }
    Vt8Slot(ctx, o + 112, true);
    if (const u32 e = load<u32>(o + 108); e != 0) {
        call(ctx, 0x004D0280u, {e});  // Effect_Delete
        store<u32>(o + 108, 0);
    }
    store<u32>(o, kRegBaseVt);
    RegBaseTail(ctx, o, true);
}

// T::~T(o) (vtables 0x00B1D158 / +24 0x00B1D1A8; Delete_002CCBE8): when
// attached (u8 +376), the body of the def's (o->+8 vt[16]) +32 is
// given HkBody_SetMotion91A228(body, o + 24) and +376 cleared; the
// bodies +116 / +120 leave their world (+8) and are released; +380
// vt[40](c, 0) (cleared); +356 cleared and, unless +360 has bit
// 0x80000000, the handler at *0x011DF31C +20 gets (+352, +360 * 4); +352 = 0,
// +360 = 0x80000000; vtables 0x00B1200C (+24) / base part.
DECOMP(002CC960, Obj_Dtor_B1D158_002CC960) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 attached = load<u8>(o + 376);
    store<u32>(o, 0x00B1D158u);
    store<u32>(o + 24, 0x00B1D1A8u);
    if (attached != 0) {
        const u32 d = load<u32>(o + 8);
        const u32 w = static_cast<u32>(VCall(ctx, d, 64, {d}));
        if (const u32 x = load<u32>(w + 32); x != 0) {
            if (const u32 b = static_cast<u32>(call(ctx, kGetBody, {x})); b != 0) {
                call(ctx, kBodySetOwner, {b, o + 24});
                store<u8>(o + 376, 0);
            }
        }
    }
    for (u32 off : {116u, 120u}) {
        u32 p = load<u32>(o + off);
        if (p == 0) continue;
        if (const u32 wd = load<u32>(p + 8); wd != 0) {
            call(ctx, kBodyRemove, {wd, p});
            p = load<u32>(o + off);
        }
        call(ctx, kSkelRelease, {p});
    }
    if (const u32 c = load<u32>(o + 380); c != 0) {
        VCall(ctx, c, 40, {c, 0});
        store<u32>(o + 380, 0);
    }
    const u32 f = load<u32>(o + 360);
    store<u32>(o + 356, 0);
    if ((f & 0x80000000u) == 0) {
        const u32 h = 0x011DF31Cu;  // handler record: function descriptor at (*h) + 20
        VCall(ctx, h, 20, {h, load<u32>(o + 352), static_cast<u64>(static_cast<s64>(static_cast<s32>(f << 2)))});
    }
    store<u32>(o + 352, 0);
    store<u32>(o + 360, 0x80000000u);
    store<u32>(o + 24, 0x00B1200Cu);
    store<u32>(o, kRegBaseVt);
    RegBaseTail(ctx, o, true);
}

// T::~T(o) (vtables 0x00B1D1E0 / +24 0x00B1D230; Delete_002D127C):
// World_ListRemove_0046B7A4(world, 0, 0x00F35980, o, o->+8); 0x002D0BA0(o,
// 0); the component +36 gets vt[40](c, 0) and then, when its +172 has bit
// 0x40000 (re-read, unchecked), vt[40](c, 1); the skeleton +32 is
// given HkBody_SetMotion91A228(s, o + 24) and each of its parts (0x0091A2CC
// count, re-read; 0x0091A7D8(s, i)) released, then it is released;
// RefNode_Release(+64), RefNode_Release(+60); vtables 0x00B1200C (+24) /
// base part.
DECOMP(002D1034, Obj_Dtor_B1D1E0_002D1034) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1D1E0u);
    store<u32>(o + 24, 0x00B1D230u);
    call(ctx, World_ListRemove, {load<u32>(mid25::kWorld25), 0, 0x00F35980u, o, load<u32>(o + 8)});
    call(ctx, 0x002D0BA0u, {o, 0});
    if (const u32 c = load<u32>(o + 36); c != 0) {
        VCall(ctx, c, 40, {c, 0});
        const u32 c2 = load<u32>(o + 36);
        if ((load<u32>(c2 + 172) & 0x40000u) != 0) VCall(ctx, c2, 40, {c2, 1});
    }
    call(ctx, kBodySetOwner, {load<u32>(o + 32), o + 24});
    for (s32 i = 0; i < static_cast<s32>(call(ctx, 0x0091A2CCu, {load<u32>(o + 32)})); ++i) {
        const u32 part = static_cast<u32>(call(ctx, 0x0091A7D8u, {load<u32>(o + 32), static_cast<u64>(static_cast<s64>(i))}));
        call(ctx, kSkelRelease, {part});
    }
    call(ctx, kSkelRelease, {load<u32>(o + 32)});
    call(ctx, kRefNodeRelease, {load<u32>(o + 64)});
    call(ctx, kRefNodeRelease, {load<u32>(o + 60)});
    store<u32>(o + 24, 0x00B1200Cu);
    store<u32>(o, kRegBaseVt);
    RegBaseTail(ctx, o, false);
}

// T::~T(o) (vtables 0x00B1BF48 / +24 0x00B1BF9C; Delete_002695D8): when
// +32 is set and +416 has bit 0x80000000, the entity of its body (+8)
// leaves the world W = o->+28->+4 (result on the stack) and so does the
// body +48 (when in a world); bit 0x80000000 of +416 cleared; the three
// parts +36 are dropped from the world's array (Array_RemoveSwap_00346698
// on *(world + 0x1281D4)), then given HkBody_SetMotion91A228(body, o + 24) and
// released; +48 released; vtables 0x00B1200C (+24) / base part.
DECOMP(00269348, Obj_Dtor_B1BF48_00269348) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);  // the result word at 112 in the original frame
    const u32 p32 = load<u32>(o + 32);
    store<u32>(o, 0x00B1BF48u);
    store<u32>(o + 24, 0x00B1BF9Cu);
    if (p32 != 0 && (load<u32>(o + 416) & 0x80000000u) != 0) {
        const u32 b = static_cast<u32>(call(ctx, kGetBody, {p32}));
        const u32 w = load<u32>(load<u32>(o + 28) + 4);
        if (const u32 ent = load<u32>(b + 8); ent != 0) call(ctx, kRemoveEntity, {frame.addr(), w, b});
        if (const u32 s = load<u32>(o + 48); s != 0 && load<u32>(s + 8) != 0) call(ctx, kBodyRemove, {w, s});
    }
    store<u32>(o + 416, load<u32>(o + 416) & 0x7FFFFFFFu);
    for (u32 k = 0; k < 3; ++k) {
        if (const u32 e = load<u32>(o + 36 + 4 * k); e != 0) call(ctx, 0x00346698u, {load<u32>(load<u32>(mid25::kWorld25) + 0x1281D4u), e});
    }
    for (u32 k = 0; k < 3; ++k) {
        const u32 slot = o + 36 + 4 * k;
        const u32 e = load<u32>(slot);
        if (e == 0) continue;
        const u32 body = static_cast<u32>(call(ctx, kGetBody, {e}));
        call(ctx, kBodySetOwner, {body, o + 24});
        call(ctx, kSkelRelease, {load<u32>(slot)});
    }
    if (const u32 s = load<u32>(o + 48); s != 0) call(ctx, kSkelRelease, {s});
    store<u32>(o + 24, 0x00B1200Cu);
    store<u32>(o, kRegBaseVt);
    RegBaseTail(ctx, o, true);
}
