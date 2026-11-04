// The shaders' small helpers: the Shader base constructor and program
// loading, the fragment program's texture input bits, the UV animation
// sources and the 3x3 transform the lit shaders use for directions.
#include "shader4.h"

#include "prog_load.h"

using namespace sly4;

namespace {

namespace s3 = sly4::render::shader3;
namespace s4 = sly4::render::shader4;
namespace sp = sly4::render::pool;

constexpr u32 kShaderVtable = 0x00B233A0;
constexpr u32 kAnimZero = 0x0050BF44;  // f32 0.0 in .text

// fctidz / fcfid / frsp: x truncated toward zero through a 64-bit integer.
f64 trunc_s(f64 x) { return sp::sgl(rt::fcfid(rt::fctid(x, true))); }

// fmuls / fctidz / fcfid / frsp / fdivs: offset snapped to 1 / steps.
f64 snap(f64 offset, f64 steps) { return sp::sgl(trunc_s(sp::sgl(offset * steps)) / steps); }

}  // namespace

// ---------------------------------------------------------------- Shader

// void Shader_Construct(Shader* s)
// The base Shader vtable.
DECOMP(0050AFDC, Shader_Construct) {
    store<u32>(arg<u32>(ctx, 0), kShaderVtable);
}

// void Shader_Load(Shader* s, const char* name, const char* fname, const void* bindings)
// s +12 = name (32 bytes); s +4 = the vertex program `name` and s +8 the
// fragment program `fname` (`name` when null), each found in or loaded
// into the program maps (post::g::vmap / fmap), the fragment program
// bound with `bindings` when it is loaded. Returns the fragment program
// when it was cached, else FProgram_Setup's result.
DECOMP(0053A9D8, Shader_Load) {
    const u32 s = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), fname = arg<u32>(ctx, 2), bindings = arg<u32>(ctx, 3);
    call(ctx, sly4::fn::String_Copy, {s + s4::kShaderName, name, 32});
    progload::load_vertex(ctx, s + 4, name);
    ret(ctx, progload::load_fragment(ctx, s + 8, fname != 0 ? fname : name, bindings));
}

// void Shader_SetFpControl(Shader* s, u32 bits)
// The fragment program's attribute input mask becomes the vertex
// program's output mask with bits 0xFC0 (the texture coordinate inputs)
// taken from `bits`; then the program is re-sent
// (Gcm_SetFragmentProgramLoad into its +1136).
DECOMP(0053ABB8, Shader_SetFpControl) {
    const u32 s = arg<u32>(ctx, 0), bits = arg<u32>(ctx, 1);
    const u32 out = static_cast<u32>(call(ctx, s4::fn::Cg_GetIOMask, {load<u32>(load<u32>(s + 4) + 4)}));
    const u32 merged = (out & ~s4::kTexInputBits) | (bits & s4::kTexInputBits);  // rlwimi 0,20,25
    call(ctx, s4::fn::Cg_SetIOMask, {load<u32>(load<u32>(s + 8) + 4), merged});
    const u32 fp = load<u32>(s + 8);
    call(ctx, post::fn::Gcm_SetFragmentProgramLoad, {load<u32>(fp + 4), fp + s4::kFpLoadOffset, 0, 1, 0});
}

// ---------------------------------------------------------------- UV animation

// const f32* MaterialAnim_GetUV(MaterialAnim* anim, f32 su, f32 sv)
// The animated UV matrix m (MaterialAnim_GetMatrix) as two vertex rows in
// g::uv_rows: {m[0] * su, m[1] * su, m[8], m[9]}, {m[4] * sv, m[5] * sv, 0, 0}.
DECOMP(0050BE5C, MaterialAnim_GetUV) {
    const f64 su = farg(ctx, 0), sv = farg(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, s4::fn::MaterialAnim_GetMatrix, {arg<u32>(ctx, 0)}));
    const u32 out = s4::g::uv_rows;
    sp::stfs(out, sp::sgl(sp::lfs(m) * su));
    sp::stfs(out + 4, sp::sgl(sp::lfs(m + 4) * su));
    sp::copy_word(out + 8, m + 32);
    sp::copy_word(out + 12, m + 36);
    sp::stfs(out + 16, sp::sgl(sp::lfs(m + 16) * sv));
    const f64 last = sp::sgl(sp::lfs(m + 20) * sv);
    sp::stfs(out + 20, last);
    ctx.fpr[1] = last;  // f1 as the original leaves it
    store<u32>(out + 24, 0);
    store<u32>(out + 28, 0);
    ret(ctx, out);
}

// void MaterialAnim_GetUVAlt(MaterialAnim* anim, f32 out[2])
// The U / V offsets (+44 / +92), each snapped to 1 / steps (+12 / +60)
// unless its steps are 0. With the wrap flag (+4), U becomes
// frac(V offset * V steps) snapped to 1 / U steps instead (the original
// multiplies the unsnapped V offset by V steps and divides by U steps
// even when U steps is 0).
DECOMP(0050BF48, MaterialAnim_GetUVAlt) {
    const u32 anim = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const f64 zero = sp::lfs(kAnimZero);
    const f64 us = sp::lfs(anim + 12);
    const bool wrap = ref<u8>(anim + 4) != 0;
    const f64 u = us != zero ? snap(sp::lfs(anim + 44), us) : sp::lfs(anim + 44);  // fcmpu
    const f64 vs = sp::lfs(anim + 60);
    const f64 voff = sp::lfs(anim + 92);
    const f64 v = vs != zero ? snap(voff, vs) : voff;
    // r3 / f1 as the original leaves them: anim + 56 and the last value in f1.
    ctx.gpr[3] = anim + 56;
    if (!wrap) {
        sp::stfs(out, u);
        sp::stfs(out + 4, v);
        ctx.fpr[1] = vs;
        return;
    }
    const f64 p = sp::sgl(voff * vs);  // fmuls
    const f64 frac = sp::sgl(p - trunc_s(p));  // fsubs
    const f64 t = rt::fcfid(rt::fctid(sp::sgl(frac * us), true));  // fctidz / fcfid, before frsp
    sp::stfs(out, sp::sgl(sp::sgl(t) / us));
    sp::stfs(out + 4, v);
    ctx.fpr[1] = t;
}

// ---------------------------------------------------------------- vectors

// void Vec3_ToItemSpace(f32 v[3], const f32 m[3][4])
// v = (m rows) . v, in place: out[i] = m[i][0] v0 + m[i][1] v1 + m[i][2] v2
// (fmuls then two fmadds per row; rows 16 bytes apart).
DECOMP(00553468, Vec3_ToItemSpace) {
    const u32 v = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const f64 y = sp::lfs(v + 4), x = sp::lfs(v), z = sp::lfs(v + 8);
    for (u32 i = 0; i < 3; ++i) {
        const u32 row = m + 16 * i;
        f64 r = sp::sgl(y * sp::lfs(row + 4));
        r = sp::fmadds(x, sp::lfs(row), r);
        r = sp::fmadds(z, sp::lfs(row + 8), r);
        sp::stfs(v + 4 * i, r);
        ctx.fpr[1] = r;  // f1 ends as the last component, as in the original
    }
}

// void Vec3_RotateTo(f32 out[3], const f32 v[3], const f32 m[3][4])
// Vec3_ToItemSpace into a separate vector; v is re-read for each row.
DECOMP(005533EC, Vec3_RotateTo) {
    const u32 out = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    for (u32 i = 0; i < 3; ++i) {
        const u32 row = m + 16 * i;
        f64 r = sp::sgl(sp::lfs(v + 4) * sp::lfs(row + 4));
        r = sp::fmadds(sp::lfs(v), sp::lfs(row), r);
        r = sp::fmadds(sp::lfs(v + 8), sp::lfs(row + 8), r);
        sp::stfs(out + 4 * i, r);
        ctx.fpr[1] = r;  // f1 ends as the last component, as in the original
    }
}
