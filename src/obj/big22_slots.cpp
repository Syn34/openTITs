// Game code, large-function sweep part 22: the slot launcher's reset and
// respawn (callee of f_000449A8 / f_00046718; Obj_LaunchSlots43784 at the end).
#include "big22.h"

using namespace sly4;
using namespace sly4::big22;

namespace {
constexpr u32 kSlotTime = 0x00043BB8u;  // f32 the new effects' start time
constexpr u32 kTwHold = 0x00F95E6Cu;    // tweak records (f32 value +72)
constexpr u32 kTwStep = 0x00F95F04u;
constexpr u32 kTwLift = 0x00F95F9Cu;

namespace sfn {
constexpr u32 Obj_LaunchSlots = 0x00043784u;     // (Obj*, u32 k)
constexpr u32 WorldObj_FindById = 0x004A5AE8u;   // (WorldObj*, u32 hash)
constexpr u32 Effect_Spawn = 0x004A5FC8u;        // (u32 name, owner, value, u8 start)
constexpr u32 Effect_FindComponent = 0x004BB4B4u;
constexpr u32 Effect_SetTime = 0x004CF710u;      // (Effect*; f32)
constexpr u32 Effect_SetHash = 0x004CF988u;
constexpr u32 PtrVector_Reserve = 0x00551718u;
constexpr u32 WorldObj_MarkDirty = 0x0057D000u;
constexpr u32 WorldObj_UpdateLod = 0x0057EE00u;
constexpr u32 Obj_CollectLodGroup = 0x00590FECu;
}  // namespace sfn

// World object w (when set) moved to src's position (+128) and refreshed.
void PlaceAt(PPUContext& ctx, u32 o, u32 wo, u32 so) {
    const u32 w = load<u32>(o + wo);
    if (w == 0) return;
    const u32 s = load<u32>(o + so);
    if (s == 0) return;
    Cw(w + 64, s + 128);
    Cw(w + 68, s + 132);
    Cw(w + 72, s + 136);
    call(ctx, sfn::WorldObj_MarkDirty, {w, s + 128});  // r4 left at the source position
    call(ctx, sfn::WorldObj_UpdateLod, {load<u32>(o + wo), 0, 0});
}

// The component's +20 set when its vfn[8] reports kind 2.
void SetIfKind2(PPUContext& ctx, u32 eff, u32 hash, u32 ent, bool flag) {
    const u32 c = static_cast<u32>(call(ctx, sfn::Effect_FindComponent, {eff, hash}));
    if (c == 0) return;
    if (static_cast<u32>(VCall(ctx, c, 32, {c})) != 2) return;
    store<u32>(c + 20, flag ? (load<u8>(ent + 16) != 0 ? 1u : 0u) : load<u32>(ent + 0));
}
}  // namespace

// u32 Obj_RespawnSlots43BC0(Obj* o): the n = u16 +316 slots (112 bytes,
// vector +308) are re-armed: +84 = the hold tweak, +88 = step tweak * (n -
// 1 - i), +0..8 / +48..56 = the slot object's (+68) position (+48 raised by
// the lift tweak), u8 +100 = 1. +696 = 0; the switches +700 / +704 get
// vfn[76](1) / (0); the effect +624 gets the hash 0x8C0A3A7E; +648 = 4,
// +692 = 0. With a spawn table +612: its 116-byte record +716 (+92: list,
// u16 +100 count -> +692) picks the group +688, and one draw of the game's
// generator (seed % count) one of its sets. For each 20-byte entry of the
// set with kind 0..3 (holders +684 / +676 / +680 / +672, objects
// 0x78C44B5D / 0xADAA869E / 0xA52B5E59 / 0x814373A2) and the targets +664 /
// +668 set: the holder's vfn[14](5) gives the parent, the object is found
// by id under it, an effect 0x4838D5DC spawned on the parent (u8 start 1)
// with the LOD group refreshed; the effect (clear bit 9 of +136) gets its
// components 0x3257B96C (+20 = the entry's kind) and 0x71FE8003 (+20 = u8
// entry +16 != 0) when they are kind 2, and the start time. A slot {zero
// rows, entry, parent, object, its f32 +72, effect, 0...} is pushed (the
// vector doubles when full; high-water +318). Then the world objects +656 /
// +660 get vfn[10](1, 1) / (0, 1) and move to +664 / +668, and r3 =
// Obj_LaunchSlots43784(o, n).
DECOMP(00043BC0, Obj_RespawnSlots43BC0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n0 = load<u16>(o + 316);
    for (u32 i = 0, off = 0; n0 != 0;) {  // do-while on the live count
        const u32 e = load<u32>(o + 308) + off;
        Cw(e + 84, kTwHold + 72);
        const f64 st = Lf(kTwStep + 72);
        const u32 so = load<u32>(e + 68) + 128;
        const f64 k = S(static_cast<f64>(static_cast<s64>(static_cast<u64>(n0) - i - 1)));
        Sf(e + 88, S(st * k));
        const u32 p0 = load<u32>(so + 0);
        store<u32>(e + 0, p0);
        const u32 p1 = load<u32>(so + 4);
        store<u32>(e + 4, p1);
        const u32 p2 = load<u32>(so + 8);
        store<u32>(e + 48, p0);
        store<u32>(e + 8, p2);
        store<u32>(e + 52, p1);
        store<u32>(e + 56, p2);
        Sf(e + 48, S(Lf(e + 48) + Lf(kTwLift + 72)));
        store<u8>(e + 100, 1);
        ++i;
        off += 112;
        if (i >= load<u16>(o + 316)) break;
    }
    store<u32>(o + 696, 0);
    if (const u32 s = load<u32>(o + 700); s != 0) VCall(ctx, s, 304, {s, 1});
    if (const u32 s = load<u32>(o + 704); s != 0) VCall(ctx, s, 304, {s, 0});
    if (const u32 fx = load<u32>(o + 624); fx != 0) call(ctx, sfn::Effect_SetHash, {fx, 0x8C0A3A7Eu});
    const u32 tab = load<u32>(o + 612);
    store<u32>(o + 648, 4);
    store<u32>(o + 692, 0);
    if (tab != 0) {
        const u32 rec = tab + 116 * load<u32>(o + 716) + 92;
        const u32 cnt = load<u16>(rec + 8);
        store<u32>(o + 692, cnt);
        const u32 grp = cnt != 0 ? load<u32>(load<u32>(rec + 0) + (load<u32>(o + 688) << 2)) : 0;
        const u32 gn = cnt != 0 ? load<u16>(grp + 8) : 0;  // no null check on the group
        if (gn != 0) {
            const u32 seed = load<u32>(kRand + 8);
            const u32 pick = seed % gn;
            const u32 mix = mid15::RandMix15(seed);
            store<u32>(kRand + 8, std::rotl(mix, 16));
            store<u64>(kRand, mix & 0xFFFFu);
            const u32 set = load<u32>(load<u32>(grp + 0) + 4 * pick);
            for (u32 j = 0; j < load<u16>(set + 8); ++j) {
                const u32 ent = load<u32>(set + 0) + 20 * j;
                const s32 kind = static_cast<s32>(load<u32>(ent + 0));
                static constexpr u32 kHolder[4] = {684, 676, 680, 672};
                static constexpr u32 kKey[4] = {0x78C44B5Du, 0xADAA869Eu, 0xA52B5E59u, 0x814373A2u};
                if (kind < 0 || kind > 3) continue;
                const u32 key = kKey[kind];
                const u32 h = load<u32>(o + kHolder[kind]);
                if (h == 0 || load<u32>(o + 664) == 0 || load<u32>(o + 668) == 0) continue;
                const u32 parent = static_cast<u32>(VCall(ctx, h, 56, {h, 5}));
                const u32 obj = static_cast<u32>(call(ctx, sfn::WorldObj_FindById, {parent, key}));
                const u32 ow = load<u32>(obj + 72);
                const u32 eff = static_cast<u32>(call(ctx, sfn::Effect_Spawn, {0x4838D5DCu, parent, 0, 1}));
                call(ctx, sfn::Obj_CollectLodGroup, {parent, 1});
                if (eff != 0) {
                    store<u32>(eff + 136, load<u32>(eff + 136) & ~0x200u);
                    SetIfKind2(ctx, eff, 0x3257B96Cu, ent, false);
                    SetIfKind2(ctx, eff, 0x71FE8003u, ent, true);
                    callf(ctx, sfn::Effect_SetTime, {eff}, {Lf(kSlotTime)});
                }
                if (load<u16>(o + 316) == load<u16>(o + 312))
                    call(ctx, sfn::PtrVector_Reserve, {o + 308, (static_cast<u32>(load<u16>(o + 312)) << 1) & 0x1FFFEu});
                const u32 c = load<u16>(o + 316);
                const u32 d = load<u32>(o + 308) + 112 * c;
                for (u32 r = 0; r < 64; r += 16)  // the zeroed stack rows, 12 bytes each
                    for (u32 w = 0; w < 12; w += 4) store<u32>(d + r + w, 0);
                store<u32>(d + 64, ent);
                store<u32>(d + 68, parent);
                store<u32>(d + 72, obj);
                store<u32>(d + 76, ow);
                store<u32>(d + 80, eff);
                store<u32>(d + 84, 0);
                store<u32>(d + 88, 0);
                store<u32>(d + 92, 0);
                store<u32>(d + 96, 0);
                store<u8>(d + 100, 0);
                const u32 n1 = c + 1;
                const u32 hw = load<u16>(o + 318);
                store<u16>(o + 316, static_cast<u16>(n1));
                store<u16>(o + 318, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
            }
        }
    }
    if (const u32 w = load<u32>(o + 656); w != 0) VCall(ctx, w, 40, {w, 1, 1});
    if (const u32 w = load<u32>(o + 660); w != 0) VCall(ctx, w, 40, {w, 0, 1});
    PlaceAt(ctx, o, 656, 664);
    PlaceAt(ctx, o, 660, 668);
    call(ctx, sfn::Obj_LaunchSlots, {o, n0});
}
