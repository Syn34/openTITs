// World_HandleEvent callees, part 16f: the coach's mode enter handler
// (called by Coach_SetMode2A8AE0, hevent15_c.cpp, after the previous mode's
// leave, Coach_Leave2A2998 in hevent16_d.cpp).
#include <bit>

#include "hevent16.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent16;
using hevent7::lfs_f64;

namespace {
namespace ffn {
constexpr u32 Obj_Notify24 = 0x000D949C;            // (Object*) -> Player* or 0 (symbols.txt)
constexpr u32 Obj_PopLayer = 0x000FBB88;            // (Object*, Animator*; f32 t) (symbols.txt)
constexpr u32 Target_Enable10D9B0 = 0x0010D9B0;     // (Target*, u8) (hevent16_d.cpp)
constexpr u32 Obj_Reset12E5E8 = 0x0012E5E8;         // (Object*) (name?)
constexpr u32 Coach_Refresh2A0F5C = 0x002A0F5C;     // (Coach*) (hevent16_b.cpp)
constexpr u32 Coach_Current2A11D8 = 0x002A11D8;     // (Coach*) -> entry (+12 Widget*) or 0 (name?)
constexpr u32 Coach_Show2A1290 = 0x002A1290;        // (Coach*, u32, Page*) (name?)
constexpr u32 Text_Lookup34A398 = 0x0034A398;       // (u32 id) -> text or 0 (name?)
constexpr u32 Speaker_Say3514AC = 0x003514AC;       // (Speaker*, text, u8, -, -, -, 0, 0, u32; f32, f32, f32) (name?)
constexpr u32 Effect_Release36A504 = 0x0036A504;    // (Effect** slot) (name?)
constexpr u32 Sound_Play44C474 = 0x0044C474;        // (out, SoundDesc**, ...) 11 arguments (symbols.txt)
constexpr u32 Hud_SetIcon4645A0 = 0x004645A0;       // (Hud*, 1, Icon*) (name?)
constexpr u32 Query_Init465218 = 0x00465218;        // (Query* 24 bytes, Object*) (name?)
constexpr u32 World_Query482D14 = 0x00482D14;       // (World*, u32 mask, Query*) (name?)
constexpr u32 World_Pause48A01C = 0x0048A01C;       // (World*, 0) (name?)
constexpr u32 World_Resume48A068 = 0x0048A068;      // (World*, 0) (name?)
constexpr u32 Effect_Spawn = 0x004A5FC8;            // (u32 name, owner, value, u8 start) -> Effect* (symbols.txt)
constexpr u32 Effect_Bind4A63FC = 0x004A63FC;       // (Effect** slot, name, 0, owner, value, u8) (name?)
constexpr u32 Effect_Bind4A6558 = 0x004A6558;       // (Effect** slot, name, name, owner, value, u8) (hevent16_d.cpp)
constexpr u32 Effect_SetTime = 0x004CF710;          // (Effect*, f32) (symbols.txt)
constexpr u32 Effect_SetHash4CF988 = 0x004CF988;    // (Effect*, u32 hash) (symbols.txt)
constexpr u32 UiElem_SetTexture = 0x004D8A80;       // (UiElem*, Texture*, const f32* uv0, const f32* uv1) (symbols.txt)
constexpr u32 Obj_Attach4E2DB0 = 0x004E2DB0;        // (source id, key, u8, u8) -> Binding* (symbols.txt)
constexpr u32 Emitter_Stop = 0x00443424;            // (handle, bool fade) (hevent.h)
}  // namespace ffn

// Coach fields (see also hevent16_d.cpp).
constexpr u32 kCoFlags = 244, kCoSubShift = 28;
constexpr u32 kCoObject = 8, kCoOwner = 48, kCoSpeaker = 52, kCoWidget56 = 56, kCoSpeaker60 = 60;
constexpr u32 kCoEffSlots = 68, kCoEffSlotStride = 24, kCoEffNames = 128, kCoEnterNames = 116, kCoEffName148 = 148;
constexpr u32 kCoEffName152 = 152, kCoPanel176 = 176, kPanelChild = 148, kCoTarget = 248;
constexpr u32 kCoText220 = 220, kCoText224 = 224, kCoText228 = 228, kCoText232 = 232, kCoText236 = 236, kCoText240 = 240, kCoText2292 = 2292;
constexpr u32 kCoSound2264 = 2264, kCoSound2268 = 2268, kCoSound2284 = 2284;
constexpr u32 kCoWidgets2340 = 2340, kCoNumWidgets2340 = 8;
constexpr u32 kCoMood = 2312, kCoMoodScale = 2316, kCoPageIndex = 2456, kCoPages = 2480, kCoStyle = 2564;
constexpr u32 kCoNext = 2568, kCoPrev = 2572;
constexpr u32 kCoWidget688 = 688, kCoWidget708 = 708, kCoIcon712 = 712, kCoSrc916 = 916, kCoSrc920 = 920, kCoBinding = 924;
constexpr u32 kCoWidget948 = 948, kCoWidget952 = 952, kCoAlpha964 = 964;
constexpr u32 kCoEffects[] = {188, 192, 196, 200, 204};
constexpr u32 kCoEmitters[] = {2296, 2300, 2304};
constexpr u32 kCoReadouts[][2] = {{724, 936}, {788, 944}, {852, 940}};
constexpr u32 kFlag12 = 0x1000, kFlag13 = 0x2000, kFlag14 = 0x4000, kFlag15 = 0x8000, kFlag16 = 0x10000;

constexpr u32 kNodeList = 0x00F64A58;      // as hevent16_d.cpp
constexpr u32 kIconHud = 0x00F64A60, kIconTexture = 0x00F64A5C, kIconScale = 0x00F64A64;
constexpr u32 kEffectHub = 0x00F64A6C;     // Effect* given hashes on enter (name?)
constexpr u32 kWorldCamera = 0x1281E0, kWorldPages = 0x128168, kWorldFlags40 = 40;
constexpr u32 kPagesPanel = 656;           // +4 / +8 two Widget*
constexpr u32 kEmptyName = 0x00AD4B7C;     // ""
constexpr u32 kTint948 = 0x010599E4, kTint952 = 0x01059DCC;  // style names (name?)
constexpr u32 kHud10B19E0 = 0x010B19E0;
constexpr u32 kConst = 0x002A1628;         // f32 0.0167, 1.0, 0.0, 0.5, 255.0; f64 255.0 (+24), 0.0 (+32); f32 -1.0 (+40)
// Function statics of the icon set-up (guard bytes, uv rectangle corners).
constexpr u32 kUvGuard0 = 0x01068088, kUvGuard1 = 0x01068090, kUv0 = 0x01068098, kUv1 = 0x010680A0;

// Object / player bits.
constexpr u32 kObjFlags192 = 192, kObjUsable = 0x8, kObjFlags184 = 184, kPlFlags2338 = 2338;

// Stack (offsets as the original's r1).
constexpr u32 kFrame = 272, kSpRead = 144, kSpSound192 = 192, kSpSound196 = 196, kSpSound200 = 200, kSpSound204 = 204;
constexpr u32 kSpQuery = 208, kSpState244 = 244, kSpState240 = 240;

u32 sub_mode(u32 c) { return load<u32>(c + kCoFlags) >> kCoSubShift; }
u64 sound_play(PPUContext& ctx, u32 out, u32 desc) { return call(ctx, ffn::Sound_Play44C474, {out, desc, 0, 0, 0, 0, 0, 0, 0, 0, 0}); }
// Virtual call with r5 / r6 set as the original sequence leaves them.
u64 vcall_r56(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args, u64 r5, u64 r6) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    ctx.gpr[5] = r5;
    ctx.gpr[6] = r6;
    return call_ptr(ctx, opd, args);
}
}  // namespace

// u32 Coach_Enter2A1658(Coach* c, s32 mode) (name?): enters coach mode `mode`
// (the counterpart of Coach_Leave2A2998): mode-specific speech
// (Speaker_Say3514AC with a looked-up text), sounds, widgets and world
// pause flags; modes 1..5, 8, 9 and the unknown ones then spawn the sub
// mode's effects and refresh the +2340 widgets, 0, 6, 7 and 10 refresh all
// widgets instead; 6..10 finally enable the camera target (+248).
DECOMP(002A1658, Coach_Enter2A1658) {
    const u32 c = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 1);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    u64 r3 = 0;
    bool flag30 = false;   // r30 = 1: the +148 effect too
    bool target = false;   // r29: enable the camera target
    const f64 one = lfs_f64(kConst + 4), zero = lfs_f64(kConst + 8);

    auto vc = [&](u32 obj, u32 slot, std::initializer_list<u64> args) { return vcall(ctx, obj, slot, args); };
    // Speech: f_0034A398(text id), then Speaker_Say3514AC(speaker, text, a3,
    // r6, r7, r8 (r6..r8 not read), 0, 0, stack word; f1, f2, f3).
    auto say = [&](u32 speaker, u32 text_id, u32 a3, bool r6_const, f64 f2, f64 f3) {
        if (speaker == 0) return;
        const u32 t = static_cast<u32>(call(ctx, ffn::Text_Lookup34A398, {text_id}));
        if (t == 0) return;
        const u64 r6 = r6_const ? kConst : ctx.gpr[6];
        callf(ctx, ffn::Speaker_Say3514AC, {speaker, t, a3, r6, ctx.gpr[7], ctx.gpr[8], 0, 0, 0}, {one, f2, f3});
    };
    auto world = []() { return load<u32>(kCurrentWorld); };
    auto set_pause_bits = [&]() {
        const u32 w = world();
        store<u32>(w + kWorldFlags40, load<u32>(w + kWorldFlags40) | 2);
        const u32 w2 = world();
        store<u32>(w2 + kWorldFlags40, load<u32>(w2 + kWorldFlags40) | 4);
    };

    enum class Tail { Effects, Refresh };
    Tail tail = Tail::Effects;

    if (mode == 1) {
        say(load<u32>(c + kCoSpeaker), load<u32>(c + kCoText220), 0, true, zero, zero);
        flag30 = true;
        if (sub_mode(c) != 2) {
            call(ctx, ffn::Effect_Bind4A63FC, {c + 192, load<u32>(c + kCoEffName152), 0, load<u32>(c + kCoOwner), load<u32>(c + kCoSpeaker), 1});
            if (const u32 e = load<u32>(kEffectHub); e != 0) call(ctx, ffn::Effect_SetHash4CF988, {e, 0x03F10EB8});
        }
        sound_play(ctx, sp + kSpSound192, load<u32>(c + kCoSound2264));
        const u32 page = c + kCoPages + (load<u32>(c + kCoPageIndex) << 4);
        call(ctx, ffn::Coach_Show2A1290, {c, load<u32>(world() + kWorldPages), page});
    } else if (mode == 2) {
        flag30 = true;
        say(load<u32>(c + kCoSpeaker), load<u32>(c + kCoText232), 0, true, zero, zero);
    } else if (mode == 3) {
        flag30 = true;
        if (const u32 x = load<u32>(c + kCoWidget688); x != 0) vc(x, 88, {x, 1});
        if (const u32 p = load<u32>(c + kCoPanel176); p != 0 && load<u32>(p + kPanelChild) != 0) {
            vc(p, 88, {p, 1});
            const u32 ch = load<u32>(load<u32>(c + kCoPanel176) + kPanelChild);
            vc(ch, 88, {ch, 1});
        }
        if (const u32 x = load<u32>(c + kCoWidget948); x != 0) {
            const u32 opd = load<u32>(load<u32>(x) + 88);
            vcall_r56(ctx, x, 88, {x, sub_mode(c) == 0 ? 1u : 0u}, load<u32>(opd), opd);
            if (load<s32>(c + kCoStyle) == 1) {
                const u32 y = load<u32>(c + kCoWidget948);
                vc(y, 52, {sp + kSpState244, y});
                // Alpha from +964: clamp(b / 255, ..) * 255 (fsel pair), fctiwz.
                const f64 f1 = lfs_f64(kConst + 16);
                f64 f2 = static_cast<f32>(static_cast<f64>(load<u8>(c + kCoAlpha964)));
                const f64 f3 = load<f64>(kConst + 24), f4 = load<f64>(kConst + 32);
                f2 = static_cast<f32>(f2 / f1);
                const f64 f5 = static_cast<f32>(f1 - f2);
                f2 = static_cast<f32>(f5 >= 0.0 ? f2 : f3);
                f2 = static_cast<f32>(f2 >= 0.0 ? f2 : f4);
                const f64 a = fmuls(f2, f1);
                const u32 alpha = static_cast<u32>(rt::fctiw(a, true));
                ctx.fpr[1] = std::bit_cast<f64>(0xFFF8000000000000ull | alpha);  // fctiwz result left in f1
                store<u8>(sp + kSpState244 + 3, static_cast<u8>(alpha));
                const u32 packed = (static_cast<u32>(load<u8>(sp + kSpState244)) << 24) | (static_cast<u32>(load<u8>(sp + kSpState244 + 1)) << 16) |
                                   (static_cast<u32>(load<u8>(sp + kSpState244 + 2)) << 8) | (alpha & 0xFF);
                const u32 y2 = load<u32>(c + kCoWidget948);
                const u32 opd2 = load<u32>(load<u32>(y2) + 56);
                vcall_r56(ctx, y2, 56, {y2, packed}, load<u32>(opd2), opd2);
                const u32 y3 = load<u32>(c + kCoWidget948);
                vc(y3, 188, {y3, kTint948, 0});
            } else {
                const u32 y = load<u32>(c + kCoWidget948);
                vc(y, 188, {y, kEmptyName, 0});
            }
        }
        if (const u32 x = load<u32>(c + kCoWidget952); x != 0) {
            const u32 opd = load<u32>(load<u32>(x) + 88);
            vcall_r56(ctx, x, 88, {x, sub_mode(c) == 0 ? 1u : 0u}, load<u32>(opd), opd);
            const u32 y = load<u32>(c + kCoWidget952);
            vc(y, 188, {y, load<s32>(c + kCoStyle) == 1 ? kTint952 : kEmptyName, 0});
        }
        for (u32 k = 0; k < 24; k += 4) store<u32>(sp + kSpQuery + k, 0);
        call(ctx, ffn::Query_Init465218, {sp + kSpQuery, load<u32>(c + kCoObject)});
        call(ctx, ffn::World_Query482D14, {world(), 0xFFFFFFFFu, sp + kSpQuery});
        const u32 text = sub_mode(c) != 0 ? kCoText236 : kCoText224;
        say(load<u32>(c + kCoSpeaker), load<u32>(c + text), 1, false, zero, zero);
    } else if (mode == 4) {
        if (const u32 o = load<u32>(c + kCoObject); o != 0 && (load<u32>(o + kObjFlags192) & kObjUsable) != 0) vc(o, 88, {o, 1, 1, 1, 1});
        say(load<u32>(c + kCoSpeaker), load<u32>(c + kCoText240), 0, true, zero, zero);
    } else if (mode == 5) {
        say(load<u32>(c + kCoSpeaker), load<u32>(c + kCoText228), 0, true, zero, zero);
        sound_play(ctx, sp + kSpSound196, load<u32>(c + kCoSound2268));
    } else if (mode == 10 || mode == 6) {
        target = true;
        set_pause_bits();
        if (const u32 x = load<u32>(c + kCoWidget56); x != 0) {
            vc(x, 40, {x, 1, 1});
            say(load<u32>(c + kCoSpeaker60), load<u32>(c + kCoText2292), 1, true, zero, zero);
            sound_play(ctx, sp + (mode == 10 ? kSpSound204 : kSpSound200), load<u32>(c + kCoSound2284));
        }
        if (mode == 10) {
            const u32 f = load<u32>(c + kCoFlags);
            const u32 mood = (f & 0x100) != 0 ? 0u : (f & 0x80) != 0 ? 1u : (f & 0x200) != 0 ? 2u : 3u;
            store<u32>(c + kCoMood, mood);
            store<u32>(c + kCoMoodScale, 0x3F800000u);  // 1.0f
        } else {
            call(ctx, ffn::World_Pause48A01C, {world(), 0});
        }
        tail = Tail::Refresh;
    } else if (mode == 7 || mode == 8 || mode == 9) {
        bool effects = true;  // r27
        if (mode == 7) {
            const u32 icon = load<u32>(c + kCoIcon712);
            const u32 hud = load<u32>(kIconHud);
            const u32 tex = icon != 0 && hud != 0 ? load<u32>(kIconTexture) : 0u;
            if (icon != 0 && hud != 0 && tex != 0) {
                // The icon's uv rectangle, from the HUD's u16 width / height
                // (+52 / +54), set up once.
                const f64 w = static_cast<f32>(static_cast<f64>(load<u16>(hud + 52)));
                const f64 h = static_cast<f32>(static_cast<f64>(load<u16>(hud + 54)));
                f64 f1 = w;
                const f64 u0 = fmuls(static_cast<f32>(static_cast<f32>(w - h) / w), lfs_f64(kConst + 12));
                if (load<u8>(kUvGuard0) == 0) {
                    store<f32>(kUv0, static_cast<f32>(u0));
                    store<u8>(kUvGuard0, 1);
                    store<u32>(kUv0 + 4, 0);
                }
                if (load<u8>(kUvGuard1) == 0) {
                    f1 = static_cast<f32>(h / w);
                    store<u32>(kUv1 + 4, load<u32>(kIconScale));
                    store<u8>(kUvGuard1, 1);
                    store<f32>(kUv1, static_cast<f32>(f1));
                }
                ctx.fpr[1] = f1;
                call(ctx, ffn::UiElem_SetTexture, {load<u32>(c + kCoIcon712), tex, kUv0, kUv1});
                call(ctx, ffn::Hud_SetIcon4645A0, {kHud10B19E0, 1, load<u32>(kIconHud) + 56});
            }
            call(ctx, ffn::World_Resume48A068, {world(), 0});
            effects = false;
        }
        if (mode != 8) {
            if (const u32 x = load<u32>(c + kCoWidget708); x != 0)
                store<u32>(c + kCoBinding, static_cast<u32>(call(ctx, ffn::Obj_Attach4E2DB0, {load<u32>(c + (mode == 7 ? kCoSrc916 : kCoSrc920)), x, 0, 0})));
        }
        target = true;
        if (const u32 x = load<u32>(c + kCoWidget708); x != 0) vc(x, 88, {x, 1});
        if (!effects) tail = Tail::Refresh;
    } else if (mode == 0) {
        const u32 pages = load<u32>(world() + kWorldPages);
        const u32 panel = load<u32>(pages + kPagesPanel) + 4;
        const u32 a = load<u32>(panel), b = load<u32>(panel + 4);
        vc(a, 8, {a, 0});
        vc(b, 8, {b, 0});
        if (const u32 e = load<u32>(kEffectHub); e != 0) call(ctx, ffn::Effect_SetHash4CF988, {e, 0x0020B11E});
        // Unlink c from the node list.
        const u32 prev = load<u32>(c + kCoPrev);
        store<u32>(c + kCoFlags, load<u32>(c + kCoFlags) & ~kFlag15);
        if (prev != 0) store<u32>(prev + kCoNext, load<u32>(c + kCoNext));
        else store<u32>(kNodeList, load<u32>(c + kCoNext));
        const u32 o = load<u32>(c + kCoObject);
        if (o != 0 && (load<u32>(o + kObjFlags192) & kObjUsable) != 0) {
            store<u64>(o + kObjFlags184, load<u64>(o + kObjFlags184) & ~0x100ull);
            vc(o, 88, {o, 1, 1, 1, 1});
            vc(o, 128, {o, 0x0A6FCB42, 1});
            if (const u32 fx = static_cast<u32>(vc(o, 272, {o})); fx != 0) call(ctx, ffn::Effect_SetHash4CF988, {fx, 0x99A2098A});
            call(ctx, ffn::Obj_Reset12E5E8, {o});
            if (const u32 pl = static_cast<u32>(call(ctx, ffn::Obj_Notify24, {o})); pl != 0) {
                const u32 f = load<u32>(c + kCoFlags);
                if ((f & kFlag13) != 0) {
                    vc(pl, 44, {pl, (f & kFlag14) != 0 ? 1u : 0u});
                    store<u32>(c + kCoFlags, load<u32>(c + kCoFlags) & ~kFlag13);
                }
                store<u8>(pl + kPlFlags2338, static_cast<u8>(load<u8>(pl + kPlFlags2338) | 0x40));
            }
            u32 t = load<u32>(c + kCoTarget);
            if (t != 0) {
                if (const u32 cam = load<u32>(world() + kWorldCamera); cam != 0) {
                    callf(ctx, ffn::Obj_PopLayer, {cam, t}, {lfs_f64(kConst + 40)});
                    t = load<u32>(c + kCoTarget);
                }
                if (t != 0) {
                    vc(t, 4, {t});
                    store<u32>(c + kCoTarget, 0);
                }
            }
        }
        for (const u32 off : kCoEffects)
            call(ctx, ffn::Effect_Bind4A6558, {c + off, kEmptyName, kEmptyName, load<u32>(c + kCoOwner), load<u32>(c + kCoSpeaker), 1});
        for (const u32 off : kCoEmitters) call(ctx, ffn::Emitter_Stop, {load<u32>(c + off), 1});
        store<u32>(c + kCoFlags, load<u32>(c + kCoFlags) & ~kFlag16);
        for (u32 k = 0; k < 3; ++k) {
            const u32 w = load<u32>(c + kCoReadouts[k][0]);
            if (w == 0) continue;
            const u32 v = static_cast<u32>(vc(w, 24, {w}));
            const u32 vec = sp + kSpRead + 16 * k;
            store<u32>(vec, load<u32>(v));
            store<u32>(vec + 4, load<u32>(v + 4));
            store<u32>(vec + 4, load<u32>(c + kCoReadouts[k][1]));  // y replaced by the saved readout
            store<u32>(vec + 8, load<u32>(v + 8));
            ctx.fpr[1] = lfs_f64(v + 8);
            const u32 w2 = load<u32>(c + kCoReadouts[k][0]);
            vc(w2, 28, {w2, vec});
        }
        if (const u32 x = load<u32>(c + kCoWidget948); x != 0) {
            vc(x, 52, {sp + kSpState240, x});
            const u32 y = load<u32>(c + kCoWidget948);
            const u8 a964 = load<u8>(c + kCoAlpha964);
            const u32 packed = (static_cast<u32>(load<u8>(sp + kSpState240)) << 24) | (static_cast<u32>(load<u8>(sp + kSpState240 + 1)) << 16) |
                               (static_cast<u32>(load<u8>(sp + kSpState240 + 2)) << 8) | a964;
            const u32 opd = load<u32>(load<u32>(y) + 56);
            ctx.gpr[7] = load<u8>(sp + kSpState240 + 1);
            vcall_r56(ctx, y, 56, {y, packed}, a964, opd);
        }
        for (u32 k = 0; k < 2; ++k) call(ctx, ffn::Effect_Release36A504, {c + kCoEffSlots + kCoEffSlotStride * k});
        store<u32>(c + kCoFlags, load<u32>(c + kCoFlags) & ~kFlag12);
        tail = Tail::Refresh;
    }

    if (tail == Tail::Refresh) {
        r3 = call(ctx, ffn::Coach_Refresh2A0F5C, {c});
    } else {
        // loc_002A17A0: the sub mode's effect, the two slot effects, +148
        // (flag30 with +244 bit 12), the +2340 widgets, the current entry.
        const u32 owner = load<u32>(c + kCoOwner);
        r3 = owner;
        if (owner != 0) {
            r3 = call(ctx, ffn::Effect_Spawn, {load<u32>(c + kCoEnterNames + 4 * sub_mode(c)), owner, load<u32>(c + kCoSpeaker), 1});
            if (static_cast<u32>(r3) != 0) {
                callf(ctx, ffn::Effect_SetTime, {static_cast<u32>(r3)}, {lfs_f64(kConst)});
                r3 = ctx.gpr[3];
            }
            const f64 t0 = lfs_f64(kConst);
            auto spawn = [&](u32 name) {
                r3 = call(ctx, ffn::Effect_Spawn, {name, load<u32>(c + kCoOwner), load<u32>(c + kCoSpeaker), 1});
                if (static_cast<u32>(r3) != 0) {
                    callf(ctx, ffn::Effect_SetTime, {static_cast<u32>(r3)}, {t0});
                    r3 = ctx.gpr[3];
                }
            };
            for (u32 k = 0; k < 2; ++k) {
                r3 = load<u32>(c + kCoEffSlots + kCoEffSlotStride * k);
                if (static_cast<u32>(r3) != 0) spawn(load<u32>(c + kCoEffNames + 4 * k));
            }
            if (flag30) {
                r3 = (load<u32>(c + kCoFlags) & kFlag12) != 0 ? 1u : 0u;
                if (static_cast<u32>(r3) != 0) spawn(load<u32>(c + kCoEffName148));
            }
            for (u32 k = 0; k < kCoNumWidgets2340; ++k) {
                const u32 w = load<u32>(c + kCoWidgets2340 + 16 * k);
                r3 = w != 0 ? vc(w, 40, {w, 0, 1}) : 0;
            }
            r3 = sub_mode(c);
            if (static_cast<u32>(r3) == 0) {
                r3 = call(ctx, ffn::Coach_Current2A11D8, {c});
                if (static_cast<u32>(r3) != 0) {
                    const u32 w = load<u32>(static_cast<u32>(r3) + 12);
                    r3 = w;
                    if (w != 0) r3 = vc(w, 40, {w, 1, 1});
                }
            }
        }
    }
    // loc_002A17AC
    if (target) {
        if (const u32 t = load<u32>(c + kCoTarget); t != 0) r3 = call(ctx, ffn::Target_Enable10D9B0, {t, 1});
    }
    ctx.gpr[3] = r3;
}
