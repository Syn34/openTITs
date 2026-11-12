// Game code, medium sweep part 32: target collectors (three more variants of
// Targets_Collect270258, each with its own 32-entry candidate table).
#include "mid32.h"

using namespace sly4;
using namespace sly4::mid32;

namespace {
using namespace m32fn;
constexpr u32 TgtSync32 = 0x0026FAD4u;      // Targets_Sync26FAD4(o, table, n) -> count
constexpr u32 TgtConsider32 = 0x0026FD90u;  // Targets_Consider_0026FD90(o, eye, x, table, &n, keep, unique)
constexpr u32 kSlots32 = 0x010573A8u;       // 12-byte object slots (+0 the object)
constexpr u32 kSlotCount32 = 0x01057528u;   // u16 slot count
constexpr u32 kPropOwner32 = 0x91C7CF5Bu;   // collidable property: owner object
constexpr u32 kPropKind32 = 0xA734004Du;    // collidable property: kind tag
constexpr u32 kKindTarget32 = 0x7A30D7E3u;  // ... value accepted by 002708E8
constexpr u32 kFltMax32 = 0x7F7FFFFFu;

// The shared head of the collectors: the table kTab (32 {0, FLT_MAX} pairs)
// is reset once (u8 *kInit), the count *cnt = 0, then the first s32
// o->12->0 entries are reset again (at least one, unchecked against 32).
// True when the search runs: entries, o->12->4 > *thr and the kind o->8
// gate (2: bit 0x8000 of self +188 set; 4: it clear; 5: bit 0x100 of self
// +188 clear; others pass). self = o->4->8.
bool TgtHead32(PPUContext& ctx, u32 o, u32 kInit, u32 kTab, u32 cnt, u32 thr, u32& self) {
    if (load<u8>(kInit) == 0) {
        for (u32 k = 0; k < 32; ++k) {
            store<u32>(kTab + 8 * k, 0);
            store<u32>(kTab + 8 * k + 4, kFltMax32);
        }
        store<u8>(kInit, 1);
    }
    const u32 q = load<u32>(o + 12);
    store<u32>(cnt, 0);
    const u32 n = load<u32>(q);
    if (n != 0) {
        s32 left = static_cast<s32>(n);
        u32 off = 0;
        do {  // quirk: at least once, no bound against the 32 entries
            --left;
            store<u32>(kTab + off, 0);
            store<u32>(kTab + off + 4, kFltMax32);
            off += 8;
        } while (left > 0);
    }
    self = load<u32>(load<u32>(o + 4) + 8);
    const s32 kind = static_cast<s32>(load<u32>(o + 8));
    if (n == 0) return false;
    ctx.fpr[1] = Lf(q + 4);  // f1 / f2 as the original leaves them
    ctx.fpr[2] = Lf(thr);
    if (!(ctx.fpr[1] > ctx.fpr[2])) return false;
    const u32 w = load<u32>(self + 188);
    if (kind > 3) {
        if (kind == 4) return (w & 0x8000u) == 0;
        if (kind == 5) return (w & 0x100u) == 0;
        return true;
    }
    if (kind == 2) return (w & 0x8000u) != 0;
    return true;
}

// The low word of the u64 value of property key in collidable r's
// {key, -, u64 value} table (+124, s32 count +128), or 0.
u32 TgtProp32(u32 r, u32 key) {
    for (s32 j = 0; j < static_cast<s32>(load<u32>(r + 128)); ++j) {
        const u32 e = load<u32>(r + 124) + 16 * j;
        if (load<u32>(e) == key) return load<u32>(e + 12);
    }
    return 0;
}

bool TgtActive32(u32 x) { return (load<u32>(x + 192) & 1u) != 0; }

// Kind 1: every active object of the slot table other than self is
// considered with (keep 1, unique 0).
void TgtSlots32(PPUContext& ctx, u32 o, u32 a1, u32 self, u32 kTab, u32 cnt) {
    if (static_cast<s32>(load<u32>(o + 8)) != 1) return;
    const s32 n = static_cast<s32>(load<u16>(kSlotCount32));
    for (s32 i = 0; i < n; ++i) {
        const u32 x = load<u32>(kSlots32 + 12 * i);
        if (x == 0 || !TgtActive32(x) || x == self) continue;
        call(ctx, TgtConsider32, {o, a1, x, kTab, cnt, 1, 0});
    }
}
}  // namespace

// u32 Targets_CollectHits_002704C0(Query* o, Eye* a1, Vec* hits): like
// Targets_Collect270258 with the table 0x01067C00 (init u8 0x01067BF8,
// threshold 0x002704B8). Each 8-byte hit (+4 collidable c) is resolved to
// its owner: b = c + (s8)c[5], type (s8)b[24] 1 (or 2 while bit 0x2 of
// 0x01066DC0 is set), root b + (s8)b[16], owner = property 0x91C7CF5B. An
// active owner other than self is considered (keep 0, unique 1); any other
// hit gets +4 cleared. Kind 1 adds the slot objects as well. r3 =
// Targets_Sync26FAD4(o, table, count).
DECOMP(002704C0, Targets_CollectHits_002704C0) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1), hits = arg<u32>(ctx, 2);
    Stack frame(ctx, 192);
    const u32 cnt = frame.addr() + 112;
    constexpr u32 kTab = 0x01067C00u;
    u32 self = 0;
    if (TgtHead32(ctx, o, 0x01067BF8u, kTab, cnt, 0x002704B8u, self)) {
        for (s32 i = 0; i < static_cast<s32>(load<u32>(hits + 4)); ++i) {
            const u32 ent = load<u32>(hits) + 8 * i;
            const u32 c = load<u32>(ent + 4);
            if (c == 0) continue;
            const u32 b = c + static_cast<s32>(static_cast<s8>(load<u8>(c + 5)));
            const s32 t = static_cast<s8>(load<u8>(b + 24));
            u32 owner = 0;
            bool ok = false;
            if (t == 1 || ((load<u32>(0x01066DC0u) & 2u) != 0 && t == 2)) {
                const u32 r = b + static_cast<s32>(static_cast<s8>(load<u8>(b + 16)));
                if (r != 0) {
                    owner = TgtProp32(r, kPropOwner32);
                    ok = true;
                }
            }
            if (ok && owner != 0 && TgtActive32(owner) && owner != self) {
                call(ctx, TgtConsider32, {o, a1, owner, kTab, cnt, 0, 1});
                continue;
            }
            store<u32>(ent + 4, 0);
        }
        TgtSlots32(ctx, o, a1, self, kTab, cnt);
    }
    ctx.gpr[3] = call(ctx, TgtSync32, {o, kTab, load<u32>(cnt)});
}

// u32 Targets_CollectTagged_002708E8(Query* o, Eye* a1, Vec* list): table
// 0x01067D08 (init u8 0x01067D00, threshold 0x002708E4). The first 80-byte
// entry of list whose collidable +68 has (+28 & 0x1F) < 16 is resolved
// (type 1 or 2, root as above) and, when its root's property 0xA734004D is
// 0x7A30D7E3, its active owner (property 0x91C7CF5B) other than self is
// considered (keep 0, unique 0). r3 = Targets_Sync26FAD4(o, table, count).
DECOMP(002708E8, Targets_CollectTagged_002708E8) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1), list = arg<u32>(ctx, 2);
    Stack frame(ctx, 144);
    const u32 cnt = frame.addr() + 112;
    constexpr u32 kTab = 0x01067D08u;
    u32 self = 0;
    if (TgtHead32(ctx, o, 0x01067D00u, kTab, cnt, 0x002708E4u, self)) {
        u32 c = 0;
        bool found = false;
        for (s32 i = 0; i < static_cast<s32>(load<u32>(list + 4)); ++i) {
            const u32 x = load<u32>(load<u32>(list) + 80 * i + 68);
            if ((load<u32>(x + 28) & 0x1Fu) < 16) {
                c = x;
                found = true;
                break;
            }
        }
        if (found) {
            const s32 t = static_cast<s8>(load<u8>(c + 24));
            if (t == 1 || t == 2) {
                const u32 r = c + static_cast<s32>(static_cast<s8>(load<u8>(c + 16)));
                if (r != 0 && TgtProp32(r, kPropKind32) == kKindTarget32) {
                    const u32 owner = TgtProp32(r, kPropOwner32);
                    if (owner != 0 && owner != self && TgtActive32(owner))
                        call(ctx, TgtConsider32, {o, a1, owner, kTab, cnt, 0, 0});
                }
            }
        }
    }
    ctx.gpr[3] = call(ctx, TgtSync32, {o, kTab, load<u32>(cnt)});
}

// u32 Targets_CollectPlayers_00270D78(Query* o, Eye* a1): table 0x01067E10
// (init u8 0x01067E08, threshold 0x00270D74). Every active player object
// (world *0x00F65CA0, u8 +29 players, object at +4216 + 0x20C80 * i) other
// than self is considered (keep 0, unique 0); kind 1 adds the slot objects.
// r3 = Targets_Sync26FAD4(o, table, count).
DECOMP(00270D78, Targets_CollectPlayers_00270D78) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1);
    Stack frame(ctx, 176);
    const u32 cnt = frame.addr() + 112;
    constexpr u32 kTab = 0x01067E10u;
    u32 self = 0;
    if (TgtHead32(ctx, o, 0x01067E08u, kTab, cnt, 0x00270D74u, self)) {
        for (u32 i = 0;; ++i) {
            const u32 w = load<u32>(0x00F65CA0u);
            if (!(i < load<u8>(w + 29))) break;
            const u32 x = load<u32>(w + 0x20C80u * i + 4216);
            if (x == 0 || !TgtActive32(x) || x == self) continue;
            call(ctx, TgtConsider32, {o, a1, x, kTab, cnt, 0, 0});
        }
        TgtSlots32(ctx, o, a1, self, kTab, cnt);
    }
    ctx.gpr[3] = call(ctx, TgtSync32, {o, kTab, load<u32>(cnt)});
}
