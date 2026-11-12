// The skinned depth draw (DrawItem_DrawDepthSkinned), the occlusion
// query proxy draw (Slot_DrawProxy) and the normal item draw path's shader
// slots.
//
// Skinning runs on the SPUs: each skinned draw takes a 48-byte bone
// matrix (the item matrix transposed to 3 rows of 4) from g::skin_matrices
// and a 256-byte job record from g::skin_jobs, both double-buffered by a
// frame index, 0x300 entries per buffer, and both counted with an atomic
// increment. Skin_BuildJob fills the job record from a 24-byte
// descriptor: {+0 gcm context, +4 g::skin_table, +8 bone matrix, +12
// g::skin_block, +16 SPU image g::skin_image, +20 u32 its size in 16-byte
// units}.
//
// Mesh (DrawItem +12), as these read it: +16 Material*, +88 f32 copied
// into the job record (+188), +112 skin (+0 base of 128-byte part
// records, +4 VertexBuffer*[] per part (+4 offset << 1 | location), +8 u32
// part count). Mesh vtable slot 16 draws (mesh, item).
#pragma once

#include "pool.h"

namespace sly4::render {
namespace draw3 {

namespace fn {
// Decompiled in draw3*.cpp.
// (pool.h: pool::fn::DrawItem_DrawDepthSkinned 0x005382C0, pool::query::fn::Slot_DrawProxy 0x00527708.)
// Still recompiled.
constexpr u32 DrawItem_Create = 0x0079D6F8;  // native (rsx.h) (Mesh*, Material*, const ItemDesc*) -> 80-byte DrawItem from the frame arena (name?)
constexpr u32 Skin_BuildJob = 0x00537824;  // native (rsx.h) (job record, desc*, part record, bones, flags) (name?)
// libgcm.
constexpr u32 cellGcmSetVertexDataArray = 0x005E0BA4;        // (context, index, frequency, stride, size, type, location, offset) (name?)
constexpr u32 cellGcmSetVertexDataArrayUnsafe = 0x005E4394;  // the same without the buffer check (name?)
}  // namespace fn

namespace g {
constexpr u32 skin_matrix_count = 0x01163F00;  // u32, atomic: bone matrices taken this frame (name?)
constexpr u32 skin_matrix_frame = 0x01163E90;  // u32: buffer index for skin_matrices (name?)
constexpr u32 skin_matrices = 0x01151A80;      // 2 x 0x300 x 48 bytes (ends at pool::g::skin_table) (name?)
constexpr u32 skin_job_count = 0x01151900;     // u32, atomic: job records taken this frame (gcm.cpp kAtomic01151900) (name?)
constexpr u32 skin_job_frame = 0x01151880;     // u32: buffer index for skin_jobs (gcm.cpp kUnk01151880) (name?)
constexpr u32 skin_jobs = 0x010F1880;          // 2 x 0x300 x 256 bytes (name?)
constexpr u32 skin_job_overflow = 0x01151980;  // u32: draws dropped for want of a job record (gcm.cpp kUnk01151980) (name?)
constexpr u32 skin_block = 0x01163B00;         // passed in the job descriptor (name?)
constexpr u32 skin_image = 0x00D69100;         // SPU program image (name?)
constexpr u32 skin_reset_state = 0x00F674FD;   // u8: reset the vertex state after a skinned draw (name?)
}  // namespace g

constexpr u32 kSkinEntries = 0x300, kMatrixSize = 48, kJobSize = 256, kPartSize = 128;
constexpr u32 kSkinImageQuads = 0x952;
constexpr u32 kNoVertexArrays = 0x40000000;  // flags bit: leave the vertex arrays alone
constexpr u32 kVertexF = 2, kVertexSF = 3, kVertexUB = 4;  // CELL_GCM_VERTEX_F / _SF / _UB

// Render_Init (0x0050E410, gfx.h Agl_Init50E410): the render module's
// buffers. layers3::g::block_dd600 (0x010DD600): +0 u32 0, +8 u32 size
// and +12 / +16 two buffers of it (pool 0), +260 u32 size and +264 / +268
// two buffers of it (pool 1), +280 LinearHeap*.
namespace init {
constexpr u32 kCfgFrameBytes = 324, kCfgFrameBytes2 = 328, kCfgHeapBytes = 424;  // g::config fields (0: default)
constexpr u32 kDefaultFrameBytes = 0x40000, kDefaultHeapBytes = 0x8000;
constexpr u32 kParams = 0x0050E404;          // f32 pair in .text, RenderTarget_SetParams' f1 / f2
constexpr u32 flags_ddad4 = 0x010DDAD4;      // u8[3], cleared (name?)
constexpr u32 flag_ddbb0 = 0x010DDBB0;       // u8, cleared (name?)
constexpr u32 target_ddb88 = 0x010DDB88;     // layers3::g::block_ddb88: RenderTarget_SetParams(.., 2042, 2043, .., 240) (name?)
constexpr u32 block_f66fb8 = 0x00F66FB8;     // u32: an 80-byte zeroed block (name?)
namespace fn {
constexpr u32 Render_Init546EF4 = 0x00546EF4;  // () (unknown)
constexpr u32 RenderTarget_SetParams = 0x0053C9F8;
constexpr u32 memset = 0x00726E54;
}  // namespace fn
}  // namespace init

}  // namespace draw3
}  // namespace sly4::render
