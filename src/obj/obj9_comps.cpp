// World objects, part 9: the component animation queries Comps_Query runs
// (0x00593630 / 0x00592DA8 / 0x00592890), which set up the peer record's
// animation blocks (+128 / +132 / +136) from a component's property table.
#include "obj9.h"

using namespace sly4;
using namespace sly4::obj8;
using obj9::Lf;

namespace {

constexpr u32 kSplineMap_Find = 0x007335C4;  // (map, const u32* name hash) -> ApeSplineDef** slot (name?)
constexpr u32 kSplineHolder = 0x00F8F158;    // header*: +16 the spline definitions by name hash (obj8::kSplineInstList)
constexpr u32 kHolderMap = 16;
constexpr u32 kAnimAllocator = 4;            // *kSlotPeers +4: the allocator of the peers' animation blocks (name?)
constexpr u32 kVfnAllocAlloc = 12;

// ---------------------------------------------------------------- the spline animation block (peer +136, 112 bytes)
constexpr u32 kPeerSplineAnim = 136;
constexpr u32 kSplineAnimSize = 112;
constexpr u32 kSaMatrix = 0;          // f32[4][4]: the identity rows' xyz at first
constexpr u32 kSa64 = 64;             // f32 (key 0x273C260F) (name?)
constexpr u32 kSaRunning = 72;        // f32: 1, or 0 without a duration (name?)
constexpr u32 kSaRate = 76;           // f32: 1 / duration (or the class +120 / duration with 0x40)
constexpr u32 kSaSpline = 80;         // ApeSplineDef*: the path
constexpr u32 kSaSpline2 = 84;        // ApeSplineDef*: a second spline (key 0x45187468) (name?)
constexpr u32 kSaDuration = 88;       // f32 (key 0xB7EA3D94)
constexpr u32 kSaStart = 92, kSaTime = 96;  // f32 (key 0x03DEC163) both (name?)
constexpr u32 kSaRecord = 100, kSaRecord2 = 104;  // the splines' 96-byte records (obj7::kSplineList), or 0
constexpr u32 kSaFlags = 108;         // u8: 0x80 from key 0x8C42E601 bit 0, 0x40 = rate from the class (name?)
constexpr u32 kSaFlagLoop = 0x80, kSaFlagClassRate = 0x40;

constexpr u32 kKeySplineName = 0x00255C25;
constexpr u32 kKeySpline2Name = 0x45187468;
constexpr u32 kKey64 = 0x273C260F;
constexpr u32 kKeyDuration = 0xB7EA3D94;
constexpr u32 kKeyStart = 0x03DEC163;
constexpr u32 kKeyLoop = 0x8C42E601;
constexpr u32 kQcConsts = 0x00592884;  // f32 {0, 1}
constexpr u32 kClassRate = 120;        // f32 in the spline's class (+208)

// ---------------------------------------------------------------- the oscillation blocks
// Peer +128 (rotation, Comps_QueryAnimA) and +132 (position,
// Comps_QueryAnimB): three channels (x, y, z), the fields as parallel
// f32[3] arrays. The first one taken uses the peer's inline block at +64
// (+156 0x10 marks it used).
constexpr u32 kPeerRotAnim = 128, kPeerPosAnim = 132, kPeerInlineAnim = 64;
constexpr u32 kPeerBits = 156, kPeerInlineUsed = 0x10;
constexpr u32 kOaStart = 0, kOaPeriod = 12, kOaDelay = 24, kOaOffset = 36, kOaAmplitude = 48;  // f32[3] each
constexpr u32 kOaFlags = 60;          // u8[3]: bit 0 cleared on set-up, bits 1-2: 1 loop, 2 ping-pong
constexpr u32 kOaChannels = 3;
constexpr u32 kOaPingPong = 2;
constexpr u32 kOaConsts[2] = {0x0059361C, 0x00592D98};  // (A, B): f32 {0, 360, 0.5, 1/65535}
constexpr u32 kOcZero = 0, kOcFullTurn = 4, kOcHalf = 8, kOcInv65535 = 12;
// Comps_QueryAnimA's keys per channel, in its lookup order: ping-pong,
// offset, amplitude, period, start time, delay and random start (the
// hashes of names like Comps_QueryAnimB's "%sPos..." ones).
constexpr u32 kRotKeys[3][7] = {
    {0x687C6CC5, 0xA42F1FAE, 0x0887CB68, 0xA5DB30DC, 0x3DDA4514, 0x5F872AE8, 0xFCCBCE07},
    {0x4A5A3664, 0xABE0C50D, 0x626335A9, 0xAD8CD63B, 0x97B5AF55, 0xF46BDDA9, 0x047D7366},
    {0x2C380003, 0xB3926A6C, 0xBC3E9FEA, 0xB53E7B9A, 0xF1911996, 0x8950906A, 0x0C2F18C5},
};
// Comps_QueryAnimB's key names, formatted with "x" / "y" / "z".
constexpr u32 kPosKeyFormats[7] = {0x00AF4FB4, 0x00AF4FC4, 0x00AF4FD4, 0x00AF4FE4, 0x00AF4FF4, 0x00AF5004, 0x00AF5014};
constexpr u32 kStrX = 0x00AF4F9C, kStrY = 0x00AF4FA4, kStrZ = 0x00AF4FAC;
constexpr u32 kString_Format = 0x005534D0;  // (const char* fmt, ...) -> a formatted string

inline u32 Table(u32 comps, u32 i) { return load<u32>(comps + 4 + i * 4); }

// The spline definition named by the u32 hash at `v` (0 when unknown); the
// hash goes through the stack slot at `tmp`.
u32 FindSpline(PPUContext& ctx, u32 v, u32 tmp) {
    store<u32>(tmp, load<u32>(v));
    const u32 map = load<u32>(load<u32>(kSplineHolder) + kHolderMap);
    return load<u32>(static_cast<u32>(call(ctx, kSplineMap_Find, {map, tmp})));
}

// The spline record of definition d (its +284 index into obj7::kSplineList), or 0.
u32 SplineRecord(u32 d) {
    const u32 idx = load<u32>(d + obj7::kObjSplineIndex);
    const u32 count = load<u16>(load<u32>(obj7::kSplineList + kSlHeader) + 2);
    if (idx == ~0u || idx >= count) return 0;
    return load<u32>(obj7::kSplineList + kSlRecs) + idx * obj5::kK13RecSize;
}

}  // namespace

// bool Comps_QueryAnimC(PeerRecord* peer, Comps* comps, s32 comp, bool anim_only)
// For a component naming a known spline (key 0x00255C25): true; unless
// anim_only, also fills the peer's spline animation block (+136, made on
// first use from the peers' allocator: zeroed, identity rows): the spline
// and its record, the optional second spline, and the 0x273C260F /
// duration / start / loop properties; the rate is 1 / duration (the
// spline class's +120 / duration with flag 0x40), both 0 without a
// duration. False when the component has no such spline.
DECOMP(00592890, Comps_QueryAnimC) {
    const u32 peer = arg<u32>(ctx, 0), comps = arg<u32>(ctx, 1);
    const u32 ci = arg<u32>(ctx, 2);
    const u32 anim_only = arg<u32>(ctx, 3) & 0xFF;
    Stack frame(ctx, 160);
    const u32 sp = frame.addr();
    const u32 v = PropFind(Table(comps, ci), kKeySplineName);
    if (v == 0) return ret(ctx, 0u);
    const u32 s = FindSpline(ctx, v, sp + 116);
    if (s == 0) return ret(ctx, 0u);
    if (anim_only != 0) return ret(ctx, 1u);
    u32 b = load<u32>(peer + kPeerSplineAnim);
    if (b == 0) {
        const u32 alloc = load<u32>(load<u32>(obj3::kSlotPeers) + kAnimAllocator);
        b = static_cast<u32>(obj4::Vcall0(ctx, alloc, kVfnAllocAlloc));
        store<u32>(peer + kPeerSplineAnim, b);
        call(ctx, obj4::o4fn::memset, {b, 0, kSplineAnimSize});
        for (u32 row = 0; row < 64; row += 16)
            for (u32 k = 0; k < 12; k += 4) store<u32>(b + kSaMatrix + row + k, load<u32>(0x01164BD0u + row + k));
        b = load<u32>(peer + kPeerSplineAnim);
    }
    store<u32>(b + kSaSpline, s);
    store<u32>(b + kSaRecord, SplineRecord(s));

    if (const u32 v2 = PropFind(Table(comps, ci), kKeySpline2Name); v2 != 0) {
        const u32 s2 = FindSpline(ctx, v2, sp + 112);
        store<u32>(b + kSaSpline2, s2);
        if (s2 != 0) store<u32>(b + kSaRecord2, SplineRecord(s2));
    }
    const u32 t = Table(comps, ci);
    if (const u32 p = PropFind(t, kKey64); p != 0) store<u32>(b + kSa64, load<u32>(p));
    if (const u32 p = PropFind(t, kKeyDuration); p != 0) store<u32>(b + kSaDuration, load<u32>(p));
    if (const u32 p = PropFind(t, kKeyStart); p != 0) {
        store<u32>(b + kSaStart, load<u32>(p));
        store<u32>(b + kSaTime, load<u32>(p));
    }
    if (const u32 p = PropFind(t, kKeyLoop); p != 0)
        store<u8>(b + kSaFlags, static_cast<u8>((load<u8>(b + kSaFlags) & ~kSaFlagLoop) | ((load<u8>(p) & 1) << 7)));

    const f32 zero = Lf(kQcConsts), one = Lf(kQcConsts + 4);
    const f32 dur = Lf(b + kSaDuration);
    f32 running = zero, rate = zero;
    if (dur != zero) {
        running = one;
        const f32 num = (load<u8>(b + kSaFlags) & kSaFlagClassRate) != 0
                            ? Lf(load<u32>(load<u32>(b + kSaSpline) + obj4::kObjClassPtr) + kClassRate)
                            : one;
        rate = obj9::Fdivs(num, dur);
    }
    store<f32>(b + kSaRate, rate);
    store<f32>(b + kSaRunning, running);
    ctx.fpr[1] = static_cast<f64>(running);  // f1 as the original leaves it
    ret(ctx, 1u);
}

namespace {

// One oscillation channel as read from a component's properties (raw f32
// words), with what the original leaves in f1 when the reader is
// Comps_QueryAnimB.
struct Osc {
    u32 start, period, delay, offset, amp, type;
    bool any;
    f64 f1;
};

// The shared body of Comps_QueryAnimA / B: the seven lookups (skipped for
// a negative component): ping-pong (type 2, amplitude halved), offset,
// amplitude, period (offset 0 when the period is 0), start time, delay and
// random start (start = random / 65535 * (delay + period), the inlined
// random stepped). `any`: one of them nonzero (an amplitude of 360 counts
// as 0).
Osc ReadOsc(u32 comps, s32 ci, const u32 keys[7], u32 consts, f64 f1) {
    const u32 zero = load<u32>(consts + kOcZero);
    Osc o{zero, zero, zero, zero, zero, 1, false, f1};
    if (ci >= 0) {
        const u32 ui = static_cast<u32>(ci);
        if (const u32 p = PropFind(Table(comps, ui), keys[0]); p != 0 && load<u8>(p) != 0) o.type = kOaPingPong;
        if (const u32 p = PropFind(Table(comps, ui), keys[1]); p != 0) o.offset = load<u32>(p);
        if (const u32 p = PropFind(Table(comps, ui), keys[2]); p != 0) {
            o.amp = load<u32>(p);
            if (o.type == kOaPingPong) {
                o.f1 = static_cast<f64>(Lf(consts + kOcHalf));
                o.amp = std::bit_cast<u32>(obj9::Fmuls(std::bit_cast<f32>(o.amp), Lf(consts + kOcHalf)));
            }
        }
        if (const u32 p = PropFind(Table(comps, ui), keys[3]); p != 0) {
            o.period = load<u32>(p);
            if (std::bit_cast<f32>(o.period) == std::bit_cast<f32>(o.start)) o.offset = o.start;
        }
        if (const u32 p = PropFind(Table(comps, ui), keys[4]); p != 0) o.start = load<u32>(p);
        if (const u32 p = PropFind(Table(comps, ui), keys[5]); p != 0) o.delay = load<u32>(p);
        if (const u32 p = PropFind(Table(comps, ui), keys[6]); p != 0 && load<u8>(p) != 0) {
            const f32 range = obj9::Fadds(std::bit_cast<f32>(o.delay), std::bit_cast<f32>(o.period));
            // The inlined random: the previous value (0..65535), the state stepped.
            const u64 last = load<u64>(obj6::kRandomLast);
            const u32 s = load<u32>(obj6::kRandomState);
            const u32 mix = (std::rotl(s, 4) + std::rotl(s, 16)) ^ obj6::kRandomMix;
            store<u32>(obj6::kRandomState, std::rotl(mix, 16));
            store<u64>(obj6::kRandomLast, mix & 0xFFFFu);
            const f32 r = obj9::Fmuls(static_cast<f32>(static_cast<f64>(static_cast<s64>(last))), Lf(consts + kOcInv65535));
            o.f1 = static_cast<f64>(r);
            o.start = std::bit_cast<u32>(obj9::Fmuls(r, range));
        }
    }
    const f32 fz = std::bit_cast<f32>(zero);
    const f32 fa = std::bit_cast<f32>(o.amp);
    o.any = std::bit_cast<f32>(o.start) != fz || std::bit_cast<f32>(o.period) != fz ||
            std::bit_cast<f32>(o.delay) != fz || std::bit_cast<f32>(o.offset) != fz;
    if (!o.any && fa != fz) {
        o.f1 = static_cast<f64>(Lf(consts + kOcFullTurn));
        o.any = fa != Lf(consts + kOcFullTurn);
    }
    return o;
}

// The zeroed three channels of a new block (type 2 each).
void InitOscBlock(u32 blk, u32 zero) {
    for (u32 i = 0; i < kOaChannels; ++i) {
        for (u32 off : {kOaStart, kOaPeriod, kOaDelay, kOaOffset, kOaAmplitude}) store<u32>(blk + off + i * 4, zero);
        store<u8>(blk + kOaFlags + i, 2);
    }
}

// Channel ch of the block at *slot (re-read for every field, as the
// original does for the position block).
void StoreOsc(u32 slot, s32 ch, const Osc& o) {
    const u32 k = static_cast<u32>(ch) * 4;
    store<u32>(load<u32>(slot) + kOaStart + k, o.start);
    store<u32>(load<u32>(slot) + kOaPeriod + k, o.period);
    store<u32>(load<u32>(slot) + kOaDelay + k, o.delay);
    store<u32>(load<u32>(slot) + kOaOffset + k, o.offset);
    store<u32>(load<u32>(slot) + kOaAmplitude + k, o.amp);
    const u32 fl = load<u32>(slot) + kOaFlags + static_cast<u32>(ch);
    store<u8>(fl, static_cast<u8>((load<u8>(fl) & 1) | ((o.type << 1) & 6)));
    const u32 fl2 = load<u32>(slot) + kOaFlags + static_cast<u32>(ch);
    store<u8>(fl2, static_cast<u8>(load<u8>(fl2) & ~1u));
}

}  // namespace

// bool Comps_QueryAnimA(PeerRecord* peer, Comps* comps, s32 comp, s32 channel, bool anim_only)
// The rotation oscillation of one channel (0, 1, anything else as 2) from
// the component's properties (ReadOsc); true when it does anything, and
// unless anim_only it is then stored in the peer's rotation block (+128;
// on first use the inline one at +64, all three channels zeroed).
// Quirk kept: with +156 0x10 already set but no block, the set-up writes
// through address 0.
DECOMP(00593630, Comps_QueryAnimA) {
    const u32 peer = arg<u32>(ctx, 0), comps = arg<u32>(ctx, 1);
    const s32 ci = arg<s32>(ctx, 2), ch = arg<s32>(ctx, 3);
    const u32 consts = kOaConsts[0];
    const Osc o = ReadOsc(comps, ci, kRotKeys[ch == 0 ? 0 : ch == 1 ? 1 : 2], consts, 0.0);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(o.start));  // f1 as the original leaves it
    if (!o.any || (arg<u32>(ctx, 4) & 0xFF) != 0) return ret(ctx, o.any ? 1u : 0u);
    u32 blk = load<u32>(peer + kPeerRotAnim);
    if (blk == 0) {
        const u32 bits = load<u8>(peer + kPeerBits);
        if ((bits & kPeerInlineUsed) == 0) {
            blk = peer + kPeerInlineAnim;
            store<u32>(peer + kPeerRotAnim, blk);
            store<u8>(peer + kPeerBits, static_cast<u8>(bits | kPeerInlineUsed));
        }
        InitOscBlock(blk, load<u32>(consts + kOcZero));
    }
    StoreOsc(peer + kPeerRotAnim, ch, o);
    ret(ctx, 1u);
}

// bool Comps_QueryAnimB(PeerRecord* peer, Comps* comps, s32 comp, s32 channel, bool anim_only)
// As Comps_QueryAnimA for the position oscillation: the keys are the
// hashes of "%sPosPingPong" .. "%sPosRandomStart" with "x" / "y" / "z"
// (channel 1 / 2, anything else "x"), and the block is the peer's +132
// (on first use the inline one at +64 when free, else one from the peers'
// allocator).
DECOMP(00592DA8, Comps_QueryAnimB) {
    const u32 peer = arg<u32>(ctx, 0), comps = arg<u32>(ctx, 1);
    const s32 ci = arg<s32>(ctx, 2), ch = arg<s32>(ctx, 3);
    const u32 anim_only = arg<u32>(ctx, 4) & 0xFF;
    const u32 prefix = ch == 1 ? kStrY : ch == 2 ? kStrZ : kStrX;
    u32 keys[7];
    for (u32 k = 0; k < 7; ++k)
        keys[k] = static_cast<u32>(call(ctx, fn::String_HashNoCase, {call(ctx, kString_Format, {kPosKeyFormats[k], prefix})}));
    const u32 consts = kOaConsts[1];
    const Osc o = ReadOsc(comps, ci, keys, consts, ctx.fpr[1]);
    ctx.fpr[1] = o.f1;  // f1 as the original leaves it
    if (!o.any || anim_only != 0) return ret(ctx, o.any ? 1u : 0u);
    if (load<u32>(peer + kPeerPosAnim) == 0) {
        const u32 bits = load<u8>(peer + kPeerBits);
        u32 blk;
        if ((bits & kPeerInlineUsed) == 0) {
            blk = peer + kPeerInlineAnim;
            store<u8>(peer + kPeerBits, static_cast<u8>(bits | kPeerInlineUsed));
        } else {
            const u32 alloc = load<u32>(load<u32>(obj3::kSlotPeers) + kAnimAllocator);
            blk = static_cast<u32>(obj4::Vcall0(ctx, alloc, kVfnAllocAlloc));
        }
        store<u32>(peer + kPeerPosAnim, blk);
        InitOscBlock(blk, load<u32>(consts + kOcZero));
    }
    StoreOsc(peer + kPeerPosAnim, ch, o);
    ret(ctx, 1u);
}
