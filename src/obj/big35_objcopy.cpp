// Game code, large-function sweep part 35: the constructor of the
// 3280-byte object (vtable 0x00B1E3E0) built by 0x003292C8.
#include "../sly4.h"
#include "big35.h"

using namespace sly4;
using namespace sly4::big35;

namespace {
constexpr u32 kVtable = 0x00B1E3E0;
constexpr u32 Obj_Construct_00769BD8 = 0x00769BD8;  // (Obj*, const Def*): the base class
constexpr u32 PtrVector_Construct = 0x005516E0;     // (vec)
constexpr u32 PtrVector_Init = 0x00551598;          // (vec, capacity, a, b)
constexpr u32 PtrVector_Reserve = 0x00551718;       // (vec, capacity)

inline void CopyWords(u32 dst, u32 src, u32 bytes) {
    for (u32 k = 0; k < bytes; k += 4) store<u32>(dst + k, load<u32>(src + k));
}
}  // namespace

// u32 ObjB1E3E0_Construct328D24(Obj* o, const Def* d, const Desc* s, const
// Vec12* list) (from 0x003292C8, on a 3280-byte SmallAlloc block): the base
// Obj_Construct_00769BD8(o, d); +2572 0, +0 the vtable, +2576 its top two
// bits cleared. Then raw word copies from s (the lfs / stfs pairs too):
// s+0..+180 to +2592; three 96-byte records s+192.. to +2784.. (words +0..
// +84 except +12); s+480 / +484 to +3072 / +3076; s+488..+500 to +3080;
// s+512..+520 to +3104, +528..+536 to +3120, +544..+552 to +3136, +560..
// +568 to +3152 (with +576 / +580 to +3168 / +3172 and +584 to +3176
// interleaved as in the original); s+592..+640 to +3184. Then the vector
// +3248 (Construct, step 12 at +3254, Init(16, 0, 1)) gets a copy of each
// 12-byte element of list (+0 data, +8 u16 count). Returns list's count.
DECOMP(00328D24, ObjB1E3E0_Construct328D24) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 2), list = arg<u32>(ctx, 3);
    call(ctx, Obj_Construct_00769BD8, {o, ctx.gpr[4], ctx.gpr[5], ctx.gpr[6]});
    const u32 flags = load<u32>(o + 2576) & 0x3FFFFFFFu;  // rlwinm 0,1,31 then rlwimi of 0 into bit 1
    store<u32>(o + 2572, 0);
    store<u32>(o + 0, kVtable);
    store<u32>(o + 2576, flags);
    CopyWords(o + 2592, s, 180);
    for (u32 r = 0; r < 3; ++r) {
        const u32 src = s + 192 + 96 * r, dst = o + 2784 + 96 * r;
        CopyWords(dst, src, 12);
        CopyWords(dst + 16, src + 16, 72);
    }
    CopyWords(o + 3072, s + 480, 8);
    CopyWords(o + 3080, s + 488, 16);
    CopyWords(o + 3104, s + 512, 12);
    CopyWords(o + 3120, s + 528, 12);
    CopyWords(o + 3136, s + 544, 12);
    store<u32>(o + 3152, load<u32>(s + 560));
    store<u32>(o + 3156, load<u32>(s + 564));
    store<u32>(o + 3168, load<u32>(s + 576));
    store<u32>(o + 3160, load<u32>(s + 568));
    store<u32>(o + 3172, load<u32>(s + 580));
    store<u32>(o + 3176, load<u32>(s + 584));
    CopyWords(o + 3184, s + 592, 52);
    ctx.fpr[1] = Lfs(s + 584);  // f1 is left as the last lfs (the vector calls below are integer-only)

    const u32 v = o + 3248;  // +0 data, +4 u16 capacity, +8 u16 count, +10 u16 high-water mark
    call(ctx, PtrVector_Construct, {v});
    store<u16>(o + 3254, 12);
    call(ctx, PtrVector_Init, {v, 16, 0, 1});
    u32 n;
    for (u32 i = 0;; ++i) {
        n = load<u16>(list + 8);
        if (i >= n) break;
        u32 cnt = load<u16>(v + 8);
        const u32 src = load<u32>(list) + 12 * i;
        const u32 cap = load<u16>(v + 4);
        if (cnt == cap) {
            call(ctx, PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
            cnt = load<u16>(v + 8);
        }
        CopyWords(load<u32>(v) + 12 * cnt, src, 12);
        const u32 c1 = load<u16>(v + 8) + 1u;
        const u32 hi = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(c1));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c1 & 0xFFFF) ? hi : c1));
    }
    ret(ctx, n);
}
