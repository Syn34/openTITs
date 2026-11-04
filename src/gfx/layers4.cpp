// The render module statics' constructors left by RenderLayers_Construct
// (the 16 slots, the pool), the pool's small vtable slots (vtable
// 0x00B241E8: 2 destructor, 3 deleting destructor, 4 / 11 empty, 10
// refresh) and its part destructor.
#include "layers4.h"

using namespace sly4;

namespace {

namespace l3 = sly4::render::layers3;
namespace l4 = sly4::render::layers4;

constexpr u32 kPoolPart = 12;  // {+0 buffer, +4 u32 bit 0: pool 5, +8 u32 bit 0: not owned, +12 u32 tag in bits 28..31}

// The part destructor (Pool_DestructPart) as the deleting destructor
// inlines it: frees the buffer unless it isn't owned. Returns the r3 the
// original leaves (0, or the buffer when it was kept).
u64 destruct_part(PPUContext& ctx, u32 part) {
    const u32 p = load<u32>(part);
    if (p == 0 || (load<u32>(part + 8) & 1) != 0) return p;
    const u32 pool = (load<u32>(part + 4) & 1) != 0 ? 5 : 4;
    call(ctx, l4::fn::Mem_FreePool, {p, pool, (load<u32>(part + 12) >> 28) & 0xF});
    store<u32>(part, 0);
    return 0;
}

// The pool destructor body before its part: base vtable, both buffers
// freed (no null check).
void free_buffers(PPUContext& ctx, u32 pool) {
    const u32 buf = load<u32>(pool + 4);
    store<u32>(pool, l4::kPoolBaseVtable);
    call(ctx, l3::fn::Mem_FreeFrom, {buf, 0});
    call(ctx, l3::fn::Mem_FreeFrom, {load<u32>(pool + 304), 0});
}

}  // namespace

// void SlotBase_Construct(Slot* s)   (name?)
// Base vtable 0x00B23720 and the byte at +4 cleared.
DECOMP(005276AC, SlotBase_Construct) {
    const u32 s = arg<u32>(ctx, 0);
    store<u8>(s + 4, 0);
    store<u32>(s, l4::kSlotBaseVtable);
    ctx.gpr[4] = l4::kSlotBaseVtable;  // left in r4 / r5
    ctx.gpr[5] = 0;
}

// void Slot_Construct(Slot* s)   (one of the 16 176-byte slots; name?)
DECOMP(005485B4, Slot_Construct) {
    const u32 s = arg<u32>(ctx, 0);
    call(ctx, l4::fn::SlotBase_Construct, {s});
    store<u32>(s, l4::kSlotVtable);
    ctx.gpr[3] = l4::kSlotVtable;  // left in r3
}

// void Pool_Construct(Pool* p)   (the base part of layers3::g::pool; name?)
// Base vtable, +4 / +8 / +12 / +304 cleared, +16 = 1 and +20 = 2 (the
// bitfields set as constants), +24 bits 28..31 cleared, +308.. RGBA
// {204, 204, 204, 255}, +312 f32 1.0, +316 f32 5.0, +320 u8 1.
DECOMP(00526D18, Pool_Construct) {
    const u32 p = arg<u32>(ctx, 0);
    store<u32>(p, l4::kPoolBaseVtable);
    store<u32>(p + 4, 0);
    store<u32>(p + 8, 0);
    const u32 w24 = load<u32>(p + 24) & 0x0FFFFFFFu;  // rlwinm 0,4,31
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 1);  // (+16 & 1) | 1
    store<u32>(p + 20, 2);  // (+20 & 0xE) with bits 28..30 then set to 1 << 1
    store<u32>(p + 24, w24);
    store<u32>(p + 304, 0);
    store<u8>(p + 308, 204);
    store<u8>(p + 309, 204);
    store<u8>(p + 310, 204);
    store<u8>(p + 311, 255);
    store<u32>(p + 312, 0x3F800000);  // 1.0f
    store<u32>(p + 316, 0x40A00000);  // 5.0f
    store<u8>(p + 320, 1);
    // Volatile registers as the original leaves them (no known reader).
    ctx.gpr[4] = 255;
    ctx.gpr[5] = 1;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = 0x40A00000;
    ctx.gpr[8] = w24;
    ctx.gpr[9] = 204;
    ctx.gpr[10] = 0x3F800000;
}

// void Pool_DestructPart(Part* part)   (name?)
// Frees part->+0 (Mem_FreePool(p, +4 bit 0 ? 5 : 4, +12 >> 28)) unless it
// is null or +8 bit 0 marks it as not owned; clears +0 after a free.
DECOMP(0052EFE0, Pool_DestructPart) {
    ctx.gpr[3] = destruct_part(ctx, arg<u32>(ctx, 0));
}

// Pool::~Pool   (vtable 0x00B241E8 slot 2; name?)
// Base vtable, the buffers at +4 and +304 freed (no null check), the part
// at +12 destroyed. Returns what Pool_DestructPart returns.
DECOMP(0079C414, Pool_Destruct) {
    const u32 p = arg<u32>(ctx, 0);
    free_buffers(ctx, p);
    ctx.gpr[3] = call(ctx, l4::fn::Pool_DestructPart, {p + kPoolPart});
}

// Pool deleting destructor   (slot 3; name?)
// The destructor (its part destructor inlined), then the object freed.
DECOMP(007A3BE8, Pool_Delete) {
    const u32 p = arg<u32>(ctx, 0);
    if (p == 0) return;  // r3 stays 0
    free_buffers(ctx, p);
    destruct_part(ctx, p + kPoolPart);
    ctx.gpr[3] = call(ctx, l3::fn::Mem_FreeFrom, {p, 0});
}

// Empty slots 4 and 11.
DECOMP(007A2214, Pool_Nop4) {}
DECOMP(0050F9A0, Pool_Nop11) {}

// void Pool_Refresh(Pool* p)   (slot 10; name?)
// Virtual slot 9 then slot 11 (empty here) on p.
DECOMP(00527150, Pool_Refresh) {
    const u32 p = arg<u32>(ctx, 0);
    fx::vcall(ctx, p, 36, {p});
    ctx.gpr[3] = fx::vcall(ctx, p, 44, {p});
}

// void Draw_ApplyView()   (name?)
// Virtual slot 20 of the current View (current_surface +672) on itself.
DECOMP(00527108, Draw_ApplyView) {
    const u32 view = load<u32>(load<u32>(draw::g::current_surface) + 672);
    ctx.gpr[3] = fx::vcall(ctx, view, 20, {view});
}

// void Pool_ResetState(Pool* p)   (pool vtable slot 9; name?)
// Draw_ApplyView, then the default depth / stencil state: stencil test
// off, func (cached +160, +164) with mask 255 (+172), write mask 255
// (+168); when g::config +152 bit 5 (two-sided stencil) also the back
// func with mask -1 (+200; +188 / +192 cached) and back write mask 255
// (+196); depth test as the state has it, LEQUAL, cull on, CCW front
// face, all colour writes on.
DECOMP(0054BD38, Pool_ResetState) {
    namespace l4 = sly4::render::layers4;
    using fx::emit;
    call(ctx, l4::fn::Draw_ApplyView);
    const u32 st = fx::g::state;
    constexpr u32 kStencilFunc = 0x000C0330, kStencilMask = 0x0004032C;
    constexpr u32 kBackStencilFunc = 0x000C0350, kBackStencilMask = 0x0004034C;
    emit({fx::kStencilTestEnable, 0});
    store<u32>(st + 172, 255);
    emit({kStencilFunc, load<u32>(st + 160), load<u32>(st + 164), 255});
    store<u32>(st + 168, 255);
    emit({kStencilMask, 255});
    if (((ref<u8>(sly4::g::config + 152) >> 5) & 1) != 0) {  // rlwinm 27,31,31
        store<u32>(st + 200, 0xFFFFFFFFu);
        emit({kBackStencilFunc, load<u32>(st + 188), load<u32>(st + 192), 0xFFFFFFFFu});
        store<u32>(st + 196, 255);
        emit({kBackStencilMask, 255});
    }
    emit({l4::kDepthTest, ref<u8>(st + 93)});
    emit({l4::kDepthFunc, l4::kLequal});
    emit({l4::kCullEnable, 1});
    emit({l4::kFrontFace, l4::kCcw});
    emit({l4::kColorMask, 0x01010101});
    ctx.gpr[3] = l4::kColorMask;  // left in r3
}
