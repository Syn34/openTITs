// Game code, part 29: helpers shared by obj29*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 45
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj28.h"

namespace sly4 {
namespace obj29 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj28;
using namespace sly4::obj27;
using namespace sly4::obj26;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o29fn {
constexpr u32 Material_ParamSlot = 0x0050C018;  // (Material*, u32 index) -> u32*
constexpr u32 Iff_ReadU8 = 0x005643C4;          // (IffReader*, u8* out, name)
constexpr u32 FileReader_ReadByte = 0x0056998C; // (reader, u8* out, name)
constexpr u32 Memset = 0x00726E54;              // memset(p, v, n)
constexpr u32 VtB21B58_DtorTrivial = 0x004A0E08;  // (p)
constexpr u32 Obj_BaseDtor_0D7024 = 0x000D7024; // (o): the base destructor (vtable 0x00B12550)
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;  // (cursor, hash, u32* out, u32 default)
}  // namespace o29fn

// Single-precision fnmsubs: -(a * c - b) rounded once.
inline f32 Nmsub29(f64 a, f64 c, f64 b) { return static_cast<f32>(-std::fma(a, c, -b)); }

// Deleting ~T() of a class with a PtrVector at +off whose base holds an
// owned 20-byte PtrVector* at +16 (base vtable 0x00B12260): the vtable, the
// vector, the base's vtable and owned vector (SmallFree(p, 20)), then
// free_fn(o, size). r3 = 0 for null.
inline void DeleteVecVecPtr29(PPUContext& ctx, u32 vt, u32 off, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt);
    VecDtor(ctx, o + off);
    const u32 p = load<u32>(o + 16);
    store<u32>(o, 0x00B12260);
    if (p != 0) {
        VecDtor(ctx, p);
        call(ctx, o19fn::SmallFree, {load<u32>(o + 16), 20});
    }
    ctx.gpr[3] = call(ctx, free_fn, {o, size});
}

// ~T() of the 0x00B12890 class (vtables at +0 / +380 / +384) with an owned
// 20-byte PtrVector* at +388: when it is set, the class vtables, the vector
// and SmallFree(p, 20) (the vtable stores are skipped without one, as the
// original does); then the base set 0x00B12550 / 0x00B1200C / 0x00B12528
// and the base destructor 0x000D7024.
inline void DtorB12890_29(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 388);
    if (p != 0) {
        store<u32>(o + 384, 0x00B12A6C);
        store<u32>(o + 380, 0x00B12A44);
        store<u32>(o, 0x00B12898);
        VecDtor(ctx, p);
        call(ctx, o19fn::SmallFree, {load<u32>(o + 388), 20});
    }
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B12550);
    call(ctx, o29fn::Obj_BaseDtor_0D7024, {o});
}

// The hit handler shape with a constant: when Obj_TestE0BCC(o, h) and the
// f32 q->+qoff (q = o->+216, read first) > *kmin, r = o->vt[352](o, -,
// h->+4, k; h->+52 * that) and bit 0x100 of the u16 h->+14 = (r == 0) (r3 =
// cntlzw(r); kmin when not above)
inline void HitApplyK29(PPUContext& ctx, u32 qoff, u32 kmin, u32 k) {
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
    const u32 r = static_cast<u32>(VCallOpdR4_28(ctx, o, 352, load<u32>(h + 4), k));
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(h + 14, static_cast<u16>((load<u16>(h + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ctx.gpr[3] = z;
}

// Deleting ~T() of the 0x00B16B48 class at o (vtables +0 / +380 0x00B16D1C
// / +384 0x00B16D44): the PtrVectors +576 and +556, the base
// 0x001AE468(o), then SmallFree(o, 608). Nothing for a null o (r3 left).
inline void DeleteB16B48_29(PPUContext& ctx, u32 o) {
    if (o == 0) return;
    store<u32>(o, 0x00B16B48);
    store<u32>(o + 380, 0x00B16D1C);
    store<u32>(o + 384, 0x00B16D44);
    VecDtor(ctx, o + 576);
    VecDtor(ctx, o + 556);
    call(ctx, 0x001AE468, {o});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 608});
}

// void f(T* o; f32 a, f32 b): when the object o->+8 has the flag 0x8 of
// +192 and bit 0 of its +188, Obj_Call563000_00464488(the world slot table
// 0x010B19E0 + 176 + 11968 * the u8 +1 of world slot (s8 +172); a, b)
// (f1 = b, f2 = a and r3 = o->+8 otherwise, as the original leaves them)
inline void ApplySlot29(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    ctx.fpr[1] = b;
    ctx.fpr[2] = a;
    const u32 e = load<u32>(o + 8);
    ctx.gpr[3] = e;
    if (e == 0 || (load<u32>(e + 192) & 0x8u) == 0 || (load<u32>(e + 188) & 1u) == 0) return;
    const u32 s = load<u32>(kWorldPtr) + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(e + 172)))) * 0x20C80u + 4208;
    callf(ctx, 0x00464488, {0x010B19E0u + static_cast<u32>(load<u8>(s + 1)) * 11968u + 176}, {a, b});
}

// VecPush28 with an element size that is not a power of two: fill(data +
// count * size), count + 1 and the high-water mark raised (r3 = the old
// mark).
template <typename F>
inline void VecPushN29(PPUContext& ctx, u32 vec, u32 size, F fill) {
    const u32 cap = load<u16>(vec + 4);
    if (load<u16>(vec + 8) == cap) call(ctx, o27fn::PtrVector_Reserve, {vec, cap << 1});
    fill(load<u32>(vec) + static_cast<u32>(load<u16>(vec + 8)) * size);
    const u32 n = load<u16>(vec + 8) + 1u;
    const u32 m = load<u16>(vec + 10);
    store<u16>(vec + 8, static_cast<u16>(n));
    store<u16>(vec + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFFu) ? m : n));
    ctx.gpr[3] = m;
}

}  // namespace obj29
}  // namespace sly4
