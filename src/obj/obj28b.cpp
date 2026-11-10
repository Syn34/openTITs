// Game code, part 28 (continued): the small-function sweep from 44
// instructions up. See obj28.cpp for the conventions.
#include "obj28.h"

using namespace sly4;
using namespace sly4::obj28;

// ---------------------------------------------------------------- sweep: 44 instructions
// T::~T() deleting (vtable 0x00B0DDC0): the child +24 gets vt[12](), then
// (reloaded, when set) vt[4](); the base 0x002505A0(o), Mem_FreeFrom(o, 0)
DECOMP(00018018, VtB0DDC0_Delete_00018018) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 c = load<u32>(o + 24);
    store<u32>(o, 0x00B0DDC0);
    VCall(ctx, c, 12, {c});
    const u32 c2 = load<u32>(o + 24);
    if (c2 != 0) VCall(ctx, c2, 4, {c2});
    call(ctx, 0x002505A0, {o});
    ctx.gpr[3] = call(ctx, o25fn::Mem_FreeFrom, {o, 0});
}

// The update body shared by 0x00027C4C / 0x0002BCB4: unless the flags
// 0x40 / 0x20 of o->+48->+172 are set, 0x00012E70(o; t) and o->vt[40](o;
// t); returns false when it stopped (r3 = the word or 1 as the original)
static bool UpdateBody28(PPUContext& ctx, u32 o, f64 t) {
    const u32 w = load<u32>(load<u32>(o + 48) + 172);
    if ((w & 0x40u) != 0) {
        ctx.gpr[3] = w;
        return false;
    }
    if ((w & 0x20u) != 0) {
        ctx.gpr[3] = 1;
        return false;
    }
    callf(ctx, 0x00012E70, {o}, {t});
    ctx.fpr[1] = t;
    VCall(ctx, o, 40, {o});
    return true;
}

// void f(T* o, f32 t): UpdateBody28, then o->vt[228](o; t)
DECOMP(00027C4C, Obj_Update_00027C4C) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    if (!UpdateBody28(ctx, o, t)) return;
    ctx.fpr[1] = t;
    VCall(ctx, o, 228, {o});
}

// void f(T* o, f32 t): UpdateBody28 unless the flag 0x8000 of
// o->+352->+24->+164 is set
DECOMP(0002BCB4, Obj_Update_0002BCB4) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    const u32 a = load<u32>(o + 352);
    if (a != 0) {
        const u32 b = load<u32>(a + 24);
        if (b != 0 && (load<u32>(b + 164) & 0x8000u) != 0) {
            ctx.gpr[3] = load<u32>(o + 48);
            return;
        }
    }
    UpdateBody28(ctx, o, t);
}

// void Cmd::Run(): with the id +12 (or the kind u8 +20 == 9), the object
// Obj_FindById(id) with flag 0x2 of +192 gets 0x00369910(it, +16) when the
// u8 +21 is set, else 0x00369A80(it); without, for a clear +21,
// HashedName_Set(+20, 0x00AC4DD4)
DECOMP(0007D098, Cmd_ApplyToObj_0007D098) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 id = load<u32>(o + 12);
    if (id == 0) {
        const u32 k = load<u8>(o + 20);
        if (k != 9) {
            ctx.gpr[3] = load<u8>(o + 21);
            if (ctx.gpr[3] == 0) call(ctx, 0x0036960C, {k, 0x00AC4DD4});
            return;
        }
    }
    const u32 r = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
    ctx.gpr[3] = r;
    if (r == 0 || (load<u32>(r + 192) & 0x2u) == 0) return;
    if (load<u8>(o + 21) != 0) call(ctx, 0x00369910, {r, load<u32>(o + 16)});
    else call(ctx, 0x00369A80, {r});
}

// T* f(?, XmlCursor* c): new 16-byte record (base 0x004BB3B0(p, c), the
// f32 +12 = -1, vtable 0x00B101C0), +12 read from 0x048C90E5 (default
// *0x00087208)
DECOMP(00087210, NewXmlF32_00087210) { NewXmlF32Init28(ctx, 0x00B101C0, 0x048C90E5, 0x00087208, 0xBF800000u); }

// T* f(?, XmlCursor* c): as 0x00087210 with vtable 0x00B101E8, the hash
// 0x002644E0 and the default *0x00087340
DECOMP(00087348, NewXmlF32_00087348) { NewXmlF32Init28(ctx, 0x00B101E8, 0x002644E0, 0x00087340, 0xBF800000u); }

// T* f(?, XmlCursor* c): as 0x00087210 with vtable 0x00B10670 and the
// default *0x0008C348
DECOMP(0008C350, NewXmlF32_0008C350) { NewXmlF32Init28(ctx, 0x00B10670, 0x048C90E5, 0x0008C348, 0xBF800000u); }

// void Cmd::Run(): Music_SetNode449E48(the node +20, the u8s +24, +25)
// when the node and its first word are set; else, with the debug flag
// 0x010AEC60, warns (format 0x00AC52C4 and the names of +12 and +16)
DECOMP(0008C62C, Cmd_SetMusicNode_0008C62C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 20);
    const u32 t = load<u32>(o + 12);
    if (n != 0 && load<u32>(n) != 0) {
        call(ctx, 0x00449E48, {n, load<u8>(o + 24), load<u8>(o + 25)});
        return;
    }
    ctx.gpr[3] = t;
    if (load<u8>(0x010AEC60) == 0) return;
    const u32 a = static_cast<u32>(call(ctx, o27fn::Hash_ToString, {t}));
    const u32 b = static_cast<u32>(call(ctx, o27fn::Hash_ToString, {load<u32>(o + 16)}));
    call(ctx, o27fn::String_Format, {0x00AC52C4, a, b});
}

// void f(?, T* o, u32 hash): for 0x9535758E, +256 = 60.0f,
// o->vt[24](o; *0x00090CF8), +264 = 0.035f, +268 = 0.4f and the child +4
// gets +20 = -60.0f, +24 = 140.0f
DECOMP(00090D00, Obj_Setup9535_00090D00) {
    const u32 o = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0x9535758Eu;
    if (arg<u32>(ctx, 2) != 0x9535758Eu) return;
    const u32 vt = load<u32>(o);
    store<u32>(o + 256, 0x42700000u);
    ctx.fpr[1] = LfsAt(0x00090CF8);
    VCallVt(ctx, vt, 24, {o});
    store<u32>(o + 264, 0x3D0F5C29u);
    store<u32>(o + 268, 0x3ECCCCCDu);
    const u32 q = load<u32>(o + 4);
    ctx.gpr[3] = 0x3D0F5C29u;
    if (q != 0) {
        store<u32>(q + 20, 0xC2700000u);
        store<u32>(q + 24, 0x430C0000u);
        ctx.gpr[3] = 0xC2700000u;
    }
}

// bool f(T* o): true when neither group of o->+216's floats (+256, +240,
// +248, +268, +276 / +280..+292) is entirely *0x000E8250
DECOMP(000E8258, Obj_BothGroupsSet_000E8258) {
    const u32 q = load<u32>(arg<u32>(ctx, 0) + 216);
    const f64 k = LfsAt(0x000E8250);
    ctx.fpr[2] = k;
    bool a = true;
    for (u32 off : {256u, 240u, 248u, 268u, 276u}) {
        const f64 v = LfsAt(q + off);
        ctx.fpr[3] = v;
        if (v != k) {
            a = false;
            break;
        }
    }
    bool b = true;
    for (u32 off : {280u, 284u, 288u, 292u}) {
        const f64 v = LfsAt(q + off);
        ctx.fpr[1] = v;
        if (v != k) {
            b = false;
            break;
        }
    }
    ctx.gpr[3] = !a && !b ? 1 : 0;
}

// u32 f(T* o, Hit* h): when Obj_TestE0BCC(o, h) and o->+216's f32 +240 >
// *0x000E8400, r = o->vt[352](o, -, h->+4, 0x045D9554; h->+64 * that) and
// bit 0x100 of the u16 h->+14 = (r == 0) (r3 = cntlzw(r))
DECOMP(000E8408, Obj_ApplyHit_000E8408) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 216);
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, h})) == 0) return;
    const f64 v = LfsAt(q + 240);
    ctx.fpr[1] = v;
    if (!(v > LfsAt(0x000E8400))) {
        ctx.gpr[3] = 0x000E8400;
        return;
    }
    ctx.fpr[1] = static_cast<f64>(Mul28(LfsAt(h + 64), v));
    const u32 r = static_cast<u32>(VCallOpdR4_28(ctx, o, 352, load<u32>(h + 4), 0x045D9554u));
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(h + 14, static_cast<u16>((load<u16>(h + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ctx.gpr[3] = z;
}

// void f(T* o): +736 = (s * k) * |v|^2 / |+512 - +496| and +724 += s * |v|
// with v = +720..+728, s = o->+216->+420, k = *0x000F2FAC; then
// 0x000EF564(o)
DECOMP(000F2FB0, Obj_UpdateSpeed_000F2FB0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = o + 496;
    const f32 dy = Sub28(LfsAt(o + 516), LfsAt(p + 4));
    const f32 dx = Sub28(LfsAt(o + 512), LfsAt(p));
    const f32 dz = Sub28(LfsAt(o + 520), LfsAt(p + 8));
    f32 d2 = Mul28(dy, dy);
    const f32 y2 = Mul28(LfsAt(o + 724), LfsAt(o + 724));
    d2 = Madd28(dx, dx, d2);
    d2 = Madd28(dz, dz, d2);
    const f64 s = LfsAt(load<u32>(o + 216) + 420);
    f32 v2 = Madd28(LfsAt(o + 720), LfsAt(o + 720), y2);
    const f32 sk = Mul28(s, LfsAt(0x000F2FAC));
    const f32 d = static_cast<f32>(std::sqrt(static_cast<f64>(d2)));
    v2 = Madd28(LfsAt(o + 728), LfsAt(o + 728), v2);
    const f32 num = Mul28(sk, v2);
    const f32 v = static_cast<f32>(std::sqrt(static_cast<f64>(v2)));
    store<f32>(o + 736, static_cast<f32>(static_cast<f64>(num) / static_cast<f64>(d)));
    const f32 nv = Madd28(s, v, LfsAt(o + 724));
    store<f32>(o + 724, nv);
    ctx.fpr[1] = static_cast<f64>(nv);
    ctx.gpr[3] = call(ctx, 0x000EF564, {o});
}

// u16 f(T* o): VtFieldHi28 with vt[64] and the word +428 (0 otherwise)
DECOMP(00124954, Obj_GetHiField_00124954) { VtFieldHi28(ctx, 64, 428, 0); }

// void f(T* o, f32 t): when o->vt[64]() is set and has a body
// (Obj_GetBody_00252940), the u16 body->+510 = the high half of t's f32
// bits; then 0x000DA25C(o; t)
DECOMP(00124C94, Obj_SetBodyHalf_00124C94) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    if (static_cast<u32>(VCall(ctx, o, 64, {o})) != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
        const u32 b = static_cast<u32>(call(ctx, 0x00252940, {r}));
        if (b != 0) store<u16>(b + 510, static_cast<u16>(std::bit_cast<u32>(static_cast<f32>(t)) >> 16));
    }
    callf(ctx, 0x000DA25C, {o}, {t});
}

// void f(T* o, u32 id): the attached target c = Obj_Notify26(o) changes to
// id: c (when not id) is detached (0x000D91BC(o, c)); for a nonzero id,
// 0x0012E41C(o) and the value v of id in the map 0x01056F04
// (SortedMap_Find744AC4) is attached (Obj_Attach0D135C(o, v, 1), v->+8 = 0)
DECOMP(0012E534, Obj_SetTarget_0012E534) {
    Stack frame(ctx, 144);
    const u32 key = frame.addr() + 112;
    const u32 o = arg<u32>(ctx, 0), id = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x000D94CC, {o}));
    if (c != 0) {
        if (id == load<u32>(c + 4)) return;
        call(ctx, 0x000D91BC, {o, c});
    }
    if (id == 0) return;
    call(ctx, 0x0012E41C, {o});
    store<u32>(key, id);
    const u32 v = load<u32>(static_cast<u32>(call(ctx, 0x00744AC4, {0x01056F04, key})));
    if (v == 0) return;
    call(ctx, 0x000D135C, {o, v, 1});
    store<u32>(v + 8, 0);
    ctx.gpr[3] = 0;
}

// void f(?, T* o): when o has any of the flags 0x40 / 0x20 / 0x10 of the
// u8 +968 and is not in the global list 0x01013B48 (u16 count +8),
// 0x000D64F4(o)
DECOMP(00130430, Obj_EnsureListed_00130430) {
    const u32 o = arg<u32>(ctx, 1);
    ctx.gpr[3] = o;
    if (o == 0) return;
    if ((load<u8>(o + 968) & 0x70u) == 0) return;
    constexpr u32 kList = 0x01013B48;
    const u32 n = load<u16>(kList + 8);
    for (u32 i = n; i-- > 0;)
        if (load<u32>(load<u32>(kList) + 4 * i) == o) return;
    ctx.gpr[3] = call(ctx, 0x000D64F4, {o});
}

// u32 f(T* o, Hit* h): HitApply28 on the f32 +232, *0x0016903C
DECOMP(00169040, Obj_ApplyHit_00169040) { HitApply28(ctx, 232, 0x0016903C); }

// u32 f(T* o, Hit* h): HitApply28 on the f32 +236, *0x0017938C
DECOMP(00179390, Obj_ApplyHit_00179390) { HitApply28(ctx, 236, 0x0017938C); }

// u16 f(T* o): VtFieldHi28 with vt[144] and the word +44 (o->+412
// otherwise)
DECOMP(00184C08, Obj_GetHiField_00184C08) { VtFieldHi28(ctx, 144, 44, load<u32>(arg<u32>(ctx, 0) + 412)); }

// u16 f(T* o): same as 0x00184C08
DECOMP(001924F8, Obj_GetHiField_001924F8) { VtFieldHi28(ctx, 144, 44, load<u32>(arg<u32>(ctx, 0) + 412)); }

// void f(T* o, Msg* m): for the message 0xD77F98D6 / 0x00014FBC, when
// o->vt[320]() is 0, o->vt[352](o, -, 0, 0; the f32 +344)
DECOMP(00197150, Obj_OnMsg_00197150) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0xD77F98D6u;
    if (load<u32>(m + 8) != 0xD77F98D6u) return;
    ctx.gpr[3] = 0x00014FBCu;
    if (load<u32>(m + 12) != 0x00014FBCu) return;
    if (static_cast<u32>(VCall(ctx, o, 320, {o})) != 0) return;
    ctx.fpr[1] = LfsAt(o + 344);
    VCallOpdR4_28(ctx, o, 352, 0, 0);
}

// void f(T* o, bool a, bool b): unless o->vt[32](o, 0) is 4,
// o->vt[28](o, 0, 0) and o->+20's word gets bit 31 = b, bit 30 = a
DECOMP(001B7974, Obj_SetFlags_001B7974) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) == 4) return;
    VCall(ctx, o, 28, {o, 0, 0});
    const u32 p = load<u32>(o + 20);
    store<u32>(p, (load<u32>(p) & 0x3FFFFFFFu) | (b << 31) | ((a & 1u) << 30));
    ctx.gpr[3] = p;
}

// void f(T* o, Hit* h): with o->+216 = q set, Obj_TestE0BCC(o, h) and q's
// f32 +276 > *0x001CEE2C: 0x000E8678(o, h) unless the u8 q->+487 is set,
// then 0x001CEC8C(o) only when the u8 o->+853 is set
DECOMP(001CEE30, Obj_OnHit_001CEE30) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 216);
    if (q == 0) return;
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, h})) == 0) return;
    const f64 v = LfsAt(q + 276), k = LfsAt(0x001CEE2C);
    ctx.fpr[1] = v;
    ctx.fpr[2] = k;
    if (v <= k) {
        ctx.gpr[3] = 0x001CEE2C;
        return;
    }
    if (load<u8>(q + 487) == 0) {
        call(ctx, 0x000E8678, {o, h});
        return;
    }
    ctx.gpr[3] = load<u8>(o + 853);
    if (ctx.gpr[3] == 0) return;
    call(ctx, 0x001CEC8C, {o});
}

// void f(T* o): the component 0x366FD481 (Obj_FindByHash_000DB290) of the
// child +856 gets vt[88](c, 0, 1, 1, 1) and vt[40](c, 1)
DECOMP(001EDCD4, Obj_ResetComp_001EDCD4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 856);
    if (p == 0) return;
    const u32 c = static_cast<u32>(call(ctx, o25fn::Obj_FindByHash_0DB290, {p, 0x366FD481u}));
    if (c == 0) return;
    VCall(ctx, c, 88, {c, 0, 1, 1, 1});
    VCall(ctx, c, 40, {c, 1});
}

// u32 f(T* o, Hit* h): HitApply28 on the f32 +268, *0x001FC40C
DECOMP(001FC410, Obj_ApplyHit_001FC410) { HitApply28(ctx, 268, 0x001FC40C); }

// void f(T* o, U* a): +336 = a; with a->+160 = w set, the cached links
// +452 / +456 are re-resolved (WorldObj_FindById4A5AE8(w, link->+204)) and
// replaced when the result is of kind 13 (bits 10..15 of +4)
DECOMP(002167BC, Obj_ResolveLinks_002167BC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    store<u32>(o + 336, a);
    if (a == 0) return;
    const u32 w = load<u32>(a + 160);
    ctx.gpr[3] = w;
    if (w == 0) return;
    auto kind13 = [](u32 r) { return r != 0 && ((load<u32>(r + 4) >> 10) & 0x3Fu) == 13; };
    const u32 x = load<u32>(o + 452);
    if (x != 0) {
        const u32 r = static_cast<u32>(call(ctx, 0x004A5AE8, {w, load<u32>(x + 204)}));
        if (kind13(r)) store<u32>(o + 452, r);
    }
    const u32 y = load<u32>(o + 456);
    ctx.gpr[3] = y;
    if (y == 0) return;
    const u32 b = load<u32>(o + 336);
    const u32 r = static_cast<u32>(call(ctx, 0x004A5AE8, {load<u32>(b + 160), load<u32>(y + 204)}));
    if (kind13(r)) store<u32>(o + 456, r);
}

// void f(T* o, U* t): unless the flag 0x01056AA8 is set and t (with flag
// 0x8 of +192) answers vt[320]() nonzero, 0x002163A8(o, t) and
// Area_Leave(o, t)
DECOMP(0021690C, Area_MaybeLeave_0021690C) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    if (load<u8>(0x01056AA8) != 0 && t != 0 && (load<u32>(t + 192) & 0x8u) != 0 && static_cast<u32>(VCall(ctx, t, 320, {t})) != 0) return;
    call(ctx, 0x002163A8, {o, t});
    ctx.gpr[3] = call(ctx, 0x0020EBC8, {o, t});
}

// void f(T* o, u8 on): bit 0x20 of the u8 +2337 = on; the handles
// 0x00F64CD4 and (unless the flag 0x0106C3B4 is set) 0x00F64CDC each get
// Obj_VCall12IfSet_0035B3BC and Flag_Set396_2000000_00367518(it, on)
DECOMP(0029F374, Obj_SetFlag20_0029F374) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    store<u8>(o + 2337, static_cast<u8>((load<u8>(o + 2337) & ~0x20u) | ((on << 5) & 0x20u)));
    auto apply = [&](u32 g) {
        call(ctx, 0x0035B3BC, {static_cast<u32>(call(ctx, 0x003568E0, {load<u32>(g)}))});
        call(ctx, 0x00367518, {static_cast<u32>(call(ctx, 0x003568E0, {load<u32>(g)})), on});
    };
    apply(0x00F64CD4);
    ctx.gpr[3] = load<u8>(0x0106C3B4);
    if (ctx.gpr[3] != 0) return;
    apply(0x00F64CDC);
}

// void f(T* o, u32 h, u8 alt): when the effect +436 (bit 0x40000000 of
// +396) changes its hash +428 to h and has a target +20, that gets the
// hash of String_Format(0x00AD55D8, alt ? 0x00AD55E0 : 0x00AD55E8, h)
DECOMP(002C6D78, Effect_SetVariant_002C6D78) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), alt = arg<u32>(ctx, 2) & 0xFFu;
    const u32 e = load<u32>(o + 436);
    if (e == 0 || (load<u32>(e + 396) & 0x40000000u) == 0 || h == load<u32>(o + 428)) return;
    const u32 t = load<u32>(e + 20);
    store<u32>(o + 428, h);
    if (t == 0) return;
    const u32 s = static_cast<u32>(call(ctx, o27fn::String_Format, {0x00AD55D8, alt != 0 ? 0x00AD55E0u : 0x00AD55E8u, h}));
    const u32 hs = static_cast<u32>(call(ctx, 0x0056A710, {s}));
    ctx.gpr[3] = call(ctx, o27fn::Effect_SetHash, {t, hs});
}

// The count of 0x002DAA54 / 0x002DAEA0: o's index among the 860-byte
// records of 0x00F64C20->* (+20 data, +24 count; matched by the id +724;
// -1 when absent), then how many of the table's 8-byte entries (+2 u16
// count, +8 data) point (+4) at a byte equal to that index
static void CountIndexRefs28(PPUContext& ctx, u32 table) {
    const u32 id = load<u32>(arg<u32>(ctx, 0) + 724);
    const u32 g = load<u32>(0x00F64C20);
    const u32 n = load<u32>(g + 24);
    u32 idx = 0xFFFFFFFFu;
    for (u32 i = 0, e = load<u32>(g + 20); i < n; ++i, e += 860)
        if (load<u32>(e + 724) == id) {
            idx = i;
            break;
        }
    u32 cnt = 0;
    for (s32 j = 0; j < static_cast<s32>(load<u16>(table + 2)); ++j)
        if (static_cast<u32>(load<u8>(load<u32>(load<u32>(table + 8) + 8 * static_cast<u32>(j) + 4))) == idx) ++cnt;
    ctx.gpr[3] = cnt;
}

// u32 f(T* o): CountIndexRefs28 on the table 0x01068184
DECOMP(002DAA54, Obj_CountRefs_002DAA54) { CountIndexRefs28(ctx, 0x01068184); }

// u32 f(T* o): CountIndexRefs28 on the table 0x0106819C
DECOMP(002DAEA0, Obj_CountRefs_002DAEA0) { CountIndexRefs28(ctx, 0x0106819C); }

// u32 f(T* o): the total number of bits set in the u16 masks
// kWorldPtr->* + 4056 * (the u8 +4208) + 204 [k] for each 860-byte record
// of o (+20 data, +24 count) whose u8 +825 = k is not 255
DECOMP(002E6270, Obj_CountMaskBits_002E6270) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    const u32 base = w + static_cast<u32>(load<u8>(w + 4208)) * 4056u + 204;
    const u32 n = load<u32>(o + 24);
    u32 cnt = 0;
    u32 p = load<u32>(o + 20);
    for (u32 i = 0; i < n; ++i, p += 860) {
        if (p == 0) continue;
        const u32 k = load<u8>(p + 825);
        if (k == 255) continue;
        cnt += static_cast<u32>(std::popcount(static_cast<u32>(load<u16>(base + 2 * k))));
    }
    ctx.gpr[3] = cnt;
}

// void f(T* o, u8 on): releases the held object +2612 (when o->+12->+8 is
// notified (Obj_Notify24), 0x0029F510(that, held->+168) first;
// vt[40](held, 0); cleared), then Recs_Reset325A30(o, on)
DECOMP(003334C8, Obj_ReleaseHeld_003334C8) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (load<u32>(o + 2612) != 0) {
        const u32 q = load<u32>(load<u32>(o + 12) + 8);
        if (q != 0) {
            const u32 r = static_cast<u32>(call(ctx, 0x000D949C, {q}));
            if (r != 0) call(ctx, 0x0029F510, {r, load<u32>(load<u32>(o + 2612) + 168)});
        }
        const u32 c = load<u32>(o + 2612);
        VCall(ctx, c, 40, {c, 0});
        store<u32>(o + 2612, 0);
    }
    ctx.gpr[3] = call(ctx, 0x00325A30, {o, on});
}

// T* f(?, XmlCursor* c, x): reads the f32 0x00274ACD (default
// *0x0034150C), then p = SmallAlloc(2580) built by 0x00769BD8(p, x) with
// +2572 = 0, +2576 = that value, vtable 0x00B1F1B0
DECOMP(00341510, Obj_NewFromXml_00341510) {
    Stack frame(ctx, 160);
    const u32 v = frame.addr() + 112;
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    callf(ctx, o28fn::XmlCursor_ReadF32, {c, 0x00274ACD, v}, {LfsAt(0x0034150C)});
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {2580}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 f = load<u32>(v);
    call(ctx, 0x00769BD8, {p, x});
    store<u32>(p + 2572, 0);
    store<u32>(p + 2576, f);
    store<u32>(p, 0x00B1F1B0);
    ctx.gpr[3] = p;
}

// bool f(T* o): false only when o->vt[16](&ok, o) gives ok, o->+8 = c is
// set and c->vt[20](c, the u64 +16, 0) returns the u64 +16 (struct return
// through r3)
DECOMP(00347D38, Obj_CheckBinding_00347D38) {
    Stack frame(ctx, 128);
    const u32 ok = frame.addr() + 112;
    const u32 o = arg<u32>(ctx, 0);
    const u32 opd = load<u32>(load<u32>(o) + 16);
    call_ptr(ctx, opd, {ok, o, opd, load<u32>(opd)});
    ctx.gpr[3] = 1;
    if (load<u8>(ok) == 0) return;
    const u32 c = load<u32>(o + 8);
    if (c == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const u64 r = VCall(ctx, c, 20, {c, load<u64>(o + 16), 0});
    ctx.gpr[3] = r == load<u64>(o + 16) ? 0 : 1;
}

// The name binding of 0x003AB280 / 0x003AB9B8 / 0x003ABA70:
// 0x003AB0B8(o, the no-case hashes of String_Format(0x00ADF148, &o->+28,
// s) and String_Format(0x00ADF158, &o->+60, s), a6, -, a8; *kf), s being
// the string sp or (when sp is 0) the global 0x00ADF13C, read each time
static void BindNames28(PPUContext& ctx, u32 sp, u32 a6, u32 a8, u32 kf) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s1 = static_cast<u32>(call(ctx, o27fn::String_Format, {0x00ADF148, o + 28, sp != 0 ? sp : load<u32>(0x00ADF13C)}));
    const u32 h1 = static_cast<u32>(call(ctx, 0x0056A710, {s1}));
    const u32 s2 = static_cast<u32>(call(ctx, o27fn::String_Format, {0x00ADF158, o + 60, sp != 0 ? sp : load<u32>(0x00ADF13C)}));
    const u32 h2 = static_cast<u32>(call(ctx, 0x0056A710, {s2}));
    // r7 is not set (what String_HashNoCase left); the callee doesn't read it.
    callf(ctx, 0x003AB0B8, {o, h1, h2, a6, ctx.gpr[7], a8}, {LfsAt(kf)});
}

// void f(T* o): BindNames28 with the global string 0x00ADF13C, 1, 0 and
// *0x003AB278
DECOMP(003AB280, Obj_BindNames_003AB280) { BindNames28(ctx, 0, 1, 0, 0x003AB278); }

// void f(T* o): BindNames28 with 0x00ADF310, 0, 1 and *0x003AB9B0
DECOMP(003AB9B8, Obj_BindNames_003AB9B8) { BindNames28(ctx, 0x00ADF310, 0, 1, 0x003AB9B0); }

// void f(T* o): BindNames28 with 0x00ADF324, 0, 1 and *0x003ABA68
DECOMP(003ABA70, Obj_BindNames_003ABA70) { BindNames28(ctx, 0x00ADF324, 0, 1, 0x003ABA68); }

// T::~T() (vtable 0x00B20AE0): flags the owner +4 (bit 0x1000 of the u16
// +176, when it has bit 0x40000 of +172), clears +4 and the PtrVectors +160
// and +140
DECOMP(0041310C, VtB20AE0_Dtor_0041310C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 4);
    store<u32>(o, 0x00B20AE0);
    if (c != 0 && (load<u32>(c + 172) & 0x40000u) != 0) store<u16>(c + 176, static_cast<u16>(load<u16>(c + 176) | 0x1000u));
    store<u32>(o + 4, 0);
    for (u32 off : {160u, 140u}) {
        call(ctx, o19fn::PtrVector_Clear, {o + off});
        call(ctx, o19fn::Nop, {o + off});
    }
}

// T::T(const char* name, const Vec2* a, const Vec2* b, bool on): base
// 0x0042C9D0(o, on), +140/+144 = a, vtable 0x00B210B8, +148/+152 = b
// (raw), the name copied to +9 (String_CopyN, 127) when nonempty
DECOMP(00433938, VtB210B8_Ctor_00433938) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    CmdBase27(ctx, o, arg<u32>(ctx, 4));
    store<u32>(o + 140, load<u32>(a));
    store<u32>(o + 144, load<u32>(a + 4));
    const u32 b0 = load<u32>(b);
    store<u32>(o, 0x00B210B8);
    store<u32>(o + 148, b0);
    const u32 b1 = load<u32>(b + 4);
    store<u32>(o + 152, b1);
    ctx.fpr[1] = F32Bits28(b1);
    ctx.gpr[3] = 0x00B210B8;
    if (name == 0) return;
    ctx.gpr[3] = load<u8>(name);
    if (ctx.gpr[3] == 0) return;
    call(ctx, 0x00553638, {o + 9, name, 127});
}

// void f(T* o; f32 t): 0x004A0990(o; t), then +28 = (s32)((k - +12) *
// (f32)+20) + (s32)(+12 * (f32)+24) (k = *0x004A0CF8), copied to *+32 when
// set
DECOMP(004A0D00, Obj_UpdateCount_004A0D00) {
    const u32 o = arg<u32>(ctx, 0);
    callf(ctx, 0x004A0990, {o}, {farg(ctx, 0)});
    const f64 k = LfsAt(0x004A0CF8);
    const f32 a = static_cast<f32>(static_cast<f64>(load<s32>(o + 20)));
    const f64 v = LfsAt(o + 12);
    const u32 i1 = static_cast<u32>(rt::fctiw(static_cast<f64>(Mul28(Sub28(k, v), a)), true));
    const f32 b = static_cast<f32>(static_cast<f64>(load<s32>(o + 24)));
    const u32 i2 = static_cast<u32>(rt::fctiw(static_cast<f64>(Mul28(v, b)), true));
    const u32 sum = i1 + i2;
    const u32 p = load<u32>(o + 32);
    store<u32>(o + 28, sum);
    if (p != 0) store<u32>(p, sum);
    ctx.fpr[1] = std::bit_cast<f64>(0xFFF8000000000000ull | i2);  // the fctiwz result register
    ctx.gpr[3] = p;
}

// s32 f(f32 a, f32 b, f32 c, Out* r (r6)): the real roots of a x^2 + b x +
// c (k = 0x004A25A0: disc = b^2 - k0 a c, none below k1; 2a = a * k2): 0,
// 1 (r[0] = -b / 2a when |sqrt(disc) / 2a| < k3) or 2 (r[0] <= r[1])
DECOMP(004A25B0, Quadratic_Solve_004A25B0) {
    constexpr u32 k = 0x004A25A0;
    const f64 a = farg(ctx, 0), b = farg(ctx, 1), c = farg(ctx, 2);
    const u32 r = arg<u32>(ctx, 3);
    const f32 d = static_cast<f32>(std::fma(b, b, -static_cast<f64>(Mul28(Mul28(a, LfsAt(k)), c))));
    if (static_cast<f64>(d) < LfsAt(k + 4)) {
        ctx.fpr[1] = static_cast<f64>(d);
        ctx.gpr[3] = 0;
        return;
    }
    const f32 s = static_cast<f32>(std::sqrt(static_cast<f64>(d)));
    const f32 a2 = Mul28(a, LfsAt(k + 8));
    const f32 q = static_cast<f32>(b / static_cast<f64>(a2));
    const f32 h = static_cast<f32>(static_cast<f64>(s) / static_cast<f64>(a2));
    if (std::fabs(static_cast<f64>(h)) < LfsAt(k + 12)) {
        store<f32>(r, -q);
        ctx.fpr[1] = static_cast<f64>(-q);
        ctx.gpr[3] = 1;
        return;
    }
    const f32 x1 = Sub28(h, q), x2 = Sub28(-q, h);
    store<f32>(r, x1);
    store<f32>(r + 4, x2);
    if (x1 > x2) {
        store<f32>(r, x2);
        store<f32>(r + 4, x1);
    }
    ctx.fpr[1] = static_cast<f64>(x1);
    ctx.gpr[3] = 2;
}

// void f(Vec3* out, const Vec3* a, const Vec3* b, const Vec3* c, f32 t):
// out = (b - a - k1 t^2 c) * (k2 / t) for t > k0 (k = 0x004A3058), else
// the vector 0x01164B40
DECOMP(004A3068, Vec3_SolveAccel_004A3068) {
    constexpr u32 k = 0x004A3058;
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const f64 t = farg(ctx, 0);
    if (!(t > LfsAt(k))) {
        CopyVec3F28(ctx, out, 0x01164B40);
        return;
    }
    const f32 inv = static_cast<f32>(LfsAt(k + 8) / t);
    const f32 d0 = Sub28(LfsAt(b), LfsAt(a)), d1 = Sub28(LfsAt(b + 4), LfsAt(a + 4)), d2 = Sub28(LfsAt(b + 8), LfsAt(a + 8));
    const f32 t2 = Mul28(Mul28(t, LfsAt(k + 4)), t);
    auto nms = [&](f64 cv, f32 dv) { return static_cast<f32>(-std::fma(static_cast<f64>(t2), cv, -static_cast<f64>(dv))); };
    const f32 c0 = LfsAt(c), c1 = LfsAt(c + 4), c2 = LfsAt(c + 8);
    const f32 x = Mul28(nms(c0, d0), inv), y = Mul28(nms(c1, d1), inv), z = Mul28(nms(c2, d2), inv);
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    ctx.fpr[1] = static_cast<f64>(z);
}

// T::~T() (handle array, 48-byte elements): +20 = 0; unless the top bit of
// +24 is set, the vtable becomes 0x00F1DE58 and the Havok allocator
// 0x011DF31C ->vt[20](it, +16, (+24 & 0x3FFFFFFF) * 48) frees the
// buffer; +16 = 0, +24 = 0x80000000, vtable 0x00B13C84 (r3 = 0)
DECOMP(00760760, HkArray48_Dtor_00760760) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 24);
    const u32 vt = load<u32>(0x011DF31C);
    const u32 p = load<u32>(o + 16);
    store<u32>(o + 20, 0);
    if ((w & 0x80000000u) == 0) {
        const u32 opd = load<u32>(vt + 20);
        store<u32>(o, 0x00F1DE58);
        // The entry, the vtable constant and a temporary are left in r6 /
        // r7 / r8, the OPD in r9.
        const u64 x16 = static_cast<u64>(w & 0x3FFFFFFFu) << 4;
        call_ptr(ctx, opd, {0x011DF31C, p, Sx((w & 0x3FFFFFFFu) * 48u), load<u32>(opd), 0x00F1DE58, x16 * 2, opd});
    }
    store<u32>(o + 16, 0);
    store<u32>(o + 24, 0x80000000u);
    store<u32>(o, 0x00B13C84);
    ctx.gpr[3] = 0;
}

// T::~T() (vtable 0x00B1C040): the buffer +56 is freed through the
// allocator +60 (vt[8](alloc, p, the u16 +48 * (the u16 +54 >> 1))),
// Array_Destruct(&o->+48), base vtable 0x00B0DAAC and
// SortedMap_Remove7330C0(0x01056F04, &o->+4)
DECOMP(00762F10, VtB1C040_Dtor_00762F10) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = (static_cast<u32>(load<u16>(o + 54)) >> 1) & 0x7FFFu;
    const u32 m = load<u16>(o + 48);
    store<u32>(o, 0x00B1C040);
    const u32 p = load<u32>(o + 56);
    if (p != 0) {
        const u32 a = load<u32>(o + 60);
        VCall(ctx, a, 8, {a, p, m * n});
    }
    call(ctx, 0x0054EC94, {o + 48});
    store<u32>(o, 0x00B0DAAC);
    ctx.gpr[3] = call(ctx, 0x007330C0, {0x01056F04, o + 4});
}

// ---------------------------------------------------------------- sweep: 45 instructions
// void f(T* o, s32 mode): mode 0 sets the flag 0x80 of the u8 +464; mode 1
// starts the anim 0x1DE2AB30 (Anim_Restart2775A8(o->+64, 0x1DE2AB30, 0, 0,
// -1, -, 0, 0, -, -, 0, -1; the floats 0x0003088C [0], [1], [2], [2]),
// +32 = it when started)
DECOMP(00030898, Obj_SetMode_00030898) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 1);
    if (mode == 0) {
        const u32 b = load<u8>(o + 464) | 0x80u;
        store<u8>(o + 464, static_cast<u8>(b));
        ctx.gpr[3] = b;
        return;
    }
    if (mode != 1) return;
    const u64 r8 = ctx.gpr[8];  // never set by the original; the callee doesn't read it
    const f64 c = LfsAt(0x00030894);
    // Arguments 9 / 10 (r1 +112 / +120) are never written (float slots).
    callf(ctx, 0x002775A8, {load<u32>(o + 64), 0x1DE2AB30u, 0, 0, ~0ull, r8, 0, 0, 0, 0, 0, ~0ull}, {LfsAt(0x0003088C), LfsAt(0x00030890), c, c});
    if (static_cast<u32>(ctx.gpr[3]) == 0) return;
    store<u32>(o + 32, 0x1DE2AB30u);
    ctx.gpr[3] = 0x1DE2AB30u;
}

// void f(T* a, const char* name): the point named name (PointMap_Find on
// 0x00F8F154->*->+16 with its no-case hash) gets +280 =
// 0x00064D48(*0x00F557F4, point, a) and +276 = 0x7B672814; a missing one
// warns (format 0x00AC0480)
DECOMP(00064F48, Point_Bind_00064F48) {
    Stack frame(ctx, 144);
    const u32 key = frame.addr() + 112;
    const u32 a = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    if (name == 0) return;
    store<u32>(key, static_cast<u32>(call(ctx, 0x0056A710, {name})));
    const u32 m = load<u32>(load<u32>(0x00F8F154) + 16);
    const u32 v = load<u32>(static_cast<u32>(call(ctx, 0x0073343C, {m, key})));
    if (v == 0) {
        call(ctx, o27fn::String_Format, {0x00AC0480, name});
        return;
    }
    const u32 x = static_cast<u32>(call(ctx, 0x00064D48, {load<u32>(0x00F557F4), v, a}));
    store<u32>(v + 280, x);
    store<u32>(v + 276, 0x7B672814u);
    ctx.gpr[3] = 0x7B672814u;
}

// void Cmd::Run(): for the world object WorldObj_FindByHash(+12, 0), pushes
// a new 24-byte command VtB21310_Ctor(p, it, +16, the u8s +20, +21)
// (CmdList_Push_0042C138, null when the allocation fails)
DECOMP(000A5950, Cmd_PushForObj_000A5950) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = load<u32>(o + 12);
    if (h == 0) return;
    const u32 w = static_cast<u32>(call(ctx, 0x005801DC, {h, 0}));
    if (w == 0) return;
    PushNew26(ctx, 24, [&](u32 p) { call(ctx, 0x004390AC, {p, w, load<u32>(o + 16), load<u8>(o + 20), load<u8>(o + 21)}); });
}

// u32 f(const char* path, u8 flag): loads the xmlb document path (a local
// XmlDoc: XmlDoc_Init4FA7A4, XmlDoc_LoadXmlb(doc, path, 0)) and returns
// Xml_Load0D5F58(doc, path + 4, flag), or flag when it doesn't load
DECOMP(000D643C, Xml_LoadFile_000D643C) {
    Stack frame(ctx, 2384);
    const u32 doc = frame.addr();
    const u32 path = arg<u32>(ctx, 0), flag = arg<u32>(ctx, 1) & 0xFFu;
    call(ctx, 0x004FA7A4, {doc});
    if (static_cast<u32>(call(ctx, 0x004FACCC, {doc, path, 0})) == 0) {
        call(ctx, 0x004FA9C0, {doc});
        ctx.gpr[3] = flag;
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x000D5F58, {doc, path + 4, flag}));
    call(ctx, 0x004FA9C0, {doc});
    ctx.gpr[3] = r;
}

// void f(T* o, U* t, s32 mode): o->vt[40](o, 0); for mode 3 with bit 0x1 of
// t's word +188, clears bit (the u8 t->+172) of o's component 27's +88
DECOMP(000DFB48, Obj_ClearCompBit_000DFB48) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const s32 mode = arg<s32>(ctx, 2);
    VCall(ctx, o, 40, {o, 0});
    if (mode != 3) return;
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288, {o, 27, 0}));
    if (c == 0) return;
    ctx.gpr[3] = load<u32>(t + 188) & 1u;
    if (ctx.gpr[3] == 0) return;
    const u32 n = load<u8>(t + 172);
    const u32 bit = (n & 0x20u) != 0 ? 0u : (1u << (n & 31u));
    const u32 v = load<u32>(c + 88) & ~bit;
    store<u32>(c + 88, v);
    ctx.gpr[3] = v;
}

// u32 f(T* o, Hit* h): when Obj_TestE0BCC(o, h) and h->+32 * o->+216's f32
// +268 > *0x000E85B8, r = o->vt[352](o, -, h->+4, 0x811B1D5E; that) (the
// entry in r4, the OPD in r7) and bit 0x100 of the u16 h->+14 = (r == 0)
DECOMP(000E85C0, Obj_ApplyHit_000E85C0) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 216);
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, h})) == 0) return;
    const f32 v = Mul28(LfsAt(h + 32), LfsAt(q + 268));
    ctx.fpr[1] = static_cast<f64>(v);
    if (!(static_cast<f64>(v) > LfsAt(0x000E85B8))) {
        ctx.gpr[3] = 0x000E85B8;
        return;
    }
    const u32 opd = load<u32>(load<u32>(o) + 352);
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, load<u32>(opd), load<u32>(h + 4), 0x811B1D5Eu, opd}));
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(h + 14, static_cast<u16>((load<u16>(h + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ctx.gpr[3] = z;
}

// T::~T() (vtable 0x00B13CF0): releases the children +12 and +16
// (vt[40](c, 0), cleared), base vtable 0x00B0DAEC
DECOMP(001096C0, VtB13CF0_Dtor_001096C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 12);
    store<u32>(o, 0x00B13CF0);
    if (c != 0) VCall(ctx, c, 40, {c, 0});
    const u32 d = load<u32>(o + 16);
    store<u32>(o + 12, 0);
    if (d != 0) {
        VCall(ctx, d, 40, {d, 0});
        store<u32>(o + 16, 0);
    }
    store<u32>(o, 0x00B0DAEC);
    ctx.gpr[3] = 0x00B0DAEC;
}

// T::~T() (vtable 0x00B14078): releases the children +16 and +12
// (vt[40](c, 0), cleared), base vtable 0x00B0DAEC
DECOMP(00115D2C, VtB14078_Dtor_00115D2C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 16);
    store<u32>(o, 0x00B14078);
    if (c != 0) VCall(ctx, c, 40, {c, 0});
    const u32 d = load<u32>(o + 12);
    store<u32>(o + 16, 0);
    if (d != 0) VCall(ctx, d, 40, {d, 0});
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B0DAEC);
    ctx.gpr[3] = 0x00B0DAEC;
}

// void f(T* o, u8 a, u8 b, u8 c, u8 d): the child +424 (when set) gets
// vt[88](child, a, b, c, d), then 0x000D9F1C(o, a, b, c, d)
DECOMP(00128E2C, Obj_Forward88_00128E2C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu, c = arg<u32>(ctx, 3) & 0xFFu, d = arg<u32>(ctx, 4) & 0xFFu;
    const u32 x = load<u32>(o + 424);
    if (x != 0) VCall(ctx, x, 88, {x, a, b, c, d});
    ctx.gpr[3] = call(ctx, 0x000D9F1C, {o, a, b, c, d});
}

// void f(T* o, const Vec3* v): the body b = o->vt[64]() gets +672..+680 =
// *v (raw), bit 0x00800000 of +416 and Body_SetMode(b, 2); then o->+20
// gets +80 = -1.0f and bit 0x40000000 of +48
DECOMP(0012BCA8, Obj_SetBodyPos_0012BCA8) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 b = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (b == 0) return;
    const u32 x = load<u32>(v);
    const u32 w = load<u32>(b + 416);
    store<u32>(b + 672, x);
    store<u32>(b + 676, load<u32>(v + 4));
    const u32 z = load<u32>(v + 8);
    store<u32>(b + 416, w | 0x00800000u);
    store<u32>(b + 680, z);
    call(ctx, 0x00261328, {b, 2});
    const u32 p = load<u32>(o + 20);
    store<u32>(p + 80, 0xBF800000u);
    const u32 n = load<u32>(p + 48) | 0x40000000u;
    store<u32>(p + 48, n);
    ctx.fpr[1] = F32Bits28(z);
    ctx.gpr[3] = n;
}

// void f(T* o): when o->vt[32](o, 1) is 6, o->vt[28](o, 0, 0),
// o->vt[28](o, 1, 0) and Nop_004902F0
DECOMP(0012D290, Obj_Reset6_0012D290) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 1})) != 6) return;
    VCall(ctx, o, 28, {o, 0, 0});
    VCall(ctx, o, 28, {o, 1, 0});
    ctx.gpr[3] = call(ctx, 0x004902F0, {static_cast<u32>(ctx.gpr[3])});
}

// void f(T* o, u8 on): o->vt[388](), o->vt[28](o, 0, on), the u8 at o +
// o->+440 + 433 = on, o->vt[384]()
DECOMP(0017DB34, Obj_SetState_0017DB34) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    VCall(ctx, o, 388, {o});
    VCall(ctx, o, 28, {o, 0, on});
    const u32 off = load<u32>(o + 440);
    const u32 vt = load<u32>(o);
    store<u8>(o + off + 433, static_cast<u8>(on));
    ctx.gpr[3] = VCallVt(ctx, vt, 384, {o});
}
