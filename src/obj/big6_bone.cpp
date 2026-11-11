// Game code, large-function sweep part 6: an object's attachments placed
// at their bones (000D7A00).
#include "big6.h"

using namespace sly4;
using namespace sly4::big6;

namespace {
// Object.
constexpr u32 kObjMat = 96;         // f32 [4][4]
constexpr u32 kObjWorldObj = 160;   // WorldObj* (its position at +144), or 0
constexpr u32 kObjAttach = 340;     // {+0 entries (240 bytes each), +8 u16 count} or 0
// Attachment entry.
constexpr u32 kAtPos = 128;         // f32 [3]: offset
constexpr u32 kAtRot = 144;         // f32 [4]: quaternion
constexpr u32 kAtFlags = 176;       // 0x01000000 off, 0x40000000 keep the object's rotation,
                                    // 0x20000000 keep the child's rotation, 0x10000000 leave its position
constexpr u32 kAtChild = 192;       // the attached object
constexpr u32 kAtParentNode = 196;  // node (bone index u16 at +68, matrix multiplied in) or 0
constexpr u32 kAtParentBone = 200;  // s32 bone of the object, -1: none
constexpr u32 kAtParentRef = 204;   // an object whose matrix (+80) is used instead, or 0
constexpr u32 kAtChildNode = 208;   // as +196, on the child
constexpr u32 kAtChildBone = 212;   // as +200, on the child
constexpr u32 kChildWorldObjFlags = 192;  // child +192: 0x00400000 = place even without a WorldObj

constexpr u32 kPermConst = 0x000D79E0u;  // .text: vperm masks for the transposes (+0 column 1, +16 column 2)
constexpr u32 kIdentity = 0x01164BD0u;
constexpr u32 kZeroPos = 0x01164B40u;
constexpr u32 kZeroVec = 0x01164B30u;

constexpr u32 kQuatToMatrix = 0x00553A90u;   // (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 kBoneMatrix = 0x000CE688u;     // (f32 out[4][4], Object*, s32 bone, u8 world, u8 raw)
constexpr u32 kMatMultiply = 0x005541F0u;    // (Matrix* a, const Matrix* b): a = a x b
constexpr u32 kMatNormalize = 0x0056D374u;   // (f32 m[4][4])
constexpr u32 kObjMoveTo = 0x000D7904u;      // (Object*, const vec3*, u8 recurse)
constexpr u32 kMarkDirty = 0x0057D000u;      // (WorldObj*)
constexpr u32 kVfnSetMatrix = 84;            // child vtable: (child, const f32 m[4][4]) (name?)

// The local matrix L (at sp+144) taken into the space of bone matrix B (at
// bm): B's rotation transposed (the columns built with vmrghw / vperm, so
// their w lanes hold B's leftovers) and its translation inverted; B is
// overwritten with that inverse, L = L x inverse (row 3 + the inverse's
// translation, added in that order).
void InvMulLocal(u32 bm, u32 l, const V4& p1, const V4& p2) {
    const V4 zero{{0.0f, 0.0f, 0.0f, 0.0f}};
    const V4 r0 = Lvx(bm), r1 = Lvx(bm + 16), r2 = Lvx(bm + 32);
    const V4 t = Sub4(zero, Lvx(bm + 48));
    const V4 h = Mrghw(r0, r2), lo = Mrglw(r0, r2);
    const V4 c0 = Mrghw(h, r1), c1 = Perm(h, r1, p1), c2 = Perm(lo, r1, p2);
    const V4 l0 = Lvx(l), l1 = Lvx(l + 16), l2 = Lvx(l + 32), l3 = Lvx(l + 48);
    const V4 tinv = Row3(c0, c1, c2, t.e[0], t.e[1], t.e[2]);
    Stvx(bm, c0);
    Stvx(bm + 16, c1);
    Stvx(bm + 32, c2);
    Stvx(bm + 48, tinv);
    Stvx(l, Row3(c0, c1, c2, l0.e[0], l0.e[1], l0.e[2]));
    Stvx(l + 16, Row3(c0, c1, c2, l1.e[0], l1.e[1], l1.e[2]));
    Stvx(l + 32, Row3(c0, c1, c2, l2.e[0], l2.e[1], l2.e[2]));
    Stvx(l + 48, Add4(tinv, Row3(c0, c1, c2, l3.e[0], l3.e[1], l3.e[2])));
}

// out = L x M (rows; row 3 = M3 + ..., in that order).
void MulLM(u32 out, u32 l, u32 m) {
    const V4 l0 = Lvx(l), l1 = Lvx(l + 16), l2 = Lvx(l + 32), l3 = Lvx(l + 48);
    const V4 m0 = Lvx(m), m1 = Lvx(m + 16), m2 = Lvx(m + 32), m3 = Lvx(m + 48);
    const V4 o0 = Row3(m0, m1, m2, l0.e[0], l0.e[1], l0.e[2]);
    const V4 o1 = Row3(m0, m1, m2, l1.e[0], l1.e[1], l1.e[2]);
    const V4 o2 = Row3(m0, m1, m2, l2.e[0], l2.e[1], l2.e[2]);
    const V4 o3 = Add4(m3, Row3(m0, m1, m2, l3.e[0], l3.e[1], l3.e[2]));
    Stvx(out, o0);
    Stvx(out + 16, o1);
    Stvx(out + 32, o2);
    Stvx(out + 48, o3);
}

// Rows 0..2 (x, y, z words) of a 4x4 matrix, raw.
void CopyRot3W(u32 dst, u32 src) {
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(dst + 16 * r + 4 * k, src + 16 * r + 4 * k);
}
}  // namespace

// void Obj_PlaceAttachments(Object* o)
// For each attachment entry (+340) with a child that is not switched off
// (and that has a WorldObj, or child +192 has 0x00400000): L = the entry's
// rotation and offset; M = the object's matrix, or its bone matrix (+200),
// or the matrix of +204, or a node's bone matrix times the node (+196);
// the child's own bone / node matrix (+208 / +212) is taken out of L; the
// result L x M (rotation rows 0..2 from the object / child per flags) is
// normalised and handed to the child's vfn[84]; unless flag 0x10000000 the
// child's WorldObj is moved to the object's WorldObj position when they
// differ; a child WorldObj (+160) gets the matrix and is marked dirty.
// The 704-byte frame is the original's, so the w lanes Quat_ToMatrix and
// the copies leave alone hold the same stack contents. r3 at exit: the
// entry count (0 without a list); f1 / f2: what the last float loads left.
DECOMP(000D7A00, Obj_PlaceAttachments) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 704);
    const u32 sp = frame.addr();
    const u32 vec = sp + 112, vec2 = sp + 128, lm = sp + 144, mm = sp + 208, res = sp + 272;
    const V4 p1 = Lvx(kPermConst), p2 = Lvx(kPermConst + 16);
    for (u32 i = 0;; ++i) {
        const u32 lst = load<u32>(o + kObjAttach);
        const u32 n = lst != 0 ? load<u16>(lst + 8) : 0;
        if (i >= n) {
            ctx.gpr[3] = n;
            return;
        }
        const u32 e = load<u32>(lst) + 240 * i;
        const u32 c = load<u32>(e + kAtChild);
        if (c == 0 || (load<u32>(e + kAtFlags) & 0x01000000u) != 0) continue;
        if (load<u32>(c + kObjWorldObj) == 0 && (load<u32>(c + kChildWorldObjFlags) & 0x00400000u) == 0) continue;
        call(ctx, kQuatToMatrix, {lm, e + kAtRot, kZeroPos});
        for (u32 k = 0; k < 3; ++k) CopyW(lm + 48 + 4 * k, e + kAtPos + 4 * k);
        CopyMat43W(mm, o + kObjMat);
        SetF1FromWord(ctx, o + kObjMat + 56);
        // M: the object's matrix, bone or node.
        if (const u32 node = load<u32>(e + kAtParentNode); node != 0) {
            const u32 b = sp + 336;
            call(ctx, kBoneMatrix, {b, o, static_cast<u64>(static_cast<s64>(static_cast<s16>(load<u16>(node + 68)))), 0, 1});
            call(ctx, kMatMultiply, {b, load<u32>(e + kAtParentNode)});
            call(ctx, kMatMultiply, {mm, b});
        } else if (const s32 bone = load<s32>(e + kAtParentBone); bone != -1) {
            const u32 b = sp + 400;
            call(ctx, kBoneMatrix, {b, o, static_cast<u64>(static_cast<s64>(bone)), 1, 1});
            CopyMat43W(mm, b);
            SetFFromWord(ctx, 1, b + 52);
            SetFFromWord(ctx, 2, b + 56);
        } else if (const u32 ref = load<u32>(e + kAtParentRef); ref != 0) {
            CopyMat43W(mm, ref + 80);
            SetF1FromWord(ctx, ref + 80 + 56);
        }
        if ((load<u32>(e + kAtFlags) & 0x40000000u) != 0) {
            CopyRot3W(mm, o + kObjMat);
            SetF1FromWord(ctx, o + kObjMat + 40);
        }
        // The child's own bone / node taken out of L.
        if (const u32 node = load<u32>(e + kAtChildNode); node != 0) {
            const u32 b = sp + 464;
            call(ctx, kBoneMatrix, {b, c, static_cast<u64>(static_cast<s64>(static_cast<s16>(load<u16>(node + 68)))), 0, 1});
            call(ctx, kMatMultiply, {b, load<u32>(e + kAtChildNode)});
            InvMulLocal(b, lm, p1, p2);
        } else if (const s32 bone = load<s32>(e + kAtChildBone); bone != -1) {
            const u32 b = sp + 528;
            call(ctx, kBoneMatrix, {b, c, static_cast<u64>(static_cast<s64>(bone)), 0, 1});
            InvMulLocal(b, lm, p1, p2);
        }
        // (The original first copies the identity's 12 words into res; the
        // product below overwrites all of them.)
        SetFFromWord(ctx, 1, kIdentity + 52);
        SetFFromWord(ctx, 2, kIdentity + 56);
        MulLM(res, lm, mm);
        if ((load<u32>(e + kAtFlags) & 0x20000000u) != 0) {
            CopyRot3W(res, c + kObjMat);
            SetF1FromWord(ctx, c + kObjMat + 40);
        }
        call(ctx, kMatNormalize, {res});
        if ((load<u32>(e + kAtFlags) & 0x10000000u) == 0) {
            const u32 owo = load<u32>(o + kObjWorldObj);
            const u32 a = owo != 0 ? owo + 144 : kZeroVec;
            CopyW(vec, a);
            CopyW(vec + 4, a + 4);
            const u32 az = load<u32>(a + 8);
            store<u32>(vec + 8, az);
            const u32 cwo = load<u32>(c + kObjWorldObj);
            const u32 b = cwo != 0 ? cwo + 144 : kZeroVec;
            CopyW(vec2, b);
            CopyW(vec2 + 4, b + 4);
            SetF1FromWord(ctx, b + 4);
            if (load<u64>(vec) != load<u64>(vec2) || az != load<u32>(b + 8)) call(ctx, kObjMoveTo, {c, vec, 0});
        }
        VCall(ctx, c, kVfnSetMatrix, {c, res});
        if (const u32 wo = load<u32>(c + kObjWorldObj); wo != 0) {
            for (u32 r = 0; r < 4; ++r)
                for (u32 k = 0; k < 3; ++k) CopyW(wo + 16 + 16 * r + 4 * k, res + 16 * r + 4 * k);
            SetF1FromWord(ctx, res + 56);
            call(ctx, kMarkDirty, {wo});
        }
    }
}
