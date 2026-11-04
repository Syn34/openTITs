// World_HandleEvent callees, part 4: 127..129 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent4e {
namespace efn {
constexpr u32 Anim_Play = 0x004A5C90;            // (Anim*, clip, 0; f32 blend)
constexpr u32 StateMachine_Construct = 0x0045CAA0;  // (this, levels, data size, u8 initial)
constexpr u32 StateMachine_SetState = 0x00464D1C;   // (sm, level, id)
constexpr u32 PtrVector_Construct = 0x005516E0;  // (PtrVector*)
constexpr u32 Ui_Find = 0x004D5544;              // (path, parent)
constexpr u32 Session_Check2E5414 = 0x002E5414;  // (session) -> value (name?)
constexpr u32 Save_Sync0AE3FC = 0x000AE3FC;      // (SaveSlot*, u8* slot byte) (name?)
constexpr u32 Session_Check2E53D4 = 0x002E53D4;  // () (name?)
constexpr u32 Session_Check2E5394 = 0x002E5394;  // () (name?)
constexpr u32 Level_GetKind2DA85C = 0x002DA85C;  // (level info) -> 2 (name?)
constexpr u32 Session_Set2E15BC = 0x002E15BC;    // (session, level +724) (name?)
constexpr u32 SubMachine_Construct = 0x0047C5D8; // (this, a, 0, 0, b)
constexpr u32 Sub_Init48F27C = 0x0048F27C;       // (sub-object +56) (name?)
constexpr u32 Sub_Set48FA54 = 0x0048FA54;        // (+56, u32 hash, 0) (name?)
constexpr u32 Obj_Init3DCB40 = 0x003DCB40;       // (Object*) (name?)
}  // namespace efn

constexpr u32 kAnimBlend36FACC = 0x0036FACC;     // f32 1.0
constexpr u32 kAnimFlagHash = 0x04296E4Cu;       // given to sub_004CF988 (name?)
constexpr u32 kSession = 0x00F64C20;             // u32 session: +36 level info, +40 player info (name?)
constexpr u32 kStrLetterbox = 0x00AE8AF8;        // "InGame.Cutscene.Letterbox"
constexpr u32 kWorldPlayerKey = 0x1285A8;        // u32: player info +468 (name?)
constexpr u32 kWorldActiveSlot = 4208;           // u8 (name?)
constexpr u32 kVtable787E74 = 0x00B206A8;        // vtable of the 0x00787E74 object (name?)
constexpr u32 kVtable3DCCB4 = 0x00B20630, kVtable3DCCB4Sub = 0x00B20694;  // (name?)
constexpr u32 kSubHash3DCCB4 = 0x93BE1B3Fu;
constexpr u32 kGlobalF65818 = 0x00F65818;        // u32 cleared by 0x003DCCB4 (name?)
}  // namespace hevent4e
}  // namespace sly4

using namespace sly4::hevent4e;

// u32 Obj_Activate36FAD0(Object* o, a): nothing in state 1 (+160 >> 1).
// Otherwise +204 = 0, state 1 (+160 = (+160 & 1) | 2); +24 gets vfunc
// +40(x, 1, 1); +176, +180, +184 vfunc +88(y, 1); when +180's vfunc +12 returns 3
// it gets vfunc +188(z, a, 0) and +148(z); the anim +28 plays clip [+188 + state *
// 4] (blend 1.0) and +20 gets sub_004CF988(+20, 0x04296E4C) (name?)
DECOMP(0036FAD0, Obj_Activate36FAD0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 st = load<u32>(o + 160);
    if ((st >> 1) == 1) return ret(ctx, st);
    store<u32>(o + 204, 0);
    store<u32>(o + 160, (st & 1u) | 2u);
    const u32 x = load<u32>(o + 24);
    if (x != 0) vcall(ctx, x, 40, {x, 1, 1});
    for (u32 off : {176u, 180u, 184u}) {
        const u32 y = load<u32>(o + off);
        if (y != 0) vcall(ctx, y, 88, {y, 1});
    }
    const u32 z = load<u32>(o + 180);
    if (z != 0 && static_cast<s32>(vcall(ctx, z, 12, {z})) == 3) {
        const u32 z1 = load<u32>(o + 180);
        vcall(ctx, z1, 188, {z1, a, 0});
        const u32 z2 = load<u32>(o + 180);
        vcall(ctx, z2, 148, {z2});
    }
    const u32 clip = load<u32>(o + (load<u32>(o + 160) >> 1) * 4 + 188);
    callf(ctx, efn::Anim_Play, {load<u32>(o + 28), clip, 0}, {static_cast<f64>(load<f32>(kAnimBlend36FACC))});
    const u32 q = load<u32>(o + 20);
    if (q != 0) call(ctx, hfn::sub_004CF988, {q, kAnimFlagHash});
}

// u32 Obj_Construct787E74(Object* o, a, b): a 4-level StateMachine (256 data
// bytes) with vtable 0x00B206A8; +52 flags: 0x80000000 and 0x20000000 set,
// 0x40000000 clear; +48 = a, +44 = b; the PtrVector at +2340 (+2346 = 4); +2288
// = +2296 = 1.0, +2292 = 0.6, +2184 = 1, bytes +2132, +2133, +2208, +2209 = 1,
// +28 |= 1, everything else listed below 0. Returns o + 2144 as the original's r3
// (name?)
DECOMP(00787E74, Obj_Construct787E74) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    call(ctx, efn::StateMachine_Construct, {o, 4, 256, 0});
    store<u32>(o + 52, ((load<u32>(o + 52) | 0x80000000u) & ~0x40000000u) | 0x20000000u);
    store<u32>(o + 48, a);
    store<u32>(o + 44, b);
    store<u32>(o + 40, 0);
    store<u32>(o + 36, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 28, 0);
    store<u32>(o + 0, kVtable787E74);
    for (u32 off = 56; off <= 72; off += 4) store<u32>(o + off, 0);
    store<u32>(o + 2128, 0);
    store<u32>(o + 2152, 0);
    store<u16>(o + 2154, 0);
    store<u32>(o + 2156, 0);
    store<u16>(o + 2158, 0);
    store<u32>(o + 2160, 0);
    store<u32>(o + 2164, 0);
    store<u32>(o + 2168, 0);
    store<u32>(o + 2176, 0);
    store<u32>(o + 2180, 0);
    store<u32>(o + 2184, 1);
    store<u8>(o + 2208, 1);
    store<u8>(o + 2209, 1);
    store<u32>(o + 2288, 0x3F800000u);  // 1.0
    store<u32>(o + 2292, 0x3F19999Au);  // 0.6
    store<u32>(o + 2296, 0x3F800000u);
    store<u32>(o + 2300, 0);
    store<u32>(o + 2304, 0);
    store<u32>(o + 2308, 0);
    store<u8>(o + 2325, 0);
    store<u8>(o + 2328, 0);
    call(ctx, efn::PtrVector_Construct, {o + 2340});
    store<u8>(o + 2368, 0);
    store<u16>(o + 2346, 4);
    store<u32>(o + 28, load<u32>(o + 28) | 1u);
    store<u32>(o + 2212, 0);
    store<u8>(o + 2132, 1);
    store<u32>(o + 2228, 0);
    store<u8>(o + 2133, 1);
    store<u32>(o + 2216, 0);
    store<u32>(o + 2232, 0);
    store<u32>(o + 2220, 0);
    store<u32>(o + 2236, 0);
    store<u32>(o + 2136, 0);
    store<u32>(o + 2224, 0);
    store<u32>(o + 2240, 0);
    store<u32>(o + 2140, 0);
    store<u8>(o + 2369, 0);
    store<u32>(o + 2144, 0);
    store<u32>(o + 2148, 0);
    ret(ctx, o + 2144);
}

// u32 World_BeginCutscene48EC44(World* w) (name?): hides the
// "InGame.Cutscene.Letterbox" element (vfunc +88(e, 0)); +0x1285A8 = the player
// info's +468 key (0 without one); k = Session_Check2E5414(session);
// Save_Sync0AE3FC(active save slot, &w +4136 [slot]). Unless both session checks
// are 0, there is level info and Level_GetKind2DA85C(it) == 2 (then
// Session_Set2E15BC(session, level +724) and nothing else), level 1 goes to 7 with
// the state data's +68 = 0 and +72 = k when k != 0, else to 2 (written straight
// to +49 while +50 is 1..11, otherwise StateMachine_SetState; w +47 = *w->+8
// first). Returns 0.
DECOMP(0048EC44, World_BeginCutscene48EC44) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 el = static_cast<u32>(call(ctx, efn::Ui_Find, {kStrLetterbox, 0}));
    if (el != 0) vcall(ctx, el, 88, {el, 0});
    const u32 pinfo = load<u32>(load<u32>(kSession) + 40);
    store<u32>(w + kWorldPlayerKey, pinfo != 0 ? load<u32>(pinfo + 468) : 0u);
    const u32 k = static_cast<u32>(call(ctx, efn::Session_Check2E5414, {load<u32>(kSession)}));
    const u32 slot = load<u8>(w + kWorldActiveSlot);
    call(ctx, efn::Save_Sync0AE3FC, {w + slot * kSaveSlotSize + kSaveData, w + slot + 4136});
    bool set_state = true;
    if (static_cast<u32>(call(ctx, efn::Session_Check2E53D4, {load<u32>(kSession)})) == 0 &&
        static_cast<u32>(call(ctx, efn::Session_Check2E5394, {load<u32>(kSession)})) == 0) {
        const u32 linfo = load<u32>(load<u32>(kSession) + 36);
        if (linfo != 0 && static_cast<s32>(call(ctx, efn::Level_GetKind2DA85C, {linfo})) == 2) {
            const u32 ses = load<u32>(kSession);
            call(ctx, efn::Session_Set2E15BC, {ses, load<u32>(load<u32>(ses + 36) + 724)});
            set_state = false;
        }
    }
    if (set_state) {
        const u32 cur = load<u8>(w + 50);
        store<u8>(w + 47, load<u8>(load<u32>(w + 8)));
        const bool direct = cur != 0 && static_cast<s32>(cur) < 12;
        if (k != 0) {
            if (direct) store<u8>(w + 49, 7);
            else call(ctx, efn::StateMachine_SetState, {w, 1, 7});
            const u32 d = load<u32>(w + 20) + load<u16>(w + 6);
            store<u32>(d + 68, 0);
            store<u32>(d + 72, k);
        } else {
            if (direct) store<u8>(w + 49, 2);
            else call(ctx, efn::StateMachine_SetState, {w, 1, 2});
        }
    }
    ret(ctx, 0u);
}

// u32 Obj_Construct3DCCB4(Object* o, a, const u32* p, b): SubMachine_Construct(o,
// a, 0, 0, b); Sub_Init48F27C(+56); vtables 0x00B20630 (+0) and 0x00B20694 (+56);
// +84 = *p, +92 = +96 = 1.0; PtrVectors at +120 (+126 = 4) and +320, +340 (+6
// = 16); +160 = &+148 (an empty list: +148 = 0, so the original's walk of it
// collecting into +164.. (count +244) never runs and +248 = 0); bytes +156 = 1,
// +309 = 1; the rest listed below 0; 0x00F65818 = 0; +28 |= 0x100;
// Sub_Set48FA54(+56, 0x93BE1B3F, 0); Obj_Init3DCB40(o) (name?)
DECOMP(003DCCB4, Obj_Construct3DCCB4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    call(ctx, efn::SubMachine_Construct, {o, a, 0, 0, b});
    const u32 sub = o + 56;
    call(ctx, efn::Sub_Init48F27C, {sub});
    store<u32>(o + 0, kVtable3DCCB4);
    store<u32>(sub, kVtable3DCCB4Sub);
    const u32 pv = load<u32>(p);
    store<u32>(o + 80, 0);
    store<u32>(o + 84, pv);
    store<u32>(o + 88, 0);
    store<u32>(o + 92, 0x3F800000u);  // 1.0
    store<u32>(o + 96, 0x3F800000u);
    store<u32>(o + 100, 0);
    store<u8>(o + 117, 0);
    call(ctx, efn::PtrVector_Construct, {o + 120});
    store<u8>(o + 141, 0);
    store<u32>(o + 148, 0);
    store<u32>(o + 152, 0);
    store<u16>(o + 126, 4);
    store<u32>(o + 160, o + 148);
    store<u8>(o + 156, 1);
    store<u16>(o + 244, 0);
    // The original walks the list at +148 (+12 links) here: every node with a
    // successor is appended to +164 [+244++], the last one goes to +248. +148
    // was just set to 0, so this only reads it back.
    u32 n = load<u32>(o + 148);
    for (u32 e = n; e != 0;) {
        const u32 nx = load<u32>(e + 12);
        if (nx == 0) {
            n = e;
            break;
        }
        const s16 c = static_cast<s16>(load<u16>(o + 244));
        store<u16>(o + 244, static_cast<u16>(c + 1));
        store<u32>(o + 164 + static_cast<u32>(static_cast<s32>(c)) * 4, e);
        e = nx;
    }
    store<u32>(o + 248, n);
    store<u8>(o + 156, 1);
    store<u8>(o + 252, 0);
    store<u8>(o + 308, 0);
    store<u8>(o + 309, 1);
    for (u32 k = 0; k < 2; ++k) {
        const u32 v = o + 320 + k * 20;
        call(ctx, efn::PtrVector_Construct, {v});
        store<u16>(v + 6, 16);
    }
    store<u32>(o + 64, 0);
    store<u32>(o + 68, 0);
    store<u32>(o + 72, 0);
    store<u32>(o + 76, 0);
    const u32 f = load<u32>(o + 28);
    store<u32>(kGlobalF65818, 0);
    store<u32>(o + 28, f | 0x100u);
    call(ctx, efn::Sub_Set48FA54, {sub, kSubHash3DCCB4, 0});
    call(ctx, efn::Obj_Init3DCB40, {o});
}
