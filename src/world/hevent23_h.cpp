// World_HandleEvent callees, part 23h: a user's control hand-over (called
// by World_SpawnUsers487670) and the HUD element rescale.
#include "hevent22.h"

#include <bit>

using namespace sly4;
using namespace sly4::hevent;

namespace {
// User_SetControl4865EC: World +0x1280F0 Object*[9] (one per user), the
// world part object (+0x128164: +0 vector {+0 data, +8 u16 count} of
// 60-byte spawn entries {+36 u32 point hash, +56 u8 AI}). Object +184 u64
// flags (bit 0x20 any point, bit 0 cleared at the end), +192 bit 0x08,
// +216 owner (+52 point hash), +380 / +384 controllers.
constexpr u32 kWorldUsers = 0x1280F0, kWorldPart = 0x128164;
constexpr u32 kAnyPoint = 0x14120;
constexpr u32 kEntrySize = 60, kEntryPoint = 36, kEntryAi = 56;
constexpr u32 kObjFlags184 = 184, kObjFlags192 = 192, kObjOwner = 216, kOwnerPoint = 52;
constexpr u32 kObjCtrl = 380, kObjCtrlSaved = 384;
constexpr u64 kFlagAnyPoint = 0x20, kFlagPending = 1;
constexpr u32 kFlagSpawnable = 0x08;
constexpr u32 kSlotEnable = 44;  // controller vtable: (c, u8 enable)
constexpr u32 kObjNotify24 = 0x000D949C, kObjCallK2 = 0x000D93D4, kCoachResetMode = 0x0029F26C;
constexpr u32 kObjAttachControlAI = 0x00128EE0;
}  // namespace

// u64 User_SetControl4865EC(World* w, u32 user, u8 enable) (name?): for
// user's object (none: nothing): its spawn point (owner +52, or 0x14120
// with flag 0x20; 0 without an owner) is looked up in the world part's
// 60-byte entries; a match on an object with +192 bit 0x08 either
// re-attaches the AI controller (entry +56 set: +380 vtable +44 (c, 0),
// Obj_AttachControlAI128EE0, +380 vtable +44 (c, 1)) or restores +380
// from +384 (then vtable +44 (c, 0)). Then the Obj_Notify24 controller
// gets vtable +44 (q, enable) and, when not enabling,
// Coach_ResetMode29F26C(q); the CallK2_000D93D4 one gets vtable +44 (r,
// enable); finally +184 bit 0 is cleared. Returns the new +184 dword.
DECOMP(004865EC, User_SetControl4865EC) {
    const u32 w = arg<u32>(ctx, 0), user = arg<u32>(ctx, 1);
    const u32 enable = arg<u8>(ctx, 2);
    const u32 p = load<u32>(w + ((user << 2) & ~3u) + kWorldUsers);
    if (p == 0) return;
    u32 key = kAnyPoint;
    const u32 part = load<u32>(w + kWorldPart);
    if ((load<u64>(p + kObjFlags184) & kFlagAnyPoint) == 0) {
        const u32 owner = load<u32>(p + kObjOwner);
        key = owner != 0 ? load<u32>(owner + kOwnerPoint) : 0;
    }
    u32 entry = 0;
    for (u32 i = 0, off = 0;; ++i, off += kEntrySize) {
        const u32 vec = load<u32>(part);
        const u32 n = vec != 0 ? load<u16>(vec + 8) : 0u;
        if (!(i < n)) break;
        const u32 data = load<u32>(vec);
        if (load<u32>(data + off + kEntryPoint) == key) {
            entry = data + i * kEntrySize;
            break;
        }
    }
    if (entry != 0 && (load<u32>(p + kObjFlags192) & kFlagSpawnable) != 0) {
        if (load<u8>(entry + kEntryAi) != 0) {
            const u32 c = load<u32>(p + kObjCtrl);
            if (c != 0) vcall(ctx, c, kSlotEnable, {c, 0});
            call(ctx, kObjAttachControlAI, {p});
            const u32 c2 = load<u32>(p + kObjCtrl);
            if (c2 != 0) vcall(ctx, c2, kSlotEnable, {c2, 1});
        } else {
            const u32 c = load<u32>(p + kObjCtrlSaved);
            store<u32>(p + kObjCtrl, c);
            if (c != 0) vcall(ctx, c, kSlotEnable, {c, 0});
        }
    }
    const u32 q = static_cast<u32>(call(ctx, kObjNotify24, {p}));
    if (q != 0) {
        vcall(ctx, q, kSlotEnable, {q, enable});
        if (enable == 0) call(ctx, kCoachResetMode, {q});
    }
    const u32 r = static_cast<u32>(call(ctx, kObjCallK2, {p}));
    if (r != 0) vcall(ctx, r, kSlotEnable, {r, enable});
    const u64 f = load<u64>(p + kObjFlags184) & ~kFlagPending;
    store<u64>(p + kObjFlags184, f);
    ctx.gpr[3] = f;
}

namespace {
// Hud_Rescale467610: 13 HUD elements {+0 const char* path, +4 u8 fit to
// the render target, +5 u8 use the tuning scale} at 0x00F65DF4; the scale
// tuning (0x010BBFE8 +72) and constants 720.0 / 1.0 at 0x00467608.
constexpr u32 kHudTable = 0x00F65DF4, kHudCount = 13, kHudEntrySize = 8;
constexpr u32 kHudFit = 4, kHudTuned = 5;
constexpr u32 kHudTuning = 0x010BBFE8, kTuningScale = 72;
constexpr u32 kHudK = 0x00467608;  // f32 720.0 (reference height), 1.0
constexpr u32 kUiFind = 0x004D5544, kDisplayGetSize = 0x004D4104;
constexpr u32 kRenderW = 28, kRenderH = 30, kDisplayW = 4, kDisplayH = 6;
constexpr u32 kSlotSetScale = 40, kSlotGetSize = 36, kSlotSetSize = 48, kSlotSetAnchor = 104, kSlotRefresh = 148;

f64 lfs(u32 ea) { return static_cast<f64>(std::bit_cast<f32>(load<u32>(ea))); }
}  // namespace

// void Hud_Rescale467610() (name?): for each of the 13 HUD elements found
// (Ui_Find(path, 0)): scale = display height * (tuning +72 or 1.0) / 720
// given as a vec3 to vtable +40; elements marked +4 also get their size
// set (vtable +48) to the display size times render / display size (config
// +28 / +4 and +30 / +6) over their current size (vtable +36, read twice)
// and vtable +104 (e, 0); all get vtable +148 (refresh).
DECOMP(00467610, Hud_Rescale467610) {
    const f64 ref = lfs(kHudK);
    Stack frame(ctx, 32);
    const u32 vec = frame.addr(), sz = frame.addr() + 16;
    for (u32 i = 0; i < kHudCount; ++i) {
        const u32 e = kHudTable + i * kHudEntrySize;
        const u32 ui = static_cast<u32>(call(ctx, kUiFind, {load<u32>(e), 0}));
        if (ui == 0) continue;
        call(ctx, kDisplayGetSize, {sz});
        const f64 h = lfs(sz + 4);
        const f64 k = load<u8>(e + kHudTuned) != 0 ? lfs(kHudTuning + kTuningScale) : lfs(kHudK + 4);
        const f32 s = static_cast<f32>(static_cast<f64>(static_cast<f32>(h * k)) / ref);
        store<f32>(vec + 0, s);
        store<f32>(vec + 4, s);
        store<f32>(vec + 8, s);
        ctx.fpr[1] = s;
        ctx.fpr[2] = h;
        vcall(ctx, ui, kSlotSetScale, {ui, vec});
        if (load<u8>(e + kHudFit) != 0) {
            const u32 cfg = kDisplayConfig;
            const f32 rx = static_cast<f32>(static_cast<f64>(static_cast<f32>(load<u16>(cfg + kRenderW))) / static_cast<f32>(load<u16>(cfg + kDisplayW)));
            const f32 ry = static_cast<f32>(static_cast<f64>(static_cast<f32>(load<u16>(cfg + kRenderH))) / static_cast<f32>(load<u16>(cfg + kDisplayH)));
            const f32 x = static_cast<f32>(lfs(sz + 0) * rx);
            store<f32>(sz + 0, x);
            const f32 y = static_cast<f32>(lfs(sz + 4) * ry);
            store<f32>(sz + 4, y);
            ctx.fpr[1] = y;
            const u32 cur = static_cast<u32>(vcall(ctx, ui, kSlotGetSize, {ui}));
            const f64 cw = lfs(cur);
            const u32 cur2 = static_cast<u32>(vcall(ctx, ui, kSlotGetSize, {ui}));
            const f32 nx = static_cast<f32>(lfs(sz + 0) / cw);
            const f32 ny = static_cast<f32>(lfs(sz + 4) / lfs(cur2 + 4));
            store<f32>(sz + 0, nx);
            store<f32>(sz + 4, ny);
            ctx.fpr[1] = nx;
            ctx.fpr[2] = ny;
            vcall(ctx, ui, kSlotSetSize, {ui, sz});
            vcall(ctx, ui, kSlotSetAnchor, {ui, 0});
        }
        vcall(ctx, ui, kSlotRefresh, {ui});
    }
}
