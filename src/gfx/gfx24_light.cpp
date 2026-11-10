// The light volume draws (gfx24): LightVolume_DrawStencil and
// LightVolume_DrawOutside, LightVolumes_Draw's two passes.
#include <bit>
#include <cmath>

#include "fx.h"
#include "pool.h"
#include "gfx24.h"

using namespace sly4;

namespace {

namespace lv = sly4::render::gfx24::lightvol;
namespace sp = sly4::render::pool;
using fx::emit;

inline f32 F(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }
inline void P(u32 a, f32 v) { store<u32>(a, std::bit_cast<u32>(v)); }
inline f32 Mul(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * b); }
inline f32 Sub(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - b); }
inline f32 Div(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) / b); }
inline f32 FromInt(s32 v) { return static_cast<f32>(static_cast<f64>(v)); }  // fcfid + frsp

// The frame both functions use, at the original's sp offsets.
constexpr u32 kFrame = 896;
constexpr u32 kVec = 112;       // f32[4] {0, 0, -depth} for Vec_TransformByMatrix
constexpr u32 kColour = 128;    // f32[4] vertex constant 4
constexpr u32 kNearClip = 144, kFarClip = 160;  // f32[4] each
constexpr u32 kRange = 176;     // f32[4] fragment constant 4: the clamped depths, 0, 0
constexpr u32 kInv = 192;       // Mat4_Invert's out
constexpr u32 kAffine = 256;    // kBasis times Mat4_Invert's tmp (+320)
constexpr u32 kInvTmp = 320;
constexpr u32 kLightVP = 384;   // viewproj * kLightBias
constexpr u32 kTexMatrix = 448; // kAffine * kLightVP, transposed: fragment constants 0..3
constexpr u32 kVerts = 512;     // Matrix_PackShaderRows' 14 vertices

void BindPrograms(PPUContext& ctx, u32 pair) {  // {+4 VProgram*, +8 FProgram*}
    fx::bind_vertex(ctx, pair + 4);
    fx::bind_fragment(pair + 8);
}

void FrontFace(u32 face) {
    emit({fx::kCullFaceEnable, 1});
    emit({lv::kFrontFace, face});
}

// The cached stencil state (render state +160 func, +164 ref, +168 write
// mask, +172 mask, +176..184 ops) set and sent.
void Stencil(u32 fail, u32 zfail, u32 zpass, u32 func, u32 ref, u32 mask, u32 wmask) {
    const u32 st = fx::g::state;
    store<u32>(st + 176, fail);
    store<u32>(st + 180, zfail);
    store<u32>(st + 184, zpass);
    store<u32>(st + 160, func);
    store<u32>(st + 164, ref);
    store<u32>(st + 172, mask);
    store<u32>(st + 168, wmask);
    emit({lv::kStencilOp, fail, zfail, zpass});
    emit({lv::kStencilMask, wmask});
    emit({lv::kStencilFunc, load<u32>(st + 160), load<u32>(st + 164), load<u32>(st + 172)});
}

// Draw_InlineArray(6, 14, data, kVertexFormat) as LightVolume_DrawOutside
// inlines it.
void DrawBoxInline(PPUContext& ctx, u32 data) {
    const u32 st = fx::g::state, vf = lv::kVertexFormat;
    sp::upload_fragment(ctx);
    if (load<u32>(st + 8) != 0) {
        const u32 stride = load<u16>(vf + 120);
        store<u32>(st + 8, 0);
        sp::send_attributes(vf, stride, load<u16>(vf + 126), 0, 0x80000000u);
    }
    const u32 words = (14 * static_cast<u32>(load<u16>(vf + 120))) >> 2;
    ctx.gpr[3] = call(ctx, draw::fn::cellGcmSetDrawInlineArray, {load<u32>(st), 6, words, data});
}

void DrawBox(PPUContext& ctx, u32 data, bool inline_draw) {
    if (inline_draw) DrawBoxInline(ctx, data);
    else ctx.gpr[3] = call(ctx, lv::Draw_InlineArray, {6, 14, data, lv::kVertexFormat});
}

// Common start: the inverse view (Mat4_Invert of view +880, whose tmp at
// +320 is the one used) times kBasis, the light's viewproj times
// kLightBias, the box vertices, shader group 2's first program pair and
// the view's 16 vertex constants (view +944). Returns view +944.
u32 Setup(PPUContext& ctx, u32 f, u32 viewproj, u32 corners, bool inline_affine) {
    const u32 view = static_cast<u32>(call(ctx, lv::View_GetCurrent));
    const u32 vconst = view + 944;
    call(ctx, lv::Mat4_Invert, {f + kInv, f + kInvTmp, view + 880});
    if (inline_affine) {
        // Matrix_MulAffine inlined (vmaddfp lane by lane): row r = T0 * B[r].x
        // (+ T3 for row 3) + T1 * B[r].y + T2 * B[r].z.
        const u32 T = f + kInvTmp;
        for (u32 r = 0; r < 4; ++r) {
            const u32 b = lv::kBasis + 16 * r;
            const f32 x = F(b), y = F(b + 4), z = F(b + 8);
            for (u32 k = 0; k < 4; ++k) {
                f32 a = std::fma(F(T + 4 * k), x, r == 3 ? F(T + 48 + 4 * k) : 0.0f);
                a = std::fma(F(T + 16 + 4 * k), y, a);
                P(f + kAffine + 16 * r + 4 * k, std::fma(F(T + 32 + 4 * k), z, a));
            }
        }
    } else {
        call(ctx, lv::Matrix_MulAffine, {f + kAffine, lv::kBasis, f + kInvTmp});
    }
    call(ctx, lv::Matrix_MultiplyTo, {f + kLightVP, viewproj, lv::kLightBias});
    call(ctx, lv::Matrix_PackShaderRows, {corners, f + kVerts});
    const u32 group = static_cast<u32>(call(ctx, lv::Shader_LoadGroup, {2}));
    BindPrograms(ctx, load<u32>(load<u32>(load<u32>(group + 16) + 12)));
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 0, 16, vconst});
    return vconst;
}

f32 ClampDepth(f32 z, u32 C) {  // fsel pair: z > 1 (or NaN) -> 1, then z < 0.001 -> 0.001
    const f32 one = static_cast<f32>(std::bit_cast<f64>(load<u64>(C + lv::kCOneD)));
    const f32 lo = static_cast<f32>(std::bit_cast<f64>(load<u64>(C + lv::kCMinD)));
    if (!(Sub(F(C + lv::kCOne), z) >= 0.0f)) z = one;
    if (!(Sub(z, F(C + lv::kCMin)) >= 0.0f)) z = lo;
    return z;
}

// The lit pass both end with: colour (bytes / 255, alpha * a) as vertex
// constant 4, the texture matrix (kAffine * kLightVP, transposed) as
// fragment constants 0..3, the depth range (the light's near / far through
// proj, z / w clamped to [0.001, 1]) as constant 4, the program pair (the
// second when both depths are ~0), the display depth texture on unit 0
// and the light's texture (+96 -> +56) on unit 1, alpha blending, depth
// test ALWAYS, and the box drawn.
void LitPass(PPUContext& ctx, u32 f, u32 L, f64 a, u32 proj, u32 vconst, u32 C, bool inline_draw) {
    const u32 st = fx::g::state;
    const u32 c = L + lv::kLtColour;
    const f32 k255 = F(C + lv::kC255th);
    const f32 r = FromInt(load<u8>(c)), g = FromInt(load<u8>(c + 1)), b = FromInt(load<u8>(c + 2)),
              al = FromInt(load<u8>(c + 3));
    P(f + kColour, Mul(r, k255));
    P(f + kColour + 4, Mul(g, k255));
    P(f + kColour + 8, Mul(b, k255));
    P(f + kColour + 12, static_cast<f32>(static_cast<f64>(Mul(al, k255)) * a));
    call(ctx, lv::Matrix_MultiplyTo, {f + kTexMatrix, f + kAffine, f + kLightVP});
    u32 m[16];
    for (u32 k = 0; k < 16; ++k) m[k] = load<u32>(f + kTexMatrix + 4 * k);
    for (u32 i = 0; i < 4; ++i)
        for (u32 j = 0; j < 4; ++j) store<u32>(f + kTexMatrix + 16 * i + 4 * j, m[4 * j + i]);  // vmrghw / vmrglw
    const u32 nearb = load<u32>(L + lv::kLtNear), farb = load<u32>(L + lv::kLtFar);
    const f32 eps = F(C + lv::kCEps);
    const bool flat = std::fabs(std::bit_cast<f32>(nearb)) < eps && std::fabs(std::bit_cast<f32>(farb)) < eps;
    store<u32>(f + kVec, 0);
    store<u32>(f + kVec + 4, 0);
    store<u32>(f + kVec + 8, nearb ^ 0x80000000u);  // fneg
    call(ctx, lv::Vec_TransformByMatrix, {f + kNearClip, f + kVec, proj});
    store<u32>(f + kVec, 0);
    store<u32>(f + kVec + 4, 0);
    store<u32>(f + kVec + 8, farb ^ 0x80000000u);
    call(ctx, lv::Vec_TransformByMatrix, {f + kFarClip, f + kVec, proj});
    const f32 z0 = Div(F(f + kNearClip + 8), F(f + kNearClip + 12));
    const f32 z1 = Div(F(f + kFarClip + 8), F(f + kFarClip + 12));
    store<u32>(f + kRange + 8, 0);
    store<u32>(f + kRange + 12, 0);
    P(f + kRange, ClampDepth(z0, C));
    P(f + kRange + 4, ClampDepth(z1, C));
    BindPrograms(ctx, load<u32>(load<u32>(load<u32>(lv::kLightShaders) + 12) + (flat ? 4u : 0u)));
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 0, 16, vconst});
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 4, 4, f + kColour});
    for (u32 k = 0; k < 4; ++k) call(ctx, lv::FProgram_SetConstant, {k, f + kTexMatrix + 16 * k});
    call(ctx, lv::FProgram_SetConstant, {4, f + kRange});
    const u32 depth = static_cast<u32>(call(ctx, lv::Display_GetDepthTexture, {lv::kDisplay}));
    const u32 tex = load<u32>(L + lv::kLtTexture) + 56;
    fx::bind_texture(ctx, 0, depth, lv::kTexAddr0, 0);
    if (load<u32>(st + 56) != tex) {  // unit 1: the address goes after the filter
        store<u32>(st + 56, tex);
        call(ctx, draw::fn::cellGcmSetTexture, {load<u32>(st), 1, tex + 16});
        const u32 levels = load<u8>(tex + 17);
        call(ctx, draw::fn::cellGcmSetTextureControl, {load<u32>(st), 1, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
    }
    call(ctx, draw::fn::cellGcmSetTextureFilter, {load<u32>(st), 1, 0, load<u8>(st + 229), load<u8>(st + 223), 1});
    emit({lv::kTexAddress1, lv::kTexAddr1});
    emit({fx::kBlendEnable, 1});
    emit({fx::kBlendFunc, fx::kSrcAlpha2, fx::kOneMinusSrcAlpha2});
    emit({fx::kBlendEquation, fx::kFuncAdd2});
    emit({fx::kDepthMask, 0});
    emit({fx::kDepthTestEnable, load<u8>(st + 93)});
    emit({fx::kDepthFunc, lv::kAlways});
    FrontFace(lv::kCCW);
    DrawBox(ctx, f + kVerts, inline_draw);
}

// The state before the lit pass: depth test as before, EQUAL, colour
// writes and blending on, stencil test passing where the stencil is 4.
void LitState() {
    const u32 st = fx::g::state;
    emit({fx::kDepthTestEnable, load<u8>(st + 93)});
    emit({fx::kDepthFunc, lv::kEqual});
    FrontFace(lv::kCW);
    emit({fx::kColorMask, 0x01010101});
    emit({fx::kBlendEnable, 1});
    Stencil(lv::kKeep, lv::kKeep, lv::kKeep, lv::kEqual, 4, 255, 0);
}

}  // namespace

// void LightVolume_DrawStencil(Light* light, LightMats* mats, const f32
// viewproj[16], const f32 proj[16], const f32 corners[8][4]; f1 alpha)
// (name?)   [LightVolumes_Draw, for volumes not wholly in front of the
// view's plane]
// Colour and depth writes off, depth LESS: the box drawn three times into
// the stencil (write mask 247; NOTEQUAL ref 8 / 12 / 8, replacing on depth
// pass / fail by face winding), then the lit pass where the stencil is 4.
DECOMP(00573600, LightVolume_DrawStencil) {
    const u32 L = arg<u32>(ctx, 0), viewproj = arg<u32>(ctx, 2), proj = arg<u32>(ctx, 3), corners = arg<u32>(ctx, 4);
    const f64 a = farg(ctx, 0);
    Stack frame(ctx, kFrame);
    const u32 f = frame.addr(), st = fx::g::state;
    const u32 vconst = Setup(ctx, f, viewproj, corners, true);
    emit({fx::kDepthTestEnable, load<u8>(st + 93)});
    emit({fx::kDepthFunc, lv::kLess});
    emit({fx::kBlendEnable, 0});
    emit({fx::kColorMask, 0});
    FrontFace(lv::kCCW);
    Stencil(lv::kKeep, lv::kReplace, lv::kReplace, lv::kNotEqual, 8, 8, 247);
    DrawBox(ctx, f + kVerts, false);
    FrontFace(lv::kCCW);
    Stencil(lv::kKeep, lv::kReplace, lv::kKeep, lv::kNotEqual, 12, 8, 247);
    DrawBox(ctx, f + kVerts, false);
    FrontFace(lv::kCW);
    Stencil(lv::kKeep, lv::kReplace, lv::kKeep, lv::kNotEqual, 8, 8, 247);
    DrawBox(ctx, f + kVerts, false);
    LitState();
    LitPass(ctx, f, L, a, proj, vconst, lv::kConstsStencil, false);
}

// void LightVolume_DrawOutside(Light* light, -, const f32 viewproj[16],
// const f32 proj[16], const f32 corners[8][4]; f1 alpha)   (name?)
// [LightVolumes_Draw, for volumes wholly in front of the view's plane]
// As LightVolume_DrawStencil with two stencil passes (replacing on depth
// pass; NOTEQUAL ref 12 then 8, by face winding), Matrix_MulAffine called
// rather than inlined and Draw_InlineArray inlined.
DECOMP(00574370, LightVolume_DrawOutside) {
    const u32 L = arg<u32>(ctx, 0), viewproj = arg<u32>(ctx, 2), proj = arg<u32>(ctx, 3), corners = arg<u32>(ctx, 4);
    const f64 a = farg(ctx, 0);
    Stack frame(ctx, kFrame);
    const u32 f = frame.addr(), st = fx::g::state;
    const u32 vconst = Setup(ctx, f, viewproj, corners, false);
    emit({fx::kDepthTestEnable, load<u8>(st + 93)});
    emit({fx::kDepthFunc, lv::kLess});
    emit({fx::kBlendEnable, 0});
    emit({fx::kColorMask, 0});
    FrontFace(lv::kCW);
    Stencil(lv::kKeep, lv::kKeep, lv::kReplace, lv::kNotEqual, 12, 8, 247);
    DrawBox(ctx, f + kVerts, true);
    FrontFace(lv::kCCW);
    Stencil(lv::kKeep, lv::kKeep, lv::kReplace, lv::kNotEqual, 8, 8, 247);
    DrawBox(ctx, f + kVerts, true);
    LitState();
    LitPass(ctx, f, L, a, proj, vconst, lv::kConstsOutside, true);
}
