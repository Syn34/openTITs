// Game code, VMX sweep part 2 (see vmx2.h): hit-event handlers that build
// impulse vectors with the compiler's splat / vsel / vandc pattern.
#include "../sly4.h"
#include "vmx2.h"

using namespace sly4;
using namespace sly4::vmx2;

namespace {
constexpr u32 kObjTestHit = 0x000E0BCC;   // Obj_TestE0BCC (Obj*, const HitEvent*): false when queued / ignored
constexpr u32 kHkActivate = 0x0091A4C8;   // Hk_Activate91A4C8 (hkpRigidBody*)
constexpr u32 kTrophyOnMsg = 0x000B2AE0;  // Trophy_OnMsg_000B2AE0 (Obj*, const HitEvent*)
constexpr u32 kHitConst2 = 0x001FBC40;    // 0.0f, then the lane masks
constexpr u32 kDamageHash2 = 0x045D9554;  // passed to the damage virtual
constexpr u32 kHitFlagBlocked = 0x0100;   // ev +14
constexpr u32 kHitFlagArmed = 0x0200;     // ev +14: the refused-hit test applies
}  // namespace

// u32 f(Obj* o, HitEvent* ev) (a callback): o takes a hit. Ignored when
// the def (+216) +300 has 0x20000000 and ev +12 has 0x1000 (r3 0x1000).
// The strength s = ev+64 * def+232. When Obj_TestE0BCC(o, ev) refuses it,
// only ev+14 bit 0x100 = (s >= o->+348), and only when its bit 0x200 is
// set (r3 the flags). Else, with a body (+416) not of kind 4 / 5, ev+68 and
// def+236 both != 0: the impulse -ev(+32..40) * ev+68 * def+236 at the
// point ev+16..24 (both as {x, y, z, 0} rows, vsel masks after 0.0) is
// applied: Hk_Activate91A4C8(body), (body+224)->vfn[22](body+224, &imp,
// &pt). With s > 0, r = o->vfn[88](o, <entry>, ev+4, 0x045D9554; f1 s)
// and ev+14 bit 0x100 = (r == 0). Then Trophy_OnMsg_000B2AE0(o, ev) (its
// r3). f1 as the original leaves it.
DECOMP(001FBC80, Obj_OnHitPush_001FBC80) {
    const u32 o = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    const u32 def = load<u32>(o + 216);
    if ((load<u32>(def + 300) & 0x20000000u) != 0 && (load<u16>(ev + 12) & 0x1000u) != 0) {
        ctx.gpr[3] = 0x1000;
        return;
    }
    const f32 s = Muls(F(ev + 64), F(def + 232));
    SetF1Bits(ctx, load<u32>(def + 232));
    if (static_cast<s32>(call(ctx, kObjTestHit, {o, ev})) == 0) {
        u32 h = load<u16>(ev + 14);
        if ((h & kHitFlagArmed) != 0) {
            SetF1Bits(ctx, load<u32>(o + 348));
            h = (h & ~kHitFlagBlocked) | (s >= F(o + 348) ? kHitFlagBlocked : 0u);
            store<u16>(ev + 14, static_cast<u16>(h));
        }
        ctx.gpr[3] = h;
        return;
    }
    const f32 zero = F(kHitConst2);
    const u32 body = load<u32>(o + 416);
    if (body != 0) {
        const u8 kind = load<u8>(body + 232);
        if (kind != 5 && kind != 4 && F(ev + 68) != zero && F(def + 236) != zero) {
            const f32 a = F(ev + 68), k = F(def + 236);
            const f32 x = Muls(Muls(-F(ev + 32), a), k);
            const f32 y = Muls(Muls(-F(ev + 36), a), k);
            const f32 z = Muls(Muls(-F(ev + 40), a), k);
            Stack frame(ctx, 208);
            const u32 sp = frame.addr(), imp = sp + 112, pt = sp + 128;
            const Masks3 m = LoadMasks3(kHitConst2 + 16);
            Stvx(sp + 144, Fill4(z));
            Stvx(imp, RowSel(Fill4(x), Fill4(y), Fill4(z), m));
            Stvx(pt, RowSelW(ev + 16, ev + 20, ev + 24, m));
            call(ctx, kHkActivate, {body});
            VCall(ctx, body + 224, 88, {body + 224, imp, pt});
        }
    }
    SetF1Bits(ctx, load<u32>(kHitConst2));
    if (s > zero) {
        ctx.fpr[1] = s;
        const u32 opd = load<u32>(load<u32>(o) + 352);
        const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(ev + 4), kDamageHash2, load<u32>(opd)}));
        const u32 cz = static_cast<u32>(std::countl_zero(r));
        store<u16>(ev + 14, static_cast<u16>((load<u16>(ev + 14) & ~kHitFlagBlocked) | (std::rotl(cz, 3) & kHitFlagBlocked)));
    }
    call(ctx, kTrophyOnMsg, {o, ev});
}
