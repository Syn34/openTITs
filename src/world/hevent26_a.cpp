// World_HandleEvent callees, part 26a: the scroller object constructor of
// the level spawn table.
#include <bit>

#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kObjBaseConstruct = 0x0035695C;  // (Object*, u32 kind, Def*) (symbols.txt Obj_BaseConstruct35695C)
constexpr u32 kStringFormat = 0x005534D0;      // (fmt, ...) -> const char* (symbols.txt)
constexpr u32 kStringHashNoCase = 0x0056A710;  // (const char*) -> u32 (symbols.txt)
constexpr u32 kFindById = 0x004A5AE8;          // (WorldObj*, s32 id) -> WorldObj* (WorldObj_FindById4A5AE8)
constexpr u32 kMemAlloc = 0x0054ED38;          // (size, 0, align) (symbols.txt Mem_Alloc)
constexpr u32 kMemset = 0x00726E54;            // (dst, c, n) -> dst
constexpr u32 kPtrVectorConstruct = 0x005516E0;  // (PtrVector*) (symbols.txt PtrVector_Construct)
constexpr u32 kGeomFindByHash = 0x005801A0;    // (u32 hash) -> GeomInstance* (symbols.txt GeomInstance_FindByHash)
constexpr u32 kCollectLodGroup = 0x00590FEC;   // (Obj*, u8 recurse) (symbols.txt Obj_CollectLodGroup)
constexpr u32 kVtable358760 = 0x00B1F950;
constexpr u32 kTag358760 = 0x01164CA4;         // 4 bytes copied to +228
// "%s_Intro" / "%s_OnUpdate" / "%s_Idle" / "%s_Outro" / "%s_ScrollUp" / "%s_ScrollDn"
constexpr u32 kFmtIntro = 0x00ADC768, kFmtOnUpdate = 0x00ADC778, kFmtIdle = 0x00ADC788, kFmtOutro = 0x00ADC7A8;
constexpr u32 kFmtScrollUp = 0x00ADC930, kFmtScrollDn = 0x00ADC940;
constexpr u32 kVtable359E08 = 0x00B1F870;
constexpr u32 kFindByHash = 0x005801DC;       // (u32 hash, s32 kind) -> WorldObj* (symbols.txt WorldObj_FindByHash)
constexpr u32 kTexFindLoaded = 0x0050C398;    // (u32 name) -> Texture* (symbols.txt Tex_FindLoaded)
constexpr u32 kTexAcquire = 0x00523B7C;       // (Texture*) -> TexRef* (symbols.txt Tex_Acquire)
constexpr u32 kZeroVec = 0x01164B40;          // vec3 {0, 0, 0}
constexpr u32 kFltMax = 0x7F7FFFFFu, kFltMaxNeg = 0xFF7FFFFFu;
constexpr u32 kFmtNameIndex = 0x00ADC760;     // "%s_%i"
constexpr u32 kVtable35A868 = 0x00B1F6B0;
constexpr u32 kUiFind = 0x004D5544;           // (const char* path, Ui* root) -> Ui* (symbols.txt Ui_Find)
constexpr u32 kUiCloneCounter = 0x00F64F84;   // s32: suffix of the next "<name>_<n>" clone (name?)


// Shared by the two scroller constructors: the d +184 target ids (28-byte
// records at d +180, id at +8) resolved into a new u32 array at +232.
void scroller_targets(PPUContext& ctx, u32 o, u32 d) {
    if (load<u32>(d + 184) == 0) return;
    const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {load<u32>(d + 184) << 2, 0, 16}));
    const u32 arr = static_cast<u32>(call(ctx, kMemset, {p, 0, load<u32>(d + 184) << 2}));
    store<u32>(o + 232, arr);
    for (u32 i = 0; i < load<u32>(d + 184); ++i) {
        const u32 id = load<u32>(load<u32>(d + 180) + 28 * i + 8);
        const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), id}));
        store<u32>(load<u32>(o + 232) + 4 * i, r);
    }
}
// +200..+224 zeroed, then with a name (d +36, else d +4) +204 / +208 / +212
// / +224 / +216 / +220 = the hashes of "<name>_Intro" / "_OnUpdate" /
// "_Idle" / "_Outro" / "_ScrollUp" / "_ScrollDn".
void scroller_hashes(PPUContext& ctx, u32 o, u32 d) {
    for (u32 k = 0; k < 28; k += 4) store<u32>(o + 200 + k, 0);
    u32 name = d + 36;
    if (load<u8>(name) == 0) name = d + 4;
    if (name == 0 || load<u8>(name) == 0) return;
    const u32 fmts[6] = {kFmtIntro, kFmtOnUpdate, kFmtIdle, kFmtOutro, kFmtScrollUp, kFmtScrollDn};
    const u32 offs[6] = {204, 208, 212, 224, 216, 220};
    for (u32 k = 0; k < 6; ++k) {
        const u32 s = static_cast<u32>(call(ctx, kStringFormat, {fmts[k], name}));
        store<u32>(o + offs[k], static_cast<u32>(call(ctx, kStringHashNoCase, {s})));
    }
}
// The geometry instance of hash d +172 (the same hash each time) through
// its vt +56 (5), that object's vt +40 (0, 1) and Obj_CollectLodGroup(..,
// 1) into n slots from slot0; 0 without one.
void scroller_geoms(PPUContext& ctx, u32 slot0, u32 d, u32 n) {
    for (u32 k = 0; k < n; ++k) {
        const u32 slot = slot0 + 4 * k;
        const u32 g = static_cast<u32>(call(ctx, kGeomFindByHash, {load<u32>(d + 172)}));
        if (g == 0) {
            store<u32>(slot, 0);
            continue;
        }
        const u32 inst = static_cast<u32>(vcall(ctx, g, 56, {g, 5}));
        store<u32>(slot, inst);
        vcall(ctx, inst, 40, {inst, 0, 1});
        call(ctx, kCollectLodGroup, {load<u32>(slot), 1});
    }
}
}  // namespace

// ---------------------------------------------------------------- 0x00358760
// void Scroller_Construct358760(Object* o, Def* d) (name?): base object
// (kind 0), vtable 0x00B1F950, +228 = the 4 bytes at 0x01164CA4; +252 bits
// 31 and 30 = d +192 bit 0 (both), bits 19..29 cleared; +236 = d +188 (f32),
// +240 = -1.0f, +260 = d +152, other words +232..+276 zeroed; PtrVector at
// +304 with +310 = 20; +4 |= 0x2000, +164 = 0. d +184 ids (28-byte records
// at d +180, id at +8) resolved into a new u32 array at +232. +200..+224
// zeroed, then with a name (d +36, else d +4) +204 / +208 / +212 / +224 /
// +216 / +220 the hashes of "<name>_Intro" / "_OnUpdate" / "_Idle" /
// "_Outro" / "_ScrollUp" / "_ScrollDn". Six slots +280..+300: the geometry
// instance of hash d +172 (the same hash each time) through its vt +56 (5),
// that object's vt +40 (0, 1) and Obj_CollectLodGroup(.., 1); 0 without
// one. +268 / +272 / +276: the objects of ids d +156 / +160 / +164.
DECOMP(00358760, Scroller_Construct358760) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    store<u32>(o, kVtable358760);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 228 + k, load<u8>(kTag358760 + k));
    u32 w = (load<u32>(o + 252) & 0x7FFFFFFFu) | ((static_cast<u32>(load<u8>(d + 192)) & 1u) << 31);
    const u32 scale = load<u32>(d + 188);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(scale));  // the original's lfs leaves it in f1
    store<u32>(o + 232, 0);
    store<u32>(o + 252, w);
    w = (w & ~0x40000000u) | ((static_cast<u32>(load<u8>(d + 192)) & 1u) << 30);
    store<u32>(o + 236, scale);
    store<u32>(o + 244, 0);
    w &= ~0x3FF80000u;  // bits 19..29
    store<u32>(o + 240, 0xBF800000u);  // -1.0f
    store<u32>(o + 256, 0);
    store<u32>(o + 268, 0);
    store<u32>(o + 252, w);
    store<u32>(o + 264, 0);
    store<u32>(o + 260, load<u32>(d + 152));
    store<u32>(o + 272, 0);
    store<u32>(o + 276, 0);
    call(ctx, kPtrVectorConstruct, {o + 304});
    store<u16>(o + 310, 20);
    store<u32>(o + 4, load<u32>(o + 4) | 0x2000u);
    store<u8>(o + 164, 0);

    scroller_targets(ctx, o, d);
    scroller_hashes(ctx, o, d);

    for (u32 k = 0; k < 24; k += 8) store<u64>(o + 280 + k, 0);
    scroller_geoms(ctx, o + 280, d, 6);

    for (u32 k = 0; k < 3; ++k) {
        const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(d + 156 + 4 * k)}));
        store<u32>(o + 268 + 4 * k, r);
    }
    ret(ctx, load<u32>(o + 276));  // r3: the last lookup's result
}

// ---------------------------------------------------------------- 0x0035A868
// void UiObj_Construct35A868(Object* o, Def* d) (name?): base object (kind
// 0), vtable 0x00B1F6B0, +316 / +320 = 0, +324 = the 4 bytes at 0x01164CA4;
// +396 bits 31 and 30 = d +160 bit 0, bits 21..29 and 18..19 cleared;
// +332 = d +156 (f32), +336 = -1.0f, +392 = 1.0f, +328 / +340 = 0, +164 =
// 0. d +152 32-byte records (d +148) become 16-byte entries in a new array
// at +328: +0 the object of id rec +4; without rec +28 bit 31, +4..+12 =
// rec +8..+16; with it and a template (rec +8): +4 = template vt +8
// ("<template +96>_<n>" with n from 0x00F64F84 pre-incremented, template
// +148, +160, 1), and when made: +8 = Ui_Find(rec +12 +96, clone) when rec
// +12, +12 = Ui_Find(rec +16 +96, +8 or the clone) when its vt +12 kind is
// 3 or (asked again) 5. +296..+312 zeroed, then with a name (d +36, else d
// +4) +300 / +304 / +308 / +312 the hashes of "<name>_Intro" / "_OnUpdate"
// / "_Idle" / "_Outro".
DECOMP(0035A868, UiObj_Construct35A868) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    store<u32>(o, kVtable35A868);
    store<u32>(o + 316, 0);
    store<u32>(o + 320, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 324 + k, load<u8>(kTag358760 + k));
    u32 w = load<u32>(o + 396);
    const u32 scale = load<u32>(d + 156);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(scale));  // the original's lfs leaves it in f1
    w = (w & 0x7FFFFFFFu) | ((static_cast<u32>(load<u8>(d + 160)) & 1u) << 31);
    store<u32>(o + 396, w);
    store<u32>(o + 328, 0);
    w = (w & ~0x40000000u) | ((static_cast<u32>(load<u8>(d + 160)) & 1u) << 30);
    store<u32>(o + 332, scale);
    store<u32>(o + 340, 0);
    store<u32>(o + 392, 0x3F800000u);  // 1.0f
    store<u32>(o + 336, 0xBF800000u);  // -1.0f
    store<u8>(o + 164, 0);
    w &= ~0x3FEC0000u;  // bits 21..29, 18, 19 (bit 20 kept)
    store<u32>(o + 396, w);

    if (load<u32>(d + 152) != 0) {
        const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {load<u32>(d + 152) << 4, 0, 16}));
        const u32 arr = static_cast<u32>(call(ctx, kMemset, {p, 0, load<u32>(d + 152) << 4}));
        store<u32>(o + 328, arr);
        for (u32 i = 0; i < load<u32>(d + 152); ++i) {
            const u32 rec = load<u32>(d + 148) + 32 * i;
            const u32 dst = load<u32>(o + 328) + 16 * i;
            store<u32>(dst, static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(rec + 4)})));
            if ((load<u32>(rec + 28) & 0x80000000u) == 0) {
                for (u32 k = 4; k < 16; k += 4) store<u32>(dst + k, load<u32>(rec + 4 + k));
                continue;
            }
            if (load<u32>(rec + 8) == 0) continue;
            const u32 tmpl = load<u32>(rec + 8);
            const u32 n = load<u32>(kUiCloneCounter) + 1;
            store<u32>(kUiCloneCounter, n);
            const u32 name = static_cast<u32>(call(ctx, kStringFormat, {kFmtNameIndex, tmpl + 96, static_cast<u64>(static_cast<s64>(static_cast<s32>(n)))}));
            const u32 t2 = load<u32>(rec + 8);
            const u32 clone = static_cast<u32>(vcall(ctx, tmpl, 8, {tmpl, name, load<u32>(t2 + 148), load<u32>(t2 + 160), 1}));
            store<u32>(dst + 4, clone);
            if (clone == 0) continue;
            u32 root = load<u32>(dst + 8);
            if (load<u32>(rec + 12) != 0) {
                root = static_cast<u32>(call(ctx, kUiFind, {load<u32>(rec + 12) + 96, clone}));
                store<u32>(dst + 8, root);
            }
            if (root == 0) root = load<u32>(dst + 4);
            const u32 path = load<u32>(rec + 16);
            if (path == 0) continue;
            const u32 u = static_cast<u32>(call(ctx, kUiFind, {path + 96, root}));
            if (u == 0) continue;
            if (static_cast<s32>(vcall(ctx, u, 12, {u})) == 3 || static_cast<s32>(vcall(ctx, u, 12, {u})) == 5) store<u32>(dst + 12, u);
        }
    }

    for (u32 k = 0; k < 20; k += 4) store<u32>(o + 296 + k, 0);
    u32 nm = d + 36;
    u32 c = load<u8>(nm);
    if (c == 0) nm = d + 4;
    if (nm == 0 || (c = load<u8>(nm)) == 0) {
        ret(ctx, c);
        return;
    }
    const u32 fmts[4] = {kFmtIntro, kFmtOnUpdate, kFmtIdle, kFmtOutro};
    for (u32 k = 0; k < 4; ++k) {
        const u32 s = static_cast<u32>(call(ctx, kStringFormat, {fmts[k], nm}));
        store<u32>(o + 300 + 4 * k, static_cast<u32>(call(ctx, kStringHashNoCase, {s})));
    }
}

// ---------------------------------------------------------------- 0x00359E08
namespace {
void vzero(u32 ea) {  // stvx of a zero vector
    ea &= ~15u;
    for (u32 k = 0; k < 16; k += 4) store<u32>(ea + k, 0);
}
}  // namespace

// void PathObj_Construct359E08(Object* o, Def* d) (name?): base object
// (kind 0), vtable 0x00B1F870; +288 vec3 FLT_MAX, +304 vec3 -FLT_MAX, +320
// = the zero vec3, +164..+172 / +192 / +336..+358 zeroed, +360 bits 22..31
// cleared, +4 |= 0x200. d +152 16-byte records (d +148: +4 u32 ids at a
// 16-byte stride, +8 count) become a u32* array at +164 of per-record
// arrays of the objects of those ids. +176..+191 and +208..+287 zeroed,
// +176[4] = the kind-8 objects of hashes d +172[k*2]; +288 = A +128, +304
// = B +128 and +320 = A +128 - C +128 for A, B, C = +176, +188, +184 (read
// unchecked). +172 = Tex_Acquire of texture d +164 (after its vt +16 (1, 1,
// 1)) with render state +104: bit 31 set, bits 15..30 = 770, +4 = 0x8006,
// +6 = 771, +12 = 0. +192 = the owner's (+24) +272 child (+268 chain) of id
// d +212. +362 / +490 = 0; returns that child or 0.
DECOMP(00359E08, PathObj_Construct359E08) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    u32 w = load<u32>(o + 360);
    store<u32>(o + 288, kFltMax);
    store<u32>(o + 292, kFltMax);
    store<u32>(o + 296, kFltMax);
    store<u32>(o + 304, kFltMaxNeg);
    store<u32>(o, kVtable359E08);
    store<u32>(o + 308, kFltMaxNeg);
    store<u32>(o + 312, kFltMaxNeg);
    store<u32>(o + 164, 0);
    store<u32>(o + 168, 0);
    store<u32>(o + 172, 0);
    store<u32>(o + 192, 0);
    store<u32>(o + 320, load<u32>(kZeroVec + 0));
    store<u32>(o + 336, 0);
    store<u32>(o + 324, load<u32>(kZeroVec + 4));
    store<u32>(o + 328, load<u32>(kZeroVec + 8));
    for (u32 k = 340; k <= 352; k += 4) store<u32>(o + k, 0);
    store<u16>(o + 356, 0);
    store<u16>(o + 358, 0);
    w &= 0x003FFFFFu;  // bits 22..31
    store<u32>(o + 360, w);
    store<u32>(o + 4, load<u32>(o + 4) | 0x200u);

    if (load<u32>(d + 152) != 0) {
        const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {load<u32>(d + 152) << 2, 0, 16}));
        store<u32>(o + 164, static_cast<u32>(call(ctx, kMemset, {p, 0, load<u32>(d + 152) << 2})));
        for (u32 i = 0; i < load<u32>(d + 152); ++i) {
            const u32 rec = load<u32>(d + 148) + 16 * i;
            const u32 n = load<u32>(rec + 8);
            if (n == 0) continue;
            const u32 slot = load<u32>(o + 164) + 4 * i;
            const u32 q = static_cast<u32>(call(ctx, kMemAlloc, {n << 2, 0, 16}));
            store<u32>(slot, static_cast<u32>(call(ctx, kMemset, {q, 0, load<u32>(rec + 8) << 2})));
            for (u32 j = 0; j < load<u32>(rec + 8); ++j) {
                const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(load<u32>(rec + 4) + 16 * j)}));
                store<u32>(load<u32>(slot) + 4 * j, r);
            }
        }
    }

    vzero(o + 176);
    for (u32 k = 0; k < 80; k += 16) vzero(o + 208 + k);
    for (u32 k = 0; k < 4; ++k) {
        const u32 h = load<u32>(d + 172 + 8 * k);
        if (h != 0) store<u32>(o + 176 + 4 * k, static_cast<u32>(call(ctx, kFindByHash, {h, 8})));
    }

    const u32 pa = load<u32>(o + 176) + 128, pb = load<u32>(o + 188) + 128, pc = load<u32>(o + 184) + 128;
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 288 + k, load<u32>(pa + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 304 + k, load<u32>(pb + k));
    f32 diff = 0;
    for (u32 k = 0; k < 12; k += 4) {
        diff = static_cast<f32>(static_cast<f64>(std::bit_cast<f32>(load<u32>(pa + k))) - static_cast<f64>(std::bit_cast<f32>(load<u32>(pc + k))));
        if (k == 0) ctx.fpr[1] = diff;  // f1 keeps the x difference, as the original's
        store<f32>(o + 320 + k, diff);
    }

    const u32 tex = static_cast<u32>(call(ctx, kTexFindLoaded, {load<u32>(d + 164)}));
    vcall(ctx, tex, 16, {tex, 1, 1, 1});
    const u32 ref = static_cast<u32>(call(ctx, kTexAcquire, {tex}));
    store<u32>(o + 172, ref);
    const u32 rs = ref + 104;
    const u32 rs0 = load<u32>(rs) | 0x80000000u;
    store<u16>(rs + 4, 0x8006);
    store<u16>(rs + 6, 771);
    store<u32>(rs + 0, (rs0 & ~0x7FFF8000u) | (770u << 15));
    store<u16>(load<u32>(o + 172) + 104 + 12, 0);

    u32 x = 0;
    if (load<u32>(o + 24) != 0) {
        x = load<u32>(load<u32>(o + 24) + 272);
        while (x != 0 && load<u32>(x + 204) != load<u32>(d + 212)) x = load<u32>(x + 268);
        if (x != 0) store<u32>(o + 192, x);
    }
    store<u8>(o + 362, 0);
    store<u8>(o + 490, 0);
    ret(ctx, x);
}

// ---------------------------------------------------------------- 0x00358B64
namespace {
constexpr u32 kVtable358B64 = 0x00B1F918;
// Four u32[6] hash tables of the second scroller's part objects (kind 0).
constexpr u32 kPartHashesA = 0x00F64E58, kPartHashesB = 0x00F64E70, kPartHashesC = 0x00F64EA0, kPartHashesD = 0x00F64EE8;
// Three template geometry objects copied six times (vt +56 (5)).
constexpr u32 kTemplateA = 0xEF3B431Eu, kTemplateB = 0x04DABC6Fu, kTemplateC = 0x15AA444Au;
u32 clone5(PPUContext& ctx, u32 t) { return t != 0 ? static_cast<u32>(vcall(ctx, t, 56, {t, 5})) : 0u; }
}  // namespace

// void ScrollerB_Construct358B64(Object* o, Def* d) (name?): base object
// (kind 0), vtable 0x00B1F918, +228 = the 4 bytes at 0x01164CA4; +252 bits
// 31 and 30 = d +192 bit 0, bits 19..29 cleared; +236 = d +188, +240 =
// -1.0f, +264 = d +152, other words +232..+284 zeroed; +4 |= 0x1000, +164 =
// 0. Targets at +232 and the six name hashes as Scroller_Construct358760.
// +288..+479 zeroed, +400 = 0, +404 = 1; +288[4] the d +172 geometry
// instances; +304 / +328 / +352 / +376 [6] the objects of the hash tables
// at 0x00F64E58 / 0x00F64E70 / 0x00F64EA0 / 0x00F64EE8; +408 / +432 / +456
// [6] clones (vt +56 (5)) of three template objects, each collected with
// Obj_CollectLodGroup(.., 1). +272..+284: the objects of ids d +156..+168.
DECOMP(00358B64, ScrollerB_Construct358B64) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    store<u32>(o, kVtable358B64);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 228 + k, load<u8>(kTag358760 + k));
    u32 w = (load<u32>(o + 252) & 0x7FFFFFFFu) | ((static_cast<u32>(load<u8>(d + 192)) & 1u) << 31);
    const u32 scale = load<u32>(d + 188);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(scale));  // the original's lfs leaves it in f1
    store<u32>(o + 232, 0);
    store<u32>(o + 252, w);
    w = (w & ~0x40000000u) | ((static_cast<u32>(load<u8>(d + 192)) & 1u) << 30);
    store<u32>(o + 236, scale);
    store<u32>(o + 244, 0);
    store<u32>(o + 240, 0xBF800000u);  // -1.0f
    store<u32>(o + 256, 0);
    store<u32>(o + 260, 0);
    store<u32>(o + 272, 0);
    store<u32>(o + 276, 0);
    w &= ~0x3FF80000u;  // bits 19..29
    store<u32>(o + 252, w);
    store<u32>(o + 268, 0);
    store<u32>(o + 264, load<u32>(d + 152));
    store<u32>(o + 280, 0);
    store<u32>(o + 284, 0);
    store<u32>(o + 4, load<u32>(o + 4) | 0x1000u);
    store<u8>(o + 164, 0);

    scroller_targets(ctx, o, d);
    scroller_hashes(ctx, o, d);

    for (u32 k = 0; k < 16; k += 4) store<u32>(((o + 288) & ~15u) + k, 0);  // stvx
    for (u32 k = 304; k < 480; k += 8) {
        if (k == 400) k = 408;  // +400 / +404 are set below
        store<u64>(o + k, 0);
    }
    store<u32>(o + 400, 0);
    store<u32>(o + 404, 1);
    scroller_geoms(ctx, o + 288, d, 4);

    for (u32 k = 0; k < 6; ++k) {
        store<u32>(o + 304 + 4 * k, static_cast<u32>(call(ctx, kFindByHash, {load<u32>(kPartHashesA + 4 * k), 0})));
        store<u32>(o + 328 + 4 * k, static_cast<u32>(call(ctx, kFindByHash, {load<u32>(kPartHashesB + 4 * k), 0})));
        store<u32>(o + 352 + 4 * k, static_cast<u32>(call(ctx, kFindByHash, {load<u32>(kPartHashesC + 4 * k), 0})));
        store<u32>(o + 376 + 4 * k, static_cast<u32>(call(ctx, kFindByHash, {load<u32>(kPartHashesD + 4 * k), 0})));
    }
    const u32 ta = static_cast<u32>(call(ctx, kFindByHash, {kTemplateA, 0}));
    const u32 tb = static_cast<u32>(call(ctx, kFindByHash, {kTemplateB, 0}));
    const u32 tc = static_cast<u32>(call(ctx, kFindByHash, {kTemplateC, 0}));
    for (u32 k = 0; k < 6; ++k) {
        const u32 sa = o + 408 + 4 * k, sb = o + 432 + 4 * k, sc = o + 456 + 4 * k;
        store<u32>(sa, clone5(ctx, ta));
        store<u32>(sb, clone5(ctx, tb));
        store<u32>(sc, clone5(ctx, tc));
        for (const u32 s : {sa, sb, sc}) {
            const u32 x = load<u32>(s);
            if (x != 0) call(ctx, kCollectLodGroup, {x, 1});
        }
    }

    for (u32 k = 0; k < 4; ++k) {
        const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(d + 156 + 4 * k)}));
        store<u32>(o + 272 + 4 * k, r);
    }
    ret(ctx, load<u32>(o + 284));  // r3: the last lookup's result
}
