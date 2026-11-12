// The shaders' small helpers: the Shader base constructor and program
// loading, the fragment program's texture input bits, the UV animation
// sources and the 3x3 transform the lit shaders use for directions.
//
// MaterialAnim (material +128), as MaterialAnim_GetUVAlt reads it: +4 u8
// wrap flag, +12 f32 U steps (0: continuous), +44 f32 U offset, +60 f32 V
// steps, +92 f32 V offset.
#include "shader4.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace s4 = sly4::render::shader4;

constexpr u32 kShaderVtable = 0x00B233A0;
constexpr u32 Gcm_SetFragmentProgramLoad = 0x005DF980;  // (CgProgram*, out, 0, 1, 0) (name?)

// fctidz / fcfid / frsp: x truncated toward zero through a 64-bit integer.
f32 trunc_s(f32 x) { return static_cast<f32>(static_cast<f64>(fctidz(x))); }

// fmuls / fctidz / fcfid / frsp / fdivs: offset snapped to 1 / steps.
f32 snap(f32 offset, f32 steps) { return trunc_s(offset * steps) / steps; }

}  // namespace

// ---------------------------------------------------------------- Shader

// void Shader_Construct(Shader* s)
// The base Shader vtable.
void Shader_Construct(ShaderPrograms* s) { s->vtable = kShaderVtable; }

// void Shader_Load(Shader* s, const char* name, const char* fname, const void* bindings)
// s +12 = name (32 bytes); s +4 = the vertex program `name` and s +8 the
// fragment program `fname` (`name` when null), each found in or loaded
// into the program maps, the fragment program bound with `bindings`
// when it is loaded (VProgram_Load / FProgram_Load inlined).
void Shader_Load(ShaderPrograms* s, const char* name, const char* fname, const Binding* bindings) {
    call(sly4::fn::String_Copy, at<char>(s, s4::kShaderName), name, 32u);
    VProgram_Load(&s->vprogram, name, 0);
    FProgram_Load(&s->fprogram, fname ? fname : name, bindings);
}

// void Shader_SetFpControl(Shader* s, u32 bits)
// The fragment program's attribute input mask becomes the vertex
// program's output mask with bits 0xFC0 (the texture coordinate inputs)
// taken from `bits`; then the program is re-sent
// (Gcm_SetFragmentProgramLoad into its +1136).
void Shader_SetFpControl(ShaderPrograms* s, u32 bits) {
    const u32 out = call<u32>(s4::fn::Cg_GetIOMask, or_zero_page(s->vprogram.get())->cg.get());
    const u32 merged = (out & ~s4::kTexInputBits) | (bits & s4::kTexInputBits);  // rlwimi 0,20,25
    call(s4::fn::Cg_SetIOMask, or_zero_page(s->fprogram.get())->cg.get(), merged);
    FProgram* fp = or_zero_page(s->fprogram.get());
    call(Gcm_SetFragmentProgramLoad, fp->cg.get(), &fp->program, 0u, 1u, 0u);
}

// ---------------------------------------------------------------- UV animation

// const f32* MaterialAnim_GetUV(MaterialAnim* anim, f32 su, f32 sv)
// The animated UV matrix m (MaterialAnim_GetMatrix) as two vertex rows in
// g::uv_rows: {m[0] * su, m[1] * su, m[8], m[9]}, {m[4] * sv, m[5] * sv, 0, 0}.
be_f32* MaterialAnim_GetUV(u8* anim, f32 su, f32 sv) {
    const be_f32* m = call<be_f32*>(s4::fn::MaterialAnim_GetMatrix, anim);
    m = or_zero_page(m);
    be_f32* out = host<be_f32>(s4::g::uv_rows);
    out[0] = m[0] * su;
    out[1] = m[1] * su;
    copy_word(out + 2, m + 8);
    copy_word(out + 3, m + 9);
    out[4] = m[4] * sv;
    out[5] = m[5] * sv;
    reinterpret_cast<be_u32*>(out)[6] = 0u;
    reinterpret_cast<be_u32*>(out)[7] = 0u;
    return out;
}

// void MaterialAnim_GetUVAlt(MaterialAnim* anim, f32 out[2])
// The U / V offsets (+44 / +92), each snapped to 1 / steps (+12 / +60)
// unless its steps are 0. With the wrap flag (+4), U becomes
// frac(V offset * V steps) snapped to 1 / U steps instead (the original
// multiplies the unsnapped V offset by V steps and divides by U steps
// even when U steps is 0).
void MaterialAnim_GetUVAlt(UVAnim* anim, be_f32* out) {
    const f32 us = anim->u_steps;
    const bool wrap = anim->wrap != 0;
    const f32 u = us != 0.0f ? snap(anim->u_offset, us) : static_cast<f32>(anim->u_offset);  // fcmpu
    const f32 vs = anim->v_steps;
    const f32 voff = anim->v_offset;
    const f32 v = vs != 0.0f ? snap(voff, vs) : voff;
    if (!wrap) {
        out[0] = u;
        out[1] = v;
        return;
    }
    const f32 p = voff * vs;               // fmuls
    const f32 frac = p - trunc_s(p);       // fsubs
    out[0] = trunc_s(frac * us) / us;      // fctidz / fcfid / frsp / fdivs
    out[1] = v;
}

// ---------------------------------------------------------------- vectors

// void Vec3_ToItemSpace(f32 v[3], const f32 m[3][4])
// v = (m rows) . v, in place: out[i] = m[i][0] v0 + m[i][1] v1 + m[i][2] v2
// (fmuls then two fmadds per row; rows 16 bytes apart).
void Vec3_ToItemSpace(be_f32* v, const be_f32* m) {
    const f32 y = v[1], x = v[0], z = v[2];
    for (u32 i = 0; i < 3; ++i) {
        const be_f32* row = at<be_f32>(m, 16 * i);
        f32 r = y * row[1];
        r = fmadds(x, row[0], r);
        r = fmadds(z, row[2], r);
        *at<be_f32>(v, 4 * i) = r;
    }
}

// void Vec3_RotateTo(f32 out[3], const f32 v[3], const f32 m[3][4])
// Vec3_ToItemSpace into a separate vector; v is re-read for each row.
void Vec3_RotateTo(be_f32* out, const be_f32* v, const be_f32* m) {
    for (u32 i = 0; i < 3; ++i) {
        const be_f32* row = at<be_f32>(m, 16 * i);
        f32 r = v[1] * row[1];
        r = fmadds(v[0], row[0], r);
        r = fmadds(v[2], row[2], r);
        *at<be_f32>(out, 4 * i) = r;
    }
}

}  // namespace sly4::gfx

NATIVE(0050AFDC, sly4::gfx::Shader_Construct)
NATIVE(0053A9D8, sly4::gfx::Shader_Load)
NATIVE(0053ABB8, sly4::gfx::Shader_SetFpControl)
NATIVE(0050BE5C, sly4::gfx::MaterialAnim_GetUV)
NATIVE(0050BF48, sly4::gfx::MaterialAnim_GetUVAlt)
NATIVE(00553468, sly4::gfx::Vec3_ToItemSpace)
NATIVE(005533EC, sly4::gfx::Vec3_RotateTo)
