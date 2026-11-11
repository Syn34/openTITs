// Game code, VMX sweep part 6 (see vmx6.h): Havok shape / body setup.
#include "../sly4.h"
#include "vmx6.h"

using namespace sly4;
using namespace sly4::vmx6;

namespace {
constexpr u32 kVtC0B80 = 0x00B12048;  // the def's vtable
constexpr u32 kMasksC0B80 = 0x000C0B50;  // lane masks {y}, {z}, {w}
constexpr u32 kBoxRadiusC0B80 = 0x00F9203C;  // f32 0.05
constexpr u32 kVtTranslateC0B80 = 0x00F1E7F8;  // stored over the translate shape after its constructor
}  // namespace

// void f(Def* o, const Rec* rec) (called by XmlDef_NewC1780): the box
// volume def constructor. Thunk_000C3078(o, rec ? rec + 4 : 0); the vectors
// rec+128 / +144 / +160 into o+192 (half extents) / +208 / +224, vtable
// 0x00B12048, the words +176..+188 and bytes +192..+194 into o+244.. /
// o+260..; then, with a physics world list (World + 0x1281D4) and non-zero
// half extents: a box shape (radius 0.05) wrapped in a translate shape
// offset by {0, half y, 0, 0} (both from the Havok allocator, its u16 +4
// set to the size even when the allocation failed), a rigid body (motion
// 3, the cinfo bytes +180 = 4, +200 = 9, +204 = 1) into o+240, the shapes
// released. Returns 0 on the early exits, else what the last release
// leaves; f1 is the lfs of rec+188 until a shape constructor runs.
DECOMP(000C0B80, XmlDef_FromRecC0B80) {
    const u32 o = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    Stack frame(ctx, 400);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    call(ctx, 0x000C3078, {o, rec != 0 ? rec + 4 : 0});
    CopyW6(o + 192, rec + 128, 3);
    CopyW6(o + 208, rec + 144, 3);
    store<u32>(o, kVtC0B80);
    CopyW6(o + 224, rec + 160, 3);
    store<u32>(o + 244, load<u32>(rec + 176));
    store<u8>(o + 260, load<u8>(rec + 192));
    store<u32>(o + 248, load<u32>(rec + 180));
    store<u32>(o + 252, load<u32>(rec + 184));
    store<u8>(o + 261, load<u8>(rec + 193));
    store<u32>(o + 256, load<u32>(rec + 188));
    store<u8>(o + 262, load<u8>(rec + 194));
    ctx.fpr[1] = Lfs4(rec + 188);  // the last lfs of the copies
    if (load<u32>(load<u32>(kV3WorldPtr) + 0x1281D4u) == 0 ||
        (load<u64>(o + 192) == load<u64>(kV3Zero3) && load<u32>(o + 200) == load<u32>(kV3Zero3 + 8))) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 cinfo = sp + 128;
    call(ctx, 0x0091FB50, {cinfo});  // Hk_ShapeCinfoCtor91FB50
    const V4 hy = SplatW(o + 196);  // v31, read before the allocations
    const Masks3 mk = LoadMasks3(kMasksC0B80);
    const u32 box = HkAlloc(ctx, 48);  // stores the size at +4 even when 0 (quirk)
    if (box != 0) {
        Stvx(sp + 112, RowSelW(o + 192, o + 196, o + 200, mk));
        callf(ctx, v5fn::HkBoxShape_Ctor, {box, sp + 112}, {Lfs4(kBoxRadiusC0B80)});
    }
    u32 shape = 0;
    const u32 s = HkAlloc(ctx, 48);
    if (s != 0) {
        Stvx(sp + 112, Andc(Andc(Sel(Zero4(), hy, mk.m[0]), mk.m[1]), mk.m[2]));
        // f1 = box->+16 (the radius), read even when the box allocation failed (quirk);
        // r5 = 32 is left from the vector indexing.
        callf(ctx, 0x00898B30, {s, 10, 32, box, 1}, {Lfs4(box + 16)});  // Hk_TranslateShapeCtor_00898B30
        store<u32>(s, kVtTranslateC0B80);
        Stvx(s + 32, Andc(Lvx(sp + 112), mk.m[2]));
        store<u32>(s + 28, 0);
        shape = s;
    }
    store<u32>(cinfo + 4, shape);
    call(ctx, 0x007D4EA8, {box});  // AnimSkel_Release7D4EA8 (the reference release)
    store<u8>(cinfo + 204, 1);
    store<u32>(cinfo, 3);
    store<u8>(cinfo + 180, 4);
    store<u8>(cinfo + 200, 9);
    const u32 nb = HkAlloc(ctx, 544);
    u32 body = 0;
    if (nb != 0) {
        call(ctx, 0x0091E100, {nb, cinfo});  // Hk_ShapeCtor91E100 (rigid body)
        body = nb;
    }
    const u32 sh = load<u32>(cinfo + 4);
    store<u32>(o + 240, body);
    call(ctx, 0x007D4EA8, {sh});
}

namespace {
constexpr u32 kPoolFE050 = 0x000FE010;  // mask {y}, f32 the base z (+16), masks {z} (+32), {w} (+48)
}  // namespace

// void f(CamProbe* o) (called by CamCtrl_Construct0FF0D8, 00104310,
// 00105F90): with o->+20, o->+24 and (o->+24)->+8 set, builds a two-point
// shape (0090D190 on a 64-byte Havok allocation; points {0, 0, z, 0} with
// z the pool's +16 word, and o's +32..+40 through 0090D55C). Without a
// phantom o->+28: a 176-byte object (0090EBDC; its byte +32 = 1, written
// even when the allocation failed) set from the shape and two zero vectors
// (0090EA98), then o->+28 = 009399D0((o->+24)->+8, o->+20, o->+24, it), both
// released; with one: 0090E9FC((o->+28)->+12, shape), the shape released.
// The vector lanes are built with the pool's masks (vsel / vandc). Returns
// what the last release leaves, or o->+20 / 0 on the early exits.
DECOMP(000FE050, CamProbe_UpdateShape_000FE050) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 304);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 a = load<u32>(o + 20);
    if (a == 0 || load<u32>(o + 24) == 0 || load<u32>(load<u32>(o + 24) + 8) == 0) {
        ctx.gpr[3] = a;
        return;
    }
    const u32 world = load<u32>(load<u32>(o + 24) + 8);
    const Masks3 mk{{Lvx(kPoolFE050), Lvx(kPoolFE050 + 32), Lvx(kPoolFE050 + 48)}};
    // {0 & ~mask y, splat z under mask z, w cleared}
    auto base = [&]() { return Andc(Sel(Andc(Zero4(), mk.m[0]), SplatW(kPoolFE050 + 16), mk.m[1]), mk.m[2]); };
    const u32 ph = load<u32>(o + 28);
    if (ph == 0) {
        const u32 p = HkAlloc(ctx, 64);  // size stored at +4 even when 0 (quirk)
        u32 shape = 0;
        if (p != 0) {
            call(ctx, 0x0090D190, {p});
            shape = p;
        }
        Stvx(sp + 112, base());
        call(ctx, 0x0090D55C, {shape, sp + 112});
        Stvx(sp + 128, RowSelW(o + 32, o + 36, o + 40, mk));
        call(ctx, 0x0090D55C, {shape, sp + 128});
        const u32 q = HkAlloc(ctx, 176);
        u32 obj = 0;
        if (q != 0) {
            call(ctx, 0x0090EBDC, {q});
            obj = q;
        }
        const V4 z = Andc(Andc(Andc(Zero4(), mk.m[0]), mk.m[1]), mk.m[2]);
        store<u8>(obj + 32, 1);  // written even when the allocation failed (quirk)
        Stvx(sp + 112, z);
        Stvx(sp + 144, z);
        call(ctx, 0x0090EA98, {obj, sp + 112, sp + 144, shape});
        store<u32>(o + 28, static_cast<u32>(call(ctx, 0x009399D0, {world, load<u32>(o + 20), load<u32>(o + 24), obj})));
        call(ctx, 0x007D4EA8, {shape});
        call(ctx, 0x007D4EA8, {obj});
        return;
    }
    const u32 target = load<u32>(ph + 12);
    const u32 p = HkAlloc(ctx, 64);
    u32 shape = 0;
    if (p != 0) {
        call(ctx, 0x0090D190, {p});
        shape = p;
    }
    Stvx(sp + 160, base());
    call(ctx, 0x0090D55C, {shape, sp + 160});
    Stvx(sp + 176, RowSelW(o + 32, o + 36, o + 40, mk));
    call(ctx, 0x0090D55C, {shape, sp + 176});
    call(ctx, 0x0090E9FC, {target, shape});
    call(ctx, 0x007D4EA8, {shape});
}

namespace {
constexpr u32 kPool2108A8 = 0x00210870;  // masks {y}, {z} (+16), {w} (+32), f32 0.5 (+48)
}  // namespace

// u32 f(ShapeObj* o) (called by 0075A3AC, 0075A4FC): refreshes o's Havok
// phantom ph = o->+416. With o->+412 & 0x08000000: Hk_SetAabb924B90(ph,
// {o+144 + o+496, o+144 + o+512}). Else by the kind o->+428: 0 = a sphere
// shape of radius o->+488 (32-byte Havok allocation) replaces ph->+16
// (old one released, then f_0085F1F0(ph + 16)) and the kind is re-read; 3 =
// a box (radius 0.05) of half extents (o+512 - o+496) * 0.5 replaces it
// the same way, then HkPhantom_SetPos929FDC(ph, o+592; 0.0). The Havok
// allocations store their size at +4 even when they fail (quirk). The
// vectors are built from splats with the pool's lane masks. Returns what
// the original leaves in r3 (o without a phantom).
DECOMP(002108A8, ShapeObj_RefreshPhantom_002108A8) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 272);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 ph = load<u32>(o + 416);
    if (ph == 0) {
        ctx.gpr[3] = o;
        return;
    }
    const Masks3 mk = LoadMasks3(kPool2108A8);
    if ((load<u32>(o + 412) & 0x08000000u) != 0) {
        const f64 px = Lfs4(o + 144), py = Lfs4(o + 148), pz = Lfs4(o + 152);
        const f32 lx = Adds(px, Lfs4(o + 496)), ly = Adds(py, Lfs4(o + 500)), lz = Adds(pz, Lfs4(o + 504));
        const f32 hx = Adds(px, Lfs4(o + 512)), hy = Adds(py, Lfs4(o + 516)), hz = Adds(pz, Lfs4(o + 520));
        Stvx(sp + 144, RowSel(Fill4(lx), Fill4(ly), Fill4(lz), mk));
        Stvx(sp + 160, RowSel(Fill4(hx), Fill4(hy), Fill4(hz), mk));
        ctx.fpr[1] = hx;  // f1 as the original leaves it
        call(ctx, 0x00924B90, {ph, sp + 144});  // Hk_SetAabb924B90
        return;
    }
    u32 kind = load<u32>(o + 428);
    if (kind == 0) {
        const u32 s = HkAlloc(ctx, 32);
        u32 shape = 0;
        if (s != 0) {
            callf(ctx, v5fn::HkSphereShape_Ctor, {s}, {Lfs4(o + 488)});
            shape = s;
        }
        call(ctx, 0x007D4EA8, {load<u32>(ph + 16)});  // AnimSkel_Release7D4EA8 (the reference release)
        store<u32>(ph + 16, shape);
        call(ctx, 0x0085F1F0, {ph + 16});
        kind = load<u32>(o + 428);
    } else {
        ctx.gpr[3] = 0;  // the masked flag word
    }
    if (static_cast<s32>(kind) != 3) return;
    const f64 h = Lfs4(kPool2108A8 + 48);
    const f32 ex = Muls(Subs(Lfs4(o + 512), Lfs4(o + 496)), h);
    const f32 ey = Muls(Subs(Lfs4(o + 516), Lfs4(o + 500)), h);
    const f32 ez = Muls(Subs(Lfs4(o + 520), Lfs4(o + 504)), h);
    Stvx(sp + 128, RowSel(Fill4(ex), Fill4(ey), Fill4(ez), mk));
    const u32 b = HkAlloc(ctx, 48);
    u32 shape = 0;
    if (b != 0) {
        callf(ctx, v5fn::HkBoxShape_Ctor, {b, sp + 128}, {Lfs4(0x00F9203C)});  // radius 0.05
        shape = b;
    }
    call(ctx, 0x007D4EA8, {load<u32>(ph + 16)});
    store<u32>(ph + 16, shape);
    call(ctx, 0x0085F1F0, {ph + 16});
    Stvx(sp + 112, RowSelW(o + 592, o + 596, o + 600, mk));
    callf(ctx, 0x00929FDC, {ph, sp + 112}, {Lfs4(kPool2108A8)});  // HkPhantom_SetPos929FDC; f1 = the pool's first word (0.0)
}
