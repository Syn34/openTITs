// TimelineDef_KeyFromXml: one key of a <timeline> definition.
#include "tlbuild.h"

#include "../memory/mem.h"
#include "uiload.h"

namespace sly4::tlbuild {
using namespace opentit::native;

namespace {
constexpr u32 kHashTime = 0x03DEC163;      // the time attribute, also the tag of a plain key (name?)
constexpr u32 kHashTime2 = 0x00274ACD;     // read by a plain key, defaulting to the time (name?)
constexpr u32 kTagAnimB = 0x81042B14, kTagEvent = 0xA0CED6D9;
constexpr u32 kTimeDefault = 0x004DF704;   // f32 0.0
constexpr u32 kTlKeyNopVtbl = 0x00B228D8;  // 12 bytes: vtbl, f32 time, next

struct TlKeyNop {
    be_u32 vtable;  // +0
    be_u32 time;    // +4 (f32 bits)
    be_u32 next;    // +8
};
}  // namespace

// By tag: a plain key (12 bytes, vtable 0x00B228D8), or one built by
// TlKeyEvent_FromXml / TlKeyAnim_FromXml with the key's time; an unknown
// tag is reported and gives 0.
void* TimelineDef_KeyFromXml(xml::XmlCursor* doc, xml::XmlCursor* node, void* def) {
    GuestFrame frame(16);
    be_f32* value = frame.as<be_f32>();

    const u32 tag = (node->flags & 0x80) != 0 ? node->hash.get() : 0u;
    xml::XmlCursor_ReadF32(node, kHashTime, value, gref<be_f32>(kTimeDefault));
    const f32 time = *value;

    if (tag == kHashTime) {
        xml::XmlCursor_ReadF32(node, kHashTime2, value, time);
        TlKeyNop* key = static_cast<TlKeyNop*>(memory::Mem_Alloc(12, 0, 16));
        if (!key) return nullptr;
        key->next = 0u;
        key->time = reinterpret_cast<be_u32*>(value)->get();  // lfs / stfs pair
        key->vtable = kTlKeyNopVtbl;
        return key;
    }
    if (tag == kTagAnimA || tag == kTagAnimB) return TlKeyAnim_FromXml(doc, node, def, time);
    if (tag == kTagEvent) return TlKeyEvent_FromXml(doc, node, def, time);
    uiload::XmlCursor_DocPath(doc);
    return nullptr;
}

}  // namespace sly4::tlbuild

NATIVE(004DF708, sly4::tlbuild::TimelineDef_KeyFromXml)
