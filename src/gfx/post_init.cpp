// Start-up loading of the post-processing shader programs. Each program
// pair is looked up by name hash in the vertex / fragment program maps
// and loaded from System\Shaders\<name>_V.vpo / <name>.fpo when missing;
// some fragment programs then get a minimum register count.
#include "post.h"

using namespace sly4;

namespace {

namespace pfn = sly4::post::fn;
namespace pg = sly4::post::g;

constexpr u32 kConfig = g::config;

// One stack frame of the original per load: a 256-byte path, the key and
// the file size.
struct LoadFrame {
    explicit LoadFrame(PPUContext& ctx) : frame(ctx, 272) {}
    u32 path() const { return frame.addr(); }
    u32 key() const { return frame.addr() + 256; }
    u32 size() const { return frame.addr() + 260; }
    Stack frame;
};

u32 hash_into(PPUContext& ctx, const LoadFrame& f, u32 name) {
    store<u32>(f.key(), static_cast<u32>(call(ctx, pfn::String_HashNoCase, {name})));
    return f.key();
}

// *global = the vertex program `name`, loaded and registered if missing.
void load_vertex(PPUContext& ctx, u32 global, u32 name) {
    LoadFrame f(ctx);
    const u32 key = hash_into(ctx, f, name);
    const u32 found = load<u32>(static_cast<u32>(call(ctx, pfn::VMap_Find, {pg::vmap, key})));
    store<u32>(global, found);
    if (found != 0) return;
    call(ctx, pfn::String_Printf, {f.path(), pg::fmt_vpo, name});
    store<u32>(f.size(), 0);
    const u32 data = static_cast<u32>(call(ctx, pfn::File_LoadWhole, {f.path(), f.size()}));
    call(ctx, pfn::VProgram_Create, {global, data, 0});
    call(ctx, pfn::VMap_Insert, {pg::vmap, key, load<u32>(global)});
    const u32 mask = static_cast<u32>(call(ctx, pfn::Cg_GetAttributeMask, {load<u32>(load<u32>(global) + 4)}));
    store<u16>(load<u32>(global) + 10, static_cast<u16>(mask));
}

// *global = the fragment program `name`, loaded, registered and bound
// (FProgram_Setup with `bindings`) if missing.
void load_fragment(PPUContext& ctx, u32 global, u32 name, u32 bindings) {
    LoadFrame f(ctx);
    const u32 key = hash_into(ctx, f, name);
    const u32 found = load<u32>(static_cast<u32>(call(ctx, pfn::FMap_Find, {pg::fmap, key})));
    store<u32>(global, found);
    if (found != 0) return;
    call(ctx, pfn::String_Printf, {f.path(), pg::fmt_fpo, name});
    store<u32>(f.size(), 0);
    const u32 data = static_cast<u32>(call(ctx, pfn::File_LoadWhole, {f.path(), f.size()}));
    call(ctx, pfn::FProgram_Create, {global, data, load<u32>(f.size())});
    call(ctx, pfn::FMap_Insert, {pg::fmap, key, load<u32>(global)});
    call(ctx, pfn::FProgram_Setup, {global, bindings});
}

// At least `min` registers for the fragment program at *global.
void min_registers(PPUContext& ctx, u32 global, u8 min) {
    const u32 f = load<u32>(global);
    const u8 have = ref<u8>(f + 1156);
    ref<u8>(f + 1156) = have > min ? have : min;
    if (static_cast<u32>(call(ctx, pfn::Cg_GetRegisterCount, {load<u32>(load<u32>(global) + 4)})) < min)
        call(ctx, pfn::Cg_SetRegisterCount, {load<u32>(load<u32>(global) + 4), min});
}

struct Pair {
    u32 name;      // const char*, or the address it is read from (from_table)
    bool from_table;
    u32 vertex, fragment, bindings;
};

void load_pairs(PPUContext& ctx, const Pair* pairs, u32 count) {
    for (u32 i = 0; i < count; ++i) {
        const Pair& p = pairs[i];
        const u32 name = p.from_table ? load<u32>(p.name) : p.name;
        load_vertex(ctx, p.vertex, name);
        load_fragment(ctx, p.fragment, name, p.bindings);
    }
}

}  // namespace

// void PostProc_Init()
// Once (0x00F67920): the MSAA resolve programs (2x / 4x colour and
// depth) and PostProc_RestoreZCull; the four resolve fragment programs
// get at least 32 registers.
DECOMP(00534310, PostProc_Init) {
    constexpr u32 kDone = 0x00F67920;
    if (ref<u8>(kDone) != 0) return;
    ref<u8>(kDone) = 1;
    static constexpr Pair kPairs[] = {
        {0x00AF3344, false, 0x010F0FF4, 0x010F0FF8, 0},  // PostProc_Resolve2xMSAA
        {0x00AF335C, false, 0x010F0FFC, 0x010F1000, 0},  // PostProc_Resolve4xMSAA
        {0x00AF3374, false, 0x010F1004, 0x010F1008, 0},  // PostProc_ResolveDepth2xMSAA
        {0x00AF3394, false, 0x010F100C, 0x010F1010, 0},  // PostProc_ResolveDepth4xMSAA
        {0x00AF33B4, false, 0x010F1014, 0x010F1018, 0},  // PostProc_RestoreZCull
    };
    load_pairs(ctx, kPairs, 5);
    for (u32 f : {0x010F0FF8u, 0x010F1000u, 0x010F1008u, 0x010F1010u}) min_registers(ctx, f, 32);
}

// void PostProc_InitCopy()
// PostProc_InitEffects, then the copy / fill / texture / resample / blur
// programs (names and bindings from the effect tables at 0x00F67550..),
// their minimum register counts, and the blur tap offsets.
DECOMP(00546F6C, PostProc_InitCopy) {
    call(ctx, pfn::PostProc_InitEffects);
    static constexpr Pair kPairs[] = {
        {0x00F67560, true, 0x01164080, 0x01164084, 0x00F67564},   // PostProc_Copy
        {0x00F6756C, true, 0x01164088, 0x0116408C, 0x00F67570},   // PostProc_Fill
        {0x00F67578, true, 0x01164090, 0x01164094, 0x00F6757C},   // PostProc_RenderTxr
        {0x00F6758C, true, 0x01164098, 0x0116409C, 0x00F67590},   // PostProc_RenderAlphaTxr
        {0x00F675A0, true, 0x011640A0, 0x011640A4, 0x00F675A4},   // PostProc_DownSample4x
        {0x00F675AC, true, 0x011640A8, 0x011640AC, 0x00F675B0},   // PostProc_UpSample4x
        {0x00F675C0, true, 0x011640B0, 0x011640B4, 0x00F675C4},   // PostProc_GuassianBlur11H
        {0x00F67624, true, 0x011640B8, 0x011640BC, 0x00F67628},   // PostProc_GuassianBlur11V
        {0x00F67688, true, 0x011640C0, 0x011640C4, 0x00F6768C},   // PostProc_GuassianBlur03H
        {0x00F676AC, true, 0x011640C8, 0x011640CC, 0x00F676B0},   // PostProc_GuassianBlur03V
        {0x00AF22EC, false, 0x011640DC, 0x011640E0, 0x00F676D0},  // PostProc_DownSampleDepth
        {0x00AF20A8, false, 0x01164110, 0x01164114, 0x00F67550},  // PostProc_CopyScale
    };
    load_pairs(ctx, kPairs, 12);
    // Fill is left alone; the 11-tap blurs get 8 and the down-sampler 43.
    static constexpr struct { u32 fragment; u8 min; } kRegisters[] = {
        {0x01164084, 32}, {0x01164094, 32}, {0x0116409C, 32}, {0x011640A4, 43}, {0x011640AC, 8}, {0x011640C4, 32},
        {0x011640CC, 32}, {0x011640B4, 8},  {0x011640BC, 8},  {0x011640E0, 32}, {0x01164114, 32},
    };
    for (const auto& r : kRegisters) min_registers(ctx, r.fragment, r.min);
    call(ctx, pfn::Blur11H_SetOffsets);
    ctx.gpr[3] = call(ctx, pfn::Blur11V_SetOffsets);
}

// void StencilShadow_Init()
// The StencilShadow and StencilShadow_DrawQuad program pairs.
DECOMP(0054BF48, StencilShadow_Init) {
    static constexpr Pair kPairs[] = {
        {0x00AF3554, false, 0x010DF950, 0x010DF954, 0x00F67830},  // StencilShadow
        {0x00AF3564, false, 0x010DF958, 0x010DF95C, 0x00F67838},  // StencilShadow_DrawQuad
    };
    load_pairs(ctx, kPairs, 2);
}

// void PostProc_InitEffects()
// The other post effects' initialisers, then (unless config +158 is
// set) the Sly_PostProc_DarkScreen pair with at least 32 registers.
DECOMP(0051FF1C, PostProc_InitEffects) {
    for (u32 f : {pfn::PostProc_Init546334, pfn::PostProc_Init53D800, pfn::PostProc_Init540824,
                  pfn::PostProc_Init53EC4C, pfn::PostProc_Init53ECE0, pfn::PostProc_Init54588C,
                  pfn::PostProc_Init543774, pfn::PostProc_Init5447F4, pfn::PostProc_Init5420CC})
        call(ctx, f);
    if (field<u8>(kConfig, 158) != 0) return;
    constexpr u32 kDarkV = 0x010F0B50, kDarkF = 0x010F0B54;
    call(ctx, pfn::ProgramPair_Load, {kDarkV, kDarkF, 0x00AF17F0 /* "Sly_PostProc_DarkScreen" */, 0x00F670C0, 0});
    min_registers(ctx, kDarkF, 32);
}
