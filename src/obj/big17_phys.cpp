// Game code, large-function sweep part 17: the constructor of a physics
// object (vtable 0x00B18B48) that takes over the rigid body of its def's
// Havok template and configures it from the def.
#include "big17.h"

using namespace sly4;
using namespace sly4::big17;

namespace {
namespace hfa {
using big4::CopyW;

constexpr u32 Base_Ctor_000E02D8 = 0x000E02D8;   // (o, def): the base constructor (Thunk_000E02D8)
constexpr u32 Shape_Reset = 0x0063778C;          // (Shape*)
constexpr u32 Finish_001F9F70 = 0x001F9F70;      // (o, 1, 0) -> r3 (name?)
constexpr u32 Hk_SetTransform = 0x0091EAB8;      // (body, const f32 m[4][4])
constexpr u32 Hk_Activate = 0x0091A4C8;          // (body)
constexpr u32 HkBody_AddListenerA82C = 0x0091A82C;  // (body, listener)
constexpr u32 HkBody_AddListenerA5F8 = 0x0091A5F8;  // (body, listener)

constexpr u32 kVt = 0x00B18B40;           // +8 the vtable, +416 / +456 the two listener vtables
constexpr u32 kPool = 0x001FA1F0;         // .text f32 0, lane masks at +16 / +32 / +48, f32 FLT_MAX at +64
constexpr u32 kUnknown = 0x00B0DA98;      // "<unknown>"
constexpr u32 kKeyFilter = 0x81D42737;    // property: collision filter info
constexpr u32 kKeyA = 0xA734004D, kValA = 0x7A30D7E3;
constexpr u32 kKeyObj = 0x91C7CF5B;       // property: the game object
constexpr u32 kZeroVec = 0x01164B40;      // f32[3] 0

// Frame layout (offsets from the 288-byte frame's base, as the original).
constexpr u32 kFrame = 288;
constexpr u32 kVec = 112;   // Vector4 velocity
constexpr u32 kMat = 128;   // 4x4 transform (rows (x, y, z, 0))

// The Vector4 (w0, w1, w2, 0) of the three words at src (lvlx / vspltw /
// vsel / vandc with the lane masks; src word-aligned).
inline void Row(u32 dst, u32 src) {
    store<u32>(dst, load<u32>(src));
    store<u32>(dst + 4, load<u32>(src + 4));
    store<u32>(dst + 8, load<u32>(src + 8));
    store<u32>(dst + 12, 0);
}
}  // namespace hfa
}  // namespace

// u32 f(Obj* o, const Def* d): the base constructor (0x000E02D8), vtables
// (+0, listeners +380 / +384), +400..+408 the zero vector, +416 / +420 /
// +428 = 0, +424 = -1, u16 +432 low 4 bits kept; Shape_Reset(o +436). From
// the template t = o +216: +424 = t +228, bit 0x80 of +432 = bit 0x40 of u8
// t +301. When t +296 is set, the body (t +296)->vfunc 32() -> o +416, named
// by (o +160)->vfunc 12() (or "<unknown>"), its transform from the rows d
// +48 / +64 / +80 / +96; d +244 (mass) above 0: 0x0091D618(body; f1) (for a
// keyframed / fixed body without a filter property first +44 = 4,
// 0x0091D470(body, 1, 1, 0), +42 = 4); exactly 0 for a dynamic body
// without a filter property: +44 = 2, 0x0091D470(body, 5, 1, 0), +42 = 0.
// d +248 below FLT_MAX: u16 body +510 = its high half. Dynamic bodies get
// the linear / angular velocities d +208 / +224 (activating first); d +252
// forces +42 = 2. Then the properties 0xA734004D = 0x7A30D7E3 and
// 0x91C7CF5B = o, and the two listeners. Always: +428 = d +240; returns
// 0x001F9F70(o, 1, 0).
DECOMP(001FA238, PhysObj_Ctor_001FA238) {
    using namespace b17fn;
    using namespace hfa;
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    call(ctx, Base_Ctor_000E02D8, {o, d});

    const u32 h = load<u16>(o + 432) & 0x000Fu;
    CopyW(o + 400, kZeroVec);
    store<u32>(o + 0, kVt + 8);
    store<u32>(o + 380, kVt + 416);
    store<u32>(o + 384, kVt + 456);
    CopyW(o + 404, kZeroVec + 4);
    store<u32>(o + 416, 0);
    CopyW(o + 408, kZeroVec + 8);
    store<u32>(o + 420, 0);
    store<u32>(o + 424, 0xBF800000u);
    store<u32>(o + 428, 0);
    store<u16>(o + 432, static_cast<u16>(h));
    const f64 k0 = LfsX(kPool);
    call(ctx, Shape_Reset, {o + 436});

    const u32 t = load<u32>(o + 216);
    CopyW(o + 424, t + 228);
    SetF1(ctx, t + 228);  // f1 as the original leaves it without a body
    store<u16>(o + 432, static_cast<u16>((load<u16>(o + 432) & ~0x80u) | ((load<u8>(t + 301) << 1) & 0x80u)));
    const u32 tb = load<u32>(t + 296);
    if (tb != 0) {
        u32 body = static_cast<u32>(VCall(ctx, tb, 32, {tb}));
        store<u32>(o + 416, body);
        const u32 no = load<u32>(o + 160);
        const u32 name = no != 0 ? static_cast<u32>(VCall(ctx, no, 12, {no})) : kUnknown;
        call(ctx, HkBody_SetName, {body + 120, name});
        for (u32 k = 0; k < 4; ++k) Row(sp + kMat + 16 * k, d + 48 + 16 * k);
        call(ctx, Hk_SetTransform, {load<u32>(o + 416), sp + kMat});

        const f64 mass = LfsX(d + 244);
        if (mass > k0) {
            body = load<u32>(o + 416);
            const u32 mt = load<u8>(body + 232);
            if ((mt == 5 || mt == 4) && HkPropIndex(body, kKeyFilter) < 0) {
                store<u32>(body + 44, 4);
                call(ctx, Hk_SetMotionFlags_0091D470, {body, 1, 1, 0});
                body = load<u32>(o + 416);
                store<u8>(body + 42, 4);
            }
            callf(ctx, Hk_SetMass_0091D618, {body}, {LfsX(d + 244)});
        } else if (mass == k0) {
            body = load<u32>(o + 416);
            const u32 mt = load<u8>(body + 232);
            if (mt != 5 && mt != 4 && HkPropIndex(body, kKeyFilter) < 0) {
                store<u32>(body + 44, 2);
                call(ctx, Hk_SetMotionFlags_0091D470, {body, 5, 1, 0});
                store<u8>(load<u32>(o + 416) + 42, 0);
            }
        }
        SetF1(ctx, kPool + 64);  // f1 as the original leaves it
        if (LfsX(d + 248) < LfsX(kPool + 64))
            store<u16>(load<u32>(o + 416) + 510, static_cast<u16>(load<u32>(d + 248) >> 16));

        body = load<u32>(o + 416);
        const u32 mt = load<u8>(body + 232);
        if (mt != 5 && mt != 4) {
            Row(sp + kVec, d + 208);
            call(ctx, Hk_Activate, {body});
            VCall(ctx, body + 224, 68, {body + 224, sp + kVec});
            Row(sp + kVec, d + 224);
            const u32 b2 = load<u32>(o + 416);
            call(ctx, Hk_Activate, {b2});
            VCall(ctx, b2 + 224, 72, {b2 + 224, sp + kVec});
            body = load<u32>(o + 416);
        }
        if (load<u8>(d + 252) != 0 && static_cast<s8>(load<u8>(body + 42)) != 2) store<u8>(body + 42, 2);
        call(ctx, Hk_AddProperty, {body, static_cast<u64>(static_cast<s64>(static_cast<s32>(kKeyA))), kValA});  // lis: sign-extended key
        call(ctx, Hk_AddProperty, {load<u32>(o + 416), kKeyObj, o});
        call(ctx, HkBody_AddListenerA82C, {load<u32>(o + 416), o + 380});
        call(ctx, HkBody_AddListenerA5F8, {load<u32>(o + 416), o + 384});
    }
    CopyW(o + 428, d + 240);
    ret(ctx, static_cast<u32>(call(ctx, Finish_001F9F70, {o, 1, 0})));
}
