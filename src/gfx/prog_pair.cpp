// Out-of-line program loaders and FProgram_Finish (the embedded-constant
// patch table of a fragment program).
#include "prog_load.h"

using namespace sly4;

namespace {

namespace lfn = sly4::progload::fn;

// Parameter types whose embedded constants get patched (Cg type ids).
bool patched_type(s32 type) {
    return (type >= 1045 && type <= 1048) || type == 1091 || (type >= 1114 && type <= 1118);
}

}  // namespace

// void ProgramPair_Load(VProgram** v, FProgram** f, const char* name, const Binding* bindings, u32 unused)
DECOMP(00534160, ProgramPair_Load) {
    const u32 v = arg<u32>(ctx, 0), f = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), bindings = arg<u32>(ctx, 3);
    progload::load_vertex(ctx, v, name);
    ctx.gpr[3] = progload::load_fragment(ctx, f, name, bindings);
}

// void VProgram_Load(VProgram** v, const char* name, u32 unused)
DECOMP(00534074, VProgram_Load) {
    ctx.gpr[3] = progload::load_vertex(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1));
}

// void FProgram_Load(FProgram** f, const char* name, const Binding* bindings)
DECOMP(00533DF8, FProgram_Load) {
    ctx.gpr[3] = progload::load_fragment(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2));
}

// void FProgram_Finish(FProgram** out)
// Drops parameter records with no embedded constants and builds the patch
// table: for each kept record of a patchable type, its count followed by
// its constant offsets; 0-terminated, copied to a Mem_Alloc block (+36,
// size in bytes at +32). An unexpected parameter type stops the build
// before the table is made.
DECOMP(0053378C, FProgram_Finish) {
    const u32 out = arg<u32>(ctx, 0);
    Stack frame(ctx, 1024);  // the table, built on the stack as the original does
    const u32 table = frame.addr();
    u32 n = 0;
    u32 rec = load<u32>(out) + 592;
    for (u32 i = 0; i < 34; ++i, rec += 16) {
        const u32 param = load<u32>(rec);
        if (param == 0) continue;
        const u32 count = static_cast<u32>(
            call(ctx, lfn::Cg_GetEmbeddedConstantCount, {load<u32>(load<u32>(out) + 4), param}));
        if (count == 0) {
            store<u32>(rec, 0);
            continue;
        }
        const s32 type = static_cast<s32>(
            call(ctx, lfn::Cg_GetParameterType, {load<u32>(load<u32>(out) + 4), load<u32>(rec)}));
        if (!patched_type(type)) {
            ctx.gpr[3] = static_cast<u64>(static_cast<s64>(type));
            return;
        }
        store<u32>(table + 4 * n++, count);
        for (u32 j = 0; j < count; ++j) {
            const u32 offset = static_cast<u32>(call(ctx, lfn::Cg_GetEmbeddedConstantOffset,
                                                     {load<u32>(load<u32>(out) + 4), load<u32>(rec), j}));
            store<u32>(table + 4 * n++, offset);
        }
    }
    store<u32>(table + 4 * n, 0);
    const u32 p = load<u32>(out);
    store<u32>(p + 32, (n + 1) * 4);
    store<u32>(load<u32>(out) + 36, static_cast<u32>(call(ctx, fn::Mem_Alloc, {load<u32>(p + 32), 0, 16})));
    const u32 q = load<u32>(out);
    ctx.gpr[3] = call(ctx, post::fn::memmove, {load<u32>(q + 36), table, load<u32>(q + 32)});
}
