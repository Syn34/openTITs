// Game code, large-function sweep part 34: the script action that starts a
// camera shake from its node's parameters.
#include "native/objtypes.h"

namespace sly4::obj {

struct ShakeDesc {
    be_u32 w[21];  // +0 .. +80 (see Action_CamShake_003958C8)
};
struct ShakeCtx {
    pad<12> _0;
    be_u32 pos_obj;  // +12: object with its position at +144
};

namespace {
constexpr u32 kEffects = 0x00F64C38;      // u32: the effects system (Effects_Construct)
constexpr u32 CamShake_Add = 0x002F2140;  // (effects, const ShakeDesc*)
constexpr u32 kOne = 0x3F800000, kFour = 0x40800000, kSixAndHalf = 0x40D00000, kTwoAndHalf = 0x40200000;
constexpr u32 kMinusOne = 0xBF800000;
constexpr u32 kShakeHash = 0x85D98325;  // the default kind (+16), replaced by parameter 3
}  // namespace

// u32 Action_CamShake(Node* n, Ctx* c) (called through a pointer).
// Fills a shake descriptor on the stack, words (offset: value): 0: 0,
// 4: param 1, 8: param 2, 12: 0, 16: param 3 (the kind hash), 20..48:
// params 4..10, 48: param 0, 64..72: c->12's +144 position (a zero vector
// without one), 80: param 11; +52..60 and +76 are left unset as in the
// original. Each parameter word is copied raw (NodeParamAddr). Then
// CamShake_Add(*kEffects, &desc); returns 0.
u32 Action_CamShake_003958C8(u32 n, const ShakeCtx* c) {
    GuestFrame frame(96);
    ShakeDesc* d = frame.as<ShakeDesc>();
    auto w = [&](u32 off) -> be_u32& { return d->w[off / 4]; };
    // the defaults, most of them overwritten below
    w(0) = 0u;
    w(4) = kOne;
    w(8) = kOne;
    w(12) = 0u;
    w(16) = kShakeHash;
    w(20) = 0u;
    w(24) = kOne;
    w(28) = 0u;
    w(32) = kOne;
    w(36) = kFour;
    w(40) = kSixAndHalf;
    w(44) = kTwoAndHalf;
    w(48) = 0u;
    const Vec3& z = global<Vec3>(kZeroVec);
    w(64) = bits(z.x);
    w(68) = bits(z.y);
    w(72) = bits(z.z);
    w(80) = kMinusOne;

    const u32 pos = c->pos_obj;
    auto param = [&](u32 off, u32 k) { w(off) = gm<be_u32>(NodeParamAddr(n, k)).get(); };
    param(4, 1);
    param(8, 2);
    param(16, 3);
    for (u32 k = 4; k <= 10; ++k) param(20 + 4 * (k - 4), k);
    param(48, 0);
    param(80, 11);
    if (pos != 0)
        for (u32 k = 0; k < 12; k += 4) w(64 + k) = gm<be_u32>(pos + 144 + k).get();
    call(CamShake_Add, global<be_u32>(kEffects).get(), d);
    return 0;
}

}  // namespace sly4::obj

NATIVE(003958C8, sly4::obj::Action_CamShake_003958C8)
