// Game code, medium sweep part 8: def readers allocated with Mem_Alloc
// (192-byte and similar records whose names are copied into the record
// and registered in their own name maps).
#include "mid8.h"

using namespace sly4;
using namespace sly4::mid8;

namespace {

// The "name" attribute's string (no check) copied (31 chars) to dst.
void CopyAttr(PPUContext& ctx, u32 c, u32 attr, u32 dst) {
    const u32 s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, attr}));
    call(ctx, m8fn::String_CopyN, {dst, s, 31});
}

}  // namespace

// LegDef* LegDef_Read292210(?, XmlCursor* c) (reads r4 only): a 192-byte
// def (Mem_Alloc(192, 0, 16); when that succeeded: vec3 +128 the vector at
// 0x01164B70, +144 the zero vector, f32 +160..+188 {0.12, 0.4, -0.3, 0.15,
// -1, 0, 0.6, 0.2}, empty strings): the attributes "name", "hipJoint",
// "kneeJoint", "ankleJoint" copied to +0 / +32 / +64 / +96 (not checked),
// vec3 +128 (normalized: the f32 at 0x0029220C / length) / +144, eight
// f32; {String_HashNoCase(+0), def} appended to the map 0x0105788C.
// Quirk: when Mem_Alloc fails, everything is read and written on p = 0.
DECOMP(00292210, LegDef_Read292210) {
    const u32 c = arg<u32>(ctx, 1);
    constexpr u32 kMap = 0x0105788C, kOne = 0x0029220C;
    u32 p = static_cast<u32>(call(ctx, m7fn::Mem_Alloc, {192, 0, 16}));
    if (p != 0) {
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 128 + 4 * k, load<u32>(0x01164B70 + 4 * k));  // lfs / stfs
        store<u32>(p + 144, load<u32>(0x01164B40));
        store<u32>(p + 148, load<u32>(0x01164B44));
        store<u32>(p + 160, 0x3DF5C28F);
        store<u32>(p + 152, load<u32>(0x01164B48));
        store<u32>(p + 164, 0x3ECCCCCD);
        store<u32>(p + 180, 0);
        store<u32>(p + 168, 0xBE99999A);
        store<u32>(p + 172, 0x3E19999A);
        store<u32>(p + 176, 0xBF800000);
        store<u8>(p, 0);
        store<u8>(p + 64, 0);
        store<u32>(p + 184, 0x3F19999A);
        store<u32>(p + 188, 0x3E4CCCCD);
        store<u8>(p + 32, 0);
        store<u8>(p + 96, 0);
    }
    CopyAttr(ctx, c, kStrName, p);
    CopyAttr(ctx, c, 0x00AD4FA4, p + 32);  // "hipJoint"
    CopyAttr(ctx, c, 0x00AD4FB4, p + 64);  // "kneeJoint"
    CopyAttr(ctx, c, 0x00AD4FC4, p + 96);  // "ankleJoint"
    XVec3(ctx, c, 0x06C7CB64, p + 128);
    XVec3(ctx, c, 0x81C9963D, p + 144);
    constexpr u32 kF[8] = {0x055F7F61, 0xF2ADE153, 0xDD33F4C5, 0x471290F3, 0xDD33E5C1, 0x471281EF, 0x83BD0148, 0x95934AC1};
    for (u32 k = 0; k < 8; ++k) XF32(ctx, c, kF[k], p + 160 + 4 * k);
    const f32 x = F32(load<u32>(p + 128)), y = F32(load<u32>(p + 132)), z = F32(load<u32>(p + 136));
    f32 l = static_cast<f32>(static_cast<f64>(y) * y);
    l = static_cast<f32>(std::fma(static_cast<f64>(x), x, l));
    l = static_cast<f32>(std::fma(static_cast<f64>(z), z, l));
    l = static_cast<f32>(std::sqrt(static_cast<f64>(l)));
    const f32 s = static_cast<f32>(static_cast<f64>(F32(load<u32>(kOne))) / l);
    store<u32>(p + 128, W32(x * s));
    store<u32>(p + 132, W32(y * s));
    store<u32>(p + 136, W32(z * s));
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 128 + 4 * k, load<u32>(p + 128 + 4 * k));  // copied onto itself (lfs / stfs)
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(p + 136)));
    const u32 h = static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {p}));
    RegPushKV(ctx, kMap, h, p);
    ret(ctx, p);
}

// Def* IkChainDef_Read2929EC(a0, XmlCursor* c): the attributes are gathered on
// the stack first: "name" (+0), the index (0 / 1) of the hash 0x0027873A
// in the table at 0x00AD5020 (+32; another value gets
// XmlCursor_DocPath(a0)), +36 0, four more name attributes (0x00AD4FE0,
// 0x00AD4FF0, 0x00AD5028, 0x00AD5000; default the empty string at
// 0x00AD4B7C) for +40 / +72 / +104 / +136, vec3 0xEDAE5B2C (default the
// vector at 0x01164B70) for +176. Then a 192-byte def (Mem_Alloc; 0 when
// that fails) gets them (the strings copied, 31 chars) and {String_HashNoCase
// (+0), def} is appended to the map 0x010578BC.
DECOMP(002929EC, IkChainDef_Read2929EC) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kMap = 0x010578BC, kEmpty = 0x00AD4B7C, kVec = 0x01164B70, kKinds = 0x00AD5020;
    Stack frame(ctx, 64);
    const u32 L = frame.addr();  // the original's r1 + 112
    store<u32>(L, kEmpty);
    store<u32>(L + 32, load<u32>(kVec));  // lfs / stfs
    store<u32>(L + 4, 0);
    store<u32>(L + 8, 0);
    for (u32 off : {12u, 16u, 20u}) store<u32>(L + off, kEmpty);
    store<u32>(L + 36, load<u32>(kVec + 4));
    store<u32>(L + 24, kEmpty);
    store<u32>(L + 40, load<u32>(kVec + 8));
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kVec + 8)));
    store<u32>(L, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrName})));
    if (call(ctx, xfn::XmlCursor_ReadHash, {c, 0x0027873A, L + 48, 0}) == 0) {
        u32 i = 0;
        while (i < 2 && load<u32>(L + 48) != load<u32>(kKinds + 4 * i)) ++i;
        if (i < 2)
            store<u32>(L + 4, i);
        else
            call(ctx, m8fn::XmlCursor_DocPath, {a0});
    }
    store<u32>(L + 12, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD4FE0})));
    store<u32>(L + 16, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD4FF0})));
    store<u32>(L + 20, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD5028})));
    store<u32>(L + 24, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD5000})));
    XVec3(ctx, c, 0xEDAE5B2C, L + 32);
    const u32 p = static_cast<u32>(call(ctx, m7fn::Mem_Alloc, {192, 0, 16}));
    if (p == 0) return ret(ctx, 0);
    store<u32>(p + 32, load<u32>(L + 4));
    store<u32>(p + 36, load<u32>(L + 8));
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 176 + 4 * k, load<u32>(L + 32 + 4 * k));  // lfs / stfs
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(L + 40)));
    call(ctx, m8fn::String_CopyN, {p, load<u32>(L), 31});
    call(ctx, m8fn::String_CopyN, {p + 40, load<u32>(L + 12), 31});
    call(ctx, m8fn::String_CopyN, {p + 72, load<u32>(L + 16), 31});
    call(ctx, m8fn::String_CopyN, {p + 104, load<u32>(L + 20), 31});
    call(ctx, m8fn::String_CopyN, {p + 136, load<u32>(L + 24), 31});
    const u32 h = static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {p}));
    RegPushKV(ctx, kMap, h, p, true);
    ret(ctx, p);
}

// Def* IkChainDef_Read292618(a0, XmlCursor* c): as IkChainDef_Read2929EC,
// 208 bytes, map 0x010578A4: "name" (+0), the index (0 / 1) of the hash
// 0x0027873A in the table at 0x00AD4FD4 (+32; another value gets
// XmlCursor_DocPath(a0)), hash 0x044DDD31 (+36, default 0), the names
// "firstJoint" / "secondJoint" / "endJoint" / "effectorJoint" (+40 / +72
// / +104 / +136), vec3 0xEDAE5B2C (+176, normalized: the f32 at
// 0x00292610 / length), f32 0x0021437F / 0xDD33E5C1 / 0x471281EF (+192..,
// defaults 1, 1, -1).
// Quirk: without the 0x0027873A attribute, +32 is the stack word the
// original never wrote.
DECOMP(00292618, IkChainDef_Read292618) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kMap = 0x010578A4, kEmpty = 0x00AD4B7C, kVec = 0x01164B70, kKinds = 0x00AD4FD4, kOne = 0x00292610;
    Stack frame(ctx, 80);
    const u32 L = frame.addr();  // the original's r1 + 112
    store<u32>(L + 8, 0);
    store<u32>(L + 32, load<u32>(kVec));  // lfs / stfs
    store<u32>(L, kEmpty);
    store<u32>(L + 12, kEmpty);
    store<u32>(L + 16, kEmpty);
    store<u32>(L + 20, kEmpty);
    store<u32>(L + 36, load<u32>(kVec + 4));
    store<u32>(L + 24, kEmpty);
    store<u32>(L + 40, load<u32>(kVec + 8));
    store<u32>(L + 48, 0x3F800000);
    store<u32>(L + 52, 0x3F800000);
    store<u32>(L + 56, 0xBF800000);
    store<u32>(L, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrName})));
    if (call(ctx, xfn::XmlCursor_ReadHash, {c, 0x0027873A, L + 64, 0}) == 0) {
        u32 i = 0;
        while (i < 2 && load<u32>(L + 64) != load<u32>(kKinds + 4 * i)) ++i;
        if (i < 2)
            store<u32>(L + 4, i);
        else
            call(ctx, m8fn::XmlCursor_DocPath, {a0});
    }
    XHash(ctx, c, 0x044DDD31, L + 8);
    store<u32>(L + 12, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD4FE0})));  // "firstJoint"
    store<u32>(L + 16, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD4FF0})));  // "secondJoint"
    store<u32>(L + 20, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD5000})));  // "endJoint"
    store<u32>(L + 24, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD5010})));  // "effectorJoint"
    XVec3(ctx, c, 0xEDAE5B2C, L + 32);
    XF32(ctx, c, 0x0021437F, L + 48);
    XF32(ctx, c, 0xDD33E5C1, L + 52);
    XF32(ctx, c, 0x471281EF, L + 56);
    const f32 x = F32(load<u32>(L + 32)), y = F32(load<u32>(L + 36)), z = F32(load<u32>(L + 40));
    f32 l = static_cast<f32>(static_cast<f64>(y) * y);
    l = static_cast<f32>(std::fma(static_cast<f64>(x), x, l));
    l = static_cast<f32>(std::fma(static_cast<f64>(z), z, l));
    l = static_cast<f32>(std::sqrt(static_cast<f64>(l)));
    const f32 s = static_cast<f32>(static_cast<f64>(F32(load<u32>(kOne))) / l);
    store<u32>(L + 32, W32(x * s));
    store<u32>(L + 36, W32(y * s));
    store<u32>(L + 40, W32(z * s));
    ctx.fpr[1] = y * s;
    const u32 p = static_cast<u32>(call(ctx, m7fn::Mem_Alloc, {208, 0, 16}));
    if (p == 0) return ret(ctx, 0);
    store<u32>(p + 32, load<u32>(L + 4));
    store<u32>(p + 36, load<u32>(L + 8));
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 176 + 4 * k, load<u32>(L + 32 + 4 * k));  // lfs / stfs
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 192 + 4 * k, load<u32>(L + 48 + 4 * k));
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(L + 56)));
    call(ctx, m8fn::String_CopyN, {p, load<u32>(L), 31});
    call(ctx, m8fn::String_CopyN, {p + 40, load<u32>(L + 12), 31});
    call(ctx, m8fn::String_CopyN, {p + 72, load<u32>(L + 16), 31});
    call(ctx, m8fn::String_CopyN, {p + 104, load<u32>(L + 20), 31});
    call(ctx, m8fn::String_CopyN, {p + 136, load<u32>(L + 24), 31});
    const u32 h = static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {p}));
    RegPushKV(ctx, kMap, h, p, true);
    ret(ctx, p);
}

// Def* HeadDef_Read291E6C(?, XmlCursor* c) (reads r4 only): gathered on
// the stack first: "name" (+0) and "headJoint" (+36;
// default the empty string at 0x00AD4B7C), hash 0x044DDD31 (+32, default
// 0), vec3 0xE4127843 / 0x799BEF43 (+80 / +96, default the vector at
// 0x01164B30) and 0xDE06974A (+112, zero), f32 0xB4C235F8 / 0x7C6A11FF /
// 0x112E8F04 / 0x8D0CA293 / 0x7C66969A (+128.., default pi), vec2
// 0x63F2322B (+148, {-FLT_MAX, FLT_MAX}). Then a 160-byte def (Mem_Alloc;
// 0 when that fails) gets them and {String_HashNoCase(+0), def} is
// appended to the map 0x01057874.
DECOMP(00291E6C, HeadDef_Read291E6C) {
    const u32 c = arg<u32>(ctx, 1);
    constexpr u32 kMap = 0x01057874, kEmpty = 0x00AD4B7C, kVec = 0x01164B30, kZero = 0x01164B40, kPi = 0x40490FDB;
    Stack frame(ctx, 96);
    const u32 L = frame.addr();  // the original's r1 + 112
    store<u32>(L, kEmpty);
    store<u32>(L + 8, kEmpty);
    store<u32>(L + 4, 0);
    for (u32 k = 0; k < 3; ++k) {
        store<u32>(L + 16 + 4 * k, load<u32>(kVec + 4 * k));  // lfs / stfs
        store<u32>(L + 32 + 4 * k, load<u32>(kVec + 4 * k));
        store<u32>(L + 48 + 4 * k, load<u32>(kZero + 4 * k));
    }
    for (u32 k = 0; k < 5; ++k) store<u32>(L + 64 + 4 * k, kPi);
    store<u32>(L + 84, 0xFF7FFFFF);
    store<u32>(L + 88, 0x7F7FFFFF);
    store<u32>(L, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrName})));
    store<u32>(L + 8, static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD4F98})));
    XHash(ctx, c, 0x044DDD31, L + 4);
    XVec3(ctx, c, 0xE4127843, L + 16);
    XVec3(ctx, c, 0x799BEF43, L + 32);
    XVec3(ctx, c, 0xDE06974A, L + 48);
    XF32(ctx, c, 0xB4C235F8, L + 64);
    XF32(ctx, c, 0x7C6A11FF, L + 68);
    XF32(ctx, c, 0x112E8F04, L + 72);
    XF32(ctx, c, 0x8D0CA293, L + 76);
    XF32(ctx, c, 0x7C66969A, L + 80);
    XVec2(ctx, c, 0x63F2322B, L + 84);
    const u32 p = static_cast<u32>(call(ctx, m7fn::Mem_Alloc, {160, 0, 16}));
    if (p == 0) return ret(ctx, 0);
    store<u32>(p + 32, load<u32>(L + 4));
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 80 + 4 * k, load<u32>(L + 16 + 4 * k));  // lfs / stfs
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 96 + 4 * k, load<u32>(L + 32 + 4 * k));
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 112 + 4 * k, load<u32>(L + 48 + 4 * k));
    for (u32 k = 0; k < 7; ++k) store<u32>(p + 128 + 4 * k, load<u32>(L + 64 + 4 * k));
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(L + 88)));
    call(ctx, m8fn::String_CopyN, {p, load<u32>(L), 31});
    call(ctx, m8fn::String_CopyN, {p + 36, load<u32>(L + 8), 31});
    const u32 h = static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {p}));
    RegPushKV(ctx, kMap, h, p, true);
    ret(ctx, p);
}

// Def* DefNew_002741F4(a0, XmlCursor* c): the "name" attribute (none, or
// a def of that String_HashNoCase already in the map 0x01057544 ->
// XmlCursor_DocPath(a0), 0); otherwise the def's fields are read into a
// stack record ({+0 the name, +4 hash 0x0027873A, bools +8 / +9 / +10,
// hashes +12..+24, f32 +28 (FLT_MAX), +32 (-1), +36..+48 (0), +52 (1),
// hashes +56..+72}, all other defaults 0) and a 128-byte def is built from
// it (Mem_Alloc(128, 0, 16), then Def_Construct273E90(def, &record));
// returns the def (0 when Mem_Alloc fails).
DECOMP(002741F4, DefNew_002741F4) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kMap = 0x01057544, SortedMap_Find749B9C = 0x00749B9C, Def_Construct273E90 = 0x00273E90;
    Stack frame(ctx, 80);
    const u32 key = frame.addr(), L = key + 4;  // the original's r1 + 112 / + 116
    const u32 name = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrName}));
    if (name == 0) {
        call(ctx, m8fn::XmlCursor_DocPath, {a0});
        return ret(ctx, 0);
    }
    store<u32>(key, static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {name})));
    if (load<u32>(static_cast<u32>(call(ctx, SortedMap_Find749B9C, {kMap, key}))) != 0) {
        call(ctx, m8fn::XmlCursor_DocPath, {a0});
        return ret(ctx, 0);
    }
    store<u32>(L, name);
    store<u32>(L + 4, 0);
    for (u32 k = 0; k < 3; ++k) store<u8>(L + 8 + k, 0);
    for (u32 off = 12; off < 28; off += 4) store<u32>(L + off, 0);
    store<u32>(L + 28, 0x7F7FFFFF);
    store<u32>(L + 32, 0xBF800000);
    for (u32 off = 36; off < 52; off += 4) store<u32>(L + off, 0);
    store<u32>(L + 52, 0x3F800000);
    for (u32 off = 56; off < 76; off += 4) store<u32>(L + off, 0);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xF7B7D48C, L + 12, 0});
    XHash(ctx, c, 0xF7B7BD79, L + 16);
    XHash(ctx, c, 0xFF49D35B, L + 20);
    XHash(ctx, c, 0xFF450835, L + 24);
    XHash(ctx, c, 0x0027873A, L + 4);
    XF32(ctx, c, 0xDC7AD045, L + 28);
    XF32(ctx, c, 0xDC7ACF57, L + 32);
    XF32(ctx, c, 0x716A8C33, L + 36);
    XF32(ctx, c, 0xB2E21D07, L + 40);
    XBool(ctx, c, 0x7C172E97, L + 8);
    XBool(ctx, c, 0xF7B80BCC, L + 9);
    XBool(ctx, c, 0xF7B7FE06, L + 10);
    XF32(ctx, c, 0xE0490B9B, L + 44);
    XF32(ctx, c, 0x0906364F, L + 48);
    XF32(ctx, c, 0x7FD0E766, L + 52);
    XHash(ctx, c, 0x9178C4CB, L + 56);
    XHash(ctx, c, 0x2DD296EB, L + 60);
    XHash(ctx, c, 0x23E2109C, L + 64);
    XHash(ctx, c, 0x57D2C1CB, L + 68);
    XHash(ctx, c, 0x23E11794, L + 72);
    const u32 p = static_cast<u32>(call(ctx, m7fn::Mem_Alloc, {128, 0, 16}));
    if (p == 0) return ret(ctx, 0);
    call(ctx, Def_Construct273E90, {p, L});
    ret(ctx, p);
}

// Def* DefNew_002C0EA4(a0, XmlCursor* c): the "name" attribute (none, or a
// def of that String_HashNoCase already in the map 0x0105ABB8 ->
// XmlCursor_DocPath(a0), 0); the fields are read on the stack first:
// hashes 0x03B88866 / 0x03B88850 / 0x03B88855 (+40 / +44 / +48), the
// index of the hash 0x0027873A in the nine at 0x00AD5534 (+60; another
// value gets XmlCursor_DocPath(a0)), hash 0x044DDD31 (+52), bool
// 0xE589ABCC (+56), f32 0x89040DB0 / 0x6327B013 / 0xED297827 (+64.., -1,
// -1, FLT_MAX), hash 0xA80C013B (+76). Then an 80-byte def (Mem_Alloc; 0
// when that fails), vtable 0x00B1D020, the name copied to +4 (31 chars),
// +36 its String_HashNoCase, {hash, def} appended to the map.
// Quirk: without the 0x0027873A attribute, +60 is 0 (the stack default).
DECOMP(002C0EA4, DefNew_002C0EA4) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kMap = 0x0105ABB8, kKinds = 0x00AD5534, SortedMap_Find75E260 = 0x0075E260;
    Stack frame(ctx, 64);
    const u32 L = frame.addr();  // the original's r1 + 112
    const u32 name = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrName}));
    if (name == 0) {
        call(ctx, m8fn::XmlCursor_DocPath, {a0});
        return ret(ctx, 0);
    }
    store<u32>(L, static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {name})));
    if (load<u32>(static_cast<u32>(call(ctx, SortedMap_Find75E260, {kMap, L}))) != 0) {
        call(ctx, m8fn::XmlCursor_DocPath, {a0});
        return ret(ctx, 0);
    }
    store<u32>(L + 8, name);
    for (u32 off = 12; off < 32; off += 4) store<u32>(L + off, 0);
    store<u8>(L + 32, 0);
    store<u32>(L + 36, 0xBF800000);
    store<u32>(L + 40, 0xBF800000);
    store<u32>(L + 44, 0x7F7FFFFF);
    store<u32>(L + 48, 0);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x03B88866, L + 12, 0});
    XHash(ctx, c, 0x03B88850, L + 16);
    XHash(ctx, c, 0x03B88855, L + 20);
    if (call(ctx, xfn::XmlCursor_ReadHash, {c, 0x0027873A, L + 4, 0}) == 0) {
        u32 i = 0;
        while (i < 9 && load<u32>(L + 4) != load<u32>(kKinds + 4 * i)) ++i;
        if (i < 9)
            store<u32>(L + 24, i);
        else
            call(ctx, m8fn::XmlCursor_DocPath, {a0});
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x044DDD31, L + 28, 0});
    XBool(ctx, c, 0xE589ABCC, L + 32);
    XF32(ctx, c, 0x89040DB0, L + 36);
    XF32(ctx, c, 0x6327B013, L + 40);
    XF32(ctx, c, 0xED297827, L + 44);
    XHash(ctx, c, 0xA80C013B, L + 48);
    const u32 p = static_cast<u32>(call(ctx, m7fn::Mem_Alloc, {80, 0, 16}));
    if (p == 0) return ret(ctx, 0);
    store<u32>(p, 0x00B1D020);
    store<u32>(p + 40, load<u32>(L + 12));
    store<u32>(p + 44, load<u32>(L + 16));
    store<u32>(p + 48, load<u32>(L + 20));
    store<u32>(p + 52, load<u32>(L + 28));
    store<u8>(p + 56, load<u8>(L + 32));
    store<u32>(p + 60, load<u32>(L + 24));
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 64 + 4 * k, load<u32>(L + 36 + 4 * k));  // lfs / stfs
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(L + 44)));
    store<u32>(p + 76, load<u32>(L + 48));
    call(ctx, m8fn::String_CopyN, {p + 4, load<u32>(L + 8), 31});
    store<u32>(p + 36, static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {p + 4})));
    const u32 h = static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {p + 4}));
    RegPushKV(ctx, kMap, h, p, true);
    ret(ctx, p);
}

// u32 XmlDef_Read_001F5E50(a0, XmlCursor* c, Def* o): the base part
// (XmlDef_ReadBase), hash +240, vec3 +256, the euler angles 0xC7400937
// (degrees, default zero; constants at 0x001F5E44) as the quaternion +272,
// f32 +288 / +292 / +296, hash +300, bool +304 (each defaulting to its
// value). Returns the last ReadBool result.
DECOMP(001F5E50, XmlDef_Read_001F5E50) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    constexpr u32 kZero = 0x01164B40, K = 0x001F5E44;
    Stack frame(ctx, 16);
    const u32 ang = frame.addr();
    call(ctx, mid6::XmlDef_ReadBase, {a0, c, o});
    XHash(ctx, c, 0x2899600F, o + 240);
    XVec3(ctx, c, 0xEF4DDD13, o + 256);
    for (u32 k = 0; k < 3; ++k) store<u32>(ang + 4 * k, load<u32>(kZero + 4 * k));  // lfs / stfs
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xC7400937, ang, kZero);
    const mid7::EulerQuat q = mid7::EulerToQuat(ctx, ang, K);
    store<u32>(o + 284, W32(q.w));
    store<u32>(o + 272, W32(q.x));
    store<u32>(o + 280, W32(q.z));
    store<u32>(o + 276, W32(q.y));
    XF32(ctx, c, 0xDD61F47C, o + 288);
    XF32(ctx, c, 0x3CC6000D, o + 292);
    XF32(ctx, c, 0x6AA47C5A, o + 296);
    XHash(ctx, c, 0x6AA4B8C7, o + 300);
    ret(ctx, XBool(ctx, c, 0xEA3BBF97, o + 304));
}

// u32 XmlDef_Read_001C6838(a0, XmlCursor* c, Def* o): the reader of the
// 001C6D30 def: the base part (XmlDef_ReadBase), f32 +268 / +240..+264,
// hashes +272 / +276, f32 +280 / +284 (each defaulting to its value), bit
// 30 of the flags +768 (bool 0xA8DB95B1); six 80-byte slots at +288 from
// the 0xEB9EE198 children (the first six; {+0 hash 0x75849CFE, +16 / +32
// vec3 0x2B7B3EB4 / 0x313AB0C1, +48 the euler angles 0x313AB844 (degrees;
// constants at 0x001C6830) as a quaternion when present, +64 hash
// 0x632DA119, bools +68 / +69}); bit 31 of +768 = whether there were any.
// Returns that bit.
DECOMP(001C6838, XmlDef_Read_001C6838) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    constexpr u32 kZero = 0x01164B40, K = 0x001C6830, kSlot = 0xEB9EE198;
    Stack frame(ctx, 32);
    const u32 ang = frame.addr(), t128 = ang + 16;  // the original's r1 + 112 / + 128
    call(ctx, mid6::XmlDef_ReadBase, {a0, c, o});
    XF32(ctx, c, 0x7B2F95A0, o + 268);
    constexpr u32 kF[7] = {0xD5F932F0, 0xEABE8E36, 0x800ACF3A, 0xF9217A56, 0x7BA17740, 0xA9262FCF, 0xA88C3581};
    for (u32 k = 0; k < 7; ++k) XF32(ctx, c, kF[k], o + 240 + 4 * k);
    XHash(ctx, c, 0x2DF2E2B5, o + 272);
    XHash(ctx, c, 0x53FFAFC2, o + 276);
    XF32(ctx, c, 0x667AB4F5, o + 280);
    XF32(ctx, c, 0x7A392528, o + 284);
    store<u8>(t128, 0);
    mid5::XBit<u32>(ctx, c, 0xA8DB95B1, o + 768, 0x40000000u, t128);
    u32 n = 0;
    for (u32 node = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, kSlot})); node != 0;
         node = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {node, kSlot})), ++n) {
        if (n >= 6) continue;
        const u32 s = o + 288 + 80 * n;
        XHash(ctx, node, 0x75849CFE, s);
        XVec3(ctx, node, 0x2B7B3EB4, s + 16);
        XVec3(ctx, node, 0x313AB0C1, s + 32);
        if (XRead(ctx, xfn::XmlCursor_ReadVec3, node, 0x313AB844, ang, kZero) == 0) {
            const mid7::EulerQuat q = mid7::EulerToQuat(ctx, ang, K);
            store<u32>(s + 60, W32(q.w));
            store<u32>(s + 48, W32(q.x));
            store<u32>(s + 56, W32(q.z));
            store<u32>(s + 52, W32(q.y));
            ctx.fpr[1] = q.y;
        }
        XHash(ctx, node, 0x632DA119, s + 64);
        XBool(ctx, node, 0x8141EC8A, s + 68);
        XBool(ctx, node, 0xF8E455AC, s + 69);
    }
    const u32 any = n != 0 ? 1u : 0u;
    store<u32>(o + 768, (load<u32>(o + 768) & 0x7FFFFFFFu) | (any << 31));
    ret(ctx, any);
}

// Def* DefNew_0033322C(?, XmlCursor* c, x) (reads r4 / r5): the fields are
// read on the stack (hash 0x7A30D7E3, vec3 0x8ACA5893 (zero), bools
// 0x5D28E3FC / 0x9446A097 / 0xD9159FE7 / 0x09DA53D5, hash 0x6E6EDD6B;
// bool 0x3B66FD69 sets the pair 0x04656B91 / 0x310F51B2, else they are
// read; bool 0x37785A87), then a 2624-byte object (SmallAlloc; 0 when that
// fails) is built by Obj_Construct332E28(p, x, hash, b6, &vec, b0..b5,
// hash2) (twelve arguments, the last four on the stack).
DECOMP(0033322C, DefNew_0033322C) {
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    constexpr u32 kZero = 0x01164B40, Obj_Construct332E28 = 0x00332E28;
    Stack frame(ctx, 48);
    const u32 vec = frame.addr(), h160 = vec + 16, h164 = vec + 20, b = vec + 24;  // the original's r1 + 144 / 160 / 164 / 168
    store<u32>(h160, 0);
    store<u32>(h164, 0);
    for (u32 k = 0; k < 3; ++k) store<u32>(vec + 4 * k, load<u32>(kZero + 4 * k));  // lfs / stfs
    for (u32 k = 0; k < 7; ++k) store<u8>(b + k, 0);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kZero + 8)));  // the copy went through f1
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, h160, 0);
    XVec3(ctx, c, 0x8ACA5893, vec);
    store<u8>(b + 7, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x3B66FD69, b + 7, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x5D28E3FC, b + 1, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x9446A097, b + 2, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xD9159FE7, b + 3, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x09DA53D5, b + 4, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x6E6EDD6B, h164, 0);
    if (load<u8>(b + 7) != 0) {
        store<u8>(b, 1);
        store<u8>(b + 5, 1);
    } else {
        XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x04656B91, b, 0);
        XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x310F51B2, b + 5, 0);
    }
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x37785A87, b + 6, 0);
    const u32 p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {2624}));
    if (p == 0) return ret(ctx, 0);
    call(ctx, Obj_Construct332E28, {p, x, load<u32>(h160), load<u8>(b + 6), vec, load<u8>(b), load<u8>(b + 1), load<u8>(b + 2),
                                    load<u8>(b + 3), load<u8>(b + 4), load<u8>(b + 5), load<u32>(h164)});
    ret(ctx, p);
}

// u32 XmlDef_Read_00223D38(a0, XmlCursor* c, x, const Def* tmpl, Def* o):
// the base part (Def_ReadBase20BFE8(a0, c, x, tmpl, o)), +32 = 1; with a
// template lacking bit 0x2 of its +24 word, XmlCursor_DocPath(a0); hash
// +84, hash +88 (0), +92 = the index of the hash 0xEC2CEB42 in the three
// at 0x00AD38B0 (0 when absent or unknown), f32 +96..+116 (default the
// f32 at 0x00223D34), bool +120 (0). Returns the ReadBool result.
DECOMP(00223D38, XmlDef_Read_00223D38) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    constexpr u32 Def_ReadBase20BFE8 = 0x0020BFE8, K = 0x00223D34, kKinds = 0x00AD38B0;
    Stack frame(ctx, 16);
    const u32 t = frame.addr();
    call(ctx, Def_ReadBase20BFE8, {a0, c, x, tmpl, o});
    store<u32>(o + 32, 1);
    if (tmpl != 0 && (load<u32>(tmpl + 24) & 2u) == 0) call(ctx, m8fn::XmlCursor_DocPath, {a0});
    XHash(ctx, c, 0xF89FDD94, o + 84);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x98B61B57, o + 88, 0);
    store<u32>(o + 92, 0);
    store<u32>(t, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xEC2CEB42, t, 0);
    if (load<u32>(t) != 0) {
        for (u32 i = 0; i < 3; ++i)
            if (load<u32>(t) == load<u32>(kKinds + 4 * i)) {
                store<u32>(o + 92, i);
                break;
            }
    }
    constexpr u32 kF[6] = {0x7C8F6049, 0x8FE21435, 0x1FA627BC, 0x8C014439, 0xE0E67DAF, 0xE92958FC};
    for (u32 k = 0; k < 6; ++k) XF32Def(ctx, c, kF[k], o + 96 + 4 * k, K);
    ret(ctx, XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x8F8BE00A, o + 120, 0));
}

// u32 XmlDef_Read_00219FF8(a0, XmlCursor* c, x, const Def* tmpl, Def* o):
// the base part (Def_ReadBase20BFE8), then the defaults (zero, or with a
// template having bit 0x08000000 of its +20 word: its vec3 +176, hash
// +192, f32 +196..+204, bools +208 / +209): vec3 0x3C6AE50B +96 (an angle
// when present: scaled by the f32 at 0x00219FF0), hash 0x632DA119 +112,
// f32 +116 / +120 / +124, bools +128..+131. Returns the last ReadBool
// result. (The original also fills a larger default def record on the
// stack that nothing reads.)
DECOMP(00219FF8, XmlDef_Read_00219FF8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    constexpr u32 Def_ReadBase20BFE8 = 0x0020BFE8, kZero = 0x01164B40, kScale = 0x00219FF0;
    Stack frame(ctx, 48);
    const u32 D = frame.addr();  // the original's r1 + 208
    call(ctx, Def_ReadBase20BFE8, {a0, c, x, tmpl, o});
    for (u32 k = 0; k < 3; ++k) store<u32>(D + 4 * k, load<u32>(kZero + 4 * k));  // lfs / stfs
    for (u32 off = 16; off < 32; off += 4) store<u32>(D + off, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(D + 32 + k, 0);
    if (tmpl != 0 && (load<u32>(tmpl + 20) & 0x08000000u) != 0) {
        for (u32 k = 0; k < 3; ++k) store<u32>(D + 4 * k, load<u32>(tmpl + 176 + 4 * k));
        store<u32>(D + 16, load<u32>(tmpl + 192));
        for (u32 k = 0; k < 3; ++k) store<u32>(D + 20 + 4 * k, load<u32>(tmpl + 196 + 4 * k));
        store<u8>(D + 32, load<u8>(tmpl + 208));
        store<u8>(D + 33, load<u8>(tmpl + 209));
    }
    if (XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x3C6AE50B, o + 96, D) == 0) {
        const f32 k = F32(load<u32>(kScale));
        f32 v[3];
        for (u32 j = 0; j < 3; ++j) v[j] = F32(load<u32>(o + 96 + 4 * j)) * k;
        for (u32 j = 0; j < 3; ++j) store<u32>(o + 96 + 4 * j, W32(v[j]));
        ctx.fpr[1] = v[0];
    }
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x632DA119, o + 112, load<u32>(D + 16));
    XF32Def(ctx, c, 0x1486FF40, o + 116, D + 20);
    XF32Def(ctx, c, 0x96B07E43, o + 120, D + 24);
    XF32Def(ctx, c, 0x1B75F0DA, o + 124, D + 28);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xC53B9DE4, o + 128, load<u8>(D + 32));
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xF7C51457, o + 129, load<u8>(D + 33));
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x013BDD92, o + 130, load<u8>(D + 34));
    ret(ctx, XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x23872185, o + 131, load<u8>(D + 35)));
}

// u32 Def_ReadKind481828(a0, XmlCursor* c, Def* o): the kind hash
// 0x0027873A picks +5416 and what is read (absent: return that non-zero
// ReadHash result): 0x61313EAC -> 1, Def_Read480E68(a0, c, o);
// 0x0C8B8AB8 -> 3, hash 0x760CB725 at +0; 0xA7CFB837 -> 4, hashes
// 0x0353CB09 / 0x760CB725 / 0x922D3BD1 (+0..+8, 0), six f32 +12..+32
// (defaults the f32 at 0x0048180C..), Def_Read3F0990(a0, c, o);
// 0x871F673D -> 2. Returns the last call's result (2 for 0x871F673D, the
// hash itself for an unknown kind).
DECOMP(00481828, Def_ReadKind481828) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    constexpr u32 Def_Read480E68 = 0x00480E68, Def_Read3F0990 = 0x003F0990, K = 0x0048180C;
    Stack frame(ctx, 16);
    const u32 t = frame.addr();
    const u32 r = XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x0027873A, t, 0);
    if (r != 0) return ret(ctx, r);
    const u32 h = load<u32>(t);
    switch (h) {
    case 0x61313EAC:
        store<u32>(o + 5416, 1);
        return ret(ctx, call(ctx, Def_Read480E68, {a0, c, o}));
    case 0x0C8B8AB8:
        store<u32>(o + 5416, 3);
        return ret(ctx, XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x760CB725, o, 0));
    case 0xA7CFB837: {
        store<u32>(o + 5416, 4);
        XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x0353CB09, o, 0);
        XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x760CB725, o + 4, 0);
        XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x922D3BD1, o + 8, 0);
        constexpr u32 kF[6] = {0xAF9EDCAB, 0xAF9EDBBD, 0x313AA4BF, 0x313AA3D1, 0x00F35D02, 0x807548E2};
        for (u32 k = 0; k < 6; ++k) XF32Def(ctx, c, kF[k], o + 12 + 4 * k, K + 4 * k);
        return ret(ctx, call(ctx, Def_Read3F0990, {a0, c, o}));
    }
    case 0x871F673D:
        store<u32>(o + 5416, 2);
        return ret(ctx, 2);
    default:
        return ret(ctx, h);
    }
}
