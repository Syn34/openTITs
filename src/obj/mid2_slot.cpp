// Game code, medium sweep part 2: more of the UI slot definitions read
// from XML (siblings of mid1_slot.cpp, built by SlotDef_Factory_0036196C).
// SlotDef layout: see mid1_slot.cpp.
#include "mid2.h"

using namespace sly4;
using namespace sly4::mid2;

namespace m2s {
constexpr u32 kVtSlotDefB = 0x00B1F9B8;     // SlotDefB_Read_0035D708's class
constexpr u32 kVtSlotDefC = 0x00B1F9F8;     // SlotDefC_Read_0035CA00's class
constexpr u32 kVtSlotDefD = 0x00B1F978;     // SlotDefList + the "0x8BB559E6" records
constexpr u32 kVtSlotDefE = 0x00B1F70C;     // the 3D-text entries with a hash
constexpr u32 kChildTag = 0x8BB559E6;       // SlotDefD's record elements
}  // namespace m2s

namespace {

// The 3D-text entries of SlotDefList / SlotDefE: up to 40 32-byte entries
// built on the stack (zeroed, +20 = 0xD8A5F8E7, +24 = 0x3C193BAC), copied
// to the heap at the end. Stack layout as the original's: the entries at
// sp + 112, the hash at sp + 1392 and the bool at sp + 1396 right after
// them (a 41st child overruns into them, unchecked as in the original).
constexpr u32 kMaxEntries = 40;
constexpr u32 kEntryBytes = 32;

void InitEntries(u32 buf) {
    for (u32 k = 0; k < kMaxEntries; ++k) {
        const u32 e = buf + kEntryBytes * k;
        for (u32 j = 0; j < kEntryBytes; j += 4) store<u32>(e + j, 0);  // stvx zeros
        store<u32>(e + 20, 0xD8A5F8E7u);
        store<u32>(e + 24, 0x3C193BACu);
    }
}

// One "3DTextObject" child c into entry e (s->+148 + off, re-read after
// every call as the original): +4 the hash already read, +8 the
// "parentFrame2D" element; when found, +12 its "textFrame2D" child and
// +16 the "text2D" element under +12 (or +8) when its type (vfn[3]) is 3
// or 5. reread12: SlotDefE reads +12 back from memory, SlotDefList keeps
// the call's result. Returns false when there is no parent frame (the
// rest of the entry is then skipped).
bool ReadTextFrames(PPUContext& ctx, u32 s, u32 off, u32 c, bool reread12) {
    auto ent = [&]() { return load<u32>(s + 148) + off; };
    u32 path = static_cast<u32>(call(ctx, m2fn::XmlCursor_FindAttrByName, {c, kStrParentFrame}));
    const u32 pf = static_cast<u32>(call(ctx, m2fn::Ui_Find, {path, 0}));
    store<u32>(ent() + 8, pf);
    if (pf == 0) return false;
    path = static_cast<u32>(call(ctx, m2fn::XmlCursor_FindAttrByName, {c, kStrTextFrame}));
    u32 tf = static_cast<u32>(call(ctx, m2fn::Ui_Find, {path, load<u32>(ent() + 8)}));
    store<u32>(ent() + 12, tf);
    if (reread12) tf = load<u32>(ent() + 12);
    const u32 parent = tf != 0 ? tf : load<u32>(ent() + 8);
    path = static_cast<u32>(call(ctx, m2fn::XmlCursor_FindAttrByName, {c, kStrText}));
    const u32 t = static_cast<u32>(call(ctx, m2fn::Ui_Find, {path, parent}));
    if (t == 0) return true;
    bool ok = static_cast<u32>(VCall(ctx, t, 12, {t})) == 3;
    if (!ok) ok = static_cast<u32>(VCall(ctx, t, 12, {t})) == 5;
    if (ok) store<u32>(ent() + 16, t);
    return true;
}

// The shared loop: for each "3DTextObject" child, s->+148 = the stack
// buffer, +152 += 1, and when the child has the hash 0x934CAC5E the entry
// at off is filled (fill(c, off)); every child advances the entry. Then
// the entries are copied to a new heap array (+148) when there are any.
template <typename Fill>
void ReadEntries(PPUContext& ctx, u32 s, u32 cur, u32 buf, Fill fill) {
    const u32 tmp = buf + kMaxEntries * kEntryBytes;  // sp + 1392
    store<u32>(s + 152, 0);
    store<u32>(s + 148, 0);
    u32 c = static_cast<u32>(call(ctx, m2fn::Xml_FindChildren, {cur, kStr3DTextObject}));
    for (u32 off = 0; c != 0; off += kEntryBytes) {
        const u32 n = load<u32>(s + 152);
        store<u32>(s + 148, buf);  // a stack address, as the original
        store<u32>(tmp, 0);
        store<u32>(s + 152, n + 1);
        if (static_cast<u32>(call(ctx, m2fn::XmlCursor_ReadHash, {c, 0x934CAC5Eu, tmp, 0})) == 0) {
            store<u32>(load<u32>(s + 148) + off + 4, load<u32>(tmp));
            fill(c, off);
        }
        c = static_cast<u32>(call(ctx, m2fn::Xml_NextSibling_004F9078, {c, kStr3DTextObject}));
    }
    if (load<u32>(s + 152) == 0) return;
    const u32 p = static_cast<u32>(call(ctx, m2fn::Mem_Alloc, {(load<u32>(s + 152) << 5) & 0xFFFFFFE0u, 0, 16}));
    const u32 a = static_cast<u32>(call(ctx, m2fn::memset, {p, 0, (load<u32>(s + 152) << 5) & 0xFFFFFFE0u}));
    store<u32>(s + 148, a);
    call(ctx, m2fn::memmove, {a, buf, (load<u32>(s + 152) << 5) & 0xFFFFFFE0u});
}

void ReadHashAt(PPUContext& ctx, u32 c, u32 hash, u32 p) {
    call(ctx, m2fn::XmlCursor_ReadHash, {c, hash, p, load<u32>(p)});
}

}  // namespace

// SlotDefB* SlotDefB_New_0035D958(x, XmlCursor* cur): a new 224-byte def
// (vtable 0x00B1F9B8: +148 = -1.0, +152..+196 cleared, the colours +200 /
// +204 / +208 from their defaults, +80 |= 0x8000), then
// SlotDefB_Read_0035D708 (on 0 when the allocation fails, as the
// original); returns its result.
DECOMP(0035D958, SlotDefB_New_0035D958) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    u32 s = static_cast<u32>(call(ctx, m2fn::Mem_Alloc, {224, 0, 16}));
    if (s != 0) {
        SlotDefBaseInit(ctx, s);
        store<u32>(s + 0, m2s::kVtSlotDefB);
        store<u32>(s + 148, kMinusOneF);
        store<u8>(s + 152, 0);
        for (u32 off = 156; off <= 180; off += 4) store<u32>(s + off, 0);
        store<u32>(s + 192, 0);
        store<u32>(s + 196, 0);
        CopyColor(s + 200, kSlotColorAC);
        CopyColor(s + 204, kSlotColorB0);
        CopyColor(s + 208, kSlotColorC4);
        store<u32>(s + 80, load<u32>(s + 80) | 0x8000u);
    }
    ret(ctx, call(ctx, m2fn::SlotDefB_Read_0035D708, {s, x, cur}));
}

// SlotDefC* SlotDefC_New_0035D4B0(x, XmlCursor* cur): a new 448-byte def
// (vtable 0x00B1F9F8: +156 = -1.0, +412 / +416 = 1.0, the colours +240 /
// +424 / +428 / +432 from their defaults, +80 |= 0x10000, +168..+199
// memset, the other fields cleared), then SlotDefC_Read_0035CA00 (on 0
// when the allocation fails); returns its result.
DECOMP(0035D4B0, SlotDefC_New_0035D4B0) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    u32 s = static_cast<u32>(call(ctx, m2fn::Mem_Alloc, {448, 0, 16}));
    if (s != 0) {
        SlotDefBaseInit(ctx, s);
        store<u32>(s + 0, m2s::kVtSlotDefC);
        store<u32>(s + 148, 0);
        store<u32>(s + 152, 0);
        store<u32>(s + 156, kMinusOneF);
        store<u8>(s + 160, 0);
        store<u32>(s + 164, 0);
        store<u32>(s + 200, 0);
        store<u32>(s + 204, 0);
        CopyColor(s + 240, kSlotColorA0);
        for (u32 off = 244; off <= 256; off += 4) store<u32>(s + off, 0);
        for (u32 off = 388; off <= 400; off += 4) store<u32>(s + off, 0);
        store<u32>(s + 412, kOneF);
        store<u32>(s + 416, kOneF);
        store<u32>(s + 420, 0);
        CopyColor(s + 424, kSlotColorAC);
        CopyColor(s + 428, kSlotColorB0);
        CopyColor(s + 432, kSlotColorC4);
        store<u32>(s + 80, load<u32>(s + 80) | 0x10000u);
        call(ctx, m2fn::memset, {s + 168, 0, 32});
        store<u8>(s + 208, 0);
        store<u8>(s + 260, 0);
        store<u8>(s + 356, 0);
    }
    ret(ctx, call(ctx, m2fn::SlotDefC_Read_0035CA00, {s, x, cur}));
}

// SlotDefList* SlotDefList_Read_0035C3E4(SlotDefList* s, x, XmlCursor* cur):
// SlotDef_Read, then one 32-byte entry per "3DTextObject" child (+152 the
// count, +148 the array): {+0 the colour 0x0F1B2B8E, +4 the hash
// 0x934CAC5E, +8 / +12 / +16 the frames (ReadTextFrames), +20 / +24 the
// hashes 0xB4B14A17 / 0xE00F9276, +28 bits 31 / 30 the bools 0x6F82CABE /
// 0xBE95A9DF}; then the float 0xE690CE6F (+156) and the bool 0x280D85C8
// (+160). Returns s.
DECOMP(0035C3E4, SlotDefList_Read_0035C3E4) {
    const u32 s = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), cur = arg<u32>(ctx, 2);
    Stack frame(ctx, kMaxEntries * kEntryBytes + 16);
    const u32 buf = frame.addr();  // the original's sp + 112
    const u32 tb = buf + kMaxEntries * kEntryBytes + 4;  // sp + 1396
    InitEntries(buf);
    call(ctx, m2fn::SlotDef_Read, {s, x, cur});
    ReadEntries(ctx, s, cur, buf, [&](u32 c, u32 off) {
        auto ent = [&]() { return load<u32>(s + 148) + off; };
        if (!ReadTextFrames(ctx, s, off, c, false)) return;
        call(ctx, m2fn::XmlCursor_ReadColor, {c, 0x0F1B2B8Eu, ent(), ent()});
        ReadHashAt(ctx, c, 0xB4B14A17u, ent() + 20);
        ReadHashAt(ctx, c, 0xE00F9276u, ent() + 24);
        store<u8>(tb, 0);
        call(ctx, m2fn::XmlCursor_ReadBool, {c, 0x6F82CABEu, tb, load<u32>(ent() + 28) >> 31});
        u32 w = load<u32>(ent() + 28);
        store<u32>(ent() + 28, (w & 0x7FFFFFFFu) | ((load<u8>(tb) & 1u) << 31));
        call(ctx, m2fn::XmlCursor_ReadBool, {c, 0xBE95A9DFu, tb, (load<u32>(ent() + 28) >> 30) & 1u});
        w = load<u32>(ent() + 28);
        store<u32>(ent() + 28, (w & ~0x40000000u) | ((load<u8>(tb) & 1u) << 30));
    });
    callf(ctx, m2fn::XmlCursor_ReadF32, {cur, 0xE690CE6Fu, s + 156}, {std::bit_cast<f64>(LfsBits(load<u32>(s + 156)))});
    call(ctx, m2fn::XmlCursor_ReadBool, {cur, 0x280D85C8u, s + 160, load<u8>(s + 160)});
    ret(ctx, s);
}

// SlotDefE* SlotDefE_New_003608E8(x, XmlCursor* cur): a new 160-byte def
// (vtable 0x00B1F70C: +80 |= 0x2, +148 / +152 / +156 cleared),
// SlotDef_Read (on 0 when the allocation fails), then the 32-byte entries
// as SlotDefList_Read_0035C3E4 but with +20 / +24 / +28 the hashes
// 0xB4B14A17 / 0xE00F9276 / 0x91B; finally the bool 0x2EC1584C (+156).
DECOMP(003608E8, SlotDefE_New_003608E8) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    u32 s = static_cast<u32>(call(ctx, m2fn::Mem_Alloc, {160, 0, 16}));
    if (s != 0) {
        SlotDefBaseInit(ctx, s);
        store<u32>(s + 148, 0);
        store<u32>(s + 0, m2s::kVtSlotDefE);
        store<u32>(s + 152, 0);
        store<u8>(s + 156, 0);
        store<u32>(s + 80, load<u32>(s + 80) | 0x2u);
    }
    call(ctx, m2fn::SlotDef_Read, {s, x, cur});
    Stack frame(ctx, kMaxEntries * kEntryBytes + 16);
    const u32 buf = frame.addr();  // the original's sp + 112
    InitEntries(buf);
    ReadEntries(ctx, s, cur, buf, [&](u32 c, u32 off) {
        auto ent = [&]() { return load<u32>(s + 148) + off; };
        if (!ReadTextFrames(ctx, s, off, c, true)) return;
        call(ctx, m2fn::XmlCursor_ReadColor, {c, 0x0F1B2B8Eu, ent(), ent()});
        ReadHashAt(ctx, c, 0xB4B14A17u, ent() + 20);
        ReadHashAt(ctx, c, 0xE00F9276u, ent() + 24);
        ReadHashAt(ctx, c, 0x0000091Bu, ent() + 28);
    });
    call(ctx, m2fn::XmlCursor_ReadBool, {cur, 0x2EC1584Cu, s + 156, load<u8>(s + 156)});
    ret(ctx, s);
}

// SlotDefD* SlotDefD_New_00360DE8(x, XmlCursor* cur): a new 176-byte def
// (vtable 0x00B1F978, a SlotDefList: +156 = -1.0, +80 |= 0x4001, the rest
// cleared), SlotDefList_Read_0035C3E4 (on 0 when the allocation fails),
// then one 40-byte record per 0x8BB559E6 child (+168 the count, +164 the
// zeroed array; both written through a null def too, as the original):
// {+0 s32 0x042930B2 (default 0), +4 f32 0x00274ACD (default 0.0), +8 the
// string 0x00273D2D (31 chars)}; finally the float 0xBF654E61 (+172,
// default 0.0). Returns the def.
DECOMP(00360DE8, SlotDefD_New_00360DE8) {
    const u32 x = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1);
    u32 s = static_cast<u32>(call(ctx, m2fn::Mem_Alloc, {176, 0, 16}));
    if (s != 0) {
        SlotDefBaseInit(ctx, s);
        store<u32>(s + 148, 0);
        store<u32>(s + 152, 0);
        store<u8>(s + 160, 0);
        store<u32>(s + 156, kMinusOneF);
        store<u32>(s + 164, 0);
        store<u32>(s + 0, m2s::kVtSlotDefD);
        store<u32>(s + 168, 0);
        store<u32>(s + 80, load<u32>(s + 80) | 0x4001u);
    }
    call(ctx, m2fn::SlotDefList_Read_0035C3E4, {s, x, cur});
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();  // the original's sp + 112
    u32 c = static_cast<u32>(call(ctx, m2fn::XmlCursor_FirstChild, {cur, m2s::kChildTag}));
    if (c != 0) {
        u32 n = 0;
        for (u32 it = c;;) {
            ++n;
            it = static_cast<u32>(call(ctx, m2fn::XmlCursor_NextSibling, {it, m2s::kChildTag}));
            if (it == 0) break;
            ++n;
            it = static_cast<u32>(call(ctx, m2fn::XmlCursor_NextSibling, {it, m2s::kChildTag}));
            if (it == 0) break;
        }
        store<u32>(s + 168, n);
        if (n != 0) {
            const u32 bytes = n * 40;
            const u32 p = static_cast<u32>(call(ctx, m2fn::Mem_Alloc, {bytes, 0, 16}));
            store<u32>(s + 164, static_cast<u32>(call(ctx, m2fn::memset, {p, 0, bytes})));
            for (u32 off = 0; c != 0; off += 40) {
                const u32 r = load<u32>(s + 164) + off;
                store<u32>(tmp, 0);
                call(ctx, m2fn::XmlCursor_ReadString_004FA2AC, {c, 0x00273D2Du, tmp, kStrEmpty});
                const u32 str = load<u32>(tmp);
                if (str != 0 && load<u8>(str) != 0) call(ctx, m2fn::String_Copy, {r + 8, str, 31});
                call(ctx, m2fn::XmlCursor_ReadS32, {c, 0x042930B2u, r, 0});
                callf(ctx, m2fn::XmlCursor_ReadF32, {c, 0x00274ACDu, r + 4}, {0.0});
                c = static_cast<u32>(call(ctx, m2fn::XmlCursor_NextSibling, {c, m2s::kChildTag}));
            }
        }
    }
    callf(ctx, m2fn::XmlCursor_ReadF32, {cur, 0xBF654E61u, s + 172}, {0.0});
    ret(ctx, s);
}
