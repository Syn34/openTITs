// Game code, large-function sweep part 26: the constructor of the 976-byte
// path object of Spawner1D1B18 (reset by Obj_ResetPath1D0F68).
#include "../sly4.h"
#include "big26.h"

using namespace sly4;
using namespace sly4::big26;

namespace {
constexpr u32 Owner_Construct = 0x001ADD08;     // Owner_Construct1ADD08 (o, rec)
constexpr u32 PtrVector_Construct = 0x005516E0; // (vec)
constexpr u32 PtrVector_Init = 0x00551598;      // (vec, 16, 0, 1)
constexpr u32 PtrVector_Reserve = 0x00551718;   // (vec, u32 n)
constexpr u32 CallK2_D956C = 0x000D956C;        // CallK2_000D956C (Obj*) -> the camera thing or 0
constexpr u32 CallK2_D94F4 = 0x000D94F4;        // CallK2_000D94F4 (Obj*) -> controller or 0
constexpr u32 Cam_SetFov = 0x0028BC28;          // Cam_SetFov_0028BC28 (cam; f1)
constexpr u32 Obj_Step = 0x0028A590;            // Obj_Step_0028A590 (cam; f1)
constexpr u32 Obj_FindBone = 0x000DA9D0;        // (Obj*, u32 key) -> bone or 0
constexpr u32 Obj_Entry16 = 0x000DA954;         // Obj_Entry16_000DA954 (Obj*, s32 index)
constexpr u32 Obj_FindByKey = 0x000D1454;       // Obj_FindByKey_000D1454 (Obj*, u32 key)
constexpr u32 Field_Get160Body = 0x000DD880;    // (Obj*) -> f32 m[4][4]
constexpr u32 Obj_ResetPath = 0x001D0F68;       // Obj_ResetPath1D0F68 (Obj*)

constexpr u32 kVt1D1 = 0x00B174B8;      // vtable block: +8 at o+0, +476 at o+380, +516 at o+384
constexpr u32 kIdentity = 0x01164BD0;   // identity matrix
constexpr u32 kZero3 = 0x01164B40;      // zero vector
constexpr u32 kAxis3 = 0x01164B70;      // the vector copied to o+928
constexpr u32 kZero1D1 = 0x001D116C;    // f32 0 (the fov / step thresholds)

// The x/y/z words of the four rows (16 bytes apart) of src to dst.
void Copy43(u32 dst, u32 src) { CopyMat43W(dst, src); }

// The rows at m (+0 / +16 / +32 / +48, all four lanes, lvx) transformed by
// the matrix f: row k = f0 * m[k].x + f1 * m[k].y + f2 * m[k].z (vmaddfp
// chain from 0), the last row plus f3; stored rows 0..3 in order. NaN
// lanes follow the AltiVec operand priority (Row3N26 / Add4N26).
void XformRows(u32 m, u32 f) {
    const V4 c0 = Lvx(f), c1 = Lvx(f + 16), c2 = Lvx(f + 32), c3 = Lvx(f + 48);
    V4 r[4];
    for (u32 k = 0; k < 4; ++k) {
        const V4 v = Lvx(m + 16 * k);
        r[k] = Row3N26(c0, c1, c2, v.e[0], v.e[1], v.e[2]);
    }
    r[3] = Add4N26(c3, r[3]);
    for (u32 k = 0; k < 4; ++k) Stvx(m + 16 * k, r[k]);
}

// bone = Obj_FindBone(o, key) into o+bo; none: o+fo = Obj_FindByKey(o,
// key) (key re-read), else o+eo = Obj_Entry16(o, (s16) bone->0->+100).
void BindKey(PPUContext& ctx, u32 o, u32 kp, u32 bo, u32 fo, u32 eo) {
    const u32 key = load<u32>(kp);
    if (key == 0) return;
    const u32 b = static_cast<u32>(call(ctx, Obj_FindBone, {o, key}));
    store<u32>(o + bo, b);
    if (b == 0) {
        store<u32>(o + fo, static_cast<u32>(call(ctx, Obj_FindByKey, {o, load<u32>(kp)})));
    } else {
        const s32 idx = static_cast<s16>(load<u16>(load<u32>(b) + 100));
        store<u32>(o + eo, static_cast<u32>(call(ctx, Obj_Entry16, {o, static_cast<u64>(static_cast<s64>(idx))})));
    }
}
}  // namespace

// u32 f(PathObj* o, const Rec* d) (called by Spawner1D1B18, 001D2EA8).
// 1. Owner_Construct1ADD08, the vtables, the 28-byte record vector at
//    o+556 (+562 = 28, PtrVector_Init(16, 0, 1)); identity frames at
//    o+576 / +640 / +720; d->+260 / +264 to o+704 / +708; zero vectors at
//    o+832 / +848 / +864 / +912, 0x01164B70 at o+928, +944 = 1.0, zeroed
//    words 784..820 / 880..904; d->+276..+296 to o+948..+968; the flags
//    +972: bit 31 from d->+300 bit 31, bit 30 from its bit 31, bit 29 from
//    its bit 30 (three stores), bit 28 clear, bit 27 set, bits 26 / 25
//    clear. Without o->+216 returns the flags word (f1: d->+288).
// 2. o->+816 / +820 from CallK2_000D956C / CallK2_000D94F4; with a camera
//    d->+268 > 0 sets its fov and d->+272 > 0 steps it.
// 3. d's +240 records (+248 u16 count, 28 bytes each) are pushed onto
//    o+556 (capacity doubled when full; +566 the high-water count).
// 4. The keys (o->+216)->+400 / +404 bind bones o+792 / +804 (entries
//    +788 / +800) or objects o+784 / +796.
// 5. The frame o+640 (and its copy o+576) from o->+160 + 80 (else o+96),
//    replaced by bone 804's matrix times the body frame or object 796's
//    +80; the frame o+720 from bone 792's matrix times the body frame or
//    object 784's +80. Then Obj_ResetPath1D0F68; returns what it leaves.
DECOMP(001D1170, PathObj_Ctor_001D1170) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, Owner_Construct, {o, d});
    const u32 vec = o + 556;
    store<u32>(o + 0, kVt1D1 + 8);
    store<u32>(o + 380, kVt1D1 + 476);
    store<u32>(o + 384, kVt1D1 + 516);
    call(ctx, PtrVector_Construct, {vec});
    store<u16>(o + 562, 28);
    call(ctx, PtrVector_Init, {vec, 16, 0, 1});
    Copy43(o + 576, kIdentity);
    Copy43(o + 640, kIdentity);
    // o+720.. interleaved with o+704 / +708 in the original (no overlap)
    Copy43(o + 720, kIdentity);
    store<u32>(o + 704, load<u32>(d + 260));
    store<u32>(o + 708, load<u32>(d + 264));
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 832 + 4 * k, load<u32>(kZero3 + 4 * k));
    for (u32 a : {784u, 788u, 792u, 796u, 800u, 804u, 816u, 820u}) store<u32>(o + a, 0);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 848 + 4 * k, load<u32>(kZero3 + 4 * k));
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 864 + 4 * k, load<u32>(kZero3 + 4 * k));
    for (u32 a : {880u, 884u, 888u, 896u, 900u, 904u}) store<u32>(o + a, 0);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 912 + 4 * k, load<u32>(kZero3 + 4 * k));
    store<u32>(o + 892, 0);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 928 + 4 * k, load<u32>(kAxis3 + 4 * k));
    store<u32>(o + 944, 0x3F800000u);
    for (u32 k = 0; k < 6; ++k) store<u32>(o + 948 + 4 * k, load<u32>(d + 276 + 4 * k));
    u32 fl = (load<u32>(o + 972) & 0x7FFFFFFFu) | (load<u32>(d + 300) & 0x80000000u);
    store<u32>(o + 972, fl);
    fl = (fl & ~0x40000000u) | ((load<u32>(d + 300) >> 1) & 0x40000000u);
    store<u32>(o + 972, fl);
    fl = (fl & ~0x20000000u) | ((load<u32>(d + 300) >> 1) & 0x20000000u);
    fl = ((fl & ~0x10000000u) | 0x08000000u) & ~0x06000000u;
    store<u32>(o + 972, fl);
    const u32 def = load<u32>(o + 216);
    if (def == 0) {
        ctx.gpr[3] = fl;
        ctx.fpr[1] = Lf(d + 288);  // the last lfs of the copies
        return;
    }

    // 2. Camera / controller.
    store<u32>(o + 816, static_cast<u32>(call(ctx, CallK2_D956C, {o})));
    store<u32>(o + 820, static_cast<u32>(call(ctx, CallK2_D94F4, {o})));
    const f64 zero = Lf(kZero1D1);
    if (u32 cam = load<u32>(o + 816); cam != 0) {
        if (Lf(d + 268) > zero) {
            callf(ctx, Cam_SetFov, {cam}, {Lf(d + 268)});
            cam = load<u32>(o + 816);
        }
        if (Lf(d + 272) > zero) callf(ctx, Obj_Step, {cam}, {Lf(d + 272)});
    }

    // 3. The records.
    for (u32 i = 0; i < load<u16>(d + 248); ++i) {
        const u32 src = load<u32>(d + 240) + 28 * i;
        u32 n = load<u16>(o + 564);
        const u32 cap = load<u16>(o + 560);
        if (n == cap) {
            call(ctx, PtrVector_Reserve, {vec, (cap << 1) & 0x1FFFEu});
            n = load<u16>(o + 564);
        }
        const u32 dst = load<u32>(vec) + n * 28;
        for (u32 k = 0; k < 7; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
        const u32 n1 = static_cast<u32>(load<u16>(o + 564)) + 1;
        const s32 hw = load<u16>(o + 566);
        store<u16>(o + 564, static_cast<u16>(n1));
        store<u16>(o + 566, static_cast<u16>(hw > static_cast<s32>(n1 & 0xFFFF) ? static_cast<u32>(hw) : n1));
    }

    // 4. Bones / objects.
    BindKey(ctx, o, def + 400, 792, 784, 788);
    BindKey(ctx, o, def + 404, 804, 796, 800);

    // 5. Frames.
    const u32 body = load<u32>(o + 160);
    Copy43(o + 640, body != 0 ? body + 80 : o + 96);
    f64 f1 = Lf(o + 640);
    if (const u32 b = load<u32>(o + 804); b != 0) {
        const u32 m = load<u32>(b + 8);
        Copy43(o + 640, m);
        ctx.fpr[1] = Lf(m + 56);
        XformRows(o + 640, static_cast<u32>(call(ctx, Field_Get160Body, {o})));
        f1 = Lf(o + 640);
    } else if (const u32 p = load<u32>(o + 796); p != 0) {
        Copy43(o + 640, p + 80);
        f1 = Lf(o + 640);
    }
    Copy43(o + 576, o + 640);
    ctx.fpr[1] = f1;
    if (const u32 b = load<u32>(o + 792); b != 0) {
        const u32 m = load<u32>(b + 8);
        Copy43(o + 720, m);
        ctx.fpr[1] = Lf(m + 56);
        XformRows(o + 720, static_cast<u32>(call(ctx, Field_Get160Body, {o})));
    } else if (const u32 p = load<u32>(o + 784); p != 0) {
        Copy43(o + 720, p + 80);
        ctx.fpr[1] = Lf(p + 80 + 56);
    }
    call(ctx, Obj_ResetPath, {o});
}
