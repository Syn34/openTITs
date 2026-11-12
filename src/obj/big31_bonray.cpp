// Game code, large-function sweep part 31: a controller's aim ray (Havok
// ray cast along row 2 of its target matrix).
#include "../sly4.h"
#include "big31.h"

using namespace sly4;
using namespace sly4::big31;

namespace {
constexpr u32 kAimK = 0x002947B0;          // .text: f32 0 (hit threshold), 40.0 (ray length); lane masks y / z / w at
                                        // +16 / +32 / +48; f32 -1.0 (no hit) at +64
constexpr u32 kIdent = 0x01164BD0;      // the global identity matrix (rows at +0 / +16 / +32 / +48)
constexpr u32 kGame = 0x00F65CA0;       // Game* (+0x1281D4 the physics holder, +4 its world)
constexpr u32 kVtRayCol = 0x00F1DE80;   // all-hits ray collector vtable (80-byte hits, 8 inline)
constexpr u32 kHkHeap = 0x011DF31C;     // Havok heap allocator (vfn[20] (p, bytes) frees)
constexpr u32 kPropKind = 0xA734004D;   // shape property: the shape's kind
constexpr u32 kKindOwned = 0x7A30D7E3;  // that kind: the shape belongs to the object in kPropOwner
constexpr u32 kPropOwner = 0x91C7CF5B;  // shape property: the owning object
constexpr u32 BoneCtl_Target = 0x00250F40;         // (Ctl*, Out* m, u32 k, const f32* pos) -> m
constexpr u32 HkWorld_CastRay92E510 = 0x0092E510;  // (hkpWorld*, const RayInput*, Collector*)
constexpr u32 HkAllHits_Sort887E28 = 0x00887E28;   // (Collector*)

// The low word of property `key` in the shape's list (+124 the 16-byte
// entries {key, -, u64 value}, +128 the s32 count), 0 when absent.
u32 PropLo(u32 props, u32 key) {
    for (s32 j = 0; j < load<s32>(props + 128); ++j) {
        const u32 e = load<u32>(props + 124) + 16 * static_cast<u32>(j);
        if (load<u32>(e) == key) return load<u32>(e + 12);
    }
    return 0;
}
}  // namespace

// bool f(Ctl* c, f32 out[16]) (called by f_00295268 and f_00296328). The
// controller's target matrix M (sp + 224, set to the identity's rows
// first; its +64 source pointer is the 64 bytes at sp + 160, +68 = 0,
// u8 +72 = 0) comes from BoneCtl_Target(c, &M, c->+1264, 0). A ray from M's
// row 3 to row 3 + 40 * row 2 is cast (RayInput at sp + 112, collector at
// sp + 304 with eight inline 80-byte hits at sp + 336) and the hits
// sorted. The first hit whose shape (+68) type (+28 & 0x1F) is below 16
// and not 5 or 6, that is not an owned shape (byte +24 == 2), and whose
// owner (a +24 == 1 shape with kPropKind kKindOwned: its kPropOwner) is
// none or c->+8, gives the fraction. out's rows 0..2 = the identity's
// (x, y, z words); out row 3 = the hit point for a fraction above 0 (r3 =
// 1), else the ray's end (r3 = 0). A heap hit array is freed through
// kHkHeap->vfn[20] (the original passes entry / OPD in r6 / r7 without a
// hit and OPD / entry with one).
DECOMP(002947F8, BoneCtl_RayTarget_002947F8) {
    const u32 c = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    Stack frame(ctx, 896);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    const u32 m = sp + 224, ray = sp + 112, coll = sp + 304, hits = sp + 336;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(m + 16 * r + k, load<u32>(kIdent + 16 * r + k));
    const f64 thr = Lf(kAimK);
    store<u32>(sp + 292, 0);
    store<u8>(sp + 296, 0);
    store<u32>(sp + 288, sp + 160);
    call(ctx, BoneCtl_Target, {c, m, load<u32>(c + 1264), 0});
    const f64 len = Lf(kAimK + 4);
    const f64 sx = Lf(m + 48), sy = Lf(m + 52), sz = Lf(m + 56);
    const f64 ex = Fmadds(len, Lf(m + 32), sx), ey = Fmadds(len, Lf(m + 36), sy), ez = Fmadds(len, Lf(m + 40), sz);
    store<u8>(sp + 144, 0);
    store<u32>(sp + 152, 0);
    const auto mk = LoadMasks3(kAimK + 16);
    Stvx(ray, RowSelW(m + 48, m + 52, m + 56, mk));
    Stvx(ray + 16, RowSel(Fill4(static_cast<f32>(ex)), Fill4(static_cast<f32>(ey)), Fill4(static_cast<f32>(ez)), mk));
    store<u32>(sp + 148, 0);
    const u32 world = load<u32>(load<u32>(load<u32>(kGame) + 0x1281D4) + 4);
    store<u32>(coll + 4, 0x3F800000u);
    store<u32>(coll, kVtRayCol);
    store<u32>(coll + 16, hits);
    store<u32>(coll + 20, 0);
    store<u32>(coll + 24, 0x80000008u);
    for (u32 j = 0; j < 8; ++j) {
        const u32 h = hits + 80 * j;
        store<u32>(h + 16, 0x3F800000u);
        store<u32>(h + 20, 0xFFFFFFFFu);
        store<u32>(h + 64, 0);
        store<u32>(h + 32, 0xFFFFFFFFu);
        store<u32>(h + 68, 0);
    }
    store<u32>(coll + 20, 0);
    store<u32>(coll + 4, 0x3F800000u);
    call(ctx, HkWorld_CastRay92E510, {world, ray, coll});
    call(ctx, HkAllHits_Sort887E28, {coll});
    const s32 n = load<s32>(coll + 20);
    const u32 hp = load<u32>(coll + 16);
    const u32 self = load<u32>(c + 8);
    const u32 cw = load<u32>(coll + 24);
    f64 t = Lf(kAimK + 64);
    for (s32 i = 0; i < n; ++i) {
        const u32 hit = hp + 80 * static_cast<u32>(i);
        const u32 sh = load<u32>(hit + 68);
        const u32 ty = load<u32>(sh + 28) & 0x1F;
        if (ty >= 16 || ty == 6 || ty == 5) continue;
        const s8 kind = static_cast<s8>(load<u8>(sh + 24));
        const u32 rel = sh + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(sh + 16))));
        const u32 props = kind == 1 ? rel : 0;
        const u32 owned = kind == 2 ? rel : 0;
        u32 owner = 0;
        if (props == 0) {
            if (owned != 0) continue;
        } else if (PropLo(props, kPropKind) == kKindOwned) {
            owner = load<s32>(props + 128) > 0 ? PropLo(props, kPropOwner) : 0;
        }
        if (owner != 0 && owner != self) continue;
        t = Lf(hit + 16);
        break;
    }
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(out + 16 * r + k, load<u32>(kIdent + 16 * r + k));
    const bool got = t > thr;
    f64 f1 = t;  // left in f1 as the original does (no caller reads it)
    if (got) {
        store<u32>(out + 48, Fb(Fmadds(Subs(ex, sx), t, sx)));
        store<u32>(out + 52, Fb(Fmadds(Subs(ey, sy), t, sy)));
        f1 = Fmadds(Subs(ez, sz), t, sz);
        store<u32>(out + 56, Fb(f1));
    } else {
        store<u32>(out + 48, Fb(ex));
        store<u32>(out + 52, Fb(ey));
        store<u32>(out + 56, Fb(ez));
    }
    ctx.fpr[1] = f1;
    if ((cw & 0x80000000u) == 0) {
        const u32 opd = load<u32>(load<u32>(kHkHeap) + 20);
        const u64 size = static_cast<u64>(static_cast<s64>(static_cast<s32>(80u * (cw & 0x3FFFFFFFu))));
        store<u32>(coll + 20, 0);
        store<u32>(coll, kVtRayCol);
        if (got) call_ptr(ctx, opd, {kHkHeap, hp, size, opd, load<u32>(opd)});  // OPD in r6, entry in r7
        else call_ptr(ctx, opd, {kHkHeap, hp, size, load<u32>(opd), opd});      // entry in r6, OPD in r7
    }
    ret(ctx, got ? 1u : 0u);
}
