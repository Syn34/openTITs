// Game code, medium sweep part 34: per-frame updates of the scripted
// animation actors (vtable methods Update(o, float dt)).
//
// The actors: +20 effect set, +24 the world object, +28 the animation
// (Anim_Advance34C918 / Obj_Anim353EB0_004A5D40), +32 / +144 two offsets
// added to the zero vector at 0x01164B40 for the world object's position,
// +160 u32 state << 1 | flag.
#include "mid34.h"

namespace sly4::obj {

// A placed world object: four matrix rows at +16 (+64: the position),
// +164 flags (bit 0x8000: live).
struct ActorWorldObj {
    pad<16> _0;
    Quad rows[4];    // +16 .. +79 (rows[3] = the position)
    pad<84> _80;
    be_u32 flags;    // +164
};
static_assert(offsetof(ActorWorldObj, flags) == 164);

struct AnimActor {
    pad<20> _0;
    be_u32 effects;            // +20: effect set
    gp<ActorWorldObj> wo;      // +24
    be_u32 anim;               // +28
    Vec3 off1;                 // +32
    pad<100> _44;
    Vec3 off2;                 // +144
    pad<4> _156;
    be_u32 state;              // +160: state << 1 | flag
};
static_assert(offsetof(AnimActor, off2) == 144 && sizeof(AnimActor) == 164);

struct ActorTwoPart {
    AnimActor a;
    pad<32> _164;
    be_u32 id196;              // +196
    pad<136> _200;
    be_u32 saved[4][4];        // +336: the world object's rows (3 words each)
    u8 saved_flag;             // +400
};
static_assert(offsetof(ActorTwoPart, saved) == 336 && offsetof(ActorTwoPart, saved_flag) == 400);

struct ActorOneShot {
    AnimActor a;
    pad<8> _164;
    be_u32 id172;              // +172
    pad<8> _176;
    be_u32 on;                 // +184: bit 31
};
static_assert(offsetof(ActorOneShot, on) == 184);

struct ActorRegistered {
    AnimActor a;
    pad<48> _164;
    u8 flag212;                // +212
};
static_assert(offsetof(ActorRegistered, flag212) == 212);

struct WatchedObj {
    pad<172> _0;
    be_u32 flags;              // +172: bit 0x40000 = live
};
struct WatchEntry {
    pad<4> _0;
    gp<WatchedObj> obj;        // +4
    pad<4> _8;
};
static_assert(sizeof(WatchEntry) == 12);
struct ActorWatch {
    AnimActor a;
    gp<WatchEntry> list;       // +164
    pad<4> _168;
    be_u16 count;              // +172
    pad<18> _174;
    be_u32 id192;              // +192
    be_u32 id196;              // +196
};
static_assert(offsetof(ActorWatch, count) == 172 && offsetof(ActorWatch, id192) == 192);

struct LevelSrc {
    pad<192> _0;
    be_u32 flags;              // +192: bit 2
    pad<148> _196;
    be_f32 max;                // +344
    be_f32 cur;                // +348
};
static_assert(offsetof(LevelSrc, max) == 344);
struct LevelSrcHolder {
    pad<8> _0;
    gp<LevelSrc> src;          // +8
};
struct ActorGauge {
    AnimActor a;
    gp<LevelSrcHolder> holder; // +164
    gp<LevelSrc> src;          // +168
    u8 flags172;               // +172: 0x80 = use +168
    pad<11> _173;
    be_u32 id184;              // +184
    be_u32 id188;              // +188
    pad<28> _192;
    be_f32 delay;              // +220
    be_f32 dur;                // +224
    be_f32 timer;              // +228
    be_f32 from;               // +232
    be_f32 to;                 // +236
    be_f32 target;             // +240
};
static_assert(offsetof(ActorGauge, delay) == 220 && offsetof(ActorGauge, target) == 240);

struct ActorCue {
    AnimActor a;
    pad<4> _164;
    be_u32 id168;              // +168
    pad<4> _172;
    be_u32 objs[3];            // +176 / +180 / +184
    pad<8> _188;
    be_u32 id196;              // +196
    be_u32 sound;              // +200
    be_s32 phase;              // +204
};
static_assert(offsetof(ActorCue, objs) == 176 && offsetof(ActorCue, phase) == 204);

struct CuePartnerEntry {
    pad<4> _0;
    be_u32 flags;              // +4: bit 2
};
struct PlayerRecord {
    pad<8> _0;
    be_u32 a;                  // +8
    be_u32 b;                  // +12
};
struct GameWorld4208 {
    pad<4209> _0;
    u8 player;                 // +4209
};

namespace {
constexpr u32 AnimAdvance34 = 0x0034C918u;    // Anim_Advance34C918(anim, float dt)
constexpr u32 AnimDone34 = 0x004A5D40u;       // Obj_Anim353EB0_004A5D40(anim, 0) -> finished
constexpr u32 WorldTest34 = 0x0046C7B0u;      // Global_Test10C5A20_0046C7B0(world)
constexpr u32 MarkDirty34 = 0x0057D000u;      // WorldObj_MarkDirty(wo)
constexpr u32 SayId34 = 0x004A5C90u;          // Speaker_SayId4A5C90(anim, id, loop, float k)
constexpr u32 WorldRegister34 = 0x0046B5F8u;  // f_0046B5F8(world, kind, OPD, o, 0)
constexpr u32 kHashStart34 = 0x002237D4u;
constexpr u32 kHashEnd34 = 0x048129EBu;
constexpr u32 kHashOver34 = 0xA464660Eu;
constexpr u32 SetLevel34 = 0x00364C90u;       // f_00364C90(o, float level)
constexpr u32 SoundPlay34 = 0x0044C474u;      // Sound_Play44C474(out, desc, 0 x 6; stack 0, 0, 0)
constexpr u32 MapFind34 = 0x0076611Cu;        // SortedMap_Find3(map, const u32* key) -> value slot
constexpr u32 StateEnter6_34 = 0x0048970Cu;   // State_Enter6_0048970C(world)
constexpr u32 ActorRelease34 = 0x004A82DCu;   // f_004A82DC(actor)

u32 World34() { return global<be_u32>(kWorld); }
bool AnimDone(const AnimActor* o) { return call<u32>(AnimDone34, o->anim.get(), 0u) != 0; }

// The shared end of the updates: Anim_Advance(+28, dt) when there is an
// animation; then, when the world object at +24 has bit 0x8000 of its
// +164, the offset at +144 is not the zero vector (compared as words) and
// the world test passes, its position (+64) = zero + (+144) + (+32)
// (fadds) and WorldObj_MarkDirty(it).
void Tail34(AnimActor* o, f32 dt) {
    if (const u32 a = o->anim) call(AnimAdvance34, a, dt);
    ActorWorldObj* t = o->wo;
    if (t == nullptr) return;
    if (((t->flags >> 15) & 1u) == 0) return;
    const Vec3& z = global<Vec3>(kZeroVec);
    if (bits(o->off2.x) == bits(z.x) && bits(o->off2.y) == bits(z.y) && bits(o->off2.z) == bits(z.z)) return;
    if (call<u32>(WorldTest34, World34()) == 0) return;
    const be_f32* zs[3] = {&z.x, &z.y, &z.z};
    const be_f32* as[3] = {&o->off2.x, &o->off2.y, &o->off2.z};
    const be_f32* bs[3] = {&o->off1.x, &o->off1.y, &o->off1.z};
    f32 v[3];
    for (u32 k = 0; k < 3; ++k) v[k] = static_cast<f32>(Adds(lf(*zs[k]), lf(*as[k])) + lf(*bs[k]));
    ActorWorldObj* w = o->wo;
    for (u32 k = 0; k < 3; ++k) set_bits(w->rows[3].w[k], std::bit_cast<u32>(v[k]));
    call(MarkDirty34, w);
}

// The +160 state set to `s`, keeping the flag bit.
void SetState34(AnimActor* o, u32 s) { o->state = (o->state & 1u) | (s << 1); }

// A state change with a line: the state set (flag kept), Speaker_SayId(
// +28, id, loop, the f32 at kaddr), then `hash` on the effect set when
// there is one.
void Start34(AnimActor* o, u32 state, const be_u32& id, u32 loop, u32 kaddr, u32 hash) {
    SetState34(o, state);
    call(SayId34, o->anim.get(), id.get(), loop, lf(global<be_f32>(kaddr)));
    if (const u32 e = o->effects) call(fn::Effect_SetHash, e, hash);
}
}  // namespace

// Update(o, dt) of a two-part actor: state 1 -> 2 (SayId(+196, no loop))
// when its animation ends; state 2 (once, +400 clear): the world object's
// four matrix rows (+16 ...) saved at +336 (+400 = 1); state 3: when the
// animation ends, the world object's slot 40 (wo, 0, 1) and back to state
// 0. Then the shared end (state 0: nothing).
void AnimActor_Update36DED8(ActorTwoPart* o, f32 dt) {
    const u32 s = o->a.state >> 1;
    if (s == 0) return;
    if (s == 1) {
        if (AnimDone(&o->a)) Start34(&o->a, 2, o->id196, 0, 0x0036DED0u, kHashStart34);
    } else if (s == 2) {
        ActorWorldObj* t = o->a.wo;
        if (t != nullptr && o->saved_flag == 0) {
            for (u32 r = 0; r < 4; ++r)
                for (u32 k = 0; k < 3; ++k) {
                    if (r == 3 && k == 2) o->saved_flag = 1;
                    o->saved[r][k] = t->rows[r].w[k].get();
                }
        }
    } else if (s == 3) {
        if (AnimDone(&o->a)) {
            if (ActorWorldObj* t = o->a.wo) vcall(t, 40, 0u, 1u);
            o->a.state = o->a.state & 1u;
        }
    }
    Tail34(&o->a, dt);
}

// Update(o, dt) of a one-shot actor (only while +184 has bit 0x80000000):
// state 1 -> 2 (SayId(+172, looped)) when its animation ends; state 3: the
// actor's slot 12 when it ends. Then the shared end.
void AnimActor_Update36ED28(ActorOneShot* o, f32 dt) {
    if ((o->on >> 31) == 0) return;
    const u32 s = o->a.state >> 1;
    if (s == 0) return;
    if (s == 1) {
        if (AnimDone(&o->a)) Start34(&o->a, 2, o->id172, 1, 0x0036ED24u, kHashStart34);
    } else if (s == 3) {
        if (AnimDone(&o->a)) vcall(o, 12);
    }
    Tail34(&o->a, dt);
}

// Update(o, dt) of a registered actor: f_0046B5F8(world, 9, 0x00F378B8,
// o, 0) first; state 1 -> 2 when the animation ends; state 2 -> 3 with the
// "start" hash; state 4: when the animation ends, the actor's slot 12
// (with +212 cleared around the call and set to 1 after it when it was
// set). Then the shared end (state 0: nothing).
void AnimActor_Update36C5BC(ActorRegistered* o, f32 dt) {
    u32 s = o->a.state >> 1;
    if (s == 0) return;
    call(WorldRegister34, World34(), 9u, 0x00F378B8u, o, 0u);
    s = o->a.state >> 1;
    if (s == 4) {
        if (AnimDone(&o->a)) {
            if (o->flag212 != 0) {
                o->flag212 = 0;
                vcall(o, 12);
                o->flag212 = 1;
            } else {
                vcall(o, 12);
            }
        }
    } else if (s == 2) {
        SetState34(&o->a, 3);
        if (const u32 e = o->a.effects) call(fn::Effect_SetHash, e, kHashStart34);
    } else if (s == 1) {
        if (AnimDone(&o->a)) SetState34(&o->a, 2);
    }
    Tail34(&o->a, dt);
}

// Update(o, dt) of an actor with a list of watched objects (only while its
// +160 flag is set): f_0046B5F8(world, 7 / 8, ...) first; then the
// 12-byte entries {+4 object} of the u16-counted list at +164 / +172 whose
// object lacks bit 0x40000 of its +172 are removed (the last entry's
// first 9 bytes copied over it unless it is the last; the same index
// looked at again). State 1 -> 2 (SayId(+192)) and 2 -> 3 (SayId(+196),
// the "end" hash) when the animation ends; state 3: slot 12 when it ends.
// Then the shared end.
void AnimActor_Update3704E8(ActorWatch* o, f32 dt) {
    const u32 x = o->a.state;
    if ((x & 1u) == 0) return;
    if ((x >> 1) == 0) return;
    call(WorldRegister34, World34(), 7u, 0x00F379E0u, o, 0u);
    call(WorldRegister34, World34(), 8u, 0x00F379E8u, o, 0u);
    u32 n = o->count;
    for (s32 i = 0; i < static_cast<s32>(n); ++i) {
        WatchEntry* arr = o->list;
        WatchEntry* p = arr + i;
        WatchedObj* e = p->obj;
        if (e == nullptr || (e->flags & 0x40000u) != 0) continue;
        const u16 c = static_cast<u16>(o->count - 1);
        o->count = c;
        n = c;
        if (static_cast<s32>(i) != static_cast<s32>(n)) {
            call(fn::Memmove, p, arr + n, 9u);
            n = o->count;
        }
        --i;
    }
    const u32 s = o->a.state >> 1;
    if (s == 1) {
        if (AnimDone(&o->a)) Start34(&o->a, 2, o->id192, 0, 0x003704E0u, kHashStart34);
    } else if (s == 2) {
        if (AnimDone(&o->a)) Start34(&o->a, 3, o->id196, 0, 0x003704E0u, kHashEnd34);
    } else if (s == 3) {
        if (AnimDone(&o->a)) vcall(o, 12);
    }
    Tail34(&o->a, dt);
}

// Update(o, dt) of a gauge actor. State 1 -> 2 (SayId(+184)) when the
// animation ends; state 3: slot 20 (o, 0) and back to 0 when it ends.
// State 2: the source (+168 when bit 0x80 of +172, else the +8 object of
// +164 when its +192 has bit 2) gives the target level +348 / +344
// (fdivs); no source: slot 20 (o, 0). Above the old target (+240) the
// "over" hash. While the +220 delay runs it counts down by dt (to 0 at
// the least, the constant at 0x0036D348); else the +228 timer counts down
// and the level (+232 -> +236 over +224 seconds) goes to f_00364C90(o,
// level); at the end (level <= 0) slot 20 (o, 1), state 3 with SayId(+188)
// and the "end" hash, unless the state is already 0 / 3. A changed target
// restarts the timer from +224 (from +240 to the new target), or, when
// +224 is not positive, sets the level at once (+228 = 0, the same end
// test); +240 = the target. Then the shared end.
void AnimActor_Update36D350(ActorGauge* o, f32 dt) {
    const u32 s = o->a.state >> 1;
    if (s == 0) return;
    constexpr u32 kZero = 0x0036D348u, kLine = 0x0036D34Cu;
    const f64 fdt = dt;
    auto finish = [&]() {  // the end test after a level change
        const u32 st = o->a.state >> 1;
        if (st == 0 || st == 3) return;
        vcall(o, 20, 1u);
        Start34(&o->a, 3, o->id188, 0, kLine, kHashEnd34);
    };
    if (s == 1) {
        if (AnimDone(&o->a)) Start34(&o->a, 2, o->id184, 0, kLine, kHashStart34);
    } else if (s == 3) {
        if (AnimDone(&o->a)) {
            vcall(o, 20, 0u);
            o->a.state = o->a.state & 1u;
        }
    } else if (s == 2) {
        LevelSrc* q;
        if (o->flags172 & 0x80u) {
            q = o->src;
        } else {
            q = nullptr;
            LevelSrc* c = o->holder->src;
            if (c != nullptr && (c->flags & 2u) != 0) q = c;
        }
        if (q == nullptr) {
            vcall(o, 20, 0u);
        } else {
            const f64 tgt = Divs(lf(q->cur), lf(q->max));
            if (lf(o->target) > tgt) {
                if (const u32 e = o->a.effects) call(fn::Effect_SetHash, e, kHashOver34);
            }
            const f64 zero = lf(global<be_f32>(kZero));
            const f64 d = lf(o->delay);
            if (d > zero) {
                const f64 v = Subs(d, fdt);
                o->delay = static_cast<f32>(v > zero ? v : zero);
            } else {
                const f64 tm = lf(o->timer);
                if (tm > zero) {
                    const f64 v0 = Subs(tm, fdt);
                    const f64 v = v0 > zero ? v0 : zero;
                    const f64 frac = Divs(v, lf(o->dur));
                    const f64 to = lf(o->to);
                    const f64 from = lf(o->from);
                    o->timer = static_cast<f32>(v);
                    const f64 lv = Fmadds(Subs(from, to), frac, to);
                    call(SetLevel34, o, lv);
                    if (lv < zero || lv == zero) finish();
                }
                const f64 old = lf(o->target);
                if (old != tgt) {
                    if (old > tgt) {
                        const f64 dur = lf(o->dur);
                        if (dur > zero) {
                            o->timer = o->dur;  // lfs / stfs: raw words
                            o->from = o->target;
                            o->to = static_cast<f32>(tgt);
                        } else {
                            o->timer = 0.0f;
                            call(SetLevel34, o, tgt);
                            if (tgt < zero || tgt == zero) finish();
                        }
                    } else {
                        o->timer = 0.0f;
                        call(SetLevel34, o, tgt);
                    }
                    o->target = static_cast<f32>(tgt);
                }
            }
        }
    }
    Tail34(&o->a, dt);
}

// Update(o, dt) of a cue actor: f_0046B5F8(world, 8, 0x00F379A0, o, 0)
// first; the partner is the actor the map at 0x0106C228 gives for the id
// at 0x00F64CF0 (when its +4 has bit 2). The +204 phase: 0 -> 1 once bit
// 0x80 of the player record's +8 & +12 is set (the cue sound (+200)
// played; a partner not in state 1 / 3 gets its world object's slot 40
// (wo, 1, 1), state 1 with SayId(+168) and its hash, then
// f_004A82DC(partner)); 1 -> 2 when the partner's animation ends (or
// there is no partner): State_Enter6(world) and slot 20 (o, 0). State 1
// -> 2 (SayId(+196, looped), the three +176 / +180 / +184 objects' slot 88
// (x, 1)) when the animation ends. Then the shared end.
void AnimActor_Update36F438(ActorCue* o, f32 dt) {
    const u32 s = o->a.state >> 1;
    if (s == 0) return;
    GuestFrame f(32);
    be_u32* out = f.as<be_u32>(0);   // Sound_Play's out record
    be_u32* key = f.as<be_u32>(16);
    call(WorldRegister34, World34(), 8u, 0x00F379A0u, o, 0u);
    const u32 kv = global<be_u32>(0x00F64CF0u);
    *key = kv;
    ActorCue* p = nullptr;
    if (kv != 0) {
        gp<CuePartnerEntry>* slot = call<gp<CuePartnerEntry>*>(MapFind34, 0x0106C228u, key);
        CuePartnerEntry* c = *slot;
        if (c != nullptr && (c->flags & 2u) != 0) p = reinterpret_cast<ActorCue*>(c);
    }
    const s32 m = o->phase;
    if (m == 0) {
        const GameWorld4208* w = host<GameWorld4208>(World34());
        const PlayerRecord& rec = gm<PlayerRecord>(0x010B19E0u + static_cast<u32>(w->player) * 11968u + 176u);
        if ((rec.a & rec.b) & 0x80u) {
            call(SoundPlay34, out, o->sound.get(), 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
            if (p != nullptr) {
                const u32 ps = p->a.state >> 1;
                if (ps != 1 && ps != 3) {
                    if (ActorWorldObj* pt = p->a.wo) vcall(pt, 40, 1u, 1u);
                    Start34(&p->a, 1, p->id168, 0, 0x0036F434u, 0x04296E4Cu);
                }
                call(ActorRelease34, p);
            }
            o->phase = 1;
        }
    } else if (m == 1) {
        if (p == nullptr || AnimDone(&p->a)) {
            call(StateEnter6_34, World34());
            o->phase = 2;
            vcall(o, 20, 0u);
        }
    }
    if ((o->a.state >> 1) == 1 && AnimDone(&o->a)) {
        Start34(&o->a, 2, o->id196, 1, 0x0036F434u, kHashStart34);
        for (u32 k = 0; k < 3; ++k) {
            if (const u32 q = o->objs[k]) vcall(host<u8>(q), 88, 1u);
        }
    }
    Tail34(&o->a, dt);
}

}  // namespace sly4::obj

NATIVE(0036DED8, sly4::obj::AnimActor_Update36DED8)
NATIVE(0036ED28, sly4::obj::AnimActor_Update36ED28)
NATIVE(0036C5BC, sly4::obj::AnimActor_Update36C5BC)
NATIVE(003704E8, sly4::obj::AnimActor_Update3704E8)
NATIVE(0036D350, sly4::obj::AnimActor_Update36D350)
NATIVE(0036F438, sly4::obj::AnimActor_Update36F438)
