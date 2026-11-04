// World_HandleEvent callees, part 4: 123..125 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent4d {
namespace dfn {
constexpr u32 sub_00475C28 = 0x00475C28;         // (r3 passed through) -> nonzero (name?)
constexpr u32 Save_AutoSaveGame = 0x000AEF98;    // (SaveSlot*, 4056) -> 1 when done
constexpr u32 Trophy_CountUnlocked = 0x000B2914;
constexpr u32 Trophy_GetValue200 = 0x000B28F0;
constexpr u32 SaveSlot_MarkSeen = 0x002E2874;    // (session, u32 hash)
constexpr u32 Handle_Call363A3C = 0x0036735C;    // (handle, name, 1, -, 1, 0, d; f32)
constexpr u32 Handle_ResolveBit20000 = 0x00365B28;
constexpr u32 StateMachine_SetState = 0x00464D1C;  // (sm, level, id)
constexpr u32 Player_Set3632C8 = 0x003632C8;     // (Player*, value, u8 which) (name?)
constexpr u32 Tex_Acquire = 0x00523B7C;          // (Texture*) -> material
constexpr u32 Prim_Create523AE0 = 0x00523AE0;    // (material, 3, 4, 0) -> prim (name?)
constexpr u32 Prim_Lock523A5C = 0x00523A5C;      // (prim) -> vertices (name?)
constexpr u32 Prim_Unlock523A98 = 0x00523A98;    // (prim) (name?)
constexpr u32 Prim_Destroy5239E8 = 0x005239E8;   // (prim) (name?)
}  // namespace dfn

constexpr u32 kSession = 0x00F64C20;             // u32 session: +36 level info, +40 player info (name?)
constexpr u32 kWorldActiveSlot = 4208;          // u8 active save slot (World +80 + n * 4056) (name?)
constexpr u32 kPlayerHandle = 0x00F64D4C;        // u32 handle (Handle_ResolveBit20000)

// Level headline (World_ShowHeadline).
constexpr u32 kWorldHeadlineKey = 0x128114;      // u32
constexpr u32 kWorldLevel = 0x128164;            // u32 level object: +5452 PtrVector*, +5456 text, +5508 handle
constexpr u32 kTuneHeadlineRepeat = 0x010B9CD8;  // u8 (0x010B9C90 + 72) (name?)
constexpr u32 kStrUnknownHeadline = 0x00AE9124;  // "<unknown headline string! Check level params for level!>"
constexpr u32 kHeadlineHandleDefault = 0x00F64CEC;  // u32 handle (name?)
constexpr u32 kHeadlineTime = 0x00482B18;        // f32 -1.0
constexpr u32 kVec1164CA4 = 0x01164CA4;

// The UI backdrop quad (src/ui/render.cpp).
constexpr u32 kBackdrop = 0x00F66854;            // u32 material, or 0
constexpr u32 kBackdropPrim = 0x00F6685C;        // u32 prim (name?)
constexpr u32 kBackdropColor = 0x010C2AD4;       // u8[4]
constexpr u32 kQuadUv = 0x00AEF460;              // f32[4][2]
constexpr u32 kQuadDepthBits = 0xC1A00000u;      // -20.0f
}  // namespace hevent4d
}  // namespace sly4

using namespace sly4::hevent4d;

namespace {
// Save_AutoSaveGame twice unless the first call is done (returns 1).
u64 AutoSaveTwice(PPUContext& ctx, u32 slot) {
    u64 r = call(ctx, dfn::Save_AutoSaveGame, {slot, kSaveSlotSize});
    if (static_cast<s32>(r) != 1) r = call(ctx, dfn::Save_AutoSaveGame, {slot, kSaveSlotSize});
    return r;
}
void MarkSeenName(PPUContext& ctx, u32 session, u32 name) {
    if (name == 0) return;
    const u32 h = static_cast<u32>(call(ctx, hfn::String_HashNoCase, {name}));
    call(ctx, dfn::SaveSlot_MarkSeen, {session, h});
}
}  // namespace

// u32 Save_CommitLevelProgress(x): with sub_00475C28(x) and a session with both
// level (+36) and player (+40) info, the current world's save data (+80) gets
// +464 [player +502] = 2, +40 = level +744, +44 = +48 = 0 and the u16 at
// +416 [level +822 * 4] has bits 0x18 set to 0x10; the level's names at +324,
// +453 and (when Trophy_GetValue200() == Trophy_CountUnlocked(), each called
// twice) +582 are marked seen. In every case the save is then started and
// polled once more unless it finished (Save_AutoSaveGame). Returns its
// result (name?)
DECOMP(0048888C, Save_CommitLevelProgress) {
    const u32 x = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, dfn::sub_00475C28, {x})) == 0) {
        AutoSaveTwice(ctx, load<u32>(kCurrentWorld) + kSaveData);
        return;
    }
    u32 ses = load<u32>(kSession);
    const u32 s = load<u32>(kCurrentWorld) + kSaveData;
    const u32 pinfo = load<u32>(ses + 40), linfo = load<u32>(ses + 36);
    if (pinfo != 0 && linfo != 0) {
        store<u8>(s + load<u8>(pinfo + 502) + 464, 2);
        const u32 bits = s + load<u8>(linfo + 822) * 4 + 416;
        ses = load<u32>(kSession);
        const u32 v = load<u16>(bits);
        store<u32>(s + 40, load<u32>(linfo + 744));
        store<u32>(s + 44, 0);
        store<u32>(s + 48, 0);
        store<u16>(bits, static_cast<u16>((v & ~0x18u) | 0x10u));
        if (load<u32>(ses + 36) + 324 != 0) {
            MarkSeenName(ctx, ses, load<u32>(ses + 36) + 324);
            ses = load<u32>(kSession);
        }
        MarkSeenName(ctx, ses, load<u32>(ses + 36) + 453);
        call(ctx, dfn::Trophy_CountUnlocked);
        call(ctx, dfn::Trophy_GetValue200);
        const s32 got = static_cast<s32>(call(ctx, dfn::Trophy_GetValue200));
        if (got == static_cast<s32>(call(ctx, dfn::Trophy_CountUnlocked))) {
            ses = load<u32>(kSession);
            MarkSeenName(ctx, ses, load<u32>(ses + 36) + 582);
        }
    }
    AutoSaveTwice(ctx, s);
}

// u32 World_ShowHeadline(World* w, u8 force): needs a headline key (+0x128114),
// a level (+0x128164) and w +29. The key is looked up (last match wins) in the
// active save slot's 64 entries {u32 key, u8 shown} at +3504 (a zero key ends
// the list); an entry already shown is only shown again with force or tuning
// 0x010B9CD8. It is marked shown and the level's headline text (+5456, or a
// warning string when empty; the player info's own text when the level's
// +5452 table of 40-byte {key, text...} entries has its +468 key) is played on
// the level's handle +5508 (or 0x00F64CEC's): Handle_Call363A3C(h, text, 1, -, 1,
// 0, 0x01164CA4; -1.0). Returns w when nothing is shown (name?)
DECOMP(00482B20, World_ShowHeadline) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 force = arg<u32>(ctx, 1) & 0xFF;
    const u32 key = load<u32>(w + kWorldHeadlineKey);
    if (key == 0 || load<u32>(w + kWorldLevel) == 0 || load<u8>(w + 29) == 0) return ret(ctx, w);
    const u32 slot = w + load<u8>(w + kWorldActiveSlot) * kSaveSlotSize;
    u32 idx = 0xFFFFFFFFu;
    bool found = false;
    for (u32 i = 0; i < 64; ++i) {
        const u32 k = load<u32>(slot + 3504 + i * 8);
        if (k == 0) break;
        if (k == key) {
            idx = i;
            found = true;
        }
    }
    if (!found) return ret(ctx, w);
    const u32 shown = slot + idx * 8 + 3508;
    if (load<u8>(shown) != 0 && load<u8>(kTuneHeadlineRepeat) == 0 && force == 0) return ret(ctx, w);
    store<u8>(shown, 1);
    const u32 lv = load<u32>(w + kWorldLevel);
    u32 text = lv + 5456;
    if (load<u8>(text) == 0) text = kStrUnknownHeadline;
    const u32 pinfo = load<u32>(load<u32>(kSession) + 40);
    if (pinfo != 0) {
        for (u32 i = 0;; ++i) {
            const u32 vec = load<u32>(lv + 5452);
            const u32 n = vec != 0 ? load<u16>(vec + 8) : 0u;
            if (i >= n) break;
            const u32 e = load<u32>(vec) + i * 40;
            if (load<s32>(e) == load<s32>(pinfo + 468)) {
                text = e + 4;
                break;
            }
        }
    }
    u32 h = load<u32>(lv + 5508);
    if (h == 0) h = load<u32>(kHeadlineHandleDefault);
    // r6 = the player info pointer, as the original leaves it (not read).
    callf(ctx, dfn::Handle_Call363A3C, {h, text, 1, pinfo, 1, 0, kVec1164CA4},
          {static_cast<f64>(load<f32>(kHeadlineTime))});
}

// bool World_StartPlayerAction(World* w, Object* src, a2, a3, a4, a5, a6, a7,
// u8 a8, u8 a9) [a8, a9 on the stack]: like Player_StartAction (hevent4_a.cpp)
// without the world check and the w +31 test: 0 unless the player resolves,
// is not in state 2 / 3 and its +24 has +164 bit 0x8000 clear. w +51 = w +30,
// w +30 = src +172 (low bit of src's u64 +184) or 0; level 1 goes to 7 and then
// 28 (each written straight to +49 while +50 is 1..11, otherwise
// StateMachine_SetState; w +47 = *w->+8 before each); then the player gets
// Player_Set3632C8(a2, 0), (a3, 1) and +400.. as there. Returns 1 (name?)
DECOMP(00489C08, World_StartPlayerAction) {
    const u32 w = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u32 a2 = arg<u32>(ctx, 2), a3 = arg<u32>(ctx, 3), a4 = arg<u32>(ctx, 4);
    const u32 a5 = arg<u32>(ctx, 5), a6 = arg<u32>(ctx, 6), a7 = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u8 a8 = static_cast<u8>(load<u32>(sp + 116));  // 9th argument (low word)
    const u8 a9 = static_cast<u8>(load<u32>(sp + 124));  // 10th argument (low word)
    const u32 p = static_cast<u32>(call(ctx, dfn::Handle_ResolveBit20000, {load<u32>(kPlayerHandle)}));
    if (p == 0) return ret(ctx, 0u);
    const s32 st = static_cast<s32>(load<u32>(p + 160) >> 1);
    if (st == 2 || st == 3) return ret(ctx, 0u);
    const u32 t = load<u32>(p + 24);
    if (t != 0 && (load<u32>(t + 164) & 0x8000u) != 0) return ret(ctx, 0u);
    store<u8>(w + 51, load<u8>(w + 30));
    u8 v = 0;
    if (src != 0 && (load<u64>(src + 184) & 1) != 0) v = load<u8>(src + 172);
    store<u8>(w + 30, v);
    u32 cur = load<u8>(w + 50);
    store<u8>(w + 47, load<u8>(load<u32>(w + 8)));
    if (cur == 0 || static_cast<s32>(cur) >= 12) {
        call(ctx, dfn::StateMachine_SetState, {w, 1, 7});
        cur = load<u8>(w + 50);
    } else {
        store<u8>(w + 49, 7);
    }
    store<u8>(w + 47, load<u8>(load<u32>(w + 8)));
    if (cur == 0 || static_cast<s32>(cur) >= 12) call(ctx, dfn::StateMachine_SetState, {w, 1, 28});
    else store<u8>(w + 49, 28);
    call(ctx, dfn::Player_Set3632C8, {p, a2, 0});
    call(ctx, dfn::Player_Set3632C8, {p, a3, 1});
    store<u32>(p + 400, a4);
    store<u32>(p + 408, a5);
    store<u32>(p + 404, a6);
    store<u32>(p + 412, a7);
    store<u32>(p + 416, src);
    store<u8>(p + 420, a8);
    store<u8>(p + 421, a9);
    ret(ctx, 1u);
}

// u32 Ui_SetBackdropColor(u32 rgba): stores the colour (bytes R, G, B, A). A
// zero alpha destroys the backdrop prim and material (vfunc +8); otherwise
// they are made on first use (Tex_Acquire(0), Prim_Create523AE0(m, 3, 4, 0)) and
// the 4 vertices refilled as a full-screen quad (kQuadUv times the display
// size, z -20, the colour) (name?)
DECOMP(004D5070, Ui_SetBackdropColor) {
    const u32 rgba = arg<u32>(ctx, 0);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    store<u32>(sp + 48, rgba);  // spilled to the caller's parameter save area and read back
    for (u32 k = 0; k < 4; ++k) store<u8>(kBackdropColor + k, load<u8>(sp + 48 + k));
    if (load<u8>(kBackdropColor + 3) == 0) {
        const u32 m = load<u32>(kBackdrop);
        if (m == 0) return ret(ctx, 0u);
        call(ctx, dfn::Prim_Destroy5239E8, {load<u32>(kBackdropPrim)});
        const u32 m2 = load<u32>(kBackdrop);
        store<u32>(kBackdropPrim, 0);
        vcall(ctx, m2, 8, {m2});
        store<u32>(kBackdrop, 0);
        return;
    }
    if (load<u32>(kBackdrop) == 0) {
        const u32 m = static_cast<u32>(call(ctx, dfn::Tex_Acquire, {0}));
        store<u32>(kBackdrop, m);
        store<u32>(kBackdropPrim, static_cast<u32>(call(ctx, dfn::Prim_Create523AE0, {m, 3, 4, 0})));
    }
    u32 v = static_cast<u32>(call(ctx, dfn::Prim_Lock523A5C, {load<u32>(kBackdropPrim)}));
    const f64 wd = static_cast<f32>(static_cast<f64>(load<u16>(kDisplayConfig + 4)));
    const f64 ht = static_cast<f32>(static_cast<f64>(load<u16>(kDisplayConfig + 6)));
    u8 c[4];
    for (u32 k = 0; k < 4; ++k) c[k] = load<u8>(kBackdropColor + k);
    for (u32 i = 0; i < 4; ++i, v += 24) {
        const u32 uv = kQuadUv + i * 8;
        store<u32>(v + 16, load<u32>(uv));
        store<u32>(v + 20, load<u32>(uv + 4));
        const f32 x = fmuls(load<f32>(uv), wd), y = fmuls(load<f32>(uv + 4), ht);
        store<u32>(v + 8, kQuadDepthBits);
        store<f32>(v + 0, x);
        store<f32>(v + 4, y);
        for (u32 k = 0; k < 4; ++k) store<u8>(v + 12 + k, c[k]);
    }
    call(ctx, dfn::Prim_Unlock523A98, {load<u32>(kBackdropPrim)});
}
