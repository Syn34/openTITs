// Game code, medium sweep part 13: def factories called by 0x000CA038 (a
// defaults record on the stack, the reader, the def built from it) and the
// recursive node-tree reader 0x000C9AB8.
#include "mid13.h"

using namespace sly4;
using namespace sly4::mid13;

// Def* XmlDef_New18F8E8(a0, XmlCursor* c, x, const Def* tmpl)
// A defaults record on the stack (DefHeadInit with x->12..20 and f 100.0,
// +0 |= 0x200; +124 "" (0x00AC8C70), then the floats -1 x4, 1, -1, 1, -1,
// 0, 0.3, -1, 1, -1, 1, 1, 10, 1, 40 at +128..+196, byte +200 0, +204..
// +212 0, f32 +216 1, +220 / +224 0, f32 +228 5, bytes +232..+238 1) read
// by 0x0018EFB0(a0, c, x, tmpl, rec); a 320-byte def (SmallAlloc) built by
// 0x0018EA50(p, rec). Then for each child 0x523827DF of c a 60-byte entry
// (SmallAlloc, zeroed) is appended to the def's vector +296 and reads three
// hashes (0x0024728B, 0x00255BF3, 0x7A30D7E3) into +32..+40, a float
// (0x5B9EF532, default 0) into +44, a hash (0xF08EF516) into +48, bools
// into bytes +54 / +52 / +53 / +55, an s32 (0x04B8CADE) into +56 (all
// defaulting to 0) and its name (attribute 0x0024728B) into its +0.
// Returns the def. Quirk: neither allocation is checked before the entries
// are added (a failed def allocation puts them in a vector at address 296;
// a failed entry allocation is zeroed and filled at address 0).
DECOMP(0018F8E8, XmlDef_New18F8E8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 240);
    const u32 rec = frame.addr();  // the original's sp + 112
    mid10::DefHeadInit(rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 100.0);
    store<u32>(rec, load<u32>(rec) | 0x200u);
    ctx.fpr[1] = -4.0;  // as DefHeadInit's float loop leaves it
    store<u32>(rec + 124, 0x00AC8C70u);
    static constexpr u32 kF[18] = {0xBF800000u, 0xBF800000u, 0xBF800000u, 0xBF800000u, 0x3F800000u, 0xBF800000u,
                                   0x3F800000u, 0xBF800000u, 0,           0x3E99999Au, 0xBF800000u, 0x3F800000u,
                                   0xBF800000u, 0x3F800000u, 0x3F800000u, 0x41200000u, 0x3F800000u, 0x42200000u};
    for (u32 k = 0; k < 18; ++k) store<u32>(rec + 128 + 4 * k, kF[k]);
    store<u8>(rec + 200, 0);
    for (u32 k = 204; k <= 212; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 216, 0x3F800000u);
    store<u32>(rec + 220, 0);
    store<u32>(rec + 224, 0);
    store<u32>(rec + 228, 0x40A00000u);
    for (u32 k = 232; k < 239; ++k) store<u8>(rec + k, 1);
    call(ctx, 0x0018EFB0, {a0, c, x, tmpl, rec});
    u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {320}));
    if (p != 0) call(ctx, 0x0018EA50, {p, rec});
    const f64 f0 = LoadF(0x0018F8E0);
    for (u32 e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, 0x523827DFu})); e != 0;
         e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {e, 0x523827DFu}))) {
        const u32 n = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {60}));
        call(ctx, mid2::m2fn::memset, {n, 0, 60});
        mid1::PtrVecPush(ctx, p + 296, n);
        call(ctx, xfn::XmlCursor_ReadHash, {e, 0x0024728Bu, n + 32, 0});
        call(ctx, xfn::XmlCursor_ReadHash, {e, 0x00255BF3u, n + 36, 0});
        call(ctx, xfn::XmlCursor_ReadHash, {e, 0x7A30D7E3u, n + 40, 0});
        callf(ctx, xfn::XmlCursor_ReadF32, {e, 0x5B9EF532u, n + 44}, {f0});
        call(ctx, xfn::XmlCursor_ReadHash, {e, 0xF08EF516u, n + 48, 0});
        call(ctx, xfn::XmlCursor_ReadBool, {e, 0x8A0781F4u, n + 54, 0});
        call(ctx, xfn::XmlCursor_ReadBool, {e, 0x716F7D47u, n + 52, 0});
        call(ctx, xfn::XmlCursor_ReadBool, {e, 0x1B3959CBu, n + 53, 0});
        call(ctx, xfn::XmlCursor_ReadBool, {e, 0xE757CAA1u, n + 55, 0});
        call(ctx, xfn::XmlCursor_ReadS32, {e, 0x04B8CADEu, n + 56, 0});
        const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {e, 0x0024728Bu}));
        call(ctx, m1fn::String_Assign, {n, s});
    }
    ret(ctx, p);
}

// AreaDef* AreaDef_New2183FC(a0, XmlCursor* c, x, const Def* tmpl)
// An area defaults record on the stack: +0 0x01400000, +12 = x->12, +16 /
// +20 "" (0x00AD366C), +32 1, +36 / +40 / +72 -1, +76
// "TriggerCameraBehavior_", byte +80 1, the rest of the head 0;
// AreaSettingsA_Construct at +96; then the current tuning values (the value
// word +72 of consecutive 76-byte settings entries, copied raw) from
// 0x010334D0 (38) at +224..+372, 0x01034574 at +384, 0x01034628 (9) at
// +416..+448, 0x01034938 (10) at +480..+516, and three-word vectors
// 0x010345D0 / 0x010348E0 at +400..+408 / +464..+472; +528 -1, +544 / +548 / +552 / +560 / +564 FLT_MAX. Read by
// 0x00217470(a0, c, x, tmpl, rec); a 208-byte def (SmallAlloc) built by
// AreaDef_Construct(p, rec). Returns the def (0 when the allocation fails).
DECOMP(002183FC, AreaDef_New2183FC) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 576);
    const u32 rec = frame.addr();  // the original's sp + 112
    store<u32>(rec, 0x00400000u);
    store<u32>(rec + 4, 0);
    store<u32>(rec + 8, 0);
    store<u32>(rec + 12, load<u32>(x + 12));
    store<u32>(rec + 16, 0x00AD366Cu);
    store<u32>(rec + 20, 0x00AD366Cu);
    for (u32 k = 24; k < 32; ++k) store<u8>(rec + k, 0);
    store<u32>(rec + 32, 0);
    store<u32>(rec + 36, 0xBF800000u);
    store<u32>(rec + 40, 0xBF800000u);
    store<u8>(rec + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 72, 0xBF800000u);
    store<u32>(rec + 76, 0x00AD3750u);  // "TriggerCameraBehavior_"
    store<u8>(rec + 80, 1);
    for (u32 k = 81; k < 84; ++k) store<u8>(rec + k, 0);
    store<u32>(rec + 84, 0);
    call(ctx, 0x000FD7AC, {rec + 96});  // AreaSettingsA_Construct
    for (u32 k = 0; k < 38; ++k) CopyW(rec + 224 + 4 * k, 0x010334D0u + 76 * k);
    CopyW(rec + 384, 0x01034574u);
    for (u32 k = 0; k < 12; k += 4) CopyW(rec + 400 + k, 0x010345D0u + k);
    for (u32 k = 0; k < 9; ++k) CopyW(rec + 416 + 4 * k, 0x01034628u + 76 * k);
    for (u32 k = 0; k < 12; k += 4) CopyW(rec + 464 + k, 0x010348E0u + k);
    for (u32 k = 0; k < 10; ++k) CopyW(rec + 480 + 4 * k, 0x01034938u + 76 * k);
    store<u32>(rec + 528, 0xBF800000u);
    for (u32 k : {544u, 548u, 552u, 560u, 564u}) store<u32>(rec + k, 0x7F7FFFFFu);
    store<u32>(rec, load<u32>(rec) | 0x01000000u);
    store<u32>(rec + 32, 1);
    SetF1(ctx, 0x01034BE4u);  // the last lfs, as the original leaves f1
    call(ctx, 0x00217470, {a0, c, x, tmpl, rec});
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {208}));
    if (p != 0) call(ctx, 0x00216AB0, {p, rec});  // AreaDef_Construct
    ret(ctx, p);
}

// void XmlNode_ReadTree(a0, XmlCursor* c, PtrVector** slot)
// For each child 0xCCEB9AF9 of c, a 192-byte node {char name[64] ("name"),
// char nameOverride[64] ("nameOverride"), +128 vec3 position (0xEF4DDD13),
// +144 quaternion (from the euler degrees 0xC7400937, scaled by 0x000C9AB0
// +0 then halved by +4), +160 hash (0x20C00073), +168 hash (0xF91B96E2),
// +176 flags (bits 0x10000000 / 0x80000000 / 0x01000000 from the bools
// 0xD1A6825E / 0x36E7D729 / 0xEAE49DB2), +180 children} is appended to
// *slot (created on first use: SmallAlloc(20), PtrVector_Construct,
// element size 192) by Vector_Add733964, then the child's children are read
// into the new element's +180 recursively. The stack node's own vector
// (always empty) is destroyed after each element. Returns 0.
DECOMP(000C9AB8, XmlNode_ReadTree) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), slot = arg<u32>(ctx, 2);
    constexpr u32 kTag = 0xCCEB9AF9;
    u32 e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_FirstChild, {c, kTag}));
    if (e == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 224);
    const u32 eu = frame.addr(), nd = eu + 16, b = eu + 208;  // the original's sp + 112 / + 128 / + 320
    const u32 vz = mid11::kVecZero, qi = mid11::kQuatIdent;
    constexpr u32 kName = 0x00AC689C, kNameOvr = 0x00AC68A4;  // "name", "nameOverride"
    do {
        const f64 deg = LoadF(0x000C9AB0), half = LoadF(0x000C9AB4);
        store<u32>(nd + 176, load<u32>(nd + 176) & 0x00FFFFFFu);
        for (u32 k = 0; k < 12; k += 4) CopyW(nd + 128 + k, vz + k);
        for (u32 k = 0; k < 16; k += 4) CopyW(nd + 144 + k, qi + k);
        for (u32 k = 160; k <= 172; k += 4) store<u32>(nd + k, 0);
        store<u32>(nd + 180, 0);
        store<u8>(nd, 0);
        store<u8>(nd + 64, 0);
        if (call(ctx, m11fn::XmlCursor_FindAttrByName, {e, kName}) != 0) {
            const u32 s = static_cast<u32>(call(ctx, m11fn::XmlCursor_FindAttrByName, {e, kName}));
            call(ctx, m1fn::String_CopyN, {nd, s, 64});
        }
        if (call(ctx, m11fn::XmlCursor_FindAttrByName, {e, kNameOvr}) != 0) {
            const u32 s = static_cast<u32>(call(ctx, m11fn::XmlCursor_FindAttrByName, {e, kNameOvr}));
            call(ctx, m1fn::String_CopyN, {nd + 64, s, 64});
        }
        call(ctx, xfn::XmlCursor_ReadHash, {e, 0x20C00073u, nd + 160, 0});
        call(ctx, xfn::XmlCursor_ReadVec3, {e, 0xEF4DDD13u, nd + 128, vz});
        for (u32 k = 0; k < 12; k += 4) CopyW(eu + k, vz + k);
        call(ctx, xfn::XmlCursor_ReadVec3, {e, 0xC7400937u, eu, vz});
        const f64 ex = MulS(LoadF(eu), deg), ey = MulS(LoadF(eu + 4), deg), ez = MulS(LoadF(eu + 8), deg);
        mid12::EulerQuatAt(ctx, nd + 144, ex, ey, ez, half);
        call(ctx, xfn::XmlCursor_ReadHash, {e, 0xF91B96E2u, nd + 168, 0});
        call(ctx, xfn::XmlCursor_ReadBool, {e, 0xD1A6825Eu, b, 0});
        store<u32>(nd + 176, PutBit(load<u32>(nd + 176), 3, load<u8>(b)));
        call(ctx, xfn::XmlCursor_ReadBool, {e, 0x36E7D729u, b, 0});
        store<u32>(nd + 176, PutBit(load<u32>(nd + 176), 0, load<u8>(b)));
        call(ctx, xfn::XmlCursor_ReadBool, {e, 0xEAE49DB2u, b, 0});
        store<u32>(nd + 176, PutBit(load<u32>(nd + 176), 7, load<u8>(b)));
        u32 v = load<u32>(slot);
        if (v == 0) {
            v = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {20}));
            store<u32>(slot, v);
            if (v != 0) {
                call(ctx, m1fn::PtrVector_Construct, {v});
                store<u16>(v + 6, 192);
                v = load<u32>(slot);
            }
        }
        call(ctx, 0x00733964, {v, nd});  // Vector_Add733964
        const u32 vv = load<u32>(slot);
        const u32 last = load<u32>(vv) + (static_cast<u32>(load<u16>(vv + 8)) - 1u) * 192u;
        call(ctx, 0x000C9AB8, {a0, e, last + 180});
        mid1::PtrVecDeleteSmall(ctx, nd + 180);
        e = static_cast<u32>(call(ctx, mid2::m2fn::XmlCursor_NextSibling, {e, kTag}));
    } while (e != 0);
    ret(ctx, 0u);
}
