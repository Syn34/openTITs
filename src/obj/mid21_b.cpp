// Game code, medium sweep part 21: switches, effect cues and misc object
// helpers.
#include "mid21.h"

using namespace sly4;
using namespace sly4::mid21;

namespace {
constexpr u32 kNotify26 = 0x000D94CCu;  // Obj_Notify26(o)
constexpr u32 kSetHash = 0x004CF988u;   // Effect_SetHash4CF988(fx, hash)
}  // namespace

// Switch_SetOn252DFC(Switch* o, u8 on): switching on classifies the target
// o->24 (r = Obj_Notify26(o->24), with its byte +16 set): kind (r->244 >> 20)
// & 0xFF 6..9 -> o->52 = 2, another non-zero kind -> 1, else 0. The effect
// o->28 gets the cue of the state o->52: on 0xA6CB0022 / 0xC0FD2685 /
// 0x12252850, off 0xF6239C09 / 0x1A67FE7E / 0xF96B3877 (states 0 / 1 / 2).
// Then o->8->vfn[96](o->8, on) and the byte o->16 = on. r3 = that call's
// result.
DECOMP(00252DFC, Switch_SetOn252DFC) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFF;
    if (on != 0) {
        const u32 p = load<u32>(o + 24);
        store<u32>(o + 52, 0);
        if (p != 0) {
            const u32 r = static_cast<u32>(call(ctx, kNotify26, {p, 0}));
            if (r != 0 && load<u8>(r + 16) != 0) {
                const s32 k = static_cast<s32>((load<u32>(r + 244) >> 20) & 0xFF);
                if (k == 6 || k == 7 || k == 8 || k == 9) store<u32>(o + 52, 2);
                else if (k != 0) store<u32>(o + 52, 1);
            }
        }
    }
    const u32 fx = load<u32>(o + 28);
    if (fx != 0) {
        const s32 s = static_cast<s32>(load<u32>(o + 52));
        static constexpr u32 kOn[3] = {0xA6CB0022u, 0xC0FD2685u, 0x12252850u};
        static constexpr u32 kOff[3] = {0xF6239C09u, 0x1A67FE7Eu, 0xF96B3877u};
        if (s >= 0 && s <= 2) call(ctx, kSetHash, {fx, on != 0 ? kOn[s] : kOff[s]});
    }
    const u32 h = load<u32>(o + 8);
    ctx.gpr[3] = VCall(ctx, h, 96, {h, on != 0 ? 1u : 0u});
    store<u8>(o + 16, static_cast<u8>(on));
}

namespace {
constexpr u32 kFindComponent = 0x004BB4B4u;  // Effect_FindComponent(e, hash)
constexpr u32 kFindById = 0x000D6A28u;       // Obj_FindById(id)
constexpr u32 kPartDone = 0x002E2740u;       // Session_PartDone2E2740(session, id)
constexpr u32 kEffectSpawn = 0x004A5FC8u;    // Effect_Spawn(hash, a, b, c)
constexpr u32 kSession = 0x00F64C20u;        // Session* g_Session
}  // namespace

// u32 Obj_PartEffects3B92D8(Obj* o, -, u32* out): *out = 0; for each of the
// u8 def->2636 parts of the definition def = o->60 (288-byte entries at
// def+2640: object id +0, part id +276, effect hashes +280 / +284) whose
// object t = Obj_FindById(id) exists, Effect_Spawn(done ? +284 : +280,
// t->160, t->164, 1) with done = Session_PartDone2E2740(g_Session, part id).
// Then the component c = Effect_FindComponent(o->32, 0x81BAD6B3) of kind 5
// (c->vfn[32]()) with a c->28 handler: unless the part o->192 is done,
// c->28->vfn[40](c->28, 0, 1). r3 as the original leaves it.
DECOMP(003B92D8, Obj_PartEffects3B92D8) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    u32 def = load<u32>(o + 60);
    store<u32>(out, 0);
    if (load<u8>(def + 2636) != 0) {
        u32 j = 0, off = 0;
        do {
            const u32 t = static_cast<u32>(call(ctx, kFindById, {load<u32>(def + 2640 + off)}));
            if (t != 0) {
                const u32 done = static_cast<u32>(call(ctx, kPartDone, {load<u32>(kSession), load<u32>(load<u32>(o + 60) + off + 2916)}));
                const u32 hash = load<u32>(load<u32>(o + 60) + off + (done != 0 ? 2924 : 2920));
                call(ctx, kEffectSpawn, {hash, load<u32>(t + 160), load<u32>(t + 164), 1});
            }
            def = load<u32>(o + 60);
            ++j;
            off += 288;
        } while (j < load<u8>(def + 2636));
    }
    const u32 c = static_cast<u32>(call(ctx, kFindComponent, {load<u32>(o + 32), 0x81BAD6B3u}));
    ctx.gpr[3] = c;
    if (c == 0) return;
    const u32 kind = static_cast<u32>(VCall(ctx, c, 32, {c}));
    if (kind != 5) return;
    const u32 h = load<u32>(c + 28);
    ctx.gpr[3] = h;
    if (h == 0) return;
    const u32 k = load<u32>(o + 192);
    const u32 done = static_cast<u32>(call(ctx, kPartDone, {load<u32>(kSession), load<u32>(load<u32>(o + 60) + 288 * k + 2916)}));
    if (done != 0) return;
    const u32 h2 = load<u32>(c + 28);
    ctx.gpr[3] = VCall(ctx, h2, 40, {h2, 0, 1});
}

// u32 Child_ArcLand407520(Child* c): during the state 10 arc (clock c->116
// + 0.2 over the duration c->124, clamped to 1) the height c->100 + (fx
// c->28's +68 - c->100) * t + c->128 * (1 - (2 (t - 0.5))^2) entering the
// band (-1.2, 2.1) lands the child: state 11, the owner's (c->4) effect r =
// c->4->vfn[272]() gets its kind-4 component 0x87208BC1 (vfn[32]()) posed
// from the owner's rows c->4+96.. (+32 / +48 / +64 / +80) and the cue
// Effect_SetHash(r, 0xFD2A); c->120 = c->116 + 0.2 and r3 = 1. Else r3 = 0.
// f1 as the original leaves it.
DECOMP(00407520, Child_ArcLand407520) {
    const u32 c = arg<u32>(ctx, 0);
    constexpr u32 C = 0x00407500u;  // 0.2f, 1.0f, 0.5f, 2.0f, 2.1, -1.2 (doubles)
    const u32 st = load<u32>(c + 112);
    const u32 fx = load<u32>(c + 28);
    f64 f1 = Lf(c + 100);
    const u32 own = load<u32>(c + 4);
    ctx.fpr[1] = f1;
    ctx.gpr[3] = 0;
    if (st != 10) return;
    const f64 dt = Lf(C);
    f64 t = Divs(S(Lf(c + 116) + dt), Lf(c + 124));
    const f64 one = Lf(C + 4);
    if (t > one) t = one;
    const f64 u = S(S(t - Lf(C + 8)) * Lf(C + 12));
    const f64 d = S(Lf(fx + 68) - f1);
    f1 = Fmadds(d, t, f1);
    const f64 y = Fmadds(Lf(c + 128), Fnmsubs(u, u, one), f1);
    ctx.fpr[1] = f1;
    if (!(y < Ld(C + 16))) return;
    ctx.fpr[1] = Ld(C + 24);  // lfd f1
    if (!(y > Ld(C + 24))) return;
    store<u32>(c + 112, 11);
    const u32 r = static_cast<u32>(VCall(ctx, own, 272, {own}));
    if (r != 0) {
        const u32 p = static_cast<u32>(call(ctx, kFindComponent, {r, 0x87208BC1u}));
        if (p != 0 && static_cast<u32>(VCall(ctx, p, 32, {p})) == 4) {
            for (u32 row = 0; row < 4; ++row)
                for (u32 k = 0; k < 3; ++k) Cw(p + 32 + 16 * row + 4 * k, own + 96 + 16 * row + 4 * k);
            ctx.fpr[1] = Lf(own + 96 + 56);
        }
        call(ctx, kSetHash, {r, 0xFD2Au});
    }
    const f64 nt = S(Lf(c + 116) + dt);
    ctx.fpr[1] = nt;
    Sf(c + 120, nt);
    ctx.gpr[3] = 1;
}

// Obj* Scene_FindFacing1B028(Scene* L, const f32 pos[3], -, const f32
// dir[3]; f32 maxDist, f32 maxAngle): over the s32 L->0 entries e of L+592
// that are active (e->vfn[120]()) and not excluded (!e->vfn[156]()), the
// object x = e->48 within maxDist of pos (x+144) with the smallest angle
// acosf(clamp(dot(dir, x+144 - pos) / dist, -1, 1)) below maxAngle. r3 = that
// object or 0; f1 as the original leaves it.
DECOMP(0001B028, Scene_FindFacing1B028) {
    const u32 L = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1), dir = arg<u32>(ctx, 3);
    const f64 maxDist = ctx.fpr[1];
    f64 best = ctx.fpr[2];
    constexpr u32 C = 0x0001B008u;  // 1.0f, 1.0, -1.0f, -1.0
    u32 found = 0;
    if (static_cast<s32>(load<u32>(L)) > 0) {
        const f64 k1 = Lf(C), d1 = Ld(C + 8), km = Lf(C + 16), dm = Ld(C + 24);
        for (s32 i = 0, off = 0; i < static_cast<s32>(load<u32>(L)); ++i, off += 4) {
            const u32 e = load<u32>(L + 592 + off);
            if (static_cast<u32>(VCall(ctx, e, 120, {e})) == 0) continue;
            const u32 e2 = load<u32>(L + 592 + off);
            if (static_cast<u32>(VCall(ctx, e2, 156, {e2})) != 0) continue;
            const u32 x = load<u32>(load<u32>(L + 592 + off) + 48);
            const f64 dy = S(Lf(x + 148) - Lf(pos + 4));
            const f64 dx = S(Lf(x + 144) - Lf(pos));
            const f64 dz = S(Lf(x + 152) - Lf(pos + 8));
            const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy))));
            ctx.fpr[1] = len;
            if (!(len <= maxDist)) continue;
            const f64 dot = Fmadds(Lf(dir + 8), dz, Fmadds(Lf(dir), dx, S(Lf(dir + 4) * dy)));
            f64 c = Divs(dot, len);
            c = S(Fsel15(S(k1 - c), c, d1));
            c = S(Fsel15(S(c - km), c, dm));
            c = S(Fsel15(S(c + k1), c, dm));
            c = S(Fsel15(S(c - k1), d1, c));
            const f64 a = callf(ctx, 0x005E649Cu, {}, {c});  // acosf
            if (a < best) {
                found = x;
                best = a;
            }
        }
    }
    ctx.gpr[3] = found;
}

namespace {
constexpr u32 kHandleOpen = 0x00443CE0u;   // 0x00443CE0(handle, Params* p)
constexpr u32 kHandleTouch = 0x00443A78u;  // 0x00443A78(handle, 0)
constexpr u32 kRestVec = 0x01164B40u;      // f32[3] copied into the default parameters
}  // namespace

// Handle_Acquire443EC0(u32* out, Slot* a, Params* p): a slot with its byte
// a->97 set and a live handle a->4 (pool enabled byte 0x010AEC60, serial =
// low 16 bits, index = high 16 bits < s32 0x010B0200->24, entry +256 of
// 272-byte records at 0x010B0200->28 with the serial in bits 15..30 and bit
// 0x80 of its first byte clear) gets *out = the handle (re-read after the calls),
// else *out = 0 and r3 = 0. Parameters default to a local block (two copies
// of *0x01164B40, 1.0f, zeros); unless the byte 0x00F65B88 or a->102 is set
// (then a->100 = 1): 0x00443CE0(handle, p ? p : &local), 0x00443A78(a->4,
// 0) and a->98 = 1. Handlers left in the local block at +60 / +64 get
// vfn[40](h, 0) (+60 cleared before the second). r3 as left.
DECOMP(00443EC0, Handle_Acquire443EC0) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    u32 p = arg<u32>(ctx, 2);
    Stack frame(ctx, 112);
    const u32 sp = frame.addr() - 112;
    const u32 h = load<u32>(a + 4);
    bool ok = false;
    if (load<u8>(a + 97) != 0 && load<u8>(0x010AEC60u) != 0) {
        const u32 serial = h & 0xFFFF;
        const u32 idx = h >> 16;
        constexpr u32 T = 0x010B0200u;
        if (serial != 0 && static_cast<s32>(idx) < static_cast<s32>(load<u32>(T + 24))) {
            const u32 e = load<u32>(T + 28) + 272 * idx + 256;
            ok = ((load<u32>(e) >> 15) & 0xFFFF) == serial && (load<u8>(e) & 0x80) == 0;
        }
    }
    if (!ok) {
        store<u32>(out, 0);
        ctx.gpr[3] = 0;
        return;
    }
    const u32 L = sp + 112;  // the local parameter block (words +12 / +28 / +52 stay unset)
    for (u32 k = 0; k < 3; ++k) {
        Cw(L + 4 * k, kRestVec + 4 * k);
        Cw(L + 16 + 4 * k, kRestVec + 4 * k);
    }
    ctx.fpr[1] = Lf(kRestVec);
    store<u32>(L + 64, 0);
    store<u32>(L + 60, 0);
    store<u32>(L + 56, 0);
    store<u32>(L + 48, 0);
    store<u8>(L + 44, 0);
    store<u32>(L + 40, 0);
    store<u32>(L + 36, 0);
    store<u32>(L + 32, 0x3F800000u);
    store<u8>(L + 45, 0);
    store<u8>(L + 46, 0);
    store<u8>(L + 47, 0);
    if (p == 0) p = L;
    if (load<u8>(0x00F65B88u) != 0 || load<u8>(a + 102) != 0) {
        store<u8>(a + 100, 1);
    } else {
        call(ctx, kHandleOpen, {h, p});
        call(ctx, kHandleTouch, {load<u32>(a + 4), 0});
        store<u8>(a + 98, 1);
    }
    const u32 q = load<u32>(L + 60);
    const u32 res = load<u32>(a + 4);
    if (q != 0) VCall(ctx, q, 40, {q, 0});
    const u32 q2 = load<u32>(L + 64);
    ctx.gpr[3] = q2;
    if (q2 != 0) {
        store<u32>(L + 60, 0);
        ctx.gpr[3] = VCall(ctx, q2, 40, {q2, 0});
    }
    store<u32>(out, res);
}

namespace {
// The effect cue of Obj_CueEffects3B5388: the kind-1 component 0xE965B9B4 of
// fx (vfn[32]() == 1) takes the value o->96->316 at +20 (raw word, f1 left
// with it), then Effect_SetHash(fx, 0xAAC9DC92); `own` re-reads fx from
// o->32 for that call, as the original does. r3 = Effect_SetHash's result.
void CueAAC9(PPUContext& ctx, u32 o, u32 fx, bool own) {
    const u32 c = static_cast<u32>(call(ctx, kFindComponent, {fx, 0xE965B9B4u}));
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 1) {
        const u32 src = load<u32>(o + 96) + 316;
        ctx.fpr[1] = Lf(src);
        Cw(c + 20, src);
    }
    call(ctx, kSetHash, {own ? load<u32>(o + 32) : fx, 0xAAC9DC92u});
}
}  // namespace

// Obj_CueEffects3B5388(Obj* o): the cue 0xAAC9DC92 (CueAAC9) on the effect
// o->104->vfn[272]() (with o->104), on o->132->32->vfn[272]() (with both
// links) and on o->32 (when set). r3 as the original leaves it.
DECOMP(003B5388, Obj_CueEffects3B5388) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = load<u32>(o + 104);
    if (x != 0) {
        const u32 fx = static_cast<u32>(VCall(ctx, x, 272, {x}));
        if (fx != 0) CueAAC9(ctx, o, fx, false);
    }
    u32 y = load<u32>(o + 132);
    if (y != 0 && (y = load<u32>(y + 32)) != 0) {
        const u32 fx = static_cast<u32>(VCall(ctx, y, 272, {y}));
        if (fx != 0) CueAAC9(ctx, o, fx, false);
    }
    const u32 fx = load<u32>(o + 32);
    if (fx != 0) CueAAC9(ctx, o, fx, true);  // else r3 stays as the steps above left it (o at first)
}

namespace {
constexpr u32 kTrackSet36 = 0x0029879Cu;   // Anim_TrackSet36_0029879C(a, 0, 0)
constexpr u32 kTrackSet164 = 0x000D8760u;  // Anim_TrackSet164_000D8760(o, 0, 0)
constexpr u32 kSlotApply = 0x000B6208u;    // 0x000B6208(x, w0, w1)

// The animation-done test of Obj_Layer2TickBE7F0: Anim_TrackSet36(o->720, 0,
// 0) with an animator o->720, else Anim_TrackSet164(o, 0, 0).
u32 TrackDone(PPUContext& ctx, u32 o) {
    const u32 a = load<u32>(o + 720);
    return static_cast<u32>(a != 0 ? call(ctx, kTrackSet36, {a, 0, 0}) : call(ctx, kTrackSet164, {o, 0, 0}));
}
}  // namespace

// Obj_Layer2TickBE7F0(Obj* o) (vtable entry): the layer-2 state s =
// o->vfn[32](o, 2): 0 -> cue 0x081279EA on o->356 and o->vfn[28](o, 2, 1);
// 1 / 2, once the animation is done (TrackDone): cue 0xFEF79CEC / 0xB3F507F7
// (with an effect o->356), for 1 also 0x000B6208(o->488, the two words at
// o->20 + u16 o->6), then o->vfn[28](o, 2, 2 / 3). r3 as left.
DECOMP(000BE7F0, Obj_Layer2TickBE7F0) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = static_cast<s32>(VCall(ctx, o, 32, {o, 2}));
    if (s == 0) {
        const u32 fx = load<u32>(o + 356);
        if (fx != 0) call(ctx, kSetHash, {fx, 0x081279EAu});
        ctx.gpr[3] = VCall(ctx, o, 28, {o, 2, 1});
        return;
    }
    if (s != 1 && s != 2) return;
    if (TrackDone(ctx, o) == 0) return;
    const u32 fx = load<u32>(o + 356);
    if (fx != 0) call(ctx, kSetHash, {fx, s == 1 ? 0xFEF79CECu : 0xB3F507F7u});
    if (s == 1) {
        const u32 p = load<u32>(o + 20) + load<u16>(o + 6);
        call(ctx, kSlotApply, {load<u32>(o + 488), load<u32>(p), load<u32>(p + 4)});
    }
    ctx.gpr[3] = VCall(ctx, o, 28, {o, 2, s == 1 ? 2u : 3u});
}
