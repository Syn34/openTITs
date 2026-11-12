// The render module statics' constructors left by RenderLayers_Construct
// (the 16 slots, the pool), the pool's small vtable slots (vtable
// 0x00B241E8: 2 destructor, 3 deleting destructor, 4 / 11 empty, 9 reset
// state, 10 refresh) and its part destructor.
#include "layers4.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace l4 = sly4::render::layers4;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;  // (p, pool)
constexpr u32 Mem_FreePool = 0x0056C730;  // (p, pool, tag)
constexpr u32 kCurrentSurface = 0x010DDA74;  // object whose +672 is the current view

// The part destructor (Pool_DestructPart), also inlined by the deleting
// destructor: frees the buffer unless it isn't owned.
void destruct_part(VertexBlock* part) {
    u8* p = part->data;
    if (!p || (part->flags & 1) != 0) return;
    const u32 pool = (part->where & 1) != 0 ? 5 : 4;
    call(Mem_FreePool, p, pool, (part->tag >> 28) & 0xF);
    part->data = nullptr;
}

// The pool destructor body before its part: base vtable, both buffers
// freed (no null check).
void free_buffers(StencilShadow* pool) {
    const u32 buf = pool->cursor.ea;
    pool->vtable = l4::kPoolBaseVtable;
    call(Mem_FreeFrom, buf, 0u);
    call(Mem_FreeFrom, pool->face_sign.ea.get(), 0u);
}

}  // namespace

// void SlotBase_Construct(QuerySlot* s)   (name?)
// Base vtable 0x00B23720 and the byte at +4 cleared.
void SlotBase_Construct(QuerySlot* s) {
    s->b4 = 0;
    s->vtable = l4::kSlotBaseVtable;
}

// void Slot_Construct(QuerySlot* s)   (one of the 16 176-byte slots; name?)
void Slot_Construct(QuerySlot* s) {
    SlotBase_Construct(s);
    s->vtable = l4::kSlotVtable;
}

// void Pool_Construct(StencilShadow* p)   (the base part of layers3::g::pool; name?)
// Base vtable, +4 / +8 / +12 / +304 cleared, +16 = 1 and +20 = 2 (the
// bitfields set as constants), +24 bits 28..31 cleared, +308.. RGBA
// {204, 204, 204, 255}, +312 f32 1.0, +316 f32 5.0, +320 u8 1.
void Pool_Construct(StencilShadow* p) {
    p->vtable = l4::kPoolBaseVtable;
    p->cursor = nullptr;
    p->w8 = 0u;
    const u32 w24 = p->quad.tag & 0x0FFFFFFFu;  // rlwinm 0,4,31
    p->quad.data = nullptr;
    p->quad.where = 1u;  // (+16 & 1) | 1
    p->quad.flags = 2u;  // (+20 & 0xE) with bits 28..30 then set to 1 << 1
    p->quad.tag = w24;
    p->face_sign = nullptr;
    p->colour[0] = 204;
    p->colour[1] = 204;
    p->colour[2] = 204;
    p->colour[3] = 255;
    p->intensity = 1.0f;
    p->extrude = 5.0f;
    p->caps = 1;
}

// void Pool_DestructPart(VertexBlock* part)   (name?)
// Frees part->+0 (Mem_FreePool(p, +4 bit 0 ? 5 : 4, +12 >> 28)) unless it
// is null or +8 bit 0 marks it as not owned; clears +0 after a free.
void Pool_DestructPart(VertexBlock* part) { destruct_part(part); }

// Pool::~Pool   (vtable 0x00B241E8 slot 2; name?)
// Base vtable, the buffers at +4 and +304 freed (no null check), the part
// at +12 destroyed.
void Pool_Destruct(StencilShadow* p) {
    free_buffers(p);
    Pool_DestructPart(&p->quad);
}

// Pool deleting destructor   (slot 3; name?)
// The destructor (its part destructor inlined), then the object freed.
void Pool_Delete(StencilShadow* p) {
    if (!p) return;
    free_buffers(p);
    destruct_part(&p->quad);
    call(Mem_FreeFrom, p, 0u);
}

// Empty slots 4 and 11.
void Pool_Nop4() {}
void Pool_Nop11() {}

// void Pool_Refresh(StencilShadow* p)   (slot 10; name?)
// Virtual slot 9 then slot 11 (empty here) on p.
void Pool_Refresh(StencilShadow* p) {
    vcall(p, 36);
    vcall(p, 44);
}

// void Draw_ApplyView()   (name?)
// Virtual slot 20 of the current View (current_surface +672) on itself.
void Draw_ApplyView() {
    u8* view = *at<gp<u8>>(global<gp<u8>>(kCurrentSurface).get(), 672);
    vcall(or_zero_page(view), 20);
}

// void Pool_ResetState(StencilShadow* p)   (pool vtable slot 9; name?)
// Draw_ApplyView, then the default depth / stencil state: stencil test
// off, func (cached +160, +164) with mask 255 (+172), write mask 255
// (+168); when g::config +152 bit 5 (two-sided stencil) also the back
// func with mask -1 (+200; +188 / +192 cached) and back write mask 255
// (+196); depth test as the state has it, LEQUAL, cull on, CCW front
// face, all colour writes on.
void Pool_ResetState(StencilShadow*) {
    Draw_ApplyView();
    RenderState& st = rs();
    emit({kStencilTestEnable, 0});
    st.stencil_func_mask = 255u;
    emit({kStencilFunc, st.stencil_func, st.stencil_ref, 255});
    st.stencil_mask = 255u;
    emit({kStencilMask, 255});
    if (two_sided_stencil()) {
        st.back_func_mask = 0xFFFFFFFFu;
        emit({kBackStencilFunc, st.back_func, st.back_ref, 0xFFFFFFFFu});
        st.back_mask = 255u;
        emit({kBackStencilMask, 255});
    }
    emit({kDepthTestEnable, st.dirty_b[1]});
    emit({kDepthFunc, kGlLequal});
    emit({kCullFaceEnable, 1});
    emit({kFrontFace, kGlCcw});
    emit({kColorMask, 0x01010101});
}

}  // namespace sly4::gfx

NATIVE(005276AC, sly4::gfx::SlotBase_Construct)
NATIVE(005485B4, sly4::gfx::Slot_Construct)
NATIVE(00526D18, sly4::gfx::Pool_Construct)
NATIVE(0052EFE0, sly4::gfx::Pool_DestructPart)
NATIVE(0079C414, sly4::gfx::Pool_Destruct)
NATIVE(007A3BE8, sly4::gfx::Pool_Delete)
NATIVE(007A2214, sly4::gfx::Pool_Nop4)
NATIVE(0050F9A0, sly4::gfx::Pool_Nop11)
NATIVE(00527150, sly4::gfx::Pool_Refresh)
NATIVE(00527108, sly4::gfx::Draw_ApplyView)
NATIVE(0054BD38, sly4::gfx::Pool_ResetState)
