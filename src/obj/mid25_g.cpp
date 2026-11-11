// Game code, medium sweep part 25: binding re-key handler of the 128-byte
// binding records (+104 array, u16 count +112) of the animated set pieces.
#include "mid25.h"

using namespace sly4;
using namespace sly4::mid25;

namespace {
constexpr u32 kAnimRestart = 0x002775A8u;    // Anim_Restart2775A8 (anim, key, 0, 0, -, -, 0, 0, -, -, 0, -1; f1..f4)
constexpr u32 kSyncBindings = 0x003BB8ACu;   // Obj_SyncBindingPoses(o, mode, skip)
constexpr u32 kCurPlayerKey = 0x002E23ACu;   // f_002E23AC(session) -> the current player key
constexpr u32 kLevelKeyByte = 0x002E1B1Cu;   // Player_LevelKeyByte_002E1B1C(-, key) -> u8
constexpr u32 kHashNoCase = 0x0056A710u;     // String_HashNoCase(s)
constexpr u32 kSession = 0x00F64C20u;        // Session*
}  // namespace

// u32 f(T* o) (called by 0x003C60C8): when the animation source o->+1588
// reports one (vt[60]), Anim_Restart2775A8(that, 0xACC3FD87, ..; f1-f3 =
// the floats at 0x003C5E2C, f4 = f3). With mode 2 (o->+20->+4):
// Obj_SyncBindingPoses(o, 2, 0); each binding whose +80 is not o->+1600
// gets key 0x32A3716C, demoted to 0x5F9B8391 when the session's player is
// the current one (f_002E23AC) and Player_LevelKeyByte_002E1B1C(session,
// hash of the binding name +108 +4) is 0, and bit 0x80 of the byte +104;
// then, when the def o->+1584 has a +216 record of kind 0x27EAE169 (+52),
// the bindings whose +80 is o->+1600 get key 0xB6162705 and bit 0x80.
// r3 = the binding count read last (mode 2), else the restart's or 0.
DECOMP(003C5E38, Obj_RekeyBindings_003C5E38) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r3 = 0;
    if (const u32 a = load<u32>(o + 1588); a != 0) {
        r3 = static_cast<u32>(VCall(ctx, a, 60, {a}));
        if (r3 != 0) {
            const u32 a2 = load<u32>(o + 1588);
            const u32 an = static_cast<u32>(VCall(ctx, a2, 60, {a2}));
            const f64 f3 = Lf(0x003C5E34u);
            // Stack arguments 9 / 10 (112 / 120) are not written by the original (not read).
            callf(ctx, kAnimRestart, {an, 0xACC3FD87u, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {Lf(0x003C5E2Cu), Lf(0x003C5E30u), f3, f3});
            r3 = static_cast<u32>(ctx.gpr[3]);
        }
    }
    if (load<u32>(load<u32>(o + 20) + 4) != 2) {
        ctx.gpr[3] = r3;
        return;
    }
    call(ctx, kSyncBindings, {o, 2, 0});
    const u32 me = load<u32>(o + 1600);
    u32 n = 0;
    for (u32 i = 0;; ++i) {
        n = load<u16>(o + 112);
        if (i >= n) break;
        const u32 e = load<u32>(o + 104) + 128 * i;
        if (load<u32>(e + 80) == me) continue;
        store<u32>(e, 0x32A3716Cu);
        const u32 s = load<u32>(kSession);
        const u32 cur = load<u32>(s + 36);
        if (cur == static_cast<u32>(call(ctx, kCurPlayerKey, {s, s}))) {
            const u32 s2 = load<u32>(kSession);
            const u32 h = static_cast<u32>(call(ctx, kHashNoCase, {load<u32>(e + 108) + 4}));
            if ((call(ctx, kLevelKeyByte, {s2, h}) & 0xFFFFFFFFu) == 0) store<u32>(e, 0x5F9B8391u);
        }
        store<u8>(e + 104, static_cast<u8>(load<u8>(e + 104) | 0x80u));
    }
    ctx.gpr[3] = n;
    const u32 def = load<u32>(o + 1584);
    if (def == 0) return;
    const u32 rec = load<u32>(def + 216);
    if ((rec != 0 ? load<u32>(rec + 52) : 0) != 0x27EAE169u) return;
    const u32 me2 = load<u32>(o + 1600);
    for (u32 j = 0; j < n; ++j) {
        const u32 e = load<u32>(o + 104) + 128 * j;
        if (load<u32>(e + 80) != me2) continue;
        store<u32>(e, 0xB6162705u);
        store<u8>(e + 104, static_cast<u8>(load<u8>(e + 104) | 0x80u));
        n = load<u16>(o + 112);
    }
    ctx.gpr[3] = n;
}
