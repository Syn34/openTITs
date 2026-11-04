// World_HandleEvent callees, part 8a: small leaves under the hevent7
// helpers (motion limits, mirrored offsets, treasure release, object
// text, event argument records).
#include "hevent8.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;

namespace sly4 {
namespace hevent8a {
constexpr u32 kMotionValue = 4, kMotionLimit = 8;
constexpr u32 kMotionZero = 0x004A0C18;       // f64 0.0
constexpr u32 kWorldTreasure = 0x0012847C;    // World +0x12847C: Treasure* (name?)
constexpr u32 kTreasureFlags = 164;           // bit 31: held
constexpr u32 kObjDef = 8, kDefTextSlots = 152, kObjTextSlots = 328;
constexpr u32 kTextSlotSize = 16, kTextSlotWidget = 12;
constexpr u32 kWidgetSetText = 188, kWidgetRefresh = 148;  // vtable slots
}  // namespace hevent8a
}  // namespace sly4

using namespace sly4::hevent8a;

// void Motion_Stop4A0C08(Motion* m) (name?): value and limit = 0 (+8 first).
DECOMP(004A0C08, Motion_Stop4A0C08) {
    const u32 m = arg<u32>(ctx, 0);
    store<u32>(m + kMotionLimit, 0);
    store<u32>(m + kMotionValue, 0);
    ctx.gpr[4] = 0;  // as the original leaves it
}

// f32 Motion_Set4A0C20(Motion* m, f32 limit) (name?): +8 = limit, and the
// value at +4 drops to the limit when above it, then is clamped at 0
// (fsel: NaNs take the limit, then 0). Returns the new value.
DECOMP(004A0C20, Motion_Set4A0C20) {
    const u32 m = arg<u32>(ctx, 0);
    const f64 limit = farg(ctx, 0);
    const f64 cur = lfs_f64(m + kMotionValue);
    const f64 d = static_cast<f32>(limit - cur);  // fsubs
    store<f32>(m + kMotionLimit, static_cast<f32>(limit));
    f64 v = d >= 0.0 ? cur : limit;
    v = v >= 0.0 ? v : std::bit_cast<f64>(load<u64>(kMotionZero));
    const f32 r = static_cast<f32>(v);  // frsp
    store<f32>(m + kMotionValue, r);
    ret(ctx, r);
}

// void World_Pause489F3C(World* w, u8 lost) (name?; releases the held
// treasure): Treasure_Release(t, lost) when the world's treasure at
// +0x12847C is set and held (+164 bit 31). r3 is the treasure (or 0) when
// nothing is called.
DECOMP(00489F3C, World_Pause489F3C) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 lost = arg<u32>(ctx, 1) & 0xFF;
    const u32 t = load<u32>(w + kWorldTreasure);
    ctx.gpr[3] = t;
    if (t == 0) return;
    if ((load<u32>(t + kTreasureFlags) & 0x80000000u) == 0) return;
    call(ctx, h8fn::Treasure_Release, {t, lost});
}

// void Obj_Offset362830(vec3* out, s32 kind, const vec3* pos) (name?):
// out = pos, mirrored by kind: 0 negates y, 1 negates x and y, 3 negates
// x; other kinds copy. Words are copied raw (lfs/stfs); f1 is left as the
// last value loaded, as in the original.
DECOMP(00362830, Obj_Offset362830) {
    const u32 out = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 2);
    const s32 kind = arg<s32>(ctx, 1);
    for (u32 k = 0; k < 12; k += 4) store<u32>(out + k, load<u32>(pos + k));
    u32 last = out + 8;
    if (kind == 1 || kind == 3) {
        store<u32>(out, fneg_bits(load<u32>(out)));
        last = out;
    }
    if (kind == 0 || kind == 1) {
        store<u32>(out + 4, fneg_bits(load<u32>(out + 4)));
        last = out + 4;
        ctx.gpr[4] = out + 4;
    }
    ctx.fpr[1] = lfs_f64(last);
}

// void Obj_SetText367530(Object* o, const char* text, u32 slot) (name?):
// for a slot below the def's count (+8 -> +152, unsigned), the slot's
// widget (o +328 -> 16-byte slots, +12) gets SetText(text, 0) and then
// Refresh() (vtable +188, +148); the widget is reloaded between the calls.
DECOMP(00367530, Obj_SetText367530) {
    const u32 o = arg<u32>(ctx, 0), text = arg<u32>(ctx, 1), slot = arg<u32>(ctx, 2);
    if (o == 0 || text == 0) return;  // r3 stays o (or 0)
    if (!(slot < load<u32>(load<u32>(o + kObjDef) + kDefTextSlots))) return;
    const u32 e = load<u32>(o + kObjTextSlots) + (slot << 4);
    const u32 w = load<u32>(e + kTextSlotWidget);
    ctx.gpr[3] = w;
    if (w == 0) return;
    hevent::vcall(ctx, w, kWidgetSetText, {w, text, 0});
    const u32 w2 = load<u32>(e + kTextSlotWidget);
    hevent::vcall(ctx, w2, kWidgetRefresh, {w2});
}

namespace {
// The shared body of the Args_Add* pair: a zeroed record {value, hash,
// type} appended at count (capacity doubled first when full), count + 1 and
// the high-water mark raised to it. Nothing when hash or value is 0.
void args_add(PPUContext& ctx, u8 type) {
    const u32 v = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), value = arg<u32>(ctx, 2);
    if (hash == 0 || value == 0) return;  // r3 stays v
    Stack frame(ctx, 80);  // the record lives at r1+112 (74 bytes)
    const u32 rec = frame.addr();
    store<u32>(rec + kArgHash, 0);
    store<u8>(rec + kArgType, 0);
    store<u8>(rec + kArgType2, 0);
    call(ctx, h8fn::Memset, {rec, 0, 68});
    store<u32>(rec + kArgHash, hash);
    store<u32>(rec + kArgValue, value);
    store<u8>(rec + kArgType, type);
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, h8fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + 8);
    }
    call(ctx, h8fn::Memmove, {load<u32>(v) + n * kArgSize, rec, kArgCopy});
    const u32 count = load<u16>(v + 8) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(count));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(count & 0xFFFF) ? hw : count));
    ctx.gpr[3] = hw;  // as the original leaves it
}
}  // namespace

// void Args_AddObject49FD84(ArgVector* v, u32 hash, Object* o) (name?):
// appends an object argument (type 2).
DECOMP(0049FD84, Args_AddObject49FD84) { args_add(ctx, kArgObject); }

// void Args_AddValue49FE90(ArgVector* v, u32 hash, u32 value) (name?):
// appends an integer argument (type 13).
DECOMP(0049FE90, Args_AddValue49FE90) { args_add(ctx, kArgInt); }

// void SubMesh_ConstructBase4FCFE8(SubMesh* m) (name?): forwards to
// 0x004FCE88 (which reads only r3).
DECOMP(004FCFE8, SubMesh_ConstructBase4FCFE8) {
    call(ctx, h8fn::SubMesh_ConstructBase2, {arg<u32>(ctx, 0)});
}
