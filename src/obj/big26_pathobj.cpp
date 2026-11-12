// Game code, large-function sweep part 26: the constructor of the 976-byte
// path object of Spawner1D1B18 (reset by Obj_ResetPath1D0F68).
#include "big26.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 Owner_Construct = 0x001ADD08;     // Owner_Construct1ADD08 (o, rec)
constexpr u32 PtrVector_Construct = 0x005516E0; // (vec)
constexpr u32 PtrVector_Init = 0x00551598;      // (vec, 16, 0, 1)
constexpr u32 CallK2_D956C = 0x000D956C;        // CallK2_000D956C (Obj*) -> the camera thing or 0
constexpr u32 CallK2_D94F4 = 0x000D94F4;        // CallK2_000D94F4 (Obj*) -> controller or 0
constexpr u32 Cam_SetFov = 0x0028BC28;          // Cam_SetFov_0028BC28 (cam; f1)
constexpr u32 Obj_Step = 0x0028A590;            // Obj_Step_0028A590 (cam; f1)
constexpr u32 Obj_Entry16 = 0x000DA954;         // Obj_Entry16_000DA954 (Obj*, s32 index)
constexpr u32 Obj_FindByKey = 0x000D1454;       // Obj_FindByKey_000D1454 (Obj*, u32 key)
constexpr u32 Field_Get160Body = 0x000DD880;    // (Obj*) -> f32 m[4][4]
constexpr u32 Obj_ResetPath = 0x001D0F68;       // Obj_ResetPath1D0F68 (Obj*)

constexpr u32 kVt1D1 = 0x00B174B8;      // vtable block: +8 at o+0, +476 at o+380, +516 at o+384
constexpr u32 kIdentity = 0x01164BD0;   // identity matrix
constexpr u32 kZero3 = 0x01164B40;      // zero vector
constexpr u32 kAxis3 = 0x01164B70;      // the vector copied to o+928
constexpr u32 kZero1D1 = 0x001D116C;    // f32 0 (the fov / step thresholds)

// The 28-byte record vector: data, u16 capacity, element size, count,
// high-water mark.
struct RecVec28 {
    be_u32 data;   // +0
    be_u16 cap;    // +4
    be_u16 esize;  // +6
    be_u16 count;  // +8
    be_u16 mark;   // +10
};
// The description record.
struct PathDef {
    pad<240> _0;
    be_u32 recs;       // +240: 28-byte records
    pad<4> _244;
    be_u16 nrecs;      // +248
    pad<10> _250;
    be_u32 w260;       // +260
    be_u32 w264;       // +264
    be_f32 fov;        // +268
    be_f32 step;       // +272
    be_u32 w276[6];    // +276 .. +296
    be_u32 flags;      // +300
};
static_assert(offsetof(PathDef, recs) == 240 && offsetof(PathDef, fov) == 268 && offsetof(PathDef, flags) == 300);
struct PathObj976 {
    be_u32 vt0;          // +0
    pad<92> _4;
    Mtx44 m;             // +96
    be_u32 body;         // +160: its +80 a frame
    pad<52> _164;
    be_u32 def;          // +216: +400 / +404 the bone / object keys
    pad<160> _220;
    be_u32 vt380;        // +380
    be_u32 vt384;        // +384
    pad<168> _388;
    RecVec28 recs;       // +556
    pad<8> _568;
    Mtx44 f576;          // +576
    Mtx44 f640;          // +640
    be_u32 w704;         // +704
    be_u32 w708;         // +708
    pad<8> _712;
    Mtx44 f720;          // +720
    be_u32 objA;         // +784
    be_u32 entA;         // +788
    be_u32 boneA;        // +792
    be_u32 objB;         // +796
    be_u32 entB;         // +800
    be_u32 boneB;        // +804
    pad<8> _808;
    be_u32 cam;          // +816
    be_u32 ctl;          // +820
    pad<8> _824;
    Vec3 v832;           // +832
    pad<4> _844;
    Vec3 v848;           // +848
    pad<4> _860;
    Vec3 v864;           // +864
    pad<4> _876;
    be_u32 w880[7];      // +880 .. +904
    pad<4> _908;
    Vec3 v912;           // +912
    pad<4> _924;
    Vec3 v928;           // +928
    pad<4> _940;
    be_u32 w944;         // +944
    be_u32 w948[6];      // +948 .. +968
    be_u32 flags;        // +972
};
static_assert(sizeof(PathObj976) == 976 && offsetof(PathObj976, recs) == 556 && offsetof(PathObj976, f720) == 720);
static_assert(offsetof(PathObj976, objA) == 784 && offsetof(PathObj976, cam) == 816 && offsetof(PathObj976, v832) == 832);
static_assert(offsetof(PathObj976, w880) == 880 && offsetof(PathObj976, v928) == 928 && offsetof(PathObj976, flags) == 972);

// The rows of m (all four lanes, lvx) transformed by the matrix f: row k =
// f0 * m[k].x + f1 * m[k].y + f2 * m[k].z (vmaddfp chain from 0), the last
// row plus f3; stored rows 0..3 in order (AltiVec NaN priority).
void XformRows(Mtx44& m, const Mtx44& f) {
    const V4 c0 = Lvx(&f.r[0]), c1 = Lvx(&f.r[1]), c2 = Lvx(&f.r[2]), c3 = Lvx(&f.r[3]);
    V4 r[4];
    for (u32 k = 0; k < 4; ++k) {
        const V4 v = Lvx(&m.r[k]);
        r[k] = Row3N(c0, c1, c2, v.e[0], v.e[1], v.e[2]);
    }
    r[3] = Add4N(c3, r[3]);
    for (u32 k = 0; k < 4; ++k) Stvx(&m.r[k], r[k]);
}

// bone = Obj_FindBone(o, *kp) into bo; none: fo = Obj_FindByKey(o, *kp)
// (re-read), else eo = Obj_Entry16(o, (s16) bone->0->+100).
void BindKey(PathObj976* o, const be_u32& kp, be_u32& bo, be_u32& fo, be_u32& eo) {
    const u32 key = kp;
    if (key == 0) return;
    const u32 b = call<u32>(b26fn::Obj_FindBone, o, key);
    bo = b;
    if (b == 0) {
        fo = call<u32>(Obj_FindByKey, o, kp.get());
    } else {
        const s32 idx = gm<be_s16>(gm<be_u32>(b) + 100);
        eo = call<u32>(Obj_Entry16, o, idx);
    }
}
}  // namespace

// void f(PathObj* o, const Rec* d) (called by Spawner1D1B18, 001D2EA8).
// 1. Owner_Construct1ADD08, the vtables, the 28-byte record vector at
//    o+556 (+562 = 28, PtrVector_Init(16, 0, 1)); identity frames at
//    o+576 / +640 / +720; d->+260 / +264 to o+704 / +708; zero vectors at
//    o+832 / +848 / +864 / +912, 0x01164B70 at o+928, +944 = 1.0, zeroed
//    words 784..820 / 880..904; d->+276..+296 to o+948..+968; the flags
//    +972: bit 31 from d->+300 bit 31, bit 30 from its bit 31, bit 29 from
//    its bit 30 (three stores), bit 28 clear, bit 27 set, bits 26 / 25
//    clear. Nothing more without o->+216.
// 2. o->+816 / +820 from CallK2_000D956C / CallK2_000D94F4; with a camera
//    d->+268 > 0 sets its fov and d->+272 > 0 steps it.
// 3. d's +240 records (+248 u16 count, 28 bytes each) are pushed onto
//    o+556 (capacity doubled when full; +566 the high-water count).
// 4. The keys (o->+216)->+400 / +404 bind bones o+792 / +804 (entries
//    +788 / +800) or objects o+784 / +796.
// 5. The frame o+640 (and its copy o+576) from o->+160 + 80 (else o+96),
//    replaced by bone 804's matrix times the body frame or object 796's
//    +80; the frame o+720 from bone 792's matrix times the body frame or
//    object 784's +80. Then Obj_ResetPath1D0F68. No caller reads r3 or f1 (r3.sh).
void PathObj_Ctor_001D1170(PathObj976* o, const PathDef* d) {
    call(Owner_Construct, o, d);
    o->vt0 = kVt1D1 + 8;
    o->vt380 = kVt1D1 + 476;
    o->vt384 = kVt1D1 + 516;
    call(PtrVector_Construct, &o->recs);
    o->recs.esize = 28;
    call(PtrVector_Init, &o->recs, 16u, 0u, 1u);
    const Mtx44& id = global<Mtx44>(kIdentity);
    CopyMtxXYZ(o->f576, id);
    CopyMtxXYZ(o->f640, id);
    CopyMtxXYZ(o->f720, id);
    o->w704 = d->w260.get();
    o->w708 = d->w264.get();
    const Vec3& zv = global<Vec3>(kZero3);
    CopyXYZ(o->v832, zv);
    for (be_u32* a : {&o->objA, &o->entA, &o->boneA, &o->objB, &o->entB, &o->boneB, &o->cam, &o->ctl}) *a = 0u;
    CopyXYZ(o->v848, zv);
    CopyXYZ(o->v864, zv);
    for (be_u32& w : o->w880) w = 0u;
    CopyXYZ(o->v912, zv);
    CopyXYZ(o->v928, global<Vec3>(kAxis3));
    o->w944 = 0x3F800000u;
    for (u32 k = 0; k < 6; ++k) o->w948[k] = d->w276[k].get();
    u32 fl = (o->flags & 0x7FFFFFFFu) | (d->flags & 0x80000000u);
    o->flags = fl;
    fl = (fl & ~0x40000000u) | ((d->flags >> 1) & 0x40000000u);
    o->flags = fl;
    fl = (fl & ~0x20000000u) | ((d->flags >> 1) & 0x20000000u);
    fl = ((fl & ~0x10000000u) | 0x08000000u) & ~0x06000000u;
    o->flags = fl;
    const u32 def = o->def;
    if (def == 0) return;

    // 2. Camera / controller.
    o->cam = call<u32>(CallK2_D956C, o);
    o->ctl = call<u32>(CallK2_D94F4, o);
    const f64 zero = lf(global<be_f32>(kZero1D1));
    if (u32 cam = o->cam; cam != 0) {
        if (lf(d->fov) > zero) {
            call(Cam_SetFov, cam, lf(d->fov));
            cam = o->cam;
        }
        if (lf(d->step) > zero) call(Obj_Step, cam, lf(d->step));
    }

    // 3. The records.
    for (u32 i = 0; i < d->nrecs; ++i) {
        const u32 src = d->recs + 28 * i;
        u32 n = o->recs.count;
        const u32 cap = o->recs.cap;
        if (n == cap) {
            call(fn::PtrVector_Reserve, &o->recs, (cap << 1) & 0x1FFFEu);
            n = o->recs.count;
        }
        const u32 dst = o->recs.data + n * 28;
        for (u32 k = 0; k < 7; ++k) gm<be_u32>(dst + 4 * k) = gm<be_u32>(src + 4 * k).get();
        const u32 n1 = static_cast<u32>(o->recs.count) + 1;
        const s32 hw = o->recs.mark;
        o->recs.count = static_cast<u16>(n1);
        o->recs.mark = static_cast<u16>(hw > static_cast<s32>(n1 & 0xFFFF) ? static_cast<u32>(hw) : n1);
    }

    // 4. Bones / objects.
    BindKey(o, gm<be_u32>(def + 400), o->boneA, o->objA, o->entA);
    BindKey(o, gm<be_u32>(def + 404), o->boneB, o->objB, o->entB);

    // 5. Frames.
    const u32 body = o->body;
    CopyMtxXYZ(o->f640, body != 0 ? gm<Mtx44>(body + 80) : o->m);
    if (const u32 b = o->boneB; b != 0) {
        CopyMtxXYZ(o->f640, gm<Mtx44>(gm<be_u32>(b + 8)));
        XformRows(o->f640, gm<Mtx44>(call<u32>(Field_Get160Body, o)));
    } else if (const u32 p = o->objB; p != 0) {
        CopyMtxXYZ(o->f640, gm<Mtx44>(p + 80));
    }
    CopyMtxXYZ(o->f576, o->f640);
    if (const u32 b = o->boneA; b != 0) {
        CopyMtxXYZ(o->f720, gm<Mtx44>(gm<be_u32>(b + 8)));
        XformRows(o->f720, gm<Mtx44>(call<u32>(Field_Get160Body, o)));
    } else if (const u32 p = o->objA; p != 0) {
        CopyMtxXYZ(o->f720, gm<Mtx44>(p + 80));
    }
    call(Obj_ResetPath, o);
}

}  // namespace sly4::obj

NATIVE(001D1170, sly4::obj::PathObj_Ctor_001D1170)
