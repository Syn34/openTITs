// Game code, part 50: the small-function sweep continued from the remaining
// 110 instruction functions (size order; obj49*.cpp did the ones before).
// Each was checked against the original in the PPC simulator (memory
// writes, calls and their arguments, r3 and f1).
#include "obj50.h"

using namespace sly4;
using namespace sly4::obj50;

// ---------------------------------------------------------------- sweep: 110 instructions
// Effect* f(Obj* o, u32 key, u8 force): looks key up in the map of the
// effect set 0 (Set_Find(0); root +16->+0, nodes key +4, def +8, left +12
// when the node key is above, right +16), recording the path in the set
// (u16 depth +100, nodes at +20 + 4 * i) and the last node visited at +104;
// a hit also stores the node's def in the global 0x011DE288 (an empty map
// clears it and +104). The current effect o->408 is released
// (Effect_Release(e, 0x00010CBB), e->136 |= 0x200, o->408 = 0) when a def
// was found or force is set; a found def then gets a new effect
// (Obj_CreateEffectDCA20(o, def, 0, 1), bit 0x200 of +136 cleared) in
// o->408. r3 = o->408 (0 when the set is missing).
DECOMP(00128C74, Obj_SwitchSetEffect_00128C74) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 force = arg<u32>(ctx, 2);
    const u32 s = static_cast<u32>(call(ctx, 0x004BCE90u, {0}));
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 def = 0;
    u32 n = load<u32>(load<u32>(s + 16));
    if (n == 0) {
        store<u32>(s + 104, 0);
        store<u32>(0x011DE288u, 0);
    } else {
        store<u16>(s + 100, 0);
        for (;;) {
            const u32 cur = n;
            const u32 k = load<u32>(cur + 4);
            if (k == key) {
                store<u32>(s + 104, n);
                store<u32>(0x011DE288u, load<u32>(n + 8));
                if (const u32 h = load<u32>(s + 104); h != 0) def = load<u32>(h + 8);
                break;
            }
            n = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
            if (n == 0) {
                // the original re-tests the last node (never null) and
                // would clear the depth too for a null one
                store<u32>(s + 104, cur);
                break;
            }
            const s32 d = static_cast<s16>(load<u16>(s + 100));
            store<u16>(s + 100, static_cast<u16>(d + 1));
            store<u32>(s + static_cast<u32>(d) * 4 + 20, cur);
        }
    }
    u32 r = load<u32>(o + 408);
    if (r != 0 && (def != 0 || (force & 0xFF) != 0)) {
        call(ctx, 0x004CF3C4u, {r, 0x00010CBBu});
        const u32 e = load<u32>(o + 408);
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        store<u32>(o + 408, 0);
        r = 0;
    }
    if (def == 0) {
        ret(ctx, r);
        return;
    }
    const u32 e = static_cast<u32>(call(ctx, 0x000DCA20u, {o, def, 0, 1}));
    store<u32>(o + 408, e);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
    ret(ctx, load<u32>(o + 408));
}

// T* f(A* a, B* b): a 1920-byte 0x001A7180 object built from a stack query:
// Obj_Init22(q, 0, 0, a->vslot 32(a, b), a, 0, vt 0x00B15190, 0x01164BD0,
// 0x01164B30, 0, 0, 0, 1, 0 x5, the u32 -1; f1 = f2 = -1.0f), q->8 |=
// 0x800, q->144 = -1, b->vslot 8(b, q); m = SmallAlloc(1920) and the ctor
// when that worked; m->vslot 8(m) (on null too, as the original); then q's
// lazily allocated 20-byte PtrVector (+0) is freed. r3 = m.
DECOMP(001A76B0, Obj1A7180_Spawn_001A76B0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f64 k = Lfs48(0x001A76ACu);  // -1.0f
    const u32 v = static_cast<u32>(VCall(ctx, a, 32, {a, b}));
    Stack fr(ctx, 192);  // the query at sp+224..383 (frame of 416 bytes)
    const u32 q = fr.addr();
    callf(ctx, 0x000CDFB0u,
          {q, 0, 0, v, a, 0, 0x00B15190u, 0x01164BD0u, 0x01164B30u, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0xFFFFFFFFu, 0, 0, 0},
          {k, k});
    store<u32>(q + 144, 0xFFFFFFFFu);
    store<u32>(q + 8, load<u32>(q + 8) | 0x800u);
    VCall(ctx, b, 8, {b, q});
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {1920}));
    u32 r = 0;
    if (m != 0) {
        call(ctx, 0x001A7180u, {m, q});
        r = m;
    }
    VCall(ctx, r, 8, {r});
    FreeVec0_50(ctx, q);
    ret(ctx, r);
}

// T* f(A* a, B* b): a 432-byte object (0x000E40B4 ctor, then the vtables
// 0x00B17BB8 / +380 0x00B17D64 / +384 0x00B17D8C) built from a stack
// record: Record_Init_000E44E0(rec, a->vslot 32(a, b), a, 0, 0x01164BD0,
// 0x01164B30, 1, 0, 0, 0, 0, 0), rec->12 |= 4, b->vslot 8(b, rec);
// m = SmallAlloc(432); m->vslot 8(m) (on null too, as the original); then
// the record's teardown (+164, +144, the 20-byte block +0). r3 = m.
DECOMP(001DBA58, ObjB17BB8_Spawn_001DBA58) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, a, 32, {a, b}));
    Stack fr(ctx, 224);  // the record at sp+144..359 (frame of 384 bytes)
    const u32 rec = fr.addr();
    call(ctx, 0x000E44E0u, {rec, v, a, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0, 0});
    store<u32>(rec + 12, load<u32>(rec + 12) | 4u);
    VCall(ctx, b, 8, {b, rec});
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {432}));
    u32 r = 0;
    if (m != 0) {
        call(ctx, 0x000E40B4u, {m, rec});
        store<u32>(m, 0x00B17BB8u);
        store<u32>(m + 380, 0x00B17D64u);
        store<u32>(m + 384, 0x00B17D8Cu);
        r = m;
    }
    VCall(ctx, r, 8, {r});
    ClearVec50(ctx, rec + 164);
    ClearVec50(ctx, rec + 144);
    FreeVec0_50(ctx, rec);
    ret(ctx, r);
}

// void f(Obj* o): state step of a two-object switch (o->576 / o->580 and the
// state o->560, u8 flags +564 / +565). States 0 and 1: the +580 object gets
// Obj_Dispatch362(p, 1, 0, 0); when the +576 object's bit 0x40000000 of
// +396 is clear, its animation is (re)started (Obj_SetAnim(q, 0, 1, -, 0,
// q, 0x01164CA4; f1 = -1.0f)) and +565 = 1; then with both flags set, +564
// is cleared and Player_Set3632C8(q, cmd, 0) is sent for the state read
// again (0: 0x00AD1F28, 1: 0x00AD1F48, 2: 0x00AD1F08). State 2: the mirror
// image (+576 dispatched, +580 animated, no command). Other states do
// nothing. r3 is left as the original leaves it (the last call's result or
// the last word tested).
DECOMP(001E77C8, Obj_SwitchStep_001E77C8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = Lfs48(0x001E77C4u);  // -1.0f
    const s32 st = static_cast<s32>(load<u32>(o + 560));
    if (st > 1) {
        if (st > 2) {
            ret(ctx, static_cast<u32>(st));
            return;
        }
        u32 r = load<u32>(o + 576);
        if (r != 0) r = static_cast<u32>(call(ctx, 0x0036325Cu, {r, 1, 0, 0}));
        const u32 p = load<u32>(o + 580);
        if (p == 0) {
            ret(ctx, r);
            return;
        }
        if ((load<u32>(p + 396) & 0x40000000u) != 0) {
            ret(ctx, 1u);
            return;
        }
        // r6 is left as the caller's (Obj_SetAnim does not read it)
        callf(ctx, 0x00363A3Cu, {p, 0, 1, ctx.gpr[6], 0, p, 0x01164CA4u}, {k});  // r3 = its result
        return;
    }
    if (st < 0) {
        ret(ctx, static_cast<u32>(st));
        return;
    }
    if (const u32 p = load<u32>(o + 580); p != 0) call(ctx, 0x0036325Cu, {p, 1, 0, 0});
    u32 q = load<u32>(o + 576);
    if (q == 0) {
        ret(ctx, 0u);
        return;
    }
    if ((load<u32>(q + 396) & 0x40000000u) == 0) {
        store<u8>(o + 565, 1);
        callf(ctx, 0x00363A3Cu, {q, 0, 1, ctx.gpr[6], 0, q, 0x01164CA4u}, {k});
        q = load<u32>(o + 576);
    }
    ret(ctx, q);
    if (load<u8>(o + 564) == 0 || load<u8>(o + 565) == 0) return;
    store<u8>(o + 564, 0);
    const s32 s2 = static_cast<s32>(load<u32>(o + 560));
    if (s2 == 0) call(ctx, 0x003632C8u, {q, 0x00AD1F28u, 0});
    else if (s2 == 1) call(ctx, 0x003632C8u, {q, 0x00AD1F48u, 0});
    else if (s2 == 2) call(ctx, 0x003632C8u, {q, 0x00AD1F08u, 0});
}

// TriggerFx* f(a, b, Src* c, d): the default 112-byte trigger-effect def on
// the stack (+0 0x80400000, +12 = c->12, names +16 / +20 = 0x00AD366C,
// +32 = 1, floats -1.0f at +36 / +40 / +72, +76 = 0x00AD37CC, u8 +80 = 1,
// the rest 0), read by TriggerFxDef_Read21DEE0(a, b, c, d, def), then a
// 208-byte object (Mem_Alloc(208, 0, 16); Thunk_000C2B90(m, def); vtable
// 0x00B19510 and later 0x00B1A728; u8 +172..175 = def +80..83, the 31-char
// name +140 = def->76, +176 / +180 = def +84 / +88, floats +184..196 =
// def +92..104, +200 = def->108). r3 = m (0 on failure).
DECOMP(0021E23C, TriggerFx_New_0021E23C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    Stack fr(ctx, 112);  // the def at sp+112..223 (frame of 256 bytes)
    const u32 f = fr.addr();
    const u32 c12 = load<u32>(c + 12);
    store<u32>(f + 16, 0x00AD366Cu);
    store<u32>(f + 12, c12);
    store<u32>(f + 20, 0x00AD366Cu);
    for (u32 i = 24; i < 32; ++i) store<u8>(f + i, 0);
    store<u32>(f + 36, 0xBF800000u);
    store<u32>(f + 40, 0xBF800000u);
    store<u8>(f + 44, 0);
    for (u32 i = 48; i <= 68; i += 4) store<u32>(f + i, 0);
    store<u32>(f + 72, 0xBF800000u);
    store<u32>(f + 4, 0);
    store<u32>(f + 8, 0);
    store<u32>(f + 76, 0x00AD37CCu);
    store<u8>(f + 80, 1);
    store<u8>(f + 81, 0);
    store<u8>(f + 82, 0);
    store<u8>(f + 83, 0);
    for (u32 i = 84; i <= 108; i += 4) store<u32>(f + i, 0);
    store<u32>(f, 0x80400000u);
    store<u32>(f + 32, 1);
    // r8..r10 as the stores leave them
    call(ctx, 0x0021DEE0u, {a, b, c, d, f, 0xBF800000u, 1, 0x80400000u});
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {208, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000C2B90u, {m, f});
    store<u8>(m + 172, load<u8>(f + 80));
    store<u8>(m + 173, load<u8>(f + 81));
    store<u32>(m, 0x00B19510u);
    store<u8>(m + 174, load<u8>(f + 82));
    store<u8>(m + 175, load<u8>(f + 83));
    call(ctx, 0x00553638u, {m + 140, load<u32>(f + 76), 31});
    Cp50(m + 184, f + 92);
    Cp50(m + 176, f + 84);
    Cp50(m + 188, f + 96);
    Cp50(m + 180, f + 88);
    store<u32>(m, 0x00B1A728u);
    Cp50(m + 192, f + 100);
    Cp50(m + 200, f + 108);
    Cp50(m + 196, f + 104);
    ctx.fpr[1] = Lfs48(f + 104);  // f1 as the last copy leaves it
    ret(ctx, m);
}

// u32 f(Obj* o): ends the charge of o when it is on (u8 +336): bit 0x80 of
// the target's (+276) +188 word is cleared (as a 64-bit read-modify-write
// of +184); when the target's bit 1 of +188 is set and the level object
// (root +0x1281E0) exists, the pop of layer +280 (Obj_PopLayer(lvl, l;
// f1 = o->144)) is followed by the level's current slot entry (+364 + 4 *
// u8 +345, when +345 is set): if its vslot 8 says 11 and the layer's u8
// +68 is not 2, its vslot 28 is called. Then +336 = 0, +337 = 1, +340 = 0,
// the +296 object (if any) gets vslot 40(x, 0) and +296 / u8 +288 are
// cleared, u8 +316 / +320 and +324 cleared, and
// Obj_ChargeShotFx_002C2420(o, 0). r3 = that result (0 when off).
DECOMP(002C5C34, Obj_ChargeEnd_002C5C34) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 336) == 0) {
        ret(ctx, 0u);
        return;
    }
    {
        const u32 t = load<u32>(o + 276);
        store<u64>(t + 184, load<u64>(t + 184) & ~0x80ull);
    }
    const u32 t = load<u32>(o + 276);
    if (t != 0 && (load<u64>(t + 184) & 1) != 0) {
        const u32 lvl = load<u32>(load<u32>(0x00F65CA0u) + 0x001281E0u);
        const u32 l = load<u32>(o + 280);
        if (lvl != 0 && l != 0) {
            const u32 kind = load<u8>(l + 68) - 2u;
            callf(ctx, 0x000FBB88u, {lvl, l}, {Lfs48(o + 144)});
            const u32 cur = load<u8>(lvl + 345);
            if (cur != 0) {
                const u32 x = load<u32>(lvl + cur * 4 + 364);
                if (x != 0 && static_cast<u32>(VCall(ctx, x, 8, {x})) == 11 && kind != 0) VCall(ctx, x, 28, {x});
            }
        }
    }
    const u32 p = load<u32>(o + 296);
    store<u8>(o + 336, 0);
    store<u8>(o + 337, 1);
    store<u32>(o + 340, 0);
    if (p != 0) {
        VCall(ctx, p, 40, {p, 0});
        store<u32>(o + 296, 0);
        store<u8>(o + 288, 0);  // (0 != 0) as the original computes it
    }
    store<u8>(o + 316, 0);
    store<u8>(o + 320, 0);
    store<u32>(o + 324, 0);
    ret(ctx, call(ctx, 0x002C2420u, {o, 0}));
}

// u32 f(Obj* o, u32 b): constructor of the vtable 0x00B1DD80 state machine
// (StateMachine_ConstructThunk(o, 1, 0, 0); first vtable 0x00B1DCA8):
// +28 = 0, +32 = b, +36 / +40 = 0, a 16-entry PtrVector at +44 (u16 +50 =
// 4 before PtrVector_Init(v, 16, 0, 1)), +64..72 = the vector 0x01164B40,
// +80 = the float 0x01069B38, the 0x004A0DB0 member at +84, u8 +120 = 0,
// Obj_SetEffect40_003044B0(o, 0x83EA2095), then the no-case hashes of the
// six names of the table 0x00AD99A8 (null ones skipped) pushed into +44.
// r3 = the last push's old high-water count (SetEffect40's result when no
// name was pushed).
DECOMP(00304A5C, StateMachineB1DD80_Ctor_00304A5C) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x0045CBE8u, {o, 1, 0, 0});
    store<u32>(o + 28, 0);
    store<u32>(o + 32, b);
    store<u32>(o, 0x00B1DCA8u);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    call(ctx, 0x005516E0u, {o + 44});
    store<u16>(o + 50, 4);
    call(ctx, 0x00551598u, {o + 44, 16, 0, 1});
    Cp50(o + 64, 0x01164B40u);
    store<u32>(o, 0x00B1DD80u);
    Cp50(o + 68, 0x01164B44u);
    Cp50(o + 72, 0x01164B48u);
    Cp50(o + 80, 0x01069B38u);
    ctx.fpr[1] = Lfs48(0x01069B38u);  // f1 as the copy leaves it (for the callees)
    call(ctx, 0x004A0DB0u, {o + 84});
    store<u8>(o + 120, 0);
    u32 r = static_cast<u32>(call(ctx, 0x003044B0u, {o, 0x83EA2095u}));
    for (u32 i = 0; i < 6; ++i) {
        const u32 s = load<u32>(0x00AD99A8u + 4 * i);
        if (s == 0) continue;
        const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
        r = PtrVec_Push50(ctx, o + 44, h);
    }
    ret(ctx, r);
}

// u32 f(Mgr* m): frees m's two pointer tables: each non-null entry of the
// +52 table (u16 count +60; re-read every step) has its +96 object (if
// any) told vslot 40(x, 0) and, when bit 0x00040000 of its +172 is set,
// vslot 40(x, 1), then +96 cleared; its +100 object (if any) gets vslot
// 40(y, 0) and vslot 40(y, 1) and +100 cleared; the 144-byte entry is
// freed. The +72 table's entries (u16 count +80) are freed as 112-byte
// blocks. Both counts are then zeroed. r3 = the +80 count last read.
DECOMP(0030B4AC, Mgr_FreeTables_0030B4AC) {
    const u32 m = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u16>(m + 60); ++i) {
        const u32 e = load<u32>(load<u32>(m + 52) + 4 * i);
        if (e == 0) continue;
        if (const u32 x = load<u32>(e + 96); x != 0) {
            VCall(ctx, x, 40, {x, 0});
            const u32 x2 = load<u32>(e + 96);
            if ((load<u32>(x2 + 172) & 0x00040000u) != 0) VCall(ctx, x2, 40, {x2, 1});
            store<u32>(e + 96, 0);
        }
        if (const u32 y = load<u32>(e + 100); y != 0) {
            VCall(ctx, y, 40, {y, 0});
            const u32 y2 = load<u32>(e + 100);
            VCall(ctx, y2, 40, {y2, 1});
            store<u32>(e + 100, 0);
        }
        call(ctx, 0x00552420u, {e, 144});
    }
    u32 n;
    for (u32 j = 0; j < (n = load<u16>(m + 80)); ++j) {
        const u32 p = load<u32>(load<u32>(m + 72) + 4 * j);
        if (p != 0) call(ctx, 0x00552420u, {p, 112});
    }
    store<u16>(m + 60, 0);
    store<u16>(m + 80, 0);
    ret(ctx, n);
}

// u32 f(Obj* o, f32 t): updates the motions of o's +164 list (u16 count
// +172): each entry runs the updater of its kind (+84: 9 0x0040B3C0, 8
// 0x0040B6F0, 6 0x0040BD30, 4 0x0040C330, 3 Obj_TiltRestart_0040C848, 2
// 0x0040CA90, 1 Obj_SlideTo40CFA0, 0 Obj_RestartMotion_0040D230; (e; f1 =
// t)); an entry (re-read) whose kind is then 12 is dropped: vslot 4(e)
// when it is not null, the count decremented and the last entry moved into
// its slot (the same index is examined again). r3 = the count last read.
DECOMP(00410E98, Obj_UpdateMotions_00410E98) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = ctx.fpr[1];
    u32 i = 0, n;
    while (i < (n = load<u16>(o + 172))) {
        const u32 off = 4 * i;
        u32 e = load<u32>(load<u32>(o + 164) + off);
        u32 fn = 0;
        switch (static_cast<s32>(load<u32>(e + 84))) {
        case 9: fn = 0x0040B3C0u; break;
        case 8: fn = 0x0040B6F0u; break;
        case 6: fn = 0x0040BD30u; break;
        case 4: fn = 0x0040C330u; break;
        case 3: fn = 0x0040C848u; break;
        case 2: fn = 0x0040CA90u; break;
        case 1: fn = 0x0040CFA0u; break;
        case 0: fn = 0x0040D230u; break;
        default: break;
        }
        if (fn != 0) callf(ctx, fn, {e}, {t});
        u32 arr = load<u32>(o + 164);
        e = load<u32>(arr + off);
        if (load<u32>(e + 84) != 12) {
            ++i;
            continue;
        }
        if (e != 0) {
            VCall(ctx, e, 4, {e});
            arr = load<u32>(o + 164);
        }
        const u32 c = load<u16>(o + 172) - 1u;
        store<u16>(o + 172, static_cast<u16>(c));
        if (i == (c & 0xFFFFu)) continue;
        store<u32>(arr + off, load<u32>(arr + ((c << 2) & 0x3FFFCu)));
    }
    ret(ctx, n);
}

// Pushes the 8-byte pair {a, b} onto the u16-counted array l (+0 cap, +2
// count, +4 high-water, +6 flags (bit 1 set by a push), +8 data), doubling
// it (Array_Reserve(l, (cap << 1) & 0xFFFE)) when full; returns the new
// flags word.
static u32 Arr8_Push50(PPUContext& ctx, u32 l, u32 a, u32 b) {
    const u32 cap = load<u16>(l);
    if (load<u16>(l + 2) == cap) call(ctx, 0x00558F3Cu, {l, (cap << 1) & 0xFFFEu});
    const u32 i0 = (static_cast<u32>(load<u16>(l + 2)) << 3) & 0x7FFF8u;
    const u32 d = load<u32>(l + 8);
    const u32 n = load<u16>(l + 2);
    const u32 slot = d + (n << 3);
    store<u32>(d + i0, a);
    store<u16>(l + 2, static_cast<u16>(n + 1));
    store<u32>(slot + 4, b);
    const u32 hi = load<u16>(l + 4);
    const u32 fl = load<u16>(l + 6) | 1u;
    store<u16>(l + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>((n + 1) & 0xFFFFu) ? hi : n + 1));
    store<u16>(l + 6, static_cast<u16>(fl));
    return fl;
}

// u32 f(Obj* o, u32 kind): registers o (when not null, bit 0x00400000 of
// +20 clear and bit 2 of +28 clear) in the global pair array 0x010BB754
// as {o + 56, o}, and, when kind is one of the seven non-zero hashes of
// the table 0x00AE9490 (index i), in the array 0x010BB754 + 24 * i too.
// r3 = the last flags word pushed (7 when kind is not in the table; the
// tested word on the early outs).
DECOMP(00490360, Obj_RegisterKind_00490360) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    if ((load<u32>(o + 20) & 0x00400000u) != 0) {
        ret(ctx, o + 20);
        return;
    }
    if (const u32 f = load<u32>(o + 28) & 2u; f != 0) {
        ret(ctx, f);
        return;
    }
    constexpr u32 A = 0x010BB754u;
    Arr8_Push50(ctx, A, o + 56, o);
    for (u32 i = 0; i < 7; ++i) {
        const u32 t = load<u32>(0x00AE9490u + 4 * i);
        if (k == t && k != 0) {
            ret(ctx, Arr8_Push50(ctx, A + 24 * i, o + 56, o));
            return;
        }
    }
    ret(ctx, 7u);
}

// u32 f(XmlCursor* x): reads the global settings block 0x010C2C28 (if
// any): floats +684 / +688 ("0x00AF022C" / "0x00AF0370", defaults the
// .text pool 0x004EECA4 [0] / [1]), the mode +692 from the attribute
// 0x00AF0380 (0x00AF0390: 1, 0x00AF0398: 0, 0x00AF03A0: 2, compared without
// case), u8 +676 = 1, float +696 (0x00AF03A8, default [0]) and the vector
// +704 (0x00AF03B8, default 0x01164B90), which is then normalized
// ([2] / |v|, single precision). r3 = 1 (0 without the block); f1 = the
// normalized y.
DECOMP(004EECB0, Settings_Read_004EECB0) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x010C2C28u);
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 K = 0x004EECA4u;
    const f64 k0 = Lfs48(K);
    callf(ctx, 0x004FA5C4u, {x, 0x00AF022Cu, g + 684}, {k0});
    callf(ctx, 0x004FA5C4u, {x, 0x00AF0370u, g + 688}, {Lfs48(K + 4)});
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AF0380u}));
    if (call(ctx, 0x00555D70u, {s, 0x00AF0390u}) == 0) store<u32>(g + 692, 1);
    if (call(ctx, 0x00555D70u, {s, 0x00AF0398u}) == 0) store<u32>(g + 692, 0);
    if (call(ctx, 0x00555D70u, {s, 0x00AF03A0u}) == 0) store<u32>(g + 692, 2);
    store<u8>(g + 676, 1);
    callf(ctx, 0x004FA5C4u, {x, 0x00AF03A8u, g + 696}, {k0});
    call(ctx, 0x004FA624u, {x, 0x00AF03B8u, g + 704, 0x01164B90u});
    const f64 vy = Lfs48(g + 708), vx = Lfs48(g + 704), vz = Lfs48(g + 712);
    const f64 l = Sqrt49(Fma49(vz, vz, Fma49(vx, vx, Fm49(vy, vy))));
    const f64 inv = static_cast<f64>(static_cast<f32>(Lfs48(K + 8) / l));
    const f64 ny = Fm49(vy, inv);
    Stfs32(g + 704, Fm49(vx, inv));
    Stfs32(g + 708, ny);
    Stfs32(g + 712, Fm49(vz, inv));
    ctx.fpr[1] = ny;
    ret(ctx, 1u);
}

// Node* f(Builder* b, u32 r4, Src* s): builds the node of s and,
// recursively, its children: c = s->vslot 32(s, r4), c->vslot 8(c); the
// children of s (count +40, table +52 of 16-byte entries: +4 next sibling
// index, +8 source, +12 the next link >> 15; first child: u16 +32, link of
// that entry) are built with f(b, 0, entry->+8), flagged (u8 +32 = 1) and
// appended to c's table (+20, count +28). The walk follows the packed
// (index << 16 | link) words until the count is reached; the packed word
// starts in a stack slot (sp+112) as the original builds it. r3 = c.
DECOMP(004F496C, Node_Build_004F496C) {
    const u32 b = arg<u32>(ctx, 0), r4 = arg<u32>(ctx, 1), s = arg<u32>(ctx, 2);
    const u32 c = static_cast<u32>(VCall(ctx, s, 32, {s, r4}));
    VCall(ctx, c, 8, {c});
    Stack fr(ctx, 16);  // the packed start word at sp+112
    const u32 w = fr.addr();
    const u32 t = s + 12;
    s32 cnt = static_cast<s32>(load<u32>(t + 16));
    if (cnt != 0) {
        const u32 first = load<u32>(t + 8);
        store<u32>(w, 0);
        store<u16>(w, static_cast<u16>(first));
        const u32 l = load<u32>(load<u32>(t + 28) + (first << 4) + 12);
        store<u16>(w + 2, static_cast<u16>(l >> 15));
    } else {
        store<u32>(w, 0);
        store<u16>(w + 2, 0);
    }
    u32 pk = load<u32>(w);
    if (cnt > 0) {
        for (s32 i = 0;;) {
            const u32 cur = pk;
            const u32 off = ((cur >> 16) & 0xFFFFu) << 4;
            const u32 ch = static_cast<u32>(call(ctx, 0x004F496Cu, {b, 0, load<u32>(load<u32>(t + 28) + off + 8)}));
            store<u8>(ch + 32, 1);
            store<u32>(load<u32>(c + 20) + (load<u32>(c + 28) << 2), ch);
            store<u32>(c + 28, load<u32>(c + 28) + 1);
            const u32 last = load<u32>(t + 12);
            cnt = static_cast<s32>(load<u32>(t + 16));
            pk = 0;
            if ((cur >> 16) != last) {
                const u32 tab = load<u32>(t + 28);
                const u32 nx = load<u32>(tab + off + 4);
                pk = (nx << 16) | ((load<u32>(tab + (nx << 4) + 12) >> 15) & 0xFFFFu);
            }
            if (!(++i < cnt)) break;
        }
    }
    ret(ctx, c);
}

// ---------------------------------------------------------------- 111 instructions
// u32 f(Obj* o): o->vslot 220(o, 1); then the animation named in the
// current entry of o's name table (o->224->+0->+12 [index *(0x010AAF8C)
// ->64]) is restarted on o->64 (Anim_Restart2775A8(o->64, hash, 0, 0, 0,
// -1, 0, 0, slot 10 = 0, slot 11 = -1; f1..f4 = the .text pool 0x00023818
// [0], [1], [2], [2])) and o->32 = its hash when that worked; without a
// name the hash 0x002237D4 is used (r7 = -1 then, r8 left as is). When
// o->224 is set, the hash of the entry indexed by *(0x010AAFA4)->64 goes to
// Obj_ApplyAnimHash_00014004 (when non-zero) and Msg_Post(o->48, m, m,
// o->224->4->204) is sent with m = *(0x00F65C54). r3 = Msg_Post's result
// (0 without o->224).
DECOMP(00023828, Obj_RestartNamedAnim_00023828) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 220, {o, 1});
    constexpr u32 K = 0x00023818u;
    const u32 tab = load<u32>(load<u32>(o + 224));
    const u32 g = load<u32>(0x010AAF8Cu);
    const u32 names = load<u32>(tab + 12);
    const u32 s = load<u32>(names + (load<u32>(g + 64) << 2));
    const f64 k0 = Lfs48(K), k1 = Lfs48(K + 4), k2 = Lfs48(K + 8);
    if (s != 0) {
        const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
        callf(ctx, 0x002775A8u, {load<u32>(o + 64), h, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {k0, k1, k2, k2});
        if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, h);
    } else {
        // r8 is left as the caller's (the callee does not read it)
        callf(ctx, 0x002775A8u, {load<u32>(o + 64), 0x002237D4u, 0, 0, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull},
              {k0, k1, k2, k2});
        if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, 0x002237D4u);
    }
    const u32 t = load<u32>(o + 224);
    if (t == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 g2 = load<u32>(0x010AAFA4u);
    const u32 n2 = load<u32>(load<u32>(t) + 12);
    const u32 h2 = static_cast<u32>(call(ctx, 0x0056A710u, {load<u32>(n2 + (load<u32>(g2 + 64) << 2))}));
    if (h2 != 0) call(ctx, 0x00014004u, {o, h2});
    const u32 t2 = load<u32>(o + 224);
    const u32 m = load<u32>(0x00F65C54u);
    ret(ctx, call(ctx, 0x00470660u, {load<u32>(o + 48), m, m, load<u32>(load<u32>(t2 + 4) + 204)}));
}

// u32 f(Pair* p, s32 a, s32 b, s32 c): finds the two objects named by the
// formats 0x00AC02C0 / 0x00AC02D8, both of (a + c + 1, b + 1)
// (Obj_FindById of the no-case hashes) into p->4 / p->0, each
// switched on (vslot 96(x, 1)) when found; when p->4 exists both get
// Obj_SceneEnd0DD02C(x, 0, 0) and vslot 88(x, 0, 1, 1, 1) (p->0 unchecked,
// as the original). p->12 = 4; r3 = 4.
DECOMP(00052080, Pair_FindObjs_00052080) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u64 x = static_cast<u64>(static_cast<s64>(static_cast<s32>(a + c + 1)));
    const u64 y = static_cast<u64>(static_cast<s64>(static_cast<s32>(b + 1)));
    const u32 s1 = static_cast<u32>(call(ctx, 0x005534D0u, {0x00AC02C0u, x, y}));
    const u32 h1 = static_cast<u32>(call(ctx, 0x0056A710u, {s1}));
    const u32 s2 = static_cast<u32>(call(ctx, 0x005534D0u, {0x00AC02D8u, x, y}));
    const u32 h2 = static_cast<u32>(call(ctx, 0x0056A710u, {s2}));
    const u32 o1 = static_cast<u32>(call(ctx, 0x000D6A28u, {h1}));
    store<u32>(p + 4, o1);
    if (o1 != 0) VCall(ctx, o1, 96, {o1, 1});
    const u32 o2 = static_cast<u32>(call(ctx, 0x000D6A28u, {h2}));
    store<u32>(p, o2);
    if (o2 != 0) VCall(ctx, o2, 96, {o2, 1});
    if (load<u32>(p + 4) != 0) {
        call(ctx, 0x000DD02Cu, {load<u32>(p + 4), 0, 0});
        const u32 q1 = load<u32>(p + 4);
        VCall(ctx, q1, 88, {q1, 0, 1, 1, 1});
        call(ctx, 0x000DD02Cu, {load<u32>(p), 0, 0});
        const u32 q2 = load<u32>(p);
        VCall(ctx, q2, 88, {q2, 0, 1, 1, 1});
    }
    store<u32>(p + 12, 4);
    ret(ctx, 4u);
}

// u32 f(Cmd* c, Fx* fx): applies the camera / control command c (u8 flags
// +20 "set", +21 "attach", +22 the argument; target hashes +12 (an effect
// component of fx) and +16 (an object id)) when either flag is set. The
// target is the component's object (vslot 32 == 5, its +28 object with
// +276 == 0x02235C33 gives its +280) or else Obj_FindById(c->16); a missing
// name is reported with Hash_ToString. A target without bit 8 of +192
// reports its +168 hash; otherwise "set" copies +384 into +380, "attach"
// (without "set") calls Obj_AttachControlAI128EE0(t), and the +380 object
// (if any) gets vslot 44(x, c->22). r3 as the original leaves it.
DECOMP(00087E34, CamCmd_Apply_00087E34) {
    const u32 c = arg<u32>(ctx, 0), fx = arg<u32>(ctx, 1);
    u32 r = load<u32>(c + 12);
    if (load<u8>(c + 20) == 0 && load<u8>(c + 21) == 0) {
        ret(ctx, r);
        return;
    }
    u32 t = 0;
    int step = 0;  // 0: report c->12, 1: look c->16 up, 2: target found
    if (r != 0) {
        const u32 k = static_cast<u32>(call(ctx, 0x004BB4B4u, {fx, r}));
        r = load<u32>(c + 12);
        if (k != 0) {
            const u32 v = static_cast<u32>(VCall(ctx, k, 32, {k}));
            r = load<u32>(c + 12);
            if (v == 5) {
                if (const u32 q = load<u32>(k + 28); q != 0) {
                    if (load<u32>(q + 276) == 0x02235C33u) t = load<u32>(q + 280);
                    if (t != 0) {
                        step = 2;
                    } else {
                        call(ctx, 0x0056A6E0u, {r});
                        step = 1;
                    }
                }
            }
        }
    }
    if (step == 0 && r != 0) call(ctx, 0x0056A6E0u, {r});
    if (step <= 1) {
        r = load<u32>(c + 16);
        if (r != 0) {
            t = static_cast<u32>(call(ctx, 0x000D6A28u, {r}));
            r = t;
            if (t == 0) r = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(c + 16)}));
        }
    }
    if (t == 0) {
        ret(ctx, r);
        return;
    }
    if ((load<u32>(t + 192) & 8u) == 0) {
        ret(ctx, call(ctx, 0x0056A6E0u, {load<u32>(t + 168)}));
        return;
    }
    if (load<u8>(c + 20) != 0) {
        r = load<u32>(t + 384);
        store<u32>(t + 380, r);
    } else {
        r = load<u8>(c + 21);
        if (r != 0) r = static_cast<u32>(call(ctx, 0x00128EE0u, {t}));
    }
    const u32 x = load<u32>(t + 380);
    if (x == 0) {
        ret(ctx, r);
        return;
    }
    ret(ctx, VCall(ctx, x, 44, {x, load<u8>(c + 22)}));
}

// u32 f(): loads the boot file 0x00AC5E94 (File_LoadWhole(name, &size))
// into the globals 0x0101278C (data) / 0x01012790 (size), its size in KB
// (rounded up, + 4 / + 1) into 0x01012794 / 0x01012798, requires the
// system modules 50 and 69 (u8 0x00F645A8 = 1), then runs the SDK setup
// imports 0x00ABE744(0x20000, 0x00F9D788) (ok when >= 0 or 0x8002AA02),
// 0x00ABE844(0) (>= 0: u8 0x00F645AB = 1; 0x8002A101 accepted too),
// module 3, 0x00ABE8C4(0x00FBD788, 0x4B000) (u8 0x00F645AC = 1), module
// 4 and 0x00ABE864(0x01008788, 0xA000) (u8 0x00F645AD = 1). The u8
// 0x0101278A = 1 when every step worked, else 0. r3 = 0x01010000.
DECOMP(000AE4A4, Boot_SysInit_000AE4A4) {
    Stack fr(ctx, 16);  // the size word at sp+112
    const u32 w = fr.addr();
    store<u32>(w, 0);
    const u32 buf = static_cast<u32>(call(ctx, 0x00560B90u, {0x00AC5E94u, w}));
    const u32 sz = load<u32>(w);
    const u32 kb = static_cast<u32>(static_cast<s32>(sz + 1023u) / 1024);
    store<u32>(0x01012794u, kb);
    store<u32>(0x01012798u, kb);
    store<u32>(0x0101278Cu, buf);
    store<u32>(0x01012790u, sz);
    store<u32>(0x01012794u, load<u32>(0x01012794u) + 4);
    store<u32>(0x01012798u, load<u32>(0x01012798u) + 1);
    call(ctx, 0x0056C020u, {50});
    store<u8>(0x00F645A8u, 1);
    call(ctx, 0x0056C020u, {69});
    constexpr u32 B = 0x00F9D788u;
    u8 ok = 0;
    s32 r = static_cast<s32>(call(ctx, 0x00ABE744u, {0x00020000u, B}));
    if (r >= 0 || static_cast<u32>(r) == 0x8002AA02u) {
        r = static_cast<s32>(call(ctx, 0x00ABE844u, {0}));
        bool go = true;
        if (r >= 0) store<u8>(0x00F645ABu, 1);
        else go = static_cast<u32>(r) == 0x8002A101u;
        if (go) {
            call(ctx, 0x0056C020u, {3});
            if (static_cast<s32>(call(ctx, 0x00ABE8C4u, {B + 0x00020000u, 0x0004B000u})) >= 0) {
                store<u8>(0x00F645ACu, 1);
                call(ctx, 0x0056C020u, {4});
                if (static_cast<s32>(call(ctx, 0x00ABE864u, {B + 0x0006B000u, 0x0000A000u})) >= 0) {
                    store<u8>(0x00F645ADu, 1);
                    ok = 1;
                }
            }
        }
    }
    store<u8>(0x0101278Au, ok);
    ret(ctx, 0x01010000u);
}

// void f(Tuning* t): copies 28 tuning values (the f32 +72 of the
// variables in the 0x0103xxxx table, two of them other fields) into t
// (+0..+116, +12 / +28 untouched). f1 = the last value copied.
DECOMP(000FD5F0, Tuning_Copy_000FD5F0) {
    const u32 t = arg<u32>(ctx, 0);
    static constexpr u32 kSrc[][2] = {
        {0, 0x01032418u},   {4, 0x010324B0u},   {8, 0x010324FCu},   {16, 0x01032550u},  {20, 0x01032554u},
        {24, 0x01032558u},  {32, 0x010325A8u},  {36, 0x010325F4u},  {40, 0x01032640u},  {44, 0x0103268Cu},
        {48, 0x010326D8u},  {52, 0x010327BCu},  {56, 0x01032808u},  {60, 0x01032724u},  {64, 0x01032770u},
        {68, 0x01032854u},  {72, 0x010328A0u},  {76, 0x010328ECu},  {80, 0x01032938u},  {84, 0x01032984u},
        {88, 0x010329D0u},  {92, 0x01032A1Cu},  {100, 0x01032FE0u}, {104, 0x0103302Cu}, {96, 0x01035410u},
        {108, 0x01033078u}, {112, 0x010330C4u}, {116, 0x01033110u},
    };
    for (const auto& e : kSrc) Cp50(t + e[0], e[1]);
    ctx.fpr[1] = Lfs48(0x01033110u);
}

// Def* f(x, XmlCursor* cur, a, b): a 288-byte def (SmallAlloc; when it
// worked: Wrapper_0CC4D4(m, a, b, 0, 0x01164BD0, 0x01164B30, 1, 0, slots
// 8 / 10 = 0; f1 = the .text pool 0x00167F4C [1]), vtable 0x00B14390, two
// vectors +240 / +256 = 0x01164B40, +272 / +276 = 0, +4 |= 0x80), then
// XmlDef_ReadBase_000CBE10(x, cur, m), the vec3s +240 (0xADDC2CDD) and
// +256 (0x0948D097, then scaled by [0]). A failed allocation reads into
// address 0 (as the original). r3 = m; f1 = the scaled z.
DECOMP(00167F58, Def167F58_New_00167F58) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    constexpr u32 K = 0x00167F4Cu;
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {288}));
    if (m != 0) {
        callf(ctx, 0x000CC4D4u, {m, a, b, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs48(K + 4)});
        store<u32>(m, 0x00B14390u);
        Cp50(m + 240, 0x01164B40u);
        Cp50(m + 244, 0x01164B44u);
        Cp50(m + 248, 0x01164B48u);
        const u32 f4 = load<u32>(m + 4);
        Cp50(m + 256, 0x01164B40u);
        Cp50(m + 260, 0x01164B44u);
        store<u32>(m + 272, 0);
        Cp50(m + 264, 0x01164B48u);
        store<u32>(m + 276, 0);
        store<u32>(m + 4, f4 | 0x80u);
    }
    call(ctx, 0x000CBE10u, {x, cur, m});
    call(ctx, 0x004F9E0Cu, {cur, 0xADDC2CDDu, m + 240, m + 240});
    call(ctx, 0x004F9E0Cu, {cur, 0x0948D097u, m + 256, m + 256});
    const f64 k = Lfs48(K);
    const f64 vx = Lfs48(m + 256), vy = Lfs48(m + 260), vz = Lfs48(m + 264);
    const f64 nz = Fm49(vz, k);
    Stfs32(m + 256, Fm49(vx, k));
    Stfs32(m + 260, Fm49(vy, k));
    Stfs32(m + 264, nz);
    ctx.fpr[1] = nz;
    ret(ctx, m);
}

// T* f(A* a, B* b): a 752-byte 0x001DCBA0 object built from a stack
// record: Record_Init_000E44E0(rec, a->vslot 32(a, b), a, 0, 0x01164BD0,
// 0x01164B30, 1, 0, 0, 0, 0, 0), rec->8 |= 0x40000000, the f32 pair
// rec->208 / 212 = a +632 / +636 (0x01164AFC when a is null), b->vslot
// 8(b, rec); m = SmallAlloc(752) and the ctor when that worked;
// m->vslot 8(m) (on null too, as the original); then the record's
// teardown. r3 = m.
DECOMP(001DD20C, Obj1DCBA0_Spawn_001DD20C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 v = static_cast<u32>(VCall(ctx, a, 32, {a, b}));
    Stack fr(ctx, 224);  // the record at sp+144..359 (frame of 384 bytes)
    const u32 rec = fr.addr();
    call(ctx, 0x000E44E0u, {rec, v, a, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0, 0});
    const u32 r8 = load<u32>(rec + 8);
    const u32 src = a != 0 ? a + 632 : 0x01164AFCu;
    Cp50(rec + 208, src);
    store<u32>(rec + 8, r8 | 0x40000000u);
    Cp50(rec + 212, src + 4);
    ctx.fpr[1] = Lfs48(src + 4);  // f1 as the copy leaves it
    VCall(ctx, b, 8, {b, rec});
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {752}));
    u32 r = 0;
    if (m != 0) {
        call(ctx, 0x001DCBA0u, {m, rec});
        r = m;
    }
    VCall(ctx, r, 8, {r});
    ClearVec50(ctx, rec + 164);
    ClearVec50(ctx, rec + 144);
    FreeVec0_50(ctx, rec);
    ret(ctx, r);
}

// TriggerFx* f(a, b, Src* c, d): as TriggerFx_New_0021E23C with the name
// +76 = 0x00AD3868 and +4 = 1, read by Xml_Load_00221760, a SmallAlloc'd
// 208-byte object whose final vtable is 0x00B1ACF0. r3 = m (0 on failure).
DECOMP(002217E0, TriggerFx_New_002217E0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    Stack fr(ctx, 112);  // the def at sp+112..223 (frame of 256 bytes)
    const u32 f = fr.addr();
    const u32 c12 = load<u32>(c + 12);
    store<u32>(f + 16, 0x00AD366Cu);
    store<u32>(f + 12, c12);
    store<u32>(f + 20, 0x00AD366Cu);
    for (u32 i = 24; i < 32; ++i) store<u8>(f + i, 0);
    store<u32>(f + 36, 0xBF800000u);
    store<u32>(f + 40, 0xBF800000u);
    store<u8>(f + 44, 0);
    for (u32 i = 48; i <= 68; i += 4) store<u32>(f + i, 0);
    store<u32>(f + 72, 0xBF800000u);
    store<u32>(f + 8, 0);
    store<u32>(f + 76, 0x00AD3868u);
    store<u8>(f + 80, 1);
    store<u8>(f + 81, 0);
    store<u8>(f + 82, 0);
    store<u8>(f + 83, 0);
    for (u32 i = 84; i <= 108; i += 4) store<u32>(f + i, 0);
    store<u32>(f, 0x80400000u);
    store<u32>(f + 4, 1);
    store<u32>(f + 32, 1);
    // r8..r10 as the stores leave them
    call(ctx, 0x00221760u, {a, b, c, d, f, 0xBF800000u, 1, 0x80400000u});
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {208}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000C2B90u, {m, f});
    store<u8>(m + 172, load<u8>(f + 80));
    store<u8>(m + 173, load<u8>(f + 81));
    store<u32>(m, 0x00B19510u);
    store<u8>(m + 174, load<u8>(f + 82));
    store<u8>(m + 175, load<u8>(f + 83));
    call(ctx, 0x00553638u, {m + 140, load<u32>(f + 76), 31});
    Cp50(m + 184, f + 92);
    Cp50(m + 176, f + 84);
    store<u32>(m, 0x00B1A728u);
    Cp50(m + 188, f + 96);
    Cp50(m + 180, f + 88);
    Cp50(m + 192, f + 100);
    Cp50(m + 200, f + 108);
    Cp50(m + 196, f + 104);
    store<u32>(m, 0x00B1ACF0u);
    ctx.fpr[1] = Lfs48(f + 104);  // f1 as the last copy leaves it
    ret(ctx, m);
}

// u32 f(Obj* o): when o has a source (+396), copies the source's 4x3
// matrix rows (+240, +256, +272, +288; through a stack copy, as the
// original) into o->96 / 112 / 128 / 144, calls 0x002430C8(o) and
// WorldObj_UpdateLod(o->160, 0, 0) when o->160 is set. r3 as the original
// leaves it (0 without a source).
DECOMP(0024356C, Obj_CopySrcMatrix_0024356C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 396);
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 64);  // the rows at sp+112 / 128 / 144 / 160
    const u32 t = fr.addr();
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 12; k += 4) Cp50(t + 16 * row + k, s + 240 + 16 * row + k);
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 12; k += 4) Cp50(o + 96 + 16 * row + k, t + 16 * row + k);
    call(ctx, 0x002430C8u, {o});
    if (const u32 w = load<u32>(o + 160); w != 0) call(ctx, 0x0057EE00u, {w, 0, 0});
}

// Item* f(x, XmlCursor* cur, u8 v): the 28-byte item named by the
// attribute 0x00AD4C0C of cur (XmlCursor_DocPath(x) and r3 = 0 when it
// is missing): vtable 0x00B0DAAC, +4 = the no-case name hash, +8 = 0, u8
// +16 = 0, +12 = 39, registered as {hash, m} in the global pair array
// 0x01056F04, final vtable 0x00B1CBA8; then u8 +16 = v and the floats +20
// (0x8FA16092) / +24 (0x996B4041) read (into address 0 when the
// allocation failed, as the original). r3 = m.
DECOMP(002B06D4, Item_New_002B06D4) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {cur, 0x00AD4C0Cu}));
    if (s == 0) {
        call(ctx, 0x004F8944u, {x});
        ret(ctx, 0u);
        return;
    }
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {28}));
    if (m != 0) {
        store<u32>(m, 0x00B0DAACu);
        u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
        store<u32>(m + 8, 0);
        store<u8>(m + 16, 0);
        store<u32>(m + 4, h);
        store<u32>(m + 12, 39);
        constexpr u32 A = 0x01056F04u;
        const u32 cap = load<u16>(A);
        if (load<u16>(A + 2) == cap) {
            call(ctx, 0x00558F3Cu, {A, (cap << 1) & 0xFFFEu});
            h = load<u32>(m + 4);
        }
        const u32 n = load<u16>(A + 2);
        const u32 d = load<u32>(A + 8);
        store<u32>(d + ((n << 3) & 0x7FFF8u), h);
        store<u32>(d + (static_cast<u32>(load<u16>(A + 2)) << 3) + 4, m);
        const u32 c = load<u16>(A + 2) + 1u;
        const u32 hi = load<u16>(A + 4);
        const u32 fl = load<u16>(A + 6);
        store<u16>(A + 2, static_cast<u16>(c));
        store<u16>(A + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c & 0xFFFFu) ? hi : c));
        store<u16>(A + 6, static_cast<u16>(fl | 1u));
        store<u32>(m, 0x00B1CBA8u);
    }
    store<u8>(m + 16, static_cast<u8>(v));
    callf(ctx, 0x004F9B20u, {cur, 0x8FA16092u, m + 20}, {Lfs48(m + 20)});
    callf(ctx, 0x004F9B20u, {cur, 0x996B4041u, m + 24}, {Lfs48(m + 24)});
    ret(ctx, m);
}

// u32 f(Obj* o): destructor body of the vtable 0x00B1EF58 object: frees
// the 12-byte blocks of the pair array +2600 (u16 count +2594) and the
// 4-byte blocks of the pair array +2624 (count +2618) (second words, when
// set), clears the PtrVector +2640, releases the two arrays' storage
// through their allocators (+2628 / +2604 ->vslot 8(alloc, data, elem
// size * (u16 cap >> 1)) when allocated) before Array_Destruct of +2616 /
// +2592, then Obj_BaseDtor_003286D8(o). r3 = its result.
DECOMP(0033A308, ObjB1EF58_Dtor_0033A308) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1EF58u);
    for (s32 i = 0; i < static_cast<s32>(load<u16>(o + 2594)); ++i) {
        const u32 p = load<u32>(load<u32>(o + 2600) + 8 * static_cast<u32>(i) + 4);
        if (p != 0) call(ctx, 0x00552420u, {p, 12});
    }
    for (s32 i = 0; i < static_cast<s32>(load<u16>(o + 2618)); ++i) {
        const u32 p = load<u32>(load<u32>(o + 2624) + 8 * static_cast<u32>(i) + 4);
        if (p != 0) call(ctx, 0x00552420u, {p, 4});
    }
    ClearVec50(ctx, o + 2640);
    for (u32 base : {2616u, 2592u}) {
        const u32 sz = load<u16>(o + base) * ((static_cast<u32>(load<u16>(o + base + 6)) >> 1) & 0x7FFFu);
        if (const u32 d = load<u32>(o + base + 8); d != 0) {
            const u32 al = load<u32>(o + base + 12);
            VCall(ctx, al, 8, {al, d, sz});
        }
        call(ctx, 0x0054EC94u, {o + base});
    }
    ret(ctx, call(ctx, 0x003286D8u, {o}));
}

// bool f(Cell* c, Grid* g): advances the timer of c's grid entry (index
// s16 c->6 below u16 g->144, entries of 132 bytes at g+152 whose first
// word is below 10; any other index uses address 0) by the frame time
// (the f32 0x011DE098, the block 0x011DE094 being reset on first use:
// +20 / +24 = 0, bits 0xC0 of u8 +32 cleared, 400 bytes at +92 zeroed,
// the global 0x011972E8 = it, u8 0x011DE280 = 1). The timer is the f32 at
// entry + u16 0x0106CF78 + 4. When it reaches the threshold of c's type
// (the cell table *(0x00F65038)->24, 128-byte records: the record of
// s16 c->4 (0 for -1) + u16 0x0106CF70 + 4 * the u8 at (c->2's record +
// u16 0x0106CF76, or address 0 when its u8 +3 is not positive)) it is
// reset to 0 and r3 = 1; else r3 = 0. f1 = the threshold.
DECOMP(0038E8F0, Cell_TickTimer_0038E8F0) {
    const u32 c = arg<u32>(ctx, 0), g = arg<u32>(ctx, 1);
    const s32 i = static_cast<s16>(load<u16>(c + 6));
    u32 e = 0;
    if (i < static_cast<s32>(load<u16>(g + 144))) {
        e = g + static_cast<u32>(i) * 132 + 152;
        if (static_cast<s32>(load<u32>(e)) >= 10) e = 0;
    }
    const u32 tp = e + load<u16>(0x0106CF78u) + 4;
    f64 t = Lfs48(tp);
    constexpr u32 B = 0x011DE094u;
    if (load<u8>(0x011DE280u) == 0) {
        const u32 b32 = load<u8>(B + 32);
        store<u32>(B + 20, 0);
        store<u32>(B + 24, 0);
        store<u8>(B + 32, static_cast<u8>(b32 & ~0xC0u));
        call(ctx, 0x00726E54u, {B + 92, 0, 400});
        store<u32>(0x011972E8u, B);
        store<u8>(0x011DE280u, 1);
    }
    t = static_cast<f64>(static_cast<f32>(t + Lfs48(B + 4)));
    Stfs32(tp, t);
    const u32 tab = load<u32>(0x00F65038u);
    u32 r = 0;
    if (const s32 k = static_cast<s16>(load<u16>(c + 4)); k != -1) r = load<u32>(tab + 24) + static_cast<u32>(k) * 128;
    u32 q = 0;
    if (const s32 k2 = static_cast<s16>(load<u16>(c + 2)); k2 != -1) q = load<u32>(tab + 24) + static_cast<u32>(k2) * 128;
    u32 qa = 0;
    if (static_cast<s32>(load<u8>(q + 3)) > 0) qa = q + load<u16>(0x0106CF76u);
    const u32 th = r + load<u16>(0x0106CF70u) + (static_cast<u32>(load<u8>(qa)) << 2);
    const f64 lim = Lfs48(th);
    ctx.fpr[1] = lim;
    if (t >= lim) {
        store<u32>(tp, 0);
        ret(ctx, 1u);
    } else {
        ret(ctx, 0u);
    }
}

// u32 f(Obj* o, b, c, d, e): constructor of the vtable 0x00B202F0 state
// machine (StateMachine_ConstructThunk(o, 1, 0, 0)): +92 = b, +96 = c,
// +100 = d, +104 = e, +108..+132 = 0 except +116 = 3, the f32 pairs
// +136 / +144 = 0x01164AFC, u8 +152..+164 = 0 except +161 = 1, +28..+88
// zeroed; then the u16 use count +178 of c, d and e (when set) is bumped.
// r3 = the last count address bumped (0 when d and e are null); f1 as the
// copies leave it.
DECOMP(003AABBC, StateMachineB202F0_Ctor_003AABBC) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3),
              e = arg<u32>(ctx, 4);
    call(ctx, 0x0045CBE8u, {o, 1, 0, 0});
    store<u32>(o + 96, c);
    store<u32>(o + 120, 0);
    store<u32>(o + 124, 0);
    store<u32>(o + 128, 0);
    store<u32>(o + 132, 0);
    store<u32>(o, 0x00B202F0u);
    store<u32>(o + 92, b);
    Cp50(o + 136, 0x01164AFCu);
    store<u32>(o + 100, d);
    store<u32>(o + 104, e);
    store<u32>(o + 108, 0);
    store<u32>(o + 112, 0);
    store<u32>(o + 116, 3);
    Cp50(o + 140, 0x01164B00u);
    store<u8>(o + 152, 0);
    Cp50(o + 144, 0x01164AFCu);
    store<u8>(o + 153, 0);
    Cp50(o + 148, 0x01164B00u);
    for (u32 k = 154; k <= 164; ++k) store<u8>(o + k, k == 161 ? 1 : 0);
    for (u32 k = 28; k <= 88; k += 4) store<u32>(o + k, 0);
    ctx.fpr[1] = Lfs48(0x01164B00u);
    u32 r = 0;
    for (u32 off : {96u, 100u, 104u}) {
        const u32 x = load<u32>(o + off);
        if (x != 0) {
            store<u16>(x + 178, static_cast<u16>(load<u16>(x + 178) + 1));
            r = x + 178;
        } else if (off != 104) {
            r = 0;
        }
    }
    ret(ctx, r);
}

// Slot* f(a, Pool* p, c): takes two records from p's 76-byte record pool
// (p->4: u16 +0 limit, +2 used; records p->12; the optional pointer table
// p->8 gets 0 at the taken index): the first is marked 3 and its slot
// (+4; address 4 when the pool is full) reset (AnimSlot_Reset_004B3C70),
// the second is marked 4, its index stored in the slot's u16 +60 (-1 when
// full), its +4 (u8) / +8..+20 cleared (address 0 when full) and given
// to AnimClip_ReadNames_004B3F28(slot, a, p, c, rec). r3 = the slot.
DECOMP(004B4110, AnimSlot_Take_004B4110) {
    const u32 a = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    auto take = [&]() -> s32 {
        u32 P = load<u32>(p + 4);
        const u32 n = load<u16>(P + 2);
        if (!(static_cast<s32>(n) < static_cast<s32>(load<u16>(P)))) return -1;
        if (const u32 t = load<u32>(p + 8); t != 0) {
            store<u32>(t + ((n << 2) & 0x3FFFCu), 0);
            P = load<u32>(p + 4);
        }
        store<u16>(P + 2, static_cast<u16>(load<u16>(P + 2) + 1));
        return static_cast<s32>(n);
    };
    const s32 i = static_cast<s16>(take());
    u32 e = 0;
    if (i >= 0 && i < static_cast<s32>(load<u16>(load<u32>(p + 4) + 2))) {
        e = load<u32>(p + 12) + static_cast<u32>(i) * 76;
        store<u32>(e, 3);
    }
    const u32 slot = e + 4;
    call(ctx, 0x004B3C70u, {slot});
    const s32 j0 = take();
    store<u16>(slot + 60, static_cast<u16>(j0));
    const s32 j = static_cast<s16>(j0);
    u32 r = 0;
    if (j >= 0 && j < static_cast<s32>(load<u16>(load<u32>(p + 4) + 2))) {
        r = load<u32>(p + 12) + static_cast<u32>(j) * 76;
        store<u32>(r, 4);
    }
    store<u32>(r + 8, 0);
    store<u32>(r + 12, 0);
    store<u32>(r + 16, 0);
    store<u32>(r + 20, 0);
    store<u8>(r + 4, 0);
    call(ctx, 0x004B3F28u, {slot, a, p, c, r});
    ret(ctx, slot);
}

// u32 f(Cmd* c, Fx* fx): float operation on effect components: the
// component c->12 of fx (reported with Hash_ToString when missing or not
// of kind 1 (vslot 32)) has its value (vslot 20, f1) combined with the
// operand (the f32 c->20, or the value of the kind-1 component c->16 when
// there is one) by the u8 op c->24 (0: set, 1: add, 2: sub, 3: mul, 4:
// div, skipped when c->20 equals the .text constant 0x004CAAAC); the result
// goes to its f32 +20. r3 / f1 as the original leaves them.
DECOMP(004CAAB0, FxCmd_FloatOp_004CAAB0) {
    const u32 c = arg<u32>(ctx, 0), fx = arg<u32>(ctx, 1);
    const u32 k = static_cast<u32>(call(ctx, 0x004BB4B4u, {fx, load<u32>(c + 12)}));
    if (k == 0 || static_cast<u32>(VCall(ctx, k, 32, {k})) != 1) {
        call(ctx, 0x0056A6E0u, {load<u32>(c + 12)});
        return;
    }
    VCall(ctx, k, 20, {k});
    const f64 a = ctx.fpr[1];
    f64 b = Lfs48(c + 20);
    if (const u32 h = load<u32>(c + 16); h != 0) {
        const u32 k2 = static_cast<u32>(call(ctx, 0x004BB4B4u, {fx, h}));
        if (k2 != 0 && static_cast<u32>(VCall(ctx, k2, 32, {k2})) == 1) {
            VCall(ctx, k2, 20, {k2});
            b = ctx.fpr[1];
        }
    }
    const u32 op = load<u8>(c + 24);
    ret(ctx, op);
    f64 r;
    switch (op) {
    case 4: {
        constexpr u32 K = 0x004CAAACu;
        ret(ctx, K);
        ctx.fpr[1] = Lfs48(c + 20);
        if (ctx.fpr[1] == Lfs48(K)) return;
        r = static_cast<f64>(static_cast<f32>(a / b));
        break;
    }
    case 3: r = Fm49(a, b); break;
    case 2: r = Fs49(a, b); break;
    case 1: r = static_cast<f64>(static_cast<f32>(a + b)); break;
    case 0:
        Stfs32(k + 20, b);
        return;
    default: return;
    }
    Stfs32(k + 20, r);
    ctx.fpr[1] = r;
}

// u32 f(XmlCursor* x): adds a name to the global settings block
// 0x010C2C28 (unless missing or locked by u8 +587, r3 = 0 / 1): the
// attribute 0x00AF0120 is copied (Mem_Alloc(len + 1, 0, 16), String_Assign,
// terminated) and pushed into the PtrVector +336, and the hash attribute
// 0x00AF0220 (default 0, read into a stack word) into the PtrVector +356.
// r3 = 1 (0 without the block or the attribute).
DECOMP(004EE164, Settings_AddName_004EE164) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x010C2C28u);
    if (g == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(g + 587) != 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AF0120u}));
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 len = static_cast<u32>(call(ctx, 0x00555FA4u, {s}));
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {len + 1, 0, 16}));
    call(ctx, 0x00555F80u, {m, s});
    const u32 n = static_cast<u32>(call(ctx, 0x00555FA4u, {s}));
    store<u8>(m + n, 0);
    PtrVec_PushNm50(ctx, g + 336, m);
    Stack fr(ctx, 16);  // the hash word at sp+112
    const u32 w = fr.addr();
    call(ctx, 0x004FA6E4u, {x, 0x00AF0220u, w, 0});
    PtrVec_PushNm50(ctx, g + 356, load<u32>(w));
    ret(ctx, 1u);
}

// u32 f(Binder* o): clears o's output table (o->12, 4 * src->8->48
// bytes, src = o->16), walks src's children (count +64, table +76 of
// 16-byte entries: +4 next index, +8 node, +12 next link >> 15; first:
// +56; last index +60, the packed start word built in a stack slot as the
// original does) calling node->vslot 8(node, table), then sets the first
// (u16 src->8->16) words of o->24 to 1. r3 = 4 * that count.
DECOMP(004F7A84, Binder_Bind_004F7A84) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00726E54u, {load<u32>(o + 12), 0, load<u32>(load<u32>(load<u32>(o + 16) + 8) + 48) << 2});
    const u32 s = load<u32>(o + 16);
    const u32 out = load<u32>(o + 12);
    Stack fr(ctx, 16);  // the packed start word at sp+112
    const u32 w = fr.addr();
    s32 cnt = static_cast<s32>(load<u32>(s + 64));
    if (cnt != 0) {
        const u32 first = load<u32>(s + 56);
        store<u32>(w, 0);
        store<u16>(w, static_cast<u16>(first));
        store<u16>(w + 2, static_cast<u16>(load<u32>(load<u32>(s + 76) + (first << 4) + 12) >> 15));
    } else {
        store<u32>(w, 0);
        store<u16>(w + 2, 0);
    }
    u32 pk = load<u32>(w);
    if (cnt > 0) {
        for (s32 i = 0;;) {
            const u32 cur = pk;
            const u32 off = ((cur >> 16) & 0xFFFFu) << 4;
            const u32 node = load<u32>(load<u32>(s + 76) + off + 8);
            VCall(ctx, node, 8, {node, out});
            const u32 last = load<u32>(s + 60);
            cnt = static_cast<s32>(load<u32>(s + 64));
            pk = 0;
            if ((cur >> 16) != last) {
                const u32 tab = load<u32>(s + 76);
                const u32 nx = load<u32>(tab + off + 4);
                pk = (nx << 16) | ((load<u32>(tab + (nx << 4) + 12) >> 15) & 0xFFFFu);
            }
            if (!(++i < cnt)) break;
        }
    }
    u32 j = 0;
    for (; j < load<u16>(load<u32>(load<u32>(o + 16) + 8) + 16); ++j) store<u32>(load<u32>(o + 24) + 4 * j, 1);
    ret(ctx, 4 * j);
}

// Node* f(s16 id): a 24-byte node (Mem_Alloc(24, 0, 16); Node_Init(m, id,
// 0x00B1FC96)) whose cell record (table *(0x00F65038)->24, s16 +16;
// address 0 for -1) gets the u16 16, and five bindings on m->12
// (Node_Bind(m->12, name, type, 1, -1, 0): 0x00B1FCA8 / AD / B2 type 3,
// 0x00B1FCBB type 5 (flag 0), 0x00B1FCC0 type 4); each binding's +76 = 0
// (1 for the last) and its u8 +64 goes to the bytes m->20 + u16
// 0x0106CF82 + 0..4. r3 = m (0 on failure). The argument is spilled to
// sp+112 as the original does.
DECOMP(00776EB4, Node_New5_00776EB4) {
    Stack fr(ctx, 16);  // the spill slot at sp+112
    store<u64>(fr.addr(), ctx.gpr[3]);
    const u64 id = load<u64>(fr.addr());
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x00391694u, {m, static_cast<u64>(static_cast<s64>(static_cast<s16>(id))), 0x00B1FC96u});
    const s32 k = static_cast<s16>(load<u16>(m + 16));
    const u32 tab = load<u32>(0x00F65038u);
    u32 rec = 0;
    if (k != -1) rec = load<u32>(tab + 24) + static_cast<u32>(k) * 128;
    store<u16>(rec, 16);
    const u32 dst = load<u32>(m + 20) + load<u16>(0x0106CF82u);
    static constexpr u32 kB[5][3] = {
        {0x00B1FCA8u, 3, 1}, {0x00B1FCADu, 3, 1}, {0x00B1FCB2u, 3, 1}, {0x00B1FCBBu, 5, 0}, {0x00B1FCC0u, 4, 1}};
    for (u32 i = 0; i < 5; ++i) {
        const u32 b = static_cast<u32>(call(ctx, 0x004D3088u, {load<u32>(m + 12), kB[i][0], kB[i][1], kB[i][2], ~0ull, 0}));
        const u32 v = load<u32>(b + 64);
        store<u32>(b + 76, i == 4 ? 1 : 0);
        store<u8>(dst + i, static_cast<u8>(v));
    }
    ret(ctx, m);
}
