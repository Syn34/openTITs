// Voice API wrappers ("src/common/api.cpp" in the sound library).
#include "audio.h"

using namespace sly4;
using namespace sly4::audio;

// handle Voice_IsPlaying(handle)  (name?)
// Returns the handle itself while its voice exists, else 0.
DECOMP(006D5E10, Voice_IsPlaying) {
    const u64 handle = ctx.gpr[3];
    call(ctx, afn::Voice_Lock, {handle});  // r3 still holds the handle (unused)
    u64 result = 0;
    if (static_cast<u32>(call(ctx, afn::Voice_Lookup, {handle})) != 0) result = handle;
    call(ctx, afn::Voice_Unlock);
    ctx.gpr[3] = result;
}

// s32 Voice_GetValue(handle)  (name?)
DECOMP(006D64AC, Voice_GetValue) {
    const u64 handle = ctx.gpr[3];
    call(ctx, afn::Voice_Lock, {handle});
    u64 result = 0;
    const u64 voice = call(ctx, afn::Voice_Lookup, {handle});
    if (static_cast<u32>(voice) != 0) result = call(ctx, afn::Voice_GetCount, {voice});
    call(ctx, afn::Voice_Unlock);
    ctx.gpr[3] = result;
}

// handle Voice_Stop(handle, u32 arg)  (name?)
// Returns the handle if there was a voice to stop, else 0.
DECOMP(006D60AC, Voice_Stop) {
    const u64 handle = ctx.gpr[3], value = ctx.gpr[4];
    call(ctx, afn::Voice_Lock, {handle, value});
    u64 result = 0;
    const u64 voice = call(ctx, afn::Voice_Lookup, {handle});
    if (static_cast<u32>(voice) != 0) {
        call(ctx, afn::Voice_DoStop, {voice, 1, value, 0});
        result = handle;
    }
    call(ctx, afn::Voice_Unlock);
    ctx.gpr[3] = result;
}
