// World_HandleEvent callees, part 18c: speaker line nodes, per-level
// ability lists and the main object's treasure re-attach.
#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
// Speaker line node (44 bytes): +0 vtable, +4 Speaker*, +8 u32 line, +12
// u32 flags (bits 26/27 and 0..21 kept), +16 / +20 f32 1.0, +24..+40 0.
constexpr u32 kNodeSize = 44, kNodeVtable = 0x00B1F558;
constexpr u32 kNodeSpeaker = 4, kNodeLine = 8, kNodeFlags = 12, kNodeFlagsKeep = 0x063FFFFFu;
constexpr u32 kOneF = 0x3F800000u;
// Speaker: +8 Owner* (+28 u32), +92 PtrVector* of nodes (20 bytes, grow step 4).
constexpr u32 kSpeakerOwner = 8, kOwnerArg = 28, kSpeakerNodes = 92, kNodesVecSize = 20, kNodesStep = 4;
constexpr u32 kNodeInit350178 = 0x00350178;  // (Node*, u32 owner arg, 0) (name?)

// Level table: u32 count at 0x010C1A9C, 152-byte records from 0x010BE8BC
// {+0 u32, +4 u32 ability id, +8 u32 level hash, +148 u8 hidden}. The save
// data (World +80) holds a flag byte per record at +3948 (bit 0 and bit 1
// both needed).
constexpr u32 kLevelCount = 0x010C1A9C, kLevelRecs = 0x010BE8BC, kLevelRecSize = 152;
constexpr u32 kLvA = 0, kLvId = 4, kLvHash = 8, kLvHidden = 148;
constexpr u32 kSaveLevelFlags = 3948;

// Object: +128 u32, +132 Object* holder, +136 Object* treasure; +172 bit
// 18 (0x40000): attachable.
constexpr u32 kObjArg128 = 128, kObjHolder = 132, kObjTreasure = 136, kObjFlags172 = 172, kObjAttachable = 0x40000;
// The 240-byte attach descriptor (as Obj_Attach0D15C0), built at r1 +112 of
// the original's 368-byte frame.
constexpr u32 kFrame = 368, kDescAt = 112;
constexpr u32 kDescPos = 128, kDescRot = 144, kDescArg = 160, kDescFlags = 176, kDescVec = 180, kDescObject = 192;
constexpr u32 kDescFlagsKeep = 0x00FFFFFFu, kDescFlagsSet = 0x18000000u;
constexpr u32 kIdentityQuat = 0x01164C80;  // f32[4]
constexpr u32 kTreasurePlace425F18 = 0x00425F18;  // (Object*, Quat* rot, vec3* pos) (name?)
constexpr u32 kObjAttach0D15C0 = 0x000D15C0;      // (Object*, const AttachDesc*) (name?)
constexpr u32 kNop54EC18 = 0x0054EC18;            // (PtrVector*): empty
}  // namespace

// Node* Speaker_First35133C(Speaker* s, u32 line) (name?): allocates a
// 44-byte line node, initialises it (0x00350178 with the owner's +28) and
// appends it to the speaker's node vector (created on first use). A failed
// node allocation still writes +4 / +8 of address 0; a failed vector
// allocation pushes through a null vector (both as the original).
DECOMP(0035133C, Speaker_First35133C) {
    const u32 s = arg<u32>(ctx, 0), line = arg<u32>(ctx, 1);
    const u32 node = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kNodeSize}));
    if (node != 0) {
        const u32 flags = load<u32>(node + kNodeFlags) & kNodeFlagsKeep;
        store<u32>(node + 0, kNodeVtable);
        store<u32>(node + 16, kOneF);
        store<u32>(node + 20, kOneF);
        store<u32>(node + 24, 0);
        store<u32>(node + 28, 0);
        store<u32>(node + 32, 0);
        store<u32>(node + kNodeFlags, flags);
        store<u32>(node + 36, 0);
        store<u32>(node + 40, 0);
    }
    store<u32>(node + kNodeSpeaker, s);
    store<u32>(node + kNodeLine, line);
    const u32 owner = load<u32>(s + kSpeakerOwner);
    call(ctx, kNodeInit350178, {node, owner != 0 ? load<u32>(owner + kOwnerArg) : 0u, 0});
    u32 v = load<u32>(s + kSpeakerNodes);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kNodesVecSize}));
        store<u32>(s + kSpeakerNodes, v);
        if (v != 0) {
            call(ctx, hevent9::h9fn::PtrVector_Construct, {v});
            store<u16>(v + kPvStep, kNodesStep);
            v = load<u32>(s + kSpeakerNodes);
        }
    }
    pv_push(ctx, v, node);
    ret(ctx, node);
}

// void Level_Abilities49E43C(u32 level, PtrVector* out) (name?): appends
// the ability id of every level-table record of that level that has an id
// and a first word, is not hidden and whose save flag byte has bits 0 and 1.
DECOMP(0049E43C, Level_Abilities49E43C) {
    const u32 level = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 w = load<u32>(kCurrentWorld);
    u32 n = load<u32>(kLevelCount);
    for (u32 i = 0; i < n; ++i) {
        const u32 rec = kLevelRecs + kLevelRecSize * i;
        if (load<u32>(rec + kLvHash) != level) continue;
        if (load<u32>(rec + kLvId) == 0 || load<u32>(rec + kLvA) == 0) continue;
        if (load<u8>(rec + kLvHidden) != 0) continue;
        const u32 f = load<u8>(w + kSaveData + kSaveLevelFlags + i);
        if ((f & 1) == 0 || (f & 2) == 0) continue;
        pv_push(ctx, out, load<u32>(rec + kLvId));
        n = load<u32>(kLevelCount);
    }
    ctx.gpr[3] = n;  // r3 left as the original leaves it
}

// void Main_Refresh42685C(Object* o) (name?): when both the holder (+132)
// and the treasure (+136) are attachable, places the treasure
// (0x00425F18 into an attach descriptor) and attaches it to the holder;
// then frees the descriptor's vector if the attach left one.
DECOMP(0042685C, Main_Refresh42685C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 holder = load<u32>(o + kObjHolder);
    if (holder == 0 || (load<u32>(holder + kObjFlags172) & kObjAttachable) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 t = load<u32>(o + kObjTreasure);
    if (t == 0 || (load<u32>(t + kObjFlags172) & kObjAttachable) == 0) {
        ret(ctx, t);  // r3 left as the original leaves it
        return;
    }
    // The frame matches the original's, so the flags word read before it is
    // written (bits 8..31 kept) is the same stack word.
    Stack frame(ctx, kFrame);
    const u32 d = frame.addr() + kDescAt;
    const u32 flags = load<u32>(d + kDescFlags) & kDescFlagsKeep;
    for (u32 k = 0; k < 12; k += 4) store<u32>(d + kDescPos + k, load<u32>(hevent7::kZeroVec + k));
    for (u32 k = 0; k < 16; k += 4) store<u32>(d + kDescRot + k, load<u32>(kIdentityQuat + k));
    store<u32>(d + 164, 0);
    store<u32>(d + 168, 0);
    store<u32>(d + 172, 0);
    store<u32>(d + kDescVec, 0);
    store<u32>(d + kDescFlags, flags);
    store<u8>(d + 0, 0);
    store<u8>(d + 64, 0);
    store<u32>(d + 196, 0);
    store<u32>(d + 200, 0xFFFFFFFFu);
    store<u32>(d + 204, 0);
    store<u32>(d + 208, 0);
    store<u32>(d + 212, 0xFFFFFFFFu);
    store<u32>(d + 216, 0);
    store<u32>(d + 220, 0);
    store<u32>(d + 224, 0);
    store<u32>(d + kDescObject, t);
    store<u32>(d + kDescArg, load<u32>(o + kObjArg128));
    ctx.fpr[1] = hevent7::lfs_f64(kIdentityQuat + 12);  // f1 / f2 as the copies leave them
    ctx.fpr[2] = hevent7::lfs_f64(kIdentityQuat + 8);
    call(ctx, kTreasurePlace425F18, {o, d + kDescRot, d + kDescPos});
    store<u32>(d + kDescFlags, load<u32>(d + kDescFlags) | kDescFlagsSet);
    call(ctx, kObjAttach0D15C0, {load<u32>(o + kObjHolder), d});
    const u32 vec = load<u32>(d + kDescVec);
    if (vec == 0) return;
    call(ctx, hfn::PtrVector_Clear, {vec});
    call(ctx, kNop54EC18, {vec});
    call(ctx, hevent7::h7fn::SmallFree, {load<u32>(d + kDescVec), 20});
}
