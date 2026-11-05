// World_HandleEvent callees, part 18h: an object's bone matrix.
#include <bit>
#include <cmath>

#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
// Object +96: local matrix; +160 WorldObj (+80 world matrix, vt +28
// skeleton (+40 bone matrices, 64 bytes each)).
constexpr u32 kObjMatrix = 96, kWobjMatrix = 80, kWobjVtSkeleton = 28, kSkelBones = 40, kBoneSize = 64;
constexpr u32 kMatrixNormalize = 0x0056D268;  // (f32 dst[4][4], const f32 src[4][4]) (symbols.txt)

f32 lane(u32 ea) { return std::bit_cast<f32>(load<u32>(ea)); }
}  // namespace

// void Obj_BoneMatrixCE688(f32 out[4][4], Object* o, s32 bone, u8 world,
// u8 raw) (name?): the bone's matrix (bone < 0: none). world: starts from
// the object's matrix (overridden by its WorldObj's) and, after the bone
// copy, takes the bone into world space with the WorldObj matrix
// (normalized first unless raw), as the original's vmaddfp chains.
// Otherwise starts from the identity and returns the bone's local matrix.
// The 3x4 copies leave each row's w as it was; the vector stores write all
// 16 bytes (out is assumed 16-byte aligned, as stvx).
DECOMP(000CE688, Obj_BoneMatrixCE688) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 bone = arg<s32>(ctx, 2);
    const u32 world = arg<u8>(ctx, 3), raw = arg<u8>(ctx, 4);
    hevent13::copy_mat(ctx, out, world != 0 ? o + kObjMatrix : hevent13::kIdentity);
    u32 w = load<u32>(o + hevent10::kObjWorldObj);
    if (w != 0 && world != 0) {
        hevent13::copy_mat(ctx, out, w + kWobjMatrix);
        w = load<u32>(o + hevent10::kObjWorldObj);
    }
    if (w == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 skel = static_cast<u32>(vcall(ctx, w, kWobjVtSkeleton, {w}));
    if (skel == 0 || bone < 0) return;
    hevent13::copy_mat(ctx, out, load<u32>(skel + kSkelBones) + static_cast<u32>(bone) * kBoneSize);
    if (world == 0) return;
    Stack frame(ctx, 64);  // the normalized matrix (r1 +112 in the original frame)
    u32 t;
    if (raw == 0) {
        call(ctx, kMatrixNormalize, {frame.addr(), load<u32>(o + hevent10::kObjWorldObj) + kWobjMatrix});
        t = frame.addr();
    } else {
        t = (load<u32>(o + hevent10::kObjWorldObj) + kWobjMatrix) & ~15u;  // lvx
    }
    const u32 base = out & ~15u;
    f32 m[4][4], r[4][4];
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) m[i][k] = lane(base + 16 * i + 4 * k);
    for (u32 i = 0; i < 4; ++i) {
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fma(lane(t + 4 * k), m[i][0], 0.0f);
            v = std::fma(lane(t + 16 + 4 * k), m[i][1], v);
            v = std::fma(lane(t + 32 + 4 * k), m[i][2], v);
            if (i == 3) v = lane(t + 48 + 4 * k) + v;
            r[i][k] = v;
        }
    }
    // Stored rows 0, 1, 2, 3.
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) store<u32>(base + 16 * i + 4 * k, std::bit_cast<u32>(r[i][k]));
}

namespace {
// Named colours (rgba bytes in the colour table records, 72 bytes apart).
struct NamedColour {
    u32 hash, rgba;
};
constexpr NamedColour kColours[] = {
    {0xB8A505E6u, 0x010B9330}, {0x9B70B6C9u, 0x010B924C}, {0x933D1EE0u, 0x010B8FEC}, {0x88397820u, 0x010B8F08},
    {0x04B05162u, 0x010B90D0}, {0x040EE87Eu, 0x010B9298}, {0x0026B78Du, 0x010B91B4}, {0x00014120u, 0x010B8EBC},
    {0xFB7FA14Du, 0x010B937C}, {0x7A100C61u, 0x010B9200}, {0x72CC3493u, 0x010B8FA0}, {0x632AC793u, 0x010B9168},
    {0x1D8C2817u, 0x010B8F54}, {0xE08BB723u, 0x010B9084}, {0xDF0611F9u, 0x010B911C}, {0xCAC02187u, 0x010B9038},
    {0xC1EED72Cu, 0x010B92E4},
};
}  // namespace

// void World_Colour4883C0(u8 out[4], World* w (unread), u32 name, u32 rgba)
// (name?): the named colour's bytes, or rgba (big-endian bytes) for any
// other name.
DECOMP(004883C0, World_Colour4883C0) {
    const u32 out = arg<u32>(ctx, 0), name = arg<u32>(ctx, 2), rgba = arg<u32>(ctx, 3);
    for (const NamedColour& c : kColours) {
        if (c.hash != name) continue;
        for (u32 k = 0; k < 4; ++k) store<u8>(out + k, load<u8>(c.rgba + k));
        return;
    }
    for (u32 k = 0; k < 4; ++k) store<u8>(out + k, static_cast<u8>(rgba >> (24 - 8 * k)));
}
