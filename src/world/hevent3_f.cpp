// World_HandleEvent callees, part 3: 82 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3f {
namespace ffn {
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 SaveParams_Set = 0x000AE78C;   // (dst, a, 128, b, 128, c, 1024, d, [stack] 64) (name?)
constexpr u32 sys_ppu_thread_create = 0x00ABF144;  // import stub
constexpr u32 Anim_Play = 0x004A5C90;        // (Anim*, clip, 0; f32 blend)
constexpr u32 sub_004CF988 = 0x004CF988;     // (Object*, u32 tag)
constexpr u32 sub_00364C90 = 0x00364C90;     // (Object*; f32)
constexpr u32 sub_002E5394 = 0x002E5394;     // () -> s32 (reads no arguments)
constexpr u32 sub_002DABCC = 0x002DABCC;     // (Session*, key) -> Object*
constexpr u32 sub_00475C28 = 0x00475C28;     // (World*) -> bool
}  // namespace ffn

// The save threads (see SaveTaskPoll in hevent_g.cpp).
constexpr u32 kSaveTaskState = 0x00F9D1A8;   // s32: 0 idle, 1 running, 2 ok, 3 failed
constexpr u32 kSaveBusy = 0x00F64538;        // u8: a save thread is running (name?)
constexpr u32 kSaveTaskBuffer = 0x010127A0;
constexpr u32 kSaveParamA = 0x00F9D1C5, kSaveParamB = 0x00F9D245;  // 128 bytes each (name?)
constexpr u32 kSaveParamC = 0x00F9D2C5;      // 1024 bytes (name?)
constexpr u32 kSaveParamD = 0x00F9D6C5;      // 64 bytes (name?)
constexpr u32 kSaveTaskPrio = 1001, kSaveTaskStack = 16384;
constexpr u32 kSaveGameToListEntry = 0x00F30FD0, kSaveGameToListName = 0x00AC5FA8;  // "SaveGameToList"
constexpr u32 kAutoSaveGameEntry = 0x00F30FD8, kAutoSaveGameName = 0x00AC5FCC;      // "AutoSaveGame"

constexpr u32 kWorldHudLock = 0x128214;      // u16 (wupdate.h)
constexpr u32 kOne364D24 = 0x00364D24;       // f32 1.0
constexpr u32 kTag4296E4C = 0x04296E4C;      // as in Obj_EnterState1

constexpr u32 kSessionPtr = 0x00F64C20;      // u32 Session*

// The two save-thread starters: 0 starts the thread on a copy of `size` bytes
// of `data` (state 1, kSaveBusy = 1) and returns the state then, without data
// returns 3; 2 and 3 are returned once and reset (state 0, kSaveBusy = 0);
// others (1, negative) are returned as they are.
void SaveTaskStart(PPUContext& ctx, u32 entry, u32 name) {
    const u32 data = arg<u32>(ctx, 0), size = arg<u32>(ctx, 1);
    const s32 s = load<s32>(kSaveTaskState);
    ctx.gpr[4] = data;
    ctx.gpr[6] = size;
    if (s == 2 || s == 3) {
        store<u32>(kSaveTaskState, 0);
        store<u8>(kSaveBusy, 0);
        ctx.gpr[4] = 0x00FA0000u;
        ctx.gpr[5] = 0;
        ctx.gpr[6] = 0x00F60000u;
        return ret(ctx, s);
    }
    if (s != 0) {
        ctx.gpr[5] = static_cast<u32>(s);
        return ret(ctx, s);
    }
    if (data == 0) {
        ctx.gpr[5] = 0;
        return ret(ctx, 3);
    }
    store<u32>(kSaveTaskState, 1);
    store<u8>(kSaveBusy, 1);
    call(ctx, ffn::memmove, {kSaveTaskBuffer, data, size});
    call(ctx, ffn::SaveParams_Set,
         {kSaveTaskBuffer, kSaveParamA, 128, kSaveParamB, 128, kSaveParamC, 1024, kSaveParamD, 64});
    Stack frame(ctx, 16);  // sys_ppu_thread_t out at r1+128
    call(ctx, ffn::sys_ppu_thread_create, {frame.addr(), entry, 0, kSaveTaskPrio, kSaveTaskStack, 0, name});
    const s32 now = load<s32>(kSaveTaskState);
    ctx.gpr[5] = static_cast<u32>(now);
    ret(ctx, now);
}
}  // namespace hevent3f
}  // namespace sly4

using namespace sly4::hevent3f;

// s32 Save_SaveGameToList(const void* data, u32 size): the "SaveGameToList"
// thread starter / poll (see SaveTaskStart) (name?)
DECOMP(000AED78, Save_SaveGameToList) { SaveTaskStart(ctx, kSaveGameToListEntry, kSaveGameToListName); }

// s32 Save_AutoSaveGame(const void* data, u32 size): the same for "AutoSaveGame" (name?)
DECOMP(000AEF98, Save_AutoSaveGame) { SaveTaskStart(ctx, kAutoSaveGameEntry, kAutoSaveGameName); }

// u32 Obj_StartFromIdle(Object* o): unless the HUD is locked (World
// +0x128214) or o is not idle (state +160 >> 1 != 0): vfunc +20(o, 1),
// state 1, Anim_Play(+28, clip +180, 0; 1.0), sub_004CF988(+20, 0x04296E4C);
// then, with a reference object (+168 when +172 bit 0x80, else +164 -> +8 when
// its +192 has 0x2): +228 = +220 = 0, +240 = ref +348 / ref +344 and
// sub_00364C90(o) (name?)
DECOMP(00364D28, Obj_StartFromIdle) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 lock = load<u16>(load<u32>(kCurrentWorld) + kWorldHudLock);
    if (lock != 0) return ret(ctx, lock);
    const u32 st = load<u32>(o + 160) >> 1;
    if (st != 0) return ret(ctx, st);
    vcall(ctx, o, 20, {o, 1});
    const u32 w = (load<u32>(o + 160) & 1u) | (1u << 1);
    const u32 anim = load<u32>(o + 28);
    store<u32>(o + 160, w);
    ctx.gpr[6] = w;
    ctx.gpr[7] = kOne364D24;
    callf(ctx, ffn::Anim_Play, {anim, load<u32>(o + 180), 0}, {static_cast<f64>(load<f32>(kOne364D24))});
    const u32 p = load<u32>(o + 20);
    if (p != 0) call(ctx, ffn::sub_004CF988, {p, kTag4296E4C});
    u32 ref_ = 0;
    if (load<u8>(o + 172) & 0x80u) {
        ref_ = load<u32>(o + 168);
    } else {
        const u32 q = load<u32>(load<u32>(o + 164) + 8);
        if (q != 0 && (load<u32>(q + 192) & 0x2u) != 0) ref_ = q;
    }
    if (ref_ == 0) return ret(ctx, 0u);
    const f32 r = static_cast<f32>(static_cast<f64>(load<f32>(ref_ + 348)) / static_cast<f64>(load<f32>(ref_ + 344)));
    store<u32>(o + 228, 0);
    store<u32>(o + 220, 0);
    store<f32>(o + 240, r);
    callf(ctx, ffn::sub_00364C90, {o}, {static_cast<f64>(r)});
}

// Session* World_UpdateSaveIds(World* w): fills the SaveData's (+80) words
// +36..+48 from the session at 0x00F64C20. With sub_002E5394() == 1: the
// object sub_002DABCC(session, +340) gives {+340, obj +744, 0, 0} (all 0
// without it; returns obj +744 / 0). Otherwise: +36 = player +724, +40 =
// player +744 (player = session +36), +44 = session +40 -> +468 unless
// sub_00475C28(w, +40), +48 = session +72 -> +2560 unless that is set or the
// object missing; returns the session (name?)
DECOMP(00475E20, World_UpdateSaveIds) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 sd = w + kSaveData;
    if (static_cast<s32>(call(ctx, ffn::sub_002E5394, {load<u32>(kSessionPtr)})) == 1) {
        const u32 o = static_cast<u32>(call(ctx, ffn::sub_002DABCC, {load<u32>(kSessionPtr), load<u32>(sd + 340)}));
        if (o == 0) {
            store<u32>(sd + 36, 0);
            store<u32>(sd + 40, 0);
            store<u32>(sd + 44, 0);
            store<u32>(sd + 48, 0);
            return ret(ctx, 0u);
        }
        store<u32>(sd + 36, load<u32>(sd + 340));
        const u32 v = load<u32>(o + 744);
        store<u32>(sd + 44, 0);
        store<u32>(sd + 40, v);
        store<u32>(sd + 48, 0);
        ctx.gpr[4] = 0;
        return ret(ctx, v);
    }
    u32 p = load<u32>(load<u32>(kSessionPtr) + 36);
    store<u32>(sd + 36, p != 0 ? load<u32>(p + 724) : 0);
    p = load<u32>(load<u32>(kSessionPtr) + 36);
    const u32 v2 = p != 0 ? load<u32>(p + 744) : 0;
    store<u32>(sd + 40, v2);
    const u32 r = static_cast<u32>(call(ctx, ffn::sub_00475C28, {w, v2}));
    const u32 q = load<u32>(load<u32>(kSessionPtr) + 40);
    store<u32>(sd + 44, (r == 0 && q != 0) ? load<u32>(q + 468) : 0);
    const u32 sess = load<u32>(kSessionPtr);
    const u32 x = load<u32>(sess + 72);
    store<u32>(sd + 48, (x == 0 || r != 0) ? 0 : load<u32>(x + 2560));
    ctx.gpr[4] = q;
    ret(ctx, sess);
}
