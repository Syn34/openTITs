// Ui_LoadElement and Ui_LoadFile.
#include "loader.h"

using namespace sly4;
using namespace sly4::uiload;

namespace {

// The element vfuncs the instance path reads its template through.
constexpr u32 kVfnGetMatrix = 16, kVfnScalePtr = 36, kVfnGetColor = 52;
constexpr u32 kVfnFieldA = 60, kVfnFieldB = 68, kVfnFieldC = 76;
constexpr u32 kVfnFlags[7] = {84, 92, 100, 108, 116, 124, 132};  // IsActive .. GetFlag05
constexpr u32 kVfnClone = 8;

inline u32 vslot(u32 obj, u32 off) { return load<u32>(load<u32>(obj) + off); }
inline u32 vcall(PPUContext& ctx, u32 obj, u32 off, u32 a0) {
    const u32 opd = vslot(obj, off);
    return static_cast<u32>(call_ptr(ctx, opd, {a0, opd, load<u32>(opd)}));  // r4 / r5 = OPD and entry
}

void CopyRows(u32 dst, u32 src) {  // four 3-float rows, 16 bytes apart
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(dst + row + k, load<u32>(src + row + k));
}

// Fills a default description.
void DescDefaults(u32 d, u32 name) {
    store<u32>(d, name);
    CopyRows(d + 16, kIdentity);
    for (u32 k = 0; k < 12; k += 4) store<u32>(d + 80 + k, load<u32>(kOneVec3 + k));
    for (u32 k = 0; k < 4; ++k) store<u8>(d + 96 + k, load<u8>(kDefaultColor + k));
    for (u32 off = 100; off <= 116; off += 4) store<u32>(d + off, 0);
    static constexpr u8 kFlags[7] = {1, 0, 0, 1, 1, 1, 0};
    for (u32 k = 0; k < 7; ++k) store<u8>(d + 120 + k, kFlags[k]);
}

// The h (bits 0-1) / v (bits 4-5) alignment letters of "anchor" / "align".
void ApplyAlign(u32 field, u32 s) {
    for (u8 c; (c = load<u8>(s)) != 0; ++s) {
        u32 v = load<u32>(field);
        switch (static_cast<s8>(c)) {
            case 'T': case 't': v &= ~0x30u; break;
            case 'M': case 'm': v = (v & ~0x30u) | 0x10; break;
            case 'B': case 'b': v = (v & ~0x30u) | 0x20; break;
            case 'L': case 'l': v &= ~0x3u; break;
            case 'C': case 'c': v = (v & ~0x3u) | 0x1; break;
            case 'R': case 'r': v = (v & ~0x3u) | 0x2; break;
            default: continue;
        }
        store<u32>(field, v);
    }
}

// Calls a builder hook: (doc, node, desc, tmpl), r7 / r8 = OPD and entry.
u32 CallHook(PPUContext& ctx, u32 hook, u32 doc, u32 node, u32 desc, u32 tmpl) {
    const u32 opd = load<u32>(hook);
    if (opd == 0) return 0;
    return static_cast<u32>(call_ptr(ctx, opd, {doc, node, desc, tmpl, opd, load<u32>(opd)}));
}

}  // namespace

// Element* Ui_LoadElement(XmlCursor* doc, XmlCursor* node, Element* parent, u32 arg)
// Builds the element a node describes (by tag), then its child nodes
// under it. An instance node (kTagInstance) copies the "from=" element's
// properties, builds an element of its type and, unless told otherwise,
// clones its children too. Common attributes: scale, color, the seven
// flags, pos, rot (degrees), blend, anchor, align.
DECOMP(004DE0F8, Ui_LoadElement) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), parent = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3);

    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 544 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 rot = sp + 112;   // f32[16]
    const u32 da = sp + 176;    // the description that is built
    const u32 db = sp + 304;    // the defaults (the template's, for an instance)
    const u32 angle = sp + 432, blends = sp + 436, clone_children = sp + 448, color = sp + 449;

    const u32 tag = (load<u8>(node + 24) & 0x80) != 0 ? load<u32>(node + 20) : 0;
    const u32 name = static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {node, 0x00AEF4C0}));
    if (name == 0) {
        call(ctx, ufn::XmlCursor_DocPath, {doc});
        return ret(ctx, 0u);
    }
    DescDefaults(da, name);
    DescDefaults(db, name);
    store<u8>(clone_children, 1);

    u32 tmpl = 0;
    if (tag == kTagInstance) {
        const u32 from = static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {node, kStrFrom}));
        if (from == 0) {
            call(ctx, ufn::XmlCursor_DocPath, {doc});
            return ret(ctx, 0u);
        }
        tmpl = static_cast<u32>(call(ctx, lofn::Ui_Find, {from, parent}));
        if (tmpl == 0) {
            call(ctx, ufn::XmlCursor_DocPath, {doc});
            return ret(ctx, 0u);
        }
        CopyRows(db + 16, vcall(ctx, tmpl, kVfnGetMatrix, tmpl));
        const u32 sc = vcall(ctx, tmpl, kVfnScalePtr, tmpl);
        for (u32 k = 0; k < 12; k += 4) store<u32>(db + 80 + k, load<u32>(sc + k));
        {
            const u32 opd = vslot(tmpl, kVfnGetColor);
            call_ptr(ctx, opd, {color, tmpl, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
        }
        for (u32 k = 0; k < 4; ++k) store<u8>(db + 96 + k, load<u8>(color + k));
        store<u32>(db + 100, vcall(ctx, tmpl, kVfnFieldA, tmpl));
        store<u32>(db + 112, vcall(ctx, tmpl, kVfnFieldB, tmpl));
        store<u32>(db + 116, vcall(ctx, tmpl, kVfnFieldC, tmpl));
        for (u32 k = 0; k < 7; ++k) store<u8>(db + 120 + k, static_cast<u8>(vcall(ctx, tmpl, kVfnFlags[k], tmpl)));
        call(ctx, ufn::XmlCursor_ReadBool, {node, kHashCloneChildren, clone_children, 1});
    }

    store<u32>(da + 104, parent);
    store<u32>(da + 108, a);
    if (parent != 0) store<u8>(db + 126, (load<u32>(parent + 136) & 0x20) != 0 ? 1 : 0);

    call(ctx, lofn::XmlCursor_ReadVec3, {node, kHashScale, da + 80, db + 80});
    call(ctx, lofn::XmlCursor_ReadColor, {node, kHashColor, da + 96, db + 96});
    for (u32 k = 0; k < 7; ++k)
        call(ctx, ufn::XmlCursor_ReadBool, {node, kHashFlags[k], da + 120 + k, load<u8>(db + 120 + k)});
    CopyRows(da + 16, db + 16);
    call(ctx, lofn::XmlCursor_ReadVec3, {node, kHashPos, da + 64, da + 64});
    ctx.fpr[1] = static_cast<f64>(load<f32>(kLoaderConsts));
    if (static_cast<u32>(call(ctx, ufn::XmlCursor_ReadF32, {node, kHashRot, angle})) == 0) {
        ctx.fpr[1] = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<f32>(angle)) *
                                                       static_cast<f64>(load<f32>(kLoaderConsts + 4))));
        call(ctx, lofn::Matrix_FromAxisAngle, {rot, kRotAxis});
        // The position is kept (it also lands in the matrix's last row).
        for (u32 k = 0; k < 12; k += 4) store<u32>(rot + 48 + k, load<u32>(da + 64 + k));
        CopyRows(da + 16, rot);
    }

    store<u32>(blends + 0, kBlendNames);
    store<u32>(blends + 4, kBlendNames + 8);
    store<u32>(blends + 8, kBlendNames + 16);
    store<u32>(da + 100, load<u32>(db + 100));
    const u32 blend = static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {node, kStrBlend}));
    if (blend != 0) {
        u32 k = 0;
        while (k < 3 && static_cast<u32>(call(ctx, lofn::String_CompareN, {blend, load<u32>(blends + k * 4), 2})) != 0) ++k;
        if (k < 3)
            store<u32>(da + 100, k);
        else
            call(ctx, ufn::XmlCursor_DocPath, {doc});
    }
    store<u32>(da + 116, load<u32>(db + 116));
    store<u32>(da + 112, load<u32>(db + 112));
    for (u32 which = 0; which < 2; ++which) {
        const u32 attr = static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {node, which ? kStrAlign : kStrAnchor}));
        if (attr != 0) ApplyAlign(which ? da + 112 : da + 116, attr);
    }

    u32 e = 0;
    const s32 st = static_cast<s32>(tag);
    if (st > 0x0400B14D) {
        switch (tag) {
            case 0x33FDE730: e = static_cast<u32>(call(ctx, lofn::UiElem2_FromXml, {doc, node, da, tmpl})); break;
            case 0x30CF0025: e = static_cast<u32>(call(ctx, lofn::UiElem8_FromXml, {doc, node, da, tmpl})); break;
            case 0x044D86D4: e = static_cast<u32>(call(ctx, lofn::UiText_FromXml, {doc, node, da, tmpl})); break;
            case 0x0428B13B: e = static_cast<u32>(call(ctx, lofn::UiElem1_FromXml, {doc, node, da, tmpl})); break;
            case 0x040EFE5F: e = static_cast<u32>(call(ctx, lofn::UiElem7_FromXml, {doc, node, da, tmpl})); break;
            default:
                e = CallHook(ctx, kUiElementHook, doc, node, da, tmpl);
                if (e == 0) call(ctx, ufn::XmlCursor_DocPath, {doc});
        }
    } else if (tag == 0x0400B14D) {
        e = static_cast<u32>(call(ctx, lofn::UiElem0_FromXml, {doc, node, da, tmpl}));
    } else if (tag == kTagInstance) {
        const s32 type = static_cast<s32>(vcall(ctx, tmpl, kVfnType, tmpl));
        u32 fn = 0;
        switch (type) {
            case 0: fn = lofn::UiElem0_FromXml; break;
            case 1: fn = lofn::UiElem1_FromXml; break;
            case 2: fn = lofn::UiElem2_FromXml; break;
            case 3: fn = lofn::UiText_FromXml; break;
            case 5: fn = lofn::UiText5_FromXml; break;
            case 6: fn = lofn::UiElem6_FromXml; break;
            case 7: fn = lofn::UiElem7_FromXml; break;
            case 8: fn = lofn::UiElem8_FromXml; break;
            case 9: fn = lofn::UiElem9_FromXml; break;
            default: break;  // 4, 10 and up, negative: the hook
        }
        if (fn != 0) {
            e = static_cast<u32>(call(ctx, fn, {doc, node, da, tmpl}));
        } else {
            e = CallHook(ctx, kUiInstanceHook, doc, node, da, tmpl);
            if (e == 0) call(ctx, ufn::XmlCursor_DocPath, {doc});
        }
    } else {
        switch (tag) {
            case 0x00274AAE: e = static_cast<u32>(call(ctx, lofn::UiElem6_FromXml, {doc, node, da, tmpl})); break;
            case 0xAA8CFA44: e = static_cast<u32>(call(ctx, lofn::UiElem9_FromXml, {doc, node, da, tmpl})); break;
            case 0x937B5F0A: e = static_cast<u32>(call(ctx, lofn::UiText5_FromXml, {doc, node, da, tmpl})); break;
            default:
                e = CallHook(ctx, kUiElementHook, doc, node, da, tmpl);
                if (e == 0) call(ctx, ufn::XmlCursor_DocPath, {doc});
        }
    }
    if (e == 0) return ret(ctx, 0u);

    if (tmpl != 0 && load<u8>(clone_children) != 0) {
        for (u32 c = load<u32>(tmpl + 152); c != 0; c = load<u32>(c + 156)) {
            const u32 opd = vslot(c, kVfnClone);
            call_ptr(ctx, opd, {c, c + 96, e, a, 1, opd, load<u32>(opd)});  // r8 / r9 = OPD and entry
        }
    }
    for (u32 child = static_cast<u32>(call(ctx, lofn::XmlCursor_FirstChildAny, {node})); child != 0;
         child = static_cast<u32>(call(ctx, lofn::XmlCursor_NextSiblingAny, {child})))
        call(ctx, lofn::Ui_LoadElement, {doc, child, e, a});
    ret(ctx, e);
}

// bool Ui_LoadFile(Include* inc, XmlCursor* doc, XmlCursor* root)
// The <resources> section (an optional named bank, then textures, fonts
// and font sets), the timelines, then the top-level elements. Returns 1.
DECOMP(004DFA60, Ui_LoadFile) {
    const u32 inc = arg<u32>(ctx, 0), doc = arg<u32>(ctx, 1), root = arg<u32>(ctx, 2);
    const u32 res = static_cast<u32>(call(ctx, lofn::XmlCursor_FirstChild, {root, kTagResources}));
    if (res != 0) {
        const u32 name = static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {res, 0x00AEF4C0}));
        if (name != 0) {
            const u32 bank = static_cast<u32>(call(ctx, lofn::Bank_Create, {0}));
            store<u32>(inc + kIncludeBank, bank);
            if (bank != 0) {
                call(ctx, lofn::Bank_Open, {bank, name, 0});
                call(ctx, lofn::Bank_Start, {load<u32>(inc + kIncludeBank)});
            }
        }
        struct Section { u32 tag, fn; };
        static constexpr Section kSections[3] = {
            {kTagTexture, lofn::Texture_FromXml}, {kTagFont, lofn::Font_FromXml}, {kTagFontSet, lofn::FontSet_FromXml}};
        for (const Section& s : kSections)
            for (u32 c = static_cast<u32>(call(ctx, lofn::XmlCursor_FirstChild, {res, s.tag})); c != 0;
                 c = static_cast<u32>(call(ctx, lofn::XmlCursor_NextSibling, {c, s.tag})))
                call(ctx, s.fn, {doc, c, inc});
    }
    const u32 tls = static_cast<u32>(call(ctx, lofn::XmlCursor_FirstChild, {root, kTagTimelines}));
    if (tls != 0)
        for (u32 c = static_cast<u32>(call(ctx, lofn::XmlCursor_FirstChild, {tls, kTagTimeline})); c != 0;
             c = static_cast<u32>(call(ctx, lofn::XmlCursor_NextSibling, {c, kTagTimeline})))
            call(ctx, lofn::Timeline_FromXml, {doc, c, inc});
    const u32 els = static_cast<u32>(call(ctx, lofn::XmlCursor_FirstChild, {root, kTagElements}));
    if (els != 0)
        for (u32 c = static_cast<u32>(call(ctx, lofn::XmlCursor_FirstChildAny, {els})); c != 0;
             c = static_cast<u32>(call(ctx, lofn::XmlCursor_NextSiblingAny, {c})))
            call(ctx, lofn::Ui_LoadElement, {doc, c, 0, inc});
    ret(ctx, 1u);
}
