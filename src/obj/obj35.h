// Game code, part 35: helpers shared by obj35*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the last 69
// instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj34.h"

namespace sly4 {
namespace obj35 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj34;
using namespace sly4::obj33;
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

// The XmlCursor readers the record factories here use.
constexpr u32 kXmlReadBool35 = 0x004F986Cu;  // (x, key, u8* dst, u8 dflt)
constexpr u32 kXmlReadS32_35 = 0x004F99BCu;  // (x, key, s32* dst, s32 dflt)
constexpr u32 kXmlReadF32_35 = 0x004F9B20u;  // (x, key, f32* dst; f1 = dflt)
constexpr u32 kXmlReadHash35 = 0x004FA14Cu;  // (x, key, u32* dst, u32 dflt)

// The 24-byte XML record with two hashes and two bools (NewXml34 body):
// +12 / +16 / u8 +20 cleared, vtable vt, u8 +21 cleared; the hash h1 to
// +12 (default 0), the hash h2 to +16 and the bools b1 / b2 to +20 / +21
// (current defaults).
inline void XmlHHBB35(PPUContext& ctx, u32 vt, u32 h1, u32 h2, u32 b1, u32 b2) {
    NewXml34(ctx, 24, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u8>(m + 20, 0);
        store<u32>(m, vt);
        store<u8>(m + 21, 0);
        RdHash34(ctx, x, h1, m + 12, 0);
        RdHashCur34(ctx, x, h2, m + 16);
        RdBoolCur34(ctx, x, b1, m + 20);
        RdBoolCur34(ctx, x, b2, m + 21);
    });
}

// The 28-byte XML record with four hashes (NewXml34 body): +12 / +16 /
// +20 cleared, vtable vt, +24 cleared; the hashes h1..h4 to +12..+24 (the
// first with default 0, the others current).
inline void XmlHHHH35(PPUContext& ctx, u32 vt, u32 h1, u32 h2, u32 h3, u32 h4) {
    NewXml34(ctx, 28, [&](u32 m, u32 x) {
        store<u32>(m + 12, 0);
        store<u32>(m + 16, 0);
        store<u32>(m + 20, 0);
        store<u32>(m, vt);
        store<u32>(m + 24, 0);
        RdHash34(ctx, x, h1, m + 12, 0);
        RdHashCur34(ctx, x, h2, m + 16);
        RdHashCur34(ctx, x, h3, m + 20);
        RdHashCur34(ctx, x, h4, m + 24);
    });
}

// The deleting destructor of the classes with vtables vt + 8 / + 436 /
// + 476 at +0 / +380 / +384, a pointer vector at +424 and an owned 20-byte
// vector +388 (0x0075140C, 0x00756DCC): nothing for o = 0; else the class
// vtables, the vector +424 cleared (PtrVector_Clear, Nop), with a +388 the
// 0x00B12890 class vtables (+384 / +380 / +0 order), that vector cleared
// and SmallFree'd (20), then the base set 0x00B126B8 / 0x00B1200C /
// 0x00B12528, VtB12550_Dtor_000DF934(o) and SmallFree(o, size). r3 as
// left.
inline void Delete424_35(PPUContext& ctx, u32 vt, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt + 8);
    store<u32>(o + 380, vt + 436);
    store<u32>(o + 384, vt + 476);
    call(ctx, 0x0054EC1C, {o + 424});
    call(ctx, 0x0054EC18, {o + 424});
    const u32 p = load<u32>(o + 388);
    if (p != 0) {
        store<u32>(o + 384, 0x00B12A6Cu);
        store<u32>(o + 380, 0x00B12A44u);
        store<u32>(o, 0x00B12898u);
        call(ctx, 0x0054EC1C, {p});
        call(ctx, 0x0054EC18, {p});
        call(ctx, 0x00552420, {load<u32>(o + 388), 20});
    }
    store<u32>(o + 384, 0x00B12528u);
    store<u32>(o + 380, 0x00B1200Cu);
    store<u32>(o, 0x00B126B8u);
    call(ctx, 0x000DF934, {o});
    call(ctx, 0x00552420, {o, size});
}

// The binary-tree seek of 0x004B6894 / 0x004B6C88 (void f(T* o)): key =
// o->+20 when o's u8 +24 has 0x80, else 0; in the tree S (root *(S->+12);
// nodes: key +4, object +8, left (greater keys) +12, right +16) the path
// is recorded (u16 depth S->+96, nodes S->+16..) while searching. Found:
// S->+100 = the node, the word cur = its object, and that object (when
// set) gets vt[slot](it) (r4 = o, left over). Not found: S->+100 = the last node visited (cur
// untouched); empty tree: cur = S->+100 = 0. r3 = 0 or the call's. (The
// original also has an unreachable "no node" branch.)
inline void TreeSeek35(PPUContext& ctx, u32 S, u32 cur, u32 slot) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 key = (load<u8>(o + 24) & 0x80u) != 0 ? load<u32>(o + 20) : 0u;
    ctx.gpr[3] = 0;
    u32 n = load<u32>(load<u32>(S + 12));
    if (n == 0) {
        store<u32>(cur, 0);
        store<u32>(S + 100, 0);
        return;
    }
    store<u16>(S + 96, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) break;
        const u32 c = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
        if (c == 0) {
            store<u32>(S + 100, n);
            return;
        }
        const s32 d = static_cast<s16>(load<u16>(S + 96));
        store<u16>(S + 96, static_cast<u16>(d + 1));
        store<u32>(S + 16 + 4 * static_cast<u32>(d), n);
        n = c;
    }
    store<u32>(S + 100, n);
    store<u32>(cur, load<u32>(n + 8));
    const u32 p = load<u32>(n + 8);
    if (p != 0) VCall(ctx, p, slot, {p, o});  // r4 still o in the original's call
}

}  // namespace obj35
}  // namespace sly4
