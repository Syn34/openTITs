// World_HandleEvent callees, part 13c: the loader's enter / leave of a user
// block (called by Loader_Start2DEA50): placing the player at the entry's
// spawn point.
#include "hevent13.h"

using namespace sly4;
using namespace sly4::hevent13;

namespace {
// Entry (spawn point): vtable +76 (e, u32 index) -> point name hash or 0.
constexpr u32 kEnVtPoint = 76;
constexpr u32 kEntryIndex325EBC = 0x00325EBC;  // (Entry*, Object* player) -> s32 index or -1 (name?)
// UserBlock: +2 u8 user index, +8 player Object* (vtable +84 place(o, Mat*),
// +16 on the owner), +16 Mat (rows of 3 floats at +0/+16/+32/+48).
constexpr u32 kUbIndex = 2, kUbPlayer = 8, kUbMatrix = 16;
constexpr u32 kObjVtPlace = 84, kOwnerVtPlaced = 16;
constexpr u32 kObjFlags192 = 192, kObjFlag192Bit3 = 0x8;
constexpr u32 kObj12BF70 = 0x0012BF70;          // (Object*) (name?)
constexpr u32 kObjOwner = 0x000D93FC;           // Obj_Owner0D93FC (symbols.txt)
// World: +0x128160 u32 pending point hash (cleared when used), +0x1280F0
// Object*[9] (per user), +0x1281E0 HUD-ish object {+345 u8 i, +364 Obj*[]}.
constexpr u32 kWPendingPoint = 0x128160, kW1281E0 = 0x1281E0;
constexpr u32 k1E0Index = 345, k1E0Slots = 364, kSlotVt28 = 28;
}  // namespace

// u32 Loader_Enter2DE41C(Loader*, UserBlock* ub, Entry* e) (name?): places
// the user's player at the entry's spawn point: the point named by e (vt +76)
// or else the world's pending point (then cleared; the player's +192 bit 3
// then also gets 0x0012BF70), falling back to 0x00486460 for the user index
// (e's 0x00325EBC index, or ub +2). Then vt +84 (o, ub +16) and the owner's
// vt +16, and the World +0x1281E0 object's current slot gets vt +28.
DECOMP(002DE41C, Loader_Enter2DE41C) {
    const u32 ub = arg<u32>(ctx, 1), e = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);  // r1+112, r1+116: map keys
    s32 index = -1;
    if (e != 0) index = static_cast<s32>(call(ctx, kEntryIndex325EBC, {e, load<u32>(ub + kUbPlayer)}));
    if (index == -1) index = load<u8>(ub + kUbIndex);
    const u32 idx = static_cast<u32>(index);
    const u32 mat = ub + kUbMatrix;
    bool pending = false;
    u32 named = 0;
    if (e != 0) named = static_cast<u32>(hevent::vcall(ctx, e, kEnVtPoint, {e, idx}));
    if (named != 0) {
        const u32 p = find_point(ctx, frame.addr() + 4, named);
        if (p != 0) copy_mat(ctx, mat, p + kPointMatrix);
        else call(ctx, kWorldSpawn486460, {load<u32>(hevent::kCurrentWorld), idx, mat});
    } else {
        const u32 w = load<u32>(hevent::kCurrentWorld);
        const u32 hash = load<u32>(w + kWPendingPoint);
        if (hash == 0) {
            call(ctx, kWorldSpawn486460, {w, idx, mat});
        } else {
            const u32 p = find_point(ctx, frame.addr(), hash);
            if (p != 0) {
                pending = true;
                copy_mat(ctx, mat, p + kPointMatrix);
            } else {
                call(ctx, kWorldSpawn486460, {load<u32>(hevent::kCurrentWorld), idx, mat});
            }
            store<u32>(load<u32>(hevent::kCurrentWorld) + kWPendingPoint, 0);
        }
    }
    const u32 o = load<u32>(ub + kUbPlayer);
    if (o == 0) return ret(ctx, 0u);
    hevent::vcall(ctx, o, kObjVtPlace, {o, mat});
    const u32 owner = static_cast<u32>(call(ctx, kObjOwner, {load<u32>(ub + kUbPlayer)}));
    if (owner != 0) hevent::vcall(ctx, owner, kOwnerVtPlaced, {owner});
    if (pending) {
        const u32 q = load<u32>(ub + kUbPlayer);
        if (q != 0 && (load<u32>(q + kObjFlags192) & kObjFlag192Bit3) != 0) call(ctx, kObj12BF70, {q});
    }
    const u32 w = load<u32>(hevent::kCurrentWorld);
    const u32 h = load<u32>(w + kW1281E0);
    if (h == 0) return ret(ctx, w);
    const u32 i = load<u8>(h + k1E0Index);
    if (i == 0) return ret(ctx, 0u);
    const u32 s = load<u32>(h + (i << 2) + k1E0Slots);
    if (s == 0) return ret(ctx, 0u);
    hevent::vcall(ctx, s, kSlotVt28, {s});
}

// u32 Loader_Leave2DE794(Loader*, Entry* e, UserBlock* ub) (name?): with e,
// every other user's object (World +0x1280F0[i], i != ub +2) is placed (vt
// +84) at e's spawn point for it (vt +76 of e's 0x00325EBC index, or i)
// starting from the identity, else at 0x00486460's point; then the owner's
// vt +16. r3 as the original leaves it.
DECOMP(002DE794, Loader_Leave2DE794) {
    const u32 e = arg<u32>(ctx, 1), ub = arg<u32>(ctx, 2);
    if (e == 0) return;  // r3 = the loader
    Stack frame(ctx, 80);  // r1+112 Mat (64 bytes), r1+176 map key
    const u32 mat = frame.addr();
    u64 r3 = 0;
    for (u32 i = 0; i < kWNumUsers; ++i) {
        const u32 self = load<u8>(ub + kUbIndex);
        if (i == self) {
            r3 = self;
            continue;
        }
        const u64 slot = static_cast<u64>(load<u32>(hevent::kCurrentWorld)) + 4 * i + kWUsers;  // addc, not truncated
        const u32 p = load<u32>(static_cast<u32>(slot));
        if (p == 0) {
            r3 = slot;
            continue;
        }
        copy_identity(ctx, mat);
        const s32 index = static_cast<s32>(call(ctx, kEntryIndex325EBC, {e, p}));
        const u32 idx = index != -1 ? static_cast<u32>(index) : i;
        const u32 named = static_cast<u32>(hevent::vcall(ctx, e, kEnVtPoint, {e, idx}));
        u32 pt = 0;
        if (named != 0) pt = find_point(ctx, frame.addr() + 64, named);
        if (pt != 0) copy_mat(ctx, mat, pt + kPointMatrix);
        else call(ctx, kWorldSpawn486460, {load<u32>(hevent::kCurrentWorld), idx, mat});
        hevent::vcall(ctx, p, kObjVtPlace, {p, mat});
        const u32 owner = static_cast<u32>(call(ctx, kObjOwner, {p}));
        r3 = owner;
        if (owner != 0) r3 = hevent::vcall(ctx, owner, kOwnerVtPlaced, {owner});
    }
    ctx.gpr[3] = r3;
}
