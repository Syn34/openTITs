// Game code, part 25: the continued sweep of small still-recompiled game
// functions (in size order, from 27 instructions up). Most have no known
// caller context yet, so their names describe what they do and carry the
// address. Each was checked against the original in the PPC simulator.
//
// Register notes: where a known caller reads a volatile register the
// original leaves behind (regflow liveout), the value is set the same way,
// as the comments say ("r4 left ...").
#include "obj25.h"

using namespace sly4;
using namespace sly4::obj25;

// ---------------------------------------------------------------- sweep: 27 instructions
// void f(T* o, u8 v): when v differs from the u8 +16, o->+24->vfn[16](v);
// then +16 = v
DECOMP(0001821C, Obj_SetByte16_0001821C) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1) & 0xFFu;
    const u32 cur = load<u8>(o + 16);
    ctx.gpr[3] = cur;
    if (v != cur) {
        const u32 p = load<u32>(o + 24);
        VCall(ctx, p, 16, {p, v});
    }
    store<u8>(o + 16, static_cast<u8>(v));
}
// void f(T* o): Global_SetHashObj_00369D14(o->+48, the string at
// 0x00AC01D4) when the kind +4 is 0
DECOMP(0002B3BC, Obj_SetHashIfKind0_0002B3BC) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 k = load<s32>(o + 4);
    ctx.gpr[4] = static_cast<u32>(k);
    // 9, 11, 8, 6, 5, 2 and every other nonzero kind are tested one by one
    if (k != 0) return;
    call(ctx, 0x00369D14, {load<u32>(o + 48), 0x00AC01D4});
}
// T::~T deleting form: vtable 0x00B0EF30, the global 0x00F557F8 = 0,
// Thunk_004B1308(o + 4), Mem_FreeFrom(o, 0). Null does nothing.
DECOMP(00065044, VtB0EF30_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B0EF30);
    store<u32>(0x00F557F8, 0);
    call(ctx, o25fn::Thunk_004B1308, {o + 4});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}
// void f(): p = world->+4208->+8; when p is set and its +192 has bit 0x8,
// Obj_Notify26(p) and, when that returns an object, Call_2A8AE0_244(it)
DECOMP(0006FDA0, World_NotifyFlagged_0006FDA0) {
    const u32 p = load<u32>(load<u32>(kWorldPtr) + 4208 + 8);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 f = load<u32>(p + 192) & 0x8u;
    ctx.gpr[4] = f;
    if (f == 0) return;
    const u32 r = static_cast<u32>(call(ctx, 0x000D94CC, {p, f}));
    if (r != 0) call(ctx, 0x002AB8F0, {r});
}
// void f(T* o): World_UserCall484BE0(world, o->+12, o->+16, 0, the u8 +20,
// 0, 0, 0, 1) when +12 >= 0 and +16 is set
DECOMP(0008D344, Obj_WorldUserCall_0008D344) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 a = load<s32>(o + 12);
    if (a < 0) return;
    const u32 b = load<u32>(o + 16);
    if (b == 0) return;
    call(ctx, 0x00488118, {load<u32>(kWorldPtr), Sx(static_cast<u32>(a)), b, 0, load<u8>(o + 20), 0, 0, 0, 1});
}
// s32 f(): the s32 at 0x00F645B0; when it is -2 (not started), start the
// thread (entry OPD 0x00F31068, priority 1001, 16 KiB stack, the name at
// 0x00AC5FF8) and set it to -1 first, which is then returned
DECOMP(000AF27C, Thread_StartOnce_000AF27C) {
    constexpr u32 kState = 0x00F645B0;
    s32 v = load<s32>(kState);
    if (v == -2) {
        Stack frame(ctx, 16);
        call(ctx, o25imp::sys_ppu_thread_create, {frame.addr(), 0x00F31068, 0, 1001, 16384, 0, 0x00AC5FF8});
        store<s32>(kState, -1);
        v = -1;  // the id is not kept: the state is always -1 afterwards
    }
    ret(ctx, v);
}
// void f(T* o, Node* n): when n->+76 is set, o->+676 |= 0x80000000 if the
// u16 n->+12 has bit 0x4, else by the top two bits of n->+76->+72: 1 ->
// 0x40000000, 2 -> 0x20000000, other -> 0x10000000
DECOMP(000BD6D0, Obj_SetKindBits_000BD6D0) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    const u32 p = load<u32>(n + 76);
    ctx.gpr[5] = p;
    if (p == 0) return;
    u32 bit;
    if ((load<u16>(n + 12) & 0x4u) != 0) {
        bit = 0x80000000u;
    } else {
        const u32 k = load<u32>(p + 72) >> 30;
        bit = k == 1 ? 0x40000000u : k == 2 ? 0x20000000u : 0x10000000u;
    }
    const u32 w = load<u32>(o + 676) | bit;
    store<u32>(o + 676, w);
    ctx.gpr[4] = w;
}
// void f(T* o, a) (o points 344 bytes into its object b): b->+376 |=
// 0x20000000, then b->+244->vfn[208](a) when +244 is set
DECOMP(000DF848, Obj_FlagAndForward_000DF848) {
    const u32 b = arg<u32>(ctx, 0) - 344, a = arg<u32>(ctx, 1);
    store<u32>(b + 376, load<u32>(b + 376) | 0x20000000u);
    const u32 q = load<u32>(b + 244);
    if (q == 0) {
        ret(ctx, a);
        return;
    }
    VCall(ctx, q, 208, {q, a});
}
// T::~T: vtable 0x00B13718, Emitter_Stop(+348, 1), RefNode_Release(+356),
// then the base destructor 0x000D7024
DECOMP(000F5EFC, VtB13718_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B13718);
    call(ctx, o25fn::Emitter_Stop, {load<u32>(o + 348), 1});
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 356)});
    call(ctx, 0x000D7024, {o});
}
// void f(T* o): c = 0x000DB290(o->+856, 0x366FD481) when +856 is set, then
// c->vfn[128](0x7B2728A1, 1) when found
DECOMP(001F01A8, Obj_CallComp_001F01A8) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 856);
    ctx.gpr[3] = p;
    if (p == 0) return;
    const u32 c = static_cast<u32>(call(ctx, o25fn::Obj_FindByHash_0DB290, {p, 0x366FD481}));
    if (c == 0) return;
    VCall(ctx, c, 128, {c, 0x7B2728A1, 1});
}
// void f(T* o, u32 hash, u8 b): Effect_SetHash4CF988(o->+428, hash) when
// +428 is set, then 0x000DA684(o, hash, b)
DECOMP(0020B95C, Obj_SetEffectHash_0020B95C) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2) & 0xFFu;
    const u32 e = load<u32>(o + 428);
    if (e != 0) call(ctx, 0x004CF988, {e, h});
    call(ctx, 0x000DA684, {o, h, b});
}
// T::~T: when +12 is set, vtable 0x00B1D578 and +12->vfn[8](); then
// vtable 0x00B1D4D8 (r3 left as it)
DECOMP(002F457C, VtB1D578_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 12);
    ctx.gpr[4] = 0x00B1D578;
    if (p != 0) {
        const u32 vt = load<u32>(p);  // read before the store (p may be o)
        store<u32>(o, 0x00B1D578);
        VCallVt(ctx, vt, 8, {p});
    }
    store<u32>(o, 0x00B1D4D8);
    ret(ctx, 0x00B1D4D8u);
}
// void f(T* o, f32 t): e = o->+48->+52; when set and not done
// (Effect_Check4CF620(e, 1) == 0), Effect_SetTime(e; t)
DECOMP(002FE448, Obj_EffectSetTime_002FE448) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    const u32 p = load<u32>(o + 48);
    if (p == 0) return;
    const u32 e = load<u32>(p + 52);
    ctx.gpr[3] = e;
    if (e == 0) return;
    if (call(ctx, 0x004CF620, {e, 1}) != 0) return;
    callf(ctx, 0x004CF710, {load<u32>(p + 52)}, {t});
}
// void f(T* o, u32 key, const Vec3* v): every 64-byte entry of the list
// o->+104 (+0 the array, the u16 +8 the count; re-read each step) whose +0
// is key gets +32..+40 = v. f1 left as the last word copied.
DECOMP(00352C6C, List_SetVecByKey_00352C6C) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(o + 104);
        const u32 n = l != 0 ? load<u16>(l + 8) : 0u;
        if (i >= n) break;
        const u32 e = load<u32>(l) + 64 * i;
        if (key != load<u32>(e)) continue;
        Cp(e + 32, v);
        Cp(e + 36, v + 4);
        ctx.fpr[1] = F32Bits(Cp(e + 40, v + 8));
    }
}
// void f(T* o): the u8 +164 = 1 when Anim_TrackSet36_0029879C(o->+96->
// vfn[60](), 0, 0) returns true
DECOMP(003ABB20, Obj_TrackReset_003ABB20) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 96);
    const u32 a = static_cast<u32>(VCall(ctx, p, 60, {p}));
    if (call(ctx, 0x0029879C, {a, 0, 0}) == 0) return;
    store<u8>(o + 164, 1);
    ctx.gpr[4] = 1;
}
// void f(): for the handle 0x8D2C4D64, Obj_Forward20(resolved, 0x72BAA908)
// when it resolves, then Handle_Notify36742C(handle, 0, 1, 0)
DECOMP(003C3784, Handle_Notify_003C3784) {
    constexpr u32 kHandle = 0x8D2C4D64;
    const u32 r = static_cast<u32>(call(ctx, 0x003568E0, {kHandle}));
    if (r != 0) call(ctx, 0x0036A4C0, {r, 0x72BAA908});
    call(ctx, 0x0036742C, {kHandle, 0, 1, 0});
}
// void f(T* o, u8 b): o->+1212->vfn[40](b == 0, 1) when +1212 is set
// (otherwise r3 is left as b)
DECOMP(003CB1BC, Obj_VCall40_003CB1BC) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 b = ctx.gpr[4];
    const u32 p = load<u32>(o + 1212);
    ctx.gpr[3] = b;
    ctx.gpr[4] = p;
    ctx.gpr[5] = o;
    if (p == 0) return;
    VCall(ctx, p, 40, {p, (b & 0xFFu) == 0 ? 1u : 0u, 1});
}
// T::T(T* o, a, b): the 0x00B20CC8 base (o, 1), +16 = a, +24 = b, +12 =
// +20 = 0, vtable 0x00B20DA0 (r3 / r4 left as the vtable / 0)
DECOMP(0042DDDC, VtB20DA0_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 16, a);
    store<u32>(o + 24, b);
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B20DA0);
    store<u32>(o + 20, 0);
    ctx.gpr[4] = 0;
    ret(ctx, 0x00B20DA0u);
}
// void f(T* o): Rec_Set_00470560(world + 0x1283A8; o->+12, +16, +20,
// (s32)+24, the u8 +28), then Flag_Set8_40000000(o). r5 / r6 pass through.
DECOMP(0042F1D4, Obj_SetRec_0042F1D4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    callf(ctx, 0x00470560, {w + 0x1283A8u, w, ctx.gpr[5], ctx.gpr[6], Sx(load<u32>(o + 24)), load<u8>(o + 28)},
          {LfsAt(o + 12), LfsAt(o + 16), LfsAt(o + 20)});
    call(ctx, o24fn::Flag_Set8_40000000, {o});
}
// u32 f(T* o): Obj_SetState30_0048A0E4(world, o->+12), Call_470750_0(world,
// 7), 0x00371968(Player_Get()), the u8 +16 = 1; returns 1
DECOMP(00434D94, Obj_Start_00434D94) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0048A0E4, {load<u32>(kWorldPtr), load<u32>(o + 12)});
    call(ctx, 0x00472208, {load<u32>(kWorldPtr), 7});
    const u32 pl = static_cast<u32>(call(ctx, 0x00365C24));
    call(ctx, 0x00371968, {pl});
    store<u8>(o + 16, 1);
    ret(ctx, 1u);
}
// T::T(T* o, a, bool b): the 0x00B20CC8 base (o, 1), +12 = a, bit 31 of
// +16 = b, vtable 0x00B212F8 (r3 / r4 left as the +16 word / the vtable)
DECOMP(00438F3C, VtB212F8_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    const u32 w = (load<u32>(o + 16) & 0x7FFFFFFFu) | (b << 31);
    store<u32>(o + 12, a);
    store<u32>(o, 0x00B212F8);
    store<u32>(o + 16, w);
    ctx.gpr[4] = 0x00B212F8;
    ret(ctx, w);
}
// void f(T* o): r = 0x0042BD0C(o->+12) when +12 is set; when found, r->+24
// |= 0x80000000 and 0x0042BC44(r); then Flag_Set8_40000000(o)
DECOMP(0043A670, Obj_MarkAndFlag_0043A670) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 12);
    if (p != 0) {
        const u32 r = static_cast<u32>(call(ctx, 0x0042BD0C, {p}));
        if (r != 0) {
            const u32 old = load<u32>(r + 24), w = old | 0x80000000u;
            store<u32>(r + 24, w);
            call(ctx, 0x0042BC44, {r, r, w, old});
        }
    }
    call(ctx, o24fn::Flag_Set8_40000000, {o});
}
// T::T(T* o, u32 hash, b): the 0x00B20CC8 base (o, 1), vtable 0x00B21418,
// +12 = Hash_ToIndex_0044D118(hash), +16 = b
DECOMP(0043AD8C, VtB21418_Ctor) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o, 0x00B21418);
    const u32 r = static_cast<u32>(call(ctx, 0x0044D118, {h}));
    store<u32>(o + 12, r);
    store<u32>(o + 16, b);
}
// s32 f(?, ?, u32 i): in the 72-byte record i of the table at 0x010B0BC8,
// the first nonzero word of the 16 at +8 is cleared and -2 returned; 0
// when all are 0
DECOMP(00450214, Table_ClearFirst_00450214) {
    const u32 rec = 0x010B0BC8u + static_cast<u32>(ctx.gpr[5]) * 72u;
    ctx.gpr[4] = 0;
    ctx.gpr[6] = rec;
    for (u32 i = 0; i < 16; ++i) {
        const u32 w = load<u32>(rec + 8 + 4 * i);
        ctx.gpr[7] = w;
        if (w != 0) {
            store<u32>(rec + 8 + 4 * i, 0);
            ctx.gpr[5] = rec + 8 + 4 * i;
            ret(ctx, -2);
            return;
        }
    }
    ctx.gpr[5] = 16;
    ret(ctx, 0u);
}
// T::~T: vtable 0x00B219C0; when +28 is set, +28->vfn[32](0) and +28 = 0.
// Returns 0.
DECOMP(00457100, VtB219C0_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B219C0);
    const u32 p = load<u32>(o + 28);
    ctx.gpr[4] = 0x00B219C0;
    if (p != 0) {
        VCall(ctx, p, 32, {p, 0});
        store<u32>(o + 28, 0);
    }
    ret(ctx, 0u);
}
// void f(T* o): 0x004A0990(o), then +28 = (1 - +12) * +20 + +12 * +24
// (fused; 1.0f the constant at 0x004A0E5C), also stored through +32 when
// that is set. f1 left as the result, f2..f4 as the operands.
DECOMP(004A0E60, Obj_Blend_004A0E60) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x004A0990, {o, 0x004A0000});
    const f32 k = load<f32>(0x004A0E5C), a = load<f32>(o + 12), b = load<f32>(o + 24), c = load<f32>(o + 20);
    const f32 d = static_cast<f32>(static_cast<f64>(k) - a);
    const f32 m = static_cast<f32>(static_cast<f64>(a) * b);
    const f32 r = static_cast<f32>(std::fma(static_cast<f64>(d), static_cast<f64>(c), static_cast<f64>(m)));
    store<f32>(o + 28, r);
    const u32 p = load<u32>(o + 32);
    if (p != 0) store<f32>(p, r);
    ctx.fpr[1] = r;
    ctx.fpr[2] = a;
    ctx.fpr[3] = m;
    ctx.fpr[4] = c;
    ret(ctx, p);
}
// f32 f(f32 x, f32 e): a smooth ease with the knee at e (the constants a,
// b at 0x004A57DC / 0x004A57E0): x < e: (x * b * x) / (e * (a - e));
// x > a - e: a - ((a - x) * b * (a - x)) / (e * (a - e)); else
// (x - e * b) / (a - e)
DECOMP(004A57E8, Ease_Knee_004A57E8) {
    const f64 x = farg(ctx, 0), e = farg(ctx, 1);
    const f64 a = LfsAt(0x004A57DC), b = LfsAt(0x004A57E0);
    const f64 m = static_cast<f32>(a - e);
    f32 r;
    if (x < e) {
        const f64 t = static_cast<f32>(x * b), d = static_cast<f32>(e * m);
        r = static_cast<f32>(static_cast<f64>(static_cast<f32>(t * x)) / d);
    } else if (x > m) {
        const f64 u = static_cast<f32>(a - x), d = static_cast<f32>(e * m);
        const f64 t = static_cast<f32>(u * b);
        r = static_cast<f32>(a - static_cast<f64>(static_cast<f32>(static_cast<f64>(static_cast<f32>(t * u)) / d)));
    } else {
        const f64 t = static_cast<f32>(-std::fma(e, b, -x));
        r = static_cast<f32>(t / m);
    }
    ctx.gpr[3] = 0x004A57DC;  // r3 left as the constants' address
    ret(ctx, r);
}
// T::~T: when +28 is set, vtable 0x00B21DC8 and +28->vfn[8](); then
// vtable 0x00B11B20 (r3 left as it)
DECOMP(004BEF40, VtB21DC8_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 28);
    ctx.gpr[4] = 0x00B21DC8;
    if (p != 0) {
        const u32 vt = load<u32>(p);  // read before the store (p may be o)
        store<u32>(o, 0x00B21DC8);
        VCallVt(ctx, vt, 8, {p});
    }
    store<u32>(o, 0x00B11B20);
    ret(ctx, 0x00B11B20u);
}
// bool f(WorldObj* o, const u8* b): when o has a peer, bit 0x20 of the u8
// Peer_GetVec(peer)->+28 = bit 0 of *b. Returns 1.
DECOMP(004CC25C, Obj_SetPeerBit_004CC25C) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    if (call(ctx, 0x0057A428, {o, b}) != 0) {
        const u32 p = static_cast<u32>(call(ctx, 0x0057A428, {o}));
        const u32 bit = load<u8>(b) & 1u;
        const u32 v = static_cast<u32>(call(ctx, 0x0057BE98, {p}));
        store<u8>(v + 28, static_cast<u8>((load<u8>(v + 28) & ~0x20u) | (bit << 5)));
    }
    ret(ctx, 1u);
}
// T::~T deleting form, called through the interface 380 bytes in: vtables
// 0x00B126B8 / 0x00B1200C (at +380), VtB12550_Dtor, SmallFree(o, 448).
// Null does nothing.
DECOMP(007443FC, VtB126B8_DeleteAdj380) {
    const u32 o = arg<u32>(ctx, 0) - 380;
    if (o == 0) return;
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o});
    call(ctx, 0x00552420, {o, 448});
}
// f32 f(Rng* s): the next random number: the 64-bit +0 converted and
// scaled by the constants at 0x0074E198 (x * k0 * k1 - k2, fused); then
// w = +8: x = (rotl(w, 4) + rotl(w, 16)) ^ 0x87654321, +8 = rotl(x, 16),
// +0 = x & 0xFFFF. f2..f4 / r4..r6 left.
DECOMP(0074E1A8, Rng_Next_0074E1A8) {
    const u32 s = arg<u32>(ctx, 0);
    const u64 old = load<u64>(s);
    const u32 w = load<u32>(s + 8);
    const u64 x = (static_cast<u64>(std::rotl(w, 4)) + std::rotl(w, 16)) ^ 0xFFFFFFFF87654321ull;
    const u64 lo = x & 0xFFFFu;
    const u32 nw = std::rotl(static_cast<u32>(x), 16);
    store<u32>(s + 8, nw);
    store<u64>(s, lo);
    const f64 k0 = LfsAt(0x0074E198), k1 = LfsAt(0x0074E19C), k2 = LfsAt(0x0074E1A0);
    const f64 v = static_cast<f32>(static_cast<f64>(static_cast<s64>(old)));
    const f64 t = static_cast<f32>(v * k0);
    ctx.fpr[2] = v;
    ctx.fpr[3] = k1;
    ctx.fpr[4] = k2;
    ctx.gpr[4] = 0x0074E198;
    ctx.gpr[5] = nw;
    ctx.gpr[6] = lo;
    ctx.gpr[7] = std::rotl(w, 4);
    ret(ctx, static_cast<f32>(std::fma(t, k1, -k2)));
}
// void f(Array* a, s32 i): removes the 8-byte element i (the u16 +8 the
// count, re-read each step): the later ones move down, the count drops by
// one. i == the last index (or i past it) only drops the count.
DECOMP(00755C34, Array8_RemoveAt_00755C34) {
    const u32 a = arg<u32>(ctx, 0);
    u32 n = load<u16>(a + 8);
    const u32 i0 = arg<u32>(ctx, 1);
    if (static_cast<s32>(i0) != static_cast<s32>(n - 1)) {
        u32 i = i0 & 0xFFFFu;
        while (static_cast<s32>(i) < static_cast<s32>(n - 1)) {
            const u32 e = load<u32>(a) + 8 * i;
            i = (i + 1) & 0xFFFFu;
            n = load<u16>(a + 8);
            Cp(e, e + 8);
            Cp(e + 4, e + 12);
        }
    }
    store<u16>(a + 8, static_cast<u16>(n - 1));
    ctx.gpr[4] = n - 1;
}
// s32 f(const T* a, const T* b): compares by the u64 +8 (-1 / 0 / 1); a
// null on either side compares the pointers (0 when equal, else -1 when
// a < b, unsigned, and 1 otherwise)
DECOMP(00767418, Cmp_U64At8_00767418) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    if (a == 0 || b == 0) {
        ret(ctx, a == b ? 0 : (a < b ? -1 : 1));
        return;
    }
    const u64 x = load<u64>(a + 8), y = load<u64>(b + 8);
    ctx.gpr[4] = x;
    ret(ctx, x < y ? -1 : (x > y ? 1 : 0));
}
// void f(T* o, const U* a): q = a->+4; when the s8 q->+24 is 1 and q plus
// the s8 q->+16 is not null, o->vfn[16](that)
DECOMP(007713C0, Obj_VCall16Rel_007713C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(arg<u32>(ctx, 1) + 4);
    if (static_cast<s8>(load<u8>(q + 24)) != 1) return;
    const u32 p = q + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(q + 16))));
    if (p == 0) return;
    VCall(ctx, o, 16, {o, p});
}

// ---------------------------------------------------------------- sweep: 28 instructions
// bool f(const T* o): 1 unless the kind +8 is 9; for kind 9, whether the
// squared distance from +96..+104 to +176..+184 is below +208 (fused sums;
// f1..f4 / r4 left)
DECOMP(00024FF8, Obj_InRange_00024FF8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 8);
    if (k != 9) {
        ctx.gpr[5] = k;
        ctx.gpr[4] = 1;
        ret(ctx, 1u);
        return;
    }
    const u32 p = o + 176;
    ctx.gpr[5] = p;
    const f64 d1 = static_cast<f32>(LfsAt(o + 100) - LfsAt(p + 4));
    const f64 d2 = static_cast<f32>(LfsAt(o + 96) - LfsAt(p));
    const f64 d3 = static_cast<f32>(LfsAt(o + 104) - LfsAt(p + 8));
    const f64 lim = LfsAt(o + 208);
    f64 s = static_cast<f32>(d1 * d1);
    s = static_cast<f32>(std::fma(d2, d2, s));
    s = static_cast<f32>(std::fma(d3, d3, s));
    const u32 r = s < lim ? 1u : 0u;
    ctx.fpr[1] = s;
    ctx.fpr[2] = d2;
    ctx.fpr[3] = d3;
    ctx.fpr[4] = lim;
    ctx.gpr[4] = r;
    ret(ctx, r);
}
// void f(T* o): for the level o->+12 (when set): World_SetFlags44(world, 0)
// when it is the world's current one (+0x128114), else World_StartLevel(
// world, level, 0)
DECOMP(00080C88, Obj_StartLevel_00080C88) {
    const u32 lv = load<u32>(arg<u32>(ctx, 0) + 12);
    ctx.gpr[3] = lv;
    if (lv == 0) return;
    const u32 w = load<u32>(kWorldPtr);
    if (load<u32>(w + 0x128114) == lv) call(ctx, 0x004896F8, {w, 0});
    else call(ctx, 0x004674C4, {w, lv, 0});
}
// T::~T deleting form: vtable 0x00B0FF40, RefNode_Release(+24), +24 = 0,
// VtB11B20_DtorTrivial, Mem_FreeFrom(o, 0). Null does nothing.
DECOMP(00083CE0, VtB0FF40_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B0FF40);
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 24)});
    store<u32>(o + 24, 0);
    call(ctx, 0x004BB41C, {o});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}
// s32 f(): the sum of the s32 +4 of the first n 8-byte records of the
// array at *0x00F64620 (re-read each step), n the u16 +200 of the object
// at *0x00F668E8 when its u8 +8 is set, else 0
DECOMP(000B29B0, Rec_SumCounts_000B29B0) {
    const u32 s = load<u32>(0x00F668E8);
    s32 n = 0;
    if (load<u8>(s + 8) != 0) n = static_cast<s32>(load<u32>(s + 200) & 0xFFFFu);
    s32 sum = 0;
    u32 off = 0;
    while (n > 0) {
        --n;
        sum = static_cast<s32>(static_cast<u32>(sum) + load<u32>(load<u32>(0x00F64620) + off + 4));
        off += 8;
    }
    ctx.gpr[4] = static_cast<u32>(n);
    ret(ctx, sum);
}
// T::~T: vtables 0x00B13528 / 0x00B136C8 (+380) / 0x00B136F0 (+384), the
// members at +680 and +644 (VtB21B58_DtorTrivial), then the base
// destructor 0x000EB0F8
DECOMP(000F4ED0, VtB13528_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B13528);
    store<u32>(o + 380, 0x00B136C8);
    store<u32>(o + 384, 0x00B136F0);
    call(ctx, 0x004A0E08, {o + 680});
    call(ctx, 0x004A0E08, {o + 644});
    call(ctx, 0x000EB0F8, {o});
}
// T::~T: vtable 0x00B13EB8, +16->vfn[84](o + 32), then vtable 0x00B0DAEC
// (r3 left as it)
DECOMP(001125D8, VtB13EB8_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B13EB8);
    const u32 p = load<u32>(o + 16);
    VCall(ctx, p, 84, {p, o + 32});
    store<u32>(o, 0x00B0DAEC);
    ret(ctx, 0x00B0DAECu);
}
// bool f(const T* o): whether +720..+728 and +704..+712 differ by less than
// the constant at 0x001CF688 in each of the three components (f1..f4 left
// as on the path taken)
DECOMP(001CF690, Obj_Near3_001CF690) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = LfsAt(0x001CF688);
    ctx.fpr[3] = k;
    ctx.gpr[4] = 0;
    f64 d = std::fabs(static_cast<f64>(static_cast<f32>(LfsAt(o + 720) - LfsAt(o + 704))));
    ctx.fpr[4] = d;
    ctx.fpr[2] = LfsAt(o + 708);
    ctx.fpr[1] = LfsAt(o + 712);
    u32 r = 0;
    if (d < k) {
        ctx.fpr[4] = LfsAt(o + 724);
        d = std::fabs(static_cast<f64>(static_cast<f32>(ctx.fpr[4] - ctx.fpr[2])));
        ctx.fpr[2] = d;
        if (d < k) {
            ctx.fpr[2] = LfsAt(o + 728);
            d = std::fabs(static_cast<f64>(static_cast<f32>(ctx.fpr[2] - ctx.fpr[1])));
            ctx.fpr[1] = d;
            if (d < k) r = 1;
        }
    }
    ctx.gpr[4] = r;
    ret(ctx, r);
}
// T::~T deleting form: vtable 0x00B17DB0, Mem_FreeIfSet(+600, +596) when
// +600 is set, Thunk_000E2A34, SmallFree(o, 640). Null does nothing.
DECOMP(001DC910, VtB17DB0_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 p = load<u32>(o + 600);
    store<u32>(o, 0x00B17DB0);
    if (p != 0) call(ctx, 0x00555CC0, {p, load<u32>(o + 596)});
    call(ctx, 0x000E2A34, {o});
    call(ctx, 0x00552420, {o, 640});
}
// void f(T* o, const U* a): a->+4->vfn[40](0) when set, then
// 0x001ED85C(o, a->+0)
DECOMP(001ED964, Obj_Forward_001ED964) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 p = load<u32>(a + 4);
    if (p != 0) VCall(ctx, p, 40, {p, 0});
    call(ctx, 0x001ED85C, {o, load<u32>(a)});
}
// void f(Object* o, bool b): bit 0x10000000 of Target_GetDef0D951C(o)->+124
// = b, unless b is set and o->+980 has no bit 0x01000000
DECOMP(001EF800, Target_SetBit28_001EF800) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1);
    const u32 t = static_cast<u32>(call(ctx, 0x000D951C, {o, b}));
    if ((b & 0xFFu) != 0) {
        const u32 f = (load<u32>(o + 980) >> 24) & 1u;
        ctx.gpr[4] = f;
        if (f == 0) return;
    }
    const u32 w = (load<u32>(t + 124) & ~0x10000000u) | ((b & 1u) << 28);
    store<u32>(t + 124, w);
    ctx.gpr[4] = w;
}
// void f(const char* name, ?, T* t; f32 v): when t is set, name is the
// string at 0x00AD4F54 and t->+168 differs from v: +168 = v and the u8
// +456 |= 0x80
DECOMP(0028BD40, Obj_SetByName_0028BD40) {
    const u32 t = arg<u32>(ctx, 2);
    const f64 v = farg(ctx, 0);
    if (t == 0) return;
    if (call(ctx, 0x00555D94, {arg<u32>(ctx, 0), 0x00AD4F54}) != 0) return;
    const f64 cur = LfsAt(t + 168);
    ctx.fpr[1] = cur;
    if (cur == v) return;
    store<f32>(t + 168, static_cast<f32>(v));
    const u32 b = load<u8>(t + 456) | 0x80u;
    store<u8>(t + 456, static_cast<u8>(b));
    ret(ctx, b);
}
// void f(T* o): when Obj_Owner0D93FC(o->+16)->+40 has bit 0x80000000, the
// u8 +112 counts up to 3 (+20 = 0 on each step); otherwise +20 = 0 and +112
// = 0
DECOMP(002ECD78, Obj_CountUp_002ECD78) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = static_cast<u32>(call(ctx, 0x000D93FC, {load<u32>(o + 16)}));
    if ((load<u32>(q + 40) & 0x80000000u) == 0) {
        store<u32>(o + 20, 0);
        store<u8>(o + 112, 0);
        ret(ctx, 0u);
        return;
    }
    const u32 c = load<u8>(o + 112);
    if (static_cast<s32>(c) < 3) {
        store<u8>(o + 112, static_cast<u8>(c + 1));
        store<u32>(o + 20, 0);
        ctx.gpr[4] = 0;
        ret(ctx, c + 1);
        return;
    }
    ret(ctx, c);
}
// The shared body of 0x002F98D0 / 0x002FCC14: e = o->+8->+52; when set and
// not done (Effect_Check4CF620(e, 1) == 0), Effect_SetTime(e; s * o->+12).
static void EffectSetTimeScaled(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 8);
    if (p == 0) return;
    const f64 t = static_cast<f32>(farg(ctx, 0) * LfsAt(o + 12));
    const u32 e = load<u32>(p + 52);
    ctx.gpr[3] = e;
    if (e == 0) return;
    if (call(ctx, 0x004CF620, {e, 1}) != 0) return;
    callf(ctx, 0x004CF710, {load<u32>(p + 52)}, {t});
}
// void f(T* o, f32 s): see EffectSetTimeScaled
DECOMP(002F98D0, Obj_EffectSetTimeScaled_002F98D0) { EffectSetTimeScaled(ctx); }
// void f(T* o, f32 s): see EffectSetTimeScaled
DECOMP(002FCC14, Obj_EffectSetTimeScaled_002FCC14) { EffectSetTimeScaled(ctx); }
// void f(T* o, bool b): the u8 +41 = 0 when b; then +48->vfn[4]() and +48
// = 0 when +48 is set. Returns 0.
DECOMP(002FD910, Obj_ReleaseChild_002FD910) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 p = load<u32>(o + 48);
    if (b != 0) {
        store<u8>(o + 41, 0);
        ctx.gpr[4] = 0;
    }
    if (p != 0) {
        VCall(ctx, p, 4, {p});
        store<u32>(o + 48, 0);
    }
    ret(ctx, 0u);
}
// The tail shared by 0x003A3F90 / 0x00364E70: c = a->+208->+132->+48->+4->
// +128 + 8 (a->+208 non-null is checked by the callers); when both are set,
// c->+0 = 0 and bits 0x0FF00000 of c->+32 = 2 (r3..r6 left).
static void ResetStateFrom(PPUContext& ctx, u32 a) {
    const u32 c = load<u32>(load<u32>(load<u32>(load<u32>(load<u32>(a + 208) + 132) + 48) + 4) + 128);
    ctx.gpr[3] = c;
    if (c == 0) return;
    const u32 d = c + 8;
    ctx.gpr[3] = d;
    if (d == 0) return;  // (the original's r3 keeps the carry in its upper half)
    store<u32>(d, 0);
    const u32 w = (load<u32>(d + 32) & ~0x0FF00000u) | 0x00200000u;
    store<u32>(d + 32, w);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 2;
    ctx.gpr[6] = w;
}
// void f(?, T** pp): ResetStateFrom(*pp) when pp and *pp are set
DECOMP(003A3F90, Obj_ResetState_003A3F90) {
    const u32 pp = arg<u32>(ctx, 1);
    if (pp == 0) return;
    const u32 a = load<u32>(pp);
    ctx.gpr[3] = a;
    if (a == 0) return;
    ResetStateFrom(ctx, a);
}
// T::T(T* o, u8 a, b; f32 x): the 0x00B20CC8 base (o, 1), +12 = b, the u8
// +16 = a, +20 = x, vtable 0x00B20CE0 (r3 left as the vtable)
DECOMP(0042CC10, VtB20CE0_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 x = farg(ctx, 0);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, b);
    store<u8>(o + 16, static_cast<u8>(a));
    store<f32>(o + 20, static_cast<f32>(x));
    store<u32>(o, 0x00B20CE0);
    ret(ctx, 0x00B20CE0u);
}
// T::T(T* o, a, b, c): the 0x00B20CC8 base (o, 1), +12 / +16 / +20 = a / b
// / c, vtable 0x00B20D10 (r3 left as the vtable)
DECOMP(0042D13C, VtB20D10_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    store<u32>(o, 0x00B20D10);
    ret(ctx, 0x00B20D10u);
}
// T::T(T* o, a, b; f32 x): the 0x00B20CC8 base (o, 1), +12 / +16 / +20 = a
// / b / x, vtable 0x00B20D28 (r3 left as the vtable)
DECOMP(0042D20C, VtB20D28_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 x = farg(ctx, 0);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<f32>(o + 20, static_cast<f32>(x));
    store<u32>(o, 0x00B20D28);
    ret(ctx, 0x00B20D28u);
}
// T::T(T* o, a, b, u8 c): the 0x00B20CC8 base (o, 1), +12 / +16 / the u8
// +20 = a / b / c, vtable 0x00B20DB8 (r3 left as the vtable)
DECOMP(0042DED0, VtB20DB8_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u8>(o + 20, static_cast<u8>(c));
    store<u32>(o, 0x00B20DB8);
    ret(ctx, 0x00B20DB8u);
}
// T::T(T* o, a, u8 b, u8 c): the 0x00B20CC8 base (o, 1), +12 / the u8 +16
// / the u8 +17 = a / b / c, vtable 0x00B20DE8 (r3 left as the vtable)
DECOMP(0042E20C, VtB20DE8_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, a);
    store<u8>(o + 16, static_cast<u8>(b));
    store<u8>(o + 17, static_cast<u8>(c));
    store<u32>(o, 0x00B20DE8);
    ret(ctx, 0x00B20DE8u);
}
// T::T(T* o; f32 x, y, z): the 0x00B20CC8 base (o, 1), +12..+20 = x, y, z,
// vtable 0x00B20F20 (r3 left as the vtable)
DECOMP(0042FA20, VtB20F20_Ctor) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1), z = farg(ctx, 2);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<f32>(o + 12, static_cast<f32>(x));
    store<f32>(o + 16, static_cast<f32>(y));
    store<f32>(o + 20, static_cast<f32>(z));
    store<u32>(o, 0x00B20F20);
    ret(ctx, 0x00B20F20u);
}
// bool f(const T* o): 1 when +28 is not set or Obj_Notify26(+28) finds
// nothing; else whether the byte (r->+244 >> 20) is 0 (3 when bit 0x8 of
// o->+12 is set)
DECOMP(004382D8, Obj_StateIs_004382D8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 28);
    if (p == 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x000D94CC, {p}));
    if (r == 0) {
        ret(ctx, 1u);
        return;
    }
    u32 v = (load<u32>(r + 244) >> 20) & 0xFFu;
    const u32 f = (load<u32>(o + 12) >> 3) & 1u;
    if (f != 0) v -= 3;
    ctx.gpr[4] = load<u32>(o + 12);
    ret(ctx, v == 0 ? 1u : 0u);
}
// T::T(T* o, a, b, u8 c): the 0x00B20CC8 base (o, 1), +12 / +16 / the u8
// +20 = a / b / c, vtable 0x00B213B8 (r3 left as the vtable)
DECOMP(0043A6DC, VtB213B8_Ctor) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    call(ctx, o24fn::Obj_InitVt_B20CC8, {o, 1});
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u8>(o + 20, static_cast<u8>(c));
    store<u32>(o, 0x00B213B8);
    ret(ctx, 0x00B213B8u);
}
// void f(T* o): 0x0044D1E8(+16) when +16 is set, else 0x0044D168(+12, the
// u8 +20) when +12 is set, else Emitters_StopAll(); then
// Flag_Set8_40000000(o)
DECOMP(0043A74C, Obj_Stop_0043A74C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 16);
    if (p != 0) {
        call(ctx, 0x0044D1E8, {p});
    } else {
        const u32 q = load<u32>(o + 12);
        if (q != 0) call(ctx, 0x0044D168, {q, load<u8>(o + 20)});
        else call(ctx, 0x00449950, {q});
    }
    call(ctx, o24fn::Flag_Set8_40000000, {o});
}
// void f(World* w, bool b): w->+0x1281F4->vfn[60](b == 0) when set
// (otherwise r3 is left as b)
DECOMP(004897AC, World_VCall60_004897AC) {
    const u32 w = arg<u32>(ctx, 0);
    const u64 b = ctx.gpr[4];
    const u32 p = load<u32>(w + 0x1281F4);
    ctx.gpr[3] = b;
    ctx.gpr[4] = p;
    ctx.gpr[5] = w;
    ctx.gpr[6] = 0x1281F4;
    if (p == 0) return;
    VCall(ctx, p, 60, {p, (b & 0xFFu) == 0 ? 1u : 0u});
}
// void f(T* o, bool b): bit 0x20000000 of +52 = b; then Game_SetPaused(!b,
// 0) when o->vfn[56]() returns true
DECOMP(00490F88, Obj_SetPausedBit_00490F88) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    store<u32>(o + 52, (load<u32>(o + 52) & ~0x20000000u) | ((b & 1u) << 29));
    if (VCall(ctx, o, 56, {o}) == 0) return;
    call(ctx, 0x00365A54, {b == 0 ? 1u : 0u, 0});
}
// void f(T* o, Effect* e): Effect_FindComponent(e, +12); then for
// c = Effect_Bind4BB7E0(e, +12, 0) of kind 11 ((c->+4 >> 10) & 63),
// c->+340 = o->+16
DECOMP(004C44BC, Effect_SetParam_004C44BC) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    call(ctx, 0x004BB4B4, {e, load<u32>(o + 12)});
    const u32 c = static_cast<u32>(call(ctx, 0x004BB7E0, {e, load<u32>(o + 12), 0}));
    if (c == 0) return;
    const u32 k = (load<u32>(c + 4) >> 10) & 63u;
    ctx.gpr[4] = k;
    if (k != 11) return;
    ctx.fpr[1] = F32Bits(Cp(c + 340, o + 16));
}
// const char* f(XmlCursor* o): String_Format(the format at 0x00AF0A8C,
// XmlCursor_DocPath(0x004F9434(o)), +8 + 1, +12 + 1)
DECOMP(004F61C4, Xml_Where_004F61C4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = static_cast<u32>(call(ctx, 0x004F9434, {o, 0x00AF0000}));
    const u32 d = static_cast<u32>(call(ctx, 0x004F8944, {a}));
    call(ctx, o24fn::String_Format, {0x00AF0A8C, d, Sx(load<u32>(o + 8) + 1), Sx(load<u32>(o + 12) + 1)});
}
// T::T(T* o, a, const u8* b): Ctor_Base4FBFA8(o, a), vtable 0x00B0DA04,
// +72..+75 = b[0..3] (byte copy; r3 / r4 left)
DECOMP(00736320, VtB0DA04_Ctor) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2);
    call(ctx, o24fn::Ctor_Base4FBFA8, {o, arg<u32>(ctx, 1)});
    store<u32>(o, 0x00B0DA04);
    u32 c = 0;
    for (u32 k = 0; k < 4; ++k) {
        c = load<u8>(b + k);
        store<u8>(o + 72 + k, static_cast<u8>(c));
    }
    ctx.gpr[4] = o + 72;
    ret(ctx, c);
}
// s32 f(const T* t, u16 key): binary search of the 8-byte entries at +8
// (the u16 +4 the count) by their u16 +0 >> 3; the index or -1
DECOMP(007604C8, Table_Find_007604C8) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 key = arg<u32>(ctx, 1) & 0xFFFFu;
    s32 hi = static_cast<s32>(load<u16>(t + 4)) - 1, lo = 0;
    if (hi >= 0) {
        ctx.gpr[3] = key;
        ctx.gpr[4] = t + 8;
        for (;;) {
            const s32 mid = (hi + lo) >> 1;
            const s32 v = static_cast<s32>((load<u16>(t + 8 + 8 * static_cast<u32>(mid)) >> 3) & 0x1FFFu);
            ctx.gpr[7] = Sx(static_cast<u32>(mid));
            ctx.gpr[8] = static_cast<u32>(v);
            if (v > static_cast<s32>(key)) hi = mid - 1;
            else if (v < static_cast<s32>(key)) lo = mid + 1;
            else {
                ctx.gpr[5] = Sx(static_cast<u32>(hi));
                ctx.gpr[6] = Sx(static_cast<u32>(lo));
                ret(ctx, mid);
                return;
            }
            if (hi < lo) break;
        }
    }
    ctx.gpr[5] = Sx(static_cast<u32>(hi));
    ctx.gpr[6] = Sx(static_cast<u32>(lo));
    ret(ctx, -1);
}
// bool f(const T* s, U* d): d->+0..+8 / +16..+28 / +32..+40 / the u16 +44 =
// s->+416..+424 / +432..+444 / +448..+456 / the u16 +460; returns bit
// 0x10000000 of s->+400 (f1 / r5 / r6 left)
DECOMP(00761518, Obj_CopyState_00761518) {
    const u32 s = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Cp(d, s + 416);
    Cp(d + 4, s + 420);
    Cp(d + 8, s + 424);
    Cp(d + 16, s + 432);
    Cp(d + 32, s + 448);
    Cp(d + 20, s + 436);
    Cp(d + 24, s + 440);
    Cp(d + 36, s + 452);
    ctx.fpr[1] = F32Bits(Cp(d + 28, s + 444));
    ctx.gpr[5] = Cp(d + 40, s + 456);
    const u16 h = load<u16>(s + 460);
    store<u16>(d + 44, h);
    ctx.gpr[6] = h;
    ret(ctx, (load<u32>(s + 400) & 0x10000000u) != 0 ? 1u : 0u);
}
// The shared body of 0x00776320 / 0x0077644C: p = Mem_Alloc(24, 0, 16),
// Node_Init(p, (s16)x, name) when allocated; returns p.
static void NewNode24(PPUContext& ctx, u32 name) {
    const u64 x = ctx.gpr[3];
    const u32 p = static_cast<u32>(call(ctx, o22fn::Mem_Alloc, {24, 0, 16, 0}));
    if (p == 0) {
        ctx.gpr[4] = x;
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x00391694, {p, static_cast<u64>(static_cast<s64>(static_cast<s16>(x))), name});
    ret(ctx, p);
}
// Node* f(s16 x): a 24-byte node named by the string at 0x00B1FBB1
DECOMP(00776320, Node_New_00776320) { NewNode24(ctx, 0x00B1FBB1); }
// Node* f(s16 x): a 24-byte node named by the string at 0x00B1FBBC
DECOMP(0077644C, Node_New_0077644C) { NewNode24(ctx, 0x00B1FBBC); }
// void f(Mat* d, const T* a): the 3x3 part of the 4x4 matrix a->+96 + 96
// copied to d (raw words; f1 / r4 left)
DECOMP(0077AD88, Mat_Copy3x3_0077AD88) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 s = load<u32>(arg<u32>(ctx, 1) + 96) + 96;
    u32 w = 0;
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) w = Cp(d + 16 * r + 4 * c, s + 16 * r + 4 * c);
    ctx.fpr[1] = F32Bits(w);
    ctx.gpr[4] = s;
}

// ---------------------------------------------------------------- sweep: 29 instructions
// void f(T* o, s32 key): removes every key from the s32 array at +3700 (the
// count +20, re-read after each removal): the last element moves into the
// freed slot, which is then checked again
DECOMP(0001B274, IntSet_Remove_0001B274) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 key = arg<u32>(ctx, 1);
    const u32 base = o + 3700;
    s32 n = load<s32>(o + 20);
    if (n <= 0) return;
    for (s32 i = 0; i < n; ++i) {
        const u32 slot = base + 4 * static_cast<u32>(i);
        if (load<u32>(slot) != key) continue;
        const s32 m = n - 1;
        store<s32>(o + 20, m);
        store<u32>(slot, load<u32>(base + 4 * static_cast<u32>(m)));
        n = load<s32>(o + 20);
        --i;
    }
}
// void f(T* o): Obj_Register4F7128(o, d) for the four descriptors at
// 0x00F94AB4, 0x00F94ACC, 0x00F94AE4 and 0x00F94AFC
DECOMP(00023F3C, Obj_Register4_00023F3C) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 4; ++k) call(ctx, 0x004F7128, {o, 0x00F94AB4u + 24 * k});
}
// void f(T* o): for h = Handle_ResolveBit1(+12) (when set):
// Obj_SetField180_003676DC(h, +16) when +16 is set, then
// Prompt_Fade367758(h; +20)
DECOMP(0007D7C8, Handle_SetAndFade_0007D7C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, 0x003568E0, {load<u32>(o + 12)}));
    if (h == 0) return;
    const u32 v = load<u32>(o + 16);
    if (v != 0) call(ctx, 0x003676DC, {h, v});
    callf(ctx, 0x00367758, {h}, {LfsAt(o + 20)});
}
// void f(T* o): when +28 is not set, +12 is set and the node +24 is gone
// (Ptr_DerefNonZero == 0): RefNode_Release(+24), +24 =
// Node_PushF65B7C(+12, +16)
DECOMP(00083880, Obj_Rebind_00083880) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 28);
    ctx.gpr[3] = a;
    if (a != 0) return;
    const u32 b = load<u32>(o + 12);
    ctx.gpr[3] = b;
    if (b == 0) return;
    if (call(ctx, 0x00446960, {load<u32>(o + 24)}) != 0) return;
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 24)});
    const u32 n = static_cast<u32>(call(ctx, 0x00446984, {load<u32>(o + 12), load<u32>(o + 16)}));
    store<u32>(o + 24, n);
}
// void f(T* o): when +12 is set, 0x000AE2BC(the 4056-byte world slot (+80)
// picked by the u8 world->+4208 of the 0x20C80-byte block +16, +12, the u8
// +20)
DECOMP(0008CDCC, World_SlotCall_0008CDCC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    ctx.gpr[4] = a;
    if (a == 0) return;
    const u32 w = load<u32>(kWorldPtr);
    const u32 b = load<u8>(w + load<u32>(o + 16) * 0x20C80u + 4208);
    call(ctx, 0x000AE2BC, {w + b * 4056 + 80, a, load<u8>(o + 20)});
}
// void f(T* o): 0x0042C138(a new VtB21130 (20 bytes, SmallAlloc) made from
// +12, or null)
DECOMP(000A2E14, Obj_PushNewB21130_000A2E14) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {20}));
    u32 r = 0;
    if (p != 0) {
        call(ctx, 0x00434CA8, {p, load<u32>(o + 12)});
        r = p;
    }
    call(ctx, 0x0042C138, {r});
}
// void f(T* o): 0x0042C138(a new Ctor_B21220 (16 bytes, SmallAlloc) made
// from the f32 +12, or null)
DECOMP(000A43C4, Obj_PushNewB21220_000A43C4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {16}));
    u32 r = 0;
    if (p != 0) {
        callf(ctx, 0x00438498, {p}, {LfsAt(o + 12)});
        r = p;
    }
    call(ctx, 0x0042C138, {r});
}
// The body of 0x000F1D0C / 0x000F1D80: void f(T* o, U* a): when the state
// +400 is 2, 0x000F0E48(o, a, +388) and 0x000EFFE4(o, a, +548); otherwise
// the u8 a->+24->+15 |= 0x8.
static void ObjDispatchAt(PPUContext& ctx, u32 o, u32 a) {
    if (load<u32>(o + 400) != 2) {
        const u32 q = load<u32>(a + 24);
        const u32 b = load<u8>(q + 15) | 0x8u;
        store<u8>(q + 15, static_cast<u8>(b));
        ctx.gpr[4] = b;
        ret(ctx, q);
        return;
    }
    call(ctx, 0x000F0E48, {o, a, load<u32>(o + 388)});
    call(ctx, 0x000EFFE4, {o, a, load<u32>(o + 548)});
}
// void f(T* o, U* a): see ObjDispatchAt
DECOMP(000F1D0C, Obj_Dispatch_000F1D0C) { ObjDispatchAt(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1)); }
// The body of VtB13528_Dtor for an adjusted this.
static void VtB13528DtorAt(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B13528);
    store<u32>(o + 380, 0x00B136C8);
    store<u32>(o + 384, 0x00B136F0);
    call(ctx, 0x004A0E08, {o + 680});
    call(ctx, 0x004A0E08, {o + 644});
    call(ctx, 0x000EB0F8, {o});
}
// VtB13528_Dtor called through the interface 384 bytes in
DECOMP(000F50EC, VtB13528_DtorAdj384) { VtB13528DtorAt(ctx, arg<u32>(ctx, 0) - 384); }
// VtB13528_Dtor called through the interface 380 bytes in
DECOMP(000F5160, VtB13528_DtorAdj380) { VtB13528DtorAt(ctx, arg<u32>(ctx, 0) - 380); }
// void f(T* o): when world->+0x1281E0 is set and Obj_HasWord368_000FC5DC
// (it, +1184): 0x001071CC(+1184, o + 1200) and +1184 = 0. Returns 0.
DECOMP(0013AE28, Obj_Drop1184_0013AE28) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(load<u32>(kWorldPtr) + 0x1281E0);
    ctx.gpr[3] = p;
    if (p == 0) return;
    if (call(ctx, 0x000FC5DC, {p, load<u32>(o + 1184)}) == 0) return;
    call(ctx, 0x001071CC, {load<u32>(o + 1184), o + 1200});
    store<u32>(o + 1184, 0);
    ret(ctx, 0u);
}
// The shared body of the VtB126B8 deleting destructors (0x001681E4 /
// 0x00759038): vtables 0x00B126B8 / 0x00B1200C (+380) / 0x00B12528 (+384),
// VtB12550_Dtor, SmallFree(o, size). Null does nothing.
static void VtB126B8Delete(PPUContext& ctx, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o});
    call(ctx, 0x00552420, {o, size});
}
// T::~T deleting form (480 bytes): see VtB126B8Delete
DECOMP(001681E4, VtB126B8_Delete480) { VtB126B8Delete(ctx, 480); }
// void f(T* o, Event* e): for the event 0xC6678E85 whose +12 is o->+464
// (set), o->+452 += 1; then 0x000E5E44(o, e)
DECOMP(001AC060, Obj_CountEvent_001AC060) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    if (load<u32>(e + 8) == 0xC6678E85u) {
        const u32 v = load<u32>(o + 464);
        if (v != 0 && v == load<u32>(e + 12)) store<u32>(o + 452, load<u32>(o + 452) + 1);
    }
    call(ctx, 0x000E5E44, {o, e, e});
}
// void f(T* o, U* d): 0x000DFBFC(o, d), then d->+208..+216 = o->+240..+248,
// d->+224 = o->+256 and the u8 d->+232..+234 = o->+264..+266
DECOMP(001BDFE8, Obj_CopyOut_001BDFE8) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {o, d});
    Cp(d + 224, o + 256);
    store<u8>(d + 232, load<u8>(o + 264));
    Cp(d + 208, o + 240);
    Cp(d + 212, o + 244);
    store<u8>(d + 233, load<u8>(o + 265));
    ctx.fpr[1] = F32Bits(Cp(d + 216, o + 248));
    const u8 b = load<u8>(o + 266);
    store<u8>(d + 234, b);
    ret(ctx, static_cast<u32>(b));
}
// u32 f(T* o, a, s32 b, c): 0x000E7FF0(o, a, b, c) when +392 >> 1 is 1 and
// b is 3, else 0
DECOMP(001BE858, Obj_CallIfState_001BE858) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 s = load<u32>(o + 392) >> 1;
    ctx.gpr[7] = s;
    if (s == 1 && b == 3) {
        call(ctx, 0x000E7FF0, {o, a, b, c, b, c});
        return;
    }
    ctx.gpr[4] = b;
    ctx.gpr[5] = a;
    ctx.gpr[6] = o;
    ret(ctx, 0u);
}
// u32 f(T* o, const s32* p): 0x001F69B0(o, p) when *p is 2, else 1
DECOMP(001F70B4, Obj_Handle2_001F70B4) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const s32 v = load<s32>(p);
    if (v == 2) {
        call(ctx, 0x001F69B0, {o, p, p});
        return;
    }
    ctx.gpr[4] = o;
    ctx.gpr[5] = static_cast<u32>(v);
    ret(ctx, 1u);
}
// The shared body of 0x00219E10 / 0x00221DD0: bool f(T* o, U* p): p set,
// p->+off has a bit of mask and fn(o, p) returns true.
static void TestFlagCall(PPUContext& ctx, u32 off, u32 mask, u32 fn) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    ctx.gpr[4] = o;
    ctx.gpr[5] = p;
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 f = load<u32>(p + off) & mask;
    ctx.gpr[5] = f;
    if (f == 0) {
        ret(ctx, 0u);
        return;
    }
    ret(ctx, call(ctx, fn, {o, p, p}) != 0 ? 1u : 0u);
}
// bool f(T* o, U* p): p->+192 bit 0x8 and 0x0020E7A4(o, p)
DECOMP(00219E10, Obj_TestAndCall_00219E10) { TestFlagCall(ctx, 192, 0x8u, 0x0020E7A4); }
// bool f(T* o, U* p): p->+196 bit 0x20000 and 0x0021F9E8(o, p)
DECOMP(00221DD0, Obj_TestAndCall_00221DD0) { TestFlagCall(ctx, 196, 0x20000u, 0x0021F9E8); }
// bool f(const T* o): q = o->+612; 0 when q or q->+28 is not set; else
// whether none of the 224-byte entries q->+72 (the s8 q->+28->+32 of them,
// an unsigned count) has +36 set
DECOMP(00235D48, Obj_AllIdle_00235D48) {
    const u32 q = load<u32>(arg<u32>(ctx, 0) + 612);
    if (q == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 c = load<u32>(q + 28);
    ctx.gpr[6] = c;
    if (c == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 n = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(c + 32))));
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = n;
    if (n != 0) {
        u32 p = load<u32>(q + 72);
        for (u32 i = 0;;) {
            const u32 x = load<u32>(p + 36);
            ctx.gpr[7] = x;
            if (x != 0) {
                ret(ctx, 0u);
                return;
            }
            ++i;
            p += 224;
            ctx.gpr[5] = i;
            if (!(n > i)) break;
        }
    }
    ret(ctx, 1u);
}
// The shared body of 0x00243F4C / 0x002440BC / 0x00244130: d = the Vec3 at
// o->+396 + off when +396 is set, else the fallback (f1 left as the last
// word).
static void CopyVecOr(PPUContext& ctx, u32 off, u32 fallback) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 p = load<u32>(arg<u32>(ctx, 1) + 396);
    const u32 s = p != 0 ? p + off : fallback;
    Cp(d, s);
    Cp(d + 4, s + 4);
    ctx.fpr[1] = F32Bits(Cp(d + 8, s + 8));
    if (p != 0) {  // r3..r7 left as the original's walking pointers
        ctx.gpr[3] = d + 8;
        ctx.gpr[4] = s + 8;
        ctx.gpr[5] = d + 4;
        ctx.gpr[6] = s + 4;
        ctx.gpr[7] = d + 8;
    }
}
// void f(Vec3* d, const T* o): o->+396->+320, else o->+144
DECOMP(00243F4C, Obj_GetVec320_00243F4C) { CopyVecOr(ctx, 320, arg<u32>(ctx, 1) + 144); }
// void f(Vec3* d, const T* o): o->+396->+432, else the Vec3 at 0x01164B40
DECOMP(002440BC, Obj_GetVec432_002440BC) { CopyVecOr(ctx, 432, 0x01164B40); }
// void f(Vec3* d, const T* o): o->+396->+448, else the Vec3 at 0x01164B40
DECOMP(00244130, Obj_GetVec448_00244130) { CopyVecOr(ctx, 448, 0x01164B40); }
// void f(T* o, bool b): the u8 +16 = b; when b is 0, o->vfn[16]() and bits
// 0x70000000 of +416 put back as they were before the call
DECOMP(0026D7B0, Obj_SetOn_0026D7B0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    store<u8>(o + 16, static_cast<u8>(b));
    if (b != 0) {
        ctx.gpr[4] = b;
        return;
    }
    const u32 save = load<u32>(o + 416);
    VCall(ctx, o, 16, {o});
    const u32 w = (load<u32>(o + 416) & ~0x70000000u) | (save & 0x70000000u);
    store<u32>(o + 416, w);
    ret(ctx, w);
}
// f32 f(const T* o): by the state (+416 >> 28) & 7: 1, 4, 5 -> k0; else
// +484 - +292 when +272 has bit 0x80000000, else k1 when +420 > k0, else
// k2 (k0..k2 the constants at 0x0026D874)
DECOMP(0026D880, Obj_StateValue_0026D880) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 k = 0x0026D874;
    ctx.gpr[4] = k;
    const u32 s = (load<u32>(o + 416) >> 28) & 7u;
    ctx.gpr[5] = s;
    if (s == 1 || s == 4 || s == 5) {
        ctx.fpr[1] = LfsAt(k);
        return;
    }
    const u32 f = load<u32>(o + 272) >> 31;
    ctx.gpr[5] = f;
    if (f != 0) {
        const f64 b = LfsAt(o + 292);
        ctx.fpr[2] = b;
        ctx.fpr[1] = static_cast<f32>(LfsAt(o + 484) - b);
        return;
    }
    const f64 k0 = LfsAt(k), v = LfsAt(o + 420);
    ctx.fpr[2] = v;
    ctx.fpr[1] = v > k0 ? LfsAt(k + 4) : LfsAt(k + 8);
}
// T::~T deleting form: vtable 0x00B1C020, SortedMap_Remove75DC84(the map
// at 0x0105752C, o + 36), RefNode_Release(+280), Mem_FreeFrom(o, 0). Null
// does nothing.
DECOMP(002725E8, VtB1C020_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1C020);
    call(ctx, 0x0075DC84, {0x0105752C, o + 36});
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 280)});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}
// void f(T* o, a, ?, key; f32 t): for c = Obj_FindComp34CC34(o->+36, key)
// (when +36 is set and c found), 0x00352F70(c, ?, a; t)
DECOMP(00298AC8, Anim_Apply_00298AC8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), key = arg<u32>(ctx, 3);
    const f64 t = farg(ctx, 0);
    const u32 q = load<u32>(o + 36);
    ctx.gpr[3] = key;
    if (q == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x0034CC34, {q, key}));
    if (c == 0) return;
    callf(ctx, 0x00352F70, {c, ctx.gpr[4], a}, {t});  // r4 as the call left it
}
// bool f(const T* o): 0 unless the kind +16 is 8; then 1 when o->+8 or its
// +52 is not set, else Effect_Check4CF620(+52, 0) != 0
DECOMP(002FB460, Obj_EffectDone_002FB460) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 16);
    ctx.gpr[4] = k;
    if (k != 8) {
        ret(ctx, 0u);
        return;
    }
    const u32 p = load<u32>(o + 8);
    ctx.gpr[4] = p;
    if (p == 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 e = load<u32>(p + 52);
    ctx.gpr[4] = e;
    if (e == 0) {
        ret(ctx, 1u);
        return;
    }
    ret(ctx, call(ctx, 0x004CF620, {e, 0}) != 0 ? 1u : 0u);
}
// T::~T: when +48 is set, vtable 0x00B1D730, +48->vfn[4]() and +48 = 0;
// then vtable 0x00B1D6A0 (r3 left as it)
DECOMP(002FD7C0, VtB1D730_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 48);
    ctx.gpr[4] = 0x00B1D730;
    if (p != 0) {
        const u32 vt = load<u32>(p);  // read before the store (p may be o)
        store<u32>(o, 0x00B1D730);
        VCallVt(ctx, vt, 4, {p});
        store<u32>(o + 48, 0);
    }
    store<u32>(o, 0x00B1D6A0);
    ret(ctx, 0x00B1D6A0u);
}
// void f(T* o, XmlCursor* c): the bool 0x64FDE756 into the u8 +40 and the
// hash 0x0024728B into +44 (the current values as defaults)
DECOMP(002FE510, Obj_ReadXml_002FE510) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, 0x004F986C, {c, 0x64FDE756, o + 40, load<u8>(o + 40)});
    call(ctx, 0x004FA14C, {c, 0x0024728B, o + 44, load<u32>(o + 44)});
}
// void f(T* o, XmlCursor* c): the hash 0x0024728B into +4 and the f32
// 0x79792193 into +8 (the current values as defaults)
DECOMP(00303328, Obj_ReadXml_00303328) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, 0x004FA14C, {c, 0x0024728B, o + 4, load<u32>(o + 4)});
    callf(ctx, 0x004F9B20, {c, 0x79792193, o + 8}, {LfsAt(o + 8)});
}
// void f(T* o; f32 x): when +32 is set, +32->+64 = x and bits 0x01000000 /
// 0x00800000 / 0x00400000 of +12 = x > k / x < k / x == k (k the constant
// at 0x003541DC; r4..r6 / f2 left)
DECOMP(003541E0, Obj_SetSign_003541E0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    const u32 q = load<u32>(o + 32);
    ctx.gpr[4] = q;
    if (q == 0) return;
    store<f32>(q + 64, static_cast<f32>(x));
    const f64 k = LfsAt(0x003541DC);
    const u32 gt = x > k ? 1u : 0u, lt = x < k ? 1u : 0u, eq = x == k ? 1u : 0u;
    const u32 w = (load<u32>(o + 12) & ~0x01C00000u) | (gt << 24) | (lt << 23) | (eq << 22);
    store<u32>(o + 12, w);
    ctx.fpr[2] = k;
    ctx.gpr[4] = w;
    ctx.gpr[5] = eq;
    ctx.gpr[6] = gt;
}
// void f(?, T** out, U* p): when p and p->+208 are set, *out = p and
// ResetStateFrom(p)
DECOMP(00364E70, Obj_ResetState_00364E70) {
    const u32 out = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    if (p == 0) return;
    const u32 a = load<u32>(p + 208);
    ctx.gpr[3] = a;
    if (a == 0) return;
    store<u32>(out, p);
    ResetStateFrom(ctx, p);
}
// u32 f(const T* o): the byte offset past two variable-size records: with
// i / j the u16 +146 / +144 and the 132-byte slots at +152, a = (12 *
// slot[i].u16 + 23) & ~15, b = (8 * the u16 at +152 + 132 * i + a + 21) &
// ~15; j * 132 + a + b + 152 (r4..r7 left)
DECOMP(0038BA6C, Rec_Offset_0038BA6C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 i = load<u16>(o + 146), j = load<u16>(o + 144);
    const u32 n = load<u16>(o + i * 132 + 152);
    const u32 a = (12 * n + 23) & ~15u;
    const u32 h = load<u16>(o + i * 132 + a + 152);
    const u32 b = ((h << 3) + 21) & ~15u;
    ctx.gpr[4] = j * 132 + a;
    ctx.gpr[5] = a;
    ctx.gpr[6] = j * 4;
    ctx.gpr[7] = 2 * n;
    ret(ctx, j * 132 + a + b + 152);
}
// void f(T* o, Event* e): see ObjOnEventA7DC
DECOMP(003AA9F0, Obj_OnEvent_003AA9F0) { ObjOnEventA7DC(ctx, arg<u32>(ctx, 0)); }
// void f(World* w, s32 i, a): p = the object at +8 of the player block i
// (0x20C80 bytes each, from +4208); 0x0012BC20(p, a) when p is set and its
// +192 has bit 0x8 (otherwise r3 is left as a)
DECOMP(00488004, World_PlayerCall_00488004) {
    const u32 w = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u64 a = ctx.gpr[5];
    const u32 p = load<u32>(w + i * 0x20C80u + 4208 + 8);
    ctx.gpr[3] = a;
    ctx.gpr[4] = p;
    if (p == 0) return;
    const u32 f = load<u32>(p + 192) & 0x8u;
    ctx.gpr[5] = f;
    if (f == 0) return;
    call(ctx, 0x0012BC20, {p, a});
}
// u32 f(Effect* e, u32 type): c = Effect_FindComponent(e, type); c->+36
// when c is found and c->vfn[32]() is 5, else 0
DECOMP(004CF2BC, Effect_GetComp36_004CF2BC) {
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {arg<u32>(ctx, 0), arg<u32>(ctx, 1)}));
    if (c != 0 && VCall(ctx, c, 32, {c}) == 5) {
        const u32 v = load<u32>(c + 36);
        if (v != 0) {
            ret(ctx, v);
            return;
        }
    }
    ret(ctx, 0u);
}
// T::~T deleting form: vtable 0x00B10698, RefNode_Release(+20), vtable
// 0x00B0F1D0, VtB11B20_DtorTrivial, Mem_FreeFrom(o, 0). Null does nothing.
DECOMP(0073EED8, VtB10698_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B10698);
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 20)});
    store<u32>(o, 0x00B0F1D0);
    call(ctx, 0x004BB41C, {o});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}
// The shared body of 0x00742DD8 / 0x00762CC0: PtrVector_Construct(v), the
// u16 +6 (the element size) = k, PtrVector_Init(v, a, b, (u8)c).
static void PtrVectorMake(PPUContext& ctx, u16 k) {
    const u32 v = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u64 c = ctx.gpr[6];
    call(ctx, 0x005516E0, {v, a, b, c});
    store<u16>(v + 6, k);
    call(ctx, 0x00551598, {v, a, b, c & 0xFFu});
}
// void f(PtrVector* v, a, b, u8 c): a vector of 76-byte elements
DECOMP(00742DD8, PtrVector_Make76_00742DD8) { PtrVectorMake(ctx, 76); }
// PtrVector* f(PtrVector** pp): *pp, made first when null (20 bytes,
// SmallAlloc; PtrVector_Construct, element size 4)
DECOMP(0074E818, PtrVector_GetOrNew_0074E818) {
    const u32 pp = arg<u32>(ctx, 0);
    u32 p = load<u32>(pp);
    if (p == 0) {
        p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {20}));
        store<u32>(pp, p);
        if (p != 0) {
            call(ctx, 0x005516E0, {p});
            store<u16>(p + 6, 4);
            p = load<u32>(pp);
        }
    }
    ret(ctx, p);
}
// void f(Quat* d, const Quat* q): d = the inverse of q (conjugate over the
// squared length; fused sums; f2..f7 left)
DECOMP(00757E38, Quat_Inverse_00757E38) {
    const u32 d = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    const f64 x = LfsAt(q), y = LfsAt(q + 4), z = LfsAt(q + 8), w = LfsAt(q + 12);
    f64 n = static_cast<f32>(y * y);
    n = static_cast<f32>(std::fma(x, x, n));
    n = static_cast<f32>(std::fma(z, z, n));
    n = static_cast<f32>(std::fma(w, w, n));
    const f64 a = static_cast<f32>(-x / n), b = static_cast<f32>(-y / n), c = static_cast<f32>(-z / n), e = static_cast<f32>(w / n);
    store<f32>(d, static_cast<f32>(a));
    store<f32>(d + 4, static_cast<f32>(b));
    store<f32>(d + 8, static_cast<f32>(c));
    store<f32>(d + 12, static_cast<f32>(e));
    ctx.fpr[1] = e;
    ctx.fpr[2] = a;
    ctx.fpr[3] = b;
    ctx.fpr[4] = c;
    ctx.fpr[5] = -x;
    ctx.fpr[6] = -z;
    ctx.fpr[7] = w;
    ctx.gpr[5] = q + 8;
    ctx.gpr[6] = q + 12;
}
// T::~T deleting form (464 bytes): see VtB126B8Delete
DECOMP(00759038, VtB126B8_Delete464) { VtB126B8Delete(ctx, 464); }
// void f(Mat* d, const Mat* s): the 3x3 part and the word +48 copied (raw;
// f1 / r5..r8 left)
DECOMP(00760BE0, Mat_Copy3x3W_00760BE0) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    for (u32 r = 0; r < 3; ++r)
        for (u32 c = 0; c < 3; ++c) Cp(d + 16 * r + 4 * c, s + 16 * r + 4 * c);
    ctx.fpr[1] = F32Bits(Cp(d + 48, s + 48));
    ctx.gpr[5] = s + 16;
    ctx.gpr[6] = d + 16;
    ctx.gpr[7] = s + 32;
    ctx.gpr[8] = d + 32;
}
// void f(PtrVector* v, a, b, u8 c): a vector of 4-byte elements
DECOMP(00762CC0, PtrVector_Make4_00762CC0) { PtrVectorMake(ctx, 4); }
// Obj_VCall16Rel_007713C0 called through the interface 12 bytes in
DECOMP(0077142C, Obj_VCall16RelAdj12_0077142C) {
    const u32 o = arg<u32>(ctx, 0) - 12;
    const u32 q = load<u32>(arg<u32>(ctx, 1) + 4);
    ctx.gpr[3] = o;
    if (static_cast<s8>(load<u8>(q + 24)) != 1) return;
    const u32 p = q + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(q + 16))));
    if (p == 0) return;
    VCall(ctx, o, 16, {o, p});
}
// void f(T* o, const Mat* m): +80..+136 = the 4x3 part of m (rows of 16
// bytes; raw), bit 0x800 of +164 cleared (f1 / r5 / r6 left)
DECOMP(0077CE28, Obj_SetMat_0077CE28) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 r = 0; r < 3; ++r)
        for (u32 c = 0; c < 3; ++c) Cp(o + 80 + 16 * r + 4 * c, m + 16 * r + 4 * c);
    const u32 w = load<u32>(o + 164) & ~0x800u;
    Cp(o + 128, m + 48);
    Cp(o + 132, m + 52);
    store<u32>(o + 164, w);
    ctx.fpr[1] = F32Bits(Cp(o + 136, m + 56));
    ctx.gpr[5] = 0;
    ctx.gpr[6] = w;
}
// T* f(): a new 56-byte T (SmallAlloc): +4..+24 = 0, the u8 +28 = 0, +32 =
// 0, vtable 0x00B218D8; null when the allocation fails (r4 / r5 left)
DECOMP(00788734, VtB218D8_New) {
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {56}));
    ctx.gpr[4] = 0;
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    for (u32 off = 4; off <= 24; off += 4) store<u32>(p + off, 0);
    store<u8>(p + 28, 0);
    store<u32>(p + 32, 0);
    store<u32>(p, 0x00B218D8);
    ctx.gpr[4] = p;
    ctx.gpr[5] = 0x00B218D8;
    ret(ctx, p);
}

// ---------------------------------------------------------------- sweep: 30 instructions
// The pattern of the script-command builders below: p = SmallAlloc(size),
// make(p) when allocated, then 0x0042C138(p or null) (the command list push,
// name?).
template <typename F>
static void PushNew(PPUContext& ctx, u32 size, F make) {
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {size}));
    u32 r = 0;
    if (p != 0) {
        make(p);
        r = p;
    }
    call(ctx, 0x0042C138, {r});
}
// void f(T* o, bool a, u8 b): WorldObj_SetDrawCallback(o->+160, 0, cb, a ?
// o : 0, b) with cb the OPD 0x00F2D8E8 when a, else 0 (when +160 is set)
DECOMP(0001CC30, Obj_SetDrawCb_0001CC30) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    const u32 q = load<u32>(o + 160);
    ctx.gpr[5] = q;
    if (q == 0) return;
    if (a == 0) call(ctx, 0x00583C04, {q, 0, 0, 0, b});
    else call(ctx, 0x00583C04, {q, 0, 0x00F2D8E8, o, b});
}
// void f(T* o, s32 k): bit 0x10 of the u8 +305 set for k 0 / 1, cleared for
// k 2 / 3; then +348 = k
DECOMP(00024048, Obj_SetMode_00024048) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = arg<u32>(ctx, 1);
    const s32 ks = static_cast<s32>(k);
    ctx.gpr[5] = k;
    if (ks == 0 || ks == 1) {
        const u32 b = load<u8>(o + 305) | 0x10u;
        store<u8>(o + 305, static_cast<u8>(b));
        ctx.gpr[5] = b;
    } else if (ks == 2 || ks == 3) {
        const u32 b = load<u8>(o + 305) & ~0x10u;
        store<u8>(o + 305, static_cast<u8>(b));
        ctx.gpr[5] = 0;
        ctx.gpr[6] = b;
    }
    store<u32>(o + 348, k);
}
// void f(T* o): unless the u8 0x00F9BC28 is set, 0x0048F90C(o->+48,
// 0xCB186A06, 0x752CA457, 0; the f32 table o->+308->+12 indexed by the s32
// at (*0x00F951CC)->+64)
DECOMP(00035D44, Obj_SendValue_00035D44) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f = load<u8>(0x00F9BC28);
    ctx.gpr[4] = f;
    if (f != 0) return;
    const u32 t = load<u32>(load<u32>(o + 308) + 12);
    const u32 i4 = load<u32>(load<u32>(0x00F951CC) + 64) << 2;
    callf(ctx, 0x0048F90C, {load<u32>(o + 48), 0xCB186A06u, 0x752CA457u, 0, i4, t}, {LfsAt(t + i4)});
}
// void f(T* o): when world->+0x1281E0 is set, has bit 0x40000 in +172 and
// o->+12 is set: 0x000FCE18(it, +12, the u8 +20, o, the u8 +21; +16)
DECOMP(0006EF5C, Obj_WorldCall_0006EF5C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(load<u32>(kWorldPtr) + 0x1281E0);
    ctx.gpr[4] = p;
    if (p == 0) return;
    const u32 f = load<u32>(p + 172) & 0x40000u;
    ctx.gpr[5] = f;
    if (f == 0) return;
    const u32 a = load<u32>(o + 12);
    ctx.gpr[5] = a;
    if (a == 0) return;
    callf(ctx, 0x000FCE18, {p, a, load<u8>(o + 20), o, load<u8>(o + 21)}, {LfsAt(o + 16)});
}
// void f(T* o): pushes a new VtB20D40 (20 bytes) made from (f32 +12, the
// u8 +16)
DECOMP(0009B2A4, Cmd_PushB20D40_0009B2A4) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew(ctx, 20, [&](u32 p) { callf(ctx, 0x0042D464, {p, ctx.gpr[4], load<u8>(o + 16)}, {LfsAt(o + 12)}); });
}
// void f(T* o): pushes a new VtB20E00 (12 bytes) made from the u8 +12, +13
DECOMP(0009C53C, Cmd_PushB20E00_0009C53C) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew(ctx, 12, [&](u32 p) { call(ctx, 0x0042E3AC, {p, load<u8>(o + 12), load<u8>(o + 13)}); });
}
// void f(T* o): pushes a new Obj_InitVt_B20E48 object (12 bytes) when the
// u8 +12 is set; then Global_StepF6585C()
DECOMP(0009CB40, Cmd_PushB20E48_0009CB40) {
    const u32 b = load<u8>(arg<u32>(ctx, 0) + 12);
    ctx.gpr[3] = b;
    if (b != 0) PushNew(ctx, 12, [&](u32 p) { call(ctx, 0x0042E9F0, {p}); });
    call(ctx, 0x0042BC24, {ctx.gpr[3]});
}
// void f(T* o): pushes a new VtB20E78 (20 bytes) made from (+12, the u8 +16)
DECOMP(0009CEBC, Cmd_PushB20E78_0009CEBC) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew(ctx, 20, [&](u32 p) { call(ctx, 0x0042EBC0, {p, load<u32>(o + 12), load<u8>(o + 16)}); });
}
// void f(T* o): pushes a new VtB21298 (20 bytes) made from (+12, +16)
DECOMP(000A4ED4, Cmd_PushB21298_000A4ED4) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew(ctx, 20, [&](u32 p) { call(ctx, 0x004388A0, {p, load<u32>(o + 12), load<u32>(o + 16)}); });
}
// void f(T* o): pushes a new VtB212B0 (20 bytes) made from the u8 +12, +13
DECOMP(000A506C, Cmd_PushB212B0_000A506C) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew(ctx, 20, [&](u32 p) { call(ctx, 0x004389D4, {p, load<u8>(o + 12), load<u8>(o + 13)}); });
}
// void f(T* o): pushes a new VtB21418 (20 bytes) made from (+12, +16)
DECOMP(000A7598, Cmd_PushB21418_000A7598) {
    const u32 o = arg<u32>(ctx, 0);
    PushNew(ctx, 20, [&](u32 p) { call(ctx, 0x0043AD8C, {p, load<u32>(o + 12), load<u32>(o + 16)}); });
}
// void f(T* o, Event* e): o->vfn[376]() for the event 0xC39AF658, then
// 0x000E1060(o, e)
DECOMP(000E5E44, Obj_OnEvent_000E5E44) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    if (load<u32>(e + 8) == 0xC39AF658u) VCall(ctx, o, 376, {o});
    call(ctx, 0x000E1060, {o, e});
}
// void f(T* o, U* d): 0x000DFBFC(o, d), then d->+208..+216 = o->+240..+248,
// d->+224..+232 = o->+256..+264, d->+240 / +244 = o->+272 / +276
DECOMP(000EB080, Obj_CopyOut_000EB080) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {o, d});
    for (u32 k = 0; k < 3; ++k) Cp(d + 208 + 4 * k, o + 240 + 4 * k);
    Cp(d + 224, o + 256);
    Cp(d + 228, o + 260);
    ctx.fpr[1] = F32Bits(Cp(d + 232, o + 264));
    ctx.gpr[3] = Cp(d + 240, o + 272);
    ctx.gpr[4] = Cp(d + 244, o + 276);
}
// Obj_Dispatch_000F1D0C called through the interface 380 bytes in
DECOMP(000F1D80, Obj_DispatchAdj380_000F1D80) { ObjDispatchAt(ctx, arg<u32>(ctx, 0) - 380, arg<u32>(ctx, 1)); }
// void f(u32 key, u32* a, u32* b): *a / *b = the pair found under key in
// the sorted map at 0x0103217C (nothing for key 0 or no entry)
DECOMP(000F816C, Map_GetPair_000F816C) {
    const u32 key = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (key == 0) return;
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);
    const u32 r = static_cast<u32>(call(ctx, 0x00744F5C, {0x0103217C, frame.addr(), key, key}));
    const u32 e = load<u32>(r);
    ctx.gpr[3] = e;
    if (e == 0) return;
    store<u32>(a, load<u32>(e));
    ret(ctx, Cp(b, e + 4));
}
// void f(T* s, U* d): d->+680 = s->+8; on the effect s->+4: +136 bit 0x200
// set, bit 0x400 cleared, Effect_SetHash4CF988(it, 0x879F2BDA); s->+4 = 0
DECOMP(0016DC78, Effect_Handoff_0016DC78) {
    const u32 s = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    store<u32>(d + 680, load<u32>(s + 8));
    u32 e = load<u32>(s + 4);
    store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
    e = load<u32>(s + 4);
    store<u32>(e + 136, load<u32>(e + 136) & ~0x400u);
    call(ctx, 0x004CF988, {load<u32>(s + 4), 0x879F2BDA});
    store<u32>(s + 4, 0);
}
// void f(T* o, U* d): Thunk_000E44C0(o, d), then d->+208 / +212 / +216 =
// o->+240 / +244 / +248 and bits 0x80000000 / 0x40000000 of d->+220 from
// o->+252 (f1 / r3 / r4 left)
DECOMP(001B0654, Obj_CopyOut_001B0654) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {o, d});
    u32 w = load<u32>(d + 220);
    Cp(d + 208, o + 240);
    Cp(d + 212, o + 244);
    w = (w & 0x7FFFFFFFu) | (load<u32>(o + 252) & 0x80000000u);
    store<u32>(d + 220, w);
    const u32 f = load<u32>(o + 252);
    w = (w & ~0x40000000u) | (f & 0x40000000u);
    store<u32>(d + 220, w);
    ctx.fpr[1] = F32Bits(Cp(d + 216, o + 248));
    ctx.gpr[4] = f;
    ret(ctx, w);
}
// void f(T* o): only when the state 0x010426B0 is 3: waits
// (sys_timer_usleep 300 in a loop) for the busy word 0x01042500 to clear,
// then 0x001BBC40(o)
DECOMP(001BD168, Obj_WaitAndRun_001BD168) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 st = load<u32>(0x01042668 + 72);
    ctx.gpr[3] = st;
    if (st != 3) return;
    while (load<u32>(0x01042500) != 0) syscall(ctx, 141, {300});
    call(ctx, 0x001BBC40, {o});
}
// T::~T: vtables 0x00B174C0 / 0x00B17694 (+380) / 0x00B176BC (+384), the
// PtrVector at +556 destroyed, then the base destructor 0x001AE468
DECOMP(001D1D94, VtB174C0_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B174C0);
    store<u32>(o + 380, 0x00B17694);
    store<u32>(o + 384, 0x00B176BC);
    VecDtor(ctx, o + 556);
    call(ctx, 0x001AE468, {o});
}
// void f(T* o, U* p): when o->+704 is set: p null clears it (returns 0);
// else when the low 15 bits of +704->+208->+112 and p->+208->+112 agree,
// +704 = p and 0x001D0F68(o, o, p) (otherwise r3 is left as p)
DECOMP(001D3968, Obj_SetLink_001D3968) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 c = load<u32>(o + 704);
    ctx.gpr[3] = p;
    ctx.gpr[4] = o;
    ctx.gpr[5] = c;
    if (c == 0) return;
    if (p == 0) {
        store<u32>(o + 704, 0);
        ret(ctx, 0u);
        return;
    }
    const u32 x = (load<u32>(load<u32>(c + 208) + 112) ^ load<u32>(load<u32>(p + 208) + 112)) & 0x7FFFu;
    ctx.gpr[5] = x;
    if (x != 0) return;
    store<u32>(o + 704, p);
    call(ctx, 0x001D0F68, {o, o, p});
}
// bool f(const T* o): t = +820 set, +980 bit 0x10000000, t->+172 bit 0x100
// and Field_404Bit15_001BD4A0(t)
DECOMP(001F465C, Obj_Check_001F465C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(o + 820);
    if (t == 0 || (load<u32>(o + 980) & 0x10000000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 r = 0;
    if ((load<u32>(t + 172) & 0x100u) != 0 && call(ctx, 0x001BD4A0, {t}) != 0) r = 1;
    ret(ctx, r);
}
// void f(T* o, a): Emitter_Stop(+484, 1); then 0x0020EBC8(o, a) unless the
// state +428 is 0 or 3
DECOMP(0021167C, Obj_StopAndRun_0021167C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    call(ctx, o25fn::Emitter_Stop, {load<u32>(o + 484), 1});
    const u32 k = load<u32>(o + 428);
    ctx.gpr[3] = k;
    if (k == 0 || k == 3) return;
    call(ctx, 0x0020EBC8, {o, a});
}
// void f(T* o): when +216 is set, its +180 = MapGet_0044C404(+176) and +188
// = Sound_Find(+184); then +616 = +620 = 0 and 0x000D82BC(o, 0)
DECOMP(00214038, Obj_Resolve_00214038) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 216);
    if (p != 0) {
        store<u32>(p + 180, static_cast<u32>(call(ctx, 0x0044C404, {load<u32>(p + 176)})));
        store<u32>(p + 188, static_cast<u32>(call(ctx, 0x0044C43C, {load<u32>(p + 184)})));
    }
    store<u32>(o + 616, 0);
    store<u32>(o + 620, 0);
    call(ctx, 0x000D82BC, {o, 0});
}
// void f(T* o, s32 key, bool on): for the first of the 31 12-byte entries
// at 0x00F64A78 whose +0 is key (index i), bit i of the mask at +84 = !on
// (an off mask)
DECOMP(0027C914, Obj_SetKeyBit_0027C914) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 on = arg<u32>(ctx, 2) & 0xFFu;
    for (u32 i = 0; i < 31; ++i) {
        const u32 k = load<u32>(0x00F64A78 + 12 * i);
        ctx.gpr[9] = k;
        if (k != key) continue;
        const u32 w = o + ((i >> 3) & 0x1FFFFFFCu) + 84;
        const u32 bit = 1u << (i & 31);
        u32 v = load<u32>(w) & ~bit;
        store<u32>(w, v);
        if (on == 0) {
            v |= bit;
            store<u32>(w, v);
        }
        ctx.gpr[4] = v;
        ctx.gpr[5] = ~bit;
        ctx.gpr[6] = bit;
        ret(ctx, w);
        return;
    }
    ctx.gpr[6] = 31;
    ctx.gpr[8] = 372;
}
// void f(T* o, bool v, u8 b): when bit 0x20 of the u8 +2336 differs from v,
// it is set to v and o->vfn[48](v, b) (the vtable read before the store)
DECOMP(0029FA28, Obj_SetBit2336_0029FA28) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 v0 = ctx.gpr[4];
    const u32 v = static_cast<u32>(v0) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    const u32 byte = load<u8>(o + 2336);
    const u32 cur = (byte >> 5) & 1u;
    if (cur == v) {
        ctx.gpr[3] = v0;
        ctx.gpr[4] = o;
        ctx.gpr[5] = byte;
        ctx.gpr[6] = b;
        ctx.gpr[7] = v;
        ctx.gpr[8] = cur;
        return;
    }
    const u32 vt = load<u32>(o);
    store<u8>(o + 2336, static_cast<u8>((byte & ~0x20u) | ((v & 1u) << 5)));
    const u32 opd = load<u32>(vt + 48);
    call_ptr(ctx, opd, {o, v, b, load<u32>(opd), opd});  // r6 = the entry, r7 = the OPD here
}
// void f(T* o, bool b): for the object +424 (when set): Obj_SetAnim(it, 0,
// 1, ...; the f32 at 0x002C6CF8) with the anim set 0x01164CA4 when b, else
// Obj_Dispatch362(it, 1, 1, 0)
DECOMP(002C6D00, Obj_Animate_002C6D00) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 q = load<u32>(o + 424);
    ctx.gpr[3] = q;
    if (q == 0) return;
    if (b == 0) {
        call(ctx, 0x0036325C, {q, 1, 1, 0});
        return;
    }
    callf(ctx, 0x00363A3C, {q, 0, 1, 0x002C6CF8, 0, 0, 0x01164CA4}, {LfsAt(0x002C6CF8)});
}
// XmlCursor_ReadBool(c, 0x64FDE756, o + 40, the u8 +40): the shared first
// read of 0x002FD5C8 / 0x002FE198 / 0x002FE510
static void ReadBool40(PPUContext& ctx, u32 o, u32 c) { call(ctx, 0x004F986C, {c, 0x64FDE756, o + 40, load<u8>(o + 40)}); }
// void f(T* o, XmlCursor* c): the bool 0x64FDE756 into the u8 +40 and the
// f32 0xB7EA3D94 into +44 (the current values as defaults)
DECOMP(002FD5C8, Obj_ReadXml_002FD5C8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    ReadBool40(ctx, o, c);
    callf(ctx, 0x004F9B20, {c, 0xB7EA3D94u, o + 44}, {LfsAt(o + 44)});
}
// void f(T* o, XmlCursor* c): the bool 0x64FDE756 into the u8 +40 and the
// hash 0x9178C4CB into +44 (the current values as defaults)
DECOMP(002FE198, Obj_ReadXml_002FE198) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    ReadBool40(ctx, o, c);
    call(ctx, 0x004FA14C, {c, 0x9178C4CBu, o + 44, load<u32>(o + 44)});
}
// u32 f(const T* o, s32 key): the index of the first 8-byte entry of the
// list o->+48->+8 (+0 the array, the u16 +8 the count, re-read each step)
// whose +4 is key; 0 when there is none or key is 0
DECOMP(0031F8EC, List_IndexOf_0031F8EC) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    if (key == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 l = load<u32>(load<u32>(o + 48) + 8);
    ctx.gpr[5] = 0;
    for (u32 i = 0;; ++i) {
        const u32 n = l != 0 ? load<u16>(l + 8) : 0u;
        ctx.gpr[6] = i;
        ctx.gpr[7] = n;
        if (i >= n) break;
        const u32 k = load<u32>(load<u32>(l) + 8 * i + 4);
        ctx.gpr[7] = k;
        if (key == k) {
            ret(ctx, i);
            return;
        }
    }
    ret(ctx, 0u);
}
// void f(T* o): when the u8 +2812 is set, 0x0033B9F8 on each word of the
// array +2772 (the u16 +2780 the count; both re-read each step)
DECOMP(0033E218, Obj_ForEach_0033E218) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f = load<u8>(o + 2812);
    ctx.gpr[3] = f;
    if (f == 0) return;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 2780);
        ctx.gpr[3] = n;
        if (i >= n) break;
        call(ctx, 0x0033B9F8, {load<u32>(load<u32>(o + 2772) + 4 * i)});
    }
}
// s32 f(T* o): o->vfn[16](&r) fills a 16-byte result; -1 unless its first
// byte is set and +8 is set, else the s32 +8->+268
DECOMP(00347EA0, Obj_QueryId_00347EA0) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 opd = load<u32>(load<u32>(o) + 16);
    call_ptr(ctx, opd, {frame.addr(), o, opd, load<u32>(opd)});
    if (load<u8>(frame.addr()) == 0) {
        ret(ctx, -1);
        return;
    }
    const u32 p = load<u32>(o + 8);
    if (p == 0) {
        ret(ctx, -1);
        return;
    }
    ret(ctx, load<s32>(p + 268));  // ld +264 then extsw: the low word
}
// u32 f(s32 key): how many entries of the four 20-byte lists at 0x0106C2D4
// (+0 the array of objects, the u16 +8 the count, re-read each step) have
// ->+8->+84 equal to key
DECOMP(00367134, Lists_CountKey_00367134) {
    const u32 key = arg<u32>(ctx, 0);
    u32 n = 0;
    for (u32 j = 0; j < 4; ++j) {
        const u32 l = 0x0106C2D4u + 20 * j;
        for (u32 i = 0; i < load<u16>(l + 8); ++i)
            if (load<u32>(load<u32>(load<u32>(load<u32>(l) + 4 * i) + 8) + 84) == key) ++n;
    }
    ctx.gpr[4] = 0x0106C2D4u + 80;
    ctx.gpr[9] = 4;
    ret(ctx, n);
}
// The shared body of 0x003AD168 / 0x003AD1EC: bool f(const T* o): 0 when
// the u8 +flag is 0; 1 when +168 >= the f32 at k; else Effect_Spawn(fx, 0,
// 0, 1) and 0.
static void ReadyOrSpawn(PPUContext& ctx, u32 flag, u32 k, u32 fx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f = load<u8>(o + flag);
    ctx.gpr[4] = f;
    if (f == 0) {
        ret(ctx, 0u);
        return;
    }
    const f64 v = LfsAt(o + 168), lim = LfsAt(k);
    ctx.gpr[4] = k - 72;
    ctx.fpr[1] = v;
    ctx.fpr[2] = lim;
    if (v > lim || v == lim) {
        ret(ctx, 1u);
        return;
    }
    call(ctx, 0x004A5FC8, {fx, 0, 0, 1});
    ret(ctx, 0u);
}
// bool f(const T* o): the u8 +156, the limit at 0x01086528, effect 0x2BD9DDB4
DECOMP(003AD168, Obj_ReadyOrSpawn_003AD168) { ReadyOrSpawn(ctx, 156, 0x01086528, 0x2BD9DDB4); }
// bool f(const T* o): the u8 +154, the limit at 0x010864D8, effect 0x5E77B5FF
DECOMP(003AD1EC, Obj_ReadyOrSpawn_003AD1EC) { ReadyOrSpawn(ctx, 154, 0x010864D8, 0x5E77B5FF); }
// u32 f(const T* o): with b = o->+48->+12 a table of 4-byte slots indexed
// by (*0x010AB0F8)->+64 and (*0x010AB128)->+64: 0 when the first slot's
// byte is 0; else +8->+40 when the second's byte is set and that is set;
// else +60
DECOMP(00451E08, Obj_Pick_00451E08) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(load<u32>(o + 48) + 12);
    const u32 i = load<u32>(load<u32>(0x010AB0F8) + 64) << 2;
    ctx.gpr[4] = b;
    if (load<u8>(b + i) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 j = load<u32>(load<u32>(0x010AB128) + 64) << 2;
    const u32 c = load<u8>(b + j);
    ctx.gpr[4] = c;
    if (c != 0) {
        const u32 p = load<u32>(o + 8);
        ctx.gpr[4] = p;
        if (p != 0) {
            const u32 q = load<u32>(p + 40);
            ctx.gpr[4] = q;
            if (q != 0) {
                ret(ctx, q);
                return;
            }
        }
    }
    ret(ctx, load<u32>(o + 60));
}
// bool f(u64* id, u64* kb): *id = Thread_StartOnce_000AF27C() (retried
// until it is not negative), *kb = Game_HddFreeSizeKB(); returns 1
DECOMP(0049AF80, Sys_Start_0049AF80) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    u64 r;
    do r = call(ctx, 0x000AF27C); while (static_cast<s32>(r) < 0);
    store<u64>(a, r);
    store<u64>(b, call(ctx, 0x005721B4));
    ret(ctx, 1u);
}
// void f(T* o, Effect* e, bool v): sets bit 0x10 of the u8 +42 of
// e->+208->+112 to v for e of kind 10 ((+4 >> 10) & 63); when the u8 o->+17
// is set, continues with e->+272 first, then along +268
DECOMP(004CC68C, Effect_SetFlagTree_004CC68C) {
    const u32 o = arg<u32>(ctx, 0);
    u32 e = arg<u32>(ctx, 1);
    const u32 v = arg<u32>(ctx, 2);
    u32 first = 0;
    while (e != 0) {
        if (((load<u32>(e + 4) >> 10) & 63u) == 10) {
            const u32 p = load<u32>(e + 208);
            if (p != 0) {
                const u32 q = load<u32>(p + 112);
                if (q != 0) store<u8>(q + 42, static_cast<u8>((load<u8>(q + 42) & ~0x10u) | ((v & 1u) << 4)));
            }
        }
        if (load<u8>(o + 17) == 0) break;
        if (first == 0) {
            first = 1;
            e = load<u32>(e + 272);
        } else {
            e = load<u32>(e + 268);
        }
    }
    ctx.gpr[4] = first;
}
// T::~T deleting form: vtable 0x00B225B0, Mem_FreeFrom(+304, 0), vtable
// 0x00B22598, ApeRoot_Destruct, Mem_FreeFrom(o, 0). Null does nothing.
DECOMP(004D2354, VtB225B0_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 p = load<u32>(o + 304);
    store<u32>(o, 0x00B225B0);
    call(ctx, o25fn::Mem_FreeFrom, {p, 0});
    store<u32>(o, 0x00B22598);
    call(ctx, 0x00558CE8, {o});
    call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}
// f32 f(f32 x, s32 n): x to the power n by squaring (single rounding each
// step; 1 / r for negative n, the double at 0x00742D58 (+inf) when r is 0; the
// constants 1.0f / 0.0f at 0x00742D50 / 0x00742D54)
DECOMP(00742D60, Math_PowI_00742D60) {
    f64 x = farg(ctx, 0);
    const u32 n = arg<u32>(ctx, 1);
    const bool neg = static_cast<s32>(n) < 0;
    u32 m = neg ? 0u - n : n;
    const f64 one = LfsAt(0x00742D50);
    f64 r = one;
    for (;;) {
        ctx.gpr[4] = m & 1u;
        if ((m & 1u) != 0) r = static_cast<f32>(r * x);
        m >>= 1;
        if (m == 0) break;
        x = static_cast<f32>(x * x);
    }
    ctx.gpr[3] = 0;
    ctx.fpr[2] = one;
    if (!neg) {
        ctx.fpr[3] = r;
        ret(ctx, static_cast<f32>(r));
        return;
    }
    const f64 z = LfsAt(0x00742D54);
    ctx.gpr[3] = 0x00742D50;
    ctx.fpr[4] = z;
    const f64 t = r == z ? load<f64>(0x00742D58) : static_cast<f64>(static_cast<f32>(one / r));
    ctx.fpr[3] = t;
    ret(ctx, static_cast<f32>(t));
}
// T::~T: see VtB123D0DtorAt
DECOMP(00747180, VtB123D0_Dtor) { VtB123D0DtorAt(ctx, arg<u32>(ctx, 0)); }
// The shared body of the pool-freed deleting destructors 0x007487D8 /
// 0x00748978: vtable vt, 0x009000A0(o), then the r13 pool (-28332)->+44->
// vfn[12](o, the u16 +4). Null does nothing.
static void PoolDelete9000A0(PPUContext& ctx, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt);
    call(ctx, 0x009000A0, {o});
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}
// T::~T deleting form, vtable 0x00B1388C: see PoolDelete9000A0
DECOMP(007487D8, VtB1388C_Delete) { PoolDelete9000A0(ctx, 0x00B1388C); }
// T::~T deleting form, vtable 0x00B138B8: see PoolDelete9000A0
DECOMP(00748978, VtB138B8_Delete) { PoolDelete9000A0(ctx, 0x00B138B8); }
// void f(T* d, const Vec3* a, b, c, e; f32 x, f32 y; s32 k, u8 flag
// (stack)): d->+0 / +16 / +32 / +48 = a / b / c / e (raw), +64 = x, +68 =
// y, +72 = k, the u8 +76 = flag
DECOMP(0074CCA8, Frame_Set_0074CCA8) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 src[4] = {arg<u32>(ctx, 1), arg<u32>(ctx, 2), arg<u32>(ctx, 3), arg<u32>(ctx, 4)};
    const u32 k = arg<u32>(ctx, 7);
    const u32 flag = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116);  // the 9th argument's slot
    u32 w = 0;
    for (u32 r = 0; r < 3; ++r)
        for (u32 c = 0; c < 3; ++c) w = Cp(d + 16 * r + 4 * c, src[r] + 4 * c);
    Cp(d + 48, src[3]);
    store<u32>(d + 72, k);
    Cp(d + 52, src[3] + 4);
    w = load<u32>(src[3] + 8);
    store<f32>(d + 64, static_cast<f32>(farg(ctx, 0)));
    store<u32>(d + 56, w);
    store<f32>(d + 68, static_cast<f32>(farg(ctx, 1)));
    store<u8>(d + 76, static_cast<u8>(flag));
    ctx.fpr[3] = F32Bits(w);
    ctx.gpr[4] = flag;
}
// void f(T* o, a, b; f32 x): Hk_Activate91A4C8(o, a, b; x), then the
// member +224's vfn[96](b; x)
DECOMP(00757C24, Hk_Activate_00757C24) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 x = farg(ctx, 0);
    callf(ctx, 0x0091A4C8, {o, a, b}, {x});
    const u32 m = o + 224;
    const u32 opd = load<u32>(load<u32>(m) + 96);
    ctx.fpr[1] = x;
    call_ptr(ctx, opd, {m, opd, b, load<u32>(opd)});  // r4 = the OPD, r6 = the entry here
}
// T::~T deleting form: see VtB13974DeleteAt
DECOMP(0075A684, VtB13974_B194F8_Delete) {
    if (arg<u32>(ctx, 0) == 0) ctx.gpr[4] = 0;  // (r4 = r3 here)
    VtB13974DeleteAt(ctx, arg<u32>(ctx, 0));
}
// T::T(T* o): +0..+12 = 0, 0x006279E8(o + 16), the u8 +24 = 0,
// 0x006279FC(o + 28), +32..+44 = 0
DECOMP(00760CE0, Obj_Init_00760CE0) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 16; k += 4) store<u32>(o + k, 0);
    call(ctx, 0x006279E8, {o + 16});
    store<u8>(o + 24, 0);
    call(ctx, 0x006279FC, {o + 28});
    for (u32 k = 32; k < 48; k += 4) store<u32>(o + k, 0);
}
// T::~T deleting form: see VtB205B8DeleteAt
DECOMP(0077EBA4, VtB205B8_Delete) { VtB205B8DeleteAt(ctx, arg<u32>(ctx, 0)); }
// T::~T: vtable 0x00B21160, the PtrVector at +80 destroyed, the member +44
// (VtB21B58_DtorTrivial), then VtB20CC8_DtorTrivial_0042CA50(o)
DECOMP(0077F628, VtB21160_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B21160);
    VecDtor(ctx, o + 80);
    call(ctx, 0x004A0E08, {o + 44});
    call(ctx, 0x0042CA50, {o});
}
// void f(Part* o): unless the u8 +48 is set, the part +40 (when set) is
// released (Part_Release_0052F06C, Mem_FreeFrom(it, 0)) and +40 = 0; then
// Pool_DestructPart(o)
DECOMP(007881DC, Part_Destroy_007881DC) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 48) == 0) {
        const u32 p = load<u32>(o + 40);
        if (p != 0) {
            call(ctx, 0x0052F06C, {p});
            call(ctx, o25fn::Mem_FreeFrom, {p, 0});
            store<u32>(o + 40, 0);
        }
    }
    call(ctx, 0x0052EFE0, {o});
}
// void f(u8* d, const f32* s): d[0..3] = (u8)(s32)(s[i] * k) (truncated; k
// the f32 at 0x00799C10, e.g. 255). r4 / f1 left as the last conversion.
DECOMP(00799C18, Color_ToBytes_00799C18) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const f64 k = LfsAt(0x00799C10);
    u32 v = 0;
    for (u32 i = 0; i < 4; ++i) {
        v = static_cast<u32>(rt::fctiw(static_cast<f32>(LfsAt(s + 4 * i) * k), true));
        store<u8>(d + i, static_cast<u8>(v));
    }
    ctx.gpr[4] = v;
    ctx.fpr[1] = std::bit_cast<f64>(0xFFF8000000000000ull | rt::fctiw(static_cast<f32>(LfsAt(s) * k), true));  // the fctiwz bits, as the hardware leaves them
}
