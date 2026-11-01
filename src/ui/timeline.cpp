// Ui_UpdateTimeline: runs a UI timeline (keys that start tracks).
#include "ui.h"

using namespace sly4;
using namespace sly4::ui;

// bool Ui_UpdateTimeline(Timeline* tl, f32 dt)  (name?)
//
// Runs every track (vfunc +8 with dt) and deletes those that report they
// are done. Then fires the keys whose delay has run out: each key's vfunc
// +8 (tl, f1 = its delay) may return a new track, which runs once with the
// time left and is kept if it wants to go on. When the last key has fired
// and the template loops, the tracks are deleted and the keys start over
// (at most four times a frame). Returns whether anything is left to run.
DECOMP(004D3B00, Ui_UpdateTimeline) {
    const u32 tl = arg<u32>(ctx, 0);
    f64 time = farg(ctx, 0);

    u32 last = 0;  // the last track kept; new tracks are linked after it
    for (u32 track = field<u32>(tl, kTlTracks); track != 0;) {
        const u32 next = field<u32>(track, kTrackNext);
        ctx.fpr[1] = time;
        if (static_cast<u32>(call_ptr(ctx, vfunc(track, kVfnRun), {track})) != 0) {
            last = track;
        } else {
            if (last != 0)
                field<u32>(last, kTrackNext) = next;
            else
                field<u32>(tl, kTlTracks) = next;
            call_ptr(ctx, vfunc(track, kVfnDelete), {track});
        }
        track = next;
    }

    u32 key = field<u32>(tl, kTlKey);
    u32 loops = 0;
    while (key != 0) {
        const f32 delay = field<f32>(tl, kTlDelay);
        if (!(time >= delay)) {
            field<f32>(tl, kTlDelay) = fsubs(delay, time);
            break;
        }
        ctx.fpr[1] = delay;
        const u32 track = static_cast<u32>(call_ptr(ctx, vfunc(key, kVfnRun), {key, tl}));
        if (track != 0) {
            ctx.fpr[1] = time;
            if (static_cast<u32>(call_ptr(ctx, vfunc(track, kVfnRun), {track})) != 0) {
                if (last != 0)
                    field<u32>(last, kTrackNext) = track;
                else
                    field<u32>(tl, kTlTracks) = track;
                last = track;
            } else {
                call_ptr(ctx, vfunc(track, kVfnDelete), {track});
            }
        }

        const u32 fired = field<u32>(tl, kTlKey);
        time = fsubs(time, field<f32>(tl, kTlDelay));
        key = field<u32>(fired, kKeyNext);
        field<u32>(tl, kTlKey) = key;
        if (key == 0 && field<u8>(field<u32>(tl, kTlTemplate), kTemplateLoops) != 0 && ++loops < kMaxLoopsPerFrame) {
            for (u32 t = field<u32>(tl, kTlTracks); t != 0;) {
                const u32 next = field<u32>(t, kTrackNext);
                call_ptr(ctx, vfunc(t, kVfnDelete), {t});
                t = next;
            }
            // `last` is not reset, as in the original: a track started after
            // the restart is linked after a deleted one.
            field<u32>(tl, kTlTracks) = 0;
            key = field<u32>(field<u32>(tl, kTlTemplate), kTemplateFirstKey);
            field<u32>(tl, kTlKey) = key;
        }
        field<f32>(tl, kTlDelay) = key != 0 ? field<f32>(key, kKeyDelay).get() : 0.0f;
    }
    ret(ctx, (field<u32>(tl, kTlTracks) != 0 || key != 0) ? 1u : 0u);
}
