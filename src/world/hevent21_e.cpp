// World_HandleEvent callees, part 21e: the coach pause toggle and the
// online save block byte swap.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kEffectBind = 0x004A6558;      // (Effect** slot, name, set name, owner, value, u8) (symbols.txt Effect_Bind4A6558)
constexpr u32 kWorldObjForEach = 0x0057F5D8; // (obj, fn OPD, arg, bool recurse) (as hevent9.h)
constexpr u32 kPauseVisitOpd = 0x00F35F50;   // WorldObj_ForEach callback OPD: {u8 paused, Coach*} (name?)
constexpr u32 kEmptyName = 0x00AD4B7C;       // ""

// The 0/1 value the original makes of a bit with cntlzw / xori.
inline u32 nz(u32 v) { return v != 0 ? 1u : 0u; }

inline void vnotify(PPUContext& ctx, u32 x) { vcall(ctx, x, 40, {x, 0, 1}); }

// Byte swaps in place (byte loads, one store).
inline void sw16(u32 a) { store<u16>(a, static_cast<u16>((load<u8>(a + 1) << 8) | load<u8>(a))); }
inline void sw32(u32 a) {
    store<u32>(a, (static_cast<u32>(load<u8>(a + 3)) << 24) | (static_cast<u32>(load<u8>(a + 2)) << 16) |
                      (static_cast<u32>(load<u8>(a + 1)) << 8) | load<u8>(a));
}
inline void sw64(u32 a) {
    u64 v = 0;
    for (u32 k = 0; k < 8; ++k) v |= static_cast<u64>(load<u8>(a + k)) << (8 * k);
    store<u64>(a, v);
}
}  // namespace

// u32 Coach_Pause2AAEBC(Coach* c, u8 paused) (name?): nested pause count
// +2324. Pausing from 0: the animator +184 (pause count +268 ++, vt +88
// (a, 0)); the owner +48 records its +164 bit 0x8000 in +244 bit 0x40
// and, when set, has its world objects visited (WorldObj_ForEach(owner,
// 0x00F35F50, {1, c}, 0)) and gets vt +40 (o, 0, 1); the effect +192 is
// rebound (Effect_Bind4A6558(+192, "", "", owner, +52, 1)); then the
// objects at +2588 (9), +2624 / +2672 / +2720 (12 each, interleaved),
// +2768 (32) and +2896..+2904 get vt +40 (x, 0, 1). The count goes up in
// any case. Resuming takes the count down; on reaching 0 the animator's
// count (if positive) goes down and it gets vt +88 (a, bit 0x40), and an
// owner that was paused has its objects visited with {0, c}, u16 +26 = 0
// and vt +40 (o, bit 0x40, 0). r3 is left as the original leaves it.
DECOMP(002AAEBC, Coach_Pause2AAEBC) {
    const u32 c = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 st = frame.addr();
    if ((arg<u32>(ctx, 1) & 0xFF) != 0) {
        if (load<u32>(c + 2324) == 0) {
            const u32 a = load<u32>(c + 184);
            if (a != 0) {
                store<u32>(a + 268, load<u32>(a + 268) + 1);
                vcall(ctx, a, 88, {a, 0});
            }
            u32 owner = load<u32>(c + 48);
            if (owner != 0) {
                const u32 bit = (load<u32>(owner + 164) >> 15) & 1u;
                store<u32>(c + 244, (load<u32>(c + 244) & ~0x40u) | (nz(bit) << 6));
                if (bit != 0) {
                    store<u8>(st + 8, 1);
                    store<u32>(st + 12, c);
                    call(ctx, kWorldObjForEach, {owner, kPauseVisitOpd, st + 8, 0});
                    const u32 o = load<u32>(c + 48);
                    vcall(ctx, o, 40, {o, 0, 1});
                    owner = load<u32>(c + 48);
                }
            }
            call(ctx, kEffectBind, {c + 192, kEmptyName, kEmptyName, owner, load<u32>(c + 52), 1});
            for (u32 k = 0; k < 9; ++k) {
                const u32 x = load<u32>(c + 2588 + 4 * k);
                if (x != 0) vnotify(ctx, x);
            }
            for (u32 k = 0; k < 12; ++k) {
                for (u32 base : {2624u, 2672u, 2720u}) {
                    const u32 x = load<u32>(c + base + 4 * k);
                    if (x != 0) vnotify(ctx, x);
                }
            }
            for (u32 k = 0; k < 32; ++k) {
                const u32 x = load<u32>(c + 2768 + 4 * k);
                if (x != 0) vnotify(ctx, x);
            }
            for (u32 off : {2896u, 2900u, 2904u}) {
                const u32 x = load<u32>(c + off);
                if (x != 0) vnotify(ctx, x);
            }
        }
        const u32 n = load<u32>(c + 2324) + 1;
        store<u32>(c + 2324, n);
        ret(ctx, n);
        return;
    }
    u32 n = load<u32>(c + 2324);
    if (!(static_cast<s32>(n) > 0)) {
        ret(ctx, n);
        return;
    }
    n -= 1;
    store<u32>(c + 2324, n);
    if (n != 0) {
        ret(ctx, n);
        return;
    }
    const u32 a = load<u32>(c + 184);
    if (a != 0) {
        const u32 pc = load<u32>(a + 268);
        if (static_cast<s32>(pc) > 0) store<u32>(a + 268, pc - 1);
        vcall(ctx, a, 88, {a, nz((load<u32>(c + 244) >> 6) & 1u)});
    }
    const u32 owner = load<u32>(c + 48);
    ret(ctx, owner);
    if (owner == 0) return;
    if (((load<u32>(c + 244) >> 6) & 1u) == 0) return;
    store<u8>(st, 0);
    store<u32>(st + 4, c);
    call(ctx, kWorldObjForEach, {owner, kPauseVisitOpd, st, 0});
    const u32 o = load<u32>(c + 48);
    const u32 bit = (load<u32>(c + 244) >> 6) & 1u;
    store<u16>(c + 26, 0);
    vcall(ctx, o, 40, {o, nz(bit), 0});
}

// u8* SaveBlock_Swap0AF8E4(u8* b) (name?): byte swaps the online save block
// in place, field by field in the original order: u16 +2 / +4; u32 +8,
// +32..+48; 12 x {u32 +52, u16 +100, u16 +124, u16 +416 (stride 4)}; u32
// +160, +292..+308; u64 +312; u16 +320 / +322; u32 +332, +340, +348; 8 x
// u32 pairs +352; 64 x 5 u32 +608 (44-byte records); 64 u32 +3424 (stride
// 8); u32 +3936..+3944; u64 +4048. Returns b.
DECOMP(000AF8E4, SaveBlock_Swap0AF8E4) {
    const u32 b = arg<u32>(ctx, 0);
    sw16(b + 2);
    sw16(b + 4);
    sw32(b + 8);
    for (u32 off = 32; off <= 48; off += 4) sw32(b + off);
    for (u32 k = 0; k < 12; ++k) {
        sw32(b + 52 + 4 * k);
        sw16(b + 100 + 2 * k);
        sw16(b + 124 + 2 * k);
        sw16(b + 416 + 4 * k);
    }
    sw32(b + 160);
    for (u32 off = 292; off <= 308; off += 4) sw32(b + off);
    sw64(b + 312);
    sw16(b + 320);
    sw16(b + 322);
    sw32(b + 332);
    sw32(b + 340);
    sw32(b + 348);
    for (u32 k = 0; k < 8; ++k) {
        sw32(b + 352 + 8 * k);
        sw32(b + 356 + 8 * k);
    }
    for (u32 j = 0; j < 64; ++j)
        for (u32 i = 0; i < 5; ++i) sw32(b + 608 + 44 * j + 4 * i);
    for (u32 k = 0; k < 64; ++k) sw32(b + 3424 + 8 * k);
    sw32(b + 3936);
    sw32(b + 3940);
    sw32(b + 3944);
    sw64(b + 4048);
}
