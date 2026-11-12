// Game code, large-function sweep part 28: the team / volume objects' base
// update (stale members pruned, members synchronised with the body's
// overlap query, then the object base update).
#include "../sly4.h"
#include "big28.h"

using namespace sly4;
using namespace sly4::big28;

namespace {
constexpr f32 kHalf = 0.5f;                   // .text pool 0x0020DFD0
constexpr u32 kVtQuery = 0x00F1DE30;          // overlap result collector (16-byte entries)
constexpr u32 kVtQueryEnd = 0x00B13C9C;       // stored over it when it ends
constexpr u32 Obj_UpdateBase = 0x000D83C4;    // (Obj*; f1 dt)
constexpr u32 Obj_FindKeyValue = 0x0020DB54;  // (T*) -> member object or 0
constexpr u32 Team_Refresh_0020DC6C = 0x0020DC6C;  // (T*) (f_0020DC6C) (name?)
constexpr u32 Volume_FlushPending = 0x0020DE3C;    // (Volume*)
constexpr u32 Body_Add = 0x00931D10;          // (world, body)
constexpr u32 Game = 0x00F65CA0;              // Game*
constexpr u32 GamePhys = 0x001281D4;          // Game +0x1281D4: the physics holder (+4 the world)
}  // namespace

// u32 f(T* o; f32 dt) (called by Obj_UpdateTeam_0020FB9C, Team_UpdateEntries,
// Obj_TeamScan444 / 428, Obj_UpdateArea, Area_UpdateOccupants, ...).
// With o->+412 bit 0x08000000 the members (+372, u16 count +366) and the
// pending list (+396, count +390) lose the entries whose object's +172 bit
// 0x40000 is clear (later entries moved down), then Volume_FlushPending(o).
// Without +412 bit 0x80000000: Obj_UpdateBase(o, dt) only. With bit
// 0x04000000 and no world on the body o->+416 (+8): Body_Add(world, body)
// and o->+144.. = the centre of the body's bounds (vfn[32]). Then with
// bit 0x08000000 Team_Refresh_0020DC6C(o); else the body's overlaps
// (vfn[76] into a stack collector, entries' +8 mapped by Obj_FindKeyValue)
// are compared with the members (+344, u16 count +352): members not
// overlapping get o->vfn[288](o, m); overlapping objects not members get
// o->vfn[284](o, x) (unless +412 bit 0x40000000 is set and x's +184 bit 0
// is clear); a heap result array is freed through HkHeap->vfn[20]. Ends
// with Obj_UpdateBase(o, dt) (its r3 / f1).
DECOMP(0020DFD8, Obj_UpdateTeamBase_0020DFD8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    u32 w = load<u32>(o + 412);
    if ((w & 0x08000000u) != 0) {
        // The members: the loop bound is the running count.
        u32 cnt = load<u16>(o + 366), r9 = cnt;
        for (s32 i = 0;; ++i) {
            if (!(i < static_cast<s32>(cnt & 0xFFFF))) break;
            const u32 arr = load<u32>(o + 372);
            const u32 e = arr + 8 * static_cast<u32>(i);
            if ((load<u32>(load<u32>(e + 4) + 172) & 0x40000u) != 0) continue;
            r9 &= 0xFFFF;
            if (static_cast<s32>(r9) >= 2 && i < static_cast<s32>(r9) - 1) {
                u32 p = e, off = 8 * static_cast<u32>(i);
                s32 j = i;
                do {
                    store<u32>(arr + off, load<u32>(p + 8));
                    store<u32>(p + 4, load<u32>(p + 12));
                    off += 8, p += 8, ++j;
                    r9 = load<u16>(o + 366);
                } while (j < static_cast<s32>(r9) - 1);
            }
            cnt = r9 - 1;
            store<u16>(o + 366, static_cast<u16>(cnt));
            r9 = cnt;
            --i;
        }
        // The pending list: the loop bound is re-read.
        for (s32 i = 0;; ++i) {
            const u32 n = load<u16>(o + 390);
            if (!(i < static_cast<s32>(n))) break;
            const u32 arr = load<u32>(o + 396);
            const u32 e = arr + 8 * static_cast<u32>(i);
            if ((load<u32>(load<u32>(e + 4) + 172) & 0x40000u) != 0) continue;
            u32 r9 = n;
            if (static_cast<s32>(n) >= 2 && i < static_cast<s32>(n) - 1) {
                u32 p = e, off = 8 * static_cast<u32>(i);
                s32 j = i;
                do {
                    store<u32>(arr + off, load<u32>(p + 8));
                    store<u32>(p + 4, load<u32>(p + 12));
                    off += 8, p += 8, ++j;
                    r9 = load<u16>(o + 390);
                } while (j < static_cast<s32>(r9) - 1);
            }
            store<u16>(o + 390, static_cast<u16>(r9 - 1));
            --i;
        }
        call(ctx, Volume_FlushPending, {o});
        w = load<u32>(o + 412);
    }
    if ((w & 0x80000000u) == 0) {
        callf(ctx, Obj_UpdateBase, {o}, {dt});
        return;
    }
    if ((w & 0x04000000u) != 0) {
        const u32 body = load<u32>(o + 416);
        if (load<u32>(body + 8) == 0) {
            call(ctx, Body_Add, {load<u32>(load<u32>(load<u32>(Game) + GamePhys) + 4), body});
            Stack frame(ctx, 512);
            const u32 sp = frame.addr();
            const u32 b = load<u32>(o + 416);
            VCall(ctx, b, 32, {b, sp + 112});  // bounds: min sp+112.., max sp+128..
            Sf(o + 144, Muls(Adds(Lf(sp + 112), Lf(sp + 128)), kHalf));
            Sf(o + 148, Muls(Adds(Lf(sp + 116), Lf(sp + 132)), kHalf));
            Sf(o + 152, Muls(Adds(Lf(sp + 120), Lf(sp + 136)), kHalf));
            w = load<u32>(o + 412);
        }
    }
    if ((w & 0x08000000u) != 0) {
        call(ctx, Team_Refresh_0020DC6C, {o});
        callf(ctx, Obj_UpdateBase, {o}, {dt});
        return;
    }
    {
        Stack frame(ctx, 512);
        const u32 sp = frame.addr();
        const u32 COL = sp + 144;  // +4 u8, +8 data, +12 count, +16 capacity (inline room for 16 at +20)
        const u32 body = load<u32>(o + 416);
        store<u32>(COL + 0, kVtQuery);
        store<u32>(COL + 12, 0);
        store<u8>(COL + 4, 0);
        store<u32>(COL + 8, sp + 164);
        store<u32>(COL + 16, 0x80000010u);
        VCall(ctx, body, 76, {body, COL, 0});
        const auto hit = [&](u32 k) { return static_cast<u32>(call(ctx, Obj_FindKeyValue, {load<u32>(load<u32>(COL + 8) + 16 * k + 8)})); };
        for (u32 i = 0; i < load<u16>(o + 352); ++i) {
            const u32 m = load<u32>(load<u32>(o + 344) + 4 * i);
            u32 k = 0;
            for (; k < load<u32>(COL + 12); ++k)
                if (hit(k) == m) break;
            if (!(k < load<u32>(COL + 12))) VCall(ctx, o, 288, {o, m});
        }
        for (u32 k = 0; k < load<u32>(COL + 12); ++k) {
            const u32 x = hit(k);
            if (x == 0) continue;
            const s32 n = static_cast<s32>(load<u16>(o + 352));
            const u32 arr = load<u32>(o + 344), w412 = load<u32>(o + 412);
            bool member = false;
            for (s32 j = 0; j < n; ++j)
                if (load<u32>(arr + 4 * static_cast<u32>(j)) == x) {
                    member = true;
                    break;
                }
            if (member) continue;
            if ((w412 & 0x40000000u) == 0 || (load<u64>(x + 184) & 1) != 0) VCall(ctx, o, 284, {o, x});
        }
        const u32 cap = load<u32>(COL + 16);
        store<u32>(COL + 12, 0);
        if ((cap & 0x80000000u) == 0) {
            store<u32>(COL + 0, kVtQuery);
            const u32 bytes = std::rotl(cap, 4) & 0xFFFFFFF0u;
            VCall(ctx, b28fn::HkHeap, 20, {b28fn::HkHeap, load<u32>(COL + 8), static_cast<u64>(static_cast<s64>(static_cast<s32>(bytes)))});
        }
        store<u32>(COL + 8, 0);
        store<u32>(COL + 16, 0x80000000u);
        store<u32>(COL + 0, kVtQueryEnd);
    }
    callf(ctx, Obj_UpdateBase, {o}, {dt});
}
