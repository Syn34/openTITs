// The trophy manager World_Init builds (and World_Destruct frees).
#include "meta.h"

namespace sly4::meta {
using namespace opentit::native;

namespace {
constexpr u32 kTrophyListElemFlags = 16;  // 8-byte elements, sorted
}  // namespace

// Clears the manager, sets up its sorted list (capacity entries of 8 bytes),
// publishes it at kTrophy, then initialises sceNpTrophy, creates the context
// and handle, two events and the "TrophyThread". Stops at the first sceNpTrophy
// call that fails (negative result).
void Trophy_Construct(Trophy* t, u32 comm_id, u32 sig, u32 callback, u16 capacity, u8 flag) {
    t->comm_id = comm_id;
    t->signature = sig;
    t->b8 = 0;
    t->b9 = 0;
    t->b10 = 0;
    t->flag = flag;
    t->b12 = 0;
    t->b13 = 0;
    t->u16 = 0u;
    call(mfn::Lock_Init, t->lock, 32u);

    t->b92 = 0;
    t->w164 = 0u;
    t->thread = 0u;
    t->b93 = 0;
    t->b94 = 0;
    t->w96 = 0u;
    t->w100 = 0u;
    t->state = 2u;
    t->callback = 0u;
    for (auto& w : t->w140) w = 0u;
    for (auto& d : t->u168) d = 0u;

    SortedArray& list = t->list;
    list.capacity = capacity;
    list.count = 0;
    list.high = 0;
    list.flags = kTrophyListElemFlags;
    list.data = 0u;
    Allocator* allocator = call<Allocator*>(mfn::Mem_DefaultAllocator);
    const u32 cap = list.capacity;
    list.allocator = allocator;
    if (cap != 0) {
        const u32 esize = (static_cast<u32>(list.flags) >> 1) & 0x7FFF;
        list.data = vcall<u32>(allocator, kAllocVfnAlloc, cap * esize, 0u, 16u);
    }
    call(mfn::Array_Init, &list);
    list.compare = kCompareU32;
    global<be_u32>(kTrophy) = guest(t);
    list.w16 = 0xFFFFFFFFu;
    t->callback = callback;

    if (call<s32>(np::sceNpTrophyInit, 0u, 0u, np::kMemContainerInvalid, 0u) < 0) return;
    t->context = 0u;
    if (call<s32>(np::sceNpTrophyCreateContext, &t->context, t->comm_id.get(), t->signature.get(), 0u) < 0) return;
    t->handle = 0u;
    if (call<s32>(np::sceNpTrophyCreateHandle, &t->handle) < 0) return;

    call(mfn::Event_Init, t->event60, 0u);
    call(mfn::Event_Init, t->event112, 0u);
    GuestFrame frame(sizeof(ThreadAttr));
    ThreadAttr* attr = frame.as<ThreadAttr>();
    call(mfn::ThreadAttr_Init, attr);
    attr->stack = kTrThreadStack;
    attr->name = kTrThreadName;
    call(mfn::Thread_Create, &t->thread, attr, kTrThreadOpd, t->lock);
    call(mfn::ThreadAttr_Destroy, attr);
}

}  // namespace sly4::meta

NATIVE(004FB0F0, sly4::meta::Trophy_Construct)
