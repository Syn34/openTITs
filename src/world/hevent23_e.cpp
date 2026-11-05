// World_HandleEvent callees, part 23e: more object mode setters called by
// Obj_SetMode470750 (the shop object's mode 4 and a speaker object's
// talk mode).
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent22;

namespace {
// Shop objects (as hevent22_a.cpp Obj_ModeIdle367C20): +216 flags, +196 the
// mode 4 text id, def +196 28-byte widget records {+12 widget, +16 u32},
// +200 count, +148 56-byte entries {+12 u32}; +172 entry index, +204, +212.
constexpr u32 kOneF3684B0 = 0x003684B0;  // f32 1.0
constexpr u32 kShopFlags = 216, kShopModeText = 196, kShopEntry = 172, kShopTimer = 204, kShopValue = 212;
constexpr u32 kShopHashes = 0x00F64F9C;  // u32[mode] effect hashes
constexpr u32 kDefWidgets = 196, kDefNumWidgets = 200, kWidgetSize = 28, kWidgetObj = 12, kWidgetArg = 16;
constexpr u32 kDefEntries = 148, kEntrySize = 56, kEntryValue = 12;
constexpr u32 kWidgetShow = 88;               // widget vtable: (w, bool)
constexpr u32 kShopState = 0x010B19E0 + 176;  // u32 +4 bit 0x8000 mirrors flag 0x00800000 (name?)
constexpr u32 kShopFlagDisabled = 0x08000000, kShopFlagActive = 0x40000000;
constexpr u32 kShopFlagSaid = 0x04000000, kShopFlagEffect = 0x02000000, kShopFlagDone = 0x01000000, kShopFlagShown = 0x00800000;

// Speaker objects: +960 flags, +232 4 bytes, +236 holder (+12 widget),
// +240 / +244 / +248 f32, +252 target, +208 talk text id.
constexpr u32 kConstsF386554 = 0x00386554;  // f32 0.0, 1.0
constexpr u32 kSpkFlags = 960, kSpkTag = 232, kSpkHolder = 236, kSpkMin = 240, kSpkValue = 244, kSpkUsed = 248, kSpkTarget = 252;
constexpr u32 kSpkTalkText = 208;
constexpr u32 kSpkHashes = 0x00F65018;  // u32[mode] effect hashes
constexpr u32 kSpkFlagBusy = 0x02000000, kSpkFlagActive = 0x40000000, kSpkFlagForced = 0x20000000;
constexpr u32 kSpkFlagSaid = 0x01000000, kSpkFlagEffect = 0x00800000;
constexpr u32 kTargetFlag = 0x8000;  // target +24 object's +164 bit: skip the refresh
constexpr u32 kSpkRefresh385008 = 0x00385008;  // (SpeakerObj*) (name?)
constexpr u32 kHolderWidget = 12, kWidgetSet = 188, kWidgetUpdate = 148;

constexpr u32 set_bit(u32 w, u32 mask, u32 r) { return (w & ~mask) | ((r & 1u) != 0 ? mask : 0u); }
}  // namespace

// u32 Obj_ShopModeOff3684B8(ShopObj* o) (name?): skipped with +216 flag
// 0x08000000 (r3: the flags) or without 0x40000000 (r3: 0). Unless the
// mode is 0 or 4: mode 4, Speaker_SayId4A5C90(+28, +196, 0; 1.0) into flag
// 0x04000000, Effect_SetHash4CF988(+20, 0x00F64F9C[4]) into 0x02000000,
// the +24 object's vtable +40 (p, 1, 1), and every def widget gets vtable
// +88 (w, record +16 != 0) (count re-read after each). Then +204 = 0,
// flag 0x01000000 set, +212 = def entry[+172] +12 and 0x010B1A94 bit
// 0x8000 = flag 0x00800000. Returns that bit.
DECOMP(003684B8, Obj_ShopModeOff3684B8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 flags = load<u32>(o + kShopFlags);
    if ((flags & kShopFlagDisabled) != 0) return ret(ctx, flags);
    if ((flags & kShopFlagActive) == 0) return ret(ctx, 0u);
    const u32 mode = load<u32>(o + kObjMode);
    const u32 def = load<u32>(o + 8);
    const u32 m = mode >> 1;
    if (m != 0 && m != 4) {
        store<u32>(o + kObjMode, (mode & 1u) | (4u << 1));
        callf(ctx, h22fn::Speaker_SayId, {load<u32>(o + 28), load<u32>(o + kShopModeText), 0}, {hevent7::lfs_f64(kOneF3684B0)});
        const u32 said = static_cast<u32>(ctx.gpr[3]);
        store<u32>(o + kShopFlags, set_bit(load<u32>(o + kShopFlags), kShopFlagSaid, said));
        const u32 e = load<u32>(o + 20);
        if (e != 0) {
            const u32 r = static_cast<u32>(call(ctx, h22fn::Effect_SetHash, {e, load<u32>(kShopHashes + 16)}));
            store<u32>(o + kShopFlags, set_bit(load<u32>(o + kShopFlags), kShopFlagEffect, r));
        }
        const u32 p = load<u32>(o + 24);
        if (p != 0) vcall(ctx, p, 40, {p, 1, 1});
        u32 n = load<u32>(def + kDefNumWidgets);
        for (u32 i = 0, off = 0; n != 0 && i < n; ++i, off += kWidgetSize) {
            const u32 rec = load<u32>(def + kDefWidgets) + off;
            const u32 w = load<u32>(rec + kWidgetObj);
            if (w == 0) continue;
            vcall(ctx, w, kWidgetShow, {w, load<u32>(rec + kWidgetArg) != 0 ? 1u : 0u});
            n = load<u32>(def + kDefNumWidgets);
        }
    }
    store<u32>(o + kShopTimer, 0);
    const u32 idx = load<u32>(o + kShopEntry);
    const u32 f2 = load<u32>(o + kShopFlags) | kShopFlagDone;
    store<u32>(o + kShopFlags, f2);
    const u32 shown = (f2 & kShopFlagShown) != 0 ? 1u : 0u;
    store<u32>(o + kShopValue, load<u32>(load<u32>(def + kDefEntries) + idx * kEntrySize + kEntryValue));
    store<u32>(kShopState + 4, set_bit(load<u32>(kShopState + 4), kTargetFlag, shown));
    ret(ctx, shown);
}

// void Obj_SpeakerTalk386560(SpeakerObj* o, u32 a, -, u8 force,
// Object* target, const u8 tag[4]; f32 value) (name?): skipped with +960
// flag 0x02000000 or without 0x40000000 (r3: the flags). force sets flag
// 0x20000000; tag is copied to +232 and value stored at +244. Modes 2 / 3:
// +248 = value when >= 0 else +240, mode 2, Speaker_SayId4A5C90(+28,
// +208, 0; 1.0) into flag 0x01000000, Effect_SetHash4CF988(+20,
// 0x00F65018[2]) into 0x00800000. Modes 0 / 7: +252 = target and, unless
// the target's +24 object has +164 bit 0x8000, 0x00385008(o). Then with a
// and def +152, the +236 holder's widget gets vtable +188 (w, a, 0) and
// +148 (w), the widget re-read in between.
DECOMP(00386560, Obj_SpeakerTalk386560) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u8 force = arg<u8>(ctx, 3);
    const u32 target = arg<u32>(ctx, 4), tag = arg<u32>(ctx, 5);
    const f64 value = farg(ctx, 0);
    const u32 flags = load<u32>(o + kSpkFlags);
    if ((flags & kSpkFlagBusy) != 0 || (flags & kSpkFlagActive) == 0) return ret(ctx, flags);
    if (force != 0) store<u32>(o + kSpkFlags, flags | kSpkFlagForced);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + kSpkTag + k, load<u8>(tag + k));
    const u32 mode = load<u32>(o + kObjMode);
    store<f32>(o + kSpkValue, static_cast<f32>(value));
    const u32 m = mode >> 1;
    const u32 speaker = load<u32>(o + 28);
    if (m == 2 || m == 3) {
        const f64 zero = hevent7::lfs_f64(kConstsF386554);
        // +240 is copied as a raw word (lfs / stfs).
        if (value > zero || value == zero) store<f32>(o + kSpkUsed, static_cast<f32>(value));
        else store<u32>(o + kSpkUsed, load<u32>(o + kSpkMin));
        store<u32>(o + kObjMode, (mode & 1u) | (2u << 1));
        callf(ctx, h22fn::Speaker_SayId, {speaker, load<u32>(o + kSpkTalkText), 0}, {hevent7::lfs_f64(kConstsF386554 + 4)});
        const u32 said = static_cast<u32>(ctx.gpr[3]);
        const u32 e = load<u32>(o + 20);
        store<u32>(o + kSpkFlags, set_bit(load<u32>(o + kSpkFlags), kSpkFlagSaid, said));
        if (e != 0) {
            const u32 r = static_cast<u32>(call(ctx, h22fn::Effect_SetHash, {e, load<u32>(kSpkHashes + 8)}));
            store<u32>(o + kSpkFlags, set_bit(load<u32>(o + kSpkFlags), kSpkFlagEffect, r));
        }
    } else if (m == 0 || m == 7) {
        store<u32>(o + kSpkTarget, target);
        u32 x = 0;
        if (target == 0 || (x = load<u32>(target + 24)) == 0 || (load<u32>(x + 164) & kTargetFlag) == 0)
            call(ctx, kSpkRefresh385008, {o});
    }
    const u32 def = load<u32>(o + 8);
    if (a == 0) return ret(ctx, def);
    const u32 d152 = load<u32>(def + 152);
    if (d152 == 0) return ret(ctx, 0u);
    const u32 holder = load<u32>(o + kSpkHolder);
    const u32 w = load<u32>(holder + kHolderWidget);
    if (w == 0) return ret(ctx, 0u);
    vcall(ctx, w, kWidgetSet, {w, a, 0});
    const u32 w2 = load<u32>(holder + kHolderWidget);
    vcall(ctx, w2, kWidgetUpdate, {w2});
}
