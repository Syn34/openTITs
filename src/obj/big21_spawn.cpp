// Game code, large-function sweep part 21: the layer-2 spawn slot (callee of
// Obj_Layer2TickBE7F0).
#include "big21.h"

using namespace sly4;
using namespace sly4::big21;

namespace {
constexpr u32 kZeroVec = 0x01164B40u;   // f32[3] 0
constexpr u32 kQuatId = 0x01164C90u;    // f32[4] the identity quaternion
constexpr u32 kRowsId = 0x01164C50u;    // f32[3][4] identity rows
constexpr u32 kOneVec = 0x01164B30u;    // f32[3] the vector before kZeroVec
constexpr u32 kIdentity = 0x01164BD0u;  // f32[4][4]
constexpr u32 kSlotK = 0x000B61FCu;     // f32 -FLT_MAX, 0.0, -1.0
constexpr u32 kQueryVt = 0x00B12260u;   // the hand-built source query's vtable
constexpr u32 kQueryExVt = 0x00B12070u;
constexpr u32 kEmptyName = 0x00AC637Cu;  // ""
constexpr u32 kTutorialName = 0x00AC63ACu;  // "MineLaserField_Tutorial"

namespace sfn {
constexpr u32 Obj_AimMatrix = 0x0049FFE0u;
constexpr u32 SmallAlloc = 0x0055238Cu;
constexpr u32 SmallFree = 0x00552420u;
constexpr u32 PtrVector_Construct = 0x005516E0u;
constexpr u32 PtrVector_Reserve = 0x00551718u;
constexpr u32 PtrVector_Clear = 0x0054EC1Cu;
constexpr u32 Nop = 0x0054EC18u;
constexpr u32 Vector_Add = 0x00733964u;
constexpr u32 String_Copy = 0x00553684u;
constexpr u32 Obj_InstanceName = 0x000CBA40u;
constexpr u32 QueryEx_Construct = 0x000DFDF8u;
constexpr u32 Obj_CopyQuery = 0x000CDB28u;
constexpr u32 Spawned_Construct = 0x000B2E18u;  // f_000B2E18(p, QueryEx*): the 448-byte spawned object
}  // namespace sfn

// One step of the game's generator; returns the old seed.
u32 RandStep() {
    const u32 seed = load<u32>(kRand + 8);
    const u32 mix = mid15::RandMix15(seed);
    store<u32>(kRand + 8, std::rotl(mix, 16));
    store<u64>(kRand, mix & 0xFFFFu);
    return seed;
}

// The 20-byte PtrVector at *vp (when made) cleared and freed.
void FreeVec(PPUContext& ctx, u32 vp) {
    if (const u32 v = load<u32>(vp); v != 0) {
        call(ctx, sfn::PtrVector_Clear, {v});
        call(ctx, sfn::Nop, {v});
        call(ctx, sfn::SmallFree, {load<u32>(vp), 20});
    }
}
}  // namespace

// u32 Obj_SlotApply_000B6208(Spawner* x, s32 lo, s32 hi): marks x->8 bit 31
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
// object built from it (0x000B2E18, vfn[8]) is pushed with the marker index
// onto x's 8-byte pair vector (+52, u16 count +60 / capacity +56, high-water
// +62). Ends with x->140 = 1 and x->144 = x->112->68 (r3 = x->112).
// Returns early with r3 = x->4.
DECOMP(000B6208, Obj_SlotApply_000B6208) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 lo = arg<u32>(ctx, 1), hi = arg<u32>(ctx, 2);
    Stack frame(ctx, 928);
    const u32 sp = frame.addr();
    constexpr u32 K = kSlotK;
    store<u32>(x + 8, load<u32>(x + 8) | 0x80000000u);
    const u32 tmpl = load<u32>(x + 120);
    const u32 markers0 = load<u16>(x + 100);
    ctx.gpr[3] = load<u32>(x + 4);
    const u32 base = load<u32>(x + 112);
    if (tmpl == 0) return;
    s32 n = 1;
    const u32 fixedSlot = load<u32>(x + 148);
    if (fixedSlot == 0) {
        const u32 span = hi - lo;
        const u32 seed = RandStep();
        const u32 q = static_cast<u32>(rt::divwu(seed, span + 1));
        n = static_cast<s32>(lo + (seed - (q * span + q)));
        if (n <= 0) return;
        if (markers0 == 0) return;
    }
    call(ctx, sfn::Obj_AimMatrix,
         {load<u32>(x + 4), sp + 208, load<u32>(x + 124), static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(x + 128)))),
          load<u32>(x + 132), base + 32, kZeroVec, kQuatId});
    call(ctx, b21fn::Mat34_FromRotPos, {sp + 272, kRowsId, sp + 256});
    u32 ppos = kZeroVec;
    if (const u32 pl = load<u32>(load<u32>(big20::kWorld) + 4208 + 8); pl != 0) ppos = pl + 144;
    const f64 px = Lf(ppos), pz = Lf(ppos + 8);
    for (s32 i = 0; i < n; ++i) {
        u32 marker = 0, src;
        if (load<u32>(x + 148) != 0) {
            src = load<u32>(x + 116);
        } else {
            const f64 y2 = S(Lf(K + 4) * Lf(K + 4));
            f64 best = Lf(K);
            u32 pick = ~0u, cnt, k = 0;
            for (;; ++k) {
                cnt = load<u16>(x + 100);
                if (k >= cnt) break;
                const u32 o = load<u32>(load<u32>(x + 72) + (load<u32>(load<u32>(x + 92) + 4 * k) << 2));
                const f64 dx = S(px - Lf(o + 128)), dz = S(pz - Lf(o + 136));
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
            const u32 list = load<u32>(x + 92);
            const u32 last = (cnt - 1) & 0xFFFFu;
            marker = load<u32>(list + 4 * pick);
            src = load<u32>(load<u32>(x + 72) + (marker << 2));
            store<u16>(x + 100, static_cast<u16>(last));
            if (static_cast<s32>(pick) != static_cast<s32>(last)) store<u32>(list + 4 * pick, load<u32>(list + 4 * last));
        }
        // ---- the source query q1 at sp+336
        const u32 q1 = sp + 336;
        const u32 o = load<u32>(x + 120);
        store<u32>(q1, kQueryVt);
        store<u32>(q1 + 16, 0);
        for (u32 row = 0; row < 4; ++row)
            for (u32 k = 0; k < 3; ++k) Cw(q1 + 32 + 16 * row + 4 * k, sp + 272 + 16 * row + 4 * k);
        for (u32 k = 0; k < 3; ++k) Cw(q1 + 96 + 4 * k, kOneVec + 4 * k);
        store<u32>(q1 + 112, 0);
        store<u32>(q1 + 116, 0);
        store<u32>(q1 + 120, o);
        Cw(q1 + 124, o != 0 ? o + 36 : K + 8);
        Cw(q1 + 128, o != 0 ? o + 40 : K + 8);
        u32 fl = (load<u32>(q1 + 132) | 0x80000000u) & ~0x40000000u;
        u32 bit = 0;
        if (o != 0) {
            store<u32>(q1 + 132, fl);
            bit = (load<u8>(o + 32) >> 2) & 1u;
        }
        store<u32>(q1 + 136, 0);
        store<u32>(q1 + 4, 0);
        store<u32>(q1 + 8, 0);
        store<u32>(q1 + 132, ((fl & ~0x20000000u) | (bit << 29)) | 0x1FE00000u);
        store<u32>(q1 + 12, 0);
        store<u8>(q1 + 140, 0);
        call(ctx, sfn::String_Copy, {q1 + 140, kEmptyName, 40});
        // the template's 192-byte records, appended to q1's lazily made vector
        if (o != 0 && load<u32>(o + 136) != 0 && load<u16>(load<u32>(o + 136) + 8) != 0) {
            u32 recs = load<u32>(o + 136), off = 0;
            for (u32 j = 0;; ++j) {
                const u32 cnt = recs != 0 ? load<u16>(recs + 8) : 0u;
                if (j >= cnt) break;
                const u32 rec = load<u32>(recs) + off;
                u32 v = load<u32>(q1 + 16);
                if (v == 0) {
                    v = static_cast<u32>(call(ctx, sfn::SmallAlloc, {20}));
                    store<u32>(q1 + 16, v);
                    if (v != 0) {
                        call(ctx, sfn::PtrVector_Construct, {v});
                        store<u16>(v + 6, 192);
                        v = load<u32>(q1 + 16);
                    }
                }
                call(ctx, sfn::Vector_Add, {v, rec});
                off += 192;
                recs = load<u32>(o + 136);
            }
        }
        const u32 name = load<u32>(x + 148) != 0 ? kTutorialName : static_cast<u32>(call(ctx, sfn::Obj_InstanceName, {load<u32>(x + 120)}));
        // ---- the QueryEx q2 at sp+528
        const u32 q2 = sp + 528;
        const u32 t = load<u32>(x + 120);
        const u32 a260 = t != 0 ? load<u8>(t + 260) : 0u, a261 = t != 0 ? load<u8>(t + 261) : 0u;
        call(ctx, sfn::QueryEx_Construct,
             {q2, 4, 128, name, t, 0, kQueryExVt, kIdentity, kOneVec, a260, a261, 0, 1, 0, 1, 0, 0, 0, 0xFFFFFFFFu});
        for (u32 k = 0; k < 3; ++k) {
            Cw(q2 + 208 + 4 * k, kZeroVec + 4 * k);
            Cw(q2 + 224 + 4 * k, kZeroVec + 4 * k);
        }
        const u32 q2f = load<u32>(q2 + 4);
        Cw(q2 + 240, t != 0 ? t + 244 : K + 4);
        Cw(q2 + 244, t != 0 ? t + 248 : K + 4);
        Cw(q2 + 248, t != 0 ? t + 252 : K + 4);
        Cw(q2 + 252, t != 0 ? t + 256 : K + 8);
        store<u32>(q2 + 4, q2f | 0x00200000u);
        ctx.fpr[1] = t != 0 ? Lf(t + 248) : Lf(K + 4);
        call(ctx, sfn::Obj_CopyQuery, {q1, q2});
        for (u32 k = 0; k < 3; ++k) Cw(q2 + 208 + 4 * k, sp + 320 + 4 * k);
        const u32 rot = src != 0 ? src + 128 : kZeroVec;
        for (u32 k = 0; k < 3; ++k) Cw(q2 + 224 + 4 * k, rot + 4 * k);
        Cw(q2 + 240, load<u32>(x + 112) + 68);
        ctx.fpr[1] = Lf(load<u32>(x + 112) + 68);
        u32 obj = 0;
        if (const u32 p = static_cast<u32>(call(ctx, sfn::SmallAlloc, {448})); p != 0) {
            call(ctx, sfn::Spawned_Construct, {p, q2});
            obj = p;
        }
        VCall(ctx, obj, 8, {obj});  // the original calls through a null object too
        u32 cnt = load<u16>(x + 60);
        if (cnt == load<u16>(x + 56)) {
            call(ctx, sfn::PtrVector_Reserve, {x + 52, (static_cast<u32>(load<u16>(x + 56)) << 1) & 0x1FFFEu});
            cnt = load<u16>(x + 60);
        }
        const u32 slot = load<u32>(x + 52) + ((cnt << 3) & 0x7FFF8u);
        store<u32>(slot, obj);
        store<u32>(slot + 4, marker);
        const u32 nc = load<u16>(x + 60) + 1;
        const u32 hw = load<u16>(x + 62);
        store<u16>(x + 60, static_cast<u16>(nc));
        store<u16>(x + 62, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(nc & 0xFFFFu) ? hw : nc));
        // q2's record vectors and PtrVector, then q1's
        call(ctx, sfn::PtrVector_Clear, {q2 + 164});
        call(ctx, sfn::Nop, {q2 + 164});
        call(ctx, sfn::PtrVector_Clear, {q2 + 144});
        call(ctx, sfn::Nop, {q2 + 144});
        FreeVec(ctx, q2);
        store<u32>(q1, kQueryVt);
        FreeVec(ctx, q1 + 16);
    }
    const u32 b = load<u32>(x + 112);
    store<u32>(x + 140, 1);
    Cw(x + 144, b + 68);
    ctx.fpr[1] = Lf(b + 68);
    ctx.gpr[3] = b;
}
