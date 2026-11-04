// The element classes' clone slots (vtable +8): (e, name, parent, arg,
// deep) -> a new element of the same class built from a description of
// e, plus (when deep) clones of e's children under it.
#include "class.h"

using namespace sly4;
using namespace sly4::uielem;

namespace {

namespace kfn {
constexpr u32 UiElem_Construct = 0x004D76D0;
constexpr u32 UiElem8_Construct = 0x004D7F24;
constexpr u32 UiElem_SetTexture = 0x004D8A80;
constexpr u32 UiElem6_Build = 0x004D8E88;
constexpr u32 UiElem2_SetBlend = 0x004DD08C;
constexpr u32 Tex_GetDefault = 0x004FEF30;
constexpr u32 Tex_Acquire = 0x00523B7C;
constexpr u32 SmallAlloc = 0x0055238C;
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 memset = 0x00726E54;
constexpr u32 memmove = 0x0071C3F0;
}  // namespace kfn

constexpr u32 kEmpty = 0x00AEF4A0;  // ""

inline u8 Bit(u32 v, u32 b) { return (v & b) != 0 ? 1 : 0; }

// The description every clone starts from (the base constructor's inverse).
void FillDesc(u32 d, u32 e, u32 name, u32 parent, u32 a, bool flag126) {
    const u32 f = load<u32>(e + 136);
    store<u32>(d, name);
    for (u32 row = 16; row <= 80; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(d + row + k, load<u32>(e + row + k));
    for (u32 k = 0; k < 4; ++k) store<u8>(d + 96 + k, load<u8>(e + 132 + k));
    store<u32>(d + 100, (f >> 22) & 3);
    store<u32>(d + 104, parent);
    store<u32>(d + 108, a);
    store<u32>(d + 112, (f >> 14) & 0xFF);
    store<u32>(d + 116, (f >> 6) & 0xFF);
    static constexpr u32 kBits[6] = {0x80000000, 0x40000000, 0x20000000, 0x10000000, 0x08000000, 0x04000000};
    for (u32 k = 0; k < 6; ++k) store<u8>(d + 120 + k, Bit(f, kBits[k]));
    store<u8>(d + 126, flag126 ? Bit(f, 0x20) : 0);
}

// Clones e's children under `result` when deep; r8 / r9 = OPD and entry.
void CloneChildren(PPUContext& ctx, u32 e, u32 result, u64 a, u64 deep) {
    if ((deep & 0xFF) == 0) return;
    for (u32 c = load<u32>(e + kElemFirstChild); c != 0; c = load<u32>(c + kElemNext)) {
        const u32 opd = text::vfunc(c, kVfnClone);
        call_ptr(ctx, opd, {c, c + 96, result, a, 1, opd, load<u32>(opd)});
    }
}

struct Args {
    u32 e, name, parent;
    u64 a, deep;
};
Args GetArgs(PPUContext& ctx) {
    return {arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2), ctx.gpr[6], ctx.gpr[7]};
}

inline void Copy(u32 dst, u32 src, u32 bytes) {
    for (u32 k = 0; k < bytes; k += 4) store<u32>(dst + k, load<u32>(src + k));
}

}  // namespace

// [type 0, vtable 0x00B22920, 176 bytes]: +168 size.
DECOMP(004E4BF8, UiElem0_Clone) {
    const Args g = GetArgs(ctx);
    Stack frame(ctx, 336 - 112);
    const u32 d = frame.addr();
    FillDesc(d, g.e, g.name, g.parent, static_cast<u32>(g.a), true);
    Copy(d + 128, g.e + 168, 8);
    u32 r = 0;
    const u32 n = static_cast<u32>(call(ctx, kfn::SmallAlloc, {176}));
    if (n != 0) {
        call(ctx, kfn::UiElem_Construct, {n, d});
        r = n;
        store<u32>(n, kVtbl920);
        Copy(n + 168, d + 128, 8);
    }
    CloneChildren(ctx, g.e, r, g.a, g.deep);
    ret(ctx, r);
}

// [type 1, vtable 0x00B229E8, 208 bytes]: the texture and uvs. A texture
// set with UiElem_SetTexture (+172) is set again on the copy; otherwise
// the +168 one is shared (one more reference).
DECOMP(004E53F0, UiElem1_Clone) {
    const Args g = GetArgs(ctx);
    Stack frame(ctx, 480 - 112);
    const u32 d = frame.addr();
    FillDesc(d, g.e, g.name, g.parent, static_cast<u32>(g.a), true);
    const u32 res = load<u32>(g.e + 172);
    const u32 owned = res == 0 ? load<u32>(g.e + 168) : 0;
    store<u32>(d + 132, owned);
    store<u32>(d + 128, res);
    Copy(d + 136, g.e + 176, 24);
    u32 r = 0;
    const u32 n = static_cast<u32>(call(ctx, kfn::SmallAlloc, {208}));
    if (n != 0) {
        call(ctx, kfn::UiElem_Construct, {n, d});
        Copy(n + 176, d + 136, 24);
        store<u32>(n, kVtbl9E8);
        const u32 o = load<u32>(d + 132);
        store<u32>(n + 168, o);
        store<u32>(n + 172, 0);
        if (o != 0)
            store<u16>(o + 4, static_cast<u16>(load<u16>(o + 4) + 1));
        else
            call(ctx, kfn::UiElem_SetTexture, {n, load<u32>(d + 128), n + 176, n + 184});
        r = n;
    }
    CloneChildren(ctx, g.e, r, g.a, g.deep);
    ret(ctx, r);
}

// [type 2, vtable 0x00B22AB0, 208 bytes]: size and four corner colours.
DECOMP(004E6414, UiElem2_Clone) {
    const Args g = GetArgs(ctx);
    Stack frame(ctx, 496 - 112);
    const u32 d = frame.addr();
    FillDesc(d, g.e, g.name, g.parent, static_cast<u32>(g.a), true);
    Copy(d + 128, g.e + 168, 8);
    for (u32 k = 0; k < 16; ++k) store<u8>(d + 136 + k, load<u8>(g.e + 176 + k));
    u32 r = 0;
    const u32 n = static_cast<u32>(call(ctx, kfn::SmallAlloc, {208}));
    if (n != 0) {
        call(ctx, kfn::UiElem_Construct, {n, d});
        store<u32>(n, kVtblAB0);
        Copy(n + 168, d + 128, 8);
        for (u32 k = 0; k < 16; ++k) store<u8>(n + 176 + k, load<u8>(d + 136 + k));
        const u32 tex = static_cast<u32>(call(ctx, kfn::Tex_GetDefault, {1}));
        store<u32>(n + 192, static_cast<u32>(call(ctx, kfn::Tex_Acquire, {tex})));
        call(ctx, kfn::UiElem2_SetBlend, {n, (load<u32>(n + 136) >> 22) & 3});
        r = n;
    }
    CloneChildren(ctx, g.e, r, g.a, g.deep);
    ret(ctx, r);
}

// [type 6, vtable 0x00B22C40, 256 bytes]: nine resources and size.
DECOMP(004E7D60, UiElem6_Clone) {
    const Args g = GetArgs(ctx);
    Stack frame(ctx, 368 - 112);
    const u32 d = frame.addr();
    FillDesc(d, g.e, g.name, g.parent, static_cast<u32>(g.a), true);
    Copy(d + 164, g.e + 240, 8);
    call(ctx, kfn::memmove, {d + 128, g.e + 168, 36});  // (the original also has a never-taken "zero them" path)
    u32 r = 0;
    const u32 n = static_cast<u32>(call(ctx, kfn::Mem_Alloc, {256, 0, 16}));
    if (n != 0) {
        call(ctx, kfn::UiElem_Construct, {n, d});
        store<u32>(n, kVtblC40);
        store<u32>(n + 240, load<u32>(d + 164));
        store<u8>(n + 248, 1);
        store<u32>(n + 244, load<u32>(d + 168));
        call(ctx, kfn::memset, {n + 204, 0, 36});
        call(ctx, kfn::memmove, {n + 168, d + 128, 36});
        ctx.fpr[1] = static_cast<f64>(load<f32>(0x004E7D58));  // 0.0
        call(ctx, kfn::UiElem6_Build, {n});
        r = n;
    }
    CloneChildren(ctx, g.e, r, g.a, g.deep);
    ret(ctx, r);
}

// [type 7, vtable 0x00B22D08, 192 bytes]: two vec2s, two enum bytes, a float.
DECOMP(004E8460, UiElem7_Clone) {
    const Args g = GetArgs(ctx);
    Stack frame(ctx, 448 - 112);
    const u32 d = frame.addr();
    FillDesc(d, g.e, g.name, g.parent, static_cast<u32>(g.a), true);
    Copy(d + 128, g.e + 168, 16);
    store<u32>(d + 144, load<u8>(g.e + 184));
    store<u32>(d + 148, load<u8>(g.e + 185));
    store<u32>(d + 152, load<u32>(g.e + 188));
    u32 r = 0;
    const u32 n = static_cast<u32>(call(ctx, kfn::SmallAlloc, {192}));
    if (n != 0) {
        call(ctx, kfn::UiElem_Construct, {n, d});
        store<u32>(n, kVtblD08);
        Copy(n + 168, d + 128, 16);
        store<u8>(n + 184, static_cast<u8>(load<u32>(d + 144)));
        r = n;
        store<u8>(n + 185, static_cast<u8>(load<u32>(d + 148)));
        store<u32>(n + 188, load<u32>(d + 152));
    }
    CloneChildren(ctx, g.e, r, g.a, g.deep);
    ret(ctx, r);
}

// [type 8, 176 bytes, constructed by 0x004D7F24]: the object name is
// not copied (""), only the +168 object; desc +126 is cleared.
DECOMP(004E9144, UiElem8_Clone) {
    const Args g = GetArgs(ctx);
    Stack frame(ctx, 336 - 112);
    const u32 d = frame.addr();
    FillDesc(d, g.e, g.name, g.parent, static_cast<u32>(g.a), false);
    store<u32>(d + 128, kEmpty);
    store<u32>(d + 132, load<u32>(g.e + 168));
    store<u32>(d + 136, 0);
    store<u8>(d + 140, 0);
    u32 r = 0;
    const u32 n = static_cast<u32>(call(ctx, kfn::SmallAlloc, {176}));
    if (n != 0) {
        call(ctx, kfn::UiElem8_Construct, {n, d});
        r = n;
    }
    CloneChildren(ctx, g.e, r, g.a, g.deep);
    ret(ctx, r);
}

// [type 9, vtable 0x00B22E98, 192 bytes]: two vec2s.
DECOMP(004E9424, UiElem9_Clone) {
    const Args g = GetArgs(ctx);
    Stack frame(ctx, 432 - 112);
    const u32 d = frame.addr();
    FillDesc(d, g.e, g.name, g.parent, static_cast<u32>(g.a), true);
    Copy(d + 128, g.e + 168, 16);
    u32 r = 0;
    const u32 n = static_cast<u32>(call(ctx, kfn::SmallAlloc, {192}));
    if (n != 0) {
        call(ctx, kfn::UiElem_Construct, {n, d});
        r = n;
        store<u32>(n, kVtblE98);
        Copy(n + 168, d + 128, 16);
    }
    CloneChildren(ctx, g.e, r, g.a, g.deep);
    ret(ctx, r);
}
