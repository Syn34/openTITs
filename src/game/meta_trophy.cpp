// The trophy manager World_Init builds (and World_Destruct frees).
#include "meta.h"

using namespace sly4;
using namespace sly4::meta;

// void Trophy_Construct(Trophy* t, const SceNpCommunicationId* comm_id,
//                       const SceNpCommunicationSignature* sig, Opd* callback,
//                       u16 capacity, u8 flag)
// Clears the manager, sets up its sorted list (capacity entries of 8 bytes),
// publishes it at kTrophy, then initialises sceNpTrophy, creates the context
// and handle, two events and the "TrophyThread". Stops at the first sceNpTrophy
// call that fails (negative result).
DECOMP(004FB0F0, Trophy_Construct) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 comm_id = arg<u32>(ctx, 1), sig = arg<u32>(ctx, 2), callback = arg<u32>(ctx, 3);
    const u64 capacity = ctx.gpr[7];
    const u8 flag = static_cast<u8>(ctx.gpr[8]);
    store<u32>(t + kTrCommId, comm_id);
    store<u32>(t + kTrSignature, sig);
    ref<u8>(t + 8) = 0;
    ref<u8>(t + 9) = 0;
    ref<u8>(t + 10) = 0;
    ref<u8>(t + kTrFlag11) = flag;
    ref<u8>(t + 12) = 0;
    ref<u8>(t + 13) = 0;
    store<u64>(t + 16, 0);
    call(ctx, mfn::Lock_Init, {t + kTrLock, 32});

    ref<u8>(t + 92) = 0;
    store<u32>(t + 164, 0);
    store<u32>(t + kTrThread, 0);
    ref<u8>(t + 93) = 0;
    ref<u8>(t + 94) = 0;
    store<u32>(t + 96, 0);
    store<u32>(t + 100, 0);
    store<u32>(t + kTrState, 2);
    store<u32>(t + kTrCallback, 0);
    for (u32 off : {12u, 8u, 4u, 0u}) store<u32>(t + 140 + off, 0);
    for (u32 off = 0; off < 32; off += 8) store<u64>(t + 168 + off, 0);

    const u32 list = t + kTrList;
    store<u16>(list + kArrCapacity, static_cast<u16>(capacity));
    store<u16>(list + kArrCount, 0);
    store<u16>(list + kArrHigh, 0);
    store<u16>(list + kArrFlags, 16);  // 8-byte elements
    store<u32>(list + kArrData, 0);
    const u32 allocator = static_cast<u32>(call(ctx, mfn::Mem_DefaultAllocator));
    const u32 cap = load<u16>(list + kArrCapacity);
    store<u32>(list + kArrAllocator, allocator);
    if (cap != 0) {
        const u32 esize = (static_cast<u32>(load<u16>(list + kArrFlags)) >> 1) & 0x7FFF;
        const u32 opd = load<u32>(load<u32>(allocator) + kAllocVfnAlloc);
        // This call site has the entry in r7 and the OPD in r8.
        ctx.gpr[7] = load<u32>(opd);
        ctx.gpr[8] = opd;
        store<u32>(list + kArrData, static_cast<u32>(call_ptr(ctx, opd, {allocator, cap * esize, 0, 16})));
    }
    call(ctx, mfn::Array_Init, {list});
    store<u32>(t + kTrList + kArrCompare, kCompareU32);
    store<u32>(kTrophy, t);
    store<u32>(t + kTrList + kArr16, 0xFFFFFFFFu);
    store<u32>(t + kTrCallback, callback);

    if (static_cast<s32>(call(ctx, np::sceNpTrophyInit, {0, 0, np::kMemContainerInvalid, 0})) < 0) return;
    store<u32>(t + kTrContext, 0);
    if (static_cast<s32>(call(ctx, np::sceNpTrophyCreateContext,
                              {t + kTrContext, load<u32>(t + kTrCommId), load<u32>(t + kTrSignature), 0})) < 0)
        return;
    store<u32>(t + kTrHandle, 0);
    if (static_cast<s32>(call(ctx, np::sceNpTrophyCreateHandle, {t + kTrHandle})) < 0) return;

    call(ctx, mfn::Event_Init, {t + kTrEvent60, 0});
    call(ctx, mfn::Event_Init, {t + kTrEvent112, 0});
    Stack frame(ctx, 48);  // ThreadAttr (32 bytes) at r1+112 of the original
    const u32 attr = frame.addr();
    call(ctx, mfn::ThreadAttr_Init, {attr});
    store<u32>(attr + 4, kTrThreadStack);
    store<u32>(attr + 28, kTrThreadName);
    call(ctx, mfn::Thread_Create, {t + kTrThread, attr, kTrThreadOpd, t + kTrLock});
    call(ctx, mfn::ThreadAttr_Destroy, {attr});
}
