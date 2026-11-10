// Game code, part 27 (continued): more of the small-function sweep, from
// the 39-instruction functions on. See obj27.cpp for the conventions.
#include "obj27.h"

using namespace sly4;
using namespace sly4::obj27;

// ---------------------------------------------------------------- sweep: 39 instructions (cont.)
// void f(): r = 0x00446984(0x010B9EEC->*, 0): Music_SetNode449E48(r, 0, 0)
// when Ptr_DerefNonZero(r), else Emitter_StopGlobal(1); RefNode_Release(r);
// then bits 0x60 set in the world's u8 +44 (r3 / r4 / r5 left)
DECOMP(0048A4E0, Music_Restart_0048A4E0) {
    const u32 r = static_cast<u32>(call(ctx, 0x00446984, {load<u32>(0x010B9EEC), 0}));
    if (static_cast<s32>(call(ctx, 0x00446960, {r})) != 0) call(ctx, 0x00449E48, {r, 0, 0});
    else call(ctx, 0x00449E10, {1});
    call(ctx, o24fn::RefNode_Release, {r});
    const u32 g = load<u32>(0x00F65CA0);
    const u32 b = load<u8>(g + 44) | 0x60u;
    store<u8>(g + 44, static_cast<u8>(b));
    ctx.gpr[3] = g;
    ctx.gpr[4] = 1;
    ctx.gpr[5] = b;
}

// void f(T* o): unless +20 has bit 0x00400000 or +28 has bit 2,
// 0x00786D94(the 24-byte record i of 0x010BB754, &(o + 56)) for i = 0..6
DECOMP(00490518, Obj_Register7_00490518) {
    Stack frame(ctx, 144);
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if ((load<u32>(o + 20) & 0x00400000u) != 0 || (load<u32>(o + 28) & 2u) != 0) return;  // r3 = o
    const u32 slot = frame.addr() + 112;
    for (u32 i = 0; i < 7; ++i) {
        store<u32>(slot, o + 56);
        call(ctx, 0x00786D94, {0x010BB754u + 24 * i, slot});
    }
}

// The 24-byte value-holder clone shape: p = SmallAlloc(24) (vtable vt, u16
// +4 = 1, +20 = 0); +20 = o->vfn[slot](o) (as an f32 when flt), +8 =
// o->+8. A failed SmallAlloc still writes them to 20 and 8, as in the
// original. Returns p.
static void CloneHolder27(PPUContext& ctx, u32 vt, u32 slot, bool flt) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {24}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p + 20, 0);
        store<u32>(p, vt);
    }
    const u32 r = static_cast<u32>(VCall(ctx, o, slot, {o}));
    if (flt) {
        const u32 w = load<u32>(o + 8);
        store<f32>(p + 20, static_cast<f32>(ctx.fpr[1]));
        store<u32>(p + 8, w);
    } else {
        store<u32>(p + 20, r);
        store<u32>(p + 8, load<u32>(o + 8));
    }
    ctx.gpr[3] = p;
}
// T* f(T* o): CloneHolder27 (vtable 0x00B21CD8, the u32 of vfn[28])
DECOMP(004BEA10, VtB21CD8_Clone) { CloneHolder27(ctx, 0x00B21CD8, 28, false); }
// T* f(T* o): CloneHolder27 (vtable 0x00B21D28, the u32 of vfn[24])
DECOMP(004BEBC0, VtB21D28_Clone) { CloneHolder27(ctx, 0x00B21D28, 24, false); }
// T* f(T* o): CloneHolder27 (vtable 0x00B21D78, the f32 of vfn[20])
DECOMP(004BEDB8, VtB21D78_Clone) { CloneHolder27(ctx, 0x00B21D78, 20, true); }

// void f(T* o, u32 v): bits 0x00C00000 of +136 = the low 2 bits of v; then
// each of the 9 children +204.. gets vfn[64](child, v)
DECOMP(004E83B8, Ui_SetState_004E83B8) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    store<u32>(o + 136, (load<u32>(o + 136) & ~0x00C00000u) | ((v << 22) & 0x00C00000u));
    for (u32 i = 0; i < 9; ++i) {
        const u32 c = load<u32>(o + 204 + 4 * i);
        ctx.gpr[3] = c;
        if (c != 0) VCall(ctx, c, 64, {c, v});
    }
}

// The body of the 0x00B12260-based destructors: vtable vt, ~PtrVector(o +
// off), vtable 0x00B12260, then the owned PtrVector +16 (20 bytes)
// destroyed and freed when set (r3 = 0x00B12260 otherwise).
static void VtB12260DtorBody(PPUContext& ctx, u32 vt, u32 off = 180) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    VecDtor(ctx, o + off);
    const u32 v = load<u32>(o + 16);
    store<u32>(o, 0x00B12260);
    ctx.gpr[3] = 0x00B12260;
    if (v == 0) return;
    VecDtor(ctx, v);
    call(ctx, o19fn::SmallFree, {load<u32>(o + 16), 20});
}
// T::~T: VtB12260DtorBody with the vtable 0x00B138E8
DECOMP(007468F8, VtB138E8_Dtor) { VtB12260DtorBody(ctx, 0x00B138E8); }
// T::~T: VtB12260DtorBody with the vtable 0x00B15008
DECOMP(0074F9D8, VtB15008_Dtor) { VtB12260DtorBody(ctx, 0x00B15008); }
// T::~T: VtB12260DtorBody with the vtable 0x00B1A758 and the vector at +188
DECOMP(00759B20, VtB1A758_Dtor) { VtB12260DtorBody(ctx, 0x00B1A758, 188); }

// T::~T deleting form: vtables 0x00B1F23C (+20) / 0x00B1F228 (+8) /
// 0x00B13974 / 0x00B1F26C (+16) / 0x00B1F250 (+12), then the r13 pool
// (-28332)->+44->vfn[12](pool, o, the u16 +4). Null does nothing (r3 = 0).
DECOMP(00770F08, VtB1F228_Delete5Vt_00770F08) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o + 20, 0x00B1F23C);
    store<u32>(o + 8, 0x00B1F228);
    store<u32>(o, 0x00B13974);
    store<u32>(o + 16, 0x00B1F26C);
    store<u32>(o + 12, 0x00B1F250);
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}

// The 0x00B1F404 deleting destructor body (as 0x0077153C) for this = o +
// delta: vtables 0x00B1F404 / 0x00B1F434 (+8) / 0x00B1F44C (+12) /
// 0x00B1F464 (+16), HkBase_Dtor940F24(o), then the r13 pool
// (-28332)->+44->vfn[12](pool, o, the u16 +4). A null o does nothing (r3
// = this).
static void VtB1F404DeleteAdj(PPUContext& ctx, u32 delta) {
    const u32 o = arg<u32>(ctx, 0) - delta;
    if (o == 0) return;
    store<u32>(o, 0x00B1F404);
    store<u32>(o + 8, 0x00B1F434);
    store<u32>(o + 12, 0x00B1F44C);
    store<u32>(o + 16, 0x00B1F464);
    call(ctx, 0x00940F24, {o});
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}
// T::~T deleting form for this = o + 16 (VtB1F404DeleteAdj)
DECOMP(007715D4, VtB1F404_DeleteAdj16) { VtB1F404DeleteAdj(ctx, 16); }
// T::~T deleting form for this = o + 12 (VtB1F404DeleteAdj)
DECOMP(00771670, VtB1F404_DeleteAdj12) { VtB1F404DeleteAdj(ctx, 12); }
// T::~T deleting form for this = o + 8 (VtB1F404DeleteAdj)
DECOMP(0077170C, VtB1F404_DeleteAdj8) { VtB1F404DeleteAdj(ctx, 8); }

// ---------------------------------------------------------------- sweep: 40 instructions
// void f(T* o, f32 v): unless o->vfn[124](o) or bits 0x40 / 0x20 of
// o->+48->+172, o->+48->vfn[52](it) with f1 = v
DECOMP(00013358, Obj_ApplyValue_00013358) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0);
    if (static_cast<u32>(VCall(ctx, o, 124, {o})) != 0) return;
    const u32 a = load<u32>(o + 48);
    ctx.gpr[3] = a;
    if ((load<u32>(a + 172) & 0x60u) != 0) return;
    ctx.fpr[1] = v;
    VCall(ctx, a, 52, {a});
}

// bool f(T* o): 0 unless bit 1 of the u8 +304; 1 unless bit 2 of the u8
// +465, the target +256 and bit 0x40000 of its +172; else whether the f32
// +288 is below the larger of the two per-language values (indices at
// 0x00F9521C and 0x00F94E74) in o->+308->+12
DECOMP(00032794, Obj_InRange_00032794) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 304) & 1u;
    ctx.gpr[3] = b;
    if (b == 0 || (load<u8>(o + 465) & 2u) == 0) return;
    const u32 t = load<u32>(o + 256);
    if (t == 0 || (load<u32>(t + 172) & 0x40000u) == 0) return;
    const u32 tab = load<u32>(load<u32>(o + 308) + 12);
    const f64 f = static_cast<f64>(load<f32>(o + 288));
    const f64 x = static_cast<f64>(load<f32>(tab + IndexOff(0x00F9521C)));
    const f64 y = static_cast<f64>(load<f32>(tab + IndexOff(0x00F94E74)));
    const f64 m = Fsel24(static_cast<f64>(static_cast<f32>(y - x)), y, x);
    ctx.fpr[1] = f;
    ctx.fpr[2] = m;
    ctx.gpr[3] = f < m ? 1u : 0u;
}

// bool f(T* o): true when o->vfn[156](o) is 0, the f32 +432 is not above
// the constant at 0x00032900, bit 0x10 of the u8 +305 is clear and the
// kind +4 is not 36, 37 or 40
DECOMP(00032908, Obj_CanAct_00032908) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = 0;
    if (static_cast<u32>(VCall(ctx, o, 156, {o})) == 0) {
        ctx.fpr[1] = static_cast<f64>(load<f32>(o + 432));  // f1 / f2 left = the value and the limit
        ctx.fpr[2] = static_cast<f64>(load<f32>(0x00032900));
        if (!(ctx.fpr[1] > ctx.fpr[2]) && (load<u8>(o + 305) & 0x10u) == 0) {
            const s32 k = load<s32>(o + 4);
            r = (k == 40 || k == 36 || k == 37) ? 0u : 1u;
        }
    }
    ctx.gpr[3] = r;
}

// void f(T* o): a random value (the generator's old low 16 bits times the
// constant at 0x00033998, stepping it) below the per-language threshold
// (index at 0x00F94F94) in o->+308->+12 sets bit 0x40 of the u8 +464
DECOMP(000339A0, Obj_RandomFlag_000339A0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = static_cast<f64>(load<f32>(0x00033998));
    const s64 last = load<s64>(update::kRandomLast);
    update::Random_Step();
    const f64 r = static_cast<f64>(static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(last))) * k));
    const f64 t = static_cast<f64>(load<f32>(load<u32>(load<u32>(o + 308) + 12) + IndexOff(0x00F94F94)));
    ctx.fpr[1] = r;
    ctx.fpr[2] = t;
    if (r < t) store<u8>(o + 464, static_cast<u8>(load<u8>(o + 464) | 0x40u));
}

// void f(T* o, u8 a, u8 b): o->+656->vfn[40](it, a, 1) and
// o->+660->vfn[40](it, b, 1), each when set
DECOMP(00043A80, Obj_Set40Pair_00043A80) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    const u32 p = load<u32>(o + 656);
    ctx.gpr[3] = p;
    if (p != 0) VCall(ctx, p, 40, {p, a, 1});
    const u32 q = load<u32>(o + 660);
    if (q != 0) VCall(ctx, q, 40, {q, b, 1});
}

// The component flag shape (o, e): with c = Effect_FindComponent(e,
// o->+12) of kind 2 (vfn[32]), c->+20 = the world flag bit (u8 at off).
static void ComponentFlag27(PPUContext& ctx, u32 off, u32 bit) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 h = load<u32>(o + 12);
    ctx.gpr[3] = h;
    if (h == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {e, h}));
    if (c == 0) return;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) != 2) return;
    const u32 v = (load<u8>(load<u32>(0x00F65CA0) + off) & bit) != 0 ? 1u : 0u;
    store<u32>(c + 20, v);
    ctx.gpr[3] = v;
}
// void f(T* o, Effect* e): ComponentFlag27 with bit 0x10 of the world's u8 +45
DECOMP(0007B8D4, Effect_SetWorldFlag_0007B8D4) { ComponentFlag27(ctx, 45, 0x10); }
// void f(T* o, Effect* e): ComponentFlag27 with bit 0x02 of the world's u8 +44
DECOMP(0007C670, Effect_SetWorldFlag_0007C670) { ComponentFlag27(ctx, 44, 0x02); }

// void f(T* o, X* x): with b = Effect_Bind4BB7E0(x, o->+32, 0) of kind
// (+276) 0x02235C33 whose target +280 has bit 2 of +192,
// 0x000E1288(target, o + 12, x)
DECOMP(00084584, Effect_Forward_00084584) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 b = static_cast<u32>(call(ctx, 0x004BB7E0, {x, load<u32>(o + 32), 0}));
    if (b == 0 || load<u32>(b + 276) != 0x02235C33u) return;
    const u32 t = load<u32>(b + 280);
    ctx.gpr[3] = t;
    if (t == 0 || (load<u32>(t + 192) & 2u) == 0) return;
    call(ctx, 0x000E1288, {t, o + 12, x});
}

// void f(T* o, X* x): with b = Effect_Bind4BB7E0(x, o->+12, 0) of kind
// 0x02235C33 and its target +280 having a kind-34 component c: bit
// 0x00200000 of c->+148 = bit 0 of the u8 o->+24, c->+152 / +156 = o->+16 /
// +20
DECOMP(0008A260, Effect_ConfigTarget_0008A260) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 b = static_cast<u32>(call(ctx, 0x004BB7E0, {x, load<u32>(o + 12), 0}));
    if (b == 0 || load<u32>(b + 276) != 0x02235C33u) return;
    const u32 t = load<u32>(b + 280);
    ctx.gpr[3] = t;
    if (t == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288, {t, 34, 0}));
    if (c == 0) return;
    const u32 f = load<u8>(o + 24);
    const u32 w = (load<u32>(c + 148) & ~0x00200000u) | ((f & 1u) << 21);
    const u32 a = load<u32>(o + 16), d = load<u32>(o + 20);
    store<u32>(c + 152, a);
    store<u32>(c + 156, d);
    store<u32>(c + 148, w);
}

// void f(T* o): when +12 and +16 are set, pushes a new 32-byte VtB20EF0
// command (VtB20EF0_Ctor(p, +12, +16, +20, +24, the u8 +28)); else r3 = 0
DECOMP(0009DD54, Obj_PushNewB20EF0_0009DD54) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 12) == 0 || load<u32>(o + 16) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    PushNew26(ctx, 32, [&](u32 p) {
        call(ctx, 0x0042F588, {p, load<u32>(o + 12), load<u32>(o + 16), load<u32>(o + 20), load<u32>(o + 24), load<u8>(o + 28)});
    });
}

// void f(T* o): pushes a new 32-byte 0x00437F24 command (p, +12, the u8s
// +28 / +29, +16, +20, the u8s +30 / +31, then on the stack the u8 +32 and
// +24)
DECOMP(000A3FD8, Obj_PushNew437F24_000A3FD8) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew26(ctx, 32, [&](u32 p) {
        call(ctx, 0x00437F24, {p, load<u32>(o + 12), load<u8>(o + 28), load<u8>(o + 29), load<u32>(o + 16), load<u32>(o + 20),
                               load<u8>(o + 30), load<u8>(o + 31), load<u8>(o + 32), load<u32>(o + 24)});
    });
}

// The 18-hash flag table 0x00F6453C: the index of h, or -1.
static s32 FlagIndex27(u32 h) {
    for (u32 i = 0; i < 18; ++i)
        if (load<u32>(0x00F6453C + 4 * i) == h) return static_cast<s32>(i);
    return -1;
}

// void f(T* o, u32 h, u8 on): sets (on == 1) or clears bit (index of h in
// the table 0x00F6453C) of +348; an unknown nonzero h gets Hash_ToString
// (r3 = o)
DECOMP(000AE2BC, Obj_SetHashFlag_000AE2BC) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), on = arg<u32>(ctx, 2) & 0xFFu;
    if (h == 0) return;
    const s32 i = FlagIndex27(h);
    if (i < 0) {
        call(ctx, o27fn::Hash_ToString, {h});
        return;
    }
    const u32 bit = 1u << i;
    store<u32>(o + 348, on == 1 ? (load<u32>(o + 348) | bit) : (load<u32>(o + 348) & ~bit));
    ctx.gpr[3] = o;
}

// bool f(T* o, u32 h): whether bit (index of h in the table 0x00F6453C) of
// +348 is set; an unknown nonzero h gets Hash_ToString (and 0)
DECOMP(000AE35C, Obj_GetHashFlag_000AE35C) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    if (h == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const s32 i = FlagIndex27(h);
    if (i < 0) {
        call(ctx, o27fn::Hash_ToString, {h});
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = (load<u32>(o + 348) & (1u << i)) != 0 ? 1u : 0u;
}

// void f(T* o): o->vfn[28](o, 0, 2), o->vfn[28](o, 1, 0), then the f32 at
// o->+20 + the u16 +6 = (o->+216, read first)->+488
DECOMP(000BC728, Obj_ResetValue_000BC728) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(o + 216);
    VCall(ctx, o, 28, {o, 0, 2});
    VCall(ctx, o, 28, {o, 1, 0});
    const u32 a = load<u32>(o + 20) + load<u16>(o + 6);
    const u32 w = load<u32>(t + 488);
    store<u32>(a, w);
    ctx.gpr[3] = a;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
}

// T::~T deleting form: vtable 0x00B13E58, +12->vfn[40](it, 0) when set, +12
// = 0, 0x009F6A60(o + 16), vtable 0x00B0DAEC, SmallFree(o, 272). Null does
// nothing (r3 = 0).
DECOMP(001118C8, VtB13E58_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(o + 12);
    store<u32>(o, 0x00B13E58);
    if (p != 0) VCall(ctx, p, 40, {p, 0});
    store<u32>(o + 12, 0);
    call(ctx, 0x009F6A60, {o + 16});
    store<u32>(o, 0x00B0DAEC);
    call(ctx, o19fn::SmallFree, {o, 272});
}

// The effect-holder deleting destructor shape: when +4 = e is set, vtable
// vt, bit 0x200 set and 0x400 cleared in e->+136, Effect_Delete(+4), +4 =
// 0; then vtable 0x00B145E0 and Mem_FreeFrom(o, 0). Null does nothing.
static void EffectHolderDelete27(PPUContext& ctx, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u32>(o + 4) != 0) {
        const u32 e = load<u32>(o + 4);
        store<u32>(o, vt);
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        const u32 e2 = load<u32>(o + 4);
        store<u32>(e2 + 136, load<u32>(e2 + 136) & ~0x400u);
        call(ctx, 0x004D0280, {load<u32>(o + 4)});
        store<u32>(o + 4, 0);
    }
    store<u32>(o, 0x00B145E0);
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}
// T::~T deleting form: EffectHolderDelete27 with the vtable 0x00B146D0
DECOMP(0016D928, VtB146D0_Delete) { EffectHolderDelete27(ctx, 0x00B146D0); }
// T::~T deleting form: EffectHolderDelete27 with the vtable 0x00B14700
DECOMP(0016E850, VtB14700_Delete) { EffectHolderDelete27(ctx, 0x00B14700); }

// T::~T deleting form: vtables 0x00B14968 / 0x00B14B00 (+380) / 0x00B14B28
// (+384), List1013B48_Remove(o), the base vtables 0x00B12528 (+384) /
// 0x00B1200C (+380) / 0x00B126B8, VtB12550_Dtor_000DF934(o), Mem_FreeFrom(o,
// 0). Null does nothing (r3 = 0).
DECOMP(001780E4, VtB14968_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B14968);
    store<u32>(o + 380, 0x00B14B00);
    store<u32>(o + 384, 0x00B14B28);
    call(ctx, 0x000D68CC, {o});
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

// void f(T* o, u32 v): when the world's +0x1281D4 = w has +4 = x set and
// o->+388 = a is set: a->+44 = (v << 16) | its low 5 bits and, with a body
// a->+8, 0x00930B70(x, a, 0, 1); then +412 = v and 0x000DA138(o, v)
DECOMP(001975DC, Obj_SetId_001975DC) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 x = load<u32>(load<u32>(load<u32>(0x00F65CA0) + 0x1281D4) + 4);
    if (x != 0) {
        const u32 a = load<u32>(o + 388);
        if (a != 0) {
            const u32 w = (load<u32>(a + 44) & 0x1Fu) | (v << 16);
            const u32 b = load<u32>(a + 8);
            store<u32>(a + 44, w);
            if (b != 0) call(ctx, 0x00930B70, {x, a, 0, 1});
        }
    }
    store<u32>(o + 412, v);
    call(ctx, 0x000DA138, {o, v});
}

// T::~T deleting form: vtable 0x00B16058, AnimSkel_Release(+228) when set,
// ~PtrVector(+192), vtable 0x00B11D98, the base destructor 0x000C2DA0,
// SmallFree(o, 256). Null does nothing (r3 = 0).
DECOMP(001B3B34, VtB16058_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 s = load<u32>(o + 228);
    store<u32>(o, 0x00B16058);
    if (s != 0) call(ctx, o26fn::AnimSkel_Release, {s});
    VecDtor(ctx, o + 192);
    store<u32>(o, 0x00B11D98);
    call(ctx, o26fn::Obj_BaseDtor_0C2DA0, {o});
    call(ctx, o19fn::SmallFree, {o, 256});
}

// bool f(T* o, f32 a, f32 b): 1 when +552 has bit 0x20000000; when b is
// below the constant at 0x001C1BF4 or a is at least b, o->vfn[420](o, 3, 0)
// (vtable read first), bit 0x80 of the u8 +576 set and 0; else 1
DECOMP(001C1BF8, Obj_TryState3_001C1BF8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 vt = load<u32>(o);
    if ((load<u32>(o + 552) & 0x20000000u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    if (b < static_cast<f64>(load<f32>(0x001C1BF4)) || a >= b) {
        VCallVt(ctx, vt, 420, {o, 3, 0});
        store<u8>(o + 576, static_cast<u8>(load<u8>(o + 576) | 0x80u));
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = 1;
}

// The 0x00B17270 deleting destructor body for an adjusted this (this =
// o + delta): vtable 0x00B17270, the u16 +820 = 0, 0x00B17444 (+380) /
// 0x00B1746C (+384), ~PtrVector(+812), the base destructor 0x001AE468,
// SmallFree(o, 864). A null o does nothing (r3 = this).
static void VtB17270DeleteAdj(PPUContext& ctx, u32 delta) {
    const u32 o = arg<u32>(ctx, 0) - delta;
    if (o == 0) return;
    store<u32>(o, 0x00B17270);
    store<u16>(o + 820, 0);
    store<u32>(o + 380, 0x00B17444);
    store<u32>(o + 384, 0x00B1746C);
    VecDtor(ctx, o + 812);
    call(ctx, 0x001AE468, {o});
    call(ctx, o19fn::SmallFree, {o, 864});
}
// T::~T deleting form for this = o + 384 (VtB17270DeleteAdj)
DECOMP(001CBF2C, VtB17270_DeleteAdj384) { VtB17270DeleteAdj(ctx, 384); }
// T::~T deleting form for this = o + 380 (VtB17270DeleteAdj)
DECOMP(001CBFCC, VtB17270_DeleteAdj380) { VtB17270DeleteAdj(ctx, 380); }

// void f(T* o, U* x): the u16 x->+178 += 1, then x pushed onto the
// PtrVector o + 812 (+816 u16 capacity, doubled when full; +820 count, +822
// the high-water mark). r3 = the old mark, r4 = the new count.
DECOMP(001CC174, Obj_AddChild_001CC174) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    store<u16>(x + 178, static_cast<u16>(load<u16>(x + 178) + 1));
    const u32 cap = load<u16>(o + 816);
    if (load<u16>(o + 820) == cap) call(ctx, o27fn::PtrVector_Reserve, {o + 812, (cap << 1) & 0x1FFFEu});
    store<u32>(load<u32>(o + 812) + ((static_cast<u32>(load<u16>(o + 820)) << 2) & 0x3FFFCu), x);
    const u32 n = load<u16>(o + 820) + 1;
    const u32 m = load<u16>(o + 822);
    store<u16>(o + 820, static_cast<u16>(n));
    store<u16>(o + 822, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFF) ? m : n));
    ctx.gpr[3] = m;
    ctx.gpr[4] = n & 0xFFFF;
}

// bool f(T* o, Area* p): false unless Area_Enter(o, p); m = 0 and
// 0x0021612C(o, p, &m); when m has a bit of the u16 +428 and the world's
// u16 +0x128214 is 0, 0x00215D98(o, p); true
DECOMP(0021686C, Area_EnterAndTrigger_0021686C) {
    Stack frame(ctx, 144);
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    if (static_cast<s32>(call(ctx, 0x0020E7A4, {o, p})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 m = frame.addr() + 112;
    store<u32>(m, 0);
    call(ctx, 0x0021612C, {o, p, m});
    if ((load<u32>(m) & load<u16>(o + 428)) != 0 && load<u16>(load<u32>(0x00F65CA0) + 0x128214) == 0) call(ctx, 0x00215D98, {o, p});
    ctx.gpr[3] = 1;
}

// void f(T* o, u32 k): +368 = 0 and bits 23..30 of +384 = k; for k < 6, +368
// = Save_Profile0C9FC8(the hash 0x00AD4828[k]) (a nonzero hash that fails
// warns with the format 0x00AD48DC and k)
DECOMP(00254040, Obj_SetProfile_00254040) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    const u32 w = (load<u32>(o + 384) & ~0x7F800000u) | ((k << 23) & 0x7F800000u);
    store<u32>(o + 368, 0);
    store<u32>(o + 384, w);
    if (k >= 6) return;  // r3 = o
    const u32 h = load<u32>(0x00AD4828 + 4 * k);
    const u32 r = static_cast<u32>(call(ctx, 0x000C9FC8, {h}));
    if (h != 0 && r == 0) call(ctx, o27fn::String_Format, {0x00AD48DC, k});
    store<u32>(o + 368, r);
}

// u8 f(T* o, u32 i): the byte i of 0x00AD4E35 when the target +8 has bit 8
// of +192 and its o->vfn[64] result has kind 2 in bits 28..30 of +416;
// else the byte i of 0x00AD4E2D
DECOMP(00276978, Obj_GetModeByte_00276978) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 t = load<u32>(o + 8);
    u32 tab = 0x00AD4E2D;
    if (t != 0 && (load<u32>(t + 192) & 8u) != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, t, 64, {t}));
        if (r != 0 && ((load<u32>(r + 416) >> 28) & 7u) == 2) tab = 0x00AD4E35;
    }
    ctx.gpr[3] = load<u8>(i + tab);
}

// bool f(Iff* f): clears 0x0106BFAC / 0x0106BFB0, Iff_ReadChunks(f, 0);
// when that set a path at 0x0106BFAC and File_Open(path, 0x00ADBE84)
// succeeds, 0x0034B2C8(file) and file->vfn[8](file). Returns 1.
DECOMP(0034B4F4, Iff_LoadAndRead_0034B4F4) {
    const u32 f = arg<u32>(ctx, 0);
    store<u32>(0x0106BFAC, 0);
    store<u32>(0x0106BFB0, 0);
    call(ctx, 0x00569388, {f, 0, 0, 0x01070000});
    const u32 path = load<u32>(0x0106BFAC);
    if (path != 0) {
        const u32 h = static_cast<u32>(call(ctx, 0x0055BD3C, {path, 0x00ADBE84}));
        if (h != 0) {
            call(ctx, 0x0034B2C8, {h});
            VCall(ctx, h, 8, {h});
        }
    }
    ctx.gpr[3] = 1;
}

// void f(T* o, u8 on): o->+24->vfn[40](it, on, 1) when set; when on, with
// the effect +20, Effect_SetHash(+20, +180 + 0x02B81827)
DECOMP(0036EAD4, Obj_SetActive_0036EAD4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu;
    const u32 p = load<u32>(o + 24);
    ctx.gpr[3] = p;
    if (p != 0) VCall(ctx, p, 40, {p, a, 1});
    if (a == 0) return;
    const u32 i = load<u32>(o + 180);
    const u32 e = load<u32>(o + 20);
    ctx.gpr[3] = i;
    if (e == 0) return;
    call(ctx, o27fn::Effect_SetHash, {e, i + 0x02B81827u});
}

// T::T(x, a, b, c, d): SubMachine_Construct(o, x, b, c, d), the member
// 0x0048F27C(o + 56), +100 = a, vtables 0x00B205B8 / 0x00B2061C (+56), +64
// / +68 = 0, bit 8 set in +28 (r3..r6 as left)
DECOMP(003D8B34, VtB205B8_Ctor) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    call(ctx, 0x0047C5D8, {o, x, arg<u32>(ctx, 3), arg<u32>(ctx, 4), arg<u32>(ctx, 5)});
    call(ctx, 0x0048F27C, {o + 56});
    store<u32>(o + 100, a);
    store<u32>(o, 0x00B205B8);
    const u32 w = load<u32>(o + 28) | 8u;
    store<u32>(o + 56, 0x00B2061C);
    store<u32>(o + 64, 0);
    store<u32>(o + 68, 0);
    store<u32>(o + 28, w);
    ctx.gpr[3] = o + 28;
    ctx.gpr[4] = w;
    ctx.gpr[5] = 0x00B2061C;
    ctx.gpr[6] = 0;
}

// T::T(a, b, c, f32 x, f32 y, ..., u32 d (r9)): base 0x0042C9D0(o, 1),
// +12..+20 = a..c, +24 / +28 = x / y, vtable 0x00B20D88, +32 = d (r3 = the
// vtable)
DECOMP(0042DAB4, VtB20D88_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 6);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1);
    CmdBase27(ctx, o, 1);
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    store<f32>(o + 24, static_cast<f32>(x));
    store<f32>(o + 28, static_cast<f32>(y));
    store<u32>(o, 0x00B20D88);
    store<u32>(o + 32, d);
    ctx.gpr[3] = 0x00B20D88;
}

// T::T(u8 a, f32 x, f32 y, f32 z, f32 w): base 0x0042C9D0(o, 1), the u8 +9
// = a, +12 / +16 / +20 = x / y / z, +24 = 1.0f, +28 = w, +32 = 0, vtable
// 0x00B20DD0 (r3 / r4 / r5 as left)
DECOMP(0042E040, VtB20DD0_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1), z = farg(ctx, 2), w = farg(ctx, 3);
    CmdBase27(ctx, o, 1);
    store<u8>(o + 9, static_cast<u8>(a));
    store<f32>(o + 12, static_cast<f32>(x));
    store<f32>(o + 16, static_cast<f32>(y));
    store<u32>(o, 0x00B20DD0);
    store<f32>(o + 20, static_cast<f32>(z));
    store<u32>(o + 24, 0x3F800000);
    store<f32>(o + 28, static_cast<f32>(w));
    store<u32>(o + 32, 0);
    ctx.gpr[3] = 0x00B20DD0;
    ctx.gpr[4] = 0x3F800000;
    ctx.gpr[5] = 0;
}

// T::T(u32 a, u8 b..f, bool on): base 0x0042C9D0(o, on), +12 = a, the u8s
// +16..+20 = b..f, vtable 0x00B20E18 (r3 = the vtable)
DECOMP(0042E618, VtB20E18_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4), e = arg<u32>(ctx, 5), f = arg<u32>(ctx, 6);
    CmdBase27(ctx, o, arg<u32>(ctx, 7));
    store<u32>(o + 12, a);
    store<u8>(o + 16, static_cast<u8>(b));
    store<u8>(o + 17, static_cast<u8>(c));
    store<u8>(o + 18, static_cast<u8>(d));
    store<u8>(o + 19, static_cast<u8>(e));
    store<u32>(o, 0x00B20E18);
    store<u8>(o + 20, static_cast<u8>(f));
    ctx.gpr[3] = 0x00B20E18;
}

// void f(World* g, u32 i, x, u8 kind): with player i's object (bit 8 of its
// +192 set), Obj_DispatchKind_0012BB9C(p, x) when kind is set, else
// 0x0012B908(p, x) (r3 = x otherwise)
DECOMP(00488078, Player_Dispatch_00488078) {
    const u32 g = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), k = arg<u32>(ctx, 3) & 0xFFu;
    const u32 p = load<u32>(g + i * 0x20C80u + 4208 + 8);
    ctx.gpr[3] = x;
    if (p == 0 || (load<u32>(p + 192) & 8u) == 0) return;
    call(ctx, k != 0 ? 0x0012BB9C : 0x0012B908, {p, x});
}

// void f(T* o): the u8 +47 = o->+8->+0; for a state +50 in 1..11, +49 = 6,
// else StateMachine_SetState(o, 1, 6); then the u8 +1 of the state record
// (+20 + the u16 +6) = 1 and, with a player, Obj_ShowPanel(player, 0)
DECOMP(0048970C, State_Enter6_0048970C) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = load<u8>(o + 50);
    store<u8>(o + 47, load<u8>(load<u32>(o + 8)));
    if (s < 12 && s != 0) store<u8>(o + 49, 6);
    else call(ctx, 0x00464D1C, {o, 1, 6});
    store<u8>(load<u32>(o + 20) + load<u16>(o + 6) + 1, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x00365C24, {1}));
    if (p == 0) return;
    const u32 q = static_cast<u32>(call(ctx, 0x00365C24, {p}));
    call(ctx, 0x0037370C, {q, 0});
}

// Node* f(Node* n, s32 key): the first node in the chain n (next at +268)
// of kind 13 (bits 10..15 of +4) with +204 equal to key, searching each
// node's children (+272) recursively before its siblings; 0 for none
DECOMP(004A21F8, Node_FindKind13_004A21F8) {
    const u32 n = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    if (n == 0 || key == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    for (u32 c = n; c != 0; c = load<u32>(c + 268)) {
        if (((load<u32>(c + 4) >> 10) & 0x3Fu) == 13 && load<u32>(c + 204) == key) {
            ctx.gpr[3] = c;
            return;
        }
        const u32 r = static_cast<u32>(call(ctx, 0x004A21F8, {load<u32>(c + 272), key}));
        if (r != 0) {
            ctx.gpr[3] = r;
            return;
        }
    }
    ctx.gpr[3] = 0;
}

// Material* f(WorldObj* o, u32 k): WorldObj_Material4A59B4(o, k), else the
// first match among o's kind-8 children (+272, chained by +268; bits
// 10..15 of +4), searched recursively; 0 for none
DECOMP(004A5A48, WorldObj_FindMaterial_004A5A48) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    u32 r = static_cast<u32>(call(ctx, 0x004A59B4, {o, k}));
    if (r == 0) {
        for (u32 c = load<u32>(o + 272); c != 0; c = load<u32>(c + 268)) {
            if (((load<u32>(c + 4) >> 10) & 0x3Fu) != 8) continue;
            r = static_cast<u32>(call(ctx, 0x004A5A48, {c, k}));
            if (r != 0) break;
        }
    }
    ctx.gpr[3] = r;
}

// void f(WorldObj* o): for each of the u8 +112 sections of the model
// o->+208 (+132, 8-byte stride) and each of its u16 +52 parts (+48, 8-byte
// entries, +4 the object), bits 15..30 of object->+112 = 519
DECOMP(004A5B80, Model_SetPartsMode_004A5B80) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 m = load<u32>(o + 208);
    ctx.gpr[3] = m;
    if (m == 0) return;
    for (u32 i = 0; i < load<u8>(m + 112); ++i) {
        const u32 sp = m + 132 + 8 * i;
        for (u32 j = 0; j < load<u16>(load<u32>(sp) + 52); ++j) {
            const u32 e = load<u32>(load<u32>(load<u32>(sp) + 48) + 8 * j + 4);
            if (e == 0) continue;
            store<u32>(e + 112, (load<u32>(e + 112) & ~0x7FFF8000u) | (519u << 15));
        }
    }
}

// void f(T* o, Vec3* out, f32 t): out = the player's position (+144, when
// bit 2 of its +192 is set) or the default 0x01164B40; when t is above the
// constant at 0x007353C0, out += o->+528..+536 * t (f1 = the last value)
DECOMP(007353C8, Player_PredictPos_007353C8) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const f64 t = farg(ctx, 0);
    const u32 p = load<u32>(load<u32>(0x00F65CA0) + 4208 + 8);
    const u32 src = (p != 0 && (load<u32>(p + 192) & 2u) != 0) ? p + 144 : 0x01164B40u;
    u32 w[3];
    for (u32 k = 0; k < 3; ++k) {
        w[k] = load<u32>(src + 4 * k);
        store<u32>(out + 4 * k, w[k]);
    }
    if (!(t > static_cast<f64>(load<f32>(0x007353C0)))) return;
    auto mad = [&](u32 k) {
        return static_cast<f32>(std::fma(static_cast<f64>(load<f32>(o + 528 + 4 * k)), t, static_cast<f64>(std::bit_cast<f32>(w[k]))));
    };
    // the three sums are computed before any is stored, as in the original
    const f32 a = mad(0), b = mad(1), r = mad(2);
    store<f32>(out, a);
    store<f32>(out + 4, b);
    store<f32>(out + 8, r);
    ctx.fpr[1] = static_cast<f64>(r);
}

// void f(T* d, const T* s): copies +0..+8, +16, the u8 +20 (also into the
// top byte of the +20 word), bits 0x00FE0000 of +20 one at a time (each
// stored), +24, +28, +32 (r4..r6 / f1 as left)
DECOMP(0074C814, Rec_Copy_0074C814) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 12; k += 4) store<u32>(d + k, load<u32>(s + k));
    const u32 f16 = load<u32>(s + 16);
    u32 w = load<u32>(d + 20);
    store<u32>(d + 16, f16);
    const u32 b = load<u8>(s + 20);
    store<u8>(d + 20, static_cast<u8>(b));
    w = (w & 0x00FFFFFFu) | (b << 24);
    u32 v = 0;
    for (u32 m = 0x00800000u; m >= 0x00020000u; m >>= 1) {
        v = load<u32>(s + 20);
        w = (w & ~m) | (v & m);
        store<u32>(d + 20, w);
    }
    const u32 w24 = load<u32>(s + 24), f28 = load<u32>(s + 28);
    store<u32>(d + 24, w24);
    const u32 w32 = load<u32>(s + 32);
    store<u32>(d + 28, f28);
    store<u32>(d + 32, w32);
    ctx.gpr[4] = w32;
    ctx.gpr[5] = w24;
    ctx.gpr[6] = v;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f28));
}

// The 0x00B0DAAC-based destructor shape: vtable vt, ~PtrVector(+40),
// ~PtrVector(+20), vtable 0x00B0DAAC, SortedMap_Remove(0x01056F04, o + 4).
static void VtB0DAACDtorBody(PPUContext& ctx, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    VecDtor(ctx, o + 40);
    VecDtor(ctx, o + 20);
    store<u32>(o, 0x00B0DAAC);
    call(ctx, 0x007330C0, {0x01056F04, o + 4});
}
// T::~T: VtB0DAACDtorBody with the vtable 0x00B1C8C8
DECOMP(00764644, VtB1C8C8_Dtor) { VtB0DAACDtorBody(ctx, 0x00B1C8C8); }
// T::~T: VtB0DAACDtorBody with the vtable 0x00B1CE68
DECOMP(0076539C, VtB1CE68_Dtor) { VtB0DAACDtorBody(ctx, 0x00B1CE68); }

// T::~T deleting form: vtables 0x00B1F298 / 0x00B1F314 (+20) / 0x00B1F2D0
// (+8) / 0x00B1F2E4 (+12) / 0x00B1F300 (+16), the base 0x00884FB8(o), then
// the r13 pool (-28332)->+44->vfn[12](pool, o, the u16 +4). Null does
// nothing (r3 = 0).
DECOMP(007709D8, VtB1F298_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o, 0x00B1F298);
    store<u32>(o + 20, 0x00B1F314);
    store<u32>(o + 8, 0x00B1F2D0);
    store<u32>(o + 12, 0x00B1F2E4);
    store<u32>(o + 16, 0x00B1F300);
    call(ctx, 0x00884FB8, {o});
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}
