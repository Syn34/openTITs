// Game code, large-function sweep part 30: stopping an object's voice and
// those of its attached children.
#include "big30.h"

namespace sly4::obj {

namespace {
constexpr u32 Obj_FindComp34CC34 = 0x0034CC34;  // (owner, key) -> component or 0
constexpr u32 Entry_Stop353CC0 = 0x00353CC0;    // (Entry*, u8 release)
}  // namespace

// A 240-byte attachment slot: +176 bit 31 attached, +192 the child object.
struct VoiceSlot {
    pad<176> _0;
    be_u32 flags;  // +176
    pad<12> _180;
    be_u32 child;  // +192
    pad<44> _196;
};
static_assert(sizeof(VoiceSlot) == 240);
struct VoiceSlots {
    be_u32 data;   // +0: VoiceSlot*
    pad<4> _4;
    be_u16 count;  // +8
};
struct VoiceObj {
    pad<164> _0;
    be_u32 owner;           // +164
    pad<172> _168;
    gp<VoiceSlots> slots;   // +340
};
static_assert(offsetof(VoiceObj, owner) == 164 && offsetof(VoiceObj, slots) == 340);

// void f(Obj* o, key, u8 release) (called by f_00060238,
// EffectCmd_StopAnim_0009658C, Obj_SetRenderTarget, Obj_Release364_000FA4F4,
// f_000FC628, f_00242DF8, PairTable_Add_0042C328 and itself; none reads
// r3). Each slot of o->slots whose flags have bit 31 set recurses into the
// slot's child. Then, with an owner, Obj_FindComp34CC34(owner, key) and,
// when found, Entry_Stop353CC0(it, release). The header pointer is re-read
// each pass (a callee may change it).
void Obj_StopVoiceTree_000DC314(VoiceObj* o_, u32 key, u8 rel) {
    VoiceObj* o = &gm<VoiceObj>(guest(o_));  // a null child is read at address 0 (unchecked)
    for (u32 i = 0, off = 0;; ++i, off += 240) {
        const VoiceSlots* hdr = o->slots;
        const u32 n = hdr != nullptr ? hdr->count : 0u;
        if (i >= n) break;
        const VoiceSlot& slot = gm<VoiceSlot>(hdr->data + off);
        if (slot.flags & 0x80000000u) Obj_StopVoiceTree_000DC314(host<VoiceObj>(slot.child), key, rel);
    }
    const u32 owner = o->owner;
    if (owner == 0) return;
    const u32 e = call<u32>(Obj_FindComp34CC34, owner, key);
    if (e == 0) return;
    call(Entry_Stop353CC0, e, rel);
}

}  // namespace sly4::obj

NATIVE(000DC314, sly4::obj::Obj_StopVoiceTree_000DC314)
