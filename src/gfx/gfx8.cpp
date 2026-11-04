// The renderer range's functions of 19 to ~30 instructions: destructors
// that reset the vtable around a base destructor, a countdown, a list
// walker, an event-flag signal and the 0x00B24130 object's destructor.
// Names carry the address where the role is unclear.
#include <atomic>
#include <bit>

#include "opentit/recomp.h"

#include "fx.h"
#include "gfx8.h"

using namespace sly4;

namespace {

namespace g8 = sly4::render::gfx8;

constexpr u32 k8RenderEvent = 0x010C5A10;  // Obj*: +20 u32 event flag id, +48 u8 signalled (name?)
constexpr u32 k8TileSlots = 0x00F67244;    // u16 (tgt::g::tile_slots)
constexpr u32 k8GcmState = 0x010F10A0;     // fx::g::state (+0 the gcm context)
constexpr u32 k8Mode011640E4 = 0x011640E4; // u32 (gfx7_misc.cpp kMode011640E4; name?)
constexpr u32 k8SysEventFlagSet = 87;      // sys_event_flag_set(id, u64 bits)
constexpr u32 k8SysTimerUsleep = 141;      // sys_timer_usleep(u64 us)
constexpr u32 k8InitOnce23FF0 = 0x00F67A00;  // u8: Effect23FF0_InitOnce ran (name?)
constexpr u32 k8InitOnce24038 = 0x00F67A01;  // u8: Effect24038_InitOnce ran (name?)
constexpr u32 k8ReportBase = 0x011640E8;   // pool::query::g::report_base
constexpr u32 k8ReportNext = 0x011640EC;   // pool::query::g::report_next

f64 lfs(u32 a) { return static_cast<f64>(load<f32>(a)); }

// The 16-byte buffer part {+0 p, +4 bit 0: pool 5, +8 bit 0: not owned,
// +12 tag in bits 28..31} freed the way Pool_DestructPart (0052EFE0) does,
// inlined: Mem_FreePool(p, pool, tag) unless p is null or not owned; +0
// cleared after a free.
void free_part(PPUContext& ctx, u32 part) {
    const u32 p = load<u32>(part);
    if (p == 0 || (load<u32>(part + 8) & 1) != 0) return;
    const u32 pool = (load<u32>(part + 4) & 1) != 0 ? 5u : 4u;
    call(ctx, g8::fn::Mem_FreePool, {p, pool, load<u32>(part + 12) >> 28});
    store<u32>(part, 0);
}

// ApeTFreeList<T>::~ApeTFreeList: the list's vtable, FreeList_Destroy,
// then the base vtable 0x00B24298 and FreeList_Destroy again.
void freelist_destruct(PPUContext& ctx, u32 vtable) {
    const u32 self = arg<u32>(ctx, 0);
    store<u32>(self, vtable);
    ctx.gpr[4] = vtable;
    call(ctx, g8::fn::FreeList_Destroy, {self});
    store<u32>(self, 0x00B24298u);
    ctx.gpr[4] = 0x00B24298u;
    call(ctx, g8::fn::FreeList_Destroy, {self});
}

// Deleting destructor: when set, the vtable, the destructor body `dtor`,
// then Mem_FreeFrom(this, 0). A null this returns 0 untouched.
void delete_via(PPUContext& ctx, u32 vtable, u32 dtor) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    store<u32>(self, vtable);
    ctx.gpr[4] = vtable;
    call(ctx, dtor, {self});
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0});
}

// Slot 6 of the two effect vtables (base 0x00B234C8): slot 7 on this, the first time only.
void init_once(PPUContext& ctx, u32 flag) {
    const u32 done = load<u8>(flag);
    ctx.gpr[4] = done;
    if (done != 0) return;
    const u32 self = arg<u32>(ctx, 0);
    store<u8>(flag, 1);
    fx::vcall(ctx, self, 28, {self});
}

}  // namespace

// void (this, f32 dt): while this->u8 +88 bit 0x80 is set, this->f32 +84
// -= dt; at or below 0.0 (k at 0x0051F158) the bit and the u8 at +4 clear
DECOMP(0051F160, Countdown_Tick) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 flags = load<u8>(self + 88);
    if ((flags & 0x80) == 0) return;
    const f32 left = static_cast<f32>(lfs(self + 84) - farg(ctx, 0));  // fsubs
    store<f32>(self + 84, left);
    ctx.fpr[1] = left;
    if (static_cast<f64>(left) <= lfs(0x0051F158u)) {  // fcmpu, cror lt|eq (NaN: no)
        store<u8>(self + 4, 0);
        store<u8>(self + 88, static_cast<u8>(flags & ~0x80u));
    }
}

// ApeTFreeList<ApeDebug::Prim>::~ApeTFreeList (debug.cpp
// kPrimFreeListVtable)  [vtable 00B24418[0]]
DECOMP(007A4F3C, PrimList_Destruct) { freelist_destruct(ctx, 0x00B24418u); }

// ApeTFreeList<IFF::ChunkDesc>::~ApeTFreeList (iff.cpp kFreeListVtable)
// [vtable 00B24438[0]]
DECOMP(007A4FF8, ChunkDescList_Destruct) { freelist_destruct(ctx, 0x00B24438u); }

// this (this, u32 v): this->+96 = v, then each of the this->u16 +52
// entries of the 8-byte array at this->+48 gets ->+20 = this->+96 (all
// three re-read every pass)
DECOMP(0050A14C, SetAll_0050A14C) {
    const u32 self = arg<u32>(ctx, 0);
    store<u32>(self + 96, arg<u32>(ctx, 1));
    if (load<u16>(self + 52) == 0) return;
    u32 i = 0, n;
    do {
        const u32 base = load<u32>(self + 48), v = load<u32>(self + 96);
        n = load<u16>(self + 52);  // read before the store
        store<u32>(load<u32>(base + 8 * i) + 20, v);
        ++i;
    } while (i < n);  // cmplw
}

// u32 (s32 skip, a, b, c, d): skip ? d : f_005A9DB0(a, b, c, d)
DECOMP(005AC7AC, Call5A9DB0_005AC7AC) {
    const u64 a = ctx.gpr[4], b = ctx.gpr[5], c = ctx.gpr[6], d = ctx.gpr[7];
    if (arg<s32>(ctx, 0) != 0) {
        ctx.gpr[3] = d;
        return;
    }
    call(ctx, 0x005A9DB0u, {a, b, c, d});
}

// void (this): vtable 0x00B232C0, Holder_FreeData(this + 16), then the base
// vtable 0x00B232B0 (left in r3)  [vtable 00B232C0[0]]
DECOMP(007A07B8, Holder232C0_Destruct) {
    const u32 self = arg<u32>(ctx, 0);
    store<u32>(self, 0x00B232C0u);
    ctx.gpr[4] = 0x00B232C0u;
    call(ctx, g8::fn::Holder_FreeData, {static_cast<u32>(self + 16)});
    store<u32>(self, 0x00B232B0u);
    ctx.gpr[3] = 0x00B232B0u;
}

// ShaderType::~ShaderType as the derived 0x00B241B0 type inlines it: base
// vtable 0x00B23F00, PtrVector_Clear(this + 12), Nop(this + 12)
// [vtable 00B241B0[2]]
DECOMP(007A2BA4, ShaderType241B0_Destruct) {
    const u32 self = arg<u32>(ctx, 0), v = self + 12;
    store<u32>(self, 0x00B23F00u);
    ctx.gpr[4] = 0x00B23F00u;
    ctx.gpr[5] = v;
    call(ctx, g8::fn::PtrVector_Clear, {v});
    call(ctx, g8::fn::Nop, {v});
}

// ShaderType::~ShaderType: vtable 0x00B23F00, PtrVector_Clear(this + 12),
// Nop(this + 12)  [vtable 00B23F00[2]]
DECOMP(007A344C, ShaderType_Destruct) {
    const u32 self = arg<u32>(ctx, 0), v = self + 12;
    store<u32>(self, 0x00B23F00u);
    ctx.gpr[4] = 0x00B23F00u;
    ctx.gpr[5] = v;
    call(ctx, g8::fn::PtrVector_Clear, {v});
    call(ctx, g8::fn::Nop, {v});
}

// void (): sys_event_flag_set(obj->+20, 1) on the object at 0x010C5A10,
// then obj->u8 +48 = 1 (obj re-read; left in r3)
DECOMP(00512BE0, SignalRender_00512BE0) {
    syscall(ctx, k8SysEventFlagSet, {load<u32>(load<u32>(k8RenderEvent) + 20), 1});
    const u32 obj = load<u32>(k8RenderEvent);
    store<u8>(obj + 48, 1);
    ctx.gpr[3] = obj;
    ctx.gpr[4] = 1;
}

// The 0x00B24130 object's destructor (Ctor_00548D14 builds it): for each
// of the surfaces at +644 and +648 (52-byte records, +0 buffer part, +40
// Target*, +48 u8 keep target): unless kept, the target's tile slot (s8
// +45, -1 none) is released from tgt::g::tile_slots for an original (u8
// +56 == 0), its part freed and the target Mem_FreeFrom'd, +40 = 0; then
// the surface's own part freed, the surface freed and the pointer
// cleared. Ends with the base vtable 0x00B23CD0 (left in r3).
// [vtable 00B24130[0]]
DECOMP(00548D44, Obj24130_Destruct) {
    const u32 self = arg<u32>(ctx, 0);
    for (u32 off = 644; off <= 648; off += 4) {
        const u32 s = load<u32>(self + off);
        if (off == 644) store<u32>(self, 0x00B24130u);  // after the +644 load
        if (s == 0) continue;
        u32 t;
        if (load<u8>(s + 48) == 0 && (t = load<u32>(s + 40)) != 0) {
            if (load<u8>(t + 56) == 0) {
                const u32 slot = load<u8>(t + 45);
                if (slot != 0xFF) {
                    const u32 bit = (slot & 0x20) != 0 ? 0u : (1u << (slot & 31));  // slw (6-bit amount)
                    store<u16>(k8TileSlots, static_cast<u16>(load<u16>(k8TileSlots) & ~bit));
                }
                free_part(ctx, t);
                // The +648 copy frees the part a second time (a no-op: +0 is
                // now 0 or the part was skipped).
                if (off == 648) free_part(ctx, t);
            } else {
                free_part(ctx, t);
            }
            call(ctx, g8::fn::Mem_FreeFrom, {t, 0});
            store<u32>(s + 40, 0);
        }
        free_part(ctx, s);
        call(ctx, g8::fn::Mem_FreeFrom, {s, 0});
        store<u32>(self + off, 0);
    }
    store<u32>(self, 0x00B23CD0u);
    ctx.gpr[3] = 0x00B23CD0u;
}

// The other ApeTFreeList<T> destructors (vtable slot 0 each).
DECOMP(007A23D4, SurfaceList_Destruct) { freelist_destruct(ctx, 0x00B23D80u); }       // <AglRenderSurface>
DECOMP(007A249C, TextureList_Destruct) { freelist_destruct(ctx, 0x00B23DA0u); }       // <AglTexture>
DECOMP(007A262C, SubMeshList_Destruct) { freelist_destruct(ctx, 0x00B23DE0u); }
DECOMP(007A26F4, TopologyList_Destruct) { freelist_destruct(ctx, 0x00B23E00u); }
DECOMP(007A27BC, MeshMtlTableList_Destruct) { freelist_destruct(ctx, 0x00B23E20u); }
DECOMP(007A2884, FreeList23E40_Destruct) { freelist_destruct(ctx, 0x00B23E40u); }     // 144-byte elements
DECOMP(007A294C, LightList_Destruct) { freelist_destruct(ctx, 0x00B23E60u); }
DECOMP(007A2A14, EmitPropsList_Destruct) { freelist_destruct(ctx, 0x00B23E80u); }
DECOMP(007A2ADC, DeferredFreeList_Destruct) { freelist_destruct(ctx, 0x00B23EA0u); }

// Material deleting destructor: when set, vtable 0x00B233D0, f_0050C118
// (the base destructor body), then Mem_FreeFrom(this, 0)  [vtable 00B233D0[1]]
DECOMP(007A0F90, Material_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;  // r3 stays 0
    store<u32>(self, 0x00B233D0u);
    ctx.gpr[4] = 0x00B233D0u;
    call(ctx, 0x0050C118u, {self});
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0});
}

// void (this): vtable 0x00B232D8, EmitPropsRef_Set(this + 4, 0), then
// f_0079CA4C(this + 4)  [vtable 00B232D8[0]]
DECOMP(00500548, EmitPropsHolder_Destruct) {
    const u32 self = arg<u32>(ctx, 0), ref = self + 4;
    store<u32>(self, 0x00B232D8u);
    ctx.gpr[5] = 0x00B232D8u;
    call(ctx, g8::fn::EmitPropsRef_Set, {ref, 0});
    call(ctx, 0x0079CA4Cu, {ref});
}

// u32 (IffReader* r, Obj* o): FileReader_Read(r, o->+16, Iff_ChunkSize(r)),
// returns 1 (an IFF chunk handler)
DECOMP(00524400, ReadChunk16_00524400) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 size = static_cast<u32>(call(ctx, g8::fn::Iff_ChunkSize, {r, o}));
    call(ctx, g8::fn::FileReader_Read, {r, load<u32>(o + 16), size});
    ctx.gpr[3] = 1;
}

// u32 (IffReader* r, Obj* o): FileReader_Read(r, o->+20, Iff_ChunkSize(r)),
// returns 1 (an IFF chunk handler)
DECOMP(00524454, ReadChunk20_00524454) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 size = static_cast<u32>(call(ctx, g8::fn::Iff_ChunkSize, {r, o}));
    call(ctx, g8::fn::FileReader_Read, {r, load<u32>(o + 20), size});
    ctx.gpr[3] = 1;
}

// void (): f_00527C4C(), then f_005E31B8(gcm context, 1) and
// f_005E337C(gcm context, *(u32*)0x011640E4)
DECOMP(005488AC, Call_005488AC) {
    call(ctx, 0x00527C4Cu);
    call(ctx, 0x005E31B8u, {load<u32>(k8GcmState), 1});
    call(ctx, 0x005E337Cu, {load<u32>(k8GcmState), load<u32>(k8Mode011640E4)});
}

// Texture deleting destructor (the Texture_DestructBase body inlined:
// vtable 0x00B23EE0, f_0050A5E4)  [vtable 00B23EE0[1]]
DECOMP(0053A2F4, TextureBase_Delete) { delete_via(ctx, 0x00B23EE0u, 0x0050A5E4u); }

// RenderSurface deleting destructor: vtable 0x00B23F20, f_0051AD90
// [vtable 00B23F20[1]]
DECOMP(0053C8A0, RenderSurface_Delete) { delete_via(ctx, 0x00B23F20u, 0x0051AD90u); }

// The 0x00B23D28 object's deleting destructor (Destruct_0079E9B4 inlined:
// vtable, Thunk_00549140 -> Obj24130_Destruct)  [vtable 00B23D28[1]]
DECOMP(007A2338, Obj23D28_Delete) { delete_via(ctx, 0x00B23D28u, 0x00549140u); }

// RenderTarget deleting destructor (load.h kRenderTargetVtbl 0x00B240D0,
// f_0052163C)  [vtable 00B240D0[1]]
DECOMP(007A38A4, RenderTarget_Delete) { delete_via(ctx, 0x00B240D0u, 0x0052163Cu); }

// ApeTFreeList<T> base deleting destructor: when set, vtable 0x00B24298,
// FreeList_Destroy, then Mem_FreePool(this, 0, 0)  [vtable 00B24298[1]]
DECOMP(007A4C94, FreeListBase_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    store<u32>(self, 0x00B24298u);
    ctx.gpr[4] = 0x00B24298u;
    call(ctx, g8::fn::FreeList_Destroy, {self});
    call(ctx, g8::fn::Mem_FreePool, {self, 0, 0});
}

// The pool.h Buffer's sibling with vtable 0x00B24180: frees +8
// (Mem_FreePool pool 4, tag 1; no null check), then the BufferBase vtable
// 0x00B24110 (left in r3)  [vtable 00B24180[0]]
DECOMP(007A3B1C, Buffer24180_Destruct) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 buf = load<u32>(self + 8);
    store<u32>(self, 0x00B24180u);
    call(ctx, g8::fn::Mem_FreePool, {buf, 4, 1});
    store<u32>(self, 0x00B24110u);
    ctx.gpr[3] = 0x00B24110u;
}

// void (DrawRecordList* l): sleeps 300 us (sys_timer_usleep) until the
// list's +256 counter (obj12.h kTrList) is 0
DECOMP(005244A8, DrawRecordList_Wait) {
    const u32 busy = arg<u32>(ctx, 0) + 256;
    while (load<u32>(busy) != 0) syscall(ctx, k8SysTimerUsleep, {300});
    ctx.gpr[3] = 0;
}

// u32 (CmdBuf* b, u32 needed_words): formats "AglPs3CmndBuf: Overflow: ..."
// (0x00AF3428) into a 256-byte stack buffer with f_00724304 (sprintf): the
// +0 / +4 / +8 words, then the overflow (needed - (+4 - +8) / 4, signed)
// in bytes and KB; the text is not used. Returns 0.
DECOMP(00537ED0, CmdBuf_OnOverflow) {
    const u32 b = arg<u32>(ctx, 0);
    const u32 end = load<u32>(b + 4), cur = load<u32>(b + 8), begin = load<u32>(b);
    const u32 free_words = static_cast<u32>(static_cast<s32>(end - cur) / 4);  // srawi + addze
    const u32 over = arg<u32>(ctx, 1) - free_words;
    Stack text(ctx, 256);  // 112(r1) of a 368-byte frame
    ctx.gpr[10] = 0x00AF0000u;
    call(ctx, 0x00724304u, {text.addr(), 0x00AF3428u, begin, end, cur, over << 2, (over >> 8) & 0x003FFFFFu});
    ctx.gpr[3] = 0;
}

// Slot 6 of the effect vtable 0x00B23FF0: slot 7 (Effect23FF0_GetTargets)
// once, guarded by the u8 at 0x00F67A00  [vtable 00B23FF0[6]]
DECOMP(005428B0, Effect23FF0_InitOnce) { init_once(ctx, k8InitOnce23FF0); }

// Slot 6 of the effect vtable 0x00B24038: slot 7 once, guarded by the u8
// at 0x00F67A01  [vtable 00B24038[6]]
DECOMP(00543BAC, Effect24038_InitOnce) { init_once(ctx, k8InitOnce24038); }

// void (this): copies seven globals of the 0x01164040 block (fx.h work
// targets) into +132..+156: +132 = [0x01164078], +136 = work1, +148 =
// work0, +140 = [0x01164054], +152 = [0x01164050], +144 = [0x0116405C],
// +156 = [0x01164058]  [vtable 00B23FF0[7]]
DECOMP(00542908, Effect23FF0_GetTargets) {
    const u32 self = arg<u32>(ctx, 0);
    store<u32>(self + 132, load<u32>(0x01164078u));
    store<u32>(self + 136, load<u32>(fx::g::work1));
    store<u32>(self + 148, load<u32>(fx::g::work0));
    store<u32>(self + 140, load<u32>(0x01164054u));
    store<u32>(self + 152, load<u32>(0x01164050u));
    const u32 a = load<u32>(0x0116405Cu);
    store<u32>(self + 144, a);
    const u32 c = load<u32>(0x01164058u);
    store<u32>(self + 156, c);
    ctx.gpr[4] = c;
    ctx.gpr[5] = load<u32>(0x01164050u);
}

// void (): f_00527B28(), the occlusion report ring reset (report_next and
// the u32 at 0x011640E4 = 0), report_base =
// cellGcmGetReportDataAddressLocation(0, 0)  [called by Graphics_Init]
DECOMP(00548974, Query_InitReports) {
    call(ctx, 0x00527B28u);
    store<u32>(k8ReportNext, 0);
    store<u32>(k8Mode011640E4, 0);
    const u64 base = call(ctx, 0x00ABD6E4u, {0, 0});
    store<u32>(k8ReportBase, static_cast<u32>(base));
}

// void (): when the u8 at 0x00F66FD4 is set and the object at 0x00F66FD0
// (shader6_draw.cpp kFurLight, hevent3_g.cpp kCamRig) exists, its virtual
// slot 4 on itself
DECOMP(005176F8, FurLight_Update) {
    if (load<u8>(0x00F66FD4u) == 0) return ret(ctx, 0u);
    const u32 obj = load<u32>(0x00F66FD0u);
    if (obj == 0) return ret(ctx, 0u);
    fx::vcall(ctx, obj, 16, {obj});
}

// bool (Slot* s): finds s among the 16 query slots (layers3::g::slots,
// 176 bytes each) and clears its u8 +4 (in use); 0 when not found
DECOMP(00527A70, Slots_MarkFree) {
    const u32 s = arg<u32>(ctx, 0);
    for (u32 i = 0; i < 16; ++i) {
        const u32 slot = 0x010DFAD0u + 176 * i;
        if (slot == s) {  // cmpw
            store<u8>(slot + 4, 0);
            ctx.gpr[4] = slot;
            ctx.gpr[6] = 0;
            ctx.gpr[7] = i;
            return ret(ctx, 1u);
        }
    }
    ctx.gpr[4] = 0x010DFAD0u + 176 * 16;
    ctx.gpr[7] = 16;
    ret(ctx, 0u);
}

// void (Slot* s): when s->+8 is set, its virtual slot 2 on itself, then
// s->+8 = 0 (returns 0)
DECOMP(00527ACC, Slot_ReleaseChild) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 obj = load<u32>(s + 8);
    if (obj == 0) return ret(ctx, 0u);
    fx::vcall(ctx, obj, 8, {obj});
    store<u32>(s + 8, 0);
    ret(ctx, 0u);
}

// void (BloomParams* b): unless b->f32 +16 (strength) < 0.01 (k at
// 0x0053DF30; NaN runs), the u8 at 0x00F67948 = 1 and Bloom_Chain(b)
DECOMP(0053DF38, Bloom_Run) {
    const u32 b = arg<u32>(ctx, 0);
    ctx.gpr[4] = b;
    ctx.fpr[1] = lfs(b + 16);
    ctx.fpr[2] = lfs(0x0053DF30u);
    if (ctx.fpr[1] < ctx.fpr[2]) return;
    if (load<u8>(0x00F67948u) == 0) store<u8>(0x00F67948u, 1);
    call(ctx, 0x0053D1B0u, {b});
}

// const char* (Shader*): the name from this->+20's virtual slot 3, or
// "NULL" (0x00B233C0) when +20 is 0  [vtable 00B233A0[7], 00B233D0[3]]
DECOMP(007A0F08, Shader_GetName) {
    const u32 o = load<u32>(arg<u32>(ctx, 0) + 20);
    if (o == 0) return ret(ctx, 0x00B233C0u);
    fx::vcall(ctx, o, 12, {o});
}

// void (): f_005E24F8(gcm context, 2043), then f_00517FB8() and
// Draw_InvalidateState() unless the u8 at 0x010DDBB0 is set and the one at
// 0x010DDBA8 (layers3 block_ddb88 +32) is 0
DECOMP(00518180, ResetGcmState_00518180) {
    call(ctx, 0x005E24F8u, {load<u32>(k8GcmState), 2043});
    if (load<u8>(0x010DDBB0u) != 0 && load<u8>(0x010DDBA8u) == 0) return ret(ctx, 0u);
    call(ctx, 0x00517FB8u);
    call(ctx, 0x0051279Cu);
}

// void (this, bool on): on: this->u8 +4 = on, the countdown (+84) = 0 and
// the u8 +88 gets bit 0x40, loses 0x80. Off: when no countdown runs (+88
// bit 0x80 clear) and +4 is set, one starts: bit 0x80, +84 = +80 (raw).
// See Countdown_Tick.
DECOMP(0051F09C, Countdown_Set) {
    const u32 self = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFF;
    if (on != 0) {
        const u32 flags = load<u8>(self + 88);
        store<u8>(self + 4, static_cast<u8>(on));
        store<u32>(self + 84, 0);
        store<u8>(self + 88, static_cast<u8>((flags & ~0xC0u) | 0x40u));
        ctx.gpr[4] = on;
        ctx.gpr[5] = 0;
        ctx.gpr[6] = 32;  // cntlzw 0
        return;
    }
    const u32 flags = load<u8>(self + 88);
    ctx.gpr[4] = flags;
    if ((flags & 0x80) != 0) return;
    if (load<u8>(self + 4) == 0) return;
    const u32 start = load<u32>(self + 80);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(start));
    store<u8>(self + 88, static_cast<u8>(flags | 0x80));
    store<u32>(self + 84, start);
}

// u32 (this, const Header* h): +52 = u16 h+4, +48 = h+0, +54 = u16 h+6,
// String_CopyN(this + 16, h + 11, 32), +47 = 0; returns 0
DECOMP(0050A28C, InitFromHeader_0050A28C) {
    const u32 self = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u16 a = load<u16>(h + 4);
    const u32 b = load<u32>(h);
    store<u16>(self + 52, a);
    store<u32>(self + 48, b);
    store<u16>(self + 54, load<u16>(h + 6));
    call(ctx, 0x00553638u, {static_cast<u32>(self + 16), static_cast<u32>(h + 11), 32});
    store<u8>(self + 47, 0);
    ctx.gpr[3] = 0;
}

// void (this): f_0053B668(this->+512, 1) unless +512 is the u32 at
// 0x00F66FBC already, then f_00532450(this->+512, this + 96, this + 104)
DECOMP(00520030, Bind_00520030) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 cur = load<u32>(0x00F66FBCu);
    if (load<u32>(self + 512) != cur) call(ctx, 0x0053B668u, {load<u32>(self + 512), 1});  // cmpw
    call(ctx, 0x00532450u, {load<u32>(self + 512), static_cast<u32>(self + 96), static_cast<u32>(self + 104)});
}

// void (Surface* s, u32 w, u32 h, u32 unused, s32 format, Target** out):
// Target_Init(&t, w, h, format, 0, 0) into a stack slot,
// Surface_SetBuffer(s, t), then *out = t when out is set
DECOMP(00537040, Surface_InitTarget_00537040) {
    const u32 s = arg<u32>(ctx, 0), out = arg<u32>(ctx, 5);
    Stack t(ctx, 16);  // 112(r1) of a 144-byte frame
    call(ctx, 0x00530DB4u, {t.addr(), ctx.gpr[4], ctx.gpr[5], ctx.gpr[7], 0, 0});
    call(ctx, 0x00532D5Cu, {s, load<u32>(t.addr())});
    if (out == 0) return;
    const u32 tv = load<u32>(t.addr());
    store<u32>(out, tv);
    ctx.gpr[3] = tv;
}

// void (Surface* s): f_00522140(s->+672), the draw-record list at
// 0x011DE300 built on first use (DrawRecordList_Init, guard u8 at
// 0x011DE480), then DrawRecordList_Wait on it
DECOMP(00548550, Sync_00548550) {
    call(ctx, 0x00522140u, {load<u32>(arg<u32>(ctx, 0) + 672)});
    if (load<u8>(0x011DE480u) == 0) {
        call(ctx, 0x004FD214u, {0x011DE300u});
        store<u8>(0x011DE480u, 1);
    }
    call(ctx, 0x005244A8u, {0x011DE300u});
}

// (this, a, b, c, d, e, f, g, h, i): f_0060DA10(this->+11980, a + 2, b - 2,
// c, d & 0xFFFF, e, f & 0xFFFF, g, h, i & 0xFFFF, 1) (the Edge zlib
// queue wrapper)  [vtable 00B245A4[10]]
DECOMP(007A6878, Call60DA10_007A6878) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 h = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116), i = load<u32>(static_cast<u32>(ctx.gpr[1]) + 124);
    call(ctx, 0x0060DA10u,
         {load<u32>(self + 11980), static_cast<u32>(ctx.gpr[4] + 2), static_cast<u32>(ctx.gpr[5] - 2), ctx.gpr[6],
          ctx.gpr[7] & 0xFFFF, ctx.gpr[8], ctx.gpr[9] & 0xFFFF, ctx.gpr[10], h, i & 0xFFFF, 1});
}

// bool (?, State* dst, const u8* src): dst->+0 = [0x00F67A08 + 8 * src[0]],
// +4 = [0x00F67A54 + 20 * src[2] + 4 * src[3]], u8 +8 = src word +8, +9 =
// src[1], +10 = src[4]; returns 1 (two RSX enum tables, name?)
DECOMP(0054A930, ConvertState_0054A930) {
    const u32 dst = arg<u32>(ctx, 1), src = arg<u32>(ctx, 2);
    store<u8>(dst + 8, static_cast<u8>(load<u32>(src + 8)));
    store<u32>(dst + 4, load<u32>(0x00F67A54u + 4 * load<u8>(src + 3) + 20 * load<u8>(src + 2)));
    store<u32>(dst, load<u32>(0x00F67A08u + 8 * load<u8>(src)));
    store<u8>(dst + 9, load<u8>(src + 1));
    const u32 last = load<u8>(src + 4);
    store<u8>(dst + 10, static_cast<u8>(last));
    ctx.gpr[5] = last;
    ret(ctx, 1u);
}

// PoolBase::~PoolBase: base vtable 0x00B236E8 (layers4.h kPoolBaseVtable),
// the buffers at +4 and +304 freed (no null check), then
// Pool_DestructPart(this + 12)  [vtable 00B236E8[2]]
DECOMP(0079ED18, PoolBase_Destruct) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 buf = load<u32>(self + 4);
    store<u32>(self, 0x00B236E8u);
    call(ctx, g8::fn::Mem_FreeFrom, {buf, 0, buf});
    call(ctx, g8::fn::Mem_FreeFrom, {load<u32>(self + 304), 0});
    call(ctx, 0x0052EFE0u, {static_cast<u32>(self + 12)});
}

// The 0x00B232C0 holder's deleting destructor: when set, vtable,
// Holder_FreeData(this + 16), base vtable 0x00B232B0, Mem_FreeFrom(this, 0)
// [vtable 00B232C0[1]]
DECOMP(007A0808, Holder232C0_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    store<u32>(self, 0x00B232C0u);
    call(ctx, g8::fn::Holder_FreeData, {static_cast<u32>(self + 16), static_cast<u32>(self + 16), 0x00B232C0u});
    store<u32>(self, 0x00B232B0u);
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0, 0x00B232B0u});
}

// void (this): sleeps 300 us (sys_timer_usleep) until this->+128 is 0,
// then f_00516CD0(this)
DECOMP(00517048, WaitThen_00517048) {
    const u32 self = arg<u32>(ctx, 0);
    while (load<u32>(self + 128) != 0) syscall(ctx, k8SysTimerUsleep, {300});
    call(ctx, 0x00516CD0u, {self});
}

namespace {
// The 0x00B23650 object's destructor: when +20 is set, vtable 0x00B23650
// and +20's virtual slot 2 on itself; then the base vtable 0x00B234C8
// (left in r3).
void obj23650_destruct(PPUContext& ctx) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 o = load<u32>(self + 20);
    ctx.gpr[4] = 0x00B23650u;
    if (o != 0) {
        store<u32>(self, 0x00B23650u);
        fx::vcall(ctx, o, 8, {o});
    }
    store<u32>(self, 0x00B234C8u);
    ctx.gpr[3] = 0x00B234C8u;
}
}  // namespace

// [vtable 00B23650[0]]
DECOMP(0051F3C0, Obj23650_Destruct) { obj23650_destruct(ctx); }

// The same body, called directly by f_007A3700.
DECOMP(0051F4B0, Obj23650_DestructInline) { obj23650_destruct(ctx); }

namespace {
// ApeTFreeList<T> deleting destructor: when set, the list's vtable,
// FreeList_Destroy, the base vtable 0x00B24298, FreeList_Destroy again,
// then Mem_FreePool(this, 0, 0).
// The renderer's lists free themselves with Mem_FreeFrom(this, 0) instead.
void freelist_delete(PPUContext& ctx, u32 vtable, bool pool = true) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    freelist_destruct(ctx, vtable);
    if (pool) call(ctx, g8::fn::Mem_FreePool, {self, 0, 0});
    else call(ctx, g8::fn::Mem_FreeFrom, {self, 0});
}
}  // namespace

DECOMP(007A4F88, PrimList_Delete) { freelist_delete(ctx, 0x00B24418u); }       // [vtable 00B24418[1]]
DECOMP(007A5044, ChunkDescList_Delete) { freelist_delete(ctx, 0x00B24438u); }  // [vtable 00B24438[1]]

// void (this, f32 v): this->f32 +64 = v clamped by the .text pair at
// 0x005202C0 {hi 5000.0, lo 10.0}: m = v < hi ? v : hi, then m > lo ? m :
// lo (NaN gives lo); the u8 +52 gets bit 0x80
DECOMP(005202C8, SetFactor_005202C8) {
    const u32 self = arg<u32>(ctx, 0);
    const f64 v = farg(ctx, 0), hi = lfs(0x005202C0u), lo = lfs(0x005202C4u);
    const u32 flags = load<u8>(self + 52);
    const f64 m = v < hi ? v : hi;
    f64 res, f1;
    if (m > lo) {
        f1 = m;
        res = m;
    } else {
        f1 = hi;
        res = lo;
    }
    store<f32>(self + 64, static_cast<f32>(res));
    store<u8>(self + 52, static_cast<u8>(flags | 0x80));
    ctx.fpr[1] = f1;
    ctx.fpr[2] = v;
    ctx.fpr[3] = res;
    ctx.fpr[4] = m;
}

// u32 (Obj* o, u32 flags, s32 n): flags bit 1 -> 10. Otherwise with
// base = bit 0 ? 2 : 4 and lo = bit 0: x = base - n * ((base + 1) / u8
// o->+20) (divw; 0 for a zero divisor), max(lo, x) signed, + 5 when bit 0
DECOMP(0052DAEC, Calc_0052DAEC) {
    const u32 flags = arg<u32>(ctx, 1);
    const bool odd = (flags & 1) != 0;
    const u32 base = odd ? 2 : 4, lo = odd ? 1 : 0;
    const u32 q = static_cast<u32>(rt::divw(base + 1, load<u8>(arg<u32>(ctx, 0) + 20)));
    const u32 x = base - arg<u32>(ctx, 2) * q;  // mullw, subfc
    u32 r = static_cast<s32>(lo) < static_cast<s32>(x) ? x : lo;  // cmpw
    if ((flags & 2) != 0) r = 10;
    else if (odd) r += 5;
    ret(ctx, r);
}

// void (): sets the display's u8 +136 when its +52 bits 22..24 exceed 1
// (signed), then Draw_BindTargets(*depth, *colour, 0) from
// Display_GetTargets (two zeroed stack slots)
DECOMP(00535C84, Display_BindScreenTargets) {
    const u32 disp = load<u32>(0x00F66FBCu);
    if (static_cast<s32>((load<u32>(disp + 52) >> 22) & 7) > 1) store<u8>(disp + 136, 1);
    Stack slots(ctx, 16);  // 112 / 116(r1) of a 128-byte frame
    const u32 colour = slots.addr(), depth = slots.addr() + 4;
    store<u32>(colour, 0);
    store<u32>(depth, 0);
    call(ctx, 0x005356F4u, {colour, depth, 0});
    call(ctx, 0x00532090u, {load<u32>(load<u32>(depth)), load<u32>(load<u32>(colour)), 0});
}

// The renderer's ApeTFreeList<T> deleting destructors (vtable slot 1 each).
DECOMP(007A2428, SurfaceList_Delete) { freelist_delete(ctx, 0x00B23D80u, false); }
DECOMP(007A24F0, TextureList_Delete) { freelist_delete(ctx, 0x00B23DA0u, false); }
DECOMP(007A2680, SubMeshList_Delete) { freelist_delete(ctx, 0x00B23DE0u, false); }
DECOMP(007A2748, TopologyList_Delete) { freelist_delete(ctx, 0x00B23E00u, false); }
DECOMP(007A2810, MeshMtlTableList_Delete) { freelist_delete(ctx, 0x00B23E20u, false); }
DECOMP(007A28D8, FreeList23E40_Delete) { freelist_delete(ctx, 0x00B23E40u, false); }
DECOMP(007A29A0, LightList_Delete) { freelist_delete(ctx, 0x00B23E60u, false); }
DECOMP(007A2A68, EmitPropsList_Delete) { freelist_delete(ctx, 0x00B23E80u, false); }
DECOMP(007A2B30, DeferredFreeList_Delete) { freelist_delete(ctx, 0x00B23EA0u, false); }

// u32 (IffReader* r, Obj* o): appends a zeroed 24-byte entry to o's array
// (++u16 +12; entry = +0 + 24 * (count - 1): u16 +0, u32 +4, u16 +8, u32
// +12 / +16 / +20 = 0), then Iff_ReadChunks(r, entry, 0); returns 1 (an
// IFF chunk handler)
DECOMP(00523F04, ReadChunkEntry_00523F04) {
    const u32 o = arg<u32>(ctx, 1);
    const u32 n = load<u16>(o + 12) + 1u;
    const u32 base = load<u32>(o);
    store<u16>(o + 12, static_cast<u16>(n));
    const u32 e = base + 24 * (n & 0xFFFF) - 24;
    store<u16>(e, 0);
    store<u32>(e + 4, 0);
    store<u16>(e + 8, 0);
    store<u32>(e + 12, 0);
    store<u32>(e + 16, 0);
    store<u32>(e + 20, 0);
    call(ctx, 0x00569388u, {ctx.gpr[3], e, 0});
    ctx.gpr[3] = 1;
}

// void (this): when u8 +20 and +28 are set: f_00549F30(this) unless the
// u8 +96 has bit 0x40 or 0x80, then f_0052CC88(this)
// [vtable 00B23D50[4], 00B24158[4], 00B24228[4]]
DECOMP(0054A420, Update_0054A420) {
    const u32 self = arg<u32>(ctx, 0);
    if (load<u8>(self + 20) == 0 || load<u32>(self + 28) == 0) return ret(ctx, 0u);
    if ((load<u8>(self + 96) & 0xC0) == 0) call(ctx, 0x00549F30u, {self});
    call(ctx, 0x0052CC88u, {self});
}

// bool (ShaderType*, Material* m, Mesh* mesh): unless g::config +152 bit
// 0x80, Material_ParamSlot(m, 0) = mesh->u8 +84 ? mesh->+88 : -1; returns 1
// [vtable 00B25798[1]]
DECOMP(005B8D58, SAC_FastPropType_SetupMaterial) {
    if ((load<u8>(0x010C5A20u + 152) & 0x80) != 0) return ret(ctx, 1u);
    const u32 mesh = arg<u32>(ctx, 2);
    const u32 v = load<u8>(mesh + 84) < 1 ? 0xFFFFFFFFu : load<u32>(mesh + 88);  // cmpwi
    const u32 slot = static_cast<u32>(call(ctx, 0x0050C018u, {ctx.gpr[4], 0, mesh, 0x010C5A20u}));
    store<u32>(slot, v);
    ret(ctx, 1u);
}

// void (this, const u32* src, u8 n): this->u8 +20 = n, then +4 + 4 * i =
// src[i] while (u8) i < this->u8 +20 (re-read before each store)
DECOMP(0052D018, SetList_0052D018) {
    const u32 self = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2) & 0xFF;
    store<u8>(self + 20, static_cast<u8>(n));
    if (n == 0) return;
    u32 i = 0, cnt;
    do {
        const u32 v = load<u32>(src + 4 * i);
        cnt = load<u8>(self + 20);
        store<u32>(self + 4 + 4 * i, v);
        i = (i + 1) & 0xFF;
    } while (static_cast<s32>(i) < static_cast<s32>(cnt));
}

namespace {
// ShaderType deleting destructor (the destructor inlined): when set, base
// vtable 0x00B23F00, PtrVector_Clear(this + 12), Nop(this + 12), then
// Mem_FreeFrom(this, 0).
void shadertype_delete(PPUContext& ctx) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    const u32 v = self + 12;
    store<u32>(self, 0x00B23F00u);
    ctx.gpr[4] = v;
    ctx.gpr[5] = 0x00B23F00u;
    call(ctx, g8::fn::PtrVector_Clear, {v});
    call(ctx, g8::fn::Nop, {v});
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0});
}
}  // namespace

DECOMP(007A34A4, ShaderType_Delete) { shadertype_delete(ctx); }       // [vtable 00B23F00[3]]
DECOMP(007A3D44, ShaderType241B0_Delete) { shadertype_delete(ctx); }  // [vtable 00B241B0[3]]

// Buffer24180_Destruct, then Mem_FreeFrom(this, 0); null returns 0
// [vtable 00B24180[1]]
DECOMP(007A3B74, Buffer24180_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    const u32 buf = load<u32>(self + 8);
    store<u32>(self, 0x00B24180u);
    call(ctx, g8::fn::Mem_FreePool, {buf, 4, 1, buf});
    store<u32>(self, 0x00B24110u);
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0, 0x00B24110u});
}

// u32 (IffReader* r, Obj* o): the u32 at 0x01164E48 = the size of the
// chunk on top of r's stack (Iff_ChunkSize inlined, with its read of
// address 8 for an empty stack), then Iff_ReadChunks(r, o) (an IFF chunk
// handler)
DECOMP(007A5208, IffChunk_ReadSized_007A5208) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 st = load<u32>(r);
    const s32 top = static_cast<s32>(load<u32>(st + 4));
    const u32 e = top > -1 ? st + 8 + 12 * static_cast<u32>(top) : 0u;
    const u32 size = load<u32>(e + 8);
    store<u32>(0x01164E48u, size);
    ctx.gpr[7] = 0x01160000u;
    call(ctx, 0x00569388u, {r, o, size, e});
}

// u32 (x): f_00569638(x, buf) into a 256-byte stack buffer; when that
// returns nonzero, String_Dup(buf) is appended to the u32 list at
// 0x010DDADC (+28 count, +32 entries). Returns f_00569638's result.
DECOMP(0051D198, ReadName_0051D198) {
    Stack buf(ctx, 256);  // 112(r1) of a 384-byte frame
    const u64 r = call(ctx, 0x00569638u, {ctx.gpr[3], buf.addr()});
    if (static_cast<u32>(r) != 0) {
        const u32 s = static_cast<u32>(call(ctx, 0x00556038u, {buf.addr()}));
        const u32 list = 0x010DDADCu, n = load<u32>(list + 28);
        store<u32>(list + 28, n + 1);
        store<u32>(list + 32 + 4 * n, s);
    }
    ctx.gpr[3] = r;
}

// void (): three RSX commands written inline at the gcm context's
// current pointer (no space check): colour mask 0x01010101, cull face
// enable 1, front face CCW (0x901)
DECOMP(00527D34, Gcm_ResetMaskCull_00527D34) {
    const u32 words[3][2] = {{0x00040324u, 0x01010101u}, {0x0004183Cu, 1u}, {0x00041834u, 0x901u}};
    for (const auto& w : words) {
        const u32 c = load<u32>(k8GcmState), cur = load<u32>(c + 8);
        store<u32>(cur, w[0]);
        store<u32>(cur + 4, w[1]);
        store<u32>(c + 8, cur + 8);
        ctx.gpr[3] = c;  // the last context load stays in r3
    }
}

// void (CallRec* rec): f_005E0600(gcm context, the RSX offset of rec->+0),
// then *rec->+4 = the offset of the context's current pointer | 0x20000000
// (cellGcmAddressToOffset into two stack slots)
DECOMP(00537D5C, CmdBuf_Call_00537D5C) {
    const u32 rec = arg<u32>(ctx, 0);
    Stack off(ctx, 16);  // 112 / 116(r1) of a 144-byte frame
    call(ctx, 0x00ABD5E4u, {load<u32>(rec), off.addr()});
    call(ctx, 0x005E0600u, {load<u32>(k8GcmState), load<u32>(off.addr())});
    call(ctx, 0x00ABD5E4u, {load<u32>(load<u32>(k8GcmState) + 8), off.addr() + 4});
    const u32 v = load<u32>(off.addr() + 4) | 0x20000000u;
    store<u32>(load<u32>(rec + 4), v);
    ctx.gpr[3] = v;
}

// void (Texture* t, const Header* h): InitFromHeader_0050A28C(t, h), then,
// when the u32 at 0x01164DEC (obj13 kBankState) is set, Texture_FromGtf(t,
// bank->+8 + h->+44, h->+48, 1)
DECOMP(0053A94C, Texture_LoadFromHeader_0053A94C) {
    const u32 t = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    call(ctx, 0x0050A28Cu, {t, h});
    const u32 bank = load<u32>(0x01164DECu);
    if (bank == 0) return ret(ctx, 0u);
    call(ctx, 0x0053A448u, {t, static_cast<u32>(load<u32>(bank + 8) + load<u32>(h + 44)), load<u32>(h + 48), 1});
}

// u32 (ShaderType* t, Material* m, DrawItem* item): item->u8 +69 = 1 when
// its material (+28) has u8 +100 bit 0x80 and item->+44 is set (sel =
// *item->+44 + 4), else 0 (sel = 0); returns m->+16->+12 [2 * sel + (m->+128
// != 0)]  [vtable 00B26118[4]]
DECOMP(005D7064, SHO_DefaultType_SelectShader) {
    const u32 m = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 mat = load<u32>(item + 28);
    store<u8>(item + 69, 0);
    u32 sel = 0;
    if ((load<u8>(mat + 100) & 0x80) != 0) {
        const u32 p = load<u32>(item + 44);
        if (p != 0) {
            sel = load<u32>(p) + 4;
            store<u8>(item + 69, 1);
        }
    }
    const u32 idx = (sel << 1) | (load<u32>(m + 128) != 0 ? 1u : 0u);  // rlwimi
    ctx.gpr[5] = idx;
    ret(ctx, load<u32>(load<u32>(load<u32>(m + 16) + 12) + 4 * idx));
}

// The 0x00B232D8 holder's deleting destructor: when set, vtable,
// EmitPropsRef_Set(this + 4, 0), f_0079CA4C(this + 4), then
// Mem_FreeFrom(this, 0)  [vtable 00B232D8[1]]
DECOMP(0050059C, EmitPropsHolder_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    const u32 ref = self + 4;
    store<u32>(self, 0x00B232D8u);
    call(ctx, g8::fn::EmitPropsRef_Set, {ref, 0, ref, 0x00B232D8u});
    call(ctx, 0x0079CA4Cu, {ref});
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0});
}

// void (f32 gamma): clamped to [0.8, 1.2] (k at 0x0052F898; NaN stays);
// when it differs from render::g::mem_stats +56 it is stored there and
// cellVideoOutSetGamma(0, gamma) is called
DECOMP(0052F8A0, Display_SetGamma) {
    const f64 lo = lfs(0x0052F898u), hi = lfs(0x0052F89Cu);
    f64 v = farg(ctx, 0);
    if (lo > v) v = lo;
    else if (hi < v) v = hi;
    const f64 cur = lfs(0x010F0E20u + 56);
    ctx.fpr[1] = v;
    ctx.fpr[2] = cur;
    ctx.fpr[3] = lo;
    ctx.gpr[3] = 0x010F0E20u;
    if (cur == v) return;
    store<f32>(0x010F0E20u + 56, static_cast<f32>(v));
    ctx.fpr[2] = v;
    callf(ctx, 0x00ABE3A4u, {0, 0x010F0E20u}, {v});
}

namespace {
// The stream reader's align step (f_005159E8's records): p + need past
// the end (+4, unsigned) wraps to the start (+0); then aligned up to 4.
u32 wrap_align(u32 p, u32 need, u32 start, u32 end) {
    if (p + need > end) p = start;  // cmplw
    return (p + 3) & ~3u;
}
}  // namespace

// u16 (Stream* s, u32* a, u32* b, u32* c, bool* d): skips a 12-byte header
// (wrapping at +4 to +0), then reads a u64 at the cursor (+12, += 8) and
// stores its four 16-bit fields from the top as a, b, c and d != 0;
// returns c
DECOMP(00515168, Stream_Read4x16_00515168) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 p = load<u32>(s + 12), end = load<u32>(s + 4), start = load<u32>(s);
    const u32 h = wrap_align(p, 15, start, end);
    const u32 q = (h + 23 > end ? start : h + 12);
    const u32 at = (q + 3) & ~3u;
    store<u32>(s + 12, at + 8);
    const u64 d = load<u64>(at);
    const u32 a = static_cast<u32>(d >> 48), b = static_cast<u32>(d >> 32) & 0xFFFF;
    const u32 c = static_cast<u32>(d >> 16) & 0xFFFF, flag = (d & 0xFFFF) != 0 ? 1 : 0;
    store<u32>(arg<u32>(ctx, 1), a);
    store<u32>(arg<u32>(ctx, 2), b);
    store<u32>(arg<u32>(ctx, 3), c);
    store<u8>(arg<u32>(ctx, 4), static_cast<u8>(flag));
    ctx.gpr[4] = flag;
    ret(ctx, c);
}

// u32 (Stream* s, u32* out): skips a 12-byte header (wrapping at +4 to
// +0), then reads two u32 words at the cursor (+12, each wrapping the same
// way); *out = the second, returns the first
DECOMP(00515544, Stream_ReadPair_00515544) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 p = load<u32>(s + 12), end = load<u32>(s + 4), start = load<u32>(s);
    const u32 h = wrap_align(p, 15, start, end);
    const u32 at = (h + 19 > end ? start : h + 12);
    const u32 a = (at + 3) & ~3u;
    store<u32>(s + 12, a + 4);
    const u32 first = load<u32>(a);
    const u32 b = ((a + 11 > end ? start : a + 4) + 3) & ~3u;
    const u32 second = load<u32>(b);
    store<u32>(s + 12, b + 4);
    store<u32>(arg<u32>(ctx, 1), second);
    ret(ctx, first);
}

// u32 (): the thread-id static (load.h kThreadIds, zeroed and guarded on
// first use); on the main thread (ThreadId_005DB070 == the first u64) the
// u32 at 0x00F66FC4, on any other its ->+672
DECOMP(00522C98, GetForThread_00522C98) {
    const u64 tid = call(ctx, 0x005DB070u);
    if (load<u8>(0x011DE2A0u) == 0) {
        store<u64>(0x011DE290u, 0);
        store<u64>(0x011DE298u, 0);
        store<u8>(0x011DE2A0u, 1);
    }
    const u32 obj = load<u32>(0x00F66FC4u);
    ret(ctx, tid == load<u64>(0x011DE290u) ? obj : load<u32>(obj + 672));  // cmpd
}

// void (): the u32 at 0x010E0614 (gfx7_thunks.cpp kList010E05D4Count) = 0;
// a material copied from Shader_LoadGroup(0)'s (Material_Alloc,
// +16 = the group's +16, Material_CopyFrom) goes to the u32 at 0x010DFAC0,
// with bit 31 of its +112 cleared  [called by Query_InitReports]
DECOMP(00527B28, Query_InitProxyMaterial) {
    store<u32>(0x010E0614u, 0);
    const u32 sh = static_cast<u32>(call(ctx, 0x0050F0F0u, {0, 0x010E0000u, 0}));
    const u32 tex = load<u32>(sh + 16);
    const u32 m = static_cast<u32>(call(ctx, 0x004FF2E8u, {sh}));
    store<u32>(m + 16, tex);
    call(ctx, 0x00509090u, {m, sh});
    store<u32>(0x010DFAC0u, m);
    const u32 flags = load<u32>(m + 112) & 0x7FFFFFFFu;
    store<u32>(m + 112, flags);
    ctx.gpr[3] = flags;
    ctx.gpr[4] = m + 104;
}

// PoolBase deleting destructor: PoolBase_Destruct inlined, then
// Mem_FreeFrom(this, 0)  [vtable 00B236E8[3]]
DECOMP(007A2194, PoolBase_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    const u32 buf = load<u32>(self + 4);
    store<u32>(self, 0x00B236E8u);
    call(ctx, g8::fn::Mem_FreeFrom, {buf, 0, buf});
    call(ctx, g8::fn::Mem_FreeFrom, {load<u32>(self + 304), 0});
    call(ctx, 0x0052EFE0u, {static_cast<u32>(self + 12)});
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0});
}

// The 0x00B23650 object's deleting destructor: Obj23650_Destruct inlined,
// then Mem_FreeFrom(this, 0)  [vtable 00B23650[1]]
DECOMP(0051F42C, Obj23650_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    obj23650_destruct(ctx);
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0, 0x00B234C8u});
}

// void (): when the render object at 0x010C5A10 (SignalRender_00512BE0)
// has u8 +48 set: sys_event_flag_wait(obj->+28, 1, mode 1, 0, 0) and, when
// that returns 0, sys_event_flag_clear(obj->+28, 0); then obj->u8 +48 = 0
DECOMP(0051265C, WaitRender_0051265C) {
    const u32 obj = load<u32>(k8RenderEvent);
    if (load<u8>(obj + 48) == 0) return ret(ctx, 0u);
    const u32 ef = obj + 24;
    const u64 r = syscall(ctx, 85, {load<u32>(ef + 4), 1, 1, 0, 0});  // sys_event_flag_wait
    if (static_cast<u32>(r) == 0) syscall(ctx, 118, {load<u32>(ef + 4), 0});  // sys_event_flag_clear
    const u32 o2 = load<u32>(k8RenderEvent);
    store<u8>(o2 + 48, 0);
    ctx.gpr[3] = o2;
    ctx.gpr[4] = 0;
}

// void (f32 a, f32 b): mem_stats +168 = clamp(a, 0.1, 150), +172 =
// clamp(b, 0.01, 0.1) (k at 0x00536A2C: 0.1, 0.01, 150; NaN stays) -- the
// two SetClamp* setters of gfx7_misc.cpp in one
DECOMP(00536A38, SetClamps_00536A38) {
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    const f64 k0 = lfs(0x00536A2Cu), k1 = lfs(0x00536A30u);
    f64 x;
    if (a < k0) x = k0;
    else {
        const f64 k2 = lfs(0x00536A34u);
        x = a > k2 ? k2 : a;
        ctx.fpr[4] = k2;
    }
    store<f32>(0x010F0E20u + 168, static_cast<f32>(x));
    f64 y;
    if (b < k1) y = k1;
    else y = b > k0 ? k0 : b;
    store<f32>(0x010F0E20u + 172, static_cast<f32>(y));
    ctx.fpr[1] = k0;
    ctx.fpr[2] = y;
    ctx.fpr[3] = b;
    ctx.gpr[3] = 0x010F0E20u;
    ctx.gpr[4] = 0x00536A2Cu;
}

namespace {
// The effects' Draw sequence: virtual slots 6 then 8 on this.
void effect_slots_6_8(PPUContext& ctx, u32 self) {
    fx::vcall(ctx, self, 24, {self});
    fx::vcall(ctx, self, 32, {self});
}
}  // namespace

// void (Effect* e): unless e->f32 +24 < 0.01 (k at 0x0053EC50; NaN
// runs), virtual slots 6 and 8 on e  [vtable 00B23F60[4]]
DECOMP(0053EC58, Effect23F60_Run) {
    const u32 self = arg<u32>(ctx, 0);
    ctx.fpr[1] = lfs(self + 24);
    ctx.fpr[2] = lfs(0x0053EC50u);
    if (ctx.fpr[1] < ctx.fpr[2]) return;
    effect_slots_6_8(ctx, self);
}

// void (Effect* e): e->+68 / +72 = View_GetCurrent() +60 / +64 (raw), then
// virtual slots 6 and 8 on e  [vtable 00B23F90[4]]
DECOMP(00540D1C, Effect23F90_Run) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 v = static_cast<u32>(call(ctx, 0x004FDBA8u, {self}));
    const u32 x = load<u32>(v + 60), y = load<u32>(v + 64);
    store<u32>(self + 68, x);
    store<u32>(self + 72, y);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(x));
    ctx.fpr[2] = static_cast<f64>(std::bit_cast<f32>(y));
    effect_slots_6_8(ctx, self);
}

// void (Effect* e, u32 src, u32 dst): Draw_SetTarget(e->+136[dst], 0),
// cellGcmSetInvalidateTextureCache(gcm context, 1),
// Draw_SetTexture0(e->+148[src], 0, 1), Draw_PrepareQuad()
// [vtable 00B23FF0[12]]
DECOMP(00543304, Effect23FF0_Pass) {
    const u32 self = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), dst = arg<u32>(ctx, 2);
    call(ctx, 0x005373A8u, {load<u32>(self + 4 * dst + 136), 0});
    call(ctx, 0x005E3E64u, {load<u32>(k8GcmState), 1});
    call(ctx, 0x00536AC0u, {load<u32>(self + 4 * src + 148), 0, 1});
    call(ctx, 0x0053539Cu);
}

// (File* f, x): r = virtual slot 3 (f, x), then virtual slot 5 (f, r,
// x + 24)  [vtable 00B245A4[2]]
DECOMP(007A601C, Call12Then20_007A601C) {
    const u32 f = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u64 r = fx::vcall(ctx, f, 12, {f, x});
    fx::vcall(ctx, f, 20, {f, r, static_cast<u32>(x + 24)});
}

// void (): for the stats object at 0x010DDBBC: +220 = min(+220, [+384] -
// +240) and +224 = min(+224, [+512] - +244) (signed), the counters at +384
// and +512 atomically cleared (lwarx / stwcx. loops), then +240, +232,
// +244, +236 = 0
DECOMP(00510554, Stats_ResetCounters_00510554) {
    const u32 o = load<u32>(0x010DDBBCu);
    const u32 a = o + 384, b = o + 512;
    const u32 base_a = load<u32>(o + 240), base_b = load<u32>(o + 244);
    const s32 min_a = static_cast<s32>(load<u32>(o + 220));
    const s32 da = static_cast<s32>(load<u32>(a) - base_a), db = static_cast<s32>(load<u32>(b) - base_b);
    store<u32>(o + 220, static_cast<u32>(min_a < da ? min_a : da));
    const s32 min_b = static_cast<s32>(load<u32>(o + 224));
    store<u32>(o + 224, static_cast<u32>(min_b < db ? min_b : db));
    std::atomic_ref<u32>(ref<u32>(a)).exchange(0);  // 0 is the same in either byte order
    const u32 last = __builtin_bswap32(std::atomic_ref<u32>(ref<u32>(b)).exchange(0));  // big-endian raw
    store<u32>(o + 240, 0);
    store<u32>(o + 232, 0);
    store<u32>(o + 244, 0);
    store<u32>(o + 236, 0);
    ctx.gpr[3] = last;  // the second lwarx
    ctx.gpr[4] = 0;
}

// u32 (Stream* s, u32* a, u32* b): skips a 12-byte header (wrapping at +4
// to +0), reads a word into *a, then another (wrapping when the cursor +
// 7 passes the end) into *b; returns *b
DECOMP(00514CC8, Stream_ReadTwo_00514CC8) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 p = load<u32>(s + 12), end = load<u32>(s + 4), start = load<u32>(s);
    const u32 h = wrap_align(p, 15, start, end);
    const u32 a = ((h + 19 > end ? start : h + 12) + 3) & ~3u;
    store<u32>(s + 12, a + 4);
    store<u32>(arg<u32>(ctx, 1), load<u32>(a));
    const u32 end2 = load<u32>(s + 4);
    u32 q = load<u32>(s + 12);
    if (q + 7 > end2) q = load<u32>(s);  // cmplw
    const u32 b = (q + 3) & ~3u;
    store<u32>(s + 12, b + 4);
    const u32 v = load<u32>(b);
    store<u32>(arg<u32>(ctx, 2), v);
    ret(ctx, v);
}

// void (Stream* s): skips a 12-byte header (wrapping at +4 to +0), reads
// an f32 width, then writes the RSX command SET_LINE_WIDTH (0x000403B8)
// = (s32)(width * 8.0) (k at 0x00515014; fctiwz) inline at the gcm
// context's current pointer (no space check)
DECOMP(00515018, Stream_SetLineWidth_00515018) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 p = load<u32>(s + 12), end = load<u32>(s + 4), start = load<u32>(s);
    const u32 h = wrap_align(p, 15, start, end);
    const u32 a = ((h + 19 > end ? start : h + 12) + 3) & ~3u;
    store<u32>(s + 12, a + 4);
    const f32 w = static_cast<f32>(lfs(a) * lfs(0x00515014u));  // fmuls
    const u32 c = load<u32>(k8GcmState), cur = load<u32>(c + 8);
    store<u32>(cur, 0x000403B8u);
    const u64 iw = rt::fctiw(w, true);
    store<u32>(cur + 4, static_cast<u32>(iw));  // stfiwx
    store<u32>(c + 8, cur + 8);
    ctx.fpr[1] = std::bit_cast<f64>(iw);  // the fctiwz result stays in f1
    ctx.fpr[2] = w;
    ctx.gpr[3] = c;
}

// Shader* (ShaderType* t, Material*, DrawItem* item): item->u8 +69 = 0;
// returns t->+12 [skinned + 2 * colour], where skinned = mesh (item->+12)
// +112 and +84 both set, and colour = the mesh's vertex format (+68,
// 12-byte entries until +4 == 0) has an entry of kind 2 with u8 +9 == 0
// [vtable 00B241B0[4]]
DECOMP(0054DAB4, ShaderType241B0_SelectShader) {
    const u32 t = arg<u32>(ctx, 0), item = arg<u32>(ctx, 2);
    const u32 mesh = load<u32>(item + 12);
    const u32 skinned = (load<u32>(mesh + 112) != 0 && load<u32>(mesh + 84) != 0) ? 1 : 0;
    u32 colour = 0;
    for (u32 e = load<u32>(mesh + 68); load<u32>(e + 4) != 0; e += 12) {
        if (load<u32>(e) == 2 && load<u8>(e + 9) == 0) {  // cmpwi
            if (e != 0) colour = 2;
            break;
        }
    }
    store<u8>(item + 69, 0);
    ctx.gpr[4] = 0;
    ret(ctx, load<u32>(load<u32>(t + 12) + 4 * (skinned + colour)));
}

// void (Anims* a): once per frame (a->+0 != layers4 g::anim_stamp):
// +0 = the stamp, then MaterialAnim_Update on the tracks at +8, +56 and
// +104 with dt = anim_rate * anim_scale (fmuls)
DECOMP(00501780, MaterialAnim3_Update_00501780) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 stamp = load<u32>(0x010DDA7Cu);
    if (load<u32>(a) == stamp) return ret(ctx, stamp);  // cmpw
    store<u32>(a, stamp);
    const f64 dt = static_cast<f32>(lfs(0x010DD5ACu) * lfs(0x010DD5B4u));
    for (u32 off = 8; off <= 104; off += 48) callf(ctx, 0x00501520u, {static_cast<u32>(a + off)}, {dt});
}

namespace {
// The surface base destructor: vtable 0x00B234B0 (surface_alloc.h
// vt_surface_base), f_00537684 on each set handle slot at +16 / +20 / +24,
// then ApeRoot_Destruct.
void surfacebase_destruct(PPUContext& ctx) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 first = load<u32>(self + 16);
    store<u32>(self, 0x00B234B0u);
    ctx.gpr[4] = 0x00B234B0u;
    ctx.gpr[5] = first;
    for (u32 off = 16; off <= 24; off += 4)
        if (load<u32>(self + off) != 0) call(ctx, 0x00537684u, {static_cast<u32>(self + off)});
    call(ctx, 0x00558CE8u, {self});
}
}  // namespace

DECOMP(0051AC50, SurfaceBase_Destruct) { surfacebase_destruct(ctx); }  // [vtable 00B234B0[0]]

// The same body, called directly by RenderSurface_Destruct / _Delete.
DECOMP(0051AD90, SurfaceBase_DestructBody) { surfacebase_destruct(ctx); }

// bool (ShaderType*, Material* m, const Desc* d): t = Tex_FindLoaded(d->+4);
// when found, t's virtual slot 4 (t, 3, 3, 1) and f_004FF008(m, t),
// returning 1; else 0  [vtable 00B255C0[1]]
DECOMP(005B5880, AngleIndexType_SetupMaterial) {
    const u32 m = arg<u32>(ctx, 1);
    const u32 t = static_cast<u32>(call(ctx, 0x0050C398u, {load<u32>(arg<u32>(ctx, 2) + 4)}));
    if (t == 0) return ret(ctx, 0u);
    fx::vcall(ctx, t, 16, {t, 3, 3, 1});
    call(ctx, 0x004FF008u, {m, t});
    ret(ctx, 1u);
}

// void (Effect* e): when virtual slot 3 (e) returns nonzero, slots 7 and
// 8 on e  [vtable 00B23FC0[4]]
DECOMP(00542598, Effect23FC0_Run) {
    const u32 self = arg<u32>(ctx, 0);
    if (static_cast<u32>(fx::vcall(ctx, self, 12, {self})) == 0) return;
    fx::vcall(ctx, self, 28, {self});
    fx::vcall(ctx, self, 32, {self});
}

// The Edge zlib inflate queue wrapper's destructor (vtable 0x00B24558):
// u8 +11992 = 0; when the event flag is attached (+11972),
// cellSpursEventFlagDetachLv2EventQueue(this + 10624); when the taskset
// (+11976) exists, cellSpursShutdownTaskset + cellSpursJoinTaskset on it
// and +11976 = 0; then the base vtable 0x00B2451C  [vtable 00B24558[0]]
DECOMP(007A60A0, InflateQueue_Destruct) {
    const u64 saved = ctx.gpr[3];  // std r3, 112(r1) around the imports
    const u32 self = arg<u32>(ctx, 0);
    const u32 ef = load<u32>(self + 11972);
    store<u32>(self, 0x00B24558u);
    store<u8>(self + 11992, 0);
    if (ef != 0) call(ctx, 0x00ABDF84u, {static_cast<u32>(self + 10624), static_cast<u32>(self + 10624), 0, ef});
    const u32 ts = load<u32>(self + 11976);
    if (ts != 0) {
        call(ctx, 0x00ABE144u, {ts});
        call(ctx, 0x00ABE104u, {load<u32>(self + 11976)});
        store<u32>(self + 11976, 0);
    }
    store<u32>(self, 0x00B2451Cu);
    ctx.gpr[3] = saved;
    ctx.gpr[4] = 0x00B2451Cu;
}

// void (): when the current surface's view (draw::g::current_surface
// ->+672) shows the display at 0x00F66FBC and that display's u8 +148
// lacks bit 0x80: f_005358A8(3), Display_BindScreenTargets(),
// f_00532450(view->+512, view + 96, view + 104), then the two u64 at
// 0x010DD598 / 0x010DD5A0 = -1
DECOMP(00518574, View_Restore_00518574) {
    const u32 view = load<u32>(load<u32>(0x010DDA74u) + 672);
    const u32 disp = load<u32>(0x00F66FBCu), shown = load<u32>(view + 512);
    if (shown != disp) return ret(ctx, shown);  // cmpw
    const u32 bit = (load<u8>(shown + 148) >> 7) & 1;
    if (bit != 0) return ret(ctx, bit);
    call(ctx, 0x005358A8u, {3});
    call(ctx, 0x00535C84u);
    call(ctx, 0x00532450u, {load<u32>(view + 512), static_cast<u32>(view + 96), static_cast<u32>(view + 104)});
    store<u64>(0x010DD598u, ~0ull);
    store<u64>(0x010DD5A0u, ~0ull);
    ctx.gpr[3] = 0x010E0000u;
    ctx.gpr[4] = ~0ull;
}

// f32 (Camera* c): tanf(c->f32 +56 * 0.5) / tanf(0.48) (the latter cached
// once at 0x010F0DD0, guard u8 0x010F0DC8), capped at 1.0 by fsel (NaN
// passes); k at 0x005206E8: 0.48, 0.5, 1.0
DECOMP(005206F8, Camera_FovScale_005206F8) {
    const u32 c = arg<u32>(ctx, 0);
    if (load<u8>(0x010F0DC8u) == 0) {
        const f64 t = callf(ctx, 0x005E62E8u, {}, {lfs(0x005206E8u)});
        store<f32>(0x010F0DD0u, static_cast<f32>(t));
        store<u8>(0x010F0DC8u, 1);
    }
    const f64 half = static_cast<f32>(lfs(c + 56) * lfs(0x005206ECu));  // fmuls
    const f64 t = callf(ctx, 0x005E62E8u, {}, {half});
    const f64 r = static_cast<f32>(t / lfs(0x010F0DD0u));  // fdivs
    const f64 cap = lfs(0x005206F0u);
    const f64 d = static_cast<f32>(r - cap);  // fsubs
    ctx.fpr[1] = d >= 0.0 ? cap : r;  // fsel
    ctx.gpr[3] = 0x010F0000u;
    ctx.fpr[2] = cap;
    ctx.fpr[3] = d;
}

// Texture* (Texture* t, const char* path): File_LoadWhole(path, &size, 0);
// with data and a nonzero size, r = Texture_FromGtf(t, data, size, 0),
// Mem_Free(data), returns r; otherwise f_005398CC(t) and 0 (quirk: data
// with size 0 is not freed)
DECOMP(0053A8B0, Texture_LoadFile_0053A8B0) {
    const u32 t = arg<u32>(ctx, 0);
    Stack size(ctx, 16);  // 112(r1) of a 144-byte frame
    store<u32>(size.addr(), 0);
    const u32 data = static_cast<u32>(call(ctx, 0x00560B90u, {ctx.gpr[4], size.addr(), 0, size.addr()}));
    const u32 n = data != 0 ? load<u32>(size.addr()) : 0u;
    if (data == 0 || n == 0) {
        call(ctx, 0x005398CCu, {t});
        return ret(ctx, 0u);
    }
    const u64 r = call(ctx, 0x0053A448u, {t, data, n, 0});
    call(ctx, 0x005611B0u, {data});
    ctx.gpr[3] = r;
}

// void (ShaderType*, Material* m): MaterialAnim_UploadUV(m->+128, 4, 1.0,
// 1.0) when m has one, then vertex constant 6 = {1, 1, 1, 1} (a static
// f32[4] at 0x011972D0, filled once; guard u8 0x011972C0) via
// cellGcmSetVertexProgramConstants  [vtable entry 0x00B261A0]
DECOMP(005D8060, Bind_005D8060) {
    const u32 anim = load<u32>(arg<u32>(ctx, 1) + 128);
    if (anim != 0) {
        const f64 one = lfs(0x005D805Cu);
        callf(ctx, 0x0050D124u, {anim, 4, 0x005D805Cu}, {one, one});
    }
    if (load<u8>(0x011972C0u) == 0) {
        for (u32 k = 0; k < 4; ++k) store<u32>(0x011972D0u + 4 * k, 0x3F800000u);
        store<u8>(0x011972C0u, 1);
    }
    call(ctx, 0x005E5CB0u, {load<u32>(k8GcmState), 6, 4, 0x011972D0u});
}

// void (Slot* s, u32 a, const f32 m[16]): one occlusion-query slot (pool.h
// query): +8 = a, +16.. = m's rows 0..3, three words each at a 16-byte
// stride (raw copies), the 80 bytes at +80 (stvx, 16-byte aligned) = 0,
// u16 +160 (current entry) = 0
DECOMP(005485F4, Slot_Setup) {
    const u32 s = arg<u32>(ctx, 0), m = arg<u32>(ctx, 2);
    for (u32 row = 0; row < 4; ++row) {
        if (row == 2) store<u32>(s + 8, arg<u32>(ctx, 1));  // between rows 1 and 2, as the original
        for (u32 k = 0; k < 3; ++k) store<u32>(s + 16 + 16 * row + 4 * k, load<u32>(m + 16 * row + 4 * k));
    }
    const u32 z = (s + 80) & ~15u;
    for (u32 k = 0; k < 80; k += 4) store<u32>(z + k, 0);
    store<u16>(s + 160, 0);
    ctx.fpr[1] = static_cast<f64>(load<f32>(m + 56));
    ctx.gpr[4] = s + 80;
}

// bool (ShaderType*, Material* m, Mesh* mesh): unless g::config +152 bit
// 0x80, Material_ParamSlot(m, 0) = mesh->+88 and (m, 1) = mesh->+92 (raw
// f32 words); returns 1  [vtable 00B25A48[1]]
DECOMP(005C1D1C, SlyCharacterFurFinType_SetupMaterial) {
    if ((load<u8>(0x010C5A20u + 152) & 0x80) != 0) return ret(ctx, 1u);
    const u32 m = arg<u32>(ctx, 1), mesh = arg<u32>(ctx, 2);
    for (u32 i = 0; i < 2; ++i) {
        const u32 v = load<u32>(mesh + 88 + 4 * i);
        const u32 slot = static_cast<u32>(call(ctx, 0x0050C018u, {m, i, static_cast<u32>(mesh + 88 + 4 * i)}));
        store<u32>(slot, v);
    }
    ret(ctx, 1u);
}

// void (): the proxy material at 0x010DFAC0 (Query_InitProxyMaterial)
// released (virtual slot 2) and cleared, then every query slot in use (u8
// +4 of the 16 at layers3::g::slots) appended to the list at 0x010E05D4
// (count 0x010E05D4 + 64, unchecked)  [called by Call_005489CC]
DECOMP(00527BA8, Query_CollectSlots_00527BA8) {
    const u32 m = load<u32>(0x010DFAC0u);
    if (m != 0) {
        fx::vcall(ctx, m, 8, {m});
        store<u32>(0x010DFAC0u, 0);
    }
    for (u32 i = 0; i < 16; ++i) {
        const u32 slot = 0x010DFAD0u + 176 * i;
        if (load<u8>(slot + 4) == 0) continue;
        const u32 n = load<u32>(0x010E0614u);
        store<u32>(0x010E0614u, n + 1);
        store<u32>(0x010E05D4u + 4 * n, slot);
    }
    ctx.gpr[3] = 0x010DFAD0u + 176 * 16;
}

// void (): WaitRender_0051265C inlined (when the render object's u8 +48 is
// set: event flag wait / clear, +48 = 0), f_0053BF70(), then f_00512148()
// until it returns nonzero
DECOMP(005126E4, WaitRenderIdle_005126E4) {
    const u32 obj = load<u32>(k8RenderEvent);
    if (load<u8>(obj + 48) != 0) {
        const u32 ef = obj + 24;
        const u64 r = syscall(ctx, 85, {load<u32>(ef + 4), 1, 1, 0, 0});  // sys_event_flag_wait
        if (static_cast<u32>(r) == 0) syscall(ctx, 118, {load<u32>(ef + 4), 0});  // sys_event_flag_clear
        store<u8>(load<u32>(k8RenderEvent) + 48, 0);
    }
    call(ctx, 0x0053BF70u);
    while (static_cast<u32>(call(ctx, 0x00512148u)) == 0) {
    }
}

// void (this): two steps of the game's random generator (update.h
// Random_Step, inlined with one store each at the end): this->f32 +64 =
// the previous kRandomLast, +68 = the first step's 16 bits, each times
// 1/65535 (k at 0x0051F65C; frsp, fmuls)
DECOMP(0051F660, Random_Pair_0051F660) {
    const auto rotl = [](u32 v, int n) { return (v << n) | (v >> (32 - n)); };
    const u32 self = arg<u32>(ctx, 0);
    const u64 last = load<u64>(0x01164888u);
    const u32 x = load<u32>(0x01164890u);
    const f64 k = lfs(0x0051F65Cu);
    const u32 mix1 = (rotl(x, 4) + rotl(x, 16)) ^ 0x87654321u;
    const u32 r1 = mix1 & 0xFFFF;
    const u32 mix2 = (rotl(mix1, 20) + mix1) ^ 0x87654321u;
    store<u32>(0x01164890u, rotl(mix2, 16));
    store<u64>(0x01164888u, mix2 & 0xFFFF);
    const f32 a = static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(static_cast<s64>(last)))) * k);
    const f32 b = static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(r1))) * k);
    store<f32>(self + 64, a);
    store<f32>(self + 68, b);
    ctx.fpr[1] = b;
    ctx.fpr[2] = a;
    ctx.gpr[3] = self + 68;
}

// void (Stream* w, u8 v): unless v is w->u8 +200 already: +200 = v, then a
// 12-byte record {0x11000000, v, 0} and the byte v are appended at the
// cursor (+16), each wrapping to the start (+0) when it would pass the
// end (+4) -- the writer side of the Stream_Read* records
DECOMP(005234DC, Stream_PutByteCmd_005234DC) {
    const u32 w = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1) & 0xFF;
    if (v == load<u8>(w + 200)) return;  // cmpw
    store<u8>(w + 200, static_cast<u8>(v));
    u32 p = load<u32>(w + 16);
    if (p + 15 > load<u32>(w + 4)) p = load<u32>(w);  // cmplw
    p = (p + 3) & ~3u;
    store<u32>(w + 16, p);
    store<u32>(p + 8, 0);
    store<u32>(p, 0x11000000u);
    store<u32>(p + 4, v);
    const u32 end = load<u32>(w + 4), q = load<u32>(w + 16);
    const u32 b = ((q + 16 > end ? load<u32>(w) : q + 12) + 3) & ~3u;
    store<u32>(w + 16, b);
    store<u8>(b, static_cast<u8>(v));
    store<u32>(w + 16, load<u32>(w + 16) + 1);
}

// void (): f_0053BC14(), f_005E0600(gcm context, the u32 at 0x010F0E1C),
// the gcm context pointer (fx::g::state) = the u32 at 0x011972EC; when the
// double-buffer object at 0x010F0E08 has its current slot (+284) set in
// +268[], the slot flips (+284 ^= 1; the original also takes lwarx
// reservations on +260 / +256 without storing); then the u8 at 0x010F0F98 =
// 1
DECOMP(0053BE94, Gcm_FlipContext_0053BE94) {
    call(ctx, 0x0053BC14u);
    const u32 a = load<u32>(0x010F0E1Cu);
    call(ctx, 0x005E0600u, {load<u32>(k8GcmState), a, a});
    store<u32>(k8GcmState, load<u32>(0x011972ECu));
    const u32 o = load<u32>(0x010F0E08u);
    const u32 idx = load<u32>(o + 284);
    if (load<u32>(o + 4 * idx + 268) != 0) store<u32>(o + 284, load<u32>(o + 284) ^ 1);
    store<u8>(0x010F0F98u, 1);
    ctx.gpr[3] = 0x010F0000u;
    ctx.gpr[4] = 1;
}

// SurfaceBase deleting destructor: SurfaceBase_Destruct inlined, then
// Mem_FreeFrom(this, 0)  [vtable 00B234B0[1]]
DECOMP(0051ACE4, SurfaceBase_Delete) {
    const u32 self = arg<u32>(ctx, 0);
    if (self == 0) return;
    surfacebase_destruct(ctx);
    call(ctx, g8::fn::Mem_FreeFrom, {self, 0});
}

// void (): makes the texture for the active mode once: with g::config +34
// set, the u32 at 0x00F67504 = f_0051E514(mem_stats +100, 0) unless set
// or +100 is 0; otherwise the same for 0x00F67500 from mem_stats +84
DECOMP(00542804, EnsureTextures_00542804) {
    const bool alt = load<u8>(0x010C5A20u + 34) != 0;
    const u32 slot = alt ? 0x00F67504u : 0x00F67500u;
    const u32 cur = load<u32>(slot);
    if (cur != 0) return ret(ctx, cur);
    const u32 src = load<u32>(0x010F0E20u + (alt ? 100 : 84));
    ctx.gpr[4] = src;
    if (src == 0) return ret(ctx, 0u);
    const u32 t = static_cast<u32>(call(ctx, 0x0051E514u, {src, 0}));
    store<u32>(slot, t);
    ctx.gpr[4] = 0x00F60000u;
}

// bool (ShaderType*, Material* m, Mesh* mesh): m->u16 +46 = 4095, +112 bits
// 1..16 (IBM) = 513, Material_ParamSlot(m, 0) = w * w with w = mesh->+88
// when mesh->u8 +84 is nonzero, else 30.0 (k at 0x005B027C); then +104
// gets bit 31 and bits 1..16 = 770, u16 +108 = 0x8006, u16 +110 = 771;
// returns 1  [vtable 00B251C8[1]]
DECOMP(005B0280, OutlineType_SetupMaterial) {
    const u32 m = arg<u32>(ctx, 1), mesh = arg<u32>(ctx, 2);
    const u32 r = m + 104;
    store<u16>(m + 46, 4095);
    store<u32>(r + 8, (load<u32>(r + 8) & 0x80007FFFu) | (513u << 15));  // rlwimi 15, 1, 16
    const f64 w = static_cast<s32>(load<u8>(mesh + 84)) > 0 ? lfs(mesh + 88) : lfs(0x005B027Cu);
    const f32 sq = static_cast<f32>(w * w);  // fmuls
    ctx.fpr[1] = w;  // still in f1 at the call
    const u32 slot = static_cast<u32>(call(ctx, 0x0050C018u, {m, 0, mesh}));
    store<f32>(slot, sq);
    const u32 flags = ((load<u32>(r) | 0x80000000u) & 0x80007FFFu) | (770u << 15);
    store<u16>(r + 4, 0x8006);
    store<u32>(r, flags);
    store<u16>(r + 6, 771);
    ret(ctx, 1u);
}
