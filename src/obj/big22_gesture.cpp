// Game code, large-function sweep part 22: the cooldown-gated animation pick
// of f_0041DEC0 / f_0041E6A8's object (list +104, animator source +468).
#include "big22.h"
#include "big23.h"

namespace sly4::obj {

namespace {
constexpr u32 kGestureK = 0x0041D148u;   // f32 0.0, 1.0, 0.1
constexpr u32 kPlayerRecs = 0x010B19E0u; // 11968-byte per-player records; +176 +8 / +12 flag words
constexpr u32 kCooldown = 0x3E99999Au;   // f32 0.3
constexpr u32 Anim_TrackSet36 = 0x0029879Cu;  // (an, 0, 0) -> bool
constexpr u32 Anim_Find = 0x00298874u;        // (an, 0) -> track or 0
constexpr u32 Anim_Restart = 0x002775A8u;     // (an, key, a, b; f, f, c, d; f, f, e, g)

struct GestureObj {
    pad<104> _0;
    gp<gp<GestureItem>> items;  // +104
    pad<4> _108;
    be_u16 count;               // +112
    pad<354> _114;
    be_u32 anim;                // +468: vfn[60] the animator
    pad<12> _472;
    u8 cooling;                 // +484
    pad<3> _485;
    be_f32 cooldown;            // +488
};
static_assert(offsetof(GestureObj, count) == 112 && offsetof(GestureObj, anim) == 468 && offsetof(GestureObj, cooldown) == 488);
// An item's u8 +164: ready.
struct GestureItemHead {
    pad<164> _0;
    u8 ready;  // +164
};

f64 C(u32 off) { return lf(global<be_f32>(kGestureK + off)); }
u32 Animator(const GestureObj* o) { return vcall<u32>(&gm<u8>(o->anim), 60); }

// Anim_Restart2775A8(an, key, 0, 0; 1.0, f2, 0, 0; 0.0, 0.0, 0, -1) with
// f2 0.1 (alt) or 0.0.
void Restart(const GestureObj* o, u32 key, bool alt) {
    const u32 an = Animator(o);
    const f64 a = C(0), f1 = C(4), f2 = alt ? C(8) : C(0);
    call(Anim_Restart, an, key, 0u, 0u, f1, f2, 0u, 0u, a, a, 0u, -1);
}
}  // namespace

// void Obj_GesturePick41D158(Obj* o; f32 dt) (no caller reads r3 or f1):
// while the cooldown u8 +484 is set, counts f32 +488 down by dt and clears
// both at <= 0.0. Otherwise, with an animator (+468 vfn[15]) that has
// track 0 (Anim_Find) or accepts Anim_TrackSet36(0, 0): the local player's
// record (0x010B19E0 + 11968 * u8 world+4209, flag words +184 / +188)
// picks the first mode whose bit is set in both words: 0 (0x10), 1 (0x20),
// 2 (0x80), 3 (0x40); none returns. Every list item (+104, u16 count +112)
// matching the mode (Item_GestureStep41B200) marks a match; one with u8
// +164 set marks "ready". Ready restarts the mode's clip 0x74799DD2 /
// 0x154F4C1D / 0xE106CE20 / 0xFA48404A with f32 (1.0, 0.0, 0.0, 0.0) and
// starts the 0.3 cooldown; otherwise the clip 0x66DA775B / 0x42C053B0 /
// 0x7ADF054D / 0x83F1A5E3 with (1.0, 0.1, 0.0, 0.0).
void Obj_GesturePick41D158(GestureObj* o, f64 dt) {
    if (o->cooling != 0) {
        const f64 t = S(lf(o->cooldown) - dt);
        o->cooldown = static_cast<f32>(t);
        if (t < C(0) || t == C(0)) {
            o->cooling = 0;
            set_bits(o->cooldown, 0u);
        }
        return;
    }
    if (call<u32>(Anim_Find, Animator(o), 0u) == 0 && call<u32>(Anim_TrackSet36, Animator(o), 0u, 0u) == 0) return;
    const u32 rec = kPlayerRecs + gm<u8>(global<be_u32>(kWorld) + 4208 + 1) * 11968u + 176;
    const u32 fa = gm<be_u32>(rec + 8), fb = gm<be_u32>(rec + 12);
    u32 mode;
    if ((fa & 0x10u) != 0 && (fb & 0x10u) != 0) {
        mode = 0;
    } else if ((fa & 0x20u) != 0 && (fb & 0x20u) != 0) {
        mode = 1;
    } else if ((fa & 0x80u) != 0 && (fb & 0x80u) != 0) {
        mode = 2;
    } else if ((fa & 0x40u) == 0 || (fb & 0x40u) == 0) {
        return;
    } else {
        mode = 3;
    }
    bool matched = false, ready = false;
    for (u32 i = 0; i < o->count; ++i) {
        if (Item_GestureStep41B200(o->items[i], mode)) {
            matched = true;
            if (gm<GestureItemHead>(o->items[i].ea).ready != 0) ready = true;
        }
    }
    static constexpr u32 kReady[4] = {0x74799DD2u, 0x154F4C1Du, 0xE106CE20u, 0xFA48404Au};
    static constexpr u32 kIdle[4] = {0x66DA775Bu, 0x42C053B0u, 0x7ADF054Du, 0x83F1A5E3u};
    Restart(o, ready ? kReady[mode] : kIdle[mode], !ready);
    if (ready && matched) {
        o->cooling = 1;
        set_bits(o->cooldown, kCooldown);
    }
}

}  // namespace sly4::obj

NATIVE(0041D158, sly4::obj::Obj_GesturePick41D158)
