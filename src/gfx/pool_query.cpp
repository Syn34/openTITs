// The render module's 16 occlusion query slots (vtable 0x00B240F8, base
// 0x00B23720) and the two small classes whose vtables follow theirs
// (0x00B24110, 0x00B24120).
#include "pool.h"

using namespace sly4;

namespace {

namespace l3 = sly4::render::layers3;
namespace l4 = sly4::render::layers4;
namespace q = sly4::render::pool::query;

// Takes the next report from the ring, marks it pending (1) and returns
// its report index.
u32 next_report() {
    const u32 i = load<u32>(q::g::report_next);
    const u32 base = load<u32>(q::g::report_base);
    store<u32>(q::g::report_next, (i + 1) & q::kReportRing);
    store<u32>(base + (i << 4) + q::kReportMark, 1);
    return i + q::kReportIndex;
}

// Sets the vtable; r4 is left holding it.
void set_vtable(PPUContext& ctx, u32 obj, u32 vt) {
    store<u32>(obj, vt);
    ctx.gpr[4] = vt;
}

}  // namespace

// SlotBase::~SlotBase   (vtable 0x00B23720 slot 0; name?)
DECOMP(004FCC10, SlotBase_Destruct) { set_vtable(ctx, arg<u32>(ctx, 0), q::kBaseVtable); }

// SlotBase::~SlotBase as Slot_Delete calls it   (name?)
DECOMP(004FCC20, SlotBase_DestructInline) { set_vtable(ctx, arg<u32>(ctx, 0), q::kBaseVtable); }

// Slot::~Slot   (slot 0; only the base vtable is restored; name?)
DECOMP(0079C404, Slot_Destruct) { set_vtable(ctx, arg<u32>(ctx, 0), q::kBaseVtable); }

// SlotBase deleting destructor   (base slot 1; name?)
DECOMP(005276C4, SlotBase_Delete) {
    const u32 s = arg<u32>(ctx, 0);
    ctx.gpr[4] = s;
    if (s == 0) return;
    store<u32>(s, q::kBaseVtable);
    ctx.gpr[3] = call(ctx, l3::fn::Mem_FreeFrom, {s, 0});
}

// Slot deleting destructor   (slot 1; name?)
// Its own vtable, the base destructor, then the free.
DECOMP(007A395C, Slot_Delete) {
    const u32 s = arg<u32>(ctx, 0);
    if (s == 0) return;
    store<u32>(s, q::kVtable);
    call(ctx, q::fn::SlotBase_DestructInline, {s});
    ctx.gpr[3] = call(ctx, l3::fn::Mem_FreeFrom, {s, 0});
}

// void Slot_Issue(Slot* s)   (slot 2; name?)
// Marks the current entry issued and runs two query passes over the
// slot's proxy geometry (Slot_DrawProxy), each between a report clear
// and a report write: the first with the depth test the render state
// has (+93 bit 0; report index in entry +12), the second with the depth
// test off (+8).
DECOMP(00548694, Slot_Issue) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 st = fx::g::state;
    const u32 e = s + q::kEntries + q::kEntrySize * load<u16>(s + q::kCurrent);
    store<u8>(e + 16, 1);
    store<u32>(e + 12, next_report());
    call(ctx, q::fn::cellGcmSetClearReport, {load<u32>(st), 1});
    fx::emit({l4::kDepthTest, (ref<u8>(st + 93) & 1) != 0 ? 1u : 0u});
    call(ctx, q::fn::Slot_DrawProxy, {s});
    call(ctx, q::fn::cellGcmSetReport, {load<u32>(st), 1, load<u32>(e + 12)});
    store<u32>(e + 8, next_report());
    call(ctx, q::fn::cellGcmSetClearReport, {load<u32>(st), 1});
    fx::emit({l4::kDepthTest, 0});
    call(ctx, q::fn::Slot_DrawProxy, {s});
    ctx.gpr[3] = call(ctx, q::fn::cellGcmSetReport, {load<u32>(st), 1, load<u32>(e + 8)});
}

// bool Slot_GetResult(Slot* s, u32 out[2], u32 back)   (slot 3; name?)
// Copies the result pair of the entry `back` before the current one
// ((current - back + 4) % 4, C remainder; a negative one is not wrapped)
// and returns whether that entry is done (state 7).
DECOMP(00548848, Slot_GetResult) {
    const u32 s = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const s32 x = static_cast<s32>(load<u16>(s + q::kCurrent) - arg<u32>(ctx, 2) + 4);
    const u32 idx = static_cast<u32>(x % 4) & 0xFFFF;  // srawi / addze remainder
    const u32 e = s + q::kEntries + q::kEntrySize * idx;
    const u32 state = ref<u8>(e + 16);
    const u32 a = load<u32>(e), b = load<u32>(e + 4);
    store<u32>(out, a);
    store<u32>(out + 4, b);
    ctx.gpr[3] = state == 7 ? 1u : 0u;
    ctx.gpr[5] = state == 7 ? 32u : static_cast<u32>(__builtin_clz(state - 7));  // cntlzw left in r5
    ctx.gpr[6] = a;
    ctx.gpr[7] = b;
}

// BufferBase::~BufferBase   (vtable 0x00B24110 slot 0; name?)
DECOMP(007A39B4, BufferBase_Destruct) { set_vtable(ctx, arg<u32>(ctx, 0), q::kBufferBaseVtable); }

// BufferBase deleting destructor   (slot 1; name?)
DECOMP(007A39C4, BufferBase_Delete) {
    const u32 s = arg<u32>(ctx, 0);
    ctx.gpr[4] = s;
    if (s == 0) return;
    store<u32>(s, q::kBufferBaseVtable);
    ctx.gpr[3] = call(ctx, l3::fn::Mem_FreeFrom, {s, 0});
}

// Buffer::~Buffer   (vtable 0x00B24120 slot 0; name?)
// Frees +8 (Mem_FreePool pool 4, tag 1; no null check), then the base
// vtable. Returns it in r3.
DECOMP(007A3A08, Buffer_Destruct) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 buf = load<u32>(s + 8);
    store<u32>(s, q::kBufferVtable);
    call(ctx, l4::fn::Mem_FreePool, {buf, 4, 1});
    store<u32>(s, q::kBufferBaseVtable);
    ctx.gpr[3] = q::kBufferBaseVtable;
}

// Buffer deleting destructor   (slot 1; name?)
DECOMP(007A3A60, Buffer_Delete) {
    const u32 s = arg<u32>(ctx, 0);
    if (s == 0) return;
    const u32 buf = load<u32>(s + 8);
    store<u32>(s, q::kBufferVtable);
    call(ctx, l4::fn::Mem_FreePool, {buf, 4, 1});
    store<u32>(s, q::kBufferBaseVtable);
    ctx.gpr[3] = call(ctx, l3::fn::Mem_FreeFrom, {s, 0});
}
