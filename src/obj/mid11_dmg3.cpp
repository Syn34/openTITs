// Game code, medium sweep part 11: damage / collision def factories of the
// large readers (001AFA78, 001C2058, 001C49E8, 001CA080, 001E6300; all
// called by 000CA038): each presets a defaults record, has the reader fill
// it, then builds the def from it.
#include "mid11.h"

using namespace sly4;
using namespace sly4::mid11;

namespace {

constexpr u32 kVec4B70 = 0x01164B70;  // f32[3]

// The factory tail with inline vectors: reader(a0, c, x, tmpl, rec), a def
// of `size` bytes built by ctor(p, rec), then the given inline vectors of
// the record cleared (PtrVector_Clear, Nop) and the record's vector +124
// destroyed.
u32 DmgNewVec(PPUContext& ctx, u32 a0, u32 c, u32 x, u32 tmpl, u32 rec, u32 reader, u32 size, u32 ctor, std::initializer_list<u32> vecs) {
    call(ctx, reader, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {size}));
    if (p != 0) call(ctx, ctor, {p, rec});
    for (u32 v : vecs) {
        call(ctx, m1fn::PtrVector_Clear, {v});
        call(ctx, m1fn::Nop, {v});
    }
    DmgDestroy(ctx, rec);
    return p;
}

}  // namespace

// Def* DamageColDef_NewB0120(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (DmgCtor with x->12..20, r7 1, r8 x->20, pool
// 0x001B010C; +224..+232 = the vec3 0x01164B70, f32 +240..+276 = 1.0, 6.0,
// 3.5, 2.0, 0.5, 6.0, 3.5, 0x4196CBE4, 0x3DB2B8C2, 0.1, +280..+288 0, the
// top byte of +292 0xFF with its bits 8 / 11 set and 9 / 10 clear, +4 |=
// 0x40000) read by 0x001AFA78, then a 384-byte def built by 0x001AF598(p,
// rec). Returns it (0 when the allocation fails).
DECOMP(001B0120, DamageColDef_NewB0120) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 368);
    const u32 rec = frame.addr();
    const u32 x20 = load<u32>(x + 20);
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, 1, x20, 0x001B010C);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 224 + 4 * k, kVec4B70 + 4 * k);
    static constexpr u32 kF[10] = {0x3F800000u, 0x40C00000u, 0x40600000u, 0x40000000u, 0x3F000000u,
                                   0x40C00000u, 0x40600000u, 0x4196CBE4u, 0x3DB2B8C2u, 0x3DCCCCCDu};
    for (u32 k = 0; k < 10; ++k) store<u32>(rec + 240 + 4 * k, kF[k]);
    for (u32 k = 0; k < 3; ++k) store<u32>(rec + 280 + 4 * k, 0);
    const u32 w = (load<u32>(rec + 292) & 0x000FFFFFu) | 0xFF900000u;
    store<u8>(rec + 292, 0xFF);
    store<u32>(rec + 292, w);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x40000u);
    ctx.fpr[1] = LoadF(kVec4B70 + 8);  // the last lfs
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001AFA78, 384, 0x001AF598));
}

// Def* DamageColDef_NewE6B18(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (DmgCtor with x->12..20, r7 the pool 0x001E6B04, r8
// x->20; +224..+232 / +264 / +296 / +300 / +316..+352 / +384..+392 0,
// +272..+280 / +368..+376 the zero vec3, f32 +288 55.0, +292 0.5, +396 4.0,
// +4 |= 0x10000000) read by 0x001E6300, then a 480-byte def built by
// 0x001E5170(p, rec). Returns it (0 when the allocation fails).
DECOMP(001E6B18, DamageColDef_NewE6B18) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 400);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001E6B04;
    const u32 x20 = load<u32>(x + 20);
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, kPool, x20, kPool);
    store<u32>(rec + 264, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 272 + 4 * k, kVecZero + 4 * k);
    store<u32>(rec + 288, 0x425C0000u);
    store<u32>(rec + 292, 0x3F000000u);
    for (u32 off : {224u, 296u, 228u, 300u, 232u}) store<u32>(rec + off, 0);
    for (u32 k = 0; k < 10; ++k) store<u32>(rec + 316 + 4 * k, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 368 + 4 * k, kVecZero + 4 * k);
    for (u32 k = 0; k < 3; ++k) store<u32>(rec + 384 + 4 * k, 0);
    store<u32>(rec + 396, 0x40800000u);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x10000000u);
    ctx.fpr[1] = LoadF(kVecZero + 8);  // the last lfs
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001E6300, 480, 0x001E5170));
}

// Def* DamageColDef_NewCA7C0(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (DmgCtor with x->12, name 0x00AD13B8, r7 the pool
// 0x001CA7AC, r8 as the caller left it; Dmg80000; +320..+328 / +352..+380
// / +396 0, +336..+344 the zero vec3, f32 +384 -1, +388 0x3EB2B8C2, +392
// 0x3DB2B8C2, +400 -1, bytes +404..+406 1 / +407 0, +4 |= 0x02080000) read
// by 0x001CA080, then a 496-byte def built by 0x001C9E34(p, rec). Returns
// it (0 when the allocation fails).
DECOMP(001CA7C0, DamageColDef_NewCA7C0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 416);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001CA7AC;
    // r8 is whatever the caller left (the callee ignores it).
    DmgCtor(ctx, rec, load<u32>(x + 12), kStrAD13B8, kStrAD13B8, kPool, ctx.gpr[8], kPool);
    Dmg80000(ctx, rec, 0x02080000u);
    for (u32 k = 0; k < 3; ++k) store<u32>(rec + 320 + 4 * k, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 336 + 4 * k, kVecZero + 4 * k);
    for (u32 k = 0; k < 8; ++k) store<u32>(rec + 352 + 4 * k, 0);
    store<u32>(rec + 384, 0xBF800000u);
    store<u32>(rec + 388, 0x3EB2B8C2u);
    store<u32>(rec + 392, 0x3DB2B8C2u);
    store<u32>(rec + 396, 0);
    store<u32>(rec + 400, 0xFFFFFFFFu);
    for (u32 k = 0; k < 3; ++k) store<u8>(rec + 404 + k, 1);
    store<u8>(rec + 407, 0);
    ctx.fpr[1] = LoadF(kVecZero + 8);  // the last lfs (f2 the one before)
    ctx.fpr[2] = LoadF(kVecZero + 4);
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001CA080, 496, 0x001C9E34));
}

// Def* DamageColDef_NewC28E8(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (DmgCtor with x->12, name 0x00AD13B8, r7 the pool
// 0x001C28D0, r8 as the caller left it; Dmg80000; inline PtrVectors at
// +320 and +340 (capacity 4), f32 +360 -1, bytes +364..+367 0 / 1 / 0 / 1,
// +4 |= 0x01080000) read by 0x001C2058, then a 448-byte def built by
// 0x001C1D4C(p, rec); the two vectors cleared (+340 first). Returns the def
// (0 when the allocation fails).
DECOMP(001C28E8, DamageColDef_NewC28E8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 400);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001C28D0;
    // r8 is whatever the caller left (the callee ignores it).
    DmgCtor(ctx, rec, load<u32>(x + 12), kStrAD13B8, kStrAD13B8, kPool, ctx.gpr[8], kPool);
    Dmg80000(ctx, rec, 0x80000u);
    mid1::PtrVecNew(ctx, rec + 320, 4, 4, 1);
    mid1::PtrVecNew(ctx, rec + 340, 4, 4, 1);
    store<u32>(rec + 360, 0xBF800000u);
    store<u8>(rec + 364, 0);
    store<u8>(rec + 365, 1);
    store<u8>(rec + 366, 0);
    store<u8>(rec + 367, 1);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x01000000u);
    ret(ctx, DmgNewVec(ctx, a0, c, x, tmpl, rec, 0x001C2058, 448, 0x001C1D4C, {rec + 340, rec + 320}));
}

// Def* DamageColDef_NewC5408(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (DmgCtor with x->12, name 0x00AD13B8, r7 the pool
// 0x001C53F0, r8 1; Dmg80000; +320..+328 the zero vec3, +336 / +344..+352
// 0, f32 +340 / +356 / +360 -1, bytes +364..+374 0; six 80-byte entries
// at +384 {+0 0, +16 / +32 the zero vec3, +48 the quaternion 0x01164C90,
// +64 0, bytes +68 1 / +69 0}; +4 |= 0x480000) read by 0x001C49E8, then a
// 928-byte def built by 0x001C463C(p, rec). Returns it (0 when the
// allocation fails).
DECOMP(001C5408, DamageColDef_NewC5408) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 880);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001C53F0;
    DmgCtor(ctx, rec, load<u32>(x + 12), kStrAD13B8, kStrAD13B8, kPool, 1, kPool);
    Dmg80000(ctx, rec, 0x80000u);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 320 + 4 * k, kVecZero + 4 * k);
    store<u32>(rec + 336, 0);
    store<u32>(rec + 340, 0xBF800000u);
    for (u32 k = 0; k < 3; ++k) store<u32>(rec + 344 + 4 * k, 0);
    store<u32>(rec + 356, 0xBF800000u);
    store<u32>(rec + 360, 0xBF800000u);
    for (u32 k = 0; k < 11; ++k) store<u8>(rec + 364 + k, 0);
    for (u32 i = 0; i < 6; ++i) {
        const u32 e = rec + 384 + 80 * i;
        store<u32>(e, 0);
        for (u32 k = 0; k < 3; ++k) CopyW(e + 16 + 4 * k, kVecZero + 4 * k);
        for (u32 k = 0; k < 3; ++k) CopyW(e + 32 + 4 * k, kVecZero + 4 * k);
        for (u32 k = 0; k < 4; ++k) CopyW(e + 48 + 4 * k, kQuat4C90 + 4 * k);
        store<u32>(e + 64, 0);
        store<u8>(e + 68, 1);
        store<u8>(e + 69, 0);
    }
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x400000u);
    ctx.fpr[1] = LoadF(kQuat4C90 + 12);  // the last lfs (f2 an earlier one)
    ctx.fpr[2] = LoadF(kVecZero + 4);
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001C49E8, 928, 0x001C463C));
}

// Def* DamageColDef_NewC8780(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (DmgCtor with x->12, name 0x00AD13B8, r7 the pool
// 0x001C8768, r8 1; Dmg80000; six 96-byte entries at +320 {+0 0, +16 / +32
// the zero vec3, +48 the quaternion 0x01164C90, bytes +64 1 / +65 0, +68..
// +84 0}, then memset(+320, 0, 576) over them; +4 |= 0x800000) read by
// 0x001C8138, then a 992-byte def built by 0x001C7F28(p, rec). Returns it
// (0 when the allocation fails).
DECOMP(001C8780, DamageColDef_NewC8780) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 896);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001C8768;
    DmgCtor(ctx, rec, load<u32>(x + 12), kStrAD13B8, kStrAD13B8, kPool, 1, kPool);
    Dmg80000(ctx, rec, 0x80000u);
    for (u32 i = 0; i < 6; ++i) {
        const u32 e = rec + 320 + 96 * i;
        store<u32>(e, 0);
        for (u32 k = 0; k < 3; ++k) CopyW(e + 16 + 4 * k, kVecZero + 4 * k);
        for (u32 k = 0; k < 3; ++k) CopyW(e + 32 + 4 * k, kVecZero + 4 * k);
        for (u32 k = 0; k < 4; ++k) CopyW(e + 48 + 4 * k, kQuat4C90 + 4 * k);
        store<u8>(e + 64, 1);
        store<u8>(e + 65, 0);
        for (u32 k = 0; k < 5; ++k) store<u32>(e + 68 + 4 * k, 0);
    }
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x800000u);
    ctx.fpr[1] = LoadF(kQuat4C90 + 12);  // the last lfs
    call(ctx, 0x00726E54, {rec + 320, 0, 576});  // memset
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001C8138, 992, 0x001C7F28));
}

// Def* DamageColDef_NewD71D8(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (DmgCtor with x->12..20, r7 the pool 0x001D71C4, r8
// x->20; +256..+312 four zero vec3s, f32 +320 50.0, +324 0, +328 / +332
// 0.5, +336 -1, +340..+348 0, +352 1, +356..+408 0, bits 0..4 of +412
// cleared then bits 5 / 6 set, byte +222 0, +8 |= 1) read by 0x001D62A8,
// then a 496-byte def built by 0x001D5A40(p, rec). Returns it (0 when the
// allocation fails).
DECOMP(001D71D8, DamageColDef_NewD71D8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 416);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001D71C4;
    const u32 x20 = load<u32>(x + 20);
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, kPool, x20, kPool);
    for (u32 v = 0; v < 4; ++v)
        for (u32 k = 0; k < 3; ++k) CopyW(rec + 256 + 16 * v + 4 * k, kVecZero + 4 * k);
    store<u32>(rec + 320, 0x42480000u);
    store<u32>(rec + 324, 0);
    store<u32>(rec + 328, 0x3F000000u);
    store<u32>(rec + 332, 0x3F000000u);
    store<u32>(rec + 336, 0xBF800000u);
    for (u32 k = 0; k < 3; ++k) store<u32>(rec + 340 + 4 * k, 0);
    store<u32>(rec + 352, 1);
    for (u32 k = 0; k < 14; ++k) store<u32>(rec + 356 + 4 * k, 0);
    store<u32>(rec + 412, (load<u32>(rec + 412) & 0x07FFFFFFu) | 0x06000000u);
    store<u8>(rec + 222, 0);
    store<u32>(rec + 8, load<u32>(rec + 8) | 1u);
    ctx.fpr[1] = LoadF(kVecZero);  // the last lfs (f1..f3 = the zero vec3)
    ctx.fpr[2] = LoadF(kVecZero + 4);
    ctx.fpr[3] = LoadF(kVecZero + 8);
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001D62A8, 496, 0x001D5A40));
}

// Def* DamageColDef_NewDE610(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (DmgCtor with x->12..20, r7 the pool 0x001DE5F8, r8
// x->20; zero vec3s at +240 / +272 / +304 / +352, identity quaternions at
// +320 / +368, f32 limits +440..+452 = -90 / 90 / -180 / 180, +460..+472
// 1.0, +484..+492 / +540..+548 -1, +496..+508 0x3B8EFA35 / 0x3D0EFA35 x2,
// +552 / +556 = the two floats at 0x01164AFC, byte +432 1, the rest of
// +224..+536 0; +4 |= 0x40000000) read by 0x001DD3F8, then a 640-byte def
// built by 0x001DC5F8(p, rec); the record's buffer +520 freed
// (Mem_FreeIfSet) when set. Returns the def (0 when the allocation fails).
DECOMP(001DE610, DamageColDef_NewDE610) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 560);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001DE5F8;
    const u32 x20 = load<u32>(x + 20);
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, kPool, x20, kPool);
    for (u32 v : {240u, 272u, 304u, 352u})
        for (u32 k = 0; k < 3; ++k) CopyW(rec + v + 4 * k, kVecZero + 4 * k);
    for (u32 q : {320u, 368u})
        for (u32 k = 0; k < 4; ++k) CopyW(rec + q + 4 * k, kQuatIdent + 4 * k);
    for (u32 off : {224u, 256u, 296u, 300u, 340u, 384u, 388u, 392u, 396u, 400u, 436u, 456u, 476u, 480u, 512u, 516u, 520u, 524u, 528u, 532u})
        store<u32>(rec + off, 0);
    for (u32 k = 0; k < 7; ++k) store<u32>(rec + 404 + 4 * k, 0);
    store<u8>(rec + 336, 0);
    store<u8>(rec + 337, 0);
    store<u32>(rec + 440, 0xC2B40000u);
    store<u32>(rec + 444, 0x42B40000u);
    store<u32>(rec + 448, 0xC3340000u);
    store<u32>(rec + 452, 0x43340000u);
    store<u8>(rec + 432, 1);
    store<u8>(rec + 433, 0);
    for (u32 k = 0; k < 4; ++k) store<u32>(rec + 460 + 4 * k, 0x3F800000u);
    for (u32 off : {484u, 488u, 492u, 540u, 544u, 548u}) store<u32>(rec + off, 0xBF800000u);
    for (u32 k = 0; k < 2; ++k) {
        store<u32>(rec + 496 + 8 * k, 0x3B8EFA35u);
        store<u32>(rec + 500 + 8 * k, 0x3D0EFA35u);
    }
    for (u32 k = 0; k < 3; ++k) store<u8>(rec + 536 + k, 0);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x40000000u);
    CopyW(rec + 552, kVec2_4AFC);
    CopyW(rec + 556, kVec2_4AFC + 4);
    ctx.fpr[1] = LoadF(kVec2_4AFC + 4);  // the last lfs
    call(ctx, 0x001DD3F8, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {640}));
    if (p != 0) call(ctx, 0x001DC5F8, {p, rec});
    const u32 buf = load<u32>(rec + 520);
    if (buf != 0) call(ctx, 0x00555CC0, {buf, load<u32>(rec + 516)});  // Mem_FreeIfSet (reads r3 only)
    DmgDestroy(ctx, rec);
    ret(ctx, p);
}
