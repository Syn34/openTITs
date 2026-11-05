// World_HandleEvent callees, part 28d: the view's construction from its
// definition (View_Init0F9854: fields, the main and the additive camera).
#include "hevent28.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;
using namespace sly4::hevent28;

namespace {
namespace vfn {
constexpr u32 View_BaseInit = 0x000D3B94;     // (View*, Def*) (Thunk_000D3B94)
constexpr u32 View_SetSplit = 0x000F94C0;     // (View*, s32 split) (View_SetSplit0F94C0)
constexpr u32 Ctl_Construct = 0x0034BDEC;     // (Ctl* 120 bytes, Body*) (Ctl_Construct34BDEC)
constexpr u32 Box_SetPoint = 0x00556F60;      // (box, p)
constexpr u32 PointDef_Reset = 0x0059C7B0;    // (def)
constexpr u32 Sprintf = 0x00724304;           // (buf, fmt, ...)
constexpr u32 Def_SetName = 0x00584BDC;       // (def, name, bool hash)
constexpr u32 WorldObj_SetName = 0x0057D368;  // (obj, name, bool)
constexpr u32 Mem_Free = 0x00735F54;          // (p) (Mem_Free735F54)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;      // (p, pool)
}  // namespace vfn

constexpr u32 kViewVtable = 0x00B139B8;
constexpr u32 kSubVtA = 0x00B13988, kSubVtB = 0x00B13998, kSubVtC = 0x00B139A8;  // embedded objects at +688 / +784 / +880 (name?)
constexpr u32 kCtlVtable = 0x00F31530;     // controller +60 (name?)
// View: +160 / +612 cameras, +164 / +608 their controllers, +344 flags
// (bit 31 clear, bits 27..30 split, bits 24..26 mode), +368 64 zero bytes.
constexpr u32 kViewCam = 160, kViewCtl = 164, kViewFlags = 344, kViewCamAdd = 612, kViewCtlAdd = 608;
// Def: +28 name, +144 u32, +148 split, +152 mode, +20 bit 16: a controller.
constexpr u32 kDefName = 28;
constexpr u32 kIdentity43 = 0x01164BD0;    // 4 rows of 3 f32 (stride 16)
constexpr u32 kVecB40 = 0x01164B40, kVecC90 = 0x01164C90, kVecBC0 = 0x01164BC0;  // f32[3] / f32[4] / f32[4] constants (name?)
constexpr u32 kBytesCA8 = 0x01164CA8;      // 4 bytes copied to +464 / +576
constexpr u32 kF35378 = 0x01035378;        // f32 copied to +776 / +872 / +908 (name?)
constexpr u32 kCamDefSerial = 0x01164CD0;  // u32 counter: every new camera def's +8 bits 11..31
constexpr u32 kCamDefVtable = 0x00B246B0, kPointDefVtable = 0x00B247B8;
constexpr u32 kCamTag = 0x02235C33;        // camera +276; +280 = the view
constexpr u32 kAdditiveFmt = 0x00AC6D5C;   // "%s_additive"
constexpr u32 kFreeListName = 0x00AC6D0C;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeCameraDef]"
constexpr u32 kFrame = 416, kBufAt = 112;  // the original's frame; name buffer at r1 +112

void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
void copy43(u32 dst, u32 src) {
    for (u32 row = 0; row < 4; ++row)
        for (u32 c = 0; c < 12; c += 4) copy32(dst + 16 * row + c, src + 16 * row + c);
}

// A new camera def from the allocator's free list (vt +12) made a point
// def: header (+4 = 1 / 23, +8 serial), empty bounds at +16, +48 the
// constant rotation, an old +108 reference released, then PointDef_Reset.
// f1 is left as the original's last load.
void camdef_init(PPUContext& ctx, u32 def) {
    const u32 w4 = load<u32>(def + 4);
    store<u16>(def + 4, 1);
    store<u32>(def + 4, 0x00010000u | (23u << 10) | (w4 & 0x3FFu));
    const u32 serial = load<u32>(kCamDefSerial) + 1;
    store<u32>(kCamDefSerial, serial);
    u32 w8 = load<u32>(def + 8);
    store<u32>(def + 0, kCamDefVtable);
    w8 = ((w8 & 0x7FFu) | (serial << 11)) & ~0x400u & ~0x3C0u;
    store<u32>(def + 32, 0xFF7FFFFFu);  // -FLT_MAX
    store<u32>(def + 36, 0xFF7FFFFFu);
    store<u32>(def + 16, 0x7F7FFFFFu);  // FLT_MAX
    store<u32>(def + 20, 0x7F7FFFFFu);
    store<u32>(def + 24, 0x7F7FFFFFu);
    store<u32>(def + 40, 0xFF7FFFFFu);
    store<u32>(def + 108, 0);
    store<u32>(def + 12, 0);
    store<u32>(def + 8, w8);
    call(ctx, vfn::Box_SetPoint, {def + 16, kVecB40});
    copy32(def + 48, kVecBC0 + 0);
    copy32(def + 52, kVecBC0 + 4);
    const u32 ref = load<u32>(def + 108);
    copy32(def + 56, kVecBC0 + 8);
    store<u32>(def + 96, 0);
    copy32(def + 60, kVecBC0 + 12);
    ctx.fpr[1] = lf(kVecBC0 + 12);
    store<u32>(def + 100, 0);
    store<u8>(def + 64, 0);
    store<u32>(def + 104, 0);
    if (ref != 0) {
        store<u32>(def + 108, 0);
        const u16 n = static_cast<u16>(load<u16>(ref) - 1);
        store<u16>(ref, n);
        if (static_cast<s16>(n) <= 0) {
            const u32 p = load<u32>(ref + 4);
            if (p != 0) {
                call(ctx, vfn::Mem_Free, {p - 16});
                store<u32>(ref + 4, 0);
            }
            call(ctx, h27fn::Mem_FreeFrom, {ref, 0});
        }
    }
    store<u32>(def + 0, kPointDefVtable);
    call(ctx, vfn::PointDef_Reset, {def});
}

// Camera instance for def named name (or the name read from name_slot
// each time, when non-zero): stored at view +slot, named, tagged (+276 /
// +280 = the view), then the def's reference dropped (vt +8).
void camera_make(PPUContext& ctx, u32 v, u32 def, u32 name, u32 name_slot, u32 slot) {
    call(ctx, vfn::Def_SetName, {def, name_slot != 0 ? load<u32>(name_slot) : name, 1});
    const u32 cam = static_cast<u32>(call(ctx, h28fn::CameraInstance_New, {def}));
    store<u32>(v + slot, cam);
    call(ctx, vfn::WorldObj_SetName, {cam, name_slot != 0 ? load<u32>(name_slot) : name, 1});
    const u32 c = load<u32>(v + slot);
    store<u32>(c + 280, v);
    store<u32>(c + 276, kCamTag);
    vcall(ctx, def, 8, {def});
}

// A 120-byte controller for the body at view +body (0 when out of memory).
u32 ctl_new(PPUContext& ctx, u32 v, u32 body) {
    const u32 c = static_cast<u32>(call(ctx, h28fn::SmallAlloc, {120}));
    if (c == 0) return 0;
    call(ctx, vfn::Ctl_Construct, {c, load<u32>(v + body)});
    return c;
}
}  // namespace

// u32 View_Init0F9854(View* v, ViewDef* d) (name?): base init, then the
// view's fields (split / mode from d +148 / +152, fov 55, 7.5, identity
// matrices, embedded objects), the main camera from a fresh camera def
// named d +28 (with a controller when d +20 bit 16; the controller is
// written even when its allocation failed, as the original), and the
// additive camera "<name>_additive" with its controller. Returns that
// controller (0 when there is no second camera def).
DECOMP(000F9854, View_Init0F9854) {
    const u32 v = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, kFrame);
    call(ctx, vfn::View_BaseInit, {v, d});
    u32 w = load<u32>(v + kViewFlags) & 0x7FFFFFFFu;
    store<u32>(v + kViewFlags, w);
    store<u16>(v + 348, 0);
    w = (w & ~0x78000000u) | ((load<u32>(d + 148) << 27) & 0x78000000u);
    store<u32>(v + kViewFlags, w);
    store<u32>(v + 0, kViewVtable);
    w = (w & ~0x07000000u) | ((load<u32>(d + 152) & 7u) << 24);
    store<u16>(v + 350, 0);
    store<u16>(v + 352, 0);
    store<u32>(v + kViewFlags, w);
    store<u8>(v + 345, 0);
    store<u16>(v + 346, 0);
    store<u32>(v + 356, 0x425C0000u);  // 55.0f
    store<u32>(v + 360, 0x40F00000u);  // 7.5f
    copy32(v + 364, d + 144);
    for (const u32 o : {432u, 436u, 444u, 448u, 452u, 460u}) store<u32>(v + o, 0);
    store<u8>(v + 440, 0);
    store<u8>(v + 456, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(v + 464 + k, load<u8>(kBytesCA8 + k));
    store<u32>(v + 468, (load<u32>(v + 468) & 0x7FFFFFFFu) | 0x60000000u);
    copy43(v + 480, kIdentity43);
    store<u32>(v + 544, 0x425C0000u);
    store<u32>(v + 548, 0x40F00000u);
    for (const u32 o : {556u, 560u, 564u, 572u}) store<u32>(v + o, 0);
    store<u8>(v + 552, 0);
    store<u8>(v + 568, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(v + 576 + k, load<u8>(kBytesCA8 + k));
    store<u32>(v + 592, 0xBF800000u);  // -1.0f
    store<u32>(v + 596, 0xBF800000u);
    copy43(v + 624, kIdentity43);
    for (const u32 o : {704u, 720u, 736u, 752u})
        for (u32 c = 0; c < 12; c += 4) copy32(v + o + c, kVecB40 + c);
    for (const u32 o : {608u, 612u, 768u, 600u}) store<u32>(v + o, 0);
    copy32(v + 776, kF35378);
    store<u8>(v + 604, 0);
    store<u8>(v + 605, 0);
    store<u32>(v + 688, kSubVtA);
    for (u32 c = 0; c < 16; c += 4) copy32(v + 800 + c, kVecC90 + c);
    for (u32 c = 0; c < 12; c += 4) copy32(v + 816 + c, kVecB40 + c);
    store<u32>(v + 784, kSubVtB);
    store<u8>(v + 772, 0);
    for (u32 c = 0; c < 16; c += 4) copy32(v + 832 + c, kVecC90 + c);
    store<u32>(v + 880, kSubVtC);
    for (u32 c = 0; c < 12; c += 4) copy32(v + 848 + c, kVecB40 + c);
    store<u32>(v + 864, 0);
    store<u8>(v + 868, 0);
    copy32(v + 872, kF35378);
    for (u32 o = 884; o <= 900; o += 4) store<u32>(v + o, 0);
    store<u8>(v + 904, 0);
    copy32(v + 908, kF35378);
    ctx.fpr[1] = lf(kF35378);

    // The main camera.
    {
        const u32 alloc = camdef_allocator(ctx, kFreeListName);
        const u32 def = static_cast<u32>(vcall(ctx, alloc, 12, {alloc}));  // also on a null allocator (as the original)
        if (def != 0) {
            camdef_init(ctx, def);
            camera_make(ctx, v, def, 0, d + kDefName, kViewCam);
        }
    }
    call(ctx, vfn::View_SetSplit, {v, (load<u32>(v + kViewFlags) >> 27) & 0xFu});
    {
        const u32 z = (v + 368) & ~15u;  // four stvx of zero
        for (u32 k = 0; k < 64; k += 4) store<u32>(z + k, 0);
    }
    if ((load<u32>(d + 20) & 0x00010000u) != 0) {
        const u32 c = ctl_new(ctx, v, kViewCam);
        store<u32>(v + kViewCtl, c);
        store<u32>(c + 60, kCtlVtable);  // written even when c is 0 (as the original)
        store<u32>(c + 64, v);
    }

    // The additive camera.
    const u32 alloc = camdef_allocator(ctx, kFreeListName);
    const u32 def = static_cast<u32>(vcall(ctx, alloc, 12, {alloc}));
    if (def == 0) return;  // r3 = 0
    camdef_init(ctx, def);
    const u32 buf = frame.addr() + kBufAt;
    call(ctx, vfn::Sprintf, {buf, kAdditiveFmt, load<u32>(d + kDefName)});
    camera_make(ctx, v, def, buf, 0, kViewCamAdd);
    const u32 c = ctl_new(ctx, v, kViewCamAdd);
    store<u32>(v + kViewCtlAdd, c);
    ctx.gpr[3] = c;
}
