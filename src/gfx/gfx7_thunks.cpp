// The renderer range's thunks and other short functions: tail wrappers,
// the typed free lists' Alloc / Free / Init slots, destructors that reset
// the vtable and call the base, once-flags, u32 sort compares and a few
// setters. Names carry the address where the class is not known yet.
#include <bit>

#include "gfx7.h"

using namespace sly4;

namespace {

constexpr u32 kfn_FreeList_Alloc = 0x00556C04;  // common/freelist.cpp
constexpr u32 kfn_FreeList_Free = 0x00556D18;
constexpr u32 kfn_FreeList_Init = 0x00551A58;   // (list, elem_size, count, heap, bool)
constexpr u32 kfn_free = 0x0072A8FC;
constexpr u32 kfn_malloc = 0x0072A928;
constexpr u32 kList010E05D4 = 0x010E05D4;       // u32[]: PushGlobal_00527A48's list (name?)
constexpr u32 kList010E05D4Count = 0x010E0614;  // u32 its count (name?)
constexpr u32 kFlag010DDB84 = 0x010DDB84;       // u8 (name?)
constexpr u32 kF60Min = 0x00520334;             // .text f32: View_Set520338's floor

}  // namespace

// void (this, u16 a, u16 b, u16 c): this->u16 +122 / +124 / +126 = a / b / c, +128 |= 2  [vtable 00B23380[4], 00B23EE0[4]]
DECOMP(007A0E98, Texture_SetWrap) {
    const u32 self = arg<u32>(ctx, 0);
    const u16 flags = static_cast<u16>(load<u16>(self + 128) | 2);  // lhz before the sth's
    store<u16>(self + 122, arg<u16>(ctx, 1));
    store<u16>(self + 124, arg<u16>(ctx, 2));
    store<u16>(self + 126, arg<u16>(ctx, 3));
    store<u16>(self + 128, flags);
    ctx.gpr[7] = flags;
}

// tail wrapper: f_00500798(the same arguments)  [called directly]
DECOMP(005008D8, Thunk_005008D8) {
    call(ctx, 0x00500798u);  // arguments and results passed through
}

// tail wrapper: f_0052DA10(the same arguments)  [called directly]
DECOMP(0052DACC, Thunk_0052DACC) {
    call(ctx, 0x0052DA10u);  // arguments and results passed through
}

// tail wrapper: Draw_RestoreDepth(the same arguments)  [called directly]
DECOMP(0053C830, Thunk_0053C830) {
    call(ctx, 0x00535294u);  // arguments and results passed through
}

// tail wrapper: Draw_PrepareQuad(the same arguments)  [called directly]
DECOMP(0053C850, Thunk_0053C850) {
    call(ctx, 0x0053539Cu);  // arguments and results passed through
}

// u32 (): once-flag u8 at 0x00F679FC: when clear, set it (r3 = 0x00F60000); when set, return it  [vtable 00B23F60[6]]
DECOMP(0053E164, InitOnce_0053E164) {
    const u8 v = load<u8>(0x00F679FCu);
    if (v != 0) return ret(ctx, static_cast<u32>(v));
    store<u8>(0x00F679FCu, 1);
    ctx.gpr[3] = 0x00F60000u;  // lis r3
    ctx.gpr[4] = 1;
}

// u32 (): once-flag u8 at 0x00F679FE: when clear, set it (r3 = 0x00F60000); when set, return it  [vtable 00B23F90[6]]
DECOMP(0053F874, InitOnce_0053F874) {
    const u8 v = load<u8>(0x00F679FEu);
    if (v != 0) return ret(ctx, static_cast<u32>(v));
    store<u8>(0x00F679FEu, 1);
    ctx.gpr[3] = 0x00F60000u;  // lis r3
    ctx.gpr[4] = 1;
}

// u32 (): once-flag u8 at 0x00F679FF: when clear, set it (r3 = 0x00F60000); when set, return it  [vtable 00B23FC0[7]]
DECOMP(00540DA4, InitOnce_00540DA4) {
    const u8 v = load<u8>(0x00F679FFu);
    if (v != 0) return ret(ctx, static_cast<u32>(v));
    store<u8>(0x00F679FFu, 1);
    ctx.gpr[3] = 0x00F60000u;  // lis r3
    ctx.gpr[4] = 1;
}

// tail wrapper: Draw_RestoreDepth(the same arguments)  [vtable 00B23FF0[13]]
DECOMP(0054338C, Thunk_0054338C) {
    call(ctx, 0x00535294u);  // arguments and results passed through
}

// tail wrapper: f_00548D44(the same arguments)  [called directly]
DECOMP(00549140, Thunk_00549140) {
    call(ctx, 0x00548D44u);  // arguments and results passed through
}

// tail wrapper: f_005AF298(the same arguments)  [vtable 00B250A0[1]]
DECOMP(005AF4EC, Thunk_005AF4EC) {
    call(ctx, 0x005AF298u);  // arguments and results passed through
}

// tail wrapper: f_005D20E0(the same arguments)  [vtable 00B25FB8[1]]
DECOMP(005D2334, Thunk_005D2334) {
    call(ctx, 0x005D20E0u);  // arguments and results passed through
}

// tail wrapper: f_005D7DE8(the same arguments)  [vtable 00B26148[1]]
DECOMP(005D803C, Thunk_005D803C) {
    call(ctx, 0x005D7DE8u);  // arguments and results passed through
}

// void (Buffer* b, u8* data, u32 size, u32 x): {+0 data, +4 data + (size & ~3) - 4, +8 data, +12 x}  [called directly]
DECOMP(005DF960, Buffer_Init_005DF960) {
    const u32 b = arg<u32>(ctx, 0);
    const u64 data = ctx.gpr[4];
    const u64 last = (ctx.gpr[5] & 0xFFFFFFFCu) + data - 4;  // rlwinm 0,0,29 / add / addi
    store<u32>(b + 12, arg<u32>(ctx, 3));
    store<u32>(b + 0, static_cast<u32>(data));
    store<u32>(b + 8, static_cast<u32>(data));
    store<u32>(b + 4, static_cast<u32>(last));
    ctx.gpr[5] = last;
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B24418[3]]
DECOMP(007A4AD0, PrimFreeList_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B24418[4]]
DECOMP(007A4AF0, PrimFreeList_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B24438[3]]
DECOMP(007A4B10, ChunkFreeList_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B24438[4]]
DECOMP(007A4B30, ChunkFreeList_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// tail wrapper: Iff_ReadChunks(the same arguments)  [called directly]
DECOMP(007A50B4, Thunk_007A50B4) {
    call(ctx, 0x00569388u);  // arguments and results passed through
}

// void (this): *cursor = {-1, 0}, cursor += 8 (cursor at this->+8)  [called directly]
DECOMP(0051300C, PushMarker_0051300C) {
    const u32 self = arg<u32>(ctx, 0);
    store<u32>(load<u32>(self + 8), 0xFFFFFFFFu);
    const u32 cur = load<u32>(self + 8);  // re-read after the store
    store<u32>(cur + 4, 0);
    store<u32>(self + 8, cur + 8);
    ctx.gpr[4] = cur;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = static_cast<u64>(cur) + 8;
}

// void (u32 v): list[count++] = v (u32 list at 0x010E05D4, count 0x010E0614; unchecked)  [called directly]
DECOMP(00527A48, PushGlobal_00527A48) {
    const u32 n = load<u32>(kList010E05D4Count);
    store<u32>(kList010E05D4Count, n + 1);
    store<u32>(kList010E05D4 + (n << 2), arg<u32>(ctx, 0));  // stwx, no bounds check
    ctx.gpr[4] = 0x010E0000u;
    ctx.gpr[5] = kList010E05D4;
    ctx.gpr[6] = static_cast<u32>(n << 2);
    ctx.gpr[7] = static_cast<u64>(n) + 1;
}

// tail wrapper: f_00527484(the same arguments)  [called directly]
DECOMP(0054BF24, Thunk_0054BF24) {
    call(ctx, 0x00527484u);  // arguments and results passed through
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B23D80[3]]
DECOMP(0079E9F4, FreeList23D80_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B23D80[4]]
DECOMP(0079EA18, FreeList23D80_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B23DA0[3]]
DECOMP(0079EA3C, FreeList23DA0_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B23DA0[4]]
DECOMP(0079EA60, FreeList23DA0_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B23DE0[3]]
DECOMP(0079EACC, FreeList23DE0_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B23DE0[4]]
DECOMP(0079EAF0, FreeList23DE0_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B23E00[3]]
DECOMP(0079EB14, FreeList23E00_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B23E00[4]]
DECOMP(0079EB38, FreeList23E00_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B23E20[3]]
DECOMP(0079EB5C, FreeList23E20_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B23E20[4]]
DECOMP(0079EB80, FreeList23E20_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B23E40[3]]
DECOMP(0079EBA4, FreeList23E40_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B23E40[4]]
DECOMP(0079EBC8, FreeList23E40_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B23E60[3]]
DECOMP(0079EBEC, FreeList23E60_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B23E60[4]]
DECOMP(0079EC10, FreeList23E60_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B23E80[3]]
DECOMP(0079EC34, FreeList23E80_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B23E80[4]]
DECOMP(0079EC58, FreeList23E80_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B23EA0[3]]
DECOMP(0079EC7C, DeferredFreeList_Alloc) {
    ret(ctx, call(ctx, kfn_FreeList_Alloc, {ctx.gpr[3]}));
}

// void (FreeList*, T*): FreeList_Free  [vtable 00B23EA0[4]]
DECOMP(0079ECA0, DeferredFreeList_Free) {
    ret(ctx, call(ctx, kfn_FreeList_Free, {ctx.gpr[3], ctx.gpr[4]}));
}

// tail wrapper: f_0060D980(the same arguments)  [vtable 00B245A4[6]]
DECOMP(007A67B8, Thunk_007A67B8) {
    call(ctx, 0x0060D980u);  // arguments and results passed through
}

// void (this, f32 v): this->+60 = v - k >= 0 ? v : k (fsel on v - k; k = 0x00520334), this->+52 |= 0x80  [called directly]
DECOMP(00520338, View_Set520338) {
    const u32 self = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0), k = static_cast<f64>(load<f32>(kF60Min));
    const u8 flags = static_cast<u8>(load<u8>(self + 52) | 0x80);
    store<u8>(self + 52, flags);
    const f64 r = static_cast<f64>(static_cast<f32>(v - k)) >= 0.0 ? v : k;  // fsubs, fsel
    store<f32>(self + 60, static_cast<f32>(r));
    ctx.gpr[4] = flags;
    ctx.fpr[1] = r;
}

// void (x): 00569638(x, a 256-byte stack buffer)  [called directly]
DECOMP(00525E18, WithBuffer_00525E18) {
    Stack buf(ctx, 256);  // 112(r1) of a 368-byte frame
    call(ctx, 0x00569638u, {ctx.gpr[3], buf.addr()});
}

// void (this, void* p): free(p)  [vtable 00F48844[6]]
DECOMP(005DEE84, SysAllocator_Free) {
    call(ctx, kfn_free, {ctx.gpr[4]});
}

// void (this, void* p): free(p)  [vtable 00B245D8[1]]
DECOMP(007A5F54, FiosAllocator_Free) {
    call(ctx, kfn_free, {ctx.gpr[4]});
}

// void (this, x): 0060D9C4(x)  [vtable 00B245A4[7]]
DECOMP(007A67DC, Call60D9C4_007A67DC) {
    call(ctx, 0x0060D9C4u, {ctx.gpr[4]});
}

// void (this): this->vtable = 0x00B233D0 (shadergroup.h vt_material), then f_0050C118(this)  [vtable 00B233D0[0]]
DECOMP(007A0F64, Material_Destruct) {
    store<u32>(arg<u32>(ctx, 0), 0x00B233D0u);
    ctx.gpr[4] = 0x00B233D0u;  // still in r4 at the call
    call(ctx, 0x0050C118u);
}

// void (this): this->vtable = 0x00B24298 (common.h kFreeListBaseVtbl), then FreeList_Destroy(this)  [vtable 00B24298[0]]
DECOMP(007A4C68, FreeListBase_Destruct) {
    store<u32>(arg<u32>(ctx, 0), 0x00B24298u);
    ctx.gpr[4] = 0x00B24298u;  // still in r4 at the call
    call(ctx, 0x005568FCu);
}

// void (this): this->vtable = 0x00B23EE0 (font/image.h kTexVtbl), then f_0050A5E4(this)  [vtable 00B23EE0[0]]
DECOMP(0053A2C4, Texture_DestructBase) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23EE0u);
    ctx.gpr[4] = 0x00B23EE0u;  // still in r4 at the call
    call(ctx, 0x0050A5E4u);
}

// void (this): this->vtable = 0x00B23F20 (surface_alloc.h vt_render_surface), then f_0051AD90(this)  [vtable 00B23F20[0]]
DECOMP(0053C870, RenderSurface_Destruct) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23F20u);
    ctx.gpr[4] = 0x00B23F20u;  // still in r4 at the call
    call(ctx, 0x0051AD90u);
}

// void (): Nop_00520028(), then the u8 at 0x010DDB84 = 0  [called directly]
DECOMP(00546F3C, Call520028_00546F3C) {
    call(ctx, 0x00520028u);
    store<u8>(kFlag010DDB84, 0);
    ctx.gpr[3] = 0x010E0000u;
    ctx.gpr[4] = 0;
}

// void (this): vtable 0x00B24130; u16 +28..+34, u32 +644 / +648, u8 +652 = 0  [called directly]
DECOMP(00548D14, Ctor_00548D14) {
    const u32 self = arg<u32>(ctx, 0);
    for (u32 off = 28; off <= 34; off += 2) store<u16>(self + off, 0);
    store<u32>(self, 0x00B24130u);
    store<u32>(self + 644, 0);
    store<u32>(self + 648, 0);
    store<u8>(self + 652, 0);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0x00B24130u;
}

// void* (this, u32 n, u32 size): malloc(n * size)  [vtable 00F48844[5]]
DECOMP(005DEEAC, SysAllocator_Alloc) {
    const u32 bytes = static_cast<u32>(ctx.gpr[4] * ctx.gpr[5]);  // mullw, rldicl 0,32
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(static_cast<s32>(ctx.gpr[4]) * static_cast<s64>(static_cast<s32>(ctx.gpr[5]))));
    ret(ctx, static_cast<u32>(call(ctx, kfn_malloc, {bytes})));
}

// void (this): this->vtable = 0x00B23D28 (name?), then f_00549140(this)  [vtable 00B23D28[0]]
DECOMP(0079E9B4, Destruct_0079E9B4) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23D28u);
    ctx.gpr[4] = 0x00B23D28u;  // still in r4 at the call
    call(ctx, 0x00549140u);
}

// s32 (const u32* a, const u32* b): 1 / 0 / -1 for *a > / == / < *b (unsigned; a sort compare)  [called directly]
DECOMP(007A2218, CompareU32_007A2218) {
    const u32 x = load<u32>(arg<u32>(ctx, 0)), y = load<u32>(arg<u32>(ctx, 1));
    const s32 r = x > y ? 1 : (x < y ? -1 : 0);  // cmplw
    ctx.gpr[6] = ctx.gpr[4];
    ctx.gpr[4] = x;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(r));
    ret(ctx, r);
}

// s32 (const u32* a, const u32* b): 1 / 0 / -1 for *a > / == / < *b (unsigned; a sort compare)  [called directly]
DECOMP(007A2248, CompareU32_007A2248) {
    const u32 x = load<u32>(arg<u32>(ctx, 0)), y = load<u32>(arg<u32>(ctx, 1));
    const s32 r = x > y ? 1 : (x < y ? -1 : 0);  // cmplw
    ctx.gpr[6] = ctx.gpr[4];
    ctx.gpr[4] = x;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(r));
    ret(ctx, r);
}

// void (this): this->vtable = 0x00B240D0 (load.h kRenderTargetVtbl), then f_0052163C(this)  [vtable 00B240D0[0]]
DECOMP(007A319C, RenderTarget_Destruct) {
    store<u32>(arg<u32>(ctx, 0), 0x00B240D0u);
    ctx.gpr[4] = 0x00B240D0u;  // still in r4 at the call
    call(ctx, 0x0052163Cu);
}

// s32 (const u32* a, const u32* b): 1 / 0 / -1 for *a > / == / < *b (unsigned; a sort compare)  [called directly]
DECOMP(007A38FC, CompareU32_007A38FC) {
    const u32 x = load<u32>(arg<u32>(ctx, 0)), y = load<u32>(arg<u32>(ctx, 1));
    const s32 r = x > y ? 1 : (x < y ? -1 : 0);  // cmplw
    ctx.gpr[6] = ctx.gpr[4];
    ctx.gpr[4] = x;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(r));
    ret(ctx, r);
}

// s32 (const u32* a, const u32* b): 1 / 0 / -1 for *a > / == / < *b (unsigned; a sort compare)  [called directly]
DECOMP(007A392C, CompareU32_007A392C) {
    const u32 x = load<u32>(arg<u32>(ctx, 0)), y = load<u32>(arg<u32>(ctx, 1));
    const s32 r = x > y ? 1 : (x < y ? -1 : 0);  // cmplw
    ctx.gpr[6] = ctx.gpr[4];
    ctx.gpr[4] = x;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(r));
    ret(ctx, r);
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 124, count, heap, 0)  [vtable 00B24418[5]]
DECOMP(007A4C38, PrimFreeList_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 124, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 20, count, heap, 0)  [vtable 00B24438[5]]
DECOMP(007A4CEC, ChunkFreeList_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 20, ctx.gpr[4], ctx.gpr[5], 0}));
}

// s32 (const u32* a, const u32* b): 1 / 0 / -1 for *a > / == / < *b (unsigned; a sort compare)  [called directly]
DECOMP(007A4D68, CompareU32_007A4D68) {
    const u32 x = load<u32>(arg<u32>(ctx, 0)), y = load<u32>(arg<u32>(ctx, 1));
    const s32 r = x > y ? 1 : (x < y ? -1 : 0);  // cmplw
    ctx.gpr[6] = ctx.gpr[4];
    ctx.gpr[4] = x;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(r));
    ret(ctx, r);
}

// s32 (const u32* a, const u32* b): 1 / 0 / -1 for *a > / == / < *b (unsigned; a sort compare)  [called directly]
DECOMP(007A4E50, CompareU32_007A4E50) {
    const u32 x = load<u32>(arg<u32>(ctx, 0)), y = load<u32>(arg<u32>(ctx, 1));
    const s32 r = x > y ? 1 : (x < y ? -1 : 0);  // cmplw
    ctx.gpr[6] = ctx.gpr[4];
    ctx.gpr[4] = x;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(r));
    ret(ctx, r);
}
