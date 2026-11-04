// Stencil shadow volume construction (StencilShadow vtable 0x00B241E8
// slots 0, 1 and 6): per-face light terms, back-facing caps and the
// extruded silhouette quads, written as triangles at the pool's cursor.
#include "pool.h"

using namespace sly4;

namespace {

namespace sp = sly4::render::pool;

using sp::fmadds;
using sp::fmsubs;
using sp::lfs;
using sp::sgl;
using sp::stfs;

constexpr u32 kTermScale = 0x0052664C;  // f32 127.0
constexpr u32 kSignConsts = 0x00526954;  // f32 1.0, 127.0

// Vertex `i` of the index list: positions are 16 bytes apart.
u32 vertex(u32 pos, u32 idx, u32 i) { return pos + (static_cast<u32>(load<u16>(idx + 2 * i)) << 4); }

// Writes face j's triangle (positions at `pos`) at the cursor and, unless
// caps are off (+320), its copy pushed `+316` along the light in reverse
// winding.
void add_cap(u32 p, u32 light, u32 idx, u32 pos, u32 j) {
    const u32 a = vertex(pos, idx, 3 * j), b = vertex(pos, idx, 3 * j + 1);
    const u32 w = load<u32>(p + sp::kCursor);
    const u32 c = vertex(pos, idx, 3 * j + 2);
    sp::copy3(w, a);
    sp::copy3(w + 12, b);
    sp::copy3(w + 24, c);
    const u8 caps = ref<u8>(p + sp::kCaps);
    store<u32>(p + sp::kCursor, w + 36);
    if (caps != 0) return;
    const f64 d = lfs(p + sp::kExtrude);
    const f64 lx = lfs(light), ly = lfs(light + 4), lz = lfs(light + 8);
    const f64 ax = fmadds(lx, d, lfs(a)), ay = fmadds(ly, d, lfs(a + 4)), az = fmadds(lz, d, lfs(a + 8));
    const f64 bx = fmadds(lx, d, lfs(b)), by = fmadds(ly, d, lfs(b + 4)), bz = fmadds(lz, d, lfs(b + 8));
    const f64 cx = fmadds(lx, d, lfs(c)), cy = fmadds(ly, d, lfs(c + 4)), cz = fmadds(lz, d, lfs(c + 8));
    const f64 out[9] = {cx, cy, cz, bx, by, bz, ax, ay, az};
    for (u32 k = 0; k < 9; ++k) stfs(w + 36 + 4 * k, out[k]);
    store<u32>(p + sp::kCursor, w + 72);
}

// Adds a cap for every face whose light term (+304) is negative. `pos`
// is re-read per face for the skinned path. Returns the index count.
template <typename Pos>
void add_caps(u32 p, u32 light, u32 idx, u32 count, Pos pos) {
    const s32 faces = static_cast<s32>(count) / 3;
    if (faces <= 0) return;
    u32 j = 0;
    do {
        if (static_cast<s8>(ref<u8>(load<u32>(p + sp::kFaceSign) + j)) < 0) add_cap(p, light, idx, pos(), j);
        j = (j + 1) & 0xFFFF;
    } while (static_cast<s32>(j) < faces);
}

}  // namespace

// void StencilShadow_AddCaps(StencilShadow* p, Caster* o, const f32 light[3])   (slot 0; name?)
// Skinned meshes go to slot 1. Otherwise each face's light term
// 127 * N.L (the stored normals, +0x8004) goes into +304 (faces =
// index count / 3, rounded up), then the faces facing away get caps.
DECOMP(00526650, StencilShadow_AddCaps) {
    const u32 p = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), light = arg<u32>(ctx, 2);
    const u32 m = load<u32>(o);
    const u32 pos = load<u32>(m + sp::kPositions), idx = load<u32>(m + sp::kIndices);
    if (load<u32>(load<u32>(m) + 72) != 0) {
        ctx.gpr[3] = fx::vcall(ctx, p, 4, {p, o, light});
        return;
    }
    u32 count = load<u16>(m + sp::kIndexCount);
    if (count != 0) {
        const f64 k = lfs(kTermScale);
        u32 i = 0, n = 0;
        do {
            const u32 nrm = load<u32>(m + sp::kNormals) + (i << 4);
            const f64 ly = lfs(light + 4), lx = lfs(light), lz = lfs(light + 8);
            n = (n + 3) & 0xFFFF;
            f64 dot = sgl(lfs(nrm + 4) * ly);
            dot = fmadds(lfs(nrm), lx, dot);
            dot = fmadds(lfs(nrm + 8), lz, dot);
            store<u8>(load<u32>(p + sp::kFaceSign) + i, static_cast<u8>(rt::fctiw(sgl(dot * k), true)));
            count = load<u16>(m + sp::kIndexCount);
            i = (i + 1) & 0xFFFF;
        } while (static_cast<s32>(n) < static_cast<s32>(count));
    }
    add_caps(p, light, idx, count, [pos] { return pos; });
    ctx.gpr[3] = light;
}

// void StencilShadow_FaceSign(StencilShadow* p, u32 index, const u16* idx, const f32* pos, const f32 light[3])   (name?)
// Face index / 3's light term from its corners: 127 * normalize((A - B) x
// (C - B)) . light into +304.
DECOMP(00526960, StencilShadow_FaceSign) {
    const u32 p = arg<u32>(ctx, 0), j = arg<u32>(ctx, 1), light = arg<u32>(ctx, 4);
    const u32 t = arg<u32>(ctx, 2) + ((j << 1) & 0x1FFFE);  // rlwinm 1,15,30
    const u32 pos = arg<u32>(ctx, 3);
    const u32 a = pos + (static_cast<u32>(load<u16>(t)) << 4);
    const u32 b = pos + (static_cast<u32>(load<u16>(t + 2)) << 4);
    const u32 c = pos + (static_cast<u32>(load<u16>(t + 4)) << 4);
    const f64 bx = lfs(b), by = lfs(b + 4), bz = lfs(b + 8);
    const f64 e1x = sgl(lfs(a) - bx), e2z = sgl(lfs(c + 8) - bz), e1z = sgl(lfs(a + 8) - bz);
    const f64 e2x = sgl(lfs(c) - bx), e2y = sgl(lfs(c + 4) - by), e1y = sgl(lfs(a + 4) - by);
    const f64 ny = fmsubs(e1z, e2x, sgl(e1x * e2z));
    const f64 nx = fmsubs(e1y, e2z, sgl(e1z * e2y));
    const f64 nz = fmsubs(e1x, e2y, sgl(e1y * e2x));
    const f64 len = sgl(std::sqrt(fmadds(nz, nz, fmadds(nx, nx, sgl(ny * ny)))));
    const f64 inv = sgl(lfs(kSignConsts) / len);
    f64 dot = sgl(sgl(ny * inv) * lfs(light + 4));
    dot = fmadds(sgl(nx * inv), lfs(light), dot);
    dot = fmadds(sgl(nz * inv), lfs(light + 8), dot);
    const u32 buf = load<u32>(p + sp::kFaceSign);
    const s32 face = static_cast<s32>(j & 0xFFFF) / 3;
    const u32 term = static_cast<u32>(rt::fctiw(sgl(dot * lfs(kSignConsts + 4)), true));
    store<u8>(buf + static_cast<u32>(face), static_cast<u8>(term));
    ctx.gpr[3] = buf;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(face));
    ctx.gpr[5] = term;
}

// void StencilShadow_AddCapsSkinned(StencilShadow* p, Caster* o, const f32 light[3])   (slot 1; name?)
// AddCaps for skinned meshes: the light terms from the skinned positions
// (+0x8010) through StencilShadow_FaceSign, then the caps. Clears the
// caster's triangle count first. Returns the index count.
DECOMP(00526A84, StencilShadow_AddCapsSkinned) {
    const u32 p = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), light = arg<u32>(ctx, 2);
    const u32 m = load<u32>(o);
    store<u16>(o + 4, 0);
    u32 count = load<u16>(m + sp::kIndexCount);
    const u32 idx = load<u32>(m + sp::kIndices);
    if (count != 0) {
        u32 j = 0;
        do {
            call(ctx, sp::fn::StencilShadow_FaceSign, {p, j, idx, load<u32>(m + sp::kSkinnedPositions), light, o});
            j = (j + 3) & 0xFFFF;
            count = load<u16>(m + sp::kIndexCount);
        } while (static_cast<s32>(j) < static_cast<s32>(count));
    }
    add_caps(p, light, idx, count, [m] { return load<u32>(m + sp::kSkinnedPositions); });
    ctx.gpr[3] = count;
}

// void StencilShadow_Build(StencilShadow* p, Caster* o, const f32 light[3], bool caps)   (slot 6; name?)
// Records the caps flag and the cursor (+324), adds the caps (slot 0),
// then for every edge between a lit and an unlit face a quad (two
// triangles) from the edge to its copy pushed +316 along the light,
// wound by which side is lit. Stores the triangle count in the caster
// (+4) and rewinds the cursor to where it started.
DECOMP(00526D98, StencilShadow_Build) {
    const u32 p = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), light = arg<u32>(ctx, 2);
    const u32 caps = arg<u32>(ctx, 3);
    const u32 m = load<u32>(o);
    const u32 start = load<u32>(p + sp::kCursor);
    const u32 pos = load<u32>(m + (load<u32>(load<u32>(m) + 72) != 0 ? sp::kSkinnedPositions : sp::kPositions));
    store<u8>(p + sp::kCaps, static_cast<u8>(caps));
    const u32 idx = load<u32>(m + sp::kIndices);
    store<u32>(p + sp::kStart, start);
    fx::vcall(ctx, p, 0, {p, o, light});
    const u32 edges = load<u16>(m + sp::kEdgeCount);
    u32 w = load<u32>(p + sp::kCursor);
    if (edges != 0) {
        const u32 sign = load<u32>(p + sp::kFaceSign);
        const f64 d = lfs(p + sp::kExtrude);
        const f64 lx = lfs(light), ly = lfs(light + 4), lz = lfs(light + 8);
        u32 i = 0;
        do {
            const u32 e = load<u32>(m + sp::kEdges + (i << 2));
            const u32 face = (e >> 18) & 0x3FFF;  // rlwinm 14,18,31
            const u32 other = (e >> 4) & 0x3FFF;  // rlwinm 28,18,31
            const s32 lit_other = static_cast<s8>(ref<u8>(sign + other / 3)) >> 7;
            const s32 lit_face = static_cast<s8>(ref<u8>(sign + face / 3)) >> 7;
            if (lit_face != lit_other) {
                const u32 p0 = vertex(pos, idx, face + ((e >> 2) & 3));
                const u32 p1 = vertex(pos, idx, face + (e & 3));
                const u32 p1x = load<u32>(p1), p0x = load<u32>(p0);
                const f64 e1x = fmadds(lx, d, lfs(p1)), e0x = fmadds(lx, d, lfs(p0));
                const f64 e0y = fmadds(ly, d, lfs(p0 + 4)), e0z = fmadds(lz, d, lfs(p0 + 8));
                const f64 e1y = fmadds(ly, d, lfs(p1 + 4)), e1z = fmadds(lz, d, lfs(p1 + 8));
                // The face's own side faces the light: p0 p1' p0' / p0 p1 p1';
                // otherwise p1 p0' p1' / p1 p0 p0'.
                const bool own = lit_face == 0;
                const u32 a = own ? p0 : p1, b = own ? p1 : p0;
                const f64 ea[3] = {e0x, e0y, e0z}, eb[3] = {e1x, e1y, e1z};
                const f64* ext_a = own ? ea : eb;
                const f64* ext_b = own ? eb : ea;
                store<u32>(w, own ? p0x : p1x);
                sp::copy_word(w + 4, a + 4);
                sp::copy_word(w + 8, a + 8);
                for (u32 k = 0; k < 3; ++k) stfs(w + 12 + 4 * k, ext_b[k]);
                for (u32 k = 0; k < 3; ++k) stfs(w + 24 + 4 * k, ext_a[k]);
                sp::copy3(w + 36, a);
                sp::copy3(w + 48, b);
                for (u32 k = 0; k < 3; ++k) stfs(w + 60 + 4 * k, ext_b[k]);
                w += 72;
                store<u32>(p + sp::kCursor, w);
            }
            i = (i + 1) & 0xFFFF;
        } while (static_cast<s32>(i) < static_cast<s32>(edges));
    }
    const u32 begin = load<u32>(p + sp::kStart);
    const s32 tris = (static_cast<s32>(w - begin) / 12) / 3;
    store<u32>(p + sp::kCursor, begin);
    store<u16>(o + 4, static_cast<u16>(tris));
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(tris));
}
