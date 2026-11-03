// Small leaf functions: a strcpy wrapper, stubs, the NP manager status
// callback and a render-target parameter block setter.
#include "common.h"

using namespace sly4;
using namespace sly4::common;

namespace {
// sceNpManagerRegisterCallback statuses (SceNpManagerStatus).
constexpr s32 SCE_NP_MANAGER_STATUS_OFFLINE = -1;
constexpr s32 SCE_NP_MANAGER_STATUS_ONLINE = 3;
constexpr u32 kNpOfflineFlag = 0x80;          // the g::sysutil_flags bit sly4.h calls kSysutilNpOffline
constexpr u32 kNpStatusCleared = 0x00F65C94;  // u32, zeroed when going offline (name?)
}  // namespace

// char* String_Assign(char* dst, const char* src)
// strcpy (0x007239D8).
DECOMP(00555F80, String_Assign) { ret(ctx, call(ctx, cmfn::strcpy, {ctx.gpr[3], ctx.gpr[4]})); }

// bool Stub_True()  (name?)
DECOMP(0056DF78, Stub_True) { ret(ctx, 1u); }

// bool Stub_CallTrue()  (name?): calls Stub_True, returns 1 regardless
DECOMP(0055E284, Stub_CallTrue) {
    call(ctx, cmfn::Np_Stub1);
    ret(ctx, 1u);
}

// void NpManager_OnStatus(s32 status, ...)
// The sceNpManagerRegisterCallback handler Game_Load installs: going
// offline raises sysutil flag 0x80 (which pauses the main loop), coming
// back online clears it.
DECOMP(0049AF38, NpManager_OnStatus) {
    const s32 status = arg<s32>(ctx, 0);
    auto& flags = ref<be_u32>(g::sysutil_flags);
    if (status == SCE_NP_MANAGER_STATUS_OFFLINE) {
        const u32 f = flags;
        store<u32>(kNpStatusCleared, 0);
        flags = f | kNpOfflineFlag;
    } else if (status == SCE_NP_MANAGER_STATUS_ONLINE) {
        flags = flags & ~kNpOfflineFlag;
    }
}

// void RenderTarget_SetParams(Params* p, u32 a, u32 b, f32 x, f32 y, u32 c at r8)
// RenderTarget_Create passes 2044, 2046, 16.0, 10.0, 240.
DECOMP(0053C9F8, RenderTarget_SetParams) {
    const u32 p = arg<u32>(ctx, 0);
    field<u32>(p, 0) = arg<u32>(ctx, 1);
    field<u32>(p, 4) = arg<u32>(ctx, 2);
    field<f32>(p, 8) = static_cast<f32>(farg(ctx, 0));
    field<f32>(p, 12) = static_cast<f32>(farg(ctx, 1));
    field<u32>(p, 16) = arg<u32>(ctx, 5);
}
