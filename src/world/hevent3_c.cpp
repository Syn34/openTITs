// World_HandleEvent callees, part 3: 70..75 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3c {
namespace cfn {
constexpr u32 SortedMap_Find3 = 0x0076611C;  // (map, const u32* key) -> value slot
constexpr u32 Set_Find = 0x004BCE90;         // (u32 name) -> Set*
constexpr u32 sub_004BD1CC = 0x004BD1CC;     // (Set*)
constexpr u32 Table560_Count = 0x004ED0CC;   // (Object*, u32 key) -> u16
constexpr u32 Table560_At = 0x004ED140;      // (Object*, u32 key, s32 i) -> u32
constexpr u32 XmlDoc_Construct = 0x004FA888; // (XmlDoc*, const char* path)
constexpr u32 XmlDoc_Parse = 0x004FAF10;     // (XmlDoc*, 0) -> bool
constexpr u32 XmlDoc_Destruct = 0x004FA9C0;  // (XmlDoc*)
constexpr u32 Set_AddXml = 0x004BDA70;       // (Set*, const char* path, XmlDoc*, 0) (name?)
constexpr u32 RenderTarget_Create = 0x00522FB8;      // (u32 type, bool flag) -> rt
constexpr u32 RenderTarget_SetViewport = 0x005200AC; // (rt, const ViewRect*)
constexpr u32 sub_000DC314 = 0x000DC314;     // (Object*, 0, 0)
constexpr u32 sub_000D87CC = 0x000D87CC;     // (Object*, x)
constexpr u32 WorldObj_UpdateLod = 0x0057EE00; // (obj, 0, 0)
constexpr u32 sub_0012C968 = 0x0012C968;     // (Object*, -1, 0xFFFFFFFF, f32)
constexpr u32 sub_00129D94 = 0x00129D94;     // (Object*)
constexpr u32 sub_00566318 = 0x00566318;     // (q + 40, q->+48)
constexpr u32 sub_00390DC4 = 0x00390DC4;     // (u32)
constexpr u32 sub_004B25C0 = 0x004B25C0;     // (u32)
}  // namespace cfn

// World +0x128334: 4 entries of 28 bytes, next free index at +0x1283A4 (no bounds check):
//   +0 u32 (bits 28..31 a, 24..27 b, bit 23 c; its low half is overwritten by the
//   first 4 bytes below)  +2 4 bytes  +6 4 bytes  +12/+16/+20 f32  +24 u32 = 0 (name?)
constexpr u32 kWorldEntries = 0x128334, kWorldEntryNext = 0x1283A4, kWorldEntrySize = 28;

constexpr u32 kWorldPostFx = 0x1281E0;       // u32 object, +468 bit 0x40000000 (wupdate.h)
constexpr u32 kWorldCamSrc = 0x1281C0;       // u32 object: +769 u8 on, +772/+776/+780 f32, +784 u8 (name?)
constexpr u32 kWorldMainView = 0x128168;     // View*
constexpr u32 kClampLo = 0x0048C5BC, kClampHi = 0x0048C5C0;  // f32 0.01, 999.0

constexpr u32 kXmlTable = 0x010C2C28;        // u32 Object* with the +560 table (name?)
constexpr u32 kXmlTableKey = 0x3D50438C;     // key of the path list (name?)
constexpr u32 kXmlDocBytes = 2248;           // r1+112..r1+2360 in the original

constexpr u32 kHalf488290 = 0x00488290;      // f32 0.5

constexpr u32 kGlobalF557F4 = 0x00F557F4;    // u32 object: +4 -> u16 +2, u16 +18, +40, +48 (name?)
constexpr u32 kGlobalF6578C = 0x00F6578C;
constexpr u32 kGlobalF557F8 = 0x00F557F8;
}  // namespace hevent3c
}  // namespace sly4

using namespace sly4::hevent3c;

// u32 Obj_NotifyLinked(Object* o, u8 v): vfunc +20(obj, v) on the live objects
// of the two handles at o->+8 +156 and +160 (name?)
// r3 is returned as the original leaves it on each path.
DECOMP(003738AC, Obj_NotifyLinked) {
    const u32 t = load<u32>(arg<u32>(ctx, 0) + 8);
    const u32 v = arg<u32>(ctx, 1) & 0xFF;
    Stack frame(ctx, 16);  // r1+112 (second handle), r1+116 (first handle)
    u64 r3 = 0;
    const u32 h1 = load<u32>(t + 156);
    store<u32>(frame.addr() + 4, h1);
    if (h1 != 0) {
        const u32 obj = load<u32>(static_cast<u32>(call(ctx, cfn::SortedMap_Find3, {kHandleMap, frame.addr() + 4})));
        r3 = obj;
        if (obj != 0 && (load<u32>(obj + 4) & 1u) != 0) r3 = vcall(ctx, obj, 20, {obj, v});
    }
    const u32 h2 = load<u32>(t + 160);
    if (h2 != 0) {
        store<u32>(frame.addr(), h2);
        const u32 slot = static_cast<u32>(call(ctx, cfn::SortedMap_Find3, {kHandleMap, frame.addr()}));
        const u32 obj = load<u32>(slot);
        r3 = slot;
        if (obj != 0) {
            r3 = 0;  // the flag test result
            if ((load<u32>(obj + 4) & 1u) != 0) r3 = vcall(ctx, obj, 20, {obj, v});
        }
    }
    ctx.gpr[3] = r3;
}

// s32 World_SetEntry(World* w, const u8 a[4], const u8 b[4], -, -, u32 hi4, u32 mid4,
// -, [stack] u32 bit, s32 index; f32 x, y, z): fills entry `index` of +0x128334
// (index < 0 takes the next free one, +0x1283A4++). Returns the index (name?)
DECOMP(00787BAC, World_SetEntry) {
    u32 w = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 hi = arg<u32>(ctx, 5), mid = arg<u32>(ctx, 6);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 bit = load<u32>(sp + 116);       // argument 8 (stack slot r1+112, low word)
    s32 idx = load<s32>(sp + 124);             // argument 9 (stack slot r1+120, low word)
    if (idx < 0) {
        idx = load<s32>(w + kWorldEntryNext);
        store<s32>(w + kWorldEntryNext, idx + 1);
    }
    const u32 e = w + static_cast<u32>(idx) * kWorldEntrySize + kWorldEntries;
    u32 word = load<u32>(e) & 0x0FFFFFFFu;
    word = (word & ~0xF0000000u) | ((hi & 0xF) << 28);
    word = (word & ~0x0F000000u) | ((mid & 0xF) << 24);
    word = (word & ~0x00800000u) | ((bit & 1) << 23);
    store<u32>(e, word);
    for (u32 k = 0; k < 4; ++k) ref<u8>(e + 2 + k) = ref<u8>(a + k);
    for (u32 k = 0; k < 4; ++k) ref<u8>(e + 6 + k) = ref<u8>(b + k);
    store<f32>(e + 12, static_cast<f32>(ctx.fpr[1]));
    store<f32>(e + 16, static_cast<f32>(ctx.fpr[2]));
    store<f32>(e + 20, static_cast<f32>(ctx.fpr[3]));
    store<u32>(e + 24, 0);
    ret(ctx, static_cast<u32>(idx));  // lwz: zero-extended
}

// u32 World_ApplyCamSettings(World* w): when no post effect (+0x1281E0 absent or
// +468 bit 0x40000000) and the object at +0x1281C0 has +769 set: the main view's
// camera (+656 -> +12) gets +24 = +772, +28 = +776 clamped to [0.01, 999],
// +32 = +780, +64 = +784 clamped to [1, 4], then its vfunc +8 (name?)
DECOMP(0048C5C8, World_ApplyCamSettings) {
    const u32 w = arg<u32>(ctx, 0);
    ctx.gpr[3] = w;
    const u32 fx = load<u32>(w + kWorldPostFx);
    if (fx != 0 && (load<u32>(fx + 468) & 0x40000000u) == 0) return;
    const u32 s = load<u32>(w + kWorldCamSrc);
    if (s == 0) return;
    const u32 on = load<u8>(s + 769);
    if (on == 0) return;
    const u32 cam = load<u32>(load<u32>(load<u32>(w + kWorldMainView) + 656) + 12);
    ctx.gpr[3] = cam;
    if (cam == 0) return;
    store<u32>(cam + 24, load<u32>(s + 772));
    const u32 lo = load<u32>(kClampLo), hi = load<u32>(kClampHi);
    const f32 c = load<f32>(s + 776);
    u32 v = load<u32>(s + 776);
    u32 bound = kClampLo;  // the bound last loaded into f2
    if (c < load<f32>(kClampLo)) {
        v = lo;
    } else {
        bound = kClampHi;
        if (c > load<f32>(kClampHi)) v = hi;
    }
    store<u32>(cam + 28, v);
    store<u32>(cam + 32, load<u32>(s + 780));
    const s32 m = static_cast<s32>(load<u8>(s + 784));
    store<u8>(cam + 64, static_cast<u8>(m < 1 ? 1 : (m > 4 ? 4 : m)));
    // As in the original: r4 = +769, f1 = +780, f2 = the last clamp bound loaded.
    ctx.fpr[1] = static_cast<f64>(load<f32>(s + 780));
    ctx.fpr[2] = static_cast<f64>(load<f32>(bound));
    ctx.gpr[4] = on;
    const u32 opd = load<u32>(load<u32>(cam) + 8);
    ctx.gpr[5] = opd;
    ctx.gpr[6] = load<u32>(opd);
    call_ptr(ctx, opd, {cam});
}

// u32 Set_LoadTableXmls(): for the set Set_Find(0): sub_004BD1CC(set), then
// every path listed under 0x3D50438C in the table at 0x010C2C28 is parsed as
// an XmlDoc and handed to Set_AddXml(set, path, doc, 0) (name?)
DECOMP(004BDDB8, Set_LoadTableXmls) {
    const u32 set = static_cast<u32>(call(ctx, cfn::Set_Find, {0}));
    if (set == 0) return;
    call(ctx, cfn::sub_004BD1CC, {set});
    const u32 tbl = load<u32>(kXmlTable);
    if (tbl == 0) return;
    const u32 n = static_cast<u32>(call(ctx, cfn::Table560_Count, {tbl, kXmlTableKey}));
    if (n == 0) return;
    Stack frame(ctx, kXmlDocBytes);
    const u32 doc = frame.addr();
    for (u32 i = 0; i < n; ++i) {
        const u32 path = static_cast<u32>(call(ctx, cfn::Table560_At,
            {load<u32>(kXmlTable), kXmlTableKey, static_cast<u64>(static_cast<s64>(static_cast<s32>(i)))}));
        call(ctx, cfn::XmlDoc_Construct, {doc, path});
        if (static_cast<u32>(call(ctx, cfn::XmlDoc_Parse, {doc, 0})) != 0)
            call(ctx, cfn::Set_AddXml, {set, path, doc, 0});
        call(ctx, cfn::XmlDoc_Destruct, {doc});
    }
}

// u32 Obj_SetRenderTarget(Object* o, u8 on): +344 bit 31 tracks it. On: makes the
// target at +364 (RenderTarget_Create(0, 0), or +4 u16 refs++ on an existing one)
// and sets its viewport to +346. Off: sub_000DC314(o, 0, 0) and the target's
// vfunc +8; +364 = 0 when its refs (+4, s16) are still above 0 afterwards.
// Returns the new +344 (name?)
DECOMP(000FA3CC, Obj_SetRenderTarget) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    const bool was = (load<u32>(o + 344) >> 31) != 0;
    if ((on & 0xFF) != 0) {
        if (!was) {
            u32 rt = load<u32>(o + 364);
            if (rt == 0) {
                rt = static_cast<u32>(call(ctx, cfn::RenderTarget_Create, {0, 0}));
                store<u32>(o + 364, rt);
            } else {
                store<u16>(rt + 4, static_cast<u16>(load<u16>(rt + 4) + 1));
            }
            call(ctx, cfn::RenderTarget_SetViewport, {rt, o + 346});
        }
    } else if (was) {
        call(ctx, cfn::sub_000DC314, {o, 0, 0});
        const u32 rt = load<u32>(o + 364);
        vcall(ctx, rt, 8, {rt});
        const s32 refs = static_cast<s16>(load<u16>(load<u32>(o + 364) + 4));
        if (refs > 0) store<u32>(o + 364, 0);
    }
    const u32 v = (load<u32>(o + 344) & 0x7FFFFFFFu) | ((on & 1u) << 31);
    store<u32>(o + 344, v);
    ret(ctx, v);
}

// u32 User_RefreshObject(World* w, u32 user, x): for the object of user block
// `user` (+8): vfunc +84(obj, x), sub_000D87CC(obj, x), WorldObj_UpdateLod(+160, 0, 0)
// when set, vfunc +44(obj); then with +192 bit 0x8, sub_0012C968(obj, -1,
// 0xFFFFFFFF, 0.5) and sub_00129D94(obj) (name?)
DECOMP(00488298, User_RefreshObject) {
    const u32 w = arg<u32>(ctx, 0), user = arg<u32>(ctx, 1);
    const u64 x = ctx.gpr[5];
    const u32 blk = w + user * kUserBlockSize + kUserBlocks;
    u32 o = load<u32>(blk + 8);
    if (o == 0) return ret(ctx, 0u);
    vcall(ctx, o, 84, {o, x});
    call(ctx, cfn::sub_000D87CC, {load<u32>(blk + 8), x});
    o = load<u32>(blk + 8);
    const u32 lod = load<u32>(o + 160);
    if (lod != 0) {
        call(ctx, cfn::WorldObj_UpdateLod, {lod, 0, 0});
        o = load<u32>(blk + 8);
    }
    vcall(ctx, o, 44, {o});
    o = load<u32>(blk + 8);
    if (o == 0) return;  // r3 = vfunc +44's result
    if ((load<u32>(o + 192) & 0x8u) == 0) return ret(ctx, 0u);
    callf(ctx, cfn::sub_0012C968, {o, ~0ull, 0xFFFFFFFFull}, {static_cast<f64>(load<f32>(kHalf488290))});
    call(ctx, cfn::sub_00129D94, {o});
}

// void Obj_ResetSlots(Object* o): like Obj_InitSlots (hevent3_a.cpp) with the
// reset calls: clears +0, +4, the counters of the object at 0x00F557F4 (+4 ->
// +2, +18) and runs sub_00566318(q + 40, q->+48), sub_00390DC4(*0x00F6578C),
// sub_004B25C0(*0x00F557F8 + 4); +24 = 0 and the 2 x 8 slot pointers (name?)
DECOMP(0001AB3C, Obj_ResetSlots) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(kGlobalF557F4);
    store<u32>(o + 0, 0);
    store<u32>(o + 4, 0);
    store<u16>(load<u32>(q + 4) + 2, 0);
    store<u16>(q + 18, 0);
    call(ctx, cfn::sub_00566318, {q + 40, load<u32>(q + 48)});
    call(ctx, cfn::sub_00390DC4, {load<u32>(kGlobalF6578C)});
    call(ctx, cfn::sub_004B25C0, {load<u32>(kGlobalF557F8) + 4});
    store<u32>(o + 24, 0);
    for (u32 k = 0; k < 2; ++k)
        for (u32 j = 0; j < 8; ++j) store<u32>(o + 3732 + k * 32 + j * 4, o + 3808 + k * 640 + j * 80);
}
