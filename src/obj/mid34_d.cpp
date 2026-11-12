// Game code, medium sweep part 34: the wind areas' fade-object lists.
//
// A wind area keeps its fade objects (+28 owner / id, +32 the area's Havok
// reference, +44 state) in two pointer vectors: +128 the active ones, +148
// the inactive ones; +172 u8 the area's on flag.
#include "mid34.h"

namespace sly4::obj {

// The fades' embedded timer (vtable 0x00B21B88; Timer_Advance4A0990).
struct FadeTimer {
    be_u32 vtable;   // +0
    be_f32 elapsed;  // +4
    be_f32 limit;    // +8
    be_f32 t;        // +12: progress 0..1
    be_u32 flags;    // +16: bit 0 kept on construction, bits 1..15 a count
    be_u16& flags_hi() { return *reinterpret_cast<be_u16*>(&flags); }
    u8& flags_lo() { return reinterpret_cast<u8*>(&flags)[3]; }
};
static_assert(sizeof(FadeTimer) == 20);

struct FadeMode {
    pad<416> _0;
    be_u32 mode;  // +416: bits 28-30
};
// The object a fade follows (also its id).
struct FadeOwner {
    be_u32 vtable;   // +0
    pad<140> _4;
    Vec3 pos;        // +144
    pad<16> _156;
    be_u32 flags;    // +172: bit 0x40000 = live
    pad<2> _176;
    be_u16 fades;    // +178: fade count
    pad<4> _180;
    be_u32 mask_hi;  // +184: u64 mask (high word)
    be_u32 mask_lo;  // +188: (low word; bit 0 = active)
};
static_assert(offsetof(FadeOwner, flags) == 172 && offsetof(FadeOwner, fades) == 178 && offsetof(FadeOwner, mask_lo) == 188);
struct FadeEntityRef {
    pad<8> _0;
    be_u32 world;  // +8
};
struct WindFade {
    be_u32 vtable;             // +0
    pad<20> _4;                // the Havok entity base
    gp<FadeEntityRef> ent;     // +24
    gp<FadeOwner> owner;       // +28 (also the id)
    be_u32 area;               // +32: Havok reference
    be_f32 a36, a40;           // +36, +40
    be_s32 state;              // +44
    FadeTimer timer;           // +48
    be_f32 from;               // +68
    be_f32 to;                 // +72
    be_f32 value;              // +76
    gp<be_f32> value_out;      // +80
    FadeTimer timer2;          // +84
    be_f32 s_from;             // +104
    be_f32 s_to;               // +108
    be_f32 strength;           // +112
    gp<be_f32> strength_out;   // +116
    pad<8> _120;
    Vec3 pos;                  // +128
    pad<4> _140;
    be_f32 w144;               // +144 (+80 points here)
    be_f32 k148, k152;         // +148, +152
    u8 enabled;                // +156
};
static_assert(offsetof(WindFade, timer) == 48 && offsetof(WindFade, timer2) == 84 && offsetof(WindFade, pos) == 128 &&
              offsetof(WindFade, enabled) == 156);

struct WindArea {
    pad<128> _0;
    PtrVec<WindFade> active;    // +128
    pad<8> _140;
    PtrVec<WindFade> inactive;  // +148
    pad<12> _160;
    u8 on;                      // +172
};
static_assert(offsetof(WindArea, inactive) == 148 && offsetof(WindArea, on) == 172);

namespace {
constexpr u32 FadeSetState34 = 0x004A9130u;   // Fade_SetState_004A9130(fade, state)
constexpr u32 HkRelease34 = 0x007D4EA8u;      // AnimSkel_Release7D4EA8(ref)
constexpr u32 HkAddRef34 = 0x007D48C4u;       // Hk_AddRef7D48C4(ref)
constexpr u32 HkRemove34 = 0x00933120u;       // f_00933120(world, entity)
constexpr u32 kStrBadWind34 = 0x00AEDCE4u;    // "invalid wind state %h"
using FadeVec = PtrVec<WindFade>;

// The inlined push_back: PtrVector_Reserve(v, cap * 2) when full, the
// element stored at the count (read from `src` after the reserve when
// `src` != 0, else `val`), the count and the high-water mark updated.
void Push34(FadeVec* v, const gp<WindFade>* src, WindFade* val) {
    const u16 cap = v->cap;
    u32 c = v->count;
    if (c == cap) {
        call(fn::PtrVector_Reserve, v, (static_cast<u32>(cap) << 1) & 0x1FFFEu);
        c = v->count;
    }
    gp<WindFade>* d = v->data;
    const u32 n = v->count + 1u;
    const u32 w = src != nullptr ? src->ea.get() : guest(val);
    const u16 mx = v->mark;
    gm<be_u32>(guest(d) + c * 4u) = w;
    v->count = static_cast<u16>(n);
    v->mark = static_cast<u16>(mx > static_cast<u16>(n) ? mx : n);
}

// The count - 1 when it went negative: the original's extra copy
// data[c] = data[c & 0xFFFF] (guest address arithmetic, wrapping).
void FixNegCount34(FadeVec* v, u32 c) {
    if (static_cast<s32>(c) != static_cast<s32>(c & 0xFFFFu)) {
        const u32 d2 = v->data.ea;
        gm<be_u32>(d2 + c * 4u) = gm<be_u32>(d2 + (c & 0xFFFFu) * 4u).get();
    }
}

// The inlined swap-remove of entry i (data `d` as the caller read it):
// the entry at guest address `last` moves into its place and the count
// (re-read) drops by one. Returns the removed element.
WindFade* SwapRemoveAt34(FadeVec* v, u32 d, u32 i, u32 last) {
    be_u32& p = gm<be_u32>(d + i * 4u);
    const u32 c = v->count - 1u;
    const u32 obj = p;
    const u32 lv = gm<be_u32>(last);
    v->count = static_cast<u16>(c);
    p = lv;
    FixNegCount34(v, c);
    return host<WindFade>(obj);
}

WindFade* FadeAt(u32 d, u32 i) { return host<WindFade>(gm<be_u32>(d + i * 4u)); }
void Remove34(WindFade* f) { call(HkRemove34, f->ent->world.get(), f); }
}  // namespace

// Area_SetOn(area, u8 on): on: every inactive fade object (expected in
// state 3, else "invalid wind state %h" is printed) gets Fade_SetState(1)
// and is appended to the active list, then the inactive list is emptied;
// off: the same from the active list (states 1 / 2 expected) with
// Fade_SetState(3) into the inactive one. +172 = on.
void WindArea_SetOn4AD580(WindArea* a, u8 on) {
    FadeVec* from = on ? &a->inactive : &a->active;
    FadeVec* to = on ? &a->active : &a->inactive;
    for (u32 i = 0;; ++i) {
        const u32 n = from->count;
        if (i >= n) break;
        u32 d = from->data.ea;
        const s32 st = FadeAt(d, i)->state;
        if (on ? st != 3 : !(st == 1 || st == 2)) {
            call(fn::String_Format, kStrBadWind34, st);
            d = from->data.ea;
        }
        call(FadeSetState34, FadeAt(d, i), on ? 1u : 3u);
        Push34(to, &gm<gp<WindFade>>(from->data.ea + i * 4u), nullptr);
    }
    from->count = 0;
    a->on = on;
}

// Area_Move(area, id, new_area): the fade object with id `id` (active
// list first, then inactive) taken out of `area` and made active in
// `new_area`: its Havok reference released, +32 = new_area (AddRef),
// Fade_SetState(1), appended to new_area's active list. Quirk kept: when
// it was inactive, the entry moved into its place is the one at the
// active list's count - 1.
void WindArea_Move4ADAF8(WindArea* a, u32 id, WindArea* na) {
    const u32 ca = a->active.count;
    const u32 da = a->active.data.ea;
    WindFade* obj = nullptr;
    bool found = false;
    if (ca != 0) {
        for (u32 i = 0; i < ca; ++i)
            if (FadeAt(da, i)->owner.ea == id) {
                obj = SwapRemoveAt34(&a->active, da, i, da + (ca - 1u) * 4u);
                found = true;
                break;
            }
    }
    if (!found) {
        for (u32 j = 0; j < a->inactive.count; ++j) {
            const u32 db = a->inactive.data.ea;
            if (FadeAt(db, j)->owner.ea == id) {
                obj = SwapRemoveAt34(&a->inactive, db, j, db + (ca - 1u) * 4u);  // quirk: the active count
                found = true;
                break;
            }
        }
    }
    if (!found) return;
    call(HkRelease34, obj->area.get());
    obj->area = guest(na);
    call(HkAddRef34, na);
    call(FadeSetState34, obj, 1u);
    Push34(&na->active, nullptr, obj);
}

// Area_Remove(area, id, u8 remove) (callers Obj_UpdateArea_00224BC8 /
// Obj_LeaveArea_00224F48 / ...): the fade object with id `id` taken out
// of its list. Active and `remove` clear: Fade_SetState(3) and appended
// to the inactive list; otherwise f_00933120(its +24's +8, it).
void WindArea_Remove4AE160(WindArea* a, u32 id, u8 rm) {
    const u32 ca = a->active.count;
    const u32 da = a->active.data.ea;
    if (ca != 0) {
        for (u32 i = 0; i < ca; ++i)
            if (FadeAt(da, i)->owner.ea == id) {
                WindFade* obj = SwapRemoveAt34(&a->active, da, i, da + (ca - 1u) * 4u);
                if (rm != 0) {
                    Remove34(obj);
                } else {
                    call(FadeSetState34, obj, 3u);
                    Push34(&a->inactive, nullptr, obj);
                }
                return;
            }
    }
    for (u32 j = 0;; ++j) {
        const u32 cb = a->inactive.count;
        if (j >= cb) return;
        const u32 db = a->inactive.data.ea;
        if (FadeAt(db, j)->owner.ea == id) {
            Remove34(SwapRemoveAt34(&a->inactive, db, j, db + (cb - 1u) * 4u));
            return;
        }
    }
}

namespace {
// The swap-remove as Area_Update inlines it: data from the iteration,
// `last` from the count the loop compare read; `p_first`: the entry's
// store before the count's (else after).
void SwapRemoveU34(FadeVec* v, u32 d, u32 off, u32 n, bool p_first) {
    be_u32& p = gm<be_u32>(d + off);
    const u32 c = v->count - 1u;
    const u32 lv = gm<be_u32>(d + (n - 1u) * 4u);
    if (p_first) p = lv;
    v->count = static_cast<u16>(c);
    if (!p_first) p = lv;
    FixNegCount34(v, c);
}

// The out record of the owner's vfunc 140.
struct OwnerSlot140 {
    be_u32 w0;     // +0
    be_f32 level;  // +4
};
}  // namespace

void Fade_Update4AC820(WindFade* f, f32 dt);

// Area_Update(area, dt): the inactive fade objects first: one whose owner
// (+28) lacks bit 0x40000 of its +172 is removed (swap-remove,
// f_00933120(its +24's +8, it)); the others get Fade_Update(f, dt); one
// left in state 0 whose level (+132) did not rise is removed too unless
// its owner's slot 140 (out, owner) gives a second word (out + 4) below
// the constant at 0x004AD7C4; states other than 0 / 3 print "invalid wind
// state %h". Then the active ones: no owner bit -> removed; else
// Fade_Update(f, dt), states other than 1 / 2 printed.
void WindArea_Update4AD7C8(WindArea* a, f32 dt) {
    GuestFrame fr(32);
    OwnerSlot140* out = fr.as<OwnerSlot140>();
    const f64 kmin = lf(global<be_f32>(0x004AD7C4u));
    s32 i = 0;
    u32 off = 0;
    for (;; ++i, off += 4) {
        const u32 n = a->inactive.count;
        if (!(i < static_cast<s32>(n))) break;
        const u32 d = a->inactive.data.ea;
        WindFade* f = host<WindFade>(gm<be_u32>(d + off));
        if ((f->owner->flags & 0x40000u) == 0) {
            SwapRemoveU34(&a->inactive, d, off, n, true);
            Remove34(f);
            off -= 4;
            --i;
            continue;
        }
        const f64 lv0 = lf(f->pos.y);
        Fade_Update4AC820(f, dt);
        f = host<WindFade>(gm<be_u32>(a->inactive.data.ea + off));
        const s32 st = f->state;
        if (st != 0) {
            if (st != 3) call(fn::String_Format, kStrBadWind34, st);
            continue;
        }
        if (lf(f->pos.y) > lv0) continue;
        FadeOwner* q = f->owner;
        call_ptr(vfn(q, 140), out, q);
        const f64 lv = lf(out->level);
        const u32 n2 = a->inactive.count;
        const u32 d2 = a->inactive.data.ea;
        if (lv < kmin) continue;
        SwapRemoveU34(&a->inactive, d2, off, n2, false);
        Remove34(f);
        off -= 4;
        --i;
    }
    i = 0;
    off = 0;
    for (;; ++i, off += 4) {
        const u32 n = a->active.count;
        if (!(i < static_cast<s32>(n))) break;
        const u32 d = a->active.data.ea;
        WindFade* f = host<WindFade>(gm<be_u32>(d + off));
        if ((f->owner->flags & 0x40000u) == 0) {
            SwapRemoveU34(&a->active, d, off, n, true);
            Remove34(f);
            off -= 4;
            --i;
            continue;
        }
        Fade_Update4AC820(f, dt);
        WindFade* f2 = host<WindFade>(gm<be_u32>(a->active.data.ea + off));
        const s32 st = f2->state;
        if (st != 1 && st != 2) call(fn::String_Format, kStrBadWind34, st);
    }
}

namespace {
constexpr u32 TimerAdvance34 = 0x004A0990u;   // Timer_Advance4A0990(timer, float dt)
constexpr u32 kFadeK34 = 0x004AC810u;         // f32 1, f32 0, f64 0 (the fade constants)

// The fade's +48 timer stepped (dt) and its +76 value = (1 - t) * +68 +
// t * +72 with t = +60 (copied to *+80 when set); then the end test: +56
// below 0, or (no +64 >> 1 count) +52 >= +56 -> Fade_SetState(f, next).
void FadeStep34(WindFade* f, f32 dt, u32 next) {
    call(TimerAdvance34, &f->timer, dt);
    const f64 t = lf(f->timer.t);
    const f64 b = Muls(t, lf(f->to));
    const f64 a = lf(f->from);
    const f64 v = Fmadds(Subs(lf(global<be_f32>(kFadeK34)), t), a, b);
    f->value = static_cast<f32>(v);
    if (be_f32* p = f->value_out) *p = static_cast<f32>(v);
    const f64 x = lf(f->timer.limit);
    if (x < lf(global<be_f32>(kFadeK34 + 4))) {
        call(FadeSetState34, f, next);
        return;
    }
    const u32 cnt = (f->timer.flags >> 1) & 0x7FFFu;
    if (cnt != 0) return;
    const f64 y = lf(f->timer.elapsed);
    if (y > x || y == x) call(FadeSetState34, f, next);
}
}  // namespace

// Fade_Update(fade, dt) (callee of Area_Update): while the owner (+28)
// is live (+172 bit 0x40000, +188 bit 0) and its slot 64 gives something
// and the fade is enabled (+156): the strength (+112 -> +104) restarts
// when the owner's slot-64 object's +416 mode (bits 28-30) turned 2
// (fade in from +112, +108 = 1) or stopped being 2 (+108 = 0), with +92 =
// +148 * the distance and +88 = it clamped to 0 / the f64 at 0x004AC818
// (fsel); then the +84 timer stepped and +112 = (1 - t) * +104 + t * +108
// (t = +96; to *+116 too). The owner's position (+144) copied to +128;
// state 1 / 3: FadeStep to 2 / 0.
void Fade_Update4AC820(WindFade* f, f32 dt) {
    FadeOwner* q = f->owner;
    if (q != nullptr && (q->flags & 0x40000u) != 0 && (q->mask_lo & 1u) != 0) {
        const u32 r = vcall<u32>(q, 64);
        q = f->owner;
        if (r != 0 && f->enabled != 0) {
            const f64 one = lf(global<be_f32>(kFadeK34));
            const bool full = lf(f->s_to) == one;
            FadeMode* m = vcall<FadeMode*>(q, 64);
            const u32 mode = (m->mode >> 28) & 7u;
            const f64 s = lf(f->strength), k = lf(f->k148);
            const bool in = mode == 2;
            const f64 dmax = global<be_f64>(kFadeK34 + 8).get();
            if (in && !full) {
                const f64 d = Subs(one, s);
                f->s_from = f->strength;  // lfs / stfs: raw word
                set_bits(f->s_to, 0x3F800000u);
                const f64 g = Muls(k, d);
                f->timer2.limit = static_cast<f32>(g);
                const f64 z = lf(global<be_f32>(kFadeK34 + 4));
                const f64 c = Fsel(Fsel(Subs(g, z), z, g), Fsel(Subs(g, z), z, g), dmax);
                f->timer2.elapsed = static_cast<f32>(S(c));
            } else if (!in && full) {
                const f64 g = Muls(k, s);
                f->s_from = f->strength;
                const f64 z = lf(global<be_f32>(kFadeK34 + 4));
                set_bits(f->s_to, 0);
                f->timer2.limit = static_cast<f32>(g);
                const f64 c = Fsel(Fsel(Subs(g, z), z, g), Fsel(Subs(g, z), z, g), dmax);
                f->timer2.elapsed = static_cast<f32>(S(c));
            }
            call(TimerAdvance34, &f->timer2, dt);
            const f64 t = lf(f->timer2.t);
            const f64 b = Muls(t, lf(f->s_to));
            const f64 v = Fmadds(Subs(one, t), lf(f->s_from), b);
            f->strength = static_cast<f32>(v);
            if (be_f32* p = f->strength_out) *p = static_cast<f32>(v);
        }
    }
    const FadeOwner& qq = gm<FadeOwner>(f->owner.ea);  // read even when null (address 144)
    const s32 st = f->state;
    f->pos.x = qq.pos.x;  // raw words
    f->pos.y = qq.pos.y;
    f->pos.z = qq.pos.z;
    if (st == 1) FadeStep34(f, dt, 2);
    else if (st == 3) FadeStep34(f, dt, 0);
}

namespace {
constexpr u32 HkEntityCtor34 = 0x008FFEA0u;   // f_008FFEA0(entity, world, 0)
constexpr u32 kFadeVt34 = 0x00B21BC0u;
constexpr u32 kFadeCbVt34 = 0x00B21B88u;
constexpr u32 kShapeK34 = 0x004A9260u;        // f32 / f64 0, then the u64 mask or'ed into the owner's +184
}  // namespace

// Fade_Construct(fade, owner, world, area, float a1 .. a4, u8 enabled)
// (Hk_ShapeInit; callee of Hk_NewFromSrc_004AC718 / Area_Test_004ADD68):
// the Havok entity base (f_008FFEA0(fade, world, 0)), +28 owner, +32 area
// (AddRef), +36 / +40 / +148 / +152 = a1 / a2 / a3 / a4, +156 enabled,
// two embedded timers (+48 / +84, vtable 0x00B21B88; their +64 / +100
// words keep only bit 0 and get +67 / +103 = 1) cleared, +128 = the zero
// vector, +80 = &+144. The owner's +178 count incremented and its +184
// or'ed with the u64 at 0x004A9268; +88 = the constant (fsel'ed against
// itself), +128 = the owner's position, then Fade_SetState(fade, 1).
void Fade_Construct4A9270(WindFade* o, FadeOwner* q, u32 world, u32 area, f32 a1, f32 a2, f32 a3, f32 a4, u8 flag) {
    call(HkEntityCtor34, o, world, 0u);
    o->owner = q;
    o->area = area;
    o->a36 = a1;
    const u32 w64 = o->timer.flags & 1u;
    o->vtable = kFadeVt34;
    o->a40 = a2;
    o->state = 0;
    const f64 k = lf(global<be_f32>(kShapeK34));
    set_bits(o->timer.elapsed, 0);
    set_bits(o->timer.limit, 0);
    o->timer.flags_hi() = 0;
    set_bits(o->timer.t, 0);
    const u32 w100 = o->timer2.flags & 1u;
    o->timer.flags = w64;
    o->timer.flags_lo() = static_cast<u8>(w64 | 1u);
    set_bits(o->from, 0);
    set_bits(o->to, 0);
    set_bits(o->value, 0);
    set_bits(o->timer2.elapsed, 0);
    set_bits(o->timer2.limit, 0);
    o->timer2.flags_hi() = 0;
    set_bits(o->timer2.t, 0);
    o->timer2.flags = w100;
    o->timer.vtable = kFadeCbVt34;
    o->timer2.flags_lo() = static_cast<u8>(w100 | 1u);
    set_bits(o->s_from, 0);
    set_bits(o->s_to, 0);
    o->value_out = nullptr;
    set_bits(o->strength, 0);
    o->timer2.vtable = kFadeCbVt34;
    const Vec3& z = global<Vec3>(kZeroVec);
    o->pos.x = z.x;  // lfs / stfs: raw words
    o->strength_out = nullptr;
    o->pos.y = z.y;
    set_bits(o->w144, 0);
    o->pos.z = z.z;
    o->k148 = a3;
    o->k152 = a4;
    o->enabled = flag;
    call(HkAddRef34, area);
    FadeOwner* q2 = o->owner;
    const f64 d = Subs(k, k);
    o->value_out = &o->w144;
    const u64 m = (static_cast<u64>(q2->mask_hi) << 32) | q2->mask_lo;
    const u16 c = static_cast<u16>(q2->fades + 1);
    const f64 s = Fsel(d, k, k);
    q2->fades = c;
    const u64 mask = global<be_u64>(kShapeK34 + 8);
    set_bits(o->s_from, 0);
    set_bits(o->s_to, 0);
    set_bits(o->timer2.limit, 0);
    q2->mask_hi = static_cast<u32>((m | mask) >> 32);
    q2->mask_lo = static_cast<u32>(m | mask);
    const f64 kd = global<be_f64>(kShapeK34).get();
    o->timer2.elapsed = static_cast<f32>(S(Fsel(s, s, kd)));
    o->pos.x = q->pos.x;
    o->pos.y = q->pos.y;
    o->pos.z = q->pos.z;
    call(FadeSetState34, o, 1u);
}

}  // namespace sly4::obj

NATIVE(004AD580, sly4::obj::WindArea_SetOn4AD580)
NATIVE(004ADAF8, sly4::obj::WindArea_Move4ADAF8)
NATIVE(004AE160, sly4::obj::WindArea_Remove4AE160)
NATIVE(004AD7C8, sly4::obj::WindArea_Update4AD7C8)
NATIVE(004AC820, sly4::obj::Fade_Update4AC820)
NATIVE(004A9270, sly4::obj::Fade_Construct4A9270)
