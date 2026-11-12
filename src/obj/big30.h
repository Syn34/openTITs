// Game code, large-function sweep part 30: types shared by big30_*.cpp
// (the typed references Section_Ready resets and Ref_Resolve resolves).
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

// A 20-byte typed reference: +0 the id, +4 the kind (0..7), +8 a word whose
// bits 31..30 are the entry's show mode and whose bytes +9 / +10 / +11 are
// its "hidden" / "enabled" / "missing" flags, +12 u8 pending, +16 the
// resolved pointer.
struct Ref {
    be_u32 id;      // +0
    be_s32 kind;    // +4
    WordBytes u;    // +8
    u8 pending;     // +12
    pad<3> _13;
    be_u32 target;  // +16
};
static_assert(sizeof(Ref) == 20 && offsetof(Ref, target) == 16);
// An array of Refs: +0 data, +8 u16 count.
struct RefVec {
    gp<Ref> data;    // +0
    pad<4> _4;
    be_u16 count;    // +8
};
static_assert(offsetof(RefVec, count) == 8);

void Ref_Resolve_00321324(Ref* r, u8 warn);

// big30_voice.cpp: stops the voices of an object tree (key, release).
struct VoiceObj;
void Obj_StopVoiceTree_000DC314(VoiceObj* o, u32 key, u8 rel);

}  // namespace sly4::obj
