// Game code, medium sweep part 21: controller state entries, effect spawns
// and UI / list helpers.
#include "mid21.h"

using namespace sly4;
using namespace sly4::mid21;

namespace {
constexpr u32 kPartsReady = 0x001F465Cu;  // Obj_Check_001F465C(x)
constexpr u32 kMotionSet = 0x004A0C20u;   // Motion_Set4A0C20(m; f32 limit)
constexpr u32 kAnimTarget = 0x0030DBF0u;  // Anim_SetTarget_0030DBF0(x, hash, -, u8 force; f32 t)
constexpr u32 kAnimRestart = 0x002775A8u;  // Anim_Restart2775A8

// The shared body of the two hold-state entries (constants C: f32 c0, k1,
// k2): s->28 += 1, s->36 = 0, s->64 = 2 when the second part chain of the
// owner x = s->32 is ready (Obj_Check_001F465C(x->784)) and the first
// (x->776) is not, else 0; s->vfn[28](s, 0, 1); x->1048 = 0, x->1044 =
// x->1052, x->1028 = 0, Motion_Set4A0C20(x+1024; x->1052 * k1),
// Anim_SetTarget_0030DBF0(x, 0xDB8DE820, -, 0; c0) (r5 left by Motion_Set).
// Unless x->668 already holds the clip (hNz with s->64 set, else hZ):
// Anim_Restart2775A8(x->vfn[60](), clip, 0, r6, -1, 0, 0, 0, -, -, 0, -1;
// k2 * *0x01069008, (byte x->700 ? c0 : *0x01068FB8) != c0, c0, c0), x->700
// = 0, x->668 = clip. r3 / f1 as the original leaves them.
void HoldEnter(PPUContext& ctx, u32 s, u32 C, u32 hNz, u32 hZ, u32 r6) {
    store<u32>(s + 28, load<u32>(s + 28) + 1);
    const u32 x0 = load<u32>(s + 32);
    const f64 c0 = Lf(C);
    const u32 p1 = load<u32>(x0 + 776);
    store<u32>(s + 36, 0);
    u32 st = 0;
    u32 x = x0;
    bool first = false;
    if (p1 != 0) {
        first = call(ctx, kPartsReady, {p1}) != 0;
        x = load<u32>(s + 32);
    }
    if (!first) {
        const u32 p2 = load<u32>(x + 784);
        if (p2 != 0 && call(ctx, kPartsReady, {p2}) != 0) st = 2;
    }
    store<u32>(s + 64, st);
    VCall(ctx, s, 28, {s, 0, 1});
    const u32 q = load<u32>(s + 32);
    const u32 w1052 = load<u32>(q + 1052);
    store<u32>(q + 1048, 0);
    store<u32>(q + 1044, w1052);
    store<u32>(q + 1028, 0);
    callf(ctx, kMotionSet, {q + 1024}, {S(LfsF(w1052) * Lf(C + 4))});
    // r5 is whatever Motion_Set4A0C20 left (Anim_SetTarget does not read it)
    callf(ctx, kAnimTarget, {load<u32>(s + 32), 0xDB8DE820u, ctx.gpr[5], 0}, {c0});
    const u32 sv = load<u32>(s + 64);
    const u32 o = load<u32>(s + 32);
    const u32 clip = sv != 0 ? hNz : hZ;
    const u32 cur = load<u32>(o + 668);
    const f64 rate = S(Lf(C + 8) * Lf(0x01069008u));
    ctx.gpr[3] = cur;
    ctx.fpr[1] = Lf(0x01069008u);
    if (clip == cur) return;
    const f64 v = load<u8>(o + 700) != 0 ? c0 : Lf(0x01068FB8u);
    const u32 flag = v == c0 ? 0 : 1;
    const u32 an = static_cast<u32>(VCall(ctx, o, 60, {o}));
    // r8 is left by the vfn[60] call (Anim_Restart does not read it); the 9th / 10th
    // arguments are the uninitialised stack words 112(r1) / 120(r1)
    callf(ctx, kAnimRestart, {an, clip, 0, r6, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {rate, static_cast<f64>(static_cast<f32>(flag)), c0, c0});
    store<u8>(o + 700, 0);
    store<u32>(o + 668, clip);
}
}  // namespace

// Ctl_HoldEnter31A5A8(State* s) (vtable entry): HoldEnter with the
// constants at 0x0031A598 and the clips 0x0B131C5A / 0x6BB381E9.
DECOMP(0031A5A8, Ctl_HoldEnter31A5A8) {
    HoldEnter(ctx, arg<u32>(ctx, 0), 0x0031A598u, 0x0B131C5Au, 0x6BB381E9u, 0);
}

// Ctl_HoldEnter31A0E8(State* s) (vtable entry): HoldEnter with the
// constants at 0x0031A0D8, the clips 0x9E21728B / 0x0D592198 and the 4th
// Anim_Restart argument 1.
DECOMP(0031A0E8, Ctl_HoldEnter31A0E8) {
    HoldEnter(ctx, arg<u32>(ctx, 0), 0x0031A0D8u, 0x9E21728Bu, 0x0D592198u, 1);
}

namespace {
constexpr u32 kSetFind = 0x004BCE90u;       // Set_Find(name) -> Set*
constexpr u32 kSmallAlloc = 0x0055238Cu;    // SmallAlloc(size)
constexpr u32 kEffectCtor = 0x004C7A68u;    // Effect_Construct(e, def)
constexpr u32 kFindComponent = 0x004BB4B4u;  // Effect_FindComponent(e, hash)
constexpr u32 kEffectStart = 0x004C753Cu;   // Effect_Start(e)
constexpr u32 kEffectSetTime = 0x004CF710u;  // Effect_SetTime(e; f32 t)
constexpr u32 kSetLast = 0x011DE288u;       // the last looked-up set value

// The effect definition lookup shared by the two spawners: the set s =
// Set_Find(0); its tree root *(s->16) is walked for key (nodes: key +4,
// value +8, children +12 (key < node key, unsigned) / +16), recording the
// path in s+20 (s16 count s->100). A hit stores the node in s->104 and its
// value in *0x011DE288 and returns s->104->8 (r3 = s->104); a miss stores the
// last node in s->104 (an empty tree: 0 there and in *0x011DE288) and
// returns 0 (r3 = s). No set: 0. (The original's "no node" fallback after a
// miss is unreachable: the last node is never null.)
u32 SetLookup(PPUContext& ctx, u32 key, bool keyLater, u32 keyObj, u32& r3) {
    const u32 s = static_cast<u32>(call(ctx, kSetFind, {0}));
    r3 = s;
    if (s == 0) return 0;
    if (keyLater) key = load<u32>(keyObj + 944);
    u32 node = load<u32>(load<u32>(s + 16));
    if (node == 0) {
        store<u32>(s + 104, 0);
        store<u32>(kSetLast, 0);
        return 0;
    }
    store<u16>(s + 100, 0);
    for (;;) {
        const u32 k = load<u32>(node + 4);
        if (k == key) break;
        const u32 next = load<u32>(node + (k > key ? 12 : 16));
        if (next == 0) {
            store<u32>(s + 104, node);
            return 0;
        }
        const s32 c = static_cast<s16>(load<u16>(s + 100));
        store<u16>(s + 100, static_cast<u16>(c + 1));
        store<u32>(s + 20 + static_cast<u32>(c) * 4, node);
        node = next;
    }
    store<u32>(s + 104, node);
    store<u32>(kSetLast, load<u32>(node + 8));
    const u32 hit = load<u32>(s + 104);
    r3 = hit;
    if (hit == 0) return 0;
    return load<u32>(hit + 8);
}

// The component binding of the two spawners: the kind-5 component `hash`
// of e (vfn[32]()) drops its handler c->28 (vfn[8]()) for `h` (whose u16
// +4 count is bumped when set).
void BindHandler(PPUContext& ctx, u32 e, u32 hash, u32 ho) {
    const u32 c = static_cast<u32>(call(ctx, kFindComponent, {e, hash}));
    if (c == 0 || static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) return;
    const u32 old = load<u32>(c + 28);
    const u32 h = load<u32>(ho);
    if (old != 0) VCall(ctx, old, 8, {old});
    store<u32>(c + 28, h);
    if (h != 0) store<u16>(h + 4, static_cast<u16>(load<u16>(h + 4) + 1));
}
}  // namespace

// Spawn_Effect240288(Obj* o): the effect of the definition keyed by
// o->216->944 (SetLookup): a 140-byte effect e (SmallAlloc, Effect_Construct
// (e, def); 0 when the allocation fails) becomes o->548, its component
// 0x0027469E takes the handler o->160 (BindHandler), bit 0x200 of e->136 is
// cleared, Effect_Start(e) and Effect_SetTime(e; *0x00240284). r3 as left.
DECOMP(00240288, Spawn_Effect240288) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ko = load<u32>(o + 216);
    u32 r3 = 0;
    const u32 def = SetLookup(ctx, 0, true, ko, r3);
    ctx.gpr[3] = r3;
    if (def == 0) return;
    const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {140}));
    u32 e = 0;
    if (p != 0) {
        call(ctx, kEffectCtor, {p, def});
        e = p;
    }
    store<u32>(o + 548, e);
    BindHandler(ctx, e, 0x0027469Eu, o + 160);
    const u32 e2 = load<u32>(o + 548);
    store<u32>(e2 + 136, load<u32>(e2 + 136) & ~0x200u);
    call(ctx, kEffectStart, {load<u32>(o + 548)});
    callf(ctx, kEffectSetTime, {load<u32>(o + 548)}, {Lf(0x00240284u)});
}

// Effect* Spawn_Effect25A8A8(Obj* o, u32 key, u8 flag): the effect of the
// definition `key` (SetLookup; key 0: none): a 140-byte effect e
// (SmallAlloc + Effect_Construct(e, def), 0 when the allocation fails), its
// component 0x618B8D94 takes the handler o->2268 (BindHandler), bit 0x200 of
// e->136 = bit 0 of flag, Effect_Start(e), Effect_SetTime(e; *0x0025A8A4).
// r3 = e (0 when nothing was spawned).
DECOMP(0025A8A8, Spawn_Effect25A8A8) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 2);
    ctx.gpr[3] = 0;
    if (key == 0) return;
    u32 r3 = 0;
    const u32 def = SetLookup(ctx, key, false, 0, r3);
    if (def == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {140}));
    u32 e = 0;
    if (p != 0) {
        call(ctx, kEffectCtor, {p, def});
        e = p;
    }
    BindHandler(ctx, e, 0x618B8D94u, o + 2268);
    store<u32>(e + 136, (load<u32>(e + 136) & ~0x200u) | ((flag & 1) << 9));
    call(ctx, kEffectStart, {e});
    callf(ctx, kEffectSetTime, {e}, {Lf(0x0025A8A4u)});
    ctx.gpr[3] = e;
}

namespace {
constexpr u32 kObjFindComponent = 0x000D1288u;  // Obj_FindComponent(o, kind, 0)
constexpr u32 kFreeIndex = 0x002B92A0u;         // Obj_FreeIndex_002B92A0(c)
}  // namespace

// u32 Obj_CanTakeHitScaledE7FF0(Obj* o, Obj* src, s32 kind, f32* dmg): only
// in states (o->392 >> 1) 1 or above 6. Kind 2 with a free slot in the
// component 28 (Obj_FreeIndex_002B92A0 != -1) is accepted outright; kind 2
// with bit 0x00800000 of o->216->300, kind 1 and other kinds are refused.
// Kinds 0 / 2 are refused with bit 0x40000000 of o->420 or src == o->400,
// else *dmg *= 5.0 * o->216->296. Then the base filter of
// Obj_CanTakeHitBase_000DF960 (kind 3: component 27 check; the handler
// o->244; src bit 0x01 of +192; o->vfn[300] / src->vfn[296] masks).
DECOMP(000E7FF0, Obj_CanTakeHitScaledE7FF0) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), kind = arg<u32>(ctx, 2), dmg = arg<u32>(ctx, 3);
    const u32 def = load<u32>(o + 216);
    const s32 st = static_cast<s32>(load<u32>(o + 392) >> 1);
    ctx.gpr[3] = 0;
    if (!(st == 1 || st > 6)) return;
    const s32 k = static_cast<s32>(kind);
    bool scale;
    if (k == 2) {
        const u32 c = static_cast<u32>(call(ctx, kObjFindComponent, {o, 28, 0}));
        if (c != 0 && static_cast<s32>(call(ctx, kFreeIndex, {c})) != -1) {
            ctx.gpr[3] = 1;
            return;
        }
        if ((load<u32>(def + 300) & 0x00800000u) != 0) {
            ctx.gpr[3] = 0;
            return;
        }
        scale = true;
    } else if (k == 0) {
        scale = true;
    } else if (k == 3) {
        scale = false;
    } else {
        return;
    }
    if (scale) {
        if ((load<u32>(o + 420) & 0x40000000u) != 0 || (src != 0 && load<u32>(o + 400) == src)) {
            ctx.gpr[3] = 0;
            return;
        }
        const f64 v = S(S(Lf(def + 296) * Lf(0x000E7FECu)) * Lf(dmg));
        ctx.fpr[1] = v;
        Sf(dmg, v);
    }
    if (k == 3) {
        const u32 c = static_cast<u32>(call(ctx, kObjFindComponent, {o, 27, 0}));
        if (c == 0 || load<u8>(c + 16) == 0 || static_cast<s32>(load<u32>(c + 68)) > 1) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    const u32 h = load<u32>(o + 244);
    if (h != 0 && static_cast<u32>(VCall(ctx, h, 84, {h})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (src == 0 || (load<u32>(src + 192) & 1) == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 a = static_cast<u32>(VCall(ctx, o, 300, {o, kind, src}));
    if (a == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 b = static_cast<u32>(VCall(ctx, src, 296, {src, kind, o}));
    ctx.gpr[3] = (b & a) == a ? 1 : 0;
}

namespace {
// Drops the handler of the kind-5 component `hash` of fx (vfn[32]() == 5):
// c->28->vfn[8]() when set, then c->28 = c->36 = 0.
void DropHandler(PPUContext& ctx, u32 fx, u32 hash) {
    const u32 c = static_cast<u32>(call(ctx, kFindComponent, {fx, hash}));
    if (c == 0 || static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) return;
    const u32 h = load<u32>(c + 28);
    if (h != 0) VCall(ctx, h, 8, {h});
    store<u32>(c + 28, 0);
    store<u32>(c + 36, 0);
}
}  // namespace

// Obj_ReleaseAll3BB1AC(Obj* o): with an effect o->32: the world w =
// o->vfn[76]()'s player p = *(w + 0x1281E0) loses the component 0x760CB725
// handler of o->32 (DropHandler), p->vfn[176](p, 0) and bit 0x1000 of the
// u16 p->164->40; then every member e = +80 of the u16 o->112 128-byte
// entries at o->104 gets e->vfn[176](e, 0), bit 0x1000 of e->164->40 (with
// e->164) cleared and the o->32 component e->216->52 dropped (when both are
// set). r3 as the original leaves it.
DECOMP(003BB1AC, Obj_ReleaseAll3BB1AC) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = load<u32>(o + 32);
    if (ctx.gpr[3] == 0) return;
    const u32 w = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 fx = load<u32>(o + 32);
    const u32 p = load<u32>(w + 0x1281E0u);
    if (fx != 0) DropHandler(ctx, fx, 0x760CB725u);
    VCall(ctx, p, 176, {p, 0});
    const u32 x = load<u32>(p + 164);
    store<u16>(x + 40, static_cast<u16>(load<u16>(x + 40) & ~0x1000));
    u32 i = 0;
    for (u32 off = 0;; off += 128, ++i) {
        const u32 n = load<u16>(o + 112);
        ctx.gpr[3] = n;
        if (!(i < n)) return;
        const u32 e = load<u32>(load<u32>(o + 104) + off + 80);
        if (e == 0) continue;
        VCall(ctx, e, 176, {e, 0});
        const u32 y = load<u32>(e + 164);
        if (y != 0) store<u16>(y + 40, static_cast<u16>(load<u16>(y + 40) & ~0x1000));
        const u32 d = load<u32>(e + 216);
        const u32 f = load<u32>(o + 32);
        const u32 hash = load<u32>(d + 52);
        if (f == 0 || hash == 0) continue;
        DropHandler(ctx, f, hash);
    }
}

namespace {
constexpr u32 kUiFind = 0x004D5544u;  // Ui_Find(name, parent)

// Sets the label of the option widget `name` under menu (Ui_Find, a kind-3
// widget: vfn[12]() == 3) to `on` / `off` by bit `bit` of *flags:
// w->vfn[188](w, label, 0).
void SetOptionLabel(PPUContext& ctx, u32 menu, u32 name, u32 flags, u32 bit, u32 on, u32 off) {
    const u32 w = static_cast<u32>(call(ctx, kUiFind, {name, menu}));
    if (w == 0 || static_cast<u32>(VCall(ctx, w, 12, {w})) != 3) return;
    VCall(ctx, w, 188, {w, (load<u32>(flags) & bit) != 0 ? on : off, 0});
}
}  // namespace

// Options_RefreshLabels46D398(Options* a, Screen* s): under the "Menu" widget
// of s->12 the "CameraX" / "CameraY" options show "$STR_INVERTED" /
// "$STR_NORMAL" by bits 0x40000000 / 0x20000000 of a->4144 and "Vibration"
// "$STR_ON" / "$STR_OFF" by bit 0x10000000. r3 as the original leaves it.
DECOMP(0046D398, Options_RefreshLabels46D398) {
    const u32 a = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u32 flags = a + 4144;
    constexpr u32 kInverted = 0x00AE88A0u, kNormal = 0x00AE88B0u, kOn = 0x00AE8818u, kOff = 0x00AE8820u;
    const u32 menu = static_cast<u32>(call(ctx, kUiFind, {0x00AE8800u, load<u32>(s + 12)}));  // "Menu"
    if (menu == 0) return;
    SetOptionLabel(ctx, menu, 0x00AE8898u, flags, 0x40000000u, kInverted, kNormal);  // "CameraX"
    SetOptionLabel(ctx, menu, 0x00AE88C0u, flags, 0x20000000u, kInverted, kNormal);  // "CameraY"
    SetOptionLabel(ctx, menu, 0x00AE88C8u, flags, 0x10000000u, kOn, kOff);           // "Vibration"
}

namespace {
constexpr u32 kFreeHandle = 0x004B2A04u;   // Pool_FreeHandle_004B2A04(pool, h)
constexpr u32 kEmitterStop = 0x00443424u;  // Emitter_Stop(handle, fade)
constexpr u32 kVecClear = 0x0054EC1Cu;     // PtrVector_Clear(v)
constexpr u32 kVecNop = 0x0054EC18u;       // Nop(v)
constexpr u32 kListRemove = 0x000D6940u;   // List1013B5C_Remove(x)
}  // namespace

// EmitterOwner_Destruct297900(Owner* o): the destructor body (vtable
// 0x00B1C880): the u16 o->368 handles of o->360 go back to the pool
// (Pool_FreeHandle_004B2A04(*0x00F557F8 + 4, h)); while s32 o->448 > 0 the
// first of the 8-byte (handle, x) pairs at o+384 is stopped (Emitter_Stop(h,
// 1)) and replaced by the last one (whose slot is zeroed); PtrVector_Clear /
// Nop on o+360; then the base part (vtable 0x00B1BE30): o is removed from
// the 8-byte entries (+4 = object) of the u16-counted table 0x01056F1C
// (+2 count, +8 entries; later entries move down) and List1013B5C_Remove(o).
// r3 = that call's result.
DECOMP(00297900, EmitterOwner_Destruct297900) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1C880u);
    for (u32 i = 0; i < load<u16>(o + 368); ++i) {
        const u32 v = load<u32>(o + 360);
        const u32 pool = load<u32>(0x00F557F8u) + 4;
        call(ctx, kFreeHandle, {pool, load<u32>(v + 4 * i)});
    }
    if (static_cast<s32>(load<u32>(o + 448)) > 0) {
        do {
            call(ctx, kEmitterStop, {load<u32>(o + 384), 1});
            const u32 n = load<u32>(o + 448) - 1;
            store<u32>(o + 448, n);
            store<u32>(o + 384, load<u32>(o + 384 + 8 * n));
            store<u32>(o + 388, load<u32>(o + 388 + 8 * load<u32>(o + 448)));
            store<u32>(o + 384 + 8 * load<u32>(o + 448), 0);
            store<u32>(o + 388 + 8 * load<u32>(o + 448), 0);
        } while (static_cast<s32>(load<u32>(o + 448)) > 0);
    }
    call(ctx, kVecClear, {o + 360});
    call(ctx, kVecNop, {o + 360});
    constexpr u32 T = 0x01056F1Cu;
    store<u32>(o, 0x00B1BE30u);
    const u32 n = load<u16>(T + 2);
    const u32 arr = load<u32>(T + 8);
    if (n != 0) {
        for (u32 i = 0; i < n; ++i) {
            if (load<u32>(arr + 8 * i + 4) != o) continue;
            u32 r4 = n;
            if (static_cast<s32>(n) >= 2 && static_cast<s32>(i) < static_cast<s32>(n - 1)) {
                for (s32 j = static_cast<s32>(i);;) {
                    store<u32>(arr + 8 * j, load<u32>(arr + 8 * j + 8));
                    store<u32>(arr + 8 * j + 4, load<u32>(arr + 8 * j + 12));
                    ++j;
                    r4 = load<u16>(T + 2);
                    if (!(j < static_cast<s32>(r4 - 1))) break;
                }
            }
            store<u16>(T + 2, static_cast<u16>(r4 - 1));
            break;
        }
    }
    ctx.gpr[3] = call(ctx, kListRemove, {o});
}

namespace {
constexpr u32 kWorldPtr = 0x00F65CA0u;   // World* (slots of 0x20C80 bytes at +4208)
constexpr u32 kSetU8_214 = 0x001071C4u;  // SetU8_214_001071C4(x, v)
constexpr u32 kReset84 = 0x0027C8C8u;    // Obj_Reset84_0027C8C8(x, 0)
constexpr u32 kSetNot88 = 0x0027C98Cu;   // Field_SetNot88b_0027C98C(x, 0)
constexpr u32 kFlagSet8 = 0x0042CA60u;   // Flag_Set8_40000000(o)
}  // namespace

// Cmd_ResetPlayer434EFC(Cmd* o) (vtable entry): with a player p = *(world +
// 0x1281E0): o+16.. takes the pose p->160->160 (with p->160; rows +0 / +16 / +32 / +48 ->
// o+16 / +32 / +48 / +64) when set; p->vfn[176](p, 0) runs with p->592
// temporarily 0.5f; the current weapon x = p+364[u8 p->345] (0 for slot 0,
// called anyway: quirk) gets SetU8_214_001071C4(x, 1) when x->vfn[8]() is
// 11. The world's first slot object q (bit 0x08 of q->192) gets its
// q->384 vfn[44](1) + vfn[16]() and Obj_Reset84 / Field_SetNot88b(r, 0) for
// its kind-5 record r = q->380 (or 0). Then Flag_Set8_40000000(o); r3 = its
// result.
DECOMP(00434EFC, Cmd_ResetPlayer434EFC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(load<u32>(kWorldPtr) + 0x1281E0u);
    if (p != 0) {
        const u32 m = load<u32>(p + 160);
        const u32 w592 = load<u32>(p + 592);
        if (m != 0) {
            const u32 mat = load<u32>(m + 160);
            for (u32 row = 0; row < 4; ++row)
                for (u32 k = 0; k < 3; ++k) Cw(o + 16 + 16 * row + 4 * k, mat + 16 * row + 4 * k);
            ctx.fpr[1] = Lf(mat + 56);
        }
        store<u32>(p + 592, 0x3F000000u);
        VCall(ctx, p, 176, {p, 0});
        const u32 b = load<u8>(p + 345);
        store<u32>(p + 592, w592);
        const u32 x = b != 0 ? load<u32>(p + 364 + 4 * b) : 0;
        if (static_cast<u32>(VCall(ctx, x, 8, {x})) == 11) call(ctx, kSetU8_214, {x, 1});
    }
    const u32 q = load<u32>(load<u32>(kWorldPtr) + 4208 + 8);
    if (q != 0 && (load<u32>(q + 192) & 0x8) != 0) {
        const u32 h = load<u32>(q + 384);
        if (h != 0) {
            VCall(ctx, h, 44, {h, 1});
            VCall(ctx, h, 16, {h});
        }
        const u32 r = load<u32>(q + 380);
        const u32 rec = (r != 0 && load<u32>(r + 12) == 5) ? r : 0;
        call(ctx, kReset84, {rec, 0});
        call(ctx, kSetNot88, {rec, 0});
    }
    ctx.gpr[3] = call(ctx, kFlagSet8, {o});
}

namespace {
constexpr u32 kReserve = 0x00551718u;  // PtrVector_Reserve(v, n)

// Appends w to the u16-counted vector at v (data +0, capacity +4, count +8,
// high-water mark +10), doubling the capacity with PtrVector_Reserve when
// full, as the original's inline code does.
void PushU16Vec(PPUContext& ctx, u32 v, u32 w) {
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    u32 data = load<u32>(v);
    if (n == cap) {
        call(ctx, kReserve, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + 8);
        data = load<u32>(v);
    }
    store<u32>(data + ((n << 2) & 0x3FFFCu), w);
    const u32 n1 = load<u16>(v + 8) + 1u;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
}
}  // namespace

// Obj_AddContact2AEE58(Obj* o, Pair* pr): up to 16 contacts. The other side
// x of the pair (pr->4, or pr->8 when that is o->8->vfn[144]()) counts when
// its property table (x->124, s32 x->128 16-byte entries) has the key
// 0x91C7CF5B with a non-zero word +12 (x->128 is read before the null test:
// quirk). A known contact (in o+240) bumps its counter o->280[i]; a new one
// is appended to o+240 and gets the counter 1 appended to o+280; unless it
// is in the list o->260 (u16 o->268), bit 0x80 of the byte o->336 is set.
// r3 as the original leaves it.
DECOMP(002AEE58, Obj_AddContact2AEE58) {
    const u32 o = arg<u32>(ctx, 0), pr = arg<u32>(ctx, 1);
    ctx.gpr[3] = load<u16>(o + 248);
    if (ctx.gpr[3] > 16) return;
    const u32 h = load<u32>(o + 8);
    const u32 me = static_cast<u32>(VCall(ctx, h, 144, {h}));
    const u32 v = load<u32>(o + 240);
    const u32 n = load<u16>(o + 248);
    u32 x = load<u32>(pr + 4);
    if (x == me) x = load<u32>(pr + 8);
    u32 val = 0;
    for (s32 i = 0; i < static_cast<s32>(load<u32>(x + 128)); ++i) {
        const u32 e = load<u32>(x + 124) + 16 * i;
        if (load<u32>(e) == 0x91C7CF5Bu) {
            val = load<u32>(e + 12);  // ld +8, low word tested
            break;
        }
    }
    ctx.gpr[3] = n;
    if (x == 0 || val == 0) return;
    for (u32 j = 0; j < n; ++j) {
        if (load<u32>(v + 4 * j) != x) continue;
        const u32 c = load<u32>(o + 280) + 4 * j;
        store<u32>(c, load<u32>(c) + 1);
        ctx.gpr[3] = c;
        return;
    }
    PushU16Vec(ctx, o + 240, x);
    PushU16Vec(ctx, o + 280, 1);
    const u32 m = load<u16>(o + 268);
    const u32 lst = load<u32>(o + 260);
    for (u32 j = 0; j < m; ++j) {
        if (load<u32>(lst + 4 * j) == x) {
            ctx.gpr[3] = m;
            return;
        }
    }
    const u32 bt = load<u8>(o + 336) | 0x80u;
    store<u8>(o + 336, static_cast<u8>(bt));
    ctx.gpr[3] = bt;
}

namespace {
constexpr u32 kHandleOpen = 0x00443CE0u;  // 0x00443CE0(handle, Params* p) -> 0 when the emitter is gone
constexpr u32 kRecDtor = 0x00440AC4u;     // SpawnRec_Dtor_00440AC4(p, 0)
constexpr u32 kRecDtor2 = 0x00440A40u;    // SpawnRec_Dtor2_00440A40(p, 0)
constexpr u32 kRestVec = 0x01164B40u;     // f32[3] copied into the default parameters
}  // namespace

// EmitterOwner_Refresh297BA8(Owner* o): builds an emitter parameter block on
// the stack (as Handle_Acquire443EC0: two copies of *0x01164B40, 1.0f,
// zeros; with an owner object o->8 its position +144 and +320 instead and
// the flags +56 = 0x0A), passes it to 0x00443CE0 for each (handle, x) pair
// of o+384 (s32 count o->448); a pair whose emitter is gone (0) is stopped
// (Emitter_Stop(h, 1)) and replaced by the last pair (zeroed), and the same
// slot is examined again. Then SpawnRec_Dtor_00440AC4 / SpawnRec_Dtor2(block,
// 0); r3 = the last call's result.
DECOMP(00297BA8, EmitterOwner_Refresh297BA8) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 144);
    const u32 L = frame.addr();  // sp+112; words +12 / +28 / +52 stay unset
    store<u32>(L + 56, 0);
    for (u32 k = 0; k < 3; ++k) {
        Cw(L + 4 * k, kRestVec + 4 * k);
        Cw(L + 16 + 4 * k, kRestVec + 4 * k);
    }
    ctx.fpr[1] = Lf(kRestVec);
    const u32 own = load<u32>(o + 8);
    store<u32>(L + 64, 0);
    store<u32>(L + 60, 0);
    store<u8>(L + 44, 0);
    store<u32>(L + 48, 0);
    store<u32>(L + 40, 0);
    store<u32>(L + 36, 0);
    store<u32>(L + 32, 0x3F800000u);
    store<u8>(L + 45, 0);
    store<u8>(L + 46, 0);
    store<u8>(L + 47, 0);
    const u32 fl = load<u32>(L + 56);
    if (own != 0) {
        for (u32 k = 0; k < 3; ++k) Cw(L + 4 * k, own + 144 + 4 * k);
        store<u32>(L + 56, fl | 2);
        for (u32 k = 0; k < 3; ++k) Cw(L + 16 + 4 * k, own + 320 + 4 * k);
        store<u32>(L + 56, fl | 0xA);
        ctx.fpr[1] = Lf(own + 328);
    }
    if (load<u32>(o + 448) != 0) {
        u32 pa = o + 384, pb = o + 388;
        for (u32 i = 0; i < load<u32>(o + 448); ++i, pa += 8, pb += 8) {
            if (call(ctx, kHandleOpen, {load<u32>(pa), L}) != 0) continue;
            call(ctx, kEmitterStop, {load<u32>(pa), 1});
            const u32 n = load<u32>(o + 448) - 1;
            store<u32>(o + 448, n);
            store<u32>(pa, load<u32>(o + 384 + 8 * n));
            store<u32>(pb, load<u32>(o + 388 + 8 * load<u32>(o + 448)));
            store<u32>(o + 384 + 8 * load<u32>(o + 448), 0);
            store<u32>(o + 388 + 8 * load<u32>(o + 448), 0);
            pa -= 8;
            pb -= 8;
            --i;
        }
    }
    call(ctx, kRecDtor, {L, 0});
    ctx.gpr[3] = call(ctx, kRecDtor2, {L, 0});
}

namespace {
constexpr u32 kEventInit = 0x00464ED4u;   // Event1_Init_00464ED4(ev, obj, a, b, x, y, z)
constexpr u32 kWorldQuery = 0x00482D14u;  // World_Query482D14(world, user, ev)
}  // namespace

// u32 Ai_RespawnTick4F530(Ai* o) (vtable entry): nothing in states o->4 20 /
// 21 (r3 = 0). A pending request (bit 0x80 of the byte o->305, cleared here)
// whose ratio d->348 / d->344 (d = o->60) is within the row limit +72 of the
// 76-byte row (o->1800 + 0x6C271198) at 0x00F98684 raises d->348 to
// max(min(d->344 * limit, d->344), 0.0) (the constant at 0x0004F528); in state
// 18 that also posts a zeroed event (Event1_Init_00464ED4(ev, d, d->216 ?
// d->216->52 : 0, o->1800, 0, 0, 0), World_Query482D14(*0x00F65CA0, -1, ev))
// and switches o->vfn[220](o, 20 for o->1800 == 0x93D8EE6A else 16); r3 = 1.
// Otherwise, once s32 o->1764 reaches *0x00F98848: o->1764 = 0, o->1812 = 1,
// o->vfn[220](o, 18) and r3 = 1; else 0. f1 as the original leaves it.
DECOMP(0004F530, Ai_RespawnTick4F530) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 48);
    const u32 ev = frame.addr();
    const s32 s = static_cast<s32>(load<u32>(o + 4));
    if (s == 20 || s == 21) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 b = load<u8>(o + 305);
    if ((b & 0x80) != 0) {
        store<u8>(o + 305, static_cast<u8>(b & ~0x80));
        const u32 d = load<u32>(o + 60);
        const f64 a344 = Lf(d + 344);
        const f64 ratio = Divs(Lf(d + 348), a344);
        const u32 key = load<u32>(o + 1800);
        const f64 lim = Lf(0x00F98684u + 76 * (key + 0x6C271198u) + 72);
        ctx.fpr[1] = lim;
        if (ratio <= lim) {
            const f64 v = S(a344 * lim);
            ctx.fpr[1] = v;
            const f64 mn = v < a344 ? v : a344;
            const f64 c = Lf(0x0004F528u);
            Sf(d + 348, mn > c ? mn : c);
            if (s == 18) {
                for (u32 k = 0; k < 24; k += 4) store<u32>(ev + k, 0);
                const u32 h = load<u32>(d + 216);
                call(ctx, kEventInit, {ev, d, h != 0 ? load<u32>(h + 52) : 0, key, 0, 0, 0});
                call(ctx, kWorldQuery, {load<u32>(kWorldPtr), ~0ull, ev});
                VCall(ctx, o, 220, {o, load<u32>(o + 1800) == 0x93D8EE6Au ? 20u : 16u});
                ctx.gpr[3] = 1;
                return;
            }
        }
    }
    if (static_cast<s32>(load<u32>(o + 1764)) < static_cast<s32>(load<u32>(0x00F98848u))) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o + 1764, 0);
    store<u8>(o + 1812, 1);
    VCall(ctx, o, 220, {o, 18});
    ctx.gpr[3] = 1;
}
