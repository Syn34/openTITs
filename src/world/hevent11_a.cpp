// World_HandleEvent callees, part 11a: small lookups and user-slot helpers
// named by the hevent10_*.cpp functions.
#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent11;

namespace {
constexpr u32 kSessionMovies = 0x010681E4;   // map searched by Session_FindMovie2E2D44 (name?)
constexpr u32 kMapFind7668C4 = 0x007668C4;   // (map, const u32* key) -> value slot (name?)
constexpr u32 kObjFlags = 4, kObjFlagLive = 1;
constexpr u32 kObjState396 = 396;
constexpr u32 kObjState396Busy = 0x02000000;  // +396 bit 25: notification ignored
constexpr u32 kObjNotifyA = 0x00362F1C;  // (Object*, u8, u8) (name?)
constexpr u32 kObjNotifyB = 0x00362B40;  // (Object*, u8, u8) (name?)
// Listener: +32 s32 count, +28708 Volume*[] (u32 slots).
constexpr u32 kLstCount = 32, kLstItems = 28708;
constexpr u32 kLstMatch = 0x00637C3C;    // (slot*, Volume*) -> non-zero on a match (name?)
constexpr u32 kLstRelease = 0x00637894;  // (slot*) (name?)
constexpr u32 kLstClear = 0x00638360;    // (slot* or Volume*) (name?)
constexpr u32 kLstMove = 0x00637D14;     // (slot* dst, slot* src) (name?)
constexpr u32 kEntryStart = 0x002E9B08;  // (u32 hash) (name?)
// Registry_Find value: +28 u8 bits (0x80 loaded, 0x40 failed) (name?)
constexpr u32 kRegState = 28;
constexpr u8 kRegLoaded = 0x80, kRegFailed = 0x40;

// The slot's holder vector: count read through holder->vector, 0 for none.
u32 slot_count(u32 vec) { return vec != 0 ? load<u16>(vec + 8) : 0u; }
}  // namespace

// MovieEntry* Session_FindMovie2E2D44(Session*, u32 hash) (name?): the value
// stored for hash in the movie map at 0x010681E4 (the slot is read
// unchecked).
DECOMP(002E2D44, Session_FindMovie2E2D44) {
    const u32 hash = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), hash);
    const u32 slot = static_cast<u32>(call(ctx, kMapFind7668C4, {kSessionMovies, frame.addr()}));
    ret(ctx, load<u32>(slot));
}

// void Handle_Notify36742C(u32 handle, u8 a, u8 b, u8 c) (name?): for a live
// object (flags bit 0) in the handle map that isn't busy (+396 bit 25),
// calls 0x00362F1C (b set) or 0x00362B40 (b clear) with (obj, a, c). r3 is
// left as the original leaves it on each path.
DECOMP(0036742C, Handle_Notify36742C) {
    const u32 h = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1) & 0xFF, b = arg<u32>(ctx, 2) & 0xFF, c = arg<u32>(ctx, 3) & 0xFF;
    if (h == 0) return ret(ctx, 0u);
    u32 obj;
    {
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), h);
        obj = load<u32>(static_cast<u32>(call(ctx, h11fn::SortedMap_Find3, {hevent::kHandleMap, frame.addr(), h})));
    }
    if (obj == 0 || (load<u32>(obj + kObjFlags) & kObjFlagLive) == 0) return ret(ctx, obj);
    if ((load<u32>(obj + kObjState396) & kObjState396Busy) != 0) return ret(ctx, a);
    call(ctx, b != 0 ? kObjNotifyA : kObjNotifyB, {obj, a, c});
}

// u32 Volume_Notify112D8(Listener* l, Volume* v) (name?): every slot of l
// that matches v is released, cleared and replaced by the last slot (count
// decremented); the loop index still advances past the replaced slot. Ends
// with 0x00638360(v), whose result is returned.
DECOMP(000112D8, Volume_Notify112D8) {
    const u32 l = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    for (s32 i = 0; i < static_cast<s32>(load<u32>(l + kLstCount)); ++i) {
        const u32 off = static_cast<u32>(i) * 4;
        if (static_cast<u32>(call(ctx, kLstMatch, {load<u32>(l + kLstItems) + off, v})) == 0) continue;
        call(ctx, kLstRelease, {load<u32>(l + kLstItems) + off});
        call(ctx, kLstClear, {load<u32>(l + kLstItems) + off});
        const u32 n = load<u32>(l + kLstCount) - 1;
        const u32 base = load<u32>(l + kLstItems);
        store<u32>(l + kLstCount, n);
        call(ctx, kLstMove, {base + off, base + n * 4});
    }
    call(ctx, kLstClear, {v});
}

// void User_Start325B58(UserBlock* ub, s32 slot) (name?): for each entry of
// the slot's holder with a hash and without the started bit (+28 bit 27),
// calls 0x002E9B08(hash) and sets the bit. r3 ends as the slot's holder (ub
// for slot -1), as in the original.
DECOMP(00325B58, User_Start325B58) {
    const u32 ub = arg<u32>(ctx, 0);
    const s32 slot = arg<s32>(ctx, 1);
    if (slot == -1) return ret(ctx, ub);
    const u32 p = ub + static_cast<u32>(slot) * kUbSlotSize + kUbSlots;
    u32 holder = load<u32>(p);
    if (holder == 0 || load<u32>(holder) == 0 || load<u16>(load<u32>(holder) + 8) == 0) return ret(ctx, holder);
    for (u32 i = 0, off = 0;; ++i, off += 4) {
        const u32 vec = load<u32>(holder);
        if (!(i < slot_count(vec))) break;
        const u32 e = load<u32>(load<u32>(vec) + off);
        const u32 hash = load<u32>(e + kSlotEntryHash);
        if (hash == 0 || (load<u32>(e + kSlotEntryFlags) & kSlotEntryStarted) != 0) continue;
        call(ctx, kEntryStart, {hash});
        store<u32>(e + kSlotEntryFlags, load<u32>(e + kSlotEntryFlags) | kSlotEntryStarted);
        holder = load<u32>(p);
    }
    ret(ctx, holder);
}

// u8 User_Prepare3265A4(UserBlock* ub, s32 slot) (name?): 0 when an entry of
// the slot's holder has been started (+28 bit 27) and its registry record
// is loaded (+28 bit 7) but not failed (bit 6); 1 otherwise.
DECOMP(003265A4, User_Prepare3265A4) {
    const u32 ub = arg<u32>(ctx, 0);
    const s32 slot = arg<s32>(ctx, 1);
    if (slot == -1) return ret(ctx, 1u);
    const u32 p = ub + static_cast<u32>(slot) * kUbSlotSize + kUbSlots;
    const u32 holder = load<u32>(p);
    if (holder == 0 || load<u32>(holder) == 0 || load<u16>(load<u32>(holder) + 8) == 0) return ret(ctx, 1u);
    for (u32 i = 0, off = 0;; ++i, off += 4) {
        const u32 vec = load<u32>(load<u32>(p));
        if (!(i < slot_count(vec))) break;
        const u32 e = load<u32>(load<u32>(vec) + off);
        const u32 hash = load<u32>(e + kSlotEntryHash);
        if (hash == 0 || (load<u32>(e + kSlotEntryFlags) & kSlotEntryStarted) == 0) continue;
        u32 rec;
        {
            Stack frame(ctx, 16);
            store<u32>(frame.addr(), hash);
            rec = load<u32>(static_cast<u32>(call(ctx, h11fn::Registry_Find, {kRegistry, frame.addr(), hash})));
        }
        if (rec == 0) continue;
        const u8 st = load<u8>(rec + kRegState);
        if ((st & kRegLoaded) == 0 || (st & kRegFailed) != 0) continue;
        return ret(ctx, 0u);
    }
    ret(ctx, 1u);
}
