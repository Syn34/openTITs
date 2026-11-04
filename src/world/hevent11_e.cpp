// World_HandleEvent callees, part 11e: the POW meter object constructor.
#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent9;
using namespace sly4::hevent11;

namespace {
constexpr u32 kObjBaseConstruct = 0x0035695C;  // (Object*, 0, Def*) (name?)
constexpr u32 kPowMeterVtable = 0x00B1F790;
constexpr u32 kGeomFindByHash = 0x005801A0;     // GeomInstance_FindByHash (u32) -> instance
constexpr u32 kSetGeom = 0x0036468C;            // (Object*, u32* slot, instance) (name?)
constexpr u32 kStringFormat = 0x005534D0;       // String_Format (fmt, ...) -> const char*
constexpr u32 kFmtIntro = 0x00ADCAD0;           // "POWMeter_Intro"
constexpr u32 kFmtIdle = 0x00ADCAE0;            // "POWMeter_Idle"
constexpr u32 kFmtOutro = 0x00ADCAF0;           // "POWMeter_Outro"
constexpr u32 kOne = 0x3F800000;                // 1.0f
constexpr u32 kParts = 10;
}  // namespace

// void Obj_Construct364700(PowMeter* o, const PowMeterDef* d) (name?): base
// construction, the POW meter vtable, the +164 vector (flags 4), the def's
// two times (+292 / +296 at +304 / +312), user block d +148, the two main
// geometry instances (d +152 / +156 via 0x0036468C at +204 / +208), ten
// part instances (+212) with their values (+264), d +244 / +248, and the
// Intro / Idle / Outro animation hashes when the def has a name (d +36, or
// d +4 when empty). r3 is left as the original leaves it.
DECOMP(00364700, Obj_Construct364700) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    store<u32>(o, kPowMeterVtable);
    call(ctx, h9fn::PtrVector_Construct, {o + 164});
    store<u32>(o + 304, load<u32>(d + 292));
    store<u32>(o + 308, 0);
    store<u16>(o + 170, 4);
    store<u32>(o + 316, 0);
    store<u32>(o + 312, load<u32>(d + 296));
    ctx.fpr[1] = hevent7::lfs_f64(d + 296);  // left in f1 by the original's copy
    store<u32>(o + 320, 0);
    store<u32>(o + 324, 0);
    store<u32>(o + 328, kOne);
    store<u8>(o + 400, 0);
    store<u32>(o + 4, load<u32>(o + 4) | 0x10);
    store<u32>(o + 184, load<u32>(kCurrentWorld) + load<u32>(d + 148) * kUserBlockSize + kUserBlocks);
    for (u32 k = 0; k < 2; ++k) {
        const u32 h = load<u32>(d + 152 + k * 4);
        const u32 g = h != 0 ? static_cast<u32>(call(ctx, kGeomFindByHash, {h})) : 0u;
        call(ctx, kSetGeom, {o, o + 204 + k * 4, g});
    }
    for (u32 i = 0; i < kParts; ++i) {
        store<u32>(o + 212 + i * 4, static_cast<u32>(call(ctx, kGeomFindByHash, {load<u32>(d + 160 + i * 4)})));
        store<u32>(o + 264 + i * 4, load<u32>(d + 252 + i * 4));
    }
    store<u32>(o + 256, load<u32>(d + 244));
    store<u32>(o + 260, load<u32>(d + 248));
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 188 + k * 4, 0);
    u32 name = d + 36;
    if (load<u8>(name) == 0) name = d + 4;
    if (name == 0 || load<u8>(name) == 0) return ret(ctx, 0u);
    const u32 fmts[3] = {kFmtIntro, kFmtIdle, kFmtOutro};
    for (u32 k = 0; k < 3; ++k) {
        const u32 s = static_cast<u32>(call(ctx, kStringFormat, {fmts[k], name}));
        store<u32>(o + 192 + k * 4, static_cast<u32>(call(ctx, hfn::String_HashNoCase, {s})));
    }
}
