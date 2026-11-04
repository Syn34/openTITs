// The included UI file resource (vtable 0x00B22678).
#include "tex.h"

using namespace sly4;
using namespace sly4::font;

namespace {

// The UiRes base destructor as these classes inline it, leaving r4..r6
// as the original does (r4 = the base vtable, r5 = the previous entry,
// r6 = 0x00F60000).
void ResTeardown(PPUContext& ctx, u32 r) {
    store<u32>(r, kResVtbl);
    Res_Unlink(ctx, r);
    const u64 prev = ctx.gpr[4];
    store<u32>(r, kRefVtbl);
    ctx.gpr[4] = kRefVtbl;
    ctx.gpr[5] = prev;
    ctx.gpr[6] = 0x00F60000;
}

}  // namespace

DECOMP(00795724, Include_Destruct) { ResTeardown(ctx, arg<u32>(ctx, 0)); }

DECOMP(00795790, Include_Delete) {
    const u32 r = arg<u32>(ctx, 0);
    if (r == 0) return;
    ResTeardown(ctx, r);
    call(ctx, rfn::Mem_FreeFrom, {r, 0, r, kRefVtbl});
}

// u16 Include_Load(Include* inc)  [+12]
// Parses the file at the resource's path and loads it (Ui_LoadFile);
// 2 on success, 3 otherwise.
DECOMP(004DFD04, Include_Load) {
    const u32 inc = arg<u32>(ctx, 0);
    Stack frame(ctx, 2384 - 112);
    const u32 doc = frame.addr();
    u32 st = 3;
    call(ctx, tfn2::XmlDoc_Construct, {doc, inc + kResPath});
    if (static_cast<u32>(call(ctx, tfn2::XmlDoc_Parse, {doc, 0})) == 0) {
        call(ctx, lfn::XmlCursor_DocPath, {doc});
    } else {
        const u32 root = static_cast<u32>(call(ctx, lfn::XmlCursor_FirstChild, {doc, kTagUi}));
        if (root != 0 && static_cast<u32>(call(ctx, tfn2::Ui_LoadFile, {inc, doc, root})) != 0)
            st = 2;
        else
            call(ctx, lfn::XmlCursor_DocPath, {doc});
    }
    call(ctx, tfn2::XmlDoc_Destruct, {doc});
    ret(ctx, st);
}

// void Include_Unload(Include* inc)  [+16]
// Deletes the timelines and top-level elements the file made, drops a
// user of each resource it declared, and destroys its bank.
DECOMP(004DFDDC, Include_Unload) {
    const u32 inc = arg<u32>(ctx, 0);
    for (u32 t = load<u32>(kUiTimelineList); t != 0;) {
        const u32 next = load<u32>(t + 52);
        if (load<u32>(t + 40) == inc) {
            const u32 opd = load<u32>(load<u32>(t) + 4);
            call_ptr(ctx, opd, {t, opd, load<u32>(opd)});
        }
        t = next;
    }
    for (u32 e = load<u32>(kUiRootElements); e != 0;) {
        const u32 next = load<u32>(e + 156);
        if (load<u32>(e + 160) == inc) {
            const u32 opd = load<u32>(load<u32>(e) + 4);
            call_ptr(ctx, opd, {e, opd, load<u32>(opd)});
        }
        e = next;
    }
    for (u32 r = load<u32>(kResList); r != 0;) {
        const u32 next = load<u32>(r + kResNext);
        if (load<u32>(r + kResArg) == inc) {
            const u32 opd = load<u32>(load<u32>(r) + kVfnUnuse);
            call_ptr(ctx, opd, {r, opd, load<u32>(opd)});
        }
        r = next;
    }
    if (load<u32>(inc + kIncBank) != 0) {
        call(ctx, tfn2::Bank_Destroy, {load<u32>(inc + kIncBank)});
        store<u32>(inc + kIncBank, 0);
    }
}
