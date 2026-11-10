// Game code, large-function sweep part 2: the constructor 00769BD8 of a
// large world-object class (vtable 0x00B1E1F8, at least 2572 bytes) from
// its def (the template the XML readers fill; 29 callers via
// New_VtB1E458_0032A3B4 / Obj_NewFromXml_00341510 and others).
#include "big2.h"

using namespace sly4;
using namespace sly4::big2;

namespace b2o {
constexpr u32 Obj_BaseCtor_0048F27C = 0x0048F27C;  // (o): the base constructor (name?)
constexpr u32 Pair_Clear_00321230 = 0x00321230;    // (u32 p[2]): p[0] = p[1] = 0
constexpr u32 String_Replace_00321288 = 0x00321288;  // (String* s, const char*) (name?)
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 SmallAlloc = 0x0055238C;
constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 PtrVector_Init = 0x00551598;     // (vec, capacity, a, b)
constexpr u32 PtrVector_Reserve = 0x00551718;  // (vec, capacity)
constexpr u32 String_CopyN = 0x00553638;       // (dst, src, n)
constexpr u32 String_Copy = 0x00553684;        // (dst, src, size)
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 kVtObj = 0x00B1E1F8;
}  // namespace b2o

namespace {

// The inlined Array push: grow to twice the capacity when full (the u16
// count +8 equal to the u16 capacity +4), returns the slot index (the
// count, re-read after growing).
u32 ArrayRoom(PPUContext& ctx, u32 v) {
    const u32 cnt = load<u16>(v + 8), cap = load<u16>(v + 4);
    if (cnt != cap) return cnt;
    call(ctx, b2o::PtrVector_Reserve, {v, (cap << 1) & 0xFFFFFFFEu});
    return load<u16>(v + 8);
}

// count + 1 into +8 and the high-water mark +10 raised to it (the
// original stores the unmasked count + 1 when it is not below +10).
void ArrayBump(u32 v) {
    const u32 n = static_cast<u32>(load<u16>(v + 8)) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n & 0xFFFFu) ? hw : n));
}

// The PtrVector at *slot, made on first use (SmallAlloc(20) stored to
// the slot even when 0, constructed with elem-byte elements), re-read.
u32 LazyVector(PPUContext& ctx, u32 slot, u16 elem) {
    u32 v = load<u32>(slot);
    if (v != 0) return v;
    v = static_cast<u32>(call(ctx, b2o::SmallAlloc, {20}));
    store<u32>(slot, v);
    if (v == 0) return 0;
    call(ctx, b2o::PtrVector_Construct, {v});
    store<u16>(v + 6, elem);
    return load<u32>(slot);
}

}  // namespace

// u32 Obj_Construct_00769BD8(Obj* o, const Def* d): the base constructor,
// vtable 0x00B1E1F8 and the scalar fields from d; the five slot words
// +2232 cleared; four owned buffers (+2488 / +2500 / +2512 / +2524 {ptr,
// size, x}) freed and re-made as copies of d +312.. (Mem_Alloc + memmove);
// the name hash +2560; for each of d's five slot lists (d +52[i], a holder
// whose +0 is a PtrVector of PtrVectors of 76-byte items) a new holder at
// +2232[i] whose PtrVector gets a deep copy of every list (74 of the 76
// bytes of each item copied, as the original); the 20-byte records of d
// +92 appended to +2272; the arrays +2292 / +2328 / +2364 (9 each) and
// +2416 (9); the strings +28 (127) / +156 / +1180 (1023); the vec4 +2276;
// the use count in the top 12 bits of (o->+12)->+4 bumped (the old value
// into +2452); the string pairs +2544 (count +2536, a counted new[] whose
// String / f32 halves are copied from d +276); the 16-char name +2461.
// Returns String_Copy's result (ignored by the callers).
DECOMP(00769BD8, Obj_Construct_00769BD8) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, b2o::Obj_BaseCtor_0048F27C, {o});
    {
        const u32 d0 = load<u32>(d + 0), d8 = load<u32>(d + 8);
        store<u32>(o + 8, d0);
        const u32 d4 = load<u32>(d + 4);
        store<u8>(o + 16, static_cast<u8>(d8));
        store<u8>(o + 17, 0);
        store<u8>(o + 18, 0);
        const u32 d12 = load<u32>(d + 12);
        store<u32>(o + 20, d12);
        store<u32>(o + 24, d12);
        const u32 d28 = load<u32>(d + 28);
        store<u32>(o + 0, b2o::kVtObj);
        store<u32>(o + 2204, d28);
        const u32 d32 = load<u32>(d + 32);
        store<u32>(o + 12, d4);
        store<u32>(o + 2208, d32);
        const u32 d36 = load<u32>(d + 36);
        const u8 d48 = load<u8>(d + 48);
        store<u32>(o + 2212, d36);
        const u32 d40 = load<u32>(d + 40);
        store<u8>(o + 2228, d48);
        const u8 d49 = load<u8>(d + 49);
        store<u32>(o + 2216, d40);
        const u32 d44 = load<u32>(d + 44);
        store<u8>(o + 2229, d49);
        const u8 d50 = load<u8>(d + 50);
        store<u32>(o + 2220, d44);
        store<u32>(o + 2224, 0);
        store<u8>(o + 2230, d50);
        store<u8>(o + 2231, load<u8>(d + 51));
    }
    for (u32 i = 0; i < 5; ++i) {
        const u32 e = o + 2232 + 8 * i;
        const u32 w = load<u32>(e + 4);
        store<u32>(e + 0, 0);
        store<u8>(e + 4, 0);
        store<u32>(e + 4, w & 0x007FFFFFu);
    }
    {
        store<u32>(o + 2400, load<u32>(d + 220));
        const u32 d224 = load<u32>(d + 224);
        const u8 d232 = load<u8>(d + 232);
        store<u32>(o + 2404, d224);
        const u32 d228 = load<u32>(d + 228);
        store<u32>(o + 2272, 0);
        store<u32>(o + 2408, d228);
        store<u32>(o + 2452, 0);
        const u32 d284 = load<u32>(d + 284);
        store<u8>(o + 2412, d232);
        store<u32>(o + 2456, d284);
        const u8 d272 = load<u8>(d + 272);
        const u32 d304 = load<u32>(d + 304);
        store<u8>(o + 2460, d272);
        const u32 d308 = load<u32>(d + 308);  // lfs / stfs
        store<u32>(o + 2480, d304);
        store<u32>(o + 2484, d308);
        ctx.set_fpr_bits(1, LfsBits(d308));
    }
    for (u32 k = 0; k < 4; ++k) {
        const u32 f = o + 2488 + 12 * k, s = d + 312 + 12 * k;
        if (load<u32>(f) != 0) {
            call(ctx, b2o::Mem_FreeFrom, {load<u32>(f), 0});
            store<u32>(f, 0);
        }
        store<u32>(f + 8, load<u32>(s + 8));
        const u32 size = load<u32>(s + 4);
        store<u32>(f + 4, size);
        const u32 p = static_cast<u32>(call(ctx, b2o::Mem_Alloc, {size, 0, 16}));
        store<u32>(f, p);
        call(ctx, b2o::memmove, {p, load<u32>(s), load<u32>(f + 4)});
    }
    {
        store<u32>(o + 2540, 0);
        const u32 name = load<u32>(d + 16);
        store<u8>(o + 2556, 1);
        store<u8>(o + 2557, load<u8>(d + 273));
        store<u8>(o + 2558, load<u8>(d + 274));
        store<u32>(o + 2560, static_cast<u32>(call(ctx, b2o::String_HashNoCase, {name})));
        store<u32>(o + 2564, 0);
        store<u32>(o + 2568, 0);
    }

    // The five slot lists: deep copies.
    for (u32 i = 0; i < 5; ++i) {
        const u32 src = load<u32>(d + 52 + 8 * i);
        if (src == 0 || load<u32>(src) == 0 || load<u16>(load<u32>(src) + 8) == 0) continue;
        const u32 m = static_cast<u32>(call(ctx, b2o::Mem_Alloc, {4, 0, 16}));
        u32 holder = 0;
        if (m != 0) {
            holder = m;
            store<u32>(m, 0);
        }
        for (u32 j = 0;; ++j) {
            const u32 lists = load<u32>(src);
            if (j >= (lists != 0 ? load<u16>(lists + 8) : 0u)) break;
            const u32 nv = static_cast<u32>(call(ctx, b2o::Mem_Alloc, {32, 0, 16}));
            u32 copy = 0;
            if (nv != 0) {
                call(ctx, b2o::PtrVector_Construct, {nv});
                store<u16>(nv + 6, 76);
                call(ctx, b2o::PtrVector_Init, {nv, 4, 0, 0});
                copy = nv;
                const u32 w = load<u32>(nv + 28);
                store<u32>(nv + 20, 0);
                store<u32>(nv + 24, 0);
                store<u32>(nv + 28, w & 0x03FFFFFFu);
            }
            const u32 list = load<u32>(load<u32>(load<u32>(src)) + 4 * j);
            for (u32 k = 0, off = 0; k < load<u16>(list + 8); ++k, off += 76) {
                const u32 item = load<u32>(list) + off;
                const u32 n = ArrayRoom(ctx, copy);
                call(ctx, b2o::memmove, {load<u32>(copy) + n * 76u, item, 74});
                ArrayBump(copy);
            }
            {
                const u32 s20 = load<u32>(list + 20);
                const u32 w = load<u32>(copy + 28);
                store<u32>(copy + 20, s20);
                u32 nw = (w & 0x7FFFFFFFu) | (load<u32>(list + 28) & 0x80000000u);
                store<u32>(copy + 28, nw);
                nw = (nw & ~0x40000000u) | (load<u32>(list + 28) & 0x40000000u);
                store<u32>(copy + 28, nw);
            }
            const u32 hv = LazyVector(ctx, holder, 4);
            const u32 n = ArrayRoom(ctx, hv);
            const u32 data = load<u32>(hv);
            const u32 n1 = static_cast<u32>(load<u16>(hv + 8)) + 1;
            store<u32>(data + ((n << 2) & 0xFFFFFFFCu), copy);
            const u32 hw = load<u16>(hv + 10);
            store<u16>(hv + 8, static_cast<u16>(n1));
            store<u16>(hv + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
        }
        store<u32>(o + 2232 + 8 * i, holder);
    }

    // d +92: 20-byte records appended to +2272.
    u32 rv = load<u32>(d + 92);
    if (rv != 0 && load<u16>(rv + 8) != 0) {
        for (u32 i = 0, off = 0; i < (rv != 0 ? load<u16>(rv + 8) : 0u); ++i, off += 20) {
            const u32 item = load<u32>(rv) + off;
            const u32 v = LazyVector(ctx, o + 2272, 20);
            const u32 n = ArrayRoom(ctx, v);
            const u32 dst = load<u32>(v) + n * 20u;
            const u32 w0 = load<u32>(item + 0), w1 = load<u32>(item + 4), w2 = load<u32>(item + 8), w3 = load<u32>(item + 12);
            store<u32>(dst + 0, w0);
            store<u32>(dst + 4, w1);
            store<u32>(dst + 8, w2);
            store<u32>(dst + 12, w3);
            store<u32>(dst + 16, load<u32>(item + 16));
            const u32 n1 = static_cast<u32>(load<u16>(v + 8)) + 1;
            const u32 hw = load<u16>(v + 10);
            rv = load<u32>(d + 92);
            store<u16>(v + 8, static_cast<u16>(n1));
            store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
        }
    }

    for (u32 k = 0; k < 9; ++k) {
        const u32 a = load<u32>(d + 112 + 4 * k), f = load<u32>(d + 184 + 4 * k);
        store<u32>(o + 2292 + 4 * k, a);
        const u32 b = load<u32>(d + 148 + 4 * k);
        if ((k & 1) == 0) {  // the original's two-way unrolled loop orders the stores so
            store<u32>(o + 2328 + 4 * k, b);
            store<u32>(o + 2364 + 4 * k, f);
        } else {
            store<u32>(o + 2364 + 4 * k, f);
            store<u32>(o + 2328 + 4 * k, b);
        }
        ctx.set_fpr_bits(1, LfsBits(f));
    }
    for (u32 k = 0; k < 9; ++k) store<u32>(o + 2416 + 4 * k, load<u32>(d + 236 + 4 * k));

    store<u8>(o + 28, 0);
    if (const u32 s = load<u32>(d + 16); s != 0) call(ctx, b2o::String_CopyN, {o + 28, s, 127});
    store<u8>(o + 156, 0);
    if (const u32 s = load<u32>(d + 20); s != 0) call(ctx, b2o::String_CopyN, {o + 156, s, 1023});
    store<u8>(o + 1180, 0);
    if (const u32 s = load<u32>(d + 24); s != 0) call(ctx, b2o::String_CopyN, {o + 1180, s, 1023});
    {
        const u32 w0 = load<u32>(d + 96), w1 = load<u32>(d + 100), w2 = load<u32>(d + 104), w3 = load<u32>(d + 108);
        store<u32>(o + 2276, w0);
        store<u32>(o + 2280, w1);
        store<u32>(o + 2284, w2);
        store<u32>(o + 2288, w3);
    }
    {
        const u32 x = load<u32>(o + 12);
        store<u32>(o + 2452, load<u32>(x + 4) >> 20);
        const u32 w = load<u32>(x + 4);
        store<u32>(x + 4, (w & 0x000FFFFFu) | ((((w >> 20) + 1) << 20) & 0xFFF00000u));
    }
    const u32 np = load<u32>(d + 280);
    store<u32>(o + 2544, 0);
    store<u32>(o + 2536, np);
    if (np != 0) {
        u32 p = static_cast<u32>(call(ctx, b2o::Mem_Alloc, {((np << 3) & 0xFFFFFFF8u) + 4, 0, 16}));
        if (p != 0) {
            store<u32>(p, np);
            p += 4;
            for (s32 k = 0; k < static_cast<s32>(np); ++k) call(ctx, b2o::Pair_Clear_00321230, {p + 8 * static_cast<u32>(k)});
        }
        store<u32>(o + 2544, p);
        for (s32 k = 0; k < static_cast<s32>(load<u32>(o + 2536)); ++k) {
            const u32 dst = load<u32>(o + 2544) + 8 * static_cast<u32>(k);
            const u32 s = load<u32>(d + 276) + 8 * static_cast<u32>(k);
            const u32 f = load<u32>(s + 4);  // lfs / stfs
            const u32 str = load<u32>(s);
            store<u32>(dst + 4, f);
            ctx.set_fpr_bits(1, LfsBits(f));
            call(ctx, b2o::String_Replace_00321288, {dst, str});
        }
    }
    call(ctx, b2o::String_Copy, {o + 2461, d + 288, 16});
}
