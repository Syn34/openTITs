// Game code, large-function sweep part 1: the UI slot-def reader of the
// 448-byte SlotDefC class (vtable 0x00B1F9F8) and the factory that builds
// every SlotDef kind from the XML children (siblings of mid1_slot.cpp /
// mid2_slot.cpp; SlotDef layout: see mid1_slot.cpp).
#include "big1.h"

using namespace sly4;
using namespace sly4::big1;

namespace b1s {
using namespace mid2;
constexpr u32 kStrTLPoint = 0x00ADCA24;  // "TLPoint"
constexpr u32 kStrTRPoint = 0x00ADCA2C;  // "TRPoint"
constexpr u32 kStrBLPoint = 0x00ADCA34;  // "BLPoint"
constexpr u32 kStrBRPoint = 0x00ADCA3C;  // "BRPoint"
}  // namespace b1s

namespace {

// SlotDefC's 3D-text entries: up to 40 36-byte entries built on the stack
// (zeroed by memset, +20 = 0xD8A5F8E7, +24 = 0x3C193BAC) and copied to the
// heap at the end. Stack layout as the original's: the hash at sp + 112,
// the entries at sp + 116, the bool at sp + 1556 right after them. A 41st
// child overruns past the buffer (in the original into the register save
// area); the frame here keeps some slack instead.
constexpr u32 kMaxEntries = 40;
constexpr u32 kEntryBytes = 36;
constexpr u32 kEntryHash = 0x934CAC5E;

// One entry's text frames (as mid2's ReadTextFrames, keeping the
// "textFrame2D" call's result): +8 the "parentFrame2D" element; when found,
// +12 its "textFrame2D" child and +16 the "text2D" element under +12 (or
// +8) when its type (vfn[3]) is 3 or 5. The entry address is re-read from
// s->+148 after every call, as the original. Returns false when there is
// no parent frame.
bool ReadTextFrames(PPUContext& ctx, u32 s, u32 off, u32 c) {
    auto ent = [&]() { return load<u32>(s + 148) + off; };
    u32 path = static_cast<u32>(call(ctx, b1fn::XmlCursor_FindAttrByName, {c, b1s::kStrParentFrame}));
    const u32 pf = static_cast<u32>(call(ctx, b1fn::Ui_Find, {path, 0}));
    store<u32>(ent() + 8, pf);
    if (pf == 0) return false;
    path = static_cast<u32>(call(ctx, b1fn::XmlCursor_FindAttrByName, {c, b1s::kStrTextFrame}));
    const u32 tf = static_cast<u32>(call(ctx, b1fn::Ui_Find, {path, load<u32>(ent() + 8)}));
    const u32 e = ent();
    store<u32>(e + 12, tf);
    const u32 parent = tf != 0 ? tf : load<u32>(e + 8);
    path = static_cast<u32>(call(ctx, b1fn::XmlCursor_FindAttrByName, {c, b1s::kStrText}));
    const u32 t = static_cast<u32>(call(ctx, b1fn::Ui_Find, {path, parent}));
    if (t == 0) return true;
    bool ok = static_cast<u32>(VCall(ctx, t, 12, {t})) == 3;
    if (!ok) ok = static_cast<u32>(VCall(ctx, t, 12, {t})) == 5;
    if (ok) store<u32>(ent() + 16, t);
    return true;
}

// The rest of an entry: +0 the colour 0x0F1B2B8E, +20 / +24 the hashes
// 0xB4B14A17 / 0xE00F9276, +28 bits 31 / 30 the bools 0x6F82CABE /
// 0xBE95A9DF (through the stack byte tb), +32 the hash 0x91B.
void ReadEntryRest(PPUContext& ctx, u32 s, u32 off, u32 c, u32 tb) {
    auto ent = [&]() { return load<u32>(s + 148) + off; };
    call(ctx, b1fn::XmlCursor_ReadColor, {c, 0x0F1B2B8Eu, ent(), ent()});
    ReadHash(ctx, c, 0xB4B14A17u, ent() + 20);
    ReadHash(ctx, c, 0xE00F9276u, ent() + 24);
    store<u8>(tb, 0);
    call(ctx, b1fn::XmlCursor_ReadBool, {c, 0x6F82CABEu, tb, load<u32>(ent() + 28) >> 31});
    u32 w = load<u32>(ent() + 28);
    store<u32>(ent() + 28, (w & 0x7FFFFFFFu) | ((load<u8>(tb) & 1u) << 31));
    call(ctx, b1fn::XmlCursor_ReadBool, {c, 0xBE95A9DFu, tb, (load<u32>(ent() + 28) >> 30) & 1u});
    w = load<u32>(ent() + 28);
    store<u32>(ent() + 28, (w & ~0x40000000u) | ((load<u8>(tb) & 1u) << 30));
    ReadHash(ctx, c, 0x91Bu, ent() + 32);
}

// The "3DTextObject" children into s->+148 / +152 (the count), through the
// stack buffer (s->+148 points at it while reading, as the original), then
// copied to a new heap array when there are any.
void ReadTextEntries(PPUContext& ctx, u32 s, u32 cur, u32 tmp, u32 buf, u32 tb) {
    store<u32>(s + 152, 0);
    store<u32>(s + 148, 0);
    u32 c = static_cast<u32>(call(ctx, b1fn::Xml_FindChildren, {cur, b1s::kStr3DTextObject}));
    for (u32 off = 0; c != 0; off += kEntryBytes) {
        const u32 n = load<u32>(s + 152);
        store<u32>(s + 148, buf);
        store<u32>(tmp, 0);
        store<u32>(s + 152, n + 1);
        if (static_cast<u32>(call(ctx, b1fn::XmlCursor_ReadHash, {c, kEntryHash, tmp, 0})) == 0) {
            store<u32>(load<u32>(s + 148) + off + 4, load<u32>(tmp));
            if (ReadTextFrames(ctx, s, off, c)) ReadEntryRest(ctx, s, off, c, tb);
        }
        c = static_cast<u32>(call(ctx, b1fn::Xml_NextSibling_004F9078, {c, b1s::kStr3DTextObject}));
    }
    if (load<u32>(s + 152) == 0) return;
    const u32 p = static_cast<u32>(call(ctx, b1fn::Mem_Alloc, {load<u32>(s + 152) * kEntryBytes, 0, 16}));
    const u32 a = static_cast<u32>(call(ctx, b1fn::memset, {p, 0, load<u32>(s + 152) * kEntryBytes}));
    store<u32>(s + 148, a);
    call(ctx, b1fn::memmove, {a, buf, load<u32>(s + 152) * kEntryBytes});
}

// One of the four corner children ("TLPoint" ...): the hash 0x0024728B
// into p and the colour 0x5399A8B2 into p + 4.
void ReadCorner(PPUContext& ctx, u32 cur, u32 name, u32 p) {
    const u32 c = static_cast<u32>(call(ctx, b1fn::Xml_FindChildren, {cur, name}));
    if (c == 0) return;
    ReadHash(ctx, c, 0x0024728Bu, p);
    ReadColor(ctx, c, 0x5399A8B2u, p + 4);
}

}  // namespace

// SlotDefC* SlotDefC_Read_0035CA00(SlotDefC* s, x, XmlCursor* cur):
// SlotDef_Read, the 36-byte "3DTextObject" entries (+148 / +152), the float
// 0xE690CE6F (+156) and the bool 0x280D85C8 (+160); the four corners
// +168..+199 (cleared, colours from the default 0x01164CA0, then the
// "TLPoint" / "TRPoint" / "BLPoint" / "BRPoint" children); then the hashes,
// names (32 chars: +208, +260, +292, +324, +356), colours and floats of
// the rest of the def. Returns s.
DECOMP(0035CA00, SlotDefC_Read_0035CA00) {
    const u32 s = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), cur = arg<u32>(ctx, 2);
    Stack frame(ctx, 4 + kMaxEntries * kEntryBytes + 4 + 128);
    const u32 tmp = frame.addr();  // the original's sp + 112
    const u32 buf = tmp + 4;       // sp + 116
    const u32 tb = buf + kMaxEntries * kEntryBytes;  // sp + 1556
    for (u32 k = 0; k < kMaxEntries; ++k) {
        const u32 e = buf + kEntryBytes * k;
        call(ctx, b1fn::memset, {e, 0, kEntryBytes});
        store<u32>(e + 20, 0xD8A5F8E7u);
        store<u32>(e + 24, 0x3C193BACu);
    }
    call(ctx, b1fn::SlotDef_Read, {s, x, cur});
    ReadTextEntries(ctx, s, cur, tmp, buf, tb);
    ReadF32(ctx, cur, 0xE690CE6Fu, s + 156);
    ReadBool(ctx, cur, 0x280D85C8u, s + 160);
    for (u32 off = 168; off < 200; off += 4) store<u32>(s + off, 0);  // four std
    for (u32 k = 0; k < 4; ++k) mid2::CopyColor(s + 172 + 8 * k, mid2::kSlotColorA0);
    ReadCorner(ctx, cur, b1s::kStrTLPoint, s + 168);
    ReadCorner(ctx, cur, b1s::kStrTRPoint, s + 176);
    ReadCorner(ctx, cur, b1s::kStrBLPoint, s + 184);
    ReadCorner(ctx, cur, b1s::kStrBRPoint, s + 192);
    ReadHash(ctx, cur, 0x5A63486Cu, s + 164);
    ReadHash(ctx, cur, 0xA3DAD21Cu, s + 200);
    ReadHash(ctx, cur, 0xFC3E33C2u, s + 204);
    ReadName(ctx, cur, 0x654B8263u, s + 208);
    ReadColor(ctx, cur, 0x39887387u, s + 240);
    static constexpr u32 kHashes1[] = {0x624CED18u, 0x624CED1Eu, 0x624CEAEAu, 0x624CEAF0u};
    for (u32 k = 0; k < 4; ++k) ReadHash(ctx, cur, kHashes1[k], s + 244 + 4 * k);
    ReadName(ctx, cur, 0xC86B54EDu, s + 260);
    ReadName(ctx, cur, 0x05D0B873u, s + 292);
    ReadName(ctx, cur, 0x7A962D3Fu, s + 324);
    ReadName(ctx, cur, 0xB7FB90C5u, s + 356);
    static constexpr u32 kHashes2[] = {0x5CCAD404u, 0x1395597Cu, 0x4853B1E7u, 0x2D45A764u, 0x429F935Au, 0x08B994E1u};
    for (u32 k = 0; k < 6; ++k) ReadHash(ctx, cur, kHashes2[k], s + 388 + 4 * k);
    ReadF32(ctx, cur, 0xEA68DF12u, s + 412);
    ReadF32(ctx, cur, 0xC9037FF5u, s + 416);
    ReadHash(ctx, cur, 0x62AA4B26u, s + 420);
    ReadColor(ctx, cur, 0x894AC0DBu, s + 424);
    ReadColor(ctx, cur, 0xAFE4553Du, s + 428);
    ReadColor(ctx, cur, 0x62E42FD6u, s + 432);
    ret(ctx, s);
}

// ---------------------------------------------------------------- factory

namespace b1f {
constexpr u32 Array_Reserve = 0x00558F3C;  // (array, u16 capacity)
constexpr u32 Map_Get = 0x0075EA74;        // (map, const u32* key) -> the value slot
// The def map (hash -> SlotDef*, +0 u16 capacity, +2 u16 count, +4 u16
// high-water mark, +6 u16 flags (bit 0: unsorted), +8 the 8-byte {key,
// value} entries) the defs are appended to, and the map of the 28-byte
// "0xFA405F99" colour records.
constexpr u32 kDefMap = 0x0106C210;
constexpr u32 kColorMap = 0x0106C2A8;
constexpr u32 kSlotColorA8 = 0x01164CA8;
constexpr u32 kTagColorRecord = 0xFA405F99;
// The constructors of the still-recompiled kinds: (x, XmlCursor*) -> def.
constexpr u32 SlotDef_New_0035DB38 = 0x0035DB38;
constexpr u32 SlotDef_New_0035E21C = 0x0035E21C;
constexpr u32 SlotDef_New_0035E928 = 0x0035E928;
constexpr u32 SlotDef_New_0035F074 = 0x0035F074;
constexpr u32 SlotDef_New_0035F8FC = 0x0035F8FC;
constexpr u32 SlotDef_New_00361234 = 0x00361234;
// The decompiled ones.
constexpr u32 SlotDefListB1FA38_New_0035C8CC = 0x0035C8CC;
constexpr u32 SlotDefC_New_0035D4B0 = 0x0035D4B0;
constexpr u32 SlotDefB_New_0035D958 = 0x0035D958;
constexpr u32 SlotDefMsg_New_003602D4 = 0x003602D4;
constexpr u32 SlotDef3DText_New_003604E0 = 0x003604E0;
constexpr u32 SlotDefE_New_003608E8 = 0x003608E8;
constexpr u32 SlotDefD_New_00360DE8 = 0x00360DE8;
constexpr u32 SlotDefListB1F6A0_New_00361108 = 0x00361108;
constexpr u32 SlotDefB780_New_00361374 = 0x00361374;
constexpr u32 SlotDefB748_New_003616A4 = 0x003616A4;
}  // namespace b1f

namespace {

// Appends {key, value} to a map without sorting it: grown to twice the
// capacity when full; the high-water mark is raised and flag bit 0 (unsorted)
// set.
void MapAppend(PPUContext& ctx, u32 m, u32 key, u32 value) {
    u32 n = load<u16>(m + 2);
    const u32 cap = load<u16>(m + 0);
    if (n == cap) {
        call(ctx, b1f::Array_Reserve, {m, (cap << 1) & 0xFFFEu});
        n = load<u16>(m + 2);
    }
    const u32 e = load<u32>(m + 8) + (n << 3);
    store<u32>(e, key);
    store<u16>(m + 2, static_cast<u16>(n + 1));
    store<u32>(e + 4, value);
    const u32 hw = load<u16>(m + 4), fl = load<u16>(m + 6);
    store<u16>(m + 4, static_cast<u16>(hw > ((n + 1) & 0xFFFFu) ? hw : n + 1));
    store<u16>(m + 6, static_cast<u16>(fl | 1u));
}

// The constructor for an element tag, or 0 for the colour record / unknown tags.
u32 SlotDefCtor(u32 tag) {
    switch (tag) {
    case 0x40BB98C4u: return b1f::SlotDef_New_0035F074;
    case 0x7FAACD05u: return b1f::SlotDefC_New_0035D4B0;
    case 0x75AEE2B6u: return b1f::SlotDef_New_0035F8FC;
    case 0x74194F1Au: return b1f::SlotDefD_New_00360DE8;
    case 0x50EF70F0u: return b1f::SlotDefMsg_New_003602D4;
    case 0x4071F06Fu: return b1f::SlotDefB_New_0035D958;
    case 0x3D883554u: return b1f::SlotDef_New_0035E928;
    case 0x2B1B81ABu: return b1f::SlotDef_New_00361234;
    case 0x043A35DDu: return b1f::SlotDef_New_0035DB38;
    case 0xCFF60149u: return b1f::SlotDefListB1F6A0_New_00361108;
    case 0xFC34BC83u: return b1f::SlotDef_New_0035E21C;
    case 0xE131E1A4u: return b1f::SlotDefE_New_003608E8;
    case 0xD22F2156u: return b1f::SlotDefB748_New_003616A4;
    case 0xCB5728FBu: return b1f::SlotDefListB1FA38_New_0035C8CC;
    case 0x91E9E41Au: return b1f::SlotDef3DText_New_003604E0;
    case 0x81DE879Du: return b1f::SlotDefB780_New_00361374;
    default: return 0;
    }
}

// The 28-byte colour record {+0 the key 0x0024728B, +4 / +8 the hashes
// 0x7A30D7E3 / 0x760CB725, +12 / +16 / +20 the colours 0xEAB9B1D2 /
// 0xEAB9B1D3 / 0x68762E36 (defaults 0x01164CA8, 0x01164CA8, 0x01164CA0 at
// construction, 0x01164CA0 when read), +24 the hash 0x31F8E7E5}: added to
// kColorMap unless its key is 0 or already there (then freed). A failed
// allocation still reads into address 0, as the original.
void ReadColorRecord(PPUContext& ctx, u32 cur, u32 key) {
    u32 p = static_cast<u32>(call(ctx, b1fn::Mem_Alloc, {28, 0, 16}));
    if (p != 0) {
        store<u32>(p + 0, 0);
        store<u32>(p + 4, 0);
        store<u32>(p + 8, 0);
        mid2::CopyColor(p + 12, b1f::kSlotColorA8);
        mid2::CopyColor(p + 16, b1f::kSlotColorA8);
        mid2::CopyColor(p + 20, mid2::kSlotColorA0);
        store<u32>(p + 24, 0);
    }
    call(ctx, b1fn::XmlCursor_ReadHash, {cur, 0x0024728Bu, key, 0});
    if (load<u32>(key) != 0) {
        store<u32>(p + 0, load<u32>(key));
        call(ctx, b1fn::XmlCursor_ReadHash, {cur, 0x7A30D7E3u, p + 4, 0});
        call(ctx, b1fn::XmlCursor_ReadHash, {cur, 0x760CB725u, p + 8, 0});
        call(ctx, b1fn::XmlCursor_ReadColor, {cur, 0xEAB9B1D2u, p + 12, mid2::kSlotColorA0});
        call(ctx, b1fn::XmlCursor_ReadColor, {cur, 0xEAB9B1D3u, p + 16, mid2::kSlotColorA0});
        call(ctx, b1fn::XmlCursor_ReadColor, {cur, 0x68762E36u, p + 20, mid2::kSlotColorA0});
        call(ctx, b1fn::XmlCursor_ReadHash, {cur, 0x31F8E7E5u, p + 24, 0});
        const u32 slot = static_cast<u32>(call(ctx, b1f::Map_Get, {b1f::kColorMap, p}));
        if (load<u32>(slot) == 0) {
            MapAppend(ctx, b1f::kColorMap, load<u32>(p + 0), p);
            return;
        }
    }
    if (p != 0) call(ctx, b1fn::Mem_FreeFrom, {p, 0});
}

}  // namespace

// void SlotDef_Factory_0036196C(x, XmlCursor* cur): for every child element
// of cur with flag 0x80 at +24, the SlotDef kind its tag (+20) names is
// built (ctor(x, child)) and appended to the def map under its hash (+68;
// read even from a null def, as the original); tag 0xFA405F99 is a colour
// record instead. Returns with r3 = 0 (the last cursor call's result).
DECOMP(0036196C, SlotDef_Factory_0036196C) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 key = frame.addr();  // the original's sp + 112
    u32 c = static_cast<u32>(call(ctx, b1fn::XmlCursor_FirstChildAny, {cur}));
    while (c != 0) {
        if (load<u8>(c + 24) & 0x80u) {
            const u32 tag = load<u32>(c + 20);
            if (tag == b1f::kTagColorRecord) {
                ReadColorRecord(ctx, c, key);
            } else if (const u32 ctor = SlotDefCtor(tag); ctor != 0) {
                const u32 d = static_cast<u32>(call(ctx, ctor, {x, c}));
                MapAppend(ctx, b1f::kDefMap, load<u32>(d + 68), d);
            }
        }
        c = static_cast<u32>(call(ctx, b1fn::XmlCursor_NextSiblingAny, {c}));
    }
    ret(ctx, 0u);
}
