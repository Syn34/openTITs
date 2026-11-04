// World objects, part 6: copying an AglTopology for a class copy, its
// materials shared through the copy table.
#include "obj6.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj6;

// AglTopology* Topology_Copy(const AglTopology* src, CopyTable* table)
// A new AglTopology (112 bytes) from the list behind *0x00F8F124 (the list
// made on first use; no copy at all when that header is null) constructed
// by 0x00508690. Each of src's draw entries (+48, 8 bytes each, +52
// count) {u32 a, AglMaterial* m} is added with 0x00508B94(copy, a, m'):
// m' is m's copy from the table ({src, copy} pairs, count at +1280; the
// first pair with this source and a non-null copy), or a new material
// copied from m (its +16 kept) that the table then records (no bounds
// check) after the creator's reference is dropped (vfunc +8). Then the
// +54 bits 0x80 / 0x40 / 0x20, +64..+76, +80, +96, +84, the ref-counted
// +88 (old one released by 0x005084CC), and a 16-byte copy of the +92
// record (bytes +8..+11, +12, +4, then 0x0079C668 with its +0; its +4
// then points back at the copy). Quirks kept: a null copy is written
// through, and a failed Mem_Alloc for the record copies into address 0.
DECOMP(005095C0, Topology_Copy) {
    const u32 src = arg<u32>(ctx, 0), table = arg<u32>(ctx, 1);
    u32 t = 0;
    if (load<u32>(kTopologyList) != 0) {
        const u32 alloc = ListAllocator(ctx, kTopologyList, kTopologyListVtbl, kStrTopologyList, kTopologySize);
        const u32 got = static_cast<u32>(Vcall0(ctx, alloc, 12));
        if (got != 0) {
            call(ctx, o6fn::Topology_Construct, {got});
            t = got;
        }
    }
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(src + kTopoDrawCount)); ++i) {
        const u32 e = load<u32>(src + kTopoDraws) + i * 8;
        const u32 a = load<u32>(e), mat = load<u32>(e + 4);
        u32 copy = 0;
        for (s32 j = 0; copy == 0 && j < static_cast<s32>(load<u32>(table + kCopyTableCount)); ++j)
            if (load<u32>(table + static_cast<u32>(j) * 8) == mat) copy = load<u32>(table + static_cast<u32>(j) * 8 + 4);
        if (copy != 0) {
            call(ctx, o6fn::Topology_AddDraw, {t, a, copy});
            continue;
        }
        const u32 keep16 = load<u32>(mat + 16);
        const u32 m = static_cast<u32>(call(ctx, o6fn::Material_Alloc));
        store<u32>(m + 16, keep16);
        call(ctx, o6fn::Material_CopyFrom, {m, mat});
        call(ctx, o6fn::Topology_AddDraw, {t, a, m});
        const u32 opd = load<u32>(load<u32>(m) + 8);
        call_ptr(ctx, opd, {m, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
        store<u32>(table + static_cast<u32>(static_cast<s32>(load<u32>(table + kCopyTableCount) << 3)), mat);
        store<u32>(table + 4 + (load<u32>(table + kCopyTableCount) << 3), m);
        store<u32>(table + kCopyTableCount, load<u32>(table + kCopyTableCount) + 1);
    }
    u8 b = load<u8>(t + kTopoFlags);
    for (u8 mask : {u8{0x80}, u8{0x40}, u8{0x20}}) {
        b = static_cast<u8>((b & ~mask) | (load<u8>(src + kTopoFlags) & mask));
        store<u8>(t + kTopoFlags, b);
    }
    for (u32 off : {64u, 68u, 72u, 76u, 80u, 96u}) store<u32>(t + off, load<u32>(src + off));
    const u32 v84 = load<u32>(src + 84);
    const u32 old = load<u32>(t + kTopoShared);
    store<u32>(t + 84, v84);
    const u32 sh = load<u32>(src + kTopoShared);
    if (old != sh) {
        store<u32>(t + kTopoShared, sh);
        if (sh != 0) store<u16>(sh, static_cast<u16>(load<u16>(sh) + 1));
        if (old != 0) call(ctx, o6fn::TopoShared_Release, {old});
    }
    const u32 rec = load<u32>(src + kTopoRecord);
    if (rec == 0) return ret(ctx, t);
    u32 r = 0;
    const u32 blk = static_cast<u32>(call(ctx, o6fn::Mem_Alloc, {kTopoRecordSize, 0, 16}));
    if (blk != 0) {
        r = blk;
        store<u32>(blk, 0);
    }
    for (u32 k = 0; k < 4; ++k) store<u8>(r + 8 + k, load<u8>(rec + 8 + k));
    store<u32>(r + 12, load<u32>(rec + 12));
    store<u32>(r + 4, load<u32>(rec + 4));
    call(ctx, o6fn::TopoRecord_Set, {r, load<u32>(rec)});
    store<u32>(t + kTopoRecord, r);
    store<u32>(r + 4, t);
    ret(ctx, t);
}
