// World_HandleEvent callees, part 20i: the set tree drain, a transform
// forwarder and the AR render target setup.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kIter = 0x010C2888;            // tree iterator: +12 Tree* (+0 root), +16 Node*[] stack, +96 s16 depth, +100 Node*
constexpr u32 kSetErase4BD4B4 = 0x004BD4B4;  // (Set*) (name?)
constexpr u32 kTransform2CCFF0 = 0x002CCFF0; // (Obj*, const f32 pos[3] (row 3 of a 4x4 copy), u8, a, b, c, d) (name?)
constexpr u32 kThunk427BD0 = 0x00427BD0;     // (Obj*, const {u16 333, u32 1}*) (symbols.txt Thunk_00427BD0)
constexpr u32 kRenderTargetCreate = 0x00522FB8;  // (2, 0) -> RenderTarget* (symbols.txt)
constexpr u32 kStringCopy = 0x00553684;          // (dst, src, size) (symbols.txt String_Copy)
constexpr u32 kRenderTargetSetByte49 = 0x0052038C;  // (rt, u8) (symbols.txt)
constexpr u32 kRenderTargetSetViewport = 0x005200AC; // (rt, const u16 rect[4]) (symbols.txt)
constexpr u32 kArVtable = 0x00B11D80;        // the AR object's vtable
constexpr u32 kArTarget = 0x00F558D0;        // u32 RenderTarget*
constexpr u32 kArName = 0x00AC5C48;          // "RenderTarget_SlyAR"
constexpr u32 kDisplaySize = 0x010C5A20;     // +4 u16 width, +6 u16 height
}  // namespace

// void Sets_Drain4BD7D4() (name?): repeatedly takes the first (leftmost)
// node of the tree (the path pushed on the iterator's stack, +100 = the
// node) and erases its set (0x004BD4B4(node +4)) until the tree is empty.
DECOMP(004BD7D4, Sets_Drain4BD7D4) {
    for (;;) {
        u32 node = load<u32>(load<u32>(kIter + 12));
        store<u16>(kIter + 96, 0);
        if (node != 0) {
            for (u32 l; (l = load<u32>(node + 12)) != 0; node = l) {
                const s32 d = static_cast<s16>(load<u16>(kIter + 96));
                store<u16>(kIter + 96, static_cast<u16>(d + 1));
                store<u32>(kIter + 16 + (static_cast<u32>(d) << 2), node);
            }
        }
        store<u32>(kIter + 100, node);
        if (node == 0) break;
        call(ctx, kSetErase4BD4B4, {load<u32>(node + 4)});
    }
    ret(ctx, 0);
}

// void Obj_Transform2CD91C(Obj* o, u8 a, b, c, d, e) (name?): the owner's
// (+8) vt +64, then its +96 4x4 matrix copied (rows of three) to the stack
// and 0x002CCFF0(o, &row 3, a, b, c, d, e).
DECOMP(002CD91C, Obj_Transform2CD91C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFF;
    const u64 b = ctx.gpr[5], c = ctx.gpr[6], d = ctx.gpr[7], e = ctx.gpr[8];
    const u32 owner = load<u32>(o + 8);
    vcall(ctx, owner, 64, {owner});
    const u32 m = owner + 96;
    Stack frame(ctx, 64);
    const u32 rows = static_cast<u32>(ctx.gpr[1]);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 12; k += 4) store<u32>(rows + 16 * r + k, load<u32>(m + 16 * r + k));
    ctx.fpr[1] = hevent7::lfs_f64(m + 56);
    call(ctx, kTransform2CCFF0, {o, rows + 48, a, b, c, d, e});
}

// void Ar_Init0A85C0(ArObj* o) (name?): 0x00427BD0(o, {333, 1}), vtable
// 0x00B11D80, the AR globals (0x00F558C8..0x00F558E0) reset (0x00F558CC =
// -1), o +36 = 0; a render target (RenderTarget_Create(2, 0)) named
// "RenderTarget_SlyAR" at 0x00F558D0, byte 49 cleared, viewport {0, 0,
// display width, height}.
DECOMP(000A85C0, Ar_Init0A85C0) {
    const u32 o = arg<u32>(ctx, 0);
    {
        Stack frame(ctx, 16);
        const u32 p = static_cast<u32>(ctx.gpr[1]);
        store<u16>(p, 333);
        store<u32>(p + 4, 1);
        call(ctx, kThunk427BD0, {o, p, 333, 1});
    }
    store<u32>(o, kArVtable);
    store<u32>(0x00F558CC, 0xFFFFFFFFu);
    store<u16>(0x00F558D8, 0);
    store<u8>(0x00F558E0, 0);
    store<u16>(0x00F558DA, 0);
    store<u8>(0x00F558C9, 0);
    store<u32>(0x00F558DC, 0);
    store<u8>(0x00F558C8, 0);
    store<u32>(o + 36, 0);
    store<u32>(0x00F558D4, 0);
    const u32 rt = static_cast<u32>(call(ctx, kRenderTargetCreate, {2, 0}));
    store<u32>(kArTarget, rt);
    call(ctx, kStringCopy, {rt + 16, kArName, 32});
    call(ctx, kRenderTargetSetByte49, {load<u32>(kArTarget), 0});
    Stack frame(ctx, 16);
    const u32 rect = static_cast<u32>(ctx.gpr[1]);
    const u32 t = load<u32>(kArTarget);
    store<u16>(rect, 0);
    store<u16>(rect + 2, 0);
    const u32 w = load<u16>(kDisplaySize + 4), h = load<u16>(kDisplaySize + 6);
    store<u16>(rect + 4, static_cast<u16>(w));
    store<u16>(rect + 6, static_cast<u16>(h));
    call(ctx, kRenderTargetSetViewport, {t, rect, w, h});
}
