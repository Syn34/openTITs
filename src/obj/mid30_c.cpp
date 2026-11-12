// Game code, medium sweep part 30: targeting, object shutdown / update,
// valve counters, dying-object cleanup, damage relay and contact tracking.
#include "mid30.h"

using namespace sly4;
using namespace sly4::mid30;

namespace {
using namespace m30fn;

// The player when its +192 has bit 2, else 0.
u32 PlayerC30() {
    const u32 p = load<u32>(load<u32>(kWorld29) + 4216);
    return p != 0 && (load<u32>(p + 192) & 2) != 0 ? p : 0;
}

// A virtual call returning a struct: o->vt[slot](out, o) with the OPD /
// entry after them in r5 / r6 (as the original's sequence leaves them).
u64 VCallOut30(PPUContext& ctx, u32 o, u32 slot, u32 out) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return call_ptr(ctx, opd, {out, o, opd, load<u32>(opd)});
}

// The first slot of the u32 array data[0..n) holding v is cleared.
void ClearFirst30(u32 data, s32 n, u32 v) {
    for (s32 i = 0; i < n; ++i)
        if (load<u32>(data + 4 * static_cast<u32>(i)) == v) {
            store<u32>(data + 4 * static_cast<u32>(i), 0);
            return;
        }
}

// Every u32 of arr[0..n) equal to v is cleared.
void ClearAll30(u32 n, u32 arr, u32 v) {
    for (u32 i = 0; i < n; ++i)
        if (load<u32>(arr + 4 * i) == v) store<u32>(arr + 4 * i, 0);
}

constexpr u32 FindCompObj30 = 0x000D1288u;  // Obj_FindComponent(o, kind, 0)
}  // namespace

// u32 f(T* o, Obj* t): aims o at t (t needs bit 0x40000 of +172; nothing
// while the lock byte *0x00F9BC28 is set). A new target replaces +256
// unless vt[124](o) refuses (then the old one, when targetable, gets
// vt[40](0) and t's u16 +178 is incremented); (o->+48)->vt[96](1) and the
// body's +164 flags (u16 +40: bit 0x1000 set, 0x400 cleared); +256 = t.
// The direction from the body's vt[80] point to t's vt[80] point is
// normalised into +272..+280 with its length in +288; aiming at the player
// also copies t's point to (*0x00F557F0)->+48.. (+44 = 0). r3 / f1 as the
// original leaves them.
DECOMP(00016218, Obj_SetTarget_00016218) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = arg<u32>(ctx, 1);
    if (t == 0) return;  // r3 = o
    if ((load<u32>(t + 172) & 0x00040000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (const u8 lk = load<u8>(0x00F9BC28u); lk != 0) {
        ctx.gpr[3] = lk;
        return;
    }
    if (load<u32>(o + 256) != t && VCall(ctx, o, 124, {o}) == 0) {
        if (const u32 cur = load<u32>(o + 256); cur != 0 && (load<u32>(cur + 172) & 0x00040000u) != 0) VCall(ctx, cur, 40, {cur, 0});
        store<u16>(t + 178, static_cast<u16>(load<u16>(t + 178) + 1));
    }
    {
        const u32 b = load<u32>(o + 48);
        VCall(ctx, b, 96, {b, 1});
    }
    if (const u32 q = load<u32>(load<u32>(o + 48) + 164); q != 0)
        store<u16>(q + 40, static_cast<u16>((load<u16>(q + 40) | 0x1000u) & ~0x0400u));
    store<u32>(o + 256, t);
    Stack frame(ctx, 32);  // the body point at 112(r1), t's point at 128(r1)
    const u32 sp = frame.addr(), tp = sp + 16;
    VCallOut30(ctx, t, 80, tp);
    const u32 b = load<u32>(o + 48);
    if (t == PlayerC30()) {
        const u32 g = load<u32>(kGame30);
        store<u32>(g + 48, load<u32>(tp));
        store<u32>(g + 52, load<u32>(tp + 4));
        store<u32>(g + 44, 0);
        store<u32>(g + 56, load<u32>(tp + 8));
    }
    VCallOut30(ctx, b, 80, sp);
    const f64 dy = Sgl28(Lf(tp + 4) - Lf(sp + 4));
    const f64 dx = Sgl28(Lf(tp) - Lf(sp));
    const f64 dz = Sgl28(Lf(tp + 8) - Lf(sp + 8));
    f64 l = Sgl28(dy * dy);
    l = Sgl28(std::fma(dx, dx, l));
    l = Sgl28(std::fma(dz, dz, l));
    l = Sgl28(std::sqrt(l));
    const f64 nx = Sgl28(dx / l);
    store<u32>(o + 288, Bits28(l));
    const f64 ny = Sgl28(dy / l), nz = Sgl28(dz / l);
    store<u32>(o + 272, Bits28(nx));
    store<u32>(o + 276, Bits28(ny));
    store<u32>(o + 280, Bits28(nz));
    ctx.fpr[1] = ny;
}

// u32 f(Obj* o, u8 full): drops a reference (u16 +178); below zero
// vt[76](o) (r3 = the count). With full: bit 0x40000 of +172 cleared,
// World_DropPlayer46B6CC(world, o); the +252 listeners get vt[296](o)
// (list emptied); the holder +220 gets vt[32](o) and is cleared; a parent
// +336 detaches o (Obj_Detach0D72C0); the +340 slots (240 bytes, object at
// +192) get vt[184](0) and vt[40](1) (list emptied); the +248 stack is
// popped (last moved to the front) with vt[4]() of each top; the
// (+172 >> 19) & 7 sound slots at +256 (12 bytes) are stopped
// (Emitter_Stop(h, 1); h = 0, +8 = 0, +4 = -1); o is cleared from the
// tables 0x01019BE0 (count *0x01019BD0) / 0x0101DBF0 (*0x0101DBE0) and
// swap-removed from the list 0x01013B48. r3 as the original leaves it.
DECOMP(000D74C0, Obj_ShutdownBase_000D74C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 full = arg<u32>(ctx, 1) & 0xFF;
    const u32 n = load<u16>(o + 178) - 1u;
    store<u16>(o + 178, static_cast<u16>(n));
    if (static_cast<s16>(n) < 0) {
        VCall(ctx, o, 76, {o});
        ctx.gpr[3] = load<u16>(o + 178);
        return;
    }
    if (full == 0) {
        ctx.gpr[3] = n;
        return;
    }
    store<u32>(o + 172, load<u32>(o + 172) & ~0x00040000u);
    call(ctx, 0x0046B6CCu, {load<u32>(kWorld29), o});  // World_DropPlayer46B6CC
    u32 v;
    for (u32 i = 0;; ++i) {
        v = load<u32>(o + 252);
        if (i >= (v != 0 ? load<u16>(v + 8) : 0u)) break;
        const u32 e = load<u32>(load<u32>(v) + 4 * i);
        VCall(ctx, e, 296, {e, o});
    }
    if (v != 0) store<u16>(v + 8, 0);
    if (const u32 c = load<u32>(o + 220); c != 0) {
        VCall(ctx, c, 32, {c, o});
        store<u32>(o + 220, 0);
    }
    if (const u32 d = load<u32>(o + 336); d != 0) call(ctx, 0x000D72C0u, {d, o});  // Obj_Detach0D72C0
    for (u32 i = 0;; ++i) {
        v = load<u32>(o + 340);
        if (i >= (v != 0 ? load<u16>(v + 8) : 0u)) break;
        const u32 p = load<u32>(load<u32>(v) + 240 * i + 192);
        if (p == 0) continue;
        VCall(ctx, p, 184, {p, 0});
        VCall(ctx, p, 40, {p, 1});  // full is set here
    }
    if (v != 0) store<u16>(v + 8, 0);
    for (;;) {
        const u32 s = load<u32>(o + 248);
        if (s == 0 || load<u16>(s + 8) == 0) break;
        const u32 d = load<u32>(s);
        const u32 c = load<u16>(s + 8) - 1u;
        const u32 top = load<u32>(d);
        store<u16>(s + 8, static_cast<u16>(c));
        if ((c & 0xFFFF) != 0) store<u32>(d, load<u32>(d + ((c << 2) & 0x0003FFFCu)));
        if (top != 0) VCall(ctx, top, 4, {top});
    }
    for (u32 i = 0; i < ((load<u32>(o + 172) >> 19) & 7); ++i) {
        const u32 s = o + 256 + 12 * i;
        call(ctx, 0x00443424u, {load<u32>(s), 1});  // Emitter_Stop
        store<u32>(s + 0, 0);
        store<u32>(s + 8, 0);
        store<u32>(s + 4, 0xFFFFFFFFu);
    }
    ClearAll30(load<u32>(0x01019BD0u), 0x01019BE0u, o);
    ClearAll30(load<u32>(0x0101DBE0u), 0x0101DBF0u, o);
    const u32 L = 0x01013B48u;
    u32 i = 0;
    for (; i < load<u16>(L + 8); ++i) {
        const u32 d = load<u32>(L);
        if (load<u32>(d + 4 * i) != o) continue;
        const u32 c = load<u16>(L + 8) - 1u;
        store<u16>(L + 8, static_cast<u16>(c));
        if (i == (c & 0xFFFF)) {
            ctx.gpr[3] = c & 0xFFFF;
            return;
        }
        const u32 x = load<u32>(d + ((c << 2) & 0x0003FFFCu));
        store<u32>(d + 4 * i, x);
        ctx.gpr[3] = x;
        return;
    }
    ctx.gpr[3] = 4 * i;
}

// u32 f(Obj* o): the per-frame object update after Obj_UpdateBase (0 when
// that fails, else 1). Bit 0x20000000 of +376 makes the +216 def's effect
// +184 (Obj_CreateEffectDCA20(o, fx, 0, 1)) with hash 0x04B8CC42; bit
// 0x40000000 finishes the effect +352 with hash 0x04B6FB5B (first, with
// bit 0x80000000 and a +160 owner whose +204 is a known prop in the map
// 0x01013B1C with bit 0x40 of +12, 0x0049FCC0(fx, prop + 180, 0, 0, 0));
// bit 0x10000000 releases +352 once Effect_Check4CF620(fx, 0) reports done
// (its +136 |= 0x200, cleared). A +348 body without an entity is added to
// the physics world (Hk_AddEntity92F2A8(world->+0x1281D4->+4, body, 1)).
DECOMP(000DE8D8, Obj_Update_000DE8D8) {
    const u32 o = arg<u32>(ctx, 0);
    if (call(ctx, 0x000D83C4u, {o}) == 0) {  // Obj_UpdateBase (reads f1 as passed)
        ctx.gpr[3] = 0;
        return;
    }
    if (const u32 v = load<u32>(o + 376); (v & 0x20000000u) != 0) {
        const u32 d = load<u32>(o + 216);
        store<u32>(o + 376, v & ~0x20000000u);
        if (d != 0)
            if (const u32 fx = load<u32>(d + 184); fx != 0) {
                const u32 e = static_cast<u32>(call(ctx, 0x000DCA20u, {o, fx, 0, 1}));  // Obj_CreateEffectDCA20
                if (e != 0) call(ctx, 0x004CF988u, {e, 0x04B8CC42u});
            }
    }
    u32 v = load<u32>(o + 376);
    if ((v & 0x40000000u) != 0) {
        if (load<u32>(o + 352) != 0) {
            if ((v & 0x80000000u) != 0)
                if (const u32 ow = load<u32>(o + 160); ow != 0) {
                    Stack frame(ctx, 16);  // the key at 112(r1)
                    const u32 key = frame.addr();
                    store<u32>(key, load<u32>(ow + 204));
                    const u32 p = static_cast<u32>(call(ctx, 0x00732F38u, {0x01013B1Cu, key}));  // SortedMap_Find
                    if (const u32 q = load<u32>(p); q != 0 && (load<u32>(q + 12) & 0x40u) != 0)
                        call(ctx, 0x0049FCC0u, {load<u32>(o + 352), q + 180, 0, 0, 0});
                }
            call(ctx, 0x004CF988u, {load<u32>(o + 352), 0x04B6FB5Bu});
            v = load<u32>(o + 376);
        }
        v &= ~0x40000000u;
        store<u32>(o + 376, v);
    }
    if ((v & 0x10000000u) != 0 && load<u32>(o + 352) != 0 && call(ctx, 0x004CF620u, {load<u32>(o + 352), 0}) != 0) {
        const u32 e = load<u32>(o + 352);
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        const u32 f = load<u32>(o + 376);
        store<u32>(o + 352, 0);
        store<u32>(o + 376, f & ~0x10000000u);
    }
    if (const u32 h = load<u32>(o + 348); h != 0 && load<u32>(h + 8) == 0)
        call(ctx, 0x0092F2A8u, {load<u32>(load<u32>(load<u32>(kWorld29) + 0x001281D4u) + 4), h, 1});  // Hk_AddEntity92F2A8
    ctx.gpr[3] = 1;
}

// u32 f(T* o, Obj* t): a valve turned by t (targetable: bit 0x40000 of
// +172) while o (vt[32](0) == 2) has valves (+536, 12 bytes, u16 +544).
// t's unfinished valves get bit 0x80000000 of +8 (and "Valve%i_Finished"
// on the effect +356); the last one's vt[272]() gets "%i_Valve(s)_
// Finished" for the finished count. With some left, a still-active o
// (vt[32](0) == 2) whose vt[32](1) is 0 / 1 marks u8 (o->+20)[u16 +6] + 4.
// All done: 0x00470660(o, 0xAFEE1340, *(0x00F64704 + 4 * +724), 0). r3 as
// the original leaves it.
DECOMP(000BC7C8, Valve_Turn_000BC7C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = arg<u32>(ctx, 1);
    if (t == 0) return;  // r3 = o
    if ((load<u32>(t + 172) & 0x00040000u) == 0 || load<u16>(o + 544) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) != 2) return;
    u32 done = 0, last = 0;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 544);
        if (i >= n) {
            ctx.gpr[3] = n;
            break;
        }
        const u32 e = load<u32>(o + 536) + 12 * i;
        u32 f = load<u32>(e + 8);
        if ((f & 0x80000000u) == 0 && load<u32>(e) == t) {
            if (const u32 fx = load<u32>(o + 356); fx != 0) {
                const u32 s = static_cast<u32>(call(ctx, Format29, {0x00AC6450u, i + 1}));  // "Valve%i_Finished"
                const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));             // the string's hash
                call(ctx, 0x004CF988u, {fx, h});
                f = load<u32>(e + 8);
            }
            last = load<u32>(e);
            f |= 0x80000000u;
            store<u32>(e + 8, f);
        }
        if ((f & 0x80000000u) != 0) ++done;
    }
    if (last == 0) return;
    if (const u32 c = static_cast<u32>(VCall(ctx, last, 272, {last})); c != 0) {
        const u32 s = static_cast<u32>(call(ctx, Format29, {0x00AC6468u, done + 1}));  // "%i_Valve(s)_Finished"
        const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
        call(ctx, 0x004CF988u, {c, h});
    }
    if (done >= load<u16>(o + 544)) {
        call(ctx, 0x00470660u, {o, 0xAFEE1340u, load<u32>(0x00F64704u + 4 * load<u32>(o + 724)), 0});
        return;
    }
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) != 2) return;
    if (static_cast<u32>(VCall(ctx, o, 32, {o, 1})) != 0 && static_cast<u32>(VCall(ctx, o, 32, {o, 1})) != 1) return;
    store<u8>(load<u32>(o + 20) + load<u16>(o + 6) + 4, 1);
    ctx.gpr[3] = 1;
}

// u32 f(Obj* o, u8 full): with full, o is shut down: a world timed callback
// (World_AddTimedCallback(world, 12, 0x00F32400, o, o)), the list
// 0x01013B48 removal, vt[12](o), Obj_Reset121FA8(o, 0, 0, 0, 0, 1); the
// helpers +428 / +420 get vt[40](0); +380 / +396..+404 cleared; the
// pieces +392 / +388 (unless shared with +384) and +384 deleted (vt[4]);
// the effects +408 / +412 and the eight at +796 (16 bytes apart) get bit
// 0x200 of +136 and are cleared; +484 / +488 get vt[8](); every child left
// in +1328 (u16 +1336) gets vt[300](o) until the list empties. Then
// Obj_SetRagdollBase(o, full) (r3).
DECOMP(00122518, Obj_Kill_00122518) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 full = arg<u32>(ctx, 1) & 0xFF;
    if (full != 0) {
        call(ctx, 0x0046B5F8u, {load<u32>(kWorld29), 12, 0x00F32400u, o, o});  // World_AddTimedCallback
        call(ctx, 0x000D68CCu, {o});                                          // List1013B48_Remove
        VCall(ctx, o, 12, {o});
        call(ctx, 0x00121FA8u, {o, 0, 0, 0, 0, 1});  // Obj_Reset121FA8
        if (const u32 a = load<u32>(o + 428); a != 0) VCall(ctx, a, 40, {a, 0});
        const u32 b = load<u32>(o + 420);
        store<u32>(o + 428, 0);
        if (b != 0) VCall(ctx, b, 40, {b, 0});
        u32 c384 = load<u32>(o + 384);
        store<u32>(o + 420, 0);
        const u32 c392 = load<u32>(o + 392);
        store<u32>(o + 380, 0);
        store<u32>(o + 396, 0);
        store<u32>(o + 400, 0);
        store<u32>(o + 404, 0);
        if (c392 != c384 && c392 != 0) {
            VCall(ctx, c392, 4, {c392});
            c384 = load<u32>(o + 384);
        }
        const u32 c388 = load<u32>(o + 388);
        store<u32>(o + 392, 0);
        if (c388 != c384 && c388 != 0) {
            VCall(ctx, c388, 4, {c388});
            c384 = load<u32>(o + 384);
        }
        store<u32>(o + 388, 0);
        if (c384 != 0) {
            VCall(ctx, c384, 4, {c384});
            store<u32>(o + 384, 0);
        }
        for (u32 off : {408u, 412u}) {
            if (const u32 e = load<u32>(o + off); e != 0) {
                store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
                store<u32>(o + off, 0);
            }
        }
        for (u32 k = 0; k < 8; ++k) {
            const u32 s = o + 796 + 16 * k;
            if (const u32 e = load<u32>(s); e != 0) {
                store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
                store<u32>(s, 0);
            }
        }
        for (u32 off : {484u, 488u}) {
            if (const u32 p = load<u32>(o + off); p != 0) {
                VCall(ctx, p, 8, {p});
                store<u32>(o + off, 0);
            }
        }
        while (load<u16>(o + 1336) != 0) {
            const u32 x = load<u32>(load<u32>(o + 1328));
            VCall(ctx, x, 300, {x, o});
        }
        store<u16>(o + 1336, 0);
    }
    call(ctx, 0x000E02F8u, {o, full});  // Obj_SetRagdollBase
}

// u32 f(Obj* o, Obj* e, u32 a, u32 kind; f32 dmg): relays damage to o
// when its health +348 is above *0x001275B4 (else 0). The damage is
// clamped to that floor for a killing blow on a flagged target (bit 0 of
// +188 with the cheat byte *0x010388F0, or an unflagged one whose holder
// +244 has bit 0x10 of u8 +305, or one without the +172 state bits
// 0x00C00000 and bit 0x08000000 of +188). Obj_ApplyDamage(o, -, a, kind;
// dmg) gives r3. A blow above the floor resets o: the +424 attacker's
// +244 gets vt[204](def +448) and Obj_Reset121FA8(o, 0, 0, 0, 0, 1);
// bit 0 of +188 releases the treasure (World_ReleaseTreasure_0047BC68
// (world, (s8) +172)), else bit 0x00400000 calls 0x0047C1F0(world, o); a
// kind other than 0x20CE2C3A posts a zeroed event (0x00465494(ev, o, 0,
// 0, 0), World_Query482D14(world, (s8) +172, ev)).
DECOMP(001275B8, Obj_DamageRelay_001275B8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 2);
    const u32 kind = arg<u32>(ctx, 3);
    f64 dmg = ctx.fpr[1];
    const f64 f31 = Lf(0x001275B4u);
    const u32 hold = load<u32>(o + 244);
    if (Lf(o + 348) <= f31) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = load<u32>(o + 188);  // the low word of the u64 +184
    bool clamp;
    u32 r4 = hold;  // what r4 holds at the call (Obj_ApplyDamage does not read it)
    if ((v & 1) != 0) {
        clamp = load<u8>(0x010388F0u) != 0;
    } else if (hold == 0) {
        clamp = false;
    } else {
        r4 = (load<u8>(hold + 305) >> 4) & 1;
        if (r4 != 0) clamp = true;
        else if (((load<u32>(o + 172) >> 22) & 3) != 0) clamp = false;
        else clamp = (v & 0x08000000u) == 0;
    }
    if (clamp) dmg = f31;
    callf(ctx, 0x000E1738u, {o, r4, a, kind}, {dmg});  // Obj_ApplyDamage
    const u32 r = static_cast<u32>(ctx.gpr[3]);
    if (!(dmg > f31)) {
        ctx.gpr[3] = r;
        return;
    }
    if (const u32 at = load<u32>(o + 424); at != 0) {
        const u32 d = load<u32>(o + 216);
        if (const u32 b = load<u32>(at + 244); b != 0) VCall(ctx, b, 204, {b, d + 448});
        call(ctx, 0x00121FA8u, {o, 0, 0, 0, 0, 1});  // Obj_Reset121FA8
    }
    const u32 v2 = load<u32>(o + 188);
    const u64 slot = static_cast<u64>(static_cast<s64>(static_cast<s8>(load<u8>(o + 172))));
    if ((v2 & 1) != 0) call(ctx, 0x0047BC68u, {load<u32>(kWorld29), slot});  // World_ReleaseTreasure_0047BC68
    else if ((v2 & 0x00400000u) != 0) call(ctx, 0x0047C1F0u, {load<u32>(kWorld29), o});
    if (kind != 0x20CE2C3Au) {
        Stack frame(ctx, 32);  // the event at 112(r1)..135(r1)
        const u32 ev = frame.addr();
        for (u32 k = 0; k < 24; k += 4) store<u32>(ev + k, 0);
        call(ctx, 0x00465494u, {ev, o, 0, 0, 0});
        call(ctx, 0x00482D14u, {load<u32>(kWorld29), static_cast<u64>(static_cast<s64>(static_cast<s8>(load<u8>(o + 172)))), ev});  // World_Query482D14
    }
    ctx.gpr[3] = r;
}

// u32 f(T* o, Contact* c): counts contacts with o's body (vt[144]) while
// fewer than 129 are tracked. The other body (c->+4 / +8) is ignored when
// its 0x91C7CF5B table entry (+124, 16 bytes, s32 +128) points at an
// object with bit 0x400 of +192 (then bit 0x08 of u8 (c->+24)->+15 is
// set); a known one bumps its count (+520[i]); a new one is appended to
// +508 (s32 +512, capacity +516 & 0x3FFFFFFF) with count 1 in +520 (+524,
// +528), growing both with hkArray 0x007CCD60(0x011DF31C, arr, 4). r3 as
// the original leaves it.
DECOMP(001728F0, Contact_Track_001728F0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    const u32 n0 = load<u32>(o + 512);
    if (static_cast<s32>(n0) > 128) {
        ctx.gpr[3] = n0;
        return;
    }
    if (VCall(ctx, o, 144, {o}) == 0) return;
    const u32 me = static_cast<u32>(VCall(ctx, o, 144, {o}));
    const u32 data = load<u32>(o + 508), n = load<u32>(o + 512);
    if (me == 0) {
        ctx.gpr[3] = n;
        return;
    }
    u32 t = load<u32>(c + 4);
    if (t == me) t = load<u32>(c + 8);
    if (t == 0) {
        ctx.gpr[3] = n;
        return;
    }
    u32 p = 0;
    for (s32 i = 0; i < static_cast<s32>(load<u32>(t + 128)); ++i) {
        const u32 e = load<u32>(t + 124) + 16 * static_cast<u32>(i);
        if (load<u32>(e) == 0x91C7CF5Bu) {
            p = load<u32>(e + 12);  // the low word of the u64 +8
            break;
        }
    }
    if (p != 0 && (load<u32>(p + 192) & 0x400u) != 0) {
        const u32 b = load<u32>(c + 24);
        store<u8>(b + 15, load<u8>(b + 15) | 8);
        ctx.gpr[3] = b;
        return;
    }
    for (s32 i = 0; i < static_cast<s32>(n); ++i)
        if (load<u32>(data + 4 * static_cast<u32>(i)) == t) {
            const u32 a = load<u32>(o + 520) + 4 * static_cast<u32>(i);
            store<u32>(a, load<u32>(a) + 1);
            ctx.gpr[3] = a;
            return;
        }
    const u32 H = 0x011DF31Cu;
    u32 cnt = n, dat = data;
    if (cnt == (load<u32>(o + 516) & 0x3FFFFFFFu)) {
        call(ctx, 0x007CCD60u, {H, o + 508, 4});  // hkArray reserve
        cnt = load<u32>(o + 512);
        dat = load<u32>(o + 508);
    }
    store<u32>(dat + 4 * cnt, t);
    const u32 n3 = load<u32>(o + 512) + 1;
    u32 c2 = load<u32>(o + 524);
    const u32 cap2 = load<u32>(o + 528) & 0x3FFFFFFFu;
    store<u32>(o + 512, n3);
    if (c2 == cap2) {
        call(ctx, 0x007CCD60u, {H, o + 520, 4});
        c2 = load<u32>(o + 524);
    }
    store<u32>(load<u32>(o + 520) + 4 * c2, 1);
    const u32 n4 = load<u32>(o + 524) + 1;
    store<u32>(o + 524, n4);
    ctx.gpr[3] = n4;
}

// u32 f(T* o, Contact* c): a contact of kind 0xCCF83851 / 0xB94AE51B with
// body b = c->+0. b's vt[144] object is cleared from the tracked lists of
// the other player (two-player mode: world slot 1 - o->+772) and of o.
// A live b (bit 2 of +192) owned by o (+372): +748 += 1 unless the kind is
// 0xB94AE51B, and its kind-40 component q gets 0x002B21D8(q, n, m) with n
// = (s32) q->+24 and m = o->+668 (1 when 0); the player's +752 += m * n.
// r3 / f1 as the original leaves them.
DECOMP(00175758, Contact_Count_00175758) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    const u32 kind = load<u32>(c + 8);
    const u32 b = load<u32>(c + 0);
    if (kind != 0xCCF83851u && kind != 0xB94AE51Bu) {
        ctx.gpr[3] = kind;
        return;
    }
    if (VCall(ctx, b, 144, {b}) != 0) {
        const u32 w = load<u32>(kWorld29);
        if (load<u8>(w + 29) != 1) {
            const u32 p2 = load<u32>(w + (1 - load<u32>(o + 772)) * 0x00020C80u + 4216);
            if (p2 != 0)
                if (const u32 r = static_cast<u32>(VCall(ctx, b, 144, {b})); r != 0) {
                    const s32 n2 = static_cast<s32>(load<u32>(p2 + 512));
                    for (s32 i = 0; i < n2; ++i)
                        if (load<u32>(load<u32>(p2 + 508) + 4 * static_cast<u32>(i)) == r) {
                            store<u32>(load<u32>(p2 + 508) + 4 * static_cast<u32>(i), 0);
                            break;
                        }
                }
        }
        if (const u32 r = static_cast<u32>(VCall(ctx, b, 144, {b})); r != 0)
            ClearFirst30(load<u32>(o + 508), static_cast<s32>(load<u32>(o + 512)), r);
    }
    if ((load<u32>(b + 192) & 2) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    u32 m = load<u32>(o + 668);
    if (m == 0) m = 1;
    if (b == 0) {  // dead: b was dereferenced above
        ctx.gpr[3] = 2;
        return;
    }
    const u32 own = load<u32>(b + 372);
    if (own != o) {
        ctx.gpr[3] = own;
        return;
    }
    if (load<u32>(c + 8) != 0xB94AE51Bu) store<u32>(o + 748, load<u32>(o + 748) + 1);
    const u32 q = static_cast<u32>(call(ctx, FindCompObj30, {b, 40, 0}));
    if (q == 0) return;
    const u64 fi = rt::fctid(Lf(q + 24), true);
    const u32 k = static_cast<u32>(fi);
    ctx.fpr[1] = std::bit_cast<f64>(fi);
    call(ctx, 0x002B21D8u, {q, k, m});
    const u32 p = load<u32>(load<u32>(kWorld29) + 4216);
    store<u32>(p + 752, load<u32>(p + 752) + m * k);
    ctx.gpr[3] = p;
}
