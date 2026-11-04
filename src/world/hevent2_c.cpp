// World_HandleEvent callees, part 2: 56..59 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent2 {
namespace cfn {
constexpr u32 sub_000FAE00 = 0x000FAE00;   // (Object*, 0)
constexpr u32 sub_000CF214 = 0x000CF214;   // (f32 dt)
constexpr u32 sub_0020F400 = 0x0020F400;   // ()
constexpr u32 sub_000CF0C0 = 0x000CF0C0;   // (Object*, f32 dt)
constexpr u32 sub_00441AC0 = 0x00441AC0;   // (a, u32* b) -> u32 (name?)
constexpr u32 SmallAlloc = 0x0055238C;
constexpr u32 sub_0039BC48 = 0x0039BC48;   // (Elem112*)
constexpr u32 sub_003991A0 = 0x003991A0;   // (Elem112*)
constexpr u32 sub_0039F3D8 = 0x0039F3D8;   // (Elem368*)
}  // namespace cfn

// Job B's results (see Objects_RunJobB).
constexpr u32 kJobBPending = 0x01021E80, kJobBCount = 0x01019BD0, kJobBObjects = 0x01019BE0;

// The registry next to the map at 0x010686A8 (Registry_Find): u32 count at +24 (name?)
constexpr u32 kRegistry = 0x010686A8, kRegistryCount = 0x010686C0;
constexpr u32 kRegistrySkipA = 0x0353CB09, kRegistrySkipB = 0x63C971C0;  // type ids left alone (name?)

// Node_PushF65B7C: a doubly linked list of 24-byte nodes, newest first (name?)
//   +0 u32 (sub_00441AC0's result) +4 next +8 prev +12 b +16 a +20 u32 refs = 1
constexpr u32 kNodeHead = 0x00F65B7C;

// World_GetSaveObj160 (name?)
constexpr u32 kWorldObj = 0x128164;          // u32 object: +5540 override, +5572 bit 0x08000000
constexpr u32 kSessionPtr = 0x00F64C20;      // u32 Session*: +36 player (+812, u8 +822 index)
constexpr u32 kFallback10683D4 = 0x010683D4; // u32 (0x0106838C + 72) (name?)
constexpr u32 kSlot160 = 160;                // SaveSlot + : u32
constexpr u32 kSlotFlags496 = 496 - kSaveData;  // SaveSlot + : u16[] by player index, bit 0x20 (name?)
}  // namespace hevent2
}  // namespace sly4

using namespace sly4::hevent2;

// void Obj_RestoreFrom544(Object* o): with +344 bit 31 set, vfunc +84(o, o + 480),
// then copies the saved fields +544..+579 back to +356.. and calls sub_000FAE00(o, 0) (name?)
DECOMP(000FCC3C, Obj_RestoreFrom544) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 344) >> 31) == 0) return ret(ctx, 0u);
    vcall(ctx, o, 84, {o, o + 480});
    store<u32>(o + 356, load<u32>(o + 544));
    store<u32>(o + 360, load<u32>(o + 548));
    ref<u8>(o + 440) = ref<u8>(o + 552);
    store<u32>(o + 444, load<u32>(o + 556));
    store<u32>(o + 448, load<u32>(o + 560));
    store<u32>(o + 452, load<u32>(o + 564));
    ref<u8>(o + 456) = ref<u8>(o + 568);
    store<u32>(o + 460, load<u32>(o + 572));
    for (u32 k = 0; k < 4; ++k) ref<u8>(o + 464 + k) = ref<u8>(o + 576 + k);
    ctx.gpr[5] = ref<u8>(o + 579);
    ctx.gpr[6] = o + 464;
    ctx.gpr[7] = ref<u8>(o + 578);
    call(ctx, cfn::sub_000FAE00, {o, 0});
}

// u32 Users_ForEachVfn32_36(Object* o): when o->+40 is set, every object of
// user block 0's list (+80 data, +88 u16 count) gets vfunc +32() and +36(0) (name?)
DECOMP(002DFEFC, Users_ForEachVfn32_36) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 40) == 0) return ret(ctx, 0u);
    const u32 blk = load<u32>(kCurrentWorld) + kUserBlocks;
    u32 i = 0;
    for (; i < load<u16>(blk + 88); ++i) {
        const u32 e = load<u32>(load<u32>(blk + 80) + i * 4);
        if (e == 0) continue;
        vcall(ctx, e, 32, {e});
        vcall(ctx, e, 36, {e, 0});
    }
    ret(ctx, load<u16>(blk + 88));
}

// u32 Registry_ForEachVfn16(): vfunc +16() on the value of every entry of the
// registry at 0x010686A8 whose type (+12) is neither 0x0353CB09 nor 0x63C971C0 (name?)
DECOMP(002E9A28, Registry_ForEachVfn16) {
    u32 n = load<u32>(kRegistryCount);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(kRegistry + 8) + i * 8 + 4);
        const u32 t = load<u32>(e + 12);
        if (t != kRegistrySkipA && t != kRegistrySkipB) {
            vcall(ctx, e, 16, {e});
            n = load<u32>(kRegistryCount);
        }
    }
    ret(ctx, n);
}

// u32 Objects_FinishJobB(f32 dt): waits (300 us sleeps) for job B's SPU work,
// runs sub_000CF214(dt) and sub_0020F400(), then sub_000CF0C0(obj, dt) for each
// object it listed, and clears the list (name?)
DECOMP(000CF398, Objects_FinishJobB) {
    const f64 dt = farg(ctx, 0);
    while (load<u32>(kJobBPending) != 0) syscall(ctx, kSysTimerUsleep, {300});
    callf(ctx, cfn::sub_000CF214, {}, {dt});
    call(ctx, cfn::sub_0020F400);
    u32 n = load<u32>(kJobBCount);
    if (n != 0) {
        for (u32 i = 0; i < load<u32>(kJobBCount); ++i)
            callf(ctx, cfn::sub_000CF0C0, {load<u32>(kJobBObjects + i * 4)}, {dt});
        n = load<u32>(kJobBCount);
    }
    store<u32>(kJobBCount, 0);
    ret(ctx, n);
}

// List* World_RunTimedCallbacks(World* w, ?, u32 kind, ?, u8 clear, f32 dt): calls
// every entry {opd, arg, -} of the World's list `kind` (see World_AddTimedCallback)
// as opd(<code address>, arg, dt), then empties the list when `clear` (name?)
DECOMP(0045900C, World_RunTimedCallbacks) {
    const u32 w = arg<u32>(ctx, 0), kind = arg<u32>(ctx, 2);
    const u32 clear = arg<u32>(ctx, 4);
    const f64 dt = farg(ctx, 0);
    const u32 v = w + kind * 20 + 0x128480;
    for (u32 i = 0; i < load<u16>(v + 8); ++i) {
        const u32 e = load<u32>(v) + i * 12;
        const u32 opd = load<u32>(e), a = load<u32>(e + 4);
        // r3 is left holding the code address loaded from the OPD, r5 the OPD
        // and f1 dt, as in the original.
        ctx.gpr[5] = opd;
        ctx.fpr[1] = dt;
        call_ptr(ctx, opd, {load<u32>(opd), a});
    }
    ctx.gpr[4] = clear & 0xFF;
    if ((clear & 0xFF) != 0) store<u16>(v + 8, 0);
    ret(ctx, v);
}

// Node* Node_PushF65B7C(u32 a, u32 b): 0 for a == 0; otherwise a new node {h =
// sub_00441AC0(a, &b), b, a} pushed at the front of the list at 0x00F65B7C (name?)
// Quirk: a failed SmallAlloc still writes +0, +12 and +16 of address 0.
DECOMP(00446984, Node_PushF65B7C) {
    const u32 a = arg<u32>(ctx, 0), b0 = arg<u32>(ctx, 1);
    if (a == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);  // b at r1+112
    store<u32>(frame.addr(), b0);
    const u32 h = static_cast<u32>(call(ctx, cfn::sub_00441AC0, {a, frame.addr()}));
    const u32 b = load<u32>(frame.addr());
    const u32 node = static_cast<u32>(call(ctx, cfn::SmallAlloc, {24}));
    if (node != 0) {
        store<u32>(node + 0, 0);
        store<u32>(node + 8, 0);
        store<u32>(node + 12, 0);
        store<u32>(node + 16, 0);
        store<u32>(node + 20, 1);
        const u32 head = load<u32>(kNodeHead);
        if (head != 0) store<u32>(head + 8, node);
        store<u32>(node + 4, head);
        store<u32>(kNodeHead, node);
        ctx.gpr[4] = head;
        ctx.gpr[5] = 0x00F60000u;
    }
    store<u32>(node + 12, b);
    store<u32>(node + 16, a);
    store<u32>(node + 0, h);
    ret(ctx, node);
}

// u32 World_GetSaveObj160(): the World object's +5540 when it has one (and
// +5572 bit 0x08000000 is clear); else the session player's +812 when its
// flags bit 0x20 is set in the active save slot (0 when clear); else the slot's
// +160, or *0x010683D4 when that is 0 (name?)
DECOMP(002E29B0, World_GetSaveObj160) {
    const u32 w = load<u32>(kCurrentWorld);
    const u32 user_off = ref<u8>(w + kUserBlocks) * kSaveSlotSize;
    const u32 slot = w + user_off + kSaveData;
    const u32 o = load<u32>(w + kWorldObj);
    auto slot_value = [&]() {
        const u32 r = load<u32>(slot + kSlot160);
        return r != 0 ? r : load<u32>(kFallback10683D4);
    };
    if (o == 0 || (load<u32>(o + 5572) & 0x08000000u) != 0) return ret(ctx, slot_value());
    const u32 p = load<u32>(o + 5540);
    if (p != 0) return ret(ctx, p);
    const u32 q = load<u32>(load<u32>(kSessionPtr) + 36);
    if (q != 0) {
        const u32 bits = load<u16>(slot + kSlotFlags496 + ref<u8>(q + 822) * 4);
        if ((bits & 0x20) != 0) return ret(ctx, load<u32>(q + 812));
        return ret(ctx, 0u);
    }
    ret(ctx, slot_value());
}

// u32 Obj_Refresh3A0380(Object* o): sub_0039BC48 and sub_003991A0 on each
// 112-byte element of +12 (count u16 at +4 -> +2), then sub_0039F3D8 on each
// 368-byte element of +44 (count at +36 -> +2) (name?)
DECOMP(003A0380, Obj_Refresh3A0380) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u16>(load<u32>(o + 4) + 2); ++i) {
        const u32 e = load<u32>(o + 12) + i * 112;
        call(ctx, cfn::sub_0039BC48, {e});
        call(ctx, cfn::sub_003991A0, {e});
    }
    u32 b = load<u32>(o + 36);
    for (u32 j = 0; j < load<u16>(b + 2); ++j) {
        const u32 e = load<u32>(o + 44) + j * 368;
        if (e == 0) continue;
        call(ctx, cfn::sub_0039F3D8, {e});
        b = load<u32>(o + 36);
    }
    ret(ctx, b);
}

namespace sly4 {
namespace hevent2 {
constexpr u32 sub_002E0930 = 0x002E0930;   // (a, Elem536*, b, u8 c, u8 d, u8 e, u8 f, u8 g, 1) (name?)
constexpr u32 SortedMap_Find786C0C = 0x00786C0C;  // (map, const u32* key) -> slot (name?)

// Msg_Post: subscriber map key -> node list {+0 Object*, +4 next, +8 u8 flag};
// queued records {+0 msg, +4 Object*, +8, +12, +16 u8 flag}, 20 bytes, at most 128.
constexpr u32 kMsgSubscribers = 0x010BB738;
constexpr u32 kMsgQueueCount = 0x010BA134;  // s32
constexpr u32 kMsgQueue = 0x010BA138;
constexpr s32 kMsgQueueMax = 128;
constexpr u32 kMsgRecord = 20;
}  // namespace hevent2
}  // namespace sly4

// u32 Obj_Call2E0930ById(Object* o, s32 id, b, u8 c, u8 d, u8 e, u8 f, u8 g): finds
// the 536-byte element of o (+28 data, +32 count) whose +468 is id (none for 0)
// and returns sub_002E0930(o, elem or 0, b, c, d, e, f, g, 1) (name?)
DECOMP(002E1DB0, Obj_Call2E0930ById) {
    const u32 o = arg<u32>(ctx, 0), id = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3) & 0xFF, d = arg<u32>(ctx, 4) & 0xFF, e = arg<u32>(ctx, 5) & 0xFF;
    const u32 f = arg<u32>(ctx, 6) & 0xFF, g = arg<u32>(ctx, 7) & 0xFF;
    u32 elem = 0;
    if (id != 0) {
        const u32 n = load<u32>(o + 32);
        for (u32 i = 0; i < n; ++i) {
            const u32 p = load<u32>(o + 28) + i * 536;
            if (load<u32>(p + 468) == id) {
                elem = p;
                break;
            }
        }
    }
    call(ctx, sub_002E0930, {o, elem, b, c, d, e, f, g, 1});
}

// u32 Msg_Post(u32 msg, u32 key, u32 a, u32 b): queues {msg, obj, a, b, flag}
// for every subscriber of `key` (skipping null objects that have the flag),
// while the queue at 0x010BA138 has room; returns 0 (name?)
DECOMP(00470660, Msg_Post) {
    const u32 msg = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    Stack frame(ctx, 16);  // key at r1+112
    store<u32>(frame.addr(), key);
    const u32 slot = static_cast<u32>(call(ctx, SortedMap_Find786C0C, {kMsgSubscribers, frame.addr()}));
    const u32 list = load<u32>(slot);
    if (list == 0) return ret(ctx, 0u);
    u32 node = load<u32>(list);
    if (node == 0) return ret(ctx, 0u);
    s32 count = static_cast<s32>(load<u32>(kMsgQueueCount));
    for (; node != 0; node = load<u32>(node + 4)) {
        const u32 obj = load<u32>(node);
        const u8 flag = ref<u8>(node + 8);
        if (obj == 0 && flag != 0) continue;
        if (count >= kMsgQueueMax) continue;
        const u32 r = kMsgQueue + static_cast<u32>(count) * kMsgRecord;
        ++count;
        store<u32>(r, msg);
        store<u32>(kMsgQueueCount, static_cast<u32>(count));
        store<u32>(r + 4, obj);
        store<u32>(r + 8, a);
        store<u32>(r + 12, b);
        ref<u8>(r + 16) = flag;
    }
    ret(ctx, 0u);
}
