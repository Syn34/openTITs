// The last shader helpers: material parameter slots, the sorted map
// lookup, the 4x4 product, SlyWater3's fragment constants and
// SlyHeightFog's UV matrices.
#include "shader6.h"
#include "shader_draw.h"

namespace sly4::gfx {

// Map (as Map_Find3 reads it).
struct SortedMap {
    pad<2> _0;
    be_u16 count;       // +2
    pad<2> _4;
    be_u16 flags;       // +6: bit 0 unsorted
    gp<MapEntry> data;  // +8: {u32 key, value}
    pad<4> _12;
    be_u32 missing;     // +16: the value returned for a missing key
    be_u32 compare;     // +20: compare OPD (int(const Entry*, const u32* key); also qsort's)
};
static_assert(offsetof(SortedMap, data) == 8 && offsetof(SortedMap, missing) == 16 && offsetof(SortedMap, compare) == 20);

namespace {

namespace s3 = sly4::render::shader3;
namespace s6 = sly4::render::shader6;

// fctidz / fcfid / frsp: x truncated toward zero through a 64-bit integer.
f32 trunc_s(f32 x) { return static_cast<f32>(static_cast<f64>(fctidz(x))); }

u8& param_capacity(Material* mat) { return *at<u8>(mat, s6::kMatParamCapacity); }

// One of SlyHeightFog's matrices: m = rotation about s3::g::rotation_axis
// by `angle` (Matrix_FromAxisAngle); t = m row 0 * su + row 1 * sv + row 2
// * 0 + row 3 per lane (vmaddfp from +0.0, then vaddfp; rows read with
// lvx); rows 0..2 scaled by `scale` (Matrix_ScaleRows); then row 2's xyz
// = {t.x - trunc(t.x), t.y - trunc(t.y), t.z}: the scroll, wrapped.
void heightfog_matrix(be_f32* m, f32 su, f32 sv, f32 angle, const be_u32* scale) {
    call(s3::fn::Matrix_FromAxisAngle, m, s3::g::rotation_axis, angle);
    const be_f32* a = aligned16(m);
    f32 t[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(a[k], su, 0.0f);
        v = std::fmaf(a[4 + k], sv, v);
        v = std::fmaf(a[8 + k], 0.0f, v);
        t[k] = a[12 + k] + v;
    }
    const f32 x = t[0], y = t[1];
    const f32 tx = trunc_s(x), ty = trunc_s(y);
    call(s3::fn::Matrix_ScaleRows, m, scale);
    m[8] = x - tx;  // fsubs
    m[9] = y - ty;
    *reinterpret_cast<be_u32*>(m + 10) = bits(t[2]);  // lfs / stfs from the stvx'd vector
}

}  // namespace

// u32* Material_ParamSlot(Material* mat, u32 index)
// &params[index] in the material's parameter words (+124), first making
// room: a missing array is allocated with max(index + 1, 8) words; one too
// small grows to max(index + 1, capacity == 16 ? 32 : 16) words (copied,
// the old one freed). The capacity (+121) is a u8: as in the original a
// count above 255 is stored truncated (the allocation is not).
be_u32* Material_ParamSlot(Material* mat, u32 index) {
    const u32 n = index + 1;
    u32 params = mat->w124;
    if (params == 0) {
        const u32 cap = n < 8 ? 8 : n;  // cmplwi
        param_capacity(mat) = static_cast<u8>(cap);
        params = call<u32>(s6::fn::SmallAlloc, (cap & 0xFF) << 2);  // rlwinm 2,22,29
        mat->w124 = params;
    } else {
        const u32 cap = param_capacity(mat);
        if (n > cap) {  // cmplw
            const u32 step = cap == 16 ? 32 : 16;
            const u32 grown = n < step ? step : n;  // cmplw
            params = call<u32>(s6::fn::SmallAlloc, grown << 2);
            call(s6::fn::memmove, params, mat->w124.get(), cap << 2);
            call(s6::fn::SmallFree, mat->w124.get(), cap << 2);
            mat->w124 = params;
            param_capacity(mat) = static_cast<u8>(grown);
        }
    }
    return at<be_u32>(nullptr, params + (index << 2));
}

// void* Map_Find3(Map* map, const u32* key)  (shader5::fn::Map_Find; the third copy of src/obj/map.cpp's lookup)
// The value of `key` in a sorted map, sorting it first (qsort with its
// own compare) when flagged unsorted; the map's +16 when the key is
// missing. A binary search: with the default compare the keys are
// compared inline, unsigned (the key re-read each step); otherwise the
// compare is called on (entry, a stack copy of the key) per step.
be_u32* Map_Find3(SortedMap* map, const be_u32* key) {
    const u32 flags = map->flags;
    if ((flags & 1) != 0) {
        map->flags = static_cast<u16>(flags & 0xFFFE);  // rlwinm 0,16,30
        const u32 count = map->count;
        if (count != 0) call(s6::fn::qsort, map->data.ea.get(), count, 8u, map->compare.get());
    }
    s32 hi = static_cast<s32>(map->count.get()) - 1;
    const u32 data = map->data.ea;
    s32 lo = 0;
    if (map->compare == s6::kMapCompareU32) {
        while (hi >= lo) {  // cmpw
            const s32 mid = (hi + lo) >> 1;  // srawi
            const u32 k = *key;
            MapEntry* e = at<MapEntry>(nullptr, data + 8 * static_cast<u32>(mid));
            const u32 ek = e->key;
            if (ek > k) {  // cmplw
                hi = mid - 1;
            } else if (ek < k) {
                lo = mid + 1;
            } else {
                return &e->value;
            }
        }
        return &map->missing;
    }
    GuestFrame frame(16);  // the original's sp+112
    be_u32* copy = frame.as<be_u32>();
    *copy = key->get();
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const s32 r = call_ptr<s32>(map->compare, data + 8 * static_cast<u32>(mid), copy);
        if (r > 0) {  // cmpwi
            hi = mid - 1;
        } else if (r < 0) {
            lo = mid + 1;
        } else {
            // The entries pointer re-read for the result.
            return at<be_u32>(nullptr, map->data.ea + 8 * static_cast<u32>(mid) + 4);
        }
    }
    return &map->missing;
}

// f32* Matrix_MultiplyTo(f32 out[16], const f32 a[16], const f32 b[16])
// out row i = a row i through b (Vec4_TransformByMatrix), rows 0..3;
// returns out.
be_f32* Matrix_MultiplyTo(be_f32* out, const be_f32* a, const be_f32* b) {
    for (u32 i = 0; i < 64; i += 16) call(s6::fn::Vec4_TransformByMatrix, at<u8>(out, i), at<u8>(a, i), b);
    return out;
}

// void SlyWater3_UploadFragment(Material* mat)
// Fragment constants 3 = {t * p+84, t * p+88, p+92 * 640 / width, 1} and 4
// = {t * p+96, t * p+100, p+104 * 640 / width, 1}, with p the material
// parameters (0.0 each when missing), t the material clock
// (s3::g::material_time) and width g::config +4 (u16); then the screen
// copy (Draw_GetScreenCopy) on texture unit 5 when it isn't already there
// (render state +72).
void SlyWater3_UploadFragment(Material* mat) {
    constexpr u32 k = 0x005D4FE8;  // .text: f32 0.0, 640.0
    const u32 params = mat->w124;
    const auto p = [&](u32 off) { return host<be_f32>(params != 0 ? params + off : k); };
    const f32 t = global<be_f32>(s3::g::material_time);
    GuestFrame frame(32);  // the original's sp+112
    be_f32* v = frame.as<be_f32>();
    be_u32* w = reinterpret_cast<be_u32*>(v);
    v[0] = t * *p(84);
    v[1] = t * *p(88);
    copy_word(v + 2, p(92));
    w[3] = 0x3F800000u;  // 1.0
    v[4] = t * *p(96);
    v[5] = t * *p(100);
    copy_word(v + 6, p(104));
    w[7] = 0x3F800000u;
    const auto per_width = [&] {  // lhz / fcfid / frsp, fdivs
        const f32 width = static_cast<f32>(config().width);
        return global<be_f32>(k + 4) / width;
    };
    v[2] = v[2] * per_width();  // fmuls
    v[6] = v[6] * per_width();
    FProgram_SetConstant(3, w);
    FProgram_SetConstant(4, w + 4);
    Texture* tex = Draw_GetScreenCopy();
    RenderState& st = rs();
    if (st.texture[5].get() == tex) return;
    st.texture[5] = tex;
    Texture* t5 = or_zero_page(tex);
    call(s6::fn::cellGcmSetTexture, st.context.get(), 5u, &t5->gcm);
    const u32 levels = t5->gcm.mipmap;
    call(s6::fn::cellGcmSetTextureControl, st.context.get(), 5u, 1u, 0u, ((levels << 8) - 256) & 0xFFFF, 0u);
}

// void SlyHeightFog_BuildMatrices(Material* mat, f32 a[4][4], f32 b[4][4])
// Two UV matrices (heightfog_matrix) from the material parameters p (0.0
// each when missing) and the material clock t: a rotated by p+8 and
// scrolled by (t * p+0, t * p+4), b rotated by p+32 and scrolled by (t *
// p+24, t * p+28); both scaled by (p+48, p+52, 0) as read before a (p is
// re-read for b).
void SlyHeightFog_BuildMatrices(Material* mat, be_f32* a, be_f32* b) {
    constexpr u32 k = 0x005C6EB4;  // .text: f32 0.0
    const f32 t = global<be_f32>(s3::g::material_time);
    u32 params = mat->w124;
    auto p = [&](u32 off) { return host<be_f32>(params != 0 ? params + off : k); };
    GuestFrame frame(16);  // the original's sp+128: the scale
    be_u32* scale = frame.as<be_u32>();
    copy_word(scale + 1, p(52));
    copy_word(scale, p(48));
    scale[2] = global<be_u32>(k).get();
    f32 su = t * *p(0);
    f32 sv = t * *p(4);
    heightfog_matrix(a, su, sv, *p(8), scale);
    params = mat->w124;
    su = t * *p(24);
    sv = t * *p(28);
    heightfog_matrix(b, su, sv, *p(32), scale);
}

}  // namespace sly4::gfx

NATIVE(0050C018, sly4::gfx::Material_ParamSlot)
NATIVE(007A68DC, sly4::gfx::Map_Find3)
NATIVE(0055499C, sly4::gfx::Matrix_MultiplyTo)
NATIVE(005D4FF0, sly4::gfx::SlyWater3_UploadFragment)
NATIVE(005C6EB8, sly4::gfx::SlyHeightFog_BuildMatrices)
