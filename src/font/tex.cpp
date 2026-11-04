// Texture resource slots and Texture_FromXml.
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

inline void Release(PPUContext& ctx, u32 obj) {
    const u32 opd = load<u32>(load<u32>(obj) + 8);
    call_ptr(ctx, opd, {obj, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
}

}  // namespace

// UiTexture* Texture_FromXml(XmlCursor* doc, XmlCursor* node, u32 arg)
// <... name= filename= preload= + two flags>.
DECOMP(004D5A4C, Texture_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    Stack frame(ctx, 176 - 112);
    const u32 bools = frame.addr();
    const u32 name = static_cast<u32>(call(ctx, lfn::XmlCursor_FindAttrByName, {node, kStrName}));
    const u32 file = static_cast<u32>(call(ctx, lfn::XmlCursor_FindAttrByName, {node, kStrFileName}));
    call(ctx, lfn::XmlCursor_ReadBool, {node, kHashPreload, bools + 0, 0});
    call(ctx, lfn::XmlCursor_ReadBool, {node, kHashFontFlag40, bools + 1, 0});
    call(ctx, lfn::XmlCursor_ReadBool, {node, kHashFontFlag20, bools + 2, 0});
    if (name == 0 || file == 0) {
        call(ctx, lfn::XmlCursor_DocPath, {doc});
        return ret(ctx, 0u);
    }
    const u32 t = static_cast<u32>(call(ctx, lfn::Mem_Alloc, {kTexSize, 0, 16}));
    if (t == 0) return ret(ctx, 0u);
    const u8 preload = load<u8>(bools + 0), b1 = load<u8>(bools + 1), b2 = load<u8>(bools + 2);
    call(ctx, rfn::UiRes_Construct, {t, name, file, preload, a});
    u32 f = load<u32>(t + kTexFlags) & 0x7FFFFFFF;
    store<u32>(t + kTexHandle, 0);
    f |= static_cast<u32>(b1) << 31;
    f = (f & ~0x40000000u) | ((static_cast<u32>(b2) << 30) & 0x40000000);
    store<u32>(t, kTextureVtbl);
    store<u32>(t + kTexFlags, f);
    for (u32 k = 0; k < 12; ++k) store<u8>(t + kTexPages + k, 0);
    if (preload != 0) {
        const u16 users = static_cast<u16>(load<u16>(t + kResUsers) + 1);
        store<u16>(t + kResUsers, users);
        if (static_cast<s16>(users) > 0 && load<u16>(t + kResLoaded) < 1) {
            const u32 opd = load<u32>(load<u32>(t) + kVfnLoad);
            store<u16>(t + kResLoaded, static_cast<u16>(call_ptr(ctx, opd, {t, opd, load<u32>(opd)})));
        }
    }
    ret(ctx, t);
}

// u16 Texture_Load(UiTexture* t)  [+12]
// An already loaded texture with the same file name (one more reference),
// else the file itself; 2 on success, 3 on failure.
DECOMP(004D5EAC, Texture_Load) {
    const u32 t = arg<u32>(ctx, 0);
    u32 h = static_cast<u32>(call(ctx, tfn2::Tex_FindLoaded, {load<u32>(t + kResFileHash)}));
    store<u32>(t + kTexHandle, h);
    if (h != 0) {
        store<u16>(h + 4, static_cast<u16>(load<u16>(h + 4) + 1));
    } else {
        h = static_cast<u32>(call(ctx, tfn2::Tex_LoadFile, {t + kResPath, 0}));
        store<u32>(t + kTexHandle, h);
    }
    if (h == 0) return ret(ctx, 3u);
    call(ctx, tfn2::Texture_MakePages, {t});
    ret(ctx, 2u);
}

// void Texture_Unload(UiTexture* t)  [+16]
DECOMP(004D5F38, Texture_Unload) {
    const u32 t = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 3; ++k) {
        const u32 p = load<u32>(t + kTexPages + k * 4);
        if (p == 0) continue;
        Release(ctx, p);
        store<u32>(t + kTexPages + k * 4, 0);
    }
    const u32 h = load<u32>(t + kTexHandle);
    if (h == 0) return;
    Release(ctx, h);
    store<u32>(t + kTexHandle, 0);
}

// The texture resource adds nothing to the UiRes destructors.
DECOMP(00794FD0, Texture_Destruct) { ResTeardown(ctx, arg<u32>(ctx, 0)); }

DECOMP(0079503C, Texture_Delete) {
    const u32 t = arg<u32>(ctx, 0);
    if (t == 0) return;
    ResTeardown(ctx, t);
    call(ctx, rfn::Mem_FreeFrom, {t, 0, t, kRefVtbl});  // r5 / r6 as in the original
}

