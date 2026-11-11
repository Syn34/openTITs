// Game code, part 38: the small-function sweep continued from the last 76
// instruction functions (size order; obj37*.cpp did 73-76). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj38.h"

using namespace sly4;
using namespace sly4::obj38;

// ---------------------------------------------------------------- sweep: 76 instructions
// u32 f(T* o, Cmd* c) (no direct caller: a vtable slot): with the queue q =
// c->+12, the two s16 indices i = o->+4 and j = o->+2 select 128-byte
// records of (*0x00F65038)->+24 (a record address 0 for -1); the byte pair
// of record j sits at j + u16 *0x0106CF7E when its byte +2 is not 0 (else
// at address 0, read anyway: a quirk), and each byte b picks the word at
// record i + u16 *0x0106CF70 + 4 * b: Queue_Push_004705F8(q, q,
// word(b0), word(b1), 1) (q passed twice). r3 = 0 (also without q).
DECOMP(0039366C, Cmd_PushRecordPair_0039366C) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u32 q = load<u32>(c + 12);
    if (q == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 g = load<u32>(0x00F65038u);
    const s32 i = static_cast<s16>(load<u16>(o + 4));
    const s32 j = static_cast<s16>(load<u16>(o + 2));
    auto rec = [&](s32 k) { return k != -1 ? load<u32>(g + 24) + (static_cast<u32>(k) << 7) : 0u; };
    auto pair = [&]() {
        const u32 r = rec(j);
        return load<u8>(r + 2) != 0 ? r + load<u16>(0x0106CF7Eu) : 0u;
    };
    const u32 h = load<u16>(0x0106CF70u);
    const u32 w0 = load<u32>(rec(i) + h + (static_cast<u32>(load<u8>(pair())) << 2));
    const u32 w1 = load<u32>(rec(i) + h + (static_cast<u32>(load<u8>(pair() + 1)) << 2));
    call(ctx, 0x004705F8u, {q, q, w0, w1, 1});
    ret(ctx, 0u);
}

// u32 f(Cmd* c) (no direct caller): the object x = Obj_FindByHandle42C2B0(
// c->+12, 0); without it, or when bit 3 of x->+192 is clear, a warning
// (String_Format "SequenceCommandEnablePhysics failed because ..." with
// the hash name of c->+12); otherwise, for the physics object y =
// x->vt[16](x) when not 0: when the u8 c->+17 is set, y->vt[11](y, u8
// c->+16) (OPD / entry after the arguments), then Obj_SetPhysicsActive(y,
// u8 c->+16). Always ends with
// Flag_Set8_40000000(c) (r3 = its).
DECOMP(0042E27C, SeqCmd_EnablePhysics_0042E27C) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = static_cast<u32>(call(ctx, 0x0042C2B0u, {load<u32>(c + 12), 0}));
    const u32 h = load<u32>(c + 12);
    if (x == 0 || (load<u32>(x + 192) & 0x8u) == 0) {
        const u32 fmt = x == 0 ? 0x00AE2364u : 0x00AE2314u;
        const u32 s = static_cast<u32>(call(ctx, o38fn::Hash_ToString, {h}));
        call(ctx, o38fn::String_Format, {fmt, s});
    } else if (const u32 y = static_cast<u32>(VCall(ctx, x, 64, {x})); y != 0) {
        if (load<u8>(c + 17) != 0) VCall(ctx, y, 44, {y, load<u8>(c + 16)});
        call(ctx, 0x00260C14u, {y, load<u8>(c + 16)});
    }
    call(ctx, 0x0042CA60u, {c});
}

// void f(f32 v) (World_UpdateAudioFilter): while the u8 *0x010AEC60 is
// set, v (or, when the u8 0x010B0808 is set, the f32 0x010B0858) above 0
// that differs from the current 0x00F65BE4 by at least 1e-5 becomes the
// current value and, under Snd_Lock, goes to the f32 slot 0x00F8E568[i]
// (with the word 0x011D7CC8[i] = 1) of each of the 31 entries
// 0x010B0104[i] whose u8 +72 is set; then the word 0x011D86A0 = 1 and
// Snd_Unlock. r3 / f1 as the original leaves them.
DECOMP(0044D740, Audio_SetFilterValue_0044D740) {
    f64 v = farg(ctx, 0);
    if (load<u8>(0x010AEC60u) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(0x010B0808u) != 0) v = Lfs32(0x010B0858u);
    constexpr u32 K = 0x0044D738u;  // {0.0f, 1e-5f}
    const f64 k0 = Lfs32(K);
    if (v < k0 || v == k0) {
        ctx.gpr[3] = K;
        ctx.fpr[1] = k0;
        return;
    }
    const f64 d = std::fabs(F38(Lfs32(0x00F65BE4u) - v));
    if (d < Lfs32(K + 4)) {
        ctx.gpr[3] = 0x00F60000u;
        ctx.fpr[1] = d;
        return;
    }
    Stfs32(0x00F65BE4u, v);
    ctx.fpr[1] = d;
    call(ctx, o38fn::Snd_Lock);
    for (u32 i = 0; i < 31; ++i) {
        if (load<u8>(load<u32>(0x010B0104u + 4 * i) + 72) == 0) continue;
        Stfs32(0x00F8E568u + 4 * i, v);
        store<u32>(0x011D7CC8u + 4 * i, 1);
    }
    store<u32>(0x011D86A0u, 1);
    call(ctx, o38fn::Snd_Unlock);
}

// T* f(T* o, s32 state): o->+44 = state, with the tuning t = o->+32 + 16
// and k = {0.0f, 1.0f, 0.0}: state 0 clears the f32 +144, 2 sets it to
// 1.0; states 1 / 3 restart a fade over t->+84 / t->+88: when that is 0,
// f(o, 2) / f(o, 0) instead (r3 = its); otherwise +68 = +144, +72 = 1.0
// (state 1, from 1 - +144) or 0 (state 3, from +144), +52 = 0, +56 = the
// fade time duration * remaining, then +52 = the fsel clamp of it (always
// 0). Other states only store +44. r3 = o, f1 as the original leaves it.
DECOMP(004A9130, Fade_SetState_004A9130) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = arg<s32>(ctx, 1);
    const u32 t = load<u32>(o + 32) + 16;
    store<u32>(o + 44, static_cast<u32>(s));
    constexpr u32 K = 0x004A9120u;
    auto fade = [&](u32 toff, bool in) {
        const f64 k0 = Lfs32(K);
        ctx.fpr[1] = k0;
        if (Lfs32(t + toff) == k0) {
            call(ctx, 0x004A9130u, {o, in ? 2u : 0u});
            return;
        }
        const u32 w = load<u32>(o + 144);
        const f64 cur = static_cast<f64>(std::bit_cast<f32>(w));
        f64 rem = cur;
        if (in) {
            store<u32>(o + 68, w);
            rem = F38(Lfs32(K + 4) - cur);
            store<u32>(o + 72, 0x3F800000u);
        } else {
            store<u32>(o + 72, 0);
            store<u32>(o + 68, w);
        }
        store<u32>(o + 52, 0);
        const f64 d = F38(Lfs32(t + toff) * rem);
        const f64 e = F38(d - k0);
        Stfs32(o + 56, d);
        f64 r = Fsel38(e, k0, d);
        r = Fsel38(r, r, std::bit_cast<f64>(load<u64>(K + 8)));
        r = F38(r);
        Stfs32(o + 52, r);
        ctx.fpr[1] = r;
        ctx.gpr[3] = o;
    };
    if (s == 0) {
        store<u32>(o + 144, 0);
    } else if (s == 1) {
        fade(84, true);
    } else if (s == 2) {
        store<u32>(o + 144, 0x3F800000u);
    } else if (s == 3) {
        fade(88, false);
    }
}

// u32 f(T* o) (no direct caller: a base destructor body): o->vt =
// 0x00B22F60; every entry e of the list *0x00F668A0 (next +28) whose +4
// is o gets e->vt[1](e); o is unlinked from the list *0x00F6689C (next
// +52); then each node n of o's own list +48 (next +8) gets n->vt[1](n),
// +48 following along. r3 = 0.
DECOMP(004E32A4, Registrant_Unregister_004E32A4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B22F60u);
    for (u32 e = load<u32>(0x00F668A0u); e != 0;) {
        const u32 nx = load<u32>(e + 28);
        if (load<u32>(e + 4) == o) VCall(ctx, e, 4, {e});
        e = nx;
    }
    for (u32 p = 0, e = load<u32>(0x00F6689Cu); e != 0; p = e, e = load<u32>(e + 52)) {
        if (e != o) continue;
        if (p != 0) store<u32>(p + 52, load<u32>(o + 52));
        else store<u32>(0x00F6689Cu, load<u32>(o + 52));
        break;
    }
    for (u32 n = load<u32>(o + 48); n != 0;) {
        const u32 nx = load<u32>(n + 8);
        VCall(ctx, n, 4, {n});
        n = nx;
        store<u32>(o + 48, nx);
    }
    ret(ctx, 0u);
}

// void f(Pool* out, u32 n, u32 extra, Allocator* a) (from Clip_Init4B1708):
// the pool setup PoolInit38 with 76-byte elements.
DECOMP(00789280, Pool_Init76_00789280) { PoolInit38(ctx, 76); }

// ---------------------------------------------------------------- sweep: 77 instructions
// Node* f(Node* n) (from 0x0001574C): the root of n's parent chain in the
// 20-byte node array g = *0x00F557F4 (data +12, count u16 (+4)->+2; an
// s16 index out of range resolves to address 0, followed anyway): when n's
// s16 parent +10 differs from its +8 while its +14 equals +8, the parents
// are followed from +10 until a node whose +10 equals its +14; otherwise
// the node of n's s16 +12 (0 when out of range).
DECOMP(000154AC, Node_FindRoot_000154AC) {
    const u32 n = arg<u32>(ctx, 0);
    const s32 p = static_cast<s16>(load<u16>(n + 10));
    const s32 s = static_cast<s16>(load<u16>(n + 8));
    auto node = [](u32 g, s32 i) -> u32 {
        if (i < 0 || i >= static_cast<s32>(load<u16>(load<u32>(g + 4) + 2))) return 0;
        return load<u32>(g + 12) + static_cast<u32>(i) * 20u;
    };
    if (p == s || static_cast<s16>(load<u16>(n + 14)) != s) {
        const s32 i = static_cast<s16>(load<u16>(n + 12));
        ret(ctx, node(load<u32>(0x00F557F4u), i));
        return;
    }
    const u32 g = load<u32>(0x00F557F4u);
    u32 r = node(g, p);
    while (static_cast<s16>(load<u16>(r + 10)) != static_cast<s16>(load<u16>(r + 14)))
        r = node(g, static_cast<s16>(load<u16>(r + 10)));
    ret(ctx, r);
}

// u32 f(Cmd* c) (no direct caller: a vtable slot): the camera def of the
// hash c->+12 (CameraDefMap_Find in (*0x00F8F134)->+16) and the object of
// c->+16 (PointMap_Find in (*0x00F8F154)->+16), each on a stack copy of
// its key (a missing one only gets Hash_ToString, its warning compiled
// out); when either is found, p = SmallAlloc(24) is built by
// VtB20D28_Ctor(p, def, obj; f1 = the f32 c->+20) when not 0 and pushed
// with CmdList_Push_0042C138(p) (r3 = its). r3 as the original leaves it.
DECOMP(0009ADE0, SeqCmd_PushCamera_0009ADE0) {
    const u32 c = arg<u32>(ctx, 0);
    u32 def = 0, obj = 0, r3 = 0;
    Stack fr(ctx, 16);
    if (const u32 k = load<u32>(c + 12); k != 0) {
        store<u32>(fr.addr() + 4, k);
        r3 = static_cast<u32>(call(ctx, 0x0073374Cu, {load<u32>(load<u32>(0x00F8F134u) + 16), fr.addr() + 4}));
        def = load<u32>(r3);
        if (def == 0) r3 = static_cast<u32>(call(ctx, o38fn::Hash_ToString, {load<u32>(c + 12)}));
    }
    r3 = load<u32>(c + 16);
    if (r3 != 0) {
        store<u32>(fr.addr(), r3);
        r3 = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), fr.addr()}));
        obj = load<u32>(r3);
        if (obj == 0) r3 = static_cast<u32>(call(ctx, o38fn::Hash_ToString, {load<u32>(c + 16)}));
    }
    if (def == 0 && obj == 0) {
        ret(ctx, r3);
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {24}));
    if (p != 0) callf(ctx, 0x0042D20Cu, {p, def, obj}, {Lfs32(c + 20)});
    call(ctx, 0x0042C138u, {p});
}

// AnimCmd* f(-, XmlCursor* x): p = Mem_Alloc(28, 0, 16); when not 0,
// AnimCmd_ConstructBase(p), +12..+24 = 0, vtable 0x00B11670, then from x:
// +12 = the s32 0x368328E5 (default -1, which becomes 0), the f32s
// 0x6D8F483A -> +16, 0xC7F891C1 -> +20 and 0xB4FCA1EC -> +24 (each
// defaulting to the current value). r3 = p.
DECOMP(000A2ED8, AnimCmd_New_000A2ED8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004B9CB8u, {p});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    store<u32>(p, 0x00B11670u);
    {
        Stack fr(ctx, 16);
        store<u32>(fr.addr(), 0xFFFFFFFFu);
        call(ctx, 0x004F99BCu, {x, 0x368328E5u, fr.addr(), 0xFFFFFFFFu});
        u32 v = load<u32>(fr.addr());
        if (v == 0xFFFFFFFFu) {
            v = 0;
            store<u32>(fr.addr(), 0);
        }
        store<u32>(p + 12, v);
    }
    callf(ctx, 0x004F9B20u, {x, 0x6D8F483Au, p + 16}, {Lfs32(p + 16)});
    callf(ctx, 0x004F9B20u, {x, 0xC7F891C1u, p + 20}, {Lfs32(p + 20)});
    callf(ctx, 0x004F9B20u, {x, 0xB4FCA1ECu, p + 24}, {Lfs32(p + 24)});
    ret(ctx, p);
}

// void f(T* o) (no direct caller: a destructor body): vtable 0x00B11E00;
// the effect e = o->+4->vt[68](o->+4) (OPD / entry after the arguments);
// its component c = Effect_FindComponent(e, 0xB0F92970), when its
// vt[8](c) is 5: c->+28->vt[2](c->+28) when set, then c->+28 = c->+36 =
// 0; o->+212->vt[10](o->+212, 0) when set; vtable 0x00B11DC4 (r3 = it).
DECOMP(000B48FC, EffectUser_Dtor_000B48FC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B11E00u);
    const u32 h = load<u32>(o + 4);
    if (const u32 e = static_cast<u32>(VCall(ctx, h, 272, {h})); e != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, 0xB0F92970u}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
            if (const u32 s = load<u32>(c + 28); s != 0) VCall(ctx, s, 8, {s});
            store<u32>(c + 28, 0);
            store<u32>(c + 36, 0);
        }
    }
    if (const u32 s = load<u32>(o + 212); s != 0) VCall(ctx, s, 40, {s, 0});
    store<u32>(o, 0x00B11DC4u);
    ret(ctx, 0x00B11DC4u);
}

// u32 f(T* o, u8 on, u8 apply) (no direct caller: a vtable slot): the peer
// p = o->+160 of type 8 (bits 10..15 of p->+4) whose bit 7 of the u8 +322
// differs from on gets it (bit 7 = bit 0 of on) and p->vt[9](p) (OPD /
// entry after the arguments); when apply, each 240-byte entry of the
// array o->+340 (data +0, u16 count +8, re-read each time) whose +192 is
// set gets x->vt[31](x, on, apply). r3 as the original leaves it.
DECOMP(000DA550, Peer_SetFlag7_000DA550) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu, apply = arg<u32>(ctx, 2) & 0xFFu;
    u32 r3 = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 160);
    if (p != 0 && ((load<u32>(p + 4) >> 10) & 0x3Fu) == 8) {
        const u32 b = load<u8>(p + 322);
        if (((b >> 7) & 1u) != on) {
            store<u8>(p + 322, static_cast<u8>((b & ~0x80u) | ((r3 << 7) & 0x80u)));
            r3 = static_cast<u32>(VCall(ctx, p, 36, {p}));
        }
    }
    if (apply == 0) {
        ret(ctx, r3);
        return;
    }
    for (u32 i = 0;; ++i) {
        const u32 a = load<u32>(o + 340);
        const u32 n = a != 0 ? load<u16>(a + 8) : 0u;
        if (i >= n) {
            ret(ctx, a);
            return;
        }
        if (const u32 x = load<u32>(load<u32>(a) + 240 * i + 192); x != 0) VCall(ctx, x, 124, {x, on, apply});
    }
}

// u32 f(T* o, Msg* m) (no direct caller: a message handler): for the
// message 0xCCF83851 whose sender m->+0 is o->+436 (when set): that one
// gets vt[10](x, 0) (OPD / entry after the arguments) and +436 = 0, and
// o->+620's +48 the same (when both are set); otherwise, when the sender
// is o->+432, that one likewise. Then (always) 0x000E1060(o, m) (r3 =
// its).
DECOMP(000ED74C, Obj_OnRelease_000ED74C) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    if (load<u32>(m + 8) == 0xCCF83851u) {
        const u32 s = load<u32>(m);
        if (const u32 a = load<u32>(o + 436); a != 0 && s == a) {
            VCall(ctx, a, 40, {a, 0});
            const u32 b = load<u32>(o + 620);
            store<u32>(o + 436, 0);
            if (b != 0) {
                if (const u32 c = load<u32>(b + 48); c != 0) VCall(ctx, c, 40, {c, 0});
                store<u32>(b + 48, 0);
            }
        } else if (const u32 c = load<u32>(o + 432); c != 0 && s == c) {
            VCall(ctx, c, 40, {c, 0});
            store<u32>(o + 432, 0);
        }
    }
    call(ctx, 0x000E1060u, {o, m});
}

// u32 f(T* o): the body a = o->vt[16](o) and its state b = a->vt[2](a)
// (OPD / entry after the arguments); b->+16..+28 = Quat_FromMatrix of
// o + 96 (through a stack quaternion, copied as raw words);
// Body_SetMode(a, 2); o->vt[7](o, 0, 1), o->vt[7](o, 1, 0); bit 9 of
// o->+20->+48 set. r3 = that word's address.
DECOMP(0015F104, Body_SyncRotation_0015F104) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = static_cast<u32>(VCall(ctx, o, 64, {o}));
    const u32 b = static_cast<u32>(VCall(ctx, a, 8, {a}));
    {
        Stack fr(ctx, 16);
        call(ctx, 0x00555478u, {fr.addr(), o + 96});
        for (u32 k = 0; k < 16; k += 4) store<u32>(b + 16 + k, load<u32>(fr.addr() + k));
        ctx.fpr[1] = Lfs32(fr.addr() + 12);  // f1 as the original leaves it
    }
    call(ctx, 0x00261328u, {a, 2});
    VCall(ctx, o, 28, {o, 0, 1});
    VCall(ctx, o, 28, {o, 1, 0});
    const u32 w = load<u32>(o + 20) + 48;
    store<u32>(w, load<u32>(w) | 0x00400000u);
    ret(ctx, w);
}

// void f(T* o) (no direct caller: a deleting destructor): for o != 0:
// vtable 0x00B151A0; when +1896 is set, +1900 (unchecked) and +1896 get
// vt[2](x) (OPD / entry after the arguments) and are cleared, and then
// each of the u16 +1888 (re-read) 96-byte slots from +432 likewise (only
// in that case); Thunk_000D729C(o), SmallFree(o, 1920) (r3 = its).
DECOMP(001A7EA8, Obj_Delete_001A7EA8) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B151A0u);
    if (load<u32>(o + 1896) != 0) {
        const u32 a = load<u32>(o + 1900);
        VCall(ctx, a, 8, {a});
        const u32 b = load<u32>(o + 1896);
        store<u32>(o + 1900, 0);
        VCall(ctx, b, 8, {b});
        const u32 n = load<u16>(o + 1888);
        store<u32>(o + 1896, 0);
        if (n != 0) {
            for (u32 i = 0; i < load<u16>(o + 1888); ++i) {
                const u32 s = o + 432 + 96 * i;
                const u32 x = load<u32>(s);
                VCall(ctx, x, 8, {x});
                store<u32>(s, 0);
            }
        }
    }
    call(ctx, 0x000D729Cu, {o});
    call(ctx, 0x00552420u, {o, 1920});
}

// u32 f(T* o) (from ThunkArg1_001D3944): when +804 or +792 is set: a pose
// is built on the stack from o + 912 (0x001D0B40(pose, o, o + 912)) and
// applied with 0x001D3520(o, +804, +800, pose); when +792 is set, a second
// one from it (0x001D09B8(o, pose, pose2, o + 720)) with 0x001D3520(o,
// +792, +788, pose2). Then for the world object w = o->+160: bit 7 of the
// u8 +31 of w->vt[7](w) set (OPD / entry after the arguments),
// Skeleton_Update(w->vt[7](w)), WorldObj_MarkDirty(w),
// WorldObj_UpdateLod(w, 0, 0) (r3 = its). r3 = 0 when neither is set.
DECOMP(001D3810, Pose_Apply_001D3810) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 804) == 0 && load<u32>(o + 792) == 0) {
        ret(ctx, 0u);
        return;
    }
    {
        Stack fr(ctx, 144);  // the original's 112..255
        const u32 p1 = fr.addr(), p2 = fr.addr() + 64;
        call(ctx, 0x001D0B40u, {p1, o, o + 912});
        call(ctx, 0x001D3520u, {o, load<u32>(o + 804), load<u32>(o + 800), p1});
        if (load<u32>(o + 792) != 0) {
            call(ctx, 0x001D09B8u, {o, p1, p2, o + 720});
            call(ctx, 0x001D3520u, {o, load<u32>(o + 792), load<u32>(o + 788), p2});
        }
    }
    u32 w = load<u32>(o + 160);
    const u32 x = static_cast<u32>(VCall(ctx, w, 28, {w}));
    store<u8>(x + 31, static_cast<u8>(load<u8>(x + 31) | 0x80u));
    w = load<u32>(o + 160);
    const u32 y = static_cast<u32>(VCall(ctx, w, 28, {w}));
    call(ctx, 0x0057E8D0u, {y});
    call(ctx, 0x0057D000u, {load<u32>(o + 160)});
    call(ctx, 0x0057EE00u, {load<u32>(o + 160), 0, 0});
}

// u32 f(T* o, Query* q): Obj_CopyQuery_000CDB28(o, q), then q->+144 =
// +180; the u8 flags q->+148..+152 / +153 = bits 7..3 / 1 of the u8 +184
// (re-read each time); q->+156 = +188, +160 = +192, +164 = the f32 +196;
// bits 31..27 of q->+168 copied one at a time from +200 (re-read, each
// step stored). r3 = the last word.
DECOMP(0021C47C, Obj_CopyQueryFlags_0021C47C) {
    const u32 o = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28u, {o, q});
    store<u32>(q + 144, load<u32>(o + 180));
    auto bit = [&](u32 k) { return static_cast<u8>((load<u8>(o + 184) >> k) & 1u); };
    store<u8>(q + 148, bit(7));
    store<u8>(q + 149, bit(6));
    store<u8>(q + 150, bit(5));
    store<u8>(q + 151, bit(4));
    store<u8>(q + 152, bit(3));
    const u32 w188 = load<u32>(o + 188);
    const u8 b1 = bit(1);
    store<u32>(q + 156, w188);
    store<u32>(q + 160, load<u32>(o + 192));
    store<u32>(q + 164, load<u32>(o + 196));
    ctx.fpr[1] = Lfs32(o + 196);  // f1 as the original leaves it
    const u32 w168 = load<u32>(q + 168);
    store<u8>(q + 153, b1);
    u32 r = (w168 & 0x7FFFFFFFu) | (load<u32>(o + 200) & 0x80000000u);
    store<u32>(q + 168, r);
    for (u32 m = 0x40000000u; m >= 0x08000000u; m >>= 1) {
        r = (r & ~m) | (load<u32>(o + 200) & m);
        store<u32>(q + 168, r);
    }
    ret(ctx, r);
}

// u32 f(T* o) (from 0x0023C168): with the target t = o->+612->+24 (both
// set): 0x004A69C0(o + 512, o + 416, copy of t + 432) and (o + 528, o +
// 440, copy of t + 448) (stack copies of the vectors), 0x004A69C0(o + 592,
// o + 464, o + 576), then 0x00239C50(o + 608, o + 488; f1 = |o + 576|,
// single precision) (r3 = its). r3 = 0 without the target.
DECOMP(00239DA4, Aim_Update_00239DA4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 612);
    if (a == 0 || load<u32>(a + 24) == 0) {
        ret(ctx, 0u);
        return;
    }
    {
        Stack fr(ctx, 32);
        const u32 t = load<u32>(load<u32>(o + 612) + 24);
        for (u32 k = 0; k < 12; k += 4) store<u32>(fr.addr() + k, load<u32>(t + 432 + k));
        call(ctx, 0x004A69C0u, {o + 512, o + 416, fr.addr()});
        const u32 t2 = load<u32>(load<u32>(o + 612) + 24);
        for (u32 k = 0; k < 12; k += 4) store<u32>(fr.addr() + 16 + k, load<u32>(t2 + 448 + k));
        call(ctx, 0x004A69C0u, {o + 528, o + 440, fr.addr() + 16});
    }
    call(ctx, 0x004A69C0u, {o + 592, o + 464, o + 576});
    f64 l = F38(Lfs32(o + 580) * Lfs32(o + 580));
    l = F38(std::fma(Lfs32(o + 576), Lfs32(o + 576), l));
    l = F38(std::fma(Lfs32(o + 584), Lfs32(o + 584), l));
    l = F38(std::sqrt(l));
    callf(ctx, 0x00239C50u, {o + 608, o + 488}, {l});
}

// u32 f(T* o) (from 0x0025AAE0, 0x0029EF74): bits 0..1 of the u8 +2337
// cleared; with the nav source +2320 and the world object w = +2324 (both
// set): bit 1 set, WorldObj_UpdateLod(w, 0, 0), w's position (+128) copied
// to +2304; a stack NavQuery from it (NavQuery_Init630F94(q, pos, 0,
// params {0, 1, -1, 16, u8 0, 0, 0, 0}, 0, 16, 1)) is assigned to +2328
// (NavQuery_Assign62D414) and freed, and bit 0 = NavQuery_IsValid62E164(o
// + 2328). r3 = the u8 +2337 as last stored.
DECOMP(0025A55C, Nav_Rebuild_0025A55C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 2337) & 0xFCu;
    store<u8>(o + 2337, static_cast<u8>(b));
    const u32 w = load<u32>(o + 2324);
    if (load<u32>(o + 2320) == 0 || w == 0) {
        ret(ctx, b);
        return;
    }
    store<u8>(o + 2337, static_cast<u8>(b | 2u));
    call(ctx, 0x0057EE00u, {w, 0, 0});
    Stack fr(ctx, 64);  // query +0, position +8, parameters +20 (the original's 112..163)
    const u32 q = fr.addr(), pos = q + 8, prm = q + 20;
    for (u32 k = 0; k < 12; k += 4) {
        const u32 v = load<u32>(w + 128 + k);
        store<u32>(o + 2304 + k, v);
        store<u32>(pos + k, v);
    }
    ctx.fpr[1] = Lfs32(pos);  // f1 as the original leaves it
    store<u32>(prm + 0, 0);
    store<u32>(prm + 8, 0xFFFFFFFFu);
    store<u8>(prm + 16, 0);
    store<u32>(prm + 20, 0);
    store<u32>(prm + 24, 0);
    store<u32>(prm + 28, 0);
    store<u32>(prm + 12, 16);
    store<u32>(prm + 4, 1);
    call(ctx, 0x00630F94u, {q, pos, 0, prm, 0, 16, 1});
    call(ctx, 0x0062D414u, {o + 2328, q});
    call(ctx, 0x0062D08Cu, {q});
    const u32 v = static_cast<u32>(call(ctx, 0x0062E164u, {o + 2328}));
    const u32 r = (load<u8>(o + 2337) & 0xFEu) | (v & 1u);
    store<u8>(o + 2337, static_cast<u8>(r));
    ret(ctx, r);
}

// void f(T* o) (no direct caller: a destructor body): vtable 0x00B1C4C8,
// off the world list 0 (table 0x00F35278), then the shared unlist tail
// (DtorWorldUnlist38).
DECOMP(00280780, Dtor_B1C4C8_00280780) { DtorWorldUnlist38(ctx, 0x00B1C4C8u, 0, 0x00F35278u); }

// void f(T* o) (no direct caller: a destructor body): vtable 0x00B1C730,
// off the world list 1 (table 0x00F353E0), then the shared unlist tail.
DECOMP(0028F2B0, Dtor_B1C730_0028F2B0) { DtorWorldUnlist38(ctx, 0x00B1C730u, 1, 0x00F353E0u); }

// u32 f(T* o, u8 v) (from 0x0030C14C): for each of the u8 states +9 (its
// view +4) and then +8 (its view +0) that differs from v: the state = v
// and, when the view is set, view->vt[22](view, v, 1, 1, 1) and
// view->vt[23](view, v == 0, -1) (OPD / entry after the arguments; the
// view re-read). r3 as the original leaves it.
DECOMP(0030C018, View_SetState_0030C018) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1) & 0xFFu;
    u32 r3 = 0;
    auto set = [&](u32 boff, u32 voff) {
        r3 = load<u8>(o + boff);
        if (v == r3) return;
        r3 = load<u32>(o + voff);
        store<u8>(o + boff, static_cast<u8>(v));
        if (r3 == 0) return;
        VCall(ctx, r3, 88, {r3, v, 1, 1, 1});
        const u32 x = load<u32>(o + voff);
        r3 = static_cast<u32>(VCall(ctx, x, 92, {x, v == 0 ? 1u : 0u, 0xFFFFFFFFu}));
    };
    set(9, 4);
    set(8, 0);
    ret(ctx, r3);
}

// u32 f(T* o, T* x, -, u8 force; f32 t) (from many vtable thunks): unless
// GetU8_0_004A08C0(o + 556) is 0 without force, or x is already +672: the
// entry e of o->+216's pointer array +304 (data +0, u16 count +8) whose
// +32 is x (0 for x = 0 or none); t < 0 takes e's f32 +52 (when e is set);
// when +656 is set, 0x003055E8(+656, e, e, 0; f1 = t) and +672 = x (r3 =
// its). r3 / f1 as the original leaves them.
DECOMP(0030DBF0, Anim_SetTarget_0030DBF0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), force = arg<u32>(ctx, 3) & 0xFFu;
    f64 t = farg(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x004A08C0u, {o + 556})) == 0 && force == 0) {
        ret(ctx, 0u);
        return;
    }
    if (x == load<u32>(o + 672)) {
        ret(ctx, 0u);
        return;
    }
    u32 e = 0;
    if (x != 0) {
        const u32 a = load<u32>(o + 216) + 304;
        for (u32 i = 0; i < load<u16>(a + 8); ++i) {
            const u32 d = load<u32>(a);
            if (load<u32>(load<u32>(d + 4 * i) + 32) == x) {
                e = load<u32>(d + 4 * i);
                break;
            }
        }
    }
    const f64 k = Lfs32(0x0030DBE8u);  // 0.0f
    if (t < k && e != 0) t = Lfs32(e + 52);
    const u32 h = load<u32>(o + 656);
    if (h == 0) {
        ctx.fpr[1] = k;
        ret(ctx, e);
        return;
    }
    callf(ctx, 0x003055E8u, {h, e, e, 0}, {t});
    store<u32>(o + 672, x);
}

// u32 f(Section* s, s32 mode, u8 keep) (no direct caller: a vtable slot):
// unless the mode is unchanged (u8 +18) without keep: entering mode 4 with
// no world child (the word at s->+8 + 0x1281F4) calls s->vt[27](s) (OPD /
// entry after the arguments) and keeps the new child in +2572, which gets
// vt[2](child) without keep; modes other than 4..6 release the kept child
// +2572 through World_ReleaseChild1281F4(s->+8, child) while it is still
// the world's. Then Section_SetModeBase_003260F0(s, mode, keep) (r3 =
// its).
DECOMP(00328BF0, Section_SetMode_00328BF0) {
    const u32 s = arg<u32>(ctx, 0), keep = arg<u32>(ctx, 2) & 0xFFu;
    const s32 m = arg<s32>(ctx, 1);
    constexpr u32 kChild = 0x001281F4u;
    if (keep != 0 || m != static_cast<s32>(load<u8>(s + 18))) {
        if (m == 4) {
            if (load<u32>(load<u32>(s + 8) + kChild) == 0) {
                VCall(ctx, s, 108, {s});
                const u32 c = load<u32>(load<u32>(s + 8) + kChild);
                store<u32>(s + 2572, c);
                if (keep == 0) VCall(ctx, c, 8, {c});
            }
        } else if (m != 5 && m != 6) {
            const u32 c = load<u32>(s + 2572);
            if (c != 0 && load<u32>(load<u32>(s + 8) + kChild) == c) call(ctx, 0x0047C490u, {load<u32>(s + 8), c});
        }
    }
    call(ctx, 0x003260F0u, {s, static_cast<u32>(m), keep});
}

// u32 f(Owner* a, HkEntity* b) (no direct caller): HkRemoveEntity38 with
// the map a + 12 and the calls on a - 4 / a.
DECOMP(003A9D7C, HkOwner_RemoveEntity_003A9D7C) { HkRemoveEntity38(ctx, 12, 0u - 4u, 0); }

// u32 f(T* o, a, b, x) (from Obj_NewChild_0032A440): SubMachine_Construct(o,
// a, b, 0, x), +56 = x, vtable 0x00B20338, +80..+88 = the vector
// 0x01164B40, +60..+68 and +96..+108 = 0; the pointer vector +112
// (PtrVector_Construct, u16 +118 = 8, PtrVector_Init(v, 32, 0, 1)); nine
// 36-byte Ctor_B21B88_004A0DB0 members from +132; +456 / +460 and the u8s
// +464..+466 / +468 = 0, u8 +467 = the u8 0x01085E98; then
// StateMachine_SetState(o, 0, 0) (r3 = its; f1 as the original leaves it).
DECOMP(003B04F8, Machine_Construct_003B04F8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), x = arg<u32>(ctx, 3);
    call(ctx, 0x0047C5D8u, {o, a, b, 0, x});
    constexpr u32 V = 0x01164B40u;
    store<u32>(o + 56, x);
    store<u32>(o + 80, load<u32>(V));
    store<u32>(o, 0x00B20338u);
    store<u32>(o + 60, 0);
    store<u32>(o + 64, 0);
    store<u32>(o + 68, 0);
    store<u32>(o + 84, load<u32>(V + 4));
    store<u32>(o + 96, 0);
    store<u32>(o + 88, load<u32>(V + 8));
    store<u32>(o + 100, 0);
    store<u32>(o + 104, 0);
    store<u32>(o + 108, 0);
    ctx.fpr[1] = Lfs32(V + 8);  // f1 as the original leaves it
    call(ctx, 0x005516E0u, {o + 112});
    store<u16>(o + 118, 8);
    call(ctx, 0x00551598u, {o + 112, 32, 0, 1});
    for (u32 i = 0; i < 9; ++i) call(ctx, 0x004A0DB0u, {o + 132 + 36 * i});
    store<u8>(o + 464, 0);
    store<u8>(o + 465, 0);
    store<u8>(o + 466, 0);
    store<u32>(o + 456, 0);
    store<u32>(o + 460, 0);
    const u8 k = load<u8>(0x01085E98u);
    store<u8>(o + 468, 0);
    store<u8>(o + 467, k);
    call(ctx, 0x00464D1Cu, {o, 0, 0});
}

// Item* f(Item* o, Src* s) (from 0x00455728): clears o (+8, +16..+24,
// +32..+36, +48..+60), +0 = s->+0, u8 +28 = 1, +40 = s->+8; then three
// values of s->+0's field table (+12) at the indices (*0x010ABD38)->+64,
// (*0x010ABD80)->+64 and (*0x010ABD50)->+64 classify it: +4 = 1 for
// 0x13234CFF, 2 for 0xFD81, else 0, and 2 whenever the second is 0; +44 =
// 1 for 0x74BC7BAD, 2 for 0x95E51416, else 0. r3 = o.
DECOMP(004555F4, Item_InitFromSrc_004555F4) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    store<u32>(o + 48, 0);
    store<u32>(o + 52, 0);
    store<u32>(o + 56, 0);
    store<u32>(o + 60, 0);
    const u32 d = load<u32>(s);
    store<u32>(o + 8, 0);
    store<u32>(o, d);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    const u32 s8 = load<u32>(s + 8);
    store<u32>(o + 24, 0);
    store<u8>(o + 28, 1);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, s8);
    const u32 tab = load<u32>(d + 12);
    auto field = [&](u32 g) { return load<u32>(tab + (load<u32>(load<u32>(g + 16) + 64) << 2)); };
    const u32 v0 = field(0x010ABD28u);
    store<u32>(o + 4, v0 == 0x13234CFFu ? 1u : v0 == 0xFD81u ? 2u : 0u);
    if (field(0x010ABD70u) == 0) store<u32>(o + 4, 2);
    store<u32>(o + 32, 0);
    const u32 v2 = field(0x010ABD40u);
    store<u32>(o + 44, v2 == 0x74BC7BADu ? 1u : v2 == 0x95E51416u ? 2u : 0u);
}

// u32 f(Stream* o, Src* x) (no direct caller): u8 +4 = 1; with the
// scheduler s = Fios_GetScheduler5FC90C() and its +352 (both set): the
// media m = Fios_New(1072, 0x10000100, 3) built by Ps3Media_Construct(m,
// x) when not 0 goes to +12; the stream r = 0x005FC1BC(s->+352,
// 0x00AE9A10, 0, m, 0x00AE9A10, 0, o + 8); when r is not 0,
// 0x004911C0(r), then the media +12 (when set) gets vt[1](m) (OPD / entry
// after the arguments) and +12 = 0. r3 as the original leaves it.
DECOMP(0049146C, Stream_Open_0049146C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    store<u8>(o + 4, 1);
    const u32 s = static_cast<u32>(call(ctx, 0x005FC90Cu, {1}));
    if (s == 0 || load<u32>(s + 352) == 0) {
        ret(ctx, s);
        return;
    }
    const u32 q = load<u32>(s + 352);
    u32 m = static_cast<u32>(call(ctx, 0x005E7E1Cu, {1072, 0x10000100u, 3}));
    if (m != 0) call(ctx, 0x00606834u, {m, x});
    store<u32>(o + 12, m);
    const u32 r = static_cast<u32>(call(ctx, 0x005FC1BCu, {q, 0x00AE9A10u, 0, m, 0x00AE9A10u, 0, o + 8}));
    if (r == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004911C0u, {r});
    m = load<u32>(o + 12);
    if (m == 0) return;
    VCall(ctx, m, 4, {m});
    store<u32>(o + 12, 0);
}

// T* f(T* o) (no direct caller: a clone): p = SmallAlloc(96); when not 0,
// u16 +4 = 1, vtable 0x00B21E68 and the 3x3 rows +32 / +48 / +64 / +80
// (12 bytes each) from the identity 0x01164BD0; then the rows of o are
// copied over (to address 0 + 32... when p is 0: a quirk) and +8 = o->+8.
// r3 = p (f1 as the original leaves it).
DECOMP(004BF6A4, Xform_Clone_004BF6A4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {96}));
    constexpr u32 offs[12] = {32, 36, 40, 48, 52, 56, 64, 68, 72, 80, 84, 88};
    if (p != 0) {
        constexpr u32 I = 0x01164BD0u;
        store<u16>(p + 4, 1);
        store<u32>(p, 0x00B21E68u);
        for (u32 k : offs) store<u32>(p + k, load<u32>(I + k - 32));
    }
    u32 w[12];
    for (u32 i = 0; i < 12; ++i) w[i] = load<u32>(o + offs[i]);
    for (u32 i = 0; i < 12; ++i) store<u32>(p + offs[i], w[i]);
    store<u32>(p + 8, load<u32>(o + 8));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w[0]));  // f1 as the original leaves it
    ret(ctx, p);
}

// T* f(-, XmlCursor* x) (no direct caller: a factory): p = Mem_Alloc(24,
// 0, 16); when not 0: u16 +4 = 1, vtable 0x00B22150, +12 / +16 = 0, bit 7
// of the u8 +20 cleared; from x: +12 = the hash 0x8A93973F (when present
// also read as the s32 0x5C8998CD), +16 = the hash 0x000107E5, bit 7 of
// +20 = the bool 0x6F82BD88 (default false). r3 = p.
DECOMP(004CA534, XmlNode_New_004CA534) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 b = load<u8>(p + 20) & ~0x80u;
    store<u16>(p + 4, 1);
    store<u32>(p, 0x00B22150u);
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, static_cast<u8>(b));
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0})) != 0)
        call(ctx, 0x004F99BCu, {x, 0x5C8998CDu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x000107E5u, p + 16, 0});
    {
        Stack fr(ctx, 16);
        store<u8>(fr.addr(), 0);
        call(ctx, 0x004F986Cu, {x, 0x6F82BD88u, fr.addr(), 0});
        store<u8>(p + 20, static_cast<u8>((load<u8>(p + 20) & ~0x80u) | ((static_cast<u32>(load<u8>(fr.addr())) << 7) & 0x80u)));
    }
    ret(ctx, p);
}

// void f(Pool* out, u32 n, u32 extra, Allocator* a) (from 0x003A0010): the
// pool setup PoolInit38 with 368-byte elements.
DECOMP(0077A9AC, Pool_Init368_0077A9AC) { PoolInit38(ctx, 368); }

// void f(Pool* out, u32 n, u32 extra, Allocator* a) (from 0x003A0010): the
// pool setup PoolInit38 with 336-byte elements.
DECOMP(0077AAE0, Pool_Init336_0077AAE0) { PoolInit38(ctx, 336); }

// ---------------------------------------------------------------- sweep: 78 instructions
// u32 f(T* o) (no direct caller: a vtable slot): in state 1 (+4) the
// animation +32 is restarted, in state 4 (when Anim_TrackSet36_0029879C(
// o->+64, 0, 1) is not 0) the animation 0x922FF61A: Anim_Restart2775A8(
// o->+64, anim, 0, 0, -1, -, 0, 0, -, -, 0, -1; the floats 0x00025588 [0],
// [1], [2], [2]), and when that started +32 = anim. r3 = 0.
DECOMP(00025598, Obj_RestartAnim_00025598) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = load<s32>(o + 4);
    u32 anim;
    if (s == 1) {
        anim = load<u32>(o + 32);
    } else if (s == 4) {
        if (static_cast<u32>(call(ctx, 0x0029879Cu, {load<u32>(o + 64), 0, 1})) == 0) {
            ret(ctx, 0u);
            return;
        }
        anim = 0x922FF61Au;
    } else {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 K = 0x00025588u;  // {1.0f, 0.25f, 0.0f}
    const u64 r8 = ctx.gpr[8];      // never set by the original; the callee doesn't read it
    const f64 c = Lfs32(K + 8);
    // Arguments 9 / 10 (r1 +112 / +120) are never written (float slots).
    callf(ctx, 0x002775A8u, {load<u32>(o + 64), anim, 0, 0, ~0ull, r8, 0, 0, 0, 0, 0, ~0ull}, {Lfs32(K), Lfs32(K + 4), c, c});
    if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, anim);
    ret(ctx, 0u);
}

// u32 f(Follower* o) (from 0x00034598, 0x00039670): with the target t =
// +256 and the larger m (fsel) of the two f32 fields of the tuning
// o->+308->+12 at the indices (*0x00F94EE4)->+64 and (*0x00F94E84)->+64:
// r3 = 0 without t or when the f32 +288 is not below m; 1 when the u8
// field (*0x00F9504C)->+64 is 0; otherwise the line of sight
// Follow_LineOfSight138B0(o, from, to, t) between the positions
// o->+48->vt[20](out, o->+48) and t->vt[20](out, t) (stack vectors; OPD /
// entry after the arguments). f1 = m.
DECOMP(00034444, Follow_CanSee_00034444) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 tab = load<u32>(load<u32>(o + 308) + 12);
    auto idx = [](u32 g) { return load<u32>(load<u32>(g + 16) + 64) << 2; };
    const f64 a = Lfs32(tab + idx(0x00F94ED4u)), b = Lfs32(tab + idx(0x00F94E74u));
    const u32 t = load<u32>(o + 256);
    const f64 m = Fsel38(F38(a - b), a, b);
    ctx.fpr[1] = m;
    if (t == 0 || !(Lfs32(o + 288) < m)) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(tab + idx(0x00F9503Cu)) == 0) {
        ret(ctx, 1u);
        return;
    }
    Stack fr(ctx, 32);
    const u32 from = fr.addr(), to = fr.addr() + 16;
    const u32 h = load<u32>(o + 48);
    VCall(ctx, h, 80, {from, h});
    VCall(ctx, t, 80, {to, t});
    call(ctx, 0x000138B0u, {o, from, to, t});
}

// T* f(-, XmlCursor* x) (no direct caller: a factory): p = Mem_Alloc(32, 0,
// 16); when not 0: Obj_InitVt_B21C98(p, x), +12..+24 = 0, vtable
// 0x00B0F4A0, u8 +28 = 0; from x the hashes 0x00014B07 -> +12 (default
// 0), 0x7A30D7E3 -> +16, 0xCCEB9AF9 -> +20, 0x52A72CDD -> +24 and the
// bool 0x7A281983 -> +28 (defaulting to the current values). r3 = p.
DECOMP(0007283C, XmlNode_New_0007283C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    store<u32>(p, 0x00B0F4A0u);
    store<u8>(p + 28, 0);
    call(ctx, 0x004FA14Cu, {x, 0x00014B07u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {x, 0xCCEB9AF9u, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004FA14Cu, {x, 0x52A72CDDu, p + 24, load<u32>(p + 24)});
    call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 28, load<u8>(p + 28)});
    ret(ctx, p);
}

// u32 f(Prompt* o, Effect* e) (no direct caller): with the component c =
// Effect_FindComponent(e, +48) (when +48 is set) whose vt[8](c) is 2 (OPD /
// entry after the arguments), the text +12 (32 bytes) = "%d" of
// c->vt[6](c); the object h = Handle_ResolveBit1(+52), when set, gets
// Obj_SetField180_003676DC(h, +56) (when +56 is set),
// Prompt_Fade367758(h; f1 = the f32 +60) and Obj_SetText367530(h, o + 12,
// +64) (r3 = its; 0 without h).
DECOMP(0007D540, Prompt_Update_0007D540) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    if (const u32 k = load<u32>(o + 48); k != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, k}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 2) {
            const s32 v = static_cast<s32>(VCall(ctx, c, 24, {c}));
            const u32 s = static_cast<u32>(call(ctx, o38fn::String_Format, {0x00AC4DD8u, static_cast<u64>(static_cast<s64>(v))}));
            call(ctx, 0x00553684u, {o + 12, s, 32});
        }
    }
    const u32 h = static_cast<u32>(call(ctx, 0x003568E0u, {load<u32>(o + 52)}));
    if (h == 0) {
        ret(ctx, 0u);
        return;
    }
    if (const u32 v = load<u32>(o + 56); v != 0) call(ctx, 0x003676DCu, {h, v});
    callf(ctx, 0x00367758u, {h}, {Lfs32(o + 60)});
    call(ctx, 0x00367530u, {h, o + 12, load<u32>(o + 64)});
}

// T* f(-, XmlCursor* x) (no direct caller: a factory): p = Mem_Alloc(28, 0,
// 16); when not 0: Obj_InitVt_B21C98(p, x), +12 = 0, vtable 0x00B10148,
// f32 +20 = 1.0, u8 +24 = 0; from x the hash 0x8A93973F -> +12 (default
// 0), and only when that is absent the f32 0x03B7B93E -> +20 (default the
// f32 0x00086B14, 1.0), the hash 0xA11F2109 -> +16 and the bool 0xF5897C08
// -> +24 (defaults 0). r3 = p.
DECOMP(00086B18, XmlNode_New_00086B18) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p, 0x00B10148u);
    const f64 k = Lfs32(0x00086B14u);
    store<u32>(p + 20, 0x3F800000u);
    store<u8>(p + 24, 0);
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0})) == 0) {
        callf(ctx, 0x004F9B20u, {x, 0x03B7B93Eu, p + 20}, {k});
        call(ctx, 0x004FA14Cu, {x, 0xA11F2109u, p + 16, 0});
        call(ctx, 0x004F986Cu, {x, 0xF5897C08u, p + 24, 0});
    }
    ret(ctx, p);
}

// T* f(-, XmlCursor* x) (no direct caller: a factory): p = Mem_Alloc(24, 0,
// 16); when not 0: Obj_InitVt_B21C98(p, x), +12 / +16 and the u8s
// +20..+22 = 0, vtable 0x00B10288; from x the hashes 0x8A93973F -> +12 and
// 0x7A30D7E3 -> +16 and the bools 0x3A5ABCBE -> +20, 0x706ED613 -> +21 and
// 0x7A281983 -> +22 (defaults 0). r3 = p.
DECOMP(00087CFC, XmlNode_New_00087CFC) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 0);
    store<u8>(p + 21, 0);
    store<u8>(p + 22, 0);
    store<u32>(p, 0x00B10288u);
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, 0});
    call(ctx, 0x004F986Cu, {x, 0x3A5ABCBEu, p + 20, 0});
    call(ctx, 0x004F986Cu, {x, 0x706ED613u, p + 21, 0});
    call(ctx, 0x004F986Cu, {x, 0x7A281983u, p + 22, 0});
    ret(ctx, p);
}

// u32 f(u32* out, a, b, c, d, u8 e, u8 g, h, i) (from many callers): a
// stack query q {vector 0x01164B40 at +0 and +16, f32 +32 = 1.0, +36 = 0,
// +40 = h, u8s +44..+47 = 0, +48 / +56 / +60 / +64 = 0} goes to
// 0x000DB348(result, a, b, q, c, d, e, g, i) (the 9th argument passed
// on), then 0x00440AC4(q, 0) and 0x00440A40(q, 0); *out = r3 = the
// result's first word (f1 as the original leaves it).
DECOMP(000DB94C, Query_Run_000DB94C) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 d = arg<u32>(ctx, 4), e = arg<u32>(ctx, 5) & 0xFFu, g = arg<u32>(ctx, 6) & 0xFFu, h = arg<u32>(ctx, 7);
    const u32 i = StkArg34(ctx, 116);  // the 9th argument
    constexpr u32 V = 0x01164B40u;
    Stack fr(ctx, 96);  // query +0..+67, result +80 (the original's 128..223)
    const u32 q = fr.addr(), res = fr.addr() + 80;
    for (u32 k = 0; k < 12; k += 4) {
        store<u32>(q + k, load<u32>(V + k));
        store<u32>(q + 16 + k, load<u32>(V + k));
    }
    store<u32>(q + 32, 0x3F800000u);
    store<u32>(q + 36, 0);
    store<u32>(q + 40, h);
    for (u32 k = 44; k < 48; ++k) store<u8>(q + k, 0);
    store<u32>(q + 48, 0);
    store<u32>(q + 56, 0);
    store<u32>(q + 60, 0);
    store<u32>(q + 64, 0);
    ctx.fpr[1] = Lfs32(V);  // f1 as the original leaves it
    call(ctx, 0x000DB348u, {res, a, b, q, c, d, e, g, i});
    call(ctx, 0x00440AC4u, {q, 0});
    call(ctx, 0x00440A40u, {q, 0});
    const u32 r = load<u32>(res);
    store<u32>(out, r);
    ret(ctx, r);
}

// u32 f(T* o, const char* name, -, Owner* x) (from Obj_SetAttr_000FC88C):
// for x, unless the name hashes (String_HashNoCase) to 0x5294E198,
// 0x1B12498F or 0xD49B5F41: when it starts with "hkAttach" (Mem_Compare of
// 8 bytes), the 240-byte entry of x->+336's array +340 (data +0, u16 count
// +8) whose +192 is o gets +160 = the hash of the rest of the name. r3 as
// the original leaves it.
DECOMP(000DD368, Attach_SetName_000DD368) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), x = arg<u32>(ctx, 3);
    if (x == 0) return;
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {name}));
    if (h == 0x5294E198u || h == 0x1B12498Fu || h == 0xD49B5F41u) return;
    if (static_cast<u32>(call(ctx, 0x00555DB8u, {name, 0x00AC6BB4u, 8})) != 0) return;
    const u32 h2 = static_cast<u32>(call(ctx, 0x0056A710u, {name + 8}));
    const u32 a = load<u32>(x + 336);
    if (a == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 arr = load<u32>(a + 340);
    for (u32 i = 0; i < (arr != 0 ? static_cast<u32>(load<u16>(arr + 8)) : 0u); ++i) {
        const u32 e = load<u32>(arr) + 240 * i;
        if (load<u32>(e + 192) != o) continue;
        if (e != 0) store<u32>(e + 160, h2);
        break;
    }
    ret(ctx, arr);
}

// u32 f(T* o) (no direct caller: a vtable slot): Nop_000D8FC0(o); the blink
// flag (u8 +761) stays on while cos(4 pi * the f32 +764) (Math_Cosf; the
// products single precision, k = 0x00175A10 {0, pi, 2}) is not below 0,
// otherwise +764 = 0 (the word); when the f32 +348 is above 0, the state
// +396 is not 3, the flag is on and bit 8 of +172 is set:
// Obj_ApplyTree_000D8F08(o, o + 768, 0), u8 +760 = 1, o->vt[22](o, 0, 1,
// 1, 0) (OPD / entry after the arguments). r3 / f1 as the original leaves
// them.
DECOMP(00175A20, Obj_BlinkUpdate_00175A20) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000D8FC0u, {o});
    constexpr u32 K = 0x00175A10u;
    bool on = load<u8>(o + 761) != 0;
    if (on) {
        f64 t = F38(Lfs32(o + 764) * Lfs32(K + 4));
        t = F38(t * Lfs32(K + 8));
        t = F38(t * Lfs32(K + 8));
        if (callf(ctx, 0x007344E0u, {}, {t}) < Lfs32(K)) on = false;
    } else {
        store<u32>(o + 764, 0);
    }
    const f64 v = Lfs32(o + 348);
    ctx.fpr[1] = v;
    if (!(v > Lfs32(K))) {
        ret(ctx, K);
        return;
    }
    const u32 st = load<u32>(o + 396);
    if (st == 3 || !on) {
        ret(ctx, st);
        return;
    }
    if (((load<u32>(o + 172) >> 8) & 1u) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000D8F08u, {o, o + 768, 0});
    store<u8>(o + 760, static_cast<u8>(((load<u32>(o + 172) >> 8) & 1u) != 0 ? 1 : 0));
    VCall(ctx, o, 88, {o, 0, 1, 1, 0});
}

// u32 f(T* o, s32 s) (from 0x001774D8): the physics state +392 = s when it
// changes; 1: o->vt[22](o, 1, 1, 1, 1) (OPD / entry after the arguments)
// and, for the body h = +388 not yet in the world (h->+8 is 0), the
// collision layer of h->+44 (bits 16..31 = +408, keeping bits 0..4) and
// Hk_AddEntity92F2A8((*0x010B19E0 + 0x1281D4)->+4, h, 1), then +396 = 0;
// 3: +396 = 0; 4: Hk_RemoveEntity92FFC8(stack result, h->+8, h) when h is
// in a world. r3 as the original leaves it.
DECOMP(0017734C, Phys_SetState_0017734C) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = arg<s32>(ctx, 1);
    u32 r3 = load<u32>(o + 392);
    if (static_cast<s32>(r3) == s) {
        ret(ctx, r3);
        return;
    }
    if (s == 1) {
        VCall(ctx, o, 88, {o, 1, 1, 1, 1});
        if (const u32 h = load<u32>(o + 388); h != 0) {
            const u32 g = load<u32>(0x010B19E0u);
            const u32 w = (load<u32>(h + 44) & 0x1Fu) | (load<u32>(o + 408) << 16);
            const u32 in = load<u32>(h + 8);
            store<u32>(h + 44, w);
            if (in == 0) call(ctx, 0x0092F2A8u, {load<u32>(load<u32>(g + 0x001281D4u) + 4), h, 1});
        }
        store<u32>(o + 396, 0);
        r3 = 0;
    } else if (s == 3) {
        store<u32>(o + 396, 0);
        r3 = 0;
    } else if (s == 4) {
        const u32 h = load<u32>(o + 388);
        r3 = h;
        if (h != 0 && load<u32>(h + 8) != 0) {
            Stack fr(ctx, 16);
            r3 = static_cast<u32>(call(ctx, 0x0092FFC8u, {fr.addr(), load<u32>(h + 8), h}));
        }
    }
    store<u32>(o + 392, static_cast<u32>(s));
    ret(ctx, r3);
}

// void f(T* o) (from its deleting / adjusting thunks 0x001ED72C ...):
// vtables 0x00B186C8 / +380 0x00B18874 / +384 0x00B1889C; the pointer
// vectors +928 and +640 cleared (PtrVector_Clear, Nop) and the members
// +876 (VtB21B58_DtorTrivial_004A0CA4) / +544 (VtB21B58_DtorTrivial_
// 004A0E08) destroyed; the owned 20-byte vector +388 (when set: vtables
// 0x00B12898 / 0x00B12A44 / 0x00B12A6C first) cleared and SmallFree'd;
// vtables 0x00B126B8 / 0x00B1200C / 0x00B12528, VtB12550_Dtor_000DF934(o)
// (r3 = its).
DECOMP(001ED5F4, Dtor_B186C0_001ED5F4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B186C8u);
    store<u32>(o + 380, 0x00B18874u);
    store<u32>(o + 384, 0x00B1889Cu);
    call(ctx, 0x0054EC1Cu, {o + 928});
    call(ctx, 0x0054EC18u, {o + 928});
    call(ctx, 0x004A0CA4u, {o + 876});
    call(ctx, 0x0054EC1Cu, {o + 640});
    call(ctx, 0x0054EC18u, {o + 640});
    call(ctx, 0x004A0E08u, {o + 544});
    if (const u32 v = load<u32>(o + 388); v != 0) {
        store<u32>(o + 384, 0x00B12A6Cu);
        store<u32>(o + 380, 0x00B12A44u);
        store<u32>(o, 0x00B12898u);
        call(ctx, 0x0054EC1Cu, {v});
        call(ctx, 0x0054EC18u, {v});
        call(ctx, 0x00552420u, {load<u32>(o + 388), 20});
    }
    store<u32>(o + 384, 0x00B12528u);
    store<u32>(o + 380, 0x00B1200Cu);
    store<u32>(o, 0x00B126B8u);
    call(ctx, 0x000DF934u, {o});
}

// u32 f(T* o) (from 0x002C7340): each 12-byte link of the array +356 (u16
// count +364, re-read): when +0 is set, x = CallK2_000D9614(+0) gets
// x->vt[11](x, 1) (OPD / entry after the arguments) when x and the u8 +8
// are set; then +0->vt[10](+0, 0) and +4->vt[2](+4), each cleared (when
// set). Then the count = 0. r3 = the count.
DECOMP(002C6848, Links_Release_002C6848) {
    const u32 o = arg<u32>(ctx, 0);
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(o + 364)); ++i) {
        const u32 e = load<u32>(o + 356) + 12 * i;
        if (const u32 a = load<u32>(e); a != 0) {
            const u32 x = static_cast<u32>(call(ctx, 0x000D9614u, {a}));
            if (x != 0 && load<u8>(e + 8) != 0) VCall(ctx, x, 44, {x, 1});
        }
        if (const u32 a = load<u32>(e); a != 0) {
            VCall(ctx, a, 40, {a, 0});
            store<u32>(e, 0);
        }
        if (const u32 b = load<u32>(e + 4); b != 0) {
            VCall(ctx, b, 8, {b});
            store<u32>(e + 4, 0);
        }
    }
    store<u16>(o + 364, 0);
    ret(ctx, n);
}

// u32 f(Mgr* o, s32 k) (from several callers): for the 536-byte session s =
// o->+28 + 536 k (not 0): Session_Stop2DDAB0(o), Set_Stop4BD1CC on
// Set_Find(0x7719A548) when found, +40 / +72 / +48 = 0, UserBlock_Reset
// 45C7A8 on each of the u8 (*0x00F65CA0)->+29 user blocks (world + 4208,
// 0x20C80 apart), +36 = 0; r = 0x002D9CC0(s), and when not 0:
// Vec_MoveAll_002DA04C(s, world + 4208), +40 = s, +36 = +20 + 860 * u8
// s->+500. r3 = r (0 without s).
DECOMP(002E1F44, Session_Start_002E1F44) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 28) + static_cast<u32>(arg<s32>(ctx, 1)) * 536u;
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x002DDAB0u, {o});
    if (const u32 t = static_cast<u32>(call(ctx, 0x004BCE90u, {0x7719A548u})); t != 0) call(ctx, 0x004BD1CCu, {t});
    store<u32>(o + 40, 0);
    store<u32>(o + 72, 0);
    store<u32>(o + 48, 0);
    for (u32 i = 0;; ++i) {
        const u32 w = load<u32>(0x00F65CA0u);
        if (i >= load<u8>(w + 29)) break;
        call(ctx, 0x0045C7A8u, {w + 0x20C80u * i + 4208});
    }
    store<u32>(o + 36, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x002D9CC0u, {s}));
    if (r != 0) {
        call(ctx, 0x002DA04Cu, {s, load<u32>(0x00F65CA0u) + 4208});
        store<u32>(o + 40, s);
        store<u32>(o + 36, load<u32>(o + 20) + load<u8>(s + 500) * 860u);
    }
    ret(ctx, r);
}

// u32 f(T* o, XmlCursor* x) (no direct caller: a loader): +4 = the s32
// 0xF95DFD73 (default 0, through a stack copy), then the f32s 0x94A773B8
// -> +48, 0x7B738C62 -> +52, 0xC687E664 -> +56, 0xC3C5F05B -> +60,
// 0x85AEB11C -> +64 and 0x8BB08F02 -> +68 (each defaulting to the current
// value). r3 = the last read's.
DECOMP(00302FB0, Tuning_Load_00302FB0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    {
        Stack fr(ctx, 16);
        store<u32>(fr.addr(), 0);
        call(ctx, 0x004F99BCu, {x, 0xF95DFD73u, fr.addr(), 0});
        store<u32>(o + 4, load<u32>(fr.addr()));
    }
    constexpr u32 keys[6] = {0x94A773B8u, 0x7B738C62u, 0xC687E664u, 0xC3C5F05Bu, 0x85AEB11Cu, 0x8BB08F02u};
    for (u32 i = 0; i < 6; ++i) callf(ctx, 0x004F9B20u, {x, keys[i], o + 48 + 4 * i}, {Lfs32(o + 48 + 4 * i)});
}

// u32 f(T* o, u32 key) (from Obj_OnEvent_003123AC): in the tree of the
// effect set e = o->+40->+0->+24 (root (+12)->+0, nodes {-, key +4,
// value +8, left +12 for larger keys, right +16}) key is looked up, the
// path of visited nodes pushed on e's s16-counted stack (+16, count +96,
// reset first); found: e->+100 = the node, *0x011DE2A8 = its value, and
// when e->+100->+8 is set, Effect_SetHash4CF988(o->+40, key); not found:
// e->+100 = the last node visited; an empty tree clears e->+100 and
// *0x011DE2A8. r3 as the original leaves it.
DECOMP(00312274, Effect_SelectKey_00312274) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 a = load<u32>(o + 40);
    if (a == 0) return;
    const u32 b = load<u32>(a);
    if (b == 0) return;
    const u32 e = load<u32>(b + 24);
    if (e == 0) return;
    u32 n = load<u32>(load<u32>(e + 12));
    if (n == 0) {
        store<u32>(e + 100, 0);
        store<u32>(0x011DE2A8u, 0);
        ret(ctx, 0x011E0000u);
        return;
    }
    store<u16>(e + 96, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) break;
        const u32 c = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
        if (c == 0) {
            store<u32>(e + 100, n);  // (the original's null-node branch is unreachable)
            ret(ctx, n);
            return;
        }
        const s32 d = static_cast<s16>(load<u16>(e + 96));
        store<u16>(e + 96, static_cast<u16>(d + 1));
        store<u32>(e + 16 + 4 * static_cast<u32>(d), n);
        n = c;
    }
    store<u32>(e + 100, n);
    store<u32>(0x011DE2A8u, load<u32>(n + 8));
    u32 r3 = load<u32>(b + 24);
    if (r3 != 0) r3 = load<u32>(r3 + 100);
    if (r3 != 0) r3 = load<u32>(r3 + 8);
    if (r3 == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004CF988u, {load<u32>(o + 40), key});
}

// void f(T* o) (no direct caller: a deleting destructor): for o != 0, the
// destructor body DtorB1F678_36, then Mem_FreeFrom(o, 0) (r3 = its).
DECOMP(0036A1C0, Delete_B1F678_0036A1C0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    DtorB1F678_36(ctx);
    call(ctx, 0x0054EE6Cu, {o, 0});
}

// u32 f(T* o) (from World_SubmitMain): for each 368-byte entry e of +44
// (count u16 (+36)->+2, re-read) with bit 31 of +356 set: its three
// 76-byte parts p (from e + 80) each go to 0x0039F840(e + 60 (or e + 40
// when the u16 e->+68 is 0), e + 308, p). r3 = the count.
DECOMP(003A13C8, Entries_Submit_003A13C8) {
    const u32 o = arg<u32>(ctx, 0);
    u32 n = load<u16>(load<u32>(o + 36) + 2);
    for (u32 i = 0; i < n; ++i, n = load<u16>(load<u32>(o + 36) + 2)) {
        const u32 e = load<u32>(o + 44) + 368 * i;
        if (e == 0 || (load<u32>(e + 356) & 0x80000000u) == 0) continue;
        for (u32 j = 0; j < 3; ++j) call(ctx, 0x0039F840u, {load<u16>(e + 68) != 0 ? e + 60 : e + 40, e + 308, e + 80 + 76 * j});
    }
    ret(ctx, n);
}

// u32 f(Owner* a, HkEntity* b) (no direct caller): HkRemoveEntity38 with
// the map a + 72 and the calls on a + 56 / a + 60.
DECOMP(003A9C44, HkOwner_RemoveEntity_003A9C44) { HkRemoveEntity38(ctx, 72, 56, 60); }

// u32 f(T* o) (from 0x003AD270, 0x003AECB0): the direction d = 0..3 of
// the f32 pair x = +136, y = +140 (|x| > |y| and |x| >= the f32
// 0x010869D8: 1 / 0 for x >= / < 0; otherwise 2 / 3 for y >= / < 0); the
// clip hashes String_HashNoCase("Clip_%s_%s" of o + 28 and the name
// 0x00ADF200[d]) and of "%s_%s" of o + 60 and it go to 0x003AB0B8(o, h1,
// h2, 0, -, 1; f1 = 1.0) (r3 = its).
DECOMP(003AB560, Dir_PlayClip_003AB560) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x003AB558u;  // {0.0f, 1.0f}
    const f64 x = Lfs32(o + 136), y = Lfs32(o + 140);
    u32 d;
    if (std::fabs(x) > std::fabs(y) && !(std::fabs(x) < Lfs32(0x010869D8u))) d = x < Lfs32(K) ? 0u : 1u;
    else d = y < Lfs32(K) ? 3u : 2u;
    const u32 name = load<u32>(0x00ADF200u + 4 * d);
    u32 s = static_cast<u32>(call(ctx, o38fn::String_Format, {0x00ADF148u, o + 28, name}));
    const u32 h1 = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
    s = static_cast<u32>(call(ctx, o38fn::String_Format, {0x00ADF158u, o + 60, load<u32>(0x00ADF200u + 4 * d)}));
    const u32 h2 = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
    callf(ctx, 0x003AB0B8u, {o, h1, h2, 0, K, 1}, {Lfs32(K + 4)});
}

// u32 f(T* o, a, Def* d, b) (from NewAttach_00331590): SubMachine_Construct(
// o, a, b, d->+4, r7 as passed in), vtable 0x00B20428, the pointer vector
// +56 (PtrVector_Construct, u16 +62 = 64, PtrVector_Init(v, 32, 0, 0)),
// +80..+88 = the vector 0x01164B40, +96 = d, +100..+132 / +140 / +144 /
// +152 = 0, +136 = -1, bit 31 of +148 and the u8 +149 cleared, +160..+172
// = the four f32s 0x01164C80, then bit 4 of +28 set. r3 = o + 28 (f1 as the
// original leaves it).
DECOMP(003B3148, Machine_Construct_003B3148) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    // r7 is not set by the original: SubMachine_Construct reads the r7 this
    // function was called with.
    call(ctx, 0x0047C5D8u, {o, a, b, load<u32>(d + 4), ctx.gpr[7]});
    store<u32>(o, 0x00B20428u);
    call(ctx, 0x005516E0u, {o + 56});
    store<u16>(o + 62, 64);
    call(ctx, 0x00551598u, {o + 56, 32, 0, 0});
    constexpr u32 V = 0x01164B40u, W = 0x01164C80u;
    store<u32>(o + 80, load<u32>(V));
    store<u32>(o + 84, load<u32>(V + 4));
    store<u32>(o + 88, load<u32>(V + 8));
    const u32 w148 = load<u32>(o + 148) & 0x7FFFFFFFu;
    store<u32>(o + 140, 0);
    store<u32>(o + 96, d);
    store<u32>(o + 148, w148);
    store<u32>(o + 152, 0);
    for (u32 k = 100; k <= 124; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 160, load<u32>(W));
    store<u32>(o + 128, 0);
    store<u32>(o + 132, 0);
    store<u32>(o + 136, 0xFFFFFFFFu);
    store<u32>(o + 144, 0);
    store<u32>(o + 164, load<u32>(W + 4));
    store<u8>(o + 149, 0);
    store<u32>(o + 168, load<u32>(W + 8));
    store<u32>(o + 172, load<u32>(W + 12));
    store<u32>(o + 28, load<u32>(o + 28) | 0x10u);
    ctx.fpr[1] = Lfs32(W + 12);  // f1 as the original leaves it
    ret(ctx, o + 28);
}

// u32 f(T* o) (no direct caller): ObjClose38 with the table 0x00F386A8,
// the hashes 0x63D3E645 / 0x7FAD07CE and the view +92.
DECOMP(00405358, Obj_Close_00405358) { ObjClose38(ctx, 0x00F386A8u, 0x63D3E645u, 0x7FAD07CEu, 92, 0x00405354u); }

// u32 f(T* o) (no direct caller): ObjClose38 with the table 0x00F38840,
// the hashes 0x251B46E4 / 0x20EB352D and the view +468.
DECOMP(0041F0C0, Obj_Close_0041F0C0) { ObjClose38(ctx, 0x00F38840u, 0x251B46E4u, 0x20EB352Du, 468, 0x0041F0BCu); }

// s32 f(u32 handle) (from 0x003D75A8): while the u8 *0x010AEC60 is set,
// the handle (index << 16 | serial) names the 272-byte voice entry e of
// the table 0x010B0200 (count +24, data +28; e's word +256 holds the
// serial in bits 15..30, valid while its top bit is clear); then for i <
// Voice_GetValue(e->+16): when 0x006F9F1C(0x006D6514(e->+16, i), &a, &b,
// &c) is not 0, r3 = b (stack outputs). 0 otherwise.
DECOMP(0044C9D0, Voice_FindParam_0044C9D0) {
    const u32 h = arg<u32>(ctx, 0);
    if (load<u8>(0x010AEC60u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 serial = h & 0xFFFFu, idx = h >> 16;
    constexpr u32 T = 0x010B0200u;
    if (serial == 0 || static_cast<s32>(idx) >= load<s32>(T + 24)) {
        ret(ctx, 0u);
        return;
    }
    const u32 base = load<u32>(T + 28);
    const u32 e = base + idx * 272u;
    if (((load<u32>(e + 256) >> 15) & 0xFFFFu) != serial || (load<u8>(e + 256) & 0x80u) != 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 v = e + 16;
    const u32 n = static_cast<u32>(call(ctx, 0x006D64ACu, {load<u32>(v)}));
    Stack fr(ctx, 16);
    for (u32 i = 0; i < n; ++i) {
        const u32 x = static_cast<u32>(call(ctx, 0x006D6514u, {load<u32>(v), static_cast<u64>(static_cast<s64>(static_cast<s16>(i)))}));
        if (static_cast<u32>(call(ctx, 0x006F9F1Cu, {x, fr.addr(), fr.addr() + 4, fr.addr() + 8})) != 0) {
            ret(ctx, load<s32>(fr.addr() + 4));
            return;
        }
    }
    ret(ctx, 0u);
}
