// Small leaf functions: a strcpy wrapper, stubs, the NP manager status
// callback and a render-target parameter block setter.
#include "common.h"

namespace sly4::common {
using namespace opentit::native;

namespace {
// sceNpManagerRegisterCallback statuses (SceNpManagerStatus).
constexpr s32 SCE_NP_MANAGER_STATUS_OFFLINE = -1;
constexpr s32 SCE_NP_MANAGER_STATUS_ONLINE = 3;
constexpr u32 kNpOfflineFlag = 0x80;          // the g::sysutil_flags bit sly4.h calls kSysutilNpOffline
constexpr u32 kNpStatusCleared = 0x00F65C94;  // u32, zeroed when going offline (name?)

struct RtParams {
    be_u32 a;   // +0
    be_u32 b;   // +4
    be_f32 x;   // +8
    be_f32 y;   // +12
    be_u32 c;   // +16
};
}  // namespace

// strcpy (0x007239D8).
char* String_Assign(char* dst, const char* src) { return call<char*>(cmfn::strcpy, dst, src); }

bool Stub_True() { return true; }

// Calls Stub_True, returns 1 regardless.
bool Stub_CallTrue() {
    Stub_True();
    return true;
}

// The sceNpManagerRegisterCallback handler Game_Load installs: going
// offline raises sysutil flag 0x80 (which pauses the main loop), coming
// back online clears it.
void NpManager_OnStatus(s32 status) {
    be_u32& flags = global<be_u32>(g::sysutil_flags);
    if (status == SCE_NP_MANAGER_STATUS_OFFLINE) {
        const u32 f = flags;
        global<be_u32>(kNpStatusCleared) = 0;
        flags = f | kNpOfflineFlag;
    } else if (status == SCE_NP_MANAGER_STATUS_ONLINE) {
        flags = flags & ~kNpOfflineFlag;
    }
}

// RenderTarget_Create passes 2044, 2046, 16.0, 10.0, 240 (c in r8).
void RenderTarget_SetParams(RtParams* p, u32 a, u32 b, f32 x, f32 y, u32 c) {
    p->a = a;
    p->b = b;
    p->x = x;
    p->y = y;
    p->c = c;
}

}  // namespace sly4::common

NATIVE(00555F80, sly4::common::String_Assign)
NATIVE(0056DF78, sly4::common::Stub_True)
NATIVE(0055E284, sly4::common::Stub_CallTrue)
NATIVE(0049AF38, sly4::common::NpManager_OnStatus)
NATIVE(0053C9F8, sly4::common::RenderTarget_SetParams)
