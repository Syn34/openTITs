// Game code, part 32: the small-function sweep continued from the last 57
// instruction functions (size order; obj31*.cpp did 53-57). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj32.h"

using namespace sly4;
using namespace sly4::obj32;

// ---------------------------------------------------------------- sweep: 57 instructions
// void f(T* o, u8 on): with the controller c = o->+424: its +244 child k
// gets k->vt[204](k, o->+216 + 448); Obj_Reset121FA8(o, 0, 0, 0, 0, 1);
// with no child and on, c->vt[40](c, 1). r3 = o->+216 for no controller.
DECOMP(001244A4, Obj_ResetCtl_001244A4) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 c = load<u32>(o + 424);
    const u32 w = load<u32>(o + 216);
    if (c == 0) {
        ctx.gpr[3] = w;
        return;
    }
    const u32 k = load<u32>(c + 244);
    if (k != 0) VCall(ctx, k, 204, {k, w + 448});
    ctx.gpr[3] = call(ctx, 0x00121FA8, {o, 0, 0, 0, 0, 1});
    if (k != 0) return;
    ctx.gpr[3] = on;
    if (on == 0) return;
    ctx.gpr[3] = VCall(ctx, c, 40, {c, 1});
}

// void f(T* s, T* d): Obj_CopyQueryEx_000DFBFC(s, d), then the fields
// s+240..+328 into d+208..+292 / d+200 (raw words; u8s at +256 / +288).
// r3 = s->+328, f1 = s->+316.
DECOMP(00191024, Obj_CopyQueryFields_00191024) {
    const u32 s = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {s, d});
    CopyW32(d + 208, s + 240);
    CopyW32(d + 212, s + 244);
    CopyW32(d + 216, s + 248);
    CopyW32(d + 224, s + 256);
    CopyW32(d + 228, s + 260);
    CopyW32(d + 232, s + 264);
    CopyW32(d + 240, s + 272);
    CopyW32(d + 244, s + 276);
    CopyW32(d + 248, s + 280);
    CopyW32(d + 252, s + 284);
    CopyW32(d + 268, s + 300);
    store<u32>(d + 260, load<u32>(s + 292));
    CopyW32(d + 272, s + 304);
    store<u8>(d + 256, load<u8>(s + 288));
    store<u32>(d + 264, load<u32>(s + 296));
    CopyW32(d + 276, s + 308);
    CopyW32(d + 280, s + 312);
    const u32 b = load<u8>(s + 320);
    CopyW32(d + 284, s + 316);
    const u32 w = load<u32>(s + 324);
    store<u8>(d + 288, static_cast<u8>(b));
    store<u32>(d + 292, w);
    const u32 r = load<u32>(s + 328);
    store<u32>(d + 200, r);
    ctx.gpr[3] = r;
    ctx.fpr[1] = Lfs32(s + 316);
}

// void f(T* o): the screen point of o (0x001DFC58 into a local) projected
// by the world's view (g = *0x00F65CA0: View_ProjectPoint(g->+0x128168,
// out, pt), then Call_0052296C(g->+0x12816C, out2, {out.x, out.y})); when
// o->+716->+24 = p is set, p+64/+68 = out2.x/y (+72 rewritten as is) and
// WorldObj_MarkDirty(p).
DECOMP(001DFEA0, Obj_ProjectToScreen_001DFEA0) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 80);
    const u32 pt = frame.addr(), out = pt + 16, out2 = pt + 32, xy = pt + 48;  // sp+112 / +128 / +144 / +160
    call(ctx, 0x001DFC58, {pt, o});
    const u32 g = load<u32>(0x00F65CA0);
    const u32 v2 = load<u32>(g + 0x12816C), v1 = load<u32>(g + 0x128168);
    call(ctx, 0x00522750, {v1, out, pt});
    const f64 x = Lfs32(out), y = Lfs32(out + 4);
    CopyW32(xy, out);
    CopyW32(xy + 4, out + 4);
    ctx.fpr[1] = x;
    ctx.fpr[2] = y;
    ctx.gpr[3] = call(ctx, 0x0052296C, {v2, out2, xy});
    u32 p = load<u32>(o + 716);
    if (p != 0) p = load<u32>(p + 24);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 z = load<u32>(p + 72);
    ctx.fpr[1] = Lfs32(out2);
    ctx.fpr[2] = Lfs32(out2 + 4);
    ctx.fpr[3] = std::bit_cast<f32>(z);
    CopyW32(p + 64, out2);
    CopyW32(p + 68, out2 + 4);
    store<u32>(p + 72, z);
    ctx.gpr[3] = call(ctx, 0x0057D000, {p});
}

// Deleting ~T() (vt 0x00B19FB0, 208 bytes): Mem_FreeFrom of the buffers
// +180/+184/+188/+192 (each then 0), vt 0x00B19510, Obj_BaseDtor_000C2DA0,
// SmallFree(o, 208). Nothing for a null o.
DECOMP(00217374, Dtor_B19FB0_00217374) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B19FB0);
    FreeField32(ctx, o, 180);
    FreeField32(ctx, o, 184);
    FreeField32(ctx, o, 188);
    FreeField32(ctx, o, 192);
    store<u32>(o, 0x00B19510);
    call(ctx, 0x000C2DA0, {o});
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {o, 208});
}

// T* f(Reader* r, a, b): a new 464-byte object (0x00746148(p, b, 0, 0, 1,
// 0, 1, 0; stack 0, 0), vt 0x00B1AA20, +188/+192 = 0, the u8s +196/+197/
// +204 = 0, flag 0x40000000 in +4), then Reader_Read_0022083C(r, a, p)
// (also for a failed allocation, with p = 0). r3 = p.
DECOMP(00220A64, Obj_ReadNew_00220A64) {
    const u32 r = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    u32 p = static_cast<u32>(call(ctx, 0x0055238C, {464}));
    if (p != 0) {
        call(ctx, 0x00746148, {p, b, 0, 0, 1, 0, 1, 0, 0, 0});
        const u32 f = load<u32>(p + 4);
        store<u32>(p + 188, 0);
        store<u32>(p + 192, 0);
        store<u8>(p + 196, 0);
        store<u8>(p + 197, 0);
        store<u32>(p, 0x00B1AA20);
        store<u8>(p + 204, 0);
        store<u32>(p + 4, f | 0x40000000u);
    }
    call(ctx, 0x0022083C, {r, a, p});
    ctx.gpr[3] = p;
}

// void f(T* o, u8 k): o->+2660 = 0; each 32-byte record of o->+2572 (u16
// count +2580, re-read after every call) with a +16 object e gets
// e->vt[40](e, 0); the count then 0 (when it was not); Recs_Reset325A30(o,
// k).
DECOMP(0033FB84, Recs_Release_0033FB84) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1) & 0xFFu;
    store<u32>(o + 2660, 0);
    u32 n = load<u16>(o + 2580);
    if (n != 0) {
        u32 i = 0;
        do {
            const u32 e = load<u32>(load<u32>(o + 2572) + 32 * i + 16);
            if (e != 0) {
                VCall(ctx, e, 40, {e, 0});
                n = load<u16>(o + 2580);
            }
            ++i;
        } while (i < n);
        store<u16>(o + 2580, 0);
    }
    ctx.gpr[3] = call(ctx, 0x00325A30, {o, k});
}

// s32 f(Stream* s, u8* buf, s32 n): when s->vt[16](&ok, s) gives ok, n >=
// 1 and the reader s->+8 is set: FileReader_Read(s->+8, buf, min(n,
// remaining)) (sign-extended), or 0 with the u8 s->+12 = 0 when
// ChunkReader_Remaining(s->+8) is negative. Otherwise 0.
DECOMP(003555BC, Stream_Read_003555BC) {
    const u32 s = arg<u32>(ctx, 0), buf = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 ok = frame.addr();  // sp+112
    VCall(ctx, s, 16, {ok, s});
    ctx.gpr[3] = 0;
    if (load<u8>(ok) == 0 || static_cast<s32>(n) < 1) return;
    const u32 rd = load<u32>(s + 8);
    if (rd == 0) return;
    const u32 rem = static_cast<u32>(call(ctx, 0x0056424C, {rd}));
    const u32 rd2 = load<u32>(s + 8);
    if (static_cast<s32>(rem) < 0) {
        if (static_cast<s32>(n) > 0) store<u8>(s + 12, 0);
        ctx.gpr[3] = 0;
        return;
    }
    const u32 len = n < rem ? n : rem;
    const u32 got = static_cast<u32>(call(ctx, 0x00563768, {rd2, buf, len}));
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(static_cast<s32>(got)));
}

// Deleting ~T() (vt 0x00B204E8): the global 0x00F65844 = 0;
// World_ListRemove_0046B7A4(*0x00F65CA0, 0, 0x00F38458, o, 0); the +508
// object (when set) goes through 0x002EF648(*0x00F64C38, it) and gets flag
// 0x1000 in its u16 +176; +260 / +116 cleared when set;
// Dtor_B219D0_00490B34(o), Mem_FreeFrom(o, 0). Nothing for a null o.
DECOMP(003D1F1C, Dtor_B204E8_003D1F1C) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B204E8);
    const u32 g = load<u32>(0x00F65CA0);
    store<u32>(0x00F65844, 0);
    call(ctx, 0x0046B7A4, {g, 0, 0x00F38458u, o, 0});
    if (load<u32>(o + 508) != 0) {
        call(ctx, 0x002EF648, {load<u32>(0x00F64C38), load<u32>(o + 508)});
        const u32 p = load<u32>(o + 508);
        store<u16>(p + 176, static_cast<u16>(load<u16>(p + 176) | 0x1000u));
    }
    if (load<u32>(o + 260) != 0) store<u32>(o + 260, 0);
    if (load<u32>(o + 116) != 0) store<u32>(o + 116, 0);
    call(ctx, 0x00490B34, {o});
    ctx.gpr[3] = call(ctx, o32fn::Mem_FreeFrom, {o, 0});
}

// T::T(T* o, a, const Desc* s, u8 k): Obj_InitVt_B20CC8(o, k), vt
// 0x00B20D58, +12 = a, then s's words into o+16.. (s+4 twice: +20 and
// +24; s+16 to +32). f1 = s->+80.
DECOMP(0042D72C, Obj_Ctor_B20D58_0042D72C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), s = arg<u32>(ctx, 2), k = arg<u32>(ctx, 3) & 0xFFu;
    call(ctx, 0x0042C9D0, {o, k});
    store<u32>(o + 12, a);
    CopyW32(o + 16, s);
    CopyW32(o + 20, s + 4);
    CopyW32(o + 24, s + 4);
    CopyW32(o + 28, s + 12);
    CopyW32(o + 36, s + 20);
    CopyW32(o + 40, s + 24);
    CopyW32(o + 44, s + 28);
    CopyW32(o + 48, s + 32);
    CopyW32(o + 52, s + 36);
    const u32 w40 = load<u32>(s + 40);
    store<u32>(o, 0x00B20D58);
    store<u32>(o + 56, w40);
    CopyW32(o + 60, s + 44);
    const u32 w16 = load<u32>(s + 16);
    store<u32>(o + 32, w16);
    CopyW32(o + 64, s + 48);
    CopyW32(o + 80, s + 64);
    CopyW32(o + 84, s + 68);
    CopyW32(o + 88, s + 72);
    CopyW32(o + 96, s + 80);
    ctx.gpr[3] = w16;
    ctx.fpr[1] = Lfs32(s + 80);
}

// void f(Msg* m): for the handle 0xFD81, Obj_BroadcastChildren_0001B3BC(
// *0x00F557F0, m->+16, +20, +24; f1 = +28); else the object
// Obj_FindByHandle(m->+12, u8 m->+36) goes to m->+32 and its +244 c gets
// c->vt[164](c, +16, +20, +24; f1 = +28). Then Flag_Set8(m).
DECOMP(00437ACC, Msg_Dispatch_00437ACC) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 h = load<u32>(m + 12);
    if (h == 0xFD81u) {
        callf(ctx, 0x0001B3BC, {load<u32>(0x00F557F0), load<u32>(m + 16), load<u32>(m + 20), load<u32>(m + 24)}, {Lfs32(m + 28)});
    } else {
        const u32 p = static_cast<u32>(call(ctx, 0x0042C2B0, {h, load<u8>(m + 36)}));
        store<u32>(m + 32, p);
        const u32 a = load<u32>(m + 16), b = load<u32>(m + 20), c3 = load<u32>(m + 24);
        ctx.fpr[1] = Lfs32(m + 28);
        if (p != 0) {
            const u32 c = load<u32>(p + 244);
            if (c != 0) VCall(ctx, c, 164, {c, a, b, c3});
        }
    }
    ctx.gpr[3] = call(ctx, 0x0042CA60, {m});
}

// void f(T* o): o->+12 = 0, then by the two level indices (*(*0x010AB038
// + 64) / *(*0x010AB050 + 64)) into the f32 table o->+48->+12: when the
// first value v1 > K0 (0x00455C6C), +12 = K0 + v1; when the second v2 >
// K0, +12 = f1 + v2 * (rand * K1 (0x00455C70)), with the inlined random
// step. f1 = the last value.
DECOMP(00455C78, Obj_RollValue_00455C78) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k0 = Lfs32(0x00455C6C);
    store<u32>(o + 12, 0);
    const u32 tab = load<u32>(load<u32>(o + 48) + 12);
    f64 f1 = k0;
    const f64 v1 = Lfs32(tab + (load<u32>(load<u32>(0x010AB038) + 64) << 2));
    if (v1 > f1) {
        f1 = static_cast<f32>(k0 + v1);
        Stfs32(o + 12, f1);
    }
    const f64 v2 = Lfs32(tab + (load<u32>(load<u32>(0x010AB050) + 64) << 2));
    if (v2 > k0) {
        const f64 k1 = Lfs32(0x00455C70);
        const u64 old = RandStep28();
        const f64 r = static_cast<f32>(static_cast<f64>(static_cast<s64>(old)));
        const f64 f3 = static_cast<f32>(r * k1);
        f1 = static_cast<f32>(std::fma(v2, f3, f1));
        Stfs32(o + 12, f1);
    }
    ctx.fpr[1] = f1;
}

// bool f(Xml* x, name, bool (*fp)(node, u), u, u8 all): walks the children
// Xml_FindChildren(x, name, fp, u, all) / Xml_FindChildValue_004F9078(c,
// name), calling fp(c, u) on each while every result so far was true (or
// on all of them for all); r3 = 1 when there are none, else whether every
// call that ran returned true.
DECOMP(004F4494, Xml_AllChildren_004F4494) {
    const u32 x = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), fp = arg<u32>(ctx, 2), u = arg<u32>(ctx, 3), all7 = arg<u32>(ctx, 4);
    u32 c = static_cast<u32>(call(ctx, 0x004F936C, {x, name, fp, u, all7}));
    if (c == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    u32 res = 1;
    const u32 all = all7 & 0xFFu;
    while (res != 0 || all != 0) {
        if (static_cast<u32>(call_ptr(ctx, fp, {c, u, load<u32>(fp)})) == 0) res = 0;
        c = static_cast<u32>(call(ctx, 0x004F9078, {c, name}));
        if (c == 0) break;
    }
    ctx.gpr[3] = res;
}

// Deleting ~T() (vt 0x00B23050): the three arrays +12 / +20 / +24 freed
// (SmallFree with 4 * the counts t->+8->+48 / t->+28 / t->+8->+48 of the
// type t = o->+16, re-read after each free) and cleared; Mem_FreeFrom(o,
// 0). Nothing for a null o.
DECOMP(004F78DC, Dtor_B23050_004F78DC) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 t = load<u32>(o + 16);
    store<u32>(o, 0x00B23050);
    u32 n = load<u32>(load<u32>(t + 8) + 48) << 2;
    if (load<u32>(o + 12) != 0) {
        call(ctx, o32fn::SmallFree, {load<u32>(o + 12), n});
        store<u32>(o + 12, 0);
    }
    n = load<u32>(load<u32>(o + 16) + 28) << 2;
    if (load<u32>(o + 20) != 0) {
        call(ctx, o32fn::SmallFree, {load<u32>(o + 20), n});
        store<u32>(o + 20, 0);
    }
    n = load<u32>(load<u32>(load<u32>(o + 16) + 8) + 48) << 2;
    if (load<u32>(o + 24) != 0) {
        call(ctx, o32fn::SmallFree, {load<u32>(o + 24), n});
        store<u32>(o + 24, 0);
    }
    ctx.gpr[3] = call(ctx, o32fn::Mem_FreeFrom, {o, 0});
}

// void f(T* o): with p = o->+256 set: unless o->vt[124](o), p->vt[40](p,
// 0); +256 = 0; q = o->+48: q->vt[96](q, 0) and q->+164's u16 +40 gets bit
// 0x400 set and 0x1000 cleared. r3 = 0 for no p.
DECOMP(007385A0, Obj_Detach_007385A0) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = load<u32>(o + 256);
    if (ctx.gpr[3] == 0) return;
    if (static_cast<u32>(VCall(ctx, o, 124, {o})) == 0) {
        const u32 p = load<u32>(o + 256);
        VCall(ctx, p, 40, {p, 0});
    }
    const u32 q = load<u32>(o + 48);
    store<u32>(o + 256, 0);
    ctx.gpr[3] = VCall(ctx, q, 96, {q, 0});
    const u32 s = load<u32>(load<u32>(o + 48) + 164);
    if (s == 0) return;
    store<u16>(s + 40, static_cast<u16>((load<u16>(s + 40) & ~0x1000u) | 0x400u));
    ctx.gpr[3] = 0;
}

// void Quat_Lerp(Quat* out, const Quat* a, const Quat* b, f32 t): out = a
// + (b' - a) * t with b' = -b when dot(a, b) < K (0x0074BD70), single
// precision; a's components re-read after each store. f1 = out.w.
DECOMP(0074BD78, Quat_Lerp_0074BD78) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 t = farg(ctx, 0);
    f64 b0 = Lfs32(b);
    const f64 m = static_cast<f32>(Lfs32(a + 4) * Lfs32(b + 4));
    f64 d = static_cast<f32>(std::fma(Lfs32(a), b0, m));
    d = static_cast<f32>(std::fma(Lfs32(a + 8), Lfs32(b + 8), d));
    const f64 k = Lfs32(0x0074BD70);
    f64 b1 = Lfs32(b + 4), b2 = Lfs32(b + 8), b3 = Lfs32(b + 12);
    const f64 a0 = Lfs32(a), a1 = Lfs32(a + 4), a2 = Lfs32(a + 8), a3 = Lfs32(a + 12);
    d = static_cast<f32>(std::fma(Lfs32(a + 12), Lfs32(b + 12), d));
    if (d < k) {
        b0 = -b0;
        b1 = -b1;
        b2 = -b2;
        b3 = -b3;
    }
    const f64 d0 = static_cast<f32>(b0 - a0), d1 = static_cast<f32>(b1 - a1), d2 = static_cast<f32>(b2 - a2), d3 = static_cast<f32>(b3 - a3);
    Stfs32(out, static_cast<f32>(std::fma(d0, t, a0)));
    Stfs32(out + 4, static_cast<f32>(std::fma(d1, t, Lfs32(a + 4))));
    Stfs32(out + 8, static_cast<f32>(std::fma(d2, t, Lfs32(a + 8))));
    const f64 w = static_cast<f32>(std::fma(d3, t, Lfs32(a + 12)));
    Stfs32(out + 12, w);
    ctx.fpr[1] = w;
}

// ---------------------------------------------------------------- sweep: 58 instructions
// void f(T* o, a, u8 flip): 0x0062DB80(a + 4, p, q) into two locals (p/q
// swapped by flip); 0x00013E40(out, o, first 3 words of the 24-byte
// result, its last 3, o + 176); o+112..+120 = out; then o->vt[248](o).
// f1 = out.z.
DECOMP(0003B984, Obj_ApplyPose_0003B984) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), flip = arg<u32>(ctx, 2) & 0xFFu;
    Stack frame(ctx, 96);
    const u32 out = frame.addr(), v0 = out + 16, v1 = out + 32, q = out + 48, r = out + 72;  // sp+112 / +128 / +144 / +160 / +184
    if (flip != 0) call(ctx, 0x0062DB80, {a + 4, r, q});
    else call(ctx, 0x0062DB80, {a + 4, q, r});
    for (u32 k = 0; k < 3; ++k) CopyW32(v0 + 4 * k, r + 4 * k);
    for (u32 k = 0; k < 3; ++k) CopyW32(v1 + 4 * k, r + 12 + 4 * k);
    ctx.fpr[1] = Lfs32(r + 12);
    ctx.fpr[2] = Lfs32(r + 16);
    ctx.fpr[3] = Lfs32(r + 20);
    call(ctx, 0x00013E40, {out, o, v0, v1, o + 176});
    CopyW32(o + 112, out);
    CopyW32(o + 116, out + 4);
    CopyW32(o + 120, out + 8);
    ctx.fpr[1] = Lfs32(out + 8);
    ctx.gpr[3] = VCall(ctx, o, 248, {o});
}

// T* f(?, Xml* x): a new 56-byte XML element (Mem_Alloc(56, 0, 16),
// XmlElem_Ctor_0006E63C(p, x), vt 0x00B0F220) reading the hashes
// 0x040EFE5F -> +44 and 0x718F756D -> +48 and the f32 0x04A2411D -> +52
// (defaults 0). r3 = p.
DECOMP(0006EA64, XmlElem_New_0006EA64) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {56, 0, 16}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    call(ctx, 0x0006E63C, {p, x});
    store<u32>(p + 44, 0);
    store<u32>(p + 48, 0);
    store<u32>(p + 52, 0);
    store<u32>(p, 0x00B0F220);
    call(ctx, 0x004FA14C, {x, 0x040EFE5Fu, p + 44, 0});
    call(ctx, 0x004FA14C, {x, 0x718F756Du, p + 48, load<u32>(p + 48)});
    callf(ctx, 0x004F9B20, {x, 0x04A2411Du, p + 52}, {Lfs32(p + 52)});
    ctx.gpr[3] = p;
}

// Deleting ~T() (vt 0x00B0FBF8): each element e of the vector +24 (u16
// count +32) gets e->vt[8](e); the vector cleared,
// VtB11B20_DtorTrivial_004BB41C(o), Mem_FreeFrom(o, 0). Nothing for a null
// o.
DECOMP(0007ECC0, Dtor_B0FBF8_0007ECC0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B0FBF8);
    for (u32 i = 0; i < load<u16>(o + 32); ++i) {
        const u32 e = load<u32>(load<u32>(o + 24) + 4 * i);
        if (e != 0) VCall(ctx, e, 8, {e});
    }
    PvClear32(ctx, o + 24);
    call(ctx, 0x004BB41C, {o});
    ctx.gpr[3] = call(ctx, o32fn::Mem_FreeFrom, {o, 0});
}

// The enum-9 XML node of vtable 0x00B0FE78 (see XmlNewEnum9_32).
DECOMP(00082DCC, XmlEnum9_B0FE78_New_00082DCC) { XmlNewEnum9_32(ctx, 0x00B0FE78); }

// The enum-9 XML node of vtable 0x00B0FEC8 (see XmlNewEnum9_32).
DECOMP(000831A4, XmlEnum9_B0FEC8_New_000831A4) { XmlNewEnum9_32(ctx, 0x00B0FEC8); }

// T* f(?, Xml* x): a 16-byte node (Mem_Alloc(16, 0, 16), 0x004BB3B0(p),
// vt 0x00B11530) whose bit 0x80000000 of +12 is set and then replaced by
// the bool 0x8488565B (default: the bit, i.e. true). r3 = p.
DECOMP(000A0FFC, XmlBoolNode_New_000A0FFC) {
    const u32 x = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 b = frame.addr();  // sp+112
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {16, 0, 16}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    call(ctx, 0x004BB3B0, {p, x});
    const u32 w = load<u32>(p + 12) | 0x80000000u;
    store<u32>(p + 12, w);
    store<u32>(p, 0x00B11530);
    store<u8>(b, static_cast<u8>(w >> 31));
    call(ctx, 0x004F986C, {x, 0x8488565Bu, b, load<u32>(p + 12) >> 31});
    store<u32>(p + 12, (load<u32>(p + 12) & 0x7FFFFFFFu) | ((static_cast<u32>(load<u8>(b)) & 1u) << 31));
    ctx.gpr[3] = p;
}

// void f(T* o): 0x004A4780(o, t, 0x00F32A48) with t the nearer (to o+144)
// of the world's two points g->+4216 / g->+0x21CF8 (g = *0x00F65CA0; the
// first when the second is null or not strictly nearer).
DECOMP(0017A1E0, Obj_PickNearer_0017A1E0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x00F65CA0);
    u32 a = load<u32>(g + 4216);
    const u32 b = load<u32>(g + 0x21CF8);
    if (b != 0) {
        const u32 po = o + 144, pa = a + 144, pb = b + 144;
        const f64 ox = Lfs32(po), oy = Lfs32(po + 4), oz = Lfs32(po + 8);
        const f64 ay = static_cast<f32>(oy - Lfs32(pa + 4)), ax = static_cast<f32>(ox - Lfs32(pa));
        const f64 by = static_cast<f32>(oy - Lfs32(pb + 4)), bx = static_cast<f32>(ox - Lfs32(pb));
        const f64 az = static_cast<f32>(oz - Lfs32(pa + 8)), bz = static_cast<f32>(oz - Lfs32(pb + 8));
        f64 da = static_cast<f32>(ay * ay), db = static_cast<f32>(by * by);
        da = static_cast<f32>(std::fma(ax, ax, da));
        db = static_cast<f32>(std::fma(bx, bx, db));
        da = static_cast<f32>(std::fma(az, az, da));
        db = static_cast<f32>(std::fma(bz, bz, db));
        da = static_cast<f32>(std::sqrt(da));
        db = static_cast<f32>(std::sqrt(db));
        ctx.fpr[1] = db;
        ctx.fpr[2] = da;
        if (da > db) a = b;
    }
    ctx.gpr[3] = call(ctx, 0x004A4780, {o, a, 0x00F32A48u});
}

// void f(Volume* v, u8 on): 0x004AD580(v->+436, on) when set; when bit
// 0x80000000 of +412 differs from on it becomes on, then for on
// Volume_Activate20C2BC(v), else Volume_Notify112D8(*0x00F557EC, v + 424),
// Volume_Prepare20C670(v) unless bit 0x02000000 of +412, and v->vt[96](v,
// 0). r3 = +412 when unchanged.
DECOMP(0022506C, Volume_SetOn_0022506C) {
    const u32 v = arg<u32>(ctx, 0), r4 = arg<u32>(ctx, 1), on = r4 & 0xFFu;
    if (load<u32>(v + 436) != 0) call(ctx, 0x004AD580, {load<u32>(v + 436), on});
    const u32 w = load<u32>(v + 412);
    if ((w >> 31) == on) {
        ctx.gpr[3] = w;
        return;
    }
    store<u32>(v + 412, (w & 0x7FFFFFFFu) | ((r4 & 1u) << 31));
    if (on != 0) {
        ctx.gpr[3] = call(ctx, 0x0020C2BC, {v});
        return;
    }
    call(ctx, 0x000112D8, {load<u32>(0x00F557EC), v + 424});
    if ((load<u32>(v + 412) & 0x02000000u) == 0) call(ctx, 0x0020C670, {v});
    ctx.gpr[3] = VCall(ctx, v, 96, {v, 0});
}

// void f(T* o): o->+44 = lo + (hi - lo) * rand * K (lo/hi the f32s +36 /
// +40, K 0x00257F0C, the inlined random step); before that, for the kind
// +24 0xE2E902C9 the index +56 steps modulo the count +52, for 0x026F1EA5
// it steps while below the count. f1 = the value.
DECOMP(00257F10, Obj_RandRange_00257F10) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 h = static_cast<s32>(load<u32>(o + 24));
    const f64 lo = Lfs32(o + 36), hi = Lfs32(o + 40);
    if (h > static_cast<s32>(0xE2E902C9u)) {
        if (h == 0x026F1EA5) {
            const u32 n = load<u32>(o + 56) + 1;
            if (static_cast<s32>(n) < static_cast<s32>(load<u32>(o + 52))) store<u32>(o + 56, n);
        }
    } else if (h > static_cast<s32>(0xE2E902C8u)) {
        const u32 n = load<u32>(o + 56) + 1, m = load<u32>(o + 52);
        store<u32>(o + 56, n - Divw32(n, m) * m);
    }
    const f64 d = static_cast<f32>(hi - lo);
    const f64 k = Lfs32(0x00257F0C);
    const f64 r = RandF32_32();
    const f64 f3 = static_cast<f32>(r * k);
    const f64 v = static_cast<f32>(std::fma(d, f3, lo));
    Stfs32(o + 44, v);
    ctx.fpr[1] = v;
}

// void f(?, u8 on): for the flagged object p = FindFlagged_00368A48(
// *0x00F64D3C): on with bit 0x8000 of p->+24->+164 -> Obj_ModeOff368AC8(p,
// 0, 0); off without it (or no +24) -> Obj_SetMode368870(p, -, 0, 0; f1 =
// *0x0029F424); then bit 0x01000000 of p->+532 = on. r3 = 0 for no p.
DECOMP(0029F428, Flagged_SetOn_0029F428) {
    const u32 r4 = arg<u32>(ctx, 1), on = r4 & 0xFFu;
    const u32 p = static_cast<u32>(call(ctx, 0x00368A48, {load<u32>(0x00F64D3C)}));
    if (p == 0) return;
    const u32 c = load<u32>(p + 24);
    const bool bit = c != 0 && (load<u32>(c + 164) & 0x8000u) != 0;
    if (on != 0) {
        if (bit) call(ctx, 0x00368AC8, {p, 0, 0});
    } else if (!bit) {
        callf(ctx, 0x00368870, {p, 0x0029F424u, 0, 0}, {Lfs32(0x0029F424)});
    }
    const u32 w = (load<u32>(p + 532) & ~0x01000000u) | ((r4 & 1u) << 24);
    store<u32>(p + 532, w);
    ctx.gpr[3] = w;
}

// Deleting ~T() (vt 0x00B1D5C8, 336 bytes): each 80-byte element e of the
// vector +304 (u16 count +312) gets 0x002F6054(e) and SmallFree(e, 80);
// the vector cleared, Thunk_000E2A34(o), SmallFree(o, 336). Nothing for a
// null o.
DECOMP(002F62E4, Dtor_B1D5C8_002F62E4) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1D5C8);
    for (u32 i = 0; i < load<u16>(o + 312); ++i) {
        const u32 e = load<u32>(load<u32>(o + 304) + 4 * i);
        if (e == 0) continue;
        call(ctx, 0x002F6054, {e});
        call(ctx, o32fn::SmallFree, {e, 80});
    }
    PvClear32(ctx, o + 304);
    call(ctx, 0x000E2A34, {o});
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {o, 336});
}

// Deleting ~T() (vt 0x00B1EB70, 2684 bytes): 0x00334114(o), the vectors
// +2652 / +2632 / +2612 / +2592 cleared, Thunk_00326A38(o), SmallFree(o,
// 2684). Nothing for a null o.
DECOMP(003343D0, Dtor_B1EB70_003343D0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1EB70);
    call(ctx, 0x00334114, {o});
    PvClear32(ctx, o + 2652);
    PvClear32(ctx, o + 2632);
    PvClear32(ctx, o + 2612);
    PvClear32(ctx, o + 2592);
    call(ctx, 0x00326A38, {o});
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {o, 2684});
}

// bool f(T* o, u8 strict): true without the state s = o->+36, when s->+120
// >= 2, or when the flags o->+12 have 0x08000000 / 0x00400000, or
// 0x01000000 with s->+8 equal to the reference (o->+32->+40->+12->+12, else
// K 0x00353F94), or 0x00800000 with s->+8 == K; otherwise strict && s->+68
// > 0.
DECOMP(00353F98, Obj_IsSettled_00353F98) {
    const u32 o = arg<u32>(ctx, 0), strict = arg<u32>(ctx, 1) & 0xFFu;
    const u32 s = load<u32>(o + 36), a = load<u32>(o + 32), w = load<u32>(o + 12);
    ctx.gpr[3] = 1;
    if (s == 0) return;
    f64 ref = Lfs32(0x00353F94);
    const bool eqk = Lfs32(s + 8) == ref;
    if (a != 0 && load<u32>(a + 40) != 0) ref = Lfs32(load<u32>(load<u32>(a + 40) + 12) + 12);
    const bool eqr = Lfs32(s + 8) == ref;
    ctx.fpr[1] = ref;
    ctx.fpr[2] = Lfs32(s + 8);
    if (static_cast<s32>(load<u32>(s + 120)) >= 2) return;
    if ((w & 0x08000000u) == 0 && (w & 0x00400000u) == 0) {
        if ((w & 0x01000000u) != 0 && eqr) return;
        if ((w & 0x00800000u) != 0 && eqk) return;
    }
    ctx.gpr[3] = (strict != 0 && static_cast<s32>(load<u32>(s + 68)) > 0) ? 1 : 0;
}

// void Heap_Free(?, void* p): under the lock 0x0106BFC8 (0x00ABF0E4 /
// 0x00ABF104), the block size ((p[-1] & ~15) + 16) comes off the counters
// 0x0106BFE8 / 0x0106BFEC and p goes to Pool_Free(pool, p) when the pool
// 0x0106BFE0 is enabled (u8 0x0106BFE4) and holds p, else free(p). Nothing
// for a null p (r3 left).
DECOMP(003554D4, Heap_Free_003554D4) {
    const u32 p = arg<u32>(ctx, 1);
    if (p == 0) return;
    call(ctx, 0x00ABF0E4, {0x0106BFC8u, 100000});
    const u32 sz = (load<u32>(p - 4) & ~15u) + 16;
    const u32 c1 = load<u32>(0x0106BFE8), c2 = load<u32>(0x0106BFEC);
    const u32 on = load<u8>(0x0106BFE4);
    store<u32>(0x0106BFE8, c1 - sz);
    store<u32>(0x0106BFEC, c2 - sz);
    bool pooled = false;
    u32 pool = 0;
    if (on != 0) {
        pool = load<u32>(0x0106BFE0);
        const u32 b = load<u32>(pool);
        pooled = p >= b && p < b + load<u32>(pool + 4);
    }
    if (pooled) call(ctx, 0x00565C50, {pool, p});
    else call(ctx, 0x0072A8FC, {p});
    ctx.gpr[3] = call(ctx, 0x00ABF104, {0x0106BFC8u});
}

// void f(Ref* a, T* b): b->+12 = q gets q->vt[128](q, v, 0) with v the
// word of the table 0x00F65038->+24 (128-byte records) at record a->+4 +
// u16 0x0106CF70 + 4 * the byte at u16 0x0106CF7E of record a->+2 (byte 0
// when that record's +2 is 0); records -1 read as address 0. r3 = 0.
DECOMP(00393944, Ref_Dispatch_00393944) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 q = load<u32>(b + 12);
    if (q != 0) {
        const u32 v = load<u32>(RefEntry32(a));
        const u32 opd = load<u32>(load<u32>(q) + 128);
        call_ptr(ctx, opd, {q, v, 0, v, load<u32>(opd)});  // entry in r7 (r6 = v)
    }
    ctx.gpr[3] = 0;
}

// bool f(Msg* m): with the target o = m->+12 having flag 8 in +192: unless
// o->vt[32](o, 0) is 5, bit 0x4000 of o->+172 cleared; r3 = (o->vt[32](o,
// 0) != 5). Otherwise 1.
DECOMP(0043AAB8, Msg_TargetNot5_0043AAB8) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 o = load<u32>(m + 12);
    ctx.gpr[3] = 1;
    if (o == 0 || (load<u32>(o + 192) & 8u) == 0) return;
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) != 5) store<u32>(o + 172, load<u32>(o + 172) & ~0x4000u);
    ctx.gpr[3] = static_cast<u32>(VCall(ctx, o, 32, {o, 0})) != 5 ? 1 : 0;
}

// T* f(?, Xml* x): a new 28-byte node (SmallAlloc, vt 0x00B21F90, u16 +4 =
// 1, +8/+20/+24 = 0) reading the hash 0x0024728B -> +8 and 0x00014B01 ->
// +20, or when that one is missing 0x04DA9951 -> +20. r3 = p.
DECOMP(004C0B24, XmlHashNode_New_004C0B24) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {28}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u16>(p + 4, 1);
    store<u32>(p, 0x00B21F90);
    store<u32>(p + 8, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    call(ctx, 0x004FA14C, {x, 0x0024728Bu, p + 8, 0});
    if (static_cast<u32>(call(ctx, 0x004FA14C, {x, 0x00014B01u, p + 20, 0})) != 0) call(ctx, 0x004FA14C, {x, 0x04DA9951u, p + 20, 0});
    ctx.gpr[3] = p;
}

// bool f(Xml* x): with the settings t = *0x010C2C28: the f32s named
// 0x00AF0194 / 0x00AF01A4 / 0x00AF01BC / 0x00AF01CC into t+660..+672
// (defaults *0x004EDA04, *0x004EDA08, *0x010C5A90, *0x010C5A94), u8
// t+656 = 1, r3 = 1; 0 for no t.
DECOMP(004EDA10, Settings_ReadF4_004EDA10) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 t = load<u32>(0x010C2C28);
    ctx.gpr[3] = 0;
    if (t == 0) return;
    callf(ctx, 0x004FA5C4, {x, 0x00AF0194u, t + 660}, {Lfs32(0x004EDA04)});
    callf(ctx, 0x004FA5C4, {x, 0x00AF01A4u, t + 664}, {Lfs32(0x004EDA08)});
    callf(ctx, 0x004FA5C4, {x, 0x00AF01BCu, t + 668}, {Lfs32(0x010C5A90)});
    callf(ctx, 0x004FA5C4, {x, 0x00AF01CCu, t + 672}, {Lfs32(0x010C5A94)});
    store<u8>(t + 656, 1);
    ctx.gpr[3] = 1;
}

// bool f(Xml* x): with the settings t = *0x010C2C28: the bool named
// 0x00AF0144 -> u8 t+805, the s32 named 0x00AF02F0 -> u8 t+806, the f32s
// 0x00AF02F8 / 0x00AF0308 -> t+808 / +812 (default *0x004EE98C), u8 t+804
// = 1, r3 = 1; 0 for no t.
DECOMP(004EE990, Settings_ReadMisc_004EE990) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 t = load<u32>(0x010C2C28);
    ctx.gpr[3] = 0;
    if (t == 0) return;
    Stack frame(ctx, 16);
    const u32 n = frame.addr();  // sp+112
    call(ctx, 0x004FA504, {x, 0x00AF0144u, t + 805, 0});
    call(ctx, 0x004FA564, {x, 0x00AF02F0u, n, 0});
    store<u8>(t + 806, static_cast<u8>(load<u32>(n)));
    const f64 k = Lfs32(0x004EE98C);
    callf(ctx, 0x004FA5C4, {x, 0x00AF02F8u, t + 808}, {k});
    callf(ctx, 0x004FA5C4, {x, 0x00AF0308u, t + 812}, {k});
    store<u8>(t + 804, 1);
    ctx.gpr[3] = 1;
}

// void f(Node* n, u64 mask, u8 set, u8 rec): the u64 flags n->+184 |= mask
// (set) or &= ~mask; for rec, the same for every child (240-byte entries
// of n->+340, u16 count +8; child +192), recursively. r3 = the flags, or
// n->+340 after the children.
DECOMP(007348D8, Node_SetFlags64_007348D8) {
    const u32 n = arg<u32>(ctx, 0);
    const u64 mask = ctx.gpr[4];
    const u32 set = arg<u32>(ctx, 2) & 0xFFu, rec = arg<u32>(ctx, 3) & 0xFFu;
    const u64 v = set != 0 ? (load<u64>(n + 184) | mask) : (load<u64>(n + 184) & ~mask);
    store<u64>(n + 184, v);
    ctx.gpr[3] = v;
    if (rec == 0) return;
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(n + 340);
        const u32 cnt = l != 0 ? load<u16>(l + 8) : 0u;
        ctx.gpr[3] = l;
        if (i >= cnt) return;
        call(ctx, 0x007348D8, {load<u32>(load<u32>(l) + 240 * i + 192), mask, set, rec});
    }
}

// T::~T() (vt 0x00B11E20): the vectors +92 / +72 / +52 / +32 / +12
// cleared, then the base vtable 0x00B11DC4 (r3 = it).
DECOMP(00743A04, Dtor_B11E20_00743A04) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B11E20);
    PvClear32(ctx, o + 92);
    PvClear32(ctx, o + 72);
    PvClear32(ctx, o + 52);
    PvClear32(ctx, o + 32);
    PvClear32(ctx, o + 12);
    store<u32>(o, 0x00B11DC4);
    ctx.gpr[3] = 0x00B11DC4;
}

// T::~T() (vt 0x00B16088): the vector +236 cleared, vt 0x00B12868, the
// vectors +200 / +180 cleared, vt 0x00B12260; the 20-byte vector object
// +16 (when set) cleared and SmallFree'd. r3 = 0x00B12260 without it.
DECOMP(00750408, Dtor_B16088_00750408) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B16088);
    PvClear32(ctx, o + 236);
    store<u32>(o, 0x00B12868);
    PvClear32(ctx, o + 200);
    PvClear32(ctx, o + 180);
    const u32 v = load<u32>(o + 16);
    store<u32>(o, 0x00B12260);
    ctx.gpr[3] = 0x00B12260;
    if (v == 0) return;
    PvClear32(ctx, v);
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {load<u32>(o + 16), 20});
}

// Deleting ~T() (vt 0x00B1D370): +376 = 0; unless bit 0x80000000 of +380,
// the allocator g = 0x011DF31C frees +372 (g->vt[20](g, +372, (s32)(+380
// << 2))); +372 = 0, +380 = 0x80000000; 0x00928008(o); then the pool
// (TLS r13 - 28332)->+44 = h: h->vt[12](h, o, u16 o->+4). Nothing for a
// null o.
DECOMP(00765BEC, Dtor_B1D370_00765BEC) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    constexpr u32 g = 0x011DF31C;
    const u32 w = load<u32>(o + 380);
    store<u32>(o, 0x00B1D370);
    store<u32>(o + 376, 0);
    if ((w & 0x80000000u) == 0) {
        const u64 n = static_cast<u64>(static_cast<s64>(static_cast<s32>(w << 2)));  // extsw
        const u32 opd = load<u32>(load<u32>(g) + 20);
        call_ptr(ctx, opd, {g, load<u32>(o + 372), n, load<u32>(opd), opd});  // entry in r6, OPD in r7
    }
    store<u32>(o + 372, 0);
    store<u32>(o + 380, 0x80000000u);
    call(ctx, 0x00928008, {o});
    const u32 h = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) - 28332) + 44);
    ctx.gpr[3] = VCall(ctx, h, 12, {h, o, load<u16>(o + 4)});
}

// ---------------------------------------------------------------- sweep: 59 instructions
// void f(Vec3* out, Nav** nav, const Vec3* p): out = p, then with a
// navigation mesh *nav set, out = the point a query (NavQuery_Init630F94(
// q, p copy, 0, 0x011AD1AC), NavQuery_Point62D25C(res, q, p copy, 0),
// Finish, Free) returns. r3 = p without a mesh; f1 = out.x / p.z.
DECOMP(0001087C, Nav_SnapPoint_0001087C) {
    const u32 out = arg<u32>(ctx, 0), nav = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    CopyW32(out, p);
    CopyW32(out + 4, p + 4);
    CopyW32(out + 8, p + 8);
    ctx.fpr[1] = Lfs32(p + 8);
    ctx.gpr[3] = p;
    if (load<u32>(nav) == 0) return;
    Stack frame(ctx, 64);
    const u32 q = frame.addr(), v = q + 8, res = q + 20;  // sp+112 / +120 / +132
    CopyW32(v, p);
    CopyW32(v + 4, p + 4);
    CopyW32(v + 8, p + 8);
    ctx.fpr[1] = Lfs32(p + 8);
    call(ctx, 0x00630F94, {q, v, 0, 0x011AD1ACu});
    call(ctx, 0x0062D25C, {res, q, v, 0});
    ctx.fpr[1] = Lfs32(res);
    ctx.fpr[2] = Lfs32(res + 4);
    ctx.fpr[3] = Lfs32(res + 8);
    CopyW32(out, res);
    CopyW32(out + 4, res + 4);
    CopyW32(out + 8, res + 8);
    call(ctx, 0x0062E094, {q});
    ctx.gpr[3] = call(ctx, 0x0062D08C, {q});
}

// void f(Actor* o, u8 on): on resets the state (bits of the u8s +464 /
// +465 / +466 / +305, the words +412..+456, u8s +384 / +385), +408 = the
// f32 of o->+308->+12 at the level index *(*0x00F95094 + 64), and the
// callback o->+64->+36 = {0x00F2DF90, o} (+52/+56); off clears that
// callback. Then 0x00012B78(o, on, cb).
DECOMP(0002F814, Actor_SetCtl_0002F814) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on == 0) {
        const u32 q = load<u32>(load<u32>(o + 64) + 36);
        store<u32>(q + 52, 0);
        store<u32>(q + 56, 0);
        ctx.gpr[3] = call(ctx, 0x00012B78, {o, on, 0});
        return;
    }
    const u32 b464 = load<u8>(o + 464) & 0x08u, b465 = load<u8>(o + 465) & ~0x3Cu, b466 = load<u8>(o + 466) & ~0x20u, b305 = load<u8>(o + 305) & ~0x08u;
    store<u8>(o + 466, static_cast<u8>(b466));
    store<u8>(o + 464, static_cast<u8>(b464));
    store<u8>(o + 384, 0);
    store<u8>(o + 305, static_cast<u8>(b305));
    store<u8>(o + 465, static_cast<u8>(b465));
    for (const u32 off : {444u, 448u, 452u, 412u, 416u, 420u, 436u, 432u}) store<u32>(o + off, 0);
    const u32 t = load<u32>(o + 308);
    store<u8>(o + 385, 0);
    const u32 lv = load<u32>(0x00F95094);
    store<u32>(o + 456, 0);
    const u32 tab = load<u32>(t + 12);
    const u32 idx = load<u32>(lv + 64) << 2;
    const u32 c = load<u32>(o + 64);
    ctx.fpr[1] = Lfs32(tab + idx);
    CopyW32(o + 408, tab + idx);
    const u32 q = load<u32>(c + 36);
    store<u32>(q + 52, 0x00F2DF90);
    store<u32>(q + 56, o);
    ctx.gpr[3] = call(ctx, 0x00012B78, {o, on, q});
}

// T* f(?, Xml* x): a 24-byte node (vt 0x00B10030, +16/+20 = 0) reading the
// hashes 0x7A30D7E3 -> +12, 0x8A93973F -> +16 and 0x00255BF3 -> +20
// (defaults 0). r3 = p.
DECOMP(00085380, XmlHash3_B10030_New_00085380) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = XmlNodeAlloc32(ctx, 24, x);
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B10030);
    call(ctx, 0x004FA14C, {x, 0x7A30D7E3u, p + 12, 0});
    call(ctx, 0x004FA14C, {x, 0x8A93973Fu, p + 16, 0});
    call(ctx, 0x004FA14C, {x, 0x00255BF3u, p + 20, 0});
    ctx.gpr[3] = p;
}

// The hash/hash/bool XML node of vtable 0x00B10B70 (see XmlHash2Bool_32).
DECOMP(00096248, XmlH2B_B10B70_New_00096248) { XmlHash2Bool_32(ctx, 0x00B10B70); }

// The three-hash XML node of vtable 0x00B10C38 (see XmlHash3_32).
DECOMP(00096C2C, XmlH3_B10C38_New_00096C2C) { XmlHash3_32(ctx, 0x00B10C38, 0x1EE23BF6); }

// The three-hash XML node of vtable 0x00B10C88 (see XmlHash3_32).
DECOMP(000971E0, XmlH3_B10C88_New_000971E0) { XmlHash3_32(ctx, 0x00B10C88, 0x5ED8D24B); }

// The three-hash XML node of vtable 0x00B10CB0 (see XmlHash3_32).
DECOMP(00097400, XmlH3_B10CB0_New_00097400) { XmlHash3_32(ctx, 0x00B10CB0, 0x5F6C8249); }

// T* f(?, Xml* x): a 20-byte node (vt 0x00B119B8, +12 = 1, f32 +16 = 0):
// the hash 0x040EFE5F read into a local (+12 = 1 when it is 0x0464F605;
// 1 either way) and the f32 0x973DDB1A -> +16. r3 = p.
DECOMP(000A6134, XmlModeF32_New_000A6134) {
    const u32 x = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 h = frame.addr();  // sp+112
    const u32 p = XmlNodeAlloc32(ctx, 20, x);
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u32>(p + 16, 0);
    store<u32>(p + 12, 1);
    store<u32>(p, 0x00B119B8);
    store<u32>(h, 0);
    call(ctx, 0x004FA14C, {x, 0x040EFE5Fu, h, 0});
    const f64 d = Lfs32(p + 16);
    if (load<u32>(h) == 0x0464F605u) store<u32>(p + 12, 1);
    callf(ctx, 0x004F9B20, {x, 0x973DDB1Au, p + 16}, {d});
    ctx.gpr[3] = p;
}

// The hash/hash/bool XML node of vtable 0x00B11A58 (see XmlHash2Bool_32).
DECOMP(000A6CF4, XmlH2B_B11A58_New_000A6CF4) { XmlHash2Bool_32(ctx, 0x00B11A58); }

// void f(T* o): resets o (+0 = 0x0A000000, +4 = 0.5f, +28 = 1.0f, the rest
// 0), releases the world's 0x1286A4 object (its vt[4](), then the slot 0),
// then +12..+24 = 1.0f, bit 0x10000000 set and 0x60000000 cleared in +0,
// u8 +32 = Global_Get_F65D90() (r3).
DECOMP(000ADCF8, Obj_Reset_000ADCF8) {
    const u32 o = arg<u32>(ctx, 0);
    for (const u32 off : {8u, 12u, 16u, 20u, 24u, 32u}) store<u32>(o + off, 0);
    store<u32>(o, 0x0A000000);
    store<u32>(o + 4, 0x3F000000);
    const u32 g = load<u32>(0x00F65CA0);
    store<u32>(o + 28, 0x3F800000);
    const u32 p = load<u32>(g + 0x1286A4);
    if (p != 0) {
        VCall(ctx, p, 4, {p});
        store<u32>(load<u32>(0x00F65CA0) + 0x1286A4, 0);
    }
    const u32 w = load<u32>(o) & ~0x60000000u;
    store<u32>(o + 12, 0x3F800000);
    store<u32>(o + 16, 0x3F800000);
    store<u32>(o + 20, 0x3F800000);
    store<u32>(o, w | 0x10000000u);
    store<u32>(o + 24, 0x3F800000);
    const u32 r = static_cast<u32>(call(ctx, 0x00483A24, {w | 0x10000000u, 0, 0x3F800000}));
    store<u8>(o + 32, static_cast<u8>(r));
}

// void f(u32 st[2], ?, Ui* u): the first time (g = *0x00F64584, g->+7344
// still 0) u is set up (+0/+4/+8/+32 = 0, +44 = 0x010127A0, +28 =
// 0x00F64588, +36/+40 = g->+7324, 16 bytes of 0x00AC5DFA at +12); st =
// {first ? 0 : 1, 50 * ++g->+7344}. r3 = st[1].
DECOMP(000B15F4, Ui_Open_000B15F4) {
    const u32 st = arg<u32>(ctx, 0), u = arg<u32>(ctx, 2);
    u32 again = 1;
    if (load<u32>(load<u32>(0x00F64584) + 7344) == 0) {
        again = 0;
        const u32 g = load<u32>(0x00F64584);
        store<u32>(u, 0);
        store<u32>(u + 44, 0x010127A0);
        const u32 v1 = load<u32>(g + 7324);
        store<u32>(u + 32, 0);
        store<u32>(u + 40, v1);
        store<u32>(u + 28, 0x00F64588);
        const u32 v2 = load<u32>(g + 7324);
        store<u32>(u + 8, 0);
        store<u32>(u + 36, v2);
        call(ctx, 0x0071C3F0, {u + 12, 0x00AC5DFAu, 16});
        store<u32>(u + 4, 0);
    }
    const u32 g = load<u32>(0x00F64584);
    store<u32>(st, again);
    store<u32>(g + 7344, load<u32>(g + 7344) + 1);
    const u32 v = static_cast<u32>(static_cast<s32>(load<u32>(g + 7344) * 100u) >> 1);
    store<u32>(st + 4, v);
    ctx.gpr[3] = v;
}

// Static destructor of nine tuning variables: 0x010137A0 / 0x01013A20
// (vt 0x00B0D934) and 0x010139D0 down to 0x010137F0 by 80 (vt
// 0x00B0D99C), each through TuningVar_DestructBase_004FC1A0.
DECOMP(000C23FC, TuningVars_Dtor_000C23FC) {
    store<u32>(0x010137A0, 0x00B0D934);
    call(ctx, 0x004FC1A0, {0x010137A0u});
    store<u32>(0x01013A20, 0x00B0D934);
    call(ctx, 0x004FC1A0, {0x01013A20u});
    for (u32 a = 0x010139D0; a >= 0x010137F0; a -= 80) {
        store<u32>(a, 0x00B0D99C);
        call(ctx, 0x004FC1A0, {a});
    }
}

// ~T() (vt 0x00B12230; see DtorB12230_32).
DECOMP(000C2BB0, Obj_BaseDtor_000C2BB0) { DtorB12230_32(ctx); }

// ~T() (vt 0x00B12230; see DtorB12230_32), the base destructor many
// classes chain to.
DECOMP(000C2DA0, Obj_BaseDtor_000C2DA0) { DtorB12230_32(ctx); }

// Bone* f(T* o, name): the bone record (16 bytes, skeleton +32) of
// Skeleton_FindBone(o->+160->vt[28](), name) (-1 for a null name or no
// skeleton); 0 when not found.
DECOMP(000DA9D0, Obj_FindBone_000DA9D0) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    u32 idx = 0xFFFFFFFFu;
    if (name != 0) {
        const u32 s = load<u32>(o + 160);
        if (s != 0) {
            const u32 sk = static_cast<u32>(VCall(ctx, s, 28, {s}));
            if (sk != 0) idx = static_cast<u32>(call(ctx, 0x005A93D8, {sk, name}));
        }
    }
    ctx.gpr[3] = 0;
    const u32 s = load<u32>(o + 160);
    if (s == 0) return;
    const u32 sk = static_cast<u32>(VCall(ctx, s, 28, {s}));
    ctx.gpr[3] = 0;
    if (sk == 0 || static_cast<s32>(idx) < 0) return;
    ctx.gpr[3] = load<u32>(sk + 32) + (idx << 4);
}

// u32 f(T* o, name): with the choice set c = CallK2_000D9424(o, name, 0),
// Choice_Pick_00288974(c, &r, String_HashNoCase(name)) (o->vt[76](o) when
// it fails); without one r = Obj_VCall188Hash_000DBD20(o,
// String_Format(0x00ACA934, o->+216 + 196, name)). r3 = r (0 by default).
DECOMP(001288C8, Obj_PickChoice_001288C8) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 r = frame.addr();  // sp+112
    store<u32>(r, 0);
    const u32 c = static_cast<u32>(call(ctx, 0x000D9424, {o, name, 0}));
    if (c != 0) {
        const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {name}));
        if (static_cast<u32>(call(ctx, 0x00288974, {c, r, h})) == 0) VCall(ctx, o, 76, {o});
    } else {
        const u32 s = static_cast<u32>(call(ctx, 0x005534D0, {0x00ACA934u, load<u32>(o + 216) + 196, name}));
        store<u32>(r, static_cast<u32>(call(ctx, 0x000DBD20, {o, s})));
    }
    ctx.gpr[3] = load<u32>(r);
}

// void f(T* o, Hit* m): for a source m->+4 other than o (and, when set,
// without flag 0x100 in its +192) that passes Obj_TestE0BCC(o, m), when
// the speed o->+216->+232 > K (0x001856A8): bit 0x100 of the u16 m->+14 =
// !o->vt[352](o, -, m->+4, 0xA77EB5E9; f1 = m->+52 * speed).
DECOMP(001856B0, Obj_OnHit_001856B0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 src = load<u32>(m + 4);
    const u32 st = load<u32>(o + 216);
    ctx.gpr[3] = src;
    if (src == o) return;
    if (src != 0) {
        const u32 f = load<u32>(src + 192) & 0x100u;
        ctx.gpr[3] = f;
        if (f != 0) return;
    }
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, m})) == 0) return;
    const f64 sp = Lfs32(st + 232), k = Lfs32(0x001856A8);
    ctx.fpr[1] = sp;
    ctx.fpr[2] = k;
    ctx.gpr[3] = 0x001856A8;
    if (!(sp > k)) return;
    ctx.fpr[2] = Lfs32(m + 52);
    ctx.fpr[1] = static_cast<f32>(Lfs32(m + 52) * sp);
    const u32 opd = load<u32>(load<u32>(o) + 352);
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, load<u32>(opd), load<u32>(m + 4), 0xA77EB5E9u, opd}));  // entry in r4, OPD in r7
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(m + 14, static_cast<u16>((load<u16>(m + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ctx.gpr[3] = z;
}

// ~T() (vt 0x00B14D98): the 60-byte elements of the vector +296 (u16
// count +304) SmallFree'd from the back (the swap-with-last tail only
// when the u16 index wraps), AnimSkel_Release7D4EA8(+292) when set, the
// vector cleared, vt 0x00B11D98, Obj_BaseDtor_000C2DA0(o).
DECOMP(0018EDB4, Dtor_B14D98_0018EDB4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B14D98);
    for (;;) {
        const u32 n = load<u16>(o + 304);
        if (n == 0) break;
        call(ctx, o32fn::SmallFree, {load<u32>(load<u32>(o + 296) + 4 * (n - 1)), 60});
        const u32 m = load<u16>(o + 304) - 1u;
        store<u16>(o + 304, static_cast<u16>(m));
        if (m == (m & 0xFFFFu)) continue;
        const u32 a = load<u32>(o + 296);
        store<u32>(a + (m << 2), load<u32>(a + ((m << 2) & 0x3FFFCu)));
    }
    if (load<u32>(o + 292) != 0) call(ctx, 0x007D4EA8, {load<u32>(o + 292)});
    PvClear32(ctx, o + 296);
    store<u32>(o, 0x00B11D98);
    ctx.gpr[3] = call(ctx, 0x000C2DA0, {o});
}

// void f(T* o): 0x000E1420(o); o+560 = the vector 0x01164B40, or with the
// physics body p = o->+576 active (p->+8) its +432 position, then
// Hk_Activate91A4C8(p) and (p+224)->vt[68](p + 224, 0x00B2C1D0).
DECOMP(001B8DF8, Obj_SyncBody_001B8DF8) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000E1420, {o});
    const u32 p = load<u32>(o + 576);
    CopyW32(o + 560, 0x01164B40);
    CopyW32(o + 564, 0x01164B44);
    CopyW32(o + 568, 0x01164B48);
    ctx.fpr[1] = Lfs32(0x01164B48);
    ctx.gpr[3] = 0x01160000;
    if (p == 0) return;
    ctx.gpr[3] = load<u32>(p + 8);
    if (ctx.gpr[3] == 0) return;
    const u32 x = load<u32>(p + 432), y = load<u32>(p + 436), z = load<u32>(p + 440);
    store<u32>(o + 560, x);
    store<u32>(o + 564, y);
    store<u32>(o + 568, z);
    ctx.fpr[1] = std::bit_cast<f32>(x);
    ctx.fpr[2] = std::bit_cast<f32>(y);
    ctx.fpr[3] = std::bit_cast<f32>(z);
    call(ctx, 0x0091A4C8, {p});
    ctx.gpr[3] = VCall(ctx, p + 224, 68, {p + 224, 0x00B2C1D0});
}

// void f(Area* o, T* a): for an a with flag 0x20000 in its +196: unless the
// area allows it (bit 2 of +436 and bit 0x40000000 of +468, or the record
// o->+460 found by Obj_FindRecord76_002D9994(*0x00F64C20->+40, key) with
// bit 0x40000000 of +468), 0x0021FB38(o, a); then Area_Leave(o, a).
DECOMP(00221E44, Area_OnLeave_00221E44) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    if (a == 0) return;
    ctx.gpr[3] = load<u32>(a + 196) & 0x20000u;
    if (ctx.gpr[3] == 0) return;
    bool allow = false;
    if ((load<u32>(o + 436) & 2u) != 0) {
        allow = true;
    } else {
        const u32 q = load<u32>(o + 460);
        if (q != 0) {
            const u32 t = load<u32>(load<u32>(0x00F64C20) + 40);
            if (t != 0) {
                Stack frame(ctx, 80);
                const u32 key = frame.addr();  // sp+112
                call(ctx, 0x0049E8A8, {key, q});
                allow = static_cast<u32>(call(ctx, 0x002D9994, {t, key})) != 0;
            }
        }
    }
    if (!allow || (load<u32>(o + 468) & 0x40000000u) == 0) call(ctx, 0x0021FB38, {o, a});
    ctx.gpr[3] = call(ctx, 0x0020EBC8, {o, a});
}

// bool f(T* o, Cmd* c): 0x0024DF08(o, c) for the kinds c->+0 0 (when
// c->+12 is 1), 1 (when c->+8 is 1) and 2 (when o->vt[32](o, 0) is 1);
// otherwise 1.
DECOMP(0024E6EC, Obj_RunCmd_0024E6EC) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const s32 t = static_cast<s32>(load<u32>(c));
    bool go = false;
    if (t > 0) {
        if (t == 1) go = load<u32>(c + 8) == 1;
        else if (t == 2) go = static_cast<u32>(VCall(ctx, o, 32, {o, 0})) == 1;
    } else if (t == 0) {
        go = load<u32>(c + 12) == 1;
    }
    ctx.gpr[3] = go ? call(ctx, 0x0024DF08, {o, c}) : 1;
}

// T::T(T* o, const Desc* d): vt 0x00B1BE30, +4/+8/+12 = d's, u8 +16 = 1,
// +20 = 0, then {o->+4, o} pushed onto the registry array 0x01056F1C (u16
// capacity +0, count +2, high-water +4, flags +6 |= 1 (r3), 8-byte entries
// at +8; doubled with Array_Reserve when full).
DECOMP(00250298, Obj_Register_00250298) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    constexpr u32 A = 0x01056F1C;
    store<u32>(o, 0x00B1BE30);
    store<u32>(o + 4, load<u32>(d + 4));
    store<u32>(o + 8, load<u32>(d + 8));
    const u32 w12 = load<u32>(d + 12);
    store<u8>(o + 16, 1);
    store<u32>(o + 12, w12);
    store<u32>(o + 20, 0);
    const u32 cap = load<u16>(A);
    if (load<u16>(A + 2) == cap) call(ctx, 0x00558F3C, {A, (cap << 1) & 0xFFFEu});
    const u32 data = load<u32>(A + 8);
    store<u32>(data + ((static_cast<u32>(load<u16>(A + 2)) << 3) & 0x7FFF8u), load<u32>(o + 4));
    store<u32>(data + (static_cast<u32>(load<u16>(A + 2)) << 3) + 4, o);
    const u32 n = load<u16>(A + 2) + 1u, hi = load<u16>(A + 4), fl = load<u16>(A + 6);
    store<u16>(A + 2, static_cast<u16>(n));
    store<u16>(A + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFFu) ? hi : n));
    store<u16>(A + 6, static_cast<u16>(fl | 1u));
    ctx.gpr[3] = fl | 1u;
}

// void f(T* o, u8 on): u8 o+16 = on; off: the two parts +36 / +40 each get
// vt[20](p, 0) and, unless their +160 >> 1 is set, Obj_Dispatch362(p, 0,
// 1, 0).
DECOMP(0025773C, Obj_SetOn_0025773C) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    store<u8>(o + 16, static_cast<u8>(on));
    if (on != 0) return;
    for (const u32 off : {36u, 40u}) {
        const u32 p = load<u32>(o + off);
        ctx.gpr[3] = p;
        if (p == 0) continue;
        VCall(ctx, p, 20, {p, 0});
        const u32 q = load<u32>(o + off);
        ctx.gpr[3] = load<u32>(q + 160) >> 1;
        if (ctx.gpr[3] != 0) continue;
        call(ctx, 0x0036325C, {q, 0, 1, 0});
    }
}

// void f(o, name, a, u8 b, ?, ?, c, d; f1..f4; stack args 11, 12):
// Anim_Restart2775A8(o, String_HashNoCase(name), a, b, -, -, c, d, -,
// -, arg11, (s32)arg12; f1..f4). Arguments 9 and 10 are passed as the
// original leaves them (never written; the callee does not read them).
DECOMP(0029817C, Anim_RestartByName_0029817C) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3) & 0xFFu;
    const u32 c = arg<u32>(ctx, 6), d = arg<u32>(ctx, 7);
    const f64 f1 = farg(ctx, 0), f2 = farg(ctx, 1), f3 = farg(ctx, 2), f4 = farg(ctx, 3);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 a11 = load<u32>(sp + 132), a12 = load<u32>(sp + 140);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {name}));
    const u64 x12 = static_cast<u64>(static_cast<s64>(static_cast<s32>(a12)));
    // r7 / r8 as String_HashNoCase left them (the original's li r7, 380 / li r8, 372 come before that call); unread by the callee
    callf(ctx, 0x002775A8, {o, h, a, b, ctx.gpr[7], ctx.gpr[8], c, d, 0, 0, a11, x12}, {f1, f2, f3, f4});
}

// void f(T* o, Obj* x): removes x from the 12-byte records of o->+2460
// (u16 count +2468; first whose +0 is x): x->vt[8](x), count - 1 and the
// last record moved into the hole. r3 = x when absent.
DECOMP(002A8F18, Recs_Remove_002A8F18) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 n = load<u16>(o + 2468);
    const u32 arr = load<u32>(o + 2460);
    ctx.gpr[3] = x;
    u32 i = 0;
    if (n == 0) return;
    while (load<u32>(arr + 12 * i) != x) {
        ++i;
        if (i >= n) return;
    }
    VCall(ctx, x, 8, {x});
    const u32 m = load<u16>(o + 2468) - 1u;
    const u32 l = m & 0xFFFFu;
    store<u16>(o + 2468, static_cast<u16>(m));
    ctx.gpr[3] = l;
    if (i == l) return;
    const u32 a = load<u32>(o + 2460);
    const u32 s = a + 12 * l, dd = a + 12 * i;
    store<u32>(dd, load<u32>(s));
    store<u32>(dd + 4, load<u32>(s + 4));
    const u32 w = load<u32>(s + 8);
    store<u32>(dd + 8, w);
    ctx.gpr[3] = w;
}

// void f(T* o, f32 dt): with u8 +376 set, the Havok body of
// o->+8->vt[64]()->+32 (Hk_GetBody9D19C8) is activated when its +8 is
// set; then the timer +348 counts down by dt while above K (0x002CCCA4)
// and +364/+368/+372 = 0. r3 = o + 364, f1 = the timer.
DECOMP(002CCCA8, Obj_TickTimer_002CCCA8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (load<u8>(o + 376) != 0) {
        const u32 a = load<u32>(o + 8);
        if (a != 0) {
            const u32 b = static_cast<u32>(VCall(ctx, a, 64, {a}));
            if (b != 0 && load<u32>(b + 32) != 0) {
                const u32 h = static_cast<u32>(call(ctx, 0x009D19C8, {load<u32>(b + 32)}));
                if (h != 0 && load<u32>(h + 8) != 0) call(ctx, 0x0091A4C8, {h});
            }
        }
    }
    f64 t = Lfs32(o + 348);
    const f64 k = Lfs32(0x002CCCA4);
    ctx.fpr[2] = k;
    if (t > k) {
        t = static_cast<f32>(t - dt);
        Stfs32(o + 348, t);
    }
    store<u32>(o + 364, 0);
    store<u32>(o + 368, 0);
    store<u32>(o + 372, 0);
    ctx.fpr[1] = t;
    ctx.gpr[3] = o + 364;
}

// void f(T* o, x): in state 4 (u8 +18) and when Obj_CanShow2E2104(
// *0x00F64C20, o): each 20-byte entry e of o->+2272 (u16 count +8, list
// re-read) with u8 +12 set gets u8 +11 = 0, 0x00321324(e, 1) and
// 0x003218B0(e, x, 1). r3 = the state otherwise, or the count.
DECOMP(00325CE4, Obj_ShowEntries_00325CE4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    ctx.gpr[3] = load<u8>(o + 18);
    if (ctx.gpr[3] != 4) return;
    if (static_cast<u32>(call(ctx, 0x002E2104, {load<u32>(0x00F64C20), o})) == 0) return;
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(o + 2272);
        const u32 n = l != 0 ? load<u16>(l + 8) : 0u;
        ctx.gpr[3] = n;
        if (i >= n) return;
        const u32 e = load<u32>(l) + 20 * i;
        if (load<u8>(e + 12) == 0) continue;
        store<u8>(e + 11, 0);
        call(ctx, 0x00321324, {e, 1});
        call(ctx, 0x003218B0, {e, x, 1});
    }
}

// void f(T* o, u8 a, s32 k): 0x0032549C(o, a, k); for k 4, each 144-byte
// record of o->+2572 (u16 count +2580) gets its object Obj_FindById(+16)
// into +20 and u8 +48 = 1, until one is missing or lacks flag 0x40000 in
// its +172: then o->vt[104](o, 6, 0) and stop. r3 = the count otherwise.
DECOMP(0032C3F4, Recs_Resolve_0032C3F4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, k = arg<u32>(ctx, 2);
    call(ctx, 0x0032549C, {o, a, k});
    if (k != 4) return;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 2580);
        ctx.gpr[3] = n;
        if (i >= n) return;
        const u32 e = load<u32>(o + 2572) + 144 * i;
        const u32 id = load<u32>(e + 16);
        if (id != 0) {
            const u32 p = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
            store<u32>(e + 20, p);
            if (p == 0 || (load<u32>(p + 172) & 0x40000u) == 0) {
                ctx.gpr[3] = VCall(ctx, o, 104, {o, 6, 0});
                return;
            }
        }
        store<u8>(e + 48, 1);
    }
}

// ~T() (vt 0x00B1F570): the two pools +76 (u16 count +76, u16 stride*2
// +82, data +84, allocator +88) and +52 (+52 / +58 / +60 / +64) give their
// data back (alloc->vt[8](alloc, data, count * (stride2 >> 1))) and are
// destructed (Array_Destruct); then 0x00353334(o).
DECOMP(003544AC, Dtor_B1F570_003544AC) {
    const u32 o = arg<u32>(ctx, 0);
    for (const u32 b : {76u, 52u}) {
        const u32 h = load<u16>(o + b + 6), c = load<u16>(o + b);
        if (b == 76) store<u32>(o, 0x00B1F570);
        const u32 n = c * ((h >> 1) & 0x7FFFu);
        const u32 p = load<u32>(o + b + 8);
        if (p != 0) {
            const u32 al = load<u32>(o + b + 12);
            VCall(ctx, al, 8, {al, p, n});
        }
        call(ctx, 0x0054EC94, {o + b});
    }
    ctx.gpr[3] = call(ctx, 0x00353334, {o});
}

// void f(T* o, u8 a, u8 notifyA, u8 c): the registered entry p of o (map
// 0x0106C228, SortedMap_Find3) with flag 0x4000 in its +4 and without
// 0x02000000 in +396 gets Obj_NotifyA362F1C(p, a, c) (notifyA) or
// Obj_NotifyB362B40(p, a, c).
DECOMP(00367B28, Obj_NotifyReg_00367B28) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    if (o == 0) return;
    Stack frame(ctx, 16);
    const u32 key = frame.addr();  // sp+112
    store<u32>(key, o);
    const u32 p = load<u32>(static_cast<u32>(call(ctx, 0x0076611C, {0x0106C228u, key, o})));
    ctx.gpr[3] = p;
    if (p == 0) return;
    if ((load<u32>(p + 4) & 0x4000u) == 0) return;
    ctx.gpr[3] = a & 0xFFu;
    if ((load<u32>(p + 396) & 0x02000000u) != 0) return;
    if ((b & 0xFFu) != 0) ctx.gpr[3] = call(ctx, 0x00362F1C, {p, a & 0xFFu, c & 0xFFu});
    else ctx.gpr[3] = call(ctx, 0x00362B40, {p, a & 0xFFu, c & 0xFFu});
}

// void f(T* o): unless blocked (flag 0x01000000 of +396 with
// Obj_Anim353EB0_004A5D40(o->+28, 0) false, or 0x00800000 with the effect
// +20 missing or Effect_Check4CF620(it, 0) false): +160 = (+160 & 1) | 6,
// +340 = +336 when >= K (0x0036BBBC) else +332, and Obj_SetState362D28(o,
// 3, !+316 || flag 0x00080000 of +396).
DECOMP(0036BBC0, Obj_EnterState3_0036BBC0) {
    const u32 o = arg<u32>(ctx, 0);
    u32 w = load<u32>(o + 396);
    if ((w & 0x01000000u) != 0) {
        if (static_cast<u32>(call(ctx, 0x004A5D40, {load<u32>(o + 28), 0})) == 0) return;
        w = load<u32>(o + 396);
    }
    if ((w & 0x00800000u) != 0) {
        const u32 e = load<u32>(o + 20);
        ctx.gpr[3] = e;
        if (e == 0) return;
        if (static_cast<u32>(call(ctx, 0x004CF620, {e, 0})) == 0) return;
        w = load<u32>(o + 396);
    }
    store<u32>(o + 160, (load<u32>(o + 160) & 1u) | 6u);
    f64 f1 = Lfs32(o + 336);
    ctx.fpr[2] = Lfs32(0x0036BBBC);
    if (!(f1 >= ctx.fpr[2])) f1 = Lfs32(o + 332);
    Stfs32(o + 340, f1);
    ctx.fpr[1] = f1;
    u32 r4 = 1;
    if (load<u32>(o + 316) != 0) r4 = (w >> 19) & 1u;
    ctx.gpr[3] = call(ctx, 0x00362D28, {o, 3, r4 != 0 ? 1u : 0u});
}

// bool f(Ref* a, T* b): b->+12 = q with flag 0x10 in its +192 gets
// Obj_SetDrawCb_0001CC30(q, the table byte for a (RefEntry32), 0). r3 = 1.
DECOMP(00394D28, Ref_SetDrawCb_00394D28) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 q = load<u32>(b + 12);
    if (q != 0 && (load<u32>(q + 192) & 0x10u) != 0) call(ctx, 0x0001CC30, {q, load<u8>(RefEntry32(a)), 0});
    ctx.gpr[3] = 1;
}

// bool f(Ref* a, T* b): b->+12 = q with flag 8 in its +192 gets bit 0x4000
// of its u64 +184 set when the table byte for a (RefEntry32) is 0, else
// cleared. r3 = 0.
DECOMP(003957DC, Ref_SetFlag4000_003957DC) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 q = load<u32>(b + 12);
    if (q != 0 && (load<u32>(q + 192) & 8u) != 0) {
        const u32 v = load<u8>(RefEntry32(a));
        const u64 f = load<u64>(q + 184);
        store<u64>(q + 184, v == 0 ? (f | 0x4000u) : (f & ~0x4000ull));
    }
    ctx.gpr[3] = 0;
}

// s32 f(path, buf, size, u32* got): a blocking fios_ReadFile of path into
// buf (attributes from JobAttr_Build(&attr, 1, 0, 0, 0, buf), the
// scheduler Fios_GetScheduler5FC90C(), then the wait 0x005F8A38(op, 0));
// *got = 0 before and size after success. r3 = 0, or -4 when the wait
// fails.
DECOMP(0044C288, File_ReadBlocking_0044C288) {
    const u32 path = arg<u32>(ctx, 0), buf = arg<u32>(ctx, 1), size = arg<u32>(ctx, 2), got = arg<u32>(ctx, 3);
    Stack frame(ctx, 48);
    const u32 attr = frame.addr();  // sp+112
    call(ctx, 0x0056D0E0, {attr, 1, 0, 0, 0, buf});
    if (got != 0) store<u32>(got, 0);
    const u32 s = static_cast<u32>(call(ctx, 0x005FC90C, {}));
    const u32 op = static_cast<u32>(call(ctx, 0x00603514, {s, attr, path, buf, size, ~0ull}));
    if (static_cast<u32>(call(ctx, 0x005F8A38, {op, 0})) != 0) {
        ctx.gpr[3] = static_cast<u64>(-4ll);
        return;
    }
    if (got != 0) store<u32>(got, size);
    ctx.gpr[3] = 0;
}

// T* f(ctx, Node* n): the first node, from n along the +156 chain, with
// vt[92]() and vt[84]() both true, else the first hit of this search over
// the +152 chain below it (recursively). 0 when none.
DECOMP(0046E7B0, Node_Find_0046E7B0) {
    const u32 c = arg<u32>(ctx, 0);
    u32 n = arg<u32>(ctx, 1);
    while (n != 0) {
        if (static_cast<u32>(VCall(ctx, n, 92, {n})) != 0 && static_cast<u32>(VCall(ctx, n, 84, {n})) != 0) {
            ctx.gpr[3] = n;
            return;
        }
        for (u32 k = load<u32>(n + 152); k != 0; k = load<u32>(k + 152)) {
            const u32 r = static_cast<u32>(call(ctx, 0x0046E7B0, {c, k}));
            if (r != 0) {
                ctx.gpr[3] = r;
                return;
            }
        }
        n = load<u32>(n + 156);
    }
    ctx.gpr[3] = 0;
}

// void f(Vec3* out, const Vec3* a, const Vec3* b, f32 t, f32 u): the 3D
// form of Curve_Lerp2_004A84A8 (constants at 0x004A8580, curve table
// 0x00AEB6D4). f1 = out.z.
DECOMP(004A8598, Curve_Lerp3_004A8598) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 t = farg(ctx, 0), u = farg(ctx, 1);
    constexpr u32 K = 0x004A8580;
    const f64 k0 = Lfs32(K);
    f64 w = static_cast<f32>(k0 - t);
    const f64 a0 = Lfs32(a), a1 = Lfs32(a + 4), a2 = Lfs32(a + 8);
    const f64 dx = static_cast<f32>(Lfs32(b) - a0), dy = static_cast<f32>(Lfs32(b + 4) - a1), dz = static_cast<f32>(Lfs32(b + 8) - a2);
    if (!(w < Lfs32(K + 4)) && !(w > Lfs32(K + 8))) {
        s32 i = static_cast<s32>(rt::fctiw(static_cast<f32>(std::fma(u, Lfs32(K + 12), Lfs32(K + 16))), true));
        if (i < 1) i = 1;
        i -= 1;
        if (i >= 6) i = 5;
        const s32 j = static_cast<s32>(rt::fctiw(static_cast<f32>(w * Lfs32(K + 20)), true));
        const u32 idx = static_cast<u32>(j) * 6u + static_cast<u32>(i);
        w = Lfs32(0x00AEB6D4u + (idx << 2));
    }
    const f64 m = static_cast<f32>(k0 - w);
    const f64 x = static_cast<f32>(std::fma(dx, m, a0)), y = static_cast<f32>(std::fma(dy, m, a1)), z = static_cast<f32>(std::fma(dz, m, a2));
    Stfs32(out, x);
    Stfs32(out + 4, y);
    Stfs32(out + 8, z);
    ctx.fpr[1] = z;
}

// Deleting ~T() (vts 0x00B20AF0 / +108 0x00B20AE0, 448 bytes): the target
// +112 with flag 0x40000 in its +172 gets 0x1000 in its u16 +176; +112 =
// 0, the vectors +268 / +248 cleared, vt 0x00B207A8,
// Dtor_B219D0_00490B34(o), SmallFree(o, 448). Nothing for a null o.
DECOMP(0077F350, Dtor_B20AF0_0077F350) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 p = load<u32>(o + 112);
    store<u32>(o, 0x00B20AF0);
    store<u32>(o + 108, 0x00B20AE0);
    if (p != 0 && (load<u32>(p + 172) & 0x40000u) != 0) store<u16>(p + 176, static_cast<u16>(load<u16>(p + 176) | 0x1000u));
    store<u32>(o + 112, 0);
    PvClear32(ctx, o + 268);
    PvClear32(ctx, o + 248);
    store<u32>(o, 0x00B207A8);
    call(ctx, 0x00490B34, {o});
    ctx.gpr[3] = call(ctx, o32fn::SmallFree, {o, 448});
}
