// Game code, medium sweep part 24: constructors and a query copier.
#include "mid24.h"

using namespace sly4;
using namespace sly4::mid24;

namespace {
constexpr u32 kPtrVectorReserve = 0x00551718u;    // PtrVector_Reserve(v, n)
constexpr u32 kSmallAlloc = 0x0055238Cu;          // SmallAlloc(size)
constexpr u32 kPtrVectorConstruct = 0x005516E0u;  // PtrVector_Construct(v)
constexpr u32 kPtrVectorInit = 0x00551598u;       // PtrVector_Init(v, n, a, b)
constexpr u32 kMemmove = 0x0071C3F0u;             // memmove(dst, src, n)
constexpr u32 kMemset = 0x00726E54u;              // memset(p, c, n)
constexpr u32 kStringCopyN = 0x00553638u;         // String_CopyN(dst, src, n)
constexpr u32 kSaveProfile = 0x000C9FC8u;         // Save_Profile0C9FC8(hash)
constexpr u32 kHashNoCase = 0x0056A710u;          // String_HashNoCase(s)
constexpr u32 kVec4B40 = 0x01164B40u;             // f32[3] default vector

// PtrVector_Construct(v), element size u16 v->6 = es, PtrVector_Init(v, n, 0, 1).
void InitVec(PPUContext& ctx, u32 v, u16 es, u32 n) {
    call(ctx, kPtrVectorConstruct, {v});
    store<u16>(v + 6, es);
    call(ctx, kPtrVectorInit, {v, n, 0, 1});
}

// The default vector's three words at d.
void DefVec(u32 d) {
    Cw(d, kVec4B40);
    Cw(d + 4, kVec4B40 + 4);
    Cw(d + 8, kVec4B40 + 8);
}
}  // namespace

// u32 Obj_CopyLightQuery_0021EEA4(const Obj* o, Query* q): the inverse of
// the 0021E620 / 002219A8 light query: Obj_CopyQuery_000CDB28(o, q), q->144
// = o->180, the bools q->148..153 = bits 0x80 / 0x40 / 0x20 / 0x10 / 0x08 /
// 0x02 of u8 o->184 (re-read each time), q->176 / 180 = o->208 / 212, the
// f32 q->184..196 = o->216..228, q->200 = o->232; then the u16 o->196
// 76-byte records of o->188 appended (74 bytes each, memmove) to the vector
// q+156 (capacity +160 doubled with PtrVector_Reserve when full, count
// +164, high-water +166). r3 = the final count as the original leaves it;
// f1 = o->228.
DECOMP(0021EEA4, Obj_CopyLightQuery_0021EEA4) {
    const u32 o = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28u, {o, q});
    store<u32>(q + 144, load<u32>(o + 180));
    auto fl = [&](u32 m) -> u8 { return (load<u8>(o + 184) & m) != 0 ? 1 : 0; };
    store<u8>(q + 148, fl(0x80));
    store<u8>(q + 149, fl(0x40));
    store<u8>(q + 150, fl(0x20));
    store<u8>(q + 151, fl(0x10));
    const u32 f216 = load<u32>(o + 216), w208 = load<u32>(o + 208);
    store<u8>(q + 152, fl(0x08));
    const u8 b153 = fl(0x02);
    store<u32>(q + 184, f216);
    store<u32>(q + 176, w208);
    const u32 f220 = load<u32>(o + 220), w212 = load<u32>(o + 212);
    store<u32>(q + 188, f220);
    store<u32>(q + 180, w212);
    const u32 f224 = load<u32>(o + 224);
    store<u8>(q + 153, b153);
    store<u32>(q + 192, f224);
    const u32 f228 = load<u32>(o + 228), w232 = load<u32>(o + 232);
    store<u32>(q + 196, f228);
    store<u32>(q + 200, w232);
    ctx.fpr[1] = LfsF(f228);
    const u32 vec = q + 156;
    u32 cnt = load<u16>(q + 164);
    u32 prev = cnt;  // the count as last computed (r28), compared to the capacity
    u32 r3 = cnt;
    for (u32 i = 0; i < load<u16>(o + 196); ++i) {
        const u32 src = load<u32>(o + 188) + 76 * i;
        if ((prev & 0xFFFFu) == load<u16>(q + 160)) {
            call(ctx, kPtrVectorReserve, {vec, (static_cast<u32>(load<u16>(q + 160)) << 1) & 0x1FFFEu});
            cnt = load<u16>(q + 164);
        }
        call(ctx, kMemmove, {load<u32>(vec) + 76 * (cnt & 0xFFFFu), src, 74});
        const u32 c1 = static_cast<u32>(load<u16>(q + 164)) + 1;
        const u32 hi = load<u16>(q + 166);
        store<u16>(q + 164, static_cast<u16>(c1));
        u32 m;
        if (static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFFu)) {
            m = hi;
            prev = c1 & 0xFFFFu;
        } else {
            m = c1;
            prev = c1;
        }
        store<u16>(q + 166, static_cast<u16>(m));
        cnt = c1;
        r3 = c1;
    }
    ctx.gpr[3] = r3;
}

// void VecRec_Construct768298(Rec* o): three default vectors at +0 / +16
// / +32, +48 / +52 = 0, +56 = -1, a 256-byte SmallAlloc buffer (+60 begin,
// +64 end, +68 / +72 cursor = begin, +76 = 16, +80 = 0), the default vector
// at +96, then nine Field_SetByte0_4_8f_004A08A4(o + 112 + 12k, 0; f) with
// f = K[0], K[1], K[1], K[2], K[3] x 5 (K = 0x00768288). r3 as the last
// call leaves it; f1 = K[3].
DECOMP(00768298, VecRec_Construct768298) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x00768288u;
    DefVec(o);
    DefVec(o + 16);
    Cw(o + 32, kVec4B40);
    store<u32>(o + 48, 0);
    Cw(o + 36, kVec4B40 + 4);
    store<u32>(o + 52, 0);
    Cw(o + 40, kVec4B40 + 8);
    store<u32>(o + 56, 0xFFFFFFFFu);
    const u32 b = static_cast<u32>(call(ctx, kSmallAlloc, {256}));
    store<u32>(o + 60, b);
    Cw(o + 96, kVec4B40);
    store<u32>(o + 68, b);
    store<u32>(o + 72, b);
    store<u32>(o + 64, b + 256);
    store<u32>(o + 76, 16);
    Cw(o + 100, kVec4B40 + 4);
    store<u32>(o + 80, 0);
    Cw(o + 104, kVec4B40 + 8);
    const f64 f[9] = {Lf(K), Lf(K + 4), Lf(K + 4), Lf(K + 8), Lf(K + 12), Lf(K + 12), Lf(K + 12), Lf(K + 12), Lf(K + 12)};
    for (u32 k = 0; k < 9; ++k) callf(ctx, 0x004A08A4u, {o + 112 + 12 * k, 0}, {f[k]});
    ctx.fpr[1] = f[8];
}

// u32 Rec_Construct304D94(Rec* o, u32 a): +0 = a, default vectors at +16
// and +32, +48 = 0, PtrVectors of 4-byte elements at +52 (PtrVector_Init(v,
// 512, 0, 1)) and +72 (PtrVector_Init(v, 0, 0, 1)), default vectors at +112
// and +128, +92 / +96 / +144..160 / +176..184 = 0, f32 +164 = *0x01069EF8;
// then the Save_Profile0C9FC8 entries for 0xD4A3F67A / 0xB267EAFB go to
// +176 / +180 when their +20 has bit 0x2000, the one for 0x8F7C1D39 to +184
// when found. r3 = that last lookup; f1 = *0x01069EF8.
DECOMP(00304D94, Rec_Construct304D94) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    Cw(o + 16, kVec4B40);
    Cw(o + 20, kVec4B40 + 4);
    store<u32>(o, a);
    Cw(o + 24, kVec4B40 + 8);
    Cw(o + 32, kVec4B40);
    Cw(o + 36, kVec4B40 + 4);
    store<u32>(o + 48, 0);
    Cw(o + 40, kVec4B40 + 8);
    InitVec(ctx, o + 52, 4, 512);
    InitVec(ctx, o + 72, 4, 0);
    DefVec(o + 112);
    store<u32>(o + 92, 0);
    Cw(o + 128, kVec4B40);
    store<u32>(o + 96, 0);
    Cw(o + 132, kVec4B40 + 4);
    store<u32>(o + 144, 0);
    Cw(o + 136, kVec4B40 + 8);
    for (u32 k = 1; k < 5; ++k) store<u32>(o + 144 + 4 * k, 0);
    store<u32>(o + 176, 0);
    Cw(o + 164, 0x01069EF8u);
    ctx.fpr[1] = Lf(0x01069EF8u);
    store<u32>(o + 180, 0);
    store<u32>(o + 184, 0);
    if (const u32 s = static_cast<u32>(call(ctx, kSaveProfile, {0xD4A3F67Au})); s != 0 && (load<u32>(s + 20) & 0x2000u) != 0)
        store<u32>(o + 176, s);
    if (const u32 s = static_cast<u32>(call(ctx, kSaveProfile, {0xB267EAFBu})); s != 0 && (load<u32>(s + 20) & 0x2000u) != 0)
        store<u32>(o + 180, s);
    if (const u32 s = static_cast<u32>(call(ctx, kSaveProfile, {0x8F7C1D39u})); s != 0) store<u32>(o + 184, s);
}

// Obj* Obj_New33DC0C(-, b, c): a 2816-byte SmallAlloc object p:
// Obj_Construct_00769BD8(p, c), vtable 0x00B1EFD0, 4-byte-element
// PtrVectors at +2572 (16) and +2592 (32), +2612 / 2616 / 2628 / 2632 = 0,
// +2620 / 2624 = 1.0f, the four bytes 0x01164CA0 at +2764 and +2768, a
// 4-byte-element PtrVector at +2772 (16) and a 32-byte one at +2792 (32),
// u8 +2812 = 0, memset(+2636, 0, 64), memset(+2700, 0, 64); then
// 0x0033D4E4(p, b) (null p too). r3 = p.
DECOMP(0033DC0C, Obj_New33DC0C) {
    const u32 b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    u32 r = 0;
    if (const u32 p = static_cast<u32>(call(ctx, kSmallAlloc, {2816})); p != 0) {
        call(ctx, 0x00769BD8u, {p, c});
        store<u32>(p, 0x00B1EFD0u);
        InitVec(ctx, p + 2572, 4, 16);
        InitVec(ctx, p + 2592, 4, 32);
        store<u32>(p + 2612, 0);
        store<u32>(p + 2616, 0);
        store<u32>(p + 2628, 0);
        store<u32>(p + 2632, 0);
        store<u32>(p + 2620, 0x3F800000u);
        store<u32>(p + 2624, 0x3F800000u);
        for (u32 k = 0; k < 4; ++k) store<u8>(p + 2764 + k, load<u8>(0x01164CA0u + k));
        for (u32 k = 0; k < 4; ++k) store<u8>(p + 2768 + k, load<u8>(0x01164CA0u + k));
        InitVec(ctx, p + 2772, 4, 16);
        InitVec(ctx, p + 2792, 32, 32);
        store<u8>(p + 2812, 0);
        call(ctx, kMemset, {p + 2636, 0, 64});
        call(ctx, kMemset, {p + 2700, 0, 64});
        r = p;
    }
    call(ctx, 0x0033D4E4u, {r, b});
    ctx.gpr[3] = r;
}

// u32 Obj_Construct4335C0(Obj* o, const Vec* src, const char* s1, const
// char* s2, a, b, c, u8 e, f [stack]): Ctor_Base42C9D0(o, e), vtable
// 0x00B210A0, a 12-byte-element PtrVector at +140 (PtrVector_Init(v,
// src->count, 0, 1)), +160 = a, +164 = b, +168 bit 31 = f bit 0, bit 30 = c
// bit 0 (the rest kept); the names s1 / s2 String_CopyN'd (64) to +9 / +74
// (u8 0 when null or empty); then the u16 src->8 12-byte records of src->0
// appended (9 bytes each, memmove), growing with PtrVector_Reserve(v, 2 *
// capacity) when full (high-water +150). r3 = src->8 (the last loop test).
DECOMP(004335C0, Obj_Construct4335C0) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), s1 = arg<u32>(ctx, 2), s2 = arg<u32>(ctx, 3);
    const u32 a = arg<u32>(ctx, 4), b = arg<u32>(ctx, 5), c = arg<u32>(ctx, 6), e = arg<u32>(ctx, 7);
    const u32 f = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116);  // 9th argument (low word of 112(r1))
    call(ctx, 0x0042C9D0u, {o, e & 0xFFu});
    const u32 vec = o + 140;
    store<u32>(o, 0x00B210A0u);
    const u32 n = load<u16>(src + 8);
    InitVec(ctx, vec, 12, n);
    const u32 w = load<u32>(o + 168);
    store<u32>(o + 160, a);
    store<u32>(o + 164, b);
    store<u32>(o + 168, (w & 0x3FFFFFFFu) | ((f & 1u) << 31) | ((c & 1u) << 30));
    if (s1 != 0 && load<u8>(s1) != 0) call(ctx, kStringCopyN, {o + 9, s1, 64});
    else store<u8>(o + 9, 0);
    if (s2 != 0 && load<u8>(s2) != 0) call(ctx, kStringCopyN, {o + 74, s2, 64});
    else store<u8>(o + 74, 0);
    u32 i = 0;
    u32 lim;
    while (i < (lim = load<u16>(src + 8))) {
        const u32 el = load<u32>(src) + 12 * i;
        u32 cnt = load<u16>(o + 148);
        const u32 cap = load<u16>(o + 144);
        if (cnt == cap) {
            call(ctx, kPtrVectorReserve, {vec, (cap << 1) & 0x1FFFEu});
            cnt = load<u16>(o + 148);
        }
        call(ctx, kMemmove, {load<u32>(vec) + 12 * cnt, el, 9});
        const u32 c1 = static_cast<u32>(load<u16>(o + 148)) + 1;
        const u32 hi = load<u16>(o + 150);
        store<u16>(o + 148, static_cast<u16>(c1));
        store<u16>(o + 150, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFFu) ? hi : c1));
        ++i;
    }
    ctx.gpr[3] = lim;
}

// u32 SubMachine_Ctor40F244(T* o, a, u32* b, c, d): SubMachine_Construct(o,
// a, c, *b, d), then the fields: u8 +53 / 54 / 68 / 80..83 / 87 / 97 / 104 =
// 0, u8 +84..86 = 1, +72 = 1.0f, vtable 0x00B20A48, +88 = b, +96 bit 31
// cleared, the default vector at +128 and +144, +160 = -1, a 4-byte-element
// PtrVector at +164 (constructed only), +220 = 2.0f, +224 = 70.0f, +264 =
// -1.0f, +268 = 1.0f, +320 = 3, the rest of +60..488 listed below zeroed,
// and +28 |= 0x2000. r3 = o + 28; f1 the vector's z.
DECOMP(0040F244, SubMachine_Ctor40F244) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    call(ctx, 0x0047C5D8u, {o, a, c, load<u32>(b), d});
    store<u8>(o + 53, 0);
    store<u8>(o + 54, 0);
    store<u32>(o + 60, 0);
    store<u8>(o + 68, 0);
    store<u32>(o + 72, 0x3F800000u);
    store<u32>(o + 76, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 80 + k, 0);
    const u32 w96 = load<u32>(o + 96);
    store<u8>(o + 84, 1);
    store<u8>(o + 85, 1);
    store<u8>(o + 86, 1);
    store<u8>(o + 87, 0);
    store<u32>(o, 0x00B20A48u);
    store<u32>(o + 88, b);
    store<u32>(o + 92, 0);
    store<u32>(o + 96, w96 & 0x7FFFFFFFu);
    store<u8>(o + 97, 0);
    store<u32>(o + 100, 0);
    store<u8>(o + 104, 0);
    store<u32>(o + 108, 0);
    store<u32>(o + 112, 0);
    DefVec(o + 128);
    Cw(o + 144, kVec4B40);
    store<u32>(o + 160, 0xFFFFFFFFu);
    Cw(o + 148, kVec4B40 + 4);
    Cw(o + 152, kVec4B40 + 8);
    ctx.fpr[1] = Lf(kVec4B40 + 8);
    call(ctx, kPtrVectorConstruct, {o + 164});
    store<u32>(o + 192, 0);
    store<u32>(o + 196, 0);
    store<u32>(o + 200, 0);
    store<u16>(o + 170, 4);
    store<u8>(o + 204, 0);
    store<u32>(o + 208, 0);
    store<u32>(o + 216, 0);
    store<u32>(o + 220, 0x40000000u);
    store<u32>(o + 224, 0x428C0000u);
    store<u32>(o + 256, 0);
    store<u32>(o + 260, 0);
    store<u32>(o + 264, 0xBF800000u);
    store<u32>(o + 268, 0x3F800000u);
    for (u32 k = 0; k < 12; ++k) store<u32>(o + 272 + 4 * k, 0);
    store<u32>(o + 320, 3);
    for (const u32 off : {324u, 328u, 332u, 340u, 344u, 348u, 352u, 368u, 372u, 376u, 416u, 420u, 424u}) store<u32>(o + off, 0);
    store<u8>(o + 453, 0);
    store<u8>(o + 464, 0);
    store<u8>(o + 465, 0);
    store<u8>(o + 466, 0);
    store<u32>(o + 468, 0);
    store<u32>(o + 472, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 476 + k, 0);
    store<u32>(o + 480, 0);
    store<u32>(o + 484, 0);
    store<u8>(o + 488, 0);
    store<u32>(o + 28, load<u32>(o + 28) | 0x2000u);
    ctx.gpr[3] = o + 28;
}

// u32 ScaleSM_Construct304834(SM* o, a): StateMachine_ConstructThunk(o, 1,
// 0, 0), +28 = 0, +32 = a, +36 / 40 = 0 (vtable 0x00B1DCA8, then
// 0x00B1DF88), a 4-byte-element PtrVector at +44 (PtrVector_Init(v, 16, 0,
// 1)), +64 / +116 = -1, the default vector at +80, the listed fields up to
// +180 zeroed (+120..151 as four doublewords), +68 = the SplineMap_Find(
// (*0x00F8F158)->16, 0xE9F0AB3F) entry, SM_SetState_003044B0(o,
// 0xC0941292); then String_HashNoCase of each non-null name of the six at
// 0x00AD9ACC appended to the vector (PtrVector_Reserve(v, 2 * capacity)
// when full, high-water +54). r3 as the last call / high-water load leaves
// it; f1 the vector's z.
DECOMP(00304834, ScaleSM_Construct304834) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    call(ctx, 0x0045CBE8u, {o, 1, 0, 0});
    store<u32>(o + 28, 0);
    store<u32>(o + 32, a);
    store<u32>(o, 0x00B1DCA8u);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    const u32 vec = o + 44;
    InitVec(ctx, vec, 4, 16);
    store<u32>(o + 68, 0);
    Cw(o + 80, kVec4B40);
    store<u32>(o, 0x00B1DF88u);
    store<u32>(o + 64, 0xFFFFFFFFu);
    Cw(o + 84, kVec4B40 + 4);
    store<u8>(o + 96, 0);
    Cw(o + 88, kVec4B40 + 8);
    ctx.fpr[1] = Lf(kVec4B40 + 8);
    store<u8>(o + 97, 0);
    store<u8>(o + 98, 0);
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 100 + 4 * k, 0);
    store<u32>(o + 116, 0xFFFFFFFFu);
    for (u32 k = 0; k < 6; ++k) store<u32>(o + 152 + 4 * k, 0);
    for (u32 k = 0; k < 4; ++k) store<u64>(o + 120 + 8 * k, 0);
    store<u8>(o + 176, 0);
    Stack frame(ctx, 16);
    const u32 key = frame.addr();
    store<u32>(o + 180, 0);
    store<u32>(key, 0xE9F0AB3Fu);
    const u32 e = static_cast<u32>(call(ctx, 0x007335C4u, {load<u32>(load<u32>(0x00F8F158u) + 16), key}));
    store<u32>(o + 68, load<u32>(e));
    u32 r3 = static_cast<u32>(call(ctx, 0x003044B0u, {o, 0xC0941292u}));
    for (u32 k = 0; k < 6; ++k) {
        const u32 s = load<u32>(0x00AD9ACCu + 4 * k);
        if (s == 0) continue;
        const u32 h = static_cast<u32>(call(ctx, kHashNoCase, {s}));
        u32 cnt = load<u16>(o + 52);
        const u32 cap = load<u16>(o + 48);
        if (cnt == cap) {
            call(ctx, kPtrVectorReserve, {vec, (cap << 1) & 0x1FFFEu});
            cnt = load<u16>(o + 52);
        }
        store<u32>(load<u32>(vec) + ((cnt << 2) & 0x3FFFCu), h);
        const u32 c1 = static_cast<u32>(load<u16>(o + 52)) + 1;
        const u32 hi = load<u16>(o + 54);
        store<u16>(o + 52, static_cast<u16>(c1));
        store<u16>(o + 54, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFFu) ? hi : c1));
        r3 = hi;
    }
    ctx.gpr[3] = r3;
}
