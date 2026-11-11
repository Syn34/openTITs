// Game code, medium sweep part 11: the hash-keyed registries filled by
// 000D5C00 (entries read from XML elements by their 0x0024728B name hash)
// and a small def reader on Rec_CopyFields_000C387C.
#include "mid11.h"

using namespace sly4;
using namespace sly4::mid11;

namespace {

constexpr u32 kRegD5A18 = 0x0103214C;  // registry {u16 +0 capacity, +2 count, +4 high mark, +6 flags, +8 {key, value}[]}
constexpr u32 kRegD54B0 = 0x01032164;  // the same, for the 176 / 160-byte objects
constexpr u32 SortedMap_Find744C4C = 0x00744C4C;  // (reg, const u32* key) -> value slot
constexpr u32 SortedMap_Find744DD4 = 0x00744DD4;  // (reg, const u32* key) -> value slot
constexpr u32 Array_Reserve = 0x00558F3C;   // (reg, u16 capacity)
constexpr u32 Hash_ToString = 0x0056A6E0;   // (u32 hash): "0x%x" (result unused here)

// Appends {key, value} to the registry (grown to twice the capacity when
// full), raises its high mark and sets flag 1 (unsorted). Returns the new
// flags (the original's r3).
u32 RegPush(PPUContext& ctx, u32 reg, u32 key, u32 value) {
    u32 n = load<u16>(reg + 2);
    const u32 cap = load<u16>(reg);
    if (n == cap) {
        call(ctx, Array_Reserve, {reg, (cap << 1) & 0xFFFEu});
        n = load<u16>(reg + 2);
    }
    const u32 data = load<u32>(reg + 8);
    store<u32>(data + ((n << 3) & 0x7FFF8u), key);
    store<u32>(data + (static_cast<u32>(load<u16>(reg + 2)) << 3) + 4, value);
    const u32 c = load<u16>(reg + 2) + 1u;
    const u32 m = load<u16>(reg + 4);
    const u32 fl = load<u16>(reg + 6) | 1u;
    store<u16>(reg + 2, static_cast<u16>(c));
    store<u16>(reg + 4, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(c & 0xFFFFu) ? m : c));
    store<u16>(reg + 6, static_cast<u16>(fl));
    return fl;
}

// The shared head: the name hash (0x0024728B, default 0) of element c.
u32 RegName(PPUContext& ctx, u32 c, u32 h) {
    store<u32>(h, 0);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x0024728Bu, h, 0});
    return load<u32>(h);
}

// The common part of the 176 / 160-byte objects' inline constructor:
// vtable vt0, +16 the quaternion 0x01164C90, five zero vec3s at +32..+96,
// bits 0..2 of +124 cleared, byte +125 0, vtable vt1, zero vec3s at +128 /
// +144 (store order as the original's).
void ObjInitD5(PPUContext& ctx, u32 q, u32 vt1, bool big) {
    store<u32>(q, 0x00B13840u);
    for (u32 k = 0; k < 4; ++k) CopyW(q + 16 + 4 * k, kQuat4C90 + 4 * k);
    const u32 w124 = load<u32>(q + 124) & 0x1FFFFFFFu;
    for (u32 v = 0; v < 4; ++v)
        for (u32 k = 0; k < 3; ++k) CopyW(q + 32 + 16 * v + 4 * k, kVecZero + 4 * k);
    CopyW(q + 96, kVecZero);
    CopyW(q + 100, kVecZero + 4);
    store<u32>(q + 124, w124);
    CopyW(q + 104, kVecZero + 8);
    store<u8>(q + 125, 0);
    store<u32>(q, vt1);
    for (u32 k = 0; k < 3; ++k) CopyW(q + 128 + 4 * k, kVecZero + 4 * k);
    CopyW(q + 144, kVecZero);
    CopyW(q + 148, kVecZero + 4);
    if (big) {
        store<u32>(q + 160, 0x3F800000u);
        CopyW(q + 152, kVecZero + 8);
        store<u32>(q + 164, 0x3F800000u);
        store<u8>(q + 125, 2);
    } else {
        store<u8>(q + 125, 1);
        CopyW(q + 152, kVecZero + 8);
    }
    ctx.fpr[1] = LoadF(kVecZero + 8);  // the last lfs
}

}  // namespace

// void XmlReg_ReadD5A18(a0, XmlCursor* c)
// (called by 000D5C00) For element c's name hash h (0x0024728B; nothing
// when 0): when the registry 0x0103214C has no entry for h, a 12-byte
// record e (Mem_Alloc(12, 0, 16); {0, 1, 1}) reads a hash (0x700F12AD)
// into e+0 and two s32s (0x187976E3 into e+4 when not negative,
// 0x187975F5 into e+8 when not below e+4), then {h, e} is appended. When h
// is already there, c's document path is noted and Hash_ToString(h).
// Quirk: a failed allocation leaves e = 0 and the reads go to address 0.
DECOMP(000D5A18, XmlReg_ReadD5A18) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 h = frame.addr(), t = h + 4;  // the original's sp + 112 / + 116
    if (RegName(ctx, c, h) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 slot = static_cast<u32>(call(ctx, SortedMap_Find744C4C, {kRegD5A18, h}));
    if (load<u32>(slot) != 0) {
        call(ctx, m11fn::XmlCursor_DocPath, {a0});
        call(ctx, Hash_ToString, {load<u32>(h)});
        return;
    }
    const u32 e = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {12, 0, 16}));
    if (e != 0) {
        store<u32>(e, 0);
        store<u32>(e + 4, 1);
        store<u32>(e + 8, 1);
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x700F12ADu, e, load<u32>(e)});
    call(ctx, xfn::XmlCursor_ReadS32, {c, 0x187976E3u, t, static_cast<u64>(static_cast<s64>(load<s32>(e + 4)))});
    const s32 v = load<s32>(t);
    const u32 old8 = load<u32>(e + 8);
    if (v >= 0) store<s32>(e + 4, v);
    call(ctx, xfn::XmlCursor_ReadS32, {c, 0x187975F5u, t, static_cast<u64>(static_cast<s64>(static_cast<s32>(old8)))});
    const s32 w = load<s32>(t);
    if (w >= load<s32>(e + 4)) store<s32>(e + 8, w);
    ret(ctx, RegPush(ctx, kRegD5A18, load<u32>(h), e));
}

// void XmlReg_ReadD54B0(a0, XmlCursor* c)
// (called by 000D5C00) As XmlReg_ReadD5A18 on the registry 0x01032164: a
// new 176-byte object (Mem_Alloc(176, 0, 16), ObjInitD5 with vtable
// 0x00B13870, f32 +160 / +164 1.0, byte +125 2; 0 when the allocation
// fails) is passed by pointer to 0x000D5330(a0, c, &obj), which may replace
// it, and the result appended under h.
DECOMP(000D54B0, XmlReg_ReadD54B0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 h = frame.addr(), t = h + 4;  // the original's sp + 112 / + 116
    if (RegName(ctx, c, h) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 slot = static_cast<u32>(call(ctx, SortedMap_Find744DD4, {kRegD54B0, h}));
    if (load<u32>(slot) != 0) {
        call(ctx, m11fn::XmlCursor_DocPath, {a0});
        call(ctx, Hash_ToString, {load<u32>(h)});
        return;
    }
    const u32 q = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {176, 0, 16}));
    if (q != 0) ObjInitD5(ctx, q, 0x00B13870u, true);
    store<u32>(t, q);
    call(ctx, 0x000D5330, {a0, c, t});
    ret(ctx, RegPush(ctx, kRegD54B0, load<u32>(h), load<u32>(t)));
}

// void XmlReg_ReadD5748(a0, XmlCursor* c)
// (called by 000D5C00) As XmlReg_ReadD54B0 with a 160-byte object
// (ObjInitD5 with vtable 0x00B13858, byte +125 1) read by 0x000D4E28(a0, c,
// &obj), then two vec3s (0x048ED2F9, 0xAAB0F29E) into its +128 / +144
// (their own defaults).
DECOMP(000D5748, XmlReg_ReadD5748) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 h = frame.addr(), t = h + 4;  // the original's sp + 112 / + 116
    if (RegName(ctx, c, h) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 slot = static_cast<u32>(call(ctx, SortedMap_Find744DD4, {kRegD54B0, h}));
    if (load<u32>(slot) != 0) {
        call(ctx, m11fn::XmlCursor_DocPath, {a0});
        call(ctx, Hash_ToString, {load<u32>(h)});
        return;
    }
    const u32 q = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {160, 0, 16}));
    if (q != 0) ObjInitD5(ctx, q, 0x00B13858u, false);
    store<u32>(t, q);
    call(ctx, 0x000D4E28, {a0, c, t});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x048ED2F9u, load<u32>(t) + 128, load<u32>(t) + 128});
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0xAAB0F29Eu, load<u32>(t) + 144, load<u32>(t) + 144});
    ret(ctx, RegPush(ctx, kRegD54B0, load<u32>(h), load<u32>(t)));
}

// void Def_Read1A7878(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// (called by 001A7AB0) Rec_CopyFields_000C387C with the same arguments,
// byte o+25 = 1, then the defaults (hash 0, s32 4, floats pool 0x001A7868
// +0, +0, +4, +8, +12, hash 0; tmpl's +140..+168 when tmpl has flag 0x800
// in +24, c's document path noted when it lacks it): a hash (0x3559A5AB)
// into o+76, an s32 (0xBEC3D7F3) into o+80, five floats into o+84..+100,
// a hash (0x10E70B4D) into o+104.
DECOMP(001A7878, Def_Read1A7878) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, mid9::m9fn::Rec_CopyFields_000C387C, {a0, c, x, tmpl, o});
    constexpr u32 kPool = 0x001A7868;
    store<u8>(o + 25, 1);
    u32 h1 = 0, h2 = 0;
    s32 s = 4;
    f64 f[5] = {LoadF(kPool), LoadF(kPool), LoadF(kPool + 4), LoadF(kPool + 8), LoadF(kPool + 12)};
    if (tmpl != 0) {
        if (load<u32>(tmpl + 24) & 0x800u) {
            h1 = load<u32>(tmpl + 140);
            s = load<s32>(tmpl + 144);
            for (u32 k = 0; k < 5; ++k) f[k] = LoadF(tmpl + 148 + 4 * k);
            h2 = load<u32>(tmpl + 168);
        } else {
            call(ctx, m11fn::XmlCursor_DocPath, {a0});
        }
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x3559A5ABu, o + 76, h1});
    call(ctx, xfn::XmlCursor_ReadS32, {c, 0xBEC3D7F3u, o + 80, static_cast<u64>(static_cast<s64>(s))});
    static constexpr u32 kF[5] = {0x75DF19EEu, 0x807548E2u, 0x9E18283Eu, 0x9E2E88C2u, 0x05F9141Fu};
    for (u32 k = 0; k < 5; ++k) callf(ctx, xfn::XmlCursor_ReadF32, {c, kF[k], o + 84 + 4 * k}, {f[k]});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x10E70B4Du, o + 104, h2});
}
