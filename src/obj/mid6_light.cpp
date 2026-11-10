// Game code, medium sweep part 6: the volume transform getter and the
// light record initializer that LightObj_Ctor_00224448 uses.
#include "mid6.h"

using namespace sly4;
using namespace sly4::mid6;

namespace {

namespace lfn {
constexpr u32 PtrVector_Construct = 0x005516E0;  // (v)
constexpr u32 PtrVector_Init = 0x00551598;       // (v, n, 0, 1)
constexpr u32 PtrVector_Reserve = 0x00551718;    // (v, capacity)
constexpr u32 SmallAlloc = 0x0055238C;           // (size) -> p or 0
constexpr u32 Array_Reserve = 0x00558F3C;        // (Array*, u16 capacity)
constexpr u32 SortedMap_Find = 0x00788D3C;       // (map, const u32* key) -> value slot
constexpr u32 Matrix_IsScaled = 0x005544FC;      // (const f32 m[4][4], f32 eps) -> bool
}  // namespace lfn

constexpr u32 kIdentity = 0x01164BD0;       // f32[4][4]
constexpr u32 kHalf224 = 0x0022416C;        // f32 0.5 just before Volume_GetTransform
constexpr u32 kVtLight = 0x00B21BF0;
constexpr u32 kLightVecCap = 0x00F65F70;    // u32: the capacity of the record's two PtrVectors
constexpr u32 kGroupVecCap = 0x00F65F6C;    // u32: the capacity of a group's PtrVector
constexpr u32 kLightGroups = 0x010C1D50;    // sorted map {+0 u16 cap, +2 u16 count, +4 u16 high-water, +6 u16 flags, +8 {u32 key, Group*}[]}
constexpr u32 kScaleEps = 0x004ACD64;       // f32 1e-5 just before Light_Init

// The inlined PtrVector constructor: construct, +6 = 4, Init(v, cap, 0, 1).
void PtrVecNew4(PPUContext& ctx, u32 v, u32 cap) {
    call(ctx, lfn::PtrVector_Construct, {v});
    store<u16>(v + 6, 4);
    call(ctx, lfn::PtrVector_Init, {v, cap, 0, 1});
}

// PushBack on a u32 PtrVector {+0 data, +4 u16 capacity, +8 u16 count,
// +10 u16 high-water mark}.
void PtrVecPush4(PPUContext& ctx, u32 v, u32 value) {
    u32 c = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (c == cap) {
        call(ctx, lfn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
        c = load<u16>(v + 8);
    }
    store<u32>(load<u32>(v) + ((c << 2) & 0x3FFFCu), value);
    const u32 n = load<u16>(v + 8) + 1u;
    const u32 m = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n & 0xFFFF) ? m : n));
}

}  // namespace

// void Volume_GetTransform_00224170(Volume* o, f32 m[4][4])
// m (rows 0..3, xyz) = the identity; then when the Havok entity o->+416 is
// in a world, by its vfn[7] kind: 1 / 2 the entity's 3x4 transform at +160
// copied in, 0 the translation row = the centre of the two vectors at +160
// (an AABB). (Named Obj_SetMatrix_00224170 before; it writes m.)
DECOMP(00224170, Volume_GetTransform_00224170) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = arg<u32>(ctx, 1);
    Stack frame(ctx, 224);  // the original's frame: the 3x4 copy goes through +144..+207
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(m + 16 * row + 4 * k, load<u32>(kIdentity + 16 * row + 4 * k));
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kIdentity + 56)));  // the lfs/stfs pairs' leftover
    const u32 ent = load<u32>(o + 416);
    if (ent == 0) return;  // r3 = o
    const u32 world = load<u32>(ent + 8);
    if (world == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const s32 kind = static_cast<s32>(VCall(ctx, ent, 28, {ent}));
    const u32 t = ent + 160;
    if (kind > 0) {
        if (kind > 2) return;
        const u32 tmp = frame.addr() + 144;
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k) store<u32>(tmp + 16 * row + 4 * k, load<u32>(t + 16 * row + 4 * k));
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k) store<u32>(m + 16 * row + 4 * k, load<u32>(tmp + 16 * row + 4 * k));
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(tmp + 56)));
        ctx.gpr[3] = tmp + 56;  // leftover
    } else if (kind == 0) {
        const u32 b = t & ~15u;  // lvx of the two vectors
        const f32 h = F32(load<u32>(kHalf224));
        f32 x = 0;
        for (u32 k = 0; k < 3; ++k) {
            const f32 v = (F32(load<u32>(b + 4 * k)) + F32(load<u32>(b + 16 + 4 * k))) * h;
            if (k == 0) x = v;
            store<u32>(m + 48 + 4 * k, W32(v));
        }
        ctx.fpr[1] = static_cast<f64>(x);
        ctx.gpr[3] = m + 48;  // leftover
    }
}

// bool Light_Init_004ACD68(Light* p, const LightRec* r)
// Copies the 105-byte record r into the 176-byte light p (r->+0 / +4 to
// +16 / +20, r's f32 rows +16.. to +32.., +120 the byte r->+104), the
// vtable, +6 = 1, two PtrVectors at +128 / +148, +172 = 1, +168 = 0. The
// light then joins the group keyed by its +16 hash in the global group map
// (a new 24-byte group {+0 key, +4 PtrVector} is made and appended when
// there is none and the key is not 0; +168 = the group). Returns
// Matrix_IsScaled(p->+32, 1e-5).
DECOMP(004ACD68, Light_Init_004ACD68) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 r = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);  // the map key
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 32 + 16 * row + 4 * k, load<u32>(r + 16 + 16 * row + 4 * k));
    store<u32>(p + 80, load<u32>(r + 64));
    store<u32>(p + 16, load<u32>(r));
    store<u32>(p + 84, load<u32>(r + 68));
    store<u16>(p + 6, 1);
    store<u32>(p, kVtLight);
    store<u32>(p + 88, load<u32>(r + 72));
    store<u32>(p + 20, load<u32>(r + 4));
    for (u32 k = 0; k < 5; ++k) store<u32>(p + 96 + 4 * k, load<u32>(r + 80 + 4 * k));
    const u32 w100 = load<u32>(r + 100);
    const u8 b104 = load<u8>(r + 104);
    store<u32>(p + 116, w100);
    store<u8>(p + 120, b104);

    PtrVecNew4(ctx, p + 128, load<u32>(kLightVecCap));
    PtrVecNew4(ctx, p + 148, load<u32>(kLightVecCap));
    store<u8>(p + 172, 1);
    const u32 key = frame.addr();
    store<u32>(key, load<u32>(p + 16));
    store<u32>(p + 168, 0);
    u32 group = load<u32>(static_cast<u32>(call(ctx, lfn::SortedMap_Find, {kLightGroups, key})));
    if (group == 0) {
        const u32 h = load<u32>(p + 16);
        if (h != 0) {
            const u32 g = static_cast<u32>(call(ctx, lfn::SmallAlloc, {24}));
            group = 0;
            if (g != 0) {
                store<u32>(g, h);
                PtrVecNew4(ctx, g + 4, load<u32>(kGroupVecCap));
                group = g;
            }
            // append {h, group} (no re-sort; flags |= 1)
            u32 c = load<u16>(kLightGroups + 2);
            const u32 cap = load<u16>(kLightGroups);
            if (c == cap) {
                call(ctx, lfn::Array_Reserve, {kLightGroups, (cap << 1) & 0xFFFEu});
                c = load<u16>(kLightGroups + 2);
            }
            const u32 data = load<u32>(kLightGroups + 8);
            store<u32>(data + ((c << 3) & 0x7FFF8u), h);
            store<u32>(data + (load<u16>(kLightGroups + 2) << 3) + 4, group);
            const u32 n = load<u16>(kLightGroups + 2) + 1u;
            const u32 hw = load<u16>(kLightGroups + 4);
            const u32 fl = load<u16>(kLightGroups + 6);
            store<u16>(kLightGroups + 2, static_cast<u16>(n));
            store<u16>(kLightGroups + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n & 0xFFFF) ? hw : n));
            store<u16>(kLightGroups + 6, static_cast<u16>(fl | 1));
        }
    }
    if (group != 0) {
        PtrVecPush4(ctx, group + 4, p);
        store<u32>(p + 168, group);
    }
    callf(ctx, lfn::Matrix_IsScaled, {p + 32}, {std::bit_cast<f64>(LfsBits(load<u32>(kScaleEps)))});
}
