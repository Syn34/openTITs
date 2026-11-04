// The element builders Ui_LoadElement dispatches to by tag (types 0, 1,
// 2, 6, 7, 8, 9). Each copies the loader's description, reads its own
// attributes (defaults from a template of the same type, if any) and
// constructs the element. The original also fills a second, default
// description on the stack that nothing reads; those stack stores are
// left out.
#include "loader.h"

using namespace sly4;
using namespace sly4::uiload;

namespace {

namespace cfn {
constexpr u32 UiElem_Construct = 0x004D76D0;
constexpr u32 UiElem8_Construct = 0x004D7F24;   // (e, desc) (name?)
constexpr u32 UiElem_SetTexture = 0x004D8A80;   // (e, Texture*, const f32 uv0[2], const f32 uv1[2]) (name?)
constexpr u32 UiElem6_Build = 0x004D8E88;       // (e; f1) (name?)
constexpr u32 UiElem2_SetBlend = 0x004DD08C;    // (e, u32 blend) (name?)
constexpr u32 Tex_GetDefault = 0x004FEF30;
constexpr u32 Tex_Acquire = 0x00523B7C;         // (Texture*) -> Texture* (name?)
constexpr u32 SmallAlloc = 0x0055238C;
constexpr u32 memset = 0x00726E54;
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 XmlCursor_ReadVec2 = 0x004F9C9C;
}  // namespace cfn

constexpr u32 kZeroVec2 = 0x01164AFC;
constexpr u32 kOneVec2 = 0x01164AF4;
constexpr u32 kHashSize = 0x0026D801;
constexpr u32 kVfnSizeOut = 44;  // template: (sret f32[2], e)
constexpr u32 kFltMax = 0x7F7FFFFF;

inline u32 vslot(u32 obj, u32 off) { return load<u32>(load<u32>(obj) + off); }

// The template's type (vfunc +12), with r4 / r5 = OPD and entry.
s32 TypeOf(PPUContext& ctx, u32 t) {
    const u32 opd = vslot(t, 12);
    return static_cast<s32>(call_ptr(ctx, opd, {t, opd, load<u32>(opd)}));
}
// The template's size (vfunc +44 with a result buffer), r5 / r6 = OPD and entry.
void SizeOf(PPUContext& ctx, u32 t, u32 out) {
    const u32 opd = vslot(t, kVfnSizeOut);
    call_ptr(ctx, opd, {out, t, opd, load<u32>(opd)});
}

void CopyBase(u32 d, u32 base, bool flag126) {
    store<u32>(d, load<u32>(base));
    for (u32 row = 16; row <= 80; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(d + row + k, load<u32>(base + row + k));
    for (u32 k = 0; k < 4; ++k) store<u8>(d + 96 + k, load<u8>(base + 96 + k));
    for (u32 off = 100; off <= 116; off += 4) store<u32>(d + off, load<u32>(base + off));
    for (u32 k = 0; k < 6; ++k) store<u8>(d + 120 + k, load<u8>(base + 120 + k));
    store<u8>(d + 126, flag126 ? load<u8>(base + 126) : 0);
}

inline void Copy2(u32 dst, u32 src) {
    store<u32>(dst, load<u32>(src));
    store<u32>(dst + 4, load<u32>(src + 4));
}

u32 FindRes(u32 hash) {
    for (u32 r = load<u32>(kResList); r != 0; r = load<u32>(r + 88))
        if (load<u32>(r + 76) == hash) return r;
    return 0;
}

// Picks the index of a hash in a table, or reports an error.
void ReadEnum(PPUContext& ctx, u32 doc, u32 node, u32 attr, u32 table, u32 n, u32 out, u32 tmp) {
    if (static_cast<u32>(call(ctx, ufn::XmlCursor_ReadHash, {node, attr, tmp, 0})) != 0) return;
    u32 k = 0;
    while (k < n && load<u32>(table + k * 4) != load<u32>(tmp)) ++k;
    if (k < n)
        store<u32>(out, k);
    else
        call(ctx, ufn::XmlCursor_DocPath, {doc});
}

}  // namespace

// Element* UiElem9_FromXml(doc, node, base, tmpl)  [type 9, vtable 0x00B22E98, 192 bytes]
// Two vec2s: size= (template default) and 0x76A60DC1 (default 0).
DECOMP(004D7910, UiElem9_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), base = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 544 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 d = sp + 112, def0 = sp + 384, def1 = sp + 392, tsize = sp + 400;

    CopyBase(d, base, true);
    Copy2(d + 128, kZeroVec2);
    Copy2(d + 136, kZeroVec2);
    Copy2(def0, kZeroVec2);
    Copy2(def1, kZeroVec2);
    if (tmpl != 0) {
        if (TypeOf(ctx, tmpl) == 9) {
            SizeOf(ctx, tmpl, tsize);
            Copy2(def0, tsize);
        } else {
            call(ctx, ufn::XmlCursor_DocPath, {doc});
        }
    }
    call(ctx, cfn::XmlCursor_ReadVec2, {node, kHashSize, d + 128, def0});
    call(ctx, cfn::XmlCursor_ReadVec2, {node, 0x76A60DC1, d + 136, def1});
    const u32 e = static_cast<u32>(call(ctx, cfn::SmallAlloc, {192}));
    if (e == 0) return ret(ctx, 0u);
    call(ctx, cfn::UiElem_Construct, {e, d});
    store<u32>(e, 0x00B22E98);
    for (u32 k = 0; k < 16; k += 4) store<u32>(e + 168 + k, load<u32>(d + 128 + k));
    ret(ctx, e);
}

// Element* UiElem8_FromXml(doc, node, base, tmpl)  [type 8, 176 bytes]
// objName= (the string), 0xA8C8CC1C (bool), and the template's +168.
// The description's +126 flag is cleared.
DECOMP(004D8200, UiElem8_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), base = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 352 - kFrameBase);
    const u32 d = frame.addr() - kFrameBase + 112;

    CopyBase(d, base, false);
    store<u32>(d + 128, kEmptyString);
    store<u32>(d + 136, 0);
    store<u8>(d + 140, 0);
    u32 obj = 0;
    if (tmpl != 0) {
        store<u32>(d + 132, 0);
        if (TypeOf(ctx, tmpl) == 8)
            obj = load<u32>(tmpl + 168);
        else
            call(ctx, ufn::XmlCursor_DocPath, {doc});
    }
    store<u32>(d + 132, obj);
    store<u32>(d + 128, static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {node, 0x00AEF580})));  // "objName"
    call(ctx, ufn::XmlCursor_ReadBool, {node, 0xA8C8CC1C, d + 140, 0});
    const u32 e = static_cast<u32>(call(ctx, cfn::SmallAlloc, {176}));
    if (e == 0) return ret(ctx, 0u);
    call(ctx, cfn::UiElem8_Construct, {e, d});
    ret(ctx, e);
}

// Element* UiElem7_FromXml(doc, node, base, tmpl)  [type 7, vtable 0x00B22D08, 192 bytes]
// size=, 0x8ACA5893 (vec2), two enums (0xFBA47770, 0xFF04AF8C) and a
// float (0xB2DC7023, default FLT_MAX or the template's +188).
DECOMP(004D8460, UiElem7_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), base = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 608 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 d = sp + 112, defsize = sp + 400, def1 = sp + 408, deffloat = sp + 424, tmp = sp + 432, tsize = sp + 436;

    CopyBase(d, base, true);
    Copy2(d + 128, kZeroVec2);
    Copy2(d + 136, kZeroVec2);
    store<u32>(d + 144, 0);
    store<u32>(d + 148, 0);
    store<u32>(d + 152, kFltMax);
    Copy2(defsize, kZeroVec2);
    Copy2(def1, kZeroVec2);
    store<u32>(sp + 416, 0);
    store<u32>(sp + 420, 0);
    store<u32>(deffloat, kFltMax);
    if (tmpl != 0) {
        if (TypeOf(ctx, tmpl) == 7) {
            SizeOf(ctx, tmpl, tsize);
            Copy2(defsize, tsize);
            store<u32>(sp + 416, load<u8>(tmpl + 184));  // read but not used, as in the original
            store<u32>(sp + 420, load<u8>(tmpl + 185));
            store<u32>(deffloat, load<u32>(tmpl + 188));
        } else {
            // The original passes r31 here before it has reloaded the
            // document into it: r31 still holds 0x01164BD0 (the identity
            // matrix), so this "error report" reads that as a cursor.
            call(ctx, ufn::XmlCursor_DocPath, {kIdentity});
        }
    }
    call(ctx, cfn::XmlCursor_ReadVec2, {node, kHashSize, d + 128, defsize});
    call(ctx, cfn::XmlCursor_ReadVec2, {node, 0x8ACA5893, d + 136, def1});
    ReadEnum(ctx, doc, node, 0xFBA47770, 0x00AEF590, 2, d + 144, tmp);
    ReadEnum(ctx, doc, node, 0xFF04AF8C, 0x00AEF5A0, 2, d + 148, tmp);
    ctx.fpr[1] = static_cast<f64>(load<f32>(deffloat));
    call(ctx, ufn::XmlCursor_ReadF32, {node, 0xB2DC7023, d + 152});
    const u32 e = static_cast<u32>(call(ctx, cfn::SmallAlloc, {192}));
    if (e == 0) return ret(ctx, 0u);
    call(ctx, cfn::UiElem_Construct, {e, d});
    store<u32>(e, 0x00B22D08);
    for (u32 k = 0; k < 16; k += 4) store<u32>(e + 168 + k, load<u32>(d + 128 + k));
    store<u8>(e + 184, static_cast<u8>(load<u32>(d + 144)));
    store<u8>(e + 185, static_cast<u8>(load<u32>(d + 148)));
    store<u32>(e + 188, load<u32>(d + 152));
    ret(ctx, e);
}

// Element* UiElem6_FromXml(doc, node, base, tmpl)  [type 6, vtable 0x00B22C40, 256 bytes]
// Nine resources named by the attributes listed at 0x00F668A8 (defaults:
// the template's +168..+200) and size=.
DECOMP(004D9DA0, UiElem6_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), base = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 640 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 d = sp + 112, defres = sp + 416, defsize = sp + 452, tsize = sp + 464;
    constexpr u32 kAttrNames = 0x00F668A8;  // const char*[9]

    CopyBase(d, base, true);
    for (u32 k = 0; k < 9; ++k) store<u32>(d + 128 + k * 4, 0);
    Copy2(d + 164, kZeroVec2);
    for (u32 k = 0; k < 9; ++k) store<u32>(defres + k * 4, 0);
    Copy2(defsize, kZeroVec2);
    if (tmpl != 0) {
        if (TypeOf(ctx, tmpl) == 6) {
            for (u32 k = 0; k < 9; ++k) store<u32>(defres + k * 4, load<u32>(tmpl + 168 + k * 4));
            SizeOf(ctx, tmpl, tsize);
            Copy2(defsize, tsize);
        } else {
            call(ctx, ufn::XmlCursor_DocPath, {doc});
        }
    }
    for (u32 k = 0; k < 9; ++k) {
        store<u32>(d + 128 + k * 4, load<u32>(defres + k * 4));
        const u32 attr = static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {node, load<u32>(kAttrNames + k * 4)}));
        if (attr == 0) continue;
        const u32 r = FindRes(static_cast<u32>(call(ctx, ufn::String_HashNoCase, {attr})));
        store<u32>(d + 128 + k * 4, r);
        if (r == 0) call(ctx, ufn::XmlCursor_DocPath, {doc});
    }
    call(ctx, cfn::XmlCursor_ReadVec2, {node, kHashSize, d + 164, defsize});
    const u32 e = static_cast<u32>(call(ctx, efn::Mem_Alloc, {256, 0, 16}));
    if (e == 0) return ret(ctx, 0u);
    call(ctx, cfn::UiElem_Construct, {e, d});
    store<u32>(e, 0x00B22C40);
    store<u32>(e + 240, load<u32>(d + 164));
    store<u8>(e + 248, 1);
    store<u32>(e + 244, load<u32>(d + 168));
    call(ctx, cfn::memset, {e + 204, 0, 36});
    call(ctx, cfn::memmove, {e + 168, d + 128, 36});
    ctx.fpr[1] = static_cast<f64>(load<f32>(0x004D9D98));  // 0.0
    call(ctx, cfn::UiElem6_Build, {e});
    ret(ctx, e);
}

// Element* UiElem2_FromXml(doc, node, base, tmpl)  [type 2, vtable 0x00B22AB0, 208 bytes]
// size= and four corner colours; the default white texture.
DECOMP(004DD1F0, UiElem2_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), base = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 576 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 d = sp + 112, defsize = sp + 400, defcol = sp + 408, tsize = sp + 432;
    static constexpr u32 kHashColors[4] = {0x63647CFB, 0x63647D01, 0x63647ACD, 0x63647AD3};

    CopyBase(d, base, true);
    Copy2(d + 128, kZeroVec2);
    for (u32 k = 0; k < 16; ++k) store<u8>(d + 136 + k, load<u8>(base + 96 + (k & 3)));  // the element's colour
    Copy2(defsize, kZeroVec2);
    for (u32 k = 0; k < 16; ++k) store<u8>(defcol + k, load<u8>(kDefaultColor + (k & 3)));
    if (tmpl != 0) {
        if (TypeOf(ctx, tmpl) == 2) {
            SizeOf(ctx, tmpl, tsize);
            Copy2(defsize, tsize);
            for (u32 k = 0; k < 16; ++k) store<u8>(defcol + k, load<u8>(tmpl + 176 + k));
        } else {
            call(ctx, ufn::XmlCursor_DocPath, {doc});
        }
    }
    call(ctx, cfn::XmlCursor_ReadVec2, {node, kHashSize, d + 128, defsize});
    for (u32 k = 0; k < 4; ++k)
        call(ctx, lofn::XmlCursor_ReadColor, {node, kHashColors[k], d + 136 + k * 4, defcol + k * 4});
    const u32 e = static_cast<u32>(call(ctx, cfn::SmallAlloc, {208}));
    if (e == 0) return ret(ctx, 0u);
    call(ctx, cfn::UiElem_Construct, {e, d});
    store<u32>(e, 0x00B22AB0);
    Copy2(e + 168, d + 128);
    for (u32 k = 0; k < 16; ++k) store<u8>(e + 176 + k, load<u8>(d + 136 + k));
    // r4 = e + 188 and r5 = the last colour byte here in the original; neither callee reads them.
    const u32 tex = static_cast<u32>(call(ctx, cfn::Tex_GetDefault, {1}));
    store<u32>(e + 192, static_cast<u32>(call(ctx, cfn::Tex_Acquire, {tex})));
    call(ctx, cfn::UiElem2_SetBlend, {e, (load<u32>(e + 136) >> 22) & 3});
    ret(ctx, e);
}

// Element* UiElem1_FromXml(doc, node, base, tmpl)  [type 1, vtable 0x00B229E8, 208 bytes]
// texture= (a resource), two uv vec2s (0xAA1, 0xA61) and size= (default
// (-1, -1), not taken from the template).
DECOMP(004DD800, UiElem1_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), base = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 592 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 d = sp + 112, deftex = sp + 400, defuv0 = sp + 408, defuv1 = sp + 416, defsize = sp + 424;
    constexpr u32 kMinusOne = 0xBF800000;

    CopyBase(d, base, true);
    store<u32>(d + 128, 0);
    store<u32>(d + 132, 0);
    Copy2(d + 136, kZeroVec2);
    Copy2(d + 144, kOneVec2);
    store<u32>(d + 152, kMinusOne);
    store<u32>(d + 156, kMinusOne);
    store<u32>(deftex, 0);
    store<u32>(sp + 404, 0);
    Copy2(defuv0, kZeroVec2);
    Copy2(defuv1, kOneVec2);
    store<u32>(defsize, kMinusOne);
    store<u32>(defsize + 4, kMinusOne);
    u32 tex = 0;
    if (tmpl != 0) {
        if (TypeOf(ctx, tmpl) == 1) {
            Copy2(defuv0, tmpl + 176);
            Copy2(defuv1, tmpl + 184);
            tex = load<u32>(tmpl + 172);
            store<u32>(deftex, tex);
        } else {
            call(ctx, ufn::XmlCursor_DocPath, {doc});
            tex = load<u32>(deftex);
        }
    }
    store<u32>(d + 128, tex);
    const u32 attr = static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {node, 0x00AEF4E4}));  // "texture"
    if (attr != 0) {
        const u32 r = FindRes(static_cast<u32>(call(ctx, ufn::String_HashNoCase, {attr})));
        store<u32>(d + 128, r);
        if (r == 0) call(ctx, ufn::XmlCursor_DocPath, {doc});
    }
    call(ctx, cfn::XmlCursor_ReadVec2, {node, 0xAA1, d + 136, defuv0});
    call(ctx, cfn::XmlCursor_ReadVec2, {node, 0xA61, d + 144, defuv1});
    call(ctx, cfn::XmlCursor_ReadVec2, {node, kHashSize, d + 152, defsize});
    const u32 e = static_cast<u32>(call(ctx, cfn::SmallAlloc, {208}));
    if (e == 0) return ret(ctx, 0u);
    call(ctx, cfn::UiElem_Construct, {e, d});
    store<u32>(e, 0x00B229E8);
    store<u32>(e + 172, 0);
    const u32 owned = load<u32>(d + 132);  // always 0 here
    store<u32>(e + 168, owned);
    for (u32 k = 0; k < 24; k += 4) store<u32>(e + 176 + k, load<u32>(d + 136 + k));
    if (owned != 0) {
        store<u16>(owned + 4, static_cast<u16>(load<u16>(owned + 4) + 1));
    } else {
        call(ctx, cfn::UiElem_SetTexture, {e, load<u32>(d + 128), e + 176, e + 184});
    }
    ret(ctx, e);
}

// Element* UiElem0_FromXml(doc, node, base, tmpl)  [type 0, vtable 0x00B22920, 176 bytes]
// Just size=.
DECOMP(004DDD2C, UiElem0_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), base = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 576 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 d = sp + 112, defsize = sp + 384, tsize = sp + 400;

    CopyBase(d, base, true);
    Copy2(d + 128, kZeroVec2);
    Copy2(defsize, kZeroVec2);
    if (tmpl != 0) {
        if (TypeOf(ctx, tmpl) == 0) {
            SizeOf(ctx, tmpl, tsize);
            Copy2(defsize, tsize);
        } else {
            call(ctx, ufn::XmlCursor_DocPath, {doc});
        }
    }
    call(ctx, cfn::XmlCursor_ReadVec2, {node, kHashSize, d + 128, defsize});
    const u32 e = static_cast<u32>(call(ctx, cfn::SmallAlloc, {176}));
    if (e == 0) return ret(ctx, 0u);
    call(ctx, cfn::UiElem_Construct, {e, d});
    store<u32>(e, 0x00B22920);
    Copy2(e + 168, d + 128);
    ret(ctx, e);
}
