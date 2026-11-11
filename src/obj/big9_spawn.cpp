// Game code, large-function sweep part 9: a flyer's (re)spawn on a spline
// picked at random from three names (0040DBA0).
#include "big9.h"

using namespace sly4;
using namespace sly4::big9;
using namespace sly4::big9::b9fn;

namespace {
constexpr u32 kRng = 0x01164888u;          // u64 +0 low 16 bits of the last value, u32 +8 seed
constexpr u32 kSplineHolder = 0x00F8F158u; // spline instance holder (+16 SplineMap)
constexpr u32 kProfileMap = 0x01013B1Cu;   // SortedMap: profile key -> live spawn params or 0
constexpr u32 kUp = 0x01164B60u;           // f32[3] the up axis
constexpr u32 kOne3 = 0x01164B30u;         // f32[3] (unit scale?)
constexpr u32 kZeroOne = 0x0040DB98u;      // .text: f32 0 (spline t), 1.0
constexpr u32 kParamsVt = 0x00B12260u;     // spawn params vtable
constexpr u32 kName = 0x00ADFBFCu;         // "Pterodactyl"
constexpr u32 kFallbackSpline = 0xE720C39Cu;
constexpr u32 kSplineFind = 0x007335C4u;   // SplineMap_Find (map, const u32* hash) -> slot
constexpr u32 kSortedFind = 0x00732F38u;   // SortedMap_Find732F38 (map, const u32* key) -> slot
constexpr u32 kSaveProfile = 0x000C9FC8u;  // Save_Profile0C9FC8 (key) -> profile or 0
constexpr u32 kSplineTangent = 0x005A80D8u;  // WorldObj_SplinePointTangentAt (spline, -, f32 pos[4], f32 tan[4], bool back; f1 t)
constexpr u32 kStrCopy = 0x00553684u;      // String_Copy (dst, src, size)
constexpr u32 kPtrVecConstruct = 0x005516E0u;  // PtrVector_Construct (v)
constexpr u32 kPtrVecClear = 0x0054EC1Cu;  // PtrVector_Clear (v)
constexpr u32 kNop = 0x0054EC18u;          // Nop (v)
constexpr u32 kVectorAdd = 0x00733964u;    // Vector_Add733964 (Vector*, const Rec* 192 bytes) -> s32
constexpr u32 kSmallFree = 0x00552420u;    // SmallFree (p, size)
// Object fields.
constexpr u32 kSpawned = 76;               // spawned object (+176 u16 flags: 0x1000 release) or 0
constexpr u32 kSpawned2 = 80;
constexpr u32 kSpline = 84;
constexpr u32 kPos = 112;                  // f32[4]
constexpr u32 kTan = 176;                  // f32[4]

inline f32 Wf(u32 w) { return std::bit_cast<f32>(w); }
}  // namespace

// u32 Flyer_Respawn(Flyer* o, const SpawnDef* d, Cfg* cfg)
// Releases the spawned objects at +76 / +80 (flag 0x1000), copies the 72
// def bytes into o, +72 = cfg, +88 / +92 = 0, +96 = 5.0; one of the def's
// spline names +24 / +28 / +32 picked by the RNG at 0x01164888 (seed % 3)
// gives +84 (when cfg +53 and +97 are both 0, the spline 0xE720C39C
// instead). With a spline, nothing spawned and a profile for the def's +48
// key: the spline's start point and tangent (t = 0) give a basis (up x
// tangent, tangent x that) for spawn params (vtable 0x00B12260, name
// "Pterodactyl", the profile's +136 records in a PtrVector) that go to the
// profile's live params when the key has some; profile vfunc +28 spawns
// (kept at +76), which gets vfunc +88 (1, 1, 1, 1) and +84 (the matrix).
DECOMP(0040DBA0, Flyer_Respawn) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = arg<u32>(ctx, 1);
    const u32 cfg = arg<u32>(ctx, 2);
    const u32 p = load<u32>(o + kSpawned);
    if (p != 0) {
        store<u16>(p + 176, static_cast<u16>(load<u16>(p + 176) | 0x1000u));
        const u32 p2 = load<u32>(o + kSpawned2);
        store<u32>(o + kSpawned, 0);
        if (p2 != 0) {
            store<u16>(p2 + 176, static_cast<u16>(load<u16>(p2 + 176) | 0x1000u));
            store<u32>(o + kSpawned2, 0);
        }
    }
    store<u32>(o + 88, 0);
    for (u32 k = 0; k < 72; k += 8) {
        const u32 w0 = load<u32>(d + k), w1 = load<u32>(d + k + 4);
        store<u32>(o + k, w0);
        store<u32>(o + k + 4, w1);
    }
    Stack frame(ctx, 288);  // the original's sp+112..+399
    const u32 sp = frame.addr() - 112;
    const u32 seed = load<u32>(kRng + 8);
    store<u32>(o + 72, cfg);
    store<u32>(sp + 380, load<u32>(d + 24));
    store<u32>(sp + 384, load<u32>(d + 28));
    const u32 n2 = load<u32>(d + 32);
    store<u32>(o + 92, 0);
    store<u32>(o + 96, 0x40A00000u);  // 5.0
    store<u32>(sp + 388, n2);
    const u32 x = (std::rotl(seed, 4) + std::rotl(seed, 16)) ^ 0x87654321u;
    store<u32>(kRng + 8, std::rotl(x, 16));
    store<u64>(kRng, x & 0xFFFFu);
    store<u32>(sp + 376, load<u32>(sp + 380 + 4 * (seed % 3)));
    u32 spl = load<u32>(static_cast<u32>(call(ctx, kSplineFind, {load<u32>(load<u32>(kSplineHolder) + 16), sp + 376})));
    const u32 c = load<u32>(o + 72);
    store<u32>(o + kSpline, spl);
    if (load<u8>(c + 53) == 0 && load<u8>(c + 97) == 0) {
        const u32 h = load<u32>(kSplineHolder);
        store<u32>(sp + 372, kFallbackSpline);
        spl = load<u32>(static_cast<u32>(call(ctx, kSplineFind, {load<u32>(h + 16), sp + 372})));
        store<u32>(o + kSpline, spl);
    }
    const u32 cur = load<u32>(o + kSpawned);
    if (spl == 0 || cur != 0) {
        ret(ctx, cur);
        return;
    }
    const u32 key = load<u32>(d + 48);
    if (key == 0) {
        ret(ctx, cur);
        return;
    }
    const u64 prof64 = call(ctx, kSaveProfile, {key});
    const u32 prof = static_cast<u32>(prof64);
    if (prof == 0) return;  // r3 = its result
    // r4: what Save_Profile left (the callee does not read it).
    callf(ctx, kSplineTangent, {load<u32>(o + kSpline), ctx.gpr[4], o + kPos, o + kTan, 0}, {std::bit_cast<f64>(LfsBits(load<u32>(kZeroOne)))});

    const u32 tw[3] = {load<u32>(o + kTan), load<u32>(o + kTan + 4), load<u32>(o + kTan + 8)};
    const f32 tx = Wf(tw[0]), ty = Wf(tw[1]), tz = Wf(tw[2]);
    const f32 ux = F(kUp), uy = F(kUp + 4), uz = F(kUp + 8);
    const f32 one = F(kZeroOne + 4);
    // s = up x tangent, normalised
    f32 sy = Fmsubs(uz, tx, ux * tz);
    f32 sx = Fmsubs(uy, tz, uz * ty);
    f32 sz = Fmsubs(ux, ty, uy * tx);
    f32 inv = one / std::sqrt(Fmadds(sz, sz, Fmadds(sx, sx, sy * sy)));
    sz = inv * sz;
    sx = inv * sx;
    sy = inv * sy;
    // v = tangent x s, normalised
    f32 vy = Fmsubs(tz, sx, tx * sz);
    f32 vz = Fmsubs(tx, sy, ty * sx);
    f32 vx = Fmsubs(ty, sz, tz * sy);
    inv = one / std::sqrt(Fmadds(vz, vz, Fmadds(vx, vx, vy * vy)));
    vx = inv * vx;
    vy = inv * vy;
    vz = inv * vz;
    const u32 sw[3] = {FBits(sx), FBits(sy), FBits(sz)}, vw[3] = {FBits(vx), FBits(vy), FBits(vz)};
    const u32 pw[3] = {load<u32>(o + kPos), load<u32>(o + kPos + 4), load<u32>(o + kPos + 8)};
    const u32 one3[3] = {load<u32>(kOne3), load<u32>(kOne3 + 4), load<u32>(kOne3 + 8)};
    // Matrix at sp+112 (rows s, v, tangent, position; w words left as
    // stack garbage) and spawn params at sp+176.
    const u32 mat = sp + 112, pr = sp + 176;
    for (u32 k = 0; k < 3; ++k) {
        store<u32>(mat + 4 * k, sw[k]);
        store<u32>(mat + 16 + 4 * k, vw[k]);
        store<u32>(mat + 32 + 4 * k, tw[k]);
        store<u32>(mat + 48 + 4 * k, pw[k]);
        store<u32>(pr + 32 + 4 * k, sw[k]);
        store<u32>(pr + 48 + 4 * k, vw[k]);
        store<u32>(pr + 64 + 4 * k, tw[k]);
        store<u32>(pr + 80 + 4 * k, pw[k]);
        store<u32>(pr + 96 + 4 * k, one3[k]);
    }
    store<u32>(pr, kParamsVt);
    for (u32 k = 4; k <= 16; k += 4) store<u32>(pr + k, 0);
    store<u32>(pr + 112, 0);
    store<u32>(pr + 116, 0);
    store<u32>(pr + 120, prof);
    store<u32>(pr + 124, load<u32>(prof + 36));
    const u32 f40 = load<u32>(prof + 40);
    store<u32>(pr + 128, f40);
    // Flags: bits 0x80000000 set, 0x40000000 clear, 0x20000000 = profile
    // +32 bit 2, 0x1FE00000 set; the low bits are whatever the stack held
    // there (the original reads sp+308 before writing it).
    u32 fl = (load<u32>(pr + 132) | 0x80000000u) & ~0x60000000u;
    fl |= ((static_cast<u32>(load<u8>(prof + 32)) >> 2) & 1u) << 29;
    fl |= 0x1FE00000u;
    store<u32>(pr + 136, 0);
    store<u8>(pr + 140, 0);
    store<u32>(pr + 132, fl);
    SetF1Bits(ctx, f40);
    call(ctx, kStrCopy, {pr + 140, kName, 40});

    u32 list = load<u32>(prof + 136);
    if (list != 0 && load<u16>(list + 8) != 0) {
        for (u32 i = 0, off = 0;; ++i, off += 192) {
            const u32 n = list != 0 ? load<u16>(list + 8) : 0u;
            if (i >= n) break;
            const u32 item = load<u32>(list) + off;
            u64 vec = load<u32>(pr + 16);
            if (vec == 0) {
                vec = call(ctx, kSmallAlloc, {20});
                store<u32>(pr + 16, static_cast<u32>(vec));
                if (static_cast<u32>(vec) != 0) {
                    call(ctx, kPtrVecConstruct, {vec});
                    store<u16>(static_cast<u32>(vec) + 6, 192);
                    vec = load<u32>(pr + 16);
                }
            }
            call(ctx, kVectorAdd, {vec, item});
            list = load<u32>(prof + 136);
        }
    }
    store<u32>(sp + 368, key);
    const u32 live = load<u32>(static_cast<u32>(call(ctx, kSortedFind, {kProfileMap, sp + 368})));
    u32 arg = pr;
    if (live != 0) {
        store<u32>(live + 112, load<u32>(pr + 112));
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) store<u32>(live + 32 + 16 * r + 4 * k, load<u32>(pr + 32 + 16 * r + 4 * k));
        store<u32>(live + 96, load<u32>(pr + 96));
        store<u32>(live + 100, load<u32>(pr + 100));
        store<u32>(live + 120, prof);
        const u32 w104 = load<u32>(pr + 104);
        store<u32>(live + 104, w104);
        SetF1Bits(ctx, w104);
        arg = live;
    }
    const u64 obj = VCall(ctx, prof, 28, {prof, arg});
    store<u32>(o + kSpawned, static_cast<u32>(obj));
    if (static_cast<u32>(obj) != 0) {
        VCall(ctx, static_cast<u32>(obj), 88, {obj, 1, 1, 1, 1});
        const u32 s = load<u32>(o + kSpawned);
        VCall(ctx, s, 84, {s, mat});
    }
    const u32 vec = load<u32>(pr + 16);
    if (vec == 0) return;  // r3 as the last call left it
    store<u32>(pr, kParamsVt);
    call(ctx, kPtrVecClear, {vec});
    call(ctx, kNop, {vec});
    call(ctx, kSmallFree, {load<u32>(pr + 16), 20});
}
