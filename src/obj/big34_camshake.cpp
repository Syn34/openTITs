// Game code, large-function sweep part 34: the script action that starts a
// camera shake from its node's parameters.
#include "../sly4.h"
#include "big34.h"

using namespace sly4;
using namespace sly4::big34;

namespace {
constexpr u32 kEffects = 0x00F64C38;      // u32: the effects system (Effects_Construct)
constexpr u32 kZeroVec = 0x01164B40;      // f32[3] {0, 0, 0}
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
DECOMP(003958C8, Action_CamShake_003958C8) {
    const u32 n = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    Stack frame(ctx, 96);  // the descriptor (sp+112..+196 in the original)
    const u32 d = frame.addr();
    // the defaults, most of them overwritten below
    store<u32>(d + 0, 0);
    store<u32>(d + 4, kOne);
    store<u32>(d + 8, kOne);
    store<u32>(d + 12, 0);
    store<u32>(d + 16, kShakeHash);
    store<u32>(d + 20, 0);
    store<u32>(d + 24, kOne);
    store<u32>(d + 28, 0);
    store<u32>(d + 32, kOne);
    store<u32>(d + 36, kFour);
    store<u32>(d + 40, kSixAndHalf);
    store<u32>(d + 44, kTwoAndHalf);
    store<u32>(d + 48, 0);
    for (u32 k = 0; k < 12; k += 4) store<u32>(d + 64 + k, load<u32>(kZeroVec + k));
    store<u32>(d + 80, kMinusOne);

    const u32 pos = load<u32>(c + 12);
    auto param = [&](u32 off, u32 k) { store<u32>(d + off, load<u32>(NodeParamAddr(n, k))); };
    param(4, 1);
    param(8, 2);
    param(16, 3);
    for (u32 k = 4; k <= 10; ++k) param(20 + 4 * (k - 4), k);
    param(48, 0);
    param(80, 11);
    if (pos != 0)
        for (u32 k = 0; k < 12; k += 4) store<u32>(d + 64 + k, load<u32>(pos + 144 + k));
    // f1 enters the call as the last float the original loaded (not read by it)
    ctx.fpr[1] = Lfs(pos != 0 ? pos + 152 : NodeParamAddr(n, 11));
    call(ctx, CamShake_Add, {load<u32>(kEffects), d});
    ret(ctx, 0u);
}
