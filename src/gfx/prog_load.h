// Loading Cg programs by name: look the name hash up in the vertex /
// fragment program map, else load System\Shaders\<name>_V.vpo / .fpo,
// create the program and register it. VProgram_Load / FProgram_Load
// (src/gfx/prog_pair.cpp) are the out-of-line forms; the effect
// initialisers inline the same code, so it lives here once.
#pragma once

#include "post.h"

namespace sly4::progload {

namespace fn {
constexpr u32 ProgramPair_Load = 0x00534160;  // decompiled: src/gfx/prog_pair.cpp
constexpr u32 VProgram_Load = 0x00534074;     // decompiled: src/gfx/prog_pair.cpp
constexpr u32 FProgram_Load = 0x00533DF8;     // decompiled: src/gfx/prog_pair.cpp
constexpr u32 FProgram_Finish = 0x0053378C;   // decompiled: src/gfx/prog_pair.cpp
// Still recompiled: libgcm Cg helpers.
constexpr u32 Cg_GetParameterType = 0x005E5F10;          // (CgProgram*, CgParameter) -> s32 (name?)
constexpr u32 Cg_GetEmbeddedConstantCount = 0x005E60DC;  // (CgProgram*, CgParameter) -> u32 (name?)
constexpr u32 Cg_GetEmbeddedConstantOffset = 0x005E610C; // (CgProgram*, CgParameter, u32 i) -> u32 (name?)
}  // namespace fn

// A 256-byte path, the key and the file size, as each inlined copy has.
struct LoadFrame {
    explicit LoadFrame(PPUContext& ctx) : frame(ctx, 272) {}
    u32 path() const { return frame.addr(); }
    u32 key() const { return frame.addr() + 256; }
    u32 size() const { return frame.addr() + 260; }
    Stack frame;
};

// *global = the vertex program `name`; returns what r3 holds afterwards.
inline u64 load_vertex(PPUContext& ctx, u32 global, u32 name) {
    namespace pfn = sly4::post::fn;
    LoadFrame f(ctx);
    store<u32>(f.key(), static_cast<u32>(call(ctx, pfn::String_HashNoCase, {name})));
    const u32 found = load<u32>(static_cast<u32>(call(ctx, pfn::VMap_Find, {post::g::vmap, f.key()})));
    store<u32>(global, found);
    if (found != 0) return found;
    call(ctx, pfn::String_Printf, {f.path(), post::g::fmt_vpo, name});
    store<u32>(f.size(), 0);
    const u32 data = static_cast<u32>(call(ctx, pfn::File_LoadWhole, {f.path(), f.size()}));
    call(ctx, pfn::VProgram_Create, {global, data, 0});
    call(ctx, pfn::VMap_Insert, {post::g::vmap, f.key(), load<u32>(global)});
    const u64 mask = call(ctx, pfn::Cg_GetAttributeMask, {load<u32>(load<u32>(global) + 4)});
    store<u16>(load<u32>(global) + 10, static_cast<u16>(mask));
    return mask;
}

// *global = the fragment program `name`, bound with `bindings` when it
// is loaded; returns what r3 holds afterwards.
inline u64 load_fragment(PPUContext& ctx, u32 global, u32 name, u32 bindings) {
    namespace pfn = sly4::post::fn;
    LoadFrame f(ctx);
    store<u32>(f.key(), static_cast<u32>(call(ctx, pfn::String_HashNoCase, {name})));
    const u32 found = load<u32>(static_cast<u32>(call(ctx, pfn::FMap_Find, {post::g::fmap, f.key()})));
    store<u32>(global, found);
    if (found != 0) return found;
    call(ctx, pfn::String_Printf, {f.path(), post::g::fmt_fpo, name});
    store<u32>(f.size(), 0);
    const u32 data = static_cast<u32>(call(ctx, pfn::File_LoadWhole, {f.path(), f.size()}));
    call(ctx, pfn::FProgram_Create, {global, data, load<u32>(f.size())});
    call(ctx, pfn::FMap_Insert, {post::g::fmap, f.key(), load<u32>(global)});
    return call(ctx, pfn::FProgram_Setup, {global, bindings});
}

// At least `min` registers for the fragment program at *global.
inline void min_registers(PPUContext& ctx, u32 global, u8 min) {
    namespace pfn = sly4::post::fn;
    const u32 f = load<u32>(global);
    const u8 have = ref<u8>(f + 1156);
    ref<u8>(f + 1156) = have > min ? have : min;
    if (static_cast<u32>(call(ctx, pfn::Cg_GetRegisterCount, {load<u32>(load<u32>(global) + 4)})) < min)
        ctx.gpr[3] = call(ctx, pfn::Cg_SetRegisterCount, {load<u32>(load<u32>(global) + 4), min});
}

}  // namespace sly4::progload
