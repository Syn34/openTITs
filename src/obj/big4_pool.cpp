// Game code, large-function sweep part 4: the constructors of the five
// object kinds ObjPool_Create (002F0E0C) makes, and the handle pool's
// growth HandlePool_Grow.
#include "big4.h"

using namespace sly4;
using namespace sly4::big4;

namespace b4p {
constexpr u32 kCurrentWorld = 0x00F65CA0u;  // u32 World*
constexpr u32 kVtbl9965 = 0x00B1D578u;
// World->+0x128168 -> +656 -> +36: the fog / environment settings block
// the 0x996564D0 kind writes (name?).
constexpr u32 kWorldEnvList = 0x00128168u;
constexpr u32 kVtbl489E = 0x00B1D5A0u;
constexpr u32 kZeroF = 0x002EF930u;     // f32 0.0
constexpr u32 kDegToRad = 0x002EF934u;  // f32 pi / 180
constexpr u32 kDefaultRange = 0x010688E8u + 72;  // f32 tunable: the range used for a negative one
constexpr u32 kFindByIdOpd = 0x00F361F8u;  // OPD of 002F5D6C (node, {id, out}): matches node->+204 == id
constexpr u32 kVtblD0E9 = 0x00B1D500u;
constexpr u32 FxGeo_SetDef = 0x002EC2F8u;  // (o, GeomDef*) (name?)
constexpr u32 kFmtNoGeo = 0x00AD9614u;   // "Fail to find geo def %s for FXAnimControlGeoDisplay!"
constexpr u32 kFmtNoBone = 0x00AD964Cu;  // "Fail to find bone (%s) from entity (%s)"
constexpr u32 kIdentity = 0x01164BD0u;   // f32[16] identity matrix
constexpr u32 kVtbl8733Base = 0x00B1D4D8u, kVtbl8733 = 0x00B1D528u;
constexpr u32 Obj8733_Init = 0x002ED468u;     // (o): builds the bone tables (name?)
constexpr u32 WorldClass_Copy = 0x005858D4u;  // (cls) -> copy
constexpr u32 Def_SetName = 0x00584BDCu;      // (def, name, bool hash)
constexpr u32 kScale = 0x00AD9690u;           // "Scale"
constexpr u32 kVtblEB2F = 0x00B1D550u;
constexpr u32 Obj_BoneMatrix = 0x002EFBF0u;  // (f32 out[16], ObjEB2F*, const f32 parent[16], WorldObj*, s32 bone) (name?)
constexpr u32 Tex_FindLoaded = 0x0050C398u;  // (name) -> Texture* or 0
constexpr u32 Tex_GetDefault = 0x004FEF30u;  // (u32 which) -> Texture*
constexpr u32 Tex_Acquire = 0x00523B7Cu;     // (Texture*) -> new AglMaterial on it
constexpr u32 WorldObj_FindById = 0x004A5AE8u;  // (WorldObj*, s32 id) -> WorldObj*
constexpr u32 Matrix_SplitScale = 0x00554858u;  // (m, f32 rot[16], f32 scale[3]); w lanes unwritten
constexpr u32 Quat_FromMatrix = 0x00555478u;    // (f32 out[4], m)
constexpr u32 Vec4_TransformByMatrix = 0x00553910u;  // (f32 out[4], const f32 v[4], const f32 m[16])
constexpr u32 kZero3 = 0x01164B40u;          // f32[3] zero vector
constexpr u32 kDefColor = 0x01164CA0u;       // 4 bytes
constexpr u32 kSerialEB2F = 0x01068B64u;     // u32: instance counter (its low bits go to flag bits 6..5)
constexpr u32 kTuneLen = 0x01068988u + 72;   // f32: the length used when d->+156 <= 0
constexpr u32 kTuneRate = 0x01068AC8u + 72;  // f32: the rate used when d->+148 < 0
constexpr u32 kTexMode = 0x010689D8u + 72;   // u32: 0 = look the texture up by name, 2 = default 0
constexpr u32 kK = 0x002F0020u;  // f32 1.0, 0.0, FLT_MAX, 1e-5, 1e6; +24 f64 0.0; +32 f32 576.0
constexpr u32 kEnvDirtyA = 0x01068841u, kEnvDirtyB = 0x01068840u;  // u8 flags set to 1
}  // namespace b4p

namespace {
inline f32 F(u32 w) { return std::bit_cast<f32>(w); }
// fsel(x - y, ge, lt) with the difference rounded to single, on raw
// words (a NaN difference picks lt).
inline u32 FselDiff(u32 x, u32 y, u32 ge, u32 lt) {
    const f32 d = static_cast<f32>(static_cast<f64>(F(x)) - static_cast<f64>(F(y)));
    return d >= 0.0f ? ge : lt;
}
// (f32)(a * b) of two raw words, as fmuls.
inline f32 MulS(u32 a, u32 b) { return static_cast<f32>(static_cast<f64>(F(a)) * static_cast<f64>(F(b))); }
}  // namespace

// void PoolObj9965_Construct(Obj9965* o, const Desc* d): the 20-byte
// kind 0x996564D0. +0 vtable 0x00B1D578, +4 d->+4, +8 (u8) 0, +12 the
// ref d->+8 (its u16 +4 += 1). With a ref, the world's environment block
// (World->+0x128168->+656->+36) gets +96..+108 d->+32/+36/+40/+16, +20
// d->+12 clamped to [+24, +28] (two fsels: min with +28 first, then max
// with +24), +112..+132 d->+48..+68, +136 d->+72 squared, and the two
// dirty bytes 0x01068841 / 0x01068840 are set; +16 / +17 are first
// zeroed. Then +16 / +17 = the bytes d->+76 / +77. f1 is left as the
// last float op leaves it.
DECOMP(002F0CD8, PoolObj9965_Construct) {
    using namespace b4p;
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    store<u8>(o + 8, 0);
    CopyW(o + 4, d + 4);
    store<u32>(o + 0, kVtbl9965);
    const u32 ref = load<u32>(d + 8);
    store<u32>(o + 12, ref);
    u64 f1 = LfsBits(load<u32>(d + 4));
    if (ref != 0) {
        store<u16>(ref + 4, static_cast<u16>(load<u16>(ref + 4) + 1));
        store<u8>(o + 17, 0);
        store<u8>(o + 16, 0);
        const u32 env = load<u32>(load<u32>(load<u32>(load<u32>(kCurrentWorld) + kWorldEnvList) + 656) + 36);
        const u32 hi = load<u32>(env + 28), lo = load<u32>(env + 24), a = load<u32>(d + 12);
        const u32 v = FselDiff(hi, a, a, hi);  // min(d->12, env->28)
        const u32 w16 = load<u32>(d + 16);
        CopyW(env + 96, d + 32);
        CopyW(env + 100, d + 36);
        CopyW(env + 104, d + 40);
        store<u32>(env + 108, w16);
        store<u32>(env + 20, FselDiff(v, lo, v, lo));  // max(.., env->24)
        CopyWords(env + 112, d + 48, 6);
        const f32 x = F(load<u32>(d + 72));
        const f32 sq = static_cast<f32>(static_cast<f64>(x) * static_cast<f64>(x));
        store<f32>(env + 136, sq);
        f1 = std::bit_cast<u64>(static_cast<f64>(sq));
        store<u8>(kEnvDirtyA, 1);
        store<u8>(kEnvDirtyB, 1);
    }
    store<u8>(o + 16, load<u8>(d + 76));
    store<u8>(o + 17, load<u8>(d + 77));
    ctx.fpr[1] = std::bit_cast<f64>(f1);
    ret(ctx, o);
}

// void PoolObj489E_Construct(Obj489E* o, const Desc* d): the 132-byte
// kind 0x489EBD81 (an object attached to a bone of the ref's skeleton).
// +0 vtable 0x00B1D5A0, +4 d->+4, +8 (u8) 0, +16..+28 0, +12 the ref
// d->+8, +32..+48 d->+16..+32 (+44 a raw word), +52..+60 d->+36..+44 in
// degrees times pi/180, +64..+120 d->+48..+104, +124 the range d->+108
// squared (the tunable at 0x01068930 when negative), +128..+131 the bytes
// d->+112... With a ref and a bone hash d->+12: the ref's skeleton
// (vfn[28](ref)) is searched for the bone: +20 = &skel->+32[bone]
// (16-byte entries) and, when that bone's parent index (s16 at
// bone->+0->+100) is positive, +24 = &entries[parent]. Without a bone,
// when ref->+272 is set and ref->+204 isn't the hash, WorldObj_ForEach(ref,
// 002F5D6C, {hash, 0}, 0) looks for the object with id hash and +16 gets
// what it found. Last the ref's u16 +4 += 1 (through a null ref too, as
// the original). Returns with r3 = ref + 4.
DECOMP(002EF938, PoolObj489E_Construct) {
    using namespace b4p;
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    CopyW(o + 4, d + 4);
    store<u8>(o + 8, 0);
    store<u32>(o + 0, kVtbl489E);
    for (u32 k = 16; k <= 28; k += 4) store<u32>(o + k, 0);
    const u32 ref = load<u32>(d + 8);
    CopyW(o + 32, d + 16);
    store<u32>(o + 12, ref);
    CopyW(o + 36, d + 20);
    CopyW(o + 40, d + 24);
    CopyW(o + 44, d + 28);
    CopyW(o + 48, d + 32);
    const u32 k = load<u32>(kDegToRad);
    for (u32 j = 0; j < 3; ++j) store<f32>(o + 52 + 4 * j, MulS(load<u32>(d + 36 + 4 * j), k));
    CopyWords(o + 64, d + 48, 15);
    u32 range = load<u32>(d + 108);
    const u32 bone = load<u32>(d + 12);
    if (static_cast<f64>(F(range)) < static_cast<f64>(F(load<u32>(kZeroF)))) range = load<u32>(kDefaultRange);
    const f32 r2 = MulS(range, range);
    for (u32 j = 0; j < 4; ++j) store<u8>(o + 128 + j, load<u8>(d + 112 + j));
    store<f32>(o + 124, r2);
    ctx.fpr[1] = static_cast<f64>(r2);  // (what f1 holds unless a call below changes it)
    if (ref != 0 && bone != 0) {
        const u32 skel = static_cast<u32>(VCall(ctx, ref, 28, {ref}));
        if (skel != 0) {
            const s32 idx = static_cast<s32>(call(ctx, b4fn::Skeleton_FindBone, {skel, load<u32>(d + 12)}));
            if (idx >= 0) {
                const u32 ents = load<u32>(skel + 32);
                const u32 e = ents + (static_cast<u32>(idx) << 4);
                store<u32>(o + 20, e);
                if (e != 0) {
                    const s32 parent = static_cast<s16>(load<u16>(load<u32>(e) + 100));
                    if (parent > 0) store<u32>(o + 24, ents + (static_cast<u32>(parent) << 4));
                }
            }
        }
        if (load<u32>(o + 20) == 0) {
            const u32 r = load<u32>(o + 12);
            if (load<u32>(r + 272) != 0) {
                const u32 id = load<u32>(d + 12);
                if (load<u32>(r + 204) != id) {
                    Stack frame(ctx, 16);
                    const u32 q = frame.addr();  // {id, found}
                    store<u32>(q + 0, id);
                    store<u32>(q + 4, 0);
                    call(ctx, b4fn::WorldObj_ForEach, {r, kFindByIdOpd, q, 0});
                    store<u32>(o + 16, load<u32>(q + 4));
                }
            }
        }
    }
    const u32 rc = load<u32>(o + 12) + 4;
    store<u16>(rc, static_cast<u16>(load<u16>(rc) + 1));
    ret(ctx, rc);
}

// void PoolObjD0E9_Construct(ObjD0E9* o, const Desc* d): the 208-byte
// kind 0xD0E940EF (FXAnimControlGeoDisplay: a geometry shown on an
// entity's bone). +0 vtable 0x00B1D500, +4 d->+4, +8 / +112 (u8) 0,
// +12 / +20..+36 / +116 0, +16 the entity d->+12 (its u16 +178 += 1).
// A geometry name d->+8 is looked up by its case-insensitive hash in the
// geometry class's map: +12 = the def, then FxGeo_SetDef(o, def), or the
// "Fail to find geo def" message (String_Format only) when absent. +48..
// the 3x4 rows of the matrix d->+16 (the identity when null). With a bone
// name d->+20, an entity with a model (+160) whose skeleton (vfn[28]) is
// non-null: +116 = &skeleton->+32[bone] and +120 (u8) d->+24, or the
// "Fail to find bone" message with the entity's name (vfn[76]). The
// skeleton is fetched again for each use, as the original does.
DECOMP(002EC920, PoolObjD0E9_Construct) {
    using namespace b4p;
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    store<u8>(o + 8, 0);
    CopyW(o + 4, d + 4);
    store<u32>(o + 0, kVtblD0E9);
    for (const u32 k : {12u, 24u, 32u, 20u}) store<u32>(o + k, 0);
    store<u8>(o + 112, 0);
    for (const u32 k : {116u, 28u, 36u}) store<u32>(o + k, 0);
    const u32 ent = load<u32>(d + 12);
    store<u32>(o + 16, ent);
    const u32 name = load<u32>(d + 8);
    if (ent != 0) store<u16>(ent + 178, static_cast<u16>(load<u16>(ent + 178) + 1));
    if (name != 0) {
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), static_cast<u32>(call(ctx, b4fn::String_HashNoCase, {name})));
        const u32 slot = static_cast<u32>(call(ctx, b4fn::HashMap_Find2, {load<u32>(load<u32>(b4fn::kGeomDefClass) + 16), frame.addr()}));
        const u32 def = load<u32>(slot);
        store<u32>(o + 12, def);
        if (def != 0) call(ctx, FxGeo_SetDef, {o, def});
        else call(ctx, b4fn::String_Format, {kFmtNoGeo, load<u32>(d + 8)});
    }
    const u32 m = load<u32>(d + 16);
    const u32 src = m != 0 ? m : kIdentity;
    for (u32 row = 0; row < 4; ++row) CopyWords(o + 48 + 16 * row, src + 16 * row, 3);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(src + 56)));  // f1 as the copy leaves it
    u32 r3 = 0;
    if (load<u32>(d + 20) != 0) {
        const u32 model = load<u32>(load<u32>(o + 16) + 160);
        if (model != 0 && static_cast<u32>(VCall(ctx, model, 28, {model})) != 0) {
            const u32 m2 = load<u32>(load<u32>(o + 16) + 160);
            const u32 skel = static_cast<u32>(VCall(ctx, m2, 28, {m2}));
            const u32 hash = static_cast<u32>(call(ctx, b4fn::String_HashNoCase, {load<u32>(d + 20)}));
            const s32 idx = static_cast<s32>(call(ctx, b4fn::Skeleton_FindBone, {skel, hash}));
            if (idx < 0) {
                const u32 e = load<u32>(o + 16);
                const u32 bone_name = load<u32>(d + 20);
                const u32 ent_name = static_cast<u32>(VCall(ctx, e, 76, {e}));
                r3 = static_cast<u32>(call(ctx, b4fn::String_Format, {kFmtNoBone, bone_name, ent_name}));
            } else {
                const u32 m3 = load<u32>(load<u32>(o + 16) + 160);
                const u32 s3 = static_cast<u32>(VCall(ctx, m3, 28, {m3}));
                r3 = load<u32>(s3 + 32) + (static_cast<u32>(idx) << 4);
                store<u8>(o + 120, load<u8>(d + 24));
                store<u32>(o + 116, r3);
            }
        }
    }
    ret(ctx, r3);
}

// void PoolObj8733_Construct(Obj8733* o, const Desc* d): the 156-byte
// kind 0x8733FD75 (a skinned instance of a model). +0 the base vtable
// 0x00B1D4D8 then 0x00B1D528, +4 d->+4, +8 (u8) 0, +12..+20 / +32..+48
// 0, u16s +24 0 / +26 1 / +28 / +30 0, +52..+112 eight (0, 1.0f) pairs,
// then memmoved over from d->+24 (64 bytes). The flags u16 +24: bits
// 5..1 = d->+20 bits 4..0, bit 0 = d->+16 bit 0; +148 (u8) d->+88; +12
// the model d->+8.
// With a model: its u16 +4 += 1; the flags' bone count (bits 15..6) =
// the model skeleton's u16 +28 when it has one (vfn[28]); with flag bit
// 0 the model's vfn[40](m, 0, 0); +16 = the model, or d->+12 when that
// one's skeleton has the same bone count (otherwise vfn[12] on d->+12 and
// on the model). The u16 +4 of +16 += 1 then, and with +148 set and a
// kind-8 object (+4 bits 15..10), +20 = WorldClass_Copy(its +208) named
// "Scale", whose per-part flag byte (part->+132->+92 + 11) is cleared.
// Finally +116..+144 0, +26 = max(d->+18, 1), one Mem_Alloc(n * bones *
// 64 + n * 68, 0, 16) split into +32 / +36 (n * 64 bytes), +40 (n *
// bones * 64), +44, and Obj8733_Init(o) (whose r3 / f1 are returned).
DECOMP(002ED6F4, PoolObj8733_Construct) {
    using namespace b4p;
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    store<u32>(o + 0, kVtbl8733Base);
    store<u8>(o + 8, 0);
    CopyW(o + 4, d + 4);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(d + 4)));  // (f1 until a callee changes it)
    store<u32>(o + 0, kVtbl8733);
    for (const u32 k : {12u, 16u, 20u}) store<u32>(o + k, 0);
    store<u16>(o + 24, 0);
    store<u16>(o + 26, 1);
    store<u16>(o + 28, 0);
    store<u16>(o + 30, 0);
    for (u32 k = 32; k <= 48; k += 4) store<u32>(o + k, 0);
    for (u32 k = 52; k < 116; k += 8) {
        store<u32>(o + k, 0);
        store<u32>(o + k + 4, 0x3F800000u);
    }
    call(ctx, b4fn::kMemmove, {o + 52, d + 24, 64});
    u32 f = load<u16>(o + 24);
    f = (f & ~0x3Eu) | ((static_cast<u32>(load<u16>(d + 20)) << 1) & 0x3Eu);
    store<u16>(o + 24, static_cast<u16>(f));
    f = (f & 0xFFFEu) | (load<u8>(d + 16) & 1u);
    store<u16>(o + 24, static_cast<u16>(f));
    store<u8>(o + 148, load<u8>(d + 88));
    const u32 model = load<u32>(d + 8);
    store<u32>(o + 12, model);
    if (model != 0) {
        store<u16>(model + 4, static_cast<u16>(load<u16>(model + 4) + 1));
        if (static_cast<u32>(VCall(ctx, model, 28, {model})) != 0) {
            const u32 m = load<u32>(o + 12);
            const u32 skel = static_cast<u32>(VCall(ctx, m, 28, {m}));
            const u32 v = load<u16>(o + 24);
            store<u16>(o + 24, static_cast<u16>((v & ~0xFFC0u) | ((static_cast<u32>(load<u16>(skel + 28)) << 6) & 0xFFC0u)));
        }
        if ((load<u16>(o + 24) & 1u) != 0) {
            const u32 m = load<u32>(o + 12);
            VCall(ctx, m, 40, {m, 0, 0});
        }
        u32 tgt = load<u32>(o + 12);
        store<u32>(o + 16, tgt);
        const u32 other = load<u32>(d + 12);
        if (other != 0) {
            bool same = false;
            if (static_cast<u32>(VCall(ctx, other, 28, {other})) != 0) {
                const u32 a = load<u32>(d + 12);
                const u32 skel = static_cast<u32>(VCall(ctx, a, 28, {a}));
                same = load<u16>(skel + 28) == ((static_cast<u32>(load<u16>(o + 24)) >> 6) & 0x3FFu);
            }
            if (same) {
                tgt = load<u32>(d + 12);
                store<u32>(o + 16, tgt);
            } else {
                const u32 a = load<u32>(d + 12);
                VCall(ctx, a, 12, {a});
                const u32 m = load<u32>(o + 12);
                VCall(ctx, m, 12, {m});
                tgt = load<u32>(o + 16);
            }
        }
        store<u16>(tgt + 4, static_cast<u16>(load<u16>(tgt + 4) + 1));
        if (load<u8>(o + 148) != 0 && ((load<u32>(tgt + 4) >> 10) & 0x3Fu) == 8) {
            const u32 cls = static_cast<u32>(call(ctx, WorldClass_Copy, {load<u32>(tgt + 208)}));
            store<u32>(o + 20, cls);
            call(ctx, Def_SetName, {cls, kScale, 1});
            const u32 c = load<u32>(o + 20);
            const u32 n = load<u8>(c + 112);
            for (u32 i = 0; i < n; ++i) {
                const u32 part = load<u32>(c + 8 * i + 132);
                if (part == 0) continue;
                const u32 q = load<u32>(part + 92);
                if (q != 0) store<u8>(q + 8 + 3, 0);
            }
        }
    }
    for (u32 k = 116; k < 148; k += 4) store<u32>(o + k, 0);
    const u32 n = load<u16>(d + 18);
    const u32 cnt = static_cast<s32>(n) < 1 ? 1u : n;
    store<u16>(o + 26, static_cast<u16>(cnt));
    const u32 flags = load<u16>(o + 24);
    const u32 p = static_cast<u32>(call(ctx, b4fn::kMemAlloc, {cnt * (flags & 0xFFC0u) + cnt * 68u, 0, 16}));
    const u32 c2 = load<u16>(o + 26), f2 = load<u16>(o + 24);
    store<u32>(o + 32, p);
    store<u32>(o + 36, p);
    const u32 a = p + ((c2 << 6) & ~0x3Fu);
    store<u32>(o + 40, a);
    store<u32>(o + 44, a + (((c2 * ((f2 >> 6) & 0x3FFu)) << 6) & ~0x3Fu));
    call(ctx, Obj8733_Init, {o});
}

namespace {
inline f32 DivS(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) / static_cast<f64>(b)); }
inline f32 SubS(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - static_cast<f64>(b)); }
inline f32 Lf(u32 ea) { return F(load<u32>(ea)); }
// fctidz: the double truncated to s64 (saturating, NaN -> INT64_MIN).
inline u64 Fctidz(f64 x) {
    if (x != x) return 0x8000000000000000ull;
    if (x >= 9223372036854775807.0) return 0x7FFFFFFFFFFFFFFFull;
    if (x <= -9223372036854775808.0) return 0x8000000000000000ull;
    return static_cast<u64>(static_cast<s64>(x));
}
// |a - b| < 1e-5 for the three components (single-precision differences).
inline bool Near3(u32 a, u32 b, f32 eps) {
    for (u32 k = 0; k < 3; ++k)
        if (!(std::fabs(SubS(Lf(a + 4 * k), Lf(b + 4 * k))) < eps)) return false;
    return true;
}
// row3 + row0 * s[0] + row1 * s[1] + row2 * s[2] over a 4x4 matrix on the
// stack (vmaddfp chain from +0.0, then vaddfp), stored with stvx.
void TransformPoint(u32 m, const f32 s[3], u32 dst) {
    f32 acc[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for (u32 r = 0; r < 3; ++r)
        for (u32 i = 0; i < 4; ++i) acc[i] = std::fma(Lf(m + 16 * r + 4 * i), s[r], acc[i]);
    const u32 ea = dst & ~15u;
    for (u32 i = 0; i < 4; ++i) store<f32>(ea + 4 * i, Lf(m + 48 + 4 * i) + acc[i]);
}
}  // namespace

// void PoolObjEB2F_Construct(ObjEB2F* o, const Desc* d): the 256-byte
// kind 0xEB2F2138 (a textured ribbon / beam between two bones of a model:
// the "segment" points d->+36 / +40). Defaults first (+16 / +32 zero
// vectors, +184 1.0, +220 the default colour, flags u16 +236 = (old &
// 0x007F) | 0x0200 then bits 6..5 the instance counter 0x01068B64 (+1),
// u16 bones +224 / +226 -1), +212 the model d->+8. Then: +184 = 1 / len
// (d->+156, the tunable when <= 0), +188 = d->+16 when >= 0 else
// FLT_MAX, +220 the colour d->+28, flag bits 15 / 14 the bytes d->+160 /
// +161 (bit 0), +16.. / +32.. / +48.. / +64.. the vectors d->+48 / +64 /
// +80 / +96, +196 = (d->+144 >= 1e-5 ? 1 / d->+144 : 1e6) / +184, +200 =
// +184 * rate (d->+148, the tunable when < 0), +204 = max(min(+188,
// d->+152), 0), +208 d->+24, flag bit 13 = (+188 < FLT_MAX && d->+12 >=
// +188), flag bits 11..9 = clamp(d->+164, 1, 7). The material: texture
// d->+20 by name (in mode 0; vfn[16](t, 3, 1, 1) when +208 > 1) or the
// default one, Tex_Acquire'd into +216, whose blend state at +104 is set
// by the blend kind d->+32 (0..4) and, with d->+162, its +8 bits 30..15 =
// 519. +238 = clamp(d->+12 / +184 truncated (576 when d->+12 < 0), 2,
// 576) 32-byte points SmallAlloc'd into +248. The bones of the two
// segment ends are looked up in the model's skeleton (vfn[28]) into +224
// / +226; a missing one is looked up as an object id instead (+228 /
// +232). The end matrices (Obj_BoneMatrix from the model's matrix at
// +80) are split into rotation and scale: +80 / +96 the two rotations as
// quaternions, +112 / +128 the end points (+48 / +64 scaled and
// transformed), the first two points +248[0] / [1] = (+16 / +32 scaled,
// 1) transformed (w then 0). +176 / +180 0, +144 / +160 d->+112 / +128,
// +240 = 1, flag bits 7 / 8 set unless d->+112 / +128 is the zero vector
// (within 1e-5), and the u16 refcounts +4 of the model and of +228 /
// +232 (when set) += 1. r3 / f1 are left as the original leaves them.
DECOMP(002F0058, PoolObjEB2F_Construct) {
    using namespace b4p;
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 560);
    const u32 sp = frame.addr() - 112;  // the original's r1: buffers at its offsets
    const f32 kOneF = Lf(kK + 0), kZeroF = Lf(kK + 4), kMaxF = Lf(kK + 8), kEps = Lf(kK + 12);

    store<u8>(o + 8, 0);
    CopyW(o + 4, d + 4);
    CopyWords(o + 16, kZero3, 3);
    store<u32>(o + 0, kVtblEB2F);
    CopyWords(o + 32, kZero3, 3);
    store<u32>(o + 184, 0x3F800000u);
    store<u32>(o + 188, 0);
    store<u32>(o + 192, 0);
    store<u32>(o + 212, 0);
    store<u32>(o + 216, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 220 + k, load<u8>(kDefColor + k));
    store<u16>(o + 236, static_cast<u16>((load<u16>(o + 236) & 0x007Fu) | 0x0200u));
    const u32 serial = load<u32>(kSerialEB2F);
    store<u32>(o + 228, 0);
    store<u32>(o + 232, 0);
    store<u16>(o + 224, 0xFFFF);
    store<u16>(o + 226, 0xFFFF);
    store<u32>(kSerialEB2F, serial + 1);
    store<u32>(o + 248, 0);
    store<u16>(o + 238, 0);
    store<u16>(o + 240, 0);
    store<u16>(o + 236, static_cast<u16>((load<u16>(o + 236) & ~0x60u) | ((serial << 5) & 0x60u)));
    store<u16>(o + 242, 0);
    store<u32>(o + 244, 0);
    store<u32>(o + 212, load<u32>(d + 8));

    // Lengths and rates.
    const u32 lenw = static_cast<f64>(Lf(d + 156)) > static_cast<f64>(kZeroF) ? load<u32>(d + 156) : load<u32>(kTuneLen);
    store<f32>(o + 184, DivS(kOneF, F(lenw)));
    store<u32>(o + 188, Lf(d + 16) >= kZeroF ? load<u32>(d + 16) : load<u32>(kK + 8));
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 220 + k, load<u8>(d + 28 + k));
    u32 fl = (load<u16>(o + 236) & ~0x8000u) | ((load<u8>(d + 160) & 1u) << 15);
    store<u16>(o + 236, static_cast<u16>(fl));
    fl = (fl & ~0x4000u) | ((load<u8>(d + 161) & 1u) << 14);
    store<u16>(o + 236, static_cast<u16>(fl));
    for (u32 v = 0; v < 4; ++v) CopyWords(o + 16 + 16 * v, d + 48 + 16 * v, 3);
    const u32 w188 = load<u32>(o + 188);
    const f32 f188 = F(w188), f184 = Lf(o + 184), f152 = Lf(d + 152), f12 = Lf(d + 12);
    const u32 w24 = load<u32>(d + 24);
    const f32 inv = Lf(d + 144) >= kEps ? DivS(kOneF, Lf(d + 144)) : Lf(kK + 16);
    store<f32>(o + 196, DivS(inv, f184));
    const f32 rate = Lf(d + 148) >= kZeroF ? Lf(d + 148) : Lf(kTuneRate);
    store<f32>(o + 200, static_cast<f32>(static_cast<f64>(f184) * static_cast<f64>(rate)));
    store<u32>(o + 208, w24);
    const u32 wmin = SubS(f188, f152) >= 0.0f ? load<u32>(d + 152) : w188;  // fsel: min(+188, d->152)
    store<f32>(o + 204, F(wmin) >= 0.0f ? F(wmin) : static_cast<f32>(load<f64>(kK + 24)));
    const bool reach = f188 < kMaxF && f12 >= f188;
    fl = (fl & ~0x2000u) | (reach ? 0x2000u : 0u);
    store<u16>(o + 236, static_cast<u16>(fl));
    const u32 segs = load<u32>(d + 164);
    const u32 sc = segs < 7 ? segs : 7u;
    const u32 n9 = sc > 1 ? sc : 1u;
    fl = (fl & ~0x0E00u) | ((n9 << 9) & 0x0E00u);
    store<u16>(o + 236, static_cast<u16>(fl));

    // The material.
    u32 mode = load<u32>(kTexMode);
    u32 tex = 0;
    bool have = false;
    if (mode == 0) {
        tex = static_cast<u32>(call(ctx, Tex_FindLoaded, {load<u32>(d + 20)}));
        if (tex != 0) {
            have = true;
            if (Lf(o + 208) > kOneF) VCall(ctx, tex, 16, {tex, 3, 1, 1});
        } else {
            mode = load<u32>(kTexMode);
        }
    }
    if (!have) tex = static_cast<u32>(call(ctx, Tex_GetDefault, {mode == 2 ? 0u : 1u}));
    const u32 mat = static_cast<u32>(call(ctx, Tex_Acquire, {tex}));
    store<u32>(o + 216, mat);
    const u32 bs = mat + 104;  // blend state: +0 bit 31 / bits 30..15, u16 +4 / +6
    const s32 kind = load<s32>(d + 32);
    auto blend = [&](u32 bits, bool on) {  // the word +0 with bit 31 / bits 30..15 = bits
        const u32 w = load<u32>(bs) | 0x80000000u;
        const u32 x = (w & ~0x7FFF8000u) | ((bits << 15) & 0x7FFF8000u);
        return on ? x : (x & 0x7FFFFFFFu);
    };
    if (kind == 4) {
        const u32 w = blend(770, true);
        store<u16>(bs + 6, 1);
        store<u16>(bs + 4, 0x800B);
        store<u32>(bs, w);
    } else if (kind == 3 || kind == 2 || kind == 1) {
        const u32 w = blend(kind == 1 ? 1u : 770u, true);
        store<u16>(bs + 4, 0x8006);
        store<u16>(bs + 6, kind == 3 ? 1 : kind == 2 ? 771 : 0);
        store<u32>(bs, w);
    } else if (kind == 0) {
        const u32 w = blend(1, false);
        store<u16>(bs + 4, 0x8006);
        store<u16>(bs + 6, 0);
        store<u32>(bs, w);
    }
    {
        const u32 b = load<u32>(o + 216) + 104;
        const u32 w = load<u32>(b + 8);
        store<u16>(b + 12, 0);
        store<u32>(b + 8, w & 0x7FFFFFFFu);
    }
    if (load<u8>(d + 162) != 0) {
        const u32 b = load<u32>(o + 216) + 104;
        store<u32>(b + 8, (load<u32>(b + 8) & ~0x7FFF8000u) | ((519u << 15) & 0x7FFF8000u));
    }

    // The point buffer.
    const f32 dist = Lf(d + 12) >= kZeroF ? DivS(Lf(d + 12), Lf(o + 184)) : Lf(kK + 32);
    const u32 lo32 = static_cast<u32>(Fctidz(static_cast<f64>(dist)));
    const u32 lo = lo32 & 0xFFFFu;
    const u32 np = lo < 3 ? 2u : lo < 0x240 ? lo32 : 0x240u;
    store<u16>(o + 238, static_cast<u16>(np));
    store<u32>(o + 248, static_cast<u32>(call(ctx, b4fn::kSmallAlloc, {(np << 5) & 0x001FFFE0u})));

    // The bones of the two ends.
    const u32 model = load<u32>(o + 212);
    const u32 skel = static_cast<u32>(VCall(ctx, model, 28, {model}));
    if (skel != 0) {
        store<u16>(o + 224, static_cast<u16>(call(ctx, b4fn::Skeleton_FindBone, {skel, load<u32>(d + 36)})));
        store<u16>(o + 226, static_cast<u16>(call(ctx, b4fn::Skeleton_FindBone, {skel, load<u32>(d + 40)})));
    }
    if (static_cast<s16>(load<u16>(o + 224)) < 0)
        store<u32>(o + 228, static_cast<u32>(call(ctx, WorldObj_FindById, {load<u32>(o + 212), load<u32>(d + 36)})));
    if (static_cast<s16>(load<u16>(o + 226)) < 0)
        store<u32>(o + 232, static_cast<u32>(call(ctx, WorldObj_FindById, {load<u32>(o + 212), load<u32>(d + 40)})));

    // The end matrices: parent = the model's matrix (rows' xyz; w lanes left
    // as the stack holds them).
    const u32 par = sp + 240, m0 = sp + 304, m1 = sp + 368, r0 = sp + 432, r1 = sp + 496;
    const u32 s0 = sp + 144, s1 = sp + 160, q = sp + 112, vec = sp + 128, mat4 = sp + 176;
    const u32 pm = load<u32>(o + 212) + 80;
    for (u32 r = 0; r < 4; ++r) CopyWords(par + 16 * r, pm + 16 * r, 3);
    call(ctx, Obj_BoneMatrix, {m0, o, par, load<u32>(o + 228), Sx(static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(o + 224)))))});
    call(ctx, Obj_BoneMatrix, {m1, o, par, load<u32>(o + 232), Sx(static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(o + 226)))))});
    call(ctx, Matrix_SplitScale, {m0, r0, s0});
    call(ctx, Matrix_SplitScale, {m1, r1, s1});
    call(ctx, Quat_FromMatrix, {q, r0});
    CopyWords(o + 80, q, 4);
    call(ctx, Quat_FromMatrix, {q, r1});
    CopyWords(o + 96, q, 4);
    // The end points.
    const f32 a0[3] = {static_cast<f32>(static_cast<f64>(Lf(o + 48)) * Lf(s0)), static_cast<f32>(static_cast<f64>(Lf(o + 52)) * Lf(s0 + 4)),
                       static_cast<f32>(static_cast<f64>(Lf(o + 56)) * Lf(s0 + 8))};
    TransformPoint(r0, a0, o + 112);
    const f32 a1[3] = {static_cast<f32>(static_cast<f64>(Lf(o + 64)) * Lf(s1)), static_cast<f32>(static_cast<f64>(Lf(o + 68)) * Lf(s1 + 4)),
                       static_cast<f32>(static_cast<f64>(Lf(o + 72)) * Lf(s1 + 8))};
    TransformPoint(r1, a1, o + 128);
    for (u32 i = 0; i < 4; ++i) store<f32>(sp + 560 + 4 * i, a1[2]);  // (the splat buffer, as the original leaves it)
    // The first two points: (+16 / +32 scaled, 1) through rotation + translation.
    for (u32 e = 0; e < 2; ++e) {
        const u32 sv = e == 0 ? s0 : s1, rm = e == 0 ? r0 : r1, src = o + 16 + 16 * e;
        const f32 v0 = static_cast<f32>(static_cast<f64>(Lf(src)) * Lf(sv));
        const f32 v1 = static_cast<f32>(static_cast<f64>(Lf(src + 4)) * Lf(sv + 4));
        const f32 v2 = static_cast<f32>(static_cast<f64>(Lf(src + 8)) * Lf(sv + 8));
        store<u32>(vec + 12, 0x3F800000u);
        for (u32 r = 0; r < 4; ++r) {
            CopyWords(mat4 + 16 * r, rm + 16 * r, 3);
            store<u32>(mat4 + 16 * r + 12, r == 3 ? 0x3F800000u : 0u);
        }
        store<f32>(vec + 0, v0);
        store<f32>(vec + 4, v1);
        store<f32>(vec + 8, v2);
        call(ctx, Vec4_TransformByMatrix, {load<u32>(o + 248) + 16 * e, vec, mat4});
    }
    store<u32>(load<u32>(o + 248) + 12, 0);
    store<u32>(load<u32>(o + 248) + 16 + 12, 0);
    store<u32>(o + 176, 0);
    store<u32>(o + 180, 0);
    CopyWords(o + 144, d + 112, 3);
    CopyWords(o + 160, d + 128, 3);
    store<u16>(o + 240, 1);
    const u32 f236 = load<u16>(o + 236);
    const u32 mdl = load<u32>(o + 212), e0 = load<u32>(o + 228), e1 = load<u32>(o + 232);
    const u32 g = f236 | (Near3(d + 112, kZero3, kEps) ? 0u : 0x80u);
    store<u16>(o + 236, static_cast<u16>(g));
    store<u16>(o + 236, static_cast<u16>(g | (Near3(d + 128, kZero3, kEps) ? 0u : 0x100u)));
    // f1 as the second zero test leaves it.
    {
        f64 f1 = std::bit_cast<f64>(LfsBits(load<u32>(kZero3 + 8)));
        if (std::fabs(SubS(Lf(d + 128), Lf(kZero3))) < kEps && std::fabs(SubS(Lf(d + 132), Lf(kZero3 + 4))) < kEps)
            f1 = std::fabs(static_cast<f64>(SubS(Lf(d + 136), Lf(kZero3 + 8))));
        ctx.fpr[1] = f1;
    }
    u32 r3 = mdl + 4;
    store<u16>(r3, static_cast<u16>(load<u16>(r3) + 1));
    if (e0 != 0) {
        r3 = e0 + 4;
        store<u16>(r3, static_cast<u16>(load<u16>(r3) + 1));
    }
    if (e1 != 0) {
        r3 = e1 + 4;
        store<u16>(r3, static_cast<u16>(load<u16>(r3) + 1));
    }
    ret(ctx, r3);
}

// s32 HandlePool_Grow767818(HandlePool* pool, s32 cap): nothing when cap is the
// current capacity (returns it). Otherwise new entries Mem_Alloc(cap * 16,
// 0, 16) (each +8 zeroed), the old ones memmoved in and the old block
// freed, all without a null check (a failed allocation still copies to /
// writes through address 0, as the original). The new entries old..cap-1
// are chained onto the free list's tail: entry old gets +0 the old tail
// (whose +4 then points at it when the free list isn't empty) and +4
// old+1; each later one +0 / +4 its neighbours, bit 31 of +12 set and
// the generation bits 30..15 cleared (entry old keeps its +12 as the
// allocation left it); the last +4 = -1. The free head becomes old when
// the list was empty, the tail cap-1, the free count grows by cap - old
// and the capacity is cap. Returns the new free count. (See
// ObjPool_Create for the pool and entry layout.)
DECOMP(00767818, HandlePool_Grow767818) {
    using namespace b4fn;
    const u32 pool = arg<u32>(ctx, 0);
    const s32 cap = arg<s32>(ctx, 1);
    if (cap == load<s32>(pool + 24)) { ret(ctx, static_cast<u32>(cap)); return; }
    const u32 data = static_cast<u32>(call(ctx, kMemAlloc, {static_cast<u32>(cap) << 4, 0, 16}));
    if (data != 0)
        for (s32 k = 0; k < cap; ++k) store<u32>(data + 16 * static_cast<u32>(k) + 8, 0);
    call(ctx, kMemmove, {data, load<u32>(pool + 28), load<u32>(pool + 24) << 4});
    call(ctx, kMemFreeFrom, {load<u32>(pool + 28), 0});
    store<u32>(pool + 28, data);
    // (The capacity is read again after this store, as the original does.)
    const u32 old = load<u32>(pool + 24), tail = load<u32>(pool + 4);
    store<u32>(data + (old << 4), tail);  // entry[old].prev = the free tail
    u32 nfree = load<u32>(pool + 20);
    u32 old2 = load<u32>(pool + 24);
    u32 ents = load<u32>(pool + 28);
    u32 first = old;
    if (nfree != 0) {
        store<u32>(ents + (tail << 4) + 4, old);  // tail.next = old
        first = old2;
    }
    store<u32>(ents + (old2 << 4) + 4, first + 1);
    if (static_cast<s32>(old2 + 1) < cap) {
        u32 i = old2 + 1;
        for (s32 left = cap - static_cast<s32>(i); left > 0; --left, ++i) {
            const u32 e = load<u32>(pool + 28) + (i << 4);
            store<u32>(e + 0, i - 1);
            store<u32>(e + 4, i + 1);
            store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) | 0x80));
            const u32 e2 = load<u32>(pool + 28) + (i << 4) + 12;
            store<u32>(e2, load<u32>(e2) & ~0x7FFF8000u);
        }
        nfree = load<u32>(pool + 20);
        old2 = load<u32>(pool + 24);
        ents = load<u32>(pool + 28);
    }
    store<u32>(ents + (static_cast<u32>(cap) << 4) - 12, 0xFFFFFFFFu);  // entry[cap - 1].next
    if (nfree == 0) store<u32>(pool + 0, old2);
    const u32 n = nfree + (static_cast<u32>(cap) - old2);
    store<u32>(pool + 4, static_cast<u32>(cap) - 1);
    store<u32>(pool + 20, n);
    store<u32>(pool + 24, static_cast<u32>(cap));
    ret(ctx, n);
}
