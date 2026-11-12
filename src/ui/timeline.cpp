// Ui_UpdateTimeline: runs a UI timeline (keys that start tracks).
#include "ui.h"

namespace sly4::ui {
using namespace opentit::native;

// Runs every track (vfunc +8 with dt) and deletes those that report they
// are done. Then fires the keys whose delay has run out: each key's vfunc
// +8 (tl, f1 = its delay) may return a new track, which runs once with the
// time left and is kept if it wants to go on. When the last key has fired
// and the template loops, the tracks are deleted and the keys start over
// (at most four times a frame). Returns whether anything is left to run.
// dt is used at the double precision it arrives with until the first key.
u32 Ui_UpdateTimeline(Timeline* tl, f64 dt) {
    f64 time = dt;
    TlTrack* last = nullptr;  // the last track kept; new tracks are linked after it
    for (TlTrack* track = tl->tracks; track;) {
        TlTrack* next = track->next;
        if (vcall<u32>(track, kVfnRun, time) != 0) {
            last = track;
        } else {
            if (last) last->next = next;
            else tl->tracks = next;
            vcall(track, kVfnDelete);
        }
        track = next;
    }
    TlKey* key = tl->key;
    u32 loops = 0;
    while (key) {
        const f32 delay = tl->delay;
        if (!(time >= delay)) {
            tl->delay = fsubs(delay, time);
            break;
        }
        if (TlTrack* track = vcall<TlTrack*>(key, kVfnRun, tl, delay)) {
            if (vcall<u32>(track, kVfnRun, time) != 0) {
                if (last) last->next = track;
                else tl->tracks = track;
                last = track;
            } else {
                vcall(track, kVfnDelete);
            }
        }
        TlKey* fired = tl->key;
        time = fsubs(time, tl->delay);
        key = fired->next;
        tl->key = key;
        if (!key && tl->templ->loops != 0 && ++loops < kMaxLoopsPerFrame) {
            for (TlTrack* t = tl->tracks; t;) {
                TlTrack* next = t->next;
                vcall(t, kVfnDelete);
                t = next;
            }
            // `last` is not reset, as in the original: a track started after
            // the restart is linked after a deleted one.
            tl->tracks = nullptr;
            key = tl->templ->first_key;
            tl->key = key;
        }
        tl->delay = key ? key->delay.get() : 0.0f;
    }
    return (tl->tracks || key) ? 1u : 0u;
}

}  // namespace sly4::ui

NATIVE(004D3B00, sly4::ui::Ui_UpdateTimeline)
