// Game code, part 31: helpers shared by obj31*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 53
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj30.h"

namespace sly4 {
namespace obj31 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj30;
using namespace sly4::obj29;
using namespace sly4::obj28;
using namespace sly4::obj27;
using namespace sly4::obj26;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o31fn {
constexpr u32 Obj_FindByHandle = 0x0042C2B0;  // T* (handle, ?)
constexpr u32 Flag_Set8 = 0x0042CA60;         // (o): the shared tail
constexpr u32 Obj_FindComponent = 0x000D1288; // T* (o, kind, ?)
constexpr u32 PtrVector_Reserve = 0x00551718; // (vec, n)
constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 PtrVector_Clear = 0x0054EC1C;
constexpr u32 Nop_54EC18 = 0x0054EC18;
constexpr u32 SmallAlloc = 0x0055238C;        // void* (size)
}  // namespace o31fn

// Raw 32-bit copy of an lfs / stfs pair.
inline void CopyW31(u32 d, u32 s) { store<u32>(d, load<u32>(s)); }

// T::~T() body shared by 0x00786F88 / 0x007876C8: vt 0x00F18D98, +24 = 0;
// unless bit 0x80000000 of +28, the allocator g = 0x011DF31C frees +20
// (g->vt[20](g, +20, (s32)(+28 << 2))); +20 = 0, +28 = 0x80000000, then
// 0x007CDB90(o + 8, g), 0x007CD128(o + 8) and the base vtable 0x00B13974
// (r3 = it).
inline void DtorF18D98_31(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 g = 0x011DF31C;
    store<u32>(o, 0x00F18D98);
    const u32 w = load<u32>(o + 28);
    store<u32>(o + 24, 0);
    if ((w & 0x80000000u) == 0) {
        const u64 n = static_cast<u64>(static_cast<s64>(static_cast<s32>(w << 2)));  // extsw
        const u32 opd = load<u32>(load<u32>(g) + 20);
        call_ptr(ctx, opd, {g, load<u32>(o + 20), n, load<u32>(opd), opd});  // entry in r6, OPD in r7
    }
    store<u32>(o + 20, 0);
    store<u32>(o + 28, 0x80000000u);
    call(ctx, 0x007CDB90, {o + 8, g});
    call(ctx, 0x007CD128, {o + 8});
    store<u32>(o, 0x00B13974);
    ctx.gpr[3] = 0x00B13974;
}

// The u16-counted push tail (count +8, high-water +10): +8 = n + 1 and
// +10 = max(hi, n + 1) with n the count and hi the +10 as read before;
// r3 = hi.
inline void BumpCount31(PPUContext& ctx, u32 v, u32 n, u32 hi) {
    const u32 n1 = n + 1;
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFFu) ? hi : n1));
    ctx.gpr[3] = hi;
}

// The 4x3 transform copy shared by 0x0010B6A8 / 0x00113800: d+0/+16/+32/+48
// rows = s+16/+32/+48/+64 (three words each); f1 = the last value moved.
inline void CopyRows4x3_31(PPUContext& ctx, u32 d, u32 s) {
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW31(d + 16 * r + 4 * k, s + 16 + 16 * r + 4 * k);
    ctx.fpr[1] = LfsAt(s + 72);
}

// void f(u32* mode, T* s, u32 out[2]) of 0x000B1440 / 0x000B1518: g =
// *0x00F64584: g->+7324 = 4056, String_Assign(g + 7232, s + 32); bit 0x100
// or 0x200 of s->+1616: the u8 g->+7348 = 1 and *mode = 2 (else 0); out =
// {0, 1}, g->+7344 = 0 (and g->+7324 = 36 for set36). r3 = 0.
inline void UiSetupName31(PPUContext& ctx, bool set36) {
    const u32 mode = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 g = load<u32>(0x00F64584);
    store<u32>(g + 7324, 4056);
    call(ctx, 0x00555F80, {g + 7232, s + 32});
    const u32 w = load<u32>(s + 1616);
    u32 m = 0;
    if ((w & 0x100u) != 0 || (w & 0x200u) != 0) {
        store<u8>(load<u32>(0x00F64584) + 7348, 1);
        m = 2;
    }
    const u32 g3 = load<u32>(0x00F64584);
    store<u32>(out + 4, 1);
    store<u32>(out, 0);
    store<u32>(mode, m);
    store<u32>(g3 + 7344, 0);
    if (set36) store<u32>(g3 + 7324, 36);
    ctx.gpr[3] = 0;
}

// void f(T* o, x) of 0x000972CC / 0x000974EC / 0x00097708: the target t =
// the kind-5 component c = Effect_FindComponent(x, o->+12) (c->vt[32]())
// whose +28's +276 is 0x02235C33 (t = that +280), else Obj_FindById(o->+16)
// (Hash_ToString(o->+16) when none); then fn(t, o->+20)
inline void TargetCall31(PPUContext& ctx, u32 fn) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, load<u32>(o + 12)}));
    u32 t = 0;
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 c2 = load<u32>(c + 28);
        if (c2 != 0 && load<u32>(c2 + 276) == 0x02235C33u) t = load<u32>(c2 + 280);
    }
    if (t == 0) {
        t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(o + 16)}));
        if (t == 0) {
            ctx.gpr[3] = call(ctx, 0x0056A6E0, {load<u32>(o + 16)});
            return;
        }
    }
    ctx.gpr[3] = call(ctx, fn, {t, load<u32>(o + 20)});
}

// T::~T() of 0x0007DC44 / 0x0007E388 / 0x0007F218: vtable vt; pops the
// pointer vector at o+aoff (u16 count at o+coff) from the back, each
// element getting vt[8](e) (the original's swap-with-last tail only runs
// when a callee emptied the vector: then arr[-1] = arr[0xFFFF], as the
// u16 index wraps); vector cleared, then VtB11B20_DtorTrivial(o)
// (inl: the base destructor inlined as the vtable 0x00B11B20, r3 = it)
inline void DtorPopAll31(PPUContext& ctx, u32 vt, u32 coff, u32 aoff, bool inl = false) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    for (;;) {
        const u32 n = load<u16>(o + coff);
        if (n == 0) break;
        const u32 e = load<u32>(load<u32>(o + aoff) + 4 * (n - 1));
        VCall(ctx, e, 8, {e});
        const u32 n2 = load<u16>(o + coff);
        const u32 m = n2 - 1;
        store<u16>(o + coff, static_cast<u16>(m));
        if (m == (m & 0xFFFFu)) continue;
        const u32 a = load<u32>(o + aoff);
        store<u32>(a + (m << 2), load<u32>(a + ((m << 2) & 0x3FFFCu)));
    }
    call(ctx, o31fn::PtrVector_Clear, {o + aoff});
    call(ctx, o31fn::Nop_54EC18, {o + aoff});
    if (inl) {
        store<u32>(o, 0x00B11B20);
        ctx.gpr[3] = 0x00B11B20;
        return;
    }
    ctx.gpr[3] = call(ctx, 0x004BB41C, {o});
}

// void f(T* o, r4, r5, u8 r6, ?, ?, r9; f1, f2, f3; stack arg) of
// 0x000DC010 / 0x000FCAE4: r = o->vt[slot](o, r4), then o->vt[196](o, r,
// r5, (u8)r6, opd, entry, r9, r10 as left, *sarg; f1..f3) with *sarg the
// caller's stack argument at sp + soff (9: +116, 10: +124), passed on as
// argument 9
inline void VPair196_31(PPUContext& ctx, u32 slot, u32 soff) {
    const u32 o = arg<u32>(ctx, 0), r4 = arg<u32>(ctx, 1), r5 = arg<u32>(ctx, 2), r6 = arg<u32>(ctx, 3) & 0xFFu, r9 = arg<u32>(ctx, 6);
    const f64 f1 = farg(ctx, 0), f2 = farg(ctx, 1), f3 = farg(ctx, 2);
    const u32 sa = load<u32>(static_cast<u32>(ctx.gpr[1]) + soff);
    const u32 r = static_cast<u32>(VCall(ctx, o, slot, {o, r4}));
    ctx.fpr[1] = f1;
    ctx.fpr[2] = f2;
    ctx.fpr[3] = f3;
    const u32 opd = load<u32>(load<u32>(o) + 196);
    const u64 r10 = ctx.gpr[10];  // never set by the original: as the first call left it
    ctx.gpr[3] = call_ptr(ctx, opd, {o, r, r5, r6, opd, load<u32>(opd), r9, r10, sa});
}

}  // namespace obj31
}  // namespace sly4
