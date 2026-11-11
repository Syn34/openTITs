// Game code, part 32: helpers shared by obj32*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 57
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj31.h"

namespace sly4 {
namespace obj32 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj31;
using namespace sly4::obj30;
using namespace sly4::obj29;
using namespace sly4::obj28;
using namespace sly4::obj27;
using namespace sly4::obj26;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o32fn {
constexpr u32 Mem_FreeFrom = 0x0054EE6C;  // (p, 0)
constexpr u32 SmallFree = 0x00552420;     // (p, size)
}  // namespace o32fn

// Raw 32-bit copy of an lfs / stfs pair.
inline void CopyW32(u32 d, u32 s) { store<u32>(d, load<u32>(s)); }

// f32 bits at ea as a double (an lfs).
inline f64 Lfs32(u32 ea) { return static_cast<f64>(std::bit_cast<f32>(load<u32>(ea))); }

// stfs of a double.
inline void Stfs32(u32 ea, f64 v) { store<u32>(ea, std::bit_cast<u32>(static_cast<f32>(v))); }

// "if (o->+off) { Mem_FreeFrom(o->+off, 0); o->+off = 0; }" as the
// destructors here write it.
inline void FreeField32(PPUContext& ctx, u32 o, u32 off) {
    const u32 p = load<u32>(o + off);
    if (p == 0) return;
    call(ctx, o32fn::Mem_FreeFrom, {p, 0});
    store<u32>(o + off, 0);
}

// PtrVector_Clear(v) then the no-op 0x0054EC18(v), as the destructors pair
// them.
inline void PvClear32(PPUContext& ctx, u32 v) {
    call(ctx, 0x0054EC1C, {v});
    call(ctx, 0x0054EC18, {v});
}

// divw as the simulator / the Cell give it: 0 for a zero divisor, wrapping
// for INT_MIN / -1.
inline u32 Divw32(u32 a, u32 b) {
    const s32 x = static_cast<s32>(a), y = static_cast<s32>(b);
    if (y == 0) return 0;
    if (y == -1) return 0u - a;
    return static_cast<u32>(x / y);
}

// The inlined random step of RandStep28 as an f32: (f32)(s64)old.
inline f64 RandF32_32() { return static_cast<f32>(static_cast<f64>(static_cast<s64>(RandStep28()))); }

// T* f(?, Xml* x) of 0x00082DCC / 0x000831A4: a 20-byte Mem_Alloc(20, 0,
// 16) object (0x004BB3B0(p), vtable vt, s32 +12 = 0, u8 +16 = 0), the bool
// 0x7A281983 into +16 and the s32 0x368328E5 into +12 (0 unless 0..8).
// r3 = p.
inline void XmlNewEnum9_32(PPUContext& ctx, u32 vt) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {20, 0, 16}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    call(ctx, 0x004BB3B0, {p, x});
    store<u32>(p + 12, 0);
    store<u8>(p + 16, 0);
    store<u32>(p, vt);
    call(ctx, 0x004F986C, {x, 0x7A281983u, p + 16, 0});
    const u64 d = static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(p + 12))));
    call(ctx, 0x004F99BC, {x, 0x368328E5u, p + 12, d});
    const s32 v = static_cast<s32>(load<u32>(p + 12));
    if (v < 0 || v >= 9) store<u32>(p + 12, 0);
    ctx.gpr[3] = p;
}

// Mem_Alloc(size, 0, 16) and 0x004BB3B0(p) for the XML node factories;
// 0 for a failed allocation.
inline u32 XmlNodeAlloc32(PPUContext& ctx, u32 size, u32 x) {
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {size, 0, 16}));
    if (p != 0) call(ctx, 0x004BB3B0, {p, x});
    return p;
}

// T* f(?, Xml* x) of 0x00096C2C / 0x000971E0 / 0x00097400: a 24-byte node
// (vtable vt, +12/+16/+20 = 0) reading the hashes 0x7F247172 -> +12,
// 0x7A30D7E3 -> +16 and h3 -> +20 (defaults: the current values). r3 = p.
inline void XmlHash3_32(PPUContext& ctx, u32 vt, u32 h3) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = XmlNodeAlloc32(ctx, 24, x);
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, vt);
    call(ctx, 0x004FA14C, {x, 0x7F247172u, p + 12, 0});
    call(ctx, 0x004FA14C, {x, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14C, {x, h3, p + 20, load<u32>(p + 20)});
    ctx.gpr[3] = p;
}

// T* f(?, Xml* x) of 0x00096248 / 0x000A6CF4: a 24-byte node (vtable vt,
// +12/+16 = 0, u8 +20 = 0) reading the hashes 0x08F06AD0 -> +12 and
// 0x7E6EFA38 -> +16 and the bool 0x06FA4B5F -> +20 (defaults: the
// current values). r3 = p.
inline void XmlHash2Bool_32(PPUContext& ctx, u32 vt) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = XmlNodeAlloc32(ctx, 24, x);
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 0);
    store<u32>(p, vt);
    call(ctx, 0x004FA14C, {x, 0x08F06AD0u, p + 12, 0});
    call(ctx, 0x004FA14C, {x, 0x7E6EFA38u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004F986C, {x, 0x06FA4B5Fu, p + 20, load<u8>(p + 20)});
    ctx.gpr[3] = p;
}

// ~T() of 0x000C2BB0 / 0x000C2DA0 (vt 0x00B12230): the +128 buffer freed
// (Mem_FreeFrom) and cleared, Obj_Register7_00490518(o), the id o->+52
// removed from the map 0x01013B04 (SortedMap_Remove744748(map, &id)), and
// the 20-byte vector objects +136 / +16 cleared and SmallFree'd. r3 as the
// last call left it.
inline void DtorB12230_32(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 key = frame.addr();  // sp+112
    store<u32>(o, 0x00B12230);
    FreeField32(ctx, o, 128);
    call(ctx, 0x00490518, {o});
    store<u32>(key, load<u32>(o + 52));
    call(ctx, 0x00744748, {0x01013B04u, key});
    for (const u32 off : {136u, 16u}) {
        const u32 v = load<u32>(o + off);
        if (v == 0) continue;
        PvClear32(ctx, v);
        call(ctx, o32fn::SmallFree, {load<u32>(o + off), 20});
    }
}

// The record table lookup shared by 0x00393944 / 0x00394D28 / 0x003957DC:
// the address of entry (u16 0x0106CF70 + 4 * the byte at u16 0x0106CF7E of
// record a->+2, byte 0 when that record's +2 is 0) of record a->+4 in the
// table 0x00F65038->+24 (128-byte records; index -1 reads as address 0).
inline u32 RefEntry32(u32 a) {
    const u32 g = load<u32>(0x00F65038);
    const s32 i1 = static_cast<s16>(load<u16>(a + 4));
    u32 r1 = 0;
    if (i1 != -1) r1 = load<u32>(g + 24) + (static_cast<u32>(i1) << 7);
    const s32 i2 = static_cast<s16>(load<u16>(a + 2));
    u32 r2 = 0;
    if (i2 != -1) r2 = load<u32>(g + 24) + (static_cast<u32>(i2) << 7);
    u32 at = 0;
    if (load<u8>(r2 + 2) != 0) at = r2 + load<u16>(0x0106CF7E);
    return r1 + load<u16>(0x0106CF70) + (static_cast<u32>(load<u8>(at)) << 2);
}

// Table-driven form of the small XML node factories T* f(?, Xml* x): a
// Mem_Alloc(size, 0, 16) node built by ctor (0x004BB3B0(p, x), or
// AnimCmd_ConstructBase 0x004B9CB8(p)), its fields initialised in the
// listed order ('w' / 'b': u32 / u8 0, 'W' / 'B': u32 / u8 1, 'v': the
// vtable), then the listed reads: 'H' XmlCursor_ReadHash, 'B'
// XmlCursor_ReadBool, 'S' XmlCursor_ReadS32 (sign-extended default), 'F'
// XmlCursor_ReadF32 (f1 default), the default being the field's current
// value (cur) or 0. r3 = p (0 for a failed allocation).
struct XmlInit32 {
    u16 off;
    char k;
    u32 val = 0;  // 'C': u32 val
};
struct XmlRead32 {
    char k;
    u32 hash;
    u16 off;
    bool cur;
};
inline void XmlGen32(PPUContext& ctx, u32 size, u32 vt, std::initializer_list<XmlInit32> init, std::initializer_list<XmlRead32> rd,
                     u32 ctor = 0x004BB3B0) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {size, 0, 16}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    if (ctor == 0x004BB3B0) call(ctx, ctor, {p, x});
    else call(ctx, ctor, {p});
    for (const XmlInit32& i : init) {
        switch (i.k) {
        case 'w': store<u32>(p + i.off, 0); break;
        case 'W': store<u32>(p + i.off, 1); break;
        case 'b': store<u8>(p + i.off, 0); break;
        case 'B': store<u8>(p + i.off, 1); break;
        case 'C': store<u32>(p + i.off, i.val); break;
        default: store<u32>(p, vt); break;
        }
    }
    for (const XmlRead32& r : rd) {
        const u32 f = p + r.off;
        switch (r.k) {
        case 'H': call(ctx, 0x004FA14C, {x, r.hash, f, r.cur ? load<u32>(f) : 0u}); break;
        case 'B': call(ctx, 0x004F986C, {x, r.hash, f, r.cur ? load<u8>(f) : 0u}); break;
        case 'S': call(ctx, 0x004F99BC, {x, r.hash, f, r.cur ? static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(f)))) : 0u}); break;
        default: callf(ctx, 0x004F9B20, {x, r.hash, f}, {Lfs32(f)}); break;
        }
    }
    ctx.gpr[3] = p;
}

// The speed-gated hit reaction shared by 0x001856B0 / 0x000E8310: bit 0x100
// of the u16 m->+14 = !o->vt[352](o, -, m->+4, key; f1) (entry in r4, OPD
// in r7); r3 = the cntlzw of the result.
inline void HitReact32(PPUContext& ctx, u32 o, u32 m, u32 key) {
    const u32 opd = load<u32>(load<u32>(o) + 352);
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, load<u32>(opd), load<u32>(m + 4), key, opd}));
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(m + 14, static_cast<u16>((load<u16>(m + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ctx.gpr[3] = z;
}

// The per-entry broadcast of 0x00370130 / 0x00374704: o->+24 (when set)
// gets vt[40](it, on, 1); then each entry of q = o->+8 (count q->+cnt,
// re-read after each call; array q->+arr of stride-byte entries) with a
// field +fld object e gets e->vt[88](e, on) (arrInO: the array is
// o->+arr instead). r3 = the count.
inline void OnEach32(PPUContext& ctx, u32 cnt, u32 arr, u32 stride, u32 fld, bool arrInO = false) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 a = load<u32>(o + 24);
    if (a != 0) VCall(ctx, a, 40, {a, on, 1});
    const u32 q = load<u32>(o + 8);
    u32 n = load<u32>(q + cnt);
    ctx.gpr[3] = n;
    for (u32 i = 0; n != 0 && i < n; ++i) {
        const u32 e = load<u32>(load<u32>((arrInO ? o : q) + arr) + stride * i + fld);
        if (e == 0) continue;
        VCall(ctx, e, 88, {e, on});
        n = load<u32>(q + cnt);
        ctx.gpr[3] = n;
    }
}

// The sound-voice command of 0x0044CB18 / 0x0044CC08: for a live handle h
// (u8 0x010AEC60 set, slot h >> 16 below the table 0x010B0200's +24
// count, the slot's (+256 >> 15) & 0xFFFF generation equal to h & 0xFFFF
// and flag 0x80 of its u8 +256 clear) a 220-byte zeroed command {220,
// kind, ..., +off = val} goes to 0x006D612C(slot->+16, &cmd). r3 = 1 for a
// flagged slot; otherwise as the checks left it.
inline void VoiceCmd32(PPUContext& ctx, u32 kind, u32 off, u32 val) {
    const u32 h = arg<u32>(ctx, 0);
    if (load<u8>(0x010AEC60) == 0) return;
    const u32 lo = h & 0xFFFFu, hi = h >> 16;
    ctx.gpr[3] = hi;
    if (lo == 0) return;
    if (static_cast<s32>(hi) >= static_cast<s32>(load<u32>(0x010B0218))) return;
    const u32 e = load<u32>(0x010B021C) + 272 * hi;
    const u32 p = e + 256;
    ctx.gpr[3] = p;
    if (((load<u32>(p) >> 15) & 0xFFFFu) != lo) return;
    ctx.gpr[3] = (load<u8>(p) >> 7) & 1u;
    if (ctx.gpr[3] != 0) return;
    Stack frame(ctx, 224);
    const u32 cmd = frame.addr();  // sp+112
    call(ctx, 0x00726E54, {cmd, 0, 220});
    store<u32>(cmd + off, val);
    store<u32>(cmd, 220);
    store<u32>(cmd + 4, kind);
    ctx.gpr[3] = call(ctx, 0x006D612C, {load<u32>(e + 16), cmd});
}

// The scaled hit reaction of 0x000E8310 / 0x00168810 / 0x00184D20 /
// 0x00192610: for a hit passing Obj_TestE0BCC(o, m), with the tuning st =
// o->+216 (scale +os, threshold +os+4, cap +os+8; scale and cap above K)
// and m->+48 above the threshold: HitReact32 with f1 = min((m->+48 -
// threshold) * scale, cap) and key 0x80F43146. r3 = o's vtable otherwise.
inline void HitScaled32(PPUContext& ctx, u32 kaddr, u32 os) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 st = load<u32>(o + 216);
    const u32 r = static_cast<u32>(call(ctx, 0x000E0BCC, {o, m}));
    ctx.gpr[3] = load<u32>(o);
    if (r == 0) return;
    const f64 k = Lfs32(kaddr);
    const f64 sc = Lfs32(st + os);
    ctx.fpr[1] = k;
    ctx.fpr[2] = sc;
    if (!(sc > k)) return;
    ctx.fpr[3] = k;
    const f64 cap = Lfs32(st + os + 8);
    ctx.fpr[1] = cap;
    if (!(cap > k)) return;
    const f64 v = Lfs32(m + 48), th = Lfs32(st + os + 4);
    ctx.fpr[4] = v;
    ctx.fpr[3] = th;
    if (!(v > th)) return;
    const f64 d = static_cast<f32>(v - th);
    const f64 pr = static_cast<f32>(d * sc);
    ctx.fpr[3] = d;
    ctx.fpr[2] = pr;
    if (pr < cap) ctx.fpr[1] = pr;
    HitReact32(ctx, o, m, 0x80F43146u);
}

// Whether the voice handle h is live (see VoiceCmd32): r3 = 1 when the
// table is enabled, the slot is in range with a matching generation and
// flag 0x80 clear, else 0.
inline u32 VoiceLive32(u32 h) {
    if (load<u8>(0x010AEC60) == 0) return 0;
    const u32 lo = h & 0xFFFFu, hi = h >> 16;
    if (lo == 0 || static_cast<s32>(hi) >= static_cast<s32>(load<u32>(0x010B0218))) return 0;
    const u32 p = load<u32>(0x010B021C) + 272 * hi + 256;
    if (((load<u32>(p) >> 15) & 0xFFFFu) != lo) return 0;
    return ((load<u8>(p) >> 7) & 1u) != 0 ? 0u : 1u;
}

}  // namespace obj32
}  // namespace sly4
