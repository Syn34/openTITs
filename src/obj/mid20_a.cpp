// Game code, medium sweep part 20: the base message handler, the thrown-
// object flight states of Obj_Tick3CB968, and part release / effect rules.
#include "mid20.h"

using namespace sly4;
using namespace sly4::mid20;

namespace {
constexpr u32 kSetPhysicsActive = 0x00260C14u;  // Obj_SetPhysicsActive(body, bool)
constexpr u32 kNotify2E2E78 = 0x002E2E78u;      // (*(u32*)0x00F64C20)
constexpr u32 kEffectSpawn = 0x004A5FC8u;       // Effect_Spawn(hash, a, b, c)
constexpr u32 kFromRotPos = 0x00553B4Cu;        // Mat34_FromRotPos553B4C(out, rot, pos)
constexpr u32 kFindComponent = 0x004BB4B4u;     // Effect_FindComponent(e, hash)
constexpr u32 kFindById = 0x000D6A28u;          // Obj_FindById(id)
constexpr u32 kFindByHash = 0x000DB290u;        // Obj_FindByHash_000DB290(o, hash)
constexpr u32 kReleaseChild = 0x001ED85Cu;      // Obj_ReleaseChild_001ED85C(o, child)
constexpr u32 kGetDef = 0x000D951Cu;            // Target_GetDef0D951C(o)
constexpr u32 kDropHeld = 0x002B7704u;          // Obj_DropHeld_002B7704(o)
constexpr u32 kVCall404 = 0x001BD450u;          // Field_VCall404_10_001BD450(x, 0)
constexpr u32 kBounceFx = 0x1BE27372u;          // effect hash of the landing / bounce

// The shared tail of the flight states: the body b = o->108 gets the pose
// made of its own rotation rows (b+96 / b+112 / b+128, copied to the stack
// at sp+112 / +128 / +144) and the position o+32:
// Mat34_FromRotPos553B4C(sp+160, sp+112, o+32), then b->vfn[84](b, sp+160).
// r3 = that call's result; f1 = the last word copied, as the original
// leaves it.
void FlightPose(PPUContext& ctx, u32 o, u32 b, u32 sp) {
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k) Cw(sp + 112 + 16 * row + 4 * k, b + 96 + 16 * row + 4 * k);
    ctx.fpr[1] = Lf(b + 136);
    call(ctx, kFromRotPos, {sp + 160, sp + 112, o + 32});
    ctx.gpr[3] = VCall(ctx, b, 84, {b, sp + 160});
}

// Ballistic step of the flight states with the constants at C: velocity
// o+48 += (c0, c1, c0) * dt and position o+32 += v * dt + (c0, c1, c0) *
// c2 * dt * dt (with the old velocity), in the original's store order.
void FlightIntegrate(u32 o, f64 dt, u32 C) {
    const f64 c0 = Lf(C), c1 = Lf(C + 4), c2 = Lf(C + 8);
    const f64 v0 = Lf(o + 48), v1 = Lf(o + 52), v2 = Lf(o + 56);
    const f64 hx = S(c0 * c2), hy0 = S(c1 * c2);
    const f64 nv1 = Fmadds(c1, dt, v1);
    const f64 p0 = Fmadds(v0, dt, Lf(o + 32));
    const f64 nv0 = Fmadds(c0, dt, v0);
    const f64 p1 = Fmadds(v1, dt, Lf(o + 36));
    const f64 p2 = Fmadds(v2, dt, Lf(o + 40));
    const f64 nv2 = Fmadds(c0, dt, v2);
    const f64 ax = S(hx * dt), ay = S(hy0 * dt);
    Sf(o + 48, nv0);
    Sf(o + 52, nv1);
    Sf(o + 56, nv2);
    Sf(o + 32, Fmadds(ax, dt, p0));
    Sf(o + 36, Fmadds(ay, dt, p1));
    Sf(o + 40, Fmadds(ax, dt, p2));
}
}  // namespace

// u32 Obj_OnMsgBase133FC(Obj* o, Msg* m): the base message handler most
// object handlers end in. By the hash m->8: 0x7B5D8742 / 0xFB303547 set /
// clear bit 0x20 of the byte o->304; 0x76502F44 / 0x55BE125F set / clear
// bit 0x01 of the byte o->305 and Obj_SetPhysicsActive(o->68, 1 / 0) (both
// only with a body o->68); 0x9087CCEB / 0x9AB924F0: x = o->48->vfn[68](),
// x->vfn[44](x, 1 / 0); 0x9A986484: 0x002E2E78(*(u32*)0x00F64C20). r3 = o,
// or the last call's result.
DECOMP(000133FC, Obj_OnMsgBase133FC) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    u64 r3 = o;
    switch (load<u32>(m + 8)) {
    case 0x7B5D8742u: store<u8>(o + 304, static_cast<u8>(load<u8>(o + 304) | 0x20)); break;
    case 0xFB303547u: store<u8>(o + 304, static_cast<u8>(load<u8>(o + 304) & ~0x20)); break;
    case 0x76502F44u:
    case 0x55BE125Fu: {
        const bool on = load<u32>(m + 8) == 0x76502F44u;
        const u32 p = load<u32>(o + 68);
        if (p != 0) {
            const u8 v = load<u8>(o + 305);
            store<u8>(o + 305, static_cast<u8>(on ? (v | 0x01) : (v & ~0x01)));
            r3 = call(ctx, kSetPhysicsActive, {p, on ? 1u : 0u});
        }
        break;
    }
    case 0x9087CCEBu:
    case 0x9AB924F0u: {
        const u32 on = load<u32>(m + 8) == 0x9087CCEBu ? 1 : 0;
        const u32 h = load<u32>(o + 48);
        const u32 x = static_cast<u32>(VCall(ctx, h, 68, {h}));
        r3 = VCall(ctx, x, 44, {x, on});
        break;
    }
    case 0x9A986484u: r3 = call(ctx, kNotify2E2E78, {load<u32>(0x00F64C20)}); break;
    default: break;
    }
    ctx.gpr[3] = r3;
}

// Obj_ArcTick3CB260(Obj* o; f32 dt): state 6 of Obj_Tick3CB968, the arc
// from o+64 to o+80. The clock o->96 += dt, t = o->96 / o->100; past 1.0:
// o->52 = 2.5f, state o->0 = 5 and Effect_Spawn(0x1BE27372, 0, 0, 1) (r3 =
// its result, f1 = dt). Else the position o+32 = lerp(o+64, o+80, t) with
// y + 1 - (2t - 1)^2, the velocity o+48 = (position - body position b+144)
// / dt (b = o->108), then the body pose (FlightPose).
DECOMP(003CB260, Obj_ArcTick3CB260) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x003CB254u;  // 1.0f, 2.0f
    Stack frame(ctx, 128);
    const u32 sp = frame.addr() - 112;  // the original's r1: locals at their original offsets
    const f64 tm = S(Lf(o + 96) + dt);
    const f64 t = Divs(tm, Lf(o + 100));
    Sf(o + 96, tm);
    const f64 one = Lf(C);
    if (t > one) {
        store<u32>(o + 52, 0x40200000u);
        store<u32>(o, 5);
        ctx.gpr[3] = call(ctx, kEffectSpawn, {kBounceFx, 0, 0, 1});
        return;
    }
    const f64 inv = Divs(one, dt);
    const f64 dy = S(Lf(o + 84) - Lf(o + 68));
    const f64 u = Fmsubs(t, Lf(C + 4), one);
    const f64 dx = S(Lf(o + 80) - Lf(o + 64));
    const f64 dz = S(Lf(o + 88) - Lf(o + 72));
    const u32 b = load<u32>(o + 108);
    const f64 y0 = Fmadds(dy, t, Lf(o + 68));
    const f64 h = Fnmsubs(u, u, one);
    const f64 x = Fmadds(dx, t, Lf(o + 64));
    const f64 z = Fmadds(dz, t, Lf(o + 72));
    const f64 bx = Lf(b + 144), by = Lf(b + 148), bz = Lf(b + 152);
    const f64 y = S(y0 + h);
    const f64 vx = S(x - bx), vz = S(z - bz);
    Sf(o + 32, x);
    Sf(o + 40, z);
    const f64 vy = S(y - by);
    Sf(o + 36, y);
    Sf(o + 48, S(vx * inv));
    Sf(o + 52, S(vy * inv));
    Sf(o + 56, S(vz * inv));
    FlightPose(ctx, o, b, sp);
}

// Obj_FallTick3CB808(Obj* o; f32 dt): state 5 of Obj_Tick3CB968: ballistic
// step under gravity -10 (FlightIntegrate with 0.0 / -10.0 / 0.5), the
// height o->36 clamped to >= 0.0 (stored as the word 0), then the body
// pose (FlightPose).
DECOMP(003CB808, Obj_FallTick3CB808) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x003CB7F8u;  // 0.0f, -10.0f, 0.5f
    Stack frame(ctx, 128);
    const u32 sp = frame.addr() - 112;
    FlightIntegrate(o, dt, C);
    if (Lf(o + 36) < Lf(C)) store<u32>(o + 36, 0);
    FlightPose(ctx, o, load<u32>(o + 108), sp);
}

// Obj_BounceTick3CB430(Obj* o; f32 dt): state 4 of Obj_Tick3CB968: the
// same ballistic step; below the floor o->36 < 1.0 the height is set to
// 1.0, o->48 = 0 and a falling o->52 (< 0.0) is reflected by -0.9 (stored)
// with Effect_Spawn(0x1BE27372, 0, 0, 1) when the bounce exceeds 0.5. Then
// the body pose (FlightPose).
DECOMP(003CB430, Obj_BounceTick3CB430) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x003CB418u;  // 0.0f, -10.0f, 0.5f, 1.0f, -0.9f
    Stack frame(ctx, 144);
    const u32 sp = frame.addr() - 112;
    FlightIntegrate(o, dt, C);
    if (Lf(o + 36) < Lf(C + 12)) {
        store<u32>(o + 36, 0x3F800000u);
        store<u32>(o + 48, 0);
        const f64 vy = Lf(o + 52);
        if (vy < Lf(C)) {
            const f64 nv = S(vy * Lf(C + 16));
            Sf(o + 52, nv);
            if (nv > Lf(C + 8)) call(ctx, kEffectSpawn, {kBounceFx, 0, 0, 1});
        }
    }
    FlightPose(ctx, o, load<u32>(o + 108), sp);
}

// EffectRule_Enable8088C(Rule* a, Effect* e) (vtable entry): the target:
// the kind-5 component c = Effect_FindComponent(e, a->12) (c->vfn[32]())
// whose +28 has the +276 key 0x02235C33 (target = +280), else
// Obj_FindById(a->12). For a target with +172 bit 0x40000, optionally
// Obj_FindByHash_000DB290(t, a->16) (when a->16); then a target with +172
// bit 0x40000 gets vfn[352](t, 0, 0; f1 = t->344) when its +192 has bit
// 0x02 and the byte a->20 is set, else vfn[40](t, 1). With the component
// c and neither a->16 nor the byte a->21: c->28->vfn[8]() (when set) and
// c->28 = c->36 = 0. r3 as the original leaves it.
DECOMP(0008088C, EffectRule_Enable8088C) {
    const u32 a = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 keep = 0;  // r31: the component
    u32 t = 0;     // r29
    u32 r3 = static_cast<u32>(call(ctx, kFindComponent, {e, load<u32>(a + 12)}));
    const u32 c = r3;
    bool byId = true;
    if (c != 0) {
        r3 = static_cast<u32>(VCall(ctx, c, 32, {c}));
        if (r3 == 5) {
            r3 = load<u32>(c + 28);
            if (r3 != 0) {
                keep = c;
                if (load<u32>(r3 + 276) == 0x02235C33u) t = load<u32>(r3 + 280);
                if (t != 0) byId = false;
            }
        }
    }
    bool have = true;
    if (byId) {
        r3 = static_cast<u32>(call(ctx, kFindById, {load<u32>(a + 12)}));
        t = r3;
        have = t != 0;
    }
    if (have) {
        r3 = (load<u32>(t + 172) >> 18) & 1;
        if (r3 != 0) {
            r3 = load<u32>(a + 16);
            if (r3 != 0) {
                r3 = static_cast<u32>(call(ctx, kFindByHash, {t, r3}));
                t = r3;
            }
        }
        if (t != 0) {
            r3 = (load<u32>(t + 172) >> 18) & 1;
            if (r3 != 0) {
                r3 = load<u32>(t + 192) & 2;
                bool restart = false;
                if (r3 != 0) {
                    r3 = load<u8>(a + 20);
                    restart = r3 != 0;
                }
                if (restart) {
                    // vfn[352]: this call sequence leaves the OPD in r4 and the entry in r7
                    const u32 opd = load<u32>(load<u32>(t) + 352);
                    ctx.fpr[1] = Lf(t + 344);
                    r3 = static_cast<u32>(call_ptr(ctx, opd, {t, opd, 0, 0, load<u32>(opd)}));
                } else {
                    r3 = static_cast<u32>(VCall(ctx, t, 40, {t, 1}));
                }
            }
        }
    }
    if (keep != 0) {
        r3 = load<u32>(a + 16);
        if (r3 == 0) {
            r3 = load<u8>(a + 21);
            if (r3 == 0) {
                const u32 x = load<u32>(keep + 28);
                if (x != 0) VCall(ctx, x, 8, {x});
                r3 = 0;
                store<u32>(keep + 28, 0);
                store<u32>(keep + 36, 0);
            }
        }
    }
    ctx.gpr[3] = r3;
}

// Obj_ReleaseParts1F4460(Obj* o): the two handles o->828 / o->832 get
// vfn[40](h, 1) and are cleared; o->820 (when set) gets vfn[88](0, 1, 1, 1)
// and Field_VCall404_10_001BD450(o->820, 0); Obj_ReleaseChild_001ED85C(o,
// o->812); o->816 (when set): its part +160's owner +260 gets vfn[24](owner,
// part), then vfn[88](0, 1, 1, 1), vfn[40](0), vfn[40](1), o->816 = 0.
// Finally Obj_DropHeld_002B7704 on Target_GetDef0D951C(o) when non-zero
// (r3 = its result, or 0).
DECOMP(001F4460, Obj_ReleaseParts1F4460) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < 2; ++i) {
        const u32 s = o + 828 + 4 * i;
        const u32 h = load<u32>(s);
        if (h == 0) continue;
        VCall(ctx, h, 40, {h, 1});
        store<u32>(s, 0);
    }
    const u32 x = load<u32>(o + 820);
    if (x != 0) {
        VCall(ctx, x, 88, {x, 0, 1, 1, 1});
        call(ctx, kVCall404, {load<u32>(o + 820), 0});
    }
    call(ctx, kReleaseChild, {o, load<u32>(o + 812)});
    u32 y = load<u32>(o + 816);
    if (y != 0) {
        const u32 p = load<u32>(y + 160);
        if (p != 0) {
            const u32 q = load<u32>(p + 260);
            if (q != 0) {
                VCall(ctx, q, 24, {q, p});
                y = load<u32>(o + 816);
            }
        }
        VCall(ctx, y, 88, {y, 0, 1, 1, 1});
        y = load<u32>(o + 816);
        VCall(ctx, y, 40, {y, 0});
        y = load<u32>(o + 816);
        VCall(ctx, y, 40, {y, 1});
        store<u32>(o + 816, 0);
    }
    const u32 t = static_cast<u32>(call(ctx, kGetDef, {o}));
    ctx.gpr[3] = t;
    if (t != 0) ctx.gpr[3] = call(ctx, kDropHeld, {t});
}

// ---------------------------------------------------------------- batch 2

namespace {
constexpr u32 kObjOwner = 0x000D93FCu;     // Obj_Owner0D93FC(x)
constexpr u32 kAnimRestart = 0x002775A8u;  // Anim_Restart2775A8(a, h, ...; f1..f4)
constexpr u32 kPlayByLevel = 0x0001B4A0u;  // Obj_PlayByLevel_0001B4A0(list, o, k, alt)
constexpr u32 kObjSetAnim = 0x00363A3Cu;   // Obj_SetAnim(o, name, now, -, b, c, d; f32)
constexpr u32 kDispatch362 = 0x0036325Cu;  // Obj_Dispatch362(o, a, b, c)
constexpr u32 kTargetKey = 0x02235C33u;    // +276 key of a component's target record

// The rule target lookup of the effect-rule callbacks: the component c =
// Effect_FindComponent(e, hash) of kind 5 (c->vfn[32]()) whose +28 record
// has the key 0x02235C33 at +276 gives its +280; 0 otherwise.
u32 RuleTarget(PPUContext& ctx, u32 e, u32 hash) {
    const u32 c = static_cast<u32>(call(ctx, kFindComponent, {e, hash}));
    if (c == 0 || static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) return 0;
    const u32 r = load<u32>(c + 28);
    if (r == 0 || load<u32>(r + 276) != kTargetKey) return 0;
    return load<u32>(r + 280);
}

// The effect-rule colour lerp: c0 + ((c1 - c0) * w >> 8) per byte, w the
// truncated 255 * f clamped (unsigned) to 255. f1 = the fctidz bits.
// Returns the byte-3 delta ((c1[3] - c0[3]) * w >> 8), r3 of one path.
u32 RuleLerp(PPUContext& ctx, u32 out, u32 c0, const u32 c1v[4], f64 f) {
    const u64 iv = rt::fctid(S(f * Lf(0x000766E0u)), true);
    ctx.fpr[1] = std::bit_cast<f64>(iv);
    const u32 lo = static_cast<u32>(iv);
    const u32 w = lo < 255 ? lo : 255;
    u32 d = 0;
    for (u32 k = 0; k < 4; ++k) {
        const u32 b0 = load<u8>(c0 + k);
        d = static_cast<u32>(static_cast<s32>((c1v[k] - b0) * w) >> 8);
        store<u8>(out + k, static_cast<u8>(b0 + d));
    }
    return d;
}
}  // namespace

// u32 EffectRule_ForTarget7E65C(Rule* a, Effect* e, x) (vtable entry): the
// target t = the rule target of a->12 (RuleTarget) when a->12, else
// Obj_FindById(a->16) when a->16. With the byte a->49 the target must exist
// with +172 bit 0x40000 and, when its +192 has bit 0x02, t->vfn[320]() must
// be 0. The match: a->20 == t->216->52 or a->24 == t->168 (each when
// non-zero); when it differs from the byte a->48 every child of the list
// a->28 (u16 count a->36) gets vfn[12](child, e, x). r3 as the original
// leaves it.
DECOMP(0007E65C, EffectRule_ForTarget7E65C) {
    const u32 a = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    u32 t = 0;
    const u32 k = load<u32>(a + 12);
    if (k != 0) t = RuleTarget(ctx, e, k);
    if (t == 0) {
        const u32 id = load<u32>(a + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, kFindById, {id}));
    }
    u32 r3 = load<u8>(a + 49);
    if (r3 != 0) {
        if (t == 0) { ctx.gpr[3] = r3; return; }
        r3 = (load<u32>(t + 172) >> 18) & 1;
        if (r3 == 0) { ctx.gpr[3] = 0; return; }
        r3 = load<u32>(t + 192) & 2;
        if (r3 != 0) {
            r3 = static_cast<u32>(VCall(ctx, t, 320, {t}));
            if (r3 != 0) { ctx.gpr[3] = r3; return; }
        }
    }
    u32 match = 0;
    if (t != 0) {
        const u32 k1 = load<u32>(a + 20);
        if (k1 != 0 && k1 == load<u32>(load<u32>(t + 216) + 52)) match = 1;
        else {
            const u32 k2 = load<u32>(a + 24);
            if (k2 != 0 && k2 == load<u32>(t + 168)) match = 1;
        }
    }
    const u32 inv = load<u8>(a + 48);
    if (match == inv) { ctx.gpr[3] = inv; return; }
    for (u32 i = 0; i < load<u16>(a + 36); ++i) {  // count and array reloaded per child
        const u32 p = load<u32>(load<u32>(a + 28) + 4 * i);
        VCall(ctx, p, 12, {p, e, x});
    }
    ctx.gpr[3] = load<u16>(a + 36);
}

// u32 EffectRule_ByTargetKey988C0(Rule* a, Effect* e, x) (vtable entry):
// the target t = the rule target of a->128 (RuleTarget), else
// Obj_FindById(a->132) when a->132. The first of the a->124 28-byte
// entries at a+12 whose key +20 == t->216->52 or key +24 == t->168 (each
// when non-zero) runs its own list (+0, u16 count +8), otherwise the
// default list a+136 (u16 count a->144): each non-null child gets
// vfn[12](child, e, x). r3 = the final count read.
DECOMP(000988C0, EffectRule_ByTargetKey988C0) {
    const u32 a = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    u32 t = 0;
    const u32 k = load<u32>(a + 128);
    if (k != 0) t = RuleTarget(ctx, e, k);
    if (t == 0) {
        const u32 id = load<u32>(a + 132);
        if (id != 0) t = static_cast<u32>(call(ctx, kFindById, {id}));
    }
    if (t != 0) {
        const u32 n = load<u32>(a + 124);
        for (u32 i = 0; i < n; ++i) {
            const u32 en = a + 12 + 28 * i;
            const u32 k1 = load<u32>(en + 20), k2 = load<u32>(en + 24);
            if ((k1 != 0 && k1 == load<u32>(load<u32>(t + 216) + 52)) || (k2 != 0 && k2 == load<u32>(t + 168))) {
                for (u32 j = 0; j < load<u16>(en + 8); ++j) {
                    const u32 p = load<u32>(load<u32>(en) + 4 * j);
                    if (p != 0) VCall(ctx, p, 12, {p, e, x});
                }
                ctx.gpr[3] = load<u16>(en + 8);
                return;
            }
        }
    }
    for (u32 j = 0; j < load<u16>(a + 144); ++j) {
        const u32 p = load<u32>(load<u32>(a + 136) + 4 * j);
        if (p != 0) VCall(ctx, p, 12, {p, e, x});
    }
    ctx.gpr[3] = load<u16>(a + 144);
}

// u32 Obj_BindComponents5E470(Obj* o): o->308 = 1, o->320 = 0; with a body
// o->428 its effect e = vfn[272]() gives five components (0x11A825BA,
// 0x7D63B6B9, 0xE91F47B8, 0x54DAD8B7, 0xC09669B6), each record c->28 (or
// 0) appended to o+352 (count o->320). Then o->vfn[220](o, 0) (r3 = it).
DECOMP(0005E470, Obj_BindComponents5E470) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(o + 428);
    store<u32>(o + 308, 1);
    store<u32>(o + 320, 0);
    if (b != 0) {
        const u32 e = static_cast<u32>(VCall(ctx, b, 272, {b}));
        for (u32 h : {0x11A825BAu, 0x7D63B6B9u, 0xE91F47B8u, 0x54DAD8B7u, 0xC09669B6u}) {
            const u32 c = static_cast<u32>(call(ctx, kFindComponent, {e, h}));
            const u32 v = c != 0 ? load<u32>(c + 28) : 0;
            const u32 n = load<u32>(o + 320);
            store<u32>(o + 320, n + 1);
            store<u32>(o + 352 + 4 * n, v);
        }
    }
    ctx.gpr[3] = VCall(ctx, o, 220, {o, 0});
}

// u32 EffectRule_AddRecord76150(Rule* a, Effect* e) (vtable entry): the
// slot index is c->vfn[24]() of the kind-2 component c =
// Effect_FindComponent(e, a->12) (when a->12), else (none, or a negative
// index) the running counter g->0x1283A4++ of g = *(Game**)0x00F65CA0.
// The 28-byte record g + 0x128334 + 28 * idx: word 0 keeps its low 23 bits
// with a->36 (4 bits) << 28, a->40 (4 bits) << 24 and 0x00800000 when the
// byte a->44 is 0; bytes +2..+5 = a+24, +6..+9 = a+28 (overlapping the
// word), +12 / +16 / +20 = a->16 / a->20 / a->32, +24 = 0. c->20 = idx.
// r3 = idx.
DECOMP(00076150, EffectRule_AddRecord76150) {
    const u32 a = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 c = 0;
    const u32 k = load<u32>(a + 12);
    if (k != 0) {
        const u32 x = static_cast<u32>(call(ctx, kFindComponent, {e, k}));
        if (x != 0 && static_cast<u32>(VCall(ctx, x, 32, {x})) == 2) c = x;
    }
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 w16 = load<u32>(a + 16), w20 = load<u32>(a + 20), w32 = load<u32>(a + 32);
    const u32 hi = load<u32>(a + 36), mid = load<u32>(a + 40);
    const u32 z = load<u8>(a + 44) == 0 ? 0x00800000u : 0;
    u32 idx;
    bool counter = c == 0;
    if (!counter) {
        idx = static_cast<u32>(VCall(ctx, c, 24, {c}));
        counter = static_cast<s32>(idx) < 0;
    }
    if (counter) {
        idx = load<u32>(g + 0x1283A4u);
        store<u32>(g + 0x1283A4u, idx + 1);
    }
    const u32 r = g + 28 * idx + 0x128334u;
    store<u32>(r, (load<u32>(r) & 0x007FFFFFu) | (hi << 28) | ((mid & 0xF) << 24) | z);
    for (u32 i = 0; i < 4; ++i) store<u8>(r + 2 + i, load<u8>(a + 24 + i));
    for (u32 i = 0; i < 4; ++i) store<u8>(r + 6 + i, load<u8>(a + 28 + i));
    store<u32>(r + 12, w16);
    store<u32>(r + 16, w20);
    store<u32>(r + 20, w32);
    store<u32>(r + 24, 0);
    if (c != 0) store<u32>(c + 20, idx);
    ctx.gpr[3] = idx;
}

// EffectRule_Colour76708(u8 out[4], Rule* a, Effect* e, Params* p): the
// colour a+28, or a lerp from a+24 toward it (RuleLerp): with a->20 by
// the kind-1 component c = Effect_FindComponent(e, a->20) (f = its
// vfn[20]() f1 result); without a->20 by f = p->32 / a->32 when p,
// a->32 >= 1e-5 and p->32 < a->32.
DECOMP(00076708, EffectRule_Colour76708) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), e = arg<u32>(ctx, 2), p = arg<u32>(ctx, 3);
    constexpr u32 C = 0x000766E0u;  // 255.0f; +32 1e-5f
    u32 c1[4];
    for (u32 k = 0; k < 4; ++k) c1[k] = load<u8>(a + 28 + k);
    const u32 key = load<u32>(a + 20);
    u32 r3 = key;  // r3 as the original leaves it
    if (key != 0) {
        const u32 c = static_cast<u32>(call(ctx, kFindComponent, {e, key}));
        r3 = c;
        if (c != 0) {
            r3 = static_cast<u32>(VCall(ctx, c, 32, {c}));
            if (r3 == 1) {
                VCall(ctx, c, 20, {c});
                const f64 f = ctx.fpr[1];
                for (u32 k = 0; k < 4; ++k) c1[k] = load<u8>(a + 28 + k);  // reloaded after the calls
                ctx.gpr[3] = RuleLerp(ctx, out, a + 24, c1, f);
                return;
            }
        }
    } else if (p != 0) {
        const f64 lim = Lf(a + 32);
        ctx.fpr[1] = Lf(C + 32);
        r3 = C;
        if (lim >= Lf(C + 32)) {
            const f64 v = Lf(p + 32);
            ctx.fpr[1] = v;
            if (v < lim) {
                RuleLerp(ctx, out, a + 24, c1, Divs(v, lim));
                ctx.gpr[3] = load<u8>(a + 27);
                return;
            }
        }
    }
    for (u32 k = 0; k < 4; ++k) store<u8>(out + k, static_cast<u8>(c1[k]));
    ctx.gpr[3] = r3;
}

// u32 Obj_LevelCue396B6C(Node* n, Obj* o) (vtable entry): the level key of
// node n (tables of *(Level**)0x00F65038 by the s16 indices n->2 / n->4,
// the layout offsets *(u16*)0x0106CF70 / 0x0106CF7E): key 0x75208DC6 ->
// Obj_PlayByLevel_0001B4A0(*0x00F557F0, o->12, 2, 0); 0x804F6B59 -> mode 1,
// skipped entirely when o->12 has +192 bit 0x08, byte +668 bit 0x80 without
// 0x40 and vfn[32](0) == 0, vfn[32](1) == 0, vfn[32](2) == 3; any other
// key -> mode 0. r3 = 0.
DECOMP(00396B6C, Obj_LevelCue396B6C) {
    const u32 n = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 lv = load<u32>(0x00F65038u);
    const s32 s1 = static_cast<s16>(load<u16>(n + 4)), s0 = static_cast<s16>(load<u16>(n + 2));
    const u32 rowA = s1 != -1 ? load<u32>(lv + 24) + static_cast<u32>(s1) * 128 : 0;
    const u32 rowB = s0 != -1 ? load<u32>(lv + 24) + static_cast<u32>(s0) * 128 : 0;
    const u32 sel = load<u8>(rowB + 2) != 0 ? rowB + load<u16>(0x0106CF7Eu) : 0;
    const u32 key = load<u32>(rowA + load<u16>(0x0106CF70u) + 4 * load<u8>(sel));
    u32 mode = 0;
    if (static_cast<s32>(key) > static_cast<s32>(0x804F6B59u)) {
        if (key == 0x75208DC6u) mode = 2;
    } else if (static_cast<s32>(key) > static_cast<s32>(0x804F6B58u)) {
        mode = 1;
        const u32 t = load<u32>(o + 12);
        if (t != 0 && (load<u32>(t + 192) & 8) != 0) {
            const u32 b = load<u8>(t + 668);
            if ((b & 0x80) != 0 && (b & 0x40) == 0 && static_cast<u32>(VCall(ctx, t, 32, {t, 0})) == 0 &&
                static_cast<u32>(VCall(ctx, t, 32, {t, 1})) == 0 && static_cast<u32>(VCall(ctx, t, 32, {t, 2})) == 3) {
                ctx.gpr[3] = 0;
                return;
            }
        }
    }
    call(ctx, kPlayByLevel, {load<u32>(0x00F557F0u), load<u32>(o + 12), mode, 0});
    ctx.gpr[3] = 0;
}

// Obj_RandomFlinch33780(Obj* o, x) (vtable entry): in modes o->4 19 / 22 /
// 23 / 25 with a body o->68 whose +416 top state ((>> 28) & 7) is 1: one
// step of the inlined generator (mid15 kRand15); when its value * 1/65536
// (0x00033770) is below the difficulty threshold (o->308->12)[(*0x00F94F8C)
// ->64], the owner Obj_Owner0D93FC(x) decides the anim of o->64:
// 0xBCD6F543 when its +136 has (+68 >> 26) & 0xF == 0, else 0xE22B711A
// (Anim_Restart2775A8; o->32 = the hash when it returns non-zero), then
// o->vfn[220](o, 26). r3 / f1 as the original leaves them.
DECOMP(00033780, Obj_RandomFlinch33780) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    constexpr u32 C = 0x00033770u;
    ctx.gpr[3] = x;
    const s32 k = load<s32>(o + 4);
    if (!(k == 19 || k == 25 || k == 23 || k == 22)) return;
    const u32 b = load<u32>(o + 68);
    if (b == 0) return;
    if (((load<u32>(b + 416) >> 28) & 7) != 1) return;
    const u64 old = load<u64>(kRand15);
    const u32 m = RandMix15(load<u32>(kRand15 + 8));
    const f64 c0 = Lf(C);
    store<u32>(kRand15 + 8, std::rotl(m, 16));
    const u32 tb = load<u32>(load<u32>(o + 308) + 12);
    const u32 lvl = load<u32>(load<u32>(0x00F94F8Cu) + 64);
    store<u64>(kRand15, m & 0xFFFFu);
    const f64 r = S(RandF15(old) * c0);
    ctx.fpr[1] = r;  // f1 as the original leaves it
    if (!(r < Lf(tb + 4 * lvl))) return;
    const u32 ow = static_cast<u32>(call(ctx, kObjOwner, {x}));
    const u32 an = load<u32>(o + 64);
    ctx.gpr[3] = an;
    if (ow == 0) return;
    const u32 q = load<u32>(ow + 136);
    const bool first = q != 0 && ((load<u32>(q + 68) >> 26) & 0xF) == 0;
    const u32 h = first ? 0xBCD6F543u : 0xE22B711Au;
    // r7 / r8 (unread by the callee): path A sets r7 = 0, r8 = -1, path B
    // r7 = -1 and leaves r8 as Obj_Owner0D93FC did (passed as 0 here); the
    // 11th / 12th arguments are the stack words 0 and -1
    const u64 r7 = first ? 0 : ~0ull, r8 = first ? ~0ull : 0;
    callf(ctx, kAnimRestart, {an, h, 0, 0, r7, r8, 0, 0, 0, 0, 0, ~0ull}, {Lf(C + 4), Lf(C + 8), Lf(C + 12), Lf(C + 12)});
    if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, h);
    ctx.gpr[3] = VCall(ctx, o, 220, {o, 26});
}

// u32 Obj_SetLit1EFB30(Obj* o, u8 on, u8 force): bit 0x00080000 of o->980
// holds the state; switching it (or forcing) with a light o->844 sets its
// +392 = scale * o->956 / *(f32*)0x01053BA8 and calls Obj_SetAnim(light,
// 0, 1, light, 0, 0, 0x01164CA4; *(f32*)0x001EFB2C) with scale
// *(f32*)0x01053C48 (on) or Obj_Dispatch362(light, 1, 0, 0) with
// *(f32*)0x01053C98 (off), plus Effect_Spawn(0xEAD85853 / 0x9E42A8A2,
// light->24, 0, 1) when o->980 has bit 0x04000000; the bit is then stored
// as on & 1. Every part of o->928 (u16 count o->936) gets vfn[40](0, 0).
// r3 = the count.
DECOMP(001EFB30, Obj_SetLit1EFB30) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 onr = arg<u32>(ctx, 1);
    const bool on = (onr & 0xFF) != 0, force = (arg<u32>(ctx, 2) & 0xFF) != 0;
    const bool lit = (load<u32>(o + 980) & 0x00080000u) != 0;
    const bool change = on ? (!lit || force) : (lit || force);
    if (change) {
        const u32 l = load<u32>(o + 844);
        if (l != 0) {
            const f64 base = Divs(Lf(o + 956), Lf(0x01053BA8u));
            if (on) {
                Sf(l + 392, S(Lf(0x01053C48u) * base));
                callf(ctx, kObjSetAnim, {l, 0, 1, l, 0, 0, 0x01164CA4u, 0x001EFB2Cu}, {Lf(0x001EFB2Cu)});
            } else {
                const f64 v = S(Lf(0x01053C98u) * base);
                Sf(l + 392, v);
                callf(ctx, kDispatch362, {l, 1, 0, 0, l}, {v});
            }
            if ((load<u32>(o + 980) & 0x04000000u) != 0)
                call(ctx, kEffectSpawn, {on ? 0xEAD85853u : 0x9E42A8A2u, load<u32>(load<u32>(o + 844) + 24), 0, 1});
        }
        store<u32>(o + 980, (load<u32>(o + 980) & ~0x00080000u) | ((onr & 1) << 19));
    }
    for (u32 i = 0; i < load<u16>(o + 936); ++i) {
        const u32 p = load<u32>(load<u32>(o + 928) + 4 * i);
        VCall(ctx, p, 40, {p, 0, 0});
    }
    ctx.gpr[3] = load<u16>(o + 936);
}

// ---------------------------------------------------------------- batch 3

namespace {
constexpr u32 kWorldObjFindByHash = 0x005801DCu;  // WorldObj_FindByHash(hash, 0)
constexpr u32 kObjPoolCreate = 0x002F0E0Cu;       // ObjPool_Create(pool, desc, u32* handle)
constexpr u32 kEffectSetTime = 0x004CF710u;       // Effect_SetTime(e; f32)
constexpr u32 kObjFindComponent = 0x000D1288u;    // Obj_FindComponent(o, kind, 0)
constexpr u32 kCanHitBase = 0x000DF960u;          // the base hit test (o, src, kind, x)
constexpr u32 kTrackLength = 0x00276B40u;         // Anim_TrackLength36_00276B40(a, id) -> f1
constexpr u32 kAnimCall = 0x00276E4Cu;            // Obj_AnimCall3541E0_00276E4C(a, -, k; f32)

// The level-table slot of node n: rows of *(Level**)0x00F65038 (+24, 128
// bytes each) by the s16 indices n->4 (row A) and n->2 (row B, its byte +2
// selecting the byte list at row B + *(u16*)0x0106CF7E, else address 0):
// row A + *(u16*)0x0106CF70 + 4 * list[k].
u32 LevelSlot(u32 n, u32 k) {
    const u32 lv = load<u32>(0x00F65038u);
    const s32 s1 = static_cast<s16>(load<u16>(n + 4)), s0 = static_cast<s16>(load<u16>(n + 2));
    const u32 rowA = s1 != -1 ? load<u32>(lv + 24) + static_cast<u32>(s1) * 128 : 0;
    const u32 rowB = s0 != -1 ? load<u32>(lv + 24) + static_cast<u32>(s0) * 128 : 0;
    const u32 sel = load<u8>(rowB + 2) != 0 ? rowB + load<u16>(0x0106CF7Eu) : 0;
    return rowA + load<u16>(0x0106CF70u) + 4 * load<u8>(sel + k);
}
}  // namespace

// u32 EffectRule_BindTarget7958C(Rule* a, Effect* e) (vtable entry): the
// target t (RuleTarget of a->12, else Obj_FindById(a->16)), optionally
// Obj_FindByHash_000DB290(t, a->20); a found t gives a->40 = t->160 and
// a->36 = t->212. Without a->40, a->40 = WorldObj_FindByHash(a->24, 0) when
// a->24. The kind-1 component Effect_FindComponent(e, a->28) gives a->48 =
// its vfn[20]() f1. With a->40 set, ObjPool_Create(*(Pool**)0x00F64C38,
// a+32, &handle (0)) (r3 = it), else r3 = 0.
DECOMP(0007958C, EffectRule_BindTarget7958C) {
    const u32 a = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 sp = frame.addr() - 112;
    u32 t = 0;
    const u32 k = load<u32>(a + 12);
    if (k != 0) t = RuleTarget(ctx, e, k);
    if (t == 0) {
        const u32 id = load<u32>(a + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, kFindById, {id}));
    }
    if (t != 0) {
        const u32 h = load<u32>(a + 20);
        if (h != 0) t = static_cast<u32>(call(ctx, kFindByHash, {t, h}));
        if (t != 0) {
            store<u32>(a + 40, load<u32>(t + 160));
            ctx.fpr[1] = Lf(t + 212);  // f1 as the original leaves it
            Cw(a + 36, t + 212);
        }
    }
    if (load<u32>(a + 40) == 0) {
        const u32 h = load<u32>(a + 24);
        if (h != 0) store<u32>(a + 40, static_cast<u32>(call(ctx, kWorldObjFindByHash, {h, 0})));
    }
    const u32 k2 = load<u32>(a + 28);
    if (k2 != 0) {
        const u32 c = static_cast<u32>(call(ctx, kFindComponent, {e, k2}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 1) {
            VCall(ctx, c, 20, {c});
            Sf(a + 48, ctx.fpr[1]);
        }
    }
    u64 r3 = load<u32>(a + 40);
    if (r3 != 0) {
        store<u32>(sp + 112, 0);
        store<u16>(sp + 114, 0);
        r3 = call(ctx, kObjPoolCreate, {load<u32>(0x00F64C38u), a + 32, sp + 112});
    }
    ctx.gpr[3] = r3;
}

// u32 Obj_ApplyPendingFlags0BBEA8(Obj* o): in state vfn[32](0) == 4 with
// any of the pending bits 0xF0000000 of o->676: sub-state vfn[32](1) 3 ->
// vfn[28](o, 1, 4) when the byte *0x01013A68 or bit 0x80000000; 5 with
// vfn[32](2) == 2 -> vfn[28](o, 2, 3 / 4 / 5) by bit 0x40000000 /
// 0x20000000 / neither. Then the bits 0xF0000000 are cleared. r3 = 0.
DECOMP(000BBEA8, Obj_ApplyPendingFlags0BBEA8) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) == 4 && (load<u32>(o + 676) & 0xF0000000u) != 0) {
        const s32 s1 = static_cast<s32>(VCall(ctx, o, 32, {o, 1}));
        if (s1 > 3) {
            if (s1 == 5 && static_cast<s32>(VCall(ctx, o, 32, {o, 2})) == 2) {
                const u32 w = load<u32>(o + 676);
                VCall(ctx, o, 28, {o, 2, (w & 0x40000000u) != 0 ? 3u : (w & 0x20000000u) != 0 ? 4u : 5u});
            }
        } else if (s1 > 2) {
            if (load<u8>(0x01013A68u) != 0 || (load<u32>(o + 676) & 0x80000000u) != 0) VCall(ctx, o, 28, {o, 1, 4});
        }
    }
    store<u32>(o + 676, load<u32>(o + 676) & 0x0FFFFFFFu);
    ctx.gpr[3] = 0;
}

// u32 Seq_Update2E899C(Seq* o, -, u8 f) (vtable entry): runs while the
// byte o->28 has 0x80 and not 0x40. The lead o->20 (started with vfn[12]
// when its byte +28 is 0) gates by vfn[20](): 0 stops the pass. Then each
// step of o->16 (count o->24) is started likewise and stepped with
// vfn[8](step, f) until one returns 0. All done: o->28 bit 0x40 set and
// 0x80 cleared (else 0x40 cleared), and the lead's callback +60 gets
// vfn[8](cb, lead->68, lead->76) (when +68 is set), clearing +68 / +72 /
// +76. r3 as the original leaves it.
DECOMP(002E899C, Seq_Update2E899C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f = arg<u32>(ctx, 2) & 0xFF;
    const u32 b = load<u8>(o + 28);
    if ((b & 0x80) == 0) { ctx.gpr[3] = b; return; }
    if ((b & 0x40) != 0) { ctx.gpr[3] = 1; return; }
    u32 p = load<u32>(o + 20);
    bool skip = false;
    if (p != 0) {
        if (load<u8>(p + 28) == 0) {
            VCall(ctx, p, 12, {p});
            p = load<u32>(o + 20);
        }
        const u32 r = static_cast<u32>(VCall(ctx, p, 20, {p}));
        skip = r == 0 && load<u32>(o + 20) != 0;
    }
    u32 ran = 0;
    if (!skip && load<u32>(o + 24) != 0) {
        for (u32 i = 0;;) {
            u32 q = load<u32>(load<u32>(o + 16) + 4 * i);
            if (load<u8>(q + 28) == 0) VCall(ctx, q, 12, {q});
            q = load<u32>(load<u32>(o + 16) + 4 * i);
            if (static_cast<u32>(VCall(ctx, q, 8, {q, f})) == 0) break;
            ++ran;
            ++i;
            if (!(i < load<u32>(o + 24))) break;
        }
    }
    const u32 n = load<u32>(o + 24);
    u32 bb = load<u8>(o + 28);
    const u32 done = ran == n ? 1 : 0;
    bb = (bb & ~0x40u) | (done << 6);
    bb = (bb & ~0x80u) | ((((bb >> 7) & 1) ^ done) << 7);
    store<u8>(o + 28, static_cast<u8>(bb));
    if (!done) { ctx.gpr[3] = 0; return; }
    p = load<u32>(o + 20);
    if (p == 0 || load<u32>(p + 68) == 0) { ctx.gpr[3] = 1; return; }
    const u32 cb = load<u32>(p + 60);
    if (cb == 0) { ctx.gpr[3] = 0; return; }
    ctx.gpr[3] = VCall(ctx, cb, 8, {cb, load<u32>(p + 68), load<u32>(p + 76)});
    store<u32>(p + 68, 0);
    store<u32>(p + 72, 0);
    store<u32>(p + 76, 0);
}

// u32 EffectRule_IfComponent4C64E4(Rule* a, Effect* e, x) (vtable entry):
// the component c = Effect_FindComponent(e, a->36) decides the match:
// active (vfn[12]()) -> its vfn[20]() f1 == a->32; kind 3 -> its words
// +32 / +36 / +40 == a->16 / +20 / +24; kind 12 -> vfn[28]() == a->12.
// When the match differs from the byte a->60 every child of the list
// a->40 (u16 count a->48) gets vfn[12](child, e, x). r3 as the original
// leaves it.
DECOMP(004C64E4, EffectRule_IfComponent4C64E4) {
    const u32 a = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    const u32 c = static_cast<u32>(call(ctx, kFindComponent, {e, load<u32>(a + 36)}));
    if (c == 0) { ctx.gpr[3] = 0; return; }
    if (static_cast<u32>(VCall(ctx, c, 12, {c})) != 0) {
        VCall(ctx, c, 20, {c});
        const u32 eq = ctx.fpr[1] == Lf(a + 32) ? 1 : 0;
        if (load<u8>(a + 60) == eq) { ctx.gpr[3] = eq; return; }
    } else if (static_cast<s32>(VCall(ctx, c, 32, {c})) == 3) {
        ctx.fpr[1] = Lf(c + 36);  // f1 as the original leaves it
        const u32 eq = load<u32>(c + 32) == load<u32>(a + 16) && load<u32>(c + 36) == load<u32>(a + 20) &&
                               load<u32>(c + 40) == load<u32>(a + 24) ? 1 : 0;
        const u32 inv = load<u8>(a + 60);
        if (inv == eq) { ctx.gpr[3] = inv; return; }
    } else {
        const u32 k2 = static_cast<u32>(VCall(ctx, c, 32, {c}));
        if (k2 != 12) { ctx.gpr[3] = k2; return; }
        const u32 inv = load<u8>(a + 60);  // read before the call
        const u32 eq = static_cast<u32>(VCall(ctx, c, 28, {c})) == load<u32>(a + 12) ? 1 : 0;
        if (inv == eq) { ctx.gpr[3] = eq; return; }
    }
    for (u32 i = 0; i < load<u16>(a + 48); ++i) {
        const u32 p = load<u32>(load<u32>(a + 40) + 4 * i);
        VCall(ctx, p, 12, {p, e, x});
    }
    ctx.gpr[3] = load<u16>(a + 48);
}

// u32 Group_SpawnMarkers3011E0(Obj* o): for every member t of every group
// o->1032[] (u16 count o->1040; members g->20, s32 count g->24) with +172
// bit 0x40000: no +244 state -> t->vfn[352](t, 0, 0; f1 = t->344); state
// kind +4 other than 13 -> e = Effect_Spawn(0x61D2ACD0, t->160, 0, 1) and,
// when made, its kind-5 component 0x0027469E rebinds its record +28 (old
// one vfn[8]()) to t->160 (whose u16 +4 is bumped), then Effect_SetTime(e;
// *(f32*)0x003011DC). r3 = the group count.
DECOMP(003011E0, Group_SpawnMarkers3011E0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 tm = Lf(0x003011DCu);
    for (u32 i = 0; i < load<u16>(o + 1040); ++i) {
        const u32 g = load<u32>(load<u32>(o + 1032) + 4 * i);
        const u32 arr = load<u32>(g + 20);
        if (arr == 0) continue;
        for (s32 j = 0; j < load<s32>(g + 24); ++j) {
            const u32 t = load<u32>(arr + 4 * static_cast<u32>(j));
            if ((load<u32>(t + 172) & 0x40000u) == 0) continue;
            const u32 s = load<u32>(t + 244);
            if (s == 0) {
                // vfn[352]: this call sequence leaves the OPD in r4 and the entry in r7
                const u32 opd = load<u32>(load<u32>(t) + 352);
                ctx.fpr[1] = Lf(t + 344);
                call_ptr(ctx, opd, {t, opd, 0, 0, load<u32>(opd)});
                continue;
            }
            if (load<s32>(s + 4) == 13) continue;
            const u32 fx = static_cast<u32>(call(ctx, kEffectSpawn, {0x61D2ACD0u, load<u32>(t + 160), 0, 1}));
            if (fx == 0) continue;
            const u32 tw = load<u32>(t + 160);
            const u32 c = static_cast<u32>(call(ctx, kFindComponent, {fx, 0x0027469Eu}));
            if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
                const u32 r = load<u32>(c + 28);
                if (r != 0) VCall(ctx, r, 8, {r});
                store<u32>(c + 28, tw);
                if (tw != 0) store<u16>(tw + 4, static_cast<u16>(load<u16>(tw + 4) + 1));
            }
            callf(ctx, kEffectSetTime, {fx}, {tm});
        }
    }
    ctx.gpr[3] = load<u16>(o + 1040);
}

// u32 Obj_CanTakeHit127BC8(Obj* o, Obj* src, s32 kind, x) (vtable entry):
// 0 for kind 5, kind 3 without the byte o->216->1436, health o->348 <=
// *(f32*)0x00127BC4, a same-team src (bits (+172 >> 22) & 3, src +192 bit
// 0x08, not kind 4) unless o->188 has 0x20, kind 0 with o->188 bit 0x01
// and the kind-23 component's (+24 >> 25) & 0x1F == 1, o->188 bit 0x01 with
// 0x10 or 0x01000000, kind 1 while vfn[32](0/1/2) == 0/0/3 and the byte
// +668 has 0x80 without 0x40, kind 4 when o->244 vfn[124]() is non-zero;
// otherwise the base test 0x000DF960(o, src, kind, x).
DECOMP(00127BC8, Obj_CanTakeHit127BC8) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), x = arg<u32>(ctx, 3);
    const s32 kind = arg<s32>(ctx, 2);
    ctx.gpr[3] = 0;
    const u32 team = (load<u32>(o + 172) >> 22) & 3;
    u32 fl = load<u32>(o + 188);
    if (kind == 5) return;
    if (kind == 3 && load<u8>(load<u32>(o + 216) + 1436) == 0) return;
    const f64 hp = Lf(o + 348);
    ctx.fpr[1] = hp;  // f1 as the original leaves it
    if (hp <= Lf(0x00127BC4u)) return;
    if (kind != 4 && src != 0 && (load<u32>(src + 192) & 8) != 0 && team == ((load<u32>(src + 172) >> 22) & 3) &&
        (fl & 0x20) == 0)
        return;
    if ((fl & 1) != 0 && kind == 0) {
        const u32 c = static_cast<u32>(call(ctx, kObjFindComponent, {o, 23, 0}));
        if (c != 0 && ((load<u32>(c + 24) >> 25) & 0x1F) == 1) { ctx.gpr[3] = 0; return; }
    }
    fl = load<u32>(o + 188);
    if ((fl & 1) != 0 && ((fl & 0x10) != 0 || (fl & 0x01000000u) != 0)) { ctx.gpr[3] = 0; return; }
    if (kind == 1) {
        if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) != 0 || static_cast<u32>(VCall(ctx, o, 32, {o, 1})) != 0 ||
            static_cast<u32>(VCall(ctx, o, 32, {o, 2})) != 3) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 b = load<u8>(o + 668);
        if ((b & 0x80) != 0 && (b & 0x40) == 0) { ctx.gpr[3] = 0; return; }
    }
    if (kind == 4) {
        const u32 s = load<u32>(o + 244);
        if (s != 0 && static_cast<u32>(VCall(ctx, s, 124, {s})) != 0) { ctx.gpr[3] = 0; return; }
    }
    ctx.gpr[3] = call(ctx, kCanHitBase, {o, src, static_cast<u32>(kind), x});
}

// u32 Obj_PlayRandomIdle5BFD0(Obj* o): o->484 = (o->484 + 1 + rand % 3) %
// 4 (one step of the inlined generator, signed remainder); the player
// body (*(Game**)0x00F65CA0 + 4216, when its +192 has bit 0x02, else 0)
// animator vfn[60]() restarts 0xC7C116DA + idx and o->64 restarts
// 0xACDA2FD8 + idx (Anim_Restart2775A8 with *(f32*)0x0005BFC4 / +4 / +8 /
// +8; o->32 = that hash when it returns non-zero). r3 = the last restart.
DECOMP(0005BFD0, Obj_PlayRandomIdle5BFD0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 C = 0x0005BFC4u;
    Stack frame(ctx, 64);
    const u32 sp = frame.addr() - 112;  // the two hash tables at sp+144 / sp+160, as in the original
    for (u32 k = 0; k < 4; ++k) {
        store<u32>(sp + 144 + 4 * k, 0xC7C116DAu + k);
        store<u32>(sp + 160 + 4 * k, 0xACDA2FD8u + k);
    }
    const u32 s = load<u32>(kRand15 + 8);
    const u32 m = RandMix15(s);
    store<u32>(kRand15 + 8, std::rotl(m, 16));
    const s32 n = static_cast<s32>(load<u32>(o + 484) + (s % 3) + 1);
    const u32 g = load<u32>(0x00F65CA0u);
    store<u64>(kRand15, m & 0xFFFFu);
    const s32 rem = n % 4;  // (o->484 stays 0..3, so the table read stays in range)
    store<u32>(o + 484, static_cast<u32>(rem));
    const u32 h1 = load<u32>(sp + 144 + 4 * static_cast<u32>(rem));
    const u32 p = load<u32>(g + 4216);
    const u32 who = p != 0 && (load<u32>(p + 192) & 2) != 0 ? p : 0;
    const u32 an = static_cast<u32>(VCall(ctx, who, 60, {who}));
    const f64 c0 = Lf(C), c1 = Lf(C + 4), c2 = Lf(C + 8);
    callf(ctx, kAnimRestart, {an, h1, 0, 0, 0, 0, 0, 0, 0, 0, 0, ~0ull}, {c0, c1, c2, c2});
    const u32 h2 = load<u32>(sp + 160 + 4 * load<u32>(o + 484));
    callf(ctx, kAnimRestart, {load<u32>(o + 64), h2, 0, 0, 0, 0, 0, 0, 0, 0, 0, ~0ull}, {c0, c1, c2, c2});
    if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, h2);
}

// u32 Obj_LevelAnim396930(Node* n, Obj* o) (vtable entry): the animator
// a = o->12->vfn[60]() plays Obj_AnimCall3541E0_00276E4C(a, -, 0; rate)
// with the rate from the level slot 0 of n (LevelSlot), divided into
// Anim_TrackLength36_00276B40(a, 0) / slot 1 when slot 1 > *(f32*)
// 0x00396928. r3 = 0.
DECOMP(00396930, Obj_LevelAnim396930) {
    const u32 o = arg<u32>(ctx, 1);
    const u32 n = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    const u32 x = load<u32>(o + 12);
    if (x == 0) return;
    const u32 an = static_cast<u32>(VCall(ctx, x, 60, {x}));
    ctx.gpr[3] = 0;
    if (an == 0) return;
    f64 rate = Lf(LevelSlot(n, 0));
    const u32 s1 = LevelSlot(n, 1);
    u32 r4;
    if (Lf(s1) > Lf(0x00396928u)) {
        ctx.fpr[1] = rate;  // f1 as the original leaves it (the length comes back in f1)
        call(ctx, kTrackLength, {an, 0});
        const f64 len = ctx.fpr[1];
        const u32 s2 = LevelSlot(n, 1);
        rate = Divs(len, Lf(s2));
        // r4: the byte list address of the last lookup
        const u32 lv = load<u32>(0x00F65038u);
        const s32 s0 = static_cast<s16>(load<u16>(n + 2));
        const u32 rowB = s0 != -1 ? load<u32>(lv + 24) + static_cast<u32>(s0) * 128 : 0;
        r4 = load<u8>(rowB + 2) != 0 ? rowB + load<u16>(0x0106CF7Eu) : 0;
    } else {
        // r4: row A of the lookup
        const u32 lv = load<u32>(0x00F65038u);
        const s32 sa = static_cast<s16>(load<u16>(n + 4));
        r4 = sa != -1 ? load<u32>(lv + 24) + static_cast<u32>(sa) * 128 : 0;
    }
    callf(ctx, kAnimCall, {an, r4, 0}, {rate});
    ctx.gpr[3] = 0;
}

// ---------------------------------------------------------------- batch 4

namespace {
constexpr u32 kStageBounds = 0x00049060u;     // the stage bounds setter (o, const f32 lo[3], const f32 hi[3], 0)
constexpr u32 kFlag13Off = 0x0048F298u;       // Obj_Flag13Off_0048F298(x, 1)
constexpr u32 kPopLayer = 0x000FBB88u;        // Obj_PopLayer(stack, layer; f32)
constexpr u32 kSetEffectHash = 0x000DAEC8u;   // Obj_SetEffectHash_000DAEC8(o, hash)
constexpr u32 kResetBase = 0x00012928u;       // Obj_ResetBase12928(o)
constexpr u32 kDetach = 0x000D72C0u;          // Obj_Detach0D72C0(holder, o)
constexpr u32 kHandleNotify = 0x0036742Cu;    // Handle_Notify36742C(h, 1, 0, 0)
constexpr u32 kEffectSetHash = 0x004CF988u;   // Effect_SetHash4CF988(e, hash)
constexpr u32 kSetRef = 0x00121F10u;          // SetRef_00121F10(o, 0)
constexpr u32 kBodyRemove = 0x00931F00u;      // Body_Remove931F00(world, body)
constexpr u32 kHkRemoveEntity = 0x0092FFC8u;  // Hk_RemoveEntity92FFC8(out, world, entity)
constexpr u32 kHitTest1F4DC0 = 0x001F4DC0u;   // (part, a, b; f32) -> hit
constexpr u32 kSetStateFx = 0x00266E6Cu;      // Obj_SetStateFx266E6C(o, 4)
constexpr u32 kCanInteract = 0x0012B650u;     // Obj_CanInteract12B650(o)
constexpr u32 kGetSaveObj = 0x002E29B0u;      // World_GetSaveObj160(session)
constexpr u32 kSaveProfile = 0x000C9FC8u;     // Save_Profile0C9FC8(id)
constexpr u32 kPartDone = 0x002E2740u;        // Session_PartDone2E2740(session, id)
constexpr u32 kSetSave = 0x00124D58u;         // Player_SetSave124D58(o, save, 1)
constexpr u32 kHandleCall = 0x0036735Cu;      // Handle_Call (id, name, 1, -, 0, 0, 0x01164CA4; f32)

// Stores the two 3-float bound vectors (raw words) and calls the stage
// bounds setter with them.
void StageBounds(PPUContext& ctx, u32 o, u32 lo, u32 hi, std::initializer_list<u32> l, std::initializer_list<u32> h) {
    u32 k = 0;
    for (u32 w : l) store<u32>(lo + 4 * k++, w);
    k = 0;
    for (u32 w : h) store<u32>(hi + 4 * k++, w);
    call(ctx, kStageBounds, {o, lo, hi, 0});
}

// Objects found by id with +192 bit 0x00400000 get vfn[304](x, 0).
void ResetById(PPUContext& ctx, std::initializer_list<u32> ids) {
    for (u32 id : ids) {
        const u32 x = static_cast<u32>(call(ctx, kFindById, {id}));
        if (x != 0 && (load<u32>(x + 192) & 0x00400000u) != 0) VCall(ctx, x, 304, {x, 0});
    }
}

// The object found by id (when any) gets vfn[88](x, a, 1, 1, 1); r3 = it.
u64 ShowById(PPUContext& ctx, u32 id, u32 a) {
    const u32 x = static_cast<u32>(call(ctx, kFindById, {id}));
    if (x == 0) return 0;
    return VCall(ctx, x, 88, {x, a, 1, 1, 1});
}
}  // namespace

// u32 Script_Stage1_4979C(Level* o): resets 0x09E8933B / 0xA8F58862 /
// 0x3211271C (ResetById), byte o->1804 = 1, hides 0x19F541C3 / 0x8BD1C3E8
// (vfn[88](0, 1, 1, 1)), o->400 vfn[304](1), then the stage bounds
// (-65, 250, -80)..(65, 326, -20). r3 = the setter's result.
DECOMP(0004979C, Script_Stage1_4979C) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 48);
    const u32 sp = frame.addr() - 112;
    ResetById(ctx, {0x09E8933Bu, 0xA8F58862u, 0x3211271Cu});
    store<u8>(o + 1804, 1);
    ShowById(ctx, 0x19F541C3u, 0);
    ShowById(ctx, 0x8BD1C3E8u, 0);
    const u32 p = load<u32>(o + 400);
    if (p != 0) VCall(ctx, p, 304, {p, 1});
    StageBounds(ctx, o, sp + 112, sp + 128, {0xC2820000u, 0x437A0000u, 0xC2A00000u}, {0x42820000u, 0x43A30000u, 0xC1A00000u});
}

// u32 Script_Stage2_499F8(Level* o): Script_Stage1_4979C(o); the body
// o->60 health +348 = clamp(health * *(f32*)0x00F986CC, > 0.0 else 0.0)
// (min of the two, kept only above 0); stage bounds (-65, 326, -80)..(65,
// 340, -20); o->400 vfn[304](0); 0x8BD1C3E8's part +160 vfn[64](; 40.0) and
// show (vfn[88](1, 1, 1, 1)); show 0x19F541C3; 0x8BD1C3E8 (found again)
// vfn[200](0xF6531EE1, 0, 1, -, 0, 0, -, 0; 1.0, 0.25, 0.0). r3 = that
// call's result, or 0.
DECOMP(000499F8, Script_Stage2_499F8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 C = 0x000499E8u;  // 0.0f, 40.0f, 1.0f, 0.25f
    Stack frame(ctx, 64);
    const u32 sp = frame.addr() - 112;
    call(ctx, 0x0004979Cu, {o});
    const u32 b = load<u32>(o + 60);
    const f64 hp = Lf(b + 344);
    const f64 sc = S(hp * Lf(0x00F986CCu));
    const f64 m = sc < hp ? sc : hp;
    const f64 v = m > Lf(C) ? (sc < hp ? sc : hp) : Lf(C);
    Sf(b + 348, v);
    ctx.fpr[1] = v;  // f1 as the original leaves it
    StageBounds(ctx, o, sp + 128, sp + 144, {0xC2820000u, 0x43A30000u, 0xC2A00000u}, {0x42820000u, 0x43AA0000u, 0xC1A00000u});
    const u32 p = load<u32>(o + 400);
    if (p != 0) VCall(ctx, p, 304, {p, 0});
    const u32 x = static_cast<u32>(call(ctx, kFindById, {0x8BD1C3E8u}));
    if (x != 0) {
        const u32 part = load<u32>(x + 160);
        ctx.fpr[1] = Lf(C + 4);
        VCall(ctx, part, 64, {part});
        VCall(ctx, x, 88, {x, 1, 1, 1, 1});
    }
    ShowById(ctx, 0x19F541C3u, 1);
    const u32 w = static_cast<u32>(call(ctx, kFindById, {0x8BD1C3E8u}));
    if (w == 0) { ctx.gpr[3] = 0; return; }
    // vfn[200]: this call sequence leaves the entry in r7 and the OPD in r10;
    // the 9th argument (stack) is 0
    const u32 opd = load<u32>(load<u32>(w) + 200);
    ctx.fpr[1] = Lf(C + 8);
    ctx.fpr[2] = Lf(C + 12);
    ctx.fpr[3] = Lf(C);
    ctx.gpr[3] = call_ptr(ctx, opd, {w, 0xF6531EE1u, 0, 1, load<u32>(opd), 0, 0, opd, 0});
}

// u32 Script_Stage3_49C58(Level* o): Script_Stage2_499F8(o); stage bounds
// (-85, 250, -16)..(-30, 340, 50), then (-90, 318, 50)..(-30, 250, 90)
// (the second pass reuses the two stack vectors swapped); byte o->1807 = 1;
// resets 0x5A9A051B / 0x7C90A6D8; byte o->1805 = 1; o->404 vfn[304](1);
// hides 0x19F541C4. r3 = the last call's result, or 0.
DECOMP(00049C58, Script_Stage3_49C58) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 48);
    const u32 sp = frame.addr() - 112;
    call(ctx, 0x000499F8u, {o});
    StageBounds(ctx, o, sp + 112, sp + 128, {0xC2AA0000u, 0x437A0000u, 0xC1800000u}, {0xC1F00000u, 0x43AA0000u, 0x42480000u});
    StageBounds(ctx, o, sp + 128, sp + 112, {0xC2B40000u, 0x437A0000u, 0x42480000u}, {0xC1F00000u, 0x439F0000u, 0x42B40000u});
    store<u8>(o + 1807, 1);
    ResetById(ctx, {0x5A9A051Bu, 0x7C90A6D8u});
    store<u8>(o + 1805, 1);
    const u32 p = load<u32>(o + 404);
    if (p != 0) VCall(ctx, p, 304, {p, 1});
    ctx.gpr[3] = ShowById(ctx, 0x19F541C4u, 0);
}

// u32 Obj_ResetCarry5BDC0(Obj* o) (vtable entry): Obj_Flag13Off(o->48, 1),
// the byte g + 0x1285A5 = 0 (g = *(Game**)0x00F65CA0); with the player
// body p (g + 4216, +192 bit 0x02) and a held o->416: Obj_Detach0D72C0(p,
// o->416), o->416 vfn[212](0), its +172 bit 0x8000 cleared. The layers
// o->420 / o->424 are popped from o->416 (Obj_PopLayer; *(f32*)0x0005BDB8),
// released (vfn[4]) and cleared; then p gets effect 0xBC1BA9E7 and
// vfn[176](0). Returns Obj_ResetBase12928(o).
DECOMP(0005BDC0, Obj_ResetCarry5BDC0) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, kFlag13Off, {load<u32>(o + 48), 1});
    store<u8>(load<u32>(0x00F65CA0u) + 0x1285A5u, 0);
    const u32 pb = load<u32>(load<u32>(0x00F65CA0u) + 4216);
    const u32 p = pb != 0 && (load<u32>(pb + 192) & 2) != 0 ? pb : 0;
    if (p != 0) {
        const u32 h = load<u32>(o + 416);
        if (h != 0) {
            call(ctx, kDetach, {p, h});
            const u32 h2 = load<u32>(o + 416);
            VCall(ctx, h2, 212, {h2, 0});
            const u32 h3 = load<u32>(o + 416);
            store<u32>(h3 + 172, load<u32>(h3 + 172) & ~0x8000u);
        }
    }
    for (u32 off : {420u, 424u}) {
        if (load<u32>(o + off) == 0) continue;
        callf(ctx, kPopLayer, {load<u32>(o + 416), load<u32>(o + off)}, {Lf(0x0005BDB8u)});
        const u32 l = load<u32>(o + off);
        if (l != 0) VCall(ctx, l, 4, {l});
        store<u32>(o + off, 0);
    }
    if (p != 0) {
        call(ctx, kSetEffectHash, {p, 0xBC1BA9E7u});
        VCall(ctx, p, 176, {p, 0});
    }
    ctx.gpr[3] = call(ctx, kResetBase, {o});
}

// u32 Obj_Release1DB784(Obj* o, u8 v) (vtable entry): the def t =
// Target_GetDef0D951C(o); a holder t->64 is detached (Obj_Detach0D72C0(h,
// o), or (o, h) when o->216->492 has bit 0x04000000), its state x =
// vfn[68]() copies its byte +16 bit 0 into o->604 bit 0x10000000 and gets
// vfn[44](0). In states vfn[32](0) < 5: vfn[28](0, 5), the effect o->356
// set to 0xA2334D4F (after 0x7FB3B232 when it was 0x04A4252F; o->592 =
// 0xA2334D4F) and o->604 bit 0x08000000 = v & 1. Then
// Handle_Notify36742C(o->216->488, 1, 0, 0) and t->124 bit 0x00100000
// cleared (r3 = the new t->124).
DECOMP(001DB784, Obj_Release1DB784) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 d = load<u32>(o + 216);
    const u32 t = static_cast<u32>(call(ctx, kGetDef, {o}));
    const u32 h = load<u32>(t + 64);
    if (h != 0) {
        const bool swap = (load<u32>(d + 492) & 0x04000000u) != 0;
        call(ctx, kDetach, {swap ? o : h, swap ? h : o});
        const u32 x = static_cast<u32>(VCall(ctx, h, 68, {h}));
        store<u32>(o + 604, (load<u32>(o + 604) & ~0x10000000u) | ((load<u8>(x + 16) & 1u) << 28));
        VCall(ctx, x, 44, {x, 0});
    }
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) < 5) {
        VCall(ctx, o, 28, {o, 0, 5});
        const u32 k = load<u32>(o + 592);
        if (k != 0xA2334D4Fu) {
            const u32 e = load<u32>(o + 356);
            if (e != 0) {
                if (k == 0x04A4252Fu) call(ctx, kEffectSetHash, {e, 0x7FB3B232u});
                call(ctx, kEffectSetHash, {load<u32>(o + 356), 0xA2334D4Fu});
            }
            store<u32>(o + 592, 0xA2334D4Fu);
        }
        store<u32>(o + 604, (load<u32>(o + 604) & ~0x08000000u) | ((v & 1u) << 27));
    }
    call(ctx, kHandleNotify, {load<u32>(d + 488), 1, 0, 0});
    const u32 w = load<u32>(t + 124) & ~0x00100000u;
    store<u32>(t + 124, w);
    ctx.gpr[3] = w;
}

// u32 Obj_ShutdownFx0BB660(Obj* o): o->672 = 0, the bits 0xF0000000 of
// o->676 cleared, effect o->356 set to 0x529EC2A4. The layers o->648 (after
// SetRef_00121F10(player, 0) when the player body g + 4216 has +192 bit
// 0x08) and o->660 are popped from the layer stack g + 0x1281E0 (when set;
// Obj_PopLayer, *(f32*)0x000BB658), released (vfn[4]) and cleared when
// still set. o->640 gets vfn[304](0) and its body +416 is removed from its
// world (Body_Remove931F00(b->8, b)); the entity o->644 is removed from its
// world (Hk_RemoveEntity92FFC8(&out, e->8, e)). r3 as the original leaves it.
DECOMP(000BB660, Obj_ShutdownFx0BB660) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kLayerC = 0x000BB658u;
    Stack frame(ctx, 16);
    const u32 sp = frame.addr() - 112;
    store<u32>(o + 672, 0);
    const u32 w = load<u32>(o + 676);
    const u32 e = load<u32>(o + 356);
    store<u32>(o + 676, w & 0x0FFFFFFFu);
    if (e != 0) call(ctx, kEffectSetHash, {e, 0x529EC2A4u});
    u32 l = load<u32>(o + 648);
    if (l != 0) {
        u32 g = load<u32>(0x00F65CA0u);
        const u32 pb = load<u32>(g + 4216);
        if (pb != 0 && (load<u32>(pb + 192) & 8) != 0) {
            call(ctx, kSetRef, {pb, 0});
            l = load<u32>(o + 648);
            g = load<u32>(0x00F65CA0u);
        }
        const u32 st = load<u32>(g + 0x1281E0u);
        if (st != 0) {
            callf(ctx, kPopLayer, {st, l}, {Lf(kLayerC)});
            l = load<u32>(o + 648);
        }
        if (l != 0) {
            VCall(ctx, l, 4, {l});
            store<u32>(o + 648, 0);
        }
    }
    l = load<u32>(o + 660);
    if (l != 0) {
        const u32 st = load<u32>(load<u32>(0x00F65CA0u) + 0x1281E0u);
        if (st != 0) {
            callf(ctx, kPopLayer, {st, l}, {Lf(kLayerC)});
            l = load<u32>(o + 660);
        }
        if (l != 0) {
            VCall(ctx, l, 4, {l});
            store<u32>(o + 660, 0);
        }
    }
    u64 r3 = 0;
    const u32 pp = load<u32>(o + 640);
    if (pp != 0) {
        VCall(ctx, pp, 304, {pp, 0});
        const u32 b = load<u32>(load<u32>(o + 640) + 416);
        r3 = b;
        if (b != 0) {
            const u32 wd = load<u32>(b + 8);
            if (wd != 0) r3 = call(ctx, kBodyRemove, {wd, b});
        }
    }
    const u32 q = load<u32>(o + 644);
    if (q != 0) {
        const u32 wd = load<u32>(q + 8);
        r3 = wd;
        if (wd != 0) r3 = call(ctx, kHkRemoveEntity, {sp + 112, wd, q});
    }
    ctx.gpr[3] = r3;
}

// u32 Obj_HitParts30912C(Obj* o, a, b): each of the three parts o->776[i]
// hit by 0x001F4DC0(part, a, b; *(f32*)0x0105309C) spawns effect
// 0xF7919930 and, when the state vfn[32](0) (< 12) has a handler
// o->704[state], calls its vfn[52]() (the handler is re-read by a second
// vfn[32](0); a state >= 12 there reads a null handler, as the original
// does); then Obj_SetStateFx266E6C(def, 4) on Target_GetDef0D951C(part).
// r3 as the original leaves it.
DECOMP(0030912C, Obj_HitParts30912C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    u64 r3 = 0;
    for (u32 i = 0; i < 3; ++i) {
        const u32 x = load<u32>(o + 776 + 4 * i);
        r3 = x;
        if (x == 0) continue;
        callf(ctx, kHitTest1F4DC0, {x, a, b}, {Lf(0x0105309Cu)});
        r3 = static_cast<u32>(ctx.gpr[3]);
        if (static_cast<u32>(r3) == 0) continue;
        call(ctx, kEffectSpawn, {0xF7919930u, 0, 0, 1});
        const u32 s = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
        r3 = load<u32>(o);
        if (s < 12 && load<u32>(o + 704 + 4 * s) != 0) {
            const u32 s2 = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
            const u32 hd = s2 < 12 ? load<u32>(o + 704 + 4 * s2) : 0;
            r3 = VCall(ctx, hd, 52, {hd});
        }
        const u32 y = load<u32>(o + 776 + 4 * i);
        if (y == 0) continue;
        const u32 t = static_cast<u32>(call(ctx, kGetDef, {y}));
        r3 = t;
        if (t == 0) continue;
        r3 = call(ctx, kSetStateFx, {t, 4});
    }
    ctx.gpr[3] = r3;
}

// u32 Obj_InteractTick12E930(Obj* o; f32 dt): by the controller word
// (o->380->vfn[12]())->36: bit 0x80000000 -> the hold timer o->1176 = dt;
// 0x40000000 -> o->1176 += dt; else a hold released inside
// (*(f32*)0x0012E924, *(f32*)0x010B7E80) (o->1176 is reset to 0 either way)
// triggers, when Obj_CanInteract12B650(o): with byte o->699 0x80 without
// 0x40, the save Save_Profile0C9FC8(id) (id 0x14120, or
// World_GetSaveObj160(*0x00F64C20) without o->188 bit 0x20; that session
// must also pass Session_PartDone2E2740 when its +36 is set) is applied by
// Player_SetSave124D58(o, save, 1); with 0x40 the call 0x0036735C(0x1447B,
// 0x00ACABB0, 1, -, 0, 0, 0x01164CA4; *(f32*)0x0012E928). r3 / f1 as the
// original leaves them.
DECOMP(0012E930, Obj_InteractTick12E930) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x0012E924u;
    const u32 cl = load<u32>(o + 380);
    const u32 c = static_cast<u32>(VCall(ctx, cl, 12, {cl}));
    const u32 w = load<u32>(c + 36);
    if ((w & 0x80000000u) != 0) {
        Sf(o + 1176, dt);
        ctx.gpr[3] = w;
        return;
    }
    if ((w & 0x40000000u) != 0) {
        const f64 v = S(Lf(o + 1176) + dt);
        Sf(o + 1176, v);
        ctx.fpr[1] = v;
        ctx.gpr[3] = 0x40000000u;
        return;
    }
    const f64 h = Lf(o + 1176);
    ctx.fpr[1] = h;  // f1 as the original leaves it
    bool ok = false;
    ctx.gpr[3] = C;
    if (h > Lf(C)) {
        ctx.gpr[3] = 0x010B7E38u;
        ok = h < Lf(0x010B7E80u);
    }
    store<u32>(o + 1176, 0);
    if (!ok) return;
    if (static_cast<u32>(call(ctx, kCanInteract, {o})) == 0) { ctx.gpr[3] = 0; return; }
    const u32 b = load<u8>(o + 699);
    if ((b & 0x80) != 0 && (b & 0x40) == 0) {
        u32 id = 0x14120u;
        if ((load<u32>(o + 188) & 0x20) == 0) id = static_cast<u32>(call(ctx, kGetSaveObj, {load<u32>(0x00F64C20u)}));
        const u32 sv = static_cast<u32>(call(ctx, kSaveProfile, {id}));
        u64 r3 = sv;
        const u32 fl = load<u32>(o + 188);
        r3 = fl & 0x20;
        if ((fl & 0x20) == 0) {
            const u32 s = load<u32>(0x00F64C20u);
            r3 = s;
            if (load<u32>(s + 36) != 0) {
                r3 = call(ctx, kPartDone, {s, id});
                if (static_cast<u32>(r3) == 0) { ctx.gpr[3] = 0; return; }
            }
        }
        if (sv == 0) { ctx.gpr[3] = r3; return; }
        ctx.gpr[3] = call(ctx, kSetSave, {o, sv, 1});
        return;
    }
    if ((b & 0x40) == 0) { ctx.gpr[3] = 0; return; }
    // r6 / r10: as Obj_CanInteract12B650 leaves them (unread by the callee)
    callf(ctx, kHandleCall, {0x1447Bu, 0x00ACABB0u, 1, 0, 0, 0, 0x01164CA4u}, {Lf(C + 4)});
}
