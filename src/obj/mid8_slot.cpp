// Game code, medium sweep part 8: factories of the Obj_Construct_00769BD8
// classes (2632..2656-byte objects whose extra fields at +2580 are read
// from XML into a stack record first).
#include "mid8.h"

using namespace sly4;
using namespace sly4::mid8;

namespace {

constexpr u32 Obj_Construct_00769BD8 = 0x00769BD8;  // (o, def)

// A field read into the record: 'H' hash, 'S' s32 (default def), 'F' f32
// (default the f32 at the factory's constant).
struct RecField {
    char kind;
    u32 hash, off, def;
};

// The shared body: rec (nwords words, the original's r1 + 112) gets the
// init words, then the fields; p = SmallAlloc(size) (0 -> return 0),
// Obj_Construct_00769BD8(p, x), +2572 = 0, bits 31 / 30 of +2576 cleared
// and the vtable stored (before +2576 when vtFirst), then the record is
// copied to +2580 word by word. Returns p.
u32 SlotObjNew(PPUContext& ctx, u32 c, u32 x, u32 rec, u32 nwords, std::initializer_list<std::pair<u32, u32>> init,
               std::initializer_list<RecField> fields, u32 fdef, u32 size, u32 vt, bool vtFirst) {
    for (const auto& [off, v] : init) store<u32>(rec + off, v);
    for (const RecField& f : fields) {
        switch (f.kind) {
        case 'H': XRead(ctx, xfn::XmlCursor_ReadHash, c, f.hash, rec + f.off, f.def); break;
        case 'S': XRead(ctx, xfn::XmlCursor_ReadS32, c, f.hash, rec + f.off, f.def); break;
        default: XF32Def(ctx, c, f.hash, rec + f.off, fdef); break;
        }
    }
    const u32 p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {size}));
    if (p == 0) return 0;
    call(ctx, Obj_Construct_00769BD8, {p, x});
    const u32 w = load<u32>(p + 2576) & 0x3FFFFFFFu;
    store<u32>(p + 2572, 0);
    if (vtFirst) store<u32>(p, vt);
    store<u32>(p + 2576, w);
    if (!vtFirst) store<u32>(p, vt);
    for (u32 k = 0; k < nwords; ++k) store<u32>(p + 2580 + 4 * k, load<u32>(rec + 4 * k));
    return p;
}

}  // namespace

// Obj* SlotObj_New335858(?, XmlCursor* c, const Def* d) (reads r4 / r5):
// 2632 bytes, vtable 0x00B1EC68, 13 record words: hashes 0xA2A0911A /
// 0xAE38532C / 0xBB68E539 / 0x760CB725 (+0..+12), f32 0x000110AD /
// 0x79792193 / 0x33BEF09B / 0xFEC0361E (+16..+28, default the f32 at
// 0x00335854), s32 0x543CAC2D / 0xF2813FD9 (+32 / +36, 0), 0x1B6E6190
// (+44, 3), 0x89EBE64D (+40, 0); +48 0.
DECOMP(00335858, SlotObj_New335858) {
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    Stack frame(ctx, 64);
    const u32 rec = frame.addr();
    ret(ctx, SlotObjNew(ctx, c, x, rec, 13,
                        {{0, 0}, {4, 0}, {8, 0}, {12, 0}, {16, 0}, {20, 0}, {24, 0}, {28, 0x40A00000}, {32, 0}, {36, 0}, {40, 0}, {44, 3}, {48, 0}},
                        {{'H', 0xA2A0911A, 0, 0},
                         {'H', 0xAE38532C, 4, 0},
                         {'H', 0xBB68E539, 8, 0},
                         {'H', 0x760CB725, 12, 0},
                         {'F', 0x000110AD, 16, 0},
                         {'F', 0x79792193, 20, 0},
                         {'F', 0x33BEF09B, 24, 0},
                         {'F', 0xFEC0361E, 28, 0},
                         {'S', 0x543CAC2D, 32, 0},
                         {'S', 0xF2813FD9, 36, 0},
                         {'S', 0x1B6E6190, 44, 3},
                         {'S', 0x89EBE64D, 40, 0}},
                        0x00335854, 2632, 0x00B1EC68, true));
}

// Obj* SlotObj_New335350(?, XmlCursor* c, const Def* d) (reads r4 / r5):
// 2656 bytes, vtable 0x00B1EBE8, 19 record words: hashes 0xA2A0911A,
// 0x441D9642..45, 0x5BCAE834..37, 0x760CB725 (+0..+36), f32 0x000110AD /
// 0x79792193 / 0x33BEF09B (+40..+48, default the f32 at 0x0033534C), s32
// 0x543CAC2D / 0x0C605253 / 0xF2813FD9 (+56..+64, 0) and 0x1B6E6190 (+68,
// 3).
// Quirk: record words +52 and +72 are copied from stack words the original
// never wrote.
DECOMP(00335350, SlotObj_New335350) {
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    Stack frame(ctx, 80);
    const u32 rec = frame.addr();
    ret(ctx, SlotObjNew(ctx, c, x, rec, 19,
                        {{0, 0}, {36, 0}, {40, 0}, {44, 0}, {48, 0}, {56, 0}, {60, 0}, {64, 0}, {68, 3},
                         {4, 0}, {8, 0}, {12, 0}, {16, 0}, {20, 0}, {24, 0}, {28, 0}, {32, 0}},
                        {{'H', 0xA2A0911A, 0, 0},
                         {'H', 0x441D9642, 4, 0},
                         {'H', 0x441D9643, 8, 0},
                         {'H', 0x441D9644, 12, 0},
                         {'H', 0x441D9645, 16, 0},
                         {'H', 0x5BCAE834, 20, 0},
                         {'H', 0x5BCAE835, 24, 0},
                         {'H', 0x5BCAE836, 28, 0},
                         {'H', 0x5BCAE837, 32, 0},
                         {'H', 0x760CB725, 36, 0},
                         {'F', 0x000110AD, 40, 0},
                         {'F', 0x79792193, 44, 0},
                         {'F', 0x33BEF09B, 48, 0},
                         {'S', 0x543CAC2D, 56, 0},
                         {'S', 0x0C605253, 60, 0},
                         {'S', 0xF2813FD9, 64, 0},
                         {'S', 0x1B6E6190, 68, 3}},
                        0x0033534C, 2656, 0x00B1EBE8, false));
}

// Obj* SlotObj_New338DE8(?, XmlCursor* c, const Def* d) (reads r4 / r5):
// 2648 bytes, vtable 0x00B1EE60, 17 record words: hashes 0xA2A0911A /
// 0x60DE0653 / 0x987C585E / 0x9AE17203 / 0x7D754C8B (+0..+16),
// 0x760CB725 (+24), f32 0x000110AD / 0x79792193 / 0xED3A6C6F /
// 0xEED63AC8 / 0x1C3937F0 / 0xB7EA3D94 (+28..+48, default the f32 at
// 0x00338DE4), s32 0xD015A596 / 0x29DD8505 (+52 / +56, 0), 0x1B6E6190
// (+60, 3), hash 0x12380FB5 (+20); +64 0.
DECOMP(00338DE8, SlotObj_New338DE8) {
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    Stack frame(ctx, 80);
    const u32 rec = frame.addr();
    ret(ctx, SlotObjNew(ctx, c, x, rec, 17,
                        {{0, 0}, {20, 0}, {24, 0}, {28, 0}, {32, 0}, {36, 0}, {40, 0}, {44, 0}, {48, 0}, {52, 0}, {56, 0}, {60, 3},
                         {64, 0}, {4, 0}, {8, 0}, {12, 0}, {16, 0}},
                        {{'H', 0xA2A0911A, 0, 0},
                         {'H', 0x60DE0653, 4, 0},
                         {'H', 0x987C585E, 8, 0},
                         {'H', 0x9AE17203, 12, 0},
                         {'H', 0x7D754C8B, 16, 0},
                         {'H', 0x760CB725, 24, 0},
                         {'F', 0x000110AD, 28, 0},
                         {'F', 0x79792193, 32, 0},
                         {'F', 0xED3A6C6F, 36, 0},
                         {'F', 0xEED63AC8, 40, 0},
                         {'F', 0x1C3937F0, 44, 0},
                         {'F', 0xB7EA3D94, 48, 0},
                         {'S', 0xD015A596, 52, 0},
                         {'S', 0x29DD8505, 56, 0},
                         {'S', 0x1B6E6190, 60, 3},
                         {'H', 0x12380FB5, 20, 0}},
                        0x00338DE4, 2648, 0x00B1EE60, false));
}
