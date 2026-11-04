// The skinned depth draw and the occlusion query proxy draw.
#include "draw3.h"

#include <cmath>

using namespace sly4;

namespace {

namespace d3 = sly4::render::draw3;
namespace l4 = sly4::render::layers4;
namespace sp = sly4::render::pool;

// The lwarx / stwcx. loop in the original: an atomic ++; returns the old value.
u32 fetch_inc(u32 ea) {
    for (;;) {
        const u32 old = mem::r32(ea);
        if (mem::cas32(ea, old, old + 1)) return old;
    }
}

// Uploads transpose(item rows * view_proj) as vertex constants 0..3 (one
// SET_TRANSFORM_CONSTANT_LOAD of 17 words), as layers4_draw.cpp's
// ZLayer_DrawBody does: rows read with lvx (address rounded down to 16),
// fused per lane in vmaddfp order, row 3 adding view_proj's row 3.
void upload_item_matrix(u32 item) {
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
    for (u32 k = 0; k < 4; ++k) {
        for (u32 i = 0; i < 4; ++i) store<f32>(cur + 8 + 16 * k + 4 * i, r[i][k]);  // stvlx / stvrx pairs
    }
}

}  // namespace

// void DrawItem_DrawDepthSkinned(Mesh* mesh, u32 skin_table, DrawItem* item, const f32* bones, Material*, u32 flags)   (name?)
// Uploads the fragment program, takes a bone matrix slot (the item matrix
// transposed to 3 x 4) and switches off vertex attributes 1..15. Then for
// each skin part: unless flags has kNoVertexArrays, the part's vertex
// buffer (when set) as attribute 3 (4 x UB, stride 8) and attribute 8 (2
// x half float at +4); a job record, filled by Skin_BuildJob. Stops
// silently when the matrices or job records run out (the latter counted
// in g::skin_job_overflow). After the last part, with g::skin_reset_state
// set, forgets the cached vertex state (render state +4 / +16 = 0, +8 =
// -1, +32 = 0xFFFF). The fifth argument (r7) isn't read.
DECOMP(005382C0, DrawItem_DrawDepthSkinned) {
    const u32 mesh = arg<u32>(ctx, 0), table = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2), bones = arg<u32>(ctx, 3);
    const u32 flags = arg<u32>(ctx, 5);
    const u32 st = fx::g::state;
    sp::upload_fragment(ctx);

    const u32 slot = fetch_inc(d3::g::skin_matrix_count);
    if (slot >= d3::kSkinEntries) {
        ctx.gpr[3] = d3::g::skin_matrix_count;  // left in r3
        return;
    }
    const u64 frame_off = static_cast<u64>(load<u32>(d3::g::skin_matrix_frame)) * (d3::kSkinEntries * d3::kMatrixSize);
    const u32 matrix = static_cast<u32>(slot * d3::kMatrixSize + d3::g::skin_matrices + frame_off);
    if (matrix == 0) {
        ctx.gpr[3] = frame_off;  // left in r3
        return;
    }
    for (u32 r = 0; r < 3; ++r) {
        for (u32 c = 0; c < 4; ++c) sp::copy_word(matrix + 16 * r + 4 * c, item + 16 * c + 4 * r);
    }
    for (u32 i = 1; i < 16; ++i) {
        call(ctx, d3::fn::cellGcmSetVertexDataArrayUnsafe, {load<u32>(st), i & 0xFF, 0, 0, 0, d3::kVertexF, 0, 0});
    }

    u32 skin = load<u32>(mesh + 112);
    if (load<u32>(skin + 8) != 0) {
        Stack frame(ctx, 24);
        const u32 desc = frame.addr();
        u32 part = 0;
        for (u32 i = 0;; ++i, part += d3::kPartSize) {
            if ((flags & d3::kNoVertexArrays) == 0) {  // rlwinm 0,1,1
                const u32 vb = load<u32>(load<u32>(skin + 4) + 4 * i);
                if (vb != 0) {
                    const u32 w = load<u32>(vb + 4);
                    call(ctx, d3::fn::cellGcmSetVertexDataArray, {load<u32>(st), 3, 0, 8, 4, d3::kVertexUB, w & 1, w >> 1});
                    const u32 w2 = load<u32>(vb + 4);
                    call(ctx, d3::fn::cellGcmSetVertexDataArray,
                         {load<u32>(st), 8, 0, 8, 2, d3::kVertexSF, w2 & 1, (w2 >> 1) + 4});
                }
            }
            const u32 n = fetch_inc(d3::g::skin_job_count);
            u32 job = 0;
            u64 r3 = 0x01150000;  // what the original leaves in r3 when it gives up
            if (n < d3::kSkinEntries) {
                const u64 job_off = static_cast<u64>(load<u32>(d3::g::skin_job_frame)) * (d3::kSkinEntries * d3::kJobSize);
                job = static_cast<u32>(((n << 8) & 0xFFFFFF00u) + d3::g::skin_jobs + job_off);
                r3 = job_off;
            } else {
                store<u32>(d3::g::skin_job_overflow, load<u32>(d3::g::skin_job_overflow) + 1);
            }
            if (job == 0) {
                ctx.gpr[3] = r3;
                return;
            }
            store<u32>(desc + 4, table);
            store<u32>(desc + 0, load<u32>(st));
            store<u32>(desc + 12, d3::g::skin_block);
            store<u32>(desc + 8, matrix);
            store<u32>(desc + 16, d3::g::skin_image);
            store<u32>(desc + 20, d3::kSkinImageQuads);
            sp::copy_word(job + 188, mesh + 88);
            call(ctx, d3::fn::Skin_BuildJob, {job, desc, load<u32>(load<u32>(mesh + 112)) + part, bones, flags});
            skin = load<u32>(mesh + 112);
            if (!(i + 1 < load<u32>(skin + 8))) break;
        }
    }
    if (ref<u8>(d3::g::skin_reset_state) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(st + 4, 0);
    store<s32>(st + 8, -1);
    store<u32>(st + 16, 0);
    store<u16>(st + 32, 0xFFFF);
    ret(ctx, -1);  // left in r3
}

// void Slot_DrawProxy(Slot* s)   (name?)
// Draws an occlusion query's proxy: a DrawItem made (DrawItem_Create) for
// the slot's mesh (+8) and its material from a descriptor holding the
// slot's 4 x 3 matrix (+16.., 16-byte rows), the colour g::colour_ca0 and
// otherwise zeros (+69 u8 255); the material's shader for it (picker slot
// 16, no view override) bound, the item matrix uploaded, then the mesh's
// slot 16 draw. Returns what that returns.
DECOMP(00527708, Slot_DrawProxy) {
    namespace l3 = sly4::render::layers3;
    const u32 s = arg<u32>(ctx, 0);
    // ItemDesc at the original's sp+112..+207 (the rows' w words and +90 are left unset).
    Stack frame(ctx, 96);
    const u32 d = frame.addr();
    for (u32 r = 0; r < 4; ++r) sp::copy3(d + 16 * r, s + 16 + 16 * r);
    store<u32>(d + 64, 0);
    store<u32>(d + 68, 0);
    store<u32>(d + 80, 0);
    store<u32>(d + 84, 0);
    ref<u8>(d + 88) = 0;
    ref<u8>(d + 89) = 0;
    store<u32>(d + 92, 0);
    ref<u8>(d + 68) = 0;
    ref<u8>(d + 69) = 255;
    store<u32>(d + 76, 0);
    for (u32 k = 0; k < 4; ++k) ref<u8>(d + 72 + k) = ref<u8>(l3::g::colour_ca0 + k);

    const u32 mesh = load<u32>(s + 8);
    const u32 mat = load<u32>(mesh + 16);
    const u32 item = static_cast<u32>(call(ctx, d3::fn::DrawItem_Create, {mesh, mat, d}));
    const u32 picker = load<u32>(mat + 16);
    const u32 shader = static_cast<u32>(fx::vcall(ctx, picker, render::layer2::kPickShader, {picker, mat, item}));
    fx::bind_vertex(ctx, shader + 4);
    fx::bind_fragment(shader + 8);
    upload_item_matrix(item);
    const u32 m = load<u32>(s + 8);
    ctx.gpr[3] = fx::vcall(ctx, m, 16, {m, item});
}

// void Render_Init()   (gfx.h Agl_Init50E410; name?)
// Allocates the render module's buffers from g::config's sizes (0 takes
// the default; a negative size gives 0): two frame buffers (+324, pool 0),
// two more (+328, pool 1; none when 0) and a LinearHeap (+424); then
// registers the render layers, clears a few flags, calls 0x00546EF4,
// zeroes an 80-byte block and sets up the target at 0x010DDB88. Returns
// what RenderTarget_SetParams returns.
DECOMP(0050E410, Render_Init) {
    namespace in = d3::init;
    namespace l3 = sly4::render::layers3;
    const u32 cfg = sly4::g::config, b = l3::g::block_dd600;
    const auto size_of = [](u32 v, u32 def) -> u32 { return v == 0 ? def : (static_cast<s32>(v) > 0 ? v : 0); };

    store<u32>(b + 8, size_of(load<u32>(cfg + in::kCfgFrameBytes), in::kDefaultFrameBytes));
    store<u32>(b + 12, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {load<u32>(b + 8), 0, 16})));
    store<u32>(b + 16, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {load<u32>(b + 8), 0, 16})));
    const u32 n = size_of(load<u32>(cfg + in::kCfgFrameBytes2), 0);
    store<u32>(b + 260, n);
    if (n != 0) {
        store<u32>(b + 264, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {n, 1, 16})));
        store<u32>(b + 268, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {load<u32>(b + 260), 1, 16})));
    } else {
        store<u32>(b + 264, 0);
        store<u32>(b + 268, 0);
    }
    store<u32>(b, 0);
    const u32 heap_bytes = size_of(load<u32>(cfg + in::kCfgHeapBytes), in::kDefaultHeapBytes);
    u32 heap = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {36, 0, 16}));
    if (heap != 0) call(ctx, sly4::fn::Heap_Construct, {heap, heap_bytes, 0, 0});
    store<u32>(b + 280, heap);
    call(ctx, render::layer::RenderLayers_Register);
    for (u32 i = 0; i < 3; ++i) store<u8>(in::flags_ddad4 + i, 0);
    call(ctx, in::fn::Render_Init546EF4);
    const u32 block = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {80, 0, 16}));
    store<u32>(in::block_f66fb8, static_cast<u32>(call(ctx, in::fn::memset, {block, 0, 80})));  // unchecked
    store<u8>(in::flag_ddbb0, 0);
    // r6 / r7 hold the constants' address and 0x010E0000 in the original (not read).
    // r3 is left as RenderTarget_SetParams returns it.
    callf(ctx, in::fn::RenderTarget_SetParams, {in::target_ddb88, 2042, 2043, in::kParams, 0x010E0000u, 240},
          {static_cast<f64>(load<f32>(in::kParams)), static_cast<f64>(load<f32>(in::kParams + 4))});
}
