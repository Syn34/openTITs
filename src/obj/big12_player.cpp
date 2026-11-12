// Game code, large-function sweep part 12: the player's setup and rate
// helpers (callees of Player_ResetFromDef238270 and friends).
#include "big12.h"
#include "big13.h"

namespace sly4::obj {

namespace {
inline be_u32& W(u32 base, u32 off) { return gm<be_u32>(base + off); }
inline f64 F(u32 ea) { return LfsW(gm<be_u32>(ea)); }
inline void St(u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); }
inline void CopyW(u32 dst, u32 src) { gm<be_u32>(dst) = gm<be_u32>(src).get(); }
}  // namespace

namespace {
constexpr u32 Effect_Start12p = 0x004C753C;         // (Effect*)
constexpr u32 Effect_SetTime12p = 0x004CF710;       // (Effect*; f32) (reads r3 / f1)
constexpr u32 Effect_FindComponent12p = 0x004BB4B4; // (Effect*, u32 hash) -> component or 0
constexpr u32 Set_Find12p = 0x004BCE90;             // (u32 name) -> effect set
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
void Player_Setup232F00(u32 p, u32 q) {
    const u32 tier = gm<be_u32>(gm<be_u32>(gm<be_u32>(kWorld) + 0x1281D4u) + 4);
    Player_TunePart232B60(p, tier, gm<be_u32>(q + 28));

    u32 s = gm<be_u32>(p + 396) + 1248;
    u32 d = gm<be_u32>(q + 32);
    const u8 flag = gm<u8>(s + 12);
    CopyW(d + 8, s + 0);
    CopyW(d + 12, s + 4);
    const u32 w8 = gm<be_u32>(s + 8);
    gm<u8>(d + 20) = flag;
    gm<be_u32>(d + 16) = w8;
    Player_TunePart2329DC(p, gm<be_u32>(q + 28), gm<be_u32>(q + 36));

    s = gm<be_u32>(p + 396) + 1272;
    d = gm<be_u32>(q + 40);
    for (u32 k = 0; k < 4; ++k) CopyW(d + 8 + 4 * k, s + 4 * k);
    for (u32 k = 0; k < 4; ++k) CopyW(d + 24 + 4 * k, s + 28 + 4 * k);
    const u32 w44 = gm<be_u32>(s + 44);
    const u32 a28 = gm<be_u32>(q + 28);
    gm<be_u32>(d + 40) = w44;
    const u32 w48 = gm<be_u32>(s + 48);
    const u32 a44 = gm<be_u32>(q + 44);
    gm<be_u32>(d + 44) = w48;
    Player_TunePart2327A8(p, a28, a44);
    Player_TunePart232650(p, gm<be_u32>(q + 28), gm<be_u32>(q + 48));
    Player_TunePart232360(p, gm<be_u32>(q + 28), gm<be_u32>(q + 52));

    const u32 t = gm<be_u32>(p + 396);
    d = gm<be_u32>(q + 56);
    for (u32 k = 0; k < 4; ++k) CopyW(d + 8 + 4 * k, t + 1424 + 4 * k);
    // lvlx / vspltw of +1440 / +1444 / +1448, merged by the .text masks at
    // 0x00232ED0 / EE0 (vsel) and EF0 (vandc): {x, y, z, 0}.
    const u32 v0 = gm<be_u32>(t + 1440), v1 = gm<be_u32>(t + 1444), v2 = gm<be_u32>(t + 1448);
    const u32 r29 = t + 1500;
    const u32 d68 = gm<be_u32>(q + 68);
    const u32 pid = gm<be_u32>(p + 616);
    const u32 d60 = gm<be_u32>(q + 60);
    const u32 vq = (d + 32) & ~15u;
    gm<be_u32>(vq + 0) = v0;
    gm<be_u32>(vq + 4) = v1;
    gm<be_u32>(vq + 8) = v2;
    gm<be_u32>(vq + 12) = 0;
    CopyW(d68 + 8, t + 1488);
    gm<be_u32>(d60 + 12) = (pid << 16) | 0xDu;
    CopyW(d68 + 12, t + 1492);
    const u32 old = gm<be_u32>(q + 64);
    CopyW(d68 + 16, t + 1496);
    if (old != 0) {
        vcall(host<u8>(old), 4);
        gm<be_u32>(q + 64) = 0;
    }
    // Quirk: the size is stored at +4 before the null test.
    const u32 shape = HkAlloc(28);
    u32 r3 = 0;
    if (shape != 0) {
        call(0x009FE8B0u, shape, gm<be_u32>(q + 28), static_cast<s32>(gm<be_u32>(r29 + 8).get()));
        r3 = shape;
    }
    gm<be_u32>(q + 64) = r3;
    // Unchecked: a failed allocation writes at 8 / 12.
    const u32 u = gm<be_u32>(p + 396) + 1500;
    CopyW(r3 + 8, u);
    const u32 w = gm<be_u32>(u + 4);
    gm<be_u32>(r3 + 12) = w;
}

// void Player_DropLinks2349E0(Player* p) (name?): for each 128-byte link
// entry (p->676, u16 count p->684): releases (vfunc +8) and clears +100 and
// the +8 object of every 20-byte element of its five arrays ({+0 data, +8
// u16 count} ... {+80, +88}), zeroing each count; its +104 skeleton is
// detached (0x009334A0(scratch, world tier, s) when both its +20 / +24
// parts have a +8) and released, its +108 one removed from the physics
// world (when its +8 is set) and released, and +120 freed. Then the
// counts are zeroed and the p->696 objects (u16 count p->704) released.
void Player_DropLinks2349E0(u32 p) {
    constexpr u32 kTierTable = 0x1281D4u;   // world +: tier block (+4 the physics world)
    GuestFrame frame(16);
    const u32 sp = frame.addr();            // the original's sp + 112 (two byte-sized scratch outs)
    const u32 hkw = gm<be_u32>(gm<be_u32>(gm<be_u32>(kWorld) + kTierTable) + 4);
    auto release = [&](u32 slot) {
        const u32 x = gm<be_u32>(slot);
        if (x == 0) return;
        vcall(host<u8>(x), 8);
        gm<be_u32>(slot) = 0;
    };
    for (u32 i = 0; i < gm<be_u16>(p + 684); ++i) {
        const u32 e = gm<be_u32>(p + 676) + 128 * i;
        release(e + 100);
        for (u32 a = 0; a < 5; ++a) {
            for (u32 j = 0; j < gm<be_u16>(e + 20 * a + 8); ++j) release(gm<be_u32>(e + 20 * a) + 20 * j + 8);
            gm<be_u16>(e + 20 * a + 8) = 0;
        }
        const u32 tier = gm<be_u32>(gm<be_u32>(gm<be_u32>(kWorld) + kTierTable) + 4);
        u32 s = gm<be_u32>(e + 104);
        if (s != 0) {
            if (gm<be_u32>(gm<be_u32>(s + 20) + 8) != 0 && gm<be_u32>(gm<be_u32>(s + 24) + 8) != 0)
                call(0x009334A0u, sp + 1, tier, s);
            call(0x007D4EA8u, s);   // AnimSkel_Release7D4EA8
        }
        s = gm<be_u32>(e + 108);
        gm<be_u32>(e + 104) = 0;
        if (s != 0) {
            if (gm<be_u32>(s + 8) != 0) call(0x0092FFC8u, sp, hkw, s);   // Hk_RemoveEntity92FFC8
            call(0x007D4EA8u, s);
        }
        const u32 m = gm<be_u32>(e + 120);
        gm<be_u32>(e + 108) = 0;
        call(0x0054EE6Cu, m, 0);   // Mem_FreeFrom
        gm<be_u32>(e + 120) = 0;
    }
    gm<be_u16>(p + 684) = 0;
    for (u32 i = 0;; ++i) {
        if (i >= gm<be_u16>(p + 704)) break;
        const u32 x = gm<be_u32>(gm<be_u32>(p + 696) + 4 * i);
        if (x == 0) continue;
        vcall(host<u8>(x), 8);
        gm<be_u32>(gm<be_u32>(p + 696) + 4 * i) = 0;
    }
    gm<be_u16>(p + 704) = 0;
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
// when full.
void PushU32(u32 v, u32 value) {
    u32 n = gm<be_u16>(v + 8);
    const u32 cap = gm<be_u16>(v + 4);
    if (n == cap) {
        call(kVecReserve, v, (cap << 1) & 0x1FFFEu);
        n = gm<be_u16>(v + 8);
    }
    gm<be_u32>(gm<be_u32>(v) + ((n << 2) & 0x3FFFCu)) = value;
    const u32 c = gm<be_u16>(v + 8) + 1;
    const u32 hw = gm<be_u16>(v + 10);
    gm<be_u16>(v + 8) = static_cast<u16>(c);
    const u32 m = static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFF) ? hw : c;
    gm<be_u16>(v + 10) = static_cast<u16>(m);
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
void Player_Setup231470(u32 p) {
    constexpr u32 kK = 0x00231464u;   // .text f32: 0.447 (K0), 2pi, 1
    const u32 t = gm<be_u32>(p + 396) + 1072;
    const u32 ngroups = gm<be_u16>(t + 140);
    if (ngroups != 0) call(kVecReserve, p + 696, ngroups);
    GuestFrame frame(128);
    const u32 tmp = frame.addr();   // the original's sp + 112
    for (u32 g = 0; g < ngroups; ++g) {
        const u32 grp = gm<be_u32>(t + 132) + 32 * g;
        const u32 nitems = gm<be_u16>(grp + 8);
        for (u32 i = 0; i < nitems; ++i) {
            const u32 item = gm<be_u32>(grp) + 352 * i;
            for (u32 k = 0; k < 5; ++k) {
                call(kVecConstruct, tmp + 20 * k);
                gm<be_u16>(tmp + 20 * k + 6) = 20;
            }
            for (u32 k = 0; k < 6; ++k) gm<be_u32>(tmp + 100 + 4 * k) = 0;
            gm<be_u32>(tmp + 124) = 0x453B8000u;   // 3000.0f
            u32 n = gm<be_u16>(p + 684);
            const u32 cap = gm<be_u16>(p + 680);
            if (n == cap) {
                call(kVecReserve, p + 676, (cap << 1) & 0x1FFFEu);
                n = gm<be_u16>(p + 684);
            }
            const u32 dst = gm<be_u32>(p + 676) + ((n << 7) & 0x7FFF80u);
            for (u32 k = 0; k < 32; ++k) CopyW(dst + 4 * k, tmp + 4 * k);
            const u32 c = gm<be_u16>(p + 684) + 1;
            const u32 hw = gm<be_u16>(p + 686);
            gm<be_u16>(p + 684) = static_cast<u16>(c);
            gm<be_u16>(p + 686) = static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFF) ? hw : c);
            for (u32 k = 5; k-- > 0;) {
                call(kVecClear, tmp + 20 * k);
                call(kNop, tmp + 20 * k);
            }
            const u32 e = gm<be_u32>(p + 676) + ((gm<be_u16>(p + 684) - 1) << 7);
            const u32 def = gm<be_u32>(item + 84);
            if (def != 0) gm<be_u32>(e + 100) = call<u32>(kWorldObjInstance, def, 0);
            gm<be_u32>(e + 112) = call<u32>(kFindBone, p, gm<be_u32>(item + 32));
            const u32 b2 = call<u32>(kFindBone, p, gm<be_u32>(item + 36));
            const u32 b1 = gm<be_u32>(e + 112);
            gm<be_u32>(e + 116) = b2;
            if (b1 != 0 || b2 != 0) gm<be_u32>(e + 120) = call<u32>(kMemAlloc, 64, 0, 16);
            for (u32 k = 0; k < 5; ++k) Player_InitLinkPart231208(p, e + 20 * k, item + 240 + 20 * k);
            const f64 k0 = F(kK), twoPi = F(kK + 4), one = F(kK + 8);
            const f64 inv = Divs(one, Muls(F(item + 96), twoPi));
            const f64 v = Muls(Muls(Muls(F(gm<be_u32>(p + 396) + 1324), k0), inv), twoPi);
            St(e + 124, v);
        }
        u32 inst = 0;
        const u32 idef = gm<be_u32>(grp + 24);
        if (idef != 0) inst = call<u32>(kWorldObjInstance, idef, 0);
        PushU32(p + 696, inst);
    }
}

namespace {
// stvx of {x, y, z, 0} built from three words (lvlx / vspltw / vsel with
// the .text masks and vandc of the w lane).
void StoreXyz0(u32 ea, u32 x, u32 y, u32 z) {
    gm<be_u32>(ea + 0) = x;
    gm<be_u32>(ea + 4) = y;
    gm<be_u32>(ea + 8) = z;
    gm<be_u32>(ea + 12) = 0;
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
void Player_Setup230CA0(u32 p) {
    constexpr u32 kK = 0x00230C60u;      // .text f32 0, then the vsel / vandc masks
    constexpr u32 kZero3 = 0x01164B40u;  // zero vector
    const u32 t = gm<be_u32>(p + 396) + 1552;
    if (!(F(t + 12) > F(kK))) return;
    GuestFrame frame(368);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    const u32 ci = sp + 256;             // hkpRigidBodyCinfo (sp + 256 .. 464)
    call(0x0091FB50u, ci);        // Hk_ShapeCinfoCtor91FB50
    call(0x0091D208u, gm<be_u32>(gm<be_u32>(p + 612) + 24), ci);
    u32 b = HkAlloc(32);
    u32 shape = 0;
    if (b != 0) {
        call(0x0089FE70u, b, F(t + 12));
        shape = b;
    }
    gm<be_u32>(sp + 260) = shape;         // the cinfo's shape
    b = HkAlloc(544);
    u32 body = 0;
    if (b != 0) {
        call(0x0091E100u, b, ci);  // Hk_ShapeCtor91E100
        body = b;
    }
    const u32 sh = gm<be_u32>(sp + 260);
    gm<be_u32>(p + 672) = body;
    call(0x007D4EA8u, sh);        // AnimSkel_Release7D4EA8: drop the shape reference

    const u32 m = gm<be_u32>(gm<be_u32>(p + 612) + 24) + 240;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(sp + 192 + 16 * r + 4 * k, m + 16 * r + 4 * k);
    const f64 x = Adds(F(sp + 240), F(t + 16));
    St(sp + 240, x);
    St(sp + 244, Adds(F(sp + 244), F(t + 20)));
    St(sp + 248, Adds(F(sp + 248), F(t + 24)));
    const u32 xw = StfsBits(x);
    const u32 bodyNow = gm<be_u32>(p + 672);
    StoreXyz0(sp + 128, gm<be_u32>(sp + 192), gm<be_u32>(sp + 196), gm<be_u32>(sp + 200));
    for (u32 k = 0; k < 4; ++k) gm<be_u32>(sp + 464 + 4 * k) = xw;
    StoreXyz0(sp + 144, gm<be_u32>(sp + 208), gm<be_u32>(sp + 212), gm<be_u32>(sp + 216));
    StoreXyz0(sp + 160, gm<be_u32>(sp + 224), gm<be_u32>(sp + 228), gm<be_u32>(sp + 232));
    StoreXyz0(sp + 176, xw, gm<be_u32>(sp + 244), gm<be_u32>(sp + 248));
    call(0x0091EAB8u, bodyNow, sp + 128);   // Hk_SetTransform91EAB8

    const u32 v = sp + 112;
    StoreXyz0(v, gm<be_u32>(kZero3), gm<be_u32>(kZero3 + 4), gm<be_u32>(kZero3 + 8));
    u32 bd = gm<be_u32>(p + 672);
    call(0x0091A4C8u, bd);                  // Hk_Activate91A4C8
    vcall(host<u8>(bd + 224), 68, v);       // linear velocity
    StoreXyz0(v, gm<be_u32>(kZero3), gm<be_u32>(kZero3 + 4), gm<be_u32>(kZero3 + 8));
    bd = gm<be_u32>(p + 672);
    call(0x0091A4C8u, bd);
    vcall(host<u8>(bd + 224), 72, v);       // angular velocity
    bd = gm<be_u32>(p + 672);
    gm<be_u16>(bd + 510) = 0;
    call(0x0093FE00u, bd, 0xA734004Du, 0x7A30D7E3u);           // Hk_AddProperty93FE00
    call(0x0093FE00u, gm<be_u32>(p + 672), 0x91C7CF5Bu, p);
    call(0x0091A82Cu, gm<be_u32>(p + 672), p + 388);            // HkBody_AddListener91A82C
    call(0x0091A5F8u, gm<be_u32>(p + 672), p + 392);            // HkBody_AddListener91A5F8
    call(0x007CE950u, gm<be_u32>(p + 672) + 120, 0x00AD3CD8u);  // HkBody_SetName7CE950 ("vehicle trigger proxy")
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
void Player_SetRate2_236868(u32 p, u32, f64 speed, f64 spin) {
    constexpr u32 kK = 0x00236810u;      // .text f32 1e-5, 0.5, f64 0.5 (+8), 0 (+12), 1 (+16), masks (+32..), 2 (+80)
    constexpr u32 kZero3 = 0x01164B40u;
    constexpr u32 kUp = 0x01164B60u;
    constexpr u32 kAltDir = 0x01164B70u;
    const u32 hkw = gm<be_u32>(gm<be_u32>(gm<be_u32>(kWorld) + 0x1281D4u) + 4);
    const u32 rig = gm<be_u32>(p + 612);
    if (rig == 0 || gm<be_u32>(rig + 24) == 0) return;
    const u32 body = gm<be_u32>(rig + 24);
    const f64 eps = F(kK);
    f64 l2 = Muls(F(p + 660), F(p + 660));
    l2 = Fmadds(F(p + 656), F(p + 656), l2);
    l2 = Fmadds(F(p + 664), F(p + 664), l2);
    const u32 ax = l2 > eps ? p + 656 : kUp;
    const f64 ay = F(ax + 4);
    const f64 ys = static_cast<f32>(Fsel(Subs(ay, F(kK + 4)), ay, gm<be_f64>(kK + 8).get()));
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
    GuestFrame frame(160);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    const u32 m = sp + 128;
    gm<be_u32>(m + 0) = StfsBits(cx); gm<be_u32>(m + 4) = StfsBits(cy); gm<be_u32>(m + 8) = StfsBits(cz); gm<be_u32>(m + 12) = 0;
    CopyW(m + 16, kUp); CopyW(m + 20, kUp + 4); CopyW(m + 24, kUp + 8); gm<be_u32>(m + 28) = 0;
    gm<be_u32>(m + 32) = StfsBits(nx); gm<be_u32>(m + 36) = StfsBits(ny); gm<be_u32>(m + 40) = StfsBits(nz); gm<be_u32>(m + 44) = 0;
    gm<be_u32>(m + 48) = StfsBits(px); gm<be_u32>(m + 52) = StfsBits(py); gm<be_u32>(m + 56) = StfsBits(pz); gm<be_u32>(m + 60) = 0;
    call(0x0091EAB8u, body, m);   // Hk_SetTransform91EAB8

    const u32 tb = gm<be_u32>(p + 396) + 1424;
    const f64 ox = Adds(F(hkw + 16), F(tb + 32));
    const f64 oy = Adds(F(hkw + 20), F(tb + 36));
    const f64 oz = Adds(F(hkw + 24), F(tb + 40));
    f64 d2 = Muls(oy, oy);
    d2 = Fmadds(ox, ox, d2);
    const f64 s2 = Muls(spin, F(kK + 80));
    d2 = Fmadds(oz, oz, d2);
    const f64 w = static_cast<f32>(std::sqrt(static_cast<f64>(Muls(s2, static_cast<f32>(std::sqrt(d2))))));
    const f64 wx = Muls(w, F(kUp)), wy = Muls(w, F(kUp + 4)), wz = Muls(w, F(kUp + 8));
    const f64 g = call<f64>(0x00922918u, gm<be_u32>(gm<be_u32>(p + 612) + 24) + 224);   // Comp_GetF32_922918 -> f1
    const f64 vz = Muls(g, wz);
    const u32 v = sp + 112;
    gm<be_u32>(v + 0) = StfsBits(Muls(g, wx));
    gm<be_u32>(v + 4) = StfsBits(Muls(g, wy));
    gm<be_u32>(v + 8) = StfsBits(vz);
    gm<be_u32>(v + 12) = 0;
    const u32 b2 = gm<be_u32>(gm<be_u32>(p + 612) + 24);
    call(0x0091A4C8u, b2);          // Hk_Activate91A4C8
    vcall(host<u8>(b2 + 224), 84, v);
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
    const u32 ss = gm<be_u32>(d + 24);
    if (ss == 0) return 0;
    if (BstSelect(ss, 12, 96, 16, 100, kSubSelected, hash) == 0) return 0;
    const u32 ss2 = gm<be_u32>(d + 24);
    if (ss2 == 0) return 0;
    const u32 s = gm<be_u32>(ss2 + 100);
    return s != 0 ? gm<be_u32>(s + 8) : 0;
}

// A new Effect's kRefComp component: when its vfunc +32 says kind 5, its
// +28 reference is replaced by p->160 (old one released by vfunc +8, the
// new one's u16 count at +4 bumped).
void SetRefComp(u32 p, u32 e) {
    const u32 comp = call<u32>(Effect_FindComponent12p, e, kRefComp);
    if (comp == 0) return;
    if (vcall<s32>(host<u8>(comp), 32) != 5) return;
    const u32 old = gm<be_u32>(comp + 28);
    const u32 ref = gm<be_u32>(p + 160);
    if (old != 0) vcall(host<u8>(old), 8);
    gm<be_u32>(comp + 28) = ref;
    if (ref != 0) gm<be_u16>(ref + 4) = static_cast<u16>(gm<be_u16>(ref + 4) + 1);
}

// Clear e->136 bit 0x200 of the effect at slot, then Effect_Start it.
void StartCleared(u32 slot) {
    const u32 e = gm<be_u32>(slot);
    gm<be_u32>(e + 136) = gm<be_u32>(e + 136) & ~0x200u;
    call(Effect_Start12p, gm<be_u32>(slot));
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
void Player_Setup231928(u32 p) {
    constexpr u32 kK = 0x00231924u;
    const u32 tb = gm<be_u32>(p + 396);
    const u32 tA = tb + 824, tB = tb + 1552, tC = tb + 1584;
    const u32 set = call<u32>(Set_Find12p, 0u);
    auto foundDef = [&](u32 hash) -> u32 {
        if (EffectSetSelect(set, hash) == 0) return 0;
        const u32 n = gm<be_u32>(set + 104);
        return n != 0 ? gm<be_u32>(n + 8) : 0;
    };
    if (set != 0) {
        u32 def = foundDef(gm<be_u32>(tA + 28));
        if (def != 0) {
            gm<be_u32>(p + 748) = NewEffect140(def);
            SetRefComp(p, gm<be_u32>(p + 748));
            gm<be_u32>(p + 752) = SubSelectDef(gm<be_u32>(gm<be_u32>(p + 748)), 2527);
            gm<be_u32>(p + 756) = SubSelectDef(gm<be_u32>(gm<be_u32>(p + 748)), 0x10CBB);
            StartCleared(p + 748);
        }
        for (u32 k = 0; k < 2; ++k) {
            const u32 slot = p + (k == 0 ? 768 : 776);
            def = foundDef(gm<be_u32>(tA + (k == 0 ? 32 : 36)));
            if (def == 0) continue;
            gm<be_u32>(slot) = NewEffect140(def);
            SetRefComp(p, gm<be_u32>(slot));
            StartCleared(slot);
            const u32 e = gm<be_u32>(slot);
            call(Effect_SetTime12p, e, F(kK));
        }
        gm<be_u32>(p + 760) = foundDef(gm<be_u32>(tB));
        gm<be_u32>(p + 764) = foundDef(gm<be_u32>(tC));
    }
    CopyW(p + 992, tC + 4);
    const u32 w = gm<be_u32>(tB + 4);
    gm<be_u32>(p + 996) = w;
}

}  // namespace sly4::obj

NATIVE(00232F00, sly4::obj::Player_Setup232F00)
NATIVE(002349E0, sly4::obj::Player_DropLinks2349E0)
NATIVE(00231470, sly4::obj::Player_Setup231470)
NATIVE(00230CA0, sly4::obj::Player_Setup230CA0)
NATIVE(00236868, sly4::obj::Player_SetRate2_236868)
NATIVE(00231928, sly4::obj::Player_Setup231928)
