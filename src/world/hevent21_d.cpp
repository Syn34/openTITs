// World_HandleEvent callees, part 21d: the overlay grid mesh setup and the
// object request start.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kOverlay = 0x010AA1B0;         // overlay block (as Overlay_Setup428F34): u8 +0 flags (0x20 built)
constexpr u32 kBufferAlloc = 0x0052F198;     // (Buffer*, u32 bytes, 128, u8 dynamic, 2) (symbols.txt Buffer_Alloc52F198)
constexpr u32 kMemAlloc = 0x0054ED38;        // (size, pool, align) (symbols.txt Mem_Alloc)
constexpr u32 kChunkBytes = 128;

constexpr u32 kSpeakerSayId = 0x004A5C90;    // (Speaker*, u32 text id, u8; f32) (Speaker_SayId4A5C90)
constexpr u32 kEffectSetHash = 0x004CF988;   // (Effect*, u32 hash) (symbols.txt Effect_SetHash4CF988)
constexpr u32 kPtrVectorReserve = 0x00551718;  // (vec, new capacity) (as hevent8.h)
constexpr u32 kMemmove = 0x0071C3F0;         // libc memmove
constexpr u32 kOneF = 0x003690EC;            // f32 1.0
constexpr u32 kRequestHash = 0x04296E4C;     // effect hash set when a request starts (name?)

// The overlay's 36-byte chunk: +0 u32 size 128, +4 / +12 data, +8 / +16
// data end, +20 / +24 = 0, +28 = 1, u8 +32 bits 0xC0 cleared. A failed
// data allocation is still recorded (null + 128).
inline u32 new_chunk(PPUContext& ctx) {
    const u32 a = static_cast<u32>(call(ctx, kMemAlloc, {36, 0, 16}));
    if (a == 0) return 0;
    store<u32>(a, kChunkBytes);
    const u32 data = static_cast<u32>(call(ctx, kMemAlloc, {kChunkBytes, 0, 16}));
    const u32 end = data + load<u32>(a);
    const u32 fl = load<u8>(a + 32);
    store<u32>(a + 4, data);
    store<u32>(a + 12, data);
    store<u32>(a + 24, 0);
    store<u32>(a + 20, 0);
    store<u32>(a + 8, end);
    store<u32>(a + 16, end);
    store<u32>(a + 28, 1);
    store<u8>(a + 32, static_cast<u8>(fl & ~0xC0u));
    return a;
}
}  // namespace

// u32 Overlay_Setup428B74(u32 w, u32 h) (name?): unless the overlay is
// built (flag 0x20) or already w x h (u16 +82 / +84), builds a w x h grid:
// three vertex buffers (+8, +24, +40: (w+1)(h+1) 24-byte vertices, u16 +78
// count, u8 +76 = 0), a static index buffer (+56, +72 = 1, u16 +80 =
// 2wh + h + 1 indices, +64 bits 1..3 = 2) filled with a zig-zag triangle
// strip (0 first, then per row the (i, i + w + 1) pairs, walking right on
// even rows and left on odd ones), and a two-chunk list (+1124: 16-byte
// head {+0 first, +4 second, +8 first again, +12 0}; null when the head
// allocation fails). Sets the built flag; returns the new flags byte.
DECOMP(00428B74, Overlay_Setup428B74) {
    const u32 w = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 o = kOverlay;
    if (((load<u8>(o) >> 5) & 1) != 0) return;
    if (static_cast<s32>(load<u16>(o + 82)) == static_cast<s32>(w) && static_cast<s32>(load<u16>(o + 84)) == static_cast<s32>(h)) return;
    const u32 nv = w * h + h + w + 1;
    store<u16>(o + 82, static_cast<u16>(w));
    store<u16>(o + 84, static_cast<u16>(h));
    store<u8>(o + 76, 0);
    store<u16>(o + 78, static_cast<u16>(nv));
    for (u32 k = 0; k < 3; ++k) call(ctx, kBufferAlloc, {o + 8 + 16 * k, 24 * (nv & 0xFFFF), kChunkBytes, 1, 2});
    {
        const u32 w2 = (static_cast<u32>(load<u16>(o + 82)) << 1) & 0x1FFFEu;
        const u32 hh = load<u16>(o + 84);
        store<u32>(o + 72, 1);
        const u32 ni = hh * w2 + hh + 1;
        store<u16>(o + 80, static_cast<u16>(ni));
        call(ctx, kBufferAlloc, {o + 56, (ni << 1) & 0x1FFFEu, kChunkBytes, 0, 2});
    }
    const u32 ib = o + 56;
    store<u32>(ib + 8, (load<u32>(ib + 8) & ~0xEu) | 4u);
    u32 out = load<u32>(ib);
    store<u16>(out, 0);
    out += 2;
    if (static_cast<s32>(load<u16>(o + 84)) > 0) {
        u32 idx = 0;
        u32 ww = load<u16>(o + 82);
        for (u32 row = 0;;) {
            if (static_cast<s32>((ww & 0xFFFF) + 1) > 0) {
                for (u32 col = 0;;) {
                    if (col != 0) {
                        store<u16>(out, static_cast<u16>(idx));
                        out += 2;
                        ww = load<u16>(o + 82);
                    }
                    store<u16>(out, static_cast<u16>(ww + idx + 1));
                    out += 2;
                    ww = load<u16>(o + 82);
                    idx = (row & 1) == 0 ? idx + 1 : idx - 1;
                    col = (col + 1) & 0xFFFF;
                    if (!(static_cast<s32>(col) < static_cast<s32>(ww + 1))) break;
                }
            }
            const u32 t = ww + idx;
            idx = (row & 1) == 0 ? t : t + 2;
            row = (row + 1) & 0xFFFF;
            if (!(static_cast<s32>(row) < static_cast<s32>(load<u16>(o + 84)))) break;
        }
    }
    u32 list = static_cast<u32>(call(ctx, kMemAlloc, {16, 0, 16}));
    if (list != 0) {
        store<u32>(list, new_chunk(ctx));
        const u32 second = new_chunk(ctx);
        const u32 first = load<u32>(list);
        store<u32>(list + 4, second);
        store<u32>(list + 8, first);
        store<u32>(list + 12, 0);
    }
    store<u32>(o + 1124, list);
    const u32 fl = load<u8>(o) | 0x20u;
    store<u8>(o, static_cast<u8>(fl));
    ret(ctx, fl);
}

// u32 Obj_StartRequest3690F0(Object* o, u32 a, u32 b, Vector* items, u8 e)
// (name?): +208 bit 31 = e (stored in any case). Unless the mode (+160
// bits 1..31) is already 1: mode 1, vt +20 (o, 1); the definition's (+8)
// slots 0 / 1 (count +152, 16-byte table +148, object +12) get a and
// slots 2 / 3 get b, each as vt +188 (x, value, 0) then vt +148 (x), for
// a non-zero value; the items' (+0 data, u16 +8 count) first 9 bytes of
// each 12-byte element are appended to the +164 vector (u16 +168
// capacity, +172 count, +174 high water); Speaker_SayId4A5C90(+28, the
// mode's id +184 + 4 * mode, 0; 1.0), and with an effect (+20)
// Effect_SetHash4CF988(e, 0x04296E4C). Returns the last call's result.
DECOMP(003690F0, Obj_StartRequest3690F0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), items = arg<u32>(ctx, 3);
    const u32 e = arg<u32>(ctx, 4);
    u32 mode = load<u32>(o + 160);
    store<u32>(o + 208, (load<u32>(o + 208) & 0x7FFFFFFFu) | ((e & 1u) << 31));
    if ((mode >> 1) == 1) return;
    mode = (mode & 1u) | 2u;
    store<u32>(o + 160, mode);
    vcall(ctx, o, 20, {o, 1});
    u32 def = load<u32>(o + 8);
    // Slots 0..3: value a for 0 / 1, b for 2 / 3; the definition is reloaded
    // after each slot that was notified.
    for (u32 s = 0; s < 4; ++s) {
        const u32 v = s < 2 ? a : b;
        if (v == 0 || !(load<u32>(def + 152) > s)) continue;
        u32 x = load<u32>(load<u32>(def + 148) + 16 * s + 12);
        if (x == 0) continue;
        vcall(ctx, x, 188, {x, v, 0});
        x = load<u32>(load<u32>(def + 148) + 16 * s + 12);
        vcall(ctx, x, 148, {x});
        if (s < 3) def = load<u32>(o + 8);
    }
    store<u16>(o + 172, 0);
    if (items != 0) {
        const u32 vec = o + 164;
        u32 cnt = 0;  // the count as the original keeps it in a register
        for (u32 i = 0; i < load<u16>(items + 8); ++i) {
            const u32 src = load<u32>(items) + 12 * i;
            const u32 cap = load<u16>(o + 168);
            if ((cnt & 0xFFFF) == cap) {
                call(ctx, kPtrVectorReserve, {vec, (cap << 1) & 0x1FFFEu});
                cnt = load<u16>(o + 172);
            }
            call(ctx, kMemmove, {load<u32>(vec) + (cnt & 0xFFFF) * 12, src, 9});
            const u32 n = load<u16>(o + 172) + 1;
            const u32 high = load<u16>(o + 174);
            store<u16>(o + 172, static_cast<u16>(n));
            const u32 nh = static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : n;
            cnt = static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? (n & 0xFFFF) : n;
            store<u16>(o + 174, static_cast<u16>(nh));
        }
    }
    const u32 id = load<u32>(o + 184 + ((load<u32>(o + 160) >> 1) << 2));
    callf(ctx, kSpeakerSayId, {load<u32>(o + 28), id, 0}, {hevent7::lfs_f64(kOneF)});
    const u32 fx = load<u32>(o + 20);
    if (fx != 0) call(ctx, kEffectSetHash, {fx, kRequestHash});
}
