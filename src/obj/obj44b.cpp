// Game code, part 44 (continued): more of the small-function sweep (94+
// instructions). Each was checked against the original in the PPC
// simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj44.h"

using namespace sly4;
using namespace sly4::obj44;

// ---------------------------------------------------------------- sweep: 94 instructions
// u32 f(T* o): the 4x3 transform of the body o->+416 (rows +240, +256,
// +272, +288; x, y, z) into o->+96..+152 (rows 16 bytes apart) through a
// stack copy. r3 = o; f1 = the last float copied.
DECOMP(0024B064, Obj_PullBodyXform_0024B064) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(o + 416);
    if (b == 0) {
        ret(ctx, o);
        return;
    }
    Stack frame(ctx, 64);
    const u32 s = frame.addr();  // the original's sp + 48 .. + 104
    const u32 m = b + 240;
    for (u32 r = 0; r < 4; ++r)
        for (u32 j = 0; j < 3; ++j) Cp44(s + 16 * r + 4 * j, m + 16 * r + 4 * j);
    for (u32 r = 0; r < 4; ++r)
        for (u32 j = 0; j < 3; ++j) Cp44(o + 96 + 16 * r + 4 * j, s + 16 * r + 4 * j);
    ctx.fpr[1] = Lfs44(s + 56);
    ret(ctx, o);
}

// void f(T* o) (destructor): vtable 0x00B1BFD8, the 6 parts at +372 get
// vt[4]; then the 0x00B1BE30 base teardown (SlotsDtorUnlist44).
DECOMP(0026F914, Obj_Dtor_B1BFD8_0026F914) { SlotsDtorUnlist44(ctx, 0x00B1BFD8u, 372, 6, 4); }

// void f(T* o) (destructor): vtable 0x00B1C6D8, the 8 parts at +2912 get
// vt[8]; then the 0x00B1BE30 base teardown (SlotsDtorUnlist44).
DECOMP(0028E9F8, Obj_Dtor_B1C6D8_0028E9F8) { SlotsDtorUnlist44(ctx, 0x00B1C6D8u, 2912, 8, 8); }

// void f(T* o) (destructor): when the skeleton +28 is set: vtable
// 0x00B1CF80, p = HkProxy_Get9CEEAC(+28); with the world +24->+4:
// Body_Remove931F00(world, p) and AnimSkel_Release7D4EA8(p); then
// AnimSkel_Release7D4EA8(+28) and +28 = 0. Then vtable 0x00B1BE30, o taken
// out of 0x01056F1C (Unlist1056F1C39) and List1013B5C_Remove(o) (r3 =
// its).
DECOMP(002BBC18, Obj_Dtor_B1CF80_002BBC18) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 sk = load<u32>(o + 28);
    if (sk != 0) {
        const u32 w = load<u32>(load<u32>(o + 24) + 4);
        store<u32>(o, 0x00B1CF80u);
        const u32 p = static_cast<u32>(call(ctx, 0x009CEEACu, {sk}));
        if (w != 0) {
            call(ctx, 0x00931F00u, {w, p});
            call(ctx, 0x007D4EA8u, {p});
        }
        call(ctx, 0x007D4EA8u, {load<u32>(o + 28)});
        store<u32>(o + 28, 0);
    }
    store<u32>(o, 0x00B1BE30u);
    sly4::obj39::Unlist1056F1C39(o);
    call(ctx, 0x000D6940u, {o});
}

// u32 f(T* o, x, f32 t): walks o->+16 steps of the 16-byte node array
// o->+28 from node o->+8, calling node->+8->vt[28](node->+8, x; t) for each
// and following the +4 links, until node o->+12 is reached. The cursor is
// a stack word (u16 index, u16 (+12 >> 15)). r3 = the final cursor (0 at
// the end node or for no steps).
DECOMP(002EF7B8, Path_Walk_002EF7B8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const f64 t = farg(ctx, 0);
    const u32 n = load<u32>(o + 16);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();  // the original's sp + 112 (cursor) / + 116 (temp)
    if (n == 0) {
        store<u32>(s + 4, 0);
        store<u16>(s + 6, 0);
        const u32 w = load<u32>(s + 4);
        store<u32>(s, w);
        ret(ctx, w);
        return;
    }
    const u32 i0 = load<u32>(o + 8);
    const u32 b0 = load<u32>(o + 28);
    store<u32>(s + 4, 0);
    store<u16>(s + 4, static_cast<u16>(i0));
    const u32 v0 = load<u32>(b0 + (i0 << 4) + 12);
    store<u16>(s + 6, static_cast<u16>(v0 >> 15));
    u32 w = load<u32>(s + 4);
    store<u32>(s, w);
    for (u32 k = 0;;) {
        const u32 idx = load<u16>(s);
        const u32 obj = load<u32>(load<u32>(o + 28) + (idx << 4) + 8);
        ctx.fpr[1] = t;
        VCall(ctx, obj, 28, {obj, x});
        if (static_cast<s32>(idx) != static_cast<s32>(load<u32>(o + 12))) {
            const u32 b = load<u32>(o + 28);
            const u32 nx = load<u32>(b + (idx << 4) + 4);
            const u32 v = load<u32>(b + (nx << 4) + 12);
            w = (nx << 16) | ((v >> 15) & 0xFFFFu);
        } else {
            w = 0;
        }
        ++k;
        store<u32>(s, w);
        if (!(k < n)) break;
    }
    ret(ctx, w);
}

// u32 f(T* a, T* b): with c = b->+12: unless the byte of the 128-byte
// record table (*0x00F65038)->+24 [a's s16 +4] selected by the u16 table
// offsets 0x0106CF70 / 0x0106CF7E and the record of a's s16 +2 equals
// *0x003967A8, c->vt[84](c, &m) with a stack copy of c's 4x3 transform
// (rows +32..+80). The quirk: with the +2 record's byte +2 zero the
// selector byte is read from address 0. r3 = 1 (0 without c).
DECOMP(003967B0, Obj_SyncXform_003967B0) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 c = load<u32>(arg<u32>(ctx, 1) + 12);
    if (c == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 g = load<u32>(0x00F65038u);
    const s32 i4 = static_cast<s16>(load<u16>(a + 4));
    u32 r4 = 0;
    if (i4 != -1) r4 = load<u32>(g + 24) + (static_cast<u32>(i4) << 7);
    const s32 i2 = static_cast<s16>(load<u16>(a + 2));
    u32 r2 = 0;
    if (i2 != -1) r2 = load<u32>(g + 24) + (static_cast<u32>(i2) << 7);
    u32 sel = 0;
    if (load<u8>(r2 + 2) != 0) sel = r2 + load<u16>(0x0106CF7Eu);
    const u32 k = load<u8>(sel);
    const u32 v = load<u8>(r4 + load<u16>(0x0106CF70u) + (k << 2));
    const f64 ref = Lfs44(0x003967A8u);
    ctx.fpr[1] = ref;
    if (F44(static_cast<f64>(v)) == ref) {
        ret(ctx, 1);
        return;
    }
    Stack frame(ctx, 64);
    const u32 s = frame.addr();
    for (u32 r = 0; r < 4; ++r)
        for (u32 j = 0; j < 3; ++j) Cp44(s + 16 * r + 4 * j, c + 32 + 16 * r + 4 * j);
    ctx.fpr[1] = Lfs44(c + 88);
    VCall(ctx, c, 84, {c, s});
    ret(ctx, 1);
}

// void f(T* o) (destructor): vtable 0x00B202F0; each of the parts +96 /
// +100 / +104 that is set gets vt[40](p, 0), and vt[40](p, 1) as well when
// bit 18 of its +172 is set; then Dtor_B219D0_00464AD0(o) (r3 = its).
DECOMP(003AAD78, Obj_Dtor_B202F0_003AAD78) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B202F0u);
    for (u32 off = 96; off <= 104; off += 4) {
        u32 p = load<u32>(o + off);
        if (p == 0) continue;
        VCall(ctx, p, 40, {p, 0});
        p = load<u32>(o + off);
        if (load<u32>(p + 172) & 0x40000u) VCall(ctx, p, 40, {p, 1});
    }
    call(ctx, 0x00464AD0u, {o});
}

// u32 f(T* o, x, u32 key): looks key up in the tree t = o->+16 (root
// *(t->+12), key +4, larger keys to the +12 side, smaller to +16), pushing
// the path onto t's u32 stack at +16 (u16 count +96, reset first). Found:
// t->+100 = the node, *0x011DE2AC = its +8 list l, and every entry e of l
// (u16 count +16, array +8) gets e->vt[12](e, x, o) (r3 = l's count).
// Not found: t->+100 = the last node visited (0 and the count 0 for an
// empty tree, which also clears *0x011DE2AC). r3 = 0 otherwise.
DECOMP(004BCC90, Tree_Dispatch_004BCC90) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const u32 t = load<u32>(o + 16);
    if (t == 0) {
        ret(ctx, 0);
        return;
    }
    u32 n = load<u32>(load<u32>(t + 12));
    if (n == 0) {
        store<u32>(t + 100, 0);
        store<u32>(0x011DE2ACu, 0);
        ret(ctx, 0);
        return;
    }
    store<u16>(t + 96, 0);
    for (;;) {
        const u32 cur = n;
        const u32 k = load<u32>(cur + 4);
        if (k == key) break;
        n = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
        if (n == 0) {
            store<u32>(t + 100, cur);
            ret(ctx, 0);
            return;
        }
        const s32 i = static_cast<s16>(load<u16>(t + 96));
        store<u16>(t + 96, static_cast<u16>(i + 1));
        store<u32>(t + 16 + 4 * static_cast<u32>(i), cur);
    }
    store<u32>(t + 100, n);
    store<u32>(0x011DE2ACu, load<u32>(n + 8));
    const u32 nd = load<u32>(t + 100);
    u32 r = 0;
    if (nd != 0) {
        const u32 l = load<u32>(nd + 8);
        if (l != 0) {
            for (u32 i = 0;; ++i) {
                r = load<u16>(l + 16);
                if (!(i < r)) break;
                const u32 e = load<u32>(load<u32>(l + 8) + 4 * i);
                VCall(ctx, e, 12, {e, x, o});
            }
        }
    }
    ret(ctx, r);
}

// u32 f(T* o, XmlCursor* cur): fills the record o (vtable 0x00B22420, u16
// +4 = 1): hash 0xE42AC27C into +12, hash 0x00014211 into +16 (default
// its old value), and the s32 0x04DA9951 into +16 (default the value read)
// unless that hash was absent with a non-zero value; then the kind hash
// 0x0932E547 picks the u8 +20: 0x78367079 = 4, 0x559E9624 = 3, 0xB3D90B14
// = 2, 0x0000FC81 = 1 (else 0). r3 = the kind, or 0xB3D90B14 when none
// matches.
DECOMP(004CAC7C, XmlRec_Read_B22420_004CAC7C) {
    const u32 o = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    store<u16>(o + 4, 1);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u8>(o + 20, 0);
    store<u32>(o, 0x00B22420u);
    call(ctx, 0x004FA14Cu, {cur, 0xE42AC27Cu, o + 12, 0});
    const u32 r = static_cast<u32>(call(ctx, 0x004FA14Cu, {cur, 0x00014211u, o + 16, load<u32>(o + 16)}));
    const u32 v = load<u32>(o + 16);
    if (r != 0 || v == 0) call(ctx, 0x004F99BCu, {cur, 0x04DA9951u, o + 16, sly4::obj39::Sx39(v)});
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u32>(s, 0);
    call(ctx, 0x004FA14Cu, {cur, 0x0932E547u, s, 0});
    const u32 k = load<u32>(s);
    u32 kind = 0;
    if (k == 0x78367079u) kind = 4;
    else if (k == 0x559E9624u) kind = 3;
    else if (k == 0x0000FC81u) kind = 1;
    else if (k == 0xB3D90B14u) kind = 2;
    if (kind == 0) {
        ret(ctx, 0xB3D90B14u);
        return;
    }
    store<u8>(o + 20, static_cast<u8>(kind));
    ret(ctx, kind);
}

// T* f(?, XmlCursor* cur): a new 48-byte record (Mem_Alloc(48, 0, 16);
// vtable 0x00B223D0, u16 +4 = 1, +16..+24 = the vector 0x01164B30, u8 +32
// = 1): +12 = String_HashNoCase of the attribute "%s" 0x00AEF030 (else
// the s32 0x5C8998CD); the vec3 0x04B1112A into +16 (default 0x01164B30),
// or when that fails (non-zero) the f32 of that name (default *0x004CB320)
// splatted into +16..+24 when it succeeds (0); the bool 0xF5897C08 into
// +32. r3 = the
// record (0 when the allocation fails).
DECOMP(004CB328, XmlRec_New_B223D0_004CB328) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {48, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    store<u32>(p + 12, 0);
    store<u16>(p + 4, 1);
    Cp44(p + 16, 0x01164B30u);
    store<u32>(p, 0x00B223D0u);
    const u32 vy = load<u32>(0x01164B34u);
    store<u8>(p + 32, 1);
    store<u32>(p + 20, vy);
    Cp44(p + 24, 0x01164B38u);
    ctx.fpr[1] = Lfs44(0x01164B38u);
    const u32 at = static_cast<u32>(call(ctx, 0x004F9628u, {cur, 0x00AEF030u}));
    if (at != 0) store<u32>(p + 12, static_cast<u32>(call(ctx, 0x0056A710u, {at})));
    else call(ctx, 0x004F99BCu, {cur, 0x5C8998CDu, p + 12, 0});
    if (static_cast<u32>(call(ctx, 0x004F9E0Cu, {cur, 0x04B1112Au, p + 16, 0x01164B30u})) != 0) {
        Stack frame(ctx, 16);
        const u32 s = frame.addr();
        callf(ctx, 0x004F9B20u, {cur, 0x04B1112Au, s}, {Lfs44(0x004CB320u)});
        if (static_cast<u32>(ctx.gpr[3]) == 0) {
            const u32 w = load<u32>(s);
            ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
            store<u32>(p + 16, w);
            store<u32>(p + 20, w);
            store<u32>(p + 24, w);
        }
    }
    call(ctx, 0x004F986Cu, {cur, 0xF5897C08u, p + 32, load<u8>(p + 32)});
    ret(ctx, p);
}

// ---------------------------------------------------------------- sweep: 95 instructions
// u32 f(T* o, x): AiCtrl_Bind_00011D08(o, x), +352 =
// Handle_ResolveBit1(0x2508CAD1), o->+48->vt[96](+48, 1) and bit 12 set /
// bit 10 cleared in the u16 +40 of its +164 part; +208 = 3.0, +4 / +28 =
// 0xB2E5993A, bit 7 of the u8 +305 set, +308 / +312 = 0, +336..+344 = the
// vector 0x01164B40, bit 29 of +356 cleared; bit 31 of +356 = (+48 has bit
// 9 of +192) and its +508 is set (written first as set when +48 has that
// bit), bit 30 cleared; then o->vt[224](o). r3 = that call's; f1 = the
// vector's z.
DECOMP(0002BB38, AiCtrl_Reset_0002BB38) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00011D08u, {o, arg<u32>(ctx, 1)});
    store<u32>(o + 352, static_cast<u32>(call(ctx, 0x003568E0u, {0x2508CAD1u})));
    {
        const u32 p = load<u32>(o + 48);
        VCall(ctx, p, 96, {p, 1});
    }
    u32 c = load<u32>(o + 48);
    const u32 q = load<u32>(c + 164);
    if (q != 0) {
        store<u16>(q + 40, static_cast<u16>((load<u16>(q + 40) | 0x1000u) & ~0x400u));
        c = load<u32>(o + 48);
    }
    const u8 b305 = load<u8>(o + 305);
    store<u32>(o + 208, 0x40400000u);
    u32 f = load<u32>(o + 356);
    store<u32>(o + 4, 0xB2E5993Au);
    store<u32>(o + 28, 0xB2E5993Au);
    f &= ~0x20000000u;
    store<u32>(o + 308, 0);
    store<u8>(o + 305, static_cast<u8>(b305 | 0x80u));
    store<u32>(o + 312, 0);
    store<u32>(o + 356, f);
    Cp44(o + 336, 0x01164B40u);
    Cp44(o + 340, 0x01164B44u);
    Cp44(o + 344, 0x01164B48u);
    ctx.fpr[1] = Lfs44(0x01164B48u);
    u32 t = 0;
    if (c != 0 && (load<u32>(c + 192) & 0x200u)) t = c;
    u32 v = f | 0x80000000u;
    if (t != 0) {
        store<u32>(o + 356, v);
        if (load<u32>(t + 508) == 0) v = f & 0x7FFFFFFFu;
    } else {
        v = f & 0x7FFFFFFFu;
    }
    v &= ~0x40000000u;
    store<u32>(o + 356, v);
    VCall(ctx, o, 224, {o});
}

// u32 f(T* o): with the object 0xF6CB1B5C found and bit 22 of its +192
// set: obj->vt[304](obj, 1); o->+404->vt[304](+404, 0) when set; the object
// 0x19F541C4 (when found) gets vt[88](obj, 1, 1, 1, 1); then
// Region_QueryEntities_00049060(o, &(22, 250, 50), &(60, 330, 90), 0) and
// the u8 +1808 = 1. r3 = 1.
DECOMP(0004941C, Obj_EnterRegion_0004941C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 id = 0xF6CB1B5Cu;
    if (call(ctx, 0x000D6A28u, {id}) != 0 && (load<u32>(static_cast<u32>(call(ctx, 0x000D6A28u, {id})) + 192) & 0x00400000u)) {
        const u32 p = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
        if (p != 0) VCall(ctx, p, 304, {p, 1});
    }
    const u32 q = load<u32>(o + 404);
    if (q != 0) VCall(ctx, q, 304, {q, 0});
    const u32 r = static_cast<u32>(call(ctx, 0x000D6A28u, {0x19F541C4u}));
    if (r != 0) VCall(ctx, r, 88, {r, 1, 1, 1, 1});
    Stack frame(ctx, 32);
    const u32 s = frame.addr();
    store<u32>(s, 0x41B00000u);
    store<u32>(s + 4, 0x437A0000u);
    store<u32>(s + 8, 0x42480000u);
    store<u32>(s + 16, 0x42700000u);
    store<u32>(s + 20, 0x43A50000u);
    store<u32>(s + 24, 0x42B40000u);
    call(ctx, 0x00049060u, {o, s, s + 16, 0});
    store<u8>(o + 1808, 1);
    ret(ctx, 1);
}

// u32 f(T* o, ?, Hit* h; f32 v): for state +4 == 5 (or 12 with +36 >
// *0x000575E4) and v > *0x000575E8, when the u16 h->+14 has bit 12 and
// field (>> 13) & 7 == 1: ++o->+6172; unless state +308 is 9:
// o->+60->vt[352](+60, OPD, h->+4, 0; v) and Global_F64CB8_Copy216; bit 7
// of the u8 +305 set, the u8 +6176 = a random bit (the inlined Rand16 of
// 0x01164888: the low bit of the old state word), overridden by the top
// two bits of h->+76->+72 (2: 1, 1: 0). r3 as the original leaves it (the
// failing test's value, else 0 / 1).
DECOMP(000575F0, Ai_OnHitCount_000575F0) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 2);
    const f64 v = farg(ctx, 0);
    const u32 st = load<u32>(o + 4);
    if (static_cast<s32>(st) != 5) {
        if (static_cast<s32>(st) != 12) {
            ret(ctx, st);
            return;
        }
        if (!(Lfs44(o + 36) > Lfs44(0x000575E4u))) {
            ret(ctx, 0x000575E4u);
            return;
        }
    }
    if (!(v > Lfs44(0x000575E8u))) {
        ret(ctx, 0x000575E4u);
        return;
    }
    const u32 fl = load<u16>(h + 14);
    if (((fl >> 12) & 1u) == 0) {
        ret(ctx, fl);
        return;
    }
    if (((fl >> 13) & 7u) != 1u) {
        ret(ctx, (fl >> 13) & 7u);
        return;
    }
    const u32 cnt = load<u32>(o + 6172) + 1;
    const u32 s308 = load<u32>(o + 308);
    store<u32>(o + 6172, cnt);
    if (static_cast<s32>(s308) != 9) {
        const u32 p = load<u32>(o + 60);
        VCallOpdR4_28(ctx, p, 352, load<u32>(h + 4), 0);
        call(ctx, 0x00369D58u, {});
    }
    store<u8>(o + 305, static_cast<u8>(load<u8>(o + 305) | 0x80u));
    {
        const u32 g = 0x01164888u;
        const u32 w = load<u32>(g + 8);
        const u32 x = (std::rotl(w, 4) + std::rotl(w, 16)) ^ 0x87654321u;
        store<u32>(g + 8, std::rotl(x, 16));
        store<u64>(g, static_cast<u64>(x & 0xFFFFu));
        store<u8>(o + 6176, static_cast<u8>(w & 1u));
    }
    const u32 a = load<u32>(h + 76);
    u32 r = 0;
    if (a != 0) {
        const u32 k = load<u32>(a + 72) >> 30;
        if (k == 2) {
            r = 1;
            store<u8>(o + 6176, 1);
        } else if (k == 1) {
            store<u8>(o + 6176, 0);
        }
    }
    ret(ctx, r);
}

// T* f(?, XmlCursor* cur): a new 96-byte record (Mem_Alloc(96, 0, 16),
// Obj_InitVt_B21C98(p, cur); vtable 0x00B0F478, +80..+92 and the name +12
// cleared): the name = the attribute "%s" 0x00AC4A1C (String_CopyN 64,
// when non-empty), hashes 0x7A30D7E3 / 0x03D1FCFC into +80 / +84, the s32
// 0x82B8846A into +88 (decremented when present: 1-based), the bool
// 0x7A281983 into +92 (defaults: the old values). r3 = the record (0 when
// the allocation fails).
DECOMP(00072464, XmlRec_New_B0F478_00072464) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {96, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, cur});
    store<u32>(p + 80, 0);
    store<u32>(p + 84, 0);
    store<u32>(p + 88, 0);
    store<u8>(p + 92, 0);
    store<u32>(p, 0x00B0F478u);
    store<u8>(p + 12, 0);
    const u32 at = static_cast<u32>(call(ctx, 0x004F9628u, {cur, 0x00AC4A1Cu}));
    if (at != 0 && load<u8>(at) != 0) call(ctx, 0x00553638u, {p + 12, at, 64});
    call(ctx, 0x004FA14Cu, {cur, 0x7A30D7E3u, p + 80, load<u32>(p + 80)});
    call(ctx, 0x004FA14Cu, {cur, 0x03D1FCFCu, p + 84, load<u32>(p + 84)});
    const u32 r = static_cast<u32>(call(ctx, 0x004F99BCu, {cur, 0x82B8846Au, p + 88, sly4::obj39::Sx39(load<u32>(p + 88))}));
    const u32 b = load<u8>(p + 92);
    if (r == 0) store<u32>(p + 88, load<u32>(p + 88) - 1);
    call(ctx, 0x004F986Cu, {cur, 0x7A281983u, p + 92, b});
    ret(ctx, p);
}

// T* f(?, XmlCursor* cur): a new 36-byte record (Mem_Alloc(36, 0, 16),
// Obj_InitVt_B21C98(p, cur); vtable 0x00B0F5E0, +12..+32 cleared): hashes
// 0x3A1B33FA / 0x3A15BFAB / 0x7E6EFA38 into +12 / +16 / +24, the s32
// 0x5CFBABDC into +28, the bool 0x6D18DE7D into +32 (defaults: the old
// values); with +12 set and the node +20 not live (Ptr_DerefNonZero):
// RefNode_Release(+20), +20 = Node_PushF65B7C(+12, +16). r3 = the record
// (0 when the allocation fails).
DECOMP(000741D8, XmlRec_New_B0F5E0_000741D8) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {36, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, cur});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    store<u32>(p, 0x00B0F5E0u);
    store<u32>(p + 28, 0);
    store<u8>(p + 32, 0);
    call(ctx, 0x004FA14Cu, {cur, 0x3A1B33FAu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {cur, 0x3A15BFABu, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {cur, 0x7E6EFA38u, p + 24, load<u32>(p + 24)});
    call(ctx, 0x004F99BCu, {cur, 0x5CFBABDCu, p + 28, sly4::obj39::Sx39(load<u32>(p + 28))});
    call(ctx, 0x004F986Cu, {cur, 0x6D18DE7Du, p + 32, load<u8>(p + 32)});
    if (load<u32>(p + 12) != 0 && static_cast<u32>(call(ctx, 0x00446960u, {load<u32>(p + 20)})) == 0) {
        call(ctx, 0x004432DCu, {load<u32>(p + 20)});
        store<u32>(p + 20, static_cast<u32>(call(ctx, 0x00446984u, {load<u32>(p + 12), load<u32>(p + 16)})));
    }
    ret(ctx, p);
}

// u32 f(T* o, Ctx* x): resolves the target t (the component path of
// Target_ApplyFlag_0008CAE8: Effect_FindComponent(x, +12) of kind 5 whose
// +28 object is of class 0x02235C33 gives its +280; else
// Obj_FindById(+16)), then Obj_FindByHash_000DB290(t, +20) when +20 is
// set. With t: unless the u8 +29 is clear and t->vt[132](t) == +24 already,
// t->vt[128](t, +24, u8 +28) (entry / OPD after the arguments). r3 as the
// original leaves it (0 without t).
DECOMP(000752A8, Target_SetState_000752A8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    bool viaComp = false;
    u32 c = load<u32>(o + 12);
    if (c != 0) {
        c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, c}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 q = load<u32>(c + 28);
            if (q != 0) {
                if (load<u32>(q + 276) == 0x02235C33u) t = load<u32>(q + 280);
                viaComp = t != 0;
            }
        }
    }
    if (!viaComp) {
        const u32 id = load<u32>(o + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
        if (t == 0) {
            ret(ctx, 0);
            return;
        }
    }
    const u32 h = load<u32>(o + 20);
    if (h != 0) {
        t = static_cast<u32>(call(ctx, 0x000DB290u, {t, h}));
        if (t == 0) {
            ret(ctx, 0);
            return;
        }
    }
    if (load<u8>(o + 29) == 0 && static_cast<s32>(VCall(ctx, t, 132, {t})) == static_cast<s32>(load<u32>(o + 24))) return;
    VCallEO36(ctx, load<u32>(t), 128, {t, load<u32>(o + 24), load<u8>(o + 28)});
}

// u32 f(T* o, Ctx* x): finds the target object +32 (the +28 object of the
// kind-5 component Effect_FindComponent(x, +12), unchecked; when it is of
// class 0x02235C33 its +280 object's f32 +212 goes to +28), else the
// +160 of Obj_FindById(+16) (+28 = its +212), else
// WorldObj_FindByHash(+20, 0); with one, CmdList_Push_0042C138 of a new
// 132-byte command (SmallAlloc; VtB20F08_Init_0042F854(p, o + 24, u8
// +140); 0 when the allocation fails). r3 = 0 without a target, else the
// push's; f1 = the last +212 copied.
DECOMP(0009E350, Target_PushCmd_0009E350) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 c = load<u32>(o + 12);
    if (c != 0) {
        c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, c}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 q = load<u32>(c + 28);
            store<u32>(o + 32, q);
            if (load<u32>(q + 276) == 0x02235C33u) {
                const u32 r = load<u32>(q + 280);
                if (r != 0) {
                    ctx.fpr[1] = Lfs44(r + 212);
                    Cp44(o + 28, r + 212);
                }
            }
        }
    }
    u32 t = load<u32>(o + 32);
    if (t == 0) {
        const u32 id = load<u32>(o + 16);
        if (id != 0) {
            const u32 f = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
            t = load<u32>(o + 32);
            if (f != 0) {
                t = load<u32>(f + 160);
                const u32 w = load<u32>(f + 212);
                ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));
                store<u32>(o + 32, t);
                store<u32>(o + 28, w);
            }
        }
    }
    if (t == 0) {
        const u32 h = load<u32>(o + 20);
        if (h != 0) {
            t = static_cast<u32>(call(ctx, 0x005801DCu, {h, 0}));
            store<u32>(o + 32, t);
        }
    }
    if (t == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {132}));
    u32 cmd = 0;
    if (p != 0) {
        call(ctx, 0x0042F854u, {p, o + 24, load<u8>(o + 140)});
        cmd = p;
    }
    call(ctx, 0x0042C138u, {cmd});
}

// u32 f(T* o, Hit* h): when Obj_TestE0BCC(o, h): s = h->+32 * k, k =
// *0x00127350 when h->+4 has bit 28 of +192, else by the hit kind h->+8:
// 0x2DA0A51D o->+520, 0x0020EE36 o->+516, 0x8CFF6308 o->+512 * the tuning
// var 0x01039540, other o->+512. With h->+4 an attacker (bit 3 of +192)
// of o's team (bits 22..23 of +172) where bit 27 of its +188 is clear and
// (bit 5 of o->+188 clear or k <= *0x00127354): s *= the tuning var
// 0x01038988. Then 0x00125A80(o, 0x811B1D5E, h, 0x811B1D5E; s) (r3 =
// its; 0 when the test fails).
DECOMP(00127358, Obj_HitScaledKind_00127358) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    if (call(ctx, 0x000E0BCCu, {o, h}) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 q = load<u32>(h + 4);
    f64 k = Lfs44(0x00127350u);
    if (q == 0 || (load<u32>(q + 192) & 0x10000000u) == 0) {
        const u32 kind = load<u32>(h + 8);
        if (kind == 0x2DA0A51Du) k = Lfs44(o + 520);
        else if (kind == 0x0020EE36u) k = Lfs44(o + 516);
        else if (kind == 0x8CFF6308u) k = F44(Lfs44(o + 512) * Lfs44(0x01039540u));
        else k = Lfs44(o + 512);
    }
    f64 s = F44(Lfs44(h + 32) * k);
    if (q != 0 && (load<u32>(q + 192) & 0x8u) && ((load<u32>(o + 172) >> 22) & 3u) == ((load<u32>(q + 172) >> 22) & 3u) &&
        ((static_cast<u32>(load<u64>(o + 184)) & 0x20u) == 0 || !(k > Lfs44(0x00127354u))) &&
        (static_cast<u32>(load<u64>(q + 184)) & 0x08000000u) == 0)
        s = F44(s * Lfs44(0x01038988u));
    callf(ctx, 0x00125A80u, {o, 0x811B1D5Eu, h, 0x811B1D5Eu}, {s});
}

// void f(T* o) (destructor): vtable 0x00B1A460, RefNode_Release of +432 /
// +436, bit 9 of the body +428's +136 set; vtable 0x00B19558,
// Volume_Notify112D8(*0x00F557EC, o + 424), Shape_Release638498(o + 424),
// the arrays +388 / +364 and the PtrVector +344 destroyed, then the base
// Thunk_000D729C(o) (r3 = its).
DECOMP(0021C5B0, VolShape_Dtor_B1A460_0021C5B0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r0 = load<u32>(o + 432);
    store<u32>(o, 0x00B1A460u);
    call(ctx, 0x004432DCu, {r0});
    call(ctx, 0x004432DCu, {load<u32>(o + 436)});
    const u32 b = load<u32>(o + 428);
    if (b != 0) store<u32>(b + 136, load<u32>(b + 136) | 0x200u);
    store<u32>(o, 0x00B19558u);
    call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), o + 424});
    call(ctx, 0x00638498u, {o + 424});
    ArrayDtor42(ctx, o + 388);
    ArrayDtor42(ctx, o + 364);
    VecDtor(ctx, o + 344);
    call(ctx, 0x000D729Cu, {o});
}

// u32 f(T* o; f32 dt): for each 8-byte entry (obj, u8 inside) of the list
// +432 (u16 count +440): inside = bit 0 of obj->+188; on a change the u8
// is updated and Area_Enter(o, obj) (the first occupant, +428 going from
// 0, also ends the scenes of every member of the PtrVector +344 whose
// +216->+32 has bit 3: Obj_SceneEnd0DD02C(m, 1, 0)) or Area_Leave(o, obj)
// and Obj_EndScenes_002257B4(o, obj). Then Obj_UpdateTeamBase(o; dt) (r3
// = its).
DECOMP(00225870, Area_UpdateOccupants_00225870) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    for (u32 i = 0; i < load<u16>(o + 440); ++i) {
        const u32 e = load<u32>(o + 432) + 8 * i;
        const u32 a = load<u32>(e);
        const u32 in = (static_cast<u32>(load<u64>(a + 184)) & 1u) ? 1u : 0u;
        if (load<u8>(e + 4) == in) continue;
        store<u8>(e + 4, static_cast<u8>(in));
        if (in == 0) {
            call(ctx, 0x0020EBC8u, {o, a});
            call(ctx, 0x002257B4u, {o, a});
            continue;
        }
        call(ctx, 0x0020E7A4u, {o, a});
        const u32 c = load<u32>(o + 428);
        store<u32>(o + 428, c + 1);
        if (c != 0) continue;
        for (u32 j = 0; j < load<u16>(o + 352); ++j) {
            const u32 m = load<u32>(load<u32>(o + 344) + 4 * j);
            if ((load<u8>(load<u32>(m + 216) + 32) >> 3) & 1u) call(ctx, 0x000DD02Cu, {m, 1, 0});
        }
    }
    callf(ctx, 0x0020DFD8u, {o}, {dt});
}

// u32 f(Vec* dst, const Vec* src, u8 onlyMarked): appends the 64-byte
// entries of src (u16 count +8, data +0) to dst (u16 count +8, capacity
// +4, high-water u16 +10; PtrVector_Reserve(dst, 2 * capacity) when full),
// all of them or only those with the u8 +54 set (the entry word +28 is
// not copied). r3 = src's count; f1 = the last float copied.
DECOMP(0022A610, EntryVec_Append_0022A610) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u32 only = arg<u32>(ctx, 2) & 0xFFu;
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(src + 8)); ++i) {
        const u32 e = load<u32>(src) + 64 * i;
        if (only != 0 && load<u8>(e + 54) == 0) continue;
        if (load<u16>(dst + 8) == load<u16>(dst + 4)) call(ctx, 0x00551718u, {dst, (static_cast<u32>(load<u16>(dst + 4)) << 1) & 0x1FFFEu});
        const u32 d = load<u32>(dst) + ((static_cast<u32>(load<u16>(dst + 8)) << 6) & 0x003FFFC0u);
        for (u32 k = 0; k <= 24; k += 4) Cp44(d + k, e + k);
        Cp44(d + 36, e + 36);
        u32 f = load<u32>(e + 40);
        u32 w = load<u32>(e + 32);
        store<u32>(d + 40, f);
        f = load<u32>(e + 44);
        store<u32>(d + 32, w);
        store<u32>(d + 44, f);
        f = load<u32>(e + 48);
        u8 b = load<u8>(e + 52);
        store<u32>(d + 48, f);
        store<u8>(d + 52, b);
        f = load<u32>(e + 56);
        b = load<u8>(e + 53);
        store<u32>(d + 56, f);
        store<u8>(d + 53, b);
        f = load<u32>(e + 60);
        b = load<u8>(e + 54);
        store<u32>(d + 60, f);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
        const u32 c = static_cast<u32>(load<u16>(dst + 8)) + 1;
        store<u8>(d + 54, b);
        const u32 hw = load<u16>(dst + 10);
        store<u16>(dst + 8, static_cast<u16>(c));
        store<u16>(dst + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFFu) ? hw : c));
    }
    ret(ctx, n);
}

// u32 f(T* o; f32 dt): when o->vt[32](o, 0) == 1, o->vt[32](o, 1) == 2,
// +624 == 0x879F2BDA and Effect_Check4CF620(+356, 0): +348 = *0x0024386C
// (a clamp of +344 into [k, k]) and o->vt[28](o, 1, 3; k). Then unless
// Obj_Tick_00228E38(o; dt) is 0 (r3 = 0): Obj_PickPartnerTarget_00243728,
// 0x0024356C (o; dt) and 0x00242DF8(o) (r3 = 1). f1 = dt.
DECOMP(00243870, Partner_Tick_00243870) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (static_cast<s32>(VCall(ctx, o, 32, {o, 0})) == 1 && static_cast<s32>(VCall(ctx, o, 32, {o, 1})) == 2 &&
        load<u32>(o + 624) == 0x879F2BDAu) {
        const u32 r = static_cast<u32>(call(ctx, 0x004CF620u, {load<u32>(o + 356), 0}));
        const u32 vt = load<u32>(o);
        if (r != 0) {
            const f64 k = Lfs44(0x0024386Cu);
            store<u32>(o + 348, W44(k));
            ctx.fpr[1] = k;
            VCallVt(ctx, vt, 28, {o, 1, 3});
        }
    }
    if (static_cast<u32>((callf(ctx, 0x00228E38u, {o}, {dt}), ctx.gpr[3])) == 0) {
        ret(ctx, 0);
        return;
    }
    callf(ctx, 0x00243728u, {o}, {dt});
    callf(ctx, 0x0024356Cu, {o}, {dt});
    call(ctx, 0x00242DF8u, {o});
    ret(ctx, 1);
}

// void f(T* o, Src* s) (constructor): base vtable 0x00B1BE30, +4..+12 =
// s->+4..+12, u8 +16 = 1, +20 = 0; (+4, o) appended to the 8-byte entry
// array of 0x01056F1C (u16 capacity +0, count +2, high-water +4, flags
// +6 |= 1, data +8; Array_Reserve(arr, 2 * capacity) when full); vtable
// 0x00B1BF00, +24 = s, the PtrVector +28 constructed (u16 +34 = 4,
// PtrVector_Init(v, 4, 0, 0)), +48 = 0, bits 30..31 of +52 cleared; then
// WorldObj_UpdateLod(+8->+160, 0, 0) when set. r3 = its, else the +52
// word.
DECOMP(0025E7E4, Obj_Ctor_B1BF00_0025E7E4) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    constexpr u32 A = 0x01056F1Cu;
    store<u32>(o, 0x00B1BE30u);
    u32 k = load<u32>(s + 4);
    store<u32>(o + 4, k);
    u32 w = load<u32>(s + 8);
    u32 n = load<u16>(A + 2);
    store<u32>(o + 8, w);
    w = load<u32>(s + 12);
    store<u8>(o + 16, 1);
    store<u32>(o + 12, w);
    store<u32>(o + 20, 0);
    const u32 cap = load<u16>(A);
    if (n == cap) {
        call(ctx, 0x00558F3Cu, {A, (cap << 1) & 0xFFFEu});
        k = load<u32>(o + 4);
        n = load<u16>(A + 2);
    }
    const u32 d = load<u32>(A + 8);
    store<u32>(d + ((n << 3) & 0x7FFF8u), k);
    store<u32>(d + (static_cast<u32>(load<u16>(A + 2)) << 3) + 4, o);
    const u32 c = static_cast<u32>(load<u16>(A + 2)) + 1;
    const u32 hw = load<u16>(A + 4);
    const u32 fl = load<u16>(A + 6);
    store<u16>(A + 2, static_cast<u16>(c));
    store<u16>(A + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFFu) ? hw : c));
    store<u16>(A + 6, static_cast<u16>(fl | 1u));
    store<u32>(o, 0x00B1BF00u);
    store<u32>(o + 24, s);
    call(ctx, 0x005516E0u, {o + 28});
    store<u16>(o + 34, 4);
    call(ctx, 0x00551598u, {o + 28, 4, 0, 0});
    const u32 f = load<u32>(o + 52) & 0x3FFFFFFFu;
    const u32 g = load<u32>(o + 8);
    store<u32>(o + 48, 0);
    store<u32>(o + 52, f);
    const u32 lod = load<u32>(g + 160);
    if (lod == 0) {
        ret(ctx, f);
        return;
    }
    call(ctx, 0x0057EE00u, {lod, 0, 0});
}

// u32 f(?, u32 h): Hash_ToString(h); when h is one of the 18 collectible
// hashes of 0x00AD4CC8, its bit goes into the found mask +308 of the
// current save slot ((*0x00F65CA0) + 80 + 4056 * its u8 +4208); when the
// mask changed and its population count equals the target of 0x0105C110 /
// 0x0105C070: Trophy_Award0B27B4(0x7FDEFA2B / 0x7FDEFA2A). r3 as the
// original leaves it.
DECOMP(00268158, Collect_MarkFound_00268158) {
    const u32 h = arg<u32>(ctx, 1);
    call(ctx, 0x0056A6E0u, {h});
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 slot = g + static_cast<u32>(load<u8>(g + 4208)) * 4056u + 80;
    const u32 before = load<u32>(slot + 308);
    u32 r = 0;
    for (u32 i = 0; i < 18; ++i) {
        r = load<u32>(0x00AD4CC8u + 4 * i);
        if (r == h) {
            r = load<u32>(slot + 308) | (1u << i);
            store<u32>(slot + 308, r);
            break;
        }
    }
    const u32 now = load<u32>(slot + 308);
    if (before == now) {
        ret(ctx, r);
        return;
    }
    const u32 cnt = static_cast<u32>(std::popcount(now));
    if (load<u32>(0x0105C110u) == cnt) {
        call(ctx, 0x000B27B4u, {0x7FDEFA2Bu});
        return;
    }
    const u32 t = load<u32>(0x0105C070u);
    if (t == cnt) {
        call(ctx, 0x000B27B4u, {0x7FDEFA2Au});
        return;
    }
    ret(ctx, t);
}

// void f(T* o) (destructor): vtables 0x00B1DAB0 / +380 0x00B1DC5C / +384
// 0x00B1DC84, VtB21B58_DtorTrivial_004A0E08(o + 1024), the PtrVectors +920
// / +900 / +876 / +848 destroyed, SmallFree(+492, 16 * +508); the 20-byte
// part +388 (when set: vtables 0x00B12898 / 0x00B12A44 / 0x00B12A6C, its
// PtrVector destroyed, SmallFree(+388, 20)); vtables 0x00B126B8 / +380
// 0x00B1200C / +384 0x00B12528 and VtB12550_Dtor_000DF934(o) (r3 = its).
DECOMP(0030CB10, Obj_Dtor_B1DAB0_0030CB10) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1DAB0u);
    store<u32>(o + 380, 0x00B1DC5Cu);
    store<u32>(o + 384, 0x00B1DC84u);
    call(ctx, 0x004A0E08u, {o + 1024});
    VecDtor(ctx, o + 920);
    VecDtor(ctx, o + 900);
    VecDtor(ctx, o + 876);
    VecDtor(ctx, o + 848);
    call(ctx, 0x00552420u, {load<u32>(o + 492), load<u32>(o + 508) << 4});
    const u32 p = load<u32>(o + 388);
    if (p != 0) {
        store<u32>(o + 384, 0x00B12A6Cu);
        store<u32>(o + 380, 0x00B12A44u);
        store<u32>(o, 0x00B12898u);
        VecDtor(ctx, p);
        call(ctx, 0x00552420u, {load<u32>(o + 388), 20});
    }
    store<u32>(o + 384, 0x00B12528u);
    store<u32>(o + 380, 0x00B1200Cu);
    store<u32>(o, 0x00B126B8u);
    call(ctx, 0x000DF934u, {o});
}

// u32 f(T* o, s32 mode, u8 keep): mode 4 clears the byte 0x0106B62C; mode
// 5 with it still clear awards Trophy_Award0B27B4(0xCBB04602). Unless
// keep is 0 and mode is the current u8 +18: mode 4 without a world child
// (+8 + 0x1281F4) calls o->vt[108](o), +2572 = that child and (keep 0)
// child->vt[8](child); other modes except 5 / 6 release the remembered
// +2572 child when it is still the world's (World_ReleaseChild1281F4).
// Then Section_SetModeBase_003260F0(o, mode, keep) (r3 = its).
DECOMP(00339998, Section_SetMode_00339998) {
    const u32 o = arg<u32>(ctx, 0), mode = arg<u32>(ctx, 1);
    const u32 keep = arg<u32>(ctx, 2) & 0xFFu;
    const s32 m = static_cast<s32>(mode);
    if (m == 4) store<u8>(0x0106B62Cu, 0);
    if (m == 5 && load<u8>(0x0106B62Cu) == 0) call(ctx, 0x000B27B4u, {0xCBB04602u});
    if (!(keep == 0 && m == static_cast<s32>(load<u8>(o + 18)))) {
        if (m == 4) {
            if (load<u32>(load<u32>(o + 8) + 0x001281F4u) == 0) {
                VCall(ctx, o, 108, {o});
                const u32 c = load<u32>(load<u32>(o + 8) + 0x001281F4u);
                store<u32>(o + 2572, c);
                if (keep == 0) VCall(ctx, c, 8, {c});
            }
        } else if (m != 5 && m != 6) {
            const u32 c = load<u32>(o + 2572);
            if (c != 0) {
                const u32 w = load<u32>(o + 8);
                const u32 e = load<u32>(w + 0x001281F4u);
                if (e == c) call(ctx, 0x0047C490u, {w, c, e});
            }
        }
    }
    call(ctx, 0x003260F0u, {o, mode, keep});
}

// u32 f(T* o): sets bits 1..31 of +160 to 3 (bit 0 kept), moves a pending
// +336 to +340, and when the line ids (u8 +12 / +13 of the 16-byte
// entries of +8->+148 at u16 +358 / +356) changed: the new line's text
// (by the tables 0x00ADC68C (same pair), 0x00ADC674 (no first id) or
// 0x00ADC65C) looked up (Text_Lookup34A398) and, with a speaker +28 and
// a text, Speaker_Say3514AC(+28, text, 0, -, -, -, 0, 0, stack 0; f1 =
// *0x00371960, f2 = f3 = *0x00371964) setting bit 26 of +360, else
// clearing it. r3 as the original leaves it.
DECOMP(00371968, Speaker_UpdateLine_00371968) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f160 = (load<u32>(o + 160) & 1u) | 6u;
    const u32 g = load<u32>(o + 8);
    const u32 pend = load<u32>(o + 336);
    const u32 spk = load<u32>(o + 28);
    const u32 i358 = load<u16>(o + 358);
    store<u32>(o + 160, f160);
    const u32 i356 = load<u16>(o + 356);
    const u32 tbl = load<u32>(g + 148);
    if (pend != 0) {
        store<u32>(o + 340, pend);
        store<u32>(o + 336, 0);
    }
    const u32 ea = tbl + ((i358 << 4) & 0xFFFF0u), eb = tbl + ((i356 << 4) & 0xFFFF0u);
    const u32 a = load<u8>(ea + 12), b = load<u8>(eb + 12);
    if (a == b && load<u8>(ea + 13) == load<u8>(eb + 13)) {
        ret(ctx, b);
        return;
    }
    const u32 c = load<u8>(eb + 13);
    u32 t;
    if (b == c) t = load<u32>(0x00ADC68Cu + ((b << 2) & 0x3FCu));
    else if (b == 0) t = load<u32>(0x00ADC674u + (c << 2));
    else t = load<u32>(0x00ADC65Cu + ((b << 2) & 0x3FCu));
    u32 text = 0;
    if (spk != 0) text = static_cast<u32>(call(ctx, 0x0034A398u, {t}));
    if (spk == 0 || text == 0) {
        const u32 v = load<u32>(o + 360) & ~0x04000000u;
        store<u32>(o + 360, v);
        ret(ctx, v);
        return;
    }
    const f64 k0 = Lfs44(0x00371960u), k1 = Lfs44(0x00371964u);
    // r6 = 0x00371960 and r7 = 0 as the original leaves them; r8 is not read
    callf(ctx, 0x003514ACu, {load<u32>(o + 28), text, 0, 0x00371960u, 0, ctx.gpr[8], 0, 0, 0}, {k0, k1, k1});
    const u32 v = load<u32>(o + 360) | 0x04000000u;
    store<u32>(o + 360, v);
    ret(ctx, v);
}

// u32 f(T* o, const char* name): releases the instance +288 (vt[8]) and
// instantiates name (default "%s" 0x0106D0D0 when null or empty): +288 =
// GeomInstance_FindByHash(String_HashNoCase(name))->vt[56](g, 7), else
// WorldObj_Instance(*HashMap_Find2((*0x00F8F12C)->+16, &hash), 0) when
// the map has it; with one: inst->vt[40](inst, 1, 1) and +292 = the f32
// +36 of its +208. r3 = +208 + 32 then (f1 = the copied f32), else 0.
DECOMP(0039A110, Obj_SetInstance_0039A110) {
    const u32 o = arg<u32>(ctx, 0);
    u32 name = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 288);
    if (p != 0) {
        VCall(ctx, p, 8, {p});
        store<u32>(o + 288, 0);
    }
    if (name == 0 || load<u8>(name) == 0) name = 0x0106D0D0u;
    if (name == 0 || load<u8>(name) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {name}));
    const u32 gi = static_cast<u32>(call(ctx, 0x005801A0u, {h}));
    store<u32>(o + 288, gi);
    u32 inst;
    if (gi != 0) {
        inst = static_cast<u32>(VCall(ctx, gi, 56, {gi, 7}));
        store<u32>(o + 288, inst);
    } else {
        Stack frame(ctx, 16);
        const u32 s = frame.addr();
        const u32 m = load<u32>(load<u32>(0x00F8F12Cu) + 16);
        store<u32>(s, h);
        const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x007399A4u, {m, s})));
        inst = load<u32>(o + 288);
        if (e != 0) {
            inst = static_cast<u32>(call(ctx, 0x005A05E8u, {e, 0}));
            store<u32>(o + 288, inst);
        }
    }
    if (inst == 0) {
        ret(ctx, 0);
        return;
    }
    VCall(ctx, inst, 40, {inst, 1, 1});
    const u32 q = load<u32>(load<u32>(o + 288) + 208) + 32;
    ctx.fpr[1] = Lfs44(q + 4);
    Cp44(o + 292, q + 4);
    ret(ctx, q);
}

// u32 f(T* o; f32 r2): for each 8-byte entry of the list +92 + 112 (data
// +0, u16 count +8) whose object a has a->vt[320](a) == 0, bit 18 of
// +172 set and no idle link (+244 null or its +4 set), and lies within
// squared distance r2 of o->+96's position (+144): a->vt[352](a, OPD, 0,
// 0; a->+344). r3 = the count; f1 as the original leaves it.
DECOMP(003ABECC, Obj_WakeNearby_003ABECC) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 r2 = farg(ctx, 0);
    const u32 pp = load<u32>(o + 96) + 144;
    const u32 v = load<u32>(o + 92) + 112;
    const f64 px = Lfs44(pp), py = Lfs44(pp + 4), pz = Lfs44(pp + 8);
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(v + 8)); ++i) {
        const u32 e = load<u32>(load<u32>(v) + 8 * i);
        if (static_cast<u32>(VCall(ctx, e, 320, {e})) != 0) continue;
        const u32 a = load<u32>(load<u32>(v) + 8 * i);
        if (((load<u32>(a + 172) >> 18) & 1u) == 0) continue;
        const u32 s = load<u32>(a + 244);
        if (s != 0 && load<u32>(s + 4) == 0) continue;
        const f64 dy = F44(Lfs44(a + 148) - py), dx = F44(Lfs44(a + 144) - px);
        const f64 dz = F44(Lfs44(a + 152) - pz);
        f64 d = F44(dy * dy);
        d = Fma44(dx, dx, d);
        d = Fma44(dz, dz, d);
        ctx.fpr[1] = d;
        if (!(d < r2 || d == r2)) continue;
        ctx.fpr[1] = Lfs44(a + 344);
        VCallOpdR4_28(ctx, a, 352, 0, 0);
    }
    ret(ctx, n);
}

// u32 f(T* o, List* l): unless o or l is null or l is full (u16 count
// +64 == 32): ++u16 o->+178, Obj_SetFlagHash_0048FA54(o, 0x96C4050C, 1)
// and a 64-byte entry appended to l's PtrVector +56 (capacity +60,
// high-water +66; PtrVector_Reserve(v, 2 * capacity) when full): the
// vector 0x01164C90, the vector 0x01164B40, o, 0, 1.0, 1.0, 0. r3 = o
// (32 when full; else the old high-water).
DECOMP(003B77D8, List_AddEntry_003B77D8) {
    const u32 o = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1);
    if (l == 0 || o == 0) {
        ret(ctx, o);
        return;
    }
    if (load<u16>(l + 64) == 32) {
        ret(ctx, 32);
        return;
    }
    store<u16>(o + 178, static_cast<u16>(load<u16>(o + 178) + 1));
    call(ctx, 0x0048FA54u, {o, 0x96C4050Cu, 1});
    u32 a[4], b[3];
    for (u32 k = 0; k < 4; ++k) a[k] = load<u32>(0x01164C90u + 4 * k);
    for (u32 k = 0; k < 3; ++k) b[k] = load<u32>(0x01164B40u + 4 * k);
    const u32 cap = load<u16>(l + 60);
    if (load<u16>(l + 64) == cap) call(ctx, 0x00551718u, {l + 56, (cap << 1) & 0x1FFFEu});
    const u32 d = load<u32>(l + 56) + ((static_cast<u32>(load<u16>(l + 64)) << 6) & 0x003FFFC0u);
    for (u32 k = 0; k < 4; ++k) store<u32>(d + 4 * k, a[k]);
    for (u32 k = 0; k < 3; ++k) store<u32>(d + 16 + 4 * k, b[k]);
    store<u32>(d + 32, o);
    store<u32>(d + 36, 0);
    store<u32>(d + 40, 0x3F800000u);
    store<u32>(d + 44, 0x3F800000u);
    store<u32>(d + 48, 0);
    const u32 c = static_cast<u32>(load<u16>(l + 64)) + 1;
    const u32 hw = load<u16>(l + 66);
    store<u16>(l + 64, static_cast<u16>(c));
    store<u16>(l + 66, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFFu) ? hw : c));
    ret(ctx, hw);
}

// u32 f(WorldObj* o, const f32* t): with the peer WorldObj_GetPeer2(o):
// Peer_GetMatrix(peer, -, &m; *t) and m's rows 0..3 (x, y, z) go to the
// local matrix +80.. (bit 11 of +164 cleared; type 8 objects ((+4 >> 10)
// & 63) then get vt[36](o)), or, when it returns 0, to the world matrix
// +16.. and WorldObj_MarkDirty(o). r3 = 1; f1 = m's last float copied.
DECOMP(003B8AD0, WorldObj_SyncPeer_003B8AD0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = Lfs44(arg<u32>(ctx, 1));
    const u32 p = static_cast<u32>(call(ctx, 0x0057FF50u, {o}));
    if (p == 0) {
        ret(ctx, 1);
        return;
    }
    Stack frame(ctx, 64);
    const u32 s = frame.addr();
    // r4 is whatever WorldObj_GetPeer2 left (not read)
    const u32 r = static_cast<u32>(callf(ctx, 0x0057C408u, {p, ctx.gpr[4], s}, {t}), ctx.gpr[3]);
    ctx.fpr[1] = Lfs44(s + 56);
    if (r != 0) {
        for (u32 k = 0; k < 4; ++k) {
            for (u32 j = 0; j < 3; ++j) {
                if (k == 3 && j == 1) {
                    const u32 f164 = load<u32>(o + 164) & ~0x800u;
                    Cp44(o + 132, s + 52);
                    const u32 w4 = load<u32>(o + 4);
                    Cp44(o + 136, s + 56);
                    store<u32>(o + 164, f164);
                    if (((w4 >> 10) & 0x3Fu) == 8u && o != 0) VCall(ctx, o, 36, {o});
                    ret(ctx, 1);
                    return;
                }
                Cp44(o + 80 + 16 * k + 4 * j, s + 16 * k + 4 * j);
            }
        }
    }
    for (u32 k = 0; k < 4; ++k)
        for (u32 j = 0; j < 3; ++j) Cp44(o + 16 + 16 * k + 4 * j, s + 16 * k + 4 * j);
    call(ctx, 0x0057D000u, {o});
    ret(ctx, 1);
}

// u32 f(T* o): with the pressed buttons m (0x010B1A98 & 0x010B1A9C): when
// the hold time +132 > *0x003DB690: the choice c = 2 / 1 / 4 / 3 for
// bits 5 / 7 / 4 / 6 (the last set wins); bit 3 sends a zeroed 24-byte
// message (Msg_Init13_00465384(&msg, o)) through
// World_Query482D14(o->vt[76](o), 0, &msg); with a choice: +132 = 0, and
// when it is the expected one (+88): +84 = 2, u8 +174 = 1. Otherwise any
// of bits 4..7 sets +84 = 1. r3 as the original leaves it; f1 =
// *0x003DB690.
DECOMP(003DB698, Menu_PollChoice_003DB698) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = Lfs44(0x003DB690u);
    ctx.fpr[1] = k;
    const u32 B = 0x010B1A90u;
    if (!(Lfs44(o + 132) > k)) {
        const u32 m = (load<u32>(B + 8) & load<u32>(B + 12)) & 0xF0u;
        if (m != 0) {
            store<u32>(o + 84, 1);
            ret(ctx, 1);
            return;
        }
        ret(ctx, 0);
        return;
    }
    const u32 m = load<u32>(B + 8) & load<u32>(B + 12);
    u32 c = 0;
    if (m & 0x20u) c = 2;
    if (m & 0x80u) c = 1;
    if (m & 0x10u) c = 4;
    if (m & 0x40u) c = 3;
    u32 r = m & 0x8u;
    if (r != 0) {
        Stack frame(ctx, 32);
        const u32 s = frame.addr();
        for (u32 j = 0; j < 6; ++j) store<u32>(s + 4 * j, 0);
        call(ctx, 0x00465384u, {s, o});
        const u32 w = static_cast<u32>(VCall(ctx, o, 76, {o}));
        r = static_cast<u32>(call(ctx, 0x00482D14u, {w, 0, s}));
    }
    if (c != 0) {
        r = load<u32>(o + 88);
        store<u32>(o + 132, 0);
        if (static_cast<s32>(c) == static_cast<s32>(r)) {
            store<u32>(o + 84, 2);
            store<u8>(o + 174, 1);
            r = 2;
        }
    }
    ret(ctx, r);
}

// void f(T* o, a, const char* name, b, c, d, const f32* p, const f32* q,
// stack u8 s9..s14): Obj_InitVt_B20CC8(o, 1), vtable 0x00B21388, +112 /
// +128 = the vectors p / q, +12 = 0, +16 = c, +20 = a, +92 = b, +96 = d,
// u8 +144..+149 = s9..s14 (low bytes of the stack words), the name +24 =
// name (String_CopyN 64; empty when null or empty). r3 = String_CopyN's
// (0 for no name); f1 = q's z.
DECOMP(00439E4C, Obj_InitNamed_B21388_00439E4C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const u32 c = arg<u32>(ctx, 4), d = arg<u32>(ctx, 5), p = arg<u32>(ctx, 6), q = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    u32 sb[6];
    for (u32 k = 0; k < 6; ++k) sb[k] = load<u32>(sp + 116 + 8 * k);  // stack arguments 9..14
    call(ctx, 0x0042C9D0u, {o, 1});
    Cp44(o + 112, p);
    Cp44(o + 116, p + 4);
    const u32 pz = load<u32>(p + 8);
    store<u32>(o, 0x00B21388u);
    store<u32>(o + 120, pz);
    const u32 q0 = load<u32>(q);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, c);
    store<u32>(o + 20, a);
    store<u32>(o + 92, b);
    store<u32>(o + 128, q0);
    store<u32>(o + 96, d);
    const u32 q1 = load<u32>(q + 4);
    store<u8>(o + 144, static_cast<u8>(sb[0]));
    store<u32>(o + 132, q1);
    const u32 q2 = load<u32>(q + 8);
    store<u8>(o + 145, static_cast<u8>(sb[1]));
    store<u32>(o + 136, q2);
    for (u32 k = 2; k < 6; ++k) store<u8>(o + 144 + k, static_cast<u8>(sb[k]));
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(q2));
    if (name != 0 && load<u8>(name) != 0) {
        call(ctx, 0x00553638u, {o + 24, name, 64});
        return;
    }
    store<u8>(o + 24, 0);
    ret(ctx, 0);
}

// T* f(State* s, const f32* v0, ?, const f32* k; f32 dt): one classic
// Runge-Kutta step of the damped 2D oscillator a = -k.x * x - k.y * v
// (position +0 / +4, velocity +8 / +12; the stage weights
// of 0x00748D78, the start offset 0x01164AFC): s->+8 / +12 = v + dt *
// mean acceleration, s->+0 / +4 = v0 + dt * mean velocity. r3 = s; f1 =
// the new +4.
DECOMP(00748D88, Spring_Rk4Step_00748D88) {
    const u32 s = arg<u32>(ctx, 0), r4 = arg<u32>(ctx, 1), r6 = arg<u32>(ctx, 3);
    const f64 f1 = farg(ctx, 0);
    const u32 K = 0x00748D78u, G = 0x01164AFCu;
    const f64 f2 = Lfs44(s + 8), f3 = Lfs44(s + 12), k0 = Lfs44(K);
    f64 f5 = Fma44(Lfs44(G), k0, f2);
    const f64 f7a = Lfs44(r4);
    f64 f6 = Fma44(Lfs44(G + 4), k0, f3);
    const f64 f8a = Lfs44(r4 + 4);
    const f64 f9 = -Lfs44(r6), f10 = Lfs44(r6 + 4);
    f64 f12 = Fma44(f2, k0, f7a);
    f64 f4 = Fma44(f3, k0, f8a);
    const f64 f11 = F44(f1 * Lfs44(K + 4));
    f64 f13 = F44(f10 * f5);
    f64 f0 = F44(f10 * f6);
    f12 = Fms44(f9, f12, f13);
    f4 = Fms44(f9, f4, f0);
    f13 = Fma44(f5, f11, f7a);
    f0 = Fma44(f6, f11, f8a);
    f64 f31 = Fma44(f12, f11, f2);
    f64 f30 = Fma44(f4, f11, f3);
    f64 f29 = F44(f10 * f31);
    f64 f28 = F44(f10 * f30);
    f64 f27 = Fma44(f31, f11, f7a);
    f64 f26 = Fma44(f30, f11, f8a);
    f13 = Fms44(f9, f13, f29);
    f0 = Fms44(f9, f0, f28);
    f29 = Lfs44(K + 8);
    f28 = Fma44(f13, f11, f2);
    const f64 f11b = Fma44(f0, f11, f3);
    f64 f25 = F44(f10 * f28);
    const f64 f24 = F44(f10 * f11b);
    f64 f7 = Fma44(f28, f1, f7a);
    f31 = F44(f31 + f28);
    f64 f8 = Fma44(f11b, f1, f8a);
    const f64 f11c = F44(f30 + f11b);
    f30 = Lfs44(K + 12);
    f27 = Fms44(f9, f27, f25);
    f26 = Fms44(f9, f26, f24);
    f5 = Fma44(f31, f29, f5);
    f6 = Fma44(f11c, f29, f6);
    f28 = Fma44(f27, f1, f2);
    f13 = F44(f13 + f27);
    f25 = Fma44(f26, f1, f3);
    f0 = F44(f0 + f26);
    f31 = F44(f10 * f28);
    f12 = Fma44(f13, f29, f12);
    const f64 f10b = F44(f10 * f25);
    f4 = Fma44(f0, f29, f4);
    f5 = F44(f5 + f28);
    f6 = F44(f6 + f25);
    f7 = Fms44(f9, f7, f31);
    f8 = Fms44(f9, f8, f10b);
    f5 = F44(f5 / f30);
    f7 = F44(f12 + f7);
    f4 = F44(f4 + f8);
    f6 = F44(f6 / f30);
    f7 = F44(f7 / f30);
    f4 = F44(f4 / f30);
    store<u32>(s + 8, W44(Fma44(f1, f7, f2)));
    store<u32>(s + 12, W44(Fma44(f1, f4, f3)));
    const f64 x = Fma44(f1, f5, Lfs44(r4));
    const f64 y = Fma44(f1, f6, Lfs44(r4 + 4));
    store<u32>(s, W44(x));
    store<u32>(s + 4, W44(y));
    ctx.fpr[1] = y;
    ret(ctx, s);
}

// ---------------------------------------------------------------- sweep: 96 instructions
// u32 f(T* o; f32 dt): counts the timer +1792 down by dt; once it drops
// below *0x0004B49C it is re-armed to lo + (hi - lo) * rand * *0x0004B4A0
// (lo / hi = the tuning vars 0x00F98044 / 0x00F98090; two steps of the
// inlined Rand16 of 0x01164888, the second picking one of 10 effects) and
// the effect EffectDef_Find(0x1149F2C6 + n, 0, 1) is spawned: a 140-byte
// Effect (SmallAlloc, Effect_Construct; bit 9 of +136 set, even on a
// failed allocation: a null write) and Effect_Start. r3 = o when not yet
// due, else as the original leaves it; f1 = the timer.
DECOMP(0004B4A8, Ambient_TickEffect_0004B4A8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = F44(Lfs44(o + 1792) - farg(ctx, 0));
    store<u32>(o + 1792, W44(t));
    ctx.fpr[1] = t;
    if (!(t < Lfs44(0x0004B49Cu))) {
        ret(ctx, o);
        return;
    }
    const u32 g = 0x01164888u;
    const u64 prev = load<u64>(g);
    const u32 w = load<u32>(g + 8);
    const f64 k = Lfs44(0x0004B4A0u), lo = Lfs44(0x00F98044u);
    const f64 span = F44(Lfs44(0x00F98090u) - lo);
    const u32 x1 = (std::rotl(w, 4) + std::rotl(w, 16)) ^ 0x87654321u;
    const u32 r8 = std::rotl(x1, 16);
    const u32 x2 = (std::rotl(x1, 20) + x1) ^ 0x87654321u;
    const f64 rf = F44(F44(static_cast<f64>(static_cast<s64>(prev))) * k);
    store<u32>(g + 8, std::rotl(x2, 16));
    store<u64>(g, static_cast<u64>(x2 & 0xFFFFu));
    const f64 nt = Fma44(span, rf, lo);
    store<u32>(o + 1792, W44(nt));
    ctx.fpr[1] = nt;
    const u32 def = static_cast<u32>(call(ctx, 0x004BDEDCu, {0x1149F2C6u + r8 % 10u, 0, 1}));
    if (def == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {140}));
    u32 e = 0;
    if (p != 0) {
        call(ctx, 0x004C7A68u, {p, def});
        e = p;
    }
    store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
    call(ctx, 0x004C753Cu, {e});
}

// u32 f(T* o, Ctx* x): resolves the target t (the component path of
// Target_ApplyFlag_0008CAE8, else Obj_FindById(+16), unchecked), then
// Obj_FindByHash_000DB290(t, +20) when +20 is set; with t:
// Obj_CallByKey420_001AE9C8(t, +24) when bit 19 of t->+196 is set, else
// t->vt[76](t). r3 as the original leaves it (0 without t).
DECOMP(0007558C, Target_Trigger_0007558C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    bool viaComp = false;
    u32 c = load<u32>(o + 12);
    if (c != 0) {
        c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, c}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 q = load<u32>(c + 28);
            if (q != 0) {
                if (load<u32>(q + 276) == 0x02235C33u) t = load<u32>(q + 280);
                viaComp = t != 0;
            }
        }
    }
    if (!viaComp) {
        t = static_cast<u32>(call(ctx, 0x000D6A28u, {load<u32>(o + 16)}));
        if (t == 0) return;
    }
    const u32 h = load<u32>(o + 20);
    if (h != 0) {
        t = static_cast<u32>(call(ctx, 0x000DB290u, {t, h}));
        if (t == 0) return;
    }
    if (load<u32>(t + 196) & 0x00080000u) call(ctx, 0x001AE9C8u, {t, load<u32>(o + 24)});
    else VCall(ctx, t, 76, {t});
}

// u32 f(T* o, Ctx* x): the value v = f32 +20, or the vt[20] f32 of the
// kind-1 component Effect_FindComponent(x, +16); when the kind-5
// component of +12 has a +28 object of type 8 ((+4 >> 10) & 63):
// obj->vt[52](obj, (u8)(s32)(v * *0x00086C50), u8 +24). r3 as the
// original leaves it; f1 = the integer (fctiwz) on that path.
DECOMP(00086C58, Comp_SetLevel_00086C58) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 obj = 0;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, load<u32>(o + 12)}));
    if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) obj = load<u32>(c + 28);
    f64 v = Lfs44(o + 20);
    const u32 h = load<u32>(o + 16);
    if (h != 0) {
        const u32 d = static_cast<u32>(call(ctx, 0x004BB4B4u, {x, h}));
        if (d != 0 && static_cast<s32>(VCall(ctx, d, 32, {d})) == 1) {
            VCall(ctx, d, 20, {d});
            v = ctx.fpr[1];
        }
    } else {
        ret(ctx, 0);
    }
    if (obj == 0) return;
    const u32 ty = (load<u32>(obj + 4) >> 10) & 0x3Fu;
    if (ty != 8) {
        ret(ctx, ty);
        return;
    }
    const u64 i = rt::fctiw(F44(v * Lfs44(0x00086C50u)), true);
    ctx.fpr[1] = std::bit_cast<f64>(0xFFF8000000000000ull | i);  // fctiwz leaves the integer in f1 (high word 0xFFF80000)
    VCall(ctx, obj, 52, {obj, i & 0xFFu, load<u8>(o + 24)});
}

// void f(T* p, XmlCursor* cur) (constructor body): Obj_InitVt_B21C98(p,
// cur), vtable 0x00B104E0, +12 / +16 / +20 = 0, +24 = -1.0, bits 30..31 of
// +28 cleared; then hashes 0x7A30D7E3 / 0x8A93973F into +12 / +16, f32s
// 0x03DEC398 / 0x95A6FD77 into +20 / +24 and bools 0x1A07CE8B /
// 0x35D8B1D3 into bits 31 / 30 of +28 (through a stack byte; defaults
// the current values). r3 = the final +28; f1 = the old +24.
DECOMP(0008A880, XmlRec_Init_B104E0_0008A880) {
    const u32 p = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    call(ctx, 0x004BB3B0u, {p, cur});
    const u32 f28 = load<u32>(p + 28) & 0x3FFFFFFFu;
    store<u32>(p, 0x00B104E0u);
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0xBF800000u);
    store<u32>(p + 28, f28);
    call(ctx, 0x004FA14Cu, {cur, 0x7A30D7E3u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {cur, 0x8A93973Fu, p + 16, load<u32>(p + 16)});
    callf(ctx, 0x004F9B20u, {cur, 0x03DEC398u, p + 20}, {Lfs44(p + 20)});
    callf(ctx, 0x004F9B20u, {cur, 0x95A6FD77u, p + 24}, {Lfs44(p + 24)});
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u8>(s, 0);
    call(ctx, 0x004F986Cu, {cur, 0x1A07CE8Bu, s, load<u32>(p + 28) >> 31});
    u32 w = (load<u32>(p + 28) & 0x7FFFFFFFu) | (static_cast<u32>(load<u8>(s)) << 31);
    store<u32>(p + 28, w);
    call(ctx, 0x004F986Cu, {cur, 0x35D8B1D3u, s, (w >> 30) & 1u});
    w = (load<u32>(p + 28) & ~0x40000000u) | ((static_cast<u32>(load<u8>(s)) & 1u) << 30);
    store<u32>(p + 28, w);
    ret(ctx, w);
}

// T* f(?, XmlCursor* cur): a new 32-byte record (Mem_Alloc(32, 0, 16),
// Obj_InitVt_B21C98(p, cur); vtable 0x00B11230, +12 = 1.0, +16 = 0.25,
// +20 = -1.0, +24 = 0, u8 +28 = 0, u8 +29 = 1): f32s 0x653B2EE9 (default
// *0x0009D57C) / 0x4B39ED22 / 0xB7EA3D94 / 0xBE37CBE1 into +12 / +16 /
// +20 / +24, bools 0x8488565B / 0x8FE1FDC1 into +29 / +28 (defaults the
// current values). r3 = the record (0 when the allocation fails); f1 =
// the old +24.
DECOMP(0009D580, XmlRec_New_B11230_0009D580) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, cur});
    store<u32>(p, 0x00B11230u);
    const f64 k = Lfs44(0x0009D57Cu);
    store<u32>(p + 12, 0x3F800000u);
    store<u32>(p + 16, 0x3E800000u);
    store<u32>(p + 24, 0);
    store<u32>(p + 20, 0xBF800000u);
    store<u8>(p + 28, 0);
    store<u8>(p + 29, 1);
    callf(ctx, 0x004F9B20u, {cur, 0x653B2EE9u, p + 12}, {k});
    callf(ctx, 0x004F9B20u, {cur, 0x4B39ED22u, p + 16}, {Lfs44(p + 16)});
    callf(ctx, 0x004F9B20u, {cur, 0xB7EA3D94u, p + 20}, {Lfs44(p + 20)});
    callf(ctx, 0x004F9B20u, {cur, 0xBE37CBE1u, p + 24}, {Lfs44(p + 24)});
    call(ctx, 0x004F986Cu, {cur, 0x8488565Bu, p + 29, load<u8>(p + 29)});
    call(ctx, 0x004F986Cu, {cur, 0x8FE1FDC1u, p + 28, load<u8>(p + 28)});
    ret(ctx, p);
}

// u32 f(r, XmlCursor* cur, Def** out): XmlReg_ReadObjD4E28(r, cur, out),
// then for d = *out: vec3s 0x7834C22A / 0xEE79E14F into +128 / +144
// (defaults themselves), each scaled by *0x000D5320 afterwards; f32
// 0x156EFE9A (default *0x000D5324) into +160 when >= *0x000D5328; f32
// 0x156EFDAC (default +160) into +164 when >= +160. r3 = the last read's;
// f1 = the value read.
DECOMP(000D5330, XmlDef_ReadRange_000D5330) {
    const u32 cur = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 K = 0x000D5320u;
    call(ctx, 0x000D4E28u, {arg<u32>(ctx, 0), cur, out});
    u32 d = load<u32>(out);
    call(ctx, 0x004F9E0Cu, {cur, 0x7834C22Au, d + 128, d + 128});
    d = load<u32>(out);
    const f64 sc = Lfs44(K);
    {
        const u32 v = d + 128;
        const f64 x = F44(Lfs44(v) * sc), y = F44(Lfs44(v + 4) * sc), z = F44(Lfs44(v + 8) * sc);
        store<u32>(v, W44(x));
        store<u32>(v + 4, W44(y));
        store<u32>(v + 8, W44(z));
    }
    call(ctx, 0x004F9E0Cu, {cur, 0xEE79E14Fu, d + 144, d + 144});
    d = load<u32>(out);
    {
        const u32 v = d + 144;
        const f64 x = F44(Lfs44(v) * sc), y = F44(Lfs44(v + 4) * sc), z = F44(Lfs44(v + 8) * sc);
        store<u32>(v, W44(x));
        store<u32>(v + 4, W44(y));
        store<u32>(v + 8, W44(z));
    }
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    callf(ctx, 0x004F9B20u, {cur, 0x156EFE9Au, s}, {Lfs44(K + 4)});
    f64 r = Lfs44(s);
    if (r > Lfs44(K + 8) || r == Lfs44(K + 8)) store<u32>(load<u32>(out) + 160, W44(r));
    callf(ctx, 0x004F9B20u, {cur, 0x156EFDACu, s}, {Lfs44(load<u32>(out) + 160)});
    d = load<u32>(out);
    r = Lfs44(s);
    const f64 lo = Lfs44(d + 160);
    if (r > lo || r == lo) store<u32>(d + 164, W44(r));
    ctx.fpr[1] = r;
}

// u32 f(T* o): for an o with bit 5 of +188, each of the n (+216 + 772 +
// 196) 44-byte slots (from +20: handle, f32 +4, u8 +9) of its anim set:
// the object Handle_ResolveBit1(handle) in state 2 / 3 ((+160 >> 1))
// gets Field340_Pick_0036C0E0, any other one Obj_SetAnim(obj, &u8 or 0,
// 1, u8, 0, 0, 0x01164CA4; o->+1168 when >= *0x001299D4, else the slot's
// f32). r3 as the original leaves it.
DECOMP(001299D8, Obj_PlaySlotAnims_001299D8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 m = static_cast<u32>(load<u64>(o + 184)) & 0x20u;
    if (m == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 b = load<u32>(o + 216) + 772;
    u32 n = load<u32>(b + 196);
    if (n == 0) {
        ret(ctx, 0);
        return;
    }
    const f64 lim = Lfs44(0x001299D4u);
    for (u32 i = 0;;) {
        const u32 e = b + 44 * i;
        const u32 x = static_cast<u32>(call(ctx, 0x003568E0u, {load<u32>(e + 20)}));
        if (x != 0) {
            const u32 st = load<u32>(x + 160) >> 1;
            if (st == 2 || st == 3) {
                call(ctx, 0x0036C0E0u, {x});
            } else {
                f64 t = Lfs44(o + 1168);
                if (!(t > lim || t == lim)) t = Lfs44(e + 24);
                const u32 by = load<u8>(e + 29);
                callf(ctx, 0x00363A3Cu, {x, by != 0 ? e + 29 : 0u, 1, by, 0, 0, 0x01164CA4u}, {t});
            }
        }
        ++i;
        n = load<u32>(b + 196);
        if (!(i < n)) break;
    }
    ret(ctx, n);
}

// T* f(T* o; f32 dt) (leaf): by state +52: 1 adds dt to +80, then 1 / 2 /
// 4: +48 = +44 = 0, ++count +60, the delay +56 counted down (when set) and
// state 5 once it is 0 and the count reached the u16 +8; 3: +40 -= dt,
// the u8 +68 toggled, state 5 once +40 <= *0x0017FF94. Then the phase +72
// += dt sets the level +76 by the thresholds of 0x0017FF7C (1 2 3 2 1)
// and wraps past the sixth (+76 = 0). r3 = o; f1 = the phase.
DECOMP(0017FF98, Pulse_Tick_0017FF98) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 K = 0x0017FF7Cu;
    const s32 st = static_cast<s32>(load<u32>(o + 52));
    const u32 lim = load<u16>(o + 8);
    if (st == 3) {
        const f64 t = F44(Lfs44(o + 40) - dt);
        const u32 b = load<u8>(o + 68) == 0 ? 1u : 0u;
        store<u32>(o + 40, W44(t));
        const f64 k = Lfs44(K + 24);
        store<u8>(o + 68, static_cast<u8>(b));
        if (t < k || t == k) store<u32>(o + 52, 5);
    } else if (st == 1 || st == 2 || st == 4) {
        if (st == 1) store<u32>(o + 80, W44(F44(Lfs44(o + 80) + dt)));
        u32 d = load<u32>(o + 56);
        store<u32>(o + 48, 0);
        if (d != 0) {
            --d;
            store<u32>(o + 56, d);
        }
        const u32 c = load<u32>(o + 60) + 1;
        store<u32>(o + 44, 0);
        store<u32>(o + 60, c);
        if (d == 0 && !(c < lim)) store<u32>(o + 52, 5);
    }
    f64 ph = F44(Lfs44(o + 72) + dt);
    store<u32>(o + 76, 0);
    store<u32>(o + 72, W44(ph));
    static const u32 lv[5] = {1, 2, 3, 2, 1};
    for (u32 k = 0; k < 5; ++k)
        if (ph > Lfs44(K + 4 * k)) store<u32>(o + 76, lv[k]);
    const f64 wrap = Lfs44(K + 20);
    if (ph > wrap) {
        ph = F44(ph - wrap);
        store<u32>(o + 76, 0);
        store<u32>(o + 72, W44(ph));
    }
    ctx.fpr[1] = ph;
    ret(ctx, o);
}

// u32 f(T* o, s32 ev, u8 now): effect +356 events: 2 releases it with
// 0xDBA69E75 (0x8FA5922D unless both u8 +853 / +854 are set), 6 with
// 0x3C8207B4 (0x8C7121FC when +853 is set), and with now and a
// successful release Effect_SetTime(+356; *0x001CFC28); other events pick
// the def / instance +432 + 16 * ev (Effect_SetDef_004CF8D8 when now,
// else Effect_SetInstance_004CF330). r3 as the original leaves it.
DECOMP(001CFC30, Obj_EffectEvent_001CFC30) {
    const u32 o = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    const u32 now = arg<u32>(ctx, 2) & 0xFFu;
    const s32 e = static_cast<s32>(ev);
    if (e == 2 || e == 6) {
        const u32 fx = load<u32>(o + 356);
        if (fx == 0) {
            ret(ctx, 0);
            return;
        }
        u32 id;
        if (e == 2) id = (load<u8>(o + 853) == 0 || load<u8>(o + 854) == 0) ? 0x8FA5922Du : 0xDBA69E75u;
        else id = load<u8>(o + 853) != 0 ? 0x8C7121FCu : 0x3C8207B4u;
        const u32 r = static_cast<u32>(call(ctx, 0x004CF3C4u, {fx, id}));
        if (r == 0 || now == 0) return;
        callf(ctx, 0x004CF710u, {load<u32>(o + 356)}, {Lfs44(0x001CFC28u)});
        return;
    }
    const u32 p = o + 432 + (ev << 4);
    const u32 v = load<u32>(p);
    if (now == 0) call(ctx, 0x004CF330u, {load<u32>(o + 356), v});
    else call(ctx, 0x004CF8D8u, {load<u32>(o + 356), v});
}

// T* f(a, XmlCursor* b, name, tmpl): a 256-byte spawn definition
// (SmallAlloc) initialised by SpawnQuery_Init(p, name, tmpl, 0,
// 0x01164BD0, 0x01164B30, 1, 0, stack 0 / - / 0; f1 = *0x001DE9D0),
// +236 = 0, flag 4 in +4, vtable 0x00B17DE0, bit 30 of +8 set and the
// vec2 +240 from tmpl + 632 (0x01164AFC without tmpl); then
// XmlDef_ReadBase_000CBE10(a, b, p) and the vec2 0xBF3F0DB1 into +240
// (default itself), scaled by *0x001DE9CC when it was read. r3 = p (0 for
// a failed allocation, still used: the vec2 goes to address 240).
DECOMP(001DE9D8, XmlDef_New_B17DE0_001DE9D8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {256}));
    if (p != 0) {
        // stack argument 10 is not written by the original (left as is in its frame)
        callf(ctx, 0x000CC4D4u, {p, name, tmpl, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs44(0x001DE9D0u)});
        const u32 f4 = load<u32>(p + 4);
        store<u32>(p + 236, 0);
        const u32 f8 = load<u32>(p + 8);
        store<u32>(p + 4, f4 | 4u);
        store<u32>(p, 0x00B17DE0u);
        const u32 src = tmpl != 0 ? tmpl + 632 : 0x01164AFCu;
        const u32 x = load<u32>(src);
        store<u32>(p + 8, f8 | 0x40000000u);
        store<u32>(p + 240, x);
        Cp44(p + 244, src + 4);
        ctx.fpr[1] = Lfs44(src + 4);
    }
    call(ctx, 0x000CBE10u, {a, b, p});
    if (static_cast<u32>(call(ctx, 0x004F9C9Cu, {b, 0xBF3F0DB1u, p + 240, p + 240})) == 0) {
        const f64 k = Lfs44(0x001DE9CCu);
        const f64 x = F44(Lfs44(p + 240) * k), y = F44(Lfs44(p + 244) * k);
        store<u32>(p + 240, W44(x));
        store<u32>(p + 244, W44(y));
        ctx.fpr[1] = x;
    }
    ret(ctx, p);
}

// u32 f(T* o, u32 who): unless state +428 is 0 / 3 and who is not the
// player (*0x00F65CA0 + 4216) (r3 = 0): Emitter_Stop(+484, 1); when
// Ptr_DerefNonZero(+216->+184) is non-zero, a new sound at the listener 0x00F65CA0 + 0x128260 (0x00210D80(o, &pos, &a, &b)) unless
// the mute byte 0x01055B48 is set: +484 = Sound_Play44C474(&h, snd, &pos,
// &a, 0, 0, 0, +612, stack o, 0, 0). Then for states other than 0 / 3
// Area_Enter(o, who) (r3 = its), else r3 = 1.
DECOMP(002114FC, Area_TouchSound_002114FC) {
    const u32 o = arg<u32>(ctx, 0), who = arg<u32>(ctx, 1);
    const u32 st = load<u32>(o + 428);
    const u32 g = load<u32>(o + 216);
    if (!(st == 0 || st == 3) && who != load<u32>(load<u32>(0x00F65CA0u) + 4216)) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x00443424u, {load<u32>(o + 484), 1});
    if (static_cast<u32>(call(ctx, 0x00446960u, {load<u32>(g + 184)})) != 0) {
        Stack frame(ctx, 32);
        const u32 s = frame.addr();  // the original's sp + 144 (pos), + 160 / + 164, + 168 (handle)
        const u32 v = load<u32>(0x00F65CA0u) + 0x00128260u;
        for (u32 k = 0; k < 3; ++k) Cp44(s + 4 * k, v + 4 * k);
        ctx.fpr[1] = Lfs44(v + 8);
        call(ctx, 0x00210D80u, {o, s, s + 16, s + 20});
        if (load<u8>(0x01055B48u) == 0) {
            call(ctx, 0x0044C474u, {s + 24, load<u32>(g + 184), s, s + 16, 0, 0, 0, load<u32>(o + 612), o, 0, 0});
            store<u32>(o + 484, load<u32>(s + 24));
        }
    }
    const u32 st2 = load<u32>(o + 428);
    if (st2 == 0 || st2 == 3) {
        ret(ctx, 1);
        return;
    }
    call(ctx, 0x0020E7A4u, {o, who});
}

// u32 f(T* o, Def* def, u8 force): with an effect +180 and (def or force):
// Effect_Release(+180, 0x00010CBB), bit 9 of its +136 set, +180 = 0. With
// def: +180 = Obj_CreateEffectDCA20(+8, def, 0, 1); when its component
// 0x25431D90 is of kind 2 (vt[32]) and o->+8->vt[64]() is set, the
// component's +20 = the high half of that object's +428; bit 9 of the
// effect's +136 cleared and Effect_SetTime(+180; *0x00274EA8). r3 = +180
// (0 / the old effect without def).
DECOMP(00274EB0, Obj_SwapEffect_00274EB0) {
    const u32 o = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1);
    const u32 force = arg<u32>(ctx, 2) & 0xFFu;
    u32 r = load<u32>(o + 180);
    if (r != 0 && (def != 0 || force != 0)) {
        call(ctx, 0x004CF3C4u, {r, 0x00010CBBu});
        const u32 e = load<u32>(o + 180);
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        store<u32>(o + 180, 0);
        r = 0;
    }
    if (def == 0) {
        ret(ctx, r);
        return;
    }
    const u32 c = static_cast<u32>(call(ctx, 0x000DCA20u, {load<u32>(o + 8), def, 0, 1}));
    store<u32>(o + 180, c);
    if (c == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 cp = static_cast<u32>(call(ctx, 0x004BB4B4u, {c, 0x25431D90u}));
    if (cp != 0 && static_cast<s32>(VCall(ctx, cp, 32, {cp})) == 2) {
        const u32 w = load<u32>(o + 8);
        if (static_cast<u32>(VCall(ctx, w, 64, {w})) != 0) {
            const u32 w2 = load<u32>(o + 8);
            const u32 q = static_cast<u32>(VCall(ctx, w2, 64, {w2}));
            store<u32>(cp + 20, load<u32>(q + 428) >> 16);
        }
    }
    const u32 e = load<u32>(o + 180);
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
    callf(ctx, 0x004CF710u, {load<u32>(o + 180)}, {Lfs44(0x00274EA8u)});
    ret(ctx, load<u32>(o + 180));
}

// u32 f(T* o): the stance of o: k = bits 28..30 of +416 of o->+276->vt[64]
// (0 / none: 0); with the +24 part's vt[32](+24, 0) == 2: k 1 / 2 / 3 give
// 2 / 5 / 8, else 0 / 3 / 6 plus 1 when the u8 +336 is set. r3 = it.
DECOMP(002C39A0, Obj_Stance_002C39A0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 276);
    const u32 r = static_cast<u32>(VCall(ctx, w, 64, {w}));
    if (r == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 k = (load<u32>(r + 416) >> 28) & 7u;
    if (k == 0 || k > 3) {
        ret(ctx, 0);
        return;
    }
    const u32 base = 3 * (k - 1);
    if (static_cast<s32>(VCall(ctx, o + 24, 32, {o + 24, 0})) == 2) {
        ret(ctx, base + 2);
        return;
    }
    ret(ctx, base + (load<u8>(o + 336) != 0 ? 1u : 0u));
}

// void f(T* o) (destructor body): vtable 0x00B1D478; each of the +24
// parts of the array +16 gets vt[16](p) and, when still set, vt[4](p) and
// its slot cleared; the array freed (Mem_FreeFrom(+16, 0), +16 = 0); the
// part +20 likewise (vt[16], vt[4], +20 = 0); Mem_FreeFrom(+4, 0) and +4 =
// 0. r3 as the original leaves it (0 without +4).
DECOMP(002E87D4, Obj_Dtor_B1D478_002E87D4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1D478u);
    if (load<u32>(o + 24) != 0) {
        for (u32 i = 0;;) {
            u32 p = load<u32>(load<u32>(o + 16) + 4 * i);
            VCall(ctx, p, 16, {p});
            p = load<u32>(load<u32>(o + 16) + 4 * i);
            if (p != 0) {
                VCall(ctx, p, 4, {p});
                store<u32>(load<u32>(o + 16) + 4 * i, 0);
            }
            ++i;
            if (!(i < load<u32>(o + 24))) break;
        }
    }
    const u32 a = load<u32>(o + 16);
    if (a != 0) {
        call(ctx, 0x0054EE6Cu, {a, 0});
        store<u32>(o + 16, 0);
    }
    u32 q = load<u32>(o + 20);
    if (q != 0) {
        VCall(ctx, q, 16, {q});
        q = load<u32>(o + 20);
        if (q != 0) VCall(ctx, q, 4, {q});
        store<u32>(o + 20, 0);
    }
    const u32 b = load<u32>(o + 4);
    if (b == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x0054EE6Cu, {b, 0});
    store<u32>(o + 4, 0);
}

// void f(T* o, u32 a) (constructor): StateMachine_ConstructThunk(o, 1, 0,
// 0), +28 = 0, +32 = a, vtable 0x00B1DCA8, +36 / +40 = 0, the PtrVector
// +44 (u16 +50 = 4, PtrVector_Init(v, 16, 0, 1)), u8 +64 / +68 / +72 = 0,
// vtable 0x00B1DD18, SM_SetState_003044B0(o, 0x86A9B514); then the
// String_HashNoCase of each name of the 3-entry table 0x00AD9970 that is
// set is pushed onto +44 (count +52, capacity +48, high-water +54;
// PtrVector_Reserve(v, 2 * capacity) when full). r3 as the original
// leaves it.
DECOMP(00304C14, Sm_Ctor_B1DD18_00304C14) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    call(ctx, 0x0045CBE8u, {o, 1, 0, 0});
    store<u32>(o + 28, 0);
    store<u32>(o + 32, a);
    store<u32>(o, 0x00B1DCA8u);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    call(ctx, 0x005516E0u, {o + 44});
    store<u16>(o + 50, 4);
    call(ctx, 0x00551598u, {o + 44, 16, 0, 1});
    store<u8>(o + 64, 0);
    store<u32>(o + 68, 0);
    store<u32>(o + 72, 0);
    store<u32>(o, 0x00B1DD18u);
    call(ctx, 0x003044B0u, {o, 0x86A9B514u});
    for (u32 i = 0; i < 3; ++i) {
        const u32 s = load<u32>(0x00AD9970u + 4 * i);
        if (s == 0) continue;
        const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
        const u32 cap = load<u16>(o + 48);
        if (load<u16>(o + 52) == cap) call(ctx, 0x00551718u, {o + 44, (cap << 1) & 0x1FFFEu});
        store<u32>(load<u32>(o + 44) + ((static_cast<u32>(load<u16>(o + 52)) << 2) & 0x3FFFCu), h);
        const u32 c = static_cast<u32>(load<u16>(o + 52)) + 1;
        const u32 hw = load<u16>(o + 54);
        store<u16>(o + 52, static_cast<u16>(c));
        store<u16>(o + 54, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFFu) ? hw : c));
        ret(ctx, hw);
    }
}

// u32 f(T* o, Anim* a, u8 reverse, -, -, -, u32 key; f32 w, f32 t): with a
// and the skeleton +24: a 160-byte control from the allocator of the
// r13 global (-28332)->+44 (vt[8](al, 160); its u16 +4 = 160 written even
// for a null result: a quirk), Control_Init_00A4C600(c, a, 1, reverse ?
// -1 : 1); +36 = c, c->+8 = t, +64 = w, +60 = 1.0;
// AnimSkel_AddControl_00A45478(+24, c), AnimSkel_Release7D4EA8(+36); with
// key: o->vt[12](o, *SortedMap_Find76E0A8(0x0106BBCC, &key)) (entry / OPD
// after the arguments). r3 as the original leaves it.
DECOMP(003535C4, Obj_AddAnimControl_003535C4) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 rev = arg<u32>(ctx, 2) & 0xFFu, key = arg<u32>(ctx, 5);
    const f64 w = farg(ctx, 0), t = farg(ctx, 1);
    if (a == 0) {
        ret(ctx, o);
        return;
    }
    if (load<u32>(o + 24) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 al = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) - 28332u) + 44);
    const u32 c = static_cast<u32>(VCall(ctx, al, 8, {al, 160}));
    store<u16>(c + 4, 160);  // written even when the allocation failed (address 4)
    u32 cc = 0;
    if (c != 0) {
        call(ctx, 0x00A4C600u, {c, a, 1, rev != 0 ? 0xFFFFFFFFFFFFFFFFull : 1ull});
        cc = c;
    }
    store<u32>(o + 36, cc);
    store<u32>(cc + 8, W44(t));
    const u32 c2 = load<u32>(o + 36);
    store<u32>(c2 + 64, W44(w));
    store<u32>(c2 + 60, 0x3F800000u);
    call(ctx, 0x00A45478u, {load<u32>(o + 24), c2});
    call(ctx, 0x007D4EA8u, {load<u32>(o + 36)});
    if (key == 0) return;
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u32>(s, key);
    const u32 v = load<u32>(static_cast<u32>(call(ctx, 0x0076E0A8u, {0x0106BBCCu, s})));
    VCallEO36(ctx, load<u32>(o), 12, {o, v});
}

// u32 f(T* o, u8 now): unless already in state 2 (+460) and not now: the
// nine 36-byte motions at +132 get their tuning rates (the tuning vars
// 0x01088008 / 0x01088058 / 0x01087F68 / 0x01087FB8 / 0x01088148 /
// 0x01088198 / 0x010880A8 / 0x010880F8 / 0x01087B08 into +24 of each),
// u16 +16 = 1, +20 = +28, +4 = 0 and Motion_Set4A0C20(m, 1; *0x01087F18)
// (and Obj_Blend_004A0E60(m; same) when now); +460 = 2. r3 = 2; f1 = that
// rate.
DECOMP(003ACFDC, Obj_ResetMotions_003ACFDC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 now = arg<u32>(ctx, 1) & 0xFFu;
    if (static_cast<s32>(load<u32>(o + 460)) == 2 && now == 0) {
        ret(ctx, 2);
        return;
    }
    const f64 r = Lfs44(0x01087F18u);
    static const u32 tv[9] = {0x01088008u, 0x01088058u, 0x01087F68u, 0x01087FB8u, 0x01088148u,
                              0x01088198u, 0x010880A8u, 0x010880F8u, 0x01087B08u};
    for (u32 k = 0; k < 9; ++k) Cp44(o + 156 + 36 * k, tv[k]);
    for (u32 i = 0; i < 9; ++i) {
        const u32 m = o + 132 + 36 * i;
        const u32 v = load<u32>(m + 28);
        store<u16>(m + 16, 1);
        store<u32>(m + 20, v);
        store<u32>(m + 4, 0);
        callf(ctx, 0x004A0C20u, {m, 1}, {r});
        if (now != 0) callf(ctx, 0x004A0E60u, {m}, {r});
    }
    store<u32>(o + 460, 2);
    ret(ctx, 2);
}

// u32 f(FileReader* rd): loads a zipped xmlb chunk: n = Iff_ChunkSize(rd)
// (0: r3 = 0); buf = MemStack_Alloc(Mem_GetDefaultStack(), n),
// FileReader_Read(rd, buf, n), z = Archive_OpenZip(buf, n, *0x00F65FEC);
// a stack XmlDoc (XmlDoc_Init4FA7A4) loaded from z (XmlDoc_LoadXmlb(doc,
// z, 0)) feeds Clip_Load4B20B8(*0x00F65F84, doc, z + 4), else
// String_Format("%s" 0x00AEE488, z + 4); XmlDoc_Destruct(doc),
// z->vt[8](z), the default stack's vt[8](stack, buf) and *0x00F65FEC = 0.
// r3 = 0.
DECOMP(004B2C1C, ClipChunk_Load_004B2C1C) {
    const u32 rd = arg<u32>(ctx, 0);
    const u32 n = static_cast<u32>(call(ctx, 0x005637D4u, {rd}));
    if (n == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 st = static_cast<u32>(call(ctx, 0x005525FCu, {}));
    const u32 buf = static_cast<u32>(call(ctx, 0x0055260Cu, {st, n}));
    call(ctx, 0x00563768u, {rd, buf, n});
    const u32 z = static_cast<u32>(call(ctx, 0x00567034u, {buf, n, load<u32>(0x00F65FECu)}));
    const u32 lib = load<u32>(0x00F65F84u);
    Stack frame(ctx, 2272);  // the original's sp + 112 .. + 2384 (the XmlDoc)
    const u32 doc = frame.addr();
    call(ctx, 0x004FA7A4u, {doc});
    if (static_cast<u32>(call(ctx, 0x004FACCCu, {doc, z, 0})) == 0) {
        call(ctx, 0x005534D0u, {0x00AEE488u, z + 4});
        call(ctx, 0x004FA9C0u, {doc});
    } else {
        call(ctx, 0x004B20B8u, {lib, doc, z + 4});
        call(ctx, 0x004FA9C0u, {doc});
    }
    VCall(ctx, z, 8, {z});
    const u32 st2 = static_cast<u32>(call(ctx, 0x005525FCu, {}));
    VCall(ctx, st2, 8, {st2, buf});
    store<u32>(0x00F65FECu, 0);
    ret(ctx, 0);
}

// u32 f(const Quat* q, f32* x, f32* y, f32* z): the Euler angles of q
// (k = *0x00757EB0): *z = Libm_Unary5E6578(k (q.z q.w + q.x q.y)) (asin);
// at the poles (|*z| >= *0x00757EB4, pi: never for a real asin): *x = 0, *y = +-atan2(q.x, q.w) by
// the sign of *z against *0x00757EB8; else *y = atan2(k (y w - x z), 1 -
// k (y^2 + z^2)) and *x = atan2(k (x w - y z), 1 - k (x^2 + z^2)) (1 =
// *0x00757EBC). r3 / f1 as the original leaves them.
DECOMP(00757EC0, Quat_ToEuler_00757EC0) {
    const u32 q = arg<u32>(ctx, 0), px = arg<u32>(ctx, 1), py = arg<u32>(ctx, 2), pz = arg<u32>(ctx, 3);
    const u32 K = 0x00757EB0u;
    const f64 k = Lfs44(K);
    {
        f64 a = F44(Lfs44(q + 8) * k);
        const f64 b = F44(Lfs44(q) * k);
        a = F44(a * Lfs44(q + 12));
        a = Fma44(b, Lfs44(q + 4), a);
        callf(ctx, 0x005E6578u, {}, {a});
    }
    store<u32>(pz, W44(ctx.fpr[1]));
    if (std::fabs(ctx.fpr[1]) > Lfs44(K + 4) || std::fabs(ctx.fpr[1]) == Lfs44(K + 4)) {
        const f64 sg = Lfs44(K + 8);
        store<u32>(px, 0);
        callf(ctx, 0x005E665Cu, {}, {Lfs44(q), Lfs44(q + 12)});
        const f64 ang = ctx.fpr[1];
        store<u32>(py, W44(ang));
        const f64 zz = Lfs44(pz);
        const f64 r = (zz > sg || zz == sg) ? ang : -ang;
        store<u32>(py, W44(r));
        ctx.fpr[1] = r;
        return;
    }
    const f64 one = Lfs44(K + 12);
    {
        const f64 y = Lfs44(q + 4);
        const f64 x2 = F44(Lfs44(q) * k), y2 = F44(y * k);
        const f64 z = Lfs44(q + 8), w = Lfs44(q + 12);
        const f64 z2 = F44(z * k);
        const f64 xz = F44(x2 * z);
        const f64 t7 = F44(-std::fma(y2, y, -one));
        const f64 num = Fms44(y2, w, xz);
        const f64 den = F44(-std::fma(z2, z, -t7));
        callf(ctx, 0x005E665Cu, {}, {num, den});
    }
    store<u32>(py, W44(ctx.fpr[1]));
    {
        const f64 x = Lfs44(q), y = Lfs44(q + 4);
        const f64 x2 = F44(x * k), y2 = F44(y * k);
        const f64 z = Lfs44(q + 8), w = Lfs44(q + 12);
        const f64 z2 = F44(z * k);
        const f64 t1 = F44(-std::fma(x2, x, -one));
        const f64 yz = F44(y2 * z);
        const f64 den = F44(-std::fma(z2, z, -t1));
        const f64 num = Fms44(x2, w, yz);
        callf(ctx, 0x005E665Cu, {}, {num, den});
    }
    store<u32>(px, W44(ctx.fpr[1]));
}

// ---------------------------------------------------------------- sweep: 97 instructions
// u32 f(T* o; f32 dt): with a held object +256 that lost bit 18 of its
// +172: unless o->vt[124](o), held->vt[40](held, 0); +256 = 0,
// o->+48->vt[96](+48, 0) and in its +164 part bit 12 cleared / bit 10 set
// in the u16 +40. Then unless bits 6 / 5 of +48->+172 are set: when the
// surface table byte (+308->+12)[4 * (*0x00F9488C)->+16->+64] is set and
// the type +4 is not 0 / 13 / 14, Obj_SnapToGround_0001D548(o, 0); then
// 0x00012E70(o; dt). r3 as the original leaves it.
DECOMP(0001DCF0, Ai_UpdateHeld_0001DCF0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 h = load<u32>(o + 256);
    if (h != 0 && ((load<u32>(h + 172) >> 18) & 1u) == 0) {
        if (static_cast<u32>(VCall(ctx, o, 124, {o})) == 0) {
            const u32 q = load<u32>(o + 256);
            VCall(ctx, q, 40, {q, 0});
        }
        const u32 p = load<u32>(o + 48);
        store<u32>(o + 256, 0);
        VCall(ctx, p, 96, {p, 0});
        const u32 c = load<u32>(load<u32>(o + 48) + 164);
        if (c != 0) store<u16>(c + 40, static_cast<u16>((load<u16>(c + 40) & ~0x1000u) | 0x400u));
    }
    const u32 f = load<u32>(load<u32>(o + 48) + 172);
    if ((f >> 6) & 1u) {
        ret(ctx, f);
        return;
    }
    if ((f >> 5) & 1u) {
        ret(ctx, 1);
        return;
    }
    const u32 g = load<u32>(0x00F9488Cu + 16);
    const u32 idx = load<u32>(g + 64);
    if (load<u8>(load<u32>(load<u32>(o + 308) + 12) + (idx << 2)) != 0) {
        const s32 t = static_cast<s32>(load<u32>(o + 4));
        if (!(t == 0 || t == 13 || t == 14)) call(ctx, 0x0001D548u, {o, 0});
    }
    callf(ctx, 0x00012E70u, {o}, {dt});
}

// u32 f(T* o): moves the u16 +460 pending ids of the array +452 into the
// 8-byte (id, f32) vector +492 (count +500, capacity +496, high-water
// +502; PtrVector_Reserve(v, 2 * capacity) when full) with the weight
// *0x00040204 (the tuning var 0x00F9716C for odd entries), then +460 = 0;
// the player (*0x00F65CA0 + 4216) with bits 1 and 3 of +192 gets
// SetRef_00121F10(player, 0). r3 as the original leaves it.
DECOMP(00040208, Ai_FlushPending_00040208) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u16>(o + 460); ++i) {
        const u32 id = load<u32>(load<u32>(o + 452) + 4 * i);
        const u32 wsrc = (i & 1u) ? 0x00F9716Cu : 0x00040204u;
        const u32 cap = load<u16>(o + 496);
        if (load<u16>(o + 500) == cap) call(ctx, 0x00551718u, {o + 492, (cap << 1) & 0x1FFFEu});
        const u32 d = load<u32>(o + 492) + ((static_cast<u32>(load<u16>(o + 500)) << 3) & 0x7FFF8u);
        store<u32>(d, id);
        Cp44(d + 4, wsrc);
        const u32 c = static_cast<u32>(load<u16>(o + 500)) + 1;
        const u32 hw = load<u16>(o + 502);
        store<u16>(o + 500, static_cast<u16>(c));
        store<u16>(o + 502, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFFu) ? hw : c));
    }
    store<u16>(o + 460, 0);
    const u32 slot = load<u32>(0x00F65CA0u) + 4208;
    const u32 p = load<u32>(slot + 8);
    if (p == 0) {
        ret(ctx, slot);
        return;
    }
    if ((load<u32>(p + 192) & 0x2u) == 0 || (load<u32>(p + 192) & 0x8u) == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x00121F10u, {p, 0});
}

// u32 f(T* o, Ctx* x): resolves the target t: the +280 object of
// Effect_Bind4BB7E0(x, +16, 0) when it is of class 0x02235C33, else
// Obj_FindById(+12) (Hash_ToString of the missing names on the way);
// with bit 17 of t->+196 set: Obj_SetFlags_001B7974(t, u8 +21, 1) when
// the u8 +20 is set, else t->vt[28](t, 0, 0); without it Hash_ToString of
// +16 (or +12). r3 as the original leaves it.
DECOMP(0008FCA0, Target_SetFlags_0008FCA0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    bool found = false;
    const u32 h = load<u32>(o + 16);
    if (h != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, h, 0}));
        if (c != 0) {
            if (load<u32>(c + 276) == 0x02235C33u) t = load<u32>(c + 280);
            found = t != 0;
        }
        if (!found) call(ctx, 0x0056A6E0u, {load<u32>(o + 16)});
    }
    if (!found) {
        const u32 id = load<u32>(o + 12);
        if (id == 0) {
            ret(ctx, 0);
        } else {
            t = static_cast<u32>(call(ctx, 0x000D6A28u, {id}));
            if (t == 0) call(ctx, 0x0056A6E0u, {load<u32>(o + 12)});
        }
    }
    if (t == 0) return;
    if ((load<u32>(t + 196) & 0x00020000u) == 0) {
        const u32 a = load<u32>(o + 16);
        if (a != 0) {
            call(ctx, 0x0056A6E0u, {a});
        } else {
            const u32 b = load<u32>(o + 12);
            if (b != 0) call(ctx, 0x0056A6E0u, {b});
            else ret(ctx, 0);
        }
        return;
    }
    if (load<u8>(o + 20) != 0) call(ctx, 0x001B7974u, {t, load<u8>(o + 21), 1});
    else VCall(ctx, t, 28, {t, 0, 0});
}

// T* f(?, XmlCursor* cur): a new 40-byte record (Mem_Alloc(40, 0, 16),
// Obj_InitVt_B21C98(p, cur); vtable 0x00B11028, +12..+36 = 0): hashes
// 0x8A93973F / 0x03D1FCFC / 0x61CAD819 / 0x806E6EC3 into +12 / +16 / +20
// / +24, f32 0x77C17F6F into +28, hash 0x0027873A into +36, f32
// 0x1B75F0DA into +32 (defaults the current values). r3 = the record (0
// when the allocation fails); f1 = the old +32.
DECOMP(0009B9FC, XmlRec_New_B11028_0009B9FC) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {40, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, cur});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B11028u);
    store<u32>(p + 24, 0);
    store<u32>(p + 28, 0);
    store<u32>(p + 32, 0);
    store<u32>(p + 36, 0);
    call(ctx, 0x004FA14Cu, {cur, 0x8A93973Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {cur, 0x03D1FCFCu, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {cur, 0x61CAD819u, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004FA14Cu, {cur, 0x806E6EC3u, p + 24, load<u32>(p + 24)});
    callf(ctx, 0x004F9B20u, {cur, 0x77C17F6Fu, p + 28}, {Lfs44(p + 28)});
    call(ctx, 0x004FA14Cu, {cur, 0x0027873Au, p + 36, load<u32>(p + 36)});
    callf(ctx, 0x004F9B20u, {cur, 0x1B75F0DAu, p + 32}, {Lfs44(p + 32)});
    ret(ctx, p);
}
