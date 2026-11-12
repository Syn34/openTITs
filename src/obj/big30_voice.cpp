// Game code, large-function sweep part 30: stopping an object's voice and
// those of its attached children.
#include "../sly4.h"
#include "big30.h"

using namespace sly4;
using namespace sly4::big30;

// void f(Obj* o, key, u8 release) (called by f_00060238,
// EffectCmd_StopAnim_0009658C, Obj_SetRenderTarget, Obj_Release364_000FA4F4,
// f_000FC628, f_00242DF8, PairTable_Add_0042C328 and itself). o->+340 is an
// array header {Entry* data, ..., u16 count at +8} of 240-byte slots; each
// slot whose +176 has bit 31 set recurses into the slot's +192 object.
// Then, with an o->+164 owner, Obj_FindComp34CC34(owner, key) and, when
// found, Entry_Stop353CC0(it, release). The header pointer is re-read each
// pass (a callee may change it). No caller reads r3 or other leftovers;
// r3 is set as the original leaves it anyway.
DECOMP(000DC314, Obj_StopVoiceTree_000DC314) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 rel = static_cast<u8>(ctx.gpr[5]);
    u32 hdr;
    for (u32 i = 0, off = 0;; ++i, off += 240) {
        hdr = load<u32>(o + 340);
        const u32 n = hdr != 0 ? load<u16>(hdr + 8) : 0;
        if (i >= n) break;
        const u32 slot = load<u32>(hdr) + off;
        if (load<u32>(slot + 176) & 0x80000000u) call(ctx, b30fn::Obj_StopVoiceTree, {load<u32>(slot + 192), key, rel});
    }
    const u32 owner = load<u32>(o + 164);
    if (owner == 0) {
        ctx.gpr[3] = hdr;  // the last header read, as the original leaves r3
        return;
    }
    const u32 e = static_cast<u32>(call(ctx, b30fn::Obj_FindComp34CC34, {owner, key}));
    if (e == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, b30fn::Entry_Stop353CC0, {e, rel});
}
