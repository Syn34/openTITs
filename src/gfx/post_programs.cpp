// Cg program objects: VProgram_Create / FProgram_Create copy a loaded Cg
// binary's ucode into RSX-visible memory, FProgram_Setup resolves the
// named constant bindings. See post.h for the layouts.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 kContextCallback = 0x00F3BCE8;  // OPD: command-buffer callback for the vertex program build
constexpr u32 kConstantFill = 0x00533BB8;     // f32 999999.0, the default of every constant
constexpr u32 kDefaultBindings = 0x00F6748C;  // Binding list ("gvLightData[0]", ...)
constexpr u32 Mem_AllocPool = 0x0056C514;     // (align, size, pool, flags)
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 Cg_InitProgram = 0x005DFAF4;       // (CgProgram*) (name?)
constexpr u32 Cg_Sub5E5F28 = 0x005E5F28;         // (CgProgram*) (unknown)
constexpr u32 Cg_GetUCode = 0x005E5E28;          // (CgProgram*, void** ucode, u32* size) (name?)
constexpr u32 Cg_GetNamedParameter = 0x005E5E54; // (CgProgram*, const char*) -> CgParameter (name?)
constexpr u32 Cg_GetParameterResource = 0x005E5F20;   // (CgProgram*, CgParameter) -> s32, -1 none (name?)
constexpr u32 Cg_IsParameterReferenced = 0x005E60DC;  // (CgProgram*, CgParameter) -> bool (name?)
constexpr u32 Gcm_SetupContext = 0x005DF960;     // (CellGcmContextData*, begin, size, callback OPD) (name?)
constexpr u32 Gcm_SetVertexProgram = 0x005E5914; // (context, CgProgram*, ucode) (name?)
constexpr u32 Gcm_FinishContext = 0x005E3C1C;    // (context) (name?)
constexpr u32 Gcm_SetFragmentProgramLoad = 0x005DF980;  // (CgProgram*, out, 0, 1, 0) (name?)

// Resolves each {name, constant index} entry of a 0-terminated list.
void bind_list(gp<FProgram>* out, const Binding* e) {
    do {
        FProgram* p = or_zero_page(out->get());
        FParam* rec = at<FParam>(p, 592 + (e->index << 4));
        const u32 cg = p->cg;
        rec->entry = guest(e);
        const u32 param = call<u32>(Cg_GetNamedParameter, cg, e->name.ea.get());
        rec->param = param;
        if (param != 0) {
            const s32 resource = call<s32>(Cg_GetParameterResource, cg, param);
            rec->resource = static_cast<u32>(resource);
            const u32 again = rec->param;
            if (resource == -1) {
                rec->param = 0u;
            } else if (again != 0 && call<u32>(Cg_IsParameterReferenced, or_zero_page(out->get())->cg.get(), again) == 0) {
                rec->param = 0u;
            }
        }
        ++e;
    } while (e->name);
}

}  // namespace

// void VProgram_Create(VProgram** out, CgProgram* cg, u32 unused)
// A 24-byte VProgram whose ucode copy (pool 5, 128-aligned size + 128)
// is pre-built into a vertex-program command block. A failed allocation
// leaves *out null and the writes below go through it, as in the original.
void VProgram_Create(gp<VProgram>* out, u32 cg, u32) {
    VProgram* p = call<VProgram*>(sly4::fn::Mem_Alloc, 24u, 0u, 16u);
    if (p) {
        const u32 w12 = p->flags;
        p->ucode = 0u;
        p->cg = 0u;
        p->ucode_size = 0;
        p->attr_mask = 0;
        p->ucode_copy = 0u;
        p->offset = 0u;
        p->flags = (w12 & 1) | 1;
    }
    *out = p;
    auto vp = [&] { return or_zero_page(out->get()); };
    call(Cg_InitProgram, cg);
    call(Cg_Sub5E5F28, cg);
    vp()->cg = cg;
    GuestFrame f(32);  // +0 ucode size, +8 CellGcmContextData
    be_u32* size_out = f.as<be_u32>(0);
    GcmContext* context = f.as<GcmContext>(8);
    call(Cg_GetUCode, cg, &vp()->ucode, size_out);  // the ucode pointer lands in VProgram +0
    VProgram* q = vp();
    q->ucode_size = static_cast<u16>(size_out->get());
    q->flags = (q->flags & 1) | 1;
    const u32 bytes = (*size_out + 255) & ~0x7Fu;
    vp()->ucode_copy = call<u32>(Mem_AllocPool, 16u, bytes, 5u, 7u);
    call(sly4::fn::memset, vp()->ucode_copy.get(), 0u, bytes);
    call(fn::cellGcmAddressToOffset, vp()->ucode_copy.get(), &vp()->offset);
    call(Gcm_SetupContext, context, vp()->ucode_copy.get(), bytes, kContextCallback);
    VProgram* q2 = vp();
    call(Gcm_SetVertexProgram, context, q2->cg.get(), q2->ucode.get());
    call(Gcm_FinishContext, context);
}

// FProgram* FProgram_Create(FProgram** out, CgProgram* cg, u32 unused)
// A 1168-byte FProgram: constants filled with 999999.0, 34 empty
// parameter records, and the ucode copied to pool 5 (128-aligned).
FProgram* FProgram_Create(gp<FProgram>* out, u32 cg, u32) {
    FProgram* p = call<FProgram*>(sly4::fn::Mem_Alloc, 1168u, 0u, 16u);
    if (p) {
        const u32 w12 = p->offset_loc;
        p->ucode = 0u;
        p->cg = 0u;
        p->ucode_size = 0;
        *at<be_u16>(p, 10) = 0;
        p->dirty = 0;
        p->stamp = 0u;
        p->offset_loc = (w12 & 1) | 1;
        p->used = 0u;
        *at<be_u32>(p, 32) = 0u;
        p->patch_table = 0u;
        for (u32 k = 0; k < 34; ++k) {
            FParam& r = p->params[k];
            r.param = 0u;
            r.resource = 0u;
            r.entry = 0u;
            r.index = 0xFFFFFFFFu;
        }
        const u32 fill = global<be_u32>(kConstantFill);
        for (u32 v = 0; v < 34; ++v) {
            be_u32* a = host<be_u32>((guest(p) + 48 + 16 * v) & ~15u);  // stvx
            for (u32 w = 0; w < 4; ++w) a[w] = fill;
        }
    }
    *out = p;
    auto fp = [&] { return or_zero_page(out->get()); };
    call(Cg_InitProgram, cg);
    call(Cg_Sub5E5F28, cg);
    fp()->cg = cg;
    GuestFrame f(16);  // +0 ucode size, +4 ucode pointer, +8 RSX offset
    be_u32* size_out = f.as<be_u32>(0);
    be_u32* ucode_out = f.as<be_u32>(4);
    be_u32* offset_out = f.as<be_u32>(8);
    call(Cg_GetUCode, cg, ucode_out, size_out);
    fp()->ucode_size = static_cast<u16>(size_out->get());
    const u32 copy = call<u32>(Mem_AllocPool, 128u, static_cast<u32>(fp()->ucode_size), 5u, 5u);
    call(memmove, copy, ucode_out->get(), static_cast<u32>(fp()->ucode_size));
    fp()->ucode = copy;
    *offset_out = 0u;
    call(fn::cellGcmAddressToOffset, copy, offset_out);
    be_u32& w12 = fp()->offset_loc;
    w12 = (*offset_out << 1) | (w12 & 1) | 1;
    call(Gcm_SetFragmentProgramLoad, cg, &fp()->program, 0u, 1u, 0u);
    *at<be_u32>(fp(), 1136) = offset_out->get();
    return out->get();
}

// void FProgram_Setup(FProgram** out, const Binding* extra)
// Looks up the default bindings and then `extra` (if any) by name, keeps
// the referenced ones, numbers the used records' constants from 0 (+28
// counts them) and calls FProgram_Finish.
void FProgram_Setup(gp<FProgram>* out, const Binding* extra) {
    const Binding* defaults = host<Binding>(kDefaultBindings);
    if (defaults->name) bind_list(out, defaults);
    if (extra && extra->name) bind_list(out, extra);
    or_zero_page(out->get())->used = 0u;
    FParam* rec = or_zero_page(out->get())->params;
    for (u32 i = 0; i < 34; ++i, ++rec) {
        if (rec->param == 0) continue;
        be_u32& counter = or_zero_page(out->get())->used;
        const u32 n = counter;
        counter = n + 1;
        rec->index = n;
    }
    FProgram_Finish(out);
}

}  // namespace sly4::gfx

NATIVE(00533EE4, sly4::gfx::VProgram_Create)
NATIVE(00533BC0, sly4::gfx::FProgram_Create)
NATIVE(00533978, sly4::gfx::FProgram_Setup)
