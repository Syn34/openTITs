// Game code, part 47: the small-function sweep continued from the remaining
// 101 instruction functions (size order; obj46*.cpp did 99-101). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj47.h"

using namespace sly4;
using namespace sly4::obj47;

// ---------------------------------------------------------------- sweep: 101 instructions
// u32 f(Cmd* c) (sequence command, like SeqCmd_Spawn_0042EF6C): when
// Ptr_DerefNonZero(c->+20) fails, String_Format(0x00AC4B08,
// Hash_ToString(c->+12), Hash_ToString(c->+16)) (r3 = its result); when
// Ptr_Test88_00443168(c->+20) fails r3 = 0; when List_FindByKey_004464F0(
// 0x010ACC40, c->+20) finds one and u8 c->+32 is clear r3 = c->+20.
// Otherwise the 68-byte spawn record on the stack (vec3s 0x01164B40 at +0 /
// +16, +32 = 1.0f, the rest 0, +48 = c->+24, +52 = c->+28) goes to
// Call_446198_00446348(0x010ACC40, c->+20, rec, c->+28, ...) (result
// dropped), then SpawnRec_Dtor_00440AC4 / SpawnRec_Dtor2_00440A40(rec, 0).
DECOMP(00074354, SeqCmd_SpawnAt_00074354) {
    const u32 c = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x00446960u, {load<u32>(c + 20)})) == 0) {
        const u32 a = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(c + 12)}));
        const u32 b = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(c + 16)}));
        call(ctx, 0x005534D0u, {0x00AC4B08u, a, b});
        return;
    }
    if (static_cast<u32>(call(ctx, 0x00443168u, {load<u32>(c + 20)})) == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 L = 0x010ACC40u;
    const u32 r = static_cast<u32>(call(ctx, 0x004464F0u, {L, load<u32>(c + 20)}));
    const u32 k = load<u32>(c + 20);
    if (r != 0 && load<u8>(c + 32) == 0) {
        ret(ctx, k);
        return;
    }
    Stack fr(ctx, 112);  // the record at sp+112..179 (frame of 224 bytes)
    const u32 rec = fr.addr(), sp = fr.addr() - 112;
    for (u32 j = 0; j < 3; ++j) {
        Cp47(rec + 4 * j, 0x01164B40u + 4 * j);
        Cp47(rec + 16 + 4 * j, 0x01164B40u + 4 * j);
    }
    store<u32>(rec + 32, 0x3F800000u);
    store<u32>(rec + 36, 0);
    store<u32>(rec + 40, 0);
    store<u8>(rec + 44, 0);
    store<u8>(rec + 45, 0);
    store<u8>(rec + 46, 0);
    store<u8>(rec + 47, 0);
    store<u32>(rec + 56, 0);
    store<u32>(rec + 60, 0);
    store<u32>(rec + 64, 0);
    store<u32>(rec + 48, load<u32>(c + 24));
    store<u32>(rec + 52, load<u32>(c + 28));
    // f1..f3 hold the vec3 as the record stores leave them; r7 / r8 / r9 =
    // sp+1, 0, sp+3 (the byte stores' bases), r10 = c->+24; the callee reads r3..r5
    callf(ctx, 0x00446348u, {L, k, rec, load<u32>(c + 28), sp + 1, 0, sp + 3, load<u32>(c + 24)},
          {Lfs47(0x01164B40u), Lfs47(0x01164B44u), Lfs47(0x01164B48u)});
    call(ctx, 0x00440AC4u, {rec, 0});
    call(ctx, 0x00440A40u, {rec, 0});
}

// u32 f(Cmd* c, X* x) (like Cmd_ApplyBytes_00075C04): the target t
// (CompTarget34(x, c->+12) when c->+12 is set, else / when that fails
// Obj_FindById(c->+16)), narrowed by the hash c->+20
// (Obj_FindByHash_000DB290) when set; none -> r3 = 0. Then the low bit of
// u8 c->+24 goes to bit 0x40000000 of t->+420 when t->+192 has 4, to bit
// 0x80 of u16 t->+432 when t->+192 has 0x20, and to bit 0x8000 of u16
// t->+584 when t->+196 has 0x20000 (r3 = that byte; 0 otherwise).
DECOMP(00075880, Cmd_ApplyFlagBits_00075880) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    if (load<u32>(c + 12) != 0) t = CompTarget34(ctx, x, load<u32>(c + 12));
    if (t == 0) {
        const u32 id = load<u32>(c + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    }
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u32>(c + 20) != 0) {
        t = static_cast<u32>(call(ctx, 0x000DB290u, {t, load<u32>(c + 20)}));
        if (t == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    if ((load<u32>(t + 192) & 0x4u) != 0) {
        const u32 b = load<u8>(c + 24);
        store<u32>(t + 420, (load<u32>(t + 420) & ~0x40000000u) | ((b & 1u) << 30));
    }
    if ((load<u32>(t + 192) & 0x20u) != 0) {
        const u32 b = load<u8>(c + 24);
        store<u16>(t + 432, static_cast<u16>((load<u16>(t + 432) & ~0x80u) | ((b & 1u) << 7)));
    }
    if ((load<u32>(t + 196) & 0x20000u) != 0) {
        const u32 b = load<u8>(c + 24);
        store<u16>(t + 584, static_cast<u16>((load<u16>(t + 584) & ~0x8000u) | ((b & 1u) << 15)));
        ret(ctx, b);
        return;
    }
    ret(ctx, 0u);
}

// T* New(-, Xml* x): a 40-byte command (0 when Mem_Alloc(40, 0, 16)
// fails): Obj_InitVt_B21C98(m, x), vtable 0x00B10490, +12 / +16 / the u64
// +24 / u8 +32 = 0; hashes 0x8A93973F -> +12 (default 0), 0x7A30D7E3 ->
// +16; the hash 0x0020F76C (when the reader returns 0) is looked up in the
// 7 16-byte rows of 0x00AC51A0 (u32 key, u64 value at +8 -> +24), with
// String_Format(0x00AC5220, Hash_ToString(h)) when +24 is still 0; the
// bool 0x7A281983 -> +32. r3 = m.
DECOMP(0008A36C, Cmd_New40_0008A36C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = NewXmlObj47(ctx, 40, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u64>(m + 24, 0);
    store<u32>(m, 0x00B10490u);
    store<u8>(m + 32, 0);
    call(ctx, kReadHash47, {x, 0x8A93973Fu, m + 12, 0});
    RdHash47(ctx, x, 0x7A30D7E3u, m + 16);
    {
        Stack fr(ctx, 16);  // the u32 at sp+112
        const u32 hp = fr.addr();
        if (static_cast<u32>(call(ctx, kReadHash47, {x, 0x0020F76Cu, hp, 0})) == 0) {
            const u32 h = load<u32>(hp);
            for (u32 i = 0; i < 7; ++i) {
                if (load<u32>(0x00AC51A0u + 16 * i) == h) {
                    store<u64>(m + 24, load<u64>(0x00AC51A8u + 16 * i));
                    break;
                }
            }
            if (load<u64>(m + 24) == 0) {
                const u32 s = static_cast<u32>(call(ctx, 0x0056A6E0u, {h}));
                call(ctx, 0x005534D0u, {0x00AC5220u, s});
            }
        }
    }
    RdBool47(ctx, x, 0x7A281983u, m + 32);
    ret(ctx, m);
}

// T* New(-, Xml* x): a 36-byte command (0 when Mem_Alloc(36, 0, 16)
// fails): Obj_InitVt_B21C98(m, x), vtable 0x00B113C0, +12..+24 = 0, +28 =
// 1.0f, u8 +32 = 0, u8 +33 = 1; hashes 0x7A30D7E3 -> +12 (default 0),
// 0x8D47DA8F -> +16, 0xFCB43218 -> +20, 0x001EBA11 -> +24, the f32
// 0x04B70827 -> +28, bools 0x922FF61A -> +32, 0x8488565B -> +33. r3 = m.
DECOMP(0009F464, Cmd_New36_0009F464) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = NewXmlObj47(ctx, 36, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0);
    store<u32>(m, 0x00B113C0u);
    store<u32>(m + 24, 0);
    store<u32>(m + 28, 0x3F800000u);
    store<u8>(m + 32, 0);
    store<u8>(m + 33, 1);
    call(ctx, kReadHash47, {x, 0x7A30D7E3u, m + 12, 0});
    RdHash47(ctx, x, 0x8D47DA8Fu, m + 16);
    RdHash47(ctx, x, 0xFCB43218u, m + 20);
    RdHash47(ctx, x, 0x001EBA11u, m + 24);
    RdF32_47(ctx, x, 0x04B70827u, m + 28);
    RdBool47(ctx, x, 0x922FF61Au, m + 32);
    RdBool47(ctx, x, 0x8488565Bu, m + 33);
    ret(ctx, m);
}

// void f(Vec3* out, Obj* o): with a one-entry list o->+388 (u16 count +8
// == 1) whose first 48-byte row's +28 is set, out = that object's +288
// vec3 (r3 = out + 8). Otherwise, when Obj_FindComponent(o, 31, 0) finds
// one, out = o->+144 + Vec3_SetHalfY84_002BC1E0(tmp, Obj_FindComponent(o,
// 31, 0)) (fadds; r3 = its result), else out = o->+144 (r3 = 0).
DECOMP(000E74B8, Obj_AimPoint_000E74B8) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 l = load<u32>(o + 388);
    if (l != 0 && load<u16>(l + 8) == 1) {
        // the original's index loop always stops at row 0
        const u32 e = load<u32>(load<u32>(l) + 28);
        if (e != 0) {
            Cp47(out, e + 288);
            Cp47(out + 4, e + 292);
            Cp47(out + 8, e + 296);
            ctx.fpr[1] = Lfs47(e + 296);  // f1 as the copy leaves it
            ret(ctx, out + 8);
            return;
        }
    }
    if (static_cast<u32>(call(ctx, 0x000D1288u, {o, 31, 0})) == 0) {
        Cp47(out, o + 144);
        Cp47(out + 4, o + 148);
        Cp47(out + 8, o + 152);
        ctx.fpr[1] = Lfs47(o + 152);  // f1 as the copy leaves it
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 16);  // the vec3 at sp+112
    const u32 v = fr.addr();
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 31, 0}));
    const u32 r = static_cast<u32>(call(ctx, 0x002BC1E0u, {v, c}));
    const u32 x = W47(Fadds47(Lfs47(o + 144), Lfs47(v)));
    const u32 y = W47(Fadds47(Lfs47(o + 148), Lfs47(v + 4)));
    const f64 fz = Fadds47(Lfs47(o + 152), Lfs47(v + 8));
    const u32 z = W47(fz);
    store<u32>(out, x);
    store<u32>(out + 4, y);
    store<u32>(out + 8, z);
    ctx.fpr[1] = fz;  // f1 = the last sum, as the original leaves it
    ret(ctx, r);
}

// void f(o, a1..a5, -, a7, f1, f4, stack args) (r3..r10 and the FPRs
// passed through): TunedObj_Init_00119FF0 with the caller's stack
// arguments copied down one frame (words; the u8 ones masked: slots 17,
// 20..22, 26; the 4-byte struct of slot 16 copied bytewise into its high
// word). r9 is the stack word of slot 32 (as the original's r9 copy loop
// leaves it); the slots it never writes (24, 25, 27 and the low word of
// 16) are 0 here (stale stack in the original; the callee does not read them).
DECOMP(0011A42C, TunedObj_InitFwd_0011A42C) {
    auto S = [&](u32 off) -> u64 { return StkW47(ctx, off); };
    auto SB = [&](u32 off) -> u64 { return StkB47(ctx, off); };
    const u64 st = (static_cast<u64>(StkW47(ctx, 176)) << 32);
    call(ctx, 0x00119FF0u,
         {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], S(308), ctx.gpr[10],
          S(116), S(124), S(132), S(140), S(148), S(156), S(164), S(172),  // slots 8..15
          st, SB(191), S(196), S(204), SB(215), SB(223), SB(231), S(236),  // 16..23
          0, 0, SB(263), 0, S(276), S(284), S(292), S(300), S(308)});      // 24..32
}

// T* f(A* a, B* b) of 001E9D98 / 002F7BC4 / 003072A8 / 001AC38C: an object built
// from a stack record: Record_Init_000E44E0(rec, a->vslot 32(a, b), a, 0,
// 0x01164BD0, 0x01164B30, 1, 0, 0, 0, 0, 0), rec->+off |= bits,
// b->vslot 8(b, rec); m = SmallAlloc(size) and ctor(m, rec) when that
// worked; m->vslot 8(m) (unchecked: a failed alloc calls through address
// 0); then the record's teardown (PtrVector_Clear / Nop on +164, +144 and
// the 20-byte block +0, SmallFree(+0, 20)). r3 = m.
static void SpawnFromRecord47(PPUContext& ctx, u32 off, u32 bits, u32 size, u32 ctor) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, a, 32, {a, b}));
    Stack fr(ctx, 224);  // the record at sp+144..~sp+330 (frame of 384 bytes)
    const u32 rec = fr.addr();
    call(ctx, 0x000E44E0u, {rec, v, a, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0, 0});
    store<u32>(rec + off, load<u32>(rec + off) | bits);
    VCall(ctx, b, 8, {b, rec});
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {size}));
    u32 r = 0;
    if (m != 0) {
        call(ctx, ctor, {m, rec});
        r = m;
    }
    VCall(ctx, r, 8, {r});
    call(ctx, 0x0054EC1Cu, {rec + 164});
    call(ctx, 0x0054EC18u, {rec + 164});
    call(ctx, 0x0054EC1Cu, {rec + 144});
    call(ctx, 0x0054EC18u, {rec + 144});
    const u32 p = load<u32>(rec);
    if (p != 0) {
        call(ctx, 0x0054EC1Cu, {p});
        call(ctx, 0x0054EC18u, {p});
        call(ctx, 0x00552420u, {load<u32>(rec), 20});
    }
    ret(ctx, r);
}

// T* f(A* a, B* b): SpawnFromRecord47 with rec->+8 |= 0x20000000, a
// 464-byte VtB18260_Ctor_001E9C90 object.
DECOMP(001E9D98, ObjB18260_Spawn_001E9D98) { SpawnFromRecord47(ctx, 8, 0x20000000u, 464, 0x001E9C90u); }

// T* f(A* a, B* b): SpawnFromRecord47 with rec->+8 |= 0x00008000, a
// 448-byte 0x001AC1E8 object.
DECOMP(001AC38C, Obj1AC1E8_Spawn_001AC38C) { SpawnFromRecord47(ctx, 8, 0x00008000u, 448, 0x001AC1E8u); }

// T* f(x, XmlCursor* cur, const Def* tmpl, parent): the default 256-byte
// collision record d (Rec_Init7576D8(d, tmpl->+12, tmpl->+16, tmpl->+20,
// "", ...) with the floats of the .text pool 0x001F9B88, as
// ColDef_Read_001F9078 builds its own), ColDef_Read_001F9078(x, cur, tmpl,
// parent, d), then a 304-byte object (Mem_Alloc(304, 0, 16);
// 0x001F85F0(m, d)). r3 = m (0 on failure).
DECOMP(001F9BA0, ColObj_New_001F9BA0) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1), tmpl = arg<u32>(ctx, 2), parent = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001F9B88u;
    auto Cb = [](u32 off) -> u64 { return load<u32>(K + off); };
    auto Cf = [](u32 off) -> f64 { return Lfs47(K + off); };
    Stack fr(ctx, 304);  // the record at sp+432..687 (frame of 736 bytes)
    const u32 d = fr.addr();
    callf(ctx, 0x007576D8u,
          {d, load<u32>(tmpl + 12), load<u32>(tmpl + 16), load<u32>(tmpl + 20), 0x00AD2B74u, 0x0024A738u, 0x01164B40u,
           ctx.gpr[10],                                       // r8 / r9 as the slot stores leave them, r10 untouched
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                      // slots 8..17 (the FPR arguments')
           Cb(4), Cb(16), Cb(12), Cb(12), Cb(12), Cb(8), Cb(12), Cb(12),  // 18..25
           0x0024A738u,                                       // 26
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,       // 27..41
           Cb(16), Cb(16), 0, 0x01164B40u, 0x01164B40u},      // 42..46
          {Cf(16), Cf(16), Cf(16), Cf(16), Cf(16), Cf(12), Cf(0), Cf(16), Cf(12), Cf(12), Cf(12), Cf(12), Cf(8)});
    call(ctx, 0x001F9078u, {x, cur, tmpl, parent, d});
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {304, 0, 16}));
    u32 r = 0;
    if (m != 0) {
        call(ctx, 0x001F85F0u, {m, d});
        r = m;
    }
    ret(ctx, r);
}

// u32 f(Volume* v): flushes the u16-counted pending list v->+396 (count
// +390; nothing to do -> r3 = v): +412 |= 0x02000000, the 8-byte entries
// memmove'd to the buffer 0x01055D00, the count cleared; for each entry's
// object e (+4): e->vslot 40(e, 0), then v->vslot 288(v, e) when e is in
// the s32 array v->+344 (u16 count +352). Then +412 &= ~0x02000000 and,
// when bit 0x80000000 is now clear but was set before,
// Volume_Prepare20C670(v) (r3 = its result; else r3 = the new +412).
DECOMP(0020DE3C, Volume_FlushPending_0020DE3C) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 n = load<u16>(v + 390);
    if (n == 0) return;  // r3 = v
    const u32 w0 = load<u32>(v + 412) | 0x02000000u;
    store<u32>(v + 412, w0);
    const bool was = (w0 >> 31) != 0;
    constexpr u32 kBuf = 0x01055D00u;
    call(ctx, 0x0071C3F0u, {kBuf, load<u32>(v + 396), (n << 3) & 0x7FFF8u});
    store<u16>(v + 390, 0);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(kBuf + 4 + 8 * i);
        VCall(ctx, e, 40, {e, 0});
        const s32 cnt = load<u16>(v + 352);
        const u32 arr = load<u32>(v + 344);
        for (s32 j = 0; j < cnt; ++j) {
            if (load<u32>(arr + 4 * static_cast<u32>(j)) == e) {
                VCall(ctx, v, 288, {v, e});
                break;
            }
        }
    }
    const u32 w = load<u32>(v + 412) & ~0x02000000u;
    store<u32>(v + 412, w);
    if ((w >> 31) == 0 && was) {
        call(ctx, 0x0020C670u, {v});
        return;
    }
    ret(ctx, w);
}

// void f(-, T* t) (t 0 -> nothing): t->+24 ->vslot 40(.., 0) when its
// s16 +178 is positive; t->+52 ->vslot 40(.., 1) when its +172 has
// 0x40000; each 8-byte entry of the u16-counted array t->+0 (count +8):
// e = +0 ->vslot 8(e) then +0 = 0 when set, +4 = 0; the count cleared;
// 0x006C0D24(t->+56), +56 = 0, 0x006C0D24(t->+60), +60 = 0;
// PtrVector_Clear / Nop(t), SmallFree(t, 80).
DECOMP(00299F74, Owner_DeleteThing_00299F74) {
    const u32 t = arg<u32>(ctx, 1);
    if (t == 0) return;
    const u32 a = load<u32>(t + 24);
    if (a != 0 && static_cast<s16>(load<u16>(a + 178)) > 0) VCall(ctx, a, 40, {a, 0});
    const u32 b = load<u32>(t + 52);
    if (b != 0 && (load<u32>(b + 172) & 0x40000u) != 0) VCall(ctx, b, 40, {b, 1});
    for (u32 i = 0; i < load<u16>(t + 8); ++i) {
        const u32 p = load<u32>(t) + 8 * i;
        const u32 e = load<u32>(p);
        if (e != 0) {
            VCall(ctx, e, 8, {e});
            store<u32>(p, 0);
        }
        store<u32>(p + 4, 0);
    }
    store<u16>(t + 8, 0);
    call(ctx, 0x006C0D24u, {load<u32>(t + 56)});
    const u32 s60 = load<u32>(t + 60);
    store<u32>(t + 56, 0);
    call(ctx, 0x006C0D24u, {s60});
    store<u32>(t + 60, 0);
    call(ctx, 0x0054EC1Cu, {t});
    call(ctx, 0x0054EC18u, {t});
    call(ctx, 0x00552420u, {t, 80});
}

// T* f(A* a, B* b): SpawnFromRecord47 with rec->+4 |= 0x00080000, a
// 1136-byte 0x002F6578 object.
DECOMP(002F7BC4, Obj2F6578_Spawn_002F7BC4) { SpawnFromRecord47(ctx, 4, 0x00080000u, 1136, 0x002F6578u); }

// T* f(A* a, B* b): SpawnFromRecord47 with rec->+4 |= 0x00040000, a
// 1088-byte 0x00305A44 object.
DECOMP(003072A8, Obj305A44_Spawn_003072A8) { SpawnFromRecord47(ctx, 4, 0x00040000u, 1088, 0x00305A44u); }

// T::~T(T* o): vtables 0x00B1CAB0 / +24 0x00B1CB10, Obj_ResetBody_002AD030(o),
// PtrVector_Clear / Nop of +280, +260, +240; then the base part: vtables
// 0x00B1BE30 / +24 0x00B1200C, o removed from the registry 0x01056F1C,
// List1013B5C_Remove(o) (r3 = its result).
DECOMP(002AD2A4, VtB1CAB0_Dtor_002AD2A4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1CAB0u);
    store<u32>(o + 24, 0x00B1CB10u);
    call(ctx, 0x002AD030u, {o});
    for (u32 off : {280u, 260u, 240u}) {
        call(ctx, 0x0054EC1Cu, {o + off});
        call(ctx, 0x0054EC18u, {o + off});
    }
    store<u32>(o + 24, 0x00B1200Cu);
    store<u32>(o, 0x00B1BE30u);
    TUnregister(o, 0x01056F1Cu, false);
    call(ctx, 0x000D6940u, {o});
}

// void f(T* o, f32 dt): when u8 o->+2812 is set, for each element e of
// the u16-counted pointer array o->+2772 (count +2780): +88 += +92, +68 +=
// +88 * dt (fmadds), +84 -= dt, u8 +103 = (s32)(clamp(+84 / +80 (old),
// 0..255) * 255) (fsel against the .text 255.0 / 0.0 at 0x0033DE18; the
// quotient taken before the subtraction); +84 <= 0 -> +84 = 0 and the
// element is freed (SmallFree(e, 144)) and replaced by the last one (the
// count decremented, the same index looked at again). r3 = the count
// (0: the byte flag), f1 as the original leaves it.
DECOMP(0033DE30, Fader_Update_0033DE30) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (load<u8>(o + 2812) == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 K = 0x0033DE18u;
    const f64 k255 = Lfs47(K), z = Lfs47(K + 4), d255 = load<f64>(K + 8), d0 = load<f64>(K + 16);
    f64 f1 = dt;
    for (s32 i = 0; i < static_cast<s32>(load<u16>(o + 2780)); ++i) {
        const u32 p = load<u32>(o + 2772) + 4 * static_cast<u32>(i);
        const u32 e = load<u32>(p);
        const f64 s = F45(Lfs47(e + 88) + Lfs47(e + 92));
        store<u32>(e + 88, W47(s));
        store<u32>(e + 68, W47(Fma45(s, dt, Lfs47(e + 68))));
        const f64 a = Lfs47(e + 84), b = Lfs47(e + 80);
        const f64 q = F45(a / b);
        const f64 n84 = F45(a - dt);
        const f64 t = F45(k255 - q);
        f64 g = F45(t >= 0.0 ? q : d255);
        g = F45(g >= 0.0 ? g : d0);
        g = F45(g * k255);
        const u32 iv = static_cast<u32>(rt::fctiw(g, true));
        store<u32>(e + 84, W47(n84));
        store<u8>(e + 103, static_cast<u8>(iv));
        if (n84 <= z) store<u32>(e + 84, 0);
        const u32 e2 = load<u32>(p);
        f1 = Lfs47(e2 + 84);
        if (f1 <= z) {
            ctx.fpr[1] = f1;
            call(ctx, 0x00552420u, {e2, 144});
            f1 = ctx.fpr[1];  // as the callee leaves it
            const u32 cnt = static_cast<u32>(load<u16>(o + 2780)) - 1;
            store<u16>(o + 2780, static_cast<u16>(cnt));
            if (static_cast<u32>(i) != (cnt & 0xFFFFu)) {
                const u32 arr = load<u32>(o + 2772);
                store<u32>(arr + 4 * static_cast<u32>(i), load<u32>(arr + ((cnt << 2) & 0x3FFFCu)));
            }
            --i;
        }
    }
    ctx.fpr[1] = f1;
    ret(ctx, static_cast<u32>(load<u16>(o + 2780)));
}

// The release of a pooled object (+12) through its allocator a: when p
// = o->+pOff is set, x = a->+12 ->vslot 8(x, p, a->+4) (r3 = its result,
// else 0).
static bool PoolRelease47(PPUContext& ctx, u32 o, u32 pOff, u32 aOff) {
    const u32 p = load<u32>(o + pOff);
    if (p == 0) return false;
    const u32 a = load<u32>(o + aOff);
    const u32 x = load<u32>(a + 12);
    VCall(ctx, x, 8, {x, p, load<u32>(a + 4)});
    return true;
}

// T::~T(T* o): the global *(0x00F657FC) = 0; PtrVector_Clear / Nop(o +
// 92); when o->+76 is set, o->+80 ->vslot 8(.., o->+76, u16 +68 * (u16
// +74 >> 1)); Array_Destruct(o + 68); PtrVector_Clear / Nop(o + 48); then
// the three pooled blocks (+32 via +36, +16 via +20, +0 via +4) released
// (PoolRelease47). r3 = the last release's result or 0.
DECOMP(003A01E4, PoolOwner_Dtor_003A01E4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(0x00F657FCu, 0);
    call(ctx, 0x0054EC1Cu, {o + 92});
    call(ctx, 0x0054EC18u, {o + 92});
    const u32 n = static_cast<u32>(load<u16>(o + 68)) * ((static_cast<u32>(load<u16>(o + 74)) >> 1) & 0x7FFFu);
    if (load<u32>(o + 76) != 0) {
        const u32 a = load<u32>(o + 80);
        VCall(ctx, a, 8, {a, load<u32>(o + 76), n});
    }
    call(ctx, 0x0054EC94u, {o + 68});
    call(ctx, 0x0054EC1Cu, {o + 48});
    call(ctx, 0x0054EC18u, {o + 48});
    PoolRelease47(ctx, o, 32, 36);
    PoolRelease47(ctx, o, 16, 20);
    if (!PoolRelease47(ctx, o, 0, 4)) ret(ctx, 0u);
}

// u32 f(Actor* o): k = o->vslot 32(o, 0); with k 7 and u8 o->+159 clear,
// or with k 1 / 2 and the f32 o->+172 at least the threshold,
// o->vslot 28(o, 0, 8) and u8 +181 = 1 (r3 = 1; else r3 = o's vtable).
// The threshold (also left in f1) picks one of the globals 0x010876F8 /
// 0x010876A8 / 0x01087658 by the ratio q (o->+104's +348 / +344 when that
// has +192 bit 2, else the .text constant 0x003B0354) against the .text
// limits 0x003B0350 / 0x003B0358 (q below the first / the second / neither).
DECOMP(003B0360, Actor_TryReact_003B0360) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 k = static_cast<s32>(VCall(ctx, o, 32, {o, 0}));
    const u32 vt = load<u32>(o);
    if (k == 7 && load<u8>(o + 159) == 0) {
        VCall(ctx, o, 28, {o, 0, 8});
        store<u8>(o + 181, 1);
        ret(ctx, 1u);
        return;
    }
    const u32 p = load<u32>(o + 104);
    const f64 q = (p != 0 && (load<u32>(p + 192) & 0x2u) != 0) ? F45(Lfs47(p + 348) / Lfs47(p + 344)) : Lfs47(0x003B0354u);
    f64 thr;
    if (q < Lfs47(0x003B0350u)) thr = Lfs47(0x010876F8u);
    else if (q < Lfs47(0x003B0358u)) thr = Lfs47(0x010876A8u);
    else thr = Lfs47(0x01087658u);
    ctx.fpr[1] = thr;
    if (Lfs47(o + 172) >= thr && (k == 1 || k == 2)) {
        VCall(ctx, o, 28, {o, 0, 8});
        store<u8>(o + 181, 1);
        ret(ctx, 1u);
        return;
    }
    ret(ctx, vt);
}

// u32 f(T* o): for each 16-byte entry of the u16-counted array o->+80
// (count +88) with an animator a (+0) and a key k (+8): when a->vslot
// 60(a) gives a track t, Anim_Restart2775A8(t, k, 0, 0, -1, -, 0, 0, 0, 0,
// 0, -1; f1..f4 = C1, C0, C1, C1), else a->vslot 200(a, k, 0, 0, -, -, 0,
// -, 0; f1..f3 = C1, C0, C1) (C0 / C1 the .text pair 0x0043596C); a
// result clears bit 0x2000 of the entry's animator +172. r3 = the count.
DECOMP(00435978, AnimSet_Restart_00435978) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 c0 = Lfs47(0x0043596Cu), c1 = Lfs47(0x00435970u);
    for (u32 i = 0; i < load<u16>(o + 88); ++i) {
        const u32 ent = load<u32>(o + 80) + 16 * i;
        const u32 a = load<u32>(ent);
        if (a == 0) continue;
        const u32 k = load<u32>(ent + 8);
        if (k == 0) continue;
        const u32 t = static_cast<u32>(VCall(ctx, a, 60, {a}));
        u32 r;
        if (t != 0) {
            // r8 as the virtual call leaves it (the callee's p5 slot)
            callf(ctx, 0x002775A8u, {t, k, 0, 0, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {c1, c0, c1, c1});
            r = static_cast<u32>(ctx.gpr[3]);
        } else {
            const u32 x = load<u32>(load<u32>(o + 80) + 16 * i);
            const u32 opd = load<u32>(load<u32>(x) + 200);
            ctx.fpr[1] = c1;
            ctx.fpr[2] = c0;
            ctx.fpr[3] = c1;
            // r10 as the previous virtual call leaves it
            r = static_cast<u32>(call_ptr(ctx, opd, {x, k, 0, 0, opd, load<u32>(opd), 0, ctx.gpr[10], 0}));
        }
        if (r != 0) {
            const u32 y = load<u32>(load<u32>(o + 80) + 16 * i);
            store<u32>(y + 172, load<u32>(y + 172) & ~0x2000u);
        }
    }
    ret(ctx, static_cast<u32>(load<u16>(o + 88)));
}

// void f(Handle* h): releases the sound s = h->+32 when its count +108
// drops to 0: unlinked from the lists of 0x010ACC40 (prev +116 / next
// +120; head +8196 for the free kind (u8 +99 clear), +8200 / +8208 and
// tail +8204 by u8 +96 otherwise), an active emitter (u8 +97) stopped
// first (Emitter_Stop(s->+4, 0)), u8 +97 / +99 cleared, then
// Sound_Free445390(0x010ACC40, s) - unless u8 +98 is set. Always h->+32
// = 0 and u8 h->+28 = 0 (r3 = 0).
DECOMP(00446D50, SoundHandle_Release_00446D50) {
    const u32 h = arg<u32>(ctx, 0);
    const u64 r5 = ctx.gpr[5];
    constexpr u32 L = 0x010ACC40u;
    const u32 s = load<u32>(h + 32);
    if (s != 0 && load<u32>(s + 108) != 0) {
        const u32 n = load<u32>(s + 108) - 1;
        store<u32>(s + 108, n);
        if (n == 0) {
            if (load<u8>(s + 99) == 0) {
                if (const u32 pv = load<u32>(s + 116); pv != 0) store<u32>(pv + 120, load<u32>(s + 120));
                else store<u32>(L + 8196, load<u32>(s + 120));
                if (const u32 nx = load<u32>(s + 120); nx != 0) store<u32>(nx + 116, load<u32>(s + 116));
                call(ctx, 0x00445390u, {L, s, r5});  // r5 untouched on this path
            } else if (load<u8>(s + 98) == 0) {
                if (load<u8>(s + 97) != 0) call(ctx, 0x00443424u, {load<u32>(s + 4), 0});
                if (const u32 pv = load<u32>(s + 116); pv != 0) store<u32>(pv + 120, load<u32>(s + 120));
                else if (load<u8>(s + 96) != 0) store<u32>(L + 8200, load<u32>(s + 120));
                else store<u32>(L + 8208, load<u32>(s + 120));
                if (const u32 nx = load<u32>(s + 120); nx != 0) store<u32>(nx + 116, load<u32>(s + 116));
                else if (load<u8>(s + 96) != 0) store<u32>(L + 8204, load<u32>(s + 116));
                store<u8>(s + 97, 0);
                store<u8>(s + 99, 0);
                call(ctx, 0x00445390u, {L, s, 0});
            }
        }
    }
    store<u32>(h + 32, 0);
    store<u8>(h + 28, 0);
    ret(ctx, 0u);
}

// u32 f(Owner* o, const char* name) (name 0 -> nothing, r3 = o): the
// definition e = PointMap_Find(*(0x00F8F154)->+16, String_HashNoCase(name))
// (missing: String_Format(0x00AE52E0, name, o->+4)); m = a 64-byte item
// (Mem_Alloc(64, 0, 16); Item_InitFromSrc_004555F4(m, {o, 0, e})) added to
// *(0x00F65BE8) (Arr_AddEntry_00454CAC); then each child c of e (+272,
// next +268) of kind 12 ((+4 >> 10) & 63) whose key +204 maps (SortedMap_
// Find733F8C(*(0x00F65BE8) + 4)) to a slot x with no owner (+8): x->+8 =
// m, pushed on m's list (+72 = the old head +32, +76 = the head's +76 + 1
// or 0, +32 = x, +36 += 1; a failed alloc leaves m 0, unchecked). r3 as
// the original leaves it.
DECOMP(00455728, Item_Spawn_00455728) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    if (name == 0) return;
    Stack fr(ctx, 32);  // sp+112 key, sp+116 hash, sp+120 the 12-byte source record
    const u32 sp = fr.addr() - 112;
    store<u32>(sp + 116, static_cast<u32>(call(ctx, 0x0056A710u, {name})));
    const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), sp + 116})));
    if (e == 0) {
        call(ctx, 0x005534D0u, {0x00AE52E0u, name, load<u32>(o + 4)});
        return;
    }
    store<u32>(sp + 124, 0);
    store<u32>(sp + 128, e);
    store<u32>(sp + 120, o);
    const u32 m0 = static_cast<u32>(call(ctx, 0x0054ED38u, {64, 0, 16}));
    u32 m = 0;
    if (m0 != 0) {
        call(ctx, 0x004555F4u, {m0, sp + 120});
        m = m0;
    }
    u32 r3 = static_cast<u32>(call(ctx, 0x00454CACu, {load<u32>(0x00F65BE8u), m}));
    for (u32 c = load<u32>(e + 272); c != 0; c = load<u32>(c + 268)) {
        r3 = (load<u32>(c + 4) >> 10) & 0x3Fu;
        if (r3 != 12) continue;
        store<u32>(sp + 112, load<u32>(c + 204));
        r3 = static_cast<u32>(call(ctx, 0x00733F8Cu, {load<u32>(0x00F65BE8u) + 4, sp + 112}));
        const u32 x = load<u32>(r3);
        if (x == 0) continue;
        r3 = load<u32>(x + 8);
        if (r3 != 0) continue;
        store<u32>(x + 8, m);
        store<u32>(x + 72, load<u32>(m + 32));
        const u32 hd = load<u32>(m + 32);
        store<u32>(x + 76, hd != 0 ? load<u32>(hd + 76) + 1 : 0u);
        const u32 cnt = load<u32>(m + 36);
        store<u32>(m + 32, x);
        r3 = cnt + 1;
        store<u32>(m + 36, r3);
    }
    ret(ctx, r3);
}

// T* Clone(const T* src): a 44-byte node set (SmallAlloc(44); u16 +4 = 1,
// vtable 0x00B21EF8, PtrVector_Construct(+20) with u16 +26 = 4, +8 / +40 =
// 0; a failed alloc is not checked: the copy then works on address 0):
// src's u16-counted pointer vector +20 (count +28) appended (grown with
// PtrVector_Reserve(+20, 2 * capacity) when full; the high-water u16 +30
// raised); when +40 is still 0, +40 = Node_FindByKey_004D54FC(element,
// +40) for every element in turn; +8 = src->+8. r3 = the copy.
DECOMP(004BFE6C, NodeSet_Clone_004BFE6C) {
    const u32 src = arg<u32>(ctx, 0);
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {44}));
    if (m != 0) {
        store<u16>(m + 4, 1);
        store<u32>(m, 0x00B21EF8u);
        call(ctx, 0x005516E0u, {m + 20});
        store<u32>(m + 8, 0);
        store<u32>(m + 40, 0);
        store<u16>(m + 26, 4);
    }
    for (u32 i = 0; i < load<u16>(src + 28); ++i) {
        const u32 e = load<u32>(src + 20) + 4 * i;
        const u32 cap = load<u16>(m + 24);
        if (load<u16>(m + 28) == cap) call(ctx, 0x00551718u, {m + 20, cap << 1});
        store<u32>(load<u32>(m + 20) + 4 * static_cast<u32>(load<u16>(m + 28)), load<u32>(e));
        const u32 n1 = static_cast<u32>(load<u16>(m + 28)) + 1;
        const u32 hi = load<u16>(m + 30);
        store<u16>(m + 28, static_cast<u16>(n1));
        store<u16>(m + 30, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFFu) ? hi : n1));
    }
    if (load<u32>(m + 40) == 0) {
        u32 r = 0;
        for (u32 j = 0; j < load<u16>(m + 28); ++j) {
            r = static_cast<u32>(call(ctx, 0x004D54FCu, {load<u32>(load<u32>(m + 20) + 4 * j), r}));
            store<u32>(m + 40, r);
        }
    }
    store<u32>(m + 8, load<u32>(src + 8));
    ret(ctx, m);
}

// The 20-byte pointer vector of 004D2BFC (0 when k is 0 or Mem_Alloc(20,
// 0, 16) fails): PtrVector_Construct, u16 +6 = 4, PtrVector_Init(v, k, 0, 0).
static u32 NewPtrVec47(PPUContext& ctx, u32 k) {
    if (k == 0) return 0;
    const u32 v = static_cast<u32>(call(ctx, 0x0054ED38u, {20, 0, 16}));
    if (v == 0) return 0;
    call(ctx, 0x005516E0u, {v});
    store<u16>(v + 6, 4);
    call(ctx, 0x00551598u, {v, k, 0, 0});
    return v;
}

// T::T(T* o, a, n, k): +4 = 0, vtable 0x00B225D0, the vector +8
// (PtrVector_Construct, u16 +14 = 4, PtrVector_Init(+8, n or 2, 0, 0)),
// +28 / +32 two more vectors of capacity k (NewPtrVec47), +36 = a, +44 /
// +48 = 0, bits 0x80 / 0x40 of u8 +40 cleared. r3 = +32.
DECOMP(004D2BFC, NodeGraph_Construct_004D2BFC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2), k = arg<u32>(ctx, 3);
    store<u32>(o + 4, 0);
    store<u32>(o, 0x00B225D0u);
    const u32 cap = n != 0 ? n : 2u;
    call(ctx, 0x005516E0u, {o + 8});
    store<u16>(o + 14, 4);
    call(ctx, 0x00551598u, {o + 8, cap, 0, 0});
    store<u32>(o + 28, NewPtrVec47(ctx, k));
    const u32 v2 = NewPtrVec47(ctx, k);
    const u32 b40 = load<u8>(o + 40) & 0x3Fu;
    store<u32>(o + 32, v2);
    store<u32>(o + 36, a);
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0);
    store<u8>(o + 40, static_cast<u8>(b40));
    ret(ctx, v2);
}

// u32 f(Pool* o, u32 n): +0..+20 = 0, +24 = n, +28 = n 16-byte entries
// (Mem_Alloc(16 n, 0, 16), +8 of each cleared; none for n 0), u8 +32 = 1;
// with n: each entry i linked (+0 = i - 1, +4 = i + 1, bit 0x80000000 of
// +12 set and bits 0x7FFF8000 cleared), the first's +0 and the last's +4
// = -1 (unchecked: a failed alloc writes at 0), +8 / +12 = -1, +0 = 0, +4
// = count - 1 (-1 for a count below 2), +16 = 0, +20 = the count. r3 = +4
// (0 for n 0).
DECOMP(00767484, IndexPool_Init_00767484) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 24; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 24, n);
    u32 p = 0;
    if (n != 0) {
        p = static_cast<u32>(call(ctx, 0x0054ED38u, {n << 4, 0, 16}));
        if (p != 0) {
            u32 q = p;
            for (s32 k = static_cast<s32>(n); k > 0; --k, q += 16) store<u32>(q + 8, 0);
        }
    }
    store<u32>(o + 28, p);
    store<u8>(o + 32, 1);
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    s32 cnt = static_cast<s32>(load<u32>(o + 24));
    u32 a = p;
    if (cnt > 0) {
        s32 i = 0;
        do {
            const u32 e = load<u32>(o + 28) + 16 * static_cast<u32>(i);
            store<u32>(e, static_cast<u32>(i - 1));
            store<u32>(e + 4, static_cast<u32>(i + 1));
            store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) | 0x80u));
            const u32 e2 = load<u32>(o + 28) + 16 * static_cast<u32>(i);
            store<u32>(e2 + 12, load<u32>(e2 + 12) & ~0x7FFF8000u);
            ++i;
            cnt = static_cast<s32>(load<u32>(o + 24));
        } while (i < cnt);
        a = load<u32>(o + 28);
    }
    store<u32>(a, 0xFFFFFFFFu);
    store<u32>(load<u32>(o + 28) + (static_cast<u32>(cnt) << 4) - 12, 0xFFFFFFFFu);
    const u32 c2 = load<u32>(o + 24);
    store<u32>(o + 8, 0xFFFFFFFFu);
    store<u32>(o + 12, 0xFFFFFFFFu);
    store<u32>(o + 0, 0);
    const u32 r = static_cast<s32>(c2) > 1 ? c2 - 1 : 0xFFFFFFFFu;
    store<u32>(o + 4, r);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, c2);
    ret(ctx, r);
}

// u32 f(Handle* h, a, f32 x): h->+4 += 1; with h->+0 set, 0x0077C760(h, a,
// h->+0, h; f1 = x) (r3 = its result). Otherwise a 20-byte node from the
// shared free list *(0x00F8F0F0) (created on first use: 60 bytes,
// Mem_Alloc(60, 0, 16); fields cleared, bit 0x8000 of u16 +54 cleared,
// vtable 0x00B20C90, +56 = 0x00ADF00A, FreeList_Init(m, 20, 64, 0, 0); the
// global set even when that failed) via its vslot 12: u16 +0 = 0, u16 +2
// = 1, +4 = a, +8 = x, +12 / +16 = 0; h->+0 = the node (r3 = 0).
DECOMP(0077CBE0, NodeRef_Acquire_0077CBE0) {
    const u32 h = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const f64 x = farg(ctx, 0);
    const u32 h0 = load<u32>(h);
    store<u32>(h + 4, load<u32>(h + 4) + 1);
    if (h0 != 0) {
        callf(ctx, 0x0077C760u, {h, a, h0, h}, {x});
        return;
    }
    u32 pool = load<u32>(0x00F8F0F0u);
    if (pool == 0) {
        const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {60, 0, 16}));
        if (m != 0) {
            store<u32>(m + 8, 0);
            store<u32>(m + 4, 0);
            store<u32>(m + 12, 0);
            store<u32>(m + 20, 0);
            store<u32>(m + 16, 0);
            store<u32>(m + 24, 0);
            const u32 h54 = load<u16>(m + 54) & ~0x8000u;
            store<u32>(m + 32, 0);
            store<u32>(m + 28, 0);
            store<u32>(m + 36, 0);
            store<u32>(m + 40, 0);
            store<u32>(m + 44, 0);
            store<u32>(m + 48, 0);
            store<u16>(m + 52, 0);
            store<u16>(m + 54, static_cast<u16>(h54));
            store<u32>(m, 0x00B20C90u);
            store<u32>(m + 56, 0x00ADF00Au);
            call(ctx, 0x00551A58u, {m, 20, 64, 0, 0});
        }
        pool = m;
        store<u32>(0x00F8F0F0u, m);
    }
    const u32 nd = static_cast<u32>(VCall(ctx, pool, 12, {pool}));
    if (nd != 0) {
        store<u16>(nd, 0);
        store<u16>(nd + 2, 1);
        store<u32>(nd + 4, a);
        store<u32>(nd + 8, W47(x));
        store<u32>(nd + 12, 0);
        store<u32>(nd + 16, 0);
    }
    store<u32>(h, nd);
    ret(ctx, 0u);
}
