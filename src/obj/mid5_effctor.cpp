// Game code, medium sweep part 5: two more constructors of objects built
// on QuerySpawn_BaseCtor_0020C868 that start an effect (EffectDef_Find
// by hash, Obj_CreateEffectDCA20, then the effect's component 0x70D68D3A
// pointed back at the object when its type (vfn[8]) is 15).
#include "mid5.h"

using namespace sly4;
using namespace sly4::mid5;

namespace {

namespace efn {
constexpr u32 QuerySpawn_BaseCtor = 0x0020C868;  // (p, Query* q, n, 1)
constexpr u32 Obj_Clear68Set = 0x0049E8A8;       // (Rec76* r, u32 v)
constexpr u32 Obj_FindRecord76 = 0x002D9994;     // (T* o, Rec76* r) -> bool
constexpr u32 EffectDef_Find = 0x004BDEDC;       // (u32 hash, u32 set_name, u8 search_all) -> def
constexpr u32 Obj_CreateEffect = 0x000DCA20;     // (Object*, def, u32 bone hash, u8 start) -> Effect*
constexpr u32 Effect_FindComponent = 0x004BB4B4;  // (Effect*, u32 type hash) -> component or 0
constexpr u32 Obj_SetMatrix_00224170 = 0x00224170;  // (o, const f32 m[4][4]) (name?)
constexpr u32 Light_Init_004ACD68 = 0x004ACD68;   // (p, const Rec176* r) (name?)
}  // namespace efn

constexpr u32 kRecordHolder = 0x00F64C20;  // -> +40 the object the records are looked up in (name?)
constexpr u32 kBaseCtorN = 0x00F64A30;     // u32: the base ctor's third argument (name?)
constexpr u32 kIdentity = 0x01164BD0;      // f32[4][4]
constexpr u32 kHashBack = 0x70D68D3A;      // the component type pointed back at the object
constexpr s32 kTlsRouter = -28332;         // r13 +: hkMemoryRouter* (+44 the allocator, vt +8 alloc(a, size))

// The effect of def hash h on o: o->eff_at = Obj_CreateEffect(o, def, 0,
// 1) (o->def_at = def first when def_at is set); for a new effect its +136
// bit 0x200 is cleared, then mark(); its component kHashBack, when of type
// 15, gets +20 = o. r3 is left as the last call's result.
template <typename Mark>
void StartEffect(PPUContext& ctx, u32 o, u32 h, u32 def_at, u32 eff_at, Mark mark) {
    const u32 d = static_cast<u32>(call(ctx, efn::EffectDef_Find, {h, 0, 1}));
    if (def_at != 0) store<u32>(o + def_at, d);
    if (d == 0) return;
    const u32 e = static_cast<u32>(call(ctx, efn::Obj_CreateEffect, {o, d, 0, 1}));
    store<u32>(o + eff_at, e);
    if (e == 0) return;
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
    mark();
    const u32 c = static_cast<u32>(call(ctx, efn::Effect_FindComponent, {load<u32>(o + eff_at), kHashBack}));
    if (c == 0) return;
    if (static_cast<u32>(VCall(ctx, c, 32, {c})) == 15) store<u32>(c + 20, o);
}

}  // namespace

// void f(T* o, Def* q): the base (n 32), vtable 0x00B1A770; +436 / +440
// the hashes q->+176 / +180, +444.. q's four f32 +184.., +460 q->+200
// (a record id), +428 / +432 / +464 cleared; +468 bits: 0x80000000 /
// 0x40000000 / 0x20000000 cleared, 0x10000000 = q->+164 (u16) set,
// 0xC0000000 set when the record q->+200 is found in the record holder's
// object with +72 == 13 and a non-zero +0 (a 76-byte local), 0x20000000
// once the effect of q->+180 has started.
DECOMP(0021E3F8, EffectObj_Ctor_0021E3F8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = arg<u32>(ctx, 1);
    call(ctx, efn::QuerySpawn_BaseCtor, {o, q, 32, 1});
    store<u32>(o + 444, load<u32>(q + 184));
    store<u32>(o + 436, load<u32>(q + 176));
    store<u32>(o + 448, load<u32>(q + 188));
    store<u32>(o + 440, load<u32>(q + 180));
    u32 f = load<u32>(o + 468) & 0x1FFFFFFFu;
    const u32 id = load<u32>(q + 200);
    store<u32>(o + 452, load<u32>(q + 192));
    store<u32>(o, 0x00B1A770);
    store<u32>(o + 428, 0);
    store<u32>(o + 432, 0);
    store<u32>(o + 468, f);
    const u32 w196 = load<u32>(q + 196);
    store<u32>(o + 456, w196);
    store<u32>(o + 460, id);
    store<u32>(o + 464, 0);
    f = (f & ~0x10000000u) | (load<u16>(q + 164) != 0 ? 0x10000000u : 0u);
    store<u32>(o + 468, f);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w196));  // f1 left by the copies
    if (id != 0) {
        if (const u32 h = load<u32>(load<u32>(kRecordHolder) + 40); h != 0) {
            Stack frame(ctx, 80);
            const u32 r = frame.addr();
            call(ctx, efn::Obj_Clear68Set, {r, id});
            if (call(ctx, efn::Obj_FindRecord76, {h, r}) != 0 && load<u8>(r + 72) == 13 && load<u32>(r) != 0)
                store<u32>(o + 468, load<u32>(o + 468) | 0xC0000000u);
        }
    }
    StartEffect(ctx, o, load<u32>(o + 440), 432, 428, [&] { store<u32>(o + 468, load<u32>(o + 468) | 0x20000000u); });
}

// Light* f(T* o, Def* q): the base (n = *kBaseCtorN), vtable 0x00B1B010,
// +428.. cleared, Obj_SetMatrix_00224170(o, identity); then from the
// def at o->+216: +444 = its u8 +212, +428 its effect hash +176 (the
// effect started, +445 = 1); and a 176-byte light record {+0 / +4 its
// +180 / +184, +16 the identity's rows, +80 its f32 +188 / +192 / +196
// / +200 / +208 / +204, +104 u8: either of +204 / +208 differs from the
// f32 just before the function} allocated from the thread's Havok memory
// router (+4 = 176, also through a null result, as the original) and
// initialized by Light_Init_004ACD68; returns it, stored in +436.
DECOMP(00224448, LightObj_Ctor_00224448) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = arg<u32>(ctx, 1);
    call(ctx, efn::QuerySpawn_BaseCtor, {o, q, load<u32>(kBaseCtorN), 1});
    Stack frame(ctx, 64 + 176);
    const u32 m = frame.addr();         // f32[4][4]: the identity's rows (w unwritten)
    const u32 rec = frame.addr() + 64;  // the light record (the first 105 bytes written)
    store<u32>(o, 0x00B1B010);
    store<u32>(o + 428, 0);
    store<u32>(o + 432, 0);
    store<u32>(o + 436, 0);
    store<u32>(o + 440, 0);
    store<u8>(o + 444, 0);
    store<u8>(o + 445, 0);
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(m + 16 * row + 4 * k, load<u32>(kIdentity + 16 * row + 4 * k));
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kIdentity + 52)));  // the lfs pairs' leftovers
    ctx.fpr[2] = std::bit_cast<f64>(LfsBits(load<u32>(kIdentity + 56)));
    call(ctx, efn::Obj_SetMatrix_00224170, {o, m});
    const u32 d = load<u32>(o + 216);
    store<u8>(o + 444, load<u8>(d + 212));
    const u32 h = load<u32>(d + 176);
    store<u32>(o + 428, h);
    StartEffect(ctx, o, h, 0, 432, [&] { store<u8>(o + 445, 1); });
    const u32 one = load<u32>(0x00224440);  // the f32 1.0? just before the function
    const u32 w204 = load<u32>(d + 204);
    u8 differs = 1;
    if (std::bit_cast<f32>(w204) == std::bit_cast<f32>(one)) differs = std::bit_cast<f32>(load<u32>(d + 208)) == std::bit_cast<f32>(one) ? 0 : 1;
    store<u32>(rec, load<u32>(d + 180));
    store<u32>(rec + 4, load<u32>(d + 184));
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(rec + 16 + 16 * row + 4 * k, load<u32>(m + 16 * row + 4 * k));
    const u32 w188 = load<u32>(d + 188);
    store<u32>(rec + 80, w188);
    store<u32>(rec + 84, load<u32>(d + 192));
    store<u32>(rec + 88, load<u32>(d + 196));
    store<u32>(rec + 92, load<u32>(d + 200));
    store<u32>(rec + 96, load<u32>(d + 208));
    store<u32>(rec + 100, w204);
    store<u8>(rec + 104, differs);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w188));
    const u32 a = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(kTlsRouter)) + 44);
    const u32 p = static_cast<u32>(VCall(ctx, a, 8, {a, 176}));
    store<u16>(p + 4, 176);  // also at address 4 for a failed allocation, as the original
    if (p != 0) call(ctx, efn::Light_Init_004ACD68, {p, rec});
    store<u32>(o + 436, p);
    ret(ctx, p);
}
