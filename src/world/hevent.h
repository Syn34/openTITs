// Out-of-line callees of World_HandleEvent (0x00491710) above 12
// instructions (src/world/hevent_*.cpp); see event.h for the handler's map.
// Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "event.h"

namespace sly4 {
namespace hevent {

namespace hfn {
constexpr u32 Emitter_Stop = 0x00443424;        // (handle, bool fade)
constexpr u32 PtrVector_Clear = 0x0054EC1C;     // (PtrVector*)
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 Material_Alloc = 0x004FF2E8;
constexpr u32 sub_002DDD24 = 0x002DDD24;        // () -> Object* (+744) (name?)
constexpr u32 sub_004CF988 = 0x004CF988;        // (Object*, u32)
constexpr u32 sub_000AE454 = 0x000AE454;        // (SaveSlot*, u8*) (name?)
constexpr u32 Player_Get = 0x00365C24;          // () -> Player* (38 calls in the handler) (name?)
constexpr u32 World_AddTimedCallback = 0x0046B5F8;  // (World*, u32 kind, OPD, arg, 0) (name?)
constexpr u32 sub_003697B0 = 0x003697B0;        // (u8 user, f32 ratio)
constexpr u32 Material_CopyFrom = 0x00509090;   // (Material* dst, const Material* src) (name?)
}  // namespace hfn

// Globals.
using wevent::kTrophy;
using wevent::kDisplayConfig;
constexpr u32 kCurrentWorld = 0x00F65CA0;      // u32 World*
constexpr u32 kEmitterHandle10AFEA4 = 0x010AFEA4;  // u32 emitter handle (name?)
constexpr u32 kHandleMap = 0x0106C228;         // SortedMap {u16 +2 count, +8 {key, Object*}[]}
constexpr u32 kHashedName = 0x0106C278;        // u32 hash, set byte at 0x0106C274 (name?)
constexpr u32 kHashedNameSet = 0x0106C274;     // u8
constexpr u32 kBusyFlag105CB80 = 0x0105CB80;   // u32: spun on with 300 us sleeps (name?)
constexpr u32 kScaleConst48CE98 = 0x0048CE98;  // f32 32768.0
constexpr u32 kTune10AECFC = 0x010AECFC;       // tuning object, value at +72 (name?)
constexpr u32 kCallbackOpdF35510 = 0x00F35510; // OPD given to World_AddTimedCallback (name?)
constexpr u32 kReset_F65F74 = 0x00F65F74;      // 4 words cleared together (name?)
constexpr u32 kList10C2480 = 0x010C2480;       // vector, u16 count at +8 (name?)

// World layout bits used here.
constexpr u32 kUserBlocks = 4208, kUserBlockSize = 0x20C80;
constexpr u32 kSaveData = 80, kSaveSlotSize = 4056;

// lv2 sys_timer_usleep.
constexpr u32 kSysTimerUsleep = 141;

// fmuls: single-precision product (the double product of two singles is exact).
inline f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }

// Virtual call; like the original sequence, the OPD and its entry address are
// left in the two registers after the arguments.
inline u64 vcall(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    ctx.gpr[3 + args.size()] = opd;
    ctx.gpr[4 + args.size()] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}

}  // namespace hevent
}  // namespace sly4
