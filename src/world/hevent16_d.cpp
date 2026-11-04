// World_HandleEvent callees, part 16d: the coach's mode leave handler
// (called by Coach_SetMode2A8AE0, hevent15_c.cpp, before the next mode's
// enter).
#include "hevent16.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent16;

namespace {
namespace dfn {
constexpr u32 Obj_Notify24 = 0x000D949C;            // (Object*) -> Player* or 0 (symbols.txt)
constexpr u32 Camera_PushTarget0FC040 = 0x000FC040; // (Camera*, Target*, -, 0; f32 blend) (name?)
constexpr u32 Target_Construct10BD88 = 0x0010BD88;  // (Target* 688 bytes, Camera*, Object*, Object* look) (name?)
constexpr u32 Target_Enable10D9B0 = 0x0010D9B0;     // (Target*, u8) (name?)
constexpr u32 Emitter_Stop = 0x00443424;            // (handle, bool fade) (hevent.h)
constexpr u32 Ptr_DerefNonZero = 0x00446960;        // (symbols.txt)
constexpr u32 Sound_Play44C474 = 0x0044C474;        // (out, SoundDesc**, pos, gain, a, u8, c, bank, shape, u8*, u8*) (symbols.txt)
constexpr u32 Effect_Spawn = 0x004A5FC8;            // (u32 name, owner, value, u8 start) -> Effect* (symbols.txt)
constexpr u32 Effect_Bind4A6558 = 0x004A6558;       // (Effect** slot, name, name, owner, value, u8) (name?)
constexpr u32 Effect_SetTime = 0x004CF710;          // (Effect*, f32) (symbols.txt)
constexpr u32 SmallAlloc = 0x0055238C;              // (size) -> block or 0 (symbols.txt)
constexpr u32 Mat34_FromRotPos553B4C = 0x00553B4C;  // (Mat34* out, const Mat33*, const vec3*) (symbols.txt)
}  // namespace dfn

// Coach. +244 flags: bits 28..31 sub mode (the nibble), bit 16 0x10000,
// bit 14 0x4000 (copied from the player's +16 bit 0), bit 13 0x2000.
constexpr u32 kCoFlags = 244, kCoSubShift = 28, kCoFlag16 = 0x00010000u, kCoPlayerBit = 0x4000, kCoFlag13 = 0x2000;
constexpr u32 kCoObject = 8, kCoOwner = 48, kCoValue = 52, kCoWidget56 = 56, kCoEffectNames = 136;
constexpr u32 kCoTarget = 248, kCoLook = 252, kCoNext = 2568;
constexpr u32 kCoEffects[] = {188, 196, 200, 204};
constexpr u32 kCoEmitters[] = {2296, 2300, 2304};
constexpr u32 kCoSoundDesc = 2272, kCoMusic = 2308, kCoMusicLow = 2310;
constexpr u32 kCoWidget688 = 688, kCoPanel176 = 176, kPanelChild = 148, kCoWidget948 = 948, kCoWidget952 = 952;
constexpr u32 kCoWidget708 = 708, kCoTimer924 = 924;
// Readouts: widget -> f32 slot (vt +24 value() -> record, f32 at +4).
constexpr u32 kCoReadouts[][2] = {{724, 936}, {788, 944}, {852, 940}};
constexpr u32 kCoState964 = 964, kCoState960 = 960;

constexpr u32 kNodeList = 0x00F64A58;  // u32 first node, linked through +2568 (as hevent_d.cpp)
constexpr u32 kWorldCamera = 0x1281E0, kWorldFlags40 = 40;
constexpr u32 kEmptyName = 0x00AD4B7C;  // ""
constexpr u32 kBlend2A2990 = 0x002A2990;  // f32 -1.0, then f32 at +4 the effect start time

// Object: +192 bit 3 (0x8) usable, +184 u64 flags (0x100 set here), +96 rotation
// rows (16-byte stride); vt +88 (o, 0, 1, 1, 1), vt +84 set_matrix(o, Mat34*).
constexpr u32 kObjFlags192 = 192, kObjUsable = 0x8, kObjFlags184 = 184, kObjRot = 96;
// Player: +16 u8, +64 Group** (+72 u16 count; group +0 8-byte entries {Widget*},
// +8 u16 count), +2336 / +2338 u8 flags.
constexpr u32 kPlGroups = 64, kPlGroupCount = 72, kGroupEntries = 0, kGroupCount = 8;
constexpr u32 kPlFlags2336 = 2336, kPlFlags2338 = 2338, kPlBit16 = 16;

// Widget vtable slots.
constexpr u32 kVtRelease = 4, kVtValue = 24, kVtShow = 40, kVtPlayerA = 44, kVtPlayerB = 48, kVtState = 52;
constexpr u32 kVtSetMatrix = 84, kVtHide = 88;

// Stack (offsets as the original's r1): +144 Mat33, +192 Mat34, +256 / +260
// sound handles, +264 the state record vt +52 returns.
constexpr u32 kFrame = 288, kSpRot = 144, kSpMat = 192, kSpSound = 256, kSpMusic = 260, kSpState = 264;

u32 sub_mode(u32 c) { return load<u32>(c + kCoFlags) >> kCoSubShift; }
void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
u64 sound_play(PPUContext& ctx, u32 out, u32 desc) { return call(ctx, dfn::Sound_Play44C474, {out, desc, 0, 0, 0, 0, 0, 0, 0, 0, 0}); }
}  // namespace

// u8 Coach_Leave2A2998(Coach* c, s32 mode) (name?): leaves coach mode
// `mode`.
//   0: links c into the node list (0x00F64A58), marks the object (+8, if
//      usable) and its player, hides the player's widgets, pushes a new
//      camera target (sub mode 0), clears the look object, then rebinds the
//      four effects, stops the three emitters, restarts the sound (+2272)
//      and reads the readouts and the +948 state.
//   10: clears world +40 bits 1 and 2, hides +56, stops the music (+2308)
//      and restarts it when Ptr_DerefNonZero(0).
//   6: hides +56.  3: hides +688, the +176 panel and child, +948, +952.
//   7 / 9: releases the +924 timer; 7 / 8 / 9: hides +708.
//   All but 0, 6 and 10 then spawn the sub mode's effect (+136 + 4 * sub)
//   when c +48 is set; 6, 7, 8, 9, 10 finally disable the camera target
//   (+248).
DECOMP(002A2998, Coach_Leave2A2998) {
    const u32 c = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 1);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    u64 r3 = 0;
    auto hide = [&](u32 w) { r3 = w != 0 ? vcall(ctx, w, kVtHide, {w, 0}) : 0; return w != 0; };
    auto show_off = [&](u32 w) { r3 = w != 0 ? vcall(ctx, w, kVtShow, {w, 0, 1}) : 0; };
    // The camera target's disable (loc_002A2BFC).
    auto disable_target = [&]() {
        const u32 t = load<u32>(c + kCoTarget);
        if (t != 0) r3 = call(ctx, dfn::Target_Enable10D9B0, {t, 0});
        ctx.gpr[3] = r3;
    };

    if (mode == 10) {
        const u32 w = load<u32>(hevent::kCurrentWorld);
        store<u32>(w + kWorldFlags40, load<u32>(w + kWorldFlags40) & ~2u);
        const u32 w2 = load<u32>(hevent::kCurrentWorld);
        store<u32>(w2 + kWorldFlags40, load<u32>(w2 + kWorldFlags40) & ~4u);
        show_off(load<u32>(c + kCoWidget56));
        call(ctx, dfn::Emitter_Stop, {load<u32>(c + kCoMusic), 1});
        store<u16>(c + kCoMusicLow, 0);  // the handle's low half (serial)
        r3 = call(ctx, dfn::Ptr_DerefNonZero, {0});
        if (static_cast<u32>(r3) != 0) {
            sound_play(ctx, sp + kSpMusic, 0);
            r3 = load<u32>(sp + kSpMusic);
            store<u32>(c + kCoMusic, static_cast<u32>(r3));
        }
        return disable_target();
    }
    if (mode == 6) {
        show_off(load<u32>(c + kCoWidget56));
        return disable_target();
    }
    if (mode != 0) {
        bool disable = false;
        if (mode == 3) {
            hide(load<u32>(c + kCoWidget688));
            const u32 p = load<u32>(c + kCoPanel176);
            if (p != 0 && load<u32>(p + kPanelChild) != 0) {
                vcall(ctx, p, kVtHide, {p, 0});
                const u32 ch = load<u32>(load<u32>(c + kCoPanel176) + kPanelChild);
                vcall(ctx, ch, kVtHide, {ch, 0});
            }
            hide(load<u32>(c + kCoWidget948));
            hide(load<u32>(c + kCoWidget952));
        } else if (mode == 7 || mode == 8 || mode == 9) {
            if (mode != 8) {
                const u32 t = load<u32>(c + kCoTimer924);
                if (t != 0) {
                    vcall(ctx, t, kVtRelease, {t});
                    store<u32>(c + kCoTimer924, 0);
                }
            }
            disable = true;
            hide(load<u32>(c + kCoWidget708));
        }
        // loc_002A2D44
        const u32 owner = load<u32>(c + kCoOwner);
        r3 = owner;
        if (owner != 0) {
            const u32 name = load<u32>(c + kCoEffectNames + 4 * sub_mode(c));
            r3 = call(ctx, dfn::Effect_Spawn, {name, owner, load<u32>(c + kCoValue), 1});
            if (static_cast<u32>(r3) != 0) {
                callf(ctx, dfn::Effect_SetTime, {static_cast<u32>(r3)}, {hevent7::lfs_f64(kBlend2A2990 + 4)});
                r3 = ctx.gpr[3];
            }
        }
        if (disable) return disable_target();
        ctx.gpr[3] = r3;
        return;
    }

    // Mode 0.
    store<u32>(c + kCoNext, load<u32>(kNodeList));
    store<u32>(kNodeList, c);
    const u32 o = load<u32>(c + kCoObject);
    if (o != 0 && (load<u32>(o + kObjFlags192) & kObjUsable) != 0) {
        store<u64>(o + kObjFlags184, load<u64>(o + kObjFlags184) | 0x100);
        if (sub_mode(c) != 2) vcall(ctx, o, kVtHide, {o, 0, 1, 1, 1});
        const u32 pl = static_cast<u32>(call(ctx, dfn::Obj_Notify24, {o}));
        if (pl != 0) {
            const u32 f = load<u32>(c + kCoFlags);
            if ((f >> kCoSubShift) != 0) {
                store<u32>(c + kCoFlags, (f & ~kCoPlayerBit) | ((load<u8>(pl + kPlBit16) & 1u) << 14));
                vcall(ctx, pl, kVtPlayerA, {pl, 0});
                store<u32>(c + kCoFlags, load<u32>(c + kCoFlags) | kCoFlag13);
            } else {
                vcall(ctx, pl, kVtPlayerB, {pl, 1, 1});
                u8 b = static_cast<u8>(load<u8>(pl + kPlFlags2336) & ~0x80);
                store<u8>(pl + kPlFlags2336, b);
                if ((b & 0x20) != 0) store<u8>(pl + kPlFlags2336, static_cast<u8>((b & ~0x10) | 0x08));
                for (u32 i = 0; i < load<u16>(pl + kPlGroupCount); ++i) {
                    const u32 g = load<u32>(load<u32>(pl + kPlGroups) + 4 * i);
                    for (u32 j = 0; j < load<u16>(g + kGroupCount); ++j) {
                        const u32 w = load<u32>(load<u32>(g + kGroupEntries) + 8 * j);
                        if (w != 0) vcall(ctx, w, kVtShow, {w, 0, 1});
                    }
                }
                store<u8>(pl + kPlFlags2338, static_cast<u8>(load<u8>(pl + kPlFlags2338) & ~0x40));
            }
        }
        // loc_002A2DD0
        if (sub_mode(c) == 0) {
            const u32 cam = load<u32>(load<u32>(hevent::kCurrentWorld) + kWorldCamera);
            if (cam != 0) {
                const u32 t = static_cast<u32>(call(ctx, dfn::SmallAlloc, {688}));
                if (t != 0) call(ctx, dfn::Target_Construct10BD88, {t, cam, o, load<u32>(c + kCoLook)});
                store<u32>(c + kCoTarget, t);
                if (t != 0) {
                    u32 a = t;
                    if (sub_mode(c) != 0) {
                        call(ctx, dfn::Target_Enable10D9B0, {t, 1});
                        a = load<u32>(c + kCoTarget);
                    }
                    // r5 still holds the constant's address.
                    callf(ctx, dfn::Camera_PushTarget0FC040, {cam, a, kBlend2A2990, 0}, {hevent7::lfs_f64(kBlend2A2990)});
                }
                const u32 look = load<u32>(c + kCoLook);
                if (look != 0) {
                    for (u32 r = 0; r < 48; r += 16)
                        for (u32 k = 0; k < 12; k += 4) copy32(sp + kSpRot + r + k, o + kObjRot + r + k);
                    call(ctx, dfn::Mat34_FromRotPos553B4C, {sp + kSpMat, sp + kSpRot, look + 128});
                    vcall(ctx, o, kVtSetMatrix, {o, sp + kSpMat});
                }
                store<u32>(c + kCoLook, 0);
            }
        }
    }

    // loc_002A2F24
    for (const u32 off : kCoEffects)
        call(ctx, dfn::Effect_Bind4A6558, {c + off, kEmptyName, kEmptyName, load<u32>(c + kCoOwner), load<u32>(c + kCoValue), 1});
    for (const u32 off : kCoEmitters) call(ctx, dfn::Emitter_Stop, {load<u32>(c + off), 1});
    const u32 desc = load<u32>(c + kCoSoundDesc);
    store<u32>(c + kCoFlags, load<u32>(c + kCoFlags) & ~kCoFlag16);
    sound_play(ctx, sp + kSpSound, desc);
    store<u32>(c + kCoEmitters[0], load<u32>(sp + kSpSound));
    for (const auto& rd : kCoReadouts) {
        const u32 w = load<u32>(c + rd[0]);
        if (w == 0) continue;
        const u32 v = static_cast<u32>(vcall(ctx, w, kVtValue, {w}));
        copy32(c + rd[1], v + 4);
        ctx.fpr[1] = hevent7::lfs_f64(v + 4);  // lfs / stfs through f1
    }
    const u32 st = load<u32>(c + kCoWidget948);  // no null check
    vcall(ctx, st, kVtState, {sp + kSpState, st});
    const u8 s = load<u8>(sp + kSpState + 3);
    store<u8>(c + kCoState964, s);
    store<u32>(c + kCoState960, 0);
    ret(ctx, static_cast<u32>(s));
}
