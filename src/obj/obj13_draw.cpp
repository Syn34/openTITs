// World objects, part 13: the draw side of the object system: the skinning
// matrices a packet carries and the topology queue for a forced material
// (receiver).
#include "obj13.h"

using namespace sly4;
using namespace sly4::obj8;
using namespace sly4::obj9;
using namespace sly4::obj12;
using namespace sly4::obj13;

namespace {

// ---------------------------------------------------------------- the frame's bump allocator (name?)
constexpr u32 kFrameAlloc = 0x010DD600;
constexpr u32 kFaCursor = 20;                   // u32: the next free byte (never checked against an end)
constexpr u32 kFaStack = 280;                   // ApeMemStack*: the frame's stack allocator

// ---------------------------------------------------------------- the draw packet (obj12.h kDp*)
constexpr u32 kDpTranslate = 48;                // f32[3]: the matrix's row 3
constexpr u32 kDpBones = 80;                    // f32[3][4]*: the bone matrices (transposed, 48 bytes each) (name?)
constexpr u32 kDpBoneCount = 88;                // u8
constexpr u32 kBoneOut = 48;

// Hands one draw to its bucket (obj12's Topology_Apply SubmitItem with the
// receiver for the material): DrawItem_Create, then bucket vfunc +8
// (bucket, item, key) with key = (s8 layer << 8) | kind; the kind is the
// packet's +70, else the material's +45, else 4 (material +104 bit 31), 0
// (packet alpha 255) or 6.
void QueueItem(PPUContext& ctx, u32 rec, u32 mat, u32 pk) {
    const u32 item = static_cast<u32>(call(ctx, o12fn::DrawItem_Create, {rec, mat, pk}));
    u32 kind = load<u8>(pk + kDpKind);
    if (kind == 0) {
        const u32 mk = load<u8>(mat + kMatKind);
        const u32 alpha = load<u8>(pk + kDpAlpha);
        if (mk != 0) kind = mk;
        else if ((load<u32>(mat + kMatWord104) >> 31) != 0) kind = 4;
        else kind = alpha == 255 ? 0 : 6;
    }
    const s32 layer = static_cast<s8>(load<u8>(pk + kDpLayer));
    const u32 key = (static_cast<u32>(layer) << 8) | (kind & 0xFF);
    const u32 bucket = load<u32>(kDrawBuckets + ((key & 0xFF) << 2));
    const u32 opd = load<u32>(load<u32>(bucket) + 8);
    call_ptr(ctx, opd, {bucket, item, key, opd, load<u32>(opd)});
}

// Topology_Apply's start: DrawPacket_Begin, then the topology's record
// (+92) through DrawRecord_Submit while the static draw-record list is
// enabled (set up inline on first use).
void BeginTopology(PPUContext& ctx, u32 t, u32 pk) {
    call(ctx, o12fn::DrawPacket_Begin, {pk});
    u32 rec = load<u32>(t + obj6::kTopoRecord);
    if (rec == 0) return;
    if (load<u8>(kTrListGuard) == 0) {
        store<u8>(kTrList, 1);
        call(ctx, o12fn::PtrVector_Construct, {kTrList + 4});
        store<u16>(kTrList + 10, 160);
        store<u32>(kTrList + 32, 0);
        store<u32>(kTrList + 128, 0);  // lwarx / stwcx. stores in the original
        store<u32>(kTrList + 256, 0);
        store<u8>(kTrListGuard, 1);
        rec = load<u32>(t + obj6::kTopoRecord);
    }
    if (load<u8>(kTrList) != 0) call(ctx, o12fn::DrawRecord_Submit, {rec, pk});
}

}  // namespace

// void DrawPacket_SetTexture(DrawPacket* pk, const f32 bones[][4][4], u32 count)
// Despite the name, the skinning set-up (WorldObj_SubmitDraw passes the
// skeleton's +44 matrices and +28 bone count): the packet's translation
// moves by its rotation times the first bone's translation, and `count`
// bone matrices, transposed to 3 x 4 with that translation taken off, are
// written to the frame allocator (48 * count rounded up to 16, unchecked),
// whose address goes to +80 and the count to +88 (name?).
DECOMP(0050CD58, DrawPacket_SetTexture) {
    const u32 pk = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    const u32 t0 = m + 48;
    const Vec4 x = VSplat(VLoadLeft(t0), 0), y = VSplat(VLoadLeft(t0 + 4), 0), z = VSplat(VLoadLeft(t0 + 8), 0);
    Vec4 v = VMadd(VLoad(pk), x, VZero());
    v = VMadd(VLoad(pk + 16), y, v);
    v = VMadd(VLoad(pk + 32), z, v);
    const u32 tx = load<u32>(t0), ty = load<u32>(t0 + 4), tz = load<u32>(t0 + 8);
    const u32 size = (n * kBoneOut + 15) & ~15u;
    const f32 p0 = Lf(pk + kDpTranslate) + v.f(0);
    const f32 p1 = Lf(pk + kDpTranslate + 4), p2 = Lf(pk + kDpTranslate + 8);
    store<f32>(pk + kDpTranslate, p0);
    store<f32>(pk + kDpTranslate + 4, p1 + v.f(1));
    store<f32>(pk + kDpTranslate + 8, p2 + v.f(2));
    u32 dst = load<u32>(kFrameAlloc + kFaCursor);
    store<u32>(kFrameAlloc + kFaCursor, dst + size);
    store<u32>(pk + kDpBones, dst);
    store<u8>(pk + kDpBoneCount, static_cast<u8>(n));
    s32 left = static_cast<s32>(n);
    if (left == 0) {
        ctx.gpr[3] = 0;
        ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(tz));
        return;
    }
    const f32 nx = -std::bit_cast<f32>(tx), ny = -std::bit_cast<f32>(ty), nz = -std::bit_cast<f32>(tz);
    u32 src = m;
    do {
        // Each output row: a column of the bone's rotation, then its
        // translation lane minus the first bone's (raw lfs / stfs copies).
        const f32 w0 = Fadds(Lf(src + 48), nx);
        const u32 a0 = load<u32>(src), b0 = load<u32>(src + 16), c0 = load<u32>(src + 32);
        store<u32>(dst, a0);
        store<u32>(dst + 4, b0);
        store<u32>(dst + 8, c0);
        --left;
        store<f32>(dst + 12, w0);
        const f32 w1 = Fadds(Lf(src + 52), ny);
        const u32 a1 = load<u32>(src + 4), b1 = load<u32>(src + 20), c1 = load<u32>(src + 36);
        store<u32>(dst + 16, a1);
        store<u32>(dst + 20, b1);
        store<u32>(dst + 24, c1);
        store<f32>(dst + 28, w1);
        const f32 w2 = Fadds(Lf(src + 56), nz);
        const u32 a2 = load<u32>(src + 8), b2 = load<u32>(src + 24), c2 = load<u32>(src + 40);
        src += 64;
        store<u32>(dst + 32, a2);
        store<u32>(dst + 36, b2);
        store<u32>(dst + 40, c2);
        store<f32>(dst + 44, w2);
        dst += kBoneOut;
    } while (left > 0);
    ctx.gpr[3] = static_cast<u32>(left);
    ctx.fpr[1] = static_cast<f64>(nz);
}

// void Topology_ApplyRecv(AglTopology* t, DrawPacket* pk, AglMaterial* recv)
// Topology_Apply with every draw's material replaced by `recv`: when the
// topology has draws, DrawPacket_Begin and the record, then (last first,
// the draw array re-read every step) each draw's record queued with recv
// while recv is visible (+100 bit 0x20); no layer mask test. r3 = 0.
DECOMP(00509CEC, Topology_ApplyRecv) {
    const u32 t = arg<u32>(ctx, 0), pk = arg<u32>(ctx, 1), recv = arg<u32>(ctx, 2);
    if (load<u16>(t + obj6::kTopoDrawCount) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    BeginTopology(ctx, t, pk);
    for (u32 i = load<u16>(t + obj6::kTopoDrawCount) - 1;; --i) {
        const u32 r = load<u32>(load<u32>(t + obj6::kTopoDraws) + i * kTopoDrawStride);
        if ((load<u8>(recv + kMatFlags100) & kMatVisible) != 0) QueueItem(ctx, r, recv, pk);
        if (i == 0) break;
    }
    ctx.gpr[3] = 0;
}

// void Topology_ApplyTo(AglTopology* t, DrawPacket* pk, AglMaterial** recv)
// Topology_ApplyRecv with one material per draw: draw i (last first) is
// queued with recv[i] when that is set and visible. r3 = 0.
DECOMP(00509F08, Topology_ApplyTo) {
    const u32 t = arg<u32>(ctx, 0), pk = arg<u32>(ctx, 1), recv = arg<u32>(ctx, 2);
    if (load<u16>(t + obj6::kTopoDrawCount) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    BeginTopology(ctx, t, pk);
    for (u32 i = load<u16>(t + obj6::kTopoDrawCount) - 1;; --i) {
        const u32 mat = load<u32>(recv + i * 4);
        if (mat != 0) {
            const u32 r = load<u32>(load<u32>(t + obj6::kTopoDraws) + i * kTopoDrawStride);
            if ((load<u8>(mat + kMatFlags100) & kMatVisible) != 0) QueueItem(ctx, r, mat, pk);
        }
        if (i == 0) break;
    }
    ctx.gpr[3] = 0;
}

namespace {

// ---------------------------------------------------------------- WorldObj_AnimMatrix
constexpr u32 kAnimMatrix = 0x0116BD20;         // f32[4][4]: the shared result for modes 1-3 (name?)
constexpr u32 kAnimViewA = 0x010DD900;          // f32[4][4]: mode 1's rotation (name?)
constexpr u32 kAnimViewB = 0x010DD940;          // f32[4][4]: mode 2's rotation (name?)
constexpr u32 kAmConsts = 0x00572AA8;           // f32 {1, 100}
constexpr u32 kStrLink = 0x00AF4DE0;            // "_Link_"
constexpr u32 kLinkDigits = 6;                  // strlen("_Link_")
constexpr u32 kVfnName = 12;                    // obj4.h kVfnGetName
constexpr u32 kAnimShift = 6;                   // obj +164 bits 6-7 (obj12.h kObjAnimBits)
constexpr u32 kModeRotA = 1, kModeRotB = 2, kModeCamera = 3;
constexpr u32 kViewCamPosOff = obj12::kViewCamPos;

constexpr u32 String_Find = 0x00555FCC;         // (const char* s, const char* sub) -> char* (strstr) (name?)
constexpr u32 String_ToInt = 0x0055672C;        // (const char* s, s32 base) -> s32 (name?)
constexpr u32 Matrix_MulRot = 0x00553FF0;       // (f32 out[4][4], const f32 a[4][4], const f32 b[4][4]) (name?)

// out = Matrix_MulRot(m, rot) with m's translation (raw copies).
u32 RotateInto(PPUContext& ctx, u32 m, u32 rot) {
    call(ctx, Matrix_MulRot, {kAnimMatrix, m, rot});
    for (u32 k = 0; k < 12; k += 4) store<u32>(kAnimMatrix + 48 + k, load<u32>(m + 48 + k));
    return kAnimMatrix;
}

}  // namespace

// f32 (*WorldObj_AnimMatrix(WorldObj* o))[4][4]
// The matrix an object draws with: its slot entry's (or, without one, its
// own +80) matrix, transformed by the +164 bits 6-7 mode into the shared
// matrix at 0x0116BD20: 1 / 2 rotate it by the matrices at 0x010DD900 /
// 0x010DD940 (Matrix_MulRot, translation kept), 3 keeps the rotation and
// puts it at the current view's camera position, x and z scaled by the
// number after "_Link_" in its name / 100 (1 without one) (name?).
DECOMP(00572AB0, WorldObj_AnimMatrix) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 idx = load<u16>(o + obj11::kObjSlotIndex);
    const u32 m = idx == 0xFFFF ? o + obj6::kObjWorld
                                : load<u32>(obj4::kSlotTableBase) + ((idx << obj11::kSlotEntryShift) & 0xFFFFFF80u);
    const u32 mode = (load<u32>(o + obj::kObjFlags) >> kAnimShift) & 3;
    u32 r = m;
    if (mode == kModeRotA) {
        r = RotateInto(ctx, m, kAnimViewA);
    } else if (mode == kModeRotB) {
        r = RotateInto(ctx, m, kAnimViewB);
    } else if (mode == kModeCamera) {
        for (u32 row = 0; row < 48; row += 16)
            for (u32 k = 0; k < 12; k += 4) store<u32>(kAnimMatrix + row + k, load<u32>(m + row + k));
        f32 scale = Lf(kAmConsts);
        const u32 name = static_cast<u32>(obj4::Vcall0(ctx, o, kVfnName));
        const u32 p = static_cast<u32>(call(ctx, String_Find, {name, kStrLink}));
        if (p != 0) {
            const u32 v = static_cast<u32>(call(ctx, String_ToInt, {p + kLinkDigits, 10})) & 0xFF;
            scale = Fdivs(static_cast<f32>(rt::fcfid(v)), Lf(kAmConsts + 4));
        }
        const u32 cam = static_cast<u32>(call(ctx, obj11::o11fn::View_GetCurrent)) + kViewCamPosOff;
        const f32 cx = Fmuls(scale, Lf(cam)), cz = Fmuls(scale, Lf(cam + 8));
        store<u32>(kAnimMatrix + 52, load<u32>(cam + 4));
        store<f32>(kAnimMatrix + 48, cx);
        store<f32>(kAnimMatrix + 56, cz);
        r = kAnimMatrix;
    }
    ret(ctx, r);
}

namespace {

// ---------------------------------------------------------------- DrawRecord_Submit
// The 160-byte record it builds at sp+112 for DrawRecordList_Add: +0 the
// draw record, +4 u8 the bone count, +8 the bone copy, +16 f32[4][3] the
// packet's matrix (w lanes not written), +80 f32[4][4] that matrix times
// the view's (+752), +144 f32[3] the view's camera position (+864) (name?).
constexpr u32 kSubmitFrame = 304;
constexpr u32 kSbRec = 112, kSbCount = 116, kSbBones = 120, kSbMatrix = 128, kSbViewMatrix = 192, kSbCamera = 256;
constexpr u32 kViewMatrix = 752;              // View: f32[4][4]
constexpr u32 kCurrentSurface = 0x010DDA74;     // view2.h v2g::current_surface
constexpr u32 kSurfaceView = 672;               // its View* (View_GetCurrent)
constexpr u32 DrawRecordList_Add = 0x004FD2E0;  // (draw record +0, const SubmitRecord*) (name?)

}  // namespace

// void DrawRecord_Submit(DrawRecord* r, const DrawPacket* pk)
// Queues a draw record on the static draw-record list: the packet's
// matrix, it in view space (current_surface +672's +752 matrix) and the
// camera position, with the packet's bones (+80, +88 of them, 48 bytes
// each) copied to the frame's stack allocator, all handed to
// DrawRecordList_Add(r->+0, &record) (name?).
DECOMP(004FDBB8, DrawRecord_Submit) {
    const u32 r = arg<u32>(ctx, 0), pk = arg<u32>(ctx, 1);
    Stack frame(ctx, kSubmitFrame);
    const u32 sp = frame.addr();
    store<u32>(sp + kSbRec, r);
    Vec4 s[4];
    for (u32 i = 0; i < 4; ++i) {
        for (u32 k = 0; k < 3; ++k) s[i].w[k] = VLoadLeft(pk + 16 * i + 4 * k).w[0];
        s[i].w[3] = 0;
    }
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 3; ++k) store<u32>(sp + kSbMatrix + 16 * i + 4 * k, s[i].w[k]);
    const u32 view = load<u32>(load<u32>(kCurrentSurface) + kSurfaceView);
    Vec4 v[4], out[4];
    MatLoad(view + kViewMatrix, v);
    MatCombine(v, s, out);
    MatStore(sp + kSbViewMatrix, out);
    const u32 cam = view + obj12::kViewCamPos;
    const u32 n = load<u8>(pk + kDpBoneCount);
    u32 src = load<u32>(pk + kDpBones);
    for (u32 k = 0; k < 12; k += 4) store<u32>(sp + kSbCamera + k, load<u32>(cam + k));
    store<u8>(sp + kSbCount, static_cast<u8>(n));
    const u32 dst0 = static_cast<u32>(call(ctx, obj4::o4fn::MemStack_Alloc, {load<u32>(kFrameAlloc + kFaStack), n * kBoneOut}));
    store<u32>(sp + kSbBones, dst0);
    for (u32 i = 0, dst = dst0; i < n; ++i, src += kBoneOut, dst += kBoneOut)
        for (u32 k = 0; k < kBoneOut; k += 4) store<u32>(dst + k, load<u32>(src + k));
    call(ctx, DrawRecordList_Add, {load<u32>(r), sp + kSbRec});
}

namespace {

// ---------------------------------------------------------------- DrawPacket_Begin: the packet's light block
// 208 bytes from the frame allocator, at packet +92: +0 u32 the point /
// directional lights after the first, +16 vec4 the ambient colour (w 1),
// +32 vec4 light 0's colour (w 1), +48 vec3 light 0's direction in object
// space (w its parameters' w), then 48-byte entries {position or
// direction (w 1), colour (w 1), parameters} (name?).
constexpr u32 kDpLights = 64;                   // the packet's light cache (obj +288)
constexpr u32 kDpLightBlock = 92;
constexpr u32 kLbSize = 208, kLbCount = 0, kLbAmbient = 16, kLbColour0 = 32, kLbDir0 = 48, kLbEntries = 64;
constexpr u32 kLbEntrySize = 48;
constexpr u32 kLbConsts = 0x0079D8F0;           // f32 1, then the two vperm masks at +16 / +32
constexpr u32 kOneBits = 0x3F800000u;

inline Vec4 VMrgh(const Vec4& a, const Vec4& b) { return Vec4{{a.w[0], b.w[0], a.w[1], b.w[1]}}; }
inline Vec4 VMrgl(const Vec4& a, const Vec4& b) { return Vec4{{a.w[2], b.w[2], a.w[3], b.w[3]}}; }
inline Vec4 VSub(const Vec4& a, const Vec4& b) {  // vsubfp
    Vec4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = std::bit_cast<u32>(a.f(i) - b.f(i));
    return r;
}

// cx * v.x + cy * v.y + cz * v.z (the vmaddfp chain from 0).
inline Vec4 Rotate(const Vec4& cx, const Vec4& cy, const Vec4& cz, const Vec4& x, const Vec4& y, const Vec4& z) {
    Vec4 a = VMadd(cx, x, VZero());
    a = VMadd(cy, y, a);
    return VMadd(cz, z, a);
}

inline void StoreNormal(u32 dst, f32 x, f32 y, f32 z, f32 one) {
    const f32 inv = Fdivs(one, obj10::Fsqrts(Fmadds(z, z, Fmadds(x, x, Fmuls(y, y)))));
    store<f32>(dst, Fmuls(x, inv));
    store<f32>(dst + 4, Fmuls(y, inv));
    store<f32>(dst + 8, Fmuls(z, inv));
}

}  // namespace

// void DrawPacket_Begin(DrawPacket* pk)
// Builds the packet's light block from its light cache (+64; nothing
// without one) in object space (the packet's rotation transposed): the
// ambient colour, light 0 as a direction (the cache position rotated,
// negated and normalised) and, for entries 1-3, type 2 lights at their
// object-space position (rotated plus the rotated negated translation) and
// type 1 lights along their normalised rotated negated position with
// parameters {1, 0, 0, w}. The block comes from the frame allocator
// unchecked. r3 = 8 (the original's lvlx offset) (name?).
DECOMP(0079D920, DrawPacket_Begin) {
    const u32 pk = arg<u32>(ctx, 0);
    const u32 c = load<u32>(pk + kDpLights);
    if (c == 0) return;
    const u32 out = load<u32>(kFrameAlloc + kFaCursor);
    store<u32>(kFrameAlloc + kFaCursor, out + kLbSize);
    store<u32>(pk + kDpLightBlock, out);
    for (u32 k = 0; k < 12; k += 4) store<u32>(out + kLbAmbient + k, load<u32>(c + kLcAmbient + k));
    const f32 one = Lf(kLbConsts);
    store<u32>(out + kLbAmbient + 12, kOneBits);
    store<u32>(out + kLbCount, 0);
    // The rotation's columns: x via merges, y / z through the vperm masks.
    const Vec4 r0 = VLoad(pk), r2 = VLoad(pk + 32), r1 = VLoad(pk + 16);
    const Vec4 hi = VMrgh(r0, r2), lo = VMrgl(r0, r2);
    const Vec4 cx = VMrgh(hi, r1);
    const Vec4 cy = VPerm(hi, r1, VLoad(kLbConsts + 16));
    const Vec4 cz = VPerm(lo, r1, VLoad(kLbConsts + 32));
    const u32 p0 = c + kLcPos;
    const Vec4 d0 = Rotate(cx, cy, cz, VSplat(VLoadLeft(p0), 0), VSplat(VLoadLeft(p0 + 4), 0), VSplat(VLoadLeft(p0 + 8), 0));
    const Vec4 nt = VSub(VZero(), VLoad(pk + 48));
    for (u32 k = 0; k < 12; k += 4) store<u32>(out + kLbColour0 + k, load<u32>(c + kLcColour + k));
    store<u32>(out + kLbColour0 + 12, kOneBits);
    store<u32>(out + kLbDir0 + 12, load<u32>(c + kLcParams + 12));
    const Vec4 tl = Rotate(cx, cy, cz, VSplat(nt, 0), VSplat(nt, 1), VSplat(nt, 2));
    StoreNormal(out + kLbDir0, -d0.f(0), -d0.f(1), -d0.f(2), one);

    u32 e = out + kLbEntries;
    for (u32 k = 1; k < kLcEntries; ++k) {
        const u32 pos = c + kLcPos + kLcStride * k, col = c + kLcColour + kLcStride * k;
        const u32 par = c + kLcParams + kLcStride * k;
        const u8 type = load<u8>(c + kLcType + k);
        if (type == kLightSpot) {
            const Vec4 p = VAdd(Rotate(cx, cy, cz, VSplat(VLoadLeft(pos), 0), VSplat(VLoadLeft(pos + 4), 0),
                                       VSplat(VLoadLeft(pos + 8), 0)), tl);
            for (u32 j = 0; j < 3; ++j) store<u32>(e + 4 * j, p.w[j]);
            store<u32>(e + 12, kOneBits);
            for (u32 j = 0; j < 12; j += 4) store<u32>(e + 16 + j, load<u32>(col + j));
            store<u32>(e + 28, kOneBits);
            for (u32 j = 0; j < 16; j += 4) store<u32>(e + 32 + j, load<u32>(par + j));
        } else if (type == static_cast<u8>(kLightDirectional)) {
            const f32 x = -Lf(pos), y = -Lf(pos + 4), z = -Lf(pos + 8);
            const Vec4 d = Rotate(cx, cy, cz, VFill(std::bit_cast<u32>(x)), VFill(std::bit_cast<u32>(y)),
                                  VFill(std::bit_cast<u32>(z)));
            StoreNormal(e, d.f(0), d.f(1), d.f(2), one);
            store<u32>(e + 12, kOneBits);
            for (u32 j = 0; j < 12; j += 4) store<u32>(e + 16 + j, load<u32>(col + j));
            store<u32>(e + 28, kOneBits);
            const u32 w = load<u32>(par + 12);
            store<u32>(e + 32, kOneBits);
            store<u32>(e + 36, 0);
            store<u32>(e + 40, 0);
            store<u32>(e + 44, w);
        } else {
            continue;
        }
        store<u32>(out + kLbCount, load<u32>(out + kLbCount) + 1);
        e += kLbEntrySize;
    }
    ctx.gpr[3] = 8;
}
