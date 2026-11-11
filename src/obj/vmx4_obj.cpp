// Game code, VMX sweep part 4 (see vmx4.h): game-object functions.
#include "../sly4.h"
#include "vmx4.h"

using namespace sly4;
using namespace sly4::vmx4;

// u32 f(T* o, u32 text, u32 key, bool b; f32 f1) (called by 00082638,
// 001E43A0, 001F08C0, 002C3CD0, 002C7340, 00436E98): plays a text anim
// through o's anim set (+36). The slot entry e (key at +0 of the eight
// 40-byte entries at o+40) with key, else 0. With no anim set: 0. Else, with
// a 104-byte descriptor and a 16-byte info block on the stack (cleared:
// memset, stvx), the scope s = Scope_Lookup(*(handle +32 ->+8), text) when
// the handle resolves to a non-null object; without s or e the plain
// Obj_PlayTextAnim(o, text, key, b) result. With both: when s's byte +20
// has 0x10 the value type (s16)s->+56 must index the table (*0x00F557F8)
// ->+268 (count +272), else "Unknown Anim ctrl value type pass used!" goes
// to String_Format; a valid type stores f1 at info +8. info = {e->+16,
// e->+4}; Layer_Query(handle object, s, &desc, &info, -1); then the track
// a = Obj_FindComp(o->+36, key) and the text entry t = Text_Lookup(desc
// +24) must both exist (else 0): t->+40 = 1, Track_Play(a, t, b, ...; 1.0,
// 0.0 from 0x002983B4), Obj_SetF32Via(a; o->+380), 1.
DECOMP(002983C0, Obj_PlayTextAnimSlot_002983C0) {
    const u32 o = arg<u32>(ctx, 0), text = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const u32 b = arg<u32>(ctx, 3) & 0xFFu;
    const f64 f31 = farg(ctx, 0);
    u32 e = 0;
    for (u32 k = 0; k < 8; ++k)
        if (load<u32>(o + 40 + 40 * k) == key) {
            e = o + 40 + 40 * k;
            break;
        }
    if (load<u32>(o + 36) == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 320);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 info = sp + 112, desc = sp + 128;
    call(ctx, v4fn::memset, {desc, 0, 104});
    Stvx(info, Zero4());
    u32 s = 0;
    {
        const u32 rec = HandleRec(load<u32>(kHandleTab), load<u32>(o + 32));
        if (rec != 0) {
            const u32 p = load<u32>(rec + 8);
            if (p != 0) {
                const u32 p2 = load<u32>(p);
                if (p2 != 0) s = static_cast<u32>(call(ctx, v4fn::Scope_Lookup, {p2, text}));
            }
        }
    }
    if (s == 0 || e == 0) {
        call(ctx, v4fn::Obj_PlayTextAnim, {o, text, key, b});
        return;
    }
    if ((load<u8>(s + 20) & 0x10u) != 0) {
        const u32 tab = load<u32>(kHandleTab);
        const s32 ty = static_cast<s16>(load<u16>(s + 56));
        bool ok = false;
        if (ty >= 0 && ty < static_cast<s32>(load<u32>(tab + 272)) && load<u32>(tab + 268) + static_cast<u32>(ty) * 8 != 0) ok = true;
        if (ok) store<f32>(info + 8, static_cast<f32>(f31));
        else call(ctx, v4fn::String_Format, {0x00AD4E44u});  // "Unknown Anim ctrl value type pass used!"
    }
    store<u32>(info, load<u32>(e + 16));
    store<u32>(info + 4, load<u32>(e + 4));
    {
        // The handle resolved again; an unresolved one would read address 0
        // (the original's unchecked load).
        const u32 rec = HandleRec(load<u32>(kHandleTab), load<u32>(o + 32));
        const u32 p = rec != 0 ? load<u32>(rec + 8) : 0;
        call(ctx, v4fn::Layer_Query, {load<u32>(p), s, desc, info, static_cast<u64>(-1)});
    }
    const u32 a = static_cast<u32>(call(ctx, v4fn::Obj_FindComp, {load<u32>(o + 36), key}));
    if (a == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 t = static_cast<u32>(call(ctx, v4fn::Text_Lookup, {load<u32>(desc + 24)}));
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 K = 0x002983B4u;  // {1.0f, 0.0f}
    store<u8>(t + 40, 1);
    // r7 as Text_Lookup left it (unread by the callee)
    callf(ctx, v4fn::Track_Play, {a, t, b, K, ctx.gpr[7], 0}, {Lfs4(K), Lfs4(K + 4)});
    callf(ctx, v4fn::Obj_SetF32Via, {a}, {Lfs4(o + 380)});
    ret(ctx, 1u);
}

namespace {
// Three inlined random steps on the global 0x01164888 (+0 u64 last value,
// +8 u32 seed; see obj28's RandStep28) with one final write-back: the old
// u64 and the next two low halves, the state then three steps on.
struct Rand3 {
    u64 v[3];
};
inline Rand3 RandStep3() {
    Rand3 r;
    r.v[0] = load<u64>(0x01164888);
    u32 s = load<u32>(0x01164890), x = 0;
    for (u32 k = 0; k < 3; ++k) {
        x = (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u;
        s = std::rotl(x, 16);
        if (k < 2) r.v[k + 1] = x & 0xFFFFu;
    }
    store<u32>(0x01164890, s);
    store<u64>(0x01164888, x & 0xFFFFu);
    return r;
}
// fcfid + frsp of the u64 as s64.
inline f32 U64f(u64 v) { return static_cast<f32>(static_cast<f64>(static_cast<s64>(v))); }
constexpr u32 kInv65535 = 0x0001E05C;  // f32 1/65535 (then 0.5f at +4)
}  // namespace

// void f(f32 out[4], T* o) (called by 0001E420, 000215D8): a random spawn
// point for o. With the pickable p = o->+48->+220 where Obj_Pick(p) is
// non-null and the slot byte (o->+308 ->+12)[4 * (*0x00F947DC)->+64] is 0:
// a random point of the picked object's box (P->+208 + 16: min +0, max +16)
// shrunk by o's radius (+324), stored, then through P's rows +80 (stvx, all
// four lanes). Else, with R = (o->+308 ->+12)[(*0x00F94794)->+64] (f32
// slot) - radius: x, z uniform in [-R, R), y in [radius, R), halved when
// x^2 + z^2 > R^2, plus the base o->+48 's +80..+88. Without bit 0x20 of
// byte +305, out[1] = o->+180. The random state (0x01164888) advances three
// steps. f1 / r3 are left as the original leaves them.
DECOMP(0001E068, Obj_RandomSpawnPoint_0001E068) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 pk = load<u32>(load<u32>(o + 48) + 220);
    bool box = false;
    if (pk != 0 && call(ctx, v4fn::Obj_Pick, {pk}) != 0) {
        const u32 slot = load<u32>(load<u32>(0x00F947DCu) + 64) << 2;
        box = load<u8>(load<u32>(load<u32>(o + 308) + 12) + slot) == 0;
    }
    const f32 K = F(kInv65535);
    f32 f1;
    if (box) {
        const u32 P = static_cast<u32>(call(ctx, v4fn::Obj_Pick, {pk}));
        const f64 r = F(o + 324);
        const u32 B = load<u32>(P + 208) + 16;
        const f32 lx = Adds(F(B), r), ly = Adds(F(B + 4), r), lz = Adds(F(B + 8), r);
        const f32 hx = Subs(F(B + 16), r), hy = Subs(F(B + 20), r), hz = Subs(F(B + 24), r);
        const Rand3 rn = RandStep3();
        const f32 x = Fmadds(Subs(hx, lx), Muls(U64f(rn.v[0]), K), lx);
        const f32 y = Fmadds(Subs(hy, ly), Muls(U64f(rn.v[1]), K), ly);
        const f32 z = Fmadds(Subs(hz, lz), Muls(U64f(rn.v[2]), K), lz);
        store<f32>(out, x);
        store<f32>(out + 4, y);
        store<f32>(out + 8, z);
        Stvx(out, XformPoint(Lvx(P + 80), Lvx(P + 96), Lvx(P + 112), Lvx(P + 128), V4{{x, y, z, 0.0f}}));
        f1 = z;
    } else {
        const f64 r = F(o + 324);
        const u32 slot = load<u32>(load<u32>(0x00F94794u) + 64) << 2;
        const f32 R = Subs(F(load<u32>(load<u32>(o + 308) + 12) + slot), r);
        const Rand3 rn = RandStep3();
        const f32 nR = -R, R2 = Muls(R, R), d = Subs(R, nR);
        f32 x = Fmadds(d, Muls(U64f(rn.v[0]), K), nR);
        f32 z = Fmadds(d, Muls(U64f(rn.v[2]), K), nR);
        f32 y = Fmadds(Subs(R, r), Muls(U64f(rn.v[1]), K), r);
        const f32 l2 = Fmadds(x, x, Muls(z, z));
        if (l2 > R2) {
            const f32 h = F(kInv65535 + 4);
            x = Muls(x, h);
            y = Muls(y, h);
            z = Muls(z, h);
        }
        const u32 b = load<u32>(o + 48);
        x = Adds(F(b + 80), x);
        y = Adds(F(b + 84), y);
        z = Adds(F(b + 88), z);
        store<f32>(out, x);
        store<f32>(out + 4, y);
        store<f32>(out + 8, z);
        f1 = x;
    }
    if ((load<u8>(o + 305) & 0x20u) == 0) {
        CopyW(out + 4, o + 180);
        SetF1Bits(ctx, load<u32>(o + 180));
        ctx.gpr[3] = o + 180;
    } else {
        ctx.fpr[1] = f1;
        ctx.gpr[3] = 1;
    }
}

namespace {
constexpr u32 kK17F8D0 = 0x0017F8D0;  // f32 {3.0 max reach, 1.0, 20.0 max speed, 2.0, 0.6}
}  // namespace

// void f(T* o, a, const f32 m[4][4]; f32 dt) (called by 00185D18): one
// spring step of a dangling point. The anchor p (+16) = the local point
// +80 through m (stvx, four lanes); the end c (+48) is pulled to within 3.0
// of p; the previous end (+32) = c, or (p.x, c.y, p.z) with +108 set. The
// force F (+0) = (p - prev) * stiffness (+96) - v (+64) * damping (+100);
// c += v * dt, v += F / mass (+104) * dt; with +108 set c.xz = p.xz and
// v.xz = 0. A speed above 20.0 is clamped (v normalised, re-stored, then
// scaled; the original's redundant re-stores kept). Then the probe
// Rope_Probe(a, {p.x, p.y + 1, p.z}, ..., {p.y, p.y - 1, p.y}, ...)
// (the second point as the original builds it) -> byte +128; a hit with
// +116 >= 0.6 moves p to the probe's second point, one below 0.6 clears
// +128. f1 / r3 are left as the original leaves them.
DECOMP(0017F8E8, Rope_Step_0017F8E8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const f64 dt = farg(ctx, 0);
    const f32 K0 = F(kK17F8D0), K1 = F(kK17F8D0 + 4), K2 = F(kK17F8D0 + 8);
    Stvx(o + 16, XformPoint(Lvx(m), Lvx(m + 16), Lvx(m + 32), Lvx(m + 48), W3(o + 80)));
    {
        const f32 px = F(o + 16), py = F(o + 20), pz = F(o + 24);
        f32 cx = F(o + 48), cy = F(o + 52), cz = F(o + 56);
        const f32 dx = Subs(cx, px), dy = Subs(cy, py), dz = Subs(cz, pz);
        const f32 len = Len3s(dx, dy, dz);
        if (len > K0) {
            const f32 s = static_cast<f32>(static_cast<f64>(K1) / len);
            cx = Fmadds(Muls(dx, s), K0, px);
            cy = Fmadds(Muls(dy, s), K0, py);
            cz = Fmadds(Muls(dz, s), K0, pz);
            store<f32>(o + 48, cx);
            store<f32>(o + 52, cy);
            store<f32>(o + 56, cz);
        }
        store<f32>(o + 32, cx);
        store<f32>(o + 36, cy);
        store<f32>(o + 40, cz);
    }
    if (load<u32>(o + 108) != 0) {
        CopyW(o + 32, o + 16);
        CopyW(o + 40, o + 24);
    }
    const f32 qx = F(o + 32), qy = F(o + 36), qz = F(o + 40);
    const f32 px = F(o + 16), py = F(o + 20), pz = F(o + 24);
    const f32 cx = F(o + 48), cy = F(o + 52), cz = F(o + 56);
    const f32 k = F(o + 100), stf = F(o + 96), mass = F(o + 104);
    const f32 vx = F(o + 64), vy = F(o + 68), vz = F(o + 72);
    const f32 fx = Fmsubs(Subs(px, qx), stf, Muls(vx, k));
    const f32 fy = Fmsubs(Subs(py, qy), stf, Muls(vy, k));
    const f32 fz = Fmsubs(Subs(pz, qz), stf, Muls(vz, k));
    store<f32>(o + 0, fx);
    store<f32>(o + 4, fy);
    store<f32>(o + 8, fz);
    const f32 inv = static_cast<f32>(static_cast<f64>(K1) / mass);
    store<f32>(o + 48, Fmadds(vx, dt, cx));
    store<f32>(o + 56, Fmadds(vz, dt, cz));
    store<f32>(o + 52, Fmadds(vy, dt, cy));
    store<f32>(o + 64, Fmadds(Muls(fx, inv), dt, vx));
    store<f32>(o + 68, Fmadds(Muls(fy, inv), dt, vy));
    store<f32>(o + 72, Fmadds(Muls(fz, inv), dt, vz));
    if (load<u32>(o + 108) != 0) {
        CopyW(o + 48, o + 16);
        CopyW(o + 56, o + 24);
        store<u32>(o + 64, 0);
        store<u32>(o + 72, 0);
    }
    {
        const f32 wx = F(o + 64), wy = F(o + 68), wz = F(o + 72);
        const f32 sp = Len3s(wx, wy, wz);
        if (sp > K2) {
            const f32 s = static_cast<f32>(static_cast<f64>(K1) / sp);
            store<f32>(o + 64, Muls(wx, s));
            store<f32>(o + 68, Muls(wy, s));
            store<f32>(o + 72, Muls(wz, s));
            CopyW(o + 64, o + 64);  // the original's re-stores
            CopyW(o + 68, o + 68);
            store<f32>(o + 64, Muls(F(o + 64), K2));
            store<f32>(o + 68, Muls(F(o + 68), K2));
            store<f32>(o + 72, Muls(F(o + 72), K2));
        }
    }
    Stack frame(ctx, 208);
    const u32 st = static_cast<u32>(ctx.gpr[1]);
    const u32 from = st + 128, to = st + 144, lim = st + 160;
    store<f32>(from, F(o + 16));
    store<f32>(from + 8, F(o + 24));
    store<f32>(from + 4, Adds(F(o + 20), K1));
    const f32 y = F(o + 20);
    store<f32>(to, y);
    store<f32>(to + 8, y);
    store<u32>(lim, 0x447A0000u);  // 1000.0f
    store<f32>(to + 4, Subs(y, K1));
    callf(ctx, v4fn::Rope_Probe, {a, from, 0x01164B90u, kK17F8D0, to, o + 112, lim, 0, 0}, {Lfs4(kK17F8D0 + 12)});
    const u32 hit = static_cast<u32>(ctx.gpr[3]);
    store<u8>(o + 128, static_cast<u8>(hit));
    if (hit == 0) return;
    SetF1Bits(ctx, load<u32>(kK17F8D0 + 16));
    if (F(o + 116) < F(kK17F8D0 + 16)) {
        store<u8>(o + 128, 0);
        ctx.gpr[3] = 0;
        return;
    }
    CopyW(o + 16, to);
    CopyW(o + 20, to + 4);
    CopyW(o + 24, to + 8);
    SetF1Bits(ctx, load<u32>(to + 8));
    ctx.gpr[3] = kK17F8D0;
}
