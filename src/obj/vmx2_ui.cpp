// Game code, VMX sweep part 2 (see vmx2.h): UI element hit testing.
#include "../sly4.h"
#include "vmx2.h"

using namespace sly4;
using namespace sly4::vmx2;

namespace {
constexpr u32 kUiFindAtPoint = 0x004D3E60;  // this function (recursive)
constexpr u32 kUiHitFilter = 0x00F66874;    // OPD* : optional hit filter, called with a 20-byte query (0: accept)
constexpr u32 kUiHitFilterArg = 0x00F66878; // u32 : passed in the query
constexpr u32 kUiPerm = 0x004D3E40;         // two vperm masks for the transpose
constexpr u32 kUiVisible = 0x80000000u;     // +136 flags
constexpr u32 kUiHittable = 0x40000000u;
constexpr u32 kUiQueryHit = 3;
}  // namespace

// Element* Ui_FindAtPoint(Element* e, user, const f32 pt[2], f32* best)
// (called by Ui_Update and itself): the hittable element under pt nearest
// its own origin. Invisible elements (+136 bit 0x80000000 clear) give 0.
// A hittable one (bit 0x40000000) gets e->vfn[40](e, &min, &max, &m): pt
// (z 0) is brought into the element's frame (m transposed with vmrghw /
// vmrglw / vperm, R^T p - R^T t); inside the 2D bounds (>= min, <= max,
// unordered fails) with x^2 + y^2 + z^2 (fmadds) below *best, it becomes
// the result and *best that distance, unless the filter (kUiHitFilter's
// OPD, set) returns 0 for the query {3, user, kUiHitFilterArg, e, 0}
// (called with r4 = the entry and r5 = the OPD, as the original does).
// Then each child (+152, siblings +156) is searched; a non-zero result
// replaces the result. f1 is left as the original leaves it before the
// children (the local y, or *best's old value when all bounds passed).
DECOMP(004D3E60, Ui_FindAtPoint) {
    const u32 e = arg<u32>(ctx, 0), user = arg<u32>(ctx, 1), pt = arg<u32>(ctx, 2), best = arg<u32>(ctx, 3);
    u32 found = 0;
    const u32 fl = load<u32>(e + 136);
    if ((fl & kUiVisible) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if ((fl & kUiHittable) != 0) {
        Stack frame(ctx, 320);
        const u32 sp = frame.addr(), lp = sp + 112, m = sp + 128, mn = sp + 192, mx = sp + 200, q = sp + 208;
        const V4 px = SplatW(pt), py = SplatW(pt + 4);
        CopyW(lp, pt);
        CopyW(lp + 4, pt + 4);
        store<u32>(lp + 8, 0);
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(pt)));
        ctx.fpr[2] = std::bit_cast<f64>(LfsBits(load<u32>(pt + 4)));
        VCall(ctx, e, 160, {e, mn, mx, m});
        const V4 zero = Zero4();
        const V4 nt = Sub4(zero, Lvx(m + 48));
        const V4 r0 = Lvx(m), r1 = Lvx(m + 16), r2 = Lvx(m + 32);
        const V4 lo = Mrghw(r0, r2);
        const V4 c0 = Mrghw(lo, r1);
        const V4 c1 = Perm(lo, r1, Lvx(kUiPerm));
        const V4 c2 = Perm(Mrglw(r0, r2), r1, Lvx(kUiPerm + 16));
        Stvx(m, c0);
        Stvx(m + 16, c1);
        Stvx(m + 32, c2);
        const V4 a = Madd4(c2, zero, Madd4(c1, py, Madd4(c0, px, zero)));
        const V4 b = Madd4(c2, Splat(nt, 2), Madd4(c1, Splat(nt, 1), Madd4(c0, Splat(nt, 0), zero)));
        Stvx(m + 48, b);
        Stvx(lp, Add4(b, a));
        const f32 x = F(lp), y = F(lp + 4), z = F(lp + 8);
        const f32 d = Fmadds(z, z, Fmadds(x, x, Muls(y, y)));
        ctx.fpr[1] = y;
        if (x >= F(mn) && x <= F(mx) && y >= F(mn + 4) && y <= F(mx + 4)) {
            ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(best)));
            if (d < F(best)) {
                const u32 filt = load<u32>(kUiHitFilter);
                bool ok = true;
                if (filt != 0) {
                    store<u32>(q, kUiQueryHit);
                    store<u32>(q + 4, user);
                    store<u32>(q + 8, load<u32>(kUiHitFilterArg));
                    store<u32>(q + 12, e);
                    store<u32>(q + 16, found);
                    ok = static_cast<u32>(call_ptr(ctx, load<u32>(kUiHitFilter), {q, load<u32>(load<u32>(kUiHitFilter)), load<u32>(kUiHitFilter)})) != 0;
                }
                if (ok) {
                    found = e;
                    store<f32>(best, d);
                }
            }
        }
    }
    for (u32 c = load<u32>(e + 152); c != 0; c = load<u32>(c + 156)) {
        const u32 r = static_cast<u32>(call(ctx, kUiFindAtPoint, {c, user, pt, best}));
        if (r != 0) found = r;
    }
    ctx.gpr[3] = found;
}
