// World_HandleEvent callees, part 5: 171..172 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent5g {
namespace gfn {
constexpr u32 Obj_Detach0D72C0 = 0x000D72C0;    // (holder, Object*) (name?)
constexpr u32 Obj_Notify24 = 0x000D949C;        // (Object*) -> Object* or 0
constexpr u32 Obj_UpdateFlags2336 = 0x0029D838; // (Object*, 1)
constexpr u32 Obj_Dispatch362 = 0x0036325C;     // (Object*, 1, 1, 0)
constexpr u32 Handle_ResolveBit1 = 0x003568E0;  // (handle) -> Object* or 0
constexpr u32 Player_Set3632C8 = 0x003632C8;    // (Object*, value, u32 which)
constexpr u32 Handle_Call363A3C = 0x0036735C;   // (u32 id, const char* text, 1, -, 0, 0, u8* colour; f32 time)
constexpr u32 Treasure_Finish425980 = 0x00425980;  // (Treasure*) (name?)
constexpr u32 Obj_Owner0D93FC = 0x000D93FC;     // (Object*) -> Object* or 0, +40 bit 31 checked (name?)
constexpr u32 Obj_Notify26 = 0x000D94CC;        // (Object*) -> nonzero
constexpr u32 Camera_IsSplit = 0x002A6044;
constexpr u32 World_GetSaveObj160 = 0x002E29B0; // (Session*)
constexpr u32 Save_Profile0C9FC8 = 0x000C9FC8;  // () -> profile or 0, +976 u8 checked (name?)
}  // namespace gfn

constexpr u32 kTreasureActive = 0x80000000u;    // +164
constexpr u32 kTreasureFxTag = 0xE26489ECu;     // given to sub_004CF988 (name?)
constexpr u32 kZero425B9C = 0x00425B9C;         // f32 0.0, then 3.0
constexpr u32 kPlayerHandle2 = 0x00F64CCC;      // u32 handle (name?)
constexpr u32 kMsgTreasureLost = 0x0001447Bu;   // message id (name?)
constexpr u32 kStrTreasureLost = 0x00AE203C;    // "$STR_TREASURE_LOST"
constexpr u32 kEmptyStrAE2054 = 0x00AE2054;     // ""
constexpr u32 kMsgColour = 0x01164CA4;          // u8[4] (name?)
constexpr u32 kObjBusy = 0x40000000u;           // +396

constexpr u32 kCurrentWorldPtr = 0x00F65CA0;
constexpr u32 kWorldHudLock = 0x128214;         // u16
constexpr u32 kSessionPtr = 0x00F64C20;
}  // namespace hevent5g
}  // namespace sly4

using namespace sly4::hevent5g;

// void Treasure_Release425BA8(Treasure* t, u8 lost) (name?): only while +164
// bit 31 is set (cleared here). The carried object (+136): with lost, the fx
// +156 gets sub_004CF988(fx, 0xE26489EC) and the object vfunc +352(o, -, 0, 0;
// +344); detached from the holder (+132) by Obj_Detach0D72C0, then vfunc
// +40(o, 0) and +136 = 0. The holder: its Obj_Notify24 object loses +2338 bit
// 1 (Obj_UpdateFlags2336(n, 1)), vfunc +40(holder, 0), +132 = 0. With +140 >
// 0 the +152 object (when +396 bit 30) gets Obj_Dispatch362(o, 1, 1, 0). The
// player (handle 0x00F64CCC): with lost the "$STR_TREASURE_LOST" message for
// 3 s; when busy, Player_Set3632C8(p, "", 0) and Obj_Dispatch362(p, 1, 1, 0);
// the notify object gets +2337 |= 0x10. Then Treasure_Finish425980(t).
DECOMP(00425BA8, Treasure_Release425BA8) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 lost = arg<u32>(ctx, 1) & 0xFF;
    const u32 f = load<u32>(t + 164);
    if (!(f & kTreasureActive)) {
        ret(ctx, f);  // r3 as the original leaves it
        return;
    }
    store<u32>(t + 164, f & ~kTreasureActive);
    u32 a = load<u32>(t + 136);
    if (a != 0) {
        if (lost != 0) {
            const u32 fx = load<u32>(t + 156);
            if (fx != 0) {
                call(ctx, hfn::sub_004CF988, {fx, kTreasureFxTag});
                a = load<u32>(t + 136);
            }
            const u32 opd = load<u32>(load<u32>(a) + 352);
            ctx.gpr[7] = load<u32>(opd);  // the original passes the OPD in r4 and the entry in r7
            ctx.fpr[1] = load<f32>(a + 344);
            call_ptr(ctx, opd, {a, opd, 0, 0});
            a = load<u32>(t + 136);
        }
        const u32 h = load<u32>(t + 132);
        if (h != 0) {
            call(ctx, gfn::Obj_Detach0D72C0, {h, a});
            a = load<u32>(t + 136);
        }
        if (a != 0) {
            vcall(ctx, a, 40, {a, 0});
            store<u32>(t + 136, 0);
        }
    }
    u32 n = 0;
    u32 h = load<u32>(t + 132);
    if (h != 0) {
        n = static_cast<u32>(call(ctx, gfn::Obj_Notify24, {h}));
        h = load<u32>(t + 132);
    }
    if (h != 0) {
        if (n != 0) {
            store<u8>(n + 2338, load<u8>(n + 2338) & ~0x2u);
            call(ctx, gfn::Obj_UpdateFlags2336, {n, 1});
            h = load<u32>(t + 132);
        }
        vcall(ctx, h, 40, {h, 0});
        store<u32>(t + 132, 0);
    }
    if (load<f32>(t + 140) > load<f32>(kZero425B9C)) {
        const u32 o = load<u32>(t + 152);
        if (o != 0 && (load<u32>(o + 396) & kObjBusy)) call(ctx, gfn::Obj_Dispatch362, {o, 1, 1, 0});
    }
    const u32 p = static_cast<u32>(call(ctx, gfn::Handle_ResolveBit1, {load<u32>(kPlayerHandle2)}));
    if (p != 0) {
        if (lost != 0)
            callf(ctx, gfn::Handle_Call363A3C, {kMsgTreasureLost, kStrTreasureLost, 1, ctx.gpr[6], 0, 0, kMsgColour},
                  {load<f32>(kZero425B9C + 4)});
        if (load<u32>(p + 396) & kObjBusy) {
            call(ctx, gfn::Player_Set3632C8, {p, kEmptyStrAE2054, 0});
            call(ctx, gfn::Obj_Dispatch362, {p, 1, 1, 0});
        }
        if (n != 0) store<u8>(n + 2337, load<u8>(n + 2337) | 0x10);
    }
    call(ctx, gfn::Treasure_Finish425980, {t});
}

// bool Obj_CanInteract12B650(Object* o) (name?): 0 while the HUD is locked
// (World +0x128214), the owner (Obj_Owner0D93FC) has +40 bit 31, or
// Obj_Notify26(o) in split screen. Then by state (vfunc +32(o, 0)) and
// sub-state (vfunc +32(o, 1), read anew each time): 6 -> 1; 3 -> sub <= 1, 10
// or 2; 1 -> not (o +20) +48 bit 22, sub <= 1, and (+188 bit 5 clear, or the
// save profile's +976); 0 -> sub 0, 2 or 15; others 0.
DECOMP(0012B650, Obj_CanInteract12B650) {
    const u32 o = arg<u32>(ctx, 0);
    auto sub = [&](u32 which) { return static_cast<s32>(vcall(ctx, o, 32, {o, which})); };
    if (load<u16>(load<u32>(kCurrentWorldPtr) + kWorldHudLock) != 0) return ret(ctx, 0);
    const u32 ow = static_cast<u32>(call(ctx, gfn::Obj_Owner0D93FC, {o}));
    if (ow != 0 && (load<u32>(ow + 40) & 0x80000000u)) return ret(ctx, 0);
    if (call(ctx, gfn::Obj_Notify26, {o}) != 0 && call(ctx, gfn::Camera_IsSplit) != 0) return ret(ctx, 0);
    const s32 st = sub(0);
    if (st == 6) return ret(ctx, 1);
    if (st == 3) {
        if (sub(1) <= 1) return ret(ctx, 1);
        if (sub(1) == 10) return ret(ctx, 1);
        return ret(ctx, sub(1) == 2 ? 1 : 0);
    }
    if (st == 1) {
        if (load<u32>(load<u32>(o + 20) + 48) & 0x00400000u) return ret(ctx, 0);
        if (sub(1) > 1) return ret(ctx, 0);
        if (!(load<u32>(o + 188) & 0x20u)) return ret(ctx, 1);
        const u32 so = static_cast<u32>(call(ctx, gfn::World_GetSaveObj160, {load<u32>(kSessionPtr)}));
        const u32 pr = static_cast<u32>(call(ctx, gfn::Save_Profile0C9FC8, {so}));
        if (pr == 0 || load<u8>(pr + 976) == 0) return ret(ctx, 0);
        return ret(ctx, 1);
    }
    if (st == 0) {
        if (sub(1) == 0) return ret(ctx, 1);
        if (sub(1) == 2) return ret(ctx, 1);
        return ret(ctx, sub(1) == 15 ? 1 : 0);
    }
    ret(ctx, 0);
}
