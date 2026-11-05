// World objects, part 20: more shared bodies of template instances
// (functions that differ only in their constants) found by the
// project-wide survey of normalised disassembly, continuing obj19.h. Each
// instance in obj20_tmpl*.cpp is one DECOMP line calling one of these with
// the constants scanned from its own disassembly.
//
// Register notes: where the original leaves a value in a volatile register
// that a caller could read, the helper sets it explicitly, as the comments
// say.
#pragma once

#include <bit>
#include <cmath>

#include "obj19.h"
#include "obj8.h"
#include "obj17.h"
#include "opentit/decomp.h"

namespace sly4 {
namespace obj20 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj19;
using obj8::Vec4;
using obj8::VLoad;
using obj8::VStore;
using obj8::VSplat;
using obj8::VFill;
using obj8::VLoadLeft;

namespace o20fn {
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (cursor, name) -> const char*
constexpr u32 String_Length = 0x00555FA4;
constexpr u32 String_Assign = 0x00555F80;   // strcpy
constexpr u32 PtrVector_Reserve = 0x00551718;
constexpr u32 kGame = 0x010C2C28;            // the object holding the name lists (name?)
constexpr u32 kStrName = 0x00AF0120;          // "name"
constexpr u32 Array_Destruct = 0x0054EC94;  // (Array*) (empty)
constexpr u32 FreeList_Init = 0x00551A58;   // (list, elem_size, count, heap, bool)
}  // namespace o20fn

// ---------------------------------------------------------------- batch 1

// The inlined ~Array() of a 16-byte array {+0 u16 element size, +6 u16
// capacity << 1 | flag, +8 data, +12 allocator}: the data handed back
// through the allocator's slot 8 (allocator, data, bytes), then
// Array_Destruct(a).
inline void ArrayDtor(PPUContext& ctx, u32 a) {
    const u32 bytes = static_cast<u32>(load<u16>(a)) * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
    const u32 data = load<u32>(a + 8);
    if (data != 0) {
        const u32 al = load<u32>(a + 12);
        VCall(ctx, al, 8, {al, data, bytes});
    }
    call(ctx, o20fn::Array_Destruct, {a});
}

// ~T() of the volume-shape classes: the vtable, the listener notified
// about the shape at +424 (Volume_Notify(g_listener, shape)) and the shape
// released, the arrays at +388 / +364 and the PtrVector at +344
// destroyed, then the base destructor.
inline void TDtorVolumeShape(PPUContext& ctx, u32 vt, u32 listener_ea, u32 notify, u32 release, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, notify, {load<u32>(listener_ea), o + 424});
    call(ctx, release, {o + 424});
    ArrayDtor(ctx, o + 388);
    ArrayDtor(ctx, o + 364);
    VecDtor(ctx, o + 344);
    call(ctx, base, {o});
}

// AVL tree nodes: {+0 s16 balance, +2 s16 height, +4 u32 key, +8 value,
// +12 left, +16 right}, 20 bytes, from a lazily made per-type free list.

// The rebalance after an insert below *pp (only when the tree's +8 flag is
// set): the node's height / balance fixed, then a single or double rotation
// when the balance is beyond +-1.
inline void AvlRebalance(PPUContext& ctx, u32 t, u32 pp, u32 rotl, u32 rotr) {
    if (load<u8>(t + 8) == 0) return;
    u32 n = load<u32>(pp);
    if (n != 0) {
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

// A new leaf in (*pp)->+off: the node from the type's free list (made on
// first use: Mem_Alloc(60, 0, 16), its fields cleared, vtable / name set,
// FreeList_Init(p, 20, 64, 0, 0); the global is stored even when the
// allocation failed, as the original does), filled with the key / value
// unless the list gave none.
// The type's free list, made on first use. `alt`: the caller's register
// allocation leaves r8 / r9 = vtable / name at FreeList_Init (else r8 = name).
inline u32 AvlPool(PPUContext& ctx, u32 pool_ea, u32 pool_vt, u32 pool_name, bool alt = false) {
    u32 pool = load<u32>(pool_ea);
    if (pool == 0) {
        const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {60, 0, 16}));
        if (p != 0) {
            for (u32 k : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(p + k, 0);
            const u16 f = load<u16>(p + 54);
            for (u32 k : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(p + k, 0);
            store<u16>(p + 52, 0);
            store<u16>(p + 54, static_cast<u16>(f & 0x7FFFu));
            store<u32>(p, pool_vt);
            store<u32>(p + 56, pool_name);
            // r8 (/ r9) as the original's call leaves them
            if (alt) {
                ctx.gpr[8] = pool_vt;
                ctx.gpr[9] = pool_name;
            } else {
                ctx.gpr[8] = pool_name;
            }
            call(ctx, o20fn::FreeList_Init, {p, 20, 64, 0, 0});
            pool = p;
        }
        store<u32>(pool_ea, pool);  // quirk: 0 after a failed allocation, then used
    }
    return pool;
}
inline void AvlInitNode(u32 node, u32 key, u32 val) {
    store<u16>(node, 0);
    store<u16>(node + 2, 1);
    store<u32>(node + 4, key);
    store<u32>(node + 8, val);
    store<u32>(node + 12, 0);
    store<u32>(node + 16, 0);
}
inline void AvlNewLeaf(PPUContext& ctx, u32 pool_ea, u32 pool_vt, u32 pool_name, u32 key, u32 val, u32 pp, u32 off) {
    const u32 pool = AvlPool(ctx, pool_ea, pool_vt, pool_name);
    const u32 node = static_cast<u32>(VCall(ctx, pool, 12, {pool}));
    const u32 n = load<u32>(pp);
    if (node != 0) AvlInitNode(node, key, val);
    store<u32>(n + off, node);
}

// Insert(tree, u32 key, value, Node** pp): recursive descent (unsigned key
// compare; an equal key changes nothing), a new leaf at the empty child,
// then the rebalance on the way back up.
inline void TAvlInsert(PPUContext& ctx, u32 self, u32 pool_ea, u32 pool_vt, u32 pool_name, u32 rotl, u32 rotr) {
    const u32 t = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), val = arg<u32>(ctx, 2), pp = arg<u32>(ctx, 3);
    const u32 n = load<u32>(pp);
    const u32 nk = load<u32>(n + 4);
    const u32 off = key < nk ? 12u : key > nk ? 16u : 0u;
    if (off != 0) {
        if (load<u32>(n + off) != 0)
            call(ctx, self, {t, key, val, n + off});
        else
            AvlNewLeaf(ctx, pool_ea, pool_vt, pool_name, key, val, pp, off);
    }
    AvlRebalance(ctx, t, pp, rotl, rotr);
}

// ---------------------------------------------------------------- VMX (raw lanes, vmx.h semantics)

inline Vec4 VSel(const Vec4& a, const Vec4& b, const Vec4& c) {  // vsel: b where c is set, else a
    Vec4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = (a.w[i] & ~c.w[i]) | (b.w[i] & c.w[i]);
    return r;
}
inline Vec4 VAndc(const Vec4& a, const Vec4& b) {  // vandc
    Vec4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = a.w[i] & ~b.w[i];
    return r;
}

// The {x, y, z, w} row the compiler builds from three scalars with the
// three lane masks of its constant pool (splat x, vsel y, vsel z, vandc).
inline Vec4 VRow3(u32 x, u32 y, u32 z, const Vec4 m[3]) {
    return VAndc(VSel(VSel(VFill(x), VFill(y), m[0]), VFill(z), m[1]), m[2]);
}

// Transform(out, obj): the component at obj+off (none: Fallback(out, obj),
// its r3 left). Else the 4x4 matrix at gmat (rows of three floats, w from
// the masks at `masks`, the 48-byte constant pool before the function) on
// the stack, and the component's +16 object's slot 20 called with (that
// object, &matrix, k = the pool's first f32, &result); result +0..8 and
// +16..24 copied to out +0..8 / +16..24. r5 = the entry address and r7-r10
// = 192, the OPD, the object and its vtable, as the original leaves them.
inline void TXformVCall(PPUContext& ctx, u32 off, u32 fallback, u32 gmat, u32 masks) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 c = load<u32>(o + off);
    if (c == 0) {
        call(ctx, fallback, {out, o});
        return;
    }
    Stack frame(ctx, 96);
    const u32 res = frame.addr(), mat = res + 32;
    const Vec4 m[3] = {VLoad(masks), VLoad(masks + 16), VLoad(masks + 32)};
    for (u32 k = 0; k < 4; ++k) {
        const u32 r = gmat + 16 * k;
        VStore(mat + 16 * k, VRow3(load<u32>(r), load<u32>(r + 4), load<u32>(r + 8), m));
    }
    const u32 obj = load<u32>(c + 16);
    const u32 vt = load<u32>(obj);
    const u32 opd = load<u32>(vt + 20);
    ctx.fpr[1] = static_cast<f64>(load<f32>(masks));
    call_ptr(ctx, opd, {obj, mat, load<u32>(opd), res, 192, opd, obj, vt});
    for (u32 k : {0u, 4u, 8u, 16u, 20u, 24u}) store<u32>(out + k, load<u32>(res + k));
}

// Vec3ToVec4(out, const f32* p): out = {p[0], p[1], p[2]} with w from the
// masks (the 48-byte constant pool before the function); lvlx loads, so
// each scalar is the word at p + 4k (left-justified). r4-r7 left as the
// original leaves them (16, 32, p + 8, the masks).
inline void TVec3Row(PPUContext& ctx, u32 masks) {
    const u32 out = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const Vec4 m[3] = {VLoad(masks), VLoad(masks + 16), VLoad(masks + 32)};
    VStore(out, VRow3(VLoadLeft(p).w[0], VLoadLeft(p + 4).w[0], VLoadLeft(p + 8).w[0], m));
    ctx.gpr[4] = 16;
    ctx.gpr[5] = 32;
    ctx.gpr[6] = p + 8;
    ctx.gpr[7] = masks;
}

// SetRef(o, p): the reference at o+off replaced: the old one's slot 40
// called with (old, 0), p stored, and p's u16 count at +178 incremented.
// r3 / r4 = p + 178 and the new count when p is set, as the original
// leaves them.
inline void TSetRef178(PPUContext& ctx, u32 off) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 old = load<u32>(o + off);
    if (old != 0) VCall(ctx, old, 40, {old, 0});
    else ctx.gpr[3] = 0;
    store<u32>(o + off, p);
    if (p == 0) return;
    const u16 n = static_cast<u16>(load<u16>(p + 178) + 1);
    store<u16>(p + 178, n);
    ctx.gpr[3] = p + 178;
    ctx.gpr[4] = n;
}

// Notify(o, const f32* p): the component at o+off (skipped when null, its
// +8 null, or its +232 state byte 4 / 5; r3 = p then) gets the vector
// {p[0], p[1], p[2]} (w from the masks before the function) on the stack:
// first `prep`(c), then the slot 68 of the +224 sub-object with (sub, &v).
inline void TCompVecNotify(PPUContext& ctx, u32 off, u32 masks, u32 prep) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 c = load<u32>(o + off);
    ctx.gpr[3] = p;
    if (c == 0 || load<u32>(c + 8) == 0) return;
    const u8 st = load<u8>(c + 232);
    if (st == 5 || st == 4) return;
    Stack frame(ctx, 16);
    const Vec4 m[3] = {VLoad(masks), VLoad(masks + 16), VLoad(masks + 32)};
    VStore(frame.addr(), VRow3(VLoadLeft(p).w[0], VLoadLeft(p + 4).w[0], VLoadLeft(p + 8).w[0], m));
    call(ctx, prep, {c, masks, 16, 112});
    VCall(ctx, c + 224, 68, {c + 224, frame.addr()});
}

// The deleting ~T() of an owner of objects in a PtrVector at +voff (+voff+8
// the u16 count): as obj19's TDtorOwnerVec (the same quirk), then
// Mem_FreeFrom(this, 0) (r5 = the base vtable); nothing for a null this.
inline void TDeleteOwnerVecAt(PPUContext& ctx, u32 vt, u32 voff, u32 vtbase) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt);
    for (u32 n; (n = load<u16>(o + voff + 8)) != 0;) {
        const u32 obj = load<u32>(load<u32>(o + voff) + ((n - 1) << 2));
        VCall(ctx, obj, 8, {obj});
        const u32 m = static_cast<u32>(load<u16>(o + voff + 8)) - 1;
        store<u16>(o + voff + 8, static_cast<u16>(m));
        if (static_cast<s32>(m) != static_cast<s32>(m & 0xFFFF)) {
            const u32 d = load<u32>(o + voff);
            store<u32>(d + (m << 2), load<u32>(d + ((m << 2) & 0x3FFFC)));
        }
    }
    VecDtor(ctx, o + voff);
    store<u32>(o, vtbase);
    call(ctx, o19fn::Mem_FreeFrom, {o, 0, vtbase});
}

// Copy43(dst, src): obj19's TCopyMat43 at offset 0, with r3-r9 left as the
// original's address arithmetic leaves them.
inline void TCopyMat43Regs(PPUContext& ctx) {
    const u32 d = arg<u32>(ctx, 0), sp = arg<u32>(ctx, 1);
    TCopyMat43(ctx, 0, 0);
    ctx.gpr[3] = d + 52;
    ctx.gpr[4] = sp + 56;
    ctx.gpr[5] = d + 56;
    ctx.gpr[6] = d + 52;
    ctx.gpr[7] = d + 56;
    ctx.gpr[8] = d + 36;
    ctx.gpr[9] = d + 40;
}

// Tree_Insert(tree, u32 key, value): the +4 count incremented, then the
// root leaf made (empty tree) or the recursive insert from the root slot
// at +0. r3 = 0 / r4 = 1 after a new root, as the original leaves them.
inline void TAvlTreeInsert(PPUContext& ctx, u32 insert_at, u32 pool_ea, u32 pool_vt, u32 pool_name) {
    const u32 t = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), val = arg<u32>(ctx, 2);
    store<u32>(t + 4, load<u32>(t + 4) + 1);
    if (load<u32>(t) != 0) {
        call(ctx, insert_at, {t, key, val, t});
        return;
    }
    const u32 pool = AvlPool(ctx, pool_ea, pool_vt, pool_name);
    const u32 node = static_cast<u32>(VCall(ctx, pool, 12, {pool}));
    if (node != 0) {
        AvlInitNode(node, key, val);
        ctx.gpr[4] = 1;
    }
    store<u32>(t, node);
    ctx.gpr[3] = 0;
}

// RemoveMax(tree, Node** target, Node** pp, bool* changed): the rightmost
// node below *pp is unlinked (its left child takes its place), its key /
// value moved into *target, the node handed back to the free list
// (`free_node`(list, node)) and *changed = 1; the rebalance runs on the
// way back up while *changed is set.
inline void TAvlRemoveMax(PPUContext& ctx, u32 self, u32 pool_ea, u32 pool_vt, u32 pool_name, u32 free_node, u32 rotl, u32 rotr) {
    const u32 t = arg<u32>(ctx, 0), target = arg<u32>(ctx, 1), pp = arg<u32>(ctx, 2), changed = arg<u32>(ctx, 3);
    const u32 n = load<u32>(pp);
    if (load<u32>(n + 16) != 0) {
        call(ctx, self, {t, target, n + 16, changed});
        if (load<u8>(changed) == 0) return;
        AvlRebalance(ctx, t, pp, rotl, rotr);
        return;
    }
    store<u32>(load<u32>(target) + 4, load<u32>(n + 4));
    store<u32>(load<u32>(target) + 8, load<u32>(load<u32>(pp) + 8));
    const u32 node = load<u32>(pp);
    store<u32>(pp, load<u32>(node + 12));
    const u32 pool = AvlPool(ctx, pool_ea, pool_vt, pool_name, true);
    call(ctx, free_node, {pool, node});
    store<u8>(changed, 1);
    ctx.gpr[3] = 1;
}

// PushOp(u16 v, Ctx** pc, u32* out): a 132-byte record taken from the
// command block (*pc - 152; +144 the s16 count, records from +152): the
// count incremented, the record's +0 kind = 7; its +4 u16 = v with bits
// 0x8000 / 0x4000 of +6 cleared; with `out`: *out = the record's +4 address
// + the u16 at 0x0106CF84 and a zero word (`half`: halfword) stored there.
// The original's re-checks (the count compare, the kind < 10 test, the
// null record) are kept as written. Returns the old count (s16).
inline void TCmdPushOp7(PPUContext& ctx, bool half) {
    const u32 v = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    const u32 base = load<u32>(arg<u32>(ctx, 1)) - 152;
    const u16 c = load<u16>(base + 144);
    const s32 i = static_cast<s16>(c);
    auto rec = [&](s32 k) { return base + static_cast<u32>(k) * 132u + 152u; };
    store<u16>(base + 144, static_cast<u16>(c + 1));
    store<u32>(rec(i), 7);
    u32 r = 0;
    if (i < static_cast<s32>(static_cast<u16>(c + 1)) && static_cast<s32>(load<u32>(rec(i))) < 10) r = rec(i);
    r += 4;
    if (r != 0) store<u16>(r + 2, static_cast<u16>(load<u16>(r + 2) & 0x3FFFu));
    store<u16>(r, static_cast<u16>(v));
    if (out != 0) {
        const u32 q = r + load<u16>(0x0106CF84);
        store<u32>(out, q);
        if (q != 0) {
            if (half) store<u16>(q, 0);
            else store<u32>(q, 0);
        }
    }
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(i));
}

// Set(o, bool on, u8 arg): on == 0: off_fn(o, arg, 0) (r6 = a1). Else,
// unless bit 0x40000000 of o+off is set (r4 = 1 then): on_fn(o, 0, f1 =
// the f32 at kaddr, 0, 0, gptr) (r5 = kaddr, as the original leaves it).
inline void TOnOffF(PPUContext& ctx, u32 off, u32 off_fn, u32 kaddr, u32 on_fn, u32 gptr) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2) & 0xFFu;
    if ((a1 & 0xFFu) == 0) {
        call(ctx, off_fn, {o, a2, 0, a1});
        return;
    }
    if (load<u32>(o + off) & 0x40000000u) {
        ctx.gpr[4] = 1;
        return;
    }
    callf(ctx, on_fn, {o, 0, kaddr, 0, 0, gptr}, {static_cast<f64>(load<f32>(kaddr))});
}

// ForEach(o): slot 20 of each of the u16 count (at +cnt) objects of the
// pointer array at +arr, in order (r3 = the count read last).
inline void TForEachVCall20(PPUContext& ctx, u32 cnt, u32 arr) {
    const u32 o = arg<u32>(ctx, 0);
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(o + cnt)); ++i) {
        const u32 e = load<u32>(load<u32>(o + arr) + 4 * i);
        VCall(ctx, e, 20, {e});
    }
    ctx.gpr[3] = n;
}

// New(?, owner): p = Mem_Alloc(20, 0, 16); when it succeeded: ctor(p,
// owner), +12 / +16 cleared, the vtable, then bind(owner, hash1, &p->+12, 0)
// and bind(owner, hash2, &p->+16, p->+16). Returns p.
inline void TNew20Bind2(PPUContext& ctx, u32 ctor, u32 vt, u32 bind, u32 hash1, u32 hash2) {
    const u32 owner = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {20, 0, 16}));
    if (p != 0) {
        call(ctx, ctor, {p, owner});
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        store<u32>(p, vt);
        call(ctx, bind, {owner, hash1, p + 12, 0});
        call(ctx, bind, {owner, hash2, p + 16, load<u32>(p + 16)});
    }
    ctx.gpr[3] = p;
}

// Mat4From3x3(Vec4 out[4], const f32 p[4][4]): each row {p[k][0..2]} with w
// from the masks before the function. r3-r9 as the original leaves them.
inline void TMat4Rows(PPUContext& ctx, u32 masks) {
    const u32 out = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const Vec4 m[3] = {VLoad(masks), VLoad(masks + 16), VLoad(masks + 32)};
    for (u32 k = 0; k < 64; k += 16)
        VStore(out + k, VRow3(VLoadLeft(p + k).w[0], VLoadLeft(p + k + 4).w[0], VLoadLeft(p + k + 8).w[0], m));
    ctx.gpr[3] = out + 48;
    ctx.gpr[4] = p + 48;
    ctx.gpr[5] = p + 52;
    ctx.gpr[6] = p + 56;
    ctx.gpr[7] = p + 40;
    ctx.gpr[8] = out + 16;
    ctx.gpr[9] = out + 32;
}

// AddName(cursor): unless the game object is missing (r3 = 0): a copy of
// the cursor's "name" attribute (Mem_Alloc(length + 1, 0, 16), strcpy;
// unchecked) appended to the game object's PtrVector at +voff (+4 u16
// capacity, doubled when full; +8 u16 count; +10 u16 high-water mark).
// Returns 1 (r4 = the new count).
inline void TAddNameCopy(PPUContext& ctx, u32 voff) {
    const u32 cur = arg<u32>(ctx, 0);
    const u32 g = load<u32>(o20fn::kGame);
    if (g == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 s = static_cast<u32>(call(ctx, o20fn::XmlCursor_FindAttrByName, {cur, o20fn::kStrName}));
    const u32 n = static_cast<u32>(call(ctx, o20fn::String_Length, {s}));
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {n + 1, 0, 16}));
    call(ctx, o20fn::String_Assign, {p, s});
    const u32 v = g + voff;
    const u16 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, o20fn::PtrVector_Reserve, {v, static_cast<u32>(cap) << 1});
    store<u32>(load<u32>(v) + (static_cast<u32>(load<u16>(v + 8)) << 2), p);
    const u32 c = static_cast<u32>(load<u16>(v + 8)) + 1;
    const u16 hi = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(c));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c & 0xFFFF) ? hi : c));
    ctx.gpr[3] = 1;
    ctx.gpr[4] = c & 0xFFFF;
}

// ApplyCameraDef(o): for the view at o+60 (none: r3 = 0), the camera def
// keyed by the u32 at (o+88)->+koff (CameraDefMap_Find on the holder's +16
// map, the key in a stack local): its +112 4x3 rows copied to the view's
// +16..+72 with rows 0 and 2 negated -- or the identity rows of 0x01164BD0
// when the def is null -- and +80 = (o+88)->+foff, replaced by the def's
// +176 when equal to the constant at kaddr (read from a null def too, as
// the original does). r3 / r4 / r5 / f1 as the original leaves them.
inline void TApplyCameraDef(PPUContext& ctx, u32 koff, u32 foff, u32 kaddr) {
    constexpr u32 kIdentity = 0x01164BD0;
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 60) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), load<u32>(load<u32>(o + 88) + koff));
    const u32 holder = load<u32>(load<u32>(obj17::kCameraDefHolder) + 16);
    const u32 e = load<u32>(static_cast<u32>(call(ctx, obj17::o17fn::CameraDefMap_Find, {holder, frame.addr()})));
    static constexpr u32 kOff[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
    u32 w[12];
    for (u32 k = 0; k < 12; ++k) w[k] = load<u32>(kIdentity + kOff[k]);
    const u32 fsrc = load<u32>(load<u32>(o + 88) + foff);
    const u32 x = load<u32>(o + 60);
    if (e != 0)
        for (u32 k = 0; k < 12; ++k) {
            w[k] = load<u32>(e + 112 + kOff[k]);
            if (k < 3 || (k >= 6 && k < 9)) w[k] ^= 0x80000000u;  // fneg
        }
    for (u32 k = 0; k < 12; ++k) store<u32>(x + 16 + kOff[k], w[k]);
    u32 f = fsrc;
    if (std::bit_cast<f32>(fsrc) == load<f32>(kaddr)) f = load<u32>(e + 176);
    store<u32>(x + 80, f);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
    ctx.gpr[3] = x;
    ctx.gpr[4] = e;
    ctx.gpr[5] = kaddr;
}

// SlotList_Init(list, s32 n): +24 = n, +0..+20 cleared, +28 = Mem_Alloc(n *
// 16, 0, 16) (0 for n = 0), +32 = 1; then (n != 0) the 16-byte slots
// chained as a free list (+0 prev, +4 next, +12 bit 0x80 of the first byte
// set, bits 0x7FFF8000 of the word cleared), the first +0 / last +4 = -1,
// +8 / +12 = -1, +0 = 0, +4 = n > 1 ? n - 1 : -1, +16 = 0, +20 = n.
// r3-r6 as the original leaves them.
inline void TSlotListInit(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    store<u32>(o + 24, n);
    for (u32 k = 0; k <= 20; k += 4) store<u32>(o + k, 0);
    u32 a = 0;
    if (n != 0) a = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {(n << 4) & 0xFFFFFFF0u, 0, 16}));
    store<u32>(o + 28, a);
    store<u8>(o + 32, 1);
    if (n == 0) {
        ctx.gpr[3] = 0;
        ctx.gpr[4] = 0;
        ctx.gpr[5] = 1;
        return;
    }
    u32 cnt = load<u32>(o + 24);
    if (static_cast<s32>(cnt) > 0) {
        u32 off = 0;
        for (s32 i = 0;; ++i) {
            const u32 e = load<u32>(o + 28) + off;
            store<u32>(e, static_cast<u32>(i - 1));
            store<u32>(e + 4, static_cast<u32>(i + 1));
            store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) | 0x80));
            const u32 f = load<u32>(o + 28) + off + 12;
            store<u32>(f, load<u32>(f) & ~0x7FFF8000u);
            off += 16;
            cnt = load<u32>(o + 24);
            if (!(i + 1 < static_cast<s32>(cnt))) break;
        }
        a = load<u32>(o + 28);
    }
    const u32 arr = load<u32>(o + 28);
    const u32 end = arr + ((cnt << 4) & 0xFFFFFFF0u);
    store<u32>(a, 0xFFFFFFFFu);
    store<u32>(end - 12, 0xFFFFFFFFu);
    const u32 c = load<u32>(o + 24);
    store<u32>(o + 8, 0xFFFFFFFFu);
    store<u32>(o + 12, 0xFFFFFFFFu);
    store<u32>(o, 0);
    const u32 r3 = static_cast<s32>(c) > 1 ? c - 1 : 0xFFFFFFFFu;
    store<u32>(o + 4, r3);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, c);
    ctx.gpr[3] = r3;
    ctx.gpr[4] = c;
    ctx.gpr[5] = end;
    ctx.gpr[6] = arr;
}

// ~T() of the big level-section classes: vtable vt and pre(this, 0, 1)
// (r6 = vt); the section vtable 0x00B1E1F8; the new[]'d 8-byte array at
// +2544 (count at -4) destroyed back to front with elem_dtor and freed;
// for the five slots at +2232 + 8i: each holder {PtrVector* list} has the
// lists' non-null PtrVectors destroyed and freed, then its list
// (destroyed, SmallFree 20) and itself freed, the slot cleared; the
// buffers at +2524 / +2512 / +2500 / +2488 freed and cleared; the
// PtrVector* at +2272 destroyed (SmallFree 20); then base(this).
inline void TDtorSection(PPUContext& ctx, u32 vt, u32 pre, u32 elem_dtor, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, pre, {o, 0, 1, vt});
    const u32 arr = load<u32>(o + 2544);
    store<u32>(o, 0x00B1E1F8);
    if (arr != 0) {
        const u32 hdr = arr - 4;
        for (s32 n = static_cast<s32>(load<u32>(hdr)) - 1; n >= 0; --n) call(ctx, elem_dtor, {arr + static_cast<u32>(n) * 8});
        call(ctx, o19fn::Mem_FreeFrom, {hdr, 0});
        store<u32>(o + 2544, 0);
    }
    for (u32 i = 0; i < 5; ++i) {
        const u32 slot = o + 8 * i + 2232;
        u32 m = load<u32>(slot);
        if (m == 0) continue;
        for (u32 j = 0;; ++j) {
            const u32 v = load<u32>(m);
            const u32 cnt = v != 0 ? load<u16>(v + 8) : 0;
            if (!(j < cnt)) break;
            const u32 e = load<u32>(load<u32>(load<u32>(m)) + 4 * j);
            if (e != 0) {
                VecDtor(ctx, e);
                call(ctx, o19fn::Mem_FreeFrom, {e, 0});
                m = load<u32>(slot);
            }
        }
        if (m == 0) continue;  // the slot read back as null after a free: left as is
        const u32 v = load<u32>(m);
        if (v != 0) {
            VecDtor(ctx, v);
            call(ctx, o19fn::SmallFree, {load<u32>(m), 20});
        }
        call(ctx, o19fn::Mem_FreeFrom, {m, 0});
        store<u32>(slot, 0);
    }
    for (u32 off : {2524u, 2512u, 2500u, 2488u}) {
        const u32 b = load<u32>(o + off);
        if (b == 0) continue;
        call(ctx, o19fn::Mem_FreeFrom, {b, 0});
        store<u32>(o + off, 0);
    }
    const u32 v = load<u32>(o + 2272);
    if (v != 0) {
        VecDtor(ctx, v);
        call(ctx, o19fn::SmallFree, {load<u32>(o + 2272), 20});
    }
    call(ctx, base, {o});
}

// ---------------------------------------------------------------- batch 2: small accessors

// this->+off = 0 (r4 = 0).
inline void TStoreZeroAt(PPUContext& ctx, u32 off) {
    store<u32>(arg<u32>(ctx, 0) + off, 0);
    ctx.gpr[4] = 0;
}

// this->+doff = this->+soff (r4 = the word).
inline void TCopyWordIn(PPUContext& ctx, u32 soff, u32 doff) {
    const u32 o = arg<u32>(ctx, 0), w = load<u32>(o + soff);
    store<u32>(o + doff, w);
    ctx.gpr[4] = w;
}

// q = this->+a; q ? q->+b : 0 (r4 = the result too).
inline void TLoadChain0(PPUContext& ctx, u32 a, u32 b) {
    const u32 q = load<u32>(arg<u32>(ctx, 0) + a);
    const u32 r = q != 0 ? load<u32>(q + b) : 0;
    ctx.gpr[4] = r;
    ctx.gpr[3] = r;
}

// q = this->+off; q ? q->vfn[slot / 4](q) : 0.
inline void TVCallField(PPUContext& ctx, u32 off, u32 slot) {
    const u32 q = load<u32>(arg<u32>(ctx, 0) + off);
    ctx.gpr[3] = q;
    if (q != 0) VCall(ctx, q, slot, {q});
}

// if (this->+off == a1) this->vfn[5](this) (else r5 = the field, as left).
inline void TVCall20IfKey(PPUContext& ctx, u32 off) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + off);
    ctx.gpr[5] = k;
    if (k == arg<u32>(ctx, 1)) VCall(ctx, o, 20, {o});
}

// Copies a 3x3 block: rows of three words 16 bytes apart (f1 = the last).
inline void TCopyMat33(PPUContext& ctx) {
    const u32 d = arg<u32>(ctx, 0), sp = arg<u32>(ctx, 1);
    u32 w = 0;
    for (u32 r = 0; r < 48; r += 16)
        for (u32 k = 0; k < 12; k += 4) {
            w = load<u32>(sp + r + k);
            store<u32>(d + r + k, w);
        }
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
}

// ~T() through a secondary base: p = this + delta, the five vtables of the
// multiply-inherited object at +0 / +8 / +12 / +16 / +20, then base(p)
// (r4-r8 = v20, v0, v8, v12, v16 as the original leaves them).
inline void TAdjDtorVt5Call(PPUContext& ctx, s32 delta, u32 v0, u32 v8, u32 v12, u32 v16, u32 v20, u32 base) {
    const u32 p = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    store<u32>(p, v0);
    store<u32>(p + 8, v8);
    store<u32>(p + 12, v12);
    store<u32>(p + 16, v16);
    store<u32>(p + 20, v20);
    call(ctx, base, {p, v20, v0, v8, v12, v16});
}

// ~T(): the vtable, the PtrVector at +off destroyed (r4 / r5 = this + off
// and the vtable into Clear, as left), the base vtable, then base(this)
// (r4 = the base vtable).
inline void TDtorVecVt(PPUContext& ctx, u32 vt, u32 off, u32 vtbase, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, o19fn::PtrVector_Clear, {o + off, static_cast<u64>(o) + off, vt});
    call(ctx, o19fn::Nop, {o + off});
    store<u32>(o, vtbase);
    call(ctx, base, {o, vtbase});
}

// The deleting ~T() through a secondary base: p = this + delta (nothing
// when that is 0); the vtables at +384 / +380 / +0, dtor(p), then
// SmallFree(p, size).
inline void TAdjDeleteVt3Small(PPUContext& ctx, s32 delta, u32 v384, u32 v380, u32 v0, u32 dtor, u32 size) {
    const u32 p = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    ctx.gpr[3] = p;
    if (p == 0) return;
    store<u32>(p + 384, v384);
    store<u32>(p + 380, v380);
    store<u32>(p, v0);
    call(ctx, dtor, {p, v380, v0});
    call(ctx, o19fn::SmallFree, {p, size});
}

// New(s16 id): p = Mem_Alloc(24, 0, 16); when it succeeded init(p, id,
// name, k). Returns p (r4 = the 64-bit argument after a failed allocation).
inline void TNewNamedCond(PPUContext& ctx, u32 init, u32 name, u32 k) {
    const u64 a0 = ctx.gpr[3];
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {24, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        ctx.gpr[4] = a0;
        return;
    }
    call(ctx, init, {p, static_cast<u64>(static_cast<s64>(static_cast<s16>(a0))), name, k});
    ctx.gpr[3] = p;
}

// ~T(): when the reference at +off is set: the vtable, its slot 40 called
// with (ref, 0); then +off = 0 and the base vtable (r3 / r4 = it and 0).
inline void TDtorReleaseRef(PPUContext& ctx, u32 vt, u32 off, u32 vtbase) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + off);
    if (q != 0) {
        store<u32>(o, vt);
        VCall(ctx, q, 40, {q, 0});
    }
    store<u32>(o + off, 0);
    store<u32>(o, vtbase);
    ctx.gpr[3] = vtbase;
    ctx.gpr[4] = 0;
}

// ~T(): the vtable, the PtrVector at +off destroyed, the base vtable, then
// unreg(table, this + 4) (r5 / r6 = this + 4 and the base vtable, as left).
inline void TDtorVecUnreg(PPUContext& ctx, u32 vt, u32 off, u32 vtbase, u32 unreg, u32 table) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, o19fn::PtrVector_Clear, {o + off, static_cast<u64>(o) + off, vt});
    call(ctx, o19fn::Nop, {o + off});
    store<u32>(o, vtbase);
    call(ctx, unreg, {table, o + 4, o + 4, vtbase});
}

// ~T(): the vtable, the buffers at +a / +b freed (Mem_FreeFrom) and
// cleared, then the base vtable (r3 = it).
inline void TDtorFree2(PPUContext& ctx, u32 vt, u32 a, u32 b, u32 vtbase) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    for (u32 off : {a, b}) {
        const u32 p = load<u32>(o + off);
        if (p == 0) continue;
        call(ctx, o19fn::Mem_FreeFrom, {p, 0});
        store<u32>(o + off, 0);
    }
    store<u32>(o, vtbase);
    ctx.gpr[3] = vtbase;
}

// The deleting versions: nothing for a null this; ~T() as above, then
// SmallFree(this, size).
inline void TDeleteVecVt(PPUContext& ctx, u32 vt, u32 off, u32 vtbase, u32 base, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorVecVt(ctx, vt, off, vtbase, base);
    call(ctx, o19fn::SmallFree, {o, size});
}
inline void TDeleteReleaseRef(PPUContext& ctx, u32 vt, u32 off, u32 vtbase, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorReleaseRef(ctx, vt, off, vtbase);
    // r5 / r6 = 0 and the base vtable, as the original's call leaves them
    call(ctx, o19fn::SmallFree, {o, size, 0, vtbase});
}

// T* New(a0, a1): Mem_Alloc(size, 0, 16), the base constructor (p, a1, a0),
// then the class vtable; returns p or 0.
inline void TNew2Vt(PPUContext& ctx, u32 size, u32 ctor, u32 vt) {
    const u32 a0 = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {size, 0, 16}));
    if (p != 0) {
        call(ctx, ctor, {p, a1, a0});
        store<u32>(p, vt);
    }
    ret(ctx, p);
}

inline void TDeleteVecUnreg(PPUContext& ctx, u32 vt, u32 off, u32 vtbase, u32 unreg, u32 table, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorVecUnreg(ctx, vt, off, vtbase, unreg, table);
    call(ctx, o19fn::SmallFree, {o, size});
}
inline void TDeleteFree2(PPUContext& ctx, u32 vt, u32 a, u32 b, u32 vtbase) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorFree2(ctx, vt, a, b, vtbase);
    call(ctx, o19fn::Mem_FreeFrom, {o, 0, vtbase});
}

// u8 ParamByte(node): the node's two s16 indices (+4 a, +2 b) into the
// 128-byte records of the table at *0x00F65038 (+24; -1: none -> 0); the
// byte at +k of record b (or of its +*(u16*)0x0106CF7E sub-record when its
// +2 count is positive; of address 0 + k for none), times 4, plus the u16 at
// 0x0106CF70, indexes record a. r4-r7 as the original leaves them.
inline void TNodeParamByte(PPUContext& ctx, u32 k) {
    const u32 n = arg<u32>(ctx, 0);
    const u32 t = load<u32>(0x00F65038);
    const s64 a = static_cast<s16>(load<u16>(n + 4));
    const s64 b = static_cast<s16>(load<u16>(n + 2));
    u64 ra = 0;
    if (a != -1) ra = static_cast<u64>(load<u32>(t + 24)) + static_cast<u64>(a << 7);
    u32 rb = 0;
    if (b != -1) rb = static_cast<u32>(static_cast<u64>(load<u32>(t + 24)) + static_cast<u64>(b << 7));
    const u8 cnt = load<u8>(rb + 2);
    u32 q = 0;
    if (static_cast<s32>(cnt) > 0) q = rb + load<u16>(0x0106CF7E);
    const u32 base = load<u16>(0x0106CF70);
    const u32 e = static_cast<u32>(ra + base + (static_cast<u32>(load<u8>(q + k)) << 2));
    ctx.gpr[3] = load<u8>(e);
    ctx.gpr[4] = ra;
    ctx.gpr[5] = base;
    ctx.gpr[6] = cnt;
    ctx.gpr[7] = static_cast<u64>(b);
}

// ~T() through a secondary base (p = this + delta; nothing for 0): the
// vtables at +20 / +8 / +16 / +12 / +0, then the manager's (the SDA global
// at r13 + sda_off, its +44) slot 12 called with (mgr, p, p->+4 u16).
inline void TAdjDtorVt5Notify(PPUContext& ctx, s32 delta, u32 v20, u32 v8, u32 v16, u32 v12, u32 v0, s32 sda_off) {
    const u32 p = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    ctx.gpr[3] = p;
    if (p == 0) return;
    store<u32>(p + 20, v20);
    store<u32>(p + 8, v8);
    store<u32>(p + 16, v16);
    store<u32>(p + 12, v12);
    store<u32>(p, v0);
    const u32 mgr = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13] + static_cast<u32>(sda_off))) + 44);
    VCall(ctx, mgr, 12, {mgr, p, load<u16>(p + 4)});
}

// When test(this): r = this->vfn[8](this, 0); for r 0 / 1 also
// this->vfn[7](this, 0, 2). (r3 = the vtable, r4 = r otherwise.)
inline void TTestVCall2(PPUContext& ctx, u32 test) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, test, {o})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 r = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
    if (r == 0 || r == 1) {
        VCall(ctx, o, 28, {o, 0, 2});
        return;
    }
    ctx.gpr[3] = load<u32>(o);
    ctx.gpr[4] = r;
}

// The same through the other base's order: vtables at +0 / +20 / +8 / +12 /
// +16, base(p) (r4-r8 = v16, v0, v20, v8, v12), then the manager notified.
inline void TAdjDtorVt5CallNotify(PPUContext& ctx, s32 delta, u32 v0, u32 v20, u32 v8, u32 v12, u32 v16, u32 base, s32 sda_off) {
    const u32 p = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    ctx.gpr[3] = p;
    if (p == 0) return;
    store<u32>(p, v0);
    store<u32>(p + 20, v20);
    store<u32>(p + 8, v8);
    store<u32>(p + 12, v12);
    store<u32>(p + 16, v16);
    call(ctx, base, {p, v16, v0, v20, v8, v12});
    const u32 mgr = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13] + static_cast<u32>(sda_off))) + 44);
    VCall(ctx, mgr, 12, {mgr, p, load<u16>(p + 4)});
}

// New(?, cursor): p = Mem_Alloc(16, 0, 16); when it succeeded: ctor(p,
// cursor), the vtable, +12 = 1, then read(cursor, hash, &p->+12, 1).
inline void TNew16ReadFlag(PPUContext& ctx, u32 ctor, u32 vt, u32 read, u32 hash) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {16, 0, 16}));
    if (p != 0) {
        call(ctx, ctor, {p, cur});
        store<u32>(p, vt);
        store<u8>(p + 12, 1);
        call(ctx, read, {cur, hash, p + 12, 1});
    }
    ctx.gpr[3] = p;
}

// The middle class of the +380 / +384 secondary-base hierarchy: when its
// PtrVector* at +388 is set, its vtables (+384 / +380 / +0) and the vector
// destroyed (SmallFree 20); then the base's vtables (+0 = v0b) and base(p).
// r4-r6 into Clear / the base as the original leaves them.
inline void OwnVecPtrMid(PPUContext& ctx, u32 p, u32 v0b, u32 base) {
    const u32 q = load<u32>(p + 388);
    if (q != 0) {
        store<u32>(p + 384, 0x00B12A6C);
        store<u32>(p + 380, 0x00B12A44);
        store<u32>(p, 0x00B12898);
        call(ctx, o19fn::PtrVector_Clear, {q, 0x00B12A44, 0x00B12898, 0x00B12A6C});
        call(ctx, o19fn::Nop, {q});
        call(ctx, o19fn::SmallFree, {load<u32>(p + 388), 20});
    }
    store<u32>(p + 384, 0x00B12528);
    store<u32>(p + 380, 0x00B1200C);
    store<u32>(p, v0b);
    call(ctx, base, {p, 0x00B1200C, v0b});
}

// ~T() of the middle class through the second base (p = this + delta, not
// null-checked); base 0x000D7024 with vtable 0x00B12550.
inline void TAdjDtorOwnVecPtr(PPUContext& ctx, s32 delta) {
    OwnVecPtrMid(ctx, static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta)), 0x00B12550, 0x000D7024);
}

// ~T() of a class derived from it (p = this + delta): its vtables (+0 /
// +380 / +384), its PtrVector at +424, then the middle class with base
// 0x000DF934 and vtable 0x00B126B8.
inline void AdjVecOwnVecPtrBody(PPUContext& ctx, u32 p, u32 v0, u32 v380, u32 v384) {
    store<u32>(p, v0);
    store<u32>(p + 380, v380);
    store<u32>(p + 384, v384);
    call(ctx, o19fn::PtrVector_Clear, {p + 424, v380, v0, v384});
    call(ctx, o19fn::Nop, {p + 424});
    OwnVecPtrMid(ctx, p, 0x00B126B8, 0x000DF934);
}
inline void TAdjDtorVecOwnVecPtr(PPUContext& ctx, s32 delta, u32 v0, u32 v380, u32 v384) {
    AdjVecOwnVecPtrBody(ctx, static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta)), v0, v380, v384);
}
inline void TAdjDeleteVecOwnVecPtr(PPUContext& ctx, s32 delta, u32 v0, u32 v380, u32 v384, u32 size) {
    const u32 p = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    AdjVecOwnVecPtrBody(ctx, p, v0, v380, v384);
    call(ctx, o19fn::SmallFree, {p, size});
}

// PushOp6(u16 v, Ctx** pc): as TCmdPushOp7 with kind 6, bits 0xFFC0 of +6
// cleared and no out pointer. Returns the old count (s16).
inline void TCmdPushOp6(PPUContext& ctx) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 base = load<u32>(arg<u32>(ctx, 1)) - 152;
    const u16 c = load<u16>(base + 144);
    const s32 i = static_cast<s16>(c);
    auto rec = [&](s32 k) { return base + static_cast<u32>(k) * 132u + 152u; };
    store<u16>(base + 144, static_cast<u16>(c + 1));
    store<u32>(rec(i), 6);
    u32 r = 0;
    if (i < static_cast<s32>(static_cast<u16>(c + 1)) && static_cast<s32>(load<u32>(rec(i))) < 10) r = rec(i);
    r += 4;
    if (r != 0) store<u16>(r + 2, static_cast<u16>(load<u16>(r + 2) & 0x003Fu));
    store<u16>(r, static_cast<u16>(v));
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(i));
}

inline void TAdjDeleteOwnVecPtr(PPUContext& ctx, s32 delta, u32 free_fn, u32 size) {
    const u32 p = static_cast<u32>(arg<u32>(ctx, 0) + static_cast<u32>(delta));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    TAdjDtorOwnVecPtr(ctx, delta);
    call(ctx, free_fn, {p, size});
}

// Load64(reader): the 65-byte table at table cleared once (flag byte at
// flag), a u8 read into a stack local and dropped (Iff_ReadU8(r, &b, 0)),
// then table[1..64] = 64 FileReader_ReadByte(r, &table[1 + i], 0); the
// global at ptr_ea = &table[1]. Returns 1.
inline void TLoadTable64(PPUContext& ctx, u32 flag, u32 table, u32 ptr_ea) {
    const u32 r = arg<u32>(ctx, 0);
    if (load<u8>(flag) == 0) {
        call(ctx, 0x00726E54, {table, 0, 65});  // memset
        store<u8>(flag, 1);
    }
    {
        Stack frame(ctx, 16);
        call(ctx, 0x005643C4, {r, frame.addr(), 0});  // Iff_ReadU8
    }
    for (u32 i = 0; i < 64; ++i) call(ctx, 0x0056998C, {r, table + 1 + i, 0});  // FileReader_ReadByte
    store<u32>(ptr_ea, table + 1);
    ctx.gpr[3] = 1;
}

// T* Pool_New(a0, pool, a2): a slot from the pool {+4 header (u16 cap,
// u16 count), +8 optional u32 array, +12 76-byte records}: while count <
// cap: array[count] = 0 (when there is one), count++, record[old count] +0
// = kind; then e = record + 4 (4 when the pool was full -- quirk kept),
// ctor(e), init(e, a0, pool, a2). Returns e.
inline void TPoolNewRec76(PPUContext& ctx, u32 ctor, u32 init, u32 kind) {
    const u32 a0 = arg<u32>(ctx, 0), pool = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2);
    u32 hdr = load<u32>(pool + 4);
    s32 i = -1;
    if (static_cast<s32>(load<u16>(hdr + 2)) < static_cast<s32>(load<u16>(hdr))) {
        const u32 c = load<u16>(hdr + 2);
        const u32 arr = load<u32>(pool + 8);
        if (arr != 0) {
            store<u32>(arr + ((c << 2) & 0x3FFFCu), 0);
            hdr = load<u32>(pool + 4);
        }
        store<u16>(hdr + 2, static_cast<u16>(load<u16>(hdr + 2) + 1));
        i = static_cast<s16>(c);
    }
    u32 rec = 0;
    if (i >= 0 && i < static_cast<s32>(load<u16>(hdr + 2))) {
        rec = load<u32>(pool + 12) + static_cast<u32>(i) * 76u;
        store<u32>(rec, kind);
    }
    const u32 e = rec + 4;
    call(ctx, ctor, {e, rec});
    call(ctx, init, {e, a0, pool, a2});
    ctx.gpr[3] = e;
}

// GetSize(Vec2* out, o): with bit 0x20000000 of o+136 the +148 child's
// slot 140 (out, child) answers -- or, without a child, the u16 width /
// height at 0x010C5A24 as floats; else o's slot 168 fills a stack Vec2
// that is scaled by the f32s at o+sx / o+sy. f1 / f2 and r3 / r4 as the
// original leaves them.
inline void TGetSize2(PPUContext& ctx, u32 sx, u32 sy) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    if (load<u32>(o + 136) & 0x20000000u) {
        const u32 c = load<u32>(o + 148);
        if (c != 0) {
            VCall(ctx, c, 140, {out, c});
            return;
        }
        const u32 w = load<u16>(0x010C5A24), h = load<u16>(0x010C5A26);
        store<f32>(out, static_cast<f32>(static_cast<f64>(w)));
        store<f32>(out + 4, static_cast<f32>(static_cast<f64>(h)));
        ctx.gpr[3] = h;
        ctx.gpr[4] = w;
        ctx.fpr[1] = static_cast<f64>(static_cast<f32>(static_cast<f64>(h)));
        ctx.fpr[2] = static_cast<f64>(h);
        return;
    }
    Stack frame(ctx, 16);
    VCall(ctx, o, 168, {frame.addr(), o});
    const f32 x = static_cast<f32>(static_cast<f64>(load<f32>(o + sx)) * static_cast<f64>(load<f32>(frame.addr())));
    const f32 y = static_cast<f32>(static_cast<f64>(load<f32>(o + sy)) * static_cast<f64>(load<f32>(frame.addr() + 4)));
    store<f32>(out, x);
    store<f32>(out + 4, y);
    ctx.gpr[3] = o + sx;
    ctx.gpr[4] = o + sy;
    ctx.fpr[1] = static_cast<f64>(x);
    ctx.fpr[2] = static_cast<f64>(y);
}

// Its deleting version: nothing for a null this; then free_fn(this, size).
inline void TDeleteVolumeShape(PPUContext& ctx, u32 vt, u32 listener_ea, u32 notify, u32 release, u32 base, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    TDtorVolumeShape(ctx, vt, listener_ea, notify, release, base);
    call(ctx, free_fn, {o, size});
}

// SlotList_Grow(list, s32 n) (see TSlotListInit): nothing when n is the
// capacity; else the slots moved to a new Mem_Alloc(n * 16) block (memmove,
// old one freed), slot[cap] +0 = the tail and linked after it (when the
// free count is nonzero), slots cap..n-1 chained (cap's flags left as they
// are, the others' set as at init), the last +4 = -1, the head = cap when
// nothing was free, tail = n - 1, free count += n - cap, capacity = n.
inline void TSlotListGrow(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    const u32 c0 = load<u32>(o + 24);
    if (n == c0) {
        ctx.gpr[3] = c0;
        return;
    }
    const u32 na = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {(n << 4) & 0xFFFFFFF0u, 0, 16}));
    call(ctx, 0x0071C3F0, {na, load<u32>(o + 28), (load<u32>(o + 24) << 4) & 0xFFFFFFF0u});  // memmove
    call(ctx, o19fn::Mem_FreeFrom, {load<u32>(o + 28), 0});
    store<u32>(o + 28, na);
    const u32 c = load<u32>(o + 24);
    const u32 tail = load<u32>(o + 4);
    store<u32>(na + ((c << 4) & 0xFFFFFFF0u), tail);
    u32 fr = load<u32>(o + 20);
    u32 cc = load<u32>(o + 24);
    u32 arr = load<u32>(o + 28);
    u32 r3 = c;
    if (fr != 0) {
        store<u32>(arr + ((tail << 4) & 0xFFFFFFF0u) + 4, c);
        r3 = cc;
    }
    store<u32>(arr + ((cc << 4) & 0xFFFFFFF0u) + 4, r3 + 1);
    u32 i = cc + 1;
    if (static_cast<s32>(i) < static_cast<s32>(n)) {
        u32 off = (i << 4) & 0xFFFFFFF0u;
        for (s32 k = static_cast<s32>(n - i); k > 0; --k, ++i, off += 16) {
            const u32 e = load<u32>(o + 28) + off;
            store<u32>(e, i - 1);
            store<u32>(e + 4, i + 1);
            store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) | 0x80));
            const u32 f = load<u32>(o + 28) + off + 12;
            store<u32>(f, load<u32>(f) & ~0x7FFF8000u);
        }
        fr = load<u32>(o + 20);
        cc = load<u32>(o + 24);
        arr = load<u32>(o + 28);
    }
    store<u32>(arr + (n << 4) - 12, 0xFFFFFFFFu);
    if (fr == 0) store<u32>(o, cc);
    store<u32>(o + 4, n - 1);
    store<u32>(o + 20, fr + (n - cc));
    store<u32>(o + 24, n);
    ctx.gpr[3] = fr + (n - cc);
    ctx.gpr[4] = n - 1;
}

// Sync(o): for each of the u32 count (at d+cnt, d = o+8) 28-byte entries
// of d+arr whose +16 element is set and whose object (o+objs)[i] has bit
// 0x8000 of +164: the object's +80 4x4 matrix (w columns dropped, rows 0 /
// 1 negated) on the stack to the element's slot 20 (elem, &m), then the
// element's slot 152 with f1 = the constant at kaddr and its slot 156.
// The count is re-read after each update.
inline void TSyncElems(PPUContext& ctx, u32 cnt, u32 arr, u32 objs, u32 kaddr) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = load<u32>(o + 8);
    u32 n = load<u32>(d + cnt);
    ctx.gpr[3] = n;
    if (n == 0) return;
    const f64 k = static_cast<f64>(load<f32>(kaddr));
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(d + arr) + 28 * i + 16);
        if (e == 0) continue;
        const u32 x = load<u32>(load<u32>(o + objs) + 4 * i);
        if (x == 0 || (load<u32>(x + 164) & 0x8000u) == 0) continue;
        const u32 m = x + 80;
        Stack frame(ctx, 64);
        const u32 t = frame.addr();
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 3; ++c) {
                const u32 w = load<u32>(m + 16 * r + 4 * c);
                store<u32>(t + 16 * r + 4 * c, r < 2 ? w ^ 0x80000000u : w);  // fneg
            }
        // f1-f7 as the original's loads leave them
        for (u32 j = 0; j < 3; ++j) ctx.fpr[1 + j] = -static_cast<f64>(load<f32>(m + 4 * j));
        for (u32 j = 0; j < 3; ++j) ctx.fpr[4 + j] = -static_cast<f64>(load<f32>(m + 16 + 4 * j));
        ctx.fpr[7] = static_cast<f64>(load<f32>(m + 56));
        VCall(ctx, e, 20, {e, t});
        const u32 e2 = load<u32>(load<u32>(d + arr) + 28 * i + 16);
        ctx.fpr[1] = k;
        VCall(ctx, e2, 152, {e2});
        VCall(ctx, e2, 156, {e2});
        n = load<u32>(d + cnt);
        ctx.gpr[3] = n;
    }
}

inline f32 FMuls(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * static_cast<f64>(b)); }
inline bool FGt(u32 a, u32 b) { return std::bit_cast<f32>(a) > std::bit_cast<f32>(b); }  // fcmpu GT

// Hit(o, const Params* p): only when test(o). t = the k[0] constant (k =
// the 64-byte pool before the function: k[0], k[1], then three lane masks)
// or, with p+96 > p+52 and p+48 > p+52, (p+48 - p+96) / (p+48 - p+52).
// When the physics component at o+comp (state not 4 / 5) exists, p+64 !=
// k[1] and set+s1 > k[1] (set = o+216): the impulse {p+80, p+84, p+88} *
// p+64 * set+s1 * t as a vector (w from the masks) through Hk_Activate(c,
// ...) and the component's +224 slot 84 (sub, &v). Then, when set+s2 >
// k[1], o's slot 352 with f1 = p+60 * t * set+s2 (o, f1, p->+4,
// 0x03C599D8); finally p+14 |= 0x100.
inline void THitImpulse(PPUContext& ctx, u32 kpool, u32 comp, u32 s1, u32 s2, u32 test) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 set = load<u32>(o + 216);
    if (static_cast<u32>(call(ctx, test, {o})) == 0) return;
    const u32 k1 = load<u32>(kpool + 4);
    f32 t = load<f32>(kpool);
    if (FGt(load<u32>(p + 96), load<u32>(p + 52)) && FGt(load<u32>(p + 48), load<u32>(p + 52))) {
        const f32 a = static_cast<f32>(static_cast<f64>(load<f32>(p + 48)) - static_cast<f64>(load<f32>(p + 96)));
        const f32 b = static_cast<f32>(static_cast<f64>(load<f32>(p + 48)) - static_cast<f64>(load<f32>(p + 52)));
        t = static_cast<f32>(static_cast<f64>(a) / static_cast<f64>(b));
    }
    const u32 c = load<u32>(o + comp);
    if (c != 0) {
        const u8 st = load<u8>(c + 232);
        if (st != 5 && st != 4 && load<f32>(p + 64) != std::bit_cast<f32>(k1) && FGt(load<u32>(set + s1), k1)) {
            const f32 m = load<f32>(p + 64), f = load<f32>(set + s1);
            const f32 x = FMuls(FMuls(FMuls(load<f32>(p + 80), m), f), t);
            const f32 y = FMuls(FMuls(FMuls(load<f32>(p + 84), m), f), t);
            const f32 z = FMuls(FMuls(FMuls(load<f32>(p + 88), m), f), t);
            const Vec4 mk[3] = {VLoad(kpool + 16), VLoad(kpool + 32), VLoad(kpool + 48)};
            Stack frame(ctx, 16);
            VStore(frame.addr(), VRow3(std::bit_cast<u32>(x), std::bit_cast<u32>(y), std::bit_cast<u32>(z), mk));
            ctx.fpr[1] = static_cast<f64>(z);
            call(ctx, 0x0091A4C8, {c, 16, kpool, 112, 32, 48});  // Hk_Activate
            ctx.fpr[1] = static_cast<f64>(z);
            VCall(ctx, c + 224, 84, {c + 224, frame.addr()});
        }
    }
    if (FGt(load<u32>(set + s2), k1)) {
        const f32 f = FMuls(FMuls(load<f32>(p + 60), t), load<f32>(set + s2));
        const u32 opd = load<u32>(load<u32>(o) + 352);
        ctx.fpr[1] = static_cast<f64>(f);
        // f1 takes the r4 slot (left holding the OPD, r7 = its entry)
        call_ptr(ctx, opd, {o, opd, load<u32>(p + 4), 0x03C599D8, load<u32>(opd)});
    }
    const u16 fl = static_cast<u16>(load<u16>(p + 14) | 0x100);
    store<u16>(p + 14, fl);
    ctx.gpr[3] = fl;
}

// ---------------------------------------------------------------- batch 3: 3-member groups

// this->+a + b (a member's sub-object address).
inline void TFieldPlus(PPUContext& ctx, u32 a, u32 b) { ctx.gpr[3] = load<u32>(arg<u32>(ctx, 0) + a) + b; }

// q = this->+a; if q: q->+b = (f32)f1 (r3 = q).
inline void TSetF32Via(PPUContext& ctx, u32 a, u32 b) {
    const u32 q = load<u32>(arg<u32>(ctx, 0) + a);
    ctx.gpr[3] = q;
    if (q != 0) store<u32>(q + b, std::bit_cast<u32>(static_cast<f32>(ctx.fpr[1])));
}

// q = this->+a; q ? (s32)q->+b : 0, sign-extended (r4 = it too).
inline void TLoadChainS32(PPUContext& ctx, u32 a, u32 b) {
    const u32 q = load<u32>(arg<u32>(ctx, 0) + a);
    const u64 r = q != 0 ? static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(q + b)))) : 0;
    ctx.gpr[3] = r;
    ctx.gpr[4] = r;
}

// target(this, 0); its result returned.
inline void TCallThis0(PPUContext& ctx, u32 target) { call(ctx, target, {arg<u32>(ctx, 0), 0}); }

// q = this->+32: q->+992..1000 = (q->+872)->+32..40 (f1-f3 / r3 / r4 as left).
inline void TCopyVec3Via872(PPUContext& ctx) {
    const u32 q = load<u32>(arg<u32>(ctx, 0) + 32);
    const u32 r = load<u32>(q + 872);
    u32 w[3];
    for (u32 k = 0; k < 3; ++k) w[k] = load<u32>(r + 32 + 4 * k);
    for (u32 k = 0; k < 3; ++k) {
        store<u32>(q + 992 + 4 * k, w[k]);
        ctx.fpr[1 + k] = static_cast<f64>(std::bit_cast<f32>(w[k]));
    }
    ctx.gpr[3] = q;
    ctx.gpr[4] = r;
}

// this->+off = (f32)f1.
inline void TStoreF1At(PPUContext& ctx, u32 off) {
    store<u32>(arg<u32>(ctx, 0) + off, std::bit_cast<u32>(static_cast<f32>(ctx.fpr[1])));
}

// this->+off (u16) |= k (r4 = the new value).
inline void TOrU16(PPUContext& ctx, u32 off, u32 k) {
    const u16 v = static_cast<u16>(load<u16>(arg<u32>(ctx, 0) + off) | k);
    store<u16>(arg<u32>(ctx, 0) + off, v);
    ctx.gpr[4] = v;
}

// bool: this->+off + k == 0 (the field equals -k).
inline void TFieldIsK(PPUContext& ctx, u32 off, s32 k) {
    ctx.gpr[3] = (load<u32>(arg<u32>(ctx, 0) + off) + static_cast<u32>(k)) == 0 ? 1u : 0u;
}

// bool: bit `mask` of (this->+a)->+b.
inline void TFlagVia(PPUContext& ctx, u32 a, u32 b, u32 mask) {
    ctx.gpr[3] = (load<u32>(load<u32>(arg<u32>(ctx, 0) + a) + b) & mask) != 0 ? 1u : 0u;
}

// dst[0..n) = (a1 + soff)[0..n) (f1 = the last word, r4 = a1 + soff).
inline void TCopyFieldsR4(PPUContext& ctx, u32 soff, u32 n) {
    TCopyFields(ctx, soff, n);
    ctx.gpr[4] = arg<u32>(ctx, 1) + soff;
}

// bool: either flag bit of this->+off (r4 = the result, r5 = the first bit).
inline void TAnyBit2(PPUContext& ctx, u32 off, u32 m1, u32 m2) {
    const u32 w = load<u32>(arg<u32>(ctx, 0) + off);
    const u32 b1 = (w & m1) != 0 ? 1u : 0u;
    const u32 r = (b1 != 0 || (w & m2) != 0) ? 1u : 0u;
    ctx.gpr[5] = b1;
    ctx.gpr[4] = r;
    ctx.gpr[3] = r;
}

inline f64 FSel(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }  // fsel: c when a >= 0 (not NaN)

// this->+off = x clamped with the constant pool at k (f32 k[0] upper bound,
// f64 k+8 its replacement; f32 k+16 lower bound, f64 k+24 its replacement)
// through fsel; f1-f5 as the original leaves them.
inline void TClampF1(PPUContext& ctx, u32 k, u32 off) {
    const f64 x = ctx.fpr[1];
    const f64 hi = static_cast<f64>(load<f32>(k)), hr = std::bit_cast<f64>(load<u64>(k + 8));
    const f64 lo = static_cast<f64>(load<f32>(k + 16)), lr = std::bit_cast<f64>(load<u64>(k + 24));
    const f64 d1 = static_cast<f64>(static_cast<f32>(hi - x));
    const f64 v1 = static_cast<f64>(static_cast<f32>(FSel(d1, x, hr)));
    const f64 d2 = static_cast<f64>(static_cast<f32>(v1 - lo));
    const f32 v2 = static_cast<f32>(FSel(d2, v1, lr));
    store<u32>(arg<u32>(ctx, 0) + off, std::bit_cast<u32>(v2));
    ctx.fpr[1] = static_cast<f64>(v2);
    ctx.fpr[2] = d2;
    ctx.fpr[3] = lr;
    ctx.fpr[4] = hr;
    ctx.fpr[5] = lo;
}

// V Get(K key): *find((*holder)->+16, &key), the key in a stack local.
inline void THolderMapGet(PPUContext& ctx, u32 holder, u32 find) {
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), arg<u32>(ctx, 0));
    const u32 r = static_cast<u32>(call(ctx, find, {load<u32>(load<u32>(holder) + 16), frame.addr()}));
    ctx.gpr[3] = load<u32>(r);
}

// q = this->+off (not null-checked): q->vfn[slot / 4](q).
inline void TVCallFieldNC(PPUContext& ctx, u32 off, u32 slot) {
    const u32 q = load<u32>(arg<u32>(ctx, 0) + off);
    VCall(ctx, q, slot, {q});
}

}  // namespace obj20
}  // namespace sly4
