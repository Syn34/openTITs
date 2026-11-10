// The static geometry batch draw (gfx24): GeomBatches_Draw (Fn_576BB4's
// body).
#include <bit>
#include <cmath>

#include "fx.h"
#include "pool.h"
#include "gfx24.h"

using namespace sly4;

namespace {

namespace bd = sly4::render::gfx24::batchdraw;
namespace sp = sly4::render::pool;
using fx::emit;

inline f32 F(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }
inline void P(u32 a, f32 v) { store<u32>(a, std::bit_cast<u32>(v)); }
inline f32 Mul(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * b); }
inline f32 Sub(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - b); }
inline f32 Madd(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), c, b)); }  // fmadds
inline f32 Nmsub(f32 a, f32 c, f32 b) { return static_cast<f32>(-std::fma(static_cast<f64>(a), c, -static_cast<f64>(b))); }
inline f32 Sqrt(f32 x) { return static_cast<f32>(std::sqrt(static_cast<f64>(x))); }
inline u32 Dup16(u32 v) { return (v & 0xFFFF) | (v << 16); }  // rlwinm + rlwimi: the low half in both

// The frame, at the original's sp offsets.
constexpr u32 kFrame = 768;
constexpr u32 kTint = 112;       // f32[4] {1, 1, 1, fade}: vertex constant 4
constexpr u32 kPos = 128;        // f32[4] {position, 1}: vertex constant 5
constexpr u32 kWorld = 144;      // f32[16] instance x view matrix, transposed: vertex constants 0..3
constexpr u32 kMatrix = 208;     // view +880
constexpr u32 kPlanes = 272;     // view +272 (6 planes, then the 8 planes SoA at +96)
constexpr u32 kCamera = 496;     // view +816 rows (xyz, stride 16)

void CopyWords(u32 dst, u32 src, u32 n) {  // lfs / stfs pairs
    for (u32 k = 0; k < n; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
}

// The instance sphere (unaligned loads) inside the 8 planes SoA at P + 96:
// every lane of r + distance >= 0, for both plane groups.
bool SphereInside(u32 P, u32 s) {
    const f32 x = F(s), y = F(s + 4), z = F(s + 8), r = F(s + 12);
    for (u32 g = 0; g < 2; ++g) {
        const u32 p = P + 96 + 64 * g;
        for (u32 l = 0; l < 4; ++l) {
            f32 d = std::fma(F(p + 4 * l), x, F(p + 48 + 4 * l));
            d = std::fma(F(p + 16 + 4 * l), y, d);
            d = std::fma(F(p + 32 + 4 * l), z, d);
            if (!(r + d >= 0.0f)) return false;  // vaddfp, vcmpgefp.
        }
    }
    return true;
}

}  // namespace

// void GeomBatches_Draw()   [Fn_576BB4] (name?)
// For every batch: its blending (the batch's factors and equation, or
// alpha blending decided per instance by the fade when the batch isn't
// opaque), alpha test (GEQUAL +104), depth test / func / write, culling
// (+84: 0 off, 2304 -> front face 2305, else 2304), texture unit 0 (+176,
// filters, wrap), programs (+200 / +204) and its colour constants. Then
// each instance closer than the batch radius whose sphere is inside the
// view's 8 planes: fade = (far - distance) * scale (1 from 1 up), or the
// near fade below +12, skipped when it reaches 1/255; LOD = count - 1
// past +224 (squared), else 0. The instance matrix times the view's
// (transposed, constants 0..3), the fade (4), the position (5) and the
// batch's 6..8 go to the vertex program; when the LOD changes the vertex
// format and data (bind_format) and the index buffer; then the fragment
// program upload and the indexed draw. Ends with alpha testing off and
// Draw_InvalidateState.
DECOMP(00575D18, GeomBatches_Draw) {
    const u32 st = fx::g::state;
    const u32 view = static_cast<u32>(call(ctx, bd::View_GetCurrent));
    Stack frame(ctx, kFrame);
    const u32 f = frame.addr();
    CopyWords(f + kMatrix, view + bd::kViewMatrix, 16);
    CopyWords(f + kPlanes, view + bd::kViewPlanes, 24);
    CopyWords(f + kPlanes + 96, view + 368, 32);
    for (u32 r = 0; r < 4; ++r) CopyWords(f + kCamera + 16 * r, view + bd::kViewCamera + 16 * r, 3);
    const u32 C = bd::kConsts;
    for (s32 i = 0; i < static_cast<s32>(load<u32>(bd::kBatchCount)); ++i) {
        const u32 E = bd::kBatches + bd::kBatchSize * static_cast<u32>(i);
        const bool opaque = load<u8>(E + bd::kOpaque) == 0;  // r14: blending decided per instance
        if (!opaque) {
            emit({fx::kBlendEnable, 1});
            emit({fx::kBlendFunc, Dup16(load<u32>(E + bd::kSrcFactor)), Dup16(load<u32>(E + bd::kDstFactor))});
            emit({fx::kBlendEquation, Dup16(load<u32>(E + bd::kBlendEq))});
        } else {
            emit({fx::kBlendEnable, 1});
            emit({fx::kBlendFunc, fx::kSrcAlpha2, fx::kOneMinusSrcAlpha2});
            emit({fx::kBlendEquation, fx::kFuncAdd2});
            emit({fx::kBlendEnable, 0});
        }
        const u32 aref = load<u32>(E + bd::kAlphaRef);
        if (aref != 0) emit({bd::kAlphaFunc, bd::kGequal, aref});
        emit({bd::kAlphaTestEnable, aref != 0 ? 1u : 0u});
        emit({fx::kDepthTestEnable, load<u8>(st + 93)});
        emit({fx::kDepthFunc, load<u32>(E + bd::kDepthFunc)});
        emit({fx::kDepthMask, (load<u8>(E + bd::kDepthWrite) & load<u8>(st + 93)) != 0 ? 1u : 0u});
        const u32 cull = load<u32>(E + bd::kCull);
        if (cull != 0) {
            emit({fx::kCullFaceEnable, 1});
            emit({bd::kFrontFace, cull == 2304 ? 2305u : 2304u});
        } else {
            emit({fx::kCullFaceEnable, 0});
        }
        const u32 tex = load<u32>(E + bd::kTexture);
        if (load<u32>(st + 52) != tex) {
            store<u32>(st + 52, tex);
            call(ctx, draw::fn::cellGcmSetTexture, {load<u32>(st), 0, tex + 16});
            const u32 levels = load<u8>(tex + 17);
            call(ctx, draw::fn::cellGcmSetTextureControl, {load<u32>(st), 0, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
        }
        const u32 minf = load<u32>(E + bd::kMinFilter), magf = load<u32>(E + bd::kMagFilter);
        call(ctx, draw::fn::cellGcmSetTextureFilter,
             {load<u32>(st), 0, 0, load<u8>(st + 225 + 3 * magf + minf), load<u8>(st + 222 + minf), 1});
        emit({bd::kTexAddress0,
              0x00030000u | ((load<u32>(E + bd::kWrapT) & 0xFF) << 8) | (load<u32>(E + bd::kWrapS) & 0xFF)});
        fx::bind_vertex(ctx, load<u32>(E + bd::kVProgram));
        fx::bind_fragment(load<u32>(E + bd::kFProgram));
        const u32 ca = load<u32>(E + bd::kColourA), cb = load<u32>(E + bd::kColourB);
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants,
             {load<u32>(st), 32, 4, bd::kColourTable + ((ca << 4) & ~15u) + ((cb << 5) & ~31u)});
        call(ctx, bd::FProgram_SetConstant, {32, bd::kFConstTable + ((cb << 4) & ~15u)});
        const u32 first = load<u32>(E + bd::kInsts);
        const u32 end = first + load<u32>(E + bd::kInstCount) * bd::kInstSize;
        const f32 cx = F(f + kCamera + 48), cy = F(f + kCamera + 52), cz = F(f + kCamera + 56);
        u32 last_lod = 0xFFFFFFFFu;
        u32 it = first;
        do {  // at least one instance (a count of 0 runs on, as the original)
            const u32 pos = it + 48;
            const f32 dx = Sub(cx, F(pos)), dy = Sub(cy, F(pos + 4)), dz = Sub(cz, F(pos + 8));
            f32 d2 = Mul(dy, dy);
            d2 = Madd(dx, dx, d2);
            d2 = Madd(dz, dz, d2);
            if (!(d2 < F(E + bd::kRadiusSq)) || !SphereInside(f + kPlanes, it + 64)) {
                it += bd::kInstSize;
                continue;
            }
            const f32 dist = Sqrt(d2);
            const f32 scale = F(E + bd::kFadeScale), one = F(C), zero = F(C + 4);
            const f32 t = Mul(Sub(F(E + bd::kFadeFar), dist), scale);
            f32 fade = Sub(t, one) >= 0.0f ? static_cast<f32>(std::bit_cast<f64>(load<u64>(C + 8))) : t;
            const f32 nearf = F(E + bd::kFadeNear);
            if (nearf > zero && dist < nearf) {
                const f32 u = Nmsub(Sub(nearf, dist), scale, one);  // 1 - (near - distance) * scale
                f32 m = one;
                if (u < m) m = u;
                f32 k = zero;
                if (m > k) {
                    k = one;
                    if (u < k) k = u;
                }
                const f32 c255 = F(C + 16);
                if (k < c255 || k == c255) {
                    it += bd::kInstSize;
                    continue;
                }
                fade = k;
            }
            const u32 lod = d2 > F(E + bd::kLodDistSq) ? load<u32>(E + bd::kLodCount) - 1 : 0;
            if (opaque)
                emit({fx::kBlendEnable, static_cast<u32>(rt::fctiw(fade, true)) == 0 ? 1u : 0u});
            // The instance rows (lvx: 16-byte aligned) times the view matrix,
            // vmaddfp lane by lane, row 3 adding the matrix's row 3.
            u32 w[4][4];
            for (u32 r = 0; r < 4; ++r) {
                const u32 row = (it + 16 * r) & ~15u;
                const f32 x = F(row), y = F(row + 4), z = F(row + 8);
                for (u32 k = 0; k < 4; ++k) {
                    const u32 m = f + kMatrix + 4 * k;
                    f32 a = std::fma(F(m), x, r == 3 ? F(m + 48) : 0.0f);
                    a = std::fma(F(m + 16), y, a);
                    w[r][k] = std::bit_cast<u32>(std::fma(F(m + 32), z, a));
                }
            }
            for (u32 r = 0; r < 4; ++r)
                for (u32 k = 0; k < 4; ++k) store<u32>(f + kWorld + 16 * k + 4 * r, w[r][k]);  // transposed
            call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 0, 16, f + kWorld});
            P(f + kTint + 12, fade);
            for (u32 k = 0; k < 3; ++k) store<u32>(f + kTint + 4 * k, 0x3F800000u);
            call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 4, 4, f + kTint});
            CopyWords(f + kPos, pos, 3);
            store<u32>(f + kPos + 12, 0x3F800000u);
            call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 5, 4, f + kPos});
            store<u32>(E + bd::kVConst6W, load<u32>(bd::kTime));
            call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 6, 4, E + bd::kVConst6});
            call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 7, 4, E + bd::kVConst7});
            call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 8, 4, E + bd::kVConst8});
            if (lod != last_lod) {
                last_lod = lod;
                const u32 buf = load<u32>(E + bd::kLodBuffers + 4 * lod);
                sp::bind_format(load<u32>(E + bd::kVertexFormat), load<u32>(buf + 4));
                store<u32>(st + 4, load<u32>(E + bd::kLodIndexBuffers + 4 * lod));
            }
            const u32 count = load<u32>(E + bd::kLodIndexCounts + 4 * lod);
            const u32 mode = load<u32>(E + bd::kMode) & 0xFF;
            sp::upload_fragment(ctx);
            const u32 ib = load<u32>(st + 4);
            const u32 where = load<u32>(ib + 4);
            call(ctx, sp::fn::cellGcmSetDrawIndexArray,
                 {load<u32>(st), mode, count, load<u32>(ib + 16) & 0xFF, where & 1, where >> 1});
            it += bd::kInstSize;
        } while (it != end);
    }
    emit({bd::kAlphaTestEnable, 0});
    ctx.gpr[3] = call(ctx, bd::Draw_InvalidateState);
}
