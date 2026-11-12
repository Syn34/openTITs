// Game code, large-function sweep part 31: a marker's state change (its
// shown object swapped, or a ground-snapped debris object spawned).
#include "../sly4.h"
#include "big31.h"

using namespace sly4;
using namespace sly4::big31;

namespace {
constexpr u32 kMarkK = 0x00308180;        // .text: f32 25.0 (ray top y), 0 (x / z step), -50.0 (y step); lane masks y / z / w
                                      // at +16 / +32 / +48; f32 0.1 (lift) at +64
constexpr u32 kUp = 0x01164B60;       // f32[3] the up axis
constexpr u32 kIdentRot = 0x01164C50; // the global identity rotation (Mat33)
constexpr u32 kGame = 0x00F65CA0;     // Game* (+0x1281D4 the physics holder, +4 its world)
constexpr u32 kDebrisHash = 0x5F9739CF;
constexpr u32 Obj_SpawnToward307F70 = 0x00307F70;  // Obj* (Obj* a, Def* b, const Vec3* from, const Vec3* to)
constexpr u32 Obj_SpawnHashMtx307CC8 = 0x00307CC8; // Obj* (Owner*, u32 hash, const Mat34*)
constexpr u32 Mat34_FromRotPos553B4C = 0x00553B4C; // (Mat34* out, const Mat33*, const f32 p[3])
constexpr u32 PtrVector_Reserve = 0x00551718;      // (vec, u32 n)
constexpr u32 HkWorld_CastRay92E2E8 = 0x0092E2E8;  // (hkpWorld*, const hkpWorldRayCastInput*, hkpWorldRayCastOutput*)

// The shown object m->+96 is hidden (vt +40 (x, 0), again with 1 when its
// +172 has bit 0x00040000) and cleared.
void Release(PPUContext& ctx, u32 m) {
    u32 x = load<u32>(m + 96);
    if (x == 0) return;
    VCall(ctx, x, 40, {x, 0});
    x = load<u32>(m + 96);
    if (load<u32>(x + 172) & 0x00040000u) VCall(ctx, x, 40, {x, 1});
    store<u32>(m + 96, 0);
}

// Shows y = Obj_SpawnToward307F70(m->+0, m->+80, from, to) instead of
// m->+96 (hidden as in Release); y gets +178 += 1 and vt +104 (y, m->+132)
// with a non-zero +132. Nothing happens when y is already shown.
void Swap(PPUContext& ctx, u32 m, u32 from, u32 to) {
    const u32 y = static_cast<u32>(call(ctx, Obj_SpawnToward307F70, {load<u32>(m), load<u32>(m + 80), from, to}));
    u32 x = load<u32>(m + 96);
    if (x == y) return;
    if (x != 0) {
        VCall(ctx, x, 40, {x, 0});
        x = load<u32>(m + 96);
        if (load<u32>(x + 172) & 0x00040000u) VCall(ctx, x, 40, {x, 1});
    }
    if (y != 0) {
        store<u16>(y + 178, static_cast<u16>(load<u16>(y + 178) + 1));
        const u32 k = load<u32>(m + 132);
        if (k != 0) VCall(ctx, y, 104, {y, k});
    }
    store<u32>(m + 96, y);
}
}  // namespace

// void Marker_SetState(Marker* m, u32 state) (called by f_00308C98 and
// Obj_DropMarker_0030A900): m->+88 = state, then
//  1: Swap from m +16 to m +16 + up;
//  3: Swap from m +32 + up * m +52 to m +32;
//  2: Release;
//  4: with u8 +136 and a shown object x: a ray from (x.x, 25, x.z) to
//     (x.x, -25, x.z) (x = x +144; RayInput at sp + 160 with filter 13 at
//     +36, output at sp + 272) and, on a hit, when the owner m->+0 has room
//     in its +876 u32 PtrVector (count u16 +884 below capacity u16 +880):
//     y = 0x00307CC8(owner, 0x5F9739CF, identity rotation at the hit point
//     lifted by 0.1) gets +178 += 1 and is pushed (Reserve when full,
//     high-water mark u16 +886). Release in every case.
// Other states do nothing more.
DECOMP(003081C8, Marker_ChangeState_003081C8) {
    const u32 m = arg<u32>(ctx, 0);
    const s32 state = static_cast<s32>(arg<u32>(ctx, 1));
    store<u32>(m + 88, static_cast<u32>(state));
    Stack frame(ctx, 256);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    if (state == 3) {
        const f64 s = Lf(m + 52);
        store<u32>(sp + 128, Fb(Fmadds(Lf(kUp), s, Lf(m + 32))));
        store<u32>(sp + 132, Fb(Fmadds(Lf(kUp + 4), s, Lf(m + 36))));
        store<u32>(sp + 136, Fb(Fmadds(Lf(kUp + 8), s, Lf(m + 40))));
        Swap(ctx, m, sp + 128, m + 32);
    } else if (state == 1) {
        store<u32>(sp + 112, Fb(Adds(Lf(m + 16), Lf(kUp))));
        store<u32>(sp + 116, Fb(Adds(Lf(m + 20), Lf(kUp + 4))));
        store<u32>(sp + 120, Fb(Adds(Lf(m + 24), Lf(kUp + 8))));
        Swap(ctx, m, m + 16, sp + 112);
    } else if (state == 2) {
        Release(ctx, m);
    } else if (state == 4) {
        if (load<u8>(m + 136) != 0 && load<u32>(m + 96) != 0) {
            const u32 pos = load<u32>(m + 96) + 144;
            const f64 dxz = Lf(kMarkK + 4), top = Lf(kMarkK), dy = Lf(kMarkK + 8);
            const f64 px = Lf(pos), pz = Lf(pos + 8);
            const u32 in = sp + 160, out = sp + 272;
            store<u8>(in + 32, 0);
            store<u32>(in + 36, 0);
            store<u32>(in + 40, 0);
            store<u32>(out + 16, 0x3F800000u);
            store<u32>(out + 20, 0xFFFFFFFFu);
            store<u32>(out + 64, 0);
            store<u32>(out + 32, 0xFFFFFFFFu);
            store<u32>(out + 68, 0);
            const auto mk = LoadMasks3(kMarkK + 16);
            Stvx(in, RowSelW(pos, kMarkK, pos + 8, mk));
            store<u32>(in + 36, 13);
            Stvx(in + 16, RowSel(Fill4(static_cast<f32>(Adds(px, dxz))), Fill4(static_cast<f32>(Adds(top, dy))),
                                 Fill4(static_cast<f32>(Adds(pz, dxz))), mk));
            const u32 world = load<u32>(load<u32>(load<u32>(kGame) + 0x1281D4) + 4);
            call(ctx, HkWorld_CastRay92E2E8, {world, in, out});
            if (load<u32>(out + 68) != 0) {
                const f64 t = Lf(out + 16);
                const u32 p = sp + 144;
                store<u32>(p, Fb(Fmadds(t, dxz, px)));
                store<u32>(p + 8, Fb(Fmadds(t, dxz, pz)));
                store<u32>(p + 4, Fb(Adds(Fmadds(t, dy, top), Lf(kMarkK + 64))));
                const u32 own = load<u32>(m);
                if (load<u16>(own + 884) < load<u16>(own + 880)) {
                    call(ctx, Mat34_FromRotPos553B4C, {sp + 208, kIdentRot, p});
                    const u32 y = static_cast<u32>(call(ctx, Obj_SpawnHashMtx307CC8, {own, kDebrisHash, sp + 208}));
                    if (y != 0) {
                        store<u16>(y + 178, static_cast<u16>(load<u16>(y + 178) + 1));
                        u32 n = load<u16>(own + 884);
                        const u32 cap = load<u16>(own + 880);
                        if (n == cap) {
                            call(ctx, PtrVector_Reserve, {own + 876, cap << 1});
                            n = load<u16>(own + 884);
                        }
                        store<u32>(load<u32>(own + 876) + (n << 2), y);
                        const u32 c1 = load<u16>(own + 884) + 1u;
                        const u32 mx = load<u16>(own + 886);
                        store<u16>(own + 884, static_cast<u16>(c1));
                        store<u16>(own + 886, static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(c1 & 0xFFFF) ? mx : c1));
                    }
                }
            }
        }
        Release(ctx, m);
    }
}
