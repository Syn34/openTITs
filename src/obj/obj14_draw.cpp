// World objects, part 14: the draw-record list insert DrawRecord_Submit
// hands its records to.
#include "obj14.h"

using namespace sly4;
using namespace sly4::obj8;
using namespace sly4::obj9;
using namespace sly4::obj10;
using namespace sly4::obj12;
using namespace sly4::obj14;

namespace {

constexpr u32 PtrVector_Reserve = 0x00551718;   // (PtrVector*, u32 capacity)
constexpr u32 Listener_LocalDirection = 0x005532E4;  // (f32 out[3], const f32 pos[3], const f32 m[4][4])
constexpr u32 libc_memmove = 0x0071C3F0;
constexpr u32 DrawRecordList_Flush = 0x0056E610;  // (first entry, u32* stride, u32* count, u32** counter) (name?)

constexpr u32 kFrameAlloc = 0x010DD600, kFaStack = 280;   // ApeMemStack*: the frame's stack allocator
constexpr u32 kDepthScale = 0x010DD780 + 612;   // f32: the view constants block's depth scale (name?)

// The static draw-record list (obj12.h kTrList) as DrawRecordList_Init
// sets it up: +0 u8 enabled, +4 PtrVector, +10 u16 its capacity hint,
// +24 PtrVector* the list the entries go to, +28 u32 the entries' tag,
// +32 u32, +128 / +256 u32 atomics (name?).
constexpr u32 kTlEnabled = 0, kTlVector = 4, kTlVectorCap = 10, kTlTarget = 24, kTlTag = 28, kTl32 = 32;
constexpr u32 kTlAtomicA = 128, kTlAtomicB = 256;
constexpr u16 kTlInitialCap = 160;
// A second function-local static: a pointer to the list's +128 counter,
// guarded by the u8 at 0x010F0B80 (name?).
constexpr u32 kTlCounterGuard = 0x010F0B80, kTlCounterPtr = 0x010F0B90;

constexpr u32 kPvMax = 256;                     // entries beyond this are dropped

// The submit record (obj13_draw.cpp DrawRecord_Submit): +0 the draw
// record, +4 u8 the bone count, +8 the bones, +16 f32[4][3] the matrix,
// +80 f32[4][4] it in view space, +144 f32[3] the camera position.
constexpr u32 kSrRecord = 0, kSrBoneCount = 4, kSrBones = 8, kSrMatrix = 16, kSrViewZ = 128 + 8, kSrCamera = 144;
// The draw record: +4 ->+48 {u32, AglMaterial*}[] per draw, +8 u8[4] the
// colour (the last byte the alpha), +12 f32 the near fade scale (name?).
constexpr u32 kDrTopo = 4, kDrTopoDraws = 48, kDrColour = 8, kDrScale = 12;
// The list owner (DrawRecordList_Add's first argument): +4 u16 the draw
// count, +8 DrawDef*[] per draw. A DrawDef: +4 f32 the fade start, +8 f32
// the slot value, +12 u16 the slot count, +16 u8 bit 7 = takes the bones
// (name?).
constexpr u32 kOwCount = 4, kOwDefs = 8;
constexpr u32 kDdFadeStart = 4, kDdSlotValue = 8, kDdSlots = 12, kDdFlags = 16, kDdTakesBones = 0x80;
constexpr u32 kDdCopy = 17;                     // bytes copied into the entry
// The 80-byte entry: +0 the DrawDef's first 17 bytes, +32 vec4 the camera
// in the record's frame (w 0), +48 u16 the bone count / +52 the bones
// (only with kDdTakesBones), +56 the list tag, +64 the 352-byte draw.
constexpr u32 kEnSize = 80, kEnCamera = 32, kEnBoneCount = 48, kEnBones = 52, kEnTag = 56, kEnDraw = 64;
// The 352-byte draw: +0 per slot {f32 value, -, u16 0 at +8} (16 bytes
// each), +256 u16 the slot count, +272 f32[4][3] the matrix rows (16-byte
// stride), +336 u8[4] the colour (alpha replaced), +340 f32 the fade.
constexpr u32 kDwSize = 352, kDwSlotStride = 16, kDwSlotZero = 8, kDwSlots = 256, kDwMatrix = 272, kDwColour = 336, kDwFade = 340;

// DrawRecordList_Add's constants: f32 {1, 8, 2.25, 10}, f64 1, f64 0 (+20
// read as f32 0), then the camera's vperm mask.
constexpr u32 kDaConsts = 0x004FD2A0;
constexpr u32 kDaOne = 0, kDaFar = 4, kDaFarScale = 8, kDaFadeLen = 12, kDaOneD = 16, kDaZeroD = 24, kDaZero = 20;

inline u32 Alloc(PPUContext& ctx, u32 size) {
    return static_cast<u32>(call(ctx, obj4::o4fn::MemStack_Alloc, {load<u32>(kFrameAlloc + kFaStack), size}));
}

}  // namespace

// void DrawRecordList_Add(ListOwner* o, const SubmitRecord* r)
// Turns a submitted draw record into list entries, one per visible draw
// (material +100 bit 0x20) whose DrawDef has slots: the depth d = -(view z
// * the depth scale), the near scale (the record's +12, 2.25 beyond 8,
// blended towards 2.25 between 1 and 8) and the fade (1 - clamp((d -
// DrawDef +4) / 10, 0, 1)) times that scale; draws with a zero alpha or
// fade are skipped. Each kept draw gets an 80-byte entry and a 352-byte
// draw from the frame's stack allocator, and the draw is pushed onto the
// list's +24 vector (dropped once that holds 256: the allocations are
// then lost, as in the original). Finally the entry stride (|last - first|
// / (count - 1), 80 for one entry) and count go to DrawRecordList_Flush
// with the list's counter, after an atomic increment of that counter. The
// list is set up inline on first use (DrawRecordList_Init). r3 = 0 when
// nothing was kept.
DECOMP(004FD2E0, DrawRecordList_Add) {
    const u32 o = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    if (load<u8>(kTrListGuard) == 0) {
        store<u8>(kTrList + kTlEnabled, 1);
        call(ctx, o12fn::PtrVector_Construct, {kTrList + kTlVector});
        store<u16>(kTrList + kTlVectorCap, kTlInitialCap);
        store<u32>(kTrList + kTl32, 0);
        store<u32>(kTrList + kTlAtomicA, 0);  // lwarx / stwcx. store
        store<u32>(kTrList + kTlAtomicB, 0);  // lwarx / stwcx. store
        store<u8>(kTrListGuard, 1);
    }
    Stack frame(ctx, 304);
    const u32 sp = frame.addr();
    const f32 depth = -Fmuls(Lf(r + kSrViewZ), Lf(kDepthScale));
    ctx.fpr[1] = Lf(r + kSrViewZ);
    const u32 count = Alloc(ctx, 4);
    const u32 stride = Alloc(ctx, 4);
    u32 first = 0, last = 0;
    store<u32>(count, 0);
    store<u32>(stride, 0);
    const f32 one = Lf(kDaConsts + kDaOne), far = Lf(kDaConsts + kDaFar), farScale = Lf(kDaConsts + kDaFarScale);
    const f32 fadeLen = Lf(kDaConsts + kDaFadeLen), zero = Lf(kDaConsts + kDaZero);
    const f64 oneD = load<f64>(kDaConsts + kDaOneD), zeroD = load<f64>(kDaConsts + kDaZeroD);
    for (u32 i = 0; i < load<u16>(o + kOwCount); i = (i + 1) & 0xFFFF) {
        const u32 rec = load<u32>(r + kSrRecord);
        const u32 mat = load<u32>(load<u32>(load<u32>(rec + kDrTopo) + kDrTopoDraws) + 8 * i + 4);
        if ((load<u8>(mat + kMatFlags100) & kMatVisible) == 0) continue;
        const u32 def = load<u32>(load<u32>(o + kOwDefs) + 4 * i);
        if (load<u16>(def + kDdSlots) == 0) continue;
        const u8 c0 = load<u8>(rec + kDrColour), c1 = load<u8>(rec + kDrColour + 1), c2 = load<u8>(rec + kDrColour + 2);
        const u8 alpha = load<u8>(rec + kDrColour + 3);
        f64 scale = Lf(rec + kDrScale);
        ctx.fpr[1] = zero;  // f1 as the original leaves it
        const f32 dist = Fsubs(depth, Lf(def + kDdFadeStart));
        if (depth > far) {
            scale = farScale;
        } else if (one < depth) {
            const f32 q = Fdivs(Fsubs(depth, one), Fsubs(far, one));
            scale = Fmadds(Fsubs(scale, farScale), Fsubs(one, q), farScale);
        }
        f64 t = Fdivs(dist, fadeLen);
        t = static_cast<f32>(Fsubs(one, t) >= 0.0 ? t : oneD);  // fsel / frsp
        t = static_cast<f32>(t >= 0.0 ? t : zeroD);
        const f32 fade = Fmuls(scale, Fsubs(one, t));
        const f32 a = static_cast<f32>(static_cast<f64>(alpha));
        if (a < zero || a == zero) continue;
        if (fade < zero || fade == zero) continue;  // NaN carries on
        const u32 draw = Alloc(ctx, kDwSize);
        last = Alloc(ctx, kEnSize);
        const u32 n = load<u32>(count);
        if (first == 0) first = last;
        store<u32>(count, n + 1);
        call(ctx, libc_memmove, {last, def, kDdCopy});
        store<u32>(last + kEnDraw, draw);
        if (load<u8>(def + kDdFlags) & kDdTakesBones) {
            store<u16>(last + kEnBoneCount, load<u8>(r + kSrBoneCount));
            store<u32>(last + kEnBones, load<u32>(r + kSrBones));
        }
        call(ctx, Listener_LocalDirection, {sp + 112, r + kSrCamera, r + kSrMatrix});
        const u32 cam = (last + kEnCamera) & ~15u;  // stvx
        store<u32>(cam, load<u32>(sp + 112));
        store<u32>(cam + 4, load<u32>(sp + 116));
        store<u32>(cam + 8, load<u32>(sp + 120));
        store<u32>(cam + 12, 0);
        store<u32>(last + kEnTag, load<u32>(kTrList + kTlTag));
        const u32 slots = load<u16>(def + kDdSlots);
        const u32 value = load<u32>(def + kDdSlotValue);
        store<u16>(draw + kDwSlots, static_cast<u16>(slots));
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k)
                store<u32>(draw + kDwMatrix + 16 * row + 4 * k, load<u32>(r + kSrMatrix + 16 * row + 4 * k));
        store<u8>(draw + kDwColour, c0);
        store<u8>(draw + kDwColour + 1, c1);
        store<u8>(draw + kDwColour + 2, c2);
        store<u8>(draw + kDwColour + 3, alpha);  // via fctidz of the float alpha
        store<f32>(draw + kDwFade, fade);
        ctx.fpr[1] = Lf(r + kSrMatrix + 56);
        for (u32 j = 0; j < slots; ++j) {  // unchecked: slots beyond 16 run past the 352 bytes
            store<u32>(draw + kDwSlotStride * j, value);
            store<u16>(draw + kDwSlotStride * j + kDwSlotZero, 0);
        }
        const u32 vec = load<u32>(kTrList + kTlTarget);
        u32 cnt = load<u16>(vec + kPvCount);
        if (cnt >= kPvMax) continue;
        const u32 cap = load<u16>(vec + kPvCap);
        if (cnt == cap) {
            call(ctx, PtrVector_Reserve, {vec, cap << 1});
            cnt = load<u16>(vec + kPvCount);
        }
        store<u32>(load<u32>(vec + kPvItems) + 4 * cnt, draw);
        const u32 next = load<u16>(vec + kPvCount) + 1;
        const u32 peak = load<u16>(vec + kPvPeak);
        store<u16>(vec + kPvCount, static_cast<u16>(next));
        store<u16>(vec + kPvPeak, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(next & 0xFFFF) ? peak : next));
    }
    const u32 kept = load<u32>(count);
    u32 step = kEnSize;
    if (kept > 1) step = (last > first ? last - first : first - last) / (kept - 1);
    store<u32>(stride, step);
    if (load<u32>(count) == 0) {
        ctx.gpr[3] = 0;
        ctx.gpr[4] = 0;  // the count, as the original leaves it
        return;
    }
    if (load<u8>(kTlCounterGuard) == 0) {
        store<u32>(kTlCounterPtr, kTrList + kTlAtomicA);
        store<u8>(kTlCounterGuard, 1);
    }
    const u32 counter = load<u32>(kTlCounterPtr);
    for (;;) {  // lwarx / stwcx. loop: atomic increment
        const u32 old = mem::r32(counter);
        if (mem::cas32(counter, old, old + 1)) break;
    }
    call(ctx, DrawRecordList_Flush, {last > first ? first : last, stride, count, kTlCounterPtr});
}
