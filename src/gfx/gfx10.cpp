// The renderer range's functions of ~60 to ~90 instructions: a Texture
// destructor, the surface handle release, a texture from memory, the
// FluidSim shader type, the mesh bounds chunk, render job start/stop, the
// shadow pass prologue and a command-buffer call slot.
// Names carry the address where the role is unclear.
#include <atomic>
#include <algorithm>
#include <cmath>

#include "opentit/recomp.h"

#include "fx.h"
#include "gfx10.h"

using namespace sly4;

namespace {

namespace g8 = sly4::render::gfx8;
namespace g9 = sly4::render::gfx9;
namespace g10 = sly4::render::gfx10;

constexpr u32 k10GcmState = 0x010F10A0;  // fx::g::state: +0 the libgcm context
constexpr u32 k10TileSlots = 0x00F67244;  // u16 bitmask of the tile regions in use (tgt::g::tile_slots)

// The 16-byte buffer part {+0 p, +4 bit 0: pool 5, +8 bit 0: not owned,
// +12 tag in bits 28..31} freed as Pool_DestructPart (0052EFE0) does,
// inlined: Mem_FreePool(p, pool, tag) unless p is null or not owned; +0
// cleared after a free.
void free_part10(PPUContext& ctx, u32 part) {
    const u32 p = load<u32>(part);
    if (p == 0 || (load<u32>(part + 8) & 1) != 0) return;
    const u32 pool = (load<u32>(part + 4) & 1) != 0 ? 5u : 4u;
    call(ctx, g8::fn::Mem_FreePool, {p, pool, load<u32>(part + 12) >> 28});
    store<u32>(part, 0);
}

// The words needed to pad the libgcm context's current pointer (+8) to
// the next 128-byte boundary.
u32 pad_words10(u32 gcm) {
    const u32 w = load<u32>(gcm + 8) >> 2;
    return ((w + 31) & ~31u) - w;
}

// Render jobs (0x01163F80): +1 u8 started, +4 the Edge job context,
// +52 u32 buffer index (0/1), +56 Mem_FreePool'd buffer, +60 u32 stamp,
// +64 u32 the stamp last kicked, +71 u8 kicked, +72 cursor,
// +88 u32 buffers[2], +104 / +108 / +112 u32 job arguments.
constexpr u32 k10Jobs = 0x01163F80;
constexpr u32 k10JobPart = 0x01163E80;  // 16-byte buffer part freed at shutdown
constexpr u32 k10JobChain = 0x010F1400;  // CellSpursJobChain

}  // namespace

// void TextureBase_Delete_0050A4E8(Texture* t) [vtable 00B23380[1]]: for a non-null
// t, the TextureBase destructor body (Texture_DestructBody: vtable, +108
// released, Pool_DestructPart(+56) twice around the +96 free unless u8
// +104, ApeRoot_Destruct), then Mem_FreeFrom(t, 0).
DECOMP(0050A4E8, TextureBase_Delete_0050A4E8) {
    const u32 t = arg<u32>(ctx, 0);
    if (t == 0) return;
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
            call(ctx, g10::fn::Part_Release_0052F06C, {p});
            call(ctx, g8::fn::Mem_FreeFrom, {p, 0});
            store<u32>(t + 96, 0);
        }
    }
    call(ctx, g9::fn::Pool_DestructPart, {t + 56});  // the same part again
    call(ctx, g9::fn::ApeRoot_Destruct, {t});
    call(ctx, g8::fn::Mem_FreeFrom, {t, 0});
}

// void (Handle* h): unless u8 h->+56, the tile region s8 h->+45 (-1 =
// none) is cleared from the in-use mask (slw: regions 32..254 clear
// nothing); then the buffer part at +0 is freed (free_part10).
DECOMP(0052F06C, Part_Release_0052F06C) {
    const u32 h = arg<u32>(ctx, 0);
    if (load<u8>(h + 56) == 0) {
        const u32 region = load<u8>(h + 45);
        if (region != 0xFF) {
            const u32 bit = (region & 0x20) != 0 ? 0u : 1u << (region & 31);
            store<u16>(k10TileSlots, static_cast<u16>(load<u16>(k10TileSlots) & ~bit));
        }
    }
    free_part10(ctx, h);
    // r3 as left: the part's pointer when it is not owned, else 0.
    const u32 p = load<u32>(h);
    ctx.gpr[3] = p != 0 && (load<u32>(h + 8) & 1) != 0 ? p : 0u;
}

// Texture* Texture_LoadFromMemory(data, size, kind, const char* name):
// 0 for null data; else a new texture (Texture_Alloc, not checked) named
// name (+16, 32 bytes, +47 = 0, +48 its String_HashNoCase) and
// Texture_Init(tex, data, size, kind, name); a failed init releases it
// (virtual slot 2) and returns 0.
DECOMP(0051D91C, Texture_LoadFromMemory) {
    const u32 data = arg<u32>(ctx, 0), size = arg<u32>(ctx, 1), kind = arg<u32>(ctx, 2), name = arg<u32>(ctx, 3);
    if (data == 0) return ret(ctx, 0u);
    const u32 tex = static_cast<u32>(call(ctx, g10::fn::Texture_Alloc, {data, size, kind, name}));
    call(ctx, g10::fn::String_CopyN, {tex + 16, name, 32});
    store<u8>(tex + 47, 0);
    store<u32>(tex + 48, static_cast<u32>(call(ctx, sly4::fn::String_HashNoCase, {tex + 16})));
    if (call(ctx, g10::fn::Texture_Init, {tex, data, size, kind, name}) == 0) {
        fx::vcall(ctx, tex, 8, {tex});
        return ret(ctx, 0u);
    }
    ret(ctx, tex);
}

// void FluidSimType_Create(ShaderType* t) [ShaderType slot 0]: one
// Shader (vtable 0x00B253D0) with the programs "FluidSim" / "FluidSim"
// and vertex parameters 0x00F88D1C, appended to t's shader list (a failed
// allocation as a null entry).
DECOMP(005B19D4, FluidSimType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    u32 s = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {44, 0, 16}));
    if (s != 0) {
        call(ctx, 0x0050AFDCu /* Shader_Construct */, {s});
        store<u8>(s + 12, 0);
        store<u32>(s, 0x00B253D0u);
        call(ctx, 0x0053A9D8u /* Shader_Load */, {s, 0x00AF827Cu, 0x00AF827Cu, 0x00F88D1Cu, 0});  // "FluidSim"
    }
    // The type's PtrVector at +12: +16 u16 capacity, +20 u16 count, +22 u16 high-water mark.
    if (load<u16>(t + 20) == load<u16>(t + 16))
        call(ctx, 0x00551718u /* PtrVector_Reserve */, {t + 12, (static_cast<u32>(load<u16>(t + 16)) << 1) & 0x1FFFEu});
    store<u32>(load<u32>(t + 12) + ((static_cast<u32>(load<u16>(t + 20)) << 2) & 0x3FFFCu), s);
    const u32 n = load<u16>(t + 20) + 1u;
    const u32 high = load<u16>(t + 22);
    store<u16>(t + 20, static_cast<u16>(n));
    store<u16>(t + 22, static_cast<u16>(high > (n & 0xFFFF) ? high : n));
    ctx.gpr[3] = high;  // r3 as left: the old high-water mark
}

// bool (IffReader* r, Mesh* m): the mesh bounds chunk: u8 "ver" (1 if
// absent), vec3 "center", f32 "radius", u32 "hash" and, from version 1,
// u8 "vertexCompressionMode" (read and dropped); m->+64..+72 = center,
// +76 = radius, +80 = hash. Returns 1.
DECOMP(0052609C, Iff_HandleMeshBounds_0052609C) {
    const u32 r = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    Stack frame(ctx, 32);
    const u32 b = frame.addr();  // +0 centre[3], +12 radius, +16 hash, +20 compression, +24 u8 version
    for (u32 k = 0; k < 24; k += 4) store<u32>(b + k, 0);
    store<u8>(b + 24, 1);
    call(ctx, g10::fn::Iff_ReadU8, {r, b + 24, 0x00B23A03u});                 // "ver"
    call(ctx, g10::fn::FileReader_ReadVec3Swapped, {r, b, 0x00B23A07u});      // "center"
    call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, b + 12, 0x00B23A0Eu});  // "radius"
    call(ctx, g10::fn::Iff_ReadU32, {r, b + 16, 0x00B23A15u});                // "hash"
    if (load<u8>(b + 24) >= 1) call(ctx, g10::fn::Iff_ReadU8, {r, b + 20, 0x00B23A1Au});  // "vertexCompressionMode"
    for (u32 k = 0; k < 16; k += 4) store<u32>(m + 64 + k, load<u32>(b + k));  // lfs/stfs pairs: raw words
    store<u32>(m + 80, load<u32>(b + 16));
    ctx.fpr[1] = static_cast<f64>(load<f32>(b + 12));  // f1 as left: the radius
    ret(ctx, 1u);
}

// s32 RenderJobs_Shutdown_0053B558(): the Edge job context (+4) shut down
// (f_0060E4B0), the cursor (+72) reset, the buffer (+56) freed to pool 6,
// the SPURS job chain 0x010F1400 shut down and joined, the buffer part
// 0x01163E80 freed. Returns 1.
DECOMP(0053B558, RenderJobs_Shutdown_0053B558) {
    call(ctx, g10::fn::Edge_0060E4B0, {k10Jobs + 4});
    call(ctx, g10::fn::Cursor_Reset, {k10Jobs + 72});
    call(ctx, g8::fn::Mem_FreePool, {load<u32>(k10Jobs + 56), 6, 0});
    call(ctx, g10::fn::cellSpursShutdownJobChain, {k10JobChain});
    call(ctx, g10::fn::cellSpursJoinJobChain, {k10JobChain});
    free_part10(ctx, k10JobPart);
    ret(ctx, 1u);
}

// void RenderJobs_Kick_0053C8F8(): once per frame (u8 +71) while the jobs
// are started (u8 +1): f_0060DFD0(job context), f_0060DE84(job context,
// buffers[index ^ 1] (twice), +104, +108, +112, 10, 89, 1, +60 stamp),
// f_0060E01C(job context); +64 = the stamp.
DECOMP(0053C8F8, RenderJobs_Kick_0053C8F8) {
    if (load<u8>(k10Jobs + 1) == 0) return ret(ctx, 0u);
    if (load<u8>(k10Jobs + 71) != 0) return ret(ctx, static_cast<u32>(load<u8>(k10Jobs + 71)));  // r3: the byte tested
    store<u8>(k10Jobs + 71, 1);
    const u32 jc = k10Jobs + 4;
    call(ctx, g10::fn::Edge_0060DFD0, {jc});
    const u32 buf = load<u32>(k10Jobs + 88 + ((load<u32>(k10Jobs + 52) ^ 1u) << 2));
    const u32 started = load<u8>(k10Jobs + 1) != 0 ? 1u : 0u;  // re-read: cntlzw / xori
    call(ctx, g10::fn::Edge_0060DE84,
         {jc, buf, buf, load<u32>(k10Jobs + 104), load<u32>(k10Jobs + 108), load<u32>(k10Jobs + 112), 10, 89, started,
          load<u32>(k10Jobs + 60)});
    call(ctx, g10::fn::Edge_0060E01C, {jc});
    store<u32>(k10Jobs + 64, load<u32>(k10Jobs + 60));
    ctx.gpr[3] = 0x01160000u;  // r3 as left (lis)
}

// void (): f_0053CA20(0x010DDB88); unless u8 0x010DDBB0 is set while u8
// 0x010DDBA8 is clear: f_005181E0(), Draw_SetTarget(0x011640D4's target,
// 0), invalidate the texture cache, Draw_Clear(240, 0xFF000000, depth
// 1.0 (0x0051848C), 0, 255) and BLEND_ENABLE = 1 inline; then always
// f_005E24F8(gcm context, 2042).
DECOMP(00518490, ShadowPass_Begin_00518490) {
    constexpr u32 kPass = 0x010DDB88;
    call(ctx, 0x0053CA20u, {kPass});
    const u32 flag = load<u8>(0x010DDBB0u);
    if (flag == 0 || load<u8>(kPass + 32) != 0) {
        call(ctx, 0x005181E0u);  // reads no argument registers
        call(ctx, g10::fn::Draw_SetTarget, {load<u32>(0x011640D4u), 0});
        call(ctx, fx::fn::cellGcmSetInvalidateTextureCache, {load<u32>(k10GcmState), 1});
        callf(ctx, g10::fn::Draw_Clear, {240, 0xFF000000u, 0x0051848Cu /* r5: the constant's address, unused */, 0, 255},
              {static_cast<f64>(fx::load_f32(0x0051848Cu))});
        const u32 gcm = load<u32>(k10GcmState);
        const u32 cur = load<u32>(gcm + 8);
        store<u32>(cur, fx::kBlendEnable);
        store<u32>(cur + 4, 1);
        store<u32>(gcm + 8, cur + 8);
    }
    call(ctx, g10::fn::Gcm_SetReferenceCommand_005E24F8, {load<u32>(k10GcmState), 2042});
}

// void (CallSlot* out): pads the command buffer to 128 bytes
// (f_005E0AF0), records there a call to the RSX offset of that spot
// (f_005E0600; the jump target is patched later by CmdBuf_Call_00537D5C),
// pads again, and returns out = {the call's address, the address after
// it, that address's RSX offset}.
DECOMP(00537DD4, CmdBuf_ReserveCall_00537DD4) {
    const u32 out = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 b = frame.addr();
    u32 gcm = load<u32>(k10GcmState);
    u32 pad = pad_words10(gcm);
    if (pad != 0) {
        call(ctx, g10::fn::Gcm_PadNops_005E0AF0, {gcm, pad});
        gcm = load<u32>(k10GcmState);
    }
    const u32 at = load<u32>(gcm + 8);
    call(ctx, fx::imp::cellGcmAddressToOffset, {at, b});
    call(ctx, g10::fn::Gcm_SetCall_005E0600, {load<u32>(k10GcmState), load<u32>(b)});
    gcm = load<u32>(k10GcmState);
    const u32 after = load<u32>(gcm + 8);
    pad = pad_words10(gcm);
    if (pad != 0) call(ctx, g10::fn::Gcm_PadNops_005E0AF0, {gcm, pad});
    store<u32>(b + 4, at);
    store<u32>(b + 8, after);
    call(ctx, fx::imp::cellGcmAddressToOffset, {after, b + 12});
    store<u32>(out, load<u32>(b + 4));
    store<u32>(out + 4, load<u32>(b + 8));
    store<u32>(out + 8, load<u32>(b + 12));
    ctx.gpr[3] = load<u32>(b + 12);  // r3 as left
}

// bool (IffReader* r, MeshBatches* o): the batch list header: u8 (unused),
// u16 "numBatches", f32 "vertScale", u8 "isSkinned"; with batches: o+8 =
// vertScale, u16 +12 = 0, u16 +14 = count, bit 0x80 of u8 +16 =
// isSkinned, o+0 = Mem_AllocPool(16, count * 24, 0, 0). Returns 1.
DECOMP(00523DF4, Iff_HandleBatches_00523DF4) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 b = frame.addr();  // +0 u16 count, +4 vertScale, +8 u8 isSkinned, +12 u8
    store<u32>(b, 0);
    store<u32>(b + 4, 0);
    store<u32>(b + 8, 0);
    call(ctx, g10::fn::Iff_ReadU8, {r, b + 12, 0});
    call(ctx, 0x00564340u /* Iff_ReadU16 */, {r, b, 0x00B238DBu});                // "numBatches"
    call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, b + 4, 0x00B238E6u});      // "vertScale"
    call(ctx, g10::fn::Iff_ReadU8, {r, b + 8, 0x00B238F0u});                     // "isSkinned"
    const u32 n = load<u16>(b);
    if (n != 0) {
        const u32 skinned = load<u8>(b + 8) != 0 ? 0x80u : 0u;
        store<u32>(o + 8, load<u32>(b + 4));  // lfs / stfs: raw
        ctx.fpr[1] = static_cast<f64>(load<f32>(b + 4));
        store<u16>(o + 12, 0);
        store<u16>(o + 14, static_cast<u16>(n));
        store<u8>(o + 16, static_cast<u8>((load<u8>(o + 16) & 0x7F) | skinned));
        store<u32>(o, static_cast<u32>(call(ctx, 0x0056C514u /* Mem_AllocPool */, {16, n * 24, 0, 0})));
    }
    ret(ctx, 1u);
}

// void TableEntry_Acquire_005ACF90(u32 value, s32* out): with the table
// on (u8 0x0119509C) the next of its 128-byte entries (0x011950A0: +4 ->
// {u16 capacity, u16 count}, +8 back-pointer array or 0, +12 entries):
// the index goes to *out, the back-pointer array gets out, the count
// grows; a full table gives index -1 and still fills the entry before
// the first (the original's quirk). The entry is zeroed, +4 = value,
// +100..+103 = the bytes at 0x01164CA0, u8 +0 = 5. Table off: *out = -1.
// (TableEntry_Release_005AD0A0's pair.)  [called by f_0025D370]
DECOMP(005ACF90, TableEntry_Acquire_005ACF90) {
    const u32 value = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    if (load<u8>(0x0119509Cu) == 0) {
        store<u32>(out, 0xFFFFFFFFu);
        return ret(ctx, -1);
    }
    constexpr u32 kTable = 0x011950A0;
    const u32 hdr = load<u32>(kTable + 4);
    s32 idx = -1;
    if (static_cast<s32>(load<u16>(hdr + 2)) < static_cast<s32>(load<u16>(hdr))) {  // cmpw
        idx = load<u16>(hdr + 2);
        const u32 back = load<u32>(kTable + 8);
        u32 h = hdr;
        if (back != 0) {
            store<u32>(back + (static_cast<u32>(idx) << 2), out);
            h = load<u32>(kTable + 4);
        }
        store<u16>(h + 2, static_cast<u16>(load<u16>(h + 2) + 1));
    }
    const u32 e = load<u32>(kTable + 12) + (static_cast<u32>(idx) << 7);
    for (u32 k = 0; k < 128; k += 4) store<u32>((e & ~15u) + k, 0);  // stvx
    store<u32>(e + 4, value);
    for (u32 k = 0; k < 4; ++k) store<u8>(e + 100 + k, load<u8>(0x01164CA0u + k));
    store<u8>(e, 5);
    store<u32>(out, static_cast<u32>(idx));
    ret(ctx, 5u);  // r3 as left
}

// void (): the shared 20-byte light (?) record and its 12-byte child:
// f_0052730C(), f_005271F8() (the 8 direction vectors), then s =
// Mem_Alloc(20, 0, 16) ({u16 1, +4 0, +8 0, u8 +12 0, +16 5.0f}) is
// appended to the PtrVector 0x010DFA24 (null too), u8 s->+12 = 1; t =
// Mem_Alloc(12, 0, 16) ({0, u16 0, u8 0, 0}); s->+8 = t, t->+8 = 0, u16
// t->+4 = 0. A failed allocation leaves the writes at address 0 + offset
// (the original's null writes). Returns t  [called by f_00527608] (name?)
DECOMP(00527484, Records_Init_00527484) {
    call(ctx, 0x0052730Cu);
    call(ctx, 0x005271F8u);
    u32 s = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {20, 0, 16}));
    if (s != 0) {
        store<u32>(s + 4, 0);
        store<u16>(s, 1);
        store<u32>(s + 8, 0);
        store<u8>(s + 12, 0);
        store<u32>(s + 16, 0x40A00000u);  // 5.0f
    }
    constexpr u32 kVec = 0x010DFA24;  // +0 data, +4 u16 capacity, +8 u16 count, +10 u16 high-water mark
    if (load<u16>(kVec + 8) == load<u16>(kVec + 4))
        call(ctx, 0x00551718u /* PtrVector_Reserve */, {kVec, (static_cast<u32>(load<u16>(kVec + 4)) << 1) & 0x1FFFEu});
    const u32 slot = (static_cast<u32>(load<u16>(kVec + 8)) << 2) & 0x3FFFCu;
    const u32 data = load<u32>(kVec);
    const u32 n = load<u16>(kVec + 8) + 1u;
    const u32 high = load<u16>(kVec + 10);
    store<u32>(data + slot, s);
    store<u16>(kVec + 8, static_cast<u16>(n));
    store<u16>(kVec + 10, static_cast<u16>(high > (n & 0xFFFF) ? high : n));
    store<u8>(s + 12, 1);
    const u32 t = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {12, 0, 16}));
    if (t != 0) {
        store<u32>(t, 0);
        store<u16>(t + 4, 0);
        store<u8>(t + 6, 0);
        store<u32>(t + 8, 0);
    }
    store<u32>(s + 8, t);
    store<u32>(t + 8, 0);
    const u32 t2 = load<u32>(s + 8);
    store<u16>(t2 + 4, 0);
    ret(ctx, t2);
}

// bool Iff_HandleIndices_00525904(IffReader* r, Mesh* m): the index chunk
// (m->+64 u16 indices) into the buffer part at +44: m->+60 = 1,
// Buffer_Alloc52F198(part, count * 2, 128, 0, 2), bits 1..3 of the
// part's +8 = 2, the indices read into part->+0 and byte-swapped when
// the reader swaps (u8 r->+12 bit 2; count & 0x7FFFFFFF of them).
// Returns 1. (Iff_HandleXTVM's twin.)
DECOMP(00525904, Iff_HandleIndices_00525904) {
    const u32 r = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 n = load<u32>(m + 64);
    call(ctx, g8::fn::Iff_ChunkSize, {r, m});  // result unused
    const u32 part = m + 44;
    store<u32>(m + 60, 1);
    call(ctx, 0x0052F198u /* Buffer_Alloc52F198 */, {part, load<u32>(m + 64) << 1, 128, 0, 2});
    store<u32>(part + 8, (load<u32>(part + 8) & ~0xEu) | 0x4u);
    const u32 p = load<u32>(part);
    call(ctx, g8::fn::FileReader_Read, {r, p, n << 1});
    if ((load<u8>(r + 12) & 2) != 0) {
        u32 k = n & 0x7FFFFFFFu;
        for (u32 q = p; k != 0; --k, q += 2) store<u16>(q, static_cast<u16>((load<u8>(q + 1) << 8) | load<u8>(q)));
    }
    ret(ctx, 1u);
}

// void Gcm_EnterSubContext_0052F780(): switches command output to the
// secondary context at 0x010F0E0C, using the buffer the frame state
// (0x010F0E08) picks (+268[+284]): when that buffer is set, the words at
// +256 and +260 are atomically set to 0 and +264 / 4 (lwarx / stwcx.);
// a call to the buffer's RSX offset goes into the current context
// (f_005E0600), the TOC word from GetToc_005DF950's offset to
// 0x010F0E1C, Buffer_Init_005DF960(0x010F0E0C, buffer, size +264,
// 0x00F3BCF0) and 0x010F0E0C becomes the gcm context (fx::g::state).
// (Gcm_FlipContext_0053BE94 switches back.)  [called by f_005325D8]
DECOMP(0052F780, Gcm_EnterSubContext_0052F780) {
    constexpr u32 kFrame = 0x010F0E08, kSub = 0x010F0E0C, kSubOffset = 0x010F0E1C;
    u32 f = load<u32>(kFrame);
    if (load<u32>(f + 268 + (load<u32>(f + 284) << 2)) != 0) {
        std::atomic_ref<u32>(ref<u32>(load<u32>(f + 256))).store(0);
        const u32 v = load<u32>(f + 264) >> 2;
        std::atomic_ref<u32>(ref<u32>(load<u32>(f + 260))).store(__builtin_bswap32(v));
        f = load<u32>(kFrame);
    }
    const u32 size = load<u32>(f + 264);
    const u32 buf = load<u32>(f + 268 + (load<u32>(f + 284) << 2));
    Stack frame(ctx, 16);
    call(ctx, fx::imp::cellGcmAddressToOffset, {buf, frame.addr()});
    call(ctx, g10::fn::Gcm_SetCall_005E0600, {load<u32>(k10GcmState), load<u32>(frame.addr())});
    const u32 toc = static_cast<u32>(call(ctx, 0x005DF950u /* GetToc_005DF950 */));
    call(ctx, fx::imp::cellGcmAddressToOffset, {toc, kSubOffset});
    call(ctx, 0x005DF960u /* Buffer_Init_005DF960 */, {kSub, buf, size, 0x00F3BCF0u});
    store<u32>(k10GcmState, kSub);
}

// void (View* v) [called through a pointer]: for a view with u8 +49 == 1
// unless the display mode m (0x010F0E20 +184) is 3: f_00536290(a stack
// copy of the 16 floats at +1008, v+1008, m, v+1008, m; f1 / f2 = v's
// +1824 / +1828 when bit 0x20 of u8 +1776 is set, else 0x010F0E20's +168
// / +172) (name?)
DECOMP(005483EC, View_StereoMatrix_005483EC) {
    const u32 v = arg<u32>(ctx, 0);
    constexpr u32 kMode = 0x010F0E20;
    const u32 mode = load<u32>(kMode + 184);
    if (load<u8>(v + 49) != 1 || mode == 3) return;
    Stack frame(ctx, 64);
    const u32 b = frame.addr();
    for (u32 k = 0; k < 64; k += 4) store<u32>(b + k, load<u32>(v + 1008 + k));  // lfs / stfs: raw
    const u32 src = (load<u8>(v + 1776) & 0x20) != 0 ? v + 1824 : kMode + 168;
    callf(ctx, 0x00536290u, {b, v + 1008, mode, v + 1008, mode},
          {static_cast<f64>(load<f32>(src)), static_cast<f64>(load<f32>(src + 4))});
}

// FrameBuffer* FrameBuffer_Reset_00523584(FrameBuffer* fb, u32 a, u32 b, u32
// c): a type-18 record {header (18 << 24 | c's bits 16..23), c's low 16
// bits, 0; a; b} into fb's record stream (start +0, end +4, cursor +16;
// each piece wraps to the start when it would pass the end, 4-aligned),
// then the cached state reset: +80 / +84 / +88 = -1, the cursor past one
// more word, +96 = 100000.0f, +160 = -1, u16 +164 = 0xFFFF, +176..+192 =
// -1, +196 = -1.0f, u8 +200 = 0. Returns fb.
DECOMP(00523584, FrameBuffer_Reset_00523584) {
    const u32 fb = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 4);
    const u32 start = load<u32>(fb), end = 4;
    u32 p = load<u32>(fb + 16);
    if (p + 15 > load<u32>(fb + end)) p = start;  // cmplw
    p = (p + 3) & ~3u;
    store<u32>(fb + 16, p);
    store<u32>(p + 8, 0);
    store<u32>(p, 0x12000000u | (c & 0x00FF0000u));
    store<u32>(p + 4, c & 0xFFFFu);
    p = load<u32>(fb + 16);
    p = ((p + 19 > load<u32>(fb + end) ? load<u32>(fb) : p + 12) + 3) & ~3u;
    store<u32>(fb + 16, p);
    store<u32>(p, a);
    p = load<u32>(fb + 16);
    p = ((p + 11 > load<u32>(fb + end) ? load<u32>(fb) : p + 4) + 3) & ~3u;
    store<u32>(fb + 16, p);
    store<u32>(p, b);
    store<u32>(fb + 84, 0xFFFFFFFFu);
    store<u32>(fb + 80, 0xFFFFFFFFu);
    store<u32>(fb + 88, 0xFFFFFFFFu);
    const u32 cur = load<u32>(fb + 16) + 4;
    store<u32>(fb + 16, cur);
    store<u32>(fb + 96, 0x47C35000u);  // 100000.0f
    store<u32>(fb + 160, 0xFFFFFFFFu);
    store<u16>(fb + 164, 0xFFFF);
    store<u64>(fb + 176, ~u64{0});
    store<u64>(fb + 184, ~u64{0});
    store<u32>(fb + 192, 0xFFFFFFFFu);
    store<u32>(fb + 196, 0xBF800000u);  // -1.0f
    store<u8>(fb + 200, 0);
    // f_0045717C calls this again with the r7 left here (-1) on one path:
    // the volatiles are set as the original leaves them.
    ctx.gpr[4] = 0xFFFFFFFFu;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = fb + 96;
    ctx.gpr[7] = 0xFFFFFFFFu;
    ctx.gpr[8] = 0x47C35000u;
    ctx.gpr[9] = 0xFFFF;
    ctx.gpr[10] = 0xBF800000u;
    ctx.gpr[11] = cur;
    ret(ctx, fb);
}

// void Warp_Update_00545A38(Warp* w): for an active warp (u8 +4, u8 +5740
// clear) whose screen-space quad isn't degenerate (|len(+5760.. - +5768..)
// * len(+5744.. - +5768..)| not below the f32 at 0x00545A30): the first
// time (u8 0x01151A00) the grid size u8 +5713 doubles and u8 +5714
// becomes * 6 / 4 (+5784 / +5785 = each - 1) and f_00544B4C(w) runs; then
// f_00544E08(w), Warp_Draw(w) and +5780 ^= 1 (the double buffer). (name?)
DECOMP(00545A38, Warp_Update_00545A38) {
    const u32 w = arg<u32>(ctx, 0);
    if (load<u8>(w + 4) == 0) return ret(ctx, 0u);
    if (load<u8>(w + 5740) != 0) return ret(ctx, static_cast<u32>(load<u8>(w + 5740)));  // r3: the byte tested
    auto sub = [](f32 x, f32 y) { return static_cast<f32>(static_cast<f64>(x) - y); };
    auto sq = [](f32 x) { return static_cast<f32>(static_cast<f64>(x) * x); };
    auto madd = [](f32 a, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), a, b)); };
    const f32 a = madd(sub(load<f32>(w + 5760), load<f32>(w + 5768)), sq(sub(load<f32>(w + 5764), load<f32>(w + 5772))));
    const f32 b = madd(sub(load<f32>(w + 5744), load<f32>(w + 5768)), sq(sub(load<f32>(w + 5748), load<f32>(w + 5772))));
    const f32 la = static_cast<f32>(std::sqrt(static_cast<f64>(a))), lb = static_cast<f32>(std::sqrt(static_cast<f64>(b)));
    const f32 area = std::fabs(static_cast<f32>(static_cast<f64>(la) * lb));
    ctx.fpr[1] = static_cast<f64>(area);  // f1 as left
    if (area < load<f32>(0x00545A30u)) return ret(ctx, 0x00545A30u);  // r3: the constant's address
    constexpr u32 kGridBuilt = 0x01151A00;
    if (load<u8>(kGridBuilt) == 0) {
        const u32 cols = load<u8>(w + 5714), rows = (static_cast<u32>(load<u8>(w + 5713)) << 1);
        store<u8>(w + 5713, static_cast<u8>(rows));
        store<u8>(w + 5784, static_cast<u8>(rows - 1));
        const u32 c = static_cast<u32>(static_cast<s32>(cols * 6) / 4);  // srawi + addze
        store<u8>(w + 5714, static_cast<u8>(c));
        store<u8>(w + 5785, static_cast<u8>(c - 1));
        call(ctx, 0x00544B4Cu, {w});
        store<u8>(kGridBuilt, 1);
    }
    call(ctx, 0x00544E08u, {w});
    call(ctx, 0x0054522Cu /* Warp_Draw */, {w});
    const u32 flip = load<u32>(w + 5780) ^ 1u;
    store<u32>(w + 5780, flip);
    ret(ctx, flip);
}

// bool Iff_HandleTextureFile_0051D718(IffReader* r, Texture** pt): the
// texture file reference chunk (skipped when u8 r->+12 bit 4 is set, or,
// with config u8 +160 bit 0x80, when Texture_CopyDefaultFormat_005398CC(
// *pt) succeeds): a u8 (unused), a u8 length and that many name bytes;
// Texture_LoadFile_0053A8B0(*pt, String(0x010DDB40 + name)). Returns 1 /
// LoadFile's result.
DECOMP(0051D718, Iff_HandleTextureFile_0051D718) {
    const u32 r = arg<u32>(ctx, 0), pt = arg<u32>(ctx, 1);
    if ((load<u8>(r + 12) & 4) != 0) return ret(ctx, 1u);
    const u32 tex = load<u32>(pt);
    if ((load<u8>(0x010C5A20u + 160) & 0x80) != 0 &&
        call(ctx, 0x005398CCu /* Texture_CopyDefaultFormat_005398CC */, {tex}) != 0)
        return ret(ctx, 1u);
    Stack frame(ctx, 528);
    const u32 b = frame.addr();  // +0 u8, +1 u8 length, +8 String, +264 name
    const u32 str = b + 8, name = b + 264;
    store<u8>(b, 0);
    call(ctx, g8::fn::FileReader_Read, {r, b, 1});
    store<u8>(b + 1, 0);
    call(ctx, g8::fn::FileReader_Read, {r, b + 1, 1});
    call(ctx, g8::fn::FileReader_Read, {r, name, load<u8>(b + 1)});
    store<u8>(name + load<u8>(b + 1), 0);
    call(ctx, 0x00555F80u /* String_Assign */, {str, 0x010DDB40u});
    call(ctx, 0x005566A8u /* String_Append */, {str, name});
    call(ctx, 0x0053A8B0u /* Texture_LoadFile_0053A8B0 */, {tex, str});
}

namespace {
// The pooled array headers (007A7550 / 007A7674 / 007A77A0): (Array* o,
// u32 n, bool indexed, Allocator* a): for n != 0 a block of (n * elem +
// (indexed ? n * 4 : 0) + 31) & ~15 bytes from a's slot 0 (a, size, 0,
// 16): o+0 = o+4 = block {u16 n, u16 0, +4 size, +8 indexed, +12 a} (the
// header writes go to address 0 + offset when it fails), o+8 = the index
// array (block + 16, 0 when not indexed), o+12 = the elements (after the
// index array when indexed, else block + 16).
void array_init10(PPUContext& ctx, u32 elem) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), indexed = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3);
    if (n == 0) return;
    const u32 extra = indexed != 0 ? n << 2 : 0u;
    const u32 size = (n * elem + extra + 31) & ~15u;
    const u32 p = static_cast<u32>(fx::vcall(ctx, a, 0, {a, size, 0, 16}));
    store<u32>(o, p);
    store<u32>(o + 4, p);
    if (p != 0) {
        store<u16>(p + 2, 0);
        store<u32>(p + 4, 0);
        store<u32>(p + 8, 0);
    }
    store<u16>(p, static_cast<u16>(n));
    const u32 h = load<u32>(o + 4);
    store<u32>(h + 8, indexed);
    store<u32>(h + 4, size);
    store<u32>(h + 12, a);
    const u32 base = load<u32>(o);
    const u32 idx = indexed != 0 ? p + 16 : 0u;
    store<u32>(o + 8, idx);
    const u32 elems = idx != 0 ? base + 16 + (static_cast<u32>(load<u16>(h)) << 2) : base + 16;
    store<u32>(o + 12, elems);
    ctx.gpr[4] = h;  // left as the original leaves them
    ret(ctx, elems);
}
}  // namespace

// void Array_Init128_007A7550(Array*, u32 n, bool indexed, Allocator*): 128-byte elements  [called by f_00586DB4] (name?)
DECOMP(007A7550, Array_Init128_007A7550) { array_init10(ctx, 128); }

// void Array_Init96_007A7674(Array*, u32 n, bool indexed, Allocator*): 96-byte elements  [called by f_00586FEC] (name?)
DECOMP(007A7674, Array_Init96_007A7674) { array_init10(ctx, 96); }

// void Array_Init4_007A77A0(Array*, u32 n, bool indexed, Allocator*): 4-byte elements  [called by f_00588240] (name?)
DECOMP(007A77A0, Array_Init4_007A77A0) { array_init10(ctx, 4); }

// u32 DrawList_Run_00517754(): the 8-byte entries {key, Item*} at
// 0x010EF63C (count 0x010F0A3C) sorted (SortEntries), a wait (300 us
// sys_timer_usleep steps) while the u32 at 0x010DDC00 is set, then per
// entry item->+64's virtual slot 3 (renderer, item, +68, +72, the 32-byte
// slot +76 of the table at 0x00F66FD8; r8 / r9 the entry and OPD), and
// the count cleared. Returns the count it had.  [called by f_003E3428] (name?)
DECOMP(00517754, DrawList_Run_00517754) {
    constexpr u32 kList = 0x010EF63C, kCount = kList + 5120, kBusy = 0x010DDC00;
    if (load<u32>(kCount) == 0) return ret(ctx, 0u);
    call(ctx, 0x0056A81Cu /* SortEntries */, {kList, load<u32>(kCount), 8});
    while (load<u32>(kBusy) != 0) syscall(ctx, 141 /* sys_timer_usleep */, {300});
    u32 n;
    for (u32 i = 0; i < (n = load<u32>(kCount)); ++i) {  // cmplw, the count re-read each time
        const u32 item = load<u32>(kList + 8 * i + 4);
        const u32 table = load<u32>(0x00F66FD8u);
        const u32 rend = load<u32>(item + 64);
        const u32 slot = table + (load<u32>(item + 76) << 5);
        const u32 opd = load<u32>(load<u32>(rend) + 12);
        call_ptr(ctx, opd, {rend, item, load<u32>(item + 68), load<u32>(item + 72), slot, load<u32>(opd), opd});
    }
    store<u32>(kCount, 0);
    ret(ctx, n);
}

// Obj* Construct_0051F708(Obj* o): the constructor body (vtable 0x00B23670):
// u8 +4 / +5 / +12 and +8, +20..+28 cleared, u8 +32 = 1, bytes +33..+36
// = {0, 128, 255, 255}, then the tuning floats +48 0.005, +52..+68 0,
// +72 / +76 8.0, +80 0.75, +84 1.0, +88 / +92 0, +96 0.75, +100 1.0, +104
// 840.0, +108 0. Returns o + 108 (as the original leaves r3)  [called by
// f_0051F850] (name?)
DECOMP(0051F708, Construct_0051F708) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 4, 0);
    store<u8>(o + 5, 0);
    store<u32>(o + 8, 0);
    store<u8>(o + 12, 0);
    store<u32>(o, 0x00B23670u);
    store<u32>(o + 20, 0);
    store<u32>(o + 24, 0);
    store<u32>(o + 28, 0);
    store<u8>(o + 32, 1);
    store<u8>(o + 33, 0);
    store<u8>(o + 34, 128);
    store<u8>(o + 35, 255);
    store<u8>(o + 36, 255);
    static constexpr u32 kTuning[16] = {0x3BA3D70Au, 0, 0, 0, 0, 0, 0x41000000u, 0x41000000u,
                                        0x3F400000u, 0x3F800000u, 0, 0, 0x3F400000u, 0x3F800000u, 0x44520000u, 0};
    for (u32 k = 0; k < 16; ++k) store<u32>(o + 48 + 4 * k, kTuning[k]);
    ret(ctx, o + 108);
}

// bool LoadParams_005C25B8(ShaderType* t, Material* m, const Desc* d)
// [a ShaderType's slot 1]: with u8 d->+84 set: the loaded texture named
// by d->+4 (virtual slot 4 (+16) (tex, 1, 1, 1), then u16 +116 / +118 /
// +120 = 1 and bit 0 of u16 +128 set) or Tex_GetDefault(0) goes to
// f_004FF008(m, tex), and material parameter 13 = d->+140 when u8 +84 >
// 13, else 1.0 (0x005C25B4); then f_005BDD30(0x01196EC0, m, d) != 0. (name?)
DECOMP(005C25B8, LoadParams_005C25B8) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if (load<u8>(d + 84) != 0) {
        u32 tex = static_cast<u32>(call(ctx, 0x0050C398u /* Tex_FindLoaded */, {load<u32>(d + 4)}));
        if (tex != 0) {
            fx::vcall(ctx, tex, 16, {tex, 1, 1, 1});
            const u32 flags = load<u16>(tex + 128) | 1u;
            store<u16>(tex + 116, 1);
            store<u16>(tex + 128, static_cast<u16>(flags));
            store<u16>(tex + 118, 1);
            store<u16>(tex + 120, 1);
        } else {
            tex = static_cast<u32>(call(ctx, 0x004FEF30u /* Tex_GetDefault */, {0}));
        }
        call(ctx, 0x004FF008u, {m, tex});
        const u32 v = load<u8>(d + 84) > 13 ? load<u32>(d + 140) : load<u32>(0x005C25B4u);  // lfs / stfs: raw
        const u32 slot = static_cast<u32>(call(ctx, 0x0050C018u /* Material_ParamSlot */, {m, 13}));
        store<u32>(slot, v);
    }
    ret(ctx, call(ctx, 0x005BDD30u, {0x01196EC0u, m, d}) != 0 ? 1u : 0u);
}

// void View_ScreenToWorld_00522838(View* v, f32 out[3], const f32 p[4]):
// View_Update(v, 0), the inverse of v's matrix at +1328 (Mat4_Invert),
// then {(p.x * 2 - 1) w, (p.y * -2 + 1) w, p.z, w} (constants at
// 0x0052282C: 2.0, 1.0, -2.0; w = p[3]) through it; a perspective view
// (u8 +49 clear) divides the result by its w. out = xyz; f1 = z.
// [called by Call_0052296C, f_003BCB20, f_003C1450]
DECOMP(00522838, View_ScreenToWorld_00522838) {
    const u32 v = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    constexpr u32 kK = 0x0052282C;
    call(ctx, 0x005115F0u /* View_Update */, {v, 0});
    Stack frame(ctx, 160);
    const u32 b = frame.addr();  // +0 the point, +16 the result, +32 / +96 Mat4_Invert's two matrices
    call(ctx, 0x00554C28u /* Mat4_Invert */, {b + 32, b + 96, v + 1328});
    const f32 k0 = load<f32>(kK), one = load<f32>(kK + 4), k2 = load<f32>(kK + 8);
    const f32 w = load<f32>(p + 12);
    const f32 x = static_cast<f32>(std::fma(static_cast<f64>(load<f32>(p)), k0, -static_cast<f64>(one)));  // fmsubs
    const f32 y = static_cast<f32>(std::fma(static_cast<f64>(load<f32>(p + 4)), k2, one));                 // fmadds
    store<u32>(b + 8, load<u32>(p + 8));
    store<u32>(b + 12, load<u32>(p + 12));
    store<f32>(b, static_cast<f32>(static_cast<f64>(x) * w));
    store<f32>(b + 4, static_cast<f32>(static_cast<f64>(y) * w));
    call(ctx, 0x00553910u /* Vec4_TransformByMatrix */, {b + 16, b, b + 96});
    f32 rx = load<f32>(b + 16), ry = load<f32>(b + 20), rz = load<f32>(b + 24);
    if (load<u8>(v + 49) == 0) {
        const f32 inv = static_cast<f32>(static_cast<f64>(one) / load<f32>(b + 28));
        rx = static_cast<f32>(static_cast<f64>(rx) * inv);
        ry = static_cast<f32>(static_cast<f64>(ry) * inv);
        rz = static_cast<f32>(static_cast<f64>(rz) * inv);
    }
    store<f32>(out, rx);
    store<f32>(out + 4, ry);
    store<f32>(out + 8, rz);
    ctx.fpr[1] = static_cast<f64>(rz);
    ctx.gpr[3] = out + 4;  // left as the original leaves them
    ctx.gpr[4] = out + 8;
}

// bool Reader_ReadFlags_007A50D4(Obj* o) [called through a pointer]: from
// the stream o->+4 (virtual slot 3 (+12) read(stream, buf, 1, 0), each
// result added to o->+8): a version byte (1 if absent), a flag byte and,
// from version 1, a byte read and dropped; bit 0x02 of u8 o->+12 = (the
// flag == 0). Returns 1. (name?)
DECOMP(007A50D4, Reader_ReadFlags_007A50D4) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 b = frame.addr();  // +0 u8 flag, +1 u8 dropped, +2 u8 version
    store<u16>(b, 0);
    store<u8>(b + 2, 1);
    auto read = [&](u32 dst) {
        const u32 s = load<u32>(o + 4);
        const u32 n = static_cast<u32>(fx::vcall(ctx, s, 12, {s, dst, 1, 0}));
        store<u32>(o + 8, load<u32>(o + 8) + n);
    };
    read(b + 2);
    read(b);
    if (load<u8>(b + 2) >= 1) read(b + 1);
    const u32 f = load<u8>(o + 12);
    store<u8>(o + 12, static_cast<u8>(load<u8>(b) == 0 ? f | 2u : f & ~2u));
    ret(ctx, 1u);
}

// void DrawQueue_Begin_00518610(): with the layer-0 marker on (u8
// 0x00F66F94): a command-buffer call slot (CmdBuf_ReserveCall_00537DD4)
// kept at 0x010DD490, Draw_InvalidateState() and the frame stamp (render
// state +156) bumped. Otherwise the draw queue at 0x011DE300 (built the
// first time, u8 0x011DE480: u8 +0 = 1, PtrVector at +4 with u16 +10 =
// 160, +32 = 0, +128 / +256 atomically cleared) is waited on (300 us
// sys_timer_usleep steps while +128 is set) and run (f_00516CD0).
// [called by f_005073F8, f_0051669C, f_00518E78] (name?)
DECOMP(00518610, DrawQueue_Begin_00518610) {
    if (load<u8>(0x00F66F94u) != 0) {
        Stack frame(ctx, 16);
        call(ctx, 0x00537DD4u /* CmdBuf_ReserveCall_00537DD4 */, {frame.addr()});
        for (u32 k = 0; k < 12; k += 4) store<u32>(0x010DD490u + k, load<u32>(frame.addr() + k));
        call(ctx, g9::fn::Draw_InvalidateState);
        store<u32>(k10GcmState + 156, load<u32>(k10GcmState + 156) + 1);
        return ret(ctx, k10GcmState);
    }
    constexpr u32 kQueue = 0x011DE300, kBuilt = 0x011DE480;
    if (load<u8>(kBuilt) == 0) {
        store<u8>(kQueue, 1);
        call(ctx, 0x005516E0u /* PtrVector_Construct */, {kQueue + 4});
        store<u16>(kQueue + 10, 160);
        store<u32>(kQueue + 32, 0);
        std::atomic_ref<u32>(ref<u32>(kQueue + 128)).store(0);
        std::atomic_ref<u32>(ref<u32>(kQueue + 256)).store(0);
        store<u8>(kBuilt, 1);
    }
    while (load<u32>(kQueue + 128) != 0) syscall(ctx, 141 /* sys_timer_usleep */, {300});
    call(ctx, 0x00516CD0u, {kQueue});
}

// void Warp_BuildStrip_0051F520(Warp* w, u8 cols, u8 rows): the grid size
// (each at most 15) to u8 +5713 / +5714 and the triangle-strip indices
// to u16 +5144.. (count u16 +5716): per row the pairs (below, above) for
// each of the cols + 1 columns, then the two degenerate joins (last
// above, next row's first below); the count ends 2 short (0xFFFE for no
// rows). (Warp_Update_00545A38's grid.)  [called by f_005467DC] (name?)
DECOMP(0051F520, Warp_BuildStrip_0051F520) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 cols = std::min<u32>(arg<u32>(ctx, 1) & 0xFF, 15), rows = std::min<u32>(arg<u32>(ctx, 2) & 0xFF, 15);
    store<u8>(w + 5713, static_cast<u8>(cols));
    store<u8>(w + 5714, static_cast<u8>(rows));
    store<u16>(w + 5716, 0);
    const u32 cnt = w + 5716, idx = w + 5144;
    auto push = [&](u32 v) {
        const u32 n = load<u16>(cnt);
        store<u16>(cnt, static_cast<u16>(n + 1));
        store<u16>(idx + ((n << 1) & 0x1FFFEu), static_cast<u16>(v));
    };
    u32 below = cols + 1, above = 0;
    // The sizes are re-read each pass: a big grid's indices run over them.
    if (rows > 0) {
        u32 row = 0;
        do {
            for (u32 k = 0; k <= load<u8>(w + 5713); ++k) {
                push(below++);
                push(above);
                above = (above + 1) & 0xFFFF;
            }
            push(above - 1);
            push(below);
            row = (row + 1) & 0xFF;
        } while (row < load<u8>(w + 5714));
    }
    store<u16>(cnt, static_cast<u16>(load<u16>(cnt) - 2));
    ret(ctx, w);
}

// Emitter* Emitter_Construct_00500798(Emitter* o, AglEmitProperties* p):
// vtable 0x00B232D8, o+4 = p (EmitPropsRef_Set); for a set, enabled
// (u8 +68 clear) reference bit 0x02 of u8 o+64 = (EmitProps_BlendKind(
// ->+72 + 104) == 3), bit 0x04 cleared; o+8..+56 cleared but +12 = 1.0,
// bits 0xE0 of +64 set, bit 0x10 set unless p's +24 or +32 equals the
// f32 at 0x00500790 (then o+16 = 1.0 when bits 3..4 hold any), +68 = 0.
// Returns 0 (as the original leaves r3)  [called through a pointer] (name?)
DECOMP(00500798, Emitter_Construct_00500798) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    store<u32>(o, 0x00B232D8u);
    store<u32>(o + 4, 0);
    call(ctx, g8::fn::EmitPropsRef_Set, {o + 4, p});
    const u32 ref = load<u32>(o + 4);
    if (ref != 0 && load<u8>(ref + 68) == 0) {
        const u32 kind = static_cast<u32>(call(ctx, 0x0050061Cu /* EmitProps_BlendKind */, {load<u32>(ref + 72) + 104}));
        const u32 b = load<u8>(o + 64) & ~0x6u;
        store<u8>(o + 64, static_cast<u8>(b | (kind == 3 ? 2u : 0u)));
    }
    store<u32>(o + 8, 0);
    store<u32>(o + 12, 0x3F800000u);  // 1.0f
    store<u32>(o + 20, 0);
    store<u32>(o + 16, 0);
    u32 b = load<u8>(o + 64);
    for (u32 off : {24u, 28u, 56u, 52u, 48u, 40u, 36u, 32u}) store<u32>(o + off, 0);
    b |= 0xE0;
    store<u8>(o + 64, static_cast<u8>(b));
    const f32 k = load<f32>(0x00500790u);
    const bool none = load<f32>(p + 32) == k || load<f32>(p + 24) == k;  // fcmpu
    b = (b & ~0x18u) | (none ? 0u : 0x10u);
    store<u8>(o + 64, static_cast<u8>(b));
    if (((b >> 3) & 3) != 0) store<u32>(o + 16, 0x3F800000u);
    store<u32>(o + 68, 0);
    ctx.fpr[1] = static_cast<f64>(k);  // f1 as left
    ret(ctx, 0u);
}

// void RenderBuffer_FlipStats_0050CC10(): the render buffers' peak use
// (0x010DD600, AglRenderBuffer): data in use = (+20 - +24) + (+28 - the
// word +256 points at, read with lwarx); a new peak in 32 KB steps is
// printed ("DataBuffer: Peak: %5dk Free:%5d", the free space from +8)
// and +284 keeps the peak. With a vertex buffer (+260 its size) the same
// for +272 - +264[buffer index] into +288. Then the buffer index (+0)
// flips.  [called by f_0050CD7C] (name?)
DECOMP(0050CC10, RenderBuffer_FlipStats_0050CC10) {
    constexpr u32 kB = 0x010DD600;
    const u32 cur = __builtin_bswap32(std::atomic_ref<u32>(ref<u32>(load<u32>(kB + 256))).load());  // lwarx
    const u32 used = (load<u32>(kB + 20) - load<u32>(kB + 24)) + (load<u32>(kB + 28) - cur);
    u32 peak = load<u32>(kB + 284);
    if ((used >> 15) > (peak >> 15)) {
        call(ctx, g10::fn::Libc_printf, {0x00AF1998u, used >> 10, (load<u32>(kB + 8) - used - 32768) >> 10});
        peak = load<u32>(kB + 284);
    }
    store<u32>(kB + 284, peak > used ? peak : used);
    const u32 vsize = load<u32>(kB + 260);
    if (vsize != 0) {
        const u32 v = load<u32>(kB + 272) - load<u32>(kB + 264 + (load<u32>(kB) << 2));
        u32 peak2 = load<u32>(kB + 288);
        if ((v >> 15) > (peak2 >> 15)) {
            call(ctx, g10::fn::Libc_printf, {0x00AF19D0u, v >> 10, (vsize - v - 32768) >> 10});
            peak2 = load<u32>(kB + 288);
        }
        store<u32>(kB + 288, peak2 > v ? peak2 : v);
    }
    store<u32>(kB, load<u32>(kB) ^ 1u);
    ctx.gpr[3] = 0x010E0000u;  // r3 as left (lis)
}

namespace {
// ApeTListAllocator<T> delete: obj's destructor (virtual slot 0), then
// obj back to T's free list (*slot, ApeTListAllocator<T>::GetAllocator()
// inlined: built on first use as a 60-byte ApeTFreeList {vtable vt, +4..
// +52 cleared, bit 0x8000 of u16 +54 cleared, +56 the signature name}
// and FreeList_Init(list, elem_size, slot->+4 heap, 0, 0); a failed
// allocation leaves the list null and the free goes through it anyway)
// with its virtual slot 4 (+16) (list, obj).
u32 get_list10(PPUContext& ctx, u32 slot_ptr, u32 vt, u32 name, u32 elem_size) {
    const u32 slot = load<u32>(slot_ptr);
    u32 fl = load<u32>(slot);
    if (fl == 0) {
        const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {60, 0, 16}));
        if (p != 0) {
            const u32 heap = load<u32>(slot + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(p + off, 0);
            const u32 h = load<u16>(p + 54) & ~0x8000u;
            store<u16>(p + 52, 0);
            store<u16>(p + 54, static_cast<u16>(h));
            store<u32>(p, vt);
            store<u32>(p + 56, name);
            call(ctx, g9::fn::FreeList_Init, {p, elem_size, heap, 0, 0});
        }
        fl = p;
        store<u32>(slot, fl);
    }
    return fl;
}
void list_delete10(PPUContext& ctx, u32 slot_ptr, u32 vt, u32 name, u32 elem_size) {
    const u32 obj = arg<u32>(ctx, 0);
    fx::vcall(ctx, obj, 0, {obj});
    const u32 fl = get_list10(ctx, slot_ptr, vt, name, elem_size);
    fx::vcall(ctx, fl, 16, {fl, obj});
}
}  // namespace

// void AglRenderSurface_Delete_0051B0E8(AglRenderSurface*): list_delete10
// (0x00F8F110, 156-byte entries)  [called through a pointer] (name?)
DECOMP(0051B0E8, AglRenderSurface_Delete_0051B0E8) { list_delete10(ctx, 0x00F8F110u, 0x00B23D80u, 0x00AF1141u, 156); }

// void AglMaterial_Delete_0051D058(AglMaterial*): list_delete10
// (0x00F8F114, 144-byte entries)  [called through a pointer] (name?)
DECOMP(0051D058, AglMaterial_Delete_0051D058) { list_delete10(ctx, 0x00F8F114u, 0x00B10ED0u, 0x00AF1193u, 144); }

// void AglRenderTarget_Delete_00522E70(AglRenderTarget*): list_delete10
// (0x00F8F11C, 1840-byte entries)  [called through a pointer] (name?)
DECOMP(00522E70, AglRenderTarget_Delete_00522E70) { list_delete10(ctx, 0x00F8F11Cu, 0x00B23DC0u, 0x00AF122Cu, 1840); }

// void AglSubMesh_Delete_00525CD8(AglSubMesh*): list_delete10
// (0x00F8F120, 116-byte entries)  [called through a pointer] (name?)
DECOMP(00525CD8, AglSubMesh_Delete_00525CD8) { list_delete10(ctx, 0x00F8F120u, 0x00B23DE0u, 0x00AF127Du, 116); }

// void EmitPropsRef_Release_0079CA4C(AglEmitProperties** slot): drops a
// reference (s16 +0); at zero EmitProps_Destruct(p) and p goes back to the
// AglEmitProperties free list (get_list10: 0x00F8F128, 140-byte entries)
// through its virtual slot 4. The slot itself is not cleared. (name?)
DECOMP(0079CA4C, EmitPropsRef_Release_0079CA4C) {
    const u32 p = load<u32>(arg<u32>(ctx, 0));
    if (p == 0) return;  // r3 stays the slot
    const u32 n = load<u16>(p) - 1u;
    store<u16>(p, static_cast<u16>(n));
    if (static_cast<s16>(n) > 0) return ret(ctx, n);  // r3 as left
    call(ctx, 0x004FFA98u /* EmitProps_Destruct */, {p});
    const u32 fl = get_list10(ctx, 0x00F8F128u, 0x00B23E80u, 0x00AF1407u, 140);
    fx::vcall(ctx, fl, 16, {fl, p});
}

// void StreamBuffers_Flip_0050FE10(): the double-buffered stream state
// (*0x010DDBBC): index +228 flips; the new buffer pair (parts +128 / +160
// for 0, +144 / +180 for 1) gets bits 1..3 of each part's +8 = 2, +232 =
// the first part's data, +236 / +244 = the second's, +240 = +232; then
// +384 = +232 + the size +200 and +512 = +244 + +204, each stored
// atomically (lwarx / stwcx.)  [called by f_00510BFC] (name?)
DECOMP(0050FE10, StreamBuffers_Flip_0050FE10) {
    const u32 s = load<u32>(0x010DDBBCu);
    const u32 idx = (load<u32>(s + 228) + 1) & 1;
    store<u32>(s + 228, idx);
    const u32 a = s + (idx == 0 ? 128u : 144u), b = s + (idx == 0 ? 160u : 180u);
    store<u32>(a + 8, (load<u32>(a + 8) & ~0xEu) | 0x4u);
    store<u32>(s + 232, load<u32>(a));
    store<u32>(b + 8, (load<u32>(b + 8) & ~0xEu) | 0x4u);
    const u32 q = load<u32>(b);
    const u32 p = load<u32>(s + 232);
    const u32 size = load<u32>(s + 200);
    store<u32>(s + 236, q);
    store<u32>(s + 244, q);
    store<u32>(s + 240, p);
    std::atomic_ref<u32>(ref<u32>(s + 384)).store(__builtin_bswap32(p + size));
    const u32 v = load<u32>(s + 244) + load<u32>(s + 204);
    std::atomic_ref<u32>(ref<u32>(s + 512)).store(__builtin_bswap32(v));
    ctx.gpr[3] = s + 512;  // left as the original leaves them
    ctx.gpr[4] = 0;
    ctx.gpr[5] = v;
}

// void Texture_Bind_0079DF78(Texture* t, u32 unit): binds t's
// CellGcmTexture (+72; the render state's cached slot +52[unit] holds
// t + 56) with cellGcmSetTexture / SetTextureControl (enable, mip levels
// from u8 +73), then cellGcmSetTextureFilter(context, unit, 0, the min
// filter state +225[3 * u16 +120 + u16 +118], the mag +222[u16 +118], 1)
// and TEXTURE_ADDRESS inline (0x00030000 | u8 +124 << 8 | u8 +122).
// (fx::bind_texture's twin.) [called through a pointer] (name?)
DECOMP(0079DF78, Texture_Bind_0079DF78) {
    const u32 t = arg<u32>(ctx, 0), unit = arg<u32>(ctx, 1);
    const u32 st = k10GcmState;
    const u32 slot = st + 52 + static_cast<u32>(static_cast<s32>(unit << 2));
    const u32 u8unit = unit & 0xFF;
    if (load<u32>(slot) != t + 56) {
        store<u32>(slot, t + 56);
        call(ctx, draw::fn::cellGcmSetTexture, {load<u32>(st), u8unit, t + 72});
        const u32 levels = load<u8>(t + 73);
        call(ctx, draw::fn::cellGcmSetTextureControl, {load<u32>(st), u8unit, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
    }
    const u32 f118 = load<u16>(t + 118), f120 = load<u16>(t + 120);
    const u32 mag = load<u8>(st + 222 + f118);
    const u32 min = load<u8>(st + 225 + 3 * f120 + f118);
    call(ctx, draw::fn::cellGcmSetTextureFilter, {load<u32>(st), u8unit, 0, min, mag, 1});
    const u32 gcm = load<u32>(st);
    const u32 cur = load<u32>(gcm + 8);
    const u32 method = (((unit << 5) & 0x1FE0u) + 0x1A08u) | 0x00040000u;  // TEXTURE_ADDRESS
    store<u32>(cur, method);
    store<u32>(cur + 4, 0x00030000u | ((load<u16>(t + 124) & 0xFFu) << 8) | (load<u16>(t + 122) & 0xFFu));
    store<u32>(gcm + 8, cur + 8);
    ctx.gpr[3] = method;  // r3 as left
}

// bool Iff_HandleForce_00528F20(IffReader* r, Obj* o): vec3 "force"; a new
// 48-byte force record (vtable 0x00B23260, u16 refcount +4 = 1, +16..
// +32 cleared; null when the allocation fails) replaces o->+120 (new
// one's count up, the old one's down and released through its virtual
// slot 1 (+4) at zero), the creation reference dropped (no release
// check), +16..+24 = force, +32 = 0 (written through a null record when
// the allocation failed). Returns 1. (name?)
DECOMP(00528F20, Iff_HandleForce_00528F20) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 b = frame.addr();
    store<u32>(b, 0);
    store<u32>(b + 4, 0);
    store<u32>(b + 8, 0);
    call(ctx, g10::fn::FileReader_ReadVec3Swapped, {r, b, 0x00B23B9Fu});  // "force"
    u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {48, 0, 16}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p + 24, 0);
        store<u32>(p, 0x00B23260u);
        store<u32>(p + 20, 0);
        store<u32>(p + 16, 0);
        store<u32>(p + 32, 0);
    }
    const u32 old = load<u32>(o + 120);
    if (old != p) {
        store<u32>(o + 120, p);
        if (p != 0) store<u16>(p + 4, static_cast<u16>(load<u16>(p + 4) + 1));
        if (old != 0) {
            const u32 n = load<u16>(old + 4) - 1u;
            store<u16>(old + 4, static_cast<u16>(n));
            if (static_cast<s16>(n) <= 0) fx::vcall(ctx, old, 4, {old});
        }
    }
    u32 q = load<u32>(o + 120);
    store<u16>(q + 4, static_cast<u16>(load<u16>(q + 4) - 1));
    q = load<u32>(o + 120);
    for (u32 k = 0; k < 12; k += 4) store<u32>(q + 16 + k, load<u32>(b + k));  // lfs / stfs: raw
    store<u32>(load<u32>(o + 120) + 32, 0);
    ctx.fpr[1] = g10::lfs_value(load<u32>(b));  // f1..f3 as left: the force
    ctx.fpr[2] = g10::lfs_value(load<u32>(b + 4));
    ctx.fpr[3] = g10::lfs_value(load<u32>(b + 8));
    ret(ctx, 1u);
}

// bool LoadParams_005BB23C(ShaderType*, Material* m, const Desc* d) [a
// ShaderType's slot 1]: the loaded texture named by d->+16 (none: 0) gets
// virtual slot 4 (+16) (tex, 1, 1, 1) and f_004FF008(m, tex); the top
// bits of m->+120 become 2 << 28 when u8 d->+76 is 2, else 3 << 28; then
// material parameters 0..3 = d->+88..+100. Returns 1. (gfx9's
// SelfLight_LoadParams with ProjTxr_LoadParams' texture step.) (name?)
DECOMP(005BB23C, LoadParams_005BB23C) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 tex = static_cast<u32>(call(ctx, 0x0050C398u /* Tex_FindLoaded */, {load<u32>(d + 16)}));
    if (tex == 0) return ret(ctx, 0u);
    fx::vcall(ctx, tex, 16, {tex, 1, 1, 1});
    call(ctx, 0x004FF008u, {m, tex});
    const u32 top = load<u8>(d + 76) == 2 ? 2u : 3u;
    store<u32>(m + 120, (load<u32>(m + 120) & 0x0FFFFFFFu) | ((top << 28) & 0x30000000u));
    for (u32 k = 0; k < 4; ++k) {
        const u32 v = load<u32>(d + 88 + 4 * k);  // lfs / stfs: raw
        store<u32>(static_cast<u32>(call(ctx, 0x0050C018u /* Material_ParamSlot */, {m, k})), v);
    }
    ret(ctx, 1u);
}

// void VertexFormat_Init_0052CB34(Obj* o): the shared vertex format at
// 0x010DD500 (u16 +120 stride = 12, u16 +124 bit 0x8000 set, u16 +126 the
// attribute mask) copied from the 12-byte template entries at 0x00F66F98
// ({u32 index, u32 size (0 ends), u8 [3]}, the terminator too), each
// adding 1 << (index + byte 9) (slw) to the mask; then virtual slot 2
// (+8) (child, o) on each of o's u8 +20 children (+4..)  [called by
// f_0052DB58] (name?)
DECOMP(0052CB34, VertexFormat_Init_0052CB34) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kFmt = 0x010DD500, kTemplate = 0x00F66F98;
    const u32 flags = load<u16>(kFmt + 124) | 0x8000u;
    store<u16>(kFmt + 120, 12);
    store<u16>(kFmt + 126, 0);
    store<u16>(kFmt + 124, static_cast<u16>(flags));
    const u32 n = load<u8>(o + 20);
    u32 s = kTemplate, d = kFmt;
    for (;; s += 12, d += 12) {
        const u32 size = load<u32>(s + 4), index = load<u32>(s);
        store<u32>(d + 4, size);
        store<u32>(d, index);
        store<u8>(d + 8, load<u8>(s + 8));
        const u32 b9 = load<u8>(s + 9);
        store<u8>(d + 9, static_cast<u8>(b9));
        store<u8>(d + 10, load<u8>(s + 10));
        if (size == 0) break;
        const u32 sh = (index + b9) & 63;
        store<u16>(kFmt + 126, static_cast<u16>(load<u16>(kFmt + 126) | (sh < 32 ? 1u << sh : 0u)));
    }
    if (n == 0) return ret(ctx, static_cast<u32>(load<u8>(d + 10)));  // r3 as left: the last byte copied
    u32 i = 0;
    do {  // cmplw, the count re-read
        const u32 child = load<u32>(o + 4 + 4 * i);
        fx::vcall(ctx, child, 8, {child, o});
        ++i;
    } while (i < load<u8>(o + 20));
    ret(ctx, o + 4 * i);  // r3 as left: the slot pointer
}

// void ShadowMap_LoadPrograms_0052DB58(): once (u8 0x00F671D4): the
// shadow programs: vertex 0x010F0AD0 / 0x010F0AD4 = "ShadowMap_Depth" /
// "ShadowMap_Depth_SK" (names at 0x00F66FEC.., parameters 0x00F66FDC),
// fragment 0x010F0AD8 = "ShadowMap_Depth", vertex 0x010F0ADC = the first
// of the 15 names at 0x00F6703C ("ShadowMap_DeferredShadow", parameters
// 0x00F66FF4), and fragment 0x010F0AE0[i] from each of those 15 names
// (parameters 0x00F6700C; a null name leaves a null program)  [called by
// Shaders_ClampRegisterCounts] (name?)
DECOMP(0052DB58, ShadowMap_LoadPrograms_0052DB58) {
    constexpr u32 kOnce = 0x00F671D4, kNames = 0x00F66FEC, kDeferred = 0x00F6703C;
    if (load<u8>(kOnce) != 0) return ret(ctx, static_cast<u32>(load<u8>(kOnce)));
    store<u8>(kOnce, 1);
    call(ctx, 0x00534074u /* VProgram_Load */, {0x010F0AD0u, load<u32>(kNames), 0x00F66FDCu});
    call(ctx, 0x00534074u /* VProgram_Load */, {0x010F0AD4u, load<u32>(kNames + 4), 0x00F66FDCu});
    call(ctx, 0x00533DF8u /* FProgram_Load */, {0x010F0AD8u, load<u32>(kNames), 0});
    call(ctx, 0x00534074u /* VProgram_Load */, {0x010F0ADCu, load<u32>(kDeferred), 0x00F66FF4u});
    for (u32 i = 0; i < 15; ++i) {
        const u32 name = load<u32>(kDeferred + 4 * i);
        if (name == 0) {
            store<u32>(0x010F0AE0u + 4 * i, 0);
            ctx.gpr[3] = 0;  // r3 as left
        } else {
            call(ctx, 0x00533DF8u /* FProgram_Load */, {0x010F0AE0u + 4 * i, name, 0x00F6700Cu});
        }
    }
}
