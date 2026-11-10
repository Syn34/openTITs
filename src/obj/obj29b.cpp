// Game code, part 29 (continued): the small-function sweep from 48
// instructions up. See obj29.cpp for the conventions.
#include "obj29.h"

using namespace sly4;
using namespace sly4::obj29;

// ---------------------------------------------------------------- sweep: 48 instructions
// void f(T* o): o->+96.. = the position (+4->+128..) of the anchor +224
// (when set); with the flag 0x10 of the u8 +304 and a frame +212, it is
// transformed: 0x006C0AA4(frame, &pos, *0x011AD1D4), then o->+96.. =
// 0x006C1B94(&out, o->+212)'s result
DECOMP(000159EC, Obj_FollowAnchor_000159EC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 224);
    ctx.gpr[3] = c;
    if (c == 0) return;
    const u32 v = load<u32>(c + 4) + 128;
    const u32 x = load<u32>(v), y = load<u32>(v + 4), z = load<u32>(v + 8);
    store<u32>(o + 96, x);
    store<u32>(o + 100, y);
    store<u32>(o + 104, z);
    ctx.fpr[1] = F32Bits28(z);
    ctx.gpr[3] = v;
    if ((load<u8>(o + 304) & 0x10u) == 0) return;
    const u32 t = load<u32>(o + 212);
    ctx.gpr[3] = t;
    if (t == 0) return;
    Stack frame(ctx, 32);
    const u32 in = frame.addr(), out = frame.addr() + 12;
    store<u32>(in, x);
    store<u32>(in + 4, y);
    store<u32>(in + 8, z);
    call(ctx, 0x006C0AA4, {t, in, load<u32>(0x011AD1D4)});
    call(ctx, 0x006C1B94, {out, load<u32>(o + 212)});
    const u32 a = load<u32>(out), b = load<u32>(out + 4), d = load<u32>(out + 8);
    store<u32>(o + 96, a);
    store<u32>(o + 100, b);
    store<u32>(o + 104, d);
    ctx.fpr[1] = F32Bits28(a);
}

// void f(T* o): the heading of o->+48's velocity (+128 x / +136 z, with
// *0x0002D8BC as the y term): when its length > *0x0002D8C0, a =
// Atan2f(x / len * k, z / len * k) (k = *0x0002D8C4) and, when o->+48 has
// the flag 0x200 of +192, its +484 = a and +548 = 1.0f
DECOMP(0002D8C8, Obj_FaceVelocity_0002D8C8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x0002D8BC;
    const u32 v = load<u32>(o + 48) + 128;
    const f64 x = LfsAt(v), z = LfsAt(v + 8), k0 = LfsAt(K);
    const f32 len = static_cast<f32>(std::sqrt(static_cast<f64>(Madd28(z, z, Madd28(x, x, Mul28(k0, k0))))));
    ctx.fpr[1] = len;
    if (!(len > LfsAt(K + 4))) return;
    const f32 inv = static_cast<f32>(LfsAt(K + 8) / static_cast<f64>(len));
    const f64 a = callf(ctx, 0x005E665C, {}, {Mul28(x, inv), Mul28(z, inv)});  // Atan2f
    const u32 p = load<u32>(o + 48);
    if (p == 0) return;
    const u32 f = load<u32>(p + 192) & 0x200u;
    ctx.gpr[3] = f;
    if (f == 0) return;
    store<f32>(p + 484, static_cast<f32>(a));
    store<u32>(p + 548, 0x3F800000u);
    ctx.gpr[3] = 0x3F800000u;
}

// bool f(T* o; f32 dt): with the world's current object (kWorldPtr->* +
// 4208 + 8, flag 0x2 of +192): the timer o->+748 -= dt; once it is <=
// *0x00046634 it is cleared and the result is whether the object is within
// *0x00F961AC of o->+176 on x / z (that constant squared as the y term)
DECOMP(00046638, Obj_TimerNear_00046638) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    ctx.gpr[3] = 0;
    const u32 s = load<u32>(load<u32>(kWorldPtr) + 4208 + 8);
    if (s == 0 || (load<u32>(s + 192) & 0x2u) == 0) return;
    const f32 t = Sub28(LfsAt(o + 748), dt);
    store<f32>(o + 748, t);
    const f64 k = LfsAt(0x00046634);
    ctx.fpr[1] = k;
    if (!(t <= k)) return;
    store<u32>(o + 748, 0);
    const f32 dx = Sub28(LfsAt(s + 144), LfsAt(o + 176));
    const f32 dz = Sub28(LfsAt(s + 152), LfsAt(o + 184));
    const f64 r = LfsAt(0x00F961AC);
    const f32 d = Madd28(dz, dz, Madd28(dx, dx, Mul28(k, k)));
    ctx.fpr[1] = d;
    ctx.gpr[3] = d < static_cast<f64>(Mul28(r, r)) ? 1 : 0;
}

// void Cmd::Run(scope): the position +80.. = the kind-5 component
// Effect_FindComponent(scope, +12)'s target +28 position (+64.., raw;
// when set); then 0x002F2140(*0x00F64C38, &+16)
DECOMP(0006F3F8, Cmd_CopyTargetPos_0006F3F8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {arg<u32>(ctx, 1), load<u32>(o + 12)}));
    if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 q = load<u32>(c + 28);
        if (q != 0) CopyVec3F28(ctx, o + 80, q + 64);
    }
    ctx.gpr[3] = call(ctx, 0x002F2140, {load<u32>(0x00F64C38), o + 16});
}

// void Cmd::Run(): each of the nine world players (kWorldPtr->* + 0x1280F0
// + 4i) with the flag 0x8 of +192 gets the u8 +1172 = +16, the f32 +1168
// = +12 (raw), the u8 +1173 = +17 and Obj_PlayAttachedAnims(it, 1)
DECOMP(00089B0C, Cmd_SetPlayersAnim_00089B0C) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = 0;
    for (u32 i = 0; i < 9; ++i) {
        const u32 e = load<u32>(load<u32>(kWorldPtr) + 4 * i + 0x1280F0);
        r = e;
        if (e == 0 || (load<u32>(e + 192) & 0x8u) == 0) continue;
        const u32 b16 = load<u8>(o + 16);
        const u32 f = load<u32>(o + 12);
        store<u8>(e + 1172, static_cast<u8>(b16));
        store<u32>(e + 1168, f);
        store<u8>(e + 1173, load<u8>(o + 17));
        ctx.fpr[1] = F32Bits28(f);
        r = static_cast<u32>(call(ctx, 0x001231B8, {e, 1}));  // Obj_PlayAttachedAnims
    }
    ctx.gpr[3] = r;
}

// T* f(?, XmlCursor* c): new 20-byte record (base 0x004BB3B0(p, c),
// vtable 0x00B10508, f32 +12 = 1.0f) whose +12 is read from 0x2C9060E7
// (default *0x0008ACCC); the u8 +16 = whether the read failed
DECOMP(0008ACD0, NewXmlF32Flag_0008ACD0) {
    const u32 c = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {20, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, o28fn::Obj_InitVt_B21C98, {p, c});
    store<u32>(p, 0x00B10508);
    const f64 d = LfsAt(0x0008ACCC);
    store<u32>(p + 12, 0x3F800000u);
    store<u8>(p + 16, 0);
    callf(ctx, o28fn::XmlCursor_ReadF32, {c, 0x2C9060E7, p + 12}, {d});
    const u32 r = static_cast<u32>(ctx.gpr[3]);
    store<u8>(p + 16, r == 0 ? 1 : 0);
    ctx.gpr[3] = p;
}

// void Cmd::Run(scope): the kind-5 component Effect_FindComponent(scope,
// +12)'s target +28, when of kind 8 ((+4 >> 10) & 0x3F), gets
// WorldObj_Set292(it, (+292 | +16) & ~+20)
DECOMP(0008C230, Cmd_SetObjFlags_0008C230) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {arg<u32>(ctx, 1), load<u32>(o + 12)}));
    if (c == 0) return;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) != 5) return;
    const u32 q = load<u32>(c + 28);
    if (q == 0) return;
    const u32 k = (load<u32>(q + 4) >> 10) & 0x3Fu;
    ctx.gpr[3] = k;
    if (k != 8) return;
    ctx.gpr[3] = call(ctx, 0x00580AF0, {q, (load<u32>(q + 292) | load<u32>(o + 16)) & ~load<u32>(o + 20)});  // WorldObj_Set292
}

// void Cmd::Run(): the prompt Handle_ResolveBit1(+144) (when found):
// Obj_SetField180_003676DC(it, +148) when set, Prompt_Fade367758(it; +152),
// then Obj_SetAnim(it, the name +12 (0 when its first byte is 0), the u8s
// +161, +161, +160, 0, 0x01164CA4; +156)
DECOMP(00096130, Cmd_ShowPrompt_00096130) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, 0x003568E0, {load<u32>(o + 144)}));
    if (h == 0) return;
    const u32 a = load<u32>(o + 148);
    if (a != 0) call(ctx, 0x003676DC, {h, a});
    callf(ctx, 0x00367758, {h}, {LfsAt(o + 152)});
    const u32 b161 = load<u8>(o + 161), b160 = load<u8>(o + 160);
    const u32 name = load<u8>(o + 12) != 0 ? o + 12 : 0u;
    callf(ctx, 0x00363A3C, {h, name, b161, b161, b160, 0, 0x01164CA4}, {LfsAt(o + 156)});  // Obj_SetAnim
}

// void Cmd::Run(scope): the kind-13 component Effect_FindComponent(scope,
// +12) (when +12 is set) gets its emitter +20 stopped (Emitter_Stop(h, the
// u8 +16 == 0)) and cleared
DECOMP(00096B14, Cmd_StopEmitter_00096B14) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 12);
    ctx.gpr[3] = n;
    if (n == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {arg<u32>(ctx, 1), n}));
    if (c == 0) return;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) != 13) return;
    call(ctx, o25fn::Emitter_Stop, {load<u32>(c + 20), load<u8>(o + 16) == 0 ? 1u : 0u});
    store<u32>(c + 20, 0);
    ctx.gpr[3] = 0;
}

// void Cmd::Run(): when the f32s +12 and +16 are >= *0x0009D700,
// CmdList_Push_0042C138(a new 32-byte VtB20EC0_Ctor(p; +12, +16, +20, +24;
// the u8s +28, +29 in r8 / r9), or 0 when the allocation fails)
DECOMP(0009D708, Cmd_PushNew_0009D708) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = LfsAt(0x0009D700);
    ctx.fpr[1] = k;
    ctx.fpr[2] = LfsAt(o + 12);
    if (!(LfsAt(o + 12) >= k)) return;
    ctx.fpr[2] = LfsAt(o + 16);
    if (!(LfsAt(o + 16) >= k)) return;
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {32}));
    if (p != 0) {
        // the floats take the GPR slots 4..7 (left as SmallAlloc left them)
        callf(ctx, 0x0042F264, {p, ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], load<u8>(o + 28), load<u8>(o + 29)},
              {LfsAt(o + 12), LfsAt(o + 16), LfsAt(o + 20), LfsAt(o + 24)});
    }
    ctx.gpr[3] = call(ctx, 0x0042C138, {p});
}

// void f(a, b, u8 c): pushes the 16-byte record {a, b, 1, c} on the vector
// 0x00F9CDA8
DECOMP(000AB954, Queue_Push1_000AB954) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2) & 0xFFu;
    VecPush28(ctx, 0x00F9CDA8, 4, [&](u32 e) {
        store<u32>(e, a);
        store<u32>(e + 4, b);
        store<u32>(e + 8, 1);
        store<u32>(e + 12, c);
    });
}

// void f(a, u8 b, u8 c): pushes the 16-byte record {a, b, 2, c} on the
// vector 0x00F9CDA8
DECOMP(000ABA14, Queue_Push2_000ABA14) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu, c = arg<u32>(ctx, 2) & 0xFFu;
    VecPush28(ctx, 0x00F9CDA8, 4, [&](u32 e) {
        store<u32>(e, a);
        store<u32>(e + 4, b);
        store<u32>(e + 8, 2);
        store<u32>(e + 12, c);
    });
}

// s32 f(): the sum of the values (+4 of the 8-byte records at
// *0x00F64620) of the trophies Trophy_Has(*0x00F668E8, i) reports, for i
// below the count (+200 low 16 bits, when the u8 +8 is set)
DECOMP(000B2A20, Trophy_SumOwned_000B2A20) {
    const u32 g = load<u32>(0x00F668E8);
    const s32 n = load<u8>(g + 8) != 0 ? static_cast<s32>(load<u32>(g + 200) & 0xFFFFu) : 0;
    s32 sum = 0;
    for (s32 i = 0; i < n; ++i) {
        if (static_cast<u32>(call(ctx, 0x004FB56C, {load<u32>(0x00F668E8), static_cast<u32>(i)})) == 0) continue;
        sum += static_cast<s32>(load<u32>(load<u32>(0x00F64620) + 8 * static_cast<u32>(i) + 4));
    }
    ret<s32>(ctx, sum);
}

// T* f(T* o, u8 down, u8 both): o->+280 when o->+276 is 0x02235C33 and it
// is set; else, with down, the same on o->+260; else (with both) on
// o->+272, then o->+268; 0 for a null o
DECOMP(000D6AA0, Obj_FindLinked_000D6AA0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 r = load<u32>(o + 276) == 0x02235C33u ? load<u32>(o + 280) : 0u;
    ctx.gpr[3] = r;
    if (r != 0) return;
    if (a != 0) {
        call(ctx, 0x000D6AA0, {load<u32>(o + 260), a, b});
        return;
    }
    if (b == 0) return;
    if (static_cast<u32>(call(ctx, 0x000D6AA0, {load<u32>(o + 272), a, b})) != 0) return;
    call(ctx, 0x000D6AA0, {load<u32>(o + 268), a, b});
}

// void f(a, b, c): pushes the 12-byte record {a, b, c} on the vector
// 0x01013B84
DECOMP(000D6ED0, List13B84_Push_000D6ED0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    VecPushN29(ctx, 0x01013B84, 12, [&](u32 e) {
        store<u32>(e, a);
        store<u32>(e + 4, b);
        store<u32>(e + 8, c);
    });
}

// void f(T* o; f32 t): each set child (+192 of the 240-byte records of
// o->+340, u16 count +8) gets vt[112](it; t) (r3 = o->+340)
DECOMP(000DA25C, Obj_ChildrenUpdate_000DA25C) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    for (u32 i = 0;; ++i) {
        const u32 c = load<u32>(o + 340);
        const u32 n = c != 0 ? load<u16>(c + 8) : 0u;
        if (i >= n) {
            ctx.gpr[3] = c;
            return;
        }
        const u32 k = load<u32>(load<u32>(c) + i * 240 + 192);
        if (k == 0) continue;
        ctx.fpr[1] = t;
        VCall(ctx, k, 112, {k});
    }
}

// void f(T* o, u8 on): bit 0x2 of o->+172 = on & 1, o->vt[212](o, on != 0),
// then the flags 0x8000 / 0x4000 of +172 = (on != 0) / 0
DECOMP(000DB0C0, Obj_SetActive_000DB0C0) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x2u) | ((on & 1u) << 1));
    const bool b = (on & 0xFFu) != 0;
    VCall(ctx, o, 212, {o, b ? 1u : 0u});
    const u32 w = b ? (load<u32>(o + 172) | 0x8000u) & ~0x4000u : load<u32>(o + 172) & ~0xC000u;
    store<u32>(o + 172, w);
    ctx.gpr[3] = w;
}

// f32 f(T* o, u8 norm, u32 name): the time of the track Anim_Find34CC34(
// o->+off, name) (Track_Time353780), divided by Track_Length353748 when
// norm (when above *(K + 4), else *(K + 8)); *K without a track
static void TrackTimeNorm29(PPUContext& ctx, u32 off, u32 K) {
    const u32 o = arg<u32>(ctx, 0), norm = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2);
    f64 t = LfsAt(K);
    const u32 a = load<u32>(o + off);
    ctx.gpr[3] = a;
    if (a != 0) {
        const u32 tr = static_cast<u32>(call(ctx, 0x0034CC34, {a, name}));
        if (tr != 0) {
            call(ctx, 0x00353780, {tr});  // Track_Time353780
            t = ctx.fpr[1];
            ctx.gpr[3] = norm & 0xFFu;
            if ((norm & 0xFFu) != 0) {
                call(ctx, 0x00353748, {tr});  // Track_Length353748
                const f64 len = ctx.fpr[1];
                t = len > LfsAt(K + 4) ? static_cast<f64>(static_cast<f32>(t / len)) : LfsAt(K + 8);
                ctx.gpr[3] = K;
            }
        }
    }
    ctx.fpr[1] = t;
}

// TrackTimeNorm29 on o->+164 with the constants at 0x000DC47C
DECOMP(000DC488, Anim_TrackTime_000DC488) { TrackTimeNorm29(ctx, 164, 0x000DC47C); }

// void f(Player* o): unless o->vt[32](o, 0) is 5, when bit 0x20 of +188
// is set: Player_SetSave124D58(o, Save_Profile0C9FC8(0x14120), 0), and its
// world slot (s8 +172) gets +108 = 0 and the flag 0x40 of +0
DECOMP(00125998, Player_Respawn_00125998) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) == 5) return;
    const u32 f = load<u32>(o + 188) & 0x20u;
    ctx.gpr[3] = f;
    if (f == 0) return;
    const u32 p = static_cast<u32>(call(ctx, 0x000C9FC8, {0x14120}));
    call(ctx, 0x00124D58, {o, p, 0});
    const u32 s = load<u32>(kWorldPtr) + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(o + 172)))) * 0x20C80u + 4208;
    const u32 w = load<u32>(s);
    store<u32>(s + 108, 0);
    store<u32>(s, w | 0x40u);
    ctx.gpr[3] = s;
}

// void Cmd::Run(T* obj): obj->+720 = +4; when obj has a component of kind
// 19 (Obj_FindComponent(obj, 19, CallK2_000D94F4(obj))), its +1224 / +1228
// = *0x0016D5E8 / (f32)+4 and +1232 = Obj_RandRange_0025DD60(it; that,
// (f32)+4) first
DECOMP(0016D5F0, Cmd_SetRate_0016D5F0) {
    const u32 o = arg<u32>(ctx, 0), obj = arg<u32>(ctx, 1);
    u32 c = static_cast<u32>(call(ctx, 0x000D94F4, {obj}));
    if (c != 0) c = static_cast<u32>(call(ctx, 0x000D1288, {obj, 19, c}));  // Obj_FindComponent
    if (c != 0) {
        const f32 n = static_cast<f32>(static_cast<f64>(load<u32>(o + 4)));
        const f32 q = static_cast<f32>(LfsAt(0x0016D5E8) / static_cast<f64>(n));
        store<f32>(c + 1224, q);
        store<f32>(c + 1228, q);
        const f64 r = callf(ctx, 0x0025DD60, {c}, {q, n});  // Obj_RandRange_0025DD60
        store<f32>(c + 1232, static_cast<f32>(r));
    }
    const u32 v = load<u32>(o + 4);
    store<u32>(obj + 720, v);
    ctx.gpr[3] = v;
}

// f32 f(T* o): the u16 o->vt[144]()->+510 as the high half of an f32,
// when the world list (*0x010B19E0 + 0x1281D4)->+4 is set and vt[144]()
// returns an object (called twice); else *0x001787D8
DECOMP(001787E0, Obj_GetHalfF32_001787E0) {
    const u32 o = arg<u32>(ctx, 0);
    f64 v = LfsAt(0x001787D8);
    const u32 l = load<u32>(load<u32>(load<u32>(0x010B19E0) + 0x1281D4) + 4);
    ctx.gpr[3] = l;
    if (l != 0 && static_cast<u32>(VCall(ctx, o, 144, {o})) != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, o, 144, {o}));
        const u32 w = static_cast<u32>(load<u16>(r + 510)) << 16;
        ctx.gpr[3] = w;
        v = F32Bits28(w);
    }
    ctx.fpr[1] = v;
}

// bool f(Area* o, T* e): when e (flag 0x8 of +192, or 0x400 of +200) has
// bit 0 of +188 and Area_Enter(o, e) succeeds: 0x00218DD8(o, e) and true
DECOMP(0021941C, Area_TryEnter_0021941C) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0;
    if (e == 0) return;
    if ((load<u32>(e + 192) & 0x8u) == 0 && (load<u32>(e + 200) & 0x400u) == 0) return;
    if ((load<u32>(e + 188) & 1u) == 0) return;
    if (static_cast<u32>(call(ctx, 0x0020E7A4, {o, e})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x00218DD8, {o, e});
    ctx.gpr[3] = 1;
}

// void f(T* o): Nop_000D8FC0(o); with the flag 0x80 of the u8 +1384: when
// bit 0x100 of +172 is set, Obj_ApplyTree_000D8F08(o, &+1385, 1); then the
// flag 0x40 of +1384 = that bit and o->vt[88](o, 0, 1, 1, 0)
DECOMP(0024D744, Obj_Refresh_0024D744) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000D8FC0, {o});
    u32 b = load<u8>(o + 1384);
    ctx.gpr[3] = b;
    if ((b & 0x80u) == 0) return;
    u32 w = load<u32>(o + 172);
    if ((w & 0x100u) != 0) {
        call(ctx, 0x000D8F08, {o, o + 1385, 1});
        w = load<u32>(o + 172);
        b = load<u8>(o + 1384);
    }
    store<u8>(o + 1384, static_cast<u8>((b & ~0x40u) | ((w & 0x100u) != 0 ? 0x40u : 0u)));
    VCall(ctx, o, 88, {o, 0, 1, 1, 0});
}

// void f(T* o, u8 a, u8 b): each set entry of o->+2912 (u16 count +2944)
// gets vt[slot](it, a, b) (r3 = the count)
static void ForEachSlot29(PPUContext& ctx, u32 slot) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    u32 n = load<u16>(o + 2944);
    for (u32 i = 0; i < n; ++i) {
        const u32 c = load<u32>(o + 2912 + 4 * i);
        if (c == 0) continue;
        VCall(ctx, c, slot, {c, a, b});
        n = load<u16>(o + 2944);
    }
    ctx.gpr[3] = n;
}

// ForEachSlot29 with vt[40] / vt[52]
DECOMP(0028EED4, Obj_ForEachSlot40_0028EED4) { ForEachSlot29(ctx, 40); }
DECOMP(0028EF94, Obj_ForEachSlot52_0028EF94) { ForEachSlot29(ctx, 52); }

// TrackTimeNorm29 on o->+36 with the constants at 0x002989CC
DECOMP(002989D8, Anim_TrackTime_002989D8) { TrackTimeNorm29(ctx, 36, 0x002989CC); }

// bool f(T* p, U* e): true unless (with both set, the mode
// 0x010682A8->+72 not 1 and e->+56 below the limit *0x00F64C24 or no
// limit) Player_StartAction(world, p->+8, 0x00AD8EDC, 0x00AD8EEC, 0, 0, 0,
// 0, 1) is started, which returns false; false for a null p / e
DECOMP(002E74C8, Obj_CheckOrAct_002E74C8) {
    const u32 p = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    if (p == 0 || e == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (static_cast<s32>(load<u32>(0x010682A8 + 72)) == 1) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 lim = load<u32>(0x00F64C24);
    if (lim != 0 && !(load<u32>(e + 56) < lim)) {
        ctx.gpr[3] = 1;
        return;
    }
    call(ctx, 0x00489A60, {load<u32>(kWorldPtr), load<u32>(p + 8), 0x00AD8EDC, 0x00AD8EEC, 0, 0, 0, 0, 1});  // Player_StartAction
    ctx.gpr[3] = 0;
}

// void f(T* o): o->+692 = the entry of o->+216's list +324 (u16 count
// +332) whose +32 is the key 0x09A1F02B (0x0993D8AA with the u8 +665) + the
// number of set u8s among +676 / +678 (0 for none), +696 = 0
DECOMP(0030FC90, Obj_PickEntry_0030FC90) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + 216);
    const u32 base = load<u8>(o + 665) != 0 ? 0x0993D8AAu : 0x09A1F02Bu;
    u32 k = load<u8>(o + 676) != 0 ? 1u : 0u;
    if (load<u8>(o + 678) != 0) ++k;
    const u32 key = k + base;
    const u32 l = q + 324;
    u32 found = 0;
    for (u32 i = 0; i < load<u16>(l + 8); ++i) {
        const u32 data = load<u32>(l);
        if (load<u32>(load<u32>(data + 4 * i) + 32) == key) {
            found = load<u32>(data + 4 * i);
            break;
        }
    }
    store<u32>(o + 692, found);
    store<u32>(o + 696, 0);
}

// bool f(T* o): false after o->vt[28](o, 0, 2) when o->vt[32](o, 0) is 1
// and GetU8_0_004A08C0(o->+32 + 544) is set; else true
DECOMP(0031A378, Obj_TryState2_0031A378) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = static_cast<u32>(call(ctx, 0x004A08C0, {load<u32>(o + 32) + 544}));
    const s32 k = static_cast<s32>(VCall(ctx, o, 32, {o, 0}));
    if (k != 1 || v == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    VCall(ctx, o, 28, {o, 0, 2});
    ctx.gpr[3] = 0;
}

// u32 f(const char* s): the sum of String_HashNoCase of the tokens of s
// (strtok 0x0071EFA4 on a stack copy, delimiters 0x00ADB7D4); 0 for a null
// s or one of 255 characters or more
DECOMP(00324A48, String_HashTokens_00324A48) {
    const u32 s = arg<u32>(ctx, 0);
    if (s == 0 || !(static_cast<s32>(static_cast<u32>(call(ctx, 0x00555FA4, {s}))) < 255)) {  // String_Length
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 256);
    const u32 buf = frame.addr();
    call(ctx, 0x00555F80, {buf, s});  // String_Assign
    u32 sum = 0;
    u32 tok = static_cast<u32>(call(ctx, 0x0071EFA4, {buf, 0x00ADB7D4}));
    while (tok != 0) {
        sum += static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {tok}));
        tok = static_cast<u32>(call(ctx, 0x0071EFA4, {0, 0x00ADB7D4}));
    }
    ctx.gpr[3] = sum;
}

// void f(T* o): for each object of the chain o->+2576 (next +2564), the
// head o->+2576 gets vt[88]() (the original calls the head each time --
// quirk kept); then +2540 / +2552 = 0 and +2548 = o->+2544->+4 (raw) when
// +2544 is set
DECOMP(003282E8, Obj_ResetHead_003282E8) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 c = load<u32>(o + 2576); c != 0; c = load<u32>(c + 2564)) {
        const u32 h = load<u32>(o + 2576);
        VCall(ctx, h, 88, {h});
    }
    const u32 a = load<u32>(o + 2544);
    store<u32>(o + 2540, 0);
    store<u32>(o + 2552, 0);
    ctx.gpr[3] = a;
    if (a == 0) return;
    const u32 w = load<u32>(a + 4);
    store<u32>(o + 2548, w);
    ctx.fpr[1] = F32Bits28(w);
}

// void f(T* o): when o->+36 is among the u32 list o->+24->+16 (s32 count
// +20): its +112 = 0x7F7FFFEE and its state +120 = 3 when the f32 +116 is
// *0x00353DC4 (after +116 = that - +116 for the states 0 / 1), else 2
DECOMP(00353DC8, Anim_FlipState_00353DC8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 24);
    if (a == 0) return;
    const u32 k = load<u32>(o + 36);
    ctx.gpr[3] = k;
    if (k == 0) return;
    const s32 n = static_cast<s32>(load<u32>(a + 20));
    for (s32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(a + 16) + 4 * static_cast<u32>(i));
        if (e != k) continue;
        const u32 s = load<u32>(e + 120);
        const f64 K = LfsAt(0x00353DC4);
        if (s == 0 || s == 1) store<f32>(e + 116, Sub28(K, LfsAt(e + 116)));
        store<u32>(e + 112, 0x7F7FFFEEu);
        const f64 v = LfsAt(e + 116);
        const u32 st = v == K ? 3u : 2u;
        store<u32>(e + 120, st);
        ctx.gpr[3] = st;
        ctx.fpr[1] = v;
        return;
    }
}

// void f(T* o): unless in state 0 / 3 (+160 >> 1): o->vt[20](o, 1), state
// 3 (bit 0 kept), Speaker_SayId4A5C90(+28, +188, 0; *0x0036D284), then
// Effect_SetHash4CF988(+20, 0x048129EB) when +20 is set
DECOMP(0036D288, Obj_EnterState3_0036D288) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 160) >> 1;
    ctx.gpr[3] = k;
    if (k == 3 || k == 0) return;
    VCall(ctx, o, 20, {o, 1});
    const u32 w = (load<u32>(o + 160) & 1u) | 6u;
    const u32 sp = load<u32>(o + 28);
    store<u32>(o + 160, w);
    callf(ctx, 0x004A5C90, {sp, load<u32>(o + 188), 0}, {LfsAt(0x0036D284)});  // Speaker_SayId4A5C90
    const u32 e = load<u32>(o + 20);
    if (e != 0) call(ctx, o27fn::Effect_SetHash, {e, 0x048129EBu});
}

// void f(T* o): 0x003AB0B8(o, hash("IGC_OperationMousetrap_HockeyFight_%s_%s"
// of +28, "Shot_01"), the same of +60, 0, -, 1; *0x003AB8E8)
DECOMP(003AB8F0, Igc_PlayHockeyShot_003AB8F0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kFmt = 0x00ADF2D8, kShot = 0x00ADF308;
    call(ctx, o24fn::String_Format, {kFmt, o + 28, kShot});
    const u32 h1 = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {ctx.gpr[3]}));
    call(ctx, o24fn::String_Format, {kFmt, o + 60, kShot});
    const u32 h2 = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {ctx.gpr[3]}));
    // r7 is not set by the original (the callee does not read it)
    callf(ctx, 0x003AB0B8, {o, h1, h2, 0, ctx.gpr[7], 1}, {LfsAt(0x003AB8E8)});
}

// The static destructor of the tuning globals: the var 0x0108A164 (vtable
// 0x00B0D99C, TuningVar_DestructBase_004FC1A0) and the PtrVectors
// 0x0108A150, 0x0108A128, 0x0108A110, 0x0108A13C
DECOMP(004284F8, Tuning_StaticDtor_004284F8) {
    store<u32>(0x0108A164, 0x00B0D99C);
    call(ctx, 0x004FC1A0, {0x0108A164});
    for (u32 v : {0x0108A150u, 0x0108A128u, 0x0108A110u, 0x0108A13Cu}) VecDtor(ctx, v);
}

// bool Cmd::Done(): with an object +24, Call_443368_348_000F60DC(it) == 0;
// else false while the voice +28 plays, then the voice is stopped
// (Emitter_Stop(h, 1)), Audio_Release446384(0x010ACC40, +16) and +16 / +28
// cleared (true)
DECOMP(00434950, Cmd_VoiceDone_00434950) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 24);
    if (a != 0) {
        ctx.gpr[3] = static_cast<u32>(call(ctx, 0x000F60DC, {a})) == 0 ? 1 : 0;
        return;
    }
    if (static_cast<u32>(call(ctx, 0x00443368, {load<u32>(o + 28)})) != 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, o25fn::Emitter_Stop, {load<u32>(o + 28), 1});
    call(ctx, 0x00446384, {0x010ACC40, load<u32>(o + 16)});  // Audio_Release446384
    store<u32>(o + 16, 0);
    store<u32>(o + 28, 0);
    ctx.gpr[3] = 1;
}

// T* f(?, XmlCursor* c): new 28-byte record (vtable 0x00B21EB0, u16 +4 =
// 1, +20 = 0, +24 = -1) reading the hash 0x0024728B into +8 (default 0)
// and the s32 0x00014B01 into +24 (default 0)
DECOMP(004BF9C4, NewXmlHashS32_004BF9C4) {
    const u32 c = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {28}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u16>(p + 4, 1);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B21EB0);
    store<u32>(p + 24, 0xFFFFFFFFu);
    call(ctx, o29fn::XmlCursor_ReadHash, {c, 0x0024728B, p + 8, 0});
    call(ctx, 0x004F99BC, {c, 0x00014B01, p + 24, 0});  // XmlCursor_ReadS32
    ctx.gpr[3] = p;
}

// bool Load(XmlCursor* c): the settings' (0x010C2C28, when set) f32 +876 =
// 0x00AF048C (default *0x004EF918), clamped to [*0x004EF91C, *0x004EF918]
DECOMP(004EF920, Settings_LoadC_004EF920) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x010C2C28);
    if (g == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const f64 hi = LfsAt(0x004EF918);
    callf(ctx, 0x004FA5C4, {c, 0x00AF048C, g + 876}, {hi});  // XmlCursor_ReadF32ByName
    const f64 v = LfsAt(g + 876);
    const f64 m = v < hi ? v : hi;
    const f64 r = m > LfsAt(0x004EF91C) ? m : LfsAt(0x004EF91C);
    store<f32>(g + 876, static_cast<f32>(r));
    ctx.fpr[1] = r;
    ctx.gpr[3] = 1;
}

// T::~T() (vtable 0x00B1B6F4, then 0x00B13974): unless bit 31 of +16 is
// set (not owned), the block +8 (48 * (+16 & 0x3FFFFFFF) bytes) goes back
// to the allocator 0x011DF31C (vt[20](alloc, p, size)); then +12 / +8 = 0,
// +16 = 0x80000000
DECOMP(0075BEE0, VtB1B6F4_Dtor_0075BEE0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 16);
    const u32 p = load<u32>(o + 8);
    store<u32>(o + 12, 0);
    if ((w & 0x80000000u) == 0) {
        store<u32>(o, 0x00B1B6F4);
        VCall(ctx, 0x011DF31C, 20, {0x011DF31C, p, Sx((w & 0x3FFFFFFFu) * 48u)});
    }
    store<u32>(o + 8, 0);
    store<u32>(o + 16, 0x80000000u);
    store<u32>(o, 0x00B13974);
    ctx.gpr[3] = 0;
}

// T::~T() deleting (vtable 0x00B20748): the PtrVectors +232, +132, +112,
// the base 0x00490B34(o), then SmallFree(o, 272)
DECOMP(0077EDD8, VtB20748_Delete_0077EDD8) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B20748);
    VecDtor(ctx, o + 232);
    VecDtor(ctx, o + 132);
    VecDtor(ctx, o + 112);
    call(ctx, 0x00490B34, {o});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 272});
}

// T::~T() (vtable 0x00B220D8, then 0x00B11B20): the block +40 goes back to
// the allocator +44 (vt[8](alloc, p, u16 +32 * (u16 +38 >> 1))), then
// Array_Destruct(&+32) and the PtrVector +12
DECOMP(00794264, VtB220D8_Dtor_00794264) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = static_cast<u32>(load<u16>(o + 32)) * ((static_cast<u32>(load<u16>(o + 38)) >> 1) & 0x7FFFu);
    store<u32>(o, 0x00B220D8);
    const u32 p = load<u32>(o + 40);
    if (p != 0) {
        const u32 al = load<u32>(o + 44);
        VCall(ctx, al, 8, {al, p, n});
    }
    call(ctx, 0x0054EC94, {o + 32});  // Array_Destruct
    VecDtor(ctx, o + 12);
    store<u32>(o, 0x00B11B20);
    ctx.gpr[3] = 0x00B11B20;
}
