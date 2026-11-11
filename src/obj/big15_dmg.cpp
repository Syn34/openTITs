// Game code, large-function sweep part 15: damage / collision def readers
// (the Def_Construct74FD80 family).
#include "big15.h"

using namespace sly4;
using namespace sly4::big15;

// void DamageColDef_ReadAFA78(a0, XmlCursor* c, x, const Def* tmpl, Rec* o)
// DamageColDef_Read_000C85A8 with the same arguments, then a defaults record
// on the stack (DmgCtor: x->12, the def name twice, r7 = 1, pool 0x001AFA5C)
// whose part +224.. is preset: the zero vec3 +224, f32s +240..+276 = 1, 6,
// 3.5, 2, 0.5, 6, 3.5, 18.8495 (0x4196CBE4), 0.0872665 (0x3DB2B8C2), 0.1,
// f32s +280..+288 0, flag bits 0x00800000 / 0x00100000 of +292 set
// (0x00400000 / 0x00200000 clear; top byte 0xFF); +4 |= 0x40000. A tmpl
// with 0x40000 in +24 supplies them instead (+304.., +320.., +360.., the top
// 12 bits of +372); a tmpl without it notes c's document path. o is then
// read from c: vec3 0xB6EFDEE3 +224 (normalized when its squared length is
// at least 1e-5), f32s 0xD62F9754 +240 .. 0xA63D55BC +276 (+268 / +272
// degrees to radians when present), the s32 0xF2D444A7 (default 0) minus 1
// into byte +292, four bools into bits 0x00800000..0x00100000 of +292 and
// f32s +280..+288. The record's vector is destroyed; r3 / f1 as the last
// call leaves them.
DECOMP(001AFA78, DamageColDef_ReadAFA78) {
    using namespace b15fn;
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 752);
    const u32 sp = frame.addr(), rec = sp + 368, d = rec + 224, tmp = sp + 672, flag = sp + 676;
    constexpr u32 kPool = 0x001AFA5C;  // f32 {100, 0, 0.3, 1, -1, 1e-5, pi/180}
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    DmgCtor(ctx, rec, load<u32>(x + 12), kStrAD13B8, kStrAD13B8, 1, ctx.gpr[8], kPool);
    for (u32 k = 0; k < 3; ++k) CopyW(d + 4 * k, kVecZero + 4 * k);
    constexpr u32 kDef[10] = {0x3F800000u, 0x40C00000u, 0x40600000u, 0x40000000u, 0x3F000000u,
                              0x40C00000u, 0x40600000u, 0x4196CBE4u, 0x3DB2B8C2u, 0x3DCCCCCDu};
    for (u32 k = 0; k < 10; ++k) store<u32>(d + 16 + 4 * k, kDef[k]);
    for (u32 k = 0; k < 3; ++k) store<u32>(d + 56 + 4 * k, 0);
    // The flag word: the frame's stale low 20 bits are kept (never read).
    const u32 flags = (load<u32>(d + 68) & 0x000FFFFFu) | 0xFF900000u;
    store<u32>(d + 68, flags);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x40000u);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x40000u) {
            for (u32 k = 0; k < 3; ++k) CopyW(d + 4 * k, tmpl + 304 + 4 * k);
            for (u32 k = 0; k < 10; ++k) CopyW(d + 16 + 4 * k, tmpl + 320 + 4 * k);
            store<u32>(d + 68, (load<u32>(tmpl + 372) & 0xFFF00000u) | (flags & 0x000FFFFFu));
            for (u32 k = 0; k < 3; ++k) CopyW(d + 56 + 4 * k, tmpl + 360 + 4 * k);
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    call(ctx, XmlCursor_ReadVec3, {c, 0xB6EFDEE3u, o + 224, d});
    RdF32(ctx, c, 0xD62F9754u, o + 240, d + 16);
    {
        const f64 vy = LfsX(o + 228), vx = LfsX(o + 224), vz = LfsX(o + 232);
        const f64 l2 = Mas(vz, vz, Mas(vx, vx, Ms(vy, vy)));
        if (l2 >= LfsX(kPool + 20)) {
            const f64 s = Ds(LfsX(kPool + 12), Sqrts(l2));
            const f64 nx = Ms(vx, s), ny = Ms(vy, s), nz = Ms(vz, s);
            Stfs(o + 224, nx);
            Stfs(o + 228, ny);
            Stfs(o + 232, nz);
        }
    }
    RdF32(ctx, c, 0x8A8E31BEu, o + 244, d + 20);
    RdF32(ctx, c, 0xC73B5193u, o + 248, d + 24);
    RdF32(ctx, c, 0x2BFE2EDDu, o + 252, d + 28);
    RdF32(ctx, c, 0x53CAF854u, o + 256, d + 32);
    RdF32(ctx, c, 0x6A4BC424u, o + 260, d + 36);
    RdF32(ctx, c, 0xDF300BEDu, o + 264, d + 40);
    if (RdF32(ctx, c, 0x26903F9Cu, o + 268, d + 44) == 0) Stfs(o + 268, Ms(LfsX(o + 268), LfsX(kPool + 24)));
    if (RdF32(ctx, c, 0xC3017FF2u, o + 272, d + 48) == 0) Stfs(o + 272, Ms(LfsX(o + 272), LfsX(kPool + 24)));
    RdF32(ctx, c, 0xA63D55BCu, o + 276, d + 52);
    call(ctx, XmlCursor_ReadS32, {c, 0xF2D444A7u, tmp, 0});
    store<u8>(o + 292, static_cast<u8>(load<u32>(tmp) - 1));
    constexpr u32 kBools[4] = {0x4D97C801u, 0x3AF62E9Eu, 0x71C52C75u, 0x66B71F2Eu};
    for (u32 k = 0; k < 4; ++k) {
        call(ctx, XmlCursor_ReadBool, {c, kBools[k], flag, Bit(load<u32>(d + 68), 8 + k)});
        const u32 m = 0x00800000u >> k;
        store<u32>(o + 292, (load<u32>(o + 292) & ~m) | ((load<u8>(flag) & 1u) ? m : 0u));
    }
    RdF32(ctx, c, 0xAA8C2237u, o + 280, d + 56);
    RdF32(ctx, c, 0x59DEB9E0u, o + 284, d + 60);
    RdF32(ctx, c, 0x721A4F25u, o + 288, d + 64);
    DmgDestroy(ctx, rec);
}

// void DamageColDef_ReadCA080(a0, XmlCursor* c, x, const Def* tmpl, Rec* o)
// DamageColDef_ReadAD1F8 with the same arguments, then a defaults record
// on the stack (DmgCtor: x->12 / x->16 / x->20, r7 = x->20, pool 0x001CA064;
// the 0x80000 group) with hashes +320..+328 0, the zero vec3 +336, hashes
// +352 / +356 0, f32s +360..+380 0, +384 -1, +388 0.349066 (0x3EB2B8C2),
// +392 0.0872665 (0x3DB2B8C2), +396 0, s32 +400 -1, bools +404..+406 1 and
// +407 0; +4 |= 0x02080000. A tmpl with 0x02000000 in +24 supplies them
// instead (from +400..+487); a tmpl without it notes c's document path. o
// is then read from c: hashes 0x4156E113 +320, 0x7997D9A2 +324, 0xC72945F3
// +328, vec3 0xCC0D6864 +336, hashes 0x7242FC8F +352, 0x53FFAFC2 +356, f32s
// +360 .. +392 (+368 / +372 / +376 at least 0 when present, +388 / +392
// degrees to radians when present), the hash 0x04F3FC3D (default 0) whose
// index in {0x13234CFF, 0x8FA5D8C3} (0x00AD16A4) goes to +396 when found,
// the s32 0x9F5A4080 +400 and bools +404..+407. The record's vector is
// destroyed; r3 as the last call leaves it, f1 the last default (or the
// scaled +392).
DECOMP(001CA080, DamageColDef_ReadCA080) {
    using namespace b15fn;
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 880);
    const u32 sp = frame.addr(), rec = sp + 368, tmp = sp + 784;
    constexpr u32 kPool = 0x001CA064;  // f32 {100, 0, 0.3, 1, -1, pi/180}
    constexpr u32 kKinds = 0x00AD16A4;  // u32[2] hashes
    call(ctx, mid11::m11fn::DamageColDef_ReadAD1F8, {a0, c, x, tmpl, o});
    const u32 x20 = load<u32>(x + 20);
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, x20, ctx.gpr[8], kPool);
    mid11::Dmg80000(ctx, rec, 0x80000);
    store<u32>(rec + 320, 0);
    store<u32>(rec + 324, 0);
    store<u32>(rec + 328, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 336 + 4 * k, kVecZero + 4 * k);
    for (u32 k = 0; k < 8; ++k) store<u32>(rec + 352 + 4 * k, 0);
    store<u32>(rec + 384, 0xBF800000u);
    store<u32>(rec + 388, 0x3EB2B8C2u);
    store<u32>(rec + 392, 0x3DB2B8C2u);
    store<u32>(rec + 396, 0);
    store<u32>(rec + 400, 0xFFFFFFFFu);
    store<u8>(rec + 404, 1);
    store<u8>(rec + 405, 1);
    store<u8>(rec + 406, 1);
    store<u8>(rec + 407, 0);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x02080000u);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x02000000u) {
            for (u32 k = 0; k < 3; ++k) CopyW(rec + 320 + 4 * k, tmpl + 400 + 4 * k);
            for (u32 k = 0; k < 3; ++k) CopyW(rec + 336 + 4 * k, tmpl + 416 + 4 * k);
            for (u32 k = 0; k < 13; ++k) CopyW(rec + 352 + 4 * k, tmpl + 432 + 4 * k);
            for (u32 k = 0; k < 4; ++k) store<u8>(rec + 404 + k, load<u8>(tmpl + 484 + k));
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    call(ctx, XmlCursor_ReadHash, {c, 0x4156E113u, o + 320, load<u32>(rec + 320)});
    call(ctx, XmlCursor_ReadHash, {c, 0x7997D9A2u, o + 324, load<u32>(rec + 324)});
    call(ctx, XmlCursor_ReadHash, {c, 0xC72945F3u, o + 328, load<u32>(rec + 328)});
    call(ctx, XmlCursor_ReadVec3, {c, 0xCC0D6864u, o + 336, rec + 336});
    call(ctx, XmlCursor_ReadHash, {c, 0x7242FC8Fu, o + 352, load<u32>(rec + 352)});
    call(ctx, XmlCursor_ReadHash, {c, 0x53FFAFC2u, o + 356, load<u32>(rec + 356)});
    RdF32(ctx, c, 0x09198A77u, o + 360, rec + 360);
    RdF32(ctx, c, 0x58E70C1Bu, o + 364, rec + 364);
    // Clamped below at 0 when present (NaN and -0 kept; always stored back).
    constexpr u32 kClamp[3] = {0x0B109E97u, 0x0B10795Bu, 0x0B13D227u};
    for (u32 k = 0; k < 3; ++k) {
        const u32 f = o + 368 + 4 * k;
        if (RdF32(ctx, c, kClamp[k], f, rec + 368 + 4 * k) == 0) store<u32>(f, LfsX(f) < LfsX(kPool + 4) ? load<u32>(kPool + 4) : load<u32>(f));
    }
    RdF32(ctx, c, 0x667AB4F5u, o + 380, rec + 380);
    RdF32(ctx, c, 0xDBA26CDAu, o + 384, rec + 384);
    if (RdF32(ctx, c, 0x2D3AC4D0u, o + 388, rec + 388) == 0) Stfs(o + 388, Ms(LfsX(o + 388), LfsX(kPool + 20)));
    if (RdF32(ctx, c, 0x2C30B0AFu, o + 392, rec + 392) == 0) {
        const f64 v = Ms(LfsX(o + 392), LfsX(kPool + 20));
        Stfs(o + 392, v);
        ctx.fpr[1] = v;  // left in f1 to the end
    }
    store<u32>(tmp, 0);
    if (call(ctx, XmlCursor_ReadHash, {c, 0x04F3FC3Du, tmp, 0}) == 0) {
        const u32 h = load<u32>(tmp);
        for (u32 k = 0; k < 2; ++k)
            if (load<u32>(kKinds + 4 * k) == h) {
                store<u32>(o + 396, k);
                break;
            }
    }
    call(ctx, XmlCursor_ReadS32, {c, 0x9F5A4080u, o + 400, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(rec + 400))))});
    call(ctx, XmlCursor_ReadBool, {c, 0x3EFBF433u, o + 404, load<u8>(rec + 404)});
    call(ctx, XmlCursor_ReadBool, {c, 0x4128CDCDu, o + 405, load<u8>(rec + 405)});
    call(ctx, XmlCursor_ReadBool, {c, 0xAD4866C3u, o + 406, load<u8>(rec + 406)});
    call(ctx, XmlCursor_ReadBool, {c, 0xC4F817C0u, o + 407, load<u8>(rec + 407)});
    DmgDestroy(ctx, rec);
}

// void DamageColDef_ReadE6300(a0, XmlCursor* c, x, const Def* tmpl, Rec* o)
// DamageColDef_Read_000C85A8 with the same arguments, then a defaults record
// on the stack (DmgCtor: x->12, the name 0x00AD1E98 twice, r7 = 1, pool
// 0x001E62E8) with hashes +224..+232 0, f32 +264 0, the zero vec3 +272,
// f32s +288 55, +292 0.5, +296 / +300 0, hashes +316..+352 0, the zero
// vec3 +368, hashes +384..+392 0, f32 +396 4; +4 |= 0x10000000. The fields
// +236..+260 and +304..+312 keep what Def_Construct74FD80 left in the
// record. A tmpl with 0x10000000 in +24 supplies them all instead (rec +k
// = tmpl +k + 80); a tmpl without it notes c's document path. o +224..+396
// is then read from c with those defaults: ten hashes +224..+260, f32
// +264, vec3 +272, seven f32s +288..+312, ten hashes +316..+352, vec3
// +368, hashes +384..+392, f32 +396. The record's vector is destroyed; r3
// / f1 as the last call leaves them.
DECOMP(001E6300, DamageColDef_ReadE6300) {
    using namespace b15fn;
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 832);
    const u32 rec = frame.addr() + 368;
    constexpr u32 kPool = 0x001E62E8;  // f32 {100, 0, 0.3, 1, -1}
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    DmgCtor(ctx, rec, load<u32>(x + 12), mid11::kStrAD1E98, mid11::kStrAD1E98, 1, ctx.gpr[8], kPool);
    for (u32 k = 0; k < 3; ++k) store<u32>(rec + 224 + 4 * k, 0);
    store<u32>(rec + 264, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 272 + 4 * k, kVecZero + 4 * k);
    store<u32>(rec + 288, 0x425C0000u);
    store<u32>(rec + 292, 0x3F000000u);
    store<u32>(rec + 296, 0);
    store<u32>(rec + 300, 0);
    for (u32 k = 0; k < 10; ++k) store<u32>(rec + 316 + 4 * k, 0);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 368 + 4 * k, kVecZero + 4 * k);
    for (u32 k = 0; k < 3; ++k) store<u32>(rec + 384 + 4 * k, 0);
    store<u32>(rec + 396, 0x40800000u);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x10000000u);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x10000000u) {
            for (u32 k = 224; k <= 264; k += 4) CopyW(rec + k, tmpl + k + 80);
            for (u32 k = 272; k <= 280; k += 4) CopyW(rec + k, tmpl + k + 80);
            for (u32 k = 288; k <= 352; k += 4) CopyW(rec + k, tmpl + k + 80);
            for (u32 k = 368; k <= 376; k += 4) CopyW(rec + k, tmpl + k + 80);
            for (u32 k = 384; k <= 396; k += 4) CopyW(rec + k, tmpl + k + 80);
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    auto hash = [&](u32 h, u32 off) { call(ctx, XmlCursor_ReadHash, {c, h, o + off, load<u32>(rec + off)}); };
    constexpr u32 kH224[10] = {0xCA9B378Eu, 0x07AB8180u, 0xCAF309B3u, 0x00F161A9u, 0xDA59999Bu,
                               0x4B3B3ECEu, 0xA4C980A9u, 0xF86846F7u, 0xA3BB4824u, 0x1F114A8Au};
    for (u32 k = 0; k < 10; ++k) hash(kH224[k], 224 + 4 * k);
    RdF32(ctx, c, 0x16DD189Au, o + 264, rec + 264);
    call(ctx, XmlCursor_ReadVec3, {c, 0x81785FF8u, o + 272, rec + 272});
    constexpr u32 kF288[7] = {0x91B5CB68u, 0x462137C5u, 0xCA7EA426u, 0x4A3F3DB5u, 0x19C09DDEu, 0xC55EF4EBu, 0x6AF36019u};
    for (u32 k = 0; k < 7; ++k) RdF32(ctx, c, kF288[k], o + 288 + 4 * k, rec + 288 + 4 * k);
    constexpr u32 kH316[10] = {0x3A15BFABu, 0xDB732E22u, 0x5977F5AFu, 0xDB1B5BFDu, 0xCD553C2Au,
                               0x3B15E5B7u, 0xCCFD6A05u, 0x098591A0u, 0x3A1BDFB9u, 0x06556689u};
    for (u32 k = 0; k < 10; ++k) hash(kH316[k], 316 + 4 * k);
    call(ctx, XmlCursor_ReadVec3, {c, 0xC88E4511u, o + 368, rec + 368});
    hash(0x675C8395u, 384);
    hash(0x8DAC3405u, 388);
    hash(0x07E04656u, 392);
    RdF32(ctx, c, 0x3EF33B23u, o + 396, rec + 396);
    DmgDestroy(ctx, rec);
}

// void DamageColDef_ReadC2058(a0, XmlCursor* c, x, const Def* tmpl, Rec* o)
// DamageColDef_ReadAD1F8 with the same arguments, then a defaults record
// on the stack (DmgCtor: x->12 / x->16 / x->20, r7 = 0x9260C26E, pool
// 0x001C2044; the 0x80000 group) with two u32 PtrVectors +320 / +340
// (capacity 4), f32 +360 -1 and bools +364..+367 = 0, 1, 0, 1; +4 |=
// 0x01000000. A tmpl with 0x01000000 in +24 supplies them instead (its
// vectors +400 / +420 pushed into the record's, +440 and the bytes
// +444..+447); a tmpl without it notes c's document path. The record's
// vectors are then pushed onto o +320 / +340. The hashes 0x2DF2E2B5
// (default 0) and k - 0x6F968BE4 (k = 1..3; default the last one read) are
// pushed onto o +320 when non-zero; the s32s 0xCBC57F7D (default -1) and k -
// 0x53158FAC (k = 1..3; default the last one) are pushed onto o +340 as
// max(v - 1, 0) when present. Then the f32 0x2DF33CE3 +360 and the bools
// 0x45703A61 / 0xB10A3CDE / 0x89D243B8 / 0x428E4540 +364..+367. The
// record's vectors are cleared (not freed) and its +124 vector destroyed;
// r3 / f1 as the last call leaves them.
DECOMP(001C2058, DamageColDef_ReadC2058) {
    using namespace b15fn;
    namespace m1 = mid1::m1fn;
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 832);
    const u32 sp = frame.addr(), rec = sp + 368, va = rec + 320, vb = rec + 340, h = sp + 736, n = sp + 740;
    constexpr u32 kPool = 0x001C2044;  // f32 {100, 0, 0.3, 1, -1}
    call(ctx, mid11::m11fn::DamageColDef_ReadAD1F8, {a0, c, x, tmpl, o});
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 0x9260C26Eu, ctx.gpr[8], kPool);
    mid11::Dmg80000(ctx, rec, 0x80000);
    mid1::PtrVecNew(ctx, va, 4, 4, 1);
    mid1::PtrVecNew(ctx, vb, 4, 4, 1);
    store<u32>(rec + 360, 0xBF800000u);
    store<u8>(rec + 364, 0);
    store<u8>(rec + 365, 1);
    store<u8>(rec + 366, 0);
    store<u8>(rec + 367, 1);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x01000000u);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x01000000u) {
            for (u32 i = 0; i < load<u16>(tmpl + 408); ++i) PushFrom(ctx, va, load<u32>(tmpl + 400) + 4 * i);
            for (u32 i = 0; i < load<u16>(tmpl + 428); ++i) PushFrom(ctx, vb, load<u32>(tmpl + 420) + 4 * i);
            CopyW(rec + 360, tmpl + 440);
            for (u32 k = 0; k < 4; ++k) store<u8>(rec + 364 + k, load<u8>(tmpl + 444 + k));
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    for (u32 i = 0; i < load<u16>(va + 8); ++i) PushFrom(ctx, o + 320, load<u32>(va) + 4 * i);
    for (u32 i = 0; i < load<u16>(vb + 8); ++i) PushFrom(ctx, o + 340, load<u32>(vb) + 4 * i);
    store<u32>(h, 0);
    call(ctx, XmlCursor_ReadHash, {c, 0x2DF2E2B5u, h, 0});
    if (load<u32>(h) != 0) PushFrom(ctx, o + 320, h);
    for (u32 k = 1; k < 4; ++k) {
        call(ctx, XmlCursor_ReadHash, {c, Sx64(k - 0x6F968BE4u), h, load<u32>(h)});
        if (load<u32>(h) != 0) PushFrom(ctx, o + 320, h);
    }
    store<u32>(n, 0xFFFFFFFFu);
    for (u32 k = 0; k < 4; ++k) {
        const u64 hash = k == 0 ? u64{0xCBC57F7Du} : Sx64(k - 0x53158FACu);
        if (call(ctx, XmlCursor_ReadS32, {c, hash, n, Sx64(load<u32>(n))}) == 0) {
            const s32 v = static_cast<s32>(load<u32>(n)) - 1;
            store<u32>(n, static_cast<u32>(v < 0 ? 0 : v));
            PushFrom(ctx, o + 340, n);
        }
    }
    RdF32(ctx, c, 0x2DF33CE3u, o + 360, rec + 360);
    call(ctx, XmlCursor_ReadBool, {c, 0x45703A61u, o + 364, load<u8>(rec + 364)});
    call(ctx, XmlCursor_ReadBool, {c, 0xB10A3CDEu, o + 365, load<u8>(rec + 365)});
    call(ctx, XmlCursor_ReadBool, {c, 0x89D243B8u, o + 366, load<u8>(rec + 366)});
    call(ctx, XmlCursor_ReadBool, {c, 0x428E4540u, o + 367, load<u8>(rec + 367)});
    call(ctx, m1::PtrVector_Clear, {vb});
    call(ctx, m1::Nop, {vb});
    call(ctx, m1::PtrVector_Clear, {va});
    call(ctx, m1::Nop, {va});
    DmgDestroy(ctx, rec);
}

// void DamageColDef_ReadC49E8(a0, XmlCursor* c, x, const Def* tmpl, Rec* o)
// DamageColDef_ReadAD1F8 with the same arguments, then a defaults record
// on the stack (DmgCtor: x->12 / x->16 / x->20, r7 = x->20, r8 = 1, pool
// 0x001C49C8; the 0x80000 group) with the zero vec3 +320, f32s +336 0,
// +340 -1, +344 0, hashes +348 / +352 0, f32s +356 / +360 -1, bools
// +364..+374 0 and six 80-byte slots at +384 {hash 0, two zero vec3s +16 /
// +32, the quaternion 0x01164C90 +48, hash +64 0, bools +68 1 / +69 0};
// +4 |= 0x00400000. A tmpl with 0x00400000 in +24 supplies them instead
// (+400.., the bools +366..+374 from bits 0x40000000..0x00400000 of +444,
// the slots by memmove from +448); a tmpl without it notes c's document
// path. o is then read from c: vec3 0x3C6AE50B +320 (degrees to radians
// when present), f32s +336..+344, hashes +348 / +352, f32s +356 / +360,
// eleven bools +364..+374; the slots copied to o +384 unless o +369 is set;
// the first six children tagged 0xEB9EE198 fill o's slots as
// DamageColDef_ReadC8138 does (hash 0x75849CFE, vec3s 0x2B7B3EB4 /
// 0x313AB0C1, the euler 0x313AB844 into the quaternion +48, hash 0x632DA119
// +64, bools 0x8141EC8A / 0xF8E455AC +68 / +69). Then +336 and +344 are
// clamped below at 0 and +366 / +367 both cleared when both are set. The
// record's vector is destroyed. r3 = 0 / the +367 byte (or SmallFree's),
// f1 = the +344 read for the clamp.
DECOMP(001C49E8, DamageColDef_ReadC49E8) {
    using namespace b15fn;
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 1408);
    const u32 sp = frame.addr(), rec = sp + 384, slots = rec + 384, ev = sp + 368;
    constexpr u32 kPool = 0x001C49C8;  // f32 {100, 0, 0.3, 1, -1, pi/180, 0.5}
    constexpr u32 kTag = 0xEB9EE198u;
    call(ctx, mid11::m11fn::DamageColDef_ReadAD1F8, {a0, c, x, tmpl, o});
    const u32 x20 = load<u32>(x + 20);
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), x20, x20, 1, kPool);
    mid11::Dmg80000(ctx, rec, 0x80000);
    for (u32 k = 0; k < 3; ++k) CopyW(rec + 320 + 4 * k, kVecZero + 4 * k);
    store<u32>(rec + 336, 0);
    store<u32>(rec + 340, 0xBF800000u);
    store<u32>(rec + 344, 0);
    store<u32>(rec + 348, 0);
    store<u32>(rec + 352, 0);
    store<u32>(rec + 356, 0xBF800000u);
    store<u32>(rec + 360, 0xBF800000u);
    for (u32 k = 0; k < 11; ++k) store<u8>(rec + 364 + k, 0);
    for (u32 i = 0; i < 6; ++i) {
        const u32 s = slots + 80 * i;
        store<u32>(s, 0);
        for (u32 k = 0; k < 3; ++k) CopyW(s + 16 + 4 * k, kVecZero + 4 * k);
        for (u32 k = 0; k < 3; ++k) CopyW(s + 32 + 4 * k, kVecZero + 4 * k);
        for (u32 k = 0; k < 4; ++k) CopyW(s + 48 + 4 * k, mid11::kQuat4C90 + 4 * k);
        store<u32>(s + 64, 0);
        store<u8>(s + 68, 1);
        store<u8>(s + 69, 0);
    }
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x00400000u);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x00400000u) {
            for (u32 k = 0; k < 3; ++k) CopyW(rec + 320 + 4 * k, tmpl + 400 + 4 * k);
            for (u32 k = 0; k < 7; ++k) CopyW(rec + 336 + 4 * k, tmpl + 416 + 4 * k);
            const u32 fl = load<u32>(tmpl + 444);
            for (u32 k = 0; k < 9; ++k) store<u8>(rec + 366 + k, static_cast<u8>(Bit(fl, 1 + k)));
            call(ctx, mid12::m12fn::Memmove, {slots, tmpl + 448, 480});
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    if (call(ctx, XmlCursor_ReadVec3, {c, 0x3C6AE50Bu, o + 320, rec + 320}) == 0) {
        const f64 k = LfsX(kPool + 20);
        const f64 vx = Ms(LfsX(o + 320), k), vy = Ms(LfsX(o + 324), k), vz = Ms(LfsX(o + 328), k);
        Stfs(o + 320, vx);
        Stfs(o + 324, vy);
        Stfs(o + 328, vz);
    }
    RdF32(ctx, c, 0x7D873487u, o + 336, rec + 336);
    RdF32(ctx, c, 0x7BC2B446u, o + 340, rec + 340);
    RdF32(ctx, c, 0x1B75F0DAu, o + 344, rec + 344);
    call(ctx, XmlCursor_ReadHash, {c, 0x2DF2E2B5u, o + 348, load<u32>(rec + 348)});
    call(ctx, XmlCursor_ReadHash, {c, 0x53FFAFC2u, o + 352, load<u32>(rec + 352)});
    RdF32(ctx, c, 0x667AB4F5u, o + 356, rec + 356);
    RdF32(ctx, c, 0x7A392528u, o + 360, rec + 360);
    constexpr u32 kBools[11] = {0xBC50985Bu, 0xCD7DD617u, 0xA4AC4BB2u, 0xC56C09B3u, 0x9698E4BFu, 0xA8F641D7u,
                                0xC53B9DE4u, 0xF7C51457u, 0x013BDD92u, 0x23872185u, 0xA8DB95B1u};
    for (u32 k = 0; k < 11; ++k) call(ctx, XmlCursor_ReadBool, {c, kBools[k], o + 364 + k, load<u8>(rec + 364 + k)});
    if (load<u8>(o + 369) == 0) call(ctx, mid12::m12fn::Memmove, {o + 384, slots, 480});
    const f64 k20 = LfsX(kPool + 20), k24 = LfsX(kPool + 24);
    u32 i = 0;
    for (u32 n = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, kTag})); n != 0;
         n = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {n, kTag})), ++i) {
        if (i >= 6) continue;
        const u32 s = o + 384 + 80 * i;
        call(ctx, XmlCursor_ReadHash, {n, 0x75849CFEu, s, load<u32>(s)});
        call(ctx, XmlCursor_ReadVec3, {n, 0x2B7B3EB4u, s + 16, s + 16});
        call(ctx, XmlCursor_ReadVec3, {n, 0x313AB0C1u, s + 32, s + 32});
        if (call(ctx, XmlCursor_ReadVec3, {n, 0x313AB844u, ev, kVecZero}) == 0) {
            const f64 ey = Ms(LfsX(ev + 4), k20), ex = Ms(LfsX(ev), k20), ez = Ms(LfsX(ev + 8), k20);
            Stfs(ev + 4, ey);
            Stfs(ev, ex);
            Stfs(ev + 8, ez);
            mid12::EulerQuatAt(ctx, s + 48, ex, ey, ez, k24);
        }
        call(ctx, XmlCursor_ReadHash, {n, 0x632DA119u, s + 64, load<u32>(s + 64)});
        call(ctx, XmlCursor_ReadBool, {n, 0x8141EC8Au, s + 68, load<u8>(s + 68)});
        call(ctx, XmlCursor_ReadBool, {n, 0xF8E455ACu, s + 69, load<u8>(s + 69)});
    }
    const f64 zero = LfsX(kPool + 4);
    if (LfsX(o + 336) < zero) store<u32>(o + 336, 0);
    const f64 f344 = LfsX(o + 344);
    if (f344 < zero) store<u32>(o + 344, 0);
    ctx.fpr[1] = f344;
    u32 r3 = load<u8>(o + 366);
    if (r3 != 0) {
        r3 = load<u8>(o + 367);
        if (r3 != 0) {
            store<u8>(o + 366, 0);
            store<u8>(o + 367, 0);
        }
    }
    ctx.gpr[3] = r3;
    DmgDestroy(ctx, rec);
}

namespace {
namespace hd62 {
constexpr u32 XmlCursor_NextSiblingAny = 0x004F8D74;
constexpr u32 XmlCursor_FirstChildAny = 0x004F9140;
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (c, const char* name) -> value string or 0
constexpr u32 String_CopyN = 0x00553638;              // (dst, src, n)
constexpr u32 Kind_FromHash = 0x001D6164;             // f_001D6164(u32 hash) -> kind (reads r3 only)
constexpr u32 sHudPromptMsg = 0x00AD1EACu;            // "hudPromptMsg"

// The bit (IBM order) of the u32 at ea as a bool flag (0 / 1).
inline u32 BitAt(u32 ea, u32 ibm) { return Bit(load<u32>(ea), ibm); }
// *ea's bit (IBM order) set to the low bit of v.
inline void PutBitAt(u32 ea, u32 ibm, u32 v) {
    const u32 m = 0x80000000u >> ibm;
    store<u32>(ea, (load<u32>(ea) & ~m) | ((v & 1u) ? m : 0u));
}
// The f32 at ea clamped below at the f32 at zero (fcmpu lt; NaN and -0
// kept), stored back either way as the stfs of the result; returns it.
inline f64 ClampLo(u32 ea, u32 zero) {
    const f64 v = LfsX(ea), z = LfsX(zero);
    const bool lt = v < z;
    store<u32>(ea, lt ? load<u32>(zero) : load<u32>(ea));
    return lt ? z : v;
}
}  // namespace hd62
}  // namespace

// void DamageColDef_ReadD62A8(a0, XmlCursor* c, x, const Def* tmpl, Rec* o)
// DamageColDef_Read_000C85A8 with the same arguments, then a defaults record
// on the stack (DmgCtor: x->12, the name 0x00AD1E98 twice, r7 / r8 as the
// call leaves them, pool 0x001D6290) with four zero vec3s +256..+304, f32s
// +320 50, +324 0, +328 / +332 0.5, +336 -1, +340..+348 0, s32 +352 1,
// +356..+408 0, flag bits 0x06000000 of +412 set (0xF8000000 clear), the
// string +222 empty; +8 |= 1. A tmpl with bit 1 in +28 supplies them
// instead (rec +k = tmpl +k + 80; the top 6 flag bits; the string when not
// empty); a tmpl without it notes c's document path. o is then read from
// c: vec3s 0x3665776A +256 / 0xA21F9D02 +288, f32s +320..+348 (+348 at
// least 0), hash 0xCC4EF294 +408, flag bools 0x80000000 / 0x40000000 /
// 0x20000000 of +412 (bits 0x18000000 from the defaults, or 1 / 2 / 0 for
// the hash 0xA50A23DF = 0x03FCEF54 / 0xFF4019A3 / other when present),
// 0x04000000, the "hudPromptMsg" attribute (when not empty) into the
// 32-byte string +222, vec3s 0xA5936640 +272 / 0x760CE7A8 +304 (degrees to
// radians when present), the bool 0x02000000 of +412 and +340 at least 0.
// Each child with bit 0x80 of +24 then sets, by its tag (+20):
// 0x3257217B: +356 kind (f_001D6164 of the hash 0x3257B96C when present),
// +352 = 1, f32s +360..+368, bit 0x80000000 of +376, +364 at least 0, f32
// +372; 0x89F23F47: +352 = 3, +356 = the square of the f32 0x82E789DB
// (default 0), f32 +360, bits 0x80000000 / 0x40000000 of +364 from the
// defaults and the hashes 0x04B8E870 / 0x906DDB56; 0x89F1FF19: +356 kind
// (0..3 from the hash 0xB456B7AA), +352 = 2, f32s +360..+400, bits
// 0xE0000000 of +404, +360 / +364 at least 0. The record's vector is
// destroyed; r3 as the last call leaves it, f1 as the original's last
// float.
DECOMP(001D62A8, DamageColDef_ReadD62A8) {
    using namespace b15fn;
    using namespace hd62;
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 1008);
    const u32 sp = frame.addr(), rec = sp + 368, fl = rec + 412, b8 = sp + 804;
    constexpr u32 kPool = 0x001D6290;  // f32 {100, 0, 0.3, 1, -1, pi/180}
    const u32 kZero = kPool + 4;
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    DmgCtor(ctx, rec, load<u32>(x + 12), mid11::kStrAD1E98, mid11::kStrAD1E98, ctx.gpr[7], ctx.gpr[8], kPool);
    for (u32 v = 256; v <= 304; v += 16)
        for (u32 k = 0; k < 3; ++k) CopyW(rec + v + 4 * k, kVecZero + 4 * k);
    store<u32>(rec + 320, 0x42480000u);
    store<u32>(rec + 324, 0);
    store<u32>(rec + 328, 0x3F000000u);
    store<u32>(rec + 332, 0x3F000000u);
    store<u32>(rec + 336, 0xBF800000u);
    for (u32 k = 340; k <= 348; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 352, 1);
    for (u32 k = 356; k <= 408; k += 4) store<u32>(rec + k, 0);
    store<u32>(fl, (load<u32>(fl) & 0x01FFFFFFu) | 0x06000000u);  // the frame's stale low bits kept
    store<u32>(rec + 8, load<u32>(rec + 8) | 1u);
    store<u8>(rec + 222, 0);
    if (tmpl != 0) {
        if (load<u32>(tmpl + 28) & 1u) {
            for (u32 v = 256; v <= 304; v += 16)
                for (u32 k = 0; k < 3; ++k) CopyW(rec + v + 4 * k, tmpl + v + 80 + 4 * k);
            for (u32 k = 320; k <= 408; k += 4) CopyW(rec + k, tmpl + k + 80);
            store<u32>(fl, (load<u32>(tmpl + 492) & 0xFC000000u) | (load<u32>(fl) & 0x03FFFFFFu));
            if (load<u8>(tmpl + 302) != 0) call(ctx, String_CopyN, {rec + 222, tmpl + 302, 32});
        } else {
            call(ctx, XmlCursor_DocPath, {a0});
        }
    }
    call(ctx, XmlCursor_ReadVec3, {c, 0x3665776Au, o + 256, rec + 256});
    call(ctx, XmlCursor_ReadVec3, {c, 0xA21F9D02u, o + 288, rec + 288});
    constexpr u32 kF320[8] = {0x760CBA9Eu, 0x176EDFCFu, 0x440242A6u, 0x7D51873Bu, 0x36EF44EBu, 0x11AA6429u, 0x26A56A39u, 0xCB13F34Du};
    for (u32 k = 0; k < 8; ++k) RdF32(ctx, c, kF320[k], o + 320 + 4 * k, rec + 320 + 4 * k);
    ctx.fpr[1] = ClampLo(o + 348, kZero);
    call(ctx, XmlCursor_ReadHash, {c, 0xCC4EF294u, o + 408, load<u32>(rec + 408)});
    store<u8>(b8, 0);
    call(ctx, XmlCursor_ReadBool, {c, 0x65AFACDFu, b8, BitAt(fl, 0)});
    PutBitAt(o + 412, 0, load<u8>(b8));
    call(ctx, XmlCursor_ReadBool, {c, 0x59C5087Eu, b8, BitAt(fl, 1)});
    PutBitAt(o + 412, 1, load<u8>(b8));
    call(ctx, XmlCursor_ReadBool, {c, 0xB9AB0DC3u, b8, BitAt(fl, 2)});
    {
        const u32 w = (load<u32>(o + 412) & ~0x20000000u) | ((load<u8>(b8) & 1u) ? 0x20000000u : 0u);
        store<u32>(o + 412, (w & ~0x18000000u) | (load<u32>(fl) & 0x18000000u));
    }
    if (call(ctx, XmlCursor_ReadHash, {c, 0xA50A23DFu, sp + 784, 0}) == 0) {
        const u32 h = load<u32>(sp + 784);
        const u32 k = h == 0x03FCEF54u ? 1u : h == 0xFF4019A3u ? 2u : 0u;
        store<u32>(o + 412, (load<u32>(o + 412) & ~0x18000000u) | (k << 27));
    }
    call(ctx, XmlCursor_ReadBool, {c, 0x73D6C6F2u, b8, BitAt(fl, 5)});
    PutBitAt(o + 412, 5, load<u8>(b8));
    const u32 msg = static_cast<u32>(call(ctx, XmlCursor_FindAttrByName, {c, sHudPromptMsg}));
    if (msg != 0 && load<u8>(msg) != 0) call(ctx, String_CopyN, {o + 222, msg, 32});
    for (u32 v : {272u, 304u}) {
        if (call(ctx, XmlCursor_ReadVec3, {c, v == 272 ? 0xA5936640u : 0x760CE7A8u, o + v, rec + v}) == 0) {
            const f64 k = LfsX(kPool + 20);
            const f64 vx = Ms(LfsX(o + v), k), vy = Ms(LfsX(o + v + 4), k), vz = Ms(LfsX(o + v + 8), k);
            Stfs(o + v, vx);
            Stfs(o + v + 4, vy);
            Stfs(o + v + 8, vz);
            ctx.fpr[1] = vx;
        }
    }
    call(ctx, XmlCursor_ReadBool, {c, 0x57DA11C3u, b8, BitAt(fl, 6)});
    PutBitAt(o + 412, 6, load<u8>(b8));
    ctx.fpr[1] = ClampLo(o + 340, kZero);
    for (u32 n = static_cast<u32>(call(ctx, XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, XmlCursor_NextSiblingAny, {n}))) {
        if ((load<u8>(n + 24) & 0x80u) == 0) continue;
        const u32 tag = load<u32>(n + 20);
        if (tag == 0x3257217Bu) {
            CopyW(o + 356, rec + 356);
            if (call(ctx, XmlCursor_ReadHash, {n, 0x3257B96Cu, sp + 788, 0}) == 0)
                store<u32>(o + 356, static_cast<u32>(call(ctx, Kind_FromHash, {load<u32>(sp + 788)})));
            store<u32>(o + 352, 1);
            // These two hashes go sign-extended (the original keeps them in the frame as doublewords).
            callf(ctx, XmlCursor_ReadF32, {n, Sx64(0xE1555935u), o + 360}, {LfsX(rec + 360)});
            RdF32(ctx, n, 0x636A522Bu, o + 364, rec + 364);
            callf(ctx, XmlCursor_ReadF32, {n, Sx64(0xE18D39E5u), o + 368}, {LfsX(rec + 368)});
            store<u8>(sp + 805, 0);
            call(ctx, XmlCursor_ReadBool, {n, 0x0E0CA78Cu, sp + 805, BitAt(rec + 376, 0)});
            PutBitAt(o + 376, 0, load<u8>(sp + 805));
            ClampLo(o + 364, kZero);
            RdF32(ctx, n, 0x0C28FD40u, o + 372, rec + 372);
        } else if (tag == 0x89F23F47u) {
            store<u32>(o + 352, 3);
            store<u32>(sp + 796, 0);
            callf(ctx, XmlCursor_ReadF32, {n, 0x82E789DBu, sp + 796}, {LfsX(kZero)});
            Stfs(o + 356, Ms(LfsX(sp + 796), LfsX(sp + 796)));
            RdF32(ctx, n, 0x02D738DFu, o + 360, rec + 360);
            store<u32>(o + 364, (load<u32>(o + 364) & 0x7FFFFFFFu) | (load<u32>(rec + 364) & 0x80000000u));
            store<u32>(sp + 800, 0);
            if (call(ctx, XmlCursor_ReadHash, {n, 0x04B8E870u, sp + 800, 0}) == 0) {
                const s32 h = static_cast<s32>(load<u32>(sp + 800));
                if (h == 0x04A5C9FC) store<u32>(o + 364, load<u32>(o + 364) | 0x80000000u);
                else if (h == 0x00239807) store<u32>(o + 364, load<u32>(o + 364) & 0x7FFFFFFFu);
            }
            PutBitAt(o + 364, 1, BitAt(rec + 364, 1));
            if (call(ctx, XmlCursor_ReadHash, {n, 0x906DDB56u, sp + 800, 0}) == 0) {
                const s32 h = static_cast<s32>(load<u32>(sp + 800));
                if (h == 0x000103F7) PutBitAt(o + 364, 1, 0);
                else if (h == 2164) store<u32>(o + 364, load<u32>(o + 364) | 0x40000000u);
            }
        } else if (tag == 0x89F1FF19u) {
            CopyW(o + 356, rec + 356);
            if (call(ctx, XmlCursor_ReadHash, {n, Sx64(0xB456B7AAu), sp + 792, 0}) == 0) {
                const u32 h = load<u32>(sp + 792);
                store<u32>(o + 356, h == 0x46BDBD35u ? 1u : h == 0x842320BBu ? 3u : h == 0x842320BAu ? 2u : 0u);
            }
            store<u32>(o + 352, 2);
            constexpr u32 kF360[11] = {0x82E789DBu, 0x0BF5ADB7u, 0x96CB0082u, 0x3CE11DA0u, 0x46FFF512u, 0x31D7B193u,
                                       0x036A8EAFu, 0x4C005DE1u, 0xA4D3EB03u, 0x57557B45u, 0x8C06F037u};
            for (u32 k = 0; k < 11; ++k) RdF32(ctx, n, kF360[k], o + 360 + 4 * k, rec + 360 + 4 * k);
            constexpr u32 kB404[3] = {0xF6866423u, 0x2443C7A5u, 0x153912A7u};
            store<u8>(sp + 806, 0);
            for (u32 k = 0; k < 3; ++k) {
                call(ctx, XmlCursor_ReadBool, {n, kB404[k], sp + 806, BitAt(rec + 404, k)});
                PutBitAt(o + 404, k, load<u8>(sp + 806));
            }
            ClampLo(o + 360, kZero);
            ctx.fpr[1] = ClampLo(o + 364, kZero);
        }
    }
    DmgDestroy(ctx, rec);
}
