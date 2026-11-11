// Game code, medium sweep part 11: other def factories on
// Def_Construct74FD80 (called by 000CA038).
#include "mid11.h"

using namespace sly4;
using namespace sly4::mid11;

// Def* XmlDef_New307A78(a0, XmlCursor* c, x, const Def* tmpl)
// A defaults record (DmgCtor with x->12..20, r7 the pool 0x00307A64, r8
// x->20; +0 |= 0x40000) read by 0x003078D8, then a 368-byte def:
// Thunk_000E27FC(p, rec), vtable 0x00B1DA78, inline PtrVectors at +304 /
// +324 (capacity 16) and +344 (constructed only, element size 4); then
// 0x0030743C(p, c) (with p = 0 as well when the allocation fails).
// Returns p.
DECOMP(00307A78, XmlDef_New307A78) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 272);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x00307A64;
    const u32 x20 = load<u32>(x + 20);
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, kPool, x20, kPool);
    store<u32>(rec, load<u32>(rec) | 0x40000u);
    call(ctx, 0x003078D8, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {368}));
    if (p != 0) {
        call(ctx, m11fn::Thunk_000E27FC, {p, rec});
        store<u32>(p, 0x00B1DA78u);
        mid1::PtrVecNew(ctx, p + 304, 16, 4, 1);
        mid1::PtrVecNew(ctx, p + 324, 16, 4, 1);
        call(ctx, m1fn::PtrVector_Construct, {p + 344});
        store<u16>(p + 350, 4);
    }
    call(ctx, 0x0030743C, {p, c});  // called with 0 too when the allocation fails
    DmgDestroy(ctx, rec);
    ret(ctx, p);
}

// Def* XmlDef_New2F8EE0(a0, XmlCursor* c, x, const Def* tmpl)
// A defaults record (DmgCtor with x->12..20, r7 the pool 0x002F8EC8, r8
// x->20; an inline PtrVector v at +224 (element size 4, capacity 0); +0 |=
// 0x80000) read by 0x002F8CF0. Then for each child of c tagged 0x88B4A252
// an 80-byte item (SmallAlloc; +32..+48 / byte +72 / +76 0, a PtrVector at
// +52, memset(item, 0, 32)) read by 0x002F8AFC(item, child) (called with 0
// too when the allocation fails) and pushed onto v. Then a 336-byte def:
// Thunk_000E27FC(p, rec), vtable 0x00B1D5C8, a PtrVector at +304
// (capacity 32) holding v's items. v is cleared and the record's vector
// destroyed. Returns the def (0 when the allocation fails).
DECOMP(002F8EE0, XmlDef_New2F8EE0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 320);
    const u32 rec = frame.addr(), v = rec + 224;  // the original's sp + 368 / + 592
    constexpr u32 kPool = 0x002F8EC8;
    const u32 x20 = load<u32>(x + 20);
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, kPool, x20, kPool);
    mid1::PtrVecNew(ctx, v, 0, 4, 0);
    store<u32>(rec, load<u32>(rec) | 0x80000u);
    call(ctx, 0x002F8CF0, {a0, c, x, tmpl, rec});
    constexpr u32 kTag = 0x88B4A252u;
    for (u32 n = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, kTag})); n != 0;
         n = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {n, kTag}))) {
        const u32 q = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {80}));
        if (q != 0) {
            store<u32>(q + 32, 0);
            store<u32>(q + 36, 0);
            store<u8>(q + 40, 0);
            store<u32>(q + 44, 0);
            store<u32>(q + 48, 0);
            call(ctx, m1fn::PtrVector_Construct, {q + 52});
            store<u16>(q + 58, 4);
            store<u8>(q + 72, 0);
            store<u32>(q + 76, 0);
            call(ctx, 0x00726E54, {q, 0, 32});  // memset
        }
        call(ctx, 0x002F8AFC, {q, n});
        mid1::PtrVecPush(ctx, v, q);
    }
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {336}));
    if (p != 0) {
        call(ctx, m11fn::Thunk_000E27FC, {p, rec});
        store<u32>(p, 0x00B1D5C8u);
        mid1::PtrVecNew(ctx, p + 304, 32, 4, 1);
        for (u32 i = 0; i < load<u16>(v + 8); ++i) {
            const u32 e = load<u32>(v) + 4 * i;
            const u32 cap = load<u16>(p + 308);
            if (load<u16>(p + 312) == cap) call(ctx, m1fn::PtrVector_Reserve, {p + 304, (cap << 1) & 0xFFFFFFFEu});
            store<u32>(load<u32>(p + 304) + (static_cast<u32>(load<u16>(p + 312)) << 2), load<u32>(e));
            const u32 cnt = (load<u16>(p + 312) + 1u) & 0xFFFFu;
            const u32 m = load<u16>(p + 314);
            store<u16>(p + 312, static_cast<u16>(cnt));
            store<u16>(p + 314, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(cnt) ? m : cnt));
        }
    }
    call(ctx, m1fn::PtrVector_Clear, {v});
    call(ctx, m1fn::Nop, {v});
    DmgDestroy(ctx, rec);
    ret(ctx, p);
}
