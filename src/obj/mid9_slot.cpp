// Game code, medium sweep part 9: factories of the Obj_Construct_00769BD8
// classes (2648..2672-byte objects whose extra fields at +2580 are read
// from XML into a stack record first; as mid8_slot.cpp).
#include "mid9.h"

using namespace sly4;
using namespace sly4::mid9;

namespace {

// A field read into the record: 'H' hash, 'S' s32 (default def), 'F' f32
// (default the f32 at the factory's constant), 'f' f32 / 's' s32
// defaulting to the record's value.
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
        case 's': XS32(ctx, c, f.hash, rec + f.off); break;
        case 'f': XF32(ctx, c, f.hash, rec + f.off); break;
        default: XF32Def(ctx, c, f.hash, rec + f.off, fdef); break;
        }
    }
    const u32 p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {size}));
    if (p == 0) return 0;
    call(ctx, m9fn::Obj_Construct_00769BD8, {p, x});
    const u32 w = load<u32>(p + 2576) & 0x3FFFFFFFu;
    store<u32>(p + 2572, 0);
    if (vtFirst) store<u32>(p, vt);
    store<u32>(p + 2576, w);
    if (!vtFirst) store<u32>(p, vt);
    for (u32 k = 0; k < nwords; ++k) store<u32>(p + 2580 + 4 * k, load<u32>(rec + 4 * k));
    return p;
}

}  // namespace

// Obj* SlotObj_New335CC8(?, XmlCursor* c, const Def* d) (reads r4 / r5):
// 2652 bytes, vtable 0x00B1ECE8, 18 record words: hashes 0xCF4A1DD4 /
// 0x760CB725 (+0 / +4), f32 0x000110AD / 0x79792193 (+8 / +12, default the
// f32 at 0x00335CC0), hashes 0xA2A0911A, 0x33BEE3CF, 0xB37A3E80..82,
// 0x87D98D43, 0xBA1A6687, 0x8A223FF8, 0x93116591 (+16..+48), f32 0x33BEF09B
// (+52), s32 0xB390E3A6 / 0xBE36FB14 (+56 / +60, 0), 0x1B6E6190 (+64, 3),
// f32 0x4359F70E (+68).
DECOMP(00335CC8, SlotObj_New335CC8) {
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    Stack frame(ctx, 80);
    const u32 rec = frame.addr();
    ret(ctx, SlotObjNew(ctx, c, x, rec, 18,
                        {{0, 0}, {4, 0}, {12, 0x3F800000}, {8, 0xBF800000}, {16, 0}, {20, 0}, {24, 0}, {28, 0}, {32, 0}, {36, 0}, {40, 0}, {44, 0},
                         {48, 0}, {52, 0}, {56, 0}, {60, 0}, {64, 3}, {68, 0}},
                        {{'H', 0xCF4A1DD4, 0, 0},
                         {'H', 0x760CB725, 4, 0},
                         {'F', 0x000110AD, 8, 0},
                         {'F', 0x79792193, 12, 0},
                         {'H', 0xA2A0911A, 16, 0},
                         {'H', 0x33BEE3CF, 20, 0},
                         {'H', 0xB37A3E80, 24, 0},
                         {'H', 0xB37A3E81, 28, 0},
                         {'H', 0xB37A3E82, 32, 0},
                         {'H', 0x87D98D43, 36, 0},
                         {'H', 0xBA1A6687, 40, 0},
                         {'H', 0x8A223FF8, 44, 0},
                         {'H', 0x93116591, 48, 0},
                         {'F', 0x33BEF09B, 52, 0},
                         {'S', 0xB390E3A6, 56, 0},
                         {'S', 0xBE36FB14, 60, 0},
                         {'S', 0x1B6E6190, 64, 3},
                         {'F', 0x4359F70E, 68, 0}},
                        0x00335CC0, 2652, 0x00B1ECE8, false));
}

// Obj* SlotObj_New336210(?, XmlCursor* c, const Def* d) (reads r4 / r5):
// 2648 bytes, vtable 0x00B1ED68, 17 record words: hashes 0xA2A0911A /
// 0xCAD37C1E (+0 / +4), 0x9E635873 (+16, read twice: before and after
// the next two), 0xCF4A1DD4 (+8), f32 0x7F8F56C5 (+12), hash 0x760CB725
// (+20), then f32 +24..+64 (0x000110AD, 0x79792193, 0x71E2D9D1, 0x0857649E,
// 0xCAD388EA, 0xDA3FA842, 0xDA3FA930, then +60 / +64 / +52 / +56), every
// f32 defaulting to the f32 at 0x0033620C.
DECOMP(00336210, SlotObj_New336210) {
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    Stack frame(ctx, 80);
    const u32 rec = frame.addr();
    ret(ctx, SlotObjNew(ctx, c, x, rec, 17,
                        {{0, 0}, {4, 0}, {8, 0}, {12, 0}, {16, 0}, {20, 0}, {24, 0xBF800000}, {28, 0x3F800000}, {32, 0x42C80000},
                         {36, 0x42C80000}, {40, 0x42C80000}, {44, 0x3FC00000}, {48, 0x3F000000}, {52, 0x40A00000}, {56, 0x3E4CCCCD},
                         {60, 0x40000000}, {64, 0x41200000}},
                        {{'H', 0xA2A0911A, 0, 0},
                         {'H', 0xCAD37C1E, 4, 0},
                         {'H', 0x9E635873, 16, 0},
                         {'H', 0xCF4A1DD4, 8, 0},
                         {'F', 0x7F8F56C5, 12, 0},
                         {'H', 0x9E635873, 16, 0},
                         {'H', 0x760CB725, 20, 0},
                         {'F', 0x000110AD, 24, 0},
                         {'F', 0x79792193, 28, 0},
                         {'F', 0x71E2D9D1, 32, 0},
                         {'F', 0x0857649E, 36, 0},
                         {'F', 0xCAD388EA, 40, 0},
                         {'F', 0xDA3FA842, 44, 0},
                         {'F', 0xDA3FA930, 48, 0},
                         {'F', 0x42E92547, 60, 0},
                         {'F', 0x0C93BE83, 64, 0},
                         {'F', 0xB6E3CD4B, 52, 0},
                         {'F', 0x592BFAF6, 56, 0}},
                        0x0033620C, 2648, 0x00B1ED68, true));
}

// Obj* SlotObj_New32A560(?, XmlCursor* c, const Def* d) (reads r4 / r5):
// 2672 bytes, vtable 0x00B1E550, 23 record words: hashes 0xCF4A1DD4 /
// 0x760CB725 / 0x9E635873 (+0 / +8 / +12), f32 0x000110AD / 0x79792193
// (+16 / +20, default the f32 at 0x0032A55C), then f32 +24..+84 and +4
// (defaulting to their values: 1, 0.1, ?, 5, 1, 45, 4, 5, 1, 10, 1, 4 x5;
// +4 20), s32 0x543CAC2D (+88, default 25).
// Quirk: the record word +32 is never initialized, so its default is the
// stack garbage there (as in the original).
DECOMP(0032A560, SlotObj_New32A560) {
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    Stack frame(ctx, 96);
    const u32 rec = frame.addr();
    constexpr u32 k4 = 0x40800000;
    ret(ctx, SlotObjNew(ctx, c, x, rec, 23,
                        {{0, 0}, {8, 0}, {4, 0x41A00000}, {12, 0}, {16, 0xBF800000}, {20, 0x3F800000}, {24, 0x3F800000}, {28, 0x3DCCCCCD},
                         {36, 0x40A00000}, {40, 0x3F800000}, {44, 0x42340000}, {48, k4}, {52, 0x40A00000}, {56, 0x3F800000}, {60, 0x41200000},
                         {64, 0x3F800000}, {68, k4}, {72, k4}, {76, k4}, {80, k4}, {84, k4}, {88, 25}},
                        {{'H', 0xCF4A1DD4, 0, 0},
                         {'H', 0x760CB725, 8, 0},
                         {'H', 0x9E635873, 12, 0},
                         {'F', 0x000110AD, 16, 0},
                         {'F', 0x79792193, 20, 0},
                         {'f', 0x591E0E5E, 24, 0},
                         {'f', 0xDAC6B163, 28, 0},
                         {'f', 0x6EB97B5A, 32, 0},
                         {'f', 0x7F8F56C5, 36, 0},
                         {'f', 0x6AEF6477, 40, 0},
                         {'f', 0xEA445994, 44, 0},
                         {'f', 0xB45F6FDB, 48, 0},
                         {'f', 0xB45F6EED, 52, 0},
                         {'f', 0xD791BA44, 56, 0},
                         {'f', 0x6993ED21, 60, 0},
                         {'f', 0xAF21AF1C, 64, 0},
                         {'f', 0x58BE11B6, 68, 0},
                         {'f', 0x35092DB8, 72, 0},
                         {'f', 0xFA04CC11, 76, 0},
                         {'f', 0xFA04CB23, 80, 0},
                         {'f', 0x74C4E83D, 84, 0},
                         {'f', 0xB7EA3D94, 4, 0},
                         {'s', 0x543CAC2D, 88, 0}},
                        0x0032A55C, 2672, 0x00B1E550, true));
}

// Obj* SlotObj_New3373E8(?, XmlCursor* c, const Def* d) (reads r4 / r5): the
// fields are read on the stack first (hashes 0x35107D59 (default
// 0x35107ACF), 0x268C74EF (default 0xBE4B81CB), 0x6E6EDD6B (0), f32
// 0x0A1A79FB (default the f32 at 0x003373E4), the attribute 0xBFB1D754,
// vec3 0x4A4D4D4D (default zero), bool 0x9446A097 (0)); a 2992-byte object
// (SmallAlloc; 0 when that fails): Obj_Construct_00769BD8(p, d), vtable
// 0x00B1EDE8, +2572 / +2604 / +2636 the hashes, vec3 +2640, f32 +2736, the
// byte +2656 the bool, +2596 -1, the rest zero; +2576[5] = the values the
// map *0x00F65BE8 + 4 holds for the keys +2572 + 0..4
// (SortedMap_Find733F8C, not checked); the attribute (when present) copied
// (32 chars) to +2740 with +2771 = 0, else +2740 = "". Returns p.
DECOMP(003373E8, SlotObj_New3373E8) {
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    constexpr u32 kF = 0x003373E4, kZero = 0x01164B40, kMapPtr = 0x00F65BE8, SortedMap_Find733F8C = 0x00733F8C;
    Stack frame(ctx, 48);
    const u32 L = frame.addr();  // the original's r1 + 112: vec3 +0, hashes +16 / +20 / +24, f32 +28, key +32, bool +36
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x35107D59, L + 16, 0x35107ACF);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x268C74EF, L + 20, 0xBE4B81CB);
    XF32Def(ctx, c, 0x0A1A79FB, L + 28, kF);
    const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0xBFB1D754}));
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x6E6EDD6B, L + 24, 0);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x4A4D4D4D, L, kZero);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x9446A097, L + 36, 0);
    const u32 p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {2992}));
    if (p == 0) return ret(ctx, 0);
    const u32 h16 = load<u32>(L + 16), h20 = load<u32>(L + 20), h24 = load<u32>(L + 24), b = load<u8>(L + 36), f = load<u32>(L + 28);
    call(ctx, m9fn::Obj_Construct_00769BD8, {p, x});
    store<u32>(p + 2572, h20);
    store<u32>(p, 0x00B1EDE8);
    CopyW(p + 2640, L);
    store<u32>(p + 2596, 0xFFFFFFFFu);
    store<u8>(p + 2600, 0);
    store<u32>(p + 2604, h16);
    store<u8>(p + 2632, 0);
    store<u8>(p + 2633, 0);
    store<u8>(p + 2634, 0);
    CopyW(p + 2644, L + 4);
    store<u32>(p + 2636, h24);
    store<u32>(p + 2976, 0);
    store<u32>(p + 2660, 0);
    store<u8>(p + 2980, 0);
    store<u32>(p + 2608, 0);
    store<u32>(p + 2900, 0);
    store<u32>(p + 2736, f);
    store<u8>(p + 2656, static_cast<u8>(b));
    store<u32>(p + 2612, 0);
    CopyW(p + 2648, L + 8);
    store<u32>(p + 2616, 0);
    store<u32>(p + 2620, 0);
    store<u32>(p + 2624, 0);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(L + 8)));
    for (u32 i = 0; i < 5; ++i) {
        store<u32>(L + 32, load<u32>(p + 2572) + i);
        const u32 r = static_cast<u32>(call(ctx, SortedMap_Find733F8C, {load<u32>(kMapPtr) + 4, L + 32}));
        store<u32>(p + 2576 + 4 * i, load<u32>(r));
    }
    if (s == 0) {
        store<u8>(p + 2740, 0);
    } else {
        call(ctx, m8fn::String_CopyN, {p + 2740, s, 32});
        store<u8>(p + 2771, 0);
    }
    ret(ctx, p);
}
