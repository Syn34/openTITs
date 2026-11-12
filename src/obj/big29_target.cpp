// Game code, large-function sweep part 29: the target collector's
// candidate test (height window, range and view cone, then a sorted
// insert into the caller's best-score table).
#include "big29.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kPoolTgt = 0x0026FD60;  // .text: +0 0.0, +4 1e-5, +8 0.5, +12 1.0, +16 -1.0, +24 f64 -1.0, +32 f64 1.0, +40 180/pi
constexpr u32 Target_GetDef = 0x000D951C;  // (Obj*) -> the target component or 0
constexpr u32 Acosf = 0x005E649C;          // f32 acosf(f32)

// The collector's tuning: +0 the table's capacity, +4 the range, +8 the
// view cone (degrees), +12 / +16 the height window, +20 the angle weight.
struct TgtTuning {
    be_s32 n;       // +0
    be_f32 range;   // +4
    be_f32 cone;    // +8
    be_f32 ymin;    // +12
    be_f32 ymax;    // +16
    be_f32 weight;  // +20
};
struct TgtSrc {
    pad<8> _0;
    be_u32 w8;  // +8
};
struct TgtCollector {
    pad<4> _0;
    gp<TgtSrc> src;  // +4
    be_u32 mode;     // +8: 3 uses the target component's offset
    be_u32 tuning;   // +12: TgtTuning*
};
// The eye: +32 the view direction, +48 the position.
struct TgtEye {
    pad<32> _0;
    Vec3 dir;   // +32
    pad<4> _44;
    Vec3 pos;   // +48
};
static_assert(offsetof(TgtEye, pos) == 48);
// The candidate's target component: +32 the aim offset, +68 its kind.
struct TgtDef {
    pad<32> _0;
    Vec3 off;     // +32
    pad<24> _44;
    be_s32 kind;  // +68
};
static_assert(offsetof(TgtDef, kind) == 68);
struct TgtObj {
    pad<96> _0;
    Mtx44 m;  // +96
};
// A table entry: the object and its score (raw).
struct TgtEntry {
    be_u32 obj;    // +0
    be_u32 score;  // +4
};

f32 Fw(const be_f32& v) { return std::bit_cast<f32>(bits(v)); }
}  // namespace

// void f(Collector* o, const Frame* eye, Obj* x, Entry table[], s32* count,
// bool keep, bool unique) (called by Targets_Collect270258, f_002704C0,
// f_002708E8, f_00270D78). The point: with o->+8 == 3 and x's target
// component d (Target_GetDef) whose +68 <= 1, d's offset +32 through x's
// matrix +96 (VMX); else x->vfn[284](&p, x). With T = o->+12 and the
// offset e = p - eye->+48: e.y within [T+12, T+16], the range |e| <= T+4
// and the angle (degrees) between e and eye->+32 (0 at |e| <= 1e-5) <=
// T+8 / 2, the score is |e| / T+4 * (1 - T+20) + angle / (T+8 / 2) * T+20.
// Without keep, x->vfn[304](x, o->+4->+8, o->+8, &score) must accept it (and may change it). Then (T+0 = the table's
// capacity n) when the score beats the last entry's, x is not already
// listed (with unique), and it beats entry i: entries i..n-2 move down
// one, entry i = {x, score} and *count = min(*count + 1, n). No caller
// reads r3 or f1 (r3.sh).
void Targets_Consider_0026FD90(TgtCollector* o, const TgtEye* eye, TgtObj* x, TgtEntry* table, be_u32* cntp, u8 keep, u8 unique) {
    GuestFrame frame(224);  // the original's frame (+128 the point, +144 the score)
    f64 px, py, pz;
    u32 d = 0;
    bool vmx = false;
    if (o->mode == 3u) {
        d = call<u32>(Target_GetDef, x);
        vmx = d != 0 && !(gm<TgtDef>(d).kind > 1);
    }
    if (vmx) {
        const TgtDef& td = gm<TgtDef>(d);
        const V4 q = Add4(Lvx(&x->m.r[3]), Row3(Lvx(&x->m.r[0]), Lvx(&x->m.r[1]), Lvx(&x->m.r[2]), Fw(td.off.x), Fw(td.off.y), Fw(td.off.z)));
        px = q.e[0], py = q.e[1], pz = q.e[2];
    } else {
        Vec3* pt = frame.as<Vec3>(128);
        call_ptr(vfn(x, 284), pt, x);  // (out, x): the object is the second argument
        px = lf(pt->x), py = lf(pt->y), pz = lf(pt->z);
    }
    const TgtTuning* t = &gm<TgtTuning>(o->tuning);
    const f64 dy = Subs(py, lf(eye->pos.y)), dx = Subs(px, lf(eye->pos.x)), dz = Subs(pz, lf(eye->pos.z));
    if (!(dy >= lf(t->ymin)) || !(dy <= lf(t->ymax))) return;
    const f64 dist = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    f64 angle = lf(global<be_f32>(kPoolTgt));
    if (dist > lf(global<be_f32>(kPoolTgt + 4))) {
        const f64 one = lf(global<be_f32>(kPoolTgt + 12)), inv = Divs(one, dist);
        const Vec3& dir = eye->dir;
        const f64 ny = Muls(dy, inv), nx = Muls(dx, inv), nz = Muls(dz, inv);
        const f64 dot = Fmadds(nz, lf(dir.z), Fmadds(nx, lf(dir.x), Muls(ny, lf(dir.y))));
        const f64 m1 = lf(global<be_f32>(kPoolTgt + 16));
        f64 c = one;
        if (dot < c) c = dot;
        if (c > m1) {
            c = one;
            if (dot < c) c = dot;
        } else {
            c = m1;
        }
        // fsel clamp to [-1, 1] (f64 -1.0 / 1.0 at +24 / +32).
        f64 a = Adds(c, one);
        a = S(a >= 0.0 ? c : ld(global<be_f64>(kPoolTgt + 24)));
        const f64 b = Subs(a, one);
        a = S(b >= 0.0 ? ld(global<be_f64>(kPoolTgt + 32)) : a);
        angle = Muls(call<f64>(Acosf, a), lf(global<be_f32>(kPoolTgt + 40)));
        t = &gm<TgtTuning>(o->tuning);
    }
    const f64 range = lf(t->range);
    if (!(dist <= range)) return;
    const f64 half = lf(global<be_f32>(kPoolTgt + 8));
    if (!(angle <= Muls(lf(t->cone), half))) return;
    const f64 one = lf(global<be_f32>(kPoolTgt + 12));
    f64 score = Muls(Divs(dist, range), Subs(one, lf(t->weight)));
    const f64 ar = Divs(angle, Muls(lf(t->cone), half));
    score = Fmadds(ar, lf(t->weight), score);
    be_f32* sp = frame.as<be_f32>(144);
    *sp = static_cast<f32>(score);
    u32 sraw = bits(*sp);
    if (keep == 0) {
        if (vcall<u32>(x, 304, gm<TgtSrc>(o->src.ea).w8.get(), o->mode.get(), sp) == 0) return;
        sraw = bits(*sp);
        score = LfsW(sraw);
        t = &gm<TgtTuning>(o->tuning);
    }
    const u32 n = static_cast<u32>(t->n.get());
    const u32 tb = guest(table);
    const f64 last = lf(gm<be_f32>(tb + 8 * (n - 1) + 4));
    if (!(score < last)) return;
    if (unique != 0 && static_cast<s32>(n) > 0)
        for (s32 i = 0; i < static_cast<s32>(n); ++i)
            if (gm<TgtEntry>(tb + 8 * static_cast<u32>(i)).obj == guest(x)) return;
    if (!(static_cast<s32>(n) > 0)) return;
    s32 i = 0;
    for (;; ++i) {
        if (!(i < static_cast<s32>(n))) return;
        if (score < lf(gm<be_f32>(tb + 8 * static_cast<u32>(i) + 4))) break;
    }
    for (s32 j = static_cast<s32>(n) - 1; j > i; --j) {
        TgtEntry& dst = gm<TgtEntry>(tb + 8 * static_cast<u32>(j));
        const TgtEntry& srce = gm<TgtEntry>(tb + 8 * static_cast<u32>(j) - 8);
        dst.obj = srce.obj.get();
        dst.score = srce.score.get();
    }
    TgtEntry& e = gm<TgtEntry>(tb + 8 * static_cast<u32>(i));
    e.obj = guest(x);
    e.score = sraw;
    const u32 c1 = *cntp + 1;
    const s32 cap = t->n;
    *cntp = static_cast<s32>(c1) < cap ? c1 : static_cast<u32>(cap);
}

}  // namespace sly4::obj

NATIVE(0026FD90, sly4::obj::Targets_Consider_0026FD90)
