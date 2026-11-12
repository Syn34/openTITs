// Loading Cg programs by name (VProgram_Load / FProgram_Load and the
// pair) and FProgram_Finish (the embedded-constant patch table of a
// fragment program). The effect initialisers inline the same loading code.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 kVMap = 0x010F1350;    // ProgramMap of VProgram* by name hash
constexpr u32 kFMap = 0x010F1368;    // ProgramMap of FProgram* by name hash
constexpr u32 kFmtFpo = 0x00AF3314;  // "System\Shaders\%s.fpo"
constexpr u32 kFmtVpo = 0x00AF332C;  // "System\Shaders\%s_V.vpo"
constexpr u32 String_Printf = 0x00551540;  // sprintf
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 Cg_GetAttributeMask = 0x005E5FF0;          // (CgProgram*) -> u16 (name?)
constexpr u32 Cg_GetParameterType = 0x005E5F10;          // (CgProgram*, CgParameter) -> s32 (name?)
constexpr u32 Cg_GetEmbeddedConstantCount = 0x005E60DC;  // (CgProgram*, CgParameter) -> u32 (name?)
constexpr u32 Cg_GetEmbeddedConstantOffset = 0x005E610C; // (CgProgram*, CgParameter, u32 i) -> u32 (name?)

// Parameter types whose embedded constants get patched (Cg type ids).
bool patched_type(s32 type) {
    return (type >= 1045 && type <= 1048) || type == 1091 || (type >= 1114 && type <= 1118);
}

// A 256-byte path, the key and the file size, as each loader has.
struct LoadFrame {
    char path[256];  // +0
    be_u32 key;      // +256
    be_u32 size;     // +260
};

}  // namespace

// void VProgram_Load(VProgram** v, const char* name, u32 unused)
// *v = the vertex program `name`: the cached one, else loaded from
// System\Shaders\<name>_V.vpo, created and registered.
void VProgram_Load(gp<VProgram>* v, const char* name, u32) {
    GuestFrame frame(272);
    LoadFrame* f = frame.as<LoadFrame>();
    f->key = call<u32>(sly4::fn::String_HashNoCase, name);
    VProgram* found = *VMap_Find(host<ProgramMap>(kVMap), &f->key);
    *v = found;
    if (found) return;
    call(String_Printf, f->path, kFmtVpo, name);
    f->size = 0u;
    const u32 data = call<u32>(sly4::fn::File_LoadWhole, f->path, &f->size);
    VProgram_Create(v, data, 0);
    VMap_Insert(host<ProgramMap>(kVMap), &f->key, *v);
    const u32 mask = call<u32>(Cg_GetAttributeMask, or_zero_page(v->get())->cg.get());
    or_zero_page(v->get())->attr_mask = static_cast<u16>(mask);
}

// void FProgram_Load(FProgram** f, const char* name, const Binding* bindings)
// *f = the fragment program `name`, bound with `bindings` when it is loaded.
void FProgram_Load(gp<FProgram>* out, const char* name, const Binding* bindings) {
    GuestFrame frame(272);
    LoadFrame* f = frame.as<LoadFrame>();
    f->key = call<u32>(sly4::fn::String_HashNoCase, name);
    FProgram* found = *FMap_Find(host<ProgramMap>(kFMap), &f->key);
    *out = found;
    if (found) return;
    call(String_Printf, f->path, kFmtFpo, name);
    f->size = 0u;
    const u32 data = call<u32>(sly4::fn::File_LoadWhole, f->path, &f->size);
    FProgram_Create(out, data, f->size);
    FMap_Insert(host<ProgramMap>(kFMap), &f->key, *out);
    FProgram_Setup(out, bindings);
}

// void ProgramPair_Load(VProgram** v, FProgram** f, const char* name, const Binding* bindings, u32 unused)
void ProgramPair_Load(gp<VProgram>* v, gp<FProgram>* f, const char* name, const Binding* bindings, u32) {
    VProgram_Load(v, name, 0);
    FProgram_Load(f, name, bindings);
}

// void FProgram_Finish(FProgram** out)
// Drops parameter records with no embedded constants and builds the patch
// table: for each kept record of a patchable type, its count followed by
// its constant offsets; 0-terminated, copied to a Mem_Alloc block (+36,
// size in bytes at +32). An unexpected parameter type stops the build
// before the table is made.
void FProgram_Finish(gp<FProgram>* out) {
    GuestFrame frame(1024);  // the table, built on the stack as the original does
    be_u32* table = frame.as<be_u32>();
    u32 n = 0;
    FParam* rec = or_zero_page(out->get())->params;
    for (u32 i = 0; i < 34; ++i, ++rec) {
        const u32 param = rec->param;
        if (param == 0) continue;
        const u32 count = call<u32>(Cg_GetEmbeddedConstantCount, or_zero_page(out->get())->cg.get(), param);
        if (count == 0) {
            rec->param = 0u;
            continue;
        }
        const s32 type = call<s32>(Cg_GetParameterType, or_zero_page(out->get())->cg.get(), rec->param.get());
        if (!patched_type(type)) return;
        table[n++] = count;
        for (u32 j = 0; j < count; ++j) table[n++] = call<u32>(Cg_GetEmbeddedConstantOffset, or_zero_page(out->get())->cg.get(), rec->param.get(), j);
    }
    table[n] = 0u;
    FProgram* p = or_zero_page(out->get());
    *at<be_u32>(p, 32) = (n + 1) * 4;
    or_zero_page(out->get())->patch_table = call<u32>(sly4::fn::Mem_Alloc, at<be_u32>(p, 32)->get(), 0u, 16u);
    FProgram* q = or_zero_page(out->get());
    call(memmove, q->patch_table.get(), table, at<be_u32>(q, 32)->get());
}

}  // namespace sly4::gfx

NATIVE(00534160, sly4::gfx::ProgramPair_Load)
NATIVE(00534074, sly4::gfx::VProgram_Load)
NATIVE(00533DF8, sly4::gfx::FProgram_Load)
NATIVE(0053378C, sly4::gfx::FProgram_Finish)
