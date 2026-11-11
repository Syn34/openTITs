// Game code, part 39 (continued): the 82 instruction functions of the
// small-function sweep. Each was checked against the original in the PPC
// simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj39.h"

using namespace sly4;
using namespace sly4::obj39;

// ---------------------------------------------------------------- sweep: 82 instructions
// u32 f(Obj* o; f32 s) (from f_00036DF0): the 4x3 matrix o + 128 is copied
// to the stack, the track a = o->+48 is sampled at ((1 - phase) * length
// * s) (Anim_TrackLength164_000DC418(a, 0), Obj_AnimPhase_000DC6A0(a, 0);
// a->vt[52](a, &m, -, 0; f1) with the OPD in r5 and the entry in r7), the
// copy is multiplied by it (Matrix_Multiply(&copy, &m)) and
// 0x00015C98(o, &copy row 3) gets its translation row (r3 = its).
DECOMP(00015E40, Obj_ApplyTrackPose_00015E40) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 s = farg(ctx, 0);
    Stack fr(ctx, 128);  // m (sp+112), the copy (sp+176)
    const u32 m = fr.addr(), cp = fr.addr() + 64;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(cp + 16 * r + 4 * k, load<u32>(o + 128 + 16 * r + 4 * k));
    const u32 a = load<u32>(o + 48);
    const f64 len = callf(ctx, 0x000DC418u, {a, 0}, {});
    const f64 ph = callf(ctx, 0x000DC6A0u, {load<u32>(o + 48), 0}, {});
    const f64 t = F39(F39(F39(Lfs32(0x00015E38u) - ph) * len) * s);
    const u32 opd = load<u32>(load<u32>(a) + 208);
    ctx.fpr[1] = t;
    call_ptr(ctx, opd, {a, m, opd, 0, load<u32>(opd)});
    call(ctx, 0x005541F0u, {cp, m});
    call(ctx, 0x00015C98u, {o, cp + 48});
}

// T* f(?, XmlCursor* x) (no direct caller): a 24-byte node (Mem_Alloc(24,
// 0, 16); 0 on failure) set up by Obj_InitVt_B21C98(p, x), +12 / +16 = 0,
// u8 +20 = 1, vtable 0x00B0F568, u8 +21 / +22 / +23 = 0; hashes
// 0x8A93973F -> +12 (default 0) and 0x7A30D7E3 -> +16, bools 0x7A281983
// -> +20 and 0x8629270A -> +22, each setting u8 +21 / +23 = 1 when the
// read returns 0. r3 = p.
DECOMP(00073600, XmlNew_Node24c_00073600) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 1);
    store<u32>(p, 0x00B0F568u);
    store<u8>(p + 21, 0);
    store<u8>(p + 22, 0);
    store<u8>(p + 23, 0);
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    const u32 r = static_cast<u32>(call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 20, load<u8>(p + 20)}));
    const u8 b = load<u8>(p + 22);
    if (r == 0) store<u8>(p + 21, 1);
    if (static_cast<u32>(call(ctx, 0x004F986Cu, {x, 0x8629270Au, p + 22, b})) == 0) store<u8>(p + 23, 1);
    ret(ctx, p);
}

// u64 f(T* o, Effect* x) (no direct caller): the target y = the owner of
// the component c = Effect_FindComponent(x, o->+12) when c->vt[8](c) == 5
// (OPD / entry after the argument) and c->+28 has type 0x02235C33 at +276
// (its +280), else Target_Resolve_0009A190(o, o->+16, x) (when o->+16 is
// set); y's u64 +184 gets bit 37 (0x0000002000000000, from 0x0008B068)
// set when u8 o->+20 is 0, else cleared (r3 = the new value). Without y,
// the names of o->+16 and o->+12 are looked up (Hash_ToString; r3 =
// its).
DECOMP(0008B070, EffectCmd_SetTargetBit_0008B070) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 y = 0;
    const u32 h = load<u32>(o + 12);
    if (h != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, h}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 d = load<u32>(c + 28);
            if (d != 0 && load<u32>(d + 276) == 0x02235C33u) y = load<u32>(d + 280);
        }
    }
    if (y == 0) {
        const u32 id = load<u32>(o + 16);
        if (id != 0) y = static_cast<u32>(call(ctx, 0x0009A190u, {o, id, x}));
    }
    if (y == 0) {
        call(ctx, 0x0056A6E0u, {load<u32>(o + 16)});
        call(ctx, 0x0056A6E0u, {load<u32>(o + 12)});
        return;
    }
    u64 v = load<u64>(y + 184);
    if (load<u8>(o + 20) == 0) v |= load<u64>(0x0008B068u);
    else v &= ~(1ull << 37);
    store<u64>(y + 184, v);
    ctx.gpr[3] = v;
}

// T* f(?, XmlCursor* x) (no direct caller): a 28-byte node (Mem_Alloc(28,
// 0, 16); 0 on failure) set up by Obj_InitVt_B21C98(p, x), +20 = 0, u8 +25
// = 0, u8 +24 = 1, vtable 0x00B10698 (+12 / +16 left as read); hashes
// 0x0464F605 -> +12 and 0x001EFE3C -> +16 (defaults 0), bools 0x83F172B0
// -> +24 and 0x0138B87F -> +25; when Ptr_DerefNonZero(p->+20) is 0,
// RefNode_Release(p->+20) and p->+20 = Node_PushF65B7C(p->+12, p->+16).
// r3 = p.
DECOMP(0008C4E4, XmlNew_RefNode_0008C4E4) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 20, 0);
    store<u8>(p + 25, 0);
    store<u8>(p + 24, 1);
    store<u32>(p, 0x00B10698u);
    call(ctx, 0x004FA14Cu, {x, 0x0464F605u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x001EFE3Cu, p + 16, 0});
    call(ctx, 0x004F986Cu, {x, 0x83F172B0u, p + 24, load<u8>(p + 24)});
    call(ctx, 0x004F986Cu, {x, 0x0138B87Fu, p + 25, load<u8>(p + 25)});
    if (static_cast<u32>(call(ctx, 0x00446960u, {load<u32>(p + 20)})) == 0) {
        call(ctx, 0x004432DCu, {load<u32>(p + 20)});
        store<u32>(p + 20, static_cast<u32>(call(ctx, 0x00446984u, {load<u32>(p + 12), load<u32>(p + 16)})));
    }
    ret(ctx, p);
}

// u32 f(T* o, Effect* x) (no direct caller): the target y = the owner of
// the component c = Effect_FindComponent(x, o->+12) when c->vt[8](c) == 5
// and c->+28 (read unchecked) has type 0x02235C33 at +276 (its +280), else
// Obj_FindById(o->+16) (when set); narrowed by Obj_FindByHash_000DB290(y,
// o->+20) when o->+20 is set; then with t = y->vt[15](y) (OPD / entry
// after the argument): Anim_Stop254814(t, o->+24), or without t
// 0x000DC314(y, o->+24, 1) (r3 = its). r3 = 0 when no target.
DECOMP(0009658C, EffectCmd_StopAnim_0009658C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 y = 0;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 12)}));
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 d = load<u32>(c + 28);
        if (load<u32>(d + 276) == 0x02235C33u) y = load<u32>(d + 280);
    }
    if (y == 0) {
        const u32 id = load<u32>(o + 16);
        ctx.gpr[3] = id;
        if (id != 0) y = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
        if (y == 0) return;
    }
    const u32 h = load<u32>(o + 20);
    if (h != 0) {
        y = static_cast<u32>(call(ctx, 0x000DB290u, {y, h}));
        if (y == 0) return;
    }
    const u32 t = static_cast<u32>(VCall(ctx, y, 60, {y}));
    if (t != 0) call(ctx, 0x00254814u, {t, load<u32>(o + 24)});
    else call(ctx, 0x000DC314u, {y, load<u32>(o + 24), 1});
}

// u32 f(Obj* o, a, b) (from Effect_Forward_00084584): the effect e =
// Obj_CreateEffectDCA20(o, o->+360, 0, 1) (r3 = o without o->+360, 0 when
// not created) gets Effect_AddArgs49FCC0(e, q + 200, b, 0, 0) first when
// o->+376 has bit 0x00040000 and the record q = *SortedMap_Find732F38(
// 0x01013B1C, &(o->+160)->+204) has bit 0x2 of +4, then
// Effect_AddArgs49FCC0(e, a, b, 0, 0), bit 0x200 of e->+136 = bit
// 0x80000000 of o->+376, and Effect_SetHash4CF988(e, 0x04B8CC42) (r3 =
// its).
DECOMP(000E1288, Obj_SpawnEffect_000E1288) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 e0 = load<u32>(o + 360);
    if (e0 == 0) return;
    const u32 e = static_cast<u32>(call(ctx, 0x000DCA20u, {o, e0, 0, 1}));
    if (e == 0) return;
    if ((load<u32>(o + 376) & 0x00040000u) != 0) {
        const u32 w = load<u32>(o + 160);
        if (w != 0) {
            Stack fr(ctx, 16);  // the key (sp+112)
            store<u32>(fr.addr(), load<u32>(w + 204));
            const u32 q = load<u32>(static_cast<u32>(call(ctx, 0x00732F38u, {0x01013B1Cu, fr.addr()})));
            if (q != 0 && (load<u32>(q + 4) & 0x2u) != 0) call(ctx, 0x0049FCC0u, {e, q + 200, b, 0, 0});
        }
    }
    call(ctx, 0x0049FCC0u, {e, a, b, 0, 0});
    const u32 bit = (load<u32>(o + 376) & 0x80000000u) != 0 ? 1u : 0u;
    store<u32>(e + 136, (load<u32>(e + 136) & ~0x200u) | (bit << 9));
    call(ctx, 0x004CF988u, {e, 0x04B8CC42u});
}

// T* f(T* t, const Def* d) (the DamageColDef_NewD3E58 constructor body): as
// 0x001CFE00 (base Thunk_000E27FC(t, d), vtable 0x00B15970, f32 +288..
// +296 -> t + 368.., +304 / +308 -> +384 / +388, f32 +312 -> +392, bit
// 0x80 of u8 +316 into u8 t->+396, the seven pairs d + 224.. -> t + 304..),
// then +320 -> +400, vtable 0x00B176E0, f32 +324 -> +404 (f1) and +328 ->
// +408. r3 = d->+328.
DECOMP(001D3A24, DamageColDef_Ctor_001D3A24) {
    const u32 t = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E27FCu, {t, d});
    store<u32>(t, 0x00B15970u);
    store<u32>(t + 368, load<u32>(d + 288));
    store<u32>(t + 372, load<u32>(d + 292));
    store<u32>(t + 376, load<u32>(d + 296));
    store<u32>(t + 384, load<u32>(d + 304));
    const u8 b = load<u8>(t + 396);
    store<u32>(t + 388, load<u32>(d + 308));
    store<u32>(t + 392, load<u32>(d + 312));
    store<u8>(t + 396, static_cast<u8>((b & 0x7Fu) | (load<u8>(d + 316) & 0x80u)));
    const u32 w320 = load<u32>(d + 320), w324 = load<u32>(d + 324), w328 = load<u32>(d + 328);
    for (u32 k = 0; k < 7; ++k) {
        store<u32>(t + 304 + 8 * k, load<u32>(d + 224 + 8 * k));
        store<u32>(t + 308 + 8 * k, load<u32>(d + 228 + 8 * k));
    }
    store<u32>(t + 400, w320);
    store<u32>(t + 0, 0x00B176E0u);
    store<u32>(t + 404, w324);
    store<u32>(t + 408, w328);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w324));  // the f32 +324 went through f1
    ret(ctx, w328);
}

// u32 f(Obj* o, bool on) (no direct caller): with on set:
// Obj_Reset100_0045C81C(world slot (s8) o->+172 + 4208), o->vt[3](o) (OPD /
// entry after the argument), the skeleton o->+1344 (when set; its body
// +8 removed first with Body_Remove931F00(+8, s)) released by
// AnimSkel_Release7D4EA8 and cleared, o->+396 = 0, and the effects
// o->+1364, o->+1360, o->+1352 (each when set) get bit 0x200 of +136 and
// are cleared (o->+1368 / +1372 too with +1364). Always ends with
// Obj_SetRagdollBase(o, on) (r3 = its).
DECOMP(002482D4, Obj_SetRagdoll_002482D4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        const s32 s = static_cast<s8>(load<u8>(o + 172));
        call(ctx, 0x0045C81Cu, {load<u32>(0x00F65CA0u) + static_cast<u32>(s) * 0x00020C80u + 4208});
        VCall(ctx, o, 12, {o});
        const u32 k = load<u32>(o + 1344);
        if (k != 0) {
            const u32 bd = load<u32>(k + 8);
            if (bd != 0) call(ctx, 0x00931F00u, {bd, k});
            call(ctx, 0x007D4EA8u, {load<u32>(o + 1344)});
            store<u32>(o + 1344, 0);
        }
        const u32 x = load<u32>(o + 1364);
        store<u32>(o + 396, 0);
        if (x != 0) {
            store<u32>(x + 136, load<u32>(x + 136) | 0x200u);
            store<u32>(o + 1364, 0);
            store<u32>(o + 1368, 0);
            store<u32>(o + 1372, 0);
        }
        const u32 y = load<u32>(o + 1360);
        if (y != 0) {
            store<u32>(y + 136, load<u32>(y + 136) | 0x200u);
            store<u32>(o + 1360, 0);
        }
        const u32 z = load<u32>(o + 1352);
        if (z != 0) {
            store<u32>(z + 136, load<u32>(z + 136) | 0x200u);
            store<u32>(o + 1352, 0);
        }
    }
    call(ctx, 0x000E02F8u, {o, on});
}

// void f(T* o) (no direct caller): DeletePair112_39 with vtables
// 0x00B1C258 / 0x00B1C2A8.
DECOMP(0027CF60, Obj_Delete112_0027CF60) { DeletePair112_39(ctx, 0x00B1C258u, 0x00B1C2A8u); }

// void f(T* o) (no direct caller): DeletePair112_39 with vtables
// 0x00B1C328 / 0x00B1C378.
DECOMP(0027EA94, Obj_Delete112b_0027EA94) { DeletePair112_39(ctx, 0x00B1C328u, 0x00B1C378u); }

// void f(T* o) (no direct caller): DeletePair112_39 with vtables
// 0x00B1C3F8 / 0x00B1C444.
DECOMP(00280244, Obj_Delete112c_00280244) { DeletePair112_39(ctx, 0x00B1C3F8u, 0x00B1C444u); }

// u32 f(Obj* o): for o != 0, bit 22 of the u64 o->+184 is cleared; the
// entry of the pointer vector 0x0106C280 (data +0, u16 count +8) whose
// element has +168 == o is appended to the vector 0x0106C294 (u16
// capacity +4, count +8, peak +10; PtrVector_Reserve when full) and
// removed (the last element moved into its place). r3 = the first
// vector's data when found, else o.
DECOMP(00369A80, Obj_Retire_00369A80) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u64>(o + 184, load<u64>(o + 184) & ~(1ull << 22));
    constexpr u32 L = 0x0106C280u, V = 0x0106C294u;
    const u32 n = load<u16>(L + 8);
    const u32 d = load<u32>(L);
    u32 i = 0;
    for (;; ++i) {
        if (!(i < n)) return;
        if (load<u32>(load<u32>(d + 4 * i) + 168) == o) break;
    }
    u32 m = load<u16>(V + 8);
    const u32 cap = load<u16>(V + 4);
    if (m == cap) {
        call(ctx, 0x00551718u, {V, (cap << 1) & 0x1FFFEu});
        m = load<u16>(V + 8);
    }
    const u32 e = load<u32>(d + 4 * i);
    const u32 vd = load<u32>(V);
    const u32 m2 = load<u16>(V + 8) + 1u;
    const u32 peak = load<u16>(V + 10);
    store<u32>(vd + ((m << 2) & 0x3FFFCu), e);
    store<u16>(V + 8, static_cast<u16>(m2));
    const u32 c = load<u16>(L + 8);
    const u32 ld = load<u32>(L);
    store<u16>(V + 10, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(m2 & 0xFFFFu) ? peak : m2));
    const u32 last = c - 1;
    store<u16>(L + 8, static_cast<u16>(last));
    ret(ctx, ld);
    if (i == (last & 0xFFFFu)) return;
    store<u32>(ld + 4 * i, load<u32>(ld + ((last << 2) & 0x3FFFCu)));
}

// u32 f(T* o) (from ThunkArg1NZ_003746D8): with the view v =
// (*0x00F65CA0)->+0x12816C, the vec4 0x01164BC0 is projected into o + 224,
// o + 256 and o + 240 (View_ProjectPoint(v, &copy, dst), three stack
// copies), the copies scaled by the display size (Display_GetSize, on the
// stack; unused afterwards), then 0x003742E0(o, u16 o->+358) when it
// differs from u16 o->+356, and 0x003742E0(o, u16 o->+356) (r3 = its).
DECOMP(00374590, View_ProjectMarkers_00374590) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(load<u32>(0x00F65CA0u) + 0x0012816Cu);
    Stack fr(ctx, 64);  // three vec4 copies (sp+112, +128, +144), the size (sp+160)
    const u32 s = fr.addr();
    call(ctx, 0x004D4104u, {s + 48});
    for (u32 c = 0; c < 3; ++c)
        for (u32 k = 0; k < 4; ++k) store<u32>(s + 16 * c + 4 * k, load<u32>(0x01164BC0u + 4 * k));
    call(ctx, 0x00522750u, {v, s, o + 224});
    call(ctx, 0x00522750u, {v, s + 16, o + 256});
    call(ctx, 0x00522750u, {v, s + 32, o + 240});
    const f64 w = Lfs32(s + 48), h = Lfs32(s + 52);
    const f64 a = F39(Lfs32(s + 4) * h), b = F39(Lfs32(s + 16) * w), c = F39(Lfs32(s + 20) * h), d = F39(Lfs32(s + 32) * w);
    Stfs32(s + 4, a);
    Stfs32(s + 16, b);
    Stfs32(s + 20, c);
    Stfs32(s + 32, d);
    ctx.fpr[1] = a;
    const u32 n = load<u16>(o + 356), m = load<u16>(o + 358);
    if (n != m) call(ctx, 0x003742E0u, {o, m});
    call(ctx, 0x003742E0u, {o, load<u16>(o + 356)});
}

// u32 f(Cmd* c) (no direct caller): unless c->+28 is already set (r3 =
// it), c->+28 = List_FindByKey_004464F0(0x010ACC40, c->+12); when not
// found or u8 c->+24 is set, a 68-byte spawn record on the stack (two
// copies of the vec3 0x01164B40, f32 1.0, zeros, +48 = c->+16, +52 =
// c->+20) gives c->+28 = Call_446198_00446348(0x010ACC40, c->+12, &rec,
// c->+16, c->+20) and is destroyed (0x00440AC4(&rec, 0), 0x00440A40(&rec,
// 0); r3 = its). Otherwise r3 = c->+12.
DECOMP(0042EF6C, SeqCmd_Spawn_0042EF6C) {
    const u32 c = arg<u32>(ctx, 0);
    ctx.gpr[3] = load<u32>(c + 28);
    if (ctx.gpr[3] != 0) return;
    constexpr u32 L = 0x010ACC40u;
    const u32 r = static_cast<u32>(call(ctx, 0x004464F0u, {L, load<u32>(c + 12)}));
    store<u32>(c + 28, r);
    const u32 c16 = load<u32>(c + 16), c20 = load<u32>(c + 20), c12 = load<u32>(c + 12);
    if (r != 0 && load<u8>(c + 24) == 0) {
        ret(ctx, c12);
        return;
    }
    Stack fr(ctx, 112);  // the record at sp+112..179 (frame of 224 bytes)
    const u32 rec = fr.addr(), sp = fr.addr() - 112;
    for (u32 k = 0; k < 3; ++k) {
        store<u32>(rec + 4 * k, load<u32>(0x01164B40u + 4 * k));
        store<u32>(rec + 16 + 4 * k, load<u32>(0x01164B40u + 4 * k));
    }
    store<u32>(rec + 32, 0x3F800000u);
    store<u32>(rec + 36, 0);
    store<u32>(rec + 48, 0);
    store<u32>(rec + 40, 0);
    store<u8>(rec + 44, 0);
    store<u32>(rec + 56, 0);
    store<u32>(rec + 60, 0);
    store<u8>(rec + 45, 0);
    store<u32>(rec + 64, 0);
    store<u8>(rec + 46, 0);
    store<u8>(rec + 47, 0);
    store<u32>(rec + 48, c16);
    store<u32>(rec + 52, c20);
    ctx.fpr[1] = Lfs32(0x01164B40u);  // the vec3 copies go through f1..f3
    // r8 / r9 / r10 as the record stores leave them (sp+2, sp+1, 0); the callee reads r3..r5
    store<u32>(c + 28, static_cast<u32>(call(ctx, 0x00446348u, {L, c12, rec, c16, c20, sp + 2, sp + 1, 0})));
    call(ctx, 0x00440AC4u, {rec, 0});
    call(ctx, 0x00440A40u, {rec, 0});
}

// u32 f(Obj* src, u32 key, u32 a, u32 b; f32 r) (Obj_PlayByLevel_0001B4A0
// and others): for each node n of the list *(*SortedMap_Find786C0C(
// 0x010BB738, &key)) (next +4) with u8 +8 set whose object o = n->+0 lies
// within r of src (positions +144, squared distance by fmuls / fmadds), a
// 20-byte record {src, o, a, b, u8 1} is appended to 0x010BA138 (count
// 0x010BA134, at most 128). r3 = 0; f1 = src's z (or r when the key is
// unknown).
DECOMP(0048F90C, Obj_CollectNear_0048F90C) {
    const u32 src = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const f64 r = farg(ctx, 0);
    Stack fr(ctx, 16);  // the key (sp+112)
    store<u32>(fr.addr(), key);
    const u32 q = load<u32>(static_cast<u32>(call(ctx, 0x00786C0Cu, {0x010BB738u, fr.addr()})));
    ctx.gpr[3] = q;
    if (q == 0) return;
    const u32 pos = src + 144;
    u32 n = load<u32>(q);
    const f64 r2 = F39(r * r);
    const f64 x = Lfs32(pos), y = Lfs32(pos + 4), z = Lfs32(pos + 8);
    ctx.fpr[1] = z;
    ctx.gpr[3] = n;
    if (n == 0) return;
    constexpr u32 C = 0x010BA134u, A = 0x010BA138u;
    u32 cnt = load<u32>(C);
    do {
        if (load<u8>(n + 8) != 0) {
            const u32 o = load<u32>(n);
            const u32 p = o + 144;
            const f64 dy = F39(Lfs32(p + 4) - y), dx = F39(Lfs32(p) - x), dz = F39(Lfs32(p + 8) - z);
            f64 d = F39(dy * dy);
            d = F39(std::fma(dx, dx, d));
            d = F39(std::fma(dz, dz, d));
            if (d < r2 && o != 0 && static_cast<s32>(cnt) < 128) {
                const u32 e = A + 20 * cnt;
                store<u32>(e, src);
                ++cnt;
                store<u32>(C, cnt);
                store<u32>(e + 4, o);
                store<u32>(e + 8, a);
                store<u32>(e + 12, b);
                store<u8>(e + 16, 1);
            }
        }
        n = load<u32>(n + 4);
    } while (n != 0);
    ctx.gpr[3] = 0;
}

// u32 f(T* o, Effect* x) (no direct caller): for the component c =
// Effect_FindComponent(x, o->+12) in state 10 (c->vt[8](c); OPD / entry
// after the argument), the node t = c->+40 (when 0, looked up through the
// u16 c->+28 keys of c->+20: t = Node_FindByKey_004D54FC(key, t), stored
// each step) gets t->vt[10](t, o + 16); otherwise the bound object b =
// Effect_Bind4BB7E0(x, o->+12, 0) gets WorldObj_SetScale(b, o + 16, u8
// o->+32). r3 = 0 or the last call's.
DECOMP(004CB4A0, EffectCmd_SetScale_004CB4A0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 12)}));
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 10) {
        u32 t = load<u32>(c + 40);
        ctx.gpr[3] = t;
        if (t == 0) {
            for (u32 i = 0; i < load<u16>(c + 28); ++i) {
                t = static_cast<u32>(call(ctx, 0x004D54FCu, {load<u32>(load<u32>(c + 20) + 4 * i), t}));
                store<u32>(c + 40, t);
            }
        }
        if (t != 0) VCall(ctx, t, 40, {t, o + 16});
        return;
    }
    const u32 b = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, load<u32>(o + 12), 0}));
    if (b != 0) call(ctx, 0x0057D100u, {b, o + 16, load<u8>(o + 32)});
}

// void f() (from Game_Shutdown): clears 0x00F66884; the head objects of
// the lists 0x00F668A0 and then 0x00F6689C get p->vt[1](p) until each list
// is empty (OPD / entry after the argument); 0x004D3798(0); then the
// object 0x00F6684C (when set) gets vt[2](p) and is cleared (r3 =
// 0x00F60000, else 0).
DECOMP(004D39B0, Game_ReleaseLists_004D39B0) {
    const u32 p0 = load<u32>(0x00F668A0u);
    store<u32>(0x00F66884u, 0);
    for (u32 p = p0; p != 0; p = load<u32>(0x00F668A0u)) VCall(ctx, p, 4, {p});
    for (u32 p; (p = load<u32>(0x00F6689Cu)) != 0;) VCall(ctx, p, 4, {p});
    call(ctx, 0x004D3798u, {0});
    const u32 q = load<u32>(0x00F6684Cu);
    ctx.gpr[3] = q;
    if (q == 0) return;
    VCall(ctx, q, 8, {q});
    store<u32>(0x00F6684Cu, 0);
    ctx.gpr[3] = 0x00F60000u;
}

// T* f(T* o, Obj* a, const Mat43* m, u32 b) (from f_001415F0): o->+4 = a,
// u8 +8 = 0, m's four rows -> o + 16.. (vtable 0x00B13D48 stored among
// them), +80 / +88 = f32 0x01164AFC, +84 / +92 = f32 0x01164B00, +96 = b,
// +100 = f32 a->+356, +104 / +112 = 0, bits 0xE0000000 of +116 cleared,
// +108 = lo + (hi - lo) * rand (0x01034490 +72 / +76; the old random
// value of RandStep28 * 2^-16). r3 = o, f1 = +108.
DECOMP(0074C998, Obj_InitTimer_0074C998) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    store<u32>(o + 4, a);
    store<u8>(o + 8, 0);
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 16 + 16 * r + 4 * k, load<u32>(m + 16 * r + 4 * k));
    store<u32>(o, 0x00B13D48u);
    store<u32>(o + 64, load<u32>(m + 48));
    store<u32>(o + 68, load<u32>(m + 52));
    store<u32>(o + 72, load<u32>(m + 56));
    constexpr u32 K2 = 0x01164AFCu;
    store<u32>(o + 80, load<u32>(K2));
    store<u32>(o + 84, load<u32>(K2 + 4));
    store<u32>(o + 88, load<u32>(K2));
    const u32 w116 = load<u32>(o + 116);
    store<u32>(o + 96, b);
    store<u32>(o + 92, load<u32>(K2 + 4));
    store<u32>(o + 116, (w116 & 0x7FFFFFFFu) & ~0x60000000u);
    store<u32>(o + 100, load<u32>(a + 356));
    store<u32>(o + 104, 0);
    store<u32>(o + 112, 0);
    constexpr u32 T = 0x01034490u;
    const f64 lo = Lfs32(T + 72);
    const f64 span = F39(Lfs32(T + 76) - lo);
    const f64 r = F39(static_cast<f64>(static_cast<s64>(RandStep28())));
    const f64 v = F39(std::fma(span, F39(r * Lfs32(0x0074C990u)), lo));
    Stfs32(o + 108, v);
    ctx.fpr[1] = v;
    ret(ctx, o);
}

// u32 f(Vec* v, const Rec* s) (from f_004AED50): appends the 128-byte
// record s to v (data +0, u16 capacity +4, count +8, peak +10;
// PtrVector_Reserve(v, 2 * capacity) when full): words +0..+8, +16..+24,
// +32..+40, +48..+56, +64..+72, +80..+88, +96, +100, +104, +108 and the
// byte +112. r3 = the new index, f1 = the f32 +108.
DECOMP(00788A00, Vec128_Push_00788A00) {
    const u32 v = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + 8);
    }
    const u32 d = load<u32>(v) + ((n << 7) & 0x007FFF80u);
    for (u32 r = 0; r < 6; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(d + 16 * r + 4 * k, load<u32>(s + 16 * r + 4 * k));
    store<u32>(d + 96, load<u32>(s + 96));
    store<u32>(d + 100, load<u32>(s + 100));
    store<u32>(d + 104, load<u32>(s + 104));
    store<u32>(d + 108, load<u32>(s + 108));
    store<u8>(d + 112, load<u8>(s + 112));
    ctx.fpr[1] = Lfs32(s + 108);
    const u32 n2 = load<u16>(v + 8) + 1u;
    const u32 peak = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n2));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n2 & 0xFFFFu) ? peak : n2));
    ret(ctx, (n2 & 0xFFFFu) - 1u);
}

// ---------------------------------------------------------------- sweep: 83 instructions
// void f(T* o) (no direct caller; an event handler): by o->+4: 1 sets
// f32 o->+312 = lo + (hi - lo) * rand (lo / hi = the f32 entries of the
// table (o->+308)->+12 at the indices (*0x00F948BC)->+16->+64 / (*0x00F948D4)
// ->+16->+64; the old random value of RandStep28 * 2^-16); 10 and 3..8
// except 6 clear bits 0x4 and 0x10 of u8 o->+304; 15 clears bit 0x4; 18
// copies bit 0 of the table byte at index (*0x00F9482C)->+16->+64 into
// bit 0x40 of u8 o->+305. r3 = o; f1 = the new value (state 1).
DECOMP(000232A8, State_OnEvent_000232A8) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = load<s32>(o + 4);
    auto idx = [](u32 g) { return load<u32>(load<u32>(g + 16) + 64); };
    if (s == 1) {
        const u32 tb = load<u32>(load<u32>(o + 308) + 12);
        const u32 ia = idx(0x00F948D4u), ib = idx(0x00F948BCu);
        const f64 hi = Lfs32(tb + (ia << 2)), lo = Lfs32(tb + (ib << 2));
        const f64 r = F39(static_cast<f64>(static_cast<s64>(RandStep28())));
        const f64 v = F39(std::fma(F39(hi - lo), F39(r * Lfs32(0x000232A4u)), lo));
        Stfs32(o + 312, v);
        ctx.fpr[1] = v;
    } else if (s == 18) {
        const u32 tb = load<u32>(load<u32>(o + 308) + 12);
        const u32 v = load<u8>(tb + (idx(0x00F9482Cu) << 2));
        store<u8>(o + 305, static_cast<u8>((load<u8>(o + 305) & ~0x40u) | ((v << 6) & 0x40u)));
    } else if (s == 15) {
        store<u8>(o + 304, static_cast<u8>(load<u8>(o + 304) & ~0x4u));
    } else if (s == 10 || (s > 2 && s <= 8 && s != 6)) {
        store<u8>(o + 304, static_cast<u8>(load<u8>(o + 304) & ~0x14u));
    }
    ret(ctx, o);
}

// u32 f(T* o, Effect* x, Timer* y) (no direct caller): for the owner z
// (+280, type 0x02235C33 at +276) of c->+28 of the component c =
// Effect_FindComponent(x, o->+12) with c->vt[8](c) == 5: the value v =
// o->+20, or while y->+32 < T (T = o->+24 >= 0) the lerp o->+16 + (o->+20 -
// o->+16) * y->+32 / T, is scaled by 255 (0x0007712C) and truncated
// (fctiwz) into z->vt[29](z, u8, u8 o->+28, 1, 0) (OPD / entry after the
// arguments; r3 = its; 0, the vt result or 0x02235C33 when there is no
// owner). f1 = the fctiwz result register.
DECOMP(00077138, EffectCmd_SetAlpha_00077138) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), y = arg<u32>(ctx, 2);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 12)}));
    if (c == 0 || static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) return;
    const u32 d = load<u32>(c + 28);
    if (d == 0) return;  // r3 = 5 (the vt result)
    ctx.gpr[3] = 0x02235C33u;
    if (load<u32>(d + 276) != 0x02235C33u) return;
    const u32 z = load<u32>(d + 280);
    if (z == 0) return;
    constexpr u32 K = 0x0007712Cu;  // {1e-5f, 255.0f}
    const f64 T = Lfs32(o + 24);
    f64 v = Lfs32(o + 20);
    if (T > Lfs32(K) || T == Lfs32(K)) {
        const f64 t = Lfs32(y + 32);
        if (t < T) {
            const f64 a = Lfs32(o + 16);
            v = F39(a + F39(F39(F39(v - a) * t) / T));
        }
    }
    const u64 i = rt::fctiw(F39(v * Lfs32(K + 4)), true);
    ctx.fpr[1] = std::bit_cast<f64>(0xFFF8000000000000ull | i);  // fctiwz leaves the integer in f1 (high word 0xFFF80000)
    VCall(ctx, z, 116, {z, i & 0xFFu, load<u8>(o + 28), 1, 0});
}

// T* f(?, XmlCursor* x) (no direct caller): a 24-byte node (Mem_Alloc(24,
// 0, 16); 0 on failure) set up by Obj_InitVt_B21C98(p, x), +12 = 0, u8 +18
// = 0, u8 +16 = 1, vtable 0x00B11140, u8 +17 = 1, u8 +19 / +20 = 0; hash
// 0xE89DF217 -> +12 (default 0), bools 0x7A281983 -> +16, 0x6557DBDD ->
// +17 (default and preset: u8 +16 == 0), 0x03A6C100 -> +18, 0x04A4252F ->
// +19. r3 = p.
DECOMP(0009C878, XmlNew_Node24d_0009C878) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u8>(p + 18, 0);
    store<u8>(p + 16, 1);
    store<u32>(p, 0x00B11140u);
    store<u8>(p + 17, 1);
    store<u8>(p + 19, 0);
    store<u8>(p + 20, 0);
    call(ctx, 0x004FA14Cu, {x, 0xE89DF217u, p + 12, 0});
    call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 16, load<u8>(p + 16)});
    const u32 inv = load<u8>(p + 16) == 0 ? 1u : 0u;
    store<u8>(p + 17, static_cast<u8>(inv));
    call(ctx, 0x004F986Cu, {x, 0x6557DBDDu, p + 17, inv});
    call(ctx, 0x004F986Cu, {x, 0x03A6C100u, p + 18, load<u8>(p + 18)});
    call(ctx, 0x004F986Cu, {x, 0x04A4252Fu, p + 19, load<u8>(p + 19)});
    ret(ctx, p);
}

// u32 f(Obj* o, Src* x) (no direct caller): the 4x3 transform o + 448 =
// the identity 0x01164BD0 with the translation x->vt[71](&t, x) (OPD /
// entry after the arguments; the identity's translation without x); then
// q->vt[21](q, o + 448) for q = o->+440 (when set). r3 = 0 or its; f1 =
// the translation's x.
DECOMP(000EF25C, Obj_SetTranslation_000EF25C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    constexpr u32 I = 0x01164BD0u;
    u32 rows[9];
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) rows[3 * r + k] = load<u32>(I + 16 * r + 4 * k);
    u32 t0 = load<u32>(I + 48), t1 = load<u32>(I + 52), t2 = load<u32>(I + 56);
    ctx.gpr[3] = x;
    if (x != 0) {
        Stack fr(ctx, 16);  // t (sp+112)
        const u32 opd = load<u32>(load<u32>(x) + 284);
        call_ptr(ctx, opd, {fr.addr(), x, opd, load<u32>(opd)});
        t0 = load<u32>(fr.addr());
        t1 = load<u32>(fr.addr() + 4);
        t2 = load<u32>(fr.addr() + 8);
    }
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 448 + 16 * r + 4 * k, rows[3 * r + k]);
    store<u32>(o + 496, t0);
    store<u32>(o + 500, t1);
    store<u32>(o + 504, t2);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(t0));
    const u32 q = load<u32>(o + 440);
    ctx.gpr[3] = q;
    if (q != 0) VCall(ctx, q, 84, {q, o + 448});
}

// u32 f(Obj* o, Msg* m) (no direct caller): unless the sender m->+4 has
// bit 0x400 of +192 or the factor f = f32 (o->+216)->+232 is not above 0
// (r3 = o's vtable), r = o->vt[88](o, m->+4, m->+8; f1 = m->+52 * f) (OPD in
// r4, entry in r7) sets bit 0x100 of u16 m->+14 when 0; then o->+736 =
// the heading of m's point (+16, +20) from o + 144 (Atan2f(dz, dx) * 180 /
// pi, wrapped into [0, 360]). r3 = 0x001702B8, f1 = the heading.
DECOMP(001702C8, Obj_FaceSender_001702C8) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 t = load<u32>(m + 4);
    const u32 s = load<u32>(o + 216);
    const u32 vt = load<u32>(o);
    ctx.gpr[3] = vt;
    if (t != 0 && (load<u32>(t + 192) & 0x400u) != 0) return;
    constexpr u32 K = 0x001702B8u;  // {0.0f, 180.0f, pi, 360.0f}
    const f64 f = Lfs32(s + 232);
    const f64 k0 = Lfs32(K);
    if (!(f > k0)) {
        ctx.fpr[1] = f;
        return;
    }
    const u32 opd = load<u32>(vt + 352);
    ctx.fpr[1] = F39(Lfs32(m + 52) * f);
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, opd, t, load<u32>(m + 8), load<u32>(opd)}));
    const u16 h = load<u16>(m + 14);
    store<u16>(m + 14, static_cast<u16>((h & ~0x100u) | (r == 0 ? 0x100u : 0u)));
    const u32 p = o + 144;
    const f64 dx = F39(Lfs32(m + 16) - Lfs32(p)), dz = F39(Lfs32(m + 20) - Lfs32(p + 4));
    f64 a = callf(ctx, 0x005E665Cu, {}, {dz, dx});
    a = F39(F39(a * Lfs32(K + 4)) / Lfs32(K + 8));
    if (a < k0) a = F39(a + Lfs32(K + 12));
    if (a > Lfs32(K + 12)) a = F39(a - Lfs32(K + 12));
    Stfs32(o + 736, a);
    ctx.fpr[1] = a;
    ret(ctx, K);
}

// u32 f(Obj* o, Obj* x) (from Obj_TestAndCall_00221DD0): with bit 0x4 of
// o->+436 the test x->vt[56](x, o) (OPD / entry after the arguments) must
// pass (r3 = 0 otherwise, or when bit 0x2 of o->+436 and bit 0x80000000 of
// o->+468 are both set); then Area_Enter(o, x) != 0, and 0x0021F4E8(o, x)
// unless bit 0x80000000 of o->+468 is set while bit 0x2 of o->+436 is set
// or the record of o->+460 (Obj_Clear68Set_0049E8A8 on the stack) is
// found by Obj_FindRecord76_002D9994((*0x00F64C20)->+40, &rec). r3 = 1
// after Area_Enter.
DECOMP(0021F9E8, Obj_TryEnterArea_0021F9E8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if ((load<u32>(o + 436) & 0x4u) != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, x, 224, {x, o}));
        if (((load<u32>(o + 436) & 0x2u) != 0 && (load<u32>(o + 468) & 0x80000000u) != 0) || r == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    if (static_cast<u32>(call(ctx, 0x0020E7A4u, {o, x})) == 0) {
        ret(ctx, 0u);
        return;
    }
    bool check = (load<u32>(o + 436) & 0x2u) != 0;
    if (!check) {
        const u32 h = load<u32>(o + 460);
        if (h != 0) {
            const u32 w = load<u32>(load<u32>(0x00F64C20u) + 40);
            if (w != 0) {
                Stack fr(ctx, 112);  // the record (sp+112; frame of 240 bytes)
                call(ctx, 0x0049E8A8u, {fr.addr(), h});
                check = static_cast<u32>(call(ctx, 0x002D9994u, {w, fr.addr()})) != 0;
            }
        }
    }
    if (!check || (load<u32>(o + 468) & 0x80000000u) == 0) call(ctx, 0x0021F4E8u, {o, x});
    ret(ctx, 1u);
}

// void f(Base80* p) (no direct caller): AdjDeletePair112_39 with vtables
// 0x00B1C258 / 0x00B1C2A8 (the thunk of 0x0027CF60).
DECOMP(0027D0A8, Obj_AdjDelete112_0027D0A8) { AdjDeletePair112_39(ctx, 0x00B1C258u, 0x00B1C2A8u); }

// void f(Base80* p) (no direct caller): AdjDeletePair112_39 with vtables
// 0x00B1C328 / 0x00B1C378 (the thunk of 0x0027EA94).
DECOMP(0027EBDC, Obj_AdjDelete112b_0027EBDC) { AdjDeletePair112_39(ctx, 0x00B1C328u, 0x00B1C378u); }

// void f(Base80* p) (no direct caller): AdjDeletePair112_39 with vtables
// 0x00B1C3F8 / 0x00B1C444 (the thunk of 0x00280244).
DECOMP(0028038C, Obj_AdjDelete112c_0028038C) { AdjDeletePair112_39(ctx, 0x00B1C3F8u, 0x00B1C444u); }

// u32 f(T* o) (no direct caller): the u16 o->+368 handles of o->+360 go
// back to the pool (Pool_FreeHandle_004B2A04((*0x00F557F8) + 4, h)); then
// while o->+448 > 0 the emitter o->+384 is stopped (Emitter_Stop(+384, 1))
// and the last 8-byte pair (o + 384 + 8 (n - 1)) moved into o->+384 /
// o->+388 and cleared, n - 1 stored. r3 as the original leaves it.
DECOMP(002977B4, Obj_StopEmitters_002977B4) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u16>(o + 368); ++i) {
        const u32 h = load<u32>(load<u32>(o + 360) + 4 * i);
        call(ctx, 0x004B2A04u, {load<u32>(0x00F557F8u) + 4, h});
    }
    s32 n = load<s32>(o + 448);
    ctx.gpr[3] = static_cast<u32>(n);
    while (n > 0) {
        call(ctx, 0x00443424u, {load<u32>(o + 384), 1});
        const u32 m = load<u32>(o + 448) - 1;
        store<u32>(o + 448, m);
        store<u32>(o + 384, load<u32>(o + 384 + 8 * m));
        store<u32>(o + 388, load<u32>(o + 388 + 8 * load<u32>(o + 448)));
        store<u32>(o + 384 + 8 * load<u32>(o + 448), 0);
        const u32 c = load<u32>(o + 448);
        store<u32>(o + 388 + 8 * c, 0);
        ctx.gpr[3] = o + 388 + 8 * c;
        n = static_cast<s32>(c);
    }
}

