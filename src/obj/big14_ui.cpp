// Game code, large-function sweep part 14: the hideout menu's job list.
#include "big14.h"

namespace sly4::obj {

namespace {
inline be_u32& W(u32 base, u32 off) { return gm<be_u32>(base + off); }
inline f64 F(u32 ea) { return LfsW(gm<be_u32>(ea)); }
inline void St(u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); }
inline void CopyW(u32 dst, u32 src) { gm<be_u32>(dst) = gm<be_u32>(src).get(); }
}  // namespace

namespace {
namespace h3bc {
constexpr u32 Anim_Restart = 0x002775A8;        // Anim_Restart2775A8
constexpr u32 Scene_HasEntry = 0x002E16E8;      // (scene, hash) -> bool
constexpr u32 Scene_FindEntry = 0x002E1900;     // (scene, hash) -> entry (its name first)
constexpr u32 Handle_Resolve = 0x003568E0;      // Handle_ResolveBit1(handle) -> object or 0
constexpr u32 Obj_Dispatch = 0x0036325C;        // Obj_Dispatch362(o, 0, 1, 0)
constexpr u32 Player_Set = 0x003632C8;          // Player_Set3632C8(o, text, which)
constexpr u32 Obj_SetAnim = 0x00363A3C;         // (o, name, now, -, b, c, colour*; f32)
constexpr u32 Handle_Notify = 0x0036742C;       // Handle_Notify36742C(handle, a, b, c)
constexpr u32 World_Colour = 0x004883C0;        // (u8 out[4], world, name, rgba)
constexpr u32 Effect_FindComponent = 0x004BB4B4;
constexpr u32 Effect_SetHash = 0x004CF988;      // Effect_SetHash4CF988
constexpr u32 Display_GetSize = 0x004D4104;     // (f32 out[2])
constexpr u32 StringTable_Lookup = 0x004F87C4;  // (strings, id, fallback, quiet)
constexpr u32 View_ProjectPoint = 0x00522750;   // (view, out vec4, in vec3)
constexpr u32 View_ScreenToWorld = 0x00522838;  // (view, out[3], const p[4])
constexpr u32 String_Format = 0x005534D0;       // (fmt, ...) -> rotating static buffer
constexpr u32 String_CopyN = 0x00553638;        // (dst, src, n)
constexpr u32 String_FormatN = 0x00555CF4;      // (buf, size, fmt, ...)
constexpr u32 Hash_ToString = 0x0056A6E0;
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 WorldObj_FindByHash = 0x005801DC; // (hash, 0) -> obj or 0
constexpr u32 Memset = 0x00726E54;

constexpr u32 kK = 0x003BCB10u;                 // .text f32 {1, 0.25, 0, -1}
constexpr u32 kSelHandle = 0x00F64CCCu;         // u32 handle of the "select" prompt
constexpr u32 kScene = 0x00F64C20u;             // u32 the current scene
constexpr u32 kStrings = 0x00F6687Cu;           // u32 the string table
constexpr u32 kPromptPos = 0x01164B40u;         // f32[3] default prompt position
constexpr u32 kPromptRgba = 0x01164CA0u;        // u32 prompt colour
constexpr u32 kPromptColour = 0x01164CA4u;      // u8[4] button colour
constexpr u32 kLanguage = 0x010C5A48u;          // s32 (12: no trailing "$HIDEOUT_SELECT %s.")
constexpr u32 kEmpty = 0x00ADF108u;             // ""
constexpr u32 sBack = 0x00ADF560u;              // "$HIDEOUT_BUTTON_BACK"
constexpr u32 sBuy = 0x00ADF578u;               // "$BUTTON_X $HIDEOUT_BUY"
constexpr u32 sQuoted = 0x00ADF594u;            // "STR_STRING_IN_QUOTES"
constexpr u32 sPct = 0x00ADF5ACu;               // "%s"
constexpr u32 sJob = 0x00ADF5B4u;               // "%s%s $HIDEOUT_JOB_AVAILABLE %s $HIDEOUT_SELECT"
constexpr u32 sJobSel = 0x00ADF5E4u;            // "%s%s $HIDEOUT_JOB_AVAILABLE %s $HIDEOUT_SELECT %s."
constexpr u32 sSelect = 0x00ADF61Cu;            // "$HIDEOUT_SELECT"
constexpr u32 sSelectPct = 0x00ADF62Cu;         // "$HIDEOUT_SELECT %s."
}  // namespace h3bc

// Obj_SetAnim(o, 0, 1, -, 0, 0, the button colour; -1) on a prompt whose
// +24 world object is missing or lacks bit 0x8000 of +164 (the float in
// the r6 slot, which the callee does not read).
void PromptButton(u32 o) {
    const u32 w = gm<be_u32>(o + 24);
    if (w != 0 && (gm<be_u32>(w + 164) & 0x8000u) != 0) return;
    call(h3bc::Obj_SetAnim, o, 0u, 1u, F(h3bc::kK + 12), 0u, 0u, h3bc::kPromptColour);
}

// The prompt colour: World_Colour(buf, world, name, rgba) with name = the
// entry's +80 object's +216 vfunc +16 (0 without one), copied to buf - 4,
// then Obj_SetAnim(o, text, 1, buf[3], 0, 0, buf - 4; -1).
void PromptText(u32 o, u32 text, u32 e80, u32 buf) {
    using namespace h3bc;
    const u32 world = gm<be_u32>(kWorld);
    u32 name = 0;
    if (e80 != 0) {
        const u32 x = gm<be_u32>(e80 + 216);
        name = vcall<u32>(host<u8>(x), 16);
    }
    call(World_Colour, buf, world, name, gm<be_u32>(kPromptRgba));
    for (u32 k = 0; k < 4; ++k) gm<u8>(buf - 4 + k) = gm<u8>(buf + k);
    call(Obj_SetAnim, o, text, 1u, gm<u8>(buf + 3), 0u, 0u, buf - 4, F(kK + 12));
}
}  // namespace

// u32 HideoutMenu_RefreshJobs3BCB20(Menu* m, s32 cur, s32 sel, s32 prev,
// s32 mode) (name?). Mode 1 (opening): m->180's vfunc +40 (0, 1), m's vfunc
// +28 (0, 10) and (1, 0), m->20 = {m->60->2596, m->1544, 3}, the menu
// animator restarts 0x7CDE2234 when m->1580's vfunc +60 returns one, the
// select handle is dispatched, two handles notified and the back / buy
// prompts get their texts (and the button colour unless bit 0x8000 of their
// object +164 is set). Then for each 128-byte entry e of m->104 (u16 count
// m->112) with job j = e->108:
// - mode 1: e->0 = 0xC20B1F0C and e->104 |= 0x80 when j->249 (modes
//   >= 2 and <= -1 change nothing);
// - mode 0 at index sel: the job's +240 effect hash is kept for the end;
//   m->32's 0xBE907220 component with type 5 and a +28 object moves the
//   camera target m->160 to the job's world object (j->224, or e->80's
//   position when missing; a missing object only logs its hash), m->144
//   to the component object's +64 and m->176 = 0 (0.25, plus the object's
//   vfunc +40 (1, 1) when the scene lacks the job, at index cur too); the
//   select prompt is placed at the projected job prompt position and gets
//   "%s%s $HIDEOUT_JOB_AVAILABLE %s ..." (job +69, name, quoted scene entry)
//   or "$HIDEOUT_SELECT" in the entry's colour;
// - mode 0: e->0 = 0x9CD94DEF at sel; else (prev == 0) 0xC181C0F0 at cur,
//   (prev != 0) 0xACC3FD87 for prev 1 and an available job (j->249),
//   which also calls m->1580's vfunc +60.
// At the end Effect_SetHash4CF988(m->32, hash) for a kept hash (no caller
// reads r3 or f1).
void HideoutMenu_RefreshJobs3BCB20(u32 th, s32 a4, s32 a5, s32 a6, s32 a7) {
    using namespace h3bc;
    const s32 mode = static_cast<s32>(a7);
    const u32 diff = static_cast<u32>(a7 - a6);
    GuestFrame frame(528);
    const u32 sp = frame.addr();   // offsets as the original's frame
    u32 keep = 0;
    if (mode == 1) {
        const u32 o180 = gm<be_u32>(th + 180);
        if (o180 != 0) vcall(host<u8>(o180), 40, 0, 1);
        const u32 st = gm<be_u32>(th + 20);
        vcall(host<u8>(th), 28, 0, 10);
        vcall(host<u8>(th), 28, 1, 0);
        gm<be_u32>(st + 4) = gm<be_u32>(th + 1544);
        const u32 t = gm<be_u32>(gm<be_u32>(th + 60) + 2596);
        gm<be_u32>(st + 8) = 3;
        gm<be_u32>(st) = t;
        u32 p = gm<be_u32>(th + 1580);
        if (p != 0 && vcall<u32>(host<u8>(p), 60) != 0) {
            p = gm<be_u32>(th + 1580);
            const u32 an = vcall<u32>(host<u8>(p), 60);
            // The floats sit in r7 / r8 and arguments 9 / 10 (the callee does not read them).
            call(Anim_Restart, an, 0x7CDE2234u, 0u, 0u, F(kK), F(kK + 4), 0u, 0u, F(kK + 8), F(kK + 8), 0u, ~0ull);
        }
        call(Obj_Dispatch, call<u32>(Handle_Resolve, gm<be_u32>(kSelHandle)), 0u, 1u, 0u);
        call(Handle_Notify, 0xD8535CE6u, 0, 1, 0);
        call(Handle_Notify, 0xE85E3DE6u, 0, 1, 0);
        u32 o = call<u32>(Handle_Resolve, 0xFC2DCDC9u);
        if (o != 0) {
            call(Player_Set, o, sBack, 0);
            PromptButton(o);
        }
        o = call<u32>(Handle_Resolve, 0x77C531F4u);
        if (o != 0) {
            call(Player_Set, o, sBuy, 0);
            PromptButton(o);
        }
    }

    // Mode 0 at index sel: the camera target follows the job (see above).
    auto focus = [&](u32 j, u32 e80, u32 fromCur) {
        keep = gm<be_u32>(j + 240);
        const u32 c = call<u32>(Effect_FindComponent, gm<be_u32>(th + 32), 0xBE907220u);
        if (c == 0 || vcall<u32>(host<u8>(c), 32) != 5 || gm<be_u32>(c + 28) == 0) return;
        const u32 pa = e80 + 96;
        u32 p0 = gm<be_u32>(pa + 48), p1 = gm<be_u32>(pa + 52), p2 = gm<be_u32>(pa + 56);
        const u32 wo = call<u32>(WorldObj_FindByHash, gm<be_u32>(j + 224), 0);
        if (wo != 0) {
            p0 = gm<be_u32>(wo + 128);
            p1 = gm<be_u32>(wo + 132);
            p2 = gm<be_u32>(wo + 136);
        } else {
            call(Hash_ToString, gm<be_u32>(j + 224));
        }
        gm<be_u32>(th + 160) = p0;
        gm<be_u32>(th + 164) = p1;
        gm<be_u32>(th + 168) = p2;
        const u32 src = gm<be_u32>(c + 28) + 64;
        CopyW(th + 144, src);
        CopyW(th + 148, src + 4);
        const u32 z = gm<be_u32>(src + 8);
        gm<be_u32>(th + 176) = 0;
        gm<be_u32>(th + 152) = z;
        if (fromCur != 0) return;
        gm<be_u32>(th + 176) = 0x3E800000u;
        const u32 sc = gm<be_u32>(kScene);
        if (call<u32>(Scene_HasEntry, sc, call<u32>(String_HashNoCase, j + 4)) != 0) return;
        const u32 x = gm<be_u32>(c + 28);
        vcall(host<u8>(x), 40, 1, 1);
    };

    // The select prompt's position: the default (or the 0x7B7009C8 world
    // object's) position projected by the first view and unprojected by the
    // second into the prompt object's translation (x, y; z kept).
    auto place = [&](u32 h) {
        const u32 v = sp + 144;
        const u32 d0 = gm<be_u32>(kPromptPos), d1 = gm<be_u32>(kPromptPos + 4);
        gm<be_u32>(v) = d0;
        const u32 d2 = gm<be_u32>(kPromptPos + 8);
        gm<be_u32>(v + 4) = d1;
        gm<be_u32>(v + 8) = d2;
        const u32 wo = call<u32>(WorldObj_FindByHash, 0x7B7009C8u, 0);
        if (wo != 0) {
            for (u32 k = 0; k < 3; ++k) CopyW(v + 4 * k, wo + 128 + 4 * k);
        }
        call(Display_GetSize, sp + 240);
        const u32 src = gm<be_u32>(h + 24) + 16;
        for (u32 k = 0; k < 4; ++k)
            for (u32 i = 0; i < 3; ++i) CopyW(sp + 176 + 16 * k + 4 * i, src + 16 * k + 4 * i);
        const u32 z = gm<be_u32>(sp + 232);
        call(View_ProjectPoint, gm<be_u32>(gm<be_u32>(kWorld) + 0x128168u), sp + 160, v);
        call(View_ScreenToWorld, gm<be_u32>(gm<be_u32>(kWorld) + 0x12816Cu), sp + 224, sp + 160);
        gm<be_u32>(sp + 232) = z;
        const u32 w = gm<be_u32>(h + 24);
        for (u32 k = 0; k < 4; ++k)
            for (u32 i = 0; i < 3; ++i) CopyW(w + 16 + 16 * k + 4 * i, sp + 176 + 16 * k + 4 * i);
        call(fn14::WorldObj_MarkDirty, w);
    };

    // Mode 0 at index sel: the select prompt.
    auto prompt = [&](u32 j, u32 e80) {
        const u32 h = call<u32>(Handle_Resolve, 0xE85E3DE6u);
        if (h != 0 && gm<be_u32>(h + 24) != 0) place(h);
        const u32 sel = call<u32>(Handle_Resolve, gm<be_u32>(kSelHandle));
        const u32 sc = gm<be_u32>(kScene);
        if (sel == 0) return;
        const u32 nm = j + 4;
        u32 disp = nm;
        if (j + 134 != 0 && gm<u8>(j + 134) != 0) disp = j + 134;
        if (call<u32>(Scene_HasEntry, sc, call<u32>(String_HashNoCase, nm)) == 0) {
            const u32 o = call<u32>(Handle_Resolve, gm<be_u32>(kSelHandle));
            u32 text = sSelect;
            if (gm<be_s32>(kLanguage) != 12) text = call<u32>(String_Format, sSelectPct, disp);
            PromptText(o, text, e80, sp + 268);
            return;
        }
        const u32 sc2 = gm<be_u32>(kScene);
        const u32 ent = call<u32>(Scene_FindEntry, sc2, call<u32>(String_HashNoCase, nm));
        const u32 buf = sp + 272;
        gm<u8>(buf) = gm<u8>(kEmpty);
        call(Memset, buf + 1, 0, 255);
        const u32 tbl = gm<be_u32>(kStrings);
        if (tbl == 0) {
            call(String_CopyN, buf, ent, 256);
        } else {
            const u32 fmt = call<u32>(StringTable_Lookup, tbl, sQuoted, sPct, 0);
            call(String_FormatN, buf, 256, fmt, ent);
        }
        if (gm<be_s32>(kLanguage) == 12) {
            const u32 o = call<u32>(Handle_Resolve, gm<be_u32>(kSelHandle));
            const u32 text = call<u32>(String_Format, sJob, j + 69, disp, buf);
            PromptText(o, text, e80, sp + 252);
        } else {
            const u32 o = call<u32>(Handle_Resolve, gm<be_u32>(kSelHandle));
            const u32 text = call<u32>(String_Format, sJobSel, j + 69, disp, buf, disp);
            PromptText(o, text, e80, sp + 260);
        }
    };

    for (u32 i = 0;; ++i) {
        if (i >= gm<be_u16>(th + 112)) break;
        const u32 e = gm<be_u32>(th + 104) + 128 * i;
        const u32 atSel = static_cast<u32>(a5 - i), atCur = static_cast<u32>(a4 - i);
        const u32 j = gm<be_u32>(e + 108), e80 = gm<be_u32>(e + 80);
        u32 hash = 0;
        if (mode > 0) {
            if (mode != 1 || gm<u8>(j + 249) == 0) continue;
            hash = 0xC20B1F0Cu;
        } else if (mode != 0) {
            continue;
        } else {
            if (atSel == 0) focus(j, e80, atCur);
            if (atSel == 0) hash = 0x9CD94DEFu;
            else if (diff == 0) hash = atCur == 0 ? 0xC181C0F0u : 0;
            else if (gm<u8>(j + 249) != 0 && static_cast<s32>(a6) == 1) hash = 0xACC3FD87u;
            if (atSel == 0) prompt(j, e80);
            if (gm<u8>(j + 249) != 0) {
                const u32 p = gm<be_u32>(th + 1580);
                if (p != 0) vcall(host<u8>(p), 60);
            }
        }
        if (hash != 0) {
            const u32 e2 = gm<be_u32>(th + 104) + 128 * i;
            const u8 b = gm<u8>(e2 + 104);
            gm<be_u32>(e2) = hash;
            gm<u8>(e2 + 104) = static_cast<u8>(b | 0x80u);
        }
    }
    if (keep != 0) {
        const u32 fx = gm<be_u32>(th + 32);
        if (fx != 0) call(Effect_SetHash, fx, keep);
    }
}

}  // namespace sly4::obj

NATIVE(003BCB20, sly4::obj::HideoutMenu_RefreshJobs3BCB20)
