// Game code, large-function sweep part 21: a speaker node's line start (the
// animation control it builds and the skeleton binding it makes on first
// use).
#include "big21.h"

namespace sly4::obj {

namespace {
constexpr u32 kSayK = 0x00350910u;  // f32 1.0, 2^-23, ~FLT_MAX, 0.0; masks {0,-1,0,0} {0,0,0,-1} {0,0,-1,0}; f32 0.33, 0.66
constexpr u32 kFmtS = 0x00ADC098u;  // "%s"
constexpr u32 kSkelMap = 0x0106BC14u;  // skeletons by name hash (SortedMap_Find76D894)
constexpr u32 kLineMap = 0x0106BBCCu;  // (SortedMap_Find76E0A8)
constexpr u32 kHkMem = 0x011DF31Cu;    // Havok container allocator (vt +20: free(p, bytes))
constexpr u32 kNotOwned = 0x80000000u, kCapMask = 0x3FFFFFFFu;
constexpr u32 String_HashNoCase = 0x0056A710u;
constexpr u32 SortedMap_Find76D894 = 0x0076D894u;
constexpr u32 SortedMap_Find76E0A8 = 0x0076E0A8u;
constexpr u32 HkArray_Reserve = 0x007CCE88u;   // (allocator, hkArray*, s32 n, s32 elem size)
constexpr u32 AnimSkel_Release = 0x007D4EA8u;  // (AnimSkel*)
constexpr u32 AnimSkel_Construct = 0x00A45750u;  // (AnimSkel*, Skeleton*)
constexpr u32 AnimSkel_AddControl = 0x00A45478u;  // f_00A45478(AnimSkel*, control)
constexpr u32 Control_Init = 0x00A4C600u;      // f_00A4C600(control, anim, 0, s32 dir)
constexpr u32 Pose_SetReference = 0x00A56E68u;
constexpr u32 kStrRChr = 0x00556014u;       // String_Thunk71EDAC(s, ch): the last ch in s or 0
constexpr u32 kStrLen = 0x00555FA4u;        // String_Length(s)
constexpr u32 kFindDescendant = 0x0034F600u;  // Obj_FindDescendant_0034F600(c, u32 hash, root)
constexpr u32 kRows = 0x01164BD0u;          // f32[4][4] identity
constexpr u32 kVecB30 = 0x01164B30u;        // f32[3]

// A 56-byte pose: +0 the skeleton, hkArrays +4 / +16 (48-byte
// transforms), +28 (u32 flags), +44 (f32 slots).
struct Pose {
    be_u32 skel;            // +0
    HkArray<u8> local;      // +4
    HkArray<u8> model;      // +16
    HkArray<u8> flags;      // +28
    u8 b40, b41;            // +40 / +41
    pad<2> _42;
    HkArray<u8> floats;     // +44
};
static_assert(sizeof(Pose) == 56 && offsetof(Pose, flags) == 28 && offsetof(Pose, floats) == 44);
// An animated skeleton (48 bytes): +4 u16 size, +16 the controls, +20
// their count, +32 the rate.
struct AnimSkel {
    pad<4> _0;
    be_u16 size;            // +4
    pad<10> _6;
    be_u32 controls;        // +16: Control*[]
    be_s32 ncontrols;       // +20
    pad<8> _24;
    be_u32 rate;            // +32
};
// A 160-byte animation control.
struct Control {
    pad<4> _0;
    be_u16 size;            // +4
    pad<2> _6;
    be_f32 blend;           // +8
    pad<52> _12;
    be_f32 weight;          // +64
    pad<12> _68;
    Quad masks[2];          // +80 / +96
    be_f32 inv_rate;        // +112
    be_f32 phase;           // +116
    be_u32 state;           // +120
};
static_assert(offsetof(Control, weight) == 64 && offsetof(Control, masks) == 80 && offsetof(Control, state) == 120);
// A skeleton: +24 the bone names (8-byte entries), +28 the bone count,
// +64 the float slot count.
struct Skel {
    pad<24> _0;
    be_u32 names;           // +24
    be_s32 bones;           // +28
    pad<32> _32;
    be_s32 nfloats;         // +64
};
static_assert(offsetof(Skel, bones) == 28 && offsetof(Skel, nfloats) == 64);
// The speaker node (Speaker_Say350958): +4 the speaker, +12 flags, +24 the
// animated skeleton, +28 the pose, +32 the newest control.
struct SayNode {
    pad<4> _0;
    be_u32 speaker;         // +4
    pad<4> _8;
    be_u32 flags;           // +12
    pad<8> _16;
    be_u32 askel;           // +24
    be_u32 pose;            // +28
    be_u32 ctl;             // +32
};
// The speaker (Speaker_SetLine34F690): +4 the owner, +8 / +88 animated
// skeletons, +12 / +84 poses, +20 / +24 the bone tables (sizes +32 / +36).
struct Speaker {
    pad<4> _0;
    be_u32 owner;           // +4
    be_u32 askel;           // +8
    be_u32 pose;            // +12
    pad<4> _16;
    be_u32 recs;            // +20
    be_u32 tab;             // +24
    pad<4> _28;
    be_u32 recs_size;       // +32
    be_u32 tab_size;        // +36
    pad<44> _40;
    be_u32 pose2;           // +84
    be_u32 askel2;          // +88
};
static_assert(offsetof(Speaker, recs_size) == 32 && offsetof(Speaker, pose2) == 84);
// The 80-byte bone record.
struct BoneRec {
    Mtx44 m;                // +0
    Vec4 v64;               // +64
};
static_assert(sizeof(BoneRec) == 80);

f64 C(u32 off) { return lf(global<be_f32>(kSayK + off)); }
u32 Allocator() { return gm<be_u32>(gm<be_u32>(static_cast<u32>(ctx().gpr[13]) - 28332u) + 44); }

// kHkMem->vfn[20](kHkMem, p, bytes).
void HkFree(u32 p, u32 bytes) { vcall(&gm<u8>(kHkMem), 20, p, static_cast<s32>(bytes)); }

void Reserve(HkArray<u8>& arr, s32 n, u32 elem) { call(HkArray_Reserve, kHkMem, &arr, n, elem); }
void Grow(HkArray<u8>& arr, s32 n, u32 elem) {
    const u32 cap = arr.capflags;
    if (static_cast<s32>(cap & kCapMask) >= n) return;
    const s32 dbl = static_cast<s32>((cap << 1) & 0x7FFFFFFEu);
    Reserve(arr, dbl > n ? dbl : n, elem);
}

// Destroys the pose *slot (its hkArrays +44 / +28 / +16 / +4 freed through
// kHkMem when owned, cleared and marked not owned; the 56-byte block back
// to the thread allocator) and zeroes the slot.
void DestroyPose(be_u32& slot) {
    if (const u32 old = slot; old != 0) {
        Pose& p = gm<Pose>(old);
        u32 cap = p.floats.capflags;
        p.floats.size = 0;
        if ((cap & kNotOwned) == 0) HkFree(p.floats.data.ea, cap << 2);
        cap = p.flags.capflags;
        p.floats.data = nullptr;
        p.floats.capflags = kNotOwned;
        p.flags.size = 0;
        if ((cap & kNotOwned) == 0) HkFree(p.flags.data.ea, cap << 2);
        cap = p.model.capflags;
        p.flags.data = nullptr;
        p.flags.capflags = kNotOwned;
        p.model.size = 0;
        if ((cap & kNotOwned) == 0) HkFree(p.model.data.ea, (cap & kCapMask) * 48);
        cap = p.local.capflags;
        p.model.data = nullptr;
        p.model.capflags = kNotOwned;
        p.local.size = 0;
        if ((cap & kNotOwned) == 0) HkFree(p.local.data.ea, (cap & kCapMask) * 48);
        p.local.data = nullptr;
        p.local.capflags = kNotOwned;
        vcall(&gm<u8>(Allocator()), 12, old, 56u);
        slot = 0u;
    }
}

// A new 56-byte pose for skel (thread allocator; 0 on failure): bone
// arrays +4 / +16 (48-byte transforms) and +28 (u32 flags) sized to the
// bone count skel->28, float slots +44 to skel->64 (new ones zeroed).
u32 NewPose(u32 skel) {
    const u32 pe = vcall<u32>(&gm<u8>(Allocator()), 8, 56u);
    if (pe != 0) {
        Pose& p = gm<Pose>(pe);
        p.skel = skel;
        for (HkArray<u8>* a : {&p.local, &p.model, &p.flags, &p.floats}) {
            a->data = nullptr;
            a->size = 0;
            a->capflags = kNotOwned;
        }
        p.b40 = 0;
        p.b41 = 0;
        const s32 bones = gm<Skel>(skel).bones;
        const s32 rounded = static_cast<s32>(static_cast<u32>(bones + 3) & ~3u);
        if (rounded > 0) Reserve(p.model, rounded, 48);
        if (static_cast<s32>(p.local.capflags & kCapMask) < rounded) Reserve(p.local, rounded, 48);
        if (static_cast<s32>(p.flags.capflags & kCapMask) < rounded) Reserve(p.flags, rounded, 4);
        Grow(p.model, bones, 48);
        p.model.size = bones;
        Grow(p.local, bones, 48);
        p.local.size = bones;
        Grow(p.flags, bones, 4);
        p.flags.size = bones;
        const s32 floats = gm<Skel>(p.skel).nfloats;
        const s32 f4 = static_cast<s32>(static_cast<u32>(floats + 3) & ~3u);
        if (static_cast<s32>(p.floats.capflags & kCapMask) < f4) Reserve(p.floats, f4, 4);
        Grow(p.floats, floats, 4);
        const s32 have = p.floats.size;
        const u32 base = p.floats.data.ea + static_cast<u32>(have) * 4;
        for (s32 k = 0; k < floats - have; ++k) gm<be_u32>(base + 4 * static_cast<u32>(k)) = 0u;
        p.floats.size = floats;
    }
    return pe;
}

// A new animated skeleton for skel (48 bytes, thread allocator; u16 +4 and
// the rate 0.01 written even when the allocation failed).
u32 NewAnimSkel(u32 skel) {
    const u32 as = vcall<u32>(&gm<u8>(Allocator()), 8, 48u);
    gm<AnimSkel>(as).size = 48;
    if (as != 0) call(AnimSkel_Construct, as, skel);
    return as;
}

// Four words (the mask arithmetic is on raw bits).
struct W4 {
    u32 w[4];
};
W4 LoadW(u32 ea) {
    const Quad& q = gm<Quad>(ea);
    return {{q.w[0], q.w[1], q.w[2], q.w[3]}};
}
W4 SplatW4(u32 v) { return {{v, v, v, v}}; }
W4 SelW(const W4& a, const W4& b, const W4& m) {
    W4 r;
    for (u32 k = 0; k < 4; ++k) r.w[k] = (a.w[k] & ~m.w[k]) | (b.w[k] & m.w[k]);
    return r;
}
W4 AndCW(const W4& a, const W4& b) {
    W4 r;
    for (u32 k = 0; k < 4; ++k) r.w[k] = a.w[k] & ~b.w[k];
    return r;
}
W4 AndW(const W4& a, const W4& b) {
    W4 r;
    for (u32 k = 0; k < 4; ++k) r.w[k] = a.w[k] & b.w[k];
    return r;
}
void StoreW(Quad& q0, const W4& v) {
    Quad& q = gm<Quad>(guest(&q0) & ~15u);
    for (u32 k = 0; k < 4; ++k) q.w[k] = v.w[k];
}

// The control's play state: +112 = 1 / rate (rate above 2^-23, else
// ~FLT_MAX), state +120 = done when the phase +116 is 1.0.
void SetRate(Control& e, f64 rate, u32 runState, u32 doneState) {
    f64 inv = C(8);
    if (rate > C(4)) inv = Divs(C(0), rate);
    e.inv_rate = static_cast<f32>(inv);
    e.state = lf(e.phase) == C(0) ? doneState : runState;
}

// The 80-byte bone record at r: identity rows (3 x 4 with the position) and
// the vector 0x01164B30 at +64.
void InitBone(BoneRec& r) {
    CopyMtxXYZ(r.m, global<Mtx44>(kRows));
    CopyXYZ(r.v64, global<Vec3>(kVecB30));
}
}  // namespace

// void Speaker_SetLine34F690(Speaker* c, Skeleton* skel, u8 force) (name?)
// (no caller reads r3): rebinds c to skel (unless already bound and not
// forced): releases the animated skeleton c->8, destroys the poses
// c->12 / c->84, releases c->88, makes a new animated skeleton (rate 0.01)
// and pose (Pose_SetReference), frees the bone tables c->24 / c->20
// (SmallFree with sizes c->36 / c->32). Unless c->4->vfn[28](c->4): a kind
// 11 owner (bits 10..15 of c->4->4) gets one 80-byte bone record (c->20,
// identity) and c->24[0] = c->4, owner->160 = the record; otherwise one per
// skeleton bone (skel->28, names skel->24 in 8-byte entries, the part after
// the last ':'): bone 0 is the owner, others Obj_FindDescendant(c,
// hash(name), c->4); each found object gets +160 = its record.
void Speaker_SetLine34F690(Speaker* c, u32 skel, u8 force) {
    const u32 as0 = c->askel;
    if (as0 != 0 && force == 0) return;
    if (skel == 0) return;
    if (as0 != 0) {
        call(AnimSkel_Release, as0, skel);
        c->askel = 0u;
    }
    DestroyPose(c->pose);
    DestroyPose(c->pose2);
    if (const u32 a88 = c->askel2; a88 != 0) {
        call(AnimSkel_Release, a88);
        c->askel2 = 0u;
    }
    const u32 as = NewAnimSkel(skel);
    c->askel = as;
    gm<AnimSkel>(as).rate = 0x3C23D70Au;  // 0.01f (address 32 when the allocation failed)
    const u32 p = NewPose(skel);
    c->pose = p;
    call(Pose_SetReference, p);
    if (const u32 t = c->tab; t != 0) {
        call(fn::SmallFree, t, c->tab_size.get());
        c->tab_size = 0u;
    }
    if (const u32 t = c->recs; t != 0) {
        call(fn::SmallFree, t, c->recs_size.get());
        c->recs_size = 0u;
    }
    if (vcall<u64>(&gm<u8>(c->owner), 28) != 0) return;
    if (((gm<be_u32>(c->owner + 4) >> 10) & 0x3Fu) == 11) {
        c->recs_size = 80u;
        c->recs = call<u32>(fn::SmallAlloc, 80u);
        c->tab_size = 4u;
        const u32 tab = call<u32>(fn::SmallAlloc, 4u);
        c->tab = tab;
        const u32 rec = c->recs;
        const u32 o = c->owner;
        if (rec != 0) InitBone(gm<BoneRec>(rec));
        gm<be_u32>(o + 160) = rec;
        gm<be_u32>(tab) = o;
        return;
    }
    Skel& sk = gm<Skel>(skel);
    const u32 n = static_cast<u32>(sk.bones.get());
    c->recs_size = n * 80;
    c->recs = call<u32>(fn::SmallAlloc, n * 80);
    c->tab_size = static_cast<u32>(sk.bones.get()) << 2;
    c->tab = call<u32>(fn::SmallAlloc, static_cast<u32>(sk.bones.get()) << 2);
    for (s32 i = 0; i < sk.bones; ++i) {
        u32 name = gm<be_u32>(sk.names + 8 * static_cast<u32>(i)) & ~1u;
        if (const u32 colon = call<u32>(kStrRChr, name, 58u); colon != 0) {
            if (call<s32>(kStrLen, colon) > 1) name = colon + 1;
        }
        u32 o;
        if (i != 0) {
            const u32 h = call<u32>(String_HashNoCase, name);
            o = call<u32>(kFindDescendant, c, h, c->owner.get());
        } else {
            o = c->owner;
        }
        const u32 rec = c->recs + 80 * static_cast<u32>(i);
        if (rec != 0) InitBone(gm<BoneRec>(rec));
        gm<be_u32>(c->tab + 4 * static_cast<u32>(i)) = o;
        if (o != 0) gm<be_u32>(o + 160) = c->recs + 80 * static_cast<u32>(i);
    }
}

// void Speaker_Say350958(Node* c, Anim* an, u8 reverse, -, -, -, s32
// weights, u32 line; f32 weight, f32 rate, f32 blend) (name?) (no caller
// reads r3 or f1): with an: a node without its animated skeleton c->24 first binds the skeleton
// named by an->8 (SortedMap 0x0106BC14 of String_HashNoCase("%s")):
// destroys the pose c->28, makes the animated skeleton (rate 0.01) and a
// pose for it (as Node_Init350178), Pose_SetReference and the speaker's
// line (Speaker_SetLine34F690 below); no such skeleton asks c->4->4->vfn[12]. Then (with c->24): unless
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
void Speaker_Say350958(SayNode* c, u32 an, u32 rev, u32, u32, u32, s32 wsel, u32 line, f64 weight, f64 rate, f64 blend) {
    GuestFrame frame(256);
    be_u32* keys = frame.as<be_u32>(112);  // sp+112 the line key, sp+116 the skeleton key
    if (an == 0) return;
    if (c->askel == 0u) {
        // ---- bind the skeleton named by an->8
        const u32 s = call<u32>(fn::String_Format, kFmtS, gm<be_u32>(an + 8) & ~1u, gm<be_s32>(an + 20).get());
        keys[1] = call<u32>(String_HashNoCase, s);
        const u32 skel = gm<be_u32>(call<u32>(SortedMap_Find76D894, kSkelMap, &keys[1]));
        if (skel == 0) {
            vcall(&gm<u8>(gm<be_u32>(c->speaker + 4)), 12);
        } else {
            DestroyPose(c->pose);
            const u32 as = NewAnimSkel(skel);
            c->askel = as;
            gm<AnimSkel>(as).rate = 0x3C23D70Au;  // 0.01f (address 32 when the allocation failed)
            const u32 p = NewPose(skel);
            c->pose = p;
            call(Pose_SetReference, p);
            if (const u32 spk = c->speaker; spk != 0) Speaker_SetLine34F690(&gm<Speaker>(spk), skel, 0);
        }
    }
    const u32 skel = c->askel;
    if (skel == 0) return;
    if (gm<u8>(an + 40) != 1) {
        AnimSkel& as = gm<AnimSkel>(skel);
        for (s32 i = 0; i < as.ncontrols; ++i) {
            Control& e = gm<Control>(gm<be_u32>(as.controls + 4 * static_cast<u32>(i)));
            const u32 st = e.state;
            if (st == 0 || st == 1) e.phase = static_cast<f32>(S(C(0) - lf(e.phase)));
            SetRate(e, rate, 2, 3);
        }
    }
    // ---- the new control
    const u32 p = vcall<u32>(&gm<u8>(Allocator()), 8, 160u);
    gm<Control>(p).size = 160;  // written even when the allocation failed
    u32 ctl = 0;
    if (p != 0) {
        call(Control_Init, p, an, 0u, (rev & 0xFFu) != 0 ? -1 : 1);
        ctl = p;
    }
    c->ctl = ctl;
    Control& nc = gm<Control>(ctl);
    const u32 st = nc.state;
    if (st == 2 || st == 3) nc.phase = static_cast<f32>(S(C(0) - lf(nc.phase)));
    SetRate(nc, rate, 0, 1);
    Control& e = gm<Control>(c->ctl);
    e.blend = static_cast<f32>(blend);
    e.weight = static_cast<f32>(weight);
    const W4 v2 = SplatW4(global<be_u32>(kSayK)), m4 = LoadW(kSayK + 16), m6 = LoadW(kSayK + 32), m5 = LoadW(kSayK + 48), z = SplatW4(0);
    const u32 sk = c->askel;
    if (wsel == 0) {
        const W4 a = SelW(AndCW(z, m4), v2, m5);
        StoreW(e.masks[0], SelW(a, v2, m6));
        StoreW(e.masks[1], AndCW(AndCW(SelW(v2, v2, m4), m5), m6));
    } else if (wsel == 2) {
        StoreW(e.masks[0], SelW(AndCW(AndCW(z, m4), m5), v2, m6));
        StoreW(e.masks[1], AndCW(AndCW(AndCW(v2, m4), m5), m6));
    } else if (wsel == 1) {
        const W4 v3 = SplatW4(global<be_u32>(kSayK + 64)), v7 = SplatW4(global<be_u32>(kSayK + 68));
        const W4 v8 = AndW(v3, m4);
        const W4 a = SelW(v2, v7, m4);
        StoreW(e.masks[0], SelW(SelW(v8, v7, m5), v2, m6));
        StoreW(e.masks[1], AndCW(SelW(a, v3, m5), m6));
    }
    call(AnimSkel_AddControl, sk, &e);
    call(AnimSkel_Release, c->ctl.get());
    if (line != 0) {
        keys[0] = line;
        const u32 v = gm<be_u32>(call<u32>(SortedMap_Find76E0A8, kLineMap, &keys[0]));
        vcall(c, 12, v);
    }
    const f64 zero = C(12);
    u32 fl = c->flags;
    fl = (fl & ~0x08000000u) | ((rev & 1u) << 27);
    fl = (fl & ~0x01000000u) | (weight > zero ? 0x01000000u : 0u);
    fl = (fl & ~0x00800000u) | (weight < zero ? 0x00800000u : 0u);
    fl = (fl & ~0x00400000u) | (weight == zero ? 0x00400000u : 0u);
    fl &= ~0x06000000u;
    c->flags = fl;
}

}  // namespace sly4::obj

NATIVE(0034F690, sly4::obj::Speaker_SetLine34F690)
NATIVE(00350958, sly4::obj::Speaker_Say350958)
