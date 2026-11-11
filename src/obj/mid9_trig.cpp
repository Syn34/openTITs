// Game code, medium sweep part 9: trigger def readers on
// Def_ReadBase20BFE8 (the record / object of vtable 0x00B19510 and the
// inlined default constructors that give their defaults).
#include "mid9.h"

using namespace sly4;
using namespace sly4::mid9;

// u32 TriggerAlertDef_Read2124D0(a0, XmlCursor* c, x, const Def* tmpl, Def*
// o): the base part (Def_ReadBase20BFE8(a0, c, x, tmpl, o)), then the bool
// 0x4E6A50D1 (+84) whose default is tmpl->176 for a template with the flag
// 8 in +24 (another template gets XmlCursor_DocPath(a0)), else that of a
// default TriggerAlert def (objPrefix "TriggerAlert_", vtable 0x00B19AE8)
// built on the stack and destroyed again (Obj_BaseDtor_000C2DA0). Returns
// the destructor's result.
DECOMP(002124D0, TriggerAlertDef_Read2124D0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m9fn::Def_ReadBase20BFE8, {a0, c, x, tmpl, o});
    Stack frame(ctx, 288);
    const u32 obj = frame.addr(), rec = obj + 192;  // the original's r1 + 112 / r1 + 304
    TrigRecInit(rec, x, 0x00400000, 8, 0, 0x00AD36D0);
    store<u8>(rec + 84, 0);
    TrigFromRec(ctx, obj, rec, 1);
    store<u8>(obj + 176, load<u8>(rec + 84));
    store<u32>(obj, 0x00B19AE8);
    u32 def = load<u8>(obj + 176);
    if (tmpl != 0) {
        if ((load<u32>(tmpl + 24) & 8u) != 0) {
            def = load<u8>(tmpl + 176);
        } else {
            call(ctx, m8fn::XmlCursor_DocPath, {a0});
            def = load<u8>(obj + 176);
        }
    }
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x4E6A50D1, o + 84, def);
    store<u32>(obj, kTrigVt);
    ret(ctx, static_cast<u32>(call(ctx, m9fn::Obj_BaseDtor_000C2DA0, {obj})));
}

// Def* TriggerSoundZoneDef_New212DC8(a0, XmlCursor* c, x, const Def* tmpl):
// a default TriggerSoundZone record (flags 0x04400000, +32 1, objPrefix
// "TriggerSoundZone_") is read on the stack (Def_ReadBase20BFE8(a0, c, x,
// tmpl, rec), +32 = 1 again, hashes 0x3A20DD1B (+84) / 0xEF44A7B5 (+88)
// default 0, f32 0x48AD591B (+92, default the f32 at 0x00212DC0)); a
// 208-byte def (SmallAlloc; 0 when that fails) is built from it: the
// trigger part, vtable 0x00B19C98, +176 / +184 the hashes, +180 / +188 0,
// f32 +192. Returns the def; f1 = the f32 when one was made.
DECOMP(00212DC8, TriggerSoundZoneDef_New212DC8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 96);
    const u32 rec = frame.addr();  // the original's r1 + 112
    const u32 k = load<u32>(0x00212DC0);
    TrigRecInit(rec, x, 0x04400000, 0, 1, 0x00AD36EC);
    call(ctx, m9fn::Def_ReadBase20BFE8, {a0, c, x, tmpl, rec});
    store<u32>(rec + 32, 1);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x3A20DD1B, rec + 84, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xEF44A7B5, rec + 88, 0);
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x48AD591B, rec + 92}, {std::bit_cast<f64>(LfsBits(k))});
    const u32 p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {208}));
    if (p == 0) return ret(ctx, 0);
    TrigFromRec(ctx, p, rec, 2);
    store<u32>(p, 0x00B19C98);
    store<u32>(p + 180, 0);
    store<u32>(p + 176, load<u32>(rec + 84));
    store<u32>(p + 188, 0);
    store<u32>(p + 184, load<u32>(rec + 88));
    CopyW(p + 192, rec + 92);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(rec + 92)));
    ret(ctx, p);
}

namespace {
// The kind hashes of the 0x51609AAA children (bits 1 / 8 / 4 / 2).
u32 KindBit(u32 h) {
    switch (h) {
    case 0x7DD0CD87: return 1;
    case 0x03D66C07: return 8;
    case 0x03D3CE95: return 4;
    case 0x0000FD2A: return 2;
    default: return 0;
    }
}
}  // namespace

// u32 TriggerDef_ReadKinds214C28(a0, XmlCursor* c, x, const Def* tmpl, Def*
// o): the base part (Def_ReadBase20BFE8(a0, c, x, tmpl, o)), +32 = 1; the
// kind hash 0x0024728B of each 0x51609AAA child ORs its bit (0x7DD0CD87 1,
// 0x0000FD2A 2, 0x03D3CE95 4, 0x03D66C07 8) into a mask, and so does the
// hash attribute 0x51609AAA of c itself (neither: all bits, 0xFFFF). The
// u16 +84 = the mask, +86 = its complement (without the bit 2 when the
// mask has 4). Returns the mask.
DECOMP(00214C28, TriggerDef_ReadKinds214C28) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    constexpr u32 kChild = 0x51609AAA;
    call(ctx, m9fn::Def_ReadBase20BFE8, {a0, c, x, tmpl, o});
    store<u32>(o + 32, 1);
    Stack frame(ctx, 16);
    const u32 t = frame.addr();  // the original's r1 + 112 / + 116
    u32 mask = 0;
    for (u32 n = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, kChild})); n != 0;
         n = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {n, kChild}))) {
        store<u32>(t, 0);
        XRead(ctx, xfn::XmlCursor_ReadHash, n, 0x0024728B, t, 0);
        mask |= KindBit(load<u32>(t));
    }
    store<u32>(t + 4, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, kChild, t + 4, 0);
    const u32 v = load<u32>(t + 4);
    const u32 own = (v == 0 && mask == 0) ? 0xFFFFu : KindBit(v);
    const u32 m = own | mask;
    u32 inv = ~m;
    store<u16>(o + 84, static_cast<u16>(m));
    store<u16>(o + 86, static_cast<u16>(inv));
    if ((m & 4u) != 0) {
        inv &= ~2u;
        store<u16>(o + 86, static_cast<u16>(inv));
    }
    ret(ctx, m);
}

// Def* TriggerDeathDef_New21B898(a0, XmlCursor* c, x, const Def* tmpl): a
// default TriggerDeath record (flags 0x10400000, objPrefix "TriggerDeath_",
// +84 0x03D3CDC0, +88 0, +92 -1, f32 +96 0, flags word +100 with bits 31 /
// 30 clear) is read on the stack: Def_ReadBase20BFE8(a0, c, x, tmpl, rec),
// +92 the index of the hash 0x51609AAA in the table at 0x00AD3FE4 (7
// entries; else left), f32 0x7D873487 (+96, default the f32 at
// 0x0021B890), hashes 0x63C8D78E (+84) / 0xF89FDD94 (+88) default 0, bools
// 0x98FD6F46 / 0xF403C89E into the bits 31 / 30. A 208-byte def (SmallAlloc;
// 0 when that fails): the trigger part, vtable 0x00B1A418, +176 / +180 the
// hashes, +188 the index, f32 +192, the bits 31 / 30 of +196, +184 the
// EffectDef_Find(+88 hash, 0, 1) result. Returns the def.
// The record's flags word is never initialized (stack garbage) beyond its
// two bits; only those two are used.
DECOMP(0021B898, TriggerDeathDef_New21B898) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kTab = 0x00AD3FE4, kDef = 0x0021B890, EffectDef_Find = 0x004BDEDC;
    Stack frame(ctx, 112);
    const u32 tmp = frame.addr(), rec = tmp + 4, b = tmp + 108;  // the original's r1 + 112 / + 116 / + 220
    TrigRecInit(rec, x, 0x10400000, 0, 0, 0x00AD3794);
    store<u32>(rec + 84, 0x03D3CDC0);
    store<u32>(rec + 88, 0);
    store<u32>(rec + 92, 0xFFFFFFFFu);
    store<u32>(rec + 96, 0);
    store<u32>(rec + 100, load<u32>(rec + 100) & 0x3FFFFFFFu);
    const u32 k = load<u32>(kDef);
    call(ctx, m9fn::Def_ReadBase20BFE8, {a0, c, x, tmpl, rec});
    store<u32>(tmp, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x51609AAA, tmp, 0);
    for (u32 i = 0; i < 7; ++i)
        if (load<u32>(tmp) == load<u32>(kTab + 4 * i)) {
            store<u32>(rec + 92, i);
            break;
        }
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x7D873487, rec + 96}, {std::bit_cast<f64>(LfsBits(k))});
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x63C8D78E, rec + 84, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xF89FDD94, rec + 88, 0);
    store<u8>(b, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x98FD6F46, b, load<u32>(rec + 100) >> 31);
    u32 fl = (load<u32>(rec + 100) & 0x7FFFFFFFu) | ((load<u8>(b) & 1u) << 31);
    store<u32>(rec + 100, fl);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xF403C89E, b, (fl >> 30) & 1u);
    store<u32>(rec + 100, (load<u32>(rec + 100) & ~0x40000000u) | ((load<u8>(b) & 1u) << 30));
    const u32 p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {208}));
    if (p == 0) return ret(ctx, 0);
    TrigFromRec(ctx, p, rec, 2);
    store<u32>(p, 0x00B1A418);
    store<u32>(p + 176, load<u32>(rec + 84));
    const u32 h88 = load<u32>(rec + 88);
    store<u32>(p + 184, 0);
    store<u32>(p + 180, h88);
    store<u32>(p + 188, load<u32>(rec + 92));
    CopyW(p + 192, rec + 96);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(rec + 96)));
    u32 w = (load<u32>(p + 196) & 0x7FFFFFFFu) | (load<u32>(rec + 100) & 0x80000000u);
    store<u32>(p + 196, w);
    w = (w & ~0x40000000u) | (load<u32>(rec + 100) & 0x40000000u);
    store<u32>(p + 196, w);
    store<u32>(p + 184, static_cast<u32>(call(ctx, EffectDef_Find, {h88, 0, 1})));
    ret(ctx, p);
}

// u32 TriggerFxDef_Read21DEE0(a0, XmlCursor* c, x, const Def* tmpl, Def*
// o): the base part (Def_ReadBase20BFE8(a0, c, x, tmpl, o)), +32 = 1. The
// defaults come from a template with bit 31 of +20 (bits +176, hash +180,
// f32 +184..+196, hash +200; another template gets XmlCursor_DocPath(a0)),
// else 0 and the f32 at 0x0021DEDC (k). Hash 0xF89FDD94 (+88); the bools of
// the table at 0x00AD37C0 ORed into +84 as bits 0..2; f32 0x11CB4BE1, when
// present, gives both +92 and +96, else 0xB8D446B1 / 0xB8D445C3; likewise
// 0x49786225 for +100 / +104, else 0xD02659ED / 0xD02658FF; hash 0x82BF2A4A
// (+108). The four f32 are then raised to at least k, and +96 / +104 to at
// least +92 / +100. Returns the last ReadHash result; f1 = +104 before that
// last raise.
DECOMP(0021DEE0, TriggerFxDef_Read21DEE0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    constexpr u32 kK = 0x0021DEDC, kBools = 0x00AD37C0;
    auto fl = [](u32 w) { return std::bit_cast<f64>(LfsBits(w)); };
    call(ctx, m9fn::Def_ReadBase20BFE8, {a0, c, x, tmpl, o});
    store<u32>(o + 32, 1);
    const u32 k = load<u32>(kK);
    u32 bits = 0, h88 = 0, h108 = 0, d92 = k, d96 = k, d100 = k, d104 = k;
    if (tmpl != 0) {
        if ((load<u32>(tmpl + 20) & 0x80000000u) != 0) {
            bits = load<u32>(tmpl + 176);
            h88 = load<u32>(tmpl + 180);
            d92 = load<u32>(tmpl + 184);
            d96 = load<u32>(tmpl + 188);
            d100 = load<u32>(tmpl + 192);
            d104 = load<u32>(tmpl + 196);
            h108 = load<u32>(tmpl + 200);
        } else {
            call(ctx, m8fn::XmlCursor_DocPath, {a0});
        }
    }
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xF89FDD94, o + 88, h88);
    Stack frame(ctx, 16);
    const u32 t = frame.addr(), b = t + 4;  // the original's r1 + 112 / + 116
    for (u32 i = 0; i < 3; ++i) {
        const u32 def = (bits & (1u << i)) != 0 ? 1u : 0u;
        store<u8>(b, static_cast<u8>(def));
        XRead(ctx, xfn::XmlCursor_ReadBool, c, load<u32>(kBools + 4 * i), b, def);
        store<u32>(o + 84, load<u32>(o + 84) | (load<u8>(b) << i));
    }
    store<u32>(t, 0);
    store<u32>(o + 92, d92);
    store<u32>(o + 96, d96);
    if (callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x11CB4BE1, t}, {fl(k)}), ctx.gpr[3] == 0) {
        store<u32>(o + 92, load<u32>(t));
        store<u32>(o + 96, load<u32>(t));
    } else {
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xB8D446B1, o + 92}, {fl(d92)});
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xB8D445C3, o + 96}, {fl(d96)});
    }
    store<u32>(o + 100, d100);
    store<u32>(o + 104, d104);
    if (callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x49786225, t}, {fl(k)}), ctx.gpr[3] == 0) {
        store<u32>(o + 100, load<u32>(t));
        store<u32>(o + 104, load<u32>(t));
    } else {
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xD02659ED, o + 100}, {fl(d100)});
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xD02658FF, o + 104}, {fl(d104)});
    }
    const u32 r = XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x82BF2A4A, o + 108, h108);
    auto lt = [](u32 a, u32 bb) { return F32(a) < F32(bb); };  // fcmpu: false for NaNs
    u32 v92 = load<u32>(o + 92), v96 = load<u32>(o + 96), v100 = load<u32>(o + 100), v104 = load<u32>(o + 104);
    if (lt(v92, k)) v92 = k;
    store<u32>(o + 92, v92);
    if (lt(v96, k)) v96 = k;
    store<u32>(o + 96, v96);
    if (lt(v100, k)) v100 = k;
    store<u32>(o + 100, v100);
    if (lt(v104, k)) v104 = k;
    store<u32>(o + 104, v104);
    if (lt(v96, v92)) store<u32>(o + 96, v92);
    if (lt(v104, v100)) store<u32>(o + 104, v100);
    ctx.fpr[1] = fl(v104);
    ret(ctx, r);
}
