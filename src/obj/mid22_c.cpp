// Game code, medium sweep part 22: section acquire, distance cues, player
// rate checks and a constructor.
#include "mid22.h"

using namespace sly4;
using namespace sly4::mid22;

namespace {
constexpr u32 kFindById = 0x000D6A28u;       // Obj_FindById(id)
constexpr u32 kPtrReserve = 0x00551718u;     // PtrVector_Reserve(v, n)
constexpr u32 kSectionMode = 0x00333BC8u;    // 0x00333BC8(o, mode, 1)
constexpr u32 kSetFlagHash = 0x0048FA54u;    // Obj_SetFlagHash_0048FA54(o, hash, b)
constexpr u32 kFindComponent = 0x004BB4B4u;  // Effect_FindComponent(e, hash)
constexpr u32 kSetHash = 0x004CF988u;        // Effect_SetHash4CF988(fx, hash)
constexpr u32 kEffectCheck = 0x004CF620u;    // Effect_Check4CF620(fx, 0)
constexpr u32 kTrackSet36 = 0x0029879Cu;     // Anim_TrackSet36_0029879C(an, 0, 0)
constexpr u32 kAnimRestart = 0x002775A8u;    // Anim_Restart2775A8
constexpr u32 kSinf = 0x007345F0u;           // Math_Sinf(f1)
constexpr u32 kSetRate2 = 0x00236868u;       // Player_SetRate2_236868(p, -; f1, f2)

// Appends x to the pointer vector v {+0 data, u16 +4 capacity, u16 +8
// count, u16 +10 high-water}, doubling the capacity when full.
void PushPtr(PPUContext& ctx, u32 v, u32 x) {
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, kPtrReserve, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + 8);
    }
    store<u32>(load<u32>(v) + ((n << 2) & 0x3FFFCu), x);
    const u32 c1 = load<u16>(v + 8) + 1u;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(c1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c1 & 0xFFFFu) ? hw : c1));
}
}  // namespace

// Section_AcquireAll334B74(Section* o): unless already active (u8 o->2680,
// r3 = that byte): the ids of the u16 o->2600 list o->2592 and the u16
// o->2620 list o->2612 are resolved (Obj_FindById); each object found gets
// its u16 +178 use count raised and is appended to the vector o+2632 / o+2652.
// Then 0x00334954(o, 1), Obj_SetFlagHash(o, 0x7BADF81E, 0), o->2681 = 1,
// 0x00333BC8(o, 2, 1) and o->2680 = 1 (the inverse of
// Section_ReleaseAll334114).
DECOMP(00334B74, Section_AcquireAll334B74) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 act = load<u8>(o + 2680);
    if (act != 0) {
        ctx.gpr[3] = act;
        return;
    }
    static constexpr u32 kLists[2][3] = {{2592, 2600, 2632}, {2612, 2620, 2652}};
    for (const auto& l : kLists) {
        for (u32 k = 0; k < load<u16>(o + l[1]); ++k) {
            const u32 x = static_cast<u32>(call(ctx, kFindById, {load<u32>(load<u32>(o + l[0]) + 4 * k)}));
            if (x == 0) continue;
            store<u16>(x + 178, static_cast<u16>(load<u16>(x + 178) + 1));
            PushPtr(ctx, o + l[2], x);
        }
    }
    call(ctx, 0x00334954u, {o, 1});
    call(ctx, kSetFlagHash, {o, 0x7BADF81Eu, 0});
    store<u8>(o + 2681, 1);
    call(ctx, kSectionMode, {o, 2, 1});
    store<u8>(o + 2680, 1);
}

// Obj_RangeCue3B3298(Obj* o): with a body o->104: its offset vfn[140]
// (into sp+144) at distance d >= def o->96's +196 picks the clip +212 and
// the cue 0x7B2E3209, else +208 and 0x002237D4. A clip restarts on the
// body's animator (vfn[60]) when Anim_TrackSet36(an, 0, 0) (constants
// 0x003B3288; r8 passed through). Unless o->124: the body's effect
// (vfn[272]) passing Effect_Check(e, 0) gets the cue unless already set
// (e->4->8). r3 / f1 as the original leaves them.
DECOMP(003B3298, Obj_RangeCue3B3298) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 192);
    const u32 sp = frame.addr();
    constexpr u32 C = 0x003B3288u;
    const u32 b = load<u32>(o + 104);
    if (b == 0) return;  // r3 = o
    VCall(ctx, b, 140, {sp + 144, b});
    const f64 x = Lf(sp + 144), y = Lf(sp + 148), z = Lf(sp + 152);
    const u32 w = load<u32>(o + 96);
    const f64 d = Sqrts(Fmadds(z, z, Fmadds(x, x, S(y * y))));
    ctx.fpr[1] = d;
    u32 key, hash;
    if (d >= Lf(w + 196)) {
        key = load<u32>(w + 212);
        hash = 0x7B2E3209u;
    } else {
        key = load<u32>(w + 208);
        hash = 0x002237D4u;
    }
    if (key != 0) {
        const u32 b1 = load<u32>(o + 104);
        if (VCall(ctx, b1, 60, {b1}) != 0) {
            const u32 b2 = load<u32>(o + 104);
            const u32 an = static_cast<u32>(VCall(ctx, b2, 60, {b2}));
            if (call(ctx, kTrackSet36, {an, 0, 0}) != 0) {
                const u32 b3 = load<u32>(o + 104);
                const u32 an3 = static_cast<u32>(VCall(ctx, b3, 60, {b3}));
                // r8 as the vfn[60] call leaves it; arguments 9 / 10 are frame garbage
                callf(ctx, kAnimRestart, {an3, key, 0, 0, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull},
                      {Lf(C), Lf(C + 4), Lf(C + 8), Lf(C + 8)});
            }
        }
    }
    const u32 busy = load<u32>(o + 124);
    ctx.gpr[3] = busy;
    if (busy != 0) return;
    const u32 b4 = load<u32>(o + 104);
    const u32 e = static_cast<u32>(VCall(ctx, b4, 272, {b4}));
    if (e == 0) return;
    if (call(ctx, kEffectCheck, {e, 0}) == 0) return;
    const u32 st = load<u32>(e + 4);
    ctx.gpr[3] = st;
    if ((st != 0 ? load<u32>(st + 8) : 0) == hash) return;
    call(ctx, kSetHash, {e, hash});
}

// Player_RateCheck23B3E8(Player* p): with the rig flag p->396->1592: when a
// rate p->976 or p->984 is positive while p->608 < 0 (K = 0x0023B3D4: 0.0,
// K1, K2, K3): the cached sin(K1) (u8 0x01056ED8 / f32 0x01056EE0, computed
// once) above p->116 resets the rates (Player_SetRate2(p; 0, K2), p->vfn[44]);
// then K3 plus K0 per entry of p->612 (u8 count +28->32, 224-byte entries
// +72 with +36 set), over the count, unless 0 also resets them. Quirk: p->612
// (and its +28) are read without a check on that path. r3 / f1 as the
// original leaves them.
DECOMP(0023B3E8, Player_RateCheck23B3E8) {
    const u32 p = arg<u32>(ctx, 0);
    constexpr u32 K = 0x0023B3D4u;
    const u32 f = load<u8>(load<u32>(p + 396) + 1592);
    ctx.gpr[3] = f;
    if (f == 0) return;
    const f64 k0 = Lf(K);
    const f64 v608 = Lf(p + 608);
    ctx.fpr[1] = v608;
    const bool a = Lf(p + 976) > k0 && v608 < k0;
    const bool b = Lf(p + 984) > k0 && v608 < k0;
    ctx.gpr[3] = b;
    if (!a && !b) return;
    if (load<u8>(0x01056ED8u) == 0) {
        Sf(0x01056EE0u, callf(ctx, kSinf, {}, {Lf(K + 4)}));
        store<u8>(0x01056ED8u, 1);
    }
    if (Lf(0x01056EE0u) > Lf(p + 116)) {
        callf(ctx, kSetRate2, {p, K}, {k0, Lf(K + 8)});
        VCall(ctx, p, 44, {p});
    }
    f64 sum = Lf(K + 12);
    const u32 g = load<u32>(p + 612);
    if (g != 0 && load<u32>(g + 28) != 0) {
        const s32 n = static_cast<s8>(load<u8>(load<u32>(g + 28) + 32));
        if (n != 0)
            for (u32 i = 0; i < static_cast<u32>(n); ++i)
                if (load<u32>(load<u32>(g + 72) + 224 * i + 36) != 0) sum = S(sum + k0);
    }
    const s32 n2 = static_cast<s8>(load<u8>(load<u32>(g + 28) + 32));  // quirk: unchecked
    ctx.fpr[1] = Divs(sum, S(static_cast<f64>(n2)));
    ctx.gpr[3] = static_cast<u32>(n2);
    if (ctx.fpr[1] == k0) return;
    callf(ctx, kSetRate2, {p, K}, {k0, Lf(K + 8)});
    VCall(ctx, p, 44, {p});
}

// Obj_Construct1AACEC(Obj* o, Params* p): base Thunk_000E40B4(o, p); vtables
// 0x00B15550 (+380 0x00B156FC, +384 0x00B15724); p->208..224 -> o->424..440;
// o->444..464 = 0; flags o->468: bit 0x80000000 and 0x40000000 from p->228,
// 0x20000000 cleared. Effect o->356's kind-1 components 0xBBBC16FF / 0x3F5805D6
// go to o->444 / o->448 (their +20 = 0). o->464 = +28 of entry def o->216's
// +324 of the 48-byte vector o->388 (u16 count +8; 0 when out of range).
// Obj_SetFlagHash(o, 0x5924BAFA, 1); r3 = 0x00464D1C(o, 0, 0).
DECOMP(001AACEC, Obj_Construct1AACEC) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    call(ctx, 0x000E40B4u, {o, p});
    constexpr u32 V = 0x00B15548u;
    store<u32>(o + 380, V + 436);
    store<u32>(o + 384, V + 476);
    Cw(o + 424, p + 208);
    store<u32>(o + 0, V + 8);
    Cw(o + 428, p + 212);
    Cw(o + 432, p + 216);
    store<u32>(o + 444, 0);
    Cw(o + 436, p + 220);
    store<u32>(o + 448, 0);
    Cw(o + 440, p + 224);
    ctx.fpr[1] = Lf(p + 224);
    for (u32 k = 452; k <= 464; k += 4) store<u32>(o + k, 0);
    u32 fl = (load<u32>(o + 468) & 0x7FFFFFFFu) | (load<u32>(p + 228) & 0x80000000u);
    const u32 e = load<u32>(o + 356);
    const u32 def = load<u32>(o + 216);
    store<u32>(o + 468, fl);
    fl = (fl & ~0x40000000u) | (load<u32>(p + 228) & 0x40000000u);
    fl &= ~0x20000000u;
    store<u32>(o + 468, fl);
    if (e != 0) {
        u32 c = static_cast<u32>(call(ctx, kFindComponent, {e, 0xBBBC16FFu}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 1) {
            store<u32>(o + 444, c);
            store<u32>(c + 20, 0);
        }
        c = static_cast<u32>(call(ctx, kFindComponent, {load<u32>(o + 356), 0x3F5805D6u}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 1) {
            store<u32>(o + 448, c);
            store<u32>(c + 20, 0);
        }
    }
    const u32 n = load<u32>(def + 324);
    const u32 v = load<u32>(o + 388);
    u32 val = 0;
    if (v != 0 && n < load<u16>(v + 8)) val = load<u32>(load<u32>(v) + 48 * n + 28);
    store<u32>(o + 464, val);
    call(ctx, kSetFlagHash, {o, 0x5924BAFAu, 1});
    call(ctx, 0x00464D1Cu, {o, 0, 0});
}

namespace {
constexpr u32 kFindByHandle = 0x0042C2B0u;  // Obj_FindByHandle42C2B0(handle, 0)
constexpr u32 kCallK2 = 0x000D94F4u;        // CallK2_000D94F4(o): Obj_FindComponent(o, 19, 0)
constexpr u32 kRemoveAll = 0x000D91BCu;     // List_RemoveAll_000D91BC(o, x)
constexpr u32 kTrackSet164 = 0x000D8760u;   // Anim_TrackSet164_000D8760(o, 0, 0)
constexpr u32 kApplyTrackKey = 0x000B6A58u;  // Obj_ApplyTrackKey_000B6A58(o)

// x->vfn[72](x, key, opd, entry, r7, r8, 0; k, k, k, k): the original call
// leaves the OPD / entry in r5 / r6, passes r7 / r8 through and zeroes r9.
u64 Vfn72(PPUContext& ctx, u32 x, u32 key, f64 k) {
    const u32 opd = load<u32>(load<u32>(x) + 72);
    ctx.fpr[1] = ctx.fpr[2] = ctx.fpr[3] = ctx.fpr[4] = k;
    return call_ptr(ctx, opd, {x, key, opd, load<u32>(opd), ctx.gpr[7], ctx.gpr[8], 0});
}
}  // namespace

// Cmd_Attach42F620(Cmd* c): the object x of handle c->12 (Obj_FindByHandle)
// and its component 19 (CallK2) a: with a key c->20, x->vfn[72](key; K x4,
// K = *0x0042F618) must accept and a is fetched again; without one, a and its
// +4 (the old key) must exist. The target t of handle c->16 must be active
// (bit 0x01 of +192); then 0x00296810(a, t, c->24). With c->28: the old key
// (or, without one, after List_RemoveAll(x, a) when a) is replayed through
// vfn[72]. r3 = 0x0042CA60(c) (or as the original leaves it on the way out).
DECOMP(0042F620, Cmd_Attach42F620) {
    const u32 c = arg<u32>(ctx, 0);
    const f64 K = Lf(0x0042F618u);
    u32 old = 0, a = 0;
    const u32 x = static_cast<u32>(call(ctx, kFindByHandle, {load<u32>(c + 12), 0}));
    if (x == 0) return;
    if (load<u32>(c + 20) == 0) {
        a = static_cast<u32>(call(ctx, kCallK2, {x}));
        if (a == 0) return;
        old = load<u32>(a + 4);
        if (old == 0) return;
    } else {
        const u32 a0 = static_cast<u32>(call(ctx, kCallK2, {x}));
        const u32 key = load<u32>(c + 20);
        if (a0 != 0) old = load<u32>(a0 + 4);
        if (static_cast<u32>(Vfn72(ctx, x, key, K)) == 0) return;
        a = static_cast<u32>(call(ctx, kCallK2, {x}));
        if (a == 0) return;
    }
    const u32 t = static_cast<u32>(call(ctx, kFindByHandle, {load<u32>(c + 16), 0}));
    if (t == 0 || (load<u32>(t + 192) & 1u) == 0) return;  // r3 = t
    call(ctx, 0x00296810u, {a, t, load<u32>(c + 24)});
    if (load<u8>(c + 28) != 0) {
        if (old == 0 && a != 0) call(ctx, kRemoveAll, {x, a});
        Vfn72(ctx, x, old, K);
    }
    call(ctx, 0x0042CA60u, {c});
}

// Obj_Layer2TickBE18C(Obj* o) vtable entry: layer-2 state s = o->vfn[32](o,
// 2) over the slot q = o->20 + u16 o->6. 0: cue 0x18FBE4D7 (o->356), vfn[28]
// (2, 1). 1 / 2 once the animator settles (Anim_TrackSet36(o->720) or
// Anim_TrackSet164(o)): 1 -> cue 0x7B320099, 0x000B3A60(o->480, q->0, q->4),
// vfn[28](2, 2); 2 -> q->8 -= 1, still positive: cue 0xCF934AEC, vfn[28](2,
// 3), else Obj_ApplyTrackKey(o). 3: o->12->8 >= q->12 -> vfn[28](2, 0). r3 /
// f1 as the original leaves them.
DECOMP(000BE18C, Obj_Layer2TickBE18C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + 20) + load<u16>(o + 6);
    const s32 s = static_cast<s32>(VCall(ctx, o, 32, {o, 2}));
    auto settled = [&]() {
        const u32 an = load<u32>(o + 720);
        return an != 0 ? call(ctx, kTrackSet36, {an, 0, 0}) : call(ctx, kTrackSet164, {o, 0, 0});
    };
    auto cue = [&](u32 h) {
        const u32 e = load<u32>(o + 356);
        if (e != 0) call(ctx, kSetHash, {e, h});
    };
    if (s == 0) {
        cue(0x18FBE4D7u);
        VCall(ctx, o, 28, {o, 2, 1});
    } else if (s == 1) {
        if (static_cast<u32>(settled()) == 0) return;
        cue(0x7B320099u);
        call(ctx, 0x000B3A60u, {load<u32>(o + 480), load<u32>(q), load<u32>(q + 4)});
        VCall(ctx, o, 28, {o, 2, 2});
    } else if (s == 2) {
        if (static_cast<u32>(settled()) == 0) return;
        const u32 n = load<u32>(q + 8) - 1;
        store<u32>(q + 8, n);
        if (static_cast<s32>(n) > 0) {
            cue(0xCF934AECu);
            VCall(ctx, o, 28, {o, 2, 3});
        } else {
            call(ctx, kApplyTrackKey, {o});
        }
    } else if (s == 3) {
        const u32 t = load<u32>(o + 12);
        ctx.fpr[1] = Lf(q + 12);
        ctx.gpr[3] = t;
        if (Lf(t + 8) >= ctx.fpr[1]) VCall(ctx, o, 28, {o, 2, 0});
    }
}

namespace {
constexpr u32 kNotify26 = 0x000D94CCu;       // Obj_Notify26(o)
constexpr u32 kCmdPost = 0x0042C328u;        // 0x0042C328(w, x, a, 0 x5, stack 0, 0, 0, b)
constexpr u32 kSetSplitStyle = 0x002AB920u;  // Camera_SetSplitStyle(cam, i, style)
constexpr u32 kGetSplitA = 0x002AB9B0u;      // Camera_GetSplitA(cam, i)
constexpr u32 kIsSplit = 0x002A6044u;        // Camera_IsSplit(cam)
constexpr u32 kCoachSetMode = 0x002A8AE0u;   // Coach_SetMode2A8AE0(c, enter, sub, pause)
constexpr u32 kFlagSet8 = 0x0042CA60u;       // Flag_Set8_40000000(o)
}  // namespace

// Cmd_CameraSplit43807C(Cmd* c): the subject c->28 (re-resolved from id
// c->24 when set; else the world's slot object +4216) and its camera p =
// Obj_Notify26(subject). With both: 0x0042C328(c->4, p->8, 0.., b = 1); with
// flag 0x08 of c->12 the split styles c->16 / c->20 go to views 0 / 1 and each
// existing view (Camera_GetSplitA) is posted with its style. Unless flag 0x04
// with the split state already equal to flag 0x08 (Camera_IsSplit):
// Coach_SetMode(p, flag 0x08, c->12 >> 4, flag 0x01) and bit 0x1000 of p->244
// = flag 0x02. r3 = Flag_Set8_40000000(c).
DECOMP(0043807C, Cmd_CameraSplit43807C) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 id = load<u32>(c + 24);
    u32 o = load<u32>(c + 28);
    if (id != 0) {
        o = static_cast<u32>(call(ctx, kFindById, {id}));
        store<u32>(c + 28, o);
    }
    if (o == 0) {
        o = load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8);
        store<u32>(c + 28, o);
    }
    const u32 p = o != 0 ? static_cast<u32>(call(ctx, kNotify26, {o})) : 0;
    if (p != 0) {
        call(ctx, kCmdPost, {load<u32>(c + 4), load<u32>(p + 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 1});
        if ((load<u32>(c + 12) >> 3) & 1u) {
            call(ctx, kSetSplitStyle, {p, 0, load<u32>(c + 16)});
            call(ctx, kSetSplitStyle, {p, 1, load<u32>(c + 20)});
            for (u32 i = 0; i < 2; ++i) {
                if (call(ctx, kGetSplitA, {p, i}) == 0) continue;
                const u32 w = load<u32>(c + 4);
                const u32 a = static_cast<u32>(call(ctx, kGetSplitA, {p, i}));
                call(ctx, kCmdPost, {w, a, load<u32>(c + (i == 0 ? 16 : 20)), 0, 0, 0, 0, 0, 0, 0, 0, 0});
            }
        }
        bool apply = true;
        if (((load<u32>(c + 12) >> 2) & 1u) == 0) {
            const u32 want = (load<u32>(c + 12) >> 3) & 1u;
            if (want == static_cast<u32>(call(ctx, kIsSplit, {p}))) apply = false;
        }
        if (apply) {
            const u32 fl = load<u32>(c + 12);
            call(ctx, kCoachSetMode, {p, (fl >> 3) & 1u, fl >> 4, fl & 1u});
            const u32 f2 = load<u32>(c + 12);
            store<u32>(p + 244, (load<u32>(p + 244) & ~0x1000u) | (((f2 >> 1) & 1u) << 12));
        }
    }
    call(ctx, kFlagSet8, {c});
}

namespace {
constexpr u32 kRefPos = 0x0026A42Cu;         // Obj_RefPos26A42C(out[3], o)
constexpr u32 kDetach = 0x002D0BA0u;         // 0x002D0BA0(o, 0)
constexpr u32 kAddTimedCb = 0x0046B5F8u;     // World_AddTimedCallback(w, kind, opd, arg, c)
constexpr u32 kSceneEnd = 0x000DD02Cu;       // Obj_SceneEnd0DD02C(o, drop, keep)
constexpr u32 kSmallAlloc = 0x0055238Cu;     // SmallAlloc(n)
constexpr u32 kPtrConstruct = 0x005516E0u;   // PtrVector_Construct(v)

// o->vfn[304](o, b): this call site leaves the entry in r5 and the OPD in r6.
void Vfn304(PPUContext& ctx, u32 o, u32 b) {
    const u32 opd = load<u32>(load<u32>(o) + 304);
    call_ptr(ctx, opd, {o, b, load<u32>(opd), opd});
}
}  // namespace

// Track_Follow2D1F0C(Track* t): with bit 0x8000 of u16 t->28: the pose rows
// of t->32 (+240..) are copied to the frame (sp+128); the reference r =
// t->8->vfn[64]() at (r+480, r+488), moved by r+528 / r+536 times the
// reference height (Obj_RefPos26A42C(sp+112, r)->y) when r->32, closer than
// *0x0105B5E8 to the pose position (x, z) keeps the follower t->36 posed
// (vfn[84](rows)) and, unless the s16 t->30 is negative, re-queues it
// (World_AddTimedCallback(g_World, 0, 0x00F35980, t, t->8)); otherwise
// 0x002D0BA0(t, 0). r3 / f1 as the original leaves them.
DECOMP(002D1F0C, Track_Follow2D1F0C) {
    const u32 t = arg<u32>(ctx, 0);
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();
    ctx.gpr[3] = 0;
    if ((load<u16>(t + 28) & 0x8000u) == 0) return;
    const u32 m = load<u32>(t + 32) + 240;
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) Cw(sp + 128 + 16 * row + 4 * k, m + 16 * row + 4 * k);
    const u32 tgt = load<u32>(t + 8);
    const u32 r = static_cast<u32>(VCall(ctx, tgt, 64, {tgt}));
    f64 x = Lf(r + 480), z = Lf(r + 488);
    if (load<u32>(r + 32) != 0) {
        call(ctx, kRefPos, {sp + 112, r});
        const f64 h = Lf(sp + 116);
        x = Fmadds(Lf(r + 528), h, x);
        z = Fmadds(Lf(r + 536), h, z);
    }
    const f64 dz = S(z - Lf(sp + 184)), dx = S(x - Lf(sp + 176));
    const f64 d = Sqrts(Fmadds(dx, dx, S(dz * dz)));
    ctx.fpr[1] = d;
    if (d >= Lf(0x0105B5E8u)) {
        call(ctx, kDetach, {t, 0});
        return;
    }
    const u32 f = load<u32>(t + 36);
    if (f != 0) VCall(ctx, f, 84, {f, sp + 128});
    const s32 q = static_cast<s16>(load<u16>(t + 30));
    ctx.gpr[3] = static_cast<u32>(q);
    if (q < 0) return;
    call(ctx, kAddTimedCb, {load<u32>(0x00F65CA0u), 0, 0x00F35980u, t, load<u32>(t + 8)});
}

// Obj_EndScene74E70(Cmd* c, Effect* e): the target x is the component c->12
// of e (kind 5 with +28->276 == 0x02235C33: +28->280; kind 15: +20) or else
// Obj_FindById(c->16). x (vfn[304](c->20) first when bit 0x00400000 of +192)
// gets Obj_SceneEnd(x, c->20, c->21); with c->22 so does every child +192 of
// the 240-byte entries of x->340 (created empty with element size 240 when
// missing; quirk: children may be 0 and are still passed). r3 as the
// original leaves it.
DECOMP(00074E70, Obj_EndScene74E70) {
    const u32 c = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 x = 0;
    const u32 k = load<u32>(c + 12);
    if (k != 0) {
        const u32 p = static_cast<u32>(call(ctx, kFindComponent, {e, k}));
        if (p != 0) {
            const u32 q = static_cast<u32>(VCall(ctx, p, 32, {p})) == 5 ? load<u32>(p + 28) : 0;
            if (q != 0) {
                if (static_cast<s32>(load<u32>(q + 276)) == static_cast<s32>(0x02235C33u)) x = load<u32>(q + 280);
            } else if (static_cast<u32>(VCall(ctx, p, 32, {p})) == 15) {
                x = load<u32>(p + 20);
            }
        }
    }
    if (x == 0) {
        const u32 id = load<u32>(c + 16);
        ctx.gpr[3] = id;
        if (id != 0) x = static_cast<u32>(call(ctx, kFindById, {id}));
    }
    if (x == 0) return;
    if ((load<u32>(x + 192) & 0x00400000u) != 0) Vfn304(ctx, x, load<u8>(c + 20));
    call(ctx, kSceneEnd, {x, load<u8>(c + 20), load<u8>(c + 21)});
    const u32 keep = load<u8>(c + 22);
    ctx.gpr[3] = keep;
    if (keep == 0) return;
    u32 v = load<u32>(x + 340);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, kSmallAlloc, {20}));
        store<u32>(x + 340, v);
        if (v != 0) {
            call(ctx, kPtrConstruct, {v});
            store<u16>(v + 6, 240);
            v = load<u32>(x + 340);
        }
    }
    for (u32 j = 0, off = 0;; ++j, off += 240) {
        const u32 n = load<u16>(v + 8);  // quirk: v may be 0 after a failed allocation
        ctx.gpr[3] = n;
        if (!(j < n)) break;
        const u32 y = load<u32>(load<u32>(v) + off + 192);
        if (y != 0 && (load<u32>(y + 192) & 0x00400000u) != 0) Vfn304(ctx, y, load<u8>(c + 20));
        call(ctx, kSceneEnd, {y, load<u8>(c + 20), load<u8>(c + 21)});
    }
}
