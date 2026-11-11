// Game code, large-function sweep part 5: the bone matrix of the ribbon
// effect (kind 0xEB2F2138) (002EFBF0).
#include "big5.h"

using namespace sly4;
using namespace sly4::big5;

namespace {
constexpr u32 kIdentity = 0x01164BD0u;  // f32[16]: the identity matrix
constexpr u32 kObjSkelOwner = 212;      // ObjEB2F: the object whose vfn[28] gives the skeleton pose
constexpr u32 kObjFlags = 236;          // u16: 0x8000 = the bone matrix is relative to the parent
constexpr u32 kSkelMatrices = 40;       // pose: f32[16] per bone
constexpr u32 kWobjMatrix = 80;         // WorldObj: its matrix
}  // namespace

// void Obj_BoneMatrix(f32 out[16], ObjEB2F* o, const f32 parent[16],
//                     WorldObj* wo, s32 bone)
// The matrix of one end of the ribbon. out's x/y/z words start as the
// identity; then:
//  - with a world object: its matrix, and with flag 0x8000 that times the
//    inverse of the (rigid) parent: rows * transpose(parent 3x3), row 3
//    also minus the parent's translation the same way;
//  - else with bone < 0: the parent's matrix, unless flag 0x8000 (then
//    the identity stays);
//  - else: bone `bone` of o->212's pose (vfn[28] -> +40), and without
//    flag 0x8000 that times the parent (row 3 plus the parent's translation).
// The VMX steps run lane by lane over all four lanes, so the w words of
// out get what the original's vmaddfp / vaddfp produce from the w words
// already there. r3 / f1 at exit are what the original leaves (the flag
// test, an address, and the last word copied with lfs).
DECOMP(002EFBF0, Obj_BoneMatrix) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), parent = arg<u32>(ctx, 2);
    const u32 wo = arg<u32>(ctx, 3);
    const u64 bone = ctx.gpr[7];
    CopyMat43W(out, kIdentity);
    u32 last = load<u32>(kIdentity + 56);  // the last word moved through f1
    u32 r3;
    if (wo != 0) {
        CopyMat43W(out, wo + kWobjMatrix);
        last = load<u32>(wo + kWobjMatrix + 56);
        r3 = 0;
        if ((load<u16>(o + kObjFlags) & 0x8000u) != 0) {
            // Transpose of the parent's 3x3 (vmrghw / vmrglw / vperm with
            // the masks at 0x002EFBD0 / 0x002EFBE0): columns with these w lanes.
            const V4 p0 = Lvx(parent), p1 = Lvx(parent + 16), p2 = Lvx(parent + 32), p3 = Lvx(parent + 48);
            const V4 cx = {{p0.e[0], p1.e[0], p2.e[0], p1.e[1]}};
            const V4 cy = {{p0.e[1], p1.e[1], p2.e[1], p0.e[0]}};
            const V4 cz = {{p0.e[2], p1.e[2], p2.e[2], p0.e[2]}};
            const f32 nx = 0.0f - p3.e[0], ny = 0.0f - p3.e[1], nz = 0.0f - p3.e[2];  // vsubfp 0 - t
            const V4 o0 = Lvx(out), o1 = Lvx(out + 16), o2 = Lvx(out + 32), o3 = Lvx(out + 48);
            const V4 r3v = Add4(Row3(cx, cy, cz, nx, ny, nz), Row3(cx, cy, cz, o3.e[0], o3.e[1], o3.e[2]));
            Stvx(out, Row3(cx, cy, cz, o0.e[0], o0.e[1], o0.e[2]));
            Stvx(out + 16, Row3(cx, cy, cz, o1.e[0], o1.e[1], o1.e[2]));
            Stvx(out + 32, Row3(cx, cy, cz, o2.e[0], o2.e[1], o2.e[2]));
            Stvx(out + 48, r3v);
            r3 = 0x002EFBD0u;  // the second permute mask's address
        }
    } else if (static_cast<s32>(bone) < 0) {
        if ((load<u16>(o + kObjFlags) & 0x8000u) != 0) {
            r3 = 1;
        } else {
            r3 = 0;
            CopyMat43W(out, parent);
            last = load<u32>(parent + 56);
        }
    } else {
        const u32 owner = load<u32>(o + kObjSkelOwner);
        const u32 pose = static_cast<u32>(VCall(ctx, owner, 28, {owner}));
        const u32 m = load<u32>(pose + kSkelMatrices) + static_cast<u32>(bone << 6);
        CopyMat43W(out, m);
        last = load<u32>(m + 56);
        r3 = 1;
        if ((load<u16>(o + kObjFlags) & 0x8000u) == 0) {
            const V4 p0 = Lvx(parent), p1 = Lvx(parent + 16), p2 = Lvx(parent + 32), p3 = Lvx(parent + 48);
            const V4 o0 = Lvx(out), o1 = Lvx(out + 16), o2 = Lvx(out + 32), o3 = Lvx(out + 48);
            const V4 r0v = Row3(p0, p1, p2, o0.e[0], o0.e[1], o0.e[2]);
            const V4 r1v = Row3(p0, p1, p2, o1.e[0], o1.e[1], o1.e[2]);
            const V4 r2v = Row3(p0, p1, p2, o2.e[0], o2.e[1], o2.e[2]);
            const V4 r3v = Add4(p3, Row3(p0, p1, p2, o3.e[0], o3.e[1], o3.e[2]));
            Stvx(out, r0v);
            Stvx(out + 16, r1v);
            Stvx(out + 32, r2v);
            Stvx(out + 48, r3v);
            r3 = out + 48;
        }
    }
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(last));
    ctx.gpr[3] = r3;
}
