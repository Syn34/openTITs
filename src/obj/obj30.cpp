// Game code, part 30: the continued sweep of small still-recompiled game
// functions (in size order, from 49 instructions up). Most have no known
// caller context yet, so their names describe what they do and carry the
// address. Each was checked against the original in the PPC simulator.
//
// Register notes: where a known caller reads a volatile register the
// original leaves behind (regflow liveout), the value is set the same way,
// as the comments say.
#include "obj30.h"

using namespace sly4;
using namespace sly4::obj30;

// ---------------------------------------------------------------- sweep: 49 instructions
// T* f(T* o): unless bit 0x02 of the u8 o->+464 is set or o->+4 is 40,
// returns 0x00016A40(o, o->+68, o->+4); otherwise, when the child c =
// o->+68 is set, r = c->vt[8](c) is reset: +128 = 1.0f, bit 0x02000000 of
// +168 cleared, +16..+28 = the 16 bytes at 0x01164C90, +0/+4/+8 = 0, then
// bit 0x04000000 of +168 set (r3 = r; o when there is no child)
DECOMP(0003A92C, Obj_ResetChild_0003A92C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 68);
    if ((load<u8>(o + 464) & 0x2u) == 0) {
        const u32 k = load<u32>(o + 4);
        if (k != 40) {
            ctx.gpr[3] = call(ctx, 0x00016A40, {o, c, k});
            return;
        }
    }
    if (c == 0) return;
    const u32 r = static_cast<u32>(VCall(ctx, c, 8, {c}));
    const u32 f = load<u32>(r + 168) & ~0x02000000u;
    store<u32>(r + 128, 0x3F800000u);
    store<u32>(r + 168, f);
    Cp(r + 16, 0x01164C90);
    Cp(r + 20, 0x01164C94);
    Cp(r + 24, 0x01164C98);
    const u32 w = load<u32>(0x01164C9C);
    store<u32>(r, 0);
    store<u32>(r + 28, w);
    store<u32>(r + 4, 0);
    store<u32>(r + 8, 0);
    store<u32>(r + 168, f | 0x04000000u);
    ctx.gpr[3] = r;
    ctx.fpr[1] = F32Bits28(w);
}

// void f(T* o): with a stack o->+20: when the u8 o->+12 is set and the
// stack is not in the table, PushStack_Push(stack; o->+24); when clear and
// it is, PushStack_Remove(stack; o->+24). Without one, the warning
// String_Format(0x00AC48EC, Hash_ToString(o->+16)) when the u8 0x010AEC60
// is set.
DECOMP(0006BC70, Obj_PushStackToggle_0006BC70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 20);
    ctx.gpr[3] = s;
    if (s == 0) {
        const u32 on = load<u8>(0x010AEC60);
        ctx.gpr[3] = on;
        if (on != 0) WarnHash27(ctx, 0x00AC48EC, load<u32>(o + 16));
        return;
    }
    const u32 b = load<u8>(o + 12);
    if (b != 0) {
        if (static_cast<u32>(call(ctx, o30fn::Table_Contains, {s, b})) != 0) return;
        callf(ctx, o30fn::PushStack_Push, {load<u32>(o + 20)}, {LfsAt(o + 24)});
    } else {
        if (static_cast<u32>(call(ctx, o30fn::Table_Contains, {s, b})) == 0) return;
        callf(ctx, o30fn::PushStack_Remove, {load<u32>(o + 20)}, {LfsAt(o + 24)});
    }
}

// void f(T* o): when (o->+12 or o->+20) and (o->+16 or o->+24),
// CmdList_Push(c) of c = SmallAlloc(112) built by 0x0042FF28(c, o->+12,
// +16, +20, +24; +28, +32, +36) (CmdList_Push(0) when the alloc fails)
DECOMP(0009ED90, Cmd_NewPush_0009ED90) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = load<u32>(o + 12);
    if (ctx.gpr[3] == 0) {
        ctx.gpr[3] = load<u32>(o + 20);
        if (ctx.gpr[3] == 0) return;
    }
    ctx.gpr[3] = load<u32>(o + 16);
    if (ctx.gpr[3] == 0) {
        ctx.gpr[3] = load<u32>(o + 24);
        if (ctx.gpr[3] == 0) return;
    }
    const u32 c = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {112}));
    if (c != 0)
        callf(ctx, 0x0042FF28, {c, load<u32>(o + 12), load<u32>(o + 16), load<u32>(o + 20), load<u32>(o + 24)},
              {LfsAt(o + 28), LfsAt(o + 32), LfsAt(o + 36)});
    ctx.gpr[3] = call(ctx, o30fn::CmdList_Push, {c});
}

// void f(T* o): resets the state block: +556 = 1, +528..+540 = the 16
// bytes at 0x01164AF4, +524 = +544 = 0, the bytes +548..+551 = 0,0,0,255
// and +552..+555 = 0, +557..+562 = 0,0,1,1,1,1 and +12 / +140 / +268 /
// +396 = 0 (r3 = o)
DECOMP(000A1C28, Obj_ResetBlock_000A1C28) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 556, 1);
    Cp(o + 528, 0x01164AF4);
    store<u32>(o + 524, 0);
    Cp(o + 532, 0x01164AF8);
    store<u32>(o + 544, 0);
    Cp(o + 536, 0x01164AFC);
    const u32 w = Cp(o + 540, 0x01164B00);
    store<u8>(o + 551, 255);
    store<u8>(o + 550, 0);
    store<u8>(o + 549, 0);
    store<u8>(o + 548, 0);
    store<u8>(o + 555, 0);
    store<u8>(o + 554, 0);
    store<u8>(o + 553, 0);
    store<u8>(o + 552, 0);
    store<u8>(o + 562, 1);
    store<u8>(o + 561, 1);
    store<u8>(o + 12, 0);
    store<u8>(o + 560, 1);
    store<u8>(o + 559, 1);
    store<u8>(o + 558, 0);
    store<u8>(o + 557, 0);
    store<u8>(o + 140, 0);
    store<u8>(o + 268, 0);
    store<u8>(o + 396, 0);
    ctx.fpr[1] = F32Bits28(w);
}

// T::~T() deleting (vtable 0x00B11D80): the handles o->+28
// (Handles_Destroy, Mem_FreeFrom(h, 0)) cleared, the global *0x00F558D0
// gets vt[8]() and is cleared, 0x00427E2C(o), then Mem_FreeFrom(o, 0)
DECOMP(000A8784, VtB11D80_Delete_000A8784) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 h = load<u32>(o + 28);
    store<u32>(o, 0x00B11D80);
    if (h != 0) {
        call(ctx, o30fn::Handles_Destroy, {h});
        call(ctx, o19fn::Mem_FreeFrom, {h, 0});
    }
    store<u32>(o + 28, 0);
    const u32 g = load<u32>(0x00F558D0);
    VCall(ctx, g, 8, {g});
    store<u32>(0x00F558D0, 0);
    call(ctx, 0x00427E2C, {o});
    ctx.gpr[3] = call(ctx, o19fn::Mem_FreeFrom, {o, 0});
}

// bool f(T* o; f32 m): the point o + 144 projected by the world view
// (*0x00F65CA0)->+0x128168; true when x or y is outside [-m, m + *0x000DADA4]
DECOMP(000DADA8, Obj_OffScreen_000DADA8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 m = farg(ctx, 0);
    Stack frame(ctx, 16);
    const u32 buf = frame.addr();
    call(ctx, o30fn::View_ProjectPoint, {load<u32>(load<u32>(kWorldPtr) + 0x128168), buf, o + 144});
    const f64 lo = -m;
    ctx.fpr[1] = lo;
    ctx.gpr[3] = 1;
    const f64 x = LfsAt(buf);
    if (x < lo) return;
    const f64 hi = Add28(m, LfsAt(0x000DADA4));
    if (x > hi) return;
    const f64 y = LfsAt(buf + 4);
    ctx.fpr[1] = y;
    if (y < lo || y > hi) return;
    ctx.gpr[3] = 0;
}

// bool f(T* o, ?, u32 a; f32 v): o->vt[16], vt[20], vt[24](o, -, a; v) (the
// OPD in r4, the entry in r6), then the u8 o->+80 = 0; returns 1
DECOMP(00107734, Obj_Call3Slots_00107734) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 2);
    const f64 v = farg(ctx, 0);
    VCallOpdR4E6_30(ctx, o, 16, a);
    ctx.fpr[1] = v;
    VCallOpdR4E6_30(ctx, o, 20, a);
    ctx.fpr[1] = v;
    VCallOpdR4E6_30(ctx, o, 24, a);
    store<u8>(o + 80, 0);
    ctx.gpr[3] = 1;
}

// void f(T* o, u32 i, f32* p; f32 a, f32 b, f32 c): when bit i of the
// bitset o->+12 is set: cleared when |*p - a| < *0x0010EB18, else *p =
// 0x004A83D8(o, -, -; *p, a, b, c) (f1 = c on the other paths)
DECOMP(0010EB20, Obj_BitSettle_0010EB20) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1), c = farg(ctx, 2);
    const u32 off = (i >> 5) * 4u, mask = 1u << (i & 31u);
    ctx.fpr[1] = c;
    if ((load<u32>(o + 12 + off) & mask) == 0) return;
    const f64 v = LfsAt(p);
    const f64 d = std::fabs(static_cast<f64>(Sub28(v, a)));
    if (d < LfsAt(0x0010EB18)) {
        const u32 ea = o + off + 12;
        store<u32>(ea, load<u32>(ea) & ~mask);
        ctx.gpr[3] = ea;
        return;
    }
    const f64 r = callf(ctx, 0x004A83D8, {o, mask, off}, {v, a, b, c});
    store<f32>(p, static_cast<f32>(r));
}

// void f(T* o): 0x004A783C(o->+160, o->+976, the u8 o->+996), then
// Call_4A783C_000DF4CC(e) for every element e = rec->+192 of the 240-byte
// records of the vector o->+340 (u16 count +8) whose +200 has bit 0x40 (r3
// = the vector)
DECOMP(00123F70, Obj_UpdateFlagged_00123F70) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x004A783C, {load<u32>(o + 160), load<u32>(o + 976), load<u8>(o + 996)});
    for (u32 i = 0, off = 0;; ++i, off += 240) {
        const u32 v = load<u32>(o + 340);
        ctx.gpr[3] = v;
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) return;
        const u32 e = load<u32>(load<u32>(v) + off + 192);
        if (e != 0 && (load<u32>(e + 200) & 0x40u) != 0) call(ctx, 0x000DF4CC, {e});
    }
}

// void f(T* o; f32 v): when the world list has a +4 and o->vt[144]() is
// set, (o->vt[144]())->+510 = the high half of v's bits; then
// Obj_ChildrenUpdate(o; v)
DECOMP(001788A0, Obj_SetHalfThenUpdate_001788A0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0);
    if (load<u32>(load<u32>(load<u32>(kGame30) + 0x1281D4) + 4) != 0 &&
        static_cast<u32>(VCall(ctx, o, 144, {o})) != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, o, 144, {o}));
        store<u16>(r + 510, static_cast<u16>(std::bit_cast<u32>(static_cast<f32>(v)) >> 16));
    }
    callf(ctx, o30fn::Obj_ChildrenUpdate, {o}, {v});
}

// bool f(T* o; f32 a, f32 b): 1 when bit 0x20000000 of o->+552 is set or
// not (b < *0x001C4574 or a >= b); otherwise o->vt[420](o, 1, 0) and, when
// bit 0x80 of the u8 o->+604 is set, it is cleared and 0x001C3EF0(o, 1)
// (returns 0)
DECOMP(001C4578, Obj_RangeGate_001C4578) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    if ((load<u32>(o + 552) & 0x20000000u) != 0 || !(b < LfsAt(0x001C4574) || a > b || a == b)) {
        ctx.gpr[3] = 1;
        return;
    }
    VCall(ctx, o, 420, {o, 1, 0});
    const u32 f = load<u8>(o + 604);
    if ((f & 0x80u) != 0) {
        store<u8>(o + 604, static_cast<u8>(f & 0x7Fu));
        call(ctx, 0x001C3EF0, {o, 1});
    }
    ctx.gpr[3] = 0;
}

// void f(T* a, U* b): Thunk_E44C0(a, b), then b->+236 = a->+268, the seven
// words b->+208.. = a->+240.. (raw f32 copies), the field 0xFFFE0000 of
// b->+20 = 4, b->+240 = a->+272 and b->+244 = a->+276 (r3 = a->+272, f1 =
// the last word)
DECOMP(001D4620, Obj_CopyParams_001D4620) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, o30fn::Thunk_E44C0, {a, b});
    Cp(b + 236, a + 268);
    for (u32 k = 0; k < 7; ++k) Cp(b + 208 + 4 * k, a + 240 + 4 * k);
    store<u32>(b + 20, (load<u32>(b + 20) & 0x1FFFFu) | (4u << 17));
    ctx.gpr[3] = Cp(b + 240, a + 272);
    ctx.fpr[1] = F32Bits28(Cp(b + 244, a + 276));
}

// void f(T* o, u8 on): SetFlag412_30 (see obj30.h)
DECOMP(0020C7A0, Obj_SetFlag412_0020C7A0) { SetFlag412_30(ctx); }

// void f(T* o, u8 on): the same body as 0x0020C7A0
DECOMP(0021D300, Obj_SetFlag412_0021D300) { SetFlag412_30(ctx); }

// void f(T* o): Thunk_E02D8(o), then the vtable 0x00B1B500, +380 = 0, bits
// of +384 (set 0x80000000 / 0x20000000, clear 0x40000000), +208 = 0 when
// bit 0 of the u64 +184 is set, bits 0x00C00000 of +172 = bits 0..1 of
// (o->+216)->+740, bit 5 of the u64 +184 = the top bit of (o->+216)->+744,
// then StateMachine_SetState(o, 0, 0)
DECOMP(00228AC0, Obj_Init_00228AC0) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, o30fn::Thunk_E02D8, {o, arg<u32>(ctx, 1)});
    const u32 f384 = ((load<u32>(o + 384) | 0x80000000u) & ~0x40000000u) | 0x20000000u;
    const u64 d = load<u64>(o + 184);
    store<u32>(o + 380, 0);
    store<u32>(o, 0x00B1B500);
    const u32 q = load<u32>(o + 216);
    const u32 f172 = load<u32>(o + 172);
    store<u32>(o + 384, f384);
    const u32 w = load<u32>(q + 740);
    if ((d & 1u) != 0) store<u32>(o + 208, 0);
    store<u32>(o + 172, (f172 & ~0x00C00000u) | ((w & 3u) << 22));
    const u64 d2 = (load<u32>(q + 744) >> 31) != 0 ? (d | 0x20u) : (d & ~0x20ull);
    store<u64>(o + 184, d2);
    ctx.gpr[3] = call(ctx, o30fn::StateMachine_SetState, {o, 0, 0});
}

// void f(T* o, u8 b): the 4 bytes at o->+56 (b) / +60 (!b) go to o->+72 and
// to the 128-byte record o->+240 of the table *0x011950AC (+100), the
// bytes at +64 (b) / +68 (!b) to +608, and the u8 +626 = b (r3 = o)
DECOMP(00289DC4, Obj_SelectColor_00289DC4) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 tab = load<u32>(0x011950AC);
    const u32 s = b != 0 ? o + 56 : o + 60;
    CopyBytes30(o + 72, s, 4);
    const u32 d = tab + (load<u32>(o + 240) << 7) + 100;
    CopyBytes30(d, s, 4);
    CopyBytes30(o + 608, b != 0 ? o + 64 : o + 68, 4);
    store<u8>(o + 626, static_cast<u8>(b));
}

// void f(T* o, u8 b): e = Effect_Bind4BB7E0(o->+8->vt[272](), 0x2D3BE751,
// 0); when both are set, e->vt[40](e, b && !(bit 0x00080000 of o->+124), 1)
DECOMP(002B81FC, Obj_EffectToggle_002B81FC) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 c = load<u32>(o + 8);
    const u32 r = static_cast<u32>(VCall(ctx, c, 272, {c}));
    if (r == 0) return;
    const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {r, 0x2D3BE751u, 0}));
    if (e == 0) return;
    const u32 on = (b != 0 && (load<u32>(o + 124) & 0x00080000u) == 0) ? 1u : 0u;
    VCall(ctx, e, 40, {e, on, 1});
}

// T::~T() (vtable 0x00B1DFF0, then base 0x00B1DCA8): the PtrVector +64,
// the owned effect +40 (Effect_Destruct, SmallFree(p, 140)) cleared, the
// PtrVector +44, then Dtor_B219D0_00464AD0(o)
DECOMP(00318674, VtB1DFF0_Dtor_00318674) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1DFF0);
    VecDtor(ctx, o + 64);
    const u32 e = load<u32>(o + 40);
    store<u32>(o, 0x00B1DCA8);
    if (e != 0) {
        call(ctx, 0x004C8E80, {e});
        call(ctx, o19fn::SmallFree, {e, 140});
        store<u32>(o + 40, 0);
    }
    VecDtor(ctx, o + 44);
    ctx.gpr[3] = call(ctx, 0x00464AD0, {o});
}

// T* f(u32 h, U* k): 0 unless both are set; r = 0x003677C4(e, k) with e
// the entry for h in the sorted map 0x0106C228 when bit 0 of its +4 is set
// (else 0); Hash_ToString(h) when r is null (the original's warning call
// with the string unused). Returns r.
DECOMP(00367900, Map_LookupThen_00367900) {
    const u32 h = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    if (h == 0 || k == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), h);
    u32 e = load<u32>(static_cast<u32>(call(ctx, 0x0076611C, {0x0106C228u, frame.addr()})));
    if (e != 0 && (load<u32>(e + 4) & 1u) == 0) e = 0;
    const u32 r = static_cast<u32>(call(ctx, 0x003677C4, {e, k}));
    if (r == 0) call(ctx, o27fn::Hash_ToString, {h});
    ctx.gpr[3] = r;
}

// void f(T* o, s32 n): each set child c of the ten o->+212[k] gets
// c->vt[40](c, k < n, 0)
DECOMP(0036E9BC, Obj_EnableFirstN_0036E9BC) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 n = arg<s32>(ctx, 1);
    for (s32 k = 0; k < 10; ++k) {
        const u32 c = load<u32>(o + 212 + 4 * static_cast<u32>(k));
        ctx.gpr[3] = c;
        if (c != 0) VCall(ctx, c, 40, {c, n > k ? 1u : 0u, 0});
    }
}

// void f(T* o, u32 i): with s = the string table 0x00ADF254[i] (i > 2 ->
// 0), 0x003AB0B8(o, HashNoCase(Format(0x00ADF148, o + 28, s)),
// HashNoCase(Format(0x00ADF158, o + 60, s)), 0, 0x003AB698, 1;
// *0x003AB698)
DECOMP(003AB6A0, Obj_BindNamed_003AB6A0) {
    const u32 o = arg<u32>(ctx, 0);
    u32 i = arg<u32>(ctx, 1);
    if (i > 2) i = 0;
    const u32 t = 0x00ADF254 + i * 4;
    const u32 s1 = static_cast<u32>(call(ctx, o27fn::String_Format, {0x00ADF148, o + 28, load<u32>(t)}));
    const u32 h1 = static_cast<u32>(call(ctx, 0x0056A710, {s1}));
    const u32 s2 = static_cast<u32>(call(ctx, o27fn::String_Format, {0x00ADF158, o + 60, load<u32>(t)}));
    const u32 h2 = static_cast<u32>(call(ctx, 0x0056A710, {s2}));
    callf(ctx, 0x003AB0B8, {o, h1, h2, 0, 0x003AB698, 1}, {LfsAt(0x003AB698)});
}

// void f(T* o): 0x0042C328(o->+4, o->+12, o->+16, 0, the eight u8 o->+20..
// +27), Flag_Set8_40000000(o), then the child o->+12 (when set) gets
// vt[40](c, 0) and is cleared
DECOMP(004387DC, Cmd_Fire_004387DC) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0042C328, {load<u32>(o + 4), load<u32>(o + 12), load<u32>(o + 16), 0, load<u8>(o + 20),
                           load<u8>(o + 21), load<u8>(o + 22), load<u8>(o + 23), load<u8>(o + 24), load<u8>(o + 25),
                           load<u8>(o + 26), load<u8>(o + 27)});
    call(ctx, o27fn::Flag_Set8_40000000, {o});
    const u32 c = load<u32>(o + 12);
    ctx.gpr[3] = 0;
    if (c == 0) return;
    VCall(ctx, c, 40, {c, 0});
    ctx.gpr[3] = 0;
    store<u32>(o + 12, 0);
}

// void f(T* o): o->+12 = 0; with u = Ui_Find(0x00AE2624, 0):
// Obj_Set4E3168(0x00AE2644, u) and o->+12 = Timeline_Play(top bit of
// o->+16 ? 0x00AE264C : 0x00AE265C, u, !(top bit of o->+8), 0); then
// Flag_Set8_40000000(o)
DECOMP(00438A3C, Cmd_PlayUi_00438A3C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 12, 0);
    const u32 u = static_cast<u32>(call(ctx, 0x004D5544, {0x00AE2624, 0}));
    if (u != 0) {
        call(ctx, 0x004E3168, {0x00AE2644, u});
        const u32 a = load<u32>(o + 8) >> 31, b = load<u32>(o + 16) >> 31;
        const u32 name = b != 0 ? 0x00AE264Cu : 0x00AE265Cu;
        store<u32>(o + 12, static_cast<u32>(call(ctx, 0x004E2EF0, {name, u, a == 0 ? 1u : 0u, 0})));
    }
    ctx.gpr[3] = call(ctx, o27fn::Flag_Set8_40000000, {o});
}

// void f(T* o): with an id o->+40, the found object (Spawn_FindNearest(
// o->+60 + 128) for 0x5E71DA39, else Obj_FindById(id)) gets +220 = o; then
// o->+52 = o->+8->+44 (when +8 is set) and, unless bit 0x04 of the u8
// o->+88 or o->+52 is set, o->vt[24](o)
DECOMP(00455BA8, Obj_Attach_00455BA8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 id = load<u32>(o + 40);
    if (id != 0) {
        const u32 f = id == 0x5E71DA39u ? static_cast<u32>(call(ctx, 0x000D6B68, {load<u32>(o + 60) + 128}))
                                        : static_cast<u32>(call(ctx, 0x000D6A28, {id, 0x5E71DA39u}));
        if (f != 0) store<u32>(f + 220, o);
    }
    const u32 p = load<u32>(o + 8);
    if (p != 0) store<u32>(o + 52, load<u32>(p + 44));
    ctx.gpr[3] = (load<u8>(o + 88) >> 2) & 1u;
    if (ctx.gpr[3] != 0) return;
    ctx.gpr[3] = load<u32>(o + 52);
    if (ctx.gpr[3] != 0) return;
    VCall(ctx, o, 24, {o});
}

// void f(Vec3 p[3]; f32 t): moves p[0] toward p[1] by p[2].x * t (the
// unit direction from 1.0f / its length), snapping to p[1] when the
// distance is no more than the step; nothing when the two already match
// bit for bit
DECOMP(004A10F0, Vec3_MoveToward_004A10F0) {
    const u32 p = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    if (load<u64>(p) == load<u64>(p + 16) && load<u32>(p + 8) == load<u32>(p + 24)) return;
    const f64 x0 = LfsAt(p), y0 = LfsAt(p + 4), z0 = LfsAt(p + 8);
    const f64 x1 = LfsAt(p + 16), y1 = LfsAt(p + 20), z1 = LfsAt(p + 24);
    const f64 dy = Sub28(y1, y0), dx = Sub28(x1, x0), dz = Sub28(z1, z0);
    const f64 step = Mul28(LfsAt(p + 32), t);
    f64 l = Mul28(dy, dy);
    l = Madd28(dx, dx, l);
    l = Madd28(dz, dz, l);
    l = static_cast<f32>(std::sqrt(l));
    const f64 inv = static_cast<f32>(LfsAt(0x004A10EC) / l);
    if (l < step || l == step) {
        const u32 w = Cp(p, p + 16);
        Cp(p + 4, p + 20);
        Cp(p + 8, p + 24);
        ctx.fpr[1] = F32Bits28(w);
        return;
    }
    const f32 nx = Madd28(Mul28(dx, inv), step, x0);
    store<f32>(p, nx);
    store<f32>(p + 4, Madd28(Mul28(dy, inv), step, y0));
    store<f32>(p + 8, Madd28(Mul28(dz, inv), step, z0));
    ctx.fpr[1] = nx;
}

// void* f(T* o, Pool* h): takes the next free slot of the 76-byte records
// h->+12 (pool header h->+4: u16 count +0, u16 next +2; the free-bit words
// h->+8 cleared for it): the record's +0 = 2 (written before its null
// check, as the original does), o's u16 first index +52 (when its u16
// count +54 was 0) and count; returns record + 4, or 0 when full
DECOMP(004B3870, Pool_Take_004B3870) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    u32 pool = load<u32>(h + 4);
    const u32 idx = load<u16>(pool + 2);
    s32 i = -1;
    if (static_cast<s32>(idx) < static_cast<s32>(load<u16>(pool))) {
        const u32 bits = load<u32>(h + 8);
        if (bits != 0) {
            store<u32>(bits + ((idx << 2) & 0x3FFFCu), 0);
            pool = load<u32>(h + 4);
        }
        store<u16>(pool + 2, static_cast<u16>(load<u16>(pool + 2) + 1));
        i = static_cast<s16>(idx);
    }
    ctx.gpr[3] = 0;
    if (i < 0 || !(i < static_cast<s32>(load<u16>(pool + 2)))) return;
    const u32 e = load<u32>(h + 12) + static_cast<u32>(i) * 76u;
    store<u32>(e, 2);
    if (e == 0) return;
    const u32 n = load<u16>(o + 54);
    if (n == 0) store<u16>(o + 52, static_cast<u16>(i));
    store<u16>(o + 54, static_cast<u16>(n + 1));
    ctx.gpr[3] = e + 4;
}

// T::T() (vtable 0x00B22128): the defaults (-1.0f in the range fields, 8 /
// 7 / 255 / 4 / 1 in the counters, 0 elsewhere) (r3 = o)
DECOMP(004B9CD8, VtB22128_Ctor_004B9CD8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kM1 = 0xBF800000u;  // -1.0f
    store<u32>(o + 12, 0);
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B22128);
    for (u32 off : {28u, 84u, 88u, 92u, 52u, 56u, 60u, 64u, 120u, 124u, 128u, 104u, 156u, 160u, 164u, 168u})
        store<u32>(o + off, 0);
    store<u32>(o + 16, 8);
    store<u32>(o + 24, kM1);
    store<u32>(o + 32, 7);
    for (u32 off : {36u, 40u, 44u, 48u, 100u}) store<u32>(o + off, kM1);
    store<u32>(o + 108, 255);
    store<u32>(o + 112, 0);
    store<u32>(o + 116, kM1);
    store<u8>(o + 185, 4);
    store<u32>(o + 188, 1);
    for (u32 off : {148u, 152u, 140u, 144u}) store<u32>(o + off, kM1);
    store<u32>(o + 132, 0);
    store<u32>(o + 136, 0);
    store<u8>(o + 20, 0);
}

// T* f(?, XmlCursor* x): Mem_Alloc(20, 0, 16) built as the 0x00B22100
// record (u16 +4 = 1) with the hash 0x03D1FCFC (+12) and the bool
// 0x2A9C853E (+16) read from x (0 when the alloc fails)
DECOMP(004C8DB4, VtB22100_NewXml_004C8DB4) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 o = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {20, 0, 16}));
    if (o != 0) {
        store<u16>(o + 4, 1);
        store<u32>(o, 0x00B22100);
        store<u32>(o + 12, 0);
        store<u8>(o + 16, 0);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x03D1FCFC, o + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadBool, {x, 0x2A9C853E, o + 16, 0});
    }
    ctx.gpr[3] = o;
}

// T::~T() (vtable 0x00B23050): DtorB23050_30 (see obj30.h)
DECOMP(004F7818, VtB23050_Dtor_004F7818) { DtorB23050_30(ctx); }

// The same body as 0x004F7818 (a second copy of the destructor)
DECOMP(004F79C0, VtB23050_Dtor_004F79C0) { DtorB23050_30(ctx); }

// void f(T* o, u32 a, const f32 m[16], ?, U* owner; f32 v): the record
// (vtable 0x00B13B38): +4 = a, u8 +8 = 0, the 3x4 rows of m (+0/+16/+32/+48
// .. +8) at +16/+32/+48/+64, +80 = v, bits 0xFF800000 of +88 = 0x00800000
// (bit 31 cleared), +92..+100 = 0, +84 = owner and the owner's u16 +178
// counted up (r3 = owner + 178 with one, else o)
DECOMP(0074C500, VtB13B38_Init_0074C500) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2), owner = arg<u32>(ctx, 4);
    const f64 v = farg(ctx, 0);
    store<u32>(o + 4, a);
    store<u8>(o + 8, 0);
    for (u32 k : {0u, 4u, 8u, 16u, 20u, 24u}) Cp(o + 16 + k, m + k);
    u32 w = load<u32>(m + 32);
    store<u32>(o, 0x00B13B38);
    store<u32>(o + 48, w);
    Cp(o + 52, m + 36);
    w = load<u32>(m + 40);
    const u32 f = load<u32>(o + 88);
    store<u32>(o + 56, w);
    Cp(o + 64, m + 48);
    Cp(o + 68, m + 52);
    const u32 w2 = load<u32>(m + 56);
    store<f32>(o + 80, static_cast<f32>(v));
    store<u32>(o + 72, w2);
    store<u32>(o + 88, (f & 0x007FFFFFu) | 0x00800000u);
    store<u32>(o + 92, 0);
    store<u32>(o + 96, 0);
    store<u32>(o + 100, 0);
    store<u32>(o + 84, owner);
    if (owner != 0) {
        store<u16>(owner + 178, static_cast<u16>(load<u16>(owner + 178) + 1));
        ctx.gpr[3] = owner + 178;
    }
}

// void f(T* d, const T* s): copies the 3x4 matrix rows +0/+16/+32/+48,
// +64..+72, +80..+88 and the words +96 / +100 / +104..+112 (raw; r3 = d,
// f1 = s->+112)
DECOMP(0074CE30, Rec_Copy_0074CE30) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    for (u32 k : {0u, 4u, 8u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 48u, 52u, 56u, 64u, 68u, 72u, 80u, 84u, 88u, 96u, 104u,
                  108u})
        Cp(d + k, s + k);
    const u32 w100 = Cp(d + 100, s + 100);
    ctx.fpr[1] = F32Bits28(Cp(d + 112, s + 112));
    ctx.gpr[5] = w100;  // r5 = s->+100, as the original leaves it
}

// ---------------------------------------------------------------- sweep: 50 instructions
// void f(T* o): with the records o->+612 (116 bytes, current o->+716):
// the counter o->+596 counts up and, once it reaches the record's u16
// limit (+180 when bit 0x40000000 of o->+516, +160 when bit 0x80000000,
// else +120), o->+592 = -1 and the counter = 0; then 0x0003E0CC(o, limit)
DECOMP(00042D5C, Obj_TickRecord_00042D5C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 base = load<u32>(o + 612);
    if (base == 0) return;
    const u32 rec = base + load<u32>(o + 716) * 116u + 92;
    const u32 n = load<u32>(o + 596) + 1;
    store<u32>(o + 596, n);
    const u32 f = load<u32>(o + 516);
    const u32 lim = load<u16>(rec + ((f & 0x40000000u) != 0 ? 88u : (f & 0x80000000u) != 0 ? 68u : 28u));
    if (!(static_cast<s32>(n) < static_cast<s32>(lim))) {
        store<u32>(o + 592, 0xFFFFFFFFu);
        store<u32>(o + 596, 0);
    }
    ctx.gpr[3] = call(ctx, 0x0003E0CC, {o, lim, 0, 0xFFFFFFFFu, rec, n});
}

// void f(): 0x00056ED0(), then, when *0x00F9C01C is set, the pair
// {0x00F2E248, 0x00F9C01C} pushed on the u16-counted vector 0x010BB854
DECOMP(000570A4, Registry_Push_000570A4) {
    call(ctx, 0x00056ED0, {arg<u32>(ctx, 0), arg<u32>(ctx, 1)});
    ctx.gpr[3] = load<u32>(0x00F9C01C);
    if (ctx.gpr[3] == 0) return;
    VecPush28(ctx, 0x010BB854, 3, [](u32 e) {
        store<u32>(e, 0x00F2E248);
        store<u32>(e + 4, 0x00F9C01C);
    });
}

// T* f(?, XmlCursor* x): Mem_Alloc(20, 0, 16) built by 0x004BB3B0(p, x) as
// the 0x00B10990 record; when XmlCursor_ReadF32(x, 0xA016BE6F, &v;
// *0x00091C04) returns 0, the f32 +16 = v and bit 0 of +12 is set (r3 = p
// or 0)
DECOMP(00091C08, VtB10990_NewXml_00091C08) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {20, 0, 16}));
    if (p != 0) {
        call(ctx, o28fn::Obj_InitVt_B21C98, {p, x});
        store<u32>(p + 12, 0);
        store<u32>(p, 0x00B10990);
        Stack frame(ctx, 16);
        callf(ctx, o28fn::XmlCursor_ReadF32, {x, 0xA016BE6Fu, frame.addr()}, {LfsAt(0x00091C04)});
        if (static_cast<u32>(ctx.gpr[3]) == 0) {
            const u32 f = load<u32>(p + 12);
            const u32 v = load<u32>(frame.addr());
            store<u32>(p + 16, v);
            store<u32>(p + 12, f | 1u);
            ctx.fpr[1] = F32Bits28(v);
        }
    }
    ctx.gpr[3] = p;
}

// T* f(?, XmlCursor* x): the 0x00B10C10 record (hash 0x3404783C, bool
// 0x55E57B9C)
DECOMP(00096A4C, VtB10C10_NewXml_00096A4C) { NewXmlHashBool30(ctx, 0x00B10C10, 0x3404783C, 0x55E57B9C, false); }

// T* f(?, XmlCursor* x): the 0x00B11320 record (hash 0x0024728B, bool
// 0x66D71193 defaulting to the current u8)
DECOMP(0009EA54, VtB11320_NewXml_0009EA54) { NewXmlHashBool30(ctx, 0x00B11320, 0x0024728B, 0x66D71193, true); }

// void f(T* o; f32 v): the state handler from the member table 0x00F64660
DECOMP(000BDAA8, Obj_StateCall_000BDAA8) { PmfDispatch30(ctx, 0x00F64660); }

// void f(T* o; f32 v): the state handler from the member table 0x00F64688
DECOMP(000BDCD8, Obj_StateCall_000BDCD8) { PmfDispatch30(ctx, 0x00F64688); }

// void f(T* o, u8 b): WorldObj_UpdateLod(o->+160, 0, 0) (when set); for b,
// itself on every element rec->+192 of the 240-byte records of the vector
// o->+340 (r3 = the vector)
DECOMP(000DD7B8, Obj_UpdateLodTree_000DD7B8) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 w = load<u32>(o + 160);
    ctx.gpr[3] = w;
    if (w != 0) call(ctx, 0x0057EE00, {w, 0, 0});
    if (b == 0) return;
    for (u32 i = 0, off = 0;; ++i, off += 240) {
        const u32 v = load<u32>(o + 340);
        ctx.gpr[3] = v;
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) return;
        const u32 e = load<u32>(load<u32>(v) + off + 192);
        if (e != 0) call(ctx, 0x000DD7B8, {e, b});
    }
}

// u32 f(T* o, u8 b): 0 when o->vt[32](o, 0) or vt[32](o, 1) is set;
// otherwise the +4 of the entry o->+20 + u16 o->+6, or 0 for b when bit
// 0x20000 of its +0 is clear
DECOMP(001293C0, Obj_EntryValue_001293C0) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    ctx.gpr[3] = 0;
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) != 0 || static_cast<u32>(VCall(ctx, o, 32, {o, 1})) != 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 e = load<u32>(o + 20) + load<u16>(o + 6);
    if (b != 0 && (load<u32>(e) & 0x20000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = load<u32>(e + 4);
}

// void f(T* o): p = o when o is set with bit 0x40000000 of +196, else 0
// (and the body runs on address 0 then, as the original does):
// Obj_SetTarget_00296758(CallK2_000D94F4(p), p->+728, 0), p->+608 +=
// (p->+216)->+488 and, once below (p->+216)->+484 with a state +740, the
// state toggles 1 <-> 2 (other values -> 1); then p->+736 = 0
DECOMP(001E4E14, Obj_TimerToggle_001E4E14) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = (o != 0 && (load<u32>(o + 196) & 0x40000000u) != 0) ? o : 0u;
    const u32 q = load<u32>(p + 216);
    const u32 r = static_cast<u32>(call(ctx, 0x000D94F4, {p}));
    call(ctx, 0x00296758, {r, load<u32>(p + 728), 0});
    const f32 t = Add28(LfsAt(p + 608), LfsAt(q + 488));
    const u32 st = load<u32>(p + 740);
    store<f32>(p + 608, t);
    ctx.gpr[3] = st;
    ctx.fpr[1] = t;
    if (st != 0 && static_cast<f64>(t) < LfsAt(q + 484)) {
        const u32 n = st == 1 ? 2u : 1u;
        store<u32>(p + 740, n);
        ctx.gpr[3] = n;
    }
    store<u32>(p + 736, 0);
}

// void f(T* o, Msg* m): m->+8 == 0x8E69AEC8 -> o->vt[304](o, 0),
// 0x7A281983 -> o->vt[304](o, 1), else 0x000DAF24(o, m)
DECOMP(0020E6DC, Obj_OnMsg_0020E6DC) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 id = load<u32>(m + 8);
    if (id == 0x8E69AEC8u) ctx.gpr[3] = VCall(ctx, o, 304, {o, 0});
    else if (id == 0x7A281983u) ctx.gpr[3] = VCall(ctx, o, 304, {o, 1});
    else ctx.gpr[3] = call(ctx, 0x000DAF24, {o, m});
}

// void f(T* o, Area* a): 0x004AE160(o->+436, a, 0) (when set); with the
// effect e = o->+432: its param 0x2C73353E cleared when it is a, param
// 0x1ABD6576 = a->+160 and Effect_SetHash4CF988(e, 0x8B3AB37D); then
// Area_Leave(o, a)
DECOMP(00224F48, Obj_LeaveArea_00224F48) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 s = load<u32>(o + 436);
    if (s != 0) call(ctx, 0x004AE160, {s, a, 0});
    const u32 e = load<u32>(o + 432);
    if (e != 0) {
        if (static_cast<u32>(call(ctx, 0x004A66D0, {e, 0x2C73353E})) == a)
            call(ctx, 0x004A65F0, {load<u32>(o + 432), 0x2C73353E, 0});
        call(ctx, 0x004A65F0, {load<u32>(o + 432), 0x1ABD6576, load<u32>(a + 160)});
        call(ctx, 0x004CF988, {load<u32>(o + 432), 0x8B3AB37Du});
    }
    ctx.gpr[3] = call(ctx, 0x0020EBC8, {o, a});
}

// void f(T* o, u8 b): for b, the links o->+424 (then +424..+432 = 0) and
// o->+548 (then 0) get bit 0x200 of their +136, and Obj_Reset100_0045C81C(
// the world slot of the s8 o->+172); then 0x000E02F8(o, b)
DECOMP(0024109C, Obj_Release_0024109C) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    if (b != 0) {
        u32 l = load<u32>(o + 424);
        if (l != 0) {
            store<u32>(l + 136, load<u32>(l + 136) | 0x200u);
            store<u32>(o + 424, 0);
            store<u32>(o + 428, 0);
            store<u32>(o + 432, 0);
        }
        l = load<u32>(o + 548);
        if (l != 0) {
            store<u32>(l + 136, load<u32>(l + 136) | 0x200u);
            store<u32>(o + 548, 0);
        }
        const s32 k = static_cast<s8>(load<u8>(o + 172));
        call(ctx, 0x0045C81C, {load<u32>(kWorldPtr) + static_cast<u32>(k) * 0x20C80u + 4208});
    }
    ctx.gpr[3] = call(ctx, 0x000E02F8, {o, b});
}

// void f(T* o, u32 x): every element e of the u16-counted list o->+64
// (count +72) whose e->+24 has +168 == x: 0x00299F74(o, e), the count
// drops and the last element moves into the slot (the next element is
// then skipped, as the original does). r3 = the final count.
DECOMP(0029F510, List_RemoveMatching_0029F510) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    for (u32 i = 0, off = 0;; ++i, off += 4) {
        const u32 n = load<u16>(o + 72);
        ctx.gpr[3] = n;
        if (!(i < n)) return;
        const u32 e = load<u32>(load<u32>(o + 64) + off);
        const u32 t = load<u32>(e + 24);
        if (t == 0 || load<u32>(t + 168) != x) continue;
        call(ctx, 0x00299F74, {o, e});
        const u32 m = (load<u16>(o + 72) - 1u) & 0xFFFFu;
        store<u16>(o + 72, static_cast<u16>(m));
        if (i == m) continue;
        const u32 v = load<u32>(o + 64);
        store<u32>(v + off, load<u32>(v + ((m << 2) & 0x3FFFCu)));
    }
}

// u32 f(T* o): the number of set bits in the mask word +132 of the
// 4056-byte world record (u8 world+4208) indexed by the u8 o->+823 (0 for
// 255; the warning String_Format(0x00B1D308, i, 12) and 0 for i >= 12)
DECOMP(002DAB04, Obj_MaskBits_002DAB04) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    const u32 i = load<u8>(o + 823);
    const u32 r = w + load<u8>(w + 4208) * 4056u;
    ctx.gpr[3] = 0;
    if (i == 255) return;
    if (i >= 12) {
        call(ctx, o27fn::String_Format, {0x00B1D308, i, 12});
        ctx.gpr[3] = 0;
        return;
    }
    u32 x = load<u32>(r + i * 4 + 132), n = 0;
    while (x != 0) {
        x &= x - 1;
        ++n;
    }
    ctx.gpr[3] = n;
}

// bool f(T* o): once (u8 o->+52 clear) with a child c = o->+48:
// c->vt[16](c); when (reloaded) c->vt[24](c) is set, c->vt[4](c) (when
// set), o->+48 = 0, the u8 +52 = 1 and returns 1
DECOMP(002FE020, Obj_FinishChild_002FE020) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = load<u8>(o + 52);
    if (ctx.gpr[3] != 0) return;
    const u32 c = load<u32>(o + 48);
    ctx.gpr[3] = c;
    if (c == 0) return;
    VCall(ctx, c, 16, {c});
    const u32 c2 = load<u32>(o + 48);
    if (static_cast<u32>(VCall(ctx, c2, 24, {c2})) == 0) return;
    const u32 c3 = load<u32>(o + 48);
    if (c3 != 0) {
        VCall(ctx, c3, 4, {c3});
        store<u32>(o + 48, 0);
    }
    store<u8>(o + 52, 1);
    ctx.gpr[3] = 1;
}

// void f(T* o): the child c = o->+32 gets c->+992..+1000 = o->+80..+88,
// or its own +432..+440 when o->vt[32](o, 0) is below 1 or (asked again)
// above 4 (raw copies; r3 = c)
DECOMP(003178D0, Obj_PickTarget_003178D0) {
    const u32 o = arg<u32>(ctx, 0);
    bool own = static_cast<s32>(VCall(ctx, o, 32, {o, 0})) < 1;
    if (!own) own = static_cast<s32>(VCall(ctx, o, 32, {o, 0})) > 4;
    const u32 c = load<u32>(o + 32);
    const u32 s = own ? c + 432 : o + 80;
    Cp(c + 992, s);
    Cp(c + 996, s + 4);
    ctx.fpr[1] = F32Bits28(Cp(c + 1000, s + 8));
    ctx.gpr[3] = c;
}

// void f(T* o, u8 b): bit 0x80 of o->+20 = b, then every element p of the
// chain o->+2576 (next +2564) gets p->vt[96](p, b)
DECOMP(003283A8, Obj_SetChainFlag_003283A8) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    u32 p = load<u32>(o + 2576);
    const u32 f = load<u32>(o + 20);
    store<u32>(o + 20, b != 0 ? (f | 0x80u) : (f & ~0x80u));
    while (p != 0) {
        VCall(ctx, p, 96, {p, b});
        p = load<u32>(p + 2564);
    }
}

// void f(T* o, s32 a, s32 b): v = (a / b) * k + k (k = *0x0036E490, single
// precision) stored to the +44 of x->+132->+48->+4->+128 for x = the +208
// of o->+204 and of o->+208 (each when set)
DECOMP(0036E498, Obj_SetRatio_0036E498) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 a = arg<s32>(ctx, 1), b = arg<s32>(ctx, 2);
    const f64 fa = static_cast<f32>(static_cast<f64>(a)), fb = static_cast<f32>(static_cast<f64>(b));
    const f64 k = LfsAt(0x0036E490);
    const f32 v = Madd28(static_cast<f32>(fa / fb), k, k);
    ctx.fpr[1] = v;
    const u32 p = load<u32>(o + 204), q = load<u32>(o + 208);
    auto put = [&](u32 x) {
        const u32 t = load<u32>(load<u32>(load<u32>(load<u32>(x + 132) + 48) + 4) + 128);
        store<f32>(t + 44, v);
        return t;
    };
    if (p != 0 && load<u32>(p + 208) != 0) put(load<u32>(p + 208));
    ctx.gpr[3] = q;
    if (q == 0) return;
    const u32 y = load<u32>(q + 208);
    ctx.gpr[3] = y;
    if (y != 0) ctx.gpr[3] = put(y);
}

// void f(T* o): o->vt[20](o, 0), Obj_Place3626E8(o, 0x01164B40, 1); with
// bit 0x80000000 of o->+208, o->+160 = (bit 0) | 4, o->vt[20](o, 1) and
// +160 |= 1; else the u16 +172 = 0 and +160 = 1
DECOMP(00370860, Obj_Reset_00370860) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 20, {o, 0});
    call(ctx, 0x003626E8, {o, 0x01164B40, 1});
    u32 v;
    if ((load<u32>(o + 208) >> 31) != 0) {
        store<u32>(o + 160, (load<u32>(o + 160) & 1u) | 4u);
        VCall(ctx, o, 20, {o, 1});
        v = load<u32>(o + 160);
    } else {
        v = load<u32>(o + 160);
        store<u16>(o + 172, 0);
        v &= 1u;
    }
    v |= 1u;
    store<u32>(o + 160, v);
    ctx.gpr[3] = v;
}

// void f(const A* a, B* b): clears one word of b's 132-byte record b->+146
// (records at b + 152): at its block (+12 * u16 rec->+0 + 23, rounded down
// to 16) + u16 +4 + 4 * the s16 entry *0x0106CF80 + 4 of record a->+6
// (0 when a->+6 is past b->+144 or that record's +0 is 10 or more, as the
// original reads it then). r3 = 0.
DECOMP(00395280, Rec_ClearSlot_00395280) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const s32 i = static_cast<s16>(load<u16>(a + 6));
    const u32 j = load<u16>(b + 146);
    u32 r1 = 0;
    if (i < static_cast<s32>(load<u16>(b + 144))) {
        const u32 rec = b + static_cast<u32>(i) * 132u + 152;
        if (static_cast<s32>(load<u32>(rec)) < 10) r1 = rec;
    }
    const u32 r2 = b + j * 132u + 152;
    const u32 v = load<u16>(r1 + load<u16>(0x0106CF80) + 4);
    const u32 blk = b + j * 132u + ((load<u16>(r2) * 12u + 23u) & ~15u) + 152;
    const u32 ea = blk + load<u16>(blk + 4) + (static_cast<u32>(static_cast<s32>(static_cast<s16>(v))) << 2);
    store<u32>(ea, 0);
    ctx.gpr[3] = 0;
    ctx.gpr[4] = ea;  // r4 = the cleared address, as the original leaves it
}

// void f(T* o): for each of the u16 o->+78 entries (8 bytes at o->+84,
// s16 index +4), the 336-byte record (*0x00F657FC)->+28[index] (when
// not -1 and set) gets +88 = +184 = 0. The original runs the whole loop
// count times over (r3 = o).
DECOMP(0039BEFC, Obj_ClearLinks_0039BEFC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u16>(o + 78);
    for (u32 pass = 0; pass < n; ++pass) {
        for (u32 k = 0; k < n; ++k) {
            const s32 idx = static_cast<s16>(load<u16>(load<u32>(o + 84) + k * 8 + 4));
            const u32 g = load<u32>(0x00F657FC);
            if (idx == -1) continue;
            const u32 rec = load<u32>(g + 28) + static_cast<u32>(idx) * 336u;
            if (rec == 0) continue;
            store<u32>(rec + 88, 0);
            store<u32>(rec + 184, 0);
        }
    }
}

// T::~T() (vtable 0x00B204E8): *0x00F65844 = 0, 0x0046B7A4(world, 0,
// 0x00F38458, o, 0); the link o->+508 (when set) goes through
// 0x002EF648(*0x00F64C38, link) and gets bit 0x1000 of its u16 +176; +260
// and +116 cleared when set; then Dtor_B219D0_00490B34(o)
DECOMP(003D1E4C, VtB204E8_Dtor_003D1E4C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B204E8);
    const u32 w = load<u32>(kWorldPtr);
    store<u32>(0x00F65844, 0);
    call(ctx, 0x0046B7A4, {w, 0, 0x00F38458, o, 0});
    const u32 l = load<u32>(o + 508);
    if (l != 0) {
        call(ctx, 0x002EF648, {load<u32>(0x00F64C38), l});
        const u32 l2 = load<u32>(o + 508);
        store<u16>(l2 + 176, static_cast<u16>(load<u16>(l2 + 176) | 0x1000u));
    }
    if (load<u32>(o + 260) != 0) store<u32>(o + 260, 0);
    if (load<u32>(o + 116) != 0) store<u32>(o + 116, 0);
    ctx.gpr[3] = call(ctx, 0x00490B34, {o});
}

// T::~T() deleting (vtable 0x00B20AE0): the owner o->+4 gets bit 0x1000 of
// its u16 +176 when bit 0x40000 of its +172 is set and is cleared, the
// PtrVectors +160 and +140, then SmallFree(o, 180)
DECOMP(004131BC, VtB20AE0_Delete_004131BC) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 w = load<u32>(o + 4);
    store<u32>(o, 0x00B20AE0);
    if (w != 0 && (load<u32>(w + 172) & 0x40000u) != 0) store<u16>(w + 176, static_cast<u16>(load<u16>(w + 176) | 0x1000u));
    store<u32>(o + 4, 0);
    VecDtor(ctx, o + 160);
    VecDtor(ctx, o + 140);
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 180});
}

// void f(T* o): with the busy u8 o->+0 set, every entry e (+4 of the
// 8-byte records o->+12, u16 count +6) whose u8 +88 has bit 0x08 gets it
// cleared and e->vt[24](e); then the busy byte is cleared (r3 = the count)
DECOMP(00454DBC, List_FlushPending_00454DBC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o, 1);
    for (u32 i = 0, off = 0;; ++i, off += 8) {
        const u32 n = load<u16>(o + 6);
        ctx.gpr[3] = n;
        if (!(i < n)) break;
        const u32 e = load<u32>(load<u32>(o + 12) + off + 4);
        const u32 b = load<u8>(e + 88);
        if ((b & 0x08u) == 0) continue;
        store<u8>(e + 88, static_cast<u8>(b & ~0x08u));
        VCall(ctx, e, 24, {e});
    }
    store<u8>(o, 0);
}

// bool f(u32 id): the index i of id among the *0x010C1A9C 152-byte
// records at 0x010BE8BC; 1 when the world's u8 +4028 + i has bit 0 (or
// (*0x00F64C20)->+36 is null) and the world's +112 is not below the
// record's +12; else 0
DECOMP(0049E268, Rec_IsReady_0049E268) {
    const u32 id = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr) + 80;
    const u32 n = load<u32>(0x010C1A9C);
    ctx.gpr[3] = 0;
    if (n == 0) return;
    u32 i = 0;
    for (;; ++i) {
        if (load<u32>(0x010BE8BC + i * 152) == id) break;
        if (!(n > i + 1)) return;
    }
    if ((load<u8>(w + i + 3948) & 1u) == 0 && load<u32>(load<u32>(0x00F64C20) + 36) != 0) return;
    if (static_cast<s32>(load<u32>(w + 32)) < static_cast<s32>(load<u32>(0x010BE8C8 + i * 152))) return;
    ctx.gpr[3] = 1;
}

// void f(T* o, Pool* h, a, b, s32 i): the record r of entry i (the 76-byte
// pool record u16 o->+68 + i of h when 0 <= i < o->+60, its +4; null
// record -> 4; otherwise r = 0x004B5708(o, h, a, b, i)), then
// 0x004B55E0(h->+16, r->+8, a, b, -1)
DECOMP(004B584C, Pool_Forward_004B584C) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const s32 i = arg<s32>(ctx, 4);
    u32 r;
    if (i >= 0 && i < static_cast<s32>(load<u32>(o + 60))) {
        const s32 k = static_cast<s16>(load<u16>(o + 68) + static_cast<u32>(i));
        u32 rec = 0;
        if (k >= 0 && k < static_cast<s32>(load<u16>(load<u32>(h + 4) + 2))) rec = load<u32>(h + 12) + static_cast<u32>(k) * 76u;
        r = rec + 4;
    } else {
        r = static_cast<u32>(call(ctx, 0x004B5708, {o, h, a, b, static_cast<u32>(i)}));
    }
    ctx.gpr[3] = call(ctx, 0x004B55E0, {load<u32>(h + 16), load<u32>(r + 8), a, b, 0xFFFFFFFFu});
}

// T* f(): Mem_Alloc(36, 0, 16) built as the 0x00B22028 record (base
// 0x00B21FD8, u16 +4 = 1) with the PtrVector +8 (u16 +14 = 4), +28 = 0
// and +32 = 0x7342860F (0 when the alloc fails)
DECOMP(004C14E0, VtB22028_New_004C14E0) {
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {36, 0, 16}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p, 0x00B21FD8);
        call(ctx, 0x005516E0, {p + 8});
        store<u16>(p + 14, 4);
        call(ctx, 0x00551598, {p + 8, 0, 0, 0});
        store<u32>(p + 28, 0);
        store<u32>(p, 0x00B22028);
        store<u32>(p + 32, 0x7342860Fu);
    }
    ctx.gpr[3] = p;
}

// T* f(XmlCursor* c, const char* name): the first sibling node named name
// (XmlCursor_NextSiblingRaw on its no-case hash) whose vt[12]() is set;
// returns that vt[12]() (asked again), or 0
DECOMP(004F9078, Xml_FindChildValue_004F9078) {
    const u32 cur = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {name}));
    u32 c = static_cast<u32>(call(ctx, 0x004F8C70, {cur, h}));
    while (c != 0) {
        if (static_cast<u32>(VCall(ctx, c, 12, {c})) != 0) {
            ctx.gpr[3] = VCall(ctx, c, 12, {c});
            return;
        }
        c = static_cast<u32>(call(ctx, 0x004F8C70, {c, h}));
    }
    ctx.gpr[3] = 0;
}

// void f(T* o, u8 a): the color from o->vt[52] (returned in a stack
// buffer) with its last byte replaced by a, passed to o->vt[56](o, color)
DECOMP(00795B68, Obj_SetColorAlpha_00795B68) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    u32 c;
    {
        Stack frame(ctx, 16);
        VCall(ctx, o, 52, {frame.addr(), o});
        c = load<u32>(frame.addr());
    }
    const u32 w = (c & 0xFFFFFF00u) | (a & 0xFFu);
    const u32 opd = load<u32>(load<u32>(o) + 56);
    // r6 = ((a << 24) | rgb) >> 8, as the original leaves it
    const u32 r6 = (((a & 0xFFu) << 24) | (c >> 8)) >> 8;
    call_ptr(ctx, opd, {o, w, opd, r6});
}

// ---------------------------------------------------------------- sweep: 51 instructions
// void f(T* o, u32 v): removes every entry whose first word is v from the
// pointer array o->+3732 (count o->+24) by swapping in the last entry
// (r3 = o)
DECOMP(0001285C, PtrArray_RemoveValue_0001285C) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 arr = o + 3732;
    for (s32 i = 0; i < static_cast<s32>(load<u32>(o + 24)); ++i) {
        if (load<u32>(load<u32>(arr + static_cast<u32>(i) * 4)) != v) continue;
        store<u32>(o + 24, load<u32>(o + 24) - 1);
        const u32 pi = arr + static_cast<u32>(i) * 4;
        const u32 n = load<u32>(o + 24);
        const u32 a = load<u32>(pi);
        const u32 b = load<u32>(arr + n * 4);
        store<u32>(arr + n * 4, a);
        store<u32>(pi, b);
        --i;
    }
}

// bool f(T* o): o->vt[84]() set and (the u8 at (o->+308)->+12 + 4 *
// ((*0x00F94B84)->+64) clear, bit 0 of the u8 o->+304 clear, or
// o->vt[152]() set)
DECOMP(00032834, Obj_CanAct_00032834) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (static_cast<u32>(VCall(ctx, o, 84, {o})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 k = load<u32>(load<u32>(0x00F94B84) + 64);
    const u32 b = load<u8>(load<u32>(load<u32>(o + 308) + 12) + k * 4);
    if (b != 0 && (load<u8>(o + 304) & 1u) != 0 && static_cast<u32>(VCall(ctx, o, 152, {o})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = 1;
}

// T* f(?, XmlCursor* x): the 0x00B0F9F0 record with the hashes 0x7C72D70C
// (+12) and 0xD4196802 (+16)
DECOMP(0007BA4C, VtB0F9F0_NewXml_0007BA4C) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        store<u32>(p, 0x00B0F9F0);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x7C72D70C, p + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0xD4196802u, p + 16, 0});
    });
}

// T::~T() (vtable 0x00B0FBF8): every set element e of the u16-counted
// PtrVector +24 gets e->vt[8](e), the vector, then
// VtB11B20_DtorTrivial_004BB41C(o)
DECOMP(0007EBF4, VtB0FBF8_Dtor_0007EBF4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B0FBF8);
    for (u32 i = 0; i < load<u16>(o + 32); ++i) {
        const u32 e = load<u32>(load<u32>(o + 24) + i * 4);
        if (e != 0) VCall(ctx, e, 8, {e});
    }
    VecDtor(ctx, o + 24);
    ctx.gpr[3] = call(ctx, 0x004BB41C, {o});
}

// T* f(?, XmlCursor* x): the 0x00B10440 record with the hash 0x8A93973F
// (+12) and the f32 0x04B70827 (+16, defaulting to its current value)
DECOMP(00089F8C, VtB10440_NewXml_00089F8C) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        store<u32>(p, 0x00B10440);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x8A93973Fu, p + 12, 0});
        callf(ctx, o28fn::XmlCursor_ReadF32, {x, 0x04B70827, p + 16}, {LfsAt(p + 16)});
    });
}

// T* f(?, XmlCursor* x): the 16-byte 0x00B110F0 record with the bools
// 0x7A281983 (u8 +12, default 1) and 0x04A4252F (u8 +13, default current)
DECOMP(0009C470, VtB110F0_NewXml_0009C470) {
    NewXml30(ctx, 16, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u8>(p + 12, 1);
        store<u8>(p + 13, 0);
        store<u32>(p, 0x00B110F0);
        call(ctx, o28fn::XmlCursor_ReadBool, {x, 0x7A281983, p + 12, 1});
        call(ctx, o28fn::XmlCursor_ReadBool, {x, 0x04A4252F, p + 13, load<u8>(p + 13)});
    });
}

// T* f(?, XmlCursor* x): the 0x00B111B8 command (AnimCmd_ConstructBase)
// with the bool 0xCB8496B5 (u8 +16, default 1) and the hash 0x7A30D7E3
// (+12, default current)
DECOMP(0009CDF0, VtB111B8_NewXml_0009CDF0) {
    NewXml30(ctx, 20, o28fn::AnimCmd_ConstructBase, false, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0);
        store<u32>(p, 0x00B111B8);
        store<u8>(p + 16, 1);
        call(ctx, o28fn::XmlCursor_ReadBool, {x, 0xCB8496B5u, p + 16, 1});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x7A30D7E3, p + 12, load<u32>(p + 12)});
    });
}

// T* f(?, XmlCursor* x): the 0x00B118F0 record with the hash 0x7A30D7E3
// (+12) and the bool 0x95932CC9 (u8 +16, default current)
DECOMP(000A53B4, VtB118F0_NewXml_000A53B4) {
    NewXml30(ctx, 20, o28fn::Obj_InitVt_B21C98, true, [&](u32 p, u32 x) {
        store<u32>(p + 12, 0);
        store<u8>(p + 16, 0);
        store<u32>(p, 0x00B118F0);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x7A30D7E3, p + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadBool, {x, 0x95932CC9u, p + 16, load<u8>(p + 16)});
    });
}

// void f(T* o, W* w, u32 bone, u8 c, u8 d): Obj_BoneMatrixCE688(o, w, i,
// c, d) with i = Skeleton_FindBone(w->+160->vt[28](), bone), or -1 for no
// bone / skeleton
DECOMP(000DAABC, Obj_BoneMatrixByName_000DAABC) {
    const u32 o = arg<u32>(ctx, 0), w = arg<u32>(ctx, 1), bone = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3) & 0xFFu, d = arg<u32>(ctx, 4) & 0xFFu;
    u32 i = 0xFFFFFFFFu;
    if (bone != 0) {
        const u32 s = load<u32>(w + 160);
        if (s != 0) {
            const u32 k = static_cast<u32>(VCall(ctx, s, 28, {s}));
            if (k != 0) i = static_cast<u32>(call(ctx, 0x005A93D8, {k, bone}));
        }
    }
    ctx.gpr[3] = call(ctx, 0x000CE688, {o, w, i, c, d});
}

// T::~T() (vtable 0x00B139B8): the children +608 (vt[4]) and +612 (vt[8])
// released and cleared, the sub-vtables +880 / +784 / +688 = 0x00B139A8 /
// 0x00B13998 / 0x00B13988, then Thunk_000D729C(o)
DECOMP(000FA214, VtB139B8_Dtor_000FA214) {
    const u32 o = arg<u32>(ctx, 0);
    u32 c = load<u32>(o + 608);
    store<u32>(o, 0x00B139B8);
    if (c != 0) {
        VCall(ctx, c, 4, {c});
        store<u32>(o + 608, 0);
    }
    c = load<u32>(o + 612);
    if (c != 0) {
        VCall(ctx, c, 8, {c});
        store<u32>(o + 612, 0);
    }
    store<u32>(o + 880, 0x00B139A8);
    store<u32>(o + 784, 0x00B13998);
    store<u32>(o + 688, 0x00B13988);
    ctx.gpr[3] = call(ctx, 0x000D729C, {o});
}

// void f(T* o, Msg* m): m->+8 == 0xCBF1C4F3 -> the slot child
// o->+364[u8 o->+345] (when the index and child are set) gets vt[28]();
// 0x5A5F3B9B -> Obj_BlendPose0FA660(o; *0x000FA9EC) and o->+596 = 0.1f
DECOMP(000FA9F0, Obj_OnMsg_000FA9F0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 id = load<u32>(m + 8);
    ctx.gpr[3] = id;
    if (id == 0xCBF1C4F3u) {
        const u32 b = load<u8>(o + 345);
        ctx.gpr[3] = b;
        if (b == 0) return;
        const u32 ea = o + ((b << 2) & 0x3FCu) + 364;
        ctx.gpr[3] = ea;
        const u32 c = load<u32>(ea);
        if (c != 0) VCall(ctx, c, 28, {c});
    } else if (id == 0x5A5F3B9Bu) {
        callf(ctx, 0x000FA660, {o}, {LfsAt(0x000FA9EC)});
        store<u32>(o + 596, 0x3DCCCCCDu);
        ctx.gpr[3] = 0x3DCCCCCDu;
    }
}

// T::~T() deleting (vtable 0x00B13CF0, base 0x00B0DAEC): the children
// +12 and +16 get vt[40](c, 0) and are cleared, then SmallFree(o, 336)
DECOMP(00109774, VtB13CF0_Delete_00109774) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 c = load<u32>(o + 12);
    store<u32>(o, 0x00B13CF0);
    if (c != 0) VCall(ctx, c, 40, {c, 0});
    const u32 c2 = load<u32>(o + 16);
    store<u32>(o + 12, 0);
    if (c2 != 0) {
        VCall(ctx, c2, 40, {c2, 0});
        store<u32>(o + 16, 0);
    }
    store<u32>(o, 0x00B0DAEC);
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 336});
}

// T::~T() deleting (vtable 0x00B14078, base 0x00B0DAEC): the children
// +16 and +12 get vt[40](c, 0) and are cleared, then SmallFree(o, 224)
DECOMP(00115DE0, VtB14078_Delete_00115DE0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 c = load<u32>(o + 16);
    store<u32>(o, 0x00B14078);
    if (c != 0) VCall(ctx, c, 40, {c, 0});
    const u32 c2 = load<u32>(o + 12);
    store<u32>(o + 16, 0);
    if (c2 != 0) VCall(ctx, c2, 40, {c2, 0});
    store<u32>(o, 0x00B0DAEC);
    store<u32>(o + 12, 0);
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 224});
}

// void f(T* o): unless (*0x00F557F0)->+28 (u8) is set, the count u8
// world+4136+i (i = the u8 world+4208 of o's world slot) goes up while
// count + the record's u8 +404 is below 255; once count + that reaches
// *0x01039AA0, Trophy_Award0B27B4(0x93D26E8C)
DECOMP(0013D530, Obj_CountTrophy_0013D530) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(load<u32>(0x00F557F0) + 28) != 0) return;
    const u32 w = load<u32>(kWorldPtr);
    const s32 k = static_cast<s8>(load<u8>(o + 172));
    const u32 i = load<u8>(w + static_cast<u32>(k) * 0x20C80u + 4208);
    const u32 cnt = w + i + 4136;
    const u32 rec = w + i * 4056u + 80;
    const u32 b = load<u8>(cnt);
    ctx.gpr[3] = b;
    if (!(static_cast<s32>(b + load<u8>(rec + 324)) < 255)) return;
    store<u8>(cnt, static_cast<u8>(b + 1));
    const u32 sum = load<u8>(rec + 324) + ((b + 1) & 0xFFu);
    ctx.gpr[3] = sum;
    if (static_cast<s32>(load<u32>(0x01039AA0)) > static_cast<s32>(sum)) return;
    ctx.gpr[3] = call(ctx, 0x000B27B4, {0x93D26E8Cu});
}

// void f(T* o, u8 on): Flag_Set172_80_000DA0CC(o, on); with the physics
// entity e = o->+388 whose world e->+8 is set exactly when on is:
// on -> Hk_RemoveEntity92FFC8(&tmp, world, e); off -> (with a body
// o->+392) Hk_AddEntity92F2A8(the game's physics world, e, 1)
DECOMP(00178548, Obj_PhysicsToggle_00178548) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    call(ctx, 0x000DA0CC, {o, on});
    const u32 e = load<u32>(o + 388);
    ctx.gpr[3] = e;
    if (e == 0) return;
    const u32 wd = load<u32>(e + 8);
    if ((wd != 0) != (on != 0)) return;
    if (on != 0) {
        Stack frame(ctx, 32);
        ctx.gpr[3] = call(ctx, 0x0092FFC8, {frame.addr(), wd, e});
        return;
    }
    const u32 pw = load<u32>(load<u32>(load<u32>(kGame30) + 0x1281D4) + 4);
    if (load<u32>(o + 392) == 0) return;
    ctx.gpr[3] = call(ctx, 0x0092F2A8, {pw, e, 1});
}

// bool f(T* o, U* h; f32 a, f32 b): 1 when bit 0x20000000 of o->+552 is
// set or not (b < *0x001C44A4 or a >= b); otherwise o->vt[420](o, 4, 1)
// and, when the u8 h->+446 and bit 0x80 of the u8 o->+604 are set,
// 0x001C3EF0(o, 0) (returns 0)
DECOMP(001C44A8, Obj_RangeGate_001C44A8) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    if ((load<u32>(o + 552) & 0x20000000u) != 0 || !(b < LfsAt(0x001C44A4) || a > b || a == b)) {
        ctx.gpr[3] = 1;
        return;
    }
    VCall(ctx, o, 420, {o, 4, 1});
    if (load<u8>(h + 446) != 0 && (load<u8>(o + 604) & 0x80u) != 0) call(ctx, 0x001C3EF0, {o, 0});
    ctx.gpr[3] = 0;
}

// bool f(T* o): 0 unless 0x001AEA50(o); then, with o->+216 set and bit
// 0x80 of the u8 o->+568 clear, o->+556 = the object Obj_FindById(o->+560)
// when it has bit 0x00400000 of +192 (else 0; its u16 +178 counted up)
// and the bit is set; returns 1
DECOMP(001D47E4, Obj_BindTarget_001D47E4) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x001AEA50, {o})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = 1;
    if (load<u32>(o + 216) == 0 || (load<u8>(o + 568) & 0x80u) != 0) return;
    const u32 t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(o + 560)}));
    const u32 r = (t != 0 && (load<u32>(t + 192) & 0x00400000u) != 0) ? t : 0u;
    store<u32>(o + 556, r);
    if (r != 0) store<u16>(r + 178, static_cast<u16>(load<u16>(r + 178) + 1));
    store<u8>(o + 568, static_cast<u8>(load<u8>(o + 568) | 0x80u));
    ctx.gpr[3] = 1;
}

// void f(T* o): with both layers o->+496 / +500: Obj_PopLayer(a, b; t)
// when Obj_HasWord368_000FC5DC(a, b) (t = *0x001EE4D4 with bit 0x00100000
// of o->+980, else *0x010535B8), b->vt[4](b); then both are cleared
DECOMP(001EE4D8, Obj_DropLayer_001EE4D8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 496);
    if (a != 0 && load<u32>(o + 500) != 0) {
        const u32 b = load<u32>(o + 500);
        const f64 t = (load<u32>(o + 980) & 0x00100000u) != 0 ? LfsAt(0x001EE4D4) : LfsAt(0x010535B8);
        if (static_cast<u32>(call(ctx, 0x000FC5DC, {a, b})) != 0)
            callf(ctx, 0x000FBB88, {load<u32>(o + 496), load<u32>(o + 500)}, {t});
        const u32 c = load<u32>(o + 500);
        if (c != 0) VCall(ctx, c, 4, {c});
    }
    store<u32>(o + 500, 0);
    store<u32>(o + 496, 0);
    ctx.gpr[3] = 0;
}

// bool f(T* o): the 3x4 rows (+0/+16/+32/+48 .. +8) of
// Target_GetDef0D951C(o)->+64 + 96 copied to o->+432.. (raw), then
// o->vt[28](o, 0, 1); returns 1
DECOMP(001F66E8, Obj_CopyDefPose_001F66E8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(static_cast<u32>(call(ctx, 0x000D951C, {o})) + 64) + 96;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) ctx.fpr[1] = F32Bits28(Cp(o + 432 + 16 * r + 4 * k, s + 16 * r + 4 * k));
    VCall(ctx, o, 28, {o, 0, 1});
    ctx.gpr[3] = 1;
}

// void f(T* o, Hit* h): AttachTarget30 when allowed (not with bit
// 0x08000000 of (o->+216)->+452 while the u8 0x01054974 is clear)
DECOMP(00201FC8, Obj_AttachTarget_00201FC8) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 r6 = load<u32>(load<u32>(o + 216) + 452);
    ctx.gpr[3] = o;
    if ((r6 & 0x08000000u) != 0 && load<u8>(0x01054974) == 0) return;
    AttachTarget30(ctx, o, h, load<u32>(o + 592), true, r6);
}

// void f(T* o, Hit* h): AttachTarget30 when allowed (as 0x00201FC8) and
// h->+64 * (o->+216)->+232 is above *0x00202094
DECOMP(00202098, Obj_AttachTargetScaled_00202098) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 216);
    ctx.gpr[3] = o;
    if ((load<u32>(q + 452) & 0x08000000u) != 0 && load<u8>(0x01054974) == 0) return;
    const f64 v = Mul28(LfsAt(h + 64), LfsAt(q + 232));
    ctx.fpr[1] = v;
    const f64 k = LfsAt(0x00202094);
    if (v < k || v == k) return;
    AttachTarget30(ctx, o, h, load<u32>(o + 592), false, 0);
}

// T::~T() (vtable 0x00B19FB0, base 0x00B19510): the buffers +180..+192
// Mem_FreeFrom(p, 0)'d and cleared, then Obj_BaseDtor_000C2DA0(o)
DECOMP(002172A8, VtB19FB0_Dtor_002172A8) {
    const u32 o = arg<u32>(ctx, 0);
    u32 p = load<u32>(o + 180);
    store<u32>(o, 0x00B19FB0);
    for (u32 off = 180; off <= 192; off += 4) {
        if (off != 180) p = load<u32>(o + off);
        if (p == 0) continue;
        call(ctx, o19fn::Mem_FreeFrom, {p, 0});
        store<u32>(o + off, 0);
    }
    store<u32>(o, 0x00B19510);
    ctx.gpr[3] = call(ctx, 0x000C2DA0, {o});
}

// void f(T* o, Area* a): 0x0021FB38(o, a) unless (bit 0x2 of o->+436, or
// the record of o->+460 is found in (*0x00F64C20)->+40) and bit
// 0x40000000 of o->+468 is set; then Area_Leave(o, a)
DECOMP(0021FF58, Obj_LeaveArea_0021FF58) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    bool skip = false;
    if ((load<u32>(o + 436) & 2u) != 0) {
        skip = (load<u32>(o + 468) & 0x40000000u) != 0;
    } else {
        const u32 r = load<u32>(o + 460);
        if (r != 0) {
            const u32 t = load<u32>(load<u32>(0x00F64C20) + 40);
            if (t != 0) {
                Stack frame(ctx, 80);
                call(ctx, 0x0049E8A8, {frame.addr(), r});
                if (static_cast<u32>(call(ctx, 0x002D9994, {t, frame.addr()})) != 0)
                    skip = (load<u32>(o + 468) & 0x40000000u) != 0;
            }
        }
    }
    if (!skip) call(ctx, 0x0021FB38, {o, a});
    ctx.gpr[3] = call(ctx, 0x0020EBC8, {o, a});
}

// bool f(T* o, u32 k): the handle o->+32 (index low half, slot high half)
// resolves in the table *0x00F557F8 (count +256, 16-byte slots +260; the
// slot's +12 holds the index in bits 15..30 and a dead bit 0x80000000)
// to a scope whose root (+8)->+0 has Scope_Lookup_00760C54(root, k)
DECOMP(00298B44, Handle_HasKey_00298B44) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    const u32 t = load<u32>(0x00F557F8);
    const u32 h = load<u32>(o + 32);
    const u32 idx = h & 0xFFFFu;
    ctx.gpr[3] = 0;
    if (idx == 0 || !(static_cast<s32>(h >> 16) < static_cast<s32>(load<u32>(t + 256)))) return;
    const u32 e = load<u32>(t + 260) + ((h >> 12) & 0x000FFFF0u);
    if (((load<u32>(e + 12) >> 15) & 0xFFFFu) != idx || (load<u8>(e + 12) & 0x80u) != 0) return;
    const u32 p = load<u32>(e + 8);
    if (p == 0) return;
    const u32 q = load<u32>(p);
    if (q == 0) return;
    const u32 r = static_cast<u32>(call(ctx, 0x00760C54, {q, k}));
    ctx.gpr[3] = r != 0 ? 1u : 0u;
}

// T::~T() (vtable 0x00B1D5C8): every set element e of the u16-counted
// PtrVector +304 goes through 0x002F6054(e) and SmallFree(e, 80), the
// vector, then Thunk_000E2A34(o)
DECOMP(002F6218, VtB1D5C8_Dtor_002F6218) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1D5C8);
    for (u32 i = 0; i < load<u16>(o + 312); ++i) {
        const u32 e = load<u32>(load<u32>(o + 304) + i * 4);
        if (e == 0) continue;
        call(ctx, 0x002F6054, {e});
        call(ctx, o19fn::SmallFree, {e, 80});
    }
    VecDtor(ctx, o + 304);
    ctx.gpr[3] = call(ctx, 0x000E2A34, {o});
}

// void f(?, u8 b): with the world's player e = (world+4208)->+8 flagged
// 0x8 in +192 and Obj_Notify24(e) set: 0x0029F5D8(Obj_Notify24(e again,
// or 0 when it lost the flag), b)
DECOMP(0030FBC0, Player_Notify_0030FBC0) {
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 e = load<u32>(load<u32>(kWorldPtr) + 4216);
    ctx.gpr[3] = e;
    if (e == 0) return;
    if ((load<u32>(e + 192) & 0x8u) == 0) return;
    if (static_cast<u32>(call(ctx, 0x000D949C, {e})) == 0) return;
    const u32 e2 = load<u32>(load<u32>(kWorldPtr) + 4216);
    const u32 a = (e2 != 0 && (load<u32>(e2 + 192) & 0x8u) != 0) ? e2 : 0u;
    const u32 r = static_cast<u32>(call(ctx, 0x000D949C, {a}));
    ctx.gpr[3] = call(ctx, 0x0029F5D8, {r, b});
}

// T::~T() deleting (vtable 0x00B1DD80, then base 0x00B1DCA8): the member
// +84 (VtB21B58_DtorTrivial), the owned effect +40 (Effect_Destruct,
// SmallFree(p, 140)) cleared, the PtrVector +44, Dtor_B219D0_00464AD0(o),
// then SmallFree(o, 128)
DECOMP(00312C8C, VtB1DD80_Delete_00312C8C) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1DD80);
    call(ctx, o29fn::VtB21B58_DtorTrivial, {o + 84});
    const u32 e = load<u32>(o + 40);
    store<u32>(o, 0x00B1DCA8);
    if (e != 0) {
        call(ctx, 0x004C8E80, {e});
        call(ctx, o19fn::SmallFree, {e, 140});
        store<u32>(o + 40, 0);
    }
    VecDtor(ctx, o + 44);
    call(ctx, 0x00464AD0, {o});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 128});
}

// void f(T* o): the u8 o->+84 = 1; each set child c of the three
// (o->+32)->+776[k] goes through 0x001F4460(c) and c->vt[88](c, 1, 0, 0,
// 0); then Field_SetByteClear4_004A090C(o->+32 + 640, 0)
DECOMP(00316548, Obj_ResetChildren_00316548) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 32);
    store<u8>(o + 84, 1);
    for (u32 k = 0; k < 3; ++k) {
        const u32 slot = p + 776 + 4 * k;
        if (load<u32>(slot) == 0) continue;
        call(ctx, 0x001F4460, {load<u32>(slot)});
        const u32 c = load<u32>(slot);
        VCall(ctx, c, 88, {c, 1, 0, 0, 0});
    }
    ctx.gpr[3] = call(ctx, 0x004A090C, {p + 640, 0});
}

// T::~T() (vtable 0x00B1EB70): 0x00334114(o), the PtrVectors +2652,
// +2632, +2612 and +2592, then Thunk_00326A38(o)
DECOMP(00334304, VtB1EB70_Dtor_00334304) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1EB70);
    call(ctx, 0x00334114, {o});
    for (u32 off : {2652u, 2632u, 2612u, 2592u}) VecDtor(ctx, o + off);
    ctx.gpr[3] = call(ctx, 0x00326A38, {o});
}

// void f(T* o, a, Msg* m): in state 4 (u8 o->+18) on a type 6 message the
// timer o->+2572 -= m->+24; once at or below *0x003415C0 it is cleared
// and o->vt[104](o, 5, 0); then 0x003252D8(o, a, m)
DECOMP(003415C8, Obj_OnTick_003415C8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    if (load<u8>(o + 18) == 4 && load<u32>(m) == 6) {
        const f32 t = Sub28(LfsAt(o + 2572), LfsAt(m + 24));
        store<f32>(o + 2572, t);
        ctx.fpr[1] = t;
        const f64 k = LfsAt(0x003415C0);
        if (t < k || t == k) {
            store<u32>(o + 2572, 0);
            VCall(ctx, o, 104, {o, 5, 0});
        }
    }
    ctx.gpr[3] = call(ctx, 0x003252D8, {o, a, m});
}

// bool f(const Rec* s): copies the 32-byte s into the first free one of
// the sixteen 40-byte slots at 0x010AA394 (u8 +32 = 1 in use, +36 = 0),
// with the f32 +4..+12 floored to *0x0042A240 when negative (fsel);
// r3 = 1, or s when all are in use
DECOMP(0042A248, Slots_Add_0042A248) {
    const u32 s = arg<u32>(ctx, 0);
    u32 e = 0;
    for (u32 i = 0;; ++i) {
        if (i >= 16) return;
        e = 0x010AA394 + 40 * i;
        if (load<u8>(e + 32) == 0) break;
    }
    const u32 v[3] = {load<u32>(s + 4), load<u32>(s + 8), load<u32>(s + 12)};
    for (u32 k = 0; k < 32; k += 4) Cp(e + k, s + k);
    const u32 dflt = load<u32>(0x0042A240);
    store<u32>(e + 36, 0);
    store<u8>(e + 32, 1);
    u32 w[3];
    for (u32 k = 0; k < 3; ++k) {
        w[k] = std::bit_cast<f32>(v[k]) >= 0.0f ? v[k] : dflt;
        store<u32>(e + 4 + 4 * k, w[k]);
    }
    ctx.fpr[1] = F32Bits28(w[0]);
    ctx.gpr[3] = 1;
}

// bool f(T* o; f32 dt): the timer o->+32 += dt; below its length +28,
// 0x0048C6EC(world, 1; o->+12, o->+16, v) with v = t * |+20 - +24| + +24
// (or *0x0042E138 - t * |+20 - +24| with the u8 +9 set) for t = timer /
// length, returning 0; else 0x0048C6EC(world, u8 +9; +12, +16, +20),
// returning 1
DECOMP(0042E140, Fade_Step_0042E140) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const f32 t = Add28(LfsAt(o + 32), dt);
    const f64 len = LfsAt(o + 28), a = LfsAt(o + 12), b = LfsAt(o + 16);
    store<f32>(o + 32, t);
    if (t > len || t == len) {
        const u32 flag = load<u8>(o + 9);
        callf(ctx, 0x0048C6EC, {load<u32>(kWorldPtr), flag}, {a, b, LfsAt(o + 20)});
        ctx.gpr[3] = 1;
        return;
    }
    const f64 u = static_cast<f32>(t / len);
    const f64 y = LfsAt(o + 24);
    const f64 d = std::fabs(static_cast<f64>(Sub28(LfsAt(o + 20), y)));
    f64 v = Madd28(u, d, y);
    if (load<u8>(o + 9) != 0) v = Nmsub29(u, d, LfsAt(0x0042E138));
    callf(ctx, 0x0048C6EC, {load<u32>(kWorldPtr), 1}, {a, b, v});
    ctx.gpr[3] = 0;
}

// void f(Cmd* o): with t = Obj_FindByHandle42C2B0(o->+12, 0):
// t->vt[176](t, 0) when bit 0x2 of t->+172 and the u8 o->+21 are set, then
// t->vt[128](t, o->+16, u8 o->+20) (entry in r6, OPD in r7); then
// Flag_Set8_40000000(o)
DECOMP(0042EAEC, Cmd_Apply_0042EAEC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(o + 12), 0}));
    if (t != 0) {
        if ((load<u32>(t + 172) & 2u) != 0 && load<u8>(o + 21) != 0) VCall(ctx, t, 176, {t, 0});
        const u32 a = load<u32>(o + 16), b = load<u8>(o + 20);
        const u32 opd = load<u32>(load<u32>(t) + 128);
        call_ptr(ctx, opd, {t, a, b, load<u32>(opd), opd});
    }
    ctx.gpr[3] = call(ctx, o27fn::Flag_Set8_40000000, {o});
}

// T* f(T* o, const u32 src[29], u8 on): the 0x00B20F08 command (base
// 0x0042C9D0(o, on)) with the 116 bytes of src at +12, u32 +128 = 0 and
// u16 +130 = 0 (r3 = 0)
DECOMP(0042F854, VtB20F08_Init_0042F854) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), on = arg<u32>(ctx, 2) & 0xFFu;
    call(ctx, o27fn::Obj_InitVt_B20CC8, {o, on});
    store<u32>(o, 0x00B20F08);
    for (u32 k = 0; k < 116; k += 4) Cp(o + 12 + k, src + k);
    store<u32>(o + 128, 0);
    store<u16>(o + 130, 0);
    ctx.gpr[3] = 0;
}

// void f(T* o, a, b, c, -, -, bool p, bool q, u8 on; f32 x, f32 y): the
// 0x00B20F80 command (base 0x0042C9D0(o, on)): +16 = a, +20 = b, +12 = 0,
// +24 = c, +28 = x, +32 = y and bits 0x80000000 / 0x40000000 of +36 = p /
// q (r3 = the new +36)
DECOMP(004306F4, VtB20F80_Init_004306F4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 p = arg<u32>(ctx, 6), q = arg<u32>(ctx, 7);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1);
    const u32 on = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116) & 0xFFu;  // the 9th argument's slot
    call(ctx, o27fn::Obj_InitVt_B20CC8, {o, on});
    u32 f = load<u32>(o + 36) & 0x7FFFFFFFu;
    store<u32>(o + 16, a);
    f |= p << 31;
    store<u32>(o + 20, b);
    store<u32>(o, 0x00B20F80);
    f = (f & ~0x40000000u) | ((q & 1u) << 30);
    store<u32>(o + 12, 0);
    store<u32>(o + 24, c);
    store<f32>(o + 28, static_cast<f32>(x));
    store<f32>(o + 32, static_cast<f32>(y));
    store<u32>(o + 36, f);
    ctx.gpr[3] = f;
}

// bool f(T* o): 1 with bit 0x80 of the u8 o->+556; 0 without bit 0x40 or
// when the world's vt[32](w, 1) is 12; otherwise 1, after
// Letterbox_Show(world, bit 0x20) when bit 0x10 is set
DECOMP(00433F04, Obj_LetterboxGate_00433F04) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 556);
    if ((b & 0x80u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    ctx.gpr[3] = 0;
    if ((b & 0x40u) == 0) return;
    const u32 w = load<u32>(kWorldPtr);
    if (static_cast<u32>(VCall(ctx, w, 32, {w, 1})) - 12u == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 b2 = load<u8>(o + 556);
    if ((b2 & 0x10u) != 0) call(ctx, 0x00470590, {load<u32>(kWorldPtr), (b2 & 0x20u) != 0 ? 1u : 0u});
    ctx.gpr[3] = 1;
}

// bool f(s32 i): 0 unless the u8 0x010AEC60 is set and i != -1; else
// 0x006D56E8(the s32 table 0x00F65AB0[i]) != 0 (for i == 2, entries 2 or
// 30)
DECOMP(0044D2E0, Pad_IsDown_0044D2E0) {
    const s32 i = arg<s32>(ctx, 0);
    auto test = [&](u32 ea) {
        const s64 v = static_cast<s32>(load<u32>(ea));
        return static_cast<u32>(call(ctx, 0x006D56E8, {static_cast<u64>(v)})) != 0;
    };
    if (load<u8>(0x010AEC60) == 0 || i == -1) {
        ctx.gpr[3] = 0;
        return;
    }
    if (i != 2) {
        ctx.gpr[3] = test(0x00F65AB0 + static_cast<u32>(i) * 4) ? 1u : 0u;
        return;
    }
    ctx.gpr[3] = (test(0x00F65AB8) || test(0x00F65B28)) ? 1u : 0u;
}

// void f(T* o): every element e of the u16-counted lists o->+128 (count
// +136) and o->+148 (count +156) goes through 0x00933120(e->+24->+8, e),
// then the counts are cleared (r3 = the second count read)
DECOMP(004AD088, Obj_ReleaseBodies_004AD088) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 v : {128u, 148u}) {
        for (u32 i = 0;; ++i) {
            const u32 n = load<u16>(o + v + 8);
            ctx.gpr[3] = n;
            if (!(i < n)) break;
            const u32 e = load<u32>(load<u32>(o + v) + i * 4);
            call(ctx, 0x00933120, {load<u32>(load<u32>(e + 24) + 8), e});
        }
        const u32 n = static_cast<u32>(ctx.gpr[3]);
        store<u16>(o + v + 8, 0);
        ctx.gpr[3] = n;
    }
}

// T* f(?, XmlCursor* x): Mem_Alloc(20, 0, 16) built as the 0x00B22448
// record (u16 +4 = 1) with the hashes 0xE42AC27C (+12) and 0x8A93973F
// (+16, default current) (0 when the alloc fails)
DECOMP(004CB0E8, VtB22448_NewXml_004CB0E8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {20, 0, 16}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p, 0x00B22448);
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0xE42AC27Cu, p + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x8A93973Fu, p + 16, load<u32>(p + 16)});
    }
    ctx.gpr[3] = p;
}

// T* f(?, XmlCursor* x): Mem_Alloc(20, 0, 16) built as the 0x00B224E8
// record (u16 +4 = 1) with the hashes 0x9178C4CB (+12) and 0x8A93973F
// (+16) (0 when the alloc fails)
DECOMP(004CC328, VtB224E8_NewXml_004CC328) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {20, 0, 16}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p, 0x00B224E8);
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x9178C4CBu, p + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x8A93973Fu, p + 16, 0});
    }
    ctx.gpr[3] = p;
}

// T::~T() deleting (vtables 0x00B12898 / +380 0x00B12A44 / +384
// 0x00B12A6C): DtorB12890_29, then Mem_FreeFrom(o, 0)
DECOMP(00747764, VtB12898_Delete_00747764) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    DtorB12890_29(ctx);
    ctx.gpr[3] = call(ctx, o19fn::Mem_FreeFrom, {o, 0});
}

// T::~T() deleting (the same class): DtorB12890_29, then SmallFree(o, 432)
DECOMP(00747D88, VtB12898_Delete_00747D88) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    DtorB12890_29(ctx);
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 432});
}

// T::~T() deleting (vtable 0x00B1C040, base 0x00B0DAAC): the buffer
// o->+56 handed back to its allocator o->+60 (vt[8](a, p, u16 +48 * (u16
// +54 >> 1))), the array +48, the entry o + 4 removed from the sorted map
// 0x01056F04, then SmallFree(o, 72)
DECOMP(00762FC0, VtB1C040_Delete_00762FC0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 size = load<u16>(o + 48) * ((load<u16>(o + 54) >> 1) & 0x7FFFu);
    store<u32>(o, 0x00B1C040);
    const u32 p = load<u32>(o + 56);
    if (p != 0) {
        const u32 a = load<u32>(o + 60);
        VCall(ctx, a, 8, {a, p, size});
    }
    call(ctx, 0x0054EC94, {o + 48});
    store<u32>(o, 0x00B0DAAC);
    call(ctx, 0x007330C0, {0x01056F04, o + 4});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 72});
}

// T::~T() deleting (vtable 0x00B1F280, then 0x00B13974): the skeletons
// +104, +88 and +84 released (AnimSkel_Release7D4EA8) and cleared, then
// the r13 pool (-28332)->+44->vt[12](pool, o, u16 o->+4)
DECOMP(007705B8, VtB1F280_Delete_007705B8) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    u32 s = load<u32>(o + 104);
    store<u32>(o, 0x00B1F280);
    if (s != 0) call(ctx, 0x007D4EA8, {s});
    s = load<u32>(o + 88);
    store<u32>(o + 104, 0);
    if (s != 0) call(ctx, 0x007D4EA8, {s});
    s = load<u32>(o + 84);
    store<u32>(o + 88, 0);
    if (s != 0) call(ctx, 0x007D4EA8, {s});
    store<u32>(o + 84, 0);
    store<u32>(o, 0x00B13974);
    const u32 pool = Pool28332(ctx);
    ctx.gpr[3] = VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}
