// Game code, large-function sweep part 7: an object's per-frame state
// machine update / tick with its part list, and an attachment matrix.
#include "big7.h"

using namespace sly4;
using namespace sly4::big7;
using namespace sly4::big7::b7fn;

namespace {
// Object fields.
constexpr u32 kObjMat = 96;          // f32 [4][4]
constexpr u32 kObjWorldObj = 160;    // WorldObj*: +80 its matrix; vfunc +28 -> skeleton
constexpr u32 kObjMsgArg = 168;      // u32 passed to Msg_Post
constexpr u32 kObjFlags172 = 172;    // 0x40000: re-init on a reset, 0x8: effect check
constexpr u32 kObjFlags176 = 176;    // u16: 0x1000 frozen (no update)
constexpr u32 kObjFlags192 = 192;    // 0x10
constexpr u32 kObjSkel = 216;        // +188 u16: 0x1000 effect check, bits 4..7 state
constexpr u32 kObjCtl = 220;         // controller: vfunc +36 (ctl, o) on a reset
constexpr u32 kObjParts = 248;       // {+0 Part** items, +8 u16 count} or 0; Part: +16 u8 active
constexpr u32 kObjTimer = 352;       // f32, counts down by dt
constexpr u32 kObjEffect = 356;      // Effect*
constexpr u32 kObjQueue = 368;       // event queue object (its +172 0x40000: live) or 0
constexpr u32 kObjFlags376 = 376;    // 0x40000000 reset, 0x20000000 effect state change, 0x00F00000 / 0x0F000000 new / old state, 0x20000, 0x8000
constexpr u32 kZero3 = 0x01164B40u;  // the zero vector
constexpr u32 kWorldPtr = 0x00F65CA0u;  // World*

constexpr u32 kVfnPartUpdate = 24;   // Part::update(f1 dt)
constexpr u32 kVfnPartTick = 28;     // Part::tick(f1 dt)
constexpr u32 kVfnSkeleton = 28;     // WorldObj -> skeleton (+32: nodes, 16 bytes each, +8 matrix)
constexpr u32 kVfnEffectOn = 96;     // Obj (o, bool on)
constexpr u32 kVfnTarget = 144;      // Obj (o) -> target (+44 low 5 bits: kind)
constexpr u32 kVfnCtlReset = 36;     // Controller (ctl, o)
constexpr u32 kReinit = 0x000E0620u;  // (o) (name?)

// Each active part of the list at +248 (re-read every step) gets vfunc slot
// with f1 = dt.
void RunParts(PPUContext& ctx, u32 o, f64 dt, u32 slot) {
    for (u32 i = 0;; ++i) {
        const u32 list = load<u32>(o + kObjParts);
        const u32 n = list != 0 ? load<u16>(list + 8) : 0u;
        if (i >= n) break;
        const u32 part = load<u32>(load<u32>(list) + 4 * i);
        if (load<u8>(part + 16) == 0) continue;
        ctx.fpr[1] = dt;
        VCall(ctx, part, slot, {part});
    }
}
}  // namespace

// bool Obj_UpdateBase(Obj* o, f32 dt)
// Not when frozen (u16 +176 & 0x1000): its state machine's update, then
// every active part's update(dt). True when it ran to the end.
DECOMP(000D83C4, Obj_UpdateBase) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    if ((load<u16>(o + kObjFlags176) & 0x1000u) != 0) {
        ret(ctx, 0u);
        return;
    }
    callf(ctx, kStateMachineUpdate, {o}, {dt});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    RunParts(ctx, o, dt, kVfnPartUpdate);
    ret(ctx, 1u);
}

// void Obj_AttachMatrix(Obj* o, f32 out[4][4], const Attach* el, const f32 base[4][4])
// An attachment's matrix: with a WorldObj (+160) and a bone (el +20 >= 0),
// that bone's matrix from the WorldObj's skeleton times the WorldObj's
// matrix; else the driver's matrix (el +16, its +80) or base. Then a
// non-zero offset (el +0, 3 f32) is transformed into the translation row.
// Only the x/y/z words of each row are copied in; the product writes whole
// rows.
DECOMP(000E3534, Obj_AttachMatrix) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 out = arg<u32>(ctx, 1);
    const u32 el = arg<u32>(ctx, 2);
    const u32 base = arg<u32>(ctx, 3);
    const u32 wo = load<u32>(o + kObjWorldObj);
    u32 last;  // the last word copied (left in f1)
    if (wo != 0 && static_cast<s32>(load<u32>(el + 20)) >= 0) {
        const u32 bone = load<u32>(el + 20);
        const u32 skel = static_cast<u32>(VCall(ctx, wo, kVfnSkeleton, {wo}));
        const u32 node = skel != 0 ? load<u32>(skel + 32) + (bone << 4) : 0u;
        // Quirk: without a skeleton the matrix pointer is read from address 8.
        last = CopyMat43Last(out, load<u32>(node + 8));
        const u32 w = load<u32>(o + kObjWorldObj) + 80;
        const V4 w0 = Lvx(w), w1 = Lvx(w + 16), w2 = Lvx(w + 32), w3 = Lvx(w + 48);
        const V4 r0 = Lvx(out), r1 = Lvx(out + 16), r2 = Lvx(out + 32), r3 = Lvx(out + 48);
        Stvx(out, Row3(w0, w1, w2, r0.e[0], r0.e[1], r0.e[2]));
        Stvx(out + 16, Row3(w0, w1, w2, r1.e[0], r1.e[1], r1.e[2]));
        Stvx(out + 32, Row3(w0, w1, w2, r2.e[0], r2.e[1], r2.e[2]));
        Stvx(out + 48, Add4(w3, Row3(w0, w1, w2, r3.e[0], r3.e[1], r3.e[2])));
    } else {
        const u32 drv = load<u32>(el + 16);
        last = CopyMat43Last(out, drv != 0 ? drv + 80 : base);
    }
    if (load<u64>(el) == load<u64>(kZero3) && load<u32>(el + 8) == load<u32>(kZero3 + 8)) {
        SetF1Bits(ctx, last);
        ret(ctx, 0u);
        return;
    }
    const f32 s0 = std::bit_cast<f32>(LaneW(Lvlx(el), 0));
    const f32 s1 = std::bit_cast<f32>(LaneW(Lvlx(el + 4), 0));
    const f32 s2 = std::bit_cast<f32>(LaneW(Lvlx(el + 8), 0));
    const V4 p = Add4(Lvx(out + 48), Row3(Lvx(out), Lvx(out + 16), Lvx(out + 32), s0, s1, s2));
    store<u32>(out + 48, LaneW(p, 0));
    store<u32>(out + 52, LaneW(p, 1));
    store<u32>(out + 56, LaneW(p, 2));
    SetF1Bits(ctx, LaneW(p, 2));
    ret(ctx, 112u);  // what r3 holds at the original's exit on this path
}

// bool Obj_TickBase(Obj* o, f32 dt)
// Counts +352 down by dt. A pending effect state change (+376 0x20000000)
// posts its hashes (and re-inits via 0x000E0620 when the new state matches
// the skeleton's); an effect check may switch the effect (vfunc +96). On a
// reset (+376 0x40000000): event hash, the controller's reset, queue push
// and Msg_Post; unless the target is of kind 5 or +192 has 0x10 the world
// is queried with a fresh event; then the reset flag is cleared and the
// queue dropped (with +172 0x40000, re-init and go on, else false). Then the
// state machine's tick and every active part's tick(dt).
DECOMP(000E0734, Obj_TickBase) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const f32 timer = static_cast<f32>(static_cast<f64>(F(o + kObjTimer)) - dt);
    u32 fl = load<u32>(o + kObjFlags376);
    const u32 skel = load<u32>(o + kObjSkel);
    store<f32>(o + kObjTimer, timer);
    ctx.fpr[1] = timer;
    if ((fl & 0x20000000u) != 0) {
        u32 e = load<u32>(o + kObjEffect);
        if (e != 0) {
            call(ctx, kEffectSetHash, {e, 0x77C17F6Fu});
            e = load<u32>(o + kObjEffect);
            if (e != 0) {
                const u32 f = load<u32>(o + kObjFlags376);
                const u32 now = (f >> 20) & 15, was = (f >> 24) & 15;
                if (now != was) {
                    store<u32>(o + kObjFlags376, (f & ~0x0F000000u) | (now << 24));
                    call(ctx, kEffectSetHash, {e, now - 0x7F91915Fu});
                } else if (was == ((load<u16>(skel + 188) >> 4) & 15u)) {
                    call(ctx, kReinit, {o});
                }
            }
        }
        fl = load<u32>(o + kObjFlags376) & ~0x20000000u;
        store<u32>(o + kObjFlags376, fl);
    }
    if ((load<u16>(skel + 188) & 0x1000u) != 0 && (fl & 0x8000u) == 0 && (load<u32>(o + kObjFlags172) & 0x8u) != 0) {
        const u32 e = load<u32>(o + kObjEffect);
        if (e != 0 && static_cast<u32>(call(ctx, kEffectCheck, {e, 0})) != 0) {
            const u32 opd = load<u32>(load<u32>(o) + kVfnEffectOn);
            const u32 on = (load<u32>(o + kObjFlags376) >> 15) & 1;
            call_ptr(ctx, opd, {o, on, load<u32>(opd), opd});  // entry in r5, OPD in r6
        }
    }
    fl = load<u32>(o + kObjFlags376);
    if ((fl & 0x40000000u) != 0) {
        const u32 e = load<u32>(o + kObjEffect);
        store<u32>(o + kObjFlags376, fl & ~0x20000u);
        if (e != 0) call(ctx, kEffectSetHash, {e, 0x879F2BDAu});
        u32 q = 0;
        const u32 q0 = load<u32>(o + kObjQueue);
        if (q0 != 0 && (load<u32>(q0 + 172) & 0x40000u) != 0) q = q0;
        const u32 c = load<u32>(o + kObjCtl);
        if (c != 0) VCall(ctx, c, kVfnCtlReset, {c, o});
        call(ctx, kQueuePush, {q, o, 0x03DE9A54u, 0, 1});
        call(ctx, kMsgPost, {o, 0xABB250AEu, 0xCCF83851u, load<u32>(o + kObjMsgArg)});
        const u32 t = static_cast<u32>(VCall(ctx, o, kVfnTarget, {o}));
        const bool skip = (t != 0 && (load<u32>(t + 44) & 0x1Fu) == 5) || (o != 0 && (load<u32>(o + kObjFlags192) & 0x10u) != 0);
        if (!skip) {
            Stack frame(ctx, 48);  // the original's event at sp+112..+151
            const u32 ev = frame.addr();
            for (u32 k = 0; k < 6; ++k) store<u32>(ev + 4 * k, 0);
            call(ctx, kEventInit, {ev, o, 0, 0, 0, 0, 0, 0});
            u32 kind = 0xFFFFFFFFu;
            if (q != 0 && (load<u32>(q + 188) & 1u) != 0) kind = static_cast<u32>(static_cast<s32>(load<s8>(q + 172)));
            call(ctx, kWorldQuery, {load<u32>(kWorldPtr), kind, ev});
        }
        const u32 f2 = load<u32>(o + kObjFlags376);
        const u32 f172 = load<u32>(o + kObjFlags172);
        store<u32>(o + kObjFlags376, f2 & ~0x40000000u);
        store<u32>(o + kObjQueue, 0);
        if ((f172 & 0x40000u) == 0) {
            ret(ctx, 0u);
            return;
        }
        call(ctx, kReinit, {o});
    }
    callf(ctx, kStateMachineTick, {o}, {dt});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    RunParts(ctx, o, dt, kVfnPartTick);
    ret(ctx, 1u);
}
