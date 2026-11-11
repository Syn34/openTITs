// Game code, part 35: the small-function sweep continued from the last 69
// instruction functions (size order; obj34*.cpp did 65-69). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj35.h"

using namespace sly4;
using namespace sly4::obj35;

// ---------------------------------------------------------------- sweep: 69 instructions
// T* New(-, Xml* x): a 24-byte record (Mem_Alloc(24, 0, 16); 0 on
// failure) built by AnimCmd_ConstructBase(m), +12 / +16 cleared, vtable 0x00B10738, u8 +20 cleared; the
// hash 0xD782B635 to +12 (default 0), the s32 0x3683141C to +16 (current
// default; when the read returns 0 and the value is positive it is
// decremented), then the bool 0x00014042 to +20 (default: the byte read
// before the decrement). r3 = m.
DECOMP(0008CCB8, VtB10738_NewXml_0008CCB8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {24, 0, 16}));
    if (m != 0) {
        call(ctx, 0x004B9CB8, {m});
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m, 0x00B10738u);
        store<u8>(m + 20, 0);
        RdHash34(ctx, x, 0xD782B635u, m + 12, 0);
        const u32 ok = static_cast<u32>(call(ctx, kXmlReadS32_35, {x, 0x3683141Cu, m + 16, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(m + 16))))}));
        const u32 b = load<u8>(m + 20);
        if (ok == 0 && static_cast<s32>(load<u32>(m + 16)) > 0) store<u32>(m + 16, load<u32>(m + 16) - 1u);
        call(ctx, kXmlReadBool35, {x, 0x00014042u, m + 20, b});
    }
    ctx.gpr[3] = m;
}

// T* New(-, Xml* x): XmlHHBB35 with vtable 0x00B10850, the hashes
// 0x7A30D7E3 / 0x7F247172 and the bools 0xF395ADCC / 0x1E389259.
DECOMP(0008FB8C, VtB10850_NewXml_0008FB8C) {
    XmlHHBB35(ctx, 0x00B10850u, 0x7A30D7E3u, 0x7F247172u, 0xF395ADCCu, 0x1E389259u);
}

// T* New(-, Xml* x): XmlHHBB35 with vtable 0x00B108F0, the hashes
// 0x8A93973F / 0x7A30D7E3 and the bools 0x7A281983 / 0x6334AE6F.
DECOMP(00090738, VtB108F0_NewXml_00090738) {
    XmlHHBB35(ctx, 0x00B108F0u, 0x8A93973Fu, 0x7A30D7E3u, 0x7A281983u, 0x6334AE6Fu);
}

// void f(T* o, X* x): the target: when o->+12 names a component of x
// (Effect_FindComponent(x, +12)) whose vt[32]() is 5 and whose +28 owner
// has the class hash 0x02235C33 at +276, that owner's +280; otherwise
// (no such target) Obj_FindById(o->+16) when +16 is set. A target then
// goes through 0x000D94F4 and, when that gives one, 0x002973B8(it, o->+20,
// o->+24). r3 as left (0 when nothing was found).
DECOMP(00090FE0, Effect_ApplyToTarget_00090FE0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    bool found = false;
    const u32 id = load<u32>(o + 12);
    if (id != 0) {
        const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {x, id}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 p = load<u32>(c + 28);
            if (p != 0) {
                if (load<u32>(p + 276) == 0x02235C33u) t = load<u32>(p + 280);
                found = t != 0;
            }
        }
    }
    if (!found) {
        const u32 k = load<u32>(o + 16);
        ctx.gpr[3] = k;
        if (k == 0) return;
        t = static_cast<u32>(call(ctx, 0x000D6A28, {k}));
        if (t == 0) return;
    }
    t = static_cast<u32>(call(ctx, 0x000D94F4, {t}));
    if (t == 0) return;
    call(ctx, 0x002973B8, {t, load<u32>(o + 20), load<u32>(o + 24)});
}

// T* New(-, Xml* x): XmlHHHH35 with vtable 0x00B10B98 and the hashes
// 0x8A93973F / 0x7A30D7E3 / 0x03D1FCFC / 0x044DDD31.
DECOMP(00096478, VtB10B98_NewXml_00096478) {
    XmlHHHH35(ctx, 0x00B10B98u, 0x8A93973Fu, 0x7A30D7E3u, 0x03D1FCFCu, 0x044DDD31u);
}

// T* New(-, Xml* x): NewXml34 (28 bytes): u8 +12 = 1, vtable 0x00B10F10,
// +20 / +24 cleared (+16 is left to the read); the bool 0xCBF1C4F3 to +12
// (default 1), the hash 0xEF44A7B5 to +16 (default 0), the f32 0x21CA4030
// to +24 (current default); a set +16 becomes the sound Sound_Find(+16) in
// +20.
DECOMP(0009A3E4, VtB10F10_NewXml_0009A3E4) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        store<u8>(m + 12, 1);
        store<u32>(m, 0x00B10F10u);
        store<u32>(m + 20, 0);
        store<u32>(m + 24, 0);
        call(ctx, kXmlReadBool35, {x, 0xCBF1C4F3u, m + 12, 1});
        RdHash34(ctx, x, 0xEF44A7B5u, m + 16, 0);
        RdF32Cur34(ctx, x, 0x21CA4030u, m + 24);
        const u32 h = load<u32>(m + 16);
        if (h != 0) store<u32>(m + 20, static_cast<u32>(call(ctx, 0x0044C43C, {h})));
    });
}

// T* New(-, Xml* x): XmlHHBB35 with vtable 0x00B11190, the hashes
// 0x7A30D7E3 / 0x04B8CC71 and the bools 0xF5897C08 / 0x466FBEF5.
DECOMP(0009CC08, VtB11190_NewXml_0009CC08) {
    XmlHHBB35(ctx, 0x00B11190u, 0x7A30D7E3u, 0x04B8CC71u, 0xF5897C08u, 0x466FBEF5u);
}

// T* New(-, Xml* x): XmlHHBB35 with vtable 0x00B11940, the hashes
// 0x0024728B / 0x00255BF3 and the bools 0x469D7C52 / 0xF5897C08.
DECOMP(000A583C, VtB11940_NewXml_000A583C) {
    XmlHHBB35(ctx, 0x00B11940u, 0x0024728Bu, 0x00255BF3u, 0x469D7C52u, 0xF5897C08u);
}

// void f(T* o, const Mat43* m): unless m is nearly o's matrix +96
// (Matrix_NearlyEqual(m, o + 96; f1 = the tolerance at 0x001280F4)), the
// 3x3 rows and the position of m (16-byte rows) copied to o +96..+152,
// then o->vt[64](o); a result r gets 0x00261304(r, m), and the attached
// o->+244 (when set) CopyMat43_00012DE0(it, m). r3 / f1 as left.
DECOMP(001280F8, Obj_SetMatrix_001280F8) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    callf(ctx, 0x005545B4, {m, o + 96}, {Lfs32(0x001280F4u)});
    if (static_cast<u32>(ctx.gpr[3]) != 0) return;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 96 + 16 * r + 4 * k, load<u32>(m + 16 * r + 4 * k));
    ctx.fpr[1] = Lfs32(m + 56);
    const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (r != 0) call(ctx, 0x00261304, {r, m});
    const u32 p = load<u32>(o + 244);
    if (p != 0) call(ctx, 0x00012DE0, {p, m});
}

// void f(T* o, f32 d): the f32 o->+348 kept at max(K, min(v, o->+344))
// (K the constant at 0x0018E8E0): with o->+1708 == 1 v is the partner
// o->+1704's +348 after partner->vt[316](partner) (nothing without a
// partner), else v = o->+348 + d. r3 = 0x0018E8E0 (0 without a partner),
// f1 as the original leaves it.
DECOMP(0018E8E8, Obj_ClampF348_0018E8E8) {
    const u32 o = arg<u32>(ctx, 0);
    f64 v;
    if (load<u32>(o + 1708) == 1) {
        const u32 p = load<u32>(o + 1704);
        ctx.gpr[3] = p;
        if (p == 0) return;
        VCall(ctx, p, 316, {p});
        v = Lfs32(load<u32>(o + 1704) + 348);
    } else {
        v = static_cast<f32>(Lfs32(o + 348) + farg(ctx, 0));
    }
    const f64 a = Lfs32(o + 344);
    const f64 mn = v < a ? v : a;
    const f64 k = Lfs32(0x0018E8E0u);
    ctx.gpr[3] = 0x0018E8E0u;
    if (mn > k) {
        Stfs32(o + 348, mn);
        ctx.fpr[1] = mn;
    } else {
        Stfs32(o + 348, k);
        ctx.fpr[1] = a;
    }
}

// bool f(T* o, U* u; f32 a, f32 b): when o's +552 lacks 0x20000000 and
// (b < K or a >= b; K the constant at 0x001C438C): o->vt[420](o, 3, 0),
// then 0x001C3EF0(o, 0) when u's u8 +445 is set and o's u8 +604 has 0x80;
// r3 = 0. Otherwise o->vt[420](o, 1, 1) when u's u8 +444 is set and o's
// u8 +604 lacks 0x20; r3 = 1.
DECOMP(001C4390, Obj_RangeSwitch_001C4390) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    if ((load<u32>(o + 552) & 0x20000000u) == 0 && (b < Lfs32(0x001C438Cu) || a >= b)) {
        VCall(ctx, o, 420, {o, 3, 0});
        if (load<u8>(u + 445) != 0 && (load<u8>(o + 604) & 0x80u) != 0) call(ctx, 0x001C3EF0, {o, 0});
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u8>(u + 444) != 0 && (load<u8>(o + 604) & 0x20u) == 0) VCall(ctx, o, 420, {o, 1, 1});
    ctx.gpr[3] = 1;
}

// bool f(T* o): 0 when 0x001AEA50(o) fails (it reads the caller's f1,
// passed through); else, when o's +172 has 0x200, for each of the 6 slots
// i whose flag byte (o->+216)->+481 + 96 i is set and whose object q =
// o->+560 + 16 i is set: 0x001C8A74(&tmp, o, i) and q->vt[84](q, &tmp)
// (tmp a 64-byte stack block). r3 = 1.
DECOMP(001C9A28, Obj_SlotsNotify_001C9A28) {
    const u32 o = arg<u32>(ctx, 0);
    callf(ctx, 0x001AEA50, {o}, {farg(ctx, 0)});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 d = load<u32>(o + 216);
    if ((load<u32>(o + 172) & 0x200u) != 0) {
        Stack frame(ctx, 64);
        const u32 tmp = frame.addr();  // sp+112
        for (u32 i = 0; i < 6; ++i) {
            if (load<u8>(d + 481 + 96 * i) == 0) continue;
            const u32 q = load<u32>(o + 560 + 16 * i);
            if (q == 0) continue;
            call(ctx, 0x001C8A74, {tmp, o, i});
            VCall(ctx, q, 84, {q, tmp});
        }
    }
    ctx.gpr[3] = 1;
}

// void f(T* o, bool full): with full, o is taken off the world list
// (World_ListRemove_0046B7A4(*0x00F65CA0, 3, 0x00F33C18, o, 0)), its
// references +584 / +588 and the two triples +592.. / +604.. released
// (RefNode_Release, interleaved), and the effect +552 (when set)
// destroyed (Effect_Destruct, SmallFree 140) and cleared; then always
// Obj_SoundOffBase(o, full). r3 as left.
DECOMP(001E6FA0, Obj_SoundOff_001E6FA0) {
    const u32 o = arg<u32>(ctx, 0), full = arg<u32>(ctx, 1) & 0xFFu;
    if (full != 0) {
        call(ctx, 0x0046B7A4, {load<u32>(0x00F65CA0u), 3, 0x00F33C18u, o, 0});
        call(ctx, 0x004432DC, {load<u32>(o + 584)});
        call(ctx, 0x004432DC, {load<u32>(o + 588)});
        for (u32 i = 0; i < 3; ++i) {
            call(ctx, 0x004432DC, {load<u32>(o + 592 + 4 * i)});
            call(ctx, 0x004432DC, {load<u32>(o + 604 + 4 * i)});
        }
        const u32 e = load<u32>(o + 552);
        if (e != 0) {
            call(ctx, 0x004C8E80, {e});
            call(ctx, 0x00552420, {e, 140});
            store<u32>(o + 552, 0);
        }
    }
    call(ctx, 0x000E4544, {o, full});
}

// T* f(a, Xml* x, P* p, Def* d): a target def read into a stack record
// with its defaults (Def_ReadTarget_002220C8(a, x, p, d, &rec); +12 =
// p->+12, vtables 0x00AD366C / 0x00AD3894, -1.0 / 15.0 floats), then a
// 192-byte object built from it (SmallAlloc; 0x00221F3C(obj, &rec)). r3 =
// the object or 0.
DECOMP(002221D8, Def_NewTarget_002221D8) {
    const u32 a = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    Stack frame(ctx, 104);
    const u32 r = frame.addr();  // sp+112
    const u32 v1 = load<u32>(p + 12);
    store<u32>(r + 16, 0x00AD366Cu);
    store<u32>(r + 12, v1);
    store<u32>(r + 20, 0x00AD366Cu);
    for (u32 k = 24; k < 32; ++k) store<u8>(r + k, 0);
    store<u32>(r + 32, 0);
    store<u32>(r + 36, 0xBF800000u);  // -1.0f
    store<u32>(r + 40, 0xBF800000u);
    store<u8>(r + 44, 0);
    for (u32 k = 48; k < 72; k += 4) store<u32>(r + k, 0);
    store<u32>(r + 72, 0xBF800000u);
    store<u32>(r + 4, 0);
    store<u32>(r + 8, 0);
    store<u32>(r + 76, 0x00AD3894u);
    store<u8>(r + 80, 1);
    store<u8>(r + 81, 0);
    store<u8>(r + 82, 0);
    store<u8>(r + 83, 0);
    store<u32>(r + 84, 0);
    store<u32>(r + 88, 0x41700000u);  // 15.0f
    store<u32>(r, 0x20400000u);
    call(ctx, 0x002220C8, {a, x, p, d, r});
    const u32 o = static_cast<u32>(call(ctx, 0x0055238C, {192}));
    if (o != 0) call(ctx, 0x00221F3C, {o, r});
    ctx.gpr[3] = o;
}

// Vec3* f(Vec3* out, T* t): out = the default 0x01164B40, then, when
// t->+64 < the u16 count +32 (signed), the position of the first free
// 144-byte entry of t->+24 (+48 == 0; index -1 when there is none) is
// built: 0x000CEA48(&m, t->+8, (s32)e->+8, e->+12, &v, e + 32) with v =
// {e->+16..+24, 0}, and out = m's row +48. r3 as left (out when skipped),
// f1 as left.
DECOMP(002B92F8, Slot_FreePos_002B92F8) {
    const u32 out = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    store<u32>(out, load<u32>(0x01164B40u));
    const s32 c = static_cast<s32>(load<u32>(t + 64));  // read between the stores, as the original
    store<u32>(out + 4, load<u32>(0x01164B44u));
    store<u32>(out + 8, load<u32>(0x01164B48u));
    ctx.fpr[1] = Lfs32(0x01164B48u);
    const s32 n = static_cast<s32>(load<u16>(t + 32));
    if (!(c < n)) return;
    const u32 arr = load<u32>(t + 24);
    s32 idx = -1;
    for (s32 i = 0; i < n; ++i) {
        if (load<u32>(arr + 144u * static_cast<u32>(i) + 48) == 0) {
            idx = i;
            break;
        }
    }
    const u32 e = arr + 144u * static_cast<u32>(idx);
    Stack frame(ctx, 80);
    const u32 v = frame.addr(), m = v + 16;  // sp+112, sp+128
    const u32 e8 = load<u32>(e + 8), e12 = load<u32>(e + 12);
    store<u32>(v, load<u32>(e + 16));
    store<u32>(v + 4, load<u32>(e + 20));
    store<u32>(v + 8, load<u32>(e + 24));
    store<u32>(v + 12, 0);
    ctx.fpr[1] = Lfs32(e + 16);
    ctx.fpr[2] = Lfs32(e + 20);
    ctx.fpr[3] = Lfs32(e + 24);
    call(ctx, 0x000CEA48, {m, load<u32>(t + 8), static_cast<u64>(static_cast<s64>(static_cast<s32>(e8))), e12, v, e + 32});
    for (u32 k = 0; k < 12; k += 4) store<u32>(out + k, load<u32>(m + 48 + k));
    ctx.fpr[1] = Lfs32(m + 56);
}

// const char* f(T* o): for a type-4 o (u8 +18), the first entry p of the
// chain +2576 / +2564 that is also type 4 with u8 +2460 set and whose
// p->vt[48](p) is a non-empty string: r3 = p->vt[48](p) (called three
// times, as the original does). Otherwise "..." at 0x00B1E26C for a
// type-2 o, else o + 156.
DECOMP(003280F8, Obj_DisplayName_003280F8) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 18) == 4) {
        for (u32 p = load<u32>(o + 2576); p != 0; p = load<u32>(p + 2564)) {
            if (load<u8>(p + 18) != 4 || load<u8>(p + 2460) == 0) continue;
            if (static_cast<u32>(VCall(ctx, p, 48, {p})) == 0) continue;
            if (load<u8>(static_cast<u32>(VCall(ctx, p, 48, {p}))) == 0) continue;
            VCall(ctx, p, 48, {p});
            return;
        }
    }
    ctx.gpr[3] = load<u8>(o + 18) == 2 ? 0x00B1E26Cu : o + 156;
}

// void f(T* o): the channel record c = 0x010B19E0 + 11968 * (the u8 at
// (*0x00F65CA0)->+4209) + 176: o's bytes +154 / +155 / +156 = whether
// c->+8 & c->+12 has 0x80 / 0x20 / 0x10, then the f32s +136..+148 =
// Obj_GetChannel_0045B160(c, 0..3). r3 = o + 148, f1 the last channel.
DECOMP(003AEAAC, Obj_ReadChannels_003AEAAC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 c = 0x010B19E0u + 11968u * load<u8>(g + 4209) + 176u;
    const u32 f = load<u32>(c + 8) & load<u32>(c + 12);
    store<u8>(o + 154, (f & 0x80u) != 0 ? 1 : 0);
    store<u8>(o + 155, (f & 0x20u) != 0 ? 1 : 0);
    store<u8>(o + 156, (f & 0x10u) != 0 ? 1 : 0);
    for (u32 k = 0; k < 4; ++k) {
        call(ctx, 0x0045B160, {c, k});
        Stfs32(o + 136 + 4 * k, ctx.fpr[1]);
    }
    ctx.gpr[3] = o + 148;
}

// bool f(T* o, -, u8 b, u32 w; f32 t): when o->vt[32](o, 0) is 8 or 6 and
// the track 0 of o->+96 is at most K through (Anim_TrackTime_000DC488 /
// Anim_TrackLength164_000DC418 <= K, K the f32 at 0x010867A8): r3 = 0.
// Otherwise the f32 +128 = t, u8 +159 = b, o->vt[28](o, 0, 7) (vtable
// read before the stores) and +116 = w; r3 = 1. f1 as left.
DECOMP(003B0104, Anim_Restart_003B0104) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2), w = arg<u32>(ctx, 3);
    const f64 t = farg(ctx, 0);
    const s32 k = static_cast<s32>(VCall(ctx, o, 32, {o, 0}));
    if (k == 8 || k == 6) {
        call(ctx, 0x000DC488, {load<u32>(o + 96), 0, 0});
        const f64 t1 = ctx.fpr[1];
        call(ctx, 0x000DC418, {load<u32>(o + 96), 0});
        const f64 q = static_cast<f32>(t1 / ctx.fpr[1]);
        ctx.fpr[1] = q;
        if (q <= Lfs32(0x010867A8u)) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    const u32 vt = load<u32>(o);
    Stfs32(o + 128, t);
    store<u8>(o + 159, static_cast<u8>(b));
    VCallVt(ctx, vt, 28, {o, 0, 7});
    store<u32>(o + 116, w);
    ctx.gpr[3] = 1;
}

// Static init: the pointer vectors 0x0108A13C (u16 +6 = 304),
// 0x0108A110 (132; PtrVector_Init(v, 32, 0, 0)), 0x0108A128 (16; 256) and
// 0x0108A150 (292; 16), then the tuning var 0x0108A164
// (TuningVar_ConstructBase_004FBFA8(v, 0x00AE21B4), f32 +72 = 0.5, vtable
// 0x00B0D99C). r3 = the vtable.
DECOMP(004285B8, StaticInit_004285B8) {
    call(ctx, 0x005516E0, {0x0108A13Cu});
    store<u16>(0x0108A13Cu + 6, 304);
    const u32 vs[3] = {0x0108A110u, 0x0108A128u, 0x0108A150u};
    const u16 fl[3] = {132, 16, 292};
    const u32 cap[3] = {32, 256, 16};
    for (int i = 0; i < 3; ++i) {
        call(ctx, 0x005516E0, {vs[i]});
        store<u16>(vs[i] + 6, fl[i]);
        call(ctx, 0x00551598, {vs[i], cap[i], 0, 0});
    }
    call(ctx, 0x004FBFA8, {0x0108A164u, 0x00AE21B4u});
    store<u32>(0x0108A164u + 72, 0x3F000000u);
    store<u32>(0x0108A164u, 0x00B0D99Cu);
    ctx.gpr[3] = 0x00B0D99Cu;
}

// void f(u32 h, a, s8 c): when voice is on (u8 0x010AEC60) and the handle
// h (index h >> 16 below the count 0x010B0218, serial h & 0xFFFF != 0)
// matches its 272-byte entry e of 0x010B021C (serial (e->+256 >> 15) &
// 0xFFFF, flag 0x80 of the u8 +256 clear) and e's voice +16 is playing
// (Voice_IsPlaying): Voice_SetParam(e->+16, a, c). r3 as left.
DECOMP(0044A78C, Voice_SetParamByHandle_0044A78C) {
    const u32 h = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const s32 c = static_cast<s8>(arg<u32>(ctx, 2) & 0xFFu);
    const u32 on = load<u8>(0x010AEC60u);
    ctx.gpr[3] = on;
    if (on == 0) return;
    const u32 lo = h & 0xFFFFu;
    ctx.gpr[3] = h;
    if (lo == 0) return;
    constexpr u32 tab = 0x010B0200u;
    const u32 hi = h >> 16;
    ctx.gpr[3] = hi;
    if (!(static_cast<s32>(hi) < static_cast<s32>(load<u32>(tab + 24)))) return;
    const u32 e = load<u32>(tab + 28) + 272u * hi;
    ctx.gpr[3] = e;
    if (((load<u32>(e + 256) >> 15) & 0xFFFFu) != lo) return;
    if ((load<u8>(e + 256) & 0x80u) != 0) return;
    if (static_cast<u32>(call(ctx, 0x006D5E10, {load<u32>(e + 16)})) == 0) return;
    const u32 e2 = load<u32>(tab + 28) + 272u * hi;
    call(ctx, 0x006D50F0, {load<u32>(e2 + 16), a, static_cast<u64>(static_cast<s64>(c))});
}

// void f(const char* title, a, Ui* b, Ui* c, T* d): the dialog request
// record 0x010C1B88: its title (cleared, then String_CopyN(rec, title,
// 127) for a non-empty one), +128 = a, +132 = b (default Ui_Find(
// 0x00AEB5A8, 0)), +136 = c (default Ui_Find(0x00AEB5D0, b)), +144 = d
// (d->+108 counted up when set), then the u8s 0x010C1C1C = 0 /
// 0x010C1C1D = 1. r3 = 0x010C0000.
DECOMP(004A8CD0, Dialog_Request_004A8CD0) {
    const u32 s = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), d = arg<u32>(ctx, 4);
    u32 b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    constexpr u32 rec = 0x010C1B88u;
    store<u8>(rec, 0);
    if (s != 0 && load<u8>(s) != 0) call(ctx, 0x00553638, {rec, s, 127});
    store<u32>(rec + 128, a);
    if (b == 0) b = static_cast<u32>(call(ctx, 0x004D5544, {0x00AEB5A8u, 0}));
    if (c == 0) c = static_cast<u32>(call(ctx, 0x004D5544, {0x00AEB5D0u, b}));
    store<u32>(rec + 132, b);
    store<u32>(rec + 136, c);
    if (d != 0) store<u32>(d + 108, load<u32>(d + 108) + 1u);
    store<u32>(rec + 144, d);
    store<u8>(0x010C1C1Cu, 0);
    store<u8>(0x010C1C1Du, 1);
    ctx.gpr[3] = 0x010C0000u;
}

// void f(Pool* o, u32 h): frees the handle h (index h >> 16 below o->+252,
// serial h & 0xFFFF != 0) when its 16-byte entry e of o->+256 matches
// (serial (e->+12 >> 15) & 0xFFFF, flag 0x80 of the u8 +12 clear): the
// object p = e->+8 gets *p = 0, the release hook (*0x00F65F84)->+292
// (when set) is called with p->+4 (r4 = the hook's entry, as the
// original's call leaves it), p's u16 +6 = 0, then
// HandlePool_Free_0078967C(&tmp, o + 228, h). The original spills p->+4
// and h to its stack (sp+48 / sp+64) on the way; kept. r3 as left.
DECOMP(004B2A04, Pool_FreeHandle_004B2A04) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 lo = h & 0xFFFFu, hi = h >> 16;
    ctx.gpr[3] = lo;
    if (lo == 0) return;
    if (!(static_cast<s32>(hi) < static_cast<s32>(load<u32>(o + 252)))) return;
    const u32 e = load<u32>(o + 256) + ((h >> 12) & 0xFFFF0u);
    ctx.gpr[3] = e + 12;
    if (((load<u32>(e + 12) >> 15) & 0xFFFFu) != lo) return;
    ctx.gpr[3] = 1;
    if ((load<u8>(e + 12) & 0x80u) != 0) return;
    const u32 p = load<u32>(e + 8);
    store<u32>(p, 0);
    Stack frame(ctx, 32);
    const u32 tmp = frame.addr();  // sp+112; tmp + 16 / + 20 stand in for the spill slots
    const u32 cb = load<u32>(load<u32>(0x00F65F84u) + 292);
    if (cb != 0) {
        const u32 v = load<u32>(p + 4);
        store<u32>(tmp + 16, v);
        call_ptr(ctx, cb, {v, load<u32>(cb)});
    }
    store<u16>(p + 6, 0);
    store<u32>(tmp + 20, h);
    call(ctx, 0x0078967C, {tmp, o + 228, h});
}

// s16 f(T* o): resets the record o: the index of "..." (0x00AEE2E8) in the
// 5-entry, 20-byte name-hash table 0x00F65F88 (String_HashNoCase; not
// found: String_Format(0x00AEE4FC, name) and -1) to the s16 +0, the
// f32s +24 / +28 = 0.25 / 1.0, the flags 0xF4 of the u8 +20 cleared,
// -1 in the s16s +52 / +56 / +58, everything else (+4..+16, +32..+48,
// u16 +54, u8 +21, +60, +64) 0. r3 = the index.
DECOMP(004B42CC, Rec_Reset_004B42CC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 hs = static_cast<u32>(call(ctx, 0x0056A710, {0x00AEE2E8u}));
    s32 idx = -1;
    for (u32 i = 0; i < 5; ++i) {
        if (load<u32>(0x00F65F88u + 20 * i) == hs) {
            idx = static_cast<s32>(i);
            break;
        }
    }
    if (idx < 0) call(ctx, 0x005534D0, {0x00AEE4FCu, 0x00AEE2E8u});
    const u32 b = load<u8>(o + 20);
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 24, 0x3E800000u);  // 0.25f
    store<u32>(o + 28, 0x3F800000u);  // 1.0f
    store<u16>(o + 54, 0);
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0);
    store<u8>(o + 20, static_cast<u8>(b & ~0xF4u));
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    store<u32>(o + 32, 0);
    store<u8>(o + 21, 0);
    store<u16>(o + 52, 0xFFFF);
    store<u16>(o, static_cast<u16>(idx));
    store<u16>(o + 56, 0xFFFF);
    store<u16>(o + 58, 0xFFFF);
    store<u32>(o + 60, 0);
    store<u32>(o + 64, 0);
    ret(ctx, idx);
}

// bool f(Xml* x): the current level record (Mem_Alloc(1168, 0, 16) built
// by 0x004EAD20; 0 on failure) to 0x010C2C28; with an attribute "..."
// (0x00AF0120) its name is copied to the record (String_CopyN, 128) and
// the record filed in the tree 0x010C2CAC under its String_HashNoCase;
// a second attribute (0x00AF0130) is copied to record + 128. r3 = 1.
DECOMP(004ED58C, Level_ReadRecord_004ED58C) {
    const u32 x = arg<u32>(ctx, 0);
    u32 m = static_cast<u32>(call(ctx, 0x0054ED38, {1168, 0, 16}));
    if (m != 0) call(ctx, 0x004EAD20, {m});
    constexpr u32 cur = 0x010C2C28u;
    store<u32>(cur, m);
    const u32 a = static_cast<u32>(call(ctx, 0x004F9628, {x, 0x00AF0120u}));
    if (a != 0) {
        const u32 hs = static_cast<u32>(call(ctx, 0x0056A710, {a}));
        call(ctx, 0x00553638, {load<u32>(cur), a, 128});
        call(ctx, 0x00799A7C, {0x010C2CACu, hs, load<u32>(cur)});
        const u32 a2 = static_cast<u32>(call(ctx, 0x004F9628, {x, 0x00AF0130u}));
        if (a2 != 0) call(ctx, 0x00553638, {load<u32>(cur) + 128, a2, 128});
    }
    ctx.gpr[3] = 1;
}

// const char* f(Table* o, u32 key, const char* dflt, bool quiet): the
// string for key in o->+4 (binary search of its sorted {key, index} pairs
// +24, count +20; a negative index counts as missing): +32 + the offset
// (+28)[index]. Missing (or no table): unless quiet, String_Format(
// 0x00AF0E38, Hash_ToString(key)); r3 = dflt, or "" at 0x00AF0E58 for
// none.
DECOMP(004F86B0, StrTable_Lookup_004F86B0) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), dflt = arg<u32>(ctx, 2), quiet = arg<u32>(ctx, 3) & 0xFFu;
    const u32 t = load<u32>(o + 4);
    if (t != 0) {
        const u32 arr = load<u32>(t + 24);
        s32 lo = 0, hi = static_cast<s32>(load<u32>(t + 20) - 1u);
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 off = static_cast<u32>(mid) << 3;
            const u32 k = load<u32>(arr + off);
            if (k > key) {
                hi = mid - 1;
            } else if (k < key) {
                lo = mid + 1;
            } else {
                const s32 idx = static_cast<s32>(load<u32>(arr + off + 4));
                if (idx < 0) break;
                ctx.gpr[3] = load<u32>(t + 32) + load<u32>(load<u32>(t + 28) + (static_cast<u32>(idx) << 2));
                return;
            }
        }
    }
    if (quiet == 0) {
        const u32 s = static_cast<u32>(call(ctx, 0x0056A6E0, {key}));
        call(ctx, 0x005534D0, {0x00AF0E38u, s});
    }
    ctx.gpr[3] = dflt != 0 ? dflt : 0x00AF0E58u;
}

// void Quat_FromEuler(Quat* q; f32 x, f32 y, f32 z): with k the f32 at
// 0x0074D8F8 (half-angle scale), a = y k, b = z k, c = x k (singles) and
// their cosines / sines (Math_Cosf / Math_Sinf, in the order cos a, b, c,
// sin a, b, c): q = {sa sb cc + ca cb sc, cb sa cc + ca sb sc,
// ca sb cc - cb sa sc, ca cb cc - sa sb sc} (stored w, x, z, y). r3 as
// left, f1 = q.y.
DECOMP(0074D900, Quat_FromEuler_0074D900) {
    const u32 q = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0), y = farg(ctx, 1), z = farg(ctx, 2);
    const f64 k = Lfs32(0x0074D8F8u);
    const f64 a = static_cast<f32>(y * k);
    const f64 ca = callf(ctx, 0x007344E0, {}, {a});
    const f64 b = static_cast<f32>(z * k);
    const f64 cb = callf(ctx, 0x007344E0, {}, {b});
    const f64 c = static_cast<f32>(x * k);
    const f64 cc = callf(ctx, 0x007344E0, {}, {c});
    const f64 sa = callf(ctx, 0x007345F0, {}, {a});
    const f64 sb = callf(ctx, 0x007345F0, {}, {b});
    const f64 sc = callf(ctx, 0x007345F0, {}, {c});
    const f64 f2 = static_cast<f32>(sa * sb), f3 = static_cast<f32>(ca * cb);
    const f64 f4 = static_cast<f32>(cb * sa), f5 = static_cast<f32>(ca * sb);
    const f64 f6 = static_cast<f32>(f2 * sc), f7 = static_cast<f32>(f3 * sc);
    const f64 f8 = static_cast<f32>(f4 * sc), f1 = static_cast<f32>(f5 * sc);
    const f64 w = static_cast<f32>(std::fma(f3, cc, -f6));
    const f64 qx = static_cast<f32>(std::fma(f2, cc, f7));
    const f64 qz = static_cast<f32>(std::fma(f5, cc, -f8));
    const f64 qy = static_cast<f32>(std::fma(f4, cc, f1));
    Stfs32(q + 12, w);
    Stfs32(q, qx);
    Stfs32(q + 8, qz);
    Stfs32(q + 4, qy);
    ctx.fpr[1] = qy;
}

// void delete(T* o): Delete424_35 with the class 0x00B15308 and 480 bytes.
DECOMP(0075140C, VtB15308_Delete_0075140C) { Delete424_35(ctx, 0x00B15308u, 480); }

// void delete(T* o): Delete424_35 with the class 0x00B18030 and 624 bytes.
DECOMP(00756DCC, VtB18030_Delete_00756DCC) { Delete424_35(ctx, 0x00B18030u, 624); }

// u32 f(Vec32* v, const Rec* r): appends r to the u16-counted vector of
// 32-byte records v (data +0, capacity +4 doubled through
// PtrVector_Reserve when full, count +8, high-water +10): words +0..+8
// and +16..+24 copied (not +12), then the top 6 flag bits of +28 merged
// from r->+28 one at a time (the rest of the slot's old +28 kept). r3 =
// the new record's index.
DECOMP(0076A800, Vec32_Push_0076A800) {
    const u32 v = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, 0x00551718, {v, (cap << 1) & 0x1FFFEu});
    const u32 e = load<u32>(v) + ((static_cast<u32>(load<u16>(v + 8)) << 5) & 0x1FFFE0u);
    store<u32>(e, load<u32>(r));
    store<u32>(e + 4, load<u32>(r + 4));
    store<u32>(e + 8, load<u32>(r + 8));
    ctx.fpr[1] = Lfs32(r + 8);
    store<u32>(e + 16, load<u32>(r + 16));
    const u32 w20 = load<u32>(r + 20);
    u32 f = load<u32>(e + 28) & 0x7FFFFFFFu;
    store<u32>(e + 20, w20);
    store<u32>(e + 24, load<u32>(r + 24));
    f |= load<u32>(r + 28) & 0x80000000u;
    store<u32>(e + 28, f);
    for (u32 bit = 0x40000000u; bit >= 0x04000000u; bit >>= 1) {
        f = (f & ~bit) | (load<u32>(r + 28) & bit);
        store<u32>(e + 28, f);
    }
    const u32 n = load<u16>(v + 8) + 1u;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n & 0xFFFFu) ? hw : n));
    ctx.gpr[3] = (n & 0xFFFFu) - 1u;
}

// ---------------------------------------------------------------- sweep: 70 instructions
// void f(T* o): when bit 1 of o's u8 +304 is set it is cleared, and with
// an attachment +256: unless o->vt[124](o), the attachment's vt[40](it,
// 0); +256 = 0, (o->+48)->vt[96](it, 0) and the u16 +40 of
// (o->+48)->+164 (when set) gets 0x400 and loses 0x1000; then the word
// at o->+292 (when set) and +292 itself are cleared. r3 = 0 (the u8 when
// the bit is clear).
DECOMP(0002EC04, Obj_Detach_0002EC04) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 304);
    ctx.gpr[3] = b;
    if ((b & 1u) == 0) return;
    const u32 p = load<u32>(o + 256);
    store<u8>(o + 304, static_cast<u8>(b & 0xFEu));
    if (p != 0) {
        if (static_cast<u32>(VCall(ctx, o, 124, {o})) == 0) {
            const u32 a = load<u32>(o + 256);
            VCall(ctx, a, 40, {a, 0});
        }
        const u32 q = load<u32>(o + 48);
        store<u32>(o + 256, 0);
        VCall(ctx, q, 96, {q, 0});
        const u32 r = load<u32>(load<u32>(o + 48) + 164);
        if (r != 0) store<u16>(r + 40, static_cast<u16>((load<u16>(r + 40) & ~0x1000u) | 0x400u));
    }
    const u32 s = load<u32>(o + 292);
    if (s != 0) store<u32>(s, 0);
    store<u32>(o + 292, 0);
    ctx.gpr[3] = 0;
}

// void f(Game* o): once (u8 +512): +416 = the word at g + 0x1281E0 (g =
// *0x00F65CA0, the game itself); when g's player (g->+4216) has flags 2
// and 8 in +192, its u8 +968 gets 1, Player_SetOutfit11F32C(p,
// 0xFE11127C), p->vt[176](p, 1) and Obj_SetEffectHash_000DAEC8(p,
// 0x24099D19); then the u8 at g + 0x1285A5 = 1. r3 = g (o when already
// done).
DECOMP(00060100, Game_StartOnce_00060100) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 512) != 0) return;
    store<u8>(o + 512, 1);
    store<u32>(o + 416, load<u32>(load<u32>(0x00F65CA0u) + 0x1281E0u));
    u32 g = load<u32>(0x00F65CA0u);
    const u32 p = load<u32>(g + 4216);
    if (p != 0 && (load<u32>(p + 192) & 2u) != 0 && (load<u32>(p + 192) & 8u) != 0) {
        store<u8>(p + 968, static_cast<u8>(load<u8>(p + 968) | 1u));
        call(ctx, 0x0011F32C, {p, 0xFE11127Cu});
        VCall(ctx, p, 176, {p, 1});
        call(ctx, 0x000DAEC8, {p, 0x24099D19u});
        g = load<u32>(0x00F65CA0u);
    }
    store<u8>(g + 0x1285A5u, 1);
    ctx.gpr[3] = g;
}

// T* New(-, Xml* x): NewXml34 (24 bytes): +12 / u8 +20 cleared, f32 +16 =
// 1.0, vtable 0x00B0F270, u8 +21 cleared; the hash 0x001EBA11 to +12
// (default 0), the f32 0x04B70827 to +16 and the bools 0x0023BEA4 /
// 0x855E65C3 to +20 / +21 (current defaults).
DECOMP(0006EE44, VtB0F270_NewXml_0006EE44) {
    NewXml34(ctx, 24, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u8>(m + 20, 0);
        store<u32>(m + 16, 0x3F800000u);
        store<u32>(m, 0x00B0F270u);
        store<u8>(m + 21, 0);
        RdHash34(ctx, x, 0x001EBA11u, m + 12, 0);
        RdF32Cur34(ctx, x, 0x04B70827u, m + 16);
        RdBoolCur34(ctx, x, 0x0023BEA4u, m + 20);
        RdBoolCur34(ctx, x, 0x855E65C3u, m + 21);
    });
}

// T* New(-, Xml* x): NewXml34 (28 bytes): +12..+20 cleared, vtable
// 0x00B0F6D0, u8 +24 cleared; the hashes 0x8A93973F (+12, default 0) /
// 0x7A30D7E3 / 0x03D1FCFC and the bool 0x8CB95A92 (+24; current
// defaults).
DECOMP(00075768, VtB0F6D0_NewXml_00075768) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, 0x00B0F6D0u);
        store<u8>(m + 24, 0);
        RdHash34(ctx, x, 0x8A93973Fu, m + 12, 0);
        RdHashCur34(ctx, x, 0x7A30D7E3u, m + 16);
        RdHashCur34(ctx, x, 0x03D1FCFCu, m + 20);
        RdBoolCur34(ctx, x, 0x8CB95A92u, m + 24);
    });
}

// T* New(-, Xml* x): NewXml34 (48 bytes): +12 cleared, the vec3 +16 = the
// default 0x01164B90, vtable 0x00B10918, the u8s +32 / +33 cleared; the
// hash 0xA12A9A30 to +12 (default 0; a failed read sets u8 +32), the vec3
// 0xA89D0F29 to +16 (current default; a failed read sets u8 +33).
DECOMP(00090AE4, VtB10918_NewXml_00090AE4) {
    NewXml34(ctx, 48, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, load<u32>(0x01164B90u));
        store<u32>(m, 0x00B10918u);
        store<u32>(m + 20, load<u32>(0x01164B94u));
        store<u8>(m + 32, 0);
        store<u32>(m + 24, load<u32>(0x01164B98u));
        store<u8>(m + 33, 0);
        ctx.fpr[1] = Lfs32(0x01164B98u);
        if (static_cast<u32>(call(ctx, kXmlReadHash35, {x, 0xA12A9A30u, m + 12, 0})) == 0) store<u8>(m + 32, 1);
        if (static_cast<u32>(call(ctx, 0x004F9E0C, {x, 0xA89D0F29u, m + 16, m + 16})) == 0) store<u8>(m + 33, 1);
    });
}

// T* New(-, Xml* x): NewXml34 (20 bytes): +12 / u8 +16 / u8 +17 cleared,
// vtable 0x00B10D28, u8 +18 cleared; the hash 0x000107E5 to +12 (default
// 0) and the bools 0x00D8E5D6 / 0xA18DAB9F / 0xB555B755 to +16..+18
// (current defaults).
DECOMP(00097D38, VtB10D28_NewXml_00097D38) {
    NewXml34(ctx, 20, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u8>(m + 16, 0);
        store<u8>(m + 17, 0);
        store<u32>(m, 0x00B10D28u);
        store<u8>(m + 18, 0);
        RdHash34(ctx, x, 0x000107E5u, m + 12, 0);
        RdBoolCur34(ctx, x, 0x00D8E5D6u, m + 16);
        RdBoolCur34(ctx, x, 0xA18DAB9Fu, m + 17);
        RdBoolCur34(ctx, x, 0xB555B755u, m + 18);
    });
}

// T* New(-, Xml* x): XmlHHBB35 with vtable 0x00B11788, the hashes
// 0x8A93973F / 0x00255BF3 and the bools 0x7A281983 / 0xF5897C08.
DECOMP(000A40C8, VtB11788_NewXml_000A40C8) {
    XmlHHBB35(ctx, 0x00B11788u, 0x8A93973Fu, 0x00255BF3u, 0x7A281983u, 0xF5897C08u);
}

// The trophy check of 0x000B2AE0 (one flag): the hash 0x67DEAB5E in a
// stack slot (Hash_ToString of it called for nothing), looked up in the
// map *0x00F64624 (SortedMap_Find742EF4); an id other than -1 unlocks
// (Trophy_Unlock4FB4A0(*0x00F668E8, id)).
static void TrophyCheck35(PPUContext& ctx, u32 slot) {
    store<u32>(slot, 0x67DEAB5Eu);
    call(ctx, 0x0056A6E0, {0x67DEAB5Eu});
    const u32 r = static_cast<u32>(call(ctx, 0x00742EF4, {load<u32>(0x00F64624u), slot}));
    const s32 id = static_cast<s32>(load<u32>(r));
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(id));
    if (id != -1) call(ctx, 0x004FB4A0, {load<u32>(0x00F668E8u), static_cast<u64>(static_cast<s64>(id))});
}

// void f(T* o, Msg* m): when o->+244 is set and m's sender (+4) has flag
// 1 of +188: the trophy check for each of the flags 0x400 / 0x800 of m's
// u16 +12 (TrophyCheck35, stack slots sp+116 / sp+112). r3 as left.
DECOMP(000B2AE0, Trophy_OnMsg_000B2AE0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0;
    if (load<u32>(o + 244) == 0) return;
    const u32 s = load<u32>(m + 4);
    if (s == 0 || (load<u32>(s + 188) & 1u) == 0) return;
    Stack frame(ctx, 16);
    if ((load<u16>(m + 12) & 0x400u) != 0) TrophyCheck35(ctx, frame.addr() + 4);
    const u32 f = load<u16>(m + 12) & 0x800u;
    ctx.gpr[3] = f;
    if (f != 0) TrophyCheck35(ctx, frame.addr());
}

// void f(T* o): by o->vt[32](o, 2): 0 -> the effect +356 (when set) gets
// Effect_SetHash4CF988(it, 0x5BA5377E) and o->vt[28](o, 2, 1); 1 -> the
// track reset (Anim_TrackSet36_0029879C(o->+720, 0, 0) with an animator
// +720, else Anim_TrackSet164_000D8760(o, 0, 0)) and, when it reports
// success, o->vt[28](o, 1, 5); anything else nothing. r3 as left.
DECOMP(000BFD7C, Obj_StateRestart_000BFD7C) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 k = static_cast<s32>(VCall(ctx, o, 32, {o, 2}));
    if (k == 0) {
        const u32 e = load<u32>(o + 356);
        if (e != 0) call(ctx, 0x004CF988, {e, 0x5BA5377Eu});
        VCall(ctx, o, 28, {o, 2, 1});
    } else if (k == 1) {
        const u32 a = load<u32>(o + 720);
        const u32 r = a != 0 ? static_cast<u32>(call(ctx, 0x0029879C, {a, 0, 0})) : static_cast<u32>(call(ctx, 0x000D8760, {o, 0, 0}));
        if (r != 0) VCall(ctx, o, 28, {o, 1, 5});
    }
}

// void f(T* o, Query* q): Obj_CopyQuery_000CDB28(o, q), q->+164 =
// o->+200, then each 76-byte record of o (+180, u16 count +188) appended
// (74 bytes, memmove) to q's u16-counted vector +144 (capacity +148
// doubled through PtrVector_Reserve when full, count +152, high-water
// +154). The count used for the slot is the one the loop carries (re-read
// only after a Reserve), as the original does. r3 = the last count + 1
// (q's count at entry when nothing was added).
DECOMP(000DE454, Obj_CopyQueryRecs_000DE454) {
    const u32 o = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28, {o, q});
    store<u32>(q + 164, load<u32>(o + 200));
    u32 c = load<u16>(q + 152);
    ctx.gpr[3] = c;
    for (u32 i = 0; i < load<u16>(o + 188); ++i) {
        const u32 src = load<u32>(o + 180) + 76u * i;
        const u32 cap = load<u16>(q + 148);
        if ((c & 0xFFFFu) == cap) {
            call(ctx, 0x00551718, {q + 144, (cap << 1) & 0x1FFFEu});
            c = load<u16>(q + 152);
        }
        call(ctx, 0x0071C3F0, {load<u32>(q + 144) + 76u * (c & 0xFFFFu), src, 74});
        const u32 n = load<u16>(q + 152) + 1u;
        const u32 hw = load<u16>(q + 154);
        store<u16>(q + 152, static_cast<u16>(n));
        store<u16>(q + 154, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n & 0xFFFFu) ? hw : n));
        c = n;
        ctx.gpr[3] = n;
    }
}

// void f(Set* o, u32 i, Vec3* cur, const Vec3* tgt; f32 a, f32 b): while
// bit i of o's bit set +12 is on, cur eases toward tgt: once every axis is
// within eps (the f32 at 0x0010EBE4) the bit is cleared, otherwise cur =
// Curve_Lerp3_004A8598(cur, tgt; a, b) (through a stack vec3). r3 as
// left, f1 = b (the last lerped z after a lerp).
DECOMP(0010EBE8, Set_EaseBit_0010EBE8) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), cur = arg<u32>(ctx, 2), tgt = arg<u32>(ctx, 3);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    const u32 wa = o + 12 + ((i >> 3) & 0x1FFFFFFCu);
    const u32 bit = 1u << (i & 31u);
    ctx.fpr[1] = b;
    if ((load<u32>(wa) & bit) == 0) return;
    const f64 eps = Lfs32(0x0010EBE4u);
    bool near = true;
    for (u32 k = 0; k < 12 && near; k += 4) near = std::fabs(static_cast<f32>(Lfs32(cur + k) - Lfs32(tgt + k))) < eps;
    if (near) {
        store<u32>(wa, load<u32>(wa) & ~bit);
        ctx.gpr[3] = wa;
        return;
    }
    Stack frame(ctx, 16);
    const u32 t = frame.addr();  // sp+112
    callf(ctx, 0x004A8598, {t, cur, tgt}, {a, b});
    for (u32 k = 0; k < 12; k += 4) store<u32>(cur + k, load<u32>(t + k));
    ctx.fpr[1] = Lfs32(t + 8);
}

// void f(T* o): for an o whose +216 def has the class hash 0x1D8C2817 at
// +192: every entry e (Obj_EntryAt3769C4(h, k) for k below
// Obj_Kind376A08(h), re-counted each step) of the resolved handle h =
// Handle_ResolveBit400(*0x00F64D34) found in the 107-entry, 28-byte table
// 0x00AC9A1C at index j clears flag 0x80 of o's u8 +656 + j. r3 as left.
DECOMP(0012E41C, Obj_ClearKnownFlags_0012E41C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = load<u32>(o + 216);
    ctx.gpr[3] = d;
    if (d == 0) return;
    ctx.gpr[3] = load<u32>(d + 192);
    if (load<u32>(d + 192) != 0x1D8C2817u) return;
    const u32 h = static_cast<u32>(call(ctx, 0x00365BA4, {load<u32>(0x00F64D34u)}));
    if (h == 0) return;
    for (s32 k = 0; k < static_cast<s32>(call(ctx, 0x00376A08, {h})); ++k) {
        const u32 e = static_cast<u32>(call(ctx, 0x003769C4, {h, static_cast<u64>(static_cast<s64>(k))}));
        if (e == 0) continue;
        for (u32 j = 0; j < 107; ++j) {
            if (load<u32>(0x00AC9A1Cu + 28 * j) == e) {
                store<u8>(o + 656 + j, static_cast<u8>(load<u8>(o + 656 + j) & 0x7Fu));
                break;
            }
        }
    }
}

// void f(T* o, U* u): Thunk_000E44C0(o, u), then u takes o's state: the
// f32 +236 = o->+268, the 7 words +208.. = o->+240.., the top 15 bits of
// +20 = 4 (<< 17), +240 / +244 = o->+272 / +276, f32 +248 / +252 =
// o->+280 / +284, the flags 0x80000000 / 0x40000000 of +736 from
// o->+768 (one at a time), and the 480 bytes +256 = o->+288 (memmove).
// r3 as left.
DECOMP(001C6FB0, Obj_CopyState_001C6FB0) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {o, u});
    store<u32>(u + 236, load<u32>(o + 268));
    for (u32 k = 0; k < 28; k += 4) store<u32>(u + 208 + k, load<u32>(o + 240 + k));
    store<u32>(u + 20, (load<u32>(u + 20) & 0x1FFFFu) | 0x80000u);
    store<u32>(u + 240, load<u32>(o + 272));
    store<u32>(u + 244, load<u32>(o + 276));
    const u32 w280 = load<u32>(o + 280);
    const u32 f736 = load<u32>(u + 736);
    store<u32>(u + 248, w280);
    store<u32>(u + 252, load<u32>(o + 284));
    ctx.fpr[1] = Lfs32(o + 284);
    u32 f = (f736 & 0x7FFFFFFFu) | (load<u32>(o + 768) & 0x80000000u);
    store<u32>(u + 736, f);
    f = (f & ~0x40000000u) | (load<u32>(o + 768) & 0x40000000u);
    store<u32>(u + 736, f);
    call(ctx, 0x0071C3F0, {u + 256, o + 288, 480});
}

// void f(T* o, bool full): with full, o leaves the list 0x01013B48
// (List1013B48_Remove) and the world list (World_ListRemove_0046B7A4(
// *0x00F65CA0, 10, 0x00F34340, o, 0)), its effect +428 is released
// (Effect_Release(e, 0x00010CBB); e->+136 |= 0x200) and the objects +432
// (vt[4]) / +416 (vt[8]) dropped, each slot cleared; then always
// 0x000D74C0(o, full). r3 as left.
DECOMP(0020ABD8, Obj_Shutdown_0020ABD8) {
    const u32 o = arg<u32>(ctx, 0), full = arg<u32>(ctx, 1) & 0xFFu;
    if (full != 0) {
        call(ctx, 0x000D68CC, {o});
        call(ctx, 0x0046B7A4, {load<u32>(0x00F65CA0u), 10, 0x00F34340u, o, 0});
        const u32 e = load<u32>(o + 428);
        if (e != 0) {
            call(ctx, 0x004CF3C4, {e, 0x00010CBBu});
            const u32 e2 = load<u32>(o + 428);
            store<u32>(e2 + 136, load<u32>(e2 + 136) | 0x200u);
            store<u32>(o + 428, 0);
        }
        const u32 p = load<u32>(o + 432);
        if (p != 0) {
            VCall(ctx, p, 4, {p});
            store<u32>(o + 432, 0);
        }
        const u32 q = load<u32>(o + 416);
        if (q != 0) {
            VCall(ctx, q, 8, {q});
            store<u32>(o + 416, 0);
        }
    }
    call(ctx, 0x000D74C0, {o, full});
}

// u32 f(T* o): for an o of kind (s8 +24) 1 or 2, the value of the key
// 0x91C7CF5B in the 16-byte {key, -, -, value} table (+124, count +128)
// of the block at o + (s8)o->+16; 0 when absent (or for other kinds).
DECOMP(0020DB54, Obj_FindKeyValue_0020DB54) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 t = static_cast<s8>(load<u8>(o + 24));
    u32 r = 0;
    const u32 b = o + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(o + 16))));
    if ((t == 1 || t == 2) && b != 0) {
        for (s32 i = 0; i < static_cast<s32>(load<u32>(b + 128)); ++i) {
            const u32 e = load<u32>(b + 124) + 16u * static_cast<u32>(i);
            if (load<u32>(e) == 0x91C7CF5Bu) {
                r = load<u32>(e + 12);
                break;
            }
        }
    }
    ctx.gpr[3] = r;
    ctx.gpr[4] = r;
}

// void f(T* o, Query* q): Obj_CopyQuery_000CDB28(o, q), then q's +144 =
// o->+180, the bools +148..+153 = the flags 0x80 / 0x40 / 0x20 / 0x10 /
// 0x08 / 0x02 of o's u8 +184, +156 / +160 / +168 = o->+188 / +192 / +200,
// the u8s +164 / +165 = o's +196 / +197 and the name +172 = o + 204
// (String_CopyN, 256). r3 as left.
DECOMP(00220B48, Obj_CopyQueryFlags_00220B48) {
    const u32 o = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28, {o, q});
    store<u32>(q + 144, load<u32>(o + 180));
    auto fl = [&](u32 m) -> u8 { return (load<u8>(o + 184) & m) != 0 ? 1 : 0; };
    store<u8>(q + 148, fl(0x80));
    store<u8>(q + 149, fl(0x40));
    store<u8>(q + 150, fl(0x20));
    store<u8>(q + 151, fl(0x10));
    const u32 w188 = load<u32>(o + 188);
    store<u8>(q + 152, fl(0x08));
    const u8 f2 = fl(0x02);
    store<u32>(q + 156, w188);
    store<u32>(q + 160, load<u32>(o + 192));
    const u32 w200 = load<u32>(o + 200);
    store<u8>(q + 153, f2);
    const u8 b196 = load<u8>(o + 196);
    store<u32>(q + 168, w200);
    store<u8>(q + 164, b196);
    store<u8>(q + 165, load<u8>(o + 197));
    call(ctx, 0x00553638, {q + 172, o + 204, 256});
}

// void f(Cam* o; f32 v): for v > 0: e = the view record (*0x011950AC) +
// 128 * o->+240; by o's mode +24: 0 (perspective) -> h = e->+80 * 2 *
// tanf(v * (pi / 180) * 0.5) (libm_tanf) and w = h / o->+28: e->+92 = h,
// e->+88 = w, o->+36 = w, o->+40 = h; 1 -> all four = v * 2; then flag
// 0x80 of o's u8 +456 when o->+404 is 1 or 2. Constants at 0x0028BC18.
// r3 as left, f1 the last value computed (v when none).
DECOMP(0028BC28, Cam_SetFov_0028BC28) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0);
    if (!(v > Lfs32(0x0028BC18u))) return;
    const u32 e = load<u32>(0x011950A0u + 12) + ((load<u32>(o + 240) << 7) & 0xFFFFFF80u);
    const s32 t = static_cast<s32>(load<u32>(o + 24));
    const f64 k1 = Lfs32(0x0028BC1Cu);
    if (t == 1) {
        const f64 w = static_cast<f32>(v * k1);
        Stfs32(o + 36, w);
        Stfs32(o + 40, w);
        Stfs32(e + 88, w);
        Stfs32(e + 92, w);
        ctx.fpr[1] = w;
    } else if (t == 0) {
        const f64 a = static_cast<f32>(static_cast<f32>(v * Lfs32(0x0028BC20u)) * Lfs32(0x0028BC24u));
        const f64 asp = Lfs32(o + 28);
        const f64 tn = callf(ctx, 0x005E62E8, {}, {a});
        const f64 h = static_cast<f32>(static_cast<f32>(Lfs32(e + 80) * k1) * tn);
        const f64 w = static_cast<f32>(h / asp);
        Stfs32(e + 92, h);
        Stfs32(e + 88, w);
        Stfs32(o + 36, w);
        Stfs32(o + 40, h);
        ctx.fpr[1] = h;
    }
    const u32 m = load<u32>(o + 404);
    ctx.gpr[3] = m;
    if (m == 1 || m == 2) {
        const u32 b = load<u8>(o + 456) | 0x80u;
        store<u8>(o + 456, static_cast<u8>(b));
        ctx.gpr[3] = b;
    }
}

// void f(T* o): pops every entry of the u16-counted pointer stacks +32
// (count +40; each gets vt[8](it)) and +52 (count +60; vt[40](it, 1)),
// the count stored before the entry is read. (The original also has a
// "move the last entry down" step that cannot trigger for a u16 count.)
// r3 = 0.
DECOMP(002AF294, Obj_PopAll_002AF294) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 n; (n = load<u16>(o + 40)) != 0;) {
        const u32 d = load<u32>(o + 32);
        store<u16>(o + 40, static_cast<u16>(n - 1u));
        const u32 p = load<u32>(d + 4 * (n - 1u));
        VCall(ctx, p, 8, {p});
    }
    for (u32 n; (n = load<u16>(o + 60)) != 0;) {
        const u32 d = load<u32>(o + 52);
        store<u16>(o + 60, static_cast<u16>(n - 1u));
        const u32 p = load<u32>(d + 4 * (n - 1u));
        VCall(ctx, p, 40, {p, 1});
    }
    ctx.gpr[3] = 0;
}

// ~T() body: the effect +28 (when set; vtable 0x00B1D100 first) gets
// flag 0x200 of +136 and is dropped, vtable 0x00B1BE30, o's entry (+4 ==
// o) removed from the 8-byte-entry list 0x01056F1C (count u16 +2, data +8;
// later entries moved down a word at a time), then List1013B5C_Remove(o).
// r3 as left.
DECOMP(002CBD94, VtB1BE30_Dtor_002CBD94) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 e = load<u32>(o + 28);
    if (e != 0) {
        store<u32>(o, 0x00B1D100u);
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        store<u32>(o + 28, 0);
    }
    store<u32>(o, 0x00B1BE30u);
    constexpr u32 L = 0x01056F1Cu;
    for (u32 i = 0; i < load<u16>(L + 2); ++i) {
        const u32 arr = load<u32>(L + 8);
        if (load<u32>(arr + 8 * i + 4) != o) continue;
        u32 n = load<u16>(L + 2);
        if (static_cast<s32>(n) >= 2) {
            for (u32 j = i; static_cast<s32>(j) < static_cast<s32>(n) - 1; ++j) {
                store<u32>(arr + 8 * j, load<u32>(arr + 8 * j + 8));
                store<u32>(arr + 8 * j + 4, load<u32>(arr + 8 * j + 12));
                n = load<u16>(L + 2);
            }
        }
        store<u16>(L + 2, static_cast<u16>(n - 1u));
        break;
    }
    call(ctx, 0x000D6940, {o});
}

// void f(T* o, u32 h): the effect +52 (when set) gets Effect_Release(it,
// h); when o is one of the three parts p->+900.. of its owner p = o->+56,
// that part's two handlers p->+916 + 8 i (only while p->+432 == 1) and
// p->+920 + 8 i get vt[128](it, h, 1). r3 as left.
DECOMP(002F9424, Part_Release_002F9424) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 e = load<u32>(o + 52);
    if (e != 0) call(ctx, 0x004CF3C4, {e, h});
    const u32 p = load<u32>(o + 56);
    ctx.gpr[3] = p;
    if (p == 0) return;
    for (u32 i = 0; i < 3; ++i) {
        if (load<u32>(p + 900 + 4 * i) != o) continue;
        const u32 s = p + 8 * i;
        if (load<u32>(p + 432) == 1) {
            const u32 q = load<u32>(s + 916);
            if (q != 0) VCall(ctx, q, 128, {q, h, 1});
        }
        const u32 r = load<u32>(s + 920);
        ctx.gpr[3] = s + 920;
        if (r != 0) VCall(ctx, r, 128, {r, h, 1});
        return;
    }
}

// f32 f(Set* o, U* u): the height of the first of o's objects (+8, u16
// count +16; each object's position at +128) whose distance from the
// origin is within 1.5 (the f32 at 0x0030C5C4) of u's horizontal radius
// (u + 144: sqrt(x^2 + 0^2 + z^2)) and whose y is at least u's; 0 (the f32
// at 0x0030C5C0) when there is none, when an object farther out comes
// first, for no u, or when u's position equals the default 0x01164B40.
// That test is the original's: the 64-bit compare of {stack word, 0} with
// the default's x / y, the stack word being read from the frame (sp - 16)
// before u's x is stored there, and of z; both kept. r3 as left.
DECOMP(0030C5C8, Set_FindLedge_0030C5C8) {
    const u32 o = arg<u32>(ctx, 0), u = arg<u32>(ctx, 1);
    const f64 k0 = Lfs32(0x0030C5C0u);
    ctx.fpr[1] = k0;
    if (u == 0) return;
    const u32 p = u + 144;
    const u32 slot = static_cast<u32>(ctx.gpr[1]) - 16;  // the original's sp+48
    const u32 stale = load<u32>(slot);
    const u32 zw = load<u32>(p + 8);
    const f64 x = Lfs32(p), y = Lfs32(p + 4), z = Lfs32(p + 8);
    store<u32>(slot, load<u32>(p));
    if (stale == load<u32>(0x01164B40u) && load<u32>(0x01164B44u) == 0 && zw == load<u32>(0x01164B48u)) return;
    f64 r = static_cast<f32>(k0 * k0);
    r = static_cast<f32>(std::fma(x, x, r));
    const f64 k1 = Lfs32(0x0030C5C4u);
    const u32 n = load<u16>(o + 16);
    u32 a = load<u32>(o + 8);
    ctx.gpr[3] = a;
    const f64 len = static_cast<f32>(std::sqrt(static_cast<f32>(std::fma(z, z, r))));
    for (u32 i = 0; i < n; ++i, a += 4) {
        ctx.gpr[3] = a;
        const u32 q = load<u32>(a) + 128;
        const f64 qy = Lfs32(q + 4);
        f64 d = static_cast<f32>(qy * qy);
        d = static_cast<f32>(std::fma(Lfs32(q), Lfs32(q), d));
        d = static_cast<f32>(std::fma(Lfs32(q + 8), Lfs32(q + 8), d));
        d = static_cast<f32>(std::sqrt(d));
        if (std::fabs(static_cast<f32>(d - len)) <= k1) {
            if (y <= qy) ctx.fpr[1] = qy;
            return;
        }
        if (d > len) return;
    }
    ctx.gpr[3] = a;
}

// void f(T* o, a, Ev* e): for a type-4 o (u8 +18): event 6 counts the
// f32 +2576 down by e->+24 and, at or below 0 (the f32 at 0x0032D508),
// o->vt[104](o, 5, 0); event 8 with e->+24 >= +2572 gives o->vt[104](o,
// 6, 0). Then +2584 = a and 0x003252D8(o, a, e). r3 as left, f1 the last
// float read.
DECOMP(0032D510, Obj_TimerEvent_0032D510) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), e = arg<u32>(ctx, 2);
    if (load<u8>(o + 18) == 4) {
        const s32 k = static_cast<s32>(load<u32>(e));
        if (k == 6) {
            const f64 v = static_cast<f32>(Lfs32(o + 2576) - Lfs32(e + 24));
            Stfs32(o + 2576, v);
            ctx.fpr[1] = v;
            if (v <= Lfs32(0x0032D508u)) VCall(ctx, o, 104, {o, 5, 0});
        } else if (k == 8) {
            ctx.fpr[1] = Lfs32(e + 24);
            if (Lfs32(e + 24) >= Lfs32(o + 2572)) VCall(ctx, o, 104, {o, 6, 0});
        }
    }
    store<u32>(o + 2584, a);
    call(ctx, 0x003252D8, {o, a, e});
}

// T* f(-, Xml* x, P* p): reads {s32 0x04B8CADE (default 0), hash
// 0x922D3BD1 (0), f32 0x39FB22A9 (4.0)} into a stack record, then a
// 2592-byte object (SmallAlloc; Obj_Construct_00769BD8(m, p), vtable
// 0x00B1E918, +2572 = 0, the top two bits of +2576 cleared, the record
// copied to +2580..+2588). r3 = the object or 0.
DECOMP(00330798, VtB1E918_NewXml_00330798) {
    const u32 x = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 r = frame.addr();  // sp+112
    store<u32>(r, 0);
    store<u32>(r + 4, 0);
    store<u32>(r + 8, 0x40800000u);  // 4.0f
    call(ctx, kXmlReadS32_35, {x, 0x04B8CADEu, r, 0});
    RdHashCur34(ctx, x, 0x922D3BD1u, r + 4);
    RdF32Cur34(ctx, x, 0x39FB22A9u, r + 8);
    const u32 m = static_cast<u32>(call(ctx, 0x0055238C, {2592}));
    if (m != 0) {
        call(ctx, 0x00769BD8, {m, p});
        const u32 f = load<u32>(m + 2576);
        store<u32>(m + 2572, 0);
        store<u32>(m, 0x00B1E918u);
        store<u32>(m + 2576, f & 0x3FFFFFFFu);
        store<u32>(m + 2580, load<u32>(r));
        store<u32>(m + 2584, load<u32>(r + 4));
        store<u32>(m + 2588, load<u32>(r + 8));
        ctx.fpr[1] = Lfs32(r + 8);
    }
    ctx.gpr[3] = m;
}

// void f(Section* o, s32 mode, bool force): unless the mode is 4, a
// forced or changed mode (u8 +18) first gets o->vt[28](o, 0); mode 4
// clears flag 0x01000000 of +2612; other modes with that flag set stop
// the talking object (FindFlagged_00368A48(*0x00F64D3C) ->
// Obj_SetTalking_00384750(it, 0)), clear the flag and give the effect
// +2224 (when set) Effect_SetHash4CF988(it, 0x684DE37B). Then
// Section_SetModeBase_003260F0(o, mode, force). r3 as left.
DECOMP(00332624, Section_SetMode_00332624) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2) & 0xFFu;
    const s32 mode = static_cast<s32>(arg<u32>(ctx, 1));
    if ((b != 0 || mode != static_cast<s32>(load<u8>(o + 18))) && mode != 4) VCall(ctx, o, 28, {o, 0});
    if (mode == 4) {
        store<u32>(o + 2612, load<u32>(o + 2612) & ~0x01000000u);
    } else if ((load<u32>(o + 2612) & 0x01000000u) != 0) {
        const u32 f = static_cast<u32>(call(ctx, 0x00368A48, {load<u32>(0x00F64D3Cu)}));
        if (f != 0) call(ctx, 0x00384750, {f, 0});
        const u32 w = load<u32>(o + 2612);
        const u32 e = load<u32>(o + 2224);
        store<u32>(o + 2612, w & ~0x01000000u);
        if (e != 0) call(ctx, 0x004CF988, {e, 0x684DE37Bu});
    }
    call(ctx, 0x003260F0, {o, static_cast<u64>(static_cast<s64>(mode)), b});
}

// void f(T* o): the state "on" = the source +24 has flag 0x8000 of +164
// and o's u8 +192 has 0x40; the parts +196 (nothing without it) and +204
// whose own 0x8000 flag of +164 differs get vt[40](it, on, 1). r3 as
// left.
DECOMP(0036D9E4, Obj_SyncParts_0036D9E4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 196);
    const u32 b = load<u8>(o + 192);
    ctx.gpr[3] = b;
    if (a == 0) return;
    const u32 s = load<u32>(o + 24);
    const u32 on = (s != 0 && (load<u32>(s + 164) & 0x8000u) != 0 && (b & 0x40u) != 0) ? 1u : 0u;
    auto sync = [&](u32 p) {
        const u32 d = on ^ ((load<u32>(p + 164) & 0x8000u) != 0 ? 1u : 0u);
        ctx.gpr[3] = d;
        if (d != 0) VCall(ctx, p, 40, {p, on, 1});
    };
    sync(a);
    const u32 c = load<u32>(o + 204);
    if (c != 0) sync(c);
}

// void f(T* o, bool on): with a source +24: on -> every part of the
// u16-counted list +164 (count +172) and then the source get vt[40](it,
// on, 0); off -> the count is cleared and o->vt[28](o, source, 0) (vtable
// read before the store). r3 as left.
DECOMP(0036E750, Obj_SetPartsOn_0036E750) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 s = load<u32>(o + 24);
    ctx.gpr[3] = s;
    if (s == 0) return;
    if (on == 0) {
        const u32 vt = load<u32>(o);
        store<u16>(o + 172, 0);
        VCallVt(ctx, vt, 28, {o, s, 0});
        return;
    }
    for (u32 i = 0; i < load<u16>(o + 172); ++i) {
        const u32 p = load<u32>(load<u32>(o + 164) + 4 * i);
        VCall(ctx, p, 40, {p, on, 0});
    }
    const u32 s2 = load<u32>(o + 24);
    VCall(ctx, s2, 40, {s2, on, 0});
}

// bool f(Vec* v): 0 when any entry's object (**(data +0)[i], count u16 +8)
// projects (View_ProjectPoint(view, &xy, obj + 144), view = the word at
// g + 0x128168, g = *0x00F65CA0) to x and y within [-0.5, 1.5] (the f32s
// at 0x003EDAFC); 1 otherwise. The count is re-read after each
// projection.
DECOMP(003EDB08, Vec_AnyOnScreen_003EDB08) {
    const u32 v = arg<u32>(ctx, 0);
    u32 n = load<u16>(v + 8);
    const f64 lo = Lfs32(0x003EDAFCu), hi = Lfs32(0x003EDB00u);
    Stack frame(ctx, 16);
    const u32 xy = frame.addr();  // sp+112
    for (u32 i = 0; i < n; ++i) {
        const u32 q = load<u32>(load<u32>(load<u32>(v) + 4 * i));
        if (q == 0) continue;
        call(ctx, 0x00522750, {load<u32>(load<u32>(0x00F65CA0u) + 0x128168u), xy, q + 144});
        n = load<u16>(v + 8);
        const f64 x = Lfs32(xy), y = Lfs32(xy + 4);
        ctx.fpr[1] = x;
        if (x < lo || x > hi) continue;
        ctx.fpr[1] = y;
        if (y < lo || y > hi) continue;
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = 1;
}

// void f(u32 h; f32 v): for a live voice handle h (as
// Voice_SetParamByHandle_0044A78C checks it): a zeroed 220-byte parameter
// block (memset; size +0 = 220, flags 0x20 of +4 and 0x04 of +40, +76 =
// 1, f32 +84 = v, f32 +88 = the default at 0x010B03A8) sent with
// 0x006D612C(voice, &block). r3 as left, f1 the default when sent.
DECOMP(0044CCF8, Voice_SetVolumeByHandle_0044CCF8) {
    const u32 h = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0);
    if (load<u8>(0x010AEC60u) == 0) return;  // r3 = h
    const u32 lo = h & 0xFFFFu, hi = h >> 16;
    ctx.gpr[3] = hi;
    if (lo == 0) return;
    constexpr u32 tab = 0x010B0200u;
    if (!(static_cast<s32>(hi) < static_cast<s32>(load<u32>(tab + 24)))) return;
    const u32 e = load<u32>(tab + 28) + 272u * hi;
    ctx.gpr[3] = e + 256;
    if (((load<u32>(e + 256) >> 15) & 0xFFFFu) != lo) return;
    ctx.gpr[3] = 1;
    if ((load<u8>(e + 256) & 0x80u) != 0) return;
    Stack frame(ctx, 224);
    const u32 b = frame.addr();  // sp+112
    call(ctx, 0x00726E54, {b, 0, 220});
    const u32 w4 = load<u32>(b + 4) | 0x20u, w40 = load<u32>(b + 40) | 0x4u;
    store<u32>(b, 220);
    Stfs32(b + 84, v);
    store<u32>(b + 4, w4);
    store<u32>(b + 40, w40);
    store<u32>(b + 76, 1);
    store<u32>(b + 88, load<u32>(0x010B03A8u));
    ctx.fpr[1] = Lfs32(0x010B03A8u);
    call(ctx, 0x006D612C, {load<u32>(e + 16), b});
}

// s16 f(T* o): Rec_Reset_004B42CC's twin for "..." (0x00AEE2F8): the same
// index lookup, the same fields in a different store order, the u8 +20
// getting 0x10 (0xE4 cleared) and the s16 +64 = -1 (+60 = 0). r3 = the
// index.
DECOMP(004B4AEC, Rec_Reset_004B4AEC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 hs = static_cast<u32>(call(ctx, 0x0056A710, {0x00AEE2F8u}));
    s32 idx = -1;
    for (u32 i = 0; i < 5; ++i) {
        if (load<u32>(0x00F65F88u + 20 * i) == hs) {
            idx = static_cast<s32>(i);
            break;
        }
    }
    if (idx < 0) call(ctx, 0x005534D0, {0x00AEE4FCu, 0x00AEE2F8u});
    const u32 b = load<u8>(o + 20);
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 24, 0x3E800000u);  // 0.25f
    store<u32>(o + 28, 0x3F800000u);  // 1.0f
    store<u16>(o + 54, 0);
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    store<u32>(o + 32, 0);
    store<u8>(o + 21, 0);
    store<u16>(o + 52, 0xFFFF);
    store<u16>(o, static_cast<u16>(idx));
    store<u16>(o + 56, 0xFFFF);
    store<u16>(o + 58, 0xFFFF);
    store<u16>(o + 64, 0xFFFF);
    store<u8>(o + 20, static_cast<u8>((b & ~0xF4u) | 0x10u));
    store<u32>(o + 60, 0);
    ret(ctx, idx);
}

// void f(T* o): TreeSeek35 on the tree 0x010C27B8 (current 0x011DE490),
// the found node's object getting vt[56].
DECOMP(004B6894, Tree_SeekA_004B6894) { TreeSeek35(ctx, 0x010C27B8u, 0x011DE490u, 56); }

// void f(T* o): TreeSeek35 on the tree 0x010C2820 (current 0x011DE4A0),
// the found node's object getting vt[24].
DECOMP(004B6C88, Tree_SeekB_004B6C88) { TreeSeek35(ctx, 0x010C2820u, 0x011DE4A0u, 24); }
