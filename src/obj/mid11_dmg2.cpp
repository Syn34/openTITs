// Game code, medium sweep part 11: damage / collision def readers and
// factories on the base reader DamageColDef_Read_000C85A8 (the factories
// are called by 000CA038, each reader only by its factory).
#include "mid11.h"

using namespace sly4;
using namespace sly4::mid11;

namespace {

constexpr u32 kEnum27873A = 0x00AD1EBC;  // u32[1]: the hash 0x0027873A must name
constexpr u32 kEnum27873A_2 = 0x00AD2018;  // the same for 001EC7F8
constexpr u32 kStrAD1EC8 = 0x00AD1EC8;   // the attribute name looked up when it does not

// fmuls on two register doubles: one rounding to single.
inline f64 Muls(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a * b)); }

// The 001BD668 defaults (rec +224 0, +240..+248 the zero vec3, +4 |= 0x200,
// the ctor's +160 / +168 / +176 / +188 / +196 0, +220 |= 0x04000000, f32
// +36 / +40 -1, byte +44 0).
void DefsBD668(PPUContext& ctx, u32 rec) {
    store<u32>(rec + 224, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 240 + 4 * k, kVecZero + 4 * k);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x200u);
    for (u32 off : {160u, 168u, 176u, 188u, 196u}) store<u32>(rec + off, 0);
    store<u32>(rec + 220, load<u32>(rec + 220) | 0x04000000u);
    store<u32>(rec + 36, 0xBF800000u);
    store<u32>(rec + 40, 0xBF800000u);
    store<u8>(rec + 44, 0);
    ctx.fpr[1] = LoadF(kVecZero + 8);  // the last lfs
}

// The 001E9F48 defaults: +240..+248 the zero vec3, +256..+268 the identity
// quaternion, +272..+284 0, f32 +288 1.0, +4 |= 0x20000000.
void DefsE9F48(PPUContext& ctx, u32 rec) {
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 240 + 4 * k, kVecZero + 4 * k);
    for (u32 k = 0; k < 4; ++k) CopyW(rec + 256 + 4 * k, kQuatIdent + 4 * k);
    for (u32 k = 0; k < 4; ++k) store<u32>(rec + 272 + 4 * k, 0);
    store<u32>(rec + 288, 0x3F800000u);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x20000000u);
    ctx.fpr[1] = LoadF(kQuatIdent + 12);  // the last lfs
}

// The 001EC7F8 defaults: +224 / +228 / +256 / +260 / +336..+344 0, the
// vec3s +240 / +272 / +288 / +304 / +320 zero, +8 |= 8, the ctor's +160 /
// +168 / +176 / +188 / +196 0.
void DefsEC7F8(u32 rec) {
    for (u32 off : {224u, 228u, 256u, 260u, 336u, 340u, 344u}) store<u32>(rec + off, 0);
    for (u32 v : {240u, 272u, 288u, 304u, 320u})
        for (u32 k = 0; k < 3; ++k) CopyW(rec + v + 4 * k, kVecZero + 4 * k);
    store<u32>(rec + 8, load<u32>(rec + 8) | 8u);
    for (u32 off : {160u, 168u, 176u, 188u, 196u}) store<u32>(rec + off, 0);
}

// The 001F5938 defaults: +224 0, +240..+248 the zero vec3, +256..+268 the
// identity quaternion, +272..+284 0, +8 |= 0x10, the ctor's +160 / +168 /
// +176 / +188 / +196 0.
void DefsF5938(PPUContext& ctx, u32 rec) {
    store<u32>(rec + 224, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 240 + 4 * k, kVecZero + 4 * k);
    for (u32 k = 0; k < 4; ++k) CopyW(rec + 256 + 4 * k, kQuatIdent + 4 * k);
    store<u32>(rec + 8, load<u32>(rec + 8) | 0x10u);
    for (u32 k = 0; k < 4; ++k) store<u32>(rec + 272 + 4 * k, 0);
    for (u32 off : {160u, 168u, 176u, 188u, 196u}) store<u32>(rec + off, 0);
    ctx.fpr[1] = LoadF(kQuatIdent + 12);  // the last lfs
    ctx.fpr[2] = LoadF(kQuatIdent + 8);
}

// The factories' record: DmgCtor(rec, x->12..20, r7 = the pool, r8 =
// x->20).
void DmgNewCtorX(PPUContext& ctx, u32 x, u32 rec, u32 pool) {
    const u32 x20 = load<u32>(x + 20);
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, pool, x20, pool);
}

// The enum read of 001DBC28 / 001EC7F8: o+224 = rec+224, then the hash
// 0x0027873A into the stack word h (not preset); when found, o+224 = 0 if
// it equals the one hash listed at tbl, else the attribute named
// 0x00AD1EC8 is looked up (the result unused). Returns the original's r3.
u64 Enum27873A(PPUContext& ctx, u32 c, u32 o, u32 rec, u32 h, u32 tbl) {
    store<u32>(o + 224, load<u32>(rec + 224));
    const u64 r = call(ctx, xfn::XmlCursor_ReadHash, {c, 0x0027873Au, h, 0});
    if (static_cast<u32>(r) != 0) return r;
    const u32 v = load<u32>(h);
    if (load<u32>(tbl) == v) {
        store<u32>(o + 224, 0);
        ctx.gpr[3] = v;  // r3 holds the hash read
        return v;
    }
    return call(ctx, m11fn::XmlCursor_FindAttrByName, {c, kStrAD1EC8});
}

}  // namespace

// Def* DamageColDef_NewBD668(a0, XmlCursor* c, x, const Def* tmpl)
// (called by 000CA038) A def record rec1 (DmgCtor with x->12..20, r7 the
// pool 0x001BD654, r8 0x92600000; DefsBD668) read by DamageColDef_Read_
// 000C85A8(a0, c, x, tmpl, rec1), its ctor fields +160..+196 cleared and
// +220 |= 0x04000000 again; a second record rec2 (name 0x00AD13B8,
// DefsBD668) with +224 / +240..+248 = tmpl's +304 / +320..+328 when tmpl
// has flag 0x200 in +24 (c's document path noted when it lacks it). Reads a
// float (0x7DDB32EA, default rec2's +224) into rec1+224 and a vec3
// (0x7DDB2524) into rec1+240, then a 336-byte def: Thunk_000E27FC(p, rec1),
// vtable 0x00B163F0, +304 / +320..+328 = rec1's +224 / +240..+248. Both
// records' vectors destroyed (rec2 first). Quirks: with no tmpl the float's
// default is the pool's +4; the vec3's default is rec1's own (rec2's copy
// from tmpl is unused).
DECOMP(001BD668, DamageColDef_NewBD668) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 512);
    const u32 rec1 = frame.addr(), rec2 = rec1 + 256;  // the original's sp + 368 / + 624
    constexpr u32 kPool = 0x001BD654;
    DmgCtor(ctx, rec1, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), kPool, 0x92600000u, kPool);
    DefsBD668(ctx, rec1);
    call(ctx, m11fn::DamageColDef_Read_000C85A8, {a0, c, x, tmpl, rec1});
    for (u32 off : {160u, 168u, 176u, 188u, 196u}) store<u32>(rec1 + off, 0);
    store<u32>(rec1 + 220, load<u32>(rec1 + 220) | 0x04000000u);
    // r7 / r8 are left over from the call above (the callee ignores them).
    DmgCtor(ctx, rec2, load<u32>(x + 12), kStrAD13B8, kStrAD13B8, ctx.gpr[7], ctx.gpr[8], kPool);
    DefsBD668(ctx, rec2);
    f64 first = LoadF(kPool + 4);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x200u) {
            CopyW(rec2 + 224, tmpl + 304);
            for (u32 k = 0; k < 3; ++k) CopyW(rec2 + 240 + 4 * k, tmpl + 320 + 4 * k);
        } else {
            call(ctx, m11fn::XmlCursor_DocPath, {a0});
        }
        first = LoadF(rec2 + 224);
    }
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x7DDB32EAu, rec1 + 224}, {first});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x7DDB2524u, rec1 + 240, rec1 + 240});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {336}));
    if (p != 0) {
        call(ctx, m11fn::Thunk_000E27FC, {p, rec1});
        store<u32>(p, 0x00B163F0u);
        CopyW(p + 304, rec1 + 224);
        for (u32 k = 0; k < 3; ++k) CopyW(p + 320 + 4 * k, rec1 + 240 + 4 * k);
        ctx.fpr[1] = LoadF(rec1 + 248);  // the last lfs, as the original leaves f1
    }
    DmgDestroy(ctx, rec2);
    DmgDestroy(ctx, rec1);
    ret(ctx, p);
}

// void DamageColDef_ReadE9F48(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_Read_000C85A8, then the defaults record (pool 0x001E9F2C,
// name 0x00AD1E98, DefsE9F48) with +240..+288 from tmpl's +320..+368 (all
// but +332) when tmpl has flag 0x20000000 in +24. Reads a vec3
// (0x81785FF8) into o+240, an euler vec3 (0x91B5F872, default zero) onto
// the stack, four hashes into o+272..+284, a float (0x35F1E76E) into
// o+288; then o+256..+268 = the quaternion of the euler angles scaled by
// the pool's +20 (yaw y, roll z, pitch x: sin / cos via Math_Sinf /
// Math_Cosf), and destroys the record's vector +124.
DECOMP(001E9F48, DamageColDef_ReadE9F48) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m11fn::DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    Stack frame(ctx, 448);
    const u32 e = frame.addr(), rec = e + 16;  // the original's sp + 368 / + 384
    constexpr u32 kPool = 0x001E9F2C;
    // r8 is left over from the call above (the callee ignores it).
    DmgCtor(ctx, rec, load<u32>(x + 12), kStrAD1E98, kStrAD1E98, 1, ctx.gpr[8], kPool);
    DefsE9F48(ctx, rec);
    if (DmgTmpl(ctx, a0, tmpl, 0x20000000u)) {
        for (u32 k = 0; k < 3; ++k) CopyW(rec + 240 + 4 * k, tmpl + 320 + 4 * k);
        for (u32 k = 0; k < 4; ++k) CopyW(rec + 256 + 4 * k, tmpl + 336 + 4 * k);
        for (u32 k = 0; k < 4; ++k) store<u32>(rec + 272 + 4 * k, load<u32>(tmpl + 352 + 4 * k));
        CopyW(rec + 288, tmpl + 368);
    }
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x81785FF8u, o + 240, rec + 240});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x91B5F872u, e, kVecZero});
    static constexpr u32 kH[4] = {0xC8168A44u, 0x942EEC95u, 0x7D68F822u, 0x4C4DB3D0u};
    for (u32 k = 0; k < 4; ++k) call(ctx, xfn::XmlCursor_ReadHash, {c, kH[k], o + 272 + 4 * k, load<u32>(rec + 272 + 4 * k)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x35F1E76Eu, o + 288}, {LoadF(rec + 288)});
    const f64 k = LoadF(kPool + 20);
    const f64 ay = Muls(LoadF(e + 4), k), az = Muls(LoadF(e + 8), k), ax = Muls(LoadF(e), k);
    const f64 cy = callf(ctx, m11fn::Math_Cosf, {}, {ay});
    const f64 cz = callf(ctx, m11fn::Math_Cosf, {}, {az});
    const f64 cx = callf(ctx, m11fn::Math_Cosf, {}, {ax});
    const f64 sy = callf(ctx, m11fn::Math_Sinf, {}, {ay});
    const f64 sz = callf(ctx, m11fn::Math_Sinf, {}, {az});
    const f64 sx = callf(ctx, m11fn::Math_Sinf, {}, {ax});
    const f64 ss = Muls(sy, sz), cc = Muls(cy, cz), cs = Muls(cz, sy), csz = Muls(cy, sz);
    const f64 t6 = Muls(ss, sx), t7 = Muls(cc, sx), t8 = Muls(cs, sx), t1 = Muls(csz, sx);
    const f32 q3 = static_cast<f32>(std::fma(cc, cx, -t6));
    const f32 q0 = static_cast<f32>(std::fma(ss, cx, t7));
    const f32 q2 = static_cast<f32>(std::fma(csz, cx, -t8));
    const f32 q1 = static_cast<f32>(std::fma(cs, cx, t1));
    store<f32>(o + 268, q3);
    store<f32>(o + 256, q0);
    store<f32>(o + 264, q2);
    store<f32>(o + 260, q1);
    ctx.fpr[1] = q1;  // as the original leaves f1
    DmgDestroy(ctx, rec);
}

// Def* DamageColDef_NewEA3B8(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (pool 0x001EA3A4, DmgNewCtorX, DefsE9F48) read by
// DamageColDef_ReadE9F48, then a 384-byte def: Thunk_000E27FC(p, rec),
// vtable 0x00B18230, +320..+368 = the record's +240..+288 (all but +332).
// Returns it (0 when the allocation fails).
DECOMP(001EA3B8, DamageColDef_NewEA3B8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 384);
    const u32 rec = frame.addr();
    DmgNewCtorX(ctx, x, rec, 0x001EA3A4);
    DefsE9F48(ctx, rec);
    call(ctx, 0x001E9F48, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {384}));
    if (p != 0) {
        call(ctx, m11fn::Thunk_000E27FC, {p, rec});
        store<u32>(p, 0x00B18230u);
        for (u32 k = 0; k < 3; ++k) CopyW(p + 320 + 4 * k, rec + 240 + 4 * k);
        for (u32 k = 0; k < 4; ++k) CopyW(p + 336 + 4 * k, rec + 256 + 4 * k);
        for (u32 k = 0; k < 4; ++k) store<u32>(p + 352 + 4 * k, load<u32>(rec + 272 + 4 * k));
        CopyW(p + 368, rec + 288);
        ctx.fpr[1] = LoadF(rec + 288);  // the last lfs, as the original leaves f1
    }
    DmgDestroy(ctx, rec);
    ret(ctx, p);
}

// void DamageColDef_ReadEC7F8(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_Read_000C85A8, then o's ctor fields +160 / +168 / +176 /
// +188 / +196 cleared, the defaults record (pool 0x001EC7E0 with f1 / f7
// swapped, name 0x00AD1E98, DefsEC7F8) with tmpl's +304.. fields when tmpl
// has flag 8 in +28 (c's document path noted when it lacks it); o+224 =
// the record's +224, then the 0x0027873A enum (Enum27873A, the list at
// 0x00AD2018); hashes / vec3s into o+228..+340 and a float into o+344, each
// with o's own value as the default, the vec3 o+320 scaled by the pool's
// +20; then destroys the record's vector +124.
DECOMP(001EC7F8, DamageColDef_ReadEC7F8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m11fn::DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    for (u32 off : {160u, 168u, 176u, 188u, 196u}) store<u32>(o + off, 0);
    Stack frame(ctx, 416);
    const u32 rec = frame.addr();
    constexpr u32 kPool = 0x001EC7E0;
    // r8 is left over from the call above (the callee ignores it).
    DmgCtor(ctx, rec, load<u32>(x + 12), kStrAD1E98, kStrAD1E98, 1, ctx.gpr[8], kPool, true);
    DefsEC7F8(rec);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 28) & 8u) {
            store<u32>(rec + 224, load<u32>(tmpl + 304));
            store<u32>(rec + 228, load<u32>(tmpl + 308));
            for (u32 k = 0; k < 3; ++k) CopyW(rec + 240 + 4 * k, tmpl + 320 + 4 * k);
            store<u32>(rec + 256, load<u32>(tmpl + 336));
            store<u32>(rec + 260, load<u32>(tmpl + 340));
            for (u32 k = 0; k < 3; ++k) CopyW(rec + 272 + 4 * k, tmpl + 352 + 4 * k);
            for (u32 k = 0; k < 3; ++k) CopyW(rec + 304 + 4 * k, tmpl + 384 + 4 * k);
            for (u32 k = 0; k < 3; ++k) CopyW(rec + 320 + 4 * k, tmpl + 400 + 4 * k);
            store<u32>(rec + 336, load<u32>(tmpl + 416));
        } else {
            call(ctx, m11fn::XmlCursor_DocPath, {a0});
        }
    }
    Enum27873A(ctx, c, o, rec, rec + 352, kEnum27873A_2);  // h: the original's sp + 720
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x161851F3u, o + 228, load<u32>(o + 228)});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0xB8B33A30u, o + 240, o + 240});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xEFE3F2BEu, o + 256, load<u32>(o + 256)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x80F96352u, o + 260, load<u32>(o + 260)});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x43DAD405u, o + 272, o + 272});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0xE7590BDAu, o + 288, o + 288});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x02528C1Cu, o + 304, o + 304});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x0252939Fu, o + 320, o + 320});
    const f32 k = F32(load<u32>(kPool + 20));
    const f32 v0 = F32(load<u32>(o + 320)) * k, v1 = F32(load<u32>(o + 324)) * k, v2 = F32(load<u32>(o + 328)) * k;
    store<f32>(o + 320, v0);
    store<f32>(o + 324, v1);
    store<f32>(o + 328, v2);
    ctx.fpr[2] = v0;  // f2 / f3 as the original leaves them
    ctx.fpr[3] = v1;
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x6350BDB4u, o + 336, load<u32>(o + 336)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x04623C7Cu, o + 340, load<u32>(o + 340)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x81B5B597u, o + 344}, {LoadF(o + 344)});
    DmgDestroy(ctx, rec);
}

// Def* DamageColDef_NewECD08(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (pool 0x001ECCF4, DmgNewCtorX, DefsEC7F8) read by
// DamageColDef_ReadEC7F8, then a 432-byte def: Thunk_000E27FC(p, rec),
// vtable 0x00B18680, +304..+424 = the record's +224..+344 (+232..+236,
// +244.., +264..+268 and +332 skipped as in the record layout). Returns it
// (0 when the allocation fails).
DECOMP(001ECD08, DamageColDef_NewECD08) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 416);
    const u32 rec = frame.addr();
    DmgNewCtorX(ctx, x, rec, 0x001ECCF4);
    DefsEC7F8(rec);
    ctx.fpr[1] = LoadF(kVecZero);  // the original's last lfs (f1 / f2 / f3 = the zero vec3)
    ctx.fpr[2] = LoadF(kVecZero + 4);
    ctx.fpr[3] = LoadF(kVecZero + 8);
    call(ctx, 0x001EC7F8, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {432}));
    if (p != 0) {
        call(ctx, m11fn::Thunk_000E27FC, {p, rec});
        store<u32>(p, 0x00B18680u);
        store<u32>(p + 304, load<u32>(rec + 224));
        store<u32>(p + 308, load<u32>(rec + 228));
        for (u32 k = 0; k < 3; ++k) CopyW(p + 320 + 4 * k, rec + 240 + 4 * k);
        store<u32>(p + 336, load<u32>(rec + 256));
        store<u32>(p + 340, load<u32>(rec + 260));
        for (u32 v = 0; v < 4; ++v)
            for (u32 k = 0; k < 3; ++k) CopyW(p + 352 + 16 * v + 4 * k, rec + 272 + 16 * v + 4 * k);
        store<u32>(p + 416, load<u32>(rec + 336));
        store<u32>(p + 420, load<u32>(rec + 340));
        CopyW(p + 424, rec + 344);
        ctx.fpr[1] = LoadF(rec + 344);  // the last lfs, as the original leaves f1
    }
    DmgDestroy(ctx, rec);
    ret(ctx, p);
}

// Def* XmlDef_NewF5BA8(a0, XmlCursor* c, x, const Def* tmpl)
// (called by 000CA038) The defaults record (pool 0x001F5B94, DmgNewCtorX,
// DefsF5938) read by XmlDef_Read_001F5938, then a 368-byte def:
// Thunk_000E27FC(p, rec), vtable 0x00B188C0, +304 / +320..+328 / +336..
// +364 = the record's +224 / +240..+248 / +256..+284. Returns it (0 when the allocation fails).
DECOMP(001F5BA8, XmlDef_NewF5BA8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 352);
    const u32 rec = frame.addr();
    DmgNewCtorX(ctx, x, rec, 0x001F5B94);
    DefsF5938(ctx, rec);
    call(ctx, 0x001F5938, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {368}));
    if (p != 0) {
        call(ctx, m11fn::Thunk_000E27FC, {p, rec});
        store<u32>(p, 0x00B188C0u);
        store<u32>(p + 304, load<u32>(rec + 224));
        for (u32 k = 0; k < 3; ++k) CopyW(p + 320 + 4 * k, rec + 240 + 4 * k);
        for (u32 k = 0; k < 7; ++k) CopyW(p + 336 + 4 * k, rec + 256 + 4 * k);
        store<u32>(p + 364, load<u32>(rec + 284));
        ctx.fpr[1] = LoadF(rec + 280);  // the last lfs, as the original leaves f1
    }
    DmgDestroy(ctx, rec);
    ret(ctx, p);
}

// void DamageColDef_ReadDBC28(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_Read_000C85A8, then the defaults record (pool 0x001DBC10,
// name 0x00AD1E98; +224 0, +8 |= 4) with +224 = tmpl's +304 when tmpl has
// flag 4 in +28 (c's document path noted when it lacks it); o+224 = the
// record's +224, then the 0x0027873A enum (Enum27873A, the list at
// 0x00AD1EBC); then destroys the record's vector +124.
DECOMP(001DBC28, DamageColDef_ReadDBC28) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m11fn::DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    Stack frame(ctx, 304);
    const u32 rec = frame.addr();
    // r8 is left over from the call above (the callee ignores it).
    DmgCtor(ctx, rec, load<u32>(x + 12), kStrAD1E98, kStrAD1E98, 1, ctx.gpr[8], 0x001DBC10);
    store<u32>(rec + 224, 0);
    store<u32>(rec + 8, load<u32>(rec + 8) | 4u);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 28) & 4u) {
            store<u32>(rec + 224, load<u32>(tmpl + 304));
        } else {
            call(ctx, m11fn::XmlCursor_DocPath, {a0});
        }
    }
    Enum27873A(ctx, c, o, rec, rec + 240, kEnum27873A);  // h: the original's sp + 608
    DmgDestroy(ctx, rec);
}
