// Game code, large-function sweep part 22: the slot launcher's reset and
// respawn (callee of f_000449A8 / f_00046718; Obj_LaunchSlots43784 at the end).
#include "big22.h"

namespace sly4::obj {

namespace {
constexpr u32 kSlotTime = 0x00043BB8u;  // f32 the new effects' start time
constexpr u32 kTwHold = 0x00F95E6Cu;    // tweak records (f32 value +72)
constexpr u32 kTwStep = 0x00F95F04u;
constexpr u32 kTwLift = 0x00F95F9Cu;
constexpr u32 Obj_LaunchSlots = 0x00043784u;     // (Obj*, u32 k)
constexpr u32 WorldObj_FindById = 0x004A5AE8u;   // (WorldObj*, u32 hash)
constexpr u32 Effect_Spawn = 0x004A5FC8u;        // (u32 name, owner, value, u8 start)
constexpr u32 Effect_FindComponent = 0x004BB4B4u;
constexpr u32 Effect_SetTime = 0x004CF710u;      // (Effect*; f32)
constexpr u32 WorldObj_MarkDirty = 0x0057D000u;
constexpr u32 WorldObj_UpdateLod = 0x0057EE00u;
constexpr u32 Obj_CollectLodGroup = 0x00590FECu;

// One 112-byte slot.
struct Slot {
    Vec4 rows[4];    // +0: row 0 the position, row 3 the raised one
    be_u32 ent;      // +64
    be_u32 parent;   // +68 (+128 its position)
    be_u32 obj;      // +72
    be_u32 ow;       // +76
    be_u32 eff;      // +80
    be_u32 hold;     // +84
    be_f32 step;     // +88
    be_u32 w92;      // +92
    be_u32 w96;      // +96
    u8 armed;        // +100
    pad<11> _101;
};
static_assert(sizeof(Slot) == 112 && offsetof(Slot, ent) == 64 && offsetof(Slot, armed) == 100);
// The vector of slots at +308.
struct SlotVec {
    gp<Slot> data;   // +0
    be_u16 cap;      // +4
    pad<2> _6;
    be_u16 count;    // +8
    be_u16 mark;     // +10
};
struct SlotObj {
    pad<308> _0;
    SlotVec slots;    // +308
    pad<292> _320;
    be_u32 tab;       // +612
    pad<8> _616;
    be_u32 fx;        // +624
    pad<20> _628;
    be_u32 w648;      // +648
    pad<4> _652;
    be_u32 wobj[2];   // +656 / +660
    be_u32 dest[2];   // +664 / +668
    be_u32 holders[4];  // +672 / +676 / +680 / +684
    be_u32 group;     // +688
    be_u32 w692;      // +692
    be_u32 w696;      // +696
    be_u32 sw[2];     // +700 / +704
    pad<8> _708;
    be_u32 rec;       // +716
};
static_assert(offsetof(SlotObj, tab) == 612 && offsetof(SlotObj, fx) == 624 && offsetof(SlotObj, wobj) == 656);
static_assert(offsetof(SlotObj, holders) == 672 && offsetof(SlotObj, sw) == 700 && offsetof(SlotObj, rec) == 716);
// A list {+0 data, +8 (u16) count}.
struct SlotList {
    be_u32 data;     // +0
    pad<4> _4;
    be_u16 count;    // +8
};

f64 Tw(u32 rec) { return lf(gm<be_f32>(rec + 72)); }

// World object wobj[k] (when set) moved to dest[k]'s position (+128) and refreshed.
void PlaceAt(SlotObj* o, u32 k) {
    const u32 w = o->wobj[k];
    if (w == 0) return;
    const u32 s = o->dest[k];
    if (s == 0) return;
    CopyXYZ(gm<Vec3>(w + 64), gm<Vec3>(s + 128));
    call(WorldObj_MarkDirty, w, s + 128);  // r4 left at the source position
    call(WorldObj_UpdateLod, o->wobj[k].get(), 0u, 0u);
}

// The component's +20 set when its vfn[8] reports kind 2.
void SetIfKind2(u32 eff, u32 hash, u32 ent, bool flag) {
    const u32 c = call<u32>(Effect_FindComponent, eff, hash);
    if (c == 0) return;
    if (vcall<u32>(&gm<u8>(c), 32) != 2) return;
    gm<be_u32>(c + 20) = flag ? (gm<u8>(ent + 16) != 0 ? 1u : 0u) : gm<be_u32>(ent + 0).get();
}
}  // namespace

// void Obj_RespawnSlots43BC0(Obj* o): the n = u16 +316 slots (112 bytes,
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
// +660 get vfn[10](1, 1) / (0, 1) and move to +664 / +668, then
// Obj_LaunchSlots43784(o, n). No caller reads r3.
void Obj_RespawnSlots43BC0(SlotObj* o) {
    const u32 n0 = o->slots.count;
    for (u32 i = 0; n0 != 0;) {  // do-while on the live count
        Slot& e = o->slots.data[i];
        e.hold = gm<be_u32>(kTwHold + 72).get();
        const f64 st = Tw(kTwStep);
        const Vec3& sp = gm<Vec3>(e.parent + 128);
        const f64 k = S(static_cast<f64>(static_cast<s64>(static_cast<u64>(n0) - i - 1)));
        e.step = static_cast<f32>(S(st * k));
        CopyXYZ(e.rows[0], sp);
        CopyXYZ(e.rows[3], sp);
        e.rows[3].x = static_cast<f32>(S(lf(e.rows[3].x) + Tw(kTwLift)));
        e.armed = 1;
        ++i;
        if (i >= o->slots.count) break;
    }
    o->w696 = 0u;
    if (const u32 s = o->sw[0]; s != 0) vcall(&gm<u8>(s), 304, 1u);
    if (const u32 s = o->sw[1]; s != 0) vcall(&gm<u8>(s), 304, 0u);
    if (const u32 fx = o->fx; fx != 0) call(fn::Effect_SetHash, fx, 0x8C0A3A7Eu);
    const u32 tab = o->tab;
    o->w648 = 4u;
    o->w692 = 0u;
    if (tab != 0) {
        const SlotList& rec = gm<SlotList>(tab + 116 * o->rec + 92);
        const u32 cnt = rec.count;
        o->w692 = cnt;
        const u32 grp = cnt != 0 ? gm<be_u32>(rec.data + (o->group << 2)).get() : 0u;
        const u32 gn = cnt != 0 ? gm<SlotList>(grp).count.get() : 0u;  // no null check on the group
        if (gn != 0) {
            be_u32& state = global<be_u32>(kGameRandState);
            const u32 seed = state;
            const u32 pick = seed % gn;
            const u32 mix = GameRandMix(seed);
            state = std::rotl(mix, 16);
            global<be_u64>(kGameRandLast) = static_cast<u64>(mix & 0xFFFFu);
            const SlotList& set = gm<SlotList>(gm<be_u32>(gm<SlotList>(grp).data + 4 * pick));
            for (u32 j = 0; j < set.count; ++j) {
                const u32 ent = set.data + 20 * j;
                const s32 kind = static_cast<s32>(gm<be_u32>(ent + 0).get());
                static constexpr u32 kHolder[4] = {3, 1, 2, 0};  // +684 / +676 / +680 / +672
                static constexpr u32 kKey[4] = {0x78C44B5Du, 0xADAA869Eu, 0xA52B5E59u, 0x814373A2u};
                if (kind < 0 || kind > 3) continue;
                const u32 key = kKey[kind];
                const u32 h = o->holders[kHolder[kind]];
                if (h == 0 || o->dest[0] == 0u || o->dest[1] == 0u) continue;
                const u32 parent = vcall<u32>(&gm<u8>(h), 56, 5u);
                const u32 obj = call<u32>(WorldObj_FindById, parent, key);
                const u32 ow = gm<be_u32>(obj + 72);
                const u32 eff = call<u32>(Effect_Spawn, 0x4838D5DCu, parent, 0u, 1u);
                call(Obj_CollectLodGroup, parent, 1u);
                if (eff != 0) {
                    gm<be_u32>(eff + 136) = gm<be_u32>(eff + 136) & ~0x200u;
                    SetIfKind2(eff, 0x3257B96Cu, ent, false);
                    SetIfKind2(eff, 0x71FE8003u, ent, true);
                    call(Effect_SetTime, eff, lf(global<be_f32>(kSlotTime)));
                }
                if (o->slots.count == o->slots.cap)
                    call(fn::PtrVector_Reserve, &o->slots, (static_cast<u32>(o->slots.cap) << 1) & 0x1FFFEu);
                const u32 c = o->slots.count;
                Slot& d = o->slots.data[c];
                for (Vec4& row : d.rows) {  // the zeroed stack rows, 12 bytes each
                    set_bits(row.x, 0u);
                    set_bits(row.y, 0u);
                    set_bits(row.z, 0u);
                }
                d.ent = ent;
                d.parent = parent;
                d.obj = obj;
                d.ow = ow;
                d.eff = eff;
                d.hold = 0u;
                set_bits(d.step, 0u);
                d.w92 = 0u;
                d.w96 = 0u;
                d.armed = 0;
                const u32 n1 = c + 1;
                const u32 hw = o->slots.mark;
                o->slots.count = static_cast<u16>(n1);
                o->slots.mark = static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1);
            }
        }
    }
    if (const u32 w = o->wobj[0]; w != 0) vcall(&gm<u8>(w), 40, 1u, 1u);
    if (const u32 w = o->wobj[1]; w != 0) vcall(&gm<u8>(w), 40, 0u, 1u);
    PlaceAt(o, 0);
    PlaceAt(o, 1);
    call(Obj_LaunchSlots, o, n0);
}

}  // namespace sly4::obj

NATIVE(00043BC0, sly4::obj::Obj_RespawnSlots43BC0)
