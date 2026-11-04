// Text element destruction and update, and the Timeline destructors.
#include "text_slots.h"

using namespace sly4;
using namespace sly4::uielem;
using namespace sly4::text;

namespace {

void FreeBuffer(PPUContext& ctx, u32 e, u32 cap, u32 ptr) {
    if (field<u32>(e, cap) != 0) call(ctx, tsfn::SmallFree, {field<u32>(e, ptr), field<u32>(e, cap)});
}

// Timeline destruction shared by both destructors.
void Timeline_Release(PPUContext& ctx, u32 t) {
    field<u32>(t, 0) = kTimelineVtbl;
    if (ref<u8>(t + kTlOwns) != 0 && field<u32>(t, kTlOwned) != 0) {
        const u32 o = field<u32>(t, kTlOwned);
        call_ptr(ctx, vfunc(o, kVfnDelete), {o});
        field<u32>(t, kTlOwned) = 0;
    }
    for (u32 tr = field<u32>(t, kTlTracks); tr != 0;) {
        const u32 next = field<u32>(tr, kTrackNext);
        call_ptr(ctx, vfunc(tr, kVfnDelete), {tr});
        tr = next;
    }
    field<u32>(t, kTlTracks) = 0;
    field<u32>(t, kTlKey) = 0;
    field<u32>(t, kTlDelay) = 0;
    u32 prev = 0;
    for (u32 it = load<u32>(kUiTimelines); it != 0; prev = it, it = field<u32>(it, kTlNext)) {
        if (it != t) continue;
        if (prev != 0)
            field<u32>(prev, kTlNext) = field<u32>(t, kTlNext);
        else
            store<u32>(kUiTimelines, field<u32>(t, kTlNext));
        break;
    }
}

}  // namespace

// void UiText_Destruct(TextElem* e)  [text vfunc +0]
// Frees the text buffers, deletes the children, unlinks the element from
// its parent (or the top-level list) and from the four focus slots.
DECOMP(004E1604, UiText_Destruct) {
    const u32 e = arg<u32>(ctx, 0);
    field<u32>(e, 0) = kTextVtbl;
    FreeBuffer(ctx, e, kTextBufCap, kTextBuf);
    FreeBuffer(ctx, e, kTextQuadBytes, kTextQuads);
    FreeBuffer(ctx, e, kTextSourceCap, kTextSource);
    field<u32>(e, 0) = kElemVtbl;

    // Each child's deleting destructor takes it out of this list.
    for (u32 c; (c = field<u32>(e, kElemFirstChild)) != 0;) call_ptr(ctx, vfunc(c, kVfnDelete), {c});

    if ((field<u32>(e, kElemFlags) & kElemDetached) == 0) {
        const u32 parent = field<u32>(e, kElemParent);
        const u32 head = parent != 0 ? field<u32>(parent, kElemFirstChild).get() : load<u32>(kUiRootList);
        u32 prev = 0;
        if (head != e) {
            // No end check: the element must be in the list, as in the original.
            u32 it = head;
            while (field<u32>(it, kElemNext) != e) it = field<u32>(it, kElemNext);
            prev = it;
        }
        if (prev != 0)
            field<u32>(prev, kElemNext) = field<u32>(e, kElemNext);
        else if (parent != 0)
            field<u32>(parent, kElemFirstChild) = field<u32>(e, kElemNext);
        else
            store<u32>(kUiRootList, field<u32>(e, kElemNext));
        field<u32>(e, kElemNext) = 0;
    }
    for (u32 k = 0; k < kUiFocusCount; ++k)
        if (e == 0 || load<u32>(kUiFocus + k * 4) == e) store<u32>(kUiFocus + k * 4, 0);
}

// void UiText_DestructThunk(TextElem* e)  (name?)
DECOMP(004E181C, UiText_DestructThunk) { call(ctx, tsfn::UiText_Destruct, {arg<u32>(ctx, 0)}); }

// void UiText_CountGlyphs(TextElem* e)
// Stores in +212 how many characters of the formatted text (up to the
// quad capacity) have a glyph; newlines and spaces don't count.
DECOMP(004E2178, UiText_CountGlyphs) {
    const u32 e = arg<u32>(ctx, 0);
    u32 n = 0;
    const u32 text = field<u32>(e, kTextBuf);
    if (text != 0) {
        Stack frame(ctx, 8);
        const u32 pp = frame.addr();
        store<u32>(pp, text);
        while (load<u8>(load<u32>(pp)) != 0 && n < field<u32>(e, kTextQuadCap)) {
            const u32 c = call32(ctx, tfn::Utf8_Next, {e, pp, 0, 1});
            if (c == '\n' || c == '\r' || c == ' ') continue;
            const u32 set = field<u32>(e, kTextFontSet);
            const GlyphRef g = FontSet_FindGlyph(set, c);
            if (g.index >= field<u32>(set, kSetFontCount)) continue;
            if (load<u32>(set + kSetFonts + g.index * 4) == 0) continue;
            ++n;
        }
    }
    field<u32>(e, kTextGlyphCountCached) = n;
}

// void UiText_Update(TextElem* e; f1 = dt)  [text vfunc +152]
// Lays the text out if it is pending; while active, advances the reveal
// clock (marking the size dirty while revealing), updates every quad and
// then the children.
DECOMP(004E2480, UiText_Update) {
    const u32 e = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (field<u32>(e, kTextFlags) & kTextLayoutPending) call(ctx, tfn::UiText_Layout, {e, 0});

    u32 flags = field<u32>(e, kElemFlags);
    if (flags & kElemActive) {
        stfs(e + kTextRevealElapsed, fadds(lfs(e + kTextRevealElapsed), dt));
        const bool revealing = lfs(e + kTextReveal) > 0.0;
        flags |= revealing ? kElemSizeDirty : 0;
        field<u32>(e, kElemFlags) = flags;
        if (field<u32>(e, kTextQuadCap) != 0) {
            u32 off = 0;
            for (u32 k = 0; k < field<u32>(e, kTextQuadCap); ++k, off += kQuadSize) {
                ctx.fpr[1] = dt;
                ctx.fpr[2] = lfs(e + kText232);
                call(ctx, tsfn::UiQuad_Update,
                     {field<u32>(e, kTextQuads) + off, 0, (field<u32>(e, kTextFlags) >> 22) & 0xF});
            }
            flags = field<u32>(e, kElemFlags);
        }
    }
    if ((flags & kElemActive) == 0) return;
    for (u32 c = field<u32>(e, kElemFirstChild); c != 0; c = field<u32>(c, kElemNext)) {
        ctx.fpr[1] = dt;
        call_ptr(ctx, vfunc(c, kVfnUpdate), {c});
    }
}

// void Timeline_Destruct(Timeline* t)  [vtable 0x00B228A0 +0]
// Deletes the owned object and the running tracks and unlinks the timeline.
DECOMP(004E3528, Timeline_Destruct) { Timeline_Release(ctx, arg<u32>(ctx, 0)); }

// void Timeline_Delete(Timeline* t)  [+4]
DECOMP(004E3648, Timeline_Delete) {
    const u32 t = arg<u32>(ctx, 0);
    if (t == 0) return;
    Timeline_Release(ctx, t);
    call(ctx, tsfn::SmallFree, {t, kTimelineSize});
}
