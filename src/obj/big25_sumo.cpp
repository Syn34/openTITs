// Game code, large-function sweep part 25: the sumo minigame's target
// spawner tick (state machine; state 0 picks a random spline and spawns the
// "Target" effect object along it).
#include "../sly4.h"
#include "big25.h"

using namespace sly4;
using namespace sly4::big25;

namespace {
constexpr u32 kCSumo = 0x0040E9EC;              // .text f32 table: 1/65535, 0.7, 0.8, 0, 1, 0.25
constexpr u32 kRng = 0x01164888;                // u64 last output; +8 u32 state
constexpr u32 kRngXor = 0x87654321u;
constexpr u32 kSplineHolder = 0x00F8F158;       // header*; +16 the map SplineMap_Find searches
constexpr u32 kSplineDefault = 0xE720C39Cu;     // the spline used when the game is not in progress
constexpr u32 kAnimKey = 0x8919BDE9u;           // the animation restarted on the animator
constexpr u32 kProfileKey = 0xC5877D2Du;        // Save_Profile0C9FC8 key (the target effect def)
constexpr u32 kPoolMap = 0x01013B1C;            // SortedMap_Find732F38 map of pooled spawn descs
constexpr u32 kDescVt = 0x00B12260;             // the stack spawn desc's vtable
constexpr u32 kTargetStr = 0x00ADFC08;          // "Target"
constexpr u32 kVec_01164B30 = 0x01164B30;       // f32[3] copied into the desc (+96)
constexpr u32 kDoneFlag = 0x0106B62C;           // u8 set when a round ends
constexpr u32 kSaveProfile = 0x000C9FC8;        // Save_Profile0C9FC8 (key) -> def or 0
constexpr u32 kAnimRestart = 0x002775A8;        // Anim_Restart2775A8 (12 arguments; f1..f4)
constexpr u32 kSet180 = 0x0036EA80;             // Obj_Set180_0036EA80 (Obj*, a)
constexpr u32 kRoundEnd = 0x0040E220;           // (Game*)
constexpr u32 kState3 = 0x0040E400;             // (Spawner*; f1 as the caller left it)
constexpr u32 kState1 = 0x0040E6E0;             // (Spawner*; f1 as the caller left it)
constexpr u32 kNop = 0x0054EC18;
constexpr u32 kPtrVectorClear = 0x0054EC1C;
constexpr u32 kPtrVectorConstruct = 0x005516E0;
constexpr u32 kSmallAlloc = 0x0055238C;         // (size) -> p or 0
constexpr u32 kSmallFree = 0x00552420;          // (p, size)
constexpr u32 kStringCopy = 0x00553684;         // (dst, src, n)
constexpr u32 kSplineTangent = 0x005A80D8;      // WorldObj_SplinePointTangentAt (spline, -, f32 pos[4], f32 tan[4], bool back; f1 t)
constexpr u32 kSortedMapFind = 0x00732F38;      // (map, const u32* key) -> slot*
constexpr u32 kSplineMapFind = 0x007335C4;      // (map, const u32* key) -> slot*
constexpr u32 kVectorAdd = 0x00733964;          // Vector_Add733964 (Vector*, const Rec* (192 bytes))

f64 C(u32 off) { return Lf(kCSumo + off); }
u32 Rotl(u32 v, u32 n) { return (v << n) | (v >> (32 - n)); }
// One step of the inlined generator: the pre-rotation value (its low 16
// bits are the output stored to the u64 at kRng; rotated by 16 it is the
// next state).
u32 RngMix(u32 s) { return (Rotl(s, 4) + Rotl(s, 16)) ^ kRngXor; }
}  // namespace

// void f(Spawner* o) (called by 00411D30, 00412640). o: +24/+28/+32 three
// spline hashes, +72 the game (u8 +53 / +97: in progress), +76 the
// animator owner, +80 the spawned target, +84 the spline, +88 the state,
// +92/+96 timer (0, 5.0), +112 position, +160 velocity, +176 tangent,
// +192 speed, +196 reversed (u8).
// State 3 / 1: 0x0040E400 / 0x0040E6E0 (o) (with the f1 the caller left).
// State 5: state 0; if the game is in progress, its +468 and +484 count up
// (+484 reaching its def's +64 ends the round, 0x0040E220), Obj_Set180(+480,
// +484), and the byte 0x0106B62C = 1.
// State 0: speed = 0.8 + 0.7 * (last random output) / 65535; a previous
// target (+80) has bit 0x1000 of its +176 set and is dropped (the random
// generator's stored state advances once; without one the advanced state
// is still used below); then reversed = (state & 3) == 1 and the spline
// is one of +24/+28/+32 picked by the next output (mod 3) (the default
// spline 0xE720C39C, not reversed, when the game is not in progress); the
// state becomes 1, velocity 0; the animator (+76 vfn[60]) restarts
// 0x8919BDE9; with the target def (Save_Profile0C9FC8(0xC5877D2D)): the
// spline position / tangent at t 0 (reversed: t 1, tangent negated) gives
// a frame (back = -tangent, up' = right x tangent normalized, right = up x
// tangent normalized; reversed: back and right negated) and the position
// (y - 1) for a spawn desc (on the stack, or the pooled one for
// 0xC5877D2D) named "Target" with the def's +136 records (192 bytes each)
// added to a PtrVector; def->vfn[28](desc) is the target (+80), which gets
// vfn[88](1, 1, 1, 1) and vfn[84](back vector).
DECOMP(0040EA08, Sumo_SpawnerTick_0040EA08) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 512);
    const u32 sp = frame.addr();
    const u32 back = sp + 144, up2 = sp + 160, right = sp + 176, pos = sp + 192;
    const u32 desc = sp + 208, key3 = sp + 408, key2 = sp + 412, key1 = sp + 416, hashes = sp + 420;
    const u32 st = load<u32>(o + 88);

    if (st == 5) {
        store<u32>(o + 88, 0);
        const u32 g = load<u32>(o + 72);
        if (load<u8>(g + 53) == 0 && load<u8>(g + 97) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 a = load<u32>(g + 468), n = load<u32>(g + 484) + 1;
        const u32 def = load<u32>(g + 88);
        store<u32>(g + 468, a + 1);
        store<u32>(g + 484, n);
        u32 cur = n;
        if (n == load<u32>(def + 64)) {
            call(ctx, kRoundEnd, {g});
            cur = load<u32>(g + 484);
        }
        call(ctx, kSet180, {load<u32>(g + 480), cur});
        store<u8>(kDoneFlag, 1);
        ctx.gpr[3] = 0x01070000;  // as the original leaves r3
        return;
    }
    if (st == 3) {
        call(ctx, kState3, {o});
        return;
    }
    if (st == 1) {
        call(ctx, kState1, {o});
        return;
    }
    if (st != 0) {
        ctx.gpr[3] = st;
        return;
    }

    // State 0: pick a spline.
    const u64 last = load<u64>(kRng);
    store<u64>(sp + 400, last);
    u32 s = load<u32>(kRng + 8);
    {
        const u32 m = RngMix(s);
        Sf(o + 192, Fmadds(S(S(static_cast<f64>(static_cast<s64>(last))) * C(0)), C(4), C(8)));
        // The advanced state is used either way, but only stored with a
        // previous target.
        s = Rotl(m, 16);
        if (const u32 prev = load<u32>(o + 80)) {
            store<u32>(kRng + 8, s);
            store<u64>(kRng, m & 0xFFFF);
            store<u16>(prev + 176, static_cast<u16>(load<u16>(prev + 176) | 0x1000));
            store<u32>(o + 80, 0);
            s = load<u32>(kRng + 8);
        }
    }
    const u32 y = RngMix(s);
    const u32 s1 = Rotl(y, 16);
    const u32 z = RngMix(s1);
    store<u8>(o + 196, (s & 3) == 1 ? 1 : 0);
    const f64 zero = C(12);
    store<u32>(hashes, load<u32>(o + 24));
    store<u32>(o + 92, 0);
    store<u32>(hashes + 4, load<u32>(o + 28));
    store<u32>(o + 96, 0x40A00000u);  // 5.0
    store<u32>(hashes + 8, load<u32>(o + 32));
    store<u32>(kRng + 8, Rotl(z, 16));
    store<u64>(kRng, z & 0xFFFF);
    const u32 map = load<u32>(load<u32>(kSplineHolder) + 16);
    store<u32>(key1, load<u32>(hashes + 4 * (s1 % 3)));
    store<u32>(o + 84, load<u32>(static_cast<u32>(call(ctx, kSplineMapFind, {map, key1}))));
    {
        const u32 g = load<u32>(o + 72);
        if (load<u8>(g + 53) == 0 && load<u8>(g + 97) == 0) {
            const u32 map2 = load<u32>(load<u32>(kSplineHolder) + 16);
            store<u32>(key2, kSplineDefault);
            const u32 sp2 = load<u32>(static_cast<u32>(call(ctx, kSplineMapFind, {map2, key2})));
            store<u8>(o + 196, 0);
            store<u32>(o + 84, sp2);
        }
    }
    store<u32>(o + 88, 1);
    Cw(o + 160, b25fn::ZeroVec);
    Cw(o + 164, b25fn::ZeroVec + 4);
    Cw(o + 168, b25fn::ZeroVec + 8);
    {
        const u32 ao = load<u32>(o + 76);
        const u32 an = static_cast<u32>(VCall(ctx, ao, 60, {ao}));
        const u64 r8 = ctx.gpr[8];  // never set by the original: what the vcall left
        // Arguments 9 / 10 (r1 +112 / +120) are not stored by the original;
        // the callee doesn't read them.
        callf(ctx, kAnimRestart, {an, kAnimKey, 0, 0, ~0ull, r8, 0, 0, 0, 0, 0, ~0ull}, {C(16), C(20), zero, zero});
    }
    const u32 def = static_cast<u32>(call(ctx, kSaveProfile, {kProfileKey}));
    if (def == 0) return;

    // The spawn frame.
    const bool rev = load<u8>(o + 196) != 0;
    f64 tx, ty, tz;
    if (!rev) {
        callf(ctx, kSplineTangent, {load<u32>(o + 84), o + 176, o + 112, o + 176, 0}, {zero});
        tz = Lf(o + 184);
        ty = Lf(o + 180);
        tx = Lf(o + 176);
    } else {
        callf(ctx, kSplineTangent, {load<u32>(o + 84), o + 176, o + 112, o + 176, 0}, {C(16)});
        tx = -Lf(o + 176);
        ty = -Lf(o + 180);
        tz = -Lf(o + 184);
        Sf(o + 176, tx);
        Sf(o + 180, ty);
        Sf(o + 184, tz);
    }
    const f64 one = C(16);
    const u32 up = b25fn::UpVec;
    const f64 ux = Lf(up), uy = Lf(up + 4), uz = Lf(up + 8);
    f64 rx = Fmsubs(uy, tz, S(uz * ty));
    f64 ry = Fmsubs(uz, tx, S(ux * tz));
    f64 rz = Fmsubs(ux, ty, S(uy * tx));
    const f64 lr = Sqrts(Fmadds(rz, rz, Fmadds(rx, rx, S(ry * ry))));
    f64 bx = -tx, by = -ty, bz = -tz;
    const f64 py = Lf(o + 116);
    const u32 wpx = load<u32>(o + 112), wpz = load<u32>(o + 120);
    store<u32>(pos, wpx);
    store<u32>(pos + 8, wpz);
    Sf(back, bx);
    Sf(back + 4, by);
    Sf(back + 8, bz);
    const f64 ir = Divs(one, lr);
    rz = MulsA(ir, rz);
    ry = MulsA(ir, ry);
    rx = MulsA(ir, rx);
    const f64 vy = Fmsubs(tz, rx, S(tx * rz));
    const f64 vx = Fmsubs(ty, rz, S(tz * ry));
    const f64 vz = Fmsubs(tx, ry, S(ty * rx));
    Sf(right, rx);
    Sf(right + 4, ry);
    Sf(right + 8, rz);
    const f64 lv = Sqrts(Fmadds(vz, vz, Fmadds(vx, vx, S(vy * vy))));
    const f64 iv = Divs(one, lv);
    const f64 pyy = Subs(py, one);
    Sf(pos + 4, pyy);
    const f64 ox = MulsA(iv, vx), oy = MulsA(iv, vy), oz = MulsA(iv, vz);
    Sf(up2, ox);
    Sf(up2 + 4, oy);
    Sf(up2 + 8, oz);
    if (load<u8>(o + 196) != 0) {
        bx = -bx, by = -by, bz = -bz, rx = -rx, ry = -ry, rz = -rz;
        Sf(back, bx);
        Sf(back + 4, by);
        Sf(back + 8, bz);
        Sf(right, rx);
        Sf(right + 4, ry);
        Sf(right + 8, rz);
    }

    // The spawn desc.
    store<u32>(desc + 16, 0);
    store<u32>(desc, kDescVt);
    Sf(desc + 32, bx);
    Sf(desc + 36, by);
    Sf(desc + 40, bz);
    Sf(desc + 48, ox);
    Sf(desc + 52, oy);
    Sf(desc + 56, oz);
    Sf(desc + 64, rx);
    Sf(desc + 68, ry);
    Sf(desc + 72, rz);
    store<u32>(desc + 80, wpx);
    Sf(desc + 84, pyy);
    store<u32>(desc + 88, wpz);
    Cw(desc + 96, kVec_01164B30);
    Cw(desc + 100, kVec_01164B30 + 4);
    Cw(desc + 104, kVec_01164B30 + 8);
    store<u32>(desc + 112, 0);
    store<u32>(desc + 116, 0);
    store<u32>(desc + 120, def);
    {
        // +132: the top bit set, bit 0x40000000 clear, 0x20000000 = bit 2 of
        // the def's +32, 0x1FE00000 set; the low 21 bits stay as the stack had them.
        u32 w = load<u32>(desc + 132) | 0x80000000u;
        Cw(desc + 124, def + 36);
        w &= ~0x40000000u;
        Cw(desc + 128, def + 40);
        store<u32>(desc + 132, w);
        const u32 f = (load<u8>(def + 32) >> 2) & 1;
        store<u32>(desc + 136, 0);
        store<u32>(desc + 4, 0);
        w = (w & ~0x20000000u) | (f << 29);
        store<u32>(desc + 8, 0);
        w |= 0x1FE00000u;
        store<u32>(desc + 12, 0);
        store<u8>(desc + 140, 0);
        store<u32>(desc + 132, w);
    }
    call(ctx, kStringCopy, {desc + 140, kTargetStr, 40});
    {
        u32 recs = load<u32>(def + 136);
        if (recs != 0 && load<u16>(recs + 8) != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 192) {
                const u32 cnt = recs != 0 ? load<u16>(recs + 8) : 0;
                if (!(i < cnt)) break;
                const u32 rec = load<u32>(recs) + off;
                u32 v = load<u32>(desc + 16);
                if (v == 0) {
                    v = static_cast<u32>(call(ctx, kSmallAlloc, {20}));
                    store<u32>(desc + 16, v);
                    if (v != 0) {
                        call(ctx, kPtrVectorConstruct, {v});
                        store<u16>(v + 6, 192);
                        v = load<u32>(desc + 16);
                    }
                }
                call(ctx, kVectorAdd, {v, rec});  // v may be 0 after a failed allocation (quirk kept)
                recs = load<u32>(def + 136);
            }
        }
    }
    store<u32>(key3, kProfileKey);
    const u32 pooled = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind, {kPoolMap, key3})));
    u32 d = desc;
    if (pooled != 0) {
        store<u32>(pooled + 112, load<u32>(desc + 112));
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) Cw(pooled + 32 + 16 * r + 4 * k, desc + 32 + 16 * r + 4 * k);
        Cw(pooled + 96, desc + 96);
        Cw(pooled + 100, desc + 100);
        store<u32>(pooled + 120, def);
        Cw(pooled + 104, desc + 104);
        ctx.fpr[1] = Lf(desc + 104);  // the copy goes through f1
        d = pooled;
    }
    const u32 tgt = static_cast<u32>(VCall(ctx, def, 28, {def, d}));
    store<u32>(o + 80, tgt);
    if (tgt != 0) {
        VCall(ctx, tgt, 88, {tgt, 1, 1, 1, 1});
        const u32 t2 = load<u32>(o + 80);
        VCall(ctx, t2, 84, {t2, back});
    }
    const u32 v = load<u32>(desc + 16);
    if (v == 0) return;  // r3: what the last call left
    store<u32>(desc, kDescVt);
    call(ctx, kPtrVectorClear, {v});
    call(ctx, kNop, {v});
    call(ctx, kSmallFree, {load<u32>(desc + 16), 20});
}
