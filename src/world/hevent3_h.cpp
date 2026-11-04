// World_HandleEvent callees, part 3: 85..87 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3h {
namespace hhfn {
constexpr u32 SubMachine_Construct = 0x0047C5D8;  // (m, a, b, c, d)
constexpr u32 PtrVector_Construct = 0x005516E0;   // (PtrVector*)
constexpr u32 sub_00445C94 = 0x00445C94;          // (0x010ACC40)
constexpr u32 SortedMap_Find73B608 = 0x0073B608;  // (map, const u32* key) -> value slot (name?)
constexpr u32 sub_002DDAB0 = 0x002DDAB0;          // (Session*)
constexpr u32 Set_Find = 0x004BCE90;              // (u32 name) -> Set*
constexpr u32 sub_004BD1CC = 0x004BD1CC;          // (Set*)
constexpr u32 UserBlock_Reset45C7A8 = 0x0045C7A8; // (UserBlock*) (name?)
constexpr u32 sub_002DB8B0 = 0x002DB8B0;          // (Player*)
constexpr u32 World_StartLevel = 0x004674C4;      // (World*, u32 level, u8 flag) (hevent3_e.cpp)
}  // namespace hhfn

constexpr u32 kSubMachine20748Vtbl = 0x00B20748;  // (name?)
constexpr u32 kF1 = 0x3F800000;

// Emitter pool (game/frame.h).
constexpr u32 kAudioUpdateObj = 0x010ACC40;
constexpr u32 kEmitters = 0x010B0200;
constexpr u32 kPoolFirst = 8, kPoolLast = 12, kPoolUsed = 16, kPoolNodes = 28;
constexpr u32 kNodeSize = 272, kNodeNext = 4, kNodeData = 16, kNodeSerial = 256;
constexpr u32 kEmDef = 4;
constexpr u32 kKeepBank = 0x00F65B60;             // u32: emitters whose def +4 equals it are kept (name?)

constexpr u32 kGlobalF65BE8 = 0x00F65BE8;         // u32 object, sorted map at +28 (name?)
constexpr u32 kSetName7719A548 = 0x7719A548;
}  // namespace hevent3h
}  // namespace sly4

using namespace sly4::hevent3h;

// u32 SubMachine20748_Construct(m, a, const u32 src[13], d): SubMachine_Construct(m,
// a, 0, 0, d), vtable 0x00B20748, +56 = 0, +60..+111 = src, three
// PtrVectors (+112, +132, +232; +6 = 4), +156 = +160 = 1.0, the rest of
// +164..+261 cleared, +28 |= 0x20. Returns m + 28 (name?)
DECOMP(003F02B8, SubMachine20748_Construct) {
    const u32 m = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), src = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    call(ctx, hhfn::SubMachine_Construct, {m, a, 0, 0, d});
    store<u32>(m + 56, 0);
    store<u32>(m + 0, kSubMachine20748Vtbl);
    for (u32 k = 0; k < 52; k += 4) store<u32>(m + 60 + k, load<u32>(src + k));
    // r4..r7 as the copy leaves them (PtrVector_Construct reads only r3).
    ctx.gpr[4] = m + 60;
    ctx.gpr[5] = load<u32>(src + 48);
    ctx.gpr[6] = load<u32>(src + 36);
    ctx.gpr[7] = load<u32>(src + 40);
    ctx.gpr[8] = load<u32>(src + 44);
    call(ctx, hhfn::PtrVector_Construct, {m + 112});
    store<u16>(m + 118, 4);
    call(ctx, hhfn::PtrVector_Construct, {m + 132});
    store<u16>(m + 138, 4);
    store<u32>(m + 164, 0);
    store<u8>(m + 181, 0);
    store<u32>(m + 156, kF1);
    store<u32>(m + 160, kF1);
    store<u8>(m + 184, 0);
    store<u32>(m + 192, 0);
    store<u32>(m + 196, 0);
    store<u32>(m + 200, 0);
    call(ctx, hhfn::PtrVector_Construct, {m + 232});
    store<u8>(m + 260, 0);
    store<u16>(m + 238, 4);
    store<u8>(m + 261, 0);
    const u32 f = load<u32>(m + 28) | 0x20u;
    store<u32>(m + 28, f);
    ctx.gpr[4] = f;
    ret(ctx, m + 28);
}

// u32 Emitters_StopForeign(): sub_00445C94(0x010ACC40), then Emitter_Stop(h, 0)
// for every emitter in the pool whose def's +4 differs from 0x00F65B60 (the
// next handle is taken first). Returns 0 (name?)
DECOMP(00449478, Emitters_StopForeign) {
    call(ctx, hhfn::sub_00445C94, {kAudioUpdateObj});
    Stack frame(ctx, 16);  // r1+112 current handle, r1+116 first handle
    u32 h = 0;
    if (load<u32>(kEmitters + kPoolUsed) != 0) {
        const u32 first = load<u32>(kEmitters + kPoolFirst);
        const u32 node = load<u32>(kEmitters + kPoolNodes) + first * kNodeSize;
        h = ((first & 0xFFFF) << 16) | ((load<u32>(node + kNodeSerial) >> 15) & 0xFFFF);
    }
    store<u32>(frame.addr() + 4, h);
    store<u32>(frame.addr(), h);
    while ((h & 0xFFFF) != 0) {
        const u32 index = h >> 16;
        const u32 nodes = load<u32>(kEmitters + kPoolNodes);
        const u32 node = nodes + index * kNodeSize;
        u32 next = 0;
        if (static_cast<s32>(index) != load<s32>(kEmitters + kPoolLast)) {
            const u32 n = load<u32>(node + kNodeNext);
            next = ((n << 16) & 0xFFFF0000u) | ((load<u32>(nodes + n * kNodeSize + kNodeSerial) >> 15) & 0xFFFF);
        }
        if (load<u32>(load<u32>(node + kNodeData + kEmDef) + 4) != load<u32>(kKeepBank))
            call(ctx, hfn::Emitter_Stop, {h, 0});
        h = next;
        store<u32>(frame.addr(), h);
    }
    ret(ctx, 0u);
}

// u32 Group_LinkToOwners(Group* g): for every object e of the group's list
// (+12 entries {?, Object*}, +6 u16 count): the owner is looked up in the map
// at *0x00F65BE8 + 28 by e->+44, or (without +44 and +8) by e->+60 -> +260 ->
// +204 when that has type 12 (+4 bits 10..15). A found owner r gets e pushed on
// its list: e->+8 = r, e->+72 = r->+32, e->+76 = depth (+76 of the old head
// + 1, or 0), r->+32 = e, r->+36++. Returns the count (name?)
DECOMP(00451B70, Group_LinkToOwners) {
    const u32 g = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);  // keys at r1+112 / r1+116
    for (u32 i = 0; i < load<u16>(g + 6); ++i) {
        const u32 e = load<u32>(load<u32>(g + 12) + i * 8 + 4);
        if (e == 0) continue;
        u32 r;
        const u32 key = load<u32>(e + 44);
        if (key != 0) {
            store<u32>(frame.addr() + 4, key);
            r = load<u32>(static_cast<u32>(call(ctx, hhfn::SortedMap_Find73B608,
                {load<u32>(kGlobalF65BE8) + 28, frame.addr() + 4})));
        } else {
            if (load<u32>(e + 8) != 0) continue;
            const u32 m = load<u32>(e + 60);
            if (m == 0) continue;
            const u32 m2 = load<u32>(m + 260);
            if (m2 == 0) continue;
            if (((load<u32>(m2 + 4) >> 10) & 0x3F) != 12) continue;
            store<u32>(frame.addr(), load<u32>(m2 + 204));
            r = load<u32>(static_cast<u32>(call(ctx, hhfn::SortedMap_Find73B608,
                {load<u32>(kGlobalF65BE8) + 28, frame.addr()})));
        }
        if (r == 0) continue;
        store<u32>(e + 8, r);
        const u32 head = load<u32>(r + 32);
        store<u32>(e + 72, head);
        store<u32>(e + 76, head != 0 ? load<u32>(head + 76) + 1 : 0);
        const u32 n = load<u32>(r + 36);
        store<u32>(r + 32, e);
        store<u32>(r + 36, n + 1);
    }
    ret(ctx, load<u16>(g + 6));
}

// u32 Session_SetPlayer(Session* s, Player* p, u8 keep_level): 0 for no p.
// Otherwise sub_002DDAB0(s), sub_004BD1CC(Set 0x7719A548) when it exists, s +40 =
// +72 = +48 = 0, UserBlock_Reset45C7A8 on each of the World's +29 user blocks,
// s +36 = p, sub_002DB8B0(p), bit 0x80 of the save slot's u16 at 496 + 4 * p+822
// cleared, and World_StartLevel(world, p +744, 0) when that is set and
// keep_level is 0. Returns 1 (name?)
DECOMP(002E12D0, Session_SetPlayer) {
    const u32 s = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 keep = arg<u32>(ctx, 2) & 0xFF;
    if (p == 0) return ret(ctx, 0u);
    call(ctx, hhfn::sub_002DDAB0, {s});
    const u32 set = static_cast<u32>(call(ctx, hhfn::Set_Find, {kSetName7719A548}));
    if (set != 0) call(ctx, hhfn::sub_004BD1CC, {set});
    store<u32>(s + 40, 0);
    store<u32>(s + 72, 0);
    store<u32>(s + 48, 0);
    for (u32 u = 0; u < load<u8>(load<u32>(kCurrentWorld) + 29); ++u)
        call(ctx, hhfn::UserBlock_Reset45C7A8, {load<u32>(kCurrentWorld) + u * kUserBlockSize + kUserBlocks});
    store<u32>(s + 36, p);
    call(ctx, hhfn::sub_002DB8B0, {p});
    const u32 w = load<u32>(kCurrentWorld);
    const u32 h = w + load<u8>(w + kUserBlocks) * kSaveSlotSize + load<u8>(p + 822) * 4 + 496;
    const u32 level = load<u32>(p + 744);
    store<u16>(h, static_cast<u16>(load<u16>(h) & ~0x80u));
    if (level != 0 && keep == 0) call(ctx, hhfn::World_StartLevel, {load<u32>(kCurrentWorld), level, 0});
    ret(ctx, 1u);
}
