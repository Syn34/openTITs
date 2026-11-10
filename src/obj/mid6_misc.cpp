// Game code, medium sweep part 6: leaf lookups (no calls).
#include "mid6.h"

using namespace sly4;
using namespace sly4::mid6;

namespace {

constexpr u32 kGameRoot = 0x00F64C20;  // ->+36: the current level/scene (+716 536-byte entries, +720 u8 count, +822 u8 slot)
constexpr u32 kSaveData = 0x00F65CA0;  // save data: +4208 u8 profile, 4056-byte profiles (+498 u8[], +544 u8[])

// The pairs of hashes that also match each other (key -> the second one).
u32 HashAlias(u32 k) {
    switch (k) {
    case 0x1D8C2817: return 0x9AA76568;
    case 0x00014120: return 0x1D5B6571;
    case 0xDF0611F9: return 0xB4D78147;
    case 0x88397820: return 0xC92661E2;
    default: return 0;
    }
}

// The first entry of the current scene whose state (by its +502 index in
// the profile's +544 table) is not 2, whose +501 matches the profile's
// value for the scene's slot and whose key +492 matches h (or its alias);
// 0 when none (or h is 0).
u32 SceneEntryFind(u32 h) {
    const u32 g = load<u32>(load<u32>(kGameRoot) + 36);
    if (g == 0) return 0;
    const u32 w = load<u32>(kSaveData);
    const u32 prof = load<u8>(w + 4208) * 4056u;
    const u32 cur = load<u8>(w + prof + (static_cast<u32>(load<u8>(g + 822)) << 2) + 498);
    const u32 n = load<u8>(g + 720);
    u32 e = load<u32>(g + 716);
    for (u32 i = 0; i < n; ++i, e += 536) {
        if (load<u8>(w + prof + load<u8>(e + 502) + 544) == 2) continue;
        if (load<u8>(e + 501) != cur) continue;
        if (h == 0 || e == 0) continue;
        const u32 k = load<u32>(e + 492);
        if (k == 0) continue;
        const u32 alias = HashAlias(k);
        if (alias != 0 ? (h == k || h == alias) : h == k) return e;
    }
    return 0;
}

}  // namespace

// bool Scene_HasEntry_002E16E8(a0, u32 hash): SceneEntryFind(hash) != 0.
DECOMP(002E16E8, Scene_HasEntry_002E16E8) {
    ctx.gpr[3] = SceneEntryFind(arg<u32>(ctx, 1)) != 0 ? 1 : 0;
}

// Entry* Scene_FindEntry_002E1900(a0, u32 hash): SceneEntryFind(hash).
DECOMP(002E1900, Scene_FindEntry_002E1900) {
    ctx.gpr[3] = SceneEntryFind(arg<u32>(ctx, 1));
}

// s32 Kind_FromHash_003557B8(u32 hash): the index (0..28) of a hash name,
// 23 for any other.
DECOMP(003557B8, Kind_FromHash_003557B8) {
    static constexpr std::pair<u32, u32> kKinds[] = {
        {0x03DE9E33, 0},  {0x9260C26E, 1},  {0x03FCEF54, 2},  {0x4FDDAF96, 3},  {0xA970EC1F, 4},  {0x77F51D7B, 5},
        {0xA77EB5E9, 6},  {0xA5E5FC40, 7},  {0x8CD18EE1, 8},  {0x03F0D768, 9},  {0x0A210FE7, 10}, {0x3A00E74D, 11},
        {0xB7BC028F, 12}, {0x4D0D4D5E, 13}, {0xD0F5D7B2, 14}, {0x16E57A97, 15}, {0x760CB725, 16}, {0x6E2FCD45, 17},
        {0xA0D734D0, 18}, {0x8CC1BC14, 19}, {0x57C3E0EE, 20}, {0x6E2FC83F, 21}, {0x6E2F94E9, 22}, {0x34BCCF8D, 23},
        {0x04E8CCF7, 24}, {0x05C3A339, 25}, {0x3ECC2A7C, 26}, {0x6A3FDA1F, 27}, {0x786E8593, 28},
    };
    const u32 h = arg<u32>(ctx, 0);
    u32 r = 23;
    for (const auto& [k, v] : kKinds)
        if (k == h) r = v;
    ctx.gpr[3] = r;
}

namespace {

constexpr u32 kRand = 0x01164888;  // {u64 last value, u32 state}

// The game's small RNG: returns state % n (n > 1), steps the state and
// stores {u64 low 16 bits, state}.
u32 RandMod(u32 state, u32 n, u32& next) {
    const u32 r = state - (state / n) * n;
    const u32 t = (std::rotl(state, 4) + std::rotl(state, 16)) ^ 0x87654321u;
    next = std::rotl(t, 16);
    store<u32>(kRand + 8, next);
    store<u64>(kRand, t & 0xFFFFu);
    return r;
}

// A {+4 u32: top two bits = mode, +8: count << 4, +16 u32 values[]}
// choice: values[0], or a random one in mode 1 with more than one.
u32 ChoicePick(u32 t, u32 n, u32& state, bool cached) {
    u32 idx = 0;
    if ((load<u32>(t + 4) >> 30) == 1 && static_cast<s32>(n) > 1) {
        if (!cached) state = load<u32>(kRand + 8);
        idx = RandMod(state, n, state);
    }
    return load<u32>(load<u32>(t + 16) + 4 * idx);
}

}  // namespace

// bool Choice_Pick_00288974(Obj* o, u32* out, u32 key): *out = 0; for
// each source i of o->+24 (+36 count, +40 ids, +44 tables {+4 count, +8
// keys, +12 choices}) whose id is o->+28, the first choice under key with
// a count; the first non-zero pick wins (1). When none did, source 0 is
// tried as a fallback when its id is not o->+28. Returns 0 otherwise.
DECOMP(00288974, Choice_Pick_00288974) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    store<u32>(out, 0);
    u32 p = load<u32>(o + 24);
    ctx.gpr[3] = 0;
    if (p == 0) return;
    u32 last = 0;
    if (load<u32>(p + 36) != 0) {
        u32 state = load<u32>(kRand + 8);
        for (u32 i = 0; i < load<u32>(p + 36); ++i) {
            if (load<u32>(load<u32>(p + 40) + 4 * i) != load<u32>(o + 28)) continue;
            const u32 s = load<u32>(load<u32>(p + 44) + 4 * i);
            const u32 cnt = load<u32>(s + 4);
            for (u32 j = 0; j < cnt; ++j) {
                if (load<u32>(load<u32>(s + 8) + 4 * j) != key) continue;
                const u32 t = load<u32>(load<u32>(s + 12) + 4 * j);
                const u32 n = load<u32>(t + 8) >> 4;
                if (static_cast<s32>(n) <= 0) continue;
                last = ChoicePick(t, n, state, true);
                store<u32>(out, last);
                if (last != 0) {
                    ctx.gpr[3] = 1;
                    return;
                }
                p = load<u32>(o + 24);
                break;
            }
        }
        if (last != 0) return;
    }
    // source 0 once more
    if (load<u32>(p + 36) == 0) return;
    if (load<u32>(load<u32>(p + 40)) == load<u32>(o + 28)) return;
    const u32 s = load<u32>(load<u32>(p + 44));
    const u32 cnt = load<u32>(s + 4);
    for (u32 j = 0; j < cnt; ++j) {
        if (load<u32>(load<u32>(s + 8) + 4 * j) != key) continue;
        const u32 t = load<u32>(load<u32>(s + 12) + 4 * j);
        const u32 n = load<u32>(t + 8) >> 4;
        if (static_cast<s32>(n) <= 0) continue;
        u32 state = 0;
        const u32 v = ChoicePick(t, n, state, false);
        store<u32>(out, v);
        ctx.gpr[3] = v != 0 ? 1 : 0;
        return;
    }
}

// void Flags_Latch_003BE554(Obj* o, const Input* in): each of the ten
// latch bytes o->+1532.. becomes set (and stays set) when the matching
// bit of in->+8 & in->+12 is set.
DECOMP(003BE554, Flags_Latch_003BE554) {
    static constexpr u32 kMasks[10] = {0x100, 0x200, 0x4, 0x8, 0x1, 0x2, 0x80, 0x10, 0x40, 0x10};
    const u32 o = arg<u32>(ctx, 0), in = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 10; ++k) {
        const bool v = load<u8>(o + 1532 + k) != 0 || ((load<u32>(in + 8) & load<u32>(in + 12)) & kMasks[k]) != 0;
        store<u8>(o + 1532 + k, v ? 1 : 0);
    }
}

// void Records_RemoveKey_0029F7A0(Owner* o, u32 key): the 128-byte
// records o->+112.. (count +2160) with +8 == key are removed by moving the
// last record into their slot, field by field (+12 and +28 are not
// copied; bits 31..21 of +120 are, the rest stay). Quirk: the record
// moved in is not checked again.
DECOMP(0029F7A0, Records_RemoveKey_0029F7A0) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    for (s32 i = 0; i < static_cast<s32>(load<u32>(o + 2160)); ++i) {
        const u32 dst = o + 112 + 128 * static_cast<u32>(i);
        if (load<u32>(dst + 8) != key) continue;
        const u32 n = load<u32>(o + 2160) - 1;
        store<u32>(o + 2160, n);
        const u32 src = o + 112 + (n << 7);
        for (u32 k = 0; k < 4; ++k) store<u8>(dst + k, load<u8>(src + k));
        for (u32 off : {4u, 16u, 8u, 20u, 36u, 24u, 32u, 40u}) store<u32>(dst + off, load<u32>(src + off));
        for (u32 off = 44; off <= 116; off += 4) store<u32>(dst + off, load<u32>(src + off));
        u32 w = (load<u32>(dst + 120) & 0x7FFFFFFFu) | (load<u32>(src + 120) & 0x80000000u);
        store<u32>(dst + 120, w);
        for (u32 m = 0x40000000u; m >= 0x00200000u; m >>= 1) {
            w = (w & ~m) | (load<u32>(src + 120) & m);
            store<u32>(dst + 120, w);
        }
        store<u32>(dst + 124, load<u32>(src + 124));
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(src + 124)));  // the lfs/stfs pairs' leftover
    }
}

// u32 NodeVec_Apply_003955EC(Node* n, Ctx* c): with the target c->+12,
// its +224 / +228 / +232 / +240 = the node's float parameters 0..3 and
// +176 |= 0x800. Returns 0.
DECOMP(003955EC, NodeVec_Apply_003955EC) {
    const u32 n = arg<u32>(ctx, 0), t = load<u32>(arg<u32>(ctx, 1) + 12);
    ctx.gpr[3] = 0;
    if (t == 0) return;
    // The node's two records (128-byte entries of the table 0x00F65038->+24,
    // indices s16 +2 / +4, -1 none = address 0): parameter k is the f32 at
    // record(+4) + *(u16*)0x0106CF70 + 4 * the k-th index byte of
    // record(+2) + *(u16*)0x0106CF7E (when its +2 byte is set).
    const u32 tab = load<u32>(0x00F65038);
    const s32 ia = static_cast<s16>(load<u16>(n + 2)), iv = static_cast<s16>(load<u16>(n + 4));
    const auto rec = [&](s32 i) { return i == -1 ? 0u : load<u32>(tab + 24) + (static_cast<u32>(i) << 7); };
    const auto param = [&](u32 k) {
        const u32 r = rec(ia);
        const u32 q = load<u8>(r + 2) != 0 ? r + load<u16>(0x0106CF7E) : 0;  // quirk: address 0 without the byte
        return load<u32>(rec(iv) + load<u16>(0x0106CF70) + (static_cast<u32>(load<u8>(q + k)) << 2));
    };
    u32 p[4];
    for (u32 k = 0; k < 4; ++k) p[k] = param(k);
    const u32 fl = load<u16>(t + 176);
    store<u32>(t + 224, p[0]);
    store<u32>(t + 228, p[1]);
    store<u32>(t + 232, p[2]);
    store<u32>(t + 240, p[3]);
    store<u16>(t + 176, static_cast<u16>(fl | 0x800));
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(p[2]));
}
