// Game code, medium sweep part 29: AnimScript commands that queue a
// command object (SmallAlloc + constructor + CmdList_Push_0042C138, a
// null pushed when the allocation fails), and two object component syncs.
#include "mid29.h"

using namespace sly4;
using namespace sly4::mid29;

namespace {
using namespace m29fn;
constexpr u32 kPush = 0x0042C138u;  // CmdList_Push_0042C138(cmd)

// The value of the kind-2 component `key` (vt[24]()), else def (r3 = 0
// for an empty key, else as the steps leave it).
u32 Kind2Val29(PPUContext& ctx, u32 x, u32 key, u32 def) {
    if (key == 0) {
        ctx.gpr[3] = 0;
        return def;
    }
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, key}));
    if (e == 0 || static_cast<u32>(VCall(ctx, e, 32, {e})) != 2) return def;
    return static_cast<u32>(VCall(ctx, e, 24, {e}));
}

// The kind-`want` component h of the object list o->+356, else 0.
u32 KindOf29(PPUContext& ctx, u32 o, u32 h, u32 want) {
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {load<u32>(o + 356), h}));
    if (e == 0 || static_cast<u32>(VCall(ctx, e, 32, {e})) != want) return 0;
    return e;
}

inline u64 Sx32c(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }
inline f64 Ld29(u32 ea) { return std::bit_cast<f64>(load<u64>(ea)); }
}  // namespace

// u32 f(Cmd* c, Effect* x): queues a VtB21040 command (44 bytes,
// VtB21040_Ctor_004321A4(p, c->+12, v1, c->+28, f1 = c->+32, f2 = c->+36,
// v2, u8 c->+24 == 1, u8 c->+48..+51)) when v1 (c->+16, or the kind-2
// component c->+20's value) and c->+12 are set; v2 = c->+40 or the kind-2
// component c->+44's. r7 / r8 = u8 c->+49 / +48 as the original leaves
// them. r3 as the original leaves it.
DECOMP(000A0270, AnimCmd_QueueVtB21040_000A0270) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 v1 = Kind2Val29(ctx, x, load<u32>(c + 20), load<u32>(c + 16));
    const u32 v2 = Kind2Val29(ctx, x, load<u32>(c + 44), load<u32>(c + 40));
    if (v1 == 0) return;
    if (load<u32>(c + 12) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, SmallAlloc28, {44}));
    if (p != 0) {
        const u32 b48 = load<u8>(c + 48), b49 = load<u8>(c + 49);
        callf(ctx, 0x004321A4u, {p, load<u32>(c + 12), v1, load<u32>(c + 28), b49, b48, v2, load<u8>(c + 24) == 1 ? 1u : 0u, b48, b49, load<u8>(c + 50), load<u8>(c + 51)}, {Lf(c + 32), Lf(c + 36)});
    }
    call(ctx, kPush, {p});
}

// u32 f(Cmd* c, Effect* x): queues an Obj_Ctor_0042CCD4 command (128
// bytes) when c->+12 and c->+16 are set: (p, c->+12, c->+16, v (c->+20 or
// the kind-2 component c->+24's), &a, &b, c->+84, c + 96, c + 112, u8
// c->+128..+131) with the vectors a = c->+32.. (or the kind-3 component
// c->+48's +32..) and b = c->+64.. (or the kind-3 component c->+80's).
// r3 / f1 as the original leaves them.
DECOMP(0009AA20, AnimCmd_QueueObj42CCD4_0009AA20) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (load<u32>(c + 12) == 0 || load<u32>(c + 16) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = Kind2Val29(ctx, x, load<u32>(c + 24), load<u32>(c + 20));
    Stack frame(ctx, 32);  // a at 160(r1), b at 176(r1)
    const u32 va = frame.addr(), vb = va + 16;
    auto copy3 = [&](u32 dst, u32 src) {
        for (u32 i = 0; i < 3; ++i) store<u32>(dst + 4 * i, load<u32>(src + 4 * i));
    };
    auto vec = [&](u32 dst, u32 src, u32 keyOff, u32 last) {
        copy3(dst, src);
        ctx.fpr[1] = Lf(last);  // the last lfs into f1
        if (const u32 k = load<u32>(c + keyOff); k != 0) {
            const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
            if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 3) {
                copy3(dst, e + 32);
                ctx.fpr[1] = Lf(e + 32);
            }
        }
    };
    vec(va, c + 32, 48, c + 40);
    vec(vb, c + 64, 80, c + 72);
    const u32 p = static_cast<u32>(call(ctx, SmallAlloc28, {128}));
    if (p != 0)
        call(ctx, 0x0042CCD4u, {p, load<u32>(c + 12), load<u32>(c + 16), v, va, vb, load<u32>(c + 84), c + 96, c + 112, load<u8>(c + 128), load<u8>(c + 129), load<u8>(c + 130), load<u8>(c + 131)});  // Obj_Ctor_0042CCD4
    call(ctx, kPush, {p});
}

// u32 f(Cmd* c, Effect* x): queues an Obj_Construct4335C0 command (172
// bytes): the names at c + 12 / c + 77 first get the strings of the
// kind-16 components c->+164 / +168 (String_Copy(dst, comp + 20, 64) when
// non-empty); then (p, c + 144, c + 12, c + 77, v1, v2, bits 0x20000000 /
// 0x80000000 / 0x40000000 of c->+188) with v1 / v2 = c->+172 / +180 or
// the kind-2 components c->+176 / +184's values. r3 as the original
// leaves it.
DECOMP(000A16C0, AnimCmd_QueueObj4335C0_000A16C0) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    auto name = [&](u32 keyOff, u32 dst) {
        const u32 k = load<u32>(c + keyOff);
        if (k == 0) return;
        const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
        if (e == 0 || static_cast<u32>(VCall(ctx, e, 32, {e})) != 16) return;
        const u32 s = e + 20;
        if (s == 0 || load<u8>(s) == 0) return;
        call(ctx, 0x00553684u, {c + dst, s, 64});  // String_Copy
    };
    name(164, 12);
    name(168, 77);
    const u32 v1 = Kind2Val29(ctx, x, load<u32>(c + 176), load<u32>(c + 172));
    const u32 v2 = Kind2Val29(ctx, x, load<u32>(c + 184), load<u32>(c + 180));
    const u32 p = static_cast<u32>(call(ctx, SmallAlloc28, {172}));
    if (p != 0) {
        const u32 f = load<u32>(c + 188);
        call(ctx, 0x004335C0u, {p, c + 144, c + 12, c + 77, v1, v2, (f >> 29) & 1u, f >> 31, (f >> 30) & 1u});  // Obj_Construct4335C0
    }
    call(ctx, kPush, {p});
}

// u32 f(Cmd* c, Effect* x): unless the key c->+36 passes Key_Check_0044D3AC:
// with the kind-2 component c->+12's value v, c->+24 =
// Node_PushF65B7C(v, c->+20) (Hash_ToString of v and c->+20 when
// Ptr_DerefNonZero of it fails); with c->+16 the same check of c->+24
// (Hash_ToString of c->+16 and c->+20). Then with the owned object o =
// Obj_FindOwned73_000F6118(c->+32), or without one while f32 c->+28 >
// *0x000A28C0, queues f_004342DC(p (124 bytes), c->+24, (s32) c->+132, o,
// c + 56, c->+124, c->+44, c->+48, f1 = c->+28, u8 c->+40, u8 c->+41,
// c->+52, c->+128, (s32) c->+132). r3 / f1 as the original leaves them.
DECOMP(000A28C8, AnimCmd_QueueObj4342DC_000A28C8) {
    constexpr u32 kDeref = 0x00446960u;  // Ptr_DerefNonZero
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (const u32 k = load<u32>(c + 36); k != 0 && static_cast<u32>(call(ctx, 0x0044D3ACu, {k})) != 0) return;  // Key_Check_0044D3AC
    if (const u32 h = load<u32>(c + 12); h != 0) {
        const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, h}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 2) {
            if (const u32 v = static_cast<u32>(VCall(ctx, e, 24, {e})); v != 0) {
                const u32 r = static_cast<u32>(call(ctx, 0x00446984u, {v, load<u32>(c + 20)}));  // Node_PushF65B7C
                store<u32>(c + 24, r);
                if (static_cast<u32>(call(ctx, kDeref, {r})) == 0) {
                    call(ctx, HashStr29, {v});
                    call(ctx, HashStr29, {load<u32>(c + 20)});
                }
            }
        }
    }
    if (load<u32>(c + 16) != 0 && static_cast<u32>(call(ctx, kDeref, {load<u32>(c + 24)})) == 0) {
        call(ctx, HashStr29, {load<u32>(c + 16)});
        call(ctx, HashStr29, {load<u32>(c + 20)});
    }
    const u32 o = static_cast<u32>(call(ctx, 0x000F6118u, {load<u32>(c + 32)}));  // Obj_FindOwned73_000F6118
    if (o == 0) {
        const f64 f = Lf(c + 28);
        ctx.fpr[1] = f;
        if (!(f > Lf(0x000A28C0u))) {
            ctx.gpr[3] = 0x000A28C0u;
            return;
        }
    }
    const u32 p = static_cast<u32>(call(ctx, SmallAlloc28, {124}));
    if (p != 0) {
        const u32 n = load<u32>(c + 132);
        callf(ctx, 0x004342DCu, {p, load<u32>(c + 24), Sx32c(n), o, c + 56, load<u32>(c + 124), load<u32>(c + 44), load<u32>(c + 48), load<u8>(c + 40), load<u8>(c + 41), load<u32>(c + 52), load<u32>(c + 128), Sx32c(n)}, {Lf(c + 28)});
    }
    call(ctx, kPush, {p});
}

// u32 f(Cmd* c, Effect* x): queues a VtB21280 command (28 bytes,
// VtB21280_Ctor_004386C4(p, t, c->+24, u8 c->+34, +35, flag, u8 c->+36..
// +40)) for the target t: the one bound as c->+20 (Effect_Bind4BB7E0
// create 0), else Obj_FindByHash_000DB290(Obj_FindById(c->+16), c->+12),
// else Obj_FindById(c->+12). flag = u8 c->+32, or whether the kind-2
// component c->+28's value is non-zero. r3 as the original leaves it.
DECOMP(000A4BB8, AnimCmd_QueueVtB21280_000A4BB8) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 a = load<u32>(c + 12);
    if (a == 0 && load<u32>(c + 20) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    u32 t;
    if (const u32 k = load<u32>(c + 20); k != 0) {
        const u32 b = static_cast<u32>(call(ctx, Bind29, {x, k, 0}));
        if (b == 0) return;
        t = load<u32>(b + 276) == kTarget29 ? load<u32>(b + 280) : 0;
        if (t == 0) return;  // r3 = the record
    } else {
        if (a == 0) return;  // unreachable
        if (const u32 id = load<u32>(c + 16); id != 0) {
            t = static_cast<u32>(call(ctx, FindById29, {id}));
            if (t == 0) return;
            t = static_cast<u32>(call(ctx, FindByHash29, {t, load<u32>(c + 12)}));
        } else {
            t = static_cast<u32>(call(ctx, FindById29, {a}));
        }
        if (t == 0) return;
    }
    u32 flag = load<u8>(c + 32);
    if (const u32 k = load<u32>(c + 28); k != 0) {
        const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 2) flag = static_cast<u32>(VCall(ctx, e, 24, {e})) != 0 ? 1u : 0u;
    }
    const u32 p = static_cast<u32>(call(ctx, SmallAlloc28, {28}));
    if (p != 0)
        call(ctx, 0x004386C4u, {p, t, load<u32>(c + 24), load<u8>(c + 34), load<u8>(c + 35), flag, load<u8>(c + 36), load<u8>(c + 37), load<u8>(c + 38), load<u8>(c + 39), load<u8>(c + 40)});  // VtB21280_Ctor_004386C4
    call(ctx, kPush, {p});
}

// u32 f(Cmd* c, Effect* x): queues an Obj_InitNamed_B21388_00439E4C command
// (160 bytes) for the save profile of h (c->+80, or the kind-2 component
// c->+84's value; Hash_ToString(h) when Save_Profile0C9FC8 finds none):
// (p, prof, c + 12, pt, c->+96, c->+100, c + 112, c + 128, u8 c->+144..
// +149) with pt the point map entry (PointMap_Find(*0x00F8F154 ->+16, &m))
// of m = c->+88 or the kind-2 component c->+92's value (Hash_ToString(m)
// when empty), else 0. r3 as the original leaves it.
DECOMP(000A67E4, AnimCmd_QueueNamedB21388_000A67E4) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    u32 h = load<u32>(c + 80);
    if (const u32 k = load<u32>(c + 84); k != 0) {  // r3 stays c without a key
        const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 2) h = static_cast<u32>(VCall(ctx, e, 24, {e}));
    }
    if (h == 0) return;
    const u32 prof = static_cast<u32>(call(ctx, 0x000C9FC8u, {h}));  // Save_Profile0C9FC8
    if (prof == 0) {
        call(ctx, HashStr29, {h});
        return;
    }
    const u32 m = Kind2Val29(ctx, x, load<u32>(c + 92), load<u32>(c + 88));
    u32 pt = 0;
    if (m != 0) {
        Stack frame(ctx, 16);  // the key at 160(r1)
        const u32 key = frame.addr();
        store<u32>(key, m);
        const u32 r = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), key}));  // PointMap_Find
        pt = load<u32>(r);
        if (pt == 0) call(ctx, HashStr29, {m});
    }
    const u32 p = static_cast<u32>(call(ctx, SmallAlloc28, {160}));
    if (p != 0)
        call(ctx, 0x00439E4Cu, {p, prof, c + 12, pt, load<u32>(c + 96), load<u32>(c + 100), c + 112, c + 128, load<u8>(c + 144), load<u8>(c + 145), load<u8>(c + 146), load<u8>(c + 147), load<u8>(c + 148), load<u8>(c + 149)});
    call(ctx, kPush, {p});
}

// void f(Obj* o): o->+308 = 2, o->+320 = 0; Obj_ResetPos_0026D9D4 of the
// player's (slot word *0x00F65CA0 + 4216, used with bit 2 of +192, else a
// null call as in the original) vt[64](); with o->+428 its vt[272]()'s
// components 0xF9E58103 / 0x65A11202 / 0xD15CA301 / 0xB6B2CE06 /
// 0x226E5F05 (their +28, 0 when missing) appended to o->+352 (count
// +320). Then o->+60->+348 = min(+344, +344 * *0x0005E16C) when that is
// above *0x0005E170, else that constant; vt[220](o, 0). r3 / f1 as the
// original leaves them.
DECOMP(0005E178, Obj_GatherComps_0005E178) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 308, 2);
    store<u32>(o + 320, 0);
    const u32 pl = PlayerSlot29(0);
    const u32 t = pl != 0 && (load<u32>(pl + 192) & 2u) != 0 ? pl : 0;
    const u32 r = static_cast<u32>(VCall(ctx, t, 64, {t}));
    call(ctx, 0x0026D9D4u, {r});  // Obj_ResetPos_0026D9D4
    if (const u32 e = load<u32>(o + 428); e != 0) {
        const u32 s = static_cast<u32>(VCall(ctx, e, 272, {e}));
        for (u32 h : {0xF9E58103u, 0x65A11202u, 0xD15CA301u, 0xB6B2CE06u, 0x226E5F05u}) {
            const u32 f = static_cast<u32>(call(ctx, FindComp29, {s, h}));
            const u32 v = f != 0 ? load<u32>(f + 28) : 0;
            const u32 n = load<u32>(o + 320);
            store<u32>(o + 320, n + 1);
            store<u32>(o + 352 + ((n << 2) & 0xFFFFFFFCu), v);
        }
    }
    const u32 q = load<u32>(o + 60);
    const f64 f1 = Lf(q + 344);
    const f64 f2 = Sgl28(f1 * Lf(0x0005E16Cu));
    const f64 m = f2 < f1 ? f2 : f1;
    if (m > Lf(0x0005E170u)) {
        store<u32>(q + 348, Bits28(m));
        ctx.fpr[1] = m;
    } else {
        store<u32>(q + 348, load<u32>(0x0005E170u));
        ctx.fpr[1] = f1;
    }
    VCall(ctx, o, 220, {o, 0});
}

// void f(Obj* o, Obj* a, f32 p[3], f32 d[3]): syncs the components of
// o->+356: kind-2 0x28314D16 = (a != 0); kind-2 0xC493B59D = a's def
// (+216) +52 (0 without a def; untouched without a); kind-1 0xE00B1920 =
// acosf(p[1] clamped to the constants at 0x000F0B20) * *0x000F0B40;
// kind-1 0x76CA7662 = f_000F0298(o, &out (360 preset), p + d, p)'s out.
// r3 / f1 as the original leaves them.
DECOMP(000F0B48, Obj_SyncComps_000F0B48) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    const u32 p = arg<u32>(ctx, 2);
    const u32 d = arg<u32>(ctx, 3);
    if (load<u32>(o + 356) == 0) return;
    if (const u32 e = KindOf29(ctx, o, 0x28314D16u, 2); e != 0) store<u32>(e + 20, a != 0 ? 1u : 0u);
    if (const u32 e = KindOf29(ctx, o, 0xC493B59Du, 2); e != 0 && a != 0) {
        const u32 df = load<u32>(a + 216);
        store<u32>(e + 20, df != 0 ? load<u32>(df + 52) : 0);
    }
    if (const u32 e = KindOf29(ctx, o, 0xE00B1920u, 1); e != 0) {
        constexpr u32 kB = 0x000F0B20u;
        const f64 one = Lf(kB), f3 = Ld29(kB + 8), f5 = Lf(kB + 16), f6 = Ld29(kB + 24);
        const f64 y = Lf(p + 4);
        f64 v = Sgl28(Sgl28(one - y) >= 0.0 ? y : f3);
        v = Sgl28(Sgl28(v - f5) >= 0.0 ? v : f6);
        v = Sgl28(Sgl28(v + one) >= 0.0 ? v : f6);
        v = Sgl28(Sgl28(v - one) >= 0.0 ? f3 : v);
        callf(ctx, 0x005E649Cu, {}, {v});  // acosf
        const f64 r = Sgl28(ctx.fpr[1] * Lf(kB + 32));
        store<u32>(e + 20, Bits28(r));
        ctx.fpr[1] = r;
    }
    const u32 e4 = static_cast<u32>(call(ctx, FindComp29, {load<u32>(o + 356), 0x76CA7662u}));
    if (e4 == 0) return;
    if (static_cast<u32>(VCall(ctx, e4, 32, {e4})) != 1) return;
    Stack frame(ctx, 32);  // the point at 112(r1), out at 128(r1)
    const u32 s = frame.addr(), out = s + 16;
    for (u32 i = 0; i < 3; ++i) store<u32>(s + 4 * i, Bits28(Lf(d + 4 * i) + Lf(p + 4 * i)));
    store<u32>(out, 0x43B40000u);
    call(ctx, 0x000F0298u, {o, out, s, p});
    const u32 w = load<u32>(out);
    store<u32>(e4 + 20, w);
    ctx.fpr[1] = Lf(out);
}
