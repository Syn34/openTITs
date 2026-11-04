// Small callees of the UI loader (see more.h).
#include "more.h"

using namespace sly4;
using namespace sly4::uimore;

namespace {

// XmlCursor / XmlDoc / XMLB element fields (src/xml/xml.h).
constexpr u32 kCurType = 4, kCurDoc = 16, kCurHash = 20, kCurFlags = 24, kCurDepth = 25, kCurIndex = 26, kCurParent = 28;
constexpr u8 kCurValid = 0x80;
constexpr u32 kDocData = 44, kDocRing = 176, kDocCursors = 192;
constexpr u32 kXmlbTopCount = 16, kXmlbTop = 20;
constexpr u32 kElemName = 4, kElemChildCount = 12, kElemChildren = 16;
constexpr u32 kCurVfnAsElement = 12;

inline u32 vslot(u32 obj, u32 off) { return load<u32>(load<u32>(obj) + off); }
// A virtual call with one argument; r4 / r5 = OPD and entry, as in the original.
inline u32 vcall1(PPUContext& ctx, u32 obj, u32 off) {
    const u32 opd = vslot(obj, off);
    return static_cast<u32>(call_ptr(ctx, opd, {obj, opd, load<u32>(opd)}));
}

// The next of the 4 pool cursors for `depth` (inlined in the original).
u32 XmlDoc_NextCursor(u32 doc, u32 depth) {
    auto& ring = ref<u8>(doc + depth + kDocRing);
    const u32 slot = ring;
    const u32 cursor = doc + ((depth << 7) & 0xFFFFFF80) + slot * 32 + kDocCursors;
    const u32 next = slot + 1;
    ring = static_cast<u8>((next & 0xFF) >= 4 ? 0 : next);
    return cursor;
}

// The next element after this one under the same parent, whatever its tag
// (inlined into both walkers below).
u32 XmlCursor_NextSiblingRawAny(u32 cursor) {
    if ((ref<u8>(cursor + kCurFlags) & kCurValid) == 0) return 0;
    const u32 parent = field<u32>(cursor, kCurParent);
    const u32 doc = field<u32>(cursor, kCurDoc);
    u32 list, count;
    if (parent != 0) {
        list = field<u32>(parent, kElemChildren);
        count = field<u16>(parent, kElemChildCount);
    } else {
        const u32 data = field<u32>(doc, kDocData);
        list = data + kXmlbTop;
        count = field<u16>(data, kXmlbTopCount);
    }
    const u32 i = field<u16>(cursor, kCurIndex) + 1u;
    if (i >= count) return 0;
    const u32 child = load<u32>(list + i * 4);
    const u32 out = XmlDoc_NextCursor(doc, ref<u8>(cursor + kCurDepth));
    // `out` can be `cursor` itself once the ring has gone round; this is
    // the original's order of reads and writes.
    ref<u8>(out + kCurFlags) |= kCurValid;
    ref<u8>(out + kCurDepth) = ref<u8>(cursor + kCurDepth);
    const u32 parent2 = field<u32>(cursor, kCurParent);
    const u32 hash = load<u32>(child);
    field<u32>(out, kCurParent) = parent2;
    field<u16>(out, kCurIndex) = static_cast<u16>(i);
    const u32 doc2 = field<u32>(cursor, kCurDoc);
    field<u32>(out, kCurHash) = hash;
    field<u32>(out, kCurDoc) = doc2;
    return out;
}

// From c on, the first cursor whose AsElement() isn't 0 (in practice c).
u32 FirstElementFrom(PPUContext& ctx, u32 c) {
    for (; c != 0; c = XmlCursor_NextSiblingRawAny(c))
        if (vcall1(ctx, c, kCurVfnAsElement) != 0) return vcall1(ctx, c, kCurVfnAsElement);
    return 0;
}

inline void Release(PPUContext& ctx, u32 obj) { vcall1(ctx, obj, kVfnRelease); }
inline void AddRef(u32 obj) { field<u16>(obj, kResRefCount) += 1; }

}  // namespace

// XmlCursor* XmlCursor_FirstChildRawAny(XmlCursor* cursor)
// The first child element whatever its tag (for the document: the first
// top-level element), in the next pool cursor one level deeper.
DECOMP(004F899C, XmlCursor_FirstChildRawAny) {
    const u32 cursor = arg<u32>(ctx, 0);
    u32 doc = field<u32>(cursor, kCurDoc);
    if ((ref<u8>(cursor + kCurFlags) & kCurValid) == 0) return ret(ctx, 0u);
    const u32 parent = field<u32>(cursor, kCurParent);
    const u32 type = field<u32>(cursor, kCurType);
    if (parent == 0 && type == 0) doc = cursor;  // the document itself

    u32 list, count, depth, elem;
    if (parent != 0) {
        depth = ref<u8>(cursor + kCurDepth) + 1u;
        elem = load<u32>(field<u32>(parent, kElemChildren) + field<u16>(cursor, kCurIndex) * 4u);
        list = field<u32>(elem, kElemChildren);
        count = field<u16>(elem, kElemChildCount);
    } else {
        const u32 data = field<u32>(doc, kDocData);
        if (type != 0) {  // a top-level element
            depth = 1;
            elem = load<u32>(data + field<u16>(cursor, kCurIndex) * 4u + kXmlbTop);
            list = field<u32>(elem, kElemChildren);
            count = field<u16>(elem, kElemChildCount);
        } else {
            depth = 0;
            elem = 0;
            list = data + kXmlbTop;
            count = field<u16>(data, kXmlbTopCount);
        }
    }
    if (count == 0) return ret(ctx, 0u);

    const u32 child = load<u32>(list);
    const u32 out = XmlDoc_NextCursor(doc, depth);
    const u8 flags = ref<u8>(out + kCurFlags);
    ref<u8>(out + kCurDepth) = static_cast<u8>(depth);
    field<u32>(out, kCurParent) = elem;
    field<u16>(out, kCurIndex) = 0;
    ref<u8>(out + kCurFlags) = static_cast<u8>(flags | kCurValid);
    field<u32>(out, kCurDoc) = doc;
    field<u32>(out, kCurHash) = load<u32>(child);
    ret(ctx, out);
}

// XmlCursor* XmlCursor_FirstChildAny(XmlCursor* cursor)
// The first child element, whatever its tag (XmlCursor_FirstChild without
// the tag filter).
DECOMP(004F9140, XmlCursor_FirstChildAny) {
    const u32 cursor = arg<u32>(ctx, 0);
    const u32 c = static_cast<u32>(call(ctx, mfn::XmlCursor_FirstChildRawAny, {cursor}));
    ret(ctx, FirstElementFrom(ctx, c));
}

// XmlCursor* XmlCursor_NextSiblingAny(XmlCursor* cursor)
// The next element under the same parent, whatever its tag.
DECOMP(004F8D74, XmlCursor_NextSiblingAny) {
    const u32 cursor = arg<u32>(ctx, 0);
    ret(ctx, FirstElementFrom(ctx, XmlCursor_NextSiblingRawAny(cursor)));
}

// const char* XmlCursor_Text(XmlCursor* cursor)
// The element's +4 string (0 for an invalid cursor).
DECOMP(004FA744, XmlCursor_Text) {
    const u32 cursor = arg<u32>(ctx, 0);
    if ((ref<u8>(cursor + kCurFlags) & kCurValid) == 0) return ret(ctx, 0u);
    const u32 parent = field<u32>(cursor, kCurParent);
    const u32 index = field<u16>(cursor, kCurIndex);
    u32 elem;
    if (parent != 0)
        elem = load<u32>(field<u32>(parent, kElemChildren) + index * 4);
    else
        elem = load<u32>(field<u32>(field<u32>(cursor, kCurDoc), kDocData) + index * 4 + kXmlbTop);
    ret(ctx, load<u32>(elem + kElemName));
}

// int String_CompareN(const char* a, const char* b, s32 n)
// strncasecmp for ASCII: -1, 0 or 1. Characters are signed; only A-Z fold.
DECOMP(00555EB0, String_CompareN) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const s32 n = arg<s32>(ctx, 2);
    auto lower = [](s32 c) -> s32 { return (c >= 'A' && c <= 'Z') ? static_cast<s8>(c + 32) : c; };
    for (s32 i = 0;; ++i) {
        const u8 ca = load<u8>(a + static_cast<u32>(i));
        if (ca == 0 && load<u8>(b + static_cast<u32>(i)) == 0) return ret(ctx, s32{0});
        if (i >= n) return ret(ctx, s32{0});
        const s32 la = lower(static_cast<s8>(ca));
        const s32 lb = lower(static_cast<s8>(load<u8>(b + static_cast<u32>(i))));
        if (la < lb) return ret(ctx, s32{-1});
        if (la > lb) return ret(ctx, s32{1});
    }
}

// Material* Tex_Acquire(Texture* tex)
// A new material for drawing with `tex` (one more reference to it); for a
// null texture, an untextured one.
DECOMP(00523B7C, Tex_Acquire) {
    const u32 tex = arg<u32>(ctx, 0);
    if (tex == 0) {
        const u32 shader = static_cast<u32>(call(ctx, mfn::Shader_FindByHash, {kShaderUntextured}));
        const u32 m = static_cast<u32>(call(ctx, mfn::Material_Alloc));
        store<u32>(m + kMatShader, shader);  // no null check, as in the original
        return ret(ctx, m);
    }
    const u32 shader = static_cast<u32>(call(ctx, mfn::Shader_FindByHash, {kShaderTextured}));
    const u32 m = static_cast<u32>(call(ctx, mfn::Material_Alloc));
    const u8 n = static_cast<u8>(load<u8>(m + kMatTexCount) + 1);
    store<u32>(m + kMatShader, shader);
    const u32 index = static_cast<u32>(n) - 1;  // -1 when the count wrapped to 0
    store<u8>(m + kMatTexCount, n);
    const u32 slot = m + kMatTextures + index * 4;
    const u32 old = load<u32>(slot);
    if (old != tex) {
        store<u32>(slot, tex);
        AddRef(tex);
        if (old != 0) Release(ctx, old);
    }
    if (index == 0) store<u16>(m + kMatTex0Word, static_cast<u16>(load<u32>(tex + 112)));
    ret(ctx, m);
}

// void UiElem2_SetBlend(Element* e, u32 blend)  [type 2]
// Stores the blend index in the flags and sets the material's blend
// state; depth test off, depth func ALWAYS.
DECOMP(004DD08C, UiElem2_SetBlend) {
    const u32 e = arg<u32>(ctx, 0), blend = arg<u32>(ctx, 1);
    const u32 flags = field<u32>(e, kElemFlags);
    const u32 m = field<u32>(e, kElemMaterial2);
    field<u32>(e, kElemFlags) = (flags & ~0x00C00000u) | ((blend << 22) & 0x00C00000u);
    if (m == 0) return;
    Blend_SetMode(m + kMatBlend, load<s32>(kBlendModes + blend * 4));
    Blend_SetDepth(load<u32>(e + kElemMaterial2) + kMatBlend, kGlAlways);
    const u32 b = load<u32>(e + kElemMaterial2) + kMatBlend;
    store<u32>(b + 8, load<u32>(b + 8) & 0x7FFFFFFF);
}

// void Texture_MakePages(UiTexture* t)
// Three materials on the loaded texture, one per blend index ("av",
// "ad", "su"), with depth func LEQUAL (flag 0x80000000) or ALWAYS and
// depth test from flag 0x40000000.
DECOMP(004D5C64, Texture_MakePages) {
    const u32 t = arg<u32>(ctx, 0);
    static constexpr s32 kModes[3] = {2, 3, 4};  // also stored at sp+112 in the original
    u32 h = load<u32>(t + kTexHandle);
    if (h == 0) return;
    for (u32 k = 0; k < 3; ++k) {
        const u32 slot = t + kTexPages + k * 4;
        const u32 m = static_cast<u32>(call(ctx, mfn::Tex_Acquire, {h}));
        store<u32>(slot, m);
        Blend_SetMode(m + kMatBlend, kModes[k]);
        const u32 b = load<u32>(slot) + kMatBlend;
        Blend_SetDepth(b, (load<u32>(t + kTexFlags) & 0x80000000) != 0 ? kGlLequal : kGlAlways);
        const u32 test = (load<u32>(t + kTexFlags) & 0x40000000) != 0 ? 1 : 0;
        const u32 b2 = load<u32>(slot) + kMatBlend;
        store<u32>(b2 + 8, (load<u32>(b2 + 8) & 0x7FFFFFFF) | (test << 31));
        h = load<u32>(t + kTexHandle);
    }
}

// void UiElem_SetTexture(Element* e, UiTexture* res, const f32 uv0[2], const f32 uv1[2])
// Points the element at a texture resource. Its material is kept when it
// already draws the same texture; otherwise the old material and resource
// are released and the material becomes the resource's page for the
// element's blend index (or a new one on the texture, or on the default
// texture when the resource isn't loaded or is null). Sets 0x02000000.
DECOMP(004D8A80, UiElem_SetTexture) {
    const u32 e = arg<u32>(ctx, 0), res = arg<u32>(ctx, 1), uv0 = arg<u32>(ctx, 2), uv1 = arg<u32>(ctx, 3);

    const u32 cur = field<u32>(e, kElemMaterial);
    bool keep = false;
    if (cur != 0) {
        if (res != 0) {
            const u32 bound = load<u32>(cur + kMatTextures);
            const u32 want = load<u16>(res + kResLoaded) != 2
                                 ? static_cast<u32>(call(ctx, mfn::Tex_GetDefault, {0}))
                                 : load<u32>(res + kTexHandle);
            keep = bound == want;
        }
        if (!keep) {
            const u32 old = field<u32>(e, kElemTexRes);
            if (old != 0) {
                Release(ctx, old);
                field<u32>(e, kElemTexRes) = 0;
            }
            Release(ctx, field<u32>(e, kElemMaterial));
            field<u32>(e, kElemMaterial) = 0;
        }
    }

    field<u32>(e, kElemTexRes) = res;
    const u32 u0 = res != 0 ? uv0 : kZeroVec2, u1 = res != 0 ? uv1 : kOneVec2;
    for (u32 k = 0; k < 8; k += 4) store<u32>(e + kElemUv0 + k, load<u32>(u0 + k));  // lfs / stfs pairs
    for (u32 k = 0; k < 8; k += 4) store<u32>(e + kElemUv1 + k, load<u32>(u1 + k));

    if (!keep) {
        auto set_blend = [&] {
            const u32 opd = vslot(e, kVfnSetBlend);
            const u32 blend = (field<u32>(e, kElemFlags) >> 22) & 3;
            call_ptr(ctx, opd, {e, blend, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
        };
        if (res == 0) {
            const u32 def = static_cast<u32>(call(ctx, mfn::Tex_GetDefault, {1}));
            field<u32>(e, kElemMaterial) = static_cast<u32>(call(ctx, mfn::Tex_Acquire, {def}));
            set_blend();
        } else {
            const u16 refs = static_cast<u16>(load<u16>(res + kResRefCount) + 1);
            const u16 users = static_cast<u16>(load<u16>(res + kResUsers) + 1);
            store<u16>(res + kResRefCount, refs);
            store<u16>(res + kResUsers, users);
            if (static_cast<s16>(users) > 0 && load<u16>(res + kResLoaded) < 1)
                store<u16>(res + kResLoaded, static_cast<u16>(vcall1(ctx, res, kResVfnLoad)));
            // Blend index 3 reads the word after the three pages, as in the original.
            const u32 page = load<u32>(res + kTexPages + ((field<u32>(e, kElemFlags) >> 22) & 3) * 4);
            field<u32>(e, kElemMaterial) = page;
            if (page != 0) {
                AddRef(page);
            } else {
                const u32 tex = load<u16>(res + kResLoaded) == 2
                                    ? load<u32>(res + kTexHandle)
                                    : static_cast<u32>(call(ctx, mfn::Tex_GetDefault, {0}));
                field<u32>(e, kElemMaterial) = static_cast<u32>(call(ctx, mfn::Tex_Acquire, {tex}));
                set_blend();
            }
        }
    }
    field<u32>(e, kElemFlags) |= 0x02000000;
}

// Texture* Tex_FindLoaded(u32 file_hash)
// The texture already loaded from a file of that name, or 0.
DECOMP(0050C398, Tex_FindLoaded) {
    const u32 hash = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), hash);
    const u32 map = load<u32>(kTexDefaults + 24);
    // r5 = kTexDefaults here in the original; the callee doesn't read it.
    const u32 slot = static_cast<u32>(call(ctx, mfn::Map_Find, {map, frame.addr()}));
    ret(ctx, load<u32>(slot));
}

// Texture* Tex_LoadFile(const char* path, bool fallback)
// Loads a texture file (any format the texture code knows by extension).
// On failure, with `fallback`, the default texture 0 (one more reference)
// or 0.
DECOMP(0051E68C, Tex_LoadFile) {
    const u32 path = arg<u32>(ctx, 0);
    const u8 fallback = static_cast<u8>(arg<u32>(ctx, 1));
    Stack frame(ctx, 16);
    const u32 size_out = frame.addr();
    u32 tex = 0;
    const u32 data = static_cast<u32>(call(ctx, mfn::File_LoadWhole, {path, size_out}));
    if (data != 0) {
        const u32 ext = static_cast<u32>(call(ctx, mfn::Path_GetExtension, {path}));
        const u32 size = load<u32>(size_out);
        const u32 t = static_cast<u32>(call(ctx, mfn::Texture_Alloc));
        const u32 name = t + 16;
        call(ctx, mfn::String_CopyN, {name, path, 32});
        store<u8>(t + 47, 0);
        store<u32>(t + 48, static_cast<u32>(call(ctx, mfn::String_HashNoCase, {name})));
        tex = t;
        if (static_cast<u32>(call(ctx, mfn::Texture_Init, {t, data, size, ext, path})) == 0) {
            Release(ctx, t);
            tex = 0;
        }
        call(ctx, mfn::Mem_Free, {data});
        if (tex != 0) return ret(ctx, tex);
    }
    if (fallback != 0) {
        tex = load<u32>(kTexDefaults);
        if (tex != 0) AddRef(tex);
    }
    ret(ctx, tex);
}

// TimelineDef* Timeline_FromXml(XmlCursor* doc, XmlCursor* node, Include* inc)
// <timeline name=...>: a 56-byte definition appended to the list at
// 0x00F6689C, with its keys (each child element) chained from +48 by +8.
DECOMP(004DF88C, Timeline_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), inc = arg<u32>(ctx, 2);
    const u32 name = static_cast<u32>(call(ctx, mfn::XmlCursor_FindAttrByName, {node, kStrName}));
    if (name == 0) {
        call(ctx, mfn::XmlCursor_DocPath, {doc});
        return ret(ctx, 0u);
    }
    const u32 t = static_cast<u32>(call(ctx, mfn::Mem_Alloc, {kTimelineDefSize, 0, 16}));
    if (t != 0) {
        store<u32>(t, kTimelineDefVtbl);
        store<u32>(t + 36, static_cast<u32>(call(ctx, mfn::String_HashNoCase, {name})));
        store<u32>(t + 40, inc);
        store<u8>(t + 44, 0);
        store<u32>(t + 48, 0);
        store<u32>(t + 52, 0);
        call(ctx, mfn::String_CopyN, {t + 4, name, 31});
        u32 last = load<u32>(kTimelineDefs);
        if (last == 0) {
            store<u32>(kTimelineDefs, t);
        } else {
            while (load<u32>(last + 52) != 0) last = load<u32>(last + 52);
            store<u32>(last + 52, t);
        }
    }
    // With a failed allocation these touch guest addresses 44 and 48, as in the original.
    call(ctx, mfn::XmlCursor_ReadBool, {node, kHashTimelineFlag, t + 44, 0});
    u32 prev = load<u32>(t + 48);
    for (u32 c = static_cast<u32>(call(ctx, mfn::XmlCursor_FirstChildAny, {node})); c != 0;
         c = static_cast<u32>(call(ctx, mfn::XmlCursor_NextSiblingAny, {c}))) {
        const u32 key = static_cast<u32>(call(ctx, mfn::TimelineDef_KeyFromXml, {doc, c, t}));
        if (key == 0) continue;
        if (prev != 0)
            store<u32>(prev + 8, key);
        else
            store<u32>(t + 48, key);
        prev = key;
    }
    ret(ctx, t);
}

// void UiElem8_Construct(Element* e, const Elem8Desc* d)  [type 8, vtable 0x00B22DD0]
// An element showing a world object: d +132 (an object to use), d +136 (a
// class to instance) or else the object named d +128 (looked up, then
// instanced from the class of that name). d +140 set: the object itself
// is used (one more reference); otherwise a copy named "<name>_GUIInst".
// The object is scaled by the element's scale (times the parent's if flag
// 0x10000000) with x corrected for the screen's aspect ratio.
DECOMP(004D7F24, UiElem8_Construct) {
    const u32 e = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 208 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 scale = sp + 112, pscale = sp + 128, key = sp + 144;

    call(ctx, mfn::UiElem_Construct, {e, d});
    field<u32>(e, kElemMaterial) = 0;
    store<u32>(e, kElem8Vtbl);
    u32 obj = load<u32>(d + 132);
    u32 cls = load<u32>(d + 136);
    u64 r3 = 0;  // what r3 holds if the function returns early (no caller reads it)
    if (obj == 0 && cls == 0) {
        obj = static_cast<u32>(call(ctx, mfn::WorldObj_FindByHash,
                                    {call(ctx, mfn::String_HashNoCase, {load<u32>(d + 128)}), 0}));
        if (obj == 0) {
            store<u32>(key, static_cast<u32>(call(ctx, mfn::String_HashNoCase, {load<u32>(d + 128)})));
            const u32 map = load<u32>(load<u32>(kWorldClassMapHolder) + 16);
            const u32 slot = static_cast<u32>(call(ctx, mfn::HashMap_Find, {map, key}));
            cls = load<u32>(slot);
            r3 = slot;
        }
    }
    if (obj != 0) {
        if (load<u8>(d + 140) != 0) {
            field<u32>(e, kElemMaterial) = obj;
            AddRef(obj);
        } else {
            const u32 opd = vslot(obj, 56);
            r3 = call_ptr(ctx, opd, {obj, 5, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
            field<u32>(e, kElemMaterial) = static_cast<u32>(r3);
        }
    } else if (cls != 0) {
        r3 = call(ctx, mfn::WorldObj_Instance, {cls, 0});
        field<u32>(e, kElemMaterial) = static_cast<u32>(r3);
    }

    u32 o = field<u32>(e, kElemMaterial);
    if (o == 0) {
        ctx.gpr[3] = r3;
        return;
    }
    if (load<u8>(d + 140) == 0) {
        const u32 src_name = vcall1(ctx, load<u32>(o + 208), 12);
        const u32 inst_name = static_cast<u32>(call(ctx, mfn::String_Format, {kStrGuiInst, src_name}));
        call(ctx, mfn::WorldObj_SetName, {o, inst_name, 1});
        o = field<u32>(e, kElemMaterial);
    }
    call(ctx, mfn::WorldObj_Func583E54, {o, 12});

    for (u32 k = 0; k < 12; k += 4) store<u32>(scale + k, load<u32>(e + 80 + k));  // lfs / stfs pairs
    f64 sx = static_cast<f64>(load<f32>(scale));
    const u32 parent = load<u32>(e + 148);
    if ((field<u32>(e, kElemFlags) & 0x10000000) != 0 && parent != 0) {
        const u32 opd = vslot(parent, 168);
        call_ptr(ctx, opd, {pscale, parent, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
        auto fmuls = [](f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a * b)); };
        sx = fmuls(load<f32>(scale), load<f32>(pscale));
        const f64 sy = fmuls(load<f32>(scale + 4), load<f32>(pscale + 4));
        const f64 sz = fmuls(load<f32>(scale + 8), load<f32>(pscale + 8));
        store<f32>(scale, static_cast<f32>(sx));
        store<f32>(scale + 4, static_cast<f32>(sy));
        store<f32>(scale + 8, static_cast<f32>(sz));
    }
    const f64 w = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<u16>(kConfig + 4))));
    const f64 h = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<u16>(kConfig + 6))));
    const f64 aspect = static_cast<f64>(static_cast<f32>(w / h));
    const f64 k = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<f32>(kConfig + 16)) / aspect));
    store<f32>(scale, static_cast<f32>(sx * k));
    call(ctx, mfn::WorldObj_SetScale, {field<u32>(e, kElemMaterial), scale, 0});
    {
        const u32 obj2 = field<u32>(e, kElemMaterial);
        const u32 opd = vslot(obj2, 40);
        call_ptr(ctx, opd, {obj2, 0, 0, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
    }
    call(ctx, mfn::UiElem8_Attach, {e, field<u32>(e, kElemMaterial), 1});
}
