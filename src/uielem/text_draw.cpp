// UiText_Draw (text vfunc +156, 0x004E2628).
#include "text_slots.h"

using namespace sly4;
using namespace sly4::uielem;
using namespace sly4::text;

namespace {
constexpr u32 Draw_SetState = 0x005218D8, Draw_SetTexture = 0x00523310;
constexpr u32 Draw_BeginPrims = 0x005236A0, Draw_EndPrims = 0x00523748;
constexpr u32 kUiTarget = 0x00F6688C;      // RenderTarget* the UI draws into (name?)
constexpr u32 kUiFrame = 0x00F66890;       // FrameBuffer* (load.h kUiClear2); 0 = the default below
constexpr u32 kDefaultFrame = 0x010DDBB4;  // FrameBuffer*
constexpr u32 kCorners = 0x00AEF970;       // f32[4][2]: (0,0) (0,1) (1,0) (1,1)
constexpr f32 kMinusOne = -1.0f, kZero = 0.0f, kOne = 1.0f;
constexpr u32 kVfnDraw = 156, kVfnColor = 172, kVfnWorldMatrix = 164;
constexpr u32 kTexWidth = 52, kTexHeight = 54;  // u16 of the page's +20 texture
constexpr u32 kVertexSize = 24;
inline f64 fdivs(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a / b)); }
inline f64 to_f32(u32 v) { return static_cast<f64>(static_cast<f32>(static_cast<f64>(v))); }
}  // namespace

// void UiText_Draw(TextElem* e)  [vfunc +156]
// Draws the glyph quads (those revealed so far) with the element's world
// matrix and colour, one 4-vertex primitive per quad, then the children.
// Glyph quads with bit 19 set use (255, 255, 255, alpha) instead of the colour.
DECOMP(004E2628, UiText_Draw) {
    const u32 e = arg<u32>(ctx, 0);
    if ((field<u32>(e, kElemFlags) & kElemActive) != 0 && field<u32>(e, kTextFontSet) != 0 &&
        field<u32>(e, kTextBuf) != 0) {
        // The original's frame: the matrix at +112, the prims at +176, the
        // colour at +188 and the two vertex colours at +192 / +196.
        constexpr u32 kFrameBase = 112;
        Stack frame(ctx, 216 - kFrameBase);
        const u32 sp = frame.addr() - kFrameBase;
        const u32 world = sp + 112, prims = sp + 176, color = sp + 188, col_a = sp + 192, col_b = sp + 196;

        call_ptr(ctx, vfunc(e, kVfnWorldMatrix), {world, e});
        const u32 target = load<u32>(kUiTarget);
        if (target != 0 && ref<u8>(target + 49) != 1) store<f32>(world + 56, kMinusOne);
        call_ptr(ctx, vfunc(e, kVfnColor), {color, e});
        for (u32 k = 0; k < 4; ++k) store<u8>(col_a + k, load<u8>(color + k));
        for (u32 k = 0; k < 3; ++k) store<u8>(col_b + k, 0xFF);
        store<u8>(col_b + 3, load<u8>(color + 3));

        u32 count = field<u32>(e, kTextQuadCap);
        const f64 reveal = lfs(e + kTextReveal);
        if (reveal > static_cast<f64>(kZero)) {
            const u32 n = static_cast<u32>(rt::fctid(fdivs(lfs(e + kTextRevealElapsed), reveal), true));
            if (n < count) count = n;
        }
        u32 fb = load<u32>(kUiFrame);
        if (fb == 0) fb = load<u32>(kDefaultFrame);
        call(ctx, Draw_SetState, {fb, world});

        u32 lines = 0;
        for (u32 k = 0; k < count; ++k) {
            const u32 q = field<u32>(e, kTextQuads) + k * kQuadSize;
            const u32 line = (static_cast<u32>(load<u64>(q + 16)) & 0x3FF) + 1;
            if (!(lines > line)) lines = line;
            const u32 page = load<u32>(q + 24);
            if (page == 0) continue;
            const u32 tex = load<u32>(page + 20);
            const f64 inv_w = fdivs(kOne, to_f32(load<u16>(tex + kTexWidth)));
            const f64 inv_h = fdivs(kOne, to_f32(load<u16>(tex + kTexHeight)));
            // Both calls keep r7 as the original leaves it: Draw_BeginPrims
            // ors its fifth argument (r7, never set here) into its command.
            call(ctx, Draw_SetTexture, {fb, page});
            call(ctx, Draw_BeginPrims, {prims, fb, 6, 4});

            const f64 x = lfs(q + 0), y = lfs(q + 4), w = lfs(q + 8), h = lfs(q + 12);
            u32 vert = load<u32>(prims);
            u64 index = load<u16>(prims + 6);
            for (u32 v = 0; v < 4; ++v) {
                const u64 bits = load<u64>(q + 16);
                const u32 col = ((bits >> 19) & 1) != 0 ? col_b : col_a;
                for (u32 b = 0; b < 4; ++b) store<u8>(vert + 12 + b, load<u8>(col + b));
                const f64 cx = lfs(kCorners + v * 8), cy = lfs(kCorners + v * 8 + 4);
                store<u32>(vert + 8, 0);
                ++index;
                stfs(vert + 0, fmadds(w, cx, x));
                stfs(vert + 4, fmadds(h, cy, y));
                const u64 b2 = load<u64>(q + 16);
                // The original converts each field through the u64 -> double idiom; they are
                // 11-bit (4-bit) values, so the result is the integer itself.
                const f64 tex_u = static_cast<f64>((b2 >> 53) & 0x7FF);
                const f64 cell = static_cast<f64>((b2 >> 11) & 0xF);
                const f64 gw = static_cast<f64>((b2 >> 31) & 0x7FF);
                const f64 tex_v = static_cast<f64>((b2 >> 42) & 0x7FF);
                const f64 gh = static_cast<f64>((b2 >> 20) & 0x7FF);
                const f64 u = fmadds(gw, fadds(cell, cx), tex_u);
                stfs(vert + 16, fmuls(u, inv_w));
                stfs(vert + 20, fmuls(fmadds(gh, cy, tex_v), inv_h));
                vert += kVertexSize;
                store<u32>(prims, vert);
                store<u16>(prims + 6, static_cast<u16>(index));
            }
            ctx.gpr[7] = index;  // r7 is the index register when the next quad's calls run
            call(ctx, Draw_EndPrims, {fb});
        }
        stfs(e + kTextGlyphCount, to_f32(count));
        stfs(e + kTextLineCount, to_f32(lines));
    }

    if ((field<u32>(e, kElemFlags) & kElemActive) == 0) return;
    for (u32 c = field<u32>(e, kElemFirstChild); c != 0; c = field<u32>(c, kElemNext))
        call_ptr(ctx, vfunc(c, kVfnDraw), {c});
}
