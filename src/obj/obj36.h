// Game code, part 36: helpers shared by obj36*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the last 70
// instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj35.h"

namespace sly4 {
namespace obj36 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj35;
using namespace sly4::obj34;
using namespace sly4::obj33;
using namespace sly4::obj32;
using namespace sly4::obj31;
using namespace sly4::obj30;
using namespace sly4::obj29;
using namespace sly4::obj28;
using namespace sly4::obj27;
using namespace sly4::obj26;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

constexpr u32 kGetPeer36 = 0x0057A428u;    // WorldObj_GetPeer(w)
constexpr u32 kPeerGetVec36 = 0x0057BE98u; // Peer_GetVec(peer)

// XmlCursor_ReadS32(x, key, m + off, current value sign-extended).
inline u64 RdS32Cur36(PPUContext& ctx, u32 x, u32 key, u32 a) {
    return call(ctx, 0x004F99BCu, {x, key, a, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(a))))});
}

// o's world peer vector flag 0x20 (byte +28) set to `on`, when o->+160 is
// set, has a peer and that peer's +136 is set (WorldObj_GetPeer is called
// three times, as in the original; Peer_GetVec on the last one). r3: 0
// for a missing step, else Peer_GetVec's result.
inline void PeerVecFlag36(PPUContext& ctx, u32 o, bool on) {
    const u32 w = load<u32>(o + 160);
    ctx.gpr[3] = w;
    if (w == 0) return;
    if (static_cast<u32>(call(ctx, kGetPeer36, {w})) == 0) return;
    const u32 p = static_cast<u32>(call(ctx, kGetPeer36, {load<u32>(o + 160)}));
    const u32 k = load<u32>(p + 136);
    ctx.gpr[3] = k;
    if (k == 0) return;
    const u32 q = static_cast<u32>(call(ctx, kGetPeer36, {load<u32>(o + 160)}));
    const u32 v = static_cast<u32>(call(ctx, kPeerGetVec36, {q}));
    store<u8>(v + 28, static_cast<u8>((load<u8>(v + 28) & ~0x20u) | (on ? 0x20u : 0u)));
}

// Call through slot `slot` of the vtable vt with the entry address in the
// register after the arguments and the OPD in the one after that (the
// reverse of VCall's order, as some call sequences leave them).
inline u64 VCallEO36(PPUContext& ctx, u32 vt, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(vt + slot);
    u64 r[8] = {};
    u32 n = 0;
    for (u64 a : args) r[n++] = a;
    r[n++] = load<u32>(opd);
    r[n++] = opd;
    switch (n) {
    case 2: return call_ptr(ctx, opd, {r[0], r[1]});
    case 3: return call_ptr(ctx, opd, {r[0], r[1], r[2]});
    case 4: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3]});
    case 5: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4]});
    case 6: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5]});
    case 7: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6]});
    default: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]});
    }
}

// The countdown step of 0x0036BAA0 / 0x00384088 (void f(T* o; f1 = dt)):
// unless Obj_Test_004A0BB0(o + 48), Obj_LerpPos_004A0FC8(o + 48; dt) and,
// with a world object w = o->+24, o +32..+40 copied to w +64..+72 (raw)
// and WorldObj_MarkDirty(w). Then, when the timer o->foff is above the
// constant K at kaddr: it is decreased by dt when Obj_Test_004A0BB0(o +
// 48) is set, and when it is (then) <= K, fn(o, bit m1 of o->woff, bit
// m2). r3 = kaddr / the test's / fn's; f1 = the timer / fn's.
inline void TimerTick36(PPUContext& ctx, u32 foff, u32 kaddr, u32 woff, u32 m1, u32 m2, u32 fn) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 s = o + 48;
    if (static_cast<u32>(call(ctx, 0x004A0BB0u, {s})) == 0) {
        callf(ctx, 0x004A0FC8u, {s}, {dt});
        const u32 w = load<u32>(o + 24);
        if (w != 0) {
            store<u32>(w + 64, load<u32>(o + 32));
            store<u32>(w + 68, load<u32>(o + 36));
            store<u32>(w + 72, load<u32>(o + 40));
            call(ctx, 0x0057D000u, {w, w});
        }
    }
    const f64 K = Lfs32(kaddr);
    f64 v = Lfs32(o + foff);
    ctx.fpr[1] = v;
    ctx.gpr[3] = kaddr;
    if (!(v > K)) return;
    const u32 t = static_cast<u32>(call(ctx, 0x004A0BB0u, {s}));
    v = Lfs32(o + foff);
    if (t != 0) {
        v = static_cast<f32>(v - dt);
        Stfs32(o + foff, v);
    }
    ctx.fpr[1] = v;
    if (!(v <= K)) return;
    const u32 w = load<u32>(o + woff);
    call(ctx, fn, {o, (w & m1) != 0 ? 1u : 0u, (w & m2) != 0 ? 1u : 0u});
}

// The message relay of 0x0012CEA8 / 0x0012CFCC (void f(-, Msg* m)): the
// target t = the +280 of m->+32 (p) when p's class hash +276 is
// 0x02235C33, else the same test on p->+260 (when set); with a target:
// fa(t, m) when bit 4 of t->+192, else fb(t, m) for bit 0x800 of t->+196,
// else fc(t, m) for bit 0x1000 of it; then Msg_Post(t, t, msg, m). r3 =
// 0, 0x02235C33 (no target behind a set p->+260) or Msg_Post's.
inline void OwnerMsg36(PPUContext& ctx, u32 fa, u32 fb, u32 fc, u32 msg) {
    const u32 m = arg<u32>(ctx, 1);
    const u32 p = load<u32>(m + 32);
    u32 t = 0;
    ctx.gpr[3] = 0;
    if (load<u32>(p + 276) == 0x02235C33u) t = load<u32>(p + 280);
    if (t == 0) {
        const u32 q = load<u32>(p + 260);
        if (q != 0) {
            ctx.gpr[3] = 0x02235C33u;
            if (load<u32>(q + 276) == 0x02235C33u) t = load<u32>(q + 280);
        }
    }
    if (t == 0) return;
    if ((load<u32>(t + 192) & 4u) != 0) call(ctx, fa, {t, m});
    else if ((load<u32>(t + 196) & 0x800u) != 0) call(ctx, fb, {t, m});
    else if ((load<u32>(t + 196) & 0x1000u) != 0) call(ctx, fc, {t, m});
    call(ctx, 0x00470660u, {t, t, msg, m});
}

// The destructor body of 0x0036A098 / 0x0036D118 (void f(T* o)):
// vtable 0x00B1F678; an effect o->+20 is released (Effect_Release(e,
// 0x00010CBB)) and gets bit 0x200 of +136; an object p = o->+24 (its
// +276 / +280 cleared when +276 is 0x7DF841DC) gets p->vt[40](p, 0, 1)
// and o->+24->vt[8](); q = o->+28 gets q->vt[4]() and the word is
// cleared; then VtB21B58_DtorTrivial_004A0F70(o + 48) (r3 = its).
inline void DtorB1F678_36(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 e = load<u32>(o + 20);
    store<u32>(o, 0x00B1F678u);
    if (e != 0) {
        call(ctx, 0x004CF3C4u, {e, 0x00010CBBu});
        const u32 e2 = load<u32>(o + 20);
        store<u32>(e2 + 136, load<u32>(e2 + 136) | 0x200u);
    }
    u32 p = load<u32>(o + 24);
    if (p != 0) {
        if (load<u32>(p + 276) == 0x7DF841DCu) {
            store<u32>(p + 276, 0);
            store<u32>(p + 280, 0);
        }
        VCall(ctx, p, 40, {p, 0, 1});
        p = load<u32>(o + 24);
        VCall(ctx, p, 8, {p});
    }
    const u32 q = load<u32>(o + 28);
    if (q != 0) {
        VCall(ctx, q, 4, {q});
        store<u32>(o + 28, 0);
    }
    call(ctx, 0x004A0F70u, {o + 48});
}

// The voice trigger of 0x003841B0 / 0x00387748 (void f(T* o)), with the
// flag word w = o->woff: with flag mA, nothing unless
// Obj_Anim353EB0_004A5D40(o->+28, 0); with flag mB, nothing unless the
// effect o->+20 is set and Effect_Check4CF620(it, 0). Then the low bits
// of o->+160 = 3 << 1 (bit 0 kept), o->fdst = o->fa when that is >= the
// constant at kaddr, else o->fb (raw), mA = bit 0 of
// Speaker_SayId4A5C90(o->+28, o->idoff, 1; f1 = the float at kaddr + 4),
// and with the effect set mB = bit 0 of Effect_SetHash4CF988(it,
// *hashaddr). r3 as left.
inline void SayStep36(PPUContext& ctx, u32 woff, u32 mA, u32 mB, u32 fa, u32 fb, u32 fdst, u32 kaddr, u32 idoff,
                      u32 hashaddr) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + woff) & mA) != 0) {
        if (static_cast<u32>(call(ctx, 0x004A5D40u, {load<u32>(o + 28), 0})) == 0) return;
    }
    const u32 w = load<u32>(o + woff);
    if ((w & mB) != 0) {
        const u32 e = load<u32>(o + 20);
        ctx.gpr[3] = e;
        if (e == 0) return;
        if (static_cast<u32>(call(ctx, 0x004CF620u, {e, 0})) == 0) return;
    }
    store<u32>(o + 160, (load<u32>(o + 160) & 1u) | 6u);
    const f64 a = Lfs32(o + fa);
    store<u32>(o + fdst, a >= Lfs32(kaddr) ? load<u32>(o + fa) : load<u32>(o + fb));
    const u32 id = load<u32>(o + idoff), sp = load<u32>(o + 28);
    callf(ctx, 0x004A5C90u, {sp, id, 1, sp, id}, {Lfs32(kaddr + 4)});
    const u32 r = static_cast<u32>(ctx.gpr[3]);
    const u32 e = load<u32>(o + 20);
    store<u32>(o + woff, (load<u32>(o + woff) & ~mA) | ((r & 1u) != 0 ? mA : 0u));
    ctx.gpr[3] = e;
    if (e == 0) return;
    const u32 h = static_cast<u32>(call(ctx, 0x004CF988u, {e, load<u32>(hashaddr)}));
    store<u32>(o + woff, (load<u32>(o + woff) & ~mB) | ((h & 1u) != 0 ? mB : 0u));
}

// The hold release of 0x0042FCA4 / 0x00430940 (bool f(T* o)): for the
// target t = o->+12 with bit 8 of +192 and a set t->+380 (c): false when
// (c + 80)->vt[32](c + 80, 0) is set; else, when t->vt[376]() is set,
// t->vt[376]()->vt[44](it, 1), and t->+380 = t->+384. True otherwise.
inline void ReleaseHold36(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(o + 12);
    if (t == 0 || (load<u32>(t + 192) & 8u) == 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 c = load<u32>(t + 380);
    if (c == 0) {
        ret(ctx, 1u);
        return;
    }
    if (static_cast<u32>(VCall(ctx, c + 80, 32, {c + 80, 0})) != 0) {
        ret(ctx, 0u);
        return;
    }
    if (static_cast<u32>(VCall(ctx, t, 376, {t})) != 0) {
        const u32 y = static_cast<u32>(VCall(ctx, t, 376, {t}));
        VCall(ctx, y, 44, {y, 1});
    }
    store<u32>(t + 380, load<u32>(t + 384));
    ret(ctx, 1u);
}

// Bit 31 of o->+384 = bit 0 of on (r3 = the new word).
inline void SetBit384_36(PPUContext& ctx, u32 o, u32 on) {
    const u32 w = (load<u32>(o + 384) & 0x7FFFFFFFu) | ((on & 1u) << 31);
    store<u32>(o + 384, w);
    ctx.gpr[3] = w;
}

// The name-pair readers 0x0034B594 / 0x0034B884 / 0x0034BB58 (bool f(Reader*
// r)): the 129-byte buffer buf is cleared once (memset, the flag byte
// flag set); Iff_ReadU8(r, &tmp, 0) (a stack byte, unused), then 64 bytes
// to buf + 1 and 64 to buf + 65 (FileReader_ReadByte(r, p, 0) each);
// *w1 = buf + 65 and *w2 = String_HashNoCase(buf + 1). r3 = 1.
inline void ReadNamePair36(PPUContext& ctx, u32 flag, u32 buf, u32 w1, u32 w2) {
    const u32 r = arg<u32>(ctx, 0);
    if (load<u8>(flag) == 0) {
        call(ctx, 0x00726E54u, {buf, 0, 129});
        store<u8>(flag, 1);
    }
    {
        Stack fr(ctx, 16);
        call(ctx, 0x005643C4u, {r, fr.addr(), 0});
    }
    for (u32 i = 0; i < 64; ++i) call(ctx, 0x0056998Cu, {r, buf + 1 + i, 0});
    for (u32 i = 0; i < 64; ++i) call(ctx, 0x0056998Cu, {r, buf + 65 + i, 0});
    store<u32>(w1, buf + 65);
    store<u32>(w2, static_cast<u32>(call(ctx, 0x0056A710u, {buf + 1})));
    ret(ctx, 1u);
}

}  // namespace obj36
}  // namespace sly4
