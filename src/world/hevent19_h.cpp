// World_HandleEvent callees, part 19h: the animator record vector pushes
// (field-wise copies of the 224-byte look-at and 320-byte scale records)
// and the speaker line node constructor.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
// A field of a record copy: offset and kind (1 / 2 / 4 bytes, or 16 for an
// aligned vector, lvx / stvx).
struct Field {
    u16 off, size;
};
// The 224-byte look-at record's fields (Anim_A351B60), in copy order.
constexpr Field kLookAtFields[] = {
    {0, 4},   {16, 2},  {18, 2},  {20, 2},  {22, 2},  {24, 2},  {32, 16}, {48, 4},  {52, 4},  {56, 4},
    {60, 4},  {64, 4},  {80, 16}, {96, 16}, {112, 16}, {128, 16}, {144, 1}, {145, 1}, {160, 4}, {176, 16},
    {192, 4}, {196, 4}, {200, 4}, {204, 4}, {208, 4}, {212, 4}, {216, 4},
};
// The 320-byte scale record's fields (Anim_C352190), in copy order.
constexpr Field kScaleFields[] = {
    {0, 4},    {16, 4},   {20, 2},   {22, 2},   {24, 2},   {32, 16},  {48, 16},  {64, 16},  {80, 16},  {96, 4},
    {100, 4},  {104, 4},  {108, 4},  {112, 4},  {116, 4},  {120, 4},  {124, 4},  {128, 4},  {132, 1},  {144, 4},
    {160, 16}, {176, 16}, {192, 16}, {208, 16}, {224, 16}, {240, 16}, {256, 1},  {260, 4},  {264, 4},  {268, 4},
    {272, 4},  {276, 4},  {280, 4},  {284, 4},  {288, 4},  {292, 1},  {304, 4},  {308, 4},
};

// Appends a copy of rec (the listed fields only; float fields copied as raw
// words) to the vector; returns the new element's index (count - 1).
template <size_t N>
void record_push(PPUContext& ctx, u32 v, u32 rec, u32 size, const Field (&fields)[N], u32 last_f) {
    u32 n = load<u16>(v + hevent18::kPvCount);
    const u32 cap = load<u16>(v + hevent18::kPvCap);
    if (n == cap) {
        call(ctx, hevent8::h8fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + hevent18::kPvCount);
    }
    const u32 dst = load<u32>(v + hevent18::kPvData) + n * size;
    for (const Field& f : fields) {
        switch (f.size) {
            case 1: store<u8>(dst + f.off, load<u8>(rec + f.off)); break;
            case 2: store<u16>(dst + f.off, load<u16>(rec + f.off)); break;
            case 4: store<u32>(dst + f.off, load<u32>(rec + f.off)); break;
            default: {
                const u32 s = (rec + f.off) & ~15u, d = (dst + f.off) & ~15u;
                for (u32 k = 0; k < 16; k += 4) store<u32>(d + k, load<u32>(s + k));
            }
        }
    }
    ctx.fpr[1] = hevent7::lfs_f64(rec + last_f);  // f1 as the lfs / stfs copies leave it
    const u32 n1 = load<u16>(v + hevent18::kPvCount) + 1;
    const u32 hi = load<u16>(v + hevent18::kPvHigh);
    store<u16>(v + hevent18::kPvCount, static_cast<u16>(n1));
    store<u16>(v + hevent18::kPvHigh, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF) ? hi : n1));
    ret(ctx, static_cast<s64>(n1 & 0xFFFF) - 1);
}

// Speaker line node (108 bytes): +0 vtable 0x00B1F570, +4 / +8 / +24 /
// +28 / +32..+48 0, +12 u32 flags (bits 30..31 = 01, bits 22..23, 24, 27,
// 28..29 cleared), +16 / +20 f32 1.0, two Arrays of 8-byte entries at +52
// and +76 (capacity 64, compare OPD 0x00F377A8), +100 0, +104 u8 0.
constexpr u32 kNodeVtable = 0x00B1F570, kOneF = 0x3F800000u, kNodeCompare = 0x00F377A8;
constexpr u32 kMemDefaultAllocator = 0x00550EE8, kArrayInit = 0x00558F38;  // (symbols.txt)
constexpr u32 kArrCap = 0, kArrCount = 2, kArrHigh = 4, kArrFlags = 6, kArrData = 8, kArrAllocator = 12, kArr16 = 16, kArrCompare = 20;
constexpr u16 kNodeArrCap = 64, kNodeArrFlags = 16;
constexpr u32 kAllocVtAlloc = 0, kAllocAlign = 16;

// Array storage: allocator vt +0 alloc(size, 0, align); the original leaves
// the entry address in r7 and the OPD in r8.
inline u32 array_alloc(PPUContext& ctx, u32 al, u32 count, u32 flags) {
    const u32 opd = load<u32>(load<u32>(al) + kAllocVtAlloc);
    ctx.gpr[7] = load<u32>(opd);
    ctx.gpr[8] = opd;
    return static_cast<u32>(call_ptr(ctx, opd, {al, count * ((flags >> 1) & 0x7FFF), 0, kAllocAlign}));
}
// One of the node's Arrays: fields, allocator, storage when it has a
// capacity, then Array_Init.
inline void node_array(PPUContext& ctx, u32 arr) {
    const u32 al = static_cast<u32>(call(ctx, kMemDefaultAllocator));
    const u32 cap = load<u16>(arr + kArrCap);
    store<u32>(arr + kArrAllocator, al);
    if (cap != 0) store<u32>(arr + kArrData, array_alloc(ctx, al, cap, load<u16>(arr + kArrFlags)));
    call(ctx, kArrayInit, {arr});
}
}  // namespace

// s32 Vector_Push76E400(PtrVector* v, const Rec224* r) (name?): appends a
// look-at record (Anim_A351B60's layout); returns its index.
DECOMP(0076E400, Vector_Push76E400) {
    record_push(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), 224, kLookAtFields, 216);
}

// s32 Vector_Push76E704(PtrVector* v, const Rec320* r) (name?): appends a
// scale record (Anim_C352190's layout); returns its index.
DECOMP(0076E704, Vector_Push76E704) {
    record_push(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), 320, kScaleFields, 284);
}

// void Node_Construct3516C4(Node* n) (name?): constructs a 108-byte speaker
// line node. Returns r3 = 0.
DECOMP(003516C4, Node_Construct3516C4) {
    const u32 n = arg<u32>(ctx, 0);
    u32 w = load<u32>(n + 12);
    store<u32>(n + 4, 0);
    w = (w & 0x3FFFFFFFu) | 0x40000000u;
    store<u32>(n + 8, 0);
    store<u32>(n + 24, 0);
    w &= ~0x30000000u;
    store<u32>(n + 28, 0);
    w &= ~(0x08000000u | 0x01000000u | 0x00800000u | 0x00400000u);
    store<u32>(n + 12, w);
    store<u32>(n + 16, kOneF);
    store<u32>(n + 20, kOneF);
    store<u32>(n + 32, 0);
    store<u32>(n + 36, 0);
    store<u32>(n + 40, 0);
    store<u32>(n + 0, kNodeVtable);
    store<u32>(n + 44, 0);
    store<u32>(n + 48, 0);
    const u32 a1 = n + 52, a2 = n + 76;
    store<u16>(a1 + kArrCap, kNodeArrCap);
    store<u16>(a1 + kArrCount, 0);
    store<u16>(a1 + kArrHigh, 0);
    store<u16>(a1 + kArrFlags, kNodeArrFlags);
    store<u32>(a1 + kArrData, 0);
    node_array(ctx, a1);
    store<u32>(a1 + kArrCompare, kNodeCompare);
    store<u16>(a2 + kArrCap, kNodeArrCap);
    store<u16>(a2 + kArrFlags, kNodeArrFlags);
    store<u32>(a1 + kArr16, 0);
    store<u16>(a2 + kArrCount, 0);
    store<u16>(a2 + kArrHigh, 0);
    store<u32>(a2 + kArrData, 0);
    node_array(ctx, a2);
    store<u32>(a2 + kArrCompare, kNodeCompare);
    store<u32>(n + 100, 0);
    store<u8>(n + 104, 0);
    store<u32>(a1 + kArr16, 0);
    store<u32>(a2 + kArr16, 0);
    ret(ctx, 0u);
}
