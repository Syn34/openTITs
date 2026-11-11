// Game code, large-function sweep part 11: the scripted camera rig's mode
// handlers (CamRig_Update4168B0 steps the mode +80 and calls these with the
// rig and f1 = dt).
//
// CamRig layout used here: +4 the rig's own object, +20 limit, +40 / +44
// rates, +60 / +64 / +68 / +72 durations, +76 the target (+92 its object,
// +53 u8, +76 s32 state, +424 / +425 / +426 / +432 u8 flags), +80 mode,
// +84 / +88 step timer / its length, +92 .. +124 mode state (+96 / +100 a
// second timer / its length, +104 position, +112 pad state, +116 / +120
// timers, +124 count), +128 / +132 / +136 blend, +140 / +160 two u32
// PtrVectors {+0 data, +4 u16 capacity, +8 u16 count, +10 u16 mark}
// (anim keys and their blend flags).
#include "big11.h"

using namespace sly4;
using namespace sly4::big11;

namespace {
constexpr u32 kRestart = 0x002775A8u;     // Anim_Restart2775A8 (anim, key, 0, 0, -, -, 0, 0, -, -, 0, -1; f1..f4)
constexpr u32 kTrackSet = 0x0029879Cu;    // Anim_TrackSet36_0029879C (anim, 0, 0)
constexpr u32 kAnimFind = 0x00298874u;    // Anim_Find298874 (anim, 0)
constexpr u32 kAnimRate = 0x00276E4Cu;    // Obj_AnimCall3541E0_00276E4C (anim, -, 0; f32)
constexpr u32 kFindComp = 0x004BB4B4u;    // Effect_FindComponent (fx, type hash)
constexpr u32 kSetHash = 0x004CF988u;     // Effect_SetHash4CF988 (fx, hash)
constexpr u32 kSetSelect = 0x00491000u;   // SetSelect_491000 (-, u32 hash): the global set entry for hash (name?)
constexpr u32 kReserve = 0x00551718u;     // PtrVector_Reserve (v, capacity)
constexpr u32 kMapFind = 0x0076611Cu;     // SortedMap_Find3 (map, const u32* key) -> node**
constexpr u32 kSay = 0x004A5C90u;         // Speaker_SayId4A5C90 (speaker, id, 1; f32)
constexpr u32 kCos = 0x007344E0u;         // Math_Cosf (f32)
constexpr u32 kFromRotPos = 0x00553B4Cu;  // Mat34_FromRotPos553B4C (Mat34* out, const Mat33* rot, const vec3* pos)
constexpr u32 kNotify = 0x0036742Cu;      // Handle_Notify36742C (handle, a, b, c)
constexpr u32 kStartScene = 0x0046C3B8u;  // World_StartScene46C3B8 (world, hash, 0 x 8)
constexpr u32 kResolve = 0x003568E0u;     // Handle_ResolveBit1 (handle)
constexpr u32 kFormat = 0x005534D0u;      // String_Format (fmt, ...)
constexpr u32 kPlayerSet = 0x003632C8u;   // Player_Set3632C8 (obj, const char*, u8)
constexpr u32 kClear68 = 0x0049E860u;     // Obj_Clear68_0049E860 (record*)
constexpr u32 kFindRec = 0x002D9994u;     // Obj_FindRecord76_002D9994 (records, record*)
constexpr u32 kRecUpdate = 0x002D98DCu;   // Rec_Update_002D98DC (records, record*)

constexpr u32 kWorldPtr = 0x00F65CA0u;
constexpr u32 kRecordsPtr = 0x00F64C20u;  // -> (+40 the record set)
constexpr u32 kSpeakers = 0x0106C228u;    // speaker map (node +28 the speaker)
constexpr u32 kRand = 0x01164888u;        // u64 previous random value, +8 u32 state
constexpr u32 kPads = 0x010B19E0u;        // pad states, 11968 bytes per port (+176 +8 / +12 button masks)
constexpr u32 kCamBusy = 0x0106B62Cu;     // u8
constexpr u32 kFmtD = 0x00ADF3D8u;        // "%d"
constexpr u32 kFxComp = 0x87208BC1u;      // the component type the target's matrix is copied to

u32 Anim(PPUContext& ctx, u32 obj) { return static_cast<u32>(VCall(ctx, obj, 60, {obj})); }

// Anim_Restart2775A8(anim, key, 0, 0, r7, r8, 0, 0, -, -, 0, -1; a, b, c, d):
// r7 / r8 are whatever the anim getter left (the callee does not read
// them), the 9th / 10th arguments (r1 +112 / +120) are not written.
void Restart(PPUContext& ctx, u32 anim, u32 key, f64 a, f64 b, f64 c, f64 d) {
    callf(ctx, kRestart, {anim, key, 0, 0, ctx.gpr[7], ctx.gpr[8], 0, 0, 0, 0, 0, ~0ull}, {a, b, c, d});
}

// The target's object (tg +92) restarts k1, then the rig's object (+4) k2.
void RestartBoth(PPUContext& ctx, u32 o, u32 tg, u32 k1, u32 k2, f64 a, f64 b, f64 c, f64 d) {
    Restart(ctx, Anim(ctx, load<u32>(tg + 92)), k1, a, b, c, d);
    const u32 o4 = load<u32>(o + 4);
    Restart(ctx, Anim(ctx, o4), k2, a, b, c, d);
}

// fx's component kFxComp gets src's matrix rows (+96) when its kind (vfunc
// +32) is 4 (f1 = the last word copied), then Effect_SetHash4CF988(fx, hash).
void EffectSync(PPUContext& ctx, u32 src, u32 fx, u32 hash) {
    const u32 comp = static_cast<u32>(call(ctx, kFindComp, {fx, kFxComp}));
    if (comp != 0 && static_cast<u32>(VCall(ctx, comp, 32, {comp})) == 4) SetF1Bits(ctx, CopyMat43Last(comp + 32, src + 96));
    call(ctx, kSetHash, {fx, hash});
}

// Effect of obj (vfunc +272) synced as above when there is one; returns it.
u32 ObjEffectSync(PPUContext& ctx, u32 obj, u32 hash) {
    const u32 fx = static_cast<u32>(VCall(ctx, obj, 272, {obj}));
    if (fx != 0) EffectSync(ctx, obj, fx, hash);
    return fx;
}

// The inlined random step: state' = rotl((rotl(s, 4) + rotl(s, 16)) ^
// 0x87654321, 16), previous value = the low half; returns the old state.
u32 RandStep() {
    const u32 st = load<u32>(kRand + 8);
    const u32 x = (std::rotl(st, 4) + std::rotl(st, 16)) ^ 0x87654321u;
    store<u32>(kRand + 8, std::rotl(x, 16));
    store<u64>(kRand, x & 0xFFFFu);
    return st;
}

// The pad state entry of the world's current port (World +4209).
u32 PadEntry() { return kPads + load<u8>(load<u32>(kWorldPtr) + 4209) * 11968u + 176u; }

// Speaker line: speaker map node for key (a stack word at keyp) +28 says id.
void Say(PPUContext& ctx, u32 keyp, u32 key, u32 id, u32 k) {
    store<u32>(keyp, key);
    const u32 node = load<u32>(static_cast<u32>(call(ctx, kMapFind, {kSpeakers, keyp})));
    callf(ctx, kSay, {load<u32>(node + 28), id, 1}, {LfsAt(k)});
}

// The inlined "remove the last element" loop: the count is decremented
// until it is 0, one store per step (the shifting branch for an index below
// the last one is unreachable: n - 1 and (n - 1) & 0xFFFF are equal).
void ClearAll(u32 o, u32 cnt) {
    for (u32 n; (n = load<u16>(o + cnt)) != 0;) store<u16>(o + cnt, static_cast<u16>(n - 1));
}

// The inlined pop-front shift of vector (o + data, count at o + cnt) whose
// count was read as n: data[i] = data[i + 1] for i < count - 1, the count
// re-read each step (before the copy, or after it); returns the last count
// read (the caller stores it - 1).
template <bool kReadFirst>
u32 Shift(u32 o, u32 cnt, u32 data, u32 n) {
    if (static_cast<s32>(n - 1) > 0) {
        u32 i = 0;
        do {
            const u32 d = load<u32>(o + data);
            const u32 a = d + 4 * i;
            i = (i + 1) & 0xFFFFu;
            if (kReadFirst) n = load<u16>(o + cnt);
            store<u32>(a, load<u32>(a + 4));
            if (!kReadFirst) n = load<u16>(o + cnt);
        } while (static_cast<s32>(i) < static_cast<s32>(n - 1));
    }
    return n;
}

// Pop-front of both vectors (+140, then +160); B's count is read before A's
// is stored; between() runs before B's count is stored.
template <bool kReadFirst, class Fn>
void PopBoth(u32 o, Fn between) {
    const u32 na = Shift<kReadFirst>(o, 148, 140, load<u16>(o + 148));
    const u32 nb0 = load<u16>(o + 168);
    store<u16>(o + 148, static_cast<u16>(na - 1));
    const u32 nb = Shift<kReadFirst>(o, 168, 160, nb0);
    between();
    store<u16>(o + 168, static_cast<u16>(nb - 1));
}

// The high-water mark update: max(mark, n) by signed compare of the u16s.
u16 Mark(u32 mark, u32 n) { return static_cast<u16>(static_cast<s32>(mark) > static_cast<s32>(n & 0xFFFFu) ? mark : n); }

// PushBack va to vector +140 and vb to +160 (grown to twice the capacity
// when full): B's count is read before A's count is stored, B's capacity
// after it; between() runs between B's count and mark stores.
template <class Fn>
void PushBoth(PPUContext& ctx, u32 o, u32 va, u32 vb, Fn between) {
    u32 c = load<u16>(o + 148);
    const u32 cap = load<u16>(o + 144);
    if (c == cap) {
        call(ctx, kReserve, {o + 140, (cap << 1) & 0x1FFFEu});
        c = load<u16>(o + 148);
    }
    store<u32>(load<u32>(o + 140) + ((c << 2) & 0x3FFFCu), va);
    const u32 n = load<u16>(o + 148) + 1;
    const u32 m = load<u16>(o + 150);
    u32 c2 = load<u16>(o + 168);
    store<u16>(o + 148, static_cast<u16>(n));
    const u32 cap2 = load<u16>(o + 164);
    store<u16>(o + 150, Mark(m, n));
    if (c2 == cap2) {
        call(ctx, kReserve, {o + 160, (cap2 << 1) & 0x1FFFEu});
        c2 = load<u16>(o + 168);
    }
    store<u32>(load<u32>(o + 160) + ((c2 << 2) & 0x3FFFCu), vb);
    const u32 n2 = load<u16>(o + 168) + 1;
    const u32 m2 = load<u16>(o + 170);
    store<u16>(o + 168, static_cast<u16>(n2));
    between();
    store<u16>(o + 170, Mark(m2, n2));
}
void PushBoth(PPUContext& ctx, u32 o, u32 va, u32 vb) { PushBoth(ctx, o, va, vb, [] {}); }

// Step timer: +84 += dt; returns true (and +84 = +88, raw) when it passed
// +88. f1 is left = +88 either way (lim_w).
bool StepTimer(u32 o, f64 dt, u32& lim_w) {
    const f32 t = Adds(F(o + 84), dt);
    lim_w = load<u32>(o + 88);
    store<f32>(o + 84, t);
    return t > std::bit_cast<f32>(lim_w);
}

// The pad buttons test: the masks at +8 and +12 of the pad entry both hit
// the word.
bool PadBoth(u32 e, u32 w) { return (load<u32>(e + 8) & w) != 0 && (load<u32>(e + 12) & w) != 0; }
}  // namespace

// void CamRig_Mode2(CamRig* o; f32 dt) (name?): enter mode 3: +112 = -1,
// +92 / +96 / +116 / +120 / +124 = 0, the set entry 0x0E3F4988 selected,
// two (0x0E3F4988, 32 / 64) key pushes, both objects restart 0x21B0C54E and
// sync their effects (0x76F558B7).
DECOMP(00416360, CamRig_Mode2_416360) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x00416354u;   // .text f32: 0, 1, 0.25
    store<u32>(o + 112, 0xFFFFFFFFu);
    const f64 k0 = LfsAt(kK);
    store<u32>(o + 92, 0);
    store<u32>(o + 96, 0);
    store<u32>(o + 116, 0);
    store<u32>(o + 120, 0);
    store<u32>(o + 124, 0);
    store<u32>(o + 80, 3);
    call(ctx, kSetSelect, {load<u32>(o + 76), 0x0E3F4988u});
    PushBoth(ctx, o, 0x0E3F4988u, 32);
    PushBoth(ctx, o, 0x0E3F4988u, 64);
    RestartBoth(ctx, o, load<u32>(o + 76), 0x21B0C54Eu, 0x21B0C54Eu, LfsAt(kK + 4), LfsAt(kK + 8), k0, k0);
    ObjEffectSync(ctx, load<u32>(load<u32>(o + 76) + 92), 0x76F558B7u);
    ObjEffectSync(ctx, load<u32>(o + 4), 0x76F558B7u);   // r3 = 0 without an effect, else the SetHash result
}

// void CamRig_Mode3(CamRig* o; f32 dt) (name?): the pad-driven wait: a
// speaker line when the step timer first passes +88, +104 drifts with +44
// while +116 is past +64, the pad (+112 state against data[0] / data[1] of
// the +160 keys) pulls +104 back and counts +124 (a -1 state only feeds an
// unused Math_Cosf); then both objects are placed at the points between
// them (+104 weight K8) and mode 4 starts when +96 passes +60.
DECOMP(00415F10, CamRig_Mode3_415F10) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kK = 0x00415EF8u;   // .text f32: +0 speaker blend, +4 drift scale, +8 placement weight, +12 / +16 cos scales
    Stack frame(ctx, 256);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    const f32 t = Adds(F(o + 84), dt);
    const f32 lim = F(o + 88);
    store<f32>(o + 84, t);
    if (t > lim && Subs(t, dt) <= lim) Say(ctx, sp + 352, 0x8687021Fu, 0x19CF3109u, kK);

    const f32 a = Adds(F(o + 116), dt);
    const f32 len = F(o + 64);
    const f32 b = Adds(F(o + 120), dt);
    store<f32>(o + 116, a);
    store<f32>(o + 120, b);
    if (a > len) store<f32>(o + 104, Fmadds(Muls(F(o + 44), F(kK + 4)), dt, F(o + 104)));
    const u32 e = PadEntry();
    const u32 m = load<u32>(o + 112);
    u32 mm = m;
    bool pressed0 = false, held = false;
    if (m != 0 && PadBoth(e, load<u32>(load<u32>(o + 160)))) {
        pressed0 = true;
        mm = 0;
        store<u32>(o + 112, 0);
    }
    if ((pressed0 || m != 1) && PadBoth(e, load<u32>(load<u32>(o + 160) + 4))) {
        store<u32>(o + 112, 1);
        held = true;
    }
    f64 fa = a, fl = len;
    if (!held) {
        if (static_cast<s32>(mm) == -1) {
            const f32 k12 = F(kK + 12);
            callf(ctx, kCos, {}, {Muls(Muls(Muls(F(o + 96), k12), F(kK + 16)), k12)});   // result unused
            fl = F(o + 64);
            fa = F(o + 116);
        }
        held = pressed0;
    }
    if (held) {
        if (fa < fl) store<f32>(o + 104, Fnmsubs(Muls(F(o + 44), F(kK + 4)), Subs(fl, fa), F(o + 104)));
        const u32 c = load<u32>(o + 124);
        store<u32>(o + 116, 0);
        store<u32>(o + 124, c + 1);
    }

    // Place the rig's object at the point K8 of the way from the target's
    // object (+144 positions), and the target's object as far beyond it.
    const u32 tg = load<u32>(o + 76);
    const u32 o4 = load<u32>(o + 4);
    const f32 k8 = F(kK + 8);
    const u32 an = load<u32>(tg + 92);
    const f32 p0 = F(o4 + 144), p1 = F(o4 + 148), p2 = F(o4 + 152);
    const f32 q0 = F(an + 144), q1 = F(an + 148), q2 = F(an + 152);
    const f32 d0 = Subs(p0, q0), d1 = Subs(p1, q1), d2 = Subs(p2, q2);
    store<f32>(sp + 112, Fmadds(d0, k8, q0));
    store<f32>(sp + 116, Fmadds(d1, k8, q1));
    store<f32>(sp + 120, Fmadds(d2, k8, q2));
    for (u32 r = 0; r < 3; ++r)
        for (u32 c = 0; c < 3; ++c) CopyW(sp + 176 + 16 * r + 4 * c, o4 + 96 + 16 * r + 4 * c);
    call(ctx, kFromRotPos, {sp + 288, sp + 176, sp + 112});
    VCall(ctx, o4, 84, {o4, sp + 288});
    store<f32>(sp + 112, Fnmsubs(d0, k8, p0));
    store<f32>(sp + 116, Fnmsubs(d1, k8, p1));
    store<f32>(sp + 120, Fnmsubs(d2, k8, p2));
    const u32 an2 = load<u32>(load<u32>(o + 76) + 92);
    for (u32 r = 0; r < 3; ++r)
        for (u32 c = 0; c < 3; ++c) CopyW(sp + 128 + 16 * r + 4 * c, an2 + 96 + 16 * r + 4 * c);
    call(ctx, kFromRotPos, {sp + 224, sp + 128, sp + 112});
    VCall(ctx, an2, 84, {an2, sp + 224});
    const f32 w = Adds(F(o + 96), dt);
    const f32 lim2 = F(o + 60);
    store<f32>(o + 96, w);
    ctx.fpr[1] = w;
    if (w > lim2) {
        store<u32>(o + 80, 4);
        ctx.gpr[3] = 4;
    }
}

// void CamRig_Mode4(CamRig* o; f32 dt) (name?): when the step timer passes
// +88: pop the front of both key vectors, then mode 9 when +120 / (+124 + 1)
// / +64 <= 1 else 5 (5 also when the target's +53 is set and its state is
// 1), +96 = 0. f1 = the quotient (or +88).
DECOMP(00415DA0, CamRig_Mode4_415DA0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    u32 lim_w;
    if (!StepTimer(o, dt, lim_w)) {
        SetF1Bits(ctx, lim_w);
        return;
    }
    store<u32>(o + 84, lim_w);
    PopBoth<false>(o, [] {});
    const f32 n = static_cast<f32>(static_cast<f64>(static_cast<s32>(load<u32>(o + 124) + 1)));
    const f32 q = Divs(Divs(F(o + 120), n), F(o + 64));
    store<u32>(o + 80, q <= F(0x00415D9Cu) ? 9 : 5);   // .text f32 1.0
    const u32 tg = load<u32>(o + 76);
    if (load<u8>(tg + 53) != 0 && load<u32>(tg + 76) == 1) store<u32>(o + 80, 5);
    store<u32>(o + 96, 0);
    ctx.fpr[1] = q;
}

// void CamRig_Mode5(CamRig* o; f32 dt) (name?): empty both key vectors, push
// three random (key, flag) pairs, timers reset (+100 = 2), mode 6, both
// objects restart 0xC3B92BAB / 0x2CA2AA7D, set entry 0x74128921 selected.
DECOMP(00415830, CamRig_Mode5_415830) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x00415824u;   // .text f32 x3
    static constexpr u32 kPairs[4][2] = {{0xFD029907u, 128}, {0xD5FC2D81u, 64}, {0xF1BBC9CEu, 32}, {0xFA3C0E79u, 16}};
    ClearAll(o, 148);
    ClearAll(o, 168);
    for (u32 i = 0; i < 3; ++i) {
        const u32 s = RandStep() & 3;
        PushBoth(ctx, o, kPairs[s][0], kPairs[s][1]);
    }
    const u32 tg = load<u32>(o + 76);
    const f64 k0 = LfsAt(kK);
    store<u32>(o + 84, 0);
    store<u32>(o + 96, 0);
    store<u32>(o + 100, 0x40000000u);
    store<u32>(o + 80, 6);
    store<u32>(o + 116, 0);
    RestartBoth(ctx, o, tg, 0xC3B92BABu, 0x2CA2AA7Du, LfsAt(kK + 4), LfsAt(kK + 8), k0, k0);
    call(ctx, kSetSelect, {load<u32>(o + 76), 0x74128921u});
}

// void CamRig_Mode6(CamRig* o; f32 dt) (name?): when +96 passes +100: timers
// reset (+100 = 2), mode 7, both objects restart 0x3B55C203 / 0xD8E1A885,
// the set entry of the first key selected, blend restarted at rate 0.9.
DECOMP(00415680, CamRig_Mode6_415680) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kK = 0x00415674u;   // .text f32: 0, 1, 0.25
    const f32 t = Adds(F(o + 96), dt);
    const f32 lim = F(o + 100);
    store<f32>(o + 96, t);
    if (!(t > lim)) {
        ctx.fpr[1] = t;
        return;
    }
    const u32 tg = load<u32>(o + 76);
    const f64 k0 = LfsAt(kK);
    store<u32>(o + 96, 0);
    store<u32>(o + 100, 0x40000000u);
    store<u32>(o + 80, 7);
    store<u32>(o + 116, 0);
    const f32 k1 = F(kK + 4);
    RestartBoth(ctx, o, tg, 0x3B55C203u, 0xD8E1A885u, k1, LfsAt(kK + 8), k0, k0);
    const u32 d = load<u32>(o + 140);
    const u32 t76 = load<u32>(o + 76);
    call(ctx, kSetSelect, {t76, load<u32>(d)});
    const f32 bt = F(o + 128), bb = F(o + 136), ba = F(o + 132);
    store<u32>(o + 128, 0);
    store<u32>(o + 132, 0x3F666666u);
    const f32 r = Fmadds(ba, bt, Muls(Subs(k1, bt), bb));
    store<f32>(o + 136, r);
    ctx.gpr[3] = 0x3F666666u;
    ctx.fpr[1] = r;
}

// void CamRig_Mode7(CamRig* o; f32 dt) (name?): step timer (capped at +88);
// when +116 passes +68, +104 += +44 * K0 and the objects restart
// 0x712EA213 / 0xB3261AD1; then on both pad masks hitting the first key:
// restart 0xDFBB2F0D / 0x81B7704B and sync the rig object's effect
// (0x2496BD80); else on both masks' 0xF0 bits: the drift / restart again and
// the effect synced with 0xB6931B97. Mode 8 (+84 = 0) after any of these.
DECOMP(00415088, CamRig_Mode7_415088) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kK = 0x00415078u;   // .text f32: +0 drift scale, +4 / +8 / +12 restart blends
    const f32 t = Adds(F(o + 84), dt);
    const u32 lim_w = load<u32>(o + 88);
    f32 pos = F(o + 104);
    const f32 t116 = F(o + 116);
    const f32 len = F(o + 68);
    f32 rate = F(o + 44);
    u32 tg = load<u32>(o + 76);
    store<f32>(o + 84, t);
    if (t > std::bit_cast<f32>(lim_w)) store<u32>(o + 84, lim_w);
    const f32 a = Adds(t116, dt);
    bool moved = false;
    store<f32>(o + 116, a);
    ctx.fpr[1] = a;
    auto drift = [&] {
        store<f32>(o + 104, Fmadds(rate, F(kK), pos));
        RestartBoth(ctx, o, tg, 0x712EA213u, 0xB3261AD1u, LfsAt(kK + 4), LfsAt(kK + 8), LfsAt(kK + 12), LfsAt(kK + 12));
    };
    if (a > len) {
        moved = true;
        drift();
        tg = load<u32>(o + 76);
        pos = F(o + 104);
        rate = F(o + 44);
    }
    const u32 e = PadEntry();
    const u32 w = load<u32>(load<u32>(o + 160));
    const u32 x8 = load<u32>(e + 8), x12 = load<u32>(e + 12);
    if ((x8 & w) != 0 && (x12 & w) != 0) {
        RestartBoth(ctx, o, tg, 0xDFBB2F0Du, 0x81B7704Bu, LfsAt(kK + 4), LfsAt(kK + 8), LfsAt(kK + 12), LfsAt(kK + 12));
        ObjEffectSync(ctx, load<u32>(o + 4), 0x2496BD80u);
    } else if ((x8 & 0xF0u) != 0 && (x12 & 0xF0u) != 0) {
        drift();
        ObjEffectSync(ctx, load<u32>(o + 4), 0xB6931B97u);
    } else if (!moved) {
        ctx.gpr[3] = (x8 & 0xF0u) == 0 ? x12 : 0;   // as the original leaves r3
        return;
    }
    store<u32>(o + 80, 8);
    store<u32>(o + 84, 0);
    ctx.gpr[3] = 8;
}

// void CamRig_Mode8(CamRig* o; f32 dt) (name?): when the step timer passes
// +88 and the target's anim track is done (TrackSet, else Find): pop the
// front of both key vectors, +96 = 0, +100 = 1, mode 6, set entry
// 0x74128921, +116 = 0; then both objects restart 0xC3B92BAB / 0x2CA2AA7D,
// and with no keys left mode 0 and the target's +432 flag set
// (CamRig_Mode13Done when its +53 is set and its state is 1).
DECOMP(00414CB0, CamRig_Mode8_414CB0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kK = 0x00414CA4u;   // .text f32: 0, 1, 0.25
    const f32 t = Adds(F(o + 84), dt);
    const u32 lim_w = load<u32>(o + 88);
    const u32 tg = load<u32>(o + 76);
    store<f32>(o + 84, t);
    SetF1Bits(ctx, lim_w);
    if (!(t > std::bit_cast<f32>(lim_w))) {
        ctx.gpr[3] = tg;
        return;
    }
    store<u32>(o + 84, lim_w);
    if (static_cast<u32>(call(ctx, kTrackSet, {Anim(ctx, load<u32>(tg + 92)), 0, 0})) == 0 &&
        static_cast<u32>(call(ctx, kAnimFind, {Anim(ctx, load<u32>(load<u32>(o + 76) + 92)), 0})) == 0)
        return;
    PopBoth<false>(o, [] {});
    const f64 k0 = LfsAt(kK);
    store<u32>(o + 96, 0);
    const f64 k1 = LfsAt(kK + 4);
    store<u32>(o + 100, 0x3F800000u);
    const u32 t76 = load<u32>(o + 76);
    store<u32>(o + 80, 6);
    call(ctx, kSetSelect, {t76, 0x74128921u});
    const u32 n = load<u16>(o + 148);
    store<u32>(o + 116, 0);
    if (n != 0) {
        RestartBoth(ctx, o, load<u32>(o + 76), 0xC3B92BABu, 0x2CA2AA7Du, k1, LfsAt(kK + 8), k0, k0);
        return;
    }
    const u32 tg2 = load<u32>(o + 76);
    store<u32>(o + 80, 0);
    RestartBoth(ctx, o, tg2, 0xC3B92BABu, 0x2CA2AA7Du, k1, LfsAt(kK + 8), k0, k0);
    store<u8>(load<u32>(o + 76) + 432, 1);
    const u32 t4 = load<u32>(o + 76);
    if (load<u8>(t4 + 53) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 st = load<u32>(t4 + 76);
    if (st != 1) {
        ctx.gpr[3] = st;
        return;
    }
    call(ctx, 0x00413BACu, {t4});   // CamRig_Mode13Done_413BAC
}

// void CamRig_Mode9(CamRig* o; f32 dt) (name?): empty both key vectors (a
// random step whose value is unused), push (0x0E4082B6, 128), +84 = 0, both
// objects restart 0x2CA2AA7D / 0xC3B92BAB, +96 = 0, +100 = +72, mode 10,
// the set entry of the first key selected (f1 = +72).
DECOMP(004149D0, CamRig_Mode9_4149D0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x004149C4u;   // .text f32: 0, 1, 0.25
    ClearAll(o, 148);
    ClearAll(o, 168);
    RandStep();
    u32 tg = 0;
    f64 k0 = 0;
    PushBoth(ctx, o, 0x0E4082B6u, 128, [&] {
        tg = load<u32>(o + 76);
        k0 = LfsAt(kK);
        store<u32>(o + 84, 0);
    });
    RestartBoth(ctx, o, tg, 0x2CA2AA7Du, 0xC3B92BABu, LfsAt(kK + 4), LfsAt(kK + 8), k0, k0);
    const u32 w72 = load<u32>(o + 72);
    store<u32>(o + 96, 0);
    const u32 d = load<u32>(o + 140);
    store<u32>(o + 100, w72);
    store<u32>(o + 80, 10);
    const u32 t76 = load<u32>(o + 76);
    SetF1Bits(ctx, w72);
    call(ctx, kSetSelect, {t76, load<u32>(d)});
}

namespace {
// Obj_AnimCall3541E0_00276E4C(obj's anim, r4, 0; x): r4 (unread by the
// callee) is what the anim getter left, or the constant the original had
// just loaded into it (r4_set).
void Rate(PPUContext& ctx, u32 obj, f64 x, u32 r4_set = 0) {
    const u32 a = Anim(ctx, obj);
    callf(ctx, kAnimRate, {a, r4_set != 0 ? r4_set : ctx.gpr[4], 0}, {x});
}
}  // namespace

// void CamRig_Mode10(CamRig* o; f32 dt) (name?): a speaker line when the
// step timer first passes +88 (and an unused speaker map lookup); on both pad
// masks hitting the first key: +104 -= +40 * K12 * dt, the target object's
// effect synced (0xC65881CC), and unless the rig object's anim is still
// found (Find, Find, TrackSet), both restart 0x05642823 at rate K0; when
// +96 passes +100 mode 11 (+84 = 0, rates K0); when the rig object's track
// is done: rates K0 and restarts 0x2CA2AA7D / 0xC3B92BAB.
DECOMP(004142F0, CamRig_Mode10_4142F0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kK = 0x004142DCu;   // .text f32: +0 / +4 / +8 restart blends (+0 rate, speaker), +12 drift scale
    Stack frame(ctx, 48);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    const f32 t = Adds(F(o + 84), dt);
    const f32 lim = F(o + 88);
    store<f32>(o + 84, t);
    if (t > lim && Subs(t, dt) <= lim) Say(ctx, sp + 148, 0x392E4B66u, 0x02759C22u, kK);
    store<u32>(sp + 144, 0x392E4B66u);
    call(ctx, kMapFind, {kSpeakers, sp + 144});   // result unused
    const u32 e = PadEntry();
    if (PadBoth(e, load<u32>(load<u32>(o + 160)))) {
        store<f32>(o + 104, Fnmsubs(Muls(F(o + 40), F(kK + 12)), dt, F(o + 104)));
        ObjEffectSync(ctx, load<u32>(load<u32>(o + 76) + 92), 0xC65881CCu);
        bool go;
        if (static_cast<u32>(call(ctx, kAnimFind, {Anim(ctx, load<u32>(o + 4)), 0})) != 0)
            go = true;
        else if (static_cast<u32>(call(ctx, kAnimFind, {Anim(ctx, load<u32>(o + 4)), 0})) != 0)
            go = false;
        else
            go = static_cast<u32>(call(ctx, kTrackSet, {Anim(ctx, load<u32>(o + 4)), 0, 0})) != 0;
        if (go) {
            const f64 a = LfsAt(kK), b = LfsAt(kK + 4), c = LfsAt(kK + 8);
            RestartBoth(ctx, o, load<u32>(o + 76), 0x05642823u, 0x05642823u, a, b, c, c);
            Rate(ctx, load<u32>(o + 4), a);
            Rate(ctx, load<u32>(load<u32>(o + 76) + 92), a);
        }
    }
    const f32 w = Adds(F(o + 96), dt);
    const f32 lim2 = F(o + 100);
    store<f32>(o + 96, w);
    ctx.fpr[1] = w;
    if (w > lim2) {
        const u32 o4 = load<u32>(o + 4);
        store<u32>(o + 80, 11);
        store<u32>(o + 84, 0);
        Rate(ctx, o4, LfsAt(kK), kK);   // r4 = &K
        Rate(ctx, load<u32>(load<u32>(o + 76) + 92), LfsAt(kK));
    }
    if (static_cast<u32>(call(ctx, kTrackSet, {Anim(ctx, load<u32>(o + 4)), 0, 0})) == 0) return;
    if (static_cast<u32>(call(ctx, kAnimFind, {Anim(ctx, load<u32>(o + 4)), 0})) != 0) return;
    const f64 k = LfsAt(kK);
    Rate(ctx, load<u32>(o + 4), k, 0x00410000u);   // r4 = the high half of &K
    Rate(ctx, load<u32>(load<u32>(o + 76) + 92), k);
    RestartBoth(ctx, o, load<u32>(o + 76), 0x2CA2AA7Du, 0xC3B92BABu, k, LfsAt(kK + 4), LfsAt(kK + 8), LfsAt(kK + 8));
}

// void CamRig_Mode11(CamRig* o; f32 dt) (name?): when the step timer passes
// +88: set entry 0x74128921, pop the front of both key vectors, mode 0, the
// objects restart 0x2CA2AA7D / 0xC3B92BAB unless the rig object's anim is
// found; the target's +432 flag set, mode 12 when its +53 is set and its
// state is 3.
DECOMP(00414058, CamRig_Mode11_414058) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kK = 0x0041404Cu;   // .text f32: 1, 0.25, 0
    const f32 t = Adds(F(o + 84), dt);
    const u32 lim_w = load<u32>(o + 88);
    const u32 tg = load<u32>(o + 76);
    store<f32>(o + 84, t);
    SetF1Bits(ctx, lim_w);
    if (!(t > std::bit_cast<f32>(lim_w))) {
        ctx.gpr[3] = tg;
        return;
    }
    store<u32>(o + 84, lim_w);
    call(ctx, kSetSelect, {tg, 0x74128921u});
    u32 o4 = 0;
    PopBoth<true>(o, [&] {
        o4 = load<u32>(o + 4);
        store<u32>(o + 80, 0);
    });
    if (static_cast<u32>(call(ctx, kAnimFind, {Anim(ctx, o4), 0})) == 0)
        RestartBoth(ctx, o, load<u32>(o + 76), 0x2CA2AA7Du, 0xC3B92BABu, LfsAt(kK), LfsAt(kK + 4), LfsAt(kK + 8), LfsAt(kK + 8));
    const u32 t1 = load<u32>(o + 76);
    const u32 t2 = load<u32>(o + 76);
    store<u8>(t1 + 432, 1);
    if (load<u8>(t2 + 53) == 0) {
        ctx.gpr[3] = t2;   // the byte went to r4
        return;
    }
    const u32 st = load<u32>(t2 + 76);
    if (st != 3) {
        ctx.gpr[3] = st;
        return;
    }
    store<u32>(o + 80, 12);
    ctx.gpr[3] = 12;
}

// void CamRig_Mode12(CamRig* o; f32 dt) (name?): mode 13, the objects
// restart 0x71CDB228 / 0x055E16F9, the rig object's effect synced
// (0xC65270FF), set entry 0x74128921 selected.
DECOMP(00413DF8, CamRig_Mode12_413DF8) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x00413DECu;   // .text f32: 1, 0.25, 0
    const u32 tg = load<u32>(o + 76);
    store<u32>(o + 80, 13);
    const f64 a = LfsAt(kK), b = LfsAt(kK + 4), c = LfsAt(kK + 8);
    RestartBoth(ctx, o, tg, 0x71CDB228u, 0x055E16F9u, a, b, c, c);
    ObjEffectSync(ctx, load<u32>(o + 4), 0xC65270FFu);
    call(ctx, kSetSelect, {load<u32>(o + 76), 0x74128921u});
}

// u8 CamRig_Mode13Done(Target* tg) (name?): the target's finish: with +53
// set just the +425 / +424 flags, else (once, +424 clear) a record
// 0x1B2EEF42 (kind 13) counted up and shown on 0x35B15B25 ("%d" of the
// count, then "6"), +87 = 1 and scene 0x6DB32D4F; then 0x6D7DA1D8 notified
// and shown "1" twice, set entry 0x74128921, +424 / +425 = 1. Returns 1.
DECOMP(00413BAC, CamRig_Mode13Done_413BAC) {
    const u32 tg = arg<u32>(ctx, 0);
    Stack frame(ctx, 96);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    auto show = [&](u32 handle, u64 v, u32 which) {
        const u32 h = static_cast<u32>(call(ctx, kResolve, {handle}));
        const u32 s = static_cast<u32>(call(ctx, kFormat, {kFmtD, v}));
        call(ctx, kPlayerSet, {h, s, which});
    };
    if (load<u8>(tg + 53) != 0) {
        store<u8>(tg + 425, 1);
        store<u8>(tg + 424, 1);
    } else if (load<u8>(tg + 424) == 0) {
        const u32 recs = load<u32>(load<u32>(kRecordsPtr) + 40);
        if (recs != 0) {
            const u32 rec = sp + 128;   // 76-byte record on the stack
            call(ctx, kClear68, {rec});
            store<u8>(sp + 200, 13);
            store<u32>(sp + 128, 1);
            store<u32>(sp + 196, 0x1B2EEF42u);
            call(ctx, kFindRec, {recs, rec});
            store<u32>(sp + 128, load<u32>(sp + 128) + 1);
            call(ctx, kRecUpdate, {recs, rec});
            show(0x35B15B25u, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(sp + 128)))), 0);
            show(0x35B15B25u, 6, 1);
        }
        const u32 world = load<u32>(kWorldPtr);
        store<u8>(tg + 87, 1);
        call(ctx, kStartScene, {world, 0x6DB32D4Fu, 0, 0, 0, 0, 0, 0, 0, 0});
    }
    call(ctx, kNotify, {0x6D7DA1D8u, 0, 1, 0});
    call(ctx, kSetSelect, {tg, 0x74128921u});
    show(0x6D7DA1D8u, 1, 0);
    show(0x6D7DA1D8u, 1, 1);
    store<u8>(tg + 424, 1);
    store<u8>(tg + 425, 1);
    ctx.gpr[3] = 1;
}

// void CamRig_Mode14(CamRig* o; f32 dt) (name?): mode 15, the objects
// restart 0x055E16F9 / 0x71CDB228, the target object's effect synced
// (0xC65270FF).
DECOMP(00413970, CamRig_Mode14_413970) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x00413964u;   // .text f32: 1, 0.25, 0
    const u32 tg = load<u32>(o + 76);
    store<u32>(o + 80, 15);
    const f64 a = LfsAt(kK), b = LfsAt(kK + 4), c = LfsAt(kK + 8);
    RestartBoth(ctx, o, tg, 0x055E16F9u, 0x71CDB228u, a, b, c, c);
    ObjEffectSync(ctx, load<u32>(load<u32>(o + 76) + 92), 0xC65270FFu);
}

// u32 CamRig_Mode15(CamRig* o; f32 dt) (name?): when the target object's
// track is done: the busy byte set; with +53 the +426 / +424 flags, else
// (top bit of +96 clear, a +44 def whose +18 is not 6, +424 clear) scene
// 0xB2A0FD73, +424 / +426 = 1, 0x6D7DA1D8 notified, set entry 0x74128921,
// +87 = 1, +96 top bit set. r3 as the original leaves it.
DECOMP(00413804, CamRig_Mode15_413804) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, kTrackSet, {Anim(ctx, load<u32>(load<u32>(o + 76) + 92)), 0, 0})) == 0) return;
    const u32 tg = load<u32>(o + 76);
    store<u8>(kCamBusy, 1);
    u32 r3;
    if (load<u8>(tg + 53) != 0) {
        store<u8>(tg + 426, 1);
        store<u8>(tg + 424, 1);
        r3 = 1;
    } else if ((load<u32>(tg + 96) & 0x80000000u) != 0) {
        r3 = 1;
    } else if (const u32 def = load<u32>(tg + 44); def == 0) {
        r3 = 0;
    } else if (load<u8>(def + 18) == 6) {
        r3 = 6;
    } else if (const u32 busy = load<u8>(tg + 424); busy != 0) {
        r3 = busy;
    } else {
        call(ctx, kStartScene, {load<u32>(kWorldPtr), 0xB2A0FD73u, 0, 0, 0, 0, 0, 0, 0, 0});
        store<u8>(tg + 424, 1);
        store<u8>(tg + 426, 1);
        call(ctx, kNotify, {0x6D7DA1D8u, 0, 1, 0});
        call(ctx, kSetSelect, {tg, 0x74128921u});
        const u32 w = load<u32>(tg + 96);
        store<u8>(tg + 87, 1);
        r3 = w | 0x80000000u;
        store<u32>(tg + 96, r3);
    }
    ctx.gpr[3] = r3;
}
