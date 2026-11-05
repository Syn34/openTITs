// World_HandleEvent callees, part 28c: spawning the players at level start
// (World_SpawnPlayers486854).
#include "hevent28.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;
using namespace sly4::hevent28;

namespace {
namespace cfn {
constexpr u32 Save_Profile = 0x000C9FC8;        // (save obj) -> Profile* (Save_Profile0C9FC8)
constexpr u32 Profile_Item = 0x00121B40;        // (profile, u32 k) -> item (Profile_Item121B40)
constexpr u32 Registry_Notify = 0x002E9BE4;     // (item)
constexpr u32 Callbacks_Bind = 0x0044BD74;      // () (Callbacks_Bind44BD74)
constexpr u32 World_SpawnPoint = 0x00486460;    // (World*, u32 user, Mat* out) (World_SpawnPoint486460)
constexpr u32 User_SetControl = 0x004865EC;     // (World*, u32 user, u8 enable) (User_SetControl4865EC)
constexpr u32 User_Travel = 0x00484BE0;         // (World*, UserBlock*, level, Mat*, 0, 0, 0, 0, stack 8) (User_Travel484BE0)
constexpr u32 PointMap_Find = 0x0073343C;       // (map, const u32* key) -> value slot
constexpr u32 String_Format = 0x005534D0;       // (fmt, ...) -> static buffer
}  // namespace cfn

constexpr u32 kWorldUsers = 29;                 // u8 user count
constexpr u32 kWorldObj = 0x128164;             // u32 object (+0 -> u16 +8 count, +5572 flags) (as event.h)
constexpr u32 kWorldSpawned = 0x1280F0;         // u32[9] spawned player objects
constexpr u32 kSessionHolder = 0x00F64C20;      // +72 session (+20 flags, +2416 u32 save slots[9], vt +76 (s, i) -> point key)
constexpr u32 kSessionSaves = 2416, kSessionSlots = 9;
constexpr u32 kSessionVtPoint = 76;
constexpr u32 kPointHolder = 0x00F8F154;        // +16 PointMap (as hevent13.h); point +16 4x3 matrix
constexpr u32 kIdentity43 = 0x01164BD0;         // 4 rows of 3 f32 (stride 16) (as construct.h)
constexpr u32 kZeroVec = 0x01164B30;
constexpr u32 kPlayerFmt = 0x00AE91A8;          // "Player%02d_Controllable"
constexpr u32 kBlockLevel = 12, kBlockIndex = 2, kBlockMatrix = 16, kBlockFlags = 0, kBlockF100 = 100, kBlockF108 = 108;
// Spawn query (as SpawnQueue_Update4AE498).
constexpr u32 kQueryVtable = 0x00B12260;
constexpr u32 kQueryRecs = 16, kQueryFlags = 132, kQueryName = 140, kQueryNameSize = 40;
constexpr u32 kProfRecs = 136, kProfRecSize = 192, kProfSpawnVirtual = 28;
// The original's 512-byte frame: matrix at r1 +128, query at r1 +192 (its
// flags word is read before written), point keys at r1 +384 / +388.
constexpr u32 kFrame = 512, kMatAt = 128, kQueryAt = 192, kKeyUserAt = 384, kKeyPlayerAt = 388;

void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
// Copies a 4x3 matrix (rows of three words, stride 16).
void copy43(u32 dst, u32 src) {
    for (u32 row = 0; row < 4; ++row)
        for (u32 c = 0; c < 12; c += 4) copy32(dst + 16 * row + c, src + 16 * row + c);
}

// The session's spawn point for slot i, if it names one: its matrix (+16)
// copied to dst (f1 = its last word, as the copy leaves it).
void session_point(PPUContext& ctx, u32 s, u32 i, u32 keyp, u32 dst) {
    if (vcall(ctx, s, kSessionVtPoint, {s, i}) == 0) return;
    store<u32>(keyp, static_cast<u32>(vcall(ctx, s, kSessionVtPoint, {s, i})));
    const u32 slot = static_cast<u32>(call(ctx, cfn::PointMap_Find, {load<u32>(load<u32>(kPointHolder) + 16), keyp}));
    const u32 pt = load<u32>(slot);
    if (pt == 0) return;
    copy43(dst, pt + 16);
    ctx.fpr[1] = lf(pt + 16 + 56);
}
}  // namespace

// u8 World_SpawnPlayers486854(World* w) (name?): with the session's
// controllable flag (+20 bit 5) and players in w +0x128164: notifies the
// four items of the profile in save slot 0 for every used slot (as the
// original), binds callbacks (+5572 bit 26), then for every used slot j
// spawns "Player%02d_Controllable" (profile vt +28 with a query at the
// spawn point, or the session's point for j) from the profile of the
// next player's slot, stores it at w +0x1280F0 and gives it control off.
// Then every user block is placed (World_SpawnPoint, or the session's
// point) and travels (User_Travel), +0 |= 0x40, +108 = +100. Returns the
// user count (u8 w +29).
DECOMP(00486854, World_SpawnPlayers486854) {
    const u32 w = arg<u32>(ctx, 0);
    Stack frame(ctx, kFrame);
    const u32 mat = frame.addr() + kMatAt, q = frame.addr() + kQueryAt;
    const u32 obj = load<u32>(w + kWorldObj);
    if (obj != 0 && load<u32>(obj) != 0 && load<u16>(load<u32>(obj) + 8) != 0) {
        const u32 s = load<u32>(load<u32>(kSessionHolder) + 72);
        if (s != 0 && (load<u32>(s + 20) & 0x20u) != 0) {
            const u32 saves = s + kSessionSaves;
            for (u32 i = 0; i < kSessionSlots; ++i) {
                if (load<u32>(saves + 4 * i) == 0) continue;
                const u32 prof = static_cast<u32>(call(ctx, cfn::Save_Profile, {load<u32>(saves)}));  // slot 0 (as the original)
                if (prof == 0 || (load<u32>(prof + 20) & 0x8u) == 0) continue;
                for (u32 k = 0; k < 4; ++k) call(ctx, cfn::Registry_Notify, {call(ctx, cfn::Profile_Item, {prof, k})});
            }
            const u32 o = load<u32>(w + kWorldObj);
            if (o != 0 && (load<u32>(o + 5572) & 0x04000000u) != 0) call(ctx, cfn::Callbacks_Bind, {1});
            u32 player = 0;
            for (u32 j = 0; j < kSessionSlots; ++j) {
                if (load<u32>(saves + 4 * j) == 0) continue;
                copy43(mat, kIdentity43);
                ctx.fpr[1] = lf(kIdentity43 + 52);
                call(ctx, cfn::World_SpawnPoint, {w, player, mat});
                session_point(ctx, s, j, frame.addr() + kKeyPlayerAt, mat);
                const u32 prof = static_cast<u32>(call(ctx, cfn::Save_Profile, {load<u32>(saves + 4 * player)}));
                if (prof == 0) continue;
                const u32 next = player + 1;
                const u32 name = static_cast<u32>(call(ctx, cfn::String_Format, {kPlayerFmt, next}));
                store<u32>(q + 0, kQueryVtable);
                store<u32>(q + kQueryRecs, 0);
                const u32 old = load<u32>(q + kQueryFlags);  // stack word read before written (as the original)
                copy43(q + 32, mat);
                for (u32 c = 0; c < 12; c += 4) copy32(q + 96 + c, kZeroVec + c);
                store<u32>(q + 112, 0);
                store<u32>(q + 116, 0);
                store<u32>(q + 120, prof);
                copy32(q + 124, prof + 36);
                copy32(q + 128, prof + 40);
                u32 flags = (old | 0x80000000u) & ~0x40000000u;
                flags = (flags & ~0x20000000u) | (((load<u8>(prof + 32) >> 2) & 1u) << 29);
                store<u32>(q + 136, 0);
                store<u32>(q + 4, 0);
                store<u32>(q + 8, 0);
                store<u32>(q + 12, 0);
                store<u8>(q + kQueryName, 0);
                store<u32>(q + kQueryFlags, flags | 0x1FE00000u);
                ctx.fpr[1] = lf(prof + 40);
                if (name != 0) call(ctx, h28fn::String_Copy, {q + kQueryName, name, kQueryNameSize});
                for (u32 k = 0;; ++k) {
                    const u32 recs = load<u32>(prof + kProfRecs);
                    if (k >= (recs != 0 ? static_cast<u32>(load<u16>(recs + 8)) : 0u)) break;
                    const u32 rec = load<u32>(recs) + k * kProfRecSize;
                    u32 v = load<u32>(q + kQueryRecs);
                    if (v == 0) {
                        v = static_cast<u32>(call(ctx, h28fn::SmallAlloc, {20}));
                        store<u32>(q + kQueryRecs, v);
                        if (v != 0) {
                            call(ctx, h28fn::PtrVector_Construct, {v});
                            store<u16>(v + 6, kProfRecSize);
                            v = load<u32>(q + kQueryRecs);
                        }
                    }
                    call(ctx, h28fn::Vector_Add, {v, rec});  // also with a failed allocation (as the original)
                }
                store<u32>(q + kQueryFlags, load<u32>(q + kQueryFlags) & ~0x1FE00000u);
                const u32 spawned = static_cast<u32>(vcall(ctx, prof, kProfSpawnVirtual, {prof, q}));
                store<u32>(w + kWorldSpawned + 4 * player, spawned);
                call(ctx, cfn::User_SetControl, {w, player, 0});
                store<u32>(q + 0, kQueryVtable);
                const u32 v = load<u32>(q + kQueryRecs);
                if (v != 0) {
                    call(ctx, h28fn::PtrVector_Clear, {v});
                    call(ctx, h28fn::Nop, {v});
                    call(ctx, h28fn::SmallFree, {load<u32>(q + kQueryRecs), 20});
                }
                player = next;
            }
        }
    }

    const u32 s = load<u32>(load<u32>(kSessionHolder) + 72);
    u32 n;
    for (u32 i = 0; i < (n = load<u8>(w + kWorldUsers)); ++i) {
        const u32 blk = w + kUserBlocks + i * kUserBlockSize;
        const u32 m = blk + kBlockMatrix;
        store<u8>(blk + kBlockIndex, static_cast<u8>(i));
        call(ctx, cfn::World_SpawnPoint, {w, i & 0xFFu, m});
        if (s != 0 && load<u32>(s + kSessionSaves + 4 * i) != 0) session_point(ctx, s, i, frame.addr() + kKeyUserAt, m);
        call(ctx, cfn::User_Travel, {w, blk, load<u32>(blk + kBlockLevel), m, 0, 0, 0, 0, 0});
        const u32 f = load<u32>(blk + kBlockFlags);
        ctx.fpr[1] = lf(blk + kBlockF100);
        store<u32>(blk + kBlockF108, load<u32>(blk + kBlockF100));
        store<u32>(blk + kBlockFlags, f | 0x40u);
    }
    ctx.gpr[3] = n;
}
