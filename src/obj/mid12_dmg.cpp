// Game code, medium sweep part 12: damage / collision def readers.
#include "mid12.h"

using namespace sly4;
using namespace sly4::mid12;

// void DamageColDef_ReadC8138(a0, XmlCursor* c, x, const Def* tmpl, Rec* o)
// DamageColDef_ReadAD1F8(a0, c, x, tmpl, o), then a defaults record (DmgCtor
// with x->12..20, r7 = 1, r8 as the call left it, pool 0x001C8118; the
// 0x80000 group (Dmg80000); six 96-byte slots at +320 preset and then
// zeroed by memset(rec + 320, 0, 576); +4 |= 0x00800000), the slots taken
// from tmpl +416 when tmpl has 0x00800000 in +24 (else c's document path
// noted when tmpl is set) and copied to o +320. Then the first six
// children tagged 0xEB9EE198 (further ones skipped) fill o's slots: hash
// 0x75849CFE +0, vec3s 0x2B7B3EB4 +16 / 0x313AB0C1 +32, the euler vec3
// 0x313AB844 (default 0; when found, scaled by pool +20 into a stack copy
// and turned into the quaternion +48, half angles by pool +24), bools
// 0x8141EC8A +64 / 0xF8E455AC +65, Xml_ReadKind_00223CA0(child, &tmp) (the
// result unused), f32s 0x7C8F6049 +80 / 0x8FA5922D +72 / 0x3C8207B4 +76 /
// 0x8C014439 +84. The record's vector is destroyed. r3 = 0 or SmallFree's.
DECOMP(001C8138, DamageColDef_ReadC8138) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 1072);
    const u32 rec = frame.addr(), ev = rec + 1008, kind = rec + 1016;  // the original's sp + 384 / + 368 / + 1280
    constexpr u32 kPool = 0x001C8118;
    call(ctx, m11fn::DamageColDef_ReadAD1F8, {a0, c, x, tmpl, o});
    mid11::DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 1, ctx.gpr[8], kPool);
    mid11::Dmg80000(ctx, rec, 0x80000);
    // The six 96-byte slot presets (as in DamageColDef_FromRecC7F28) are
    // dead: the memset below clears them. f1 is left by their last lfs.
    SetF1(ctx, mid11::kQuat4C90 + 12);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x00800000u);
    call(ctx, mid2::m2fn::memset, {rec + 320, 0, 576});
    if (mid11::DmgTmpl(ctx, a0, tmpl, 0x00800000u)) call(ctx, m12fn::Memmove, {rec + 320, tmpl + 416, 576});
    call(ctx, m12fn::Memmove, {o + 320, rec + 320, 576});
    constexpr u32 kTag = 0xEB9EE198u;
    const f64 k20 = LoadF(kPool + 20), k24 = LoadF(kPool + 24);
    u32 i = 0;
    for (u32 n = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, kTag})); n != 0;
         n = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {n, kTag})), ++i) {
        if (i >= 6) continue;
        const u32 s = o + 320 + 96 * i;
        mid5::XHash(ctx, n, 0x75849CFEu, s);
        call(ctx, xfn::XmlCursor_ReadVec3, {n, 0x2B7B3EB4u, s + 16, s + 16});
        call(ctx, xfn::XmlCursor_ReadVec3, {n, 0x313AB0C1u, s + 32, s + 32});
        if (call(ctx, xfn::XmlCursor_ReadVec3, {n, 0x313AB844u, ev, mid11::kVecZero}) == 0) {
            const f64 ey = MulS(LoadF(ev + 4), k20), ex = MulS(LoadF(ev), k20), ez = MulS(LoadF(ev + 8), k20);
            StoreF(ev + 4, ey);
            StoreF(ev, ex);
            StoreF(ev + 8, ez);
            EulerQuatAt(ctx, s + 48, ex, ey, ez, k24);
        }
        mid5::XBool(ctx, n, 0x8141EC8Au, s + 64);
        mid5::XBool(ctx, n, 0xF8E455ACu, s + 65);
        store<u32>(kind, 0);
        call(ctx, 0x00223CA0, {n, kind});  // Xml_ReadKind_00223CA0
        mid5::XF32(ctx, n, 0x7C8F6049u, s + 80);
        mid5::XF32(ctx, n, 0x8FA5922Du, s + 72);
        mid5::XF32(ctx, n, 0x3C8207B4u, s + 76);
        mid5::XF32(ctx, n, 0x8C014439u, s + 84);
    }
    ctx.gpr[3] = 0;
    mid11::DmgDestroy(ctx, rec);
}
