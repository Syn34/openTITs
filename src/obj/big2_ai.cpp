// Game code, large-function sweep part 2: 00011D08, the shared setup of
// an AI controller bound to its body object (10 callers, the controller
// classes' attach functions).
#include "big2.h"

using namespace sly4;
using namespace sly4::big2;

namespace b2a {
constexpr u32 GetFlag_002697F8 = 0x002697F8;         // (o) -> bit 31 of o->+416
constexpr u32 Camera_FovScale_005206F8 = 0x005206F8; // (Camera*) -> f1
constexpr u32 Ctrl_Fn0001199C = 0x0001199C;          // (ctrl) (name?)
constexpr u32 kWorldSlot = 0x00F65CA0;               // -> the world; +0x128168 the active camera
constexpr u32 kCameraOff = 0x00128168;
constexpr u32 kNavSlot = 0x00F557F4;                 // -> nav graph {+4 -> {+2 u16 node count}, +12 the 20-byte nodes {+4 -> obj, +128 vec3}}
constexpr u32 kCtrlListSlot = 0x00F557F0;            // -> the controller lists {+0 n, +4 m, +16 k, +592 [256], +1616 [], +3664 [9]}
constexpr u32 kRandState = 0x01164888;               // the shared RNG {+0 u64 last, +8 seed}
constexpr u32 kTeamBase = 0x010AAFDC;                // +16 -> {+64 the current index into a node's u32 flags}
constexpr u32 kVec4Default = 0x01164C90;             // f32[4]
constexpr u32 kF0 = 0x00000000u, kF1 = 0x3F800000u, kFMax = 0x7F7FFFFFu;  // the .text constants at 0x00011CF8
}  // namespace b2a

namespace {

inline f32 Fl(u32 bits) { return std::bit_cast<f32>(bits); }
inline u32 Bits(f32 f) { return std::bit_cast<u32>(f); }
inline u32 Rotl(u32 v, u32 n) { return (v << n) | (v >> (32 - n)); }

// The nav node nearest (squared distance, single precision as the
// original's fsubs / fmuls / fmadds) to the vec3 at p, or -1 (none
// closer than FLT_MAX). f1 = the smallest distance found.
s32 NearestNode(PPUContext& ctx, u32 g, u32 p, u32 n) {
    f32 best = Fl(b2a::kFMax);
    s32 found = -1;
    const f32 x = Fl(load<u32>(p)), y = Fl(load<u32>(p + 4)), z = Fl(load<u32>(p + 8));
    for (u32 j = 0; j < n; ++j) {
        const u32 q = load<u32>(load<u32>(g + 12) + 20 * j + 4) + 128;
        const f32 dy = y - Fl(load<u32>(q + 4));
        const f32 dx = x - Fl(load<u32>(q + 0));
        const f32 dz = z - Fl(load<u32>(q + 8));
        f32 d = dy * dy;
        d = static_cast<f32>(std::fma(static_cast<f64>(dx), static_cast<f64>(dx), static_cast<f64>(d)));
        d = static_cast<f32>(std::fma(static_cast<f64>(dz), static_cast<f64>(dz), static_cast<f64>(d)));
        if (best > d) {
            best = d;
            found = static_cast<s32>(j);
        }
    }
    ctx.fpr[1] = static_cast<f64>(best);
    return found;
}

}  // namespace

// AiCtrl_Bind_00011D08(Ctrl* o, Body* b): o->+48 = b, b->+244 = o; +64 /
// +68 from b's vfn[15] / vfn[16]; +305 bit 0 = GetFlag(+68); +60 = b when
// its +192 has 0x8; b's matrix rows (+96..) into +128..+184 and b->+172
// |= 0x10000; with an active camera, b->+208 raised to (b->+160)->+312 /
// FovScale^2 when b's model kind is 8; +224 = b's nav node (vfn[5] of
// b->+220 and its s16 +116) or the nearest one to +176; a random +304 bit
// 1 when the node's team flag is set; the flag bytes, the positions +80 /
// +96 / +192 / +272, the forward vector +192 normalised; registered in
// the controller lists (up to 256, the vfn[30] / vfn[31] sub-lists, up to
// 9 of +60). r3 as the original leaves it (ignored by the callers).
DECOMP(00011D08, AiCtrl_Bind_00011D08) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    store<u32>(o + 48, b);
    store<u32>(b + 244, o);
    store<u32>(o + 64, static_cast<u32>(VCall(ctx, b, 60, {b})));
    const u32 c68 = static_cast<u32>(VCall(ctx, load<u32>(o + 48), 64, {load<u32>(o + 48)}));
    store<u32>(o + 68, c68);
    const u32 flag = c68 != 0 ? static_cast<u32>(call(ctx, b2a::GetFlag_002697F8, {c68})) : 0u;
    store<u8>(o + 305, static_cast<u8>((load<u8>(o + 305) & 0xFEu) | (flag & 1u)));
    u32 body = load<u32>(o + 48);
    store<u32>(o + 60, body != 0 && (load<u32>(body + 192) & 0x8u) != 0 ? body : 0u);
    store<u32>(o + 52, 0);
    store<u32>(o + 212, 0);
    for (const u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u}) store<u32>(o + 128 + off, load<u32>(body + 96 + off));
    ctx.set_fpr_bits(1, LfsBits(load<u32>(body + 152)));
    store<u32>(body + 172, load<u32>(body + 172) | 0x10000u);
    const u32 cam = load<u32>(load<u32>(b2a::kWorldSlot) + b2a::kCameraOff);
    body = load<u32>(o + 48);
    if (cam != 0) {
        const f32 s = static_cast<f32>(callf(ctx, b2a::Camera_FovScale_005206F8, {cam}, {}));
        body = load<u32>(o + 48);
        const u32 m = load<u32>(body + 160);
        if (m != 0 && ((load<u32>(m + 4) >> 10) & 0x3Fu) == 8) {
            const f32 cur = Fl(load<u32>(body + 208));
            const f32 q = Fl(load<u32>(m + 312)) / (s * s);
            ctx.fpr[1] = static_cast<f64>(q);
            if (cur != Fl(b2a::kF0) && q > cur) store<u32>(body + 208, Bits(q));
        }
    }

    // The nav node.
    u32 node = 0;
    if (const u32 nav = load<u32>(body + 220); nav != 0 && static_cast<u32>(VCall(ctx, nav, 20, {nav})) != 0) {
        const u32 g = load<u32>(b2a::kNavSlot);
        const s32 i = static_cast<s16>(load<u16>(nav + 116));
        if (i >= 0 && i < static_cast<s32>(load<u16>(load<u32>(g + 4) + 2))) node = load<u32>(g + 12) + static_cast<u32>(i) * 20;
    }
    store<u32>(o + 224, node);
    if (node == 0) {
        const u32 g = load<u32>(b2a::kNavSlot);
        const u32 n = load<u16>(load<u32>(g + 4) + 2);
        const s32 i = static_cast<s16>(static_cast<u16>(NearestNode(ctx, g, o + 176, n)));  // extsh
        node = (i >= 0 && i < static_cast<s32>(n)) ? load<u32>(g + 12) + static_cast<u32>(i) * 20 : 0u;
        store<u32>(o + 224, node);
    }
    u32 f304 = load<u8>(o + 304) & ~0x2u;
    if (node != 0) {
        store<u8>(o + 304, static_cast<u8>(f304));
        const u32 flags = load<u32>(load<u32>(node) + 12);
        const u32 idx = load<u32>(load<u32>(b2a::kTeamBase + 16) + 64);
        if (load<u8>(flags + ((idx << 2) & 0xFFFFFFFCu)) != 0) {
            const u32 seed = load<u32>(b2a::kRandState + 8);
            const u64 sum = static_cast<u64>(Rotl(seed, 4)) + static_cast<u64>(Rotl(seed, 16));
            const u64 r = sum ^ 0xFFFFFFFF87654321ull;
            store<u32>(b2a::kRandState + 8, Rotl(static_cast<u32>(r), 16));
            store<u64>(b2a::kRandState, r & 0xFFFFu);
            f304 = (load<u8>(o + 304) & ~0x2u) | (((static_cast<u32>(r) >> 16) & 1u) << 1);
        }
    }
    {
        const u32 f305 = load<u8>(o + 305), f306 = load<u8>(o + 306);
        store<u32>(o + 76, 0);
        store<u8>(o + 306, static_cast<u8>(f306 | 0x40u));
        store<u8>(o + 304, static_cast<u8>(f304 & 0x2u));
        store<u8>(o + 305, static_cast<u8>((f305 & ~0x22u) | 0x40u));
        store<u32>(o + 96, load<u32>(o + 176));
        store<u32>(o + 100, load<u32>(o + 180));
        store<u32>(o + 72, 0);
        store<u32>(o + 104, load<u32>(o + 184));
        ctx.set_fpr_bits(1, LfsBits(load<u32>(o + 184)));
    }
    if (load<u32>(o + 68) != 0 && static_cast<u32>(VCall(ctx, o, 124, {o})) == 0) {
        const u32 c = load<u32>(o + 68);
        const u32 x = static_cast<u32>(VCall(ctx, c, 8, {c}));
        store<u32>(x + 168, load<u32>(x + 168) | 0x02000000u);
        for (u32 k = 0; k < 3; ++k) store<u32>(x + 16 + 4 * k, load<u32>(b2a::kVec4Default + 4 * k));
        const u32 w3 = load<u32>(b2a::kVec4Default + 12);
        store<u32>(x + 0, 0);
        store<u32>(x + 28, w3);
        store<u32>(x + 4, 0);
        store<u32>(x + 8, 0);
        ctx.set_fpr_bits(1, LfsBits(w3));
        store<u32>(o + 300, (load<u32>(load<u32>(o + 68) + 416) >> 28) & 7u);
    }
    {
        const u32 f305 = load<u8>(o + 305);
        store<u32>(o + 256, 0);
        store<u8>(o + 305, static_cast<u8>(f305 & ~0x90u));
        store<u32>(o + 208, b2a::kF1);
        store<u32>(o + 292, 0);
        store<u32>(o + 32, 0);
        store<u32>(o + 44, 0);
        store<u32>(o + 4, 0xFFFFFFFFu);
        store<u32>(o + 28, 0xFFFFFFFFu);
        store<u64>(o + 8, 0);
        store<u64>(o + 16, 0);
        store<u32>(o + 24, 0);
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 80 + 4 * k, load<u32>(o + 176 + 4 * k));
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 272 + 4 * k, load<u32>(o + 160 + 4 * k));
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 192 + 4 * k, load<u32>(o + 160 + 4 * k));
        ctx.set_fpr_bits(1, LfsBits(load<u32>(o + 168)));
    }
    {
        const u32 f305 = load<u8>(o + 305);
        // (0x20 was just cleared and 0x40 set, so the original always
        // clears +196 here and never makes the vfn[33] call.)
        if ((f305 & 0x20u) == 0) {
            if ((f305 & 0x40u) != 0 || static_cast<u32>(VCall(ctx, o, 132, {o})) == 0) store<u32>(o + 196, 0);
        }
        const f32 y = Fl(load<u32>(o + 196)), x = Fl(load<u32>(o + 192)), z = Fl(load<u32>(o + 200));
        f32 l = y * y;
        l = static_cast<f32>(std::fma(static_cast<f64>(x), static_cast<f64>(x), static_cast<f64>(l)));
        l = static_cast<f32>(std::fma(static_cast<f64>(z), static_cast<f64>(z), static_cast<f64>(l)));
        l = static_cast<f32>(std::sqrt(static_cast<f64>(l)));
        const f32 inv = Fl(b2a::kF1) / l;
        const f32 nx = x * inv, ny = y * inv, nz = z * inv;
        store<u32>(o + 192, Bits(nx));
        store<u32>(o + 196, Bits(ny));
        store<u32>(o + 200, Bits(nz));
        ctx.fpr[1] = static_cast<f64>(ny);
        ctx.fpr[2] = static_cast<f64>(nx);
        ctx.fpr[3] = static_cast<f64>(nz);
    }
    if (static_cast<u32>(VCall(ctx, o, 268, {o})) != 0) call(ctx, b2a::Ctrl_Fn0001199C, {o});
    const u32 g2 = load<u32>(b2a::kCtrlListSlot);
    const u32 n = load<u32>(g2);
    if (static_cast<s32>(n) >= 256) {
        ret(ctx, n);
        return;
    }
    store<u32>(g2 + 592 + ((n << 2) & 0xFFFFFFFCu), o);
    store<u32>(g2, load<u32>(g2) + 1);
    if (static_cast<u32>(VCall(ctx, o, 120, {o})) != 0) {
        const u32 m = load<u32>(g2 + 4);
        store<u32>(g2 + 1616 + ((m << 2) & 0xFFFFFFFCu), o);
        store<u32>(g2 + 4, load<u32>(g2 + 4) + 1);
    }
    if (static_cast<u32>(VCall(ctx, o, 124, {o})) == 0) return;
    const u32 k = load<u32>(g2 + 16);
    if (static_cast<s32>(k) >= 9) {
        ret(ctx, k);
        return;
    }
    store<u32>(g2 + 3664 + ((k << 2) & 0xFFFFFFFCu), load<u32>(o + 60));
    store<u32>(g2 + 16, load<u32>(g2 + 16) + 1);
    ret(ctx, load<u32>(g2 + 16));
}
