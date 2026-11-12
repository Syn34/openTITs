// Game code, part 50 (continued): the small-function sweep, 112
// instructions and up. Each was checked against the original in the PPC
// simulator (memory writes, calls and their arguments, r3 and f1).
#include "obj50.h"

using namespace sly4;
using namespace sly4::obj50;

// ---------------------------------------------------------------- 112 instructions
// u32 f(Obj* o): picks o's idle animation: o->400 by default; with the
// key 0x739C20CA on o->64 (Handle_HasKey_00298B44) 0x8797408A, or
// 0x739C20CA when the f32 o->100 is above o->180; with the key 0xBFE11843
// the facing tests (d1 = (o->192, K0, o->200) . o->160..168, d2 = the same
// with o->128..136, K the .text pool 0x000210E0): when d1 < K[1], d2 >
// K[2] gives 0xBFE11843 and d2 < K[3] gives 0x3C985140. A pick other than
// o->32 is restarted (Anim_Restart2775A8(o->64, hash, 0, 0, -1, -, 0, 0,
// slot 10 = 0, slot 11 = -1; f1 = K[4], f2 = K[5], f3 = f4 = K[0])) and
// stored in o->32 when that worked. r3 / f1 as the original leaves them.
DECOMP(000210F8, Obj_PickIdleAnim_000210F8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x000210E0u;
    u32 pick = load<u32>(o + 400);
    if (call(ctx, 0x00298B44u, {load<u32>(o + 64), 0x739C20CAu}) != 0) {
        const f64 a = Lfs48(o + 100);
        ctx.fpr[1] = a;
        pick = a > Lfs48(o + 180) ? 0x739C20CAu : 0x8797408Au;
    }
    const u32 t = static_cast<u32>(call(ctx, 0x00298B44u, {load<u32>(o + 64), 0xBFE11843u}));
    const u32 cur = load<u32>(o + 32);
    const u32 h = load<u32>(o + 64);
    if (t != 0) {
        const f64 k0 = Lfs48(K);
        const f64 x = Lfs48(o + 192), z = Lfs48(o + 200);
        const f64 d1 = Fma49(z, Lfs48(o + 168), Fma49(x, Lfs48(o + 160), Fm49(k0, Lfs48(o + 164))));
        const f64 d2 = Fma49(z, Lfs48(o + 136), Fma49(x, Lfs48(o + 128), Fm49(k0, Lfs48(o + 132))));
        ctx.fpr[1] = d2;
        if (d1 < Lfs48(K + 4)) {
            if (d2 > Lfs48(K + 8)) pick = 0xBFE11843u;
            else if (d2 < Lfs48(K + 12)) pick = 0x3C985140u;
        }
    }
    if (pick == cur) {
        ret(ctx, h);
        return;
    }
    const f64 k0 = Lfs48(K);
    // r8 is left as the callee before left it (Anim_Restart2775A8 does not read it)
    callf(ctx, 0x002775A8u, {h, pick, 0, 0, ~0ull, ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull},
          {Lfs48(K + 16), Lfs48(K + 20), k0, k0});
    if (static_cast<u32>(ctx.gpr[3]) != 0) store<u32>(o + 32, pick);
}

// u32 f(Cmd* c, Fx* fx): sends a timed command to the controller (+244)
// of c's target: the target is Target_Resolve_0009A190(c, c->12, fx) or,
// failing that, the object of the bound effect c->16 (Effect_Bind4BB7E0
// (fx, c->16, 0); +280 when its +276 is 0x02235C33); the parameter is
// c->28, or the non-zero value (vslot 24) of the kind-2 effect component
// c->32. The controller gets vslot 164(x, c->20, c->24, param; f1 =
// c->36) (r7 = the entry, r8 = the OPD, as the original loads them). A
// missing target is reported (Hash_ToString(c->12, else c->16)) unless
// u8 c->40 is set. r3 as the original leaves it.
DECOMP(000866D4, Cmd_SendTimed_000866D4) {
    const u32 c = arg<u32>(ctx, 0), fx = arg<u32>(ctx, 1);
    u32 t = 0;
    if (const u32 h = load<u32>(c + 12); h != 0) t = static_cast<u32>(call(ctx, 0x0009A190u, {c, h, fx}));
    if (t == 0) {
        if (const u32 h2 = load<u32>(c + 16); h2 != 0) {
            const u32 b = static_cast<u32>(call(ctx, 0x004BB7E0u, {fx, h2, 0}));
            if (b != 0 && load<u32>(b + 276) == 0x02235C33u) t = load<u32>(b + 280);
        }
    }
    u32 prm = load<u32>(c + 28);
    u32 r = load<u32>(c + 32);
    if (r != 0) {
        const u32 k = static_cast<u32>(call(ctx, 0x004BB4B4u, {fx, r}));
        r = k;
        if (k != 0) {
            r = static_cast<u32>(VCall(ctx, k, 32, {k}));
            if (r == 2) {
                r = static_cast<u32>(VCall(ctx, k, 24, {k}));
                if (r != 0) prm = r;
            }
        }
    }
    if (t == 0) {
        r = load<u8>(c + 40);
        if (r == 0) {
            r = load<u32>(c + 12);
            if (r == 0) r = load<u32>(c + 16);
            r = static_cast<u32>(call(ctx, 0x0056A6E0u, {r}));
        }
        ret(ctx, r);
        return;
    }
    const u32 x = load<u32>(t + 244);
    if (x == 0) {
        ret(ctx, r);
        return;
    }
    const u32 opd = load<u32>(load<u32>(x) + 164);
    ctx.fpr[1] = Lfs48(c + 36);
    call_ptr(ctx, opd, {x, load<u32>(c + 20), load<u32>(c + 24), prm, load<u32>(opd), opd});
}

// u32 f(Obj* o): destructor body of the vtable 0x00B10D50 object: the
// entries of the PtrVectors in the o->124 groups (28 bytes apart from
// +12) get vslot 8 and their counts are zeroed, as is +124; the same for
// the PtrVector +136 (count +144), which is then cleared with the four
// group vectors (+96, +68, +40, +12); then VtB11B20_DtorTrivial_004BB41C(o).
// r3 = its result.
DECOMP(00098500, ObjB10D50_Dtor_00098500) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B10D50u);
    auto release = [&](u32 v) {
        for (u32 j = 0; j < load<u16>(v + 8); ++j) {
            const u32 p = load<u32>(load<u32>(v) + 4 * j);
            if (p != 0) VCall(ctx, p, 8, {p});
        }
        store<u16>(v + 8, 0);
    };
    if (load<u32>(o + 124) != 0) {
        for (u32 i = 0; i < load<u32>(o + 124); ++i) release(o + 28 * i + 12);
    }
    store<u32>(o + 124, 0);
    release(o + 136);
    ClearVec50(ctx, o + 136);
    for (s32 k = 3; k >= 0; --k) ClearVec50(ctx, o + 28 * static_cast<u32>(k) + 12);
    ret(ctx, call(ctx, 0x004BB41Cu, {o}));
}

// bool f(Obj* o): whether o may start an action: its notify object
// (Obj_Notify26) exists with Obj_Field244Clear true, u8 o->710 has bit
// 0x80 and not 0x40, bit 0x8000 of o->188 is clear, its owner
// (Obj_Owner0D93FC) is not flagged 0x80000000 without 0x10000 in +40, the
// CallK2_000D95C4 object (if any) has u8 +336 clear, and its state pair
// (vslot 32(o, 0) / (o, 1)) is (0, 0), or the first is 3 and the u8 at
// 0x00ACA644 + 20 * vslot 32(o, 1) has bit 0x40.
DECOMP(0012E664, Obj_CanAct_0012E664) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = static_cast<u32>(call(ctx, 0x000D94CCu, {o}));
    const u32 b = static_cast<u32>(call(ctx, 0x000D93FCu, {o}));
    const u32 c = static_cast<u32>(call(ctx, 0x000D95C4u, {o}));
    auto no = [&]() { ret(ctx, 0u); };
    if (a == 0 || call(ctx, 0x0029A40Cu, {a}) == 0) return no();
    const u32 v = load<u8>(o + 710);
    if ((v & 0x80u) == 0 || (v & 0x40u) != 0) return no();
    if ((load<u64>(o + 184) & 0x8000u) != 0) return no();
    if (b != 0) {
        const u32 w = load<u32>(b + 40);
        if ((w & 0x80000000u) != 0 && (w & 0x00010000u) == 0) return no();
    }
    if (c != 0 && load<u8>(c + 336) != 0) return no();
    const u32 s0 = static_cast<u32>(VCall(ctx, o, 32, {o, 0}));
    const u32 s1 = static_cast<u32>(VCall(ctx, o, 32, {o, 1}));
    if (s0 == 0 && s1 == 0) {
        ret(ctx, 1u);
        return;
    }
    if (s0 != 3) return no();
    const u32 i = static_cast<u32>(VCall(ctx, o, 32, {o, 1}));
    ret(ctx, (load<u8>(0x00ACA644u + i * 20) & 0x40u) != 0 ? 1u : 0u);
}

// T* f(a, b, Src* c, d): the default 112-byte def on the stack (+12..+20
// = c->12..20, +4 = 2048, u8 +25 = 1, +36 / +40 / +72 / +84 / +88 = -1.0f,
// +80 = 4, +96 = 3.0f, +100 = 1.0f, the rest 0) read by Def_Read1A7878
// (a, b, c, d, def), then a 192-byte object (SmallAlloc; Thunk_000C2B90
// (m, def); vtable 0x00B15150; +140..+168 = def +76..+104) with five nodes
// +172..+188 = Node_PushF65B7C(name, 0) for the names of the table
// 0x00AD1410. r3 = m (0 on failure).
DECOMP(001A7AB0, Obj15150_New_001A7AB0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    Stack fr(ctx, 112);  // the def at sp+112..223 (frame of 272 bytes)
    const u32 f = fr.addr();
    const u32 c12 = load<u32>(c + 12), c16 = load<u32>(c + 16), c20 = load<u32>(c + 20);
    store<u32>(f + 12, c12);
    store<u32>(f + 16, c16);
    store<u32>(f + 20, c20);
    store<u8>(f + 24, 0);
    for (u32 i = 26; i < 32; ++i) store<u8>(f + i, 0);
    store<u32>(f + 32, 0);
    store<u32>(f + 36, 0xBF800000u);
    store<u32>(f + 40, 0xBF800000u);
    store<u8>(f + 44, 0);
    for (u32 i = 48; i <= 68; i += 4) store<u32>(f + i, 0);
    store<u32>(f + 72, 0xBF800000u);
    store<u32>(f, 0);
    store<u32>(f + 8, 0);
    store<u32>(f + 76, 0);
    store<u32>(f + 80, 4);
    store<u32>(f + 84, 0xBF800000u);
    store<u32>(f + 88, 0xBF800000u);
    store<u32>(f + 92, 0);
    store<u32>(f + 96, 0x40400000u);
    store<u32>(f + 100, 0x3F800000u);
    store<u32>(f + 104, 0);
    store<u32>(f + 4, 2048);
    store<u8>(f + 25, 1);
    // r8..r10 as the stores leave them
    call(ctx, 0x001A7878u, {a, b, c, d, f, 0x40400000u, 4, 0x3F800000u});
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {192}));
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000C2B90u, {m, f});
    Cp50(m + 148, f + 84);
    Cp50(m + 140, f + 76);
    Cp50(m + 152, f + 88);
    Cp50(m + 144, f + 80);
    store<u32>(m, 0x00B15150u);
    Cp50(m + 156, f + 92);
    Cp50(m + 160, f + 96);
    Cp50(m + 168, f + 104);
    Cp50(m + 164, f + 100);
    ctx.fpr[1] = Lfs48(f + 100);  // f1 as the last copy leaves it
    for (u32 i = 0; i < 5; ++i) {
        const u32 n = static_cast<u32>(call(ctx, 0x00446984u, {load<u32>(0x00AD1410u + 4 * i), 0}));
        store<u32>(m + 172 + 4 * i, n);
    }
    ret(ctx, m);
}

// T* f(a, b, Src* x, d): the damage-collision defaults record on the stack
// (Def_Construct74FD80(rec, x->12, x->16, x->20, r7 = the pool 0x001EB2A4,
// r8 = x->20, 0x9260C26E, 0x01164B40; floats from the pool as
// DamageColDef_Read_001EB118 builds them), rec->8 |= 2,
// DamageColDef_Read_001EB118(a, b, x, d, rec), then a 304-byte object
// (SmallAlloc; Thunk_000E27FC(m, rec); vtable 0x00B18458) and the
// record's vector +124 freed. r3 = m (0 on failure).
DECOMP(001EB2B8, DamageCol_New_001EB2B8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001EB2A4u;
    const f64 k0 = Lfs48(K), k4 = Lfs48(K + 4), k8 = Lfs48(K + 8), k12 = Lfs48(K + 12), k16 = Lfs48(K + 16);
    const u64 f3w = load<u32>(K + 12), f2w = load<u32>(K + 16);
    const u32 x20 = load<u32>(x + 20);
    Stack fr(ctx, 256);  // the record at sp+368 (frame of 640 bytes)
    const u32 rec = fr.addr();
    // stack slots 8..18 are not set by the original (the callee's FPR slots)
    callf(ctx, 0x0074FD80u,
          {rec, load<u32>(x + 12), load<u32>(x + 16), x20, K, x20, 0x9260C26Eu, 0x01164B40u, 0, 0, 0, 0, 0, 0, 0, 0, 0,
           0, 0, f3w, f3w, f3w, f3w, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, f2w, f2w, 0},
          {k0, k16, k12, k12, k12, k12, k4, k8, k16, k12, k12, k12, k12});
    store<u32>(rec + 8, load<u32>(rec + 8) | 2u);
    call(ctx, 0x001EB118u, {a, b, x, d, rec});
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {304}));
    u32 r = 0;
    if (m != 0) {
        call(ctx, 0x000E27FCu, {m, rec});
        store<u32>(m, 0x00B18458u);
        r = m;
    }
    FreeVec0_50(ctx, rec + 124);
    ret(ctx, r);
}

// u32 f(Curve* o, ?, XmlCursor* c): reads the floats +0..+12 of o
// (0x3EE830CF, 0x811B15C8, 0x80C35223, 0x5538433A; defaults the current
// values) and, for each child of c flagged 0x80 in u8 +24 whose type +20 is
// 0x0021518F, the float 0x04A257EB (default the .text constant 0x0022D1D8,
// read into a stack word preset to 2.0f) appended to the f32 PtrVector
// +16 (u16 cap +20, count +24, high-water +26). r3 = 0 (o without c);
// f1 as the original leaves it.
DECOMP(0022D1E0, Curve_Read_0022D1E0) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 2);
    if (c == 0) {
        ret(ctx, o);
        return;
    }
    callf(ctx, 0x004F9B20u, {c, 0x3EE830CFu, o}, {Lfs48(o)});
    callf(ctx, 0x004F9B20u, {c, 0x811B15C8u, o + 4}, {Lfs48(o + 4)});
    callf(ctx, 0x004F9B20u, {c, 0x80C35223u, o + 8}, {Lfs48(o + 8)});
    callf(ctx, 0x004F9B20u, {c, 0x5538433Au, o + 12}, {Lfs48(o + 12)});
    u32 ch = static_cast<u32>(call(ctx, 0x004F9140u, {c}));
    const f64 k = Lfs48(0x0022D1D8u);
    Stack fr(ctx, 16);  // the float word at sp+112
    const u32 w = fr.addr();
    while (ch != 0) {
        if ((load<u8>(ch + 24) & 0x80u) != 0 && load<u32>(ch + 20) == 0x0021518Fu) {
            store<u32>(w, 0x40000000u);
            callf(ctx, 0x004F9B20u, {ch, 0x04A257EBu, w}, {k});
            const u32 cap = load<u16>(o + 20);
            if (load<u16>(o + 24) == cap) call(ctx, 0x00551718u, {o + 16, (cap << 1) & 0x1FFFEu});
            const u32 slot = load<u32>(o + 16) + ((static_cast<u32>(load<u16>(o + 24)) << 2) & 0x3FFFCu);
            const u32 n = load<u16>(o + 24) + 1u;
            const u32 hi = load<u16>(o + 26);
            store<u16>(o + 24, static_cast<u16>(n));
            Cp50(slot, w);
            ctx.fpr[1] = Lfs48(w);
            store<u16>(o + 26, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFFu) ? hi : n));
        }
        ch = static_cast<u32>(call(ctx, 0x004F8D74u, {ch}));
    }
    ret(ctx, 0u);
}

// u32 f(Book* o): resolves the text entries of o (when +32 and +36 are
// set): for each group of the table +44 (count +36), each entry of its
// table +12 (count +4) and each id of the entry's +12 list (count +8 >> 4),
// the text Text_Lookup34A398(id) goes to the entry's +16 list; a found
// text gets u8 +40 = bit 8 of the entry's u8 +4; a missing one (id set)
// reports the group, entry and id hashes (Hash_ToString). r3 = o->36.
DECOMP(00287DE0, Book_ResolveTexts_00287DE0) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 32) == 0 || load<u32>(o + 36) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 i = 0;
    do {
        const u32 g = load<u32>(load<u32>(o + 44) + 4 * i);
        for (u32 j = 0; j < load<u32>(g + 4); ++j) {
            const u32 e = load<u32>(load<u32>(g + 12) + 4 * j);
            for (u32 k = 0; k < (load<u32>(e + 8) >> 4); ++k) {
                const u32 off = 4 * k;
                const u32 t = static_cast<u32>(call(ctx, 0x0034A398u, {load<u32>(load<u32>(e + 12) + off)}));
                store<u32>(load<u32>(e + 16) + off, t);
                const u32 l16 = load<u32>(e + 16), l12 = load<u32>(e + 12);
                if (const u32 v = load<u32>(l16 + off); v != 0) store<u8>(v + 40, (load<u8>(e + 4) & 8u) != 0 ? 1 : 0);
                if (load<u32>(l12 + off) != 0 && load<u32>(l16 + off) == 0) {
                    call(ctx, 0x0056A6E0u, {load<u32>(g)});
                    call(ctx, 0x0056A6E0u, {load<u32>(e)});
                    call(ctx, 0x0056A6E0u, {load<u32>(load<u32>(e + 12) + off)});
                }
            }
        }
        ++i;
    } while (i < load<u32>(o + 36));
    ret(ctx, load<u32>(o + 36));
}

// u32 f(Obj* o): destructor body of the vtable 0x00B1C838 object (base
// vtable 0x00B1BE30): the +1244 object gets vslot 40(x, 0) and is
// cleared; the +1284 object, when its bit 0x00040000 of +172 is set,
// vslot 40(y, 1); +1284 = 0, the PtrVector +1288 cleared; o is removed
// from the global pair list 0x01056F1C (u16 count +2, 8-byte entries +8
// with the object second; later entries moved down) and from the list
// 0x01013B5C (List1013B5C_Remove(o)). r3 = its result.
DECOMP(00294540, ObjB1C838_Dtor_00294540) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = load<u32>(o + 1244);
    store<u32>(o, 0x00B1C838u);
    if (x != 0) {
        VCall(ctx, x, 40, {x, 0});
        store<u32>(o + 1244, 0);
    }
    if (const u32 y = load<u32>(o + 1284); y != 0 && (load<u32>(y + 172) & 0x00040000u) != 0) VCall(ctx, y, 40, {y, 1});
    store<u32>(o + 1284, 0);
    ClearVec50(ctx, o + 1288);
    store<u32>(o, 0x00B1BE30u);
    constexpr u32 L = 0x01056F1Cu;
    const u32 n = load<u16>(L + 2);
    const u32 arr = load<u32>(L + 8);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(arr + 8 * i + 4) != o) continue;
        u32 c = n;
        if (static_cast<s32>(n) >= 2 && static_cast<s32>(i) < static_cast<s32>(n - 1)) {
            for (u32 k = i;;) {
                store<u32>(arr + 8 * k, load<u32>(arr + 8 * k + 8));
                store<u32>(arr + 8 * k + 4, load<u32>(arr + 8 * k + 12));
                c = load<u16>(L + 2);
                if (!(static_cast<s32>(++k) < static_cast<s32>(c - 1))) break;
            }
        }
        store<u16>(L + 2, static_cast<u16>(c - 1));
        break;
    }
    ret(ctx, call(ctx, 0x000D6940u, {o}));
}

// Item* f(x, XmlCursor* cur, u8 v): as Item_New_002B06D4 with +12 = 2,
// the final vtable 0x00B1434C and +20 / +24 = 0; then u8 +16 = v and the
// hashes +20 (0x00014042) / +24 (0x00276B9E) read (defaults the current
// values; into address 0 when the allocation failed, as the original).
// r3 = m (0 when the attribute 0x00AD4C0C is missing).
DECOMP(002975F4, Item_New_002975F4) {
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
        store<u32>(m + 12, 2);
        constexpr u32 A = 0x01056F04u;
        const u32 cap = load<u16>(A);
        if (load<u16>(A + 2) == cap) {
            call(ctx, 0x00558F3Cu, {A, (cap << 1) & 0xFFFEu});
            h = load<u32>(m + 4);
        }
        const u32 d = load<u32>(A + 8);
        store<u32>(d + ((static_cast<u32>(load<u16>(A + 2)) << 3) & 0x7FFF8u), h);
        store<u32>(d + (static_cast<u32>(load<u16>(A + 2)) << 3) + 4, m);
        const u32 c = load<u16>(A + 2) + 1u;
        const u32 hi = load<u16>(A + 4);
        const u32 fl = load<u16>(A + 6);
        store<u16>(A + 2, static_cast<u16>(c));
        store<u16>(A + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c & 0xFFFFu) ? hi : c));
        store<u16>(A + 6, static_cast<u16>(fl | 1u));
        store<u32>(m, 0x00B1434Cu);
        store<u32>(m + 20, 0);
        store<u32>(m + 24, 0);
    }
    store<u8>(m + 16, static_cast<u8>(v));
    call(ctx, 0x004FA14Cu, {cur, 0x00014042u, m + 20, load<u32>(m + 20)});
    call(ctx, 0x004FA14Cu, {cur, 0x00276B9Eu, m + 24, load<u32>(m + 24)});
    ret(ctx, m);
}

// u32 f(Shape* o, a, b, const f32* pos, c, d, e, f, [stack] g..m, n;
// f32 r): shape constructor: PtrVector_Construct(o), u16 +6 = 8, +20 = a,
// +24 = b, +32..40 = pos, +48 = c, +52 = d, +64 = r, +68 = -1, flag bits
// 31..23 of +72 = the low bits of e, f and the stack arguments 8..13 and
// 15; then two physics proxies +56 / +60 = Proxy_Create(&desc, &cinfo)
// from a stack cinfo (ProxyCinfo_Construct, +16 = 1, +24 = 16) and a
// zeroed 12-byte desc. r3 = the second proxy.
DECOMP(002997C8, Shape_Init_002997C8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), pos = arg<u32>(ctx, 3),
              c = arg<u32>(ctx, 4), d = arg<u32>(ctx, 5), e = arg<u32>(ctx, 6), f = arg<u32>(ctx, 7);
    const u32 s8 = StkW47(ctx, 116), s9 = StkW47(ctx, 124), s10 = StkW47(ctx, 132), s11 = StkW47(ctx, 140),
              s12 = StkW47(ctx, 148), s13 = StkW47(ctx, 156), s15 = StkW47(ctx, 172);
    const f64 r = ctx.fpr[1];
    call(ctx, 0x005516E0u, {o});
    u32 w = load<u32>(o + 72) & 0x7FFFFFFFu;
    Cp50(o + 32, pos);
    w = (w & ~0x80000000u) | ((e & 1u) << 31);
    store<u16>(o + 6, 8);
    w = (w & ~0x40000000u) | ((f & 1u) << 30);
    store<u32>(o + 20, a);
    w = (w & ~0x20000000u) | ((s8 & 1u) << 29);
    Cp50(o + 36, pos + 4);
    store<u32>(o + 24, b);
    w = (w & ~0x10000000u) | ((s9 & 1u) << 28);
    Cp50(o + 40, pos + 8);
    w = (w & ~0x08000000u) | ((s10 & 1u) << 27);
    store<u32>(o + 48, c);
    w = (w & ~0x04000000u) | ((s11 & 1u) << 26);
    store<u32>(o + 52, d);
    w = (w & ~0x02000000u) | ((s12 & 1u) << 25);
    store<u32>(o + 56, 0);
    store<u32>(o + 60, 0);
    w = (w & ~0x01000000u) | ((s13 & 1u) << 24);
    Stfs32(o + 64, r);
    store<u32>(o + 68, 0xFFFFFFFFu);
    w = (w & ~0x00800000u) | ((s15 & 1u) << 23);
    store<u32>(o + 72, w);
    ctx.fpr[1] = Lfs48(pos + 8);  // f1 as the copies leave it
    Stack fr(ctx, 96);  // desc at sp+112, cinfo at sp+124 (registers saved from sp+208)
    const u32 desc = fr.addr(), ci = desc + 12;
    store<u32>(desc, 0);
    store<u32>(desc + 4, 0);
    store<u32>(desc + 8, 0);
    call(ctx, 0x006C09B4u, {ci});
    store<u32>(ci + 24, 16);
    store<u32>(ci + 16, 1);
    store<u32>(o + 56, static_cast<u32>(call(ctx, 0x006C0FF8u, {desc, ci})));
    const u32 p2 = static_cast<u32>(call(ctx, 0x006C0FF8u, {desc, ci}));
    store<u32>(o + 60, p2);
    ret(ctx, p2);
}

// u32 f(f32 x): sets the global meter value 0x010592EC to x clamped to
// [K[1], the limit 0x010592F0] (K = the .text pool 0x002AB418). When the
// limit is above K[1]: a value that is now ~0 (|v| < K[0]) spawns the
// effect of 0x01059F2C (Effect_Spawn(def, 0, 0, 1); Effect_SetTime(e;
// f1 = K[2]) when spawned), sets bit 0x8000 of the player's (0x00F64A58)
// +244 word when its bits 20..27 are set, and sends 0xCB2E45B1 to the
// sound effect *(0x00F64A6C); otherwise a value leaving ~0 sends
// 0x2BF3BEB4, and one reaching the limit (|v - limit| < K[0]) 0xF56FA63D
// (Effect_SetHash4CF988, when the effect exists). r3 / f1 as the original
// leaves them.
DECOMP(002AB428, Meter_Set_002AB428) {
    constexpr u32 K = 0x002AB418u, G0 = 0x010592ECu, G1 = 0x010592F0u, E = 0x00F64A6Cu;
    f64 x = ctx.fpr[1];
    const f64 eps = Lfs48(K);
    u32 r = std::fabs(Lfs48(G0)) < eps ? 1u : 0u;
    const u32 was = r;
    f64 lim = Lfs48(G1);
    if (lim < x) x = lim;
    const f64 k1 = Lfs48(K + 4);
    if (x < k1) x = k1;
    f64 cur = x;
    Stfs32(G0, x);
    ctx.fpr[1] = x;
    if (!(lim > k1)) {
        ret(ctx, r);
        return;
    }
    const f64 ax = std::fabs(x);
    ctx.fpr[1] = ax;
    if (ax < eps) {
        const u32 s = static_cast<u32>(call(ctx, 0x004A5FC8u, {load<u32>(0x01059F2Cu), 0, 0, 1}));
        if (s != 0) callf(ctx, 0x004CF710u, {s}, {Lfs48(K + 8)});
        if (const u32 p = load<u32>(0x00F64A58u); p != 0) {
            const u32 w = load<u32>(p + 244);
            if (((w >> 20) & 0xFFu) != 0) store<u32>(p + 244, w | 0x8000u);
        }
        r = load<u32>(E);
        if (r != 0) call(ctx, 0x004CF988u, {r, 0xCB2E45B1u});
        else ret(ctx, 0u);
        return;
    }
    if (was != 0) {
        r = load<u32>(E);
        if (r != 0) {
            call(ctx, 0x004CF988u, {r, 0x2BF3BEB4u});
            r = 0x01060000u;  // the reload's address high half, as the original leaves r3
            cur = Lfs48(G0);
            lim = Lfs48(G1);
        }
    }
    const f64 d = std::fabs(Fs49(cur, lim));
    ctx.fpr[1] = d;
    if (d < eps) {
        r = load<u32>(E);
        if (r != 0) {
            call(ctx, 0x004CF988u, {r, 0xF56FA63Du});
            return;
        }
    }
    ret(ctx, r);
}
