// Voice API wrappers ("src/common/api.cpp" in the sound library).
#include "audio.h"

namespace sly4::audio {
using namespace opentit::native;

// Returns the handle itself while its voice exists, else 0.  (name?)
u32 Voice_IsPlaying(u32 handle) {
    call(afn::Voice_Lock);
    u32 result = 0;
    if (call<u32>(afn::Voice_Lookup, handle) != 0) result = handle;
    call(afn::Voice_Unlock);
    return result;
}

// The voice's count (Handler_CountActive), or 0 without a voice.  (name?)
s32 Voice_GetValue(u32 handle) {
    call(afn::Voice_Lock);
    s32 result = 0;
    if (const u32 voice = call<u32>(afn::Voice_Lookup, handle)) result = call<s32>(afn::Voice_GetCount, voice);
    call(afn::Voice_Unlock);
    return result;
}

// Returns the handle if there was a voice to stop, else 0.  (name?)
u32 Voice_Stop(u32 handle, u32 value) {
    call(afn::Voice_Lock);
    u32 result = 0;
    if (const u32 voice = call<u32>(afn::Voice_Lookup, handle)) {
        call(afn::Voice_DoStop, voice, 1u, value, 0u);
        result = handle;
    }
    call(afn::Voice_Unlock);
    return result;
}

}  // namespace sly4::audio

NATIVE(006D5E10, sly4::audio::Voice_IsPlaying)
NATIVE(006D64AC, sly4::audio::Voice_GetValue)
NATIVE(006D60AC, sly4::audio::Voice_Stop)
