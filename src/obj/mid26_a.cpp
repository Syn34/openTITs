// Game code, medium sweep part 26: the other binding re-key handlers of the
// animated set pieces (siblings of Obj_RekeyBindings_003C5E38, all reached
// through 0x003C60C8). They re-key the 128-byte binding records (+104
// array, u16 count +112): +0 is the binding's animation key, bit 0x80 of the
// byte +104 marks it dirty, +80 names the owner it belongs to and +108 its
// name record.
#include "mid26.h"

using namespace sly4;
using namespace sly4::mid26;

namespace {
using namespace m26fn;

constexpr u32 kLayerSlot = 0x001281E0u;   // offset of the animator layer stack in the object vt[76] returns
constexpr u32 kFindHash = 0xFE11127Cu;    // child component re-armed after a re-key
constexpr u32 kNotifyHandle = 0x0001447Bu;

// The binding at index i.
inline u32 Binding(u32 o, u32 i) { return load<u32>(o + 104) + 128 * i; }

// Every binding whose +80 is not `skip` gets key k, demoted to 0x5F9B8391
// when the session's player is the current one (f_002E23AC) and
// Player_LevelKeyByte_002E1B1C(session, hash of the binding name +108 +4)
// is 0; then bit 0x80 of +104. The count is re-read each iteration.
void RekeyOthers(PPUContext& ctx, u32 o, u32 skip, u32 k) {
    for (u32 i = 0; i < load<u16>(o + 112); ++i) {
        const u32 e = Binding(o, i);
        if (load<u32>(e + 80) == skip) continue;
        store<u32>(e, k);
        const u32 s = load<u32>(kSession26);
        const u32 cur = load<u32>(s + 36);
        if (cur == static_cast<u32>(call(ctx, CurPlayerKey, {s, s}))) {
            const u32 s2 = load<u32>(kSession26);
            const u32 h = static_cast<u32>(call(ctx, HashNoCase, {load<u32>(e + 108) + 4}));
            if (static_cast<u32>(call(ctx, LevelKeyByte, {s2, h})) == 0) store<u32>(e, 0x5F9B8391u);
        }
        store<u8>(e + 104, static_cast<u8>(load<u8>(e + 104) | 0x80u));
    }
}

// Every binding whose +80 is `key` gets key k and bit 0x80 of +104.
void RekeyMatching(u32 o, u32 key, u32 k) {
    for (u32 i = 0; i < load<u16>(o + 112); ++i) {
        const u32 e = Binding(o, i);
        if (load<u32>(e + 80) != key) continue;
        store<u32>(e, k);
        store<u8>(e + 104, static_cast<u8>(load<u8>(e + 104) | 0x80u));
    }
}

// The def o->+1584 has a +216 record of kind 0x27EAE169 (+52). Returns the
// kind compared (0 without a record); `def` gets o->+1584.
u32 DefKind(u32 o, u32& def) {
    def = load<u32>(o + 1584);
    if (def == 0) return 0;
    const u32 rec = load<u32>(def + 216);
    return rec != 0 ? load<u32>(rec + 52) : 0;
}
constexpr u32 kKindShared = 0x27EAE169u;

// Pops the animator o->+off off the layer stack of vt[76](o) (+0x1281E0)
// when both are set (Obj_PopLayer(stack, animator; f1 = *t)), then releases
// it (vt[4]) and clears the slot; without a stack the release happens only
// when `always`.
void DropLayer(PPUContext& ctx, u32 o, u32 off, u32 t, bool always) {
    const u32 w = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 stack = load<u32>(w + kLayerSlot);
    if (stack != 0) {
        const u32 l = load<u32>(o + off);
        if (l == 0) return;
        callf(ctx, Obj_PopLayer, {stack, l}, {Lf(t)});
    } else if (!always) {
        return;
    }
    if (const u32 l = load<u32>(o + off); l != 0) {
        VCall(ctx, l, 4, {l});
        store<u32>(o + off, 0);
    }
}

// The child o->+1608 component 0xFE11127C (Obj_FindByHash_000DB290) gets
// vt[88](c, a, 1, 1, 1). Returns r3 as the original leaves it (the lookup's
// result, or the vt call's).
u32 RearmChild(PPUContext& ctx, u32 o, u32 a) {
    const u32 p = load<u32>(o + 1608);
    if (p == 0) return 0;
    const u32 c = static_cast<u32>(call(ctx, Obj_FindByHash, {p, kFindHash}));
    if (c == 0) return 0;
    return static_cast<u32>(VCall(ctx, c, 88, {c, a, 1, 1, 1}));
}

// The player (session +36) is missing or has its slot record flag 0x40.
bool PlayerFlagged(PPUContext& ctx) {
    const u32 p = load<u32>(load<u32>(kSession26) + 36);
    if (p == 0) return true;
    return static_cast<u32>(call(ctx, SlotRec_Flag40, {p})) != 0;
}
}  // namespace

// u32 f(T* o): when the animation source o->+1588 reports one (vt[60]),
// Anim_Restart2775A8(that, 0xACC3FD87, ..; f1-f3 = the floats at 0x003C5578,
// f4 = f3); the animator +1516 is popped (f1 = *0x003C5584) and released
// (both only when vt[76](o) has a layer stack).
// In mode 3 (o->+20->+4) every binding with an owner (+80) gets key
// 0xBA291C90 (RekeyOthers), then Obj_SyncBindingPoses(o, mode, 0) and the
// child component +1608 is re-armed (vt[88](c, 0, 1, 1, 1)). Always ends
// with Handle_Notify36742C(0x1447B, 0, 1, 0), whose r3 it returns.
DECOMP(003C5588, Obj_RekeyBindings_003C5588) {
    const u32 o = arg<u32>(ctx, 0);
    if (const u32 a = load<u32>(o + 1588); a != 0) {
        if (VCall(ctx, a, 60, {a}) != 0) {
            const u32 a2 = load<u32>(o + 1588);
            const u32 an = static_cast<u32>(VCall(ctx, a2, 60, {a2}));
            const f64 f3 = Lf(0x003C5580u);
            // Stack arguments 9 / 10 (112 / 120) are not written by the original (not read).
            callf(ctx, AnimRestart, {an, 0xACC3FD87u, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {Lf(0x003C5578u), Lf(0x003C557Cu), f3, f3});
        }
    }
    DropLayer(ctx, o, 1516, 0x003C5584u, false);
    const u32 m = load<u32>(o + 20);
    if (load<u32>(m + 4) == 3) {
        RekeyOthers(ctx, o, 0, 0xBA291C90u);
        call(ctx, SyncBindings, {o, load<u32>(m + 4), 0});
        RearmChild(ctx, o, 0);
    }
    call(ctx, HandleNotify, {kNotifyHandle, 0, 1, 0});
}

// u32 f(T* o): the animator +600 is popped (f1 = *0x003C5874) and released;
// the child component +1608 is re-armed (vt[88](c, 1, 1, 1, 1)). In mode 2
// (o->+20->+4): Obj_SyncBindingPoses(o, 2, 0); when the player is missing
// or flagged (SlotRec_Flag40_002DA89C), every binding not owned by o->+1600
// gets key 0x32A3716C (RekeyOthers); then, when the def's record is of kind
// 0x27EAE169, the bindings owned by +1600 get key 0xB6162705.
// r3: o->+1600 after the last loop, the kind compared when it isn't
// 0x27EAE169, 0 without a def, else (mode not 2) what the re-arm left.
DECOMP(003C5878, Obj_RekeyBindings_003C5878) {
    const u32 o = arg<u32>(ctx, 0);
    DropLayer(ctx, o, 600, 0x003C5874u, true);
    const u32 r = RearmChild(ctx, o, 1);
    const u32 m = load<u32>(o + 20);
    if (load<u32>(m + 4) != 2) {
        ctx.gpr[3] = r;
        return;
    }
    const bool flagged = PlayerFlagged(ctx);
    call(ctx, SyncBindings, {o, load<u32>(m + 4), 0});
    if (flagged) RekeyOthers(ctx, o, load<u32>(o + 1600), 0x32A3716Cu);
    u32 def;
    const u32 kind = DefKind(o, def);
    if (def == 0 || kind != kKindShared) {
        ctx.gpr[3] = kind;
        return;
    }
    const u32 me = load<u32>(o + 1600);
    RekeyMatching(o, me, 0xB6162705u);
    ctx.gpr[3] = me;
}

// u32 f(T* o): mode 3 (o->+20->+4) only. Unless the def's record is of kind
// 0x27EAE169, every binding not owned by `key` (o->+1604 when the player
// is missing or flagged, else 0) gets key 0xBA291C90 (RekeyOthers);
// otherwise the bindings owned by o->+1608 get 0xBA291C90 and bit 0x80 and,
// unless the player is missing or flagged, so do those owned by o->+1604
// (that loop re-reads the count only after a match). Then
// Obj_SyncBindingPoses(o, mode, 0) and the child +1608 is re-armed
// (vt[88](c, 0, 1, 1, 1)). Always ends with Handle_Notify36742C(0x1447B,
// 0, 1, 0), whose r3 it returns.
DECOMP(003C5B54, Obj_RekeyBindings_003C5B54) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = load<u32>(o + 20);
    if (load<u32>(m + 4) == 3) {
        const bool flagged = PlayerFlagged(ctx);
        u32 def;
        const u32 kind = DefKind(o, def);
        if (def != 0 && kind != kKindShared) {
            RekeyOthers(ctx, o, flagged ? load<u32>(o + 1604) : 0, 0xBA291C90u);
        } else {
            RekeyMatching(o, load<u32>(o + 1608), 0xBA291C90u);
            u32 n = load<u16>(o + 112);
            if (!flagged && n != 0) {
                const u32 key = load<u32>(o + 1604);
                u32 i = 0;
                do {
                    const u32 e = Binding(o, i);
                    if (load<u32>(e + 80) == key) {
                        store<u32>(e, 0xBA291C90u);
                        store<u8>(e + 104, static_cast<u8>(load<u8>(e + 104) | 0x80u));
                        n = load<u16>(o + 112);
                    }
                    ++i;
                } while (i < n);
            }
        }
        call(ctx, SyncBindings, {o, load<u32>(m + 4), 0});
        RearmChild(ctx, o, 0);
    }
    call(ctx, HandleNotify, {kNotifyHandle, 0, 1, 0});
}

namespace {
constexpr u32 kFindWorldObj = 0x005801DCu;  // WorldObj_FindByHash(hash, kind)
constexpr u32 kLevelKeys = 0x00F64E00u;     // u32[7] level keys, then u32[7] matching object hashes (+28)
constexpr u32 kDefaultLevelKey = 0x04318BE6u;

inline void Enable40(PPUContext& ctx, u32 p, u32 on) {
    if (p != 0) VCall(ctx, p, 40, {p, on, 1});
}
}  // namespace

// void f(T* o, u8 on) (called with 0 by Obj_Dtor_B1F8E0_00376F88): the
// parent +24 gets vt[40](p, on, 1); each def part (o->+8: 28-byte records
// +184, u32 count +188, re-read only after a call) with an object +12 gets
// vt[88](p, on); the children +288 (u16 count +296) and the three 6-slot
// arrays +328 / +352 / +376 (interleaved per slot) get vt[40](c, 0, 1).
// Finally the world object whose hash pairs (0x00F64E00 / +28, entry 0
// when not found) with the level key (the player's +724, or 0x04318BE6
// without a player) gets vt[40](w, on, 1).
// r3 = that call's result, or 0 when the object is not found.
DECOMP(00376C98, Obj_SetPartsEnabled_00376C98) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    Enable40(ctx, load<u32>(o + 24), on);
    const u32 d = load<u32>(o + 8);
    if (u32 n = load<u32>(d + 188); n != 0) {
        u32 i = 0;
        do {
            if (const u32 p = load<u32>(load<u32>(d + 184) + 28 * i + 12); p != 0) {
                VCall(ctx, p, 88, {p, on});
                n = load<u32>(d + 188);
            }
            ++i;
        } while (i < n);
    }
    for (u32 j = 0; j < load<u16>(o + 296); ++j) {
        if (const u32 c = load<u32>(load<u32>(o + 288) + 4 * j); c != 0) VCall(ctx, c, 40, {c, 0, 1});
    }
    for (u32 k = 0; k < 6; ++k) {
        Enable40(ctx, load<u32>(o + 328 + 4 * k), 0);
        Enable40(ctx, load<u32>(o + 352 + 4 * k), 0);
        Enable40(ctx, load<u32>(o + 376 + 4 * k), 0);
    }
    u32 key = kDefaultLevelKey;
    if (const u32 pl = load<u32>(load<u32>(kSession26) + 36); pl != 0) key = load<u32>(pl + 724);
    u32 idx = 0;
    for (u32 k = 0; k < 7; ++k) {
        if (load<u32>(kLevelKeys + 4 * k) == key) {
            idx = k;
            break;
        }
    }
    const u32 w = static_cast<u32>(call(ctx, kFindWorldObj, {load<u32>(kLevelKeys + 28 + 4 * idx), 0}));
    if (w == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    VCall(ctx, w, 40, {w, on, 1});
}
