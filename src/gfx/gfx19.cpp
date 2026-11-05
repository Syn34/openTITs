// The renderer range's next recompiled functions (gfx19). Names carry the
// address where the role is unclear.
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>

#include "fx.h"
#include "gfx19.h"
#include "effect_init.h"
#include "post.h"
#include "pool.h"
#include "layers4.h"
#include "shader3.h"

using namespace sly4;

namespace {

namespace g10 = sly4::render::gfx10;
namespace g11 = sly4::render::gfx11;
namespace g12 = sly4::render::gfx12;
namespace g13 = sly4::render::gfx13;
namespace g14 = sly4::render::gfx14;
namespace g15 = sly4::render::gfx15;
namespace g16 = sly4::render::gfx16;
namespace g17 = sly4::render::gfx17;
namespace g18 = sly4::render::gfx18;
namespace g19 = sly4::render::gfx19;
using g11::lfs;
using g11::sgl;

// ApeTListAllocator<T>::GetAllocator inlined: the allocator's free list,
// made on first use (60 bytes from Mem_Alloc(60, 0, 16), its fields
// cleared, FreeList_Init(list, elem, heap, 0, 0)); a failed allocation
// stores and returns 0.
u32 get_free_list(PPUContext& ctx, u32 alloc, u32 vt, u32 name, u32 elem) {
    u32 list = load<u32>(alloc);
    if (list != 0) return list;
    const u32 p = static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {60, 0, 16}));
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

// A new AglSubMesh: the free list *g12::g::submesh_list (virtual slot 3)
// and SubMesh_Construct5386DC; 0 without an allocator or entry (the
// callers then store to low memory, as the original does).
u32 new_submesh(PPUContext& ctx) {
    const u32 alloc = load<u32>(g12::g::submesh_list);
    if (alloc == 0) return 0;
    const u32 fl = get_free_list(ctx, alloc, g12::g::vt_submesh_list, g12::g::str_submesh_list, 116);
    const u32 e = static_cast<u32>(fx::vcall(ctx, fl, 12, {fl}));
    if (e == 0) return 0;
    call(ctx, g12::fn::SubMesh_Construct5386DC, {e});
    return e;
}

// The mesh's material (+16) set to mat: the new one's u16 +4 reference
// taken, the old one released (virtual slot 2).
void set_material(PPUContext& ctx, u32 mesh, u32 mat) {
    const u32 old = load<u32>(mesh + 16);
    if (old == mat) return;
    store<u32>(mesh + 16, mat);
    if (mat != 0) store<u16>(mat + 4, static_cast<u16>(load<u16>(mat + 4) + 1));
    if (old != 0) fx::vcall(ctx, old, 8, {old});
}

// A buffer part's (+0 data, +4 bit 0, +8 bits 1..3 the usage) usage set
// (rlwimi 1,28,30); returns its data.
u32 part_set_usage(u32 part, u32 usage) {
    store<u32>(part + 8, (load<u32>(part + 8) & ~0xEu) | ((usage << 1) & 0xEu));
    return load<u32>(part);
}

// An 11-byte vertex element copy (two words, three bytes), as the
// original's lwz / stw and lbz / stb pairs.
void copy_element(u32 dst, u32 src) {
    store<u32>(dst, load<u32>(src));
    store<u32>(dst + 4, load<u32>(src + 4));
    for (u32 k = 8; k < 11; ++k) store<u8>(dst + k, load<u8>(src + k));
}

// The 4x4 rows at it (lvx) through the 4x4 at m: row r = it[r].x * m0 +
// .y * m1 + .z * m2 (+ m3 for row 3, the first vmaddfp's addend), each
// lane fused in the original's order; stored transposed (the vmrghw /
// vmrglw block) as 16 floats at out.
void rows_through_transposed(u32 it, u32 m, u32 out) {
    f32 r[4][4];
    for (u32 row = 0; row < 4; ++row) {
        const u32 a = (it + 16 * row) & ~15u;
        for (u32 k = 0; k < 4; ++k) {
            f32 v = row == 3 ? load<f32>(((m + 48) & ~15u) + 4 * k) : 0.0f;
            v = std::fmaf(load<f32>((m & ~15u) + 4 * k), load<f32>(a), v);
            v = std::fmaf(load<f32>(((m + 16) & ~15u) + 4 * k), load<f32>(a + 4), v);
            v = std::fmaf(load<f32>(((m + 32) & ~15u) + 4 * k), load<f32>(a + 8), v);
            r[row][k] = v;
        }
    }
    for (u32 k = 0; k < 4; ++k)
        for (u32 row = 0; row < 4; ++row) store<f32>(out + 16 * k + 4 * row, r[row][k]);
}

// transpose(item rows * layers4::g::view_proj) as vertex constants 0..3
// (one 17-word SET_TRANSFORM_CONSTANT_LOAD; shader3_draw.cpp's
// upload_item_transform): rows read with lvx, fused per lane in vmaddfp
// order, row 3 adding view_proj's row 3; the cursor advanced first.
void upload_item_transform(u32 item) {
    namespace l4 = sly4::render::layers4;
    const u32 m = l4::g::view_proj;
    f32 r[4][4];
    for (u32 i = 0; i < 4; ++i) {
        const u32 row = (item & ~15u) + 16 * i;
        const f32 x = load<f32>(row), y = load<f32>(row + 4), z = load<f32>(row + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(m + 4 * k), x, i == 3 ? load<f32>(m + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(m + 16 + 4 * k), y, v);
            r[i][k] = std::fmaf(load<f32>(m + 32 + 4 * k), z, v);
        }
    }
    const u32 c = load<u32>(fx::g::state);
    const u32 cur = load<u32>(c + 8);
    store<u32>(c + 8, cur + 72);
    store<u32>(cur, l4::kVertexConst17);
    store<u32>(cur + 4, 0);  // first constant
    for (u32 k = 0; k < 4; ++k)
        for (u32 i = 0; i < 4; ++i) store<f32>(cur + 8 + 16 * k + 4 * i, r[i][k]);  // stvlx / stvrx pairs
}

// Words to the command buffer (render state +0's cursor), then the cursor.
void emit(std::initializer_list<u32> words) {
    const u32 c = load<u32>(fx::g::state);
    const u32 cur = load<u32>(c + 8);
    u32 i = 0;
    for (u32 w : words) store<u32>(cur + 4 * i++, w);
    store<u32>(c + 8, cur + 4 * i);
}

// A colour word with bytes 1 and 3 swapped (ARGB <-> ABGR; the
// rlwinm / rlwimi pair).
u32 swap_rb(u32 c) { return (c & 0xFF00FF00u) | ((c >> 16) & 0xFF) | ((c & 0xFF) << 16); }

// A ref-counted pointer field (u16 +4 refs; at zero virtual slot 1)
// set to p: the new one's refs up, the old one's down.
void set_ref(PPUContext& ctx, u32 field, u32 p) {
    const u32 old = load<u32>(field);
    if (old == p) return;
    store<u32>(field, p);
    if (p != 0) store<u16>(p + 4, static_cast<u16>(load<u16>(p + 4) + 1));
    if (old != 0) {
        const u16 n = static_cast<u16>(load<u16>(old + 4) - 1);
        store<u16>(old + 4, n);
        if (static_cast<s16>(n) <= 0) fx::vcall(ctx, old, 4, {old});
    }
}

}  // namespace

// Mesh* Mesh_CreateCube_0051C1A8(Material* m, bool faces)
// [Debug_Init, Resources_InitBoundsCube] (name?)
// A new AglSubMesh (new_submesh) named "Cube" with format 1
// (MeshFormat_Get_0051B5C0): 8 vertices (a buffer part +24 [u8 +109++],
// Buffer_Alloc52F198(part, 8 * stride, 128, 0, 2); +40 = 8, +60 = 1)
// and an index part (+44) of 36 (faces, triangles) or 24 (edges, lines)
// u16s (+64 the count, +76 the primitive); the material set; both parts'
// usage 2. With vertex data, the corners {+-0.5, +-0.5, +-0.5} (bit 0 of
// the index: -x, 1 -y, 2 -z), colour 255 x4 and uv 0, then the indices from the
// byte table (no check of the index data). Returns the mesh.
DECOMP(0051C1A8, Mesh_CreateCube_0051C1A8) {
    const u32 mat = arg<u32>(ctx, 0);
    const bool faces = (arg<u32>(ctx, 1) & 0xFF) != 0;
    const u32 nidx = faces ? g19::kCubeFaceIndices : g19::kCubeLineIndices;
    const u32 mesh = new_submesh(ctx);
    store<u32>(mesh + 20, g19::kStrCube);
    const u32 fmt = static_cast<u32>(call(ctx, g18::fn::MeshFormat_Get_0051B5C0, {1}));
    store<u32>(mesh + 68, fmt);
    const u32 k = load<u8>(mesh + 109);
    store<u8>(mesh + 109, static_cast<u8>(k + 1));
    call(ctx, g12::fn::Buffer_Alloc52F198, {mesh + 24 + (k << 4), static_cast<u32>(load<u16>(fmt + 120)) << 3, 128, 0, 2});
    store<u32>(mesh + 40, g19::kCubeVertices);
    store<u32>(mesh + 60, 1);
    call(ctx, g12::fn::Buffer_Alloc52F198, {mesh + 44, nidx << 1, 128, 0, 2});
    store<u32>(mesh + 64, nidx);
    store<u32>(mesh + 76, faces ? g19::kPrimTriangles : g19::kPrimLines);
    set_material(ctx, mesh, mat);
    u32 v = part_set_usage(mesh + 24, 2);
    if (v == 0) return ret(ctx, mesh);  // quirk: the indices are then left unwritten
    const u32 K = g19::kCubeConsts;
    for (u32 i = 0; i < g19::kCubeVertices; ++i, v += g18::kSpriteVertexSize) {
        for (u32 c = 0; c < 4; ++c) store<u8>(v + 12 + c, 0xFF);
        for (u32 a = 0; a < 3; ++a) store<u32>(v + 4 * a, load<u32>(K + (((i >> a) & 1) != 0 ? 4u : 0u)));
        store<u32>(v + 16, 0);
        store<u32>(v + 20, 0);
    }
    ctx.fpr[1] = lfs(K + 4);  // as left (corner 7's y)
    ctx.fpr[2] = lfs(K + 4);
    const u32 idx = part_set_usage(mesh + 44, 2);
    const u32 tbl = g19::kCubeIndices + (faces ? g19::kCubeLineIndices : 0u);
    for (u32 i = 0; i < nidx; ++i) store<u16>(idx + 2 * i, load<u8>(tbl + i));
    ret(ctx, mesh);
}

// Mesh* Mesh_CreateSphereIdx_0051BC20(Material* m, s32 rings, s32 segments)
// [Mesh_Default, Debug_Init] (name?)
// Mesh_CreateSphere_0051B788's indexed form: a new AglSubMesh named
// "SphereIdx" with format 1; rings * segments vertices (part +24, only
// when not 0) and 2 * (segments + 1) * (rings - 1) + rings - 2 u16
// indices (part +44, +60 = 1, only when not 0) drawn as a triangle strip;
// the material set, both parts' usage 2. With vertex data (quirk: none
// leaves the indices unwritten): ring i at polar angle (i + 0.5) * pi /
// rings, segment j at azimuth (j + 0.5) * 2 pi / segments: {sin p sin a,
// cos p, sin p cos a}, colour 255 x4, uv = xy. Then each pair of rings a
// strip of (j mod segments) + base, + segments (j = 0..segments), the
// strips joined by 0xFFFF (no check of the index data). Returns the mesh.
DECOMP(0051BC20, Mesh_CreateSphereIdx_0051BC20) {
    const u32 mat = arg<u32>(ctx, 0);
    const s32 rings = arg<s32>(ctx, 1), segs = arg<s32>(ctx, 2);
    const u32 ur = static_cast<u32>(rings), us = static_cast<u32>(segs);
    const u32 nvert = ur * us;
    const u32 nidx = (ur * (us << 1) - (us << 1)) + ((ur - 1) << 1) + ur - 2;
    const u32 mesh = new_submesh(ctx);
    store<u32>(mesh + 20, g19::kStrSphereIdx);
    const u32 fmt = static_cast<u32>(call(ctx, g18::fn::MeshFormat_Get_0051B5C0, {1}));
    store<u32>(mesh + 68, fmt);
    if (nvert != 0) {
        const u32 k = load<u8>(mesh + 109);
        store<u8>(mesh + 109, static_cast<u8>(k + 1));
        call(ctx, g12::fn::Buffer_Alloc52F198, {mesh + 24 + (k << 4), nvert * load<u16>(fmt + 120), 128, 0, 2});
    }
    store<u32>(mesh + 40, nvert);
    if (nidx != 0) {
        store<u32>(mesh + 60, 1);
        call(ctx, g12::fn::Buffer_Alloc52F198, {mesh + 44, nidx << 1, 128, 0, 2});
    }
    store<u32>(mesh + 64, nidx);
    store<u32>(mesh + 76, g18::kPrimTriangleStrip);
    set_material(ctx, mesh, mat);
    const u32 K = g19::kSphereIdxConsts;
    const f64 dp = sgl(lfs(K) / sgl(static_cast<f64>(rings)));
    const f64 da = sgl(lfs(K + 4) / sgl(static_cast<f64>(segs)));
    f64 f1 = sgl(static_cast<f64>(segs));
    u32 v = part_set_usage(mesh + 24, 2);
    if (v == 0) {
        ctx.fpr[1] = f1;  // as left
        return ret(ctx, mesh);
    }
    if (rings > 0) {
        const f64 half = lfs(K + 8);
        const f64 p0 = sgl(dp * half), a0 = sgl(da * half);
        for (s32 i = 0; i < rings; ++i) {
            const f64 p = g11::madds(sgl(static_cast<f64>(i)), dp, p0);
            const f64 sp = callf(ctx, g18::fn::Math_Sinf, {}, {p});
            const f64 cp = callf(ctx, g18::fn::Math_Cosf, {}, {p});
            f1 = cp;
            for (s32 j = 0; j < segs; ++j, v += g18::kSpriteVertexSize) {
                const f64 a = g11::madds(sgl(static_cast<f64>(j)), da, a0);
                const f64 x = sgl(sp * callf(ctx, g18::fn::Math_Sinf, {}, {a}));
                const f64 z = sgl(sp * callf(ctx, g18::fn::Math_Cosf, {}, {a}));
                f1 = z;
                store<u32>(v, g11::stfs_bits(x));
                store<u32>(v + 4, g11::stfs_bits(cp));
                for (u32 c = 0; c < 4; ++c) store<u8>(v + 12 + c, 0xFF);
                store<u32>(v + 16, g11::stfs_bits(x));
                store<u32>(v + 20, g11::stfs_bits(cp));
                store<u32>(v + 8, g11::stfs_bits(z));
            }
        }
    }
    ctx.fpr[1] = f1;  // as left
    u32 q = part_set_usage(mesh + 44, 2);
    s32 base = 0;
    for (s32 i = 0; i < rings - 1; ++i, base += segs) {
        if (i > 0) {
            store<u16>(q, g19::kRestartIndex);
            q += 2;
        }
        for (s32 j = 0; j < segs + 1; ++j, q += 4) {
            const s32 col = segs != 0 ? j % segs : j;  // segs 0: only j = 0
            // the compiler's (offset * 24) / 24, wrapping as the original
            const s32 a = static_cast<s32>(static_cast<u32>(col + base) * 24u) / 24;
            const s32 b = static_cast<s32>(static_cast<u32>(col + base + segs) * 24u) / 24;
            store<u16>(q, static_cast<u16>(a));
            store<u16>(q + 2, static_cast<u16>(b));
        }
    }
    ret(ctx, mesh);
}

// VtxDecl* VtxDecl_Get_005246A8(const u8 (*elements)[12], u32 count, bool b)
// [00524B68] (name?)
// The shared vertex declaration for count source elements: the hash of
// their bytes (Data_Hash31(elements, count * 12)) looked up in the
// AglVtxDecl list's used entries (Obj_GetField4: the first node, +0 the
// next; the decl 16 bytes in, +4 its hash), the list made on first use
// (get_free_list). Not found: a new entry (virtual slot 3; quirk: none
// carries on at address 0) with (count + 1) * 12 bytes of elements
// (Mem_Alloc(.., 0, 16)) and the hash. Each source element converted
// into it (ConvertState_0054A930(decl, dst, src, b); dst advances when
// it returns non-zero), u8 +5 noted as the stride of stream u8 +4 in a
// u16[4] on the stack, then the terminator; stride 0 to +136, +140 |=
// 0x8000, +142 = the mask of 1 << (word +0 + u8 +9) over the elements,
// copied to +16 with the terminator, then strides 1.. up to the highest
// stream used. Returns the decl.
// Quirk: the stride array is indexed by the stream byte unchecked; the
// original's frame (192 bytes: the array at +112, r23..r31 saved at
// +120.., the caller's frame from +192) is reproduced, so streams 4 and
// up land where the original's do, saved registers included (restored
// from the frame on exit as the original's epilogue does; its LR slot
// in the caller's frame, stream 48, is not modelled).
DECOMP(005246A8, VtxDecl_Get_005246A8) {
    const u32 elems = arg<u32>(ctx, 0), count = arg<u32>(ctx, 1);
    const u32 flag = arg<u32>(ctx, 2) & 0xFF;
    Stack frame(ctx, 192);
    const u32 sp = frame.addr();
    for (u32 k = 0; k < 9; ++k) store<u64>(sp + 120 + 8 * k, ctx.gpr[23 + k]);
    auto leave = [&](u32 decl) {
        for (u32 k = 0; k < 9; ++k) ctx.gpr[23 + k] = load<u64>(sp + 120 + 8 * k);
        ret(ctx, decl);
    };
    const u32 hash = static_cast<u32>(call(ctx, g19::fn::Data_Hash31, {elems, count * g19::kDeclElementSize}));
    const u32 L = g19::g::vtxdecl_list;
    auto list = [&] { return get_free_list(ctx, L, g19::g::vt_vtxdecl_list, g19::g::str_vtxdecl_list, g19::kDeclSize); };
    const u32 first = static_cast<u32>(call(ctx, g17::fn::Obj_GetField4, {list()}));
    for (u32 d = first != 0 ? first + 16 : 0; d != 0;) {
        if (load<u32>(d + 4) == hash) return leave(d);
        const u32 next = load<u32>(d - 16);
        d = next != 0 ? next + 16 : 0;
    }
    const u32 fl = list();
    u32 decl = static_cast<u32>(fx::vcall(ctx, fl, 12, {fl}));
    if (decl != 0) {
        const u32 p = static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {(count + 1) * g19::kDeclElementSize, 0, 16}));
        store<u32>(decl, p);
        store<u32>(decl + 4, hash);
    }
    for (u32 k = 0; k < 4; ++k) store<u16>(sp + 112 + 2 * k, 0);
    s32 streams = 0;
    u32 dst = load<u32>(decl);
    for (u32 i = 0, src = elems; i < count; ++i, src += g19::kDeclElementSize) {
        const u32 done = static_cast<u32>(call(ctx, g19::fn::ConvertState_0054A930, {decl, dst, src, flag}));
        const u32 s = load<u8>(src + 4);
        if (done != 0) dst += g19::kDeclElementSize;
        store<u16>(sp + 112 + ((s << 1) & 0x1FE), load<u8>(src + 5));
        streams = std::max(static_cast<s32>(s + 1), streams);
    }
    copy_element(dst, g19::kDeclTerminator);
    const u32 stride0 = load<u16>(sp + 112);
    const u32 flags = load<u16>(decl + g19::kDeclFlags) | 0x8000;
    store<u16>(decl + g19::kDeclStrides, static_cast<u16>(stride0));
    store<u16>(decl + g19::kDeclMask, 0);
    store<u16>(decl + g19::kDeclFlags, static_cast<u16>(flags));
    u32 e = load<u32>(decl), out = decl + g19::kDeclElements;
    for (; load<u32>(e + 4) != 0; e += g19::kDeclElementSize, out += g19::kDeclElementSize) {
        copy_element(out, e);
        const u32 sh = (load<u32>(e) + load<u8>(e + 9)) & 0x3F;  // slw: 32 and up give 0
        const u32 bit = sh < 32 ? 1u << sh : 0u;
        store<u16>(decl + g19::kDeclMask, static_cast<u16>(load<u16>(decl + g19::kDeclMask) | bit));
    }
    copy_element(out, e);
    for (s32 k = streams - 1; k > 0; --k)
        store<u16>(decl + g19::kDeclStrides + 2 * static_cast<u32>(k), load<u16>(sp + 112 + 2 * static_cast<u32>(k)));
    leave(decl);
}

// void RenderState_Destruct_0054E30C()  [static destructor] (name?)
// RenderState_Construct_0054E7D8's counterpart: the transform list
// (g15::g::tr_list: PtrVector_Clear, Nop) when built, its u64 guard
// cleared; each static surface whose u64 guard is set destroyed
// (Surface_Destruct_007881DC(surface, 2), or the target arrays'
// TargetArray_Destruct_*) and its guard cleared, in the original's order
// (kRenderStatics); the thermal shader object back to its base vtable
// and its PtrVector (+12) cleared; the job part's block (+0, unless bit 0
// of +8) freed (Mem_FreePool(p, +4 bit 0 ? 5 : 4, top nibble of +12)) and
// +0 cleared; then the program maps fmap and vmap: data (+8) handed back
// to their allocator (+12, virtual slot 2: (alloc, data, u16 +0 * (u16
// +6 >> 1))), Array_Destruct.
DECOMP(0054E30C, RenderState_Destruct_0054E30C) {
    if (load<u64>(g15::g::tr_list_live) != 0) {
        call(ctx, g15::fn::PtrVector_Clear, {g15::g::tr_list});
        call(ctx, g15::fn::Nop, {g15::g::tr_list});
        store<u64>(g15::g::tr_list_live, 0);
    }
    for (const auto& t : g19::kRenderStatics) {
        if (t.guard == 0) break;
        if (load<u64>(t.guard) == 0) continue;
        if (t.fn != 0) call(ctx, t.fn, {});
        else call(ctx, g19::fn::Surface_Destruct_007881DC, {t.guard + 8, 2});
        store<u64>(t.guard, 0);
    }
    const u32 sh = sly4::effect::g::thermal_shader;
    store<u32>(sh, g19::kThermalShaderVtable);
    call(ctx, g15::fn::PtrVector_Clear, {sh + 12});
    call(ctx, g15::fn::Nop, {sh + 12});
    const u32 jp = g18::kJobPart;
    const u32 blk = load<u32>(jp);
    if (blk != 0 && (load<u32>(jp + 8) & 1) == 0) {
        const u32 pool = (load<u32>(jp + 4) & 1) != 0 ? g15::kPoolTextureAlt : g15::kPoolTexture;
        call(ctx, g15::fn::Mem_FreePool, {blk, pool, load<u32>(jp + 12) >> 28});
        store<u32>(jp, 0);
    }
    for (u32 map : {sly4::post::g::fmap, sly4::post::g::vmap}) {
        const u32 bytes = static_cast<u32>(load<u16>(map)) * ((load<u16>(map + 6) >> 1) & 0x7FFF);
        const u32 data = load<u32>(map + 8);
        if (data != 0) {
            const u32 alloc = load<u32>(map + 12);
            fx::vcall(ctx, alloc, 8, {alloc, data, bytes});
        }
        call(ctx, g19::fn::Array_Destruct, {map});
    }
}

// void Emitter_InitSystem_0050E614()  [Graphics_Init] (name?)
// The particle system's set-up: the AglEmitProperties allocator header
// (g13::g::emit_props_list: 16 bytes from Mem_Alloc(16, 0, 16), {+0 0,
// +4 capacity, +8 0}; config +358, see kCfgEmitPropsCount), the emitter
// chunk handlers (Iff_RegisterChunk(tag, OPD, 0, 0, 0)), then the sprite
// stream buffers (g16::g::stream_buffers; Mem_NewAligned(640, 128), 0 on
// failure): for n sprites (config +344) two vertex parts (+128, 16 bytes
// apart) of 24 n bytes and two index parts (+160, 20 apart; +16 = 1) of
// (6 n / 2) & ~1 bytes, all with +0 0, +4 bit 0 set, +8 usage 1 (the
// other bits cleared), +12's top nibble cleared, then allocated
// (Buffer_Alloc52F198(part, bytes, 128, 1, 2)); +200 / +220 the vertex
// bytes, +204 / +224 the index bytes, +208 n, +212 6 n / 4, +216 stride
// 24, +228..+252 0, +256 1, u16 +120 stride, +124 |= 0x8000, the format
// copied in at +0 with its attribute mask in u16 +126. Then
// layers3::g::atomic_ddc00 atomically cleared; r3 its old value.
DECOMP(0050E614, Emitter_InitSystem_0050E614) {
    const u32 cfg = 0x010C5A20u;  // g::config
    u32 cap = g19::kDefaultEmitProps;
    const s16 h = static_cast<s16>(load<u16>(cfg + g19::kCfgEmitPropsCount));
    if (h != 0) cap = h > 0 ? static_cast<u32>(static_cast<s32>(h)) : 0u;
    u32 hdr = static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {16, 0, 16}));
    if (hdr != 0) {
        store<u32>(hdr, 0);
        store<u32>(hdr + 8, 0);
        store<u32>(hdr + 4, cap);
    }
    store<u32>(render::gfx13::g::emit_props_list, hdr);
    for (const g18::ChunkReg& c : g19::kEmitterChunks) call(ctx, g12::fn::Iff_RegisterChunk, {c.tag, c.opd, 0, 0, 0});
    u32 n = g19::kDefaultSprites;
    const s32 cn = static_cast<s32>(load<u32>(cfg + g19::kCfgSpriteCount));
    if (cn != 0) n = cn > 0 ? static_cast<u32>(cn) : 0u;
    const u32 six = n * 6, quarter = six >> 2;
    const u32 sb = static_cast<u32>(call(ctx, g18::fn::Mem_NewAligned, {g19::kStreamBuffersSize, g19::kStreamBuffersAlign}));
    if (sb != 0) {
        auto reset = [](u32 part, bool index) {
            const u32 w4 = (load<u32>(part + 4) & 1) | 1, w12 = load<u32>(part + 12) & 0x0FFFFFFF;
            store<u32>(part, 0);
            if (index) store<u32>(part + 16, 1);
            store<u32>(part + 8, 1u << 1);  // usage 1; rlwinm 28,30 then rlwimi: nothing else kept
            store<u32>(part + 4, w4);
            store<u32>(part + 12, w12);
        };
        for (u32 k = 0; k < 2; ++k) reset(sb + 128 + 16 * k, false);
        for (u32 k = 0; k < 2; ++k) reset(sb + 160 + 20 * k, true);
        const u32 vbytes = n * 24, ibytes = (six >> 1) & 0x7FFFFFFEu;
        store<u32>(sb + 208, n);
        store<u32>(sb + 204, ibytes);
        store<u32>(sb + 212, quarter);
        store<u32>(sb + 200, vbytes);
        store<u32>(sb + g18::kSbStride, g18::kSpriteVertexSize);
        store<u32>(sb + 220, vbytes);
        store<u32>(sb + 224, ibytes);
        const u32 h124 = load<u16>(sb + 124);
        for (u32 off = 228; off <= 252; off += 4) store<u32>(sb + off, 0);
        store<u32>(sb + 256, 1);
        store<u16>(sb + 120, static_cast<u16>(g18::kSpriteVertexSize));
        store<u16>(sb + 124, static_cast<u16>(h124 | 0x8000));
        store<u16>(sb + 126, 0);
        u32 e = g19::kSpriteFormat, out = sb;
        for (; load<u32>(e + 4) != 0; e += 12, out += 12) {
            copy_element(out, e);
            const u32 sh = (load<u32>(e) + load<u8>(e + 9)) & 0x3F;  // slw: 32 and up give 0
            store<u16>(sb + 126, static_cast<u16>(load<u16>(sb + 126) | (sh < 32 ? 1u << sh : 0u)));
        }
        copy_element(out, e);
        for (u32 k = 0; k < 2; ++k) {
            call(ctx, g12::fn::Buffer_Alloc52F198, {sb + 128 + 16 * k, load<u32>(sb + 200), 128, 1, 2});
            store<u32>(sb + 160 + 20 * k + 16, 1);
            call(ctx, g12::fn::Buffer_Alloc52F198, {sb + 160 + 20 * k, ibytes, 128, 1, 2});
        }
    }
    store<u32>(render::gfx16::g::stream_buffers, sb);
    const u32 old = std::atomic_ref<u32>(ref<u32>(render::gfx14::g::draw_busy)).exchange(0);  // lwarx / stwcx.
    ret(ctx, __builtin_bswap32(old));
}

// void DrawList_DrawDepth_005499D8(?, View* v)  [00549F30] (name?)
// The depth pass over v's draw records (+1024 .. +1152, 64 bytes each, at
// most 512): records whose material (+28) has bit 5 of u8 +100 set. When
// the record's key (+60) differs from the last one, its 4x4 (+0) through
// v's matrix (+48) goes to vertex constants 0..3 transposed
// (cellGcmSetVertexProgramConstants(ctx, 0, 16, m)). A mesh (+12) with
// +112 is drawn skinned (DrawItem_DrawDepthSkinned(mesh, v + 1280 + 128
// * u8 v+652, record, record +44, 0, 0xC0000000)); otherwise its format
// bound (pool::bind_format) and, as DrawItem_DrawDepth, the fragment
// program uploaded and an indexed (+64 count, +44 indices) or array
// (+40 vertices) draw of mode +76.
DECOMP(005499D8, DrawList_DrawDepth_005499D8) {
    namespace sp = render::pool;
    const u32 v = arg<u32>(ctx, 1);
    u32 it = load<u32>(v + 1024);
    const u32 count = (load<u32>(v + 1152) - it) >> 6;
    if (count == 0) return ret(ctx, 0u);  // r3 as left
    const u32 n = count < 512 ? count : 512u;
    const u32 skin = v + 1280 + ((static_cast<u32>(load<u8>(v + 652)) << 7) & 0x7F80);
    const u32 end = it + (n << 6);
    const u32 st = fx::g::state;
    u32 last = 0;
    Stack frame(ctx, 64);
    for (; it < end; it += 64) {
        const u32 mesh = load<u32>(it + 12), mat = load<u32>(it + 28), bones = load<u32>(it + 44);
        const u32 key = load<u32>(it + 60);
        if (((load<u8>(mat + 100) >> 5) & 1) == 0) {
            ctx.gpr[3] = key;  // as left
            continue;
        }
        if (key != last) {
            rows_through_transposed(it, v + 48, frame.addr());
            last = key;
            call(ctx, g16::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 0, 16, frame.addr()});
        }
        if (load<u32>(mesh + 112) != 0) {
            call(ctx, sp::fn::DrawItem_DrawDepthSkinned, {mesh, skin, it, bones, 0, 0xC0000000u});
            continue;
        }
        sp::bind_format(load<u32>(mesh + 68), load<u32>(mesh + 28));
        const u32 indices = load<u32>(mesh + 64), mode = load<u32>(mesh + 76) & 0xFF;
        if (indices != 0) {
            store<u32>(st + 4, mesh + 44);
            sp::upload_fragment(ctx);
            const u32 ib = load<u32>(st + 4);
            const u32 where = load<u32>(ib + 4);
            call(ctx, g16::fn::cellGcmSetDrawIndexArray, {load<u32>(st), mode, indices, load<u32>(ib + 16) & 0xFF, where & 1, where >> 1});
        } else {
            const u32 verts = load<u32>(mesh + 40);
            sp::upload_fragment(ctx);
            call(ctx, g16::fn::cellGcmSetDrawArrays, {load<u32>(st), mode, 0, verts});
        }
    }
}

// u8 DepthPass_Run_00549F30(DepthPass* o)  [Update_0054A420] (name?)
// Waits (sys_timer_usleep(300)) while o+128 is set, then, when any of
// o's u8 +20 views (+4 [i]) has both command blocks (+768 / +896, read
// atomically) or draw records (+1024 .. +1152), renders the depth pass:
// the display's depth target (o+28 -> +672 -> +512, chosen as
// fx::display_depth) bound (Draw_BindTargets(t, 0, 0)), cleared (colour
// 0, depth / stencil 0xFFFFFF00, stencil mask 255, then the state's
// mask, +168), DepthState_Apply_005495D8(o); per view its viewport and
// scissor (u16 +28 x, y, w, h), its virtual slot 5 (view, 0); with both
// blocks the fragment program uploaded (as pool::upload_fragment), a
// call into block +768 (Gcm_SetCall_005E0600) and a jump back to the
// current command position written to block +896's word;
// DrawList_DrawDepth_005499D8(o, view), Draw_InvalidateState. Then
// Draw_ColourOnBoundsOff_00549860, Draw_BindScreenTargets and u8 +96 |=
// 0x80. r3: the new +96, or the views checked when there was nothing.
DECOMP(00549F30, DepthPass_Run_00549F30) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 st = fx::g::state;
    while (load<u32>(o + 128) != 0) syscall(ctx, g19::kSysTimerUsleep, {g19::kDepthPassWaitUs});
    auto atomic_load = [](u32 ea) { return __builtin_bswap32(std::atomic_ref<u32>(ref<u32>(ea)).load()); };  // lwarx
    const u32 nv = load<u8>(o + 20);
    bool any = false;
    u32 i = 0;
    for (; i < nv; i = (i + 1) & 0xFF) {
        const u32 view = load<u32>(o + 4 + 4 * i);
        const u32 begin = load<u32>(view + 1024);
        const u32 a = atomic_load(view + 768), b = atomic_load(view + 896);
        const bool blocks = a != 0 && b != 0;
        const bool records = ((load<u32>(view + 1152) - begin) >> 6) != 0;
        if (blocks || records) {
            any = true;
            break;
        }
    }
    if (!any) return ret(ctx, i);
    const u32 disp = load<u32>(load<u32>(load<u32>(o + 28) + 672) + 512);
    u32 tp;
    if (((load<u32>(disp + 52) >> 25) & 1) == 0) tp = disp + 24;
    else if (load<u8>(sly4::g::config + 35) == 0 || load<u8>(sly4::g::config + 34) == 0)
        tp = disp + (((load<u8>(disp + 148) >> 7) & 1) != 0 ? 88 : 72);
    else tp = disp + (load<u32>(sly4::g::mem_stats + 184) == 1 ? 120 : 124);
    call(ctx, fx::fn::Draw_BindTargets, {load<u32>(tp), 0, 0});
    call(ctx, fx::fn::cellGcmSetClearColor, {load<u32>(st), 0});
    call(ctx, fx::fn::cellGcmSetClearDepthStencil, {load<u32>(st), g18::kClearDepthStencil});
    call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(st), 255});
    call(ctx, fx::fn::cellGcmSetClearSurface, {load<u32>(st), 3});
    call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(st), load<u32>(st + 168)});
    call(ctx, g19::fn::DepthState_Apply_005495D8, {o});
    const u32 K = g19::kDepthPassConsts;
    const f64 half = lfs(K), nhalf = lfs(K + 4);
    Stack frame(ctx, 48);
    const u32 scale = frame.addr(), offset = scale + 16, offs = scale + 32;
    for (u32 k = 0; static_cast<s32>(k) < static_cast<s32>(load<u8>(o + 20)); k = (k + 1) & 0xFF) {
        const u32 view = load<u32>(o + 4 + 4 * k);
        const u32 r = view + 28;
        const u32 x = load<u16>(r), y = load<u16>(r + 2), w = load<u16>(r + 4), h = load<u16>(r + 6);
        const f64 fw = sgl(static_cast<f64>(w)), fh = sgl(static_cast<f64>(h));
        const f64 sx = sgl(fw * half);
        store<u32>(scale, g11::stfs_bits(sx));
        store<u32>(scale + 4, g11::stfs_bits(sgl(fh * nhalf)));
        store<u32>(scale + 8, 0x3F800000u);
        store<u32>(scale + 12, 0);
        store<u32>(offset, g11::stfs_bits(sgl(sgl(static_cast<f64>(x)) + sx)));
        store<u32>(offset + 4, g11::stfs_bits(g11::madds(fh, half, sgl(static_cast<f64>(y)))));
        store<u32>(offset + 8, 0);
        store<u32>(offset + 12, 0);
        // r8 / r9 are the float arguments' unused GPR slots (the context and y, as left).
        callf(ctx, g11::fn::Gcm_SetViewport, {load<u32>(st), x, y, w, h, load<u32>(st), y, scale, offset},
              {lfs(K + 12), lfs(K + 8)});
        const u32 c = load<u32>(st);
        const u32 hx = (static_cast<u32>(load<u16>(r + 4)) << 16) | load<u16>(r);  // re-read, as the original
        const u32 vy = (static_cast<u32>(load<u16>(r + 6)) << 16) | load<u16>(r + 2);
        const u32 cur = load<u32>(c + 8);
        store<u32>(cur, g18::kScissorHorizontal2);
        store<u32>(c + 8, cur + 12);
        store<u32>(cur + 4, hx);
        store<u32>(cur + 8, vy);
        fx::vcall(ctx, view, 20, {view, 0});
        const u32 dirty = load<u32>(st + 92), gc = load<u32>(st), fp = load<u32>(st + 28);
        const u32 jump = atomic_load(view + 896), callee = atomic_load(view + 768);
        if (callee != 0 && jump != 0) {
            if ((dirty >> 31) != 0) {
                store<u32>(st + 92, dirty & 0x7FFFFFFF);
                call(ctx, g15::fn::cellGcmSetFragmentProgram, {gc, fp + 1136, 1});
                call(ctx, fx::fn::FProgram_Upload, {st + 28});
            } else if (fp != 0 && load<u64>(fp + 16) != 0) {
                call(ctx, fx::fn::FProgram_Upload, {st + 28});
            }
            call(ctx, g11::fn::cellGcmAddressToOffset, {callee, offs});
            call(ctx, g10::fn::Gcm_SetCall_005E0600, {load<u32>(st), load<u32>(offs)});
            call(ctx, g11::fn::cellGcmAddressToOffset, {load<u32>(load<u32>(st) + 8), offs + 4});
            store<u32>(jump, load<u32>(offs + 4) | g19::kGcmJump);
        }
        call(ctx, g19::fn::DrawList_DrawDepth_005499D8, {o, view});
        call(ctx, g19::fn::Draw_InvalidateState, {});
    }
    call(ctx, g19::fn::Draw_ColourOnBoundsOff_00549860, {o});
    call(ctx, g15::fn::Draw_BindScreenTargets, {});
    const u32 f96 = load<u8>(o + 96) | 0x80;
    store<u8>(o + 96, static_cast<u8>(f96));
    ret(ctx, f96);
}

// void Mesh_SwapVertices_0052528C(Mesh* mesh)  [Iff_HandleXTVM] (name?)
// The mesh's vertex data (part +24, usage set to 3) byte-swapped in place
// for the +40 vertices (stride u16 format +120): for each of the ten
// attribute keys (g19::kSwapKeys) found in the format +68
// (FindEntry_00537B7C(format, key, 0)), the element at data + u8 +8 by
// its type (+4): float types through Floats_ByteSwap_00525174(n, p,
// comps, stride), 16-bit ones swapped here. r3 as the original leaves it.
DECOMP(0052528C, Mesh_SwapVertices_0052528C) {
    const u32 mesh = arg<u32>(ctx, 0);
    const u32 data = part_set_usage(mesh + 24, 3);
    const u32 fmt = load<u32>(mesh + 68), n = load<u32>(mesh + 40);
    for (u32 k = 0; k < g19::kSwapKeyCount; ++k) {
        const u32 e = static_cast<u32>(call(ctx, g19::fn::FindEntry_00537B7C, {fmt, load<u32>(g19::kSwapKeys + 4 * k), 0}));
        if (e == 0) continue;  // r3 = 0, as left
        const u32 type = load<u32>(e + 4);
        u32 p = load<u8>(e + 8) + data;
        u32 comps = 0;
        bool f32s = false;
        for (const auto& t : g19::kSwapTypes)
            if (t.type == type) comps = t.comps, f32s = t.f32s;
        ctx.gpr[3] = p;  // as left for the types not swapped
        if (comps == 0) continue;
        const u32 stride = load<u16>(fmt + 120);
        if (f32s) {
            call(ctx, g19::fn::Floats_ByteSwap_00525174, {n, p, comps, stride});
            continue;
        }
        for (u32 c = n; c != 0; --c, p += stride)
            for (u32 j = 0; j < comps; ++j)
                store<u16>(p + 2 * j, static_cast<u16>((load<u8>(p + 2 * j + 1) << 8) | load<u8>(p + 2 * j)));
        ctx.gpr[3] = ~0ull;  // the loop counter, as left
    }
}

// The pieces DrawItem_SetupBasic_005D7598 and the stencil setups share.
namespace setup {

namespace s3 = sly4::render::shader3;

// The item matrix (upload_item_transform) and the view fog as
// shader3_draw.cpp's apply_fog: index item u8 +68 bit 6, enabled by
// material +100 bit 6; vertex constant 32 re-sent when render state
// +216 / +220.. differ; fragment constant 32 the fog colour.
void matrix_and_fog(PPUContext& ctx, u32 item) {
    const u32 st = fx::g::state;
    upload_item_transform(item);
    const u32 fog = (load<u8>(item + 68) >> 6) & 1;
    const u32 enable = (load<u8>(load<u32>(item + 28) + 100) >> 6) & 1;
    const u32 colour = s3::g::fog_colours + 16 * fog;
    if (load<u32>(st + 216) != fog || load<u8>(st + 220 + fog) != enable) {
        store<u32>(st + 216, fog);
        store<u8>(st + 220 + fog, static_cast<u8>(enable));
        call(ctx, g16::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 32, 4, s3::g::fog_vconsts + 32 * fog + 16 * enable});
    }
    call(ctx, g17::fn::FProgram_SetConstant, {32, colour});
}

// The layer list as vertex constants 16.. (shader3_draw.cpp's
// upload_layer_list): the material colour +64 tints the second rows.
// The original builds this at sp+128 of a 368-byte frame with no bound
// (more than 3 entries overrun its saved registers); the frame here is
// sized for the count, as shader3_draw.cpp does. f1 as left.
void layer_list(PPUContext& ctx, u32 mat, u32 list) {
    const u32 colour = mat + 64;
    auto tinted = [&](u32 dst, u32 src) {
        for (u32 i = 0; i < 16; i += 4) store<u32>(dst + i, g11::stfs_bits(sgl(lfs(src + i) * lfs(colour + i))));  // fmuls
    };
    auto raw = [](u32 dst, u32 src) {
        for (u32 i = 0; i < 16; i += 4) store<u32>(dst + i, load<u32>(src + i));
    };
    Stack frame(ctx, 48 + 48 * load<u32>(list));
    const u32 v = frame.addr();
    tinted(v, list + 16);
    tinted(v + 16, list + 32);
    raw(v + 32, list + 48);
    u32 dst = v + 48;
    ctx.fpr[1] = lfs(list + 60);  // as left (the last word copied)
    for (u32 n = 0, src = list + 64; n < load<u32>(list); ++n, src += 48, dst += 48) {
        raw(dst, src);
        tinted(dst + 16, src + 16);
        raw(dst + 32, src + 32);
        ctx.fpr[1] = lfs(src + 44);
    }
    const u32 floats = static_cast<u32>(static_cast<s32>(dst - v) / 16) << 2;  // srawi / addze
    call(ctx, g16::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 16, floats, v});
}

// A translucent item's (u8 +71 below 255) state: depth writes off and
// fragment constant `fc` = {0, alpha * inv255, 0, 0} or, unless material
// word +104 bit 31, blending on (src alpha, 1 - src alpha, add) with x =
// 1. f1 / f2 as left.
void translucent_begin(PPUContext& ctx, u32 item, u32 fc_index, u32 inv255) {
    Stack frame(ctx, 16);
    const u32 fc = frame.addr();
    emit({g19::kMethodDepthMask, 0});
    store<u32>(fc, 0);
    store<u32>(fc + 8, 0);
    store<u32>(fc + 12, 0);
    const f64 fa = sgl(static_cast<f64>(load<u8>(item + 71))), a = sgl(fa * lfs(inv255));
    store<u32>(fc + 4, g11::stfs_bits(a));
    ctx.fpr[1] = a;
    ctx.fpr[2] = fa;
    if ((load<u32>(load<u32>(item + 28) + 104) >> 31) == 0) {
        emit({g19::kMethodBlendEnable, 1});
        emit({g19::kMethodBlendFunc, g19::kBlendSrcAlpha, g19::kBlendOneMinusSrcAlpha});
        emit({g19::kMethodBlendEquation, g19::kBlendFuncAdd});
        store<u32>(fc, 0x3F800000u);
    }
    call(ctx, g17::fn::FProgram_SetConstant, {fc_index, fc});
}

// After the draw: a translucent item gets depth writes back as render
// state u8 +93 bit 0 when material word +112 bit 31, and blending off
// unless +104 bit 31. Sets r3 as the original leaves it.
void translucent_end(PPUContext& ctx, u32 item) {
    const u32 st = fx::g::state;
    const u32 alpha = load<u8>(item + 71);
    if (alpha >= 255) return ret(ctx, alpha);
    u32 mat = load<u32>(item + 28);
    if ((load<u32>(mat + 112) >> 31) != 0) {
        emit({g19::kMethodDepthMask, load<u8>(st + 93) & 1u});
        mat = load<u32>(item + 28);
    }
    if ((load<u32>(mat + 104) >> 31) != 0) return ret(ctx, 1u);
    emit({g19::kMethodBlendEnable, 0});
    ret(ctx, load<u32>(st));
}

}  // namespace setup

// void DrawItem_SetupBasic_005D7598(-, DrawItem* item)  [virtual] (name?)
// The item matrix and fog (setup::matrix_and_fog); with material +100
// bit 7 and a layer list (item +44) the list (setup::layer_list);
// fragment constant 8 = shader3's character_fconst2; a translucent
// item's state (setup::translucent_begin, constant 8, 1 / 255); the mesh
// (+12) drawn through its virtual slot 4 (mesh, item); then
// setup::translucent_end.
DECOMP(005D7598, DrawItem_SetupBasic_005D7598) {
    const u32 item = arg<u32>(ctx, 1);
    setup::matrix_and_fog(ctx, item);
    {
        const u32 mat = load<u32>(item + 28);
        const u32 list = load<u32>(item + 44);
        if (((load<u8>(mat + 100) >> 7) & 1) != 0 && list != 0) setup::layer_list(ctx, mat, list);
    }
    call(ctx, g17::fn::FProgram_SetConstant, {8, setup::s3::g::character_fconst2});
    if (load<u8>(item + 71) < 255) setup::translucent_begin(ctx, item, 8, g19::kInv255);
    const u32 mesh = load<u32>(item + 12);
    fx::vcall(ctx, mesh, 16, {mesh, item});
    setup::translucent_end(ctx, item);
}

// bool LoadParams_005D0CA0(-, Material* m, const ParamDesc* d)
// [virtual] (name?)
// Nothing (1) when config u8 +152 bit 7 is set or the desc version
// (u8 d+84) is 0. Otherwise the textures named at d+16, +4, +8 and +12
// (Tex_FindLoaded; any missing: 0) each prepared (virtual slot 4: (t, 1,
// 1, 1)) and added (Material_AddTexture_004FF008; the first one added
// before its call, the rest after), the params set (Material_ParamSlot)
// from the desc words when its version is above the param's, else the
// defaults (kParamsD0CA0): 8 (+120) / 9 (+124) / 10 (+128) / 11 (+132)
// = 1, 12 / 13 (+136 / +140, default 0) in degrees to radians, 0 / 1
// the colours +88 / +92 (0xFFFFFFFF / 0xFF661919) with bytes 1 and 3
// swapped, 2 / 3 (+96 / +100) 1e-4, 4 (+104) 1, 5 (+108) 0.3, 6 (+112)
// 0.5, 16 / 17 (+152 / +156) 1e-4, 7 the colour +116 (white) swapped, 14
// (+144) 30, 15 (+148) 15, 18 and 19 = 0. Returns 1.
DECOMP(005D0CA0, LoadParams_005D0CA0) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 K = g19::kParamsD0CA0;
    if (((load<u8>(sly4::g::config + 152) >> 7) & 1) != 0) return ret(ctx, 1u);
    auto ver = [&] { return static_cast<s32>(load<u8>(d + 84)); };
    if (ver() <= 0) return ret(ctx, 1u);
    auto prepare = [&](u32 t) { fx::vcall(ctx, t, 16, {t, 1, 1, 1}); };
    auto add = [&](u32 t) { call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t}); };
    auto find = [&](u32 off) { return static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + off)})); };
    auto slot = [&](u32 i, u32 word) { store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, i})), word); };
    auto pick = [&](s32 above, u32 off, u32 def) { return ver() > above ? load<u32>(d + off) : def; };
    u32 t = find(16);
    if (t == 0) return ret(ctx, 0u);
    add(t);
    prepare(t);
    for (u32 off : {4u, 8u}) {
        t = find(off);
        if (t == 0) return ret(ctx, 0u);
        prepare(t);
        add(t);
    }
    const u32 one = load<u32>(K), zero = load<u32>(K + 4);
    const f64 rad = lfs(K + 8);
    slot(8, pick(8, 120, one));
    slot(9, pick(9, 124, one));
    {
        const u32 w = pick(12, 136, zero);
        ctx.fpr[1] = g10::lfs_value(w);  // as left
        const u32 v = g11::stfs_bits(sgl(g10::lfs_value(w) * rad));
        slot(12, v);
    }
    t = find(12);
    if (t == 0) return ret(ctx, 0u);
    prepare(t);
    add(t);
    slot(10, pick(10, 128, one));
    slot(11, pick(11, 132, one));
    slot(13, g11::stfs_bits(sgl(g10::lfs_value(pick(13, 140, zero)) * rad)));
    const u32 ca = pick(0, 88, g19::kDefaultColourA);
    const u32 cb = pick(1, 92, g19::kDefaultColourB);
    slot(0, swap_rb(ca));
    slot(1, swap_rb(cb));
    const u32 tiny = load<u32>(K + 12);
    slot(2, pick(2, 96, tiny));
    slot(3, pick(3, 100, tiny));
    slot(4, pick(4, 104, one));
    slot(5, pick(5, 108, load<u32>(K + 16)));
    slot(6, pick(6, 112, load<u32>(K + 20)));
    slot(16, pick(16, 152, tiny));
    slot(17, pick(17, 156, tiny));
    slot(7, swap_rb(pick(7, 116, g19::kDefaultColourA)));
    slot(14, pick(14, 144, load<u32>(K + 24)));
    slot(15, pick(15, 148, load<u32>(K + 28)));
    slot(18, zero);
    slot(19, zero);
    ret(ctx, 1u);
}

// bool LoadParams_005D4198(-, Material* m, const ParamDesc* d)
// [virtual] (name?)
// The SHOTextureWarp params: nothing (0) unless Shader_IsReady_005AF298
// (sho_texture_warp_type, m, d). The texture d+8 (or Tex_GetDefault(5))
// prepared (virtual slot 4: (t, 1, 1, 1)) and added; d+12 (or the
// default) prepared with the wraps of d+160 / +164 (0: 1, 1: 3, others
// 2), its u16 +116 / +118 / +120 = 1 and +128 bit 0 set, added; d+16,
// when loaded, the same with wraps 1. Then the params from the desc
// words above their version (u8 d+84) or the defaults (kParamsD4198):
// 10 (+128) 16, 11 (+132) 4, 12 / 13 / 14 / 16 / 17 / 20 / 22 / 23 / 26
// / 27 / 29 / 30 1, 15 (+148) the integer 1, 21 (+172) 1.33, 24 (+184)
// 0, 25 / 28 (+188 / +200) 0.1, 31 (+212) 30, 32 (+216) 15. With
// material word +104 bit 31, u8 m+45 = 3. Returns 1.
DECOMP(005D4198, LoadParams_005D4198) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 K = g19::kParamsD4198;
    if (static_cast<u32>(call(ctx, g12::fn::Shader_IsReady_005AF298, {g12::g::sho_texture_warp_type, m, d})) == 0) return ret(ctx, 0u);
    auto ver = [&] { return static_cast<s32>(load<u8>(d + 84)); };
    auto add = [&](u32 t) { call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t}); };
    auto find_or_default = [&](u32 off) {
        u32 t = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + off)}));
        if (t == 0) t = static_cast<u32>(call(ctx, g15::fn::Tex_GetDefault, {g19::kDefaultTexKind}));
        return t;
    };
    auto wrap = [](s32 v) { return v == 0 ? 1u : v == 1 ? 3u : 2u; };
    auto slot = [&](u32 i, u32 word) { store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, i})), word); };
    auto pick = [&](s32 above, u32 off, u32 def) { return ver() > above ? load<u32>(d + off) : def; };
    u32 t = find_or_default(8);
    fx::vcall(ctx, t, 16, {t, 1, 1, 1});
    add(t);
    t = find_or_default(12);
    const u32 ws = wrap(static_cast<s32>(load<u32>(d + 160))), wt = wrap(static_cast<s32>(load<u32>(d + 164)));
    fx::vcall(ctx, t, 16, {t, ws, wt, 1});
    {
        const u32 h = load<u16>(t + 128) | 1;
        store<u16>(t + 116, 1);
        store<u16>(t + 128, static_cast<u16>(h));
        store<u16>(t + 118, 1);
        store<u16>(t + 120, 1);
    }
    add(t);
    t = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 16)}));
    if (t != 0) {
        const u32 h = load<u16>(t + 128) | 1;
        store<u16>(t + 116, 1);
        store<u16>(t + 118, 1);
        store<u16>(t + 128, static_cast<u16>(h));
        store<u16>(t + 120, 1);
        fx::vcall(ctx, t, 16, {t, 1, 1, 1});
        add(t);
    }
    const u32 one = load<u32>(K + 8), tenth = load<u32>(K + 20);
    slot(10, pick(10, 128, load<u32>(K)));
    slot(11, pick(11, 132, load<u32>(K + 4)));
    slot(12, pick(12, 136, one));
    slot(13, pick(13, 140, one));
    slot(14, pick(14, 144, one));
    slot(15, pick(15, 148, 1u));
    slot(16, pick(16, 152, one));
    slot(21, pick(21, 172, load<u32>(K + 12)));
    slot(22, pick(22, 176, one));
    slot(23, pick(23, 180, one));
    slot(24, pick(24, 184, load<u32>(K + 16)));
    slot(17, pick(17, 156, one));
    slot(20, pick(20, 168, one));
    slot(25, pick(25, 188, tenth));
    slot(26, pick(26, 192, one));
    slot(27, pick(27, 196, one));
    slot(28, pick(28, 200, tenth));
    slot(29, pick(29, 204, one));
    slot(30, pick(30, 208, one));
    slot(31, pick(31, 212, load<u32>(K + 24)));
    slot(32, pick(32, 216, load<u32>(K + 28)));
    if ((load<u32>(m + 104) >> 31) != 0) store<u8>(m + 45, 3);
    ret(ctx, 1u);
}

// bool Iff_HandleEmitShape_00528948(IffReader* r, AglEmitProperties* o)
// [an IFF chunk handler] (name?)
// The emitter's shape: a version byte, "flags", "type", "speed",
// "speedVar", "axisOfstX", "axisSpreadX", "axisOfstZ", "axisSpreadZ"
// (f32, degrees) and "size" (vec3). Type 0 a 40-byte sphere (radius size
// x), 1 a 64-byte box (half size: size * 0.5), others 0 (returned, no
// shape). The new shape (SmallAlloc; refs 1, +8..+32 0, the vtable by
// flags bit 0) referenced from o+108 (set_ref; quirk: a failed
// allocation leaves 0 and the stores below go to low memory), its
// creation reference dropped; then +8 speed, +12 variance, u8 +16 = the
// axis is offset or spread, +20 / +28 (offset - spread / 2) and +24 /
// +32 spread, in radians. Returns 1.
DECOMP(00528948, Iff_HandleEmitShape_00528948) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 48);
    const u32 sp = frame.addr();  // the original's +112: flags, type, f32 [116..136], size [140]; +152 version
    for (u32 k = 0; k < 40; k += 4) store<u32>(sp + k, 0);
    call(ctx, g10::fn::Iff_ReadU8, {r, sp + 40, 0});
    call(ctx, g10::fn::Iff_ReadU8, {r, sp, g19::kStrFlags});
    call(ctx, g10::fn::Iff_ReadU8, {r, sp + 1, g19::kStrType});
    const u32 names[6] = {g19::kStrSpeed, g19::kStrSpeedVar, g19::kStrAxisOfstX, g19::kStrAxisSpreadX, g19::kStrAxisOfstZ, g19::kStrAxisSpreadZ};
    for (u32 k = 0; k < 6; ++k) call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp + 4 + 4 * k, names[k]});
    call(ctx, g17::fn::FileReader_ReadVec3Swapped, {r, sp + 28, g19::kStrSize});
    const u32 type = load<u8>(sp + 1);
    if (type > 1) return ret(ctx, 0u);
    const bool alt = (load<u8>(sp) & 1) != 0;
    const u32 K = g19::kShapeConsts;
    const u32 p = static_cast<u32>(call(ctx, g12::fn::SmallAlloc, {type == 0 ? 40u : 64u}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p + 8, 0);
        store<u32>(p + 12, 0);
        store<u8>(p + 16, 0);
        for (u32 k = 20; k <= 32; k += 4) store<u32>(p + k, 0);
        if (type == 0) {
            store<u32>(p, alt ? g19::kVtShapeSphereA : g19::kVtShapeSphereB);
            store<u32>(p + 36, load<u32>(sp + 28));
        } else {
            store<u32>(p, alt ? g19::kVtShapeBoxA : g19::kVtShapeBoxB);
            const f64 half = lfs(K + 4);
            for (u32 k = 0; k < 3; ++k) store<u32>(p + 48 + 4 * k, g11::stfs_bits(sgl(lfs(sp + 28 + 4 * k) * half)));
        }
    }
    set_ref(ctx, o + 108, p);
    {
        const u32 q = load<u32>(o + 108);
        store<u16>(q + 4, static_cast<u16>(load<u16>(q + 4) - 1));
    }
    const u32 s = load<u32>(o + 108);
    store<u32>(s + 8, load<u32>(sp + 4));
    store<u32>(s + 12, load<u32>(sp + 8));
    const f64 zero = lfs(K), half = lfs(K + 4), rad = lfs(K + 8);
    const f64 ox = lfs(sp + 12), sx = lfs(sp + 16), oz = lfs(sp + 20), sz = lfs(sp + 24);
    const bool none = ox == zero && sx == zero && oz == zero && sz == zero;
    store<u8>(s + 16, none ? 0 : 1);
    const f64 a = g11::nmsubs(sx, half, ox), b = g11::nmsubs(sz, half, oz);
    const f64 v24 = sgl(sx * rad), v32 = sgl(sz * rad), v20 = sgl(a * rad), v28 = sgl(b * rad);
    store<u32>(s + 24, g11::stfs_bits(v24));
    store<u32>(s + 32, g11::stfs_bits(v32));
    store<u32>(s + 20, g11::stfs_bits(v20));
    store<u32>(s + 28, g11::stfs_bits(v28));
    ctx.fpr[1] = v28;  // as left
    ctx.fpr[2] = v32;
    ret(ctx, 1u);
}

// bool Iff_HandleEmitRotation_00529FE0(IffReader* r, AglEmitProperties* o)
// [an IFF chunk handler] (name?)
// The emitter's rotation: "VersionNumber" (default 1), "axis" (vec3),
// "variance", from version 1 "initialAngleRange" and "randomDirEnable".
// A new 192-byte rotation (Mem_Alloc(192, 0, 16); refs 1, vtable, u8
// +176 = 3, +32 / +36 / +172 / u8 +177 0) referenced from o+124 (set_ref;
// quirk: a failed allocation carries on at address 0), its creation
// reference dropped. +16 the axis normalised (zero: kUnitAxes[2]), u8
// +176 which of the unit axes it is (3: none); +32 the variance clamped
// to [-1, 1] (fcmpu order kept), u8 +177 set when below 0; +172 the
// angle range clamped to [0, 359.999] / 360; u8 +177 = randomDirEnable
// != 0. Then the chunk's remaining f32 "Rot" keys appended at +104
// (count +36; the first repeated), +168 = o+4, the count + 1 capped at
// 16, and +40.. the running sums of the keys in radians. Returns 1.
DECOMP(00529FE0, Iff_HandleEmitRotation_00529FE0) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 32);
    const u32 sp = frame.addr();  // the original's +112: key, axis [116], variance, range, enable, version [140]
    for (u32 k = 4; k <= 24; k += 4) store<u32>(sp + k, 0);
    store<u8>(sp + 28, 1);
    call(ctx, g10::fn::Iff_ReadU8, {r, sp + 28, g19::kStrVersionNumber});
    call(ctx, g17::fn::FileReader_ReadVec3Swapped, {r, sp + 4, g19::kStrAxis});
    call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp + 16, g19::kStrVariance});
    if (load<u8>(sp + 28) >= 1) {
        call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp + 20, g19::kStrInitialAngleRange});
        call(ctx, g10::fn::Iff_ReadU8, {r, sp + 24, g19::kStrRandomDirEnable});
    }
    const u32 p = static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {192, 0, 16}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u8>(p + 176, 3);
        store<u32>(p, g19::kVtEmitRotation);
        store<u32>(p + 32, 0);
        store<u32>(p + 36, 0);
        store<u32>(p + 172, 0);
        store<u8>(p + 177, 0);
    }
    set_ref(ctx, o + 124, p);
    {
        const u32 q = load<u32>(o + 124);
        store<u16>(q + 4, static_cast<u16>(load<u16>(q + 4) - 1));
    }
    const u32 K = g19::kRotationConsts;
    const f64 zero = lfs(K), one = lfs(K + 4), neg_one = lfs(K + 8), rad = lfs(K + 20);
    {
        const f64 x = lfs(sp + 4), y = lfs(sp + 8), z = lfs(sp + 12);
        f64 len2 = sgl(y * y);
        len2 = g11::madds(x, x, len2);
        len2 = g11::madds(z, z, len2);
        const u32 s = load<u32>(o + 124);
        if (len2 != zero) {
            const f64 inv = sgl(one / sgl(std::sqrt(len2)));
            store<u32>(s + 16, g11::stfs_bits(sgl(x * inv)));
            store<u32>(s + 20, g11::stfs_bits(sgl(y * inv)));
            store<u32>(s + 24, g11::stfs_bits(sgl(z * inv)));
        } else {
            for (u32 k = 0; k < 12; k += 4) store<u32>(s + 16 + k, load<u32>(g19::kUnitAxes[2] + k));
        }
        store<u8>(s + 176, 3);
        for (u32 a = 0; a < 3; ++a) {
            const u32 u = g19::kUnitAxes[a];
            if (load<u64>(s + 16) == load<u64>(u) && load<u32>(s + 24) == load<u32>(u + 8)) {
                store<u8>(s + 176, static_cast<u8>(a));
                break;
            }
        }
    }
    {
        const f64 var = lfs(sp + 16);
        const u32 s = load<u32>(o + 124);
        const f64 lo = var < one ? var : one;
        const f64 v = lo > neg_one ? lo : neg_one;
        store<u32>(s + 32, g11::stfs_bits(v));
        if (v < zero) store<u8>(s + 177, 1);
    }
    {
        const f64 range = lfs(sp + 20), top = lfs(K + 12);
        const u32 s = load<u32>(o + 124);
        const f64 lo = range < top ? range : top;
        const f64 v = lo > zero ? lo : zero;
        store<u32>(s + 172, g11::stfs_bits(sgl(v / lfs(K + 16))));
        store<u8>(load<u32>(o + 124) + 177, load<u8>(sp + 24) != 0 ? 1 : 0);
    }
    const u32 n = static_cast<u32>(call(ctx, g13::fn::ChunkReader_Remaining, {r})) >> 2;
    for (u32 i = 0; i < n; ++i) {
        call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp, g19::kStrRot});
        const u32 s = load<u32>(o + 124);
        const u32 key = load<u32>(sp);
        const u32 c = load<u32>(s + 36);
        store<u32>(s + 36, c + 1);
        store<u32>(s + 104 + 4 * c, key);
        const u32 c2 = load<u32>(s + 36);
        if (c2 == 1) store<u32>(s + 104 + 4 * c2, key);
    }
    const u32 s = load<u32>(o + 124);
    store<u32>(s + 168, load<u32>(o + 4));
    const u32 c = load<u32>(s + 36) + 1;
    const u32 cnt = c < g19::kRotationKeys ? c : g19::kRotationKeys;
    store<u32>(s + 36, cnt);
    f64 acc = zero;
    u32 i = 1;
    for (; cnt > 1 && i < load<u32>(s + 36); ++i) {
        const f64 out = sgl(acc * rad);
        acc = sgl(lfs(s + 104 + 4 * (i - 1)) + acc);
        store<u32>(s + 40 + 4 * (i - 1), g11::stfs_bits(out));
    }
    const f64 last = sgl(acc * rad);
    store<u32>(s + 36 + 4 * i, g11::stfs_bits(last));
    ctx.fpr[1] = last;  // as left
    ctx.fpr[2] = rad;
    ret(ctx, 1u);
}

// bool Iff_HandleMeshHeader_00524B68(IffReader* r, Mesh* mesh)
// [an IFF chunk handler] (name?)
// The mesh header into a 260-byte record on the stack (+0 u16
// numVerts, +2 numIndices, +4 u8 primitiveType, +8 materialHash, +12
// vtxDefHash, +16 vtxScale (version below 1: 1), +20 u8 visGroup, from
// version 3 +24 / +28 the vertex pack buffer, from 4 +32 boundSphere,
// +48.. the index and stream 1 pack buffers), then the remaining bytes
// / 10 vertex elements (u8 usage, usageIndex, type, size, fract, stride,
// u32 offset; 12 apart from +68). The format: the shared declaration
// (VtxDecl_Get_005246A8(elements, n, numIndices != 0)) + 16, or 0
// without elements; the material: the AglMaterial with that hash (+40)
// among the list's used entries (g15::g::material_list, made on first
// use) while referenced, else Shader_LoadGroup(0); then +68 the format,
// the material set, +40 / +64 the counts, +76 strip (6) or triangles
// (5), +88 the scale, u8 +108 the group, +92 the bound sphere, and
// Streams_Bind_00538950(mesh, record). Returns 1.
// Quirk: the element count is unchecked. The original's 496-byte frame
// (the record at +112, the version byte at +372, r18..r31 saved at
// +384..) is reproduced, so more than 16 elements land where the
// original's do, saved registers included (restored from the frame on
// exit, as its epilogue does; its LR slot is not modelled).
DECOMP(00524B68, Iff_HandleMeshHeader_00524B68) {
    const u32 r = arg<u32>(ctx, 0), mesh = arg<u32>(ctx, 1);
    Stack frame(ctx, 496);
    const u32 sp = frame.addr();
    for (u32 k = 0; k < 14; ++k) store<u64>(sp + 384 + 8 * k, ctx.gpr[18 + k]);
    auto leave = [&](u32 v) {
        for (u32 k = 0; k < 14; ++k) ctx.gpr[18 + k] = load<u64>(sp + 384 + 8 * k);
        ret(ctx, v);
    };
    const u32 h = sp + 112, verp = sp + 372;
    call(ctx, g12::fn::memset, {h, 0, 260});
    store<u8>(verp, 4);
    auto u8r = [&](u32 out, u32 name) { call(ctx, g10::fn::Iff_ReadU8, {r, out, name}); };
    auto u16r = [&](u32 out, u32 name) { call(ctx, g11::fn::Iff_ReadU16, {r, out, name}); };
    auto u32r = [&](u32 out, u32 name) { call(ctx, g10::fn::Iff_ReadU32, {r, out, name}); };
    auto ver = [&] { return static_cast<s32>(load<u8>(verp)); };
    u8r(verp, g19::kStrVer);
    u16r(h, g19::kStrNumVerts);
    u16r(h + 2, g19::kStrNumIndices);
    u8r(h + 4, g19::kStrPrimitiveType);
    u32r(h + 8, g19::kStrMaterialHash);
    u32r(h + 12, g19::kStrVtxDefHash);
    if (ver() < 1) store<u32>(h + 16, 0x3F800000u);
    else call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, h + 16, g19::kStrVtxScale});
    if (ver() < 2) store<u8>(h + 20, 0);
    else u8r(h + 20, g19::kStrVisGroup);
    if (ver() >= 3) {
        u32r(h + 24, g19::kStrVtxPackBufOfst);
        u32r(h + 28, g19::kStrVtxPackBufSize);
    }
    if (ver() >= 4) {
        call(ctx, g14::fn::FileReader_ReadVec4Swapped, {r, h + 32, g19::kStrBoundSphere});
        u32r(h + 48, g19::kStrIdxPackBufOfst);
        u32r(h + 52, g19::kStrIdxPackBufSize);
        u32r(h + 56, g19::kStrStream1PackBufOfst);
        u32r(h + 60, g19::kStrStream1PackBufSize);
    }
    const u32 n = static_cast<u32>(call(ctx, g13::fn::ChunkReader_Remaining, {r})) / g19::kMeshElementBytes;
    store<u32>(h + 64, n);
    const u32 elems = h + 68;
    for (u32 i = 0; i < load<u32>(h + 64) && n != 0; ++i) {
        const u32 e = elems + 12 * i;
        u8r(e, g19::kStrUsage);
        u8r(e + 1, g19::kStrUsageIndex);
        u8r(e + 2, g19::kStrElemType);
        u8r(e + 3, g19::kStrElemSize);
        u8r(e + 4, g19::kStrFract);
        u8r(e + 5, g19::kStrStride);
        u32r(e + 8, g19::kStrOffset);
    }
    const u32 cnt = load<u32>(h + 64);
    u32 fmt = 0;
    if (cnt != 0)
        fmt = static_cast<u32>(call(ctx, g19::fn::VtxDecl_Get_005246A8, {elems, cnt, load<u16>(h + 2) != 0 ? 1u : 0u})) + 16;
    u32 mat = 0;
    if (const u32 hash = load<u32>(h + 8); hash != 0) {
        const u32 hdr = load<u32>(g15::g::material_list);
        const u32 list = get_free_list(ctx, hdr, g15::g::vt_material_list, g15::g::str_material_list, 144);
        const u32 first = static_cast<u32>(call(ctx, g17::fn::Obj_GetField4, {list}));
        for (u32 m = first != 0 ? first + 16 : 0; m != 0;) {
            if (load<u32>(m + 40) == hash) {
                if (static_cast<s16>(load<u16>(m + 4)) > 0) mat = m;
                break;
            }
            const u32 next = load<u32>(m - 16);
            m = next != 0 ? next + 16 : 0;
        }
    }
    if (mat == 0) mat = static_cast<u32>(call(ctx, g19::fn::Shader_LoadGroup, {0}));
    store<u32>(mesh + 68, fmt);
    set_material(ctx, mesh, mat);
    store<u32>(mesh + 40, load<u16>(h));
    store<u32>(mesh + 64, load<u16>(h + 2));
    store<u32>(mesh + 76, load<u8>(h + 4) != 0 ? g18::kPrimTriangleStrip : g19::kPrimTriangles);
    store<u32>(mesh + 88, load<u32>(h + 16));
    store<u8>(mesh + 108, load<u8>(h + 20));
    for (u32 k = 0; k < 16; k += 4) store<u32>(mesh + 92 + k, load<u32>(h + 32 + k));
    ctx.fpr[1] = lfs(h + 44);  // as left
    call(ctx, g19::fn::Streams_Bind_00538950, {mesh, h});
    leave(1u);
}

// bool Iff_HandleMaterial_0051C704(IffReader* r, Material* m)
// [an IFF chunk handler] (name?)
// The material chunk into a 232-byte record (memset 0; see kMatV4):
// "VersionNumber" (default 4), then version 4's fields, version 3's (4's
// without sortLayer) or version 2's (kMatV2; other versions none), then
// the remaining bytes / 4 (as a u8 count, +84) unnamed u32 params (+88..),
// and Material_Apply_0050C3D8(m, record). Returns 1.
// Quirk: the param count is unchecked. The original's 512-byte frame
// (the record at +112, the version at +344, the record pointer and m
// saved at +352 / +360 and reloaded for the last call, r14..r31 at +368..)
// is reproduced, so more than 36 params land where the original's do,
// those saves included (restored from the frame on exit, as its
// epilogue does; its LR slot is not modelled).
DECOMP(0051C704, Iff_HandleMaterial_0051C704) {
    const u32 r = arg<u32>(ctx, 0);
    Stack frame(ctx, 512);
    const u32 sp = frame.addr();
    const u32 rec = sp + 112, verp = sp + 344;
    store<u64>(sp + 360, ctx.gpr[4]);
    store<u64>(sp + 352, rec);
    for (u32 k = 0; k < 18; ++k) store<u64>(sp + 368 + 8 * k, ctx.gpr[14 + k]);
    call(ctx, g12::fn::memset, {rec, 0, 232});
    store<u8>(verp, 4);
    call(ctx, g10::fn::Iff_ReadU8, {r, verp, g19::kStrMatVersion});
    auto read = [&](const g19::MatFieldRead& f) {
        const u32 out = rec + f.offset;
        switch (f.kind) {
        case g19::MatField::U32: call(ctx, g10::fn::Iff_ReadU32, {r, out, f.name}); break;
        case g19::MatField::Name: call(ctx, g14::fn::FileReader_Read32, {r, out, f.name}); break;
        case g19::MatField::U8: call(ctx, g10::fn::Iff_ReadU8, {r, out, f.name}); break;
        case g19::MatField::Colour: call(ctx, g16::fn::FileReader_ReadU32, {r, out, f.name}); break;
        case g19::MatField::F32: call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, out, f.name}); break;
        }
    };
    const u32 ver = load<u8>(verp);
    if (ver == 4 || ver == 3) {
        for (const auto& f : g19::kMatV4)
            if (ver == 4 || f.offset != g19::kMatSortLayerOffset) read(f);
    } else if (ver == 2) {
        for (const auto& f : g19::kMatV2) read(f);
    }
    const u32 rem = static_cast<u32>(call(ctx, g13::fn::ChunkReader_Remaining, {r}));
    store<u8>(rec + 84, static_cast<u8>(rem >> 2));
    for (u32 i = 0; i < load<u8>(rec + 84) && ((rem >> 2) & 0xFF) != 0; ++i)
        call(ctx, g10::fn::Iff_ReadU32, {r, rec + 88 + 4 * i, 0});
    call(ctx, g19::fn::Material_Apply_0050C3D8, {load<u64>(sp + 360), load<u64>(sp + 352)});
    for (u32 k = 0; k < 18; ++k) ctx.gpr[14 + k] = load<u64>(sp + 368 + 8 * k);
    ret(ctx, 1u);
}

// void Bank_StaticInit_005AD444()  [static initialiser] (name?)
// The object bank's globals set to their defaults: flag bytes, the
// default object matrix (kObjDefaultMatrix: kBankIdentity's 3x3 rows and
// kBankOrigin), the changed-object list (20000 bytes from Mem_Alloc,
// capacity 5000), three buffer parts (usage 1), empty bounds ({FLT_MAX,
// -FLT_MAX} boxes), two default projections, 12 view records {u8 5, ...,
// colour 0x01164CA0, 70, 30, 1, 1}, 32 blend records {CELL_GCM_SRC_ALPHA
// (515), 2304, 1, FUNC_ADD, ...}, the bank globals (a 160-byte block from
// Mem_Alloc, PtrVector_Construct of +5220, its element size 4) and a few
// atomically cleared words. Leaves r3 = 0 as the original does.
DECOMP(005AD444, Bank_StaticInit_005AD444) {
    auto atomic_clear = [](u32 ea) { std::atomic_ref<u32>(ref<u32>(ea)).store(0); };  // lwarx / stwcx.
    auto fill = [](u32 base, std::initializer_list<u32> offs, u32 v) {
        for (u32 o : offs) store<u32>(base + o, v);
    };
    constexpr u32 MX = g19::kFltMax, NMX = g19::kNegFltMax, ONE = g19::kOne, HALF = g19::kHalf;
    store<u8>(0x01194570, 0xFF);
    {
        const u32 d = g19::kObjDefaultMatrix, src = g19::kBankIdentity;
        store<u32>(0x01194574, ONE);
        store<u32>(0x01194578, ONE);
        for (u32 o : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u}) store<u32>(d + o, load<u32>(src + o));
        for (u32 k = 0; k < 12; k += 4) store<u32>(d + 64 + k, load<u32>(g19::kBankOrigin + k));
        ctx.fpr[1] = lfs(g19::kBankOrigin + 8);  // the lfs / stfs pairs' last, as left
        ctx.fpr[2] = lfs(src + 56);
    }
    {
        const u32 L = g19::kChangedList;
        store<u32>(L, static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {g19::kChangedListSize, 0, 16})));
        store<u32>(L + 12, g19::kChangedListCapacity);
        store<u32>(L + 4, 0);
        store<u32>(L + 8, g19::kChangedListCapacity);
    }
    fill(0x01194580, {0, 4, 8, 12}, 0);
    atomic_clear(0x0116BD80);
    store<u32>(0x01194EC4, 0);
    store<u32>(0x01194ED0, 0);
    store<u8>(0x01194ED4, 0);
    store<u32>(0x01194EC8, 0);
    store<u8>(0x01194ECC, 0);
    for (u32 k = 0; k < 3; ++k) {
        const u32 part = 0x01194ED8 + 44 + 52 * k;
        const u32 w4 = (load<u32>(part + 4) & 1) | 1, w12 = load<u32>(part + 12) & 0x0FFFFFFF;
        store<u32>(part, 0);
        store<u32>(part + 12, w12);
        store<u32>(part + 4, w4);
        store<u32>(part + 40, 0);
        store<u32>(part + 8, 1u << 1);  // usage 1; the other bits cleared
    }
    for (u32 b : {0x01194FB0u, 0x01195020u}) {
        fill(b, {16, 20, 24}, MX);
        fill(b, {32, 36, 40}, NMX);
    }
    {
        const u32 p = 0x0116BE80;
        store<u32>(p, 0x40000000);
        fill(p, {4, 8, 16}, 0);
        store<u32>(p + 20, 0xC0000000u);
        fill(p, {24, 32, 36}, 0);
        store<u32>(p + 40, ONE);
        store<u32>(p + 48, 0xBF800000u);
        store<u32>(p + 52, ONE);
        store<u32>(p + 56, 0);
        const u32 q = 0x0116BEC0;
        store<u32>(q, HALF);
        fill(q, {4, 8, 12, 16}, 0);
        store<u32>(q + 20, 0xBF000000u);
        fill(q, {24, 28, 32, 36}, 0);
        store<u32>(q + 40, ONE);
        store<u32>(q + 44, 0);
        store<u32>(q + 48, HALF);
        store<u32>(q + 52, HALF);
        store<u32>(q + 56, 0);
        store<u32>(q + 60, ONE);
        fill(0x011950A0, {0, 4, 8, 12}, 0);
    }
    for (u32 k = 0; k < 12; ++k) {
        const u32 e = 0x01195120 + 144 * k;
        store<u8>(e, 5);
        store<u32>(e + 4, 0);
        store<u32>(e + 96, 0);
        for (u32 c = 0; c < 4; ++c) store<u8>(e + 100 + c, load<u8>(0x01164CA0 + c));
        store<u32>(e + 104, 0x428C0000);  // 70
        store<u32>(e + 108, 0x41F00000);  // 30
        store<u32>(e + 112, ONE);
        store<u32>(e + 116, ONE);
    }
    for (u32 b : {0x011957E8u, 0x0119582Cu})
        for (u32 k = 0; k < 4; ++k) {
            const u32 e = b + 16 * k;
            fill(e, {0, 4, 8}, 0);
            store<u8>(e + 12, 0);
        }
    store<u32>(0x01195984, 0);
    for (u32 k = 0; k < 32; ++k) {
        const u32 e = 0x0116BF00 + 272 * k;
        store<u32>(e + 76, 515);  // CELL_GCM_SRC_ALPHA
        store<u8>(e + 80, 0);
        store<u32>(e + 84, 2304);
        store<u8>(e + 88, 0);
        store<u32>(e + 92, 1);
        store<u32>(e + 96, 0);
        store<u32>(e + 100, 0x8006);  // CELL_GCM_FUNC_ADD
        store<u32>(e + 104, 0);
        fill(e, {180, 184, 188, 192, 196}, 1);
        store<u32>(e + 208, 0);
    }
    atomic_clear(0x0117E200);
    for (u32 b : {0x0117E280u, 0x0118B180u}) {
        fill(b, {3040, 3044, 3048}, MX);
        fill(b, {3056, 3060, 3064}, NMX);
    }
    atomic_clear(0x0118BF80);
    fill(0x0118E080, {48, 52, 56}, MX);
    fill(0x0118E080, {64, 68, 72}, NMX);
    {
        const u32 G = g19::kBankGlobals;
        store<u32>(G + 40, 0);
        store<u16>(G + 44, 0);
        store<u32>(G + 56, 0x42960000);  // 75
        store<u32>(G + 48, static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {160, 0, 16})));
        store<u16>(G + 52, 0);
        store<u32>(G + 64, 0);
        store<u16>(G + 68, 0);
        store<u16>(G + 5200, 0);
        call(ctx, g19::fn::PtrVector_Construct, {G + 5220});
        fill(G, {5264, 5268, 5272}, MX);
        store<u16>(G + 5226, 4);
        fill(G, {5280, 5284, 5288}, NMX);
        fill(G, {5328, 5332, 5336}, MX);
        fill(G, {5344, 5348, 5352}, NMX);
        fill(G, {5368, 5372}, 0);
    }
    atomic_clear(0x01195880);
    atomic_clear(0x01192680);
    ret(ctx, 0u);
}

// bool Iff_HandleDds_00539C7C(FileReader* r, Texture* t)
// [an IFF chunk handler] (name?)
// A DDS image into t: t's sampler and layout words copied from
// Tex_GetDefault(0) (+56..+96; +64's low nibble and +68's top nibble
// kept, +64 bit 0 set, +96 0), the magic and 124-byte header read
// (FileReader_Read) and byte-swapped, u16 t+52 / +54 = width / height;
// the format by fourCC (DXT1 / DXT2-3 / DXT4-5, the power-of-two
// variants when both sides are; V8U8 / CxV8U8 17; anything else 0,
// returned). Texture_InitLayout(t+56, w, h, 1, format, 0, 0, 0); u16
// +116 / +118 = 1, +120 = 0, +128 bit 0 set; the (+64 >> 4) bytes read
// into a MemStack_Alloc block, V8U8 bytes doubled in place, copied to
// the texture (Rsx_Copy(block, t+56, size, 1)) and freed (the stack's
// virtual slot 2); g19::kTextureLoaded set. Returns 1.
// Quirk: a V8U8 image under 32 bytes runs the doubling loop past its
// block, as the original does.
DECOMP(00539C7C, Iff_HandleDds_00539C7C) {
    const u32 r = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 d = static_cast<u32>(call(ctx, g15::fn::Tex_GetDefault, {0}));
    for (u32 k = 0; k < 24; k += 4) store<u32>(t + 72 + k, load<u32>(d + 72 + k));
    store<u32>(t + 56, load<u32>(d + 56));
    store<u32>(t + 60, load<u32>(d + 60));
    {
        u32 w64 = (load<u32>(t + 64) & 0xF) | (load<u32>(d + 64) & ~0xFu);
        store<u32>(t + 64, w64);
        const u32 t68 = load<u32>(t + 68) & 0x0FFFFFFF;
        w64 = (w64 & ~0xEu) | (load<u32>(d + 64) & 0xE);
        store<u32>(t + 64, w64);
        const u32 d68 = load<u32>(d + 68) & 0xF0000000u;
        store<u32>(t + 64, w64 | 1);
        store<u32>(t + 96, 0);
        store<u32>(t + 68, t68 | d68);
    }
    Stack frame(ctx, 128);
    const u32 sp = frame.addr();  // the original's +112: magic, then the header at +116
    const u32 hd = sp + 4;
    call(ctx, g15::fn::FileReader_Read, {r, sp, 4});
    call(ctx, g15::fn::FileReader_Read, {r, hd, g19::kDdsHeaderSize});
    for (u32 o : {0u, 4u, 8u, 12u, 16u, 20u, 24u, 72u, 76u, 80u, 84u, 88u, 92u, 96u, 100u, 104u, 108u})
        store<u32>(hd + o, __builtin_bswap32(load<u32>(hd + o)));
    const u32 h = load<u32>(hd + 8), w = load<u32>(hd + 12), cc = load<u32>(hd + 80);
    store<u16>(t + 52, static_cast<u16>(w));
    store<u16>(t + 54, static_cast<u16>(h));
    const bool pot = ((w & (0u - w)) - w) == 0 && ((h & (0u - h)) - h) == 0;
    u32 fmt;
    bool v8u8 = false;
    if (cc == g19::kFourCcDxt5 || cc == g19::kFourCcDxt4) fmt = pot ? g19::kTexDxt5Pot : g19::kTexDxt5;
    else if (cc == g19::kFourCcDxt3 || cc == g19::kFourCcDxt2) fmt = pot ? g19::kTexDxt3Pot : g19::kTexDxt3;
    else if (cc == g19::kFourCcDxt1) fmt = pot ? g19::kTexDxt1Pot : g19::kTexDxt1;
    else if (cc == g19::kD3dCxV8U8 || cc == g19::kD3dV8U8) fmt = g19::kTexV8U8, v8u8 = true;
    else return ret(ctx, 0u);
    call(ctx, g19::fn::Texture_InitLayout, {t + 56, w & 0xFFFF, h & 0xFFFF, 1, fmt, 0, 0, 0});
    const u32 w64 = load<u32>(t + 64);
    const u32 size = w64 >> 4;
    {
        const u32 h128 = load<u16>(t + 128) | 1;
        store<u16>(t + 116, 1);
        store<u16>(t + 118, 1);
        store<u16>(t + 128, static_cast<u16>(h128));
        store<u16>(t + 120, 0);
    }
    const u32 st = static_cast<u32>(call(ctx, g12::fn::Mem_GetDefaultStack, {}));
    const u32 buf = static_cast<u32>(call(ctx, g12::fn::MemStack_Alloc, {st, size}));
    call(ctx, g15::fn::FileReader_Read, {r, buf, size});
    if (v8u8) {
        auto dbl = [](u32 a) { store<u8>(a, static_cast<u8>(load<u8>(a) << 1)); };
        u32 p = buf, c = w64 >> 5;
        for (;;) {
            dbl(p);
            dbl(p + 1);
            p += 2;
            if (c - 1 == 0) break;
            dbl(p);
            dbl(p + 1);
            p += 2;
            c -= 2;
            if (c == 0) break;
        }
    }
    call(ctx, g19::fn::Rsx_Copy, {buf, load<u32>(t + 56), size, 1});
    const u32 st2 = static_cast<u32>(call(ctx, g12::fn::Mem_GetDefaultStack, {}));
    fx::vcall(ctx, st2, 8, {st2, buf});
    if (load<u8>(g19::kTextureLoaded) == 0) store<u8>(g19::kTextureLoaded, 1);
    ret(ctx, 1u);
}

// bool LoadParams_005D5C80(-, Material* m, const ParamDesc* d)
// [virtual] (name?)
// LoadParams_005D4198's sibling with its own defaults (kParamsD5C80):
// nothing (0) unless Shader_IsReady_005AF298(sho_texture_warp_type, m,
// d); the textures d+8 and d+12 as there, d+16 (or the default) with
// wraps 1; params 10..17 and 20..34 from the desc words above their
// version or the defaults (15 the integer 1); u8 m+45 = 7. Returns 1.
DECOMP(005D5C80, LoadParams_005D5C80) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 K = g19::kParamsD5C80;
    if (static_cast<u32>(call(ctx, g12::fn::Shader_IsReady_005AF298, {g12::g::sho_texture_warp_type, m, d})) == 0) return ret(ctx, 0u);
    auto ver = [&] { return static_cast<s32>(load<u8>(d + 84)); };
    auto add = [&](u32 t) { call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t}); };
    auto find_or_default = [&](u32 off) {
        u32 t = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + off)}));
        if (t == 0) t = static_cast<u32>(call(ctx, g15::fn::Tex_GetDefault, {g19::kDefaultTexKind}));
        return t;
    };
    auto wrap = [](s32 v) { return v == 0 ? 1u : v == 1 ? 3u : 2u; };
    auto slot = [&](u32 i, u32 word) { store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, i})), word); };
    auto pick = [&](s32 above, u32 off, u32 def) { return ver() > above ? load<u32>(d + off) : def; };
    u32 t = find_or_default(8);
    fx::vcall(ctx, t, 16, {t, 1, 1, 1});
    add(t);
    t = find_or_default(12);
    const u32 ws = wrap(static_cast<s32>(load<u32>(d + 160))), wt = wrap(static_cast<s32>(load<u32>(d + 164)));
    fx::vcall(ctx, t, 16, {t, ws, wt, 1});
    {
        const u32 h = load<u16>(t + 128) | 1;
        store<u16>(t + 116, 1);
        store<u16>(t + 128, static_cast<u16>(h));
        store<u16>(t + 118, 1);
        store<u16>(t + 120, 1);
    }
    add(t);
    t = find_or_default(16);
    {
        const u32 h = load<u16>(t + 128) | 1;
        store<u16>(t + 116, 1);
        store<u16>(t + 118, 1);
        store<u16>(t + 120, 1);
        store<u16>(t + 128, static_cast<u16>(h));
    }
    fx::vcall(ctx, t, 16, {t, 1, 1, 1});
    add(t);
    auto k = [&](u32 i) { return load<u32>(K + 4 * i); };
    slot(10, pick(10, 128, k(0)));
    slot(11, pick(11, 132, k(1)));
    slot(12, pick(12, 136, k(2)));
    slot(13, pick(13, 140, k(2)));
    slot(14, pick(14, 144, k(2)));
    slot(15, pick(15, 148, 1u));
    slot(16, pick(16, 152, k(2)));
    slot(21, pick(21, 172, k(3)));
    slot(22, pick(22, 176, k(4)));
    slot(23, pick(23, 180, k(5)));
    slot(24, pick(24, 184, k(6)));
    slot(25, pick(25, 188, k(4)));
    slot(26, pick(26, 192, k(5)));
    slot(17, pick(17, 156, k(2)));
    slot(20, pick(20, 168, k(2)));
    slot(27, pick(27, 196, k(7)));
    slot(28, pick(28, 200, k(2)));
    slot(29, pick(29, 204, k(2)));
    slot(30, pick(30, 208, k(7)));
    slot(31, pick(31, 212, k(2)));
    slot(32, pick(32, 216, k(2)));
    slot(33, pick(33, 220, k(8)));
    slot(34, pick(34, 224, k(9)));
    store<u8>(m + 45, 7);
    ret(ctx, 1u);
}

namespace setup {

// DrawItem_SetupStencil_005D65C0 / _005D8100's shared body (inv255:
// each one's own copy of 1 / 255). As DrawItem_SetupBasic_005D7598 but:
// the layer list (item +44, unchecked) when u8 item +69 bit 0; fragment
// constant 2 instead of 8; items with u8 +68 bit 7 drawn with culling on
// and clockwise front faces (counter-clockwise again after); the stencil
// ref ((+68 bit 5 ? 128 : 0) | material u8 +101, masked by kStencilMaskByte)
// sent with the render state's func (+160) and mask (+172) when it
// differs from kStencilRefLast (then render state +164 / +168 updated
// and the stencil write mask sent).
void stencil_body(PPUContext& ctx, u32 inv255) {
    namespace l4 = sly4::render::layers4;
    namespace sp = sly4::render::pool;
    const u32 item = arg<u32>(ctx, 1);
    const u32 st = fx::g::state;
    matrix_and_fog(ctx, item);
    if ((load<u8>(item + 69) & 1) != 0) setup::layer_list(ctx, load<u32>(item + 28), load<u32>(item + 44));
    call(ctx, g17::fn::FProgram_SetConstant, {2, s3::g::character_fconst2});
    if (load<u8>(item + 71) < 255) translucent_begin(ctx, item, 2, inv255);
    u32 f68 = load<u8>(item + 68);
    if (((f68 >> 7) & 1) != 0) {
        emit({l4::kCullEnable, 1});
        emit({l4::kFrontFace, g19::kGcmCw});
        f68 = load<u8>(item + 68);
    }
    const u32 ref = ((((f68 >> 5) & 1) != 0 ? 128u : 0u) | load<u8>(load<u32>(item + 28) + 101)) & load<u8>(g19::kStencilMaskByte);
    if (ref != load<u8>(g19::kStencilRefLast)) {
        store<u8>(g19::kStencilRefLast, static_cast<u8>(ref));
        store<u32>(st + 164, ref);
        emit({sp::kStencilFunc, load<u32>(st + 160), ref, load<u32>(st + 172)});
        const u32 mask = load<u8>(g19::kStencilMaskByte);
        store<u32>(st + 168, mask);
        emit({sp::kStencilMask, mask});
    }
    const u32 mesh = load<u32>(item + 12);
    fx::vcall(ctx, mesh, 16, {mesh, item});
    if (((load<u8>(item + 68) >> 7) & 1) != 0) {
        emit({l4::kCullEnable, 1});
        emit({l4::kFrontFace, g19::kGcmCcw});
    }
    translucent_end(ctx, item);
}

}  // namespace setup

// void DrawItem_SetupStencil_005D65C0(-, DrawItem* item)  [virtual] (name?)
// setup::stencil_body with its 1 / 255 at .text 0x005D65BC.
DECOMP(005D65C0, DrawItem_SetupStencil_005D65C0) { setup::stencil_body(ctx, g19::kInv255_D65BC); }

// void DrawItem_SetupStencil_005D8100(-, DrawItem* item)  [virtual] (name?)
// setup::stencil_body with its 1 / 255 at .text 0x005D80FC.
DECOMP(005D8100, DrawItem_SetupStencil_005D8100) { setup::stencil_body(ctx, g19::kInv255_D80FC); }
