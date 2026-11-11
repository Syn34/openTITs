// Game code, VMX sweep part 6 (see vmx6.h): object physics bodies and
// broadphase queries.
#include "../sly4.h"
#include "vmx6.h"

using namespace sly4;
using namespace sly4::vmx6;

namespace {
constexpr u32 kMasks1C3EF0 = 0x001C3EC0;  // lane masks {y}, {z}, {w}
constexpr u32 kIdentity1C3EF0 = 0x01164BD0;  // f32[4][4] the identity matrix
}  // namespace

// void f(Obj* o, u8 on) (called by Obj_Set_001C4280, Obj_RangeSwitch_001C4390,
// Obj_RangeGate_001C44A8 / 001C4578): switches o's physics on or off. With
// d = o->+216 set: when d->+447 is set, for each body of o's list +576 (u16
// count +584; both re-read each step): on, and not in a world (+8): added
// to the physics world (World + 0x1281D4, its +4) and given the 4x3 rows of
// Obj_PoseOrDefault_000E7CE8(o, &m, (d->+420)[i], identity) (m starts as
// the identity's rows; the rows rebuilt with the pool's lane masks); off,
// and in a world: Hk_RemoveEntity92FFC8(&out, world, body). Then each part
// of o's list +556 (u16 count +564) gets its vfunc +40(part, on, 1).
// Returns o without d, else the part count (r3 as the original leaves it);
// f1 is the identity's +56 once a body has been posed.
DECOMP(001C3EF0, Obj_EnableBodies_001C3EF0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFF;
    Stack frame(ctx, 416);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 d = load<u32>(o + 216);
    if (d == 0) {
        ctx.gpr[3] = o;
        return;
    }
    if (load<u8>(d + 447) != 0) {
        const Masks3 mk = LoadMasks3(kMasks1C3EF0);
        const u32 I = kIdentity1C3EF0, m = sp + 176;
        for (u32 i = 0; i < load<u16>(o + 584); ++i) {
            const u32 body = load<u32>(load<u32>(o + 576) + 4 * i);
            if (body == 0) continue;
            if (on != 0) {
                if (load<u32>(body + 8) != 0) continue;
                const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x1281D4u) + 4);
                call(ctx, 0x0092F2A8, {world, body, 1});  // Hk_AddEntity92F2A8
                for (u32 r = 0; r < 4; ++r) CopyW6(m + 16 * r, I + 16 * r, 3);
                ctx.fpr[1] = Lfs4(I + 56);  // the last lfs of the copies
                call(ctx, 0x000E7CE8, {o, m, load<u32>(load<u32>(d + 420) + 4 * i), I});  // Obj_PoseOrDefault_000E7CE8
                for (u32 r = 0; r < 4; ++r) Stvx(sp + 112 + 16 * r, RowSelW(m + 16 * r, m + 16 * r + 4, m + 16 * r + 8, mk));
                call(ctx, 0x0091EAB8, {body, sp + 112});  // Hk_SetTransform91EAB8
            } else {
                const u32 world = load<u32>(body + 8);
                if (world != 0) call(ctx, 0x0092FFC8, {sp + 240, world, body});  // Hk_RemoveEntity92FFC8
            }
        }
    }
    u32 n = 0;
    for (u32 j = 0;; ++j) {
        n = load<u16>(o + 564);
        if (j >= n) break;
        const u32 part = load<u32>(load<u32>(o + 556) + 4 * j);
        if (part != 0) VCall(ctx, part, 40, {part, on, 1});
    }
    ctx.gpr[3] = n;
}

namespace {
constexpr u32 kMasks49060 = 0x00049030;  // lane masks {y}, {z}, {w}
constexpr u32 kPropKey49060 = 0x91C7CF5Bu;  // the entity property holding the game object
constexpr u32 kHkAllocator49060 = 0x011DF31C;  // the hkArray allocator object
}  // namespace

// u32 f(Region* q, const f32 lo[3], const f32 hi[3], u8 mode) (called by
// 0004941C and six more): the game objects of the Havok entities whose
// broadphase handles overlap the box lo..hi (vfunc +76 of the world's
// broadphase +88 into a stack hkArray {data, size, 0x80000000}). Each
// entity owner (collidable + s8 +5, an entity when its s8 +24 is 1, then
// + s8 +16) with property 0x91C7CF5B = obj, obj->+192 & 0x20: obj->+360 =
// 0; mode 0: obj's vfunc +128(obj, 0x806E6EA2, 1) when obj->+376 &
// 0x10000000, else vfunc +128(obj, 0x879F2BDA, 1) and obj->+176 |= 0x1000;
// mode != 0: unless obj's vfunc +324() or obj->+376 & 0x10000, and while
// q->+1816 + 1 < 128: obj appended to q's array +1820, obj->+178 += 1,
// obj->+376 |= 0x10000. The array's buffer is freed (allocator vfunc +20)
// unless its capacity word has bit 31. Returns that word or what the free
// leaves.
DECOMP(00049060, Region_QueryEntities_00049060) {
    const u32 q = arg<u32>(ctx, 0), lo = arg<u32>(ctx, 1), hi = arg<u32>(ctx, 2);
    const u32 mode = arg<u32>(ctx, 3) & 0xFF;
    Stack frame(ctx, 208);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const Masks3 mk = LoadMasks3(kMasks49060);
    Stvx(sp + 112, RowSelW(lo, lo + 4, lo + 8, mk));
    Stvx(sp + 128, RowSelW(hi, hi + 4, hi + 8, mk));
    const u32 bp = load<u32>(load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x1281D4u) + 4) + 88);
    const u32 arr = sp + 144;
    store<u32>(arr, 0);
    store<u32>(arr + 4, 0);
    store<u32>(arr + 8, 0x80000000u);
    VCall(ctx, bp, 76, {bp, sp + 112, arr});
    for (s32 k = 0; k < load<s32>(arr + 4); ++k) {
        const u32 col = load<u32>(load<u32>(arr) + 8u * static_cast<u32>(k) + 4);
        const u32 own = col + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(col + 5))));
        if (static_cast<s8>(load<u8>(own + 24)) != 1) continue;
        const u32 ent = own + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(own + 16))));
        if (ent == 0) continue;
        const s32 pi = HkPropIndex(ent, kPropKey49060);
        const u32 obj = pi < 0 ? 0 : static_cast<u32>(HkPropValue(ent, pi));
        if (obj == 0 || (load<u32>(obj + 192) & 0x20u) == 0) continue;
        store<u32>(obj + 360, 0);
        if (mode == 0) {
            if ((load<u32>(obj + 376) & 0x10000000u) != 0) {
                VCall(ctx, obj, 128, {obj, 0x806E6EA2u, 1});
            } else {
                VCall(ctx, obj, 128, {obj, 0x879F2BDAu, 1});
                store<u16>(obj + 176, static_cast<u16>(load<u16>(obj + 176) | 0x1000u));
            }
            continue;
        }
        if (static_cast<u32>(VCall(ctx, obj, 324, {obj})) != 0) continue;
        if ((load<u32>(obj + 376) & 0x10000u) != 0) continue;
        const s32 n = load<s32>(q + 1816);
        if (static_cast<s32>(static_cast<u32>(n) + 1) >= 128) continue;  // 32-bit wrap, signed compare
        const u32 n2 = load<u32>(q + 1816);  // re-read before the store (it may overwrite the count)
        store<u32>(q + 1820 + (static_cast<u32>(n) << 2), obj);
        const u32 e = load<u32>(q + 1820 + (n2 << 2));
        store<u16>(e + 178, static_cast<u16>(load<u16>(e + 178) + 1));
        store<u32>(e + 376, load<u32>(e + 376) | 0x10000u);
        store<u32>(q + 1816, load<u32>(q + 1816) + 1);
    }
    const u32 cap = load<u32>(arr + 8);
    if ((cap & 0x80000000u) != 0) {
        ctx.gpr[3] = cap;
        return;
    }
    store<u32>(arr + 4, 0);
    const u64 bytes = static_cast<u64>(static_cast<s64>(static_cast<s32>(cap << 3)));
    VCall(ctx, kHkAllocator49060, 20, {kHkAllocator49060, load<u32>(arr), bytes});
}

namespace {
constexpr u32 kPool110E08 = 0x00110DC0;  // f32 {0.001, 0.01, 0.0}, masks {y} (+16), {z} (+32), {w} (+48), f32 0.1 (+64)
constexpr u32 kPad110E08 = 0x010B19E0;  // 11968-byte pad records (+176 the channels)
}  // namespace

// u32 f(Obj* o, ?, Mat* out; f32 dt) (called through a pointer): drives a
// Havok body from o's animated matrix. With o->+592 (the player) set:
// bit 0x40000000 of o->+484 = bit 0 of f_001104B8(o, ?, out; dt); o->+384
// = out's translation; the velocity v = (out+48 - o+560) / dt, zero when dt
// < 0.001 or every |delta| < 0.01; when the player's doubleword +184 has
// bit 0 and the bit above is clear and the pad's channel 2 equals channel
// 24 and channel 3 equals channel 24, v is eased from o+576 instead
// (Curve_Lerp3(&r, o+576, &v, 0.1, dt)); o+576 = v; Hk_Activate on the body
// of o->+600 and its motion's (+224) vfunc +68(motion, {v, 0}) (the
// velocity); then out's 4x3 rows = o+512... Returns that vfunc's r3 (0
// without a player); f1 is the last copied word (dt without a player).
DECOMP(00110E08, Obj_KinematicVelocity_00110E08) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    const u64 a1 = ctx.gpr[4];
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 224);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    if (load<u32>(o + 592) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    callf(ctx, 0x001104B8, {o, a1, out}, {dt});
    const u32 r = static_cast<u32>(ctx.gpr[3]);
    store<u32>(o + 484, (load<u32>(o + 484) & ~0x40000000u) | ((r & 1u) << 30));
    CopyW6(o + 384, out + 48, 3);
    const f32 dx = Subs(Lfs4(out + 48), Lfs4(o + 560));
    const f32 dy = Subs(Lfs4(out + 52), Lfs4(o + 564));
    const f32 dz = Subs(Lfs4(out + 56), Lfs4(o + 568));
    u32 vx, vy, vz;
    const f64 eps = Lfs4(kPool110E08 + 4);
    if (dt < Lfs4(kPool110E08) || (std::fabs(dx) < eps && std::fabs(dy) < eps && std::fabs(dz) < eps)) {
        vx = load<u32>(kV3Zero3);
        vy = load<u32>(kV3Zero3 + 4);
        vz = load<u32>(kV3Zero3 + 8);
    } else {
        vx = std::bit_cast<u32>(Divs(dx, dt));
        vy = std::bit_cast<u32>(Divs(dy, dt));
        vz = std::bit_cast<u32>(Divs(dz, dt));
    }
    store<u32>(sp + 128, vx);
    store<u32>(sp + 132, vy);
    store<u32>(sp + 136, vz);
    store<u32>(o + 576, vx);
    store<u32>(o + 580, vy);
    store<u32>(o + 584, vz);
    const u32 pl = load<u32>(o + 592);
    if ((load<u64>(pl + 184) & 1) != 0) {
        const s32 slot = static_cast<s8>(load<u8>(pl + 172));
        const u32 rec = load<u32>(kV3WorldPtr) + static_cast<u32>(slot) * 0x20C80u + 4208;
        const u32 pad = kPad110E08 + load<u8>(rec + 1) * 11968u + 176;
        const f64 c2 = callf(ctx, v4fn::Obj_GetChannel, {pad, 2}, {});
        const f64 c24 = callf(ctx, v4fn::Obj_GetChannel, {pad, 24}, {});
        const f64 c3 = callf(ctx, v4fn::Obj_GetChannel, {pad, 3}, {});
        const f64 c24b = callf(ctx, v4fn::Obj_GetChannel, {pad, 24}, {});
        const f64 zero = Lfs4(kPool110E08 + 8);
        if ((load<u32>(o + 484) & 0x40000000u) == 0 && static_cast<f64>(Subs(c2, c24)) == zero &&
            static_cast<f64>(Subs(c3, c24b)) == zero) {
            callf(ctx, 0x004A8598, {sp + 144, o + 576, sp + 128}, {Lfs4(kPool110E08 + 64), dt});  // Curve_Lerp3_004A8598
            CopyW6(o + 576, sp + 144, 3);
        } else {
            CopyW6(o + 576, sp + 128, 3);
        }
    }
    const u32 body = static_cast<u32>(call(ctx, 0x009D19C8, {load<u32>(o + 600)}));  // Hk_GetBody9D19C8
    const Masks3 mk = LoadMasks3(kPool110E08 + 16);
    Stvx(sp + 112, RowSelW(o + 576, o + 580, o + 584, mk));
    call(ctx, 0x0091A4C8, {body});  // Hk_Activate91A4C8
    VCall(ctx, body + 224, 68, {body + 224, sp + 112});
    for (u32 k = 0; k < 4; ++k) CopyW6(out + 16 * k, o + 512 + 16 * k, 3);
    ctx.fpr[1] = Lfs4(o + 568);
}

namespace {
constexpr u32 kMasks243A80 = 0x00243A50;  // lane masks {y}, {z}, {w}
}  // namespace

// u32 f(Obj* o, const Mat* m) (called through a pointer): moves o to the
// 4x4 matrix m. o->+80 += m's translation - o's old translation (+144);
// o's 4x3 rows (+96..+152) = m's; with a body o->+396: Hk_SetTransform91EAB8
// (body, m's rows rebuilt with the lane masks), then (o->+396 re-read
// each time) Hk_Activate and the motion's (+224) vfuncs +68 and +72 with
// the zero vector (linear / angular velocity). Then +448 / +480 / +496 =
// the zero vector, +464 / +512..+524 = 0. r3 / f1 are what the final
// copies leave (0x01160000 and the zero vector's z).
DECOMP(00243A80, Obj_TeleportBody_00243A80) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    Stack frame(ctx, 304);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const f32 dx = Subs(Lfs4(m + 48), Lfs4(o + 144));
    const f32 dy = Subs(Lfs4(m + 52), Lfs4(o + 148));
    const f32 dz = Subs(Lfs4(m + 56), Lfs4(o + 152));
    store<f32>(o + 80, Adds(Lfs4(o + 80), dx));
    store<f32>(o + 84, Adds(Lfs4(o + 84), dy));
    store<f32>(o + 88, Adds(Lfs4(o + 88), dz));
    for (u32 r = 0; r < 4; ++r) CopyW6(o + 96 + 16 * r, m + 16 * r, 3);
    const u32 body = load<u32>(o + 396);
    if (body != 0) {
        const Masks3 mk = LoadMasks3(kMasks243A80);
        for (u32 r = 0; r < 4; ++r) Stvx(sp + 128 + 16 * r, RowSelW(m + 16 * r, m + 16 * r + 4, m + 16 * r + 8, mk));
        call(ctx, 0x0091EAB8, {body, sp + 128});  // Hk_SetTransform91EAB8
        for (u32 slot = 68; slot <= 72; slot += 4) {
            const u32 b = load<u32>(o + 396);
            Stvx(sp + 112, RowSelW(kV3Zero3, kV3Zero3 + 4, kV3Zero3 + 8, mk));
            call(ctx, 0x0091A4C8, {b});  // Hk_Activate91A4C8
            VCall(ctx, b + 224, slot, {b + 224, sp + 112});
        }
    }
    CopyW6(o + 448, kV3Zero3, 2);
    store<u32>(o + 464, 0);
    store<u32>(o + 456, load<u32>(kV3Zero3 + 8));
    CopyW6(o + 480, kV3Zero3, 3);
    CopyW6(o + 496, kV3Zero3, 2);
    store<u32>(o + 512, 0);
    store<u32>(o + 504, load<u32>(kV3Zero3 + 8));
    store<u32>(o + 516, 0);
    store<u32>(o + 520, 0);
    store<u32>(o + 524, 0);
    ctx.gpr[3] = 0x01160000u;
    ctx.gpr[4] = kV3Zero3;  // r4 / r5 as the original leaves them
    ctx.gpr[5] = 0;
    ctx.fpr[1] = Lfs4(kV3Zero3 + 8);
}

namespace {
constexpr u32 kPool23B00 = 0x00023AC0;  // f32 {0.0, 0.01, 1e-5}, masks {y} (+16), {z} (+32), {w} (+48)
constexpr u32 kTab23B00 = 0x00F9485C;  // (->+16)->+64: the index into the 4-byte flag table (+308)->+12
}  // namespace

// u32 f(Mover* t; f32 dt) (called through a pointer): per-frame update of
// an object mover. Nothing when the owner's (+48) word +172 has 0x40 or
// 0x20. The timer +316 (when positive) counts down by dt and bit 0x80 of
// the flags byte +304 is set once it reaches 0; with the global flag-table
// entry set, flag 0x04 and kind (+4) != 14: vfunc +260(t). Then
// f_000131C8(t, dt); with flag 0x10 the position +176 follows
// f_006C1CC0(&p, t->+212) (moves of more than 0.1 also clear 0x80 and set
// the timer to 1.0). Unless t->+68 is set and GetFlag_002697F8(it): the
// owner's body (vfunc +144) that is in a world (+8) and more than 1e-5
// (per axis, against body+288 + the zero vector) from +176 gets the rows
// +128..+176 (Hk_SetTransform91EAB8) and Body_Moved_003556AC. r3 / f1 are
// left as the original leaves them.
DECOMP(00023B00, Mover_Update_00023B00) {
    const u32 t = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 240);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 w = load<u32>(load<u32>(t + 48) + 172);
    if ((w & 0x40u) != 0) {
        ctx.gpr[3] = w;
        return;
    }
    if ((w & 0x20u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const f64 zero = Lfs4(kPool23B00);
    u32 fl = load<u8>(t + 304);
    const f64 tm = Lfs4(t + 316);
    if (tm > zero) {
        const f32 nt = Subs(tm, dt);
        store<f32>(t + 316, nt);
        fl = (fl & ~0x80u) | ((nt <= zero ? 1u : 0u) << 7);
        store<u8>(t + 304, static_cast<u8>(fl));
    }
    ctx.fpr[1] = zero;  // f1 as the original leaves it (the lfs of 0.0)
    const u32 idx = load<u32>(load<u32>(kTab23B00 + 16) + 64);
    if (load<u8>(load<u32>(load<u32>(t + 308) + 12) + (idx << 2)) != 0 && (fl & 0x04u) != 0 && load<u32>(t + 4) != 14)
        VCall(ctx, t, 260, {t});
    callf(ctx, 0x000131C8, {t}, {dt});
    if ((load<u8>(t + 304) & 0x10u) != 0) {
        const u32 p = t + 176;
        const f64 x0 = Lfs4(p), y0 = Lfs4(p + 4), z0 = Lfs4(p + 8);
        call(ctx, 0x006C1CC0, {sp + 176, load<u32>(t + 212)});
        const f32 dy = Subs(Lfs4(sp + 180), y0), dx = Subs(Lfs4(sp + 176), x0), dz = Subs(Lfs4(sp + 184), z0);
        const f32 d2 = Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)));
        if (d2 > Lfs4(kPool23B00 + 4)) {
            const u8 b = load<u8>(t + 304);
            store<u32>(t + 316, 0x3F800000u);
            store<u8>(t + 304, static_cast<u8>(b & ~0x80u));
        }
        CopyW6(p, sp + 176, 3);
        ctx.fpr[1] = Lfs4(sp + 184);
    }
    const u32 g = load<u32>(t + 68);
    if (g != 0 && static_cast<u32>(call(ctx, 0x002697F8, {g})) != 0) return;  // GetFlag_002697F8
    const u32 own = load<u32>(t + 48);
    const u32 body = static_cast<u32>(VCall(ctx, own, 144, {own}));
    if (body == 0 || load<u32>(body + 8) == 0) {
        ctx.gpr[3] = body == 0 ? 0 : load<u32>(body + 8);
        return;
    }
    const f64 eps = Lfs4(kPool23B00 + 8);
    ctx.fpr[1] = eps;
    const f32 ex = Subs(Subs(Lfs4(t + 176), Lfs4(body + 288)), Lfs4(kV3Zero3));
    if (std::fabs(ex) < eps) {
        const f32 ey = Subs(Subs(Lfs4(t + 180), Lfs4(body + 292)), Lfs4(kV3Zero3 + 4));
        if (std::fabs(ey) < eps) {
            const f32 ez = Subs(Subs(Lfs4(t + 184), Lfs4(body + 296)), Lfs4(kV3Zero3 + 8));
            if (std::fabs(ez) < eps) {
                ctx.gpr[3] = kV3Zero3;
                return;
            }
        }
    }
    const Masks3 mk = LoadMasks3(kPool23B00 + 16);
    for (u32 r = 0; r < 4; ++r) Stvx(sp + 112 + 16 * r, RowSelW(t + 128 + 16 * r, t + 132 + 16 * r, t + 136 + 16 * r, mk));
    call(ctx, 0x0091EAB8, {body, sp + 112});  // Hk_SetTransform91EAB8
    call(ctx, 0x003556AC, {body});  // Body_Moved_003556AC
}

namespace {
constexpr u32 kPool4AC210 = 0x004AC1C0;  // f32 1.0, f64 1.0 (+8), f64 0.0 (+16), masks {y} (+32), {z} (+48), {w} (+64)
}  // namespace

// u32 f(Force* t) (called by 004AC5D8): applies t's two 4-float vectors
// (+16, +32) to its body t->+48. A cinfo filled by Hk_CinfoFromBody91D208
// goes to f_004ABC28(t, its shape (+4), body + 240); the vectors are scaled
// in place by def->+144 * (def->+32)->+96 (def = t->+0) times, when def's
// u8 +156, its object +28 with flag 0x40000 of +172 and bit 0 of the
// doubleword +184, and that object's vfunc +64() are all set, 1 - def->+112
// * (1 - clamp01(def->+152)) (fsel / frsp); each scaled vector is rebuilt
// from splats with the pool's lane masks. Then (body re-read each time)
// Hk_Activate and the motion's (+224) vfuncs +96(motion, -, t+16; f1 =
// t->+52) and +104(motion, -, t+32; f1 = t->+52), the OPD in r4. Returns
// the last vfunc's r3.
DECOMP(004AC210, Force_Apply_004AC210) {
    const u32 t = arg<u32>(ctx, 0);
    Stack frame(ctx, 464);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 cinfo = sp + 112;
    call(ctx, 0x0091FB50, {cinfo});  // Hk_ShapeCinfoCtor91FB50
    call(ctx, 0x0091D208, {load<u32>(t + 48), cinfo});  // Hk_CinfoFromBody91D208
    call(ctx, 0x004ABC28, {t, load<u32>(cinfo + 4), load<u32>(t + 48) + 240});
    const u32 def = load<u32>(t);
    f64 v[8];
    for (u32 k = 0; k < 8; ++k) v[k] = Lfs4(t + 16 + 4 * k);
    const f64 one = Lfs4(kPool4AC210);
    const f32 base = Muls(Lfs4(load<u32>(def + 32) + 96), Lfs4(def + 144));
    f64 k = one;
    const u32 ob = load<u32>(def + 28);
    if (load<u8>(def + 156) != 0 && ob != 0 && (load<u32>(ob + 172) & 0x40000u) != 0 && (load<u64>(ob + 184) & 1) != 0 &&
        static_cast<u32>(VCall(ctx, ob, 64, {ob})) != 0) {
        const f64 d = Lfs4(def + 152);
        f64 c = Frsp(Fsel(Subs(one, d), d, Lfd(kPool4AC210 + 8)));
        c = Frsp(Fsel(c, c, Lfd(kPool4AC210 + 16)));
        k = Fnmsubs(Lfs4(def + 112), Subs(one, c), one);
    }
    const f32 s = Muls(base, k);
    f32 w[8];
    for (u32 j = 0; j < 8; ++j) w[j] = Muls(v[j], s);
    const V4 my = Lvx(kPool4AC210 + 32), mz = Lvx(kPool4AC210 + 48), mw = Lvx(kPool4AC210 + 64);
    Stvx(t + 16, Sel(Sel(Sel(Fill4(w[0]), Fill4(w[1]), my), Fill4(w[2]), mz), Fill4(w[3]), mw));
    const u32 body = load<u32>(t + 48);
    const f64 a = Lfs4(t + 52);
    Stvx(t + 32, Sel(Sel(Sel(Fill4(w[4]), Fill4(w[5]), my), Fill4(w[6]), mz), Fill4(w[7]), mw));
    call(ctx, 0x0091A4C8, {body});  // Hk_Activate91A4C8
    {
        const u32 m = body + 224, opd = load<u32>(load<u32>(m) + 96);
        ctx.fpr[1] = a;
        call_ptr(ctx, opd, {m, opd, t + 16, load<u32>(opd)});
    }
    const u32 body2 = load<u32>(t + 48);
    const f64 a2 = Lfs4(t + 52);
    call(ctx, 0x0091A4C8, {body2});
    const u32 m = body2 + 224, opd = load<u32>(load<u32>(m) + 104);
    ctx.fpr[1] = a2;
    call_ptr(ctx, opd, {m, opd, t + 32, load<u32>(opd)});
}

namespace {
constexpr u32 kPool354C18 = 0x00354BD0;  // u32 {8, 4, 2, 1}, a word-reversing vperm mask, f32 1.0 (+32), f64 1.0 (+40),
                                         // f32 -1.0 (+48), f64 -1.0 (+56), f32 pi / 180 (+64)
constexpr u32 kPhantom354C18 = 0x0106BFA0;  // the walkable-area hkpAabbPhantom* (or 0)
constexpr u32 kMaxSlope354C18 = 0x0106BF48;  // f32 the steepest walkable slope in degrees
constexpr u32 kCollectorVt354C18 = 0x00B1F5B8;  // the closest-hit ray collector's vtable

// The vcmpgefp / vand / vperm / vor / vcmpequw. test of the original: lanes
// 0..2 of p within [lo, hi] (NaN fails; lane 3 is forced true by the
// pool's {8, 4, 2, 1} & 7 compare).
bool Inside354C18(const V4& p, const V4& lo, const V4& hi) {
    for (u32 k = 0; k < 3; ++k)
        if (!(p.e[k] >= lo.e[k] && hi.e[k] >= p.e[k])) return false;
    return true;
}

// acosf of the clamped (fsel / frsp) word at ea below the walkable slope.
bool Walkable354C18(PPUContext& ctx, u32 ea) {
    const f64 one = Lfs4(kPool354C18 + 32), dOne = Lfd(kPool354C18 + 40), mOne = Lfs4(kPool354C18 + 48), dMOne = Lfd(kPool354C18 + 56);
    f64 c = Lfs4(ea);
    c = Frsp(Fsel(Subs(one, c), c, dOne));
    c = Frsp(Fsel(Subs(c, mOne), c, dMOne));
    c = Frsp(Fsel(Adds(c, one), c, dMOne));
    c = Frsp(Fsel(Subs(c, one), dOne, c));
    const f64 a = callf(ctx, v4fn::acosf, {}, {c});
    return a < static_cast<f64>(Muls(Lfs4(kMaxSlope354C18), Lfs4(kPool354C18 + 64)));
}
}  // namespace

// struct {u8 hit} f(ret*, ?, const Vec4* from, const Vec4* to, f32* frac,
// Vec4* normal) (called through a pointer): a Havok ray from -> to (filter
// 13) that must land on a walkable surface (acos of the normal's y below
// the slope limit 0x0106BF48 degrees). With the walkable phantom
// 0x0106BFA0 in the current physics world: both ends must lie inside the
// phantom's AABB (vfunc +32) and the ray is cast against the phantom
// (closest-hit collector, vtable 0x00B1F5B8, the world's +116); else
// against the world (HkWorld_CastRay92E2E8). On success *frac / *normal
// get the hit fraction / normal. Writes and returns the hit byte (f1 is
// what the original leaves).
DECOMP(00354C18, Ray_CastWalkable_00354C18) {
    const u32 out = arg<u32>(ctx, 0), from = arg<u32>(ctx, 2), to = arg<u32>(ctx, 3), frac = arg<u32>(ctx, 4), nrm = arg<u32>(ctx, 5);
    Stack frame(ctx, 368);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 in = sp + 144;  // hkpWorldRayCastInput
    store<u8>(in + 32, 0);
    store<u32>(in + 40, 0);
    Stvx(in, Lvx(from));
    Stvx(in + 16, Lvx(to));
    store<u32>(in + 36, 13);
    const u32 world = load<u32>(load<u32>(load<u32>(kV3WorldPtr) + 0x1281D4u) + 4);
    const u32 ph = load<u32>(kPhantom354C18);
    u32 hit = 0;
    if (ph != 0 && load<u32>(ph + 8) == world) {
        VCall(ctx, ph, 32, {ph, sp + 112});  // the phantom's AABB
        const V4 lo = Lvx(sp + 112), hi = Lvx(sp + 128);
        if (Inside354C18(Lvx(in), lo, hi) && Inside354C18(Lvx(in + 16), lo, hi)) {
            const u32 col = sp + 192;
            store<u32>(col + 4, 0x3F800000u);
            store<u32>(col, kCollectorVt354C18);
            store<u32>(col + 32, load<u32>(world + 116));
            store<u32>(col + 36, load<u32>(in + 36));
            store<u32>(col + 40, 0x3F800000u);
            store<u8>(col + 44, 0);
            call(ctx, 0x00925408, {load<u32>(kPhantom354C18), in, col});  // HkPhantom_CastRay925408
            if (load<u8>(col + 44) != 0 && Walkable354C18(ctx, col + 20)) {
                ctx.fpr[1] = Lfs4(col + 40);
                store<u32>(frac, load<u32>(col + 40));
                Stvx(nrm, Lvx(col + 16));
                hit = 1;
            }
        }
    } else {
        const u32 res = sp + 240;  // hkpWorldRayCastOutput
        store<u32>(res + 16, 0x3F800000u);
        store<u32>(res + 20, 0xFFFFFFFFu);
        store<u32>(res + 64, 0);
        store<u32>(res + 32, 0xFFFFFFFFu);
        store<u32>(res + 68, 0);
        call(ctx, v6fn::HkWorld_CastRay92E2E8, {world, in, res});
        if (load<u32>(res + 68) != 0 && Walkable354C18(ctx, res + 4)) {
            ctx.fpr[1] = Lfs4(res + 16);
            store<u32>(frac, load<u32>(res + 16));
            Stvx(nrm, Lvx(res));
            hit = 1;
        }
    }
    store<u8>(out, static_cast<u8>(hit));
    ctx.gpr[3] = hit;
}

namespace {
constexpr u32 kPool4AB2B0 = 0x004AB280;  // vperm masks yzx / zxy, f32 0.5 (+32), 1/3 (+36), 0.0 (+40)
constexpr u32 kProfTri4AB2B0 = 0x00AEDCA8;  // "TtTriangle" (profiler marker)
constexpr u32 kProfEnd4AB2B0 = 0x00B21B48;  // the closing profiler marker
}  // namespace

// u32 f(Floater* t, const Tri* tri, const hkTransform* xf) (called by
// 004ABC28): one submerged triangle's force on a floating body. Profiler
// marker "TtTriangle"; the triangle (+32, +48, +64) gives the unit normal
// n (cross product of the edges, rsqrte + one Newton step, 0 for a
// degenerate one) with area |e1 x e2| / 2, n rotated by f_007D16D0(&n, xf,
// &n) and the centroid taken by HkTransform_InvPos7D170C(&c, xf, &c). The
// medium velocity at c (vfunc +16 of t->+60, else 0) gets, when the
// depth +64 is positive, (body linear velocity + angular x (c - com)) *
// -depth added (body = t->+48: motion +224, com +320, angular +448, linear
// +432); then F = n * area * dot(v, n) is added to t's force +16 and
// (c - com) x F to its torque +32 (all four lanes). Closing marker. r3 is
// the profiler block, f1 the (negated when positive) depth.
DECOMP(004AB2B0, Floater_TriangleForce_004AB2B0) {
    const u32 t = arg<u32>(ctx, 0), tri = arg<u32>(ctx, 1), xf = arg<u32>(ctx, 2);
    Stack frame(ctx, 304);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    ProfMark(ctx, kProfTri4AB2B0);
    const V4 z = Zero4(), A = Lvx(kPool4AB2B0), B = Lvx(kPool4AB2B0 + 16);
    const V4 p0 = Lvx(tri + 32);
    const V4 e1 = Sub4(Lvx(tri + 48), p0), e2 = Sub4(Lvx(tri + 64), p0);
    const V4 n = Cross4(e1, e2, A, B);
    const V4 half = SplatW(kPool4AB2B0 + 32);
    const V4 l2 = HSum3(Mul0(n, n));
    const V4 e = Rsqrte4(l2);
    const V4 r = Andc(Madd4(e, Nmsub4(Mul0(l2, half), Mul0(e, e), half), e), Cmpge4(z, l2));
    Stvx(sp + 160, Mul0(l2, r));
    Stvx(sp + 112, Mul0(n, r));
    const f32 area = Muls(Lfs4(sp + 160), Lfs4(kPool4AB2B0 + 32));
    call(ctx, 0x007D16D0, {sp + 112, xf, sp + 112});
    Stvx(sp + 128, Add4(Mul0(Add4(e1, e2), SplatW(kPool4AB2B0 + 36)), Lvx(tri + 32)));
    call(ctx, 0x007D170C, {sp + 128, xf, sp + 128});  // HkTransform_InvPos7D170C
    const u32 vo = load<u32>(t + 60);
    if (vo != 0) VCall(ctx, vo, 16, {vo, sp + 128, sp + 144});
    else Stvx(sp + 144, z);
    const f64 dep = Lfs4(t + 64);
    f64 f1 = dep;
    if (dep > Lfs4(kPool4AB2B0 + 40)) {
        const u32 m = load<u32>(t + 48) + 224;
        const V4 rr = Sub4(Lvx(sp + 128), Lvx(m + 96));
        f1 = -dep;
        const V4 c = Cross4(Lvx(m + 224), rr, A, B);
        Stvx(sp + 144, Madd4(Add4(c, Lvx(m + 208)), Fill4(static_cast<f32>(f1)), Lvx(sp + 144)));
    }
    const V4 nrm = Lvx(sp + 112);
    const V4 dot = HSum3(Mul0(Lvx(sp + 144), nrm));
    const V4 rr = Sub4(Lvx(sp + 128), Lvx(load<u32>(t + 48) + 320));
    const V4 F = Mul0(nrm, Mul0(Fill4(area), dot));
    const V4 f0 = Lvx(t + 16), q0 = Lvx(t + 32);
    Stvx(t + 16, Add4(f0, F));
    Stvx(t + 32, Add4(q0, Cross4(rr, F, A, B)));
    ProfMark(ctx, kProfEnd4AB2B0);
    ctx.gpr[3] = load<u32>(static_cast<u32>(ctx.gpr[13]) - 28328u);
    ctx.fpr[1] = f1;
}
