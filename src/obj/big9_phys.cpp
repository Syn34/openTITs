// Game code, large-function sweep part 9: PhysObj base init and the body
// add / remove pass (000E01C0, 000E2FF0).
#include "big9.h"

using namespace sly4;
using namespace sly4::big9;
using namespace sly4::big9::b9fn;

namespace {
constexpr u32 kBaseVt = 0x00B126B8u;   // PhysObj base vtable
// Object fields.
constexpr u32 kObjFlags172 = 172;      // 0x80: no physics world
constexpr u32 kObjDef = 216;           // def (see below)
constexpr u32 kObjF344 = 344;          // f32 (tmpl +184)
constexpr u32 kObjF348 = 348;          // f32 (tmpl +184)
constexpr u32 kObjW352 = 352;
constexpr u32 kObjEffect = 356;        // Effect*
constexpr u32 kObjEffectDef = 360;     // effect def (EffectDef_Find(tmpl +192)) or 0
constexpr u32 kObjW364 = 364;
constexpr u32 kObjQueue = 368;
constexpr u32 kObjW372 = 372;
constexpr u32 kObjFlags376 = 376;
constexpr u32 kObjBodies = 388;        // PtrVector of 48-byte body records {+28 hkpEntity*, +32 volume} or 0
constexpr u32 kObjState = 392;         // bit 0 kept; bits 1.. the physics mode
constexpr u32 kObjW408 = 408;
constexpr u32 kObjGroup = 412;         // collision group (into body +44 bits 16..31)
// Def fields.
constexpr u32 kDefShapes = 192;        // shape list {+0 entries (160 bytes: +124 flags, +144 kind)}
constexpr u32 kDefKind = 208;          // 0xA970EC1F: box volumes
constexpr u32 kDefHalfExt = 224;       // f32[3] half extents
constexpr u32 kDefFlags = 300;         // 0x01000000: mask 0xFFFF (else 0xFFFE)
constexpr u32 kBoxKind = 0xA970EC1Fu;
constexpr u32 kConsts = 0x000E2FB0u;   // .text: f32 0.5, f32 1.0; lane masks y / z / w at +16 / +32 / +48
// Vtable slots.
constexpr u32 kVtSetPhysics = 88;      // (o, 1, 0, 1, 1)
constexpr u32 kVtShapeArg = 76;        // (o) -> u32 (passed to the shape init)
}  // namespace

// u32 PhysObjBase_Init(PhysObj* o, const Tmpl* t)
// Object base init (0x000D3578), then the base vtable and cleared fields:
// +376 keeps its low 20 bits with bit 31 = t +196 bit 0, 0x80000 = t +152
// non-zero, 0x40000 = t +172 non-zero, 0x8000 set, 0x10000000 = t +32 ->
// +188 bit 0x2000; +344 / +348 = t +184; the effect def for t +192 (when
// set); then Obj_SetShadow0E0000(o, t +188) (its result returned).
DECOMP(000E01C0, PhysObjBase_Init) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = arg<u32>(ctx, 1);
    call(ctx, kObjBaseInit, {o, t});
    u32 fl = load<u32>(o + kObjFlags376);
    const u32 b196 = load<u8>(t + 196);
    const u32 f184 = load<u32>(t + 184);  // lfs/stfs pair: raw word
    fl = (fl & 0x000FFFFFu) | ((b196 & 1u) << 31);
    store<u32>(o + kObjFlags376, fl);
    SetF1Bits(ctx, f184);  // the original's f1 (lfs) at the calls below
    const u32 h152 = load<u16>(t + 152);
    store<u32>(o, kBaseVt);
    store<u32>(o + kObjEffect, 0);
    store<u32>(o + kObjEffectDef, 0);
    store<u32>(o + kObjW364, 0);
    fl = (fl & ~0x00080000u) | (h152 != 0 ? 0x00080000u : 0u);
    store<u32>(o + kObjFlags376, fl);
    const u32 h172 = load<u16>(t + 172);
    store<u32>(o + kObjQueue, 0);
    store<u32>(o + kObjW372, 0);
    store<u32>(o + kObjF344, f184);
    store<u32>(o + kObjF348, f184);
    store<u32>(o + kObjW352, 0);
    fl = (fl & ~0x00074000u) | (h172 != 0 ? 0x00040000u : 0u) | 0x8000u;
    store<u32>(o + kObjFlags376, fl);
    const u32 h188 = load<u16>(load<u32>(t + 32) + 188);
    fl = (fl & ~0x10000000u) | ((h188 & 0x2000u) << 15);
    store<u32>(o + kObjFlags376, fl);
    const u32 eff = load<u32>(t + 192);
    if (eff != 0) store<u32>(o + kObjEffectDef, static_cast<u32>(call(ctx, kEffectDefFind, {eff, 0, 1})));
    call(ctx, kObjSetShadow, {o, load<u32>(t + 188)});  // r3 = its result
}

// u32 PhysObj_SetMode(PhysObj* o, s32 mode)
// Nothing when +392 >> 1 already equals mode (r3 = +388). Mode 4 (and 3,
// recorded as 5) removes each record's body from its world and notifies the
// shape system of its volume; modes 1 and 7 add the bodies that have none
// (unless +172 0x80): group +412 into body +44, hkpWorld::addEntity, the
// entry's attach hook (entry +124 0x10000000), and for box defs (def +208
// 0xA970EC1F, entry kind 0 or box) a new box volume: from the body when the
// def's half extents are zero, else from o's matrix (+96) raised by half
// the height (def +228); then vfunc +88 (o, 1, 0, 1, 1) and +408 = 0
// unless the old mode is 2. Finally +392 = bit 0 | mode << 1 (returned).
DECOMP(000E2FF0, PhysObj_SetMode) {
    const u32 o = arg<u32>(ctx, 0);
    u32 mode = arg<u32>(ctx, 1);
    const u32 def = load<u32>(o + kObjDef);
    u32 vec = load<u32>(o + kObjBodies);
    const u32 fl172 = load<u32>(o + kObjFlags172);
    if (static_cast<s32>(load<u32>(o + kObjState) >> 1) == static_cast<s32>(mode)) {
        ret(ctx, vec);
        return;
    }
    Stack frame(ctx, 96);  // the original's sp+112..+207
    const u32 sp = frame.addr() - 112;
    if (mode == 4 || mode == 3) {
        if (mode == 3) mode = 5;
        if (vec != 0 && load<u16>(vec + 8) != 0) {
            u32 off = 0;
            for (u32 i = 0;; ++i, off += 48) {
                const u32 n = vec != 0 ? load<u16>(vec + 8) : 0u;
                if (i >= n) break;
                const u32 rec = load<u32>(vec) + off;
                const u32 body = load<u32>(rec + 28);
                if (body == 0) continue;
                const u32 world = load<u32>(body + 8);
                if (world != 0) call(ctx, kHkRemoveEntity, {sp + 200, world, body});
                call(ctx, kVolNotify, {load<u32>(kShapeSystem), rec + 32});
                vec = load<u32>(o + kObjBodies);
            }
        }
    } else if (mode == 7 || mode == 1) {
        bool add = false;
        u32 hk = 0;
        if ((fl172 & 0x80u) == 0) {
            hk = load<u32>(load<u32>(load<u32>(kWorldPtr) + kWorldPhys) + 4);
            add = vec != 0 && load<u16>(vec + 8) != 0;
        }
        if (add) {
            const V4 axis = AndcSplat3(load<u32>(kConsts + 4), Lvx(kConsts + 16), Lvx(kConsts + 32), Lvx(kConsts + 48));
            const f32 half = F(kConsts);
            for (u32 i = 0;; ++i) {
                const u32 v = load<u32>(o + kObjBodies);
                const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
                if (i >= n) break;
                const u32 rec = load<u32>(v) + 48 * i;
                const u32 entry = load<u32>(load<u32>(def + kDefShapes)) + 160 * i;
                const u32 body = load<u32>(rec + 28);
                if (body == 0 || load<u32>(body + 8) != 0) continue;
                store<u32>(body + 44, (load<u32>(body + 44) & 0x1Fu) | (load<u32>(o + kObjGroup) << 16));
                call(ctx, kHkAddEntity, {hk, body, 1});
                if ((load<u32>(load<u32>(load<u32>(def + kDefShapes)) + 160 * i + 124) & 0x10000000u) != 0) {
                    Stvx(sp + 112, axis);
                    call(ctx, kPhysBodyAttach, {o, i, sp + 112});
                }
                if (load<u32>(def + kDefKind) != kBoxKind) continue;
                const u32 kind = load<u32>(entry + 144);
                if (kind != 0 && kind != kBoxKind) continue;
                const u32 mask = (load<u32>(def + kDefFlags) & 0x01000000u) != 0 ? 0xFFFFu : 0xFFFEu;
                const bool zero = load<u64>(def + kDefHalfExt) == load<u64>(kZero3) && load<u32>(def + kDefHalfExt + 8) == load<u32>(kZero3 + 8);
                const u32 vol = rec + 32;
                if (zero) {
                    const u32 sys = load<u32>(kShapeSystem);
                    const u32 a = static_cast<u32>(VCall(ctx, o, kVtShapeArg, {o}));
                    call(ctx, kShapeInitBody, {sp + 192, sys, body, 0, a, mask});
                    call(ctx, kVolSetShape, {vol, sp + 192});
                    call(ctx, kShapeRel, {sp + 192});
                } else {
                    // f32 m[4][4] at sp+128 from o's matrix (+96): the x/y/z
                    // words of each row (the w words left as stack garbage,
                    // as in the original), the position y raised by half
                    // the height.
                    const u32 m = sp + 128, src = o + 96;
                    for (u32 r = 0; r < 3; ++r)
                        for (u32 k = 0; k < 3; ++k) CopyW(m + 16 * r + 4 * k, src + 16 * r + 4 * k);
                    CopyW(m + 48, src + 48);
                    const u32 w56 = load<u32>(src + 56);
                    const f64 y = F(src + 52);
                    store<u32>(m + 56, w56);
                    const f32 ny = Fmadds(F(def + kDefHalfExt + 4), half, y);
                    store<f32>(m + 52, ny);
                    ctx.fpr[1] = ny;  // the original leaves f1 here
                    const u32 sys = load<u32>(kShapeSystem);
                    const u32 a = static_cast<u32>(VCall(ctx, o, kVtShapeArg, {o}));
                    call(ctx, kShapeInit, {sp + 196, sys, m, def + kDefHalfExt, a, mask});
                    call(ctx, kVolSetShape, {vol, sp + 196});
                    call(ctx, kShapeRel, {sp + 196});
                }
            }
        }
        VCall(ctx, o, kVtSetPhysics, {o, 1, 0, 1, 1});
        if ((load<u32>(o + kObjState) >> 1) != 2) store<u32>(o + kObjW408, 0);
    }
    const u32 st = (load<u32>(o + kObjState) & 1u) | (mode << 1);
    store<u32>(o + kObjState, st);
    ret(ctx, st);
}
