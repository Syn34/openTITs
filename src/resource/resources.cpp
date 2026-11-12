// Resource system start-up.
#include "resource.h"

#include "../container/container.h"
#include "../memory/mem.h"
#include "../platform/platform.h"

namespace sly4::resource {
using namespace opentit::native;
namespace rs = sly4::resource;
namespace pf = sly4::platform;

namespace {

// A 3x4 transform (rows of 3 floats, 16-byte stride) plus a vec3, copied
// from two constants. The original constructs it in place (with the usual
// null test on the static's address).
constexpr u32 kDefaultTransform = 0x01193010;
constexpr u32 kIdentityRows = 0x01164BD0;  // 4 rows of vec3, 16-byte stride
constexpr u32 kDefaultVec3 = 0x01164B30;
constexpr u32 kTransformVecOffset = 64;

constexpr u32 kResourceList = 0x0116BD00;     // PtrVector* (20 bytes) or 0
constexpr u32 kResourceListCapacity = 2048;
constexpr u32 kSecondList = 0x011944D8;       // PtrVector* (20 bytes) or 0
constexpr u16 kPtrVectorUnk6 = 4;             // stored at PtrVector+6 after construction

// The handle table: { Entry* entries; u16 used (+4); u16 free_top (+6); u16 count (+8); }
constexpr u32 kHandleTable = 0x01193004;
constexpr u32 kConfigHandleCount = 340;  // s32 in g::config: 0 -> 4096, negative -> 0
constexpr u32 kDefaultHandleCount = 4096;
constexpr u32 kHandleEntrySize = 128;    // also the alignment

// Values set between the two groups of chunk types.
constexpr u32 kResUnk01164CD4 = 0x01164CD4, kResUnkF3C5B0 = 0x00F3C5B0;
constexpr u32 kResUnk0117F010 = 0x0117F010;  // u32 = 0, u16 at +4 = 0
constexpr u32 kResUnk01194598 = 0x01194598, kResUnk01194EC4 = 0x01194EC4;
constexpr u32 kResUnk01194EC0 = 0x01194EC0, kResUnk01194ED0 = 0x01194ED0;

// Chunk handler OPDs.
constexpr u32 kIffPvs = 0x00F3C698, kIffPvsh = 0x00F3C6A0, kIffPvsr = 0x00F3C6A8;
constexpr u32 kIffOccp = 0x00F3C6B0, kIffOcch = 0x00F3C6B8, kIffOccv = 0x00F3C6C0, kIffOcht = 0x00F3C6C8,
              kIffOcft = 0x00F3C6D0, kIffOcnt = 0x00F3C6D8;
constexpr u32 kIffLoca = 0x00F3CA08, kIffLcrh = 0x00F3CA10, kIffLcrd = 0x00F3CA18;

struct Vec3 {
    be_u32 x, y, z;  // f32 bits (copied, not computed)
};
struct Transform {
    struct Row {
        Vec3 v;
        be_u32 w;
    } rows[4];  // +0, 16-byte stride
    Vec3 vec;   // +64
};
struct HandleTable {
    be_u32 entries;    // +0
    be_u16 used;       // +4
    be_u16 free_top;   // +6
    be_u16 count;      // +8
};

void register_chunk(u32 code, u32 handler, u32 flags) { pf::Iff_RegisterChunk(code, handler, 0, nullptr, flags); }

// Mem_Alloc(20, 0, 16) + PtrVector_Construct + (+6) = 4; 0 on failure.
container::PtrVector* new_ptr_vector() {
    auto* v = static_cast<container::PtrVector*>(memory::Mem_Alloc(20, 0, 16));
    if (!v) return nullptr;
    call(rs::fn::PtrVector_Construct, v);
    v->elem_size = kPtrVectorUnk6;
    return v;
}

}  // namespace

bool Resources_Init() {
    // lfs/stfs pairs in the original: a plain 32-bit copy.
    Transform& t = global<Transform>(kDefaultTransform);
    const Transform::Row* rows = host<Transform::Row>(kIdentityRows);
    for (u32 row = 0; row < 4; ++row) t.rows[row].v = rows[row].v;
    t.vec = global<Vec3>(kDefaultVec3);
    container::PtrVector* list = new_ptr_vector();
    if (list) container::PtrVector_Init(list, kResourceListCapacity, nullptr, 0);
    global<gp<container::PtrVector>>(kResourceList) = list;
    const s32 configured = *host<be_s32>(g::config + kConfigHandleCount);
    const u32 count = configured == 0 ? kDefaultHandleCount : configured > 0 ? static_cast<u32>(configured) : 0;
    HandleTable& table = global<HandleTable>(kHandleTable);
    table.entries = guest(memory::Mem_Alloc((count << 7) & 0xFFFFFF80u, 0, kHandleEntrySize));
    table.count = static_cast<u16>(count);
    table.used = 0;
    table.free_top = static_cast<u16>(count - 1);
    for (u32 init : {rs::fn::ResPool_Init378, rs::fn::ResPool_Init374, rs::fn::ResPool_Init376,
                     rs::fn::ResPool_Init384, rs::fn::ResPool_Init386, rs::fn::ResPool_Init370,
                     rs::fn::ResPool_Init360, rs::fn::ResPool_Init364, rs::fn::ResPool_Init362,
                     rs::fn::ResPool_Init380, rs::fn::ResPool_Init366, rs::fn::ResPool_Init372})
        call(init);
    using pf::fourcc;
    register_chunk(fourcc("PVS "), kIffPvs, 0);
    register_chunk(fourcc("PVSH"), kIffPvsh, 0);
    register_chunk(fourcc("PVSR"), kIffPvsr, 0);
    register_chunk(fourcc("OCCP"), kIffOccp, pf::kIffFlag10);
    register_chunk(fourcc("OCCH"), kIffOcch, pf::kIffFlag10);
    register_chunk(fourcc("OCCV"), kIffOccv, pf::kIffFlag10);
    register_chunk(fourcc("OCHT"), kIffOcht, pf::kIffFlag10);
    register_chunk(fourcc("OCFT"), kIffOcft, pf::kIffFlag10);
    register_chunk(fourcc("OCNT"), kIffOcnt, pf::kIffFlag10);
    global<be_u32>(kResUnk01164CD4) = kResUnkF3C5B0;
    global<be_u32>(kResUnk0117F010) = 0;
    global<be_u32>(kResUnk01194598) = kResUnk01194EC4;
    global<be_u16>(kResUnk0117F010 + 4) = 0;
    global<be_u32>(kResUnk01194EC0) = kResUnk01194ED0;
    if (call<s32>(rs::fn::Res_Check586DB4) != 0) call(rs::fn::Res_Init586BF4);
    register_chunk(fourcc("LOCA"), kIffLoca, 0);
    register_chunk(fourcc("LCRH"), kIffLcrh, 0);
    register_chunk(fourcc("LCRD"), kIffLcrd, 0);
    call(rs::fn::Res_Init586B3C);
    call(rs::fn::ResPool_Init504);
    global<gp<container::PtrVector>>(kSecondList) = new_ptr_vector();
    return true;
}

}  // namespace sly4::resource

NATIVE(00588828, sly4::resource::Resources_Init)
