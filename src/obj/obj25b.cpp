// Game code, part 25 (continued): the sweep of small still-recompiled game
// functions from 31 instructions up. See obj25.cpp for the conventions.
#include "obj25.h"

using namespace sly4;
using namespace sly4::obj25;

// ---------------------------------------------------------------- shared bodies
// p = SmallAlloc(size), make(p) when allocated, then 0x0042C138(p or null)
// (the script-command list push, name?).
template <typename F>
static void PushNewB(PPUContext& ctx, u32 size, F make) {
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {size}));
    u32 r = 0;
    if (p != 0) {
        make(p);
        r = p;
    }
    call(ctx, 0x0042C138, {r});
}
// The body of VtB174C0_Dtor (0x001D1D94) for an adjusted this: vtables
// 0x00B174C0 / 0x00B17694 (+380) / 0x00B176BC (+384), the PtrVector at +556
// destroyed, then the base destructor 0x001AE468.
static void VtB174C0DtorAt(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B174C0);
    store<u32>(o + 380, 0x00B17694);
    store<u32>(o + 384, 0x00B176BC);
    VecDtor(ctx, o + 556);
    call(ctx, 0x001AE468, {o});
}

// ---------------------------------------------------------------- sweep: 31 instructions
// T::~T: when o->+48->+220 and its +80 are set, vtable 0x00B0E428 and
// 0x0002B618(+80, it); then vtable 0x00B0DC70 and NavQuery_Free62D08C(o +
// 216)
DECOMP(0002BA28, VtB0E428_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(load<u32>(o + 48) + 220);
    if (q != 0) {
        const u32 p = load<u32>(q + 80);
        if (p != 0) {
            store<u32>(o, 0x00B0E428);
            call(ctx, 0x0002B618, {p, q});
        }
    }
    store<u32>(o, 0x00B0DC70);
    call(ctx, 0x0062D08C, {o + 216});
}
// void f(T* o): o->vfn[220](21) for the kind +4 19, o->vfn[220](15) for 14
DECOMP(000577C0, Obj_KindStep_000577C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 4);
    ctx.gpr[4] = k;
    if (k == 19) VCall(ctx, o, 220, {o, 21});
    else if (k == 14) VCall(ctx, o, 220, {o, 15});
}
// void f(T* o): pushes a new VtB21358 (20 bytes) made from ((s32)+12; f32
// +16)
DECOMP(000A6220, Cmd_PushB21358_000A6220) {
    const u32 o = arg<u32>(ctx, 0);
    PushNewB(ctx, 20, [&](u32 p) { callf(ctx, 0x00439D48, {p, Sx(load<u32>(o + 12))}, {LfsAt(o + 16)}); });
}
// T* f(T* o, s32 i): the 16-byte entry i of o->+160->vfn[28]()->+32; null
// when +160 or the result is null or i < 0
DECOMP(000DA954, Obj_Entry16_000DA954) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 i = arg<s32>(ctx, 1);
    const u32 p = load<u32>(o + 160);
    if (p != 0) {
        const u32 q = static_cast<u32>(VCall(ctx, p, 28, {p}));
        if (q != 0 && i >= 0) {
            ret(ctx, load<u32>(q + 32) + 16 * static_cast<u32>(i));
            return;
        }
    }
    ret(ctx, 0u);
}
// void f(T* o, s32 k): the u8 +624 = (k == 1 and the u8 o->+216->+436 is
// set); +628 = k
DECOMP(000F498C, Obj_SetMode_000F498C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 216);
    ctx.gpr[6] = p;
    const u32 b = static_cast<s32>(k) == 1 ? (load<u8>(p + 436) != 0 ? 1u : 0u) : 0u;
    store<u8>(o + 624, static_cast<u8>(b));
    store<u32>(o + 628, k);
    ctx.gpr[5] = b;
}
// void f(const T* o, U* d): d->+0..+56 = the 4x3 part of the matrix at o +
// 96, d->+64 / +68 = o->+356 / +360 (raw; f1 / r5 left)
DECOMP(000FA5D0, Obj_GetXform_000FA5D0) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    const u32 m = o + 96;
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) Cp(d + 16 * r + 4 * c, m + 16 * r + 4 * c);
    Cp(d + 64, o + 356);
    ctx.fpr[1] = F32Bits(Cp(d + 68, o + 360));
    ctx.gpr[5] = m;
}
// void f(T* o): when +12 and +16 are set, +32..+40 = +16->+160..+168 minus
// +12->+128..+136; then +32 = +36 = 0 (only z is kept)
DECOMP(00113458, Obj_DeltaZ_00113458) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    ctx.gpr[5] = a;
    if (a == 0) return;
    const u32 b = load<u32>(o + 16);
    ctx.gpr[4] = b;
    if (b == 0) return;
    const f64 bx = LfsAt(b + 160), ax = LfsAt(a + 128), by = LfsAt(b + 164), ay = LfsAt(a + 132);
    const f64 bz = LfsAt(b + 168), az = LfsAt(a + 136);
    const f64 dx = static_cast<f32>(bx - ax), dy = static_cast<f32>(by - ay), dz = static_cast<f32>(bz - az);
    store<f32>(o + 32, static_cast<f32>(dx));
    store<f32>(o + 36, static_cast<f32>(dy));
    store<f32>(o + 40, static_cast<f32>(dz));
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0);
    ctx.fpr[1] = dx;
    ctx.fpr[2] = dz;
    ctx.fpr[3] = dy;
    ctx.fpr[4] = ay;
    ctx.fpr[5] = bz;
    ctx.gpr[4] = o + 32;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = o + 36;
    ctx.gpr[7] = o + 36;
}
// void f(T* o, U* d): 0x000DFBFC(o, d), then d->+208..+216 = o->+240..+248,
// d->+224..+232 = o->+256..+264, d->+240 / +200 = o->+272 / +276
DECOMP(00168114, Obj_CopyOut_00168114) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {o, d});
    for (u32 k = 0; k < 3; ++k) Cp(d + 208 + 4 * k, o + 240 + 4 * k);
    Cp(d + 224, o + 256);
    Cp(d + 228, o + 260);
    ctx.fpr[1] = F32Bits(Cp(d + 232, o + 264));
    ctx.gpr[3] = Cp(d + 240, o + 272);
    ctx.gpr[4] = Cp(d + 200, o + 276);
}
// T::~T: vtable 0x00B15150, RefNode_Release on the five nodes at +172..+188,
// then the base destructor 0x000C2DA0
DECOMP(001A7058, VtB15150_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B15150);
    for (u32 i = 0; i < 5; ++i) call(ctx, o24fn::RefNode_Release, {load<u32>(o + 172 + 4 * i)});
    call(ctx, 0x000C2DA0, {o});
}
// u32 f(a, b, c, f32* d): 0 when c is set; else *d *= the f32 at
// 0x001CE8A8 and 0x000DF960(a, b, 0, d)
DECOMP(001CE8B0, Obj_ScaleAndCall_001CE8B0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    if (c != 0) {
        ctx.gpr[4] = c;
        ctx.gpr[5] = b;
        ctx.gpr[6] = a;
        ctx.gpr[7] = c;
        ctx.gpr[8] = d;
        ret(ctx, 0u);
        return;
    }
    const f32 v = static_cast<f32>(LfsAt(d) * LfsAt(0x001CE8A8));
    store<f32>(d, v);
    ctx.fpr[1] = v;
    ctx.fpr[2] = LfsAt(0x001CE8A8);
    call(ctx, 0x000DF960, {a, b, c, d, c, d});
}
// VtB174C0_Dtor called through the interface 384 bytes in
DECOMP(001D1FD8, VtB174C0_DtorAdj384) { VtB174C0DtorAt(ctx, arg<u32>(ctx, 0) - 384); }
// VtB174C0_Dtor called through the interface 380 bytes in
DECOMP(001D2054, VtB174C0_DtorAdj380) { VtB174C0DtorAt(ctx, arg<u32>(ctx, 0) - 380); }
// void f(T* o): +584 = 0, o->vfn[28](0, 1), Obj_SetStateFx266E6C(
// Target_GetDef0D951C(o), 0), +608 = 0
DECOMP(001DB9C8, Obj_Reset_001DB9C8) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 584, 0);
    VCall(ctx, o, 28, {o, 0, 1});
    const u32 t = static_cast<u32>(call(ctx, 0x000D951C, {o}));
    call(ctx, 0x00266E6C, {t, 0});
    store<u32>(o + 608, 0);
}
// void f(T* o; f32 x): the u16 +510 of o->vfn[144]() (when it returns an
// object) = the upper half of x's bits; then 0x000DA25C(o; x)
DECOMP(001FB804, Obj_SetHalf_001FB804) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    const u32 r = static_cast<u32>(VCall(ctx, o, 144, {o}));
    if (r != 0) store<u16>(r + 510, static_cast<u16>(std::bit_cast<u32>(static_cast<f32>(x)) >> 16));
    callf(ctx, 0x000DA25C, {o}, {x});
}
// T::~T deleting form: vtable 0x00B19920, RefNode_Release(+184), +184 = 0,
// vtable 0x00B19510, the base destructor 0x000C2DA0, SmallFree(o, 192).
// Null does nothing.
DECOMP(00210238, VtB19920_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B19920);
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 184)});
    store<u32>(o + 184, 0);
    store<u32>(o, 0x00B19510);
    call(ctx, 0x000C2DA0, {o});
    call(ctx, 0x00552420, {o, 192});
}
// void f(T* o, bool b): when b and +436 is set, +436->vfn[4]() and +436 =
// 0; then 0x0020D5D4(o, b)
DECOMP(00224B48, Obj_DropChild_00224B48) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    if (b != 0) {
        const u32 p = load<u32>(o + 436);
        if (p != 0) {
            VCall(ctx, p, 4, {p});
            store<u32>(o + 436, 0);
        }
    }
    call(ctx, 0x0020D5D4, {o, b});
}
// void f(T* o, Event* e): the event 0x2FBD3D56 -> 0x002B0480(o, e->+0,
// e->+12); 0x340866A4 -> +76 = 0 (r3 left as the compared constant
// otherwise)
DECOMP(002B05D8, Obj_OnEvent_002B05D8) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const s32 h = load<s32>(e + 8);
    ctx.gpr[5] = static_cast<u32>(h);
    if (h > 0x2FBD3D56) {
        ctx.gpr[6] = 0x2FBD3D56;
        ctx.gpr[4] = o;
        if (h != 0x340866A4) {
            ret(ctx, 0x340866A4u);
            return;
        }
        store<u32>(o + 76, 0);
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[6] = 0x2FBD3D55;
    if (h > 0x2FBD3D55) {
        call(ctx, 0x002B0480, {o, load<u32>(e), load<u32>(e + 12)});
        return;
    }
    ctx.gpr[3] = e;
    ctx.gpr[4] = o;
}
// f32 f(): Rec_SumCounts_000B29B0() = n; 0x000B2A20() / n when n > 0, else
// the f32 at 0x002E3A6C (f2 left as n)
DECOMP(002E3A70, Rec_Ratio_002E3A70) {
    const s32 n = static_cast<s32>(call(ctx, 0x000B29B0));
    if (n <= 0) {
        ctx.gpr[3] = 0x002E3A6C;
        ctx.fpr[1] = LfsAt(0x002E3A6C);
        return;
    }
    const s32 m = static_cast<s32>(call(ctx, 0x000B2A20));
    const f64 a = static_cast<f32>(static_cast<f64>(static_cast<s64>(m)));
    const f64 b = static_cast<f32>(static_cast<f64>(static_cast<s64>(n)));
    ctx.fpr[2] = b;
    ctx.gpr[4] = Sx(static_cast<u32>(n));
    ctx.fpr[1] = static_cast<f32>(a / b);
    ctx.gpr[3] = Sx(static_cast<u32>(m));
}
// T::~T: when +44 is set, vtable 0x00B1D628 and the effect +44 destroyed
// (Effect_Destruct, SmallFree 140), +44 = 0; then vtable 0x00B1D600 (r3
// left as it)
DECOMP(002F99A0, VtB1D628_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 44);
    if (p != 0) {
        store<u32>(o, 0x00B1D628);
        call(ctx, 0x004C8E80, {p});
        call(ctx, 0x00552420, {p, 140});
        store<u32>(o + 44, 0);
    }
    store<u32>(o, 0x00B1D600);
    ret(ctx, 0x00B1D600u);
}
// void f(): the static destructor of the object at 0x0106B264: when the u64
// 0x0106B2C0 is set, the PtrVector at 0x0106B2C8 destroyed and it cleared;
// then vtable 0x00B0D99C and the base destructor 0x004FC1A0
DECOMP(003285C8, Static_Dtor_003285C8) {
    if (load<u64>(0x0106B2C0) != 0) {
        VecDtor(ctx, 0x0106B2C8);
        store<u64>(0x0106B2C0, 0);
    }
    store<u32>(0x0106B264, 0x00B0D99C);
    call(ctx, 0x004FC1A0, {0x0106B264});
}
// T::~T deleting form: vtable 0x00B1E3E0, the PtrVector at +3248
// destroyed, the base destructor 0x003286D8, SmallFree(o, 3280). Null does
// nothing.
DECOMP(00329240, VtB1E3E0_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1E3E0);
    VecDtor(ctx, o + 3248);
    call(ctx, 0x003286D8, {o});
    call(ctx, 0x00552420, {o, 3280});
}
// void f(s32 key): 0x00369680(e) for the first object e of the list at
// 0x0106C280 (+0 the array, the u16 +8 the count) whose +168 is key
DECOMP(00369C30, List_RunByKey_00369C30) {
    const u32 key = arg<u32>(ctx, 0);
    const u32 a = load<u32>(0x0106C280);
    const u32 n = load<u16>(0x0106C280 + 8);
    ctx.gpr[4] = a;
    ctx.gpr[5] = a;
    ctx.gpr[6] = n;
    ctx.gpr[7] = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 k = load<u32>(load<u32>(a + 4 * i) + 168);
        ctx.gpr[8] = k;
        if (k == key) {
            call(ctx, 0x00369680, {load<u32>(a + 4 * i)});
            return;
        }
        ctx.gpr[7] = i + 1;
        ctx.gpr[4] = a + 4 * (i + 1);
    }
}
// Obj_OnEvent_003AA9F0 called through the interface 64 bytes in
DECOMP(003AAA64, Obj_OnEventAdj64_003AAA64) { ObjOnEventA7DC(ctx, arg<u32>(ctx, 0) - 64); }
// void f(T* o): +12 = Obj_FindByHandle42C2B0(+16, 0); when found, +20 =
// the one for +24 and, when found too, Obj_Detach0D72C0(+12, +20); then
// Flag_Set8_40000000(o)
DECOMP(0042DE54, Obj_Detach_0042DE54) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(o + 16), 0}));
    store<u32>(o + 12, a);
    if (a != 0) {
        const u32 b = static_cast<u32>(call(ctx, 0x0042C2B0, {load<u32>(o + 24), 0}));
        store<u32>(o + 20, b);
        if (b != 0) call(ctx, 0x000D72C0, {load<u32>(o + 12), b});
    }
    call(ctx, o24fn::Flag_Set8_40000000, {o});
}
// T::~T deleting form: vtable 0x00B20E90, Audio_Release446384(the audio
// table 0x010ACC40, +28), RefNode_Release(+12), VtB20CC8_DtorTrivial,
// SmallFree(o, 32). Null does nothing.
DECOMP(0042EEF0, VtB20E90_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 h = load<u32>(o + 28);
    store<u32>(o, 0x00B20E90);
    call(ctx, 0x00446384, {0x010ACC40, h});
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 12)});
    call(ctx, 0x0042CA50, {o});
    call(ctx, 0x00552420, {o, 32});
}
// T::~T deleting form: vtable 0x00B21058, RefNode_Release(+12),
// Audio_Release446384(0x010ACC40, +16), VtB20CC8_DtorTrivial, SmallFree(o,
// 220). Null does nothing.
DECOMP(00432A2C, VtB21058_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B21058);
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 12)});
    call(ctx, 0x00446384, {0x010ACC40, load<u32>(o + 16)});
    call(ctx, 0x0042CA50, {o});
    call(ctx, 0x00552420, {o, 220});
}
// T::~T deleting form: vtable 0x00B21100, RefNode_Release(+12),
// Audio_Release446384(0x010ACC40, +16), VtB20CC8_DtorTrivial, SmallFree(o,
// 124). Null does nothing.
DECOMP(004345F8, VtB21100_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B21100);
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 12)});
    call(ctx, 0x00446384, {0x010ACC40, load<u32>(o + 16)});
    call(ctx, 0x0042CA50, {o});
    call(ctx, 0x00552420, {o, 124});
}
// void f(): Draw_Clear(1, 0xFFFFFFFF, ..., 0, 255; the f32 at 0x0048C898),
// Layer9_RunStream_005165D0(), then (*0x010DDAC0)->vfn[16]()
DECOMP(0048C8A0, Frame_Clear_0048C8A0) {
    callf(ctx, 0x00531F80, {1, 0xFFFFFFFFu, 0x0048C898, 0, 255}, {LfsAt(0x0048C898)});
    call(ctx, 0x005165D0);
    const u32 p = load<u32>(0x010DDAC0);
    VCall(ctx, p, 16, {p});
}
// bool f(): whether any of the n (*0x010C1A9C) 152-byte slots at 0x010BE8BC
// with +0 set has bit 0x4 in the world's u8 +4028 + i
DECOMP(0049E5AC, Slots_AnyFlag4_0049E5AC) {
    const u32 n = load<u32>(0x010C1A9C);
    const u32 w = load<u32>(kWorldPtr);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = w;
    ctx.gpr[7] = n;
    ctx.gpr[8] = 0;
    if (n != 0) {
        for (u32 i = 0;;) {
            const u32 v = load<u32>(0x010BE8BCu + 152 * i);
            ctx.gpr[9] = v;
            if (v != 0) {
                const u32 f = load<u8>(w + 80 + i + 3948) & 0x4u;
                ctx.gpr[9] = f;
                if (f != 0) {
                    ret(ctx, 1u);
                    return;
                }
            }
            ++i;
            ctx.gpr[8] = i;
            ctx.gpr[5] = w + 80 + i;
            if (!(n > i)) break;
        }
    }
    ret(ctx, 0u);
}
// u32 f(Effect* e, u32 h): c = Effect_Bind4BB7E0(e, h, 0); c->+280 when
// c->+276 is 0x7A30D7E3, else 0 (0 for a null e)
DECOMP(004A66D0, Effect_GetParam_004A66D0) {
    const u32 e = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 c = static_cast<u32>(call(ctx, 0x004BB7E0, {e, h, 0}));
    if (c == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 k = load<u32>(c + 276);
    ctx.gpr[4] = c;
    ctx.gpr[5] = 0x7A30D7E3;
    ctx.gpr[6] = k;
    ret(ctx, k == 0x7A30D7E3u ? load<u32>(c + 280) : 0u);
}
// T::~T: vtable 0x00B21EF8, +40 = 0 (when set), the PtrVector at +20
// destroyed, then vtable 0x00B11B20 (r3 left as it)
DECOMP(004BFCC0, VtB21EF8_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 40);
    store<u32>(o, 0x00B21EF8);
    if (p != 0) store<u32>(o + 40, 0);
    VecDtor(ctx, o + 20);
    store<u32>(o, 0x00B11B20);
    ret(ctx, 0x00B11B20u);
}
// T::T(T* o): vtable 0x00B23020, the u8 +4 = 1, +8 = 0, the slot lists at
// +12 (SlotList_Init_0079B250) and +48 (SlotList_Init_0079B0F0) with 8
// slots each, +84 = +88 = 0
DECOMP(004F6ED8, VtB23020_Ctor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B23020);
    store<u8>(o + 4, 1);
    store<u32>(o + 8, 0);
    call(ctx, 0x0079B250, {o + 12, 8});
    call(ctx, 0x0079B0F0, {o + 48, 8});
    store<u32>(o + 84, 0);
    store<u32>(o + 88, 0);
}
// void f(Vec3* d, const Vec3* a, const Vec3* b; f32 t): d = a + (b - a) *
// t, computed in place in d as the original does (single rounding each
// step; f1..f4 left)
DECOMP(00735BA0, Vec3_Lerp_00735BA0) {
    const u32 d = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 t = farg(ctx, 0);
    store<f32>(d, static_cast<f32>(LfsAt(b) - LfsAt(a)));
    store<f32>(d + 4, static_cast<f32>(LfsAt(b + 4) - LfsAt(a + 4)));
    const f64 z = static_cast<f32>(LfsAt(b + 8) - LfsAt(a + 8));
    const f64 x = static_cast<f32>(LfsAt(d) * t);
    store<f32>(d + 8, static_cast<f32>(z));
    store<f32>(d, static_cast<f32>(x));
    const f64 y = static_cast<f32>(LfsAt(d + 4) * t);
    store<f32>(d + 4, static_cast<f32>(y));
    const f64 zt = static_cast<f32>(LfsAt(d + 8) * t);
    store<f32>(d + 8, static_cast<f32>(zt));
    const f64 a0 = LfsAt(a);
    store<f32>(d, static_cast<f32>(x + a0));
    const f64 a1 = LfsAt(a + 4);
    store<f32>(d + 4, static_cast<f32>(y + a1));
    const f64 a2 = LfsAt(a + 8);
    const f32 r = static_cast<f32>(zt + a2);
    store<f32>(d + 8, r);
    ctx.fpr[1] = r;
    ctx.fpr[2] = a2;
    ctx.fpr[3] = a1;
    ctx.fpr[4] = a0;
}
// VtB123D0_Dtor called through the interface 344 bytes in
DECOMP(0074731C, VtB123D0_DtorAdj344) { VtB123D0DtorAt(ctx, arg<u32>(ctx, 0) - 344); }
// The body of 0x0074BE68 / 0x0074D6A8: s * (k / |s|) for the quaternion at
// s (fused sums, single sqrt); k the f32 at kaddr.
struct QScale {
    f64 x, y, z, w, m;
};
static QScale QuatScale(u32 s, u32 kaddr) {
    QScale q;
    q.x = LfsAt(s);
    q.y = LfsAt(s + 4);
    q.z = LfsAt(s + 8);
    q.w = LfsAt(s + 12);
    f64 n = static_cast<f32>(q.y * q.y);
    n = static_cast<f32>(std::fma(q.x, q.x, n));
    n = static_cast<f32>(std::fma(q.z, q.z, n));
    n = static_cast<f32>(std::fma(q.w, q.w, n));
    n = static_cast<f32>(std::sqrt(n));
    q.m = static_cast<f32>(LfsAt(kaddr) / n);
    return q;
}
// void f(Quat* d, const Quat* s): d = s normalized (k = 1.0f at
// 0x0074BE60); each s component re-read after the previous store
DECOMP(0074BE68, Quat_Normalize_0074BE68) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const QScale q = QuatScale(s, 0x0074BE60);
    f64 v = 0;
    for (u32 k = 0; k < 4; ++k) {
        v = static_cast<f32>((k == 0 ? q.x : LfsAt(s + 4 * k)) * q.m);
        store<f32>(d + 4 * k, static_cast<f32>(v));
    }
    ctx.fpr[1] = v;
    ctx.fpr[2] = q.m;
    ctx.fpr[3] = LfsAt(0x0074BE60);
    ctx.fpr[4] = q.w;
    ctx.gpr[5] = 0x0074BE60;
    ctx.gpr[6] = s + 12;
}
// void f(Quat* q): q normalized in place (k = 1.0f at 0x0074D6A0)
DECOMP(0074D6A8, Quat_NormalizeSelf_0074D6A8) {
    const u32 s = arg<u32>(ctx, 0);
    const QScale q = QuatScale(s, 0x0074D6A0);
    const f64 a = static_cast<f32>(q.x * q.m), b = static_cast<f32>(LfsAt(s + 4) * q.m);
    const f64 c = static_cast<f32>(LfsAt(s + 8) * q.m), e = static_cast<f32>(LfsAt(s + 12) * q.m);
    store<f32>(s, static_cast<f32>(a));
    store<f32>(s + 4, static_cast<f32>(b));
    store<f32>(s + 8, static_cast<f32>(c));
    store<f32>(s + 12, static_cast<f32>(e));
    ctx.fpr[1] = a;
    ctx.fpr[3] = b;
    ctx.fpr[4] = c;
    ctx.fpr[2] = e;
    ctx.fpr[5] = q.z;
    ctx.fpr[6] = q.w;
    ctx.gpr[4] = 0x0074D6A0;
    ctx.gpr[5] = s + 12;
}
// VtB13974_B194F8_Delete called through the interface 8 bytes in
DECOMP(0075A6FC, VtB13974_B194F8_DeleteAdj8) { VtB13974DeleteAt(ctx, arg<u32>(ctx, 0) - 8); }
// T::~T: when +92 is set, vtable 0x00B1CE10, Mem_FreeFrom(+92, 0) and +92
// = 0; then vtable 0x00B0DAAC and SortedMap_Remove7330C0(the map at
// 0x01056F04, o + 4)
DECOMP(0076528C, VtB1CE10_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 92);
    if (p != 0) {
        store<u32>(o, 0x00B1CE10);
        call(ctx, o25fn::Mem_FreeFrom, {p, 0});
        store<u32>(o + 92, 0);
    }
    store<u32>(o, 0x00B0DAAC);
    call(ctx, 0x007330C0, {0x01056F04, o + 4});
}
// void f(?, T* a): for the entity p = a + the s8 a->+32 + 16 (when the s8
// a->+40 is 1 and p is not null) whose u8 +232 is neither 4 nor 5:
// Hk_RemoveEntity92FFC8(&tmp, p->+8)
DECOMP(00771344, Hk_Remove_00771344) {
    const u32 a = arg<u32>(ctx, 1);
    const s32 k = static_cast<s8>(load<u8>(a + 40));
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(k));
    if (k != 1) return;
    const u32 p = a + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(a + 32)))) + 16;
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 b = load<u8>(p + 232);
    ctx.gpr[4] = b - 4;
    ctx.gpr[5] = b - 5;
    if (b == 5 || b == 4) return;
    Stack frame(ctx, 16);
    call(ctx, 0x0092FFC8, {frame.addr(), load<u32>(p + 8), p});
}
// VtB205B8_Delete called through the interface 56 bytes in
DECOMP(0077EC1C, VtB205B8_DeleteAdj56) { VtB205B8DeleteAt(ctx, arg<u32>(ctx, 0) - 56); }

// ---------------------------------------------------------------- sweep: 32 instructions
// bool f(const T* o): bit 0x4 of the u8 +304 set, the radius +208 above the
// f32 at 0x00012D58 and the squared distance from +96..+104 to +176..+184
// below its square (fused sums; f1..f5 left)
DECOMP(00012D60, Obj_InRadius_00012D60) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f = load<u8>(o + 304) & 0x4u;
    ctx.gpr[4] = f;
    if (f == 0) {
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[4] = 0x00012D58;
    const f64 r = LfsAt(o + 208), k = LfsAt(0x00012D58);
    ctx.fpr[1] = r;
    ctx.fpr[2] = k;
    if (!(r > k)) {
        ret(ctx, 0u);
        return;
    }
    const u32 p = o + 176;
    ctx.gpr[4] = p;
    const f64 r2 = static_cast<f32>(r * r);
    const f64 dy = static_cast<f32>(LfsAt(o + 100) - LfsAt(p + 4));
    const f64 dx = static_cast<f32>(LfsAt(o + 96) - LfsAt(p));
    const f64 c = LfsAt(o + 104), cz = LfsAt(p + 8);
    const f64 dz = static_cast<f32>(c - cz);
    f64 s = static_cast<f32>(dy * dy);
    s = static_cast<f32>(std::fma(dx, dx, s));
    s = static_cast<f32>(std::fma(dz, dz, s));
    ctx.fpr[1] = r2;
    ctx.fpr[2] = s;
    ctx.fpr[3] = dx;
    ctx.fpr[4] = dz;
    ctx.fpr[5] = c;
    ret(ctx, s < r2 ? 1u : 0u);
}
// void f(T* o): +36 = 0, bit 0x4 of the u8 +305 cleared, +40 = the next
// number of the global generator at 0x01164888 (the 64-bit +0 converted and
// scaled by the f32 at 0x00014C70; then w = +8: x = (rotl(w, 4) + rotl(w,
// 16)) ^ 0x87654321, +8 = rotl(x, 16), +0 = x & 0xFFFF)
DECOMP(00014C78, Obj_RandomPhase_00014C78) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 g = 0x01164888;
    const u64 old = load<u64>(g);
    store<u32>(o + 36, 0);
    const u32 w = load<u32>(g + 8);
    const u64 x = (static_cast<u64>(std::rotl(w, 4)) + std::rotl(w, 16)) ^ 0xFFFFFFFF87654321ull;
    const u64 lo = x & 0xFFFFu;
    const u32 nw = std::rotl(static_cast<u32>(x), 16);
    store<u32>(g + 8, nw);
    store<u64>(g, lo);
    const u32 b = load<u8>(o + 305) & ~0x4u;
    store<u8>(o + 305, static_cast<u8>(b));
    const f64 v = static_cast<f32>(static_cast<f64>(static_cast<s64>(old)));
    const f32 r = static_cast<f32>(v * LfsAt(0x00014C70));
    store<f32>(o + 40, r);
    ctx.fpr[1] = r;
    ctx.fpr[2] = v;
    ctx.gpr[4] = b;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = nw;
    ctx.gpr[7] = g;
    ctx.gpr[8] = lo;
    ctx.gpr[9] = std::rotl(w, 4);
}
// void f(T* o): for p = the world's player block +4208->+8 with bit 0x8 in
// +192: bit 0x800 of Obj_Notify26(p)->+244 (when found) = bit 0 of the u8
// o->+12
DECOMP(00087130, Player_SetBit_00087130) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(load<u32>(kWorldPtr) + 4208 + 8);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 f = load<u32>(p + 192) & 0x8u;
    ctx.gpr[4] = f;
    if (f == 0) return;
    const u32 r = static_cast<u32>(call(ctx, 0x000D94CC, {p, f}));
    if (r == 0) return;
    const u32 b = load<u8>(o + 12);
    const u32 v = (load<u32>(r + 244) & ~0x800u) | ((b & 1u) << 11);
    store<u32>(r + 244, v);
    ctx.gpr[4] = b;
    ctx.gpr[5] = v;
}
// void f(T* o): pushes a new VtB20D10 (24 bytes) made from (+12, +16, +20)
DECOMP(0009B0F0, Cmd_PushB20D10_0009B0F0) {
    const u32 o = arg<u32>(ctx, 0);
    PushNewB(ctx, 24, [&](u32 p) { call(ctx, 0x0042D13C, {p, load<u32>(o + 12), load<u32>(o + 16), load<u32>(o + 20)}); });
}
// void f(T* o): pushes a new VtB20F20 (24 bytes) made from (f32 +12, +16,
// +20)
DECOMP(0009E70C, Cmd_PushB20F20_0009E70C) {
    const u32 o = arg<u32>(ctx, 0);
    PushNewB(ctx, 24, [&](u32 p) { callf(ctx, 0x0042FA20, {p}, {LfsAt(o + 12), LfsAt(o + 16), LfsAt(o + 20)}); });
}
// void f(T* o): pushes a new 0x004360CC object (160 bytes) made from (+12,
// the u8 +16, the u8 +17)
DECOMP(000A3608, Cmd_Push4360CC_000A3608) {
    const u32 o = arg<u32>(ctx, 0);
    PushNewB(ctx, 160, [&](u32 p) { call(ctx, 0x004360CC, {p, load<u32>(o + 12), load<u8>(o + 16), load<u8>(o + 17)}); });
}
// void f(T* o): pushes a new VtB213B8 (24 bytes) made from (+12, +16, the
// u8 +20)
DECOMP(000A6DE0, Cmd_PushB213B8_000A6DE0) {
    const u32 o = arg<u32>(ctx, 0);
    PushNewB(ctx, 24, [&](u32 p) { call(ctx, 0x0043A6DC, {p, load<u32>(o + 12), load<u32>(o + 16), load<u8>(o + 20)}); });
}
// T::~T: vtable 0x00B11B38; Emitter_Stop(the handle +20, 1) when either u16
// half of it is set; then VtB11B20_DtorTrivial_004B6884
DECOMP(000A7644, VtB11B38_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u16>(o + 20);
    store<u32>(o, 0x00B11B38);
    if (a != 0 || load<u16>(o + 22) != 0) call(ctx, o25fn::Emitter_Stop, {load<u32>(o + 20), 1});
    call(ctx, 0x004B6884, {o});
}
// void f(T* o, U* t, s32 k): the u16 +178 += 1; for k 3, bit t->+172 of the
// type-27 component's +88 set when the u64 t->+184 has bit 0
DECOMP(000DFAC8, Obj_CountAndMark_000DFAC8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 n = (load<u16>(o + 178) + 1u) & 0xFFFFu;
    store<u16>(o + 178, static_cast<u16>(n));
    ctx.gpr[6] = o + 178;
    if (arg<u32>(ctx, 2) != 3) {
        ctx.gpr[4] = n;
        return;
    }
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288, {o, 27, 0}));
    if (c == 0) return;
    const u32 f = static_cast<u32>(load<u64>(t + 184) & 1u);
    ctx.gpr[4] = f;
    if (f == 0) return;
    const u32 sh = load<u8>(t + 172) & 63u;
    const u32 v = load<u32>(c + 88) | (sh >= 32 ? 0u : 1u << sh);
    store<u32>(c + 88, v);
    ctx.gpr[4] = v;
}
// u32 f(const T* o): +412, unless world->+0x1281D4->+4 is set: then the
// high half of +28->+44 of the last 48-byte entry of the list +388 (+0 the
// array, the u16 +8 the count, re-read each step) whose +28 is set (+412
// when none)
DECOMP(000E63C8, Obj_LastHalf_000E63C8) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = load<u32>(o + 412);
    const u32 g = load<u32>(load<u32>(load<u32>(kWorldPtr) + 0x1281D4) + 4);
    ctx.gpr[4] = g;
    if (g != 0) {
        const u32 l = load<u32>(o + 388);
        ctx.gpr[4] = 0;
        for (u32 i = 0;; ++i) {
            const u32 n = l != 0 ? load<u16>(l + 8) : 0u;
            ctx.gpr[5] = 48 * i;
            ctx.gpr[6] = i;
            ctx.gpr[8] = n;
            if (i >= n) break;
            const u32 e = load<u32>(load<u32>(l) + 48 * i + 28);
            ctx.gpr[8] = e;
            if (e != 0) r = load<u32>(e + 44) >> 16;
        }
    }
    ctx.gpr[7] = r;
    ret(ctx, r);
}
// The shared body of 0x000ED898 / 0x000ED928 / 0x000ED9B8: when
// o->+216->+soff * a->+aoff exceeds the f32 at k, o->vfn[352](a->+4, the
// u16 a->+12) (the OPD in r4 and the entry in r7 here); otherwise r3 / r4
// are left as a / o.
static void ScaledVCall352(PPUContext& ctx, u32 aoff, u32 soff, u32 k) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const f64 m = LfsAt(load<u32>(o + 216) + soff);
    const f64 v = static_cast<f32>(m * LfsAt(a + aoff)), lim = LfsAt(k);
    ctx.fpr[1] = v;
    ctx.fpr[2] = lim;
    ctx.fpr[3] = m;
    if (!(v > lim)) {
        ctx.gpr[3] = a;
        ctx.gpr[4] = o;
        ctx.gpr[6] = a;
        return;
    }
    const u32 opd = load<u32>(load<u32>(o) + 352);
    call_ptr(ctx, opd, {o, opd, load<u32>(a + 4), load<u16>(a + 12), load<u32>(opd)});
}
// void f(T* o, U* a): +216->+204 * a->+64 against the f32 at 0x000ED890
DECOMP(000ED898, Obj_Threshold_000ED898) { ScaledVCall352(ctx, 64, 204, 0x000ED890); }
// void f(T* o, U* a): +216->+208 * a->+60 against the f32 at 0x000ED924
DECOMP(000ED928, Obj_Threshold_000ED928) { ScaledVCall352(ctx, 60, 208, 0x000ED924); }
// void f(T* o, U* a): +216->+212 * a->+32 against the f32 at 0x000ED9B4
DECOMP(000ED9B8, Obj_Threshold_000ED9B8) { ScaledVCall352(ctx, 32, 212, 0x000ED9B4); }
// T::~T: vtable 0x00B13C18; when the skeleton +76 is set: Body_Remove931F00(
// its +8, it) when +8 is set, AnimSkel_Release7D4EA8(+76), +76 = 0; then
// vtable 0x00B0DAEC (r3 left as it)
DECOMP(00107618, VtB13C18_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 76);
    store<u32>(o, 0x00B13C18);
    if (p != 0) {
        const u32 q = load<u32>(p + 8);
        if (q != 0) call(ctx, 0x00931F00, {q, p});
        call(ctx, 0x007D4EA8, {load<u32>(o + 76)});
        store<u32>(o + 76, 0);
    }
    store<u32>(o, 0x00B0DAEC);
    ret(ctx, 0x00B0DAECu);
}
