// World objects, part 6: skeleton set-up and pose copies, the object
// collector WorldObj_AddToWorld uses, the base-state copy, box corners and
// the emitter-system binding.
#include <cmath>

#include "box.h"
#include "obj6.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj5;
using namespace sly4::obj6;

namespace {

struct Lanes {
    f32 v[4];
};

// lvx: the quadword at ea & ~15.
inline Lanes LoadQ(u32 ea) {
    Lanes r;
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) r.v[i] = load<f32>(a + i * 4);
    return r;
}

// stvx: the quadword at ea & ~15.
inline void StoreQ(u32 ea, const Lanes& x) {
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) store<f32>(a + i * 4, x.v[i]);
}

// The x / y / z words of a 4x4 row matrix (rows 16 bytes apart), raw.
inline void CopyMatrix3x4(u32 dst, u32 src) {
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(dst + row + k, load<u32>(src + row + k));
}

}  // namespace

// void Debug_PrintBone(const BoneDef* b)
// The debug dump's per-bone print, compiled out: an empty function.
DECOMP(0055498C, Debug_PrintBone) {}

// void Box_FromSphere(Box* b, const f32 sphere[4])
// min = centre - r, max = centre + r per axis (r = sphere[3]); the centre
// is read again for max. f1 is left as max.x, as the original leaves it.
DECOMP(00556F10, Box_FromSphere) {
    const u32 b = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const f32 r = load<f32>(s + 12);
    for (u32 i = 0; i < 3; ++i) store<f32>(b + box::kBoxMin + i * 4, load<f32>(s + i * 4) - r);
    f32 x = 0.0f;
    for (u32 i = 0; i < 3; ++i) {
        const f32 v = load<f32>(s + i * 4) + r;
        if (i == 0) x = v;
        store<f32>(b + box::kBoxMax + i * 4, v);
    }
    ctx.fpr[1] = static_cast<f64>(x);
}

// void Box_Corners(const Box* b, f32 out[8][4], const Matrix* m)
// The box's eight corners (bit 0 of the index picks min.x over max.x, bit
// 1 min.y, bit 2 min.z) moved by the 4x4 row matrix: each corner's x / y /
// z is written to out first and read back (lvlx), then the whole quadword
// is overwritten with row0 * x + row1 * y + row2 * z + row3 (vmaddfp
// chains from 0, then vaddfp), all four lanes. f1 is left as the last z.
DECOMP(00557690, Box_Corners) {
    const u32 b = arg<u32>(ctx, 0), m = arg<u32>(ctx, 2);
    u32 out = arg<u32>(ctx, 1);
    f32 z = 0.0f;
    for (u32 i = 0; i < box::kBoxCorners; ++i) {
        store<u32>(out + 0, load<u32>(b + ((i & 1) != 0 ? box::kBoxMin : box::kBoxMax) + 0));
        store<u32>(out + 4, load<u32>(b + ((i & 2) != 0 ? box::kBoxMin : box::kBoxMax) + 4));
        store<u32>(out + 8, load<u32>(b + ((i & 4) != 0 ? box::kBoxMin : box::kBoxMax) + 8));
        z = load<f32>(out + 8);
        const f32 x = load<f32>(out), y = load<f32>(out + 4), zz = load<f32>(out + 8);
        const Lanes r0 = LoadQ(m), r1 = LoadQ(m + 16), r2 = LoadQ(m + 32), r3 = LoadQ(m + 48);
        Lanes p;
        for (u32 k = 0; k < 4; ++k) {
            f32 a = std::fmaf(r0.v[k], x, 0.0f);
            a = std::fmaf(r1.v[k], y, a);
            a = std::fmaf(r2.v[k], zz, a);
            p.v[k] = r3.v[k] + a;
        }
        StoreQ(out, p);
        out += 16;
    }
    ctx.fpr[1] = static_cast<f64>(z);
}

// void Skeleton_CopyPose(ApeSkeleton* dst, const ApeSkeleton* src)
// For each of dst's bones: the local (+36) and world (+40) matrices' x / y
// / z words copied from src's arrays (raw), and dst's bone entry (+32)
// pointed at its own two matrices. Nothing for a skeleton without bones.
DECOMP(00580D70, Skeleton_CopyPose) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    s32 n = load<u16>(dst + kSkelBoneCount);
    if (n == 0) return;
    u32 dl = load<u32>(dst + kSkelLocalArr), sl = load<u32>(src + kSkelLocalArr);
    u32 sw = load<u32>(src + kSkelWorldArr), dw = load<u32>(dst + kSkelWorldArr);
    u32 entry = load<u32>(dst + kSkelBones);
    do {
        CopyMatrix3x4(dl, sl);
        CopyMatrix3x4(dw, sw);
        store<u32>(entry + kSkelLocal, dl);
        store<u32>(entry + kSkelWorld, dw);
        sl += kMatrixSize;
        sw += kMatrixSize;
        entry += kSkelEntrySize;
        dl += kMatrixSize;
        dw += kMatrixSize;
        --n;
    } while (n > 0);
}

// void Bone_InitEntry(BoneEntry* e, const BoneDef* def, Matrix* local, Matrix* world, f32* vec)
// Fills a skeleton's bone entry {def, local, world, vec} and sets both
// matrices to the inverse of the bone def's bind matrix: the rotation
// transposed and the translation -(t . rows), as the vmaddfp chain
// computes it. Only the x / y / z words are written.
DECOMP(005812F0, Bone_InitEntry) {
    const u32 e = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1);
    const u32 local = arg<u32>(ctx, 2), world = arg<u32>(ctx, 3);
    store<u32>(e + 0, def);
    store<u32>(e + kSkelLocal, local);
    store<u32>(e + kSkelWorld, world);
    store<u32>(e + kSkelEntryVec, arg<u32>(ctx, 4));
    const Lanes t = LoadQ(def + 48), r0 = LoadQ(def), r2 = LoadQ(def + 32), r1 = LoadQ(def + 16);
    // vmrghw / vmrglw / vperm (masks 0x005812D0 / 0x005812E0): the columns.
    const Lanes c0{{r0.v[0], r1.v[0], r2.v[0], r1.v[1]}};
    const Lanes c1{{r0.v[1], r1.v[1], r2.v[1], r0.v[0]}};
    const Lanes c2{{r0.v[2], r1.v[2], r2.v[2], r0.v[2]}};
    const f32 nx = 0.0f - t.v[0], ny = 0.0f - t.v[1], nz = 0.0f - t.v[2];  // vsubfp from 0
    Lanes c3;
    for (u32 k = 0; k < 4; ++k) {
        f32 a = std::fmaf(c0.v[k], nx, 0.0f);
        a = std::fmaf(c1.v[k], ny, a);
        c3.v[k] = std::fmaf(c2.v[k], nz, a);
    }
    // (the original goes through sp+48..sp+111 and f1-f12; nothing reads them after)
    const Lanes* rows[4] = {&c0, &c1, &c2, &c3};
    for (u32 out : {local, world})
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) store<f32>(out + r * 16 + k * 4, rows[r]->v[k]);
}

// void Skeleton_Layout(ApeSkeleton* s, void* block, u32 size)
// Carves the bone arrays out of one block (n = s +28 bones): local
// matrices at +0, world matrices at 64n, the +44 matrices at 128n, the
// vec3s at 192n, the parents (s16) at 208n and the 16-byte bone entries
// after them (16-aligned).
DECOMP(0058142C, Skeleton_Layout) {
    const u32 s = arg<u32>(ctx, 0), blk = arg<u32>(ctx, 1);
    const u32 n = load<u16>(s + kSkelBoneCount);
    store<u32>(s + kSkelBlock, blk);
    store<u32>(s + kSkelBlockSize, arg<u32>(ctx, 2));
    store<u32>(s + kSkelLocalArr, blk);
    const u32 m64 = ((n << 6) + 15) & ~15u;
    const u32 m128 = (((n << 6) + 15) << 1) & ~31u;
    const u32 world = blk + m64;
    const u32 mats = world + m64;
    const u32 vecs = blk + m128 + m64;
    store<u32>(s + kSkelWorldArr, world);
    store<u32>(s + kSkelMatArr, mats);
    store<u32>(s + kSkelVecArr, vecs);
    const u32 parents = vecs + (((n << 4) + 15) & ~15u);
    store<u32>(s + kSkelParents, parents);
    const u32 entries = parents + (((n << 1) + 15) & ~15u);
    store<u32>(s + kSkelBones, entries);
}

// void Skeleton_Init(ApeSkeleton* s, ApeSkeletonDef* def)
// Sets a new skeleton up from its definition: +16 def, +28 its bone count,
// +30 the global skeleton serial (0x01194594, bumped) modulo the config's
// count (at least 1), +31 bits 0xE0 cleared; one SmallAlloc'd block for
// the bone arrays (Skeleton_Layout); each bone's entry and inverse bind
// matrices (Bone_InitEntry, entry address checked against 0), its parent
// and a (1, 1, 1) vec3; then the +44 matrices (0x00580EA0).
DECOMP(005814A0, Skeleton_Init) {
    const u32 s = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1);
    store<u32>(s + kSkelDef, def);
    store<u32>(s + kSkelBlock, 0);
    store<u32>(s + kSkelBlockSize, 0);
    const u32 n = load<u16>(def + kSkelDefBoneCount);
    store<u16>(s + kSkelBoneCount, static_cast<u16>(n));
    const u32 serial = load<u8>(kSkeletonSerial);
    store<u8>(kSkeletonSerial, static_cast<u8>(serial + 1));
    const s32 cfg = load<u8>(kConfigSkelMod);
    const s32 mod = cfg > 1 ? cfg : 1;
    const u8 fl = load<u8>(s + kSkelFlags);
    store<u8>(s + kSkelSerial, static_cast<u8>(static_cast<s32>(serial) % mod));
    store<u8>(s + kSkelFlags, static_cast<u8>(fl & 0x1F));
    // The block size: 192n + 32n + align16(2n), as the original rounds it.
    const u32 a = ((n << 6) & 0x003FFFC0u) + 15;
    const u32 b = ((n << 4) & 0x000FFFF0u) + 15;
    const u32 c = (((n << 1) & 0x0001FFFEu) + 15) & ~15u;
    const u32 size = (((a << 2) & ~63u) - (a & ~15u)) + ((b << 1) & ~31u) + c;
    const u32 blk = static_cast<u32>(call(ctx, o6fn::SmallAlloc, {size}));
    call(ctx, o6fn::Skeleton_Layout, {s, blk, size});
    for (u32 i = 0; load<u16>(s + kSkelBoneCount) != 0 && i < load<u16>(s + kSkelBoneCount); ++i) {
        const u32 entry = load<u32>(s + kSkelBones) + i * kSkelEntrySize;
        if (entry != 0)
            call(ctx, o6fn::Bone_InitEntry,
                 {entry, load<u32>(def + kSkelDefBones) + i * kBoneDefSize, load<u32>(s + kSkelLocalArr) + i * kMatrixSize,
                  load<u32>(s + kSkelWorldArr) + i * kMatrixSize, load<u32>(s + kSkelVecArr) + i * 16});
        const u32 bone = load<u32>(def + kSkelDefBones) + i * kBoneDefSize;
        store<u16>(load<u32>(s + kSkelParents) + i * 2, load<u16>(bone + kBoneParent));
        const u32 v = load<u32>(s + kSkelVecArr) + i * 16;
        for (u32 k = 0; k < 12; k += 4) store<u32>(v + k, load<u32>(obj::kOneVec3 + k));
    }
    call(ctx, o6fn::Skeleton_Compose, {s, load<u32>(s + kSkelMatArr)});
}

// void WorldObj_CopyBase(Obj* dst, const Obj* src, u32 flags)
// The base half of WorldObj_CopyFrom, each part skipped by its flag: the
// +12 components (ref-counted, 0x00558E24), the matrix / scale words
// +16..+152 (raw) with +160 reset, the +164 bits 0x8000 / 0x4000 / 0x2000 /
// 0x1000 / 0x100 / 0xC0 (and 0x800 set) and the +168 top byte and bits
// 0x00100000 / 0x80000 / 0x40000 / 0x10000, the name and its hash, then
// dst->SetClass(src's class) (vfunc +44; r5 = entry, r6 = OPD).
DECOMP(0057DEEC, WorldObj_CopyBase) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), flags = arg<u32>(ctx, 2);
    if ((flags & kBaseKeepComps) == 0) call(ctx, o6fn::ObjComps_Assign, {dst, src});
    if ((flags & kBaseKeepMatrix) == 0) {
        for (u32 off = 16; off <= 152; off += 16) {
            for (u32 k = 0; k < 12; k += 4) {
                if (off + k == 148) store<u32>(dst + kObj160, kObj160Default);
                store<u32>(dst + off + k, load<u32>(src + off + k));
            }
        }
    }
    if ((flags & kBaseKeepFlags) == 0) {
        // Each bit merged from a fresh read of src and stored (they may alias).
        u32 w = load<u32>(dst + obj::kObjFlags);
        auto merge = [&](u32 mask, u32 extra) {
            w = (w & ~mask) | (load<u32>(src + obj::kObjFlags) & mask) | extra;
            store<u32>(dst + obj::kObjFlags, w);
        };
        merge(0x8000, 0);
        merge(0x4000, 0);
        merge(0x2000, 0);
        merge(0x1000, obj::kObjDirty);
        const u32 src164 = load<u32>(src + obj::kObjFlags);
        u32 w168 = load<u32>(dst + 168);
        w = (w & ~0x100u) | (src164 & 0x100u);
        store<u32>(dst + obj::kObjFlags, w);
        merge(0xC0, 0);
        const u8 top = load<u8>(src + 168);
        w168 = (w168 & 0x00FFFFFFu) | (static_cast<u32>(top) << 24);
        store<u8>(dst + 168, top);
        for (u32 mask : {0x00100000u, 0x00080000u, 0x00040000u, 0x00010000u}) {
            w168 = (w168 & ~mask) | (load<u32>(src + 168) & mask);
            store<u32>(dst + 168, w168);
        }
    }
    if ((flags & kBaseKeepName) == 0) {
        call(ctx, o6fn::String_Assign, {dst + obj::kObjName, src + obj::kObjName});
        const u32 h = load<u32>(src + obj::kObjNameHash);
        if (load<u32>(dst + obj::kObjNameHash) != h) store<u32>(dst + obj::kObjNameHash, h);
    }
    if ((flags & kBaseKeepClass) != 0) return;
    const u32 opd = load<u32>(load<u32>(dst) + kVfnSetClass);
    call_ptr(ctx, opd, {dst, load<u32>(src + kObjClassPtr), load<u32>(opd), opd});
}

// bool Collector_Accepts(const Obj* o, bool visible_only)
// Whether AddToWorld's collector takes o: never a +320 type-1 object, nor
// a hidden one (+164 bit 0x8000 clear) when visible_only, nor one without
// components (+12); otherwise when one of its first four components (a
// null ends each scan) has one of the three type hashes at 0x00F88948 and
// the component query (0x00593DA8 with 0, comps, 1) says so.
DECOMP(005A9914, Collector_Accepts) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 comps = load<u32>(o + kObjComps);
    if (((load<u32>(o + kObjBits320) >> 9) & 0xF) == kObjType320Skip) return ret(ctx, 0u);
    if ((arg<u32>(ctx, 1) & 0xFF) != 0 && (load<u32>(o + obj::kObjFlags) & kObjFlagVisible) == 0) return ret(ctx, 0u);
    if (comps == 0) return ret(ctx, 0u);
    auto has = [&](u32 type_ea) {
        const u32 type = load<u32>(type_ea);
        for (u32 i = 0; i < kCompSlots; ++i) {
            const u32 c = load<u32>(comps + 4 + i * 4);
            if (c == 0) return false;
            if (load<u32>(c + 8) == type) return true;
        }
        return false;
    };
    const bool a = has(kCompTypeA);
    const bool b = has(kCompTypeB);
    if (!has(kCompTypeC) && !a && !b) return ret(ctx, 0u);
    const u32 r = static_cast<u32>(call(ctx, o6fn::Comps_Query, {0, comps, 1}));
    ret(ctx, r != 0 ? 1u : 0u);
}

// bool Collector_Add(Obj* node, Collector* c)  (OPD 0x00F3CA20)
// The WorldObj_ForEach callback of WorldObj_AddToWorld: appends an
// accepted node (Collector_Accepts with !c->option) with a flag byte (1
// for the first of the run, +8 == 0) and bumps the count and the run.
// Always continues the walk.
DECOMP(005A9AA8, Collector_Add) {
    const u32 node = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u32 ok = static_cast<u32>(call(ctx, o6fn::Collector_Accepts, {node, load<u8>(c + kColOption) == 0 ? 1u : 0u}));
    if (ok != 0) {
        const u32 run = load<u32>(c + kColRun);
        store<u8>(load<u32>(c + kColFlags) + load<u32>(c + kColCount), run == 0 ? 1 : 0);
        const u32 n = load<u32>(c + kColCount);
        store<u32>(load<u32>(c + kColData) + n * 4, node);
        store<u32>(c + kColCount, n + 1);
        store<u32>(c + kColRun, load<u32>(c + kColRun) + 1);
    }
    ret(ctx, 1u);
}

// s32 EmitProps_BlendKind(const u32 state[2])
// Classifies a blend state {u32 word, u16 at +4, u16 at +6}: with the top
// bit set and bits 15-30 = 0x302: (+6 0x303, +4 0x8006) -> 2, (+6 1, +4
// 0x8006) -> 3, (+6 1, +4 0x800B) -> 4; anything else (including the
// top-bit-clear 1 / 0 / 0x8006 case the original tests first) -> 0.
DECOMP(0050061C, EmitProps_BlendKind) {
    const u32 st = arg<u32>(ctx, 0);
    const u32 w = load<u32>(st);
    const u32 mid = (w >> 15) & 0xFFFF;
    const u32 h4 = load<u16>(st + 4), h6 = load<u16>(st + 6);
    if ((w >> 31) == 0) return ret(ctx, 0u);
    if (mid == 0x302 && h6 == 0x303 && h4 == 0x8006) return ret(ctx, 2u);
    if (mid == 0x302 && h6 == 1 && h4 == 0x8006) return ret(ctx, 3u);
    if (mid == 0x302 && h6 == 1 && h4 == 0x800B) return ret(ctx, 4u);
    ret(ctx, 0u);
}

// void EmitterSystem_Bind(EmitterSystem* s, AglEmitProperties* props)
// Points s +4 at props (ref-counted); unless props are static (+68), the
// +64 flags get 0x04 cleared and 0x02 = (the material's blend kind == 3).
DECOMP(00500714, EmitterSystem_Bind) {
    const u32 s = arg<u32>(ctx, 0);
    call(ctx, o6fn::EmitPropsRef_Set, {s + kEsProps, arg<u32>(ctx, 1)});
    const u32 p = load<u32>(s + kEsProps);
    if (p == 0 || load<u8>(p + kEpStatic) != 0) return;
    const u32 kind = static_cast<u32>(call(ctx, o6fn::EmitProps_BlendKind, {load<u32>(p + kEpMaterial) + 104}));
    const u8 f = load<u8>(s + kEsFlags);
    store<u8>(s + kEsFlags, static_cast<u8>((f & ~0x06u) | (kind == 3 ? 0x02u : 0u)));
}
