// ApeDebug start-up.
#include "debug.h"

#include "../container/container.h"
#include "../memory/mem.h"
#include "../platform/platform.h"

namespace sly4::debug {
using namespace opentit::native;
namespace pf = sly4::platform;

namespace {

constexpr u32 kFontBmpSize = 0x00F686C4;  // u32 size, followed by the embedded font BMP
constexpr u32 kFontBmp = 0x00F686C8;      // "BM..."
constexpr u32 kFontMetrics = 0x00F67F98;  // glyph table given to DebugFont_Construct
constexpr u32 kStrBmp = 0x00AF39B0;       // "Bmp"
constexpr u32 kStrMemFont = 0x00AF39B8;   // "MEM:ApeDebugFont"
constexpr u32 kDebugFontSize = 44;

// ApeTListAllocator<ApeDebug::Prim>: two pools of { FreeList* list; u32 count; }.
struct PrimPool {
    gp<container::FreeList> list;  // +0
    be_u32 count;                  // +4 items per block
};
constexpr u32 kPrimPoolCount = 2;
constexpr u32 kPrimPools = 0x01164E50;       // PrimPool* [2]
constexpr u32 kPrimPoolCurrent = 0x01164E58; // = kPrimPools[0]
constexpr u32 kPrimPoolUnk = 0x01164E5C;     // = 0
constexpr u32 kPrimsPerBlock = 16;
constexpr u32 kPrimSize = 124;
constexpr u32 kFreeListSize = 60;
constexpr u32 kPrimFreeListVtable = 0x00B24418;
constexpr u32 kStrPrimAllocator = 0x00AF35F3;  // "void ApeTListAllocator<T>::CreateAllocator() [with T = ApeDebug::Prim]"

// The function-local static { u64 main_thread_id; u64; } of Main_InitSystems
// (src/system/init.cpp), constructed here too if nobody has yet.
constexpr u32 kMainThread = 0x011DE290, kMainThreadGuard = 0x011DE2A0;

constexpr u32 kDebugShaderHash = 0x03DE9E33;
constexpr u32 kSphereRings = 8, kSphereSegments = 8;

constexpr u32 kBlendEnable = 0x80000000;
constexpr u32 kBlendSrcMask = 0x7FFF8000, kBlendSrcShift = 15;  // word +0
constexpr u16 CELL_GCM_SRC_ALPHA = 0x0302, CELL_GCM_ONE_MINUS_SRC_ALPHA = 0x0303, CELL_GCM_FUNC_ADD = 0x8006;
constexpr u32 kBlendWord2Bit31 = 0x80000000;  // cleared in word +8 (meaning unknown)

constexpr u32 kStrStars = 0x00AF39D0;   // "*****..."
constexpr u32 kStrApeDebug = 0x00AF3A10;  // "APE_DEBUG enabled in APE_FINAL\n"
constexpr u32 kPrintf = 0x007240F4;

}  // namespace

// Loads the debug font from the embedded BMP, creates the two
// ApeDebug::Prim pools, an alpha-blended debug material and sphere/cube
// meshes, and prints "APE_DEBUG enabled in APE_FINAL". Fails only if the
// font texture can't be created.
bool Debug_Init() {
    DebugState& st = global<DebugState>(kDebugState);
    global<be_u32>(kDebugState) = 0;
    st.font = nullptr;
    st.material = nullptr;
    st.sphere = nullptr;
    st.cube = nullptr;
    st.enabled = 1;

    const u32 texture = call<u32>(fn::Texture_LoadFromMemory, kFontBmp, global<be_u32>(kFontBmpSize).get(), kStrBmp, kStrMemFont);
    if (texture == 0) return false;
    // (The original also tests kFontMetrics against 0, which can't fail.)
    DebugFont* font = static_cast<DebugFont*>(memory::Mem_New(kDebugFontSize));
    if (font) call(fn::DebugFont_Construct, font, kFontMetrics, texture);
    st.font = font;

    gp<PrimPool>* pools = host<gp<PrimPool>>(kPrimPools);
    for (u32 i = 0; i < kPrimPoolCount; ++i) {
        PrimPool* pool = nullptr;
        if (PrimPool* p = static_cast<PrimPool*>(memory::Mem_New(sizeof(PrimPool)))) {
            p->list = nullptr;
            p->count = kPrimsPerBlock;
            pool = p;
        }
        pools[i] = pool;
        PrimPool& pr = gref<PrimPool>(guest(pool));  // unchecked: address 0 if the allocation failed
        if (!pr.list) {
            container::FreeList* list = nullptr;
            if (auto* f = static_cast<container::FreeList*>(memory::Mem_New(kFreeListSize))) {
                const u32 count = pr.count;
                be_u32* w = reinterpret_cast<be_u32*>(f);
                for (u32 off = 4; off <= 48; off += 4) w[off / 4] = 0;
                be_u16* h = reinterpret_cast<be_u16*>(f);
                h[52 / 2] = 0;
                h[54 / 2] &= static_cast<u16>(~0x8000);
                w[0] = kPrimFreeListVtable;
                w[56 / 4] = kStrPrimAllocator;
                call(pf::fn::FreeList_Init, f, kPrimSize, count, 0u, 0u);
                list = f;
            }
            pr.list = list;
        }
        u8& guard = global<u8>(kMainThreadGuard);
        if (guard == 0) {
            be_u64* main_thread = host<be_u64>(kMainThread);
            main_thread[0] = 0;
            main_thread[1] = 0;
            guard = 1;
        }
    }
    global<gp<PrimPool>>(kPrimPoolCurrent) = pools[0];
    global<be_u32>(kPrimPoolUnk) = 0;

    const u32 shader = call<u32>(fn::Shader_Find, kDebugShaderHash);
    st.material = call<Material*>(fn::Material_Create, shader);
    {
        MaterialBlend& blend = st.material->blend;
        const u32 w0 = blend.w0 | kBlendEnable;
        blend.dst = CELL_GCM_ONE_MINUS_SRC_ALPHA;
        blend.equation = CELL_GCM_FUNC_ADD;
        blend.w0 = (w0 & ~kBlendSrcMask) | ((static_cast<u32>(CELL_GCM_SRC_ALPHA) << kBlendSrcShift) & kBlendSrcMask);
    }
    {
        MaterialBlend& blend = st.material->blend;  // re-read, as in the original
        blend.w8 = blend.w8 & ~kBlendWord2Bit31;
    }
    st.sphere = call<Mesh*>(fn::Mesh_CreateSphere, st.material.get(), kSphereRings, kSphereSegments);
    st.cube = call<Mesh*>(fn::Mesh_CreateCube, st.material.get(), 1u);

    call(kPrintf, kStrStars);
    call(kPrintf, kStrApeDebug);
    call(kPrintf, kStrStars);
    return true;
}

}  // namespace sly4::debug

NATIVE(0056A944, sly4::debug::Debug_Init)
