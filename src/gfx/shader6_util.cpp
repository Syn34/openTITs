// The last shader helpers: material parameter slots, the sorted map
// lookup, the 4x4 product, SlyWater3's fragment constants and
// SlyHeightFog's UV matrices.
#include "shader6.h"

using namespace sly4;

namespace {

namespace s3 = sly4::render::shader3;
namespace s6 = sly4::render::shader6;
namespace sp = sly4::render::pool;
namespace d3 = sly4::render::draw3;
namespace s2 = sly4::render::shader2;

// fctidz / fcfid / frsp: x truncated toward zero through a 64-bit integer.
f64 trunc_s(f64 x) { return sp::sgl(rt::fcfid(rt::fctid(x, true))); }

// One of SlyHeightFog's matrices: m = rotation about s3::g::rotation_axis
// by `angle` (Matrix_FromAxisAngle); t = m row 0 * su + row 1 * sv + row 2
// * 0 + row 3 per lane (vmaddfp from +0.0, then vaddfp; rows read with
// lvx); rows 0..2 scaled by `scale` (Matrix_ScaleRows); then row 2's xyz
// = {t.x - trunc(t.x), t.y - trunc(t.y), t.z}: the scroll, wrapped.
void heightfog_matrix(PPUContext& ctx, u32 m, f32 su, f32 sv, f64 angle, u32 scale) {
    callf(ctx, s3::fn::Matrix_FromAxisAngle, {m, s3::g::rotation_axis}, {angle});
    const u32 a = m & ~15u;
    f32 t[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(load<f32>(a + 4 * k), su, 0.0f);
        v = std::fmaf(load<f32>(a + 16 + 4 * k), sv, v);
        v = std::fmaf(load<f32>(a + 32 + 4 * k), 0.0f, v);
        t[k] = load<f32>(a + 48 + 4 * k) + v;
    }
    const f64 x = t[0], y = t[1];
    const f64 tx = trunc_s(x), ty = trunc_s(y);
    call(ctx, s3::fn::Matrix_ScaleRows, {m, scale});
    sp::stfs(m + 32, sp::sgl(x - tx));  // fsubs
    sp::stfs(m + 36, sp::sgl(y - ty));
    store<u32>(m + 40, std::bit_cast<u32>(t[2]));  // lfs / stfs from the stvx'd vector
}

}  // namespace

// u32* Material_ParamSlot(Material* mat, u32 index)
// &params[index] in the material's parameter words (+124), first making
// room: a missing array is allocated with max(index + 1, 8) words; one too
// small grows to max(index + 1, capacity == 16 ? 32 : 16) words (copied,
// the old one freed). The capacity (+121) is a u8: as in the original a
// count above 255 is stored truncated (the allocation is not).
DECOMP(0050C018, Material_ParamSlot) {
    const u32 mat = arg<u32>(ctx, 0), index = arg<u32>(ctx, 1);
    const u32 n = index + 1;
    u32 params = load<u32>(mat + d3::kMatParams);
    if (params == 0) {
        const u32 cap = n < 8 ? 8 : n;  // cmplwi
        store<u8>(mat + s6::kMatParamCapacity, static_cast<u8>(cap));
        params = static_cast<u32>(call(ctx, s6::fn::SmallAlloc, {(cap & 0xFF) << 2}));  // rlwinm 2,22,29
        store<u32>(mat + d3::kMatParams, params);
    } else {
        const u32 cap = ref<u8>(mat + s6::kMatParamCapacity);
        if (n > cap) {  // cmplw
            const u32 step = cap == 16 ? 32 : 16;
            const u32 grown = n < step ? step : n;  // cmplw
            params = static_cast<u32>(call(ctx, s6::fn::SmallAlloc, {grown << 2}));
            call(ctx, s6::fn::memmove, {params, load<u32>(mat + d3::kMatParams), cap << 2});
            call(ctx, s6::fn::SmallFree, {load<u32>(mat + d3::kMatParams), cap << 2});
            store<u32>(mat + d3::kMatParams, params);
            store<u8>(mat + s6::kMatParamCapacity, static_cast<u8>(grown));
        }
    }
    ret(ctx, params + (index << 2));
}

// void* Map_Find3(Map* map, const u32* key)  (shader5::fn::Map_Find; the third copy of src/obj/map.cpp's lookup)
// The value of `key` in a sorted map (layout: shader6.h), sorting it first
// (qsort with its own compare) when flagged unsorted; the map's +16 when
// the key is missing. A binary search: with the default compare the keys
// are compared inline, unsigned (the key re-read each step); otherwise
// the compare is called on (entry, a stack copy of the key) per step.
DECOMP(007A68DC, Map_Find3) {
    const u32 map = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 flags = load<u16>(map + s6::kMapFlags);
    if ((flags & 1) != 0) {
        store<u16>(map + s6::kMapFlags, static_cast<u16>(flags & 0xFFFE));  // rlwinm 0,16,30
        const u32 count = load<u16>(map + s6::kMapCount);
        if (count != 0) {
            call(ctx, s6::fn::qsort, {load<u32>(map + s6::kMapEntries), count, 8, load<u32>(map + s6::kMapCompare)});
        }
    }
    s32 hi = static_cast<s32>(load<u16>(map + s6::kMapCount)) - 1;
    const u32 data = load<u32>(map + s6::kMapEntries);
    s32 lo = 0;
    if (load<u32>(map + s6::kMapCompare) == s6::kMapCompareU32) {
        while (hi >= lo) {  // cmpw
            const s32 mid = (hi + lo) >> 1;  // srawi
            const u32 k = load<u32>(key);
            const u32 e = load<u32>(data + 8 * static_cast<u32>(mid));
            if (e > k) {  // cmplw
                hi = mid - 1;
            } else if (e < k) {
                lo = mid + 1;
            } else {
                ret(ctx, data + 8 * static_cast<u32>(mid) + 4);
                return;
            }
        }
        ret(ctx, map + s6::kMapMissing);
        return;
    }
    Stack frame(ctx, 16);  // the original's sp+112
    const u32 copy = frame.addr();
    store<u32>(copy, load<u32>(key));
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const u32 cmp = load<u32>(map + s6::kMapCompare);
        // The OPD and its entry go in r5 / r6, as the original's call sequence leaves them.
        const s32 r = static_cast<s32>(call_ptr(ctx, cmp, {data + 8 * static_cast<u32>(mid), copy, cmp, load<u32>(cmp)}));
        if (r > 0) {  // cmpwi
            hi = mid - 1;
        } else if (r < 0) {
            lo = mid + 1;
        } else {
            // The entries pointer re-read for the result.
            ret(ctx, load<u32>(map + s6::kMapEntries) + 8 * static_cast<u32>(mid) + 4);
            return;
        }
    }
    ret(ctx, map + s6::kMapMissing);
}

// f32* Matrix_MultiplyTo(f32 out[16], const f32 a[16], const f32 b[16])
// out row i = a row i through b (Vec4_TransformByMatrix), rows 0..3;
// returns out.
DECOMP(0055499C, Matrix_MultiplyTo) {
    const u64 r3 = ctx.gpr[3];
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    for (u32 i = 0; i < 64; i += 16) call(ctx, s6::fn::Vec4_TransformByMatrix, {out + i, a + i, b});
    ctx.gpr[3] = r3;
}

// void SlyWater3_UploadFragment(Material* mat)
// Fragment constants 3 = {t * p+84, t * p+88, p+92 * 640 / width, 1} and 4
// = {t * p+96, t * p+100, p+104 * 640 / width, 1}, with p the material
// parameters (0.0 each when missing), t the material clock
// (s3::g::material_time) and width g::config +4 (u16); then the screen
// copy (Draw_GetScreenCopy) on texture unit 5 when it isn't already there
// (render state +72).
DECOMP(005D4FF0, SlyWater3_UploadFragment) {
    constexpr u32 k = 0x005D4FE8;  // .text: f32 0.0, 640.0
    const u32 mat = arg<u32>(ctx, 0);
    const u32 params = load<u32>(mat + d3::kMatParams);
    const auto p = [&](u32 off) { return params != 0 ? params + off : k; };
    const f64 t = sp::lfs(s3::g::material_time);
    Stack frame(ctx, 32);  // the original's sp+112
    const u32 v = frame.addr();
    sp::stfs(v, sp::sgl(t * sp::lfs(p(84))));
    sp::stfs(v + 4, sp::sgl(t * sp::lfs(p(88))));
    sp::copy_word(v + 8, p(92));
    store<u32>(v + 12, 0x3F800000u);  // 1.0
    sp::stfs(v + 16, sp::sgl(t * sp::lfs(p(96))));
    sp::stfs(v + 20, sp::sgl(t * sp::lfs(p(100))));
    sp::copy_word(v + 24, p(104));
    store<u32>(v + 28, 0x3F800000u);
    const auto per_width = [&] {  // lhz / fcfid / frsp, fdivs
        const f64 w = sp::sgl(static_cast<f64>(load<u16>(sly4::g::config + 4)));
        return sp::sgl(sp::lfs(k + 4) / w);
    };
    sp::stfs(v + 8, sp::sgl(sp::lfs(v + 8) * per_width()));  // fmuls
    sp::stfs(v + 24, sp::sgl(sp::lfs(v + 24) * per_width()));
    call(ctx, d3::fn::FProgram_SetConstant, {3, v});
    call(ctx, d3::fn::FProgram_SetConstant, {4, v + 16});
    const u32 tex = static_cast<u32>(call(ctx, s2::fn::Draw_GetScreenCopy));
    const u32 st = fx::g::state;
    if (load<u32>(st + 72) == tex) {
        ctx.gpr[3] = st;  // as the original leaves it
        return;
    }
    store<u32>(st + 72, tex);
    call(ctx, s6::fn::cellGcmSetTexture, {load<u32>(st), 5, tex + 16});
    const u32 levels = ref<u8>(tex + 17);
    call(ctx, s6::fn::cellGcmSetTextureControl, {load<u32>(st), 5, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
}

// void SlyHeightFog_BuildMatrices(Material* mat, f32 a[4][4], f32 b[4][4])
// Two UV matrices (heightfog_matrix) from the material parameters p (0.0
// each when missing) and the material clock t: a rotated by p+8 and
// scrolled by (t * p+0, t * p+4), b rotated by p+32 and scrolled by (t *
// p+24, t * p+28); both scaled by (p+48, p+52, 0) as read before a (p is
// re-read for b).
DECOMP(005C6EB8, SlyHeightFog_BuildMatrices) {
    constexpr u32 k = 0x005C6EB4;  // .text: f32 0.0
    const u32 mat = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 t = sp::lfs(s3::g::material_time);
    u32 params = load<u32>(mat + d3::kMatParams);
    auto p = [&](u32 off) { return params != 0 ? params + off : k; };
    Stack frame(ctx, 16);  // the original's sp+128: the scale
    const u32 scale = frame.addr();
    sp::copy_word(scale + 4, p(52));
    sp::copy_word(scale, p(48));
    store<u32>(scale + 8, load<u32>(k));
    f32 su = static_cast<f32>(sp::sgl(t * sp::lfs(p(0))));
    f32 sv = static_cast<f32>(sp::sgl(t * sp::lfs(p(4))));
    heightfog_matrix(ctx, a, su, sv, sp::lfs(p(8)), scale);
    params = load<u32>(mat + d3::kMatParams);
    su = static_cast<f32>(sp::sgl(t * sp::lfs(p(24))));
    sv = static_cast<f32>(sp::sgl(t * sp::lfs(p(28))));
    heightfog_matrix(ctx, b, su, sv, sp::lfs(p(32)), scale);
}
