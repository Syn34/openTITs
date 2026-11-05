// World_HandleEvent callees, part 17c: player aim UI set-up and the player
// colour blend.
#include "hevent17.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent17;

namespace {
// Player: +216 def (+1456 aim arc object hash, +1460 aim endpoint object
// hash); +484 arc WorldObj instance, +488 endpoint instance.
constexpr u32 kPlDef = 216, kDefArcName = 1456, kDefEndName = 1460, kPlArc = 484, kPlEnd = 488;
// Geometry definitions: *0x00F8F12C +16 map searched with HashMap_Find2.
constexpr u32 kGeomDefs = 0x00F8F12C, kGeomMap = 16;
constexpr u32 kHashMapFind2 = 0x007399A4;      // (map, const u32* key) -> slot (symbols.txt)
constexpr u32 kWorldObjInstance = 0x005A05E8;  // WorldObj_Instance (obj, u8) -> instance (symbols.txt)
constexpr u32 kHashToString = 0x0056A6E0;      // (u32) -> const char* (symbols.txt)
constexpr u32 kStringFormat = 0x005534D0;      // (fmt, ...) -> static buffer (symbols.txt)
constexpr u32 kStrNoEndpoint = 0x00ACA878;     // "couldn't find aim UI endpoint object '%s'."
constexpr u32 kStrNoArc = 0x00ACA850;          // "couldn't find aim UI arc object '%s'."
// Instance vt +40 show(i, 0, 0); +208 Model* (+112 u8 part count, parts of
// 8 bytes from +132: Part* (+48 {u32, Mat*}[], +52 u16 count)); Mat +64
// f32 rgba.
constexpr u32 kInstVtShow = 40, kInstModel = 208;
constexpr u32 kModelParts = 112, kModelPart = 132, kPartMats = 48, kPartMatCount = 52, kMatColour = 64;
// Def vt +16: colour source; 0x004883C0 (u8 out[4], World*, src, u32 rgba).
constexpr u32 kDefVtColour = 16;
constexpr u32 kColourOf4883C0 = 0x004883C0;  // (u8 out[4], World*, src, u32 rgba) (name?)
constexpr u32 kAimColour = 0x01164CA0;       // u8[4] default rgba
constexpr u32 kInv255 = 0x0011FCB8;          // f32 1/255

// Player colour: +544 u8 rgba current, +548 u8 rgba target, +552 f32 value,
// +556 f32 target value; +160 WorldObj*; +184 u64 flags (bit 33: hidden,
// the word at +184 bit 1).
constexpr u32 kPlColour = 544, kPlColourTarget = 548, kPlValue = 552, kPlValueTarget = 556;
constexpr u32 kPlWorldObj = 160, kPlFlags184 = 184, kPlHidden = 0x2;
// 0x0011FFE0: f32 {1.0, 1/255, 255.0, 0.0}.
constexpr u32 kBlendK = 0x0011FFE0;
constexpr u32 kBlendRate = 0x01039318 + 72;  // f32 tuning
constexpr u32 kEase4A8320 = 0x004A8320;      // (; f32 x, f32 t) -> f32 (name?)
constexpr u32 kSetColourD6F90 = 0x000D6F90;  // (obj, const u8 rgba[4]; f32 value) (name?)
// Component 21: +2912 Object*[], +2944 u16 count.
constexpr u32 kCompType21 = 21, kCompObjs = 2912, kCompObjCount = 2944;

using hevent7::lfs_f64;
}  // namespace

// u32 Player_InitAimUi11FCC0(Player* p) (name?; was Player_ResetState11FCC0):
// instantiates the aim UI arc (def +1456, tinting its parts' materials with
// the colour 0x004883C0 picks) and endpoint (def +1460) objects when not
// yet made, reporting missing ones with String_Format.
DECOMP(0011FCC0, Player_InitAimUi11FCC0) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 d = load<u32>(p + kPlDef);
    Stack frame(ctx, 16);  // key words (r1 +112 / +116) and the colour (r1 +120)
    const u32 key = frame.addr(), out = frame.addr() + 8;
    if (load<u32>(p + kPlArc) == 0) {
        store<u32>(key + 4, load<u32>(d + kDefArcName));
        const u32 slot = static_cast<u32>(call(ctx, kHashMapFind2, {load<u32>(load<u32>(kGeomDefs) + kGeomMap), key + 4}));
        const u32 arc = load<u32>(slot);
        if (arc == 0) {
            const u32 h = load<u32>(d + kDefArcName);
            if (h != 0) call(ctx, kStringFormat, {kStrNoArc, call(ctx, kHashToString, {h})});
        } else {
            const u32 x = static_cast<u32>(call(ctx, kWorldObjInstance, {arc, 1}));
            store<u32>(p + kPlArc, x);
            const u32 model = load<u32>(x + kInstModel);
            vcall(ctx, x, kInstVtShow, {x, 0, 0});
            const u32 pd = load<u32>(p + kPlDef);
            const u32 w = load<u32>(kCurrentWorld);
            const u32 src = static_cast<u32>(vcall(ctx, pd, kDefVtColour, {pd}));
            call(ctx, kColourOf4883C0, {out, w, src, load<u32>(kAimColour)});
            const f64 k = lfs_f64(kInv255);
            u8 c[4];
            for (u32 j = 0; j < 4; ++j) c[j] = load<u8>(out + j);
            ctx.fpr[1] = k;  // f1 left holding 1/255 (no known caller reads it)
            for (u32 i = 0; i < load<u8>(model + kModelParts); ++i) {
                for (u32 m = 0;; ++m) {
                    const u32 part = load<u32>(model + kModelPart + 8 * i);
                    if (m >= load<u16>(part + kPartMatCount)) break;
                    const u32 mat = load<u32>(load<u32>(part + kPartMats) + 8 * m + 4);
                    for (u32 j = 0; j < 4; ++j) store<f32>(mat + kMatColour + 4 * j, fmuls(static_cast<f64>(c[j]), k));
                }
            }
        }
    }
    if (load<u32>(p + kPlEnd) != 0) return ret(ctx, load<u32>(p + kPlEnd));
    store<u32>(key, load<u32>(d + kDefEndName));
    const u32 slot = static_cast<u32>(call(ctx, kHashMapFind2, {load<u32>(load<u32>(kGeomDefs) + kGeomMap), key}));
    const u32 e = load<u32>(slot);
    if (e != 0) {
        const u32 x = static_cast<u32>(call(ctx, kWorldObjInstance, {e, 0}));
        store<u32>(p + kPlEnd, x);
        vcall(ctx, x, kInstVtShow, {x, 0, 0});
        return;
    }
    const u32 h = load<u32>(d + kDefEndName);
    if (h == 0) return ret(ctx, 0u);
    call(ctx, kStringFormat, {kStrNoEndpoint, call(ctx, kHashToString, {h})});
}

// u32 Player_Blend11FFF0(Player* p, -, u8 force; f32 t) (name?): eases the
// colour (+544) towards the target (+548) and the value (+552) towards
// +556 by 1 - 0x004A8320 (1 - rate, t); unless nothing changed (and not
// forced), pushes them to every component-21 object, then to the player's
// WorldObj (black and 0.0 while hidden), and stores them.
DECOMP(0011FFF0, Player_Blend11FFF0) {
    const u32 p = arg<u32>(ctx, 0);
    const u8 force = arg<u8>(ctx, 2);
    const f64 t = farg(ctx, 0);
    const f64 k1 = lfs_f64(kBlendK), kInv = lfs_f64(kBlendK + 4), k255 = lfs_f64(kBlendK + 8);
    // r3 / r4 hold the constants' bases (the callee reads only f1, f2).
    const f64 a = callf(ctx, kEase4A8320, {kBlendRate - 72, 0x00120000, arg<u32>(ctx, 2)},
                        {static_cast<f32>(k1 - lfs_f64(kBlendRate)), t});
    const f64 w = static_cast<f32>(k1 - a);  // fsubs
    u8 cur[4], tgt[4], out[4];
    for (u32 j = 0; j < 4; ++j) cur[j] = load<u8>(p + kPlColour + j), tgt[j] = load<u8>(p + kPlColourTarget + j);
    for (u32 j = 0; j < 4; ++j) {
        const f64 as = fmuls(static_cast<f64>(cur[j]), kInv);
        const f64 x = static_cast<f32>(std::fma(static_cast<f64>(tgt[j]), kInv, -as));  // fmsubs
        const f64 y = static_cast<f32>(std::fma(x, w, as));                              // fmadds
        out[j] = static_cast<u8>(rt::fctiw(fmuls(y, k255), true));
    }
    const f64 v0 = lfs_f64(p + kPlValue);
    const f64 d = static_cast<f32>(lfs_f64(p + kPlValueTarget) - v0);  // fsubs
    const f64 v = static_cast<f32>(std::fma(d, w, v0));                 // fmadds
    if (force == 0) {
        bool same = true;
        for (u32 j = 0; j < 4; ++j) same = same && out[j] == cur[j];
        if (same && v == v0) {
            ctx.fpr[1] = v0;
            // r3: both colours packed as (3, 0, 1, 2) for the compare.
            return ret(ctx, (u32(cur[3]) << 24) | (u32(cur[0]) << 16) | (u32(cur[1]) << 8) | cur[2]);
        }
    }
    Stack frame(ctx, 16);  // the colour (r1 +112) and black (r1 +116)
    const u32 col = frame.addr(), black = frame.addr() + 4;
    for (u32 j = 0; j < 4; ++j) store<u8>(col + j, out[j]);
    for (u32 c = static_cast<u32>(call(ctx, hevent16::h16fn::Obj_FindComponent, {p, kCompType21, 0})); c != 0;
         c = static_cast<u32>(call(ctx, hevent16::h16fn::Obj_FindComponent, {p, kCompType21, c}))) {
        for (u32 k = 0; k < load<u16>(c + kCompObjCount); ++k) {
            const u32 o = load<u32>(c + kCompObjs + 4 * k);
            if (o != 0) callf(ctx, kSetColourD6F90, {o, col}, {v});
        }
    }
    if ((load<u32>(p + kPlFlags184) & kPlHidden) != 0) {
        for (u32 j = 0; j < 4; ++j) store<u8>(black + j, 0);
        callf(ctx, kSetColourD6F90, {load<u32>(p + kPlWorldObj), black}, {lfs_f64(kBlendK + 12)});
    } else {
        callf(ctx, kSetColourD6F90, {load<u32>(p + kPlWorldObj), col}, {v});
    }
    for (u32 j = 0; j < 4; ++j) store<u8>(p + kPlColour + j, load<u8>(col + j));
    store<f32>(p + kPlValue, static_cast<f32>(v));
    ret(ctx, static_cast<u32>(load<u8>(col + 3)));
}
