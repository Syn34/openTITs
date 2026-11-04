// World objects, part 12: small callees of the definition / follower code:
// the IFF field reads, the topology radius and dump, the spline tangent
// wrapper, the pointer table's remove and the class maps' lookups/removes.
#include "obj12.h"

using namespace sly4;
using namespace sly4::obj12;

namespace {

using obj6::kIrStack;
using obj6::kIrStream;
using obj6::kIrPos;
using obj6::kIsTop;
using obj6::kIsEntries;
using obj6::kIsEntrySize;
using obj6::kIeSize;

// The stream read the IFF field readers share: stream vfunc +12 (stream,
// out, size, 0) with the OPD / entry left in r7 / r8; the bytes read are
// added to the reader's position. Returns the count (zero-extended).
u32 StreamRead(PPUContext& ctx, u32 r, u32 out, u32 size) {
    const u32 stream = load<u32>(r + kIrStream);
    const u32 opd = load<u32>(load<u32>(stream) + kVfnStreamRead);
    const u32 n = static_cast<u32>(call_ptr(ctx, opd, {stream, out, size, 0, opd, load<u32>(opd)}));
    store<u32>(r + kIrPos, load<u32>(r + kIrPos) + n);
    return n;
}

// The class maps' lookup template (obj2.cpp ClassMap_Find): the value slot
// for *key, sorting the entries first when they changed, or the default
// slot. Direct unsigned compares with the default comparator (the key
// re-read on every probe); otherwise the map's comparator (re-read from
// the map) on (entry, &key copy at sp+112), r5 / r6 = OPD and entry.
u32 MapFind(PPUContext& ctx, u32 m, u32 keyp, u32 default_compare) {
    const u16 flags = load<u16>(m + obj::kMapFlags);
    if ((flags & 1) != 0) {
        store<u16>(m + obj::kMapFlags, static_cast<u16>(flags & 0xFFFE));
        const u32 n = load<u16>(m + obj::kMapCount);
        if (static_cast<s32>(n) > 0)
            call(ctx, obj::ofn::qsort, {load<u32>(m + obj::kMapEntries), n, obj::kMapEntrySize, load<u32>(m + obj::kMapCompare)});
    }
    s32 hi = static_cast<s32>(load<u16>(m + obj::kMapCount)) - 1, lo = 0;
    u32 entries = load<u32>(m + obj::kMapEntries);
    s32 found = -1;
    if (load<u32>(m + obj::kMapCompare) == default_compare) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 key = load<u32>(keyp);
            const u32 k = load<u32>(entries + static_cast<u32>(mid * 8));
            if (k > key) {
                hi = mid - 1;
            } else if (k < key) {
                lo = mid + 1;
            } else {
                found = mid;
                break;
            }
        }
    } else {
        Stack frame(ctx, 160 - 112);
        const u32 key = frame.addr();  // sp+112
        store<u32>(key, load<u32>(keyp));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 cmp = load<u32>(m + obj::kMapCompare);
            const s32 r = static_cast<s32>(
                call_ptr(ctx, cmp, {entries + static_cast<u32>(mid * 8), key, cmp, load<u32>(cmp)}));
            if (r > 0) {
                hi = mid - 1;
            } else if (r < 0) {
                lo = mid + 1;
            } else {
                entries = load<u32>(m + obj::kMapEntries);
                found = mid;
                break;
            }
        }
    }
    if (found < 0) return m + obj::kMapDefault;
    return entries + static_cast<u32>(found * 8) + 4;
}

// The class maps' remove template: the same sort and search, then the
// later entries shifted down one (the count re-read on every step) and
// the count decremented. r3 on return: the new count when found; on a
// miss the count (direct search), the key (comparator, nothing probed) or
// the last comparator result, as the original leaves it.
void MapRemove(PPUContext& ctx, u32 m, u32 keyp, u32 default_compare) {
    const u16 flags = load<u16>(m + obj::kMapFlags);
    if ((flags & 1) != 0) {
        store<u16>(m + obj::kMapFlags, static_cast<u16>(flags & 0xFFFE));
        const u32 n = load<u16>(m + obj::kMapCount);
        if (static_cast<s32>(n) > 0)
            call(ctx, obj::ofn::qsort, {load<u32>(m + obj::kMapEntries), n, obj::kMapEntrySize, load<u32>(m + obj::kMapCompare)});
    }
    u32 count = load<u16>(m + obj::kMapCount);
    u32 entries = load<u32>(m + obj::kMapEntries);
    s32 hi = static_cast<s32>(count) - 1, lo = 0;
    s32 found = -1;
    u64 r3 = count;
    if (load<u32>(m + obj::kMapCompare) == default_compare) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 key = load<u32>(keyp);
            const u32 k = load<u32>(entries + static_cast<u32>(mid * 8));
            if (k > key) {
                hi = mid - 1;
            } else if (k < key) {
                lo = mid + 1;
            } else {
                found = mid;
                break;
            }
        }
    } else {
        Stack frame(ctx, 160 - 112);
        const u32 key = frame.addr();  // sp+112
        r3 = load<u32>(keyp);
        store<u32>(key, static_cast<u32>(r3));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 cmp = load<u32>(m + obj::kMapCompare);
            r3 = call_ptr(ctx, cmp, {entries + static_cast<u32>(mid * 8), key, cmp, load<u32>(cmp)});
            const s32 r = static_cast<s32>(r3);
            if (r > 0) {
                hi = mid - 1;
            } else if (r < 0) {
                lo = mid + 1;
            } else {
                count = load<u16>(m + obj::kMapCount);
                entries = load<u32>(m + obj::kMapEntries);
                found = mid;
                break;
            }
        }
    }
    if (found < 0) {
        ctx.gpr[3] = r3;
        return;
    }
    if (static_cast<s32>(count) >= 2 && found < static_cast<s32>(count) - 1) {
        s32 k = found;
        do {
            const u32 e = entries + static_cast<u32>(k * 8);
            count = load<u16>(m + obj::kMapCount);
            store<u32>(e, load<u32>(e + 8));
            store<u32>(e + 4, load<u32>(e + 12));
            ++k;
        } while (k < static_cast<s32>(count) - 1);
    }
    const s32 now = static_cast<s32>(count) - 1;
    store<u16>(m + obj::kMapCount, static_cast<u16>(now));
    ret(ctx, now);
}

}  // namespace

// f32 Topology_Radius(const AglTopology* t)
// The radius of the topology's shared block (+88, its f32 at +16), or 0
// without one. r3 = the block (or the constant's address), as the
// original leaves it.
DECOMP(0050A268, Topology_Radius) {
    const u32 sh = load<u32>(arg<u32>(ctx, 0) + obj6::kTopoShared);
    if (sh == 0) {
        ret(ctx, static_cast<f64>(load<f32>(kTopoNoRadius)));
        ctx.gpr[3] = kTopoNoRadius;
        return;
    }
    ret(ctx, static_cast<f64>(load<f32>(sh + kTsRadius)));
    ctx.gpr[3] = sh;
}

// f32 Spline_TangentAt(SplineData* d, f32 t, f32 out[4], bool flag)
// Spline_TangentEval(d, -, out, d +12 (the last segment), flag). The r4
// it passes on is the caller's (not read).
DECOMP(00560690, Spline_TangentAt) {
    const u32 d = arg<u32>(ctx, 0);
    callf(ctx, o12fn::Spline_TangentEval, {d, ctx.gpr[4], ctx.gpr[5], load<u32>(d + 12), ctx.gpr[6] & 0xFF},
          {farg(ctx, 0)});
}

// u32 Iff_ChunkSize(IffReader* r)
// The size of the chunk on top of the reader's stack. Quirk: with an empty
// stack (top -1) it reads address 8, as the original does. r4 / r5 = the
// entry (or 0) / the stack, as the original leaves them.
DECOMP(005637D4, Iff_ChunkSize) {
    const u32 st = load<u32>(arg<u32>(ctx, 0) + kIrStack);
    const s32 top = static_cast<s32>(load<u32>(st + kIsTop));
    const u32 e = top > -1 ? st + kIsEntries + static_cast<u32>(top) * kIsEntrySize : 0u;
    ctx.gpr[5] = st;
    ctx.gpr[4] = e;
    ret(ctx, load<u32>(e + kIeSize));
}

// u32 Iff_ReadU8(IffReader* r, u8* out)
// One byte from the stream into *out; returns the bytes read. r4 = the
// new position, as the original leaves it.
DECOMP(005643C4, Iff_ReadU8) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 n = StreamRead(ctx, r, arg<u32>(ctx, 1), 1);
    ctx.gpr[4] = load<u32>(r + kIrPos);
    ret(ctx, n);
}

// void Iff_ReadU16(IffReader* r, u16* out, const char* name)
// A little-endian u16 from the stream into *out (the name is not used).
// r3 = the bytes read, r4 / r5 the value / its low byte, as the original
// leaves them.
DECOMP(00564340, Iff_ReadU16) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 n = StreamRead(ctx, r, out, 2);
    const u32 b0 = load<u8>(out), b1 = load<u8>(out + 1);
    const u32 v = (b1 << 8) | b0;
    store<u16>(out, static_cast<u16>(v));
    ctx.gpr[3] = n;
    ctx.gpr[4] = v;
    ctx.gpr[5] = b0;
}

// void Iff_ReadU32(IffReader* r, u32* out, const char* name)
// A little-endian u32 from the stream into *out (the name is not used).
// r3 = the bytes read, r4 the value, r5 / r6 / r7 its bytes 1 / 2 / 3
// from the top (b2, b1, b0 of the stream), as the original leaves them.
DECOMP(005642AC, Iff_ReadU32) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 n = StreamRead(ctx, r, out, 4);
    const u32 b3 = load<u8>(out + 3), b2 = load<u8>(out + 2), b1 = load<u8>(out + 1), b0 = load<u8>(out);
    const u32 v = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
    store<u32>(out, v);
    ctx.gpr[3] = n;
    ctx.gpr[4] = v;
    ctx.gpr[5] = b2;
    ctx.gpr[6] = b1;
    ctx.gpr[7] = b0;
}

// void Topology_Dump(const AglTopology* t, s32 depth)
// Topology_DumpDraw(record) for each draw (the count re-read every step;
// the depth is not passed on). r3 = the last count read.
DECOMP(0050A19C, Topology_Dump) {
    const u32 t = arg<u32>(ctx, 0);
    u32 n = load<u16>(t + obj6::kTopoDrawCount);
    for (u32 i = 0; i < n; n = load<u16>(t + obj6::kTopoDrawCount)) {
        call(ctx, o12fn::Topology_DumpDraw, {load<u32>(load<u32>(t + obj6::kTopoDraws) + i * kTopoDrawStride)});
        ++i;
    }
    ctx.gpr[3] = n;
}

// void PtrTable_Remove(PtrTable* t, u32 index)
// Moves the last element into `index` (in the data array and, when there
// is one, the parallel array), tells the header's callback (aux[index],
// index) when both are set and the moved element is nonzero, then drops
// the count (from the header re-read after the callback). A table of one
// (or none) just gets count 0 (r3 = 0).
DECOMP(007A8444, PtrTable_Remove) {
    const u32 t = arg<u32>(ctx, 0), idx = arg<u32>(ctx, 1);
    u32 hdr = load<u32>(t + kPtHeader);
    const u32 n = load<u16>(hdr + kPhCount);
    if (static_cast<s32>(n) <= 1) {
        store<u16>(hdr + kPhCount, 0);
        ret(ctx, 0u);
        ctx.gpr[4] = hdr;
        return;
    }
    const u32 last = n - 1;
    const u32 data = load<u32>(t + kPtData);
    store<u32>(data + idx * 4, load<u32>(data + last * 4));
    u32 aux = load<u32>(t + kPtAux);
    if (aux != 0) {
        store<u32>(aux + idx * 4, load<u32>(aux + last * 4));
        hdr = load<u32>(t + kPtHeader);
        aux = load<u32>(t + kPtAux);
    }
    const u32 cb = load<u32>(hdr + kPhMoved);
    if (aux != 0 && cb != 0) {
        const u32 moved = load<u32>(aux + idx * 4);
        if (moved != 0) {
            call_ptr(ctx, cb, {moved, idx, load<u32>(cb), moved, hdr, cb});  // r5 = entry, r6..r8 left over
            hdr = load<u32>(t + kPtHeader);
        }
    }
    const u32 now = load<u16>(hdr + kPhCount) - 1;
    store<u16>(hdr + kPhCount, static_cast<u16>(now));
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(static_cast<s32>(now)));
    ctx.gpr[4] = hdr;
}

// u32* MapE_Find(Map* m, const u32* key)  (the emitter definition name map)
DECOMP(0075C1DC, MapE_Find) { ret(ctx, MapFind(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), kMapCompareE)); }

// void MapB/C/D/E_Remove(Map* m, const u32* key): the class maps' removes
// (holders obj.h kMapHolderB / C / D; E the emitter definition name map).
DECOMP(007A7A80, MapB_Remove) { MapRemove(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), obj2::kMapCompareB); }
DECOMP(007A892C, MapC_Remove) { MapRemove(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), obj2::kMapCompareC); }
DECOMP(007A8B20, MapD_Remove) { MapRemove(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), obj2::kMapCompareD); }
DECOMP(007A8250, MapE_Remove) { MapRemove(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), kMapCompareE); }
