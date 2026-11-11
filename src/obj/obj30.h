// Game code, part 30: helpers shared by obj30*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 49
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj29.h"

namespace sly4 {
namespace obj30 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj29;
using namespace sly4::obj28;
using namespace sly4::obj27;
using namespace sly4::obj26;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o30fn {
constexpr u32 Table_Contains = 0x0044D6D8;    // bool (const T* p)
constexpr u32 PushStack_Push = 0x0044D5BC;    // (stack; f32)
constexpr u32 PushStack_Remove = 0x0044D614;  // (stack; f32)
constexpr u32 CmdList_Push = 0x0042C138;      // (cmd)
constexpr u32 Handles_Destroy = 0x0042725C;   // (h)
constexpr u32 View_ProjectPoint = 0x00522750; // (view, f32 out[2..], const Vec3*)
constexpr u32 Obj_ChildrenUpdate = 0x000DA25C;  // (o; f32 dt)
constexpr u32 Thunk_E44C0 = 0x000E44C0;       // (a, b): the base copy
constexpr u32 Thunk_E02D8 = 0x000E02D8;       // (o)
constexpr u32 Volume_Notify = 0x000112D8;     // (list, node)
constexpr u32 StateMachine_SetState = 0x00464D1C;  // (o, state, arg)
}  // namespace o30fn

constexpr u32 kGame30 = 0x010B19E0;  // g::game (the Game object pointer)

// o->vt[slot](o, opd, a; f1) where the original leaves the OPD in r4 and
// the entry address in r6 (a in r5 untouched).
inline u64 VCallOpdR4E6_30(PPUContext& ctx, u32 o, u32 slot, u32 a) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return call_ptr(ctx, opd, {o, opd, a, load<u32>(opd)});
}

// Copies n bytes s -> d one byte at a time (lbz / stb pairs), low first.
inline void CopyBytes30(u32 d, u32 s, u32 n) {
    for (u32 k = 0; k < n; ++k) store<u8>(d + k, load<u8>(s + k));
}

// void f(T* o, u8 on): when bit 31 of o->+412 differs from on, sets it to
// bit 0 of the r4 passed in; then, for on, 0x0020C2BC(o); for off,
// Volume_Notify(*0x00F557EC, o + 424), 0x0020C670(o) unless bit
// 0x02000000 of +412 and o->vt[96](o, 0). r3 = o->+412 as read when
// unchanged.
inline void SetFlag412_30(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0), r4 = arg<u32>(ctx, 1), on = r4 & 0xFFu;
    const u32 w = load<u32>(o + 412);
    ctx.gpr[3] = w;
    if ((w >> 31) == on) return;
    store<u32>(o + 412, (w & 0x7FFFFFFFu) | (r4 << 31));
    if (on != 0) {
        ctx.gpr[3] = call(ctx, 0x0020C2BC, {o});
        return;
    }
    call(ctx, o30fn::Volume_Notify, {load<u32>(0x00F557EC), o + 424});
    if ((load<u32>(o + 412) & 0x02000000u) == 0) call(ctx, 0x0020C670, {o});
    ctx.gpr[3] = VCall(ctx, o, 96, {o, 0});
}

// T::~T() of the 0x00B23050 class: the three SmallFree'd buffers +12 / +20
// / +24 (sizes 4 * d->+8->+48, 4 * d->+28 and 4 * d->+8->+48 of the
// descriptor d = o->+16, reloaded after each free) cleared. r3 = 0.
inline void DtorB23050_30(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    u32 d = load<u32>(o + 16);
    store<u32>(o, 0x00B23050);
    u32 a = load<u32>(o + 12);
    u32 n = load<u32>(load<u32>(d + 8) + 48) << 2;
    if (a != 0) {
        call(ctx, o19fn::SmallFree, {a, n});
        d = load<u32>(o + 16);
        store<u32>(o + 12, 0);
    }
    n = load<u32>(d + 28) << 2;
    a = load<u32>(o + 20);
    if (a != 0) {
        call(ctx, o19fn::SmallFree, {a, n});
        d = load<u32>(o + 16);
        store<u32>(o + 20, 0);
    }
    const u32 t = load<u32>(d + 8);
    a = load<u32>(o + 24);
    n = load<u32>(t + 48) << 2;
    if (a != 0) {
        call(ctx, o19fn::SmallFree, {a, n});
        store<u32>(o + 24, 0);
    }
    ctx.gpr[3] = 0;
}

// T* f(?, XmlCursor* x): Mem_Alloc(20, 0, 16) built by 0x004BB3B0(p, x)
// as the vt record with the hash h1 (+12) and the bool h2 (u8 +16) read
// from x (the bool's default is the current u8 when def_cur, else 0).
// r3 = p or 0.
inline void NewXmlHashBool30(PPUContext& ctx, u32 vt, u32 h1, u32 h2, bool def_cur) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {20, 0, 16}));
    if (p != 0) {
        call(ctx, o28fn::Obj_InitVt_B21C98, {p, x});
        store<u32>(p + 12, 0);
        store<u8>(p + 16, 0);
        store<u32>(p, vt);
        call(ctx, o28fn::XmlCursor_ReadHash, {x, h1, p + 12, 0});
        call(ctx, o28fn::XmlCursor_ReadBool, {x, h2, p + 16, def_cur ? load<u8>(p + 16) : 0u});
    }
    ctx.gpr[3] = p;
}

// void f(T* o; f32 v): the pointer-to-member {fn, adj} at table[s] (8-byte
// entries) for s = o->vt[32](o, 1): when fn is set, (o + adj)->*fn(v)
// (fn odd: a virtual slot fn - 1 of that object's vtable, else an OPD);
// r3 = adj when fn is null
inline void PmfDispatch30(PPUContext& ctx, u32 table) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0);
    const u32 s = static_cast<u32>(VCall(ctx, o, 32, {o, 1}));
    const u32 e = table + (s << 3);
    const u32 fn = load<u32>(e), adj = load<u32>(e + 4);
    ctx.gpr[3] = adj;
    if (fn == 0) return;
    const u32 t = o + adj;
    const u32 opd = (fn & 1u) != 0 ? load<u32>(load<u32>(t) + fn - 1) : fn;
    ctx.fpr[1] = v;
    call_ptr(ctx, opd, {t, load<u32>(opd)});
}

// The xml-built record: p = Mem_Alloc(size, 0, 16); when allocated,
// init(p, x) (or init(p) when !with_x) and body(p, x) (its default fields,
// vtable and XmlCursor reads). r3 = p or 0.
template <typename F>
inline void NewXml30(PPUContext& ctx, u32 size, u32 init, bool with_x, F body) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {size, 0, 16}));
    if (p != 0) {
        if (with_x) call(ctx, init, {p, x});
        else call(ctx, init, {p});
        body(p, x);
    }
    ctx.gpr[3] = p;
}

// The attach tail of 0x00201FC8 / 0x00202098 (flags = o->+592): unless
// bit 0x40000000 is already set or bit 0x80000000 clear (and, for
// chk_extra, the u8 0x01054890 is set while bit 0x02000000 of r6 is
// clear), with a target
// t = h->+4 whose u64 +184 has bit 0 and not bit 0x100: bit 0x40000000 of
// +592 set and, when o->+564 is free, o->+564 = t and t's u16 +178
// counted up (r3 = the new count; o otherwise)
inline void AttachTarget30(PPUContext& ctx, u32 o, u32 h, u32 f, bool chk_extra, u32 r6) {
    ctx.gpr[3] = o;
    if ((f & 0x40000000u) != 0 || (f & 0x80000000u) == 0) return;
    if (chk_extra && load<u8>(0x01054890) != 0 && (r6 & 0x02000000u) == 0) return;
    const u32 t = load<u32>(h + 4);
    if (t == 0) return;
    const u64 d = load<u64>(t + 184);
    if ((d & 1u) == 0 || (d & 0x100u) != 0) return;
    store<u32>(o + 592, f | 0x40000000u);
    const u32 t2 = load<u32>(h + 4);
    if (t2 == 0 || load<u32>(o + 564) != 0) return;
    store<u32>(o + 564, t2);
    const u32 n = load<u16>(t2 + 178) + 1u;
    store<u16>(t2 + 178, static_cast<u16>(n));
    ctx.gpr[3] = n;
}

}  // namespace obj30
}  // namespace sly4
