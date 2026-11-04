// Slot entries: the 128-byte records (table at *0x01193004) that kind-8 /
// kind-10 objects publish their render state in when slot entries are on.
#include <cmath>

#include "obj5.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj4;
using namespace sly4::obj5;

namespace {

// SlotEntry fields.
constexpr u32 kSeMatrix = 0;      // f32[4][4] rows (x/y/z of rows 0-3); the w words of rows 0-2 hold:
constexpr u32 kSeLight = 12;      //   obj +288 (AglLight*)
constexpr u32 kSe292 = 28;        //   obj +292
constexpr u32 kSeTopology = 44;   //   the class's current topology (or 0)
constexpr u32 kSeTopoF32 = 60;    // f32: that topology's f32 (class +116)
constexpr u32 kSeAnchor = 64;     // f32[4]: obj +240
constexpr u32 kSeHalfSize = 80;   // f32[3]: half the bounding box, grown by the skeleton root's offset
constexpr u32 kSeZoneMask = 92;   // u32: obj +324
constexpr u32 kSeOwner = 96;      // Obj*
constexpr u32 kSeRadiusSq = 100;  // f32: obj +312
constexpr u32 kSe104 = 104;       // f32: geom def +124 (name?)
constexpr u32 kSe108 = 108, kSe110 = 110;  // u16: geom def +120 / 100 * 65535 and +128 * 65535 (name?)
constexpr u32 kSeLod = 112;       // f32[3]: geom def +136 / +144 / +152 per level (name?)
constexpr u32 kSeFlags = 124;     // u16: see below
constexpr u32 kSe126 = 126;       // u8: obj +168's top byte
constexpr u32 kSeLodCount = 127;  // u8: geom def +112 (1 without one) (name?)

// kSeFlags bits.
constexpr u32 kSeVisible = 0x1, kSeHasClass = 0x2, kSe321Bit = 0x4, kSeSkinned = 0x8, kSe292Set = 0x10;
constexpr u32 kSeTopoBit = 0x80, kSe332 = 0x100;

constexpr u32 kIdentity = 0x01164BD0;      // f32[4][4] (name?)
constexpr u32 kSlotConsts = 0x00579C28;    // f32 {0.5, 100.0, 65535.0}
constexpr u32 kKindGeomDef = 20;
constexpr u32 Box_FromSphere = 0x00556F10; // (f32 box[8], const f32 sphere[4]): centre -/+ radius (name?)

// lvx: the quadword at ea & ~15.
inline void LoadQuad(u32 ea, f32 out[4]) {
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) out[i] = load<f32>(a + i * 4);
}

// The body both Alloc paths share: a fresh entry for o in `slot`.
void InitEntry(PPUContext& ctx, u32 o, u32 slot) {
    store<u16>(o + obj2::kObjWord284, static_cast<u16>(slot));
    const u32 e = load<u32>(obj3::kSlotTable) + ((slot << 7) & 0xFFFFFF80u);
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(e + row + k, load<u32>(kIdentity + row + k));
    for (u32 k = 0; k < 16; k += 4) store<u32>(e + kSeAnchor + k, load<u32>(cls::kDefaultSphere + k));
    store<u32>(e + kSeZoneMask, 0);
    store<u32>(e + kSeOwner, o);
    store<u32>(e + kSeRadiusSq, 0);
    store<u16>(e + kSeFlags, 0);
    store<u32>(e + kSeLight, 0);
    store<u32>(e + kSe292, 0);
    store<u32>(e + kSeTopology, 0);
    // r4 = 0x01164BC0, r5 = 0 here; SlotEntry_UpdatePeer reads r3 only.
    call(ctx, 0x0057A46C, {slot});
}

}  // namespace

// u32 SlotEntry_Alloc(Obj* o)
// Gives a kind-8 object the slot at the end of the first list (+4 of the
// table header, which then grows) and a kind-10 object the one at the
// start of the second (+6, which then shrinks): identity matrix, default
// sphere, owner o, everything else cleared, then the peer link. Other
// kinds: nothing. Returns the new +4 / +6 value (unmasked), as r3 is left.
DECOMP(0057A53C, SlotEntry_Alloc) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 kind = Kind(o);
    if (kind == 8) {
        InitEntry(ctx, o, load<u16>(kSlotTableBase + 4));
        const u32 n = load<u16>(kSlotTableBase + 4) + 1u;
        store<u16>(kSlotTableBase + 4, static_cast<u16>(n));
        ret(ctx, n);
    } else if (kind == 10) {
        InitEntry(ctx, o, load<u16>(kSlotTableBase + 6));
        const u32 n = load<u16>(kSlotTableBase + 6) - 1u;
        store<u16>(kSlotTableBase + 6, static_cast<u16>(n));
        ret(ctx, n);
    }
}

// void SlotEntry_Update(SlotEntry* e, Obj* o)
// Publishes o's state in its slot entry: the matrix (+80), anchor, zone
// mask, radius, the AglLight / +292 words and the flags (visible, has a
// class, +321 bit 2, skinned, +292 set, the class's topology bit (visible
// with +322 bits 0xC0), +332 or +164 bits 0xC0), the half size of the
// class box (of the box around the +240 sphere without a class), grown on
// every axis by the largest component of the skeleton root's offset under
// the matrix, and the geometry definition's LOD data (defaults without a
// geometry definition class). With the topology bit: the class's current
// topology and its f32.
DECOMP(00579C50, SlotEntry_Update) {
    const u32 e = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 cls = load<u32>(o + kObjClassPtr);
    const bool has_cls = cls != 0;
    const u32 flags = load<u32>(o + kObjFlags);
    const u32 b322 = load<u8>(o + kObjBits322);
    const bool visible = (flags & kObjFlagVisible) != 0;
    const bool topo = visible && (b322 & 0x80) != 0 && (b322 & 0x40) != 0;
    const bool b256 = ((flags >> 6) & 3) == 3 || load<u32>(o + 332) != 0;
    const u32 zmask = load<u32>(o + kObjZoneMask);
    const u32 rsq = load<u32>(o + kObjRadiusSq);
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(e + kSeMatrix + row + k, load<u32>(o + obj::kObjMatrix + 64 + row + k));
    for (u32 k = 0; k < 16; k += 4) store<u32>(e + kSeAnchor + k, load<u32>(o + kObjAnchor + k));
    store<u32>(e + kSeZoneMask, zmask);
    store<u32>(e + kSeRadiusSq, rsq);
    u32 f = (visible ? kSeVisible : 0) | (has_cls ? kSeHasClass : 0);
    if (((load<u8>(o + obj3::kObjByte321) >> 1) & 1) == 1) f |= kSe321Bit;
    const bool skinned = static_cast<u32>(Vcall0(ctx, o, kVfnGetSkeleton)) != 0;
    const u32 v292 = load<u32>(o + kObj292), light = load<u32>(o + kObjLight);
    const u32 cls2 = load<u32>(o + kObjClassPtr);
    if (skinned) f |= kSeSkinned;
    if (v292 != 0) f |= kSe292Set;
    if (topo) f |= kSeTopoBit;
    if (b256) f |= kSe332;
    store<u16>(e + kSeFlags, static_cast<u16>(f));
    store<u32>(e + kSe292, v292);
    store<u32>(e + kSeLight, light);
    f32 lo[3], hi[3];
    if (has_cls) {
        for (u32 i = 0; i < 3; ++i) {
            lo[i] = load<f32>(cls2 + kClassBoxMin + i * 4);
            hi[i] = load<f32>(cls2 + kClassBoxMax + i * 4);
        }
    } else {
        Stack frame(ctx, 48);  // sp+128: the box around the sphere (sp+112 is the vector spill)
        const u32 box = frame.addr() + 16;
        call(ctx, Box_FromSphere, {box, o + kObjAnchor});
        for (u32 i = 0; i < 3; ++i) {
            lo[i] = load<f32>(box + i * 4);
            hi[i] = load<f32>(box + 16 + i * 4);
        }
        f = load<u16>(e + kSeFlags);
    }
    const f32 half = load<f32>(kSlotConsts);
    f32 h[3];
    for (u32 i = 0; i < 3; ++i) h[i] = (hi[i] - lo[i]) * half;
    if ((f & kSeSkinned) != 0) {
        // The skeleton root's local offset (bone 0's matrix +48) under the
        // object's rotation rows (vmaddfp chain), its largest component
        // (fsel) added to every axis.
        const u32 sk = static_cast<u32>(Vcall0(ctx, o, kVfnGetSkeleton));
        const u32 t = load<u32>(load<u32>(sk + kSkelBones) + kSkelLocal) + 48;
        const f32 tx = load<f32>(t), ty = load<f32>(t + 4), tz = load<f32>(t + 8);
        f32 r0[4], r1[4], r2[4], v[4];
        LoadQuad(o + obj::kObjMatrix + 64, r0);
        LoadQuad(o + obj::kObjMatrix + 80, r1);
        LoadQuad(o + obj::kObjMatrix + 96, r2);
        for (u32 i = 0; i < 3; ++i) v[i] = std::fmaf(r2[i], tz, std::fmaf(r1[i], ty, std::fmaf(r0[i], tx, 0.0f)));
        f64 m = rt::fsel(static_cast<f64>(v[0] - v[1]), static_cast<f64>(v[1]), static_cast<f64>(v[0]));
        m = rt::fsel(static_cast<f64>(static_cast<f32>(static_cast<f64>(v[2]) - m)), m, static_cast<f64>(v[2]));
        const f32 mf = static_cast<f32>(m);
        for (u32 i = 0; i < 3; ++i) h[i] = h[i] + mf;
    }
    for (u32 i = 0; i < 3; ++i) store<f32>(e + kSeHalfSize + i * 4, h[i]);
    store<u32>(e + kSeLod + 4, 0);
    store<u8>(e + kSe126, load<u8>(o + 168));
    store<u32>(e + kSeLod + 8, 0);
    const u32 c = load<u32>(o + kObjClassPtr);
    if (!has_cls) {
        store<u32>(e + kSe104, 0);
        store<u16>(e + kSe108, 0);
        store<u16>(e + kSe110, 0);
        store<u8>(e + kSeLodCount, 1);
        store<u32>(e + kSeLod, cls::kOneF);
    } else if (Kind(c) != kKindGeomDef) {
        store<u32>(e + kSeLod, 0);
        store<u32>(e + kSe104, 0);
        store<u16>(e + kSe108, 0);
        store<u16>(e + kSe110, 0);
        store<u8>(e + kSeLodCount, 1);
        store<f32>(e + kSeLod, -load<f32>(load<u32>(c + 112) + 56));  // fneg
    } else {
        store<u32>(e + kSeLod, 0);
        store<u32>(e + kSe104, load<u32>(c + 124));
        const f32 k100 = load<f32>(kSlotConsts + 4), k65535 = load<f32>(kSlotConsts + 8);
        const f32 a = (load<f32>(c + 120) / k100) * k65535;
        store<u16>(e + kSe108, static_cast<u16>(rt::fctid(static_cast<f64>(a), true)));
        const f32 b = load<f32>(c + 128) * k65535;
        store<u16>(e + kSe110, static_cast<u16>(rt::fctid(static_cast<f64>(b), true)));
        const s32 n = load<u8>(c + 112);
        store<u8>(e + kSeLodCount, static_cast<u8>(n));
        if (n > 0) store<u32>(e + kSeLod, load<u32>(c + 136));
        if (n > 1) store<u32>(e + kSeLod + 4, load<u32>(c + 144));
        if (n > 2) store<u32>(e + kSeLod + 8, load<u32>(c + 152));
    }
    store<u32>(e + kSeTopology, 0);
    if (topo && c != 0) {
        store<u32>(e + kSeTopology, load<u32>(c + obj2::kClassTopos + ((static_cast<u32>(load<u8>(c + obj2::kClassTopoIndex)) << 3) & ~7u)));
        store<u32>(e + kSeTopoF32, load<u32>(c + 116));
    }
}
