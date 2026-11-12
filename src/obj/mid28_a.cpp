// Game code, medium sweep part 28: the callees part 27 left behind (the
// bank record reader, the section record reset, the 0x00B1D070 object's
// update and its charge-shot effect toggle).
#include "mid28.h"

using namespace sly4;
using namespace sly4::mid28;

namespace {
using namespace m28fn;
constexpr u32 kScale = 0x010AC930u;  // f32 global scale applied to the bank record's lengths
constexpr u32 kW = 0x00F65CA0u;      // the world pointer
}  // namespace

// u32 f(Array* a (unused), Rec* rec, IffReader* r, s32 version): reads one
// 112-byte bank record (from Bank_FillPairs_00441178). Locals (the
// original's 112(r1)..215(r1)): nameHash, [version < 3] volume (read and
// dropped), the inner / outer radii, the two box corners, [version < 2] a
// dropped f32, two bytes, [version >= 1] the fade curve byte; version >= 4
// re-reads the outer radius and reads five more f32 and a byte, else the
// curve picks +16 / +24 (1: 2.0, 2: 0.5, else 1.0). Version < 6 first
// skips 32 bytes. rec == 0 returns after the reads. Else the record is
// filled, its lengths scaled by *0x010AC930, +40 = max(|min|, |max|, +12)
// times +20 when that is > 1. r3 / f1 as the original leaves them.
DECOMP(00440CD8, Bank_ReadRecord_00440CD8) {
    const u32 rec = arg<u32>(ctx, 1);
    const u32 r = arg<u32>(ctx, 2);
    const s32 ver = arg<s32>(ctx, 3);
    Stack frame(ctx, 112);  // the original's 112(r1)..223(r1)
    const u32 L = frame.addr() - 112;
    constexpr u32 kOne = 0x3F800000u;
    store<u32>(L + 128, kOne);
    store<u32>(L + 132, kOne);
    store<u32>(L + 136, kOne);
    store<u32>(L + 144, kOne);
    store<u32>(L + 140, 0x46BB7E00u);
    store<u8>(L + 182, 0);
    store<u32>(L + 148, kOne);
    store<u8>(L + 183, 0);
    u32 r3 = static_cast<u32>(call(ctx, IffReadU32, {r, L + 112, 0x00AE44D8u}));
    if (ver < 6)
        for (u32 i = 0; i < 32; ++i) call(ctx, ReadByte, {r, L + 184 + i, 0});
    if (ver < 3) call(ctx, ReadF32, {r, L + 116, 0x00AE44E8u});
    call(ctx, ReadF32, {r, L + 120, 0x00AE44F0u});
    call(ctx, ReadF32, {r, L + 124, 0x00AE4508u});
    call(ctx, ReadVec3, {r, L + 156, 0x00AE4520u});
    call(ctx, ReadVec3, {r, L + 168, 0x00AE4530u});
    if (ver < 2) {
        store<u32>(L + 152, 0);
        call(ctx, ReadF32, {r, L + 152, 0x00AE4540u});
    }
    call(ctx, ReadU8, {r, L + 180, 0x00AE4550u});
    r3 = static_cast<u32>(call(ctx, ReadU8, {r, L + 181, 0x00AE4560u}));
    if (ver >= 1) r3 = static_cast<u32>(call(ctx, ReadU8, {r, L + 183, 0x00AE4568u}));
    f64 f1;
    if (ver < 4) {
        const u32 c = load<u8>(L + 183);
        r3 = c;
        if (c == 1) {
            r3 = 0x40000000u;
            store<u32>(L + 128, r3);
        } else if (c == 2) {
            r3 = 0x3F000000u;
            store<u32>(L + 128, r3);
        }
        store<u32>(L + 136, load<u32>(L + 128));
        f1 = Lf(L + 128);
    } else {
        call(ctx, ReadF32, {r, L + 124, 0x00AE4508u});
        call(ctx, ReadF32, {r, L + 128, 0x00AE4578u});
        call(ctx, ReadF32, {r, L + 132, 0x00AE4588u});
        call(ctx, ReadF32, {r, L + 136, 0x00AE4598u});
        call(ctx, ReadF32, {r, L + 140, 0x00AE45A8u});
        call(ctx, ReadF32, {r, L + 144, 0x00AE45C0u});
        call(ctx, ReadU8, {r, L + 182, 0x00AE45D0u});
        r3 = static_cast<u32>(call(ctx, ReadF32, {r, L + 148, 0x00AE45E8u}));
        f1 = Lf(L + 136);
    }
    if (rec == 0) {
        ctx.fpr[1] = f1;
        ctx.gpr[3] = r3;
        return;
    }
    store<u32>(rec + 0, load<u32>(L + 112));
    store<u32>(rec + 8, load<u32>(L + 120));
    store<u32>(rec + 16, load<u32>(L + 128));
    store<u32>(rec + 12, load<u32>(L + 124));
    store<u32>(rec + 20, ver < 5 ? kOne : load<u32>(L + 132));
    store<u32>(rec + 24, load<u32>(L + 136));
    store<u32>(rec + 28, load<u32>(L + 140));
    store<u32>(rec + 32, load<u32>(L + 144));
    store<u32>(rec + 48, load<u32>(L + 156));
    store<u32>(rec + 52, load<u32>(L + 160));
    store<u32>(rec + 56, load<u32>(L + 164));
    store<u32>(rec + 64, load<u32>(L + 168));
    store<u32>(rec + 68, load<u32>(L + 172));
    store<u32>(rec + 72, load<u32>(L + 176));
    store<u32>(rec + 36, load<u32>(L + 148));
    const f64 s = Lf(kScale);
    const f64 ay = Sgl28(Lf(rec + 52) * s), by = Sgl28(Lf(rec + 68) * s);
    const f64 ax = Sgl28(Lf(rec + 48) * s), bx = Sgl28(Lf(rec + 64) * s);
    const f64 az = Sgl28(Lf(rec + 56) * s), bz = Sgl28(Lf(rec + 72) * s);
    f64 sa = Sgl28(ay * ay), sb = Sgl28(by * by);
    sa = Sgl28(std::fma(ax, ax, sa));
    sb = Sgl28(std::fma(bx, bx, sb));
    sa = Sgl28(std::fma(az, az, sa));
    sb = Sgl28(std::fma(bz, bz, sb));
    const f64 la = Sgl28(std::sqrt(sa));
    const f64 g = Lf(rec + 20);
    store<u32>(rec + 48, Bits28(ax));
    store<u32>(rec + 52, Bits28(ay));
    store<u32>(rec + 56, Bits28(az));
    store<u32>(rec + 64, Bits28(bx));
    store<u32>(rec + 68, Bits28(by));
    store<u32>(rec + 72, Bits28(bz));
    const f64 lb = Sgl28(std::sqrt(sb));
    const f64 r12 = Sgl28(Lf(rec + 12) * s), r8 = Sgl28(Lf(rec + 8) * s);
    const u32 b0 = load<u8>(L + 180) != 0, b1 = load<u8>(L + 181) != 0, b2 = load<u8>(L + 182) != 0;
    store<u8>(rec + 81, static_cast<u8>(b1));
    store<u8>(rec + 80, static_cast<u8>(b0));
    store<u8>(rec + 82, static_cast<u8>(b2));
    store<u32>(rec + 12, Bits28(r12));
    store<u32>(rec + 8, Bits28(r8));
    f64 m = Fsel28(Sgl28(la - lb), la, lb);
    m = Fsel28(Sgl28(r12 - m), r12, m);
    store<u32>(rec + 40, Bits28(m));
    if (g > 1.0) {
        m = Sgl28(m * g);
        store<u32>(rec + 40, Bits28(m));
    }
    ctx.fpr[1] = m;
    ctx.gpr[3] = b0;
}

// u32 f(Rec* o) (from Section_Reset_0033CE20 mode 4): Obj_ClearLists(o);
// o->+68 = the section found under key o->+32 in the sorted map at
// *0x00F65BE8 + 28 (0: returns the find result). Else its sets are
// switched off (Obj_ForEachSet(s, 0)), s->+60 / +52 = o with the
// callbacks 0x00F37068 (+56) / 0x00F37070 (+48), each node of its list
// +32 (next +72) is pushed to the vector +132, then while i < the
// capacity of the vector +92 (re-read) a zeroed 12-byte SmallAlloc block
// (or 0) is pushed to it; then f_0033C2C0(o, 2) (r3 = its).
DECOMP(0033C5B4, Section_RecordReset_0033C5B4) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0033C03Cu, {o});  // Obj_ClearLists_0033C03C
    Stack frame(ctx, 16);  // the key at 112(r1)
    const u32 key = frame.addr();
    store<u32>(key, load<u32>(o + 32));
    const u32 slot = static_cast<u32>(call(ctx, 0x0073B608u, {load<u32>(0x00F65BE8u) + 28, key}));  // SortedMap_Find73B608
    const u32 s = load<u32>(slot);
    store<u32>(o + 68, s);
    if (s == 0) {
        ctx.gpr[3] = slot;
        return;
    }
    call(ctx, 0x004562F4u, {s, 0});  // Obj_ForEachSet_004562F4
    const u32 c = load<u32>(o + 68);
    u32 e = load<u32>(c + 32);
    store<u32>(c + 60, o);
    store<u32>(c + 56, 0x00F37068u);
    store<u32>(c + 48, 0x00F37070u);
    store<u32>(c + 52, o);
    while (e != 0) {
        VecPush28(ctx, o, 132, e);
        e = load<u32>(e + 72);
    }
    for (u32 i = 0; i < load<u16>(o + 96); ++i) {
        const u32 p = static_cast<u32>(call(ctx, SmallAlloc28, {12}));
        if (p != 0) {
            store<u32>(p + 0, 0);
            store<u32>(p + 4, 0);
            store<u8>(p + 8, 0);
        }
        VecPush28(ctx, o, 92, p);
    }
    call(ctx, 0x0033C2C0u, {o, 2});
}

// u32 f(T* o) (the 0x00B1D070 object, see Obj_Dtor_B1D070_002C9910): with
// a target o->+156 and both u8 +336 / +290 set: first time (u8 +289 == 0)
// Obj_SetAnim(t, 0, 1, -, 0, 0, 0x01164CA4; f1 = *0x002C96A0), the
// effect t->+20 gets hash 0xF7BC9E12 (u8 +288 set) or 0xD66F128B, the
// world's +0x12816C gets Call_0052296C(w, v, {0.5, 0.5}) with v = the
// vector 0x01164B40, t->+24 (if any) takes v.xy (+72 stored back as read)
// and WorldObj_MarkDirty, u8 +289 = 1 (r3 = 1). Later: Obj_LineOfSight(o,
// o->+296, 3, 2; f1 = *0x002C969C) toggles u8 +291 with the hash
// 0xF7BC9E12 (+288 set and visible) / 0xD66F128B. Without +336 / +290 a
// started +289 is ended: Obj_Dispatch362(t, 1, 1, 0), +289 = 0. r3 / f1 as
// the original leaves them.
DECOMP(002C96A8, Obj_Update_B1D070_002C96A8) {
    const u32 o = arg<u32>(ctx, 0);
    u32 t = load<u32>(o + 156);
    if (t == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u8>(o + 336) == 0 || load<u8>(o + 290) == 0) {
        if (load<u8>(o + 289) != 0) {
            call(ctx, 0x0036325Cu, {t, 1, 1, 0});  // Obj_Dispatch362
            store<u8>(o + 289, 0);
        }
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u8>(o + 289) != 0) {
        const u32 tgt = load<u32>(o + 296);
        callf(ctx, 0x002C2780u, {o, tgt, 3, 2, tgt}, {Lf(0x002C969Cu)});  // Obj_LineOfSight
        const u32 vis = static_cast<u32>(ctx.gpr[3]);
        const u32 on = load<u8>(o + 291), b288 = load<u8>(o + 288);
        t = load<u32>(o + 156);
        u32 r3 = t;
        if (on == 0 && b288 != 0 && vis != 0) {
            if (const u32 e = load<u32>(t + 20); e != 0) call(ctx, SetHash, {e, 0xF7BC9E12u});
            store<u8>(o + 291, 1);
            r3 = 1;
        } else if (on != 0 && (b288 == 0 || vis == 0)) {
            if (const u32 e = load<u32>(t + 20); e != 0) call(ctx, SetHash, {e, 0xD66F128Bu});
            store<u8>(o + 291, 0);
            r3 = 0;
        }
        ctx.gpr[3] = r3;
        return;
    }
    // r6 = 0x002C969C (the constant's base) and r9 = 0x01164CA4 as the
    // original sets them.
    callf(ctx, 0x00363A3Cu, {t, 0, 1, 0x002C969Cu, 0, 0, 0x01164CA4u}, {Lf(0x002C96A0u)});  // Obj_SetAnim
    if (const u32 e = load<u32>(load<u32>(o + 156) + 20); e != 0)
        call(ctx, SetHash, {e, load<u8>(o + 288) != 0 ? 0xF7BC9E12u : 0xD66F128Bu});
    {
        Stack frame(ctx, 32);  // v at 112(r1), {0.5, 0.5} at 128(r1)
        const u32 v = frame.addr(), h = v + 16;
        constexpr u32 kV = 0x01164B40u;
        store<u32>(v, load<u32>(kV));
        store<u32>(v + 4, load<u32>(kV + 4));
        store<u32>(v + 8, load<u32>(kV + 8));
        store<u32>(h, 0x3F000000u);
        store<u32>(h + 4, 0x3F000000u);
        const u32 w = load<u32>(kW);
        // r6..r8 and f1 / f2 as the original leaves them (r6 = the vector's
        // address, r7 = 0.5f, r8 = the world; f1 / f2 = its y / z).
        ctx.fpr[1] = Lf(kV + 4);
        ctx.fpr[2] = Lf(kV + 8);
        call(ctx, 0x0052296Cu, {load<u32>(w + 0x0012816Cu), v, h, kV, 0x3F000000u, w});
        if (const u32 q = load<u32>(load<u32>(o + 156) + 24); q != 0) {
            const u32 x = load<u32>(v), y = load<u32>(v + 4), z = load<u32>(q + 72);
            store<u32>(q + 64, x);
            store<u32>(q + 68, y);
            store<u32>(q + 72, z);
            ctx.fpr[1] = Lf(v);  // f1..f3 as the original leaves them
            ctx.fpr[2] = Lf(v + 4);
            ctx.fpr[3] = Lf(q + 72);
            call(ctx, 0x0057D000u, {q, q, q + 64});  // WorldObj_MarkDirty
        }
    }
    store<u8>(o + 289, 1);
    ctx.gpr[3] = 1;
}

// u32 f(T* o, u8 on) (the 0x00B1D070 object): off: the effect o->+328 (if
// any) gets Effect_Release(e, 0x00010CBB) (r3 = o when there is none). On
// with no effect: Set_Find(0) and, for the name
// "%s_ChargeShotLifetime" % (o->+276->+216 + 196) hashed, the set's
// binding tree (root **+16, path +20, depth +100, node +104, value
// 0x011DE288) gives the effect def: o->+328 = Obj_CreateEffectDCA20(o->+276,
// def, 0, 1); its +136 bit 0x200 cleared, the component 0x25431D90 with
// vt[32]() == 2 takes (o->+276 vt[64]()->+428 >> 16) at +20 when vt[64]()
// is set, then hash 0x04B6FB5B. Finally an effect o->+328 gets
// Effect_Release(e, 0xBE54C60E). r3 as the original leaves it.
DECOMP(002C2420, Obj_ChargeShotFx_002C2420) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    if (on == 0) {
        const u32 e = load<u32>(o + 328);
        if (e != 0) call(ctx, EffRelease, {e, 0x00010CBBu});
        else ctx.gpr[3] = o;
        return;
    }
    if (load<u32>(o + 328) == 0) {
        const u32 set = static_cast<u32>(call(ctx, 0x004BCE90u, {0}));  // Set_Find
        if (set != 0) {
            const u32 str = static_cast<u32>(call(ctx, 0x005534D0u, {0x00AD5578u, load<u32>(load<u32>(o + 276) + 216) + 196}));  // String_Format
            const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {str}));  // String_HashNoCase
            if (TreeFind28(set, h, 16, 20, 100, 104, 0x011DE288u)) {
                const u32 node = load<u32>(set + 104);
                const u32 owner = load<u32>(o + 276);
                u32 def;
                if (node != 0 && (def = load<u32>(node + 8)) != 0) {
                    const u32 e = static_cast<u32>(call(ctx, 0x000DCA20u, {owner, def, 0, 1}));  // Obj_CreateEffectDCA20
                    store<u32>(o + 328, e);
                    if (e != 0) {
                        store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
                        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {load<u32>(o + 328), 0x25431D90u}));  // Effect_FindComponent
                        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 2) {
                            const u32 ow = load<u32>(o + 276);
                            if (VCall(ctx, ow, 64, {ow}) != 0) {
                                const u32 ow2 = load<u32>(o + 276);
                                const u32 x = static_cast<u32>(VCall(ctx, ow2, 64, {ow2}));
                                store<u32>(c + 20, load<u32>(x + 428) >> 16);
                            }
                        }
                        call(ctx, SetHash, {load<u32>(o + 328), 0x04B6FB5Bu});
                    }
                }
            }
        }
        const u32 e = load<u32>(o + 328);
        if (e == 0) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    call(ctx, EffRelease, {load<u32>(o + 328), 0xBE54C60Eu});
}

namespace {
// The candidate list of o (o->+292->+372: {.., +16 entries (8 bytes,
// object first), +36 count}): the first non-null entry after `cur` (any
// entry when cur == 0; the list re-read), or 0.
u32 NextCandidate28(u32 o, u32 cur) {
    const u32 l = load<u32>(load<u32>(o + 292) + 372);
    const u32 n = load<u32>(l + 36);
    bool seen = cur == 0;
    for (u32 j = 0; j < n; ++j) {
        const u32 e = load<u32>(load<u32>(l + 16) + 8 * j);
        if (seen && e != 0) return e;
        if (e == cur) seen = true;
    }
    return 0;
}
}  // namespace

// Obj* f(T* o, f32 maxAngle) (the 0x00B1D070 object): with the world
// camera (+0x1281E0; none: 0) the candidates of o->+292->+372 are tried
// in order: flags +192 must have 0x8 / 0x4 / 0x20 / 0x2000, not 0x20 with
// u16 +432 bit 0x80, not 0x2000 with Obj_FlagVia set; the point vt[284]()
// must be at least sqrt(1e-5) from the camera, Obj_LineOfSight(o, e, 4, 2;
// -1) must hold, and with maxAngle > 0 the angle between the camera's
// forward (+128 row, normalised) and the direction must be below it
// (acosf of the clamped dot). Returns the first that passes, or 0. f1 as
// the original leaves it.
DECOMP(002C53E0, Obj_FindTarget_002C53E0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 maxA = ctx.fpr[1];
    constexpr u32 kK = 0x002C53C0u;  // {1.0f, 1e-5f, -1.0f, 0.0f, 1.0, -1.0}
    const u32 cam = load<u32>(load<u32>(kW) + 0x001281E0u);
    if (cam == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 m = cam + 96;
    const f64 one = Lf(kK);
    f64 fy = Lf(m + 36), fx = Lf(m + 32), fz = Lf(m + 40);
    const f64 px = Lf(m + 48), py = Lf(m + 52), pz = Lf(m + 56);
    f64 s = Sgl28(fy * fy);
    s = Sgl28(std::fma(fx, fx, s));
    s = Sgl28(std::fma(fz, fz, s));
    const f64 inv = Sgl28(one / Sgl28(std::sqrt(s)));
    ctx.fpr[1] = inv;
    fx = Sgl28(inv * fx);
    fy = Sgl28(inv * fy);
    fz = Sgl28(inv * fz);
    Stack frame(ctx, 16);  // the candidate's point at 112(r1)
    const u32 pt = frame.addr();
    u32 e = NextCandidate28(o, 0);
    while (e != 0) {
        const u32 f = load<u32>(e + 192);
        bool ok = (f & 0xCu) != 0 || (f & 0x20u) != 0 || (f & 0x2000u) != 0;
        if (ok && (f & 0x20u) != 0 && ((load<u16>(e + 432) >> 7) & 1u) != 0) ok = false;
        if (ok && (f & 0x2000u) != 0 && call(ctx, 0x000EFFC8u, {e}) != 0) ok = false;  // Obj_FlagVia_000EFFC8
        if (ok) {
            VCall(ctx, e, 284, {pt, e});
            const f64 dy = Sgl28(Lf(pt + 4) - py), dx = Sgl28(Lf(pt) - px);
            const f64 dz = Sgl28(Lf(pt + 8) - pz);
            ctx.fpr[1] = Lf(kK + 4);
            f64 d2 = Sgl28(dy * dy);
            d2 = Sgl28(std::fma(dx, dx, d2));
            d2 = Sgl28(std::fma(dz, dz, d2));
            if (!(std::fabs(d2) < ctx.fpr[1])) {
                const f64 neg = Lf(kK + 8);
                callf(ctx, 0x002C2780u, {o, e, 4, 2}, {neg});  // Obj_LineOfSight
                if (static_cast<u32>(ctx.gpr[3]) != 0) {
                    ctx.fpr[1] = Lf(kK + 12);
                    if (!(maxA > ctx.fpr[1])) break;
                    const f64 dOne = std::bit_cast<f64>(load<u64>(kK + 16)), dNeg = std::bit_cast<f64>(load<u64>(kK + 24));
                    const f64 iv = Sgl28(one / Sgl28(std::sqrt(d2)));
                    const f64 ny = Sgl28(iv * dy), nx = Sgl28(iv * dx), nz = Sgl28(iv * dz);
                    f64 dot = Sgl28(fy * ny);
                    dot = Sgl28(std::fma(fx, nx, dot));
                    dot = Sgl28(std::fma(fz, nz, dot));
                    f64 c = Sgl28(Fsel28(Sgl28(one - dot), dot, dOne));
                    c = Sgl28(Fsel28(Sgl28(c - neg), c, dNeg));
                    c = Sgl28(Fsel28(Sgl28(c + one), c, dNeg));
                    c = Sgl28(Fsel28(Sgl28(c - one), dOne, c));
                    const f64 a = callf(ctx, 0x005E649Cu, {}, {c});  // acosf
                    if (!(maxA < a || maxA == a)) break;
                }
            }
        }
        e = NextCandidate28(o, e);
    }
    ctx.gpr[3] = e;
}

// u32 f(T* o, u8 track) (the 0x00B1D070 object's aim start; once, while
// u8 +336 is clear): facing +208 = the vector 0x01164B70, the old target
// +296 released (vt[40](t, 0)), +288 = 0. With the world camera and an
// owner +276 whose +184 bit 0 is set: Obj_SetFacing2C5140 toward the
// camera's +128 row flattened (y = 0; the default vector when shorter
// than sqrt(1e-5)), a probe +280 created on first use (SmallAlloc(96),
// Probe_Construct_00107320(p, cam, owner)) and given the owner's +216
// view (+1312 rows) when it has one, Camera_PushTarget0FC040(cam, probe,
// -, 1; f1 = +140). With `track`: target +296 = Obj_FindTarget_002C53E0(o;
// -1) (u16 +178 counted, +288 = found), then twice: the probe's vt[48]
// query {matrix 0x01164BD0, 55, 7.5, .., bytes 0x01164CA8} (f1 = 0) and a
// turn toward the target's vt[284]() point. Finally the aim state +304..
// +319 cleared, a pending charge shot (+320) cancelled
// (Obj_ChargeShotFx_002C2420(o, 0)), +336 = +337 = 1 (r3 = 1).
DECOMP(002C5780, Obj_AimStart_002C5780) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 track = arg<u32>(ctx, 1);
    if (const u32 b = load<u8>(o + 336); b != 0) {
        ctx.gpr[3] = b;
        return;
    }
    constexpr u32 kDef = 0x01164B70u;  // default facing
    constexpr u32 kK = 0x002C5770u;    // {0.0f, 1e-5f, -1.0f}
    store<u32>(o + 208, load<u32>(kDef));
    store<u32>(o + 212, load<u32>(kDef + 4));
    store<u32>(o + 216, load<u32>(kDef + 8));
    ctx.fpr[1] = Lf(kDef + 8);
    if (const u32 t = load<u32>(o + 296); t != 0) {
        VCall(ctx, t, 40, {t, 0});
        store<u32>(o + 296, 0);
        store<u8>(o + 288, 0);
    }
    Stack frame(ctx, 160);  // the original's 112(r1)..263(r1)
    const u32 L = frame.addr() - 112;
    const u32 cam = load<u32>(load<u32>(kW) + 0x001281E0u);
    const u32 owner = load<u32>(o + 276);
    if (cam != 0 && owner != 0 && (load<u32>(owner + 188) & 1u) != 0) {
        const u32 v = L + 112;
        store<u32>(v, load<u32>(cam + 128));
        store<u32>(v + 4, load<u32>(cam + 132));
        store<u32>(v + 8, load<u32>(cam + 136));
        const f64 x = Lf(cam + 128), z = Lf(cam + 136);
        store<u32>(v + 4, 0);
        const f64 zero = Lf(kK), eps = Lf(kK + 4);
        f64 s = Sgl28(zero * zero);
        s = Sgl28(std::fma(x, x, s));
        s = Sgl28(std::fma(z, z, s));
        ctx.fpr[1] = Lf(kDef);
        if (std::fabs(s) < eps) {
            store<u32>(v, load<u32>(kDef));
            store<u32>(v + 4, load<u32>(kDef + 4));
            store<u32>(v + 8, load<u32>(kDef + 8));
            ctx.fpr[1] = Lf(kDef + 8);
        }
        call(ctx, 0x002C5140u, {o, v});  // Obj_SetFacing2C5140
        u32 p = load<u32>(o + 280);
        if (p == 0) {
            const u32 q = static_cast<u32>(call(ctx, SmallAlloc28, {96}));
            if (q != 0) call(ctx, 0x00107320u, {q, cam, load<u32>(o + 276)});  // Probe_Construct_00107320
            p = q;
            store<u32>(o + 280, p);
        }
        if (const u32 view = load<u32>(load<u32>(o + 276) + 216); view != 0) {
            // Quirk: written through a null probe as well.
            const u32 r = view + 1312;
            store<u32>(p + 16, load<u32>(r));
            store<u32>(p + 20, load<u32>(r + 4));
            store<u32>(p + 24, load<u32>(r + 8));
            store<u32>(p + 32, load<u32>(r + 16));
            store<u32>(p + 36, load<u32>(r + 20));
            store<u32>(p + 40, load<u32>(r + 24));
            ctx.fpr[1] = Lf(r + 32);
            store<u32>(p + 48, load<u32>(r + 32));
            p = load<u32>(o + 280);
        }
        callf(ctx, 0x000FC040u, {cam, p, 0, 1}, {Lf(o + 140)});  // Camera_PushTarget0FC040 (r5 unread)
        if ((track & 0xFFu) != 0) {
            const u32 e = static_cast<u32>(callf(ctx, 0x002C53E0u, {o}, {Lf(kK + 8)}), ctx.gpr[3]);  // Obj_FindTarget_002C53E0
            u32 t = load<u32>(o + 296);
            if (t != e) {
                if (t != 0) VCall(ctx, t, 40, {t, 0});
                store<u32>(o + 296, e);
                t = e;
                if (e != 0) {
                    store<u16>(e + 178, static_cast<u16>(load<u16>(e + 178) + 1));
                    t = load<u32>(o + 296);
                }
                store<u8>(o + 288, t != 0);
            }
            if (t != 0) {
                constexpr u32 kM = 0x01164BD0u, kB = 0x01164CA8u;
                const u32 S = L + 160, pt = L + 128, d = L + 144;
                for (u32 k = 0; k < 2; ++k) {
                    for (u32 row = 0; row < 4; ++row)
                        for (u32 c = 0; c < 3; ++c) store<u32>(S + 16 * row + 4 * c, load<u32>(kM + 16 * row + 4 * c));
                    store<u32>(S + 64, 0x425C0000u);
                    store<u32>(S + 68, 0x40F00000u);
                    store<u8>(S + 72, 0);
                    store<u32>(S + 76, 0);
                    store<u32>(S + 80, 0);
                    store<u32>(S + 84, 0);
                    store<u8>(S + 88, 0);
                    store<u32>(S + 92, 0);
                    for (u32 j = 0; j < 4; ++j) store<u8>(S + 96 + j, load<u8>(kB + j));
                    const u32 pr = load<u32>(o + 280);
                    const u32 opd = load<u32>(load<u32>(pr) + 48);
                    ctx.fpr[1] = Lf(kK);
                    call_ptr(ctx, opd, {pr, opd, S, load<u32>(opd)});  // probe vt[48](p, -, &query; f1 = 0)
                    const u32 tg = load<u32>(o + 296);
                    VCall(ctx, tg, 284, {pt, tg});
                    const f64 dy = Sgl28(Lf(pt + 4) - Lf(S + 52)), dx = Sgl28(Lf(pt) - Lf(S + 48));
                    const f64 dz = Sgl28(Lf(pt + 8) - Lf(S + 56));
                    store<u32>(d + 4, Bits28(dy));
                    store<u32>(d, Bits28(dx));
                    f64 s2 = Sgl28(std::fma(dx, dx, Sgl28(dy * dy)));
                    store<u32>(d + 8, Bits28(dz));
                    s2 = Sgl28(std::fma(dz, dz, s2));
                    ctx.fpr[1] = std::fabs(s2);
                    if (!(ctx.fpr[1] < Lf(kK + 4))) call(ctx, 0x002C5140u, {o, d});  // Obj_SetFacing2C5140
                }
            }
        }
    }
    store<u8>(o + 304, 0);
    store<u32>(o + 308, 0);
    const u32 pend = load<u8>(o + 320);
    store<u32>(o + 312, 0);
    store<u8>(o + 316, 0);
    store<u8>(o + 317, 0);
    store<u8>(o + 319, 0);
    if (pend != 0) {
        store<u8>(o + 320, 0);
        store<u32>(o + 324, 0);
        call(ctx, 0x002C2420u, {o, 0});  // Obj_ChargeShotFx_002C2420
    }
    store<u8>(o + 336, 1);
    store<u8>(o + 337, 1);
    ctx.gpr[3] = 1;
}
