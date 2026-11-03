// Shader start-up.
#include "shaders.h"

using namespace sly4;
namespace sh = sly4::shaders;

namespace {

constexpr u32 kDefaultShaderCount = 256;
constexpr u32 kStrShaderPrefix = 0x00AF1A04;  // "System\\Shaders\\"
constexpr u32 kStrShaderZip = 0x00AF1A14;     // "System\\Shaders.Zip"
constexpr u32 kArchiveBuffer = 0x4000;
constexpr u32 kOnce54CDB0 = 0x00F670D8;       // u8

constexpr u32 kIffMatl = 0x00F3B4B8, kIffMtlh = 0x00F3B4C0, kIffMtlc = 0x00F3B4C8;  // handler OPDs

constexpr u32 code(char a, char b, char c, char d) {
    return static_cast<u32>(static_cast<u8>(a)) | static_cast<u32>(static_cast<u8>(b)) << 8 |
           static_cast<u32>(static_cast<u8>(c)) << 16 | static_cast<u32>(static_cast<u8>(d)) << 24;
}

}  // namespace

// bool Shaders_Init()
// Creates the shader pool, registers MATL / MTLH / MTLC, mounts the shader
// archive (preloaded data from config +84, else the file named at config
// +88 or System\Shaders.Zip) and loads the six shader groups. Fails if the
// archive can't be mounted.
DECOMP(0050F308, Shaders_Init) {
    const u16 raw = field<u16>(g::config, sh::kConfigShaderCount);
    u32 count = kDefaultShaderCount;
    if (raw != 0) count = static_cast<s16>(raw) > 0 ? static_cast<u32>(static_cast<s16>(raw)) : 0;
    u32 pool = 0;
    const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {16, 0, 16}));
    if (p != 0) {
        store<u32>(p, 0);
        store<u32>(p + 8, 0);
        store<u32>(p + 4, count);
        pool = p;
    }
    store<u32>(sh::kShaderPool, pool);

    call(ctx, sh::fn::Iff_RegisterChunk, {code('M', 'A', 'T', 'L'), kIffMatl, 0, 0, 0});
    call(ctx, sh::fn::Iff_RegisterChunk, {code('M', 'T', 'L', 'H'), kIffMtlh, 0, 0, 0});
    call(ctx, sh::fn::Iff_RegisterChunk, {code('M', 'T', 'L', 'C'), kIffMtlc, 0, 0, 0});

    u32 archive;
    const u32 data = field<u32>(g::config, sh::kConfigShaderData);
    if (data != 0) {
        archive = static_cast<u32>(call(ctx, sh::fn::Archive_MountMemory, {data, kStrShaderPrefix, kArchiveBuffer}));
    } else {
        u32 path = field<u32>(g::config, sh::kConfigShaderPath);
        if (path == 0) path = kStrShaderZip;
        archive = static_cast<u32>(call(ctx, sh::fn::Archive_MountFile, {path, kStrShaderPrefix, kArchiveBuffer}));
    }
    if (archive == 0) return ret(ctx, 0u);

    // These take no arguments (r3 is left as the previous call's result, as
    // in the original).
    call(ctx, sh::fn::Shaders_Init50B138);
    call(ctx, sh::fn::PostProc_Init);
    call(ctx, sh::fn::PostProc_InitCopy);
    call(ctx, sh::fn::StencilShadow_Init);
    if (ref<u8>(kOnce54CDB0) == 0) {
        ref<u8>(kOnce54CDB0) = 1;
        call(ctx, sh::fn::Shaders_Init54CDB0);
    }
    call(ctx, sh::fn::Archive_Unmount, {archive});
    for (s32 group = 0; group < static_cast<s32>(sh::kShaderGroups); ++group)
        call(ctx, sh::fn::Shader_LoadGroup, {static_cast<u64>(static_cast<s64>(group))});
    ret(ctx, 1u);
}
