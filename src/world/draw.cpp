// Draw_* helpers: commands written into a frame buffer's command stream.
//
// FrameBuffer: +0 stream start, +4 stream end, +16 write cursor, +68 vertex
// cursor, +72 vertex count, +80 current texture, +96 f32[16] state matrix
// (+160 u32 0 = matrix valid), +176.. cached state.
#include "world.h"

using namespace sly4;
using namespace sly4::world;

namespace {

constexpr u32 kMatrixEpsilon = 0x005218D0;  // f32 1e-5
constexpr u32 kMatrixNearlyEqual = 0x005545B4;

// Reserves a 12-byte command at the cursor (wrapping to the stream start
// when fewer than 16 bytes are left before the end) and writes it.
void Fb_Command(u32 fb, u32 w0, u32 w1, u32 w2) {
    u32 cursor = field<u32>(fb, 16);
    if (cursor + 15 > field<u32>(fb, 4)) cursor = field<u32>(fb, 0);
    cursor = (cursor + 3) & ~3u;
    field<u32>(fb, 16) = cursor;
    store<u32>(cursor + 8, w2);
    store<u32>(cursor, w0);
    store<u32>(cursor + 4, w1);
}

u32 cmd(u32 id, u32 v) { return (id << 24) | (v & 0x00FF0000u); }

}  // namespace

// void Draw_SetTexture(FrameBuffer* fb, Texture* tex)
// Command 4 with the texture's +20/+24, followed by the texture pointer.
DECOMP(00523310, Draw_SetTexture) {
    const u32 fb = arg<u32>(ctx, 0), tex = arg<u32>(ctx, 1);
    if (tex == field<u32>(fb, 80)) return;
    const u32 a = field<u32>(tex, 20), b = field<u32>(tex, 24);
    field<u32>(fb, 84) = a;
    field<u32>(fb, 80) = tex;
    field<u32>(fb, 88) = b;
    field<u64>(fb, 176) = ~0ull;
    field<u64>(fb, 184) = ~0ull;
    field<u32>(fb, 192) = 0xFFFFFFFFu;
    Fb_Command(fb, cmd(4, a), a & 0xFFFF, 0);

    u32 cursor = field<u32>(fb, 16);
    u32 at = cursor + 12;
    if (cursor + 19 > field<u32>(fb, 4)) at = field<u32>(fb, 0);
    // The original returns with this (unaligned) cursor in r7, and
    // Draw_BeginPrims ORs r7 into its command word when callers don't set it.
    ctx.gpr[7] = at;
    at = (at + 3) & ~3u;
    field<u32>(fb, 16) = at;
    store<u32>(at, tex);
    field<u32>(fb, 16) = field<u32>(fb, 16) + 4;
}

// void Draw_SetState(FrameBuffer* fb, const f32 m[16])
// Skipped when m nearly equals the cached matrix. Otherwise caches it,
// writes command 2 and a 16-byte aligned copy of the 64 bytes at m.
DECOMP(005218D8, Draw_SetState) {
    const u32 fb = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    if (field<u32>(fb, 160) == 0) {
        ctx.fpr[1] = load<f32>(kMatrixEpsilon);
        if (static_cast<u32>(call(ctx, kMatrixNearlyEqual, {fb + 96, m})) != 0) return;
    }
    auto copy = [&](u32 dst, u32 src) { store<f32>(fb + dst, static_cast<f32>(static_cast<f64>(load<f32>(m + src)))); };
    for (const u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u}) copy(96 + off, off);
    field<u32>(fb, 160) = 0;
    copy(96 + 52, 52);
    copy(96 + 56, 56);

    Fb_Command(fb, cmd(2, m), m & 0xFFFF, 0);
    u32 at = field<u32>(fb, 16);
    const u32 next = at + 12;
    at = (at + 91 > field<u32>(fb, 4)) ? field<u32>(fb, 0) : next;
    at = (at + 15) & ~15u;
    field<u32>(fb, 16) = at;
    // lvx / stvx: the source address is rounded down to 16 bytes.
    const u32 src = m & ~15u;
    for (u32 k = 0; k < 64; k += 4) store<u32>(at + k, load<u32>(src + k));
    field<u32>(fb, 16) = field<u32>(fb, 16) + 64;
}

// void Draw_BeginPrims(Prims* out, FrameBuffer* fb, u8 type, u16 count, u32 arg)
// Command 19 for `count` vertices of 24 bytes; out = { vertices, count, 0, 0 }.
DECOMP(005236A0, Draw_BeginPrims) {
    const u32 out = arg<u32>(ctx, 0), fb = arg<u32>(ctx, 1);
    const u32 type = arg<u32>(ctx, 2) & 0xFF, count = arg<u32>(ctx, 3), extra = arg<u32>(ctx, 4);
    const u32 first = field<u32>(fb, 72);
    u32 cursor = field<u32>(fb, 16);
    if (cursor + 15 > field<u32>(fb, 4)) cursor = field<u32>(fb, 0);
    cursor = (cursor + 3) & ~3u;
    field<u32>(fb, 16) = cursor;
    store<u32>(cursor, (0x13u << 24) | (extra & 0x00FF0000u) | type);
    store<u32>(cursor + 8, first);
    store<u32>(cursor + 4, (extra & 0xFFFFu) | (count << 16));
    const u32 vertices = field<u32>(fb, 68);
    const u32 total = field<u32>(fb, 72);
    store<u16>(out + 4, static_cast<u16>(count));
    store<u16>(out + 6, 0);
    ref<u8>(out + 8) = 0;
    field<u32>(fb, 68) = vertices + count * 24;
    field<u32>(fb, 72) = total + count;
    field<u32>(fb, 16) = field<u32>(fb, 16) + 12;
    store<u32>(out, vertices);
}

// void Draw_EndPrims(...): empty in the retail build.
DECOMP(00523748, Draw_EndPrims) {}
