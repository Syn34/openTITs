// Game code, part 54: the remaining small game functions that use VMX
// (smallest first). Each was checked against the original in the PPC
// simulator (memory writes, calls and their arguments, results).
#include "obj54.h"

namespace sly4::obj {

// ---------------------------------------------------------------- types
struct Force54 {
    pad<24> _0;
    be_u32 w24;    // +24
    pad<4> _28;
    be_f32 f32_;   // +32
    be_f32 f36;    // +36
    be_f32 f40;    // +40
};
struct ForceDef54 {
    pad<32> _0;
    be_f32 amount;  // +32
};
// Force_Apply's record (on the stack).
struct ForceRec54 {
    be_u32 force;   // +0
    pad<12> _4;     // never written (Force_Apply does not read it)
    Quad q16;       // +16
    Quad q32;       // +32
    be_u32 w48;     // +48
    be_f32 step;    // +52
    be_u32 w56;     // +56
    be_u32 w60;     // +60
    be_u32 w64;     // +64
};
static_assert(offsetof(ForceRec54, w64) == 64);

struct BodyHolder54 {
    pad<24> _0;
    gp<HkRigidBody> body;  // +24
};
struct BodyObj54 {
    pad<612> _0;
    gp<BodyHolder54> holder;  // +612
};

struct LaunchMsg54 {
    be_u32 kind;    // +0
    be_u32 w4;      // +4
    pad<4> _8;
    be_u32 state;   // +12
};
struct LaunchParent54 {
    pad<348> _0;
    be_f32 level;   // +348
};
struct LaunchObj54 {
    pad<8> _0;
    gp<LaunchParent54> parent;  // +8
    pad<20> _12;
    Quad q32, q48, q64;         // +32 .. +79
    be_u32 sub;                 // +80: embedded state machine (vtable)
};
static_assert(offsetof(LaunchObj54, sub) == 80);

struct MatrixOwner54 {
    pad<96> _0;
    Quad rows[4];   // +96
};
struct TargetEvent54 {
    pad<16> _0;
    be_u32 p[3];    // +16 .. +24
};
struct Target54 {
    pad<144> _0;
    be_u32 pos[3];  // +144
};
struct TargetObj54 {
    pad<8> _0;
    gp<MatrixOwner54> frame;  // +8
    pad<40> _12;
    gp<be_u32> list;          // +52
    pad<4> _56;
    be_u16 count;             // +60
    pad<14> _62;
    be_u32 target;            // +76
    be_u32 p[3];              // +80 .. +88
    pad<4> _92;
    be_u32 local[4];          // +96 (a quadword)
};
static_assert(offsetof(TargetObj54, count) == 60 && offsetof(TargetObj54, target) == 76 && offsetof(TargetObj54, local) == 96);

struct Rec64 {
    Quad q0;        // +0
    Quad q1;        // +16
    u8 b32;         // +32
    pad<3> _33;
    be_u32 w36;     // +36
    be_u32 w40;     // +40
    pad<4> _44;
    be_u32 a;       // +48
    be_u32 b;       // +52
    be_u32 zero;    // +56
    u8 not_c;       // +60
    u8 c;           // +61
    pad<2> _62;
};
static_assert(sizeof(Rec64) == 64);
struct RecArrayOwner54 {
    pad<56> _0;
    HkArray<Rec64> arr;  // +56
};

struct CharBodyObj54 {
    pad<484> _0;
    be_u32 flags;   // +484
    pad<112> _488;
    be_u32 cbody;   // +600: hkpCharacterRigidBody*
};
static_assert(offsetof(CharBodyObj54, cbody) == 600);
struct PhysWorld54 {
    pad<4> _0;
    be_u32 hk_world;  // +4
};
struct GameWorld54 {
    pad<0x001281D4> _0;
    gp<PhysWorld54> phys;  // +0x1281D4
};

struct Elem224 {
    be_u32 w0;          // +0
    pad<12> _4;
    be_u16 h16[5];      // +16 .. +24
    pad<6> _26;
    Quad q32;           // +32
    be_u32 w48[5];      // +48 .. +64
    pad<12> _68;
    Quad q80, q96, q112, q128;  // +80 .. +143
    u8 b144, b145;      // +144, +145
    pad<14> _146;
    be_u32 w160;        // +160
    pad<12> _164;
    Quad q176;          // +176
    be_u32 w192, w196;  // +192, +196
    be_u32 w200[5];     // +200 .. +216
    pad<4> _220;
};
static_assert(sizeof(Elem224) == 224 && offsetof(Elem224, q176) == 176 && offsetof(Elem224, w200) == 200);
struct Vec224 {
    be_u32 data;     // +0: Elem224*
    pad<4> _4;
    be_u16 count;    // +8
};

namespace {
// A quadword moved as raw words (lvx / stvx: the aligned blocks).
Quad LoadQ54(const Quad& s) { return gm<Quad>(guest(&s) & ~15u); }
void StoreQ54(Quad& d, const Quad& v) {
    Quad& dd = gm<Quad>(guest(&d) & ~15u);
    for (u32 k = 0; k < 4; ++k) dd.w[k] = v.w[k].get();
}
void CopyQ54(Quad& d, const Quad& s) { StoreQ54(d, LoadQ54(s)); }
void ZeroQ54(Quad& d) {
    Quad& dd = gm<Quad>(guest(&d) & ~15u);
    for (u32 k = 0; k < 4; ++k) dd.w[k] = 0u;
}

// The shared head of the two launcher state handlers below: st = (o +
// 80)->vt[32](o + 80, 0), replaced by m->+12 when m->+0 and m->+4 are both
// 0. For a message of kind (m->+0) 2 the three quadwords at o + 32 are
// cleared and nothing is dispatched when the f32 (o->+8)->+348 <= the
// pool f32 at k. Returns whether to dispatch on st.
bool LaunchMsgHead54(LaunchObj54* o, const LaunchMsg54* m, u32 k, s32& st) {
    st = vcall<s32>(&o->sub, 32, 0u);
    const u32 kind = m->kind;
    LaunchParent54* p = o->parent;
    if (kind == 0 && m->w4 == 0) st = static_cast<s32>(m->state.get());
    if (kind != 2) return true;
    ZeroQ54(o->q32);
    ZeroQ54(o->q48);
    ZeroQ54(o->q64);
    const f64 lim = lf(global<be_f32>(k)), v = lf(p->level);
    return !(v < lim || v == lim);  // unordered dispatches
}
}  // namespace

// ---------------------------------------------------------------- VMX: 78 to 87 instructions
// void f(Force* t, const Def* d): the force d->+32 applied in steps. n =
// the f32 d->+32 / the pool f32 0x00ADBE74 (1 / 15), truncated to 64 bits
// (its low word), plus one when the fraction left is > 1e-5 (0x004AC5D0);
// step = d->+32 / n + 1e-5 (single steps). While the amount left is > 0
// (0x004AC5C4): Force_Apply_004AC210(&rec) with rec (stack) = {+0 t, +16
// / +32 two zero quadwords, +48 t->+24 (read once), +52 min(step, left),
// +56 / +60 / +64 t->+40 / +32 / +36}; left -= that. (d->+32 == 0 gives 0
// / 0: the host's default NaN, as the recompiled original, where a real
// PPU would give +NaN.)
void Force_ApplySteps_004AC5D8(Force54* t, const ForceDef54* d) {
    constexpr u32 K = 0x004AC5B8u;
    f64 left = lf(d->amount);
    const u32 t24 = t->w24;
    const f64 q = S(left / lf(global<be_f32>(0x00ADBE74u)));
    u32 n = static_cast<u32>(Fctidz(q));
    const f64 eps = lf(global<be_f32>(K + 24)), zero = lf(global<be_f32>(K + 12));
    const f64 frac = S(q - S(Fcfid(n)));
    if (frac > eps) n += 1;
    const f64 per = S(left / S(Fcfid(n)));
    const f64 step = S(per + eps);
    if (!(left > zero)) return;
    GuestFrame fr(144);
    ForceRec54* rec = fr.as<ForceRec54>();
    do {
        f64 s = step;
        if (s > left) s = left;
        const u32 w32 = bits(t->f32_), w36 = bits(t->f36), w40 = bits(t->f40);
        rec->force = guest(t);
        rec->w48 = t24;
        rec->step = static_cast<f32>(s);
        rec->w56 = w40;
        rec->w60 = w32;
        rec->w64 = w36;
        ZeroQ54(rec->q16);
        ZeroQ54(rec->q32);
        call(o54fn::Force_Apply, rec);
        left = S(left - s);
    } while (left > zero);
}

// void f(T* o, const f32 a[3], const f32 b[3]): with the rigid body
// (o->+612)->+24, its u8 +232 not 5 / 4 and its +8 set: HkSetVecs54 of a
// and b (masks 0x0023C930, vt[88]).
void Body_SetVecPair_0023C960(BodyObj54* o, const Vec3* a, const Vec3* b) {
    BodyHolder54* h = o->holder;
    HkRigidBody* rb = h != nullptr ? h->body.get() : nullptr;
    if (rb == nullptr || rb->motion_type == 5 || rb->motion_type == 4 || rb->world == 0) return;
    HkSetVecs54(rb, a, b, 0x0023C930u, 88);
}

// u32 f(T* o, const Msg* m) (called as this - 80 by AdjThunk_0027FC30):
// LaunchMsgHead54 (pool f32 0x0027FAE8), then st 1 -> f_0027F630(o, m), 2
// -> SplineFollow_Step_0027F2C0, 3 -> Launch_OnMsg_0027EE70. Returns the
// handler's result, else 1.
u32 LaunchState_Dispatch_0027FAF0(LaunchObj54* o, const LaunchMsg54* m) {
    s32 st = 0;
    if (LaunchMsgHead54(o, m, 0x0027FAE8u, st)) {
        if (st == 1) return call<u32>(0x0027F630u, o, m);
        if (st == 2) return call<u32>(0x0027F2C0u, o, m);
        if (st == 3) return call<u32>(0x0027EE70u, o, m);
    }
    return 1;
}

// void f(T* o, target, const Event* e): for every entry of the u32 list
// o->+52 (u16 count +60) equal to target: o->+76 = target, +80..+88 =
// e->+16..+24 and +96..+104 = target->+144..+152 (raw words), then the
// quadword o + 96 = that point in the frame of the matrix (o->+8) + 96
// (InvRigid4 with the masks at 0x002B0460: t + R^T p; rows read after the
// stores).
void Obj_SetTargetLocal_002B0480(TargetObj54* o, Target54* tg, const TargetEvent54* e) {
    constexpr u32 K = 0x002B0460u;
    for (u32 i = 0; i < o->count; ++i) {
        if (o->list.get()[i] != guest(tg)) continue;
        o->target = guest(tg);
        o->p[0] = e->p[0];
        o->p[1] = e->p[1];
        MatrixOwner54* mo = o->frame;
        o->p[2] = e->p[2];
        f32 p[3];
        for (u32 j = 0; j < 3; ++j) {
            const u32 w = tg->pos[j];
            p[j] = Lvlx(&tg->pos[j]).e[0];
            o->local[j] = w;
        }
        const InvRows4 r = InvRigid4(mo->rows, Lvx(host<u8>(K)), Lvx(host<u8>(K + 16)));
        Stvx(o->local, Add4(r.t, Row3(r.c[0], r.c[1], r.c[2], p[0], p[1], p[2])));
    }
}

// u32 f(T* o, const Msg* m) (called as this - 80 by AdjThunk_0027E480):
// LaunchMsgHead54 (pool f32 0x0027E328), then st 4 -> f_0027D340(o, m), 3
// -> Launch_OnMsg_0027D6E8, 2 -> SplineFollow_Step_0027DB68, 1 ->
// f_0027DEA8. Returns the handler's result, else 1.
u32 LaunchState_Dispatch_0027E330(LaunchObj54* o, const LaunchMsg54* m) {
    s32 st = 0;
    if (LaunchMsgHead54(o, m, 0x0027E328u, st)) {
        if (st == 4) return call<u32>(0x0027D340u, o, m);
        if (st == 3) return call<u32>(0x0027D6E8u, o, m);
        if (st == 2) return call<u32>(0x0027DB68u, o, m);
        if (st == 1) return call<u32>(0x0027DEA8u, o, m);
    }
    return 1;
}

// u32 f(T* o, const Rec* src, u32 a, u32 b, u8 c): pushes a 64-byte
// record onto the hkArray at o + 56 (HkArray_Grow(0x011DF31C, o + 56, 64)
// when full, src read before it): the two quadwords of src, its u8 +32
// and words +36 / +40, +48 = a, +52 = b, +56 = 0, u8 +60 = (c == 0), u8
// +61 = c (nothing written when the slot address is 0). Returns the new
// size.
u32 HkArray_PushRec64_00346734(RecArrayOwner54* o, const Rec64* src, u32 a, u32 b, u8 c) {
    const Quad q0 = LoadQ54(src->q0), q1 = LoadQ54(src->q1);
    const u8 b32 = src->b32;
    const u32 w36 = src->w36, w40 = src->w40;
    HkArray<Rec64>& arr = o->arr;
    u32 n = static_cast<u32>(arr.size.get());
    if (n == (arr.capflags & 0x3FFFFFFFu)) {
        call(fn::HkArray_Grow, kHkArrayAlloc, &arr, 64u);
        n = static_cast<u32>(arr.size.get());
    }
    const u32 pa = arr.data.ea + (n << 6);
    if (pa != 0) {
        Rec64& p = gm<Rec64>(pa);
        StoreQ54(p.q0, q0);
        StoreQ54(p.q1, q1);
        p.b32 = b32;
        p.w36 = w36;
        p.w40 = w40;
        p.a = a;
        p.b = b;
        p.zero = 0u;
        p.not_c = c == 0 ? 1 : 0;
        p.c = c;
    }
    arr.size = static_cast<s32>(n + 1);
    return n + 1;
}

// u32 f(Obj* o): sets bit 0x80000000 of o->+484. When the rigid body of
// the character body o->+600 (Hk_GetBody) has no world (+8 == 0) it is
// added to the game's physics world ((*(World + 0x1281D4))->+4,
// Hk_AddEntity(w, body, 1)). Then the body's velocity is set to the zero
// vector (HkSetVecs54 of 0x01164B40, masks 0x001111D0, vt[68]). Returns
// Hk_GetBody(o->+600) (called once more at the end).
u32 Obj_EnableBody_00111200(CharBodyObj54* o) {
    o->flags = o->flags | 0x80000000u;
    HkRigidBody* b0 = call<HkRigidBody*>(o54fn::Hk_GetBody, o->cbody.get());
    if (b0->world == 0) {
        const u32 w = global<gp<GameWorld54>>(kWorld)->phys->hk_world;
        HkRigidBody* b = call<HkRigidBody*>(o54fn::Hk_GetBody, o->cbody.get());
        call(o54fn::Hk_AddEntity, w, b, 1u);
    }
    call(o54fn::Hk_GetBody, o->cbody.get());
    HkRigidBody* rb = call<HkRigidBody*>(o54fn::Hk_GetBody, o->cbody.get());
    HkSetVecs54(rb, &global<Vec3>(kZeroVec), nullptr, 0x001111D0u, 68);
    return call<u32>(o54fn::Hk_GetBody, o->cbody.get());
}

// void f(Vec224* v, s32 i): removes element i of the vector of 224-byte
// elements (data +0, u16 count +8) by moving the last one over it: count
// - 1 first (stored even when nothing moves), nothing moved when i is the
// last. The element copy (in the original's order): the quadwords +32,
// +80, +96, +112, +128, +176, u16 +16..+24, words +0, +48..+64, +160,
// +192, +196, +200..+216 and bytes +144 / +145 (the rest is padding).
void Vec224_RemoveSwap_0076E5A8(Vec224* v, s32 i) {
    const u32 n1 = static_cast<u32>(v->count) - 1;
    const u32 last = n1 & 0xFFFFu;
    v->count = static_cast<u16>(n1);
    if (i == static_cast<s32>(last)) return;
    const u32 data = v->data;
    Elem224& d = gm<Elem224>(data + static_cast<u32>(i) * 224u);
    const Elem224& s = gm<Elem224>(data + last * 224u);
    CopyQ54(d.q32, s.q32);
    d.h16[0] = s.h16[0];
    d.w48[0] = s.w48[0];
    d.w48[1] = s.w48[1];
    d.h16[1] = s.h16[1];
    d.w48[2] = s.w48[2];
    d.h16[2] = s.h16[2];
    d.w48[3] = s.w48[3];
    d.w0 = s.w0;
    d.h16[3] = s.h16[3];
    d.w48[4] = s.w48[4];
    d.h16[4] = s.h16[4];
    CopyQ54(d.q80, s.q80);
    CopyQ54(d.q96, s.q96);
    CopyQ54(d.q112, s.q112);
    CopyQ54(d.q128, s.q128);
    d.b144 = s.b144;
    d.b145 = s.b145;
    CopyQ54(d.q176, s.q176);
    d.w160 = s.w160;
    d.w200[0] = s.w200[0];
    d.w200[1] = s.w200[1];
    d.w192 = s.w192;
    d.w200[2] = s.w200[2];
    d.w196 = s.w196;
    d.w200[3] = s.w200[3];
    d.w200[4] = s.w200[4];
}

}  // namespace sly4::obj

NATIVE(004AC5D8, sly4::obj::Force_ApplySteps_004AC5D8)
NATIVE(0023C960, sly4::obj::Body_SetVecPair_0023C960)
NATIVE(0027FAF0, sly4::obj::LaunchState_Dispatch_0027FAF0)
NATIVE(002B0480, sly4::obj::Obj_SetTargetLocal_002B0480)
NATIVE(0027E330, sly4::obj::LaunchState_Dispatch_0027E330)
NATIVE(00346734, sly4::obj::HkArray_PushRec64_00346734)
NATIVE(00111200, sly4::obj::Obj_EnableBody_00111200)
NATIVE(0076E5A8, sly4::obj::Vec224_RemoveSwap_0076E5A8)
