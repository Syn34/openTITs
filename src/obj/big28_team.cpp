// Game code, large-function sweep part 28: the team / volume objects' base
// update (stale members pruned, members synchronised with the body's
// overlap query, then the object base update).
#include "big28.h"

namespace sly4::obj {

namespace {
constexpr f32 kHalf = 0.5f;                   // .text pool 0x0020DFD0
constexpr u32 kVtQuery = 0x00F1DE30;          // overlap result collector (16-byte entries)
constexpr u32 kVtQueryEnd = 0x00B13C9C;       // stored over it when it ends
constexpr u32 Obj_UpdateBase = 0x000D83C4;    // (Obj*; f1 dt) -> u32
constexpr u32 Obj_FindKeyValue = 0x0020DB54;  // (T*) -> member object or 0
constexpr u32 Team_Refresh_0020DC6C = 0x0020DC6C;  // (T*) (f_0020DC6C) (name?)
constexpr u32 Volume_FlushPending = 0x0020DE3C;    // (Volume*)
constexpr u32 Body_Add = 0x00931D10;          // (world, body)

// A member entry {-, object}; the object's +172 bit 0x40000 = still valid.
struct TeamEntry {
    be_u32 w0;   // +0
    be_u32 obj;  // +4
};
struct TeamObj {
    pad<144> _0;
    Vec3 center;          // +144
    pad<188> _156;
    be_u32 members;       // +344: Obj*[]
    pad<4> _348;
    be_u16 nmembers;      // +352
    pad<12> _354;
    be_u16 nentries;      // +366
    pad<4> _368;
    be_u32 entries;       // +372: TeamEntry[]
    pad<14> _376;
    be_u16 npending;      // +390
    pad<4> _392;
    be_u32 pending;       // +396: TeamEntry[]
    pad<12> _400;
    be_u32 flags;         // +412
    be_u32 body;          // +416: the phantom / body (+8 its world)
};
static_assert(offsetof(TeamObj, members) == 344 && offsetof(TeamObj, nentries) == 366 && offsetof(TeamObj, entries) == 372);
static_assert(offsetof(TeamObj, npending) == 390 && offsetof(TeamObj, flags) == 412 && offsetof(TeamObj, body) == 416);
// The overlap result collector (16-byte entries, +8 the key; 16 inline).
struct TeamQuery {
    be_u32 vtable;    // +0
    u8 b4;            // +4
    pad<3> _5;
    be_u32 data;      // +8
    be_u32 count;     // +12
    be_u32 capflags;  // +16
    pad<256> inl;     // +20
};
struct TeamBounds {
    Vec4 lo;  // +0
    Vec4 hi;  // +16
};

bool Valid(u32 e) { return (gm<be_u32>(gm<TeamEntry>(e).obj + 172) & 0x40000u) != 0; }
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
// with Obj_UpdateBase(o, dt); its r3 is returned (callers read it, not
// f1).
u32 Obj_UpdateTeamBase_0020DFD8(TeamObj* o, f32 dt) {
    u32 w = o->flags;
    if ((w & 0x08000000u) != 0) {
        // The members: the loop bound is the running count.
        u32 cnt = o->nentries, r9 = cnt;
        for (s32 i = 0;; ++i) {
            if (!(i < static_cast<s32>(cnt & 0xFFFF))) break;
            const u32 arr = o->entries;
            const u32 e = arr + 8 * static_cast<u32>(i);
            if (Valid(e)) continue;
            r9 &= 0xFFFF;
            if (static_cast<s32>(r9) >= 2 && i < static_cast<s32>(r9) - 1) {
                u32 p = e, off = 8 * static_cast<u32>(i);
                s32 j = i;
                do {
                    gm<TeamEntry>(arr + off).w0 = gm<TeamEntry>(p + 8).w0.get();
                    gm<TeamEntry>(p).obj = gm<TeamEntry>(p + 8).obj.get();
                    off += 8, p += 8, ++j;
                    r9 = o->nentries;
                } while (j < static_cast<s32>(r9) - 1);
            }
            cnt = r9 - 1;
            o->nentries = static_cast<u16>(cnt);
            r9 = cnt;
            --i;
        }
        // The pending list: the loop bound is re-read.
        for (s32 i = 0;; ++i) {
            const u32 n = o->npending;
            if (!(i < static_cast<s32>(n))) break;
            const u32 arr = o->pending;
            const u32 e = arr + 8 * static_cast<u32>(i);
            if (Valid(e)) continue;
            u32 r9 = n;
            if (static_cast<s32>(n) >= 2 && i < static_cast<s32>(n) - 1) {
                u32 p = e, off = 8 * static_cast<u32>(i);
                s32 j = i;
                do {
                    gm<TeamEntry>(arr + off).w0 = gm<TeamEntry>(p + 8).w0.get();
                    gm<TeamEntry>(p).obj = gm<TeamEntry>(p + 8).obj.get();
                    off += 8, p += 8, ++j;
                    r9 = o->npending;
                } while (j < static_cast<s32>(r9) - 1);
            }
            o->npending = static_cast<u16>(r9 - 1);
            --i;
        }
        call(Volume_FlushPending, o);
        w = o->flags;
    }
    if ((w & 0x80000000u) == 0) return call<u32>(Obj_UpdateBase, o, dt);
    if ((w & 0x04000000u) != 0) {
        const u32 body = o->body;
        if (gm<be_u32>(body + 8) == 0u) {
            call(Body_Add, gm<be_u32>(gm<be_u32>(global<be_u32>(kWorld) + 0x001281D4) + 4).get(), body);
            GuestFrame frame(512);
            TeamBounds* bd = frame.as<TeamBounds>(112);
            vcall(host<u8>(o->body), 32, bd);  // bounds: min, max
            o->center.x = static_cast<f32>(Muls(Adds(lf(bd->lo.x), lf(bd->hi.x)), kHalf));
            o->center.y = static_cast<f32>(Muls(Adds(lf(bd->lo.y), lf(bd->hi.y)), kHalf));
            o->center.z = static_cast<f32>(Muls(Adds(lf(bd->lo.z), lf(bd->hi.z)), kHalf));
            w = o->flags;
        }
    }
    if ((w & 0x08000000u) != 0) {
        call(Team_Refresh_0020DC6C, o);
        return call<u32>(Obj_UpdateBase, o, dt);
    }
    {
        GuestFrame frame(512);
        TeamQuery& q = *frame.as<TeamQuery>(144);
        const u32 body = o->body;
        q.vtable = kVtQuery;
        q.count = 0u;
        q.b4 = 0;
        q.data = guest(&q.inl);
        q.capflags = 0x80000010u;
        vcall(host<u8>(body), 76, &q, 0u);
        const auto hit = [&](u32 k) { return call<u32>(Obj_FindKeyValue, gm<be_u32>(q.data + 16 * k + 8).get()); };
        for (u32 i = 0; i < o->nmembers; ++i) {
            const u32 m = gm<be_u32>(o->members + 4 * i);
            u32 k = 0;
            for (; k < q.count; ++k)
                if (hit(k) == m) break;
            if (!(k < q.count)) vcall(o, 288, m);
        }
        for (u32 k = 0; k < q.count; ++k) {
            const u32 x = hit(k);
            if (x == 0) continue;
            const s32 n = static_cast<s32>(o->nmembers);
            const u32 arr = o->members, w412 = o->flags;
            bool member = false;
            for (s32 j = 0; j < n; ++j)
                if (gm<be_u32>(arr + 4 * static_cast<u32>(j)) == x) {
                    member = true;
                    break;
                }
            if (member) continue;
            if ((w412 & 0x40000000u) == 0 || (gm<be_u32>(x + 188) & 1) != 0) vcall(o, 284, x);  // bit 0 of the u64 at +184
        }
        const u32 cap = q.capflags;
        q.count = 0u;
        if ((cap & 0x80000000u) == 0) {
            q.vtable = kVtQuery;
            const u32 bytes = std::rotl(cap, 4) & 0xFFFFFFF0u;
            vcall(host<u8>(b28fn::HkHeap), 20, q.data.get(), static_cast<s32>(bytes));
        }
        q.data = 0u;
        q.capflags = 0x80000000u;
        q.vtable = kVtQueryEnd;
    }
    return call<u32>(Obj_UpdateBase, o, dt);
}

}  // namespace sly4::obj

NATIVE(0020DFD8, sly4::obj::Obj_UpdateTeamBase_0020DFD8)
