// World_HandleEvent callees, part 19b: animator set switching, speaker line
// nodes, track stop callbacks and the animator's bone-triple records.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
// Animator: +8 Owner*, +32 u32 current set handle, +360 PtrVector of set
// handles (+360 data, +364 cap, +368 count, +370 high-water mark).
constexpr u32 kAnimOwner = 8, kAnimCurrent = 32, kAnimHandles = 360;
// The anim pool's +4 instance pool and +76 def map (by set key); a def's +0
// next def in the chain, +24 its key.
constexpr u32 kPoolInstances = 4, kPoolDefs = 76, kDefNext = 0, kDefKey = 24;

// Speaker line node (108 bytes): +4 Speaker*, +8 u32 a, +44 / +48 u32,
// +104 u8; vt +12 set(u32). Speaker: +8 Owner* (+28 u32), +92 PtrVector*
// of nodes (20 bytes, grow step 4).
constexpr u32 kNodeSize = 108, kNodeSpeaker = 4, kNodeA = 8, kNode44 = 44, kNode48 = 48, kNodeFlag = 104;
constexpr u32 kNodeVtSet = 12;
constexpr u32 kSpeakerOwner = 8, kOwnerArg = 28, kSpeakerNodes = 92, kNodesVecSize = 20, kNodesStep = 4;
constexpr u32 kLineSetMap = 0x0106BBCC;  // map searched for the node's set value

// Track: +4 u32 object handle (track pool), +40 s32 (low half: record
// index). Object +0 points 152 bytes into its base; base +144 u16 record
// count, 132-byte records from +152 {+0 s32 kind (9: stoppable), +4 u16
// type ...}. Type table at 0x00F65100, 44 bytes each: +24 OPD (optional),
// +28 OPD, +36 OPD (optional), each called (record +4, base).
constexpr u32 kTrackObject = 4, kTrackIndex = 40;
constexpr u32 kBaseBias = 152, kBaseCount = 144, kRecSize = 132, kRecKindStop = 9;
constexpr u32 kTypeTable = 0x00F65100, kTypeSize = 44, kTypeFn24 = 24, kTypeFn28 = 28, kTypeFn36 = 36;

// Animator +8 Rig* (+28 skeleton); +100 PtrVector* of 32-byte records
// {s16 bone[4], pad, +16 f32 vec[4] (x, y, z, 0)} (20 bytes, grow step 32).
constexpr u32 kRig = 8, kRigSkeleton = 28, kTriples = 100, kTriplesVecSize = 20, kTriplesStep = 32, kTripleSize = 32;
}  // namespace

// void Anim_SetSet297EDC(Animator* a, u32 key) (name?): makes the set with
// key current. If the animator already holds a handle for it, that handle
// becomes current. Otherwise the def chain from the key's def is
// instantiated (one handle per def not yet held, each appended to +360;
// the key's own becomes current), stopping at the first def already held.
DECOMP(00297EDC, Anim_SetSet297EDC) {
    const u32 a = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 cur = static_cast<u32>(call(ctx, h19fn::Anim_CurrentSet297E3C, {a}));
    if (key == cur) return;  // r3 = cur
    // r1 +112 .. +143 of the original frame.
    Stack frame(ctx, 32);
    const u32 f = frame.addr();
    const u32 h112 = f + 0, h116 = f + 4, out = f + 8, owner = f + 12, h128 = f + 16, keyw = f + 20, desc = f + 24;
    call(ctx, h19fn::Anim_Handle254438, {h112, a, key});
    if ((load<u32>(h112) & 0xFFFF) != 0) {
        call(ctx, h19fn::Anim_Handle254438, {h128, a, key});
        store<u32>(a + kAnimCurrent, load<u32>(h128));
        ret(ctx, a + kAnimCurrent);
        return;
    }
    u32 k = key;
    for (;;) {
        call(ctx, h19fn::Anim_Handle254438, {h116, a, k});
        if ((load<u32>(h116) & 0xFFFF) != 0) {
            ret(ctx, load<u32>(h116) & 0xFFFF);
            return;
        }
        store<u32>(out, 0);
        store<u16>(out + 2, 0);
        store<u32>(owner, load<u32>(a + kAnimOwner));
        store<u32>(desc + 0, 0);
        store<u32>(desc + 4, 0);
        store<u32>(keyw, k);
        const u32 slot = static_cast<u32>(call(ctx, h19fn::SortedMap_Find75E3E8, {load<u32>(kAnimPool) + kPoolDefs, keyw}));
        const u32 def = load<u32>(slot);
        if (def == 0) return;  // r3 = slot
        store<u32>(desc + 4, owner);
        store<u32>(desc + 0, def);
        call(ctx, h19fn::Pool_Create4B25C8, {load<u32>(kAnimPool) + kPoolInstances, desc, out, owner});
        hevent18::pv_push(ctx, a + kAnimHandles, load<u32>(out));
        const u32 next = load<u32>(def + kDefNext);
        if (key == k) store<u32>(a + kAnimCurrent, load<u32>(out));
        if (next == 0) {
            // r3: the high-water mark just stored (the unmasked count when that won).
            const u32 n = load<u16>(a + kAnimHandles + hevent18::kPvCount), hi = load<u16>(a + kAnimHandles + hevent18::kPvHigh);
            ret(ctx, (n == 0 && hi == 0) ? 0x10000u : hi);
            return;
        }
        k = load<u32>(next + kDefKey);
    }
}

// Node* Speaker_Add351898(Speaker* s, u32 a, u32 key, u8 flag) (name?):
// allocates and constructs a 108-byte line node, initialises it
// (0x00350178 with the owner's +28), sets it from the line-set map's value
// for key (vt +12), copies +48 to +44 when set, and appends it to the
// speaker's node vector (created on first use). A failed node allocation
// still writes +4 / +8 / +104 of address 0; a failed vector allocation
// pushes through a null vector (both as the original).
DECOMP(00351898, Speaker_Add351898) {
    const u32 s = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const u32 flag = arg<u8>(ctx, 3);
    u32 node = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kNodeSize}));
    if (node != 0) call(ctx, h19fn::Node_Construct3516C4, {node});
    store<u32>(node + kNodeA, a);
    store<u32>(node + kNodeSpeaker, s);
    const u32 owner = load<u32>(s + kSpeakerOwner);
    call(ctx, h19fn::Node_Init350178, {node, owner != 0 ? load<u32>(owner + kOwnerArg) : 0u, 0});
    store<u8>(node + kNodeFlag, static_cast<u8>(flag));
    {
        Stack frame(ctx, 16);  // the key word (r1 +112)
        store<u32>(frame.addr(), key);
        const u32 slot = static_cast<u32>(call(ctx, h19fn::SortedMap_Find76E0A8, {kLineSetMap, frame.addr()}));
        hevent::vcall(ctx, node, kNodeVtSet, {node, load<u32>(slot)});
    }
    const u32 v48 = load<u32>(node + kNode48);
    if (v48 != 0) store<u32>(node + kNode44, v48);
    u32 v = load<u32>(s + kSpeakerNodes);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kNodesVecSize}));
        store<u32>(s + kSpeakerNodes, v);
        if (v != 0) {
            call(ctx, hevent9::h9fn::PtrVector_Construct, {v});
            store<u16>(v + hevent18::kPvStep, kNodesStep);
            v = load<u32>(s + kSpeakerNodes);
        }
    }
    hevent18::pv_push(ctx, v, node);
    ret(ctx, node);
}

// void Track_Stop254210(Animator* unused, Track* t, TrackDef* d) (name?):
// for a live track object whose record (d +40) is of the stoppable kind,
// unless the record's slot (0x0038B118) is flagged (+2 bit 15), runs the
// record type's stop callbacks (+24 if set, +28, +36 if set). r3 is left
// as the original leaves it on each path.
DECOMP(00254210, Track_Stop254210) {
    const u32 t = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 pool = load<u32>(kTrackPool);
    const u32 h = load<u32>(t + kTrackObject);
    const u32 serial = h & 0xFFFF;
    if (serial == 0) return ret(ctx, 0u);
    if (!(static_cast<s32>(h >> 16) < static_cast<s32>(load<u32>(pool + kTrackPoolCount)))) return ret(ctx, serial);
    const u32 ent = load<u32>(pool + kTrackPoolEntries) + (h >> 16) * kPoolEntrySize;
    if (((load<u32>(ent + kEntryWord) >> 15) & 0xFFFF) != serial) return ret(ctx, serial);
    if ((load<u8>(ent + kEntryWord) >> 7) != 0) return ret(ctx, 1u);
    const u32 obj = load<u32>(ent + kEntryObject);
    if (obj == 0) return ret(ctx, ent + kEntryObject);
    const u32 base = load<u32>(obj) - kBaseBias;
    const s32 i = static_cast<s16>(static_cast<u16>(load<u32>(d + kTrackIndex)));
    if (!(i < static_cast<s32>(load<u16>(base + kBaseCount)))) return ret(ctx, base);
    const u32 rec = base + kBaseBias + static_cast<u32>(i) * kRecSize;
    const s32 kind = static_cast<s32>(load<u32>(rec));  // read before the null check, as the original
    if (rec == 0 || kind >= 10 || kind != static_cast<s32>(kRecKindStop)) return ret(ctx, base);
    const u32 slot = rec + 4;
    const u32 q = static_cast<u32>(call(ctx, h19fn::Track_Slot38B118, {slot, base}));
    if ((load<u16>(q + 2) & 0x8000) != 0) return ret(ctx, 1u);
    auto type = [&] { return kTypeTable + static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(slot)))) * kTypeSize; };
    // Each callback gets (slot, base); the original leaves the base (64-bit
    // form) or the entry address in r5 and the OPD or entry in r6.
    const u64 base64 = static_cast<u64>(load<u32>(obj)) - kBaseBias;
    const u32 fn24 = load<u32>(type() + kTypeFn24);
    if (fn24 != 0) {
        ctx.gpr[5] = base64;
        ctx.gpr[6] = load<u32>(fn24);
        call_ptr(ctx, fn24, {slot, static_cast<u32>(load<u32>(obj) - kBaseBias)});
    }
    {
        const u32 b = load<u32>(obj) - kBaseBias;
        const u32 fn28 = load<u32>(type() + kTypeFn28);
        ctx.gpr[5] = load<u32>(fn28);
        ctx.gpr[6] = fn28;
        call_ptr(ctx, fn28, {slot, b});
    }
    const u32 fn36 = load<u32>(type() + kTypeFn36);
    if (fn36 == 0) return ret(ctx, type());
    const u32 b = load<u32>(obj) - kBaseBias;
    ctx.gpr[5] = load<u32>(fn36);
    call_ptr(ctx, fn36, {slot, b});
}

// void Anim_B351F30(Animator* a, -, -, u32 b0, u32 b1, u32 b2, u32 b3,
// const f32 v[3]) (name?): looks the four bone hashes up in the rig's
// skeleton (0x00A59468) and, when all are found, appends {b0..b3, (x, y,
// z, 0)} to the record vector at +100 (created on first use).
DECOMP(00351F30, Anim_B351F30) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 hb[4] = {arg<u32>(ctx, 3), arg<u32>(ctx, 4), arg<u32>(ctx, 5), arg<u32>(ctx, 6)};
    const u32 vp = arg<u32>(ctx, 7);
    const u32 rig = load<u32>(a + kRig);
    if (rig == 0) return ret(ctx, vp);
    const u32 skel = load<u32>(rig + kRigSkeleton);
    if (skel == 0) return ret(ctx, vp);
    const u32 x = load<u32>(vp), y = load<u32>(vp + 4), z = load<u32>(vp + 8);
    u64 bone[4];
    for (u32 k = 0; k < 4; ++k) bone[k] = call(ctx, h19fn::Rig_FindBoneA59468, {skel, hb[k], 0});
    for (u32 k = 0; k < 4; ++k) {
        if (static_cast<s16>(static_cast<u16>(bone[k])) == -1) {
            if (k == 0) ret(ctx, bone[3]);  // r3 still the last lookup's result
            else ret(ctx, -1);
            return;
        }
    }
    u32 v = load<u32>(a + kTriples);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kTriplesVecSize}));
        store<u32>(a + kTriples, v);
        if (v != 0) {
            call(ctx, hevent9::h9fn::PtrVector_Construct, {v});
            store<u16>(v + hevent18::kPvStep, kTriplesStep);
            v = load<u32>(a + kTriples);
        }
    }
    u32 n = load<u16>(v + hevent18::kPvCount);
    const u32 cap = load<u16>(v + hevent18::kPvCap);
    if (n == cap) {
        call(ctx, hevent8::h8fn::PtrVector_Reserve, {v, (cap << 1) & 0xFFFFFFFEu});
        n = load<u16>(v + hevent18::kPvCount);
    }
    const u32 rec = load<u32>(v + hevent18::kPvData) + n * kTripleSize;
    for (u32 k = 0; k < 4; ++k) store<u16>(rec + 2 * k, static_cast<u16>(bone[k]));
    const u32 n1 = load<u16>(v + hevent18::kPvCount) + 1;
    const u32 hi = load<u16>(v + hevent18::kPvHigh);
    const u32 vec = (rec + 16) & ~15u;  // stvx
    store<u32>(vec + 0, x);
    store<u32>(vec + 4, y);
    store<u32>(vec + 8, z);
    store<u32>(vec + 12, 0);
    store<u16>(v + hevent18::kPvCount, static_cast<u16>(n1));
    const u32 high = static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF) ? hi : n1;
    store<u16>(v + hevent18::kPvHigh, static_cast<u16>(high));
    ret(ctx, hi);
}
