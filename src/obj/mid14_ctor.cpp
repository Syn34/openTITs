// Game code, medium sweep part 14: record pushes and constructors.
#include "mid14.h"

using namespace sly4;
using namespace sly4::mid14;

namespace {
constexpr u32 String_Copy553684 = 0x00553684;   // (dst, src, n)
constexpr u32 Vector_Add733964 = 0x00733964;    // (vec, const elem*)
constexpr u32 kNegOne74FA78 = 0x0074FA78;       // f32 -1
}  // namespace

// u32 Vector_Push144_76A618(Vector* v, const Rec144* s)
// (called by 0032B358) Appends a 144-byte record (grows to twice the
// capacity when full): words +0..+44 (not +12), byte +48, words +52..+72,
// +80, +96..+104, +112, byte +116, +120 / +124, the top 9 bits of +128 one
// at a time (the rest of the destination word kept), +132. Returns the new
// index.
DECOMP(0076A618, Vector_Push144_76A618) {
    const u32 v = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u32 d = VecSlot(ctx, v, 144);
    for (u32 k : {0u, 4u, 8u, 16u, 20u, 24u, 28u, 32u, 36u, 40u, 44u}) Cw(d + k, s + k);
    store<u8>(d + 48, load<u8>(s + 48));
    for (u32 k : {52u, 56u, 60u, 64u, 68u, 72u, 80u, 96u, 100u, 104u, 112u}) Cw(d + k, s + k);
    store<u8>(d + 116, load<u8>(s + 116));
    Cw(d + 120, s + 120);
    Cw(d + 124, s + 124);
    BitCopy(d + 128, s + 128, 9);
    Cw(d + 132, s + 132);
    SetF1(ctx, s + 132);  // leftover: the last lfs
    ret(ctx, VecPushTail(v));
}

// u32 Vector_Push256_7617C0(Vector* v, const Rec256* s)
// (called by 002D3850) Appends a 256-byte record: word +0, bytes +16..+143,
// words +144..+152, +160..+188, the top 8 bits of +192 one at a time, +196,
// +208..+240. Returns the new index.
DECOMP(007617C0, Vector_Push256_7617C0) {
    const u32 v = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u32 d = VecSlot(ctx, v, 256);
    Cw(d, s);
    for (u32 k = 16; k < 144; ++k) store<u8>(d + k, load<u8>(s + k));
    for (u32 k : {144u, 148u, 152u, 160u, 164u, 168u, 172u, 176u, 180u, 184u, 188u}) Cw(d + k, s + k);
    BitCopy(d + 192, s + 192, 8);
    Cw(d + 196, s + 196);
    for (u32 k = 208; k <= 240; k += 4) Cw(d + k, s + k);
    SetF1(ctx, s + 240);  // leftover: the last lfs
    ret(ctx, VecPushTail(v));
}

// Rec* Rec_Construct74DC64(Rec* o, a, b, c, u32 w7, f32 f1..f13, stack:
//     f32 x18..x23 (sp+196..+236), const u8* bytes6 (sp+244), six flags
//     (sp+252..+292))
// (called by 001763F0, 00176CC0) o+12..+20 = a, b, c; bytes +24..+31 0,
// +32 0, +36 / +40 / +72 -1.0, byte +44 0, +48..+68 0, +4 / +8 0, +76 =
// f7, +80 / +84 / +108..+116 0, u16 +120 = (old | 0x8000 | 0x200) &
// ~0x7DF0, u16 +122 0, +0 = 2; floats +88 + 4i = ((u16 +120 >> 4) & 15) - i
// for i = 0..4 (that field reread). Then +124 = w7, +128..+148 = f1..f6,
// +152..+172 = f8..f13, +176..+196 the stack floats, +208 0, +204 with
// bits 0x8000 >> k = flag k & 1, +212 0, +216 -45.0, +220 45.0, +0 |=
// 0x800 and bytes +200..+205 from bytes6 (1 each when it is null).
// Returns o.
DECOMP(0074DC64, Rec_Construct74DC64) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), w7 = arg<u32>(ctx, 4);
    u32 h = load<u16>(o + 120) | 0x8000u;
    h = (h & ~0x7C00u) | 0x200u;
    h &= ~0x1F0u;  // 0x100 and 0xF0 cleared
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    for (u32 k = 24; k <= 31; ++k) store<u8>(o + k, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0xBF800000u);
    store<u32>(o + 40, 0xBF800000u);
    store<u8>(o + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 72, 0xBF800000u);
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    StoreF(o + 76, ctx.fpr[7]);
    store<u32>(o + 80, 0);
    store<u32>(o + 84, 0);
    store<u32>(o + 108, 0);
    store<u32>(o + 112, 0);
    store<u32>(o + 116, 0);
    store<u16>(o + 120, static_cast<u16>(h));
    store<u16>(o + 122, 0);
    store<u32>(o, 2);
    for (u32 i = 0; i < 5; ++i) {
        const u32 x = i == 0 ? h : load<u16>(o + 120);  // the first from the register
        const s32 t = static_cast<s32>(((x >> 4) & 0xFu) - i);
        StoreF(o + 88 + 4 * i, static_cast<f64>(static_cast<s64>(t)));
    }
    const u32 w0 = load<u32>(o);
    u32 f = load<u32>(o + 204);
    store<u32>(o + 124, w7);
    for (u32 k = 0; k < 6; ++k) StoreF(o + 128 + 4 * k, ctx.fpr[1 + k]);
    for (u32 k = 0; k < 6; ++k) StoreF(o + 152 + 4 * k, ctx.fpr[8 + k]);
    for (u32 k = 0; k < 6; ++k) Cw(o + 176 + 4 * k, static_cast<u32>(ctx.gpr[1]) + 196 + 8 * k);
    for (u32 k = 0; k < 6; ++k) f = (f & ~(0x8000u >> k)) | ((SArg(ctx, 252 + 8 * k) & 1u) << (15 - k));
    store<u32>(o + 208, 0);
    store<u32>(o + 204, f);
    store<u32>(o + 212, 0);
    store<u32>(o + 216, 0xC2340000u);
    store<u32>(o + 220, 0x42340000u);
    store<u32>(o, w0 | 0x800u);
    const u32 src = SArg(ctx, 244);
    for (u32 k = 0; k < 6; ++k) store<u8>(o + 200 + k, src == 0 ? 1 : load<u8>(src + k));
    ret(ctx, o);
}

// u32 Node_Construct74FA80(Node* o, const char* name, const Node* src, a6,
//     const f32 m[16], const f32 v[3], u32 b9, u32 b10, u32 s30, u32 s31)
//     (s30 / s31 on the stack)
// (called by 001A7C70) o+16 0, vtable 0x00B12260, rows of m (+0 / +16 /
// +32 / +48, three floats each) to +32 / +48 / +64 / +80, v to +96, +112 =
// a6, +116 = s30, +120 = src, +124 / +128 = src's +36 / +40 (or -1.0). Word
// +132: top bits b9 & 1, b10 & 1, bit 2 of src's byte +32 (0 without src;
// the word is stored once before that bit when src is set), bits 3..10
// set, the rest kept. +136 / +4..+12 0, name copied to +140 (40 bytes,
// byte 0 cleared first). Each 192-byte element of src's vector +136 is
// added (0x00733964) to the vector +16, made on first use (SmallAlloc(20),
// element size 192; a failed allocation leaves +16 0 and passes 0). Last:
// vtable 0x00B15180, +180 = s31, +8 |= 0x800. Returns the new +8.
DECOMP(0074FA80, Node_Construct74FA80) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), src = arg<u32>(ctx, 2), a6 = arg<u32>(ctx, 3);
    const u32 m = arg<u32>(ctx, 4), vv = arg<u32>(ctx, 5), b9 = arg<u32>(ctx, 6), b10 = arg<u32>(ctx, 7);
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B12260u);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cw(o + 32 + 16 * r + 4 * k, m + 16 * r + 4 * k);
    const u32 s30 = SArg(ctx, 116), s31 = SArg(ctx, 124);  // args 9 and 10 (the original's sp+292 / +300 after its 176-byte frame)
    u32 w = load<u32>(o + 132);
    Cw(o + 96, vv);
    Cw(o + 100, vv + 4);
    store<u32>(o + 112, a6);
    Cw(o + 104, vv + 8);
    store<u32>(o + 116, s30);
    store<u32>(o + 120, src);
    Cw(o + 124, src != 0 ? src + 36 : kNegOne74FA78);
    const u32 w128 = src != 0 ? src + 40 : kNegOne74FA78;
    Cw(o + 128, w128);
    w = (w & 0x7FFFFFFFu) | ((b9 & 1u) << 31);
    w = (w & ~0x40000000u) | ((b10 & 1u) << 30);
    u32 bit = 0;
    if (src != 0) {
        store<u32>(o + 132, w);
        bit = (load<u8>(src + 32) >> 2) & 1u;
    }
    w = (w & ~0x20000000u) | (bit << 29);
    store<u32>(o + 136, 0);
    w |= 0x1FE00000u;
    store<u32>(o + 132, w);
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    store<u32>(o + 12, 0);
    store<u8>(o + 140, 0);
    SetF1(ctx, w128);  // leftover f1
    if (name != 0) call(ctx, String_Copy553684, {o + 140, name, 40});
    if (src != 0) {
        u32 vec = load<u32>(src + 136);
        if (vec != 0 && load<u16>(vec + 8) != 0) {
            for (u32 i = 0, off = 0; i < (vec != 0 ? static_cast<u32>(load<u16>(vec + 8)) : 0u); ++i, off += 192) {
                const u32 e = load<u32>(vec) + off;
                u32 dv = load<u32>(o + 16);
                if (dv == 0) {
                    dv = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {20}));
                    store<u32>(o + 16, dv);
                    if (dv != 0) {
                        call(ctx, m1fn::PtrVector_Construct, {dv});
                        store<u16>(dv + 6, 192);
                        dv = load<u32>(o + 16);
                    }
                }
                call(ctx, Vector_Add733964, {dv, e});
                vec = load<u32>(src + 136);
            }
        }
    }
    store<u32>(o, 0x00B15180u);
    const u32 w8 = load<u32>(o + 8);
    store<u32>(o + 180, s31);
    store<u32>(o + 8, w8 | 0x800u);
    ret(ctx, w8 | 0x800u);
}

// u32 Cover_Collect2E2438(const Owner* o, Vector* out)
// (called by 003B94C8) From the last of o's +24 860-byte records (array
// +20) backwards, the first with byte +828 set whose type (u16 at game +
// 4056 * game[4208] + 496 + 4 * byte +822, bits 3..4) is not 0; then from
// the last of its +720 536-byte entries (array +716) backwards, the first
// whose state byte (game + 4056 * game[4208] + 544 + byte +502) is not 0.
// Its 12-byte items (+448, u16 count +456) with byte +10 == 0, or with the
// state byte (re-read) == 2, are pushed onto out (memmove of 11 bytes,
// grown to twice the capacity when full). Returns o when it has no
// records, the game pointer when nothing matched, else the item count.
DECOMP(002E2438, Cover_Collect2E2438) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    u32 n = load<u32>(o + 24);
    if (n == 0) {
        ret(ctx, o);
        return;
    }
    const u32 g = load<u32>(kGlobF65CA0);
    u32 e = n * 860 + load<u32>(o + 20);
    u32 row = 0;
    for (;;) {
        e -= 860;
        if (load<u8>(e + 828) != 0) {
            row = load<u8>(g + 4208) * 4056u;
            if (((load<u16>(g + row + 4u * load<u8>(e + 822) + 496) >> 3) & 3u) != 0) break;
        }
        if (--n == 0) {
            ret(ctx, g);
            return;
        }
    }
    u32 m = load<u8>(e + 720);
    u32 ent = 0;
    for (;; --m) {
        if (m == 0) {
            ret(ctx, g);
            return;
        }
        ent = load<u32>(e + 716) + (m - 1) * 536;
        if (load<u8>(g + row + load<u8>(ent + 502) + 544) != 0) break;
    }
    u32 cnt;
    for (u32 i = 0, off = 0; i < (cnt = load<u16>(ent + 456)); ++i, off += 12) {
        const u32 it = load<u32>(ent + 448) + off;
        if (load<u8>(it + 10) != 0) {
            const u32 g2 = load<u32>(kGlobF65CA0);
            const u32 b = load<u8>(ent + 502);
            if (load<u8>(g2 + load<u8>(g2 + 4208) * 4056u + b + 544) != 2) continue;
        }
        call(ctx, mid12::m12fn::Memmove, {VecSlot(ctx, out, 12), it, 11});
        VecPushTail(out);
    }
    ret(ctx, cnt);
}

// u32 Spawner_Release33CB98(Spawner* o, Obj* key)
// (called by Call_33CB98_0 / _1) Removes key from the u32 list o+112
// (u16 count +120, the last entry moved into its place) and pushes it onto
// the vector +132 (grown to twice the capacity when full). Then for the
// first entry pointer of the list o+72 (u16 count +80) whose word +0 is
// key: when the entry is set, a random delay lo + (hi - lo) * rand (o+56 /
// o+60; rand from the 8-byte state at 0x01164888 as an s64 times f32
// 0x0033CB94, the state then advanced: u32 0x01164890 x -> rotl16(y), the
// 8 bytes -> y & 0xFFFF, y = (rotl4(x) + rotl16(x)) ^ 0x87654321), the
// object's slot-28 virtual (key, 0) when its word +0 is set, entry +0 = 0
// and +4 = the delay; byte +8 of the entry = 1 (address 8 when the entry
// is 0, the original's quirk). Returns that entry, or u16 o+80 when no
// entry matched.
DECOMP(0033CB98, Spawner_Release33CB98) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 cnt = load<u16>(o + 120), arr = load<u32>(o + 112);
    for (u32 i = 0; i < cnt; ++i) {
        if (load<u32>(arr + 4 * i) != key) continue;
        const u32 c1 = cnt - 1;
        store<u16>(o + 120, static_cast<u16>(c1));
        if (i != (c1 & 0xFFFFu)) store<u32>(arr + (i << 2), load<u32>(arr + ((c1 << 2) & 0x3FFFCu)));
        const u32 v = o + 132;
        u32 n = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (n == cap) {
            call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x0001FFFEu});
            n = load<u16>(v + 8);
        }
        store<u32>(load<u32>(v) + ((n << 2) & 0x3FFFCu), key);
        VecPushTail(v);
        break;
    }
    u32 i2 = 0, r3;
    for (;; ++i2) {
        r3 = load<u16>(o + 80);
        if (!(i2 < r3)) {
            ret(ctx, r3);
            return;
        }
        if (load<u32>(load<u32>(load<u32>(o + 72) + 4 * i2)) == key) break;
    }
    u32 list = load<u32>(o + 72);
    const u32 e = load<u32>(list + 4 * i2);
    if (e != 0) {
        const u32 st = 0x01164888u;
        const s64 seed = static_cast<s64>(load<u64>(st));
        const f64 lo = Lf(o + 56);
        const f64 span = S(Lf(o + 60) - lo);
        const u32 x = load<u32>(st + 8);
        const u32 y = (std::rotl(x, 4) + std::rotl(x, 16)) ^ 0x87654321u;
        const f64 rnd = S(static_cast<f64>(seed));
        store<u32>(st + 8, std::rotl(y, 16));
        store<u64>(st, y & 0xFFFFu);
        const u32 obj = load<u32>(e);
        const f64 dl = Fmadds(span, S(rnd * Lf(0x0033CB94u)), lo);
        ctx.fpr[1] = span;  // leftover f1
        if (obj != 0) {
            VCall(ctx, obj, 28, {obj, 0});
            list = load<u32>(o + 72);
        }
        store<u32>(e, 0);
        StoreF(e + 4, dl);
    }
    const u32 e2 = load<u32>(list + 4 * i2);
    store<u8>(e2 + 8, 1);
    ret(ctx, e2);
}

namespace {
constexpr u32 Owner_Construct1ADD08 = 0x001ADD08;      // (o, def)
constexpr u32 StateMachine_ConstructThunk = 0x0045CBE8;  // (o, 1, 0, 0)
constexpr u32 SM_SetState_003044B0 = 0x003044B0;       // (o, u32 hash) (reads r3 / r4) (name?)

// Push the u32 w onto the PtrVector v (+0 data, +4 capacity, +8 count, +10
// high-water; slot index masked 0xFFFF as the original's (n << 2) &
// 0x3FFFC). Returns the old high-water mark (what r3 holds after).
u32 PushU32(PPUContext& ctx, u32 v, u32 w) {
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x0001FFFEu});
        n = load<u16>(v + 8);
    }
    store<u32>(load<u32>(v) + ((n << 2) & 0x3FFFCu), w);
    const u32 c = load<u16>(v + 8) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(c));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c & 0xFFFFu) ? hw : c));
    return hw;
}
}  // namespace

// u32 Owner_Init1C2BE0(Owner* o, const OwnerDef* def)
// (called by Owner_New1C2E20) Owner_Construct1ADD08, vtables 0x00B16B48 /
// +380 0x00B16D1C / +384 0x00B16D44, PtrVectors +556 / +576 (capacity 4),
// byte +604 bits 0x80 / 0x40 / 0x20 cleared, +596 0, +600 = def+240. With
// the def's +32 template: for each u32 id of its list +420 (u16 count
// +428) below the count of the table at o+388, the object at +28 of that
// table's 48-byte entry id (when set) is pushed onto +576; then bit 0x40
// of byte +604 = (u16 template+408 != 0), returned. Without one returns the
// byte +604.
DECOMP(001C2BE0, Owner_Init1C2BE0) {
    const u32 o = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1);
    call(ctx, Owner_Construct1ADD08, {o, def});
    const u32 vt = 0x00B16B40u;
    store<u32>(o, vt + 8);
    store<u32>(o + 380, vt + 476);
    store<u32>(o + 384, vt + 516);
    mid1::PtrVecNew(ctx, o + 556, 4, 4, 1);
    mid1::PtrVecNew(ctx, o + 576, 4, 4, 1);
    const u8 b = static_cast<u8>(load<u8>(o + 604) & ~0xE0u);
    store<u32>(o + 596, 0);
    Cw(o + 600, def + 240);
    SetF1(ctx, def + 240);  // leftover: the last lfs
    store<u8>(o + 604, b);
    const u32 t = load<u32>(def + 32);
    if (t == 0) {
        ret(ctx, static_cast<u32>(b));
        return;
    }
    for (u32 i = 0; i < load<u16>(t + 428); ++i) {
        const u32 id = load<u32>(load<u32>(t + 420) + 4 * i);
        const u32 tab = load<u32>(o + 388);
        if (!(id < (tab != 0 ? static_cast<u32>(load<u16>(tab + 8)) : 0u))) continue;
        u32 j = 0;
        for (; j < (tab != 0 ? static_cast<u32>(load<u16>(tab + 8)) : 0u) && j != id; ++j) {
        }
        if (j != id) continue;
        const u32 obj = load<u32>(load<u32>(tab) + j * 48 + 28);
        if (obj == 0) continue;
        PushU32(ctx, o + 576, obj);
    }
    const u32 f = load<u16>(t + 408) != 0 ? 1u : 0u;
    store<u8>(o + 604, static_cast<u8>((load<u8>(o + 604) & ~0x40u) | (f << 6)));
    ret(ctx, f);
}

// u32 StateCtl_Construct304654(StateCtl* o, u32 a)
// (called by 00305A44) StateMachine_ConstructThunk(o, 1, 0, 0), +28 0, +32
// = a, vtable 0x00B1DCA8, +36 / +40 0, PtrVector +44 (capacity 16, u32),
// vtable 0x00B1DFF0, PtrVector +64 (capacity 16, bytes), +84 0, bytes
// +88..+91 0, +92..+120 0; 0x003044B0(o, 0x2FAE6EF3); then the hash
// (String_HashNoCase) of each set name of the five at 0x00AD9AFC pushed onto
// +44. Returns the last push's old high-water mark (else 0x003044B0's r3).
DECOMP(00304654, StateCtl_Construct304654) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    call(ctx, StateMachine_ConstructThunk, {o, 1, 0, 0});
    store<u32>(o + 28, 0);
    store<u32>(o + 32, a);
    store<u32>(o, 0x00B1DCA8u);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    mid1::PtrVecNew(ctx, o + 44, 16, 4, 1);
    store<u32>(o, 0x00B1DFF0u);
    mid1::PtrVecNew(ctx, o + 64, 16, 1, 1);
    store<u32>(o + 84, 0);
    for (u32 k = 88; k <= 91; ++k) store<u8>(o + k, 0);
    for (u32 k = 92; k <= 120; k += 4) store<u32>(o + k, 0);
    u64 r = call(ctx, SM_SetState_003044B0, {o, 0x2FAE6EF3u});
    for (u32 k = 0; k < 5; ++k) {
        const u32 s = load<u32>(0x00AD9AFCu + 4 * k);
        if (s == 0) continue;
        const u32 h = static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {s}));
        r = PushU32(ctx, o + 44, h);
    }
    ret(ctx, r);
}
