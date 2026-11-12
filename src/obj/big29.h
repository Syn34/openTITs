// Game code, large-function sweep part 29: types shared by big29_*.cpp
// (the XML def defaults record).
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

namespace b29fn {
constexpr u32 String_Alloc = 0x0055578C;  // (n) -> char* (reads r3 only)
constexpr u32 String_CopyN = 0x00553638;  // (dst, src, n)
}  // namespace b29fn

// The source block DefRec_Construct copies (raw words, with holes) to the
// record's +176.
struct DefRecSrc {
    union {
        be_u32 w[120];
        u8 b[480];
    };
};
static_assert(sizeof(DefRecSrc) == 480);
// The XML def defaults record (684 bytes; DefRec_Construct_0075C364).
struct DefRec {
    be_u32 kind;          // +0: 2
    be_u32 w4;            // +4
    be_u32 flags;         // +8: |= 0x400 (constructed), 0x800 (full defaults)
    be_u32 a;             // +12
    be_u32 b;             // +16
    be_u32 c;             // +20
    u8 z24[8];            // +24
    be_u32 w32;           // +32
    be_f32 f36;           // +36
    be_f32 f40;           // +40
    u8 b44;               // +44
    pad<3> _45;
    be_u32 w48[6];        // +48 .. +68
    be_f32 f72;           // +72
    be_f32 f76;           // +76
    be_u32 w80;           // +80
    be_u32 w84;           // +84
    be_f32 f88[5];        // +88
    be_u32 w108;          // +108
    be_u32 w112;          // +112
    be_u32 w116;          // +116
    be_u16 flags_half;    // +120
    be_u16 h122;          // +122
    u8 b124[6];           // +124
    pad<2> _130;
    Quad params[2];       // +132
    pad<12> _164;
    DefRecSrc copy;       // +176
    be_u32 r10;           // +656
    be_f32 f660;          // +660
    be_u32 w664;          // +664: bit 31
    be_u32 len;           // +668: the name's length
    be_u32 name;          // +672: char* (String_Alloc)
    be_u32 a20;           // +676
    be_u32 w680;          // +680: bit 31
};
static_assert(sizeof(DefRec) == 684 && offsetof(DefRec, params) == 132 && offsetof(DefRec, copy) == 176);
static_assert(offsetof(DefRec, r10) == 656 && offsetof(DefRec, w680) == 680);
// The full record: the defaults +684..+1540 (three PtrVectors at +760 /
// +1140 / +1328 among them), as raw words / halves / bytes.
struct DefRecFull {
    DefRec base;  // +0
    union {
        be_u32 w[215];
        be_u16 h[430];
        u8 b[860];
    } d;          // +684
};
static_assert(sizeof(DefRecFull) == 1544);

}  // namespace sly4::obj
