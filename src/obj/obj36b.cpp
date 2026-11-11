// Game code, part 36 (continued): the 73 instruction functions and up.
// Each was checked against the original in the PPC simulator (memory
// writes, calls and their arguments, r3 and f1).
#include "obj36.h"

using namespace sly4;
using namespace sly4::obj36;

// ---------------------------------------------------------------- sweep: 73 instructions
// void f(T* o, a): unless bit 0 of u8 o->+304 is set already: the byte
// of o->+308->+12 indexed by (*0x00F94B54)->+64 decides: when clear, a
// zeroed 24-byte stack event (Event3_Init_00464F88(&ev, o->+48, 0, 0, 0))
// goes to World_Query482D14(*0x00F65CA0, 0, &ev); bit 0 of +304 is set
// and, unless bit 2 of u8 +465, o->+28 = 19. Then o->vt[284](o, a) and
// o->+440 = 0. r3 = 0.
DECOMP(0003094C, Obj_Activate_0003094C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    u32 b = load<u8>(o + 304);
    if ((b & 1u) == 0) {
        const u32 x = load<u32>(load<u32>(0x00F94B54u) + 64);
        if (load<u8>(load<u32>(load<u32>(o + 308) + 12) + (x << 2)) == 0) {
            Stack fr(ctx, 32);
            const u32 ev = fr.addr();
            for (u32 k = 0; k < 24; k += 4) store<u32>(ev + k, 0);
            call(ctx, 0x00464F88, {ev, load<u32>(o + 48), 0, 0, 0});
            call(ctx, 0x00482D14, {load<u32>(0x00F65CA0u), 0, ev});
            b = load<u8>(o + 304);
        }
        const u32 c = load<u8>(o + 465);
        store<u8>(o + 304, static_cast<u8>(b | 1u));
        if ((c & 2u) == 0) store<u32>(o + 28, 19);
    }
    VCall(ctx, o, 284, {o, a});
    store<u32>(o + 440, 0);
    ret(ctx, 0u);
}

// void f(T* o): 0x00012928(o); the o->+1816 objects of the stack +1820
// are popped (count decremented, then top->vt[40](top, 0)); the world
// byte *0x00F65CA0 + 0x1285A5 cleared; a layer o->+2332 is popped from
// the world's +0x1281E0 (Obj_PopLayer(it, layer; f1 = the float at
// 0x0004B0C8), when set), released (vt[4]) and the word cleared. r3 = 0.
DECOMP(0004B0D0, Obj_PopAll_0004B0D0) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00012928, {o});
    s32 n = static_cast<s32>(load<u32>(o + 1816));
    while (n > 0) {
        const u32 k = static_cast<u32>(n - 1);
        store<u32>(o + 1816, k);
        const u32 p = load<u32>(o + 1820 + (k << 2));
        VCall(ctx, p, 40, {p, 0});
        n = static_cast<s32>(load<u32>(o + 1816));
    }
    store<u8>(load<u32>(0x00F65CA0u) + 0x001285A5u, 0);
    u32 c = load<u32>(o + 2332);
    if (c == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = load<u32>(load<u32>(0x00F65CA0u) + 0x001281E0u);
    if (w != 0) {
        callf(ctx, 0x000FBB88, {w, c}, {Lfs32(0x0004B0C8u)});
        c = load<u32>(o + 2332);
    }
    if (c != 0) VCall(ctx, c, 4, {c});
    store<u32>(o + 2332, 0);
    ret(ctx, 0u);
}

// void f(T* o, X* x): the target: when o->+12 names a component of x
// (Effect_FindComponent) whose vt[32]() is 5 and whose +28 owner has the
// class hash 0x02235C33 at +276, that owner's +280; otherwise
// Obj_FindById(o->+16) when +16 is set. With a hash o->+20 the target
// becomes Obj_FindByHash_000DB290(target, hash). A target then goes to
// 0x002EF648(*0x00F64C38, it). r3 = 0 or that call's.
DECOMP(000746B0, Effect_SendToTarget_000746B0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    const u32 id = load<u32>(o + 12);
    if (id != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, id}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 p = load<u32>(c + 28);
            if (p != 0 && load<u32>(p + 276) == 0x02235C33u) t = load<u32>(p + 280);
        }
    }
    if (t == 0) {
        const u32 k = load<u32>(o + 16);
        if (k != 0) t = static_cast<u32>(call(ctx, 0x000D6A28, {k}));
    }
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 h = load<u32>(o + 20);
    if (h != 0) {
        t = static_cast<u32>(call(ctx, 0x000DB290, {t, h}));
        if (t == 0) return;
    }
    call(ctx, 0x002EF648, {load<u32>(0x00F64C38u), t});
}

// void f(T* o, a, b): nothing for a mode o->+16 of 0 (r3 = 0). Mode 1:
// missing = the key o->+12 is in the map *0x00F65BE8 + 4
// (SortedMap_Find733F8C, a stack copy of the key) and
// 0x002E6B8C(*0x00F64C20, entry, 0) gives 0. Unless u8 o->+20 differs
// from missing (r3 = it), p->vt[12](p, a, b) for each object of the list
// +24 (u16 count +32; r3 = the count).
DECOMP(0007DF18, Handler_RunAll_0007DF18) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 k = load<u32>(o + 16);
    if (k == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 missing = 0;
    if (k == 1) {
        Stack fr(ctx, 16);
        const u32 t = fr.addr();
        store<u32>(t, load<u32>(o + 12));
        const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x00733F8C, {load<u32>(0x00F65BE8u) + 4, t})));
        if (e != 0 && static_cast<u32>(call(ctx, 0x002E6B8C, {load<u32>(0x00F64C20u), e, 0})) == 0) missing = 1;
    }
    const u32 f = load<u8>(o + 20);
    if (missing != f) {
        ret(ctx, f);
        return;
    }
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 32);
        if (!(i < n)) {
            ret(ctx, n);
            return;
        }
        const u32 p = load<u32>(load<u32>(o + 24) + 4 * i);
        VCall(ctx, p, 12, {p, a, b});
    }
}

// T* New(-, Xml* x): NewXml34 (24 bytes): +12 cleared, u8 +16 cleared,
// vtable 0x00B0FE00, the colour at 0x01164CA0 copied (bytewise) to +17;
// the hash 0x8A93973F to +12 and the bool 0xCADB1721 to +16 (current
// defaults), the colour 0x03D53743 to +17 (XmlCursor_ReadColor; default
// 0x01164CA0).
DECOMP(000819D8, VtB0FE00_NewXml_000819D8) {
    NewXml34(ctx, 24, [&](u32 m, u32 x) {
        constexpr u32 g = 0x01164CA0u;
        store<u32>(m + 12, 0);
        store<u8>(m + 16, 0);
        store<u32>(m, 0x00B0FE00u);
        for (u32 k = 0; k < 4; ++k) store<u8>(m + 17 + k, load<u8>(g + k));
        RdHashCur34(ctx, x, 0x8A93973Fu, m + 12);
        RdBoolCur34(ctx, x, 0xCADB1721u, m + 16);
        call(ctx, 0x004F9F9C, {x, 0x03D53743u, m + 17, g});
    });
}

// void f(T* o, X* x): the value v = o->+16, or c->vt[24](c) when o->+20
// names a component c of x (Effect_FindComponent) whose vt[32]() is 2;
// with o->+12 set, a 36-byte command (SmallAlloc) built by
// VtB20D88_Ctor(m, o->+12, v, o->+24, -, -, o->+36; f1 / f2 = o->+28 /
// +32; r7 / r8 as left) goes to CmdList_Push_0042C138 (0 when the
// allocation fails). r3 = 0 (no +12) or the push's.
DECOMP(0009BB80, Effect_PushCmd_0009BB80) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 v = load<u32>(o + 16);
    const u32 id = load<u32>(o + 20);
    if (id != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, id}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 2) v = static_cast<u32>(VCall(ctx, c, 24, {c}));
    }
    if (load<u32>(o + 12) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 m = static_cast<u32>(call(ctx, 0x0055238C, {36}));
    if (m != 0) {
        callf(ctx, 0x0042DAB4,
              {m, load<u32>(o + 12), v, load<u32>(o + 24), ctx.gpr[7], ctx.gpr[8], load<u32>(o + 36)},
              {Lfs32(o + 28), Lfs32(o + 32)});
    }
    call(ctx, 0x0042C138, {m});
}

// void f() (module shutdown): the byte 0x0101278A cleared; for each of
// the flags 0x00F645AD / AC / AB / AA / A9 that is 1, its shutdown stub
// (0x00ABE884 / 0x00ABE8A4 / 0x00ABE784 / 0x00ABDE24 / 0x00ABDD84) and
// the flag cleared; 0x00F645A8 cleared when 1; a buffer *0x0101278C is
// freed (Mem_Free) and 0x0101278C / 0x01012790 cleared (r3 =
// 0x01010000). r3 = 0 without the buffer.
DECOMP(000AE660, Module_Shutdown_000AE660) {
    store<u8>(0x0101278Au, 0);
    static constexpr u32 kStub[] = {0x00ABE884u, 0x00ABE8A4u, 0x00ABE784u, 0x00ABDE24u, 0x00ABDD84u};
    for (u32 k = 0; k < 5; ++k) {
        const u32 f = 0x00F645ADu - k;
        if (load<u8>(f) == 1) {
            call(ctx, kStub[k], {1});
            store<u8>(f, 0);
        }
    }
    if (load<u8>(0x00F645A8u) == 1) store<u8>(0x00F645A8u, 0);
    const u32 p = load<u32>(0x0101278Cu);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x005611B0, {p});
    store<u32>(0x0101278Cu, 0);
    store<u32>(0x01012790u, 0);
    ret(ctx, 0x01010000u);
}

// void f(T* o, u32 g): the group g: o->vt[144]() (b) gets g in the high
// half of +44 (low 5 bits kept, the rest cleared) and, with a Havok
// entity b->+8, Hk_UpdateEntityGroup(entity, b, 0, 1); o->+204 = g; then
// c->vt[104](c, g) for the set +192 object c of each 240-byte child record
// of o's list +340 (data +0, u16 count +8). r3 = the list word.
DECOMP(000DA138, Obj_SetGroup_000DA138) {
    const u32 o = arg<u32>(ctx, 0), g = arg<u32>(ctx, 1);
    const u32 b = static_cast<u32>(VCall(ctx, o, 144, {o}));
    if (b != 0) {
        const u32 e = load<u32>(b + 8);
        store<u32>(b + 44, (load<u32>(b + 44) & 0x1Fu) | (g << 16));
        if (e != 0) call(ctx, 0x00930B70, {e, b, 0, 1});
    }
    store<u32>(o + 204, g);
    for (u32 i = 0, off = 0;; ++i, off += 240) {
        const u32 l = load<u32>(o + 340);
        const u32 n = l != 0 ? load<u16>(l + 8) : 0u;
        if (!(i < n)) {
            ret(ctx, l);
            return;
        }
        const u32 c = load<u32>(load<u32>(l) + off + 192);
        if (c != 0) VCall(ctx, c, 104, {c, g});
    }
}

// bool f(T* o): false unless Obj_SyncBodyShapes(o) (f1 passed through).
// The flag = (o->vt[32](o, 0) is 4 and o->+12's first float is above
// o->+216's +420) or (bit 0x04000000 of o->+580 and the state is 6);
// p->vt[304](p, flag) for each set part p of o +588 + 20i (6). True;
// f1 as left.
DECOMP(001C7B38, Obj_SyncParts_001C7B38) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x000E5000, {o})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 t = load<u32>(o + 216);
    const u32 k = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
    u32 flag = 0;
    bool hit = false;
    if (k == 4) {
        const f64 lim = Lfs32(t + 420);
        ctx.fpr[1] = lim;
        hit = Lfs32(load<u32>(o + 12)) > lim;
    }
    if (hit) flag = 1;
    else if ((load<u32>(o + 580) & 0x04000000u) != 0 && k > 4 && k == 6) flag = 1;
    for (u32 i = 0; i < 6; ++i) {
        const u32 p = load<u32>(o + 588 + 20 * i);
        if (p != 0) VCall(ctx, p, 304, {p, flag});
    }
    ret(ctx, 1u);
}

// void f(T* o, X* x): p->vt[40](p, 0) for each object of the vector +812
// (u16 count +820), the count cleared (r3 = the count read last); with x:
// u16 x->+178 incremented, x pushed onto that vector (u16 capacity +816
// doubled by PtrVector_Reserve when full, u16 high-water +822) and +808 =
// 0 (r3 = the old high-water).
DECOMP(001CC214, Obj_ResetList_001CC214) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 n;
    for (u32 i = 0;; ++i) {
        n = load<u16>(o + 820);
        if (!(i < n)) break;
        const u32 p = load<u32>(load<u32>(o + 812) + 4 * i);
        VCall(ctx, p, 40, {p, 0});
    }
    store<u16>(o + 820, 0);
    if (x == 0) {
        ret(ctx, n);
        return;
    }
    store<u16>(x + 178, static_cast<u16>(load<u16>(x + 178) + 1u));
    u32 c = load<u16>(o + 820);
    const u32 cap = load<u16>(o + 816);
    if (c == cap) {
        call(ctx, 0x00551718, {o + 812, (cap << 1) & 0x1FFFEu});
        c = load<u16>(o + 820);
    }
    const u32 d = load<u32>(o + 812);
    const u32 c2 = load<u16>(o + 820);
    const u32 hw = load<u16>(o + 822);
    store<u32>(d + ((c << 2) & 0x3FFFCu), x);
    const u32 nc = (c2 + 1u) & 0xFFFFu;
    store<u16>(o + 820, static_cast<u16>(nc));
    store<u16>(o + 822, static_cast<u16>(hw > nc ? hw : nc));
    store<u32>(o + 808, 0);
    ret(ctx, hw);
}

// bool f(T* o; f1 = a, f2 = b): true when o->vt[32](o, 1) is set, u8
// o->+854 is clear, o->+788 is above K (the constant at 0x001CFA08), bit
// 0x20000000 of +552 is set, or neither b < K nor a >= b. Otherwise
// o->vt[420](o, 6, 0) and o->vt[420](o, 3, 0) (OPD / entry in r6 / r7)
// and false. f1 as left (K after the first test).
DECOMP(001CFA10, Obj_TryStop_001CFA10) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    const u32 r = static_cast<u32>(VCall(ctx, o, 32, {o, 1}));
    const u32 vt = load<u32>(o);
    if (r != 0 || load<u8>(o + 854) == 0) {
        ret(ctx, 1u);
        return;
    }
    const f64 K = Lfs32(0x001CFA08u);
    ctx.fpr[1] = K;
    if (!(Lfs32(o + 788) <= K) || (load<u32>(o + 552) & 0x20000000u) != 0 || !(b < K || a >= b)) {
        ret(ctx, 1u);
        return;
    }
    VCallVt(ctx, vt, 420, {o, 6, 0});
    VCall(ctx, o, 420, {o, 3, 0});
    ret(ctx, 0u);
}

// bool f(T* o, Player* p): starts the takedown when nothing else runs
// ((*0x00F64C20)->+76 clear), p is set, bit 31 of o->+592 and bit
// 0x08000000 of o->+216->+452 are set, the byte 0x01054974 is clear and
// bit 0 of p's 64-bit flags +184 is set: a target t =
// Target_GetDef0D951C(o)->+64 with bit 8 of +192 first gets
// Obj_Reset6_0012D290(t, o); then Player_StartAction(*0x00F65CA0, p,
// 0x00AD2C10, 0x00AD2C28, 0x00F34080, o, 0, 0, [stack] 1, 1). r3 = 1.
DECOMP(00201EA4, Obj_TryTakedown_00201EA4) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    ret(ctx, 1u);
    if (load<u32>(load<u32>(0x00F64C20u) + 76) != 0 || p == 0) return;
    const u32 t216 = load<u32>(o + 216);
    if ((load<u32>(o + 592) & 0x80000000u) == 0 || (load<u32>(t216 + 452) & 0x08000000u) == 0 ||
        load<u8>(0x01054974u) != 0 || (load<u32>(p + 188) & 1u) == 0)
        return;
    const u32 t = load<u32>(static_cast<u32>(call(ctx, 0x000D951C, {o})) + 64);
    if (t != 0 && (load<u32>(t + 192) & 8u) != 0) call(ctx, 0x0012D290, {t, o});
    call(ctx, 0x00489A60, {load<u32>(0x00F65CA0u), p, 0x00AD2C10u, 0x00AD2C28u, 0x00F34080u, o, 0, 0, 1, 1});
    ret(ctx, 1u);
}

// void f(T* o, Hit* m): unless the byte 0x010388F0 is set, bit
// 0x00020000 / 0x00040000 of o's flags +188 is set, o's owner
// (Obj_Owner0D93FC) has bit 0x1000 of +40, o->+380 is above K (the
// constant at 0x002295F0), Obj_TestE0BCC(o, m) is 0 or o->+216->+224 is
// not above K: o->vt[352](o, -, m->+4, m->+8; f1 = m->+52 * that float;
// OPD / entry in r4 / r7), and bit 0x100 of u16 m->+14 = (its result is
// 0). r3 / f1 as left (r3 = cntlzw of the result at the end).
DECOMP(002295F8, Obj_ApplyHit_002295F8) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 b = load<u8>(0x010388F0u);
    ctx.gpr[3] = b;
    if (b != 0) return;
    const u32 lo = load<u32>(o + 188);
    ctx.gpr[3] = lo;
    if ((lo & 0x00020000u) != 0) return;
    ctx.gpr[3] = lo & 0x00040000u;
    if ((lo & 0x00040000u) != 0) return;
    if (static_cast<u32>(call(ctx, 0x000D93FC, {o})) != 0) {
        const u32 w = static_cast<u32>(call(ctx, 0x000D93FC, {o}));
        const u32 bit = (load<u32>(w + 40) >> 12) & 1u;
        ctx.gpr[3] = bit;
        if (bit != 0) return;
    }
    const f64 K = Lfs32(0x002295F0u);
    const f64 v = Lfs32(o + 380);
    ctx.fpr[1] = v;
    ctx.gpr[3] = 0x002295F0u;
    if (v > K) return;
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, m})) == 0) return;
    const u32 t = load<u32>(o + 216);
    const f64 s = Lfs32(t + 224);
    ctx.fpr[1] = s;
    ctx.gpr[3] = t;
    if (!(s > K)) return;
    ctx.fpr[1] = static_cast<f32>(Lfs32(m + 52) * s);
    const u32 r = static_cast<u32>(VCallOpdR4_28(ctx, o, 352, load<u32>(m + 4), load<u32>(m + 8)));
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(m + 14, static_cast<u16>((load<u16>(m + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ret(ctx, z);
}

// void f(T* o, u8 on): when bit 31 of o->+384 differs from on: bit
// 0x40000000 of o->+608->+1268 cleared (when set), o->vt[136](o,
// 0x01164B40) and o->vt[384](o, 0x01164B40), +480..+488 and +496..+504 =
// the vector at 0x01164B40, +512..+524 = 0 (in the original's order).
// Then SetBit384_36. f1 as left.
DECOMP(00244468, Obj_SetActive_00244468) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    if ((on & 0xFFu) != (load<u32>(o + 384) >> 31)) {
        const u32 p = load<u32>(o + 608);
        if (p != 0) store<u32>(p + 1268, load<u32>(p + 1268) & ~0x40000000u);
        constexpr u32 g = 0x01164B40u;
        VCall(ctx, o, 136, {o, g});
        VCall(ctx, o, 384, {o, g});
        store<u32>(o + 480, load<u32>(g));
        store<u32>(o + 484, load<u32>(g + 4));
        store<u32>(o + 488, load<u32>(g + 8));
        store<u32>(o + 496, load<u32>(g));
        store<u32>(o + 500, load<u32>(g + 4));
        const u32 gz = load<u32>(g + 8);
        store<u32>(o + 512, 0);
        store<u32>(o + 504, gz);
        store<u32>(o + 516, 0);
        store<u32>(o + 520, 0);
        store<u32>(o + 524, 0);
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(gz));
    }
    SetBit384_36(ctx, o, on);
}

// void f(T* o, u8 on): when bit 31 of o->+384 differs from on: +432 /
// +428 = 0, o->vt[136](o, 0x01164B40) and o->vt[384](o, 0x01164B40), +436
// and +1168..+1208 (9 words) = 0, u8 +1216/+1217 = 0, 0x00248BB0(o; f1 =
// the float at 0x0024DC90). Then SetBit384_36. f1 as left.
DECOMP(0024DC98, Obj_SetActive_0024DC98) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    if ((on & 0xFFu) != (load<u32>(o + 384) >> 31)) {
        constexpr u32 g = 0x01164B40u;
        const f64 K = Lfs32(0x0024DC90u);
        const u32 vt = load<u32>(o);
        store<u32>(o + 432, 0);
        store<u32>(o + 428, 0);
        VCallVt(ctx, vt, 136, {o, g});
        VCall(ctx, o, 384, {o, g});
        store<u32>(o + 436, 0);
        static constexpr u32 kOff[] = {1176, 1172, 1168, 1192, 1188, 1184, 1208, 1204, 1200};
        for (u32 k : kOff) store<u32>(o + k, 0);
        store<u8>(o + 1216, 0);
        store<u8>(o + 1217, 0);
        callf(ctx, 0x00248BB0, {o}, {K});
    }
    SetBit384_36(ctx, o, on);
}

// s32 f(T* o, a, b; f1 = r): -1 when bits 22..23 of o->+68 are clear;
// else the direction v (Listener_LocalDirection(&v, b, a), a stack
// vector) inflated by r is tested against the box o +288..+296 (min) /
// +304..+312 (max): 1 when v + r > min and v - r < max on all three axes
// (the original's order), else 0. f1 as left.
DECOMP(00272F5C, Listener_InBox_00272F5C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 r = farg(ctx, 0);
    if (((load<u32>(o + 68) >> 22) & 3u) == 0) {
        ret(ctx, -1);
        return;
    }
    Stack fr(ctx, 16);
    const u32 v = fr.addr();
    call(ctx, 0x005532E4, {v, b, a});
    auto f = [](f64 x) { return static_cast<f64>(static_cast<f32>(x)); };
    const f64 x = Lfs32(v), y = Lfs32(v + 4), z = Lfs32(v + 8);
    ret(ctx, 0u);
    f64 t = f(x + r);
    ctx.fpr[1] = t;
    if (!(t > Lfs32(o + 288))) return;
    t = f(y + r);
    ctx.fpr[1] = t;
    if (!(t > Lfs32(o + 292))) return;
    ctx.fpr[1] = z;
    if (!(f(z + r) > Lfs32(o + 296))) return;
    if (!(f(x - r) < Lfs32(o + 304))) return;
    if (!(f(y - r) < Lfs32(o + 308))) return;
    t = f(z - r);
    ctx.fpr[1] = t;
    if (!(t < Lfs32(o + 312))) return;
    ret(ctx, 1u);
}

// u8 f(-, s32 key): in the current stage s = (*0x00F64C20)->+36 (0
// without one): the slot byte c = the local player's (*0x00F65CA0 +
// 4056 * u8 [+4208]) byte +498 + 4 * u8 s->+822; the first of s's u8
// +720 536-byte areas (+716) whose u8 +501 is c and whose 12-byte entry
// list (+448: data +0, u16 count +8) holds key gives that entry's u8 +9;
// 0 if none.
DECOMP(002E1C88, Stage_LookupKey_002E1C88) {
    const s32 key = arg<s32>(ctx, 1);
    const u32 s = load<u32>(load<u32>(0x00F64C20u) + 36);
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = load<u32>(0x00F65CA0u);
    const u32 c = load<u8>(w + load<u8>(w + 4208) * 4056u + (static_cast<u32>(load<u8>(s + 822)) << 2) + 498);
    const u32 n = load<u8>(s + 720);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(s + 716) + 536 * i;
        if (load<u8>(e + 501) != c) continue;
        const u32 cnt = load<u16>(e + 448 + 8);
        for (u32 j = 0; j < cnt; ++j) {
            const u32 arr = load<u32>(e + 448);
            if (static_cast<s32>(load<u32>(arr + 12 * j)) == key) {
                ret(ctx, load<u8>(arr + 12 * j + 9));
                return;
            }
        }
    }
    ret(ctx, 0u);
}

// void Delete(T* o): nothing for o = 0; else vtable 0x00B1D550; unless
// bit 0x1000 of u16 +236: with +232 set, +228->vt[8](); then, with +228
// set, +232->vt[8]() (the word as read, 0 when it was clear, as in the
// original), +212->vt[8](); always +216->vt[8](), SmallFree(+248, u16
// +238 * 32), vtable 0x00B1D4D8 and SmallFree(o, 256). r3 as left.
DECOMP(002F23A0, VtB1D550_Delete_002F23A0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1D550u);
    if ((load<u16>(o + 236) & 0x1000u) == 0) {
        u32 q = load<u32>(o + 232);
        if (q != 0) {
            const u32 p = load<u32>(o + 228);
            VCall(ctx, p, 8, {p});
            q = load<u32>(o + 232);
        }
        if (load<u32>(o + 228) != 0) VCall(ctx, q, 8, {q});
        const u32 r = load<u32>(o + 212);
        VCall(ctx, r, 8, {r});
    }
    const u32 t = load<u32>(o + 216);
    VCall(ctx, t, 8, {t});
    call(ctx, 0x00552420, {load<u32>(o + 248), (static_cast<u32>(load<u16>(o + 238)) << 5) & 0xFFFFFFE0u});
    store<u32>(o, 0x00B1D4D8u);
    call(ctx, 0x00552420, {o, 256});
}

// bool f(T* o, const Ev* e): kind e->+0 0: when i = o->vt[32](o, 0) is
// below u16 o->+52, 0x00312274(o, o->+44[i]); true. Kind 2 (time t =
// e->+4): r = o->vt[88](o; f1 = t), o->vt[64](o), an effect o->+40 gets
// Effect_SetTime(it; t); r3 = r. Other kinds: true.
DECOMP(003123AC, Obj_OnEvent_003123AC) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const s32 k = static_cast<s32>(load<u32>(e));
    const f64 t = Lfs32(e + 4);
    if (k == 0) {
        const u32 i = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
        if (i < load<u16>(o + 52)) call(ctx, 0x00312274, {o, load<u32>(load<u32>(o + 44) + (i << 2))});
    } else if (k == 2) {
        ctx.fpr[1] = t;
        const u32 r = static_cast<u32>(VCall(ctx, o, 88, {o}));
        VCall(ctx, o, 64, {o});
        const u32 fx = load<u32>(o + 40);
        if (fx != 0) callf(ctx, 0x004CF710, {fx}, {t});
        ret(ctx, r);
        return;
    }
    ret(ctx, 1u);
}

// bool f(Reader* r): ReadNamePair36 with the buffer 0x0106C060 (flag
// 0x0106C058) and the words 0x0106BFAC / 0x0106BFB0.
DECOMP(0034B594, NamePair_Read_0034B594) {
    ReadNamePair36(ctx, 0x0106C058u, 0x0106C060u, 0x0106BFACu, 0x0106BFB0u);
}

// bool f(Reader* r): ReadNamePair36 with the buffer 0x0106C0F0 (flag
// 0x0106C0E8) and the words 0x0106BFA4 / 0x0106BFA8.
DECOMP(0034B884, NamePair_Read_0034B884) {
    ReadNamePair36(ctx, 0x0106C0E8u, 0x0106C0F0u, 0x0106BFA4u, 0x0106BFA8u);
}

// bool f(Reader* r): ReadNamePair36 with the buffer 0x0106C180 (flag
// 0x0106C178) and the words 0x0106BFB4 / 0x0106BFB8.
DECOMP(0034BB58, NamePair_Read_0034BB58) {
    ReadNamePair36(ctx, 0x0106C178u, 0x0106C180u, 0x0106BFB4u, 0x0106BFB8u);
}
