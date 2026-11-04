// World objects, part 15: small helpers.
#include "obj15.h"
#include "../file/zip.h"

using namespace sly4;
using namespace sly4::obj15;

namespace {

constexpr u32 Fn_586440 = 0x00586440;           // (bank, arg): the bank base constructor (name?)
constexpr u32 Fn_51E824 = 0x0051E824;           // () (name?)
constexpr u32 kVtblBank = 0x00B24770;
constexpr u32 kBankSize = 88;
constexpr u32 kZeroVec3 = 0x01164B40;           // f32[3] 0 (cls::kOrigin)
constexpr u32 kStrNumQuads = 0x00B24E04;        // "numQuads"
constexpr u32 kIffFlags = 12;                   // IffReader u8: 4 = already loaded once (name?)
constexpr u32 kQuadGen = 0x01193000;            // u16 (name?)
constexpr u32 kDrawRecList = 0x011DE300;        // DrawRecordList (gfx8.cpp); +0 u8 enabled
constexpr u32 kDrawRecListOnce = 0x011DE480;    // u8
constexpr u32 kViewPass = 656;                  // View: +4 a record whose +32 is the draw target (name?)
constexpr u32 kSlotRecords = 0x01193004;        // obj4.h kSlotTableBase: 128-byte records
constexpr u32 kObjSlotIdx = 284;                // u16 0xFFFF = none
constexpr u32 kObjPos80 = 80;                   // f32[3]: used without a slot (name?)

// Bank_Start's steps (name?).
constexpr u32 Fn_588FFC = 0x00588FFC, Fn_58EA70 = 0x0058EA70, Fn_595968 = 0x00595968;
constexpr u32 Fn_59541C = 0x0059541C, Fn_591A30 = 0x00591A30, Fn_58F8C4 = 0x0058F8C4;
constexpr u32 Fn_591178 = 0x00591178, Fn_58F118 = 0x0058F118, Fn_58FD50 = 0x0058FD50;
constexpr u32 Fn_5900BC = 0x005900BC, Fn_58FADC = 0x0058FADC, Fn_590DA0 = 0x00590DA0;
constexpr u32 Fn_5908A0 = 0x005908A0, Fn_58DBE8 = 0x0058DBE8;
constexpr u32 kBankListA = 0x01194EC4, kBankListB = 0x01194ED0;  // next to cls::kLightRecs (name?)

}  // namespace

// u32 FileReader_ReadVec3Swapped(FileReader* r, f32 out[3]): 12 bytes
// read into out, then each word byte-reversed in place (through the
// stack); f1 = out[2] (name?).
DECOMP(005699F0, FileReader_ReadVec3Swapped) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 f = load<u32>(r + kFrFile);
    const u32 opd = load<u32>(load<u32>(f) + kVfnFileRead);
    const u32 n = static_cast<u32>(call_ptr(ctx, opd, {f, out, 12, 0, opd, load<u32>(opd)}));
    store<u32>(r + kFrCount, load<u32>(r + kFrCount) + n);
    u32 raw[3];
    for (u32 k = 0; k < 3; ++k) {
        raw[k] = load<u32>(out + 4 * k);
        store<u32>(out + 4 * k, __builtin_bswap32(raw[k]));
    }
    ctx.fpr[1] = std::bit_cast<f32>(__builtin_bswap32(raw[2]));
    ctx.gpr[3] = n;
    ctx.gpr[4] = (raw[2] >> 8) & 0xFF;  // the bytes the original copied last
    ctx.gpr[5] = out + 8;
    ctx.gpr[6] = (raw[2] >> 16) & 0xFF;
    ctx.gpr[7] = (raw[1] >> 16) & 0xFF;
}

// const char* File_FsPath(const char* path): "<content root>/<resolved
// path>" in the File_OpenFs buffer, backslashes turned into slashes (the
// same steps File_OpenFs inlines) (name?).
DECOMP(00570604, File_FsPath) {
    const u32 path = arg<u32>(ctx, 0);
    Stack frame(ctx, 272);  // the original's local buffer at 112(r1)
    const u32 local = frame.addr();
    const u32 resolved = static_cast<u32>(call(ctx, files::fn::File_ResolvePath, {path}));
    call(ctx, files::fn::String_CopyTo, {local, resolved});
    u32 root = files::kContentPath;
    if (load<u8>(files::kBootChecked) == 0) {
        store<u8>(files::kBootChecked, 1);
        if (static_cast<s32>(call(ctx, files::fn::Game_BootCheck)) < 0) root = 0;
    }
    call(ctx, files::fn::String_Printf, {files::kFsPathBuffer, 0x00AF44E8u /* "%s/%s" */, root, local});
    const s32 len = static_cast<s32>(call(ctx, files::fn::String_Length, {files::kFsPathBuffer}));
    if (len != 0) {
        for (s32 i = len - 1;; --i) {
            if (load<u8>(files::kFsPathBuffer + static_cast<u32>(i)) == '\\')
                store<u8>(files::kFsPathBuffer + static_cast<u32>(i), '/');
            if (i == 0) break;
        }
    }
    ret(ctx, files::kFsPathBuffer);
}

// Bank* Bank_Create(arg): an 88-byte bank (Fn_586440, vtable 0x00B24770),
// marked running (+64 0x80) with object queueing and the slot table on,
// then Fn_51E824, Bank_SetState(&bank->+68) and the bank origin zeroed.
// A failed allocation goes on with a null bank, as in the original.
DECOMP(005903F4, Bank_Create) {
    const u32 a = arg<u32>(ctx, 0);
    u32 b = 0;
    const u32 p = static_cast<u32>(call(ctx, o15fn::Mem_Alloc, {kBankSize, 0, 16}));
    if (p != 0) {
        call(ctx, Fn_586440, {p, a});
        b = p;
        store<u32>(p, kVtblBank);
    }
    const u8 flags = load<u8>(b + kBankFlags);
    store<u8>(kDirtyListOff, 1);
    store<u8>(b + kBankFlags, static_cast<u8>(flags | 0x80));
    store<u8>(kSlotTableUse, 1);
    store<u8>(kRenderFlags + 8, 0);
    call(ctx, Fn_51E824);
    call(ctx, o15fn::Bank_SetState, {b + kBankState});
    for (u32 k = 0; k < 12; k += 4) store<u32>(kBankGlobals + kBgOrigin + k, load<u32>(kZeroVec3 + k));
    ctx.fpr[1] = load<f32>(kZeroVec3 + 8);
    ctx.fpr[2] = load<f32>(kZeroVec3 + 4);
    ret(ctx, b);
}

// void Bank_Start(Bank* b): clears the running bit, Bank_SetState(0),
// then the load steps (the +64 0x20 ones only when set, the two list
// steps unless 0x10), and switches queueing and the slot table back off.
DECOMP(005964B4, Bank_Start) {
    const u32 b = arg<u32>(ctx, 0);
    const u8 flags = load<u8>(b + kBankFlags);
    store<u8>(kDirtyListOff2, 0);
    store<u8>(b + kBankFlags, static_cast<u8>(flags & ~0x80));
    call(ctx, o15fn::Bank_SetState, {0});
    call(ctx, Fn_588FFC, {1});
    if ((load<u8>(b + kBankFlags) & 0x20) != 0) {
        call(ctx, Fn_58EA70, {b});
        call(ctx, Fn_595968);
        if ((load<u8>(b + kBankFlags) & 0x10) == 0) {
            call(ctx, Fn_59541C, {kBankListA});
            call(ctx, Fn_591A30, {kBankListB});
        }
        call(ctx, Fn_58F8C4, {b});
        call(ctx, Fn_591178);
        call(ctx, Fn_58F118, {b});
        call(ctx, Fn_58FD50, {b});
        call(ctx, Fn_5900BC, {b});
        call(ctx, Fn_58FADC, {b});
        call(ctx, Fn_590DA0);
    }
    call(ctx, Fn_5908A0);
    call(ctx, Fn_58DBE8, {b});
    store<u8>(kSlotTableUse, 0);
    store<u8>(kDirtyListOff, 0);
    store<u8>(kRenderFlags + 8, 1);
    ctx.gpr[3] = kSlotTableUse - 10;  // as the original leaves it
}

// bool QuadArray_Read(IffReader* r): a u8 and the u16 "numQuads"; unless
// the reader was loaded before (+12 bit 4) with the same count, the
// global quad array is freed and reallocated (112 bytes each). Returns 1.
DECOMP(0059B288, QuadArray_Read) {
    const u32 r = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 cnt = frame.addr();  // u16 at +0, the u8 at +2
    store<u16>(cnt, 0);
    call(ctx, o15fn::Iff_ReadU8, {r, cnt + 2, 0});
    call(ctx, o15fn::Iff_ReadU16, {r, cnt, kStrNumQuads});
    if ((load<u8>(r + kIffFlags) & 4) != 0 && load<u16>(kBankGlobals + kBgQuadCount) == load<u16>(cnt)) {
        ret(ctx, 1u);
        return;
    }
    const u32 g = kBankGlobals;
    const u32 old = load<u32>(g + kBgQuads);
    if (old != 0) {
        call(ctx, o15fn::Mem_FreeFrom, {old, 0});
        store<u32>(g + kBgQuads, 0);
    }
    const u32 n = load<u16>(cnt);
    store<u16>(g + kBg5200, 0);
    store<u16>(kQuadGen, 0);
    store<u16>(g + kBgQuadCount, static_cast<u16>(n));
    if (n != 0) store<u32>(g + kBgQuads, static_cast<u32>(call(ctx, o15fn::Mem_Alloc, {n * kBgQuadSize, 0, 16})));
    ret(ctx, 1u);
}

// void GeomInstance_DrawAtSlot(obj): draws the object (lod 0, 255, 15)
// with the record list disabled, after copying its slot record's +48
// position (or its own +80 without a slot) to the current view's draw
// target +64; the list is set up on first use (name?).
DECOMP(005A13A0, GeomInstance_DrawAtSlot) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(kDrawRecListOnce) == 0) {
        call(ctx, o15fn::DrawRecordList_Init, {kDrawRecList});
        store<u8>(kDrawRecListOnce, 1);
    }
    const u32 v = static_cast<u32>(call(ctx, obj11::o11fn::View_GetCurrent));
    const u32 dst = load<u32>(load<u32>(v + kViewPass) + 4 + 32);
    const u32 idx = load<u16>(o + kObjSlotIdx);
    const u32 src = idx == 0xFFFF ? o + kObjPos80 : load<u32>(kSlotRecords) + ((idx << 7) & 0xFFFFFF80u) + 48;
    store<u32>(dst + 64, load<u32>(src));
    store<u32>(dst + 68, load<u32>(src + 4));
    const u8 saved = load<u8>(kDrawRecList);
    store<u8>(kDrawRecList, 0);
    store<u32>(dst + 72, load<u32>(src + 8));
    ctx.fpr[1] = load<f32>(src + 8);
    call(ctx, o15fn::GeomInstance_DrawLod, {o, 0, 255, 15});
    store<u8>(kDrawRecList, saved);
}

// void ApeObj_Delete(ApeObj* o): the base ApeObj deleting destructor
// (vtable 0x00B24610 +4) (name?).
DECOMP(0057DDDC, ApeObj_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    obj7::ApeObjDestructBody(ctx, o);
    call(ctx, o15fn::Mem_FreeFrom, {o, 0});
}

namespace {

constexpr u32 kZeroF = 0x00577678;              // f32 0
constexpr u32 PointMap_Find = 0x0073343C;       // (map, const u32* hash) -> ApePointDef** slot (name?)
constexpr u32 SplineMap_Find = 0x007335C4;      // (map, const u32* hash) -> ApeSplineDef** slot
constexpr u32 WorldObj_FindByHash = 0x005801DC; // (hash, kind)
constexpr u32 kPointDefHolder = 0x00F8F154;     // obj5::kMapHolderPoint: +16 the map
constexpr u32 kSplineDefHolder = 0x00F8F158;    // obj8::kSplineInstList: +16 the map
constexpr u32 kHolderMap = 16;
constexpr u32 kKindPointDef = 12, kKindSplineDef = 13, kKindPoint = 24, kKindSpline = 25;  // (name?)

u32 DefFind(PPUContext& ctx, u32 holder, u32 fn, u32 hash) {
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), hash);
    const u32 slot = static_cast<u32>(call(ctx, fn, {load<u32>(load<u32>(holder) + kHolderMap), frame.addr()}));
    return load<u32>(slot);
}

constexpr u32 Quads_Prepare = 0x005792F0;       // (view +816, quads, u16 count, out, u16* out count) (name?)
constexpr u32 kQuadOut = 0x011930A0;            // +4 u16 the quad count, +16 the 64-byte matrices, +5136 u16 the count out (name?)
constexpr u32 kQuadOutRef = 0x0117F010;         // {u32 matrices, u16 count} (name?)
constexpr u32 kViewQuads = 816;

constexpr u32 Iff_ReadU32 = 0x005642AC;         // (IffReader*, u32* out, name)
constexpr u32 FileReader_ReadF32Swapped = 0x005698F4;
constexpr u32 Mesh_FindByHash = 0x004FED94;     // (hash) -> mesh (name?)
constexpr u32 Mesh_Default = 0x00526300;        // () -> mesh (name?)
constexpr u32 Mesh_Attach = 0x0050A14C;         // (mesh, x) (name?)
constexpr u32 kStrMeshHash = 0x00B24B1E, kStrSwitchDist = 0x00B24B27;
constexpr u32 kLodCount = 112, kLodEntries = 132;  // u8; {mesh, f32 switch distance}[] (name?)
constexpr u32 kVfnObj12 = 12;

}  // namespace

// bool Plane_StraddlesBox(const f32 n[3], const f32 p[3], const f32 h[3])
// The box centred on -p with half-extents h against the plane through the
// origin with normal n: the nearest corner (signs of -n) gives d0 and the
// farthest d1; true when d0 <= 0 and d1 >= 0 (name?).
DECOMP(00577680, Plane_StraddlesBox) {
    const u32 n = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), h = arg<u32>(ctx, 2);
    const f64 zero = load<f32>(kZeroF);
    f32 lo[3], hi[3];
    for (u32 i = 0; i < 3; ++i) {
        const f64 b = load<f32>(p + 4 * i);
        f64 x, y;
        if (static_cast<f64>(load<f32>(n + 4 * i)) > zero) {
            x = load<f32>(h + 4 * i);
            y = -x;
        } else {
            y = load<f32>(h + 4 * i);
            x = -y;
        }
        hi[i] = static_cast<f32>(x - b);
        lo[i] = static_cast<f32>(y - b);
    }
    const f64 n0 = load<f32>(n), n1 = load<f32>(n + 4), n2 = load<f32>(n + 8);
    u32 r = 0;
    const f64 d0 = obj9::Fmadds(n2, lo[2], obj9::Fmadds(n0, lo[0], obj9::Fmuls(n1, lo[1])));
    if (!(d0 > zero)) {
        const f64 d1 = obj9::Fmadds(n2, hi[2], obj9::Fmadds(n0, hi[0], obj9::Fmuls(n1, hi[1])));
        if (d1 > zero || d1 == zero) r = 1;
    }
    ctx.fpr[1] = zero;
    ret(ctx, r);
}

// Obj* WorldObj_FindByHashKind(u32 hash, s32 kind)
// Points / splines (kinds 24 / 12, 25 / 13) come from the point / spline
// definition maps; other kinds from WorldObj_FindByHash, falling back to
// the point map and then the spline map (name?).
DECOMP(00580388, WorldObj_FindByHashKind) {
    const u32 hash = arg<u32>(ctx, 0);
    const s32 kind = arg<s32>(ctx, 1);
    if (hash == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 r;
    if (kind == kKindSpline || kind == kKindSplineDef) {
        r = DefFind(ctx, kSplineDefHolder, SplineMap_Find, hash);
    } else if (kind == kKindPoint || kind == kKindPointDef) {
        r = DefFind(ctx, kPointDefHolder, PointMap_Find, hash);
    } else {
        r = static_cast<u32>(call(ctx, WorldObj_FindByHash, {hash, static_cast<u32>(kind), hash}));
        if (r == 0) r = DefFind(ctx, kPointDefHolder, PointMap_Find, hash);
        if (r == 0) r = DefFind(ctx, kSplineDefHolder, SplineMap_Find, hash);
    }
    ret(ctx, r);
}

// void Quads_PrepareForView(): the bank quads through Quads_Prepare for
// the current view into the 64-byte matrices at 0x011930B0, published at
// 0x0117F010 with their count, then the matrices transposed in place
// ((count + 3) & ~3) * 80 bytes' worth (name?).
DECOMP(00598A24, Quads_PrepareForView) {
    const u32 v = static_cast<u32>(call(ctx, obj11::o11fn::View_GetCurrent));
    const u32 mats = kQuadOut + 16;
    call(ctx, Quads_Prepare, {v + kViewQuads, load<u32>(kBankGlobals + kBgQuads), load<u16>(kQuadOut + 4), mats,
                              kQuadOut + 5136});
    const u32 n = load<u16>(kQuadOut + 5136);
    store<u32>(kQuadOutRef, mats);
    store<u16>(kQuadOutRef + 4, static_cast<u16>(n));
    const u32 m = (n + 3) & 0xFFFC;
    ctx.gpr[3] = m;  // as the original leaves it (then the last matrix)
    if (static_cast<s32>(m) <= 0) return;
    const u32 end = mats + m * 80;
    u32 q = mats;
    do {
        const u32 p = q;
        q += 64;
        obj8::Vec4 a = obj8::VLoad(p), b = obj8::VLoad(p + 16), c = obj8::VLoad(p + 32), d = obj8::VLoad(p + 48);
        obj8::Vec4 t[4];
        for (u32 i = 0; i < 4; ++i) t[i] = obj8::Vec4{{a.w[i], b.w[i], c.w[i], d.w[i]}};
        for (u32 i = 0; i < 4; ++i) obj8::VStore(p + 16 * i, t[i]);
        ctx.gpr[3] = p;
    } while (q < end);
}

// bool LodSet_ReadLevel(IffReader* r, LodSet* o): a u8, the u32
// "meshHash" and the f32 "switchDist"; the mesh found by hash (attached
// to o's vfunc +12 result) or the default mesh goes into o's next level
// {mesh, distance} (+112 the count, +132 the levels) (name?).
DECOMP(0059BC0C, LodSet_ReadLevel) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();  // +0 u32 hash, +4 f32 distance, +8 u8
    store<u64>(sp, 0);
    call(ctx, o15fn::Iff_ReadU8, {r, sp + 8, 0});
    call(ctx, Iff_ReadU32, {r, sp, kStrMeshHash});
    call(ctx, FileReader_ReadF32Swapped, {r, sp + 4, kStrSwitchDist});
    u32 m = static_cast<u32>(call(ctx, Mesh_FindByHash, {load<u32>(sp)}));
    if (m == 0) {
        m = static_cast<u32>(call(ctx, Mesh_Default));
    } else {
        const u32 opd = load<u32>(load<u32>(o) + kVfnObj12);
        const u32 x = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(opd)}));
        call(ctx, Mesh_Attach, {m, x});
    }
    const u32 cnt = load<u8>(o + kLodCount);
    const u32 e = o + ((cnt << 3) & ~7u) + kLodEntries;
    store<u8>(o + kLodCount, static_cast<u8>(cnt + 1));
    store<u32>(e, m);
    store<u32>(e + 4, load<u32>(sp + 4));
    ctx.fpr[1] = load<f32>(sp + 4);
    ret(ctx, 1u);
}

namespace {

constexpr u32 kRandomLast = 0x01164888;         // game/update.h: u64 the last random number's low 16 bits
constexpr u32 kRandomState = 0x01164890;        // u32
constexpr u32 kRandomScale = 0x00557560;        // f32 1/65536-ish: a 16-bit random to [0, 1) (name?)

inline u32 Rotl(u32 v, int n) { return (v << n) | (v >> (32 - n)); }
inline u32 RandomMix(u32 s) { return (Rotl(s, 4) + Rotl(s, 16)) ^ 0x87654321u; }

}  // namespace

// void Box_RandomPoint(const f32 box[8], f32 out[3])
// A random point in the box: min + (max - min) * r * scale per axis, r
// the game random's last value then two new 16-bit ones (three inlined
// steps of the generator, stored once at the end) (name?).
DECOMP(00557568, Box_RandomPoint) {
    const u32 b = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u64 last = load<u64>(kRandomLast);
    u32 x = RandomMix(load<u32>(kRandomState));
    const u32 r1 = x & 0xFFFF;
    x = RandomMix(Rotl(x, 16));
    const u32 r2 = x & 0xFFFF;
    x = RandomMix(Rotl(x, 16));
    const f64 k = load<f32>(kRandomScale);
    const f32 t0 = obj9::Fmuls(static_cast<f32>(static_cast<f64>(static_cast<s64>(last))), k);
    const f32 t1 = obj9::Fmuls(static_cast<f32>(static_cast<f64>(r1)), k);
    const f32 t2 = obj9::Fmuls(static_cast<f32>(static_cast<f64>(r2)), k);
    const f64 min0 = load<f32>(b), min1 = load<f32>(b + 4), min2 = load<f32>(b + 8);
    const f32 d0 = obj9::Fsubs(load<f32>(b + 16), min0), d1 = obj9::Fsubs(load<f32>(b + 20), min1),
              d2 = obj9::Fsubs(load<f32>(b + 24), min2);
    const u32 state = Rotl(x, 16);
    store<u32>(kRandomState, state);
    store<u64>(kRandomLast, x & 0xFFFF);
    const f32 o0 = obj9::Fmadds(d0, t0, min0), o1 = obj9::Fmadds(d1, t1, min1), o2 = obj9::Fmadds(d2, t2, min2);
    store<f32>(out, o0);
    store<f32>(out + 4, o1);
    store<f32>(out + 8, o2);
    ctx.fpr[1] = o2;
    ctx.gpr[3] = state;
}

// bool Matrix3_NearlyEqual(const f32 a[3][4], const f32 b[3][4]; f1 eps)
// |a - b| < eps for the x, y, z of the three rows (name?).
DECOMP(00554730, Matrix3_NearlyEqual) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f64 eps = ctx.fpr[1];
    u32 r = 1;
    for (u32 row = 0; row < 3 && r != 0; ++row)
        for (u32 i = 0; i < 3; ++i) {
            const u32 k = row * 16 + i * 4;
            const f64 d = std::fabs(static_cast<f64>(obj9::Fsubs(load<f32>(a + k), load<f32>(b + k))));
            ctx.fpr[2] = d;
            if (!(d < eps)) {
                r = 0;
                break;
            }
        }
    ret(ctx, r);
}

// u32 FileReader_ReadVec4Swapped(FileReader* r, f32 out[4]): 16 bytes
// read into out, each word byte-reversed in place; f1 = out[3] (name?).
DECOMP(00569AE8, FileReader_ReadVec4Swapped) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 f = load<u32>(r + kFrFile);
    const u32 opd = load<u32>(load<u32>(f) + kVfnFileRead);
    const u32 n = static_cast<u32>(call_ptr(ctx, opd, {f, out, 16, 0, opd, load<u32>(opd)}));
    store<u32>(r + kFrCount, load<u32>(r + kFrCount) + n);
    u32 raw[4];
    for (u32 k = 0; k < 4; ++k) {
        raw[k] = load<u32>(out + 4 * k);
        store<u32>(out + 4 * k, __builtin_bswap32(raw[k]));
    }
    ctx.fpr[1] = std::bit_cast<f32>(__builtin_bswap32(raw[3]));
    ctx.gpr[3] = n;
    ctx.gpr[4] = (raw[3] >> 8) & 0xFF;  // the bytes the original copied last
    ctx.gpr[5] = raw[3] >> 24;
    ctx.gpr[6] = (raw[3] >> 16) & 0xFF;
    ctx.gpr[7] = (raw[2] >> 16) & 0xFF;
}

namespace {

constexpr u32 Debug_DrawPrimPool = 0x0056B2B8;  // () (name?)
constexpr u32 kPrimPools = 0x01164E50;          // debug/debug.cpp: PrimPool*[2], +8 current, +12 previous
constexpr u32 kPrimPoolCurrent = 0x01164E58, kPrimPoolPrevious = 0x01164E5C;
constexpr u32 kMainThread = 0x011DE290, kMainThreadGuard = 0x011DE2A0;

constexpr u32 kPlaneZero = 0x0055EB4C;          // f32 {0, 1}

}  // namespace

// void Debug_FlushPrimPools(): Debug_DrawPrimPool and then the inlined
// Debug_SwapPrimPools, twice (name?).
DECOMP(0056B57C, Debug_FlushPrimPools) {
    for (u32 pass = 0; pass < 2; ++pass) {
        call(ctx, Debug_DrawPrimPool);
        const u32 second = load<u32>(kPrimPools + 4);
        const u32 cur = load<u32>(kPrimPoolCurrent);
        const u8 guard = load<u8>(kMainThreadGuard);
        store<u32>(kPrimPoolPrevious, cur);
        store<u32>(kPrimPoolCurrent, cur == second ? load<u32>(kPrimPools) : second);
        ctx.gpr[3] = guard;
        if (guard == 0) {
            store<u64>(kMainThread, 0);
            store<u64>(kMainThread + 8, 0);
            store<u8>(kMainThreadGuard, 1);
            ctx.gpr[3] = pass == 0 ? 1 : 0x011E0000;  // as the original leaves it
        }
    }
}

// void Plane_Transform(f32 plane[4], const f32 m[4][4])
// The plane {n, d} through the matrix: n' = n's rows combination (VMX),
// the point -d n transformed (with the translation row), d' = -(p' . n')
// (name?).
DECOMP(0055E940, Plane_Transform) {
    const u32 pl = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    using namespace obj8;
    const Vec4 r0 = VLoad(m), r1 = VLoad(m + 16), r2 = VLoad(m + 32), r3 = VLoad(m + 48);
    Vec4 n = VMadd(r0, VSplat(VLoadLeft(pl), 0), VZero());
    n = VMadd(r1, VSplat(VLoadLeft(pl + 4), 0), n);
    n = VMadd(r2, VSplat(VLoadLeft(pl + 8), 0), n);
    const f64 nd = -static_cast<f64>(load<f32>(pl + 12));
    const f32 px = obj9::Fmuls(load<f32>(pl), nd);
    const f32 py = obj9::Fmuls(load<f32>(pl + 4), nd);
    const f32 pz = obj9::Fmuls(load<f32>(pl + 8), nd);
    Vec4 p = VMadd(r0, VFill(std::bit_cast<u32>(px)), VZero());
    p = VMadd(r1, VFill(std::bit_cast<u32>(py)), p);
    p = VMadd(r2, VFill(std::bit_cast<u32>(pz)), p);
    p = VAdd(r3, p);
    const f64 nx = n.f(0), ny = n.f(1), nz = n.f(2);
    const f32 t = obj9::Fmuls(p.f(1), ny);
    store<u32>(pl, n.w[0]);
    store<u32>(pl + 4, n.w[1]);
    store<u32>(pl + 8, n.w[2]);
    const f32 d = -obj9::Fmadds(p.f(2), nz, obj9::Fmadds(p.f(0), nx, t));
    store<f32>(pl + 12, d);
    ctx.fpr[1] = d;
}

// bool Plane_Intersect3(f32 out[3], const f32 a[4], const f32 b[4], const f32 c[4])
// The point on the three planes {n, d} (Cramer's rule); false when the
// normals' triple product is 0 (name?).
DECOMP(0055EB58, Plane_Intersect3) {
    const u32 out = arg<u32>(ctx, 0), pa = arg<u32>(ctx, 1), pb = arg<u32>(ctx, 2), pc = arg<u32>(ctx, 3);
    using obj9::Fmuls; using obj9::Fmadds; using obj10::Fmsubs; using obj9::Lf;
    const f64 ax = Lf(pa), ay = Lf(pa + 4), az = Lf(pa + 8), aw = Lf(pa + 12);
    const f64 bx = Lf(pb), by = Lf(pb + 4), bz = Lf(pb + 8), bw = Lf(pb + 12);
    const f64 cx = Lf(pc), cy = Lf(pc + 4), cz = Lf(pc + 8), cw = Lf(pc + 12);
    const f64 bcy = Fmsubs(bz, cx, Fmuls(bx, cz));   // (b x c).y
    const f64 bcx = Fmsubs(by, cz, Fmuls(bz, cy));   // (b x c).x
    const f64 bcz = Fmsubs(bx, cy, Fmuls(by, cx));   // (b x c).z
    const f64 det = Fmadds(az, bcz, Fmadds(ax, bcx, Fmuls(ay, bcy)));
    if (det == static_cast<f64>(Lf(kPlaneZero))) {
        ctx.fpr[1] = bx;
        ret(ctx, 0u);
        return;
    }
    const f64 inv = obj9::Fdivs(Lf(kPlaneZero + 4), det);
    const f64 cax = Fmsubs(cy, az, Fmuls(cz, ay));
    const f64 cay = Fmsubs(cz, ax, Fmuls(cx, az));
    const f64 caz = Fmsubs(cx, ay, Fmuls(cy, ax));
    const f64 aby = Fmsubs(az, bx, Fmuls(ax, bz));
    const f64 abz = Fmsubs(ax, by, Fmuls(ay, bx));
    const f64 abx = Fmsubs(ay, bz, Fmuls(az, by));
    const f64 sx = Fmadds(abx, cw, Fmadds(bcx, aw, Fmuls(cax, bw)));
    const f64 sy = Fmadds(aby, cw, Fmadds(bcy, aw, Fmuls(cay, bw)));
    const f64 sz = Fmadds(abz, cw, Fmadds(bcz, aw, Fmuls(caz, bw)));
    const f32 x = Fmuls(sx, inv), y = Fmuls(sy, inv), z = Fmuls(sz, inv);
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    ctx.fpr[1] = y;
    ret(ctx, 1u);
}

namespace {

constexpr u32 WorldObj_UpdateLight = 0x005832FC, ObjLight_Apply = 0x00582A84;
constexpr u32 Iff_ChunkSize = 0x005637D4;
// The draw lists (obj14 DrawList_UpdateLightsA / B): 16-byte entries {obj,
// u8 +8 lod, +9, +10 bits 0-1 the light mode} up to the end pointer at
// header +3100; the alternate object lists are u32 obj* up to header +3216.
constexpr u32 kDlHeaderA = 0x0117E280, kDlEntriesA = 0x0116E110, kDlObjsA = 0x0117F020;
constexpr u32 kDlHeaderB = 0x0118B180, kDlEntriesB = 0x01183100, kDlObjsB = 0x0118C000;
constexpr u32 kDlEnd = 3100, kDlObjsEnd = 3216;
constexpr u32 kDlUseObjsA = 0x01192780, kDlUseObjsB = 0x01192781;  // u8 (name?)

// The two lists' light update: with the flag set every object of the
// object list through WorldObj_UpdateLight, else each draw entry's object
// by its light mode (2: ObjLight_Apply, 3: nothing, else WorldObj_UpdateLight).
void UpdateLights(PPUContext& ctx, u32 flag, u32 header, u32 entries, u32 objs) {
    if (load<u8>(flag) == 0) {
        const u32 end = load<u32>(load<u32>(header + kDlEnd));
        ctx.gpr[3] = header;  // r3 as the original leaves it
        if (entries == end) return;
        u32 e = entries;
        do {
            const u32 p = e;
            e += 16;
            const u32 o = load<u32>(p);
            ctx.gpr[3] = o;
            if (o != 0) {
                const u32 mode = load<u8>(p + 10) & 3;
                if (mode == 2) call(ctx, ObjLight_Apply, {o, mode});
                else if (mode != 3) call(ctx, WorldObj_UpdateLight, {o, mode});
            }
        } while (e < end);
        return;
    }
    const u32 end = load<u32>(load<u32>(header + kDlObjsEnd));
    ctx.gpr[3] = static_cast<u32>(static_cast<s32>(end - objs) / 4);
    if (ctx.gpr[3] == 0) return;
    u32 p = objs;
    do {
        const u32 o = load<u32>(p);
        ctx.gpr[3] = o;
        if (o != 0) call(ctx, WorldObj_UpdateLight, {o});
        p += 4;
    } while (p < end);
}

}  // namespace

// void DrawList_UpdateAllLightsA() / B(): UpdateLights for the two draw
// lists (name?).
DECOMP(00599D14, DrawList_UpdateAllLightsA) { UpdateLights(ctx, kDlUseObjsA, kDlHeaderA, kDlEntriesA, kDlObjsA); }
DECOMP(0059A008, DrawList_UpdateAllLightsB) { UpdateLights(ctx, kDlUseObjsB, kDlHeaderB, kDlEntriesB, kDlObjsB); }

// bool QuadArray_ReadFlags(IffReader* r): per bank quad a u16 whose low
// four bits become the quad's +68 u16 flags (built up one bit at a time,
// as the original stores them). Returns 1 (name?).
DECOMP(0059B51C, QuadArray_ReadFlags) {
    const u32 r = arg<u32>(ctx, 0);
    u32 q = load<u32>(kBankGlobals + kBgQuads);
    call(ctx, Iff_ChunkSize, {r});
    if (load<u16>(kQuadOut + 4) != 0) {
        Stack frame(ctx, 16);
        const u32 v = frame.addr();
        for (u32 i = 0;;) {
            call(ctx, o15fn::Iff_ReadU16, {r, v, 0});
            const u32 fl = q + 68;
            store<u16>(fl, 0);
            u32 f = (load<u16>(v) & 1) ? 1 : 0;
            const u32 n = load<u16>(kQuadOut + 4);
            store<u16>(fl, static_cast<u16>(f));
            f |= (load<u16>(v) & 2) ? 2 : 0;
            store<u16>(fl, static_cast<u16>(f));
            f |= (load<u16>(v) & 4) ? 4 : 0;
            store<u16>(fl, static_cast<u16>(f));
            f |= (load<u16>(v) & 8) ? 8 : 0;
            ++i;
            q += kBgQuadSize;
            store<u16>(fl, static_cast<u16>(f));
            if (!(i < n)) break;
        }
    }
    ret(ctx, 1u);
}

namespace {

constexpr u32 kButtonLevels = 0x0056F6A0;       // f32 {0 (up), 1 (down)}
constexpr u32 kIdButtons = 48;                  // input device f32[12] (name?)
constexpr u32 kButtonCount = 12;

constexpr u32 Box_SetPoint = 0x00556F60;        // classes.h cfn::Box_SetPoint
constexpr u32 kDefaultSphere = 0x01164BC0;      // classes.h
constexpr u32 kOneVec3 = 0x01164B30;            // obj.h

constexpr u32 kGroupMask = 0x01194570;          // u8: the enabled groups (obj10 kWorldFlagA) (name?)
constexpr u32 kGroupScales = 0x01194574;        // f32[] by the object's +4 s16 (name?)
constexpr u32 kVfnSetLevel = 8;                 // (target; f1 level) (name?)

}  // namespace

// void InputDevice_SetButtonMask(InputDevice* d, u32 mask)
// Each of the 12 button levels (+48) to 1 when its mask bit is set, else
// 0 (name?).
DECOMP(0056F6A8, InputDevice_SetButtonMask) {
    const u32 d = arg<u32>(ctx, 0), mask = arg<u32>(ctx, 1);
    const u32 up = load<u32>(kButtonLevels), down = load<u32>(kButtonLevels + 4);
    for (u32 i = 0; i < kButtonCount; ++i) store<u32>(d + kIdButtons + 4 * i, (mask >> i) & 1 ? down : up);
    ctx.fpr[1] = std::bit_cast<f32>(down);
    ctx.fpr[2] = std::bit_cast<f32>((mask >> 11) & 1 ? down : up);
    ctx.fpr[3] = std::bit_cast<f32>((mask >> 10) & 1 ? down : up);
}

namespace {

// The first half of GeomDef_Reset / PointDef_Reset: the box to the origin
// point, the default sphere, +64 / +96..104 zeroed and the +108 shared
// block released (its data freed with it at the last reference).
void DefResetBounds(PPUContext& ctx, u32 d) {
    call(ctx, Box_SetPoint, {d + 16, kZeroVec3});
    store<u32>(d + 48, load<u32>(kDefaultSphere));
    store<u32>(d + 52, load<u32>(kDefaultSphere + 4));
    const u32 sh = load<u32>(d + 108);
    store<u32>(d + 56, load<u32>(kDefaultSphere + 8));
    store<u32>(d + 96, 0);
    store<u32>(d + 60, load<u32>(kDefaultSphere + 12));
    store<u32>(d + 100, 0);
    store<u8>(d + 64, 0);
    store<u32>(d + 104, 0);
    if (sh != 0) {
        store<u32>(d + 108, 0);
        const u16 refs = static_cast<u16>(load<u16>(sh) - 1);
        store<u16>(sh, refs);
        if (static_cast<s16>(refs) <= 0) {
            const u32 data = load<u32>(sh + 4);
            if (data != 0) {
                call(ctx, o15fn::Mem_FreeFrom, {data - 16, 0});
                store<u32>(sh + 4, 0);
            }
            call(ctx, o15fn::Mem_FreeFrom, {sh, 0});
        }
    }
}

}  // namespace

// void GeomDef_Reset(ApeGeomDef* d): the box to the origin point, the
// default sphere, +64 / +96..104 zeroed, the shared block released (its
// data freed with it at the last reference), then +112 = (1, 1, 1),
// +128 0, +132 1, +136 1.0, +140 127, +144 0, +148 1.0 (name?).
DECOMP(00586148, GeomDef_Reset) {
    const u32 d = arg<u32>(ctx, 0);
    DefResetBounds(ctx, d);
    store<u32>(d + 128, 0);
    store<u32>(d + 112, load<u32>(kOneVec3));
    store<u32>(d + 132, 1);
    store<u32>(d + 116, load<u32>(kOneVec3 + 4));
    store<u32>(d + 136, 0x3F800000);
    store<u32>(d + 120, load<u32>(kOneVec3 + 8));
    store<u8>(d + 140, 127);
    store<u32>(d + 144, 0);
    store<u32>(d + 148, 0x3F800000);
    ctx.fpr[1] = load<f32>(kOneVec3 + 8);
    ctx.gpr[3] = 0x3F800000;  // as the original leaves it
}

// void GroupList_SetLevels(GroupList* l): l = {f32 level, u32 count,
// Obj** items}; for each object with a positive +4 s16 whose +120 group
// bit (1 << group) is in the enabled mask: its +116 target's vfunc +8
// with level * the group's scale (name?).
DECOMP(005902B8, GroupList_SetLevels) {
    const u32 l = arg<u32>(ctx, 0);
    u32 items = load<u32>(l + 8);
    const u32 n = load<u32>(l + 4);
    const f64 level = load<f32>(l);
    if (n == 0) return;
    for (u32 left = n - 1;; --left) {
        const u32 o = load<u32>(items);
        if (static_cast<s16>(load<u16>(o + 4)) > 0) {
            const u32 g = load<u8>(o + 120);
            const u32 sh = g & 63;
            const u32 bit = (sh < 32 ? (1u << sh) : 0u) & 0xFF;
            if ((load<u8>(kGroupMask) & bit) != 0) {
                ctx.fpr[1] = obj9::Fmuls(level, load<f32>(kGroupScales + (g << 2)));
                const u32 t = load<u32>(o + 116);
                const u32 opd = load<u32>(load<u32>(t) + kVfnSetLevel);
                call_ptr(ctx, opd, {t, opd, load<u32>(opd)});
            }
        }
        items += 4;
        if (left == 0) break;
    }
    ctx.gpr[3] = 0;  // the count left, as the original leaves it
}

namespace {

constexpr u32 Tex_Acquire = 0x00523B7C;         // (Texture*) -> AglMaterial*
constexpr u32 kMatBlendState = 104;             // material: +0 u32 flags, +4 u16, +6 u16, +12 u16 (name?)
constexpr u32 kVtblRoot = 0x00B242B8;           // obj4.h
constexpr u32 kRootDestroyHook = 0x01164CD4;    // obj4.h
constexpr u32 Mem_FreePool = 0x0056C730;        // (p, pool, tag)
constexpr u32 String_Assign = 0x00555F80;       // (String* dst, const String* src)

// The shared block release ApeGeomDef / ApeSplineDef inline: {u16 refs,
// u16, data (allocated 16 bytes in)}; the data and the block are freed
// on the last reference.
void SharedRelease(PPUContext& ctx, u32 sh) {
    const u16 refs = static_cast<u16>(load<u16>(sh) - 1);
    store<u16>(sh, refs);
    if (static_cast<s16>(refs) > 0) return;
    const u32 data = load<u32>(sh + 4);
    if (data != 0) {
        call(ctx, o15fn::Mem_FreeFrom, {data - 16, 0});
        store<u32>(sh + 4, 0);
    }
    call(ctx, o15fn::Mem_FreeFrom, {sh, 0});
}

}  // namespace

// void DebugFont_Construct(DebugFont* f, const FontData* data, Texture* tex)
// +0 the data, +4 the texture, +8 a material on it (blend flags: bit 31,
// 770 in bits 15-30 (0x01810000 with the rest), +4 0x8006, +6 771, +12
// 0), +12..+24 the data's four f32, +28 u16 the glyph count, +32 the
// glyph table (data +22), +36 the u32 after it (copied bytewise), +40 the
// data after that (name?).
DECOMP(00565410, DebugFont_Construct) {
    const u32 f = arg<u32>(ctx, 0), data = arg<u32>(ctx, 1), tex = arg<u32>(ctx, 2);
    store<u32>(f, data);
    const u32 m = static_cast<u32>(call(ctx, Tex_Acquire, {tex}));
    store<u32>(f + 8, m);
    store<u32>(f + 4, tex);
    const u32 bs = m + kMatBlendState;
    const u32 w = load<u32>(bs);
    store<u16>(bs + 6, 771);
    store<u16>(bs + 4, 0x8006);
    store<u32>(bs, ((w | 0x80000000u) & ~0x7FFF8000u) | ((770u << 15) & 0x7FFF8000u));
    store<u16>(load<u32>(f + 8) + kMatBlendState + 12, 0);
    for (u32 k = 0; k < 16; k += 4) store<u32>(f + 12 + k, load<u32>(data + 4 + k));
    const u32 n = load<u16>(data + 20);
    store<u32>(f + 32, data + 22);
    store<u16>(f + 28, static_cast<u16>(n));
    const u32 p = data + 20 + ((n << 1) & 0x1FFFE) + 4;
    u8 b[4];
    for (u32 k = 0; k < 4; ++k) b[k] = load<u8>(p + k);
    for (u32 k = 0; k < 4; ++k) store<u8>(f + 36 + k, b[k]);
    store<u32>(f + 40, p + 4);
    ctx.fpr[1] = load<f32>(data + 16);
    ctx.gpr[3] = b[3];
}

namespace {

void RootDestruct(PPUContext& ctx, u32 o) {
    const u32 w8 = load<u32>(o + 8);
    store<u32>(o, kVtblRoot);
    if (((w8 >> 6) & 0xF) != 0) {
        const u32 hook = load<u32>(kRootDestroyHook);
        if (hook != 0) call_ptr(ctx, hook, {o, hook, load<u32>(hook)});
    }
    const u32 shared = load<u32>(o + 12);
    if (shared == 0) return;
    const u16 refs = static_cast<u16>(load<u16>(shared) - 1);
    store<u16>(shared, refs);
    if (static_cast<s16>(refs) > 0) return;
    for (u32 off = 16; off >= 4; off -= 4) {
        const u32 sub = load<u32>(shared + off);
        if (sub == 0) continue;
        const u16 r = static_cast<u16>(load<u16>(sub) - 1);
        store<u16>(sub, r);
        if (static_cast<s16>(r) > 0) continue;
        call(ctx, Mem_FreePool, {load<u32>(sub + 28), 0, 0});
        call(ctx, Mem_FreePool, {sub, 0, 0});
    }
    call(ctx, Mem_FreePool, {shared, 0, 0});
}

}  // namespace

// void ApeRoot_DestructVfn(ApeRoot* o): the root vtable's slot 0, the
// same body as ApeRoot_Destruct: the destroy hook for objects with +8
// bits 6-9 set, then the +12 shared block dropped (its four sub-blocks
// and their +28 buffers freed on the last references) (name?).
DECOMP(00558A48, ApeRoot_DestructVfn) { RootDestruct(ctx, arg<u32>(ctx, 0)); }

// void GeomDef_Copy(ApeGeomDef* dst, const ApeGeomDef* src)  (vtable
// 0x00B246B0 / 0x00B246D8 +16): the box, the sphere and the name, +96..104,
// and the shared block (referenced, the old one released) (name?).
DECOMP(0058492C, GeomDef_Copy) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    for (u32 k : {16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u, 60u}) store<u32>(d + k, load<u32>(s + k));
    call(ctx, String_Assign, {d + 64, s + 64});
    store<u32>(d + 96, load<u32>(s + 96));
    const u32 nsh = load<u32>(s + 108);
    store<u32>(d + 100, load<u32>(s + 100));
    const u32 old = load<u32>(d + 108);
    store<u32>(d + 104, load<u32>(s + 104));
    if (old == nsh) return;
    store<u32>(d + 108, nsh);
    if (nsh != 0) store<u16>(nsh, static_cast<u16>(load<u16>(nsh) + 1));
    if (old != 0) SharedRelease(ctx, old);
}

namespace {

constexpr u32 SetDrawCount_00500FB4 = 0x00500FB4;  // (this): +28 = the draw count
constexpr u32 Fn_527FEC = 0x00527FEC;           // (target, rec, rec +64, u32 level) (name?)
constexpr u32 kCameraPos = 0x010DD8B0;          // gfx/render.h g::camera_pos
constexpr u32 kLodDistScale = 0x0059D570;       // f32 (name?)
constexpr u32 kVfnTarget28 = 28;                // () -> nonzero when drawable (name?)

constexpr u32 Mem_New = 0x007A3DD8;             // (size) -> p
constexpr u32 FsOp_Init = 0x00570B9C;           // (op, fs, u32 type, u64, u64, u64)
constexpr u32 Fios_CloseFile = 0x00571350;      // (fs, op, u64, u64, u64) -> bool
constexpr u32 FsOp_Destruct = 0x00570DA8;       // (op)
constexpr u32 kFsOpSize = 24, kFsOpType2 = 2;

constexpr u32 kSlotJobsWanted = 0x00F88944;     // u8 (name?)
constexpr u32 kSlotJobMin = 0x200;              // slots before the jobs are used
constexpr u32 kSlotJobChunk = 128;
constexpr u32 SlotJob_Submit = 0x0056EA8C;      // WorkQueue_Submit56E970
constexpr u32 Slots_UpdateDirect = 0x00589BC8;  // (o) (name?)
constexpr u32 kVfnPrepare = 8;                  // () (name?)

}  // namespace

// bool LodTarget_Update(Obj* o, Rec* rec)
// o->+116's draw count, then (when its vfunc +28 says it is drawable and
// rec's +48 position is inside o->+112's +56 range from the camera)
// Fn_527FEC with the level (distance * scale) - (rec +68 s8) * 512,
// clamped at 0. Returns whether it was called (name?).
DECOMP(0059D578, LodTarget_Update) {
    const u32 o = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    call(ctx, SetDrawCount_00500FB4, {load<u32>(o + 116)});
    const u32 t = load<u32>(o + 116);
    const u32 opd = load<u32>(load<u32>(t) + kVfnTarget28);
    const u32 ok = static_cast<u32>(call_ptr(ctx, opd, {t, opd, load<u32>(opd)}));
    const u32 tgt = load<u32>(o + 116);
    if (ok == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 p = rec + 48;
    const f64 dy = obj9::Fsubs(load<f32>(p + 4), load<f32>(kCameraPos + 4));
    const f64 dx = obj9::Fsubs(load<f32>(p), load<f32>(kCameraPos));
    const f64 dz = obj9::Fsubs(load<f32>(p + 8), load<f32>(kCameraPos + 8));
    const f64 r = load<f32>(load<u32>(o + 112) + 56);
    const f64 d2 = obj9::Fmadds(dz, dz, obj9::Fmadds(dx, dx, obj9::Fmuls(dy, dy)));
    const f64 r2 = obj9::Fmuls(r, r);
    if (d2 > r2 || d2 == r2) {
        ctx.fpr[1] = d2;
        ret(ctx, 0u);
        return;
    }
    const f64 d = obj10::Fsqrts(d2);
    const s32 bias = static_cast<s32>((static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(rec + 68)))) << 9) & 0xFFFFFE00u);
    const u64 fi = rt::fctiw(static_cast<f64>(obj9::Fmuls(d, load<f32>(kLodDistScale))), true);
    ctx.fpr[1] = std::bit_cast<f64>(fi);  // f1 as the original passes it on
    s32 lvl = static_cast<s32>(static_cast<u32>(fi)) - bias;
    if (lvl < 0) lvl = 0;
    call(ctx, Fn_527FEC, {tgt, rec, load<u32>(rec + 64), static_cast<u32>(lvl)});
    ret(ctx, 1u);
}

// FsOp* Fios_CloseAsync(Fs* fs, u64 a, u64 b, u64 c)
// A new 24-byte FsOp of type 2 for (a, b, c) handed to Fios_CloseFile;
// destroyed and freed (0 returned) when that fails (name?).
DECOMP(00561B34, Fios_CloseAsync) {
    const u32 fs = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4], b = ctx.gpr[5], c = ctx.gpr[6];
    const u32 op = static_cast<u32>(call(ctx, Mem_New, {kFsOpSize}));
    if (op == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, FsOp_Init, {op, fs, kFsOpType2, a, b, c});
    if (call(ctx, Fios_CloseFile, {fs, op, a, b, c}) != 0) {
        ret(ctx, op);
        return;
    }
    call(ctx, FsOp_Destruct, {op});
    call(ctx, Mem_FreePool, {op, 0, 0});
    ret(ctx, 0u);
}

// void Slots_UpdateA(Obj* o)
// With the jobs wanted and at least 512 slot records: (flag 0x01192780 set)
// o's vfunc +8, then the records submitted to the slot jobs in chunks of
// max(128, count / 32 rounded up to 32); otherwise Slots_UpdateDirect(o)
// (name?).
DECOMP(005996BC, Slots_UpdateA) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n0 = load<u16>(kSlotRecords + 4);
    const u8 use = (load<u8>(kSlotJobsWanted) != 0 && n0 >= kSlotJobMin) ? 1 : 0;
    store<u8>(kDlUseObjsA, use);
    if (use == 0) {
        call(ctx, Slots_UpdateDirect, {o});
        return;
    }
    const u32 opd = load<u32>(load<u32>(o) + kVfnPrepare);
    call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
    const u32 per = (((n0 >> 5) & 0x7FF) + 31) & 0xFFFFFFE0u;
    u32 chunk = per > kSlotJobChunk ? per : kSlotJobChunk;
    u32 recs = load<u32>(kSlotRecords);
    for (u32 n = n0; n != 0;) {
        if (chunk > n) chunk = n;
        call(ctx, SlotJob_Submit, {kDlHeaderA, recs, 0, chunk, 0x0117F010u});
        n -= chunk;
        recs += chunk << 7;
    }
}

namespace {

constexpr u32 kAttrFlagTable = 0x00AF425C;      // {u32 mask, u32 flag}[7] (name?)
constexpr u32 kAttrFlagCount = 7;
constexpr u32 kAttrAllOnes = 0x0056D0D8;        // u64 ~0
constexpr u32 kAttrDefaultOpd = 0x00F3C2D8;     // (name?)
constexpr u32 kAttrFrame = 112;                 // the original's frame: +56 the u64 copied with three unset bytes

constexpr u32 kDefaultSplineDef = 0x00F88A04;   // ApeSplineDef*: made on first use (name?)
constexpr u32 SplineDef_Construct = 0x0059ECF8; // obj8::o8fn
constexpr u32 kVfnListAlloc = 12;               // obj8.h

constexpr u32 PtrVector_Init = 0x00551598;      // (vector, capacity, heap, 0)
constexpr u32 Fn_556774 = 0x00556774;           // (item) (name?)
constexpr u32 kRegistryOnce = 0x01165020;       // u8 (name?)
constexpr u32 kRegistry = 0x01165028;           // PtrVector: +0 items, +4 cap, +6 4, +8 count, +10, +12 flags, +16
constexpr u32 kDefaultHeapPtr = 0x00F88700;     // game/load.h
constexpr u32 kDefaultHeap = 0x01165048, kDefaultHeapGuard = 0x01165040, kDefaultHeapVtbl = 0x00B24278;

}  // namespace

// void JobAttr_Build(u64 out[5], u64 a, u64 b, u64 c, u32 d, u32 e)
// out = {1 when low(a) == 1 else ~0; {u8 127 / 0x80, three bytes the
// original leaves as its stack had them, u32 0 or (hi(b) != 0) 0x00F3C2D8};
// {0, the flags of every table mask contained in hi(a), | 0x20 for e in
// 0xC0000000-0xCFFFFFFF or >= 0xE0000000}; d; {hi(c) when hi(a) has 0x20,
// 0}} (name?).
DECOMP(0056D0E0, JobAttr_Build) {
    const u32 out = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4], b = ctx.gpr[5], c = ctx.gpr[6];
    const u32 d = static_cast<u32>(ctx.gpr[7]), e = static_cast<u32>(ctx.gpr[8]);
    const u32 ahi = static_cast<u32>(a >> 32);
    u32 flags = 0;
    for (u32 i = 0; i < kAttrFlagCount; ++i) {
        const u32 m = load<u32>(kAttrFlagTable + 8 * i);
        if ((m & ahi) == m) flags |= load<u32>(kAttrFlagTable + 8 * i + 4);
    }
    const u32 w60 = (b >> 32) != 0 ? kAttrDefaultOpd : 0;
    u64 first;
    u8 b56;
    if (static_cast<u32>(a) == 1) {
        b56 = 127;
        first = 1;
    } else {
        first = load<u64>(kAttrAllOnes);
        b56 = 0x80;
    }
    if (e != 0 && ((e >= 0xC0000000u && e <= 0xCFFFFFFFu) || e >= 0xE0000000u)) flags |= 0x20;
    const u32 w80 = (ahi & 0x20) != 0 ? static_cast<u32>(c >> 32) : 0;
    // The original's stack +57..+59 are never written: read them where its frame would be.
    const u32 frame = static_cast<u32>(ctx.gpr[1]) - kAttrFrame;
    const u64 w56 = (static_cast<u64>(b56) << 56) | (static_cast<u64>(load<u8>(frame + 57)) << 48) |
                    (static_cast<u64>(load<u8>(frame + 58)) << 40) | (static_cast<u64>(load<u8>(frame + 59)) << 32) | w60;
    store<u64>(out, first);
    store<u64>(out + 8, w56);
    store<u64>(out + 16, flags);
    store<u64>(out + 24, d);
    store<u64>(out + 32, static_cast<u64>(w80) << 32);
}

// ApeSplineDef* SplineDef_GetDefault(): a spline definition from the
// ApeSplineDef free list, constructed on first use (0 if the list is
// out) and cached (name?).
DECOMP(005A0000, SplineDef_GetDefault) {
    u32 d = load<u32>(kDefaultSplineDef);
    if (d == 0) {
        const u32 alloc = obj6::ListAllocator(ctx, obj8::kSplineDefList, obj8::kSplineDefListVtbl, obj8::kStrSplineDefList,
                                              obj8::kSplineDefSize);
        const u32 opd = load<u32>(load<u32>(alloc) + kVfnListAlloc);
        const u32 m = static_cast<u32>(call_ptr(ctx, opd, {alloc, opd, load<u32>(opd)}));
        if (m != 0) {
            call(ctx, SplineDef_Construct, {m});
            d = m;
        }
        store<u32>(kDefaultSplineDef, d);
    }
    ret(ctx, d);
}

// void Registry_UpdateAll(): the static item vector (0x01165028, on the
// default heap, 64 entries) set up on first use, then Fn_556774 on each
// item from the last (name?).
DECOMP(00556DB4, Registry_UpdateAll) {
    if (load<u8>(kRegistryOnce) == 0) {
        u32 heap = load<u32>(kDefaultHeapPtr);
        if (heap == 0) {
            if (load<u8>(kDefaultHeapGuard) == 0) {
                store<u32>(kDefaultHeap, kDefaultHeapVtbl);
                store<u8>(kDefaultHeapGuard, 1);
            }
            store<u32>(kDefaultHeap, kDefaultHeapVtbl);  // constructed again in place, as in the original
            heap = kDefaultHeap;
            store<u32>(kDefaultHeapPtr, heap);
        }
        const u32 v = kRegistry;
        store<u32>(v, 0);
        u32 w = load<u32>(v + 12);
        store<u16>(v + 4, 0);
        w &= 7;
        store<u16>(v + 8, 0);
        w &= ~4u;
        store<u16>(v + 10, 0);
        w &= ~2u;
        store<u32>(v + 16, 0);
        store<u16>(v + 6, 4);
        store<u32>(v + 12, w & ~1u);
        call(ctx, PtrVector_Init, {v, 64, heap, 0});
        store<u8>(kRegistryOnce, 1);
    }
    const u32 n = load<u16>(kRegistry + 8);
    if (n == 0) return;
    for (u32 i = n - 1;; --i) {
        call(ctx, Fn_556774, {load<u32>(load<u32>(kRegistry) + i * 4)});
        if (i == 0) break;
    }
}

namespace {

constexpr u32 Fios_GetFileSize = 0x00571858;    // (fs, op, x, u64, u64, u64) -> bool
constexpr u32 kFsOpType5 = 5;

constexpr u32 Box_ContainsPoint = 0x005573D0;   // (const box*, const f32 p[3]) -> bool
constexpr u32 kZonePlaneZero = 0x0057751C;      // f32 0
constexpr u32 kZonesCount = 16, kZonesMask = 20, kZonesEntries = 32;  // obj4.h kZones: u8, u32 the result, 192-byte zones
constexpr u32 kZoneSize = 192;                  // +2 u8 bit 0: precise, +4 u32 the zone's bits, +16 box, +48 six planes

}  // namespace

// FsOp* Fios_GetFileSizeAsync(Fs* fs, u32 x, u64 a, u64 b, u64 c)
// A new 24-byte FsOp of type 5 for (a, b, c) handed to Fios_GetFileSize
// with x; destroyed and freed (0 returned) when that fails (name?).
DECOMP(00562474, Fios_GetFileSizeAsync) {
    const u32 fs = arg<u32>(ctx, 0);
    const u64 x = ctx.gpr[4], a = ctx.gpr[5], b = ctx.gpr[6], c = ctx.gpr[7];
    const u32 op = static_cast<u32>(call(ctx, Mem_New, {kFsOpSize}));
    if (op == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, FsOp_Init, {op, fs, kFsOpType5, a, b, c});
    if (call(ctx, Fios_GetFileSize, {fs, op, x, a, b, c}) != 0) {
        ret(ctx, op);
        return;
    }
    call(ctx, FsOp_Destruct, {op});
    call(ctx, Mem_FreePool, {op, 0, 0});
    ret(ctx, 0u);
}

// bool Zones_MaskForPoint(Zones* z, const f32 p[3])
// The bits (+4) of every zone whose box holds p (and, for precise zones,
// that is on the inner side of all six planes) into z->+20; ~0 when there
// is none. Returns whether any matched (name?).
DECOMP(00577520, Zones_MaskForPoint) {
    const u32 zs = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    u32 found = 0, mask = 0;
    if (load<u8>(zs + kZonesCount) != 0) {
        const f64 zero = load<f32>(kZonePlaneZero);
        u32 z = zs + kZonesEntries;
        for (u32 i = 0; i < load<u8>(zs + kZonesCount); ++i, z += kZoneSize) {
            if (call(ctx, Box_ContainsPoint, {z + 16, p}) == 0) continue;
            if ((load<u8>(z + 2) & 1) != 0) {
                const f64 px = load<f32>(p), py = load<f32>(p + 4), pz = load<f32>(p + 8);
                bool inside = true;
                for (u32 k = 0; k < 6; ++k) {
                    const u32 pl = z + 48 + 16 * k;
                    f64 d = obj9::Fmuls(load<f32>(pl + 4), py);
                    d = obj9::Fmadds(load<f32>(pl), px, d);
                    d = obj9::Fmadds(load<f32>(pl + 8), pz, d);
                    d = obj9::Fadds(d, load<f32>(pl + 12));
                    if (d < zero) {
                        inside = false;
                        break;
                    }
                }
                if (!inside) continue;
            }
            mask |= load<u32>(z + 4);
            found = 1;
        }
    }
    store<u32>(zs + kZonesMask, found != 0 ? mask : ~0u);
    ret(ctx, found);
}

namespace {

constexpr u32 kIdentity = 0x01164BD0;           // f32[4][4] (game/update.h kViewMatrix's block) (name?)

constexpr u32 kTickOnce = 0x011DE280;           // u8 (name?)
constexpr u32 kTickState = 0x011DE094;          // +0 u32 the frame, +20 / +24 u32, +32 u8 flags, +92 400 bytes (name?)
constexpr u32 kTickStatePtr = 0x011972E8;       // (name?)
constexpr u32 kTickDt = 0x010DD5A8;             // f32 (name?)
constexpr u32 kBgFrame = 5380, kBgDt = 5376;    // bank globals: u32 the last frame, f32 dt (name?)
constexpr u32 Lights_Tick = 0x0059845C, Lights_Tick2 = 0x005981D4, Lights_TickB = 0x00597158;  // (list; f1 dt) (name?)
constexpr u32 Fn_588FFC_ = 0x00588FFC;

}  // namespace

// void PointDef_Reset(Def* d): DefResetBounds, then +112 the identity's
// three rows (+112 / +128 / +144 / +160, w left alone) and +176 0 (name?).
DECOMP(0059C7B0, PointDef_Reset) {
    const u32 d = arg<u32>(ctx, 0);
    DefResetBounds(ctx, d);
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) {
            if (row == 3 && k == 2) store<u32>(d + 176, 0);
            store<u32>(d + 112 + 16 * row + 4 * k, load<u32>(kIdentity + 16 * row + 4 * k));
        }
    ctx.fpr[1] = load<f32>(kIdentity + 56);
    ctx.gpr[3] = 0;
}

// void Bank_Tick(f32 dt)
// The tick state (0x011DE094) set up on first use; when its frame moved
// on: the frame and dt kept in the bank globals (and dt at 0x010DD5A8),
// the light lists ticked with dt and Fn_588FFC(0) (name?).
DECOMP(005988C8, Bank_Tick) {
    const f64 dt = ctx.fpr[1];
    const u32 last = load<u32>(kBankGlobals + kBgFrame);
    if (load<u8>(kTickOnce) == 0) {
        const u32 t = kTickState;
        const u8 b = load<u8>(t + 32);
        store<u32>(t + 20, 0);
        store<u32>(t + 24, 0);
        store<u8>(t + 32, static_cast<u8>(b & ~0xC0));
        call(ctx, 0x00726E54u /* memset */, {t + 92, 0, 400});
        store<u32>(kTickStatePtr, t);
        store<u8>(kTickOnce, 1);
    }
    const u32 frame = load<u32>(kTickState);
    ctx.gpr[3] = frame;  // as the original leaves it
    if (last == frame) return;
    store<u32>(kBankGlobals + kBgFrame, frame);
    store<f32>(kBankGlobals + kBgDt, static_cast<f32>(dt));
    store<f32>(kTickDt, static_cast<f32>(dt));
    ctx.fpr[1] = dt;
    call(ctx, Lights_Tick, {kBankListA});
    ctx.fpr[1] = dt;
    call(ctx, Lights_Tick2, {kBankListA});
    ctx.fpr[1] = dt;
    call(ctx, Lights_TickB, {kBankListB});
    call(ctx, Fn_588FFC_, {0});
}

// void ApeRoot_Delete(ApeRoot* o): the root vtable's slot +4: RootDestruct
// and the object freed (name?).
DECOMP(00558B84, ApeRoot_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    RootDestruct(ctx, o);
    call(ctx, Mem_FreePool, {o, 0, 0});
}

namespace {

constexpr u32 kGeomInstList = 0x00F8F144;       // header*: +0 allocator, +4 capacity (obj.h kMapHolderB)
constexpr u32 kGeomInstListVtbl = 0x00B0F008;
constexpr u32 kStrGeomInstList = 0x00AF4B7E;    // "... [with T = ApeGeomInstance]"
constexpr u32 kGeomInstanceSize = 352;
constexpr u32 Obj_GetField4 = 0x00556BF0;       // (p) -> p->+4
constexpr u32 kNodeObj = 16;                    // free list node: +0 next, +16 the object
constexpr u32 kObjNameHash = 204;

constexpr u32 kObliqueConsts = 0x0056A564;      // f32 {1e-5, -1e-5, 1, -1}

}  // namespace

// ApeGeomInstance* GeomInstance_FindLiveByHash(u32 hash)
// Walks the live ApeGeomInstance nodes of the free list (made on first
// use) for the object with that name hash; 0 when none (name?).
DECOMP(005A1EBC, GeomInstance_FindLiveByHash) {
    const u32 hash = arg<u32>(ctx, 0);
    const u32 alloc = obj6::ListAllocator(ctx, kGeomInstList, kGeomInstListVtbl, kStrGeomInstList, kGeomInstanceSize);
    const u32 first = static_cast<u32>(call(ctx, Obj_GetField4, {alloc}));
    for (u32 o = first != 0 ? first + kNodeObj : 0; o != 0;) {
        if (load<u32>(o + kObjNameHash) == hash) {
            ret(ctx, o);
            return;
        }
        const u32 next = load<u32>(o - kNodeObj);
        o = next != 0 ? next + kNodeObj : 0;
    }
    ret(ctx, 0u);
}

// void Matrix_ObliqueClip(f32 m[16], const f32 plane[4])
// Turns the projection's near plane into the clip plane (Lengyel): the
// far corner q = ((sgn(px) + m[8]) / m[0], (sgn(py) + m[9]) / m[5], -1,
// (1 + m[10]) / m[14]) and column 2 = plane * (1 / (plane . q)) (name?).
DECOMP(0056A578, Matrix_ObliqueClip) {
    const u32 m = arg<u32>(ctx, 0), pl = arg<u32>(ctx, 1);
    using obj9::Fadds; using obj9::Fdivs; using obj9::Fmuls; using obj9::Fmadds;
    const f64 eps = load<f32>(kObliqueConsts), neps = load<f32>(kObliqueConsts + 4);
    const f64 one = load<f32>(kObliqueConsts + 8), mone = load<f32>(kObliqueConsts + 12);
    const f64 px = load<f32>(pl), py = load<f32>(pl + 4), pz = load<f32>(pl + 8), pw = load<f32>(pl + 12);
    const s32 sx = (px > eps ? 1 : 0) - (px < neps ? 1 : 0);
    const s32 sy = (py > eps ? 1 : 0) - (py < neps ? 1 : 0);
    const f64 qx = Fdivs(Fadds(static_cast<f32>(static_cast<f64>(sx)), load<f32>(m + 32)), load<f32>(m));
    const f64 qy = Fdivs(Fadds(static_cast<f32>(static_cast<f64>(sy)), load<f32>(m + 36)), load<f32>(m + 20));
    const f64 qw = Fdivs(Fadds(load<f32>(m + 40), one), load<f32>(m + 56));
    const f64 dot = Fmadds(pw, qw, Fmadds(pz, mone, Fmadds(px, qx, Fmuls(py, qy))));
    const f64 k = Fdivs(one, dot);
    const f32 c0 = Fmuls(px, k), c1 = Fmuls(py, k), c2 = Fmuls(pz, k), c3 = Fmuls(pw, k);
    store<f32>(m + 8, c0);
    store<f32>(m + 24, c1);
    store<f32>(m + 40, c2);
    store<f32>(m + 56, c3);
    ctx.fpr[1] = c3;
}

namespace {

constexpr u32 kVtblBankRes = 0x00B24728;        // (name?)
constexpr u32 PoolBuffer_Free = 0x005662A0;
constexpr u32 PtrVector_Clear = 0x0054EC1C, Nop = 0x0054EC18;
constexpr u32 kBgSlots = 0, kBgUsers = 32, kBgShared = 36;  // bank globals: u32[] by +56 slot, u8 users, Obj* (name?)
constexpr u32 kVfnRelease = 8;

constexpr u32 Slots_PrepareB = 0x0058B128;      // () (name?)
constexpr u32 Slots_UpdateDirectB = 0x0058B4B8; // () (name?)
constexpr u32 kSlotFirstB = 6, kSlotEnd = 8;    // slot table u16s (name?)
constexpr u32 kSlotJobMinB = 0x40;

}  // namespace

namespace {

void BankResDestruct(PPUContext& ctx, u32 r) {
    const u32 buf = load<u32>(r + 60);
    store<u32>(r, kVtblBankRes);
    if (buf != 0) {
        call(ctx, PoolBuffer_Free, {buf});
        call(ctx, o15fn::Mem_FreeFrom, {buf, 0});
        store<u32>(r + 60, 0);
    }
    const u32 slot = load<u8>(r + 56);
    if (slot != 0) store<u32>(kBankGlobals + kBgSlots + (slot - 1) * 4, 0);
    const u32 g = kBankGlobals;
    store<u8>(g + kBgUsers, static_cast<u8>(load<u8>(g + kBgUsers) - 1));
    if ((load<u8>(r + kBankFlags) & 0x40) != 0) {
        const u32 x = load<u32>(g + kBgShared);
        if (x != 0) {
            const bool last = static_cast<s16>(load<u16>(x + 4)) == 1;
            const u32 opd = load<u32>(load<u32>(x) + kVfnRelease);
            call_ptr(ctx, opd, {x, opd, load<u32>(opd)});
            if (last) store<u32>(g + kBgShared, 0);
        }
    }
    call(ctx, PtrVector_Clear, {r + 68});
    call(ctx, Nop, {r + 68});
    call(ctx, PtrVector_Clear, {r + 36});
    call(ctx, Nop, {r + 36});
}

}  // namespace

// void BankBase_Destruct(Bank* r)  (the bank base vtable 0x00B24728)
// Its +60 pool buffer freed, its bank slot (+56, 1-based) cleared, one
// user less, the shared object released (and forgotten at its last
// reference) when +64 has 0x40, then the two vectors (+68, +36) cleared
// (name?).
DECOMP(005865D8, BankBase_Destruct) { BankResDestruct(ctx, arg<u32>(ctx, 0)); }

// void Slots_UpdateB(): Slots_UpdateA's second list: the records after
// the slot table's +6 index, through the jobs (flag 0x01192781) when
// wanted and at least 64, in chunks of max(128, count / 16 rounded up to
// 16); otherwise Slots_UpdateDirectB() (name?).
DECOMP(00599E40, Slots_UpdateB) {
    const u32 diff = load<u16>(kSlotRecords + kSlotEnd) - (load<u16>(kSlotRecords + kSlotFirstB) + 1u);
    u32 n = diff & 0xFFFF;
    const u8 use = (load<u8>(kSlotJobsWanted) != 0 && n >= kSlotJobMinB) ? 1 : 0;
    store<u8>(kDlUseObjsB, use);
    if (use == 0) {
        call(ctx, Slots_UpdateDirectB);
        return;
    }
    call(ctx, Slots_PrepareB);
    const u32 first = (load<u16>(kSlotRecords + kSlotFirstB) + 1u) & 0xFFFF;
    u32 recs = first != 0xFFFF ? load<u32>(kSlotRecords) + ((first << 7) & 0x007FFF80u) : 0;
    const u32 per = (((diff >> 4) & 0xFFF) + 15) & ~15u;
    u32 chunk = per > kSlotJobChunk ? per : kSlotJobChunk;
    while (n != 0) {
        if (chunk > n) chunk = n;
        call(ctx, SlotJob_Submit, {kDlHeaderB, recs, 0, chunk, 0x0117F010u});
        n -= chunk;
        recs += chunk << 7;
    }
}

namespace {

constexpr u32 WorldObj_UpdateLod = 0x0057EE00;  // (obj, 0, 0)
constexpr u32 RenderTarget_SetMatrix = 0x005201B0;  // (target, const f32 m[4][4])
constexpr u32 View_SetFov = 0x00520368;         // (View*; f1)
constexpr u32 kViewPerms = 0x005A5E70;          // vperm masks {a0 b0 a0 b3}, {a0 a1 b2 b3}, {a0 a1 b0 b1}
constexpr u32 kObjWorld = 80;                   // f32[4][4] the world matrix (name?)
constexpr u32 kObjFov = 340;                    // f32 (name?)
constexpr u32 kVcFrame = 272;                   // the original's frame: +160 a vector it reads unset, +176 the matrix

constexpr u32 Worker_Records = 0x0056F430;      // (worker, recs, u32 count) (name?)
constexpr u32 kWorkers = 0x0118E080;            // +8 a record whose +128 is cleared, +12 u16 the worker count (low byte) (name?)
constexpr u32 kVfnWorkerGet = 52;               // (o, 1, u8 i) -> worker (name?)

constexpr u32 kPrimAllocVtbl = 0x00B24418;
constexpr u32 kStrPrimList = 0x00AF35A3;        // "... [with T = ApeDebug::Prim]"
constexpr u32 kPrimSize = 124;

}  // namespace

// void Camera_ApplyView(Obj* cam, RenderTarget* t)
// The camera's LOD / transform updated, then the view matrix (the world
// matrix's rotation transposed, translation -R^T t; the w lanes as the
// original's VMX permutes leave them) and its +340 field of view set on
// the target (name?).
DECOMP(005A5EA0, Camera_ApplyView) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    using namespace obj8;
    Stack frame(ctx, kVcFrame);  // the original's frame (its +160 is read unset)
    const u32 sp = frame.addr();
    call(ctx, WorldObj_UpdateLod, {o, 0, 0});
    const u32 m = o + kObjWorld;
    auto L = [&](u32 k) { return VLoadLeft(m + k); };
    auto mrgh = [](const Vec4& a, const Vec4& b) { return Vec4{{a.w[0], b.w[0], a.w[1], b.w[1]}}; };
    const Vec4 p0 = VLoad(kViewPerms), p1 = VLoad(kViewPerms + 16), p2 = VLoad(kViewPerms + 32);
    Vec4 a = VPerm(L(48), L(52), p0);
    Vec4 b = VPerm(L(56), VLoad(sp + 160), p0);
    Vec4 tr = VPerm(a, b, p1);
    for (u32 i = 0; i < 4; ++i) tr.w[i] = std::bit_cast<u32>(0.0f - tr.f(i));
    const Vec4 r0 = VPerm(mrgh(L(0), L(16)), mrgh(L(32), L(20)), p2);
    const Vec4 r1 = VPerm(mrgh(L(4), L(20)), mrgh(L(36), L(0)), p2);
    const Vec4 r2 = VPerm(mrgh(L(8), L(24)), mrgh(L(40), L(8)), p2);
    const f32 fov = load<f32>(o + kObjFov);
    VStore(sp + 176, r0);
    Vec4 r3 = VMadd(r0, VSplat(tr, 0), VZero());
    VStore(sp + 192, r1);
    VStore(sp + 208, r2);
    r3 = VMadd(r1, VSplat(tr, 1), r3);
    r3 = VMadd(r2, VSplat(tr, 2), r3);
    VStore(sp + 224, r3);
    call(ctx, RenderTarget_SetMatrix, {t, sp + 176});
    ctx.fpr[1] = fov;
    call(ctx, View_SetFov, {t});
}

// void Workers_SubmitRecords(Obj* o, Rec* recs, u32 n)
// Clears the work record's +128 (atomic exchange), then for each worker
// (o's vfunc +52 (o, 1, i)) hands it the n records in chunks of
// clamp(n / 32 rounded up to 32, 128, 192) (name?).
DECOMP(0059A6DC, Workers_SubmitRecords) {
    const u32 o = arg<u32>(ctx, 0), recs = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    const u32 w = load<u32>(kWorkers + 8) + 128;
    for (;;) {  // lwarx / stwcx.: *w = 0
        const u32 old = load<u32>(w);
        if (mem::cas32(w, old, 0)) break;
    }
    const s32 cnt = load<u16>(kWorkers + 12) & 0xFF;
    if (cnt <= 0) return;
    const u32 per = (((n >> 5) & 0x07FFFFFF) + 31) & ~31u;
    for (u32 i = 0; static_cast<s32>(i) < cnt; i = (i + 1) & 0xFF) {
        const u32 opd = load<u32>(load<u32>(o) + kVfnWorkerGet);
        const u32 wk = static_cast<u32>(call_ptr(ctx, opd, {o, 1, i, opd, load<u32>(opd)}));
        u32 chunk = per > 128 ? per : 128;
        if (!(chunk < 0xC0)) chunk = 192;
        u32 p = recs;
        for (u32 left = n; left != 0;) {
            if (chunk > left) chunk = left;
            call(ctx, Worker_Records, {wk, p, chunk});
            left -= chunk;
            p += chunk << 7;
        }
    }
}

// Prim* Debug_NewPrim(u32 kind, const u8 colour[4], u8 c)
// A prim from the current pool's free list (made with Mem_New on first
// use: 124-byte prims): +0 kind, +4 the colour (bytewise), +8 c. A failed
// allocation writes through a null prim, as in the original (name?).
DECOMP(005536F4, Debug_NewPrim) {
    const u32 kind = arg<u32>(ctx, 0), col = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const u32 hdr = load<u32>(kPrimPoolCurrent);
    u32 alloc = load<u32>(hdr);
    if (alloc == 0) {
        const u32 obj = static_cast<u32>(call(ctx, Mem_New, {60}));
        if (obj != 0) {
            const u32 count = load<u32>(hdr + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(obj + off, 0);
            const u16 h54 = load<u16>(obj + 54);
            for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
            store<u16>(obj + 52, 0);
            store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(obj, kPrimAllocVtbl);
            store<u32>(obj + 56, kStrPrimList);
            call(ctx, 0x00551A58u /* FreeList_Init */, {obj, kPrimSize, count, 0, 0});
        }
        alloc = obj;
        store<u32>(hdr, alloc);
    }
    const u32 opd = load<u32>(load<u32>(alloc) + kVfnListAlloc);
    const u32 p = static_cast<u32>(call_ptr(ctx, opd, {alloc, opd, load<u32>(opd)}));
    for (u32 k = 0; k < 4; ++k) store<u8>(p + 4 + k, load<u8>(col + k));
    store<u32>(p, kind);
    store<u8>(p + 8, static_cast<u8>(c));
    ret(ctx, p);
}

namespace {

constexpr u32 Archive_Ready = 0x0055E56C;       // () -> bool (name?)
constexpr u32 Archive_Unmount = 0x0055E350;     // (u16 index)
constexpr u32 Archive_Nop570B90 = 0x00570B90, Archive_Return1B = 0x00571164;
constexpr u32 kMountedArchive = 0x01164A9C;     // u16: nonzero = an archive to unmount (name?)
constexpr u32 kWorkerThreads = 0x01164DF8;      // platform.cpp: WorkerThread*[1]
constexpr u32 kWorkerThreadCount = 1;
constexpr u32 kWtStartFlag = 20, kWtCompleteFlag = 28, kWtRunning = 40;  // platform.cpp WorkerThread
constexpr u32 kPlatformStarted = 0x01164880;    // platform.cpp
constexpr u32 sc_event_flag_destroy = 83, sc_event_flag_wait = 85, sc_event_flag_clear = 118;

constexpr u32 kGcmContext = 0x010F10A0;         // CellGcmContextData*: +8 the current command pointer
constexpr u32 kGcmColorMask = 0x00040324, kGcmPolyOffsetFill = 0x00040A68;  // method headers (1 word)
constexpr u32 kGcmColorMaskAll = 0x01010101;
constexpr u32 kPolyOffsetScales = 0x0058CEFC;   // f32 {0, 65536, 2^24} (name?)

}  // namespace

// bool Platform_Shutdown()
// Once the archives are ready: the mounted archive unmounted, the worker
// thread (if running) waited for (WaitComplete, then cleared) and stopped,
// its event flags destroyed and the thread object freed; the platform
// marked stopped. Returns 1 (0 when not ready) (name?).
DECOMP(0055E7C0, Platform_Shutdown) {
    if (call(ctx, Archive_Ready) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 idx = load<u16>(kMountedArchive);
    if (idx != 0) call(ctx, Archive_Unmount, {idx});
    call(ctx, Archive_Nop570B90);
    if (call(ctx, Archive_Return1B) == 0) {
        ret(ctx, 0u);
        return;
    }
    for (u32 i = 0; i < kWorkerThreadCount; ++i) {
        const u32 slot = kWorkerThreads + ((i << 2) & 0x3FFFC);
        const u32 t = load<u32>(slot);
        if (load<u8>(t + kWtRunning) != 0) {
            if (syscall(ctx, sc_event_flag_wait, {load<u32>(t + kWtCompleteFlag), 1, 1, 0, 0}) == 0)
                syscall(ctx, sc_event_flag_clear, {load<u32>(t + kWtCompleteFlag), 0});
            store<u8>(load<u32>(slot) + kWtRunning, 0);
        }
        const u32 w = load<u32>(kWorkerThreads + i * 4);
        if (w != 0) {
            syscall(ctx, sc_event_flag_destroy, {load<u32>(w + kWtCompleteFlag)});
            syscall(ctx, sc_event_flag_destroy, {load<u32>(w + kWtStartFlag)});
            call(ctx, Mem_FreePool, {w, 0, 0});
            store<u32>(kWorkerThreads + i * 4, 0);
        }
    }
    store<u8>(kPlatformStarted, 0);
    ret(ctx, 1u);
}

// void Gcm_ResetColorMaskPolyOffset(): color mask all channels and polygon
// offset fill off. (The original's other branches - polygon offset on
// with a per-depth-format scale - are unreachable: the test it computes
// is always true.) (name?)
DECOMP(0058CF08, Gcm_ResetColorMaskPolyOffset) {
    const u32 gc = load<u32>(kGcmContext);
    const u32 cur = load<u32>(gc + 8);
    store<u32>(cur, kGcmColorMask);
    store<u32>(cur + 4, kGcmColorMaskAll);
    store<u32>(gc + 8, cur + 8);
    const u32 gc2 = load<u32>(kGcmContext);
    const u32 cur2 = load<u32>(gc2 + 8);
    store<u32>(cur2, kGcmPolyOffsetFill);
    store<u32>(cur2 + 4, 0);
    store<u32>(gc2 + 8, cur2 + 8);
    ctx.fpr[1] = ctx.fpr[2] = load<f32>(kPolyOffsetScales);
    ctx.gpr[3] = gc2;  // as the original leaves them
    ctx.gpr[4] = 0;
    ctx.gpr[5] = kGcmPolyOffsetFill;
}

// void BankBase_Delete(Bank* r): the deleting destructor (name?).
DECOMP(00586744, BankBase_Delete) {
    const u32 r = arg<u32>(ctx, 0);
    if (r == 0) return;
    BankResDestruct(ctx, r);
    call(ctx, o15fn::Mem_FreeFrom, {r, 0});
}

namespace {

constexpr u32 kAxisConsts = 0x00562C40;         // f32 1, f64 1, f64 0: the clamp
constexpr u32 kIdActive = 148;                  // u8 (name?)
constexpr u32 kTickDtField = 4;                 // the tick state's f32 dt (name?)

// The tick state's first-use set up (Bank_Tick / InputDevice_IntegrateAxes).
void TickStateInit() {
    const u32 t = kTickState;
    const u8 b = load<u8>(t + 32);
    store<u32>(t + 20, 0);
    store<u32>(t + 24, 0);
    store<u8>(t + 32, static_cast<u8>(b & ~0xC0));
}

// fsel(a, c, b): a >= 0 ? c : b (NaN gives b).
inline f64 Fsel(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }

}  // namespace

// void InputDevice_IntegrateAxes(InputDevice* d)
// When active (+148): the two axes +132 / +136 advance by their rates
// (+140 / +144) * the tick dt, clamped to [0, 1] (name?).
DECOMP(00562C58, InputDevice_IntegrateAxes) {
    const u32 d = arg<u32>(ctx, 0);
    if (load<u8>(d + kIdActive) == 0) return;
    const f64 a0 = load<f32>(d + 132), r0 = load<f32>(d + 140);
    if (load<u8>(kTickOnce) == 0) {
        TickStateInit();
        call(ctx, 0x00726E54u /* memset */, {kTickState + 92, 0, 400});
        store<u32>(kTickStatePtr, kTickState);
        store<u8>(kTickOnce, 1);
    }
    const f64 dt = load<f32>(kTickState + kTickDtField);
    const f64 one = load<f32>(kAxisConsts), hi = load<f64>(kAxisConsts + 8), lo = load<f64>(kAxisConsts + 16);
    const f64 v0 = obj9::Fmadds(r0, dt, a0);
    const f64 a1 = load<f32>(d + 136), r1 = load<f32>(d + 144);
    f64 c = static_cast<f32>(Fsel(obj9::Fsubs(one, v0), v0, hi));
    c = static_cast<f32>(Fsel(c, c, lo));
    store<f32>(d + 132, static_cast<f32>(c));
    // (the original repeats the tick state's set up here; it is always done by now)
    const f64 v1 = obj9::Fmadds(r1, dt, a1);
    f64 e = static_cast<f32>(Fsel(obj9::Fsubs(one, v1), v1, hi));
    e = static_cast<f32>(Fsel(e, e, lo));
    store<f32>(d + 136, static_cast<f32>(e));
    ctx.fpr[1] = e;
}

// u32 Pad_MapButtons(-, u32 raw): the pad's raw button bits to the game's
// button mask (name?).
DECOMP(0056F7D8, Pad_MapButtons) {
    const u32 raw = arg<u32>(ctx, 1);
    static constexpr u32 kMap[16][2] = {
        {0x0010, 0x0001}, {0x0040, 0x0002}, {0x0080, 0x0004}, {0x0020, 0x0008}, {0x0008, 0x4000}, {0x0001, 0x8000},
        {0x1000, 0x0010}, {0x4000, 0x0080}, {0x8000, 0x0020}, {0x2000, 0x0040}, {0x0400, 0x0100}, {0x0100, 0x0400},
        {0x0800, 0x0200}, {0x0200, 0x0800}, {0x0002, 0x1000}, {0x0004, 0x2000}};
    u32 m = 0;
    for (const auto& e : kMap)
        if ((raw & e[0]) != 0) m |= e[1];
    ctx.gpr[4] = (raw & 4) != 0 ? 0x2000 : 0;  // as the original leaves them
    ctx.gpr[5] = raw & 4;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = raw & 2;
    ret(ctx, m);
}

namespace {

constexpr u32 Fn_572908 = 0x00572908, Fn_572728 = 0x00572728;  // (name?)
constexpr u32 CmdBuf_Call_00537D5C = 0x00537D5C;
constexpr u32 Fn_5E30D8 = 0x005E30D8;           // (gcm context, 0, 0, 0, 0, 0, 0) (name?)
constexpr u32 Draw_InvalidateState = 0x0051279C;
constexpr u32 kCbCount = 0x01195870;            // u32 the command buffers (name?)
constexpr u32 kCbBusy = 0x01195880;             // u32: waited for (300 us sleeps) (name?)
constexpr u32 kCbArray = 0x00F88970;            // 560-byte entries: +0 / +4 begin / end, +96 the sub record (name?)
constexpr u32 kCbSize = 560;
constexpr u32 kGcmDepthMask = 0x00040A74, kGcmDepthFunc = 0x00040A6C;
constexpr u32 kGlLequal = 515;
constexpr u32 kGcmDepthMaskState = kGcmContext + 93;  // u8 (name?)
constexpr u32 sc_timer_usleep = 141;

constexpr u32 kStrNameHash = 0x00B24C73, kStrName = 0x00B24C7C, kStrBboxMin = 0x00B24C81, kStrBboxMax = 0x00B24C89;
constexpr u32 FileReader_Read32 = 0x00569C10;
constexpr u32 kRadiusConsts = 0x0059E66C;       // f32 {1e-5, 0}

}  // namespace

// void Fn_57328C(): with command buffers pending: Fn_572908, waits for
// the busy word, runs every non-empty buffer (Fn_572728 on its +96, then
// CmdBuf_Call), then resets the GCM state (Fn_5E30D8, the depth mask from
// the cached state, depth func LEQUAL) and clears the count (name?).
DECOMP(0057328C, Fn_57328C) {
    if (load<u32>(kCbCount) == 0) return;
    call(ctx, Fn_572908);
    while (load<u32>(kCbBusy) != 0) syscall(ctx, sc_timer_usleep, {300});
    u32 n = load<u32>(kCbCount);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(kCbArray) + i * kCbSize;
        if (load<u32>(e + 4) == load<u32>(e)) continue;
        call(ctx, Fn_572728, {e + 96});
        call(ctx, CmdBuf_Call_00537D5C, {e});
        n = load<u32>(kCbCount);
    }
    call(ctx, Fn_5E30D8, {load<u32>(kGcmContext), 0, 0, 0, 0, 0, 0});
    call(ctx, Draw_InvalidateState);
    const u32 gc = load<u32>(kGcmContext);
    const u32 cur = load<u32>(gc + 8);
    const u8 mask = load<u8>(kGcmDepthMaskState);
    store<u32>(gc + 8, cur + 8);
    const u32 gc2 = load<u32>(kGcmContext);
    store<u32>(cur, kGcmDepthMask);
    store<u32>(cur + 4, mask);
    const u32 cur2 = load<u32>(gc2 + 8);
    store<u32>(cur2, kGcmDepthFunc);
    store<u32>(cur2 + 4, kGlLequal);
    store<u32>(gc2 + 8, cur2 + 8);
    store<u32>(kCbCount, 0);
    call(ctx, Draw_InvalidateState);
}

// bool GeomDef_ReadHeader(IffReader* r, ApeGeomDef* d)
// A u8, "nameHash", the 32-byte "name" and the byte-swapped "bboxMin" /
// "bboxMax": the box at +16, +60 the larger corner length (0 below 1e-5),
// the name assigned at +64 and the hash at +96. Returns 1 (name?).
DECOMP(0059E678, GeomDef_ReadHeader) {
    const u32 r = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 64);
    const u32 sp = frame.addr();  // +0 hash, +4 name[32], +36 min, +48 max, +60 u8
    for (u32 k = 0; k <= 56; k += 4) store<u32>(sp + k, 0);
    call(ctx, o15fn::Iff_ReadU8, {r, sp + 60, 0});
    call(ctx, Iff_ReadU32, {r, sp, kStrNameHash});
    call(ctx, FileReader_Read32, {r, sp + 4, kStrName});
    call(ctx, 0x005699F0u /* FileReader_ReadVec3Swapped */, {r, sp + 36, kStrBboxMin});
    call(ctx, 0x005699F0u, {r, sp + 48, kStrBboxMax});
    const f64 y0 = load<f32>(sp + 40), y1 = load<f32>(sp + 52);
    const f64 x0 = load<f32>(sp + 36), x1 = load<f32>(sp + 48), z0 = load<f32>(sp + 44), z1 = load<f32>(sp + 56);
    for (u32 k : {0u, 4u, 8u}) store<u32>(d + 16 + k, load<u32>(sp + 36 + k));
    store<u32>(d + 32, load<u32>(sp + 48));
    store<u32>(d + 36, load<u32>(sp + 52));
    const f64 a = obj9::Fmadds(z0, z0, obj9::Fmadds(x0, x0, obj9::Fmuls(y0, y0)));
    const f64 b = obj9::Fmadds(z1, z1, obj9::Fmadds(x1, x1, obj9::Fmuls(y1, y1)));
    store<u32>(d + 40, load<u32>(sp + 56));
    f32 rad;
    if (a > b) rad = obj10::Fsqrts(a);
    else if (b > static_cast<f64>(load<f32>(kRadiusConsts))) rad = obj10::Fsqrts(b);
    else rad = load<f32>(kRadiusConsts + 4);
    store<f32>(d + 60, rad);
    call(ctx, String_Assign, {d + 64, sp + 4});
    store<u32>(d + 96, load<u32>(sp));
    ctx.fpr[1] = rad;
    ret(ctx, 1u);
}

namespace {

constexpr u32 kPoolLive = 4, kPoolFree = 16, kPoolFreeTail = 20, kPoolFreeCount = 24;
constexpr u32 kPoolBlocks = 28, kPoolBlocksTail = 32, kPoolBlockCount = 36, kPoolHeap = 40;  // (name?)
constexpr u32 kVfnHeapFree = 8;                 // (heap, p, 0)

constexpr u32 String_Copy = 0x00553684;         // (dst, src, size)
constexpr u32 kBonePerms = 0x00584C50;          // vperm masks {a0 b0 a0 b3}, {a0 a1 b2 b3}
constexpr u32 kBdFrame = 224;                   // the original's frame: +160 a vector it reads unset

constexpr u32 kBgCollect = 5220;                // bank globals: PtrVector of collected objects (name?)
constexpr u32 kObjLodWord = 320;                // u32: bits 18-23 the LOD group (obj14 kObjLodBits)
constexpr u32 kVfnChanged = 36;

}  // namespace

// void Pool_ReleaseEmptyBlocks(Pool* p)
// Every block (+28 list, unless its +12 bit 0 pins it) holding no live
// allocation (+4 list) has its free entries unlinked from the +16 free
// list (+20 tail, +24 count), is unlinked itself (+28 / +32, +36 count)
// and goes back to the +40 heap (vfunc +8) (name?).
DECOMP(00556774, Pool_ReleaseEmptyBlocks) {
    const u32 p = arg<u32>(ctx, 0);
    for (u32 b = load<u32>(p + kPoolBlocks); b != 0;) {
        const u32 next = load<u32>(b);
        if ((load<u32>(b + 12) & 1) == 0) {
            const u32 end = b + load<u32>(b + 8);
            u32 a = load<u32>(p + kPoolLive);
            while (a != 0 && !(a >= b && a < end)) a = load<u32>(a);
            if (a == 0) {
                for (u32 f = load<u32>(p + kPoolFree); f != 0;) {
                    const u32 fn = load<u32>(f);
                    if (f >= b && f < end) {
                        const u32 prev = load<u32>(f + 4);
                        if (prev == 0) store<u32>(p + kPoolFree, fn);
                        else store<u32>(prev, fn);
                        const u32 n2 = load<u32>(f);
                        if (n2 == 0) store<u32>(p + kPoolFreeTail, load<u32>(f + 4));
                        else store<u32>(n2 + 4, load<u32>(f + 4));
                        store<u32>(p + kPoolFreeCount, load<u32>(p + kPoolFreeCount) - 1);
                    }
                    f = fn;
                }
                const u32 prev = load<u32>(b + 4);
                if (prev == 0) store<u32>(p + kPoolBlocks, load<u32>(b));
                else store<u32>(prev, load<u32>(b));
                const u32 n2 = load<u32>(b);
                if (n2 == 0) store<u32>(p + kPoolBlocksTail, load<u32>(b + 4));
                else store<u32>(n2 + 4, load<u32>(b + 4));
                const u32 heap = load<u32>(p + kPoolHeap);
                store<u32>(p + kPoolBlockCount, load<u32>(p + kPoolBlockCount) - 1);
                const u32 opd = load<u32>(load<u32>(heap) + kVfnHeapFree);
                call_ptr(ctx, opd, {heap, b, 0, opd, load<u32>(opd)});
            }
        }
        b = next;
    }
}

// void Bone_InitFromDesc(Bone* b, BoneDesc d)  (d by value: r4..r10 and the
// caller's stack)
// +64 d's first word, +68 its name (32 bytes at +44), +100 -1, and the
// matrix: rows {d8, d24 d16 - d28 d12, d20, d28 d8 - d20 d16}, {d12, ...,
// d24, d8}, {d16, d20 d12 - d24 d8, d28, d16} and the translation -(t . rows)
// for t = d +32..+40 (the w lanes as the original's VMX leaves them)
// (name?).
DECOMP(00584C70, Bone_InitFromDesc) {
    const u32 b = arg<u32>(ctx, 0);
    using namespace obj8;
    using obj9::Fmuls; using obj10::Fmsubs;
    const u32 e = static_cast<u32>(ctx.gpr[1]) + 56;  // the caller's parameter save area
    for (u32 k = 0; k < 7; ++k) store<u64>(e + 8 * k, ctx.gpr[4 + k]);
    Stack frame(ctx, kBdFrame);  // the original's frame (its +160 is read unset)
    const u32 sp = frame.addr();
    call(ctx, String_Copy, {b + 68, e + 44, 32});
    store<u32>(b + 64, load<u32>(e));
    const f64 d20 = load<f32>(e + 20), d28 = load<f32>(e + 28), d12 = load<f32>(e + 12), d16 = load<f32>(e + 16);
    const f64 d8 = load<f32>(e + 8), d24 = load<f32>(e + 24);
    const Vec4 p0 = VLoad(kBonePerms), p1 = VLoad(kBonePerms + 16);
    Vec4 t = VPerm(VPerm(VLoadLeft(e + 32), VLoadLeft(e + 36), p0), VPerm(VLoadLeft(e + 40), VLoad(sp + 160), p0), p1);
    for (u32 i = 0; i < 4; ++i) t.w[i] = std::bit_cast<u32>(0.0f - t.f(i));
    const f32 a = Fmsubs(d24, d16, Fmuls(d28, d12));
    const f32 c = Fmsubs(d28, d8, Fmuls(d20, d16));
    const f32 g = Fmsubs(d20, d12, Fmuls(d24, d8));
    auto row = [](f64 x, f64 y, f64 z, f64 w) {
        return Vec4{{std::bit_cast<u32>(static_cast<f32>(x)), std::bit_cast<u32>(static_cast<f32>(y)),
                     std::bit_cast<u32>(static_cast<f32>(z)), std::bit_cast<u32>(static_cast<f32>(w))}};
    };
    const Vec4 r0 = row(d8, a, d20, c), r1 = row(d12, c, d24, d8), r2 = row(d16, g, d28, d16);
    Vec4 r3 = VMadd(r0, VSplat(t, 0), VZero());
    r3 = VMadd(r1, VSplat(t, 1), r3);
    VStore(b, r0);
    VStore(b + 16, r1);
    VStore(b + 32, r2);
    store<u16>(b + 100, 0xFFFF);
    r3 = VMadd(r2, VSplat(t, 2), r3);
    VStore(b + 48, r3);
    ctx.fpr[1] = g;
    ctx.gpr[3] = b + 16;  // as the original leaves it
}

// void Obj_CollectLodGroup(Obj* o, u8 recurse)
// For kinds 8-14: an object with +321 bit 1 of kind 8 or 10 gets LOD group
// 12 (vfunc +36 when it changes) and is pushed on the bank's collect
// vector (kind 10 also marks its definition +120); with recurse the
// children follow (name?).
DECOMP(00590FEC, Obj_CollectLodGroup) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 rec = arg<u32>(ctx, 1) & 0xFF;
    const s32 k = static_cast<s32>(obj4::Kind(o));
    if (k < 8 || k > 14) return;
    const u32 w = load<u32>(o + kObjLodWord);
    if (((load<u8>(o + 321) >> 1) & 1) == 1) {
        const u32 k2 = obj4::Kind(o);
        if (k2 == 8 || k2 == 10) {
            if (((w >> 18) & 0x3F) != 12) {
                store<u32>(o + kObjLodWord, (w & ~0x00FC0000u) | 0x00300000u);
                const u32 opd = load<u32>(load<u32>(o) + kVfnChanged);
                call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
            }
            const u32 v = kBankGlobals + kBgCollect;
            if (load<u16>(v + 8) == load<u16>(v + 4))
                call(ctx, obj4::o4fn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + 4)) << 1) & 0x1FFFE});
            const u32 cnt = load<u16>(v + 8);
            store<u32>(load<u32>(v) + ((cnt << 2) & 0x3FFFC), o);
            const u32 n = load<u16>(v + 8) + 1u;
            const u32 peak = load<u16>(v + 10);
            const u32 def = load<u32>(o + 208);
            store<u16>(v + 8, static_cast<u16>(n));
            store<u16>(v + 10, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n & 0xFFFF) ? peak : n));
            if (obj4::Kind(o) == 10) store<u8>(def + 120, 1);
        }
    }
    if (rec == 0) return;
    for (u32 c = load<u32>(o + obj4::kObjFirst); c != 0; c = load<u32>(c + obj4::kObjNext))
        call(ctx, 0x00590FECu, {c, rec});
}

namespace {

constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 Obj577200_Construct = 0x0057711C;
constexpr u32 kBankSlots = 8;                   // bank globals +0: Bank*[8]
constexpr u32 kSharedObjSize = 6176;

constexpr u32 kStrBboxMinA = 0x00B24C14, kStrBboxMaxA = 0x00B24C1C, kStrNameHashA = 0x00B24C24;
constexpr u32 kStrAnimHash = 0x00B24C2D, kStrLightGroup = 0x00B24C36, kStrNameA = 0x00B24C41;
constexpr u32 Box_BoundingSphere = 0x00557870;  // (f32 out[4], const Box*)

}  // namespace

// void BankBase_Construct(Bank* b, u32 flags)
// The bank base (vtable 0x00B24728): +36 a 16384-entry vector (+42 4),
// +65 255, +68 a second vector (+74 4), the first free bank slot (1-based
// at +56, 0 if none), +64 flags: 0x40 = flags bit 0, 0x20 unless flags
// bit 1, 0x10 = flags bit 2 (0x80 cleared); with 0x40 the shared object
// (6176 bytes, Obj577200) is made once; one user more, +60 0 (name?).
DECOMP(00586440, BankBase_Construct) {
    const u32 b = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 1);
    store<u32>(b, kVtblBankRes);
    call(ctx, PtrVector_Construct, {b + 36});
    store<u16>(b + 42, 4);
    call(ctx, PtrVector_Init, {b + 36, 16384, 0, 0});
    const u8 f0 = load<u8>(b + kBankFlags);
    store<u8>(b + 65, 255);
    store<u8>(b + kBankFlags, static_cast<u8>(f0 & ~0x80));
    call(ctx, PtrVector_Construct, {b + 68});
    store<u16>(b + 74, 4);
    const u32 g = kBankGlobals;
    u32 slot = 0;
    for (u32 i = 0; i < kBankSlots; ++i)
        if (load<u32>(g + kBgSlots + 4 * i) == 0) {
            store<u32>(g + kBgSlots + 4 * i, b);
            slot = i + 1;
            break;
        }
    u32 f = load<u8>(b + kBankFlags);
    store<u8>(b + 56, static_cast<u8>(slot));
    f = (f & ~0x70u) | ((flags & 1) << 6) | ((flags & 2) == 0 ? 0x20 : 0) | ((flags & 4) != 0 ? 0x10 : 0);
    store<u8>(b + kBankFlags, static_cast<u8>(f));
    if ((f & 0x40) != 0 && load<u32>(g + kBgShared) == 0) {
        const u32 m = static_cast<u32>(call(ctx, o15fn::Mem_Alloc, {kSharedObjSize, 0, 16}));
        if (m != 0) call(ctx, Obj577200_Construct, {m});
        store<u32>(g + kBgShared, m);
    }
    store<u8>(g + kBgUsers, static_cast<u8>(load<u8>(g + kBgUsers) + 1));
    store<u32>(b + 60, 0);
    ret(ctx, 0u);
}

// bool GeomDef_ReadHeaderAnim(IffReader* r, Def* d)
// A u8, the byte-swapped "bboxMin" / "bboxMax", "nameHash", "animHash",
// "lightGroup" and the 32-byte "name": the box at +16, its bounding
// sphere at +48, the name at +64, +96 / +100 / +104 the three words.
// Returns 1 (name?).
DECOMP(0059DCC0, GeomDef_ReadHeaderAnim) {
    const u32 r = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    Stack frame(ctx, 96);
    const u32 sp = frame.addr();  // +0 sphere, +16 min, +28 max, +40 / +44 / +48 hashes, +52 name, +84 u8
    call(ctx, 0x00726E54u /* memset */, {sp + 16, 0, 68});
    call(ctx, o15fn::Iff_ReadU8, {r, sp + 84, 0});
    call(ctx, 0x005699F0u /* FileReader_ReadVec3Swapped */, {r, sp + 16, kStrBboxMinA});
    call(ctx, 0x005699F0u, {r, sp + 28, kStrBboxMaxA});
    call(ctx, Iff_ReadU32, {r, sp + 40, kStrNameHashA});
    call(ctx, Iff_ReadU32, {r, sp + 44, kStrAnimHash});
    call(ctx, Iff_ReadU32, {r, sp + 48, kStrLightGroup});
    call(ctx, FileReader_Read32, {r, sp + 52, kStrNameA});
    for (u32 k = 0; k < 12; k += 4) store<u32>(d + 16 + k, load<u32>(sp + 16 + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(d + 32 + k, load<u32>(sp + 28 + k));
    call(ctx, Box_BoundingSphere, {sp, d + 16});
    for (u32 k = 0; k < 16; k += 4) store<u32>(d + 48 + k, load<u32>(sp + k));
    ctx.fpr[1] = load<f32>(sp + 12);
    call(ctx, String_Assign, {d + 64, sp + 52});
    store<u32>(d + 96, load<u32>(sp + 40));
    store<u32>(d + 100, load<u32>(sp + 44));
    store<u32>(d + 104, load<u32>(sp + 48));
    ret(ctx, 1u);
}

namespace {

// One GCM command: header and its words at the context's current pointer.
void GcmEmit(std::initializer_list<u32> words) {
    const u32 gc = load<u32>(kGcmContext);
    const u32 cur = load<u32>(gc + 8);
    u32 k = 0;
    for (u32 w : words) store<u32>(cur + 4 * k++, w);
    store<u32>(gc + 8, cur + 4 * k);
}

constexpr u32 kGcmBlendEnable = 0x00040310, kGcmBlendFactors = 0x00080314, kGcmBlendEquation = 0x00040320;
constexpr u32 kGcmStencil = 0x00040A70;         // (name?)
constexpr u32 kGlLess = 514;

constexpr u32 kRayConsts = 0x005786B8;          // f32 {FLT_MAX, 0}
constexpr u32 kFltMax = 0x7F7FFFFF;

constexpr u32 EmitterDef_New = 0x0058E140;      // (props, system) -> ApeEmitterDef
constexpr u32 Iff_ReadChunks = 0x00569388;      // (reader, obj)
constexpr u32 Array_Reserve = 0x00558F3C;       // (Array*, u16 capacity)
constexpr u32 kEmitterDefHolder = 0x00F8F138;   // obj6.h kEmitterDefList: +8 callback OPD, +12 its argument, +16 the map
constexpr u32 kEmitPropsHolder = 0x00F8F128;    // obj6.h kEmitPropsList: +8 / +12 the chunk handler and its object
constexpr u32 kEmitterChunkOpd = 0x00F3C788;    // (name?)

}  // namespace

// void Gcm_ResetDepthBlendState()
// Fn_5E30D8(context, 2 x6), then: 0x0A70 0, depth mask (the cached bit),
// depth mask (the cached byte), depth func LESS, blend on, blend factors
// {0x0302, 0x0302, 0x0303, 0x0303}, blend equation 0x8006 / 0x8006 and
// blend on again (name?).
DECOMP(00572908, Gcm_ResetDepthBlendState) {
    call(ctx, Fn_5E30D8, {load<u32>(kGcmContext), 2, 2, 2, 2, 2, 2});
    GcmEmit({kGcmStencil, 0});
    GcmEmit({kGcmDepthMask, (load<u8>(kGcmDepthMaskState) & 1) != 0 ? 1u : 0u});
    GcmEmit({kGcmDepthMask, load<u8>(kGcmDepthMaskState)});
    GcmEmit({kGcmDepthFunc, kGlLess});
    GcmEmit({kGcmBlendEnable, 1});
    GcmEmit({kGcmBlendFactors, 0x03020302, 0x03030303});
    GcmEmit({kGcmBlendEquation, 0x80068006u});
    GcmEmit({kGcmBlendEnable, 1});
    ctx.gpr[3] = kGcmBlendEnable;  // as the original leaves it
}

// bool Planes_ClipRay(const f32 planes[6][4], const f32 p[3], const f32 d[3], f32* t0, f32* t1)
// The ray p + t d against six planes (n . x + w): both ts start at
// FLT_MAX (the original's choice: the entering t can then never grow);
// planes facing the ray raise *t0, the others lower *t1. False as soon as
// the ray is outside a parallel plane or *t0 > *t1; at the end true when
// either t is >= 0 (name?).
DECOMP(005786C0, Planes_ClipRay) {
    u32 pl = arg<u32>(ctx, 0);
    const u32 p = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2), pt0 = arg<u32>(ctx, 3), pt1 = arg<u32>(ctx, 4);
    using obj9::Fmuls; using obj9::Fmadds;
    store<u32>(pt0, kFltMax);
    store<u32>(pt1, kFltMax);
    const f64 zero = load<f32>(kRayConsts + 4);
    f64 t1 = load<f32>(kRayConsts);
    f64 t0 = load<f32>(pt0);
    f64 lo = t0, hi = t1;
    for (u32 i = 0; i < 6; ++i, pl += 16) {
        const f64 nx = load<f32>(pl), ny = load<f32>(pl + 4), nz = load<f32>(pl + 8);
        const f64 den = Fmadds(nz, load<f32>(d + 8), Fmadds(nx, load<f32>(d), Fmuls(ny, load<f32>(d + 4))));
        const f64 np = Fmadds(nz, load<f32>(p + 8), Fmadds(nx, load<f32>(p), Fmuls(ny, load<f32>(p + 4))));
        const f64 w = -static_cast<f64>(load<f32>(pl + 12));
        if (den == zero) {
            if (np > w) {
                ctx.fpr[1] = t1;
                ret(ctx, 0u);
                return;
            }
        } else {
            const f32 t = obj9::Fdivs(obj9::Fsubs(w, np), den);
            if (den < zero) {
                if (t > lo) {
                    store<f32>(pt0, t);
                    t0 = t;
                    t1 = load<f32>(pt1);
                }
            } else if (t < hi) {
                store<f32>(pt1, t);
                t1 = t;
                t0 = load<f32>(pt0);
            }
        }
        lo = t0;
        hi = t1;
        if (lo > t1) {
            ctx.fpr[1] = t1;
            ret(ctx, 0u);
            return;
        }
    }
    ctx.fpr[1] = t1;
    ret(ctx, (t0 > zero || t0 == zero || t1 > zero || t1 == zero) ? 1u : 0u);
}

// bool EmitterDef_Read(IffReader* r)
// A new emitter definition, the definition list's callback (def, arg, 0)
// before and (def, arg, 1) after its chunks are read (the props holder
// pointing the chunk handler at it meanwhile); then added to the list's
// {name hash -> def} table (re-sorted flag set) and flagged 0x400.
// Returns 1 (name?).
DECOMP(0059DB28, EmitterDef_Read) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 d = static_cast<u32>(call(ctx, EmitterDef_New, {0, 0}));
    auto notify = [&](u32 phase) {
        const u32 h = load<u32>(kEmitterDefHolder);
        const u32 cb = load<u32>(h + 8);
        if (cb != 0) call_ptr(ctx, cb, {d, load<u32>(h + 12), phase, cb, load<u32>(cb)});
    };
    notify(0);
    store<u32>(load<u32>(kEmitPropsHolder) + 8, kEmitterChunkOpd);
    store<u32>(load<u32>(kEmitPropsHolder) + 12, d);
    call(ctx, Iff_ReadChunks, {r, d});
    store<u32>(load<u32>(kEmitPropsHolder) + 8, 0);
    store<u32>(load<u32>(kEmitPropsHolder) + 12, 0);
    notify(1);
    const u32 v = load<u32>(load<u32>(kEmitterDefHolder) + 16);
    const u32 key = load<u32>(d + 96);
    if (load<u16>(v + 2) == load<u16>(v)) call(ctx, Array_Reserve, {v, (static_cast<u32>(load<u16>(v)) << 1) & 0xFFFE});
    const u32 ents = load<u32>(v + 8);
    const u32 n = load<u16>(v + 2);
    store<u32>(ents + ((n << 3) & ~7u), key);
    const u32 n1 = n + 1;
    const u32 peak = load<u16>(v + 4);
    store<u16>(v + 2, static_cast<u16>(n1));
    store<u32>(ents + ((n << 3) & ~7u) + 4, d);
    const u32 fl = load<u16>(v + 6);
    store<u16>(v + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n1 & 0xFFFF) ? peak : n1));
    store<u16>(v + 6, static_cast<u16>(fl | 1));
    store<u32>(d + 8, load<u32>(d + 8) | 0x400);
    ret(ctx, 1u);
}

namespace {

constexpr u32 StrBuf_Grow = 0x00566950;         // (buf, s32 capacity) (name?)
constexpr u32 kSbData = 0, kSbLen = 4, kSbCap = 8, kSbGrowable = 12, kSbCount = 16;  // (name?)

constexpr u32 Listener_LocalDirection = 0x005532E4;  // (f32 out[4], const f32 p[3], const Matrix*)
constexpr u32 Spline_Nearest = 0x00560A40;      // (SplineData*, const f32 p[3], u32* seg, f32* dist) -> f1
constexpr u32 kNearestInit = 0x005A8F54;        // f32 {FLT_MAX, 0}
constexpr u32 kSdSplineData = 112;              // ApeSplineDef +112 (obj14 kSdSpline)

constexpr u32 FsHandle_Construct = 0x00571D80, FsHandle_Destruct = 0x00571D98;
constexpr u32 Fios_OpenFile = 0x00571178;       // (op, handle, fs, path, u64, u64, u64) -> bool
constexpr u32 kFsOpOpen = 1;

}  // namespace

// char* StrBuf_Append(StrBuf* b, const char* s)
// Appends s (growing to twice the capacity when +12 allows, else
// truncating) and terminates it; +16 counts the appended strings. Returns
// where s went, or 0 when it had to be cut (the last byte terminated)
// (name?).
DECOMP(00566F1C, StrBuf_Append) {
    const u32 b = arg<u32>(ctx, 0);
    u32 s = arg<u32>(ctx, 1);
    const u32 start = load<u32>(b + kSbLen);
    u32 len = start;
    while (load<u8>(s) != 0) {
        const u32 cap = load<u32>(b + kSbCap);
        if (!(static_cast<s32>(len) < static_cast<s32>(cap - 1))) {
            if (load<u8>(b + kSbGrowable) == 0) break;
            call(ctx, StrBuf_Grow, {b, static_cast<u64>(static_cast<s64>(static_cast<s32>(cap << 1)))});
            len = load<u32>(b + kSbLen);
        }
        store<u8>(load<u32>(b + kSbData) + static_cast<u32>(static_cast<s32>(len)), load<u8>(s));
        ++s;
        len = load<u32>(b + kSbLen) + 1;
        store<u32>(b + kSbLen, len);
    }
    const u32 cap = load<u32>(b + kSbCap);
    if (!(static_cast<s32>(len) < static_cast<s32>(cap))) {
        store<u8>(load<u32>(b + kSbData) + cap - 1, 0);
        ret(ctx, 0u);
        return;
    }
    store<u8>(load<u32>(b + kSbData) + len, 0);
    store<u32>(b + kSbLen, load<u32>(b + kSbLen) + 1);
    store<u32>(b + kSbCount, load<u32>(b + kSbCount) + 1);
    ret(ctx, load<u32>(b + kSbData) + start);
}

// s32 Splines_FindNearest(PtrVector* splines, const f32 p[3], f32* t, f32* dist, Opd* filter, arg)
// The spline (accepted by filter(spline, arg) when given) whose curve
// comes nearest p (in its local space); -1 when none. Its parameter and
// distance go to t / dist when given (FLT_MAX / 0 otherwise) (name?).
DECOMP(005A8F60, Splines_FindNearest) {
    const u32 v = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), pt = arg<u32>(ctx, 2), pd = arg<u32>(ctx, 3);
    const u32 filter = arg<u32>(ctx, 4), farg = arg<u32>(ctx, 5);
    Stack frame(ctx, 32);
    const u32 local = frame.addr(), dist = frame.addr() + 16;
    f64 best_d = load<f32>(kNearestInit), best_t = load<f32>(kNearestInit + 4);
    s32 best = -1;
    for (u32 i = 0; i < load<u16>(v + 8); ++i) {
        const u32 s = load<u32>(load<u32>(v) + 4 * i);
        if (s == 0) continue;
        if (filter != 0 && call_ptr(ctx, filter, {s, farg, load<u32>(filter)}) == 0) continue;
        call(ctx, Listener_LocalDirection, {local, p, s + 80});
        const f64 t = callf(ctx, Spline_Nearest, {load<u32>(s + 208) + kSdSplineData, local, 0, dist}, {});
        const f64 d = load<f32>(dist);
        ctx.fpr[1] = d;  // as the original leaves it
        ctx.fpr[2] = t;
        if (d < best_d) {
            best = static_cast<s32>(i);
            best_d = d;
            best_t = t;
        }
    }
    if (pt != 0) store<f32>(pt, static_cast<f32>(best_t));
    if (pd != 0) store<f32>(pd, static_cast<f32>(best_d));
    ret(ctx, best);
}

// FsOp* Fios_OpenAsync(Fs* fs, const char* path, u64 a, u64 b, u64 c)
// A new file handle and a 24-byte FsOp of type 1 handed to Fios_OpenFile;
// 0 when an allocation or the open fails (the handle is then leaked, as
// in the original) (name?).
DECOMP(00561714, Fios_OpenAsync) {
    const u32 fs = arg<u32>(ctx, 0), path = arg<u32>(ctx, 1);
    const u64 a = ctx.gpr[5], b = ctx.gpr[6], c = ctx.gpr[7];
    const u32 h = static_cast<u32>(call(ctx, Mem_New, {4}));
    if (h == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, FsHandle_Construct, {h});
    const u32 op = static_cast<u32>(call(ctx, Mem_New, {kFsOpSize}));
    if (op == 0) {
        call(ctx, FsHandle_Destruct, {h});
        call(ctx, Mem_FreePool, {h, 0, 0});
        ret(ctx, 0u);
        return;
    }
    call(ctx, FsOp_Init, {op, h, kFsOpOpen, a, b, c});
    if (call(ctx, Fios_OpenFile, {op, h, fs, path, a, b, c}) != 0) {
        ret(ctx, op);
        return;
    }
    call(ctx, FsOp_Destruct, {op});
    call(ctx, Mem_FreePool, {op, 0, 0});
    ret(ctx, 0u);
}

namespace {

constexpr u32 HashMap_Find2 = 0x007399A4;       // (map, const u32* key) -> value slot
constexpr u32 HashMap_Remove2 = 0x007A7C74;     // (map, const u32* key) (name?)
constexpr u32 kVfnDestruct = 0;
constexpr u32 kDefRegistered = 0x400;           // +8: in its list's name map

}  // namespace

// void LightRecList_Destroy(RecList** pp)
// Removes every 80-byte record from the last (cls::RemoveRecord: owners'
// +382 index to 0xFFFF, the rest renumbered), frees the data and the list
// and clears *pp (name?).
DECOMP(005965BC, LightRecList_Destroy) {
    const u32 pp = arg<u32>(ctx, 0);
    const u32 l = load<u32>(pp);
    if (l == 0) return;
    while (load<u16>(l + 4) != 0) {
        const u32 last = (load<u16>(l + 4) - 1u) & 0xFFFF;
        const u32 rec = last == 0xFFFF ? 0 : load<u32>(l) + last * cls::kLightRecSize;
        const u32 idx = load<u16>(load<u32>(rec + 68) + cls::kLightRec);
        cls::RemoveRecord(ctx, l, idx, cls::kLightRecSize, 4, 68, cls::kLightRec);
    }
    const u32 data = load<u32>(l);
    if (data != 0) {
        call(ctx, o15fn::Mem_FreeFrom, {data, 0});
        store<u32>(l, 0);
    }
    call(ctx, o15fn::Mem_FreeFrom, {l, 0});
    store<u32>(pp, 0);
    ctx.gpr[3] = 0;
}

namespace {

// The ApeTMemClassFL<T> destroy the definition and instance types share:
// off the type's name map when registered (+8 0x400, and still the map's
// entry for its +key_off hash), destructed (vfunc +0) and given back to
// the type's free list (made on first use).
struct DefType {
    u32 hdr, vtbl, name, size, find, remove, key_off;
};

void DefDestroy(PPUContext& ctx, u32 d, const DefType& t) {
    if ((load<u32>(d + 8) & kDefRegistered) != 0) {
        Stack frame(ctx, 16);
        const u32 key = frame.addr();
        store<u32>(key, load<u32>(d + t.key_off));
        const u32 slot = static_cast<u32>(call(ctx, t.find, {load<u32>(load<u32>(t.hdr) + 16), key}));
        if (load<u32>(slot) == d) {
            store<u32>(key + 4, load<u32>(d + t.key_off));
            call(ctx, t.remove, {load<u32>(load<u32>(t.hdr) + 16), key + 4});
        }
        store<u32>(d + 8, load<u32>(d + 8) & ~kDefRegistered);
    }
    const u32 opd = load<u32>(load<u32>(d) + kVfnDestruct);
    call_ptr(ctx, opd, {d, opd, load<u32>(opd)});
    const u32 alloc = obj6::ListAllocator(ctx, t.hdr, t.vtbl, t.name, t.size);
    obj6::ListFree(ctx, alloc, d);
}

constexpr DefType kGeomDefType{cls::kGeomDefList, cls::kGeomDefListVtbl, cls::kStrGeomDefList, cls::kGeomDefSize,
                               HashMap_Find2, HashMap_Remove2, 96};
constexpr DefType kLightDefType{0x00F8F130, 0x00B24E58, 0x00AF49F0, 176, 0x007A68DC, 0x007A7E68, 96};
constexpr DefType kCameraDefType{0x00F8F134, 0x00B140D0, 0x00AF4A3E, 192, 0x0073374C, 0x007A805C, 96};
constexpr DefType kPointDefType{0x00F8F13C, 0x00B24E98, 0x00AF4AE0, 112, 0x007A6A64, 0x007A8544, 96};
constexpr DefType kSplineDefType{0x00F8F140, 0x00B0F028, 0x00AF4B2E, 128, 0x007936FC, 0x007A8738, 96};
constexpr DefType kCameraInstType{0x00F8F150, 0x00B24EB8, 0x00AF4C77, 352, 0x007A6D74, 0x007A8D40, 204};
constexpr DefType kPointInstType{0x00F8F154, 0x00B13900, 0x00AF4CCD, 288, 0x0073343C, 0x007A8F34, 204};
constexpr DefType kSplineInstType{0x00F8F158, 0x00B0EFE8, 0x00AF4D21, 288, 0x007335C4, 0x007A92F4, 204};

}  // namespace

// void <Type>_Destroy(T* o): DefDestroy for each type (definitions keyed
// by +96, instances by their +204 name hash) (name?).
DECOMP(0059BD24, GeomDef_Destroy) { DefDestroy(ctx, arg<u32>(ctx, 0), kGeomDefType); }
DECOMP(0059C5FC, LightDef_Destroy) { DefDestroy(ctx, arg<u32>(ctx, 0), kLightDefType); }
DECOMP(0059D1A8, CameraDef_Destroy) { DefDestroy(ctx, arg<u32>(ctx, 0), kCameraDefType); }
DECOMP(0059E7FC, PointDef_Destroy) { DefDestroy(ctx, arg<u32>(ctx, 0), kPointDefType); }
DECOMP(0059FE4C, SplineDef_Destroy) { DefDestroy(ctx, arg<u32>(ctx, 0), kSplineDefType); }
DECOMP(005A664C, CameraInstance_Destroy) { DefDestroy(ctx, arg<u32>(ctx, 0), kCameraInstType); }
DECOMP(005A72EC, PointInstance_Destroy) { DefDestroy(ctx, arg<u32>(ctx, 0), kPointInstType); }
DECOMP(005A89F0, SplineInstance_Destroy) { DefDestroy(ctx, arg<u32>(ctx, 0), kSplineInstType); }

namespace {

constexpr u32 Mem_DefaultAllocator = 0x00550EE8;  // () -> Allocator*
constexpr u32 Array_Init = 0x00558F38;          // (Array*): empty
constexpr u32 Iff_RegisterChunk = 0x0055902C;   // (fourcc, opd, arg, name, flags)
constexpr u32 kConfig = 0x010C5A20;             // g::config: u16 list sizes at +362..+384 (name?)

// A type's ApeTListAllocator header and name map, made at start-up: the
// header {0, count, 0, -, map} with count = the config's u16 (0: the
// default; negative: 0), the 24-byte map {u16 count, 0, 0, 16, data, the
// default allocator, 0, compare} (data from the allocator when count is
// nonzero), then the type's two IFF chunks registered.
struct TypeReg {
    u32 def_count, cfg_off, hdr_global, compare, chunk1, opd1, chunk2, opd2;
    u32 chunk3 = 0, opd3 = 0, flags = 0;  // a third chunk and the chunks' flags (name?)
};

void RegisterType(PPUContext& ctx, const TypeReg& t) {
    u32 n = t.def_count;
    const u32 v = load<u16>(kConfig + t.cfg_off);
    if (v != 0) n = static_cast<s16>(v) > 0 ? static_cast<u32>(static_cast<s32>(static_cast<s16>(v))) : 0;
    u32 hdr = 0;
    const u32 h = static_cast<u32>(call(ctx, o15fn::Mem_Alloc, {20, 0, 16}));
    if (h != 0) {
        store<u32>(h, 0);
        store<u32>(h + 8, 0);
        store<u32>(h + 4, n);
        store<u32>(h + 16, 0);
        hdr = h;
    }
    store<u32>(t.hdr_global, hdr);
    u32 map = 0;
    const u32 m = static_cast<u32>(call(ctx, o15fn::Mem_Alloc, {24, 0, 16}));
    if (m != 0) {
        store<u16>(m, static_cast<u16>(n));
        store<u16>(m + 2, 0);
        store<u16>(m + 4, 0);
        store<u16>(m + 6, 16);
        store<u32>(m + 8, 0);
        const u32 a = static_cast<u32>(call(ctx, Mem_DefaultAllocator));
        store<u32>(m + 12, a);
        const u32 cnt = load<u16>(m);
        if (cnt != 0) {
            const u32 bytes = cnt * ((load<u16>(m + 6) >> 1) & 0x7FFF);
            const u32 opd = load<u32>(load<u32>(a));
            store<u32>(m + 8, static_cast<u32>(call_ptr(ctx, opd, {a, bytes, 0, 16, load<u32>(opd), opd})));
        }
        call(ctx, Array_Init, {m});
        store<u32>(m + 16, 0);
        store<u32>(m + 20, t.compare);
        map = m;
    }
    store<u32>(hdr + 16, map);  // through a null header when its allocation failed, as in the original
    call(ctx, Iff_RegisterChunk, {t.chunk1, t.opd1, 0, 0, t.flags});
    call(ctx, Iff_RegisterChunk, {t.chunk2, t.opd2, 0, 0, t.flags});
    if (t.chunk3 != 0) call(ctx, Iff_RegisterChunk, {t.chunk3, t.opd3, 0, 0, t.flags});
}

}  // namespace

// void <Type>_RegisterType(): RegisterType for the six object types
// (their chunk fourccs: PNTC/PNTH, PNDC/PNDH, EINS/EINH, CAMI/CAMH,
// CAMC/CAMH, LITB/LITH) (name?).
DECOMP(005871E4, PointInstance_RegisterType) {
    RegisterType(ctx, {64, 366, 0x00F8F154, 0x00F2E1B0, 0x43544E50, 0x00F3C998, 0x48544E50, 0x00F3C9A0});
}
DECOMP(005873AC, PointDef_RegisterType) {
    RegisterType(ctx, {64, 380, 0x00F8F13C, 0x00F3CC28, 0x43444E50, 0x00F3C7C8, 0x48444E50, 0x00F3C7D0});
}
DECOMP(00587574, EmitterInstance_RegisterType) {
    RegisterType(ctx, {512, 362, 0x00F8F14C, 0x00F3A9C0, 0x534E4945, 0x00F3C928, 0x484E4945, 0x00F3C930});
}
DECOMP(00587CC8, CameraInstance_RegisterType) {
    RegisterType(ctx, {32, 370, 0x00F8F150, 0x00F3CC48, 0x494D4143, 0x00F3C968, 0x48494D43, 0x00F3C970});
}
DECOMP(00588078, CameraDef_RegisterType) {
    RegisterType(ctx, {32, 384, 0x00F8F134, 0x00F2E1F8, 0x434D4143, 0x00F3C740, 0x484D4143, 0x00F3C748});
}
DECOMP(00588660, LightDef_RegisterType) {
    RegisterType(ctx, {128, 378, 0x00F8F130, 0x00F3CBF0, 0x4254494C, 0x00F3C700, 0x4854494C, 0x00F3C708});
}
DECOMP(00587E90, SplineDef_RegisterType) {
    RegisterType(ctx, {64, 386, 0x00F8F140, 0x00F3A9B0, 0x53445053, 0x00F3C800, 0x48445053, 0x00F3C808, 0x50445053,
                       0x00F3C810, 16});
}
DECOMP(00588478, GeomDef_RegisterType) {
    RegisterType(ctx, {1024, 374, 0x00F8F12C, 0x00F2FB88, 0x424F4547, 0x00F3C6E0, 0x484F4547, 0x00F3C6E8, 0x444F4C47,
                       0x00F3C6F0, 0});
}

namespace {

constexpr u32 FileReader_ReadByte = 0x0056998C;
constexpr u32 WorldObj_ApplyHeader = 0x00580B18;  // (obj, const InstanceHeader*) (name?)
constexpr u32 kStrInst = 0x00B20000;             // the header field names: kStrInst + offset
constexpr u32 kIhSize = 116;                     // the header: +0 xBasis, +12 zBasis, +24 position, +36 scale,
                                                 // +48 flags, +52 nameHash, +56 animHash, +60 name[32], +92 defHash,
                                                 // +96 parentHash, +100 parentPartHash, +104 pvsHash, +108 lightGroup,
                                                 // +112 u8 renderPass, +113 sortOffset, +114 fogGroup (name?)

// The instance header both instance loaders read (version 2 default: the
// fog group from 1, the render pass from 2), applied with
// WorldObj_ApplyHeader.
void ReadInstanceHeader(PPUContext& ctx, u32 r, u32 o, u32 apply = WorldObj_ApplyHeader) {
    Stack frame(ctx, 128);
    const u32 h = frame.addr();  // +116 u8, +117 the version
    call(ctx, 0x00726E54u /* memset */, {h, 0, kIhSize});
    call(ctx, 0x00726E54u, {h, 0, kIhSize});
    call(ctx, o15fn::Iff_ReadU8, {r, h + 116, 0});
    store<u8>(h + 117, 2);
    call(ctx, o15fn::Iff_ReadU8, {r, h + 117, kStrInst + 19702});         // "version"
    call(ctx, 0x005699F0u, {r, h, kStrInst + 19710});                     // "xBasis"
    call(ctx, 0x005699F0u, {r, h + 12, kStrInst + 19717});                // "zBasis"
    call(ctx, 0x005699F0u, {r, h + 24, kStrInst + 19724});                // "position"
    call(ctx, 0x005699F0u, {r, h + 36, kStrInst + 19733});                // "scale"
    call(ctx, Iff_ReadU32, {r, h + 48, kStrInst + 19739});                // "flags"
    call(ctx, Iff_ReadU32, {r, h + 52, kStrInst + 19745});                // "nameHash"
    call(ctx, Iff_ReadU32, {r, h + 56, kStrInst + 19754});                // "animHash"
    call(ctx, FileReader_Read32, {r, h + 60, kStrInst + 19763});          // "name"
    call(ctx, Iff_ReadU32, {r, h + 92, kStrInst + 19768});                // "defHash"
    call(ctx, Iff_ReadU32, {r, h + 96, kStrInst + 19776});                // "parentHash"
    call(ctx, Iff_ReadU32, {r, h + 100, kStrInst + 19787});               // "parentPartHash"
    call(ctx, Iff_ReadU32, {r, h + 104, kStrInst + 19802});               // "pvsHash"
    call(ctx, Iff_ReadU32, {r, h + 108, kStrInst + 19810});               // "lightGroup"
    call(ctx, FileReader_ReadByte, {r, h + 113, kStrInst + 19821});       // "sortOffset"
    if (load<u8>(h + 117) >= 1) call(ctx, o15fn::Iff_ReadU8, {r, h + 114, kStrInst + 19832});  // "fogGroup"
    if (load<u8>(h + 117) >= 2) call(ctx, o15fn::Iff_ReadU8, {r, h + 112, kStrInst + 19841});  // "renderPass"
    call(ctx, apply, {o, h});
}

}  // namespace

// bool LightInstance_ReadHeader(IffReader* r, Obj* o) / EmitterInstance_ReadHeader:
// ReadInstanceHeader; returns 1 (the two are the same code) (name?).
DECOMP(005A39A8, LightInstance_ReadHeader) {
    ReadInstanceHeader(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1));
    ret(ctx, 1u);
}
DECOMP(005A4CE4, EmitterInstance_ReadHeader) {
    ReadInstanceHeader(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1));
    ret(ctx, 1u);
}

namespace {
constexpr u32 WorldObj_ApplyHeaderB = 0x0057D3E8;  // (obj, const InstanceHeader*) (name?)
constexpr u32 PointDef_GetDefault = 0x0059E9B0; // () -> the default point definition (name?)
constexpr u32 kVfnSetClass = 44;                // ApeObj_SetClass

// The point / spline instance loaders: the header (WorldObj_ApplyHeaderB),
// then the type's default definition when the header gave none.
void ReadInstanceHeaderB(PPUContext& ctx, u32 r, u32 o, u32 get_default) {
    ReadInstanceHeader(ctx, r, o, WorldObj_ApplyHeaderB);
    if (load<u32>(o + 208) == 0) {
        const u32 c = static_cast<u32>(call(ctx, get_default));
        const u32 opd = load<u32>(load<u32>(o) + kVfnSetClass);
        call_ptr(ctx, opd, {o, c, opd, load<u32>(opd)});
    }
}
}  // namespace

// bool PointInstance_ReadHeader(IffReader* r, Obj* o) / SplineInstance_ReadHeader:
// ReadInstanceHeaderB with the point / spline default definition; returns 1 (name?).
DECOMP(005A7070, PointInstance_ReadHeader) {
    ReadInstanceHeaderB(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), PointDef_GetDefault);
    ret(ctx, 1u);
}
DECOMP(005A8774, SplineInstance_ReadHeader) {
    ReadInstanceHeaderB(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), 0x005A0000u /* SplineDef_GetDefault */);
    ret(ctx, 1u);
}

namespace {
constexpr u32 kWorkersEnabled = 0x00F66FD4;     // u8 (name?)
constexpr u32 kFrameCounter = 0x010DDA78;       // u32 (SetDrawCount_00500FB4's word)
constexpr u32 kWkPlayer = 0, kWkRec = 8, kWkCount = 12, kWkFrame = 16, kWk20 = 20, kWk24 = 24, kWkMode = 32, kWk48 = 48;  // (name?)
}  // namespace

// void Workers_Update(Obj* o, Obj* player)
// Keeps the player; once per frame, when the work record, its count, the
// global switch and the player's +256 bit 0 allow it: the record's vfunc
// +20 (player, &+48) when +24 is set, then (unless the record's +96 has
// 0x80) o's vfunc +40 (mode +32 set) or +44 with (+20, +24), and o's
// vfunc +48 (name?).
DECOMP(00598E60, Workers_Update) {
    const u32 o = arg<u32>(ctx, 0), pl = arg<u32>(ctx, 1);
    const u32 w = kWorkers;
    store<u32>(w + kWkPlayer, pl);
    const u32 rec = load<u32>(w + kWkRec);
    if (rec == 0 || load<u16>(w + kWkCount) == 0 || load<u8>(kWorkersEnabled) == 0 || (load<u32>(pl + 256) & 1) == 0) return;
    const u32 frame = load<u32>(kFrameCounter);
    if (load<u32>(w + kWkFrame) == frame) return;
    u32 v = load<u32>(w + kWk24);
    store<u32>(w + kWkFrame, frame);
    if (v != 0) {
        const u32 opd = load<u32>(load<u32>(rec) + 20);
        call_ptr(ctx, opd, {rec, pl, w + kWk48, opd, load<u32>(opd)});
        v = load<u32>(w + kWk24);
    }
    if ((load<u8>(rec + 96) & 0x80) != 0) return;
    if (v == 0) return;
    if (load<u8>(w + kWkMode) != 0) {
        const u32 opd = load<u32>(load<u32>(o) + 40);
        call_ptr(ctx, opd, {o, load<u32>(w + kWk20), v, load<u32>(opd), opd});  // r6 = entry, r7 = OPD here
    } else {
        const u32 opd = load<u32>(load<u32>(o) + 44);
        call_ptr(ctx, opd, {o, load<u32>(w + kWk20), v, opd, load<u32>(opd)});
    }
    const u32 opd = load<u32>(load<u32>(o) + 48);
    call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
}

// void Matrix_PackShaderRows(const f32 m[42], f32 out[42])
// Copies the source words into the shader constant layout (rows 2, 3,
// 6, 7, 4, then 0, 1 repeated with the others), word by word in the
// original's order (name?).
DECOMP(00573410, Matrix_PackShaderRows) {
    const u32 s = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    static constexpr u8 kMap[42][2] = {
        {0, 32}, {4, 36}, {8, 40}, {12, 48}, {16, 52}, {20, 56}, {24, 96}, {28, 100}, {32, 104}, {36, 112}, {40, 116},
        {44, 120}, {48, 64}, {52, 68}, {60, 48}, {56, 72}, {64, 52}, {68, 56}, {72, 0}, {76, 4}, {84, 32}, {80, 8},
        {88, 36}, {92, 40}, {96, 16}, {100, 20}, {108, 96}, {104, 24}, {112, 100}, {116, 104}, {120, 80}, {124, 84},
        {132, 64}, {128, 88}, {136, 68}, {140, 72}, {144, 16}, {148, 20}, {152, 24}, {160, 4}, {156, 0}, {164, 8}};
    for (const auto& e : kMap) store<u32>(d + e[0], load<u32>(s + e[1]));
    ctx.fpr[1] = load<f32>(s);
}

namespace {
constexpr u32 Fn_5E4514 = 0x005E4514, Fn_5E3F18 = 0x005E3F18, Fn_5E3E88 = 0x005E3E88;  // gcm helpers (name?)
constexpr u32 cellGcmSetVertexProgramConstants = 0x005E5CB0;  // (context, index, count, const f32*)
constexpr u32 kGcmCurShader = kGcmContext + 52; // the bound shader record (name?)
constexpr u32 kGcmState222 = kGcmContext + 222, kGcmState225 = kGcmContext + 225;  // u8 state bytes (name?)
}  // namespace

// void ShaderRec_Bind(Rec* r)
// Binds r->+144's shader (Fn_5E4514 on its +16, Fn_5E3F18 with its +17
// count) unless it is the bound one, Fn_5E3E88 with two cached state
// bytes, the 0x1A08 method (0x30404), then r's eight vertex constants
// (+160..+272) at 8, 9 and 33-38 (name?).
DECOMP(00572728, ShaderRec_Bind) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 t = load<u32>(r + 144);
    if (load<u32>(kGcmCurShader) != t) {
        store<u32>(kGcmCurShader, t);
        call(ctx, Fn_5E4514, {load<u32>(kGcmContext), 0, t + 16});
        const u32 b = load<u8>(t + 17);
        call(ctx, Fn_5E3F18, {load<u32>(kGcmContext), 0, 1, 0, (((b << 8) & 0xFFFFFF00u) - 256) & 0xFFFF, 0});
    }
    call(ctx, Fn_5E3E88, {load<u32>(kGcmContext), 0, 0, load<u8>(kGcmState225 + 1), load<u8>(kGcmState222 + 1), 1});
    GcmEmit({0x00041A08, 0x00030404});
    static constexpr u8 kSlots[8] = {8, 9, 33, 34, 35, 36, 37, 38};
    for (u32 i = 0; i < 8; ++i)
        call(ctx, cellGcmSetVertexProgramConstants, {load<u32>(kGcmContext), kSlots[i], 4, r + 160 + 16 * i});
}

namespace {
constexpr u32 kQuadConsts = 0x005784DC;         // f32 {1, 1e-5, 0}
constexpr u32 Shader_FindByHash = 0x004FE990;
constexpr u32 kShaderBatchable = 0xFEEE0ACA;    // the shader hash a batchable mesh must use (name?)
constexpr u32 kVfnObj12b = 12;
}  // namespace

// bool Quad_IntersectSegment(const f32 quad[4][4], const f32 a[3], const f32 b[3], f32* t)
// The segment a-b against the convex quad's plane (normal from its first
// three corners): not when nearly parallel or the hit is outside t in
// [0, 1] or behind any edge; *t on a hit (name?).
DECOMP(005784E8, Quad_IntersectSegment) {
    const u32 q = arg<u32>(ctx, 0), pa = arg<u32>(ctx, 1), pb = arg<u32>(ctx, 2), pt = arg<u32>(ctx, 3);
    using obj9::Fsubs; using obj9::Fmuls; using obj9::Fmadds; using obj10::Fmsubs; using obj9::Lf;
    const f64 e1x = Fsubs(Lf(q + 16), Lf(q)), e2z = Fsubs(Lf(q + 40), Lf(q + 8)), e1z = Fsubs(Lf(q + 24), Lf(q + 8));
    const f64 e2x = Fsubs(Lf(q + 32), Lf(q)), e2y = Fsubs(Lf(q + 36), Lf(q + 4)), e1y = Fsubs(Lf(q + 20), Lf(q + 4));
    f64 ny = Fmsubs(e1z, e2x, Fmuls(e1x, e2z));
    f64 nx = Fmsubs(e1y, e2z, Fmuls(e1z, e2y));
    f64 nz = Fmsubs(e1x, e2y, Fmuls(e1y, e2x));
    const f64 one = Lf(kQuadConsts), eps = Lf(kQuadConsts + 4), zero = Lf(kQuadConsts + 8);
    const f64 ax = Lf(pa), ay = Lf(pa + 4), az = Lf(pa + 8);
    const f64 dy = Fsubs(Lf(pb + 4), ay), dx = Fsubs(Lf(pb), ax), dz = Fsubs(Lf(pb + 8), az);
    const f64 len2 = Fmadds(nz, nz, Fmadds(nx, nx, Fmuls(ny, ny)));
    const f64 inv = obj9::Fdivs(one, obj10::Fsqrts(len2));
    ny = Fmuls(ny, inv);
    nx = Fmuls(nx, inv);
    nz = Fmuls(nz, inv);
    const f64 dn = Fmadds(dz, nz, Fmadds(dx, nx, Fmuls(dy, ny)));
    ctx.fpr[1] = Lf(q + 8);
    if (std::fabs(dn) < eps) {
        ret(ctx, 0u);
        return;
    }
    const f64 t = obj9::Fdivs(Fmadds(Fsubs(Lf(q + 8), az), nz, Fmadds(Fsubs(Lf(q), ax), nx, Fmuls(Fsubs(Lf(q + 4), ay), ny))), dn);
    const f64 hx = Fmadds(t, dx, ax), hy = Fmadds(t, dy, ay), hz = Fmadds(t, dz, az);
    ctx.fpr[1] = hy;
    if (t < zero || t > one) {
        ret(ctx, 0u);
        return;
    }
    for (u32 i = 0; i < 4; ++i) {
        const u32 p = q + 16 * i, n = q + (((i + 1) << 4) & 0x30);
        const f64 px = Lf(p), py = Lf(p + 4), pz = Lf(p + 8);
        const f64 rz = Fsubs(hz, pz), ry = Fsubs(hy, py), rx = Fsubs(hx, px);
        const f64 ex = Fsubs(Lf(n), px), ez = Fsubs(Lf(n + 8), pz), ey = Fsubs(Lf(n + 4), py);
        const f64 cy = Fmsubs(ez, rx, Fmuls(ex, rz));
        const f64 cx = Fmsubs(ey, rz, Fmuls(ez, ry));
        const f64 cz = Fmsubs(ex, ry, Fmuls(ey, rx));
        if (Fmadds(cz, nz, Fmadds(cx, nx, Fmuls(cy, ny))) < zero) {
            ret(ctx, 0u);
            return;
        }
    }
    store<f32>(pt, static_cast<f32>(t));
    ret(ctx, 1u);
}

// bool WorldObj_CanBatch(Obj* o)
// A visible (+164 0x8000) static object (+320 bits 9-12 == 1) with a
// class, no parent or children, whose mesh has no +72 / +84 extras, at
// most LOD level 2 (+112), one part (+52) and the batchable shader
// (0xFEEE0ACA); the failing checks after the first call o's vfunc +12
// (name?).
DECOMP(0059578C, WorldObj_CanBatch) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 d = load<u32>(o + 208);
    if (((load<u32>(o + 320) >> 9) & 0xF) != 1 || d == 0 || (load<u32>(o + 164) & 0x8000) == 0) {
        ret(ctx, 0u);
        return;
    }
    auto reject = [&]() {
        const u32 opd = load<u32>(load<u32>(o) + kVfnObj12b);
        call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
        ret(ctx, 0u);
    };
    if (load<u32>(o + 260) != 0 || load<u32>(o + 272) != 0) return reject();
    const u32 t = load<u32>(d + 132);
    const u32 m = load<u32>(t + 48);
    const u32 mm = load<u32>(m);
    if (load<u32>(mm + 72) != 0 || load<u32>(mm + 84) != 0) return reject();
    if (load<u8>(d + 112) > 2) return reject();
    if (load<u16>(t + 52) > 1) return reject();
    const u32 sh = load<u32>(load<u32>(m + 4) + 16);
    if (sh != static_cast<u32>(call(ctx, Shader_FindByHash, {kShaderBatchable}))) return reject();
    ret(ctx, 1u);
}
