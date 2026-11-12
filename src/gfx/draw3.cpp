// The skinned depth draw, the occlusion query proxy draw and the render
// module's start-up (Render_Init).
#include "draw3.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace d3 = sly4::render::draw3;
namespace d3g = sly4::render::draw3::g;
constexpr u32 kPickShader = 16;            // material picker vtable slot: (picker, mat, item) -> ShaderPrograms*
constexpr u32 kColourCA0 = 0x01164CA0;     // u8[4] (layers3::g::colour_ca0)
constexpr u32 kBlockDD600 = 0x010DD600;    // layers3::g::block_dd600

// The descriptor Skin_BuildJob reads.
struct SkinJobDesc {
    be_u32 context;  // +0
    be_u32 table;    // +4: g::skin_table
    be_u32 matrix;   // +8: bone matrix
    be_u32 block;    // +12: g::skin_block
    be_u32 image;    // +16: SPU image
    be_u32 quads;    // +20: its size in 16-byte units
};

// What DrawItem_Create reads (the original's sp+112..+207; the rows' w
// words and +90 are left unset).
struct ItemDesc {
    be_f32 rows[4][4];  // +0
    be_u32 w64;         // +64
    u8 b68;             // +68
    u8 b69;             // +69
    pad<2> _70;
    u8 colour[4];       // +72
    be_u32 w76;         // +76
    be_u32 w80;         // +80
    be_u32 w84;         // +84
    u8 b88;             // +88
    u8 b89;             // +89
    pad<2> _90;
    be_u32 w92;         // +92
};
static_assert(sizeof(ItemDesc) == 96);

// The render module's buffers (layers3::g::block_dd600).
struct RenderBuffers {
    be_u32 w0;             // +0
    pad<4> _4;
    be_u32 frame_bytes;    // +8
    be_u32 frame[2];       // +12: pool 0
    pad<240> _20;
    be_u32 frame_bytes2;   // +260
    be_u32 frame2[2];      // +264: pool 1
    pad<8> _272;
    be_u32 heap;           // +280: LinearHeap*
};
static_assert(offsetof(RenderBuffers, frame_bytes2) == 260 && offsetof(RenderBuffers, heap) == 280);

}  // namespace

// void DrawItem_DrawDepthSkinned(DrawMesh* mesh, u32 skin_table, DrawItem* item, const f32* bones, Material*, u32 flags)   (name?)
// Uploads the fragment program, takes a bone matrix slot (the item matrix
// transposed to 3 x 4) and switches off vertex attributes 1..15. Then for
// each skin part: unless flags has kNoVertexArrays, the part's vertex
// buffer (when set) as attribute 3 (4 x UB, stride 8) and attribute 8 (2
// x half float at +4); a job record, filled by Skin_BuildJob. Stops
// silently when the matrices or job records run out (the latter counted
// in g::skin_job_overflow). After the last part, with g::skin_reset_state
// set, forgets the cached vertex state (render state +4 / +16 = 0, +8 =
// -1, +32 = 0xFFFF). The fifth argument (r7) isn't read.
void DrawItem_DrawDepthSkinned(DrawMesh* mesh, u32 table, DrawItem* item, const be_f32* bones, void*, u32 flags) {
    RenderState& st = rs();
    upload_fragment();

    const u32 slot = fetch_inc(global<be_u32>(d3g::skin_matrix_count));
    if (slot >= d3::kSkinEntries) return;
    const u64 frame_off = static_cast<u64>(global<be_u32>(d3g::skin_matrix_frame).get()) * (d3::kSkinEntries * d3::kMatrixSize);
    const u32 matrix = static_cast<u32>(slot * d3::kMatrixSize + d3g::skin_matrices + frame_off);
    if (matrix == 0) return;
    be_u32* mtx = host<be_u32>(matrix);
    const be_u32* src = reinterpret_cast<const be_u32*>(item);
    for (u32 r = 0; r < 3; ++r)
        for (u32 c = 0; c < 4; ++c) copy_word(mtx + 4 * r + c, src + 4 * c + r);
    for (u32 i = 1; i < 16; ++i)
        call(d3::fn::cellGcmSetVertexDataArrayUnsafe, gcm(), i & 0xFF, 0u, 0u, 0u, d3::kVertexF, 0u, 0u);

    MeshSkin* skin = or_zero_page(mesh->skin.get());
    if (skin->count != 0) {
        GuestFrame f(24);
        SkinJobDesc* desc = f.as<SkinJobDesc>();
        u32 part = 0;
        for (u32 i = 0;; ++i, part += d3::kPartSize) {
            if ((flags & d3::kNoVertexArrays) == 0) {  // rlwinm 0,1,1
                const VertexBufferRef* vb = *at<gp<VertexBufferRef>>(skin->buffers.get(), 4 * i);
                if (vb) {
                    const u32 w = vb->where;
                    call(d3::fn::cellGcmSetVertexDataArray, gcm(), 3u, 0u, 8u, 4u, d3::kVertexUB, w & 1, w >> 1);
                    const u32 w2 = vb->where;
                    call(d3::fn::cellGcmSetVertexDataArray, gcm(), 8u, 0u, 8u, 2u, d3::kVertexSF, w2 & 1, (w2 >> 1) + 4);
                }
            }
            const u32 n = fetch_inc(global<be_u32>(d3g::skin_job_count));
            u32 job = 0;
            if (n < d3::kSkinEntries) {
                const u64 job_off = static_cast<u64>(global<be_u32>(d3g::skin_job_frame).get()) * (d3::kSkinEntries * d3::kJobSize);
                job = static_cast<u32>(((n << 8) & 0xFFFFFF00u) + d3g::skin_jobs + job_off);
            } else {
                be_u32& overflow = global<be_u32>(d3g::skin_job_overflow);
                overflow = overflow + 1;
            }
            if (job == 0) return;
            desc->table = table;
            desc->context = guest(gcm());
            desc->block = d3g::skin_block;
            desc->matrix = matrix;
            desc->image = d3g::skin_image;
            desc->quads = d3::kSkinImageQuads;
            copy_word(host<u8>(job + 188), &mesh->job_param);
            Skin_BuildJob(host<u8>(job), reinterpret_cast<const be_u32*>(desc), host<u8>(guest(or_zero_page(mesh->skin.get())->parts.get()) + part), guest(bones), flags);
            skin = or_zero_page(mesh->skin.get());
            if (!(i + 1 < skin->count)) break;
        }
    }
    if (global<u8>(d3g::skin_reset_state) == 0) return;
    st.index_buffer = 0u;
    st.vbase_cache = -1;
    st.vformat = nullptr;
    st.attr_mask = 0xFFFF;
}

// void Slot_DrawProxy(QuerySlot* s)   (name?)
// Draws an occlusion query's proxy: a DrawItem made (DrawItem_Create) for
// the slot's mesh (+8) and its material from a descriptor holding the
// slot's 4 x 3 matrix (+16.., 16-byte rows), the colour g::colour_ca0 and
// otherwise zeros (+69 u8 255); the material's shader for it (picker slot
// 16, no view override) bound, the item matrix uploaded, then the mesh's
// slot 16 draw.
void Slot_DrawProxy(QuerySlot* s) {
    GuestFrame f(96);
    ItemDesc* d = f.as<ItemDesc>();
    for (u32 r = 0; r < 4; ++r) copy3(d->rows[r], s->rows[r]);
    d->w64 = 0u;
    *at<be_u32>(d, 68) = 0u;
    d->w80 = 0u;
    d->w84 = 0u;
    d->b88 = 0;
    d->b89 = 0;
    d->w92 = 0u;
    d->b68 = 0;
    d->b69 = 255;
    d->w76 = 0u;
    for (u32 k = 0; k < 4; ++k) d->colour[k] = global<u8>(kColourCA0 + k);

    u8* mesh = s->mesh;
    u8* mat = *at<gp<u8>>(mesh, 16);
    DrawItem* item = DrawItem_Create(reinterpret_cast<DrawMesh*>(mesh), reinterpret_cast<Material*>(mat), reinterpret_cast<const u8*>(d));
    u8* picker = *at<gp<u8>>(mat, 16);
    ShaderPrograms* shader = or_zero_page(vcall<ShaderPrograms*>(picker, kPickShader, mat, item));
    bind_vertex(shader->vprogram);
    bind_fragment(shader->fprogram);
    upload_item_matrix(item);
    vcall(s->mesh.get(), 16, item);
}

// void Render_Init()   (gfx.h Agl_Init50E410; name?)
// Allocates the render module's buffers from g::config's sizes (0 takes
// the default; a negative size gives 0): two frame buffers (+324, pool 0),
// two more (+328, pool 1; none when 0) and a LinearHeap (+424); then
// registers the render layers, clears a few flags, calls 0x00546EF4,
// zeroes an 80-byte block and sets up the target at 0x010DDB88.
void Render_Init() {
    namespace in = d3::init;
    Config& cfg = config();
    RenderBuffers& b = global<RenderBuffers>(kBlockDD600);
    const auto size_of = [](u32 v, u32 def) -> u32 { return v == 0 ? def : (static_cast<s32>(v) > 0 ? v : 0); };

    b.frame_bytes = size_of(cfg.frame_bytes, in::kDefaultFrameBytes);
    b.frame[0] = call<u32>(sly4::fn::Mem_Alloc, b.frame_bytes.get(), 0u, 16u);
    b.frame[1] = call<u32>(sly4::fn::Mem_Alloc, b.frame_bytes.get(), 0u, 16u);
    const u32 n = size_of(cfg.frame_bytes2, 0);
    b.frame_bytes2 = n;
    if (n != 0) {
        b.frame2[0] = call<u32>(sly4::fn::Mem_Alloc, n, 1u, 16u);
        b.frame2[1] = call<u32>(sly4::fn::Mem_Alloc, b.frame_bytes2.get(), 1u, 16u);
    } else {
        b.frame2[0] = 0u;
        b.frame2[1] = 0u;
    }
    b.w0 = 0u;
    const u32 heap_bytes = size_of(cfg.heap_bytes, in::kDefaultHeapBytes);
    const u32 heap = call<u32>(sly4::fn::Mem_Alloc, 36u, 0u, 16u);
    if (heap != 0) call(sly4::fn::Heap_Construct, heap, heap_bytes, 0u, 0u);
    b.heap = heap;
    RenderLayers_Register();
    for (u32 i = 0; i < 3; ++i) global<u8>(in::flags_ddad4 + i) = 0;
    call(in::fn::Render_Init546EF4);
    const u32 block = call<u32>(sly4::fn::Mem_Alloc, 80u, 0u, 16u);
    global<be_u32>(in::block_f66fb8) = call<u32>(in::fn::memset, block, 0u, 80u);  // unchecked
    global<u8>(in::flag_ddbb0) = 0;
    // (The original leaves the constants' address and 0x010E0000 in the
    // GPR slots of the two float arguments; not read.)
    const be_f32* params = host<be_f32>(in::kParams);
    call(in::fn::RenderTarget_SetParams, in::target_ddb88, 2042u, 2043u, static_cast<f32>(params[0]),
         static_cast<f32>(params[1]), 240u);
}

}  // namespace sly4::gfx

NATIVE(005382C0, sly4::gfx::DrawItem_DrawDepthSkinned)
NATIVE(00527708, sly4::gfx::Slot_DrawProxy)
NATIVE(0050E410, sly4::gfx::Render_Init)
