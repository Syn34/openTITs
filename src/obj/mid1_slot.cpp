// Game code, medium sweep part 1: the UI slot definitions read from XML
// (the classes under vtable 0x00B1F668: a base read by SlotDef_Read and
// the derived defs the factories at 0x0036196C build).
//
// SlotDef: +0 vtable, +4 the name (32 chars), +36 the prefix (32 chars),
// +68 the name's hash, +72 the geometry instance, +76 a hash, +80 flags,
// +84 the kind (0-3, 5 = none), +96 / +128 two vec3s, +112 the slot
// priority (at most 4), +144 flags (bits 31 / 30 from XML).
#include "mid1.h"

using namespace sly4;
using namespace sly4::mid1;

namespace m1s {
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (cursor, name) -> const char* or 0
constexpr u32 XmlCursor_ReadBool = 0x004F986C;        // (cursor, hash, u8* out, bool default)
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;         // (cursor, hash, s32* out, s32 default)
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;         // (cursor, hash, f32* out; f1 default)
constexpr u32 XmlCursor_ReadVec3 = 0x004F9E0C;        // (cursor, hash, vec3* out, const vec3* default)
constexpr u32 XmlCursor_ReadColor = 0x004F9F9C;       // (cursor, hash, u8[4]* out, default)
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;        // (cursor, hash, u32* out, u32 default) -> 0 when found
constexpr u32 GeomInstance_FindByHash = 0x005801A0;   // (hash) -> instance or 0
constexpr u32 Hash_ToString = 0x0056A6E0;             // (hash) -> "0x%x"
constexpr u32 Ui_Find = 0x004D5544;                   // (path, parent) -> element
constexpr u32 SlotDef_Read = 0x0035C100;
constexpr u32 kStrName = 0x00ADC998;      // "name"
constexpr u32 kStrPrefix = 0x00ADC9A0;    // "prefix"
constexpr u32 kFmtPriority = 0x00ADC9B0;  // "exceeded max slot priority of %u.  forcing to max."
constexpr u32 kStrTitleText = 0x00ADCA74;      // "titleText2D"
constexpr u32 kStrMsgText = 0x00ADCA84;        // "msgText2D"
constexpr u32 kStrAcceptMsgText = 0x00ADCA94;  // "acceptMsgText2D"
constexpr u32 kSlotDefaultVec = 0x01164B40;    // vec3 (the zero vector)
constexpr u32 kVtSlotDef = 0x00B1F668;
constexpr u32 kVtSlotDefMsg = 0x00B1F7F0;
constexpr u32 kVtSlotDefB748 = 0x00B1F748;
constexpr u32 kVtSlotDefB780 = 0x00B1F780;
constexpr u32 kVtSlotDef3DText = 0x00B1F828;
constexpr u32 Xml_FindChildren = 0x004F936C;          // (cursor, name) -> the first child named so, or 0
constexpr u32 Xml_NextSibling_004F9078 = 0x004F9078;  // (cursor, name) -> the next sibling named so, or 0 (name?)
constexpr u32 memset = 0x00726E54;
constexpr u32 kStr3DTextObject = 0x00ADC9E4;  // "3DTextObject"
constexpr u32 kStrParentFrame = 0x00ADC9F8;   // "parentFrame2D"
constexpr u32 kStrTextFrame = 0x00ADCA08;     // "textFrame2D"
constexpr u32 kStrText = 0x00ADCA18;          // "text2D"
constexpr u32 String_Printf = 0x00551540;             // sprintf(dst, fmt, ...)
constexpr u32 XmlCursor_ReadHashByName = 0x004FA6E4;  // (cursor, const char* name, u32* out, u32 default)
constexpr u32 kFmtIcon = 0x00ADCAAC;          // "icon%d"
constexpr u32 kFmtSound = 0x00ADCAB4;         // "sound%d"
constexpr u32 kFmtAddBitScript = 0x00ADCABC;  // "addBitScript%d"
}  // namespace m1s

namespace {
void ReadHash(PPUContext& ctx, u32 cur, u32 hash, u32 p) { call(ctx, m1s::XmlCursor_ReadHash, {cur, hash, p, load<u32>(p)}); }
void ReadBool(PPUContext& ctx, u32 cur, u32 hash, u32 p) { call(ctx, m1s::XmlCursor_ReadBool, {cur, hash, p, load<u8>(p)}); }
void ReadF32(PPUContext& ctx, u32 cur, u32 hash, u32 p) {
    callf(ctx, m1s::XmlCursor_ReadF32, {cur, hash, p}, {std::bit_cast<f64>(LfsBits(load<u32>(p)))});
}
void ReadColor(PPUContext& ctx, u32 cur, u32 hash, u32 p) { call(ctx, m1s::XmlCursor_ReadColor, {cur, hash, p, p}); }

// The inlined SlotDef constructor (up to the derived class's vtable): the
// two vec3s +96 / +128 from kSlotDefaultVec, kind 5, the strings, hashes,
// +80 and +112 cleared, +144 bits 31 / 30 cleared, +148 = 0.
void SlotDefInit(u32 p) {
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 96 + 4 * k, load<u32>(m1s::kSlotDefaultVec + 4 * k));  // lfs / stfs
    store<u32>(p + 0, m1s::kVtSlotDef);
    store<u32>(p + 68, 0);
    store<u32>(p + 72, 0);
    store<u32>(p + 76, 0);
    store<u32>(p + 84, 5);
    const u32 f144 = load<u32>(p + 144);
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 128 + 4 * k, load<u32>(m1s::kSlotDefaultVec + 4 * k));
    store<u32>(p + 112, 0);
    store<u8>(p + 4, 0);
    store<u32>(p + 144, f144 & 0x3FFFFFFFu);
    store<u8>(p + 36, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(p + 80 + k, 0);
    store<u32>(p + 148, 0);
}
}  // namespace

// u32 SlotDef_Read(SlotDef* s, -, XmlCursor* cur): the "name" (and its
// hash +68) and "prefix" attributes; the geometry instance hash (0x2911AA63)
// looked up into +72 (with two flag bools 0xA489BE2A / 0x38C41ADC into +144
// bits 31 / 30 when found; Hash_ToString when not); the vec3s 0x2E6543D2
// (+128) / 0xEDF08AFF (+96), the hash 0xCF4A1DD4 (+76), the priority
// 0xA4CA6DA2 (+112, clamped to 4 with a warning), and the kind hash
// 0x0026E1FE mapped into +84 (0xA7E 1, 0xA78 0, 0x850 3, 0x84A 2, else 5).
// Returns the kind.
DECOMP(0035C100, SlotDef_Read) {
    const u32 s = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();  // the original's sp + 112 / 116 / 120
    u32 a = static_cast<u32>(call(ctx, m1s::XmlCursor_FindAttrByName, {cur, m1s::kStrName}));
    if (a != 0 && load<u8>(a) != 0) {
        call(ctx, m1fn::String_CopyN, {s + 4, a, 32});
        store<u32>(s + 68, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {s + 4})));
    }
    a = static_cast<u32>(call(ctx, m1s::XmlCursor_FindAttrByName, {cur, m1s::kStrPrefix}));
    if (a != 0 && load<u8>(a) != 0) call(ctx, m1fn::String_CopyN, {s + 36, a, 32});
    store<u32>(tmp + 0, 0);
    if (static_cast<u32>(call(ctx, m1s::XmlCursor_ReadHash, {cur, 0x2911AA63u, tmp + 0, 0})) == 0) {
        const u32 g = static_cast<u32>(call(ctx, m1s::GeomInstance_FindByHash, {load<u32>(tmp + 0)}));
        store<u32>(s + 72, g);
        if (g != 0) {
            store<u8>(tmp + 8, 0);
            call(ctx, m1s::XmlCursor_ReadBool, {cur, 0xA489BE2Au, tmp + 8, (load<u32>(s + 144) & 0x80000000u) ? 1u : 0u});
            u32 f = (load<u32>(s + 144) & 0x7FFFFFFFu) | ((load<u8>(tmp + 8) & 1u) << 31);
            store<u32>(s + 144, f);
            call(ctx, m1s::XmlCursor_ReadBool, {cur, 0x38C41ADCu, tmp + 8, (f & 0x40000000u) ? 1u : 0u});
            f = (load<u32>(s + 144) & ~0x40000000u) | ((load<u8>(tmp + 8) & 1u) << 30);
            store<u32>(s + 144, f);
        } else {
            call(ctx, m1s::Hash_ToString, {load<u32>(tmp + 0)});
        }
    }
    call(ctx, m1s::XmlCursor_ReadVec3, {cur, 0x2E6543D2u, s + 128, s + 128});
    call(ctx, m1s::XmlCursor_ReadHash, {cur, 0xCF4A1DD4u, s + 76, load<u32>(s + 76)});
    call(ctx, m1s::XmlCursor_ReadVec3, {cur, 0xEDF08AFFu, s + 96, s + 96});
    call(ctx, m1s::XmlCursor_ReadS32, {cur, 0xA4CA6DA2u, s + 112, Sx(load<u32>(s + 112))});
    if (static_cast<s32>(load<u32>(s + 112)) > 4) {
        call(ctx, m1fn::String_Format, {m1s::kFmtPriority, 4});
        store<u32>(s + 112, 4);
    }
    store<u32>(tmp + 4, 0);
    call(ctx, m1s::XmlCursor_ReadHash, {cur, 0x0026E1FEu, tmp + 4, 0});
    u32 kind;
    switch (static_cast<s32>(load<u32>(tmp + 4))) {
    case 0xA7E: kind = 1; break;
    case 0xA78: kind = 0; break;
    case 0x850: kind = 3; break;
    case 0x84A: kind = 2; break;
    default: kind = 5; break;
    }
    store<u32>(s + 84, kind);
    ret(ctx, kind);
}

// SlotDef* SlotDefB_Read_0035D708(SlotDefB* s, x, XmlCursor* cur):
// SlotDef_Read, then the floats +148 / +192 / +196, the bool +152, the
// hashes +156..+188 and the colours +200 / +204 / +208. Returns s.
DECOMP(0035D708, SlotDefB_Read_0035D708) {
    const u32 s = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 2);
    call(ctx, m1s::SlotDef_Read, {s, ctx.gpr[4], cur});
    ReadF32(ctx, cur, 0xE690CE6Fu, s + 148);
    ReadBool(ctx, cur, 0x280D85C8u, s + 152);
    static constexpr u32 kHashes[] = {0x5CCAD404u, 0x57FBAF97u, 0x1395597Cu, 0x4853B1E7u, 0x2D45A764u, 0x66DDB9C0u, 0x2D6BFE05u, 0x429F935Au, 0x08B994E1u};
    for (u32 k = 0; k < 9; ++k) ReadHash(ctx, cur, kHashes[k], s + 156 + 4 * k);
    ReadF32(ctx, cur, 0x81B883B4u, s + 192);
    ReadF32(ctx, cur, 0xEDF0A684u, s + 196);
    ReadColor(ctx, cur, 0x894AC0DBu, s + 200);
    ReadColor(ctx, cur, 0xAFE4553Du, s + 204);
    ReadColor(ctx, cur, 0x62E42FD6u, s + 208);
    ret(ctx, s);
}

// SlotDef* SlotDefMsg_New_003602D4(x, XmlCursor* cur): a new 176-byte def
// (vtable 0x00B1F7F0: +80 |= 0x40, +148..+168 cleared), SlotDef_Read (on 0
// when the allocation fails, as the original), the hashes 0xD6A44C96 /
// 0x85BE0AFF / 0xA23D0AF7 into +160 / +164 / +168, and the UI elements
// "titleText2D" / "msgText2D" / "acceptMsgText2D" into +148 / +152 / +156.
DECOMP(003602D4, SlotDefMsg_New_003602D4) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    u32 s = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {176, 0, 16}));
    if (s != 0) {
        SlotDefInit(s);
        const u32 f80 = load<u32>(s + 80);
        store<u32>(s + 0, m1s::kVtSlotDefMsg);
        for (u32 off = 152; off <= 168; off += 4) store<u32>(s + off, 0);
        store<u32>(s + 80, f80 | 0x40u);
        ctx.set_fpr_bits(1, LfsBits(load<u32>(m1s::kSlotDefaultVec + 8)));  // f1 = the last lfs
    }
    call(ctx, m1s::SlotDef_Read, {s, x, cur});
    ReadHash(ctx, cur, 0xD6A44C96u, s + 160);
    ReadHash(ctx, cur, 0x85BE0AFFu, s + 164);
    ReadHash(ctx, cur, 0xA23D0AF7u, s + 168);
    static constexpr u32 kNames[] = {m1s::kStrTitleText, m1s::kStrMsgText, m1s::kStrAcceptMsgText};
    for (u32 k = 0; k < 3; ++k) {
        const u32 path = static_cast<u32>(call(ctx, m1s::XmlCursor_FindAttrByName, {cur, kNames[k]}));
        store<u32>(s + 148 + 4 * k, static_cast<u32>(call(ctx, m1s::Ui_Find, {path, 0})));
    }
    ret(ctx, s);
}

// SlotDef* SlotDefB748_New_003616A4(x, XmlCursor* cur): a new 208-byte def
// (vtable 0x00B1F748: bools +152 / +168 (1) / +180 / +192 (1), hashes
// +156..+164 / +172 / +176, floats +184 / +188, +80 |= 0x8), SlotDef_Read
// (on 0 when the allocation fails), then each field read from XML.
DECOMP(003616A4, SlotDefB748_New_003616A4) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    u32 s = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {208, 0, 16}));
    if (s != 0) {
        SlotDefInit(s);
        store<u32>(s + 0, m1s::kVtSlotDefB748);
        store<u8>(s + 152, 0);
        store<u32>(s + 156, 0);
        store<u32>(s + 160, 0);
        store<u32>(s + 164, 0);
        const u32 f80 = load<u32>(s + 80);
        store<u32>(s + 172, 0);
        store<u8>(s + 168, 1);
        store<u32>(s + 176, 0);
        store<u8>(s + 180, 0);
        store<u32>(s + 184, 0);
        store<u32>(s + 188, 0);
        store<u8>(s + 36, 0);
        store<u8>(s + 192, 1);
        store<u32>(s + 80, f80 | 0x8u);
    }
    call(ctx, m1s::SlotDef_Read, {s, x, cur});
    ReadBool(ctx, cur, 0xA9CD4232u, s + 152);
    ReadHash(ctx, cur, 0xB741B705u, s + 156);
    ReadHash(ctx, cur, 0xEDB0120Au, s + 160);
    ReadHash(ctx, cur, 0x41F6E0FEu, s + 164);
    ReadBool(ctx, cur, 0x12B5C5ADu, s + 168);
    ReadHash(ctx, cur, 0x58F3971Cu, s + 172);
    ReadHash(ctx, cur, 0xEA6A4861u, s + 176);
    ReadBool(ctx, cur, 0xB462EDB6u, s + 180);
    ReadF32(ctx, cur, 0x72C08209u, s + 184);
    ReadF32(ctx, cur, 0x08EC14A8u, s + 188);
    ReadBool(ctx, cur, 0x46C1AC39u, s + 192);
    ret(ctx, s);
}

// SlotDef* SlotDefB780_New_00361374(x, XmlCursor* cur): a new 304-byte def
// (vtable 0x00B1F780: +80 |= 0x10, +152 / +156 / +240 / +292 / +296
// cleared), SlotDef_Read (on 0 when the allocation fails), the hashes +152
// / +156 / +240 / +244 / +248, the floats +292 / +296, and for i = 0..9 the
// hashes "icon%d" (+160[i]), "sound%d" (+200[i]) and "addBitScript%d"
// (+252[i]) read by name (default 0).
DECOMP(00361374, SlotDefB780_New_00361374) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    u32 s = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {304, 0, 16}));
    if (s != 0) {
        SlotDefInit(s);
        const u32 f80 = load<u32>(s + 80);
        store<u32>(s + 0, m1s::kVtSlotDefB780);
        for (const u32 off : {152u, 156u, 240u, 292u, 296u}) store<u32>(s + off, 0);
        store<u8>(s + 36, 0);
        store<u32>(s + 80, f80 | 0x10u);
    }
    call(ctx, m1s::SlotDef_Read, {s, x, cur});
    ReadHash(ctx, cur, 0x623DA031u, s + 152);
    ReadHash(ctx, cur, 0xC9566036u, s + 156);
    ReadHash(ctx, cur, 0x00273D2Du, s + 240);
    ReadHash(ctx, cur, 0x577E6D38u, s + 244);
    ReadHash(ctx, cur, 0x59E72E8Au, s + 248);
    ReadF32(ctx, cur, 0x72C08209u, s + 292);
    ReadF32(ctx, cur, 0x08EC14A8u, s + 296);
    Stack frame(ctx, 112);
    const u32 buf = frame.addr();  // the original's sp + 112
    for (u32 i = 0; i < 10; ++i) {
        call(ctx, m1s::String_Printf, {buf, m1s::kFmtIcon, i});
        call(ctx, m1s::XmlCursor_ReadHashByName, {cur, buf, s + 160 + 4 * i, 0});
        call(ctx, m1s::String_Printf, {buf, m1s::kFmtSound, i});
        call(ctx, m1s::XmlCursor_ReadHashByName, {cur, buf, s + 200 + 4 * i, 0});
        call(ctx, m1s::String_Printf, {buf, m1s::kFmtAddBitScript, i});
        call(ctx, m1s::XmlCursor_ReadHashByName, {cur, buf, s + 252 + 4 * i, 0});
    }
    ret(ctx, s);
}

// SlotDef* SlotDef3DText_New_003604E0(x, XmlCursor* cur): a new 160-byte
// def (vtable 0x00B1F828: +80 |= 0x4, +148 / +152 / +156 cleared),
// SlotDef_Read (on 0 when the allocation fails), then one 16-byte entry
// per "3DTextObject" child (+152 the count, +148 the zeroed array): {+0 the
// hash 0x934CAC5E, +4 the "parentFrame2D" element, +8 its "textFrame2D"
// child, +12 the "text2D" element under it (or under +4) when its type
// (vfn[3]) is 3 or 5}; children without the hash only advance the entry.
// Finally the hash 0x99C55183 into +156.
DECOMP(003604E0, SlotDef3DText_New_003604E0) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    u32 s = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {160, 0, 16}));
    if (s != 0) {
        SlotDefInit(s);
        const u32 f80 = load<u32>(s + 80);
        store<u32>(s + 0, m1s::kVtSlotDef3DText);
        store<u32>(s + 152, 0);
        store<u32>(s + 156, 0);
        store<u32>(s + 80, f80 | 0x4u);
        ctx.set_fpr_bits(1, LfsBits(load<u32>(m1s::kSlotDefaultVec + 8)));  // f1 = the last lfs
    }
    call(ctx, m1s::SlotDef_Read, {s, x, cur});
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();  // the original's sp + 112
    u32 c = static_cast<u32>(call(ctx, m1s::Xml_FindChildren, {cur, m1s::kStr3DTextObject}));
    if (c != 0) {
        u32 n = 0;
        for (;;) {
            ++n;
            const u32 nx = static_cast<u32>(call(ctx, m1s::Xml_NextSibling_004F9078, {c, m1s::kStr3DTextObject}));
            if (nx == 0) break;
            ++n;
            c = static_cast<u32>(call(ctx, m1s::Xml_NextSibling_004F9078, {nx, m1s::kStr3DTextObject}));
            if (c == 0) break;
        }
        store<u32>(s + 152, n);
        if (n != 0) {
            const u32 bytes = (n << 4) & 0xFFFFFFF0u;
            const u32 p = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {bytes, 0, 16}));
            store<u32>(s + 148, static_cast<u32>(call(ctx, m1s::memset, {p, 0, bytes})));
            c = static_cast<u32>(call(ctx, m1s::Xml_FindChildren, {cur, m1s::kStr3DTextObject}));
            for (u32 off = 0; c != 0; off += 16) {
                store<u32>(tmp, 0);
                if (static_cast<u32>(call(ctx, m1s::XmlCursor_ReadHash, {c, 0x934CAC5Eu, tmp, 0})) == 0) {
                    store<u32>(load<u32>(s + 148) + off, load<u32>(tmp));
                    u32 path = static_cast<u32>(call(ctx, m1s::XmlCursor_FindAttrByName, {c, m1s::kStrParentFrame}));
                    const u32 e = static_cast<u32>(call(ctx, m1s::Ui_Find, {path, 0}));
                    store<u32>(load<u32>(s + 148) + off + 4, e);
                    if (e != 0) {
                        path = static_cast<u32>(call(ctx, m1s::XmlCursor_FindAttrByName, {c, m1s::kStrTextFrame}));
                        const u32 e2 = static_cast<u32>(call(ctx, m1s::Ui_Find, {path, load<u32>(load<u32>(s + 148) + off + 4)}));
                        store<u32>(load<u32>(s + 148) + off + 8, e2);
                        u32 parent = load<u32>(load<u32>(s + 148) + off + 8);
                        if (parent == 0) parent = load<u32>(load<u32>(s + 148) + off + 4);
                        path = static_cast<u32>(call(ctx, m1s::XmlCursor_FindAttrByName, {c, m1s::kStrText}));
                        const u32 t = static_cast<u32>(call(ctx, m1s::Ui_Find, {path, parent}));
                        if (t != 0) {
                            bool ok = static_cast<u32>(VCall(ctx, t, 12, {t})) == 3;
                            if (!ok) ok = static_cast<u32>(VCall(ctx, t, 12, {t})) == 5;
                            if (ok) store<u32>(load<u32>(s + 148) + off + 12, t);
                        }
                    }
                }
                c = static_cast<u32>(call(ctx, m1s::Xml_NextSibling_004F9078, {c, m1s::kStr3DTextObject}));
            }
        }
    }
    ReadHash(ctx, cur, 0x99C55183u, s + 156);
    ret(ctx, s);
}
