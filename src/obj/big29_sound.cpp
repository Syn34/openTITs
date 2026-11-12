// Game code, large-function sweep part 29: an object's sound start by
// kind (plain, at a bone, round-robin bone slots, through the object's
// sound owner).
#include "../sly4.h"
#include "big29.h"

using namespace sly4;
using namespace sly4::big29;

namespace {
constexpr u32 kSoundMute = 0x01021FC8;  // u8: no sounds start while set
constexpr s32 kKindSlot = 0x7641A763;   // one of the four bone slots (+256, 12 bytes each)
constexpr s32 kKindOwner = 0x411D6991;  // through o->vfn[60]() and Obj_PlaySound
constexpr s32 kKindPlain = static_cast<s32>(0xF59E7E78u);  // Audio_Start as given

// The bone's frame object: o->+160 when it is the bone's owner (+204 ==
// bone) or has no children (+272 == 0: 0); else the child found by name
// (WorldObj_ForEach with WorldObj_FindByNameCb over ctx {bone, 0} at ctxa).
u32 BoneFrame(PPUContext& ctx, u32 o, u32 bone, u32 ctxa) {
    const u32 x = load<u32>(o + 160);
    if (x == 0) return 0;
    if (load<u32>(x + 204) == bone) return x;
    if (load<u32>(x + 272) == 0) return 0;
    store<u32>(ctxa, bone);
    store<u32>(ctxa + 4, 0);
    call(ctx, b29fn::WorldObj_ForEach, {x, b29fn::WorldObj_FindByNameCb, ctxa, 0});
    return load<u32>(ctxa + 4);
}

// The bone index: Skeleton_FindBone(o->+160->vfn[28](), bone) when both
// are set, else -1.
u64 BoneIndex(PPUContext& ctx, u32 o, u32 bone) {
    const u32 x = load<u32>(o + 160);
    if (x == 0) return ~0ull;
    const u32 sk = static_cast<u32>(VCall(ctx, x, 28, {x}));
    if (sk == 0) return ~0ull;
    return call(ctx, b29fn::Skeleton_FindBone, {sk, bone});
}
}  // namespace

// Handle f(Handle* out, Obj* o, Sound* snd, Params* prm, u32 kind, u32
// bone, bool r9, bool r10, u32 tag (stack argument 8)) (called by
// f_00083DC4, Query_Run_000DB94C, f_000EC5B8, f_003941F4, f_00434674).
// *out = 0 while kSoundMute is set, o->+176 bit 0x10 is set, or snd / prm
// is 0. With r9: prm->+44 = (o->vfn[64]() ? its +408 : 0), or, when o's
// +192 bit 0x08 is set, o->vfn[32](o, 0) == 3 and o->+20->+48 is set, byte
// +1 of the flags PropSet_ReadFlags fills from it; prm->+56 |= 0x10000.
// With r10: prm->+47 = 9 when o->+244->vfn[184]() says so, else o->+216's
// +120 (0 without one); prm->+56 |= 0x80000. Then by kind: kKindPlain
// Audio_Start(snd, prm); kKindOwner Obj_PlaySound(o->vfn[60](), snd, prm,
// tag) when vfn[60] gives one; kKindSlot: slot (o->+172 >> 19) & 7 (< 4)
// = {handle, bone index, frame}, the position from Slot_BuildMatrix, the
// slot's handle the Audio_Start result and the slot counter advanced; any
// other kind: the position o->+144 (the bone's matrix translation with a
// bone), prm->+56 |= 2 and Audio_Start. Returns the handle (also *out).
DECOMP(000DB348, Obj_StartSound_000DB348) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), snd = arg<u32>(ctx, 2), prm = arg<u32>(ctx, 3);
    const s32 kind = arg<s32>(ctx, 4);
    const u32 bone = arg<u32>(ctx, 5);
    const bool f9 = static_cast<u8>(ctx.gpr[9]) != 0, f10 = static_cast<u8>(ctx.gpr[10]) != 0;
    const u32 tag = StackArg(static_cast<u32>(ctx.gpr[1]), 8);
    Stack frame(ctx, 368);
    const u32 sp = frame.addr();  // the original's frame: offsets below are its sp offsets
    u32 res = 0;
    auto finish = [&] {
        store<u32>(out, res);
        ctx.gpr[3] = res;
    };
    if (load<u8>(kSoundMute) != 0 || ((load<u16>(o + 176) >> 4) & 1) != 0 || snd == 0 || prm == 0) return finish();
    if (f9) {
        const u32 p = static_cast<u32>(VCall(ctx, o, 64, {o}));
        u32 v = p != 0 ? load<u32>(p + 408) : 0;  // stored as a byte
        if (o != 0 && (load<u32>(o + 192) & 0x8u) != 0 && static_cast<u32>(VCall(ctx, o, 32, {o, 0})) == 3) {
            const u32 owner = load<u32>(load<u32>(o + 20) + 48);
            if (owner != 0) {
                // The flags record at sp + 276: byte 0 0xFF, byte 1 0, bits
                // 0x0000FFE0 cleared (the low five bits stay what the stack
                // held), -1.0f at +4 / +8, +12 0.
                const u32 w = (load<u32>(sp + 276) & 0x1Fu) | 0xFF000000u;
                store<u8>(sp + 276, 0xFF);
                store<u8>(sp + 277, 0);
                store<u32>(sp + 280, 0xBF800000u);
                store<u32>(sp + 284, 0xBF800000u);
                store<u32>(sp + 276, w);
                store<u32>(sp + 288, 0);
                call(ctx, b29fn::PropSet_ReadFlags, {owner, sp + 276});
                v = load<u8>(sp + 277);
            }
        }
        const u32 f = load<u32>(prm + 56);
        store<u8>(prm + 44, static_cast<u8>(v));
        store<u32>(prm + 56, f | 0x10000u);
    }
    if (f10) {
        u32 v = 0;
        const u32 x = load<u32>(o + 244);
        if (x != 0 && static_cast<u32>(VCall(ctx, x, 184, {x})) != 0) {
            v = 9;
        } else {
            const u32 y = load<u32>(o + 216);
            if (y != 0) v = load<u32>(y + 120);
        }
        const u32 f = load<u32>(prm + 56);
        store<u8>(prm + 47, static_cast<u8>(v));
        store<u32>(prm + 56, f | 0x80000u);
    }
    if (kind == kKindSlot) {
        const u32 idx = (load<u32>(o + 172) >> 19) & 7;
        if (!(idx < 4)) return finish();
        const u32 e = o + 256 + 12 * idx;
        store<u32>(e, 0);
        store<u32>(e + 4, 0xFFFFFFFFu);
        store<u32>(e + 8, 0);
        u64 bi = ~0ull;
        if (bone != 0) {
            bi = BoneIndex(ctx, o, bone);
            store<u32>(e + 4, static_cast<u32>(bi));
            if (static_cast<s32>(bi) == -1) {
                const u32 fr = BoneFrame(ctx, o, bone, sp + 268);
                bi = load<u32>(e + 4);
                store<u32>(e + 8, fr);
            }
        }
        call(ctx, b29fn::Slot_BuildMatrix, {sp + 112, o, static_cast<u64>(static_cast<s64>(static_cast<s32>(bi))), load<u32>(e + 8), b29fn::QuatIdentity, b29fn::ZeroVec});
        const u32 f = load<u32>(prm + 56);
        for (int k = 0; k < 3; ++k) SetFk(ctx, 1 + k, sp + 160 + 4 * k);
        for (u32 k = 0; k < 3; ++k) store<u32>(prm + 4 * k, load<u32>(sp + 160 + 4 * k));
        store<u32>(prm + 56, f | 2);
        call(ctx, b29fn::Audio_Start, {sp + 244, snd, prm});
        res = load<u32>(sp + 244);
        store<u32>(e, res);
        const u32 w = load<u32>(o + 172);
        store<u32>(o + 172, (w & ~0x00380000u) | ((((w >> 19) & 7) + 1) & 7) << 19);
        return finish();
    }
    if (kind == kKindOwner) {
        if (static_cast<u32>(VCall(ctx, o, 60, {o})) == 0) return finish();
        const u32 own = static_cast<u32>(VCall(ctx, o, 60, {o}));
        call(ctx, b29fn::Obj_PlaySound, {sp + 248, own, snd, prm, tag});
        res = load<u32>(sp + 248);
        return finish();
    }
    if (kind == kKindPlain) {
        call(ctx, b29fn::Audio_Start, {sp + 252, snd, prm});
        res = load<u32>(sp + 252);
        return finish();
    }
    // The position: o->+144, or with a bone the translation of its matrix.
    u32 pos = o + 144;
    if (bone != 0) {
        const u64 bi = BoneIndex(ctx, o, bone);
        const u32 fr = static_cast<s32>(bi) != -1 ? 0 : BoneFrame(ctx, o, bone, sp + 260);
        call(ctx, b29fn::Slot_BuildMatrix, {sp + 176, o, bi, fr, b29fn::QuatIdentity, b29fn::ZeroVec});
        pos = sp + 224;
    }
    for (int k = 0; k < 3; ++k) SetFk(ctx, 1 + k, pos + 4 * k);
    const u32 f = load<u32>(prm + 56);
    for (u32 k = 0; k < 3; ++k) store<u32>(prm + 4 * k, load<u32>(pos + 4 * k));
    store<u32>(prm + 56, f | 2);
    call(ctx, b29fn::Audio_Start, {sp + 256, snd, prm});
    res = load<u32>(sp + 256);
    finish();
}
