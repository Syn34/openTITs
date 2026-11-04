// World objects, part 6: the IFF child-chunk loop most loaders (the point
// instance among them) read their objects with.
#include "obj6.h"

using namespace sly4;
using namespace sly4::obj6;

// bool Iff_ReadChunks(IffReader* r, void* obj)  ("PointInstance_Load" in symbols.txt)
// Reads the chunks inside the current one: for each next chunk
// (0x00569108) the registered handler (the ChunkDesc in the map at
// 0x01164E30, its OPD at +4) is called with (r, obj) and its result decides
// whether to go on; an unregistered chunk goes to the hook at *0x00F8870C
// (r, fourcc), when there is one, and stops the loop. After each chunk the
// reader pops it (no pop at depth -1: entry 0 is used), calls the system
// hook when it left depth 1, seeks the stream (vfunc +20, whence 0) to its
// end and stores that at r +8; the loop goes on while the enclosing chunk
// has bytes left (its start + size against the stream's 64-bit position at
// +264, compared on the low 32 bits). Returns the last result, or 0 when
// no chunk was found.
DECOMP(00569388, Iff_ReadChunks) {
    const u32 r = arg<u32>(ctx, 0), obj = arg<u32>(ctx, 1);
    Stack frame(ctx, 8);  // sp+112: the map key
    const u32 key = frame.addr();
    u64 ok = 1;
    u32 chunk = 0;
    while (static_cast<u32>(ok) != 0) {
        chunk = static_cast<u32>(call(ctx, o6fn::Iff_NextChunk, {r}));
        if (chunk == 0) break;
        store<u32>(key, load<u32>(chunk));
        const u32 desc = load<u32>(static_cast<u32>(call(ctx, o6fn::SortedMap_Find, {kChunkMap, key})));
        if (desc != 0) {
            const u32 opd = load<u32>(desc + kChunkHandler);
            ok = call_ptr(ctx, opd, {r, obj, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
        } else {
            const u32 hook = load<u32>(kUnknownChunkHook);
            if (hook != 0) {
                call_ptr(ctx, hook, {r, load<u32>(chunk), hook, load<u32>(hook)});  // r5 / r6 = OPD and entry
                ok = 0;
            }
        }
        // Leave the chunk.
        u32 st = load<u32>(r + kIrStack);
        const s32 top = static_cast<s32>(load<u32>(st + kIsTop));
        s32 now;
        u32 e;
        if (top > -1) {
            store<u32>(st + kIsTop, static_cast<u32>(top - 1));
            e = st + static_cast<u32>(top) * kIsEntrySize + kIsEntries;
            now = static_cast<s32>(load<u32>(st + kIsTop));
        } else {
            e = st + kIsEntries;
            now = top;
        }
        const u32 start = load<u32>(e + kIeStart), size = load<u32>(e + kIeSize);
        if (now + 1 == 1) call(ctx, o6fn::SystemState_CallHook);
        const u32 stream = load<u32>(r + kIrStream);
        const u32 end = size + start;
        const u32 seek = load<u32>(load<u32>(stream) + kVfnStreamSeek);
        call_ptr(ctx, seek, {stream, end, 0, seek, load<u32>(seek)});  // r6 / r7 = OPD and entry
        st = load<u32>(r + kIrStack);
        store<u32>(r + kIrPos, end);
        const s32 outer = static_cast<s32>(load<u32>(st + kIsTop));
        if (outer <= -1) break;
        const u32 oe = st + static_cast<u32>(outer) * kIsEntrySize + kIsEntries;
        if (oe == 0) break;
        const u32 pos = static_cast<u32>(load<u64>(load<u32>(r + kIrStream) + kStreamPos));
        if (load<u32>(oe + kIeStart) + load<u32>(oe + kIeSize) - pos == 0) break;
    }
    if (chunk == 0) ok = 0;
    ret(ctx, ok);
}
