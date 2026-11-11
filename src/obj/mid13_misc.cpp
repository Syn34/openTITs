// Game code, medium sweep part 13: assorted XML element readers.
#include "mid13.h"

using namespace sly4;
using namespace sly4::mid13;

namespace {
constexpr u32 String_Dup = 0x00556038;  // (const char*) -> char*
}  // namespace

// u32 HashList_Init4CBB40(HashList* o, XmlCursor* c)
// (called by New_004CBE40) u16 o+4 = 1, vtable 0x00B22498, the PtrVector
// o+12 (capacity 4); +32..+44 0, byte +48 0. Reads hashes 0x04B8CC71 /
// 0x698014C7 / 0x51C56A57 into o+32..+40, a float (0x8A30A2B0) into o+44
// and a bool (0x8FA5D8C3) into byte o+48 (each defaulting to the field);
// when that is set, the name hash (0x0024728B, default 0) of each child
// 0x04B8CC71 is appended to the vector. Returns 0.
DECOMP(004CBB40, HashList_Init4CBB40) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B22498u);
    const u32 v = o + 12;
    mid1::PtrVecNew(ctx, v, 4);
    for (u32 k = 32; k <= 44; k += 4) store<u32>(o + k, 0);
    store<u8>(o + 48, 0);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x04B8CC71u, o + 32, 0});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x698014C7u, o + 36, load<u32>(o + 36)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x51C56A57u, o + 40, load<u32>(o + 40)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x8A30A2B0u, o + 44}, {LoadF(o + 44)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x8FA5D8C3u, o + 48, load<u8>(o + 48)});
    if (load<u8>(o + 48) != 0) {
        Stack frame(ctx, 16);
        const u32 h = frame.addr();  // the original's sp + 112
        for (u32 e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, 0x04B8CC71u})); e != 0;
             e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {e, 0x04B8CC71u}))) {
            store<u32>(h, 0);
            call(ctx, xfn::XmlCursor_ReadHash, {e, 0x0024728Bu, h, 0});
            mid1::PtrVecPush(ctx, v, load<u32>(h));
        }
    }
    ret(ctx, 0u);
}

// XmlCursor* ColorSet_Read47F108(a0, XmlCursor* c, ColorSet* o)
// (called by 00481A88) Hashes 0x1285C1F8 / 0x5CEEFBCB / 0x760CB725 into
// o+0..+8 and colors 0xEAB9B1D2 / 0xEAB9B1D3 into o+12 / +16 (each
// defaulting to the field); then for up to four children 0x0353CB09 a
// 16-byte entry at o+20 + 16i: +0 a copy (String_Dup) of the string
// 0x6A73C305 (default 0x00AE65E4) when not empty, hashes 0x0024728B /
// 0x31F8E7E5 / 0xB2E59A9F into +4..+12 (default 0). Returns the child
// cursor where it stopped (0 after the last).
DECOMP(0047F108, ColorSet_Read47F108) {
    const u32 c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x1285C1F8u, o, load<u32>(o)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x5CEEFBCBu, o + 4, load<u32>(o + 4)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x760CB725u, o + 8, load<u32>(o + 8)});
    call(ctx, xfn::XmlCursor_ReadColor, {c, 0xEAB9B1D2u, o + 12, o + 12});
    call(ctx, xfn::XmlCursor_ReadColor, {c, 0xEAB9B1D3u, o + 16, o + 16});
    u32 e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, 0x0353CB09u}));
    if (e != 0) {
        Stack frame(ctx, 16);
        const u32 sp = frame.addr();  // the original's sp + 112
        u32 q = o;
        for (u32 i = 0; e != 0 && i < 4; ++i, q += 16) {
            store<u32>(sp, 0);
            call(ctx, mid2::m2fn::XmlCursor_ReadString_004FA2AC, {e, 0x6A73C305u, sp, 0x00AE65E4u});
            const u32 s = load<u32>(sp);
            if (load<u8>(s) != 0) store<u32>(q + 20, static_cast<u32>(call(ctx, String_Dup, {s})));
            call(ctx, xfn::XmlCursor_ReadHash, {e, 0x0024728Bu, q + 24, 0});
            call(ctx, xfn::XmlCursor_ReadHash, {e, 0x31F8E7E5u, q + 28, 0});
            call(ctx, xfn::XmlCursor_ReadHash, {e, 0xB2E59A9Fu, q + 32, 0});
            e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {e, 0x0353CB09u}));
        }
    }
    ret(ctx, e);
}

namespace {
constexpr u32 kSysState = 0x011DE094;       // function-local static (gfx12 shutdown_holder): +16 f32, +20 / +24, u8 +32 flags, +92 400 bytes
constexpr u32 kSysStateReady = 0x011DE280;  // u8: its guard
constexpr u32 kSysStatePtr = 0x011972E8;    // u32: = kSysState once built

// The inlined first-use construction of kSysState.
void SysStateGet(PPUContext& ctx) {
    if (load<u8>(kSysStateReady) != 0) return;
    const u32 g = kSysState;
    const u32 fl = load<u8>(g + 32) & 0x3Fu;
    store<u32>(g + 20, 0);
    store<u32>(g + 24, 0);
    store<u8>(g + 32, static_cast<u8>(fl));
    call(ctx, mid2::m2fn::memset, {g + 92, 0, 400});
    store<u32>(kSysStatePtr, g);
    store<u8>(kSysStateReady, 1);
}
}  // namespace

// u32 CamParams_Init4C0080(CamParams* o, XmlCursor* c)
// (called by New68_004C09D8) u16 o+4 = 1, vtable 0x00B21F40, o->24 = 8;
// f32 o+28 = the system state's +16 (built on first use); the name hash
// (0x0024728B) into o+8; the type hash 0x0027873A mapped to o->24
// (0x000140B8 0, 0x00010567 1, 0x923F4D1D 2, 0x87EBFA08 3, 0xB5B1087F 4,
// 0x6845C350 5, 0x03D53F43 6, 0x00239977 7; else unchanged); floats
// 0xF1E9AC63 / 0xA1EB953C / 0x0489115B / 0x8ACA5893 / 0x04DA9951 into
// o+36..+52 and 0x00274ACD into o+64 (1.0 when its magnitude is below the
// pool's +8), then 0x04B8CC42 / 0x00010CBB into o+56 / +60 (defaults the
// pool 0x004C0070 +0 / +4).
DECOMP(004C0080, CamParams_Init4C0080) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B21F40u);
    store<u32>(o + 24, 8);
    SysStateGet(ctx);
    CopyW(o + 28, kSysState + 16);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x0024728Bu, o + 8, 0});
    {
        Stack frame(ctx, 16);
        const u32 h = frame.addr();  // the original's sp + 112
        if (call(ctx, xfn::XmlCursor_ReadHash, {c, 0x0027873Au, h, 0}) == 0) {
            static constexpr u32 kTypes[8] = {0x000140B8u, 0x00010567u, 0x923F4D1Du, 0x87EBFA08u,
                                              0xB5B1087Fu, 0x6845C350u, 0x03D53F43u, 0x00239977u};
            for (u32 k = 0; k < 8; ++k)
                if (load<u32>(h) == kTypes[k]) {
                    store<u32>(o + 24, k);
                    break;
                }
        }
    }
    constexpr u32 kPool = 0x004C0070;
    const f64 a = LoadF(kPool), b = LoadF(kPool + 4);
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xF1E9AC63u, o + 36}, {a});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xA1EB953Cu, o + 40}, {b});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x0489115Bu, o + 44}, {b});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x8ACA5893u, o + 48}, {b});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x04DA9951u, o + 52}, {b});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x00274ACDu, o + 64}, {a});
    if (std::fabs(LoadF(o + 64)) < LoadF(kPool + 8)) store<u32>(o + 64, 0x3F800000u);
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x04B8CC42u, o + 56}, {a});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x00010CBBu, o + 60}, {b});
}

namespace {
// f32 at dst = the f32 at src clamped to [lo, hi] (as the original's
// compare chain: min(v, hi) when above lo, else lo; a NaN gives hi or lo),
// stored raw; f1 left as the original leaves it.
void ClampF(PPUContext& ctx, u32 dst, u32 lo, u32 hi) {
    const u32 v = load<u32>(dst), h = load<u32>(hi);
    const u32 m = LfsF(v) < LfsF(h) ? v : h;
    if (LfsF(m) > LfsF(load<u32>(lo))) {
        store<u32>(dst, m);
        ctx.fpr[1] = LfsF(m);
    } else {
        store<u32>(dst, load<u32>(lo));
        ctx.fpr[1] = LfsF(h);
    }
}
}  // namespace

// u32 Xform_Read229F70(Xform* o, XmlCursor* c)
// (called by XformList_Read22A310, 0022AD08) Nothing when c is 0 (returns
// o). Reads a hash (0xFE6BD5D7) into o+32, a vec3 (0x8ACA5893) into o+16,
// floats 0x08DD8365 / 0x12E79A42 into o+36 / +40, bools 0x5312BD7B /
// 0x0F59A2EC / 0x8B8C3D4B into bytes +52..+54, floats 0x9526986B /
// 0x0FC125FC into o+56 / +60 (each defaulting to the field); the rotation
// in degrees (0x00013DB7, default 0; converted by 0x00229F5C +12 when
// found) as a quaternion into o+0..+12; floats 0xF7654CCA / 0xF7654BDC into
// o+44 / +48, each clamped to [pool +8, pool +4] (0, 1). Returns the pool
// address (as r3 is left).
DECOMP(00229F70, Xform_Read229F70) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    if (c == 0) {
        ret(ctx, o);
        return;
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xFE6BD5D7u, o + 32, load<u32>(o + 32)});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x8ACA5893u, o + 16, o + 16});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x08DD8365u, o + 36}, {LoadF(o + 36)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x12E79A42u, o + 40}, {LoadF(o + 40)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x5312BD7Bu, o + 52, load<u8>(o + 52)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x0F59A2ECu, o + 53, load<u8>(o + 53)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x8B8C3D4Bu, o + 54, load<u8>(o + 54)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x9526986Bu, o + 56}, {LoadF(o + 56)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x0FC125FCu, o + 60}, {LoadF(o + 60)});
    constexpr u32 kPool = 0x00229F5C;  // f32 0.5, 1, 0, degrees to radians
    f64 ex, ey, ez;
    {
        Stack frame(ctx, 16);
        const u32 e = frame.addr();  // the original's sp + 112
        const bool found = call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x00013DB7u, e, mid11::kVecZero}) == 0;
        ex = LoadF(e);
        ey = LoadF(e + 4);
        ez = LoadF(e + 8);
        if (found) {
            const f64 k = LoadF(kPool + 12);
            ex = MulS(ex, k);
            ey = MulS(ey, k);
            ez = MulS(ez, k);
        }
    }
    mid12::EulerQuatAt(ctx, o, ex, ey, ez, LoadF(kPool));
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xF7654CCAu, o + 44}, {LoadF(o + 44)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xF7654BDCu, o + 48}, {LoadF(o + 48)});
    ClampF(ctx, o + 44, kPool + 8, kPool + 4);
    ClampF(ctx, o + 48, kPool + 8, kPool + 4);
    ret(ctx, kPool);  // r3 is left holding the pool address
}

// u32 XformList_Read22A310(XformList* v, a1, XmlCursor* c)
// (called by XmlDef_Read22DFD8) Nothing when c is 0 (returns v). Reads
// hashes 0x1B080CD1 / 0x29B8712A / 0xAEFB8B49 / 0x21EB6449 / 0x6B85440E /
// 0x6350BDB4 into v+20..+40 and a float (0xC4EC7385) into v+44 (each
// defaulting to the field); then each child element tagged 0xEDA93D29 is
// read by Xform_Read229F70 into a 64-byte record (quaternion 0x01164C90,
// zero vec3 +16, +32..+44 0, f32 +48 1, bytes +52..+54 1 / 0 / 1, f32 +56
// / +60 -1) appended to the vector v (+0 data, u16 +4 capacity, +8 count,
// +10 high mark; word +28 not copied). Returns 0.
DECOMP(0022A310, XformList_Read22A310) {
    const u32 v = arg<u32>(ctx, 0), c = arg<u32>(ctx, 2);
    if (c == 0) {
        ret(ctx, v);
        return;
    }
    static constexpr u32 kH[6] = {0x1B080CD1u, 0x29B8712Au, 0xAEFB8B49u, 0x21EB6449u, 0x6B85440Eu, 0x6350BDB4u};
    for (u32 k = 0; k < 6; ++k) call(ctx, xfn::XmlCursor_ReadHash, {c, kH[k], v + 20 + 4 * k, load<u32>(v + 20 + 4 * k)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xC4EC7385u, v + 44}, {LoadF(v + 44)});
    Stack frame(ctx, 64);
    const u32 rec = frame.addr();  // the original's sp + 112
    for (u32 e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
         e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_NextSiblingAny, {e}))) {
        for (u32 k = 0; k < 16; k += 4) CopyW(rec + k, mid11::kQuat4C90 + k);
        for (u32 k = 0; k < 12; k += 4) CopyW(rec + 16 + k, mid11::kVecZero + k);
        SetF1(ctx, mid11::kVecZero + 8);
        for (u32 k = 32; k <= 44; k += 4) store<u32>(rec + k, 0);
        store<u32>(rec + 48, 0x3F800000u);
        store<u8>(rec + 52, 1);
        store<u8>(rec + 53, 0);
        store<u8>(rec + 54, 1);
        store<u32>(rec + 56, 0xBF800000u);
        store<u32>(rec + 60, 0xBF800000u);
        if (NodeTag(e) != 0xEDA93D29u) continue;
        call(ctx, 0x00229F70, {rec, e});  // Xform_Read229F70
        u32 n = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (n == cap) {
            call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
            n = load<u16>(v + 8);
        }
        const u32 d = load<u32>(v) + ((n << 6) & 0x3FFFC0u);
        CopySeq(d, rec, {{4, 0, 0}, {4, 4, 4}, {4, 8, 8}, {4, 12, 12}, {4, 16, 16}, {4, 20, 20}, {4, 24, 24}, {4, 32, 32}, {4, 36, 36},
                         {4, 40, 40}, {4, 44, 44}, {4, 48, 48}, {1, 52, 52}, {1, 53, 53}, {1, 54, 54}, {4, 56, 56}, {4, 60, 60}});
        SetF1(ctx, rec + 60);
        const u32 m = (load<u16>(v + 8) + 1u) & 0xFFFFu, hw = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(m));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(m) ? hw : m));
    }
    ret(ctx, 0u);
}

namespace {
// *slot, created on first use (SmallAlloc(20), PtrVector_Construct,
// element size elem; 0 when the allocation fails).
u32 VecSlot(PPUContext& ctx, u32 slot, u16 elem) {
    u32 v = load<u32>(slot);
    if (v != 0) return v;
    v = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {20}));
    store<u32>(slot, v);
    if (v == 0) return 0;
    call(ctx, m1fn::PtrVector_Construct, {v});
    store<u16>(v + 6, elem);
    return load<u32>(slot);
}

// Grows the vector v (+0 data, u16 +4 capacity, +8 count) when full;
// returns the count.
u32 VecRoom(PPUContext& ctx, u32 v) {
    const u32 n = load<u16>(v + 8), cap = load<u16>(v + 4);
    if (n != cap) return n;
    call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0xFFFFFFFEu});
    return load<u16>(v + 8);
}

// count + 1 into u16 v+8 and the high mark u16 v+10 raised to it.
void VecCount(u32 v) {
    const u32 n = (load<u16>(v + 8) + 1u) & 0xFFFFu, m = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n) ? m : n));
}

// The address of the last element (count - 1, wrapping) of *slot.
u32 VecLast(u32 slot, u32 size) {
    const u32 v = load<u32>(slot);
    return load<u32>(v) + (static_cast<u32>(load<u16>(v + 8)) - 1u) * size;
}

// The inlined erase of the last element of *slot: count - 1; when that
// differs from its low 16 bits (count was 0) the element at the masked
// index is moved over the one at the full index (rows; the original's
// move-the-last-into-the-hole, which otherwise does nothing).
void VecDropLast(PPUContext& ctx, u32 slot, u32 shift, u32 mask, std::initializer_list<Cp> rows, u32 f1src) {
    const u32 v = load<u32>(slot);
    const u32 idx = static_cast<u32>(load<u16>(v + 8)) - 1u, n = idx & 0xFFFFu;
    store<u16>(v + 8, static_cast<u16>(n));
    if (idx == n) return;
    const u32 d = load<u32>(v);
    const u32 dst = d + (idx << shift), src = d + ((idx << shift) & mask);
    CopySeq(dst, src, rows);
    SetF1(ctx, src + f1src);  // the last lfs of the move
}

// The two list readers' tail: 0 when *o has no elements, else the name
// hash (0x0024728B, default o->4) into o+4 and whether it is set.
void ListTail(PPUContext& ctx, u32 c, u32 o) {
    const u32 v = load<u32>(o);
    if (v == 0 || load<u16>(v + 8) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x0024728Bu, o + 4, load<u32>(o + 4)});
    ret(ctx, load<u32>(o + 4) != 0 ? 1u : 0u);
}
}  // namespace

// bool KeyList_Read47F310(a0, XmlCursor* c, KeyList* o)
// (called by 0047FFF8) For each child element tagged 0x00014042 an 8-byte
// key {+0 hash, +4 f32} is appended (zeroed) to the vector *o (created on
// first use, element size 8): the hash 0x00014042 into +0, then when it is
// set a float (0xB7EA3D94) into +4, else the key is dropped again. Then
// ListTail: the name hash into o+4 when there are keys; returns whether
// there are keys and a name.
DECOMP(0047F310, KeyList_Read47F310) {
    const u32 c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    for (u32 e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
         e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_NextSiblingAny, {e}))) {
        if (NodeTag(e) != 0x00014042u) continue;
        const u32 v = VecSlot(ctx, o, 8);
        const u32 n = VecRoom(ctx, v);
        const u32 p = load<u32>(v) + (n << 3);
        const u32 cnt = (load<u16>(v + 8) + 1u) & 0xFFFFu, m = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(cnt));
        store<u32>(p, 0);
        store<u32>(p + 4, 0);
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(cnt) ? m : cnt));
        const u32 k = VecLast(o, 8);
        call(ctx, xfn::XmlCursor_ReadHash, {e, 0x00014042u, k, load<u32>(k)});
        if (load<u32>(k) != 0) callf(ctx, xfn::XmlCursor_ReadF32, {e, 0xB7EA3D94u, k + 4}, {LoadF(k + 4)});
        else VecDropLast(ctx, o, 3, 0x7FFF8u, {{4, 0, 0}, {4, 4, 4}}, 4);
    }
    ListTail(ctx, c, o);
}

// bool XformKeyList_Read47FCF0(a0, XmlCursor* c, KeyList* o)
// (called by 0047FFF8) As KeyList_Read47F310 for the children tagged
// 0x040EFE5F with 64-byte elements (+0 / +4 0, quaternion 0x01164C90 at
// +16, +32..+56 0) read by 0x0047F588(a0, child, element), dropped again
// when that returns 0.
DECOMP(0047FCF0, XformKeyList_Read47FCF0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    for (u32 e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
         e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_NextSiblingAny, {e}))) {
        if (NodeTag(e) != 0x040EFE5Fu) continue;
        const u32 v = VecSlot(ctx, o, 64);
        const u32 n = VecRoom(ctx, v);
        const u32 p = load<u32>(v) + (n << 6);
        store<u32>(p, 0);
        store<u32>(p + 4, 0);
        for (u32 k = 0; k < 16; k += 4) CopyW(p + 16 + k, mid11::kQuat4C90 + k);
        for (u32 k = 32; k <= 56; k += 4) store<u32>(p + k, 0);
        VecCount(v);
        if (call(ctx, 0x0047F588, {a0, e, VecLast(o, 64)}) == 0)
            VecDropLast(ctx, o, 6, 0x3FFFC0u,
                        {{4, 0, 0}, {4, 4, 4}, {4, 16, 16}, {4, 20, 20}, {4, 24, 24}, {4, 28, 28}, {4, 32, 32}, {4, 36, 36},
                         {4, 40, 40}, {4, 44, 44}, {4, 48, 48}, {4, 52, 52}, {4, 56, 56}}, 56);
    }
    ListTail(ctx, c, o);
}

// Obj* SlotObj_New331A78(a0, XmlCursor* c, a2)
// (called by 00322D88) Reads, from stack defaults, a vec3 (0x4A4D4D4D,
// zero), a hash (0x1A30BED4), floats 0x2FA14783 / 0x9E3D6E98 / 0x56EF5AEB
// (0) and 0xC26BCCA9 (-1), bools 0x76E96977, 0x3B66FD69 (b), 0x5D28E3FC,
// 0x9446A097, 0xD9159FE7, 0x09DA53D5 and, unless b is set (then both 1),
// 0x04656B91 / 0x310F51B2. The second float becomes the first unless it is
// above the pool 0x00331A70 and not below the first. Then a 2640-byte object
// (SmallAlloc) is built by 0x0033162C(p, a2, &vec, hash, the four floats,
// the seven bools on the stack). Returns it (0 when the allocation fails).
DECOMP(00331A78, SlotObj_New331A78) {
    const u32 c = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2);
    Stack frame(ctx, 48);
    const u32 s = frame.addr();  // the original's sp + 176
    for (u32 k = 0; k < 12; k += 4) CopyW(s + k, mid11::kVecZero + k);
    for (u32 k = 16; k <= 28; k += 4) store<u32>(s + k, 0);
    store<u32>(s + 32, 0xBF800000u);
    for (u32 k = 36; k < 43; ++k) store<u8>(s + k, 0);
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x4A4D4D4Du, s, s});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x1A30BED4u, s + 16, load<u32>(s + 16)});
    static constexpr u32 kF[4] = {0x2FA14783u, 0x9E3D6E98u, 0x56EF5AEBu, 0xC26BCCA9u};
    for (u32 k = 0; k < 4; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kF[k], s + 20 + 4 * k}, {LoadF(s + 20 + 4 * k)});
    store<u8>(s + 43, 0);
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x76E96977u, s + 36, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x3B66FD69u, s + 43, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x5D28E3FCu, s + 38, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x9446A097u, s + 39, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xD9159FE7u, s + 40, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x09DA53D5u, s + 41, 0});
    if (load<u8>(s + 43) != 0) {
        store<u8>(s + 37, 1);
        store<u8>(s + 42, 1);
    } else {
        call(ctx, xfn::XmlCursor_ReadBool, {c, 0x04656B91u, s + 37, 0});
        call(ctx, xfn::XmlCursor_ReadBool, {c, 0x310F51B2u, s + 42, 0});
    }
    if (LoadF(s + 24) > LoadF(0x00331A70)) {
        if (LoadF(s + 20) > LoadF(s + 24)) CopyW(s + 24, s + 20);
    } else {
        CopyW(s + 24, s + 20);
    }
    SetF1(ctx, s + 20);  // both paths end on an lfs of the first float
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {2640}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    u64 b[7];
    for (u32 k = 0; k < 7; ++k) b[k] = load<u8>(s + 36 + k);
    // r7..r10 carry the first four bools too (the float arguments' GPR
    // slots; not read by the callee), the seven bools go on the stack.
    callf(ctx, 0x0033162C, {p, a2, s, load<u32>(s + 16), b[0], b[1], b[2], b[3], b[0], b[1], b[2], b[3], b[4], b[5], b[6]},
          {LoadF(s + 20), LoadF(s + 24), LoadF(s + 28), LoadF(s + 32)});
    ret(ctx, p);
}

namespace {
// The query defs' four bools 0xBEF25C9F / 0x1EF4F9F5 / 0x2D407057 /
// 0x778A1C92 into bits 0x20 / 0x10 / 0x08 / 0x02 of byte o+184 (each
// defaulting to its bit), read through the stack byte buf (cleared first).
void QueryBits184(PPUContext& ctx, u32 c, u32 o, u32 buf) {
    store<u8>(buf, 0);
    static constexpr u32 kB[4] = {0xBEF25C9Fu, 0x1EF4F9F5u, 0x2D407057u, 0x778A1C92u};
    static constexpr u32 kBit[4] = {5, 4, 3, 1};
    for (u32 k = 0; k < 4; ++k) {
        const u32 sh = kBit[k];
        call(ctx, xfn::XmlCursor_ReadBool, {c, kB[k], buf, (load<u8>(o + 184) >> sh) & 1u});
        store<u8>(o + 184, static_cast<u8>((load<u8>(o + 184) & ~(1u << sh)) | ((load<u8>(buf) & 1u) << sh)));
    }
}
}  // namespace

// u32 QueryDef_Read215518(a0, XmlCursor* c, QueryDef* o)
// (called by 002159AC) Four bools (0xBEF25C9F, 0x1EF4F9F5, 0x2D407057,
// 0x778A1C92) into bits 0x20 / 0x10 / 0x08 / 0x02 of byte o+184 (each
// defaulting to its bit); a kind mask from the children 0x51609AAA (name
// hash 0x7DD0CD87 1, 0x0000FD2A 2, 0x03D3CE95 4, 0x03D66C07 8) or'ed with
// the attribute 0x51609AAA mapped the same way (0xFFFF when neither gives
// anything): u16 o+192 = mask, u16 o+194 = ~mask (without bit 2 when the
// mask has 4). Floats 0x79792193 / 0xB5AB2880 into o+200 / +204, a hash
// 0x88B4A252 into o+196 (defaulting to the fields); for the hash
// 0x04C5F92B a vec3 (0x313AB0C1) into o+224, a vec2 (0x313AB844) into
// o+240 scaled by the pool 0x00215514, a float (0x000110AD) into o+248 and
// a bool (0x2DE05CD7) into byte o+252; for 0x922D3BD1 three hashes
// (0xBDE51CC3, 0xBDE52AD0, 0x484957BB) into o+208..+216. Returns the last
// call's result, or the hash when nothing follows it.
DECOMP(00215518, QueryDef_Read215518) {
    const u32 c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();  // the original's sp + 112
    QueryBits184(ctx, c, o, sp + 8);
    auto kind = [](u32 h) -> u32 {
        switch (h) {
            case 0x7DD0CD87u: return 1;
            case 0x0000FD2Au: return 2;
            case 0x03D3CE95u: return 4;
            case 0x03D66C07u: return 8;
            default: return 0;
        }
    };
    u32 kids = 0;
    for (u32 e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, 0x51609AAAu})); e != 0;
         e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {e, 0x51609AAAu}))) {
        store<u32>(sp + 4, 0);
        call(ctx, xfn::XmlCursor_ReadHash, {e, 0x0024728Bu, sp + 4, 0});
        kids |= kind(load<u32>(sp + 4));
    }
    store<u32>(sp, 0);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x51609AAAu, sp, 0});
    const u32 v = load<u32>(sp);
    const u32 m = (v != 0 ? kind(v) : (kids != 0 ? 0u : 0xFFFFu)) | kids;
    store<u16>(o + 192, static_cast<u16>(m));
    store<u16>(o + 194, static_cast<u16>(~m));
    if (m & 4u) store<u16>(o + 194, static_cast<u16>(~m & ~2u));
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x79792193u, o + 200}, {LoadF(o + 200)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xB5AB2880u, o + 204}, {LoadF(o + 204)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x88B4A252u, o + 196, load<u32>(o + 196)});
    const u32 t = load<u32>(o + 196);
    if (t == 0x04C5F92Bu) {
        call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x313AB0C1u, o + 224, o + 224});
        call(ctx, xfn::XmlCursor_ReadVec2, {c, 0x313AB844u, o + 240, o + 240});
        const f64 k = LoadF(0x00215514), f248 = LoadF(o + 248);
        const f64 x = MulS(LoadF(o + 240), k), y = MulS(LoadF(o + 244), k);
        StoreF(o + 240, x);
        StoreF(o + 244, y);
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x000110ADu, o + 248}, {f248});
        call(ctx, xfn::XmlCursor_ReadBool, {c, 0x2DE05CD7u, o + 252, load<u8>(o + 252)});
    } else if (t == 0x922D3BD1u) {
        call(ctx, xfn::XmlCursor_ReadHash, {c, 0xBDE51CC3u, o + 208, load<u32>(o + 208)});
        call(ctx, xfn::XmlCursor_ReadHash, {c, 0xBDE52AD0u, o + 212, load<u32>(o + 212)});
        call(ctx, xfn::XmlCursor_ReadHash, {c, 0x484957BBu, o + 216, load<u32>(o + 216)});
    } else {
        ret(ctx, t);
    }
}

// bool QueryVar_Read49EC08(XmlNode* n, QueryVar* o)
// (called by QueryDef_ReadChild and two XML factories) 0 unless n and o
// are set, n's tag maps to a value kind (0x0049EA74) and n has a name hash
// (0x0024728B): then o+68 = the name, byte o+72 = the kind, and the value
// attribute 0x04DA9951 is read by kind: a hash into o+0 (kinds 1, 4..8,
// 10) and / or o+4 (2, 3), a float (12, default 0x0049EC04), an s32 (13),
// a bool (14), a vec3 (15; stored to o+0..+8 on success), a string copied
// into o (11, up to 64 bytes); kind 16 needs nothing. Returns 1 once a
// read succeeds (a hash only when non-zero), else 0.
DECOMP(0049EC08, QueryVar_Read49EC08) {
    const u32 n = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    if (n == 0 || o == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 kind = static_cast<u32>(call(ctx, 0x0049EA74, {NodeTag(n)}));
    if (kind == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 32);
    const u32 v = frame.addr(), h = v + 16;  // the original's sp + 112 / + 128
    store<u32>(h, 0);
    if (call(ctx, xfn::XmlCursor_ReadHash, {n, 0x0024728Bu, h, 0}) != 0 || load<u32>(h) == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o + 68, load<u32>(h));
    store<u8>(o + 72, static_cast<u8>(kind));
    for (u32 k = 0; k < 12; k += 4) CopyW(v + k, mid11::kVecZero + k);
    SetF1(ctx, mid11::kVecZero + 4);
    constexpr u32 H = 0x04DA9951;
    auto finish = [&](u32 src) {
        if (load<u8>(o + 72) == 11) call(ctx, 0x00553684, {o, src, 64});  // String_Copy
        if (load<u8>(o + 72) == 15) {
            for (u32 k = 0; k < 12; k += 4) CopyW(o + k, v + k);
            SetF1(ctx, v + 8);
        }
        ret(ctx, 1u);
    };
    if (kind == 1 || (kind >= 4 && kind <= 8) || kind == 10) {
        if (call(ctx, xfn::XmlCursor_ReadHash, {n, H, o, 0}) == 0 && load<u32>(o) != 0) return finish(0);
    }
    u32 b = load<u8>(o + 72);
    if (b == 2 || b == 3) {
        if (call(ctx, xfn::XmlCursor_ReadHash, {n, H, o + 4, 0}) == 0 && load<u32>(o + 4) != 0) return finish(0);
    }
    b = load<u8>(o + 72);
    if (b == 12) {
        callf(ctx, xfn::XmlCursor_ReadF32, {n, H, o}, {LoadF(0x0049EC04)});
        if (static_cast<u32>(ctx.gpr[3]) == 0) return finish(0);
        b = load<u8>(o + 72);
    }
    if (b == 13) {
        if (call(ctx, xfn::XmlCursor_ReadS32, {n, H, o, 0}) == 0) return finish(0);
        b = load<u8>(o + 72);
    }
    if (b == 14) {
        if (call(ctx, xfn::XmlCursor_ReadBool, {n, H, o, 0}) == 0) return finish(0);
        b = load<u8>(o + 72);
    }
    if (b == 15) {
        if (call(ctx, xfn::XmlCursor_ReadVec3, {n, H, v, mid11::kVecZero}) == 0) return finish(0);
        b = load<u8>(o + 72);
    }
    if (b == 16) return finish(0);
    if (b == 11) {
        const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {n, H}));
        if (s != 0 && load<u8>(s) != 0) return finish(s);
    }
    ret(ctx, 0u);
}

// u32 HashMap_Init4C6FB8(HashMapDef* o, XmlCursor* c)
// (called by New_004C74CC) u16 o+4 = 1, vtable 0x00B22358, an empty tree at
// o+24 (+24 / +28 0, byte +32 1, +36 = o+24) and its iterator (+40 stack,
// u16 +120 depth, +124 current) reset to the leftmost node (none: the
// tree was just emptied); +12..+20 0, byte +128 0. For each child element
// tagged 0x0486206D the pair {name hash 0x0024728B, value hash 0x04DA9951}
// is inserted (Avl_TreeInsert_0073B444(o+24, key, value)); then hashes
// 0x9178C4CB / 0xB4FE4E5C / 0xB4FE29BA into o+12..+20 and a bool
// (0x77FD0B93) into byte o+128 (default 0). Returns the last result.
DECOMP(004C6FB8, HashMap_Init4C6FB8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u32 t = o + 24;
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B22358u);
    store<u32>(o + 24, 0);
    store<u32>(o + 28, 0);
    store<u8>(o + 32, 1);
    store<u32>(o + 36, t);
    store<u16>(o + 120, 0);
    u32 cur = 0;
    for (u32 n = load<u32>(t); n != 0;) {  // descend left from the root, pushing
        cur = n;
        const u32 l = load<u32>(n + 12);
        if (l != 0) {
            const u32 d = load<u16>(o + 120);
            store<u16>(o + 120, static_cast<u16>(d + 1));
            store<u32>(o + 40 + 4u * static_cast<u32>(static_cast<s32>(static_cast<s16>(d))), n);
        }
        n = l;
    }
    store<u32>(o + 124, cur);
    store<u8>(o + 32, 1);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    store<u8>(o + 128, 0);
    {
        Stack frame(ctx, 16);
        const u32 kv = frame.addr();  // the original's sp + 112 (not cleared first)
        for (u32 e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
             e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_NextSiblingAny, {e}))) {
            if (NodeTag(e) != 0x0486206Du) continue;
            call(ctx, xfn::XmlCursor_ReadHash, {e, 0x0024728Bu, kv, 0});
            call(ctx, xfn::XmlCursor_ReadHash, {e, 0x04DA9951u, kv + 4, 0});
            call(ctx, 0x0073B444, {t, load<u32>(kv), load<u32>(kv + 4)});  // Avl_TreeInsert_0073B444
        }
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x9178C4CBu, o + 12, 0});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xB4FE4E5Cu, o + 16, 0});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xB4FE29BAu, o + 20, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x77FD0B93u, o + 128, 0});
}

// u32 QueryDef_Read21E978(a0, XmlCursor* c, QueryDef* o)
// (called by 0021ED78) QueryBits184; each child element tagged 0xC3B709C0
// read by QueryDef_ReadChild(child, o+188); a hash (0xF89FDD94) into
// o+212; three bools (the hashes at 0x00AD37E8) or'ed into bits 0..2 of
// o->208 (each defaulting to its bit); the float pair o+216 / +220 both
// from 0x11CB4BE1 when found (default the pool 0x0021E974), else from
// 0xB8D446B1 / 0xB8D445C3; the pair o+224 / +228 likewise (0x49786225, else
// 0xD02659ED / 0xD02658FF); a hash (0x82BF2A4A) into o+232. The four
// floats are raised to the pool value, then +220 to +216 and +228 to +224.
// Returns the last read's result.
DECOMP(0021E978, QueryDef_Read21E978) {
    const u32 c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();  // the original's sp + 112
    QueryBits184(ctx, c, o, sp + 5);
    for (u32 e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
         e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_NextSiblingAny, {e})))
        if (NodeTag(e) == 0xC3B709C0u) call(ctx, 0x0049EF40, {e, o + 188});  // QueryDef_ReadChild
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xF89FDD94u, o + 212, load<u32>(o + 212)});
    for (u32 k = 0; k < 3; ++k) {
        store<u8>(sp + 4, static_cast<u8>((load<u32>(o + 208) & (1u << k)) != 0));
        call(ctx, xfn::XmlCursor_ReadBool, {c, load<u32>(0x00AD37E8u + 4 * k), sp + 4, load<u8>(sp + 4)});
        store<u32>(o + 208, load<u32>(o + 208) | (static_cast<u32>(load<u8>(sp + 4)) << k));
    }
    constexpr u32 kMin = 0x0021E974;
    const f64 lo = LoadF(kMin);
    store<u32>(sp, 0);
    auto pair = [&](u32 h, u32 d, u32 ha, u32 hb) {
        callf(ctx, xfn::XmlCursor_ReadF32, {c, h, sp}, {lo});
        if (static_cast<u32>(ctx.gpr[3]) == 0) {
            CopyW(d, sp);
            CopyW(d + 4, sp);
            SetF1(ctx, sp);
        } else {
            callf(ctx, xfn::XmlCursor_ReadF32, {c, ha, d}, {LoadF(d)});
            callf(ctx, xfn::XmlCursor_ReadF32, {c, hb, d + 4}, {LoadF(d + 4)});
        }
    };
    pair(0x11CB4BE1u, o + 216, 0xB8D446B1u, 0xB8D445C3u);
    pair(0x49786225u, o + 224, 0xD02659EDu, 0xD02658FFu);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x82BF2A4Au, o + 232, load<u32>(o + 232)});
    u32 w[4];
    for (u32 k = 0; k < 4; ++k) w[k] = load<u32>(o + 216 + 4 * k);
    for (u32 k = 0; k < 4; ++k) {
        if (LfsF(w[k]) < lo) w[k] = load<u32>(kMin);
        store<u32>(o + 216 + 4 * k, w[k]);
    }
    if (LfsF(w[1]) < LfsF(w[0])) store<u32>(o + 220, w[0]);
    if (LfsF(w[3]) < LfsF(w[2])) store<u32>(o + 228, w[2]);
    ctx.fpr[1] = LfsF(w[3]);  // as the original leaves f1
}

// u32 ChoiceList_Read22C2B0(Vector* v, a1, XmlCursor* c, f32* total)
// (called by 0022C840) Nothing when c is 0 (returns v). A hash
// (0xA9F6D134) into v+20 and a bool (0x87905FBF) into byte v+28 (each
// defaulting to the field); then for each child element tagged 0x04EBC33B
// a 352-byte entry built on the stack (0x0022AB00) is appended to v
// (0x0075C8F4(v, &entry)), the stack entry's five PtrVectors (+320, +300,
// +280, +260, +240) cleared (PtrVector_Clear, Nop), the appended entry read
// by 0x0022AD08(entry, a1, child) and its f32 +96 added to *total.
// Returns 0.
DECOMP(0022C2B0, ChoiceList_Read22C2B0) {
    const u32 v = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), total = arg<u32>(ctx, 3);
    if (c == 0) {
        ret(ctx, v);
        return;
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xA9F6D134u, v + 20, load<u32>(v + 20)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x87905FBFu, v + 28, load<u8>(v + 28)});
    Stack frame(ctx, 352);
    const u32 ent = frame.addr();  // the original's sp + 112
    for (u32 e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
         e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_NextSiblingAny, {e}))) {
        if (NodeTag(e) != 0x04EBC33Bu) continue;
        call(ctx, 0x0022AB00, {ent});
        call(ctx, 0x0075C8F4, {v, ent});
        for (u32 k = 320; k >= 240; k -= 20) {
            call(ctx, m1fn::PtrVector_Clear, {ent + k});
            call(ctx, m1fn::Nop, {ent + k});
        }
        const u32 last = load<u32>(v) + (static_cast<u32>(load<u16>(v + 8)) - 1u) * 352u;
        call(ctx, 0x0022AD08, {last, a1, e});
        const f64 s = static_cast<f32>(LoadF(total) + LoadF(last + 96));
        StoreF(total, s);
        ctx.fpr[1] = s;  // the fadds result, as the original leaves f1
    }
    ret(ctx, 0u);
}

// u32 GroupTable_Read17124(XmlCursor* c)
// (called by 00017420, 000632A8) Fills four global u32 PtrVectors (counts
// reset first): for each child element tagged 0xFBA47770 of c, its hash
// 0x044FA364 goes to kGroupA, the name hashes (0x0024728B) of its children
// tagged 0x04B8CC71 to kGroupItems, and the group's first item index and
// item count to kGroupFirst / kGroupCount. Returns 0.
DECOMP(00017124, GroupTable_Read17124) {
    const u32 c = arg<u32>(ctx, 0);
    constexpr u32 kGroupA = 0x00F9BC78, kGroupItems = 0x00F9BCC0, kGroupFirst = 0x00F9BC90, kGroupCount = 0x00F9BCA8;
    store<u16>(kGroupA + 8, 0);
    store<u16>(kGroupItems + 8, 0);
    store<u16>(kGroupFirst + 8, 0);
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();  // the original's sp + 112
    u32 e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_FirstChildAny, {c}));
    store<u32>(sp, 0);
    u32 start = 0;
    for (; e != 0; e = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_NextSiblingAny, {e}))) {
        if (NodeTag(e) != 0xFBA47770u) continue;
        call(ctx, xfn::XmlCursor_ReadHash, {e, 0x044FA364u, sp, 0});
        for (u32 k = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_FirstChildAny, {e})); k != 0;
             k = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_NextSiblingAny, {k}))) {
            if (NodeTag(k) != 0x04B8CC71u) continue;
            call(ctx, xfn::XmlCursor_ReadHash, {k, 0x0024728Bu, sp + 4, 0});
            mid1::PtrVecPush(ctx, kGroupItems, load<u32>(sp + 4));
        }
        mid1::PtrVecPush(ctx, kGroupA, load<u32>(sp));
        mid1::PtrVecPush(ctx, kGroupFirst, start);
        mid1::PtrVecPush(ctx, kGroupCount, load<u16>(kGroupItems + 8) - start);
        start = load<u16>(kGroupItems + 8);
    }
    ret(ctx, 0u);
}

// Obj* SlotObj_New32D240(a0, XmlCursor* c, const Def* d)
// (called by 00322D88) Floats 0x56EF5AEB / 0x19E32604 (default the pool
// 0x0032D23C); a 2608-byte object (SmallAlloc): Obj_Construct_00769BD8(p,
// d), f32 +2572 = the first, +2576 / +2580 = the second, vtable
// 0x00B1E6C0, +2584 0, the PtrVector +2588 of 24-byte entries. Then for
// each child 0x7A30D7E3 an entry {+0 name hash (0x0024728B; when missing
// c's document path is noted and the child skipped), +4 0, f32 +8 / +12 /
// +16 (0x03B880B3, 0x8E727ACC, 0x8E7279DE; defaults the tuning values
// 0x0106B590 / 0x0106B5DC / 0x0106B628), byte +20 1} is appended (Memmove
// of 21 bytes). Returns the object (0 when the allocation fails). Quirk:
// the children are still appended (at address 2588) without an object.
DECOMP(0032D240, SlotObj_New32D240) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    Stack frame(ctx, 32);
    const u32 sp = frame.addr();  // the original's sp + 112
    const f64 k = LoadF(0x0032D23C);
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x56EF5AEBu, sp}, {k});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x19E32604u, sp + 4}, {k});
    u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {2608}));
    if (p != 0) {
        const u32 f0 = load<u32>(sp), f1 = load<u32>(sp + 4);
        call(ctx, 0x00769BD8, {p, d});  // Obj_Construct_00769BD8
        store<u32>(p + 2572, f0);
        store<u32>(p + 2576, f1);
        store<u32>(p + 2580, f1);
        store<u32>(p, 0x00B1E6C0u);
        store<u32>(p + 2584, 0);
        call(ctx, m1fn::PtrVector_Construct, {p + 2588});
        store<u16>(p + 2594, 24);
    }
    const u32 v = p + 2588, e0 = sp + 8;  // the entry: the original's sp + 120
    for (u32 e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, 0x7A30D7E3u})); e != 0;
         e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {e, 0x7A30D7E3u}))) {
        for (u32 j = 0; j <= 16; j += 4) store<u32>(e0 + j, 0);
        store<u8>(e0 + 20, 1);
        if (call(ctx, xfn::XmlCursor_ReadHash, {e, 0x0024728Bu, e0, 0}) != 0) {
            call(ctx, m11fn::XmlCursor_DocPath, {a0});
            continue;
        }
        callf(ctx, xfn::XmlCursor_ReadF32, {e, 0x03B880B3u, e0 + 8}, {LoadF(0x0106B590)});
        callf(ctx, xfn::XmlCursor_ReadF32, {e, 0x8E727ACCu, e0 + 12}, {LoadF(0x0106B5DC)});
        callf(ctx, xfn::XmlCursor_ReadF32, {e, 0x8E7279DEu, e0 + 16}, {LoadF(0x0106B628)});
        u32 n = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (n == cap) {
            call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0xFFFFFFFEu});
            n = load<u16>(v + 8);
        }
        call(ctx, m12fn::Memmove, {load<u32>(v) + n * 24u, e0, 21});
        const u32 m = (load<u16>(v + 8) + 1u) & 0xFFFFu, hw = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(m));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(m) ? hw : m));
    }
    ret(ctx, p);
}

// Obj* SlotObj_New32DD10(a0, XmlCursor* c, const Def* d)
// (called by 00322D88) Floats 0xDBDD1EA5 / 0x71F634BF (defaults the pool
// 0x0032DD04 +0 / +4); a 3376-byte object (SmallAlloc):
// Obj_Construct_00769BD8(p, d), +3360 / +3364 0, bytes +3368 / +2556 0,
// vtable 0x00B1E738, f32 +2572 = the second, +2576 = the first. Then for
// each child 0x7A30D7E3 with a name hash (0x0024728B) up to 16 48-byte
// entries at +2592 {+0 the hash, +4 0, vec3 +16 (0x8ACA5893, zero), f32 +32
// (0xBBBB80B0 then 0x8E7279DE, 0.75), f32 +36 (0x8E727ACC, 5), f32 +40 50,
// byte +44 (0x1EE7AC93)}, counted in +3360 (the reading stops when full).
// Returns the object (0 when the allocation fails).
DECOMP(0032DD10, SlotObj_New32DD10) {
    const u32 c = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    Stack frame(ctx, 48);
    const u32 sp = frame.addr();  // the original's sp + 112
    constexpr u32 kPool = 0x0032DD04;
    store<u32>(sp + 16, 0x41700000u);
    store<u32>(sp + 20, 0x40A00000u);
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xDBDD1EA5u, sp + 16}, {LoadF(kPool)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x71F634BFu, sp + 20}, {LoadF(kPool + 4)});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {3376}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 f0 = load<u32>(sp + 16), f1 = load<u32>(sp + 20);
    call(ctx, 0x00769BD8, {p, d});  // Obj_Construct_00769BD8
    store<u32>(p + 3360, 0);
    store<u32>(p + 3364, 0);
    store<u8>(p + 3368, 0);
    store<u8>(p + 2556, 0);
    store<u32>(p, 0x00B1E738u);
    store<u32>(p + 2572, f1);
    store<u32>(p + 2576, f0);
    for (u32 e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, 0x7A30D7E3u})); e != 0;
         e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {e, 0x7A30D7E3u}))) {
        for (u32 j = 0; j < 12; j += 4) CopyW(sp + j, mid11::kVecZero + j);
        store<u32>(sp + 24, 0);
        store<u32>(sp + 28, 0x3F400000u);
        store<u32>(sp + 32, 0x40A00000u);
        store<u8>(sp + 36, 0);
        SetF1(ctx, mid11::kVecZero + 8);  // the last lfs of the defaults
        if (call(ctx, xfn::XmlCursor_ReadHash, {e, 0x0024728Bu, sp + 24, 0}) != 0) continue;
        call(ctx, xfn::XmlCursor_ReadVec3, {e, 0x8ACA5893u, sp, sp});
        callf(ctx, xfn::XmlCursor_ReadF32, {e, 0xBBBB80B0u, sp + 28}, {LoadF(sp + 28)});
        callf(ctx, xfn::XmlCursor_ReadF32, {e, 0x8E727ACCu, sp + 32}, {LoadF(sp + 32)});
        callf(ctx, xfn::XmlCursor_ReadF32, {e, 0x8E7279DEu, sp + 28}, {LoadF(sp + 28)});
        call(ctx, xfn::XmlCursor_ReadBool, {e, 0x1EE7AC93u, sp + 36, load<u8>(sp + 36)});
        const s32 n = load<s32>(p + 3360);
        SetF1(ctx, sp + 32);  // the loads before the test leave f1 = +36's value
        if (n >= 16) break;
        const u32 q = p + 2592 + static_cast<u32>(n) * 48u;
        store<u32>(q, load<u32>(sp + 24));
        CopyW(q + 16, sp);
        store<u32>(q + 4, 0);
        CopyW(q + 20, sp + 4);
        CopyW(q + 24, sp + 8);
        CopyW(q + 32, sp + 28);
        CopyW(q + 36, sp + 32);
        store<u32>(q + 40, 0x42480000u);
        store<u8>(q + 44, load<u8>(sp + 36));
        store<u32>(p + 3360, load<u32>(p + 3360) + 1);
    }
    ret(ctx, p);
}
