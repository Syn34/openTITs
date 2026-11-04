// World_HandleEvent callees, part 4: 105..109 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent4a {
namespace afn {
constexpr u32 EffectDef_Find = 0x004BDEDC;    // (u32 hash, 0, 1) -> def or 0 (name?)
constexpr u32 SmallAlloc = 0x0055238C;        // (size)
constexpr u32 Effect_Construct = 0x004C7A68;  // (mem 140 bytes, def)
constexpr u32 Effect_Start = 0x004C753C;      // (Effect*)
constexpr u32 Effect_SetTime = 0x004CF710;    // (Effect*; f32)
constexpr u32 World_Check48981C = 0x0048981C; // (World*, 1) -> nonzero to go on (name?)
constexpr u32 Handle_ResolveBit20000 = 0x00365B28;  // (handle) (hevent_c.cpp)
constexpr u32 Obj_SetMode470750 = 0x00470750; // (Object*, 3, 0) (name?)
constexpr u32 Player_Set3632C8 = 0x003632C8;  // (Player*, value, u8 which) (name?)
constexpr u32 Obj_Reset3636EC = 0x003636EC;   // (Object*) (name?)
constexpr u32 Obj_SetState362D28 = 0x00362D28;  // (Object*, 2, 0) (name?)
constexpr u32 sub_002E9688 = 0x002E9688;      // (; f32 dt)
constexpr u32 sub_00365F28 = 0x00365F28;      // (; f32 dt)
constexpr u32 sub_00366710 = 0x00366710;      // (; f32 dt)
constexpr u32 Triggers_Clear = 0x000D0628;    // ()
constexpr u32 Obj_Apply5A5EA0 = 0x004F303C;   // (obj, -, 0; f32 dt)
}  // namespace afn

constexpr u32 kEffectHash = 0x9F90E54Fu;
constexpr u32 kVec4_1164C90 = 0x01164C90;     // f32[4] (set at run time) (name?)
constexpr u32 kVec3_1164B30 = 0x01164B30;     // f32[3] (hevent3_b.cpp kOneVec3)
constexpr u32 kEffectTime = 0x00424D70;       // f32 0x3C88CE70 (~1/60)
constexpr u32 kPlayerHandle = 0x00F64D4C;     // u32 handle resolved with Handle_ResolveBit20000
constexpr u32 kZeroF363888 = 0x00363888;      // f32 0.0
constexpr u32 kZeroF47C2B0 = 0x0047C2B0;      // f32 0.0
constexpr u32 kDefaultDt47C2B4 = 0x0047C2B4;  // f32 1/60 (0x3C888889)

// World +0x1281C4: u32 object updated by World_TickTimedCallbacks.
constexpr u32 kWorldObj1281C4 = 0x1281C4;
// World_AddTimedCallback lists (hevent2_a.cpp): {+0 data, +8 u16 count} of
// 12-byte {OPD, arg, -} entries; kind 0 at +0x128480, kind 12 at +0x128570.
constexpr u32 kList0Data = 0x128480, kList0Count = 0x128488;
constexpr u32 kList12Data = 0x128570, kList12Count = 0x128578;
}  // namespace hevent4a
}  // namespace sly4

using namespace sly4::hevent4a;

// u32 FxSlot_Reset(FxSlot* s): +0..+63 = 0, +64 and +80 = the vec4 at
// 0x01164C90, +96 and +112 = the vec3 at 0x01164B30, +128..+160 = 0, +164 bits
// 0x80000000 and 0x40000000 cleared; then an effect of def hash 0x9F90E54F,
// when found, is built in a 140-byte block (+156), has its +136 bit 0x200
// cleared, is started and given time 0x3C88CE70. Quirk: a failed SmallAlloc
// still has +136 of address 0 written. Returns 0 without a def, else what
// Effect_SetTime leaves in r3 (name?)
DECOMP(00424D78, FxSlot_Reset) {
    const u32 s = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 4; ++k) {
        store<u32>(s + 64 + k * 4, load<u32>(kVec4_1164C90 + k * 4));
        store<u32>(s + 80 + k * 4, load<u32>(kVec4_1164C90 + k * 4));
    }
    for (u32 k = 0; k < 3; ++k) {
        store<u32>(s + 96 + k * 4, load<u32>(kVec3_1164B30 + k * 4));
        store<u32>(s + 112 + k * 4, load<u32>(kVec3_1164B30 + k * 4));
    }
    for (u32 off = 128; off <= 160; off += 4) store<u32>(s + off, 0);
    store<u32>(s + 164, load<u32>(s + 164) & 0x3FFFFFFFu);
    for (u32 off = 0; off < 64; off += 4) store<u32>(s + off, 0);  // stvx v2 (0) x4

    const u32 def = static_cast<u32>(call(ctx, afn::EffectDef_Find, {kEffectHash, 0, 1}));
    if (def == 0) return ret(ctx, 0u);
    const u32 mem = static_cast<u32>(call(ctx, afn::SmallAlloc, {140}));
    u32 e = 0;
    if (mem != 0) {
        call(ctx, afn::Effect_Construct, {mem, def});
        e = mem;
    }
    store<u32>(s + 156, e);
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);  // null write when the alloc failed
    call(ctx, afn::Effect_Start, {load<u32>(s + 156)});
    callf(ctx, afn::Effect_SetTime, {load<u32>(s + 156)}, {static_cast<f64>(load<f32>(kEffectTime))});
}

// bool Player_StartAction(Object* o, Object* src, a2, a3, a4, a5, a6, a7, u8 a8,
// u8 a9) [a8, a9 on the stack]: 0 unless World_Check48981C(current world, 1),
// the player (Handle_ResolveBit20000 of 0x00F64D4C) exists, is not in state 2
// or 3 (+160 >> 1), its +24 is 0 or has +164 bit 0x8000 clear, and o +31 is 0.
// Then o +51 = o +30, o +30 = src +172 when src has the low bit of its u64 +184 set
// (else 0), Obj_SetMode470750(o, 3, 0), Player_Set3632C8(player, a2, 0) and
// (player, a3, 1), and the player gets +400 a4, +408 a5, +404 a6, +412 a7,
// +416 src, +420 a8, +421 a9. Returns 1 (name?)
DECOMP(00489A60, Player_StartAction) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u32 a2 = arg<u32>(ctx, 2), a3 = arg<u32>(ctx, 3), a4 = arg<u32>(ctx, 4);
    const u32 a5 = arg<u32>(ctx, 5), a6 = arg<u32>(ctx, 6), a7 = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u8 a8 = static_cast<u8>(load<u32>(sp + 116));  // 9th argument (low word)
    const u8 a9 = static_cast<u8>(load<u32>(sp + 124));  // 10th argument (low word)
    if (static_cast<u32>(call(ctx, afn::World_Check48981C, {load<u32>(kCurrentWorld), 1})) == 0) return ret(ctx, 0u);
    const u32 p = static_cast<u32>(call(ctx, afn::Handle_ResolveBit20000, {load<u32>(kPlayerHandle)}));
    if (p == 0) return ret(ctx, 0u);
    const s32 st = static_cast<s32>(load<u32>(p + 160) >> 1);
    if (st == 2 || st == 3) return ret(ctx, 0u);
    const u32 t = load<u32>(p + 24);
    if (t != 0 && (load<u32>(t + 164) & 0x8000u) != 0) return ret(ctx, 0u);
    if (load<u8>(o + 31) != 0) return ret(ctx, 0u);
    store<u8>(o + 51, load<u8>(o + 30));
    u8 v = 0;
    if (src != 0 && (load<u64>(src + 184) & 1) != 0) v = load<u8>(src + 172);
    store<u8>(o + 30, v);
    call(ctx, afn::Obj_SetMode470750, {o, 3, 0});
    call(ctx, afn::Player_Set3632C8, {p, a2, 0});
    call(ctx, afn::Player_Set3632C8, {p, a3, 1});
    store<u32>(p + 400, a4);
    store<u32>(p + 408, a5);
    store<u32>(p + 404, a6);
    store<u32>(p + 412, a7);
    store<u32>(p + 416, src);
    store<u8>(p + 420, a8);
    store<u8>(p + 421, a9);
    ret(ctx, 1u);
}

// void Obj_Play363890(Object* o, Object* a, -, u8 lock, Object* owner,
// const u8 key[4]; f32 t): nothing when +396 has 0x02000000 set or 0x40000000
// clear. Otherwise lock sets +396 |= 0x20000000; +324 = key (4 bytes), +336 = t.
// By state (+160 >> 1): 2 or 3 -> +340 = t when t >= 0 else +332, state 2
// (+160 = (+160 & 1) | 4), Obj_SetState362D28(o, 2, 0); 0 or 4 -> +344 = owner and
// Obj_Reset3636EC(o) unless owner's +24 has +164 bit 0x8000 set. Then, with a
// and o->+8 +152 set, the +328 object's +12 x gets vfunc +188(x, a, 0) and vfunc
// +148(x) (name?)
DECOMP(00363890, Obj_Play363890) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 lock = arg<u32>(ctx, 3), owner = arg<u32>(ctx, 4), key = arg<u32>(ctx, 5);
    const f64 t = farg(ctx, 0);
    const u32 flags = load<u32>(o + 396);
    if ((flags & 0x02000000u) != 0 || (flags & 0x40000000u) == 0) return ret(ctx, flags);
    if ((lock & 0xFF) != 0) store<u32>(o + 396, flags | 0x20000000u);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 324 + k, load<u8>(key + k));
    const u32 st = load<u32>(o + 160);
    store<f32>(o + 336, static_cast<f32>(t));
    const u32 prev = load<u32>(o + 332);  // lfs f2
    const s32 s = static_cast<s32>(st >> 1);
    if (s == 2 || s == 3) {
        // f1 >= 0.0 (fcmpu gt|eq; NaN keeps +332).
        if (t >= static_cast<f64>(load<f32>(kZeroF363888))) store<f32>(o + 340, static_cast<f32>(t));
        else store<u32>(o + 340, prev);
        store<u32>(o + 160, (st & 1u) | 4u);
        call(ctx, afn::Obj_SetState362D28, {o, 2, 0});
    } else if (s == 4 || s == 0) {
        store<u32>(o + 344, owner);
        bool skip = false;
        if (owner != 0) {
            const u32 ow = load<u32>(owner + 24);
            if (ow != 0 && (load<u32>(ow + 164) & 0x8000u) != 0) skip = true;
        }
        if (!skip) call(ctx, afn::Obj_Reset3636EC, {o});
    }
    if (a == 0) return ret(ctx, load<u32>(o + 8));
    const u32 r = load<u32>(load<u32>(o + 8) + 152);
    if (r == 0) return ret(ctx, 0u);
    const u32 h = load<u32>(o + 328);
    const u32 x = load<u32>(h + 12);
    if (x == 0) return ret(ctx, 0u);
    vcall(ctx, x, 188, {x, a, 0});
    const u32 x2 = load<u32>(h + 12);
    vcall(ctx, x2, 148, {x2});
}

// u32 World_TickTimedCallbacks(World* w; f32 dt): only with w +0x1281C4 set.
// dt <= 0 becomes 1/60; sub_002E9688(dt); the kind-0 timed callback list is
// emptied, sub_00365F28(dt) runs, and whatever it (re)filled is called as
// opd(<code address>, arg; dt) and emptied again; Triggers_Clear(); the current
// world's kind-12 list is called with 0.0 (not emptied); sub_00366710(dt);
// Obj_Apply5A5EA0(+0x1281C4, -, 0; dt). Returns 0 without the object, else
// what Obj_Apply5A5EA0 returns (name?)
DECOMP(0047C2B8, World_TickTimedCallbacks) {
    const u32 w = arg<u32>(ctx, 0);
    f64 dt = farg(ctx, 0);
    if (load<u32>(w + kWorldObj1281C4) == 0) return ret(ctx, 0u);
    const f64 zero = static_cast<f64>(load<f32>(kZeroF47C2B0));
    if (dt < zero || dt == zero) dt = static_cast<f64>(load<f32>(kDefaultDt47C2B4));
    callf(ctx, afn::sub_002E9688, {}, {dt});
    store<u16>(w + kList0Count, 0);
    callf(ctx, afn::sub_00365F28, {}, {dt});
    for (u32 i = 0; i < load<u16>(w + kList0Count); ++i) {
        const u32 e = load<u32>(w + kList0Data) + i * 12;
        const u32 opd = load<u32>(e), a = load<u32>(e + 4);
        // r3 = the code address loaded from the OPD, r5 = the OPD, f1 = dt.
        ctx.gpr[5] = opd;
        ctx.fpr[1] = dt;
        call_ptr(ctx, opd, {load<u32>(opd), a});
    }
    store<u16>(w + kList0Count, 0);
    call(ctx, afn::Triggers_Clear);
    for (u32 i = 0;; ++i) {
        const u32 cw = load<u32>(kCurrentWorld);
        if (i >= load<u16>(cw + kList12Count)) break;
        const u32 e = load<u32>(cw + kList12Data) + i * 12;
        const u32 opd = load<u32>(e), a = load<u32>(e + 4);
        ctx.gpr[5] = opd;
        ctx.fpr[1] = zero;
        call_ptr(ctx, opd, {load<u32>(opd), a});
    }
    callf(ctx, afn::sub_00366710, {}, {dt});
    // r4 as sub_00366710 leaves it (Obj_Apply5A5EA0 reads only r3, r5, f1).
    callf(ctx, afn::Obj_Apply5A5EA0, {load<u32>(w + kWorldObj1281C4), ctx.gpr[4], 0}, {dt});
}
