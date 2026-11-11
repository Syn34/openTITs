// Game code, medium sweep part 25: teardown of pool-allocated arrays
// ({u16 count, .., u16 2 * element size +6, data +8, allocator +12}
// released through the allocator's vt[8], then Array_Destruct) and of
// new[]-arrays of 16-byte records.
#include "mid25.h"

using namespace sly4;
using namespace sly4::mid25;

namespace {
constexpr u32 kArrayDestruct = 0x0054EC94u;   // Array_Destruct(Array*)
constexpr u32 kPoolBufferFree = 0x005662A0u;  // PoolBuffer_Free(p)

// The pool array at a: a set data block gets
// alloc->vt[8](alloc, data, count * ((u16 +6 >> 1) & 0x7FFF)); then
// Array_Destruct(a).
void ArrayFree(PPUContext& ctx, u32 a) {
    const u32 size = load<u16>(a) * ((load<u16>(a + 6) >> 1) & 0x7FFFu);
    if (const u32 d = load<u32>(a + 8); d != 0) {
        const u32 al = load<u32>(a + 12);
        VCall(ctx, al, 8, {al, d, size});
    }
    call(ctx, kArrayDestruct, {a});
}

// The new[]-array *slot (s32 count at -4, 16-byte records): each record's
// +8 block, last first, is freed (Mem_FreeFrom) and cleared; then the
// array (from its header) is freed and *slot cleared. Nothing when unset.
void RecArrayFree(PPUContext& ctx, u32 slot) {
    const u32 p = load<u32>(slot);
    if (p == 0) return;
    for (s32 i = static_cast<s32>(load<u32>(p - 4) - 1); i >= 0; --i) {
        const u32 e = p + 16 * static_cast<u32>(i);
        const u32 b = load<u32>(e + 8);
        if (b == 0) continue;
        call(ctx, m25fn::Mem_FreeFrom, {b, 0});
        store<u32>(e + 8, 0);
    }
    call(ctx, m25fn::Mem_FreeFrom, {p - 4, 0});
    store<u32>(slot, 0);
}
}  // namespace

// void f() (called by 0x00018B30): the global 0x00F6578C cleared; the track
// pool *0x00F65798 (when set): TrackPool_Update390724(it), the global
// cleared, its pool array +36 and record array +28 released, the pool freed;
// the pool *0x00F65788 (global cleared; pool arrays +24 and +0, freed); the
// buffer *0x00F65038 (global cleared; its block +24 freed,
// PoolBuffer_Free(it), freed). r3 = the last call's, or 0x00F60000 when all
// three are unset.
DECOMP(00390928, TrackPools_Shutdown_00390928) {
    u32 r3 = 0x00F60000u;  // lis r3, 0xF6 (the store's base) when nothing is called
    const u32 a = load<u32>(0x00F65798u);
    store<u32>(0x00F6578Cu, 0);
    if (a != 0) {
        call(ctx, 0x00390724u, {a});
        store<u32>(0x00F65798u, 0);
        ArrayFree(ctx, a + 36);
        RecArrayFree(ctx, a + 28);
        r3 = static_cast<u32>(call(ctx, m25fn::Mem_FreeFrom, {a, 0}));
    }
    if (const u32 b = load<u32>(0x00F65788u); b != 0) {
        store<u32>(0x00F65788u, 0);
        ArrayFree(ctx, b + 24);
        ArrayFree(ctx, b);
        r3 = static_cast<u32>(call(ctx, m25fn::Mem_FreeFrom, {b, 0}));
    }
    if (const u32 c = load<u32>(0x00F65038u); c != 0) {
        store<u32>(0x00F65038u, 0);
        call(ctx, m25fn::Mem_FreeFrom, {load<u32>(c + 24), 0});
        call(ctx, kPoolBufferFree, {c});
        call(ctx, m25fn::Mem_FreeFrom, {c, 0});
        return;
    }
    ctx.gpr[3] = r3;
}

// T::~T(o) (Thunk_004B1308): the global 0x00F65F84 cleared;
// Clips_Release4B0E70(o); the record array o->+256 and the pool arrays
// o + 72 / 48 / 24 released; PoolBuffer_Free(o) (r3 = its).
DECOMP(004B1114, Clips_Dtor_004B1114) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(0x00F65F84u, 0);
    call(ctx, 0x004B0E70u, {o});
    RecArrayFree(ctx, o + 256);
    ArrayFree(ctx, o + 72);
    ArrayFree(ctx, o + 48);
    ArrayFree(ctx, o + 24);
    call(ctx, kPoolBufferFree, {o});
}

namespace {
// For i < the u16 count at o + cnt: the object of the 8-byte entry
// (o->+arr)[i].+4, when set, gets vt[4]().
void Release8(PPUContext& ctx, u32 o, u32 cnt, u32 arr) {
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(o + cnt)); ++i) {
        const u32 e = load<u32>(load<u32>(o + arr) + 8 * i + 4);
        if (e != 0) VCall(ctx, e, 4, {e});
    }
}

// The handle of hash-table slot k: {u16 k, u16 generation (bits 15-30 of
// the slot's +12 word)}.
u32 SlotHandle(u32 tbl, u32 k) {
    return ((k & 0xFFFFu) << 16) | ((load<u32>(tbl + ((k << 4) & 0xFFFFFFF0u) + 12) >> 15) & 0xFFFFu);
}
}  // namespace

// T::~T(o) (Delete_004F4164, Thunk_004F41AC): vtable 0x00B22FC8, the
// global 0x00F668D8 cleared; the 24-byte records of the entries o->+88
// (u16 count +82) have their block +8 (+12 words) SmallFree'd and cleared
// and are SmallFree'd; the objects of the entries o->+64 (+58) and o->+40
// (+34) get vt[4](); the values (+8) of the live slots of the hash table
// o->+132 (16-byte slots, count +128, chained by +4 from the head +112 to
// the tail +116, generation in bits 15-30 of +12 and bit 31 free) get
// vt[4](); the objects of o->+16 (+10) get vt[4](); the buffer o->+4 gets
// PoolBuffer_Free and is freed and cleared; o->+132 freed and cleared; the
// pool arrays o + 80 / 56 / 32 / 8 released (r3 = the last
// Array_Destruct's).
DECOMP(004F3CB4, Obj_Dtor_B22FC8_004F3CB4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B22FC8u);
    store<u32>(0x00F668D8u, 0);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(o + 82)); ++i) {
        const u32 e = load<u32>(load<u32>(o + 88) + 8 * i + 4);
        if (e == 0) continue;
        const u32 sz = (load<u32>(e + 12) << 2) & 0xFFFFFFFCu;
        if (const u32 b = load<u32>(e + 8); b != 0) {
            call(ctx, m25fn::SmallFree, {b, sz});
            store<u32>(e + 8, 0);
        }
        call(ctx, m25fn::SmallFree, {e, 24});
    }
    Release8(ctx, o, 58, 64);
    Release8(ctx, o, 34, 40);
    u32 h = load<u32>(o + 120) == 0 ? 0 : SlotHandle(load<u32>(o + 132), load<u32>(o + 112));
    for (;;) {
        const u32 gen = h & 0xFFFFu;
        if (gen == 0) break;
        const u32 k = h >> 16;
        if (!(static_cast<s32>(k) < static_cast<s32>(load<u32>(o + 128)))) break;
        const u32 tbl = load<u32>(o + 132);
        const u32 s = tbl + 16 * k;
        if (((load<u32>(s + 12) >> 15) & 0xFFFFu) != gen) break;
        if ((load<u8>(s + 12) & 0x80u) != 0) break;
        h = k == load<u32>(o + 116) ? 0 : SlotHandle(tbl, load<u32>(s + 4));
        if (const u32 v = load<u32>(s + 8); v != 0) VCall(ctx, v, 4, {v});
    }
    Release8(ctx, o, 10, 16);
    if (const u32 b = load<u32>(o + 4); b != 0) {
        call(ctx, kPoolBufferFree, {b});
        call(ctx, m25fn::Mem_FreeFrom, {b, 0});
        store<u32>(o + 4, 0);
    }
    if (const u32 t = load<u32>(o + 132); t != 0) {
        call(ctx, m25fn::Mem_FreeFrom, {t, 0});
        store<u32>(o + 132, 0);
    }
    ArrayFree(ctx, o + 80);
    ArrayFree(ctx, o + 56);
    ArrayFree(ctx, o + 32);
    ArrayFree(ctx, o + 8);
}
