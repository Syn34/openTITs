// The included UI file resource (vtable 0x00B22678).
#include "tex.h"

#include "../common/common.h"
#include "../uiload/loader.h"
#include "../xml/xml.h"

namespace sly4::font {
using namespace opentit::native;

namespace {
struct Include {
    UiRes res;        // +0
    be_u32 bank;      // +92 Bank_Create's object
};
static_assert(offsetof(Include, bank) == kIncBank);
}  // namespace

void Include_Destruct(UiRes* r) { UiRes_Destruct(r); }

void Include_Delete(UiRes* r) {
    if (!r) return;
    UiRes_Destruct(r);
    common::Mem_FreeFrom(r, 0);
}

// [+12] Parses the file at the resource's path and loads it
// (Ui_LoadFile); 2 on success, 3 otherwise.
u32 Include_Load(UiRes* inc) {
    GuestFrame frame(2384 - 112);
    xml::XmlDoc* xdoc = frame.as<xml::XmlDoc>();
    xml::XmlCursor* doc = &xdoc->self;
    u32 st = 3;
    xml::XmlDoc_Construct(xdoc, inc->path);
    if (xml::XmlDoc_Parse(xdoc, 0) == 0) {
        uiload::XmlCursor_DocPath(doc);
    } else {
        xml::XmlCursor* root = xml::XmlCursor_FirstChild(doc, kTagUi);
        if (root && uiload::Ui_LoadFile(inc, doc, root) != 0)
            st = 2;
        else
            uiload::XmlCursor_DocPath(doc);
    }
    xml::XmlDoc_Destruct(xdoc);
    return st;
}

// [+16] Deletes the timelines and top-level elements the file made, drops
// a user of each resource it declared, and destroys its bank.
void Include_Unload(UiRes* r) {
    Include* inc = reinterpret_cast<Include*>(r);
    const u32 me = guest(inc);
    for (u32 t = global<be_u32>(kUiTimelineList); t != 0;) {
        const u32 next = gref<be_u32>(t + 52);
        if (gref<be_u32>(t + 40) == me) vcall(host<void>(t), 4);
        t = next;
    }
    for (u32 e = global<be_u32>(kUiRootElements); e != 0;) {
        const u32 next = gref<be_u32>(e + 156);
        if (gref<be_u32>(e + 160) == me) vcall(host<void>(e), 4);
        e = next;
    }
    for (UiRes* res = Res_First(); res;) {
        UiRes* next = res->next;
        if (res->arg == me) vcall(res, kVfnUnuse);
        res = next;
    }
    if (inc->bank != 0u) {
        call(tfn2::Bank_Destroy, inc->bank.get());
        inc->bank = 0u;
    }
}

}  // namespace sly4::font

NATIVE(00795724, sly4::font::Include_Destruct)
NATIVE(00795790, sly4::font::Include_Delete)
NATIVE(004DFD04, sly4::font::Include_Load)
NATIVE(004DFDDC, sly4::font::Include_Unload)
