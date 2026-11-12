// Game code, medium sweep part 33: a rotor rig's bone binding and random
// spin setup (the rig's bones are spun about the axis 0x01164B70).
//
// Rig layout as these use it: +216 the description (+304 / +308 / +312
// the spun bone ids, +316..+340 the other bone ids), +444 / +448 / +452
// the spun bones, +456..+480 the other bones (slot 40 Reset(b, 0, 0)),
// +484 / +488 / +492 the current spin angles, +496 / +500 / +504 the
// picked ones. Bone: +16 a 4x4 world matrix (rows +16 / +32 / +48, the
// position +64), +204 the id, +268 the next sibling, +272 the first child.
#include "mid33.h"

using namespace sly4;
using namespace sly4::mid33;

namespace {
using namespace m33fn;
constexpr u32 MatAxisAngle33 = 0x0056A410u;  // Matrix_FromAxisAngle(out, axis; f1 angle)
constexpr u32 MarkDirty33 = 0x0057D000u;     // WorldObj_MarkDirty(o)
constexpr u32 kSpinAxis33 = 0x01164B70u;

// Bone o->+slot gets the rotation matrix about the spin axis by `angle`
// (its position kept: read before the call), then WorldObj_MarkDirty.
// f1 is left holding the position z.
void RigSpin33(PPUContext& ctx, u32 o, u32 slot, f64 angle) {
    Stack frame(ctx, 64);
    const u32 m = frame.addr();
    const u32 b0 = load<u32>(o + slot);
    const u32 px = load<u32>(b0 + 64), py = load<u32>(b0 + 68), pz = load<u32>(b0 + 72);
    callf(ctx, MatAxisAngle33, {m, kSpinAxis33}, {angle});
    const u32 b = load<u32>(o + slot);
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(b + 16 + 16 * r + k, load<u32>(m + 16 * r + k));
    store<u32>(b + 64, px);
    store<u32>(b + 68, py);
    store<u32>(b + 72, pz);
    ctx.fpr[1] = LfsF(pz);
    call(ctx, MarkDirty33, {b});
}
}  // namespace

// Bind (rig, bone): when bone's id matches one of the description's ids
// the bone is stored in the matching slot; the three spun bones are given
// their current angle (+492 / +488 / +484 times the f32 0x001E57E0) right
// away, the others are reset (slot 40 with 0, 0). Recurses into every
// child of the bone.
DECOMP(001E57E8, Rig_BindBone_001E57E8) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    if (n == 0) return;
    const u32 d = load<u32>(o + 216);
    const u32 id = load<u32>(n + 204);
    static constexpr u32 kSpun[3][3] = {{304, 452, 492}, {308, 448, 488}, {312, 444, 484}};
    static constexpr u32 kOther[7][2] = {{336, 468}, {324, 456}, {328, 476}, {316, 464}, {332, 472}, {320, 460}, {340, 480}};
    bool hit = false;
    for (const auto& s : kSpun) {
        if (id != load<u32>(d + s[0])) continue;
        store<u32>(o + s[1], n);
        RigSpin33(ctx, o, s[1], Fmuls31(Lf(o + s[2]), Lf(0x001E57E0u)));
        hit = true;
        break;
    }
    for (const auto& s : kOther) {
        if (hit) break;
        if (id != load<u32>(d + s[0])) continue;
        store<u32>(o + s[1], n);
        VCall(ctx, n, 40, {n, 0, 0});
        hit = true;
    }
    if (!hit) ctx.gpr[3] = id;  // r3 as left when nothing matched (overwritten by a call below)
    for (u32 c = load<u32>(n + 272); c != 0; c = load<u32>(c + 268)) call(ctx, 0x001E57E8u, {o, c});
}

// Spin setup (rig): three steps of the inlined random generator pick the
// angles (+496 = +484, +500 = +488, +504 = +492; the first from the range
// that the seed's bit 0 selects), the bound spun bones +444 / +448 / +452
// are rotated to them (times K[5]), and the other bones are reset.
// K = the f32 table 0x001E70B8.
DECOMP(001E70D0, Rig_Spin_001E70D0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x001E70B8u;
    const u32 s = load<u32>(0x01164890u);
    const u32 x1 = (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u;
    const u32 x2 = (std::rotl(x1, 20) + x1) ^ 0x87654321u;
    const u32 x3 = (std::rotl(x2, 20) + x2) ^ 0x87654321u;
    const u32 x4 = (std::rotl(x3, 20) + x3) ^ 0x87654321u;
    const f64 k0 = Lf(K), k1 = Lf(K + 4);
    const f64 base0 = (s & 1) == 1 ? k1 : Lf(K + 8);
    const f64 rnd0 = Sgl28(static_cast<f64>(x1 & 0xFFFFu));
    const f64 a0 = Fmadds(Fmuls31(rnd0, k0), k1, base0);
    store<f32>(o + 496, static_cast<f32>(a0));
    store<f32>(o + 484, static_cast<f32>(a0));
    const f64 k3 = Lf(K + 12), k4 = Lf(K + 16);
    store<u32>(0x01164890u, std::rotl(x4, 16));
    store<u64>(0x01164888u, x4 & 0xFFFFu);
    const f64 a1 = Fmadds(Fmuls31(Sgl28(static_cast<f64>(x2 & 0xFFFFu)), k0), k3, k4);
    const f64 a2 = Fmadds(Fmuls31(Sgl28(static_cast<f64>(x3 & 0xFFFFu)), k0), k3, k4);
    store<f32>(o + 500, static_cast<f32>(a1));
    store<f32>(o + 488, static_cast<f32>(a1));
    store<f32>(o + 504, static_cast<f32>(a2));
    store<f32>(o + 492, static_cast<f32>(a2));
    if (load<u32>(o + 444) != 0) RigSpin33(ctx, o, 444, Fmuls31(a0, Lf(K + 20)));
    if (load<u32>(o + 448) != 0) RigSpin33(ctx, o, 448, Fmuls31(Lf(o + 488), Lf(K + 20)));
    const u32 b452 = load<u32>(o + 452);
    ctx.fpr[1] = Lf(o + 492);
    if (b452 != 0) RigSpin33(ctx, o, 452, Fmuls31(Lf(o + 492), Lf(K + 20)));
    for (u32 i = 0; i < 3; ++i) {
        const u32 a = load<u32>(o + 456 + 4 * i);
        if (a != 0) VCall(ctx, a, 40, {a, 0, 0});
        const u32 b = load<u32>(o + 468 + 4 * i);
        if (b != 0) VCall(ctx, b, 40, {b, 0, 0});
        else ctx.gpr[3] = 0;  // r3 as left: the zero just loaded
    }
    const u32 t = load<u32>(o + 480);
    if (t != 0) VCall(ctx, t, 40, {t, 0, 0});
}

namespace {
constexpr u32 TargetGetDef33 = 0x000D951Cu;  // Target_GetDef0D951C(o)
constexpr u32 HandleNotify33 = 0x0036742Cu;  // Handle_Notify36742C(handle, a, b, c)
constexpr u32 EffectSetTime33 = 0x004CF710u; // Effect_SetTime(effect; f1)
}  // namespace

// Charge meter step (o; f1 dt): while +560 bits 0x40000000 / 0x20000000
// are clear and the charge +584 has reached +496, the charge completes:
// the target definition's +64 object (slot 68, then its result's slot 44
// with 0) copies its u8 +16 bit 0 to +604 bit 0x10000000, o's slot 28
// (0, 6), Handle_Notify36742C(+216 ->+488, 1, 0, 0) and the effect +356
// switches to hash 0x1E05330A (via 0x7FB3B232 from 0x04A4252F; state
// +592). Otherwise the charge drains by dt * +504 and, once empty, the
// effect switches from 0x04A4252F to 0xD473DFE9 (via 0x7FB3B232) and
// +584 = 0. Then the meter +576 ->+20 = +584 / +496 (0 when +496 <= K0)
// and Effect_SetTime(+356, K1). K = the f32 pair 0x001DA008.
DECOMP(001DA010, Charge_Step_001DA010) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x001DA008u;
    const f64 dt = farg(ctx, 0);
    const u32 fl = load<u32>(o + 560);
    const u32 own = load<u32>(o + 216);
    const u32 e0 = load<u32>(o + 356);
    const f64 rate = Lf(o + 504);
    bool full = false;
    if ((fl & 0x60000000u) == 0) {
        const f64 c = Lf(o + 584), mx = Lf(o + 496);
        full = c >= mx;
    }
    if (full) {
        const u32 d = static_cast<u32>(call(ctx, TargetGetDef33, {o}));
        const u32 x = load<u32>(d + 64);
        if (x != 0) {
            const u32 y = static_cast<u32>(VCall(ctx, x, 68, {x}));
            const u32 b = load<u8>(y + 16);
            store<u32>(o + 604, (load<u32>(o + 604) & ~0x10000000u) | ((b & 1u) << 28));
            VCall(ctx, y, 44, {y, 0});
        }
        VCall(ctx, o, 28, {o, 0, 6});
        call(ctx, HandleNotify33, {load<u32>(own + 488), 1, 0, 0});
        const u32 h = load<u32>(o + 592);
        if (h != 0x1E05330Au) {
            const u32 e = load<u32>(o + 356);
            if (e != 0) {
                if (h == 0x04A4252Fu) call(ctx, SetHash33, {e, 0x7FB3B232u});
                call(ctx, SetHash33, {load<u32>(o + 356), 0x1E05330Au});
            }
            store<u32>(o + 592, 0x1E05330Au);
        }
    } else {
        f64 c = Lf(o + 584);
        const f64 k0 = Lf(K);
        if (c > k0) {
            c = Fnmsubs(dt, rate, c);
            store<f32>(o + 584, static_cast<f32>(c));
        }
        if (c <= k0) {
            if (load<u32>(o + 592) == 0x04A4252Fu) {
                if (e0 != 0) {
                    call(ctx, SetHash33, {e0, 0x7FB3B232u});
                    call(ctx, SetHash33, {load<u32>(o + 356), 0xD473DFE9u});
                }
                store<u32>(o + 592, 0xD473DFE9u);
            }
            store<u32>(o + 584, 0);
        }
    }
    const f64 mx = Lf(o + 496);
    f64 r = Lf(K);
    if (mx > r) r = Sgl28(Lf(o + 584) / mx);
    ctx.fpr[1] = mx;
    const u32 m = load<u32>(o + 576);
    ctx.gpr[3] = m;  // r3 as left when the meter or the effect is missing
    if (m == 0) return;
    store<f32>(m + 20, static_cast<f32>(r));
    const u32 e = load<u32>(o + 356);
    if (e == 0) return;
    callf(ctx, EffectSetTime33, {e}, {Lf(K + 4)});
}

namespace {
constexpr u32 PathPose33 = 0x0012919Cu;  // Obj_GetPathPose12919C(o, 0, 0, u32* seg) -> path
}  // namespace

// Path presence step (o; f1 dt): "occupied" when +512 is set and a live
// (+172 bit 0x40000) player with +192 bit 0x8 in state 3 (slot 32 with
// 0) is on that path (Obj_GetPathPose12919C) in segment 2..15. With a
// running timer +564 > K0: occupied counts it down by dt (clamped to K0;
// on reaching K0 hash 0x6C09E0B1 and +568 bit 31 set), unoccupied clears
// it and bit 31 (hash 0xE0856145). With no timer: occupied and +568 bit
// 31 clear, either restarts the timer from +560 (when +544 == +540; bit
// 31 = timer <= K0; hash 0xA5724322) or sets bit 31 (hash 0x8B3FA8A4);
// unoccupied with bit 31 set clears it (hash 0x4A1380A0 when +544 ==
// +548, else 0xD3FD4422). Hashes go to the effect +356 when set.
// K = the f32 at 0x001B2A44.
DECOMP(001B2A48, Path_Presence_001B2A48) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 192);
    const u32 seg = frame.addr() + 112;
    constexpr u32 K = 0x001B2A44u;
    const f64 dt = farg(ctx, 0);
    const bool was = (load<u32>(o + 568) >> 31) != 0;
    bool found = false;
    if (load<u32>(o + 512) != 0) {
        for (u32 i = 0, off = 0;; ++i, off += 0x20C80u) {
            const u32 w = load<u32>(kWorld33);
            if (i >= load<u8>(w + 29)) break;
            const u32 p = load<u32>(w + off + 4208 + 8);
            if (p == 0 || (load<u32>(p + 172) & 0x40000u) == 0 || (load<u32>(p + 192) & 8u) == 0) continue;
            store<u32>(seg, 0);
            if (static_cast<s32>(VCall(ctx, p, 32, {p, 0})) != 3) continue;
            const u32 path = load<u32>(o + 512);
            if (path != static_cast<u32>(call(ctx, PathPose33, {p, 0, 0, seg}))) continue;
            const u32 s = load<u32>(seg);
            if (s < 2 || s > 15) continue;
            found = true;
            break;
        }
    }
    const f64 t = Lf(o + 564);
    const f64 k0 = Lf(K);
    ctx.fpr[1] = k0;
    ctx.gpr[3] = K;  // r3 as left on the paths that make no call / store
    auto hash = [&](u32 h) {
        const u32 e = load<u32>(o + 356);
        if (e != 0) call(ctx, SetHash33, {e, h});
    };
    if (t > k0) {
        if (!found) {
            const u32 f = load<u32>(o + 568) & 0x7FFFFFFFu;
            store<u32>(o + 564, 0);
            store<u32>(o + 568, f);
            ctx.gpr[3] = f;
            hash(0xE0856145u);
            return;
        }
        f64 n = Sgl28(t - dt);
        if (n < k0) n = k0;
        store<f32>(o + 564, static_cast<f32>(n));
        if (n <= k0) {
            hash(0x6C09E0B1u);
            const u32 f = load<u32>(o + 568) | 0x80000000u;
            store<u32>(o + 568, f);
            ctx.gpr[3] = 0xFFFFFFFF00000000ull | f;
        }
        return;
    }
    if (found && !was) {
        if (Lf(o + 544) != Lf(o + 540)) {
            const u32 f = load<u32>(o + 568) | 0x80000000u;
            store<u32>(o + 564, 0);
            store<u32>(o + 568, f);
            ctx.gpr[3] = 0xFFFFFFFF00000000ull | f;
            hash(0x8B3FA8A4u);
            return;
        }
        const u32 st = load<u32>(o + 560);
        const u32 f0 = load<u32>(o + 568);
        store<u32>(o + 564, st);
        const u32 f = (f0 & 0x7FFFFFFFu) | (LfsF(st) <= k0 ? 0x80000000u : 0u);
        store<u32>(o + 568, f);
        ctx.gpr[3] = f;
        hash(0xA5724322u);
        return;
    }
    if (!was || found) return;
    const f64 a = Lf(o + 544), b = Lf(o + 548);
    const u32 f = load<u32>(o + 568) & 0x7FFFFFFFu;
    store<u32>(o + 568, f);
    ctx.fpr[1] = a;
    ctx.gpr[3] = f;
    hash(a == b ? 0x4A1380A0u : 0xD3FD4422u);
}

namespace {
constexpr u32 ObjReset33 = 0x00121FA8u;     // Obj_Reset121FA8(o, a, b, c, d, e)
constexpr u32 ObjSetAnim33 = 0x00363A3Cu;   // Obj_SetAnim(o, name, now, -, b, c, d; f1)
constexpr u32 ApplyYawPitch33 = 0x001DC988u;  // Obj_ApplyYawPitch_001DC988(o)
}  // namespace

// Grab start (o, x) -> 1: with a grabbable x (+192 bit 0x8) that has no
// other holder (+424 == o->+432 or 0, or the u8 +436 bit 0x80 already
// set): first contact (+436 bit 0x80 clear and +432 set) resets x
// (Obj_Reset121FA8(x, 0, 0, 0, 0, 1)), sets the bit and o's slot 28 (0, 2);
// otherwise o's slot 28 (0, 3) unless slot 32 (0) is above 1. Then
// +732 = x->+348, x->+348 = max(min(x->+344, o->+344), K0) and o->+348 =
// max(o->+344, K0). Always: the three anims +716 / +720 / +724 (0 with
// K1, 0x01052F0C / 0x01052FD4 with T(0x01052E78)), the yaw / pitch, and
// +740 = 0 / 1 / 2 from the definition +216's u8 +616 / +617.
// K = the f32 pair 0x001E3A84.
DECOMP(001E3A90, Grab_Start_001E3A90) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    constexpr u32 K = 0x001E3A84u;
    call(ctx, TargetGetDef33, {o});
    if (x != 0 && (load<u32>(x + 192) & 8u) != 0) {
        const u32 c = load<u32>(o + 432);
        bool go = true;
        if (c != 0) {
            const u32 s = load<u32>(x + 424);
            if (!(s == c && s != 0)) go = (load<u8>(o + 436) & 0x80u) != 0;
        }
        if (go) {
            if ((load<u8>(o + 436) & 0x80u) == 0 && c != 0) {
                call(ctx, ObjReset33, {x, 0, 0, 0, 0, 1});
                store<u8>(o + 436, static_cast<u8>(load<u8>(o + 436) | 0x80u));
                VCall(ctx, o, 28, {o, 0, 2});
            } else if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) <= 1) {
                VCall(ctx, o, 28, {o, 0, 3});
            }
            store<u32>(o + 732, load<u32>(x + 348));
            const f64 a = Lf(x + 344), b = Lf(o + 344);
            const f64 m = b < a ? b : a;
            const f64 k0 = Lf(K);
            store<f32>(x + 348, static_cast<f32>(m > k0 ? m : k0));
            const f64 b2 = Lf(o + 344);
            store<f32>(o + 348, static_cast<f32>(b2 > k0 ? b2 : k0));
        }
    }
    const u32 h = load<u32>(o + 716);
    callf(ctx, ObjSetAnim33, {h, 0, 1, h, 0, 0, 0x01164CA4u}, {Lf(K + 4)});
    // r6 (unused by the callee) as left by the previous call
    callf(ctx, ObjSetAnim33, {load<u32>(o + 720), 0x01052F0Cu, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {Lf(0x01052EC0u)});
    callf(ctx, ObjSetAnim33, {load<u32>(o + 724), 0x01052FD4u, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {Lf(0x01052EC0u)});
    call(ctx, ApplyYawPitch33, {o});
    const u32 d = load<u32>(o + 216);
    u32 v = 0;
    if (load<u8>(d + 616) != 0) v = load<u8>(d + 617) != 0 ? 2 : 1;
    store<u32>(o + 740, v);
    ret(ctx, 1u);
}

namespace {
constexpr u32 MarkDirty33b = 0x0057D000u;  // WorldObj_MarkDirty(o)

// The 4x3 matrix rows of 16 bytes at s copied to d (raw words, the
// original's order); returns the last word.
u32 CopyRows33(u32 d, u32 s) {
    u32 w = 0;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 12; k += 4) {
            w = load<u32>(s + 16 * r + k);
            store<u32>(d + 16 * r + k, w);
        }
    return w;
}
}  // namespace

// Attachment sync (o): when the owner +8 is live (+172 bit 0x40000) and
// has a source +160, every attachment of the list +2912 (u16 count +2944)
// takes the source's matrix +16 (WorldObj_MarkDirty), the first u16
// +2946 bone matrices of the source's skeleton (slot 28 ->+32, 16-byte
// entries, +4 the matrix), its visibility (slot 40 with the source's +164
// bit 0x8000, 0) and the slot 28 object's u8 +31 bit 0x80, then is
// marked dirty again. Leaves r3 / f1 as the original does.
DECOMP(0028EBB8, Attach_Sync_0028EBB8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = load<u32>(o + 8);
    ctx.gpr[3] = 0;
    if ((load<u32>(x + 172) & 0x40000u) == 0) return;
    const u32 s = load<u32>(x + 160);
    ctx.gpr[3] = 1;
    if (s == 0) return;
    u32 n = load<u16>(o + 2944);
    ctx.gpr[3] = n;
    for (u32 i = 0; i < n; ++i) {
        const u32 c = load<u32>(o + 2912 + 4 * i);
        if (c == 0) continue;
        ctx.fpr[1] = LfsF(CopyRows33(c + 16, s + 16));
        call(ctx, MarkDirty33b, {c});
        for (u32 j = 0; j < load<u16>(o + 2946); ++j) {
            const u32 a = static_cast<u32>(VCall(ctx, c, 28, {c}));
            const u32 d = load<u32>(load<u32>(a + 32) + 16 * j + 4);
            const u32 b = static_cast<u32>(VCall(ctx, s, 28, {s}));
            const u32 src = load<u32>(load<u32>(b + 32) + 16 * j + 4);
            ctx.fpr[1] = LfsF(CopyRows33(d, src));
        }
        {
            // slot 40 with the entry in r6 and the OPD in r7 (the reverse of VCall)
            const u32 opd = load<u32>(load<u32>(c) + 40);
            call_ptr(ctx, opd, {c, (load<u32>(s + 164) >> 15) & 1u, 0, load<u32>(opd), opd});
        }
        const u32 q = static_cast<u32>(VCall(ctx, c, 28, {c}));
        store<u8>(q + 31, static_cast<u8>(load<u8>(q + 31) | 0x80u));
        call(ctx, MarkDirty33b, {c});
        n = load<u16>(o + 2944);
        ctx.gpr[3] = n;
    }
}

namespace {
constexpr u32 CutsceneTimeScale33 = 0x004843A4u;  // World_GetCutsceneTimeScale(world) -> f1

// Renderable r: SetMatrix (slot 20 with m), then slot 152 with f1 = the
// cutscene time scale (as World_GetCutsceneTimeScale leaves it) and
// slot 156.
void ShadowPush33(PPUContext& ctx, u32 o, u32 off, u32 m) {
    const u32 r0 = load<u32>(o + off);
    VCall(ctx, r0, 20, {r0, m});
    const u32 r = load<u32>(o + off);
    call(ctx, CutsceneTimeScale33, {load<u32>(kWorld33)});
    VCall(ctx, r, 152, {r});
    VCall(ctx, r, 156, {r});
}
}  // namespace

// Mirror update (o): the matrix of the source +956 (+80, 4 rows) with its
// first two rows negated is pushed to the reflection +948, then (with a
// second renderable +952) the reflection's colour (slot 52) goes to +952
// (slot 56) and +952 gets the same matrix lifted by T(0x01059FC8) in y.
DECOMP(002ABF18, Mirror_Update_002ABF18) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 src = load<u32>(o + 956);
    if (src == 0) { ctx.gpr[3] = 0; return; }
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();
    const u32 m = sp + 128, col = sp + 192;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(m + 16 * r + k, load<u32>(src + 80 + 16 * r + k) ^ (r < 2 ? 0x80000000u : 0u));
    ctx.fpr[1] = Lf(m);  // f1 as left: the negated first word
    u32 a = load<u32>(o + 948);
    if (a != 0) {
        ShadowPush33(ctx, o, 948, m);
        a = load<u32>(o + 948);
    }
    u32 b = load<u32>(o + 952);
    if (b == 0) { ctx.gpr[3] = a; return; }
    if (a != 0) {
        VCall(ctx, a, 52, {col, a});
        const u32 c = load<u32>(col);
        store<u32>(sp + 112, c);
        store<u32>(sp + 56, c);
        // slot 56: r4 = the colour (overwriting the OPD), r5 = the entry
        const u32 opd = load<u32>(load<u32>(b) + 56);
        call_ptr(ctx, opd, {b, c, load<u32>(opd)});
        b = load<u32>(o + 952);
    }
    const f64 y = Sgl28(Lf(m + 52) + Lf(0x0105A010u));
    store<f32>(m + 52, static_cast<f32>(y));
    ctx.fpr[1] = y;
    (void)b;
    ShadowPush33(ctx, o, 952, m);
}

namespace {
constexpr u32 GetChannel33 = 0x0045B160u;  // Obj_GetChannel_0045B160(o, i) -> f1
constexpr u32 RingRecent33 = 0x0046426Cu;  // Ring_CopyRecent_0046426C(o, -, out, u32* n; f1 t)
}  // namespace

// Input steer (o, f32* yaw): the target definition's +64 player record
// (0x010B19E0 + 11968 * the slot's u8 +1, +176) feeds *yaw -= / += the
// tuning +216 ->+476 times: channel 0 (mode +560 == 0), channel 2 (mode
// 1), or for mode 2 (when the record's +28 device is active, slot 48)
// the mean of the last K[0] seconds of samples (+80 ring) in x,
// normalised by its xyz length when that exceeds K[1] (K[2] / length).
// K = the f32 table 0x001E757C. Leaves r3 / f1 as the original does.
DECOMP(001E7588, Input_Steer_001E7588) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    constexpr u32 K = 0x001E757Cu;
    Stack frame(ctx, 5936);
    const u32 sp = frame.addr();
    const u32 d = static_cast<u32>(call(ctx, TargetGetDef33, {o}));
    const u32 t = load<u32>(d + 64);
    ctx.gpr[3] = d;
    if (t == 0) return;
    const s32 pi = static_cast<s8>(load<u8>(t + 172));
    const u32 slot = load<u32>(kWorld33) + static_cast<u32>(pi * 0x20C80) + 4208;
    const u32 tun = load<u32>(o + 216);
    const s32 mode = static_cast<s32>(load<u32>(o + 560));
    const u32 base = 0x010B19E0u + load<u8>(slot + 1) * 11968u;
    const u32 rec = base + 176;
    auto chan = [&](u32 i) {
        const f64 y0 = Lf(out);
        const f64 v = callf(ctx, GetChannel33, {rec, i}, {});
        const f64 r = Fmadds(Lf(tun + 476), v, y0);
        store<f32>(out, static_cast<f32>(r));
        ctx.fpr[1] = r;
    };
    ctx.gpr[3] = base;
    if (mode == 0) return chan(0);
    if (mode == 1) return chan(2);
    if (mode != 2) return;
    const u32 n = sp + 5872, smp = sp + 112;
    store<u32>(n, 0);
    const u32 dev = load<u32>(rec + 28);
    ctx.gpr[3] = dev;
    if (dev == 0) return;
    if (static_cast<u32>(VCall(ctx, dev, 48, {dev})) == 0) return;
    // r4 = the constant table address, as left
    callf(ctx, RingRecent33, {rec + 80, K, smp, n}, {Lf(K)});
    if (static_cast<u32>(ctx.gpr[3]) == 0) return;
    f64 x = Lf(0x01164B40u), y = Lf(0x01164B44u), z = Lf(0x01164B48u);
    const u32 cnt = load<u32>(n);
    if (cnt != 0) {
        u32 r = cnt, p = smp;
        do {
            --r;
            x = Sgl28(x + Lf(p));
            y = Sgl28(y + Lf(p + 4));
            z = Sgl28(z + Lf(p + 8));
            p += 16;
        } while (static_cast<s32>(r) > 0);
    }
    const f64 fn = Sgl28(static_cast<f64>(static_cast<s64>(cnt)));
    y = Sgl28(y / fn);
    x = Sgl28(x / fn);
    z = Sgl28(z / fn);
    f64 l = Fmadds(x, x, Fmuls31(y, y));
    l = Sgl28(std::sqrt(Fmadds(z, z, l)));
    if (l > Lf(K + 4)) x = Fmuls31(x, Sgl28(Lf(K + 8) / l));
    const f64 r = Fnmsubs(Lf(tun + 476), x, Lf(out));
    store<f32>(out, static_cast<f32>(r));
    ctx.fpr[1] = r;
    ctx.gpr[3] = K;
}
