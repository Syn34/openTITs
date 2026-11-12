// Game code, large-function sweep part 31: the first object a shape's
// linear cast reaches that is a live, pickable (vt +320 / +328) object.
#include "big31.h"

namespace sly4::obj {

// One 48-byte cast hit: +40 the collidable.
struct CastHit {
    pad<40> _0;
    be_u32 shape;  // +40: an HkCollidable
    pad<4> _44;
};
static_assert(sizeof(CastHit) == 48);
// The all-hits cast collector (hkArray of CastHit at +16).
struct CastCollector {
    be_u32 vtable;     // +0
    be_u32 early_out;  // +4 (raw word)
    pad<8> _8;
    be_u32 hits;       // +16: CastHit*
    be_s32 count;      // +20
    be_u32 capflags;   // +24
    pad<4> _28;
    CastHit inline_hits[8];  // +32
};
struct CastObj {
    pad<192> _0;
    be_u32 flags;  // +192: 2 live, 0x2000 excluded when strict
};
struct Caster {
    pad<8> _0;
    be_u32 w8;  // +8
};
struct Mat44c {
    Vec4 r[4];
};

namespace {
constexpr u32 kCastMasks = 0x001E1060;  // .text: lane masks y / z / w
constexpr u32 kVtCastCol = 0x00F1DE58;  // the all-hits cast collector vtable (48-byte hits, 8 inline)
constexpr u32 HkHits_Sort8871E8 = 0x008871E8;  // (Collector*) sort the 48-byte hits

// Frees a heap hit array (the collector's count / vtable reset first).
void Release(CastCollector* c) {
    const u32 cw = c->capflags;
    if ((cw & 0x80000000u) != 0) return;
    const s32 size = static_cast<s32>(48u * (cw & 0x3FFFFFFFu));
    c->count = 0;
    const u32 p = c->hits;
    c->vtable = kVtCastCol;
    vcall(host<u8>(kHkHeap), 20, p, size);
}
}  // namespace

// Obj* f(Obj* o, Caster* c, const Mtx* m, -, -, u8 strict; f32 t0, f32 t1)
// (called by f_001E1628 and Target_Release_001E1F20's f_001E2088). 0 for
// a null c or c->w8. Otherwise c->vt[64](c, &from, &to, &coll, 0) casts
// from m row 3 + t0 * row 2 to row 3 + t1 * row 2 (the collector:
// early-out 0x7F7FFFEE, eight inline 48-byte hits). With hits (sorted) the
// first whose collidable of kind 1 or 2 has a kPropOwner object x != o such
// that x->flags has bit 2 (and, with strict, not bit 0x2000), the two
// objects' vt +100 results are not the same non-zero value, x->vt[320](x)
// is 0 and x->vt[328](x) is not, is returned (x re-checked for bit 2 after
// the vt +100 calls: the vt +320 call goes through 0 without it, as in the
// original). A heap hit array is freed first through kHkHeap.
u32 Obj_CastFindTarget_001E1090(u32 o, Caster* c, const Mat44c* m, u32, u32, u8 strict, f32 t0_, f32 t1_) {
    const f64 t0 = t0_, t1 = t1_;
    if (c == nullptr || c->w8 == 0) return 0;
    GuestFrame frame(480);  // the original's sp+112..
    Vec4* from = frame.as<Vec4>(0);
    Vec4* to = frame.as<Vec4>(16);
    Quad* zero = frame.as<Quad>(32);
    CastCollector* coll = frame.as<CastCollector>(48);
    zero->w[0] = 0x34000000u;  // overwritten by the zero quadword below, as in the original
    zero->w[1] = 0x34000000u;
    coll->hits = guest(&coll->inline_hits[0]);
    coll->vtable = kVtCastCol;
    coll->capflags = 0x80000008u;
    coll->count = 0;
    coll->early_out = 0x7F7FFFEEu;
    for (u32 k = 0; k < 4; ++k) zero->w[k] = 0u;
    const Vec4& p = m->r[3];
    const Vec4& d = m->r[2];
    const f64 fx = Fmadds(lf(d.x), t0, lf(p.x)), tx = Fmadds(lf(d.x), t1, lf(p.x));
    const f64 fy = Fmadds(lf(d.y), t0, lf(p.y)), ty = Fmadds(lf(d.y), t1, lf(p.y));
    const f64 fz = Fmadds(lf(d.z), t0, lf(p.z)), tz = Fmadds(lf(d.z), t1, lf(p.z));
    const Masks3 mk = LoadMasks3(kCastMasks);
    Stvx(from, RowSel(Fill4(static_cast<f32>(fx)), Fill4(static_cast<f32>(fy)), Fill4(static_cast<f32>(fz)), mk));
    Stvx(to, RowSel(Fill4(static_cast<f32>(tx)), Fill4(static_cast<f32>(ty)), Fill4(static_cast<f32>(tz)), mk));
    vcall(c, 64, from, to, coll, 0u);
    if (coll->count > 0) {
        call(HkHits_Sort8871E8, coll);
        for (s32 i = 0; i < coll->count; ++i) {
            const u32 sh = gm<CastHit>(coll->hits + 48 * static_cast<u32>(i)).shape;
            const HkCollidable& col = gm<HkCollidable>(sh);
            const s8 kind = col.kind;
            if (kind != 1 && kind != 2) continue;
            const u32 props = sh + static_cast<u32>(static_cast<s32>(col.owner_off));
            if (props == 0) continue;
            const u32 x = HkPropLo(props, kPropOwner);
            if (x == 0 || x == o) continue;
            const u32 f = gm<CastObj>(x).flags;
            if ((f & 2) == 0) continue;
            if (strict != 0 && (f & 0x2000) != 0) continue;
            const u32 a = vcall<u32>(host<u8>(o), 100);
            const u32 b = vcall<u32>(host<u8>(x), 100);
            if (a != 0 && a == b) continue;
            const u32 t = (gm<CastObj>(x).flags & 2) != 0 ? x : 0u;
            if (vcall<u32>(host<u8>(t), 320) != 0) continue;
            if (vcall<u32>(host<u8>(t), 328) == 0) continue;
            Release(coll);
            return t;
        }
    }
    Release(coll);
    return 0;
}

}  // namespace sly4::obj

NATIVE(001E1090, sly4::obj::Obj_CastFindTarget_001E1090)
