// TimelineDef_KeyFromXml: one key of a <timeline> definition.
#include "../sly4.h"

using namespace sly4;

namespace {

namespace kfn {
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;  // (cursor, hash, f32* out), default in f1
constexpr u32 XmlCursor_DocPath = 0x004F8944;
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 TlKeyEvent_FromXml = 0x004DEF50;  // (doc, node, def; f1 = time) -> key (name?)
constexpr u32 TlKeyAnim_FromXml = 0x004DF0D0;   // (doc, node, def; f1 = time) -> key (name?)
}  // namespace kfn

constexpr u32 kHashTime = 0x03DEC163;      // the time attribute, also the tag of a plain key (name?)
constexpr u32 kHashTime2 = 0x00274ACD;     // read by a plain key, defaulting to the time (name?)
constexpr u32 kTagAnimA = 0x00014042, kTagAnimB = 0x81042B14, kTagEvent = 0xA0CED6D9;
constexpr u32 kTimeDefault = 0x004DF704;   // f32 0.0
constexpr u32 kTlKeyNopVtbl = 0x00B228D8;  // 12 bytes: vtbl, f32 time, next

}  // namespace

// TlKey* TimelineDef_KeyFromXml(XmlCursor* doc, XmlCursor* node, TimelineDef* def)
// By tag: a plain key (12 bytes, vtable 0x00B228D8), or one built by
// 0x004DEF50 / 0x004DF0D0 with the key's time in f1; an unknown tag is
// reported and gives 0.
DECOMP(004DF708, TimelineDef_KeyFromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), def = arg<u32>(ctx, 2);
    Stack frame(ctx, 160 - 112);
    const u32 value = frame.addr();  // sp+112

    const u32 tag = (load<u8>(node + 24) & 0x80) != 0 ? load<u32>(node + 20) : 0;
    ctx.fpr[1] = static_cast<f64>(load<f32>(kTimeDefault));
    call(ctx, kfn::XmlCursor_ReadF32, {node, kHashTime, value});
    const f64 time = static_cast<f64>(load<f32>(value));

    if (tag == kHashTime) {
        ctx.fpr[1] = time;
        call(ctx, kfn::XmlCursor_ReadF32, {node, kHashTime2, value});
        const u32 key = static_cast<u32>(call(ctx, kfn::Mem_Alloc, {12, 0, 16}));
        if (key == 0) return ret(ctx, 0u);
        store<u32>(key + 8, 0);
        store<u32>(key + 4, load<u32>(value));  // lfs / stfs pair
        store<u32>(key, kTlKeyNopVtbl);
        return ret(ctx, key);
    }
    u32 fn = 0;
    if (tag == kTagAnimA || tag == kTagAnimB) fn = kfn::TlKeyAnim_FromXml;
    else if (tag == kTagEvent) fn = kfn::TlKeyEvent_FromXml;
    if (fn == 0) {
        call(ctx, kfn::XmlCursor_DocPath, {doc});
        return ret(ctx, 0u);
    }
    ctx.fpr[1] = time;
    ret(ctx, call(ctx, fn, {doc, node, def}));
}
