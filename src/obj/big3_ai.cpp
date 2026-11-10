// Game code, large-function sweep part 3: the AI controller's character
// proxy rebuild 0001199C (called by AiCtrl_Bind, big2_ai.cpp, and two
// other AI functions).
#include "big3.h"

using namespace sly4;
using namespace sly4::big3;

namespace b3i {
constexpr u32 Quat_FromMatrix = 0x00555478;     // (f32 out[4], const f32 m[4][4])
constexpr u32 Quat_ToMatrix = 0x00553A90;       // (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 Mat_ToQuat67C024 = 0x0067C024;    // (f32 out[4], const f32 row2[3], const f32 row0[3] (row1 follows))
constexpr u32 ProxyCinfo_Construct = 0x006C09B4;  // (cinfo*): Havok middleware (name?)
constexpr u32 Proxy_Create = 0x006C0FF8;          // (const Shape* desc, const cinfo*) -> proxy: Havok middleware (name?)
constexpr u32 NavQuery_Finish = 0x0062E094;     // (q)
constexpr u32 NavQuery_Init = 0x00630F94;       // (q, desc, 0, out)
constexpr u32 NavQuery_Assign = 0x0062D414;     // (dst, src)
constexpr u32 NavQuery_Free = 0x0062D08C;       // (q)
constexpr u32 kWorldPtr = 0x00F557EC;           // u32*: the nav world (*it null: no proxy)
constexpr u32 kVecZero = 0x01164B40;            // f32[3]
}  // namespace b3i

namespace {

inline void StoreF(u32 a, f64 v) {
    if (std::isnan(v)) {
        const u64 b = std::bit_cast<u64>(v);
        store<u32>(a, static_cast<u32>((b >> 32) & 0x80000000u) | 0x7F800000u | static_cast<u32>((b >> 29) & 0x7FFFFFu));
        return;
    }
    store<u32>(a, std::bit_cast<u32>(static_cast<f32>(v)));
}
inline void Copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

}  // namespace

// void AiCtrl_RebuildProxy(AiCtrl* c): when c->vfn[67]() is non-zero:
// no world (**kWorldPtr null) clears c->+212; otherwise a shape desc is
// built on the stack (the position c->+176, the rotation of c's +128
// matrix as rows z / x / y and its quaternion, Mat_ToQuat67C024), a
// proxy cinfo (ProxyCinfo_Construct, then +0 / +44 = the f32 results of
// vfn[15] / vfn[16], +52 / +48 = vfn[17] / vfn[18], +24 = vfn[61]),
// c->+212 = Proxy_Create(desc, cinfo), and c's nav query +216 is
// finished and replaced by a new one on the desc. The locals keep the
// original's frame layout (r1+112..+343) so the middleware sees the same
// offsets and alignment.
DECOMP(0001199C, AiCtrl_RebuildProxy) {
    using namespace b3i;
    const u32 c = arg<u32>(ctx, 0);
    if (static_cast<u32>(VCall(ctx, c, 268, {c})) == 0) return;
    if (load<u32>(load<u32>(kWorldPtr)) == 0) {
        store<u32>(c + 212, 0);
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 232);
    const u32 sp = frame.addr() - 112;  // so sp + off is the original's r1 + off
    const u32 quat = sp + 112, mat = sp + 128, nq = sp + 192, desc = sp + 200, cinfo = sp + 264;
    for (u32 k = 0; k < 12; k += 4) Copy32(desc + k, c + 176 + k);
    call(ctx, Quat_FromMatrix, {quat, c + 128});
    call(ctx, Quat_ToMatrix, {mat, quat, kVecZero});
    // desc +24 / +36 = rows 0 / 1, desc +12 = row 2 (the original's order).
    Copy32(sp + 224, mat + 0);
    Copy32(sp + 228, mat + 4);
    Copy32(sp + 232, mat + 8);
    Copy32(sp + 236, mat + 16);
    Copy32(sp + 240, mat + 20);
    Copy32(sp + 244, mat + 24);
    Copy32(sp + 212, mat + 32);
    Copy32(sp + 216, mat + 36);
    Copy32(sp + 220, mat + 40);
    call(ctx, Mat_ToQuat67C024, {sp + 248, sp + 212, sp + 224, sp + 236});
    call(ctx, ProxyCinfo_Construct, {cinfo});
    VCall(ctx, c, 60, {c});
    StoreF(cinfo + 0, ctx.fpr[1]);
    VCall(ctx, c, 64, {c});
    StoreF(cinfo + 44, ctx.fpr[1]);
    store<u32>(cinfo + 52, static_cast<u32>(VCall(ctx, c, 68, {c})));
    store<u32>(cinfo + 48, static_cast<u32>(VCall(ctx, c, 72, {c})));
    store<u32>(cinfo + 24, static_cast<u32>(VCall(ctx, c, 244, {c})));
    store<u32>(c + 212, static_cast<u32>(call(ctx, Proxy_Create, {desc, cinfo})));
    call(ctx, NavQuery_Finish, {c + 216});
    call(ctx, NavQuery_Init, {nq, desc, 0, cinfo + 12});
    call(ctx, NavQuery_Assign, {c + 216, nq});
    call(ctx, NavQuery_Free, {nq});
}
