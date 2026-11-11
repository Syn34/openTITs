// Game code, medium sweep part 24: episode progress marking and a
// hierarchical virtual forwarder.
#include "mid24.h"

using namespace sly4;
using namespace sly4::mid24;

namespace {
constexpr u32 kStringFormat = 0x005534D0u;  // String_Format(fmt, ...)
constexpr u32 kWorld = 0x00F65CA0u;         // World* g_World
}  // namespace

// u32 Episode_MarkKey2E2B34(Progress* s, u32 key): the episode e = s->36,
// or else the first of the s->24 860-byte episodes s->20 (not u8 +826) whose
// u8 +820 words at +748 hold *(g_World + 0x128114); none: done. An index u8
// e->822 of 12 or more: String_Format("Episode Index is out of bounds '%i'!
// Range is [0 - %i]! ...", idx, 12). For a non-zero key found in the map
// 0x010681CC (SortedMap_Find76673C) with entry v: the byte g_World + 228 +
// 4056 * u8 (g_World + 4208) + idx |= 1 << u8 *v; then when the bits of
// that row over all episodes' indices add up to exactly one,
// Trophy_Award0B27B4(0xC3FB61E2). r3 as the original leaves it.
DECOMP(002E2B34, Episode_MarkKey2E2B34) {
    const u32 s = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 kp = frame.addr();
    store<u32>(kp, key);
    u32 e = load<u32>(s + 36);
    if (e == 0) {
        const u32 want = load<u32>(load<u32>(kWorld) + 0x128114u);
        const u32 n = load<u32>(s + 24);
        ctx.gpr[3] = want;
        u32 ep = load<u32>(s + 20);
        bool found = false;
        for (u32 i = 0; i < n && !found; ++i, ep += 860) {
            if (load<u8>(ep + 826) != 0) continue;
            const u32 m = load<u8>(ep + 820);
            for (u32 j = 0; j < m; ++j)
                if (load<u32>(ep + 748 + 4 * j) == want) {
                    e = ep;
                    found = true;
                    break;
                }
        }
        if (e == 0) return;  // (also for a match at a null episode, as the original)
    }
    const u32 idx = load<u8>(e + 822);
    if (idx >= 12) {
        call(ctx, kStringFormat, {0x00B1D308u, idx, 12});
        return;
    }
    ctx.gpr[3] = idx;
    if (key == 0) return;
    const u32 v = load<u32>(static_cast<u32>(call(ctx, 0x0076673Cu, {0x010681CCu, kp})));
    ctx.gpr[3] = v;
    if (v == 0) return;
    {
        const u32 g = load<u32>(kWorld);
        const u32 sh = load<u8>(v) & 0x3Fu;
        const u32 bit = sh < 32 ? (1u << sh) : 0u;  // slw
        const u32 ea = g + load<u8>(g + 4208) * 4056u + load<u8>(e + 822) + 228;
        store<u8>(ea, static_cast<u8>(load<u8>(ea) | bit));
    }
    const u32 g = load<u32>(kWorld);
    const u32 row = g + load<u8>(g + 4208) * 4056u + 228;
    const u32 n = load<u32>(s + 24);
    ctx.gpr[3] = 0;
    if (n == 0) return;
    u32 cnt = 0;
    u32 ep = load<u32>(s + 20);
    for (u32 i = 0; i < n; ++i, ep += 860) {
        u32 b = load<u8>(row + load<u8>(ep + 822));
        while (b != 0) {
            b &= b - 1;
            ++cnt;
        }
    }
    ctx.gpr[3] = n;
    if (cnt == 1) call(ctx, 0x000B27B4u, {0xC3FB61E2u});
}

// u32 Node_ForwardNamed0DC0F8(Node* o, const char* a, b, u8 c, -, -, d,
// const char* pfx, -, e; f1, f2, f3): for each 240-byte record r of the
// list o->340 (u16 count +8) whose +176 has bit 31: x = r->192,
// x->vfn[204](x, a, b, c, -, -, d, String_Format("%s_", r), -, e; f1, f2,
// *0x000DC0F4). Then n = pfx ? String_Format("%s%s", pfx, a) : a, and r3 =
// o->vfn[196](o, o->vfn[192](o, n), b, c, -, -, d, -, e; f1, f2, f3). The
// skipped register / stack slots ("-": the OPD and its entry in r7 / r8,
// r10 and a stack slot the original leaves as they are) get the OPD, its
// entry and 0.
DECOMP(000DC0F8, Node_ForwardNamed0DC0F8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3) & 0xFFu, d = arg<u32>(ctx, 6), pfx = arg<u32>(ctx, 7);
    const u32 e = load<u32>(static_cast<u32>(ctx.gpr[1]) + 124);  // 10th argument (low word of 120(r1))
    const f64 f1 = ctx.fpr[1], f2 = ctx.fpr[2], f3 = ctx.fpr[3];
    const f64 k = Lf(0x000DC0F4u);
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(o + 340);
        if (i >= (l != 0 ? static_cast<u32>(load<u16>(l + 8)) : 0u)) break;
        const u32 r = load<u32>(l) + 240 * i;
        if ((load<u32>(r + 176) & 0x80000000u) == 0) continue;
        const u32 x = load<u32>(r + 192);
        const u32 nm = static_cast<u32>(call(ctx, kStringFormat, {0x00AC6B9Cu, r}));
        const u32 opd = load<u32>(load<u32>(x) + 204);
        ctx.fpr[1] = f1;
        ctx.fpr[2] = f2;
        ctx.fpr[3] = k;
        call_ptr(ctx, opd, {x, a, b, c, opd, load<u32>(opd), d, nm, 0, e});
    }
    const u32 n = pfx != 0 ? static_cast<u32>(call(ctx, kStringFormat, {0x00AC6BA4u, pfx, a})) : a;
    const u32 r = static_cast<u32>(VCall(ctx, o, 192, {o, n}));
    const u32 opd = load<u32>(load<u32>(o) + 196);
    ctx.fpr[1] = f1;
    ctx.fpr[2] = f2;
    ctx.fpr[3] = f3;
    // r10 as the vfn[192] call left it.
    call_ptr(ctx, opd, {o, r, b, c, opd, load<u32>(opd), d, ctx.gpr[10], e});
}
