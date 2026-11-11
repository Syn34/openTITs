// Game code, medium sweep part 12: def factories called by 0x000CA038
// (a defaults record on the stack, the reader, then the def built from it).
#include "mid12.h"

using namespace sly4;
using namespace sly4::mid12;

// Def* TriggerDef_New21A310(a0, XmlCursor* c, x, const Def* tmpl)
// A trigger defaults record (TrigRecInit: flags 0x08400000, +32 = 1,
// objPrefix 0x00AD3780; +96..+104 the zero vec3, +112..+124 0, bytes
// +128..+131 0) read by XmlDef_Read_00219FF8(a0, c, x, tmpl, rec), then a
// 224-byte def (SmallAlloc): TrigFromRec (the vtable 0x00B19510 after two
// bools), f32 +176..+184 = rec +96..+104, vtable 0x00B1A290, +192..+204 /
// bytes +208..+211 from rec +112..+131. Returns the def (0 when the
// allocation fails).
DECOMP(0021A310, TriggerDef_New21A310) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 144);
    const u32 rec = frame.addr();  // the original's sp + 112
    const u32 vz = mid11::kVecZero;
    mid9::TrigRecInit(rec, x, 0x08400000u, 0, 1, 0x00AD3780u);
    CopyW(rec + 96, vz);
    CopyW(rec + 100, vz + 4);
    CopyW(rec + 104, vz + 8);
    for (u32 k = 112; k <= 124; k += 4) store<u32>(rec + k, 0);
    for (u32 k = 128; k < 132; ++k) store<u8>(rec + k, 0);
    SetF1(ctx, vz + 8);
    call(ctx, 0x00219FF8, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {224}));
    if (p != 0) {
        mid9::TrigFromRec(ctx, p, rec, 2);
        CopySeq(p, rec, {{4, 176, 96}, {4, 180, 100}, {4, 184, 104}});
        store<u32>(p, 0x00B1A290u);
        CopySeq(p, rec, {{4, 196, 116}, {1, 208, 128}, {4, 200, 120}, {4, 192, 112}, {1, 209, 129}, {4, 204, 124}, {1, 210, 130}, {1, 211, 131}});
        SetF1(ctx, rec + 124);
    }
    ret(ctx, p);
}

// Def* XmlDef_NewC1780(a0, XmlCursor* c, x, const Def* tmpl)
// A defaults record of vtable 0x00B12034 (DefHeadInit at +4 with x->12..20
// and f 100.0; +4 |= 0x00200000; three zero vec3s +128 / +144 / +160,
// +176..+184 0, f32 +188 -1, bytes +192..+194 0) read by 0x000C12A8(a0, c,
// x, tmpl, rec), then a 272-byte def (SmallAlloc) built by 0x000C0B80(p,
// rec). Returns the def (0 when the allocation fails).
DECOMP(000C1780, XmlDef_NewC1780) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 208);
    const u32 rec = frame.addr();  // the original's sp + 112
    const u32 vz = mid11::kVecZero;
    mid10::DefHeadInit(rec + 4, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 100.0);
    store<u32>(rec, 0x00B12034u);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x00200000u);
    for (u32 k = 128; k <= 160; k += 16) {
        CopyW(rec + k, vz);
        CopyW(rec + k + 4, vz + 4);
        CopyW(rec + k + 8, vz + 8);
    }
    for (u32 k = 176; k <= 184; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 188, 0xBF800000u);
    for (u32 k = 192; k < 195; ++k) store<u8>(rec + k, 0);
    SetF1(ctx, vz);
    call(ctx, 0x000C12A8, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {272}));
    if (p != 0) call(ctx, 0x000C0B80, {p, rec});
    ret(ctx, p);
}

namespace {
// f1 as DefHeadInit's float loop leaves it: (f32)(((u16 +120 >> 4) & 0xF) - 4).
void HeadF1(PPUContext& ctx, u32 o) {
    ctx.fpr[1] = static_cast<f64>(static_cast<f32>(static_cast<s32>((load<u16>(o + 120) >> 4) & 0xFu) - 4));
}

// The DefHeadInit factories: a stack defaults record (DefHeadInit with
// x->12..20 and f 100.0, then the words w from +124 on, the bytes b at
// their offsets, +0 |= flag) read by reader(a0, c, x, tmpl, rec), then a
// def of `size` bytes (SmallAlloc, or Mem_Alloc(size, 0, 16) when big)
// built by ctor(p, rec). Returns the def (0 when the allocation fails).
struct ByteInit {
    u16 off;
    u8 n;  // 1 or 4
    u32 v;
};
void HeadFactory(PPUContext& ctx, u32 frameBytes, u32 flag, std::initializer_list<u32> w, std::initializer_list<ByteInit> b, u32 reader, u32 size, bool memAlloc, u32 ctor) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, frameBytes);
    const u32 rec = frame.addr();  // the original's sp + 112
    mid10::DefHeadInit(rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 100.0);
    HeadF1(ctx, rec);
    u32 k = 124;
    for (u32 v : w) {
        store<u32>(rec + k, v);
        k += 4;
    }
    for (const ByteInit& e : b) {
        if (e.n == 1) store<u8>(rec + e.off, static_cast<u8>(e.v));
        else store<u32>(rec + e.off, e.v);
    }
    store<u32>(rec, load<u32>(rec) | flag);
    call(ctx, reader, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(memAlloc ? call(ctx, m1fn::Mem_Alloc, {size, 0, 16}) : call(ctx, m1fn::SmallAlloc, {size}));
    if (p != 0) call(ctx, ctor, {p, rec});
    ret(ctx, p);
}
constexpr u32 kM1 = 0xBF800000u, kP1 = 0x3F800000u, kHalf = 0x3F000000u, k03 = 0x3E99999Au;
}  // namespace

// Def* XmlDef_New16ADC4(a0, XmlCursor* c, x, const Def* tmpl)
// HeadFactory: +0 |= 0x400; +124 = 0x00AC8C70, f32s +128..+188 = -1, -1,
// -1, -1, 1, -1, 1, -1, 0, 0.3, -1, 1, -1, 1, 100, 0; reader 0x0016A828,
// 272 bytes (SmallAlloc), ctor 0x0016A448.
DECOMP(0016ADC4, XmlDef_New16ADC4) {
    HeadFactory(ctx, 192, 0x400u, {0x00AC8C70u, kM1, kM1, kM1, kM1, kP1, kM1, kP1, kM1, 0, k03, kM1, kP1, kM1, kP1, 0x42C80000u, 0}, {},
                0x0016A828, 272, false, 0x0016A448);
}

// Def* XmlDef_New203BE4(a0, XmlCursor* c, x, const Def* tmpl)
// HeadFactory: +0 |= 0x1000; +124 = 6, +128 = 1, f32s +132..+152 = -1,
// 0.15, 5, 0.9, 0.5, 1, +156..+168 0; reader 0x00203718, 224 bytes
// (Mem_Alloc(224, 0, 16)), ctor 0x00203380.
DECOMP(00203BE4, XmlDef_New203BE4) {
    HeadFactory(ctx, 176, 0x1000u, {6, 1, kM1, 0x3E19999Au, 0x40A00000u, 0x3F666666u, kHalf, kP1, 0, 0, 0, 0}, {}, 0x00203718, 224, true,
                0x00203380);
}

// Def* XmlDef_New167080(a0, XmlCursor* c, x, const Def* tmpl)
// HeadFactory: +0 |= 0x80; +124 = 0x00AC8C70, f32s +128..+180 = -1, -1,
// -1, -1, 1, -1, 1, -1, 0, 0.3, -1, 1, -1, 1, +184..+204 0, +216 0, bytes
// +208..+213 1; reader 0x001668B4, 288 bytes (SmallAlloc), ctor
// 0x001664C8.
DECOMP(00167080, XmlDef_New167080) {
    HeadFactory(ctx, 224, 0x80u, {0x00AC8C70u, kM1, kM1, kM1, kM1, kP1, kM1, kP1, kM1, 0, k03, kM1, kP1, kM1, kP1, 0, 0, 0, 0, 0, 0},
                {{216, 4, 0}, {208, 1, 1}, {209, 1, 1}, {210, 1, 1}, {211, 1, 1}, {212, 1, 1}, {213, 1, 1}}, 0x001668B4, 288, false, 0x001664C8);
}

// Def* XmlDef_New17D640(a0, XmlCursor* c, x, const Def* tmpl)
// HeadFactory: +0 |= 0x100; +124 = 0x00AC8C70, f32s +128..+196 = -1, -1,
// -1, -1, 1, -1, 1, -1, 0, 0.3, -1, 1, -1, 1, 1, 0, 0.5, 0.5, +200..+212 0,
// +220 / +224 0, bytes +216 / +228 0 and +229..+234 1; reader 0x0017CD54,
// 304 bytes (SmallAlloc), ctor 0x0017C918.
DECOMP(0017D640, XmlDef_New17D640) {
    HeadFactory(ctx, 240, 0x100u,
                {0x00AC8C70u, kM1, kM1, kM1, kM1, kP1, kM1, kP1, kM1, 0, k03, kM1, kP1, kM1, kP1, kP1, 0, kHalf, kHalf, 0, 0, 0, 0},
                {{216, 1, 0}, {220, 4, 0}, {224, 4, 0}, {228, 1, 0}, {229, 1, 1}, {230, 1, 1}, {231, 1, 1}, {232, 1, 1}, {233, 1, 1}, {234, 1, 1}}, 0x0017CD54, 304, false, 0x0017C918);
}

// Def* XmlDef_New1B4394(a0, XmlCursor* c, x, const Def* tmpl)
// A defaults record (DefHeadInit with x->12..20 and f 100.0; +4 |= 0x20000;
// an inline PtrVector at +124 (element size 8, constructed only), +144..+156
// 0, +160 the name 0x00AD13B8, +164 0, f32 +168 1, +172 hash 0xA970EC1F,
// bits 31..29 of +176 cleared) read by 0x001B3BDC(a0, c, x, tmpl, rec),
// then a 256-byte def (SmallAlloc) built by 0x001B37E8(p, rec); the vector
// cleared. Returns the def (0 when the allocation fails).
DECOMP(001B4394, XmlDef_New1B4394) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 192);
    const u32 rec = frame.addr(), v = rec + 124;  // the original's sp + 112 / + 236
    mid10::DefHeadInit(rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 100.0);
    HeadF1(ctx, rec);
    call(ctx, m1fn::PtrVector_Construct, {v});
    store<u16>(v + 6, 8);
    for (u32 k = 144; k <= 156; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 160, mid11::kStrAD13B8);
    store<u32>(rec + 164, 0);
    store<u32>(rec + 168, 0x3F800000u);
    store<u32>(rec + 172, 0xA970EC1Fu);
    store<u32>(rec + 176, load<u32>(rec + 176) & 0x1FFFFFFFu);
    store<u32>(rec + 4, load<u32>(rec + 4) | 0x20000u);
    call(ctx, 0x001B3BDC, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {256}));
    if (p != 0) call(ctx, 0x001B37E8, {p, rec});
    call(ctx, m1fn::PtrVector_Clear, {v});
    call(ctx, m1fn::Nop, {v});
    ret(ctx, p);
}

namespace {
// The eight-word parameter block both 0x0075CBEC / 0x0075C364 factories
// pass: {0, 100, 3, 0, 1, 0, 1, 1}.
void FacParams(u32 p) {
    const u32 w[8] = {0, 0x42C80000u, 0x40400000u, 0, 0x3F800000u, 0, 0x3F800000u, 0x3F800000u};
    for (u32 k = 0; k < 8; ++k) store<u32>(p + 4 * k, w[k]);
}
}  // namespace

// Def* XmlDef_New22F630(a0, XmlCursor* c, x, const Def* tmpl)
// Stack objects: A (0x002266C0(A)) and the defaults record B built by
// 0x0075CBEC(B, x->12, x->16, x->20, params, A, r9 as left, 0, stack
// arguments 0; f1 = pool +4, f2 = pool +0, f3 = f4 = pool +8, pool
// 0x0022F620); B read by 0x0022DFD8(a0, c, x, tmpl, B); a 1616-byte def
// (SmallAlloc) built by 0x0022D44C(p, B); then B's vectors +1328 / +1140 /
// +760 cleared and Mem_FreeIfSet(B->672) (B->668 in r4) when set. Returns
// the def (0 when the allocation fails).
DECOMP(0022F630, XmlDef_New22F630) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 2144);
    const u32 a = frame.addr() + 80, b = frame.addr() + 560, prm = frame.addr() + 2112;  // the original's sp + 192 / + 672 / + 2224
    constexpr u32 kPool = 0x0022F620;
    const u32 x12 = load<u32>(x + 12), x16 = load<u32>(x + 16), x20 = load<u32>(x + 20);
    FacParams(prm);
    call(ctx, 0x002266C0, {a});
    const f64 k0 = LoadF(kPool), k4 = LoadF(kPool + 4), k8 = LoadF(kPool + 8);
    // r9 is not read by 0x0075CBEC; it reads stack arguments 10..15 and 18
    // (all 0); arguments 9, 16 and 17 are neither written nor read.
    callf(ctx, 0x0075CBEC, {b, x12, x16, x20, prm, a, ctx.gpr[9], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {k4, k0, k8, k8});
    call(ctx, 0x0022DFD8, {a0, c, x, tmpl, b});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {1616}));
    if (p != 0) call(ctx, 0x0022D44C, {p, b});
    for (const u32 off : {1328u, 1140u, 760u}) {
        call(ctx, m1fn::PtrVector_Clear, {b + off});
        call(ctx, m1fn::Nop, {b + off});
    }
    const u32 m = load<u32>(b + 672);
    if (m != 0) call(ctx, m12fn::Mem_FreeIfSet, {m, load<u32>(b + 668)});
    ret(ctx, p);
}

// Def* XmlDef_New245A30(a0, XmlCursor* c, x, const Def* tmpl)
// Stack objects: A (0x002266C0(A)) and the defaults record B built by
// 0x0075C364(B, x->12, x->16, x->20, params, A, 2, 0, stack arguments 0
// except 20 the name 0x00AD3C14 and 21 = 2; f1 = pool +4, f2 = pool
// +0, f3 = f4 = pool +8, pool 0x00245A20), then B's defaults (+716..+972
// below, +8 |= 0x2000); B read by 0x00244CF8(a0, c, x, tmpl, B); a
// 1040-byte def (SmallAlloc) built by 0x00244980(p, B); then
// Mem_FreeIfSet(B->672) (B->668 in r4) when set. Returns the def (0 when
// the allocation fails).
DECOMP(00245A30, XmlDef_New245A30) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 1616);
    const u32 a = frame.addr() + 112, b = frame.addr() + 592, prm = frame.addr() + 1568;  // the original's sp + 224 / + 704 / + 1680
    constexpr u32 kPool = 0x00245A20;
    const u32 vz = mid11::kVecZero;
    const u32 x12 = load<u32>(x + 12), x16 = load<u32>(x + 16), x20 = load<u32>(x + 20);
    FacParams(prm);
    call(ctx, 0x002266C0, {a});
    const f64 k0 = LoadF(kPool), k4 = LoadF(kPool + 4), k8 = LoadF(kPool + 8);
    // Stack arguments 10..15 / 18 / 19 / 22 are 0, 20 the name, 21 = 2; 9, 16
    // and 17 are not written (passed as 0).
    callf(ctx, 0x0075C364, {b, x12, x16, x20, prm, a, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00AD3C14u, 2, 0}, {k4, k0, k8, k8});
    static constexpr u32 kW[][2] = {
        {720, 0x41200000u}, {752, 0x3E4CCCCDu}, {756, 0x3DCCCCCDu}, {716, 0},           {820, 0x3F800000u}, {824, 0x40D66666u},
        {828, 0x3E99999Au}, {832, 0x41200000u}, {836, 0x41800000u}, {840, 0x3F000000u}, {844, 0x41A00000u}, {808, 0},
        {812, 0},           {816, 0},           {852, 0x40A00000u}, {856, 0x3E99999Au}, {860, 0x41700000u}, {864, 0x41C80000u},
        {868, 0x41400000u}, {872, 0x41100000u}, {876, 0x40000000u}, {880, 0x42700000u}, {884, 0x42200000u}, {888, 0x41200000u},
        {892, 0},           {896, 0x41700000u}, {900, 0x42480000u}, {904, 0x42340000u}, {908, 0x43700000u}, {912, 0x44340000u},
        {944, 0xBF800000u}, {948, 0xBF800000u}, {956, 0},           {960, 0},           {964, 0},           {968, 0},
        {760, 0x3DCCCCCDu}, {764, 0x3DCCCCCDu}, {768, 0x3DCCCCCDu}, {772, 0x3DCCCCCDu}, {776, 0xCA2D9292u}, {780, 0xCA2D9298u},
        {784, 0x5410B5F5u}, {788, 0x5410B5FBu}, {792, 0x5E850934u}, {796, 0x5E85093Au}, {800, 0xED7EE313u}, {804, 0xED7EE319u}};
    for (const auto& e : kW) store<u32>(b + e[0], e[1]);
    for (const u32 off : {736u, 928u}) {
        CopyW(b + off, vz);
        CopyW(b + off + 4, vz + 4);
        CopyW(b + off + 8, vz + 8);
    }
    store<u8>(b + 848, 0);
    store<u8>(b + 952, 1);
    store<u8>(b + 972, 1);
    store<u32>(b + 8, load<u32>(b + 8) | 0x2000u);
    SetF1(ctx, vz + 8);
    call(ctx, 0x00244CF8, {a0, c, x, tmpl, b});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {1040}));
    if (p != 0) call(ctx, 0x00244980, {p, b});
    const u32 m = load<u32>(b + 672);
    if (m != 0) call(ctx, m12fn::Mem_FreeIfSet, {m, load<u32>(b + 668)});
    ret(ctx, p);
}

// Def* XmlDef_New20A298(a0, XmlCursor* c, x, const Def* tmpl)
// A defaults record (+0 / +4 0, +8 128, +12..+20 x->12..20, bytes +24 1 /
// +25..+31 0, +32 0, f32 +36 / +40 -1, byte +44 0, +48..+68 0, f32 +72 -1,
// +76..+88 0, f32 +92..+108 = 15, 300, 1, 25, 10, bytes +112..+114 0)
// passed to Rec_CopyFields_000C387C(a0, c, x, tmpl, rec) (byte +24 set to 1
// again after it), then read from c with the defaults taken from tmpl
// when it has 0x80 in +28 (hash tmpl->140->204 (0 when tmpl->140 is 0),
// tmpl->144 / ->148, String_HashNoCase("") when tmpl->152 is set, f32s
// tmpl->156..168, bools tmpl->176..178; a template without the flag notes
// c's document path), else 0 / the pool words 0x0020A280..: hashes
// 0x3D6A1C6C +76 / 0xBD10D20C +80 / 0xC9796122 +84 / 0xF89FDD94 +88, f32s
// 0x0F8DA662 +92 / 0x0F8DA574 +96 / 0xF97D90C8 +100 / 0xF97D8FDA +104,
// bools 0xA9A5F809 +112 / 0xDF472EFC +113, f32 0x291411AE +108 (default
// pool +16), bool 0xD41AD6F9 +114. Then a 192-byte def (SmallAlloc) built
// by 0x0020A0F0(p, rec). Returns the def (0 when the allocation fails).
DECOMP(0020A298, XmlDef_New20A298) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 128);
    const u32 rec = frame.addr();  // the original's sp + 112
    constexpr u32 kPool = 0x0020A280;
    store<u32>(rec, 0);
    store<u32>(rec + 4, 0);
    store<u32>(rec + 8, 128);
    store<u32>(rec + 12, load<u32>(x + 12));
    store<u32>(rec + 16, load<u32>(x + 16));
    store<u32>(rec + 20, load<u32>(x + 20));
    store<u8>(rec + 24, 1);
    for (u32 k = 25; k < 32; ++k) store<u8>(rec + k, 0);
    store<u32>(rec + 32, 0);
    store<u32>(rec + 36, 0xBF800000u);
    store<u32>(rec + 40, 0xBF800000u);
    store<u8>(rec + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 72, 0xBF800000u);
    for (u32 k = 76; k <= 88; k += 4) store<u32>(rec + k, 0);
    const u32 w[5] = {0x41700000u, 0x43960000u, 0x3F800000u, 0x41C80000u, 0x41200000u};
    for (u32 k = 0; k < 5; ++k) store<u32>(rec + 92 + 4 * k, w[k]);
    for (u32 k = 112; k < 115; ++k) store<u8>(rec + k, 0);
    call(ctx, mid9::m9fn::Rec_CopyFields_000C387C, {a0, c, x, tmpl, rec});
    store<u8>(rec + 24, 1);
    u32 h0 = 0, h1 = 0, h2 = 0, h3 = 0, b0 = 0, b1 = 0, b2 = 0;
    u32 f[4] = {load<u32>(kPool), load<u32>(kPool + 4), load<u32>(kPool + 8), load<u32>(kPool + 12)};
    if (tmpl != 0) {
        if (load<u32>(tmpl + 28) & 0x80u) {
            const u32 t = load<u32>(tmpl + 140);
            if (t != 0) h0 = load<u32>(t + 204);
            h1 = load<u32>(tmpl + 144);
            h2 = load<u32>(tmpl + 148);
            if (load<u32>(tmpl + 152) != 0) h3 = static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {0x00B0F050u}));  // ""
            for (u32 k = 0; k < 4; ++k) f[k] = load<u32>(tmpl + 156 + 4 * k);
            b0 = load<u8>(tmpl + 176);
            b1 = load<u8>(tmpl + 177);
            b2 = load<u8>(tmpl + 178);
        } else {
            call(ctx, m11fn::XmlCursor_DocPath, {a0});
        }
    }
    auto rf = [&](u32 hash, u32 off, u32 dw) { callf(ctx, xfn::XmlCursor_ReadF32, {c, hash, rec + off}, {std::bit_cast<f64>(mid1::LfsBits(dw))}); };
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x3D6A1C6Cu, rec + 76, h0});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xBD10D20Cu, rec + 80, h1});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xC9796122u, rec + 84, h2});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xF89FDD94u, rec + 88, h3});
    rf(0x0F8DA662u, 92, f[0]);
    rf(0x0F8DA574u, 96, f[1]);
    rf(0xF97D90C8u, 100, f[2]);
    rf(0xF97D8FDAu, 104, f[3]);
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xA9A5F809u, rec + 112, b0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xDF472EFCu, rec + 113, b1});
    rf(0x291411AEu, 108, load<u32>(kPool + 16));
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xD41AD6F9u, rec + 114, b2});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {192}));
    if (p != 0) call(ctx, 0x0020A0F0, {p, rec});
    ret(ctx, p);
}

// Def* XmlDef_New23FA28(a0, XmlCursor* c, x, const Def* tmpl)
// As XmlDef_New245A30 (pool 0x0023FA1C; r9 = the name 0x00AD3C14 this
// time) with other defaults in B (+716..+961 below, +8 |= 0x1000, byte
// +681 0), read by 0x0023E590(a0, c, x, tmpl, B); the 1040-byte def
// (SmallAlloc) is built by 0x00226A28(p, B), then +780 0, vtable
// 0x00B1B9B8, +784..+1000 / +1008..+1016 / bytes +1024 / +1025 copied from
// B +720.. (p + 64 + k <- B + k) and String_CopyN(p + 745, B + 681, 32).
// Then Mem_FreeIfSet(B->672) (B->668 in r4) when set. Returns the def (0
// when the allocation fails).
DECOMP(0023FA28, XmlDef_New23FA28) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 1616);
    const u32 a = frame.addr() + 112, b = frame.addr() + 592, prm = frame.addr() + 1568;  // the original's sp + 224 / + 704 / + 1680
    constexpr u32 kPool = 0x0023FA1C;
    const u32 vz = mid11::kVecZero;
    const u32 x12 = load<u32>(x + 12), x16 = load<u32>(x + 16), x20 = load<u32>(x + 20);
    FacParams(prm);
    call(ctx, 0x002266C0, {a});
    const f64 k0 = LoadF(kPool), k4 = LoadF(kPool + 4), k8 = LoadF(kPool + 8);
    callf(ctx, 0x0075C364, {b, x12, x16, x20, prm, a, 0x00AD3C14u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00AD3C14u, 2, 0}, {k4, k0, k8, k8});
    constexpr u32 k1 = 0x3F800000u, k5 = 0x40A00000u, k180 = 0x43340000u, kN180 = 0xC3340000u, kM1 = 0xBF800000u;
    static constexpr u32 kW[] = {
        // B +716..+940, one word each (+944.. below)
        0, 0x41200000u, 0x3D4CCCCDu, 0, k1, k1, k1, k1, k5, 2, k1, 0x40000000u, k5, k5, 0x44870000u, 0x44870000u, k180, k1, 0x43B40000u,
        0x43B40000u, k180, k1, kN180, kN180, kN180, k180, k1, kN180, kN180, kN180, k1, 0x40000000u, kM1, kM1, kM1, 0, 0, 0x41700000u, 0, kM1,
        0, 0, 0, k1, 0, k1, 0x42820000u, 0x42820000u, 0x41700000u, 0x41200000u, 0x41C80000u, 0x42340000u, 0x42480000u, 0x42480000u,
        0x43480000u, 0x41000000u};
    for (u32 k = 0; k < sizeof(kW) / sizeof(kW[0]); ++k) store<u32>(b + 716 + 4 * k, kW[k]);
    CopyW(b + 944, vz);
    CopyW(b + 948, vz + 4);
    CopyW(b + 952, vz + 8);
    store<u8>(b + 960, 0);
    store<u8>(b + 961, 1);
    store<u32>(b + 8, load<u32>(b + 8) | 0x1000u);
    store<u8>(b + 681, 0);
    SetF1(ctx, vz + 8);
    call(ctx, 0x0023E590, {a0, c, x, tmpl, b});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {1040}));
    if (p != 0) {
        call(ctx, 0x00226A28, {p, b});
        store<u32>(p + 780, 0);
        store<u32>(p, 0x00B1B9B8u);
        for (u32 k = 720; k <= 936; k += 4) CopyW(p + 64 + k, b + k);
        for (u32 k = 944; k <= 952; k += 4) CopyW(p + 64 + k, b + k);
        store<u8>(p + 1024, load<u8>(b + 960));
        store<u8>(p + 1025, load<u8>(b + 961));
        SetF1(ctx, b + 952);
        call(ctx, m1fn::String_CopyN, {p + 745, b + 681, 32});
    }
    const u32 m = load<u32>(b + 672);
    if (m != 0) call(ctx, m12fn::Mem_FreeIfSet, {m, load<u32>(b + 668)});
    ret(ctx, p);
}
