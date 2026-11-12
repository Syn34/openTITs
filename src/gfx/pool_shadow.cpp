// Stencil shadow volume construction (StencilShadow vtable 0x00B241E8
// slots 0, 1 and 6): per-face light terms, back-facing caps and the
// extruded silhouette quads, written as triangles at the pool's cursor.
#include "pool.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 kTermScale = 0x0052664C;  // f32 127.0
constexpr u32 kSignConsts = 0x00526954;  // f32 1.0, 127.0

// Vertex `i` of the index list: positions are 16 bytes apart.
const be_f32* vertex(const be_f32* pos, const be_u16* idx, u32 i) {
    return at<be_f32>(pos, static_cast<u32>(idx[i]) << 4);
}

// Writes face j's triangle (positions at `pos`) at the cursor and, unless
// caps are off (+320), its copy pushed `+316` along the light in reverse
// winding.
void add_cap(StencilShadow* p, const be_f32* light, const be_u16* idx, const be_f32* pos, u32 j) {
    const be_f32* a = vertex(pos, idx, 3 * j);
    const be_f32* b = vertex(pos, idx, 3 * j + 1);
    be_f32* w = or_zero_page(p->cursor.get());
    const be_f32* c = vertex(pos, idx, 3 * j + 2);
    copy3(w, a);
    copy3(w + 3, b);
    copy3(w + 6, c);
    const u8 caps = p->caps;
    p->cursor = w + 9;
    if (caps != 0) return;
    const f32 d = p->extrude;
    const f32 lx = light[0], ly = light[1], lz = light[2];
    const f32 ax = fmadds(lx, d, a[0]), ay = fmadds(ly, d, a[1]), az = fmadds(lz, d, a[2]);
    const f32 bx = fmadds(lx, d, b[0]), by = fmadds(ly, d, b[1]), bz = fmadds(lz, d, b[2]);
    const f32 cx = fmadds(lx, d, c[0]), cy = fmadds(ly, d, c[1]), cz = fmadds(lz, d, c[2]);
    const f32 out[9] = {cx, cy, cz, bx, by, bz, ax, ay, az};
    for (u32 k = 0; k < 9; ++k) w[9 + k] = out[k];
    p->cursor = w + 18;
}

// Adds a cap for every face whose light term (+304) is negative. `pos`
// is re-read per face for the skinned path.
template <typename Pos>
void add_caps(StencilShadow* p, const be_f32* light, const be_u16* idx, u32 count, Pos pos) {
    const s32 faces = static_cast<s32>(count) / 3;
    if (faces <= 0) return;
    u32 j = 0;
    do {
        if (static_cast<s8>(*at<u8>(p->face_sign.get(), j)) < 0) add_cap(p, light, idx, pos(), j);
        j = (j + 1) & 0xFFFF;
    } while (static_cast<s32>(j) < faces);
}

}  // namespace

// void StencilShadow_AddCaps(StencilShadow* p, ShadowCaster* o, const f32 light[3])   (slot 0; name?)
// Skinned meshes go to slot 1. Otherwise each face's light term
// 127 * N.L (the stored normals, +0x8004) goes into +304 (faces =
// index count / 3, rounded up), then the faces facing away get caps.
void StencilShadow_AddCaps(StencilShadow* p, ShadowCaster* o, const be_f32* light) {
    ShadowMesh* m = or_zero_page(o->mesh.get());
    const be_f32* pos = m->positions;
    const be_u16* idx = m->indices;
    if (skinned(m)) {
        vcall(p, 4, o, light);
        return;
    }
    u32 count = m->index_count;
    if (count != 0) {
        const f32 k = global<be_f32>(kTermScale);
        u32 i = 0, n = 0;
        do {
            const be_f32* nrm = at<be_f32>(m->normals.get(), i << 4);
            const f32 ly = light[1], lx = light[0], lz = light[2];
            n = (n + 3) & 0xFFFF;
            f32 dot = nrm[1] * ly;
            dot = fmadds(nrm[0], lx, dot);
            dot = fmadds(nrm[2], lz, dot);
            *at<u8>(p->face_sign.get(), i) = static_cast<u8>(fctiwz(dot * k));
            count = m->index_count;
            i = (i + 1) & 0xFFFF;
        } while (static_cast<s32>(n) < static_cast<s32>(count));
    }
    add_caps(p, light, idx, count, [pos] { return pos; });
}

// void StencilShadow_FaceSign(StencilShadow* p, u32 index, const u16* idx, const f32* pos, const f32 light[3])   (name?)
// Face index / 3's light term from its corners: 127 * normalize((A - B) x
// (C - B)) . light into +304.
void StencilShadow_FaceSign(StencilShadow* p, u32 j, const be_u16* idx, const be_f32* pos, const be_f32* light) {
    const be_u16* t = at<be_u16>(idx, (j << 1) & 0x1FFFE);  // rlwinm 1,15,30
    const be_f32* a = at<be_f32>(pos, static_cast<u32>(t[0]) << 4);
    const be_f32* b = at<be_f32>(pos, static_cast<u32>(t[1]) << 4);
    const be_f32* c = at<be_f32>(pos, static_cast<u32>(t[2]) << 4);
    const f32 bx = b[0], by = b[1], bz = b[2];
    const f32 e1x = a[0] - bx, e2z = c[2] - bz, e1z = a[2] - bz;
    const f32 e2x = c[0] - bx, e2y = c[1] - by, e1y = a[1] - by;
    const f32 ny = fmsubs(e1z, e2x, e1x * e2z);
    const f32 nx = fmsubs(e1y, e2z, e1z * e2y);
    const f32 nz = fmsubs(e1x, e2y, e1y * e2x);
    const f32 len = std::sqrt(fmadds(nz, nz, fmadds(nx, nx, ny * ny)));
    const be_f32* k = host<be_f32>(kSignConsts);
    const f32 inv = k[0] / len;
    f32 dot = (ny * inv) * light[1];
    dot = fmadds(nx * inv, light[0], dot);
    dot = fmadds(nz * inv, light[2], dot);
    u8* buf = p->face_sign;
    const s32 face = static_cast<s32>(j & 0xFFFF) / 3;
    *at<u8>(buf, static_cast<u32>(face)) = static_cast<u8>(fctiwz(dot * k[1]));
}

// void StencilShadow_AddCapsSkinned(StencilShadow* p, ShadowCaster* o, const f32 light[3])   (slot 1; name?)
// AddCaps for skinned meshes: the light terms from the skinned positions
// (+0x8010) through StencilShadow_FaceSign, then the caps. Clears the
// caster's triangle count first.
void StencilShadow_AddCapsSkinned(StencilShadow* p, ShadowCaster* o, const be_f32* light) {
    ShadowMesh* m = or_zero_page(o->mesh.get());
    o->tris = 0;
    u32 count = m->index_count;
    const be_u16* idx = m->indices;
    if (count != 0) {
        u32 j = 0;
        do {
            StencilShadow_FaceSign(p, j, idx, m->skinned, light);  // (the original also passes o in r8)
            j = (j + 3) & 0xFFFF;
            count = m->index_count;
        } while (static_cast<s32>(j) < static_cast<s32>(count));
    }
    add_caps(p, light, idx, count, [m] { return m->skinned.get(); });
}

// void StencilShadow_Build(StencilShadow* p, ShadowCaster* o, const f32 light[3], bool caps)   (slot 6; name?)
// Records the caps flag and the cursor (+324), adds the caps (slot 0),
// then for every edge between a lit and an unlit face a quad (two
// triangles) from the edge to its copy pushed +316 along the light,
// wound by which side is lit. Stores the triangle count in the caster
// (+4) and rewinds the cursor to where it started.
void StencilShadow_Build(StencilShadow* p, ShadowCaster* o, const be_f32* light, u8 caps) {
    ShadowMesh* m = or_zero_page(o->mesh.get());
    be_f32* const start = p->cursor;
    const be_f32* pos = skinned(m) ? m->skinned : m->positions;
    p->caps = caps;
    const be_u16* idx = m->indices;
    p->start = start;
    vcall(p, 0, o, light);
    const u32 edges = m->edge_count;
    be_f32* w = or_zero_page(p->cursor.get());
    if (edges != 0) {
        const u8* sign = p->face_sign;
        const f32 d = p->extrude;
        const f32 lx = light[0], ly = light[1], lz = light[2];
        u32 i = 0;
        do {
            const u32 e = *at<be_u32>(m, 4 + (i << 2));
            const u32 face = (e >> 18) & 0x3FFF;  // rlwinm 14,18,31
            const u32 other = (e >> 4) & 0x3FFF;  // rlwinm 28,18,31
            const s32 lit_other = static_cast<s8>(*at<u8>(sign, other / 3)) >> 7;
            const s32 lit_face = static_cast<s8>(*at<u8>(sign, face / 3)) >> 7;
            if (lit_face != lit_other) {
                const be_f32* p0 = vertex(pos, idx, face + ((e >> 2) & 3));
                const be_f32* p1 = vertex(pos, idx, face + (e & 3));
                const u32 p1x = reinterpret_cast<const be_u32*>(p1)->get(), p0x = reinterpret_cast<const be_u32*>(p0)->get();
                const f32 e1x = fmadds(lx, d, p1[0]), e0x = fmadds(lx, d, p0[0]);
                const f32 e0y = fmadds(ly, d, p0[1]), e0z = fmadds(lz, d, p0[2]);
                const f32 e1y = fmadds(ly, d, p1[1]), e1z = fmadds(lz, d, p1[2]);
                // The face's own side faces the light: p0 p1' p0' / p0 p1 p1';
                // otherwise p1 p0' p1' / p1 p0 p0'.
                const bool own = lit_face == 0;
                const be_f32* a = own ? p0 : p1;
                const be_f32* b = own ? p1 : p0;
                const f32 ea[3] = {e0x, e0y, e0z}, eb[3] = {e1x, e1y, e1z};
                const f32* ext_a = own ? ea : eb;
                const f32* ext_b = own ? eb : ea;
                *reinterpret_cast<be_u32*>(w) = own ? p0x : p1x;
                copy_word(w + 1, a + 1);
                copy_word(w + 2, a + 2);
                for (u32 k = 0; k < 3; ++k) w[3 + k] = ext_b[k];
                for (u32 k = 0; k < 3; ++k) w[6 + k] = ext_a[k];
                copy3(w + 9, a);
                copy3(w + 12, b);
                for (u32 k = 0; k < 3; ++k) w[15 + k] = ext_b[k];
                w += 18;
                p->cursor = w;
            }
            i = (i + 1) & 0xFFFF;
        } while (static_cast<s32>(i) < static_cast<s32>(edges));
    }
    be_f32* begin = p->start;
    const s32 tris = (static_cast<s32>(guest(w) - guest(begin)) / 12) / 3;
    p->cursor = begin;
    o->tris = static_cast<u16>(tris);
}

}  // namespace sly4::gfx

NATIVE(00526650, sly4::gfx::StencilShadow_AddCaps)
NATIVE(00526960, sly4::gfx::StencilShadow_FaceSign)
NATIVE(00526A84, sly4::gfx::StencilShadow_AddCapsSkinned)
NATIVE(00526D98, sly4::gfx::StencilShadow_Build)
