// Game code, large-function sweep part 29: an object's sound start by
// kind (plain, at a bone, round-robin bone slots, through the object's
// sound owner).
#include "big29.h"

namespace sly4::obj {

namespace {
constexpr u32 kSoundMute = 0x01021FC8;  // u8: no sounds start while set
constexpr s32 kKindSlot = 0x7641A763;   // one of the four bone slots (+256, 12 bytes each)
constexpr s32 kKindOwner = 0x411D6991;  // through o->vfn[60]() and Obj_PlaySound
constexpr s32 kKindPlain = static_cast<s32>(0xF59E7E78u);  // Audio_Start as given
constexpr u32 Audio_Start = 0x0044A374;   // (Handle* out, snd, params)
constexpr u32 Slot_BuildMatrix = 0x000CEA48;  // (Mtx* out, Obj*, s32 bone, Obj* frame, const Quat*, const Vec*)
constexpr u32 Obj_PlaySound = 0x00298C10;  // (u32* out, Obj*, Sound*, Pos*, u32 tag)
constexpr u32 PropSet_ReadFlags = 0x004A1440;  // (Owner*, Flags* out)
constexpr u32 WorldObj_ForEach = 0x0057F5D8;   // (obj, fn opd, arg, bool recurse)
constexpr u32 Skeleton_FindBone = 0x005A93D8;  // (skeleton, u32 hash) -> s32 index or -1
constexpr u32 WorldObj_FindByNameCb = 0x00F31D20;  // OPD: ForEach callback, ctx {u32 hash, Obj* found}
constexpr u32 QuatIdentity = 0x01164C90;  // the global vector Slot_BuildMatrix callers pass with ZeroVec

// A bone slot: the sound handle, the bone index, the bone's frame object.
struct SndSlot {
    be_u32 handle;  // +0
    be_s32 bone;    // +4
    be_u32 frame;   // +8
};
static_assert(sizeof(SndSlot) == 12);
// The object's frame object: +204 its bone, +272 its children.
struct SndFrame {
    pad<204> _0;
    be_u32 bone;      // +204
    pad<64> _208;
    be_u32 children;  // +272
};
static_assert(offsetof(SndFrame, children) == 272);
struct SndHolder {
    pad<48> _0;
    be_u32 owner;  // +48
};
struct SndObj {
    pad<20> _0;
    gp<SndHolder> holder;  // +20
    pad<120> _24;
    Vec3 pos;              // +144
    pad<4> _156;
    gp<SndFrame> frame;    // +160
    pad<8> _164;
    be_u32 f172;           // +172: bits 21..19 the next slot
    be_u16 f176;           // +176: bit 4 silent
    pad<14> _178;
    be_u32 f192;           // +192
    pad<20> _196;
    be_u32 lvl;            // +216: an object with +120
    pad<24> _220;
    be_u32 x244;           // +244
    pad<8> _248;
    SndSlot slots[4];      // +256
};
static_assert(offsetof(SndObj, pos) == 144 && offsetof(SndObj, f176) == 176 && offsetof(SndObj, lvl) == 216);
static_assert(offsetof(SndObj, x244) == 244 && offsetof(SndObj, slots) == 256);
// The sound parameters: +0 the position, u8 +44 / +47, +56 flags.
struct SndParams {
    be_u32 pos[3];  // +0 (raw)
    pad<32> _12;
    u8 b44;         // +44
    pad<2> _45;
    u8 b47;         // +47
    pad<8> _48;
    be_u32 flags;   // +56
};
static_assert(offsetof(SndParams, b47) == 47 && offsetof(SndParams, flags) == 56);
// The flags record PropSet_ReadFlags fills.
struct SndFlags {
    WordBytes w;   // +0
    be_u32 a;      // +4
    be_u32 b;      // +8
    be_u32 c;      // +12
};
// The ForEach context: the bone hash and the object found.
struct SndFind {
    be_u32 bone;   // +0
    be_u32 found;  // +4
};

// The bone's frame object: o->+160 when it is the bone's owner (+204 ==
// bone) or has no children (+272 == 0: 0); else the child found by name
// (WorldObj_ForEach with WorldObj_FindByNameCb over the context c).
u32 BoneFrame(SndObj* o, u32 bone, SndFind* c) {
    const u32 x = o->frame.ea;
    if (x == 0) return 0;
    if (gm<SndFrame>(x).bone == bone) return x;
    if (gm<SndFrame>(x).children == 0u) return 0;
    c->bone = bone;
    c->found = 0u;
    call(WorldObj_ForEach, x, WorldObj_FindByNameCb, c, 0u);
    return c->found;
}

// The bone index: Skeleton_FindBone(o->+160->vfn[28](), bone) when both
// are set, else -1.
s32 BoneIndex(SndObj* o, u32 bone) {
    const u32 x = o->frame.ea;
    if (x == 0) return -1;
    const u32 sk = vcall<u32>(host<u8>(x), 28);
    if (sk == 0) return -1;
    return call<s32>(Skeleton_FindBone, sk, bone);
}

const Vec3& Xyz(const Vec4& v) { return reinterpret_cast<const Vec3&>(v); }

// The position words to prm, prm->+56 |= 2 (the flags read first).
void SetPos(SndParams* prm, const Vec3& pos) {
    const u32 f = prm->flags;
    prm->pos[0] = bits(pos.x);
    prm->pos[1] = bits(pos.y);
    prm->pos[2] = bits(pos.z);
    prm->flags = f | 2u;
}
}  // namespace

// void f(Handle* out, Obj* o, Sound* snd, Params* prm, u32 kind, u32
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
// bone), prm->+56 |= 2 and Audio_Start. *out gets the handle. No caller
// reads r3 (r3.sh).
void Obj_StartSound_000DB348(be_u32* out, SndObj* o, u32 snd, SndParams* prm, s32 kind, u32 bone, u8 f9, u8 f10, u32 tag) {
    GuestFrame frame(368);  // the original's frame: offsets below are its sp offsets
    u32 res = 0;
    auto finish = [&] { *out = res; };
    if (global<u8>(kSoundMute) != 0 || ((o->f176 >> 4) & 1) != 0 || snd == 0 || prm == nullptr) return finish();
    if (f9 != 0) {
        const u32 p = vcall<u32>(o, 64);
        u32 v = p != 0 ? gm<be_u32>(p + 408).get() : 0u;  // stored as a byte
        if (o != nullptr && (o->f192 & 0x8u) != 0 && vcall<u32>(o, 32, 0u) == 3) {
            const u32 owner = gm<SndHolder>(o->holder.ea).owner;
            if (owner != 0) {
                // The flags record at sp + 276: byte 0 0xFF, byte 1 0, bits
                // 0x0000FFE0 cleared (the low five bits stay what the stack
                // held), -1.0f at +4 / +8, +12 0.
                SndFlags* fl = frame.as<SndFlags>(276);
                const u32 w = (fl->w.w & 0x1Fu) | 0xFF000000u;
                fl->w.b[0] = 0xFF;
                fl->w.b[1] = 0;
                fl->a = 0xBF800000u;
                fl->b = 0xBF800000u;
                fl->w.w = w;
                fl->c = 0u;
                call(PropSet_ReadFlags, owner, fl);
                v = fl->w.b[1];
            }
        }
        const u32 f = prm->flags;
        prm->b44 = static_cast<u8>(v);
        prm->flags = f | 0x10000u;
    }
    if (f10 != 0) {
        u32 v = 0;
        const u32 x = o->x244;
        if (x != 0 && vcall<u32>(host<u8>(x), 184) != 0) {
            v = 9;
        } else {
            const u32 y = o->lvl;
            if (y != 0) v = gm<be_u32>(y + 120);
        }
        const u32 f = prm->flags;
        prm->b47 = static_cast<u8>(v);
        prm->flags = f | 0x80000u;
    }
    if (kind == kKindSlot) {
        const u32 idx = (o->f172 >> 19) & 7;
        if (!(idx < 4)) return finish();
        SndSlot& e = o->slots[idx];
        e.handle = 0u;
        e.bone = -1;
        e.frame = 0u;
        s32 bi = -1;
        if (bone != 0) {
            bi = BoneIndex(o, bone);
            e.bone = bi;
            if (bi == -1) {
                const u32 fr = BoneFrame(o, bone, frame.as<SndFind>(268));
                bi = e.bone;
                e.frame = fr;
            }
        }
        Mtx44* m = frame.as<Mtx44>(112);
        call(Slot_BuildMatrix, m, o, bi, e.frame.get(), QuatIdentity, kZeroVec);
        SetPos(prm, Xyz(m->r[3]));
        be_u32* h = frame.as<be_u32>(244);
        call(Audio_Start, h, snd, prm);
        res = *h;
        e.handle = res;
        const u32 w = o->f172;
        o->f172 = (w & ~0x00380000u) | ((((w >> 19) & 7) + 1) & 7) << 19;
        return finish();
    }
    if (kind == kKindOwner) {
        if (vcall<u32>(o, 60) == 0) return finish();
        const u32 own = vcall<u32>(o, 60);
        be_u32* h = frame.as<be_u32>(248);
        call(Obj_PlaySound, h, own, snd, prm, tag);
        res = *h;
        return finish();
    }
    if (kind == kKindPlain) {
        be_u32* h = frame.as<be_u32>(252);
        call(Audio_Start, h, snd, prm);
        res = *h;
        return finish();
    }
    // The position: o->+144, or with a bone the translation of its matrix.
    const Vec3* pos = &o->pos;
    if (bone != 0) {
        const s32 bi = BoneIndex(o, bone);
        const u32 fr = bi != -1 ? 0u : BoneFrame(o, bone, frame.as<SndFind>(260));
        Mtx44* m = frame.as<Mtx44>(176);
        call(Slot_BuildMatrix, m, o, bi, fr, QuatIdentity, kZeroVec);
        pos = &Xyz(m->r[3]);
    }
    SetPos(prm, *pos);
    be_u32* h = frame.as<be_u32>(256);
    call(Audio_Start, h, snd, prm);
    res = *h;
    finish();
}

}  // namespace sly4::obj

NATIVE(000DB348, sly4::obj::Obj_StartSound_000DB348)
