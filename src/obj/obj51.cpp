// Game code, part 51: the small-function sweep continued from the remaining
// 112 instruction functions (size order; obj50*.cpp did the ones before).
// Each was checked against the original in the PPC simulator (memory
// writes, calls and their arguments, r3 and f1).
#include "obj51.h"

using namespace sly4;
using namespace sly4::obj51;

// ---------------------------------------------------------------- sweep: 112 instructions
// T* f(XmlCursor* doc, XmlCursor* x, u8 flag): without the attribute named
// at 0x00AD4C0C only XmlCursor_DocPath(doc) (its message dropped) and r3 =
// 0. Otherwise a Rec28New51 record (kind 33, vtable 0x00B1CB38, +24 = 0):
// u8 +16 = flag and the hashes 0x4D0C3326 / 0x6EADF2D8 read into +20 / +24
// (XmlCursor_ReadHash, the old values as defaults). r3 = the record (a
// failed allocation still writes u8 [16] and reads [20] / [24] at 0, as
// the original does).
DECOMP(002AF0D4, Rec28_ReadXml_002AF0D4) {
    const u32 doc = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 2);
    const u32 attr = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AD4C0Cu}));
    if (attr == 0) {
        call(ctx, 0x004F8944u, {doc});
        ret(ctx, 0u);
        return;
    }
    const u32 p = Rec28New51(ctx, attr, 33, 0x00B1CB38u, true);
    store<u8>(p + 16, static_cast<u8>(flag));
    call(ctx, 0x004FA14Cu, {x, 0x4D0C3326u, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004FA14Cu, {x, 0x6EADF2D8u, p + 24, load<u32>(p + 24)});
    ret(ctx, p);
}

// void f(T* o): Dtor4Vec51 with the vtables 0x00B1CCD8 / 0x00B1CD24.
DECOMP(002B2A70, Dtor_B1CCD8_002B2A70) { Dtor4Vec51(ctx, 0x00B1CCD8u, 0x00B1CD24u); }

// void f(T* o): Dtor4Vec51 with the vtables 0x00B1CDB0 / 0x00B1CDFC.
DECOMP(002B50D4, Dtor_B1CDB0_002B50D4) { Dtor4Vec51(ctx, 0x00B1CDB0u, 0x00B1CDFCu); }

// u32 f(Loader* o, a, b): in state 4 (u8 o->18) the 32-byte entries (o->2576,
// u16 count o->2584) are counted by the mode in the top 6 bits of o->2572
// (re-read per entry): 0 = entry->20 set with ->68 <= 1, 1 = ->68 == 1, 2 =
// bit 0x00400000 of ->124, 3 = ->68 == 3, 4 = every set ->20, each released
// (Obj_Release_002B719C). All of them counted (and at least one) ->
// o->vslot 104(o, 5, 0). Then r3 = Loader_Step_003252D8(o, a, b).
DECOMP(00330174, Loader_CountEntries_00330174) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    if (load<u8>(o + 18) == 4) {
        u32 cnt = 0, i = 0, off = 0, n;
        for (;;) {
            n = load<u16>(o + 2584);
            if (!(i < n)) break;
            const u32 ent = load<u32>(o + 2576) + off;
            const u32 mode = load<u32>(o + 2572) >> 26;
            const u32 e = load<u32>(ent + 20);
            switch (mode) {
            case 4:
                if (e != 0) {
                    call(ctx, 0x002B719Cu, {e});
                    ++cnt;
                }
                break;
            case 3:
                if (e != 0 && load<u32>(e + 68) == 3) ++cnt;
                break;
            case 2:
                if (e != 0 && (load<u32>(e + 124) & 0x00400000u) != 0) ++cnt;
                break;
            case 1:
                if (e != 0 && load<u32>(e + 68) == 1) ++cnt;
                break;
            case 0:
                if (e != 0 && !(static_cast<s32>(load<u32>(e + 68)) > 1)) ++cnt;
                break;
            default:
                break;
            }
            off += 32;
            ++i;
        }
        if (cnt != 0 && cnt == n) VCall(ctx, o, 104, {o, 5, 0});
    }
    call(ctx, 0x003252D8u, {o, a, b});
}

// bool f(?, const char* path): loads the XMLB file (XmlDoc on the stack,
// XmlDoc_LoadXmlb(doc, path, 0)); fails (with the messages 0x00ADD9A8 (path
// + 4), 0x00ADD8F8 / 0x00ADD950 (names, XmlCursor_DocPath) through
// String_Format) without the file or without the children 0x00ADD8D8 ->
// 0x00ADD940. Each child 0x00ADD940 (Xml_FindChildValue_004F9078 for the
// next) goes to KListLib_Add_00390DC8(*0x00F65798, &rec + 4, &rec) with a
// 32-byte record {u32 0, u16 0, u8 0, u8 1, .., u32 0, u8 1, .., node, u32
// 0, u16 64, u16 0, u32 0}. XmlDoc_Destruct; r3 = 1 (0 on failure).
DECOMP(0039129C, KList_LoadXmlb_0039129C) {
    const u32 path = arg<u32>(ctx, 1);
    Stack fr(ctx, 2272);  // the record at sp+112..143, the XmlDoc from sp+144 (frame of 2416 bytes)
    const u32 rec = fr.addr(), doc = fr.addr() + 32;
    call(ctx, 0x004FA7A4u, {doc});
    if (static_cast<u32>(call(ctx, 0x004FACCCu, {doc, path, 0})) == 0) {
        call(ctx, 0x005534D0u, {0x00ADD9A8u, path + 4});
        call(ctx, 0x004FA9C0u, {doc});
        ret(ctx, 0u);
        return;
    }
    constexpr u32 kTop = 0x00ADD8D8u, kItem = 0x00ADD940u;
    u32 ok = 0;
    u32 c = static_cast<u32>(call(ctx, 0x004F936Cu, {doc, kTop}));
    if (c == 0) {
        const u32 dp = static_cast<u32>(call(ctx, 0x004F8944u, {doc}));
        call(ctx, 0x005534D0u, {0x00ADD8F8u, kTop, dp});
    } else {
        c = static_cast<u32>(call(ctx, 0x004F936Cu, {c, kItem}));
        if (c == 0) {
            const u32 dp = static_cast<u32>(call(ctx, 0x004F8944u, {doc}));
            call(ctx, 0x005534D0u, {0x00ADD950u, kItem, kTop, dp});
        } else {
            do {
                store<u8>(rec + 4, 0);
                store<u32>(rec + 8, 0);
                store<u8>(rec + 12, 1);
                store<u32>(rec + 20, 0);
                store<u16>(rec + 24, 64);
                store<u16>(rec + 26, 0);
                store<u32>(rec + 28, 0);
                store<u8>(rec + 5, 1);
                store<u32>(rec, 0);
                store<u32>(rec + 16, c);
                store<u16>(rec + 2, 0);
                call(ctx, 0x00390DC8u, {load<u32>(0x00F65798u), rec + 4, rec});
                c = static_cast<u32>(call(ctx, 0x004F9078u, {c, kItem}));
            } while (c != 0);
            ok = 1;
        }
    }
    call(ctx, 0x004FA9C0u, {doc});
    ret(ctx, ok);
}

// Entry address in the 128-byte table *(0x00F65038)->24 for the s16 index
// at ea (0 for -1).
static u32 Ent128_51(u32 ea) {
    const s32 i = static_cast<s16>(load<u16>(ea));
    if (i == -1) return 0;
    return load<u32>(load<u32>(0x00F65038u) + 24) + (static_cast<u32>(i) << 7);
}

// bool f(Obj* o, Msg* m): with m->12 set and bit 0x8 of m->12->192: the
// entries e4 / e2 of o's s16 indices +4 / +2 (Ent128_51; e2' = e2 + u16
// *(0x0106CF7E) when u8 e2->2 is set, else 0) give the key word at e4 + u16
// *(0x0106CF70) + 4 * u8 e2'[0]; List_FindAfter_000D92B0(m->12, key, 0)
// finds the target t (r3 = 0 without one), which gets t->vslot 44(t, b, b)
// with b = u8 at e4 + u16 *(0x0106CF70) + 4 * u8 e2'[1] (indices and
// entries looked up again after the call). r3 = 1.
DECOMP(003963E0, Obj_SendTableByte_003963E0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 l = load<u32>(m + 12);
    if (l == 0 || (load<u32>(l + 192) & 0x8u) == 0) {
        ret(ctx, 0u);
        return;
    }
    {
        const u32 e4 = Ent128_51(o + 4);
        const u32 e2 = Ent128_51(o + 2);
        const u32 q = load<u8>(e2 + 2) > 0 ? e2 + load<u16>(0x0106CF7Eu) : 0;
        const u32 k = load<u8>(q);
        const u32 key = load<u32>(e4 + load<u16>(0x0106CF70u) + (k << 2));
        const u32 t = static_cast<u32>(call(ctx, 0x000D92B0u, {l, key, 0}));
        if (t == 0) {
            ret(ctx, 0u);
            return;
        }
        const u32 e4b = Ent128_51(o + 4);
        const u32 e2b = Ent128_51(o + 2);
        const u32 base = load<u16>(0x0106CF70u);
        const u32 q2 = load<u8>(e2b + 2) > 0 ? e2b + load<u16>(0x0106CF7Eu) : 0;
        const u32 opd = load<u32>(load<u32>(t) + 44);
        const u32 b = load<u8>(e4b + base + (static_cast<u32>(load<u8>(q2 + 1)) << 2));
        // the OPD register is overwritten by the byte (r4 = r5 = b, r6 = entry)
        call_ptr(ctx, opd, {t, b, b, load<u32>(opd)});
    }
    ret(ctx, 1u);
}

// bool f(T* o, u32 h, u32 h2, u8 slot, f32 t, ?, ?, u8 force): unless h is
// already o->112 (and no force): o->112 = h, u8 o->164 = 0, and the
// animations h / h2 restarted on o->96 / o->100 (when set and their vslot
// 60 answers; Anim_Restart2775A8(p->vslot 60(p), h, 0, slot, -1, 0, 0, 0,
// .., .., 0, -1; f1 = t, f2..f4 = the .text pool 0x003AB0AC [0], [1],
// [1])). r3 = 1 (0 when nothing changed).
DECOMP(003AB0B8, AnimPair_SetHash_003AB0B8) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), h2 = arg<u32>(ctx, 2);
    const u32 slot = arg<u32>(ctx, 3) & 0xFFu;
    const f64 t = ctx.fpr[1];
    if (load<u32>(o + 112) == h && (arg<u32>(ctx, 5) & 0xFFu) == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o + 112, h);
    store<u8>(o + 164, 0);
    constexpr u32 K = 0x003AB0ACu;
    if (const u32 p = load<u32>(o + 96); p != 0 && static_cast<u32>(VCall(ctx, p, 60, {p})) != 0) {
        const u32 p2 = load<u32>(o + 96);
        const u32 a = static_cast<u32>(VCall(ctx, p2, 60, {p2}));
        callf(ctx, 0x002775A8u, {a, h, 0, slot, ~0ull, 0, 0, 0, 0, 0, 0, ~0ull}, {t, Lfs51(K), Lfs51(K + 4), Lfs51(K + 4)});
    }
    if (const u32 p = load<u32>(o + 100); p != 0 && static_cast<u32>(VCall(ctx, p, 60, {p})) != 0) {
        const u32 p2 = load<u32>(o + 100);
        const u32 a = static_cast<u32>(VCall(ctx, p2, 60, {p2}));
        callf(ctx, 0x002775A8u, {a, h2, 0, slot, ~0ull, 0, 0, 0, 0, 0, 0, ~0ull}, {t, Lfs51(K), Lfs51(K + 4), Lfs51(K + 4)});
    }
    ret(ctx, 1u);
}

// void f(T* o, Cfg* c): u8 c->44 = 1; a new 192-byte object n (Mem_Alloc(192,
// 0, 16); VtB21988_Ctor(n), +92 = 0, vtable 0x00B20710, PtrVectors +112 /
// +144 / +164 (capacity 4), +132 = +136 = +140 = +184 = 0, u8 +188 = u8 +4
// = 0) set up by 0x003EE968(n, c), u8 n->188 = 1, its position +96..+104
// from the spline point at c->8 (WorldObj_SplinePointAt(*SplineMap_Find(
// *(0x00F8F158)->16, &o->68), .., &v, 0)), then pushed on o's PtrVector
// +112 (PtrVec_Push50); o->156 = c->28 when u8 c->24 is set. r3 = the
// vector's old high-water count, f1 = c->28 (a failed allocation writes
// [188] at 0, as the original does).
DECOMP(003F3F50, SplineObj_Spawn_003F3F50) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    store<u8>(c + 44, 1);
    u32 n = static_cast<u32>(call(ctx, 0x0054ED38u, {192, 0, 16}));
    if (n != 0) {
        call(ctx, 0x004558BCu, {n});
        store<u32>(n + 92, 0);
        store<u32>(n, 0x00B20710u);
        call(ctx, 0x005516E0u, {n + 112});
        store<u32>(n + 132, 0);
        store<u32>(n + 136, 0);
        store<u16>(n + 118, 4);
        store<u32>(n + 140, 0);
        call(ctx, 0x005516E0u, {n + 144});
        store<u16>(n + 150, 4);
        call(ctx, 0x005516E0u, {n + 164});
        store<u32>(n + 184, 0);
        store<u16>(n + 170, 4);
        store<u8>(n + 188, 0);
        store<u8>(n + 4, 0);
    }
    call(ctx, 0x003EE968u, {n, c});
    store<u8>(n + 188, 1);
    Stack fr(ctx, 32);  // the point at sp+112, the key at sp+128
    const u32 v = fr.addr(), key = fr.addr() + 16;
    store<u32>(key, load<u32>(o + 68));
    const u32 pv = static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), key}));
    // r4 as SplineMap_Find leaves it (the callee does not read it)
    callf(ctx, 0x00592748u, {load<u32>(pv), ctx.gpr[4], v, 0}, {Lfs51(c + 8)});
    Cp50(n + 96, v);
    Cp50(n + 100, v + 4);
    Cp50(n + 104, v + 8);
    const u32 hi = PtrVec_Push50(ctx, o + 112, n);
    const u32 f = load<u8>(c + 24);
    ctx.fpr[1] = Lfs51(c + 28);
    if (f != 0) Cp50(o + 156, c + 28);
    ret(ctx, hi);
}

// u32 f(T* o, f32 dt): o->84 += dt (scaled by the .text pool 0x0041AB0C [3]
// when u8 o->164 is set); when Anim_TrackSet36_0029879C(o->4->vslot
// 60(o->4), 0, 0) answers: o->84 = 0, o->88 = o->48, o->80 = 2 and the
// animation 0x75FAF7BB (0x7A143360 when u8 o->164 is set) restarted on
// o->4->vslot 60(o->4) (Anim_Restart2775A8(.., h, 0, 0, -1, 0, 0, 0, ..,
// .., 0, -1; f1..f4 = pool [0], [2], [1], [1])). r3 = the last call's.
DECOMP(0041AB20, Anim_TickBlend_0041AB20) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x0041AB0Cu;
    const u32 a = load<u32>(o + 4);
    f64 dt = ctx.fpr[1];
    const f64 cur = Lfs51(o + 84);
    if (load<u8>(o + 164) != 0) dt = F51(dt * Lfs51(K + 12));
    const f64 s = F51(cur + dt);
    store<f32>(o + 84, static_cast<f32>(s));
    ctx.fpr[1] = s;
    const f64 k0 = Lfs51(K), k1 = Lfs51(K + 4);
    const u32 tr = static_cast<u32>(VCall(ctx, a, 60, {a}));
    if (static_cast<u32>(call(ctx, 0x0029879Cu, {tr, 0, 0})) == 0) return;
    store<u32>(o + 84, 0);
    Cp50(o + 88, o + 48);
    store<u32>(o + 80, 2);
    const u32 h = load<u8>(o + 164) == 0 ? 0x75FAF7BBu : 0x7A143360u;
    const f64 k2 = Lfs51(K + 8);
    const u32 a2 = load<u32>(o + 4);
    const u32 tr2 = static_cast<u32>(VCall(ctx, a2, 60, {a2}));
    callf(ctx, 0x002775A8u, {tr2, h, 0, 0, ~0ull, 0, 0, 0, 0, 0, 0, ~0ull}, {k0, k2, k1, k1});
}

// Vec4_Transform(v, m) then v *= pool / v.w (each fmuls rounded).
static void XformDiv51(PPUContext& ctx, u32 v, u32 m, f64 k) {
    call(ctx, 0x005539E4u, {v, m});
    const f64 w = Lfs51(v + 12);
    const f64 s = F51(k / w);
    const f64 x = F51(Lfs51(v) * s), y = F51(Lfs51(v + 4) * s), z = F51(Lfs51(v + 8) * s);
    store<f32>(v, static_cast<f32>(x));
    store<f32>(v + 4, static_cast<f32>(y));
    store<f32>(v + 8, static_cast<f32>(z));
    store<f32>(v + 12, static_cast<f32>(F51(w * s)));
}

// void f(Vec3* out, ?, Obj* src): the view point (camera block 0x010AA210):
// without src its +80..+88; with *(0x010AA1B4) clear the hit of the line
// from src->144 on the plane (Plane_LineHit_004A6C50(blk, &p, blk + 96,
// &hit); +80..+88 again when it misses); otherwise src->144 projected
// (w = 1; transformed by blk + 176 and blk + 112, each divided by w times
// the .text pool 0x004286D0) and scaled by blk->88 / z. r3 / f1 as the
// original leaves them.
DECOMP(004286D8, Proj_ViewPoint_004286D8) {
    const u32 out = arg<u32>(ctx, 0), src = arg<u32>(ctx, 2);
    constexpr u32 B = 0x010AA210u;
    auto def = [&] {
        Cp50(out, B + 80);
        Cp50(out + 4, B + 84);
        Cp50(out + 8, B + 88);
        ctx.fpr[1] = Lfs51(B + 88);
    };
    if (src == 0) {
        def();
        ret(ctx, out);
        return;
    }
    Stack fr(ctx, 48);  // the point at sp+112, the hit at sp+128, the vec4 at sp+144
    const u32 p = fr.addr(), hit = fr.addr() + 16, v = fr.addr() + 32;
    const u32 x = load<u32>(src + 144), y = load<u32>(src + 148), z = load<u32>(src + 152);
    store<u32>(p, x);
    store<u32>(p + 4, y);
    store<u32>(p + 8, z);
    if (load<u32>(0x010AA1B4u) == 0) {
        if (static_cast<u32>(call(ctx, 0x004A6C50u, {B, p, B + 96, hit})) == 0) {
            def();
            return;
        }
        Cp50(out, hit);
        Cp50(out + 4, hit + 4);
        Cp50(out + 8, hit + 8);
        ctx.fpr[1] = Lfs51(hit + 8);
        return;
    }
    store<u32>(v, x);
    store<u32>(v + 4, y);
    store<u32>(v + 8, z);
    store<u32>(v + 12, 0x3F800000u);
    const f64 k = Lfs51(0x004286D0u);
    XformDiv51(ctx, v, B + 176, k);
    XformDiv51(ctx, v, B + 112, k);
    const f64 vz = Lfs51(v + 8);
    const f64 s = F51(Lfs51(B + 88) / vz);
    const f64 ox = F51(Lfs51(v) * s), oy = F51(Lfs51(v + 4) * s), oz = F51(vz * s);
    store<f32>(out, static_cast<f32>(ox));
    store<f32>(out + 4, static_cast<f32>(oy));
    store<f32>(out + 8, static_cast<f32>(oz));
    ctx.fpr[1] = oz;
    ret(ctx, B + 88);
}

// bool f(Rec* r, XmlCursor* x, u32* out): reads the hashes 0x00210FAA ->
// r->0 and 2683 -> r->4 (both required), the f32 0x95FB67C0 -> r->8
// (XmlCursor_ReadF32, default the .text pool 0x004B132C; must answer 0),
// the kind 0xDB8BAEEB -> u8 r->12 (0x85D98325 -> 1, 0x0026E201 -> 2, else
// 0) and the attribute 0x91EEB2F8 -> r->20 (copied with StrBuf_Append(
// *(0x00F65F84), s)) with its String_HashNoCase in r->16 (0 without it).
// *out = r->0 << 16 | r->4 >> 16; r3 = 1 (0 on failure).
DECOMP(004B1330, XmlRec_ReadKeys_004B1330) {
    const u32 r = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    call(ctx, 0x004FA14Cu, {x, 0x00210FAAu, r, load<u32>(r)});
    if (load<u32>(r) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004FA14Cu, {x, 2683, r + 4, load<u32>(r + 4)});
    if (load<u32>(r + 4) == 0) {
        ret(ctx, 0u);
        return;
    }
    callf(ctx, 0x004F9B20u, {x, 0x95FB67C0u, r + 8}, {Lfs51(0x004B132Cu)});
    if (static_cast<u32>(ctx.gpr[3]) != 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 16);  // the kind at sp+112
    call(ctx, 0x004FA14Cu, {x, 0xDB8BAEEBu, fr.addr(), 0});
    const u32 k = load<u32>(fr.addr());
    store<u8>(r + 12, k == 0x85D98325u ? 1 : k == 0x0026E201u ? 2 : 0);
    const u32 a = static_cast<u32>(call(ctx, 0x004F9534u, {x, 0x91EEB2F8u, 0x91EEB2F8u}));
    u32 s = 0, h = 0;
    if (a != 0) {
        store<u32>(r + 20, a);
        s = static_cast<u32>(call(ctx, 0x00566F1Cu, {load<u32>(0x00F65F84u), a}));
    }
    store<u32>(r + 20, s);
    if (s != 0) h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
    store<u32>(r + 16, h);
    store<u32>(out, (load<u32>(r) << 16) | (load<u32>(r + 4) >> 16));
    ret(ctx, 1u);
}

// bool f(XmlCursor* x): with the list owner *(0x010C2C28) (r3 = 0 without
// it; 1 at once when its u8 +586 is set) the attribute 0x00AF0120 (r3 = 0
// without it) is copied (Mem_Alloc(len + 1, 0, 16) + String_Assign; or,
// with u8 *(0x010C5AC3) set, String_Dup of String_Printf(buf, 0x00AF01F4,
// s, 0x00AF01FC)) into a new 32-byte entry {s, .., .., .., 0, 0, 0}
// (Mem_Alloc(32, 0, 16)) pushed on the owner's PtrVector +296
// (PtrVec_PushNm50). r3 = 1.
DECOMP(004EDFA4, StrList_AddXmlAttr_004EDFA4) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 w = load<u32>(0x010C2C28u);
    if (w == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(w + 586) != 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 s = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AF0120u}));
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 str;
    if (load<u8>(0x010C5AC3u) == 0) {
        const u32 len = static_cast<u32>(call(ctx, 0x00555FA4u, {s}));
        str = static_cast<u32>(call(ctx, 0x0054ED38u, {len + 1, 0, 16}));
        call(ctx, 0x00555F80u, {str, s});
        const u32 len2 = static_cast<u32>(call(ctx, 0x00555FA4u, {s}));
        store<u8>(str + len2, 0);
    } else {
        Stack fr(ctx, 256);  // the buffer at sp+112 (frame of 400 bytes)
        call(ctx, 0x00551540u, {fr.addr(), 0x00AF01F4u, s, 0x00AF01FCu});
        str = static_cast<u32>(call(ctx, 0x00556038u, {fr.addr()}));
    }
    const u32 e = static_cast<u32>(call(ctx, 0x0054ED38u, {32, 0, 16}));
    store<u32>(e, str);
    store<u32>(e + 24, 0);
    store<u32>(e + 20, 0);
    store<u32>(e + 16, 0);
    PtrVec_PushNm50(ctx, w + 296, e);
    ret(ctx, 1u);
}
