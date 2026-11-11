// Game code, medium sweep part 12: damage / collision def constructors
// (Def* p, const Record* rec) used by the part 11 factories.
#include "mid12.h"

using namespace sly4;
using namespace sly4::mid12;

namespace {
constexpr u32 kVecZero = mid11::kVecZero;  // f32[3] 0
constexpr u32 kQuat4C90 = mid11::kQuat4C90;
}  // namespace

// DamageColDef::DamageColDef(Def* p, const Record* rec) for 001C8780 (992
// bytes): Thunk_000E27FC(p, rec), DmgAdCopy (vtable after the second copy),
// +400 = 22, vtable 0x00B16FD0, six 96-byte slots at +416 preset (+0 0, two
// zero vec3s at +16 / +32, the quaternion 0x01164C90 at +48, bytes +64 1 /
// +65 0, +68..+84 0) and then overwritten by memmove(p + 416, rec + 320,
// 576). r3 = memcpy's, f1 = the quaternion's last word.
DECOMP(001C7F28, DamageColDef_FromRecC7F28) {
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    call(ctx, m11fn::Thunk_000E27FC, {p, rec});
    DmgAdCopy(p, rec, 2);
    store<u32>(p + 400, 22);
    store<u32>(p, 0x00B16FD0u);
    for (u32 k = 0; k < 6; ++k) {
        const u32 s = p + 416 + 96 * k;
        CopyW(s + 16, kVecZero);
        CopyW(s + 20, kVecZero + 4);
        CopyW(s + 24, kVecZero + 8);
        CopyW(s + 32, kVecZero);
        CopyW(s + 36, kVecZero + 4);
        store<u32>(s, 0);
        CopyW(s + 40, kVecZero + 8);
        CopyW(s + 48, kQuat4C90);
        CopyW(s + 52, kQuat4C90 + 4);
        store<u8>(s + 64, 1);
        CopyW(s + 56, kQuat4C90 + 8);
        store<u8>(s + 65, 0);
        CopyW(s + 60, kQuat4C90 + 12);
        for (u32 j = 68; j <= 84; j += 4) store<u32>(s + j, 0);
    }
    SetF1(ctx, kQuat4C90 + 12);
    call(ctx, m12fn::Memmove, {p + 416, rec + 320, 576});
}

// DamageColDef::DamageColDef(Def* p, const Record* rec) for 001CA7C0 (496
// bytes): Thunk_000E27FC(p, rec), DmgAdCopy (vtable first), vtable
// 0x00B17228, then rec +320..+407 copied to +400..+487 (without the pads
// +412 / +428). r3 = rec's byte +407, f1 = rec +392.
DECOMP(001C9E34, DamageColDef_FromRecC9E34) {
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    call(ctx, m11fn::Thunk_000E27FC, {p, rec});
    DmgAdCopy(p, rec, 0);
    CopyW(p + 400, rec + 320);
    store<u32>(p, 0x00B17228u);
    CopySeq(p, rec, {{4, 404, 324}, {4, 408, 328}, {4, 416, 336}, {4, 420, 340}, {4, 424, 344}, {4, 432, 352}, {4, 436, 356},
                     {4, 440, 360}, {4, 444, 364}, {4, 448, 368}, {4, 452, 372}, {4, 456, 376}, {4, 460, 380}, {4, 464, 384},
                     {4, 468, 388}, {4, 472, 392}, {4, 476, 396}, {4, 480, 400},
                     {1, 484, 404}, {1, 485, 405}, {1, 486, 406}, {1, 487, 407}});
    ctx.gpr[3] = load<u8>(rec + 407);
    SetF1(ctx, rec + 392);
}

// DamageColDef::DamageColDef(Def* p, const Record* rec) for 001C28E8 (448
// bytes): Thunk_000E27FC(p, rec), DmgAdCopy (vtable first), vtable
// 0x00B16B00, PtrVectors at +400 / +420 with the capacities rec +328 /
// +348, f32 +440 / bytes +444..+447 from rec +360..+367, then rec's
// pointer arrays {+320, count +328} / {+340, count +348} pushed. r3 = the
// count rec +348, f1 = rec +360.
DECOMP(001C1D4C, DamageColDef_FromRecC1D4C) {
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    call(ctx, m11fn::Thunk_000E27FC, {p, rec});
    DmgAdCopy(p, rec, 0);
    const u32 n0 = load<u16>(rec + 328);
    store<u32>(p, 0x00B16B00u);
    mid1::PtrVecNew(ctx, p + 400, n0, 4, 1);
    const u32 n1 = load<u16>(rec + 348);
    mid1::PtrVecNew(ctx, p + 420, n1, 4, 1);
    CopySeq(p, rec, {{1, 444, 364}, {4, 440, 360}, {1, 445, 365}, {1, 446, 366}, {1, 447, 367}});
    for (u32 i = 0; i < load<u16>(rec + 328); ++i) mid1::PtrVecPush(ctx, p + 400, load<u32>(load<u32>(rec + 320) + 4 * i));
    for (u32 i = 0; i < load<u16>(rec + 348); ++i) mid1::PtrVecPush(ctx, p + 420, load<u32>(load<u32>(rec + 340) + 4 * i));
    ctx.gpr[3] = load<u16>(rec + 348);
    SetF1(ctx, rec + 360);
}

// DamageColDef::DamageColDef(Def* p, const Record* rec) for 001C5408 (928
// bytes): Thunk_000E27FC(p, rec), DmgAdCopy (vtable first); the flag word
// +444 gets bits 30..22 from rec's bytes +366..+374 (stored after each,
// interleaved with the copies of rec +320..+360 to +400..+440 and vtable
// 0x00B16D68) and bits 21..16 cleared; six 80-byte slots at +448 preset
// (+0 0, zero vec3s +16 / +32, the quaternion 0x01164C90 at +48, +64 0,
// bytes +68 1 / +69 0) then memmove(p + 448, rec + 384, 480); the 6-bit
// field 21..16 of +444 = 5 when rec byte +365 is set, else 3 when +364 is
// set (else left 0); then each slot's +64 copied again from rec. r3 = rec
// +848, f1 = the quaternion's last word.
DECOMP(001C463C, DamageColDef_FromRecC463C) {
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    call(ctx, m11fn::Thunk_000E27FC, {p, rec});
    DmgAdCopy(p, rec, 0);
    u32 w = load<u32>(p + 444);
    auto bit = [&](u32 b, u32 off) {
        w = PutBit(w, b, load<u8>(rec + off));
        store<u32>(p + 444, w);
    };
    bit(1, 366);
    bit(2, 367);
    bit(3, 368);
    bit(4, 369);
    CopySeq(p, rec, {{4, 400, 320}, {4, 404, 324}});
    store<u32>(p, 0x00B16D68u);
    bit(5, 370);
    CopySeq(p, rec, {{4, 408, 328}, {4, 432, 352}, {4, 416, 336}});
    bit(6, 371);
    CopySeq(p, rec, {{4, 420, 340}, {4, 428, 348}, {4, 424, 344}, {4, 436, 356}});
    bit(7, 372);
    CopyW(p + 440, rec + 360);
    bit(8, 373);
    w = PutBit(w, 9, load<u8>(rec + 374)) & ~0x003F0000u;
    store<u32>(p + 444, w);
    for (u32 k = 0; k < 6; ++k) {
        const u32 s = p + 448 + 80 * k;
        store<u32>(s, 0);
        CopyW(s + 16, kVecZero);
        CopyW(s + 20, kVecZero + 4);
        CopyW(s + 24, kVecZero + 8);
        CopyW(s + 32, kVecZero);
        CopyW(s + 36, kVecZero + 4);
        CopyW(s + 40, kVecZero + 8);
        CopyW(s + 48, kQuat4C90);
        CopyW(s + 52, kQuat4C90 + 4);
        CopyW(s + 56, kQuat4C90 + 8);
        store<u32>(s + 64, 0);
        CopyW(s + 60, kQuat4C90 + 12);
        store<u8>(s + 68, 1);
        store<u8>(s + 69, 0);
    }
    SetF1(ctx, kQuat4C90 + 12);
    call(ctx, m12fn::Memmove, {p + 448, rec + 384, 480});
    u32 mode = 0;
    if (load<u8>(rec + 365) != 0) mode = 5;
    else if (load<u8>(rec + 364) != 0) mode = 3;
    if (mode != 0) store<u32>(p + 444, (load<u32>(p + 444) & ~0x003F0000u) | (mode << 16));
    for (u32 k = 0; k < 6; ++k) CopyW(p + 512 + 80 * k, rec + 448 + 80 * k);
    ctx.gpr[3] = load<u32>(rec + 848);
}

// DamageColDef::DamageColDef(Def* p, const Record* rec) for 001DE610 (640
// bytes): Thunk_000E27FC(p, rec), rec +224..+556 copied to +304..+636
// (vtable 0x00B17DB0 among them), +600 = 0; when rec's length +516 is set,
// +600 = String_Alloc(n) and String_CopyN(+600, rec +520, n). r3 = 0 or
// String_CopyN's, f1 = rec +556.
DECOMP(001DC5F8, DamageColDef_FromRecDC5F8) {
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    call(ctx, m11fn::Thunk_000E27FC, {p, rec});
    CopySeq(p, rec, {{4, 320, 240}, {4, 324, 244}, {4, 328, 248}, {4, 352, 272}, {4, 356, 276}, {4, 360, 280}, {4, 384, 304},
                     {4, 388, 308}, {4, 392, 312}, {4, 400, 320}, {4, 404, 324}, {4, 408, 328}, {4, 412, 332}, {4, 432, 352},
                     {4, 436, 356}, {4, 440, 360}, {4, 448, 368}, {4, 452, 372}, {4, 456, 376}, {4, 460, 380}, {4, 464, 384},
                     {4, 468, 388}, {4, 472, 392}, {4, 476, 396}, {4, 484, 404}, {4, 488, 408}, {4, 492, 412}, {4, 304, 224},
                     {4, 496, 416}, {4, 500, 420}, {4, 336, 256}, {4, 504, 424}, {4, 508, 428}, {4, 376, 296}, {4, 516, 436},
                     {4, 520, 440}, {4, 380, 300}});
    store<u32>(p, 0x00B17DB0u);
    CopySeq(p, rec, {{4, 524, 444}, {4, 528, 448}, {1, 416, 336}, {4, 532, 452}, {4, 420, 340}, {4, 536, 456}, {1, 417, 337},
                     {4, 540, 460}, {4, 544, 464}, {4, 548, 468}, {4, 480, 400}, {4, 552, 472}, {4, 556, 476}, {4, 560, 480},
                     {1, 512, 432}, {4, 564, 484}, {4, 568, 488}, {1, 513, 433}, {4, 572, 492}, {4, 576, 496}, {4, 580, 500},
                     {4, 584, 504}, {4, 592, 512}, {4, 588, 508}, {4, 596, 516}, {4, 604, 524}, {4, 620, 540}, {1, 616, 536},
                     {4, 608, 528}});
    store<u32>(p + 600, 0);
    CopySeq(p, rec, {{4, 624, 544}, {1, 617, 537}, {4, 612, 532}, {4, 628, 548}, {1, 618, 538}, {4, 632, 552}, {4, 636, 556}});
    SetF1(ctx, rec + 556);
    const u32 n = load<u32>(rec + 516);
    ctx.gpr[3] = n;
    if (n != 0) {
        const u32 s = static_cast<u32>(call(ctx, m12fn::String_Alloc, {n}));
        store<u32>(p + 600, s);
        call(ctx, m1fn::String_CopyN, {s, load<u32>(rec + 520), load<u32>(p + 596)});
    }
}
