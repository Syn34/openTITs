// Game code, large-function sweep part 21: a speaker node's line start (the
// animation control it builds and the skeleton binding it makes on first
// use).
#include "big21.h"

using namespace sly4;
using namespace sly4::big21;

namespace {
constexpr u32 kSayK = 0x00350910u;  // f32 1.0, 2^-23, ~FLT_MAX, 0.0; masks {0,-1,0,0} {0,0,0,-1} {0,0,-1,0}; f32 0.33, 0.66
constexpr u32 kFmtS = 0x00ADC098u;  // "%s"
constexpr u32 kSkelMap = 0x0106BC14u;  // skeletons by name hash (SortedMap_Find76D894)
constexpr u32 kLineMap = 0x0106BBCCu;  // (SortedMap_Find76E0A8)
constexpr u32 kHkMem = 0x011DF31Cu;    // Havok container allocator (vt +20: free(p, bytes))
constexpr u32 kNotOwned = 0x80000000u, kCapMask = 0x3FFFFFFFu;

namespace sfn {
constexpr u32 String_Format = 0x005534D0u;
constexpr u32 String_HashNoCase = 0x0056A710u;
constexpr u32 SortedMap_Find76D894 = 0x0076D894u;
constexpr u32 SortedMap_Find76E0A8 = 0x0076E0A8u;
constexpr u32 HkArray_Reserve = 0x007CCE88u;   // (allocator, hkArray*, s32 n, s32 elem size)
constexpr u32 AnimSkel_Release = 0x007D4EA8u;  // (AnimSkel*)
constexpr u32 AnimSkel_Construct = 0x00A45750u;  // (AnimSkel*, Skeleton*)
constexpr u32 AnimSkel_AddControl = 0x00A45478u;  // f_00A45478(AnimSkel*, control)
constexpr u32 Control_Init = 0x00A4C600u;      // f_00A4C600(control, anim, 0, s32 dir)
constexpr u32 Pose_SetReference = 0x00A56E68u;
constexpr u32 Speaker_SetLine = 0x0034F690u;   // (Speaker*, Skeleton*, 0)
}  // namespace sfn

u32 Allocator(PPUContext& ctx) { return load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) - 28332u) + 44); }

// A virtual call whose original sequence leaves the entry address in the
// register after the arguments and the OPD after that.
u64 VCallEO(PPUContext& ctx, u32 o, u32 slot, u64 a1, u64 a2 = 0, u32 n = 2) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    const u32 ent = load<u32>(opd);
    if (n == 3) return call_ptr(ctx, opd, {o, a1, a2, ent, opd});
    return call_ptr(ctx, opd, {o, a1, ent, opd});
}

// kHkMem->vfn[20](kHkMem, p, bytes), with the entry/OPD order of the site.
void HkFree(PPUContext& ctx, u32 p, u32 bytes, bool entryFirst) {
    const u64 b = static_cast<u64>(static_cast<s64>(static_cast<s32>(bytes)));
    if (entryFirst) VCallEO(ctx, kHkMem, 20, p, b, 3);
    else VCall(ctx, kHkMem, 20, {kHkMem, p, b});
}

void Reserve(PPUContext& ctx, u32 arr, s32 n, u32 elem) {
    call(ctx, sfn::HkArray_Reserve, {kHkMem, arr, static_cast<u64>(static_cast<s64>(n)), elem});
}
void Grow(PPUContext& ctx, u32 arr, s32 n, u32 elem) {
    const u32 cap = load<u32>(arr + 8);
    if (static_cast<s32>(cap & kCapMask) >= n) return;
    const s32 dbl = static_cast<s32>((cap << 1) & 0x7FFFFFFEu);
    Reserve(ctx, arr, dbl > n ? dbl : n, elem);
}

// Destroys the pose *slot (its hkArrays +44 / +28 / +16 / +4 freed through
// kHkMem when owned, cleared and marked not owned; the 56-byte block back
// to the thread allocator) and zeroes the slot. eoN: the site's free call
// leaves the entry address before the OPD.
void DestroyPose(PPUContext& ctx, u32 slot, bool eo44, bool eo28, bool eo16, bool eo4) {
    if (const u32 old = load<u32>(slot); old != 0) {
        u32 cap = load<u32>(old + 52);
        store<u32>(old + 48, 0);
        if ((cap & kNotOwned) == 0) HkFree(ctx, load<u32>(old + 44), cap << 2, eo44);
        cap = load<u32>(old + 36);
        store<u32>(old + 44, 0);
        store<u32>(old + 52, kNotOwned);
        store<u32>(old + 32, 0);
        if ((cap & kNotOwned) == 0) HkFree(ctx, load<u32>(old + 28), cap << 2, eo28);
        cap = load<u32>(old + 24);
        store<u32>(old + 28, 0);
        store<u32>(old + 36, kNotOwned);
        store<u32>(old + 20, 0);
        if ((cap & kNotOwned) == 0) HkFree(ctx, load<u32>(old + 16), (cap & kCapMask) * 48, eo16);
        cap = load<u32>(old + 12);
        store<u32>(old + 16, 0);
        store<u32>(old + 24, kNotOwned);
        store<u32>(old + 8, 0);
        if ((cap & kNotOwned) == 0) HkFree(ctx, load<u32>(old + 4), (cap & kCapMask) * 48, eo4);
        store<u32>(old + 4, 0);
        store<u32>(old + 12, kNotOwned);
        const u32 al = Allocator(ctx);
        VCall(ctx, al, 12, {al, old, 56});
        store<u32>(slot, 0);
    }
}

// A new 56-byte pose for skel (thread allocator; 0 on failure): bone
// arrays +4 / +16 (48-byte transforms) and +28 (u32 flags) sized to the
// bone count skel->28, float slots +44 to skel->64 (new ones zeroed).
u32 NewPose(PPUContext& ctx, u32 skel) {
    const u32 al = Allocator(ctx);
    const u32 p = static_cast<u32>(VCall(ctx, al, 8, {al, 56}));
    if (p != 0) {
        store<u32>(p + 0, skel);
        store<u32>(p + 4, 0);
        store<u32>(p + 8, 0);
        store<u32>(p + 12, kNotOwned);
        store<u32>(p + 16, 0);
        store<u32>(p + 20, 0);
        store<u32>(p + 24, kNotOwned);
        store<u32>(p + 28, 0);
        store<u32>(p + 32, 0);
        store<u32>(p + 36, kNotOwned);
        store<u8>(p + 40, 0);
        store<u8>(p + 41, 0);
        store<u32>(p + 44, 0);
        store<u32>(p + 48, 0);
        store<u32>(p + 52, kNotOwned);
        const s32 bones = static_cast<s32>(load<u32>(skel + 28));
        const s32 rounded = static_cast<s32>(static_cast<u32>(bones + 3) & ~3u);
        if (rounded > 0) Reserve(ctx, p + 16, rounded, 48);
        if (static_cast<s32>(load<u32>(p + 12) & kCapMask) < rounded) Reserve(ctx, p + 4, rounded, 48);
        if (static_cast<s32>(load<u32>(p + 36) & kCapMask) < rounded) Reserve(ctx, p + 28, rounded, 4);
        Grow(ctx, p + 16, bones, 48);
        store<u32>(p + 20, static_cast<u32>(bones));
        Grow(ctx, p + 4, bones, 48);
        store<u32>(p + 8, static_cast<u32>(bones));
        Grow(ctx, p + 28, bones, 4);
        store<u32>(p + 32, static_cast<u32>(bones));
        const s32 floats = static_cast<s32>(load<u32>(load<u32>(p + 0) + 64));
        const s32 f4 = static_cast<s32>(static_cast<u32>(floats + 3) & ~3u);
        if (static_cast<s32>(load<u32>(p + 52) & kCapMask) < f4) Reserve(ctx, p + 44, f4, 4);
        Grow(ctx, p + 44, floats, 4);
        const s32 have = static_cast<s32>(load<u32>(p + 48));
        const u32 base = load<u32>(p + 44) + static_cast<u32>(have) * 4;
        for (s32 k = 0; k < floats - have; ++k) store<u32>(base + 4 * static_cast<u32>(k), 0);
        store<u32>(p + 48, static_cast<u32>(floats));
    }
    return p;
}

struct V4 {
    u32 w[4];
};
V4 LoadV(u32 ea) { return {{load<u32>(ea), load<u32>(ea + 4), load<u32>(ea + 8), load<u32>(ea + 12)}}; }
V4 Splat(u32 v) { return {{v, v, v, v}}; }
V4 Sel(const V4& a, const V4& b, const V4& m) {
    V4 r;
    for (u32 k = 0; k < 4; ++k) r.w[k] = (a.w[k] & ~m.w[k]) | (b.w[k] & m.w[k]);
    return r;
}
V4 AndC(const V4& a, const V4& b) {
    V4 r;
    for (u32 k = 0; k < 4; ++k) r.w[k] = a.w[k] & ~b.w[k];
    return r;
}
V4 And(const V4& a, const V4& b) {
    V4 r;
    for (u32 k = 0; k < 4; ++k) r.w[k] = a.w[k] & b.w[k];
    return r;
}
void StoreV(u32 ea, const V4& v) {
    ea &= ~15u;
    for (u32 k = 0; k < 4; ++k) store<u32>(ea + 4 * k, v.w[k]);
}

// The control's play state: +112 = 1 / rate (rate above 2^-23, else
// ~FLT_MAX), state +120 = done when the phase +116 is 1.0.
void SetRate(u32 e, f64 rate, u32 runState, u32 doneState) {
    constexpr u32 K = kSayK;
    f64 inv = Lf(K + 8);
    if (rate > Lf(K + 4)) inv = Divs(Lf(K), rate);
    Sf(e + 112, inv);
    store<u32>(e + 120, Lf(e + 116) == Lf(K) ? doneState : runState);
}
}  // namespace

// Speaker_Say350958(Node* c, Anim* an, u8 reverse, -, -, -, s32 weights,
// u32 line; f32 weight, f32 rate, f32 blend) (name?): with an (else r3 =
// c): a node without its animated skeleton c->24 first binds the skeleton
// named by an->8 (SortedMap 0x0106BC14 of String_HashNoCase("%s")):
// destroys the pose c->28, makes the animated skeleton (rate 0.01) and a
// pose for it (as Node_Init350178), Pose_SetReference and the speaker's
// line; no such skeleton asks c->4->4->vfn[12]. Then (with c->24): unless
// an's u8 +40 is 1 the running controls (c->24->16, s32 count +20) turn
// around (phase +116 = 1 - phase for states 0 / 1) and take the rate
// (states 2 / 3). A new 160-byte control (thread allocator; u16 +4 = 160
// even on failure) is set up for an (f_00A4C600(p, an, 0, reverse ? -1 :
// 1)) into c->32, turned around in states 2 / 3, given the rate (state 0,
// or 1 at phase 1.0), the blend +8 and the weight +64; weights 0 / 1 / 2
// write its two mask vectors +80 / +96 from the constants; it joins the
// skeleton (f_00A45478) and the old one is released (AnimSkel_Release).
// A line hash makes c->vfn[12](c, SortedMap_Find76E0A8(line)). The flags
// c->12: bit 0x08000000 = reverse bit 0, 0x01000000 / 0x00800000 /
// 0x00400000 = weight > / < / == 0, bits 0x04000000 / 0x02000000 cleared.
// r3 = (weight == 0), f1 = 0.0.
DECOMP(00350958, Speaker_Say350958) {
    const u32 c = arg<u32>(ctx, 0), an = arg<u32>(ctx, 1);
    const u32 rev = arg<u32>(ctx, 2);
    const s32 wsel = static_cast<s32>(arg<u32>(ctx, 6));
    const u32 line = arg<u32>(ctx, 7);
    const f64 weight = ctx.fpr[1], rate = ctx.fpr[2], blend = ctx.fpr[3];
    Stack frame(ctx, 256);
    const u32 sp = frame.addr();
    constexpr u32 K = kSayK;
    if (an == 0) return;
    if (load<u32>(c + 24) == 0) {
        // ---- bind the skeleton named by an->8
        const u32 s = static_cast<u32>(call(ctx, sfn::String_Format, {kFmtS, load<u32>(an + 8) & ~1u,
                                                                      static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(an + 20))))}));
        store<u32>(sp + 116, static_cast<u32>(call(ctx, sfn::String_HashNoCase, {s})));
        const u32 skel = load<u32>(static_cast<u32>(call(ctx, sfn::SortedMap_Find76D894, {kSkelMap, sp + 116})));
        if (skel == 0) {
            const u32 o = load<u32>(load<u32>(c + 4) + 4);
            VCall(ctx, o, 12, {o});
        } else {
            DestroyPose(ctx, c + 28, true, false, true, false);
            u32 al = Allocator(ctx);
            const u32 as = static_cast<u32>(VCall(ctx, al, 8, {al, 48}));
            store<u16>(as + 4, 48);  // written even when the allocation failed
            if (as != 0) call(ctx, sfn::AnimSkel_Construct, {as, skel});
            store<u32>(c + 24, as);
            store<u32>(as + 32, 0x3C23D70Au);  // 0.01f (address 32 when the allocation failed)
            const u32 p = NewPose(ctx, skel);
            store<u32>(c + 28, p);
            call(ctx, sfn::Pose_SetReference, {p});
            if (const u32 spk = load<u32>(c + 4); spk != 0) call(ctx, sfn::Speaker_SetLine, {spk, skel, 0});
        }
    }
    const u32 skel = load<u32>(c + 24);
    if (skel == 0) return;
    if (load<u8>(an + 40) != 1) {
        for (s32 i = 0; i < static_cast<s32>(load<u32>(skel + 20)); ++i) {
            const u32 e = load<u32>(load<u32>(skel + 16) + 4 * static_cast<u32>(i));
            const u32 st = load<u32>(e + 120);
            if (st == 0 || st == 1) Sf(e + 116, S(Lf(K) - Lf(e + 116)));
            SetRate(e, rate, 2, 3);
            ctx.fpr[1] = Lf(e + 116);
        }
    }
    // ---- the new control
    const u32 al = Allocator(ctx);
    const u32 p = static_cast<u32>(VCall(ctx, al, 8, {al, 160}));
    store<u16>(p + 4, 160);  // written even when the allocation failed
    u32 ctl = 0;
    if (p != 0) {
        call(ctx, sfn::Control_Init, {p, an, 0, (rev & 0xFFu) != 0 ? ~0ull : 1ull});
        ctl = p;
    }
    store<u32>(c + 32, ctl);
    const u32 st = load<u32>(ctl + 120);
    if (st == 2 || st == 3) Sf(ctl + 116, S(Lf(K) - Lf(ctl + 116)));
    SetRate(ctl, rate, 0, 1);
    const u32 e = load<u32>(c + 32);
    Sf(e + 8, blend);
    Sf(e + 64, weight);
    const V4 v2 = Splat(load<u32>(K)), m4 = LoadV(K + 16), m6 = LoadV(K + 32), m5 = LoadV(K + 48), z = Splat(0);
    const u32 sk = load<u32>(c + 24);
    if (wsel == 0) {
        const V4 a = Sel(AndC(z, m4), v2, m5);
        StoreV(e + 80, Sel(a, v2, m6));
        StoreV(e + 96, AndC(AndC(Sel(v2, v2, m4), m5), m6));
    } else if (wsel == 2) {
        StoreV(e + 80, Sel(AndC(AndC(z, m4), m5), v2, m6));
        StoreV(e + 96, AndC(AndC(AndC(v2, m4), m5), m6));
    } else if (wsel == 1) {
        const V4 v3 = Splat(load<u32>(K + 64)), v7 = Splat(load<u32>(K + 68));
        const V4 v8 = And(v3, m4);
        const V4 a = Sel(v2, v7, m4);
        StoreV(e + 80, Sel(Sel(v8, v7, m5), v2, m6));
        StoreV(e + 96, AndC(Sel(a, v3, m5), m6));
    }
    call(ctx, sfn::AnimSkel_AddControl, {sk, e});
    call(ctx, sfn::AnimSkel_Release, {load<u32>(c + 32)});
    if (line != 0) {
        store<u32>(sp + 112, line);
        const u32 v = load<u32>(static_cast<u32>(call(ctx, sfn::SortedMap_Find76E0A8, {kLineMap, sp + 112})));
        VCallEO(ctx, c, 12, v);
    }
    const f64 zero = Lf(K + 12);
    u32 fl = load<u32>(c + 12);
    fl = (fl & ~0x08000000u) | ((rev & 1u) << 27);
    fl = (fl & ~0x01000000u) | (weight > zero ? 0x01000000u : 0u);
    fl = (fl & ~0x00800000u) | (weight < zero ? 0x00800000u : 0u);
    const bool eq = weight == zero;
    fl = (fl & ~0x00400000u) | (eq ? 0x00400000u : 0u);
    fl &= ~0x06000000u;
    store<u32>(c + 12, fl);
    ctx.fpr[1] = zero;
    ctx.gpr[3] = eq ? 1u : 0u;
}

namespace {
constexpr u32 kStrRChr = 0x00556014u;       // String_Thunk71EDAC(s, ch): the last ch in s or 0
constexpr u32 kStrLen = 0x00555FA4u;        // String_Length(s)
constexpr u32 kFindDescendant = 0x0034F600u;  // Obj_FindDescendant_0034F600(c, u32 hash, root)
constexpr u32 kSmallAlloc = 0x0055238Cu;
constexpr u32 kSmallFree = 0x00552420u;
constexpr u32 kRows = 0x01164BD0u;          // f32[4][4] identity
constexpr u32 kVecB30 = 0x01164B30u;        // f32[3]

// The 80-byte bone record at r: identity rows (3 x 4 with the position) and
// the vector 0x01164B30 at +64.
void InitBone(u32 r) {
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) Cw(r + 16 * row + 4 * k, kRows + 16 * row + 4 * k);
    for (u32 k = 0; k < 3; ++k) Cw(r + 64 + 4 * k, kVecB30 + 4 * k);
}
}  // namespace

// Speaker_SetLine34F690(Speaker* c, Skeleton* skel, u8 force) (name?):
// rebinds c to skel (unless already bound and not forced; r3 = c->8 when
// nothing is done): releases the animated skeleton c->8, destroys the poses
// c->12 / c->84, releases c->88, makes a new animated skeleton (rate 0.01)
// and pose (Pose_SetReference), frees the bone tables c->24 / c->20
// (SmallFree with sizes c->36 / c->32). Unless c->4->vfn[28](c->4): a kind
// 11 owner (bits 10..15 of c->4->4) gets one 80-byte bone record (c->20,
// identity) and c->24[0] = c->4, owner->160 = the record; otherwise one per
// skeleton bone (skel->28, names skel->24 in 8-byte entries, the part after
// the last ':'): bone 0 is the owner, others Obj_FindDescendant(c,
// hash(name), c->4); each found object gets +160 = its record.
DECOMP(0034F690, Speaker_SetLine34F690) {
    const u32 c = arg<u32>(ctx, 0), skel = arg<u32>(ctx, 1);
    const u32 force = arg<u32>(ctx, 2) & 0xFFu;
    const u32 as0 = load<u32>(c + 8);
    ctx.gpr[3] = as0;
    if (as0 != 0 && force == 0) return;
    if (skel == 0) return;
    if (as0 != 0) {
        call(ctx, sfn::AnimSkel_Release, {as0, skel});
        store<u32>(c + 8, 0);
    }
    DestroyPose(ctx, c + 12, false, false, false, true);
    DestroyPose(ctx, c + 84, true, true, true, false);
    if (const u32 a88 = load<u32>(c + 88); a88 != 0) {
        call(ctx, sfn::AnimSkel_Release, {a88});
        store<u32>(c + 88, 0);
    }
    const u32 al = Allocator(ctx);
    const u32 as = static_cast<u32>(VCall(ctx, al, 8, {al, 48}));
    store<u16>(as + 4, 48);  // written even when the allocation failed
    if (as != 0) call(ctx, sfn::AnimSkel_Construct, {as, skel});
    store<u32>(c + 8, as);
    store<u32>(as + 32, 0x3C23D70Au);  // 0.01f (address 32 when the allocation failed)
    const u32 p = NewPose(ctx, skel);
    store<u32>(c + 12, p);
    call(ctx, sfn::Pose_SetReference, {p});
    if (const u32 t = load<u32>(c + 24); t != 0) {
        call(ctx, kSmallFree, {t, load<u32>(c + 36)});
        store<u32>(c + 36, 0);
    }
    if (const u32 t = load<u32>(c + 20); t != 0) {
        call(ctx, kSmallFree, {t, load<u32>(c + 32)});
        store<u32>(c + 32, 0);
    }
    const u32 own = load<u32>(c + 4);
    if (VCall(ctx, own, 28, {own}) != 0) return;
    if (((load<u32>(load<u32>(c + 4) + 4) >> 10) & 0x3Fu) == 11) {
        store<u32>(c + 32, 80);
        store<u32>(c + 20, static_cast<u32>(call(ctx, kSmallAlloc, {80})));
        store<u32>(c + 36, 4);
        const u32 tab = static_cast<u32>(call(ctx, kSmallAlloc, {4}));
        store<u32>(c + 24, tab);
        const u32 rec = load<u32>(c + 20);
        const u32 o = load<u32>(c + 4);
        if (rec != 0) {
            InitBone(rec);
            ctx.fpr[1] = Lf(kVecB30 + 8);
        }
        store<u32>(o + 160, rec);
        store<u32>(tab, o);
        ctx.gpr[3] = tab;
        return;
    }
    const u32 n = load<u32>(skel + 28);
    store<u32>(c + 32, n * 80);
    store<u32>(c + 20, static_cast<u32>(call(ctx, kSmallAlloc, {n * 80})));
    store<u32>(c + 36, load<u32>(skel + 28) << 2);
    store<u32>(c + 24, static_cast<u32>(call(ctx, kSmallAlloc, {load<u32>(skel + 28) << 2})));
    for (s32 i = 0;; ++i) {
        const u32 cnt = load<u32>(skel + 28);
        ctx.gpr[3] = cnt;
        if (i >= static_cast<s32>(cnt)) return;
        u32 name = load<u32>(load<u32>(skel + 24) + 8 * static_cast<u32>(i)) & ~1u;
        if (const u32 colon = static_cast<u32>(call(ctx, kStrRChr, {name, 58})); colon != 0) {
            if (static_cast<s32>(call(ctx, kStrLen, {colon})) > 1) name = colon + 1;
        }
        u32 o;
        if (i != 0) {
            const u32 h = static_cast<u32>(call(ctx, sfn::String_HashNoCase, {name}));
            o = static_cast<u32>(call(ctx, kFindDescendant, {c, h, load<u32>(c + 4)}));
        } else {
            o = load<u32>(c + 4);
        }
        const u32 rec = load<u32>(c + 20) + 80 * static_cast<u32>(i);
        if (rec != 0) {
            InitBone(rec);
            ctx.fpr[1] = Lf(kVecB30 + 8);
        }
        store<u32>(load<u32>(c + 24) + 4 * static_cast<u32>(i), o);
        if (o != 0) store<u32>(o + 160, load<u32>(c + 20) + 80 * static_cast<u32>(i));
    }
}
