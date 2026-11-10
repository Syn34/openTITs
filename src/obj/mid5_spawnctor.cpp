// Game code, medium sweep part 5: constructors of two 432-byte+ objects
// built on QuerySpawn_BaseCtor_0020C868 (made by the query-spawn methods
// of mid4_spawn.cpp's family): the base, the intermediate class's vtable
// and bounds (an empty box: +448 / +496 / +528 = FLT_MAX x3, +464 / +512
// / +544 = -FLT_MAX x3, then +448 / +464 set to the origin vector), then
// the class's vtable.
#include "mid5.h"

using namespace sly4;
using namespace sly4::mid5;

namespace {

namespace cfn {
constexpr u32 QuerySpawn_BaseCtor = 0x0020C868;  // (p, Query* q, 32, 1)
constexpr u32 Ptr_DerefNonZero = 0x00446960;     // (p) -> bool
constexpr u32 RefNode_Release = 0x004432DC;      // (node)
constexpr u32 Node_PushF65B7C = 0x00446984;      // (a, b) -> node
constexpr u32 Hash_ToString = 0x0056A6E0;        // (hash) -> const char*
}  // namespace cfn

constexpr u32 kVtBounded = 0x00B197D8;  // the intermediate class's vtable
constexpr u32 kOrigin = 0x01164B40;     // f32[3]
constexpr u32 kFltMax = 0x7F7FFFFF;
constexpr u32 kFltMin = 0xFF7FFFFF;
constexpr u32 kDebugNames = 0x010AEC60;  // u8: report missing nodes (name?)

// The base and the intermediate class (up to the +484 word): f1 is left
// holding kOrigin[2], as the lfs / stfs copies leave it.
void BoundedCtor(PPUContext& ctx, u32 o, u32 q, u32 vt) {
    call(ctx, cfn::QuerySpawn_BaseCtor, {o, q, 32, 1});
    store<u32>(o, kVtBounded);
    store<u32>(o + 428, 34);
    store<u32>(o + 464, kFltMin);
    store<u32>(o + 432, 0);
    store<u32>(o + 448, kFltMax);
    store<u32>(o + 452, kFltMax);
    store<u32>(o + 456, kFltMax);
    store<u32>(o + 468, kFltMin);
    store<u32>(o + 472, kFltMin);
    store<u8>(o + 480, 0);
    store<u32>(o + 448, load<u32>(kOrigin));
    store<u32>(o + 452, load<u32>(kOrigin + 4));
    store<u32>(o + 456, load<u32>(kOrigin + 8));
    store<u32>(o + 464, load<u32>(kOrigin));
    store<u32>(o + 0, vt);
    store<u32>(o + 468, load<u32>(kOrigin + 4));
    store<u32>(o + 484, 0);
    const u32 z = load<u32>(kOrigin + 8);
    store<u32>(o + 472, z);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(z));
}

// The three more boxes {+496 max, +512 min, +528 max, +544 min}.
void MoreBoxes(u32 o) {
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 496 + 4 * k, kFltMax);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 512 + 4 * k, kFltMin);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 528 + 4 * k, kFltMax);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 544 + 4 * k, kFltMin);
}

}  // namespace

// void f(T* o, Query* q): the class of vtable 0x00B19950; then the node
// of the object at o->+216: when its +184 is gone, released and pushed
// again from (+180, +176); with kDebugNames set and the node still
// missing, both hashes are turned into strings (the results unused).
DECOMP(00210478, BoundedA_Ctor_00210478) {
    const u32 o = arg<u32>(ctx, 0);
    BoundedCtor(ctx, o, arg<u32>(ctx, 1), 0x00B19950);
    store<u16>(o + 486, 0);
    MoreBoxes(o);
    const u32 d = load<u32>(o + 216);
    store<u32>(o + 612, 0);
    if (call(ctx, cfn::Ptr_DerefNonZero, {load<u32>(d + 184)}) == 0) {
        call(ctx, cfn::RefNode_Release, {load<u32>(d + 184)});
        store<u32>(d + 184, static_cast<u32>(call(ctx, cfn::Node_PushF65B7C, {load<u32>(d + 180), load<u32>(d + 176)})));
    }
    if (load<u8>(kDebugNames) == 0) {
        ret(ctx, 0u);  // r3 left as the flag
        return;
    }
    if (call(ctx, cfn::Ptr_DerefNonZero, {load<u32>(d + 184)}) != 0) return;
    call(ctx, cfn::Hash_ToString, {load<u32>(d + 180)});
    call(ctx, cfn::Hash_ToString, {load<u32>(d + 176)});
}

// void f(T* o, Query* q): the class of vtable 0x00B19CC8 (+612 / +613 u8,
// +616 / +620 u32 cleared). Returns 0 in r3, as left.
DECOMP(00212FC8, BoundedB_Ctor_00212FC8) {
    const u32 o = arg<u32>(ctx, 0);
    BoundedCtor(ctx, o, arg<u32>(ctx, 1), 0x00B19CC8);
    MoreBoxes(o);
    store<u8>(o + 612, 0);
    store<u8>(o + 613, 0);
    store<u32>(o + 616, 0);
    store<u32>(o + 620, 0);
    ret(ctx, 0u);
}
