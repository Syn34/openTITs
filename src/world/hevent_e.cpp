// World_HandleEvent callees, 38..41 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
namespace efn {
constexpr u32 sub_0035B984 = 0x0035B984;   // (u32)
constexpr u32 Obj_Notify24 = 0x000D949C;   // (Object*) -> Object* (sub_000D1288(o, 24, 0))
constexpr u32 Obj_Notify26 = 0x000D94CC;   // (Object*) -> Object* (sub_000D1288(o, 26, 0))
constexpr u32 sub_002A8AE0 = 0x002A8AE0;   // (Object*, 0, 1, 0)
constexpr u32 Trophy_Has = 0x004FB56C;     // (Trophy*, s32 i) -> bool (name?)
constexpr u32 Obj_UpdateFlags2336 = 0x0029D838;  // (Object*, u8) src/world/hevent_a.cpp
constexpr u32 XmlDoc_Construct = 0x004FA888;
constexpr u32 XmlDoc_Parse = 0x004FAF10;
constexpr u32 XmlDoc_Destruct = 0x004FA9C0;
constexpr u32 Xml_Load0D5F58 = 0x000D5F58;  // (XmlDoc*, const char* path, u8) -> u32 (name?)
}  // namespace efn

constexpr u32 kMap106C258 = 0x0106C258;     // SortedMap of objects (+208 bit 31 cleared on reset) (name?)
constexpr u32 kTune10BC1C8 = 0x010BC1C8;    // tuning object, value at +72 (name?)
constexpr u32 kWorldChild1281F4 = 0x1281F4; // u32 Object* (vfunc +12 then delete) (name?)
constexpr u32 kExposure = 0x128270;         // World +: f32 x4, u32, f32 x2 (name?)
constexpr u32 kExposureConsts = 0x0048447C; // f32 1e-5, 0.0
constexpr u32 kXmlDocBytes = 2256;          // the original keeps an XmlDoc at r1+112..r1+2360
}  // namespace

// void World_Player0Release(World* w): sub_0035B984(0); then for user 0's
// object (block +8) with +192 bit 0x8: if Obj_Notify26(p),
// sub_002A8AE0(Obj_Notify26(p), 0, 1, 0) (name?)
DECOMP(0047C218, World_Player0Release) {
    const u32 w = arg<u32>(ctx, 0);
    call(ctx, efn::sub_0035B984, {0});
    const u32 p = load<u32>(w + kUserBlocks + 8);
    if (p == 0) return ret(ctx, w + kUserBlocks);
    const u32 b = load<u32>(p + 192) & 0x8u;
    if (b == 0) return ret(ctx, 0u);
    if (call(ctx, efn::Obj_Notify26, {p}) == 0) return;
    const u32 c = static_cast<u32>(call(ctx, efn::Obj_Notify26, {p}));
    call(ctx, efn::sub_002A8AE0, {c, 0, 1, 0});
}

// s32 Trophy_CountUnlocked(): how many of the trophy manager's u16 +200 entries
// Trophy_Has() reports (0 until it is ready) (name?)
DECOMP(000B2914, Trophy_CountUnlocked) {
    const u32 t = load<u32>(kTrophy);
    s32 n = 0;
    if (ref<u8>(t + 8) != 0) n = static_cast<s32>(load<u32>(t + 200) & 0xFFFFu);
    s32 count = 0;
    for (s32 i = 0; i < n; ++i)
        if (call(ctx, efn::Trophy_Has, {load<u32>(kTrophy), static_cast<u32>(i)}) != 0) ++count;
    ret(ctx, count);
}

// void Users_UpdateFlags2336(?, u8 reset): for each user (World +29) with an
// object whose Obj_Notify24 is set: Obj_UpdateFlags2336(that, reset).
DECOMP(002E4A0C, Users_UpdateFlags2336) {
    const u32 reset = arg<u32>(ctx, 1) & 0xFF;
    for (u32 i = 0, off = 0;; ++i, off += kUserBlockSize) {
        const u32 w = load<u32>(kCurrentWorld);
        const u32 users = ref<u8>(w + 29);
        if (i >= users) {
            ctx.gpr[4] = users;
            return ret(ctx, w);
        }
        const u32 p = load<u32>(w + off + kUserBlocks + 8);
        if (p == 0) continue;
        const u32 c = static_cast<u32>(call(ctx, efn::Obj_Notify24, {p}));
        if (c != 0) call(ctx, efn::Obj_UpdateFlags2336, {c, reset});
    }
}

// void Map106C258_ResetAll(): every object of the map at 0x0106C258 gets
// +208 bit 31 cleared, then vfunc +12(obj) (name?)
DECOMP(00369500, Map106C258_ResetAll) {
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(kMap106C258 + 2)); ++i) {
        const u32 obj = load<u32>(load<u32>(kMap106C258 + 8) + i * 8 + 4);
        const u32 f = load<u32>(obj + 208) & 0x7FFFFFFFu;
        store<u32>(obj + 208, f);
        ctx.gpr[6] = f;
        vcall(ctx, obj, 12, {obj});
    }
}

// void Obj_AdvanceTimer108(Object* o, f32 dt): unless +0 bit 0x40, f108 =
// min(f100, f108 + rate * dt) with rate = f104, or tuning 0x010BC1C8 while the
// object at +8 has +192 bit 0x8 and byte +740 0x80 without 0x40. Clears +0 bit 0x40.
DECOMP(00465CB8, Obj_AdvanceTimer108) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o);
    if (((w >> 6) & 1) == 0) {
        f32 rate = load<f32>(o + 104);
        const u32 q = load<u32>(o + 8);
        ctx.gpr[4] = q;
        if (q != 0) {
            const u32 f = load<u32>(q + 192) & 0x8u;
            ctx.gpr[6] = f;
            if (f != 0) {
                const u32 b = ref<u8>(q + 740);
                ctx.gpr[6] = (b >> 7) & 1;
                ctx.gpr[4] = b;
                if (((b >> 7) & 1) != 0) {
                    ctx.gpr[4] = (b >> 6) & 1;
                    if (((b >> 6) & 1) == 0) {
                        ctx.gpr[4] = kTune10BC1C8;
                        rate = load<f32>(kTune10BC1C8 + 72);
                    }
                }
            }
        }
        const f64 dt = farg(ctx, 0);
        const f32 cur = load<f32>(o + 108), hi = load<f32>(o + 100);
        f32 v = static_cast<f32>(std::fma(static_cast<f64>(rate), dt, static_cast<f64>(cur)));
        if (hi < v) v = hi;
        store<f32>(o + 108, v);
        ctx.fpr[1] = hi;
        ctx.fpr[2] = v;
        ctx.fpr[3] = dt;
        ctx.fpr[4] = cur;
    }
    store<u32>(o, w & ~0x40u);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = w & ~0x40u;
}

// void World_ReleaseChild1281F4(World* w): when +0x1281F4 is set: vfunc +12(p),
// then (if still set) vfunc +4(p) (the deleting destructor; r4 = its OPD as in
// the original), then +0x1281F4 = 0.
DECOMP(0047C490, World_ReleaseChild1281F4) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 p = load<u32>(w + kWorldChild1281F4);
    if (p == 0) return ret(ctx, 0u);
    vcall(ctx, p, 12, {p});
    const u32 q = load<u32>(w + kWorldChild1281F4);
    if (q != 0) vcall(ctx, q, 4, {q});
    store<u32>(w + kWorldChild1281F4, 0);
    ctx.gpr[4] = 0;
    ret(ctx, kWorldChild1281F4);
}

// void World_SetExposure(World* w, f32 target, f32 rate, f32 c): with |rate| <
// 1e-5 the value (+0x128270) jumps to target and +0x128280 = 0.0; otherwise the
// old value goes to +0x128278, target to +0x128274, c to +0x128284, +0x12827C = 0
// and +0x128280 = rate (name?)
DECOMP(00484488, World_SetExposure) {
    const u32 w = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1), c = farg(ctx, 2);
    const f32 eps = load<f32>(kExposureConsts);
    ctx.fpr[4] = std::fabs(b);
    ctx.fpr[5] = eps;
    if (std::fabs(b) < static_cast<f64>(eps)) {
        store<f32>(w + kExposure, static_cast<f32>(a));
        const f32 z = load<f32>(kExposureConsts + 4);
        store<f32>(w + kExposure + 16, z);
        ctx.fpr[1] = z;
        ctx.gpr[5] = kExposureConsts;
    } else {
        store<u32>(w + kExposure + 8, load<u32>(w + kExposure));  // lfs/stfs pair
        ctx.fpr[1] = load<f32>(w + kExposure);
        store<f32>(w + kExposure + 4, static_cast<f32>(a));
        store<f32>(w + kExposure + 20, static_cast<f32>(c));
        store<u32>(w + kExposure + 12, 0);
        store<f32>(w + kExposure + 16, static_cast<f32>(b));
        ctx.fpr[1] = b;
        ctx.fpr[4] = a;
        ctx.gpr[5] = 0x12827C;
        ctx.gpr[6] = 0x128278;
        ctx.gpr[7] = 0x128274;
        ctx.gpr[8] = 0x128284;
        ctx.gpr[9] = 0x120000;
    }
    ctx.gpr[4] = 0x128280;
}

// u32 Xml_LoadFile(const char* path, u8 v): parses the file into a stack XmlDoc
// and returns Xml_Load0D5F58(doc, path, v), or v when it does not parse (name?)
DECOMP(000D6398, Xml_LoadFile) {
    const u32 path = arg<u32>(ctx, 0);
    const u32 v = arg<u32>(ctx, 1) & 0xFF;
    Stack frame(ctx, kXmlDocBytes);
    const u32 doc = frame.addr();
    call(ctx, efn::XmlDoc_Construct, {doc, path});
    if (call(ctx, efn::XmlDoc_Parse, {doc, 0}) == 0) {
        call(ctx, efn::XmlDoc_Destruct, {doc});
        return ret(ctx, v);
    }
    const u32 r = static_cast<u32>(call(ctx, efn::Xml_Load0D5F58, {doc, path, v}));
    call(ctx, efn::XmlDoc_Destruct, {doc});
    ret(ctx, r);
}
