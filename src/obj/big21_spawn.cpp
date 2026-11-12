// Game code, large-function sweep part 21: the layer-2 spawn slot (callee of
// Obj_Layer2TickBE7F0).
#include "big21.h"
#include "big22.h"

namespace sly4::obj {

namespace {
constexpr u32 kQuatId = 0x01164C90u;    // f32[4] the identity quaternion
constexpr u32 kRowsId = 0x01164C50u;    // f32[3][4] identity rows
constexpr u32 kOneVec = 0x01164B30u;    // f32[3] the vector before kZeroVec
constexpr u32 kIdentity = 0x01164BD0u;  // f32[4][4]
constexpr u32 kSlotK = 0x000B61FCu;     // f32 -FLT_MAX, 0.0, -1.0
constexpr u32 kQueryVt = 0x00B12260u;   // the hand-built source query's vtable
constexpr u32 kQueryExVt = 0x00B12070u;
constexpr u32 kEmptyName = 0x00AC637Cu;  // ""
constexpr u32 kTutorialName = 0x00AC63ACu;  // "MineLaserField_Tutorial"
constexpr u32 Obj_AimMatrix = 0x0049FFE0u;
constexpr u32 PtrVector_Construct = 0x005516E0u;
constexpr u32 PtrVector_Clear = 0x0054EC1Cu;
constexpr u32 Nop = 0x0054EC18u;
constexpr u32 Vector_Add = 0x00733964u;
constexpr u32 String_Copy = 0x00553684u;
constexpr u32 Obj_InstanceName = 0x000CBA40u;
constexpr u32 QueryEx_Construct = 0x000DFDF8u;
constexpr u32 Obj_CopyQuery = 0x000CDB28u;
constexpr u32 Mat34_FromRotPos = 0x00553B4C;    // (out, const f32 rows[3][4], const Vec3* pos)

// The spawner slot.
struct SlotSpawner {
    pad<4> _0;
    be_u32 owner;       // +4
    be_u32 flags;       // +8
    pad<40> _12;
    be_u32 pairs;       // +52: {obj, marker}[]
    be_u16 pcap;        // +56
    pad<2> _58;
    be_u16 pcount;      // +60
    be_u16 pmark;       // +62
    pad<8> _64;
    be_u32 objs;        // +72: the object table
    pad<16> _76;
    be_u32 list;        // +92: the free marker indices
    pad<4> _96;
    be_u16 nmarkers;    // +100
    pad<10> _102;
    be_u32 base;        // +112 (+32 a vector, +68 the size)
    be_u32 fixed_src;   // +116
    be_u32 tmpl;        // +120
    be_u32 w124;        // +124
    be_s32 w128;        // +128
    be_u32 w132;        // +132
    pad<4> _136;
    be_u32 w140;        // +140
    be_u32 w144;        // +144
    be_u32 fixed;       // +148
};
static_assert(offsetof(SlotSpawner, pairs) == 52 && offsetof(SlotSpawner, objs) == 72 && offsetof(SlotSpawner, nmarkers) == 100);
static_assert(offsetof(SlotSpawner, base) == 112 && offsetof(SlotSpawner, w140) == 140 && offsetof(SlotSpawner, fixed) == 148);
// The template object: +32 bit 2, +36 / +40 copied, +136 its 192-byte
// records {+0 data, +8 (u16) count}, +244 .. +256 copied, u8 +260 / +261.
struct SlotTmpl {
    pad<32> _0;
    u8 b32;             // +32
    pad<3> _33;
    be_u32 w36;         // +36
    be_u32 w40;         // +40
    pad<92> _44;
    be_u32 recs;        // +136
    pad<104> _140;
    be_u32 w244[4];     // +244 .. +256
    u8 b260, b261;      // +260 / +261
};
static_assert(offsetof(SlotTmpl, recs) == 136 && offsetof(SlotTmpl, w244) == 244 && offsetof(SlotTmpl, b260) == 260);
// The hand-built source query (192 bytes).
struct SrcQuery {
    be_u32 vt;          // +0
    be_u32 w4, w8, w12; // +4 .. +12
    be_u32 vec;         // +16: the record vector
    pad<12> _20;
    Mtx44 m;            // +32
    Vec4 v96;           // +96
    be_u32 w112;        // +112
    be_u32 w116;        // +116
    be_u32 tmpl;        // +120
    be_u32 w124;        // +124
    be_u32 w128;        // +128
    be_u32 flags;       // +132
    be_u32 w136;        // +136
    u8 name[52];        // +140
};
static_assert(sizeof(SrcQuery) == 192 && offsetof(SrcQuery, flags) == 132);
// The QueryEx (400 bytes).
struct QueryEx {
    be_u32 w0;          // +0: a vector (freed at the end)
    be_u32 flags;       // +4
    pad<136> _8;
    pad<20> vec144;     // +144
    pad<44> vec164;     // +164
    Vec4 pos;           // +208
    Vec4 rot;           // +224
    be_u32 w240[4];     // +240 .. +252
    pad<144> _256;
};
static_assert(sizeof(QueryEx) == 400 && offsetof(QueryEx, pos) == 208 && offsetof(QueryEx, w240) == 240);
// The original's locals (sp offsets - 208).
struct SlotLocals {
    Mtx44 aim;          // sp+208 (+256 the position)
    Mtx44 frame;        // sp+272 (+320 the position)
    SrcQuery q1;        // sp+336
    QueryEx q2;         // sp+528
};
static_assert(offsetof(SlotLocals, q1) == 128 && offsetof(SlotLocals, q2) == 320);

f64 C(u32 off) { return lf(global<be_f32>(kSlotK + off)); }

// One step of the game's generator; returns the old seed.
u32 RandStep() {
    be_u32& state = global<be_u32>(kGameRandState);
    const u32 seed = state;
    const u32 mix = GameRandMix(seed);
    state = std::rotl(mix, 16);
    global<be_u64>(kGameRandLast) = static_cast<u64>(mix & 0xFFFFu);
    return seed;
}

// The 20-byte PtrVector at *vp (when made) cleared and freed.
void FreeVec(be_u32& vp) {
    if (const u32 v = vp; v != 0) {
        call(PtrVector_Clear, v);
        call(Nop, v);
        call(fn::SmallFree, vp.get(), 20u);
    }
}
}  // namespace

// void Obj_SlotApply_000B6208(Spawner* x, s32 lo, s32 hi) (no caller reads
// r3 or f1): marks x->8 bit 31
// and, with a template object x->120, spawns n objects: n = 1 for a fixed
// slot (x->148 != 0), else lo + rand % (hi - lo + 1) (none when <= 0); the
// free markers (u16 count x->100, index list x->92 into the object table
// x->72) must not be used up (fixed slots excepted). The spawn frame is
// Obj_AimMatrix(x->4, .., x->124, x->128, x->132, x->112 + 32, zero, quat
// id) posed by Mat34_FromRotPos. Per object: a marker (the one farthest
// from the local player in x/z, or a random one when no distance beats
// -FLT_MAX) is taken out of the list (swapped with the last); fixed slots
// use x->116 and no marker. A source query (vtable 0x00B12260, frame rows,
// x->120's +36 / +40 / bit 2 of +32 and its 192-byte records +136) is copied
// with Obj_CopyQuery into a QueryEx (name: x->120's instance name, or
// "MineLaserField_Tutorial" for fixed slots) placed at the frame position,
// rotated like the marker (+128) and sized by x->112->68; the 448-byte
// object built from it (Spawned_Construct_000B2E18, vfn[8]) is pushed with the marker index
// onto x's 8-byte pair vector (+52, u16 count +60 / capacity +56, high-water
// +62). Ends with x->140 = 1 and x->144 = x->112->68.
void Obj_SlotApply_000B6208(SlotSpawner* x, u32 lo, u32 hi) {
    GuestFrame frame(928);
    SlotLocals& L = *frame.as<SlotLocals>(208);
    x->flags = x->flags | 0x80000000u;
    const u32 tmpl = x->tmpl;
    const u32 markers0 = x->nmarkers;
    const u32 base = x->base;
    if (tmpl == 0) return;
    s32 n = 1;
    if (x->fixed == 0u) {
        const u32 span = hi - lo;
        const u32 seed = RandStep();
        const u32 q = static_cast<u32>(rt::divwu(seed, span + 1));
        n = static_cast<s32>(lo + (seed - (q * span + q)));
        if (n <= 0) return;
        if (markers0 == 0) return;
    }
    call(Obj_AimMatrix, x->owner.get(), &L.aim, x->w124.get(), x->w128.get(), x->w132.get(), base + 32, kZeroVec, kQuatId);
    call(Mat34_FromRotPos, &L.frame, kRowsId, &L.aim.r[3]);
    u32 ppos = kZeroVec;
    if (const u32 pl = gm<be_u32>(global<be_u32>(kWorld) + 4208 + 8); pl != 0) ppos = pl + 144;
    const f64 px = lf(gm<be_f32>(ppos)), pz = lf(gm<be_f32>(ppos + 8));
    for (s32 i = 0; i < n; ++i) {
        u32 marker = 0, src;
        if (x->fixed != 0u) {
            src = x->fixed_src;
        } else {
            const f64 y2 = S(C(4) * C(4));
            f64 best = C(0);
            u32 pick = ~0u, cnt, k = 0;
            for (;; ++k) {
                cnt = x->nmarkers;
                if (k >= cnt) break;
                const u32 o = gm<be_u32>(x->objs + (gm<be_u32>(x->list + 4 * k) << 2));
                const f64 dx = S(px - lf(gm<be_f32>(o + 128))), dz = S(pz - lf(gm<be_f32>(o + 136)));
                const f64 d = Fmadds(dz, dz, Fmadds(dx, dx, y2));
                if (best < d) {
                    pick = k;
                    best = d;
                }
            }
            if (pick == ~0u) {
                const u32 seed = RandStep();
                pick = seed - static_cast<u32>(rt::divwu(seed, cnt)) * cnt;
            }
            const u32 list = x->list;
            const u32 last = (cnt - 1) & 0xFFFFu;
            marker = gm<be_u32>(list + 4 * pick);
            src = gm<be_u32>(x->objs + (marker << 2));
            x->nmarkers = static_cast<u16>(last);
            if (static_cast<s32>(pick) != static_cast<s32>(last)) gm<be_u32>(list + 4 * pick) = gm<be_u32>(list + 4 * last).get();
        }
        // ---- the source query q1
        SrcQuery& q1 = L.q1;
        const u32 o = x->tmpl;
        q1.vt = kQueryVt;
        q1.vec = 0u;
        CopyMtxXYZ(q1.m, L.frame);
        CopyXYZ(q1.v96, global<Vec3>(kOneVec));
        q1.w112 = 0u;
        q1.w116 = 0u;
        q1.tmpl = o;
        q1.w124 = gm<be_u32>(o != 0 ? o + 36 : kSlotK + 8).get();
        q1.w128 = gm<be_u32>(o != 0 ? o + 40 : kSlotK + 8).get();
        const u32 fl = (q1.flags | 0x80000000u) & ~0x40000000u;
        u32 bit = 0;
        if (o != 0) {
            q1.flags = fl;
            bit = (gm<SlotTmpl>(o).b32 >> 2) & 1u;
        }
        q1.w136 = 0u;
        q1.w4 = 0u;
        q1.w8 = 0u;
        q1.flags = ((fl & ~0x20000000u) | (bit << 29)) | 0x1FE00000u;
        q1.w12 = 0u;
        q1.name[0] = 0;
        call(String_Copy, &q1.name[0], kEmptyName, 40u);
        // the template's 192-byte records, appended to q1's lazily made vector
        if (o != 0 && gm<SlotTmpl>(o).recs != 0u && gm<be_u16>(gm<SlotTmpl>(o).recs + 8) != 0) {
            u32 recs = gm<SlotTmpl>(o).recs, off = 0;
            for (u32 j = 0;; ++j) {
                const u32 cnt = recs != 0 ? gm<be_u16>(recs + 8).get() : 0u;
                if (j >= cnt) break;
                const u32 rec = gm<be_u32>(recs) + off;
                u32 v = q1.vec;
                if (v == 0) {
                    v = call<u32>(fn::SmallAlloc, 20u);
                    q1.vec = v;
                    if (v != 0) {
                        call(PtrVector_Construct, v);
                        gm<be_u16>(v + 6) = 192;
                        v = q1.vec;
                    }
                }
                call(Vector_Add, v, rec);
                off += 192;
                recs = gm<SlotTmpl>(o).recs;
            }
        }
        const u32 name = x->fixed != 0u ? kTutorialName : call<u32>(Obj_InstanceName, x->tmpl.get());
        // ---- the QueryEx q2
        QueryEx& q2 = L.q2;
        const u32 t = x->tmpl;
        const u32 a260 = t != 0 ? gm<SlotTmpl>(t).b260 : 0u, a261 = t != 0 ? gm<SlotTmpl>(t).b261 : 0u;
        call(QueryEx_Construct, &q2, 4u, 128u, name, t, 0u, kQueryExVt, kIdentity, kOneVec, a260, a261, 0u, 1u, 0u, 1u, 0u, 0u, 0u,
             0xFFFFFFFFu);
        CopyXYZ(q2.pos, global<Vec3>(kZeroVec));
        CopyXYZ(q2.rot, global<Vec3>(kZeroVec));
        const u32 q2f = q2.flags;
        q2.w240[0] = gm<be_u32>(t != 0 ? t + 244 : kSlotK + 4).get();
        q2.w240[1] = gm<be_u32>(t != 0 ? t + 248 : kSlotK + 4).get();
        q2.w240[2] = gm<be_u32>(t != 0 ? t + 252 : kSlotK + 4).get();
        q2.w240[3] = gm<be_u32>(t != 0 ? t + 256 : kSlotK + 8).get();
        q2.flags = q2f | 0x00200000u;
        call(Obj_CopyQuery, &q1, &q2);
        CopyXYZ(q2.pos, L.frame.r[3]);
        CopyXYZ(q2.rot, gm<Vec3>(src != 0 ? src + 128 : kZeroVec));
        q2.w240[0] = gm<be_u32>(x->base + 68).get();
        u32 obj = 0;
        if (const u32 p = call<u32>(fn::SmallAlloc, 448u); p != 0) {
            Spawned_Construct_000B2E18(&gm<SpawnedObj>(p), &gm<SpawnQuery>(guest(&q2)));
            obj = p;
        }
        vcall(&gm<u8>(obj), 8);  // the original calls through a null object too
        u32 cnt = x->pcount;
        if (cnt == x->pcap) {
            call(fn::PtrVector_Reserve, &x->pairs, (static_cast<u32>(x->pcap) << 1) & 0x1FFFEu);
            cnt = x->pcount;
        }
        const u32 slot = x->pairs + ((cnt << 3) & 0x7FFF8u);
        gm<be_u32>(slot) = obj;
        gm<be_u32>(slot + 4) = marker;
        const u32 nc = x->pcount + 1;
        const u32 hw = x->pmark;
        x->pcount = static_cast<u16>(nc);
        x->pmark = static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(nc & 0xFFFFu) ? hw : nc);
        // q2's record vectors and PtrVector, then q1's
        call(PtrVector_Clear, &q2.vec164);
        call(Nop, &q2.vec164);
        call(PtrVector_Clear, &q2.vec144);
        call(Nop, &q2.vec144);
        FreeVec(q2.w0);
        q1.vt = kQueryVt;
        FreeVec(q1.vec);
    }
    const u32 b = x->base;
    x->w140 = 1u;
    x->w144 = gm<be_u32>(b + 68).get();
}

}  // namespace sly4::obj

NATIVE(000B6208, sly4::obj::Obj_SlotApply_000B6208)
