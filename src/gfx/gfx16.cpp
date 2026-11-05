// The renderer range's next recompiled functions: the frame start, the
// debug shape list, ... Names carry the address where the role is unclear.
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>

#include "fx.h"
#include "gfx16.h"

using namespace sly4;

namespace {

namespace g10 = sly4::render::gfx10;
namespace g11 = sly4::render::gfx11;
namespace g12 = sly4::render::gfx12;
namespace g13 = sly4::render::gfx13;
namespace g14 = sly4::render::gfx14;
namespace g15 = sly4::render::gfx15;
namespace g16 = sly4::render::gfx16;
namespace l4 = sly4::render::layers4;
using g11::lfs;
using g11::sgl;

// The original's lwarx / stwcx. loop: an atomic store of v.
void atomic_set(u32 ea, u32 v) { std::atomic_ref<u32>(ref<u32>(ea)).store(__builtin_bswap32(v)); }

// Appends s to a shader type's list (PtrVector +12: +12 data, u16 +16
// capacity, +20 count, +22 high-water mark), doubling the capacity when
// full (shader3_types.cpp's push). Returns the new high-water mark (the
// original's r3).
u32 push_shader16(PPUContext& ctx, u32 type, u32 s) {
    if (load<u16>(type + 20) == load<u16>(type + 16))
        call(ctx, g16::fn::PtrVector_Reserve, {type + 12, (static_cast<u32>(load<u16>(type + 16)) << 1) & 0x1FFFEu});  // rlwinm 1,15,30
    const u32 count = load<u16>(type + 20);
    store<u32>(load<u32>(type + 12) + ((count << 2) & 0x3FFFCu), s);  // rlwinm 2,14,29
    const u32 n = (load<u16>(type + 20) + 1u) & 0xFFFF;
    const u32 high = load<u16>(type + 22);
    store<u16>(type + 20, static_cast<u16>(n));
    store<u16>(type + 22, static_cast<u16>(high > n ? high : n));
    return high > n ? high : n;
}

// A new 44-byte shader (vtable vt) with the programs named at vname /
// fname (read after the allocation), or 0 when the allocation fails.
u32 make_shader16(PPUContext& ctx, u32 vt, u32 vname_at, u32 fname_at, u32 vparams, u32 fparams) {
    const u32 s = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {g16::kShaderSize, 0, 16}));
    if (s == 0) return 0;
    const u32 vname = load<u32>(vname_at), fname = load<u32>(fname_at);
    call(ctx, g16::fn::Shader_Construct, {s});
    store<u8>(s + 12, 0);
    store<u32>(s, vt);
    call(ctx, g16::fn::Shader_Load, {s, vname, fname, vparams, fparams});
    return s;
}

// The SHO types' Create: per entry of the 8-entry name table one shader
// (+0, +8) and its UVX twin (+4, +8); an entry with a null +0 adds two
// null entries. Returns the last high-water mark (the original's r3).
u32 add_sho_table16(PPUContext& ctx, u32 t, const g16::ShoType& ty) {
    u32 r3 = 0;
    for (u32 i = 0; i < g16::kShoTableCount; ++i) {
        const u32 e = ty.table + 12 * i;
        if (load<u32>(e) == 0) {
            push_shader16(ctx, t, 0);
            r3 = push_shader16(ctx, t, 0);
            continue;
        }
        push_shader16(ctx, t, make_shader16(ctx, ty.vtable, e, e + 8, ty.vparams, ty.fparams));
        r3 = push_shader16(ctx, t, make_shader16(ctx, ty.vtable, e + 4, e + 8, ty.vparams, ty.fparams));
    }
    return r3;
}

}  // namespace

// void Render_BeginFrame_0051008C()  [called by f_0049AE40] (name?)
// The frame start:
// - the system state (sly4.h system_state, built on first use) +4 is
//   added to the frame time (0x010DD9E0, read before the build); the step
//   block 0x010DD5A8 gets +4 = +0, +8 += +0, +12 = 1.0, +0 = 0; config
//   u8 +80 = 1; Call_0053BBE0;
// - CULL_FACE_ENABLE 1 and FRONT_FACE CCW written inline;
// - the draw-record list (built once, as DrawRecordList_Init) moves to
//   its other block (+176 toggled, the block's +8 bits 1..3 = 2, +180 /
//   +184 its start, the atomic end = start + base);
// - the frame arena's cursor, low and high reset to the current buffer
//   (+12 [index], 16-aligned; high = base + size, also stored atomically
//   through +256) and its second half's start / end (+264 [index],
//   128-aligned / + size); the stencil clear flag cleared;
// - FrameBuffer_Begin(the default frame buffer), StreamBuffers_Flip;
// - query_active = whether any of the 16 query slots is in use.
// r3 is left as the original leaves it.
DECOMP(0051008C, Render_BeginFrame_0051008C) {
    namespace g = g16::g;
    const f64 time = lfs(g::frame_time);
    const u32 st = system_state::object;
    if (load<u8>(system_state::guard) == 0) {  // system_state::get, with logged stores
        const u8 flags = load<u8>(st + 32);
        store<u32>(st + 20, 0);
        store<u32>(st + 24, 0);
        store<u8>(st + 32, static_cast<u8>(flags & ~0xC0));
        call(ctx, g16::fn::memset, {st + 92, 0, 400});
        store<u32>(system_state::registry_slot, st);
        store<u8>(system_state::guard, 1);
    }
    const f64 now = sgl(time + lfs(st + 4));
    const u32 step = load<u32>(g::frame_dt);
    const f64 sum = sgl(lfs(g::frame_dt + 8) + lfs(g::frame_dt));
    store<u32>(g::frame_dt + 4, step);  // lfs / stfs pair
    store<u32>(g::frame_dt + 12, 0x3F800000u);
    store<u8>(g::config_frame_flag, 1);
    store<u32>(g::frame_time, g11::stfs_bits(now));
    store<u32>(g::frame_dt + 8, g11::stfs_bits(sum));
    store<u32>(g::frame_dt, 0);
    call(ctx, g16::fn::Call_0053BBE0);

    fx::emit({l4::kCullEnable, 1});
    fx::emit({l4::kFrontFace, 0x901});  // CELL_GCM_CCW

    const u32 list = g::record_list;
    if (load<u8>(g::record_list_ready) == 0) {
        store<u8>(list, 1);
        call(ctx, g16::fn::PtrVector_Construct, {list + 4});
        store<u32>(list + 32, 0);
        store<u16>(list + 10, g::kRecordListCapacity);
        atomic_set(list + 128, 0);
        atomic_set(list + 256, 0);
        store<u8>(g::record_list_ready, 1);
    }
    {
        const u32 cur = load<u32>(list + 28);
        const u32 index = load<u32>(cur + 176) + 1;
        const u32 blk = ((cur & ~0x10u) | ((index << 4) & 0x10u)) + 128;  // rlwimi 4,27,27
        store<u32>(cur + 176, index & 1);
        store<u32>(blk + 8, (load<u32>(blk + 8) & ~0xEu) | (2u << 1));  // rlwimi 1,28,30
        const u32 base = load<u32>(cur + 160);
        const u32 start = load<u32>(blk);
        store<u32>(cur + 180, start);
        store<u32>(cur + 184, start);
        atomic_set(cur + 256, start + base);
    }
    {
        const u32 a = g::arena;
        const u32 index = load<u32>(a);
        store<u8>(g::clear_stencil, 0);
        const u32 size = load<u32>(a + 8);
        const u32 buf = load<u32>(a + (index << 2) + 12);
        const u32 end_ptr = load<u32>(a + 256);
        store<u32>(a + 28, buf + size);
        store<u32>(a + 20, (buf + 15) & ~15u);
        store<u32>(a + 24, (buf + 15) & ~15u);
        atomic_set(end_ptr, buf + size);
        const u32 index2 = load<u32>(a);
        const u32 size2 = load<u32>(a + 260);
        const u32 buf2 = load<u32>(a + (index2 << 2) + 264);
        store<u32>(a + 276, buf2 + size2);
        store<u32>(a + 272, (buf2 + 127) & ~127u);
    }
    call(ctx, g16::fn::FrameBuffer_Begin, {load<u32>(g::default_frame_buffer)});
    call(ctx, g16::fn::StreamBuffers_Flip_0050FE10);
    store<u8>(g::query_active, 0);
    for (u32 i = 0; i < g16::kQuerySlots; ++i) {
        if (load<u8>(g::query_slots + g16::kQuerySlotSize * i + 4) != 0) {
            store<u8>(g::query_active, 1);
            ctx.gpr[3] = 0x010E0000u;  // as the original leaves it (lis)
            return;
        }
    }
    ctx.gpr[3] = g::query_slots + g16::kQuerySlotSize * g16::kQuerySlots;  // as the original leaves it
}

// void ShapeList_Draw_00518B64(LineBuf* o, u32 key)  [called by f_00518E78] (name?)
// Draws the debug shapes (g16::g::shapes, u16 shape_count) whose +96 is
// key. With any shapes, o's buffers (+4 48 KiB, +304 1 KiB) are allocated
// on first use. When one matches: o's Begin (vtable +20); then for each
// matching entry with a shape list (+84): o->+316 = the list's f32 +16,
// the entry's +64 position through its 3x4 matrix (Vec3_RotateTo), and
// per shape s (list +8, then s->+8 ...): with the list's u8 +12 set a
// circle (Lines_AddCircle_00518A20, radius +80), else every skin on the
// list (+8 chain, node +0 a SkinMesh whose mesh has +72 bone weights)
// transformed by the entry and o's Shape (+24: s, the position, u8 +93;
// f1 = +80); a shape with a u16 +4 count also gets o's Skin (+28: s, the
// entry, o->+4). Then o's Flush (+32: 0, 0, 1), End (+36), the stencil
// clear flag set and both state caches reset to -1.
DECOMP(00518B64, ShapeList_Draw_00518B64) {
    namespace g = g16::g;
    namespace vt = g16::lines_vt;
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    u32 count = load<u16>(g::shape_count);
    if (count == 0) return ret(ctx, count);
    if (load<u32>(o + 4) == 0) {
        store<u32>(o + 4, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {g16::kLineBufSize, 0, 16})));
        const u32 aux = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {g16::kLineBufAuxSize, 0, 16}));
        count = load<u16>(g::shape_count);
        store<u32>(o + 304, aux);
    }
    bool found = false;
    for (u32 i = 0; i < count; ++i)
        if (load<u32>(g::shapes + g16::kShapeSize * i + 96) == key) {  // cmpw
            found = true;
            break;
        }
    if (!found) return ret(ctx, count);

    fx::vcall(ctx, o, vt::kBegin, {o});
    if (load<u16>(g::shape_count) != 0) {
        Stack frame(ctx, 16);
        const u32 pos = frame.addr();
        for (u32 i = 0; i < load<u16>(g::shape_count); ++i) {
            const u32 e = g::shapes + g16::kShapeSize * i;
            if (load<u32>(e + 96) != key) continue;
            const u32 head = load<u32>(e + 84);
            if (head == 0) continue;
            store<u32>(o + 316, load<u32>(head + 16));  // lfs / stfs pair
            call(ctx, g16::fn::Vec3_RotateTo, {pos, e + 64, e});
            for (u32 s = load<u32>(load<u32>(e + 84) + 8); s != 0; s = load<u32>(s + 8)) {
                const u32 lst = load<u32>(e + 84);
                if (load<u8>(lst + 12) != 0) {
                    callf(ctx, g16::fn::Lines_AddCircle_00518A20, {o, s, pos}, {lfs(e + 80)});
                } else {
                    for (u32 n = load<u32>(lst + 8); n != 0; n = load<u32>(n + 8)) {
                        const u32 skin = load<u32>(n);
                        if (load<u32>(load<u32>(skin) + 72) != 0)
                            call(ctx, g16::fn::Skin_TransformPositions_00518748, {skin, e});
                    }
                    ctx.fpr[1] = lfs(e + 80);
                    fx::vcall(ctx, o, vt::kShape, {o, s, pos, load<u8>(e + 93)});
                }
                if (load<u16>(s + 4) != 0) {
                    // This call site loads the entry into r7 and the OPD into r8.
                    const u32 opd = load<u32>(load<u32>(o) + vt::kSkin);
                    call_ptr(ctx, opd, {o, s, e, load<u32>(o + 4), load<u32>(opd), opd});
                }
            }
        }
    }
    fx::vcall(ctx, o, vt::kFlush, {o, 0, 0, 1});
    fx::vcall(ctx, o, vt::kEnd, {o});
    store<u8>(g::clear_stencil, 1);
    store<u64>(g::state_cache_a, ~0ull);
    store<u64>(g::state_cache_b, ~0ull);
    ctx.gpr[3] = 0x010E0000u;  // as the original leaves it (lis)
}

// void Bounds_Draw_005ABB58(const Box* box, const f32 m[4][4])  [called by f_005ABE5C] (name?)
// Queues the bounds cube (0x00F8895C) with the bounds material
// (0x00F88960) as an 80-byte draw item from the frame arena (unchecked):
// the box {min +0, max +16} becomes the scale (max - min + 0.01 per
// axis) and centre ((min + max) * 0.5); the item's rows are m (read with
// lvx) times Matrix_ScaleDiag(scale)'s rows 0..2 (per lane, in vmaddfp
// order: m0 * x + 0, + m1 * y, + m2 * z) and row 3 m0 cx + m1 cy + m2 cz
// + m3 (xyz of each stored); item +12 the cube, +28 the material, +72 the
// white colour, +44 / +60 / +64 / +69 / +70 0, +71 255, u8 +68 keeps bit
// 0 only. Then Material_Bind(material, shader) and the shader's slot 3
// (+12) on the item, the shader being material->+16->+12->+0.
DECOMP(005ABB58, Bounds_Draw_005ABB58) {
    namespace g = g16::g;
    const u32 box = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1) & ~15u;
    const f64 half = lfs(g16::kBoundsConsts), pad = lfs(g16::kBoundsConsts + 4);
    const f64 x0 = lfs(box), y0 = lfs(box + 4), z0 = lfs(box + 8);
    const f64 x1 = lfs(box + 16), y1 = lfs(box + 20), z1 = lfs(box + 24);
    auto bits = [](f64 v) { return std::bit_cast<f32>(g11::stfs_bits(v)); };
    const f32 cz = bits(sgl(sgl(z0 + z1) * half));
    const f32 cx = bits(sgl(sgl(x0 + x1) * half)), cy = bits(sgl(sgl(y0 + y1) * half));
    Stack frame(ctx, 96);
    const u32 scale = frame.addr(), s = scale + 16;
    store<u32>(scale, g11::stfs_bits(sgl(sgl(x1 - x0) + pad)));
    store<u32>(scale + 4, g11::stfs_bits(sgl(sgl(y1 - y0) + pad)));
    store<u32>(scale + 8, g11::stfs_bits(sgl(sgl(z1 - z0) + pad)));
    call(ctx, g16::fn::Matrix_ScaleDiag, {s, scale});

    f32 rows[4][4];
    for (u32 i = 0; i < 3; ++i) {
        const f32 x = load<f32>(s + 16 * i), y = load<f32>(s + 16 * i + 4), z = load<f32>(s + 16 * i + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(m + 4 * k), x, 0.0f);
            v = std::fmaf(load<f32>(m + 16 + 4 * k), y, v);
            rows[i][k] = std::fmaf(load<f32>(m + 32 + 4 * k), z, v);
        }
    }
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(load<f32>(m + 4 * k), cx, 0.0f);
        v = std::fmaf(load<f32>(m + 16 + 4 * k), cy, v);
        v = std::fmaf(load<f32>(m + 32 + 4 * k), cz, v);
        rows[3][k] = load<f32>(m + 48 + 4 * k) + v;  // vaddfp
    }

    const u32 mat = load<u32>(g::bounds_material);
    const u32 cube = load<u32>(g::bounds_cube);
    const u32 shader = load<u32>(load<u32>(load<u32>(mat + 16) + 12));
    const u32 item = load<u32>(g::arena + 20);
    store<u32>(g::arena + 20, item + g16::kBoundsItemSize);
    store<u32>(item + 12, cube);
    store<u32>(item + 28, mat);
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 3; ++k) store<u32>(item + 16 * i + 4 * k, std::bit_cast<u32>(rows[i][k]));  // lfs / stfs pairs
    for (u32 k = 0; k < 4; ++k) store<u8>(item + 72 + k, load<u8>(g15::g::white + k));
    const u8 flags = load<u8>(item + 68);
    store<u32>(item + 44, 0);
    store<u32>(item + 64, 0);
    store<u8>(item + 69, 0);
    store<u32>(item + 60, 0);
    store<u8>(item + 70, 0);
    store<u8>(item + 71, 255);
    store<u8>(item + 68, static_cast<u8>(flags & 1));
    call(ctx, g15::fn::Material_Bind, {load<u32>(g::bounds_material), shader});
    fx::vcall(ctx, shader, g16::kShaderDrawItem, {shader, item});
}

// u32 FlipEntry_DrawBounds_005ABE5C(FlipEntry* e)  [called by FlipList_Process_005AC54C] (name?)
// The occlusion pass of a flip-list entry (+8 object, +12 surface, +16
// box, +48 matrix): Call_00548508(surface->+672); colour writes off,
// stencil test on, then (through the stencil cache at fx::g::state +160
// func, +164 ref, +168 write mask, +172 mask, +176 / +180 / +184 fail /
// zfail / zpass) mask 0, write mask 255, ops KEEP / INCR, func ALWAYS
// ref 0, ops KEEP x 3 ... Bounds_Draw_005ABB58(e + 16, e + 48); colour
// writes on, mask 255, write mask 0, func NOTEQUAL ref 0; the object's
// slot 14 (+56) draws it, and Sync_00548550(surface, 0x00F3CA38).
// Each SET_STENCIL_* command is written with the cache's other fields.
DECOMP(005ABE5C, FlipEntry_DrawBounds_005ABE5C) {
    namespace sc = g16::stencil_cache;
    const u32 e = arg<u32>(ctx, 0);
    const u32 obj = load<u32>(e + 8);
    const u32 surface = load<u32>(load<u32>(e + 12) + 672);
    call(ctx, g16::fn::Call_00548508, {surface});
    const u32 st = fx::g::state;
    auto f = [&](u32 off) { return load<u32>(st + off); };
    auto set = [&](u32 off, u32 v) { store<u32>(st + off, v); };
    fx::emit({g16::kColorMask, 0});
    fx::emit({g16::kStencilTestEnable, 1});
    set(sc::kMask, 0);
    fx::emit({g16::kStencilFunc, f(sc::kFunc), f(sc::kRef), 0});
    set(sc::kWriteMask, 255);
    fx::emit({g16::kStencilWriteMask, 255});
    set(sc::kFail, g16::kKeep);
    set(sc::kZPass, g16::kIncr);
    fx::emit({g16::kStencilOp, g16::kKeep, f(sc::kZFail), g16::kIncr});
    set(sc::kFunc, g16::kAlways);
    fx::emit({g16::kStencilFunc, g16::kAlways, f(sc::kRef), f(sc::kMask)});
    set(sc::kRef, 0);
    fx::emit({g16::kStencilFunc, f(sc::kFunc), 0, f(sc::kMask)});
    set(sc::kZFail, g16::kKeep);
    fx::emit({g16::kStencilOp, f(sc::kFail), g16::kKeep, f(sc::kZPass)});
    call(ctx, g16::fn::Bounds_Draw_005ABB58, {e + 16, e + 48});
    fx::emit({g16::kColorMask, 0x01010101});
    set(sc::kMask, 255);
    fx::emit({g16::kStencilFunc, f(sc::kFunc), f(sc::kRef), 255});
    set(sc::kWriteMask, 0);
    fx::emit({g16::kStencilWriteMask, 0});
    set(sc::kFail, g16::kKeep);
    set(sc::kZPass, g16::kKeep);
    fx::emit({g16::kStencilOp, g16::kKeep, f(sc::kZFail), g16::kKeep});
    set(sc::kFunc, g16::kNotEqual);
    fx::emit({g16::kStencilFunc, g16::kNotEqual, f(sc::kRef), f(sc::kMask)});
    set(sc::kRef, 0);
    fx::emit({g16::kStencilFunc, f(sc::kFunc), 0, f(sc::kMask)});
    fx::vcall(ctx, obj, g16::kObjDrawBounded, {obj});
    call(ctx, g16::fn::Sync_00548550, {surface, g16::opd_flip_sync});
}

// void SHODefaultType_Create(ShaderType* t)  [ShaderType slot 0, the "SHO_Default" type]
// The type's 16 shaders from the 8-entry name table at 0x00F8AFA0
// ({+0 vertex program, +4 its UVX twin, +8 fragment program}: SHO_Default,
// _NoFog, the _NoFade pair, _Lit, _Lit_1 .. _Lit_3): per entry one shader
// (+0, +8) and its UVX twin (+4, +8), vtable 0x00B260F8, parameters
// 0x00F8AF80 / 0x00F8AF68; an entry with a null +0 (none in the table)
// adds two null entries. (shader3_types.cpp's add_table.)
DECOMP(005D6D50, SHODefaultType_Create) {
    ctx.gpr[3] = add_sho_table16(ctx, arg<u32>(ctx, 0), g16::sho_default);
}

// void SHONightBlendType_Create(ShaderType* t)  [ShaderType slot 0, the "SHO_NightBlend" type]
// The same for SHO_NightBlend: name table 0x00F8B088, vtable 0x00B26198,
// parameters 0x00F8B078 / 0x00F8B058.
DECOMP(005D8890, SHONightBlendType_Create) {
    ctx.gpr[3] = add_sho_table16(ctx, arg<u32>(ctx, 0), g16::sho_night_blend);
}

// bool Iff_HandleMaterialAnim_0051CD48(IffReader* r, AglMaterial* m)  [IFF chunk handler] (name?)
// Reads an unnamed u8 (the version), then the animation record (36
// bytes, zeroed first): f32 "speed", "steps", "limit", "delayMin",
// "delayMax", "playTime", "uOffset", "vOffset" and u8 +32 "mode", +33
// "channel", +34 "flags". A material without an animation (+128) gets
// one (152 bytes) from the AglMaterialAnim free list (0x011DE4B8, made on
// first use: 256 per block): +0 0, u8 +4 0, three 48-byte tracks at +8
// {0, 0, 1.0, 0, 0, 0, 0.5, 0.5, +32 low 12 bits kept and bits 12..19 =
// 1, 0, 0, 0}. FogRanges_Set_0050B6D8(the animation, &record) then
// stores the record in its channel's track. Returns 1.
// Quirks kept: a failed Mem_Alloc stores a null list and calls through
// it; a failed allocation leaves +128 0 and passes 0 on.
DECOMP(0051CD48, Iff_HandleMaterialAnim_0051CD48) {
    namespace g = g16::g;
    const u32 r = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    Stack frame(ctx, 48);
    const u32 rec = frame.addr(), ver = rec + 36;
    for (u32 k = 0; k < 9; ++k) store<u32>(rec + 4 * k, 0);
    call(ctx, g16::fn::Iff_ReadU8, {r, ver, 0});
    for (u32 k = 0; k < 8; ++k) call(ctx, g16::fn::FileReader_ReadF32Swapped, {r, rec + 4 * k, g16::str_anim_fields[k]});
    for (u32 k = 0; k < 3; ++k) call(ctx, g16::fn::Iff_ReadU8, {r, rec + 32 + k, g16::str_anim_fields[8 + k]});
    u32 anim = load<u32>(m + g16::kMatAnim);
    if (anim == 0) {
        u32 list = load<u32>(g::anim_free_list);
        if (list == 0) {
            const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {g16::kFreeListObjSize, 0, 16}));
            if (obj != 0) {
                for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
                const u16 h54 = load<u16>(obj + 54);
                for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
                store<u16>(obj + 52, 0);
                store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
                store<u32>(obj, g16::vt_free_list);
                store<u32>(obj + 56, g16::str_anim_free_list);
                call(ctx, g16::fn::FreeList_Init, {obj, g16::kAnimSize, g16::kAnimPerBlock, 0, 0});
            }
            list = obj;
            store<u32>(g::anim_free_list, list);
        }
        anim = static_cast<u32>(fx::vcall(ctx, list, g16::kFreeListAlloc, {list}));
        if (anim != 0) {
            store<u32>(anim, 0);
            store<u8>(anim + 4, 0);
            for (u32 k = 0; k < 3; ++k) {
                const u32 t = anim + 8 + 48 * k;
                const u32 w = (load<u32>(t + 32) & 0x00000FFFu) | (1u << 12);  // rlwinm 0,4,31; rlwimi 0,4,11; rlwimi 12,12,19
                store<u32>(t, 0);
                store<u32>(t + 4, 0);
                store<u32>(t + 8, 0x3F800000u);  // 1.0
                store<u32>(t + 12, 0);
                store<u32>(t + 16, 0);
                store<u32>(t + 20, 0);
                store<u32>(t + 24, 0x3F000000u);  // 0.5
                store<u32>(t + 28, 0x3F000000u);
                store<u32>(t + 32, w);
                store<u32>(t + 36, 0);
                store<u32>(t + 40, 0);
                store<u32>(t + 44, 0);
            }
        }
        store<u32>(m + g16::kMatAnim, anim);
    }
    call(ctx, g16::fn::FogRanges_Set_0050B6D8, {anim, rec});
    ret(ctx, 1u);
}

namespace {
// Material_ParamSlot(m, k) = v.
void set_param16(PPUContext& ctx, u32 m, u32 k, u32 v) {
    store<u32>(static_cast<u32>(call(ctx, g15::fn::Material_ParamSlot, {m, k})), v);
}
}  // namespace

// bool SlyCharacter_LoadParams(ShaderType*, Material* m, const Desc* d)
// [vtable 0x00B25900 slot 3, the "SlyCharacter" type's LoadParams] (name?)
// As LoadParams_005C3528 (gfx15.cpp): nothing for desc version (u8 d+84)
// 0. The texture named d+4 (slot 4 with (t, 1, 3, 1)) is added, m+120's
// top nibble = 2 when u8 d+76 == 2, else 3; without it the default
// texture (kind 5) with 3. Unless config +152 bit 7: the texture named
// d+8 is added and its slot 4 gets (t, w, h, 1) with w / h = m->+20's u16
// +122 / +124, params 0..2 = d+88 / +92 / +96, then unless config +152
// bit 6 the texture named d+12 likewise; without d+8's texture the one
// named d+16 (if loaded) gets its slot 3. Param 4 = (version > 4 ? d+104
// : 15), 0 when not positive; param 7 = 0. Returns 1.
DECOMP(005BC4F0, SlyCharacter_LoadParams) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if (static_cast<s32>(ref<u8>(d + 84)) <= 0) return ret(ctx, 1u);  // cmpwi
    const u32 t = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 4)}));
    u32 kind;
    if (t != 0) {
        fx::vcall(ctx, t, 16, {t, 1, 3, 1});
        call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t});
        kind = ref<u8>(d + 76) == 2 ? 2u : 3u;
    } else {
        call(ctx, g15::fn::Material_AddTexture_004FF008, {m, static_cast<u32>(call(ctx, g15::fn::Tex_GetDefault, {g14::kDefaultTexKind}))});
        kind = 3;
    }
    store<u32>(m + 120, (load<u32>(m + 120) & 0x0FFFFFFFu) | (kind << 28));  // rlwinm 0,4,31; rlwimi 28
    if ((field<u8>(sly4::g::config, 152) & 0x80) == 0) {  // rlwinm 25,31,31
        const u32 t2 = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 8)}));
        if (t2 != 0) {
            call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t2});
            {
                const u32 sz = load<u32>(m + 20);
                const u32 w = load<u16>(sz + 122), h = load<u16>(sz + 124);
                // This call site leaves r7 = h and r8 = w.
                call_ptr(ctx, load<u32>(load<u32>(t2) + 16), {t2, w, h, 1, h, w});
            }
            for (u32 k = 0; k < 3; ++k) set_param16(ctx, m, k, load<u32>(d + 88 + 4 * k));  // lfs / stfs
            if ((field<u8>(sly4::g::config, 152) & 0x40) == 0) {  // rlwinm 26,31,31
                const u32 t3 = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 12)}));
                if (t3 != 0) {
                    call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t3});
                    const u32 sz = load<u32>(m + 20);
                    const u32 w = load<u16>(sz + 122), h = load<u16>(sz + 124);
                    const u32 opd = load<u32>(load<u32>(t3) + 16);
                    call_ptr(ctx, opd, {t3, w, h, 1, opd, load<u32>(opd)});
                }
            }
        } else if (load<u32>(d + 16) != 0 && static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 16)})) != 0) {
            const u32 t4 = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 16)}));
            fx::vcall(ctx, t4, 12, {t4});
        }
    }
    constexpr u32 K = g16::kSlyCharacterParams;
    const f64 x = static_cast<s32>(ref<u8>(d + 84)) > 4 ? lfs(d + 104) : lfs(K);  // cmpwi
    const f64 v = -x >= 0.0 ? g11::lfd(K + 8) : x;  // fneg, fsel
    ctx.fpr[1] = v;
    set_param16(ctx, m, 4, g11::stfs_bits(sgl(v)));  // frsp
    set_param16(ctx, m, 7, 0);
    ret(ctx, 1u);
}

// void Topology_Destruct(AglTopology* t)  [called by Delete_00508B00] (name?)
// The AglTopology destructor (vtable 0x00B23368): each draw entry
// ({obj, obj}, 8 bytes at +48, u16 +52 of them, last first) has both
// objects cleared and released (vtable +8); a draw table that isn't the
// inline one (+16) goes back to the AglTopologyMeshMtlTable free list
// (0x00F66EE8, made on first use: 320-byte entries, its +4 capacity; its
// vtable +16); the shared block (+88) is released, the record (+92) reset
// (TopoRecord_Set(rec, 0)), released and freed; the inline entries'
// objects (+16..+44, last first, second word first) released, not
// cleared; then ApeRoot_Destruct.
// Quirks kept: the count is left at 0xFFFF; +88 is tested again after
// being cleared; a failed free-list Mem_Alloc stores and calls through 0.
DECOMP(005087EC, Topology_Destruct) {
    const u32 t = arg<u32>(ctx, 0);
    store<u32>(t, g16::vt_topology);
    for (;;) {
        const u32 n = load<u16>(t + g16::kTopoDrawCount);
        store<u16>(t + g16::kTopoDrawCount, static_cast<u16>(n - 1));
        if (n == 0) break;
        for (u32 k = 0; k < 8; k += 4) {
            const u32 e = load<u32>(t + g16::kTopoDraws) + ((static_cast<u32>(load<u16>(t + g16::kTopoDrawCount)) << 3) & ~7u) + k;
            const u32 p = load<u32>(e);
            if (p == 0) continue;
            store<u32>(e, 0);
            fx::vcall(ctx, p, g16::kRelease, {p});
        }
    }
    const u32 draws = load<u32>(t + g16::kTopoDraws);
    if (draws != t + 16) {  // cmpw
        const u32 holder = load<u32>(g16::g::mesh_mtl_list);
        u32 list = load<u32>(holder);
        if (list == 0) {
            const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {g16::kFreeListObjSize, 0, 16}));
            if (obj != 0) {
                const u32 capacity = load<u32>(holder + 4);
                for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(obj + off, 0);
                const u16 h54 = load<u16>(obj + 54);
                for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
                store<u16>(obj + 52, 0);
                store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
                store<u32>(obj, g16::vt_mesh_mtl_list);
                store<u32>(obj + 56, g16::str_mesh_mtl_list);
                call(ctx, g16::fn::FreeList_Init, {obj, g16::kMeshMtlTableSize, capacity, 0, 0});
            }
            list = obj;
            store<u32>(holder, list);
        }
        fx::vcall(ctx, list, g16::kFreeListFree, {list, draws});
    }
    if (const u32 shared = load<u32>(t + g16::kTopoShared); shared != 0) {
        store<u32>(t + g16::kTopoShared, 0);
        call(ctx, g16::fn::TopoShared_Release, {shared});
    }
    if (const u32 rec = load<u32>(t + g16::kTopoRecord); rec != 0) {
        call(ctx, g16::fn::TopoRecord_Set, {rec, 0});
        call(ctx, g16::fn::TopoRecordRef_Release_0079C47C, {rec});
        call(ctx, g16::fn::Mem_FreeFrom, {rec, 0});
        store<u32>(t + g16::kTopoRecord, 0);
    }
    if (const u32 shared = load<u32>(t + g16::kTopoShared); shared != 0) call(ctx, g16::fn::TopoShared_Release, {shared});
    for (s32 i = 3; i >= 0; --i) {
        const u32 e = t + 16 + 8 * static_cast<u32>(i);
        if (const u32 p = load<u32>(e + 4); p != 0) fx::vcall(ctx, p, g16::kRelease, {p});
        if (const u32 p = load<u32>(e); p != 0) fx::vcall(ctx, p, g16::kRelease, {p});
    }
    call(ctx, g16::fn::ApeRoot_Destruct, {t});
}

namespace {
// Material_AddTexture_004FF008(m, t), then t's slot 4 (+16) with (t,
// m->+20 u16 +122 / +124, 1); `sizes_after`: the sequence leaves r7 =
// the OPD, r8 / r9 = the sizes (else r7 = the OPD, r8 = its entry).
// (gfx15.cpp add_sized_texture15.)
void add_sized_texture16(PPUContext& ctx, u32 m, u32 t, bool sizes_after) {
    call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t});
    const u32 sz = load<u32>(m + 20);
    const u32 w = load<u16>(sz + 122), h = load<u16>(sz + 124);
    const u32 opd = load<u32>(load<u32>(t) + 16);
    if (sizes_after) call_ptr(ctx, opd, {t, w, h, 1, opd, w, h});
    else call_ptr(ctx, opd, {t, w, h, 1, opd, load<u32>(opd)});
}
}  // namespace

// bool SlyDecal_LoadParams(ShaderType*, Material* m, const Desc* d)
// [vtable 0x00B25B68 slot 3, the "SlyDecal" type's LoadParams] (name?)
// Nothing (returns 1) for desc version (u8 d+84) 0; returns 0 when the
// texture named d+16 isn't loaded. That texture (slot 4 with (t, 1, 1,
// 1)) is added, m+120's top nibble = 2 when u8 d+76 == 2, else 3, params
// 0 / 1 = d+88 / +92. Unless config +152 bit 7, when the texture named
// d+8 or the one named d+4 is loaded, both are added and sized (a missing
// one replaced by the default texture of kind 4), params 2..4 = d+96 /
// +100 / +104, and unless config +152 bit 6 the texture named d+12 (when
// loaded) is added and sized. Returns 1.
DECOMP(005C45D0, SlyDecal_LoadParams) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if (static_cast<s32>(ref<u8>(d + 84)) <= 0) return ret(ctx, 1u);  // cmpwi
    const u32 t = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 16)}));
    if (t == 0) return ret(ctx, 0u);
    fx::vcall(ctx, t, 16, {t, 1, 1, 1});
    call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t});
    const u32 kind = ref<u8>(d + 76) == 2 ? 2u : 3u;
    store<u32>(m + 120, (load<u32>(m + 120) & 0x0FFFFFFFu) | (kind << 28));  // rlwinm 0,4,31; rlwimi 28,2,3
    for (u32 k = 0; k < 2; ++k) set_param16(ctx, m, k, load<u32>(d + 88 + 4 * k));  // lfs / stfs
    if ((field<u8>(sly4::g::config, 152) & 0x80) != 0) return ret(ctx, 1u);  // rlwinm 25,31,31
    u32 t2 = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 8)}));
    u32 t1 = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 4)}));
    if (t2 == 0 && t1 == 0) return ret(ctx, 1u);
    if (t2 == 0) t2 = static_cast<u32>(call(ctx, g15::fn::Tex_GetDefault, {g16::kDecalDefaultTexKind}));
    add_sized_texture16(ctx, m, t2, true);
    if (t1 == 0) t1 = static_cast<u32>(call(ctx, g15::fn::Tex_GetDefault, {g16::kDecalDefaultTexKind}));
    add_sized_texture16(ctx, m, t1, false);
    for (u32 k = 2; k < 5; ++k) set_param16(ctx, m, k, load<u32>(d + 88 + 4 * k));  // lfs / stfs
    if ((field<u8>(sly4::g::config, 152) & 0x40) != 0) return ret(ctx, 1u);  // rlwinm 26,31,31
    const u32 t3 = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 12)}));
    if (t3 != 0) add_sized_texture16(ctx, m, t3, false);
    ret(ctx, 1u);
}

// u32 EdgeDecompressor_Construct(EdgeDecompressor* q, CellSpurs* spurs)  [called by Fios_Init] (name?)
// The Edge zlib queue wrapper (vtable 0x00B245A4, +4 'zlib'): the header
// flags word +20 keeps its low 10 bits with 0xA800 set; the job table
// (gfx14.h kJobQueue*) cleared: +8 / +12 0x10000, +11972 spurs, +11976
// the taskset, +11980 / +11984 0, +11988 -1, u8 +11992 off. With a SPURS
// instance, first a taskset "edgeDecompressorTaskset" at +4224 (8 SPU
// priorities of 15, at most 8 contention; 0 on any failure). With both:
// when its slot 6 (+24) answers at most 0x1000, the event flag +10624
// (SPURS-wide, clear mode 0, direction 1) is initialised and attached;
// then slot 7 (+28) is asked with +11984 = 1, 2, ... until it answers more
// than 0x200 (+11984 left one back), +11980 = slot 8 (+32), +11988 = slot
// 9 (+36) and, when those succeed (non-zero, not -1), the 704-byte slot
// table +11268 cleared and the queue switched on. r3 is left as the
// original leaves it (the last result).
DECOMP(007A5350, EdgeDecompressor_Construct) {
    namespace eq = g16::edge_queue;
    const u32 q = arg<u32>(ctx, 0), spurs = arg<u32>(ctx, 1);
    auto flags = [&]() { return (load<u32>(q + eq::kFlags) & 0x03FFu) | 0xA800u; };  // rlwimi 24,0,7; 16,8,15; or / rlwimi
    {
        const u32 w = flags();
        store<u8>(q + 16, 0);
        store<u32>(q + g14::kJobQueueMask, 0);
        store<u32>(q + 16, 0);
        store<u8>(q + 20, 0);
        store<u8>(q + 21, 0);
        store<u32>(q, g16::vt_edge_queue);
        store<u32>(q + eq::kFlags, w);
        store<u32>(q + 4, eq::kTagZlib);
    }
    u32 taskset = 0;
    if (spurs != 0) {
        Stack frame(ctx, 16 + eq::kTasksetAttrSize);
        const u32 prio = frame.addr(), attr = prio + 16;
        for (u32 k = 0; k < 8; ++k) store<u8>(prio + k, 15);
        store<u32>(q + 12, 0x10000);
        store<u32>(q + 8, 0x10000);
        store<u8>(q + g14::kJobQueueOn, 0);
        store<u32>(q + eq::kAnswer36, 0xFFFFFFFFu);
        store<u32>(q + eq::kCounter, 0);
        store<u32>(q + eq::kAnswer32, 0);
        store<u32>(q + eq::kTaskset, 0);
        store<u32>(q + eq::kSpurs, 0);
        if (call(ctx, g16::fn::_cellSpursTasksetAttributeInitialize, {attr, 1, eq::kSdkVersion, 0, prio, 8, ~0ull, 0}) == 0 &&
            call(ctx, g16::fn::cellSpursTasksetAttributeSetName, {attr, g16::str_edge_taskset}) == 0 &&
            call(ctx, g16::fn::cellSpursCreateTasksetWithAttribute, {spurs, q + eq::kTasksetObj, attr}) == 0)
            taskset = q + eq::kTasksetObj;
    }
    const u32 w = flags();
    store<u32>(q + g14::kJobQueueMask, 0);
    store<u32>(q + eq::kSpurs, spurs);
    store<u32>(q + eq::kTaskset, taskset);
    store<u32>(q + eq::kAnswer32, 0);
    store<u32>(q + eq::kCounter, 0);
    store<u32>(q + eq::kAnswer36, 0xFFFFFFFFu);
    store<u8>(q + g14::kJobQueueOn, 0);
    store<u32>(q + 8, 0x10000);
    store<u8>(q + 16, 0);
    store<u32>(q + 12, 0x10000);
    store<u32>(q + 16, 0);
    store<u8>(q + 20, 0);
    store<u8>(q + 21, 0);
    store<u32>(q + eq::kFlags, w);
    ctx.gpr[3] = w;
    if (spurs == 0 || taskset == 0) return;
    if (static_cast<u32>(fx::vcall(ctx, q, eq::kSlotA, {q})) > eq::kLimitA) return;  // cmplwi
    if (call(ctx, g16::fn::_cellSpursEventFlagInitialize, {load<u32>(q + eq::kSpurs), 0, q + g14::kJobQueueFlag, 0, 1}) != 0) return;
    if (call(ctx, g16::fn::cellSpursEventFlagAttachLv2EventQueue, {q + g14::kJobQueueFlag}) != 0) return;
    for (;;) {
        const u32 c = load<u32>(q + eq::kCounter) + 1;
        store<u32>(q + eq::kCounter, c);
        if (static_cast<u32>(fx::vcall(ctx, q, eq::kSlotB, {q, c})) > eq::kLimitB) break;  // cmplwi
    }
    store<u32>(q + eq::kCounter, load<u32>(q + eq::kCounter) - 1);
    const u32 a = static_cast<u32>(fx::vcall(ctx, q, eq::kSlotC, {q}));
    store<u32>(q + eq::kAnswer32, a);
    if (a == 0) return;
    const u32 b = static_cast<u32>(fx::vcall(ctx, q, eq::kSlotD, {q}));
    store<u32>(q + eq::kAnswer36, b);
    if (b == 0xFFFFFFFFu) return;  // cmpwi -1
    call(ctx, g16::fn::memset, {q + g14::kJobQueueSlots, 0, eq::kSlotTableSize});
    store<u8>(q + g14::kJobQueueOn, 1);
    ctx.gpr[3] = 1;  // as the original leaves it
}

// void EmitProps_AddMesh_004FEBF0(AglEmitProperties* p, Mesh* mesh)  [called by EmitProps_SetDesc_004FF4C0] (name?)
// Adds a mesh to the emitter: mesh -> p->+72[u8 +88] (then +88 ++), u8
// +68 = 1, and u16 +90 = the largest command size (in words) of a mesh
// draw so far, measured by writing one into a 256-byte scratch context
// (Buffer_Init_005DF960, callback 0x00F3BCF0): the vertex constants 0..15
// (identity_164C10) and 4..7 (0x01164BC0), then the draw (indexed when
// mesh +64 has an index count: mode u8 +76, type u8 +60, location /
// offset from +48 bit 0 / >> 1; else DrawArrays(mode, 0, +40)). The
// mesh's u16 +4 references ++ and its material's (+16) shader (+16)
// becomes the "emitter mesh" shader (hash 0x59D9B0B7, looked up once).
// r3 is left at the material.
DECOMP(004FEBF0, EmitProps_AddMesh_004FEBF0) {
    namespace g = g16::g;
    const u32 p = arg<u32>(ctx, 0), mesh = arg<u32>(ctx, 1);
    store<u32>(p + 72 + ((static_cast<u32>(load<u8>(p + 88)) << 2) & ~3u), mesh);
    store<u8>(p + 68, 1);
    Stack frame(ctx, 16 + g16::kScratchCmdSize);
    const u32 c = frame.addr(), buf = c + 16;
    ctx.gpr[7] = 1;  // as the original leaves it (the +68 value)
    call(ctx, g16::fn::Buffer_Init_005DF960, {c, buf, g16::kScratchCmdSize, g16::opd_scratch_callback});
    call(ctx, g16::fn::cellGcmSetVertexProgramConstants, {c, 0, 16, g14::g::identity_164C10});
    call(ctx, g16::fn::cellGcmSetVertexProgramConstants, {c, 4, 4, g16::g::vconst_164BC0});
    if (const u32 count = load<u32>(mesh + 64); count != 0) {
        const u32 w = load<u32>(mesh + 48);
        call(ctx, g16::fn::cellGcmSetDrawIndexArray,
             {c, load<u32>(mesh + 76) & 0xFF, count, load<u32>(mesh + 60) & 0xFF, w & 1, w >> 1});
    } else {
        call(ctx, g16::fn::cellGcmSetDrawArrays, {c, load<u32>(mesh + 76) & 0xFF, 0, load<u32>(mesh + 40)});
    }
    const u32 words = static_cast<u32>((static_cast<s32>(load<u32>(c + 8) - load<u32>(c))) / 4) & 0xFFFF;  // srawi / addze
    const u32 high = load<u16>(p + 90);
    store<u16>(p + 90, static_cast<u16>(high > words ? high : words));
    store<u8>(p + 88, static_cast<u8>(load<u8>(p + 88) + 1));
    store<u16>(mesh + 4, static_cast<u16>(load<u16>(mesh + 4) + 1));
    if (load<u8>(g::mesh_shader_ready) == 0) {
        const u32 s = static_cast<u32>(call(ctx, g15::fn::Shader_FindByHash, {g16::kEmitMeshShaderHash}));
        store<u32>(g::mesh_shader, s);
        store<u8>(g::mesh_shader_ready, 1);
    }
    const u32 mat = load<u32>(mesh + 16);
    const u32 s = load<u32>(g::mesh_shader);
    if (load<u32>(mat + 16) != s) store<u32>(mat + 16, s);  // cmpw
    ctx.gpr[3] = mat;
}

// void EmitProps_SetMaterial_004FEDD0(AglEmitProperties* p, AglMaterial* m)  [called by EmitProps_SetDesc_004FF4C0] (name?)
// The emitter draws with m: p->+72 = m (u16 +4 references ++), u8 +68 = 0,
// m's u16 +116 = 0; p's texture scale +92 / +96 = the texture's (m->+20)
// u16 width +52 / height +54 over the larger of the two; +100 / +104 =
// m's params 0 / 1 (+124; both 0 without them); m's shader (+16)
// becomes the emitter shader (hash 0x90873B1B, looked up once). r3 is
// left at that shader.
DECOMP(004FEDD0, EmitProps_SetMaterial_004FEDD0) {
    namespace g = g16::g;
    const u32 p = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    store<u32>(p + 72, m);
    store<u16>(m + 4, static_cast<u16>(load<u16>(m + 4) + 1));
    store<u8>(p + 68, 0);
    store<u16>(m + 116, 0);
    const u32 tex = load<u32>(m + 20);
    const u32 params = load<u32>(m + 124);
    const u32 h = load<u16>(tex + 54), w = load<u16>(tex + 52);
    const u32 big = w > h ? w : h;  // cmpw
    // fcfid / frsp are exact for u16s. (0 / 0 gives the host's NaN, as the
    // recompiled fdivs does.)
    auto ratio = [&](u32 a) { return g11::stfs_bits(sgl(static_cast<f64>(a) / static_cast<f64>(big))); };
    store<u32>(p + 92, ratio(w));
    store<u32>(p + 96, ratio(h));
    const u32 zero = load<u32>(g16::kEmitMaterialZero);
    store<u32>(p + 100, params != 0 ? load<u32>(params) : zero);  // lfs / stfs pairs
    const u32 last = params != 0 ? load<u32>(params + 4) : zero;
    store<u32>(p + 104, last);
    ctx.fpr[1] = g10::lfs_value(last);
    if (load<u8>(g::emit_shader_ready) == 0) {
        const u32 s = static_cast<u32>(call(ctx, g15::fn::Shader_FindByHash, {g15::kEmitShaderHash}));
        store<u32>(g::emit_shader, s);
        store<u8>(g::emit_shader_ready, 1);
    }
    const u32 s = load<u32>(g::emit_shader);
    if (load<u32>(m + 16) != s) store<u32>(m + 16, s);  // cmpw
    ctx.gpr[3] = s;
}

// u32 Emitter_Draw_00507B18(Emitter* e, Item* item, a, b, Slot* slot)
// [vtable 0x00B23338 slot 3, the renderer Draw the draw list calls] (name?)
// By the properties' (e->+4) u8 +68 kind (others: 1):
// - kind 0 / 1 without props +136 and with u8 0x010F0A40 set: the
//   batched draw (f_005077B4(e, item, slot) / f_005073F8(e, slot)), 1;
// - otherwise a 144-byte draw record from the frame stack, filled by
//   Emitter_Check_00506050(e, record, item, a, b) (0 when it says no),
//   then f_00506598(e, record, view +752, e->+68's +0 and u16 +4 (0
//   without it)); nothing more without a record +124 count; kind 1:
//   Material_Bind(the first mesh's material), Stencil_SetEqual32 and
//   f_00505740 (record mode u8 +128 == 5) or EmitterDraw_Matrices_00505518
//   with layers4::g::view_proj; kind 0: Sprite_Bind_00501820(e, item)
//   and, when True_0050129C(e, count), the draw by record mode (5:
//   f_005043B0, 4: f_00503BC0, 3: f_00503340, 2: f_00502AC0, 1:
//   f_00502240, else f_005019B8), then STENCIL_TEST_ENABLE 0 when props
//   u8 +69 set (Stencil_SetEqual32's). Returns 1.
DECOMP(00507B18, Emitter_Draw_00507B18) {
    namespace fnx = g16::fn;
    const u32 e = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3),
              slot = arg<u32>(ctx, 4);
    const u32 props = load<u32>(e + 4);
    const u32 kind = load<u8>(props + 68);
    if (kind > 1) return ret(ctx, 1u);
    const bool batched = load<u32>(props + 136) == 0 && load<u8>(g16::g::emitters_batched) != 0;
    if (batched) {
        if (kind == 0) call(ctx, fnx::EmitterDraw_005077B4, {e, item, slot});
        else call(ctx, fnx::EmitterDraw_005073F8, {e, slot});
        return ret(ctx, 1u);
    }
    const u32 rec = static_cast<u32>(call(ctx, fnx::MemStack_Alloc, {load<u32>(g16::g::arena + 280), g16::kEmitRecordSize}));
    if (static_cast<u32>(call(ctx, g14::fn::Emitter_Check_00506050, {e, rec, item, a, b})) == 0) return ret(ctx, 0u);
    {
        const u32 r = load<u32>(e + 68);
        const u32 x = r != 0 ? load<u32>(r) : 0u;
        const u32 y = r != 0 ? load<u16>(r + 4) : 0u;
        const u32 view = load<u32>(load<u32>(g15::g::current_surface) + 672);
        call(ctx, fnx::EmitterDraw_00506598, {e, rec, view + g15::kViewMatrix, x, y});
    }
    if (load<u32>(rec + 124) == 0) return ret(ctx, 1u);
    if (kind == 1) {
        const u32 mat = load<u32>(load<u32>(load<u32>(e + 4) + 72) + 16);
        call(ctx, g15::fn::Material_Bind, {mat, load<u32>(load<u32>(load<u32>(mat + 16) + 12))});
        call(ctx, fnx::Stencil_SetEqual32_005011A0, {e});
        if (load<u8>(rec + 128) == 5) call(ctx, fnx::EmitterDraw_00505740, {e, rec, l4::g::view_proj});
        else call(ctx, fnx::EmitterDraw_Matrices_00505518, {e, rec, l4::g::view_proj});
        return ret(ctx, 1u);
    }
    call(ctx, fnx::Sprite_Bind_00501820, {e, item});
    const s32 count = static_cast<s32>(load<u32>(rec + 124));  // extsw
    if (static_cast<u32>(call(ctx, fnx::True_0050129C, {e, static_cast<u64>(static_cast<s64>(count))})) != 0) {
        u32 fn;
        switch (load<u8>(rec + 128)) {
        case 5: fn = fnx::EmitterDraw_005043B0; break;
        case 4: fn = fnx::EmitterDraw_00503BC0; break;
        case 3: fn = fnx::EmitterDraw_00503340; break;
        case 2: fn = fnx::EmitterDraw_00502AC0; break;
        case 1: fn = fnx::EmitterDraw_00502240; break;
        default: fn = fnx::EmitterDraw_005019B8; break;
        }
        call(ctx, fn, {e, rec});
    }
    if (load<u8>(load<u32>(e + 4) + 69) != 0) fx::emit({g16::kStencilTestEnable, 0});
    ret(ctx, 1u);
}

namespace {
// One UV matrix of UvAnim_Matrices_005D70E0: Matrix_FromAxisAngle(out,
// the z axis, angle); (u, v) through its rows (lvx, in vmaddfp order:
// row0 * u + 0, + row1 * v, + row2 * 0, then row3 +); the rows scaled
// (Matrix_ScaleRows(out, scale)); then row 2 xyz = the point's x and y
// less their integer parts (fctidz / fcfid) and its z.
void uv_matrix16(PPUContext& ctx, u32 out, f64 u, f64 v, f64 angle, u32 scale) {
    callf(ctx, g16::fn::Matrix_FromAxisAngle, {out, g16::g::axis_z}, {angle});
    const u32 m = out & ~15u;
    const f32 fu = std::bit_cast<f32>(g11::stfs_bits(u)), fv = std::bit_cast<f32>(g11::stfs_bits(v));
    f32 p[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 x = std::fmaf(load<f32>(m + 4 * k), fu, 0.0f);
        x = std::fmaf(load<f32>(m + 16 + 4 * k), fv, x);
        x = std::fmaf(load<f32>(m + 32 + 4 * k), 0.0f, x);
        p[k] = load<f32>(m + 48 + 4 * k) + x;  // vaddfp
    }
    const f64 x = g10::lfs_value(std::bit_cast<u32>(p[0])), y = g10::lfs_value(std::bit_cast<u32>(p[1]));
    const f64 ix = sgl(g11::trunc_d(x)), iy = sgl(g11::trunc_d(y));
    call(ctx, g16::fn::Matrix_ScaleRows, {out, scale});
    store<u32>(out + 32, g11::stfs_bits(sgl(x - ix)));
    store<u32>(out + 36, g11::stfs_bits(sgl(y - iy)));
    store<u32>(out + 40, std::bit_cast<u32>(p[2]));  // lfs / stfs pair
}
}  // namespace

// void UvAnim_Matrices_005D70E0(Material* m, f32 a[4][4], f32 b[4][4], f32 out[4])  [called by f_005D7438] (name?)
// The material's two scrolling / rotating UV matrices at the time
// frame_dt +8 (t), from its params (+124; all 0 without them): a from
// speeds p[0] / p[1] and angle p[8], b from p[3] / p[4] and angle p[9],
// both scaled by {p[6], p[7], 0} (uv_matrix16); out = {p[2], p[5], 0, 0}.
// f1 / r3..r5 are left as the original leaves them.
DECOMP(005D70E0, UvAnim_Matrices_005D70E0) {
    const u32 m = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3);
    const f64 zero = lfs(g16::kUvAnimZero);
    const f64 t = lfs(g16::g::frame_dt + 8);
    auto param = [&](u32 k) { const u32 p = load<u32>(m + 124); return p != 0 ? lfs(p + 4 * k) : zero; };
    Stack frame(ctx, 16);
    const u32 scale = frame.addr();
    {
        const u32 p = load<u32>(m + 124);
        const f64 s0 = p != 0 ? lfs(p + 24) : zero, s1 = p != 0 ? lfs(p + 28) : zero;
        store<u32>(scale + 4, g11::stfs_bits(s1));
        store<u32>(scale, g11::stfs_bits(s0));
        store<u32>(scale + 8, g11::stfs_bits(zero));
        const f64 u = sgl(t * (p != 0 ? lfs(p) : zero)), v = sgl(t * (p != 0 ? lfs(p + 4) : zero));
        uv_matrix16(ctx, a, u, v, p != 0 ? lfs(p + 32) : zero, scale);
    }
    {
        const f64 u = sgl(t * param(3)), v = sgl(t * param(4));
        uv_matrix16(ctx, b, u, v, param(9), scale);
    }
    store<u32>(out, g11::stfs_bits(param(2)));
    store<u32>(out + 4, g11::stfs_bits(param(5)));
    store<u32>(out + 8, g11::stfs_bits(zero));
    store<u32>(out + 12, g11::stfs_bits(zero));
    ctx.fpr[1] = g10::lfs_value(load<u32>(b + 40));
    ctx.gpr[3] = out + 4;
    ctx.gpr[4] = out + 8;
    ctx.gpr[5] = out + 12;
}

// void EmitterDraw_Batched_005077B4(Emitter* e, Item* item, const Slot* slot)  [Emitter_Draw_00507B18, kind 0] (name?)
// A batched sprite emitter's draw: nothing without slot +4 (the index
// count) or with slot +0 (the first index) -1. Sprite_Bind_00501820(e,
// item); the stream buffers (0x010DDBBC) become the vertex format with
// the current buffer's vertex data (+128 [+228], 16 bytes each: +4 the
// location / offset word; pool::bind_format), render state +4 = its index
// buffer (+160 [+228], 20 bytes each), the fragment program uploaded
// (pool::upload_fragment) and the slot's triangles drawn from it (offset
// + first * 2 or 4 bytes by the index type); then STENCIL_TEST_ENABLE 0
// when the props' u8 +69 is set.
DECOMP(005077B4, EmitterDraw_Batched_005077B4) {
    namespace sp = sly4::render::pool;
    const u32 e = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1), slot = arg<u32>(ctx, 2);
    // r3 is left as the original leaves it on each path.
    if (load<u32>(slot + 4) == 0) return ret(ctx, 0u);
    if (load<u32>(slot) == 0xFFFFFFFFu) return ret(ctx, 0xFFFFFFFFu);  // cmpwi -1
    call(ctx, g16::fn::Sprite_Bind_00501820, {e, item});
    const u32 st = fx::g::state;
    const u32 sb = load<u32>(g16::g::stream_buffers);
    sp::bind_format(sb, load<u32>(sb + ((load<u32>(sb + g16::kSbIndex) << 4) & ~15u) + g16::kSbVertexData + 4));
    store<u32>(st + 4, sb + load<u32>(sb + g16::kSbIndex) * 20 + g16::kSbIndexBuffers);
    sp::upload_fragment(ctx);
    const u32 ib = load<u32>(st + 4);
    const u32 first = load<u32>(slot);
    const u32 type = load<u32>(ib + 16);
    const u32 bytes = type == 1 ? 2u : 4u;  // CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16 : _32
    const u32 where = load<u32>(ib + 4);
    call(ctx, g16::fn::cellGcmSetDrawIndexArray,
         {load<u32>(st), g16::kPrimTriangles, load<u32>(slot + 4), type & 0xFF, where & 1, first * bytes + (where >> 1)});
    const u32 on = load<u8>(load<u32>(e + 4) + 69);
    if (on == 0) return ret(ctx, on);
    fx::emit({g16::kStencilTestEnable, 0});
    ctx.gpr[3] = g16::kStencilTestEnable;
}

// void EmitterDraw_BatchedMeshes_005073F8(Emitter* e, const MeshSlot* slot)  [Emitter_Draw_00507B18, kind 1] (name?)
// A batched mesh emitter's draw from prebuilt command lists: nothing
// when the slot's four counts (+0..+12) are all 0. Material_Bind(the
// first mesh's material, its shader), Stencil_SetEqual32_005011A0, the
// fragment program uploaded (pool::upload_fragment); then for each of
// the props' u8 +88 meshes (props +72 [i]) with a count (slot +0 [i])
// and a command list (slot +16 [i]): its vertex format (+68) with its
// vertex data (+28) bound (pool::bind_format), render state +4 = its
// index buffer (+44) when it has indices (+64), and a call slot reserved
// in the command buffer (CmdBuf_ReserveCall_00537DD4): the list's last
// word becomes a jump back to the slot's end (offset | 0x20000000) and
// the slot a jump to the list (cellGcmAddressToOffset).
DECOMP(005073F8, EmitterDraw_BatchedMeshes_005073F8) {
    namespace sp = sly4::render::pool;
    const u32 e = arg<u32>(ctx, 0), slot = arg<u32>(ctx, 1);
    if (load<u32>(slot) == 0 && load<u32>(slot + 4) == 0 && load<u32>(slot + 8) == 0 && load<u32>(slot + 12) == 0) return ret(ctx, 0u);
    {
        const u32 mat = load<u32>(load<u32>(load<u32>(e + 4) + 72) + 16);
        call(ctx, g15::fn::Material_Bind, {mat, load<u32>(load<u32>(load<u32>(mat + 16) + 12))});
    }
    call(ctx, g16::fn::Stencil_SetEqual32_005011A0, {e});
    sp::upload_fragment(ctx);
    const u32 st = fx::g::state;
    Stack frame(ctx, 16);
    const u32 offset = frame.addr(), call_slot = offset + 4;  // {at, after, after's offset}
    for (u32 i = 0; i < load<u8>(load<u32>(e + 4) + 88); ++i) {  // cmplw
        if (load<u32>(slot + 4 * i) == 0) continue;
        const u32 list_at = slot + 16 + 4 * i;
        if (load<u32>(list_at) == 0) continue;
        const u32 mesh = load<u32>(load<u32>(e + 4) + 72 + 4 * i);
        sp::bind_format(load<u32>(mesh + 68), load<u32>(mesh + 28));
        if (load<u32>(mesh + 64) != 0) store<u32>(st + 4, mesh + 44);
        call(ctx, g16::fn::CmdBuf_ReserveCall_00537DD4, {call_slot});
        const u32 last = load<u32>(list_at) + ((load<u32>(slot + 4 * i) << 2) & ~3u) - 4;
        store<u32>(last, load<u32>(call_slot + 8) | g16::kGcmJump);
        call(ctx, fx::imp::cellGcmAddressToOffset, {load<u32>(list_at), offset});
        store<u32>(load<u32>(call_slot), load<u32>(offset) | g16::kGcmJump);
    }
    ctx.gpr[3] = load<u32>(e + 4);  // as the original leaves it (the props)
}

namespace {
// Tex_FindLoaded(name); when loaded, its slot 4 with (t, 1, 1, 1) and
// Material_AddTexture_004FF008(m, t). Returns the texture.
u32 add_texture16(PPUContext& ctx, u32 m, u32 name) {
    const u32 t = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {name}));
    if (t != 0) {
        fx::vcall(ctx, t, 16, {t, 1, 1, 1});
        call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t});
    }
    return t;
}
}  // namespace

// bool SlyDecal4_LoadParams(ShaderType*, Material* m, const Desc* d)
// [vtable 0x00B25BB8 slot 3, the "SlyDecal4" type's LoadParams] (name?)
// Nothing (returns 1) for desc version (u8 d+84) 0. The textures named
// d+4 (optional), d+8, d+12 and d+16 (each slot 4 with (t, 1, 1, 1), then
// added; a missing one of the last three returns 0 there) and, per layer
// after its texture, the params a desc older than the field leaves at
// their defaults: params 0 / 1 = d+88 / +92 (1.0), param 6 = d+112 (0)
// in radians; after d+12's texture params 2 / 3 = d+96 / +100, param 7 =
// d+116 in radians; after d+16's params 4 / 5 = d+104 / +108, param 8 =
// d+120 in radians. Returns 1.
DECOMP(005C5300, SlyDecal4_LoadParams) {
    constexpr u32 K = g16::kSlyDecal4Consts;
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    auto ver = [&]() { return static_cast<s32>(ref<u8>(d + 84)); };  // cmpwi
    // Field `off` when the desc has field n (version > n), else the default word.
    auto field_or = [&](s32 n, u32 off, u32 def) { return ver() > n ? load<u32>(d + off) : load<u32>(def); };  // lfs / stfs
    auto angle = [&](s32 n, u32 off) {
        const f64 a = ver() > n ? lfs(d + off) : lfs(K + 4);
        return g11::stfs_bits(sgl(a * lfs(K + 8)));  // fmuls
    };
    if (ver() <= 0) return ret(ctx, 1u);
    add_texture16(ctx, m, load<u32>(d + 4));
    if (add_texture16(ctx, m, load<u32>(d + 8)) == 0) return ret(ctx, 0u);
    set_param16(ctx, m, 0, field_or(0, 88, K));
    set_param16(ctx, m, 1, field_or(1, 92, K));
    set_param16(ctx, m, 6, angle(6, 112));
    if (add_texture16(ctx, m, load<u32>(d + 12)) == 0) return ret(ctx, 0u);
    set_param16(ctx, m, 2, field_or(2, 96, K));
    set_param16(ctx, m, 3, field_or(3, 100, K));
    set_param16(ctx, m, 7, angle(7, 116));
    if (add_texture16(ctx, m, load<u32>(d + 16)) == 0) return ret(ctx, 0u);
    set_param16(ctx, m, 4, field_or(4, 104, K));
    set_param16(ctx, m, 5, field_or(5, 108, K));
    set_param16(ctx, m, 8, angle(8, 120));
    ret(ctx, 1u);
}

namespace {
// Appends a colour (4 bytes from src) to the emitter curve c (u8 +6
// count, u8[4] colours at +20); the first one is also copied to slot 1.
void curve_push16(u32 c, u32 src) {
    const u32 n = load<u8>(c + 6);
    store<u8>(c + 6, static_cast<u8>(n + 1));
    const u32 dst = c + 20 + ((n << 2) & ~3u);
    for (u32 k = 0; k < 4; ++k) store<u8>(dst + k, load<u8>(src + k));
    const u32 m = load<u8>(c + 6);
    if (m == 1)
        for (u32 k = 0; k < 4; ++k) store<u8>(c + 20 + ((m << 2) & ~3u) + k, load<u8>(src + k));
}
}  // namespace

// bool Iff_HandleEmitCurve_005291F8(IffReader* r, AglEmitProperties* p)  [IFF chunk handler, OPD 0x00F3B680] (name?)
// The emitter's colour curve: f32 "alphaFadeIn" / "alphaFadeOut", then a
// new 84-byte curve (vtable 0x00B23200, refs 1, +12 / +16 1e8, see
// obj/obj6_emitter.cpp) becomes p->+112 (references moved: the old one
// released through vtable +4 when its count drops to 0, the creation one
// dropped); +12 / +16 = p->+4 (the lifetime) / fade when the fade is above
// 1e-5, else 1e8; u16 +8 = 0; then the chunk's u32 "RGBA" colours are
// appended (curve_push16), or two white ones (0x01164CA0) when it has
// none. Returns 1.
// Quirk kept: a failed SmallAlloc leaves +112 0 and the code writes
// through it.
DECOMP(005291F8, Iff_HandleEmitCurve_005291F8) {
    constexpr u32 K = g16::kEmitCurveConsts;
    const u32 r = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 fade_in = frame.addr(), fade_out = fade_in + 4, rgba = fade_in + 8;
    call(ctx, g16::fn::FileReader_ReadF32Swapped, {r, fade_in, g16::str_alpha_fade_in});
    call(ctx, g16::fn::FileReader_ReadF32Swapped, {r, fade_out, g16::str_alpha_fade_out});
    u32 c = static_cast<u32>(call(ctx, g16::fn::SmallAlloc, {g16::kEmitCurveSize}));
    if (c != 0) {
        store<u16>(c + 4, 1);
        store<u8>(c + 6, 0);
        store<u32>(c, g16::vt_emit_curve);
        store<u16>(c + 8, 0);
        store<u32>(c + 12, load<u32>(K + 4));
        store<u32>(c + 16, load<u32>(K + 4));
    }
    if (const u32 old = load<u32>(p + 112); old != c) {
        store<u32>(p + 112, c);
        if (c != 0) store<u16>(c + 4, static_cast<u16>(load<u16>(c + 4) + 1));
        if (old != 0) {
            const u16 refs = static_cast<u16>(load<u16>(old + 4) - 1);
            store<u16>(old + 4, refs);
            if (static_cast<s16>(refs) <= 0) fx::vcall(ctx, old, 4, {old});
        }
    }
    {
        const u32 cur = load<u32>(p + 112);
        store<u16>(cur + 4, static_cast<u16>(load<u16>(cur + 4) - 1));
    }
    const f64 eps = lfs(K), big = lfs(K + 4);
    {
        const f64 in = lfs(fade_in);
        const u32 cur = load<u32>(p + 112);
        const f64 life = lfs(p + 4);
        store<u32>(cur + 12, g11::stfs_bits(in > eps ? sgl(life / in) : big));  // fcmpu
    }
    {
        const f64 out = lfs(fade_out);
        const u32 cur = load<u32>(p + 112);
        const f64 life = lfs(p + 4);
        store<u32>(cur + 16, g11::stfs_bits(out > eps ? sgl(life / out) : big));
    }
    store<u16>(load<u32>(p + 112) + 8, 0);
    const u32 n = static_cast<u32>(call(ctx, g16::fn::ChunkReader_Remaining, {r})) >> 2;  // rlwinm 30,2,31
    if (n == 0) {
        curve_push16(load<u32>(p + 112), g15::g::white);
        curve_push16(load<u32>(p + 112), g15::g::white);
        return ret(ctx, 1u);
    }
    for (u32 k = 0; k < n; ++k) {  // cmplw
        call(ctx, g16::fn::FileReader_ReadU32, {r, rgba, g16::str_rgba});
        curve_push16(load<u32>(p + 112), rgba);
    }
    ret(ctx, 1u);
}

namespace {
inline u32 rotl16(u32 v, int n) { return (v << n) | (v >> (32 - n)); }
// The random generator's next mix (game/update.h Random_Step, chained:
// the state is never stored between steps).
inline u32 next_mix16(u32 mix) { return (rotl16(mix, 20) + mix) ^ g13::kRandomMix; }
// A 16-bit random value as the single it converts to (fcfid / frsp).
inline f64 rand_f16(u64 v) { return sgl(static_cast<f64>(static_cast<s64>(v))); }

// The parabolic sine of the spawn shapes' spin (constants at K +32:
// {2 pi, pi / 2, pi, 4 / pi, -4 / pi^2, 0.225}; K +16 / +24 f64 +-pi):
// the angle wrapped to [-pi, pi) and its cosine / sine, every step single.
void sincos16(u32 K, f64 a, f64& c, f64& s) {
    const f64 off = sgl(a >= 0.0 ? g11::lfd(K + 16) : g11::lfd(K + 24));  // fsel, frsp
    const f64 two_pi = lfs(K + 32);
    const f64 t = sgl(off + a);
    const f64 q = sgl(g11::trunc_d(sgl(t / two_pi)));  // fdivs, fctidz, fcfid, frsp
    const f64 w = sgl(g11::nmsubs(q, two_pi, t) - off);
    f64 u = sgl(w + lfs(K + 36));
    if (u > lfs(K + 40)) u = sgl(u - two_pi);  // fcmpu / bgt
    const f64 k1 = lfs(K + 44), k2 = lfs(K + 48), k3 = lfs(K + 52);
    c = g11::madds(u, k1, sgl(sgl(u * k2) * std::fabs(u)));
    s = g11::madds(w, k1, sgl(sgl(w * k2) * std::fabs(w)));
    c = g11::madds(g11::msubs(c, std::fabs(c), c), k3, c);
    s = g11::madds(g11::msubs(s, std::fabs(s), s), k3, s);
}

// The spawn shapes' optional spin (u8 o+16): the velocity turned about x
// by o+20 + random * o+24 and about z by o+28 + random * o+32 (each
// skipped when exactly `zero`), the random numbers chained on from mix m4
// and the generator state stored after the first turn. K is the shape's
// constant table (+0 the random scale), `sc` its sincos16 block. Leaves
// r3 / f1 as the original leaves them (without the second turn: r3 the
// last random value with `r3_low`, else the stored state; f1 the second
// angle with `f1_angle`, else the zero constant).
void spawn_spin16(PPUContext& ctx, u32 K, u32 sc, u32 zero_at, u32 o, u32 vel, u32 m4, bool r3_low, bool f1_angle = false) {
    const f64 scale = lfs(K);
    const f64 zero = lfs(zero_at);
    const u32 m5 = next_mix16(m4);
    {
        const f64 a = g11::madds(lfs(o + 24), sgl(rand_f16(m4 & 0xFFFF) * scale), lfs(o + 20));
        if (a != zero) {  // fcmpu
            f64 c, s;
            sincos16(sc, a, c, s);
            const f64 vz = lfs(vel + 8), vy = lfs(vel + 4);
            const f64 cz = sgl(c * vz), sz = sgl(s * vz);
            const f64 nz = g11::madds(s, vy, cz), ny = g11::msubs(c, vy, sz);
            store<u32>(vel + 4, g11::stfs_bits(ny));
            store<u32>(vel + 8, g11::stfs_bits(nz));
        }
    }
    const u32 m6 = next_mix16(m5);
    store<u32>(g13::g::random_state, rotl16(m6, 16));
    store<u64>(g13::g::random_last, m6 & 0xFFFF);
    const f64 b = g11::madds(lfs(o + 32), sgl(rand_f16(m5 & 0xFFFF) * scale), lfs(o + 28));
    ctx.fpr[1] = f1_angle ? b : zero;
    if (b == zero) return ret(ctx, r3_low ? m6 & 0xFFFF : rotl16(m6, 16));  // fcmpu
    f64 c, s;
    sincos16(sc, b, c, s);
    const f64 vy = lfs(vel + 4), vx = lfs(vel);
    const f64 cy = sgl(c * vy), sy = sgl(s * vy);
    const f64 ny = g11::madds(s, vx, cy), nx = g11::msubs(c, vx, sy);
    store<u32>(vel, g11::stfs_bits(nx));
    store<u32>(vel + 4, g11::stfs_bits(ny));
    ctx.fpr[1] = nx;
    ret(ctx, vel + 4);
}
}  // namespace

// void BoxShape_Spawn_0079FFB0(const BoxShape* o, f32 pos[3], f32 vel[3])
// [vtable 0x00B231D4 slot 3, an emitter shape's spawn] (name?)
// pos = a random point of the box (each axis uniform in [-1, 1) times
// o +48 / +52 / +56); vel = the up direction (0x01164B60) times o+8 less
// a random fraction of o+12. With u8 o+16 set the velocity is then spun
// about x by o+20 + random * o+24 and about z by o+28 + random * o+32
// (each skipped when exactly 0). Random numbers: the generator's last
// value, then its chained mixes (the state stored after the velocity and
// again after the spins). r3 / f1 are left as the original leaves them.
DECOMP(0079FFB0, BoxShape_Spawn_0079FFB0) {
    constexpr u32 K = g16::kBoxSpawnConsts;
    const u32 o = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1), vel = arg<u32>(ctx, 2);
    const f64 scale = lfs(K), two = lfs(K + 4), minus_one = lfs(K + 8);
    const u64 r0 = load<u64>(g13::g::random_last);
    const u32 m1 = (rotl16(load<u32>(g13::g::random_state), 4) + rotl16(load<u32>(g13::g::random_state), 16)) ^ g13::kRandomMix;
    const u32 m2 = next_mix16(m1), m3 = next_mix16(m2), m4 = next_mix16(m3);
    auto uniform = [&](u64 r) { return g11::madds(sgl(rand_f16(r) * scale), two, minus_one); };
    const f64 x = uniform(r0), y = uniform(m1 & 0xFFFF), z = uniform(m2 & 0xFFFF);
    store<u32>(pos, g11::stfs_bits(sgl(x * lfs(o + 48))));
    store<u32>(pos + 4, g11::stfs_bits(sgl(y * lfs(o + 52))));
    store<u32>(pos + 8, g11::stfs_bits(sgl(z * lfs(o + 56))));
    const f64 r3 = sgl(rand_f16(m3 & 0xFFFF) * scale);
    const f64 speed = g11::nmsubs(lfs(o + 12), r3, lfs(o + 8));
    store<u32>(g13::g::random_state, rotl16(m4, 16));
    store<u64>(g13::g::random_last, m4 & 0xFFFF);
    f64 f1 = 0;
    for (u32 k = 0; k < 3; ++k) {
        f1 = sgl(lfs(g16::g::up + 4 * k) * speed);
        store<u32>(vel + 4 * k, g11::stfs_bits(f1));
    }
    ctx.fpr[1] = f1;
    if (load<u8>(o + 16) == 0) return ret(ctx, o);
    spawn_spin16(ctx, K, K, K + 12, o, vel, m4, false);
}

namespace {
// The inline object base constructor (gfx12.cpp obj_init12, with this
// function's store order): u16 +4 = 1, then the word +4 = 1 << 16 | old
// low bits & 0x3FF; the serial += 1; +12 = 0; vtable vt_obj_base (after
// vt_obj_base0 for the first object); +8 = serial << 11 | old & 0x3F; u16
// +96..+110 = 0; two 64-byte attribute records at +128 ({u8 0, u8 1,
// obj_init_bytes, f32 50.0 at +12}); the u32 / u8 quads at +520.. /
// +536.. of the 2 x 2 blocks (68 / 32 apart) zeroed.
void obj_init16(u32 o, bool first) {
    const u32 w4 = load<u32>(o + 4);
    store<u16>(o + 4, 1);
    store<u32>(o + 4, (1u << 16) | (w4 & 0x3FFu));
    if (first) store<u32>(o, g12::vt_obj_base0);
    const u32 serial = load<u32>(g12::g::obj_serial) + 1;
    store<u32>(g12::g::obj_serial, serial);
    const u32 w8 = (serial << 11) | (load<u32>(o + 8) & 0x3Fu);
    if (!first) store<u32>(o, g12::vt_obj_base);
    store<u32>(o + 12, 0);
    store<u16>(o + 96, 0);
    store<u16>(o + 98, 0);
    if (first) store<u32>(o, g12::vt_obj_base);
    store<u16>(o + 100, 0);
    if (!first) store<u32>(o + 8, w8);
    store<u16>(o + 102, 0);
    if (first) store<u32>(o + 8, w8);
    for (u32 k = 104; k <= 110; k += 2) store<u16>(o + k, 0);
    for (u32 r = o + 128; r < o + 256; r += 64) {
        store<u8>(r, 0);
        for (u32 k = 0; k < 4; ++k) store<u8>(r + 2 + k, load<u8>(g12::g::obj_init_bytes + k));
        store<u32>(r + 12, 0x42480000u);  // 50.0f
        store<u8>(r + 1, 1);
    }
    for (u32 a = 0; a < 2; ++a)
        for (u32 k = 0; k < 2; ++k) {
            const u32 p = o + 68 * a + 32 * k;
            for (u32 b : {520u, 536u}) {
                store<u32>(p + b, 0);
                store<u32>(p + b + 4, 0);
                store<u32>(p + b + 8, 0);
                store<u8>(p + b + 12, 0);
            }
        }
}
}  // namespace

// u32 RenderTarget_InitPrimary_00520D88(RenderTarget* rt, u8 primary)  [called by RenderTarget_Create] (name?)
// As RenderTarget_Init_00521110 (gfx12.cpp), for the display: obj_init16,
// u8 +48 = primary, +516 = 0, +512 = the display (0x00F66FBC),
// RenderTarget_Setup_005203A8, name +16 "RenderTarget_Primary" (32
// bytes). The first target made without `primary` becomes the current
// one (0x00F66FC4); +52 bit 2 = rt is the current one. Then its view:
// +672 = Mem_Alloc(1840, 0, 16), memset to 0 (even when the allocation
// failed: address 0), constructed like an AglRenderTarget (obj_init16,
// vtable vt_render_target, u64 +1808 = 0, u8 +1816 = 0, +1824 = 4.0,
// +1828 = 0.01, u8 +1776 bits 0x20 / 0x10 cleared). Returns the view or 0.
DECOMP(00520D88, RenderTarget_InitPrimary_00520D88) {
    const u32 rt = arg<u32>(ctx, 0);
    const u32 primary = arg<u32>(ctx, 1);
    obj_init16(rt, true);
    store<u8>(rt + 48, static_cast<u8>(primary));
    store<u32>(rt + 516, 0);
    store<u32>(rt + 512, load<u32>(g15::g::display));
    call(ctx, g12::fn::RenderTarget_Setup_005203A8, {rt});
    call(ctx, g12::fn::String_Copy, {rt + 16, g16::str_render_target_primary, 32});
    u32 cur = load<u32>(g12::g::current_target);
    if ((cur | primary) == 0) {  // or / cmpwi
        store<u32>(g12::g::current_target, rt);
        cur = rt;
    }
    const u32 b52 = load<u8>(rt + 52);
    store<u8>(rt + 52, static_cast<u8>((b52 & ~4u) | (cur - rt == 0 ? 4u : 0u)));  // cntlzw / rlwimi
    const u32 mem = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {1840, 0, 16}));
    const u32 v = static_cast<u32>(call(ctx, g12::fn::memset, {mem, 0, 1840}));
    if (v != 0) {
        obj_init16(v, false);
        const u32 b1776 = load<u8>(v + 1776);
        store<u32>(v, g12::vt_render_target);
        store<u64>(v + 1808, 0);
        store<u8>(v + 1816, 0);
        store<u32>(v + 1824, 0x40800000u);  // 4.0f
        store<u32>(v + 1828, 0x3C23D70Au);  // 0.01f
        store<u8>(v + 1776, static_cast<u8>(b1776 & ~0x30u));
    }
    store<u32>(rt + 672, v);
    ret(ctx, v);
}

// void Render_EndFrame_005105E0()  [called by f_0049AE40] (name?)
// The frame end: Stats_ResetCounters_00510554, the default frame buffer's
// +68 = 0, RenderBuffer_FlipStats_0050CC10; the draw-record list (built
// once, as Render_BeginFrame_0051008C does) has its block's +172 = the
// smaller (signed) of itself and the atomic end (+256) less the start
// (+184), then the end atomically 0 and +180 / +184 0; the render state's
// texture cache (+52..+80) cleared with Gcm_Fn5E3F18(context, unit, 0, 0,
// 0, 0) for units 0..7 and the 16 vertex attributes switched off
// (cellGcmSetVertexDataArrayUnsafe(context, i, 0, 0, 0, 2 (F32), 0, 0)),
// +8 = -1, u16 +32 = 0, +4 = 0; a pending copy (0x010C5A08: +0 the
// surface, u8 +4 set) goes from the selected display buffer
// (Display_SelectBuffer(mem_stats)) to it (Surface_Copy_00537118) and is
// cleared; Gcm_FlipContext_0053BE94; with u8 0x00F66FD4 and the object
// list 0x00F66FD0: its u8 +96 bit 7 cleared and each of its u8 +20
// objects (+4 [i]) gets slots 4 (+16) and 3 (+12); config u8 +80 = 0.
DECOMP(005105E0, Render_EndFrame_005105E0) {
    namespace g = g16::g;
    call(ctx, g16::fn::Stats_ResetCounters_00510554);
    store<u32>(load<u32>(g::default_frame_buffer) + 68, 0);
    call(ctx, g16::fn::RenderBuffer_FlipStats_0050CC10);
    const u32 list = g::record_list;
    if (load<u8>(g::record_list_ready) == 0) {
        store<u8>(list, 1);
        call(ctx, g16::fn::PtrVector_Construct, {list + 4});
        store<u32>(list + 32, 0);
        store<u16>(list + 10, g::kRecordListCapacity);
        atomic_set(list + 128, 0);
        atomic_set(list + 256, 0);
        store<u8>(g::record_list_ready, 1);
    }
    {
        const u32 cur = load<u32>(list + 28);
        const u32 start = load<u32>(cur + 184);
        const u32 low = load<u32>(cur + 172);
        const u32 used = std::atomic_ref<u32>(ref<u32>(cur + 256)).load();  // lwarx
        const u32 v = __builtin_bswap32(used) - start;
        store<u32>(cur + 172, static_cast<s32>(low) < static_cast<s32>(v) ? low : v);  // cmpw
        atomic_set(cur + 256, 0);
        store<u32>(cur + 184, 0);
        store<u32>(cur + 180, 0);
    }
    const u32 st = fx::g::state;
    for (u32 unit = 0; unit < 8; ++unit) {
        const u32 c = load<u32>(st);
        store<u32>(st + 52 + 4 * unit, 0);
        call(ctx, g16::fn::Gcm_Fn5E3F18, {c, unit, 0, 0, 0, 0});
    }
    for (u32 i = 0; i < 16; ++i)
        call(ctx, g15::fn::cellGcmSetVertexDataArrayUnsafe, {load<u32>(st), i & 0xFF, 0, 0, 0, g16::kVertexF32, 0, 0});
    store<u32>(st + 8, 0xFFFFFFFFu);
    store<u16>(st + 32, 0);
    store<u32>(st + 4, 0);
    if (load<u8>(g::pending_copy + 4) != 0 && load<u32>(g::pending_copy) != 0) {
        const u32 from = static_cast<u32>(call(ctx, g16::fn::Display_SelectBuffer, {sly4::g::mem_stats}));
        call(ctx, g16::fn::Surface_Copy_00537118, {from, load<u32>(g::pending_copy)});
        store<u8>(g::pending_copy + 4, 0);
        store<u32>(g::pending_copy, 0);
    }
    call(ctx, g16::fn::Gcm_FlipContext_0053BE94);
    if (load<u8>(g::object_list_on) != 0) {
        if (const u32 o = load<u32>(g13::g::object_list_f66fd0); o != 0) {
            const u32 b96 = load<u8>(o + 96);
            const s32 n = load<u8>(o + 20);
            store<u8>(o + 96, static_cast<u8>(b96 & 0x7F));
            if (n > 0) {
                for (u32 k = 0; static_cast<s32>(k) < static_cast<s32>(load<u8>(o + 20)); k = (k + 1) & 0xFF) {  // cmpw
                    const u32 at = o + 4 + 4 * k;
                    const u32 p = load<u32>(at);
                    fx::vcall(ctx, p, 16, {p});
                    const u32 p2 = load<u32>(at);
                    fx::vcall(ctx, p2, 12, {p2});
                }
            }
        }
    }
    store<u8>(sly4::g::config + 80, 0);
    ctx.gpr[3] = sly4::g::config;  // as the original leaves it
}

// void SphereShape_Spawn_0079EEF8(const SphereShape* o, f32 pos[3], f32 vel[3])
// [vtable 0x00B23174 slot 3, an emitter shape's spawn] (name?)
// A random direction d (three uniform [-1, 1) values, normalised by 1 /
// fsqrts of their squares (y y, then x x +, z z +)); pos = d * o+36 (the
// radius), vel = d * (o+8 less a random fraction of o+12); the generator
// state is stored before the position. Then the spin as the box's
// (spawn_spin16) with u8 o+16.
DECOMP(0079EEF8, SphereShape_Spawn_0079EEF8) {
    constexpr u32 K = g16::kSphereSpawnConsts;
    const u32 o = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1), vel = arg<u32>(ctx, 2);
    const f64 scale = lfs(K), two = lfs(K + 4), minus_one = lfs(K + 8);
    const u64 r0 = load<u64>(g13::g::random_last);
    const u32 st = load<u32>(g13::g::random_state);
    const u32 m1 = (rotl16(st, 4) + rotl16(st, 16)) ^ g13::kRandomMix;
    const u32 m2 = next_mix16(m1), m3 = next_mix16(m2), m4 = next_mix16(m3);
    auto uniform = [&](u64 r) { return g11::madds(sgl(rand_f16(r) * scale), two, minus_one); };
    const f64 x = uniform(r0), y = uniform(m1 & 0xFFFF), z = uniform(m2 & 0xFFFF);
    const f64 len2 = g11::madds(z, z, g11::madds(x, x, sgl(y * y)));
    const f64 radius = lfs(o + 36);
    store<u32>(g13::g::random_state, rotl16(m4, 16));
    store<u64>(g13::g::random_last, m4 & 0xFFFF);
    const f64 len = len2 < 0.0 ? std::numeric_limits<f64>::quiet_NaN() : sgl(std::sqrt(len2));  // fsqrts
    const f64 r3 = sgl(rand_f16(m3 & 0xFFFF) * scale);
    const f64 inv = sgl(lfs(K + 12) / len);
    const f64 dx = sgl(x * inv), dy = sgl(y * inv), dz = sgl(z * inv);
    store<u32>(pos, g11::stfs_bits(sgl(dx * radius)));
    store<u32>(pos + 4, g11::stfs_bits(sgl(dy * radius)));
    store<u32>(pos + 8, g11::stfs_bits(sgl(dz * radius)));
    const f64 speed = g11::nmsubs(lfs(o + 12), r3, lfs(o + 8));
    store<u32>(vel, g11::stfs_bits(sgl(dx * speed)));
    store<u32>(vel + 4, g11::stfs_bits(sgl(dy * speed)));
    const f64 vz = sgl(dz * speed);
    store<u32>(vel + 8, g11::stfs_bits(vz));
    ctx.fpr[1] = vz;
    if (load<u8>(o + 16) == 0) return ret(ctx, o);
    spawn_spin16(ctx, K, K + 8, K + 16, o, vel, m4, true);
}

// void SphereVolumeShape_Spawn_0079F3E8(const SphereShape* o, f32 pos[3], f32 vel[3])
// [vtable 0x00B23194 slot 3, an emitter shape's spawn] (name?)
// As SphereShape_Spawn_0079EEF8, but inside the sphere: pos = d * o+36 *
// a random fraction, and the velocity along the up direction (0x01164B60)
// as the box's; the state is stored after the position, the spin's
// random numbers start one mix later.
DECOMP(0079F3E8, SphereVolumeShape_Spawn_0079F3E8) {
    constexpr u32 K = g16::kSphereVolumeSpawnConsts;
    const u32 o = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1), vel = arg<u32>(ctx, 2);
    const f64 scale = lfs(K), two = lfs(K + 4), minus_one = lfs(K + 8);
    const u64 r0 = load<u64>(g13::g::random_last);
    const u32 st = load<u32>(g13::g::random_state);
    const u32 m1 = (rotl16(st, 4) + rotl16(st, 16)) ^ g13::kRandomMix;
    const u32 m2 = next_mix16(m1), m3 = next_mix16(m2), m4 = next_mix16(m3), m5 = next_mix16(m4);
    auto uniform = [&](u64 r) { return g11::madds(sgl(rand_f16(r) * scale), two, minus_one); };
    const f64 x = uniform(r0), y = uniform(m1 & 0xFFFF), z = uniform(m2 & 0xFFFF);
    const f64 len2 = g11::madds(z, z, g11::madds(x, x, sgl(y * y)));
    const f64 len = len2 < 0.0 ? std::numeric_limits<f64>::quiet_NaN() : sgl(std::sqrt(len2));  // fsqrts
    const f64 inv = sgl(lfs(K + 12) / len);
    const f64 dist = sgl(lfs(o + 36) * sgl(rand_f16(m3 & 0xFFFF) * scale));
    const f64 r4 = sgl(rand_f16(m4 & 0xFFFF) * scale);
    store<u32>(pos, g11::stfs_bits(sgl(sgl(x * inv) * dist)));
    store<u32>(pos + 4, g11::stfs_bits(sgl(sgl(y * inv) * dist)));
    store<u32>(pos + 8, g11::stfs_bits(sgl(sgl(z * inv) * dist)));
    const f64 speed = g11::nmsubs(lfs(o + 12), r4, lfs(o + 8));
    const f64 ux = lfs(g16::g::up), uy = lfs(g16::g::up + 4), uz = lfs(g16::g::up + 8);
    store<u32>(g13::g::random_state, rotl16(m5, 16));
    store<u64>(g13::g::random_last, m5 & 0xFFFF);
    store<u32>(vel, g11::stfs_bits(sgl(ux * speed)));
    store<u32>(vel + 4, g11::stfs_bits(sgl(uy * speed)));
    const f64 vz = sgl(uz * speed);
    store<u32>(vel + 8, g11::stfs_bits(vz));
    ctx.fpr[1] = vz;
    if (load<u8>(o + 16) == 0) return ret(ctx, o);
    spawn_spin16(ctx, K, K + 8, K + 16, o, vel, m5, false);
}

// void BoxSurfaceShape_Spawn_0079F9A0(const BoxShape* o, f32 pos[3], f32 vel[3])
// [vtable 0x00B231B4 slot 3, an emitter shape's spawn] (name?)
// A random direction d (as SphereShape_Spawn_0079EEF8's) pushed out to
// the box's surface: its largest |component| (x, then y, else z; ties to
// the earlier) picks the half extent e (o +48 / +52 / +56, the address
// built with rlwimi on o's bits 0x0C) and pos = d * e / |d_axis|; the
// velocity along the up direction as the box's, the state stored after
// it, then the spin (f1 left at the second angle when it is 0).
DECOMP(0079F9A0, BoxSurfaceShape_Spawn_0079F9A0) {
    constexpr u32 K = g16::kBoxSurfaceSpawnConsts;
    const u32 o = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1), vel = arg<u32>(ctx, 2);
    const f64 scale = lfs(K), two = lfs(K + 4), minus_one = lfs(K + 8);
    const u64 r0 = load<u64>(g13::g::random_last);
    const u32 st = load<u32>(g13::g::random_state);
    const u32 m1 = (rotl16(st, 4) + rotl16(st, 16)) ^ g13::kRandomMix;
    const u32 m2 = next_mix16(m1), m3 = next_mix16(m2), m4 = next_mix16(m3);
    auto uniform = [&](u64 r) { return g11::madds(sgl(rand_f16(r) * scale), two, minus_one); };
    const f64 x = uniform(r0), y = uniform(m1 & 0xFFFF), z = uniform(m2 & 0xFFFF);
    const f64 len2 = g11::madds(z, z, g11::madds(x, x, sgl(y * y)));
    const f64 len = len2 < 0.0 ? std::numeric_limits<f64>::quiet_NaN() : sgl(std::sqrt(len2));  // fsqrts
    const f64 inv = sgl(lfs(K + 12) / len);
    const f64 d[3] = {sgl(x * inv), sgl(y * inv), sgl(z * inv)};
    const f64 ax = std::fabs(d[0]), ay = std::fabs(d[1]), az = std::fabs(d[2]);
    const u32 axis = (ax >= ay && ax >= az) ? 0u : (ay >= ax && ay >= az) ? 1u : 2u;  // fcmpu / cror gt|eq
    const u32 e = ((o & ~0xCu) | (axis << 2)) + 48;  // rlwimi 2,28,29
    const f64 k = sgl(lfs(e) / std::fabs(g10::lfs_value(g11::stfs_bits(d[axis]))));  // the stack copy
    const f64 r3 = sgl(rand_f16(m3 & 0xFFFF) * scale);
    for (u32 i = 0; i < 3; ++i) store<u32>(pos + 4 * i, g11::stfs_bits(sgl(d[i] * k)));
    const f64 speed = g11::nmsubs(lfs(o + 12), r3, lfs(o + 8));
    store<u32>(g13::g::random_state, rotl16(m4, 16));
    store<u64>(g13::g::random_last, m4 & 0xFFFF);
    for (u32 i = 0; i < 3; ++i) store<u32>(vel + 4 * i, g11::stfs_bits(sgl(lfs(g16::g::up + 4 * i) * speed)));
    ctx.fpr[1] = scale;  // this variant keeps the scale in f1
    if (load<u8>(o + 16) == 0) return ret(ctx, o);
    spawn_spin16(ctx, K, K + 8, K + 16, o, vel, m4, false, true);
}

namespace {
// The colour words SlyCharacterFur_LoadParams swizzles: bytes 1 and 3
// swapped (rlwinm / rlwimi).
inline u32 swap_b13_16(u32 w) { return (w & 0xFF00FF00u) | (rotl16(w, 16) & 0x00FF00FFu); }
}  // namespace

// bool SlyCharacterFur_LoadParams(ShaderType*, Material* m, const Desc* d)
// [vtable 0x00B259F0 slot 3, the "SlyCharacterFur" type's LoadParams] (name?)
// Nothing (returns 1) with config +152 bit 7. The texture named d+4 (the
// default texture of kind 0 when not loaded) gets slot 4 with (t, 1, 1,
// 1) and is added; params 0..2 = d+88..+96, 7..14 = d+116..+144 (raw
// words); param 4 = (version (u8 d+84) > 4 ? d+104 : 15), 0 when not
// positive; param 15 = d+148 (version > 15, else 1.0), 17 = d+156 (> 17);
// params 16 / 18 = the colours d+152 (> 16) / d+160 (> 18) (else 0) with
// bytes 1 and 3 swapped; param 19 = min(d+164 (> 19, else 14), 14).
// Returns 1.
DECOMP(005C0F50, SlyCharacterFur_LoadParams) {
    constexpr u32 K = g16::kSlyFurConsts;
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if ((field<u8>(sly4::g::config, 152) & 0x80) != 0) return ret(ctx, 1u);  // rlwinm 25,31,31
    u32 t = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 4)}));
    if (t == 0) t = static_cast<u32>(call(ctx, g15::fn::Tex_GetDefault, {0}));
    fx::vcall(ctx, t, 16, {t, 1, 1, 1});
    call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t});
    for (u32 k = 0; k < 3; ++k) set_param16(ctx, m, k, load<u32>(d + 88 + 4 * k));  // lfs / stfs
    for (u32 k = 7; k <= 14; ++k) set_param16(ctx, m, k, load<u32>(d + 116 + 4 * (k - 7)));
    auto ver = [&]() { return static_cast<s32>(ref<u8>(d + 84)); };  // cmpwi
    {
        const f64 x = ver() > 4 ? lfs(d + 104) : lfs(K);
        const f64 v = -x >= 0.0 ? g11::lfd(K + 8) : x;  // fneg, fsel
        set_param16(ctx, m, 4, g11::stfs_bits(sgl(v)));  // frsp
    }
    const s32 v1 = ver();
    const u32 c16 = v1 > 16 ? load<u32>(d + 152) : 0u;
    const u32 p15 = v1 > 15 ? load<u32>(d + 148) : load<u32>(K + 16);
    const u32 p17 = v1 > 17 ? load<u32>(d + 156) : load<u32>(K + 16);
    set_param16(ctx, m, 15, p15);
    set_param16(ctx, m, 17, p17);
    set_param16(ctx, m, 16, swap_b13_16(c16));
    const u32 c18 = ver() > 18 ? load<u32>(d + 160) : 0u;
    set_param16(ctx, m, 18, swap_b13_16(c18));
    const u32 n = ver() > 19 ? load<u32>(d + 164) : g16::kFurLayersMax;
    set_param16(ctx, m, 19, n < g16::kFurLayersMax ? n : g16::kFurLayersMax);  // cmplwi
    ret(ctx, 1u);
}

namespace {
// The original's lwarx / addi / stwcx. loop: an atomic += v.
void atomic_add16(u32 ea, u32 v) {
    std::atomic_ref<u32> a(ref<u32>(ea));
    u32 old = a.load();
    while (!a.compare_exchange_weak(old, __builtin_bswap32(__builtin_bswap32(old) + v))) {
    }
}
}  // namespace

// void EmitterJobs_Submit_00528130()  [called by f_00599864] (name?)
// The batched emitters' SPU pass: for the g::emit_items (80 bytes each,
// u32 count g::emit_item_count) a 32-byte slot table (gfx14::g::draw_slots
// = frame-stack memory, n slots) and n 144-byte records; each item's slot
// (+76) is cleared and its renderer (+64) fills the next record through
// vtable +16 (renderer, record, item, +68, +72, slot); the records it
// accepts are counted. With any: a 160-byte job header from the frame
// stack (the view's +752 rows 0..3 xyz, layers4::g::view_proj, the
// stream buffers +384 / +512 and +232 / +236, the first vertex format's
// u16 stride, the command buffer) and the records go to f_0056F050 in
// jobs of up to 16, the busy counter (gfx14::g::draw_busy, its pointer
// made once at 0x010F0DE0) atomically += 1 per job; then u8
// g::emitters_batched = 1.
DECOMP(00528130, EmitterJobs_Submit_00528130) {
    namespace g = g16::g;
    const u32 n = load<u32>(g::emit_item_count);
    if (n == 0) return;  // r3 left as it came
    const u32 stack = load<u32>(g::arena + 280);
    {
        const u32 slots = static_cast<u32>(call(ctx, g16::fn::MemStack_Alloc, {stack, (n << 5) & ~31u}));
        const u32 n2 = load<u32>(g::emit_item_count);
        const u32 stack2 = load<u32>(g::arena + 280);
        store<u32>(g14::g::draw_slots, slots);
        ctx.gpr[3] = call(ctx, g16::fn::MemStack_Alloc, {stack2, n2 * g16::kEmitJobRecordSize});
    }
    u32 recs = static_cast<u32>(ctx.gpr[3]);
    u32 count = 0;
    u32 r3 = load<u32>(g::emit_item_count);
    if (r3 == 0) return ret(ctx, recs);
    for (u32 i = 0; i < r3; ++i) {  // cmplw, the count re-read each time
        const u32 e = g::emit_items + g16::kEmitItemSize * i;
        const u32 off = (load<u32>(e + 76) << 5) & ~31u;
        const u32 slot = load<u32>(g14::g::draw_slots) + off;
        const u32 out = recs + count * g16::kEmitJobRecordSize;
        store<u32>(slot, 0);
        store<u32>(slot + 8, 0);
        store<u32>(slot + 12, 0);
        store<u32>(slot + 4, 0);
        const u32 rend = load<u32>(e + 64);
        const u32 slot2 = load<u32>(g14::g::draw_slots) + off;
        const u32 opd = load<u32>(load<u32>(rend) + 16);
        const u32 a = load<u32>(e + 68), b = load<u32>(e + 72);
        if (static_cast<u32>(call_ptr(ctx, opd, {rend, out, e, a, b, slot2, opd, load<u32>(opd)})) != 0) ++count;
        r3 = load<u32>(g::emit_item_count);
    }
    if (count == 0) return ret(ctx, r3);
    const u32 job = static_cast<u32>(call(ctx, g16::fn::MemStack_Alloc, {load<u32>(g::arena + 280), g16::kEmitJobHeaderSize}));
    {
        const u32 view = load<u32>(load<u32>(g15::g::current_surface) + 672) + g15::kViewMatrix;
        for (u32 row = 0; row < 64; row += 16)
            for (u32 k = 0; k < 12; k += 4) store<u32>(job + row + k, load<u32>(view + row + k));  // lfs / stfs pairs
        for (u32 k = 0; k < 64; k += 4) store<u32>(job + 64 + k, load<u32>(l4::g::view_proj + k));
        const u32 sb = load<u32>(g::stream_buffers);
        store<u32>(job + 128, sb + 384);
        store<u32>(job + 132, sb + 512);
        store<u32>(job + 136, load<u32>(sb + 232));
        store<u32>(job + 140, load<u32>(sb + 236));
        store<u32>(job + 144, load<u16>(sly4::render::g::vertex_formats + 120));
        store<u32>(job + 148, load<u32>(g13::g::command_buffer));
    }
    if (load<u8>(g::busy_ptr_ready) == 0) {
        store<u32>(g::busy_ptr, g14::g::draw_busy);
        store<u8>(g::busy_ptr_ready, 1);
    }
    while (count != 0) {
        atomic_add16(load<u32>(g::busy_ptr), 1);
        if (count < g16::kEmitJobBatch) {  // cmplwi
            call(ctx, g16::fn::EmitterJob_Run_0056F050, {job, recs, count, g::busy_ptr});
            count = 0;
        } else {
            call(ctx, g16::fn::EmitterJob_Run_0056F050, {job, recs, g16::kEmitJobBatch, g::busy_ptr});
            count -= g16::kEmitJobBatch;
            recs += g16::kEmitJobBatch * g16::kEmitJobRecordSize;
        }
    }
    store<u8>(g::emitters_batched, 1);
    ctx.gpr[3] = 0x010F0000u;  // as the original leaves it (lis)
}
