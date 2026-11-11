// Game code, medium sweep part 7: XML element / anim command constructors
// that read themselves from a cursor (Obj_InitVt_B21C98 or
// AnimCmd_ConstructBase base, vtable, defaults, attributes).
#include "mid7.h"

using namespace sly4;
using namespace sly4::mid7;

namespace {

namespace efn {
constexpr u32 Obj_InitVt_B21C98 = 0x004BB3B0;      // (o): +4 = 1, vtable 0x00B21C98 (reads r3 only)
constexpr u32 AnimCmd_ConstructBase = 0x004B9CB8;  // (o)
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;   // (c, tag hash) -> node or 0
}  // namespace efn

constexpr u32 kZeroVec = 0x01164B40;  // f32[3] 0
constexpr u32 kVec4C90 = 0x01164C90;  // f32[4] (the identity quaternion)
constexpr u32 kVec4B70 = 0x01164B70;  // f32[3]


}  // namespace

// u32 XmlElem_Ctor_0006DDB8(XmlElem* o, XmlCursor* c): vtable 0x00B0F1A8;
// hash +12, vec3 +16 / +32 (default zero), f32 +60 / +64 / +48 / +52
// (default the f32 at 0x0006DDB4), bool +56, hash +68, +76 = the s32
// attribute 0xC831C860 - 1 (at least 0; -1 when absent), bits 0..12 of
// +72 from the bools of the child 0xB2892E4A named by the table
// 0x00AC66CC; +48 = 0 when it exceeds +52. Returns the child's last bool
// result / +72 (0 without the child); f1 = +48 as read before the clamp.
DECOMP(0006DDB8, XmlElem_Ctor_0006DDB8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 K = 0x0006DDB4;
    call(ctx, efn::Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, load<u32>(kZeroVec));
    store<u32>(o, 0x00B0F1A8);
    store<u32>(o + 20, load<u32>(kZeroVec + 4));
    store<u32>(o + 24, load<u32>(kZeroVec + 8));
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 32 + 4 * k, load<u32>(kZeroVec + 4 * k));
    store<u32>(o + 48, 0);
    store<u32>(o + 52, 0);
    store<u8>(o + 56, 0);
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 60 + 4 * k, 0);
    store<u32>(o + 76, 0xFFFFFFFFu);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7F247172, o + 12, 0);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x313AB0C1, o + 16, kZeroVec);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x8C3BE0FF, o + 32, kZeroVec);
    XF32Def(ctx, c, 0x77C17F6F, o + 60, K);
    XF32Def(ctx, c, 0x9D9B30D5, o + 64, K);
    XF32Def(ctx, c, 0x6CA62860, o + 48, K);
    XF32Def(ctx, c, 0x6CA62772, o + 52, K);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xB38E5179, o + 56, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x632DA119, o + 68, 0);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u32>(s, 0);
    if (XRead(ctx, xfn::XmlCursor_ReadS32, c, 0xC831C860, s, 0) == 0) {
        const s32 v = static_cast<s32>(load<u32>(s));
        store<u32>(o + 76, v > 0 ? static_cast<u32>(v - 1) : 0u);
    }
    const u32 n = static_cast<u32>(call(ctx, efn::XmlCursor_FirstChild, {c, 0xB2892E4A}));
    u32 r = n;
    if (n != 0) {
        for (u32 k = 0; k < 13; ++k) {
            store<u8>(s + 4, 0);
            r = XRead(ctx, xfn::XmlCursor_ReadBool, n, load<u32>(0x00AC66CC + 4 * k), s + 4, 0);
            if (r != 0) continue;
            const u32 m = 1u << k;
            r = load<u8>(s + 4) != 0 ? (load<u32>(o + 72) | m) : (load<u32>(o + 72) & ~m);
            store<u32>(o + 72, r);
        }
    }
    const f32 a = F32(load<u32>(o + 48));
    ctx.fpr[1] = a;
    if (a > F32(load<u32>(o + 52))) store<u32>(o + 48, 0);
    ret(ctx, r);
}

// u32 XmlElem_Ctor_0006CC68(XmlElem* o, XmlCursor* c): vtable 0x00B0F158;
// vec3 +16 (+69 = 1 when present), the euler angles 0x898B7952 (degrees,
// scaled by the f32 at 0x0006CC60) as the quaternion +32 (+70 = 1;
// default identity), hashes +48 (+68 = 1 when present) .. +64, bool +71
// (default true). Returns the last bool result.
DECOMP(0006CC68, XmlElem_Ctor_0006CC68) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 K = 0x0006CC60;  // f32 {deg -> rad, 0.5}
    call(ctx, efn::Obj_InitVt_B21C98, {o});
    store<u32>(o + 16, load<u32>(kZeroVec));
    store<u32>(o + 20, load<u32>(kZeroVec + 4));
    store<u32>(o, 0x00B0F158);
    store<u32>(o + 24, load<u32>(kZeroVec + 8));
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 32 + 4 * k, load<u32>(kVec4C90 + 4 * k));
    store<u32>(o + 48, 0);
    store<u32>(o + 44, load<u32>(kVec4C90 + 12));
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 52 + 4 * k, 0);
    store<u8>(o + 68, 0);
    store<u8>(o + 69, 0);
    store<u8>(o + 70, 0);
    store<u8>(o + 71, 1);
    ctx.fpr[1] = F32(load<u32>(kVec4C90 + 12));
    if (XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x3818B318, o + 16, kZeroVec) == 0) store<u8>(o + 69, 1);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    if (XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x898B7952, s, kZeroVec) == 0) {
        const EulerQuat q = EulerToQuat(ctx, s, K);
        store<u8>(o + 70, 1);  // (the original stores it among the products)
        store<u32>(o + 44, W32(q.w));
        store<u32>(o + 32, W32(q.x));
        store<u32>(o + 40, W32(q.z));
        store<u32>(o + 36, W32(q.y));
        ctx.fpr[1] = q.y;
    }
    if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xA7E293B8, o + 48, 0) == 0) store<u8>(o + 68, 1);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x53D6BF42, o + 52, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x4F11B5C8, o + 56, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x5538A9B9, o + 60, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x30A5A3F1, o + 64, 0);
    ret(ctx, XBool(ctx, c, 0xC09022A5, o + 71));
}

// u32 AnimCmd_Ctor_00088704(AnimCmd* o, XmlCursor* c): vtable 0x00B10300
// and defaults; hashes +12 / +16 / +20, bool +28, s32 +32, f32 +36..+48,
// bool +96, vec3s +64 / +80 (default 0x01164B70); +24 collects the
// "present" bits (1, 2, 4, 8, 0x10, 0x20, 0x100, 0x40, 0x80). Returns the
// last ReadVec3 result, or +24 when that vector was present.
DECOMP(00088704, AnimCmd_Ctor_00088704) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, efn::AnimCmd_ConstructBase, {o});
    store<u32>(o, 0x00B10300);
    store<u32>(o + 32, 2);
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 12 + 4 * k, 0);
    store<u32>(o + 36, 0x3D4CCCCD);
    store<u32>(o + 40, 0x3DCCCCCD);
    store<u32>(o + 44, 0x3DCCCCCD);
    store<u32>(o + 48, 0x41F00000);
    auto bit = [&](u32 r, u32 m) {
        if (r == 0) store<u32>(o + 24, load<u32>(o + 24) | m);
    };
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 12, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 16, 0);
    XHash(ctx, c, 0x1F7FE8E8, o + 20);
    bit(XBool(ctx, c, 0x002480E8, o + 28), 1);
    bit(XS32(ctx, c, 0xA8F0945A, o + 32), 2);
    bit(XF32(ctx, c, 0x922FF60D, o + 36), 4);
    bit(XF32(ctx, c, 0x390C8892, o + 40), 8);
    bit(XF32(ctx, c, 0x38714C6B, o + 44), 0x10);
    const u8 b96 = load<u8>(o + 96);
    bit(XF32(ctx, c, 0xFBCA07B1, o + 48), 0x20);
    bit(XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x724F4DDA, o + 96, b96), 0x100);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    for (u32 k = 0; k < 3; ++k) store<u32>(s + 4 * k, load<u32>(kVec4B70 + 4 * k));
    ctx.fpr[1] = F32(load<u32>(kVec4B70 + 8));
    u32 r = XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x3B83FE2E, s, kZeroVec);
    if (r == 0) {
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 64 + 4 * k, load<u32>(s + 4 * k));
        store<u32>(o + 24, load<u32>(o + 24) | 0x40);
        ctx.fpr[1] = F32(load<u32>(s + 8));
    }
    r = XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x34BF0B12, s, kZeroVec);
    if (r == 0) {
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 80 + 4 * k, load<u32>(s + 4 * k));
        r = load<u32>(o + 24) | 0x80;
        store<u32>(o + 24, r);
        ctx.fpr[1] = F32(load<u32>(s + 8));
    }
    ret(ctx, r);
}
