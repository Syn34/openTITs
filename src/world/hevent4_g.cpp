// World_HandleEvent callees, part 4: 140..141 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent4g {
namespace gfn {
constexpr u32 Session_Value2DDC5C = 0x002DDC5C;  // (session) -> u32 (name?)
constexpr u32 Obj_Call2E0930ById = 0x002E1DB0;   // (session, id, b, c, d, e, f, g)
constexpr u32 Session_Set2E22A0 = 0x002E22A0;    // (session, save +36) (name?)
constexpr u32 Session_Check2E3130 = 0x002E3130;  // (session, level) -> 1 (name?)
constexpr u32 World_StartLevel = 0x004674C4;     // (World*, u32 level, u8 flag)
constexpr u32 Node_PushF65B7C = 0x00446984;      // (a, b) -> node
constexpr u32 Ptr_DerefNonZero = 0x00446960;     // (node)
constexpr u32 Emitter_Play449E48 = 0x00449E48;   // (node, 0, 0) (name?)
constexpr u32 Emitter_StopGlobal = 0x00449E10;   // (1)
constexpr u32 RefNode_Release = 0x004432DC;      // (node)
constexpr u32 Sound_Mask44BC10 = 0x0044BC10;     // (u32 mask) (name?)
}  // namespace gfn

// Pool layout (0x002EF13C): +0 u32, +4 last free index, +8 head, +12 tail,
// +16 used count, +20 free count, +24 capacity, +28 entries {prev, next,
// Object*, u32 flags: 0x80000000 free, bits 15..30 generation} of 16 bytes,
// +44 u16 (name?)
constexpr u32 kPoolEntrySize = 16;

constexpr u32 kSession = 0x00F64C20;             // u32 session (name?)
constexpr u32 kUnlockTable = 0x00AE6A60;         // {u32 threshold; ...}[8] of 12 bytes (name?)
constexpr u32 kTuneUnlockSkip = 0x010BB849;      // u8: skips entries 0, 1 and 4 (name?)
constexpr u32 kWorldStartHash = 0x3D264A9Bu;     // id given to Obj_Call2E0930ById (name?)
constexpr u32 kTuneStartSound = 0x010B9EA0;      // u32 (0x010B9E58 + 72) (name?)
}  // namespace hevent4g
}  // namespace sly4

using namespace sly4::hevent4g;

// u32 Pool_DeleteAll(Pool* p): deletes (vfunc +4) the object of each of the
// +16 used entries, walking the used list from the head by {u16 index, u16
// generation} handles, and clears their pointers; then rebuilds every entry as
// free (prev i-1, next i+1, free bit set, generation cleared) with the ends at
// -1, head = tail = -1, +0 = 0, +4 = capacity - 1 (-1 for a capacity of 0 or 1),
// used 0, free = capacity, +44 = 0. Returns the capacity (name?)
DECOMP(002EF13C, Pool_DeleteAll) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 used = load<u32>(p + 16);
    u32 it = 0;  // the original's {u16 index, u16 generation} at r1+112
    if (used != 0) {
        const u32 h = load<u32>(p + 8);
        const u32 gen = (load<u32>(load<u32>(p + 28) + (h << 4) + 12) >> 15) & 0xFFFF;
        it = ((h & 0xFFFF) << 16) | gen;
        for (u32 k = 0; k < used; ++k) {
            const u32 idx = it >> 16;
            const u32 e = load<u32>(p + 28);
            const u32 slot = e + (idx << 4) + 8;
            u32 e2 = e;
            const u32 obj = load<u32>(slot);
            if (obj != 0) {
                vcall(ctx, obj, 4, {obj});
                e2 = load<u32>(p + 28);
                store<u32>(slot, 0);
            }
            if (static_cast<s32>(idx) != load<s32>(p + 12)) {
                const u32 off = (idx << 4) & 0x000FFFF0u;
                const u32 nx = load<u32>(e + off + 4);    // via the entries before the delete
                const u32 nx2 = load<u32>(e2 + off + 4);  // via the entries after it
                it = (nx2 << 16) | ((load<u32>(e + (nx << 4) + 12) >> 15) & 0xFFFF);
            } else {
                it = 0;
            }
        }
    }
    u32 n = load<u32>(p + 24);
    if (static_cast<s32>(n) > 0) {
        u32 i = 0;
        do {
            const u32 e = load<u32>(p + 28);
            store<u32>(e + i * kPoolEntrySize, i - 1);
            store<u32>(e + i * kPoolEntrySize + 4, i + 1);
            store<u8>(e + i * kPoolEntrySize + 12, load<u8>(e + i * kPoolEntrySize + 12) | 0x80);
            const u32 f = load<u32>(load<u32>(p + 28) + i * kPoolEntrySize + 12);
            store<u32>(load<u32>(p + 28) + i * kPoolEntrySize + 12, f & ~0x7FFF8000u);
            ++i;
            n = load<u32>(p + 24);
        } while (static_cast<s32>(i) < static_cast<s32>(n));
    }
    const u32 e = load<u32>(p + 28);
    store<u32>(e, 0xFFFFFFFFu);
    store<u32>(e + (n << 4) - 12, 0xFFFFFFFFu);
    store<u32>(p + 8, 0xFFFFFFFFu);
    store<u32>(p + 12, 0xFFFFFFFFu);
    store<u32>(p + 0, 0);
    store<u32>(p + 4, static_cast<s32>(n) > 1 ? n - 1 : 0xFFFFFFFFu);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, n);
    store<u16>(p + 44, 0);
    ret(ctx, n);
}

// u32 World_BeginLevel472234(World* w) (name?): w +45 |= 0x90; for the 8 unlock
// entries, s[4032 + i] (s = the save data, w +80) loses bit 2 when
// Session_Value2DDC5C(session) is below the entry's threshold (entries 0, 1 and 4
// are kept while tuning 0x010BB849 is set). With w +31, Sound_Mask44BC10(0xFFBFFFFF).
// Then: s +44 set -> Obj_Call2E0930ById(session, s +44, s +48, 0, 0, 1, 0, 1); if
// that is 0 (or s +44 is 0): s +40 accepted by Session_Check2E3130 ->
// Session_Set2E22A0(session, s +36), World_StartLevel(w, s +40, 0); otherwise,
// without the first call's result, Obj_Call2E0930ById(session, 0x3D264A9B, 0, 1,
// 0, 1, 0, 0). Finally a node for tuning 0x010B9EA0's value is pushed, played
// (Emitter_Play449E48(n, 0, 0)) when Ptr_DerefNonZero(n), else
// Emitter_StopGlobal(1), and released.
DECOMP(00472234, World_BeginLevel472234) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 s = w + kSaveData;
    store<u8>(w + 45, load<u8>(w + 45) | 0x90);
    const u32 x = static_cast<u32>(call(ctx, gfn::Session_Value2DDC5C, {load<u32>(kSession)}));
    for (s32 i = 0; i < 8; ++i) {
        const u32 thr = load<u32>(kUnlockTable + static_cast<u32>(i) * 12);
        if ((i == 0 || i == 1 || i == 4) && load<u8>(kTuneUnlockSkip) != 0) continue;
        if (x < thr) {
            const u32 b = s + static_cast<u32>(i) + 4032;
            store<u8>(b, load<u8>(b) & ~0x2u);
        }
    }
    if (load<u8>(w + 31) != 0) call(ctx, gfn::Sound_Mask44BC10, {0xFFBFFFFFu});
    u32 r = 0;
    const u32 a = load<u32>(s + 44);
    if (a != 0)
        r = static_cast<u32>(call(ctx, gfn::Obj_Call2E0930ById, {load<u32>(kSession), a, load<u32>(s + 48), 0, 0, 1, 0, 1}));
    if (r == 0) {
        const u32 lv = load<u32>(s + 40);
        if (lv != 0 && static_cast<s32>(call(ctx, gfn::Session_Check2E3130, {load<u32>(kSession), lv})) == 1) {
            call(ctx, gfn::Session_Set2E22A0, {load<u32>(kSession), load<u32>(s + 36)});
            call(ctx, gfn::World_StartLevel, {w, load<u32>(s + 40), 0});
        } else {
            call(ctx, gfn::Obj_Call2E0930ById, {load<u32>(kSession), kWorldStartHash, 0, 1, 0, 1, 0, 0});
        }
    }
    const u32 n = static_cast<u32>(call(ctx, gfn::Node_PushF65B7C, {load<u32>(kTuneStartSound), 0}));
    if (static_cast<u32>(call(ctx, gfn::Ptr_DerefNonZero, {n})) != 0) call(ctx, gfn::Emitter_Play449E48, {n, 0, 0});
    else call(ctx, gfn::Emitter_StopGlobal, {1});
    call(ctx, gfn::RefNode_Release, {n});
}
