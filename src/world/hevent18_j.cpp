// World_HandleEvent callees, part 18j: creating an object's WorldObj from
// its spawn description.
#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
// Object: +32 / +96 3x4 matrices (rows 16 bytes apart), +144 position,
// +160 WorldObj*, +164 Ctl*, +172 u32 flags (bit 9 0x200: has a ctl, bit 10
// 0x400: shared WorldObj), +216 def, +304 vec3 home position; vt +116
// (o, 255, 1, 1, 0).
constexpr u32 kObjMat32 = 32, kObjMat96 = 96, kObjPos = 144, kObjWobj = 160, kObjCtl = 164, kObjFlags = 172;
constexpr u32 kObjHome = 304, kObjDef = 216, kObjVt116 = 116;
constexpr u32 kObjHasCtl = 0x200, kObjShared = 0x400;
// Def: +4 u32 kind name, +8 GeomInstance*, +32 u8 flags (bit 0 0x01, bit 4
// 0x10 instanced, bit 5 0x20 share), +44 / +48 u32 name hashes, +124 u32
// line hash, +128 u32, +132 f32.
constexpr u32 kDefKind = 4, kDefGeom = 8, kDefFlags = 32, kDefName2 = 44, kDefName = 48;
constexpr u32 kDefLine = 124, kDef128 = 128, kDef132 = 132;
constexpr u32 kDefF01 = 0x01, kDefFInstanced = 0x10, kDefFShare = 0x20;
// Spawn description: +20 u32 flags (bit 13 0x2000 visible, bit 14 0x4000
// keep the WorldObj's matrix, bit 15 0x8000, bit 16 0x10000 controlled),
// +28 name, +36 GeomInstance*, +40 const char* name, +112 vec3 scale, +136
// f32.
constexpr u32 kDescFlags = 20, kDescName = 28, kDescGeom = 36, kDescStr = 40, kDescScale = 112, kDesc136 = 136;
constexpr u32 kDfVisible = 0x2000, kDfKeepMatrix = 0x4000, kDfCtl8000 = 0x8000, kDfControlled = 0x10000;
// GeomInstance / WorldObj: +4 u16 references (geom), +4 u32 kind bits
// 10..15 (WorldObj, 8: mesh), +16 3x4 matrix, +208 Def*, +276 u32 tag, +280
// Object* owner; vt +40 show(w, u8, 1), vt +56 instance(g, kind).
constexpr u32 kGeomRefs = 4, kWobjMatrix = 16, kWobjDef = 208, kWobjTag = 276, kWobjOwner = 280;
constexpr u32 kWobjVtShow = 40, kGeomVtInstance = 56, kWobjTagValue = 0x02235C33;
constexpr u32 kKindPlain = 13, kKindInstanced = 15;
// Ctl (120 bytes): +16 u32, +40 u16 flags (bit 10 0x400, bit 12 0x1000),
// +44 / +48 f32, +60 callback OPD, +64 its argument.
constexpr u32 kCtlSize = 120, kCtl16 = 16, kCtlFlags = 40, kCtl44 = 44, kCtl48 = 48, kCtlCb = 60, kCtlCbArg = 64;
constexpr u32 kCtlCbOpd = 0x00F31530;
// The named-geometry map (*0x00F8F12C +16).
constexpr u32 kGeomMapHolder = 0x00F8F12C, kGeomMap = 16;
constexpr u32 kNameAC6790 = 0x00AC6790;  // name given to instanced defs (funcinfo: "FromTarget")

namespace jfn {
constexpr u32 GeomInstance_FindByHash = 0x005801A0;
constexpr u32 WorldObj_GetPeer = 0x0057A428;
constexpr u32 Peer_GetVec = 0x0057BE98;
constexpr u32 WorldObj_SetName = 0x0057D368;
constexpr u32 HashMap_Find2 = 0x007399A4;
constexpr u32 WorldObj_Instance = 0x005A05E8;
constexpr u32 Def_SetName = 0x00584BDC;
constexpr u32 Hash_ToString = 0x0056A6E0;
constexpr u32 WorldObj_AddToWorld = 0x005A0394;
constexpr u32 Obj_Prepare0D2E68 = 0x000D2E68;  // (WorldObj*, 1) (name?)
constexpr u32 Ctl_Construct34BDEC = 0x0034BDEC;
constexpr u32 Line_Find348ECC = 0x00348ECC;    // (u32 hash) -> Line* or 0 (name?)
constexpr u32 Ctl_SetLine34F690 = 0x0034F690;  // (Ctl*, Line*, 0) (symbols.txt: Speaker_SetLine34F690)
}  // namespace jfn

void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
// 3x4 copy, rows 16 bytes apart.
void copy34(u32 dst, u32 src) {
    for (u32 r = 0; r < 64; r += 16)
        for (u32 c = 0; c < 12; c += 4) copy32(dst + r + c, src + r + c);
}
bool def_flag(u32 o, u32 bit) { return (load<u8>(load<u32>(o + kObjDef) + kDefFlags) & bit) != 0; }
}  // namespace

// void Obj_ApplyQuery0D2F04(Object* o, const SpawnDesc* sd) (name?):
// finds the object's geometry (the description's, the def's, or by name
// hash) and shares it (share flag / keep-matrix) or instances it (vt +56,
// kind 13 / 15) into +160; without geometry instances the def's named
// geometry. Then tags and shows the WorldObj, applies scale and matrix
// (either way), the home position and, for controlled mesh objects,
// creates the Ctl (+164).
DECOMP(000D2F04, Obj_ApplyQuery0D2F04) {
    const u32 o = arg<u32>(ctx, 0), sd = arg<u32>(ctx, 1);
    u32 geom = load<u32>(sd + kDescGeom);
    u32 kind = 0, name = 0, name2 = 0;  // r28, r27, r26
    bool added = false;                 // r25
    const u32 d0 = load<u32>(o + kObjDef);
    const bool share = d0 != 0 && (load<u8>(d0 + kDefFlags) & kDefFShare) != 0;
    if (geom == 0 && d0 != 0) {
        geom = load<u32>(d0 + kDefGeom);
        kind = load<u32>(d0 + kDefKind);
        name = load<u32>(d0 + kDefName);
        name2 = load<u32>(d0 + kDefName2);
    }
    if (geom == 0 && name == 0) {
        const u32 s = load<u32>(sd + kDescStr);
        if (load<u8>(s) != 0) name = static_cast<u32>(call(ctx, hfn::String_HashNoCase, {s}));
    }
    if (kind == 0 && name2 == 0) {
        const u32 s = load<u32>(sd + kDescStr);
        if (load<u8>(s) != 0) name2 = static_cast<u32>(call(ctx, hfn::String_HashNoCase, {s}));
    }
    if (name != 0 && geom == 0) geom = static_cast<u32>(call(ctx, jfn::GeomInstance_FindByHash, {name}));

    if (geom != 0) {
        if ((load<u32>(sd + kDescFlags) & kDfKeepMatrix) != 0 || share) {
            store<u32>(o + kObjWobj, geom);
            store<u16>(geom + kGeomRefs, static_cast<u16>(load<u16>(geom + kGeomRefs) + 1));
            store<u32>(o + kObjFlags, load<u32>(o + kObjFlags) | kObjShared);
        } else {
            vcall(ctx, geom, kWobjVtShow, {geom, 0, 1});
            added = true;
            kind = kKindPlain;
            const u32 d = load<u32>(o + kObjDef);
            if (d != 0 && (load<u8>(d + kDefFlags) & kDefFInstanced) != 0) {
                kind = kKindInstanced;
                if (call(ctx, jfn::WorldObj_GetPeer, {geom}) != 0) {
                    const u32 peer = static_cast<u32>(call(ctx, jfn::WorldObj_GetPeer, {geom}));
                    const u32 v = static_cast<u32>(call(ctx, jfn::Peer_GetVec, {peer}));
                    store<u8>(v + 28, static_cast<u8>(load<u8>(v + 28) | 0x20));
                }
            }
            const u32 w = static_cast<u32>(vcall(ctx, geom, kGeomVtInstance, {geom, kind}));
            store<u32>(o + kObjWobj, w);
            call(ctx, jfn::WorldObj_SetName, {w, load<u32>(sd + kDescName), 1});
        }
    } else {
        if (kind == 0 && name2 != 0) {
            Stack frame(ctx, 16);  // the key word (r1 +112 in the original frame)
            store<u32>(frame.addr(), name2);
            const u32 slot = static_cast<u32>(
                call(ctx, jfn::HashMap_Find2, {load<u32>(load<u32>(kGeomMapHolder) + kGeomMap), frame.addr()}));
            kind = load<u32>(slot);
        }
        if (kind != 0) {
            u32 w = static_cast<u32>(call(ctx, jfn::WorldObj_Instance, {kind, def_flag(o, kDefFInstanced) ? 1u : 0u}));
            const bool inst = def_flag(o, kDefFInstanced);  // the def is not null-checked (as the original)
            store<u32>(o + kObjWobj, w);
            if (inst) {
                call(ctx, jfn::Def_SetName, {load<u32>(w + kWobjDef), kNameAC6790, 1});
                w = load<u32>(o + kObjWobj);
            }
            call(ctx, jfn::WorldObj_SetName, {w, load<u32>(sd + kDescName), 1});
        }
    }

    const u32 w = load<u32>(o + kObjWobj);
    if (w == 0) {
        // Debug name lookups only (results unused).
        if (name != 0) call(ctx, jfn::Hash_ToString, {name});
        else if (name2 != 0) call(ctx, jfn::Hash_ToString, {name2});
        return;
    }
    store<u32>(w + kWobjOwner, o);
    store<u32>(w + kWobjTag, kWobjTagValue);
    vcall(ctx, w, kWobjVtShow, {w, (load<u32>(sd + kDescFlags) & kDfVisible) != 0 ? 1u : 0u, 1});
    vcall(ctx, o, kObjVt116, {o, 255, 1, 1, 0});
    call(ctx, hevent10::h10fn::WorldObj_SetScale, {load<u32>(o + kObjWobj), sd + kDescScale, 0});
    u32 wr;
    if ((load<u32>(sd + kDescFlags) & kDfKeepMatrix) != 0) {
        wr = load<u32>(o + kObjWobj);
        // Stored as the original schedules them: +96.. up to +148, +32, +152,
        // then the rest of +32...
        constexpr u32 kM[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
        for (u32 k = 0; k < 11; ++k) copy32(o + kObjMat96 + kM[k], wr + kWobjMatrix + kM[k]);
        copy32(o + kObjMat32, wr + kWobjMatrix);
        copy32(o + kObjMat96 + 56, wr + kWobjMatrix + 56);
        for (u32 k = 1; k < 12; ++k) copy32(o + kObjMat32 + kM[k], wr + kWobjMatrix + kM[k]);
        for (u32 k = 0; k < 12; ++k)  // f1..f12 left holding the matrix
            ctx.fpr[1 + k] = hevent7::lfs_f64(wr + kWobjMatrix + 16 * (k / 3) + 4 * (k % 3));
    } else {
        const u32 w2 = load<u32>(o + kObjWobj);
        copy34(w2 + kWobjMatrix, o + kObjMat96);
        ctx.fpr[1] = hevent7::lfs_f64(o + kObjMat96 + 56);
        call(ctx, hevent7::h7fn::WorldObj_MarkDirty, {w2});
        wr = load<u32>(o + kObjWobj);
    }
    for (u32 c = 0; c < 12; c += 4) copy32(o + kObjHome + c, o + kObjPos + c);
    ctx.fpr[1] = hevent7::lfs_f64(o + kObjPos + 8);
    const u32 d = load<u32>(o + kObjDef);
    if (d != 0 && (load<u8>(d + kDefFlags) & kDefF01) != 0) {
        call(ctx, jfn::Obj_Prepare0D2E68, {wr, 1});
        wr = load<u32>(o + kObjWobj);
    }
    if (added) {
        call(ctx, jfn::WorldObj_AddToWorld, {wr});
        wr = load<u32>(o + kObjWobj);
    }
    if ((load<u32>(sd + kDescFlags) & kDfControlled) == 0) return;
    if (((load<u32>(wr + 4) >> 10) & 0x3F) != 8) return;
    u32 ctl = 0;
    if (const u32 c = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kCtlSize})); c != 0) {
        call(ctx, jfn::Ctl_Construct34BDEC, {c, load<u32>(o + kObjWobj)});
        ctl = c;
    }
    // A failed allocation still writes through address 0 (as the original).
    store<u32>(o + kObjCtl, ctl);
    store<u32>(ctl + kCtlCb, kCtlCbOpd);
    store<u32>(ctl + kCtlCbArg, o);
    const u32 bit = (load<u32>(sd + kDescFlags) & kDfCtl8000) != 0 ? 1u : 0u;
    u32 c = load<u32>(o + kObjCtl);
    store<u16>(c + kCtlFlags, static_cast<u16>((load<u16>(c + kCtlFlags) & ~0x1000u) | (bit << 12)));
    c = load<u32>(o + kObjCtl);
    copy32(c + kCtl48, load<u32>(o + kObjDef) + kDef132);
    store<u32>(o + kObjFlags, load<u32>(o + kObjFlags) | kObjHasCtl);
    store<u16>(c + kCtlFlags, static_cast<u16>(load<u16>(c + kCtlFlags) | 0x400));
    c = load<u32>(o + kObjCtl);
    copy32(c + kCtl44, sd + kDesc136);
    ctx.fpr[1] = hevent7::lfs_f64(sd + kDesc136);
    const u32 dd = load<u32>(o + kObjDef);
    store<u32>(c + kCtl16, load<u32>(dd + kDef128));
    const u32 line = load<u32>(dd + kDefLine);
    if (line == 0) return;
    const u32 l = static_cast<u32>(call(ctx, jfn::Line_Find348ECC, {line}));
    if (l == 0) {
        call(ctx, jfn::Hash_ToString, {line});
        return;
    }
    call(ctx, jfn::Ctl_SetLine34F690, {load<u32>(o + kObjCtl), l, 0});
}
