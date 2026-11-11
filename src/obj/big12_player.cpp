// Game code, large-function sweep part 12: the player's setup and rate
// helpers (callees of Player_ResetFromDef238270 and friends).
#include "big12.h"

using namespace sly4;
using namespace sly4::big12;

namespace {
constexpr s32 kTlsRouter = -28332;   // r13 +: hkMemoryRouter* (+44 the allocator, vt +8 alloc(a, size))
}  // namespace

// void Player_Setup232F00(Player* p, Rig* q) (q = p's +612 rig; name?):
// pushes the tuning block at p->396 into the rig's parts: 0x00232B60(p,
// world tier table +4, q->28) and its +1248 (3 floats, flag byte +12) to
// q->32; 0x002329DC(p, q->28, q->36); +1272 (4 + 6 floats) to q->40;
// 0x002327A8 / 0x00232650 / 0x00232360 with q->44 / +48 / +52; +1424 (4
// floats) and the vector {+1440, +1444, +1448, 0} to q->56; +1488 (3
// floats) to q->68 and (p->616 << 16) | 13 to q->60 +12. The old q->64 is
// released (vfunc +4) and replaced by a new 28-byte shape (0x009FE8B0(s,
// q->28, +1508)) holding +1500 / +1504 at +8 / +12.
DECOMP(00232F00, Player_Setup232F00) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 q = arg<u32>(ctx, 1);
    const u32 tier = load<u32>(load<u32>(load<u32>(kWorld) + 0x1281D4u) + 4);
    call(ctx, 0x00232B60u, {p, tier, load<u32>(q + 28)});

    u32 s = load<u32>(p + 396) + 1248;
    u32 d = load<u32>(q + 32);
    const u8 flag = load<u8>(s + 12);
    CopyW(d + 8, s + 0);
    CopyW(d + 12, s + 4);
    const u32 w8 = load<u32>(s + 8);
    store<u8>(d + 20, flag);
    store<u32>(d + 16, w8);
    call(ctx, 0x002329DCu, {p, load<u32>(q + 28), load<u32>(q + 36)});

    s = load<u32>(p + 396) + 1272;
    d = load<u32>(q + 40);
    for (u32 k = 0; k < 4; ++k) CopyW(d + 8 + 4 * k, s + 4 * k);
    for (u32 k = 0; k < 4; ++k) CopyW(d + 24 + 4 * k, s + 28 + 4 * k);
    const u32 w44 = load<u32>(s + 44);
    const u32 a28 = load<u32>(q + 28);
    store<u32>(d + 40, w44);
    const u32 w48 = load<u32>(s + 48);
    const u32 a44 = load<u32>(q + 44);
    store<u32>(d + 44, w48);
    call(ctx, 0x002327A8u, {p, a28, a44});
    call(ctx, 0x00232650u, {p, load<u32>(q + 28), load<u32>(q + 48)});
    call(ctx, 0x00232360u, {p, load<u32>(q + 28), load<u32>(q + 52)});

    const u32 t = load<u32>(p + 396);
    d = load<u32>(q + 56);
    for (u32 k = 0; k < 4; ++k) CopyW(d + 8 + 4 * k, t + 1424 + 4 * k);
    // lvlx / vspltw of +1440 / +1444 / +1448, merged by the .text masks at
    // 0x00232ED0 / EE0 (vsel) and EF0 (vandc): {x, y, z, 0}.
    const u32 v0 = load<u32>(t + 1440), v1 = load<u32>(t + 1444), v2 = load<u32>(t + 1448);
    const u32 r29 = t + 1500;
    const u32 d68 = load<u32>(q + 68);
    const u32 pid = load<u32>(p + 616);
    const u32 d60 = load<u32>(q + 60);
    const u32 vq = (d + 32) & ~15u;
    store<u32>(vq + 0, v0);
    store<u32>(vq + 4, v1);
    store<u32>(vq + 8, v2);
    store<u32>(vq + 12, 0);
    CopyW(d68 + 8, t + 1488);
    store<u32>(d60 + 12, (pid << 16) | 0xDu);
    CopyW(d68 + 12, t + 1492);
    const u32 old = load<u32>(q + 64);
    CopyW(d68 + 16, t + 1496);
    if (old != 0) {
        VCall(ctx, old, 4, {old});
        store<u32>(q + 64, 0);
    }
    const u32 al = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(kTlsRouter)) + 44);
    const u32 shape = static_cast<u32>(VCall(ctx, al, 8, {al, 28}));
    // Quirk: the size is stored at +4 before the null test.
    store<u16>(shape + 4, 28);
    u32 r3 = 0;
    if (shape != 0) {
        call(ctx, 0x009FE8B0u, {shape, load<u32>(q + 28), Sx(load<u32>(r29 + 8))});
        r3 = shape;
    }
    store<u32>(q + 64, r3);
    // Unchecked: a failed allocation writes at 8 / 12.
    const u32 u = load<u32>(p + 396) + 1500;
    CopyW(r3 + 8, u);
    const u32 w = load<u32>(u + 4);
    store<u32>(r3 + 12, w);
    ctx.gpr[3] = r3;
    SetF1Bits(ctx, w);   // f1 as the last lfs leaves it
}

// void Player_DropLinks2349E0(Player* p) (name?): for each 128-byte link
// entry (p->676, u16 count p->684): releases (vfunc +8) and clears +100 and
// the +8 object of every 20-byte element of its five arrays ({+0 data, +8
// u16 count} ... {+80, +88}), zeroing each count; its +104 skeleton is
// detached (0x009334A0(scratch, world tier, s) when both its +20 / +24
// parts have a +8) and released, its +108 one removed from the physics
// world (when its +8 is set) and released, and +120 freed. Then the
// counts are zeroed and the p->696 objects (u16 count p->704) released.
DECOMP(002349E0, Player_DropLinks2349E0) {
    const u32 p = arg<u32>(ctx, 0);
    constexpr u32 kTierTable = 0x1281D4u;   // world +: tier block (+4 the physics world)
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();            // the original's sp + 112 (two byte-sized scratch outs)
    const u32 hkw = load<u32>(load<u32>(load<u32>(kWorld) + kTierTable) + 4);
    auto release = [&](u32 slot) {
        const u32 x = load<u32>(slot);
        if (x == 0) return;
        VCall(ctx, x, 8, {x});
        store<u32>(slot, 0);
    };
    for (u32 i = 0; i < load<u16>(p + 684); ++i) {
        const u32 e = load<u32>(p + 676) + 128 * i;
        release(e + 100);
        for (u32 a = 0; a < 5; ++a) {
            for (u32 j = 0; j < load<u16>(e + 20 * a + 8); ++j) release(load<u32>(e + 20 * a) + 20 * j + 8);
            store<u16>(e + 20 * a + 8, 0);
        }
        const u32 tier = load<u32>(load<u32>(load<u32>(kWorld) + kTierTable) + 4);
        u32 s = load<u32>(e + 104);
        if (s != 0) {
            if (load<u32>(load<u32>(s + 20) + 8) != 0 && load<u32>(load<u32>(s + 24) + 8) != 0)
                call(ctx, 0x009334A0u, {sp + 1, tier, s});
            call(ctx, 0x007D4EA8u, {s});   // AnimSkel_Release7D4EA8
        }
        s = load<u32>(e + 108);
        store<u32>(e + 104, 0);
        if (s != 0) {
            if (load<u32>(s + 8) != 0) call(ctx, 0x0092FFC8u, {sp, hkw, s});   // Hk_RemoveEntity92FFC8
            call(ctx, 0x007D4EA8u, {s});
        }
        const u32 m = load<u32>(e + 120);
        store<u32>(e + 108, 0);
        call(ctx, 0x0054EE6Cu, {m, 0});   // Mem_FreeFrom
        store<u32>(e + 120, 0);
    }
    store<u16>(p + 684, 0);
    u32 n;
    for (u32 i = 0;; ++i) {
        n = load<u16>(p + 704);
        if (i >= n) break;
        const u32 x = load<u32>(load<u32>(p + 696) + 4 * i);
        if (x == 0) continue;
        VCall(ctx, x, 8, {x});
        store<u32>(load<u32>(p + 696) + 4 * i, 0);
    }
    store<u16>(p + 704, 0);
    ctx.gpr[3] = n;
}

namespace {
constexpr u32 kVecConstruct = 0x005516E0u;   // PtrVector_Construct (v)
constexpr u32 kVecReserve = 0x00551718u;     // PtrVector_Reserve (v, capacity)
constexpr u32 kVecClear = 0x0054EC1Cu;       // PtrVector_Clear (v)
constexpr u32 kNop = 0x0054EC18u;            // Nop (p)
constexpr u32 kMemAlloc = 0x0054ED38u;       // Mem_Alloc (size, pool, align)
constexpr u32 kWorldObjInstance = 0x005A05E8u;  // WorldObj_Instance (def, 0) -> instance
constexpr u32 kFindBone = 0x000DA9D0u;       // Obj_FindBone_000DA9D0 (o, u32 name) -> bone or 0
constexpr u32 kLinkPart = 0x00231208u;       // (Player*, PtrVector* part, const def* src) (name?)

// PushBack of a u32 PtrVector at v ({+0 data, +4 u16 capacity, +8 u16
// count, +10 u16 high-water mark}): reserved to (capacity * 2) & 0x1FFFE
// when full; returns the new mark (the r3 the original leaves).
u32 PushU32(PPUContext& ctx, u32 v, u32 value) {
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, kVecReserve, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + 8);
    }
    store<u32>(load<u32>(v) + ((n << 2) & 0x3FFFCu), value);
    const u32 c = load<u16>(v + 8) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(c));
    const u32 m = static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFF) ? hw : c;
    store<u16>(v + 10, static_cast<u16>(m));
    return m;
}
}  // namespace

// void Player_Setup231470(Player* p) (name?): builds the player's link
// entries from the tuning block (p->396 + 1072: +132 groups of 32 bytes,
// u16 count +140). Per group item (352 bytes; group +0 data, u16 +8 count)
// a 128-byte entry {5 PtrVectors of 20-byte elements, +100..+120 0, +124
// 3000.0f} is built on the stack and pushed to p->676 (u16 count +684,
// capacity +680, mark +686), then filled: +100 = WorldObj_Instance(item
// +84) when set, +112 / +116 the bones item +32 / +36, +120 a 64-byte
// block when either bone exists, the five parts from item +240.. (+20
// each, 0x00231208), +124 = (tune +1324 * K0) * (1 / (item +96 * 2pi)) *
// 2pi. Each group's +24 instance (or 0) is pushed to p->696.
DECOMP(00231470, Player_Setup231470) {
    const u32 p = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x00231464u;   // .text f32: 0.447 (K0), 2pi, 1
    const u32 t = load<u32>(p + 396) + 1072;
    const u32 ngroups = load<u16>(t + 140);
    u32 r3 = load<u32>(p + 396);
    if (ngroups != 0) r3 = static_cast<u32>(call(ctx, kVecReserve, {p + 696, ngroups}));
    Stack frame(ctx, 128);
    const u32 tmp = frame.addr();   // the original's sp + 112
    for (u32 g = 0; g < ngroups; ++g) {
        const u32 grp = load<u32>(t + 132) + 32 * g;
        const u32 nitems = load<u16>(grp + 8);
        for (u32 i = 0; i < nitems; ++i) {
            const u32 item = load<u32>(grp) + 352 * i;
            for (u32 k = 0; k < 5; ++k) {
                call(ctx, kVecConstruct, {tmp + 20 * k});
                store<u16>(tmp + 20 * k + 6, 20);
            }
            for (u32 k = 0; k < 6; ++k) store<u32>(tmp + 100 + 4 * k, 0);
            store<u32>(tmp + 124, 0x453B8000u);   // 3000.0f
            u32 n = load<u16>(p + 684);
            const u32 cap = load<u16>(p + 680);
            if (n == cap) {
                call(ctx, kVecReserve, {p + 676, (cap << 1) & 0x1FFFEu});
                n = load<u16>(p + 684);
            }
            const u32 dst = load<u32>(p + 676) + ((n << 7) & 0x7FFF80u);
            for (u32 k = 0; k < 32; ++k) CopyW(dst + 4 * k, tmp + 4 * k);
            const u32 c = load<u16>(p + 684) + 1;
            const u32 hw = load<u16>(p + 686);
            store<u16>(p + 684, static_cast<u16>(c));
            store<u16>(p + 686, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFF) ? hw : c));
            for (u32 k = 5; k-- > 0;) {
                call(ctx, kVecClear, {tmp + 20 * k});
                call(ctx, kNop, {tmp + 20 * k});
            }
            const u32 e = load<u32>(p + 676) + ((load<u16>(p + 684) - 1) << 7);
            const u32 def = load<u32>(item + 84);
            if (def != 0) store<u32>(e + 100, static_cast<u32>(call(ctx, kWorldObjInstance, {def, 0})));
            store<u32>(e + 112, static_cast<u32>(call(ctx, kFindBone, {p, load<u32>(item + 32)})));
            const u32 b2 = static_cast<u32>(call(ctx, kFindBone, {p, load<u32>(item + 36)}));
            const u32 b1 = load<u32>(e + 112);
            store<u32>(e + 116, b2);
            if (b1 != 0 || b2 != 0) store<u32>(e + 120, static_cast<u32>(call(ctx, kMemAlloc, {64, 0, 16})));
            for (u32 k = 0; k < 5; ++k) call(ctx, kLinkPart, {p, e + 20 * k, item + 240 + 20 * k});
            const f64 k0 = F(kK), twoPi = F(kK + 4), one = F(kK + 8);
            const f64 inv = Divs(one, Muls(F(item + 96), twoPi));
            const f64 v = Muls(Muls(Muls(F(load<u32>(p + 396) + 1324), k0), inv), twoPi);
            Stfs(e + 124, v);
            ctx.fpr[1] = v;
        }
        u32 inst = 0;
        const u32 idef = load<u32>(grp + 24);
        if (idef != 0) inst = static_cast<u32>(call(ctx, kWorldObjInstance, {idef, 0}));
        r3 = PushU32(ctx, p + 696, inst);
    }
    ctx.gpr[3] = r3;
}

namespace {
// The Havok allocator (r13 router +44) and a block of size from it (vfunc
// +8); the size is stored at +4 before the null test (quirk).
u32 HkAlloc(PPUContext& ctx, u32 size) {
    const u32 al = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(kTlsRouter)) + 44);
    const u32 b = static_cast<u32>(VCall(ctx, al, 8, {al, size}));
    store<u16>(b + 4, static_cast<u16>(size));
    return b;
}
// stvx of {x, y, z, 0} built from three words (lvlx / vspltw / vsel with
// the .text masks and vandc of the w lane).
void StoreXyz0(u32 ea, u32 x, u32 y, u32 z) {
    store<u32>(ea + 0, x);
    store<u32>(ea + 4, y);
    store<u32>(ea + 8, z);
    store<u32>(ea + 12, 0);
}
}  // namespace

// void Player_Setup230CA0(Player* p) (name?): when the tuning radius
// (p->396 + 1552 +12) is > 0, builds the player's trigger proxy: a rigid
// body cinfo (0x0091FB50, filled by 0x0091D208 from the rig's +24 body), a
// 32-byte sphere shape (0x0089FE70; radius) and a 544-byte body
// (0x0091E100) stored at p->672 (the shape's reference dropped); the body
// is placed at the rig body's +240 transform (translation + tune +16..+24),
// activated with zero velocities (vfuncs +68 / +72 of its +224), given the
// properties 0xA734004D = 0x7A30D7E3 and 0x91C7CF5B = p, the listeners
// p+388 / p+392 and the name "vehicle trigger proxy".
DECOMP(00230CA0, Player_Setup230CA0) {
    const u32 p = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x00230C60u;      // .text f32 0, then the vsel / vandc masks
    constexpr u32 kZero3 = 0x01164B40u;  // zero vector
    const u32 t = load<u32>(p + 396) + 1552;
    if (!(F(t + 12) > F(kK))) {
        SetF1Bits(ctx, load<u32>(kK));
        return;
    }
    Stack frame(ctx, 368);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    const u32 ci = sp + 256;             // hkpRigidBodyCinfo (sp + 256 .. 464)
    call(ctx, 0x0091FB50u, {ci});        // Hk_ShapeCinfoCtor91FB50
    call(ctx, 0x0091D208u, {load<u32>(load<u32>(p + 612) + 24), ci});
    u32 b = HkAlloc(ctx, 32);
    u32 shape = 0;
    if (b != 0) {
        callf(ctx, 0x0089FE70u, {b}, {LfsAt(t + 12)});
        shape = b;
    }
    store<u32>(sp + 260, shape);         // the cinfo's shape
    b = HkAlloc(ctx, 544);
    u32 body = 0;
    if (b != 0) {
        call(ctx, 0x0091E100u, {b, ci});  // Hk_ShapeCtor91E100
        body = b;
    }
    const u32 sh = load<u32>(sp + 260);
    store<u32>(p + 672, body);
    call(ctx, 0x007D4EA8u, {sh});        // AnimSkel_Release7D4EA8: drop the shape reference

    const u32 m = load<u32>(load<u32>(p + 612) + 24) + 240;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 192 + 16 * r + 4 * k, m + 16 * r + 4 * k);
    const f64 x = Adds(F(sp + 240), F(t + 16));
    Stfs(sp + 240, x);
    Stfs(sp + 244, Adds(F(sp + 244), F(t + 20)));
    Stfs(sp + 248, Adds(F(sp + 248), F(t + 24)));
    ctx.fpr[1] = x;                      // f1 as the original leaves it
    const u32 xw = StfsW(x);
    const u32 bodyNow = load<u32>(p + 672);
    StoreXyz0(sp + 128, load<u32>(sp + 192), load<u32>(sp + 196), load<u32>(sp + 200));
    for (u32 k = 0; k < 4; ++k) store<u32>(sp + 464 + 4 * k, xw);
    StoreXyz0(sp + 144, load<u32>(sp + 208), load<u32>(sp + 212), load<u32>(sp + 216));
    StoreXyz0(sp + 160, load<u32>(sp + 224), load<u32>(sp + 228), load<u32>(sp + 232));
    StoreXyz0(sp + 176, xw, load<u32>(sp + 244), load<u32>(sp + 248));
    call(ctx, 0x0091EAB8u, {bodyNow, sp + 128});   // Hk_SetTransform91EAB8

    const u32 v = sp + 112;
    StoreXyz0(v, load<u32>(kZero3), load<u32>(kZero3 + 4), load<u32>(kZero3 + 8));
    u32 bd = load<u32>(p + 672);
    call(ctx, 0x0091A4C8u, {bd, kZero3 + 4});      // Hk_Activate91A4C8 (r4 as the original leaves it)
    VCall(ctx, bd + 224, 68, {bd + 224, v});       // linear velocity
    StoreXyz0(v, load<u32>(kZero3), load<u32>(kZero3 + 4), load<u32>(kZero3 + 8));
    bd = load<u32>(p + 672);
    call(ctx, 0x0091A4C8u, {bd});
    VCall(ctx, bd + 224, 72, {bd + 224, v});       // angular velocity
    bd = load<u32>(p + 672);
    store<u16>(bd + 510, 0);
    call(ctx, 0x0093FE00u, {bd, 0xA734004Du, 0x7A30D7E3u});           // Hk_AddProperty93FE00
    call(ctx, 0x0093FE00u, {load<u32>(p + 672), 0x91C7CF5Bu, p});
    call(ctx, 0x0091A82Cu, {load<u32>(p + 672), p + 388});            // HkBody_AddListener91A82C
    call(ctx, 0x0091A5F8u, {load<u32>(p + 672), p + 392});            // HkBody_AddListener91A5F8
    call(ctx, 0x007CE950u, {load<u32>(p + 672) + 120, 0x00AD3CD8u});  // HkBody_SetName7CE950 ("vehicle trigger proxy")
}

// void Player_SetRate2_236868(Player* p, ?; f32 speed, f32 spin) (name?):
// with a rig body (p->612 +24), re-orients it to face the horizontal
// direction (p->128, 0, p->136) (the vector at 0x01164B70 when that is
// within K0 of the zero vector 0x01164B40): rows {up x n, up, n, pos}
// where up = 0x01164B60 and pos = p->144 + up' * speed / max(up'.y, 0.5)
// (up' = p->656 when its squared length > K0, else up); then activates
// it with the angular velocity up * sqrt(spin * 2 * |hkWorld +16 + tune
// p->396 +1456|) * Comp_GetF32(body +224) (vfunc +84 of its +224). The
// second argument is unread.
DECOMP(00236868, Player_SetRate2_236868) {
    const u32 p = arg<u32>(ctx, 0);
    const f64 speed = farg(ctx, 0);
    const f64 spin = farg(ctx, 1);
    constexpr u32 kK = 0x00236810u;      // .text f32 1e-5, 0.5, f64 0.5 (+8), 0 (+12), 1 (+16), masks (+32..), 2 (+80)
    constexpr u32 kZero3 = 0x01164B40u;
    constexpr u32 kUp = 0x01164B60u;
    constexpr u32 kAltDir = 0x01164B70u;
    const u32 hkw = load<u32>(load<u32>(load<u32>(kWorld) + 0x1281D4u) + 4);
    const u32 rig = load<u32>(p + 612);
    if (rig == 0 || load<u32>(rig + 24) == 0) { ctx.gpr[3] = 0; return; }
    const u32 body = load<u32>(rig + 24);
    const f64 eps = F(kK);
    f64 l2 = Muls(F(p + 660), F(p + 660));
    l2 = Fmadds(F(p + 656), F(p + 656), l2);
    l2 = Fmadds(F(p + 664), F(p + 664), l2);
    const u32 ax = l2 > eps ? p + 656 : kUp;
    const f64 ay = F(ax + 4);
    const f64 ys = static_cast<f32>(Fsel(Subs(ay, F(kK + 4)), ay, Lfd(kK + 8)));
    const f64 k = Divs(speed, ys);
    const f64 px = Fmadds(k, F(ax + 0), F(p + 144));
    const f64 py = Fmadds(k, ay, F(p + 148));
    const f64 pz = Fmadds(k, F(ax + 8), F(p + 152));
    f64 dx = F(p + 128), dy = F(kK + 12), dz = F(p + 136);
    if (std::fabs(Subs(dx, F(kZero3))) < eps && std::fabs(Subs(dy, F(kZero3 + 4))) < eps &&
        std::fabs(Subs(dz, F(kZero3 + 8))) < eps) {
        dx = F(kAltDir); dy = F(kAltDir + 4); dz = F(kAltDir + 8);
    }
    f64 len = Muls(dy, dy);
    len = Fmadds(dx, dx, len);
    len = Fmadds(dz, dz, len);
    const f64 inv = Divs(F(kK + 16), static_cast<f32>(std::sqrt(len)));
    const f64 ny = Muls(inv, dy), nz = Muls(inv, dz), nx = Muls(inv, dx);
    const f64 u0 = F(kUp), u1 = F(kUp + 4), u2 = F(kUp + 8);
    const f64 cx = Fmsubs(u1, nz, Muls(u2, ny));
    const f64 cy = Fmsubs(u2, nx, Muls(u0, nz));
    const f64 cz = Fmsubs(u0, ny, Muls(u1, nx));
    Stack frame(ctx, 160);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    const u32 m = sp + 128;
    store<u32>(m + 0, StfsW(cx)); store<u32>(m + 4, StfsW(cy)); store<u32>(m + 8, StfsW(cz)); store<u32>(m + 12, 0);
    CopyW(m + 16, kUp); CopyW(m + 20, kUp + 4); CopyW(m + 24, kUp + 8); store<u32>(m + 28, 0);
    store<u32>(m + 32, StfsW(nx)); store<u32>(m + 36, StfsW(ny)); store<u32>(m + 40, StfsW(nz)); store<u32>(m + 44, 0);
    store<u32>(m + 48, StfsW(px)); store<u32>(m + 52, StfsW(py)); store<u32>(m + 56, StfsW(pz)); store<u32>(m + 60, 0);
    call(ctx, 0x0091EAB8u, {body, m});   // Hk_SetTransform91EAB8

    const u32 tb = load<u32>(p + 396) + 1424;
    const f64 ox = Adds(F(hkw + 16), F(tb + 32));
    const f64 oy = Adds(F(hkw + 20), F(tb + 36));
    const f64 oz = Adds(F(hkw + 24), F(tb + 40));
    f64 d2 = Muls(oy, oy);
    d2 = Fmadds(ox, ox, d2);
    const f64 s2 = Muls(spin, F(kK + 80));
    d2 = Fmadds(oz, oz, d2);
    const f64 w = static_cast<f32>(std::sqrt(static_cast<f64>(Muls(s2, static_cast<f32>(std::sqrt(d2))))));
    const f64 wx = Muls(w, F(kUp)), wy = Muls(w, F(kUp + 4)), wz = Muls(w, F(kUp + 8));
    call(ctx, 0x00922918u, {load<u32>(load<u32>(p + 612) + 24) + 224});   // Comp_GetF32_922918 -> f1
    const f64 g = ctx.fpr[1];
    const f64 vz = Muls(g, wz);
    const u32 v = sp + 112;
    store<u32>(v + 0, StfsW(Muls(g, wx)));
    store<u32>(v + 4, StfsW(Muls(g, wy)));
    store<u32>(v + 8, StfsW(vz));
    store<u32>(v + 12, 0);
    const u32 b2 = load<u32>(load<u32>(p + 612) + 24);
    ctx.fpr[1] = vz;                       // f1 as the original leaves it
    call(ctx, 0x0091A4C8u, {b2, 112});     // Hk_Activate91A4C8 (r4 = 112 as the original leaves it)
    VCall(ctx, b2 + 224, 84, {b2 + 224, v});
}

namespace {
constexpr u32 kSubSelected = 0x011DE2A8u;   // the selected sub-set entry's def
constexpr u32 kRefComp = 0x0027469Eu;       // effect component hash (its +28 holds a counted reference)

// A sub-set select (root *(ss +12), path +16, count +96, selected +100,
// global kSubSelected) in the effect def d's +24 sub-set; returns the
// selected entry's def (d +24 re-read, +100, +8; 0 for any null on the
// way, or when not found).
u32 SubSelectDef(u32 d, u32 hash) {
    if (d == 0) return 0;
    const u32 ss = load<u32>(d + 24);
    if (ss == 0) return 0;
    if (TreeSelect(ss, 12, 96, 16, 100, kSubSelected, hash) == 0) return 0;
    const u32 ss2 = load<u32>(d + 24);
    if (ss2 == 0) return 0;
    const u32 s = load<u32>(ss2 + 100);
    return s != 0 ? load<u32>(s + 8) : 0;
}

// A new Effect's kRefComp component: when its vfunc +32 says kind 5, its
// +28 reference is replaced by p->160 (old one released by vfunc +8, the
// new one's u16 count at +4 bumped).
void SetRefComp(PPUContext& ctx, u32 p, u32 e) {
    const u32 comp = static_cast<u32>(call(ctx, b12fn::Effect_FindComponent, {e, kRefComp}));
    if (comp == 0) return;
    if (static_cast<s32>(VCall(ctx, comp, 32, {comp})) != 5) return;
    const u32 old = load<u32>(comp + 28);
    const u32 ref = load<u32>(p + 160);
    if (old != 0) VCall(ctx, old, 8, {old});
    store<u32>(comp + 28, ref);
    if (ref != 0) store<u16>(ref + 4, static_cast<u16>(load<u16>(ref + 4) + 1));
}

// Clear e->136 bit 0x200 of the effect at slot, then Effect_Start it.
void StartCleared(PPUContext& ctx, u32 slot) {
    const u32 e = load<u32>(slot);
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
    call(ctx, b12fn::Effect_Start, {load<u32>(slot)});
}
}  // namespace

// void Player_Setup231928(Player* p) (name?): looks up the player's
// effects in the global set by the tuning hashes (t = p->396): t+824 +28
// -> p->748 (its kRefComp reference set to p->160, then p->752 / p->756 =
// the defs of its sub-set entries 2527 / 0x10CBB, flag 0x200 cleared,
// started), t+824 +32 -> p->768 and +36 -> p->776 (reference, flag
// cleared, started, timed by the .text f32 at 0x00231924); p->760 / p->764
// = the defs found for t+1552 / t+1584. Then p->992 / p->996 = t+1588 /
// t+1556. Lookups leave the set's selection as SetSelect does.
DECOMP(00231928, Player_Setup231928) {
    const u32 p = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x00231924u;
    const u32 tb = load<u32>(p + 396);
    const u32 tA = tb + 824, tB = tb + 1552, tC = tb + 1584;
    const u32 set = static_cast<u32>(call(ctx, b12fn::Set_Find, {0}));
    u32 r3 = 0;
    auto foundDef = [&](u32 hash) -> u32 {
        if (SetSelect(set, hash) == 0) return 0;
        const u32 n = load<u32>(set + 104);
        return n != 0 ? load<u32>(n + 8) : 0;
    };
    if (set != 0) {
        u32 def = foundDef(load<u32>(tA + 28));
        if (def != 0) {
            store<u32>(p + 748, NewEffect(ctx, def));
            SetRefComp(ctx, p, load<u32>(p + 748));
            store<u32>(p + 752, SubSelectDef(load<u32>(load<u32>(p + 748)), 2527));
            store<u32>(p + 756, SubSelectDef(load<u32>(load<u32>(p + 748)), 0x10CBB));
            StartCleared(ctx, p + 748);
        }
        for (u32 k = 0; k < 2; ++k) {
            const u32 slot = p + (k == 0 ? 768 : 776);
            def = foundDef(load<u32>(tA + (k == 0 ? 32 : 36)));
            if (def == 0) continue;
            store<u32>(slot, NewEffect(ctx, def));
            SetRefComp(ctx, p, load<u32>(slot));
            StartCleared(ctx, slot);
            const u32 e = load<u32>(slot);
            callf(ctx, b12fn::Effect_SetTime, {e, e, kK}, {LfsAt(kK)});
        }
        store<u32>(p + 760, foundDef(load<u32>(tB)));
        // The last lookup sets r3 as the original leaves it: the re-read
        // node when found, the hash after a walk off the tree, 0 when empty.
        const u32 h5 = load<u32>(tC);
        u32 d5 = 0;
        if (load<u32>(load<u32>(set + 16)) == 0) {
            SetSelect(set, h5);
        } else if (SetSelect(set, h5) != 0) {
            r3 = load<u32>(set + 104);
            if (r3 != 0) d5 = load<u32>(r3 + 8);
        } else {
            r3 = h5;
        }
        store<u32>(p + 764, d5);
    }
    CopyW(p + 992, tC + 4);
    const u32 w = load<u32>(tB + 4);
    store<u32>(p + 996, w);
    SetF1Bits(ctx, w);
    ctx.gpr[3] = r3;
}
