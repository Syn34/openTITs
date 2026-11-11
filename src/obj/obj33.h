// Game code, part 33: helpers shared by obj33*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 61
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj32.h"

namespace sly4 {
namespace obj33 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj32;
using namespace sly4::obj31;
using namespace sly4::obj30;
using namespace sly4::obj29;
using namespace sly4::obj28;
using namespace sly4::obj27;
using namespace sly4::obj26;
using namespace sly4::obj25;
using namespace sly4::obj24;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o33fn {
constexpr u32 Mem_GetDefaultStack = 0x005525FC;  // () -> the default memory stack
constexpr u32 XmlReadF32ByName = 0x004FA5C4;     // (x, name, f32* out; f1 default)
constexpr u32 XmlReadBoolByName = 0x004FA504;    // (x, name, u8* out, default)
}  // namespace o33fn

constexpr u32 kGlobals10C2C28 = 0x010C2C28;  // the settings object (0 before it exists)
constexpr u32 kTlsMemRouter = 28332;          // r13 - 28332: the thread's Havok memory router

// fsel: a >= 0 ? b : c (a NaN selects c).
inline f64 Fsel33(f64 a, f64 b, f64 c) { return a >= 0.0 ? b : c; }

// The sign-extended s32 as a 64-bit register value (extsw).
inline u64 Sx33(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }

// The thread's Havok memory router (*(r13 - 28332)).
inline u32 HkRouter33(PPUContext& ctx) { return load<u32>(static_cast<u32>(ctx.gpr[13]) - kTlsMemRouter); }

// The u16-counted pointer vector push (data +0, capacity +4 doubled by
// PtrVector_Reserve when full, count +8, high-water +10) as the game
// inlines it.
inline void PushU16Vec33(PPUContext& ctx, u32 v, u32 val) {
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, 0x00551718, {v, (cap << 1) & 0x1FFFEu});
    const u32 c = load<u16>(v + 8);
    store<u32>(load<u32>(v) + ((c << 2) & 0x3FFFCu), val);
    const u32 n = c + 1u;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n & 0xFFFFu) ? hw : n));
}

// The ragdoll release of 0x00184830 / 0x0019211C / 0x001683F0 /
// 0x0016EF50 (void f(T* o, u8 on)): with on, o->vt[376](o, mode) (or
// o->vt[396](o, mode, 0) for slot 396), the skeleton o->+392 released,
// and the physics body b = o->+388 taken out of the world
// (Hk_RemoveEntity92FFC8 when b->+8 is set), its properties 0xA734004D /
// 0x91C7CF5B removed and it released; then 0x000E02F8(o, on). r3 as that
// left it.
inline void RagdollRelease33(PPUContext& ctx, u32 mode, u32 slot = 376) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        if (slot == 376) VCall(ctx, o, 376, {o, mode});
        else VCall(ctx, o, slot, {o, mode, 0});
        const u32 a = load<u32>(o + 392);
        if (a != 0) call(ctx, 0x007D4EA8, {a});
        const u32 b = load<u32>(o + 388);
        if (b != 0) {
            Stack frame(ctx, 32);
            const u32 s = frame.addr();  // sp+112 / +120 / +128
            const u32 w = load<u32>(b + 8);
            if (w != 0) call(ctx, 0x0092FFC8, {s + 16, w, b});
            call(ctx, 0x0093FED8, {s, load<u32>(o + 388), 0xA734004Du});
            call(ctx, 0x0093FED8, {s + 8, load<u32>(o + 388), 0x91C7CF5Bu});
            call(ctx, 0x007D4EA8, {load<u32>(o + 388)});
        }
    }
    ctx.gpr[3] = call(ctx, 0x000E02F8, {o, on});
}

// Deleting ~T() of 0x0007DD24 / 0x0007E468 / 0x0007F2F8 (o may be 0; vt
// vt): the owned objects of the u16-counted vector o+off (data +0, count
// +8) destroyed from the back (vt[8](e)), the count re-read and
// decremented after each (quirk kept: a count found 0 there wraps to
// 0xFFFF and data[-1] = data[0xFFFF]); then the vector cleared,
// VtB11B20_DtorTrivial_004BB41C(o) and Mem_FreeFrom(o, 0). r3 as that
// left it.
inline void DeleteOwnedVec33(PPUContext& ctx, u32 vt, u32 off) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (o == 0) return;
    store<u32>(o, vt);
    for (;;) {
        const u32 c0 = load<u16>(o + off + 8);
        if (c0 == 0) break;
        const u32 e = load<u32>(load<u32>(o + off) + 4 * (c0 - 1));
        VCall(ctx, e, 8, {e});
        const u32 n = load<u16>(o + off + 8) - 1u;
        store<u16>(o + off + 8, static_cast<u16>(n));
        if (n != (n & 0xFFFFu)) {
            const u32 d = load<u32>(o + off);
            store<u32>(d + 4 * n, load<u32>(d + ((n << 2) & 0x3FFFCu)));
        }
    }
    PvClear32(ctx, o + off);
    call(ctx, 0x004BB41C, {o});
    ctx.gpr[3] = call(ctx, o32fn::Mem_FreeFrom, {o, 0});
}

// s16 f(u16 a, u8** pp, u32* link) of 0x007737A4 / 0x00773998: pushes a
// record in the block before *pp (header at *pp - 152: u16 count +144;
// 132-byte records from +152): record +0 = 6, its u16s +4 = a and +6 &=
// 0x3F (when the record's +4 is not address 0); with link, *link =
// &rec+4 + u16 *0x0106CF7C, and at that address (when not 0) the u32 0
// (wordZero) or the u16 0xFFFF. r3 = the record's index (the old count,
// sign-extended). The original's re-check of the type just written (6 <
// 10) always passes; its other branch is dead.
inline void PushRecord33(PPUContext& ctx, bool wordZero) {
    const u32 a = arg<u32>(ctx, 0), pp = arg<u32>(ctx, 1), link = arg<u32>(ctx, 2);
    const u32 base = load<u32>(pp) - 152;
    const u32 c = load<u16>(base + 144);
    const s32 idx = static_cast<s16>(c);
    const u32 rec = base + 152 + static_cast<u32>(idx) * 132;
    store<u16>(base + 144, static_cast<u16>(c + 1));
    store<u32>(rec, 6);
    const u32 r8 = rec + 4;
    if (r8 != 0) {
        store<u16>(r8 + 2, static_cast<u16>(load<u16>(r8 + 2) & 0x3Fu));
        store<u16>(r8, static_cast<u16>(a));
    }
    if (link != 0) {
        const u32 p = r8 + load<u16>(0x0106CF7C);
        store<u32>(link, p);
        if (p != 0) {
            if (wordZero) store<u32>(p, 0);
            else store<u16>(p, 0xFFFF);
        }
    }
    ret(ctx, idx);
}

// ~T() of 0x00751314 / 0x00756CD4 (vts v+8 / +380 v+436 / +384 v+476):
// the pointer vector +424 cleared; the 20-byte vector object +388 (when
// set, the vts first becoming those of 0x00B12890) cleared and
// SmallFree'd; then vts 0x00B126B8 / 0x00B1200C / 0x00B12528 and
// VtB12550_Dtor_000DF934(o). r3 as that left it.
inline void Dtor3Vt33(PPUContext& ctx, u32 v) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, v + 8);
    store<u32>(o + 380, v + 436);
    store<u32>(o + 384, v + 476);
    PvClear32(ctx, o + 424);
    const u32 q = load<u32>(o + 388);
    if (q != 0) {
        constexpr u32 w = 0x00B12890;
        store<u32>(o + 384, w + 476);
        store<u32>(o + 380, w + 436);
        store<u32>(o, w + 8);
        PvClear32(ctx, q);
        call(ctx, o32fn::SmallFree, {load<u32>(o + 388), 20});
    }
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    ctx.gpr[3] = call(ctx, 0x000DF934, {o});
}

// Node* New(s16 id) of 0x00777FA8 / 0x0077A858: as obj19's TNewNamed
// with one binding Node_Bind(p->+12, name2, 4, 1, -1, 0), except that the
// binding's +76 is set to 1. r3 = p (0 for a failed allocation); r4 = 1,
// r5 = the binding's +64 as the original leaves them.
inline void NodeNewOn33(PPUContext& ctx, u32 name1, u32 name2) {
    const u64 a0 = ctx.gpr[3];
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {24, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x00391694, {p, static_cast<u64>(static_cast<s64>(static_cast<s16>(a0))), name1});
    const s32 idx = static_cast<s16>(load<u16>(p + 16));
    const u32 tbl = load<u32>(0x00F65038);
    const u32 rec = idx == -1 ? 0 : load<u32>(tbl + 24) + (static_cast<u32>(idx) << 7);
    store<u16>(rec, 16);  // at address 0 for index -1 (quirk kept)
    const u32 flags = load<u32>(p + 20) + load<u16>(0x0106CF82);
    const u32 r = static_cast<u32>(call(ctx, 0x004D3088, {load<u32>(p + 12), name2, 4, 1, ~0ull, 0}));
    const u32 v = load<u32>(r + 64);
    store<u32>(r + 76, 1);
    store<u8>(flags, static_cast<u8>(v));
    ctx.gpr[3] = p;
    ctx.gpr[4] = 1;
    ctx.gpr[5] = v;
}

// The sound shutdown of 0x001CC338 / 0x001D46E8 (void f(T* o, u8 on)):
// with on, the effect o->+off (when set) gets vt[40](it, 0) and is
// cleared, +536 = 0 and the seven voices +428 + 16 * i stopped with a
// fade (Emitter_Stop(h, 1)) and their +428 / +432 cleared; then
// 0x000E4544(o, on). r3 as that left it.
inline void SoundOff33(PPUContext& ctx, u32 off) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        const u32 a = load<u32>(o + off);
        if (a != 0) {
            VCall(ctx, a, 40, {a, 0});
            store<u32>(o + off, 0);
        }
        store<u32>(o + 536, 0);
        for (u32 i = 0; i < 7; ++i) {
            const u32 v = o + 428 + 16 * i;
            call(ctx, 0x00443424, {load<u32>(v), 1});
            store<u32>(v, 0);
            store<u32>(v + 4, 0);
        }
    }
    ctx.gpr[3] = call(ctx, 0x000E4544, {o, on});
}

// T f(T* o; f1 dt) of 0x0021C774 / 0x0021DD08: unless o->+off is -1,
// each entry e of o's u16-counted list (+344 / +352) whose vt[64](e)
// object r has a team ((r->+416 >> 28) & 7) other than o->+off is handed
// to o->vt[288](o, e), the index staying put; then 0x0020DFD8(o; dt). r3 as that left it.
inline void TeamScan33(PPUContext& ctx, u32 off) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    if (load<u32>(o + off) != 0xFFFFFFFFu) {
        for (u32 i = 0; i < load<u16>(o + 352);) {
            const u32 e = load<u32>(load<u32>(o + 344) + 4 * i);
            const u32 r = static_cast<u32>(VCall(ctx, e, 64, {e}));
            if (r == 0 || static_cast<s32>((load<u32>(r + 416) >> 28) & 7u) == static_cast<s32>(load<u32>(o + off))) {
                ++i;
                continue;
            }
            VCall(ctx, o, 288, {o, e});  // (removes e: the same index is looked at again)
        }
    }
    callf(ctx, 0x0020DFD8, {o}, {dt});
}

// bool f(Reader* r) of 0x0048C194 / 0x0048C3C0: the 64-byte name table
// base+1 (zeroed with its byte base once, flag u8 flag): a byte
// (Iff_ReadU8, ignored) then 64 bytes read in; *dst = the table; with the
// world's *0x00F65CA0->+0x1281C0 = x, 0x004ECF08(x, key, table). r3 = 1.
inline void NameTable33(PPUContext& ctx, u32 flag, u32 base, u32 dst, u32 key) {
    const u32 r = arg<u32>(ctx, 0);
    if (load<u8>(flag) == 0) {
        call(ctx, 0x00726E54, {base, 0, 65});
        store<u8>(flag, 1);
    }
    Stack frame(ctx, 16);
    call(ctx, 0x005643C4, {r, frame.addr(), 0});  // sp+112
    const u32 table = base + 1;
    for (u32 i = 0; i < 64; ++i) call(ctx, 0x0056998C, {r, table + i, 0});
    const u32 g = load<u32>(0x00F65CA0);
    store<u32>(dst, table);
    const u32 x = load<u32>(g + 0x1281C0);
    if (x != 0) call(ctx, 0x004ECF08, {x, key, table});
    ctx.gpr[3] = 1;
}

}  // namespace obj33
}  // namespace sly4
