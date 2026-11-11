// Game code, part 41: helpers shared by obj41*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the 86
// instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj40.h"

namespace sly4 {
namespace obj41 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj40;

// The raw f32 word of a register value (stfs).
inline u32 W41(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }

// Node* New(s16 id) of 0x00777300 / 0x00778FCC / 0x007791D8 / 0x0077979C: a 24-byte
// node (Mem_Alloc(24, 0, 16); r3 = 0 on failure; Node_Init(m, id, name)):
// its record (the 128-byte entry s16 m->+16 of (*0x00F65038)->+24;
// address 0 for -1, quirk) gets u16 +0 = 16; three bindings of m->+12
// (Node_Bind(it, bn[i], k5[i], k6[i], -1, 0), each with +76 = v76[i]) give their
// +64 bytes to m->+20 + u16 *0x0106CF82 + i. r3 = m.
inline void NodeNew3Bind41(PPUContext& ctx, u32 name, const u32 (&bn)[3], const u32 (&k5)[3], const u32 (&k6)[3], const u32 (&v76)[3]) {
    const u32 id = arg<u32>(ctx, 0);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x00391694u, {m, Sx40(static_cast<u32>(static_cast<s32>(static_cast<s16>(id)))), name});
    const s32 s = static_cast<s16>(load<u16>(m + 16));
    const u32 g = load<u32>(0x00F65038u);
    const u32 rec = s != -1 ? load<u32>(g + 24) + (static_cast<u32>(s) << 7) : 0u;
    store<u16>(rec, 16);
    const u32 dst = load<u32>(m + 20) + load<u16>(0x0106CF82u);
    for (u32 i = 0; i < 3; ++i) {
        const u32 b = static_cast<u32>(call(ctx, 0x004D3088u, {load<u32>(m + 12), bn[i], k5[i], k6[i], ~0ull, 0}));
        const u32 v = load<u32>(b + 64);
        store<u32>(b + 76, v76[i]);
        store<u8>(dst + i, static_cast<u8>(v));
    }
    ret(ctx, m);
}

// u32 f(T* o) of 0x000D6614 / 0x000D6770: adds o to the registry vector R
// (data +0, u16 capacity +4, count +8, high-water +10; doubled with
// PtrVector_Reserve when full; r3 = the high-water read) unless it is
// already listed; then it formats the warning fmt with o->+8's vt[76] name
// and Hash_ToString(o->+4) (0x00AC6AC8 for a missing o / +8; r3 =
// String_Format's).
inline void RegAddUnique41(PPUContext& ctx, u32 R, u32 fmt) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = load<u32>(R);
    const u32 n = load<u16>(R + 8), cap = load<u16>(R + 4);
    bool found = false;
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(d + 4 * i) == o) {
            found = true;
            break;
        }
    }
    if (!found) {
        if (n == cap) call(ctx, 0x00551718u, {R, (cap << 1) & 0x1FFFEu});
        const u32 c = load<u16>(R + 8);
        store<u32>(load<u32>(R) + ((c << 2) & 0x3FFFCu), o);
        const u32 n1 = c + 1;
        const u32 hw = load<u16>(R + 10);
        store<u16>(R + 8, static_cast<u16>(n1));
        store<u16>(R + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
        ret(ctx, hw);
        return;
    }
    u32 name = 0x00AC6AC8u;
    if (o != 0) {
        const u32 c = load<u32>(o + 8);
        if (c != 0) name = static_cast<u32>(VCall(ctx, c, 76, {c}));
    }
    const u32 h = o != 0 ? static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(o + 4)})) : 0x00AC6AC8u;
    call(ctx, 0x005534D0u, {fmt, name, h});
}

// u32 f(T* o) of 0x002E2E78 / 0x002E2FD4: with bit 29 of +100 set,
// unless the world's state (*0x00F65CA0)->vt[32](w, 0) is 1 and its
// state 1 is 10: unless state 1 is 5 the world is asked for states 1 = 10
// and 2 = 0 (World_RequestState); the two words +88 / +92 go to the world
// record w->+20 + u16 w->+6. Then +88 = +92 = 0 and bit 29 of +100
// cleared. r3 = 0.
inline void FlushWorldReq41(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 100) & 0x20000000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 a88 = load<u32>(o + 88), a92 = load<u32>(o + 92);
    auto state = [&](u32 k) {
        const u32 w = load<u32>(0x00F65CA0u);
        return static_cast<u32>(VCall(ctx, w, 32, {w, k}));
    };
    if (state(0) == 1 && state(1) != 10) {
        if (state(1) != 5) {
            call(ctx, 0x004665C0u, {load<u32>(0x00F65CA0u), 1, 10});
            call(ctx, 0x004665C0u, {load<u32>(0x00F65CA0u), 2, 0});
        }
        const u32 w = load<u32>(0x00F65CA0u);
        const u32 r = load<u32>(w + 20) + load<u16>(w + 6);
        store<u32>(r, a88);
        store<u32>(r + 4, a92);
    }
    store<u32>(o + 88, 0);
    store<u32>(o + 92, 0);
    store<u32>(o + 100, load<u32>(o + 100) & ~0x20000000u);
    ret(ctx, 0u);
}

// u32 f(T* o, Node* x) of 0x003EFE40 / 0x003EFFA0: when x is one of the
// u16 counted entries e of o->+112 (count +120; e->+8 == x): x->+220 = 0,
// e's child +28 gets f32 +20 = (o->+184 + its +24) + f32 *kadd, e is freed
// (Mem_FreeFrom(e, 0)) and the last entry moves into its place (count -
// 1). Otherwise x is removed from each list of o->+164 (count u16 +172;
// NodeList_Remove). r3 / f1 as the original leaves them.
inline void NodeSetRemove41(PPUContext& ctx, u32 kadd) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 n = load<u16>(o + 120), d = load<u32>(o + 112);
    u32 i = 0;
    for (; i < n; ++i)
        if (load<u32>(load<u32>(d + 4 * i) + 8) == x) break;
    if (i >= n || n == 0) {
        for (u32 k = 0;; ++k) {
            const u32 m = load<u16>(o + 172);
            if (k >= m) {
                ret(ctx, m);
                return;
            }
            call(ctx, 0x003ED8D0u, {load<u32>(load<u32>(o + 164) + 4 * k), x});
        }
    }
    store<u32>(x + 220, 0);
    const u32 p = d + (i << 2);
    f64 f = Lfs40(o + 184);
    const u32 c = load<u32>(load<u32>(p) + 28);
    if (c != 0) {
        f = F40(F40(f + Lfs40(c + 24)) + Lfs40(kadd));
        store<u32>(c + 20, W41(f));
    }
    ctx.fpr[1] = f;
    const u32 e = load<u32>(p);
    if (e != 0) {
        call(ctx, 0x0054EE6Cu, {e, 0});
        n = load<u16>(o + 120);
        d = load<u32>(o + 112);
    }
    const u32 n1 = n - 1;
    store<u16>(o + 120, static_cast<u16>(n1));
    if (static_cast<s32>(i) == static_cast<s32>(n1 & 0xFFFFu)) {
        ret(ctx, n1 & 0xFFFFu);
        return;
    }
    const u32 v = load<u32>(d + ((n1 << 2) & 0x3FFFCu));
    store<u32>(d + (i << 2), v);
    ret(ctx, v);
}

// The target of an effect command o (0x00088160 / 0x0008D8C4): the
// component e = Effect_FindComponent(x, o->+12) (o->+12 set) of kind 5
// (e->vt[32](e)) whose holder e->+28 is tagged 0x02235C33 (+276) gives
// its +280; failing that Obj_FindById(o->+16) (o->+16 set), else 0.
inline u32 EffTarget41(PPUContext& ctx, u32 o, u32 x) {
    u32 t = 0;
    const u32 c = load<u32>(o + 12);
    if (c != 0) {
        const u32 e = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, c}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 5) {
            const u32 q = load<u32>(e + 28);
            if (q != 0 && load<u32>(q + 276) == 0x02235C33u) t = load<u32>(q + 280);
        }
    }
    if (t == 0) {
        const u32 id = load<u32>(o + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
    }
    return t;
}

// T* f(a, b, Src* s, c) of 0x002203F0 / 0x002210C8 / 0x002251DC: the
// 84-byte definition on the stack (+0 = w0, +4 = w4, +12 = s->+12, +16 /
// +20 = 0x00AD366C, +32 = w32, -1.0 at +36 / +40 / +72, +76 = name, u8
// +80 = 1, zeros elsewhere) is read by reader(a, b, s, c, &def) (+32 = w32
// again afterwards when again32), then copied into a 176-byte object
// (SmallAlloc(176); r3 = 0 on failure): Thunk_000C2B90(p, &def), u8
// +172..+175 = def +80..+83, vtable 0x00B19510, String_CopyN(p + 140, def
// +76, 31), vtable vt. r3 = p.
inline void DefNewSmall41(PPUContext& ctx, u32 reader, u32 w0, u32 w4, u32 w32, bool again32, u32 name, u32 vt) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), s = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    Stack fr(ctx, 96);  // the definition at sp+112 .. sp+195
    const u32 t = fr.addr();
    store<u32>(t + 16, 0x00AD366Cu);
    store<u32>(t + 12, load<u32>(s + 12));
    store<u32>(t + 20, 0x00AD366Cu);
    for (u32 k = 24; k < 32; ++k) store<u8>(t + k, 0);
    store<u32>(t + 32, w32);
    store<u32>(t + 36, 0xBF800000u);
    store<u32>(t + 40, 0xBF800000u);
    store<u8>(t + 44, 0);
    for (u32 k = 48; k < 72; k += 4) store<u32>(t + k, 0);
    store<u32>(t + 72, 0xBF800000u);
    store<u32>(t + 4, w4);
    store<u32>(t + 8, 0);
    store<u32>(t + 76, name);
    store<u8>(t + 80, 1);
    store<u8>(t + 81, 0);
    store<u8>(t + 82, 0);
    store<u8>(t + 83, 0);
    store<u32>(t, w0);
    call(ctx, reader, {a, b, s, c, t});
    if (again32) store<u32>(t + 32, w32);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {176}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000C2B90u, {p, t});
    store<u8>(p + 172, load<u8>(t + 80));
    store<u8>(p + 173, load<u8>(t + 81));
    store<u32>(p, 0x00B19510u);
    const u8 b82 = load<u8>(t + 82);
    const u32 str = load<u32>(t + 76);
    store<u8>(p + 174, b82);
    store<u8>(p + 175, load<u8>(t + 83));
    call(ctx, 0x00553638u, {p + 140, str, 31});
    store<u32>(p, vt);
    ret(ctx, p);
}

// T* New(?, XmlCursor* x) of 0x00075144 / 0x00099B64: a 32-byte object
// (Mem_Alloc(32, 0, 16); r3 = 0 on failure) made by Obj_InitVt_B21C98,
// vtable vt, +12..+24 = 0, u8 +28 = 0, u8 +29 = 1; read: hashes 0x7F247172
// (default 0) / 0x7A30D7E3 / 0x03D1FCFC / 0x04B8CC71 into +12..+24 and the
// bools 0xF5897C08 / 0xF0CA0A04 into u8 +28 / +29 (default the current
// values). r3 = the object.
inline void NewXml6_41(PPUContext& ctx, u32 vt) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, vt);
    store<u32>(p + 24, 0);
    store<u8>(p + 28, 0);
    store<u8>(p + 29, 1);
    call(ctx, 0x004FA14Cu, {x, 0x7F247172u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {x, 0x03D1FCFCu, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004FA14Cu, {x, 0x04B8CC71u, p + 24, load<u32>(p + 24)});
    call(ctx, 0x004F986Cu, {x, 0xF5897C08u, p + 28, load<u8>(p + 28)});
    call(ctx, 0x004F986Cu, {x, 0xF0CA0A04u, p + 29, load<u8>(p + 29)});
    ret(ctx, p);
}

// u32 f(T* o, u32 key) of 0x00177078 / 0x001771DC: the effect o->+off is
// deleted (Effect_Delete) and cleared; the set s = Set_Find(0, 0) looks key
// up in its tree (*(s->+16); nodes: key +4, left +12 for larger keys, else
// +16; the path in the s16 stack s + 20, depth u16 +100): found, s->+104 =
// the node, *0x011DE288 = its +8 and o->+off = Obj_CreateEffectDCA20(o,
// +8, 0, 1) (bit 9 of its +136 cleared); missed, s->+104 = the last node
// visited (0 for an empty tree, *0x011DE288 = 0 too). r3 as the original
// leaves it.
inline void SetPickEffect41(PPUContext& ctx, u32 off) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 e = load<u32>(o + off);
    if (e != 0) call(ctx, 0x004D0280u, {e});
    store<u32>(o + off, 0);
    const u32 s = static_cast<u32>(call(ctx, 0x004BCE90u, {0, 0}));
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 n = load<u32>(load<u32>(s + 16));
    if (n == 0) {
        store<u32>(s + 104, 0);
        store<u32>(0x011DE288u, 0);
        ret(ctx, s);
        return;
    }
    store<u16>(s + 100, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) break;
        const u32 nx = load<u32>(n + (k > key ? 12 : 16));
        if (nx == 0) {
            store<u32>(s + 104, n);
            ret(ctx, s);
            return;
        }
        const s32 c = static_cast<s16>(load<u16>(s + 100));
        store<u16>(s + 100, static_cast<u16>(c + 1));
        store<u32>(s + 20 + (static_cast<u32>(c) << 2), n);
        n = nx;
    }
    store<u32>(s + 104, n);
    store<u32>(0x011DE288u, load<u32>(n + 8));
    const u32 m = load<u32>(s + 104);
    if (m == 0 || load<u32>(m + 8) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x000DCA20u, {o, load<u32>(m + 8), 0, 1}));
    store<u32>(o + off, r);
    if (r == 0) return;
    store<u32>(r + 136, load<u32>(r + 136) & ~0x200u);
}

}  // namespace obj41
}  // namespace sly4
