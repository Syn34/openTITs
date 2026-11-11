// Game code, medium sweep part 9: def readers over the shared base readers
// (DamageColDef_Read_000C85A8 and similar).
#include "mid9.h"

using namespace sly4;
using namespace sly4::mid9;

// u32 XmlDef_Read_001F5938(a0, XmlCursor* c, x, const Def* tmpl, Def* o):
// the base part (DamageColDef_Read_000C85A8(a0, c, x, tmpl, o)), hash
// +224, vec3 +240, the euler angles 0xC7400937 (degrees, default zero;
// constants at 0x001F592C) as the quaternion +256, f32 +272 / +276 / +280,
// hash +284 (each defaulting to its value). Returns the last ReadHash
// result.
DECOMP(001F5938, XmlDef_Read_001F5938) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    constexpr u32 kZero = 0x01164B40, K = 0x001F592C, DamageColDef_Read_000C85A8 = 0x000C85A8;
    Stack frame(ctx, 16);
    const u32 ang = frame.addr();  // the original's r1 + 112
    call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, tmpl, o});
    XHash(ctx, c, 0x2899600F, o + 224);
    XVec3(ctx, c, 0xEF4DDD13, o + 240);
    for (u32 k = 0; k < 3; ++k) CopyW(ang + 4 * k, kZero + 4 * k);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xC7400937, ang, kZero);
    const mid7::EulerQuat q = mid7::EulerToQuat(ctx, ang, K);
    store<u32>(o + 268, W32(q.w));
    store<u32>(o + 256, W32(q.x));
    store<u32>(o + 264, W32(q.z));
    store<u32>(o + 260, W32(q.y));
    XF32(ctx, c, 0xDD61F47C, o + 272);
    XF32(ctx, c, 0x3CC6000D, o + 276);
    XF32(ctx, c, 0x6AA47C5A, o + 280);
    ret(ctx, XHash(ctx, c, 0x6AA4B8C7, o + 284));
}
