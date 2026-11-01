// ApeDebug start-up.
#include "debug.h"

#include "../platform/platform.h"

using namespace sly4;
using namespace sly4::debug;
namespace dbg = sly4::debug;
namespace pf = sly4::platform;

namespace {

constexpr u32 kFontBmpSize = 0x00F686C4;  // u32 size, followed by the embedded font BMP
constexpr u32 kFontBmp = 0x00F686C8;      // "BM..."
constexpr u32 kFontMetrics = 0x00F67F98;  // glyph table given to DebugFont_Construct
constexpr u32 kStrBmp = 0x00AF39B0;       // "Bmp"
constexpr u32 kStrMemFont = 0x00AF39B8;   // "MEM:ApeDebugFont"
constexpr u32 kDebugFontSize = 44;

// ApeTListAllocator<ApeDebug::Prim>: two pools of { FreeList* list; u32 count; }.
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

// Blend state at Material+104.
constexpr u32 kMaterialBlend = 104;
constexpr u32 kBlendEnable = 0x80000000;
constexpr u32 kBlendSrcMask = 0x7FFF8000, kBlendSrcShift = 15;  // word +0
constexpr u16 CELL_GCM_SRC_ALPHA = 0x0302, CELL_GCM_ONE_MINUS_SRC_ALPHA = 0x0303, CELL_GCM_FUNC_ADD = 0x8006;
constexpr u32 kBlendWord2Bit31 = 0x80000000;  // cleared in word +8 (meaning unknown)

constexpr u32 kStrStars = 0x00AF39D0;   // "*****..."
constexpr u32 kStrApeDebug = 0x00AF3A10;  // "APE_DEBUG enabled in APE_FINAL\n"
constexpr u32 kPrintf = 0x007240F4;

}  // namespace

// bool Debug_Init()
// Loads the debug font from the embedded BMP, creates the two
// ApeDebug::Prim pools, an alpha-blended debug material and sphere/cube
// meshes, and prints "APE_DEBUG enabled in APE_FINAL". Fails only if the
// font texture can't be created.
DECOMP(0056A944, Debug_Init) {
    DebugState& st = ref<DebugState>(dbg::kDebugState);
    store<u32>(dbg::kDebugState, 0);
    st.font = 0;
    st.material = 0;
    st.sphere = 0;
    st.cube = 0;
    st.enabled = 1;

    const u32 texture = static_cast<u32>(
        call(ctx, dbg::fn::Texture_LoadFromMemory, {kFontBmp, load<u32>(kFontBmpSize), kStrBmp, kStrMemFont}));
    if (texture == 0) return ret(ctx, 0u);
    // (The original also tests kFontMetrics against 0, which can't fail.)
    u32 font = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {kDebugFontSize}));
    if (font != 0) call(ctx, dbg::fn::DebugFont_Construct, {font, kFontMetrics, texture});
    st.font = font;

    for (u32 i = 0; i < kPrimPoolCount; ++i) {
        u32 pool = 0;
        const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {8}));
        if (p != 0) {
            store<u32>(p, 0);
            store<u32>(p + 4, kPrimsPerBlock);
            pool = p;
        }
        store<u32>(kPrimPools + 4 * i, pool);
        if (load<u32>(pool) == 0) {  // unchecked: reads address 0 if the allocation failed
            u32 list = 0;
            const u32 f = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {kFreeListSize}));
            if (f != 0) {
                const u32 count = load<u32>(pool + 4);
                for (u32 off = 4; off <= 48; off += 4) field<u32>(f, off) = 0;
                field<u16>(f, 52) = 0;
                field<u16>(f, 54) &= static_cast<u16>(~0x8000);
                field<u32>(f, 0) = kPrimFreeListVtable;
                field<u32>(f, 56) = kStrPrimAllocator;
                call(ctx, pf::fn::FreeList_Init, {f, kPrimSize, count, 0, 0});
                list = f;
            }
            store<u32>(pool, list);
        }
        if (ref<u8>(kMainThreadGuard) == 0) {
            store<u64>(kMainThread, 0);
            store<u64>(kMainThread + 8, 0);
            ref<u8>(kMainThreadGuard) = 1;
        }
    }
    store<u32>(kPrimPoolCurrent, load<u32>(kPrimPools));
    store<u32>(kPrimPoolUnk, 0);

    const u32 shader = static_cast<u32>(call(ctx, dbg::fn::Shader_Find, {kDebugShaderHash}));
    st.material = static_cast<u32>(call(ctx, dbg::fn::Material_Create, {shader}));
    {
        const u32 blend = st.material + kMaterialBlend;
        const u32 w0 = load<u32>(blend) | kBlendEnable;
        store<u16>(blend + 6, CELL_GCM_ONE_MINUS_SRC_ALPHA);
        store<u16>(blend + 4, CELL_GCM_FUNC_ADD);
        store<u32>(blend, (w0 & ~kBlendSrcMask) | ((static_cast<u32>(CELL_GCM_SRC_ALPHA) << kBlendSrcShift) & kBlendSrcMask));
    }
    {
        const u32 blend = st.material + kMaterialBlend;  // re-read, as in the original
        store<u32>(blend + 8, load<u32>(blend + 8) & ~kBlendWord2Bit31);
    }
    st.sphere = static_cast<u32>(call(ctx, dbg::fn::Mesh_CreateSphere, {st.material, kSphereRings, kSphereSegments}));
    st.cube = static_cast<u32>(call(ctx, dbg::fn::Mesh_CreateCube, {st.material, 1}));

    call(ctx, kPrintf, {kStrStars});
    call(ctx, kPrintf, {kStrApeDebug});
    call(ctx, kPrintf, {kStrStars});
    ret(ctx, 1u);
}
