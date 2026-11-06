// Game code, medium sweep part 3: the action-script event node factories
// ("RStartMeleeAttack", "RPlaySndSFX", ...): a Node with its parameter
// bindings (as obj19::TNewNamed, with per-parameter defaults and flag
// slots).
#include "mid3.h"

using namespace sly4;
using namespace sly4::mid3;

namespace {

namespace nfn {
constexpr u32 Node_Init = 0x00391694;    // (p, s16 id, const char* name)
constexpr u32 Node_Bind = 0x004D3088;    // (owner, const char* name, kind, k6, -1, 0) -> binding
constexpr u32 kNodeTable = 0x00F65038;   // -> +24 the 128-byte records
constexpr u32 kFlagOffset = 0x0106CF82;  // u16 added to the node's +20
constexpr u32 Cond_Init = 0x0038E5DC;    // (p, s16 id, const char* name, 1): a condition node
constexpr u32 kCondFlagOffset = 0x0106CF7A;  // u16: the same for condition nodes
}  // namespace nfn

namespace nfn2 {
constexpr u32 Binding_Reserve = 0x004D2BB8;  // (b, n): +72 = n, +68 = the name array
}  // namespace nfn2

// A parameter: its name, the Node_Bind kind / k6, the default value word
// stored in the binding's +76, its slot in the node's flag bytes, and for
// an enum parameter the size of its name list and up to two default names.
struct Param {
    u32 name, kind, k6, init, slot;
    u32 reserve = 0, str0 = 0, str1 = 0;
};

// A name appended to binding b's list {+0 u32 count, +68 char*[], +72 u32
// capacity} when there is room: a 100-byte Mem_Alloc'd copy (String_CopyN).
void BindingAddName(PPUContext& ctx, u32 b, u32 s) {
    if (!(load<u32>(b) < load<u32>(b + 72))) return;
    const u32 m = static_cast<u32>(call(ctx, m3fn::Mem_Alloc, {100, 0, 16}));
    store<u32>(load<u32>(b + 68) + ((load<u32>(b) << 2) & 0xFFFFFFFCu), m);
    call(ctx, m3fn::String_CopyN, {load<u32>(load<u32>(b + 68) + ((load<u32>(b) << 2) & 0xFFFFFFFCu)), s, 100});
    store<u32>(b, load<u32>(b) + 1);
}

// Node* New(s16 id): Mem_Alloc(24, 0, 16) (0 on failure); Node_Init(p, id,
// name); the u16 rec stored at the node's record (kNodeTable->+24 + index
// * 128, the s16 index at +16; at address 0 for index -1, as the
// original); then per parameter Node_Bind(p->+12, name, kind, k6, -1, 0),
// the binding's +76 = init and its +64 copied as a byte to (p->+20 +
// *kFlagOffset)[slot]; for an enum parameter Binding_Reserve(b,
// reserve) and its default names. Returns p (r4 = the last +64, as left).
// cond: a condition node (Cond_Init(p, id, name, 1), kCondFlagOffset).
void NewNamedNode(PPUContext& ctx, u32 name, u16 rec, std::initializer_list<Param> params, bool cond = false) {
    const u64 id = ctx.gpr[3];
    const u32 p = static_cast<u32>(call(ctx, m3fn::Mem_Alloc, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u64 sid = static_cast<u64>(static_cast<s64>(static_cast<s16>(id)));
    if (cond) call(ctx, nfn::Cond_Init, {p, sid, name, 1});
    else call(ctx, nfn::Node_Init, {p, sid, name});
    const s32 idx = static_cast<s16>(load<u16>(p + 16));
    const u32 tbl = load<u32>(nfn::kNodeTable);
    const u32 r = idx == -1 ? 0 : load<u32>(tbl + 24) + (static_cast<u32>(idx) << 7);
    store<u16>(r, rec);
    const u32 flags = load<u32>(p + 20) + load<u16>(cond ? nfn::kCondFlagOffset : nfn::kFlagOffset);
    for (const Param& q : params) {
        const u32 b = static_cast<u32>(call(ctx, nfn::Node_Bind, {load<u32>(p + 12), q.name, q.kind, q.k6, ~0ull, 0}));
        const u32 v = load<u32>(b + 64);
        store<u32>(b + 76, q.init);
        store<u8>(flags + q.slot, static_cast<u8>(v));
        ctx.gpr[4] = v;
        if (q.reserve == 0) continue;
        call(ctx, nfn2::Binding_Reserve, {b, q.reserve});
        for (const u32 str : {q.str0, q.str1})
            if (str != 0) BindingAddName(ctx, b, str);
    }
    ret(ctx, p);
}

}  // namespace

// Node* New(s16 id): the "RStartMeleeAttack" event node (Mem_Alloc(24, 0, 16), record 16):
// AttachBoneName, Radius, Damage, Impulse, XOffset, YOffset, ZOffset, HitScript, ReactType.
DECOMP(0077A03C, NodeStartMeleeAttack_New_0077A03C) {
    NewNamedNode(ctx, 0x00B2009B, 16, {
        {0x00B200AD, 5, 0, 0, 0},
        {0x00B200BC, 3, 0, 0, 1},
        {0x00B200C3, 3, 0, 0, 2},
        {0x00B200CA, 3, 0, 0, 3},
        {0x00B200D2, 3, 1, 0, 4},
        {0x00B200DA, 3, 1, 0, 5},
        {0x00B200E2, 3, 1, 0, 6},
        {0x00B200EA, 2, 1, 0, 7},
        {0x00B200F4, 2, 1, 0, 8}
    });
}

// Node* New(s16 id): the "RPlaySndSFX" event node (Mem_Alloc(24, 0, 16), record 16):
// SndName, Part, UseDefaultSndLib, Is3DSnd, AttachType, AttachTypePlayer, UseSurfaceData, Layer, ExclusivityGroup, ExclusivityGroupPriority.
DECOMP(007778CC, NodePlaySndSFX_New_007778CC) {
    NewNamedNode(ctx, 0x00B1FD4F, 16, {
        {0x00B1FD5B, 2, 0, 0, 0},
        {0x00B1FD63, 2, 1, 0, 1},
        {0x00B1FD68, 4, 1, 0, 2},
        {0x00B1FD79, 4, 1, 1, 3},
        {0x00B1FD81, 2, 1, 0, 5},
        {0x00B1FD8C, 2, 1, 0, 6},
        {0x00B1FD9D, 4, 1, 1, 4},
        {0x00B1FDAC, 2, 1, 0, 7},
        {0x00B1FDB2, 2, 1, 0, 8},
        {0x00B1FDC3, 0, 1, 0, 9}
    });
}

// Node* New(s16 id): the "RCameraShake" event node (Mem_Alloc(24, 0, 16), record 16):
// Radius, Duration, Magnitude, FalloffMode, FalloffStartTime, Pitch, Yaw, Roll, PitchFrequency, YawFrequency, RollFrequency, FalloffStartDistance.
DECOMP(00778BEC, NodeCameraShake_New_00778BEC) {
    NewNamedNode(ctx, 0x00B1FEDF, 16, {
        {0x00B1FEEC, 3, 1, 0, 0},
        {0x00B1FEF3, 3, 1, 0x3F800000, 1},
        {0x00B1FEFC, 3, 1, 0x3F800000, 2},
        {0x00B1FF06, 2, 1, 0x85D98325, 3},
        {0x00B1FF12, 3, 1, 0, 4},
        {0x00B1FF23, 3, 1, 0x3F800000, 5},
        {0x00B1FF29, 3, 1, 0, 6},
        {0x00B1FF2D, 3, 1, 0x3F800000, 7},
        {0x00B1FF32, 3, 1, 0x40800000, 8},
        {0x00B1FF41, 3, 1, 0x40D00000, 9},
        {0x00B1FF4E, 3, 1, 0x40200000, 10},
        {0x00B1FF5C, 3, 1, 0xBF800000, 11}
    });
}

// Node* New(s16 id): the "RFireProjectile" event node (Mem_Alloc(24, 0, 16), record 32):
// AttachBoneName, ProjDefName, XOffset, YOffset, ZOffset, PredictTargetPos, UseHighArc, UseBoneOrient, InvertBoneOrient, ForceLaunchAngle, AttackFailAnim, AttackFailEffectName, DirX, DirY, DirZ, TargetOffsetX, TargetOffsetY, TargetOffsetZ.
DECOMP(0077A404, NodeFireProjectile_New_0077A404) {
    NewNamedNode(ctx, 0x00B2011D, 32, {
        {0x00B2012D, 5, 1, 0, 0},
        {0x00B2013C, 5, 0, 0, 1},
        {0x00B20148, 3, 1, 0, 2},
        {0x00B20150, 3, 1, 0, 3},
        {0x00B20158, 3, 1, 0, 4},
        {0x00B20160, 4, 1, 0, 8},
        {0x00B20171, 4, 1, 0, 5},
        {0x00B2017C, 4, 1, 0, 6},
        {0x00B2018A, 4, 1, 0, 7},
        {0x00B2019B, 3, 1, 0, 9},
        {0x00B201AC, 5, 1, 0, 10},
        {0x00B201BB, 5, 1, 0, 11},
        {0x00B201D0, 3, 1, 0, 12},
        {0x00B201D5, 3, 1, 0, 13},
        {0x00B201DA, 3, 1, 0, 14},
        {0x00B201DF, 3, 1, 0, 15},
        {0x00B201ED, 3, 1, 0, 16},
        {0x00B201FB, 3, 1, 0, 17}
    });
}
// Node* New(s16 id): the "RPlayDirScript" event node (Mem_Alloc(24, 0, 16), record 16):
// DirScriptName, Mode (an enum: default hash 0x36B9B6DE, a name list of 2
// holding "PlayInObj"), Attach, InheritSurface.
DECOMP(00777D58, NodePlayDirScript_New_00777D58) {
    NewNamedNode(ctx, 0x00B1FDF0, 16, {
        {0x00B1FDFF, 2, 0, 0, 0},
        {0x00B1FE0D, 2, 1, 0x36B9B6DE, 1, 2, 0x00B1FE12},
        {0x00B1FE1C, 4, 1, 0, 2},
        {0x00B1FE23, 4, 1, 0, 3}
    });
}

// Node* New(s16 id): the "CIsTimer" condition node (Mem_Alloc(24, 0, 16), record 16):
// TimerName, Value, Operator (an enum: default hash 0x59D794DD, a name list
// of 2 holding "LargerEqual" and "LessEqual").
DECOMP(00776094, CondIsTimer_New_00776094) {
    NewNamedNode(ctx, 0x00B1FB74, 16, {
        {0x00B1FB7D, 5, 0, 0, 0},
        {0x00B1FB87, 3, 0, 0, 1},
        {0x00B1FB8D, 2, 1, 0x59D794DD, 2, 2, 0x00B1FB96, 0x00B1FBA2}
    }, true);
}
