// The renderer range's Delete slots (reset the vtable to the base, then
// free: Mem_FreeFrom / SmallFree / Mem_FreePool; or run the destructor,
// then free), the refcount Release slots (DeferredDestroy at zero) and
// the typed free lists' Init slots. A null this returns 0 untouched; the
// others return what their last call returns (a tail call).
// Names carry the address until the classes are known.
#include "gfx7.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 kfn_Mem_FreeFrom = 0x0054EE6C;     // (p, pool) (layers3.h)
constexpr u32 kfn_SmallFree = 0x00552420;        // (p, u32 size) (shader6.h)
constexpr u32 kfn_Mem_FreePool = 0x0056C730;     // (p, pool, tag) (layers4.h)
constexpr u32 kfn_DeferredDestroy = 0x005082F4;  // (obj, destroy OPD) (obj3.cpp)
constexpr u32 kfn_FreeList_Init = 0x00551A58;    // (list, elem_size, count, heap, bool)

// --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, opd).
// Returns this, or what DeferredDestroy returns.
u32 release(u8* self, u32 opd) {
    be_u16& refs = *at<be_u16>(self, 4);
    const u32 n = static_cast<u32>(refs.get()) - 1;  // lhz, addic -1
    refs = static_cast<u16>(n);
    if (static_cast<s16>(n) > 0) return guest(self);  // extsh
    return call<u32>(kfn_DeferredDestroy, self, opd);
}

// A Delete slot: when this is set, the vtable reset (vtable != 0) or the
// destructor run (dtor != 0), then the free call; returns 0 for a null
// this, else what the free returns.
template <typename... A>
u32 delete_with(u8* self, u32 vtable, u32 dtor, u32 free_fn, A... args) {
    if (!self) return 0;
    if (vtable) *at<be_u32>(self, 0) = vtable;
    if (dtor) call(dtor, self);
    return call<u32>(free_fn, self, args...);
}

}  // namespace

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 140, count, heap, 0)  [vtable 00B23E80[5]]
u32 FreeList23E80_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 140u, count, heap, 0u); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 12, count, heap, 0)  [vtable 00B23EA0[5]]
u32 DeferredFreeList_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 12u, count, heap, 0u); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 320, count, heap, 0)  [vtable 00B23E20[5]]
u32 FreeList23E20_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 320u, count, heap, 0u); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 112, count, heap, 0)  [vtable 00B23E00[5]]
u32 FreeList23E00_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 112u, count, heap, 0u); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 240, count, heap, 0)  [vtable 00B23E60[5]]
u32 FreeList23E60_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 240u, count, heap, 0u); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 132, count, heap, 0)  [vtable 00B23DA0[5]]
u32 FreeList23DA0_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 132u, count, heap, 0u); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 156, count, heap, 0)  [vtable 00B23D80[5]]
u32 FreeList23D80_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 156u, count, heap, 0u); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 116, count, heap, 0)  [vtable 00B23DE0[5]]
u32 FreeList23DE0_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 116u, count, heap, 0u); }

// u32 (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 144, count, heap, 0)  [vtable 00B23E40[5]]
u32 FreeList23E40_Init(u8* list, u32 count, u32 heap) { return call<u32>(kfn_FreeList_Init, list, 144u, count, heap, 0u); }

// u32 (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B5F8)  [vtable 00B23EC0[2]]
u32 Release_00508484(u8* self) { return release(self, 0x00F3B5F8u); }

// u32 (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B618)  [vtable 00B23368[2]]
u32 Release_00508B54(u8* self) { return release(self, 0x00F3B618u); }

// u32 (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B500)  [vtable 00B23EE0[2]]
u32 Release_0050AF9C(u8* self) { return release(self, 0x00F3B500u); }

// u32 (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B4D0)  [vtable 00B233D0[2]]
u32 Release_0050CAA8(u8* self) { return release(self, 0x00F3B4D0u); }

// u32 (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B460)  [vtable 00B23F20[2]]
u32 Release_0051AEE8(u8* self) { return release(self, 0x00F3B460u); }

// u32 (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B578)  [vtable 00B236C0[2], 00B240D0[2]]
u32 Release_005216FC(u8* self) { return release(self, 0x00F3B578u); }

// u32 (this): when set, this->vtable = 0x00B23160, then Mem_FreeFrom(this, 0)  [vtable 00B23160[1]]
u32 Delete_0079ED98(u8* self) { return delete_with(self, 0x00B23160u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23160, then SmallFree(this, 40)  [vtable 00B23170[1]]
u32 Delete_0079F288(u8* self) { return delete_with(self, 0x00B23160u, 0, kfn_SmallFree, 40u); }

// u32 (this): when set, this->vtable = 0x00B23160, then SmallFree(this, 40)  [vtable 00B23190[1]]
u32 Delete_0079F7B8(u8* self) { return delete_with(self, 0x00B23160u, 0, kfn_SmallFree, 40u); }

// u32 (this): when set, this->vtable = 0x00B23160, then SmallFree(this, 64)  [vtable 00B231B0[1]]
u32 Delete_0079FDD0(u8* self) { return delete_with(self, 0x00B23160u, 0, kfn_SmallFree, 64u); }

// u32 (this): when set, this->vtable = 0x00B23160, then SmallFree(this, 64)  [vtable 00B231D0[1]]
u32 Delete_007A0330(u8* self) { return delete_with(self, 0x00B23160u, 0, kfn_SmallFree, 64u); }

// u32 (this): when set, this->vtable = 0x00B231F0, then Mem_FreeFrom(this, 0)  [vtable 00B231F0[1]]
u32 Delete_007A0384(u8* self) { return delete_with(self, 0x00B231F0u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B231F0, then SmallFree(this, 84)  [vtable 00B23200[1]]
u32 Delete_007A03D8(u8* self) { return delete_with(self, 0x00B231F0u, 0, kfn_SmallFree, 84u); }

// u32 (this): when set, this->vtable = 0x00B23210, then Mem_FreeFrom(this, 0)  [vtable 00B23210[1]]
u32 Delete_007A042C(u8* self) { return delete_with(self, 0x00B23210u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23210, then Mem_FreeFrom(this, 0)  [vtable 00B23220[1]]
u32 Delete_007A0480(u8* self) { return delete_with(self, 0x00B23210u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23230, then Mem_FreeFrom(this, 0)  [vtable 00B23230[1]]
u32 Delete_007A04D4(u8* self) { return delete_with(self, 0x00B23230u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23230, then Mem_FreeFrom(this, 0)  [vtable 00B23240[1]]
u32 Delete_007A0528(u8* self) { return delete_with(self, 0x00B23230u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23250, then Mem_FreeFrom(this, 0)  [vtable 00B23250[1]]
u32 Delete_007A057C(u8* self) { return delete_with(self, 0x00B23250u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23250, then Mem_FreeFrom(this, 0)  [vtable 00B23260[1]]
u32 Delete_007A05D0(u8* self) { return delete_with(self, 0x00B23250u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23270, then Mem_FreeFrom(this, 0)  [vtable 00B23270[1]]
u32 Delete_007A0624(u8* self) { return delete_with(self, 0x00B23270u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23270, then Mem_FreeFrom(this, 0)  [vtable 00B23280[1]]
u32 Delete_007A0678(u8* self) { return delete_with(self, 0x00B23270u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23290, then Mem_FreeFrom(this, 0)  [vtable 00B23290[1]]
u32 Delete_007A06CC(u8* self) { return delete_with(self, 0x00B23290u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23290, then SmallFree(this, 112)  [vtable 00B232A0[1]]
u32 Delete_007A0720(u8* self) { return delete_with(self, 0x00B23290u, 0, kfn_SmallFree, 112u); }

// u32 (this): when set, this->vtable = 0x00B232B0, then Mem_FreeFrom(this, 0)  [vtable 00B232B0[1]]
u32 Delete_007A0774(u8* self) { return delete_with(self, 0x00B232B0u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B233A0, then Mem_FreeFrom(this, 0)  [vtable 00B233A0[1]]
u32 Delete_007A0EC4(u8* self) { return delete_with(self, 0x00B233A0u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B234C8[1]]
u32 Delete_007A1678(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B234E8[1]]
u32 Delete_007A16E8(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23514[1]]
u32 Delete_007A1730(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B2353C[1]]
u32 Delete_007A1788(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B2356C[1]]
u32 Delete_007A17CC(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23590[1]]
u32 Delete_007A1810(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B235BC[1]]
u32 Delete_007A1858(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B235DC[1]]
u32 Delete_007A18AC(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23610[1]]
u32 Delete_007A1A4C(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23630[1]]
u32 Delete_007A1AA0(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23670[1]]
u32 Delete_007A1AF8(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23CD0, then Mem_FreeFrom(this, 0)  [vtable 00B23CD0[1]]
u32 Delete_007A2288(u8* self) { return delete_with(self, 0x00B23CD0u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23CF8, then Mem_FreeFrom(this, 0)  [vtable 00B23CF8[1]]
u32 Delete_007A22E4(u8* self) { return delete_with(self, 0x00B23CF8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23CF8, then Mem_FreeFrom(this, 0)  [vtable 00B23D50[1]]
u32 Delete_007A2390(u8* self) { return delete_with(self, 0x00B23CF8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23F38[1]]
u32 Delete_007A3524(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23F60[1]]
u32 Delete_007A3568(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B240B0[1]]
u32 Delete_007A35AC(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23F90[1]]
u32 Delete_007A35F0(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23FC0[1]]
u32 Delete_007A3634(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23FF0[1]]
u32 Delete_007A3678(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B24038[1]]
u32 Delete_007A36BC(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B24090[1]]
u32 Delete_007A3860(u8* self) { return delete_with(self, 0x00B234C8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23CF8, then Mem_FreeFrom(this, 0)  [vtable 00B24158[1]]
u32 Delete_007A3AD8(u8* self) { return delete_with(self, 0x00B23CF8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B23CF8, then Mem_FreeFrom(this, 0)  [vtable 00B24228[1]]
u32 Delete_007A3CB8(u8* self) { return delete_with(self, 0x00B23CF8u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B233A0, then Mem_FreeFrom(this, 0)  [vtable 00B24190[1]]
u32 Delete_007A3D00(u8* self) { return delete_with(self, 0x00B233A0u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B24458, then Mem_FreeFrom(this, 0)  [vtable 00B24458[3]]
u32 Delete_007A5D4C(u8* self) { return delete_with(self, 0x00B24458u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B24458, then Mem_FreeFrom(this, 0)  [vtable 00B24478[3]]
u32 Delete_007A5D90(u8* self) { return delete_with(self, 0x00B24458u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B24500, then Mem_FreeFrom(this, 0)  [vtable 00B245D8[4]]
u32 Delete_007A5EDC(u8* self) { return delete_with(self, 0x00B24500u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B24500, then Mem_FreeFrom(this, 0)  [vtable 00B24500[4]]
u32 Delete_007A5F84(u8* self) { return delete_with(self, 0x00B24500u, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B2451C, then Mem_FreeFrom(this, 0)  [vtable 00B2451C[1]]
u32 Delete_007A5FD8(u8* self) { return delete_with(self, 0x00B2451Cu, 0, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, f_00500FD0(this) (its destructor), then SmallFree(this, 80)  [vtable 00B23338[1]]
u32 Delete_00501158(u8* self) { return delete_with(self, 0, 0x00500FD0u, kfn_SmallFree, 80u); }

// u32 (this): when set, f_005087EC(this) (its destructor), then Mem_FreeFrom(this, 0)  [vtable 00B23368[1]]
u32 Delete_00508B00(u8* self) { return delete_with(self, 0, 0x005087ECu, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, f_0050C118(this) (its destructor), then Mem_FreeFrom(this, 0)  [vtable 00B233B0[1]]
u32 Delete_0050C350(u8* self) { return delete_with(self, 0, 0x0050C118u, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, f_00538724(this) (its destructor), then Mem_FreeFrom(this, 0)  [vtable 00B23EC0[1]]
u32 Delete_00538908(u8* self) { return delete_with(self, 0, 0x00538724u, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, f_00548D44(this) (its destructor), then Mem_FreeFrom(this, 0)  [vtable 00B24130[1]]
u32 Delete_005490F8(u8* self) { return delete_with(self, 0, 0x00548D44u, kfn_Mem_FreeFrom, 0u); }

// u32 (this): when set, this->vtable = 0x00B24278, then Mem_FreePool(this, 0, 0)  [vtable 00B24278[4]]
u32 Delete_007A4B60(u8* self) { return delete_with(self, 0x00B24278u, 0, kfn_Mem_FreePool, 0u, 0u); }

// u32 (this): when set, this->vtable = 0x00B24278, then Mem_FreePool(this, 0, 0)  [vtable 00B24368[4]]
u32 Delete_007A4BA8(u8* self) { return delete_with(self, 0x00B24278u, 0, kfn_Mem_FreePool, 0u, 0u); }

// u32 (this): when set, this->vtable = 0x00B24278, then Mem_FreePool(this, 0, 0)  [vtable 00B24388[4]]
u32 Delete_007A4BF0(u8* self) { return delete_with(self, 0x00B24278u, 0, kfn_Mem_FreePool, 0u, 0u); }

// u32 (this): when set, this->vtable = 0x00B242E8, then Mem_FreePool(this, 0, 0)  [vtable 00B243A8[1]]
u32 Delete_007A4DA8(u8* self) { return delete_with(self, 0x00B242E8u, 0, kfn_Mem_FreePool, 0u, 0u); }

}  // namespace sly4::gfx

NATIVE(007A0870, sly4::gfx::FreeList23E80_Init)
NATIVE(007A0DF0, sly4::gfx::DeferredFreeList_Init)
NATIVE(007A0E24, sly4::gfx::FreeList23E20_Init)
NATIVE(007A0E58, sly4::gfx::FreeList23E00_Init)
NATIVE(007A0FE4, sly4::gfx::FreeList23E60_Init)
NATIVE(007A104C, sly4::gfx::FreeList23DA0_Init)
NATIVE(007A1634, sly4::gfx::FreeList23D80_Init)
NATIVE(007A1A08, sly4::gfx::FreeList23DE0_Init)
NATIVE(007A2160, sly4::gfx::FreeList23E40_Init)
NATIVE(00508484, sly4::gfx::Release_00508484)
NATIVE(00508B54, sly4::gfx::Release_00508B54)
NATIVE(0050AF9C, sly4::gfx::Release_0050AF9C)
NATIVE(0050CAA8, sly4::gfx::Release_0050CAA8)
NATIVE(0051AEE8, sly4::gfx::Release_0051AEE8)
NATIVE(005216FC, sly4::gfx::Release_005216FC)
NATIVE(0079ED98, sly4::gfx::Delete_0079ED98)
NATIVE(0079F288, sly4::gfx::Delete_0079F288)
NATIVE(0079F7B8, sly4::gfx::Delete_0079F7B8)
NATIVE(0079FDD0, sly4::gfx::Delete_0079FDD0)
NATIVE(007A0330, sly4::gfx::Delete_007A0330)
NATIVE(007A0384, sly4::gfx::Delete_007A0384)
NATIVE(007A03D8, sly4::gfx::Delete_007A03D8)
NATIVE(007A042C, sly4::gfx::Delete_007A042C)
NATIVE(007A0480, sly4::gfx::Delete_007A0480)
NATIVE(007A04D4, sly4::gfx::Delete_007A04D4)
NATIVE(007A0528, sly4::gfx::Delete_007A0528)
NATIVE(007A057C, sly4::gfx::Delete_007A057C)
NATIVE(007A05D0, sly4::gfx::Delete_007A05D0)
NATIVE(007A0624, sly4::gfx::Delete_007A0624)
NATIVE(007A0678, sly4::gfx::Delete_007A0678)
NATIVE(007A06CC, sly4::gfx::Delete_007A06CC)
NATIVE(007A0720, sly4::gfx::Delete_007A0720)
NATIVE(007A0774, sly4::gfx::Delete_007A0774)
NATIVE(007A0EC4, sly4::gfx::Delete_007A0EC4)
NATIVE(007A1678, sly4::gfx::Delete_007A1678)
NATIVE(007A16E8, sly4::gfx::Delete_007A16E8)
NATIVE(007A1730, sly4::gfx::Delete_007A1730)
NATIVE(007A1788, sly4::gfx::Delete_007A1788)
NATIVE(007A17CC, sly4::gfx::Delete_007A17CC)
NATIVE(007A1810, sly4::gfx::Delete_007A1810)
NATIVE(007A1858, sly4::gfx::Delete_007A1858)
NATIVE(007A18AC, sly4::gfx::Delete_007A18AC)
NATIVE(007A1A4C, sly4::gfx::Delete_007A1A4C)
NATIVE(007A1AA0, sly4::gfx::Delete_007A1AA0)
NATIVE(007A1AF8, sly4::gfx::Delete_007A1AF8)
NATIVE(007A2288, sly4::gfx::Delete_007A2288)
NATIVE(007A22E4, sly4::gfx::Delete_007A22E4)
NATIVE(007A2390, sly4::gfx::Delete_007A2390)
NATIVE(007A3524, sly4::gfx::Delete_007A3524)
NATIVE(007A3568, sly4::gfx::Delete_007A3568)
NATIVE(007A35AC, sly4::gfx::Delete_007A35AC)
NATIVE(007A35F0, sly4::gfx::Delete_007A35F0)
NATIVE(007A3634, sly4::gfx::Delete_007A3634)
NATIVE(007A3678, sly4::gfx::Delete_007A3678)
NATIVE(007A36BC, sly4::gfx::Delete_007A36BC)
NATIVE(007A3860, sly4::gfx::Delete_007A3860)
NATIVE(007A3AD8, sly4::gfx::Delete_007A3AD8)
NATIVE(007A3CB8, sly4::gfx::Delete_007A3CB8)
NATIVE(007A3D00, sly4::gfx::Delete_007A3D00)
NATIVE(007A5D4C, sly4::gfx::Delete_007A5D4C)
NATIVE(007A5D90, sly4::gfx::Delete_007A5D90)
NATIVE(007A5EDC, sly4::gfx::Delete_007A5EDC)
NATIVE(007A5F84, sly4::gfx::Delete_007A5F84)
NATIVE(007A5FD8, sly4::gfx::Delete_007A5FD8)
NATIVE(00501158, sly4::gfx::Delete_00501158)
NATIVE(00508B00, sly4::gfx::Delete_00508B00)
NATIVE(0050C350, sly4::gfx::Delete_0050C350)
NATIVE(00538908, sly4::gfx::Delete_00538908)
NATIVE(005490F8, sly4::gfx::Delete_005490F8)
NATIVE(007A4B60, sly4::gfx::Delete_007A4B60)
NATIVE(007A4BA8, sly4::gfx::Delete_007A4BA8)
NATIVE(007A4BF0, sly4::gfx::Delete_007A4BF0)
NATIVE(007A4DA8, sly4::gfx::Delete_007A4DA8)
