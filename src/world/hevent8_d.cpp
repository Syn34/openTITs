// World_HandleEvent callees, part 8d: an object's attachment to a holder
// (detach the current one, attach the new one at a position and rotation).
#include "hevent8.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;

namespace sly4 {
namespace hevent8d {
namespace dfn {
constexpr u32 Obj_Detach0D72C0 = 0x000D72C0;  // (holder, Object*)
constexpr u32 Obj_Attach0D15C0 = 0x000D15C0;  // (Object*, AttachDesc*) (name?)
constexpr u32 Msg_Post = 0x00470660;          // (msg, key, a, b)
constexpr u32 Math_Cosf = 0x007344E0;
constexpr u32 Math_Sinf = 0x007345F0;         // (f1 x) -> f1 (name?)
constexpr u32 PtrVector_Clear = 0x0054EC1C;
constexpr u32 Nop = 0x0054EC18;
constexpr u32 SmallFree = 0x00552420;         // (p, size)
}  // namespace dfn

// Object: +184 u64 flags, +216 def (its +240 block: +132 default bone,
// +144 vec3 offset, +160 vec3 Euler angles in degrees), +244 controller,
// +424 holder, +432 u32, +658 / +659 u8 (bit 0x40).
constexpr u32 kObjFlags = 184, kObjDef = 216, kObjCtrl = 244, kObjHolder = 424, kObj432 = 432;
constexpr u32 kObjB658 = 658, kObjB659 = 659, kObjUses = 178, kObjMsgArg = 168;
constexpr u32 kDefAttach = 240, kAttBone = 132, kAttOffset = 144, kAttAngles = 160;
constexpr u64 kFlagAttached = 0x8000, kFlagHolding = 0x20000, kFlagBusy = 1ull << 39;
constexpr u32 kConsts = 0x00121F98;   // u64 1 << 39 (flag), f32 pi/180 at +8, f32 0.5 at +12
constexpr u32 kIdentityQuat = 0x01164C80;
constexpr u32 kMsgAttachKey = 0x600AE700, kMsgAttachA = 0x35007D87;
// Vtable slots.
constexpr u32 kVSetActive = 92, kVSetMode = 116, kVSetVisible = 40, kVCtrlBusy = 148;
// The attach descriptor (stack, 228 bytes): +0 u8, +64 u8, +128 vec3 pos,
// +144 quat, +160 bone, +164..+172 0, +176 flags, +180 PtrVector* (freed
// after the call when set), +192 holder, +196 0, +200 -1, +204 / +208 0,
// +212 -1, +216..+224 0.
constexpr u32 kDescSize = 256;
constexpr u32 kDPos = 128, kDQuat = 144, kDBone = 160, kDFlags = 176, kDVec = 180, kDHolder = 192;
}  // namespace hevent8d
}  // namespace sly4

using namespace sly4::hevent8d;

namespace {
u64 ld64(u32 ea) { return load<u64>(ea); }
}  // namespace

// void Obj_Reset121FA8(Object* o, Object* holder, const u32* bone, const
// vec3* pos, const quat* rot, u8 flag) (name?; attaches o to holder): nothing
// when holder is already o's. Otherwise the current holder is let go
// (SetActive(0, -1), SetMode(255, 1, 1, 0), Obj_Detach0D72C0, SetVisible(0),
// its +184 bit 17 cleared; o +658/+659 bit 0x40 cleared first when o's flag
// bit 39 is set). A new holder is attached through 0x000D15C0 with a stack
// descriptor: the bone (*bone, or the def's default), pos (or the def's
// offset) and rot (or the def's Euler angles as a quaternion), flag in
// bit 30; then Msg_Post(o, 0x600AE700, 0x35007D87, holder +168), the
// holder's SetActive(1, -1), use count +178 + 1 and flag bit 17, and when
// its controller is busy o gets bit 39 and +658/+659 bit 0x40. Finally,
// with a holder o gets flag 0x8000 (and bit 39 when the holder's controller
// is busy); without one bits 15 and 39 and +432 are cleared.
DECOMP(00121FA8, Obj_Reset121FA8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), bone = arg<u32>(ctx, 2);
    u32 pos = arg<u32>(ctx, 3);
    const u32 rot = arg<u32>(ctx, 4), flag = arg<u32>(ctx, 5);
    const u32 att = load<u32>(o + kObjDef) + kDefAttach;
    const u32 cur = load<u32>(o + kObjHolder);
    if (x == cur) return;
    if (cur != 0) {
        if (ld64(o + kObjFlags) & kFlagBusy) {
            const u8 a = load<u8>(o + kObjB658), b = load<u8>(o + kObjB659);
            store<u8>(o + kObjB658, a & ~0x40);
            store<u8>(o + kObjB659, b & ~0x40);
        }
        hevent::vcall(ctx, cur, kVSetActive, {cur, 0, ~0ull});
        const u32 c2 = load<u32>(o + kObjHolder);
        hevent::vcall(ctx, c2, kVSetMode, {c2, 255, 1, 1, 0});
        call(ctx, dfn::Obj_Detach0D72C0, {o, load<u32>(o + kObjHolder)});
        const u32 c3 = load<u32>(o + kObjHolder);
        hevent::vcall(ctx, c3, kVSetVisible, {c3, 0});
        const u32 c4 = load<u32>(o + kObjHolder);
        store<u64>(c4 + kObjFlags, ld64(c4 + kObjFlags) & ~kFlagHolding);
        store<u32>(o + kObjHolder, 0);
    }
    if (x != 0) {
        Stack frame(ctx, kDescSize);  // the descriptor lives at r1+112
        const u32 d = frame.addr();
        // The flags word starts from an uninitialised stack word (its low 24
        // bits survive), as in the original.
        u32 fl = load<u32>(d + kDFlags) & 0x00FFFFFFu;
        store<u32>(o + kObjHolder, x);
        for (u32 k = 0; k < 12; k += 4) store<u32>(d + kDPos + k, load<u32>(kZeroVec + k));
        for (u32 k = 0; k < 16; k += 4) store<u32>(d + kDQuat + k, load<u32>(kIdentityQuat + k));
        for (u32 off : {164u, 168u, 172u, 180u}) store<u32>(d + off, 0);
        store<u32>(d + kDFlags, fl);
        store<u8>(d + 0, 0);
        store<u8>(d + 64, 0);
        for (u32 off : {196u, 204u, 208u, 216u, 220u, 224u}) store<u32>(d + off, 0);
        store<u32>(d + 200, 0xFFFFFFFFu);
        store<u32>(d + 212, 0xFFFFFFFFu);
        store<u32>(d + kDHolder, x);
        store<u32>(d + kDBone, bone != 0 ? load<u32>(bone) : load<u32>(att + kAttBone));
        if (pos == 0) pos = att + kAttOffset;
        for (u32 k = 0; k < 12; k += 4) store<u32>(d + kDPos + k, load<u32>(pos + k));
        if (rot != 0) {
            for (u32 k = 0; k < 16; k += 4) store<u32>(d + kDQuat + k, load<u32>(rot + k));
        } else {
            // Euler angles (degrees) to a quaternion, as the original's
            // single-precision sequence.
            const f64 k = lfs_f64(kConsts + 8), half = lfs_f64(kConsts + 12);
            const f64 ay = fmuls(lfs_f64(att + kAttAngles + 4), k);
            const f64 az = fmuls(lfs_f64(att + kAttAngles + 8), k);
            const f64 ax = fmuls(lfs_f64(att + kAttAngles + 0), k);
            const f64 hy = fmuls(ay, half);
            const f64 cy = callf(ctx, dfn::Math_Cosf, {}, {hy});
            const f64 hz = fmuls(az, half);
            const f64 cz = callf(ctx, dfn::Math_Cosf, {}, {hz});
            const f64 hx = fmuls(ax, half);
            const f64 cx = callf(ctx, dfn::Math_Cosf, {}, {hx});
            const f64 sy = callf(ctx, dfn::Math_Sinf, {}, {hy});
            const f64 sz = callf(ctx, dfn::Math_Sinf, {}, {hz});
            const f64 sx = callf(ctx, dfn::Math_Sinf, {}, {hx});
            const f64 cc = fmuls(cy, cz), ss = fmuls(sy, sz), cs = fmuls(cz, sy), sc = fmuls(cy, sz);
            const f64 ccs = fmuls(cc, sx), sss = fmuls(ss, sx), css = fmuls(cs, sx), scs = fmuls(sc, sx);
            store<f32>(d + kDQuat + 0, static_cast<f32>(std::fma(ss, cx, ccs)));
            store<f32>(d + kDQuat + 4, static_cast<f32>(std::fma(cs, cx, scs)));
            store<f32>(d + kDQuat + 8, static_cast<f32>(std::fma(sc, cx, -css)));
            store<f32>(d + kDQuat + 12, static_cast<f32>(std::fma(cc, cx, -sss)));
            fl = load<u32>(d + kDFlags);
        }
        fl = (fl & ~0x40000000u) | ((flag & 1) << 30);
        store<u32>(d + kDFlags, (fl & ~0x04000000u) | 0x02000000u);
        call(ctx, dfn::Obj_Attach0D15C0, {o, d});
        store<u64>(o + kObjFlags, ld64(o + kObjFlags) | kFlagAttached);
        call(ctx, dfn::Msg_Post, {o, kMsgAttachKey, kMsgAttachA, load<u32>(x + kObjMsgArg)});
        hevent::vcall(ctx, x, kVSetActive, {x, 1, ~0ull});
        store<u16>(x + kObjUses, static_cast<u16>(load<u16>(x + kObjUses) + 1));
        store<u64>(x + kObjFlags, ld64(x + kObjFlags) | kFlagHolding);
        const u32 ctrl = load<u32>(x + kObjCtrl);
        if (ctrl != 0 && static_cast<u32>(hevent::vcall(ctx, ctrl, kVCtrlBusy, {ctrl})) != 0) {
            const u8 a = load<u8>(o + kObjB658);
            const u64 f = ld64(o + kObjFlags) | load<u64>(kConsts);
            const u8 b = load<u8>(o + kObjB659);
            store<u8>(o + kObjB658, a | 0x40);
            store<u64>(o + kObjFlags, f);
            store<u8>(o + kObjB659, b | 0x40);
        }
        const u32 v = load<u32>(d + kDVec);
        if (v != 0) {
            call(ctx, dfn::PtrVector_Clear, {v});
            call(ctx, dfn::Nop, {v});
            call(ctx, dfn::SmallFree, {load<u32>(d + kDVec), 20});
        }
    }
    const u32 h = load<u32>(o + kObjHolder);
    if (h == 0) {
        const u64 f = ld64(o + kObjFlags) & ~(kFlagAttached | kFlagBusy);
        store<u32>(o + kObj432, 0);
        store<u64>(o + kObjFlags, f);
        return;
    }
    store<u64>(o + kObjFlags, ld64(o + kObjFlags) | kFlagAttached);
    const u32 ctrl = load<u32>(h + kObjCtrl);
    if (ctrl == 0) return;
    if (static_cast<u32>(hevent::vcall(ctx, ctrl, kVCtrlBusy, {ctrl})) == 0) return;
    store<u64>(o + kObjFlags, ld64(o + kObjFlags) | load<u64>(kConsts));
}
