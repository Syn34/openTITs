// Game code, part 32 (continued): 61-instruction functions of the
// small-function sweep; helpers in obj32.h. Each was checked against the
// original in the PPC simulator (memory writes, calls and their
// arguments, r3 and f1).
#include "obj32.h"

using namespace sly4;
using namespace sly4::obj32;

// ---------------------------------------------------------------- sweep: 61 instructions
// T* f(?, Xml* x): a 36-byte query node (vt 0x00B0FF68, the vector +12
// constructed with u16 +18 = 76 and PtrVector_Init(+12, 16, 0, 1), +32 =
// 0), the hash 0x8A93973F -> +32 and QueryDef_ReadChild(x, p + 12). r3 =
// p.
DECOMP(00084490, XmlQueryNode_New_00084490) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = XmlNodeAlloc32(ctx, 36, x);
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u32>(p, 0x00B0FF68);
    call(ctx, 0x005516E0, {p + 12});
    store<u16>(p + 18, 76);
    call(ctx, 0x00551598, {p + 12, 16, 0, 1});
    store<u32>(p + 32, 0);
    call(ctx, 0x004FA14C, {x, 0x8A93973Fu, p + 32, 0});
    call(ctx, 0x0049EF40, {x, p + 12});
    ctx.gpr[3] = p;
}

DECOMP(00084684, XmlH3_B0FF90_New_00084684) {
    XmlGen32(ctx, 24, 0x00B0FF90, {{12, 'w'}, {16, 'w'}, {0, 'v'}, {20, 'C', 0xFC81}},
             {{'H', 0x93116591, 12, false}, {'H', 0x757BAFD6, 16, true}, {'H', 2529, 20, true}});
}

DECOMP(0008A628, XmlHB2_B104B8_New_0008A628) {
    XmlGen32(ctx, 20, 0x00B104B8, {{12, 'w'}, {16, 'b'}, {0, 'v'}, {17, 'b'}},
             {{'H', 0x8A93973F, 12, false}, {'B', 0x7A281983, 16, true}, {'B', 0xEEA6E96F, 17, true}});
}

// void f(T* o, x): the target t (the kind-5 component of
// Effect_FindComponent(x, o->+12) whose +28 is tagged 0x02235C33 (+276;
// t = +280), else Obj_FindById(o->+12)) with flag 0x40000 (+172) and 2
// (+192) gets Obj_Set360_000E13D0(t, o->+16). r3 = t otherwise.
DECOMP(00092534, Target_Set360_00092534) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, load<u32>(o + 12)}));
    u32 t = 0;
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 c2 = load<u32>(c + 28);
        if (c2 != 0 && load<u32>(c2 + 276) == 0x02235C33u) t = load<u32>(c2 + 280);
    }
    if (t == 0) {
        t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(o + 12)}));
        if (t == 0) return;
    }
    ctx.gpr[3] = t;
    if ((load<u32>(t + 172) & 0x40000u) == 0 || (load<u32>(t + 192) & 2u) == 0) return;
    ctx.gpr[3] = call(ctx, 0x000E13D0, {t, load<u32>(o + 16)});
}

// T* f(?, id, x): the object for id: Effect_Bind4BB7E0(x, id, 0) tagged
// 0x02235C33 (+276; +280), else the kind-15 component 0x70D68D3A of x
// (Effect_FindComponent; its +20), else Obj_FindById(id). 0 for id 0.
DECOMP(0009A190, Target_Resolve_0009A190) {
    const u32 id = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    u32 t = 0;
    if (id != 0) {
        const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {x, id, 0}));
        if (e != 0 && load<u32>(e + 276) == 0x02235C33u) t = load<u32>(e + 280);
        if (t == 0) {
            const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, 0x70D68D3Au}));
            if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 15) t = load<u32>(c + 20);
        }
        if (t == 0) t = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
    }
    ctx.gpr[3] = t;
}

DECOMP(0009ACEC, XmlH2F_B10F60_New_0009ACEC) {
    XmlGen32(ctx, 24, 0x00B10F60, {{12, 'w'}, {16, 'w'}, {0, 'v'}, {20, 'w'}},
             {{'H', 0x91B5C2A0, 12, false}, {'H', 0x8D47DA8F, 16, true}, {'F', 0x000110AD, 20, true}});
}

DECOMP(000A5F58, XmlH2B_B11990_New_000A5F58) {
    XmlGen32(ctx, 24, 0x00B11990, {{12, 'w'}, {16, 'w'}, {20, 'B'}, {0, 'v'}},
             {{'H', 0x7A30D7E3, 12, false}, {'H', 0x04B8CC71, 16, true}, {'B', 0xF5897C08, 20, true}});
}

// void f(T* o, Obj* x): every occurrence of x in the list o->+248 (u16
// count +8) is removed by swapping in the last entry (the swapped-in entry
// is then skipped) and released (x->vt[4](x)) when non-null. r3 = the list.
DECOMP(000D91BC, List_RemoveAll_000D91BC) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(o + 248);
        const u32 n = l != 0 ? load<u16>(l + 8) : 0u;
        ctx.gpr[3] = l;
        if (i >= n) return;
        const u32 a = load<u32>(l);
        const u32 e = load<u32>(a + 4 * i);
        if (e != x) continue;
        const u32 m = load<u16>(l + 8) - 1u;
        store<u16>(l + 8, static_cast<u16>(m));
        if (static_cast<s32>(i) != static_cast<s32>(m & 0xFFFFu)) store<u32>(a + 4 * i, load<u32>(a + ((m << 2) & 0x3FFFCu)));
        if (e != 0) VCall(ctx, e, 4, {e});
    }
}

// void f(Node* n, ?, name, f32 v): recursively over the 240-byte children
// (n->+340, u16 count +8) flagged 0x80000000 (+176), then
// Obj_SetSign_003541E0(Anim_Find34CC34(n->+164, name), v) when both exist.
DECOMP(000DBD7C, Node_SetSignRec_000DBD7C) {
    const u32 n = arg<u32>(ctx, 0), name = arg<u32>(ctx, 2);
    const f64 v = farg(ctx, 0);
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(n + 340);
        const u32 cnt = l != 0 ? load<u16>(l + 8) : 0u;
        ctx.gpr[3] = l;
        if (i >= cnt) break;
        const u32 e = load<u32>(l) + 240 * i;
        if ((load<u32>(e + 176) >> 31) != 0) callf(ctx, 0x000DBD7C, {load<u32>(e + 192), 1, name}, {v});  // r4 = the tested bit
    }
    const u32 a = load<u32>(n + 164);
    if (a == 0) return;
    const u32 q = static_cast<u32>(call(ctx, 0x0034CC34, {a, name}));
    if (q == 0) return;
    callf(ctx, 0x003541E0, {q}, {v});
}

// void f(T* o, Desc* d, u8 k): for o flagged 0x80000000 (+344):
// o->vt[84](o, d), then d+64..+99 into o+356..+467 (f32s / u8s / a word,
// 4 bytes at +464) and 0x000FAE00(o, k). r3 = 0 otherwise.
DECOMP(000FB31C, Obj_ApplyDesc_000FB31C) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1), k = arg<u32>(ctx, 2) & 0xFFu;
    ctx.gpr[3] = load<u32>(o + 344) >> 31;
    if (ctx.gpr[3] == 0) return;
    VCall(ctx, o, 84, {o, d});
    CopyW32(o + 356, d + 64);
    CopyW32(o + 360, d + 68);
    store<u8>(o + 440, load<u8>(d + 72));
    CopyW32(o + 444, d + 76);
    CopyW32(o + 448, d + 80);
    store<u32>(o + 452, load<u32>(d + 84));
    store<u8>(o + 456, load<u8>(d + 88));
    CopyW32(o + 460, d + 92);
    for (u32 b = 0; b < 4; ++b) store<u8>(o + 464 + b, load<u8>(d + 96 + b));
    ctx.fpr[1] = Lfs32(d + 92);
    ctx.gpr[3] = call(ctx, 0x000FAE00, {o, k});
}

// The scaled hit reactions with the tuning +212..+220.
DECOMP(00168810, Obj_OnHitScaled_00168810) { HitScaled32(ctx, 0x0016880C, 212); }
DECOMP(00184D20, Obj_OnHitScaled_00184D20) { HitScaled32(ctx, 0x00184D1C, 212); }
DECOMP(00192610, Obj_OnHitScaled_00192610) { HitScaled32(ctx, 0x0019260C, 212); }

// void f(T* o, x): 0x000E5EF0(o, x); for the six slots k whose u8 o->+216->+481
// + 96k is set and whose object o->+560 + 16k exists: 0x001C8A74(&tmp, o,
// k), then obj->vt[84](obj, &tmp).
DECOMP(001C9C18, Obj_UpdateSlots_001C9C18) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 st = load<u32>(o + 216);
    call(ctx, 0x000E5EF0, {o, ctx.gpr[4]});  // r4 passed through untouched (the callee reads it)
    Stack frame(ctx, 64);
    const u32 tmp = frame.addr();  // sp+112
    for (u32 k = 0; k < 6; ++k) {
        ctx.gpr[3] = load<u8>(st + 481 + 96 * k);
        if (ctx.gpr[3] == 0) continue;
        ctx.gpr[3] = o + 560 + 16 * k;
        const u32 e = load<u32>(o + 560 + 16 * k);
        if (e == 0) continue;
        call(ctx, 0x001C8A74, {tmp, o, k});
        VCall(ctx, e, 84, {e, tmp});
    }
}

// void f(T* o, Hit* h): with the owner a = o->+416 and h's u8 +28 set: the
// push vector h->+20 + 16 (negated when h->+8 is a) is added to o+1232
// when its |y| < sinf(K) (K at 0x0024DDC8).
DECOMP(0024DDD0, Obj_AddPush_0024DDD0) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 a = load<u32>(o + 416);
    ctx.gpr[3] = a;
    if (a == 0 || load<u8>(h + 28) == 0) return;
    const u32 v = load<u32>(h + 20) + 16;
    f64 x = Lfs32(v), y = Lfs32(v + 4), z = Lfs32(v + 8);
    if (load<u32>(h + 8) == a) {
        x = -x;
        y = -y;
        z = -z;
    }
    const f64 ay = std::fabs(y);
    const f64 s = callf(ctx, 0x007345F0, {}, {Lfs32(0x0024DDC8)});
    if (!(ay < s)) return;
    const f64 nx = static_cast<f32>(Lfs32(o + 1232) + x), ny = static_cast<f32>(Lfs32(o + 1236) + y), nz = static_cast<f32>(Lfs32(o + 1240) + z);
    Stfs32(o + 1232, nx);
    Stfs32(o + 1236, ny);
    Stfs32(o + 1240, nz);
    ctx.fpr[1] = nx;
    ctx.fpr[2] = ny;
    ctx.fpr[3] = nz;
}

// void f(?, key, PtrVector* out): pushes every entry of the global list
// 0x010681FC (u16 count +8) whose +104 is key onto out (VecPush28). r3 =
// the list count.
DECOMP(002E6FC0, List_CollectByKey_002E6FC0) {
    const u32 key = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    constexpr u32 L = 0x010681FC;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(L + 8);
        ctx.gpr[3] = n;
        if (i >= n) return;
        const u32 slot = load<u32>(L) + 4 * i;
        if (load<u32>(load<u32>(slot) + 104) != key) continue;
        VecPush28(ctx, out, 2, [&](u32 at) { store<u32>(at, load<u32>(slot)); });
    }
}

// void f(T* o, u8 on): OnEach32 with the count o->+8->+152 over o's own
// 16-byte entries (+236, object +4).
DECOMP(00384808, Obj_SetOnAll16b_00384808) { OnEach32(ctx, 152, 236, 16, 4, true); }

// bool f(Ref* a, T* b): b->+12 = q set: Emitter_Stop(slot, 1) with slot =
// Obj_FindSlotByHandle_000DBC28(&slot, q, the table word for a
// (RefEntry32)). r3 = 0.
DECOMP(0039474C, Ref_StopEmitter_0039474C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 q = load<u32>(b + 12);
    const u32 v = load<u32>(RefEntry32(a));
    if (q != 0) {
        Stack frame(ctx, 16);
        const u32 s = frame.addr();  // sp+112
        call(ctx, 0x000DBC28, {s, q, v});
        call(ctx, 0x00443424, {load<u32>(s), 1});
    }
    ctx.gpr[3] = 0;
}

// void f(Msg* m): with the effect definition EffectDef_Find(m->+36, +40, 1):
// m->+44 = a new 140-byte effect (Effect_Construct; 0 on failure, then the
// following writes go to address 136), Effect_AddArgs49FCC0(it, m + 12,
// m->+32, 1, u8 m->+48), flag 0x200 of its +136 = !(m->+8 & 0x80000000),
// Effect_Start(it). Then Flag_Set8(m).
DECOMP(004332B0, Msg_SpawnEffect_004332B0) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 d = static_cast<u32>(call(ctx, 0x004BDEDC, {load<u32>(m + 36), load<u32>(m + 40), 1}));
    if (d != 0) {
        u32 e = static_cast<u32>(call(ctx, 0x0055238C, {140}));
        if (e != 0) call(ctx, 0x004C7A68, {e, d});
        store<u32>(m + 44, e);
        call(ctx, 0x0049FCC0, {e, m + 12, load<u32>(m + 32), 1, load<u8>(m + 48)});
        const u32 off = (load<u32>(m + 8) >> 31) == 0 ? 0x200u : 0u;
        const u32 e2 = load<u32>(m + 44);
        store<u32>(e2 + 136, (load<u32>(e2 + 136) & ~0x200u) | off);
        call(ctx, 0x004C753C, {load<u32>(m + 44)});
    }
    ctx.gpr[3] = call(ctx, 0x0042CA60, {m});
}

// void f(Msg* m): the object Obj_FindByHandle(m->+12, 0) (a String_Format(
// 0x00AE2738, Hash_ToString(id)) message when missing) gets, with b = the
// bit 0x80000000 of m->+16: vt[304](it, b) when flagged 0x00400000
// (+192), then Obj_SceneEnd0DD02C(it, b, 0). Then Flag_Set8(m).
DECOMP(00438FB4, Msg_SceneEnd_00438FB4) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 t = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(m + 12), 0}));
    if (t == 0) {
        const u32 s = static_cast<u32>(call(ctx, 0x0056A6E0, {load<u32>(m + 12)}));
        call(ctx, 0x005534D0, {0x00AE2738u, s});
    } else {
        if ((load<u32>(t + 192) & 0x00400000u) != 0) {
            const u32 opd = load<u32>(load<u32>(t) + 304);
            call_ptr(ctx, opd, {t, (load<u32>(m + 16) >> 31) != 0 ? 1u : 0u, load<u32>(opd), opd});  // entry in r5, OPD in r6
        }
        call(ctx, 0x000DD02C, {t, (load<u32>(m + 16) >> 31) != 0 ? 1u : 0u, 0});
    }
    ctx.gpr[3] = call(ctx, 0x0042CA60, {m});
}

// bool Voice_IsLive(u32 key): the voice handle mapped to key in 0x010AFEF8
// (KeyMap_Find; 12-byte entries at +8, handle +4; the default *0x010AFF08
// when absent) is live (VoiceLive32).
DECOMP(0044D4C8, Voice_IsLiveByKey_0044D4C8) {
    const u32 key = arg<u32>(ctx, 0);
    constexpr u32 M = 0x010AFEF8;
    Stack frame(ctx, 16);
    const u32 k = frame.addr();  // sp+112
    store<u32>(k, key);
    const s32 idx = static_cast<s32>(call(ctx, 0x00784008, {M, k}));
    const u32 at = idx < 0 ? M + 16 : load<u32>(M + 8) + 12 * static_cast<u32>(idx) + 4;
    ctx.gpr[3] = VoiceLive32(load<u32>(at));
}
