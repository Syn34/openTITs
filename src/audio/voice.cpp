// Voice API wrappers ("src/common/api.cpp" in the sound library).
#include "audio.h"
#include "snd.h"

namespace sly4::audio {
using namespace opentit::native;

// Returns the handle itself while its voice exists, else 0.  (name?)
u32 Voice_IsPlaying(u32 handle) {
    snd::Snd_Lock();
    u32 result = 0;
    if (snd::Snd_Lookup(handle)) result = handle;
    snd::Snd_Unlock();
    return result;
}

// The voice's count (Handler_CountActive), or 0 without a voice.  (name?)
s32 Voice_GetValue(u32 handle) {
    snd::Snd_Lock();
    s32 result = 0;
    if (snd::Handler* voice = snd::Snd_Lookup(handle)) result = static_cast<s32>(snd::Handler_CountActive(voice));
    snd::Snd_Unlock();
    return result;
}

// Returns the handle if there was a voice to stop, else 0.  (name?)
u32 Voice_Stop(u32 handle, u32 value) {
    snd::Snd_Lock();
    u32 result = 0;
    if (snd::Handler* voice = snd::Snd_Lookup(handle)) {
        snd::Handler_Stop(voice, 1, value, 0);
        result = handle;
    }
    snd::Snd_Unlock();
    return result;
}

}  // namespace sly4::audio

NATIVE(006D5E10, sly4::audio::Voice_IsPlaying)
NATIVE(006D64AC, sly4::audio::Voice_GetValue)
NATIVE(006D60AC, sly4::audio::Voice_Stop)
