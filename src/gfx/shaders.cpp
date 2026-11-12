// Shader start-up.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 kDefaultShaderCount = 256;
constexpr u32 kStrShaderPrefix = 0x00AF1A04;  // "System\\Shaders\\"
constexpr u32 kStrShaderZip = 0x00AF1A14;     // "System\\Shaders.Zip"
constexpr u32 kArchiveBuffer = 0x4000;
constexpr u32 kOnce54CDB0 = 0x00F670D8;       // u8
constexpr u32 kIffMatl = 0x00F3B4B8, kIffMtlh = 0x00F3B4C0, kIffMtlc = 0x00F3B4C8;  // handler OPDs
constexpr u32 kShaderPool = 0x00F8F114;       // ShaderPool*
constexpr u32 kConfigShaderCount = 348;       // s16 in g::config
constexpr u32 kConfigShaderData = 84;         // const void*: preloaded Shaders.Zip, or 0
constexpr u32 kConfigShaderPath = 88;         // const char*: shader archive path, or 0
constexpr u32 kShaderGroups = 6;
constexpr u32 Iff_RegisterChunk = 0x0055902C;
constexpr u32 Archive_MountMemory = 0x00567F2C;  // (const void* data, const char* prefix, u32 buffer) -> archive index or 0 (name?)
constexpr u32 Archive_MountFile = 0x0055D684;    // (const char* path, const char* prefix, u32 buffer) -> archive index or 0 (name?)
constexpr u32 Archive_Unmount = 0x0055E350;      // (u16 archive index)
constexpr u32 Shader_RegisterAll = 0x0050B138;   // () -> bool (src/gfx/shader3_registry.cpp)
constexpr u32 Shaders_Init54CDB0 = 0x0054CDB0;   // () once (unknown)
constexpr u32 Shader_LoadGroup = 0x0050F0F0;     // (s32 group 0..5) (name?)

struct ShaderPool {
    be_u32 w0;     // +0
    be_u32 count;  // +4
    be_u32 w8;     // +8
};

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
bool Shaders_Init() {
    const u16 raw = global<be_u16>(sly4::g::config + kConfigShaderCount);
    u32 count = kDefaultShaderCount;
    if (raw != 0) count = static_cast<s16>(raw) > 0 ? static_cast<u32>(static_cast<s16>(raw)) : 0;
    ShaderPool* pool = nullptr;
    ShaderPool* p = call<ShaderPool*>(sly4::fn::Mem_Alloc, 16u, 0u, 16u);
    if (p) {
        p->w0 = 0u;
        p->w8 = 0u;
        p->count = count;
        pool = p;
    }
    global<gp<ShaderPool>>(kShaderPool) = pool;
    call(Iff_RegisterChunk, code('M', 'A', 'T', 'L'), kIffMatl, 0u, 0u, 0u);
    call(Iff_RegisterChunk, code('M', 'T', 'L', 'H'), kIffMtlh, 0u, 0u, 0u);
    call(Iff_RegisterChunk, code('M', 'T', 'L', 'C'), kIffMtlc, 0u, 0u, 0u);
    u32 archive;
    const u32 data = global<be_u32>(sly4::g::config + kConfigShaderData);
    if (data != 0) {
        archive = call<u32>(Archive_MountMemory, data, kStrShaderPrefix, kArchiveBuffer);
    } else {
        u32 path = global<be_u32>(sly4::g::config + kConfigShaderPath);
        if (path == 0) path = kStrShaderZip;
        archive = call<u32>(Archive_MountFile, path, kStrShaderPrefix, kArchiveBuffer);
    }
    if (archive == 0) return false;
    call(Shader_RegisterAll);
    PostProc_Init();
    PostProc_InitCopy();
    StencilShadow_Init();
    u8& once = global<u8>(kOnce54CDB0);
    if (once == 0) {
        once = 1;
        call(Shaders_Init54CDB0);
    }
    call(Archive_Unmount, archive);
    for (s32 group = 0; group < static_cast<s32>(kShaderGroups); ++group) call(Shader_LoadGroup, group);
    return true;
}

}  // namespace sly4::gfx

NATIVE(0050F308, sly4::gfx::Shaders_Init)
