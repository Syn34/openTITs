// StateMachine: the base class of the World (constructor, shutdown, tick,
// set state).
#include "vt_world.h"

using namespace sly4;
using namespace sly4::world;

namespace {

constexpr u32 kSmallAlloc = 0x0055238C;
constexpr u32 kMemset = 0x00726E54;
constexpr u32 kSmallFree = 0x00552420;   // (ptr, size)
constexpr u32 kMemFreeFrom = 0x0054EE6C;

// Virtual call. The original's call sequence leaves the OPD and its entry
// in the two registers after the arguments; we do the same.
u64 vcall(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    ctx.gpr[3 + args.size()] = opd;
    ctx.gpr[4 + args.size()] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}

}  // namespace

// void StateMachine_Construct(StateMachine* sm, u16 levels, u32 data_size, u8 initial)
// One block holds ids (16-aligned), a[] and b[] (4 bytes per level, each
// 16-aligned) and the per-level data; it is zeroed and ids[0] = initial.
// As in the original, the offsets of a[], b[] and data inside the block
// keep only bits 4..7.
DECOMP(0045CAA0, StateMachine_Construct) {
    const u32 sm = arg<u32>(ctx, 0);
    const u32 levels = arg<u32>(ctx, 1);
    const u32 size_arg = arg<u32>(ctx, 2);
    const u8 initial = static_cast<u8>(arg<u32>(ctx, 3));
    store<u16>(sm + kSmLevels, static_cast<u16>(levels));
    store<u32>(sm, kSmVtable);
    store<u32>(sm + kSmIds, 0);
    store<u32>(sm + kSmA, 0);
    store<u16>(sm + kSmDataSize, static_cast<u16>((size_arg + 15) & ~15u));
    store<u32>(sm + kSmB, 0);
    store<u32>(sm + kSmData, 0);
    store<u32>(sm + kSmBytes, 0);
    if (static_cast<s32>(levels) == 0) return;

    const u32 n = levels;  // the full register, not the u16 stored at +4
    const u32 per_level = (size_arg + 15) & 0xFFF0;
    const u32 words = n * 4;
    const u32 ids_end = (n + 15) & ~15u;
    const u32 a_off = (n + 15) & 0xF0;
    const u32 b_full = (ids_end + words + 15) & ~15u;
    const u32 b_off = (ids_end + words + 15) & 0xF0;
    const u32 data_full = (b_full + words + 15) & ~15u;
    const u32 data_off = (b_full + words + 15) & 0xF0;
    const u32 bytes = data_full + n * per_level;
    store<u32>(sm + kSmBytes, bytes);
    const u32 mem = static_cast<u32>(call(ctx, kSmallAlloc, {bytes}));
    store<u32>(sm + kSmIds, mem);
    store<u32>(sm + kSmA, mem + a_off);
    store<u32>(sm + kSmB, mem + b_off);
    store<u32>(sm + kSmData, size_arg != 0 ? mem + data_off : 0);
    call(ctx, kMemset, {mem, 0, load<u32>(sm + kSmBytes)});
    ref<u8>(load<u32>(sm + kSmIds)) = initial;
}

// bool StateMachine_Shutdown(StateMachine* sm): SetState(0, none).
DECOMP(00464B90, StateMachine_Shutdown) {
    const u32 sm = arg<u32>(ctx, 0);
    vcall(ctx, sm, kVfnSetState, {sm, 0, kSmNone});
}

// bool StateMachine_Tick(StateMachine* sm, f32 dt)
// Sends a tick event; true if no state handled it.
DECOMP(00464CBC, StateMachine_Tick) {
    const u32 sm = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 ev = frame.addr();
    store<u32>(ev, kEventTick);
    store<f32>(ev + 4, static_cast<f32>(farg(ctx, 0)));
    ref<u8>(ev + 8) = 0;
    vcall(ctx, sm, kVfnEvent, {sm, ev});
    ret(ctx, ref<u8>(ev + 8) == 0 ? 1u : 0u);
}

// bool StateMachine_SetState(StateMachine* sm, u32 level, u8 id)
// Exits the current state at `level` (if any), clears that level and the
// ones below it (a, b, data; ids below `level` become 0), then enters `id`
// unless it is none. Returns 1.
DECOMP(00464D1C, StateMachine_SetState) {
    const u32 sm = arg<u32>(ctx, 0);
    const u32 level = arg<u32>(ctx, 1);
    const u32 id = arg<u32>(ctx, 2) & 0xFF;
    Stack frame(ctx, 32);
    const u32 exit_ev = frame.addr(), enter_ev = exit_ev + 16;

    const u32 cur = ref<u8>(load<u32>(sm + kSmIds) + level);
    if (cur != kSmNone) {
        store<u32>(exit_ev, kEventExit);
        store<u32>(exit_ev + 4, level);
        store<u32>(exit_ev + 8, cur);
        store<u32>(exit_ev + 12, id);
        vcall(ctx, sm, kVfnEvent, {sm, exit_ev});
    }
    for (u32 i = level; i < load<u16>(sm + kSmLevels); ++i) {
        store<u32>(load<u32>(sm + kSmA) + i * 4, 0);
        store<u32>(load<u32>(sm + kSmB) + i * 4, 0);
        const u32 size = load<u16>(sm + kSmDataSize);
        if (size != 0) call(ctx, kMemset, {load<u32>(sm + kSmData) + i * size, 0, size});
        if (i > level) ref<u8>(load<u32>(sm + kSmIds) + i) = 0;
    }
    ref<u8>(load<u32>(sm + kSmIds) + level) = static_cast<u8>(id);
    if (id != kSmNone) {
        store<u32>(enter_ev, kEventEnter);
        store<u32>(enter_ev + 4, level);
        store<u32>(enter_ev + 8, ref<u8>(load<u32>(sm + kSmIds) + level));
        store<u32>(enter_ev + 12, id);
        vcall(ctx, sm, kVfnEvent, {sm, enter_ev});
    }
    ret(ctx, 1u);
}

// ---------------------------------------------------------------- base virtuals

// void StateMachine_Construct thunk: the same call with the same registers.
DECOMP(0045CBE8, StateMachine_ConstructThunk) { call(ctx, vfn::StateMachine_Construct); }

// void StateMachine_Destruct(StateMachine* sm): base vtable, free the block.
DECOMP(00464A24, StateMachine_Destruct) {
    const u32 sm = arg<u32>(ctx, 0);
    const u32 ids = load<u32>(sm + kSmIds);
    store<u32>(sm, kSmVtable);
    if (ids != 0) call(ctx, kSmallFree, {ids, load<u32>(sm + kSmBytes)});
}

// void StateMachine_Delete(StateMachine* sm, u32 flags): destruct and free.
DECOMP(00464A6C, StateMachine_Delete) {
    const u32 sm = arg<u32>(ctx, 0);
    if (sm == 0) return;
    store<u32>(sm, kSmVtable);
    if (load<u32>(sm + kSmIds) != 0) call(ctx, kSmallFree, {load<u32>(sm + kSmIds), load<u32>(sm + kSmBytes)});
    call(ctx, kMemFreeFrom, {sm, 0});
}

// void StateMachine_Init(StateMachine* sm): enter the level-0 state set at construction.
DECOMP(00464B18, StateMachine_Init) {
    const u32 sm = arg<u32>(ctx, 0);
    const u32 ids = load<u32>(sm + kSmIds);
    if (ids == 0 || ref<u8>(ids) == kSmNone) return;
    Stack frame(ctx, 16);
    const u32 ev = frame.addr();
    store<u32>(ev, kEventEnter);
    store<u32>(ev + 4, 0);
    store<u32>(ev + 8, kSmNone);
    store<u32>(ev + 12, ref<u8>(ids));
    vcall(ctx, sm, kVfnEvent, {sm, ev});
}

// bool StateMachine_Update(StateMachine* sm, f32 dt)
// Sends update events until one is handled or the handler returns nonzero;
// if none was handled, adds dt to every level's time (a[], f32) and counts
// a frame (b[]). Returns whether nothing handled it.
DECOMP(00464BD4, StateMachine_Update) {
    const u32 sm = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    Stack frame(ctx, 16);
    const u32 ev = frame.addr();
    store<u32>(ev, kEventUpdate);
    store<f32>(ev + 4, static_cast<f32>(dt));
    ref<u8>(ev + 8) = 0;
    u32 handled = 0;
    while (handled == 0) {
        const u32 r = static_cast<u32>(vcall(ctx, sm, kVfnEvent, {sm, ev}));
        handled = ref<u8>(ev + 8);
        if (r != 0) break;
    }
    if (handled != 0) return ret(ctx, 0u);
    if (load<u16>(sm + kSmLevels) != 0) {
        u32 i = 0;
        do {
            const u32 t = load<u32>(sm + kSmA) + i * 4;
            store<f32>(t, static_cast<f32>(static_cast<f64>(load<f32>(t)) + dt));  // fadds
            const u32 n = load<u32>(sm + kSmB) + i * 4;
            store<u32>(n, load<u32>(n) + 1);
            ++i;
        } while (i < load<u16>(sm + kSmLevels));
    }
    ret(ctx, 1u);
}

// bool StateMachine_Advance(StateMachine* sm, u32 level): SetState(level, GetState(level) + 1).
DECOMP(007371E8, StateMachine_Advance) {
    const u32 sm = arg<u32>(ctx, 0), level = arg<u32>(ctx, 1);
    const u32 id = static_cast<u32>(vcall(ctx, sm, kVfnGetState, {sm, level}));
    vcall(ctx, sm, kVfnSetState, {sm, level, (id + 1) & 0xFF});
}

// u8 StateMachine_GetState(StateMachine* sm, u32 level): ids[level].
DECOMP(0073726C, StateMachine_GetState) {
    ret(ctx, static_cast<u32>(ref<u8>(load<u32>(arg<u32>(ctx, 0) + kSmIds) + arg<u32>(ctx, 1))));
}

// bool StateMachine_HandleEvent(StateMachine*, Event*): returns 1.
DECOMP(007435B8, StateMachine_HandleEvent) { ret(ctx, 1u); }
