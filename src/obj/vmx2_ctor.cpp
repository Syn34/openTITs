// Game code, VMX sweep part 2 (see vmx2.h): constructors that clear
// quadwords with vspltisw 0 / stvx.
#include "../sly4.h"
#include "vmx2.h"

using namespace sly4;
using namespace sly4::vmx2;

namespace {
constexpr u32 kArrayReserve = 0x00558F3C;    // Array_Reserve (Array*, u16 capacity)
constexpr u32 kSortedMapRemove = 0x007330C0; // SortedMap_Remove7330C0 (map, const key*)
constexpr u32 kEntities = 0x01056F1C;        // {+0 u16 cap, +2 u16 count, +4 u16 high-water, +6 u16 flags, +8 {key, T*}[]}
constexpr u32 kTmplMap = 0x01056F04;         // the component template map
constexpr u32 kVec3Zero = 0x01164B40;        // the global default Vec3 (zero)
constexpr u32 kVtTmpl = 0x00B0DAAC;          // the template base (its dtor unmaps +4)
constexpr u32 kVtEntity = 0x00B1BE30;
constexpr u32 kVtB1C118 = 0x00B1C118;
constexpr u32 kVtB1C478 = 0x00B1C478;
constexpr u32 kVtB1C4C8 = 0x00B1C4C8;

// The entity registry push the component constructors inline: {e->+4, e}
// appended (grown to twice the capacity when full), the high-water mark
// and flag 1 updated.
void EntityPush(PPUContext& ctx, u32 e) {
    u32 n = load<u16>(kEntities + 2);
    const u32 cap = load<u16>(kEntities);
    if (n == cap) {
        call(ctx, kArrayReserve, {kEntities, (cap << 1) & 0xFFFEu});
        n = load<u16>(kEntities + 2);
    }
    const u32 data = load<u32>(kEntities + 8);
    store<u32>(data + ((n << 3) & 0x7FFF8u), load<u32>(e + 4));
    store<u32>(data + (static_cast<u32>(load<u16>(kEntities + 2)) << 3) + 4, e);
    const u32 n1 = static_cast<u32>(load<u16>(kEntities + 2)) + 1;
    const u32 hw = load<u16>(kEntities + 4), fl = load<u16>(kEntities + 6);
    store<u16>(kEntities + 2, static_cast<u16>(n1));
    store<u16>(kEntities + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFFu) ? hw : n1));
    store<u16>(kEntities + 6, static_cast<u16>(fl | 1u));
}
}  // namespace

// void f(T* o, const Tmpl* t) (called by Comp_Create2635E8): the
// component constructor for vtable 0x00B1C4C8. The template's key words
// (+4 / +8 / +12) are copied, +16 = 1, +20 = 0 and o is pushed onto the
// entity registry; then the intermediate bases' vtables, +24 = 0, the
// quadwords +32..+79 cleared (stvx), and the temporary template copy (on
// the stack, as the original builds it) is unmapped
// (SortedMap_Remove7330C0(kTmplMap, &tmp.key)); then the defaults: zero
// counters, +96 / +112 / +224 / +256 the zero vector, flags +148 bits
// 0x80000000..0x00080000 cleared. Store order, f1 (the zero vector's z)
// and r3 (0x01160000, the original's last address high half) as the
// original leaves them.
DECOMP(00255CA8, Comp_Construct_00255CA8) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    Stack frame(ctx, 176);
    const u32 tmp = frame.addr() + 112;
    const u32 k4 = load<u32>(t + 4), k8 = load<u32>(t + 8), k12 = load<u32>(t + 12);
    store<u32>(tmp, kVtTmpl);
    store<u32>(tmp + 4, k4);
    store<u32>(tmp + 8, k8);
    store<u32>(tmp + 12, k12);
    store<u8>(tmp + 16, load<u8>(t + 16));
    store<u32>(o, kVtEntity);
    store<u32>(o + 4, k4);
    store<u32>(o + 8, k8);
    store<u32>(o + 12, k12);
    store<u8>(o + 16, 1);
    store<u32>(o + 20, 0);
    EntityPush(ctx, o);
    store<u32>(o, kVtB1C118);
    store<u32>(o + 24, 0);
    for (u32 k = 0; k < 3; ++k) Stvx(o + 32 + 16 * k, Zero4());
    store<u32>(tmp, kVtTmpl);
    store<u32>(o, kVtB1C478);
    call(ctx, kSortedMapRemove, {kTmplMap, tmp + 4});
    for (u32 off : {84u, 88u, 92u}) store<u32>(o + off, 0);
    const u32 flags = load<u32>(o + 148) & 0x0007FFFFu;
    for (u32 off : {96u, 112u})
        for (u32 k = 0; k < 3; ++k) CopyW(o + off + 4 * k, kVec3Zero + 4 * k);
    for (u32 off : {128u, 132u, 136u, 140u}) store<u32>(o + off, 0);
    store<u32>(o, kVtB1C4C8);
    for (u32 off : {80u, 192u, 196u, 200u}) store<u32>(o + off, 0);
    store<u32>(o + 148, flags);
    store<u32>(o + 144, 0);
    CopyW(o + 224, kVec3Zero);
    store<u32>(o + 152, 0);
    CopyW(o + 228, kVec3Zero + 4);
    store<u32>(o + 156, 0);
    CopyW(o + 232, kVec3Zero + 8);
    store<u32>(o + 160, 0);
    store<u32>(o + 164, 0);
    CopyW(o + 256, kVec3Zero);
    store<u32>(o + 240, 0);
    CopyW(o + 260, kVec3Zero + 4);
    CopyW(o + 264, kVec3Zero + 8);
    SetF1Bits(ctx, load<u32>(kVec3Zero + 8));
    ctx.gpr[3] = 0x01160000;
}
