// Cg program objects: VProgram_Create / FProgram_Create copy a loaded Cg
// binary's ucode into RSX-visible memory, FProgram_Setup resolves the
// named constant bindings. See post.h for the layouts.
#include "post.h"

using namespace sly4;

namespace {

namespace pfn = sly4::post::fn;
namespace pg = sly4::post::g;

constexpr u32 kContextCallback = 0x00F3BCE8;  // OPD: command-buffer callback for the vertex program build
constexpr u32 kConstantFill = 0x00533BB8;     // f32 999999.0, the default of every constant

// Resolves each {name, constant index} entry of a 0-terminated list.
void bind_list(PPUContext& ctx, u32 out, u32 e) {
    do {
        const u32 p = load<u32>(out);
        const u32 rec = p + 592 + (load<u32>(e + 4) << 4);
        const u32 cg = load<u32>(p + 4);
        store<u32>(rec + 8, e);
        const u32 param = static_cast<u32>(call(ctx, pfn::Cg_GetNamedParameter, {cg, load<u32>(e)}));
        store<u32>(rec, param);
        if (param != 0) {
            const s32 resource = static_cast<s32>(call(ctx, pfn::Cg_GetParameterResource, {cg, param}));
            store<s32>(rec + 4, resource);
            const u32 again = load<u32>(rec);
            if (resource == -1) {
                store<u32>(rec, 0);
            } else if (again != 0 &&
                       static_cast<u32>(call(ctx, pfn::Cg_IsParameterReferenced,
                                             {load<u32>(load<u32>(out) + 4), again})) == 0) {
                store<u32>(rec, 0);
            }
        }
        e += 8;
    } while (load<u32>(e) != 0);
}

}  // namespace

// void VProgram_Create(VProgram** out, CgProgram* cg, u32 unused)
// A 24-byte VProgram whose ucode copy (pool 5, 128-aligned size + 128)
// is pre-built into a vertex-program command block.
DECOMP(00533EE4, VProgram_Create) {
    const u32 out = arg<u32>(ctx, 0), cg = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, fn::Mem_Alloc, {24, 0, 16}));
    if (p != 0) {
        const u32 w12 = load<u32>(p + 12);
        store<u32>(p, 0);
        store<u32>(p + 4, 0);
        store<u16>(p + 8, 0);
        store<u16>(p + 10, 0);
        store<u32>(p + 16, 0);
        store<u32>(p + 20, 0);
        store<u32>(p + 12, (w12 & 1) | 1);
    }
    store<u32>(out, p);
    call(ctx, pfn::Cg_InitProgram, {cg});
    call(ctx, pfn::Cg_Sub5E5F28, {cg});
    store<u32>(load<u32>(out) + 4, cg);
    Stack frame(ctx, 32);  // +0 ucode size, +8 CellGcmContextData
    const u32 size_out = frame.addr(), context = frame.addr() + 8;
    call(ctx, pfn::Cg_GetUCode, {cg, load<u32>(out), size_out});  // the ucode pointer lands in VProgram +0
    const u32 q = load<u32>(out);
    store<u16>(q + 8, static_cast<u16>(load<u32>(size_out)));
    store<u32>(q + 12, (load<u32>(q + 12) & 1) | 1);
    const u32 bytes = (load<u32>(size_out) + 255) & ~0x7Fu;
    store<u32>(load<u32>(out) + 16, static_cast<u32>(call(ctx, pfn::Mem_AllocPool, {16, bytes, 5, 7})));
    call(ctx, pfn::memset, {load<u32>(load<u32>(out) + 16), 0, bytes});
    call(ctx, post::imp::cellGcmAddressToOffset, {load<u32>(load<u32>(out) + 16), load<u32>(out) + 20});
    call(ctx, pfn::Gcm_SetupContext, {context, load<u32>(load<u32>(out) + 16), bytes, kContextCallback});
    const u32 q2 = load<u32>(out);
    call(ctx, pfn::Gcm_SetVertexProgram, {context, load<u32>(q2 + 4), load<u32>(q2)});
    ctx.gpr[3] = call(ctx, pfn::Gcm_FinishContext, {context});
}

// FProgram* FProgram_Create(FProgram** out, CgProgram* cg, u32 unused)
// A 1168-byte FProgram: constants filled with 999999.0, 34 empty
// parameter records, and the ucode copied to pool 5 (128-aligned).
DECOMP(00533BC0, FProgram_Create) {
    const u32 out = arg<u32>(ctx, 0), cg = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, fn::Mem_Alloc, {1168, 0, 16}));
    if (p != 0) {
        const u32 w12 = load<u32>(p + 12);
        store<u32>(p, 0);
        store<u32>(p + 4, 0);
        store<u16>(p + 8, 0);
        store<u16>(p + 10, 0);
        store<u64>(p + 16, 0);
        store<u32>(p + 24, 0);
        store<u32>(p + 12, (w12 & 1) | 1);
        store<u32>(p + 28, 0);
        store<u32>(p + 32, 0);
        store<u32>(p + 36, 0);
        for (u32 k = 0; k < 17; ++k) {
            for (u32 r : {p + 592 + 32 * k, p + 608 + 32 * k}) {
                store<u32>(r, 0);
                store<u32>(r + 4, 0);
                store<u32>(r + 8, 0);
                store<s32>(r + 12, -1);
            }
        }
        const u32 fill = load<u32>(kConstantFill);
        for (u32 v = 0; v < 34; ++v) {
            const u32 a = (p + 48 + 16 * v) & ~15u;  // stvx
            for (u32 w = 0; w < 16; w += 4) store<u32>(a + w, fill);
        }
    }
    store<u32>(out, p);
    call(ctx, pfn::Cg_InitProgram, {cg});
    call(ctx, pfn::Cg_Sub5E5F28, {cg});
    store<u32>(load<u32>(out) + 4, cg);
    Stack frame(ctx, 16);  // +0 ucode size, +4 ucode pointer, +8 RSX offset
    const u32 size_out = frame.addr(), ucode_out = frame.addr() + 4, offset_out = frame.addr() + 8;
    call(ctx, pfn::Cg_GetUCode, {cg, ucode_out, size_out});
    store<u16>(load<u32>(out) + 8, static_cast<u16>(load<u32>(size_out)));
    const u32 copy = static_cast<u32>(call(ctx, pfn::Mem_AllocPool, {128, load<u16>(load<u32>(out) + 8), 5, 5}));
    call(ctx, pfn::memmove, {copy, load<u32>(ucode_out), load<u16>(load<u32>(out) + 8)});
    store<u32>(load<u32>(out), copy);
    store<u32>(offset_out, 0);
    call(ctx, post::imp::cellGcmAddressToOffset, {copy, offset_out});
    const u32 w12 = load<u32>(out) + 12;
    store<u32>(w12, (load<u32>(offset_out) << 1) | (load<u32>(w12) & 1) | 1);
    call(ctx, pfn::Gcm_SetFragmentProgramLoad, {cg, load<u32>(out) + 1136, 0, 1, 0});
    store<u32>(load<u32>(out) + 1136, load<u32>(offset_out));
    ctx.gpr[3] = load<u32>(out);
}

// void FProgram_Setup(FProgram** out, const Binding* extra)
// Looks up the default bindings and then `extra` (if any) by name, keeps
// the referenced ones, numbers the used records' constants from 0 (+28
// counts them) and calls FProgram_Finish.
DECOMP(00533978, FProgram_Setup) {
    const u32 out = arg<u32>(ctx, 0), extra = arg<u32>(ctx, 1);
    if (load<u32>(pg::default_bindings) != 0) bind_list(ctx, out, pg::default_bindings);
    if (extra != 0 && load<u32>(extra) != 0) bind_list(ctx, out, extra);
    store<u32>(load<u32>(out) + 28, 0);
    u32 rec = load<u32>(out) + 592;
    for (u32 i = 0; i < 34; ++i, rec += 16) {
        if (load<u32>(rec) == 0) continue;
        const u32 counter = load<u32>(out) + 28;
        const u32 n = load<u32>(counter);
        store<u32>(counter, n + 1);
        store<u32>(rec + 12, n);
    }
    ctx.gpr[3] = call(ctx, pfn::FProgram_Finish, {out});
}
