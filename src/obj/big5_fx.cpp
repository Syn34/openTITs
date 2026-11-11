// Game code, large-function sweep part 5: the effect objects of the
// FX pool kinds: Obj8733_Init (002ED468), FxGeo_SetDef (002EC2F8).
#include "big5.h"

using namespace sly4;
using namespace sly4::big5;

namespace {
// Obj8733 (kind 0x8733FD75, 156 bytes)
constexpr u32 k8733Src = 16;      // the source object (vfn[12] -> name, vfn[56](12) -> a new instance)
constexpr u32 k8733Model = 20;    // a WorldObj to instance, or 0
constexpr u32 k8733Count = 24;    // u16: bits 1-5 = the number of delay copies
constexpr u32 k8733Clear0 = 28;   // u16
constexpr u32 k8733Clear1 = 30;   // u16
constexpr u32 k8733Clear2 = 48;   // u32
constexpr u32 k8733Alpha = 56;    // f32 per copy, 8 bytes apart
constexpr u32 k8733Copies = 116;  // WorldObj*[8]
constexpr u32 kFmtDelay = 0x00AD9674u;  // "FXDly%02d_%s"
constexpr f32 k255 = 255.0f;            // 0x002ED448
constexpr u32 kWobjFlags322 = 322;      // u8: 0x80 = (cleared, then vfn[36])
}  // namespace

// u32 Obj8733_Init(Obj8733* o)
// Releases the old copies (vfn[8] of each non-null o->116[k], slot
// cleared), then, with a source and a non-zero count, makes `count`
// delayed copies: an instance of o->20 (or the source's vfn[56](12)),
// named "FXDly%02d_%s" (1-based index, the source's vfn[12] name), vfn[40]
// (0, 0), vfn[52] with the byte of o->56[8i] * 255 (fctidz; f1 holds the
// converted integer as the original leaves it), and with flag 0x80 at
// +322 that flag cleared and vfn[36]. The count is re-read every
// iteration. Ends clearing +28 / +30 / +48. r3 is what the original
// leaves there (PoolObj8733_Construct returns it): the count when copies
// were made, 0 for a zero count, else the last slot pass's value.
DECOMP(002ED468, Obj8733_Init) {
    const u32 o = arg<u32>(ctx, 0);
    u64 r3 = 0;
    for (u32 k = 0; k < 8; ++k) {
        const u32 slot = o + k8733Copies + 4 * k;
        const u32 p = load<u32>(slot);
        r3 = p;
        if (p != 0) {
            r3 = VCall(ctx, p, 8, {p});
            store<u32>(slot, 0);
        }
    }
    u32 src = load<u32>(o + k8733Src);
    if (src != 0) {
        u32 n = (load<u16>(o + k8733Count) >> 1) & 31u;
        r3 = n;
        for (u32 i = 0; i < n;) {
            const u32 model = load<u32>(o + k8733Model);
            u32 inst;
            if (model != 0) inst = static_cast<u32>(call(ctx, b5fn::kWorldObjInstance, {model, 0}));
            else inst = static_cast<u32>(VCall(ctx, src, 56, {src, 12}));
            i += 1;
            const u32 s = load<u32>(o + k8733Src);
            const u32 name = static_cast<u32>(VCall(ctx, s, 12, {s}));
            const u32 str = static_cast<u32>(call(ctx, b5fn::kStringFormat, {kFmtDelay, i, name}));
            call(ctx, b5fn::kWorldObjSetName, {inst, str, 1});
            VCall(ctx, inst, 40, {inst, 0, 0});
            const f32 a = static_cast<f32>(static_cast<f64>(load<f32>(o + k8733Alpha + 8 * (i - 1))) * static_cast<f64>(k255));
            const u64 iv = rt::fctid(static_cast<f64>(a), true);
            ctx.fpr[1] = std::bit_cast<f64>(iv);
            VCall(ctx, inst, 52, {inst, iv & 0xFF, 0});
            const u8 fl = load<u8>(inst + kWobjFlags322);
            if ((fl & 0x80u) != 0) {
                store<u8>(inst + kWobjFlags322, static_cast<u8>(fl & 0x7Fu));
                VCall(ctx, inst, 36, {inst});
            }
            store<u32>(o + k8733Copies + 4 * (i - 1), inst);
            n = (load<u16>(o + k8733Count) >> 1) & 31u;
            src = load<u32>(o + k8733Src);
            r3 = n;
        }
    }
    store<u16>(o + k8733Clear0, 0);
    store<u16>(o + k8733Clear1, 0);
    store<u32>(o + k8733Clear2, 0);
    ctx.gpr[3] = r3;
}

namespace {
// ObjD0E9 (kind 0xD0E940EF, FXAnimControlGeoDisplay)
constexpr u32 kD0E9Ent = 12;        // ->+132 ->+48 -> the mesh
constexpr u32 kD0E9Verts = 24;      // 32-byte vertices {f32 pos[3], pad, u8 color[4], f32 uv[2], pad}
constexpr u32 kD0E9NVerts = 28;
constexpr u32 kD0E9Tris = 32;       // u16[3] per triangle, far to near
constexpr u32 kD0E9NIdx = 36;       // the strip's index count
// The mesh
constexpr u32 kMeshVB = 24;         // buffer {+0 data, +4 flags, +8 mode}
constexpr u32 kMeshNVerts = 40;
constexpr u32 kMeshIB = 44;         // buffer of u16 strip indices
constexpr u32 kMeshNIdx = 64;
constexpr u32 kMeshFormat = 68;     // vertex format entries; u16 +120 the stride
constexpr u32 kFmtStride = 120;
constexpr f32 k3 = 3.0f;                 // 0x002EC2F0
constexpr u32 kCmpF32Opd = 0x00F36170u;  // CmpF32_00767A04

// The buffer's data pointer after setting its mode bits 28-30 to 1 (both
// branches on flag bit 0 read +0).
u32 BufLock(u32 b) {
    store<u32>(b + 8, (load<u32>(b + 8) & ~0xEu) | 2u);
    return load<u32>(b);
}
f32 FAdd(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) + static_cast<f64>(b)); }
f32 FDiv(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) / static_cast<f64>(b)); }
f32 FMul(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * static_cast<f64>(b)); }
f32 FMadd(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); }
}  // namespace

// void FxGeo_SetDef(ObjD0E9* o, GeomDef*)
// (The second argument is not read.) Copies the mesh o->12->132->48->0
// into the object: its vertices (position from format entry 0, the color
// bytes from entry 3, the uv from entry 8, at the format's stride) into a
// new 32-byte-per-vertex array (+24, count +28), and its index strip
// (count +36) as a triangle list sorted by the squared distance of each
// triangle's centroid from the origin, ascending (qsort with CmpF32 over an
// inlined {f32 key, u32 triangle} array on the stack), into a new u16
// array at +32. The unsorted triangles go through a scratch block on the
// default memory stack, freed at the end.
DECOMP(002EC2F8, FxGeo_SetDef) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 geo = load<u32>(load<u32>(o + kD0E9Ent) + 132);
    const u32 mesh = load<u32>(load<u32>(geo + 48));
    if (mesh == 0) return ret(ctx, geo);  // (r3 as the original leaves it)
    store<u32>(o + kD0E9Verts, static_cast<u32>(call(ctx, b5fn::kMemAlloc, {load<u32>(mesh + kMeshNVerts) << 5, 0, 16})));
    store<u32>(o + kD0E9NVerts, load<u32>(mesh + kMeshNVerts));
    const u32 fmt = load<u32>(mesh + kMeshFormat);
    const u32 e0 = static_cast<u32>(call(ctx, b5fn::kFindEntry, {fmt, 0, 0}));
    const u32 e1 = static_cast<u32>(call(ctx, b5fn::kFindEntry, {fmt, 3, 0}));
    const u32 e2 = static_cast<u32>(call(ctx, b5fn::kFindEntry, {fmt, 8, 0}));
    const u32 vb = BufLock(mesh + kMeshVB);
    u32 pPos = vb + load<u8>(e0 + 8), pCol = vb + load<u8>(e1 + 8), pUv = vb + load<u8>(e2 + 8);
    const u32 nIdx = load<u32>(mesh + kMeshNIdx);
    if (load<u32>(o + kD0E9NVerts) != 0) {
        u32 last = 0;
        for (u32 i = 0, off = 0; i < load<u32>(o + kD0E9NVerts); ++i, off += 32) {
            const u32 v = load<u32>(o + kD0E9Verts) + off;
            const u32 x = load<u32>(pPos), y = load<u32>(pPos + 4), z = load<u32>(pPos + 8);  // (lfs x3, stfs x3)
            store<u32>(v, x);
            store<u32>(v + 4, y);
            store<u32>(v + 8, z);
            const u32 c = load<u32>(o + kD0E9Verts) + off + 16;
            const u8 b0 = load<u8>(pCol), b1 = load<u8>(pCol + 1), b2 = load<u8>(pCol + 2), b3 = load<u8>(pCol + 3);
            store<u8>(c, b0);
            store<u8>(c + 1, b1);
            store<u8>(c + 2, b2);
            store<u8>(c + 3, b3);
            const u32 t = load<u32>(o + kD0E9Verts) + off + 20;
            last = load<u32>(pUv);
            const u32 vv = load<u32>(pUv + 4);
            store<u32>(t, last);
            store<u32>(t + 4, vv);
            const u32 stride = load<u16>(fmt + kFmtStride);
            pPos += stride;
            pCol += stride;
            pUv += stride;
        }
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(last));  // f1 as the copy loop leaves it
    }
    store<u32>(o + kD0E9NIdx, nIdx);
    const u64 triBytes = static_cast<u64>(nIdx) * 6 - 12;  // (n - 2) triangles of 3 u16 (64-bit as the original)
    store<u32>(o + kD0E9Tris, static_cast<u32>(call(ctx, b5fn::kMemAlloc, {triBytes, 0, 16})));
    const u32 stk = static_cast<u32>(call(ctx, b5fn::kMemGetDefaultStack, {}));
    const u32 tmp = static_cast<u32>(call(ctx, b5fn::kMemStackAlloc, {stk, static_cast<u64>(load<u32>(o + kD0E9NIdx)) * 6 - 12}));

    // The sorted array {u16 cap, count, high water, flags (element size << 1
    // | unsorted), data, allocator, 0, compare} (r1+112 in the original).
    Stack frame(ctx, 24);
    const u32 arr = frame.addr();
    store<u16>(arr + 0, static_cast<u16>(load<u32>(o + kD0E9NIdx)));
    store<u16>(arr + 2, 0);
    store<u16>(arr + 4, 0);
    store<u16>(arr + 6, 16);
    store<u32>(arr + 8, 0);
    const u32 alloc = static_cast<u32>(call(ctx, b5fn::kMemDefaultAllocator, {}));
    store<u32>(arr + 12, alloc);
    const u32 cap0 = load<u16>(arr + 0);
    if (cap0 != 0) store<u32>(arr + 8, static_cast<u32>(VCall(ctx, alloc, 0, {alloc, cap0 * ((load<u16>(arr + 6) >> 1) & 0x7FFFu), 0, 16})));
    call(ctx, b5fn::kArrayInit, {arr});
    store<u32>(arr + 20, kCmpF32Opd);
    store<u32>(arr + 16, 0);
    const u32 ib = BufLock(mesh + kMeshIB);

    if (load<u32>(o + kD0E9NIdx) > 2) {
        u32 out = 0;  // byte offset into tmp
        u32 tri = 0;
        for (u32 k = 2, io = 4; k < load<u32>(o + kD0E9NIdx); ++k, ++tri, io += 2, out += 6) {
            const u32 verts = load<u32>(o + kD0E9Verts);
            const u32 v0 = verts + (static_cast<u32>(load<u16>(ib + io - 4)) << 5);
            const u32 v1 = verts + (static_cast<u32>(load<u16>(ib + io - 2)) << 5);
            const u32 v2 = verts + (static_cast<u32>(load<u16>(ib + io)) << 5);
            const auto f = [](u32 a) { return load<f32>(a); };
            f32 y = FAdd(f(v0 + 4), f(v1 + 4));
            f32 x = FAdd(f(v0), f(v1));
            f32 z = FAdd(f(v0 + 8), f(v1 + 8));
            y = FAdd(y, f(v2 + 4));
            x = FAdd(x, f(v2));
            z = FAdd(z, f(v2 + 8));
            y = FDiv(y, k3);
            x = FDiv(x, k3);
            z = FDiv(z, k3);
            const f32 xy = FMadd(x, x, FMul(y, y));
            const f32 d = FMadd(z, z, xy);
            ctx.fpr[1] = static_cast<f64>(xy);  // f1 as the original leaves it
            const u32 cap = load<u16>(arr + 0);
            if (load<u16>(arr + 2) == cap) call(ctx, b5fn::kArrayReserve, {arr, (cap << 1) & 0xFFFEu});
            const u32 data = load<u32>(arr + 8);
            store<f32>(data + (static_cast<u32>(load<u16>(arr + 2)) << 3), d);
            store<u32>(data + (static_cast<u32>(load<u16>(arr + 2)) << 3) + 4, tri);
            const u32 cnt = static_cast<u32>(load<u16>(arr + 2)) + 1;
            const u32 hw = load<u16>(arr + 4), fl = load<u16>(arr + 6);
            store<u16>(arr + 2, static_cast<u16>(cnt));
            store<u16>(arr + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(cnt & 0xFFFFu) ? hw : cnt));
            store<u16>(arr + 6, static_cast<u16>(fl | 1u));
            store<u16>(tmp + out, load<u16>(ib + io - 4));
            store<u16>(tmp + out + 2, load<u16>(ib + io - 2));
            store<u16>(tmp + out + 4, load<u16>(ib + io));
        }
    }
    // Sort if unsorted (flag cleared first).
    const u32 fl = load<u16>(arr + 6);
    if ((fl & 1u) != 0) {
        store<u16>(arr + 6, static_cast<u16>(fl & 0xFFFEu));
        const u32 cnt = load<u16>(arr + 2);
        if (static_cast<s32>(cnt) > 0) call(ctx, b5fn::kQsort, {load<u32>(arr + 8), cnt, 8, load<u32>(arr + 20)});
    }
    for (u32 i = 0, src = 0, dst = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(arr + 2)); ++i, src += 8, dst += 6) {
        const u32 t3 = load<u32>(load<u32>(arr + 8) + src + 4) * 3;
        store<u16>(load<u32>(o + kD0E9Tris) + dst, load<u16>(tmp + ((t3 << 1) & ~1u)));
        store<u16>(load<u32>(o + kD0E9Tris) + dst + 2, load<u16>(tmp + ((t3 + 1) << 1)));
        store<u16>(load<u32>(o + kD0E9Tris) + dst + 4, load<u16>(tmp + ((t3 + 2) << 1)));
    }
    const u32 stk2 = static_cast<u32>(call(ctx, b5fn::kMemGetDefaultStack, {}));
    VCall(ctx, stk2, 8, {stk2, tmp});  // free the scratch block
    const u32 bytes = static_cast<u32>(load<u16>(arr + 0)) * ((static_cast<u32>(load<u16>(arr + 6)) >> 1) & 0x7FFFu);
    const u32 data = load<u32>(arr + 8);
    if (data != 0) {
        const u32 al = load<u32>(arr + 12);
        VCall(ctx, al, 8, {al, data, bytes});
    }
    call(ctx, b5fn::kArrayDestruct, {arr});
}
