// UiElem_Construct, UiText_Construct (+ the app-level thunk),
// UiElem_CloneChildren, UiText_ReserveText, UiText_ReserveQuads, String_CopyN.
#include "ctor.h"

using namespace sly4;
using namespace sly4::uielem;
using namespace sly4::text;

namespace {

inline void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }  // lfs / stfs pair: bit exact

}  // namespace

// char* String_CopyN(char* dst, const char* src, u32 n)
DECOMP(00553638, String_CopyN) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    if (n == 0) {
        ret(ctx, 0);
        return;
    }
    ret(ctx, call(ctx, cfn::strncpy, {dst, src, n}));
}

// void UiElem_Construct(Element* e, const ElemDesc* d)
// Fills the base element from its description and appends it to the
// parent's child list (or to the top-level list when it has no parent).
DECOMP(004D76D0, UiElem_Construct) {
    const u32 e = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);

    // Matrix rows (3 floats each, the 4th word untouched), position, scale.
    for (u32 row = 16; row <= 80; row += 16)
        for (u32 k = 0; k < 12; k += 4) copy32(e + row + k, d + row + k);
    store<u32>(e, kElemVtbl);

    store<u32>(e + kElemNameHash, static_cast<u32>(call(ctx, fn::String_HashNoCase, {load<u32>(d + kDescName)})));
    for (u32 k = 0; k < 4; ++k) store<u8>(e + 132 + k, load<u8>(d + kDescColor + k));

    u32 f = load<u32>(e + 136) & 0x7FFFFFFF;
    for (u32 k = 0; k < 6; ++k)  // desc +120..+125 bit 0 -> 0x80000000 .. 0x04000000
        f = (f & ~(0x80000000u >> k)) | ((static_cast<u32>(load<u8>(d + kDescFlags + k)) << (31 - k)) & (0x80000000u >> k));
    f = (f | 0x02000000) & ~0x01000000u;
    f = (f & ~0x00C00000u) | ((load<u32>(d + kDescFieldA) << 22) & 0x00C00000);
    f = (f & ~0x003FC000u) | ((load<u32>(d + kDescFieldB) << 14) & 0x003FC000);
    f = (f & ~0x00003FC0u) | ((load<u32>(d + kDescFieldC) << 6) & 0x00003FC0);
    f = (f & ~0x20u) | ((static_cast<u32>(load<u8>(d + kDescFlags + 6)) << 5) & 0x20);
    store<u32>(e + 136, f);

    copy32(e + kElem140, kElemZeroVec2);
    store<u32>(e + kElemParent, load<u32>(d + kDescParent));
    store<u32>(e + kElemFirstChild, 0);
    store<u32>(e + kElemNext, 0);
    copy32(e + kElem140 + 4, kElemZeroVec2 + 4);
    store<u32>(e + kElemArg, load<u32>(d + kDescArg));
    store<u32>(e + kElem164, 0);
    call(ctx, cfn::String_CopyN, {e + 96, load<u32>(d + kDescName), kElemNameLen - 1});

    const u32 parent = load<u32>(e + kElemParent);
    u32 node = parent != 0 ? load<u32>(parent + kElemFirstChild) : load<u32>(kUiRootList);
    if (node != 0) {
        while (load<u32>(node + kElemNext) != 0) node = load<u32>(node + kElemNext);
        store<u32>(node + kElemNext, e);
        ret(ctx, parent);  // r3 as the original leaves it
        return;
    }
    if (parent != 0) {
        store<u32>(parent + kElemFirstChild, e);
        ret(ctx, parent);
    } else {
        store<u32>(kUiRootList, e);
        ret(ctx, kUiRootList & 0xFFFF0000u);  // r3 = lis 0x00F6
    }
}

// void UiText_Construct(TextElem* e, const ElemDesc* d)
DECOMP(004DC430, UiText_Construct) {
    const u32 e = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, cfn::UiElem_Construct, {e, d});

    store<u32>(e, kTextVtbl);
    copy32(e + kTextLineSpacing, d + kDescLineSpacing);
    copy32(e + kTextCharSpacing, d + kDescCharSpacing);
    u32 f = load<u32>(e + kTextFlags) & 0x7FFFFFFF;
    f |= (static_cast<u32>(load<u8>(d + kDescMonospace)) << 31) | kTextLayoutPending;
    f = (f & ~0x3C000000u) | ((load<u32>(d + kDescField26) << 26) & 0x3C000000);
    f = (f & ~0x03C00000u) | ((load<u32>(d + kDescGlyphMode) << 22) & 0x03C00000);
    f = (f & ~0x003FFFC0u) | ((load<u32>(d + kDescWrap) << 6) & 0x003FFFC0);
    store<u32>(e + kTextFlags, f);

    const u32 set = load<u32>(d + kDescFontSet);
    store<u32>(e + kTextFontSet, set);
    for (u32 off : {kTextBuf, kTextSource, kTextBufCap, kTextQuadCap, kTextSourceCap, kTextQuadBytes, kTextQuads,
                    kTextGlyphCountCached, kTextReveal, kTextRevealElapsed})
        store<u32>(e + off, 0);
    copy32(e + kTextGlyphCount, kElemZeroVec2);
    copy32(e + kTextLineCount, kElemZeroVec2 + 4);

    if (set != 0) {
        store<u16>(set + kSetRefs, static_cast<u16>(load<u16>(set + kSetRefs) + 1));
        const u16 users = static_cast<u16>(load<u16>(set + kSetUsers) + 1);
        store<u16>(set + kSetUsers, users);
        if (static_cast<s16>(users) > 0 && load<u16>(set + kSetLoaded) == 0) {
            const u32 opd = vfunc(set, kVfnSetLoad);
            // r4 / r5 hold the OPD and its entry in the original.
            const u32 loaded = static_cast<u32>(call_ptr(ctx, opd, {set, opd, load<u32>(opd)}));
            store<u16>(set + kSetLoaded, static_cast<u16>(loaded));
        }
    }

    call(ctx, tfn::UiText_SetText, {e, load<u32>(d + kDescText), load<u8>(d + kDescByte164)});

    const f64 rate = static_cast<f64>(load<f32>(d + kDescRevealRate));
    const f64 eps = static_cast<f64>(load<f32>(kTextCtorConsts));
    f64 period = static_cast<f64>(load<f32>(kTextCtorConsts + 4));  // FLT_MAX
    if (rate >= eps)
        period = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<f32>(kTextCtorConsts + 8)) / rate));
    store<f32>(e + kText232, static_cast<f32>(period));
    // Registers as the original leaves them (UiText2_Construct passes them on).
    ctx.fpr[1] = period;
    ctx.fpr[2] = rate;
    ctx.fpr[3] = eps;
    ret(ctx, kTextCtorConsts);  // r3 = the constants' address on both paths
}

// void UiText2_Construct(TextElem2* e, const ElemDesc* d)
// Only forwards to UiText_Construct; the caller sets the derived vtable.
DECOMP(004DC5BC, UiText2_Construct) {
    call(ctx, sfn2::UiText_Construct, {arg<u32>(ctx, 0), arg<u32>(ctx, 1)});
}

// void UiElem_CloneChildren(Element* from, Element* to, u32 arg)
// Clones each child of `from` (deep) under `to`, keeping its name.
DECOMP(004D7634, UiElem_CloneChildren) {
    const u32 to = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    for (u32 c = load<u32>(arg<u32>(ctx, 0) + kElemFirstChild); c != 0; c = load<u32>(c + kElemNext)) {
        const u32 opd = vfunc(c, kVfnClone);
        // r8 / r9 hold the OPD and its entry in the original.
        call_ptr(ctx, opd, {c, c + 96, to, a, 1, opd, load<u32>(opd)});
    }
}

// void UiText_ReserveText(TextElem* e, u32 bytes)
// Reallocates the +192 buffer unless it is already larger than `bytes`;
// 0 frees it. Contents are not kept.
DECOMP(004DB780, UiText_ReserveText) {
    const u32 e = arg<u32>(ctx, 0), need = arg<u32>(ctx, 1);
    u32 cap = load<u32>(e + kTextBufCap);
    if (cap > need && need != 0) {
        ret(ctx, cap);  // r3 as the original leaves it
        return;
    }
    const u32 buf = load<u32>(e + kTextBuf);
    if (buf != 0) {
        call(ctx, cfn::SmallFree, {buf, cap});
        store<u32>(e + kTextBuf, 0);
    }
    if (need == 0) {
        store<u32>(e + kTextBufCap, 0);
        ret(ctx, 0);
        return;
    }
    cap = (need + 32) & ~31u;
    store<u32>(e + kTextBufCap, cap);
    const u32 p = static_cast<u32>(call(ctx, cfn::SmallAlloc, {cap}));
    store<u32>(e + kTextBuf, p);
    ret(ctx, p);
}

// void UiText_ReserveQuads(TextElem* e, u32 glyphs)
// Reallocates the +208 quads (zeroed) unless they hold `glyphs` already;
// 0 frees them. The count is rounded up to a multiple of 32.
DECOMP(004DB6CC, UiText_ReserveQuads) {
    const u32 e = arg<u32>(ctx, 0), need = arg<u32>(ctx, 1);
    const u32 cap = load<u32>(e + kTextQuadCap);
    if (!(cap < need) && need != 0) {
        ret(ctx, cap);  // r3 as the original leaves it
        return;
    }
    const u32 quads = load<u32>(e + kTextQuads);
    if (quads != 0) {
        call(ctx, cfn::SmallFree, {quads, load<u32>(e + kTextQuadBytes)});
        store<u32>(e + kTextQuads, 0);
        store<u32>(e + kTextQuadBytes, 0);
    }
    if (need == 0) {
        store<u32>(e + kTextQuadCap, 0);
        ret(ctx, 0);
        return;
    }
    const u32 t = need + 31;
    store<u32>(e + kTextQuadCap, t & ~31u);
    store<u32>(e + kTextQuadBytes, (t << 5) & 0xFFFFFC00u);
    const u32 p = static_cast<u32>(call(ctx, cfn::SmallAlloc, {(t << 5) & 0xFFFFFC00u}));
    store<u32>(e + kTextQuads, p);
    ret(ctx, call(ctx, cfn::memset, {p, 0, load<u32>(e + kTextQuadBytes)}));
}
