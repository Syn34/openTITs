// World_HandleEvent callees, part 27e: another scroller-style object
// constructor of the level spawn table (0x0035AC7C).
#include "hevent27.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;

namespace {
constexpr u32 kObjBaseConstruct = 0x0035695C;  // (Object*, u32 kind, Def*) (Obj_BaseConstruct35695C)
constexpr u32 kStringFormat = 0x005534D0;      // (fmt, ...) -> const char*
constexpr u32 kStringHashNoCase = 0x0056A710;  // (const char*) -> u32
constexpr u32 kFindById = 0x004A5AE8;          // (WorldObj*, s32 id) -> WorldObj* (WorldObj_FindById4A5AE8)
constexpr u32 kMemset = 0x00726E54;            // (dst, c, n) -> dst
constexpr u32 kPtrVectorConstruct = 0x005516E0;  // (PtrVector*)
constexpr u32 kGeomFindByHash = 0x005801A0;    // (u32 hash) -> GeomInstance*
constexpr u32 kCollectLodGroup = 0x00590FEC;   // (Obj*, u8 recurse)
constexpr u32 kVtable359284 = 0x00B1F8E0;
constexpr u32 kTag = 0x01164CA4;               // 4 bytes copied to +228 (as hevent26_a.cpp)
// "%s_Intro" / "%s_OnUpdate" / "%s_Idle" / "%s_Outro" / "%s_ScrollUp" / "%s_ScrollDn"
constexpr u32 kFmtIntro = 0x00ADC768, kFmtOnUpdate = 0x00ADC778, kFmtIdle = 0x00ADC788, kFmtOutro = 0x00ADC7A8;
constexpr u32 kFmtScrollUp = 0x00ADC930, kFmtScrollDn = 0x00ADC940;
constexpr u32 kFmtInt = 0x00ADC94C;            // "%d"
// Two tables of 6 object ids (u32[6]) resolved into +352 / +376.
constexpr u32 kIdsA = 0x00F64D88, kIdsB = 0x00F64DA0;
// Def record keys (d +184 28-byte records, key at +0, object at +20) whose
// object gets the "%d" text: 0 for the first, 2 for the second.
constexpr u32 kKeyText0 = 0xF17AEB11u, kKeyText2 = 0x10C350EEu;

// The def's +184 record keyed key (u32 +188 records): its +20 object gets
// vt +188 (text, 0) and then vt +148 (re-read). Returns the r3 it leaves.
u64 set_record_text(PPUContext& ctx, u32 d, u32 key, u32 text) {
    const u32 n = load<u32>(d + 188);
    if (n == 0) return text;
    const u32 recs = load<u32>(d + 184);
    u32 m = 0;
    while (static_cast<s32>(load<u32>(recs + 28 * m)) != static_cast<s32>(key)) {
        if (!(++m < n)) return text;
    }
    const u32 obj = load<u32>(recs + 28 * m + 20);
    if (obj == 0) return text;
    vcall(ctx, obj, 188, {obj, text, 0});
    const u32 obj2 = load<u32>(load<u32>(d + 184) + 28 * m + 20);
    return vcall(ctx, obj2, 148, {obj2});
}
}  // namespace

// ---------------------------------------------------------------- 0x00359284
// void ScrollerC_Construct359284(Object* o, Def* d) (name?): base object
// (kind 0), vtable 0x00B1F8E0, +228 = the 4 bytes at 0x01164CA4; +252 bits
// 31 and 30 = d +196 bit 0, bits 19..29 cleared; +236 = d +192 (f32), +240
// = -1.0f, +260 = d +152, other words +232..+284 zeroed; PtrVectors at +288
// (+294 = 4), +308 (+314 = 1) and +400 (+406 = 4); +4 |= 0x800, +164 = 0.
// d +188 ids (28-byte records at d +184, id at +8) resolved into a new u32
// array at +232; the six "<name>_..." hashes at +204..+224 (as
// Scroller_Construct358760). Six slots each: +328 the geometry instance of
// hash d +176 (vt +56 (5), vt +40 (0, 1), Obj_CollectLodGroup), +352 / +376
// the objects of the ids at 0x00F64D88 / 0x00F64DA0 (vt +40 (0, 1) when
// found). +268..+284: the objects of ids d +156..+172. Finally the def's
// records 0xF17AEB11 and 0x10C350EE get the texts "0" and "2".
DECOMP(00359284, ScrollerC_Construct359284) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    store<u32>(o, kVtable359284);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 228 + k, load<u8>(kTag + k));
    u32 w = (load<u32>(o + 252) & 0x7FFFFFFFu) | ((static_cast<u32>(load<u8>(d + 196)) & 1u) << 31);
    const u32 scale = load<u32>(d + 192);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(scale));  // the original's lfs leaves it in f1
    store<u32>(o + 232, 0);
    store<u32>(o + 252, w);
    w = (w & ~0x40000000u) | ((static_cast<u32>(load<u8>(d + 196)) & 1u) << 30);
    store<u32>(o + 236, scale);
    store<u32>(o + 244, 0);
    w &= ~0x3FF80000u;  // bits 19..29
    store<u32>(o + 240, 0xBF800000u);  // -1.0f
    store<u32>(o + 256, 0);
    store<u32>(o + 268, 0);
    store<u32>(o + 252, w);
    const u32 d152 = load<u32>(d + 152);
    store<u32>(o + 264, 0);
    store<u32>(o + 260, d152);
    for (u32 off : {272u, 276u, 280u, 284u}) store<u32>(o + off, 0);
    call(ctx, kPtrVectorConstruct, {o + 288});
    store<u16>(o + 294, 4);
    call(ctx, kPtrVectorConstruct, {o + 308});
    store<u16>(o + 314, 1);
    call(ctx, kPtrVectorConstruct, {o + 400});
    store<u16>(o + 406, 4);
    store<u32>(o + 4, load<u32>(o + 4) | 0x800u);
    store<u8>(o + 164, 0);

    if (load<u32>(d + 188) != 0) {
        const u32 p = static_cast<u32>(call(ctx, h27fn::Mem_Alloc, {load<u32>(d + 188) << 2, 0, 16}));
        const u32 arr = static_cast<u32>(call(ctx, kMemset, {p, 0, load<u32>(d + 188) << 2}));
        store<u32>(o + 232, arr);
        for (u32 i = 0; i < load<u32>(d + 188); ++i) {
            const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(load<u32>(d + 184) + 28 * i + 8)}));
            store<u32>(load<u32>(o + 232) + 4 * i, r);
        }
    }

    for (u32 k = 0; k < 28; k += 4) store<u32>(o + 200 + k, 0);
    u32 name = d + 36;
    if (load<u8>(name) == 0) name = d + 4;
    if (name != 0 && load<u8>(name) != 0) {
        const u32 fmts[6] = {kFmtIntro, kFmtOnUpdate, kFmtIdle, kFmtOutro, kFmtScrollUp, kFmtScrollDn};
        const u32 offs[6] = {204, 208, 212, 224, 216, 220};
        for (u32 k = 0; k < 6; ++k) {
            const u32 s = static_cast<u32>(call(ctx, kStringFormat, {fmts[k], name}));
            store<u32>(o + offs[k], static_cast<u32>(call(ctx, kStringHashNoCase, {s})));
        }
    }

    for (u32 base : {328u, 352u, 376u}) {
        for (u32 k = 0; k < 24; k += 8) store<u64>(o + base + k, 0);
    }
    for (u32 k = 0; k < 6; ++k) {
        const u32 slot = o + 328 + 4 * k;
        const u32 g = static_cast<u32>(call(ctx, kGeomFindByHash, {load<u32>(d + 176)}));
        if (g == 0) {
            store<u32>(slot, 0);
        } else {
            const u32 inst = static_cast<u32>(vcall(ctx, g, 56, {g, 5}));
            store<u32>(slot, inst);
            vcall(ctx, inst, 40, {inst, 0, 1});
            call(ctx, kCollectLodGroup, {load<u32>(slot), 1});
        }
        for (u32 t = 0; t < 2; ++t) {
            const u32 ids = t == 0 ? kIdsA : kIdsB;
            const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(ids + 4 * k)}));
            store<u32>(o + (t == 0 ? 352u : 376u) + 4 * k, r);
            if (r != 0) vcall(ctx, r, 40, {r, 0, 1});
        }
    }
    for (u32 k = 0; k < 5; ++k) {
        const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(d + 156 + 4 * k)}));
        store<u32>(o + 268 + 4 * k, r);
    }

    const u32 t0 = static_cast<u32>(call(ctx, kStringFormat, {kFmtInt, 0}));
    const u32 def0 = load<u32>(o + 8);
    if (t0 != 0) set_record_text(ctx, def0, kKeyText0, t0);
    const u32 t2 = static_cast<u32>(call(ctx, kStringFormat, {kFmtInt, 2}));
    const u32 def2 = load<u32>(o + 8);
    u64 r3 = t2;
    if (t2 != 0) r3 = set_record_text(ctx, def2, kKeyText2, t2);
    ctx.gpr[3] = r3;
}
