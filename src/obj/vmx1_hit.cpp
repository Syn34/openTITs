// Game code, VMX sweep part 1 (see vmx1.h): hit-event handlers that build
// an impulse vector with the compiler's splat / vsel / vandc pattern.
#include "../sly4.h"
#include "vmx1.h"

using namespace sly4;
using namespace sly4::vmx1;

namespace {
constexpr u32 kObjTestHit = 0x000E0BCC;   // bool (Obj*, const HitEvent*): false when queued / ignored
constexpr u32 kHkActivate = 0x0091A4C8;   // (hkpRigidBody*)
constexpr u32 kHitConst = 0x001FC1C0;     // 0.0f, 1.0f (then the lane masks)
constexpr u32 kHitKindFull = 0x04E8CCF7;  // the hit kind that always counts as strength 1
constexpr u32 kDamageHash = 0x811B1D5E;   // passed to the damage virtual
}  // namespace

// u32 f(Obj* o, HitEvent* ev) (a callback): when Obj_TestE0BCC(o, ev)
// accepts the hit: with a body o->+416 whose +232 kind isn't 4 / 5, a
// non-zero ev->+36 and def (o->+216) +264 impulse scale, the impulse ev
// direction (+16..24) * ev->+36 * scale is applied (Hk_Activate91A4C8, then
// (body+224)->vfn[21](body+224, &v)). Then the strength ev->+32 * (kind
// ev->+8 == kHitKindFull ? 1 : def +260); when > 0, r = o->vfn[88](o,
// <entry>, ev->+4, kDamageHash) and ev->+14 bit 0x100 = (r == 0). r3: 0
// (rejected), cntlzw(r), or the constant pool address (the original's).
DECOMP(001FC200, Obj_OnHitImpulse_001FC200) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ev = arg<u32>(ctx, 1);
    const u32 def = load<u32>(o + 216);
    if (static_cast<s32>(call(ctx, kObjTestHit, {o, ev})) == 0) return;
    const f32 zero = F(kHitConst);
    const u32 body = load<u32>(o + 416);
    if (body != 0) {
        const u8 kind = load<u8>(body + 232);
        const f32 s = F(ev + 36);
        if (kind != 5 && kind != 4 && s != zero && F(def + 264) != zero) {
            const f32 k = F(def + 264);
            const f32 x = Muls(Muls(F(ev + 16), s), k);
            const f32 y = Muls(Muls(F(ev + 20), s), k);
            const f32 z = Muls(Muls(F(ev + 24), s), k);
            Stack frame(ctx, 32);
            const u32 v = frame.addr();  // the original's sp+112
            Stvx(v, V4{{x, y, z, 0.0f}});  // splat / vsel / vandc: w cleared
            call(ctx, kHkActivate, {body});
            VCall(ctx, body + 224, 84, {body + 224, v});
        }
    }
    f32 m = F(def + 260);
    if (load<u32>(ev + 8) == kHitKindFull) m = F(kHitConst + 4);
    const f32 strength = Muls(F(ev + 32), m);
    ctx.fpr[1] = strength;
    if (!(strength > zero)) {
        ret(ctx, kHitConst);
        return;
    }
    const u32 opd = load<u32>(load<u32>(o) + 352);
    // r4 holds the entry address (left from the OPD load), as the original.
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, load<u32>(opd), load<u32>(ev + 4), kDamageHash, opd}));
    const u32 cz = static_cast<u32>(std::countl_zero(r));
    store<u16>(ev + 14, static_cast<u16>((load<u16>(ev + 14) & ~0x100u) | (std::rotl(cz, 3) & 0x100u)));
    ret(ctx, cz);
}

namespace {
constexpr u32 kHitQueue = 0x01022100;       // 512 records of 128 bytes (+116 the object)
constexpr u32 kHitQueueCount = 0x01022080;  // s32
constexpr u32 kObjShutdownBase = 0x000D74C0;  // (o, u8 full)
constexpr u32 kEffectFindComponent = 0x004BB4B4;  // (Effect*, u32 type hash) -> component or 0
constexpr u32 kEffectRelease = 0x004CF3C4;  // (Effect*, u32 hash) -> bool
constexpr u32 kEffectKindKeep = 0x879F2BDA;   // effect def kind left running
constexpr u32 kEffectEndHash = 0x879D1B7A;    // the def switched to otherwise
constexpr u32 kFadeComponent = 0x0027469E;    // component type hash
}  // namespace

// void f(Obj* o, u8 full) (tail of the ragdoll / object releases): with
// full, o's records are dropped from the hit event queue (swap with the
// last record: 128 bytes copied with lvx / stvx, the count stored, the slot
// tested again), and o's effect (+356), when set, is ended: unless its def
// (+4) kind (+8) is kEffectKindKeep, +136 |= 0x100 and
// Effect_Release(e, kEffectEndHash); its kFadeComponent component in state
// 5 (vfn[8]) gets +36 = 0; the effect's +136 |= 0x200 and o->+356 = 0.
// o->+364 = 0. Then Obj_ShutdownBase_000D74C0(o, full) (its r3 left).
DECOMP(000E02F8, Obj_SetRagdollBase) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 full = arg<u32>(ctx, 1) & 0xFF;
    if (full != 0) {
        s32 n = load<s32>(kHitQueueCount);
        u32 last = kHitQueue + (static_cast<u32>(n) << 7);
        for (s32 i = 0; i < n; ++i) {
            const u32 e = kHitQueue + 128u * static_cast<u32>(i);
            if (load<u32>(e + 116) != o) continue;
            last -= 128;
            --n;
            for (u32 h = 0; h < 128; h += 64) {  // four lvx, then four stvx
                u32 w[16];
                for (u32 k = 0; k < 16; ++k) w[k] = load<u32>(last + h + 4 * k);
                for (u32 k = 0; k < 16; ++k) store<u32>(e + h + 4 * k, w[k]);
            }
            store<s32>(kHitQueueCount, n);
            --i;  // the moved record is tested next
        }
        const u32 fx = load<u32>(o + 356);
        if (fx != 0) {
            const u32 def = load<u32>(fx + 4);
            const u32 kind = def != 0 ? load<u32>(def + 8) : 0;
            u32 cur = fx;
            if (kind != kEffectKindKeep) {
                store<u32>(fx + 136, load<u32>(fx + 136) | 0x100);
                call(ctx, kEffectRelease, {load<u32>(o + 356), kEffectEndHash});
                cur = load<u32>(o + 356);
            }
            const u32 c = static_cast<u32>(call(ctx, kEffectFindComponent, {cur, kFadeComponent}));
            if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) store<u32>(c + 36, 0);
            const u32 fx2 = load<u32>(o + 356);
            store<u32>(fx2 + 136, load<u32>(fx2 + 136) | 0x200);
            store<u32>(o + 356, 0);
        }
        store<u32>(o + 364, 0);
    }
    call(ctx, kObjShutdownBase, {o, full});
}

namespace {
constexpr u32 kHitConst2 = 0x00208480;     // 1.0f, 0.0f (then the lane masks)
constexpr u32 kImpulseScale = 0x01054C58;  // f32 global impulse scale
constexpr u32 kDamageScale = 0x01054C08;   // f32 global damage scale
constexpr u32 kDamageHash2 = 0x03C599D8;
}  // namespace

// u32 f(Obj* o, HitEvent* ev) (a callback): when Obj_TestE0BCC(o, ev)
// accepts the hit: falloff t = 1, or (ev->+48 - ev->+96) / (ev->+48 -
// ev->+52) when +48 > +52 and +96 > +52. Impulse ev->+64 * kImpulseScale *
// t along ev->+80..88 to the body o->+388 (unless its +232 kind is 4 / 5,
// o->+400 bit 28 is set or the impulse is 0; the kind is read without a null
// check): Hk_Activate91A4C8, then
// (body+224)->vfn[21](body+224, &v). Damage ev->+60 * t * kDamageScale;
// when > 0: o->vfn[88](o, <OPD>, ev->+4, kDamageHash2; f1 damage). Then
// ev->+14 |= 0x100 (r3 = the new value; 0 when rejected).
DECOMP(002084C0, Obj_OnHitFalloff_002084C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ev = arg<u32>(ctx, 1);
    if (static_cast<s32>(call(ctx, kObjTestHit, {o, ev})) == 0) return;
    const f32 far = F(ev + 48), near = F(ev + 52);
    f32 t = F(kHitConst2);
    if (far > near) {
        const f32 d = F(ev + 96);
        if (d > near) t = Divs(Subs(far, d), Subs(far, near));
    }
    const f32 imp = Muls(Muls(F(ev + 64), F(kImpulseScale)), t);
    const u32 body = load<u32>(o + 388);
    const u8 kind = load<u8>(body + 232);
    const f32 zero = F(kHitConst2 + 4);
    if (kind != 5 && kind != 4 && ((load<u32>(o + 400) >> 28) & 1) == 0 && imp != zero) {
        const f32 x = Muls(F(ev + 80), imp), y = Muls(F(ev + 84), imp), z = Muls(F(ev + 88), imp);
        Stack frame(ctx, 32);
        const u32 v = frame.addr();  // the original's sp+112
        Stvx(v, V4{{x, y, z, 0.0f}});  // splat / vsel / vandc: w cleared
        call(ctx, kHkActivate, {body, kHitConst2});
        VCall(ctx, body + 224, 84, {body + 224, v});
    }
    const f32 dmg = Muls(Muls(F(ev + 60), t), F(kDamageScale));
    ctx.fpr[1] = F(kDamageScale);
    if (dmg > F(kHitConst2 + 4)) {
        const u32 opd = load<u32>(load<u32>(o) + 352);
        ctx.fpr[1] = dmg;
        // r4 holds the OPD and r7 the entry, as the original.
        call_ptr(ctx, opd, {o, opd, load<u32>(ev + 4), kDamageHash2, load<u32>(opd)});
    }
    const u32 h = load<u16>(ev + 14) | 0x100u;
    store<u16>(ev + 14, static_cast<u16>(h));
    ret(ctx, h);
}

namespace {
constexpr u32 kHitConst3 = 0x00192990;  // 1.0f, 0.0f (then the lane masks)
constexpr u32 kHitConst4 = 0x002086F0;  // 0.0f, 1.0f (then the lane masks)
constexpr u32 kImpulseScale2 = 0x01054CF8;  // f32 global impulse scale
constexpr u32 kDamageScale2 = 0x01054CA8;   // f32 global damage scale

// The impulse v (x, y, z, 0 from the splat / vsel / vandc pattern) on the
// stack: Hk_Activate91A4C8(body, <r4 = the constant pool>), then
// (body+224)->vfn[21](body+224, &v).
void ApplyImpulse(PPUContext& ctx, u32 body, f32 x, f32 y, f32 z, u32 pool) {
    Stack frame(ctx, 32);
    const u32 v = frame.addr();  // the original's sp+112
    Stvx(v, V4{{x, y, z, 0.0f}});
    call(ctx, kHkActivate, {body, pool});
    VCall(ctx, body + 224, 84, {body + 224, v});
}
}  // namespace

// u32 f(Obj* o, HitEvent* ev) (a callback): when Obj_TestE0BCC(o, ev)
// accepts the hit: falloff t = 1, or (ev->+48 - ev->+96) / (ev->+48 -
// ev->+52) when +96 > +52 and +48 > +52. With a body o->+388 (kind +232
// not 4 / 5), a non-zero ev->+64 and def (o->+216) +208 > 0: impulse ev
// direction (+80..88) * ev->+64 * def+208 * t. Damage ev->+60 * t * def
// +204; when > 0: o->vfn[88](o, <OPD>, ev->+4, kDamageHash2; f1 damage).
// Then ev->+14 |= 0x100 (r3 = the new value; 0 when rejected).
DECOMP(001929D0, Obj_OnHitScaled_001929D0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ev = arg<u32>(ctx, 1);
    const u32 def = load<u32>(o + 216);
    if (static_cast<s32>(call(ctx, kObjTestHit, {o, ev})) == 0) return;
    const f32 d = F(ev + 96), near = F(ev + 52);
    f32 t = F(kHitConst3);
    if (d > near) {
        const f32 far = F(ev + 48);
        if (far > near) t = Divs(Subs(far, d), Subs(far, near));
    }
    const f32 zero = F(kHitConst3 + 4);
    const u32 body = load<u32>(o + 388);
    if (body != 0) {
        const u8 kind = load<u8>(body + 232);
        const f32 s = F(ev + 64);
        if (kind != 5 && kind != 4 && s != zero && F(def + 208) > zero) {
            const f32 k = F(def + 208);
            const f32 x = Muls(Muls(Muls(F(ev + 80), s), k), t);
            const f32 y = Muls(Muls(Muls(F(ev + 84), s), k), t);
            const f32 z = Muls(Muls(Muls(F(ev + 88), s), k), t);
            ApplyImpulse(ctx, body, x, y, z, kHitConst3);
        }
    }
    const f32 dmg = Muls(Muls(F(ev + 60), t), F(def + 204));
    ctx.fpr[1] = F(def + 204);
    if (dmg > F(kHitConst3 + 4)) {
        const u32 opd = load<u32>(load<u32>(o) + 352);
        ctx.fpr[1] = dmg;
        call_ptr(ctx, opd, {o, opd, load<u32>(ev + 4), kDamageHash2, load<u32>(opd)});  // r4 = OPD, r7 = entry
    }
    const u32 h = load<u16>(ev + 14) | 0x100u;
    store<u16>(ev + 14, static_cast<u16>(h));
    ret(ctx, h);
}

// u32 f(Obj* o, HitEvent* ev) (a callback): when Obj_TestE0BCC(o, ev)
// accepts the hit: a kHitKindFull hit from a source (ev->+4) with +192 bit
// 28 calls o->vfn[7](o, 0, 4) and stops there. Else the impulse ev->+36 *
// kImpulseScale2 along ev->+16..24 goes to the body o->+388 (kind +232 not
// 4 / 5; not when 0), and the strength ev->+32 * (source with +192 bit 28 ?
// 1 : kDamageScale2) when > 0 gives r = o->vfn[88](o, <OPD>, source,
// kDamageHash) and ev->+14 bit 0x100 = (r == 0). r3: 0 (rejected), the
// virtual's result, cntlzw(r) or the source; f1 the strength.
DECOMP(00208730, Obj_OnHitSourced_00208730) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ev = arg<u32>(ctx, 1);
    if (static_cast<s32>(call(ctx, kObjTestHit, {o, ev})) == 0) return;
    auto flagged = [](u32 src) { return src != 0 && (load<u32>(src + 192) & 0x10000000u) != 0; };
    u32 src = load<u32>(ev + 4);
    if (flagged(src) && load<u32>(ev + 8) == kHitKindFull) {
        VCall(ctx, o, 28, {o, 0, 4});
        return;
    }
    const f32 imp = Muls(F(ev + 36), F(kImpulseScale2));
    const u32 body = load<u32>(o + 388);
    if (body != 0) {
        const u8 kind = load<u8>(body + 232);
        if (kind != 5 && kind != 4 && imp != F(kHitConst4)) {
            ApplyImpulse(ctx, body, Muls(F(ev + 16), imp), Muls(F(ev + 20), imp), Muls(F(ev + 24), imp), kHitConst4);
            src = load<u32>(ev + 4);
        }
    }
    const f32 m = flagged(src) ? F(kHitConst4 + 4) : F(kDamageScale2);
    const f32 strength = Muls(F(ev + 32), m);
    ctx.fpr[1] = strength;
    if (!(strength > F(kHitConst4))) {
        ret(ctx, src);
        return;
    }
    const u32 opd = load<u32>(load<u32>(o) + 352);
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, opd, src, kDamageHash, load<u32>(opd)}));  // r4 = OPD, r7 = entry
    const u32 cz = static_cast<u32>(std::countl_zero(r));
    store<u16>(ev + 14, static_cast<u16>((load<u16>(ev + 14) & ~0x100u) | (std::rotl(cz, 3) & 0x100u)));
    ret(ctx, cz);
}

namespace {
constexpr u32 kHitConst5 = 0x0016FA20;  // 1.0f, 0.0f, 180.0f, pi, 360.0f (then the lane masks at +32)
constexpr u32 kAtan2f = 0x005E665C;     // f1 = atan2f(f1 y, f2 x)
}  // namespace

// u32 f(Obj* o, HitEvent* ev) (a callback, no hit test): the same falloff
// and impulse as Obj_OnHitScaled_001929D0 (direction ev->+80..88 *
// ev->+64 * def+208 * t; Hk_Activate91A4C8 gets r4 = 32). When def
// (o->+216) +204 > 0: o->vfn[88](o, <OPD>, ev->+4, kDamageHash2; f1 =
// ev->+60 * t * def+204) and o->+736 = the heading of the hit (atan2f of
// ev->+16..20 - o->+144..148, in degrees wrapped to [0, 360]). Then
// ev->+14 |= 0x100 (r3 = the new value).
DECOMP(0016FA70, Obj_OnHitHeading_0016FA70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ev = arg<u32>(ctx, 1);
    const f32 d = F(ev + 96), near = F(ev + 52);
    f32 t = F(kHitConst5);
    const u32 def = load<u32>(o + 216);
    const f32 s = F(ev + 64);
    const u32 body = load<u32>(o + 388);
    if (d > near) {
        const f32 far = F(ev + 48);
        if (far > near) t = Divs(Subs(far, d), Subs(far, near));
    }
    const f32 zero = F(kHitConst5 + 4);
    if (body != 0) {
        const u8 kind = load<u8>(body + 232);
        if (kind != 5 && kind != 4 && s != zero && F(def + 208) > zero) {
            const f32 k = F(def + 208);
            const f32 x = Muls(Muls(Muls(F(ev + 80), s), k), t);
            const f32 y = Muls(Muls(Muls(F(ev + 84), s), k), t);
            const f32 z = Muls(Muls(Muls(F(ev + 88), s), k), t);
            ApplyImpulse(ctx, body, x, y, z, 32);
        }
    }
    const f32 ds = F(def + 204);
    ctx.fpr[1] = ds;
    if (ds > zero) {
        const u32 opd = load<u32>(load<u32>(o) + 352);
        ctx.fpr[1] = Muls(Muls(F(ev + 60), t), ds);
        call_ptr(ctx, opd, {o, opd, load<u32>(ev + 4), kDamageHash2, load<u32>(opd)});  // r4 = OPD, r7 = entry
        const f32 dx = Subs(F(ev + 16), F(o + 144)), dy = Subs(F(ev + 20), F(o + 148));
        const f64 a = callf(ctx, kAtan2f, {o + 144}, {dy, dx});
        f32 h = Divs(Muls(a, F(kHitConst5 + 8)), F(kHitConst5 + 12));
        const f32 full = F(kHitConst5 + 16);
        if (h < zero) h = Adds(h, full);
        if (h > full) h = Subs(h, full);
        store<f32>(o + 736, h);
        ctx.fpr[1] = full;
    }
    const u32 hv = load<u16>(ev + 14) | 0x100u;
    store<u16>(ev + 14, static_cast<u16>(hv));
    ret(ctx, hv);
}

namespace {
constexpr u32 kHitConst6 = 0x00184E20;   // 0.0f (then the lane masks)
constexpr u32 kDamageHash3 = 0x045D9554;
}  // namespace

// u32 f(Obj* o, HitEvent* ev) (a callback): rate-limited hit: nothing
// while the cooldown o->+1612 is above 0 (r3 = o); else the cooldown is
// set to 0.25 and, when Obj_TestE0BCC(o, ev) accepts the hit: a body
// o->+388 (kind +232 not 4 / 5) with non-zero ev->+68 and def (o->+216)
// +200 gets the impulse -(ev->+32..40) * ev->+68 * def+200 at the point
// ev->+16..24 (Hk_Activate91A4C8, then (body+224)->vfn[22](body+224, &v,
// &p)); with def +196 > 0, r = o->vfn[88](o, <OPD>, ev->+4, kDamageHash3;
// f1 = ev->+64 * def+196) and ev->+14 bit 0x100 = (r == 0). r3 / f1 as the
// original leaves them.
DECOMP(00184E60, Obj_OnHitCooldown_00184E60) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 ev = arg<u32>(ctx, 1);
    const f32 zero = F(kHitConst6);
    const u32 def = load<u32>(o + 216);
    SetF1Bits(ctx, load<u32>(o + 1612));
    if (F(o + 1612) > zero) {
        ret(ctx, o);
        return;
    }
    store<u32>(o + 1612, 0x3E800000u);  // 0.25f
    if (static_cast<s32>(call(ctx, kObjTestHit, {o, ev})) == 0) return;
    const u32 body = load<u32>(o + 388);
    if (body != 0) {
        const u32 kind = load<u8>(body + 232);
        ctx.gpr[3] = kind;  // r3 as the kind tests leave it
        if (kind != 5) {
            ctx.gpr[3] = kind - 4;
            const f32 s = F(ev + 68);
            if (kind != 4 && s != zero && F(def + 200) != zero) {
                const f32 k = F(def + 200);
                const f32 x = Muls(Muls(-F(ev + 32), s), k);
                const f32 y = Muls(Muls(-F(ev + 36), s), k);
                const f32 z = Muls(Muls(-F(ev + 40), s), k);
                Stack frame(ctx, 32);
                const u32 v = frame.addr(), p = v + 16;  // the original's sp+112 / sp+128
                Stvx(v, V4{{x, y, z, 0.0f}});
                Stvx(p, V4{{Lvlx(ev + 16).e[0], Lvlx(ev + 20).e[0], Lvlx(ev + 24).e[0], 0.0f}});
                call(ctx, kHkActivate, {body, ev + 24});
                VCall(ctx, body + 224, 88, {body + 224, v, p});
            }
        }
    }
    const f32 ds = F(def + 196);
    ctx.fpr[1] = ds;
    if (!(ds > zero)) return;
    const u32 opd = load<u32>(load<u32>(o) + 352);
    ctx.fpr[1] = Muls(F(ev + 64), ds);
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(ev + 4), kDamageHash3, load<u32>(opd)}));  // r4 = OPD, r7 = entry
    const u32 cz = static_cast<u32>(std::countl_zero(r));
    store<u16>(ev + 14, static_cast<u16>((load<u16>(ev + 14) & ~0x100u) | (std::rotl(cz, 3) & 0x100u)));
    ret(ctx, cz);
}
