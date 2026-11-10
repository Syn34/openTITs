// Game code, large-function sweep part 3: the bone-binding pose sync
// 003BB8AC (9 callers, all in the 003BC0B8..003C6EA8 character code).
#include "big3.h"

using namespace sly4;
using namespace sly4::big3;

namespace b3o {
constexpr u32 Quat_FromMatrix = 0x00555478;           // (f32 out[4], const f32 m[4][4])
constexpr u32 SlotRec_Flag40 = 0x002DA89C;            // (o) -> bool
constexpr u32 kIdentity = 0x01164BD0;                 // f32[4][4]
constexpr u32 kGlobal_F64C20 = 0x00F64C20;            // u32: an object whose +36 is checked with SlotRec_Flag40
constexpr u32 kBindingBytes = 128;
}  // namespace b3o

namespace {

// The 4x3 matrix (rows of three f32 at a 16-byte stride) at src copied
// into dst; returns the last word copied (the original leaves it in f1).
u32 CopyM43(u32 dst, u32 src) {
    u32 w = 0;
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) store<u32>(dst + 16 * r + 4 * c, w = load<u32>(src + 16 * r + 4 * c));
    return w;
}
// A matrix holder's (+16) 4x3 matrix copied into dst.
inline void CopyHolder(u32 dst, u32 holder) { CopyM43(dst, holder + 16); }

// Whether the global object passes: absent, or SlotRec_Flag40 is set.
bool GlobalFlag(PPUContext& ctx) {
    const u32 g = load<u32>(load<u32>(b3o::kGlobal_F64C20) + 36);
    return g == 0 || static_cast<u32>(call(ctx, b3o::SlotRec_Flag40, {g})) != 0;
}

}  // namespace

// void Obj_SyncBindingPoses(Obj* o, s32 mode, Node* skip): for each
// 128-byte binding e of o's array (+104, u16 count +112) whose node
// (e+80) is set and is not `skip`, a source matrix M is picked by `mode`
// (identity by default; the node's own +96 matrix, or one of the
// binding's matrix holders +84 / +88 / +92 / +96 / +100, depending on
// o's three special nodes +1600 / +1604 / +1608, o->+1544 == 3 and the
// o->+1560 bit 0x01000000), then: e+16 = the node matrix's quaternion,
// e+48 = the node's +144 position, e+32 = M's quaternion, e+64 = M's
// translation row, and o->+1552 = *o->+60. Returns the u16 count in r3
// and the last *o->+60 in f1, as the original leaves them.
DECOMP(003BB8AC, Obj_SyncBindingPoses) {
    using namespace b3o;
    const u32 o = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 1);
    const u32 skip = arg<u32>(ctx, 2);
    Stack frame(ctx, 80);
    const u32 quat = frame.addr(), m = frame.addr() + 16;  // the original's r1+112 / r1+128
    for (u32 i = 0; i < load<u16>(o + 112); ++i) {
        const u32 e = load<u32>(o + 104) + kBindingBytes * i;
        const u32 node = load<u32>(e + 80);
        if (node == 0 || node == skip) continue;
        CopyM43(m, kIdentity);
        const u32 nm = node + 96;
        const auto special = [&] { return node == load<u32>(o + 1608) || node == load<u32>(o + 1600); };
        const auto bit = [&] { return (load<u32>(o + 1560) & 0x01000000u) != 0; };
        if (mode == 4) {
            if (const u32 h = load<u32>(e + 100)) CopyHolder(m, h);
        } else if (mode == 3) {
            const bool g = GlobalFlag(ctx);
            if (node == load<u32>(o + 1604) && g) CopyM43(m, nm);
            else if (special() && !bit()) CopyM43(m, nm);
            else if (const u32 h = load<u32>(e + 96)) CopyHolder(m, h);
        } else if (mode == 2) {
            const bool g = GlobalFlag(ctx);
            if (node == load<u32>(o + 1604) && !g) {
                if (load<u32>(o + 1544) == 3) CopyM43(m, nm);
                else if (const u32 h = load<u32>(e + 84)) CopyHolder(m, h);
            } else if (const u32 h = load<u32>(e + 92)) {
                // (+96 unchecked, as the original)
                if (load<u32>(o + 1544) == 3 && special() && bit()) CopyHolder(m, load<u32>(e + 96));
                else CopyHolder(m, h);
            }
        } else if (mode == 1) {
            if (const u32 h = load<u32>(e + 88)) CopyHolder(m, h);
        } else if (mode == 0) {
            if (const u32 h = load<u32>(e + 84)) CopyHolder(m, h);
        }
        call(ctx, Quat_FromMatrix, {quat, nm});
        for (u32 k = 0; k < 16; k += 4) store<u32>(e + 16 + k, load<u32>(quat + k));
        for (u32 k = 0; k < 12; k += 4) store<u32>(e + 48 + k, load<u32>(node + 144 + k));
        call(ctx, Quat_FromMatrix, {quat, m});
        for (u32 k = 0; k < 16; k += 4) store<u32>(e + 32 + k, load<u32>(quat + k));
        for (u32 k = 0; k < 12; k += 4) store<u32>(e + 64 + k, load<u32>(m + 48 + k));
        const u32 w = load<u32>(load<u32>(o + 60));
        store<u32>(o + 1552, w);
        ctx.set_fpr_bits(1, LfsBits(w));
    }
    ret(ctx, static_cast<u32>(load<u16>(o + 112)));
}
