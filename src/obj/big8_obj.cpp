// Game code, large-function sweep part 8: object re-init after a reset /
// state match (000E0620).
#include "big8.h"

using namespace sly4;
using namespace sly4::big8;
using namespace sly4::big8::b8fn;

namespace {
constexpr u32 kObjMsgArg = 168;      // u32 passed to Msg_Post
constexpr u32 kObjEffect = 356;      // Effect*
constexpr u32 kObjQueue = 368;       // event queue object (its +172 0x40000: live; +188 bit 0: +172 s8 kind valid) or 0
constexpr u32 kObjFlags376 = 376;    // 0x4000: re-init done

// PhysObj (000E38F0) fields and callees.
constexpr u32 kObjWorldObj = 160;    // WorldObj* (+80 matrix, +204 name hash, +272 children?) or 0
constexpr u32 kObjDef = 216;         // def: +192 shape list {+0 items (160 bytes each), +8 u16 count}, +204 f32, +300 flags
constexpr u32 kObjBodies = 388;      // PtrVector of 48-byte body records (+0 data, +4 u16 capacity, +6 u16 element size, +8 u16 count, +10 u16 high water, +12 flags) or 0
constexpr u32 kPhysVt = 0x00B12898u;      // PhysObj vtable
constexpr u32 kPhysVt380 = 0x00B12A44u;   // secondary vtable at +380 (Havok listener?)
constexpr u32 kPhysVt384 = 0x00B12A6Cu;   // secondary vtable at +384
constexpr u32 kUnknownName = 0x00B0DA98u; // "<unknown>"
constexpr u32 kMasks = 0x000E38C0u;       // .text: lane masks y / z / w
constexpr u32 kFindByName = 0x00F31D20u;  // OPD: WorldObj_ForEach callback (ctx {hash, found})
constexpr u32 kBaseInit = 0x000E01C0u;    // (o, tmpl): base class init
constexpr u32 kPostInit = 0x000E2FF0u;    // (o, u32 mode, 0)
constexpr u32 kWorldObjUpdateLod = 0x0057EE00u;  // (wo, 0, 0)
constexpr u32 kWorldObjGetPeer = 0x0057A428u;    // (wo) -> peer or 0
constexpr u32 kWorldObjForEach = 0x0057F5D8u;    // (wo, OPD callback, ctx, 0)
constexpr u32 kSkeletonFindBone = 0x005A93D8u;   // (skel, u32 hash) -> s32 bone or -1
constexpr u32 kShapeReset = 0x0063778Cu;         // (Shape*)
constexpr u32 kVolumeSetShape = 0x00637D14u;     // (dst, src)
constexpr u32 kShapeRelease = 0x00638498u;       // (Shape*)
constexpr u32 kBodySetName = 0x007CE950u;        // (body + 120, const char*) (Havok, name?)
constexpr u32 kBodyAddListener = 0x0091A82Cu;    // (body, listener) (Havok, name?)
constexpr u32 kBodyAddListener2 = 0x0091A5F8u;   // (body, listener) (Havok, name?)
constexpr u32 kHkSetTransform = 0x0091EAB8u;     // (body, const hkTransform*)
constexpr u32 kHkAddProperty = 0x0093FE00u;      // (body, u32 key, u32 value)
constexpr u32 kPtrVectorConstruct = 0x005516E0u; // (v)
constexpr u32 kPtrVectorReserve = 0x00551718u;   // (v, n)
constexpr u32 kMatrixNormalize = 0x0056D374u;    // (f32 m[4][4])
constexpr u32 kObjAttachMatrix = 0x000E3534u;    // Obj_AttachMatrix (o, out, el, base)
}  // namespace

// u32 Obj_Reinit(Obj* o)
// Once (+376 0x4000 set here): the effect gets hash 0xB1BC9747, a fresh
// event (EventInit 0x0046582C) goes to the world query with the live
// queue's kind (else -1), then Msg_Post(o, 0x6A817B9D, 0x35A7B402, +168).
// Returns Msg_Post's result (the +376 word when already done).
DECOMP(000E0620, Obj_Reinit) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 fl = load<u32>(o + kObjFlags376);
    if ((fl & 0x4000u) != 0) {
        ret(ctx, fl);
        return;
    }
    const u32 e = load<u32>(o + kObjEffect);
    store<u32>(o + kObjFlags376, fl | 0x4000u);
    if (e != 0) call(ctx, kEffectSetHash, {e, 0xB1BC9747u});
    Stack frame(ctx, 48);  // the original's event at sp+112..+151
    const u32 ev = frame.addr();
    for (u32 k = 0; k < 6; ++k) store<u32>(ev + 4 * k, 0);
    call(ctx, kEventInitB, {ev, o, 0, 0, 0});
    u64 kind = ~0ull;
    const u32 q = load<u32>(o + kObjQueue);
    if (q != 0 && (load<u32>(q + 172) & 0x40000u) != 0 && (load<u32>(q + 188) & 1u) != 0) kind = Sx(static_cast<u32>(static_cast<s32>(load<s8>(q + 172))));
    call(ctx, kWorldQuery, {load<u32>(kWorldPtr), kind, ev});
    call(ctx, kMsgPost, {o, 0x6A817B9Du, 0x35A7B402u, load<u32>(o + kObjMsgArg)});
}

// u32 PhysObj_Init(PhysObj* o, const Tmpl* tmpl)
// Base init, then the PhysObj vtables (+0 / +380 / +384), cleared state
// (+388..+416, +392 bit 0, +396 / +420 top bit), +412 = tmpl +200, +404 =
// def +204, +420 0x40000000 = def +300 0x04000000; the WorldObj's LOD
// update. For each shape entry (def +192, 160 bytes, +32 shape def) a body
// record: body = shape def vfunc +32, named after the WorldObj (vfunc +12,
// else "<unknown>"), properties 0xA734004D = 0x7A30D7E3 and 0x91C7CF5B = o,
// the +380 / +384 listeners unless def +300 0x08000000, the bone (entry +36
// hash through the skeleton; when none, the WorldObj or a child with that
// name hash), transform from the attachment (entry +16 offset) normalised,
// then pushed onto +388 (made on first use: 20 bytes, element size 48).
// The vector gets flag 0x2; post-init mode is 7 when def +300 has the top
// bit or an entry +124 had it, else 1. Bit 0 of +392: def +300 0x00400000,
// else whether the WorldObj's peer has a +136.
// Quirk: a failed vector allocation is used anyway (null).
DECOMP(000E38F0, PhysObj_Init) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 tmpl = arg<u32>(ctx, 1);
    call(ctx, kBaseInit, {o, tmpl});
    const u32 b392 = load<u32>(o + 392) & 1u & ~1u;  // (the bitfield write leaves 0)
    const u32 w396 = load<u32>(o + 396) & 0x7FFFFFFFu;
    store<u32>(o, kPhysVt);
    store<u32>(o + 380, kPhysVt380);
    store<u32>(o + 392, b392);
    u32 w420 = load<u32>(o + 420) & 0x7FFFFFFFu;
    store<u32>(o + 384, kPhysVt384);
    store<u32>(o + 396, w396);
    store<u32>(o + kObjBodies, 0);
    w420 |= 0x40000000u;
    const u32 t200 = load<u32>(tmpl + 200);
    store<u32>(o + 408, 0);
    const u32 def = load<u32>(o + kObjDef);
    store<u32>(o + 400, 0);
    store<u32>(o + 412, t200);
    store<u32>(o + 420, w420);
    store<u32>(o + 416, 0);
    CopyW(o + 404, def + 204);
    SetF1Bits(ctx, load<u32>(def + 204));
    w420 = (w420 & ~0x40000000u) | ((load<u32>(def + 300) << 4) & 0x40000000u);
    store<u32>(o + 420, w420);
    if (load<u32>(o + kObjWorldObj) != 0) call(ctx, kWorldObjUpdateLod, {load<u32>(o + kObjWorldObj), 0, 0});

    u32 any = 0;
    const u32 flag = 1;  // the original's sp+328, never changed
    {
        Stack frame(ctx, 224);  // the original's sp+112..+335
        const u32 sp = frame.addr() - 112;
        const u32 el = sp + 112;   // attachment / body record: +0 offset f32[3], +16 target, +20 bone, +24 0, +28 body
        const u32 sh = sp + 144;   // Shape handle (+4: the byte stored as record +36)
        const u32 xf = sp + 160;   // hkTransform (4 rows)
        const u32 mat = sp + 224;  // f32 [4][4]
        const u32 find = sp + 288; // {hash, found}
        const V4 my = Lvx(kMasks), mz = Lvx(kMasks + 16), mw = Lvx(kMasks + 32);
        auto row = [&](u32 a) {
            const f32 x = std::bit_cast<f32>(LaneW(Lvlx(a), 0)), y = std::bit_cast<f32>(LaneW(Lvlx(a + 4), 0)), z = std::bit_cast<f32>(LaneW(Lvlx(a + 8), 0));
            return Andc(Sel(Sel(Splat1(x), Splat1(y), my), Splat1(z), mz), mw);
        };
        for (u32 i = 0;; ++i) {
            const u32 list = load<u32>(def + 192);
            const u32 count = list != 0 ? load<u16>(list + 8) : 0u;
            if (i >= count) break;
            const u32 item = load<u32>(list) + 160 * i;
            if (load<u32>(item + 32) == 0) continue;
            CopyW(el, kZero3);
            CopyW(el + 4, kZero3 + 4);
            CopyW(el + 8, kZero3 + 8);
            SetF1Bits(ctx, load<u32>(kZero3 + 8));
            store<u32>(el + 16, 0);
            store<u32>(el + 20, 0xFFFFFFFFu);
            store<u32>(el + 24, 0);
            store<u32>(el + 28, 0);
            call(ctx, kShapeReset, {sh});
            store<u32>(sh + 4, 0);
            const u32 sd = load<u32>(item + 32);
            const u32 body = static_cast<u32>(VCall(ctx, sd, 32, {sd}));
            store<u32>(el + 28, body);
            u64 name = kUnknownName;
            if (load<u32>(o + kObjWorldObj) != 0) {
                const u32 wo = load<u32>(o + kObjWorldObj);
                name = VCall(ctx, wo, 12, {wo});
            }
            call(ctx, kBodySetName, {body + 120, name});
            any = (any | (load<u32>(item + 124) >> 31)) != 0 ? 1u : 0u;
            call(ctx, kHkAddProperty, {load<u32>(el + 28), 0xA734004Du, 0x7A30D7E3u});
            call(ctx, kHkAddProperty, {load<u32>(el + 28), kObjProp, o});
            if ((load<u32>(def + 300) & 0x08000000u) == 0) {
                call(ctx, kBodyAddListener, {load<u32>(el + 28), o + 380});
                call(ctx, kBodyAddListener2, {load<u32>(el + 28), o + 384});
            }
            const u32 hash = load<u32>(item + 36);
            if (hash != 0) {
                u64 bone = 0xFFFFFFFFFFFFFFFFull;
                const u32 wo = load<u32>(o + kObjWorldObj);
                if (wo != 0) {
                    const u32 skel = static_cast<u32>(VCall(ctx, wo, 28, {wo}));
                    if (skel != 0) bone = call(ctx, kSkeletonFindBone, {skel, hash});
                }
                store<u32>(el + 20, static_cast<u32>(bone));
                if (static_cast<s32>(bone) == -1) {
                    const u32 w = load<u32>(o + kObjWorldObj);
                    const u32 h = load<u32>(item + 36);
                    u32 t = 0;
                    if (w == 0) t = 0;
                    else if (load<u32>(w + 204) == h) t = w;
                    else if (load<u32>(w + 272) == 0) t = 0;
                    else {
                        store<u32>(find, h);
                        store<u32>(find + 4, 0);
                        call(ctx, kWorldObjForEach, {w, kFindByName, find, 0});
                        t = load<u32>(find + 4);
                    }
                    store<u32>(el + 16, t);
                }
            }
            CopyW(el, item + 16);
            CopyW(el + 4, item + 20);
            CopyW(el + 8, item + 24);
            SetF1Bits(ctx, load<u32>(item + 24));
            const u32 wo = load<u32>(o + kObjWorldObj);
            const u32 base = wo != 0 ? wo + 80 : kIdentity;
            call(ctx, kObjAttachMatrix, {o, mat, el, base});
            call(ctx, kMatrixNormalize, {mat});
            Stvx(xf, row(mat));
            Stvx(xf + 16, row(mat + 16));
            Stvx(xf + 32, row(mat + 32));
            Stvx(xf + 48, row(mat + 48));
            call(ctx, kHkSetTransform, {load<u32>(el + 28), xf});
            store<u32>(sh + 4, load<u8>(load<u32>(el + 28) + 232));
            u32 v = load<u32>(o + kObjBodies);
            if (v == 0) {
                v = static_cast<u32>(call(ctx, kSmallAlloc, {20}));
                store<u32>(o + kObjBodies, v);
                if (v != 0) {
                    call(ctx, kPtrVectorConstruct, {v});
                    store<u16>(v + 6, 48);
                    v = load<u32>(o + kObjBodies);
                }
            }
            u32 n = load<u16>(v + 8);
            const u32 cap = load<u16>(v + 4);
            if (n == cap) {
                call(ctx, kPtrVectorReserve, {v, cap << 1});
                n = load<u16>(v + 8);
            }
            const u32 rec = load<u32>(v) + 48 * n;
            CopyW(rec, sp + 112);
            CopyW(rec + 4, sp + 116);
            CopyW(rec + 8, sp + 120);
            SetF1Bits(ctx, load<u32>(sp + 120));
            store<u32>(rec + 16, load<u32>(el + 16));
            store<u32>(rec + 20, load<u32>(el + 20));
            store<u32>(rec + 24, load<u32>(el + 24));
            store<u32>(rec + 28, load<u32>(el + 28));
            call(ctx, kVolumeSetShape, {rec + 32, sh});
            store<u32>(rec + 36, load<u32>(sh + 4));
            const u32 n1 = load<u16>(v + 8) + 1u;
            const u32 hi = load<u16>(v + 10);
            store<u16>(v + 8, static_cast<u16>(n1));
            store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFFu) ? hi : n1));
            call(ctx, kShapeRelease, {sh});
        }
    }
    const u32 v = load<u32>(o + kObjBodies);
    if (v != 0) store<u32>(v + 12, (load<u32>(v + 12) & ~2u) | ((flag & 1u) << 1));
    store<u32>(o + 400, load<u32>(tmpl + 204));
    const bool top = (load<u32>(def + 300) & 0x80000000u) != 0;
    call(ctx, kPostInit, {o, (top || any != 0) ? 7u : flag, 0});
    if ((load<u32>(def + 300) & 0x00400000u) != 0) {
        const u32 w = load<u32>(o + 392) | 1u;
        store<u32>(o + 392, w);
        ret(ctx, w);
        return;
    }
    const u32 wo = load<u32>(o + kObjWorldObj);
    if (wo == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 peer = static_cast<u32>(call(ctx, kWorldObjGetPeer, {wo}));
    if (peer == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = (load<u32>(o + 392) & ~1u) | (load<u32>(peer + 136) != 0 ? 1u : 0u);
    store<u32>(o + 392, w);
    ret(ctx, w);
}
