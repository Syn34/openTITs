// Start-up loading of the post-processing shader programs. Each program
// pair is looked up by name hash in the vertex / fragment program maps
// and loaded from System\Shaders\<name>_V.vpo / <name>.fpo when missing
// (VProgram_Load / FProgram_Load, which the original inlines here); some
// fragment programs then get a minimum register count.
#include "rsx.h"

namespace sly4::gfx {

namespace {

struct Pair {
    u32 name;  // const char*, or the address it is read from (from_table)
    bool from_table;
    u32 vertex, fragment, bindings;
};

void load_pairs(const Pair* pairs, u32 count) {
    for (u32 i = 0; i < count; ++i) {
        const Pair& p = pairs[i];
        const u32 name = p.from_table ? global<be_u32>(p.name).get() : p.name;
        VProgram_Load(host<gp<VProgram>>(p.vertex), host<const char>(name), 0);
        FProgram_Load(host<gp<FProgram>>(p.fragment), host<const char>(name), host<Binding>(p.bindings));
    }
}

void min_registers(u32 fragment, u8 min) { gfx::min_registers(host<gp<FProgram>>(fragment), min); }

}  // namespace

// void PostProc_Init()
// Once (0x00F67920): the MSAA resolve programs (2x / 4x colour and
// depth) and PostProc_RestoreZCull; the four resolve fragment programs
// get at least 32 registers.
void PostProc_Init() {
    u8& done = global<u8>(0x00F67920);
    if (done != 0) return;
    done = 1;
    static constexpr Pair kPairs[] = {
        {0x00AF3344, false, 0x010F0FF4, 0x010F0FF8, 0},  // PostProc_Resolve2xMSAA
        {0x00AF335C, false, 0x010F0FFC, 0x010F1000, 0},  // PostProc_Resolve4xMSAA
        {0x00AF3374, false, 0x010F1004, 0x010F1008, 0},  // PostProc_ResolveDepth2xMSAA
        {0x00AF3394, false, 0x010F100C, 0x010F1010, 0},  // PostProc_ResolveDepth4xMSAA
        {0x00AF33B4, false, 0x010F1014, 0x010F1018, 0},  // PostProc_RestoreZCull
    };
    load_pairs(kPairs, 5);
    for (u32 f : {0x010F0FF8u, 0x010F1000u, 0x010F1008u, 0x010F1010u}) min_registers(f, 32);
}

// void PostProc_InitCopy()
// PostProc_InitEffects, then the copy / fill / texture / resample / blur
// programs (names and bindings from the effect tables at 0x00F67550..),
// their minimum register counts, and the blur tap offsets.
void PostProc_InitCopy() {
    PostProc_InitEffects();
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
    load_pairs(kPairs, 12);
    // Fill is left alone; the 11-tap blurs get 8 and the down-sampler 43.
    static constexpr struct {
        u32 fragment;
        u8 min;
    } kRegisters[] = {
        {0x01164084, 32}, {0x01164094, 32}, {0x0116409C, 32}, {0x011640A4, 43}, {0x011640AC, 8}, {0x011640C4, 32},
        {0x011640CC, 32}, {0x011640B4, 8},  {0x011640BC, 8},  {0x011640E0, 32}, {0x01164114, 32},
    };
    for (const auto& r : kRegisters) min_registers(r.fragment, r.min);
    Blur11H_SetOffsets();
    Blur11V_SetOffsets();
}

// void StencilShadow_Init()
// The StencilShadow and StencilShadow_DrawQuad program pairs.
void StencilShadow_Init() {
    static constexpr Pair kPairs[] = {
        {0x00AF3554, false, 0x010DF950, 0x010DF954, 0x00F67830},  // StencilShadow
        {0x00AF3564, false, 0x010DF958, 0x010DF95C, 0x00F67838},  // StencilShadow_DrawQuad
    };
    load_pairs(kPairs, 2);
}

// void PostProc_InitEffects()
// The other post effects' initialisers, then (unless config +158 is
// set) the Sly_PostProc_DarkScreen pair with at least 32 registers.
void PostProc_InitEffects() {
    for (u32 f : {0x00546334u, 0x0053D800u, 0x00540824u, 0x0053EC4Cu, 0x0053ECE0u, 0x0054588Cu, 0x00543774u,
                  0x005447F4u, 0x005420CCu})
        call(f);
    if (config_u8(158) != 0) return;
    constexpr u32 kDarkV = 0x010F0B50, kDarkF = 0x010F0B54;
    ProgramPair_Load(host<gp<VProgram>>(kDarkV), host<gp<FProgram>>(kDarkF),
                     host<const char>(0x00AF17F0) /* "Sly_PostProc_DarkScreen" */, host<Binding>(0x00F670C0), 0);
    min_registers(kDarkF, 32);
}

}  // namespace sly4::gfx

NATIVE(00534310, sly4::gfx::PostProc_Init)
NATIVE(00546F6C, sly4::gfx::PostProc_InitCopy)
NATIVE(0054BF48, sly4::gfx::StencilShadow_Init)
NATIVE(0051FF1C, sly4::gfx::PostProc_InitEffects)
