// World_HandleEvent callees, part 12d: the base object constructor and the
// effect set's stop.
#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent6;
using namespace sly4::hevent12;

namespace {
// Base object (as Obj_Construct364700): +0 vtable, +4, +8 Def*, +12, +16
// def +76, +20, +24 def +72 (body; replaced by its vt +56 clone), +28 120-byte
// controller, +32 vec3 (zero), +48 sub-object, +144 vec3 offset, +160 u32
// (bit 0 kept, kind << 1, bit 0 set).
constexpr u32 kObjBaseVtable = 0x00B1F678;
constexpr u32 kObVt = 0, kOb4 = 4, kObDef = 8, kOb12 = 12, kOb16 = 16, kOb20 = 20, kObBody = 24, kObCtl = 28;
constexpr u32 kObPos = 32, kObSub = 48, kObOffset = 144, kObKind = 160;
// Def: +72 body, +76, +84 s32 type (5: no default offset), +128 vec3 offset,
// +144 u32 flags (bit 31: clone the body, bit 30: body type check).
constexpr u32 kDefBody = 72, kDef76 = 76, kDefType = 84, kDefOffset = 128, kDefFlags = 144;
constexpr u32 kDefClone = 0x80000000u, kDefCheckBody = 0x40000000u;
constexpr s32 kDefTypeNoOffset = 5;
constexpr u32 kDefaultOffset = 0x0106C9F0;  // vec3 used when the def's offset is zero (name?)
// Body: +4 u32 (bits 10..15 type), +4 u16 refs (as the high half), +276 u32,
// +280 owner; vtable +56 (body, 5) -> clone.
constexpr u32 kBodyInfo = 4, kBodyTag = 276, kBodyOwner = 280, kBodyVtClone = 56;
constexpr u32 kBodyTagValue = 0x7DF841DC;
constexpr u32 kBodyTypeCheck = 8;
constexpr u32 kCtlSize = 120, kCtlFlags = 40, kCtlFlagOwned = 0x1000;
constexpr u32 kSubInit = 0x004A0EE0;       // (sub-object*) (name?)
constexpr u32 kBodyAttach = 0x00590FEC;    // (body, 1) (name?)
constexpr u32 kCtlConstruct = 0x0034BDEC;  // (ctl*, body) (name?)
constexpr u32 kBodySetMode = 0x004A5C20;   // (body, 13) (name?)

// Set (as hevent6.h): +4 tree root slot (+16 map's root pointer), +8 s32
// count, +16 map; node values have vtable +8 (stop).
constexpr u32 kSetRoot = 4, kSetCount = 8, kSetVtStop = 8;
constexpr u32 kTreeFind = 0x00793080;   // (tree, key, Node** out, root) -> non-zero when found (name?)
constexpr u32 kTreeErase = 0x00792490;  // (tree, tree, key, u8* out) (name?)
}  // namespace

// void Obj_BaseConstruct35695C(Object* o, u32 kind, Def* d) (name?): the base
// object constructor: fields from the def, a zero position, sub-object init,
// the def's offset (or the default one when it is zero and the type isn't
// 5), the body (cloned or referenced) owned by o, a new controller (+28; the
// owned flag is set even when the allocation fails, at address 40) and, when
// flagged, body mode 13 for type-8 bodies. r3 is left as the original
// leaves it.
DECOMP(0035695C, Obj_BaseConstruct35695C) {
    const u32 o = arg<u32>(ctx, 0), kind = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    store<u32>(o + kObDef, d);
    store<u32>(o + kObVt, kObjBaseVtable);
    store<u32>(o + kOb12, 0);
    const u32 d76 = load<u32>(d + kDef76);
    store<u32>(o + kOb20, 0);
    store<u32>(o + kObPos, load<u32>(hevent7::kZeroVec));
    const u32 body0 = load<u32>(d + kDefBody);
    store<u32>(o + kOb16, d76);
    store<u32>(o + kObBody, body0);
    store<u32>(o + kObCtl, 0);
    store<u32>(o + kObPos + 4, load<u32>(hevent7::kZeroVec + 4));
    store<u32>(o + kObPos + 8, load<u32>(hevent7::kZeroVec + 8));
    ctx.fpr[1] = hevent7::lfs_f64(hevent7::kZeroVec + 8);  // f1 as the original leaves it (copied through f1)
    call(ctx, kSubInit, {o + kObSub});
    const u32 kv = load<u32>(o + kObKind);
    store<u32>(o + kObOffset, load<u32>(d + kDefOffset));
    store<u32>(o + kObOffset + 4, load<u32>(d + kDefOffset + 4));
    store<u32>(o + kObOffset + 8, load<u32>(d + kDefOffset + 8));
    store<u32>(o + kObKind, (kv & 1) | (kind << 1) | 1);
    ctx.fpr[1] = hevent7::lfs_f64(d + kDefOffset + 8);
    if (static_cast<s32>(load<u32>(d + kDefType)) != kDefTypeNoOffset) {
        // Raw compare of the def's offset with the zero vector.
        const bool zero = load<u64>(d + kDefOffset) == load<u64>(hevent7::kZeroVec) &&
                          load<u32>(d + kDefOffset + 8) == load<u32>(hevent7::kZeroVec + 8);
        if (zero) {
            for (u32 k = 0; k < 12; k += 4) store<u32>(o + kObOffset + k, load<u32>(kDefaultOffset + k));
            ctx.fpr[1] = hevent7::lfs_f64(kDefaultOffset + 8);
        }
    }
    store<u32>(o + kOb4, 0);
    u32 body = load<u32>(o + kObBody);
    if (body == 0) return ret(ctx, o + kOb4);
    if ((load<u32>(d + kDefFlags) & kDefClone) == 0) {
        store<u16>(body + kBodyInfo, static_cast<u16>(load<u16>(body + kBodyInfo) + 1));
    } else {
        const u32 clone = static_cast<u32>(hevent::vcall(ctx, body, kBodyVtClone, {body, 5}));
        store<u32>(o + kObBody, clone);
        call(ctx, kBodyAttach, {clone, 1});
        body = load<u32>(o + kObBody);
    }
    store<u32>(body + kBodyOwner, o);
    store<u32>(body + kBodyTag, kBodyTagValue);
    const u32 p = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kCtlSize}));
    u32 ctl = 0;
    if (p != 0) {
        call(ctx, kCtlConstruct, {p, load<u32>(o + kObBody)});
        ctl = p;
    }
    store<u32>(o + kObCtl, ctl);
    store<u16>(ctl + kCtlFlags, static_cast<u16>(load<u16>(ctl + kCtlFlags) | kCtlFlagOwned));  // unchecked
    if ((load<u32>(d + kDefFlags) & kDefCheckBody) == 0) return ret(ctx, 0u);
    const u32 b = load<u32>(o + kObBody);
    const u32 type = (load<u32>(b + kBodyInfo) >> 10) & 0x3F;
    if (type != kBodyTypeCheck) return ret(ctx, type);
    call(ctx, kBodySetMode, {b, 13});
}

// void Set_Stop4BD1CC(Set* s) (name?): stops every value in the set's map
// (vt +8, in key order), then empties the tree one leftmost node at a time
// (0x00793080 / 0x00792490, count -1 for each node erased). Returns 0.
DECOMP(004BD1CC, Set_Stop4BD1CC) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 m = s + kSetDefs;
    for (u32 n = map_first(m); n != 0;) {
        const u32 v = load<u32>(n + kNodeValue);
        hevent::vcall(ctx, v, kSetVtStop, {v});
        const u32 cur = load<u32>(m + kMapNode);
        if (cur == 0) break;
        n = map_next(m, cur);
    }
    for (;;) {
        const u32 rootp = load<u32>(m);
        store<u16>(m + kMapDepth, 0);
        u32 n = load<u32>(rootp);
        if (n == 0) {
            store<u32>(m + kMapNode, 0);
            break;
        }
        for (u32 l; (l = load<u32>(n + kNodeLeft)) != 0; n = l) map_push(m, n);
        const u32 key = load<u32>(n + kNodeKey);
        store<u32>(m + kMapNode, n);
        Stack frame(ctx, 16);  // r1+112 Node* out, r1+116 u8
        store<u8>(frame.addr() + 4, 0);
        if (static_cast<u32>(call(ctx, kTreeFind, {s + kSetRoot, key, frame.addr(), load<u32>(s + kSetRoot)})) == 0) continue;
        call(ctx, kTreeErase, {s + kSetRoot, s + kSetRoot, key, frame.addr() + 4});
        store<u32>(s + kSetCount, load<u32>(s + kSetCount) - 1);
    }
    ret(ctx, 0u);
}
