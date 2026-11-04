// World objects, part 7: adding a draw (mesh, material) to a topology.
#include "obj7.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj6;

namespace {

// AglTopologyMeshMtlTable: the 320-byte (40 draws) table a topology moves
// to when its four inline draws (+16) are full.
constexpr u32 kMeshMtlTableList = 0x00F66EE8;    // header*: +0 allocator, +4 capacity (made on first use, 64)
constexpr u32 kMeshMtlTableListVtbl = 0x00B23E20;
constexpr u32 kStrMeshMtlTableList = 0x00AF1316; // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglTopologyMeshMtlTable]"
constexpr u32 kMeshMtlTableSize = 320;
constexpr u32 kMeshMtlTableCount = 64;
constexpr u32 kTopoInlineDraws = 16;             // {mesh, material}[4]
constexpr u32 kInlineDrawCount = 4;
constexpr u32 kDrawRefs = 4;                     // u16 in meshes / materials
constexpr u32 kVfnAllocNew = 12;
constexpr u32 kMeshName = 20;                    // the mesh's +20: the topology's name
constexpr u32 kTopoName = 96;                    // const char*

// *slot = v with reference counts: v gains one (u16 at +4), the old value
// is released (vfunc +8).
void RefAssign(PPUContext& ctx, u32 slot, u32 v) {
    const u32 old = load<u32>(slot);
    if (old == v) return;
    store<u32>(slot, v);
    if (v != 0) store<u16>(v + kDrawRefs, static_cast<u16>(load<u16>(v + kDrawRefs) + 1));
    if (old != 0) Release(ctx, old);
}

}  // namespace

// void Topology_AddDraw(AglTopology* t, Mesh* mesh, AglMaterial* mtl)
// Appends {mesh, mtl} (both reference counted) to the topology's draws
// and gives the mesh the topology's name (+96). The fifth draw first moves
// the four inline ones into a zeroed AglTopologyMeshMtlTable from its free
// list (its header made on first use with 64 entries; as in the original
// failed allocations go on with null pointers), releasing the inline ones.
DECOMP(00508B94, Topology_AddDraw) {
    const u32 t = arg<u32>(ctx, 0), mesh = arg<u32>(ctx, 1), mtl = arg<u32>(ctx, 2);
    if (load<u16>(t + kTopoDrawCount) == kInlineDrawCount) {
        if (load<u32>(kMeshMtlTableList) == 0) {
            const u32 h = static_cast<u32>(call(ctx, o4fn::Mem_Alloc, {8, 0, 16}));
            if (h != 0) {
                store<u32>(h, 0);
                store<u32>(h + 4, kMeshMtlTableCount);
            }
            store<u32>(kMeshMtlTableList, h);
        }
        const u32 alloc = ListAllocator(ctx, kMeshMtlTableList, kMeshMtlTableListVtbl, kStrMeshMtlTableList, kMeshMtlTableSize);
        const u32 table = static_cast<u32>(Vcall0(ctx, alloc, kVfnAllocNew));
        store<u32>(t + kTopoDraws, table);
        call(ctx, o4fn::memset, {table, 0, kMeshMtlTableSize});
        for (u32 i = 0, off = 0; i < load<u16>(t + kTopoDrawCount); ++i, off += 8) {
            const u32 src = t + kTopoInlineDraws + off;
            RefAssign(ctx, load<u32>(t + kTopoDraws) + off, load<u32>(src));
            RefAssign(ctx, load<u32>(t + kTopoDraws) + off + 4, load<u32>(src + 4));
            for (u32 k : {0u, 4u}) {
                const u32 v = load<u32>(src + k);
                if (v == 0) continue;
                store<u32>(src + k, 0);
                Release(ctx, v);
            }
        }
    }
    const u32 n = load<u16>(t + kTopoDrawCount);
    RefAssign(ctx, load<u32>(t + kTopoDraws) + ((n << 3) & 0x7FFF8u), mesh);
    RefAssign(ctx, load<u32>(t + kTopoDraws) + ((static_cast<u32>(load<u16>(t + kTopoDrawCount)) << 3) & ~7u) + 4, mtl);
    const u32 n1 = load<u16>(t + kTopoDrawCount) + 1u;
    store<u16>(t + kTopoDrawCount, static_cast<u16>(n1));
    store<u32>(mesh + kMeshName, load<u32>(t + kTopoName));
    ret(ctx, n1);
}
