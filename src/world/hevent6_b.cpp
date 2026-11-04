// World_HandleEvent callees, part 6: small leaves and wrappers (9..60
// instructions) named in the part-5 callee list.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent6b {
namespace bfn {
constexpr u32 Obj_FindComponent = 0x000D1288;   // (Object*, kind, 0) -> component
constexpr u32 Table_Find = 0x0074493C;          // (Table*, const u32* key, key) -> entry* (name?)
constexpr u32 Anim_Find34CC34 = 0x0034CC34;
constexpr u32 Track_Length353748 = 0x00353748;
constexpr u32 cellSysutilGetSystemParamInt = 0x00ABDAA4;  // import stub
constexpr u32 String_Format = 0x005534D0;
constexpr u32 FsOp_GetError = 0x00570C38;
constexpr u32 FsOp_ErrorName = 0x00570C98;
constexpr u32 FsOp_GetPath = 0x00570D80;
constexpr u32 Ui_RefreshText = 0x004D573C;
constexpr u32 Texture_Alloc = 0x0051D368;
constexpr u32 String_CopyN = 0x00553638;
constexpr u32 Surface_Init5370A4 = 0x005370A4;  // (Surface*, w, h, 1, format, u32* out) (name?)
constexpr u32 Target_Bind51B4C4 = 0x0051B4C4;   // (u32 from the init, u32, Texture*) -> u32 (name?)
}  // namespace bfn

constexpr u32 kZero35377C = 0x0035377C;      // f32 0.0
constexpr u32 kZero353744 = 0x00353744;      // f32 0.0
constexpr u32 kPhaseEps = 0x004A5D9C;        // f32 1e-5
constexpr u32 kPhaseOne = 0x004A5DA0;        // f32 1.0
constexpr u32 kCamera = 0x00F64A58;          // as main_view.cpp
constexpr u32 kSortTable = 0x01013B04;       // as hevent3_a.cpp
constexpr u32 kMap106C258 = 0x0106C258;      // as hevent_e.cpp: SortedMap {+2 u16 count, +8 {key, Object*}[]}
constexpr u32 kSystemParamLang = 0x111;      // CELL_SYSUTIL_SYSTEMPARAM_ID_LANG
constexpr u32 kFsOpNames = 0x00F88724;       // as file/fsop.h: { s32 kind; const char* name; } [7]
constexpr u32 kFsOpNameCount = 7;
constexpr u32 kFsErrNoEntry = 0x80010708;    // not reported
constexpr u32 kFmtUnknown = 0x00AF3950;      // "Unknown:0x%x"
constexpr u32 kFmtString = 0x00AEF484;       // "%s"
constexpr u32 kColorTargetName = 0x00AF1B30; // "Color-RenderTarget"
constexpr u32 kElementText = 3;              // element type with a text id at +200
}  // namespace hevent6b
}  // namespace sly4

using namespace sly4::hevent6b;

// f32 Track_Time353780(Track* t) (name?): +32 state ? state +8 : 0.0
// (r3 is left as the state or the constant's address, as the original does).
DECOMP(00353780, Track_Time353780) {
    const u32 s = load<u32>(arg<u32>(ctx, 0) + 32);
    if (s != 0) {
        ctx.gpr[3] = s;
        ret(ctx, load<f32>(s + 8));
    } else {
        ctx.gpr[3] = kZero35377C;
        ret(ctx, load<f32>(kZero35377C));
    }
}

// Object* Obj_Owner0D93FC(Object* o) (name?): Obj_FindComponent(o, 3, 0).
DECOMP(000D93FC, Obj_Owner0D93FC) {
    ret(ctx, static_cast<u32>(call(ctx, bfn::Obj_FindComponent, {arg<u32>(ctx, 0), 3, 0})));
}

// f32 Track_Length353748(Track* t) (name?): +32 state, its +40 clip, the
// clip's +12 data +12; 0.0 when the state or clip is missing (r3 as left).
DECOMP(00353748, Track_Length353748) {
    const u32 s = load<u32>(arg<u32>(ctx, 0) + 32);
    if (s != 0) {
        const u32 c = load<u32>(s + 40);
        if (c != 0) {
            const u32 d = load<u32>(c + 12);
            ctx.gpr[3] = d;
            ret(ctx, load<f32>(d + 12));
            return;
        }
    }
    ctx.gpr[3] = kZero353744;
    ret(ctx, load<f32>(kZero353744));
}

// bool Game_CanAct2AB3E0() (name?): false only when the camera (0x00F64A58)
// exists and bits 20..27 of its +244 word are 6..9.
DECOMP(002AB3E0, Game_CanAct2AB3E0) {
    const u32 cam = load<u32>(kCamera);
    if (cam == 0) {
        ret(ctx, 1u);
        return;
    }
    const s32 mode = static_cast<s32>((load<u32>(cam + 244) >> 20) & 0xFF);
    ret(ctx, (mode > 9 || mode <= 5) ? 1u : 0u);
}

// Profile* Save_Profile0C9FC8(SaveObj* o) (name?): 0 for no object, else the
// value of o's entry in the sorted table 0x01013B04 (unchecked).
DECOMP(000C9FC8, Save_Profile0C9FC8) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 16);  // key at r1+112
    store<u32>(frame.addr(), o);
    const u32 entry = static_cast<u32>(call(ctx, bfn::Table_Find, {kSortTable, frame.addr(), o}));
    ret(ctx, load<u32>(entry));
}

// bool Ui_IsBusy3695A0() (name?): some object in the map 0x0106C258 has a +24
// component whose +164 flags have bit 0x8000 set.
DECOMP(003695A0, Ui_IsBusy3695A0) {
    const s32 n = load<u16>(kMap106C258 + 2);
    u32 e = load<u32>(kMap106C258 + 8);
    for (s32 i = 0; i < n; ++i, e += 8) {
        const u32 c = load<u32>(load<u32>(e + 4) + 24);
        if (c != 0 && (load<u32>(c + 164) & 0x8000) != 0) {
            ret(ctx, 1u);
            return;
        }
    }
    ret(ctx, 0u);
}

// Track* Anim_Find34CC34(AnimCtl* a, s32 id) (name?): the track in the +92
// vector ({+0 Track**, +8 u16 count}) whose +8 id matches; id 0 with no match
// and more than one track -> the first track; else 0.
DECOMP(0034CC34, Anim_Find34CC34) {
    const u32 v = load<u32>(arg<u32>(ctx, 0) + 92);
    const u32 id = arg<u32>(ctx, 1);
    for (u32 i = 0; i < (v != 0 ? load<u16>(v + 8) : 0u); ++i) {
        const u32 t = load<u32>(load<u32>(v) + 4 * i);
        if (load<u32>(t + 8) == id) {
            ret(ctx, t);
            return;
        }
    }
    if (id == 0 && v != 0 && load<u16>(v + 8) > 1) {
        ret(ctx, load<u32>(load<u32>(v)));
        return;
    }
    ret(ctx, 0u);
}

// void System_SetLanguage(s32 language) (name?): for a language outside
// 0..12 and 16..19 the system language is read (cellSysutilGetSystemParamInt
// LANG into a stack local, then unused). r3 is the call's result or the
// argument, as the original leaves it.
DECOMP(0056BF84, System_SetLanguage) {
    const s32 lang = arg<s32>(ctx, 0);
    Stack frame(ctx, 16);  // r1+112, preset to 1
    store<u32>(frame.addr(), 1);
    const bool known = (lang >= 0 && lang <= 12) || (lang >= 16 && lang <= 19);
    if (known) return;
    call(ctx, bfn::cellSysutilGetSystemParamInt, {kSystemParamLang, frame.addr()});
}

// void Anim_SetPhase4A5DA8(AnimCtl* a, -, u8 scaled; f32 t) (name?): sets the
// current track's (Anim_Find(a, 0)) state +8 to t, or with scaled to
// length * t (1.0 when the length is not above 1e-5).
DECOMP(004A5DA8, Anim_SetPhase4A5DA8) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 scaled = arg<u32>(ctx, 2) & 0xFF;
    f64 t = farg(ctx, 0);
    if (a == 0) return;
    const u32 track = static_cast<u32>(call(ctx, bfn::Anim_Find34CC34, {a, 0}));
    if (track == 0) return;
    const u32 s = load<u32>(track + 32);
    if (s == 0) return;
    if (scaled != 0) {
        const f64 len = callf(ctx, bfn::Track_Length353748, {track}, {});
        const f64 eps = load<f32>(kPhaseEps);
        ctx.gpr[3] = kPhaseEps;  // r3 as the original leaves it
        t = len > eps ? static_cast<f64>(fmuls(len, t)) : static_cast<f64>(load<f32>(kPhaseOne));
        ret(ctx, t);  // f1 as the original leaves it
    }
    store<f32>(s + 8, static_cast<f32>(t));
}

// void FsOp_Wait5612B8(FsOp* op) (name?): with the +20 pending flag set,
// clears it and checks the error (FsOp_GetError; 0 and 0x80010708 are
// ignored): an operation kind (+4) missing from the name table is formatted
// as "Unknown:0x%x", then FsOp_GetPath and FsOp_ErrorName are called; their
// results are dropped (the report itself is compiled out).
DECOMP(005612B8, FsOp_Wait5612B8) {
    const u32 op = arg<u32>(ctx, 0);
    if (load<u8>(op + 20) == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u8>(op + 20, 0);
    const u32 err = static_cast<u32>(call(ctx, bfn::FsOp_GetError, {op}));
    if (err == 0 || err == kFsErrNoEntry) return;
    const u32 kind = load<u32>(op + 4);
    bool named = false;
    for (u32 i = 0; i < kFsOpNameCount && !named; ++i) named = load<u32>(kFsOpNames + 8 * i) == kind;
    if (!named) call(ctx, bfn::String_Format, {kFmtUnknown, static_cast<u64>(static_cast<s64>(static_cast<s32>(kind)))});
    call(ctx, bfn::FsOp_GetPath, {op});
    call(ctx, bfn::FsOp_ErrorName, {op});
}

// void Ui_RefreshText(Element* e) (name?): a text element (vfunc +12 type 3)
// with a +200 text id gets it re-set through vfunc +188 (e,
// String_Format("%s", id), 1); then every child (+152 first, +156 next).
DECOMP(004D573C, Ui_RefreshText) {
    const u32 e = arg<u32>(ctx, 0);
    if (static_cast<s32>(vcall(ctx, e, 12, {e})) == static_cast<s32>(kElementText)) {
        const u32 id = load<u32>(e + 200);
        ctx.gpr[3] = id;  // r3 as the original leaves it
        if (id != 0) {
            const u32 s = static_cast<u32>(call(ctx, bfn::String_Format, {kFmtString, id}));
            vcall(ctx, e, 188, {e, s, 1});
        }
    }
    for (u32 c = load<u32>(e + 152); c != 0; c = load<u32>(c + 156)) call(ctx, bfn::Ui_RefreshText, {c});
}

// Texture* ColorTarget_Create51E33C(u16 w, u16 h, u32 format, u32 b) (name?): a
// texture (Texture_Alloc, unchecked) named "Color-RenderTarget" (+16
// char[32], +48 hash), +52 / +54 width / height, its +56 surface made by
// 0x005370A4 and +108 = 0x0051B4C4(the value that wrote, b, texture).
DECOMP(0051E33C, ColorTarget_Create51E33C) {
    const u32 w = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 format = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const u32 t = static_cast<u32>(call(ctx, bfn::Texture_Alloc, {w, h, format, b}));
    call(ctx, bfn::String_CopyN, {t + 16, kColorTargetName, 32});
    store<u8>(t + 47, 0);
    store<u32>(t + 48, static_cast<u32>(call(ctx, hfn::String_HashNoCase, {t + 16})));
    store<u16>(t + 52, static_cast<u16>(w));
    store<u16>(t + 54, static_cast<u16>(h));
    Stack frame(ctx, 16);  // out at r1+112
    store<u32>(frame.addr(), 0);
    call(ctx, bfn::Surface_Init5370A4, {t + 56, w, h, 1, format, frame.addr()});
    store<u32>(t + 108, static_cast<u32>(call(ctx, bfn::Target_Bind51B4C4, {load<u32>(frame.addr()), b, t})));
    ret(ctx, t);
}
