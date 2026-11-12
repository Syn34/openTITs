// Game code, large-function sweep part 31: the first object a shape's
// linear cast reaches that is a live, pickable (vt +320 / +328) object.
#include "../sly4.h"
#include "big31.h"

using namespace sly4;
using namespace sly4::big31;

namespace {
constexpr u32 kCastMasks = 0x001E1060;      // .text: lane masks y / z / w
constexpr u32 kVtCastCol = 0x00F1DE58;  // the all-hits cast collector vtable (48-byte hits, 8 inline)
constexpr u32 kHkHeap = 0x011DF31C;     // Havok heap allocator (vfn[20] (p, bytes) frees)
constexpr u32 kPropOwner = 0x91C7CF5B;  // shape property: the owning object
constexpr u32 HkHits_Sort8871E8 = 0x008871E8;  // (Collector*) sort the 48-byte hits

// The low word of property `key` in the shape's list (+124 the 16-byte
// entries {key, -, u64 value}, +128 the s32 count), 0 when absent.
u32 PropLo(u32 props, u32 key) {
    for (s32 j = 0; j < load<s32>(props + 128); ++j) {
        const u32 e = load<u32>(props + 124) + 16 * static_cast<u32>(j);
        if (load<u32>(e) == key) return load<u32>(e + 12);
    }
    return 0;
}

// Frees a heap hit array (the collector's count / vtable reset first;
// OPD in r6, entry in r7).
void Release(PPUContext& ctx, u32 coll) {
    const u32 cw = load<u32>(coll + 24);
    if ((cw & 0x80000000u) != 0) return;
    const u64 size = static_cast<u64>(static_cast<s64>(static_cast<s32>(48u * (cw & 0x3FFFFFFFu))));
    store<u32>(coll + 20, 0);
    const u32 p = load<u32>(coll + 16);
    store<u32>(coll, kVtCastCol);
    const u32 opd = load<u32>(load<u32>(kHkHeap) + 20);
    call_ptr(ctx, opd, {kHkHeap, p, size, opd, load<u32>(opd)});
}
}  // namespace

// Obj* f(Obj* o, Caster* c, const Mtx* m, -, -, u8 strict; f32 t0, f32 t1)
// (called by f_001E1628 and Target_Release_001E1F20's f_001E2088). 0 for
// a null c or c->+8. Otherwise c->vt[64](c, &from, &to, &coll, 0) (r8 / r9
// the OPD / entry) casts from m row 3 + t0 * row 2 to row 3 + t1 * row 2
// (from / to at sp + 112 / 128 through the masks; sp + 144 zeroed; the
// collector at sp + 160: early-out 0x7F7FFFEE, eight inline 48-byte hits
// at sp + 192). With hits (sorted) the first whose shape (+40) with byte
// +24 == 1 or 2 has a kPropOwner object x != o such that x +192 has bit 2
// (and, with strict, not bit 0x2000), the two objects' vt +100 results are
// not the same non-zero value, x->vt[320](x) is 0 and x->vt[328](x) is
// not, is returned (x re-checked for bit 2 of +192 after the vt +100 calls:
// the vt +320 call goes through 0 without it, as in the original). A heap
// hit array is freed first through kHkHeap->vfn[20].
DECOMP(001E1090, Obj_CastFindTarget_001E1090) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const u32 strict = arg<u32>(ctx, 5) & 0xFF;
    const f64 t0 = farg(ctx, 0), t1 = farg(ctx, 1);
    if (c == 0 || load<u32>(c + 8) == 0) return ret(ctx, 0u);
    Stack frame(ctx, 480);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    const u32 from = sp + 112, to = sp + 128, coll = sp + 160;
    store<u32>(sp + 144, 0x34000000u);  // overwritten by the zero quadword below, as in the original
    store<u32>(sp + 148, 0x34000000u);
    store<u32>(coll + 16, sp + 192);
    store<u32>(coll, kVtCastCol);
    store<u32>(coll + 24, 0x80000008u);
    store<u32>(coll + 20, 0);
    store<u32>(coll + 4, 0x7F7FFFEEu);
    for (u32 k = 0; k < 16; k += 4) store<u32>(sp + 144 + k, 0);
    const u32 p = m + 48, d = m + 32;
    const f64 fx = Fmadds(Lf(d), t0, Lf(p)), tx = Fmadds(Lf(d), t1, Lf(p));
    const f64 fy = Fmadds(Lf(d + 4), t0, Lf(p + 4)), ty = Fmadds(Lf(d + 4), t1, Lf(p + 4));
    const f64 fz = Fmadds(Lf(d + 8), t0, Lf(p + 8)), tz = Fmadds(Lf(d + 8), t1, Lf(p + 8));
    const auto mk = LoadMasks3(kCastMasks);
    Stvx(from, RowSel(Fill4(static_cast<f32>(fx)), Fill4(static_cast<f32>(fy)), Fill4(static_cast<f32>(fz)), mk));
    Stvx(to, RowSel(Fill4(static_cast<f32>(tx)), Fill4(static_cast<f32>(ty)), Fill4(static_cast<f32>(tz)), mk));
    VCall(ctx, c, 64, {c, from, to, coll, 0});
    if (load<s32>(coll + 20) > 0) {
        call(ctx, HkHits_Sort8871E8, {coll});
        for (s32 i = 0; i < load<s32>(coll + 20); ++i) {
            const u32 sh = load<u32>(load<u32>(coll + 16) + 48 * static_cast<u32>(i) + 40);
            const s8 kind = static_cast<s8>(load<u8>(sh + 24));
            if (kind != 1 && kind != 2) continue;
            const u32 props = sh + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(sh + 16))));
            if (props == 0) continue;
            const u32 x = PropLo(props, kPropOwner);
            if (x == 0 || x == o) continue;
            const u32 f = load<u32>(x + 192);
            if ((f & 2) == 0) continue;
            if (strict != 0 && (f & 0x2000) != 0) continue;
            const u32 a = static_cast<u32>(VCall(ctx, o, 100, {o}));
            const u32 b = static_cast<u32>(VCall(ctx, x, 100, {x}));
            if (a != 0 && a == b) continue;
            const u32 t = (load<u32>(x + 192) & 2) != 0 ? x : 0;
            if (static_cast<u32>(VCall(ctx, t, 320, {t})) != 0) continue;
            if (static_cast<u32>(VCall(ctx, t, 328, {t})) == 0) continue;
            Release(ctx, coll);
            return ret(ctx, t);
        }
    }
    Release(ctx, coll);
    ret(ctx, 0u);
}
