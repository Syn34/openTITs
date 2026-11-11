// Game code, medium sweep part 11: the leaf def readers / factories that
// build their defaults with Def_Construct74596C (the 000C5528 family; the
// factories are called by 000CA038, the readers chain to 000C4020).
#include "mid11.h"

using namespace sly4;
using namespace sly4::mid11;

namespace {

constexpr u32 DefLeaf_Read5528 = 0x000C5528;
constexpr u32 DefLeaf_Read5B30 = 0x000C5B30;
constexpr u32 DefLeaf_Read5FF0 = 0x000C5FF0;
constexpr u32 kEnum5FF0 = 0x00AC67D0;  // u32[2]: the hashes of the 0xE8DB4898 enum

}  // namespace

// void DefLeaf_Read5528(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DefLeaf_ReadBase_000C4020 with the same arguments, then the defaults: a
// def record on the stack (Def_Construct74596C with x->12, name 0x00AC6790,
// pool 0x000C5510; LeafRecA) with +344 / +348 / byte +352 / +356 / byte
// +378 from tmpl's +404 / +408 / +412 / +416 / +438 when tmpl has flag
// 0x20000 in +20. Reads a hash (0xFEBE4776) into o+344, floats into +348 /
// +356..+372, bools into bytes +352 / +376..+378.
DECOMP(000C5528, DefLeaf_Read5528) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m11fn::DefLeaf_ReadBase_000C4020, {a0, c, x, tmpl, o});
    Stack frame(ctx, 448);
    const u32 rec = frame.addr();
    // r9 / r10 are left over from the call above (the callee ignores them).
    LeafDefCtor(ctx, rec, load<u32>(x + 12), kStrAC6790, kStrAC6790, kStrAC6790, 0x7641A763u, ctx.gpr[9], ctx.gpr[10], 0x000C5510, 0, 0);
    LeafRecA(rec);
    if (tmpl != 0 && (load<u32>(tmpl + 20) & 0x20000u)) {
        store<u32>(rec + 344, load<u32>(tmpl + 404));
        CopyW(rec + 348, tmpl + 408);
        store<u8>(rec + 352, load<u8>(tmpl + 412));
        CopyW(rec + 356, tmpl + 416);
        store<u8>(rec + 378, load<u8>(tmpl + 438));
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xFEBE4776u, o + 344, load<u32>(rec + 344)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xD93B86F6u, o + 348}, {LoadF(rec + 348)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xFBCDDB3Bu, o + 352, load<u8>(rec + 352)});
    static constexpr u32 kF[5] = {0xD63894F0u, 0x35FE9ECFu, 0x37CC9D25u, 0xB289BA5Eu, 0x9EB4518Au};
    for (u32 k = 0; k < 5; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kF[k], o + 356 + 4 * k}, {LoadF(rec + 356 + 4 * k)});
    static constexpr u32 kB[3] = {0x0927884Eu, 0x572A6E83u, 0x644AB625u};
    for (u32 k = 0; k < 3; ++k) call(ctx, xfn::XmlCursor_ReadBool, {c, kB[k], o + 376 + k, load<u8>(rec + 376 + k)});
}

// Def* DefLeaf_New58A8(a0, XmlCursor* c, x, const Def* tmpl)
// (called by 000CA038) The defaults record (pool 0x000C588C, record +12..
// +20 = x->12..20, LeafRecA) read by DefLeaf_Read5528, then a 448-byte def:
// DefLeaf_Construct_000C30E8(p, rec), vtable 0x00B134F8, +404..+432 = the
// record's +344..+372, bytes +436..+438 its +376..+378; when byte +438 is
// set, p->400->44 = 23. Returns it (0 when the allocation fails).
DECOMP(000C58A8, DefLeaf_New58A8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 448);
    const u32 rec = frame.addr();
    const u32 x20 = load<u32>(x + 20);
    LeafDefCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, kStrAC6790, kVecZero, kVec2_4AFC, x20, 0x000C588C, 0, 0);
    LeafRecA(rec);
    call(ctx, DefLeaf_Read5528, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {448}));
    if (p != 0) {
        call(ctx, m11fn::DefLeaf_Construct_000C30E8, {p, rec});
        CopyW(p + 408, rec + 348);
        store<u8>(p + 412, load<u8>(rec + 352));
        CopyW(p + 416, rec + 356);
        CopyW(p + 420, rec + 360);
        store<u32>(p, 0x00B134F8u);
        store<u32>(p + 404, load<u32>(rec + 344));
        CopyW(p + 424, rec + 364);
        store<u8>(p + 436, load<u8>(rec + 376));
        CopyW(p + 428, rec + 368);
        store<u8>(p + 437, load<u8>(rec + 377));
        CopyW(p + 432, rec + 372);
        const u8 b = load<u8>(rec + 378);
        store<u8>(p + 438, b);
        if (b != 0) store<u32>(load<u32>(p + 400) + 44, 23);
        ctx.fpr[1] = LoadF(rec + 372);  // the last lfs, as the original leaves f1
    }
    ret(ctx, p);
}

// void DefLeaf_Read5B30(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DefLeaf_ReadBase_000C4020 with the same arguments, then the defaults
// record (pool 0x000C5B14, name 0x00AC6790, LeafRecB) with +344..+356 =
// tmpl's +404..+416 when tmpl has flag 0x10000 in +20. Reads five floats
// (0x7ECB8527, 0xCCF74C48, 0x9E8B1865, 0x21684859, 0x7D9DFBE4) into
// o+344..+360. Quirk: with no copy from tmpl the first float's default is
// the pool's +20, not the record's 1.0.
DECOMP(000C5B30, DefLeaf_Read5B30) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m11fn::DefLeaf_ReadBase_000C4020, {a0, c, x, tmpl, o});
    Stack frame(ctx, 432);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x000C5B14;
    // r10 is left over from the call above (the callee ignores it).
    LeafDefCtor(ctx, rec, load<u32>(x + 12), kStrAC6790, kStrAC6790, kStrAC6790, kVec2_4AFC, 0x7641A763u, ctx.gpr[10], kPool, 0, 0);
    LeafRecB(rec);
    f64 first = LoadF(kPool + 20);
    if (tmpl != 0 && (load<u32>(tmpl + 20) & 0x10000u)) {
        for (u32 k = 0; k < 4; ++k) CopyW(rec + 344 + 4 * k, tmpl + 404 + 4 * k);
        first = LoadF(rec + 344);
    }
    static constexpr u32 kF[5] = {0x7ECB8527u, 0xCCF74C48u, 0x9E8B1865u, 0x21684859u, 0x7D9DFBE4u};
    for (u32 k = 0; k < 5; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kF[k], o + 344 + 4 * k}, {k == 0 ? first : LoadF(rec + 344 + 4 * k)});
}

// Def* DefLeaf_New5DC8(a0, XmlCursor* c, x, const Def* tmpl)
// (called by 000CA038) The defaults record (pool 0x000C5DAC, record +12..
// +20 = x->12..20, LeafRecB) read by DefLeaf_Read5B30, then a 432-byte def:
// DefLeaf_Construct_000C30E8(p, rec), vtable 0x00B132D8, +404..+420 = the
// record's +344..+360. Returns it (0 when the allocation fails).
DECOMP(000C5DC8, DefLeaf_New5DC8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 432);
    const u32 rec = frame.addr();
    LeafDefCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), kStrAC6790, kVecZero, 1, kVec2_4AFC, 0x000C5DAC, 0, 0);
    LeafRecB(rec);
    call(ctx, DefLeaf_Read5B30, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {432}));
    if (p != 0) {
        call(ctx, m11fn::DefLeaf_Construct_000C30E8, {p, rec});
        CopyW(p + 404, rec + 344);
        CopyW(p + 408, rec + 348);
        store<u32>(p, 0x00B132D8u);
        CopyW(p + 412, rec + 352);
        CopyW(p + 416, rec + 356);
        CopyW(p + 420, rec + 360);
        ctx.fpr[1] = LoadF(rec + 360);  // the last lfs, as the original leaves f1
    }
    ret(ctx, p);
}

// void DefLeaf_Read5FF0(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DefLeaf_ReadBase_000C4020 with the same arguments, then the defaults
// record (pool 0x000C5FD4, name 0x00AC6790, LeafRecC) with +344..+372 /
// byte +376 = tmpl's +404..+432 / +436 when tmpl has flag 0x4000 in +20.
// Reads five floats into o+344..+360; an enum hash (0xE8DB4898, default 0)
// whose index in the two hashes at 0x00AC67D0 goes to o+364 when found
// (o+364 kept otherwise); floats into o+368 / +372, both then scaled by the
// pool's +24; a bool (0x99A462A8) into byte o+376. Quirk: with no copy from
// tmpl the first float's default is the pool's +12, not the record's 0.
DECOMP(000C5FF0, DefLeaf_Read5FF0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m11fn::DefLeaf_ReadBase_000C4020, {a0, c, x, tmpl, o});
    Stack frame(ctx, 400);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x000C5FD4;
    // r10 is left over from the call above (the callee ignores it).
    LeafDefCtor(ctx, rec, load<u32>(x + 12), kStrAC6790, kStrAC6790, kStrAC6790, kVec2_4AFC, 0x7641A763u, ctx.gpr[10], kPool, 0, 0);
    LeafRecC(rec);
    f64 first = LoadF(kPool + 12);
    if (tmpl != 0 && (load<u32>(tmpl + 20) & 0x4000u)) {
        for (u32 k = 0; k < 8; ++k) CopyW(rec + 344 + 4 * k, tmpl + 404 + 4 * k);
        store<u8>(rec + 376, load<u8>(tmpl + 436));
        first = LoadF(rec + 344);
    }
    static constexpr u32 kF[5] = {0xB81F0E21u, 0xB81F0D33u, 0x4BC4DED1u, 0x4BA728F9u, 0x04F32F5Cu};
    for (u32 k = 0; k < 5; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kF[k], o + 344 + 4 * k}, {k == 0 ? first : LoadF(rec + 344 + 4 * k)});
    const u32 h = rec + 384;  // the original's sp + 800
    store<u32>(h, 0);
    if (static_cast<u32>(call(ctx, xfn::XmlCursor_ReadHash, {c, 0xE8DB4898u, h, 0})) == 0) {
        const u32 v = load<u32>(h);
        for (u32 i = 0; i < 2; ++i) {
            if (load<u32>(kEnum5FF0 + 4 * i) == v) {
                store<u32>(o + 364, i);
                break;
            }
        }
    }
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x39F5434Cu, o + 368}, {LoadF(rec + 368)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xA3D3DF7Au, o + 372}, {LoadF(rec + 372)});
    const f32 k = F32(load<u32>(kPool + 24));
    const f32 v372 = F32(load<u32>(o + 372));
    const f32 a = F32(load<u32>(o + 368)) * k, b = v372 * k;
    store<f32>(o + 368, a);
    store<f32>(o + 372, b);
    ctx.fpr[1] = b;  // f1 / f2 / f3 as the original leaves them for the call
    ctx.fpr[2] = a;
    ctx.fpr[3] = v372;
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x99A462A8u, o + 376, load<u8>(rec + 376)});
}

// Def* DefLeaf_New63B0(a0, XmlCursor* c, x, const Def* tmpl)
// (called by 000CA038) The defaults record (pool 0x000C6398, record +12..
// +20 = x->12..20, LeafRecC) read by DefLeaf_Read5FF0, then a 448-byte def:
// DefLeaf_Construct_000C30E8(p, rec), vtable 0x00B130B8, +404..+432 = the
// record's +344..+372, byte +436 its +376. Returns it (0 when the
// allocation fails).
DECOMP(000C63B0, DefLeaf_New63B0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 400);
    const u32 rec = frame.addr();
    LeafDefCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), kStrAC6790, kVecZero, 1, kVec2_4AFC, 0x000C6398, 0, 0);
    LeafRecC(rec);
    call(ctx, DefLeaf_Read5FF0, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {448}));
    if (p != 0) {
        call(ctx, m11fn::DefLeaf_Construct_000C30E8, {p, rec});
        CopyW(p + 404, rec + 344);
        CopyW(p + 408, rec + 348);
        CopyW(p + 412, rec + 352);
        store<u32>(p, 0x00B130B8u);
        CopyW(p + 416, rec + 356);
        CopyW(p + 420, rec + 360);
        store<u32>(p + 424, load<u32>(rec + 364));
        CopyW(p + 428, rec + 368);
        CopyW(p + 432, rec + 372);
        store<u8>(p + 436, load<u8>(rec + 376));
        ctx.fpr[1] = LoadF(rec + 372);  // the last lfs, as the original leaves f1
    }
    ret(ctx, p);
}
