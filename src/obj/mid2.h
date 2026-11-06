// Game code, medium sweep part 2: helpers shared by mid2_*.cpp (still
// recompiled game functions of 120-400 instructions, in coherent groups:
// more UI slot-def readers, the handle slot-list inserts, the level-section
// destructors and the 0x00B20218 class's destructor thunks).
#pragma once

#include <bit>
#include <cmath>

#include "mid1.h"

namespace sly4 {
namespace mid2 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj19::VecDtor;

namespace m2fn {
constexpr u32 Mem_Alloc = 0x0054ED38;           // (size, pool, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;        // (p, pool)
constexpr u32 SmallFree = 0x00552420;           // (p, size)
constexpr u32 PtrVector_Clear = 0x0054EC1C;     // (vec)
constexpr u32 Nop = 0x0054EC18;                 // (p)
constexpr u32 Array_Destruct = 0x0054EC94;      // (Array*) (empty)
constexpr u32 memset = 0x00726E54;              // (p, c, n) -> p
constexpr u32 memmove = 0x0071C3F0;             // (dst, src, n)
constexpr u32 String_Copy = 0x00553684;         // (dst, src, size): strncpy + terminate
constexpr u32 Ptr_FreeAndClear_00321240 = 0x00321240;  // (void** pp)
constexpr u32 Obj_BaseDtor_0048F6B0 = 0x0048F6B0;      // (o): the section classes' base destructor (name?)
constexpr u32 Dtor_B219D0_00490B34 = 0x00490B34;       // (o): ~T() of vtable 0x00B219D0
constexpr u32 Section_Prepare_0032AC78 = 0x0032AC78;   // (o, 0, 1): called first by VtB1E5D0_Dtor (name?)
// The SlotDef readers.
constexpr u32 SlotDef_Read = 0x0035C100;               // (SlotDef*, x, XmlCursor*) -> kind
constexpr u32 SlotDefB_Read_0035D708 = 0x0035D708;     // (SlotDefB*, x, XmlCursor*) -> s
constexpr u32 SlotDefC_Read_0035CA00 = 0x0035CA00;     // (SlotDefC*, x, XmlCursor*) (name?)
constexpr u32 SlotDefList_Read_0035C3E4 = 0x0035C3E4;  // (SlotDefList*, x, XmlCursor*) -> s
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;   // (cursor, name) -> const char* or 0
constexpr u32 XmlCursor_ReadBool = 0x004F986C;         // (cursor, hash, u8* out, bool default)
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;          // (cursor, hash, s32* out, s32 default)
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;          // (cursor, hash, f32* out; f1 default)
constexpr u32 XmlCursor_ReadColor = 0x004F9F9C;        // (cursor, hash, u8[4]* out, default)
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;         // (cursor, hash, u32* out, u32 default) -> 0 when found
constexpr u32 XmlCursor_ReadString_004FA2AC = 0x004FA2AC;  // (cursor, hash, const char** out, const char* default) (name?)
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;       // (cursor, tag hash)
constexpr u32 XmlCursor_NextSibling = 0x004F8FC8;      // (cursor, tag hash)
constexpr u32 Xml_FindChildren = 0x004F936C;           // (cursor, name) -> the first child named so, or 0
constexpr u32 Xml_NextSibling_004F9078 = 0x004F9078;   // (cursor, name) -> the next sibling named so, or 0 (name?)
constexpr u32 Ui_Find = 0x004D5544;                    // (path, parent) -> element
}  // namespace m2fn

// ---------------------------------------------------------------- slot defs

constexpr u32 kSlotDefaultVec = 0x01164B40;  // vec3 (the zero vector)
// Default colours (u8[4]) the derived defs start from (bss, set at boot).
constexpr u32 kSlotColorA0 = 0x01164CA0;
constexpr u32 kSlotColorAC = 0x01164CAC;
constexpr u32 kSlotColorB0 = 0x01164CB0;
constexpr u32 kSlotColorC4 = 0x01164CC4;
constexpr u32 kVtSlotDef = 0x00B1F668;
constexpr u32 kStr3DTextObject = 0x00ADC9E4;  // "3DTextObject"
constexpr u32 kStrParentFrame = 0x00ADC9F8;   // "parentFrame2D"
constexpr u32 kStrTextFrame = 0x00ADCA08;     // "textFrame2D"
constexpr u32 kStrText = 0x00ADCA18;          // "text2D"
constexpr u32 kStrEmpty = 0x00ADCAA8;         // ""

constexpr u32 kOneF = 0x3F800000;     // 1.0f
constexpr u32 kMinusOneF = 0xBF800000;  // -1.0f

// The inlined SlotDef constructor up to the derived class's vtable (as in
// mid1's SlotDefInit, without the +148 store the derived classes make
// their own): the two vec3s +96 / +128 from kSlotDefaultVec, vtable
// kVtSlotDef, kind 5, the strings, the hashes, +80 and +112 cleared, +144
// bits 31 / 30 cleared. f1 = the last lfs, as the original leaves it.
inline void SlotDefBaseInit(PPUContext& ctx, u32 p) {
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 96 + 4 * k, load<u32>(kSlotDefaultVec + 4 * k));  // lfs / stfs
    store<u32>(p + 0, kVtSlotDef);
    store<u32>(p + 68, 0);
    store<u32>(p + 72, 0);
    store<u32>(p + 76, 0);
    store<u32>(p + 84, 5);
    const u32 f144 = load<u32>(p + 144);
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 128 + 4 * k, load<u32>(kSlotDefaultVec + 4 * k));
    store<u32>(p + 112, 0);
    store<u8>(p + 4, 0);
    store<u32>(p + 144, f144 & 0x3FFFFFFFu);
    store<u8>(p + 36, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(p + 80 + k, 0);
    ctx.set_fpr_bits(1, LfsBits(load<u32>(kSlotDefaultVec + 8)));
}

// A u8[4] colour copied byte by byte.
inline void CopyColor(u32 dst, u32 src) {
    for (u32 k = 0; k < 4; ++k) store<u8>(dst + k, load<u8>(src + k));
}

// ---------------------------------------------------------------- slot lists

// The global handle generation counter shared by every SlotList: bumped
// on each insert, wrapping from 0xFFFF to 1.
constexpr u32 kSlotListGen = 0x010C29DC;

inline u32 SlotListNextGen() {
    u32 g = load<u32>(kSlotListGen) + 1;
    store<u32>(kSlotListGen, g);
    if (g > 0xFFFFu) {
        g = 1;
        store<u32>(kSlotListGen, 1);
    }
    return g;
}

// SlotList {+0 free head, +4 free tail, +8 head, +12 tail, +16 count, +20
// free count, +24 capacity, +28 the 16-byte slots {+0 prev, +4 next, +8
// value, +12 bit 0x80 of the first byte = free, bits 0x7FFF8000 = the
// generation}, +32 u8 growable}. A handle is index << 16 | generation.
inline u32 SlotAt(u32 l, u32 i) { return load<u32>(l + 28) + ((i << 4) & 0xFFFFFFF0u); }

// The handle of slot i (the original builds it with sth into a stack word).
inline u32 SlotHandle(u32 l, u32 i) { return ((i & 0xFFFFu) << 16) | ((load<u32>(SlotAt(l, i) + 12) >> 15) & 0xFFFFu); }

// Sets slot i's generation and builds the handle (index << 16 | gen);
// *flags = the slot's +12 address (computed once, before the store).
inline u32 SlotStamp(u32 l, u32 i, u32 g, u32* flags) {
    const u32 h = (i << 16) | (g & 0xFFFFu);
    const u32 e = SlotAt(l, i) + 12;
    store<u32>(e, (load<u32>(e) & ~0x7FFF8000u) | ((g << 15) & 0x7FFF8000u));
    *flags = e;
    return h;
}

// The full-list case shared by both inserts: no room and not growable ->
// false (the caller writes the null handle); else grow(list, capacity * 2).
inline bool SlotListMakeRoom(PPUContext& ctx, u32 l, u32 cap, u32 grow) {
    if (load<u8>(l + 32) == 0) return false;
    call(ctx, grow, {l, obj25::Sx((cap << 1) & 0xFFFFFFFEu)});
    return true;
}

}  // namespace mid2
}  // namespace sly4
