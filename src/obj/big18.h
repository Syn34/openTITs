// Game code, large-function sweep part 18: helpers shared by big18_*.cpp
// and their part 17 callers (the def base, the body-record vector, the
// physics object).
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

// A 160-byte Havok body record (word view) and the vector of them {+0
// data, u16 +4 capacity, u16 +8 count, u16 +10 high-water mark}.
struct BodyRec160 {
    be_u32 w[40];  // +0 .. +156 (index = offset / 4)
};
static_assert(sizeof(BodyRec160) == 160);
struct BodyVec160 {
    gp<BodyRec160> data;  // +0
    be_u16 cap;           // +4
    be_u16 elem;          // +6: the element size (set to 160 on construction)
    be_u16 count;         // +8
    be_u16 mark;          // +10
};
static_assert(offsetof(BodyVec160, mark) == 10);

// The def record the shared def constructor reads (beyond DefBase_Init's).
struct DefRec18 {
    pad<12> _0;
    be_u32 name;        // +12
    pad<60> _16;
    be_u32 w76;         // +76
    be_u32 script;      // +80: the lifetime script hash
    be_u32 w84;         // +84
    be_u32 w88[5];      // +88 .. +104
    be_u32 w108;        // +108
    be_u32 w112;        // +112
    be_u32 w116;        // +116
    be_u16 h120;        // +120
    be_u16 h122;        // +122
};
static_assert(offsetof(DefRec18, w76) == 76 && offsetof(DefRec18, w108) == 108 && offsetof(DefRec18, h122) == 122);
// The shared def base (192 bytes).
struct Def18 {
    be_u32 vt;          // +0
    pad<52> _4;
    u8 name[84];        // +56
    be_u32 w140;        // +140
    be_u32 script;      // +144: the lifetime script def
    be_u32 w148;        // +148
    be_u32 w152;        // +152
    be_u32 w156[5];     // +156 .. +172
    be_u32 w176;        // +176
    be_u32 w180;        // +180
    be_u32 w184;        // +184
    be_u16 h188;        // +188
    be_u16 h190;        // +190
};
static_assert(sizeof(Def18) == 192 && offsetof(Def18, w140) == 140 && offsetof(Def18, w176) == 176);

// A Havok rigid body (hkpRigidBody) as the game code touches it.
struct HkBody18 {
    pad<8> _0;
    be_u32 world;      // +8
    pad<30> _12;
    s8 b42;            // +42
    pad<1> _43;
    be_u32 filter;     // +44
    pad<72> _48;
    u8 name[4];        // +120: the name (address taken)
    gp<HkPropEntry> props;  // +124
    be_s32 nprops;     // +128
    pad<8> _132;
    be_u32 w140;       // +140
    be_u32 w144;       // +144
    pad<76> _148;
    u8 motion[8];      // +224: the motion (vtable object; address taken)
    u8 mtype;          // +232: the motion type
    pad<171> _233;
    be_u16 h404;       // +404
    be_u16 h406;       // +406
    pad<102> _408;
    be_u16 h510;       // +510
};
static_assert(offsetof(HkBody18, b42) == 42 && offsetof(HkBody18, name) == 120 && offsetof(HkBody18, w140) == 140 &&
              offsetof(HkBody18, motion) == 224 && offsetof(HkBody18, mtype) == 232 && offsetof(HkBody18, h404) == 404 &&
              offsetof(HkBody18, h510) == 510);
// The physics object's template: +228 a word, +296 the Havok template
// object, +300 flags (u8 +301 its second byte).
struct PhysTmpl18 {
    pad<228> _0;
    be_u32 w228;       // +228
    pad<64> _232;
    be_u32 hk;         // +296
    WordBytes flags;   // +300
};
static_assert(offsetof(PhysTmpl18, hk) == 296 && offsetof(PhysTmpl18, flags) == 300);
struct PhysObj18 {
    be_u32 vt;             // +0
    pad<156> _4;
    be_u32 nameObj;        // +160
    pad<40> _164;
    be_u32 layer;          // +204
    pad<8> _208;
    gp<PhysTmpl18> tmpl;   // +216
    pad<160> _220;
    be_u32 vtA;            // +380: listener vtables (addresses taken)
    be_u32 vtB;            // +384
    pad<12> _388;
    Vec3 vec;              // +400
    pad<4> _412;
    gp<HkBody18> body;     // +416
    be_u32 w420;           // +420
    be_u32 w424;           // +424
    be_u32 w428;           // +428
    be_u16 mode;           // +432: bits 0xF000
    pad<2> _434;
    u8 volume[4];          // +436: the volume (address taken)
};
static_assert(offsetof(PhysObj18, tmpl) == 216 && offsetof(PhysObj18, vtA) == 380 && offsetof(PhysObj18, vec) == 400 &&
              offsetof(PhysObj18, body) == 416 && offsetof(PhysObj18, mode) == 432 && offsetof(PhysObj18, volume) == 436);

// The damage def record (beyond the shared def record): +124 the body
// list, +128 six bytes, +136..+216 tuning words, +220 flags.
struct DamageRec17 {
    DefRec18 base;              // +0
    gp<BodyVec160> list;        // +124
    u8 b128[6];                 // +128
    pad<2> _134;
    be_u32 w[21];               // +136 .. +216 (index = (offset - 136) / 4)
    be_u32 flags;               // +220
};
static_assert(offsetof(DamageRec17, list) == 124 && offsetof(DamageRec17, w) == 136 && offsetof(DamageRec17, flags) == 220);
// The 304-byte damage def: +192 the body-record vector, +196 six bytes,
// +204..+296 tuning words (+208 the kind), +300 flags.
struct DamageDef17 {
    Def18 base;                 // +0
    gp<BodyVec160> vec;         // +192
    u8 b196[6];                 // +196
    pad<2> _202;
    be_u32 w[24];               // +204 .. +296 (index = (offset - 204) / 4)
    be_u32 flags;               // +300
};
static_assert(sizeof(DamageDef17) == 304 && offsetof(DamageDef17, w) == 204);
// The leaf def record: words +124..+340 (+128..+136 the box vector, +144
// the entity name, +148 the mass, +152..+164 tuning, +340 flags).
struct LeafRec17 {
    DefRec18 base;              // +0
    be_u32 w[55];               // +124 .. +340 (index = (offset - 124) / 4)
};
static_assert(offsetof(LeafRec17, w) == 124);
// The 416-byte leaf def: words +192..+396 (bytes +352 / +353, +372 flags,
// +384..+392 the box vector), +400 the body.
struct LeafDef17 {
    Def18 base;                 // +0
    WordBytes w[52];            // +192 .. +396 (index = (offset - 192) / 4)
    be_u32 body;                // +400
};
static_assert(offsetof(LeafDef17, body) == 400);
void DamageDef_Ctor_000C79C0(DamageDef17* p, const DamageRec17* rec);
void DefLeaf_Construct_000C30E8(LeafDef17* p, const LeafRec17* rec);
void BodyVector_Add_00745720(BodyVec160* v, const BodyRec160* s);
void DefBase_Ctor_000C2E8C(Def18* p, const DefRec18* rec);
void PhysObj_Finish_001F9F70(PhysObj18* o, s32 mode);

}  // namespace sly4::obj
