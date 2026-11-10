// Game code, medium sweep part 4: component attachment through the
// template registry (as hevent21_f.cpp's Obj_AttachControlAI128EE0): a
// template on the stack {+0 vtable, +4 name hash, +8 owner, +12 kind, +16
// u8 0, +20 arg} is pushed onto the registry 0x01056F04 while
// Obj_Attach0D135C(o, tmpl, 0) builds the component, then removed again
// (SortedMap_Remove7330C0(registry, &tmpl->hash)).
#include "mid4.h"

using namespace sly4;
using namespace sly4::mid4;

namespace {

namespace tfn {
constexpr u32 kRegistry = 0x01056F04;         // {+0 u16 capacity, +2 u16 count, +4 u16 high-water, +6 u16 flags, +8 {hash, Tmpl*}[]}
constexpr u32 Array_Reserve = 0x00558F3C;     // (Array*, u16 capacity)
constexpr u32 SortedMap_Remove = 0x007330C0;  // (map, const u32* key)
constexpr u32 Obj_Attach = 0x000D135C;        // (o, tmpl, 0) -> component
constexpr u32 kTmplVtBuild = 0x00B0DAAC;      // the template base vtable
constexpr u32 kTmplVtControl = 0x00B1B990;    // "TmpControl"
constexpr u32 kTmplVtExtControl = 0x00B1B9A0; // "TmpExtControl"
constexpr u32 kStrTmpControl = 0x00AD3CB8;    // "TmpControl"
constexpr u32 kStrTmpExtControl = 0x00AD3CC8; // "TmpExtControl"
constexpr u32 kCurrentWorld = 0x00F65CA0;     // World*
}  // namespace tfn

// The template t initialized (base vtable, the hash of name, o, kind) and
// pushed onto the registry (grown to twice its capacity when full).
void TmplPush(PPUContext& ctx, u32 t, u32 o, u32 name, u32 kind) {
    store<u32>(t, tfn::kTmplVtBuild);
    const u32 h = static_cast<u32>(call(ctx, m4fn::String_HashNoCase, {name}));
    store<u32>(t + 8, o);
    store<u8>(t + 16, 0);
    store<u32>(t + 4, h);
    const u32 arr = tfn::kRegistry;
    u32 n = load<u16>(arr + 2);
    const u32 cap = load<u16>(arr);
    store<u32>(t + 12, kind);
    if (n == cap) {
        call(ctx, tfn::Array_Reserve, {arr, (cap << 1) & 0xFFFEu});
        n = load<u16>(arr + 2);
    }
    const u32 data = load<u32>(arr + 8);
    store<u32>(data + ((n << 3) & 0x7FFF8u), load<u32>(t + 4));
    store<u32>(data + (static_cast<u32>(load<u16>(arr + 2)) << 3) + 4, t);
    const u32 n2 = load<u16>(arr + 2) + 1;
    const u32 hi = load<u16>(arr + 4), af = load<u16>(arr + 6);
    store<u16>(arr + 2, static_cast<u16>(n2));
    store<u16>(arr + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n2 & 0xFFFF) ? hi : n2));
    store<u16>(arr + 6, static_cast<u16>(af | 1));
}

// void f(Obj* o): when o's +188 bit 0 is set, attaches its "TmpControl"
// component (kind 9, arg the byte +1 of o's world slot (World + 4208 +
// 0x20C80 * s8 o->+172)) to +cur and calls its vfn[5](c, o); then +prev =
// +cur and the "TmpExtControl" component (kind 10) goes to +ext (and to
// +prev when that is empty). Returns SortedMap_Remove's result. The two
// templates share one frame: the second's +20 is the first's vtable word
// (left unwritten, as the original).
void AttachControls(PPUContext& ctx, u32 prev, u32 cur, u32 ext) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 slot = load<u32>(tfn::kCurrentWorld) + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(o + 172)))) * 0x20C80 + 4208;
    Stack frame(ctx, 48);
    const u32 t2 = frame.addr(), t1 = t2 + 20;
    if ((load<u32>(o + 188) & 1) != 0) {
        const u32 arg1 = load<u8>(slot + 1);
        TmplPush(ctx, t1, o, tfn::kStrTmpControl, 9);
        store<u32>(t1, tfn::kTmplVtControl);
        store<u32>(t1 + 20, arg1);
        const u32 c = static_cast<u32>(call(ctx, tfn::Obj_Attach, {o, t1, 0}));
        store<u32>(o + cur, c);
        VCall(ctx, c, 20, {c, o});
        store<u32>(t1, tfn::kTmplVtBuild);
        call(ctx, tfn::SortedMap_Remove, {tfn::kRegistry, t1 + 4});
    }
    store<u32>(o + prev, load<u32>(o + cur));
    TmplPush(ctx, t2, o, tfn::kStrTmpExtControl, 10);
    store<u32>(t2, tfn::kTmplVtExtControl);
    const u32 c2 = static_cast<u32>(call(ctx, tfn::Obj_Attach, {o, t2, 0}));
    const u32 p = load<u32>(o + prev);
    store<u32>(o + ext, c2);
    if (p == 0) store<u32>(o + prev, c2);
    store<u32>(t2, tfn::kTmplVtBuild);
    call(ctx, tfn::SortedMap_Remove, {tfn::kRegistry, t2 + 4});
}

}  // namespace

// void f(Obj* o): AttachControls (+952 / +956 / +960).
DECOMP(0023099C, AttachControls_0023099C) { AttachControls(ctx, 952, 956, 960); }

// void f(Obj* o): AttachControls (+400 / +404 / +408).
DECOMP(00246D5C, AttachControls_00246D5C) { AttachControls(ctx, 400, 404, 408); }

// ---------------------------------------------------------------- XML template readers

namespace {

namespace xfn {
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (cursor, name) -> const char* or 0
constexpr u32 XmlCursor_DocPath = 0x004F8944;         // (x)
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;         // (c, hash, f32* out, f1 default)
constexpr u32 SmallAlloc = 0x0055238C;                // (size) -> p
constexpr u32 kStrName = 0x00AD4C0C;                  // "name"
}  // namespace xfn

// Tmpl* f(x, XmlCursor* c, u8 flag): a 44-byte template named by c's
// "name" attribute (XmlCursor_DocPath(x) and 0 without one): registered
// (kind, owner 0), its vtable vt and six f32 fields +20..+40 cleared, its
// u8 +16 = flag, then the six read from the attributes (hashes below),
// each defaulting to its value. Quirk kept: a failed SmallAlloc still
// writes +16 and reads the fields at address 0.
void ReadTmplF32x6(PPUContext& ctx, u32 kind, u32 vt) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    const u32 flag = arg<u32>(ctx, 2);
    const u32 name = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttrByName, {c, xfn::kStrName}));
    if (name == 0) {
        call(ctx, xfn::XmlCursor_DocPath, {x});
        ret(ctx, 0u);
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, xfn::SmallAlloc, {44}));
    if (p != 0) {
        TmplPush(ctx, p, 0, name, kind);
        store<u32>(p, vt);
        for (u32 off = 20; off <= 40; off += 4) store<u32>(p + off, 0);
    }
    store<u8>(p + 16, static_cast<u8>(flag));
    static constexpr u32 kHash[6] = {0x8FA16092, 0x996B4041, 0x807548E2, 0x476DF873, 0x0148AFE3, 0x7B2F95A0};
    for (u32 k = 0; k < 6; ++k)
        callf(ctx, xfn::XmlCursor_ReadF32, {c, kHash[k], p + 20 + 4 * k}, {std::bit_cast<f64>(LfsBits(load<u32>(p + 20 + 4 * k)))});
    ret(ctx, p);
}

}  // namespace

// Tmpl* f(x, XmlCursor* c, u8 flag): ReadTmplF32x6 (kind 42, vtable 0x00B1CCC8).
DECOMP(002B2814, XmlTmpl_Read_002B2814) { ReadTmplF32x6(ctx, 42, 0x00B1CCC8); }

// Tmpl* f(x, XmlCursor* c, u8 flag): ReadTmplF32x6 (kind 43, vtable 0x00B1CDA0).
DECOMP(002B4E78, XmlTmpl_Read_002B4E78) { ReadTmplF32x6(ctx, 43, 0x00B1CDA0); }
