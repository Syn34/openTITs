// Game code, medium sweep part 12: XML spec readers around 0x004B1708 /
// 0x004B3BC8.
#include "mid12.h"

using namespace sly4;
using namespace sly4::mid12;

namespace {
constexpr u32 kString_Append = 0x005566A8;          // (dst, src)
constexpr u32 kStrBuf_Append = 0x00566F1C;          // (StrBuf*, s) -> char*
constexpr u32 kXmlCursor_ReadF32ByName = 0x004FA5C4;  // (c, name, out), default in f1
constexpr u32 kXml_Where = 0x004F61C4;              // (c) -> const char*
constexpr u32 kGlobals = 0x00F65F84;                // the game globals pointer
}  // namespace

// bool Clip_ReadSpec_004B14F8(Clip* o, XmlCursor* c, u32* nameHash)
// Needs the attributes 0xE971C795 (a directory: copied to a 512-byte stack
// buffer unless it starts with '*', in which case the buffer is left as
// it is) and 0xE971C796 (appended); *nameHash = String_HashNoCase(the
// path). Then the f32 0x95FB67C0 into +0 (default the pool word
// 0x004B14F0; must be present), byte +4 = 1 / 2 / 0 for the hash
// 0xDB8BAEEB (default 0) 0x85D98325 / 0x0026E201 / other; the attribute
// 0x91EEB2F8 appended to the global string buffer (*0x00F65F84) into +12
// (0 when absent) and +8 its String_HashNoCase (0 when none). Returns 1, 0
// when a required attribute is missing.
DECOMP(004B14F8, Clip_ReadSpec_004B14F8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    Stack frame(ctx, 528);
    const u32 buf = frame.addr(), h = buf + 512;  // the original's sp + 112 / + 624
    const u32 s1 = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0xE971C795u}));
    if (s1 == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 s2 = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0xE971C796u}));
    if (s2 == 0) {
        ret(ctx, 0);
        return;
    }
    store<u8>(buf + 511, 0);
    if (load<u8>(s1) != '*') call(ctx, m1fn::String_CopyN, {buf, s1, 512});  // '*': the buffer is not set (as the original)
    call(ctx, kString_Append, {buf, s2});
    store<u32>(out, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {buf})));
    if (callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x95FB67C0u, o}, {std::bit_cast<f64>(mid1::LfsBits(load<u32>(0x004B14F0)))}), ctx.gpr[3] != 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xDB8BAEEBu, h, 0});
    const u32 k = load<u32>(h);
    store<u8>(o + 4, k == 0x85D98325u ? 1 : k == 0x0026E201u ? 2 : 0);
    const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0x91EEB2F8u}));
    u32 t = 0;
    if (s != 0) {
        store<u32>(o + 12, s);
        t = static_cast<u32>(call(ctx, kStrBuf_Append, {load<u32>(kGlobals), s}));
    }
    store<u32>(o + 12, t);
    store<u32>(o + 8, t != 0 ? static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {t})) : 0u);
    ret(ctx, 1);
}

// void Obj_ReadSpec_004B3940(Spec* o, XmlCursor* c, a, b)
// Reads o's fields, the "kind" +28 set by the last group present: f32
// "0x00AEE320" +4 (by name; kind 0), f32 0xE5F7DE9A +8 (kind 1; outside
// [pool, pool + 4] it is reported (String_Format(0x00AEE6DC, value as a
// double in r4, Xml_Where(c))) and set to 100), s32 0x1EAA8FE1 +12 (kind
// 2), f32 0x27819325 +20 (kind 3), hash 0xF7B7EBB0 +16 (kind 4); bool
// 0x0D17F9B7 +24, hash 0xFF45BC1C +32, bool 0x8B3AB37D +36 (kind 5 when
// set); f32 defaults the pool word 0x004B3934, the others 0. Then +40 =
// the globals' +280 callback (c, a, b) when set. r3 = that result (or 0).
DECOMP(004B3940, Obj_ReadSpec_004B3940) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    constexpr u32 kPool = 0x004B3934;
    const f64 k0 = std::bit_cast<f64>(mid1::LfsBits(load<u32>(kPool)));
    if (callf(ctx, kXmlCursor_ReadF32ByName, {c, 0x00AEE320u, o + 4}, {k0}), ctx.gpr[3] == 0) store<u32>(o + 28, 0);
    if (callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xE5F7DE9Au, o + 8}, {k0}), ctx.gpr[3] == 0) {
        const f64 v = std::bit_cast<f64>(mid1::LfsBits(load<u32>(o + 8)));
        ctx.fpr[1] = v;
        if (v < k0 || v > LoadF(kPool + 4)) {
            const u32 w = static_cast<u32>(call(ctx, kXml_Where, {c}));
            call(ctx, m1fn::String_Format, {0x00AEE6DCu, std::bit_cast<u64>(v), w});  // the value also in f1 (as left)
            store<u32>(o + 8, 0x42C80000u);
        }
        store<u32>(o + 28, 1);
    }
    if (call(ctx, xfn::XmlCursor_ReadS32, {c, 0x1EAA8FE1u, o + 12, 0}) == 0) store<u32>(o + 28, 2);
    if (callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x27819325u, o + 20}, {k0}), ctx.gpr[3] == 0) store<u32>(o + 28, 3);
    if (call(ctx, xfn::XmlCursor_ReadHash, {c, 0xF7B7EBB0u, o + 16, 0}) == 0) store<u32>(o + 28, 4);
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x0D17F9B7u, o + 24, 0});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xFF45BC1Cu, o + 32, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x8B3AB37Du, o + 36, 0});
    if (load<u8>(o + 36) != 0) store<u32>(o + 28, 5);
    const u32 f = load<u32>(load<u32>(kGlobals) + 280);
    ctx.gpr[3] = f;
    if (f != 0) store<u32>(o + 40, static_cast<u32>(call_ptr(ctx, f, {c, a, b, f, load<u32>(f)})));
}
