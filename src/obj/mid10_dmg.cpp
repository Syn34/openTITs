// Game code, medium sweep part 10: a damage / collision def reader that
// builds its defaults with Def_Construct74FD80 (the 001A9EA8 family).
#include "mid10.h"

using namespace sly4;
using namespace sly4::mid10;

namespace {

constexpr u32 DamageColDef_Read_000C85A8 = 0x000C85A8;  // (a0, c, x, tmpl, o): the base reader
constexpr u32 Def_Construct74FD80 = 0x0074FD80;
constexpr u32 XmlCursor_DocPath = 0x004F8944;  // (a0)
constexpr u32 Thunk_000E27FC = 0x000E27FC;     // (Def* p, const Record* rec): the def base constructor
constexpr u32 DamageColDef_ReadA9EA8 = 0x001A9EA8;
constexpr u32 kStrA9EA8 = 0x00AD13B8;  // the def name (both string arguments)
constexpr u32 kVecZero = 0x01164B40;

// The defaults record of the 001A9EA8 family: Def_Construct74FD80(rec, a,
// b, c, r7, r8 (both unused), hash 0x9260C26E, the zero vec3 0x01164B40;
// f1..f13 = pool +0, +16, +12 x4, +4, +8, +16, +12 x4; stack floats s204..
// s228 = pool +12, s340 / s348 = pool +16, flag s300 set, the rest 0), then
// +4 |= 0x2000 and the extension +224..+240 zeroed (DmgRecDefaults).
void DmgRecCtor(PPUContext& ctx, u32 rec, u32 a, u32 b, u32 c, u32 r7, u32 r8, u32 pool) {
    const f64 k0 = LoadF(pool), k4 = LoadF(pool + 4), k8 = LoadF(pool + 8), k12 = LoadF(pool + 12), k16 = LoadF(pool + 16);
    const u64 f3w = load<u32>(pool + 12), f2w = load<u32>(pool + 16);
    // Stack slots 112..192 are not set by the original (not read by the callee).
    callf(ctx, Def_Construct74FD80,
          {rec, a, b, c, r7, r8, 0x9260C26Eu, kVecZero,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // slots 112..192
           f3w, f3w, f3w, f3w,                  // s204..s228 (slots 200..224)
           0, 0, 0, 0, 0, 0, 0, 0,              // s232..s288
           1,                                   // s296
           0, 0, 0, 0,                          // s304..s328
           f2w, f2w, 0},                        // s340, s348 (slots 336 / 344), s352
          {k0, k16, k12, k12, k12, k12, k4, k8, k16, k12, k12, k12, k12});
}
void DmgRecDefaults(PPUContext& ctx, u32 rec, u32 a, u32 b, u32 c, u32 r7, u32 r8, u32 pool) {
    DmgRecCtor(ctx, rec, a, b, c, r7, r8, pool);
    const u32 w4 = load<u32>(rec + 4) | 0x2000u;
    for (u32 k = 224; k < 240; k += 4) store<u32>(rec + k, 0);
    store<u8>(rec + 240, 0);
    store<u32>(rec + 4, w4);
}

// The record's vector +124 destroyed.
void DmgRecDestroy(PPUContext& ctx, u32 rec) {
    const u32 v = load<u32>(rec + 124);
    if (v != 0) {
        call(ctx, m1fn::PtrVector_Clear, {v});
        call(ctx, m1fn::Nop, {v});
        call(ctx, m1fn::SmallFree, {load<u32>(rec + 124), 20});
    }
}

}  // namespace

// void DamageColDef_ReadA9EA8(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_Read_000C85A8 with the same arguments, then the defaults: a
// def record on the stack built by Def_Construct74FD80 (record +12 = x->12,
// name 0x00AD13B8, hash 0x9260C26E, the floats of the literal pool
// 0x001A9E94, flag s300 set) with +4 |= 0x2000 and +224..+240 zeroed (or
// tmpl's +304..+320 when tmpl has flag 0x2000 in +24; c's document path is
// noted otherwise). Reads four floats (0x4E14723F, 0x6A96C7DB, 0xEAEDFCE1,
// 0x0A924DA2) into o+224..+236 and a bool (0x317F6BB4) into o+240, then
// destroys the record's vector +124. Quirk: with no tmpl the first float's
// default is the pool's +4, not the record's 0.
DECOMP(001A9EA8, DamageColDef_ReadA9EA8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    Stack frame(ctx, 320);
    const u32 rec = frame.addr();
    // r8 is left over from the call above (the callee ignores it).
    DmgRecDefaults(ctx, rec, load<u32>(x + 12), kStrA9EA8, kStrA9EA8, 1, 0x001B0000, 0x001A9E94);
    const f64 k4 = LoadF(0x001A9E94 + 4);
    f64 first = k4;
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x2000u) {
            for (u32 k = 0; k < 4; ++k) CopyW(rec + 224 + 4 * k, tmpl + 304 + 4 * k);
            store<u8>(rec + 240, load<u8>(tmpl + 320));
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
        first = LoadF(rec + 224);
    }
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x4E14723Fu, o + 224}, {first});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x6A96C7DBu, o + 228}, {LoadF(rec + 228)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xEAEDFCE1u, o + 232}, {LoadF(rec + 232)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x0A924DA2u, o + 236}, {LoadF(rec + 236)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x317F6BB4u, o + 240, load<u8>(rec + 240)});
    DmgRecDestroy(ctx, rec);
}

// Def* DamageColDef_NewAA158(a0, XmlCursor* c, x, const Def* tmpl)
// (called by 000CA038) The defaults record (pool 0x001AA140, record +12..
// +20 = x->12..20) read by DamageColDef_ReadA9EA8(a0, c, x, tmpl, rec), then
// a 336-byte def: Thunk_000E27FC(p, rec), vtable 0x00B152C8, +304..+320 =
// the record's +224..+240. Returns it (0 when the allocation fails).
DECOMP(001AA158, DamageColDef_NewAA158) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 304);
    const u32 rec = frame.addr();
    DmgRecDefaults(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 0x001AA140, load<u32>(x + 20), 0x001AA140);
    call(ctx, DamageColDef_ReadA9EA8, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {336}));
    if (p != 0) {
        call(ctx, Thunk_000E27FC, {p, rec});
        store<u32>(p, 0x00B152C8u);
        for (u32 k = 0; k < 4; ++k) CopyW(p + 304 + 4 * k, rec + 224 + 4 * k);
        store<u8>(p + 320, load<u8>(rec + 240));
        ctx.fpr[1] = LoadF(rec + 236);  // the last lfs, as the original leaves f1
    }
    DmgRecDestroy(ctx, rec);
    ret(ctx, p);
}

namespace {

// The 001AC540 variant of the defaults: +4 |= 0x8000, +224..+232 zeroed,
// bits 0 / 1 of +236 cleared.
void DmgRecDefaults8000(PPUContext& ctx, u32 rec, u32 a, u32 b, u32 c, u32 r7, u32 r8, u32 pool) {
    DmgRecCtor(ctx, rec, a, b, c, r7, r8, pool);
    const u32 w236 = load<u32>(rec + 236), w4 = load<u32>(rec + 4);
    for (u32 k = 224; k < 236; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 4, w4 | 0x8000u);
    store<u32>(rec + 236, w236 & 0x3FFFFFFFu);
}

}  // namespace

// void DamageColDef_ReadAC540(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_Read_000C85A8 with the same arguments, then the defaults
// record (pool 0x001AC528, name 0x00AD13B8, DmgRecDefaults8000) with +224..
// +236 = tmpl's +304..+316 (bits 0 / 1 of +316 only) when tmpl has flag
// 0x8000 in +24 (c's document path noted when it lacks it); reads two
// hashes (0xB90456D0, 0xB0804865) into o+224 / +228, a float (0x80E60B5A)
// into o+232 and two bools (0x91863F0B, 0x0CF2D02B) into bits 0 / 1 of
// o+236 (a stack byte, cleared before the first only), then destroys the
// record's vector +124.
DECOMP(001AC540, DamageColDef_ReadAC540) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    Stack frame(ctx, 304);
    const u32 rec = frame.addr();
    // r8 is left over from the call above (the callee ignores it).
    DmgRecDefaults8000(ctx, rec, load<u32>(x + 12), kStrA9EA8, kStrA9EA8, 1, 0x001B0000, 0x001AC528);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x8000u) {
            CopyW(rec + 224, tmpl + 304);
            CopyW(rec + 228, tmpl + 308);
            CopyW(rec + 232, tmpl + 312);
            store<u32>(rec + 236, (load<u32>(rec + 236) & 0x3FFFFFFFu) | (load<u32>(tmpl + 316) & 0xC0000000u));
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xB90456D0u, o + 224, load<u32>(rec + 224)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xB0804865u, o + 228, load<u32>(rec + 228)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x80E60B5Au, o + 232}, {LoadF(rec + 232)});
    const u32 b = rec + 240;
    store<u8>(b, 0);
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x91863F0Bu, b, load<u32>(rec + 236) >> 31});
    store<u32>(o + 236, PutBit(load<u32>(o + 236), 0, load<u8>(b)));
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x0CF2D02Bu, b, (load<u32>(rec + 236) >> 30) & 1u});
    const u32 w = PutBit(load<u32>(o + 236), 1, load<u8>(b));
    store<u32>(o + 236, w);
    ctx.gpr[3] = w;  // the original's r3 when the record has no vector
    DmgRecDestroy(ctx, rec);
}

// Def* DamageColDef_NewAC858(a0, XmlCursor* c, x, const Def* tmpl)
// (called by 000CA038 family) The defaults record (pool 0x001AC840,
// DmgRecDefaults8000, record +12..+20 = x->12..20) read by
// DamageColDef_ReadAC540, then a 320-byte def: Thunk_000E27FC(p, rec),
// vtable 0x00B15748, +304..+312 = the record's +224..+232, bits 0 / 1 of
// +316 from its +236 (stored twice). Returns it (0 when the allocation
// fails).
DECOMP(001AC858, DamageColDef_NewAC858) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 288);
    const u32 rec = frame.addr();
    DmgRecDefaults8000(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 0x001AC840, load<u32>(x + 20), 0x001AC840);
    call(ctx, 0x001AC540, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {320}));
    if (p != 0) {
        call(ctx, Thunk_000E27FC, {p, rec});
        const u32 w316 = load<u32>(p + 316) & 0x7FFFFFFFu;
        store<u32>(p, 0x00B15748u);
        CopyW(p + 304, rec + 224);
        CopyW(p + 308, rec + 228);
        CopyW(p + 312, rec + 232);
        u32 w = w316 | (load<u32>(rec + 236) & 0x80000000u);
        store<u32>(p + 316, w);
        w = (w & ~0x40000000u) | (load<u32>(rec + 236) & 0x40000000u);
        store<u32>(p + 316, w);
        ctx.fpr[1] = LoadF(rec + 232);  // the last lfs, as the original leaves f1
    }
    DmgRecDestroy(ctx, rec);
    ret(ctx, p);
}

namespace {

// The 001AB190 variant of the defaults: +224..+240 zeroed, +244 -1, +248
// bit 0 set / bit 1 cleared, +4 |= 0x4000.
void DmgRecDefaults4000(PPUContext& ctx, u32 rec, u32 a, u32 b, u32 c, u32 r7, u32 r8, u32 pool) {
    DmgRecCtor(ctx, rec, a, b, c, r7, r8, pool);
    const u32 w248 = load<u32>(rec + 248);
    for (u32 k = 224; k <= 240; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 244, 0xFFFFFFFFu);
    store<u32>(rec + 248, (w248 | 0x80000000u) & ~0x40000000u);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x4000u);
}

}  // namespace

// void DamageColDef_ReadAB190(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_Read_000C85A8 with the same arguments, then the defaults
// record (pool 0x001AB17C, DmgRecDefaults4000) with +224..+248 = tmpl's
// +304..+328 (bits 0 / 1 of +328 only) when tmpl has flag 0x4000 in +24 (c's
// document path noted when it lacks it); reads five floats (0x8722570B,
// 0x1E6104E5, 0x08B6A993, 0x14EE18CD, 0x4E14723F) into o+224..+240, an s32
// (0x4C238808) into o+244 (decremented, not below 0, when found) and two
// bools (0x609A61B6, 0x994327AD) into bits 0 / 1 of o+248 (through a stack
// byte), then destroys the record's vector +124. Quirk: with no tmpl the
// first float's default is the pool's +4, not the record's 0.
DECOMP(001AB190, DamageColDef_ReadAB190) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    Stack frame(ctx, 320);
    const u32 rec = frame.addr();
    // r8 is left over from the call above (the callee ignores it).
    DmgRecDefaults4000(ctx, rec, load<u32>(x + 12), kStrA9EA8, kStrA9EA8, 1, 0x001B0000, 0x001AB17C);
    f64 first = LoadF(0x001AB17C + 4);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x4000u) {
            for (u32 k = 0; k < 6; ++k) CopyW(rec + 224 + 4 * k, tmpl + 304 + 4 * k);
            store<u32>(rec + 248, (load<u32>(rec + 248) & 0x3FFFFFFFu) | (load<u32>(tmpl + 328) & 0xC0000000u));
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
        first = LoadF(rec + 224);
    }
    static constexpr u32 kHash[5] = {0x8722570Bu, 0x1E6104E5u, 0x08B6A993u, 0x14EE18CDu, 0x4E14723Fu};
    for (u32 k = 0; k < 5; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kHash[k], o + 224 + 4 * k}, {k == 0 ? first : LoadF(rec + 224 + 4 * k)});
    const u64 def244 = static_cast<u64>(static_cast<s64>(load<s32>(rec + 244)));
    if (static_cast<u32>(call(ctx, xfn::XmlCursor_ReadS32, {c, 0x4C238808u, o + 244, def244})) == 0) {
        const s32 v = load<s32>(o + 244) - 1;
        store<s32>(o + 244, v < 0 ? 0 : v);
    }
    const u32 b = rec + 256;
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x609A61B6u, b, load<u32>(rec + 248) >> 31});
    store<u32>(o + 248, PutBit(load<u32>(o + 248), 0, load<u8>(b)));
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x994327ADu, b, (load<u32>(rec + 248) >> 30) & 1u});
    const u32 w = PutBit(load<u32>(o + 248), 1, load<u8>(b));
    store<u32>(o + 248, w);
    ctx.gpr[3] = w;  // the original's r3 when the record has no vector
    DmgRecDestroy(ctx, rec);
}

// Def* DamageColDef_NewAB548(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (pool 0x001AB534, DmgRecDefaults4000, record +12..+20
// = x->12..20) read by DamageColDef_ReadAB190, then a 336-byte def:
// Thunk_000E27FC(p, rec), vtable 0x00B15508, +304..+324 = the record's
// +224..+244, bits 0 / 1 of +328 from its +248 (stored twice). Returns it
// (0 when the allocation fails).
DECOMP(001AB548, DamageColDef_NewAB548) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 304);
    const u32 rec = frame.addr();
    DmgRecDefaults4000(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 0x001AB534, load<u32>(x + 20), 0x001AB534);
    call(ctx, 0x001AB190, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {336}));
    if (p != 0) {
        call(ctx, Thunk_000E27FC, {p, rec});
        store<u32>(p, 0x00B15508u);
        for (u32 k = 0; k < 4; ++k) CopyW(p + 304 + 4 * k, rec + 224 + 4 * k);
        const u32 w328 = load<u32>(p + 328) & 0x7FFFFFFFu;
        CopyW(p + 320, rec + 240);
        store<u32>(p + 324, load<u32>(rec + 244));
        u32 w = w328 | (load<u32>(rec + 248) & 0x80000000u);
        store<u32>(p + 328, w);
        w = (w & ~0x40000000u) | (load<u32>(rec + 248) & 0x40000000u);
        store<u32>(p + 328, w);
        ctx.fpr[1] = LoadF(rec + 240);  // the last lfs, as the original leaves f1
    }
    DmgRecDestroy(ctx, rec);
    ret(ctx, p);
}

namespace {

// The 001B2118 variant of the defaults: +224..+232 0, f32 +236 1.0, +240
// 2.0, +244 / +248 1.0, +252 0, +4 |= 0x10000.
void DmgRecDefaults10000(PPUContext& ctx, u32 rec, u32 a, u32 b, u32 c, u32 r7, u32 r8, u32 pool) {
    DmgRecCtor(ctx, rec, a, b, c, r7, r8, pool);
    static constexpr u32 kInit[8] = {0, 0, 0, 0x3F800000u, 0x40000000u, 0x3F800000u, 0x3F800000u, 0};
    for (u32 k = 0; k < 8; ++k) store<u32>(rec + 224 + 4 * k, kInit[k]);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x10000u);
}

}  // namespace

// void DamageColDef_ReadB2118(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_Read_000C85A8 with the same arguments, then the defaults
// record (pool 0x001B2104, DmgRecDefaults10000) with +224..+252 = tmpl's
// +304..+332 when tmpl has flag 0x10000 in +24 (c's document path noted
// when it lacks it); reads three hashes (0x8CD5EC64, 0xF7DACE90,
// 0x4A104549) into o+224..+232 and five floats (0xCEA8E637, 0x06CD2139,
// 0x0DE78502, 0x0B0BD5E6, 0x285C1380) into o+236..+252, raises o+236..+248
// to at least the pool's +4 and lowers o+236 to o+240, then destroys the
// record's vector +124.
DECOMP(001B2118, DamageColDef_ReadB2118) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    Stack frame(ctx, 320);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001B2104;
    // r8 is left over from the call above (the callee ignores it).
    DmgRecDefaults10000(ctx, rec, load<u32>(x + 12), kStrA9EA8, kStrA9EA8, 1, 0x001B0000, kPool);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x10000u) {
            for (u32 k = 0; k < 8; ++k) CopyW(rec + 224 + 4 * k, tmpl + 304 + 4 * k);
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    static constexpr u32 kHash[8] = {0x8CD5EC64u, 0xF7DACE90u, 0x4A104549u, 0xCEA8E637u, 0x06CD2139u, 0x0DE78502u, 0x0B0BD5E6u, 0x285C1380u};
    for (u32 k = 0; k < 3; ++k) call(ctx, xfn::XmlCursor_ReadHash, {c, kHash[k], o + 224 + 4 * k, load<u32>(rec + 224 + 4 * k)});
    for (u32 k = 3; k < 8; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kHash[k], o + 224 + 4 * k}, {LoadF(rec + 224 + 4 * k)});
    const u32 lo = load<u32>(kPool + 4);
    u32 v[4];
    for (u32 k = 0; k < 4; ++k) v[k] = load<u32>(o + 236 + 4 * k);
    for (u32 k = 0; k < 4; ++k) {
        if (F32(v[k]) < F32(lo)) v[k] = lo;
        store<u32>(o + 236 + 4 * k, v[k]);
    }
    if (F32(v[0]) > F32(v[1])) store<u32>(o + 236, v[1]);
    ctx.fpr[1] = static_cast<f64>(F32(v[3]));
    DmgRecDestroy(ctx, rec);
}

// Def* DamageColDef_NewB24D0(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (pool 0x001B24B8, DmgRecDefaults10000, record +12..
// +20 = x->12..20) read by DamageColDef_ReadB2118, then a 336-byte def:
// Thunk_000E27FC(p, rec), vtable 0x00B15E18, +304..+332 = the record's
// +224..+252. Returns it (0 when the allocation fails).
DECOMP(001B24D0, DamageColDef_NewB24D0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 304);
    const u32 rec = frame.addr();
    DmgRecDefaults10000(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 0x001B24B8, load<u32>(x + 20), 0x001B24B8);
    call(ctx, 0x001B2118, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {336}));
    if (p != 0) {
        call(ctx, Thunk_000E27FC, {p, rec});
        store<u32>(p, 0x00B15E18u);
        for (u32 k = 0; k < 8; ++k) CopyW(p + 304 + 4 * k, rec + 224 + 4 * k);
        ctx.fpr[1] = LoadF(rec + 252);  // the last lfs, as the original leaves f1
    }
    DmgRecDestroy(ctx, rec);
    ret(ctx, p);
}

namespace {

// The 001AD1F8 variant of the defaults: f32 +288..+296 = the zero vec3,
// +304 / +308 0, f32 +312 1.0, bit 0x80 of byte +316 cleared, +4 |=
// 0x80000, the seven {+224 hash 0, +228 f32 -1} pairs, f32 +36 -1, byte +44
// 0.
void DmgRecDefaults80000(PPUContext& ctx, u32 rec, u32 a, u32 b, u32 c, u32 r7, u32 r8, u32 pool) {
    DmgRecCtor(ctx, rec, a, b, c, r7, r8, pool);
    CopyW(rec + 288, kVecZero);
    const u32 b316 = load<u8>(rec + 316) & 0x7Fu;
    CopyW(rec + 292, kVecZero + 4);
    const u32 w4 = load<u32>(rec + 4);
    CopyW(rec + 296, kVecZero + 8);
    store<u32>(rec + 304, 0);
    store<u32>(rec + 308, 0);
    store<u32>(rec + 312, 0x3F800000u);
    store<u8>(rec + 316, static_cast<u8>(b316));
    store<u32>(rec + 4, w4 | 0x80000u);
    for (u32 k = 0; k < 7; ++k) store<u32>(rec + 224 + 8 * k, 0);
    for (u32 k = 0; k < 7; ++k) store<u32>(rec + 228 + 8 * k, 0xBF800000u);
    store<u32>(rec + 36, 0xBF800000u);
    store<u8>(rec + 44, 0);
    ctx.fpr[1] = LoadF(kVecZero + 8);  // the last lfs, as the original leaves f1
}

}  // namespace

// void DamageColDef_ReadAD1F8(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_Read_000C85A8 with the same arguments, then the defaults
// record (pool 0x001AD1E0, record +12..+20 = x->12..20, DmgRecDefaults80000)
// with, when tmpl is set, byte +44 = x->44 and (when tmpl has flag 0x80000
// in +24) the seven {hash 0, tmpl+308[2k]} pairs and +288..+316 from tmpl's
// +368..+396 (c's document path noted when it lacks it). Copies the
// record's +12..+20 and byte +44 into o; reads a vec3 (0x22167A42) into
// o+288, two hashes into o+304 / +308, a float (0x3E1A4D9C) into o+312
// (1.0 when below the pool's +4), seven floats into o+228[2k] and seven
// hashes into o+224[2k], and a bool (0xBD75D672) into bit 0x80 of byte
// o+316 (through a stack byte preset from it); then destroys the record's
// vector +124.
DECOMP(001AD1F8, DamageColDef_ReadAD1F8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    Stack frame(ctx, 416);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001AD1E0;
    DmgRecDefaults80000(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), load<u32>(x + 20), 1, kPool);
    if (tmpl != 0) {
        store<u8>(rec + 44, load<u8>(x + 44));
        if (load<u32>(tmpl + 24) & 0x80000u) {
            for (u32 k = 0; k < 7; ++k) {
                store<u32>(rec + 224 + 8 * k, 0);
                CopyW(rec + 228 + 8 * k, tmpl + 308 + 8 * k);
            }
            for (u32 k = 0; k < 3; ++k) CopyW(rec + 288 + 4 * k, tmpl + 368 + 4 * k);
            store<u32>(rec + 304, load<u32>(tmpl + 384));
            store<u32>(rec + 308, load<u32>(tmpl + 388));
            CopyW(rec + 312, tmpl + 392);
            store<u8>(rec + 316, static_cast<u8>((load<u8>(rec + 316) & 0x7Fu) | (load<u8>(tmpl + 396) & 0x80u)));
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    store<u32>(o + 12, load<u32>(rec + 12));
    store<u32>(o + 16, load<u32>(rec + 16));
    store<u32>(o + 20, load<u32>(rec + 20));
    store<u8>(o + 44, load<u8>(rec + 44));
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x22167A42u, o + 288, rec + 288});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x3A15BFABu, o + 304, load<u32>(rec + 304)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xC03F596Fu, o + 308, load<u32>(rec + 308)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x3E1A4D9Cu, o + 312}, {LoadF(rec + 312)});
    if (F32(load<u32>(o + 312)) < F32(load<u32>(kPool + 4))) store<u32>(o + 312, 0x3F800000u);
    static constexpr u32 kF[7] = {0xD5F932F0u, 0xEABE8E36u, 0x800ACF3Au, 0xF9217A56u, 0x7BA17740u, 0xA9262FCFu, 0xA88C3581u};
    static constexpr u32 kH[7] = {0x099867CCu, 0x20B43DD4u, 0x26F1B3DCu, 0x2F1729F4u, 0x22885BE2u, 0x095F2EEDu, 0x314C79A3u};
    for (u32 k = 0; k < 7; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kF[k], o + 228 + 8 * k}, {LoadF(rec + 228 + 8 * k)});
    for (u32 k = 0; k < 7; ++k) call(ctx, xfn::XmlCursor_ReadHash, {c, kH[k], o + 224 + 8 * k, load<u32>(rec + 224 + 8 * k)});
    const u32 b = rec + 320;
    store<u8>(b, static_cast<u8>((load<u8>(o + 316) >> 7) & 1u));
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xBD75D672u, b, (load<u8>(rec + 316) >> 7) & 1u});
    const u32 v = (load<u8>(o + 316) & 0x7Fu) | ((load<u8>(b) & 1u) << 7);
    store<u8>(o + 316, static_cast<u8>(v));
    ctx.gpr[3] = v;  // the original's r3 when the record has no vector
    DmgRecDestroy(ctx, rec);
}

// Def* DamageColDef_NewAD7F0(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (pool 0x001AD7D8, name 0x00AD13B8, DmgRecDefaults80000)
// read by DamageColDef_ReadAD1F8, then a 400-byte def (Mem_Alloc(400, 0,
// 16)): Thunk_000E27FC(p, rec), vtable 0x00B15970, +368..+396 = the
// record's +288..+316 (bit 0x80 of the byte only), the seven pairs +304[2k]
// = the record's +224[2k]. Returns it (0 when the allocation fails).
DECOMP(001AD7F0, DamageColDef_NewAD7F0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 384);
    const u32 rec = frame.addr();
    // r8 is whatever the caller left (the callee ignores it).
    DmgRecDefaults80000(ctx, rec, load<u32>(x + 12), kStrA9EA8, kStrA9EA8, 1, ctx.gpr[8], 0x001AD7D8);
    call(ctx, 0x001AD1F8, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {400, 0, 16}));
    if (p != 0) {
        call(ctx, Thunk_000E27FC, {p, rec});
        store<u32>(p, 0x00B15970u);
        for (u32 k = 0; k < 3; ++k) CopyW(p + 368 + 4 * k, rec + 288 + 4 * k);
        store<u32>(p + 384, load<u32>(rec + 304));
        const u32 b396 = load<u8>(p + 396);
        store<u32>(p + 388, load<u32>(rec + 308));
        CopyW(p + 392, rec + 312);
        store<u8>(p + 396, static_cast<u8>((b396 & 0x7Fu) | (load<u8>(rec + 316) & 0x80u)));
        for (u32 k = 0; k < 7; ++k) {
            store<u32>(p + 304 + 8 * k, load<u32>(rec + 224 + 8 * k));
            CopyW(p + 308 + 8 * k, rec + 228 + 8 * k);
        }
        ctx.fpr[1] = LoadF(rec + 276);  // the last lfs, as the original leaves f1
    }
    DmgRecDestroy(ctx, rec);
    ret(ctx, p);
}
