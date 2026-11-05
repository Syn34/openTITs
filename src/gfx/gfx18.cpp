// The renderer range's next recompiled functions (gfx18). Names carry the
// address where the role is unclear.
#include <atomic>
#include <bit>
#include <cmath>

#include "fx.h"
#include "gfx18.h"
#include "post.h"
#include "shader3.h"
#include "effect_init.h"
#include "display.h"
#include "../debug/debug.h"

using namespace sly4;

namespace {

namespace g11 = sly4::render::gfx11;
namespace g10 = sly4::render::gfx10;
namespace g12 = sly4::render::gfx12;
namespace g13 = sly4::render::gfx13;
namespace g15 = sly4::render::gfx15;
namespace g16 = sly4::render::gfx16;
namespace g17 = sly4::render::gfx17;
namespace g18 = sly4::render::gfx18;
using g11::lfs;
using g11::sgl;

// in[0] * row0 + in[1] * row1 + in[2] * row2 (+ row3 when translate) of
// the 4x4 at m (lvx: m & ~15), each lane in the original's vmaddfp
// order (from +0, then row by row; row 3 added last with vaddfp).
void rows_combine(u32 m, const f32 in[3], bool translate, f32 out[4]) {
    m &= ~15u;
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(load<f32>(m + 4 * k), in[0], 0.0f);
        v = std::fmaf(load<f32>(m + 16 + 4 * k), in[1], v);
        v = std::fmaf(load<f32>(m + 32 + 4 * k), in[2], v);
        out[k] = translate ? load<f32>(m + 48 + 4 * k) + v : v;
    }
}

// Every row of the 4x4 at m through the 4x4 at by (rows 0..2 without
// translation, row 3 with), as the stvx block of the emitter draws.
void rows_through(u32 m, u32 by) {
    f32 r[4][4];
    for (u32 row = 0; row < 4; ++row) {
        f32 in[3];
        for (u32 k = 0; k < 3; ++k) in[k] = load<f32>(((m + 16 * row) & ~15u) + 4 * k);
        rows_combine(by, in, row == 3, r[row]);
    }
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 4; ++k) store<f32>(((m + 16 * row) & ~15u) + 4 * k, r[row][k]);
}

// One sprite vertex at v: its x, y, z words, the particle's colour (four
// lbz / stb pairs) and u, v (lfs / stfs pairs: raw words). `first`: the
// coordinate stored before the others (the draws' first vertex stores
// the position's own word first), or -1.
void vertex_bits(u32 v, const u32 xyz[3], int first, u32 colour, u32 u_at, u32 v_at) {
    if (first >= 0) store<u32>(v + 4 * static_cast<u32>(first), xyz[first]);
    for (int k = 0; k < 3; ++k)
        if (k != first) store<u32>(v + 4 * static_cast<u32>(k), xyz[k]);
    for (u32 k = 0; k < 4; ++k) store<u8>(v + 12 + k, load<u8>(colour + k));
    store<u32>(v + 16, load<u32>(u_at));
    store<u32>(v + 20, load<u32>(v_at));
}
void sprite_vertex(u32 v, f64 x, f64 y, u32 z, u32 colour, u32 u_at, u32 v_at, bool z_first = false) {
    const u32 xyz[3] = {g11::stfs_bits(x), g11::stfs_bits(y), z};
    vertex_bits(v, xyz, z_first ? 2 : -1, colour, u_at, v_at);
}

// A quad's six indices (two triangles: 0 1 2, 1 2 3 from first; sth).
void quad_indices(u32 at, u32 first) {
    const u16 q[6] = {static_cast<u16>(first), static_cast<u16>(first + 1), static_cast<u16>(first + 2),
                      static_cast<u16>(first + 1), static_cast<u16>(first + 2), static_cast<u16>(first + 3)};
    for (u32 k = 0; k < 6; ++k) store<u16>(at + 2 * k, q[k]);
}

// The sprite draws' common end: the batch written from the index cursor
// the draw started at (re-read) to idx is recorded (+252 its index
// count, +248 its first index; +240 / +244 the new cursors); then, with
// a count, the stream's vertex format bound with the current buffer's
// vertex data, render state +4 = its index buffer, the fragment program
// uploaded and the batch drawn as triangles (EmitterDraw_Batched_005077B4's
// sequence). r3 as the original leaves it. cursor_first: the store order
// of EmitterDraw_Facing_005043B0 (+240 before +252).
void sprite_commit(PPUContext& ctx, u32 sb, u32 vtx, u32 idx, bool cursor_first = false) {
    namespace sp = sly4::render::pool;
    const u32 start = load<u32>(sb + g18::kSbIndexCursor);
    const s32 count = static_cast<s32>(idx - start) / 2;  // srawi / addze
    const s32 first = static_cast<s32>(start - load<u32>(sb + g18::kSbIndexBase)) / 2;
    if (cursor_first) store<u32>(sb + g18::kSbVertexCursor, vtx);
    store<s32>(sb + g18::kSbCount, count);
    if (!cursor_first) store<u32>(sb + g18::kSbVertexCursor, vtx);
    store<u32>(sb + g18::kSbIndexCursor, idx);
    store<s32>(sb + g18::kSbFirst, first);
    const u32 s = load<u32>(g16::g::stream_buffers);
    if (load<u32>(s + g18::kSbCount) == 0) return ret(ctx, 0u);
    const u32 st = fx::g::state;
    sp::bind_format(s, load<u32>(s + ((load<u32>(s + g16::kSbIndex) << 4) & ~15u) + g16::kSbVertexData + 4));
    store<u32>(st + 4, s + load<u32>(s + g16::kSbIndex) * 20 + g16::kSbIndexBuffers);
    const u32 n = load<u32>(s + g18::kSbCount), at = load<u32>(s + g18::kSbFirst);
    sp::upload_fragment(ctx);
    const u32 ib = load<u32>(st + 4);
    const u32 type = load<u32>(ib + 16);
    const u32 bytes = type == 1 ? 2u : 4u;  // CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16 : _32
    const u32 where = load<u32>(ib + 4);
    ret(ctx, call(ctx, g16::fn::cellGcmSetDrawIndexArray,
                  {load<u32>(st), g16::kPrimTriangles, n, type & 0xFF, where & 1, (where >> 1) + at * bytes}));
}

}  // namespace

// void EmitterDraw_Oriented_00505740(Emitter* o, ParticleList* list, x)
// [Emitter_Draw_00507B18, kind 1 with record mode 5] (name?)
// EmitterDraw_Matrices_00505518 with each particle's matrix turned to
// its direction: from list+124 - 1 down to 0, the particle matrix's rows
// scaled by its size (Matrix_ScaleAxes with {s, s, s}), row 3 xyz = its
// position (raw); then the direction d (g18::g::particle_dir) normalized
// to n, an up vector u (gfx11 axis_b50, or axis_b60 when |n . u| > 0.95),
// r = normalize(n x u) and t = r x n; the rows' xyz become t * M, n * M
// and r * M (M the scaled rows 0..2; the w words stay). Unless o+24 is
// set every row then goes through the list's 4x4 as in 00505518. Then
// EmitterDraw_Layered_005052FC(o, list, x), whose r3 it leaves.
DECOMP(00505740, EmitterDraw_Oriented_00505740) {
    const u32 o = arg<u32>(ctx, 0), list = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    const u32 n = load<u32>(list + 124);
    if (n != 0) {
        Stack frame(ctx, 16);  // the original's sp+112: the scale
        const u32 sv = frame.addr();
        for (u32 i = n - 1;; --i) {
            const u32 m = g13::g::particle_mtx + 64 * i;
            const u32 s = load<u32>(g13::g::particle_scale + 4 * i);  // lfsx / stfs: raw
            for (u32 k = 0; k < 12; k += 4) store<u32>(sv + k, s);
            ctx.fpr[1] = lfs(g13::g::particle_scale + 4 * i);
            call(ctx, g13::fn::Matrix_ScaleAxes, {m, sv});
            const u32 p = g13::g::particle_pos + 16 * i;
            for (u32 k = 0; k < 12; k += 4) store<u32>(m + 48 + k, load<u32>(p + k));  // lfs / stfs: raw

            // n = d / |d| (fmuls / fmadds / fsqrts / fdivs)
            const u32 d = g18::g::particle_dir + 16 * i;
            const f64 dx = lfs(d), dy = lfs(d + 4), dz = lfs(d + 8);
            f64 len = g11::madds(dz, dz, g11::madds(dx, dx, sgl(dy * dy)));
            len = sgl(std::sqrt(len));
            f64 k = sgl(static_cast<f64>(g18::kOne) / len);
            const f64 nx = sgl(dx * k), ny = sgl(dy * k), nz = sgl(dz * k);
            // the up vector
            u32 up = g11::g::axis_b50;
            {
                const f64 dot = g11::madds(nz, lfs(up + 8), g11::madds(nx, lfs(up), sgl(ny * lfs(up + 4))));
                if (std::fabs(dot) > static_cast<f64>(g18::kAlongLimit)) up = g11::g::axis_b60;
            }
            const f64 ux = lfs(up), uy = lfs(up + 4), uz = lfs(up + 8);
            // r = normalize(n x u)
            f64 ry = g11::msubs(nz, ux, sgl(nx * uz));
            f64 rz = g11::msubs(nx, uy, sgl(ny * ux));
            f64 rx = g11::msubs(ny, uz, sgl(nz * uy));
            len = g11::madds(rz, rz, g11::madds(rx, rx, sgl(ry * ry)));
            len = sgl(std::sqrt(len));
            k = sgl(static_cast<f64>(g18::kOne) / len);
            ry = sgl(ry * k);
            rx = sgl(rx * k);
            rz = sgl(rz * k);
            // t = r x n
            const f64 tz = g11::msubs(rx, ny, sgl(ry * nx));
            const f64 ty = g11::msubs(rz, nx, sgl(rx * nz));
            const f64 tx = g11::msubs(ry, nz, sgl(rz * ny));

            auto f = [](f64 v) { return static_cast<f32>(v); };
            const f32 tv[3] = {f(tx), f(ty), f(tz)}, nv[3] = {f(nx), f(ny), f(nz)}, rv[3] = {f(rx), f(ry), f(rz)};
            f32 rows[3][4];
            rows_combine(m, tv, false, rows[0]);
            rows_combine(m, nv, false, rows[1]);
            rows_combine(m, rv, false, rows[2]);
            for (u32 row = 0; row < 3; ++row)
                for (u32 c = 0; c < 3; ++c) store<u32>(m + 16 * row + 4 * c, std::bit_cast<u32>(rows[row][c]));
            ctx.fpr[1] = static_cast<f64>(rows[2][2]);  // the last lfs
            if (load<u32>(o + 24) == 0) rows_through(m, list);
            if (i == 0) break;
        }
    }
    ret(ctx, call(ctx, g13::fn::EmitterDraw_Layered_005052FC, {o, list, x}));
}

// void EmitterDraw_Sprites_00503BC0(Emitter* e, Record* rec)
// [Emitter_Draw_00507B18, kind 0 with record mode 4] (name?)
// Axis-aligned sprites into the stream buffers (g16::g::stream_buffers;
// nothing without a vertex cursor): for each particle from rec+124 - 1
// down to 0, the half sizes rec+64 / +68 scaled by its size give
// x0 / x1 = pivot * w -/+ w (and y alike); the quad's corners (x1 y1,
// x0 y1, x1 y0, x0 y0) are offset by its position, and with rec+108 set
// also by its matrix's translation, which the original first adds the
// position to (a quirk: the particle matrix's row 3 keeps it); z is
// that sum + 0 there, else the position's z (raw). Each corner gets the
// particle's colour and its texture rect (g18::g::particle_uv), and six
// indices from the stream's vertex index (vertex cursor - base) /
// stride. Then sprite_commit.
DECOMP(00503BC0, EmitterDraw_Sprites_00503BC0) {
    const u32 rec = arg<u32>(ctx, 1);
    const u32 sb = load<u32>(g16::g::stream_buffers);
    u32 idx = load<u32>(sb + g18::kSbIndexCursor);
    u32 vtx = load<u32>(sb + g18::kSbVertexCursor);
    const u32 stride = load<u32>(sb + g18::kSbStride);
    u32 first = stride != 0 ? (vtx - load<u32>(sb + g18::kSbVertexBase)) / stride : 0u;  // divwu (0 by 0)
    if (vtx == 0) return ret(ctx, idx);
    const u32 n = load<u32>(rec + g18::kRecCount);
    if (n != 0) {
        for (u32 i = n - 1;; --i) {
            const f64 s = lfs(g13::g::particle_scale + 4 * i);
            const f64 w = sgl(lfs(rec + g18::kRecHalfW) * s), h = sgl(lfs(rec + g18::kRecHalfH) * s);
            const f64 px = lfs(rec + g18::kRecPivotX), py = lfs(rec + g18::kRecPivotY);
            f64 x0 = g11::msubs(px, w, w), x1 = g11::madds(px, w, w);
            f64 y0 = g11::msubs(py, h, h), y1 = g11::madds(py, h, h);
            const u32 p = g13::g::particle_pos + 16 * i, uv = g18::g::particle_uv + 16 * i;
            const u32 colour = g13::g::particle_colour + 4 * i;
            const u32 v1 = vtx + g18::kSpriteVertexSize, v2 = v1 + g18::kSpriteVertexSize, v3 = v2 + g18::kSpriteVertexSize;
            if (load<u32>(rec + g18::kRecRelative) != 0) {
                const u32 t = g13::g::particle_mtx + 64 * i + 48;
                const f64 tx = sgl(lfs(t) + lfs(p));
                const f64 ty0 = lfs(t + 4), tz0 = lfs(t + 8);
                x1 = sgl(x1 + tx);
                x0 = sgl(x0 + tx);
                store<u32>(t, g11::stfs_bits(tx));
                const f64 ty = sgl(ty0 + lfs(p + 4));
                y1 = sgl(y1 + ty);
                y0 = sgl(y0 + ty);
                store<u32>(t + 4, g11::stfs_bits(ty));
                const f64 tz = sgl(tz0 + lfs(p + 8));
                const u32 z = g11::stfs_bits(sgl(tz + 0.0));  // + the .text 0.0 at 0x00503BB8
                store<u32>(t + 8, g11::stfs_bits(tz));
                sprite_vertex(vtx, x1, y1, z, colour, uv + 4, uv + 8);
                sprite_vertex(v1, x0, y1, z, colour, uv, uv + 8);
                sprite_vertex(v2, x1, y0, z, colour, uv + 4, uv + 12);
                sprite_vertex(v3, x0, y0, z, colour, uv, uv + 12);
            } else {
                const f64 ox = lfs(p), oy = lfs(p + 4);
                const u32 z = load<u32>(p + 8);  // lfs / stfs: raw
                x1 = sgl(ox + x1);
                y1 = sgl(oy + y1);
                x0 = sgl(ox + x0);
                y0 = sgl(oy + y0);
                sprite_vertex(vtx, x1, y1, z, colour, uv + 4, uv + 8, true);
                sprite_vertex(v1, x0, y1, z, colour, uv, uv + 8);
                sprite_vertex(v2, x1, y0, z, colour, uv + 4, uv + 12);
                sprite_vertex(v3, x0, y0, z, colour, uv, uv + 12);
            }
            ctx.fpr[1] = lfs(uv + 12);  // the last lfs
            quad_indices(idx, first);
            vtx = v3 + g18::kSpriteVertexSize;
            idx += 12;
            first += 4;
            if (i == 0) break;
        }
    }
    sprite_commit(ctx, sb, vtx, idx);
}

namespace {

// The plane a sprite draw lays its quads in: the coordinates the x and
// y half sizes go on and the one left at the position.
struct SpritePlane {
    int ax, ay, fixed;
};

// EmitterDraw_Sprites_00503BC0's loop for the other planes. For each
// particle from rec+124 - 1 down to 0: x0 / x1 = pivot x * w -/+ w and
// y0 / y1 alike (w, h the half sizes rec+64 / +68 times its size); the
// corners (x1 y1, x0 y1, x1 y0, x0 y0) are, with rec+108 set, points
// {x, y on their axes, 0} through the particle matrix (vmaddfp order),
// whose translation first gets the position added (a quirk: the
// particle matrix keeps it); else the position offset on the two axes
// (the third word raw). Colour, texture rect and six indices as there;
// then sprite_commit.
void sprite_plane_quads(PPUContext& ctx, u32 rec, u32 positions, SpritePlane pl) {
    const u32 sb = load<u32>(g16::g::stream_buffers);
    u32 idx = load<u32>(sb + g18::kSbIndexCursor);
    u32 vtx = load<u32>(sb + g18::kSbVertexCursor);
    const u32 stride = load<u32>(sb + g18::kSbStride);
    u32 first = stride != 0 ? (vtx - load<u32>(sb + g18::kSbVertexBase)) / stride : 0u;  // divwu (0 by 0)
    if (vtx == 0) return ret(ctx, idx);
    const u32 n = load<u32>(rec + g18::kRecCount);
    if (n != 0) {
        for (u32 i = n - 1;; --i) {
            const f64 s = lfs(g13::g::particle_scale + 4 * i);
            const f64 w = sgl(lfs(rec + g18::kRecHalfW) * s), h = sgl(lfs(rec + g18::kRecHalfH) * s);
            const f64 px = lfs(rec + g18::kRecPivotX), py = lfs(rec + g18::kRecPivotY);
            const f64 xs[2] = {g11::madds(px, w, w), g11::msubs(px, w, w)};  // x1, x0
            const f64 ys[2] = {g11::madds(py, h, h), g11::msubs(py, h, h)};  // y1, y0
            const u32 p = positions + 16 * i, uv = g18::g::particle_uv + 16 * i;
            const u32 colour = g13::g::particle_colour + 4 * i;
            u32 c[4][3];  // the corners x1 y1, x0 y1, x1 y0, x0 y0
            int lead = -1;
            if (load<u32>(rec + g18::kRecRelative) != 0) {
                const u32 m = g13::g::particle_mtx + 64 * i, t = m + 48;
                const f64 tx = sgl(lfs(t) + lfs(p));
                const f64 ty0 = lfs(t + 4), tz0 = lfs(t + 8);
                store<u32>(t, g11::stfs_bits(tx));
                store<u32>(t + 4, g11::stfs_bits(sgl(ty0 + lfs(p + 4))));
                store<u32>(t + 8, g11::stfs_bits(sgl(tz0 + lfs(p + 8))));
                for (u32 k = 0; k < 4; ++k) {
                    f32 in[3] = {0.0f, 0.0f, 0.0f}, out[4];
                    in[pl.ax] = static_cast<f32>(xs[k & 1]);
                    in[pl.ay] = static_cast<f32>(ys[k >> 1]);
                    rows_combine(m, in, true, out);
                    for (u32 q = 0; q < 3; ++q) c[k][q] = std::bit_cast<u32>(out[q]);
                }
            } else {
                const f64 oa = lfs(p + 4 * static_cast<u32>(pl.ax)), ob = lfs(p + 4 * static_cast<u32>(pl.ay));
                const u32 raw = load<u32>(p + 4 * static_cast<u32>(pl.fixed));  // lfs / stfs
                for (u32 k = 0; k < 4; ++k) {
                    c[k][pl.fixed] = raw;
                    c[k][pl.ax] = g11::stfs_bits(sgl(oa + xs[k & 1]));
                    c[k][pl.ay] = g11::stfs_bits(sgl(ob + ys[k >> 1]));
                }
                lead = pl.fixed;
            }
            const u32 us[2] = {uv + 4, uv}, vs[2] = {uv + 8, uv + 12};
            for (u32 k = 0; k < 4; ++k)
                vertex_bits(vtx + g18::kSpriteVertexSize * k, c[k], k == 0 ? lead : -1, colour, us[k & 1], vs[k >> 1]);
            ctx.fpr[1] = lfs(uv + 12);  // the last lfs
            quad_indices(idx, first);
            vtx += 4 * g18::kSpriteVertexSize;
            idx += 12;
            first += 4;
            if (i == 0) break;
        }
    }
    sprite_commit(ctx, sb, vtx, idx);
}

}  // namespace

// void EmitterDraw_SpritesXY_00502240(Emitter* e, Record* rec)
// [Emitter_Draw_00507B18, kind 0 with record mode 1] (name?)
// Quads in the xy plane (sprite_plane_quads; z the position's).
DECOMP(00502240, EmitterDraw_SpritesXY_00502240) {
    sprite_plane_quads(ctx, arg<u32>(ctx, 1), g13::g::particle_pos, {0, 1, 2});
}

// void EmitterDraw_SpritesXZ_00502AC0(Emitter* e, Record* rec)
// [Emitter_Draw_00507B18, kind 0 with record mode 2] (name?)
// Quads in the xz plane (the y half size on z; y the position's).
DECOMP(00502AC0, EmitterDraw_SpritesXZ_00502AC0) {
    sprite_plane_quads(ctx, arg<u32>(ctx, 1), g13::g::particle_pos, {0, 2, 1});
}

// void EmitterDraw_SpritesZY_00503340(Emitter* e, Record* rec)
// [Emitter_Draw_00507B18, kind 0 with record mode 3] (name?)
// Quads in the yz plane (the x half size on z; x the position's).
DECOMP(00503340, EmitterDraw_SpritesZY_00503340) {
    sprite_plane_quads(ctx, arg<u32>(ctx, 1), g13::g::particle_pos, {2, 1, 0});
}

// void EmitterDraw_SpritesDefault_005019B8(Emitter* e, Record* rec)
// [Emitter_Draw_00507B18, kind 0 with any other record mode] (name?)
// EmitterDraw_SpritesXY_00502240 from the second position array
// (g18::g::particle_pos2).
DECOMP(005019B8, EmitterDraw_SpritesDefault_005019B8) {
    sprite_plane_quads(ctx, arg<u32>(ctx, 1), g18::g::particle_pos2, {0, 1, 2});
}

// void EmitterDraw_Facing_005043B0(Emitter* e, Record* rec)
// [Emitter_Draw_00507B18, kind 0 with record mode 5] (name?)
// Quads turned to each particle's direction: n = d / |d|
// (g18::g::particle_dir), an up vector u (gfx11 axis_b50, or
// kAxisB70 when |n . u| > 0.95), r = normalize(n x u) and t = r x n as
// in EmitterDraw_Oriented_00505740; the corners (x1 y1, x0 y1, x1 y0,
// x0 y0 as in EmitterDraw_Sprites_00503BC0, no matrix) are position +
// x t + y r (+ 0 n; vmaddfp order: t first, the position last). The
// rows' w lanes come from stale stack words in the original but only
// xyz are stored. Colour, texture rect and indices as there; then
// sprite_commit.
DECOMP(005043B0, EmitterDraw_Facing_005043B0) {
    const u32 rec = arg<u32>(ctx, 1);
    const u32 sb = load<u32>(g16::g::stream_buffers);
    u32 idx = load<u32>(sb + g18::kSbIndexCursor);
    u32 vtx = load<u32>(sb + g18::kSbVertexCursor);
    const u32 stride = load<u32>(sb + g18::kSbStride);
    u32 first = stride != 0 ? (vtx - load<u32>(sb + g18::kSbVertexBase)) / stride : 0u;  // divwu (0 by 0)
    if (vtx == 0) return ret(ctx, idx);
    const u32 n = load<u32>(rec + g18::kRecCount);
    if (n != 0) {
        for (u32 i = n - 1;; --i) {
            const u32 d = g18::g::particle_dir + 16 * i;
            const f64 dx = lfs(d), dy = lfs(d + 4), dz = lfs(d + 8);
            f64 len = sgl(std::sqrt(g11::madds(dz, dz, g11::madds(dx, dx, sgl(dy * dy)))));
            const f64 s = lfs(g13::g::particle_scale + 4 * i);
            const f64 w = sgl(lfs(rec + g18::kRecHalfW) * s), h = sgl(lfs(rec + g18::kRecHalfH) * s);
            const f64 px = lfs(rec + g18::kRecPivotX), py = lfs(rec + g18::kRecPivotY);
            const f64 xs[2] = {g11::madds(px, w, w), g11::msubs(px, w, w)};  // x1, x0
            const f64 ys[2] = {g11::madds(py, h, h), g11::msubs(py, h, h)};  // y1, y0
            f64 k = sgl(static_cast<f64>(g18::kOne) / len);
            const f64 nz = sgl(dz * k), nx = sgl(dx * k), ny = sgl(dy * k);
            u32 up = g11::g::axis_b50;
            {
                const f64 dot = g11::madds(nz, lfs(up + 8), g11::madds(nx, lfs(up), sgl(ny * lfs(up + 4))));
                if (std::fabs(dot) > static_cast<f64>(g18::kAlongLimit)) up = g18::kAxisB70;
            }
            const f64 ux = lfs(up), uy = lfs(up + 4), uz = lfs(up + 8);
            f64 ry = g11::msubs(nz, ux, sgl(nx * uz));
            f64 rz = g11::msubs(nx, uy, sgl(ny * ux));
            f64 rx = g11::msubs(ny, uz, sgl(nz * uy));
            len = sgl(std::sqrt(g11::madds(rz, rz, g11::madds(rx, rx, sgl(ry * ry)))));
            k = sgl(static_cast<f64>(g18::kOne) / len);
            ry = sgl(ry * k);
            rx = sgl(rx * k);
            rz = sgl(rz * k);
            const f64 tz = g11::msubs(rx, ny, sgl(ry * nx));
            const f64 ty = g11::msubs(rz, nx, sgl(rx * nz));
            const f64 tx = g11::msubs(ry, nz, sgl(rz * ny));

            const u32 p = g13::g::particle_pos + 16 * i;
            auto f = [](f64 v) { return static_cast<f32>(v); };
            const f32 T[3] = {f(tx), f(ty), f(tz)}, R[3] = {f(rx), f(ry), f(rz)}, N[3] = {f(nx), f(ny), f(nz)};
            u32 c[4][3];  // the corners x1 y1, x0 y1, x1 y0, x0 y0
            for (u32 q = 0; q < 4; ++q) {
                const f32 x = f(xs[q & 1]), y = f(ys[q >> 1]);
                for (u32 l = 0; l < 3; ++l) {
                    f32 v = std::fmaf(T[l], x, 0.0f);
                    v = std::fmaf(R[l], y, v);
                    v = std::fmaf(N[l], 0.0f, v);
                    c[q][l] = std::bit_cast<u32>(load<f32>(p + 4 * l) + v);  // vaddfp
                }
            }
            const u32 uv = g18::g::particle_uv + 16 * i, colour = g13::g::particle_colour + 4 * i;
            const u32 us[2] = {uv + 4, uv}, vs[2] = {uv + 8, uv + 12};
            for (u32 q = 0; q < 4; ++q)
                vertex_bits(vtx + g18::kSpriteVertexSize * q, c[q], -1, colour, us[q & 1], vs[q >> 1]);
            ctx.fpr[1] = lfs(uv);  // the last lfs f1
            quad_indices(idx, first);
            vtx += 4 * g18::kSpriteVertexSize;
            idx += 12;
            first += 4;
            if (i == 0) break;
        }
    }
    sprite_commit(ctx, sb, vtx, idx, true);
}

namespace {

// A program map's Array (post::g::vmap / fmap) as the inlined
// constructor builds it: capacity 256, count 0, element 16, data from
// the default allocator's slot 0 when the capacity is set (capacity *
// element / 2 bytes, align 16; r7 / r8 the entry and OPD as that
// sequence leaves them), Array_Init. The +16 / +20 stores follow in the
// caller (their order differs).
void program_map(PPUContext& ctx, u32 a) {
    store<u16>(a, g18::kProgramMapCapacity);
    store<u16>(a + 2, 0);
    store<u16>(a + 4, 0);
    store<u16>(a + 6, g18::kProgramMapElement);
    store<u32>(a + 8, 0);
    const u32 al = static_cast<u32>(call(ctx, g12::fn::Mem_DefaultAllocator));
    const u32 cap = load<u16>(a);
    store<u32>(a + 12, al);
    if (cap != 0) {
        const u32 bytes = cap * ((load<u16>(a + 6) >> 1) & 0x7FFF);  // rlwinm 31,17,31
        const u32 opd = load<u32>(load<u32>(al));
        store<u32>(a + 8, static_cast<u32>(call_ptr(ctx, opd, {al, bytes, 0, 16, load<u32>(opd), opd})));
    }
    call(ctx, g12::fn::Array_Init, {a});
}

}  // namespace

// void RenderState_Construct_0054E7D8()  [static initialiser] (name?)
// The render state (fx::g::state, 236 bytes) cleared and given its
// initial values: +8 / +216 -1 (no vertex data / fog cached), +32 all
// 16 attributes on, the dirty bit 31 of +92 cleared, both stencil sides
// ALWAYS / 0 / ~0 and KEEP x3, blend and the other cached GCM enums;
// the program maps post::g::vmap / fmap built (program_map) with their
// compare OPDs; the job buffer part's flags; then the
// PostProc_ThermalObject ShaderType static (effect::g::thermal_shader:
// base vtable, name, its hash, the PtrVector (+18 = 4, capacity 2),
// its vtable). r3 as left: the vtable.
DECOMP(0054E7D8, RenderState_Construct_0054E7D8) {
    const u32 st = fx::g::state;
    call(ctx, sly4::fn::memset, {st, 0, g18::kRenderStateSize});
    store<u32>(st + 4, 0);
    store<s32>(st + 8, -1);
    store<u32>(st + 16, 0);
    store<u32>(st + 20, 0);
    store<u32>(st + 24, 0);
    store<u32>(st + 28, 0);
    store<u16>(st + 32, 0xFFFF);
    store<u32>(st + 88, 0);
    for (u32 k = 52; k < 84; k += 4) store<u32>(st + k, 0);  // the 8 texture slots
    store<s32>(st + 216, -1);
    store<u8>(st + 220, 0);
    store<u8>(st + 221, 0);
    const u32 dirty = load<u32>(st + 92) & 0x7FFFFFFFu;  // rlwinm 0,1,31
    store<u32>(st + 160, g18::kGcmAlways);
    store<u32>(st + 164, 0);
    store<s32>(st + 168, -1);
    store<u32>(st + 92, dirty);
    store<s32>(st + 172, -1);
    for (u32 k = 176; k <= 184; k += 4) store<u32>(st + k, g18::kGcmKeep);
    store<u32>(st + 188, g18::kGcmAlways);
    store<u32>(st + 192, 0);
    store<s32>(st + 196, -1);
    store<s32>(st + 200, -1);
    for (u32 k = 204; k <= 212; k += 4) store<u32>(st + k, g18::kGcmKeep);
    for (u32 k = 36; k <= 48; k += 4) store<u32>(st + k, 0);
    store<u32>(st + 84, 0);
    store<u8>(st + 93, 0);
    store<u8>(st + 96, 1);
    store<u8>(st + 97, 0);
    store<u8>(st + 98, 8);
    store<u8>(st + 99, 1);
    for (u32 k = 100; k <= 103; ++k) store<u8>(st + k, 0);
    for (u32 k = 104; k <= 116; k += 4) store<u32>(st + k, 0);
    for (u32 k = 120; k <= 132; k += 4) store<u32>(st + k, 64);
    store<u8>(st + 136, 2);
    store<u8>(st + 137, 0);
    store<u32>(st + 140, 0);
    store<u32>(st + 144, 64);
    store<u16>(st + 148, 64);
    store<u16>(st + 150, 64);
    store<u16>(st + 152, 0);
    store<u16>(st + 154, 0);
    static constexpr u8 kBytes222[12] = {1, 2, 2, 3, 4, 4, 5, 6, 6, 5, 6, 6};
    for (u32 k = 0; k < 12; ++k) store<u8>(st + 222 + k, kBytes222[k]);

    const u32 vm = sly4::post::g::vmap, fm = sly4::post::g::fmap;
    program_map(ctx, vm);
    store<u32>(vm + 20, sly4::post::g::vmap_compare);
    store<u32>(vm + 16, 0);
    program_map(ctx, fm);
    const u32 part = g18::kJobPart;
    store<u32>(part + 4, (load<u32>(part + 4) & 1) | 1);
    store<u32>(fm + 16, 0);
    store<u32>(fm + 20, sly4::post::g::fmap_compare);
    store<u32>(part, 0);
    const u32 w12 = load<u32>(part + 12) & 0x0FFFFFFFu;
    const u32 w8 = 1u << 1;  // (+8 & 0xE) with bits 28..30 replaced by 1 (rlwimi): 2
    store<u32>(part + 12, w12);
    const u32 t = sly4::effect::g::thermal_shader;
    store<u32>(t, render::shader3::types::kBaseVtable);
    store<u32>(part + 8, w8);
    store<u32>(t + 4, g18::kThermalName);
    store<u32>(part + 4, (load<u32>(part + 4) & 1) | 1);
    store<u32>(t + 8, static_cast<u32>(call(ctx, sly4::fn::String_HashNoCase, {g18::kThermalName})));
    call(ctx, g16::fn::PtrVector_Construct, {t + 12});
    store<u16>(t + 18, 4);
    call(ctx, g18::fn::PtrVector_Init, {t + 12, 2, 0, 0});
    store<u32>(t, g18::kThermalTypeVtable);
    ret(ctx, g18::kThermalTypeVtable);
}

// void Frame_Begin_005325D8(bool bind)  [Call_0053BBE0, f_000AD910] (name?)
// The frame's start: sys_ppu_thread_get_id, Gcm_EnterSubContext_0052F780;
// the gamma (g18::g::video +56) clamped to 0.8..1.2 and, when that
// changes it, stored and set (cellVideoOutSetGamma(0, gamma)); the
// shader patch buffer's lowest free (+20 = min(+16, +20)) printed when
// it drops below a new 4 KiB step, +16 = its size - 512 and the side +4
// flipped; RenderState_Reset_0053170C; the target mode (video +184) 1
// with u8 config +34 set, else 3; the display's u8 +136 cleared. With
// `bind`: the display's default viewport (Gcm_SetViewport as
// Draw_SetViewport_00532450, scissor too), its targets by bit 25 of +52,
// config +35 / +34, the mode and u8 +148 bit 7 (Draw_BindTargets), and
// the clear (colour 0, depth / stencil 0xFFFFFF00, stencil mask 255,
// clear all, the state's stencil mask +168 back). r3 as left.
DECOMP(005325D8, Frame_Begin_005325D8) {
    const u32 bind = arg<u32>(ctx, 0);
    Stack frame(ctx, 112);
    const u32 sp = frame.addr() - 112;  // the original's offsets 112..175
    call(ctx, sly4::imp::sys_ppu_thread_get_id, {sp + 128});
    call(ctx, g18::fn::Gcm_EnterSubContext_0052F780);
    const u32 K = g18::kFrameConsts;
    const u32 video = g18::g::video;
    {
        const f64 cur = lfs(video + 56);
        f64 gamma = lfs(K);
        if (!(cur < gamma)) {
            gamma = cur;
            if (gamma > lfs(K + 20)) gamma = lfs(K + 20);
        }
        if (!(cur == gamma)) {
            store<u32>(video + 56, g11::stfs_bits(gamma));
            callf(ctx, g18::fn::cellVideoOutSetGamma, {0}, {gamma});
        }
        ctx.fpr[1] = gamma;  // as left
    }
    {
        const u32 pb = g18::g::patch_buffer;
        s32 low = load<s32>(pb + 16);
        const s32 seen = load<s32>(pb + 20);
        if (seen < low) low = seen;  // cmpw
        if (low / 4096 < seen / 4096)  // srawi / addze
            call(ctx, g17::fn::printf, {g18::kStrPatchBufferFree, static_cast<u64>(static_cast<s64>(low / 1024))});
        const u32 size = load<u32>(pb) - g18::kPatchReserve;
        const u32 side = load<u32>(pb + 4) ^ 1;
        store<s32>(pb + 20, low);
        store<u32>(pb + 16, size);
        store<u32>(pb + 4, side);
    }
    call(ctx, g18::fn::RenderState_Reset_0053170C);
    const u32 cfg = sly4::g::config;
    const u32 mode = load<u8>(cfg + 34) != 0 ? 1u : 3u;
    const u32 d = load<u32>(sly4::disp::g::display);
    store<u32>(video + 184, mode);
    store<u8>(d + 136, 0);
    if ((bind & 0xFF) == 0) return ret(ctx, mode);

    // the viewport (Draw_SetViewport_00532450's sequence, null rects)
    const u32 rects = sp + 136;
    store<u32>(rects, 0);
    store<u32>(rects + 4, 0);
    call(ctx, g11::fn::Surface_DefaultRects_0051AE24, {d, rects, rects + 4});
    {
        const u32 r = load<u32>(rects);
        const u32 w = load<u16>(r + 4), h = load<u16>(r + 6), x = load<u16>(r), y = load<u16>(r + 2);
        const f64 half = lfs(K + 4);
        const f64 sx = sgl(sgl(static_cast<f64>(w)) * half);
        const f64 fh = sgl(static_cast<f64>(h));
        const u32 scale = sp + 144, offset = sp + 160;
        store<u32>(scale, g11::stfs_bits(sx));
        store<u32>(scale + 4, g11::stfs_bits(sgl(fh * lfs(K + 8))));
        store<u32>(scale + 8, 0x3F800000u);
        store<u32>(scale + 12, 0);
        store<u32>(offset + 8, 0);
        store<u32>(offset + 12, 0);
        store<u32>(offset, g11::stfs_bits(sgl(sgl(static_cast<f64>(x)) + sx)));
        store<u32>(offset + 4, g11::stfs_bits(g11::madds(fh, half, sgl(static_cast<f64>(y)))));
        // r8 / r9 are the float arguments' unused GPR slots (r8 holds y, as left).
        callf(ctx, g11::fn::Gcm_SetViewport, {load<u32>(fx::g::state), x, y, w, h, y, ctx.gpr[9], scale, offset},
              {lfs(K + 16), lfs(K + 12)});
        fx::emit({g18::kScissorHorizontal2, (w << 16) | x, (h << 16) | y});
    }
    // the targets
    const u32 flags = load<u32>(d + 52);
    const bool wide = ((flags >> 25) & 1) != 0;  // rlwinm 7,31,31
    const u32 side = (flags >> 28) & 0xC;        // rlwinm 4,28,29: the top two bits * 4
    const bool alt = (load<u8>(d + 148) & 0x80) != 0;  // rlwinm 25,31,31
    const u32 c35 = load<u8>(cfg + 35);
    u32 depth_at;
    if (!wide) depth_at = d + 24;
    else if (c35 == 0 || load<u8>(cfg + 34) == 0) depth_at = alt ? d + 88 : d + 72;
    else depth_at = load<u32>(video + 184) == 1 ? d + 120 : d + 124;
    const u32 depth = load<u32>(depth_at);
    u32 colour_at;
    if (!wide) colour_at = d + side + 16;
    else if (c35 == 0 || load<u8>(cfg + 34) == 0) colour_at = alt ? d + 84 : d + side + 64;
    else colour_at = d + side + (load<u32>(video + 184) == 1 ? 100u : 108u);
    call(ctx, fx::fn::Draw_BindTargets, {depth, load<u32>(colour_at), 0});
    const u32 st = fx::g::state;
    call(ctx, fx::fn::cellGcmSetClearColor, {load<u32>(st), 0});
    call(ctx, fx::fn::cellGcmSetClearDepthStencil, {load<u32>(st), g18::kClearDepthStencil});
    call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(st), 255});
    call(ctx, fx::fn::cellGcmSetClearSurface, {load<u32>(st), g18::kClearAll});
    ret(ctx, call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(st), load<u32>(st + 168)}));
}

namespace {

// DrawItem_DrawDepth's sequence (00538FA0, pool_draw.cpp) for its twins:
// the item's bone palette (item +12 -> +84: u8 count, u8 bone[]) as 3
// vec4 per bone from item +60 into vertex constants 64..; the skinned
// depth draw DrawItem_DrawDepthSkinned(mesh, skin, item, item +60,
// skin_r7, flags) when the mesh has +112 and pool g::skin_depth is set;
// otherwise the mesh's vertex format (+68, data +28), the fragment
// program and the indexed draw (+64 count, +44 index buffer, +76 mode)
// or the array draw of +40 vertices. r3 as the last call leaves it.
void draw_depth(PPUContext& ctx, u32 mesh, u32 item, u32 skin, u32 skin_r7, u32 flags) {
    namespace sp = sly4::render::pool;
    const u32 st = fx::g::state;
    const u32 pal = load<u32>(load<u32>(item + 12) + 84);
    if (pal != 0) {
        // The original copies to its 3248-byte frame at +112 with no bound
        // on the count; the whole 255-bone extent is reserved here.
        Stack frame(ctx, 48 * 255);
        const u32 bones = frame.addr();
        u32 n = ref<u8>(pal);
        if (n != 0) {
            const u32 mats = load<u32>(item + 60);
            for (u32 i = n; i-- > 0;) {
                const u32 src = mats + 48 * ref<u8>(pal + 1 + i);
                for (u32 k = 0; k < 48; k += 4) sp::copy_word(bones + 48 * i + k, src + k);
                ctx.fpr[1] = lfs(src + 44);  // the lfs / stfs copy's f1, as left
            }
            n = ref<u8>(pal);
        }
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 64, 12 * n, bones});
    }
    if (load<u32>(mesh + 112) != 0 && ref<u8>(sp::g::skin_depth) != 0) {
        ctx.gpr[3] = call(ctx, sp::fn::DrawItem_DrawDepthSkinned, {mesh, skin, item, load<u32>(item + 60), skin_r7, flags});
        return;
    }
    sp::bind_format(load<u32>(mesh + 68), load<u32>(mesh + 28));
    const u32 indices = load<u32>(mesh + 64), mode = load<u32>(mesh + 76) & 0xFF;
    if (indices != 0) {
        store<u32>(st + 4, mesh + 44);
        sp::upload_fragment(ctx);
        const u32 ib = load<u32>(st + 4);
        const u32 where = load<u32>(ib + 4);
        ctx.gpr[3] = call(ctx, sp::fn::cellGcmSetDrawIndexArray,
                          {load<u32>(st), mode, indices, load<u32>(ib + 16) & 0xFF, where & 1, where >> 1});
        return;
    }
    const u32 count = load<u32>(mesh + 40);
    sp::upload_fragment(ctx);
    ctx.gpr[3] = call(ctx, sp::fn::cellGcmSetDrawArrays, {load<u32>(st), mode, 0, count});
}

}  // namespace

// void DrawItem_DrawDepth_00538B44(Mesh* mesh, DrawItem* item)
// [called through a pointer] (name?)
// DrawItem_DrawDepth (00538FA0, pool_draw.cpp) with the skinned depth
// draw's flags 0 (draw_depth: DrawItem_DrawDepthSkinned(mesh,
// skin_table, item, item +60, item +28, 0)).
DECOMP(00538B44, DrawItem_DrawDepth_00538B44) {
    namespace sp = sly4::render::pool;
    const u32 mesh = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1);
    draw_depth(ctx, mesh, item, load<u32>(sp::g::skin_table), load<u32>(item + 28), 0);
}

// void DrawItem_DrawBank_005393FC(Mesh* mesh, DrawItem* item, Bank* bank)
// [Bank_DrawItem] (name?)
// draw_depth with the bank's skin table: DrawItem_DrawDepthSkinned(mesh,
// bank +1280 + u8 bank +652 * 128, item, item +60, 0, 0xC0000000).
DECOMP(005393FC, DrawItem_DrawBank_005393FC) {
    const u32 mesh = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1), bank = arg<u32>(ctx, 2);
    draw_depth(ctx, mesh, item, bank + 1280 + (static_cast<u32>(load<u8>(bank + 652)) << 7), 0, 0xC0000000u);
}

namespace {

// An lwarx / stwcx. exchange: v stored atomically, the old word returned.
u32 atomic_swap(u32 ea, u32 v) {
    return __builtin_bswap32(std::atomic_ref<u32>(ref<u32>(ea)).exchange(__builtin_bswap32(v)));
}

}  // namespace

// u32 Outline_Init_0050D2E0()  [Graphics_Init] (name?)
// Registers the outline chunk handlers (g18::kOutlineChunks); builds the
// draw-record list (g16::g::record_list) on first use (u8 +0 = 1, its
// PtrVector, u16 +10 = 160, +32 = 0, +128 / +256 atomically 0; the
// guard); a material for the outline shader (Shader_FindByHash
// 0xF23CD1AE, Material_Alloc, +16); a 512-byte stream buffer
// (Mem_NewAligned, align 128): its two parts' flags as the job part's
// (gfx18 RenderState_Construct_0054E7D8), +160 / +172 = n * 16, +164 =
// n * 2, +168 = 8 (n = config +492 or 1), +176..+196 = 0, +384 = +256,
// stride 8, bit 15 of +124 set, the vertex format copied from
// g18::kOutlineFormat with its attribute mask (+126), and both parts
// allocated (Buffer_Alloc52F198(part, +160, 128, 1, 2)); then the list's
// +28 = the buffer (or 0), +24 = a new 256-entry PtrVector (Mem_Alloc
// 20; quirk: a failed allocation still gets its u16 +8 cleared, at
// address 8), +32 = the material, u16 +12 = 0, and +128 atomically 0.
// Returns +128's old value (the lwarx).
DECOMP(0050D2E0, Outline_Init_0050D2E0) {
    for (const g18::ChunkReg& c : g18::kOutlineChunks) call(ctx, g12::fn::Iff_RegisterChunk, {c.tag, c.opd, 0, 0, 0});
    const u32 L = g16::g::record_list;
    if (load<u8>(g16::g::record_list_ready) == 0) {
        store<u8>(L, 1);
        call(ctx, g16::fn::PtrVector_Construct, {L + 4});
        store<u16>(L + 10, g16::g::kRecordListCapacity);
        store<u32>(L + 32, 0);
        atomic_swap(L + 128, 0);
        atomic_swap(L + 256, 0);
        store<u8>(g16::g::record_list_ready, 1);
    }
    const u32 shader = static_cast<u32>(call(ctx, g15::fn::Shader_FindByHash, {g18::kOutlineShaderHash}));
    const u32 mat = static_cast<u32>(call(ctx, g15::fn::Material_Alloc));
    store<u32>(mat + 16, shader);
    const u32 c = load<u32>(sly4::g::config + g18::kCfgOutlineCount);
    const u32 n = c != 0 ? c : 1u;
    const u32 buf = static_cast<u32>(call(ctx, g18::fn::Mem_NewAligned, {g18::kOutlineBufferSize, g18::kOutlineBufferAlign}));
    if (buf != 0) {
        for (u32 k = 0; k < 2; ++k) {
            const u32 part = buf + g16::kSbVertexData + 16 * k;
            const u32 w4 = (load<u32>(part + 4) & 1) | 1;
            const u32 w8 = 1u << 1;  // (+8 & 0xE) with bits 28..30 replaced by 1 (rlwimi): 2
            const u32 w12 = load<u32>(part + 12) & 0x0FFFFFFFu;
            store<u32>(part, 0);
            store<u32>(part + 8, w8);
            store<u32>(part + 4, w4);
            store<u32>(part + 12, w12);
        }
        const u32 size = n << 4;
        store<u32>(buf + 160, size);
        store<u32>(buf + 164, n << 1);
        store<u32>(buf + 168, 8);
        store<u32>(buf + 172, size);
        store<u32>(buf + 176, 0);
        const u32 m124 = load<u16>(buf + 124);
        for (u32 k = 180; k <= 196; k += 4) store<u32>(buf + k, 0);
        store<u32>(buf + 384, buf + 256);
        store<u16>(buf + 120, 8);
        store<u16>(buf + 124, static_cast<u16>(m124 | 0x8000));
        store<u16>(buf + 126, 0);
        u32 t = g18::kOutlineFormat, d = buf;
        auto copy_attr = [](u32 dst, u32 src) {
            store<u32>(dst, load<u32>(src));
            store<u32>(dst + 4, load<u32>(src + 4));
            for (u32 q = 8; q <= 10; ++q) store<u8>(dst + q, load<u8>(src + q));
        };
        while (load<u32>(t + 4) != 0) {
            copy_attr(d, t);
            const u32 shift = (load<u32>(t) + load<u8>(t + 9)) & 63;  // slw
            const u32 bit = shift < 32 ? 1u << shift : 0u;
            store<u16>(buf + 126, static_cast<u16>(load<u16>(buf + 126) | bit));
            t += 12;
            d += 12;
        }
        copy_attr(d, t);  // the terminator
        for (u32 k = 0; k < 2; ++k)
            call(ctx, g12::fn::Buffer_Alloc52F198, {buf + g16::kSbVertexData + 16 * k, load<u32>(buf + 160), 128, 1, 2});
    }
    store<u32>(L + 28, buf);
    const u32 vec = static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {20, 0, 16}));
    if (vec != 0) {
        call(ctx, g16::fn::PtrVector_Construct, {vec});
        store<u16>(vec + 6, 4);
        call(ctx, g18::fn::PtrVector_Init, {vec, 256, 0, 1});
    }
    store<u32>(L + 24, vec);
    store<u32>(L + 32, mat);
    store<u16>(L + 12, 0);
    store<u16>(vec + 8, 0);  // quirk: also when the allocation failed
    ret(ctx, atomic_swap(L + 128, 0));
}

// bool Iff_ReadGDEM_0054A490(IffReader* r, Obj* o)  [the 'GDEM' chunk
// handler RegisterGDEM registers (OPD 0x00F3BEB0)] (name?)
// Reads a u32 count n and builds o +112 = {+0 n 128-byte records
// (f_00731620(n * 128, 128)), +4 u32[n] (Mem_Alloc), +8 n} (a 12-byte
// Mem_Alloc). Per record: fourteen u16s (+20, +22, +32..+54), a u32 size
// s, u32s +60 / +64 and its first 16 bytes; then one pool-6 block
// (Mem_AllocPool(16, total, 6, 3)) read whole, its parts at +16 (+20 +
// +22 halves), +24 (+32..+36), +28 (+38..+42; each rounded up to 16)
// and +56 (+52 + +54 more). With s: a 16-byte buffer part (Mem_Alloc;
// flags as the job part's, size s in bits 4..31 of +8, location 3 in
// the top nibble of +12; quirk: a failed allocation still has these and
// the next stores made at address 0), its data from pool 4
// (Mem_AllocPool(16, s & 0x0FFFFFFF, 4, 3)), +4 = its RSX offset << 1
// (cellGcmAddressToOffset) and read in; the part (or 0) goes in +4[i].
// Each record's +68 = u16 (o +16) +116. Returns 1.
DECOMP(0054A490, Iff_ReadGDEM_0054A490) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 count_at = frame.addr(), size_at = count_at + 4, offset_at = count_at + 8;  // the original's sp+112 / +116 / +120
    store<u32>(count_at, 0);
    call(ctx, g10::fn::Iff_ReadU32, {r, count_at, 0});
    store<u32>(o + 112, static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {12, 0, 16})));
    {
        const u32 recs = static_cast<u32>(call(ctx, g18::fn::Mem_NewAligned_00731620, {load<u32>(count_at) << 7, 128}));
        store<u32>(load<u32>(o + 112), recs);
    }
    {
        const u32 parts = static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {load<u32>(count_at) << 2, 0, 16}));
        store<u32>(load<u32>(o + 112) + 4, parts);
    }
    store<u32>(load<u32>(o + 112) + 8, load<u32>(count_at));
    for (u32 i = 0; i < load<u32>(load<u32>(o + 112) + 8); ++i) {  // cmplw
        const u32 rec = load<u32>(load<u32>(o + 112)) + 128 * i;
        static constexpr u32 kHalves[14] = {20, 22, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54};
        for (u32 off : kHalves) call(ctx, g11::fn::Iff_ReadU16, {r, rec + off, 0});
        call(ctx, g10::fn::Iff_ReadU32, {r, size_at, 0});
        call(ctx, g10::fn::Iff_ReadU32, {r, rec + 60, 0});
        call(ctx, g10::fn::Iff_ReadU32, {r, rec + 64, 0});
        call(ctx, g15::fn::FileReader_Read, {r, rec, 16});
        auto h = [&](u32 off) { return static_cast<u32>(load<u16>(rec + off)); };
        const u32 a = (h(20) + h(22) + 15) & ~15u;
        const u32 b = (h(32) + h(34) + h(36) + 15) & ~15u;
        const u32 c = (h(38) + h(40) + h(42) + 15) & ~15u;
        const u32 total = a + b + c + (h(52) + h(54));
        const u32 data = static_cast<u32>(call(ctx, g15::fn::Mem_AllocPool, {16, total, 6, 3}));
        call(ctx, g15::fn::FileReader_Read, {r, data, total});
        store<u32>(rec + 16, data);
        store<u32>(rec + 24, data + a);
        store<u32>(rec + 28, data + a + b);
        store<u32>(rec + 56, data + a + b + c);
        u32 part = 0;
        const u32 s = load<u32>(size_at);
        if (s != 0) {
            const u32 p = static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {16, 0, 16}));
            if (p != 0) {
                part = p;
                const u32 w4 = (load<u32>(p + 4) & 1) | 1;
                const u32 w12 = load<u32>(p + 12) & 0x0FFFFFFFu;
                store<u32>(p, 0);
                store<u32>(p + 4, w4);
                store<u32>(p + 8, 1u << 1);  // (+8 & 0xE) with bits 28..30 replaced by 1 (rlwimi): 2
                store<u32>(p + 12, w12);
            }
            const u32 size = load<u32>(size_at);
            const u32 w8 = (size << 4) | (load<u32>(part + 8) & 0xF);              // rlwimi 4,0,27
            const u32 w12 = (3u << 28) | (load<u32>(part + 12) & 0x0FFFFFFFu);     // rlwimi 28,0,3
            store<u32>(part + 8, w8);
            store<u32>(part + 12, w12);
            const u32 mem = static_cast<u32>(call(ctx, g15::fn::Mem_AllocPool, {16, size & 0x0FFFFFFFu, 4, 3}));
            store<u32>(part, mem);
            store<u32>(offset_at, 0);
            call(ctx, fx::imp::cellGcmAddressToOffset, {mem, offset_at});
            const u32 dst = load<u32>(part);
            store<u32>(part + 4, load<u32>(offset_at) << 1);
            call(ctx, g15::fn::FileReader_Read, {r, dst, load<u32>(size_at)});
        }
        store<u32>(load<u32>(load<u32>(o + 112) + 4) + 4 * i, part);
        store<u32>(rec + 68, load<u16>(load<u32>(o + 16) + 116));
    }
    ret(ctx, 1u);
}

namespace {

// ApeTListAllocator<T>::GetAllocator() inlined: the allocator's (alloc)
// free list +0, made on first use (a 60-byte ApeTFreeList from `make`'s
// allocation: +4..+52 cleared in the original's order, bit 0x8000 of u16
// +54 cleared, +0 vtable vt, +56 the signature name; FreeList_Init(list,
// elem, the allocator's heap +4, 0, 0)) and stored back. A failed
// allocation stores and returns 0.
u32 get_free_list(PPUContext& ctx, u32 alloc, u32 vt, u32 name, u32 elem, bool mem_new) {
    u32 list = load<u32>(alloc);
    if (list != 0) return list;
    const u32 p = static_cast<u32>(mem_new ? call(ctx, g18::fn::Mem_New, {60}) : call(ctx, g15::fn::Mem_Alloc, {60, 0, 16}));
    if (p != 0) {
        const u32 heap = load<u32>(alloc + 4);
        for (u32 k : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(p + k, 0);
        const u32 h54 = load<u16>(p + 54) & 0x7FFF;  // rlwimi 0,16,16 of 0: bit 0x8000 cleared
        for (u32 k : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(p + k, 0);
        store<u16>(p + 52, 0);
        store<u16>(p + 54, static_cast<u16>(h54));
        store<u32>(p, vt);
        store<u32>(p + 56, name);
        call(ctx, g15::fn::FreeList_Init, {p, elem, heap, 0, 0});
        list = p;
    }
    store<u32>(alloc, list);
    return list;
}

}  // namespace

// void Debug_Draw_0056AEFC()  [View_EndDraw_00522140] (name?)
// The debug primitives (g18::g::debug_prims' list; nothing without the
// allocator) drawn into the default frame buffer (render::g::state_cache)
// when the current View is the main one (u8 +48 == 2, +49 == 0, bit 2 of
// +256): the list made on first use (ApeTListAllocator<ApeDebug::Prim>
// inlined, get_free_list: 60 bytes from Mem_New, FreeList_Init(list,
// 124, heap, 0, 0); a failed allocation leaves 0 and the walk then
// reads address 8); for
// each prim, kind 7 a line (Debug_DrawLine), kind 8 a rect
// (Debug_DrawRect), kind 3 text (the others, 9 and up too, skipped):
// the debug draw state and font texture bound before the first text
// after a line / rect (Draw_SetState,
// Draw_SetTexture), then, when the buffer has room for strlen * 6 more
// words and a command, DebugFont_Draw_00565550(font, buffer, {x, y,
// 1}, text, &colour; f1 size). Lines, rects and texts become kind 9
// (done) unless config +34 is set with video mode 1. Then
// Stream_PutByteCmd_005234DC(buffer, 0).
// Leftover registers: View_EndDraw_00522140 reads r4 (see docs/DECOMP.md);
// it is set on every exit as the original leaves it.
DECOMP(0056AEFC, Debug_Draw_0056AEFC) {
    namespace rg = sly4::render::g;
    if (load<u32>(g18::g::debug_prims) == 0) return ret(ctx, 0u);  // r4 untouched (the caller's)
    const u32 view = static_cast<u32>(call(ctx, g15::fn::View_GetCurrent));
    {
        const u32 mode = load<u8>(view + 48);
        if (mode != 2) {
            ctx.gpr[4] = mode;  // as left: the lbz
            return ret(ctx, view);
        }
        const u32 sub = load<u8>(view + 49);
        if (sub != 0) {
            ctx.gpr[4] = sub;
            return ret(ctx, view);
        }
        const u32 bit = load<u32>(view + 256) & 4;  // rlwinm 0,29,29
        if (bit == 0) {
            ctx.gpr[4] = 0;
            return ret(ctx, 0u);
        }
    }
    const bool retire = !(load<u8>(sly4::g::config + 34) != 0 && load<u32>(g18::g::video + 184) == 1);
    const u32 alloc = load<u32>(g18::g::debug_prims);
    const u32 list = get_free_list(ctx, alloc, g18::g::vt_debug_prim_list, g18::g::str_debug_prim_list, g18::kPrimSize, true);
    const u32 first = load<u32>(list + 8);  // quirk: address 8 without a list
    u32 prim = first != 0 ? first + g18::kPrimNode : 0u;
    bool need_state = true;
    while (prim != 0) {
        const u32 nx = load<u32>(prim - g18::kPrimNode + 4);
        const u32 next = nx != 0 ? nx + g18::kPrimNode : 0u;
        const s32 kind = load<s32>(prim);
        if (kind > 6) {
            if (kind == static_cast<s32>(g18::kPrimLine)) {
                call(ctx, g18::fn::Debug_DrawLine, {prim});
            } else if (kind == static_cast<s32>(g18::kPrimRect)) {
                call(ctx, g18::fn::Debug_DrawRect, {prim});
            } else {  // done (9) and above
                prim = next;
                continue;
            }
            if (retire) store<u32>(prim, g18::kPrimDone);
            need_state = true;
        } else if (kind == static_cast<s32>(g18::kPrimText)) {
            const u32 font_tex = sly4::debug::kDebugState + 4;  // the DebugFont*
            if (need_state) {
                const u32 fb = load<u32>(rg::state_cache);
                call(ctx, g15::fn::Draw_SetState, {fb, g18::g::debug_draw_state});
                call(ctx, g18::fn::Draw_SetTexture, {fb, load<u32>(load<u32>(font_tex) + 8)});
                need_state = false;
            }
            const u32 text = prim + 28;
            const u32 len = static_cast<u32>(call(ctx, g18::fn::strlen, {text}));
            const u32 fb = load<u32>(rg::state_cache);
            const u32 font = load<u32>(font_tex);
            // room: +72 + 6 words per character below +52, and a command's
            // 259 bytes from the cursor +16 (wrapping to +0 when the read
            // position +12 allows)
            const bool words_fit = load<u32>(fb + 72) + 6 * len < load<u32>(fb + 52);  // cmplw
            const u32 end = load<u32>(fb + 4), cur = load<u32>(fb + 16), rd = load<u32>(fb + 12);
            bool room = true;
            if (cur + 259 > end) room = !(rd > cur) && rd != load<u32>(fb);
            const u32 at = (cur + 3) & ~3u;
            const bool clear = !(at < rd) || at + 256 < rd;  // cmplw
            if (words_fit && room && clear) {
                Stack frame(ctx, 16);  // the original's sp+112: {x, y, 1}
                const u32 pos = frame.addr();
                store<u32>(pos, load<u32>(prim + 12));  // lfs / stfs: raw
                store<u32>(pos + 4, load<u32>(prim + 16));
                store<u32>(pos + 8, 0x3F800000u);
                // r8 holds the colour pointer too (as left)
                callf(ctx, g18::fn::DebugFont_Draw_00565550, {font, fb, pos, text, prim + 4, prim + 4},
                      {lfs(prim + 24), lfs(prim + 16)});
            }
            if (retire) store<u32>(prim, g18::kPrimDone);
        }
        prim = next;
    }
    const u32 fb = load<u32>(rg::state_cache);
    const bool same = load<u8>(fb + 200) == 0;  // Stream_PutByteCmd_005234DC returns early
    call(ctx, g18::fn::Stream_PutByteCmd_005234DC, {fb, 0});
    // r4 as the original 005234DC leaves it: its argument when the byte is
    // already 0, else the cursor +16 it advanced.
    ctx.gpr[4] = same ? 0u : load<u32>(fb + 16);  // r3 as that call leaves it (fb)
}

// void Obj23D50_FitBoxes_0052C6B8(Obj23D50* o)  [Obj23D50_Resolve_0052CC88] (name?)
// A look-at matrix L from 0 along o's direction (+32..+40; the up vector
// gfx11 axis_b60, or axis_b50 when ||d . up| - 1| < 1e-5; Matrix_LookAt),
// and t' = R^T (0 - L row 3) through L's transposed rotation R (vmrghw /
// vperm; vmaddfp order). Then for each of the u8 +20 entries (+4 [i]),
// each 16-byte record from its +416 up to its +512 pointer: the object's
// (+0) box (+208 -> +16: min, max) corners through L (rows_combine, 8
// corners by bits 0..2 picking min x / y / z) give a box (fsel min /
// max from FLT_MAX / -FLT_MAX), and its matrix (+80) times L^T with t'
// as row 3: Obj23D50_FitBox_0052BC9C(o, box, matrix).
DECOMP(0052C6B8, Obj23D50_FitBoxes_0052C6B8) {
    namespace sp = sly4::render::pool;
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 208);
    const u32 sp1 = frame.addr() - 112;  // the original's offsets 112..319
    const u32 dir = sp1 + 112, upv = sp1 + 128, L = sp1 + 192, box = sp1 + 160, mout = sp1 + 256;
    const u32 K = g18::kFitConsts;
    u32 up = g11::g::axis_b60;
    {
        const f64 d1 = lfs(o + 36), d0 = lfs(o + 32), d2 = lfs(o + 40);
        store<u32>(dir, load<u32>(o + 32));  // lfs / stfs: raw
        store<u32>(dir + 4, load<u32>(o + 36));
        store<u32>(dir + 8, load<u32>(o + 40));
        const f64 dot = g11::madds(d2, lfs(up + 8), g11::madds(d0, lfs(up), sgl(d1 * lfs(up + 4))));
        const f64 off = std::fabs(sgl(std::fabs(dot) - lfs(K)));
        if (off < lfs(K + 4)) up = g11::g::axis_b50;
    }
    for (u32 k = 0; k < 12; k += 4) store<u32>(upv + k, load<u32>(up + k));
    ctx.fpr[1] = lfs(up + 8);  // as left: the copy's last lfs
    call(ctx, g18::fn::Matrix_LookAt, {L, g18::kZeroVec, dir, upv});
    auto lane = [&](u32 row, u32 k) { return load<f32>(L + 16 * row + 4 * k); };
    // t' = R^T (0 - t): columns 0..2 of L's rotation, each lane in vmaddfp order
    f32 tn[3];
    {
        f32 nt[3];
        for (u32 k = 0; k < 3; ++k) nt[k] = 0.0f - lane(3, k);  // vsubfp
        for (u32 k = 0; k < 3; ++k) {
            f32 v = std::fmaf(lane(k, 0), nt[0], 0.0f);
            v = std::fmaf(lane(k, 1), nt[1], v);
            tn[k] = std::fmaf(lane(k, 2), nt[2], v);
        }
    }
    for (u32 i = 0; i < load<u8>(o + 20); i = (i + 1) & 0xFF) {  // cmpw; re-read after each entry
        const u32 e = load<u32>(o + 4 + 4 * i);
        const u32 start = load<u32>(e + 416), end = load<u32>(e + 512);
        const u32 diff = end - start;
        if ((diff >> 4) == 0) continue;
        const u32 stop = start + (diff & ~15u);
        for (u32 rec = start; rec < stop; rec += 16) {  // cmplw
            const u32 obj = load<u32>(rec);
            const u32 b = load<u32>(obj + 208) + 16;
            f64 mn[3], mx[3];
            for (u32 k = 0; k < 3; ++k) {
                mn[k] = lfs(K + 48);
                mx[k] = lfs(K + 52);
            }
            for (u32 c = 0; c < 8; ++c) {
                f32 in[3], out[4];
                for (u32 k = 0; k < 3; ++k) in[k] = load<f32>(b + 4 * k + (((c >> k) & 1) != 0 ? 0u : 16u));
                rows_combine(L, in, true, out);
                for (u32 k = 0; k < 3; ++k) {
                    const f64 p = g10::lfs_value(std::bit_cast<u32>(out[k]));
                    mn[k] = sp::sel_ge(sgl(mn[k] - p), p, mn[k]);
                    mx[k] = sp::sel_ge(sgl(mx[k] - p), mx[k], p);
                }
            }
            for (u32 k = 0; k < 3; ++k) store<u32>(box + 4 * k, g11::stfs_bits(mn[k]));
            for (u32 k = 0; k < 3; ++k) store<u32>(box + 16 + 4 * k, g11::stfs_bits(mx[k]));
            ctx.fpr[1] = mn[0];  // as left
            const u32 m = obj + 80;
            f32 rows[4][4];
            for (u32 j = 0; j < 3; ++j) {
                const f32 in[3] = {lane(0, j), lane(1, j), lane(2, j)};
                rows_combine(m, in, false, rows[j]);
            }
            rows_combine(m, tn, true, rows[3]);
            for (u32 j = 0; j < 4; ++j)
                for (u32 k = 0; k < 4; ++k) store<f32>(mout + 16 * j + 4 * k, rows[j][k]);
            call(ctx, g18::fn::Obj23D50_FitBox_0052BC9C, {o, box, mout});
        }
    }
    ret(ctx, static_cast<u32>(load<u8>(o + 20)));  // r3 as left: the count's last lbz
}

// Mesh* Mesh_CreateSphere_0051B788(Material* m, s32 rings, s32 segments)
// [QuerySlot_Acquire_00527E64] (name?)
// A new AglSubMesh (the free list *g12::g::submesh_list, virtual slot 3,
// SubMesh_Construct5386DC; quirk: no allocator leaves 0 and the stores
// below go to low memory) named "Sphere" with format 0
// (MeshFormat_Get_0051B5C0); with n = rings * (segments + 1) * 2
// vertices a buffer part (+24 [u8 +109++]) of n * stride bytes
// (Buffer_Alloc52F198(part, bytes, 128, 0, 2)); +40 = n, +76 = triangle
// strip; the material set (+16; u16 +4 refs taken, the old one's
// release, virtual slot 2); then part 0's bits 1..3 of +8 = 2 and, with
// data (+0), for each ring i and column j (0..segments) the strip's two
// vertices at polar angles i * pi / rings and (i + 1) * pi / rings,
// azimuth j * 2 pi / segments: {sin p sin a, cos p, sin p cos a}, colour
// 255 x4, uv = xy (Math_Sinf / Math_Cosf, each evaluated as the
// original calls them). Returns the mesh.
DECOMP(0051B788, Mesh_CreateSphere_0051B788) {
    const u32 mat = arg<u32>(ctx, 0);
    const s32 rings = arg<s32>(ctx, 1), segs = arg<s32>(ctx, 2);
    const u32 n = (static_cast<u32>(rings * segs) + static_cast<u32>(rings)) << 1;
    const u32 alloc = load<u32>(g12::g::submesh_list);
    u32 mesh = 0;
    if (alloc != 0) {
        const u32 fl = get_free_list(ctx, alloc, g12::g::vt_submesh_list, g12::g::str_submesh_list, 116, false);
        const u32 e = static_cast<u32>(fx::vcall(ctx, fl, 12, {fl}));
        if (e != 0) {
            call(ctx, g12::fn::SubMesh_Construct5386DC, {e});
            mesh = e;
        }
    }
    store<u32>(mesh + 20, g18::kStrSphere);
    const u32 fmt = static_cast<u32>(call(ctx, g18::fn::MeshFormat_Get_0051B5C0, {0}));
    store<u32>(mesh + 68, fmt);
    if (n != 0) {
        const u32 k = load<u8>(mesh + 109);
        store<u8>(mesh + 109, static_cast<u8>(k + 1));
        call(ctx, g12::fn::Buffer_Alloc52F198, {mesh + 24 + (k << 4), n * load<u16>(fmt + 120), 128, 0, 2});
    }
    store<u32>(mesh + 40, n);
    const u32 old = load<u32>(mesh + 16);
    store<u32>(mesh + 76, g18::kPrimTriangleStrip);
    if (old != mat) {
        store<u32>(mesh + 16, mat);
        if (mat != 0) store<u16>(mat + 4, static_cast<u16>(load<u16>(mat + 4) + 1));
        if (old != 0) fx::vcall(ctx, old, 8, {old});
    }
    const u32 K = g18::kSphereConsts;
    const f64 dp = sgl(lfs(K) / sgl(static_cast<f64>(rings)));
    const f64 da = sgl(lfs(K + 4) / sgl(static_cast<f64>(segs)));
    ctx.fpr[1] = sgl(static_cast<f64>(segs));  // as left
    const u32 part = mesh + 24;
    store<u32>(part + 8, (load<u32>(part + 8) & ~0xEu) | (2u << 1));  // rlwimi 1,28,30
    u32 v = load<u32>(part);
    if (v == 0 || rings <= 0) return ret(ctx, mesh);
    auto sinf_ = [&](f64 x) { return callf(ctx, g18::fn::Math_Sinf, {}, {x}); };
    auto cosf_ = [&](f64 x) { return callf(ctx, g18::fn::Math_Cosf, {}, {x}); };
    const f64 one = lfs(K + 8);
    for (s32 i = 0; i < rings; ++i) {
        const f64 fi = sgl(static_cast<f64>(i));
        const f64 p0 = sgl(fi * dp);
        const f64 s0 = sinf_(p0), c0 = cosf_(p0);
        const f64 p1 = sgl(sgl(fi + one) * dp);
        const f64 s1 = sinf_(p1), c1 = cosf_(p1);
        for (s32 j = 0; j < segs + 1; ++j) {
            const f64 a = sgl(sgl(static_cast<f64>(j)) * da);
            const f64 x0 = sgl(s0 * sinf_(a));
            const f64 z0 = sgl(s0 * cosf_(a));
            const f64 x1 = sgl(s1 * sinf_(a));
            const f64 z1 = sgl(s1 * cosf_(a));
            ctx.fpr[1] = z1;  // as left
            const u32 w = v + g18::kSpriteVertexSize;
            store<u32>(v, g11::stfs_bits(x0));
            store<u32>(v + 4, g11::stfs_bits(c0));
            store<u32>(v + 8, g11::stfs_bits(z0));
            for (u32 q = 12; q < 16; ++q) store<u8>(v + q, 255);
            store<u32>(v + 16, g11::stfs_bits(x0));
            store<u32>(v + 20, g11::stfs_bits(c0));
            for (u32 q = 12; q < 16; ++q) store<u8>(w + q, 255);
            store<u32>(w, g11::stfs_bits(x1));
            store<u32>(w + 4, g11::stfs_bits(c1));
            store<u32>(w + 8, g11::stfs_bits(z1));
            store<u32>(w + 16, g11::stfs_bits(x1));
            store<u32>(w + 20, g11::stfs_bits(c1));
            v = w + g18::kSpriteVertexSize;
        }
        if (segs + 1 <= 0) ctx.fpr[1] = c1;  // as left: the ring's last Math_Cosf
    }
    ret(ctx, mesh);
}
