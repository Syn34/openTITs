// World objects, part 8: Comps_Query, which reads an object's component
// property tables into its peer record.
#include <bit>

#include "obj8.h"

using namespace sly4;
using namespace sly4::obj8;

namespace {

constexpr u32 kQueryAnimA = 0x00593630;  // (peer, comps, s32 comp, s32 channel 0..2, bool) -> bool (name?)
constexpr u32 kQueryAnimB = 0x00592DA8;  // (peer, comps, s32 comp, s32 channel 0..2, bool) -> bool: "%sPos..." (name?)
constexpr u32 kQueryAnimC = 0x00592890;  // (peer, comps, s32 comp, bool) -> bool (name?)

// Constants at 0x00593D94: -1 (no range), 0, 25 (the default range), 5.
constexpr u32 kQc = 0x00593D94;
constexpr u32 kQcNone = 0, kQcZero = 4, kQcDefault = 8, kQcInner = 12;

constexpr u32 kKeyCastShadow = 0x9E0593D2;  // u8 (name?)
constexpr u32 kKeyFlagA = 0x363B25FF;       // u8, component A only (name?)
constexpr u32 kKeyFlagB = 0x3ECC2A7C;       // u8 (name?)

// The peer record's +16 range / +20 range^2 / +24 (range - 5)^2 and +28 bits.
constexpr u32 kPrRange = 16, kPrRangeSq = 20, kPrInnerSq = 24, kPrBits = 28;
constexpr u32 kPrBitShadow = 0x10, kPrBitAny = 0x20, kPrBitFlags = 0x40;
constexpr u32 kPeerBits = 156;  // u8 in the 160-byte peer record: 0x40 as kPrBitFlags

inline f32 F(u32 bits) { return std::bit_cast<f32>(bits); }
inline f32 Subs(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - static_cast<f64>(b)); }
inline f32 Muls(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * static_cast<f64>(b)); }

// fsel(a - b >= 0 ? a : b) on the raw words (the selected value is stored
// unchanged).
inline u32 SelMax(u32 a, u32 b) { return Subs(F(a), F(b)) >= 0.0f ? a : b; }

// The index of the first component of type `type` (comps +4, four slots;
// a null slot ends the search), or -1.
s32 FindComp(u32 comps, u32 type) {
    for (u32 i = 0; i < 4; ++i) {
        const u32 c = load<u32>(comps + 4 + i * 4);
        if (c == 0) return -1;
        if (load<u32>(c + 8) == type) return static_cast<s32>(i);
    }
    return -1;
}

inline u32 Table(u32 comps, s32 i) { return load<u32>(comps + 4 + static_cast<u32>(i) * 4); }

// A u8 property of component i (0 when i < 0 or it has none).
u32 ByteProp(u32 comps, s32 i, u32 key) {
    if (i < 0) return 0;
    const u32 v = PropFind(Table(comps, i), key);
    return v != 0 ? load<u8>(v) : 0;
}

}  // namespace

// bool Comps_Query(PeerRecord* peer, Comps* comps, bool anim_only)
// Runs the animation queries of the first components of the three known
// types (obj6.h's kCompTypeA..C: three channels each for A and B, one for
// C) and returns whether any of them reported something. Unless anim_only,
// also fills the peer's vector record: the largest range property of the
// three (25 when none is positive) with its square and (range - 5)^2, and
// the bits 0x10 (the first component's "cast shadow" byte, bit 0), 0x40
// (that or component A's flag A; also at peer +156) and 0x20 (any flag B).
DECOMP(00593DA8, Comps_Query) {
    const u32 peer = arg<u32>(ctx, 0), comps = arg<u32>(ctx, 1);
    const u32 anim_only = arg<u32>(ctx, 2) & 0xFF;
    bool any = false;
    const auto acc = [&](u64 r) { any = any || static_cast<u32>(r) != 0; };

    const s32 ia = FindComp(comps, load<u32>(obj6::kCompTypeA));
    if (ia >= 0)
        for (u64 ch = 0; ch < 3; ++ch) acc(call(ctx, kQueryAnimA, {peer, comps, static_cast<u64>(static_cast<s64>(ia)), ch, anim_only}));
    const s32 ib = FindComp(comps, load<u32>(obj6::kCompTypeB));
    if (ib >= 0)
        for (u64 ch = 0; ch < 3; ++ch) acc(call(ctx, kQueryAnimB, {peer, comps, static_cast<u64>(static_cast<s64>(ib)), ch, anim_only}));
    const s32 ic = FindComp(comps, load<u32>(obj6::kCompTypeC));
    if (ic >= 0) acc(call(ctx, kQueryAnimC, {peer, comps, static_cast<u64>(static_cast<s64>(ic)), anim_only}));
    if (anim_only != 0) {
        ret(ctx, any ? 1u : 0u);
        return;
    }

    // The range: the largest of the three range properties (-1 for a
    // missing one), the default when it is negative.
    const u32 none = load<u32>(kQc + kQcNone);
    const auto range_of = [&](s32 i) {
        if (i < 0) return none;
        const u32 v = PropFind(Table(comps, i), kKeyLightRange);
        return v != 0 ? load<u32>(v) : none;
    };
    const u32 ra = range_of(ia), rb = range_of(ib), rc = range_of(ic);
    u32 r = SelMax(none, ra);
    r = SelMax(r, rb);
    r = SelMax(r, rc);
    if (F(r) < load<f32>(kQc + kQcZero)) r = load<u32>(kQc + kQcDefault);
    u32 rec = PeerVec(peer);
    store<u32>(rec + kPrRange, r);
    store<f32>(rec + kPrRangeSq, Muls(F(r), F(r)));
    const f32 d = Subs(F(r), load<f32>(kQc + kQcInner));
    rec = PeerVec(peer);
    store<f32>(rec + kPrInnerSq, Muls(d, d));

    // The shadow byte comes from the first of A, B, C that has one.
    u32 shadow = 0;
    for (s32 i : {ia, ib, ic}) {
        if (i < 0) continue;
        const u32 v = PropFind(Table(comps, i), kKeyCastShadow);
        if (v != 0) {
            shadow = load<u8>(v);
            break;
        }
    }
    rec = PeerVec(peer);
    SetBit8(rec + kPrBits, kPrBitShadow, (shadow & 1) != 0);
    const u32 fa = ByteProp(comps, ia, kKeyFlagA);
    const bool flags = (shadow | fa) != 0;
    rec = PeerVec(peer);
    SetBit8(rec + kPrBits, kPrBitFlags, flags);
    SetBit8(peer + kPeerBits, kPrBitFlags, flags);
    const u32 b0 = ByteProp(comps, ia, kKeyFlagB);
    const u32 b1 = ByteProp(comps, ib, kKeyFlagB);
    const u32 b2 = ByteProp(comps, ic, kKeyFlagB);
    rec = PeerVec(peer);
    SetBit8(rec + kPrBits, kPrBitAny, b0 != 0 || b1 != 0 || b2 != 0);
    ret(ctx, any ? 1u : 0u);
}
