// Game code, large-function sweep part 4: the constructor of the class
// with vtable 0x00B13CF0 (00108AF8, called from 00215D98; its destructor
// is 001096C0), with the 0x922D3BD1 property-set read inlined.
#include "big4.h"

using namespace sly4;
using namespace sly4::big4;

namespace b4c {
constexpr u32 kVtbl = 0x00B13CF0u;
constexpr u32 kSetFull = 0x922D3BD1u;  // the property set read (as in PropSet_ReadFlags)
constexpr u32 kOne = 0x3F800000u;      // 1.0f
// The tunables copied into +28..+72 (each the f32 at +72 of a 0x4C-byte
// tuning record).
constexpr u32 kTune[12] = {0x01034068u, 0x010340B4u, 0x01034100u, 0x0103414Cu, 0x01034198u, 0x010341E4u,
                           0x01034230u, 0x0103427Cu, 0x010342C8u, 0x01033FD0u, 0x0103401Cu, 0};
constexpr u32 kVecZero = 0x01164B40u;  // the f32[3] globals copied into the vectors
constexpr u32 kVecB = 0x01164B50u, kVecC = 0x01164B70u, kMat = 0x01164BD0u, kBytes4 = 0x01164CA8u;
// The f32 properties: name hash -> field.
struct FProp {
    u32 hash, off;
};
constexpr FProp kF32Props[12] = {{0xAF9EDBBDu, 28}, {0x216839FBu, 32}, {0x000110ADu, 36}, {0x216C76DAu, 40},
                                 {0x216C76E8u, 44}, {0x66BCC076u, 48}, {0x6A96433Au, 52}, {0x492B3B23u, 56},
                                 {0x492B3B31u, 60}, {0x9A056C52u, 64}, {0xEE3F6AE0u, 68}, {0x230A334Du, 72}};
constexpr u32 kFlag10 = 0x9C5022BEu, kFlag04 = 0x791455E7u, kFlag08 = 0x6199CD19u;
constexpr u32 kVec192[3] = {0x2F3F8EDBu, 0x2F3F8EDCu, 0x2F3F8EDDu};
}  // namespace b4c

namespace {
// PropFind that also gives the search pointer it stops with when the
// name is absent: the hash array + 4 * (entries compared).
u32 PropScan(u32 s, u32 hash, u32& end) {
    const s32 n = load<u16>(s + 12);
    u32 h = load<u32>(s + 16);
    for (s32 i = 0; i < n; ++i) {
        const u32 x = load<u32>(h);
        h += 4;
        if (x == hash) return load<u32>(s + 24) + load<u16>(load<u32>(s + 20) + 2 * static_cast<u32>(i));
        if (x > hash) break;
    }
    end = h;
    return 0;
}
// flags->+176 bit `mask` = the low bit of the u8 property at p.
void SetFlagBit(u32 o, u32 mask, u32 p) {
    const u32 w = load<u32>(o + 176);
    store<u32>(o + 176, (w & ~mask) | ((load<u8>(p) & 1u) != 0 ? mask : 0u));
}
}  // namespace

// T* VtB13CF0_Ctor(T* o, u32 a, Ref* ref, Holder* h, u32 keep_scale)
// +4 a, +8 (u8) 0, +28..+72 tunables (+64 1.0), the vectors +80 / +112 /
// +144 / +160 / +192 / +208 / +224.. from globals, +176 flags
// (w & 0x03FFFFFF) | 0x50000000, +20 h, +24 keep_scale, +288 55.0f,
// +292 7.5f, the rest zeroed, +320..+323 four bytes from 0x01164CA8.
// Then the inlined Obj_SetRefs108A34(o, ref): +12 = ref (ref->+178 u16
// += 1), or for a null ref +16 released (vfn[40](it, 0)) and cleared;
// the old +12 / +16 were zeroed just above, so neither release can run.
// Then, when h->+12 holds the 0x922D3BD1 set, its properties override
// +28..+72, the flags 0x10000000 / 0x04000000 / 0x08000000 (low bit of a
// u8) and +192..+200. Finally +64 = 1.0 when keep_scale is 0. r3 / f1
// are left as the inlined searches leave them (no caller reads them).
DECOMP(00108AF8, VtB13CF0_Ctor_00108AF8) {
    using namespace b4c;
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1);
    const u32 ref = arg<u32>(ctx, 2);
    const u32 h = arg<u32>(ctx, 3);
    const u32 keep_scale = arg<u32>(ctx, 4);

    store<u32>(o + 4, a);
    store<u8>(o + 8, 0);
    for (u32 k = 0; k < 8; ++k) CopyW(o + 28 + 4 * k, kTune[k] + 72);
    store<u32>(o + 64, kOne);
    CopyW(o + 60, kTune[8] + 72);
    CopyW(o + 68, kTune[9] + 72);
    CopyW(o + 72, kTune[10] + 72);
    CopyWords(o + 80, kVecZero, 3);
    store<u32>(o + 96, 0);
    CopyWords(o + 112, kVecB, 3);
    store<u32>(o + 128, 0);
    CopyWords(o + 144, kVecZero, 3);
    const u32 flags = (load<u32>(o + 176) & 0x03FFFFFFu) | 0x50000000u;
    store<u32>(o + 12, 0);
    CopyWords(o + 160, kVecZero, 3);
    store<u32>(o + 0, kVtbl);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, h);
    store<u32>(o + 24, keep_scale);
    store<u32>(o + 176, flags);
    CopyWords(o + 192, kVecZero, 3);
    CopyWords(o + 208, kVecC, 3);
    CopyWords(o + 224, kMat + 0, 3);
    CopyWords(o + 240, kMat + 16, 3);
    CopyWords(o + 256, kMat + 32, 3);
    CopyWords(o + 272, kMat + 48, 3);
    store<u32>(o + 288, 0x425C0000u);  // 55.0f
    store<u32>(o + 292, 0x40F00000u);  // 7.5f
    store<u8>(o + 296, 0);
    store<u32>(o + 300, 0);
    store<u32>(o + 304, 0);
    store<u32>(o + 308, 0);
    store<u8>(o + 312, 0);
    store<u32>(o + 316, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 320 + k, load<u8>(kBytes4 + k));

    // Inlined Obj_SetRefs108A34(o, ref).
    const u32 old12 = load<u32>(o + 12);
    if (old12 != 0) VCall(ctx, old12, 40, {old12, 0});  // unreachable: +12 was just zeroed
    store<u32>(o + 12, ref);
    if (ref != 0) {
        store<u16>(ref + 178, static_cast<u16>(load<u16>(ref + 178) + 1));
    } else {
        const u32 old16 = load<u32>(o + 16);
        if (old16 != 0) {  // unreachable: +16 was just zeroed
            VCall(ctx, old16, 40, {old16, 0});
            store<u32>(o + 16, 0);
        }
    }

    u32 last = load<u32>(kMat + 56);  // the last word loaded with lfs (left in f1)
    const u32 block = load<u32>(load<u32>(o + 20) + 12);
    u32 r3 = block;
    if (block != 0) {
        const s32 k = PropSetIndex(block, kSetFull);
        if (k >= 0) {
            const u32 slot = block + 4 + 4 * static_cast<u32>(k);
            u32 s = load<u32>(slot);
            for (const FProp& f : kF32Props)
                if (const u32 p = PropFind(s, f.hash)) { last = load<u32>(p); store<u32>(o + f.off, last); }
            // The set pointer is read again after each flag store, as the original does.
            if (const u32 p = PropFind(s, kFlag10)) { SetFlagBit(o, 0x10000000u, p); s = load<u32>(slot); }
            if (const u32 p = PropFind(s, kFlag04)) { SetFlagBit(o, 0x04000000u, p); s = load<u32>(slot); }
            for (u32 j = 0; j < 3; ++j)
                if (const u32 p = PropFind(s, kVec192[j])) { last = load<u32>(p); store<u32>(o + 192 + 4 * j, last); }
            u32 scan_end;
            if (const u32 p = PropScan(s, kFlag08, scan_end)) {
                SetFlagBit(o, 0x08000000u, p);
                r3 = load<u8>(p);
            } else {
                r3 = scan_end;
            }
        }
    }
    if (keep_scale == 0) {
        store<u32>(o + 64, kOne);
        r3 = kOne;
    }
    // Leftovers of the inlined searches (no caller reads them).
    ctx.gpr[3] = r3;
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(last));
}
