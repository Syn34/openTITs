// Game code, part 21: template instances (functions whose disassembly
// differs only in constants) left over from the obj19 / obj20 surveys.
// Each DECOMP line calls a shared body with the constants scanned from its
// own disassembly; every instance was checked against the original in the
// PPC simulator.
#include "obj20.h"
#include "obj21.h"

using namespace sly4;
using namespace sly4::obj19;
using namespace sly4::obj20;

namespace {

// ---------------------------------------------------------------- AVL tree: remove a key
// The rebalance at *pp without the tree's +8 flag test: the node's height /
// balance fixed (AvlFix), then a single or double rotation when the
// balance is beyond +-1 (as AvlRebalance: the right child's balance < 0
// first rotates it right; the left child's > 0 first rotates it left).
void AvlBalanceAt(PPUContext& ctx, u32 t, u32 pp, u32 rotl, u32 rotr, bool fix = true) {
    u32 n = load<u32>(pp);
    if (fix && n != 0) {
        AvlFix(n);
        n = load<u32>(pp);
    }
    if (n == 0) return;
    if (static_cast<s16>(load<u16>(n)) > 1) {
        const u32 r = load<u32>(n + 16);
        if (r != 0) {
            if (static_cast<s16>(load<u16>(r)) < 0) call(ctx, rotr, {t, n + 16});
            call(ctx, rotl, {t, pp});
        }
    }
    n = load<u32>(pp);
    if (n == 0) return;
    if (static_cast<s16>(load<u16>(n)) >= -1) return;
    const u32 l = load<u32>(n + 12);
    if (l == 0) return;
    if (static_cast<s16>(load<u16>(l)) > 0) call(ctx, rotl, {t, n + 12});
    call(ctx, rotr, {t, pp});
}

// Remove(tree, Node** pp, u32 key, bool* changed): recursive descent by
// the unsigned key (an empty slot: *changed = 0). On the way back up, while
// *changed and the tree's +8 flag are set, the slot is rebalanced. The
// node with the key:
//  - no right child: *changed = 1, the left child takes its place;
//  - no left child: the right child takes its place, *changed = 1;
//  - both: `remove_max`(tree, pp, &node->left, changed) moves the left
//    subtree's rightmost key / value into it; then (while *changed and the
//    +8 flag) the left slot and the slot itself are rebalanced; *changed = 1.
// In the first two cases the node goes back to the type's free list
// (`free_node`(list, node), the list made on first use as AvlPool) and the
// slot is rebalanced without the flag test.
void TAvlRemove(PPUContext& ctx, u32 self, u32 rotl, u32 rotr, u32 remove_max, u32 pool_ea, u32 pool_vt, u32 pool_name) {
    constexpr u32 free_node = 0x00556D18;  // (list, node) (FreeList_Free?)
    const u32 t = arg<u32>(ctx, 0), pp = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2), changed = arg<u32>(ctx, 3);
    const u32 n = load<u32>(pp);
    if (n == 0) {
        store<u8>(changed, 0);
        ctx.gpr[3] = 0;
        return;
    }
    const u32 nk = load<u32>(n + 4);
    if (nk != key) {
        call(ctx, self, {t, n + (nk > key ? 12u : 16u), key, changed});
        if (load<u8>(changed) == 0 || load<u8>(t + 8) == 0) return;
        AvlBalanceAt(ctx, t, pp, rotl, rotr);
        return;
    }
    const u32 right = load<u32>(n + 16);
    if (right == 0) {
        const u32 left = load<u32>(n + 12);
        store<u8>(changed, 1);
        store<u32>(pp, left);
        const u32 pool = AvlPool(ctx, pool_ea, pool_vt, pool_name, true);
        call(ctx, free_node, {pool, n});
        AvlBalanceAt(ctx, t, pp, rotl, rotr);
        return;
    }
    if (load<u32>(n + 12) == 0) {
        store<u32>(pp, right);
        store<u8>(changed, 1);
        const u32 pool = AvlPool(ctx, pool_ea, pool_vt, pool_name, false);
        call(ctx, free_node, {pool, n});
        AvlBalanceAt(ctx, t, pp, rotl, rotr);
        return;
    }
    call(ctx, remove_max, {t, pp, n + 12, changed});
    if (load<u8>(changed) != 0 && load<u8>(t + 8) != 0) {
        const u32 l = load<u32>(load<u32>(pp) + 12);
        if (l != 0) AvlFix(l);
        AvlBalanceAt(ctx, t, load<u32>(pp) + 12, rotl, rotr, false);
        AvlBalanceAt(ctx, t, pp, rotl, rotr);
    }
    store<u8>(changed, 1);
    ctx.gpr[3] = 1;
}

}  // namespace

// The instances: (self, rotate-left, rotate-right, remove-max, free-list
// global, its vtable, its name).
// Remove(tree, Node** pp, u32 key, bool* changed) (template instance)
DECOMP(0073A36C, Avl_Remove) { TAvlRemove(ctx, 0x0073A36C, 0x00739F1C, 0x00739DD4, 0x0073A064, 0x00F8F0E0, 0x00B10EB0, 0x00AC47E8); }
// Remove(tree, Node** pp, u32 key, bool* changed) (template instance)
DECOMP(0076C424, Avl_Remove_0076C424) { TAvlRemove(ctx, 0x0076C424, 0x0076BFD4, 0x0076BE8C, 0x0076C11C, 0x00F8F0E8, 0x00B1F648, 0x00ADBDC9); }
// Remove(tree, Node** pp, u32 key, bool* changed) (template instance)
DECOMP(0077BB08, Avl_Remove_0077BB08) { TAvlRemove(ctx, 0x0077BB08, 0x0077B6B8, 0x0077B570, 0x0077B800, 0x00F8F0F0, 0x00B20C90, 0x00ADF00A); }
// Remove(tree, Node** pp, u32 key, bool* changed) (template instance)
DECOMP(0078A5C0, Avl_Remove_0078A5C0) { TAvlRemove(ctx, 0x0078A5C0, 0x0078A170, 0x0078A028, 0x0078A2B8, 0x00F8F0F8, 0x00B22570, 0x00AEEF23); }
// Remove(tree, Node** pp, u32 key, bool* changed) (template instance)
DECOMP(0078B520, Avl_Remove_0078B520) { TAvlRemove(ctx, 0x0078B520, 0x007855FC, 0x007854B4, 0x0078B218, 0x00F8F0E4, 0x00B10E90, 0x00AEED78); }
// Remove(tree, Node** pp, u32 key, bool* changed) (template instance)
DECOMP(0078C710, Avl_Remove_0078C710) { TAvlRemove(ctx, 0x0078C710, 0x0078C2C0, 0x0078C178, 0x0078C408, 0x00F8F0F4, 0x00B22510, 0x00AEEB72); }
// Remove(tree, Node** pp, u32 key, bool* changed) (template instance)
DECOMP(00790C8C, Avl_Remove_00790C8C) { TAvlRemove(ctx, 0x00790C8C, 0x0078E410, 0x0078E2C8, 0x00790984, 0x00F8F100, 0x00B22550, 0x00AEECC9); }
// Remove(tree, Node** pp, u32 key, bool* changed) (template instance)
DECOMP(00797018, Avl_Remove_00797018) { TAvlRemove(ctx, 0x00797018, 0x00796BC8, 0x00796A80, 0x00796D10, 0x00F8F104, 0x00B22F88, 0x00AEFA70); }
// Remove(tree, Node** pp, u32 key, bool* changed) (template instance)
DECOMP(00798208, Avl_Remove_00798208) { TAvlRemove(ctx, 0x00798208, 0x00797DB8, 0x00797C70, 0x00797F00, 0x00F8F108, 0x00B22FA8, 0x00AEFB23); }

namespace {
// ---------------------------------------------------------------- small virtual-call wrappers
// f(Obj* o, u8 b): o->vfn[slot](o, b)
void TVCallByte(PPUContext& ctx, u32 slot) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, slot, {o, arg<u32>(ctx, 1) & 0xFFu});
}
// f(Obj* o): o->vfn[7](o, 0, k)
void TVCall7K(PPUContext& ctx, u32 k) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 28, {o, 0, k});
}
// f(Obj* o, u8 b): p = o->+off: if (p) p->vfn[10](p, b, k)
void TVCallField10(PPUContext& ctx, u32 off, u32 k) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + off);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    ctx.gpr[4] = b;  // r3 / r4 left on the null path
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    VCall(ctx, p, 40, {p, b, k});
}
// f(?, x, Obj* o): sub_004F986C(x, 0x64FDE756 (a hash), o + 40, o->+40 (u8))
void TCall4F986CHash(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 40);
    ctx.gpr[7] = b;  // r7 holds the byte at the call, as the original
    call(ctx, 0x004F986C, {ctx.gpr[4], 0x64FDE756u, o + 40, b});
}
// bool f(const Obj* o): the current slot record (*(T**)0x00F65CA0 + its +4208 (u8) * 4056)
// + o->+822 (u8) * 4 + 496: its u16 & bit
void TSlotFlag(PPUContext& ctx, u32 bit) {
    const u32 g = load<u32>(0x00F65CA0);
    const u32 i = load<u8>(arg<u32>(ctx, 0) + 822);
    const u32 off = load<u8>(g + 4208) * 4056u;
    const u32 m = load<u16>(g + off + (i << 2) + 496) & bit;
    ctx.gpr[4] = m;  // r4 / r5 left
    ctx.gpr[5] = off;
    ret(ctx, m != 0 ? 1u : 0u);
}
// void f(Sub* s): the outer object p = s - back: its vtables (+0, +8, +12, +16) set from the
// table at 0x00B1F3FC, then sub_00940F24(p) (the base destructor?)
void TDtorVtB1F3FC(PPUContext& ctx, u32 back) {
    const u32 p = arg<u32>(ctx, 0) - back;
    constexpr u32 t = 0x00B1F3FC;
    store<u32>(p, t + 8);
    store<u32>(p + 8, t + 56);
    store<u32>(p + 12, t + 80);
    store<u32>(p + 16, t + 104);
    ctx.gpr[5] = t + 8;  // r5..r7 hold the vtables at the call, as the original
    ctx.gpr[6] = t + 56;
    ctx.gpr[7] = t + 80;
    call(ctx, 0x00940F24, {p, t + 104});
}
}  // namespace

// f(Obj* o, u8 b): o->vfn[76](o, b) (template instance)
DECOMP(0020FA20, Obj_VCall304Byte_0020FA20) { TVCallByte(ctx, 304); }
// f(Obj* o, u8 b): o->vfn[26](o, b) (template instance)
DECOMP(00795C30, Obj_VCall104Byte_00795C30) { TVCallByte(ctx, 104); }
// f(Obj* o, u8 b): o->vfn[22](o, b) (template instance)
DECOMP(00795E2C, Obj_VCall88Byte_00795E2C) { TVCallByte(ctx, 88); }
// f(?, XmlCursor* x, Obj* o): XmlCursor_ReadBool(x, 0x64FDE756, &o->+40, o->+40) (template instance)
DECOMP(002FD540, Obj_ReadBool40_002FD540) { TCall4F986CHash(ctx); }
// f(?, XmlCursor* x, Obj* o): XmlCursor_ReadBool(x, 0x64FDE756, &o->+40, o->+40) (template instance)
DECOMP(002FD708, Obj_ReadBool40_002FD708) { TCall4F986CHash(ctx); }
// f(?, XmlCursor* x, Obj* o): XmlCursor_ReadBool(x, 0x64FDE756, &o->+40, o->+40) (template instance)
DECOMP(002FD8D0, Obj_ReadBool40_002FD8D0) { TCall4F986CHash(ctx); }
// f(Obj* o): o->vfn[7](o, 0, 1) (template instance)
DECOMP(003AE86C, Obj_VCall7_1_003AE86C) { TVCall7K(ctx, 1); }
// f(Obj* o): o->vfn[7](o, 0, 2) (template instance)
DECOMP(003AEC4C, Obj_VCall7_2_003AEC4C) { TVCall7K(ctx, 2); }
// f(Obj* o): o->vfn[7](o, 0, 1) (template instance)
DECOMP(003B00C0, Obj_VCall7_1_003B00C0) { TVCall7K(ctx, 1); }
// bool f(const Obj* o): the slot record flag 0x40 (template instance)
DECOMP(002DA89C, SlotRec_Flag40_002DA89C) { TSlotFlag(ctx, 0x40); }
// bool f(const Obj* o): the slot record flag 0x100 (template instance)
DECOMP(002DA944, SlotRec_Flag100_002DA944) { TSlotFlag(ctx, 0x100); }
// bool f(const Obj* o): the slot record flag 0x80 (template instance)
DECOMP(002DAF54, SlotRec_Flag80_002DAF54) { TSlotFlag(ctx, 0x80); }
// f(Obj* o, u8 b): o->+404 ? its vfn[10](it, b, 0) (template instance)
DECOMP(001BD450, Field_VCall404_10_001BD450) { TVCallField10(ctx, 404, 0); }
// f(Obj* o, u8 b): o->+384 ? its vfn[10](it, b, 1) (template instance)
DECOMP(001BD4CC, Field_VCall384_10_001BD4CC) { TVCallField10(ctx, 384, 1); }
// f(Obj* o, u8 b): o->+24 ? its vfn[10](it, b, 1) (template instance)
DECOMP(0036A470, Field_VCall24_10_0036A470) { TVCallField10(ctx, 24, 1); }
// ~T() of the subobject at +16: vtables from 0x00B1F3FC, sub_00940F24 (template instance)
DECOMP(007717A8, Dtor_VtB1F3FC_16) { TDtorVtB1F3FC(ctx, 16); }
// ~T() of the subobject at +12: vtables from 0x00B1F3FC, sub_00940F24 (template instance)
DECOMP(007717FC, Dtor_VtB1F3FC_12) { TDtorVtB1F3FC(ctx, 12); }
// ~T() of the subobject at +8: vtables from 0x00B1F3FC, sub_00940F24 (template instance)
DECOMP(00771850, Dtor_VtB1F3FC_8) { TDtorVtB1F3FC(ctx, 8); }

namespace {
// bool f(const Obj* o): o->+off is one of four hashes
void TIsHash4(PPUContext& ctx, u32 off) {
    const u32 v = load<u32>(arg<u32>(ctx, 0) + off);
    u32 r = 0, last = 0;
    for (u32 h : {0x9260C26Eu, 0x41EB0AA3u, 0x29846DDAu, 0x4438A28Fu}) {
        last = h;
        if (v == h) {
            r = 1;
            break;
        }
    }
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(static_cast<s32>(last)));  // r4 / r5 left
    ctx.gpr[4] = r;
    ret(ctx, r);
}
// f(Obj* o, u8 k): the +4-ish word (`foff`) of each of the o->+cnt (u16) records of
// `stride` bytes at o->+arr cleared (both re-read every pass), then sub_00325A30(o, k)
void TClearRecsThen325A30(PPUContext& ctx, u32 cnt, u32 arr, u32 stride, u32 foff) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1) & 0xFFu;
    u32 i = 0, off = 0, n;
    while (i < (n = load<u16>(o + cnt))) {
        store<u32>(load<u32>(o + arr) + off + foff, 0);
        ++i;
        off += stride;
    }
    ctx.gpr[5] = off;  // r5..r8 at the call, as the loop leaves them
    ctx.gpr[6] = i;
    ctx.gpr[7] = k;
    ctx.gpr[8] = n;
    call(ctx, 0x00325A30, {o, k});
}
// bool f(IffReader* r): the once flag at `flag` (u8) set (clearing the byte at `b` the
// first time), then Iff_ReadU8(r, &local, 0); returns 1
void TIffReadU8Once(PPUContext& ctx, u32 flag, u32 b) {
    if (load<u8>(flag) == 0) {
        store<u8>(b, 0);
        store<u8>(flag, 1);
        ctx.gpr[6] = b & 0xFFFFu;  // r6..r8 at the call, as the original
        ctx.gpr[7] = 0;
        ctx.gpr[8] = 1;
    }
    Stack local(ctx, 16);
    call(ctx, 0x005643C4, {ctx.gpr[3], local.addr(), 0});
    ret(ctx, 1u);
}
// f32 f(Obj* o, s32 id): t = o->+off ? Anim_Find(o->+off, id, o->+off) : 0;
// t ? `length`(t) : 0.0f (the constant in .text at `zero`)
void TAnimLength(PPUContext& ctx, u32 off, u32 length, u32 zero) {
    const u32 a = load<u32>(arg<u32>(ctx, 0) + off);
    if (a != 0) {
        const u32 t = static_cast<u32>(call(ctx, 0x0034CC34, {a, ctx.gpr[4], a}));
        if (t != 0) {
            call(ctx, length, {t});
            return;
        }
    }
    ctx.gpr[3] = zero;  // r3 left as the constant's address
    ret(ctx, load<f32>(zero));
}
// ~T(): o->+0 = vt1; PtrVector_Clear(o + off); Nop(o + off); o->+0 = vt2 (the base's)
void TDtorPtrVector(PPUContext& ctx, u32 vt1, u32 off, u32 vt2) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt1);
    call(ctx, 0x0054EC1C, {o + off, o + off, vt1});
    call(ctx, 0x0054EC18, {o + off});
    store<u32>(o, vt2);
    ctx.gpr[3] = vt2;  // r3 left as the base vtable
}
// f(Obj* o, x): sub_0091A4C8(o, x) (Havok); then (o + 224)->vfn[slot](o + 224, x)
void THavokThenVCall224(PPUContext& ctx, u32 slot) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 x = ctx.gpr[4];
    call(ctx, 0x0091A4C8, {o, x});
    VCall(ctx, o + 224, slot, {o + 224, x});
}
}  // namespace

// bool f(const Obj* o): o->+192 is one of four hashes (template instance)
DECOMP(00747064, Field_IsHash192_00747064) { TIsHash4(ctx, 192); }
// bool f(const Obj* o): o->+208 is one of four hashes (template instance)
DECOMP(00747584, Field_IsHash208_00747584) { TIsHash4(ctx, 208); }
// bool f(const Obj* o): o->+284 is one of four hashes (template instance)
DECOMP(0075811C, Field_IsHash284_0075811C) { TIsHash4(ctx, 284); }
// f(Obj* o, u8 k): records at +2588 (24 bytes, count +2596): +4 cleared; sub_00325A30(o, k) (template instance)
DECOMP(0032D628, Recs_Clear_0032D628) { TClearRecsThen325A30(ctx, 2596, 2588, 24, 4); }
// f(Obj* o, u8 k): records at +2572 (32 bytes, count +2580): +20 cleared; sub_00325A30(o, k) (template instance)
DECOMP(0032F5FC, Recs_Clear_0032F5FC) { TClearRecsThen325A30(ctx, 2580, 2572, 32, 20); }
// f(Obj* o, u8 k): records at +2576 (32 bytes, count +2584): +20 cleared; sub_00325A30(o, k) (template instance)
DECOMP(00330334, Recs_Clear_00330334) { TClearRecsThen325A30(ctx, 2584, 2576, 32, 20); }
// bool f(IffReader* r): once 0x010B1678 / 0x010B1680, Iff_ReadU8(r, &local, 0) (template instance)
DECOMP(0044ED38, Iff_SkipU8_0044ED38) { TIffReadU8Once(ctx, 0x010B1678, 0x010B1680); }
// bool f(IffReader* r): once 0x010B1698 / 0x010B16A0, Iff_ReadU8(r, &local, 0) (template instance)
DECOMP(0044F35C, Iff_SkipU8_0044F35C) { TIffReadU8Once(ctx, 0x010B1698, 0x010B16A0); }
// bool f(IffReader* r): once 0x010B16A8 / 0x010B16B0, Iff_ReadU8(r, &local, 0) (template instance)
DECOMP(0044F648, Iff_SkipU8_0044F648) { TIffReadU8Once(ctx, 0x010B16A8, 0x010B16B0); }
// f32 f(Obj* o, s32 id): the length of the track id in o->+164 (template instance)
DECOMP(000DC418, Anim_TrackLength164_000DC418) { TAnimLength(ctx, 164, 0x00353748, 0x000DC410); }
// f32 f(Obj* o, s32 id): Field_Get32_64f of the track id in o->+36 (template instance)
DECOMP(00276AD8, Anim_Track36Get_00276AD8) { TAnimLength(ctx, 36, 0x003541B0, 0x00276AD4); }
// f32 f(Obj* o, s32 id): the length of the track id in o->+36 (template instance)
DECOMP(00276B40, Anim_TrackLength36_00276B40) { TAnimLength(ctx, 36, 0x00353748, 0x00276B3C); }
// ~T(): vtables 0x00B22498 -> 0x00B11B20, the PtrVector at +12 (template instance)
DECOMP(004CBD50, Dtor_PtrVector_004CBD50) { TDtorPtrVector(ctx, 0x00B22498, 12, 0x00B11B20); }
// ~T(): vtables 0x00B11DE0 -> 0x00B11DC4, the PtrVector at +12 (template instance)
DECOMP(0074391C, Dtor_PtrVector_0074391C) { TDtorPtrVector(ctx, 0x00B11DE0, 12, 0x00B11DC4); }
// ~T(): vtables 0x00B1BEC8 -> 0x00B1BE78, the PtrVector at +64 (template instance)
DECOMP(00762BD0, Dtor_PtrVector_00762BD0) { TDtorPtrVector(ctx, 0x00B1BEC8, 64, 0x00B1BE78); }
// f(Obj* o, x): sub_0091A4C8(o, x), then (o + 224)->vfn[21](o + 224, x) (template instance)
DECOMP(0075804C, Havok_VCall224_84_0075804C) { THavokThenVCall224(ctx, 84); }
// f(Obj* o, x): sub_0091A4C8(o, x), then (o + 224)->vfn[23](o + 224, x) (template instance)
DECOMP(007580B4, Havok_VCall224_92_007580B4) { THavokThenVCall224(ctx, 92); }
// f(Obj* o, x): sub_0091A4C8(o, x), then (o + 224)->vfn[17](o + 224, x) (template instance)
DECOMP(0077CEF8, Havok_VCall224_68_0077CEF8) { THavokThenVCall224(ctx, 68); }

namespace {
// bool f(Obj* o, s32 id, u8 v): t = o->+off ? Anim_Find(o->+off, id, o->+off) : 0;
// t ? `set`(t, v) : 1
void TAnimTrackSet(PPUContext& ctx, u32 off, u32 set) {
    const u32 v = arg<u32>(ctx, 2) & 0xFFu;
    const u32 a = load<u32>(arg<u32>(ctx, 0) + off);
    if (a != 0) {
        const u32 t = static_cast<u32>(call(ctx, 0x0034CC34, {a, ctx.gpr[4], a}));
        if (t != 0) {
            call(ctx, set, {t, v});
            return;
        }
    }
    ret(ctx, 1u);
}
// u32 f(?, s32 key): the record of `n` 12-byte {key, index, ?} entries at `recs`
// whose key matches: tbl[index * 4] (16-byte rows); 0 when none matches
void TKeyTableLookup(PPUContext& ctx, u32 recs, u32 n, u32 tbl) {
    const s32 key = arg<s32>(ctx, 1);
    u32 i = 0, off = 0, k = 0;
    for (; i < n; ++i, off += 12) {
        k = load<u32>(recs + off);
        if (static_cast<s32>(k) == key) break;
    }
    ctx.gpr[7] = i;  // r4..r8 left
    ctx.gpr[6] = off;
    ctx.gpr[8] = k;
    if (i == n) {
        ctx.gpr[5] = 0;
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[4] = recs + 4;
    ctx.gpr[5] = tbl;
    ret(ctx, load<u32>(tbl + (load<u32>(recs + 4 + 12 * i) << 4)));
}
// bool f(XmlCursor* c): g = *(T**)0x010C2C28: g ? (String_CopyN(g + off,
// XmlCursor_FindAttrByName(c, the name at 0x00AF0120), n), 1) : 0
void TReadAttrToGlobal(PPUContext& ctx, u32 off, u32 n) {
    const u32 g = load<u32>(0x010C2C28);
    ctx.gpr[4] = g;  // r4 left
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[5] = 0x00AF0000;  // r5 at the call, as the original
    const u32 s = static_cast<u32>(call(ctx, o20fn::XmlCursor_FindAttrByName, {ctx.gpr[3], 0x00AF0120}));
    call(ctx, 0x00553638, {g + off, s, n});
    ret(ctx, 1u);
}
// void f(u8* out, const Obj* o): *out = o->+8 ? (u8)o->+8->vfn[slot](&tmp, o->+8) : 0
void TVCallByteOut(PPUContext& ctx, u32 slot) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 p = load<u32>(arg<u32>(ctx, 1) + 8);
    if (p == 0) {
        store<u8>(out, 0);
        ret(ctx, 0u);
        return;
    }
    Stack tmp(ctx, 16);
    VCall(ctx, p, slot, {tmp.addr(), p});
    const u8 b = load<u8>(tmp.addr());
    store<u8>(out, b);
    ret(ctx, b);
}
// void f(u32 x): x removed from the list {+0 u32* items, +8 u16 count} at `list`
// (the last item moved into its place)
void TListRemoveSwap(PPUContext& ctx, u32 list) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 items = load<u32>(list);
    const u32 n = load<u16>(list + 8);
    ctx.gpr[4] = items;  // r4..r7 left
    ctx.gpr[5] = n;
    ctx.gpr[6] = list;
    ctx.gpr[7] = 0;
    for (u32 i = 0; i < n; ++i) {
        if (static_cast<s32>(load<u32>(items + 4 * i)) != static_cast<s32>(x)) continue;
        const u32 last = (n - 1) & 0xFFFFu;
        store<u16>(list + 8, static_cast<u16>(last));
        ctx.gpr[7] = i;
        ctx.gpr[5] = static_cast<u64>(n) - 1;
        ret(ctx, last);
        if (i == last) return;
        const u32 v = load<u32>(items + (last << 2));
        store<u32>(items + (i << 2), v);
        ctx.gpr[5] = i << 2;
        ret(ctx, v);
        return;
    }
    ctx.gpr[7] = n;
}
}  // namespace

// bool f(Obj* o, s32 id, u8 v): sub_00353EB0(the track id in o->+164, v), else 1 (template instance)
DECOMP(000D8760, Anim_TrackSet164_000D8760) { TAnimTrackSet(ctx, 164, 0x00353EB0); }
// bool f(Obj* o, s32 id, u8 v): sub_00353EB0(the track id in o->+36, v), else 1 (template instance)
DECOMP(0029879C, Anim_TrackSet36_0029879C) { TAnimTrackSet(ctx, 36, 0x00353EB0); }
// bool f(Obj* o, s32 id, u8 v): sub_00353F98(the track id in o->+36, v), else 1 (template instance)
DECOMP(00298808, Anim_TrackSet36b_00298808) { TAnimTrackSet(ctx, 36, 0x00353F98); }
// u32 f(?, s32 key): lookup in the 31 records at 0x00F64A78, rows at 0x00AD429C (template instance)
DECOMP(0027C9A0, KeyTable_F64A78_Lookup) { TKeyTableLookup(ctx, 0x00F64A78, 31, 0x00AD429C); }
// u32 f(?, s32 key): lookup in the 11 records at 0x00AD46A8, rows at 0x00AD4528 (template instance)
DECOMP(0027E8F4, KeyTable_AD46A8_Lookup) { TKeyTableLookup(ctx, 0x00AD46A8, 11, 0x00AD4528); }
// u32 f(?, s32 key): lookup in the 1 record at 0x00AD481C, rows at 0x00AD472C (template instance)
DECOMP(002800A4, KeyTable_AD481C_Lookup) { TKeyTableLookup(ctx, 0x00AD481C, 1, 0x00AD472C); }
// bool f(XmlCursor* c): the attribute into (*(T**)0x010C2C28)->+1040 (128 bytes) (template instance)
DECOMP(004EF9E0, Xml_ReadAttr10C2C28_1040) { TReadAttrToGlobal(ctx, 0x410, 128); }
// bool f(XmlCursor* c): the attribute into (*(T**)0x010C2C28)->+880 (128 bytes) (template instance)
DECOMP(004EFD28, Xml_ReadAttr10C2C28_880) { TReadAttrToGlobal(ctx, 0x370, 128); }
// bool f(XmlCursor* c): the attribute into (*(T**)0x010C2C28)->+1008 (32 bytes) (template instance)
DECOMP(004EFD94, Xml_ReadAttr10C2C28_1008) { TReadAttrToGlobal(ctx, 0x3F0, 32); }
// void f(u8* out, const Obj* o): the byte o->+8->vfn[4] gives (template instance)
DECOMP(003476A8, Field_VCallByte8_16_003476A8) { TVCallByteOut(ctx, 16); }
// void f(u8* out, const Obj* o): the byte o->+8->vfn[7] gives (template instance)
DECOMP(003477E8, Field_VCallByte8_28_003477E8) { TVCallByteOut(ctx, 28); }
// void f(u8* out, const Obj* o): the byte o->+8->vfn[10] gives (template instance)
DECOMP(00347914, Field_VCallByte8_40_00347914) { TVCallByteOut(ctx, 40); }
// void f(u32 x): x removed from the list at 0x01013B48 (template instance)
DECOMP(000D68CC, List1013B48_Remove) { TListRemoveSwap(ctx, 0x01013B48); }
// void f(u32 x): x removed from the list at 0x01013B5C (template instance)
DECOMP(000D6940, List1013B5C_Remove) { TListRemoveSwap(ctx, 0x01013B5C); }
// void f(u32 x): x removed from the list at 0x01013B70 (template instance)
DECOMP(000D69B4, List1013B70_Remove) { TListRemoveSwap(ctx, 0x01013B70); }

namespace {
// out[0..2] = the three words at src (load / store interleaved, as the original).
// Leaves r3..r7 as the original's pointer arithmetic does.
void CopyVec3Regs(PPUContext& ctx, u32 out, u32 src) {
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(src + 4 * k);
        store<u32>(out + 4 * k, bits);
    }
    ctx.gpr[3] = out + 8;
    ctx.gpr[4] = src + 8;
    ctx.gpr[5] = out + 4;
    ctx.gpr[6] = src + 4;
    ctx.gpr[7] = out + 8;
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
// void f(f32 out[3], const Obj* o): p = o->+off: out = p ? p->+432..+440 : the Vec3 at 0x01164B40
void TPosOfOrDefault(PPUContext& ctx, u32 off) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 p = load<u32>(arg<u32>(ctx, 1) + off);
    if (p != 0) {
        CopyVec3Regs(ctx, out, p + 432);
        return;
    }
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(0x01164B40 + 4 * k);
        store<u32>(out + 4 * k, bits);
    }
    ctx.gpr[4] = 0x01160000;  // r4 / r5 / f1 left
    ctx.gpr[5] = 0x01164B40;
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
// void f(f32 out[3], const Obj* o): out = o->+388 ? o->+388->+320..+328 : o->+144..+152
void TPos388Or144(PPUContext& ctx) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 388);
    ctx.gpr[5] = p;  // r5 left on the fallback path
    if (p != 0) {
        CopyVec3Regs(ctx, out, p + 320);
        return;
    }
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(o + 144 + 4 * k);
        store<u32>(out + 4 * k, bits);
    }
    ctx.gpr[4] = o + 144;  // r4 / f1 left
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
}  // namespace

// void f(f32 out[3], const Obj* o): the position of o->+348 or the default (template instance)
DECOMP(000DEDF4, Obj_PosOf348_000DEDF4) { TPosOfOrDefault(ctx, 348); }
// void f(f32 out[3], const Obj* o): the position of o->+576 or the default (template instance)
DECOMP(001B8D84, Obj_PosOf576_001B8D84) { TPosOfOrDefault(ctx, 576); }
// void f(f32 out[3], const Obj* o): the position of o->+416 or the default (template instance)
DECOMP(001FB464, Obj_PosOf416_001FB464) { TPosOfOrDefault(ctx, 416); }
// void f(f32 out[3], const Obj* o): o->+388->+320 or o->+144 (template instance)
DECOMP(001691D4, Obj_Pos388Or144_001691D4) { TPos388Or144(ctx); }
// void f(f32 out[3], const Obj* o): o->+388->+320 or o->+144 (template instance)
DECOMP(00179F74, Obj_Pos388Or144_00179F74) { TPos388Or144(ctx); }
// void f(f32 out[3], const Obj* o): o->+388->+320 or o->+144 (template instance)
DECOMP(001931C0, Obj_Pos388Or144_001931C0) { TPos388Or144(ctx); }
