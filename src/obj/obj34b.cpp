// Game code, part 34 (continued): the small-function sweep from 68
// instructions. Each was checked against the original in the PPC
// simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj34.h"

using namespace sly4;
using namespace sly4::obj34;

// ---------------------------------------------------------------- sweep: 68 instructions
// void f(T* o): unless o's kind (+4) is 5, 9, 11 or 12, when the player
// p = (*0x00F65CA0 + 4208)->+8 has flag 2 of +192 and its x/z distance
// to o (+176 / +184 vs p's +144 / +152; y term k^2 with k =
// *0x000544B4) is below *0x00F9AFF0, o's s32 +312 counts down (when
// positive) and o's class callback (o->+0)->+220 gets (o, 9). r3 as left;
// f1 = the distance when measured.
DECOMP(000544B8, Obj_NearPlayer_000544B8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 4), cls = load<u32>(o);
    if (k == 5 || k == 9 || k == 12 || k == 11) return;
    const u32 p = load<u32>(load<u32>(0x00F65CA0) + 4208 + 8);
    if (p == 0 || (load<u32>(p + 192) & 2u) == 0) return;
    const f64 c = Lfs32(0x000544B4);
    const f64 dx = static_cast<f32>(Lfs32(p + 144) - Lfs32(o + 176));
    const f64 dz = static_cast<f32>(Lfs32(p + 152) - Lfs32(o + 184));
    const f64 d = static_cast<f32>(std::sqrt(static_cast<f32>(std::fma(dz, dz, static_cast<f32>(std::fma(dx, dx, static_cast<f32>(c * c)))))));
    ctx.fpr[1] = d;
    if (!(d < Lfs32(0x00F9AFF0))) return;
    const s32 t = static_cast<s32>(load<u32>(o + 312));
    if (t > 0) store<u32>(o + 312, static_cast<u32>(t - 1));
    const u32 opd = load<u32>(cls + 220);
    call_ptr(ctx, opd, {o, 9, opd, load<u32>(opd)});
}

// void f(Cmd* c, X* x): the target r (CompTarget34(x, c->+12) when c->+12
// is set, else Obj_FindById(c->+12)) and the record {r, +20, +16, +24,
// +32, +28, +36, +40} on the stack handed to Slots_Add_0042A248. r3 as
// that left it.
DECOMP(000722FC, Cmd_AddSlot_000722FC) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 k = load<u32>(c + 12);
    u32 r = k != 0 ? CompTarget34(ctx, x, k) : 0u;
    if (r == 0) r = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(c + 12)}));
    Stack frame(ctx, 32);
    const u32 s = frame.addr();  // sp+112
    store<u32>(s, r);
    CopyW32(s + 4, c + 20);
    CopyW32(s + 8, c + 16);
    CopyW32(s + 12, c + 24);
    CopyW32(s + 20, c + 28);
    CopyW32(s + 16, c + 32);
    CopyW32(s + 24, c + 36);
    CopyW32(s + 28, c + 40);
    ctx.fpr[1] = Lfs32(c + 40);
    ctx.fpr[2] = Lfs32(c + 36);
    call(ctx, 0x0042A248, {s});
}

// T* New(-, Xml* x): a 28-byte object (Mem_Alloc(28, 0, 16); 0 on
// failure): Obj_InitVt_B21C98(m, x), vtable 0x00B0F6A8, the hashes
// 0x7F247172 / 0x7A30D7E3 / 0x03D1FCFC / 0x04B8CC71 read to +12..+24
// (defaults 0 / current). r3 = m.
DECOMP(0007547C, VtB0F6A8_NewXml_0007547C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {28, 0, 16}));
    if (m != 0) {
        call(ctx, 0x004BB3B0, {m, x});
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, 0x00B0F6A8u);
        store<u32>(m + 24, 0);
        call(ctx, 0x004FA14C, {x, 0x7F247172u, m + 12, 0});
        call(ctx, 0x004FA14C, {x, 0x7A30D7E3u, m + 16, load<u32>(m + 16)});
        call(ctx, 0x004FA14C, {x, 0x03D1FCFCu, m + 20, load<u32>(m + 20)});
        call(ctx, 0x004FA14C, {x, 0x04B8CC71u, m + 24, load<u32>(m + 24)});
    }
    ctx.gpr[3] = m;
}

// void f(Cmd* c, X* x): the target t (CompTarget34(x, c->+12), else
// Obj_FindById(c->+16)), narrowed by the hash +20 (Obj_FindByHash_000DB290
// when set), gets t->vt[88](t, u8 +27, +24, +25, +26). r3 as left.
DECOMP(00075C04, Cmd_ApplyBytes_00075C04) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = CompTarget34(ctx, x, load<u32>(c + 12));
    if (t == 0) {
        t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(c + 16)}));
        ctx.gpr[3] = t;
        if (t == 0) return;
    }
    const u32 h = load<u32>(c + 20);
    const u32 r = h != 0 ? static_cast<u32>(call(ctx, 0x000DB290, {t, h})) : t;
    ctx.gpr[3] = r;
    if (r == 0) return;
    VCall(ctx, r, 88, {r, load<u8>(c + 27), load<u8>(c + 24), load<u8>(c + 25), load<u8>(c + 26)});
}

// T* New(-, Xml* x): NewXmlList34 with vtable 0x00B10350, bool 0x98129697.
DECOMP(00088F38, VtB10350_NewXml_00088F38) { NewXmlList34(ctx, 0x00B10350u, 0x98129697u); }

// void f(Cmd* c, X* x): for the kind-5 component's +28 object t of type
// 8 ((t->+4 >> 10) & 63), its material c->+16 (WorldObj_FindMaterial_
// 004A5A48) parameters p = m->+128 get +8 = c->+20 (u8 +28 set) and +56
// = c->+24 (u8 +29 set); a missing material or parameter block is
// reported (String_Format 0x00AC562C / 0x00AC560C with the name). r3 as
// left.
DECOMP(0009230C, Cmd_SetMaterialParams_0009230C) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 comp = static_cast<u32>(call(ctx, 0x004BB4B4, {x, load<u32>(c + 12)}));
    if (comp == 0) return;
    const s32 kind = static_cast<s32>(VCall(ctx, comp, 32, {comp}));
    if (kind != 5) return;
    const u32 t = load<u32>(comp + 28);
    if (t == 0) return;
    const u32 ty = (load<u32>(t + 4) >> 10) & 0x3Fu;
    ctx.gpr[3] = ty;
    if (ty != 8) return;
    const u32 m = static_cast<u32>(call(ctx, 0x004A5A48, {t, load<u32>(c + 16)}));
    if (m == 0) {
        call(ctx, 0x005534D0, {0x00AC562Cu, load<u32>(c + 16)});
        return;
    }
    const u32 p = load<u32>(m + 128);
    if (p == 0) {
        call(ctx, 0x005534D0, {0x00AC560Cu, load<u32>(c + 16)});
        return;
    }
    ctx.gpr[3] = p;
    if (load<u8>(c + 28) != 0) {
        CopyW32(p + 8, c + 20);
        ctx.fpr[1] = Lfs32(c + 20);
    }
    if (load<u8>(c + 29) != 0) {
        CopyW32(p + 56, c + 24);
        ctx.fpr[1] = Lfs32(c + 24);
    }
}

// T* New(-, Xml* x): a 28-byte object (Mem_Alloc(28, 0, 16); 0 on
// failure): Obj_InitVt_B21C98(m, x), vtable 0x00B10A30, the hashes
// 0x7F247172 / 0x7A30D7E3 / 0x6EBFEF2B to +12..+20 (defaults 0 /
// current) and the bool 0x04B38BFF to u8 +24. r3 = m.
DECOMP(00092680, VtB10A30_NewXml_00092680) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {28, 0, 16}));
    if (m != 0) {
        call(ctx, 0x004BB3B0, {m, x});
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, 0x00B10A30u);
        store<u8>(m + 24, 0);
        call(ctx, 0x004FA14C, {x, 0x7F247172u, m + 12, 0});
        call(ctx, 0x004FA14C, {x, 0x7A30D7E3u, m + 16, load<u32>(m + 16)});
        call(ctx, 0x004FA14C, {x, 0x6EBFEF2Bu, m + 20, load<u32>(m + 20)});
        call(ctx, 0x004F986C, {x, 0x04B38BFFu, m + 24, load<u8>(m + 24)});
    }
    ctx.gpr[3] = m;
}

// void f(T* o): flag 0x80000000 of +8 cleared; each entry e of the u16
// pointer list +12 (count +20) with flag 0x40000 of +172 gets
// e->vt[128](e, 0x879D1B7A, 1) and e->vt[40](e, 1); count = 0. r3 = the
// old count.
DECOMP(000B4414, List_ReleaseAll_000B4414) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 8, load<u32>(o + 8) & 0x7FFFFFFFu);
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 20);
        ctx.gpr[3] = n;
        if (i >= n) break;
        const u32 e = load<u32>(load<u32>(o + 12) + 4 * i);
        if (e == 0 || (load<u32>(e + 172) & 0x40000u) == 0) continue;
        VCall(ctx, e, 128, {e, 0x879D1B7Au, 1});
        VCall(ctx, e, 40, {e, 1});
    }
    store<u16>(o + 20, 0);
}

// bool f(T* o): AnyStateFlag34.
DECOMP(0017C6D8, Obj_AnyStateFlag_0017C6D8) { AnyStateFlag34(ctx); }

// void f(T* o, u8 off): PhysToggle34 for state 3.
DECOMP(00184934, Obj_PhysToggle_00184934) { PhysToggle34(ctx, 3); }

// void f(T* o, u8 off): PhysToggle34 for state 2.
DECOMP(00192220, Obj_PhysToggle_00192220) { PhysToggle34(ctx, 2); }

// void f(T* o, Msg* m): ArmCheck34 (range variant) with k = *0x001BFC08
// and the +256 / +260 floats.
DECOMP(001BFC10, Obj_ArmOnMsg_001BFC10) { ArmCheck34(ctx, 0x001BFC08, 256, false, true); }

// void f(T* o): with a pending +804 / +792, World_AddTimedCallback(world,
// 1, 0x00F33610, o, o); else for a +796 / +784 source, the pose built on
// the stack (0x001D0B40(pose, o, o + 912)) applied with +796
// (0x001D0640), and with +784 a second pose (0x001D09B8(o, pose, pose2,
// o + 720)) applied too; then the world object +160 marked dirty and its
// LOD updated (0, 0). r3 as left.
DECOMP(001D0E58, Obj_ApplyPose_001D0E58) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 804) != 0 || load<u32>(o + 792) != 0) {
        call(ctx, 0x0046B5F8, {load<u32>(0x00F65CA0), 1, 0x00F33610u, o, o});
        return;
    }
    if (load<u32>(o + 796) == 0) {
        const u32 b = load<u32>(o + 784);
        ctx.gpr[3] = b;
        if (b == 0) return;
    }
    Stack frame(ctx, 144);
    const u32 p1 = frame.addr(), p2 = p1 + 64;  // sp+112 / sp+176
    call(ctx, 0x001D0B40, {p1, o, o + 912});
    call(ctx, 0x001D0640, {o, load<u32>(o + 796), p1});
    if (load<u32>(o + 784) != 0) {
        call(ctx, 0x001D09B8, {o, p1, p2, o + 720});
        call(ctx, 0x001D0640, {o, load<u32>(o + 784), p2});
    }
    call(ctx, 0x0057D000, {load<u32>(o + 160)});
    call(ctx, 0x0057EE00, {load<u32>(o + 160), 0, 0});
}

// void f(T* o, X* x): when o's target definition t (Target_GetDef0D951C)
// has +64 == x (x not 0): the three dispatchers +716..+724 get
// Obj_Dispatch362(it, 1, 1, 0); then with u8 (o->+216)->+416 set,
// 0x001DEE48(o, 1) and o->vt[28](o, 0, 8), else 0x002B719C(t). r3 as
// left.
DECOMP(001E0F4C, Obj_OnTargetHit_001E0F4C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if (x == 0) return;
    const u32 d = load<u32>(o + 216);
    const u32 t = static_cast<u32>(call(ctx, 0x000D951C, {o, x}));
    const u32 v = load<u32>(t + 64);
    ctx.gpr[3] = v;
    if (v == 0 || v != x) return;
    for (const u32 k : {716u, 720u, 724u}) call(ctx, 0x0036325C, {load<u32>(o + k), 1, 1, 0});
    if (load<u8>(d + 416) != 0) {
        call(ctx, 0x001DEE48, {o, 1});
        VCall(ctx, o, 28, {o, 0, 8});
    } else {
        call(ctx, 0x002B719C, {t});
    }
}

// void f(T* o): the layer +568 (with the world's layer stack
// *(w + 0x1281E0) set) popped (Obj_PopLayer; f1 = *0x001E904C), then
// vt[4](it) and +568 = 0; o's target definition's +64 object (when set)
// gets vt[88](it, 1, 1, 1, 1); then 0x00489F88(world, 0) and
// World_Resume48A068(world, 1). r3 as that left it.
DECOMP(001E9050, Obj_CloseMenu_001E9050) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = static_cast<u32>(call(ctx, 0x000D951C, {o}));
    const u32 tg = load<u32>(t + 64);
    const u32 L = load<u32>(load<u32>(0x00F65CA0) + 0x1281E0u);
    if (L != 0 && load<u32>(o + 568) != 0) {
        callf(ctx, 0x000FBB88, {L, load<u32>(o + 568)}, {Lfs32(0x001E904C)});
        const u32 m = load<u32>(o + 568);
        if (m != 0) {
            VCall(ctx, m, 4, {m});
            store<u32>(o + 568, 0);
        }
    }
    if (tg != 0) VCall(ctx, tg, 88, {tg, 1, 1, 1, 1});
    call(ctx, 0x00489F88, {load<u32>(0x00F65CA0), 0});
    call(ctx, 0x0048A068, {load<u32>(0x00F65CA0), 1});
}

// T* f(a, b, c): a 240-byte trigger (SmallAlloc; 0 on failure) built
// with 0x00746148(p, c, 0, 0, 1, 0, 1, 0, 0, 0), vtable 0x00B1A2C0, +192
// = the origin 0x01164B40, +208..+220 = 0, the bytes +224..+227 = 0 and
// flag 0x08000000 of +4 set; then Trigger_ReadXml_0021AA68(a, b, p). r3 =
// p (f1 left as the origin's z for it).
DECOMP(0021AD0C, Trigger_New_0021AD0C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {240}));
    if (p != 0) {
        call(ctx, 0x00746148, {p, c, 0, 0, 1, 0, 1, 0, 0, 0});
        store<u32>(p + 208, 0);
        CopyW32(p + 192, 0x01164B40u);
        store<u32>(p, 0x00B1A2C0u);
        CopyW32(p + 196, 0x01164B44u);
        const u32 w4 = load<u32>(p + 4);
        CopyW32(p + 200, 0x01164B48u);
        store<u32>(p + 212, 0);
        store<u32>(p + 216, 0);
        store<u32>(p + 220, 0);
        for (u32 k = 224; k < 228; ++k) store<u8>(p + k, 0);
        store<u32>(p + 4, w4 | 0x08000000u);
        ctx.fpr[1] = Lfs32(0x01164B48);
    }
    call(ctx, 0x0021AA68, {a, b, p});
    ctx.gpr[3] = p;
}

// void f(a, Xml* x, -, Def* d, T* o): Def_ReadBase20BFE8(a, x, -, d, o),
// then the hash 0xF988A90E to o+84 and the f32 0x41A94847 to o+88, with
// defaults from d (when its flag 0x20000000 of +20 is set: the hash of
// "..." (0x00B0F050) when +176 is set, and +180) or 0 / *0x002220C4
// (XmlCursor_DocPath(a) called for a d without the flag). r3 / f1 as
// left.
DECOMP(002220C8, Def_ReadTarget_002220C8) {
    const u32 a = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), d = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, 0x0020BFE8, {a, x, ctx.gpr[5], d, o});
    f64 k = Lfs32(0x002220C4);
    u32 h = 0;
    if (d != 0) {
        if ((load<u32>(d + 20) & 0x20000000u) != 0) {
            if (load<u32>(d + 176) != 0) h = static_cast<u32>(call(ctx, 0x0056A710, {0x00B0F050u}));
            k = Lfs32(d + 180);
        } else {
            call(ctx, 0x004F8944, {a});
        }
    }
    call(ctx, 0x004FA14C, {x, 0xF988A90Eu, o + 84, h});
    callf(ctx, 0x004F9B20, {x, 0x41A94847u, o + 88}, {k});
}

// bool f(T* o): AnyStateFlag34.
DECOMP(002824E8, Obj_AnyStateFlag_002824E8) { AnyStateFlag34(ctx); }

// void f(T* o, u8 on): without on, every element e (8-byte entries, u16
// count +8) of each of o's u16-counted lists +64 (count +72) gets
// e->vt[40](e, 0, 1); then flag 0x40 of u8 +2338 = on. r3 = that byte.
DECOMP(0029F5D8, Obj_SetListsActive_0029F5D8) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 on = ctx.gpr[4];
    if ((on & 0xFFu) == 0) {
        for (u32 i = 0; i < load<u16>(o + 72); ++i) {
            const u32 v = load<u32>(load<u32>(o + 64) + 4 * i);
            for (u32 j = 0; j < load<u16>(v + 8); ++j) {
                const u32 e = load<u32>(load<u32>(v) + 8 * j);
                if (e != 0) VCall(ctx, e, 40, {e, 0, 1});
            }
        }
    }
    const u32 b = (load<u8>(o + 2338) & ~0x40u) | (static_cast<u32>(on & 1u) << 6);
    store<u8>(o + 2338, static_cast<u8>(b));
    ctx.gpr[3] = b;
}

// bool f(T* o, u8 a): in state 2 (+68): Obj_SetStateFx266E6C(o, 0), the
// child +8 (when set) gets vt[256](c, a); with flag 0x100000 of +124 the
// held object m = +64 is reset (Obj_Reset6_0012D290(m, +8) when its flag
// 8 of +192 is set), released (vt[40](m, 0)) and +64 cleared. r3 = 1, or
// 0 in any other state.
DECOMP(002B77E0, Obj_LeaveState2_002B77E0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu;
    if (load<u32>(o + 68) != 2) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x00266E6C, {o, 0});
    const u32 c = load<u32>(o + 8);
    if (c != 0) VCall(ctx, c, 256, {c, a});
    if ((load<u32>(o + 124) & 0x00100000u) != 0) {
        const u32 c2 = load<u32>(o + 8);
        u32 m = load<u32>(o + 64);
        if (m != 0) {
            if ((load<u32>(m + 192) & 8u) != 0) {
                call(ctx, 0x0012D290, {m, c2});
                m = load<u32>(o + 64);
            }
            if (m != 0) VCall(ctx, m, 40, {m, 0});
        }
        store<u32>(o + 64, 0);
    }
    ctx.gpr[3] = 1;
}

// void f(T* o, u8 off): with off, 0x002FEA20(o), each part of o's
// definition list ((o->+216)->+304, u16 count +312) gets
// 0x002F92C0(part, 1), Global_Set106C270_F64CB4(o, 0),
// Objects_Flush365360(), Obj_Flag13Off_0048F298(o, 1) and the overlay
// +1024 (when set) gets vt[4](it) and is cleared; then
// Obj_SoundOffBase(o, off). r3 as that left it.
DECOMP(002FEEA0, Obj_Shutdown_002FEEA0) {
    const u32 o = arg<u32>(ctx, 0), off = arg<u32>(ctx, 1) & 0xFFu;
    if (off != 0) {
        call(ctx, 0x002FEA20, {o});
        const u32 d = load<u32>(o + 216);
        for (u32 i = 0; i < load<u16>(d + 312); ++i) call(ctx, 0x002F92C0, {load<u32>(load<u32>(d + 304) + 4 * i), 1});
        call(ctx, 0x00369D00, {o, 0});
        call(ctx, 0x00365360, {});
        call(ctx, 0x0048F298, {o, 1});
        const u32 p = load<u32>(o + 1024);
        if (p != 0) {
            VCall(ctx, p, 4, {p});
            store<u32>(o + 1024, 0);
        }
    }
    call(ctx, 0x000E4544, {o, off});
}

// void f(T* o, Msg* m): for the message whose id m->+8 is entry 23 of the
// 33-entry table 0x00F65BF8, an event record (kind 33, five words 0) on
// the stack is filled by Event23_Init_004658CC(rec, m->+0, m->+12, 0, 0,
// 0); when its kind changed, o->vt[16](o, (s8)(m->+0)->+172, rec) (entry
// in r6, OPD in r7). r3 as left (m->+0 for other ids).
DECOMP(00321BE4, Obj_OnMsgEvent_00321BE4) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 k = load<u32>(m + 8);
    ctx.gpr[3] = load<u32>(m);
    u32 i = 0;
    while (i < 33 && load<u32>(0x00F65BF8u + 4 * i) != k) ++i;
    if (i != 23) return;
    Stack frame(ctx, 32);
    const u32 rec = frame.addr();  // sp+112
    store<u32>(rec, 33);
    store<u32>(rec + 20, 0);
    store<u32>(rec + 16, 0);
    store<u32>(rec + 12, 0);
    store<u32>(rec + 8, 0);
    store<u32>(rec + 4, 0);
    call(ctx, 0x004658CC, {rec, load<u32>(m), load<u32>(m + 12), 0, 0, 0, 0});
    const u32 kind = load<u32>(rec);
    ctx.gpr[3] = kind;
    if (kind == 33) return;
    const u32 s = load<u32>(m);
    const u32 opd = load<u32>(load<u32>(o) + 16);
    const u32 v = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(s + 172))));
    call_ptr(ctx, opd, {o, v, rec, load<u32>(opd), opd});
}

// void f(T* o): the link +28 (when set) gets 0x0034C918(it); then for the
// world object a = +24 with flag 0x8000 of +164 and an offset +144 that
// is not the origin 0x01164B40, when Global_Test10C5A20_0046C7B0(world)
// passes, a's +64 = origin + offset + o's +32 and it is marked dirty. r3
// as left; f1 = the x stored.
DECOMP(0036A2FC, Obj_PlaceWorldObj_0036A2FC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 l = load<u32>(o + 28);
    if (l != 0) call(ctx, 0x0034C918, {l});  // reads f1: the caller's f1, passed through
    const u32 a = load<u32>(o + 24);
    ctx.gpr[3] = 0;
    if (a == 0 || (load<u32>(a + 164) & 0x8000u) == 0) return;
    constexpr u32 org = 0x01164B40u;
    if (load<u64>(o + 144) == load<u64>(org) && load<u32>(o + 152) == load<u32>(org + 8)) return;
    if (static_cast<u32>(call(ctx, 0x0046C7B0, {load<u32>(0x00F65CA0)})) == 0) return;
    f64 v[3];
    for (u32 k = 0; k < 3; ++k) v[k] = static_cast<f32>(Lfs32(org + 4 * k) + Lfs32(o + 144 + 4 * k));
    for (u32 k = 0; k < 3; ++k) v[k] = static_cast<f32>(v[k] + Lfs32(o + 32 + 4 * k));
    const u32 a2 = load<u32>(o + 24);
    for (u32 k = 0; k < 3; ++k) Stfs32(a2 + 64 + 4 * k, v[k]);
    ctx.fpr[1] = v[0];
    ctx.fpr[2] = v[1];
    ctx.fpr[3] = v[2];
    call(ctx, 0x0057D000, {a2});
}

// bool f(A* a, B* b): the slot s = (s16)a->+6 of b's 132-byte records
// (+152, count u16 +144; used while its +0 < 10, else address 0) offset
// by u16 *0x0106CF80 + 4 gives a pointer q; for b's target t = b->+12
// whose (t->+244)->+256 controller has flag 2 of +192 and flag 0x40000
// of +172, q (when set) is passed to 0x0001BCF4(*0x00F557F0, t, q), else
// t is removed from that array (PtrArray_RemoveValue_0001285C). r3 = 0.
DECOMP(00397858, Obj_SyncSlotLink_00397858) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const s32 s = static_cast<s16>(load<u16>(a + 6));
    const u32 t = load<u32>(b + 12);
    u32 p = 0;
    if (s < static_cast<s32>(load<u16>(b + 144))) {
        const u32 r = b + static_cast<u32>(s) * 132u + 152u;
        if (static_cast<s32>(load<u32>(r)) < 10) p = r;
    }
    p = p + load<u16>(0x0106CF80) + 4u;
    ctx.gpr[3] = 0;
    if (t == 0) return;
    const u32 u = load<u32>(t + 244);
    if (u == 0) return;
    const u32 c = load<u32>(u + 256);
    if (c == 0 || (load<u32>(c + 192) & 2u) == 0 || (load<u32>(c + 172) & 0x40000u) == 0) return;
    const u32 q = load<u32>(p);
    if (q != 0) call(ctx, 0x0001BCF4, {load<u32>(0x00F557F0), t, q});
    else call(ctx, 0x0001285C, {load<u32>(0x00F557F0), t});
    ctx.gpr[3] = 0;
}

// void f(a, T* o): 0x003EC950(a, o) (r5 left as 0x003F0000), then
// k = *0x003ECA94 added to the float +224 of each of o's parts
// +232..+260 that is set (+232 only without flag 0x40000000 of its
// +268). r3 = the part +256, f1 = the last sum (k without +260).
DECOMP(003ECA98, Obj_AddPartTimes_003ECA98) {
    const u32 a = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    call(ctx, 0x003EC950, {a, o, 0x003F0000u});
    const f64 k = Lfs32(0x003ECA94);
    ctx.fpr[1] = k;
    for (u32 off = 232; off <= 260; off += 4) {
        const u32 p = load<u32>(o + off);
        if (off != 260) ctx.gpr[3] = p;
        if (p == 0) continue;
        if (off == 232 && (load<u32>(p + 268) & 0x40000000u) != 0) continue;
        const f64 v = static_cast<f32>(Lfs32(p + 224) + k);
        Stfs32(p + 224, v);
        ctx.fpr[off == 260 ? 1 : 2] = v;
    }
}

// void f(T* o): SendRequests34.
DECOMP(00427BF0, Net_SendRequests_00427BF0) { SendRequests34(ctx); }

// void f(T* o): SendRequests34 (an identical copy).
DECOMP(00427E2C, Net_SendRequests_00427E2C) { SendRequests34(ctx); }

// bool f(Arr* a, T* t): {(t->+40)->+204, t} appended to the 8-byte
// u16-counted array a+28 (capacity +28 doubled when full via
// Array_Reserve, count +30, high-water +32, flag 1 set in u16 +34); a
// full array (50) reports "..." (0x00AE5208) with t's name
// ((t->+40)->vt[12]) and gives r3 = 0. r3 = 1.
DECOMP(00454CAC, Arr_AddEntry_00454CAC) {
    const u32 a = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 n = load<u16>(a + 30);
    if (static_cast<s32>(n) >= 50) {
        const u32 c = load<u32>(t + 40);
        const u32 nm = static_cast<u32>(VCall(ctx, c, 12, {c}));
        call(ctx, 0x005534D0, {0x00AE5208u, 50, nm});
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = load<u32>(load<u32>(t + 40) + 204);
    const u32 cap = load<u16>(a + 28);
    if (n == cap) call(ctx, 0x00558F3C, {a + 28, (cap << 1) & 0xFFFEu});
    const u32 d = load<u32>(a + 36);
    store<u32>(d + ((static_cast<u32>(load<u16>(a + 30)) << 3) & 0x7FFF8u), v);
    const u32 c = load<u16>(a + 30);
    const u32 hw = load<u16>(a + 32);
    store<u32>(d + (c << 3) + 4, t);
    const u32 c2 = c + 1u;
    const u32 fl = load<u16>(a + 34);
    store<u16>(a + 30, static_cast<u16>(c2));
    store<u16>(a + 32, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c2 & 0xFFFFu) ? hw : c2));
    store<u16>(a + 34, static_cast<u16>(fl | 1u));
    ctx.gpr[3] = 1;
}

// void f(T* o): AnimSlotReset34 for "..." (0x00AEE2D8), last u16 +60.
DECOMP(004B3C70, AnimSlot_Reset_004B3C70) { AnimSlotReset34(ctx, 0x00AEE2D8u, 60); }

// void f(T* o): AnimSlotReset34 for "..." (0x00AEE2C0), last u16 +68.
DECOMP(004B54D0, AnimSlot_Reset_004B54D0) { AnimSlotReset34(ctx, 0x00AEE2C0u, 68); }

// T* New(-, Xml* x): a 24-byte object (Mem_Alloc(24, 0, 16); 0 on
// failure): vtable 0x00B222E0, u16 +4 = 1, +12 = the hash of the
// attribute 0x00AEF08C (when present), the f32s 0x03DE9F9A (+20) and
// 0xF1E9AC63 (+16), default *0x004C5C24. r3 = the object.
DECOMP(004C5C28, VtB222E0_NewXml_004C5C28) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {24, 0, 16}));
    if (m != 0) {
        const f64 k = Lfs32(0x004C5C24);
        store<u16>(m + 4, 1);
        store<u32>(m, 0x00B222E0u);
        store<u32>(m + 12, 0);
        store<u32>(m + 20, 0);
        store<u32>(m + 16, 0);
        const u32 a = static_cast<u32>(call(ctx, 0x004F9628, {x, 0x00AEF08Cu}));
        if (a != 0) store<u32>(m + 12, static_cast<u32>(call(ctx, 0x0056A710, {a})));
        callf(ctx, 0x004F9B20, {x, 0x03DE9F9Au, m + 20}, {k});
        callf(ctx, 0x004F9B20, {x, 0xF1E9AC63u, m + 16}, {k});
    }
    ctx.gpr[3] = m;
}

// T* f(-, const char* name, s32 kind): the object registered under the
// name's hash in the map 0x010C2B20, or else in 0x010C2AF0
// (HashMap_Find on a stack key), whose vt[8]() kind matches. r3 = it or
// 0.
DECOMP(004E6108, Registry_FindKind_004E6108) {
    const u32 name = arg<u32>(ctx, 1), kind = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 k2 = frame.addr(), k1 = k2 + 4;  // sp+112 / sp+116
    store<u32>(k1, static_cast<u32>(call(ctx, 0x0056A710, {name})));
    const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x007948E4, {0x010C2B20u, k1})));
    if (e != 0 && static_cast<u32>(VCall(ctx, e, 8, {e})) == kind) {
        ctx.gpr[3] = e;
        return;
    }
    store<u32>(k2, static_cast<u32>(call(ctx, 0x0056A710, {name})));
    const u32 e2 = load<u32>(static_cast<u32>(call(ctx, 0x007948E4, {0x010C2AF0u, k2})));
    const bool hit = e2 != 0 && static_cast<u32>(VCall(ctx, e2, 8, {e2})) == kind;
    ctx.gpr[3] = hit ? e2 : 0u;
}

// Deleting ~T() (o may be 0): vtable 0x00F18D98, +24 = 0; unless flag
// 0x80000000 of +28 is set, the block +20 (+28 * 4 bytes) returned to the
// allocator object 0x011DF31C (its vt[20](a, p, n), entry in r6, OPD in
// r7); +20 = 0, +28 = 0x80000000, 0x007CDB90(o + 8, a), 0x007CD128(o + 8);
// vtable 0x00B13974 and the thread's memory router ->+44 frees o
// (vt[12](r, o, u16 +4)). r3 as that left it.
DECOMP(0078885C, VtF18D98_Delete_0078885C) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    constexpr u32 A = 0x011DF31Cu;
    const u32 w = load<u32>(o + 28);
    store<u32>(o, 0x00F18D98u);
    store<u32>(o + 24, 0);
    if ((w >> 31) == 0) {
        const u32 opd = load<u32>(load<u32>(A) + 20);
        call_ptr(ctx, opd, {A, load<u32>(o + 20), Sx33(w << 2), load<u32>(opd), opd});
    }
    store<u32>(o + 20, 0);
    store<u32>(o + 28, 0x80000000u);
    call(ctx, 0x007CDB90, {o + 8, A});
    call(ctx, 0x007CD128, {o + 8});
    store<u32>(o, 0x00B13974u);
    const u32 r = load<u32>(HkRouter33(ctx) + 44);
    VCall(ctx, r, 12, {r, o, load<u16>(o + 4)});
}

// ---------------------------------------------------------------- sweep: 69 instructions
// bool f(T* o, u8 big): o's position +176 resolved through
// 0x0001D1E0(a, b, out) for a vertical segment a..b on the stack (with
// big: from +388 to +372, each + (o->+48)->+84; else +-k around the
// position, k = *0x0001D544): the result out when found, else b (big)
// or o's +80 point. r3 = found, f1 = the z stored.
DECOMP(0001D548, Obj_SnapToGround_0001D548) {
    const u32 o = arg<u32>(ctx, 0), big = arg<u32>(ctx, 1) & 0xFFu;
    const u32 p = o + 176;
    Stack frame(ctx, 48);
    const u32 a = frame.addr(), b = a + 16, out = a + 32;  // sp+112 / +128 / +144
    CopyW32(b, p);
    CopyW32(b + 8, p + 8);
    CopyW32(a, p);
    CopyW32(a + 8, p + 8);
    if (big != 0) {
        const u32 s = load<u32>(o + 48);
        CopyW32(a + 4, p + 4);
        CopyW32(b + 4, p + 4);
        Stfs32(a + 4, static_cast<f32>(Lfs32(o + 388) + Lfs32(s + 84)));
        Stfs32(b + 4, static_cast<f32>(Lfs32(o + 372) + Lfs32(s + 84)));
    } else {
        const f64 y = Lfs32(p + 4), k = Lfs32(0x0001D544);
        Stfs32(a + 4, static_cast<f32>(y + k));
        Stfs32(b + 4, static_cast<f32>(y - k));
    }
    const u32 r = static_cast<u32>(call(ctx, 0x0001D1E0, {a, b, out}));
    const u32 src = r != 0 ? out : (big != 0 ? b : o + 80);
    for (u32 k = 0; k < 12; k += 4) CopyW32(p + k, src + k);
    ctx.fpr[1] = Lfs32(src + 8);
    ctx.gpr[3] = r;
}

// void f(Ev* o, X* x): for o's owner (Obj_Owner0D93FC(o->+48)), a hit
// record {2, 0.5, 13.0, 1.0 (3.0 for kind 9 (+4))} and a damage record
// {0x2DA0A51D, 0, *0x00F979BC, 0, u8 0, 0, 0} on the stack and the matrix
// built from the rows 0x01164C50 and {0, -13, 0} (Matrix_FromRows) go to
// 0x00278830(owner, hit, dmg, m, x, 0, 0). r3 as left (the kind word
// without an owner).
DECOMP(0004BCF0, Ev_ApplyHit_0004BCF0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 own = static_cast<u32>(call(ctx, 0x000D93FC, {load<u32>(o + 48)}));
    Stack frame(ctx, 128);
    const u32 v = frame.addr(), m = v + 16, hit = v + 80, dmg = v + 96;  // sp+112 / +128 / +192 / +208
    store<u32>(hit, 2);
    store<u32>(hit + 4, 0x3F000000u);
    store<u32>(hit + 8, 0x41500000u);
    store<u32>(hit + 12, 0x3F800000u);
    u32 k = load<u32>(o + 4);
    if (k == 9) {
        k = 0x40400000u;
        store<u32>(hit + 12, k);
    }
    ctx.gpr[3] = k;
    if (own == 0) return;
    store<u32>(dmg + 12, 0);
    store<u8>(dmg + 16, 0);
    store<u32>(dmg, 0x2DA0A51Du);
    CopyW32(dmg + 8, 0x00F979BCu);
    store<u32>(dmg + 4, 0);
    store<u32>(dmg + 24, 0);
    store<u32>(dmg + 20, 0);
    store<u32>(v, 0);
    store<u32>(v + 4, 0xC1500000u);
    store<u32>(v + 8, 0);
    ctx.fpr[1] = Lfs32(0x00F979BC);
    call(ctx, 0x00553B4C, {m, 0x01164C50u, v});
    call(ctx, 0x00278830, {own, hit, dmg, m, x, 0, 0});
}

// T* New(-, Xml* x): a 28-byte object (Mem_Alloc(28, 0, 16); 0 on
// failure): Obj_InitVt_B21C98(m, x), vtable 0x00B0F090, the bool
// 0xCBF1C4F3 (default 1) to u8 +12, the hash 0xEF44A7B5 to +16, the f32
// 0x21CA4030 to +24 (+20 / +24 first 0), and +20 = Sound_Find(+16) when
// the hash is set. r3 = m.
DECOMP(0006BB5C, VtB0F090_NewXml_0006BB5C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {28, 0, 16}));
    if (m != 0) {
        call(ctx, 0x004BB3B0, {m, x});
        store<u8>(m + 12, 1);
        store<u32>(m, 0x00B0F090u);
        store<u32>(m + 20, 0);
        store<u32>(m + 24, 0);
        call(ctx, 0x004F986C, {x, 0xCBF1C4F3u, m + 12, 1});
        call(ctx, 0x004FA14C, {x, 0xEF44A7B5u, m + 16, 0});
        callf(ctx, 0x004F9B20, {x, 0x21CA4030u, m + 24}, {Lfs32(m + 24)});
        const u32 s = load<u32>(m + 16);
        if (s != 0) store<u32>(m + 20, static_cast<u32>(call(ctx, 0x0044C43C, {s})));
    }
    ctx.gpr[3] = m;
}

// T* New(-, Xml* x): NewXml34 (28 bytes): +12..+24 cleared, vtable
// 0x00B0F0B8, the hashes 0x7A30D7E3 / 0x8A93973F / 0x00255BF3 /
// 0x40795527 to +12..+24 (default 0).
DECOMP(0006BE14, VtB0F0B8_NewXml_0006BE14) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, 0x00B0F0B8u);
        store<u32>(m + 24, 0);
        RdHash34(ctx, x, 0x7A30D7E3u, m + 12, 0);
        RdHash34(ctx, x, 0x8A93973Fu, m + 16, 0);
        RdHash34(ctx, x, 0x00255BF3u, m + 20, 0);
        RdHash34(ctx, x, 0x40795527u, m + 24, 0);
    });
}

// T* New(-, Xml* x): NewXml34 (28 bytes): +12..+24 cleared, vtable
// 0x00B0F3D8, the hash 0x8A93973F to +12 (default 0), the f32
// 0x77C17F6F to +16, the hash 0x0027873A to +24 and the f32 0x1B75F0DA
// to +20 (current defaults).
DECOMP(00071808, VtB0F3D8_NewXml_00071808) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, 0x00B0F3D8u);
        store<u32>(m + 24, 0);
        RdHash34(ctx, x, 0x8A93973Fu, m + 12, 0);
        RdF32Cur34(ctx, x, 0x77C17F6Fu, m + 16);
        RdHashCur34(ctx, x, 0x0027873Au, m + 24);
        RdF32Cur34(ctx, x, 0x1B75F0DAu, m + 20);
    });
}

// T* New(-, Xml* x): NewXml34 (28 bytes): +12..+20 / u8 +24 cleared,
// vtable 0x00B0FB08, the hashes 0x7FAAD4A0 (+12, default 0) /
// 0xE9C0B500 (+16) / 0x001EBA11 (+20) and the bool 0x0023BEA4 (u8 +24).
DECOMP(0007D1A4, VtB0FB08_NewXml_0007D1A4) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, 0x00B0FB08u);
        store<u8>(m + 24, 0);
        RdHash34(ctx, x, 0x7FAAD4A0u, m + 12, 0);
        RdHashCur34(ctx, x, 0xE9C0B500u, m + 16);
        RdHashCur34(ctx, x, 0x001EBA11u, m + 20);
        RdBoolCur34(ctx, x, 0x0023BEA4u, m + 24);
    });
}

// T* New(-, Xml* x): NewXml34 (24 bytes): +12 / +16 / u8 +20 / +21
// cleared, vtable 0x00B0FCE8, the hashes 0x8A93973F (+12, default 0) /
// 0x03D1FCFC (+16) and the bools 0x66D71193 (+20) / 0x739CF08B (+21).
DECOMP(00080778, VtB0FCE8_NewXml_00080778) {
    NewXml34(ctx, 24, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u8>(m + 20, 0);
        store<u32>(m, 0x00B0FCE8u);
        store<u8>(m + 21, 0);
        RdHash34(ctx, x, 0x8A93973Fu, m + 12, 0);
        RdHashCur34(ctx, x, 0x03D1FCFCu, m + 16);
        RdBoolCur34(ctx, x, 0x66D71193u, m + 20);
        RdBoolCur34(ctx, x, 0x739CF08Bu, m + 21);
    });
}

// T* New(-, Xml* x): NewXml34 (24 bytes): +12 / +16 / u8 +20 cleared,
// vtable 0x00B0FEA0, +12 = the s32 0x368328E5 (read into a stack slot,
// default 0; values outside 0..8 become 0), then the hash 0x75751B32
// (+16) and the bool 0x7A281983 (+20).
DECOMP(00082FA8, VtB0FEA0_NewXml_00082FA8) {
    NewXml34(ctx, 24, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u8>(m + 20, 0);
        Stack frame(ctx, 16);
        const u32 v = frame.addr();  // sp+112
        store<u32>(m, 0x00B0FEA0u);
        store<u32>(v, 0);
        call(ctx, 0x004F99BC, {x, 0x368328E5u, v, 0});
        u32 k = load<u32>(v);
        if (static_cast<s32>(k) < 0 || static_cast<s32>(k) >= 9) {
            k = 0;
            store<u32>(v, 0);
        }
        store<u32>(m + 12, k);
        RdHashCur34(ctx, x, 0x75751B32u, m + 16);
        RdBoolCur34(ctx, x, 0x7A281983u, m + 20);
    });
}

// T* New(-, Xml* x): NewXml34 (28 bytes): +12..+24 cleared, vtable
// 0x00B102B0, the hashes 0x00014B07 (+12, default 0) / 0x7A30D7E3 (+16)
// / 0x52A72CDD (+20) / 0x9178C4CB (+24).
DECOMP(0008804C, VtB102B0_NewXml_0008804C) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, 0x00B102B0u);
        store<u32>(m + 24, 0);
        RdHash34(ctx, x, 0x00014B07u, m + 12, 0);
        RdHashCur34(ctx, x, 0x7A30D7E3u, m + 16);
        RdHashCur34(ctx, x, 0x52A72CDDu, m + 20);
        RdHashCur34(ctx, x, 0x9178C4CBu, m + 24);
    });
}
