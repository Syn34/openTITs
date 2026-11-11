// Game code, large-function sweep part 18: callees of part 17 (the def
// base constructor, the Havok body-record vector push, the physics object
// mode change).
#include "big18.h"

using namespace sly4;
using namespace sly4::big18;

namespace {
namespace hd {
constexpr u32 PtrVector_Reserve = 0x00551718;       // (vec, u32 n)
constexpr u32 DefBase_Init_000C26D8 = 0x000C26D8;   // (p, rec): the inner base constructor (name?)
constexpr u32 EffectDef_Find = 0x004BDEDC;          // (u32 hash, 0, 1) -> def or 0
constexpr u32 EffectDef_HasCommands = 0x004B95F0;   // (def) -> bool
constexpr u32 Hash_ToString = 0x0056A6E0;           // (u32 hash) -> const char*
constexpr u32 String_Format = 0x005534D0;           // (fmt, ...)
constexpr u32 Hk_RemoveEntity = 0x0092FFC8;         // (u8* out, world, body)
constexpr u32 Hk_AddEntity = 0x0092F2A8;            // (world, body, 1)
constexpr u32 Volume_Notify112D8 = 0x000112D8;      // (volumes, Volume*)
constexpr u32 Shape_InitBody = 0x00011410;          // (Shape* out, volumes, body, const T* xf, shape, u32 mask)
constexpr u32 Volume_SetShape637D14 = 0x00637D14;   // (Volume*, const Shape*)
constexpr u32 Shape_Release638498 = 0x00638498;     // (Shape*)

constexpr u32 kDefVt = 0x00B11D98;                  // the def base vtable
constexpr u32 kMissingScript = 0x00AC6714;          // "missing lifetime script '%s' for def '%s'"
constexpr u32 kNoCommands = 0x00AC6744;             // "entity def '%s' lifetime script '%s' has no commands"
constexpr u32 kNoCommandsArg = 0x00B0F050;          // the string passed as its second '%s'
constexpr u32 kVolumes = 0x00F557EC;                // the volume manager
}  // namespace hd
}  // namespace

// u32 f(BodyVector* v, const BodyRec* src): v {+0 data, u16 +4 capacity,
// u16 +8 count, u16 +10 high-water mark}. Grows to twice the capacity when
// full (PtrVector_Reserve), copies the 160-byte record's used words (+0..8,
// +16..24, +32, +36, +48..56, +64..88, +96..120, the top four bits of +124
// one at a time over the old ones, +128..136, +144) to data + count * 160,
// bumps the count and the mark; returns the new count - 1 (u16 arithmetic,
// so a wrapped count gives -1). f1 is left as the lfs of src +136.
DECOMP(00745720, BodyVector_Add_00745720) {
    using namespace hd;
    const u32 v = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, PtrVector_Reserve, {v, (cap << 1) & 0x0001FFFEu});
        n = load<u16>(v + 8);
    }
    const u32 d = load<u32>(v) + n * 160u;
    static constexpr u32 kW[] = {0, 4, 8, 16, 20, 24, 32, 36, 48, 52, 56, 64, 68, 72, 76, 80, 84, 88,
                                 96, 100, 104, 108, 112};
    for (const u32 o : kW) store<u32>(d + o, load<u32>(s + o));
    const u32 w116 = load<u32>(s + 116);
    u32 x = load<u32>(d + 124);
    store<u32>(d + 116, w116);
    store<u32>(d + 120, load<u32>(s + 120));
    x = (x & 0x7FFFFFFFu) | (load<u32>(s + 124) & 0x80000000u);
    store<u32>(d + 124, x);
    for (u32 m = 0x40000000u; m >= 0x10000000u; m >>= 1) {
        x = (x & ~m) | (load<u32>(s + 124) & m);
        store<u32>(d + 124, x);
    }
    store<u32>(d + 128, load<u32>(s + 128));
    store<u32>(d + 132, load<u32>(s + 132));
    store<u32>(d + 136, load<u32>(s + 136));
    store<u32>(d + 144, load<u32>(s + 144));
    SetF1(ctx, s + 136);
    const u32 n1 = load<u16>(v + 8) + 1;
    const u32 c = n1 & 0xFFFFu;
    const u32 hi = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c) ? hi : n1));
    ret(ctx, static_cast<u64>(static_cast<s64>(c) - 1));
}

// u32 f(Def* p, const Record* rec): DefBase_Init_000C26D8(p, rec), the
// vtable, +140 / +148 / +152 = rec +76 / +80 / +84, +144 = 0, +176..+184 =
// rec +108..+116, bits 0xFFF0 of u16 +188 from rec +120 (one bit at a time,
// then the nibble 0x00F0), +156..+172 = rec +88..+104, u16 +190 = rec +122;
// +144 = EffectDef_Find(rec +80, 0, 1) (the lifetime script). A missing
// script (hash set) is reported, and one without commands too. Returns the
// last call's r3 (0 without a script). f1 is left as the lfs of rec +88.
DECOMP(000C2E8C, DefBase_Ctor_000C2E8C) {
    using namespace hd;
    const u32 p = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    call(ctx, DefBase_Init_000C26D8, {p, rec});
    store<u32>(p + 148, load<u32>(rec + 80));
    const u32 f76 = load<u32>(rec + 76);
    store<u32>(p + 140, f76);
    store<u32>(p + 152, load<u32>(rec + 84));
    store<u32>(p + 0, kDefVt);
    store<u32>(p + 144, 0);
    store<u32>(p + 180, load<u32>(rec + 112));
    store<u32>(p + 176, load<u32>(rec + 108));
    u32 h = load<u16>(p + 188);
    store<u32>(p + 184, load<u32>(rec + 116));
    for (u32 m = 0x8000u; m >= 0x0100u; m >>= 1) {
        h = (h & ~m) | (load<u16>(rec + 120) & m);
        store<u16>(p + 188, static_cast<u16>(h));
    }
    h = (h & ~0x00F0u) | (load<u16>(rec + 120) & 0x00F0u);
    store<u16>(p + 188, static_cast<u16>(h));
    u32 f[5];
    for (u32 k = 0; k < 5; ++k) f[k] = load<u32>(rec + 88 + 4 * k);
    const u16 h122 = load<u16>(rec + 122);
    const u32 hash = load<u32>(rec + 80);
    store<u32>(p + 156, f[0]);
    store<u32>(p + 160, f[1]);
    store<u16>(p + 190, h122);
    store<u32>(p + 164, f[2]);
    store<u32>(p + 168, f[3]);
    store<u32>(p + 172, f[4]);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(f[0]));
    u32 r = static_cast<u32>(call(ctx, EffectDef_Find, {hash, 0, 1}));
    const u32 h80 = load<u32>(rec + 80);
    store<u32>(p + 144, r);
    if (h80 != 0 && r == 0) {
        const u32 s = static_cast<u32>(call(ctx, Hash_ToString, {h80, h80}));
        call(ctx, String_Format, {kMissingScript, s, load<u32>(rec + 12)});
        r = load<u32>(p + 144);
    }
    if (r == 0) { ret(ctx, 0); return; }
    u64 res = call(ctx, EffectDef_HasCommands, {r});
    if (static_cast<u32>(res) == 0) res = call(ctx, String_Format, {kNoCommands, p + 56, kNoCommandsArg});
    ret(ctx, res);
}

// u32 f(PhysObj* o, s32 mode): nothing when the mode (bits 0xF000 of u16
// +432) is already `mode` (returns +416, the body). Else, by the new mode:
// 0 / 4: a body in a world (+8) is removed from it and the volume +436
// notified; 3: +420 = 0; 1: vfunc 88(o, 1, 1, 1, 1), then a body gets its
// collision filter (+44: low 5 bits kept, high half = o +204) and, when
// not in a world, is added to the physics world; if vfunc 168(o) then the
// template's vfunc 12 transform (to the frame) shapes the volume +436
// (Shape_InitBody with mask 0xFFFF, or 0xFFFE without bit 0x01000000 of
// the template +300, and vfunc 76(o) as the shape); +420 = 0. The mode is
// stored; returns the new u16 +432.
DECOMP(001F9F70, PhysObj_Finish_001F9F70) {
    using namespace hd;
    using big17::kWorldPtr;
    using big17::kWorldPhys;
    const u32 o = arg<u32>(ctx, 0);
    const u32 mode = arg<u32>(ctx, 1);
    Stack frame(ctx, 208);
    const u32 sp = frame.addr();
    const u32 h = load<u16>(o + 432);
    const u32 tmpl = load<u32>(o + 216);
    const u32 body = load<u32>(o + 416);
    if (static_cast<s32>((h >> 12) & 0xFu) == static_cast<s32>(mode)) { ret(ctx, body); return; }
    const s32 m = static_cast<s32>(mode);
    if (m == 4 || m == 0) {
        if (body != 0) {
            const u32 world = load<u32>(body + 8);
            if (world != 0) {
                call(ctx, Hk_RemoveEntity, {sp + 148, world, body});
                call(ctx, Volume_Notify112D8, {load<u32>(kVolumes), o + 436});
            }
        }
    } else if (m == 3) {
        store<u32>(o + 420, 0);
    } else if (m == 1) {
        VCall(ctx, o, 88, {o, 1, 1, 1, 1});
        const u32 b = load<u32>(o + 416);
        if (b != 0) {
            const u32 x = (load<u32>(b + 44) & 0x1Fu) | (load<u32>(o + 204) << 16);
            const u32 world = load<u32>(b + 8);
            store<u32>(b + 44, x);
            if (world == 0) {
                call(ctx, Hk_AddEntity, {load<u32>(load<u32>(load<u32>(kWorldPtr) + kWorldPhys) + 4), b, 1});
                if (static_cast<u32>(VCall(ctx, o, 168, {o})) != 0) {
                    const u32 t = load<u32>(o + 216);
                    VCall(ctx, t, 12, {sp + 112, t});
                    const u32 flag = (load<u32>(tmpl + 300) >> 24) & 1u;
                    const u32 vt = load<u32>(o);
                    const u32 b2 = load<u32>(o + 416);
                    const u32 mask = flag != 0 ? 0xFFFFu : 0xFFFEu;
                    const u32 opd = load<u32>(vt + 76);
                    const u32 vols = load<u32>(kVolumes);
                    // vfunc 76(o), the OPD in r4 and its entry in r6 (r5 the
                    // TOC page 0x00F50000), as the original call sequence.
                    const u32 shape = static_cast<u32>(call_ptr(ctx, opd, {o, opd, 0x00F50000u, load<u32>(opd)}));
                    call(ctx, Shape_InitBody, {sp + 144, vols, b2, sp + 112, shape, mask});
                    call(ctx, Volume_SetShape637D14, {o + 436, sp + 144});
                    call(ctx, Shape_Release638498, {sp + 144});
                }
            }
        }
        store<u32>(o + 420, 0);
    }
    const u32 h2 = (load<u16>(o + 432) & ~0xF000u) | ((mode << 12) & 0xF000u);
    store<u16>(o + 432, static_cast<u16>(h2));
    ret(ctx, h2);
}
