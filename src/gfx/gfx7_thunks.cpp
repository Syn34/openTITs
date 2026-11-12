// The renderer range's thunks and other short functions: tail wrappers,
// the typed free lists' Alloc / Free / Init slots, destructors that reset
// the vtable and call the base, once-flags, u32 sort compares and a few
// setters. Names carry the address where the class is not known yet.
// The tail wrappers pass on the argument registers their callee reads
// and return its r3.
#include <bit>

#include "gfx7.h"
#include "rsx.h"

namespace sly4::gfx {

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

be_u32& word(const void* p, u32 off) { return *at<be_u32>(p, off); }

// A once-flag: when clear, set it (r3 = 0x00F60000, the lis); when set, return it.
u32 init_once(u32 flag) {
    const u8 v = global<u8>(flag);
    if (v != 0) return v;
    global<u8>(flag) = 1;
    return 0x00F60000u;
}

// The vtable reset and the base destructor.
void destruct(u8* self, u32 vtable, u32 base) {
    word(self, 0) = vtable;
    call(base, self);
}

// 1 / 0 / -1 for *a > / == / < *b, unsigned.
s32 compare_u32(const be_u32* a, const be_u32* b) {
    const u32 x = *a, y = *b;
    return x > y ? 1 : (x < y ? -1 : 0);  // cmplw
}

}  // namespace

// void (this, u16 a, u16 b, u16 c): this->u16 +122 / +124 / +126 = a / b / c, +128 |= 2  [vtable 00B23380[4], 00B23EE0[4]]
void Texture_SetWrap(u8* self, u32 a, u32 b, u32 c) {
    const u16 flags = static_cast<u16>(*at<be_u16>(self, 128) | 2);  // lhz before the sth's
    *at<be_u16>(self, 122) = static_cast<u16>(a);
    *at<be_u16>(self, 124) = static_cast<u16>(b);
    *at<be_u16>(self, 126) = static_cast<u16>(c);
    *at<be_u16>(self, 128) = flags;
}

// tail wrapper: f_00500798(r3, r4)  [called directly]
u32 Thunk_005008D8(u32 a, u32 b) { return call<u32>(0x00500798u, a, b); }

// tail wrapper: f_0052DA10(r3)  [called directly]
u32 Thunk_0052DACC(u32 a) { return call<u32>(0x0052DA10u, a); }

// tail wrapper: Draw_RestoreDepth()  [called directly]
void Thunk_0053C830() { Draw_RestoreDepth(); }

// tail wrapper: Draw_PrepareQuad()  [called directly]
void Thunk_0053C850() { Draw_PrepareQuad(); }

// u32 (): once-flag u8 at 0x00F679FC  [vtable 00B23F60[6]]
u32 InitOnce_0053E164() { return init_once(0x00F679FCu); }

// u32 (): once-flag u8 at 0x00F679FE  [vtable 00B23F90[6]]
u32 InitOnce_0053F874() { return init_once(0x00F679FEu); }

// u32 (): once-flag u8 at 0x00F679FF  [vtable 00B23FC0[7]]
u32 InitOnce_00540DA4() { return init_once(0x00F679FFu); }

// tail wrapper: Draw_RestoreDepth()  [vtable 00B23FF0[13]]
void Thunk_0054338C() { Draw_RestoreDepth(); }

// tail wrapper: f_00548D44(r3)  [called directly]
u32 Thunk_00549140(u32 a) { return call<u32>(0x00548D44u, a); }

// tail wrapper: f_005AF298(r3, r4, r5)  [vtable 00B250A0[1]]
u32 Thunk_005AF4EC(u32 a, u32 b, u32 c) { return call<u32>(0x005AF298u, a, b, c); }

// tail wrapper: f_005D20E0(r3, r4, r5)  [vtable 00B25FB8[1]]
u32 Thunk_005D2334(u32 a, u32 b, u32 c) { return call<u32>(0x005D20E0u, a, b, c); }

// tail wrapper: f_005D7DE8(r3, r4, r5)  [vtable 00B26148[1]]
u32 Thunk_005D803C(u32 a, u32 b, u32 c) { return call<u32>(0x005D7DE8u, a, b, c); }

// void (Buffer* b, u8* data, u32 size, u32 x): {+0 data, +4 data + (size & ~3) - 4, +8 data, +12 x}  [called directly]
void Buffer_Init_005DF960(be_u32* b, u32 data, u32 size, u32 x) {
    const u32 last = (size & 0xFFFFFFFCu) + data - 4;  // rlwinm 0,0,29 / add / addi
    b[3] = x;
    b[0] = data;
    b[2] = data;
    b[1] = last;
}

// T* (FreeList*): FreeList_Alloc  [vtable 00B24418[3]]
u32 PrimFreeList_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }
// u32 (FreeList*, T*): FreeList_Free  [vtable 00B24418[4]]
u32 PrimFreeList_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }
// T* (FreeList*): FreeList_Alloc  [vtable 00B24438[3]]
u32 ChunkFreeList_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }
// u32 (FreeList*, T*): FreeList_Free  [vtable 00B24438[4]]
u32 ChunkFreeList_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }

// tail wrapper: Iff_ReadChunks(r3, r4)  [called directly]
u32 Thunk_007A50B4(u32 a, u32 b) { return call<u32>(0x00569388u, a, b); }

// void (this): *cursor = {-1, 0}, cursor += 8 (cursor at this->+8)  [called directly]
void PushMarker_0051300C(u8* self) {
    *at<be_u32>(nullptr, word(self, 8)) = 0xFFFFFFFFu;
    const u32 cur = word(self, 8);  // re-read after the store
    *at<be_u32>(nullptr, cur + 4) = 0u;
    word(self, 8) = cur + 8;
}

// void (u32 v): list[count++] = v (u32 list at 0x010E05D4, count 0x010E0614; unchecked)  [called directly]
void PushGlobal_00527A48(u32 v) {
    const u32 n = global<be_u32>(kList010E05D4Count);
    global<be_u32>(kList010E05D4Count) = n + 1;
    global<be_u32>(kList010E05D4 + (n << 2)) = v;  // stwx, no bounds check
}

// tail wrapper: f_00527484()  [called directly]
u32 Thunk_0054BF24() { return call<u32>(0x00527484u); }

// The typed free lists' Alloc / Free slots (FreeList_Alloc / FreeList_Free).
u32 FreeList23D80_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }       // [vtable 00B23D80[3]]
u32 FreeList23D80_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }  // [vtable 00B23D80[4]]
u32 FreeList23DA0_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }       // [vtable 00B23DA0[3]]
u32 FreeList23DA0_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }  // [vtable 00B23DA0[4]]
u32 FreeList23DE0_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }       // [vtable 00B23DE0[3]]
u32 FreeList23DE0_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }  // [vtable 00B23DE0[4]]
u32 FreeList23E00_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }       // [vtable 00B23E00[3]]
u32 FreeList23E00_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }  // [vtable 00B23E00[4]]
u32 FreeList23E20_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }       // [vtable 00B23E20[3]]
u32 FreeList23E20_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }  // [vtable 00B23E20[4]]
u32 FreeList23E40_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }       // [vtable 00B23E40[3]]
u32 FreeList23E40_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }  // [vtable 00B23E40[4]]
u32 FreeList23E60_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }       // [vtable 00B23E60[3]]
u32 FreeList23E60_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }  // [vtable 00B23E60[4]]
u32 FreeList23E80_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }       // [vtable 00B23E80[3]]
u32 FreeList23E80_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }  // [vtable 00B23E80[4]]
u32 DeferredFreeList_Alloc(u8* list) { return call<u32>(kfn_FreeList_Alloc, list); }       // [vtable 00B23EA0[3]]
u32 DeferredFreeList_Free(u8* list, u32 p) { return call<u32>(kfn_FreeList_Free, list, p); }  // [vtable 00B23EA0[4]]

// tail wrapper: f_0060D980()  [vtable 00B245A4[6]]
u32 Thunk_007A67B8() { return call<u32>(0x0060D980u); }

// void (this, f32 v): this->+60 = v - k >= 0 ? v : k (fsel on v - k; k = 0x00520334), this->+52 |= 0x80  [called directly]
void View_Set520338(u8* self, f32 v) {
    const f32 k = global<be_f32>(kF60Min);
    *at<u8>(self, 52) = static_cast<u8>(*at<u8>(self, 52) | 0x80);
    *at<be_f32>(self, 60) = static_cast<f32>(fsel(v - k, v, k));  // fsubs, fsel
}

// void (x): 00569638(x, a 256-byte stack buffer)  [called directly]
void WithBuffer_00525E18(u32 x) {
    GuestFrame buf(256);  // 112(r1) of a 368-byte frame
    call(0x00569638u, x, buf.as<u8>());
}

// void (this, void* p): free(p)  [vtable 00F48844[6]]
void SysAllocator_Free(void*, u32 p) { call(kfn_free, p); }

// void (this, void* p): free(p)  [vtable 00B245D8[1]]
void FiosAllocator_Free(void*, u32 p) { call(kfn_free, p); }

// void (this, x): 0060D9C4(x)  [vtable 00B245A4[7]]
void Call60D9C4_007A67DC(void*, u32 x) { call(0x0060D9C4u, x); }

// void (this): vtable 0x00B233D0 (shadergroup.h vt_material), then f_0050C118(this)  [vtable 00B233D0[0]]
void Material_Destruct(u8* self) { destruct(self, 0x00B233D0u, 0x0050C118u); }

// void (this): vtable 0x00B24298 (common.h kFreeListBaseVtbl), then FreeList_Destroy(this)  [vtable 00B24298[0]]
void FreeListBase_Destruct(u8* self) { destruct(self, 0x00B24298u, 0x005568FCu); }

// void (this): vtable 0x00B23EE0 (font/image.h kTexVtbl), then f_0050A5E4(this)  [vtable 00B23EE0[0]]
void Texture_DestructBase(u8* self) { destruct(self, 0x00B23EE0u, 0x0050A5E4u); }

// void (this): vtable 0x00B23F20 (surface_alloc.h vt_render_surface), then f_0051AD90(this)  [vtable 00B23F20[0]]
void RenderSurface_Destruct(u8* self) { destruct(self, 0x00B23F20u, 0x0051AD90u); }

// void (): Nop_00520028(), then the u8 at 0x010DDB84 = 0  [called directly]
void Call520028_00546F3C() {
    call(0x00520028u);
    global<u8>(kFlag010DDB84) = 0;
}

// void (this): vtable 0x00B24130; u16 +28..+34, u32 +644 / +648, u8 +652 = 0  [called directly]
void Ctor_00548D14(u8* self) {
    for (u32 off = 28; off <= 34; off += 2) *at<be_u16>(self, off) = 0;
    word(self, 0) = 0x00B24130u;
    word(self, 644) = 0u;
    word(self, 648) = 0u;
    *at<u8>(self, 652) = 0;
}

// void* (this, u32 n, u32 size): malloc(n * size)  [vtable 00F48844[5]]
u32 SysAllocator_Alloc(void*, u32 n, u32 size) { return call<u32>(kfn_malloc, n * size); }  // mullw, rldicl 0,32

// void (this): vtable 0x00B23D28 (name?), then f_00549140(this)  [vtable 00B23D28[0]]
void Destruct_0079E9B4(u8* self) { destruct(self, 0x00B23D28u, 0x00549140u); }

// s32 (const u32* a, const u32* b): the unsigned sort compares  [called directly]
s32 CompareU32_007A2218(const be_u32* a, const be_u32* b) { return compare_u32(a, b); }
s32 CompareU32_007A2248(const be_u32* a, const be_u32* b) { return compare_u32(a, b); }

// void (this): vtable 0x00B240D0 (load.h kRenderTargetVtbl), then f_0052163C(this)  [vtable 00B240D0[0]]
void RenderTarget_Destruct(u8* self) { destruct(self, 0x00B240D0u, 0x0052163Cu); }

s32 CompareU32_007A38FC(const be_u32* a, const be_u32* b) { return compare_u32(a, b); }
s32 CompareU32_007A392C(const be_u32* a, const be_u32* b) { return compare_u32(a, b); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 124, count, heap, 0)  [vtable 00B24418[5]]
u32 PrimFreeList_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 124u, count, heap, 0u); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 20, count, heap, 0)  [vtable 00B24438[5]]
u32 ChunkFreeList_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 20u, count, heap, 0u); }

s32 CompareU32_007A4D68(const be_u32* a, const be_u32* b) { return compare_u32(a, b); }
s32 CompareU32_007A4E50(const be_u32* a, const be_u32* b) { return compare_u32(a, b); }

}  // namespace sly4::gfx

NATIVE(007A0E98, sly4::gfx::Texture_SetWrap)
NATIVE(005008D8, sly4::gfx::Thunk_005008D8)
NATIVE(0052DACC, sly4::gfx::Thunk_0052DACC)
NATIVE(0053C830, sly4::gfx::Thunk_0053C830)
NATIVE(0053C850, sly4::gfx::Thunk_0053C850)
NATIVE(0053E164, sly4::gfx::InitOnce_0053E164)
NATIVE(0053F874, sly4::gfx::InitOnce_0053F874)
NATIVE(00540DA4, sly4::gfx::InitOnce_00540DA4)
NATIVE(0054338C, sly4::gfx::Thunk_0054338C)
NATIVE(00549140, sly4::gfx::Thunk_00549140)
NATIVE(005AF4EC, sly4::gfx::Thunk_005AF4EC)
NATIVE(005D2334, sly4::gfx::Thunk_005D2334)
NATIVE(005D803C, sly4::gfx::Thunk_005D803C)
NATIVE(005DF960, sly4::gfx::Buffer_Init_005DF960)
NATIVE(007A4AD0, sly4::gfx::PrimFreeList_Alloc)
NATIVE(007A4AF0, sly4::gfx::PrimFreeList_Free)
NATIVE(007A4B10, sly4::gfx::ChunkFreeList_Alloc)
NATIVE(007A4B30, sly4::gfx::ChunkFreeList_Free)
NATIVE(007A50B4, sly4::gfx::Thunk_007A50B4)
NATIVE(0051300C, sly4::gfx::PushMarker_0051300C)
NATIVE(00527A48, sly4::gfx::PushGlobal_00527A48)
NATIVE(0054BF24, sly4::gfx::Thunk_0054BF24)
NATIVE(0079E9F4, sly4::gfx::FreeList23D80_Alloc)
NATIVE(0079EA18, sly4::gfx::FreeList23D80_Free)
NATIVE(0079EA3C, sly4::gfx::FreeList23DA0_Alloc)
NATIVE(0079EA60, sly4::gfx::FreeList23DA0_Free)
NATIVE(0079EACC, sly4::gfx::FreeList23DE0_Alloc)
NATIVE(0079EAF0, sly4::gfx::FreeList23DE0_Free)
NATIVE(0079EB14, sly4::gfx::FreeList23E00_Alloc)
NATIVE(0079EB38, sly4::gfx::FreeList23E00_Free)
NATIVE(0079EB5C, sly4::gfx::FreeList23E20_Alloc)
NATIVE(0079EB80, sly4::gfx::FreeList23E20_Free)
NATIVE(0079EBA4, sly4::gfx::FreeList23E40_Alloc)
NATIVE(0079EBC8, sly4::gfx::FreeList23E40_Free)
NATIVE(0079EBEC, sly4::gfx::FreeList23E60_Alloc)
NATIVE(0079EC10, sly4::gfx::FreeList23E60_Free)
NATIVE(0079EC34, sly4::gfx::FreeList23E80_Alloc)
NATIVE(0079EC58, sly4::gfx::FreeList23E80_Free)
NATIVE(0079EC7C, sly4::gfx::DeferredFreeList_Alloc)
NATIVE(0079ECA0, sly4::gfx::DeferredFreeList_Free)
NATIVE(007A67B8, sly4::gfx::Thunk_007A67B8)
NATIVE(00520338, sly4::gfx::View_Set520338)
NATIVE(00525E18, sly4::gfx::WithBuffer_00525E18)
NATIVE(005DEE84, sly4::gfx::SysAllocator_Free)
NATIVE(007A5F54, sly4::gfx::FiosAllocator_Free)
NATIVE(007A67DC, sly4::gfx::Call60D9C4_007A67DC)
NATIVE(007A0F64, sly4::gfx::Material_Destruct)
NATIVE(007A4C68, sly4::gfx::FreeListBase_Destruct)
NATIVE(0053A2C4, sly4::gfx::Texture_DestructBase)
NATIVE(0053C870, sly4::gfx::RenderSurface_Destruct)
NATIVE(00546F3C, sly4::gfx::Call520028_00546F3C)
NATIVE(00548D14, sly4::gfx::Ctor_00548D14)
NATIVE(005DEEAC, sly4::gfx::SysAllocator_Alloc)
NATIVE(0079E9B4, sly4::gfx::Destruct_0079E9B4)
NATIVE(007A2218, sly4::gfx::CompareU32_007A2218)
NATIVE(007A2248, sly4::gfx::CompareU32_007A2248)
NATIVE(007A319C, sly4::gfx::RenderTarget_Destruct)
NATIVE(007A38FC, sly4::gfx::CompareU32_007A38FC)
NATIVE(007A392C, sly4::gfx::CompareU32_007A392C)
NATIVE(007A4C38, sly4::gfx::PrimFreeList_Init)
NATIVE(007A4CEC, sly4::gfx::ChunkFreeList_Init)
NATIVE(007A4D68, sly4::gfx::CompareU32_007A4D68)
NATIVE(007A4E50, sly4::gfx::CompareU32_007A4E50)
