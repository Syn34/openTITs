// Game code, large-function sweep part 35: the skeleton set's IFF chunk of
// rigid-body face adjacency tables.
#include "../sly4.h"
#include "big35.h"

using namespace sly4;
using namespace sly4::big35;

namespace {
constexpr u32 kLoadFlagA = 0x0106B950;   // u8: with kLoadFlagB clear, the chunk is skipped
constexpr u32 kLoadFlagB = 0x0106B9A0;   // u8: also marks each found body (property 0x8CFD3EFD = 1)
constexpr u32 kSkelSystem = 0x00F64CAC;  // u32: the last loaded skeleton system (SkelSet_LoadChunk_00346F84)
constexpr u32 kSkelSet = 0x00F64C98;     // u32: the skeleton set (+44 Havok array of the tables: data, +48 count, +52 capacity in the low 30 bits)
constexpr u32 kSkelMemory = 0x00F64C88;  // u32: bytes handed to the set
constexpr u32 kHkAllocator = 0x011DF31C;
constexpr u32 kAdjProperty = 0xC2928F03, kMarkProperty = 0x8CFD3EFD;
constexpr u32 Iff_ReadU32 = 0x005642AC;      // (IffReader*, u32* out, name unused)
constexpr u32 Iff_ReadU8 = 0x005643C4;       // (IffReader*, u8* out, name unused)
constexpr u32 FileReader_Read = 0x00563768;  // (reader, buf, size)
constexpr u32 Mem_Alloc = 0x0054ED38;        // (size, 0, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;     // (p, pool)
constexpr u32 Skel_FindBody9DEE58 = 0x009DEE58;     // (system, const char* name) -> body or 0 (f_009DEE58)
constexpr u32 Body_SetProperty94032C = 0x0094032C;  // (body, u32 key, value) (f_0094032C)
constexpr u32 HkArray_Grow7CCD60 = 0x007CCD60;      // (allocator, array, element size) (f_007CCD60)
// the field names (unused by Iff_ReadU32 / Iff_ReadU8)
constexpr u32 kStrVersion = 0x00ADBED0, kStrNumRigidBodies = 0x00ADBED8, kStrRbNameLen = 0x00ADBEE8;
constexpr u32 kStrRbTotalEdges = 0x00ADBEF8, kStrRbNumFaces = 0x00ADBF08, kStrFaceKey = 0x00ADBF18;
constexpr u32 kStrValidEdge = 0x00ADBF20, kStrAdjKey = 0x00ADBF30;
}  // namespace

// u32 SkelSet_LoadFaceAdjacency34714C(IffReader* r) (a chunk handler):
// skipped (r3 = 1) when kLoadFlagA is set and kLoadFlagB clear. Reads
// "version" and "numRigidBodies"; for each body: the name ("rbNameLen"
// bytes into a Mem_Alloc'd buffer, terminated), "rbTotalEdges",
// "rbNumFaces" and a table of 8 + 8 * faces bytes: +0 the body, +4 / +6 the
// u16 face / edge counts, then per face u16 (faceKey << 3) with bits 0..2
// set for its "validEdge" bytes, and three u16 "adjKey"s. The body comes
// from Skel_FindBody9DEE58(*kSkelSystem, name). A found body whose property
// list (+124 16-byte entries {key, ?, u64 value}, s32 count +128) has no
// non-zero (low word) kAdjProperty value gets the table as that property
// and the table is pushed onto the set's array (kSkelMemory += its size);
// with kLoadFlagB each found body is also marked (before that). The name,
// and a table that was not handed over, are freed. Every read's output
// word is preset (0, or -1 for the face fields) as in the original.
// Returns 1.
DECOMP(0034714C, SkelSet_LoadFaceAdjacency34714C) {
    const u32 r = arg<u32>(ctx, 0);
    if (load<u8>(kLoadFlagA) != 0 && load<u8>(kLoadFlagB) == 0) {
        ret(ctx, 1u);
        return;
    }
    // sp+112..+143 in the original: version, numRigidBodies, rbNameLen,
    // rbTotalEdges, rbNumFaces, faceKey, adjKey (+24), validEdge (u8 +28)
    Stack frame(ctx, 32);
    const u32 s = frame.addr();
    store<u32>(s + 0, 0);
    call(ctx, Iff_ReadU32, {r, s + 0, kStrVersion});
    store<u32>(s + 4, 0);
    call(ctx, Iff_ReadU32, {r, s + 4, kStrNumRigidBodies});
    if (load<u32>(s + 4) == 0) {
        ret(ctx, 1u);
        return;
    }
    for (u32 i = 0;;) {
        store<u32>(s + 8, 0);
        store<u32>(s + 12, 0);
        store<u32>(s + 16, 0);
        call(ctx, Iff_ReadU32, {r, s + 8, kStrRbNameLen});
        const u32 name = static_cast<u32>(call(ctx, Mem_Alloc, {load<u32>(s + 8) + 1u, 0, 16}));
        call(ctx, FileReader_Read, {r, name, load<u32>(s + 8)});
        store<u8>(name + load<u32>(s + 8), 0);
        call(ctx, Iff_ReadU32, {r, s + 12, kStrRbTotalEdges});
        call(ctx, Iff_ReadU32, {r, s + 16, kStrRbNumFaces});
        const u32 size = ((load<u32>(s + 16) << 3) & 0xFFFFFFF8u) + 8u;
        u32 table = static_cast<u32>(call(ctx, Mem_Alloc, {size, 0, 16}));
        const u32 nf = load<u32>(s + 16);
        store<u16>(table + 4, static_cast<u16>(nf));
        store<u16>(table + 6, static_cast<u16>(load<u32>(s + 12)));
        if (nf != 0) {
            for (u32 f = 0, e = table + 8;; e += 8) {
                store<u32>(s + 20, 0xFFFFFFFFu);
                store<u8>(s + 28, 0);
                store<u32>(s + 24, 0xFFFFFFFFu);
                call(ctx, Iff_ReadU32, {r, s + 20, kStrFaceKey});
                store<u16>(e, static_cast<u16>((load<u32>(s + 20) << 3) & 0xFFF8u));
                for (u32 j = 0; j < 3; ++j) {
                    call(ctx, Iff_ReadU8, {r, s + 28, kStrValidEdge});
                    if (load<u8>(s + 28) != 0) store<u16>(e, static_cast<u16>(load<u16>(e) | ((1u << j) & 7u)));
                }
                for (u32 j = 0; j < 3; ++j) {
                    call(ctx, Iff_ReadU32, {r, s + 24, kStrAdjKey});
                    store<u16>(e + 2 + 2 * j, static_cast<u16>(load<u32>(s + 24)));
                }
                if (++f >= load<u32>(s + 16)) break;
            }
        }
        const u32 body = static_cast<u32>(call(ctx, Skel_FindBody9DEE58, {load<u32>(kSkelSystem), name}));
        store<u32>(table, body);
        if (body != 0) {
            u32 has = 0;
            const s32 n = static_cast<s32>(load<u32>(body + 128));
            for (s32 k = 0; k < n; ++k) {
                const u32 list = load<u32>(body + 124);
                if (load<u32>(list + 16 * static_cast<u32>(k)) == kAdjProperty) {
                    // ld of the u64 value; cmpwi tests its low word
                    has = static_cast<u32>(load<u64>(list + ((static_cast<u32>(k) << 4) & 0xFFFFFFF0u) + 8));
                    break;
                }
            }
            if (load<u8>(kLoadFlagB) != 0) call(ctx, Body_SetProperty94032C, {body, kMarkProperty, 1});
            if (has == 0) {
                call(ctx, Body_SetProperty94032C, {load<u32>(table), kAdjProperty, table});
                const u32 set = load<u32>(kSkelSet);
                u32 cnt = load<u32>(set + 48);
                if (cnt == (load<u32>(set + 52) & 0x3FFFFFFFu)) {
                    call(ctx, HkArray_Grow7CCD60, {kHkAllocator, set + 44, 4});
                    cnt = load<u32>(set + 48);
                }
                const u32 slot = load<u32>(set + 44) + ((cnt << 2) & 0xFFFFFFFCu);
                const u32 mem = load<u32>(kSkelMemory) + size;
                store<u32>(slot, table);
                const u32 cnt2 = load<u32>(set + 48) + 1u;
                store<u32>(kSkelMemory, mem);
                store<u32>(set + 48, cnt2);
                table = 0;
            }
        }
        if (name != 0) call(ctx, Mem_FreeFrom, {name, 0});
        if (table != 0) call(ctx, Mem_FreeFrom, {table, 0});
        if (++i >= load<u32>(s + 4)) break;
    }
    ret(ctx, 1u);
}
