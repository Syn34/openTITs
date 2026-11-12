// Game code, part 52: the small-function sweep continued with the remaining
// 115 to 117 instruction functions (size order; 0x00121968, a 118, sits
// with its 116 twin). Each was checked against the
// original in the PPC simulator (memory writes, calls and their arguments,
// r3 and f1).
#include "obj52.h"

using namespace sly4;
using namespace sly4::obj52;

// ---------------------------------------------------------------- sweep: 115 instructions
// u16* f(u32 key, SkeletonDef* sk): the entry of key in the sorted map
// 0x0106BC14 (SortedMap_Find76D894(map, &key), key at sp+112); none ->
// String_Format(0x00ADC070, Hash_ToString(key)) when key is set, r3 = 0.
// Otherwise a bone remap table: Mem_Alloc(2 * n + 2, 0, 16) with n = u16
// sk->+2 (not checked for 0), u16 [0] = n, [1..n] = 0xFFFF; then for each of
// the e->+28 names (8-byte entries at e->+24, low bit of the pointer
// cleared; past the ':' when String_Thunk71EDAC(s, ':') finds one followed
// by more than one character) SkeletonDef_FindBone(sk,
// String_HashNoCase(name)) != -1 -> u16 [1 + bone] = the name's index. r3 =
// the table.
DECOMP(0034B0F8, BoneRemap_Build_0034B0F8) {
    const u32 key = arg<u32>(ctx, 0), sk = arg<u32>(ctx, 1);
    Stack fr(ctx, 16);  // the key at sp+112
    store<u32>(fr.addr(), key);
    const u32 e = load<u32>(static_cast<u32>(call(ctx, 0x0076D894u, {0x0106BC14u, fr.addr()})));
    if (e == 0) {
        if (key != 0) {
            const u32 s = static_cast<u32>(call(ctx, 0x0056A6E0u, {key}));
            call(ctx, 0x005534D0u, {0x00ADC070u, s});
        }
        ret(ctx, 0u);
        return;
    }
    u32 n = load<u16>(sk + 2);
    const u32 cnt = load<u32>(e + 28);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {(n << 1) + 2, 0, 16}));
    store<u16>(p, static_cast<u16>(n));
    if (n != 0) {
        u32 q = p;
        do {
            q += 2;
            --n;
            store<u16>(q, 0xFFFFu);
        } while (static_cast<s32>(n) > 0);
    }
    for (u32 i = 0; i < cnt; ++i) {
        u32 s = load<u32>(load<u32>(e + 24) + 8 * i) & 0xFFFFFFFEu;
        const u32 c = static_cast<u32>(call(ctx, 0x00556014u, {s, 58}));
        if (c != 0 && static_cast<s32>(static_cast<u32>(call(ctx, 0x00555FA4u, {c}))) > 1) s = c + 1;
        const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {s}));
        const u32 b = static_cast<u32>(call(ctx, 0x00584888u, {sk, h}));
        if (b != 0xFFFFFFFFu) store<u16>(p + 2 + (b << 1), static_cast<u16>(i));
    }
    ret(ctx, p);
}

// void f(T* o) (constructor): PtrVectors at +0, +20, +40, +60
// (PtrVector_Construct, u16 +6 = 4 / +26 = 4 / +46 = 16 / +66 = 16 set
// before PtrVector_Init(v, 16, 0, 0)); three 76-byte records at +80 (+64,
// +68, +72 = 0, memset(+0, 0, 64)); the 4 bytes of 0x01164CA4 -> +308..+311,
// +312..+352 = 0, +356 bit 0x80000000 set and bits 0x78000000 cleared; then
// memset(o + 80, 0, 228) (r3 = its result).
DECOMP(0039CB9C, Obj_Construct_0039CB9C) {
    const u32 o = arg<u32>(ctx, 0);
    static constexpr u16 kGrow[4] = {4, 4, 16, 16};
    for (u32 k = 0; k < 4; ++k) {
        const u32 v = o + 20 * k;
        call(ctx, 0x005516E0u, {v});
        store<u16>(v + 6, kGrow[k]);
        call(ctx, 0x00551598u, {v, 16, 0, 0});
    }
    for (u32 k = 0; k < 3; ++k) {
        const u32 r = o + 76 * k + 80;
        store<u32>(r + 64, 0);
        store<u32>(r + 68, 0);
        store<u32>(r + 72, 0);
        call(ctx, 0x00726E54u, {r, 0, 64});
    }
    constexpr u32 G = 0x01164CA4u;
    store<u8>(o + 308, load<u8>(G));
    const u32 fl = load<u32>(o + 356);
    store<u8>(o + 309, load<u8>(G + 1));
    store<u8>(o + 310, load<u8>(G + 2));
    store<u32>(o + 312, 0);
    store<u8>(o + 311, load<u8>(G + 3));
    for (u32 off = 316; off <= 352; off += 4) store<u32>(o + off, 0);
    store<u32>(o + 356, (fl | 0x80000000u) & ~(0x40000000u | 0x20000000u | 0x10000000u | 0x08000000u));
    ret(ctx, call(ctx, 0x00726E54u, {o + 80, 0, 228}));
}

// void f(T* o): picks the next spline node. t = f32 (o->+56)->+48 and the
// spline (o->+56)->+12; with nodes (u16 o->+140 != 0) o->+256 = node 0
// and node 0 is the default pick. p = WorldObj_SplinePointAt(spline, t)
// (sp+112) and, among the nodes n (o->+132[i]) with -1 < n->+12 < t, the
// one whose point (sp+128, at n->+12) is nearest to p in xy while farther
// than 10.0 (and below 100000.0). Found: (o->+56)->+88 = n->+4 * 0.2 + n->+12 * 0.8, u8 +92
// = 1, o->+256 = n (r3). r4 goes to the spline calls as the caller / the
// previous callee left it (WorldObj_SplinePointAt reads r3, r5, r6, f1);
// r3 / f1 keep what the original leaves in them.
DECOMP(003F2990, Spline_PickNode_003F2990) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x003F2974u;  // 100000.0, -1.0, 10.0, 0.0, 0.8, 0.2
    const u32 w = load<u32>(o + 56);
    const f64 t = Lfs52(w + 48);
    const u32 spl = load<u32>(w + 12);
    s32 best = -1;
    if (load<u16>(o + 140) != 0) {
        best = 0;
        store<u32>(o + 256, load<u32>(load<u32>(o + 132)));
    }
    Stack fr(ctx, 32);  // the points at sp+112 / sp+128
    const u32 sp = fr.addr() - 112;
    callf(ctx, 0x00592748u, {spl, static_cast<u32>(ctx.gpr[4]), sp + 112, 0}, {t});
    f64 bd = Lfs52(K), lo = Lfs52(K + 4), mn = Lfs52(K + 8), z = Lfs52(K + 12);
    bool own = false;
    f64 f1 = 0;
    u32 cnt;
    for (u32 i = 0;; ++i) {
        cnt = load<u16>(o + 140);
        if (!(i < cnt)) break;
        const u32 n = load<u32>(load<u32>(o + 132) + 4 * i);
        f1 = Lfs52(n + 12);
        own = true;
        if (!(f1 < t) || !(f1 > lo)) continue;
        callf(ctx, 0x00592748u, {load<u32>(load<u32>(o + 56) + 12), static_cast<u32>(ctx.gpr[4]), sp + 128, 0}, {f1});
        const f64 dy = F52(Lfs52(sp + 132) - Lfs52(sp + 116));
        const f64 dx = F52(Lfs52(sp + 128) - Lfs52(sp + 112));
        f64 d = F52(dy * dy);
        d = F52(std::fma(dx, dx, d));
        d = F52(std::fma(z, z, d));
        d = F52(std::sqrt(d));
        f1 = d;
        if (d > mn && d < bd) {
            best = static_cast<s32>(i);
            bd = d;
        }
    }
    if (best > -1) {
        const u32 slot = load<u32>(o + 132) + (static_cast<u32>(best) << 2);
        const u32 n = load<u32>(slot);
        const u32 w2 = load<u32>(o + 56);
        const f64 a = F52(Lfs52(n + 4) * Lfs52(K + 20));
        store<u8>(w2 + 92, 1);
        const f64 r = F52(std::fma(Lfs52(n + 12), Lfs52(K + 16), a));
        store<f32>(w2 + 88, static_cast<f32>(r));
        const u32 v = load<u32>(slot);
        store<u32>(o + 256, v);
        ctx.fpr[1] = r;
        ret(ctx, v);
        return;
    }
    if (own) ctx.fpr[1] = f1;
    ret(ctx, cnt);
}

// void f(T* o, Ref a, u32 b, u32 c, u8 d, u8 e, u8 spawn) (constructor):
// Obj_InitVt_B20CC8(o, e), vtable 0x00B20E90, +12 = a (Field_Inc20_00446170
// when set), +16 = b, +20 = c, u8 +24 = d, +28 = 0. With spawn set and +28
// still 0: +28 = List_FindByKey_004464F0(0x010ACC40, +12); when that finds
// none or u8 +24 is set, the 68-byte spawn record on the stack (vec3s
// 0x01164B40 at +0 / +16, +32 = 1.0f, the rest 0, +48 = b, +52 = c) goes to
// Call_446198_00446348(0x010ACC40, +12, rec, b, c, ...) -> +28, then
// SpawnRec_Dtor_00440AC4 / SpawnRec_Dtor2_00440A40(rec, 0) (r3 = its
// result; else r3 = +12).
DECOMP(0042ECB8, SpawnRef_Construct_0042ECB8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 d = arg<u32>(ctx, 4), e = arg<u32>(ctx, 5), sp_ = arg<u32>(ctx, 6);
    // r5..r7 still hold b, c, d (the callee reads r3 / r4)
    call(ctx, 0x0042C9D0u, {o, e & 0xFFu, b, c, d});
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    store<u32>(o, 0x00B20E90u);
    store<u8>(o + 24, static_cast<u8>(d));
    store<u32>(o + 28, 0);
    u32 r3 = a, f = 0;
    if (a != 0) {
        call(ctx, 0x00446170u, {a});
        f = load<u32>(o + 28);
        r3 = load<u32>(o + 12);
    }
    if ((sp_ & 0xFFu) == 0 || f != 0) {
        ret(ctx, r3);
        return;
    }
    constexpr u32 L = 0x010ACC40u, V = 0x01164B40u;
    const u32 h = static_cast<u32>(call(ctx, 0x004464F0u, {L, r3}));
    store<u32>(o + 28, h);
    const u32 bb = load<u32>(o + 16), cc = load<u32>(o + 20), aa = load<u32>(o + 12);
    if (h != 0 && load<u8>(o + 24) == 0) {
        ret(ctx, aa);
        return;
    }
    Stack fr(ctx, 80);  // the record at sp+112..179 (frame of 240 bytes)
    const u32 rec = fr.addr(), sp = rec - 112;
    Cp52(rec, V);
    Cp52(rec + 4, V + 4);
    Cp52(rec + 8, V + 8);
    Cp52(rec + 16, V);
    Cp52(rec + 20, V + 4);
    Cp52(rec + 24, V + 8);
    store<u32>(rec + 32, 0x3F800000u);
    store<u32>(rec + 36, 0);
    store<u32>(rec + 48, 0);
    store<u32>(rec + 40, 0);
    store<u8>(rec + 44, 0);
    store<u32>(rec + 56, 0);
    store<u32>(rec + 60, 0);
    store<u8>(rec + 45, 0);
    store<u32>(rec + 64, 0);
    store<u8>(rec + 46, 0);
    store<u8>(rec + 47, 0);
    store<u32>(rec + 48, bb);
    store<u32>(rec + 52, cc);
    // f1..f3 hold the vec3; r8 = sp+2, r9 = 0x01164B40, r10 = 0 (the callee reads r3..r5)
    callf(ctx, 0x00446348u, {L, aa, rec, bb, cc, sp + 2, V, 0}, {Lfs52(V), Lfs52(V + 4), Lfs52(V + 8)});
    store<u32>(o + 28, static_cast<u32>(ctx.gpr[3]));
    call(ctx, 0x00440AC4u, {rec, 0});
    call(ctx, 0x00440A40u, {rec, 0});
}

// u32 f(T* o): a chance roll. Nothing (r3 = 0) when bit 0x80 of u8 o->+88
// is set. With v = the f32 (o->+48)->+12[(*0x010AB068)->+64] below the
// threshold 0x00455D5C, the shared random state (u64 seed 0x01164888, word
// 0x01164890; as obj49's roll) gives r = seed * 0x00455D60 and, when r *
// threshold > v, r3 = 0. Otherwise u8 o->+88 gets 0x80 set and 0x04
// cleared, Obj_RollValue_00455C78(o), and o is pushed on the slot list
// (*0x00F65BE8)->+52 (SlotList_PushFront_00785190(&handle, list, &o));
// unless the handle {u16 index, u16 gen} names a live entry (gen set, index
// below list->+76, entry (list->+80)[index] word +12 bits 15..30 == gen and
// bit 0x80 of its byte +12 clear) String_Format(0x00AE51C4,
// vt[12](o->+60)). r3 = 1.
DECOMP(00455D68, Obj_ChanceRoll_00455D68) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x00455D5Cu, kSeed = 0x01164888u, kState = 0x01164890u;
    u32 b = load<u8>(o + 88);
    if (b & 0x80u) {
        ret(ctx, 0u);
        return;
    }
    const f64 thr = Lfs52(K);
    const u32 ix = load<u32>(load<u32>(0x010AB058u + 16) + 64);
    const f64 v = Lfs52(load<u32>(load<u32>(o + 48) + 12) + (ix << 2));
    // f1 / f2 = threshold / v (then f1 = the roll) as the calls below see them
    ctx.fpr[1] = thr;
    ctx.fpr[2] = v;
    if (v < thr) {
        const f64 rnd = F52(static_cast<f64>(static_cast<s64>(load<u64>(kSeed))));  // fcfid + frsp
        u32 w = load<u32>(kState);
        w = std::rotl(w, 4) + std::rotl(w, 16);
        w ^= 0x87654321u;
        store<u32>(kState, std::rotl(w, 16));
        store<u64>(kSeed, w & 0xFFFFu);
        const f64 r = F52(F52(rnd * Lfs52(K + 4)) * thr);
        b = load<u8>(o + 88);
        ctx.fpr[1] = r;
        if (r > v) {
            ret(ctx, 0u);
            return;
        }
    }
    store<u8>(o + 88, static_cast<u8>((b | 0x80u) & ~0x04u));
    call(ctx, 0x00455C78u, {o});
    const u32 L = load<u32>(0x00F65BE8u);
    Stack fr(ctx, 16);  // o at sp+120, the handle at sp+124
    const u32 sp = fr.addr() - 112;
    store<u32>(sp + 120, o);
    call(ctx, 0x00785190u, {sp + 124, L + 52, sp + 120});
    const u32 gen = load<u16>(sp + 126);
    const u32 x = load<u32>(sp + 120);
    bool live = false;
    if (gen != 0) {
        const u32 idx = load<u16>(sp + 124);
        if (static_cast<s32>(idx) < static_cast<s32>(load<u32>(L + 76))) {
            const u32 d = load<u32>(L + 80);
            if (((load<u32>(d + ((idx << 4) & 0x000FFFF0u) + 12) >> 15) & 0xFFFFu) == gen)
                live = (load<u8>(d + (idx << 4) + 12) & 0x80u) == 0;
        }
    }
    if (!live) {
        const u32 q = load<u32>(x + 60);
        const u32 opd = load<u32>(load<u32>(q) + 12);
        const u32 s = static_cast<u32>(call_ptr(ctx, opd, {q, opd, load<u32>(opd)}));
        call(ctx, 0x005534D0u, {0x00AE51C4u, s});
    }
    ret(ctx, 1u);
}

// void f(Vec3* out, Mat4* m, ?, Vec2* v, f32 a, f32 z) (screen point to
// world): Matrix_Perspective(P (sp+208), ..., 0x010C5A20, v) with f1 = a,
// f2 = f32 0x010C5A30, f3 = z, f4 = z + 0x004A67E0; the view matrix V
// (sp+272) from m (rows 0 and 2 and the w of rows 0..2 negated with the f32
// 0x004A67E4, row 1 and the translation row 3 as they are, w 0 / 1.0f);
// Mat4_Invert(sp+144, V' (sp+336), V); Matrix_MultiplyTo(sp+400, V', P);
// Mat4_Invert(sp+144, M (sp+464), sp+400); out = Vec4_TransformByMatrix of
// {v.x * z, v.y * z, 0, z} by M (xyz). r4 goes to Matrix_Perspective as m
// (it reads r3 and f1..f4); f1 = out.z as the last lfs leaves it.
DECOMP(004A67E8, Screen_Unproject_004A67E8) {
    const u32 out = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), v = arg<u32>(ctx, 3);
    const f64 a = farg(ctx, 0), z = farg(ctx, 1);
    constexpr u32 K = 0x004A67E0u;
    Stack fr(ctx, 416);  // sp+112..527
    const u32 sp = fr.addr() - 112;
    callf(ctx, 0x0056A200u, {sp + 208, m, 0x010C5A20u, v}, {a, Lfs52(0x010C5A30u), z, F52(z + Lfs52(K))});
    const u32 nw = Neg52(load<u32>(K + 4));
    const u32 V = sp + 272;
    Cp52(V + 16, m + 16);
    Cp52(V + 20, m + 20);
    Cp52(V + 24, m + 24);
    store<u32>(V + 28, 0);
    Cp52(V + 48, m + 48);
    Cp52(V + 52, m + 52);
    Cp52(V + 56, m + 56);
    store<u32>(V + 60, 0x3F800000u);
    store<u32>(V, Neg52(load<u32>(m)));
    store<u32>(V + 4, Neg52(load<u32>(m + 4)));
    store<u32>(V + 8, Neg52(load<u32>(m + 8)));
    store<u32>(V + 12, nw);
    store<u32>(V + 32, Neg52(load<u32>(m + 32)));
    store<u32>(V + 36, Neg52(load<u32>(m + 36)));
    store<u32>(V + 40, Neg52(load<u32>(m + 40)));
    store<u32>(V + 44, nw);
    call(ctx, 0x00554C28u, {sp + 144, sp + 336, V});
    call(ctx, 0x0055499Cu, {sp + 400, sp + 336, sp + 208});
    call(ctx, 0x00554C28u, {sp + 144, sp + 464, sp + 400});
    const f64 x = F52(Lfs52(v) * z), y = F52(Lfs52(v + 4) * z);
    store<u32>(sp + 120, 0);
    store<f32>(sp + 124, static_cast<f32>(z));
    store<f32>(sp + 112, static_cast<f32>(x));
    store<f32>(sp + 116, static_cast<f32>(y));
    call(ctx, 0x00553910u, {sp + 128, sp + 112, sp + 464});
    Cp52(out, sp + 128);
    Cp52(out + 4, sp + 132);
    Cp52(out + 8, sp + 136);
    ctx.fpr[1] = Lfs52(sp + 136);
}

// Obj* f(Ctx* c, Node* n): n's object, made on first use: o = vt[32](n, 0)
// -> n->+84, then vt[8](o); then for each child in n's slot list (+12:
// head index +20, tail +24, count +28, 16-byte entries at +40 {+4 next, +8
// child, +12 bits 15..30 gen}; handles {u16 index, u16 gen}) the child's
// object (this function, recursively) gets u8 +32 = 1 and is appended to
// o's pointer array (+20 data, +28 count). An existing n->+84 is returned
// as it is. r3 = n->+84.
DECOMP(004F57CC, Node_BuildObj_004F57CC) {
    const u32 c = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    if (load<u32>(n + 84) == 0) {
        u32 opd = load<u32>(load<u32>(n) + 32);
        const u32 o = static_cast<u32>(call_ptr(ctx, opd, {n, 0, opd, load<u32>(opd)}));
        store<u32>(n + 84, o);
        opd = load<u32>(load<u32>(o) + 8);
        call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
        const u32 L = n + 12;
        u32 cnt = load<u32>(L + 16);
        u32 h = 0;
        if (cnt != 0) {
            const u32 hd = load<u32>(L + 8);
            const u32 g = (load<u32>(load<u32>(L + 28) + ((hd << 4) & 0xFFFFFFF0u) + 12) >> 15) & 0xFFFFu;
            h = ((hd & 0xFFFFu) << 16) | g;
        }
        if (static_cast<s32>(cnt) > 0) {
            for (s32 i = 0;;) {
                const u32 off = (h >> 12) & 0x000FFFF0u;
                const u32 ch = load<u32>(load<u32>(L + 28) + off + 8);
                const u32 co = static_cast<u32>(call(ctx, 0x004F57CCu, {c, ch}));
                const u32 p = load<u32>(n + 84);
                const u32 idx = h >> 16;
                store<u8>(co + 32, 1);
                store<u32>(load<u32>(p + 20) + (load<u32>(p + 28) << 2), co);
                store<u32>(p + 28, load<u32>(p + 28) + 1);
                const u32 tail = load<u32>(L + 12);
                cnt = load<u32>(L + 16);
                u32 nh = 0;
                if (static_cast<s32>(idx) != static_cast<s32>(tail)) {
                    const u32 d = load<u32>(L + 28);
                    const u32 nx = load<u32>(d + off + 4);
                    const u32 w = load<u32>(d + ((nx << 4) & 0xFFFFFFF0u) + 12);
                    nh = ((nx << 16) & 0xFFFF0000u) | ((w >> 15) & 0xFFFFu);
                }
                ++i;
                h = nh;
                if (!(i < static_cast<s32>(cnt))) break;
            }
        }
    }
    ret(ctx, load<u32>(n + 84));
}

// ---------------------------------------------------------------- sweep: 116 instructions
// T* f(?, XmlCursor* x): a 40-byte def (Mem_Alloc(40, 0, 16); r3 = 0 when
// that fails): Obj_InitVt_B21C98(d, x), vtable 0x00B0F518, +12..+32 = 0, u8
// +36..+38 = 0; the hashes 0xAB39AA94, 0xABB2A7F3, 0x0586D771, 0x26AA71F6,
// 0x6E6A93D5, 0x448AAB12 -> +12..+32 and the bools 0x7A281983, 0x04A4252F,
// 0x9C564761 -> u8 +36..+38 (default 0). r3 = d.
DECOMP(00072EA8, Def40_ReadXml_00072EA8) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 d = static_cast<u32>(call(ctx, 0x0054ED38u, {40, 0, 16}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {d, x});
    store<u32>(d + 12, 0);
    store<u32>(d + 16, 0);
    store<u32>(d + 20, 0);
    store<u32>(d, 0x00B0F518u);
    store<u32>(d + 24, 0);
    store<u32>(d + 28, 0);
    store<u32>(d + 32, 0);
    store<u8>(d + 36, 0);
    store<u8>(d + 37, 0);
    store<u8>(d + 38, 0);
    static constexpr u32 kHash[6] = {0xAB39AA94u, 0xABB2A7F3u, 0x0586D771u, 0x26AA71F6u, 0x6E6A93D5u, 0x448AAB12u};
    for (u32 k = 0; k < 6; ++k) call(ctx, 0x004FA14Cu, {x, kHash[k], d + 12 + 4 * k, 0});
    static constexpr u32 kBool[3] = {0x7A281983u, 0x04A4252Fu, 0x9C564761u};
    for (u32 k = 0; k < 3; ++k) call(ctx, 0x004F986Cu, {x, kBool[k], d + 36 + k, 0});
    ret(ctx, d);
}

// u32 f(Cmd* c, Effect* e) (sequence command): only when u8 c->+25 or u8
// c->+27 is set. The target t: with c->+12 set, the component
// Effect_FindComponent(e, c->+12) whose vt[32]() is 5 gives its ->+28 r and,
// when r->+276 == 0x02235C33, t = r->+280; else (and when c->+12 is clear)
// t = Obj_FindById(c->+16) when c->+16 is set (the Hash_ToString calls are
// the stripped error prints). With t, unless bit 0x8 of t->+192 is set
// (Hash_ToString of t->+168 and (t->+216)->+52), i = Table_Index1205AC(c->+20)
// (-1: Hash_ToString(c->+20)) and u8 t->+656 + i gets bit 0x40 = bit 0 of u8
// c->+24 (with c->+25) or else bit 0x80 = bit 0 of u8 c->+26 (with c->+27).
// r3 is what the original leaves in it.
DECOMP(00087A40, SeqCmd_SetFlagBits_00087A40) {
    const u32 c = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 r3 = load<u32>(c + 12);
    if (load<u8>(c + 25) == 0 && load<u8>(c + 27) == 0) {
        ret(ctx, r3);
        return;
    }
    u32 t = 0;
    bool have = false, printed = false;
    if (r3 != 0) {
        const u32 comp = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, r3}));
        r3 = load<u32>(c + 12);
        if (comp != 0) {
            const u32 opd = load<u32>(load<u32>(comp) + 32);
            const u32 k = static_cast<u32>(call_ptr(ctx, opd, {comp, opd, load<u32>(opd)}));
            r3 = load<u32>(c + 12);
            const u32 r = k == 5 ? load<u32>(comp + 28) : 0;
            if (r != 0) {
                if (load<u32>(r + 276) == 0x02235C33u) t = load<u32>(r + 280);
                if (t != 0) {
                    have = true;
                } else {
                    call(ctx, 0x0056A6E0u, {r3});
                    printed = true;
                }
            }
        }
    }
    if (!have) {
        if (!printed && r3 != 0) call(ctx, 0x0056A6E0u, {r3});
        r3 = load<u32>(c + 16);
        if (r3 != 0) {
            t = static_cast<u32>(call(ctx, 0x000D6A28u, {r3}));
            r3 = t;
            if (t == 0) r3 = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(c + 16)}));
        }
        if (t == 0) {
            ret(ctx, r3);
            return;
        }
    }
    if ((load<u32>(t + 192) & 0x8u) == 0) {
        call(ctx, 0x0056A6E0u, {load<u32>(t + 168)});
        ret(ctx, call(ctx, 0x0056A6E0u, {load<u32>(load<u32>(t + 216) + 52)}));
        return;
    }
    const u32 i = static_cast<u32>(call(ctx, 0x001205ACu, {load<u32>(c + 20)}));
    if (i == 0xFFFFFFFFu) {
        ret(ctx, call(ctx, 0x0056A6E0u, {load<u32>(c + 20)}));
        return;
    }
    const u32 b = t + i + 656;
    if (load<u8>(c + 25) != 0) {
        r3 = load<u8>(c + 24);
        store<u8>(b, static_cast<u8>((load<u8>(b) & ~0x40u) | ((r3 << 6) & 0x40u)));
    } else {
        r3 = load<u8>(c + 27);
        if (r3 != 0) {
            r3 = load<u8>(c + 26);
            store<u8>(b, static_cast<u8>((load<u8>(b) & ~0x80u) | ((r3 << 7) & 0x80u)));
        }
    }
    ret(ctx, r3);
}

// u32 f(Cmd* c, Effect* e) (sequence command, spawn burst): the target t as
// in 0x00087A40 (the component Effect_FindComponent(e, c->+36) with vt[32]()
// == 5 -> ->+28 r, t = r->+280 when r->+276 == 0x02235C33; else
// Obj_FindById(c->+40) when set; the Hash_ToString calls are stripped
// prints). No t: r3 as left. Otherwise the 48-byte record on the stack
// (Obj_Reset_004AE360; +0..+11 = the vec3 c->+16 unless it equals the
// vec3 0x01164B40, +16 = t, +20 = c->+44, +24 = c->+32, +28..+36 = c->+48 ..
// +56, u8 +44..+46 = u8 c->+60..+62) goes to SpawnQueue_Burst4AED50(rec)
// (r3 = its result); f1..f3 / r4..r10 hold the fields as the stores leave
// them (the callee reads r3).
DECOMP(00098EA4, SeqCmd_SpawnBurst_00098EA4) {
    const u32 c = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    u32 r3 = load<u32>(c + 36);
    u32 t = 0;
    bool have = false, printed = false;
    if (r3 != 0) {
        const u32 comp = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, r3}));
        r3 = load<u32>(c + 36);
        if (comp != 0) {
            const u32 opd = load<u32>(load<u32>(comp) + 32);
            const u32 k = static_cast<u32>(call_ptr(ctx, opd, {comp, opd, load<u32>(opd)}));
            r3 = load<u32>(c + 36);
            const u32 r = k == 5 ? load<u32>(comp + 28) : 0;
            if (r != 0) {
                if (load<u32>(r + 276) == 0x02235C33u) t = load<u32>(r + 280);
                if (t != 0) {
                    have = true;
                } else {
                    call(ctx, 0x0056A6E0u, {r3});
                    printed = true;
                }
            }
        }
    }
    if (!have) {
        if (!printed && r3 != 0) call(ctx, 0x0056A6E0u, {r3});
        r3 = load<u32>(c + 40);
        if (r3 != 0) {
            t = static_cast<u32>(call(ctx, 0x000D6A28u, {r3}));
            r3 = t;
            if (t == 0) r3 = static_cast<u32>(call(ctx, 0x0056A6E0u, {load<u32>(c + 40)}));
        }
        if (t == 0) {
            ret(ctx, r3);
            return;
        }
    }
    Stack fr(ctx, 48);  // the record at sp+112..159
    const u32 rec = fr.addr();
    call(ctx, 0x004AE360u, {rec});
    constexpr u32 V = 0x01164B40u;
    const bool same = load<u64>(c + 16) == load<u64>(V) && load<u32>(c + 24) == load<u32>(V + 8);
    const u32 f16 = load<u32>(c + 16), f20 = load<u32>(c + 20), f24 = load<u32>(c + 24);
    const u32 w32 = load<u32>(c + 32), w44 = load<u32>(c + 44), w48 = load<u32>(c + 48);
    const u32 w52 = load<u32>(c + 52), w56 = load<u32>(c + 56);
    const u32 b60 = load<u8>(c + 60), b61 = load<u8>(c + 61), b62 = load<u8>(c + 62);
    if (!same) {
        store<u32>(rec, f16);
        store<u32>(rec + 4, f20);
        store<u32>(rec + 8, f24);
    }
    store<u32>(rec + 24, w32);
    store<u32>(rec + 16, t);
    store<u32>(rec + 20, w44);
    store<u32>(rec + 28, w48);
    store<u32>(rec + 32, w52);
    store<u32>(rec + 36, w56);
    store<u8>(rec + 44, static_cast<u8>(b60));
    store<u8>(rec + 45, static_cast<u8>(b61));
    store<u8>(rec + 46, static_cast<u8>(b62));
    callf(ctx, 0x004AED50u, {rec, b60, w56, w52, w48, w44, w32, b61},
          {static_cast<f64>(std::bit_cast<f32>(f24)), static_cast<f64>(std::bit_cast<f32>(f20)),
           static_cast<f64>(std::bit_cast<f32>(f16))});
    ret(ctx, static_cast<u32>(ctx.gpr[3]));
}

// The query-built object pattern of 0x000B9738 / 0x00121968: v =
// o->vt[32](o, w); QueryEx_Construct_000DFDF8(q (sp+208), 4, 128, v, o, 0,
// cls, 0x01164BD0, 0x01164B30, 1, a11, 0, 1, 0, 1, 0, 0, 0, 0xFFFFFFFF); bits
// of q->+4 set; w->vt[8](w, q); p = SmallAlloc(size) or Mem_Alloc(size, 0,
// 16) -> ctor(p, q) when that works; p->vt[8](p) (also with p = 0, through
// address 0: kept); the query's PtrVectors +164 / +144 cleared, its lazy
// 20-byte PtrVector +0 cleared and freed. Returns p.
static u32 QueryBuildObj52(PPUContext& ctx, u32 cls, u32 a11, u32 bits, u32 alloc, bool small, u32 size, u32 ctor) {
    const u32 o = arg<u32>(ctx, 0), w = arg<u32>(ctx, 1);
    u32 opd = load<u32>(load<u32>(o) + 32);
    const u32 v = static_cast<u32>(call_ptr(ctx, opd, {o, w, opd, load<u32>(opd)}));
    Stack fr(ctx, 208);  // the query at sp+208..415 (call() makes its own room for the stack args)
    const u32 q = fr.addr();
    call(ctx, 0x000DFDF8u, {q, 4, 128, v, o, 0, cls, 0x01164BD0u, 0x01164B30u, 1, a11, 0, 1, 0, 1, 0, 0, 0, 0xFFFFFFFFu});
    store<u32>(q + 4, load<u32>(q + 4) | bits);
    opd = load<u32>(load<u32>(w) + 8);
    call_ptr(ctx, opd, {w, q, opd, load<u32>(opd)});
    const u32 p = small ? static_cast<u32>(call(ctx, alloc, {size})) : static_cast<u32>(call(ctx, alloc, {size, 0, 16}));
    if (p != 0) call(ctx, ctor, {p, q});
    opd = load<u32>(load<u32>(p) + 8);  // p == 0 reads the vtable at address 0 (as the original)
    call_ptr(ctx, opd, {p, opd, load<u32>(opd)});
    call(ctx, 0x0054EC1Cu, {q + 164});
    call(ctx, 0x0054EC18u, {q + 164});
    call(ctx, 0x0054EC1Cu, {q + 144});
    call(ctx, 0x0054EC18u, {q + 144});
    const u32 vec = load<u32>(q);
    if (vec != 0) {
        call(ctx, 0x0054EC1Cu, {vec});
        call(ctx, 0x0054EC18u, {vec});
        call(ctx, 0x00552420u, {load<u32>(q), 20});
    }
    return p;
}

// Obj* f(Src* o, Sink* w): QueryBuildObj52 with class 0x00B11E78, a11 1, q->+4 bit
// 0x00100000, a 736-byte SmallAlloc object built by 0x000B9020. r3 = it.
DECOMP(000B9738, Query_BuildObj_000B9738) {
    ret(ctx, QueryBuildObj52(ctx, 0x00B11E78u, 1, 0x00100000u, 0x0055238Cu, true, 736, 0x000B9020u));
}

// Obj* f(Src* o, Sink* w): QueryBuildObj52 with class 0x00B0DBAC, a11 0, q->+4 bit
// 0x8, a 1360-byte Mem_Alloc(1360, 0, 16) object built by 0x00120A58. r3 =
// it.
DECOMP(00121968, Query_BuildObj_00121968) {
    ret(ctx, QueryBuildObj52(ctx, 0x00B0DBACu, 0, 0x8u, 0x0054ED38u, false, 1360, 0x00120A58u));
}

// Clears and frees the lazy 20-byte PtrVector held at *slot (PtrVector_Clear,
// Nop, SmallFree(*slot, 20) with the pointer re-read); r3 = SmallFree's
// result, or prev when *slot is 0.
static u32 FreeVecAt52(PPUContext& ctx, u32 slot, u32 prev) {
    const u32 v = load<u32>(slot);
    if (v == 0) return prev;
    call(ctx, 0x0054EC1Cu, {v});
    call(ctx, 0x0054EC18u, {v});
    return static_cast<u32>(call(ctx, 0x00552420u, {load<u32>(slot), 20}));
}

// void f(T* o) (destructor body): vtable 0x00B12838; each 160-byte entry
// of the PtrVector o->+192 (u16 count +8, data +0; the vector re-read after
// each release) with a +32 skeleton gets AnimSkel_Release7D4EA8; that vector
// freed (FreeVecAt52); vtable 0x00B12230, Mem_FreeFrom(o->+128, 0) and +128
// = 0 when set; Obj_Register7_00490518(o); SortedMap_Remove744748(0x01013B04,
// &o->+52 (copy at sp+112)); the vectors +136 and +16 freed. r3 as left.
DECOMP(000E281C, Obj_Dtor_000E281C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B12838u);
    u32 v = load<u32>(o + 192);
    if (v != 0 && load<u16>(v + 8) != 0) {
        for (u32 i = 0, off = 0;; ++i, off += 160) {
            const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
            if (!(i < n)) break;
            const u32 p = load<u32>(load<u32>(v) + off + 32);
            if (p != 0) {
                call(ctx, 0x007D4EA8u, {p});
                v = load<u32>(o + 192);
            }
        }
    }
    if (v != 0) {
        call(ctx, 0x0054EC1Cu, {v});
        call(ctx, 0x0054EC18u, {v});
        call(ctx, 0x00552420u, {load<u32>(o + 192), 20});
    }
    const u32 m = load<u32>(o + 128);
    store<u32>(o, 0x00B12230u);
    if (m != 0) {
        call(ctx, 0x0054EE6Cu, {m, 0});
        store<u32>(o + 128, 0);
    }
    call(ctx, 0x00490518u, {o});
    Stack fr(ctx, 16);  // the key at sp+112
    store<u32>(fr.addr(), load<u32>(o + 52));
    u32 r3 = static_cast<u32>(call(ctx, 0x00744748u, {0x01013B04u, fr.addr()}));
    r3 = FreeVecAt52(ctx, o + 136, r3);
    r3 = FreeVecAt52(ctx, o + 16, r3);
    ret(ctx, r3);
}

// bool f(Obj* o, Obj* x, u32 k) (may o act on x with kind k): 0 when k is 2
// and bit 0x200 of (o->+216)->+372 is set, when k >= 3, when x is 0 and so
// is o->+432, when bit 0x8000 of (o->+216)->+372 is set, or when o->+244
// is set and its vt[84]() is 0. Then 1 when x is 0 or bit 0 of x->+192 is
// clear, or a = o->vt[300](o, k, x) is 0; else (x->vt[296](x, k, o) & a) ==
// a. (A k == 3 check on component 27 that follows the k >= 3 exit is dead
// code and left out.)
DECOMP(000EDBA4, Obj_CanAct_000EDBA4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), k = arg<u32>(ctx, 2);
    const u32 p = load<u32>(o + 216);
    if ((k == 2 && (load<u32>(p + 372) & 0x200u) != 0) || k >= 3 || (x == 0 && load<u32>(o + 432) == 0) ||
        (load<u32>(p + 372) & 0x8000u) != 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 q = load<u32>(o + 244);
    if (q != 0) {
        const u32 opd = load<u32>(load<u32>(q) + 84);
        if (static_cast<u32>(call_ptr(ctx, opd, {q, opd, load<u32>(opd)})) == 0) {
            ret(ctx, 0u);
            return;
        }
    }
    if (x == 0 || (load<u32>(x + 192) & 1u) == 0) {
        ret(ctx, 1u);
        return;
    }
    u32 opd = load<u32>(load<u32>(o) + 300);
    const u32 a = static_cast<u32>(call_ptr(ctx, opd, {o, k, x, opd, load<u32>(opd)}));
    if (a == 0) {
        ret(ctx, 1u);
        return;
    }
    opd = load<u32>(load<u32>(x) + 296);
    const u32 b = static_cast<u32>(call_ptr(ctx, opd, {x, k, o, opd, load<u32>(opd)}));
    ret(ctx, (b & a) == a ? 1u : 0u);
}

// The component of the effect e with hash h whose vt[32]() is kind, or 0.
static u32 CompOfKind52(PPUContext& ctx, u32 e, u32 h, u32 kind) {
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4u, {e, h}));
    if (c == 0) return 0;
    const u32 opd = load<u32>(load<u32>(c) + 32);
    return static_cast<u32>(call_ptr(ctx, opd, {c, opd, load<u32>(opd)})) == kind ? c : 0u;
}

// void f(Obj* o): Obj_LaunchFactor_000EF3C8(o); with the effect o->+356:
// component 0x03CBAB04 of kind 4 gets the three vec3 rows o->+448 / +464 /
// +480 / +496 at +32 / +48 / +64 / +80, component 0xCD22388B of kind 2 gets
// +20 = (o->+436 != 0), component 0xCC2DABB0 of kind 1 gets the f32
// (o->+216)->+300 at +20; then Effect_SetHash4CF988(e, 0x04B8CC42) (r3 =
// its result; no effect: r3 = Obj_LaunchFactor's). f1 holds the last f32
// copied, as the original leaves it.
DECOMP(000EF564, Obj_SyncEffect_000EF564) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 216);
    const u32 r = static_cast<u32>(call(ctx, 0x000EF3C8u, {o}));
    if (load<u32>(o + 356) == 0) {
        ret(ctx, r);
        return;
    }
    u32 c = CompOfKind52(ctx, load<u32>(o + 356), 0x03CBAB04u, 4);
    if (c != 0) {
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k) Cp52(c + 32 + 16 * row + 4 * k, o + 448 + 16 * row + 4 * k);
        ctx.fpr[1] = Lfs52(o + 504);
    }
    c = CompOfKind52(ctx, load<u32>(o + 356), 0xCD22388Bu, 2);
    if (c != 0) store<u32>(c + 20, load<u32>(o + 436) != 0 ? 1u : 0u);
    c = CompOfKind52(ctx, load<u32>(o + 356), 0xCC2DABB0u, 1);
    if (c != 0) {
        Cp52(c + 20, s + 300);
        ctx.fpr[1] = Lfs52(s + 300);
    }
    ret(ctx, call(ctx, 0x004CF988u, {load<u32>(o + 356), 0x04B8CC42u}));
}

// u32 f(Obj* o, u8 b) (sets the held / colour state): u8 o->+996 = b, bit
// 0x10 of u8 o->+968 cleared; Mem_FreeFrom(o->+976, 0) and +976 = 0 when
// set; the colour {o->+560..+562, b} (sp+112; u8 o->+563 = b) goes to
// o->vt[120](o, &colour, ., 1) with f1 = f32 0x00124198; o->vt[116](o, b, 0,
// 0, 1); WorldObj_SetGroup(o->+160, o->+992); then each 240-byte entry of
// the PtrVector o->+340 (re-read each step) whose +192 object x has bit
// 0x40 of x->+200 and a world object x->+160 gets Obj_SetHeld_000DF3CC(x, 0,
// b, 1) and Obj_SetColourD6F90(x->+160, &colour) (f1 = the same f32). r3 =
// the vector's count (0 without one).
DECOMP(001241A0, Obj_SetHeldColour_001241A0) {
    const u32 o = arg<u32>(ctx, 0);
    u32 b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 fl = load<u8>(o + 968);
    const u32 m = load<u32>(o + 976);
    store<u8>(o + 996, static_cast<u8>(b));
    store<u8>(o + 968, static_cast<u8>(fl & ~0x10u));
    if (m != 0) {
        call(ctx, 0x0054EE6Cu, {m, 0});
        store<u32>(o + 976, 0);
        b = load<u8>(o + 996);
    }
    Stack fr(ctx, 16);  // the colour at sp+112
    const u32 col = fr.addr();
    store<u8>(col, load<u8>(o + 560));
    store<u8>(col + 1, load<u8>(o + 561));
    const u32 c2 = load<u8>(o + 562);
    store<u8>(col + 3, static_cast<u8>(b));
    store<u8>(col + 2, static_cast<u8>(c2));
    store<u8>(o + 563, static_cast<u8>(b));
    const f64 f = Lfs52(0x00124198u);
    u32 opd = load<u32>(load<u32>(o) + 120);
    ctx.fpr[1] = f;
    call_ptr(ctx, opd, {o, col, opd, 1, load<u32>(opd)});
    opd = load<u32>(load<u32>(o) + 116);
    call_ptr(ctx, opd, {o, load<u8>(o + 996), 0, 0, 1, opd, load<u32>(opd)});
    call(ctx, 0x00583E54u, {load<u32>(o + 160), load<u32>(o + 992)});
    u32 n;
    for (u32 i = 0, off = 0;; ++i, off += 240) {
        const u32 v = load<u32>(o + 340);
        n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) break;
        const u32 x = load<u32>(load<u32>(v) + off + 192);
        if (x == 0 || (load<u32>(x + 200) & 0x40u) == 0 || load<u32>(x + 160) == 0) continue;
        call(ctx, 0x000DF3CCu, {x, 0, load<u8>(o + 996), 1});
        callf(ctx, 0x000D6F90u, {load<u32>(x + 160), col}, {f});
    }
    ret(ctx, n);
}

// u32 f(Volume* o) (flushes the pending list): with n = u16 o->+366
// entries, bit 0x02000000 of o->+412 is set while the 8-byte entries at
// o->+372 are memmoved to the scratch 0x01055D00 and +366 = 0; for each
// entry's object e (+4): e->vt[40](e, 0); o->vt[288](o, e) when e is in
// o's list (+344, u16 count +352); o->vt[284](o, e) unless bit 0x40000000
// of o->+412 is set and bit 0 of e->+188 is clear. Then bit 0x02000000 is
// cleared and, unless bit 0x80000000 is set, Volume_Prepare20C670(o). r3 =
// o with no entries, else the +412 word or Volume_Prepare's result.
DECOMP(0020DC6C, Volume_FlushPending_0020DC6C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u16>(o + 366);
    if (n == 0) {
        ret(ctx, o);
        return;
    }
    constexpr u32 S = 0x01055D00u;
    store<u32>(o + 412, load<u32>(o + 412) | 0x02000000u);
    call(ctx, 0x0071C3F0u, {S, load<u32>(o + 372), (n << 3) & 0x0007FFF8u});
    store<u16>(o + 366, 0);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(S + 4 + 8 * i);
        u32 opd = load<u32>(load<u32>(e) + 40);
        call_ptr(ctx, opd, {e, 0, opd, load<u32>(opd)});
        const s32 cnt = static_cast<s32>(load<u16>(o + 352));
        const u32 arr = load<u32>(o + 344);
        for (s32 j = 0; j < cnt; ++j) {
            if (load<u32>(arr + 4 * static_cast<u32>(j)) != e) continue;
            opd = load<u32>(load<u32>(o) + 288);
            call_ptr(ctx, opd, {o, e, opd, load<u32>(opd)});
            break;
        }
        if ((load<u32>(o + 412) & 0x40000000u) == 0 || (load<u32>(e + 188) & 1u) != 0) {
            opd = load<u32>(load<u32>(o) + 284);
            call_ptr(ctx, opd, {o, e, opd, load<u32>(opd)});
        }
    }
    const u32 w = load<u32>(o + 412) & ~0x02000000u;
    store<u32>(o + 412, w);
    if (w & 0x80000000u) {
        ret(ctx, w);
        return;
    }
    ret(ctx, call(ctx, 0x0020C670u, {o}));
}

// u32 f(T* o, XmlCursor* x): the name attribute 0x0024728B (String_Assign(o,
// a) when found), the bool 0x0027129B -> u8 o->+56 (its old value as
// default); then for each child 0x9374020A an 88-byte item (SmallAlloc(88);
// when that works: +0 = o, +36 / +40 / +48 / +56 / +60 = 0, u8 +44 = 0, u8
// +52 = u8 +53 = 1, PtrVector +64 (u16 +70 = 4), u8 +84 = 0, memset(+4, 0,
// 32)), 0x002F7D60(item, child), appended to the PtrVector o->+36 (also
// when it is 0). r3 = 0 (the last cursor call's result).
DECOMP(002F892C, Def_ReadItems_002F892C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 a = static_cast<u32>(call(ctx, 0x004F9534u, {x, 0x0024728Bu}));
    if (a != 0) call(ctx, 0x00555F80u, {o, a});
    call(ctx, 0x004F986Cu, {x, 0x0027129Bu, o + 56, load<u8>(o + 56)});
    constexpr u32 kChild = 0x9374020Au;
    u32 c = static_cast<u32>(call(ctx, 0x004F92BCu, {x, kChild}));
    while (c != 0) {
        u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {88}));
        if (p != 0) {
            store<u32>(p, o);
            store<u32>(p + 36, 0);
            store<u32>(p + 40, 0);
            store<u8>(p + 44, 0);
            store<u32>(p + 48, 0);
            store<u8>(p + 52, 1);
            store<u8>(p + 53, 1);
            store<u32>(p + 56, 0);
            store<u32>(p + 60, 0);
            call(ctx, 0x005516E0u, {p + 64});
            store<u8>(p + 84, 0);
            store<u16>(p + 70, 4);
            call(ctx, 0x00726E54u, {p + 4, 0, 32});
        }
        call(ctx, 0x002F7D60u, {p, c});
        const u32 v = o + 36;
        if (load<u16>(o + 44) == load<u16>(o + 40))
            call(ctx, 0x00551718u, {v, (static_cast<u32>(load<u16>(o + 40)) << 1) & 0x0001FFFEu});
        store<u32>(load<u32>(o + 36) + ((static_cast<u32>(load<u16>(o + 44)) << 2) & 0x0003FFFCu), p);
        const u32 cnt = load<u16>(o + 44) + 1u;
        const u32 hi = load<u16>(o + 46);
        store<u16>(o + 44, static_cast<u16>(cnt));
        store<u16>(o + 46, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(cnt & 0xFFFFu) ? hi : cnt));
        c = static_cast<u32>(call(ctx, 0x004F8FC8u, {c, kChild}));
    }
    ret(ctx, c);
}

// u32 f(T* o, Obj* x, Vec3* v): x (when set and not yet in it) is appended
// to the PtrVector o->+920 (u16 cap +924, count +928, high-water +930);
// then with the player p = ((*0x00F65CA0)->+4208)->+8 flagged (bit 0x8 of
// p->+192) and Obj_Notify24(p) set, Obj_Notify24(p again when still flagged,
// else 0) and a 128-byte record (Rec128_Alloc_0029F760, r3 = Notify24's
// result as the original leaves it): +8 = x, +40 = 0x7DDC7E2D, +36 = 1,
// +16..+24 = *v, +120 bits 0x10000000 / 0x04000000 / 0x00800000 set and
// 0x08000000 / 0x02000000 cleared. r3 as the original leaves it (o, x, the
// record, ...).
DECOMP(0030C3F0, Obj_AddWatched_0030C3F0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    if (x == 0) {
        ret(ctx, o);
        return;
    }
    const u32 n = load<u16>(o + 928), cap = load<u16>(o + 924);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(load<u32>(o + 920) + 4 * i) == x) {
            ret(ctx, x);
            return;
        }
    }
    if (n == cap) call(ctx, 0x00551718u, {o + 920, (cap << 1) & 0x0001FFFEu});
    store<u32>(load<u32>(o + 920) + ((static_cast<u32>(load<u16>(o + 928)) << 2) & 0x0003FFFCu), x);
    const u32 cnt = load<u16>(o + 928) + 1u;
    const u32 hi = load<u16>(o + 930);
    store<u16>(o + 928, static_cast<u16>(cnt));
    store<u16>(o + 930, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(cnt & 0xFFFFu) ? hi : cnt));
    const u32 g = load<u32>(0x00F65CA0u) + 4208;
    const u32 p = load<u32>(g + 8);
    if (p == 0) {
        ret(ctx, g);
        return;
    }
    if ((load<u32>(p + 192) & 0x8u) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (static_cast<u32>(call(ctx, 0x000D949Cu, {p})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 p2 = load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8);
    const u32 a = (p2 != 0 && (load<u32>(p2 + 192) & 0x8u) != 0) ? p2 : 0u;
    const u32 nr = static_cast<u32>(call(ctx, 0x000D949Cu, {a}));
    const u32 r = static_cast<u32>(call(ctx, 0x0029F760u, {nr}));
    if (r == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(r + 8, x);
    store<u32>(r + 40, 0x7DDC7E2Du);
    const u32 w = load<u32>(r + 120);
    store<u32>(r + 36, 1);
    Cp52(r + 16, v);
    Cp52(r + 20, v + 4);
    Cp52(r + 24, v + 8);
    store<u32>(r + 120, ((w | 0x10000000u | 0x04000000u | 0x00800000u) & ~0x08000000u) & ~0x02000000u);
    ctx.fpr[1] = Lfs52(v + 8);
    ret(ctx, r);
}

// u32 f(Loader* o, ? a, Msg* m): Loader_Step_003252D8(o, a, m) (r3 = its
// result), first, when u8 o->+18 is 4 and m->+0 is 6 and the player slot
// s = (*0x00F65CA0) + o->+2572 * 0x20C80 + 4208 has a flagged object p (s+8,
// bit 0x8 of p->+192): o->vt[104](o, 5, 0) when o->+2576 is set and
// p->+380 has q with v = q->vt[64](q, o->+2576) != 0 sharing bits with both
// pad masks +8 / +12 of the pad block 0x010B19E0 + u8 s+1 * 11968 + 176 (s
// re-read), or, with o->+2576 clear, o->+2580 set and
// Pad_DirActive_00340EA0(o, pad block).
DECOMP(00340FC8, Loader_PadStep_00340FC8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    if (load<u8>(o + 18) == 4 && load<u32>(m) == 6) {
        const u32 s = load<u32>(0x00F65CA0u) + load<u32>(o + 2572) * 0x00020C80u + 4208;
        const u32 p = load<u32>(s + 8);
        if (p != 0 && (load<u32>(p + 192) & 0x8u) != 0) {
            constexpr u32 kPads = 0x010B19E0u;
            const u32 pad = kPads + load<u8>(s + 1) * 11968u + 176;
            const u32 c = load<u32>(o + 2576);
            bool act = false;
            if (c != 0) {
                const u32 q = load<u32>(p + 380);
                if (q != 0) {
                    const u32 opd = load<u32>(load<u32>(q) + 64);
                    const u32 v = static_cast<u32>(call_ptr(ctx, opd, {q, c, opd, load<u32>(opd)}));
                    if (v != 0) {
                        const u32 s2 = load<u32>(0x00F65CA0u) + load<u32>(o + 2572) * 0x00020C80u + 4208;
                        const u32 t = kPads + load<u8>(s2 + 1) * 11968u + 176;
                        act = ((load<u32>(t + 8) & v) & (load<u32>(t + 12) & v)) != 0;
                    }
                }
            } else if (load<u32>(o + 2580) != 0) {
                act = static_cast<u32>(call(ctx, 0x00340EA0u, {o, pad})) != 0;
            }
            if (act) {
                const u32 opd = load<u32>(load<u32>(o) + 104);
                call_ptr(ctx, opd, {o, 5, 0, opd, load<u32>(opd)});
            }
        }
    }
    ret(ctx, call(ctx, 0x003252D8u, {o, a, m}));
}

// void f(Vec2* out, T* o, Vec3* p): the distances of p to the current
// segment of o (entry i = o->+536 of the 16-byte array at o->+332: points
// a (+0), b (+4), c (+8), required d (+12); positions at +128), divided by
// the f32 o->+556. Without a or d: out = the defaults 0x01164AFC (no
// division). Without b or c: |p.x - a.x| / |p.z - a.z|. Otherwise |(p - a)
// . (b - a) * k / |b - a|| and the same with c (k = 0x00387E90). f1 =
// out->y as the last operation leaves it.
DECOMP(00387E98, Seg_Distances_00387E98) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    const u32 e = o + (load<u32>(o + 536) << 4);
    Cp52(out, 0x01164AFCu);
    Cp52(out + 4, 0x01164B00u);
    ctx.fpr[1] = Lfs52(0x01164B00u);
    const u32 a = load<u32>(e + 332);
    if (a == 0 || load<u32>(e + 344) == 0) return;
    const u32 A = a + 128;
    const u32 b = load<u32>(e + 336), c = b != 0 ? load<u32>(e + 340) : 0u;
    f64 x, y;
    if (b == 0 || c == 0) {
        x = std::fabs(F52(Lfs52(p) - Lfs52(A)));
        y = std::fabs(F52(Lfs52(p + 8) - Lfs52(a + 136)));
    } else {
        const u32 B = b + 128, C = c + 128;
        const f64 ax = Lfs52(A), ay = Lfs52(A + 4), az = Lfs52(A + 8);
        f64 by = F52(Lfs52(B + 4) - ay), bx = F52(Lfs52(B) - ax), bz = F52(Lfs52(B + 8) - az);
        const f64 cy = F52(Lfs52(C + 4) - ay), cx = F52(Lfs52(C) - ax), cz = F52(Lfs52(C + 8) - az);
        f64 lb = F52(by * by), lc = F52(cy * cy);
        const f64 k = Lfs52(0x00387E90u);
        lb = F52(std::fma(bx, bx, lb));
        lc = F52(std::fma(cx, cx, lc));
        lb = F52(std::fma(bz, bz, lb));
        const f64 dy = F52(Lfs52(p + 4) - ay);
        lc = F52(std::fma(cz, cz, lc));
        lb = F52(std::sqrt(lb));
        lc = F52(std::sqrt(lc));
        const f64 dx = F52(Lfs52(p) - ax), dz = F52(Lfs52(p + 8) - az);
        lb = F52(k / lb);
        lc = F52(k / lc);
        by = F52(lb * by);
        bx = F52(lb * bx);
        bz = F52(lb * bz);
        f64 s = F52(dy * by);
        s = F52(std::fma(dx, bx, s));
        s = F52(std::fma(dz, bz, s));
        const f64 ny = F52(lc * cy), nx = F52(lc * cx), nz = F52(lc * cz);
        x = std::fabs(s);
        f64 t = F52(dy * ny);
        t = F52(std::fma(dx, nx, t));
        store<f32>(out, static_cast<f32>(x));
        t = F52(std::fma(dz, nz, t));
        y = std::fabs(t);
        store<f32>(out + 4, static_cast<f32>(y));
        x = Lfs52(out);
        y = Lfs52(out + 4);
    }
    if (b == 0 || c == 0) {
        store<f32>(out, static_cast<f32>(x));
        store<f32>(out + 4, static_cast<f32>(y));
    }
    const f64 w = Lfs52(o + 556);
    const f64 rx = F52(x / w), ry = F52(y / w);
    store<f32>(out, static_cast<f32>(rx));
    store<f32>(out + 4, static_cast<f32>(ry));
    ctx.fpr[1] = ry;
}

// Node* f(Slot* o, u16 kind, const char* name) (registers a named node
// graph): u16 o->+8 = kind, +0 = 0; +12 = a 52-byte NodeGraph
// (Mem_Alloc(52, 0, 16) -> NodeGraph_Construct_004D2BFC(g, name, 0, 0); 0
// when that fails), +4 = String_HashNoCase(name). In the registry G =
// *0x00F65038 (count +28 below 0x1800; else String_Format(0x00ADD314,
// 6144) and index -1) entry n of the 16-byte table +36 gets {G->+32, o, 5}
// and +28 / +32 grow by 1 / 128; u16 o->+16 = n, +20 = the 128-byte record
// G->+24 + 128 n (0 for -1; u16 / u8 +0 / +2 = 0), entry n's +12 =
// 0x7C764E52 (for -1 that is G->+32: kept). r3 = Node_Bind(o->+12,
// 0x00ADD9EC, 3, 1, -1, 0), whose +76 = 100.0f.
DECOMP(00391694, Graph_Register_00391694) {
    const u32 o = arg<u32>(ctx, 0), kind = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2);
    store<u16>(o + 8, static_cast<u16>(kind));
    store<u32>(o, 0);
    u32 g = static_cast<u32>(call(ctx, 0x0054ED38u, {52, 0, 16}));
    if (g != 0) call(ctx, 0x004D2BFCu, {g, name, 0, 0});
    store<u32>(o + 12, g);
    store<u32>(o + 4, static_cast<u32>(call(ctx, 0x0056A710u, {name})));
    constexpr u32 kReg = 0x00F65038u;
    u32 G = load<u32>(kReg);
    u32 n = load<u32>(G + 28);
    if (n < 0x1800u) {
        store<u32>(G + 36 + (n << 4), load<u32>(G + 32));
        store<u32>(G + 40 + (load<u32>(G + 28) << 4), o);
        store<u32>(G + 44 + (load<u32>(G + 28) << 4), 5);
        n = load<u32>(G + 28);
        const u32 base = load<u32>(G + 32);
        store<u32>(G + 28, n + 1);
        store<u32>(G + 32, base + 128);
    } else {
        call(ctx, 0x005534D0u, {0x00ADD314u, 6144});
        n = 0xFFFFFFFFu;
    }
    store<u16>(o + 16, static_cast<u16>(n));
    const u32 idx = static_cast<u32>(static_cast<s32>(static_cast<s16>(n)));
    G = load<u32>(kReg);
    const u32 r = idx == 0xFFFFFFFFu ? 0u : load<u32>(G + 24) + (idx << 7);
    store<u32>(o + 20, r);
    G = load<u32>(kReg);
    if (r != 0) {
        store<u16>(r, 0);
        store<u8>(r + 2, 0);
    }
    store<u32>(G + 36 + (idx << 4) + 12, 0x7C764E52u);
    const u32 b = static_cast<u32>(call(ctx, 0x004D3088u, {load<u32>(o + 12), 0x00ADD9ECu, 3, 1, 0xFFFFFFFFu, 0}));
    store<u32>(b + 76, 0x42C80000u);
    ret(ctx, b);
}

// u32 f(Ref* a, Bank* b) (resolves a sound / anim name to an index): the
// u16 slot written is ((s16 a->+6 < u16 b->+144 and the 132-byte entry
// b + 152 + 132 i has +0 < 10) ? that entry : 0) + u16 *0x0106CF80 + 4
// (the 0 case writes near address 0: kept). Records r = G->+24 + 128 k of
// the registry G = *0x00F65038 for k = s16 a->+4 / a->+2 (-1: none); the
// name pointer at (r4 ? r4 + u16 *0x0106CF7E : 0)'s first byte * 4 +
// u16 *0x0106CF70 + record(a->+4), where the second record's u8 +2 picks
// that base. No name: slot = -1, r3 = -1. Else its String_HashNoCase is
// looked up in the list at b + 152 + 132 u16 b->+146 (u16 count +0, u16
// offset +2 of the u32 hashes): slot = the index (r3), or -1 and
// String_Format(0x00ADDB0C, name) (r3 = its result).
DECOMP(00393240, Bank_ResolveName_00393240) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const s32 i6 = static_cast<s16>(load<u16>(a + 6));
    const u32 k4 = load<u16>(a + 4), k2 = load<u16>(a + 2);
    u32 slot = 0;
    if (i6 < static_cast<s32>(load<u16>(b + 144))) {
        slot = b + static_cast<u32>(i6) * 132u + 152;
        if (!(static_cast<s32>(load<u32>(slot)) < 10)) slot = 0;
    }
    slot = slot + load<u16>(0x0106CF80u) + 4;
    const u32 G = load<u32>(0x00F65038u);
    const s32 s4 = static_cast<s16>(k4), s2 = static_cast<s16>(k2);
    const u32 r4 = s4 == -1 ? 0u : load<u32>(G + 24) + (static_cast<u32>(s4) << 7);
    const u32 r2 = s2 == -1 ? 0u : load<u32>(G + 24) + (static_cast<u32>(s2) << 7);
    const u32 pb = static_cast<s32>(load<u8>(r2 + 2)) > 0 ? r2 + load<u16>(0x0106CF7Eu) : 0u;
    const u32 nm = load<u32>(r4 + load<u16>(0x0106CF70u) + (static_cast<u32>(load<u8>(pb)) << 2));
    if (nm == 0) {
        store<u16>(slot, 0xFFFFu);
        ret(ctx, 0xFFFFFFFFu);
        return;
    }
    const u32 L = b + static_cast<u32>(load<u16>(b + 146)) * 132u + 152;
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {nm}));
    const s32 n = static_cast<s32>(load<u16>(L));
    u32 r = 0xFFFFFFFFu;
    for (s32 i = 0; i < n; i = static_cast<s16>(i + 1)) {
        if (load<u32>(L + load<u16>(L + 2) + ((static_cast<u32>(i) << 2) & 0x0003FFFCu)) == h) {
            r = static_cast<u32>(i);
            break;
        }
    }
    store<u16>(slot, static_cast<u16>(r));
    if (r == 0xFFFFFFFFu) r = static_cast<u32>(call(ctx, 0x005534D0u, {0x00ADDB0Cu, nm}));
    ret(ctx, r);
}

// void f() (static destructor): the twenty 80-byte tuning variables at
// 0x010AAD50 down to 0x010AA760 (step -80) get their vtable back
// (0x00B0D99C; 0x00B0DA6C for 0x010AA9E0; 0x00B0D934 for the last two) and
// TuningVar_DestructBase_004FC1A0 (r3 = the last one's result).
DECOMP(0043AE5C, TuningVars_StaticDtor_0043AE5C) {
    u64 r = 0;
    for (u32 k = 0; k < 20; ++k) {
        const u32 v = 0x010AAD50u - 80 * k;
        const u32 vt = k == 11 ? 0x00B0DA6Cu : (k >= 18 ? 0x00B0D934u : 0x00B0D99Cu);
        store<u32>(v, vt);
        r = call(ctx, 0x004FC1A0u, {v});
    }
    ret(ctx, r);
}

// u32 f(Def* o, Ctx* a, Out* b, Key* c) (resolves a key into b): with no
// key (c->+0 == 0) bit 0x40 of u8 b->+4 is cleared, b->+36 = o->+24 (f32),
// u8 b->+40 = u8 o->+21. With o->+40 set: b->+12 / +16 = o->+36 / +40 and
// bit 0x10 of u8 b->+4 set. Otherwise the lookup 0x00789BC0(a->+16, c->+0,
// b->+20, &rec) (rec at sp+112: f32 -3.4e38 +0, u8 +4 = 0, +8 = +12 = 0) or,
// failing that and with c->+4 set, Node_LookupKey_00789E00(a->+16,
// (c->+4)->+12, o->+12, &rec2) (sp+128; f32 +8 = -3.4e38, the rest 0)
// gives b->+36 (f32) / u8 +40 / +12 / +16 and bit 0x40 (bit 0 of the
// result) of u8 b->+4 (o->+24 / u8 o->+21 when both fail); bit 0x08 = (b->+16
// != 0) (r3). f1 = the f32 stored at +36 where the original leaves it.
DECOMP(004B36A0, Key_Resolve_004B36A0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 k = load<u32>(c);
    if (k == 0) {
        store<u8>(b + 4, static_cast<u8>(load<u8>(b + 4) & ~0x40u));
        Cp52(b + 36, o + 24);
        ctx.fpr[1] = Lfs52(o + 24);
        const u32 t = load<u8>(o + 21);
        store<u8>(b + 40, static_cast<u8>(t));
        ret(ctx, t);
        return;
    }
    const u32 h = load<u32>(o + 40);
    if (h != 0) {
        const u32 fl = load<u8>(b + 4) | 0x10u;
        store<u32>(b + 12, load<u32>(o + 36));
        store<u32>(b + 16, h);
        store<u8>(b + 4, static_cast<u8>(fl));
        ret(ctx, fl);
        return;
    }
    constexpr u32 kLow = 0xFF7FFFFFu;  // -FLT_MAX
    Stack fr(ctx, 48);  // rec at sp+112, rec2 at sp+128
    const u32 rec = fr.addr(), rec2 = rec + 16;
    store<u8>(rec + 4, 0);
    store<u32>(rec, kLow);
    store<u32>(rec + 8, 0);
    store<u32>(rec + 12, 0);
    const u32 tree = load<u32>(a + 16), alt = load<u32>(c + 4);
    u32 r3 = static_cast<u32>(call(ctx, 0x00789BC0u, {tree, k, load<u32>(b + 20), rec}));
    u32 f, by;
    if (r3 != 0) {
        f = load<u32>(rec);
        by = load<u8>(rec + 4);
    } else {
        u32 r = 0, f2 = load<u32>(0x004B3698u), b6 = 0, w8 = 0, w12 = 0;
        if (alt != 0) {
            store<u32>(rec2 + 4, 0);
            store<u32>(rec2, 0);
            store<u32>(rec2 + 20, 0);
            store<u32>(rec2 + 16, 0);
            store<u8>(rec2 + 12, 0);
            store<u32>(rec2 + 8, kLow);
            r = static_cast<u32>(call(ctx, 0x00789E00u, {tree, load<u32>(alt + 12), load<u32>(o + 12), rec2}));
            f2 = load<u32>(rec2 + 8);
            b6 = load<u8>(rec2 + 12);
            w12 = load<u32>(rec2 + 20);
            w8 = load<u32>(rec2 + 16);
        }
        r3 = r != 0 ? 1u : 0u;
        if (r3 == 0) {
            f = load<u32>(o + 24);
            by = load<u8>(o + 21);
        } else {
            f = f2;
            store<u32>(rec + 12, w12);
            by = b6;
            store<u32>(rec + 8, w8);
        }
    }
    const u32 w8 = load<u32>(rec + 8);
    u32 fl = load<u8>(b + 4);
    fl = (fl & ~0x40u) | ((r3 & 1u) << 6);
    const u32 w12 = load<u32>(rec + 12);
    store<u32>(b + 36, f);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(f));
    store<u8>(b + 40, static_cast<u8>(by));
    const u32 nz = w8 != 0 ? 1u : 0u;
    store<u32>(b + 12, w12);
    fl = (fl & ~0x08u) | (nz << 3);
    store<u32>(b + 16, w8);
    store<u8>(b + 4, static_cast<u8>(fl));
    ret(ctx, nz);
}

// ---------------------------------------------------------------- sweep: 117 instructions
// u32 f(Scene* o, Camera* cam) (starts the scene view's frame): only with
// the view v (o->+6260), o->+6272, o->+6268 and o->+6184 all set (else r3
// = v). v->+660 = 0x00F3B438; SetFactor_005202C8 / View_Set520338 /
// SetF112Flag_00520094(v) with the f32s cam->+64 / +60 / +112; with the
// camera node n = o->+6272: View_SetFov(v, n->+176) and
// RenderTarget_SetViewMatrix_00522630(v, n + 112); Job_Begin(0x00F9C01C, v,
// 255); View_DrawBackdrop(*0x00F65CA0, v, 0x00F9AB7C, 0x00F9ABC8); the
// world object w = (o->+6276)->+160 of kind 8: WorldObj_Set292(w, 0xD000),
// when bit 0x80 of u8 w->+322 is set it is cleared and w->vt[36](w), then
// 0x004A6DF0(w, 1); Obj_SetSlot20(o->+6268, v->+516, 0); r3 =
// View_BeginFrame_00459100(0x00F9C01C). (v / n re-read for each call.)
DECOMP(00056ED0, Scene_BeginView_00056ED0) {
    const u32 o = arg<u32>(ctx, 0), cam = arg<u32>(ctx, 1);
    const u32 v = load<u32>(o + 6260);
    if (v == 0 || load<u32>(o + 6272) == 0 || load<u32>(o + 6268) == 0 || load<u32>(o + 6184) == 0) {
        ret(ctx, v);
        return;
    }
    store<u32>(v + 660, 0x00F3B438u);
    callf(ctx, 0x005202C8u, {v}, {Lfs52(cam + 64)});
    callf(ctx, 0x00520338u, {load<u32>(o + 6260)}, {Lfs52(cam + 60)});
    callf(ctx, 0x00520094u, {load<u32>(o + 6260)}, {Lfs52(cam + 112)});
    const u32 n = load<u32>(o + 6272);
    if (n != 0) {
        callf(ctx, 0x00520368u, {load<u32>(o + 6260)}, {Lfs52(n + 176)});
        const u32 m = load<u32>(o + 6272) + 112;
        call(ctx, 0x00522630u, {load<u32>(o + 6260), m, m});
    }
    constexpr u32 J = 0x00F9C01Cu;
    call(ctx, 0x00459198u, {J, load<u32>(o + 6260), 255});
    call(ctx, 0x004583F8u, {load<u32>(0x00F65CA0u), load<u32>(o + 6260), 0x00F9AB7Cu, 0x00F9ABC8u});
    const u32 h = load<u32>(o + 6276);
    const u32 w = h != 0 ? load<u32>(h + 160) : 0u;
    if (w != 0 && ((load<u32>(w + 4) >> 10) & 0x3Fu) == 8) {
        call(ctx, 0x00580AF0u, {w, 0xD000});
        const u32 b = load<u8>(w + 322);
        if (b & 0x80u) {
            store<u8>(w + 322, static_cast<u8>(b & ~0x80u));
            const u32 opd = load<u32>(load<u32>(w) + 36);
            call_ptr(ctx, opd, {w, opd, load<u32>(opd)});
        }
        call(ctx, 0x004A6DF0u, {w, 1});
    }
    call(ctx, 0x004FEF48u, {load<u32>(o + 6268), load<u32>(load<u32>(o + 6260) + 516), 0});
    ret(ctx, call(ctx, 0x00459100u, {J}));
}

// T* f(?, XmlCursor* x): a 36-byte def (Mem_Alloc(36, 0, 16); r3 = 0 when
// that fails): Obj_InitVt_B21C98(d, x), vtable 0x00B11760, +12..+24 = 0, u8
// +28 / +29 / +31 = 0, u8 +30 / +32 = 1; the hash 0x0027873A -> +12 (default
// 0), the bools 0x469D7C52 / 0x03FF92CB / 0x8488565B -> u8 +28..+30, the
// hashes 0x00239807 / 0x04A5C9FC -> +16 / +20, the bools 0x822AA3AB /
// 0xFF4019A3 -> u8 +31 / +32, the hash 0x7A30D7E3 -> +24 (old values as
// defaults). r3 = d.
DECOMP(000A3E04, Def36_ReadXml_000A3E04) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 d = static_cast<u32>(call(ctx, 0x0054ED38u, {36, 0, 16}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {d, x});
    store<u32>(d + 12, 0);
    store<u32>(d + 16, 0);
    store<u32>(d + 20, 0);
    store<u32>(d + 24, 0);
    store<u8>(d + 28, 0);
    store<u32>(d, 0x00B11760u);
    store<u8>(d + 29, 0);
    store<u8>(d + 30, 1);
    store<u8>(d + 31, 0);
    store<u8>(d + 32, 1);
    call(ctx, 0x004FA14Cu, {x, 0x0027873Au, d + 12, 0});
    call(ctx, 0x004F986Cu, {x, 0x469D7C52u, d + 28, load<u8>(d + 28)});
    call(ctx, 0x004F986Cu, {x, 0x03FF92CBu, d + 29, load<u8>(d + 29)});
    call(ctx, 0x004F986Cu, {x, 0x8488565Bu, d + 30, load<u8>(d + 30)});
    call(ctx, 0x004FA14Cu, {x, 0x00239807u, d + 16, load<u32>(d + 16)});
    call(ctx, 0x004FA14Cu, {x, 0x04A5C9FCu, d + 20, load<u32>(d + 20)});
    call(ctx, 0x004F986Cu, {x, 0x822AA3ABu, d + 31, load<u8>(d + 31)});
    call(ctx, 0x004F986Cu, {x, 0xFF4019A3u, d + 32, load<u8>(d + 32)});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, d + 24, load<u32>(d + 24)});
    ret(ctx, d);
}

// Returns the 4 * count block word w's buffer p to the pool 0x011DF31C
// (pool->vt[20](pool, p, (s32)(w << 2)) unless bit 0x80000000 of w is set);
// r6 / r7 = the OPD and entry, or swapped (swap) as the two call sites leave them.
static void PoolRelease52(PPUContext& ctx, u32 p, u32 w, bool swap) {
    if (w & 0x80000000u) return;
    constexpr u32 kPool = 0x011DF31Cu;
    const u32 opd = load<u32>(load<u32>(kPool) + 20), ent = load<u32>(opd);
    const u64 n = static_cast<u64>(static_cast<s64>(static_cast<s32>(w << 2)));
    if (swap)
        call_ptr(ctx, opd, {kPool, p, n, ent, opd});
    else
        call_ptr(ctx, opd, {kPool, p, n, opd, ent});
}

// void f(T* o) (destructor body): vtables 0x00B14730 / +380 0x00B148CC /
// +384 0x00B148F4; World_ListRemove_0046B7A4(*0x00F65CA0, 0, 0x00F31548, o,
// o); List1013B48_Remove(o); each set slot o->+532[0..10] gets vt[4]();
// +524 = 0; the pooled buffers {+520, w +528} and {+508, w +516} go back to
// the pool (PoolRelease52; their pointers / +512 cleared and the words set
// to 0x80000000); vtables 0x00B126B8 / +380 0x00B1200C / +384 0x00B12528;
// r3 = VtB12550_Dtor_000DF934(o).
DECOMP(0016EC4C, Obj_Dtor_0016EC4C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B14730u);
    store<u32>(o + 380, 0x00B148CCu);
    store<u32>(o + 384, 0x00B148F4u);
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 0, 0x00F31548u, o, o});
    call(ctx, 0x000D68CCu, {o});
    for (u32 k = 0; k < 11; ++k) {
        const u32 p = load<u32>(o + 532 + 4 * k);
        if (p == 0) continue;
        const u32 opd = load<u32>(load<u32>(p) + 4);
        call_ptr(ctx, opd, {p, opd, load<u32>(opd)});
    }
    store<u32>(o + 524, 0);
    PoolRelease52(ctx, load<u32>(o + 520), load<u32>(o + 528), false);
    const u32 w = load<u32>(o + 516);
    store<u32>(o + 520, 0);
    store<u32>(o + 528, 0x80000000u);
    store<u32>(o + 512, 0);
    PoolRelease52(ctx, load<u32>(o + 508), w, true);
    store<u32>(o + 508, 0);
    store<u32>(o + 516, 0x80000000u);
    store<u32>(o + 384, 0x00B12528u);
    store<u32>(o + 380, 0x00B1200Cu);
    store<u32>(o, 0x00B126B8u);
    ret(ctx, call(ctx, 0x000DF934u, {o}));
}

// u32 f(Obj* o) (resolves the target spline, then fires its event): 0
// when Obj_Check_001AEA50(o) fails (it reads f1 as the caller left it).
// With bit 0x04 of u8 o->+576: o->+560 = the kind-13 world object behind
// component (o->+216)->+404 of the effect o->+356 (vt[32]() == 5, ->+28);
// failing that Node_FindKind13_004A21F8(o->+160, key), failing that
// *SplineMap_Find((*0x00F8F158)->+16, &key) (key at sp+112); bit 0x04
// cleared. Then unless bit 0x10 is set, with bit 0x80 set and o->+564 !=
// -1: bit 0x10 set when bit 0x40 is, the 24-byte event at sp+116
// (EventInit_0046528C(ev, o, 0, 0)) goes to World_Query482D14(*0x00F65CA0,
// o->+564, ev), bit 0x80 cleared. r3 = 1.
DECOMP(001C111C, Obj_FireSplineEvent_001C111C) {
    const u32 o = arg<u32>(ctx, 0);
    callf(ctx, 0x001AEA50u, {o}, {ctx.fpr[1]});  // f1 passed through as the caller left it
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 b = load<u8>(o + 576);
    const u32 s = load<u32>(o + 216);
    Stack fr(ctx, 32);  // key at sp+112, the event at sp+116..139
    const u32 sp = fr.addr() - 112;
    if (b & 0x04u) {
        const u32 e = load<u32>(o + 356);
        if (e != 0) {
            const u32 c = CompOfKind52(ctx, e, load<u32>(s + 404), 5);
            const u32 r = c != 0 ? load<u32>(c + 28) : 0u;
            if (r != 0 && ((load<u32>(r + 4) >> 10) & 0x3Fu) == 13) store<u32>(o + 560, r);
        }
        if (load<u32>(o + 560) == 0) {
            const u32 r = static_cast<u32>(call(ctx, 0x004A21F8u, {load<u32>(o + 160), load<u32>(s + 404)}));
            store<u32>(o + 560, r);
            if (r == 0) {
                store<u32>(sp + 112, load<u32>(s + 404));
                const u32 q = static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), sp + 112}));
                store<u32>(o + 560, load<u32>(q));
            }
        }
        b = load<u8>(o + 576) & ~0x04u;
        store<u8>(o + 576, static_cast<u8>(b));
    }
    if ((b & 0x10u) == 0 && (b & 0x80u) != 0 && load<u32>(o + 564) != 0xFFFFFFFFu) {
        if (load<u32>(o + 576) & 0x40000000u) {
            b |= 0x10u;
            store<u8>(o + 576, static_cast<u8>(b));
        }
        const u32 ev = sp + 116;
        for (u32 k = 0; k < 6; ++k) store<u32>(ev + 4 * k, 0);
        call(ctx, 0x0046528Cu, {ev, o, 0, 0});
        call(ctx, 0x00482D14u, {load<u32>(0x00F65CA0u), load<u32>(o + 564), ev});
        store<u8>(o + 576, static_cast<u8>(load<u8>(o + 576) & ~0x80u));
    }
    ret(ctx, 1u);
}

// u32 f(Obj* o) (target-zone update): 0 when Obj_Check_001AEA50(o) fails
// (f1 passed through), 1 without o->+216. Once (bit 0x80 of u8 o->+852
// then set): o->+752 = Obj_FindById(o->+756) when bit 0x00400000 of its
// +192 is set (else 0), whose u16 +178 is counted up. Then for i = 0, 1 the
// target x = Target_Next271C08(CallK2_000D93D4(o), i, 0) (none: stop):
// with bit 0x2 of x->+192, i == 0 and bit 0 of x->+188, the 24-byte event
// at sp+112 (Event3_Init_00464F88(ev, o, 0, 0, 0)) goes to
// World_Query482D14(*0x00F65CA0, s8 x->+172, ev). r3 = 1.
DECOMP(001CC660, Obj_ZoneEvent_001CC660) {
    const u32 o = arg<u32>(ctx, 0);
    callf(ctx, 0x001AEA50u, {o}, {ctx.fpr[1]});  // f1 passed through as the caller left it
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u32>(o + 216) == 0) {
        ret(ctx, 1u);
        return;
    }
    if ((load<u8>(o + 852) & 0x80u) == 0) {
        const u32 t = static_cast<u32>(call(ctx, 0x000D6A28u, {load<u32>(o + 756)}));
        const u32 r = (t != 0 && (load<u32>(t + 192) & 0x00400000u) != 0) ? t : 0u;
        store<u32>(o + 752, r);
        if (r != 0) store<u16>(r + 178, static_cast<u16>(load<u16>(r + 178) + 1));
        store<u8>(o + 852, static_cast<u8>(load<u8>(o + 852) | 0x80u));
    }
    const u32 l = static_cast<u32>(call(ctx, 0x000D93D4u, {o}));
    if (l != 0) {
        Stack fr(ctx, 32);  // the event at sp+112..135
        const u32 ev = fr.addr();
        for (u32 i = 0; i <= 1; ++i) {
            const u32 x = static_cast<u32>(call(ctx, 0x00271C08u, {l, i, 0}));
            if (x == 0 || (load<u32>(x + 192) & 0x2u) == 0 || i != 0 || (load<u32>(x + 188) & 1u) == 0) continue;
            for (u32 k = 0; k < 6; ++k) store<u32>(ev + 4 * k, 0);
            call(ctx, 0x00464F88u, {ev, o, 0, 0, 0});
            call(ctx, 0x00482D14u, {load<u32>(0x00F65CA0u), static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(x + 172)))), ev});
        }
    }
    ret(ctx, 1u);
}

// u32 f(T* o, const T* s) (copy constructor): Thunk_000E27FC(o, s); the
// f32 rows s->+256..+336 -> o->+336..+416 (with w unwritten: 3 of each 4
// words, the last row 5), vtable 0x00B17948; the 68 bytes s->+340 ->
// o->+420; +488 = s->+408; bits 0x80000000, 0x40000000, 0x20000000,
// 0x18000000, 0x04000000, 0x02000000 of +492 from s->+412 (one store
// each, s->+412 re-read); the name s->+222 -> o->+302 (String_CopyN, 32)
// when not empty (r3 = its result, else 0). f1 = s->+336 as the copy
// leaves it.
DECOMP(001D5A40, Obj_CopyConstruct_001D5A40) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    call(ctx, 0x000E27FCu, {o, s});
    for (u32 row = 0; row < 4; ++row) {
        for (u32 k = 0; k < 3; ++k) {
            Cp52(o + 336 + 16 * row + 4 * k, s + 256 + 16 * row + 4 * k);
            if (row == 3 && k == 0) store<u32>(o, 0x00B17948u);
        }
    }
    for (u32 k = 0; k < 5; ++k) Cp52(o + 400 + 4 * k, s + 320 + 4 * k);
    ctx.fpr[1] = Lfs52(s + 336);
    for (u32 k = 0; k < 68; k += 4) Cp52(o + 420 + k, s + 340 + k);
    store<u32>(o + 488, load<u32>(s + 408));
    u32 w = (load<u32>(o + 492) & 0x7FFFFFFFu) | (load<u32>(s + 412) & 0x80000000u);
    store<u32>(o + 492, w);
    static constexpr u32 kBits[5] = {0x40000000u, 0x20000000u, 0x18000000u, 0x04000000u, 0x02000000u};
    for (u32 m : kBits) {
        w = (w & ~m) | (load<u32>(s + 412) & m);
        store<u32>(o + 492, w);
    }
    const u32 c = load<u8>(s + 222);
    if (c == 0) {
        ret(ctx, c);
        return;
    }
    ret(ctx, call(ctx, 0x00553638u, {o + 302, s + 222, 32}));
}

// u32 f(T* o) (destructor body): vtable 0x00B1C050; the attached object
// o->+180 gets bit 0x200 of +136 and +180 = 0; Emitter_Stop(o->+184, 1);
// the PtrVector +160 cleared; the buffer +120 (u16 +112 * (u16 +118 >> 1)
// bytes) handed back to its allocator +124 (vt[8](alloc, buf, size)) when
// set; Array_Destruct(o + 112); vtable 0x00B1BE30; o taken out of the
// entry array 0x01056F1C (Unlist1056F1C_51); r3 = List1013B5C_Remove(o).
DECOMP(00274AC0, Obj_Dtor_00274AC0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 at = load<u32>(o + 180);
    store<u32>(o, 0x00B1C050u);
    if (at != 0) {
        store<u32>(at + 136, load<u32>(at + 136) | 0x200u);
        store<u32>(o + 180, 0);
    }
    call(ctx, 0x00443424u, {load<u32>(o + 184), 1});
    call(ctx, 0x0054EC1Cu, {o + 160});
    call(ctx, 0x0054EC18u, {o + 160});
    const u32 n = load<u16>(o + 112) * ((load<u16>(o + 118) >> 1) & 0x7FFFu);
    const u32 buf = load<u32>(o + 120);
    if (buf != 0) {
        const u32 al = load<u32>(o + 124);
        const u32 opd = load<u32>(load<u32>(al) + 8);
        call_ptr(ctx, opd, {al, buf, n, opd, load<u32>(opd)});
    }
    call(ctx, 0x0054EC94u, {o + 112});
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C_51(o);
    ret(ctx, call(ctx, 0x000D6940u, {o}));
}

// u32 f(T* o, Obj* x) (adds a tracked object): 0 for no x or when x is
// already in the 12-byte entry array o->+356 (u16 cap +360, count +364,
// high-water +366). Else the entry {x, inst, held}: inst =
// WorldObj_Instance(o->+440, 0) when +440 is set (then inst->vt[40](inst,
// 1, 1)), held = 1 when CallK2_000D9614(x) gives c with u8 c->+16 set (then
// c->vt[44](c, 0)); u16 x->+178 counted up. r3 = 1.
DECOMP(002C6B24, Tracker_Add_002C6B24) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if (x == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 n = load<u16>(o + 364), arr = load<u32>(o + 356);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(arr + 12 * i) == x) {
            ret(ctx, 0u);
            return;
        }
    }
    u32 inst = 0, held = 0;
    const u32 w = load<u32>(o + 440);
    if (w != 0) {
        inst = static_cast<u32>(call(ctx, 0x005A05E8u, {w, 0}));
        if (inst != 0) {
            const u32 opd = load<u32>(load<u32>(inst) + 40);
            call_ptr(ctx, opd, {inst, 1, 1, opd, load<u32>(opd)});
        }
    }
    const u32 c = static_cast<u32>(call(ctx, 0x000D9614u, {x}));
    if (c != 0 && load<u8>(c + 16) != 0) {
        held = 1;
        const u32 opd = load<u32>(load<u32>(c) + 44);
        call_ptr(ctx, opd, {c, 0, opd, load<u32>(opd)});
    }
    if (load<u16>(o + 364) == load<u16>(o + 360))
        call(ctx, 0x00551718u, {o + 356, (static_cast<u32>(load<u16>(o + 360)) << 1) & 0x0001FFFEu});
    const u32 e = load<u32>(o + 356) + static_cast<u32>(load<u16>(o + 364)) * 12u;
    store<u8>(e + 8, static_cast<u8>(held));
    const u32 cnt = load<u16>(o + 364) + 1u;
    store<u32>(e, x);
    store<u32>(e + 4, inst);
    const u32 hi = load<u16>(o + 366);
    store<u16>(o + 364, static_cast<u16>(cnt));
    store<u16>(o + 366, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(cnt & 0xFFFFu) ? hi : cnt));
    store<u16>(x + 178, static_cast<u16>(load<u16>(x + 178) + 1));
    ret(ctx, 1u);
}

// void f(Pools* o) (constructor; registers itself in *0x00F657FC): three
// pools on Mem_DefaultHeap() (+0..+12 = 0 then Pool112_Setup_0077AC14(o,
// 32, 0, heap); +16..+28 = 0 then Pool_Init336_0077AAE0(o + 16, 256, 0,
// heap); +32..+44 = 0 then Pool_Init368_0077A9AC(o + 32, 16, 0, heap)); the
// PtrVector +48 (u16 +54 = 4, PtrVector_Init(v, 256, 0, 0)); the array +68
// (u16 cap 32, +70 / +72 = 0, element size u16 +74 = 16, +76 = the buffer
// alloc->vt[0](alloc, 32 * 8, 0, 16) of +80 = Mem_DefaultAllocator(),
// Array_Init); +88 = 0x00F2E998, u16 +84 = 0 then -1, PtrVector +92 (u16
// +98 = 8). r3 = 8 as the original leaves it.
DECOMP(003A0010, Pools_Construct_003A0010) {
    const u32 o = arg<u32>(ctx, 0);
    u32 h = static_cast<u32>(call(ctx, 0x00550E70u));
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 4 * k, 0);
    call(ctx, 0x0077AC14u, {o, 32, 0, h});
    h = static_cast<u32>(call(ctx, 0x00550E70u));
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 16 + 4 * k, 0);
    call(ctx, 0x0077AAE0u, {o + 16, 256, 0, h});
    h = static_cast<u32>(call(ctx, 0x00550E70u));
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 32 + 4 * k, 0);
    call(ctx, 0x0077A9ACu, {o + 32, 16, 0, h});
    call(ctx, 0x005516E0u, {o + 48});
    store<u16>(o + 54, 4);
    call(ctx, 0x00551598u, {o + 48, 256, 0, 0});
    store<u16>(o + 70, 0);
    store<u16>(o + 72, 0);
    store<u16>(o + 68, 32);
    store<u32>(o + 76, 0);
    store<u16>(o + 74, 16);
    const u32 al = static_cast<u32>(call(ctx, 0x00550EE8u));
    const u32 cap = load<u16>(o + 68);
    store<u32>(o + 80, al);
    if (cap != 0) {
        const u32 size = cap * ((load<u16>(o + 74) >> 1) & 0x7FFFu);
        const u32 opd = load<u32>(load<u32>(al));
        store<u32>(o + 76, static_cast<u32>(call_ptr(ctx, opd, {al, size, 0, 16, load<u32>(opd), opd})));
    }
    call(ctx, 0x00558F38u, {o + 68});
    store<u32>(o + 88, 0x00F2E998u);
    store<u16>(o + 84, 0);
    call(ctx, 0x005516E0u, {o + 92});
    store<u16>(o + 98, 8);
    store<u32>(0x00F657FCu, o);
    store<u16>(o + 84, 0xFFFFu);
    ret(ctx, 8u);
}

// bool f(Sensor* o, f32 r, f32 maxdeg, ?, ?, Vec3* p): p within r + q->+132
// of the owner position A ((o->+96)->+144; q = o->+108), and the angle
// (Acosf of the dot product, clamped to [-1, 1], times 57.2958) between the
// owner's forward (o->+96)->+128 and the direction from A to q's position
// ((q->+96)->+144; the default 0x01164B70 when that difference equals the
// vec3 0x01164B40) at most maxdeg. f1 as the original leaves it.
DECOMP(003AC070, Sensor_InCone_003AC070) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 3);
    const f64 r = farg(ctx, 0), mx = farg(ctx, 1);
    const u32 own = load<u32>(o + 96), A = own + 144, q = load<u32>(o + 108);
    const f64 ay = Lfs52(A + 4), ax = Lfs52(A), az = Lfs52(A + 8);
    const f64 dy = F52(Lfs52(p + 4) - ay), dx = F52(Lfs52(p) - ax), dz = F52(Lfs52(p + 8) - az);
    f64 rr = F52(r + Lfs52(q + 132));
    const f64 yy = F52(dy * dy);
    rr = F52(rr * rr);
    f64 d2 = F52(std::fma(dx, dx, yy));
    d2 = F52(std::fma(dz, dz, d2));
    if (!(d2 <= rr)) {
        ctx.fpr[1] = az;
        ret(ctx, 0u);
        return;
    }
    const u32 B = load<u32>(q + 96) + 144;
    const f64 vz = F52(Lfs52(B + 8) - az), vx = F52(Lfs52(B) - ax), vy = F52(Lfs52(B + 4) - ay);
    constexpr u32 K = 0x003AC048u;  // 1.0f, 1.0, -1.0f, -1.0, 57.2958f
    constexpr u32 V = 0x01164B40u, D = 0x01164B70u;
    const u32 bx = std::bit_cast<u32>(static_cast<f32>(vx)), by = std::bit_cast<u32>(static_cast<f32>(vy));
    const u32 bz = std::bit_cast<u32>(static_cast<f32>(vz));
    f64 nx, ny, nz;
    if (bx == load<u32>(V) && by == load<u32>(V + 4) && bz == load<u32>(V + 8)) {
        nx = Lfs52(D);
        ny = Lfs52(D + 4);
        nz = Lfs52(D + 8);
    } else {
        f64 l = F52(vy * vy);
        l = F52(std::fma(vx, vx, l));
        l = F52(std::fma(vz, vz, l));
        l = F52(std::sqrt(l));
        const f64 s = F52(Lfs52(K) / l);
        nx = F52(vx * s);
        ny = F52(vy * s);
        nz = F52(vz * s);
    }
    const u32 m = own + 96;
    f64 c = F52(ny * Lfs52(m + 36));
    c = F52(std::fma(nx, Lfs52(m + 32), c));
    c = F52(std::fma(nz, Lfs52(m + 40), c));
    const f64 one = Lfs52(K), d1 = std::bit_cast<f64>(load<u64>(K + 8));
    const f64 m1 = Lfs52(K + 16), dm1 = std::bit_cast<f64>(load<u64>(K + 24));
    c = F52(F52(one - c) >= 0.0 ? c : d1);  // fsel / frsp clamps
    c = F52(F52(c - m1) >= 0.0 ? c : dm1);
    c = F52(F52(c + one) >= 0.0 ? c : dm1);
    c = F52(F52(c - one) >= 0.0 ? d1 : c);
    callf(ctx, 0x005E649Cu, {m}, {c});  // r3 = owner + 96 as left; Acosf reads f1
    const f64 deg = F52(ctx.fpr[1] * Lfs52(K + 32));
    ctx.fpr[1] = deg;
    ret(ctx, deg <= mx ? 1u : 0u);
}

// u32 f(Char* o) (per-frame sync): with o->+532 == 0 / 1 the hash
// (o->+60)->+4996 / +4984 is set on the effect o->+32 when both are set
// (Effect_SetHash4CF988); Obj_SyncBindingPoses(o, o->+1544, o->+1604);
// unless the attached o->+1584 has the hash 0x27EAE169 at (its +216)->+52,
// the 128-byte entries of o->+104 (u16 count +112) whose +80 is o->+1600
// get +0 = 0x32A3716C and bit 0x80 of u8 +104. When
// Handle_ResolveBit1(*0x00F64CCC) is set, Obj_SetAnim(it, 0x00ADF4FC, 1, .,
// 0, 0, 0x01164CA4) with f1 = f32 0x003BC70C; when bit 0x10 of
// ((*0x00F65CA0) + 80)->+348 is set, Handle_Call363A3C(0x1447B, 0x00ADF514,
// 1, ., 0, 0, 0x01164CA4) with f1 = f32 0x003BC710 and the bit cleared (r3 =
// the word, else 0). r6 goes to Obj_SetAnim as the previous callee left it
// and is 0x10000 for the second call (neither reads it).
DECOMP(003BC718, Char_SyncFrame_003BC718) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 s = static_cast<s32>(load<u32>(o + 532));
    const u32 eff = load<u32>(o + 32);
    if (s == 0 || s == 1) {
        const u32 h = load<u32>(load<u32>(o + 60) + (s == 1 ? 4984u : 4996u));
        if (h != 0 && eff != 0) call(ctx, 0x004CF988u, {eff, h});
    }
    call(ctx, 0x003BB8ACu, {o, load<u32>(o + 1544), load<u32>(o + 1604)});
    const u32 at = load<u32>(o + 1584), key = load<u32>(o + 1600);
    if (at != 0) {
        const u32 x = load<u32>(at + 216);
        const u32 h = x != 0 ? load<u32>(x + 52) : 0u;
        if (h != 0x27EAE169u) {
            for (u32 i = 0; i < load<u16>(o + 112); ++i) {
                const u32 e = load<u32>(o + 104) + 128 * i;
                if (load<u32>(e + 80) != key) continue;
                const u32 b = load<u8>(e + 104);
                store<u32>(e, 0x32A3716Cu);
                store<u8>(e + 104, static_cast<u8>(b | 0x80u));
            }
        }
    }
    constexpr u32 K = 0x003BC70Cu, kStr = 0x01164CA4u;
    if (static_cast<u32>(call(ctx, 0x003568E0u, {load<u32>(0x00F64CCCu)})) != 0) {
        const u32 r = static_cast<u32>(call(ctx, 0x003568E0u, {load<u32>(0x00F64CCCu)}));
        callf(ctx, 0x00363A3Cu, {r, 0x00ADF4FCu, 1, static_cast<u32>(ctx.gpr[6]), 0, 0, kStr}, {Lfs52(K)});
    }
    const u32 w = load<u32>(0x00F65CA0u) + 80;
    if ((load<u32>(w + 348) & 0x10u) == 0) {
        ret(ctx, 0u);
        return;
    }
    callf(ctx, 0x0036735Cu, {0x0001447Bu, 0x00ADF514u, 1, 0x00010000u, 0, 0, kStr}, {Lfs52(K + 4)});  // r6 = the lis half
    const u32 v = load<u32>(w + 348) & ~0x10u;
    store<u32>(w + 348, v);
    ret(ctx, v);
}

// u32 f(Obj* o) (activation step): with u8 o->+53 set, u8 +478 = u8 +476 =
// 1. Otherwise, the first time (bit 0x80000000 of +96 clear, o->+44 set
// with u8 ->+18 != 6, u8 +476 clear): World_StartScene46C3B8(*0x00F65CA0,
// 0xF78970B4, 0 x 8), u8 +476 = u8 +478 = 1, SetSelect_491000(o,
// 0xB3000DC2), (o->+480)->vt[24](., 0, 0), Handle_Notify36742C(0xBB9F16F3,
// 0, 1, 0), SetSelect_491000 again, u8 +87 = 1 and that bit set. Then with
// the effect host e = o->+192 whose vt[272]() is set: u8 +464 = 0 and r3 =
// Effect_Release(e->vt[272](), 0x002237D4) (else r3 as left).
DECOMP(0040E220, Obj_Activate_0040E220) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 53) != 0) {
        store<u8>(o + 478, 1);
        store<u8>(o + 476, 1);
    } else if ((load<u32>(o + 96) & 0x80000000u) == 0 && load<u32>(o + 44) != 0 &&
               load<u8>(load<u32>(o + 44) + 18) != 6 && load<u8>(o + 476) == 0) {
        call(ctx, 0x0046C3B8u, {load<u32>(0x00F65CA0u), 0xF78970B4u, 0, 0, 0, 0, 0, 0, 0, 0});
        store<u8>(o + 476, 1);
        store<u8>(o + 478, 1);
        constexpr u32 kSel = 0xB3000DC2u;
        call(ctx, 0x00491000u, {o, kSel});
        const u32 x = load<u32>(o + 480);
        const u32 opd = load<u32>(load<u32>(x) + 24);
        call_ptr(ctx, opd, {x, 0, 0, opd, load<u32>(opd)});
        call(ctx, 0x0036742Cu, {0xBB9F16F3u, 0, 1, 0});
        call(ctx, 0x00491000u, {o, kSel});
        const u32 w = load<u32>(o + 96);
        store<u8>(o + 87, 1);
        store<u32>(o + 96, w | 0x80000000u);
    }
    const u32 e = load<u32>(o + 192);
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 opd = load<u32>(load<u32>(e) + 272);
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {e, opd, load<u32>(opd)}));
    if (r == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u8>(o + 464, 0);
    const u32 e2 = load<u32>(o + 192);
    opd = load<u32>(load<u32>(e2) + 272);
    const u32 h = static_cast<u32>(call_ptr(ctx, opd, {e2, opd, load<u32>(opd)}));
    ret(ctx, call(ctx, 0x004CF3C4u, {h, 0x002237D4u}));
}

// u32 f(Cmd* c) (applies a save profile to a player): with c->+20 set, the
// player t (Obj_FindByHandle42C2B0(c->+16, 0), or the main player
// ((*0x00F65CA0)->+4208)->+8 when +16 is clear) -> c->+12. s =
// Save_Profile0C9FC8(c->+20): for a flagged player (bit 0x8 of t->+192)
// Player_SetSave124D58(t, s, u8 c->+24 == 0), then bit 0x4000 of t->+172
// when u8 +24 is clear, else with u8 c->+32: t->vt[28](t, 0, 0) and
// Body_SetMode(t->vt[64](), 1) when vt[64]() gives a body; otherwise, with
// bit 0x800 of t->+200, Player_ResetFromDef238270(t, s, u8 +24 == 0). r3 =
// Flag_Set8_40000000(c).
DECOMP(0043A8E4, SeqCmd_ApplySave_0043A8E4) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 sv = load<u32>(c + 20);
    if (sv != 0) {
        const u32 h = load<u32>(c + 16);
        const u32 t = h != 0 ? static_cast<u32>(call(ctx, 0x0042C2B0u, {h, 0}))
                             : load<u32>(load<u32>(0x00F65CA0u) + 4208 + 8);
        const u32 key = h != 0 ? load<u32>(c + 20) : sv;
        store<u32>(c + 12, t);
        if (t != 0) {
            if (load<u32>(t + 192) & 0x8u) {
                const u32 s = static_cast<u32>(call(ctx, 0x000C9FC8u, {key}));
                if (s != 0) {
                    const u32 p = load<u32>(c + 12);
                    call(ctx, 0x00124D58u, {p, s, load<u8>(c + 24) == 0 ? 1u : 0u});
                    if (load<u8>(c + 24) == 0) {
                        store<u32>(p + 172, load<u32>(p + 172) | 0x4000u);
                    } else if (load<u8>(c + 32) != 0) {
                        u32 opd = load<u32>(load<u32>(p) + 28);
                        call_ptr(ctx, opd, {p, 0, 0, opd, load<u32>(opd)});
                        opd = load<u32>(load<u32>(p) + 64);
                        if (static_cast<u32>(call_ptr(ctx, opd, {p, opd, load<u32>(opd)})) != 0) {
                            opd = load<u32>(load<u32>(p) + 64);
                            const u32 b = static_cast<u32>(call_ptr(ctx, opd, {p, opd, load<u32>(opd)}));
                            call(ctx, 0x00261328u, {b, 1});
                        }
                    }
                }
            } else if (load<u32>(t + 200) & 0x800u) {
                const u32 s = static_cast<u32>(call(ctx, 0x000C9FC8u, {key}));
                if (s != 0) call(ctx, 0x00238270u, {load<u32>(c + 12), s, load<u8>(c + 24) == 0 ? 1u : 0u});
            }
        }
    }
    ret(ctx, call(ctx, 0x0042CA60u, {c}));
}

// u32 f(Node* o) (destructor body): vtable 0x00B229E8; the parts +168 /
// +172 get vt[8]() when set; vtable 0x00B22710; while o->+152 (the first
// child) is set it gets vt[4]() (which unlinks it). Unless bit 0x01000000
// of +136 is set, o is unlinked from its parent's child chain (parent +148
// -> its +152, or the root list *0x00F66868; siblings by +156; the walk has
// no end check, as the original) and +156 = 0. Each of the four slots
// 0x010C2AD8 holding o is cleared (all four when o is 0). r3 = 0x010C2AD8.
DECOMP(004E51D4, Node_Dtor_004E51D4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 168);
    store<u32>(o, 0x00B229E8u);
    for (u32 off : {168u, 172u}) {
        const u32 p = off == 168 ? a : load<u32>(o + 172);
        if (p == 0) continue;
        const u32 opd = load<u32>(load<u32>(p) + 8);
        call_ptr(ctx, opd, {p, opd, load<u32>(opd)});
    }
    const u32 c0 = load<u32>(o + 152);
    store<u32>(o, 0x00B22710u);
    for (u32 c = c0; c != 0; c = load<u32>(o + 152)) {
        const u32 opd = load<u32>(load<u32>(c) + 4);
        call_ptr(ctx, opd, {c, opd, load<u32>(opd)});
    }
    if ((load<u32>(o + 136) & 0x01000000u) == 0) {
        constexpr u32 kRoot = 0x00F66868u;
        const u32 par = load<u32>(o + 148);
        const u32 head = par != 0 ? load<u32>(par + 152) : load<u32>(kRoot);
        u32 prev = 0;
        if (head != o) {
            u32 n = load<u32>(head + 156);
            prev = head;
            while (n != o) {
                prev = n;
                n = load<u32>(prev + 156);
            }
        }
        if (prev != 0) {
            store<u32>(prev + 156, load<u32>(o + 156));
        } else if (par != 0) {
            store<u32>(par + 152, load<u32>(o + 156));
        } else {
            store<u32>(kRoot, load<u32>(o + 156));
        }
        store<u32>(o + 156, 0);
    }
    constexpr u32 kSlots = 0x010C2AD8u;
    for (u32 k = 0; k < 4; ++k)
        if (o == 0 || load<u32>(kSlots + 4 * k) == o) store<u32>(kSlots + 4 * k, 0);
    ret(ctx, kSlots);
}
