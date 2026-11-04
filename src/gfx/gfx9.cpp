// The renderer range's functions of 45 to ~65 instructions: the
// destructor bodies of Material, Texture and RenderTarget, the surface
// handle release, the IFF include reader and the renderer's IFF chunk
// handlers, the shader types' LoadParams slots, record-stream readers and
// writers, inline RSX state blocks (stencil, query pass), the query
// slots, the zlib inflate-queue wrappers' destructors and a worker
// thread loop.
// Names carry the address where the role is unclear.
#include <atomic>
#include <bit>
#include <cmath>

#include "opentit/recomp.h"

#include "fx.h"
#include "gfx9.h"

using namespace sly4;

namespace {

namespace g8 = sly4::render::gfx8;
namespace g9 = sly4::render::gfx9;

// Name list filled by ReadName_0051D198: +28 u32 count, +32 u32 entries[]
// (String_Dup'd), +96 u32, +100 char[64] (gfx7_misc.cpp kName010DDB40).
constexpr u32 k9NameList = 0x010DDADC;
constexpr u32 k9EmptyString = 0x00AF1B54;  // ""

// The 16 query slots (layers3::g::slots), 176 bytes each, u8 +4 in use.
constexpr u32 k9Slots = 0x010DFAD0, k9SlotCount = 16, k9SlotSize = 176;
constexpr u32 k9QueryEnabled = 0x00F66FD5;  // u8 (name?)
constexpr u32 k9QueryActive = 0x010E05D0;   // u8 (name?)

constexpr u32 k9TileSlots = 0x00F67244;  // u16 bitmask of the tile regions in use (tgt::g::tile_slots)

// static ApeTFreeList<AglMaterialAnim>& ApeTMemClassFL<AglMaterialAnim, 256>::GetAllocator()
// (hevent2_b.cpp kAnimFreeList and friends).
constexpr u32 k9AnimFreeList = 0x011DE4B8;
constexpr u32 k9FreeListVtable = 0x00B0EF88;
constexpr u32 k9AnimFreeListName = 0x00AF10D9;  // the GetAllocator() signature
constexpr u32 k9AnimSize = 152, k9AnimPerBlock = 256, k9FreeListObjSize = 60, k9FreeListAlign = 16;

constexpr u32 k9FileModeRb = 0x00AF37C0;  // "rb"

// LV2 event flag system calls.
constexpr u32 k9SysEventFlagWait = 85;    // (id, bitptn, mode, u64* result, timeout)
constexpr u32 k9SysEventFlagSet = 87;     // (id, bitptn)
constexpr u32 k9SysEventFlagClear = 118;  // (id, bitptn)

// The 16-byte buffer part {+0 p, +4 bit 0: pool 5, +8 bit 0: not owned,
// +12 tag in bits 28..31} freed the way Pool_DestructPart (0052EFE0) does,
// inlined: Mem_FreePool(p, pool, tag) unless p is null or not owned; +0
// cleared after a free.
void free_part9(PPUContext& ctx, u32 part) {
    const u32 p = load<u32>(part);
    if (p == 0 || (load<u32>(part + 8) & 1) != 0) return;
    const u32 pool = (load<u32>(part + 4) & 1) != 0 ? 5u : 4u;
    call(ctx, g8::fn::Mem_FreePool, {p, pool, load<u32>(part + 12) >> 28});
    store<u32>(part, 0);
}

// The shader types' LoadParams texture step (r4 = Material* m, r5 =
// const Desc* d): the loaded texture named by d->+name_off, when found,
// gets virtual slot 4 (+16) (tex, 1, mode, 1) and f_004FF008(m, tex),
// then the top bits of m->+120 become 2 << 28 when u8 d->+76 is 2, else
// 3 << 28 (bits 30..31 cleared). Returns whether it was found.
bool load_texture9(PPUContext& ctx, u32 name_off, u32 mode) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 tex = static_cast<u32>(call(ctx, 0x0050C398u /* Tex_FindLoaded */, {load<u32>(d + name_off)}));
    if (tex == 0) return false;
    fx::vcall(ctx, tex, 16, {tex, 1, mode, 1});
    call(ctx, 0x004FF008u, {m, tex});
    const u32 top = load<u8>(d + 76) == 2 ? 2u : 3u;
    const u32 w = load<u32>(m + 120);
    store<u32>(m + 120, (w & 0x0FFFFFFFu) | ((top << 28) & 0x30000000u));
    return true;
}

// The record stream (f_005159E8) reader: skips the 12-byte record header
// at the cursor (+12; the header and each payload wrap to +0 when they
// would pass the end +4, unsigned) and returns the first payload word's
// address, the cursor moved past it.
u32 stream_first9(u32 s) {
    const u32 p = load<u32>(s + 12), end = load<u32>(s + 4), start = load<u32>(s);
    const u32 h = ((p + 15 > end ? start : p) + 3) & ~3u;  // cmplw
    const u32 a = ((h + 19 > end ? start : h + 12) + 3) & ~3u;
    store<u32>(s + 12, a + 4);
    return a;
}

// The next payload of `size` bytes after the word at a (wrapping the same way).
u32 stream_next9(u32 s, u32 a, u32 size) {
    const u32 b = ((a + 4 + size + 3 > load<u32>(s + 4) ? load<u32>(s) : a + 4) + 3) & ~3u;
    store<u32>(s + 12, b + size);
    return b;
}

// InflateQueueLF_Destruct's body (007A6638, inlined in 007A66EC).
void lfqueue_destruct9(PPUContext& ctx, u32 self) {
    const u32 lfq = load<u32>(self + 11980);
    if (lfq != 0) {
        store<u32>(self, 0x00B245A4u);
        call(ctx, g9::fn::LFQueue_Detach_0060D988, {lfq, 0x00B245A4u});  // r4: the vtable, left over
        store<u32>(self + 11980, 0);
    }
    const u32 ef = load<u32>(self + 11972);
    store<u32>(self, 0x00B24558u);
    store<u8>(self + 11992, 0);
    if (ef != 0) call(ctx, g9::fn::cellSpursEventFlagDetachLv2EventQueue, {self + 10624, 0, ef});
    const u32 ts = load<u32>(self + 11976);
    if (ts != 0) {
        call(ctx, g9::fn::cellSpursShutdownTaskset, {ts});
        call(ctx, g9::fn::cellSpursJoinTaskset, {load<u32>(self + 11976)});
        store<u32>(self + 11976, 0);
    }
    store<u32>(self, 0x00B2451Cu);
}

// Release a refcounted object: virtual slot 2 (+8) on it.
void release9(PPUContext& ctx, u32 obj) { fx::vcall(ctx, obj, 8, {obj}); }

// A texture's format copied from another (Iff_HandleNTXD, 005398CC):
// +72..+92 and +56 / +60, the low nibble of +64 kept with bits 4..31 and
// then 1..3 from src and bit 0 set, +96 = 0, the top nibble of +68 from
// src; loads and stores in the original order.
void copy_format9(u32 d, u32 src) {
    u32 w[6];
    for (u32 k = 0; k < 4; ++k) w[k] = load<u32>(src + 72 + 4 * k);
    for (u32 k = 0; k < 4; ++k) store<u32>(d + 72 + 4 * k, w[k]);
    w[4] = load<u32>(src + 88);
    w[5] = load<u32>(src + 92);
    store<u32>(d + 88, w[4]);
    store<u32>(d + 92, w[5]);
    store<u32>(d + 56, load<u32>(src + 56));
    store<u32>(d + 60, load<u32>(src + 60));
    u32 f = (load<u32>(d + 64) & 0xFu) | (load<u32>(src + 64) & ~0xFu);
    const u32 h = load<u32>(d + 68) & 0x0FFFFFFFu;
    store<u32>(d + 64, f);
    f = (f & ~0xEu) | (load<u32>(src + 64) & 0xEu);
    store<u32>(d + 64, f);
    const u32 s68 = load<u32>(src + 68);
    store<u32>(d + 64, f | 1);
    store<u32>(d + 96, 0);
    store<u32>(d + 68, h | (s68 & 0xF0000000u));
}

// The TextureBase destructor body (0050A5E4 and 0050A404).
void texture_destruct9(PPUContext& ctx, u32 t) {
    const u32 surf = load<u32>(t + 108);
    store<u32>(t, 0x00B23380u);
    if (surf != 0) {
        fx::vcall(ctx, surf, 8, {surf});
        store<u32>(t + 108, 0);
    }
    call(ctx, g9::fn::Pool_DestructPart, {t + 56});
    if (load<u8>(t + 104) == 0) {
        const u32 p = load<u32>(t + 96);
        if (p != 0) {
            call(ctx, g9::fn::Release_0052F06C, {p});
            call(ctx, g8::fn::Mem_FreeFrom, {p, 0});
            store<u32>(t + 96, 0);
        }
    }
    call(ctx, g9::fn::Pool_DestructPart, {t + 56});  // the same part again
    call(ctx, g9::fn::ApeRoot_Destruct, {t});
}

// The RenderTargetBase destructor body (0052163C and 005214A4).
void rt_destruct9(PPUContext& ctx, u32 rt) {
    const u32 buf = load<u32>(rt + 672);
    store<u32>(rt, 0x00B236C0u);
    if (buf != 0) {
        call(ctx, g8::fn::Mem_FreeFrom, {buf, 0});
        store<u32>(rt + 672, 0);
    }
    const u32 obj = load<u32>(rt + 656);
    if (obj != 0) fx::vcall(ctx, obj, 12, {obj});
    const u32 tex = load<u32>(rt + 516);
    if (tex != 0) {
        release9(ctx, tex);
        store<u32>(rt + 516, 0);
    }
    call(ctx, g9::fn::ApeRoot_Destruct, {rt});
}

}  // namespace

// void (): frees the name list at 0x010DDADC from the last entry down
// (Mem_FreeFrom(p, 0), entry cleared), with +96 = 0 first; when it held
// any entries the name at +100 is then set to "". An empty list returns 0
// without touching the name.
DECOMP(0051E824, NameList_Clear_0051E824) {
    const u32 list = k9NameList;
    store<u32>(list + 96, 0);
    u32 n = load<u32>(list + 28);
    if (n == 0) return ret(ctx, 0u);
    do {
        --n;
        store<u32>(list + 28, n);
        const u32 p = load<u32>(list + 32 + 4 * n);
        if (p != 0) {
            call(ctx, g8::fn::Mem_FreeFrom, {p, 0});
            n = load<u32>(list + 28);
            store<u32>(list + 32 + 4 * n, 0);
        }
    } while (n != 0);
    call(ctx, sly4::fn::String_Copy, {list + 100, k9EmptyString, 64});
}

// void (RenderTarget* rt, Texture* tex): releases the texture bound at
// rt->+516 (virtual slot 2, then cleared), binds tex (rt->+512 = its
// surface tex->+108, rt->+516 = tex, u16 tex->+4 refcount + 1) and sets
// the viewport to {0, 0, surface->+56, surface->+60} (name?)
DECOMP(005229B0, RenderTarget_BindTexture_005229B0) {
    const u32 rt = arg<u32>(ctx, 0), tex = arg<u32>(ctx, 1);
    const u32 old = load<u32>(rt + 516);
    if (old != 0) {
        release9(ctx, old);
        store<u32>(rt + 516, 0);
    }
    store<u32>(rt + 512, load<u32>(tex + 108));
    const u16 refs = load<u16>(tex + 4);
    store<u32>(rt + 516, tex);
    store<u16>(tex + 4, static_cast<u16>(refs + 1));
    const u32 surf = load<u32>(rt + 512);
    Stack rect(ctx, 16);  // u16 {x, y, w, h} at 112(r1) of a 144-byte frame
    store<u16>(rect.addr() + 0, 0);
    store<u16>(rect.addr() + 2, 0);
    store<u16>(rect.addr() + 4, static_cast<u16>(load<u32>(surf + 56)));
    store<u16>(rect.addr() + 6, static_cast<u16>(load<u32>(surf + 60)));
    call(ctx, g9::fn::RenderTarget_SetViewport, {rt, rect.addr()});
}

// The Edge zlib LF-queue wrapper's destructor (vtable 0x00B245A4): when
// the LF queue (+11980) is attached, 0060D988 on it (detach) and +11980 =
// 0; then the inflate-queue part, InflateQueue_Destruct (007A60A0) inlined:
// vtable 0x00B24558, u8 +11992 = 0, the event flag (+11972) detached from
// this + 10624, the taskset (+11976) shut down, joined and cleared, and
// the base vtable 0x00B2451C last  [vtable 00B245A4[0]]
DECOMP(007A6638, InflateQueueLF_Destruct) {
    lfqueue_destruct9(ctx, arg<u32>(ctx, 0));
    ctx.gpr[3] = 0x00B2451Cu;  // lis/addic r3 for the last store
}

// The LF-queue wrapper's deleting destructor [vtable 00B245A4[1]]: for a
// non-null this, InflateQueueLF_Destruct inlined, then Mem_FreeFrom(this, 0).
DECOMP(007A66EC, InflateQueueLF_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return ret(ctx, 0u);
    lfqueue_destruct9(ctx, self);
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0, 0x00B2451Cu});
}

// void (): when both u8 0x00F66FD5 and u8 0x010E05D0 are set:
// Call_005488AC(), virtual slot 2 (+8) on every query slot in use (u8
// +4), then Call_00548900() and Draw_InvalidateState()  [called by Call_00548B88]
DECOMP(00527DAC, QuerySlots_Flush_00527DAC) {
    if (load<u8>(k9QueryEnabled) == 0) return ret(ctx, 0u);
    if (load<u8>(k9QueryActive) == 0) return ret(ctx, 0u);
    call(ctx, g9::fn::Query_Begin_005488AC);
    for (u32 i = 0; i < k9SlotCount; ++i) {
        const u32 slot = k9Slots + k9SlotSize * i;
        if (load<u8>(slot + 4) != 0) fx::vcall(ctx, slot, 8, {slot});
    }
    call(ctx, g9::fn::Query_End_00548900);
    call(ctx, g9::fn::Draw_InvalidateState);
}

// void (Worker* w) thread entry, never returns: waits for bit 1 of the
// event flag w->+20 (sys_event_flag_wait, mode 1, no timeout) and on
// success clears it (sys_event_flag_clear(id, 0)); then runs the job —
// w->+32 (OPD) on w->+36 when w->+44 is 0, else w->+40 (OPD, when set) on
// w->+44 — and signals bit 1 of the event flag w->+28  [thread entry]
DECOMP(00527F2C, Worker_ThreadLoop_00527F2C) {
    const u32 w = arg<u32>(ctx, 0);
    for (;;) {
        const u64 r = syscall(ctx, k9SysEventFlagWait, {load<u32>(w + 20), 1, 1, 0, 0});
        if (static_cast<u32>(r) == 0) syscall(ctx, k9SysEventFlagClear, {load<u32>(w + 20), 0});
        const u32 user = load<u32>(w + 44);
        if (user == 0) {
            const u32 opd = load<u32>(w + 32);
            call_ptr(ctx, opd, {load<u32>(w + 36), opd, load<u32>(opd)});
        } else {
            const u32 opd = load<u32>(w + 40);
            if (opd != 0) call_ptr(ctx, opd, {user, load<u32>(opd), opd});
        }
        syscall(ctx, k9SysEventFlagSet, {load<u32>(w + 28), 1});
    }
}

// void Material_DestructBody(Material* m): vtable 0x00B233B0; the
// animation block (+128) goes back to the AglMaterialAnim free list
// (ApeTMemClassFL<AglMaterialAnim, 256>::GetAllocator inlined: the list is
// built on first use; virtual slot 4 Free(list, anim)) and +128 = 0; the
// f32 value array (+124, u8 +121 entries) is SmallFree'd and cleared; the
// u8 +44 texture slots at +20 are cleared and released (u16 +48 = 0 at
// slot 0); then the five slots +36 down to +20 are released again when
// still set (no clear), and ApeRoot_Destruct.
// Quirk kept: a failed Mem_Alloc for the free list stores 0 and still calls through it.
DECOMP(0050C118, Material_DestructBody) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 anim = load<u32>(m + 128);
    store<u32>(m, 0x00B233B0u);
    if (anim != 0) {
        u32 fl = load<u32>(k9AnimFreeList);
        if (fl == 0) {
            const u32 mem = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {k9FreeListObjSize, 0, k9FreeListAlign}));
            if (mem != 0) {
                // ApeTFreeList<AglMaterialAnim> constructor (inlined), stores in the original order
                for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(mem + off, 0);
                const u16 flags = load<u16>(mem + 54);
                for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(mem + off, 0);
                store<u16>(mem + 52, 0);
                store<u16>(mem + 54, static_cast<u16>(flags & ~0x8000u));
                store<u32>(mem + 0, k9FreeListVtable);
                store<u32>(mem + 56, k9AnimFreeListName);
                ctx.gpr[8] = k9FreeListVtable;  // left over from the stores above
                ctx.gpr[9] = k9AnimFreeListName;
                call(ctx, g9::fn::FreeList_Init, {mem, k9AnimSize, k9AnimPerBlock, 0, 0});
            }
            fl = mem;
            store<u32>(k9AnimFreeList, fl);
        }
        fx::vcall(ctx, fl, 16, {fl, anim});  // Free(anim)
        store<u32>(m + 128, 0);
    }
    const u32 values = load<u32>(m + 124);
    if (values != 0) {
        call(ctx, g9::fn::SmallFree, {values, static_cast<u32>(load<u8>(m + 121)) << 2});
        store<u32>(m + 124, 0);
    }
    if (load<u8>(m + 44) != 0) {
        for (u32 i = 0; i < load<u8>(m + 44); ++i) {
            const u32 slot = m + 20 + 4 * i;
            const u32 p = load<u32>(slot);
            if (p != 0) {
                store<u32>(slot, 0);
                release9(ctx, p);
            }
            if (i == 0) store<u16>(m + 48, 0);
        }
    }
    for (s32 k = 4; k >= 0; --k) {
        const u32 p = load<u32>(m + 20 + 4 * static_cast<u32>(k));
        if (p != 0) release9(ctx, p);
    }
    call(ctx, g9::fn::ApeRoot_Destruct, {m});
}

// void Texture_DestructBody(Texture* t): vtable 0x00B23380; the surface
// object at +108 released (virtual slot 2) and cleared; Pool_DestructPart
// (t + 56); unless u8 +104 is set, the object at +96 goes through
// 0052F06C, Mem_FreeFrom(p, 0) and +96 = 0; Pool_DestructPart(t + 56)
// again, then ApeRoot_Destruct.
DECOMP(0050A5E4, Texture_DestructBody) { texture_destruct9(ctx, arg<u32>(ctx, 0)); }

// TextureBase's destructor [vtable 00B23380[0]]: the same body as
// Texture_DestructBody, compiled separately.
DECOMP(0050A404, TextureBase_Destruct) { texture_destruct9(ctx, arg<u32>(ctx, 0)); }

// void RenderTarget_DestructBody(RenderTarget* rt): vtable 0x00B236C0;
// Mem_FreeFrom(+672, 0) and +672 = 0 when set; virtual slot 3 (+12) on
// the object at +656 when set; the bound texture (+516) released (slot
// 2) and cleared; then ApeRoot_Destruct.
DECOMP(0052163C, RenderTarget_DestructBody) { rt_destruct9(ctx, arg<u32>(ctx, 0)); }

// RenderTargetBase's destructor [vtable 00B236C0[0]]: the same body as
// RenderTarget_DestructBody, compiled separately.
DECOMP(005214A4, RenderTargetBase_Destruct) { rt_destruct9(ctx, arg<u32>(ctx, 0)); }

// RenderTargetBase's deleting destructor [vtable 00B236C0[1]]: for a
// non-null this, the same body, then Mem_FreeFrom(this, 0).
DECOMP(00521564, RenderTargetBase_Delete) {
    const u32 rt = arg<u32>(ctx, 0);
    if (rt == 0) return ret(ctx, 0u);
    rt_destruct9(ctx, rt);
    call(ctx, g8::fn::Mem_FreeFrom, {rt, 0});
}

// void Handle_Release(Handle** slot): for a handle h = *slot whose u8 +56
// (shared) is clear: cellGcmUnbindTile(s8 h->+45) and
// cellGcmUnbindZcull(s8 h->+47) unless -1; then, still unshared, the tile
// region's bit cleared in the u16 at 0x00F67244 and the buffer part at
// h+0 freed; the part is freed a second time when still set (the inlined
// destructor's member), Mem_FreeFrom(h, 0). *slot = 0 always (a shared
// handle is only dropped).
DECOMP(00537684, Handle_Release) {
    const u32 slot = arg<u32>(ctx, 0);
    u32 h = load<u32>(slot);
    if (h != 0 && load<u8>(h + 56) == 0) {
        const u8 tile = load<u8>(h + 45);
        if (static_cast<s8>(tile) != -1) {
            call(ctx, g9::fn::cellGcmUnbindTile, {tile, static_cast<u32>(static_cast<s32>(static_cast<s8>(tile)))});
            h = load<u32>(slot);
        }
        const u8 zcull = load<u8>(h + 47);
        if (static_cast<s8>(zcull) != -1) {
            call(ctx, g9::fn::cellGcmUnbindZcull, {zcull, static_cast<u32>(static_cast<s32>(static_cast<s8>(zcull)))});
            h = load<u32>(slot);
        }
        if (h != 0) {
            if (load<u8>(h + 56) == 0) {
                const u8 region = load<u8>(h + 45);
                if (static_cast<s8>(region) != -1) {
                    const u32 sh = region & 63u;  // slw: shifts of 32..63 give 0
                    const u32 bit = sh < 32 ? 1u << sh : 0u;
                    store<u16>(k9TileSlots, static_cast<u16>(load<u16>(k9TileSlots) & ~bit));
                }
                free_part9(ctx, h);
            }
            free_part9(ctx, h);
            call(ctx, g8::fn::Mem_FreeFrom, {h, 0});
        }
    }
    store<u32>(slot, 0);
    ret(ctx, 0u);
}

// bool Iff_ReadInclude(IffReader* r, char* path_out): reads the current
// chunk's data (its size from the top of r's chunk stack, entry 0 at
// depth -1 unchecked: a null entry is read at 8) into a 256-byte name
// through the stream's virtual slot 3 (+12) (stream, buf, size, 0), adding
// the count to r->+8; builds the path as the directory of the stream's own
// name (stream + 4, up to and including the last '/', ':' or '\\') plus
// that name, copies it to path_out when given, and opens it ("rb"). On
// success its chunks are read into a nested reader (a fresh 248-byte
// chunk stack {20, -1}, flags u8 r->+12) with Iff_ReadChunks(nested, 0),
// the file is closed (virtual slot 2) and the stack freed (Mem_FreePool(p,
// 0, 0)); returns 1. Returns 0 when the file does not open.
// Quirk kept: a separator at index 0 makes the directory end null, so
// 0 - base bytes are copied (as the original's strncpy does).
DECOMP(00569638, Iff_ReadInclude) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    // The original's locals sit at 112(r1)..639(r1) of a 688-byte frame;
    // reserving 576 bytes puts them at the same addresses (entry r1 - 576).
    Stack frame(ctx, 576);
    const u32 nested = frame.addr();  // IffReader {+0 stack, +4 stream, +8 pos, +12 u8 flags}
    const u32 name = nested + 16, path = nested + 272;

    const u32 stream = load<u32>(r + 4), st = load<u32>(r + 0);
    const u32 opd = load<u32>(load<u32>(stream) + 12);
    const s32 top = static_cast<s32>(load<u32>(st + 4));
    const u32 entry = top > -1 ? st + 8 + 12 * static_cast<u32>(top) : 0u;
    const u32 got = static_cast<u32>(call_ptr(ctx, opd, {stream, name, load<u32>(entry + 8), 0, opd, load<u32>(opd)}));
    store<u32>(r + 8, load<u32>(r + 8) + got);

    const u32 base = load<u32>(r + 4) + 4;  // the stream's file name
    u32 end = 0;
    if (base != 0) {
        s32 i = static_cast<s32>(call(ctx, g9::fn::Libc_strlen, {base}));
        while (i >= 0) {
            const u8 c = load<u8>(base + static_cast<u32>(i));
            if (c == '/' || c == ':' || c == '\\') break;
            --i;
        }
        end = i != 0 ? base + static_cast<u32>(i) + 1 : 0u;
    }
    const u32 n = end - base;
    if (n != 0) call(ctx, g9::fn::Libc_strncpy, {path, base, n});
    store<u8>(path + n, 0);
    call(ctx, g9::fn::Libc_strcat, {path, name});
    if (out != 0) call(ctx, g9::fn::Libc_strcpy, {out, path});

    const u32 file = static_cast<u32>(call(ctx, sly4::fn::File_Open, {path, k9FileModeRb}));
    if (file == 0) return ret(ctx, 0u);
    store<u32>(nested + 4, 0);
    const u32 stack = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {248}));
    if (stack != 0) {
        store<u32>(stack + 0, 20);  // capacity
        store<u32>(stack + 4, 0xFFFFFFFFu);  // depth -1
    }
    store<u32>(nested + 0, stack);
    // The original also reads the u8 at 0x01164E4C here, but its bit is
    // masked out again: the flags are copied unchanged.
    const u8 flags = load<u8>(r + 12);
    store<u32>(nested + 4, file);
    store<u32>(nested + 8, 0);
    store<u8>(nested + 12, flags);
    call(ctx, g9::fn::Iff_ReadChunks, {nested, 0});
    store<u32>(nested + 4, 0);
    release9(ctx, file);  // close
    const u32 left = load<u32>(nested + 0);
    if (left != 0) call(ctx, g8::fn::Mem_FreePool, {left, 0, 0});
    ret(ctx, 1u);
}

// void SystemState_CallHook(): the SystemState static (built on first
// use) calls its hook +20 (OPD) with +24 when set; returns 0 otherwise.
DECOMP(005DBB60, SystemState_CallHook) {
    // system_state::get, with the stores spelled out in the original's order
    const u32 s = system_state::object;
    if (load<u8>(system_state::guard) == 0) {
        const u8 b = load<u8>(s + 32);
        store<u32>(s + 20, 0);
        store<u32>(s + 24, 0);
        store<u8>(s + 32, static_cast<u8>(b & ~0xC0u));
        call(ctx, sly4::fn::memset, {s + 92, 0, 400});
        store<u32>(system_state::registry_slot, s);
        store<u8>(system_state::guard, 1);
    }
    const u32 opd = load<u32>(s + 20);
    if (opd == 0) return ret(ctx, 0u);
    call_ptr(ctx, opd, {load<u32>(s + 24), load<u32>(opd), opd});
}

// void (CallbackList* l): for each of the u8 l->+64 16-byte entries
// {+0 OPD, +4 user, +8 u32 id} whose OPD is set and whose id equals the
// u8 +48 of the current surface's view (draw::g::current_surface->+672):
// OPD(user), then the two cached-state u64 at 0x010DD598 / 0x010DD5A0 =
// -1. Returns the count  [called by Render_Pass575C34]
DECOMP(005132D8, CallbackList_RunForView_005132D8) {
    const u32 l = arg<u32>(ctx, 0);
    u32 n = load<u8>(l + 64);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(n); i = (i + 1) & 0xFF) {
        const u32 e = l + 16 * i;
        const u32 opd = load<u32>(e);
        if (opd == 0) continue;
        const u32 view = load<u32>(load<u32>(0x010DDA74u) + 672);
        if (static_cast<u32>(load<u8>(view + 48)) != load<u32>(e + 8)) continue;
        call_ptr(ctx, opd, {load<u32>(e + 4), opd, load<u32>(opd)});
        store<u64>(0x010DD598u, ~0ull);  // render::g::state_cache_a
        store<u64>(0x010DD5A0u, ~0ull);
        n = load<u8>(l + 64);
    }
    ret(ctx, n);
}

// void (Obj* o): the constructor body behind Thunk_0052DACC: u8 +20 = 0,
// the tuning floats +24..+92 (-1.0 at +36, 0.05, 0.5, 1.0, 0.0015, 2.0,
// 50.0, 5.0, u32 1 at +92), u8 +96 bits 0x80 / 0x40 cleared and 0x20 set,
// vtable 0x00B24158, the u32 at +128 atomically cleared (lwarx / stwcx.),
// then vtable 0x00B23D50. Returns o.
DECOMP(0052DA10, Construct_0052DA10) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 20, 0);
    store<u32>(o + 24, 0);
    store<u32>(o + 28, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0xBF800000u);  // -1.0f
    store<u32>(o + 40, 0);
    store<u32>(o + 48, 0);
    store<u32>(o + 52, 0);
    store<u32>(o + 56, 0x3D4CCCCDu);  // 0.05f
    store<u32>(o + 60, 0x3F000000u);  // 0.5f
    const u8 b = load<u8>(o + 96);
    store<u32>(o + 64, 0x3F800000u);  // 1.0f
    store<u32>(o + 68, 0x3AC49BA6u);  // 0.0015f
    store<u32>(o + 72, 0x40000000u);  // 2.0f
    store<u32>(o + 76, 0);
    store<u32>(o + 80, 0);
    store<u32>(o + 84, 0x42480000u);  // 50.0f
    store<u32>(o + 88, 0x40A00000u);  // 5.0f
    store<u32>(o + 92, 1);
    store<u8>(o + 96, static_cast<u8>((b & ~0xC0u) | 0x20u));
    store<u32>(o, 0x00B24158u);
    const u32 old = std::atomic_ref<u32>(ref<u32>(o + 128)).exchange(0);  // 0 in either byte order
    store<u32>(o, 0x00B23D50u);
    ctx.gpr[4] = 0x00B23D50u;  // left as the original leaves them
    ctx.gpr[5] = 0;
    ctx.gpr[6] = __builtin_bswap32(old);
    ret(ctx, o);
}

// void (Ent* e): when the u8 at 0x0119509C is set: entry e->+0 of the
// table at 0x011950A0 (+12 base, 128-byte entries) gets +96 = 0 and bit 0
// of its u8 +0 cleared; Obj_SetSlot20(obj, 0, 0) on the object at
// 0x00F8896C when its u8 +44 is set; f_007A9530(table, s32 index) and
// e->+0 = -1. Returns -1 (0 when the flag is clear)  [called by f_00289AA8]
DECOMP(005AD0A0, TableEntry_Release_005AD0A0) {
    const u32 e = arg<u32>(ctx, 0);
    if (load<u8>(0x0119509Cu) == 0) return ret(ctx, 0u);
    const u32 table = 0x011950A0u;
    u32 idx = load<u32>(e);
    const u32 ent = load<u32>(table + 12) + (idx << 7);
    const u8 b = load<u8>(ent);
    store<u32>(ent + 96, 0);
    store<u8>(ent, static_cast<u8>(b & ~1u));
    const u32 obj = load<u32>(0x00F8896Cu);
    if (load<u8>(obj + 44) != 0) {
        call(ctx, 0x004FEF48u /* Obj_SetSlot20 */, {obj, 0, 0});
        idx = load<u32>(e);
    }
    call(ctx, 0x007A9530u, {table, static_cast<u64>(static_cast<s64>(static_cast<s32>(idx)))});
    store<u32>(e, 0xFFFFFFFFu);
    ret(ctx, -1);
}

// bool SACSkinned_LoadParams(ShaderType* this, Material* m, const Desc* d)
// [vtable 00B25658[1], the SAC_Skinned type]: the loaded texture named by
// d->+4 (Tex_FindLoaded), when found, gets virtual slot 4 (+16) (tex, 1,
// 3, 1) and f_004FF008(m, tex), then the top bits of m->+120 become
// 2 << 28 when u8 d->+76 is 2, else 3 << 28 (bits 30..31 cleared).
// Returns 1.
DECOMP(005B7118, SACSkinned_LoadParams) {
    load_texture9(ctx, 4, 3);
    ret(ctx, 1u);
}

// bool ProjTxr_LoadParams(ShaderType*, Material* m, const Desc* d)
// [vtable 00B25330[1], the ProjTxr type]: as SACSkinned_LoadParams with
// the texture name at d->+16 and (tex, 1, 1, 1); returns whether the
// texture was found.
DECOMP(005B1728, ProjTxr_LoadParams) { ret(ctx, load_texture9(ctx, 16, 1) ? 1u : 0u); }

// bool Sly2UV_LoadParams  [vtable 00B25890[1], the Sly2UV type]: the same body.
DECOMP(005BA68C, Sly2UV_LoadParams) { ret(ctx, load_texture9(ctx, 16, 1) ? 1u : 0u); }

// bool SlyProjTxr_LoadParams  [vtable 00B25D50[1], the SlyProjTxr type]: the same body.
DECOMP(005CAC3C, SlyProjTxr_LoadParams) { ret(ctx, load_texture9(ctx, 16, 1) ? 1u : 0u); }

// bool Iff_HandlePrfn(IffReader* r): the PRFN chunk handler (registered by
// Platform_Init): the rest of the current chunk (its start +4 plus size
// +8, less the stream's u64 position +264; 0 at depth -1 or below) is
// read into a 1024-byte stack buffer through the stream's virtual slot 3
// (+12) (stream, buf, size, 0) and discarded; the count is added to r->+8.
// Returns 1. Quirk kept: a chunk rest over 1024 bytes overruns the buffer.
DECOMP(007A5284, Iff_HandlePrfn) {
    const u32 r = arg<u32>(ctx, 0);
    // The buffer sits at 112(r1) of a 1152-byte frame: reserving 1040
    // bytes puts it at the same address (entry r1 - 1040).
    Stack buf(ctx, 1040);
    const u32 st = load<u32>(r + 0), stream = load<u32>(r + 4);
    const s32 top = static_cast<s32>(load<u32>(st + 4));
    u32 size = 0;
    if (top >= 0) {
        const u32 e = st + 8 + 12 * static_cast<u32>(top);
        if (e != 0) size = load<u32>(e + 4) + load<u32>(e + 8) - static_cast<u32>(load<u64>(stream + 264));
    }
    const u32 got = static_cast<u32>(fx::vcall(ctx, stream, 12, {stream, buf.addr(), size, 0}));
    store<u32>(r + 8, load<u32>(r + 8) + got);
    ret(ctx, 1u);
}

// InflateQueue's deleting destructor [vtable 00B24558[1]]: for a non-null
// this, InflateQueue_Destruct (007A60A0) inlined, then Mem_FreeFrom(this, 0).
DECOMP(007A6138, InflateQueue_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return ret(ctx, 0u);
    const u32 ef = load<u32>(self + 11972);
    store<u32>(self, 0x00B24558u);
    store<u8>(self + 11992, 0);
    if (ef != 0) call(ctx, g9::fn::cellSpursEventFlagDetachLv2EventQueue, {self + 10624, self, 0, ef});
    const u32 ts = load<u32>(self + 11976);
    if (ts != 0) {
        call(ctx, g9::fn::cellSpursShutdownTaskset, {ts, ts, self});
        call(ctx, g9::fn::cellSpursJoinTaskset, {load<u32>(self + 11976)});
        store<u32>(self + 11976, 0);
    }
    store<u32>(self, 0x00B2451Cu);
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0, self, 0x00B2451Cu});
}

// void (Stream* s): skips a 12-byte header (wrapping at +4 to +0), reads
// a 4-byte record {u8 enable, u8 ref, u16 func} and writes the RSX
// commands ALPHA_TEST_ENABLE (0x00040304) = enable != 0, then ALPHA_FUNC
// / ALPHA_REF (0x00080308) = func, ref inline at the gcm context's current
// pointer (no space check)  [a f_005159E8 record]
DECOMP(005150A8, Stream_SetAlphaTest_005150A8) {
    const u32 a = stream_first9(arg<u32>(ctx, 0));
    const u32 rec = load<u32>(a);  // lfs / stfs to the stack: raw
    const u32 enable = rec >> 24, ref_ = (rec >> 16) & 0xFF, func = rec & 0xFFFF;
    u32 c = load<u32>(fx::g::state);
    u32 cur = load<u32>(c + 8);
    store<u32>(cur, 0x00040304u);
    store<u32>(cur + 4, enable != 0 ? 1u : 0u);
    store<u32>(c + 8, cur + 8);
    c = load<u32>(fx::g::state);
    cur = load<u32>(c + 8);
    store<u32>(cur, 0x00080308u);
    store<u32>(cur + 4, func);
    store<u32>(cur + 8, ref_);
    store<u32>(c + 8, cur + 12);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(rec));  // the lfs stays in f1
    ctx.gpr[3] = c;
}

// void (): runs the record stream of render layer 9 (0x010DDAB4) through
// f_005159E8 with a stack state {0, 0, ..., +16 the f32[16] matrix copied
// from 0x01164C10}  [called by f_003E3428, f_0048C8A0, f_0048C948, f_0049B008]
DECOMP(005165D0, Layer9_RunStream_005165D0) {
    // The state sits at 112(r1) of a 192-byte frame: reserving 80 bytes
    // puts it at the same address (entry r1 - 80).
    Stack st(ctx, 80);
    const u32 layer = load<u32>(0x010DDAB4u);
    store<u32>(st.addr() + 0, 0);
    store<u32>(st.addr() + 4, 0);
    for (u32 k = 0; k < 16; ++k) store<u32>(st.addr() + 16 + 4 * k, load<u32>(0x01164C10u + 4 * k));  // lfs / stfs: raw
    ctx.fpr[1] = static_cast<f64>(load<f32>(0x01164C10u + 56));  // left over from the copy
    ctx.fpr[2] = static_cast<f64>(load<f32>(0x01164C10u + 60));
    call(ctx, 0x005159E8u, {layer, st.addr()});
}

// QuerySlot* (const f32 m[16], s32 a, s32 b): v = f_0051B788(the proxy
// material at 0x010DFAC0, a, b); the first free query slot (u8 +4 clear)
// is marked in use and set up with Slot_Setup(slot, v, m). Quirk kept:
// with no free slot Slot_Setup still runs on a null slot. Returns the
// slot (or 0)  [called by f_0032BCD0]
DECOMP(00527E64, QuerySlot_Acquire_00527E64) {
    const u32 m = arg<u32>(ctx, 0);
    const s64 a = static_cast<s32>(arg<u32>(ctx, 1)), b = static_cast<s32>(arg<u32>(ctx, 2));
    const u32 v = static_cast<u32>(call(ctx, 0x0051B788u, {load<u32>(0x010DFAC0u), static_cast<u64>(a), static_cast<u64>(b)}));
    u32 slot = 0;
    for (u32 i = 0; i < k9SlotCount; ++i) {
        const u32 s = k9Slots + k9SlotSize * i;
        if (load<u8>(s + 4) == 0) {
            store<u8>(s + 4, 1);
            slot = s;
            break;
        }
    }
    call(ctx, 0x005485F4u /* Slot_Setup */, {slot, v, m});
    ret(ctx, slot);
}

// void (): walks the previous frame's list (index 0x01195098 ^ 1, count
// u32 0x01195090[index]) of 112-byte entries starting at 0x01194FB0 +
// 112 * index: an entry whose +0 is 0 goes to f_005ABE5C(entry, 0), any
// other sets the u8 at 0x01196982 (name?)  [called by f_00457BFC]
DECOMP(005AC54C, FlipList_Process_005AC54C) {
    const u32 index = load<u32>(0x01195098u) ^ 1;
    const u32 count = load<u32>(0x01195090u + (index << 2));
    u32 r3 = 0x01194FB0u;
    u32 e = 0x01194FB0u + 112 * index;
    for (u32 i = 0; i < count; ++i, e += 112) {  // cmplw
        r3 = e;
        if (load<u32>(e) == 0) {
            r3 = static_cast<u32>(call(ctx, 0x005ABE5Cu, {e, 0}));
            continue;
        }
        r3 = load<u8>(0x01196982u);
        if (r3 == 0) {
            r3 = 0x01190000u;
            store<u8>(0x01196982u, 1);
        }
    }
    ctx.gpr[3] = r3;  // whatever the last step left
}

// bool SelfLight_LoadParams(ShaderType* this, Material* m, const Desc* d)
// [vtable 00B25570[1], the SelfLight type]: material params 0..3 =
// d->+88, +92, +96, +100 (raw words, each read before its
// Material_ParamSlot call). Returns 1.
DECOMP(005B4FC4, SelfLight_LoadParams) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    for (u32 k = 0; k < 4; ++k) {
        const u32 v = load<u32>(d + 88 + 4 * k);
        const u32 p = static_cast<u32>(call(ctx, 0x0050C018u /* Material_ParamSlot */, {m, k}));
        store<u32>(p, v);
    }
    ret(ctx, 1u);
}

// void (Stream* s): a record {s32 index, f32 v[4]} (the vector wrapping
// as its own payload): FProgram_SetConstant(index, v) with v copied to
// the stack  [a f_005159E8 record]
DECOMP(005155C4, Stream_SetFpConstant_005155C4) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 a = stream_first9(s);
    const s32 index = static_cast<s32>(load<u32>(a));
    const u32 v = stream_next9(s, a, 16);
    Stack buf(ctx, 16);  // 112(r1) of a 128-byte frame
    for (u32 k = 0; k < 4; ++k) store<u32>(buf.addr() + 4 * k, load<u32>(v + 4 * k));  // lfs / stfs: raw
    ctx.fpr[1] = static_cast<f64>(load<f32>(v + 12));  // the last lfs, still in f1 for the call
    call(ctx, 0x0052F270u /* FProgram_SetConstant */, {static_cast<u64>(static_cast<s64>(index)), buf.addr()});
}

// void (Stream* s): a record {u32 index, f32 v[4]}:
// cellGcmSetVertexProgramConstants(gcm context, index, 4, v) with v
// copied to the stack  [a f_005159E8 record]
DECOMP(00515688, Stream_SetVpConstant_00515688) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 a = stream_first9(s);
    const u32 index = load<u32>(a);
    const u32 v = stream_next9(s, a, 16);
    Stack buf(ctx, 16);  // 112(r1) of a 128-byte frame
    for (u32 k = 0; k < 4; ++k) store<u32>(buf.addr() + 4 * k, load<u32>(v + 4 * k));  // lfs / stfs: raw
    ctx.fpr[1] = static_cast<f64>(load<f32>(v + 12));  // the last lfs, still in f1 for the call
    call(ctx, 0x005E5CB0u /* cellGcmSetVertexProgramConstants */, {load<u32>(fx::g::state), index, 4, buf.addr()});
}

// void (Surface* s, ViewRect** a, ViewRect** b): fills in missing
// viewport / scissor rects: when *a is null it becomes the static u16
// rect at 0x010F0CA8 = {0, 0, s->+56, s->+60} (guard u8 0x010F0CA0); when
// *b is null it becomes the static at 0x010F0CB8 (zeroed on first use,
// guard u8 0x010F0CB0) holding a copy of *a  [called by f_00532450, f_005325D8, f_0053B668]
DECOMP(0051AE24, Surface_DefaultRects_0051AE24) {
    const u32 s = arg<u32>(ctx, 0), pa = arg<u32>(ctx, 1), pb = arg<u32>(ctx, 2);
    u32 a = load<u32>(pa);
    if (a == 0) {
        if (load<u8>(0x010F0CA0u) == 0) store<u8>(0x010F0CA0u, 1);
        a = 0x010F0CA8u;
        store<u16>(a + 0, 0);
        const u32 w = load<u32>(s + 56);
        store<u16>(a + 2, 0);
        const u32 h = load<u32>(s + 60);
        store<u16>(a + 4, static_cast<u16>(w));
        store<u16>(a + 6, static_cast<u16>(h));
        store<u32>(pa, a);
    }
    u32 b = load<u32>(pb);
    if (b == 0) {
        b = 0x010F0CB8u;
        if (load<u8>(0x010F0CB0u) == 0) {
            for (u32 k = 0; k < 4; ++k) store<u16>(b + 2 * k, 0);
            store<u8>(0x010F0CB0u, 1);
        }
        store<u16>(b + 0, load<u16>(a + 0));
        store<u16>(b + 2, load<u16>(a + 2));
        store<u16>(b + 4, load<u16>(a + 4));
        const u16 last = load<u16>(a + 6);
        store<u32>(pb, b);
        store<u16>(b + 6, last);
    }
    ctx.gpr[3] = b;
}

// bool Iff_HandleMatl(IffReader* r): the MATL chunk handler: a new
// material (Material_Alloc) is read with Iff_ReadChunks(r, m), with the
// material allocator's hook (*0x00F8F114: +8 OPD, +12 user) called
// before (m, user, 0) and after (m, user, 1) when set. Returns 1.
DECOMP(0051C640, Iff_HandleMatl) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 m = static_cast<u32>(call(ctx, 0x004FF2E8u /* Material_Alloc */, {r}));
    u32 al = load<u32>(0x00F8F114u);
    u32 opd = load<u32>(al + 8);
    if (opd != 0) call_ptr(ctx, opd, {m, load<u32>(al + 12), 0, opd, load<u32>(opd)});
    call(ctx, g9::fn::Iff_ReadChunks, {r, m});
    al = load<u32>(0x00F8F114u);
    opd = load<u32>(al + 8);
    if (opd != 0) call_ptr(ctx, opd, {m, load<u32>(al + 12), 1, load<u32>(opd)});
    ret(ctx, 1u);
}

// Obj24070's destructor [vtable 00B24070[0]]: vtable 0x00B24070, the
// buffer parts at +5828, +5812 and +5796 (Pool_DestructPart), then
// Obj23650_Destruct inlined: when +20 is set, vtable 0x00B23650 and its
// virtual slot 2 (release); the base vtable 0x00B234C8.
DECOMP(0079E6DC, Obj24070_Destruct) {
    const u32 self = arg<u32>(ctx, 0);
    store<u32>(self, 0x00B24070u);
    call(ctx, g9::fn::Pool_DestructPart, {self + 5828, 0x00B24070u});
    call(ctx, g9::fn::Pool_DestructPart, {self + 5812});
    call(ctx, g9::fn::Pool_DestructPart, {self + 5796});
    const u32 o = load<u32>(self + 20);
    if (o != 0) {
        store<u32>(self, 0x00B23650u);
        release9(ctx, o);
    }
    store<u32>(self, 0x00B234C8u);
    ctx.gpr[3] = 0x00B234C8u;
}

namespace {
// The 40-byte Clone (vtable slot 3) of the 0x00B23170 / 0x00B23190
// objects: SmallAlloc(40), constructed as {vtable, u16 +4 1, the rest 0},
// then +8, +12, +20, +24, u8 +16, +28, +32, +36 copied from this (raw
// words). Quirk kept: a failed SmallAlloc still copies into address 0.
void clone40(PPUContext& ctx, u32 vtable) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu /* SmallAlloc */, {40}));
    if (p != 0) {
        store<u32>(p + 8, 0);
        store<u16>(p + 4, 1);
        store<u32>(p + 12, 0);
        store<u8>(p + 16, 0);
        for (u32 off = 20; off <= 36; off += 4) store<u32>(p + off, 0);
        store<u32>(p, vtable);
    }
    for (u32 off : {8u, 12u, 20u, 24u}) store<u32>(p + off, load<u32>(self + off));
    store<u8>(p + 16, load<u8>(self + 16));
    for (u32 off : {28u, 32u, 36u}) store<u32>(p + off, load<u32>(self + off));
    ctx.fpr[1] = static_cast<f64>(load<f32>(self + 36));  // the last lfs stays in f1
    ret(ctx, p);
}
}  // namespace

// Obj* (this)  [vtable 00B23170[3]]: clone40 with vtable 0x00B23170
DECOMP(0079EDE4, Clone_0079EDE4) { clone40(ctx, 0x00B23170u); }

// Obj* (this)  [vtable 00B23190[3]]: clone40 with vtable 0x00B23190
DECOMP(0079F2D4, Clone_0079F2D4) { clone40(ctx, 0x00B23190u); }

// bool SHONightBlend_LoadParams(ShaderType*, Material* m, const Desc* d)
// [vtable 00B261B8[1], the SHO_NightBlend type]: nothing when config u8
// +152 has bit 0x80; else the loaded texture named by d->+4 (or
// Tex_GetDefault(0)) goes to f_004FF008(m, tex) and gets virtual slot 4
// (+16) (tex, u16 m->+20->+122, u16 +124, 1). Returns 1.
DECOMP(005D8B9C, SHONightBlend_LoadParams) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if ((load<u8>(0x010C5A20u + 152) & 0x80) != 0) return ret(ctx, 1u);
    u32 tex = static_cast<u32>(call(ctx, 0x0050C398u /* Tex_FindLoaded */, {load<u32>(d + 4)}));
    if (tex == 0) tex = static_cast<u32>(call(ctx, 0x004FEF30u /* Tex_GetDefault */, {0}));
    call(ctx, 0x004FF008u, {m, tex});
    const u32 opd = load<u32>(load<u32>(tex) + 16);
    const u32 src = load<u32>(m + 20);
    const u32 a = load<u16>(src + 122), b = load<u16>(src + 124);
    call_ptr(ctx, opd, {tex, a, b, 1, load<u32>(opd), a});  // r7 the entry, r8 a copy of r4 (not the vcall pair)
    ret(ctx, 1u);
}

// void (): u8 0x00F66F94 = bit 0x40 of the current surface's view u8
// +1776 (draw::g::current_surface->+672); when set, render layer 0 (the
// first of render::g::layers) gets the marker {-1, 0} at its cursor (+8,
// += 8) and, when it then holds any 8-byte records (from +4), bit 0x40 of
// its u8 +12 is set and f_0056ED88(records, count, 0x010DD400) runs
// (r6 / r7 left as the flags byte and the layer)  [called by f_00599864]
DECOMP(0051A6D4, Layer0_FlushMarker_0051A6D4) {
    const u32 view = load<u32>(load<u32>(0x010DDA74u) + 672);
    const u32 on = (load<u8>(view + 1776) & 0x40) != 0 ? 1u : 0u;
    store<u8>(0x00F66F94u, static_cast<u8>(on));
    if (on == 0) return ret(ctx, 0u);
    const u32 layer = load<u32>(0x010DDA8Cu + 4);
    store<u32>(load<u32>(layer + 8), 0xFFFFFFFFu);
    const u32 cur = load<u32>(layer + 8);
    store<u32>(cur + 4, 0);
    const u32 base = load<u32>(layer + 4);
    store<u32>(layer + 8, cur + 8);
    const u32 n = static_cast<u32>(static_cast<s32>(cur + 8 - base) / 8);  // srawi + addze
    if (n == 0) return ret(ctx, 0u);
    const u8 flags = static_cast<u8>(load<u8>(layer + 12) | 0x40);
    store<u8>(layer + 12, flags);
    call(ctx, 0x0056ED88u, {base, n, 0x010DD400u, flags, layer});  // r6: the flags byte, left over
}

// void (): once (guard u8 0x00F67C72): f_0052DB58(), then for each of
// the five fragment-program wrappers at 0x010F0AE0: u8 +1156 raised to at
// least 5 and, when its Cg program's (+4) register count is below 5,
// Cg_SetRegisterCount(program, 5)  [Shaders_Init's kOnce54CDB0 step]
DECOMP(0054CDB0, Shaders_ClampRegisterCounts) {
    u64 r3 = load<u8>(0x00F67C72u);
    if (r3 != 0) return ret(ctx, static_cast<u32>(r3));
    store<u8>(0x00F67C72u, 1);
    call(ctx, 0x0052DB58u);
    for (u32 i = 0; i < 5; ++i) {
        const u32 slot = 0x010F0AE0u + 4 * i;
        const u32 w = load<u32>(slot);
        const u32 b = load<u8>(w + 1156);
        store<u8>(w + 1156, static_cast<u8>(b > 5 ? b : 5));  // cmplwi
        r3 = call(ctx, 0x005E5F38u /* Cg_GetRegisterCount */, {load<u32>(load<u32>(slot) + 4)});
        if (static_cast<u32>(r3) < 5) r3 = call(ctx, 0x005E5F9Cu /* Cg_SetRegisterCount */, {load<u32>(load<u32>(slot) + 4), 5});
    }
    ctx.gpr[3] = r3;
}

namespace {
// The KLBT / KLBM chunk handlers: a 12-byte blob {vtable, +4 size, +8
// data} (Mem_Alloc(12, 0, 16), Obj_Construct564F10, then the handler's
// vtable and Mem_AllocPool(128, size, 4, 1) for the data) registered as
// Registry_Set(slot, blob), then the chunk read into it with
// FileReader_Read(r, data, size). Quirk kept: a failed Mem_Alloc
// registers 0 and still reads into the null blob's +8. Returns 1.
void iff_blob9(PPUContext& ctx, u32 vtable, u32 slot) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 size = static_cast<u32>(call(ctx, g8::fn::Iff_ChunkSize, {r}));
    const u32 o = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {12, 0, 16}));
    if (o != 0) {
        call(ctx, 0x00564F10u /* Obj_Construct564F10 */, {o});
        store<u32>(o, vtable);
        store<u32>(o + 8, static_cast<u32>(call(ctx, 0x0056C514u /* Mem_AllocPool */, {128, size, 4, 1})));
        store<u32>(o + 4, size);
    }
    call(ctx, 0x00564F60u /* Registry_Set */, {slot, o});
    call(ctx, g8::fn::FileReader_Read, {r, load<u32>(o + 8), size});
    ret(ctx, 1u);
}
}  // namespace

// bool Iff_HandleKLBT(IffReader* r): the KLBT chunk as a blob (vtable
// 0x00B24120) in registry slot 0 (see iff_blob9)
DECOMP(00542630, Iff_HandleKLBT) { iff_blob9(ctx, 0x00B24120u, 0); }

// bool Iff_HandleKLBM(IffReader* r): the KLBM chunk as a blob (vtable
// 0x00B24180) in registry slot 1 (see iff_blob9)
DECOMP(00548C00, Iff_HandleKLBM) { iff_blob9(ctx, 0x00B24180u, 1); }

// void (): stencil reset and clear: the RSX commands (inline at the gcm
// context's current pointer, no space check) STENCIL_TEST_ENABLE
// (0x00040328) = 0, STENCIL_FUNC / REF / MASK (0x000C0330) = the cached
// state +160, +164, 0 and STENCIL_MASK (0x0004032C) = 0, with the cached
// state's +172 and +168 cleared along the way; then Draw_Clear(1, 0,
// depth 1.0 (k at 0x005AC1A0), -, 0, 255)  [called through a pointer]
DECOMP(005AC1A8, Stencil_ResetAndClear_005AC1A8) {
    const u32 g = fx::g::state;
    u32 c = load<u32>(g), cur = load<u32>(c + 8);
    store<u32>(cur, 0x00040328u);
    store<u32>(cur + 4, 0);
    store<u32>(c + 8, cur + 8);
    store<u32>(g + 172, 0);
    c = load<u32>(g);
    const u32 func = load<u32>(g + 160), ref_ = load<u32>(g + 164);
    cur = load<u32>(c + 8);
    store<u32>(cur, 0x000C0330u);
    store<u32>(cur + 4, func);
    store<u32>(cur + 8, ref_);
    store<u32>(cur + 12, 0);
    store<u32>(c + 8, cur + 16);
    c = load<u32>(g);
    store<u32>(g + 168, 0);
    cur = load<u32>(c + 8);
    store<u32>(cur, 0x0004032Cu);
    store<u32>(cur + 4, 0);
    store<u32>(c + 8, cur + 8);
    callf(ctx, 0x00531F80u /* Draw_Clear */, {1, 0, cur, 0, 255}, {static_cast<f64>(load<f32>(0x005AC1A0u))});
}

// void (this, Mesh* m)  [vtable 00B23D28[5]]: binds the vertex program
// 0x010F0AD0[skinned] (skinned: m and its +112 and +84 all set) and the
// fragment program at 0x010F0AD8 through the cached state (0x010F10A0):
// a new vertex program sets +20 and +8 = -1, and when its +4 differs from
// +24, +24 = it and f_005E3C00(gcm context, program->+20); a new fragment
// program sets +28 and bit 0x80000000 of +92.
DECOMP(0052D938, Obj23D28_BindPrograms_0052D938) {
    const u32 m = arg<u32>(ctx, 1);
    const u32 skinned = (m != 0 && load<u32>(m + 112) != 0 && load<u32>(m + 84) != 0) ? 1u : 0u;
    const u32 g = fx::g::state;
    const u32 vp = load<u32>(0x010F0AD0u + 4 * skinned);
    if (load<u32>(g + 20) != vp) {
        store<u32>(g + 20, vp);
        store<u32>(g + 8, 0xFFFFFFFFu);
        const u32 id = load<u32>(vp + 4);
        if (load<u32>(g + 24) != id) {
            const u32 c = load<u32>(g);
            store<u32>(g + 24, id);
            call(ctx, 0x005E3C00u, {c, load<u32>(vp + 20)});
        }
    }
    const u32 fp = load<u32>(0x010F0AD8u);
    if (load<u32>(g + 28) != fp) {
        const u32 flags = load<u32>(g + 92);
        store<u32>(g + 28, fp);
        store<u32>(g + 92, flags | 0x80000000u);
        ctx.gpr[3] = flags | 0x80000000u;
    } else {
        ctx.gpr[3] = fp;
    }
}

// void (f32 out[4], const f32 c[16]; f1 near, f2 far): the bounding
// sphere of a view-frustum slice, c's rows 0/1 and 2/3 being the corner
// pairs across the near and far planes: with L = far - near, A = |c0 -
// c1|^2 and B = |c2 - c3|^2, t = ((B - A) / (4 L) + L) * 0.5 (k at
// 0x0052B200: 4.0, 0.5, 0.25); when t > L the centre is at far with
// radius sqrt(B) * 0.5, else at near + t with radius sqrt(A * 0.25 + t^2).
// out = {0, 0, -centre, radius}; f1 = -centre (name?)  [called by f_0052B688]
DECOMP(0052B210, Frustum_SliceSphere_0052B210) {
    const u32 out = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const f64 near = farg(ctx, 0), far = farg(ctx, 1);
    auto s = [](f64 x) { return static_cast<f64>(static_cast<f32>(x)); };  // single rounding
    auto m = [c](u32 k) { return static_cast<f64>(load<f32>(c + 4 * k)); };
    const f64 k4 = load<f32>(0x0052B200u), kHalf = load<f32>(0x0052B204u), kQuarter = load<f32>(0x0052B208u);
    const f64 len = s(far - near);
    const f64 d1 = s(m(1) - m(5)), d0 = s(m(0) - m(4)), e1 = s(m(9) - m(13)), e0 = s(m(8) - m(12));
    const f64 d2 = s(m(2) - m(6)), e2 = s(m(10) - m(14));
    const f64 a1 = s(std::fma(d0, d0, s(d1 * d1)));
    const f64 b1 = s(std::fma(e0, e0, s(e1 * e1)));
    const f64 a = s(std::fma(d2, d2, a1));  // fmadds
    const f64 den = s(len * k4);
    const f64 b = s(std::fma(e2, e2, b1));
    const f64 t = s(s(len + s(s(b - a) / den)) * kHalf);
    f64 centre, radius;
    if (t > len) {  // fcmpu: false for NaN
        centre = far;  // fmr: the unrounded f2
        radius = s(s(std::sqrt(b)) * kHalf);
    } else {
        centre = s(t + near);
        radius = s(std::sqrt(s(std::fma(a, kQuarter, s(t * t)))));
    }
    store<u32>(out + 0, 0);
    store<u32>(out + 4, 0);
    store<f32>(out + 12, static_cast<f32>(radius));
    store<f32>(out + 8, static_cast<f32>(-centre));
    ctx.fpr[1] = -centre;
    ctx.fpr[3] = radius;
}

// void (Obj23D50* o)  [called by Update_0054A420]: when u8 o->+20 and
// o->+28 are set, the current surface's view (draw::g::current_surface
// ->+672) exists with bit 0 of its +256, and u8 o->+96 has bit 0x80: on
// bit 0x40 of +96 first Draw_Clear(2, 0, depth 1.0 (k at 0x0052CC80), -,
// 0, 255) and f_0052C6B8(o); then o's virtual slot 2 (+8) and
// Draw_InvalidateState()
DECOMP(0052CC88, Obj23D50_Resolve_0052CC88) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r3 = load<u8>(o + 20);
    if (r3 == 0) return ret(ctx, r3);
    if ((r3 = load<u32>(o + 28)) == 0) return ret(ctx, r3);
    const u32 view = load<u32>(load<u32>(0x010DDA74u) + 672);
    if ((r3 = view) == 0) return ret(ctx, r3);
    if ((r3 = load<u32>(view + 256) & 1) == 0) return ret(ctx, r3);
    const u32 flags = load<u8>(o + 96);
    if ((flags & 0x80) == 0) return ret(ctx, flags);
    if ((flags & 0x40) != 0) {
        callf(ctx, 0x00531F80u /* Draw_Clear */, {2, 0, 0x0052CC80u, 0, 255}, {static_cast<f64>(load<f32>(0x0052CC80u))});
        call(ctx, 0x0052C6B8u, {o});
    }
    fx::vcall(ctx, o, 8, {o});
    call(ctx, g9::fn::Draw_InvalidateState);
}

// void (Obj* o, u32 i)  [vtable entry at 0x00B24014]: Draw_SetTarget(the
// target o->+136[i], 0), then on the gcm context: invalidate the texture
// cache (1), clear colour 0, clear depth/stencil 0xFFFFFF00, stencil mask
// 255, clear surface 0xF0 (RGBA), stencil mask back to the cached state's
// +168; then virtual slot 4 (+16) on render layer 1 (render::g::layers)
DECOMP(00542C50, Target_ClearColour_00542C50) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 g = fx::g::state;
    call(ctx, 0x005373A8u /* Draw_SetTarget */, {load<u32>(o + 136 + (i << 2)), 0});
    call(ctx, 0x005E3E64u /* cellGcmSetInvalidateTextureCache */, {load<u32>(g), 1});
    call(ctx, 0x005E3CB4u /* cellGcmSetClearColor */, {load<u32>(g), 0});
    call(ctx, 0x005E3CD8u /* cellGcmSetClearDepthStencil */, {load<u32>(g), 0xFFFFFF00u});
    call(ctx, 0x005E3D8Cu /* cellGcmSetStencilMask */, {load<u32>(g), 255});
    call(ctx, 0x005E3C74u /* cellGcmSetClearSurface */, {load<u32>(g), 0xF0});
    call(ctx, 0x005E3D8Cu /* cellGcmSetStencilMask */, {load<u32>(g), load<u32>(g + 168)});
    const u32 layer = load<u32>(0x010DDA8Cu + 8);
    fx::vcall(ctx, layer, 16, {layer});
}

// bool Iff_HandleNTXD(IffReader*, Texture** pt): the NTXD chunk: *pt
// takes the format of the group-4 texture (shadergroup.h group_textures
// at 0x010DDADC): +72..+92 and +56 / +60 copied, the low nibble of +64
// kept with bits 4..31 and then 1..3 from the source and bit 0 set, +96 =
// 0, and the top nibble of +68 from the source (guard u8 0x010F0DC0 set
// on first use). Returns 1.
DECOMP(0051D83C, Iff_HandleNTXD) {
    if (load<u8>(0x010F0DC0u) == 0) store<u8>(0x010F0DC0u, 1);
    copy_format9(load<u32>(arg<u32>(ctx, 1)), load<u32>(0x010DDADCu));
    ret(ctx, 1u);
}

// bool Iff_HandleVTUO(IffReader* r, Obj* o): the VTUO chunk read into
// o->+4 (Iff_ChunkSize bytes); when the reader's flags (u8 r->+12) have
// bit 2 (byte swap), the u16 o->+0 records of three u16 each are
// byte-swapped in place. Returns 1.
DECOMP(00524108, Iff_HandleVTUO) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 size = static_cast<u32>(call(ctx, g8::fn::Iff_ChunkSize, {r}));
    u32 p = load<u32>(o + 4);
    call(ctx, g8::fn::FileReader_Read, {r, p, size});
    if ((load<u8>(r + 12) & 2) == 0) return ret(ctx, 1u);
    s32 n = load<u16>(o);
    while (n > 0) {
        for (u32 k = 0; k < 3; ++k) {
            const u32 q = p + 2 * k;
            store<u16>(q, static_cast<u16>((load<u8>(q + 1) << 8) | load<u8>(q)));
        }
        p += 6;
        --n;
    }
    ret(ctx, 1u);
}

// void Matrix_MulAffine_0079DE98(f32 out[16], const f32 a[16], const f32
// b[16]): out = a * b with VMX (lvx / stvx, addresses rounded down to 16):
// row i = b0 * a[i].x + b1 * a[i].y + b2 * a[i].z, fused per lane in
// vmaddfp order (from 0, row 3 from b3); a's w column is not read
// [called by f_005159E8, f_00574370]
DECOMP(0079DE98, Matrix_MulAffine_0079DE98) {
    const u32 out = arg<u32>(ctx, 0) & ~15u, a = arg<u32>(ctx, 1) & ~15u, b = arg<u32>(ctx, 2) & ~15u;
    f32 r[4][4];
    for (u32 i = 0; i < 4; ++i) {
        const f32 x = load<f32>(a + 16 * i), y = load<f32>(a + 16 * i + 4), z = load<f32>(a + 16 * i + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(b + 4 * k), x, i == 3 ? load<f32>(b + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(b + 16 + 4 * k), y, v);
            r[i][k] = std::fmaf(load<f32>(b + 32 + 4 * k), z, v);
        }
    }
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) store<f32>(out + 16 * i + 4 * k, r[i][k]);
    const u32 o3 = arg<u32>(ctx, 0), b3 = arg<u32>(ctx, 2);
    ctx.gpr[4] = o3 + 16;  // left as the original leaves them
    ctx.gpr[5] = o3 + 32;
    ctx.gpr[6] = o3 + 48;
    ctx.gpr[7] = b3 + 16;
    ctx.gpr[8] = b3 + 48;
}

// void (FrameBuffer* fb, u32 value, u32 slot): when fb->+84[slot]
// differs from value: +84[slot] = value, +80 = -1, and a type-8 record
// {header (8 << 24 | the slot address' bits 16..23), the address' low
// 16 bits, 0; value; slot} is written to fb's record stream (start +0,
// end +4, cursor +16; each piece wraps to the start when it would pass
// the end, 4-aligned). Returns fb  [called by f_002ECE10]
DECOMP(005233F8, FrameBuffer_SetSlot_005233F8) {
    const u32 fb = arg<u32>(ctx, 0), value = arg<u32>(ctx, 1), slot = arg<u32>(ctx, 2);
    const u32 p = fb + 84 + (slot << 2);
    // f_002ECE10 calls Draw_BeginPrims right after this, which reads the
    // r7 left here (its low 16 bits go into a command word): r7 and the
    // other volatiles are set as the original leaves them.
    ctx.gpr[6] = value;
    ctx.gpr[7] = p;
    if (value == load<u32>(p)) {  // cmpw; r3 stays fb
        ctx.gpr[8] = value;
        return;
    }
    store<u32>(p, value);
    store<u32>(fb + 80, 0xFFFFFFFFu);
    const u32 w0 = (p & 0x00FF0000u) | 0x08000000u, w1 = p & 0xFFFFu;  // rlwimi packing
    u32 cur = load<u32>(fb + 16);
    const u32 end0 = load<u32>(fb + 4);
    if (cur + 15 > end0) cur = load<u32>(fb);  // cmplw
    const u32 h = (cur + 3) & ~3u;
    store<u32>(fb + 16, h);
    store<u32>(h + 8, 0);
    store<u32>(h + 0, w0);
    store<u32>(h + 4, w1);
    u32 c = load<u32>(fb + 16);
    const u32 q = ((c + 19 > load<u32>(fb + 4) ? load<u32>(fb) : c + 12) + 3) & ~3u;
    ctx.gpr[7] = c + 19;
    store<u32>(fb + 16, q);
    store<u32>(q, value);
    c = load<u32>(fb + 16);
    const u32 q2in = c + 11 > load<u32>(fb + 4) ? load<u32>(fb) : c + 4;
    const u32 q2 = (q2in + 3) & ~3u;
    store<u32>(fb + 16, q2);
    store<u32>(q2, slot);
    store<u32>(fb + 16, load<u32>(fb + 16) + 4);
    ctx.gpr[4] = q2 + 4;
    ctx.gpr[5] = slot;
    ctx.gpr[6] = c + 11;
    ctx.gpr[8] = q2in;
    ctx.gpr[9] = 0;
    ctx.gpr[10] = end0;
}

// void (): query-pass state written inline at the gcm context's current
// pointer (no space check): methods 0x0310 = 0, 0x0A70 = 0 and 0x183C
// (cull face enable) = 0, cellGcmSetVertexProgramConstants(ctx, 4, 4,
// the f32[4] at 0x01164BB0), then 0x0324 (colour mask) = 0
// [called by Call_005488AC]
DECOMP(00527C4C, Query_SetupState_00527C4C) {
    const u32 g = fx::g::state;
    auto put = [g](u32 method, u32 v) {
        const u32 c = load<u32>(g), cur = load<u32>(c + 8);
        store<u32>(cur, method);
        store<u32>(cur + 4, v);
        store<u32>(c + 8, cur + 8);
        return cur;
    };
    put(0x00040310u, 0);
    put(0x00040A70u, 0);  // (0 != 0) != 0, as the original computes it
    put(0x0004183Cu, 0);
    call(ctx, 0x005E5CB0u /* cellGcmSetVertexProgramConstants */, {load<u32>(g), 4, 4, 0x01164BB0u});
    const u32 cur = put(0x00040324u, 0);
    ctx.gpr[3] = load<u32>(g);  // left as the original leaves them
    ctx.gpr[4] = 0x00040324u;
    ctx.gpr[5] = cur;
    ctx.gpr[6] = cur + 8;
}

// void (CallbackList* l, OPD* fn, u32 user, u32 id, s8 prio): inserts
// {fn, user, id, prio} into l's entries (16 bytes each, u8 count +64)
// sorted by prio: entries from the end with a higher prio move up one
// (memmove of 13 bytes). Returns the new entry  [called by f_0028BDB0]
DECOMP(005131E8, CallbackList_Insert_005131E8) {
    const u32 l = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const s8 prio = static_cast<s8>(arg<u32>(ctx, 4));
    s8 k = static_cast<s8>(load<u8>(l + 64) - 1);
    while (k >= 0) {
        const u32 e = l + 16 * static_cast<u32>(static_cast<s32>(k));
        if (static_cast<s8>(load<u8>(e + 12)) <= prio) break;
        call(ctx, 0x0071C3F0u /* memmove */, {e + 16, e, 13});
        k = static_cast<s8>(k - 1);
    }
    const u32 slot = l + 16 * static_cast<u32>(static_cast<s32>(k)) + 16;
    store<u8>(l + 64, static_cast<u8>(load<u8>(l + 64) + 1));
    store<u32>(slot + 0, a);
    store<u32>(slot + 4, b);
    store<u32>(slot + 8, c);
    store<u8>(slot + 12, static_cast<u8>(prio));
    ret(ctx, slot);
}

// Texture* DepthTarget_Create(u16 w, u16 h, u32 format): the depth twin
// of ColorTarget_Create51E33C: a texture (Texture_Alloc(w, h, format),
// unchecked) named "Z-RenderTarget" (+16 char[32], +47 = 0, +48 hash),
// +52 / +54 width / height, its +56 surface made by
// Surface_InitTarget_00537040(t + 56, w, h, 1, format, &target) and
// +108 = Target_Bind51B4C4(0, target, t)  [called by f_005868D0]
DECOMP(0051E428, DepthTarget_Create) {
    const u32 w = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), format = arg<u32>(ctx, 2);
    const u32 t = static_cast<u32>(call(ctx, 0x0051D368u /* Texture_Alloc */, {w, h, format}));
    call(ctx, 0x00553638u /* String_CopyN */, {t + 16, 0x00AF1B44u /* "Z-RenderTarget" */, 32});
    store<u8>(t + 47, 0);
    store<u32>(t + 48, static_cast<u32>(call(ctx, 0x0056A710u /* String_HashNoCase */, {t + 16})));
    store<u16>(t + 52, static_cast<u16>(w));
    store<u16>(t + 54, static_cast<u16>(h));
    Stack out(ctx, 16);  // the target at 112(r1)
    store<u32>(out.addr(), 0);
    call(ctx, 0x00537040u /* Surface_InitTarget_00537040 */, {t + 56, w, h, 1, format, out.addr()});
    store<u32>(t + 108, static_cast<u32>(call(ctx, 0x0051B4C4u /* Target_Bind51B4C4 */, {0, load<u32>(out.addr()), t})));
    ret(ctx, t);
}

// bool Iff_HandleLAPM(IffReader* r, Obj* o): the LAPM chunk of u16
// entries: o->+84 = a zeroed table (Mem_Alloc((size & ~1) + 2, 0, 16),
// unchecked) {u8 count = size / 2, u8 v[]}; each entry is read with
// Iff_ReadU16 and kept as v[i] when <= 255, else the count drops by one
// (v[i] stays 0). Returns 1.
DECOMP(00525BE8, Iff_HandleLAPM) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 size = static_cast<u32>(call(ctx, g8::fn::Iff_ChunkSize, {r}));
    const u32 n = size >> 1, bytes = (size & ~1u) + 2;
    const u32 mem = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {bytes, 0, 16}));
    const u32 tab = static_cast<u32>(call(ctx, sly4::fn::memset, {mem, 0, bytes}));
    store<u8>(tab, static_cast<u8>(n));
    store<u32>(o + 84, tab);
    // The u16 sits at 112(r1) of a 160-byte frame: reserving 48 bytes
    // puts it at the same address (entry r1 - 48).
    Stack v(ctx, 48);
    for (u32 i = 0; i < n; ++i) {  // cmplw
        call(ctx, 0x00564340u /* Iff_ReadU16 */, {r, v.addr(), 0});
        const u32 x = load<u16>(v.addr());
        if (static_cast<s32>(x) > 255)
            store<u8>(tab, static_cast<u8>(load<u8>(tab) - 1));
        else
            store<u8>(tab + 1 + i, static_cast<u8>(x));
    }
    ret(ctx, 1u);
}

// bool Iff_HandleXTVM(IffReader* r, Mesh* m): the XTVM chunk into the
// mesh's next 16-byte buffer part (u8 m->+109 counts them, parts from
// +24): Buffer_Alloc52F198(part, size, 128, 0, 2), bits 1..3 of the
// part's +8 = 2 (0x4), the chunk read into part->+0; f_0052528C(m) when
// the reader byte-swaps (u8 r->+12 bit 2). Returns 1.
DECOMP(00525818, Iff_HandleXTVM) {
    const u32 r = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 size = static_cast<u32>(call(ctx, g8::fn::Iff_ChunkSize, {r}));
    const u32 k = load<u8>(m + 109);
    store<u8>(m + 109, static_cast<u8>(k + 1));
    const u32 part = m + 24 + (k << 4);
    call(ctx, 0x0052F198u /* Buffer_Alloc52F198 */, {part, size, 128, 0, 2});
    store<u32>(part + 8, (load<u32>(part + 8) & ~0xEu) | 0x4u);
    call(ctx, g8::fn::FileReader_Read, {r, load<u32>(part), size});
    if ((load<u8>(r + 12) & 2) != 0) call(ctx, 0x0052528Cu, {m});
    ret(ctx, 1u);
}

// void (Obj* x): when u8 x->+4->+69 is set, stencil state written inline
// at the gcm context's current pointer (no space check): STENCIL_TEST_
// ENABLE (0x00040328) = 1, STENCIL_OP fail / zfail / zpass (0x000C033C)
// = 0x1E00 (keep) each, STENCIL_MASK (0x0004032C) = 255 and STENCIL_FUNC
// / REF / MASK (0x000C0330) = 0x202 (equal), 32, 32, also kept in the
// cached state (+160 func, +164 ref, +168 mask, +172 func mask, +176..
// +184 ops)  [called by f_00501820, f_005073F8, f_00507B18]
DECOMP(005011A0, Stencil_SetEqual32_005011A0) {
    const u32 x = arg<u32>(ctx, 0);
    if (load<u8>(load<u32>(x + 4) + 69) == 0) return ret(ctx, 0u);
    const u32 g = fx::g::state;
    u32 c = load<u32>(g), cur = load<u32>(c + 8);
    store<u32>(cur, 0x00040328u);
    store<u32>(cur + 4, 1);
    store<u32>(c + 8, cur + 8);
    store<u32>(g + 176, 0x1E00);
    store<u32>(g + 180, 0x1E00);
    store<u32>(g + 184, 0x1E00);
    store<u32>(g + 160, 0x202);
    store<u32>(g + 164, 32);
    store<u32>(g + 172, 32);
    store<u32>(g + 168, 255);
    c = load<u32>(g);
    cur = load<u32>(c + 8);
    store<u32>(cur, 0x000C033Cu);
    for (u32 k = 1; k <= 3; ++k) store<u32>(cur + 4 * k, 0x1E00);
    store<u32>(c + 8, cur + 16);
    c = load<u32>(g);
    cur = load<u32>(c + 8);
    store<u32>(cur, 0x0004032Cu);
    store<u32>(c + 8, cur + 8);
    c = load<u32>(g);
    store<u32>(cur + 4, 255);  // written after the cursor moved on, as the original does
    const u32 func = load<u32>(g + 160), ref_ = load<u32>(g + 164), mask = load<u32>(g + 172);
    cur = load<u32>(c + 8);
    store<u32>(cur, 0x000C0330u);
    store<u32>(cur + 4, func);
    store<u32>(cur + 8, ref_);
    store<u32>(cur + 12, mask);
    store<u32>(c + 8, cur + 16);
    ctx.gpr[3] = c;
}

// void (Obj* o)  [called by f_00544870]: o->+152 / +144 = the u32 pairs
// at 0x01164068 / 0x0116406C when the mem_stats mode (+184) is 2, else
// 0x01164060 / 0x01164064; then, when config u8 +34 is set, mode 2
// restores the saved view values (+48..+56, +64..+72, +32 from +176..
// +208, bit 0x40 of u8 +88 from u8 +164) and any other mode saves them
// (raw words)
DECOMP(00543AAC, View_SaveRestore_00543AAC) {
    const u32 o = arg<u32>(ctx, 0);
    const bool two = load<u32>(0x010F0E20u + 184) == 2;
    store<u32>(o + 152, load<u32>(two ? 0x01164068u : 0x01164060u));
    store<u32>(o + 144, load<u32>(two ? 0x0116406Cu : 0x01164064u));
    if (load<u8>(0x010C5A20u + 34) == 0) return;
    if (two) {
        const u8 b = static_cast<u8>((load<u8>(o + 88) & ~0x40u) | ((load<u8>(o + 164) << 6) & 0x40u));
        store<u32>(o + 48, load<u32>(o + 176));
        store<u8>(o + 88, b);
        for (u32 off : {52u, 56u, 64u, 68u, 72u}) store<u32>(o + off, load<u32>(o + off + 128));
        store<u32>(o + 32, load<u32>(o + 208));
        ctx.fpr[1] = static_cast<f64>(load<f32>(o + 208));
        ctx.fpr[2] = static_cast<f64>(load<f32>(o + 200));
    } else {
        store<u32>(o + 176, load<u32>(o + 48));
        store<u32>(o + 180, load<u32>(o + 52));
        store<u8>(o + 164, (load<u8>(o + 88) & 0x40) != 0 ? 1 : 0);
        for (u32 off : {56u, 64u, 68u, 72u}) store<u32>(o + off + 128, load<u32>(o + off));
        store<u32>(o + 208, load<u32>(o + 32));
        ctx.fpr[1] = static_cast<f64>(load<f32>(o + 32));
        ctx.fpr[2] = static_cast<f64>(load<f32>(o + 72));
    }
}

// bool (Texture* t): t takes the default texture's format (Tex_GetDefault(0),
// fetched twice; see copy_format9); 0 when there is no default texture
// [called by f_0051D718, f_005399CC, Texture_LoadFile_0053A8B0]
DECOMP(005398CC, Texture_CopyDefaultFormat_005398CC) {
    const u32 t = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x004FEF30u /* Tex_GetDefault */, {0})) == 0) return ret(ctx, 0u);
    const u32 src = static_cast<u32>(call(ctx, 0x004FEF30u /* Tex_GetDefault */, {0}));
    copy_format9(t, src);
    ret(ctx, 1u);
}

// void ()  [called by Graphics_Init]: f_0051DF3C(); the two textures at
// 0x00F67500 / 0x00F67504, when not made yet, = f_0051E514(mem_stats +84
// / +100, 0) when those are set; then Iff_RegisterChunk for 'FTGT' (OPD
// 0x00F3BD10, flags 24), 'NTXD' (0x00F3BD18, 8) and 'KLBT' (0x00F3BDB0,
// Iff_HandleKLBT, 8)
DECOMP(00542704, Graphics_RegisterChunks_00542704) {
    call(ctx, 0x0051DF3Cu);
    for (u32 k = 0; k < 2; ++k) {
        const u32 slot = 0x00F67500u + 4 * k;
        if (load<u32>(slot) != 0) continue;
        const u32 v = load<u32>(0x010F0E20u + (k == 0 ? 84 : 100));
        if (v == 0) continue;
        store<u32>(slot, static_cast<u32>(call(ctx, 0x0051E514u, {v, 0})));
    }
    call(ctx, 0x0055902Cu /* Iff_RegisterChunk */, {0x46544754u /* FTGT */, 0x00F3BD10u, 0, 0, 24});
    call(ctx, 0x0055902Cu /* Iff_RegisterChunk */, {0x4E545844u /* NTXD */, 0x00F3BD18u, 0, 0, 8});
    call(ctx, 0x0055902Cu /* Iff_RegisterChunk */, {0x4B4C4254u /* KLBT */, 0x00F3BDB0u, 0, 0, 8});
}
