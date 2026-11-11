// Game code, part 43: the small-function sweep continued from the remaining
// 91 instruction functions (size order; obj42*.cpp did 89-91). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj43.h"

using namespace sly4;
using namespace sly4::obj43;

// ---------------------------------------------------------------- sweep: 91 instructions
// void f(T* o, Hit* h): unless the byte 0x010388F0 is set, bits 17/18 of
// the word o->+188 are set, the owner (Obj_Owner0D93FC) has bit 12 of +40,
// or o->+380 > *0x00229318: when Obj_TestE0BCC(o, h), the strength s =
// h->+60 * t * o->+216->+216 (t = (h+48 - h+96) / (h+48 - h+52) when
// h+96 > h+52 and h+48 > h+52, else *0x0022931C); s > *0x00229318 calls
// o->vt[352](o, s, h->+4, 0x03C599D8); then bit 8 of the u16 h->+14 is
// set. r3 / f1 as the original leaves them.
DECOMP(00229320, Obj_HitReact_00229320) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 gate = load<u8>(0x010388F0u);
    if (gate != 0) {
        ret(ctx, gate);
        return;
    }
    const u64 fl = load<u64>(o + 184);
    if (fl & 0x20000u) {
        ret(ctx, fl);
        return;
    }
    if (fl & 0x40000u) {
        ret(ctx, 0x40000u);
        return;
    }
    if (call(ctx, 0x000D93FCu, {o}) != 0) {
        const u32 ow = static_cast<u32>(call(ctx, 0x000D93FCu, {o}));
        if (load<u32>(ow + 40) & 0x1000u) {
            ret(ctx, 1u);
            return;
        }
    }
    const f64 k0 = Lfs43(0x00229318u);
    const f64 v = Lfs43(o + 380);
    ctx.fpr[1] = v;
    if (v > k0) {
        ret(ctx, 0x00229318u);
        return;
    }
    const u32 hit = static_cast<u32>(call(ctx, 0x000E0BCCu, {o, h}));
    const u32 vt = load<u32>(o);
    if (hit == 0) {
        ret(ctx, vt);
        return;
    }
    const f64 a = Lfs43(h + 96), b = Lfs43(h + 52);
    const u32 q = load<u32>(o + 216);
    f64 t = Lfs43(0x0022931Cu);
    if (a > b) {
        const f64 c = Lfs43(h + 48);
        if (c > b) t = F43(F43(c - a) / F43(c - b));
    }
    t = F43(Lfs43(h + 60) * t);
    t = F43(t * Lfs43(q + 216));
    ctx.fpr[1] = t;
    if (t > k0) {
        ctx.fpr[1] = t;
        VCallOpdR4_28(ctx, o, 352, load<u32>(h + 4), 0x03C599D8u);
    }
    const u32 f = static_cast<u32>(load<u16>(h + 14)) | 0x100u;
    store<u16>(h + 14, static_cast<u16>(f));
    ret(ctx, f);
}

// s32 f(T* o): a weighted random pick among the u16 o->+72 entries of
// o->+64 (48 bytes each, weight +36): r = Rand16 * *0x00264F4C must be <=
// o->+84 and the weight sum above *0x00264F50; then r2 = Rand16 *
// *0x00264F4C * sum walks the weights (r2 <= w picks, else r2 -= w).
// -1 when nothing is picked; f1 as the original leaves it.
DECOMP(00264F58, Pick_WeightedRandom_00264F58) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = Lfs43(0x00264F4Cu);
    ctx.fpr[1] = k;
    const f64 r = F43(Rand16Step43() * k);
    const f64 lim = Lfs43(o + 84);
    if (!(r < lim || r == lim)) {
        ret(ctx, 0xFFFFFFFFu);
        return;
    }
    const u32 n = load<u16>(o + 72);
    const u32 arr = load<u32>(o + 64);
    const f64 z = Lfs43(0x00264F50u);
    f64 sum = z;
    for (u32 i = 0; i < n; ++i) sum = F43(sum + Lfs43(arr + 48 * i + 36));
    if (!(sum > z)) {
        ret(ctx, 0xFFFFFFFFu);
        return;
    }
    f64 x = F43(Rand16Step43() * k);
    x = F43(sum * x);
    ctx.fpr[1] = x;
    if (n == 0) {
        ret(ctx, 0xFFFFFFFFu);
        return;
    }
    for (u32 i = 0;;) {
        const f64 w = Lfs43(arr + 48 * i + 36);
        if (x < w || x == w) {
            ctx.fpr[1] = x;
            ret(ctx, i);
            return;
        }
        ++i;
        x = F43(x - w);
        ctx.fpr[1] = x;
        if (!(i < n)) break;
    }
    ret(ctx, 0xFFFFFFFFu);
}

// T* f(cur, xml, u8 v): with the attribute "%s" 0x00AD4C0C of xml
// (XmlCursor_FindAttrByName), a new 20-byte node (SmallAlloc): vtable
// 0x00B0DAAC, +4 = String_HashNoCase(attr), +8 = 0, +12 = 34, +16 = 0;
// {hash, node} pushed on the registry array 0x01056F04 (grown with
// Array_Reserve to twice its capacity when full; +4 high-water mark, bit
// 0 of +6 set); vtable 0x00B0EF54. Then node->+16 = v; r3 = node. Without
// the attribute: XmlCursor_DocPath(cur), r3 = 0. A failed allocation
// returns 0 after storing v to address 16 (the null node's +16).
DECOMP(00280614, Node_NewFromXml_00280614) {
    const u32 cur = arg<u32>(ctx, 0), xml = arg<u32>(ctx, 1);
    const u8 v = static_cast<u8>(arg<u32>(ctx, 2));
    const u32 attr = static_cast<u32>(call(ctx, 0x004F9628u, {xml, 0x00AD4C0Cu}));
    if (attr == 0) {
        call(ctx, 0x004F8944u, {cur});
        ret(ctx, 0u);
        return;
    }
    const u32 n = static_cast<u32>(call(ctx, 0x0055238Cu, {20}));
    if (n == 0) {
        store<u8>(16, v);  // quirk: written through the failed allocation
        ret(ctx, 0u);
        return;
    }
    store<u32>(n, 0x00B0DAACu);
    u32 hash = static_cast<u32>(call(ctx, 0x0056A710u, {attr}));
    store<u32>(n + 8, 0);
    store<u8>(n + 16, 0);
    store<u32>(n + 4, hash);
    store<u32>(n + 12, 34);
    const u32 ar = 0x01056F04u;
    const u32 cap = load<u16>(ar);
    u32 cnt = load<u16>(ar + 2);
    if (cnt == cap) {
        call(ctx, 0x00558F3Cu, {ar, (cap << 1) & 0xFFFEu});
        hash = load<u32>(n + 4);
        cnt = load<u16>(ar + 2);
    }
    const u32 data = load<u32>(ar + 8);
    store<u32>(data + ((cnt << 3) & 0x7FFF8u), hash);
    store<u32>(data + (static_cast<u32>(load<u16>(ar + 2)) << 3) + 4, n);
    const u32 c1 = static_cast<u32>(load<u16>(ar + 2)) + 1;
    const u32 hw = load<u16>(ar + 4);
    const u32 fl = load<u16>(ar + 6);
    store<u16>(ar + 2, static_cast<u16>(c1));
    store<u16>(ar + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c1 & 0xFFFFu) ? hw : c1));
    store<u16>(ar + 6, static_cast<u16>(fl | 1u));
    store<u32>(n, 0x00B0EF54u);
    store<u8>(n + 16, v);
    ret(ctx, n);
}

// ~T(T* o): vtable 0x00B1C5D0, the o->+28 220-byte records at +24 freed
// (SmallFree), +28 = 0; with o->+8 and its +164,
// Obj_FindPair_0034F580(+164, 0x00F352F0, o); vtable 0x00B1BE30, o removed
// from the registry array 0x01056F1C ({key, T*} pairs, the later ones
// moved down), then List1013B5C_Remove(o); r3 = its.
DECOMP(002869A0, Registered_Dtor_002869A0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 buf = load<u32>(o + 24);
    store<u32>(o, 0x00B1C5D0u);
    if (buf != 0) call(ctx, 0x00552420u, {buf, load<u32>(o + 28) * 220u});
    store<u32>(o + 28, 0);
    const u32 p = load<u32>(o + 8);
    if (p != 0) {
        const u32 x = load<u32>(p + 164);
        if (x != 0) call(ctx, 0x0034F580u, {x, 0x00F352F0u, o});
    }
    store<u32>(o, 0x00B1BE30u);
    const u32 ar = 0x01056F1Cu;
    for (u32 i = 0;; ++i) {
        const u32 cnt = load<u16>(ar + 2);
        if (!(i < cnt)) break;
        const u32 data = load<u32>(ar + 8);
        if (load<u32>(data + 8 * i + 4) != o) continue;
        u32 c = cnt;
        if (static_cast<s32>(c) >= 2 && static_cast<s32>(i) < static_cast<s32>(c - 1)) {
            for (u32 j = i;;) {
                const u32 e = data + 8 * j;
                ++j;
                store<u32>(e, load<u32>(e + 8));
                store<u32>(e + 4, load<u32>(e + 12));
                c = load<u16>(ar + 2);
                if (!(static_cast<s32>(j) < static_cast<s32>(c - 1))) break;
            }
        }
        store<u16>(ar + 2, static_cast<u16>(c - 1));
        break;
    }
    ret(ctx, call(ctx, 0x000D6940u, {o}));
}

// Builds the two particle pools: CallbackList_Insert(0x0119582C,
// 0x00F35360, 0, 0, 0); then 0x00F64A50 / 0x00F64A54 = a 36-byte pool
// (Mem_Alloc(36, 0, 16)) over a 2208 / 5632-byte buffer (Mem_Alloc(n,
// 0, 16)): +0 n, +4 / +12 buf, +8 / +16 buf + n, +20 / +24 = 0, +28 = 1,
// bits 6 and 7 of +32 cleared (0 when the pool allocation fails). r3 =
// 0x00F60000 as the original leaves it.
static u32 NewPool43(PPUContext& ctx, u32 n, bool second) {
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {36, 0, 16}));
    if (p == 0) return 0;
    store<u32>(p, n);
    const u32 buf = static_cast<u32>(call(ctx, 0x0054ED38u, {n, 0, 16}));
    const u32 end = buf + load<u32>(p);
    const u8 b = load<u8>(p + 32);
    store<u32>(p + 4, buf);
    store<u32>(p + 12, buf);
    store<u32>(p + 24, 0);
    if (second) store<u32>(p + 20, 0);  // the second copy stores +20 first
    store<u32>(p + 8, end);
    store<u32>(p + 16, end);
    if (!second) store<u32>(p + 20, 0);
    store<u32>(p + 28, 1);
    store<u8>(p + 32, static_cast<u8>(b & 0x3Fu));
    return p;
}
DECOMP(0028BDB0, Pools_Init_0028BDB0) {
    call(ctx, 0x005131E8u, {0x0119582Cu, 0x00F35360u, 0, 0, 0});
    store<u32>(0x00F64A50u, NewPool43(ctx, 2208, false));
    store<u32>(0x00F64A54u, NewPool43(ctx, 5632, true));
    ret(ctx, 0x00F60000u);
}

// u8 f(-, u32 key): for the current player p = (*0x00F64C20)->+36 (none:
// 1), the level byte L = byte (G + 4056 * byte G+4208 + 4 * p->+822) +498
// of G = *0x00F65CA0, less one when the u16 at +496 has bits 3-4 = 2
// and L > 0; the first of the u8 p->+720 536-byte entries of p->+716
// whose byte +501 is L and with a {key, -, u8} table at +448 (u16 count
// +456) holding key: that row's byte +8; else 0.
DECOMP(002E1B1C, Player_LevelKeyByte_002E1B1C) {
    const u32 key = arg<u32>(ctx, 1);
    const u32 p = load<u32>(load<u32>(0x00F64C20u) + 36);
    if (p == 0) {
        ret(ctx, 1u);
        return;
    }
    u32 g = load<u32>(0x00F65CA0u);
    const u32 b = load<u8>(p + 822);
    g = g + static_cast<u32>(load<u8>(g + 4208)) * 4056u;
    u32 lvl = load<u8>(g + 4 * b + 498);
    if (((load<u16>(g + 4 * b + 496) >> 3) & 3u) == 2 && lvl != 0) lvl -= 1;
    for (u32 i = 0, off = 0; i < load<u8>(p + 720); ++i, off += 536) {
        if (load<u8>(load<u32>(p + 716) + off + 501) != lvl) continue;
        const u32 n = load<u16>(load<u32>(p + 716) + off + 456);
        for (u32 j = 0; j < n; ++j) {
            const u32 t = load<u32>(load<u32>(p + 716) + off + 448);
            if (load<u32>(t + 12 * j) == key) {
                ret(ctx, load<u8>(t + 12 * j + 8));
                return;
            }
        }
    }
    ret(ctx, 0u);
}

// u32 f(T* o, u8 on): bit 27 of o->+100 = bit 0 of on; r3 = the word.
// Switching off first walks the u16 +88 entries of the list (G + 4208)
// +80 (G = *0x00F65CA0): an entry e whose kind ((G + 4208 + 0x20074)[e->
// +2452] >> 28) is 4 and whose e->vt[52](e, &h) (h three words zeroed)
// gives a handle h found in the map 0x0106C228 (SortedMap_Find3 with a
// set slot) gets Handle_Notify36742C(h, 1, 1, 0).
DECOMP(002E6E54, Obj_SetBit27Notify_002E6E54) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    if ((on & 0xFFu) == 0) {
        const u32 l = load<u32>(0x00F65CA0u) + 4208;
        Stack fr(ctx, 16);  // sp+112 (key) and sp+116 (three words)
        const u32 k = fr.addr(), h = k + 4;
        for (u32 i = 0; i < load<u16>(l + 88); ++i) {
            const u32 e = load<u32>(load<u32>(l + 80) + 4 * i);
            if ((load<u32>(l + (load<u32>(e + 2452) << 2) + 0x20074u) >> 28) != 4) continue;
            store<u32>(h, 0);
            store<u32>(h + 4, 0);
            store<u32>(h + 8, 0);
            if (static_cast<u32>(VCall(ctx, e, 52, {e, h})) == 0) continue;
            const u32 hv = load<u32>(h);
            store<u32>(k, hv);
            if (hv == 0) continue;
            const u32 slot = static_cast<u32>(call(ctx, 0x0076611Cu, {0x0106C228u, k}));
            if (load<u32>(slot) == 0) continue;
            call(ctx, 0x0036742Cu, {load<u32>(h), 1, 1, 0});
        }
    }
    const u32 w = (load<u32>(o + 100) & ~0x08000000u) | ((on & 1u) << 27);
    store<u32>(o + 100, w);
    ret(ctx, w);
}

// u32 f(T* o): unless the byte 0x01068890 is set (r3 = it), steps a
// cursor (hi 16 = entry, lo 16 = the next one, (entry +12 >> 15) of the
// 16-byte entries o->+28) o->+16 times from entry o->+8: each step calls
// entry->+8->vt[12](); then moves on to entry->+4 unless the entry is
// o->+12 (cursor 0, entry 0 next). r3 = the last cursor (0 when o->+16
// is 0).
DECOMP(002EF36C, Cursor_StepCalls_002EF36C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 gate = load<u8>(0x01068890u);
    if (gate != 0) {
        ret(ctx, gate);
        return;
    }
    const u32 n = load<u32>(o + 16);
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 idx = load<u32>(o + 8);
    u32 c = (idx << 16) | ((load<u32>(load<u32>(o + 28) + (idx << 4) + 12) >> 15) & 0xFFFFu);
    for (u32 k = 0;;) {
        const u32 cur = c >> 16;
        const u32 obj = load<u32>(load<u32>(o + 28) + (cur << 4) + 8);
        VCall(ctx, obj, 12, {obj});
        if (static_cast<s32>(cur) != static_cast<s32>(load<u32>(o + 12))) {
            const u32 arr = load<u32>(o + 28);
            const u32 nx = load<u32>(arr + (cur << 4) + 4);
            c = (nx << 16) | ((load<u32>(arr + (nx << 4) + 12) >> 15) & 0xFFFFu);
        } else {
            c = 0;
        }
        if (!(++k < n)) break;
    }
    ret(ctx, c);
}

// void f(T* o): when the player handle *0x00F64CCC resolves
// (Handle_ResolveBit1), plays on it (Obj_SetAnim(h, name, 1, -, 0, 0,
// 0x01164CA4; *0x003BC534)) the anim 0x00ADF4E8 with o->+592 set, else
// 0x00ADF4B8 when the current player is missing or SlotRec_Flag40 of it,
// else 0x00ADF4D0. Then the component 0xDB0CC563 of o->+32
// (Effect_FindComponent) whose vt[32]() is 5 has its +28->vt[40](1, 1)
// called. r3 as the original leaves it.
DECOMP(003BC538, Obj_PlayerAnimKick_003BC538) {
    const u32 o = arg<u32>(ctx, 0);
    if (call(ctx, 0x003568E0u, {load<u32>(0x00F64CCCu)}) != 0) {
        bool alt = true;
        const u32 p = load<u32>(load<u32>(0x00F64C20u) + 36);
        if (p != 0 && call(ctx, 0x002DA89Cu, {p}) == 0) alt = false;
        const u32 h = static_cast<u32>(call(ctx, 0x003568E0u, {load<u32>(0x00F64CCCu)}));
        const u32 name = load<u32>(o + 592) != 0 ? 0x00ADF4E8u : (alt ? 0x00ADF4B8u : 0x00ADF4D0u);
        // r6 is not set (Obj_SetAnim does not read it)
        callf(ctx, 0x00363A3Cu, {h, name, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {Lfs43(0x003BC534u)});
    }
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {load<u32>(o + 32), 0xDB0CC563u}));
    if (c == 0) {
        ret(ctx, 0u);
        return;
    }
    const u64 k = VCall(ctx, c, 32, {c});
    if (static_cast<u32>(k) != 5) return;
    const u32 c2 = load<u32>(c + 28);
    if (c2 == 0) return;
    VCall(ctx, c2, 40, {c2, 1, 1});
}

// void f(T* o, u32 key; f32 t): looks key up in the AVL tree o->+160->*
// (+4 key, +12 / +16 the subtrees for a greater / not greater node key),
// recording the path in the s16-counted array o->+164 (count o->+244).
// Found: o->+248 = node, *0x011DE488 = node->+8 and, when
// Tree_FindU32_0077C6F8(o + 148, key, &out, o->+148), key is removed
// (Avl_Remove_0077BB08(o + 148, o + 148, key, &changed)) and o->+152
// decremented. Missing: o->+248 = the last node visited (an empty tree
// zeroes +248 and *0x011DE488). Then 0x0077CBE0(o + 148, key; t); r3 =
// its.
DECOMP(003DE47C, Track_SelectKey_003DE47C) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const f64 t = farg(ctx, 0);
    u32 n = load<u32>(load<u32>(o + 160));
    if (n == 0) {
        store<u32>(o + 248, 0);
        store<u32>(0x011DE488u, 0);
    } else {
        store<u16>(o + 244, 0);
        for (;;) {
            const u32 cur = n;
            const u32 k = load<u32>(cur + 4);
            if (k == key) {
                store<u32>(o + 248, cur);
                const u32 fw = load<u32>(cur + 8);
                Stack fr(ctx, 16);  // sp+112 (found slot) and the byte sp+116
                store<u8>(fr.addr() + 4, 0);
                store<u32>(0x011DE488u, fw);
                ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(fw));
                if (call(ctx, 0x0077C6F8u, {o + 148, key, fr.addr(), load<u32>(o + 148)}) != 0) {
                    call(ctx, 0x0077BB08u, {o + 148, o + 148, key, fr.addr() + 4});
                    store<u32>(o + 152, load<u32>(o + 152) - 1);
                }
                break;
            }
            n = k > key ? load<u32>(cur + 12) : load<u32>(cur + 16);
            if (n == 0) {
                store<u32>(o + 248, cur);
                break;
            }
            const s32 i = static_cast<s16>(load<u16>(o + 244));
            store<u16>(o + 244, static_cast<u16>(i + 1));
            store<u32>(o + 164 + 4 * static_cast<u32>(i), cur);
        }
    }
    callf(ctx, 0x0077CBE0u, {o + 148, key}, {t});  // r3 = its
}

// u32 f(T* o): for an active o (byte +68): World_ListRemove_0046B7A4(
// *0x00F65CA0, 0, 0x00F387E0, o, 0), Handle_Notify36742C(0x6D7DA1D8, 0,
// 1, 0), SetSelect_491000(o, 0x74128921), o->+112->vt[88](0, 1, 1, 1)
// and the same for o->+92; then, when the object w = (o->vt[76]())->
// +0x1281E0 has the word o->+60 (Obj_HasWord368_000FC5DC),
// Obj_PopLayer(w, o->+60; *0x00419F2C). r3 as the original leaves it.
DECOMP(00419F30, Obj_Deactivate_00419F30) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = load<u8>(o + 68);
    if (on == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 0, 0x00F387E0u, o, 0});
    call(ctx, 0x0036742Cu, {0x6D7DA1D8u, 0, 1, 0});
    call(ctx, 0x00491000u, {o, 0x74128921u});
    const u32 a = load<u32>(o + 112);
    VCall(ctx, a, 88, {a, 0, 1, 1, 1});
    const u32 b = load<u32>(o + 92);
    VCall(ctx, b, 88, {b, 0, 1, 1, 1});
    const u32 x = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 w = load<u32>(x + 0x1281E0u);
    if (w == 0) return;  // r3 = x
    if (call(ctx, 0x000FC5DCu, {w, load<u32>(o + 60)}) == 0) return;
    const u32 l = load<u32>(o + 60);
    callf(ctx, 0x000FBB88u, {w, l, l}, {Lfs43(0x00419F2Cu)});
}

// u32 f(T* o): with a o->+544, bit 7 of the byte o->+556 = !+544->vt[24]().
// Unless that bit is set: with bit 3 Player_StartAction(G, (G + 4208)->
// +8, 0x00AE24E4, 0x00AE24FC, 0x00F38E78, o, 0x00F38E80, o, 1, 1) (G =
// *0x00F65CA0), else bit 6 is set and 0x004890A0(G, o + 9, o->+524,
// o + 528, o + 536, bit 2, bit 1, o + 548, o + 552, bit 0). Then
// Flag_Set8_40000000(o); r3 = its.
DECOMP(00433D98, Obj_StartOrQueue_00433D98) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 544);
    u32 b = load<u8>(o + 556);
    if (p != 0) {
        const u32 x = static_cast<u32>(VCall(ctx, p, 24, {p}));
        b = (load<u8>(o + 556) & 0x7Fu) | (x == 0 ? 0x80u : 0u);
        store<u8>(o + 556, static_cast<u8>(b));
    }
    if (!(b & 0x80u)) {
        const u32 g = load<u32>(0x00F65CA0u);
        if (b & 0x08u) {
            call(ctx, 0x00489A60u, {g, load<u32>(g + 4208 + 8), 0x00AE24E4u, 0x00AE24FCu, 0x00F38E78u, o, 0x00F38E80u, o, 1, 1});
        } else {
            b |= 0x40u;
            store<u8>(o + 556, static_cast<u8>(b));
            const u64 n = static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(o + 524))));
            call(ctx, 0x004890A0u, {load<u32>(0x00F65CA0u), o + 9, n, o + 528, o + 536, (b >> 2) & 1u, (b >> 1) & 1u, o + 548, o + 552, b & 1u});
        }
    }
    ret(ctx, call(ctx, 0x0042CA60u, {o}));
}

// bool f(-, T* o): s = o->+656 + 4; a = s->+28, b = s->+8; whether a->
// vt[12]() and bit 6 of byte a->+20 hold (h); y = b->vt[12](). True when
// bit 0 of o->+256 and the byte 0x00F66FD4 are both set, or the byte
// 0x00F66FC0 is set with h or y, or o->+512 is 0 / *0x00F66FBC
// and the byte 0x010B9F38 is set.
DECOMP(0045A09C, Obj_CanAct_0045A09C) {
    const u32 o = arg<u32>(ctx, 1);
    const u32 s = load<u32>(o + 656) + 4;
    const u32 f = load<u32>(o + 256) & 1u & load<u8>(0x00F66FD4u);
    const u32 a = load<u32>(s + 28);
    const u32 b = load<u32>(s + 8);
    const u32 g = load<u8>(0x00F66FC0u);
    bool h = false;
    if (VCall(ctx, a, 12, {a}) != 0 && (load<u8>(a + 20) & 0x40u)) h = true;
    const u32 y = static_cast<u32>(VCall(ctx, b, 12, {b}));
    const u32 q = load<u32>(o + 512);
    bool z = false;
    if (q == 0 || q == load<u32>(0x00F66FBCu)) z = load<u8>(0x010B9F38u) != 0;
    u32 r = 0;
    if (f != 0) r = 1;
    else if ((h || y != 0) && g != 0) r = 1;
    else if (z) r = 1;
    ret(ctx, r);
}

// u32 f(-, u32 i, u8 on): the player p = (G + 0x20C80 * i + 4208)->+8 (G
// = *0x00F65CA0) with bit 3 of p->+192 first gets: w = Obj_Notify26(p);
// k = 1 unless p->vt[32](p, 0) = 0 and p->vt[32](p, 1) = 6; with w,
// 0x002ABB60(w, k && (on || byte (*0x00F557F0)->+28)), and that flag
// replaces on. Then Effect_SetOnOffHash_00369874(u8 i, on); r3 = its.
DECOMP(0047BCF4, Player_SetEffectOnOff_0047BCF4) {
    const u32 i = arg<u32>(ctx, 1);
    u32 on = arg<u32>(ctx, 2) & 0xFFu;
    const u32 p = load<u32>(load<u32>(0x00F65CA0u) + i * 0x20C80u + 4208 + 8);
    if (p != 0 && (load<u32>(p + 192) & 8u)) {
        const u32 w = static_cast<u32>(call(ctx, 0x000D94CCu, {p}));
        const u32 any = (on | load<u8>(load<u32>(0x00F557F0u) + 28)) != 0 ? 1u : 0u;
        u32 k = 0;
        if (static_cast<u32>(VCall(ctx, p, 32, {p, 0})) != 0) k = 1;
        else if (static_cast<u32>(VCall(ctx, p, 32, {p, 1})) != 6) k = 1;
        on = any & k;
        if (w != 0) call(ctx, 0x002ABB60u, {w, on});
    }
    ret(ctx, call(ctx, 0x00369874u, {i & 0xFFu, on}));
}

// bool f(FileReader* r): reads the chunk (Iff_ChunkSize bytes; 0 -> r3 =
// 0) into a MemStack_Alloc block of the default stack, opens it as a zip
// (Archive_OpenZip(buf, n, *0x00F65FF0)), loads its xmlb into a local
// XmlDoc (sp+112) and, when that works, Set_AddXml(Set_Find(0), zip + 4,
// &doc, 0); the doc destroyed, zip->vt[8](), *0x00F65FF0 = 0 and the
// block freed through the default stack's vt[8](stack, buf). r3 = 1.
DECOMP(004BE850, Set_LoadZipChunk_004BE850) {
    const u32 rd = arg<u32>(ctx, 0);
    const u32 n = static_cast<u32>(call(ctx, 0x005637D4u, {rd}));
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 ms = static_cast<u32>(call(ctx, 0x005525FCu, {}));
    const u32 buf = static_cast<u32>(call(ctx, 0x0055260Cu, {ms, n}));
    call(ctx, 0x00563768u, {rd, buf, n});
    const u32 zip = static_cast<u32>(call(ctx, 0x00567034u, {buf, n, load<u32>(0x00F65FF0u)}));
    const u32 set = static_cast<u32>(call(ctx, 0x004BCE90u, {0}));
    {
        Stack fr(ctx, 2248);  // the XmlDoc at sp+112 (up to the saves at sp+2360)
        const u32 doc = fr.addr();
        call(ctx, 0x004FA7A4u, {doc});
        if (call(ctx, 0x004FACCCu, {doc, zip, 0}) != 0) call(ctx, 0x004BDA70u, {set, zip + 4, doc, 0});
        call(ctx, 0x004FA9C0u, {doc});
    }
    VCall(ctx, zip, 8, {zip});
    store<u32>(0x00F65FF0u, 0);
    const u32 st = static_cast<u32>(call(ctx, 0x005525FCu, {}));
    VCall(ctx, st, 8, {st, buf});
    ret(ctx, 1u);
}

// u32 f(Binding* b, T* e): the component b->+12 of e (Effect_FindComponent;
// missing: Hash_ToString(b->+12)) whose vt[32]() is 5 gets the target t =
// Effect_Bind4BB7E0(e, b->+16, byte b->+24) (missing: Hash_ToString(b->
// +16)); with a b->+20, t is replaced by the world object found by
// WorldObj_ForEach(t, 0x00F3A710, {b->+20, 0}, 0) (none: Hash_ToString(
// b->+20)). The old c->+28 gets vt[8](), c->+28 = t (its u16 +4 counted
// up) and the byte c->+32 = 0; r3 = 0. Other exits leave r3.
DECOMP(004C1E6C, Binding_Attach_004C1E6C) {
    const u32 b = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, load<u32>(b + 12)}));
    if (c == 0) {
        call(ctx, 0x0056A6E0u, {load<u32>(b + 12)});
        return;
    }
    if (static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) return;
    u32 t = static_cast<u32>(call(ctx, 0x004BB7E0u, {e, load<u32>(b + 16), load<u8>(b + 24)}));
    if (t == 0) {
        call(ctx, 0x0056A6E0u, {load<u32>(b + 16)});
        return;
    }
    const u32 key = load<u32>(b + 20);
    if (key != 0) {
        Stack fr(ctx, 16);  // {key, found} at sp+112
        const u32 q = fr.addr();
        store<u32>(q, key);
        store<u32>(q + 4, 0);
        call(ctx, 0x0057F5D8u, {t, 0x00F3A710u, q, 0});
        const u32 f = load<u32>(q + 4);
        if (f != 0) t = f;
        else call(ctx, 0x0056A6E0u, {load<u32>(b + 20)});
    }
    const u32 old = load<u32>(c + 28);
    if (old != 0) VCall(ctx, old, 8, {old});
    store<u32>(c + 28, t);
    store<u16>(t + 4, static_cast<u16>(load<u16>(t + 4) + 1));
    store<u8>(c + 32, 0);
    ret(ctx, 0u);
}

// The deleting ~T() of the shape classes 0x0075A4FC / 0x0075B074: vtable
// vt0 and pre(o), then vtable 0x00B19558, Volume_Notify(*0x00F557EC, o +
// 424), Shape_Release(o + 424), the arrays +388 / +364 and the PtrVector
// +344 destroyed, Thunk_000D729C(o) and SmallFree(o, 624); r3 = its (0 for
// a null o).
static void ShapeDelete43(PPUContext& ctx, u32 vt0, u32 pre) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o, vt0);
    call(ctx, pre, {o});
    store<u32>(o, 0x00B19558u);
    call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), o + 424});
    call(ctx, 0x00638498u, {o + 424});
    ArrayDtor42(ctx, o + 388);
    ArrayDtor42(ctx, o + 364);
    VecDtor(ctx, o + 344);
    call(ctx, 0x000D729Cu, {o});
    ret(ctx, call(ctx, 0x00552420u, {o, 624}));
}
// vtable 0x00B19950, ShapeObj_RefreshPhantom_002108A8 first.
DECOMP(0075A4FC, ShapeObj_Delete_0075A4FC) { ShapeDelete43(ctx, 0x00B19950u, 0x002108A8u); }

// vtable 0x00B19CC8, 0x00213398 first.
DECOMP(0075B074, ShapeObj_Delete_0075B074) { ShapeDelete43(ctx, 0x00B19CC8u, 0x00213398u); }

// void f(Vec3* out, const Ref* r): with the table T = (*0x00F65038)->+24
// (128-byte rows), a = T row s16 r->+4 and b = T row s16 r->+2 (0 for
// -1): the three bytes +15 / +16 / +17 of e (= b + u16 *0x0106CF7E when
// the byte b->+2 is set, else 0) index f32s at a + u16 *0x0106CF70; out =
// them (f1 = the second, as the original leaves it).
DECOMP(007753E4, Ref_GetVec3_007753E4) {
    const u32 out = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    const u32 g = load<u32>(0x00F65038u);
    const s32 ia = static_cast<s16>(load<u16>(r + 4));
    const s32 ib = static_cast<s16>(load<u16>(r + 2));
    const u32 a = ia != -1 ? load<u32>(g + 24) + (static_cast<u32>(ia) << 7) : 0u;
    const u32 b = ib != -1 ? load<u32>(g + 24) + (static_cast<u32>(ib) << 7) : 0u;
    const u32 base = a + load<u16>(0x0106CF70u);
    u32 v[3];
    for (u32 k = 0; k < 3; ++k) {
        const u32 e = load<u8>(b + 2) != 0 ? b + load<u16>(0x0106CF7Eu) : 0u;
        v[k] = load<u32>(base + (static_cast<u32>(load<u8>(e + 15 + k)) << 2));
    }
    store<u32>(out, v[0]);
    store<u32>(out + 4, v[1]);
    store<u32>(out + 8, v[2]);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v[1]));
}

// ~T(T* o) body: vtable 0x00B22710; o->+152 (the first child) gets
// vt[4]() until it is gone; unless bit 24 of o->+136, o is unlinked from
// its parent's (o->+148) child list +152 / the root list *0x00F66868
// (siblings linked by +156) and +156 = 0; then o is cleared from the four
// slots 0x010C2AD8. r3 = 0x010C2AD8 as the original leaves it. Three
// identical copies: 0x00794A7C / 0x00794BE8 / 0x00794D54.
static void NodeDtor43(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B22710u);
    for (u32 c = load<u32>(o + 152); c != 0; c = load<u32>(o + 152)) VCall(ctx, c, 4, {c});
    if (!(load<u32>(o + 136) & 0x01000000u)) {
        const u32 par = load<u32>(o + 148);
        const u32 head = par != 0 ? load<u32>(par + 152) : load<u32>(0x00F66868u);
        if (head == o) {
            if (par != 0) store<u32>(par + 152, load<u32>(o + 156));
            else store<u32>(0x00F66868u, load<u32>(o + 156));
        } else {
            u32 prev = head;
            while (load<u32>(prev + 156) != o) prev = load<u32>(prev + 156);
            store<u32>(prev + 156, load<u32>(o + 156));
        }
        store<u32>(o + 156, 0);
    }
    const u32 t = 0x010C2AD8u;
    for (u32 i = 0; i < 4; ++i)
        if (o != 0 && load<u32>(t + 4 * i) == o) store<u32>(t + 4 * i, 0);
    ret(ctx, t);
}
DECOMP(00794A7C, Node_Dtor_00794A7C) { NodeDtor43(ctx); }

// The same as 0x00794A7C.
DECOMP(00794BE8, Node_Dtor_00794BE8) { NodeDtor43(ctx); }

// The same as 0x00794A7C.
DECOMP(00794D54, Node_Dtor_00794D54) { NodeDtor43(ctx); }
