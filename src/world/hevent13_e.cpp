// World_HandleEvent callees, part 13e: World_SpawnUsers487670 (called by
// Loader_Start2DEA50), which fills the world's per-user object slots from an
// entry's spawn list.
#include "hevent13.h"

using namespace sly4;
using namespace sly4::hevent13;

namespace {
// Entry: +2416 Object*[9] spawn list; vtable +76 (e, u32 index) -> point hash.
constexpr u32 kEnSpawns = 2416, kEnVtPoint = 76;
constexpr u32 kEntryIndex325E70 = 0x00325E70;  // (Entry*, Object*) -> s32 index or -1 (name?)
constexpr u32 kSaveProfile = 0x000C9FC8;       // Save_Profile0C9FC8 (Object*) -> profile or 0 (symbols.txt)
constexpr u32 kProfileItem121B40 = 0x00121B40; // (profile, s32 k) -> u32 hash or 0 (name?)
constexpr u32 kRegistryNotify = 0x002E9BE4;    // Registry_Notify (u32 hash) (symbols.txt)
constexpr u32 kEntryIsNew = 0x002E9B78;        // Entry_IsNew2E9B78 (u32 hash) (symbols.txt)
constexpr u32 kWorld44BD74 = 0x0044BD74;       // () (name?)
constexpr u32 kDescAdd733964 = 0x00733964;     // (vector*, const 192-byte record*) (name?)
constexpr u32 kObjSetName = 0x000D9DE0;        // (Object*, const char*) (name?)
constexpr u32 kWorldUser0 = 0x00486FF4;        // (World*, 0, s32 user) (name?)
constexpr u32 kWorldUserN = 0x004865EC;        // (World*, u32 user, 1) (name?)
constexpr u32 kSingleton = 0x007D3B80;         // Singleton_011DF320_Get (symbols.txt); vtable +64
constexpr u32 kStringFormat = 0x005534D0, kStringCopy = 0x00553684;
constexpr u32 kNop = 0x0054EC18;
constexpr u32 kFmtControllable = 0x00AE91A8;   // "Player%02d_Controllable"

// Profile: +20 u32 flags (bit 3), +32 u8 (bit 2), +36/+40 f32, +136 vector of
// 192-byte records; vtable +28 spawn(profile, Desc*) -> Object*.
constexpr u32 kPfFlags = 20, kPfFlagItems = 0x8, kPfBits = 32, kPf36 = 36, kPf40 = 40, kPfRecords = 136, kPfVtSpawn = 28;
constexpr u32 kRecordSize = 192;
// Object: +188 u32 (bit 5 matches spawn 0x14120), +192 u32 flags (bit 3),
// +216 owner profile-like (+52 its spawn); vtable +40 release(o, bool).
constexpr u32 kObj188 = 188, kObj188Any = 0x20, kObjFlags = 192, kObjFlagItems = 0x8, kObjSpawnOwner = 216, kOwnerSpawn = 52;
constexpr u32 kObjVtRelease = 40;
constexpr u32 kSpawnAny = 0x14120;
constexpr u32 kW128164 = 0x128164, kObj5572 = 5572, kObj5572Bit = 0x04000000;
constexpr u32 kNumItems = 4;

// The spawn descriptor (on the stack at r1+176): +0 vtable, +4..+12 0, +16
// vector* of records (allocated on demand, 20 bytes), +32 Mat, +96 vec3, +112
// 0, +116 0, +120 profile, +124/+128 its +36/+40, +132 flags (bit 31 set,
// bit 30 clear, bit 29 profile +32 bit 2, bits 21..28 0xFF while records are
// added, the rest as the stack had them), +136 0, +140 char name[40].
constexpr u32 kDescVtable = 0x00B12260, kDescVec3 = 0x01164B30;
constexpr u32 kDsRecords = 16, kDsMat = 32, kDsVec = 96, kDs112 = 112, kDs116 = 116, kDsProfile = 120;
constexpr u32 kDs124 = 124, kDs128 = 128, kDsFlags = 132, kDs136 = 136, kDsName = 140, kDsNameSize = 40;
constexpr u32 kDsFlagOn = 0x80000000u, kDsFlagOff = 0x40000000u, kDsFlagBit = 0x20000000u, kDsFlagBusy = 0x1FE00000u;

// Frame layout as the original's (so its uninitialized words read the same stack).
constexpr u32 kFrame = 528, kFrMat = 112, kFrDesc = 176, kFrKey = 368, kFrVecA = 372, kFrVecB = 392;
constexpr u32 kVcData = 0, kVcCap = 4, kVcElem = 6, kVcCount = 8, kVcHigh = 10;

// Appends a u32 to a stack vector (the inlined push).
void push4(PPUContext& ctx, u32 v, u32 val) {
    const u32 cap = load<u16>(v + kVcCap);
    if (load<u16>(v + kVcCount) == cap) call(ctx, hevent8::h8fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFE});
    store<u32>(load<u32>(v + kVcData) + ((static_cast<u32>(load<u16>(v + kVcCount)) << 2) & 0x3FFFC), val);
    const u32 c1 = static_cast<u32>(load<u16>(v + kVcCount)) + 1;
    const u32 hw = load<u16>(v + kVcHigh);
    store<u16>(v + kVcCount, static_cast<u16>(c1));
    store<u16>(v + kVcHigh, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c1 & 0xFFFF) ? hw : c1));
}
bool contains(u32 v, u32 val) {
    for (u32 k = 0; k < load<u16>(v + kVcCount); ++k)
        if (load<u32>(load<u32>(v + kVcData) + 4 * k) == val) return true;
    return false;
}
}  // namespace

// void World_SpawnUsers487670(World* w, Entry* e) (name?): with e: (1) the
// item hashes (0x00121B40, k < 4) of every spawn's profile are notified and
// collected; (2) each spawn reuses a user object already standing for it
// (World +0x1280F0[j]) or is spawned by its profile (vt +28) at the entry's
// point for it, named "Player%02d_Controllable"; (3) the user slots take the
// new objects in order (named, 0x00486FF4 / 0x004865EC), old objects not
// reused have their new items registered (0x002E9B78) and are released (vt
// +40, 1, and 0 when replaced or left out); then the singleton's vt +64.
DECOMP(00487670, World_SpawnUsers487670) {
    const u32 w = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    const u32 mat = sp + kFrMat, desc = sp + kFrDesc, key = sp + kFrKey, va = sp + kFrVecA, vb = sp + kFrVecB;
    call(ctx, hevent9::h9fn::PtrVector_Construct, {va});
    store<u16>(va + kVcElem, 4);
    call(ctx, hevent9::h9fn::PtrVector_Construct, {vb});
    store<u16>(vb + kVcElem, 4);
    if (e != 0) {
        const u32 spawns = e + kEnSpawns;
        // (1) item hashes of the spawns' profiles.
        for (u32 i = 0; i < kWNumUsers; ++i) {
            const u32 p = load<u32>(spawns + 4 * i);
            if (p == 0) continue;
            const u32 prof = static_cast<u32>(call(ctx, kSaveProfile, {p}));
            if (prof == 0 || (load<u32>(prof + kPfFlags) & kPfFlagItems) == 0) continue;
            for (u32 k = 0; k < kNumItems; ++k) {
                const u32 h = static_cast<u32>(call(ctx, kProfileItem121B40, {prof, k}));
                call(ctx, kRegistryNotify, {h});
                if (h != 0) push4(ctx, vb, h);
            }
        }
        const u32 x = load<u32>(w + kW128164);
        if (x != 0 && (load<u32>(x + kObj5572) & kObj5572Bit) != 0) call(ctx, kWorld44BD74, {});
        // (2) the new user objects, in va.
        for (u32 i = 0; i < kWNumUsers; ++i) {
            const u32 p = load<u32>(spawns + 4 * i);
            if (p == 0) continue;
            s32 found = -1;
            for (u32 j = 0; j < kWNumUsers; ++j) {
                const u32 q = load<u32>(w + kWUsers + 4 * j);
                if (q == 0) continue;
                const u32 o = load<u32>(q + kObjSpawnOwner);
                const u32 sp2 = o != 0 ? load<u32>(o + kOwnerSpawn) : 0;
                if (sp2 == p || (p == kSpawnAny && (load<u32>(q + kObj188) & kObj188Any) != 0)) {
                    found = static_cast<s32>(j);
                    break;
                }
            }
            if (found >= 0) {
                push4(ctx, va, load<u32>(w + kWUsers + 4 * static_cast<u32>(found)));
                continue;
            }
            copy_identity(ctx, mat);
            call(ctx, kWorldSpawn486460, {w, i, mat});
            const s32 idx = static_cast<s32>(call(ctx, kEntryIndex325E70, {e, p}));
            if (idx != -1 && hevent::vcall(ctx, e, kEnVtPoint, {e, static_cast<u32>(idx)}) != 0) {
                const u32 hash = static_cast<u32>(hevent::vcall(ctx, e, kEnVtPoint, {e, static_cast<u32>(idx)}));
                const u32 pt = find_point(ctx, key, hash);
                if (pt != 0) copy_mat(ctx, mat, pt + kPointMatrix);
            }
            const u32 prof = static_cast<u32>(call(ctx, kSaveProfile, {p}));
            if (prof == 0) continue;
            const u32 name = static_cast<u32>(call(ctx, kStringFormat, {kFmtControllable, i + 1}));
            // The descriptor.
            const u32 old_flags = load<u32>(desc + kDsFlags);  // read before it is written (stack garbage)
            store<u32>(desc + 0, kDescVtable);
            store<u32>(desc + kDsRecords, 0);
            copy_mat(ctx, desc + kDsMat, mat);
            for (u32 k = 0; k < 12; ++k) ctx.fpr[1 + k] = hevent7::lfs_f64(mat + (k / 3) * 16 + (k % 3) * 4);
            for (u32 k = 0; k < 3; ++k) store<u32>(desc + kDsVec + 4 * k, load<u32>(kDescVec3 + 4 * k));
            store<u32>(desc + kDs112, 0);
            store<u32>(desc + kDs116, 0);
            store<u32>(desc + kDsProfile, prof);
            store<u32>(desc + kDs124, load<u32>(prof + kPf36));
            store<u32>(desc + kDs128, load<u32>(prof + kPf40));
            u32 f = (old_flags | kDsFlagOn) & ~kDsFlagOff;
            f = (f & ~kDsFlagBit) | (((static_cast<u32>(load<u8>(prof + kPfBits)) >> 2) & 1) << 29);
            store<u32>(desc + kDs136, 0);
            for (u32 o = 4; o <= 12; o += 4) store<u32>(desc + o, 0);
            store<u8>(desc + kDsName, 0);
            store<u32>(desc + kDsFlags, f | kDsFlagBusy);
            ctx.fpr[1] = hevent7::lfs_f64(prof + kPf40);
            ctx.fpr[2] = hevent7::lfs_f64(kDescVec3 + 4);
            if (name != 0) call(ctx, kStringCopy, {desc + kDsName, name, kDsNameSize});
            u32 recs = load<u32>(prof + kPfRecords);
            if (recs != 0 && load<u16>(recs + 8) != 0) {
                for (u32 k = 0, off = 0;; ++k, off += kRecordSize) {
                    const u32 n = recs != 0 ? load<u16>(recs + 8) : 0;
                    if (k >= n) break;
                    const u32 rec = load<u32>(recs) + off;
                    u32 vp = load<u32>(desc + kDsRecords);
                    if (vp == 0) {
                        const u32 a = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {20}));
                        store<u32>(desc + kDsRecords, a);
                        vp = a;
                        if (a != 0) {
                            call(ctx, hevent9::h9fn::PtrVector_Construct, {a});
                            store<u16>(a + kVcElem, static_cast<u16>(kRecordSize));
                            vp = load<u32>(desc + kDsRecords);
                        }
                    }
                    call(ctx, kDescAdd733964, {vp, rec});
                    recs = load<u32>(prof + kPfRecords);
                }
            }
            store<u32>(desc + kDsFlags, load<u32>(desc + kDsFlags) & ~kDsFlagBusy);
            const u32 obj = static_cast<u32>(hevent::vcall(ctx, prof, kPfVtSpawn, {prof, desc}));
            push4(ctx, va, obj);
            const u32 vp = load<u32>(desc + kDsRecords);
            store<u32>(desc + 0, kDescVtable);  // the descriptor's destructor
            if (vp != 0) {
                call(ctx, hevent::hfn::PtrVector_Clear, {vp});
                call(ctx, kNop, {vp});
                call(ctx, hevent7::h7fn::SmallFree, {load<u32>(desc + kDsRecords), 20});
            }
        }
        // (3) the user slots.
        for (u32 j = 0; j < kWNumUsers; ++j) {
            const u32 slot = w + kWUsers + 4 * j;
            const u32 old = load<u32>(slot);
            bool reused = false;
            bool release = false;
            if (old != 0) {
                reused = contains(va, old);
                if (!reused) {
                    if ((load<u32>(old + kObjFlags) & kObjFlagItems) != 0) {
                        const u32 c = load<u32>(old + kObjSpawnOwner);
                        for (u32 k = 0; k < kNumItems; ++k) {
                            const u32 h = static_cast<u32>(call(ctx, kProfileItem121B40, {c, k}));
                            if (h != 0 && !contains(vb, h)) call(ctx, kEntryIsNew, {h});
                        }
                    }
                    hevent::vcall(ctx, old, kObjVtRelease, {old, 1});
                    release = true;
                    store<u32>(slot, 0);
                }
            }
            if (j < load<u16>(va + kVcCount)) {
                const u32 nv = load<u32>(load<u32>(va + kVcData) + 4 * j);
                store<u32>(slot, nv);
                const u32 name = static_cast<u32>(call(ctx, kStringFormat, {kFmtControllable, j + 1}));
                call(ctx, kObjSetName, {nv, name});
                if (j == 0) {
                    call(ctx, kWorldUser0, {w, 0, 0});
                    release = false;
                } else {
                    call(ctx, kWorldUserN, {load<u32>(hevent::kCurrentWorld), j, 1});
                    release = !reused;
                }
            }
            if (old != 0 && release) hevent::vcall(ctx, old, kObjVtRelease, {old, 0});
        }
        const u32 s = static_cast<u32>(call(ctx, kSingleton, {}));
        hevent::vcall(ctx, s, 64, {s});
    }
    call(ctx, hevent::hfn::PtrVector_Clear, {vb});
    call(ctx, kNop, {vb});
    call(ctx, hevent::hfn::PtrVector_Clear, {va});
    call(ctx, kNop, {va});
}
