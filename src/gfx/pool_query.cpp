// The render module's 16 occlusion query slots (vtable 0x00B240F8, base
// 0x00B23720) and the two small classes whose vtables follow theirs
// (0x00B24110, 0x00B24120).
#include "pool.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace q = sly4::render::pool::query;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;  // (p, pool)
constexpr u32 Mem_FreePool = 0x0056C730;  // (p, pool, tag)

// The Buffer class (vtable 0x00B24120): +8 a buffer from pool 4.
struct PoolBuffer {
    be_u32 vtable;  // +0
    pad<4> _4;
    be_u32 data;    // +8
};

// Takes the next report from the ring, marks it pending (1) and returns
// its report index.
u32 next_report() {
    be_u32& next = global<be_u32>(q::g::report_next);
    const u32 i = next;
    const u32 base = global<be_u32>(q::g::report_base);
    next = (i + 1) & q::kReportRing;
    global<be_u32>(base + (i << 4) + q::kReportMark) = 1u;
    return i + q::kReportIndex;
}

}  // namespace

// SlotBase::~SlotBase   (vtable 0x00B23720 slot 0; name?)
void SlotBase_Destruct(QuerySlot* s) { s->vtable = q::kBaseVtable; }

// SlotBase::~SlotBase as Slot_Delete calls it   (name?)
void SlotBase_DestructInline(QuerySlot* s) { s->vtable = q::kBaseVtable; }

// Slot::~Slot   (slot 0; only the base vtable is restored; name?)
void Slot_Destruct(QuerySlot* s) { s->vtable = q::kBaseVtable; }

// SlotBase deleting destructor   (base slot 1; name?)
void SlotBase_Delete(QuerySlot* s) {
    if (!s) return;
    s->vtable = q::kBaseVtable;
    call(Mem_FreeFrom, s, 0u);
}

// Slot deleting destructor   (slot 1; name?)
// Its own vtable, the base destructor, then the free.
void Slot_Delete(QuerySlot* s) {
    if (!s) return;
    s->vtable = q::kVtable;
    SlotBase_DestructInline(s);
    call(Mem_FreeFrom, s, 0u);
}

// void Slot_Issue(Slot* s)   (slot 2; name?)
// Marks the current entry issued and runs two query passes over the
// slot's proxy geometry (Slot_DrawProxy), each between a report clear
// and a report write: the first with the depth test the render state
// has (+93 bit 0; report index in entry +12), the second with the depth
// test off (+8).
void Slot_Issue(QuerySlot* s) {
    RenderState& st = rs();
    QueryEntry* e = at<QueryEntry>(s, q::kEntries + q::kEntrySize * s->current);
    e->state = 1;
    e->report_off = next_report();
    call(q::fn::cellGcmSetClearReport, gcm(), 1u);
    emit({kDepthTestEnable, (st.dirty_b[1] & 1) != 0 ? 1u : 0u});
    Slot_DrawProxy(s);
    call(q::fn::cellGcmSetReport, gcm(), 1u, e->report_off.get());
    e->report_on = next_report();
    call(q::fn::cellGcmSetClearReport, gcm(), 1u);
    emit({kDepthTestEnable, 0});
    Slot_DrawProxy(s);
    call(q::fn::cellGcmSetReport, gcm(), 1u, e->report_on.get());
}

// bool Slot_GetResult(Slot* s, u32 out[2], u32 back)   (slot 3; name?)
// Copies the result pair of the entry `back` before the current one
// ((current - back + 4) % 4, C remainder; a negative one is not wrapped)
// and returns whether that entry is done (state 7).
bool Slot_GetResult(QuerySlot* s, be_u32* out, u32 back) {
    const s32 x = static_cast<s32>(s->current - back + 4);
    const u32 idx = static_cast<u32>(x % 4) & 0xFFFF;  // srawi / addze remainder
    const QueryEntry* e = at<QueryEntry>(s, q::kEntries + q::kEntrySize * idx);
    const u32 state = e->state;
    const u32 a = e->result[0], b = e->result[1];
    out[0] = a;
    out[1] = b;
    return state == 7;
}

// BufferBase::~BufferBase   (vtable 0x00B24110 slot 0; name?)
void BufferBase_Destruct(PoolBuffer* s) { s->vtable = q::kBufferBaseVtable; }

// BufferBase deleting destructor   (slot 1; name?)
void BufferBase_Delete(PoolBuffer* s) {
    if (!s) return;
    s->vtable = q::kBufferBaseVtable;
    call(Mem_FreeFrom, s, 0u);
}

// Buffer::~Buffer   (vtable 0x00B24120 slot 0; name?)
// Frees +8 (Mem_FreePool pool 4, tag 1; no null check), then the base
// vtable.
void Buffer_Destruct(PoolBuffer* s) {
    const u32 buf = s->data;
    s->vtable = q::kBufferVtable;
    call(Mem_FreePool, buf, 4u, 1u);
    s->vtable = q::kBufferBaseVtable;
}

// Buffer deleting destructor   (slot 1; name?)
void Buffer_Delete(PoolBuffer* s) {
    if (!s) return;
    const u32 buf = s->data;
    s->vtable = q::kBufferVtable;
    call(Mem_FreePool, buf, 4u, 1u);
    s->vtable = q::kBufferBaseVtable;
    call(Mem_FreeFrom, s, 0u);
}

}  // namespace sly4::gfx

NATIVE(004FCC10, sly4::gfx::SlotBase_Destruct)
NATIVE(004FCC20, sly4::gfx::SlotBase_DestructInline)
NATIVE(0079C404, sly4::gfx::Slot_Destruct)
NATIVE(005276C4, sly4::gfx::SlotBase_Delete)
NATIVE(007A395C, sly4::gfx::Slot_Delete)
NATIVE(00548694, sly4::gfx::Slot_Issue)
NATIVE(00548848, sly4::gfx::Slot_GetResult)
NATIVE(007A39B4, sly4::gfx::BufferBase_Destruct)
NATIVE(007A39C4, sly4::gfx::BufferBase_Delete)
NATIVE(007A3A08, sly4::gfx::Buffer_Destruct)
NATIVE(007A3A60, sly4::gfx::Buffer_Delete)
