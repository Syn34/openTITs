// Game code, large-function sweep part 35: the skeleton set's IFF chunk of
// rigid-body face adjacency tables.
#include "native/objtypes.h"

namespace sly4::obj {

// The read outputs (a stack block).
struct FaceAdjVars {
    be_u32 version;      // +0
    be_u32 num_bodies;   // +4
    be_u32 name_len;     // +8
    be_u32 total_edges;  // +12
    be_u32 num_faces;    // +16
    be_u32 face_key;     // +20
    be_u32 adj_key;      // +24
    u8 valid_edge;       // +28
    pad<3> _29;
};
static_assert(sizeof(FaceAdjVars) == 32);
struct FaceAdjEntry {
    be_u16 key;     // +0: faceKey << 3 | valid edge bits
    be_u16 adj[3];  // +2
};
struct FaceAdjTable {
    be_u32 body;    // +0
    be_u16 faces;   // +4
    be_u16 edges;   // +6
    // +8: FaceAdjEntry[faces]
};
struct BodyProp {
    be_u32 key;     // +0
    pad<4> _4;
    be_u64 value;   // +8
};
struct HkBodyProps {
    pad<124> _0;
    gp<BodyProp> props;  // +124
    be_s32 count;        // +128
};
struct SkelSetTables {
    pad<44> _0;
    HkArray<be_u32> tables;  // +44
};

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
// skipped (returns 1) when kLoadFlagA is set and kLoadFlagB clear. Reads
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
u32 SkelSet_LoadFaceAdjacency34714C(u32 r) {
    if (global<u8>(kLoadFlagA) != 0 && global<u8>(kLoadFlagB) == 0) return 1;
    GuestFrame frame(32);
    FaceAdjVars* s = frame.as<FaceAdjVars>();
    s->version = 0u;
    call(Iff_ReadU32, r, &s->version, kStrVersion);
    s->num_bodies = 0u;
    call(Iff_ReadU32, r, &s->num_bodies, kStrNumRigidBodies);
    if (s->num_bodies == 0) return 1;
    for (u32 i = 0;;) {
        s->name_len = 0u;
        s->total_edges = 0u;
        s->num_faces = 0u;
        call(Iff_ReadU32, r, &s->name_len, kStrRbNameLen);
        const u32 name = call<u32>(Mem_Alloc, s->name_len + 1u, 0u, 16u);
        call(FileReader_Read, r, name, s->name_len.get());
        gm<u8>(name + s->name_len) = 0;
        call(Iff_ReadU32, r, &s->total_edges, kStrRbTotalEdges);
        call(Iff_ReadU32, r, &s->num_faces, kStrRbNumFaces);
        const u32 size = ((s->num_faces << 3) & 0xFFFFFFF8u) + 8u;
        u32 table = call<u32>(Mem_Alloc, size, 0u, 16u);
        FaceAdjTable& t = gm<FaceAdjTable>(table);  // quirk: at address 0 when the allocation failed
        const u32 nf = s->num_faces;
        t.faces = static_cast<u16>(nf);
        t.edges = static_cast<u16>(s->total_edges);
        if (nf != 0) {
            for (u32 f = 0, ea = table + 8;; ea += 8) {
                FaceAdjEntry& e = gm<FaceAdjEntry>(ea);
                s->face_key = 0xFFFFFFFFu;
                s->valid_edge = 0;
                s->adj_key = 0xFFFFFFFFu;
                call(Iff_ReadU32, r, &s->face_key, kStrFaceKey);
                e.key = static_cast<u16>((s->face_key << 3) & 0xFFF8u);
                for (u32 j = 0; j < 3; ++j) {
                    call(Iff_ReadU8, r, &s->valid_edge, kStrValidEdge);
                    if (s->valid_edge != 0) e.key = static_cast<u16>(e.key | ((1u << j) & 7u));
                }
                for (u32 j = 0; j < 3; ++j) {
                    call(Iff_ReadU32, r, &s->adj_key, kStrAdjKey);
                    e.adj[j] = static_cast<u16>(s->adj_key);
                }
                if (++f >= s->num_faces) break;
            }
        }
        const u32 body = call<u32>(Skel_FindBody9DEE58, global<be_u32>(kSkelSystem).get(), name);
        t.body = body;
        if (body != 0) {
            u32 has = 0;
            HkBodyProps* b = host<HkBodyProps>(body);
            const s32 n = b->count;
            for (s32 k = 0; k < n; ++k) {
                BodyProp* list = b->props;
                if (list[k].key == kAdjProperty) {
                    has = static_cast<u32>(list[k].value.get());  // ld of the u64; cmpwi tests its low word
                    break;
                }
            }
            if (global<u8>(kLoadFlagB) != 0) call(Body_SetProperty94032C, body, kMarkProperty, 1u);
            if (has == 0) {
                call(Body_SetProperty94032C, t.body.get(), kAdjProperty, table);
                SkelSetTables* set = global<gp<SkelSetTables>>(kSkelSet);
                u32 cnt = static_cast<u32>(set->tables.size.get());
                if (cnt == (set->tables.capflags & 0x3FFFFFFFu)) {
                    call(HkArray_Grow7CCD60, kHkAllocator, &set->tables, 4u);
                    cnt = static_cast<u32>(set->tables.size.get());
                }
                be_u32& slot = gm<be_u32>(set->tables.data.ea + ((cnt << 2) & 0xFFFFFFFCu));
                const u32 mem = global<be_u32>(kSkelMemory) + size;
                slot = table;
                const u32 cnt2 = static_cast<u32>(set->tables.size.get()) + 1u;
                global<be_u32>(kSkelMemory) = mem;
                set->tables.size = static_cast<s32>(cnt2);
                table = 0;
            }
        }
        if (name != 0) call(Mem_FreeFrom, name, 0u);
        if (table != 0) call(Mem_FreeFrom, table, 0u);
        if (++i >= s->num_bodies) break;
    }
    return 1;
}

}  // namespace sly4::obj

NATIVE(0034714C, sly4::obj::SkelSet_LoadFaceAdjacency34714C)
