// World_HandleEvent callees, part 22e: small session / level helpers from
// the remaining callee list.
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent22;

namespace {
constexpr u32 kSessionPtr = 0x00F64C20;              // u32 Session*
constexpr u32 kSessionCurrentPart = 0x002DDD24;      // (Session*) -> Part* (symbols.txt Session_CurrentPart2DDD24)
constexpr u32 kSessionResetPart = 0x002E5060;        // (Session*) (symbols.txt Session_ResetPart2E5060)
constexpr u32 kSessionReset = 0x002E0860;            // (Session*, u8 keep) (symbols.txt Session_Reset2E0860)
constexpr u32 kWorldStartLevel = 0x004674C4;         // (World*, u32 level, u8) (symbols.txt World_StartLevel)
constexpr u32 kObjNotify26 = 0x000D94CC;             // (Object*) -> Coach* (symbols.txt Obj_Notify26)
constexpr u32 kCoachSetMode = 0x002A8AE0;            // (Coach*, u8 enter, u32 sub, u8 pause) (symbols.txt Coach_SetMode2A8AE0)
constexpr u32 kLevelPending = 0x00F65D8C;            // u8, cleared before a restart (name?)
constexpr u32 kPartLevel = 744;                      // Part +744: level id
constexpr u32 kSessionActive = 40;                   // Session +40: running part
constexpr u32 kWorldRestartFlag = 0x12811C;          // World u8 (name?)
constexpr u32 kWorldPartObj = 0x128164, kPartObjLevel = 5532;
constexpr u32 kUserObj = 8, kUserObjFlags = 192, kUserObjCoach = 0x8;  // user block 0 +8 Object*, +192 bit 0x8
}  // namespace

// u32 World_RestartLevel48A2F8() (name?): with a current part that has a
// level (+744): the running part is reset (Session_ResetPart2E5060), 0x00F65D8C
// = 0, user block 0's object leaves coach mode when +192 has bit 0x8 and
// Obj_Notify26 gives a coach (Coach_SetMode2A8AE0(c, 0, 1, 0)), World
// +0x12811C = 0 and World_StartLevel(w, part level, 0). Without one, when
// the world part object has a level (+5532): Session_Reset2E0860(s, 1) and
// World_StartLevel(w, that level, 0). Returns as the original leaves r3.
DECOMP(0048A2F8, World_RestartLevel48A2F8) {
    const u32 part = static_cast<u32>(call(ctx, kSessionCurrentPart, {load<u32>(kSessionPtr)}));
    const u32 sess = load<u32>(kSessionPtr);
    if (part == 0 || load<u32>(part + kPartLevel) == 0) {
        const u32 p2 = static_cast<u32>(call(ctx, kSessionCurrentPart, {sess}));
        if (p2 != 0 && load<u32>(p2 + kPartLevel) != 0) {
            ctx.gpr[3] = load<u32>(p2 + kPartLevel);
            return;
        }
        const u32 w = load<u32>(kCurrentWorld);
        ctx.gpr[3] = w;
        if (load<u32>(load<u32>(w + kWorldPartObj) + kPartObjLevel) == 0) return;
        store<u8>(kLevelPending, 0);
        store<u8>(w + kWorldRestartFlag, 0);
        call(ctx, kSessionReset, {load<u32>(kSessionPtr), 1});
        const u32 w2 = load<u32>(kCurrentWorld);
        call(ctx, kWorldStartLevel, {w2, load<u32>(load<u32>(w2 + kWorldPartObj) + kPartObjLevel), 0});
        return;
    }
    if (load<u32>(sess + kSessionActive) != 0) call(ctx, kSessionResetPart, {sess});
    store<u8>(kLevelPending, 0);
    const u32 obj = load<u32>(load<u32>(kCurrentWorld) + kUserBlocks + kUserObj);
    if (obj != 0 && (load<u32>(obj + kUserObjFlags) & kUserObjCoach) != 0) {
        if (static_cast<u32>(call(ctx, kObjNotify26, {obj})) != 0) {
            const u32 coach = static_cast<u32>(call(ctx, kObjNotify26, {obj}));
            call(ctx, kCoachSetMode, {coach, 0, 1, 0});
        }
    }
    const u32 w = load<u32>(kCurrentWorld);
    store<u8>(w + kWorldRestartFlag, 0);
    const u32 p = static_cast<u32>(call(ctx, kSessionCurrentPart, {load<u32>(kSessionPtr)}));
    call(ctx, kWorldStartLevel, {load<u32>(kCurrentWorld), p != 0 ? load<u32>(p + kPartLevel) : 0u, 0});
}

namespace {
constexpr u32 kObjFindComponent = 0x000D1288;  // (Object*, u32 kind, 0) -> component (symbols.txt Obj_FindComponent)
constexpr u32 kObjOwner = 0x000D93FC;          // (Object*) -> owner component (symbols.txt Obj_Owner0D93FC)
constexpr u32 kObjTransform = 0x002CD91C;      // (Comp*, u8, const Vec3*, const Vec3*, 0, 0) -> bool (symbols.txt Obj_Transform2CD91C)
constexpr u32 kVecA = 0x01164B40, kVecB = 0x01164B70;  // two Vec3 constants given to it (name?)
constexpr u32 kCompKind44 = 44;
constexpr u32 kMinusOneF = 0xBF800000u;
}  // namespace

// u32 Obj_Snap1662F4(Object* o) (name?): with a kind 44 component c and a
// vt +64 object x: x vt +8, the owner component (if any) vt +16, c +348 =
// -1.0, o's +96 3x4 matrix (rows of 16 bytes, 12 used) copied to c +128,
// then Obj_Transform2CD91C(c, 1, &a, &b, 0, 0) with the two Vec3 constants
// at 0x01164B40 / 0x01164B70 on the stack; when it succeeds o vt +28 (o,
// 0, 7). Returns the last call's result (0 when a lookup fails).
DECOMP(001662F4, Obj_Snap1662F4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = static_cast<u32>(call(ctx, kObjFindComponent, {o, kCompKind44, 0}));
    if (c == 0) return;
    const u32 x = static_cast<u32>(vcall(ctx, o, 64, {o}));
    if (x == 0) return;
    vcall(ctx, x, 8, {x});
    const u32 own = static_cast<u32>(call(ctx, kObjOwner, {o}));
    if (own != 0) vcall(ctx, own, 16, {own});
    store<u32>(c + 348, kMinusOneF);
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 12; k += 4) store<u32>(c + 128 + 16 * row + k, load<u32>(o + 96 + 16 * row + k));  // lfs / stfs: raw
    Stack frame(ctx, 32);
    const u32 a = frame.addr(), b = frame.addr() + 16;
    for (u32 k = 0; k < 12; k += 4) store<u32>(a + k, load<u32>(kVecA + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(b + k, load<u32>(kVecB + k));
    ctx.fpr[1] = hevent7::lfs_f64(kVecB + 4);  // f1 / f2 as the copy leaves them
    ctx.fpr[2] = hevent7::lfs_f64(kVecB + 8);
    if (static_cast<u32>(call(ctx, kObjTransform, {c, 1, a, b, 0, 0})) == 0) return;
    vcall(ctx, o, 28, {o, 0, 7});
}

namespace {
constexpr u32 kFileOpen = 0x0055BD3C;          // (path, mode) -> File* (symbols.txt File_Open)
constexpr u32 kModeRb = 0x00ADBE84;            // "rb"
constexpr u32 kStreamVtable = 0x00B1F338;      // stack file stream reader (vtable 0x00B1F330 + 8) (name?)
constexpr u32 kStreamInit7CE648 = 0x007CE648;  // (StreamState*) (name?)
constexpr u32 kStreamFree7CEA58 = 0x007CEA58;  // (StreamState*) (name?)
constexpr u32 kHkLoad802E08 = 0x00802E08;      // (Stream*, Result*, {0, 0, 0} by value) -> object (name?)
constexpr u32 kLevelLoad34AB90 = 0x0034AB90;   // (path, arg, object, u32 file size) -> result (name?)
constexpr u32 kHkRemoveRef = 0x007D4EA8;       // (obj) (symbols.txt AnimSkel_Release7D4EA8)
// The stack reader (frame offsets as the original's 208-byte frame):
// +112 vtable, +118 u16 1, +120 File*, +128 u64 0, +136 u8 has file, +137
// u8 owns file; +144 u32 result, +148 state; +64 a 12-byte zero struct.
constexpr u32 kRd = 112, kRdFile = 120, kRdOwns = 137, kRes = 144, kState = 148, kZeros = 64;

// The reader's destructor, inlined: state freed, an owned file closed (vt +8).
void reader_close(PPUContext& ctx, u32 s) {
    call(ctx, kStreamFree7CEA58, {s + kState});
    const u32 f = load<u32>(s + kRdFile);
    if (f == 0 || load<u8>(s + kRdOwns) == 0) return;
    store<u32>(s + kRd, kStreamVtable);
    vcall(ctx, f, 8, {f});
}
}  // namespace

// u32 Level_LoadFile34AF14(const char* path, u32 arg) (name?): opens path
// ("rb"), reads it through a stack stream with 0x00802E08 and, when that
// gives an object, returns 0x0034AB90(path, arg, object, file size (vt
// +24)) after releasing the object; the stream is torn down either way.
// Returns 0 when the file cannot be opened or nothing was loaded.
DECOMP(0034AF14, Level_LoadFile34AF14) {
    const u32 path = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1);
    const u32 file = static_cast<u32>(call(ctx, kFileOpen, {path, kModeRb}));
    if (file == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 size = static_cast<u32>(vcall(ctx, file, 24, {file}));
    Stack frame(ctx, 208);
    const u32 s = frame.addr();
    store<u32>(s + kRdFile, file);
    store<u16>(s + 118, 1);
    store<u8>(s + kRdOwns, 0);
    store<u32>(s + kRd, kStreamVtable);
    store<u64>(s + 128, 0);
    store<u32>(s + kRes, 0);
    store<u8>(s + 136, 1);  // file != 0
    call(ctx, kStreamInit7CE648, {s + kState});
    store<u32>(s + kZeros + 0, 0);
    store<u32>(s + kZeros + 8, 0);
    store<u32>(s + kZeros + 4, 0);
    // The struct goes by value in r5 / r6 (ld of +64 and +72; +76 unset).
    const u32 obj = static_cast<u32>(call(ctx, kHkLoad802E08, {s + kRd, s + kRes, load<u64>(s + kZeros), load<u64>(s + kZeros + 8)}));
    if (obj == 0) {
        reader_close(ctx, s);
        ctx.gpr[3] = 0;
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, kLevelLoad34AB90, {path, a1, obj, size}));
    call(ctx, kHkRemoveRef, {obj});
    reader_close(ctx, s);
    ctx.gpr[3] = r;
}

namespace {
// Map of scene nodes by name hash (hevent6.h map layout from 0x010C2CB8,
// i.e. +12 of the object at 0x010C2CAC); last value found at 0x011DE48C.
constexpr u32 kNodeMap = 0x010C2CAC + 12;
constexpr u32 kNodeMapLast = 0x011DE48C;
constexpr u32 kNodeChildren = 256;  // Node +256: PtrVector of child name hashes (+0 data, u16 +8 count)

// The inlined map lookup: the node for key, or 0. A hit stores it as
// current and its value in the global; a miss stores the last node visited
// as current; an empty map clears the global, then current.
u32 node_lookup(u32 key) {
    using namespace hevent6;
    const u32 root = load<u32>(load<u32>(kNodeMap));
    if (root == 0) {
        store<u32>(kNodeMapLast, 0);
        store<u32>(kNodeMap + kMapNode, 0);
        return 0;
    }
    const u32 n = map_walk(kNodeMap, root, key);
    if (n == 0) return 0;
    store<u32>(kNodeMap + kMapNode, n);
    store<u32>(kNodeMapLast, load<u32>(n + kNodeValue));
    return n;
}
}  // namespace

// u32 Scene_VisitChildren47C68C(a, Node* node) (name?): for each child
// name hash in node +256, looks it up in the node map (twice, as the
// original) and recurses into the node found (a, child). Returns the
// child count (r3 = a for a null node).
DECOMP(0047C68C, Scene_VisitChildren47C68C) {
    const u32 a = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1);
    if (node == 0) return;
    const u32 v = node + kNodeChildren;
    u32 i = 0;
    for (;; ++i) {
        const u32 n = load<u16>(v + 8);
        if (i >= n) {
            ctx.gpr[3] = n;
            return;
        }
        const u32 key = load<u32>(load<u32>(v) + 4 * i);
        if (node_lookup(key) == 0) continue;
        const u32 n2 = node_lookup(key);
        if (n2 == 0) continue;
        const u32 child = load<u32>(n2 + hevent6::kNodeValue);
        if (child != 0) call(ctx, 0x0047C68C, {a, child});
    }
}
