// World_HandleEvent callees, part 18i: the player's weapon (outfit) swap.
#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
// Player +340: the attach vector (240-byte records: +160 u32 tag, +192
// Object*). The weapon's record has tag 0x03D69F9C.
constexpr u32 kPlAttach = 340, kAttachRecSize = 240, kRecTag = 160, kRecObject = 192;
constexpr u32 kWeaponTag = 0x03D69F9C;
constexpr u32 kPlVtName = 76, kObjVtEnable = 40;
constexpr u32 kFmtWeapon = 0x00ACA844;  // "%s_Weapon"
constexpr u32 kSaveProfile0C9FC8 = 0x000C9FC8;  // (u32 hash) -> Profile* (hevent6_b.cpp)
constexpr u32 kObjDetach0D72C0 = 0x000D72C0;    // (holder, Object*) (hevent6_e.cpp)
constexpr u32 kObjAttach0D15C0 = 0x000D15C0;    // (Object*, const AttachDesc*) (name?)
constexpr u32 kStringFormat = 0x005534D0;       // (fmt, ...) -> static buffer (symbols.txt)
constexpr u32 kVectorAdd733964 = 0x00733964;    // (Vector*, const Rec* 192 bytes) (hevent18_f.cpp)
constexpr u32 kNop54EC18 = 0x0054EC18;

// Profile: +32 u8 flags (bit 2 0x4), +36 / +40 f32, +136 Vector* of
// 192-byte records; vt +28 spawn(prof, Query*) -> Object*.
constexpr u32 kProfFlags = 32, kProfA = 36, kProfB = 40, kProfRecs = 136, kProfVtSpawn = 28;
constexpr u32 kProfRecSize = 192;

// The original's 624-byte frame: the spawn query at r1 +112 (to +292) and
// the attach descriptor at r1 +304 (240 bytes). Both read a flags word
// before writing it (query +132, descriptor +176), so the frame is kept at
// the original's size and those words are the same stack words.
constexpr u32 kFrame = 624, kQueryAt = 112, kDescAt = 304;
// Query: +0 vtable 0x00B12260, +4..+12 0, +16 PtrVector* (192-byte records,
// step 192), +32 3x4 matrix (identity), +96 vec3 (0x01164B30), +112 / +116
// 0, +120 Profile*, +124 / +128 f32 from the profile, +132 flags (bit 31
// set, bit 30 clear, bit 29 profile flag bit 2, bits 21..28 set), +136 0,
// +140 char name[40].
constexpr u32 kQueryVtable = 0x00B12260, kQueryRecs = 16, kQueryMatrix = 32, kQueryPos = 96;
constexpr u32 kQueryProf = 120, kQueryA = 124, kQueryB = 128, kQueryFlags = 132, kQueryName = 140, kQueryNameSize = 40;
constexpr u32 kZeroVec4 = 0x01164B30, kIdentityQuat = 0x01164C80;
// Attach descriptor (as Main_Refresh42685C).
constexpr u32 kDescPos = 128, kDescRot = 144, kDescTag = 160, kDescFlags = 176, kDescVec = 180, kDescObject = 192;

void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// Frees a descriptor's PtrVector held at slot.
void free_vec(PPUContext& ctx, u32 slot) {
    const u32 v = load<u32>(slot);
    if (v == 0) return;
    call(ctx, hfn::PtrVector_Clear, {v});
    call(ctx, kNop54EC18, {v});
    call(ctx, hevent7::h7fn::SmallFree, {load<u32>(slot), 20});
}
}  // namespace

// void Player_SetOutfit11F32C(Player* p, u32 hash) (name?): detaches and
// disables the current weapon object, then (for a known profile) spawns the
// profile's "<name>_Weapon" object from a query carrying the profile's
// records, and attaches it with the weapon tag.
DECOMP(0011F32C, Player_SetOutfit11F32C) {
    const u32 p = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1);
    const u32 v = load<u32>(p + kPlAttach);
    for (u32 i = 0; i < (v != 0 ? load<u16>(v + 8) : 0u); ++i) {
        const u32 rec = load<u32>(v) + kAttachRecSize * i;
        if (load<u32>(rec + kRecTag) != kWeaponTag) continue;
        const u32 obj = load<u32>(rec + kRecObject);
        if (obj != 0) {
            call(ctx, kObjDetach0D72C0, {p, obj});
            vcall(ctx, obj, kObjVtEnable, {obj, 1});
        }
        break;
    }
    const u32 prof = static_cast<u32>(call(ctx, kSaveProfile0C9FC8, {hash}));
    if (prof == 0) return;
    const u32 name = static_cast<u32>(vcall(ctx, p, kPlVtName, {p}));
    const u32 str = static_cast<u32>(call(ctx, kStringFormat, {kFmtWeapon, name}));

    Stack frame(ctx, kFrame);
    const u32 q = frame.addr() + kQueryAt, d = frame.addr() + kDescAt;
    u32 qf = load<u32>(q + kQueryFlags);
    store<u32>(q + kQueryRecs, 0);
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 12; c += 4) copy32(q + kQueryMatrix + 16 * r + c, hevent13::kIdentity + 16 * r + c);
    store<u32>(q + 0, kQueryVtable);
    for (u32 c = 0; c < 12; c += 4) copy32(q + kQueryPos + c, kZeroVec4 + c);
    qf = (qf | 0x80000000u) & ~0x40000000u;
    store<u32>(q + 112, 0);
    copy32(q + kQueryA, prof + kProfA);
    store<u32>(q + 116, 0);
    store<u32>(q + kQueryProf, prof);
    copy32(q + kQueryB, prof + kProfB);
    store<u32>(q + kQueryFlags, qf);
    const u32 bit = (load<u8>(prof + kProfFlags) >> 2) & 1;
    qf = (qf & ~0x20000000u) | (bit << 29);
    store<u32>(q + 136, 0);
    store<u32>(q + 4, 0);
    store<u32>(q + 8, 0);
    store<u32>(q + 12, 0);
    store<u8>(q + kQueryName, 0);
    store<u32>(q + kQueryFlags, qf | 0x1FE00000u);
    ctx.fpr[1] = hevent7::lfs_f64(prof + kProfB);  // f1 / f2 as the copies leave them
    ctx.fpr[2] = hevent7::lfs_f64(kZeroVec4 + 4);
    if (str != 0) call(ctx, hevent16::h16fn::String_Copy, {q + kQueryName, str, kQueryNameSize});

    for (u32 i = 0;; ++i) {
        const u32 recs = load<u32>(prof + kProfRecs);
        if (recs == 0 || i >= load<u16>(recs + 8)) break;
        const u32 rec = load<u32>(recs) + kProfRecSize * i;
        u32 vec = load<u32>(q + kQueryRecs);
        if (vec == 0) {
            vec = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {20}));
            store<u32>(q + kQueryRecs, vec);
            if (vec != 0) {
                call(ctx, hevent9::h9fn::PtrVector_Construct, {vec});
                store<u16>(vec + kPvStep, kProfRecSize);
                vec = load<u32>(q + kQueryRecs);
            }
        }
        call(ctx, kVectorAdd733964, {vec, rec});
    }

    const u32 obj = static_cast<u32>(vcall(ctx, prof, kProfVtSpawn, {prof, q}));
    if (obj != 0) {
        const u32 df = load<u32>(d + kDescFlags) & 0x00FFFFFFu;
        for (u32 c = 0; c < 12; c += 4) copy32(d + kDescPos + c, hevent7::kZeroVec + c);
        for (u32 c = 0; c < 16; c += 4) copy32(d + kDescRot + c, kIdentityQuat + c);
        store<u32>(d + 164, 0);
        store<u32>(d + 168, 0);
        store<u32>(d + 172, 0);
        store<u32>(d + kDescVec, 0);
        store<u8>(d + 0, 0);
        store<u8>(d + 64, 0);
        store<u32>(d + 196, 0);
        store<u32>(d + 200, 0xFFFFFFFFu);
        store<u32>(d + 204, 0);
        store<u32>(d + 208, 0);
        store<u32>(d + 212, 0xFFFFFFFFu);
        store<u32>(d + 216, 0);
        store<u32>(d + 220, 0);
        store<u32>(d + 224, 0);
        store<u32>(d + kDescObject, obj);
        store<u32>(d + kDescTag, kWeaponTag);
        store<u32>(d + kDescFlags, df | 0x04000000u);
        ctx.fpr[1] = hevent7::lfs_f64(kIdentityQuat + 12);
        ctx.fpr[2] = hevent7::lfs_f64(kIdentityQuat + 8);
        call(ctx, kObjAttach0D15C0, {p, d});
        free_vec(ctx, d + kDescVec);
    }
    // The query's destructor: vtable reset, records freed.
    if (load<u32>(q + kQueryRecs) != 0) store<u32>(q + 0, kQueryVtable);
    free_vec(ctx, q + kQueryRecs);
}
