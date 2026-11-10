// Game code, part 28: helpers shared by obj28*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 41
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj27.h"

namespace sly4 {
namespace obj28 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj27;
using namespace sly4::obj26;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o28fn {
constexpr u32 XmlCursor_ReadBool = 0x004F986C;  // (cursor, hash, u8* out, u8 default)
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;  // (cursor, hash, u32* out, u32 default)
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;   // (cursor, hash, f32* out; f32 default)
constexpr u32 Obj_InitVt_B21C98 = 0x004BB3B0;   // (p, owner): the base of the xml-built records
constexpr u32 AnimCmd_ConstructBase = 0x004B9CB8;  // (p)
constexpr u32 List1013B48_Remove = 0x000D68CC;  // (o)
constexpr u32 Queue_Push = 0x004705F8;          // (queue, item, hash, a, b)
}  // namespace o28fn

// The xml-built record: p = Mem_Alloc(16, 0, 16); when allocated,
// init(p, owner) (or init(p) when !with_owner), +12 = 0 (a u8, or a u32
// when wide), vtable vt, read(owner, hash, &p->+12, 0). r3 = p or 0.
inline void NewXml28(PPUContext& ctx, u32 init, bool with_owner, u32 vt, u32 hash, u32 read, bool wide) {
    const u32 owner = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {16, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (with_owner) call(ctx, init, {p, owner});
    else call(ctx, init, {p});
    if (wide) store<u32>(p + 12, 0);
    else store<u8>(p + 12, 0);
    store<u32>(p, vt);
    call(ctx, read, {owner, hash, p + 12, 0});
    ctx.gpr[3] = p;
}

// The xml-built f32 record: p = Mem_Alloc(16, 0, 16); when allocated,
// 0x004BB3B0(p, owner), vtable vt, the f32 +12 = 0, XmlCursor_ReadF32(owner,
// hash, &p->+12; *kdef). r3 = p or 0.
inline void NewXmlF32_28(PPUContext& ctx, u32 vt, u32 hash, u32 kdef) {
    const u32 owner = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {16, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, o28fn::Obj_InitVt_B21C98, {p, owner});
    store<u32>(p, vt);
    const f64 d = LfsAt(kdef);
    store<u32>(p + 12, 0);
    callf(ctx, o28fn::XmlCursor_ReadF32, {owner, hash, p + 12}, {d});
    ctx.gpr[3] = p;
}

// NewXmlF32_28's variant that first sets the f32 +12 to init (raw), then
// the vtable, and loads the default after the stores.
inline void NewXmlF32Init28(PPUContext& ctx, u32 vt, u32 hash, u32 kdef, u32 init) {
    const u32 owner = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {16, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, o28fn::Obj_InitVt_B21C98, {p, owner});
    store<u32>(p + 12, init);
    store<u32>(p, vt);
    callf(ctx, o28fn::XmlCursor_ReadF32, {owner, hash, p + 12}, {LfsAt(kdef)});
    ctx.gpr[3] = p;
}

// NewXml28 with a bool (XmlCursor_ReadBool into the u8 +12).
inline void NewXmlBool28(PPUContext& ctx, u32 init, bool with_owner, u32 vt, u32 hash) {
    NewXml28(ctx, init, with_owner, vt, hash, o28fn::XmlCursor_ReadBool, false);
}

// NewXml28 with a hash (XmlCursor_ReadHash into the u32 +12), base
// 0x004BB3B0(p, owner).
inline void NewXmlHash28(PPUContext& ctx, u32 vt, u32 hash) {
    NewXml28(ctx, o28fn::Obj_InitVt_B21C98, true, vt, hash, o28fn::XmlCursor_ReadHash, true);
}

// The three-vtable deleting destructor (o = this + delta, vtables at +0,
// +380, +384): when o is set, the first set (v0 / v380 / v384), rel(o +
// reloff), the base set (b0 / b380 / b384), VtB12550_Dtor(o), then
// free(o, farg) (r3 = its result).
inline void Delete3Vt28(PPUContext& ctx, s32 delta, u32 v0, u32 v380, u32 v384, u32 rel, u32 reloff, u32 free_fn, u32 farg) {
    const u32 o = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    if (o == 0) return;
    store<u32>(o, v0);
    store<u32>(o + 380, v380);
    store<u32>(o + 384, v384);
    call(ctx, rel, {o + reloff});
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o});
    ctx.gpr[3] = call(ctx, free_fn, {o, farg});
}

// Secondary-base deleting destructor thunk: Delete3Vt28 with
// List1013B48_Remove(o) and Mem_FreeFrom(o, 0).
inline void AdjDelete28(PPUContext& ctx, s32 delta, u32 v0, u32 v380, u32 v384) {
    Delete3Vt28(ctx, delta, v0, v380, v384, o28fn::List1013B48_Remove, 0, o19fn::Mem_FreeFrom, 0);
}

// bool f(T* o, f32 a, f32 b): true unless the flag mask (0x08) of the u8 o->+boff
// is set, bit 0x20000000 of o->+552 is clear and (b < *kmin or a >= b);
// then o->vt[420](o, 2, 0) and false.
inline void RangeGate28(PPUContext& ctx, u32 boff, u32 kmin, u32 mask = 0x08u) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 vt = load<u32>(o);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    if ((load<u8>(o + boff) & mask) == 0 || (load<u32>(o + 552) & 0x20000000u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    if (b < LfsAt(kmin) || a >= b) {
        VCallVt(ctx, vt, 420, {o, 2, 0});
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = 1;
}

// The inlined random step on the global 0x01164888 (+0 u64 last value,
// +8 u32 seed): the seed becomes rotl16((rotl4(s) + rotl16(s)) ^
// 0x87654321) and the u64 its low 16 bits. Returns the old u64.
inline u64 RandStep28() {
    const u64 old = load<u64>(0x01164888);
    const u32 s = load<u32>(0x01164890);
    const u32 x = (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u;
    store<u32>(0x01164890, std::rotl(x, 16));
    store<u64>(0x01164888, x & 0xFFFFu);
    return old;
}

// Single-precision helpers: fmuls / fadds / fsubs / fmadds on doubles.
inline f32 Mul28(f64 a, f64 b) { return static_cast<f32>(a * b); }
inline f32 Sub28(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 Add28(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 Madd28(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }

// The inlined u16-counted vector push on the vector at vec (+0 data, +4
// u16 capacity, +8 u16 count, +10 u16 high-water mark): doubles it with
// PtrVector_Reserve when full, fill(data + (count << shift)), count + 1 and
// the mark raised to it (r3 = the old mark).
template <typename F>
inline void VecPush28(PPUContext& ctx, u32 vec, u32 shift, F fill) {
    const u32 cap = load<u16>(vec + 4);
    if (load<u16>(vec + 8) == cap) call(ctx, o27fn::PtrVector_Reserve, {vec, cap << 1});
    fill(load<u32>(vec) + (static_cast<u32>(load<u16>(vec + 8)) << shift));
    const u32 n = load<u16>(vec + 8) + 1u;
    const u32 m = load<u16>(vec + 10);
    store<u16>(vec + 8, static_cast<u16>(n));
    store<u16>(vec + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFFu) ? m : n));
    ctx.gpr[3] = m;
}

// Raw 32-bit copy of a 12-byte vector (lfs / stfs pairs); f1 = the last.
inline void CopyVec3F28(PPUContext& ctx, u32 d, u32 s) {
    store<u32>(d, load<u32>(s));
    store<u32>(d + 4, load<u32>(s + 4));
    const u32 z = load<u32>(s + 8);
    store<u32>(d + 8, z);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));
}

inline f64 F32Bits28(u32 w) { return static_cast<f64>(std::bit_cast<f32>(w)); }

// o->vt[slot](o, -, a, b) where the original leaves the OPD in r4 (an
// unused argument) and its entry in r7.
inline u64 VCallOpdR4_28(PPUContext& ctx, u32 o, u32 slot, u32 a, u32 b) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return call_ptr(ctx, opd, {o, opd, a, b, load<u32>(opd)});
}

// u16 f(T* o): (o->vt[slot]()->+off) >> 16 when the world list
// kWorldPtr->* + 0x1281D4 has a +4 and o->vt[slot]() is set (called twice),
// else dflt
inline void VtFieldHi28(PPUContext& ctx, u32 slot, u32 off, u32 dflt) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = dflt;
    if (load<u32>(load<u32>(load<u32>(kWorldPtr) + 0x1281D4) + 4) == 0) return;
    if (static_cast<u32>(VCall(ctx, o, slot, {o})) == 0) {
        ctx.gpr[3] = dflt;
        return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, o, slot, {o}));
    ctx.gpr[3] = load<u32>(r + off) >> 16;
}

// The hit handler shape: when Obj_TestE0BCC(o, h) and o->+216's f32 +qoff
// > *kmin, r = o->vt[352](o, -, h->+4, h->+8; h->+52 * that) and bit 0x100
// of the u16 h->+14 = (r == 0) (r3 = cntlzw(r); kmin when not above)
inline void HitApply28(PPUContext& ctx, u32 qoff, u32 kmin) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 q = load<u32>(o + 216);
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, h})) == 0) return;
    const f64 v = LfsAt(q + qoff);
    ctx.fpr[1] = v;
    if (!(v > LfsAt(kmin))) {
        ctx.gpr[3] = kmin;
        return;
    }
    ctx.fpr[1] = static_cast<f64>(Mul28(LfsAt(h + 52), v));
    const u32 r = static_cast<u32>(VCallOpdR4_28(ctx, o, 352, load<u32>(h + 4), load<u32>(h + 8)));
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(h + 14, static_cast<u16>((load<u16>(h + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ctx.gpr[3] = z;
}

}  // namespace obj28
}  // namespace sly4
