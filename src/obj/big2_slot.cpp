// Game code, large-function sweep part 2: the SlotDef constructors the
// factory SlotDef_Factory_0036196C (big1_slot.cpp) calls and part 1 left
// recompiled: the three "Checkboxes" defs, the vec3 / record def, the
// "State" def and the 160-byte bool def (SlotDef layout: see
// mid1_slot.cpp; the base constructor: mid2::SlotDefBaseInit).
#include "big2.h"

using namespace sly4;
using namespace sly4::big2;

namespace b2s {
using namespace mid2;
constexpr u32 kVtSlotDefCb940 = 0x00B1F940;   // 0035DB38
constexpr u32 kVtSlotDefCb908 = 0x00B1F908;   // 0035E21C
constexpr u32 kVtSlotDefCb8D0 = 0x00B1F8D0;   // 0035E928
constexpr u32 kVtSlotDefRec = 0x00B1F898;     // 0035F074
constexpr u32 kVtSlotDefState = 0x00B1F860;   // 0035F8FC
constexpr u32 kVtSlotDefBool = 0x00B1F7B8;    // 00361234
constexpr u32 kStrCheckboxes = 0x00ADCA44;    // "Checkboxes"
constexpr u32 kStrCheckbox = 0x00ADCA54;      // "Checkbox"
constexpr u32 kStrState = 0x00ADCA60;         // "State"
constexpr u32 kStrTLPoint = 0x00ADCA24;       // "TLPoint"
constexpr u32 kStrTRPoint = 0x00ADCA2C;       // "TRPoint"
constexpr u32 kStrBLPoint = 0x00ADCA34;       // "BLPoint"
constexpr u32 kStrBRPoint = 0x00ADCA3C;       // "BRPoint"
// The three state names' hashes (s32[3], .rodata right after "State"):
// a state attribute matching entry i stores i.
constexpr u32 kStateHashes = 0x00ADCA68;
constexpr u32 kTagRecord = 0x0026E1FE;        // 0035F074's 56-byte record children
}  // namespace b2s

namespace {

constexpr u32 kHashText = 0x934CAC5Eu;     // a 3D text entry's id
constexpr u32 kHashTextAux = 0x0000091Bu;
constexpr u32 kHashTextColor = 0x0F1B2B8Eu;
constexpr u32 kHashTextBool = 0xBE95A9DFu;
constexpr u32 kHashCheckbox = 0x2911AA63u;

// The text frames of one 3D text entry, the entry's address re-read
// through ent() after every call (as the original): +o the
// "parentFrame2D" element; when found, +o+4 its "textFrame2D" child and
// +o+8 the "text2D" element under +o+4 (or +o) when its type (vfn[3]) is
// 3 or 5. Returns false when there is no parent frame.
template <class Ent>
bool ReadFrames(PPUContext& ctx, u32 c, Ent ent, u32 o) {
    u32 path = static_cast<u32>(call(ctx, b2fn::XmlCursor_FindAttrByName, {c, b2s::kStrParentFrame}));
    const u32 pf = static_cast<u32>(call(ctx, b2fn::Ui_Find, {path, 0}));
    store<u32>(ent() + o, pf);
    if (pf == 0) return false;
    path = static_cast<u32>(call(ctx, b2fn::XmlCursor_FindAttrByName, {c, b2s::kStrTextFrame}));
    const u32 tf = static_cast<u32>(call(ctx, b2fn::Ui_Find, {path, load<u32>(ent() + o)}));
    store<u32>(ent() + o + 4, tf);
    const u32 e = ent();
    u32 parent = load<u32>(e + o + 4);
    if (parent == 0) parent = load<u32>(e + o);
    path = static_cast<u32>(call(ctx, b2fn::XmlCursor_FindAttrByName, {c, b2s::kStrText}));
    const u32 t = static_cast<u32>(call(ctx, b2fn::Ui_Find, {path, parent}));
    if (t == 0) return true;
    bool ok = static_cast<u32>(VCall(ctx, t, 12, {t})) == 3;
    if (!ok) ok = static_cast<u32>(VCall(ctx, t, 12, {t})) == 5;
    if (ok) store<u32>(ent() + o + 8, t);
    return true;
}

// The bool 0xBE95A9DF into bit 31 of the word at ent() + 24 (through the
// stack byte tb, cleared first; the default is the current bit).
template <class Ent>
void ReadTextBool(PPUContext& ctx, u32 c, Ent ent, u32 tb) {
    store<u8>(tb, 0);
    call(ctx, b2fn::XmlCursor_ReadBool, {c, kHashTextBool, tb, load<u32>(ent() + 24) >> 31});
    const u32 w = load<u32>(ent() + 24);
    store<u32>(ent() + 24, (w & 0x7FFFFFFFu) | ((load<u8>(tb) & 1u) << 31));
}

// The checkbox block of the three "Checkboxes" defs at s + b: when there
// is a "Checkboxes" child, its hash 0x2911AA63 into +b, then (when the s32
// at +148 is positive) a zeroed array of that many u32 at +b+4 and the
// hash 0x2911AA63 (default 0) of each "Checkbox" child under it, in order
// (no bound check against +148, as the original).
void ReadCheckboxes(PPUContext& ctx, u32 s, u32 cur, u32 b) {
    u32 c = static_cast<u32>(call(ctx, b2fn::Xml_FindChildren, {cur, b2s::kStrCheckboxes}));
    if (c == 0) return;
    ReadHash(ctx, c, kHashCheckbox, s + b);
    if (static_cast<s32>(load<u32>(s + 148)) <= 0) return;
    const u32 p = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {load<u32>(s + 148) << 2, 0, 16}));
    store<u32>(s + b + 4, static_cast<u32>(call(ctx, b2fn::memset, {p, 0, load<u32>(s + 148) << 2})));
    c = static_cast<u32>(call(ctx, b2fn::Xml_FindChildren, {c, b2s::kStrCheckbox}));
    for (u32 off = 0; c != 0; off += 4) {
        call(ctx, b2fn::XmlCursor_ReadHash, {c, kHashCheckbox, load<u32>(s + b + 4) + off, 0});
        c = static_cast<u32>(call(ctx, b2fn::Xml_NextSibling_004F9078, {c, b2s::kStrCheckbox}));
    }
}

// The 28-byte 3D text entries of the "Checkboxes" defs: array at s + arr,
// count at s + cnt. The count is the s32 attribute 0xDBA1AC01 of cur
// (read into the stack word n) or, when absent, the number of
// "3DTextObject" children. Entry {+0 the hash 0x91B (default 0), +4 the
// colour 0x0F1B2B8E, +8 the id 0x934CAC5E, +12..+20 the frames, +24 bit
// 31 the bool 0xBE95A9DF}; children without the id only advance the entry
// (no bound check against the count, as the original).
void ReadTextEntries28(PPUContext& ctx, u32 s, u32 cur, u32 arr, u32 cnt, u32 n, u32 id, u32 tb) {
    store<u32>(n, 0);
    u32 c = static_cast<u32>(call(ctx, b2fn::Xml_FindChildren, {cur, b2s::kStr3DTextObject}));
    if (c == 0) return;
    if (static_cast<u32>(call(ctx, b2fn::XmlCursor_ReadS32, {cur, 0xDBA1AC01u, n, 0})) != 0) {
        u32 x = c;
        do {
            store<u32>(n, load<u32>(n) + 1);
            x = static_cast<u32>(call(ctx, b2fn::Xml_NextSibling_004F9078, {x, b2s::kStr3DTextObject}));
        } while (x != 0);
    }
    const u32 count = load<u32>(n);
    store<u32>(s + cnt, count);
    if (static_cast<s32>(count) <= 0) return;
    const u32 p = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {count * 28u, 0, 16}));
    store<u32>(s + arr, static_cast<u32>(call(ctx, b2fn::memset, {p, 0, load<u32>(n) * 28u})));
    for (u32 off = 0; c != 0; off += 28) {
        auto ent = [&]() { return load<u32>(s + arr) + off; };
        store<u32>(id, 0);
        if (static_cast<u32>(call(ctx, b2fn::XmlCursor_ReadHash, {c, kHashText, id, 0})) == 0) {
            call(ctx, b2fn::XmlCursor_ReadHash, {c, kHashTextAux, ent(), 0});
            store<u32>(ent() + 8, load<u32>(id));
            if (ReadFrames(ctx, c, ent, 12)) {
                call(ctx, b2fn::XmlCursor_ReadColor, {c, kHashTextColor, ent() + 4, ent() + 4});
                ReadTextBool(ctx, c, ent, tb);
            }
        }
        c = static_cast<u32>(call(ctx, b2fn::Xml_NextSibling_004F9078, {c, b2s::kStr3DTextObject}));
    }
}

// The 208-byte "Checkboxes" defs' common header: s32s +148 / +152, hashes
// +156 / +160 / +164.
void ReadCbHeader(PPUContext& ctx, u32 s, u32 cur) {
    ReadS32(ctx, cur, 0xBF27DE6Fu, s + 148);
    ReadS32(ctx, cur, 0x073922B6u, s + 152);
    ReadHash(ctx, cur, 0xFC493ADBu, s + 156);
    ReadHash(ctx, cur, 0x90B6CF40u, s + 160);
    ReadHash(ctx, cur, 0x464245FAu, s + 164);
}

// The inlined base constructor, the derived vtable, the cleared words
// and +80 |= flag (store order within the block differs from the
// original's interleaving; nothing in it aliases).
void CbInit(PPUContext& ctx, u32 s, u32 vt, std::initializer_list<u32> zero, u32 flag) {
    mid2::SlotDefBaseInit(ctx, s);
    store<u32>(s + 0, vt);
    for (const u32 off : zero) store<u32>(s + off, 0);
    store<u32>(s + 80, load<u32>(s + 80) | flag);
}

// The common tail: the float 0xE690CE6F (+b) and the bool 0x280D85C8
// (+b+4).
void ReadCbTail(PPUContext& ctx, u32 s, u32 cur, u32 b) {
    ReadF32(ctx, cur, 0xE690CE6Fu, s + b);
    ReadBool(ctx, cur, 0x280D85C8u, s + b + 4);
}

}  // namespace

// SlotDef* SlotDefCb_New_0035DB38(x, XmlCursor* cur): a new 208-byte def
// (vtable 0x00B1F940: +188 = -1.0, +80 |= 0x2000, +148..+164 / +172..+184
// / +192 cleared), SlotDef_Read (on 0 when the allocation fails, as the
// original), the header (ReadCbHeader), the checkboxes (+172 / +176), the
// 28-byte 3D text entries (+180 / +184), the float +188 and the bool +192.
DECOMP(0035DB38, SlotDefCb_New_0035DB38) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 n = frame.addr(), id = n + 4, tb = n + 8;  // the original's sp + 112 / 116 / 120
    const u32 s = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {208, 0, 16}));
    if (s != 0) {
        CbInit(ctx, s, b2s::kVtSlotDefCb940, {148, 152, 156, 160, 164, 172, 176, 180, 184}, 0x2000);
        store<u8>(s + 192, 0);
        store<u32>(s + 188, mid2::kMinusOneF);
    }
    call(ctx, b2fn::SlotDef_Read, {s, x, cur});
    ReadCbHeader(ctx, s, cur);
    ReadCheckboxes(ctx, s, cur, 172);
    ReadTextEntries28(ctx, s, cur, 180, 184, n, id, tb);
    ReadCbTail(ctx, s, cur, 188);
    ret(ctx, s);
}

// SlotDef* SlotDefCb_New_0035E21C(x, XmlCursor* cur): as 0035DB38 with
// vtable 0x00B1F908, +80 |= 0x1000, +168 cleared too and read as the hash
// 0xA3DAD21C after the header.
DECOMP(0035E21C, SlotDefCb_New_0035E21C) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 n = frame.addr(), id = n + 4, tb = n + 8;
    const u32 s = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {208, 0, 16}));
    if (s != 0) {
        CbInit(ctx, s, b2s::kVtSlotDefCb908, {148, 152, 156, 160, 164, 168, 172, 176, 180, 184}, 0x1000);
        store<u8>(s + 192, 0);
        store<u32>(s + 188, mid2::kMinusOneF);
    }
    call(ctx, b2fn::SlotDef_Read, {s, x, cur});
    ReadCbHeader(ctx, s, cur);
    ReadHash(ctx, cur, 0xA3DAD21Cu, s + 168);
    ReadCheckboxes(ctx, s, cur, 172);
    ReadTextEntries28(ctx, s, cur, 180, 184, n, id, tb);
    ReadCbTail(ctx, s, cur, 188);
    ret(ctx, s);
}

// SlotDef* SlotDefCb_New_0035E928(x, XmlCursor* cur): as 0035DB38 with
// vtable 0x00B1F8D0, +80 |= 0x800, the hashes 0xA3DAD21C (+168) /
// 0x7B9F2997 (+172) and the bool 0x379040C0 (+197) after the header (+168
// / +172 not cleared by the constructor), and every later field 4 bytes
// on: checkboxes +176 / +180, entries +184 / +188, float +192, bool +196.
DECOMP(0035E928, SlotDefCb_New_0035E928) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 n = frame.addr(), id = n + 4, tb = n + 8;
    const u32 s = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {208, 0, 16}));
    if (s != 0) {
        CbInit(ctx, s, b2s::kVtSlotDefCb8D0, {148, 152, 156, 160, 164, 176, 180, 184, 188}, 0x800);
        store<u8>(s + 196, 0);
        store<u32>(s + 192, mid2::kMinusOneF);
        store<u8>(s + 197, 0);
    }
    call(ctx, b2fn::SlotDef_Read, {s, x, cur});
    ReadCbHeader(ctx, s, cur);
    ReadHash(ctx, cur, 0xA3DAD21Cu, s + 168);
    ReadHash(ctx, cur, 0x7B9F2997u, s + 172);
    ReadBool(ctx, cur, 0x379040C0u, s + 197);
    ReadCheckboxes(ctx, s, cur, 176);
    ReadTextEntries28(ctx, s, cur, 184, 188, n, id, tb);
    ReadCbTail(ctx, s, cur, 192);
    ret(ctx, s);
}

// SlotDef* SlotDefRec_New_0035F074(x, XmlCursor* cur): a new 224-byte def
// (vtable 0x00B1F898: +176 the vec3 from kSlotDefaultVec, +204 / +208 =
// 1.0, +80 |= 0x400, +148..+160 / +192..+200 / +212 / +216 cleared),
// SlotDef_Read (on 0 when the allocation fails), hashes +156 / +160, the
// vec3 0x33ECE109 (+176), floats +192 / +204 / +208, hashes +212 / +216;
// then one 28-byte 3D text entry per "3DTextObject" child (+196 the
// zeroed array, +200 the count): {+0 the colour 0x0F1B2B8E, +4 the id
// 0x934CAC5E, +8 the hash 0x91B, +12..+20 the frames, +24 bit 31 the bool
// 0xBE95A9DF}; finally one 56-byte record per 0x0026E1FE child (+148 the
// array, cleared up to +24 of each; +152 the count): six hashes +0..+20
// and the 32-char string 0xF7BE5DF7 at +24.
DECOMP(0035F074, SlotDefRec_New_0035F074) {
    const u32 x = arg<u32>(ctx, 0);
    u32 cur = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 id = frame.addr(), tb = id + 4;  // the original's sp + 112 / 116
    const u32 s = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {224, 0, 16}));
    if (s != 0) {
        mid2::SlotDefBaseInit(ctx, s);
        store<u32>(s + 0, b2s::kVtSlotDefRec);
        for (u32 k = 0; k < 3; ++k) store<u32>(s + 176 + 4 * k, load<u32>(mid2::kSlotDefaultVec + 4 * k));  // lfs / stfs
        for (const u32 off : {148u, 152u, 156u, 160u, 192u, 196u, 200u, 212u, 216u}) store<u32>(s + off, 0);
        store<u32>(s + 204, mid2::kOneF);
        store<u32>(s + 208, mid2::kOneF);
        store<u32>(s + 80, load<u32>(s + 80) | 0x400u);
    }
    call(ctx, b2fn::SlotDef_Read, {s, x, cur});
    ReadHash(ctx, cur, 0xEC96E888u, s + 156);
    ReadHash(ctx, cur, 0x5245F8BAu, s + 160);
    call(ctx, b2fn::XmlCursor_ReadVec3, {cur, 0x33ECE109u, s + 176, s + 176});
    ReadF32(ctx, cur, 0x2028F1FAu, s + 192);
    ReadF32(ctx, cur, 0xEA68DF12u, s + 204);
    ReadF32(ctx, cur, 0xC9037FF5u, s + 208);
    ReadHash(ctx, cur, 0x62AA4B26u, s + 212);
    ReadHash(ctx, cur, 0x0027873Au, s + 216);

    u32 c = static_cast<u32>(call(ctx, b2fn::Xml_FindChildren, {cur, b2s::kStr3DTextObject}));
    if (c != 0) {
        u32 n = 0;
        u32 y = c;
        do {
            ++n;
            y = static_cast<u32>(call(ctx, b2fn::Xml_NextSibling_004F9078, {y, b2s::kStr3DTextObject}));
        } while (y != 0);
        store<u32>(s + 200, n);
        const u32 bytes = n * 28u;
        const u32 p = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {bytes, 0, 16}));
        store<u32>(s + 196, static_cast<u32>(call(ctx, b2fn::memset, {p, 0, bytes})));
        for (u32 off = 0; c != 0; off += 28) {
            auto ent = [&]() { return load<u32>(s + 196) + off; };
            store<u32>(id, 0);
            if (static_cast<u32>(call(ctx, b2fn::XmlCursor_ReadHash, {c, kHashText, id, 0})) == 0) {
                const u32 e = ent();
                store<u32>(e + 4, load<u32>(id));
                call(ctx, b2fn::XmlCursor_ReadHash, {c, kHashTextAux, e + 8, load<u32>(e + 8)});
                if (ReadFrames(ctx, c, ent, 12)) {
                    call(ctx, b2fn::XmlCursor_ReadColor, {c, kHashTextColor, ent(), ent()});
                    ReadTextBool(ctx, c, ent, tb);
                }
            }
            c = static_cast<u32>(call(ctx, b2fn::Xml_NextSibling_004F9078, {c, b2s::kStr3DTextObject}));
        }
    }

    store<u32>(s + 152, 0);
    c = static_cast<u32>(call(ctx, b2fn::XmlCursor_FirstChild, {cur, b2s::kTagRecord}));
    while (c != 0) {
        store<u32>(s + 152, load<u32>(s + 152) + 1);
        c = static_cast<u32>(call(ctx, b2fn::XmlCursor_NextSibling, {c, b2s::kTagRecord}));
    }
    const u32 n = load<u32>(s + 152);
    if (static_cast<s32>(n) <= 0) {
        ret(ctx, s);
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {n * 56u, 0, 16}));
    if (p != 0)
        for (u32 k = 0; k < n; ++k) {
            const u32 r = p + 56 * k;
            for (u32 off = 0; off <= 20; off += 4) store<u32>(r + off, 0);
            store<u8>(r + 24, 0);
        }
    store<u32>(s + 148, p);
    cur = static_cast<u32>(call(ctx, b2fn::XmlCursor_FirstChild, {cur, b2s::kTagRecord}));
    static constexpr u32 kHashes[] = {0x4CE35A8Du, 0x2911AA63u, 0xFCF7E9DFu, 0xBC383E08u, 0x8DB5DF87u, 0xE4D43099u};
    for (u32 off = 0; cur != 0; off += 56) {
        for (u32 k = 0; k < 6; ++k) ReadHash(ctx, cur, kHashes[k], load<u32>(s + 148) + off + 4 * k);
        if (call(ctx, b2fn::XmlCursor_FindAttr, {cur, 0xF7BE5DF7u}) != 0) {
            const u32 dst = load<u32>(s + 148) + off + 24;
            const u32 a = static_cast<u32>(call(ctx, b2fn::XmlCursor_FindAttr, {cur, 0xF7BE5DF7u}));
            call(ctx, b2fn::String_Copy, {dst, a, 32});
        }
        cur = static_cast<u32>(call(ctx, b2fn::XmlCursor_NextSibling, {cur, b2s::kTagRecord}));
    }
    ret(ctx, s);
}

// SlotDef* SlotDefState_New_0035F8FC(x, XmlCursor* cur): a new 224-byte
// def (vtable 0x00B1F860: the colour +168 from its default, +204 / +208 =
// 1.0, +80 |= 0x200, +148..+164 / +212 cleared, +172..+203 memset),
// SlotDef_Read (on 0 when the allocation fails); then one 16-byte entry
// per "State" child (+148 the zeroed array, +152 the count): {+0 the hash
// 0x0024728B, +12 / +13 the index (0..2) of the hashes 0x04B42877 /
// 0x04B42878 in kStateHashes when found, +4 / +8 the state's 16-byte 3D
// text entries {+0 the id 0x934CAC5E, +4..+12 the frames} and their
// count}; then the corners +172..+203 as SlotDefC's (cleared, colours
// from their default, "TLPoint".."BRPoint"), the hash +164, the colour
// +168, the floats +204 / +208 and the hashes +212 / +156 / +160.
DECOMP(0035F8FC, SlotDefState_New_0035F8FC) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr(), id = tmp + 4;  // the original's sp + 112 / 116
    u32 s = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {224, 0, 16}));
    if (s != 0) {
        mid2::SlotDefBaseInit(ctx, s);
        store<u32>(s + 0, b2s::kVtSlotDefState);
        for (u32 off = 148; off <= 164; off += 4) store<u32>(s + off, 0);
        mid2::CopyColor(s + 168, mid2::kSlotColorA0);
        store<u32>(s + 204, mid2::kOneF);
        store<u32>(s + 212, 0);
        store<u32>(s + 208, mid2::kOneF);
        store<u32>(s + 80, load<u32>(s + 80) | 0x200u);
        call(ctx, b2fn::memset, {s + 172, 0, 32});
    }
    call(ctx, b2fn::SlotDef_Read, {s, x, cur});

    u32 c = static_cast<u32>(call(ctx, b2fn::Xml_FindChildren, {cur, b2s::kStrState}));
    if (c != 0) {
        u32 n = 0;
        do {
            ++n;
            c = static_cast<u32>(call(ctx, b2fn::Xml_NextSibling_004F9078, {c, b2s::kStrState}));
        } while (c != 0);
        store<u32>(s + 152, n);
        const u32 bytes = (n << 4) & 0xFFFFFFF0u;
        const u32 p = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {bytes, 0, 16}));
        store<u32>(s + 148, static_cast<u32>(call(ctx, b2fn::memset, {p, 0, bytes})));
        c = static_cast<u32>(call(ctx, b2fn::Xml_FindChildren, {cur, b2s::kStrState}));
    }
    for (u32 so = 0; c != 0; so += 16) {
        auto st = [&]() { return load<u32>(s + 148) + so; };
        {
            const u32 e = st();
            call(ctx, b2fn::XmlCursor_ReadHash, {c, 0x0024728Bu, e, load<u32>(e)});
        }
        static constexpr u32 kAttr[2] = {0x04B42877u, 0x04B42878u};
        for (u32 j = 0; j < 2; ++j) {
            store<u32>(tmp, 0);
            if (static_cast<u32>(call(ctx, b2fn::XmlCursor_ReadHash, {c, kAttr[j], tmp, 0})) != 0) continue;
            for (u32 i = 0; i < 3; ++i)
                if (load<u32>(b2s::kStateHashes + 4 * i) == load<u32>(tmp)) store<u8>(st() + 12 + j, static_cast<u8>(i));
        }
        u32 t = static_cast<u32>(call(ctx, b2fn::Xml_FindChildren, {c, b2s::kStr3DTextObject}));
        if (t != 0) {
            u32 n = 0;
            do {
                ++n;
                t = static_cast<u32>(call(ctx, b2fn::Xml_NextSibling_004F9078, {t, b2s::kStr3DTextObject}));
            } while (t != 0);
            store<u32>(st() + 8, n);
            const u32 bytes = (n << 4) & 0xFFFFFFF0u;
            const u32 p = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {bytes, 0, 16}));
            const u32 a = static_cast<u32>(call(ctx, b2fn::memset, {p, 0, bytes}));
            store<u32>(st() + 4, a);
            t = static_cast<u32>(call(ctx, b2fn::Xml_FindChildren, {c, b2s::kStr3DTextObject}));
            for (u32 off = 0; t != 0; off += 16) {
                auto ent = [&]() { return load<u32>(st() + 4) + off; };
                store<u32>(id, 0);
                if (static_cast<u32>(call(ctx, b2fn::XmlCursor_ReadHash, {t, kHashText, id, 0})) == 0) {
                    store<u32>(ent(), load<u32>(id));
                    ReadFrames(ctx, t, ent, 4);
                }
                t = static_cast<u32>(call(ctx, b2fn::Xml_NextSibling_004F9078, {t, b2s::kStr3DTextObject}));
            }
        }
        c = static_cast<u32>(call(ctx, b2fn::Xml_NextSibling_004F9078, {c, b2s::kStrState}));
    }

    call(ctx, b2fn::memset, {s + 172, 0, 32});
    for (u32 k = 0; k < 4; ++k) mid2::CopyColor(s + 176 + 8 * k, mid2::kSlotColorA0);
    static constexpr u32 kCorners[4] = {b2s::kStrTLPoint, b2s::kStrTRPoint, b2s::kStrBLPoint, b2s::kStrBRPoint};
    for (u32 k = 0; k < 4; ++k) {
        const u32 cc = static_cast<u32>(call(ctx, b2fn::Xml_FindChildren, {cur, kCorners[k]}));
        if (cc == 0) continue;
        ReadHash(ctx, cc, 0x0024728Bu, s + 172 + 8 * k);
        ReadColor(ctx, cc, 0x5399A8B2u, s + 176 + 8 * k);
    }
    ReadHash(ctx, cur, 0xDF617116u, s + 164);
    ReadColor(ctx, cur, 0x4C2E77FDu, s + 168);
    ReadF32(ctx, cur, 0xEA68DF12u, s + 204);
    ReadF32(ctx, cur, 0xC9037FF5u, s + 208);
    ReadHash(ctx, cur, 0x62AA4B26u, s + 212);
    ReadHash(ctx, cur, 0xC5268ED9u, s + 156);
    ReadHash(ctx, cur, 0x5AD97285u, s + 160);
    ret(ctx, s);
}

// SlotDef* SlotDefBool_New_00361234(x, XmlCursor* cur): a new 160-byte def
// (vtable 0x00B1F7B8: +145 cleared, +80 |= 0x20), SlotDef_Read (on 0 when
// the allocation fails), then the bool 0x280D85C8 into +145.
DECOMP(00361234, SlotDefBool_New_00361234) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    const u32 s = static_cast<u32>(call(ctx, b2fn::Mem_Alloc, {160, 0, 16}));
    if (s != 0) {
        mid2::SlotDefBaseInit(ctx, s);
        store<u8>(s + 145, 0);
        store<u32>(s + 0, b2s::kVtSlotDefBool);
        store<u32>(s + 80, load<u32>(s + 80) | 0x20u);
    }
    call(ctx, b2fn::SlotDef_Read, {s, x, cur});
    ReadBool(ctx, cur, 0x280D85C8u, s + 145);
    ret(ctx, s);
}
