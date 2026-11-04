// The renderer range's Delete slots (reset the vtable to the base, then
// free: Mem_FreeFrom / SmallFree / Mem_FreePool; or run the destructor,
// then free), the refcount Release slots (DeferredDestroy at zero) and
// the typed free lists' Init slots. A null this returns 0 untouched.
// Names carry the address until the classes are known.
#include "gfx7.h"

using namespace sly4;

namespace {

constexpr u32 kfn_Mem_FreeFrom = 0x0054EE6C;     // (p, pool) (layers3.h)
constexpr u32 kfn_SmallFree = 0x00552420;        // (p, u32 size) (shader6.h)
constexpr u32 kfn_Mem_FreePool = 0x0056C730;     // (p, pool, tag) (layers4.h)
constexpr u32 kfn_DeferredDestroy = 0x005082F4;  // (obj, destroy OPD) (obj3.cpp)
constexpr u32 kfn_FreeList_Init = 0x00551A58;    // (list, elem_size, count, heap, bool)

}  // namespace

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 140, count, heap, 0)  [vtable 00B23E80[5]]
DECOMP(007A0870, FreeList23E80_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 140, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 12, count, heap, 0)  [vtable 00B23EA0[5]]
DECOMP(007A0DF0, DeferredFreeList_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 12, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 320, count, heap, 0)  [vtable 00B23E20[5]]
DECOMP(007A0E24, FreeList23E20_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 320, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 112, count, heap, 0)  [vtable 00B23E00[5]]
DECOMP(007A0E58, FreeList23E00_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 112, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 240, count, heap, 0)  [vtable 00B23E60[5]]
DECOMP(007A0FE4, FreeList23E60_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 240, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 132, count, heap, 0)  [vtable 00B23DA0[5]]
DECOMP(007A104C, FreeList23DA0_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 132, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 156, count, heap, 0)  [vtable 00B23D80[5]]
DECOMP(007A1634, FreeList23D80_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 156, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 116, count, heap, 0)  [vtable 00B23DE0[5]]
DECOMP(007A1A08, FreeList23DE0_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 116, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (FreeList*, u32 count, Heap* heap): FreeList_Init(list, 144, count, heap, 0)  [vtable 00B23E40[5]]
DECOMP(007A2160, FreeList23E40_Init) {
    ret(ctx, call(ctx, kfn_FreeList_Init, {ctx.gpr[3], 144, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B5F8)  [vtable 00B23EC0[2]]
DECOMP(00508484, Release_00508484) {
    const u32 self = arg<u32>(ctx, 0);
    const u64 refs = static_cast<u64>(load<u16>(self + 4)) - 1;  // lhz, addic -1
    const s16 left = static_cast<s16>(refs);  // extsh
    store<u16>(self + 4, static_cast<u16>(refs));
    ctx.gpr[5] = refs;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(left));
    if (left > 0) return;  // r3 = this
    call(ctx, kfn_DeferredDestroy, {ctx.gpr[3], 0x00F3B5F8u});
}

// void (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B618)  [vtable 00B23368[2]]
DECOMP(00508B54, Release_00508B54) {
    const u32 self = arg<u32>(ctx, 0);
    const u64 refs = static_cast<u64>(load<u16>(self + 4)) - 1;  // lhz, addic -1
    const s16 left = static_cast<s16>(refs);  // extsh
    store<u16>(self + 4, static_cast<u16>(refs));
    ctx.gpr[5] = refs;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(left));
    if (left > 0) return;  // r3 = this
    call(ctx, kfn_DeferredDestroy, {ctx.gpr[3], 0x00F3B618u});
}

// void (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B500)  [vtable 00B23EE0[2]]
DECOMP(0050AF9C, Release_0050AF9C) {
    const u32 self = arg<u32>(ctx, 0);
    const u64 refs = static_cast<u64>(load<u16>(self + 4)) - 1;  // lhz, addic -1
    const s16 left = static_cast<s16>(refs);  // extsh
    store<u16>(self + 4, static_cast<u16>(refs));
    ctx.gpr[5] = refs;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(left));
    if (left > 0) return;  // r3 = this
    call(ctx, kfn_DeferredDestroy, {ctx.gpr[3], 0x00F3B500u});
}

// void (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B4D0)  [vtable 00B233D0[2]]
DECOMP(0050CAA8, Release_0050CAA8) {
    const u32 self = arg<u32>(ctx, 0);
    const u64 refs = static_cast<u64>(load<u16>(self + 4)) - 1;  // lhz, addic -1
    const s16 left = static_cast<s16>(refs);  // extsh
    store<u16>(self + 4, static_cast<u16>(refs));
    ctx.gpr[5] = refs;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(left));
    if (left > 0) return;  // r3 = this
    call(ctx, kfn_DeferredDestroy, {ctx.gpr[3], 0x00F3B4D0u});
}

// void (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B460)  [vtable 00B23F20[2]]
DECOMP(0051AEE8, Release_0051AEE8) {
    const u32 self = arg<u32>(ctx, 0);
    const u64 refs = static_cast<u64>(load<u16>(self + 4)) - 1;  // lhz, addic -1
    const s16 left = static_cast<s16>(refs);  // extsh
    store<u16>(self + 4, static_cast<u16>(refs));
    ctx.gpr[5] = refs;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(left));
    if (left > 0) return;  // r3 = this
    call(ctx, kfn_DeferredDestroy, {ctx.gpr[3], 0x00F3B460u});
}

// void (this): --this->u16 +4 (refs); at 0 or below DeferredDestroy(this, OPD 0x00F3B578)  [vtable 00B236C0[2], 00B240D0[2]]
DECOMP(005216FC, Release_005216FC) {
    const u32 self = arg<u32>(ctx, 0);
    const u64 refs = static_cast<u64>(load<u16>(self + 4)) - 1;  // lhz, addic -1
    const s16 left = static_cast<s16>(refs);  // extsh
    store<u16>(self + 4, static_cast<u16>(refs));
    ctx.gpr[5] = refs;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(left));
    if (left > 0) return;  // r3 = this
    call(ctx, kfn_DeferredDestroy, {ctx.gpr[3], 0x00F3B578u});
}

// void (this): when set, this->vtable = 0x00B23160, then Mem_FreeFrom(this, 0)  [vtable 00B23160[1]]
DECOMP(0079ED98, Delete_0079ED98) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23160u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23160u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23160, then SmallFree(this, 40)  [vtable 00B23170[1]]
DECOMP(0079F288, Delete_0079F288) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23160u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23160u;
    call(ctx, kfn_SmallFree, {self, 40});
}

// void (this): when set, this->vtable = 0x00B23160, then SmallFree(this, 40)  [vtable 00B23190[1]]
DECOMP(0079F7B8, Delete_0079F7B8) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23160u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23160u;
    call(ctx, kfn_SmallFree, {self, 40});
}

// void (this): when set, this->vtable = 0x00B23160, then SmallFree(this, 64)  [vtable 00B231B0[1]]
DECOMP(0079FDD0, Delete_0079FDD0) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23160u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23160u;
    call(ctx, kfn_SmallFree, {self, 64});
}

// void (this): when set, this->vtable = 0x00B23160, then SmallFree(this, 64)  [vtable 00B231D0[1]]
DECOMP(007A0330, Delete_007A0330) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23160u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23160u;
    call(ctx, kfn_SmallFree, {self, 64});
}

// void (this): when set, this->vtable = 0x00B231F0, then Mem_FreeFrom(this, 0)  [vtable 00B231F0[1]]
DECOMP(007A0384, Delete_007A0384) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B231F0u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B231F0u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B231F0, then SmallFree(this, 84)  [vtable 00B23200[1]]
DECOMP(007A03D8, Delete_007A03D8) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B231F0u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B231F0u;
    call(ctx, kfn_SmallFree, {self, 84});
}

// void (this): when set, this->vtable = 0x00B23210, then Mem_FreeFrom(this, 0)  [vtable 00B23210[1]]
DECOMP(007A042C, Delete_007A042C) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23210u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23210u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23210, then Mem_FreeFrom(this, 0)  [vtable 00B23220[1]]
DECOMP(007A0480, Delete_007A0480) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23210u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23210u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23230, then Mem_FreeFrom(this, 0)  [vtable 00B23230[1]]
DECOMP(007A04D4, Delete_007A04D4) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23230u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23230u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23230, then Mem_FreeFrom(this, 0)  [vtable 00B23240[1]]
DECOMP(007A0528, Delete_007A0528) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23230u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23230u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23250, then Mem_FreeFrom(this, 0)  [vtable 00B23250[1]]
DECOMP(007A057C, Delete_007A057C) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23250u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23250u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23250, then Mem_FreeFrom(this, 0)  [vtable 00B23260[1]]
DECOMP(007A05D0, Delete_007A05D0) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23250u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23250u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23270, then Mem_FreeFrom(this, 0)  [vtable 00B23270[1]]
DECOMP(007A0624, Delete_007A0624) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23270u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23270u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23270, then Mem_FreeFrom(this, 0)  [vtable 00B23280[1]]
DECOMP(007A0678, Delete_007A0678) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23270u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23270u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23290, then Mem_FreeFrom(this, 0)  [vtable 00B23290[1]]
DECOMP(007A06CC, Delete_007A06CC) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23290u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23290u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23290, then SmallFree(this, 112)  [vtable 00B232A0[1]]
DECOMP(007A0720, Delete_007A0720) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23290u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23290u;
    call(ctx, kfn_SmallFree, {self, 112});
}

// void (this): when set, this->vtable = 0x00B232B0, then Mem_FreeFrom(this, 0)  [vtable 00B232B0[1]]
DECOMP(007A0774, Delete_007A0774) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B232B0u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B232B0u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B233A0, then Mem_FreeFrom(this, 0)  [vtable 00B233A0[1]]
DECOMP(007A0EC4, Delete_007A0EC4) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B233A0u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B233A0u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B234C8[1]]
DECOMP(007A1678, Delete_007A1678) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B234E8[1]]
DECOMP(007A16E8, Delete_007A16E8) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23514[1]]
DECOMP(007A1730, Delete_007A1730) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B2353C[1]]
DECOMP(007A1788, Delete_007A1788) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B2356C[1]]
DECOMP(007A17CC, Delete_007A17CC) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23590[1]]
DECOMP(007A1810, Delete_007A1810) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B235BC[1]]
DECOMP(007A1858, Delete_007A1858) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B235DC[1]]
DECOMP(007A18AC, Delete_007A18AC) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23610[1]]
DECOMP(007A1A4C, Delete_007A1A4C) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23630[1]]
DECOMP(007A1AA0, Delete_007A1AA0) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23670[1]]
DECOMP(007A1AF8, Delete_007A1AF8) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23CD0, then Mem_FreeFrom(this, 0)  [vtable 00B23CD0[1]]
DECOMP(007A2288, Delete_007A2288) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23CD0u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23CD0u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23CF8, then Mem_FreeFrom(this, 0)  [vtable 00B23CF8[1]]
DECOMP(007A22E4, Delete_007A22E4) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23CF8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23CF8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23CF8, then Mem_FreeFrom(this, 0)  [vtable 00B23D50[1]]
DECOMP(007A2390, Delete_007A2390) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23CF8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23CF8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23F38[1]]
DECOMP(007A3524, Delete_007A3524) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23F60[1]]
DECOMP(007A3568, Delete_007A3568) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B240B0[1]]
DECOMP(007A35AC, Delete_007A35AC) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23F90[1]]
DECOMP(007A35F0, Delete_007A35F0) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23FC0[1]]
DECOMP(007A3634, Delete_007A3634) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B23FF0[1]]
DECOMP(007A3678, Delete_007A3678) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B24038[1]]
DECOMP(007A36BC, Delete_007A36BC) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B234C8, then Mem_FreeFrom(this, 0)  [vtable 00B24090[1]]
DECOMP(007A3860, Delete_007A3860) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B234C8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B234C8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23CF8, then Mem_FreeFrom(this, 0)  [vtable 00B24158[1]]
DECOMP(007A3AD8, Delete_007A3AD8) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23CF8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23CF8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B23CF8, then Mem_FreeFrom(this, 0)  [vtable 00B24228[1]]
DECOMP(007A3CB8, Delete_007A3CB8) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B23CF8u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B23CF8u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B233A0, then Mem_FreeFrom(this, 0)  [vtable 00B24190[1]]
DECOMP(007A3D00, Delete_007A3D00) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B233A0u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B233A0u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B24458, then Mem_FreeFrom(this, 0)  [vtable 00B24458[3]]
DECOMP(007A5D4C, Delete_007A5D4C) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B24458u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B24458u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B24458, then Mem_FreeFrom(this, 0)  [vtable 00B24478[3]]
DECOMP(007A5D90, Delete_007A5D90) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B24458u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B24458u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B24500, then Mem_FreeFrom(this, 0)  [vtable 00B245D8[4]]
DECOMP(007A5EDC, Delete_007A5EDC) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B24500u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B24500u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B24500, then Mem_FreeFrom(this, 0)  [vtable 00B24500[4]]
DECOMP(007A5F84, Delete_007A5F84) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B24500u);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B24500u;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B2451C, then Mem_FreeFrom(this, 0)  [vtable 00B2451C[1]]
DECOMP(007A5FD8, Delete_007A5FD8) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {  // cmpwi: r3 (0) is the result
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B2451Cu);
    ctx.gpr[5] = self;
    ctx.gpr[6] = 0x00B2451Cu;
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, f_00500FD0(this) (its destructor), then SmallFree(this, 80)  [vtable 00B23338[1]]
DECOMP(00501158, Delete_00501158) {
    const u64 self = ctx.gpr[3];
    if (static_cast<u32>(self) == 0) return;  // r3 (0) is the result
    call(ctx, 0x00500FD0u, {self});
    call(ctx, kfn_SmallFree, {self, 80});
}

// void (this): when set, f_005087EC(this) (its destructor), then Mem_FreeFrom(this, 0)  [vtable 00B23368[1]]
DECOMP(00508B00, Delete_00508B00) {
    const u64 self = ctx.gpr[3];
    if (static_cast<u32>(self) == 0) return;  // r3 (0) is the result
    call(ctx, 0x005087ECu, {self});
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, f_0050C118(this) (its destructor), then Mem_FreeFrom(this, 0)  [vtable 00B233B0[1]]
DECOMP(0050C350, Delete_0050C350) {
    const u64 self = ctx.gpr[3];
    if (static_cast<u32>(self) == 0) return;  // r3 (0) is the result
    call(ctx, 0x0050C118u, {self});
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, f_00538724(this) (its destructor), then Mem_FreeFrom(this, 0)  [vtable 00B23EC0[1]]
DECOMP(00538908, Delete_00538908) {
    const u64 self = ctx.gpr[3];
    if (static_cast<u32>(self) == 0) return;  // r3 (0) is the result
    call(ctx, 0x00538724u, {self});
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, f_00548D44(this) (its destructor), then Mem_FreeFrom(this, 0)  [vtable 00B24130[1]]
DECOMP(005490F8, Delete_005490F8) {
    const u64 self = ctx.gpr[3];
    if (static_cast<u32>(self) == 0) return;  // r3 (0) is the result
    call(ctx, 0x00548D44u, {self});
    call(ctx, kfn_Mem_FreeFrom, {self, 0});
}

// void (this): when set, this->vtable = 0x00B24278, then Mem_FreePool(this, 0, 0)  [vtable 00B24278[4]]
DECOMP(007A4B60, Delete_007A4B60) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B24278u);
    ctx.gpr[6] = self;
    ctx.gpr[7] = 0x00B24278u;
    call(ctx, kfn_Mem_FreePool, {self, 0, 0});
}

// void (this): when set, this->vtable = 0x00B24278, then Mem_FreePool(this, 0, 0)  [vtable 00B24368[4]]
DECOMP(007A4BA8, Delete_007A4BA8) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B24278u);
    ctx.gpr[6] = self;
    ctx.gpr[7] = 0x00B24278u;
    call(ctx, kfn_Mem_FreePool, {self, 0, 0});
}

// void (this): when set, this->vtable = 0x00B24278, then Mem_FreePool(this, 0, 0)  [vtable 00B24388[4]]
DECOMP(007A4BF0, Delete_007A4BF0) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B24278u);
    ctx.gpr[6] = self;
    ctx.gpr[7] = 0x00B24278u;
    call(ctx, kfn_Mem_FreePool, {self, 0, 0});
}

// void (this): when set, this->vtable = 0x00B242E8, then Mem_FreePool(this, 0, 0)  [vtable 00B243A8[1]]
DECOMP(007A4DA8, Delete_007A4DA8) {
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ctx.gpr[4] = ctx.gpr[3];
        return;
    }
    const u64 self = ctx.gpr[3];
    store<u32>(static_cast<u32>(self), 0x00B242E8u);
    ctx.gpr[6] = self;
    ctx.gpr[7] = 0x00B242E8u;
    call(ctx, kfn_Mem_FreePool, {self, 0, 0});
}
