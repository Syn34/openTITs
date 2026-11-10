// Game code, large-function sweep part 1: the constructor of the 976-byte
// map-screen object (vtable 0x00B1FA08, def flag +80 bit 16), built by
// Objects_CreateFromDefs35AC7C: its linked UI elements, script hashes,
// world objects, textures and the cloned map blips.
#include "big1.h"

using namespace sly4;
using namespace sly4::big1;

namespace b1m {
constexpr u32 Obj_BaseConstruct35695C = 0x0035695C;  // (Object*, u32 kind, Def*) (name?)
constexpr u32 Node_PushF65B7C = 0x00446984;          // (a, b) -> 24-byte node (name?)
constexpr u32 Material_FindByHash_00767D78 = 0x00767D78;  // (hash) -> AglMaterial node + 16, or 0 (name?)
constexpr u32 kVtMapScreen = 0x00B1FA08;
constexpr u32 kColorA4 = 0x01164CA4;       // u8[4] default colour
constexpr u32 kVec2Default = 0x01164AFC;   // f32[2] (bss)
constexpr u32 kTuning = 0x0106CD78;        // +72: a float
constexpr u32 kLinkCounter = 0x00F64F80;   // s32: the "%s_%i" name counter
constexpr u32 kFixedHashes = 0x00F64F4C;   // u32[7] (8-byte stride): world objects to find
constexpr u32 kNodeArg = 0x010B8054;       // +72: Node_PushF65B7C's second argument
constexpr u32 kStrMatFreeList = 0x00ADC60E;  // "ApeTFreeList<T> &...GetAllocator() [with T = AglMaterial]"
constexpr u32 kFmtLink = 0x00ADC760;       // "%s_%i"
constexpr u32 kFmtIntro = 0x00ADC768;      // "%s_Intro"
constexpr u32 kFmtOnUpdate = 0x00ADC778;   // "%s_OnUpdate"
constexpr u32 kFmtIdle = 0x00ADC788;       // "%s_Idle"
constexpr u32 kFmtStatus = 0x00ADC790;     // "%s_Status"
constexpr u32 kFmtMap = 0x00ADC7A0;        // "%s_Map"
constexpr u32 kFmtOutro = 0x00ADC7A8;      // "%s_Outro"
constexpr u32 kFmt02d = 0x00ADC7C4;        // "%s_%02d"
constexpr u32 kFmtNum = 0x00ADC7CC;        // "%s%d"
constexpr u32 kFmtBlipPlayer = 0x00ADC7D4;    // "MapBlipPlayer_%02d"
constexpr u32 kFmtBlipEnemy = 0x00ADC7EC;     // "MapBlipEnemy_%02d"
constexpr u32 kFmtBlipGoal = 0x00ADC804;      // "MapBlipGoal_%02d"
constexpr u32 kFmtBlipGoalEdge = 0x00ADC81C;  // "MapBlipGoalEdge_%02d"
constexpr u32 kFmtBlipBottle = 0x00ADC834;    // "MapBlipBottle_%02d"
constexpr u32 kStrBlipSafe = 0x00ADC84C;      // "MapBlipSafe_01"
constexpr u32 kBodyMode = 13;
constexpr u32 kKindModel = 8;  // WorldObj kind (+4 bits 10..15) of a model
}  // namespace b1m

namespace {

u32 Hash(PPUContext& ctx, u32 s) { return static_cast<u32>(call(ctx, b1fn::String_HashNoCase, {s})); }
u32 FindById(PPUContext& ctx, u32 o, u32 id) {
    return static_cast<u32>(call(ctx, b1fn::WorldObj_FindById4A5AE8, {load<u32>(o + 24), id}));
}
u32 FindByHash(PPUContext& ctx, u32 h, u32 kind) { return static_cast<u32>(call(ctx, b1fn::WorldObj_FindByHash, {h, kind})); }
void SetBodyMode(PPUContext& ctx, u32 w) { call(ctx, b1fn::Body_SetMode4A5C20, {w, b1m::kBodyMode}); }

// The material's render state at m + 104 {+0 flags: bit 31 set, bits
// 15..30 = 770 (blend), +4 u16 0x8006, +6 u16 771, +8 bit 31 cleared},
// the last through the pointer re-read from `reread`. def_tex: the store
// order the def texture uses (771 first), which also clears its u16 +12.
void SetMaterialBlend(u32 m, bool def_tex, u32 reread) {
    const u32 rs = m + 104;
    const u32 w = load<u32>(rs);
    if (def_tex) {
        store<u16>(rs + 6, 771);
        store<u16>(rs + 4, 0x8006);
    } else {
        store<u16>(rs + 4, 0x8006);
        store<u16>(rs + 6, 771);
    }
    store<u32>(rs, ((w | 0x80000000u) & ~0x7FFF8000u) | (770u << 15));
    const u32 rs2 = load<u32>(reread) + 104;
    const u32 w2 = load<u32>(rs2 + 8);
    if (def_tex) store<u16>(rs2 + 12, 0);
    store<u32>(rs2 + 8, w2 & 0x7FFFFFFFu);
}

// The fields set before anything is looked up (stores in the original's
// order; f1 = the last lfs, as the original leaves it).
void InitFields(PPUContext& ctx, u32 o, u32 def) {
    const u32 a = b1m::kVec2Default, b = b1m::kVec2Default + 4;
    store<u32>(o + 0, b1m::kVtMapScreen);
    mid2::CopyColor(o + 232, b1m::kColorA4);
    store<u32>(o + 244, mid2::kMinusOneF);
    store<u32>(o + 240, load<u32>(def + 156));  // lfs / stfs
    store<u32>(o + 248, 0);
    store<u32>(o + 492, 0xFF7FFFFFu);  // -FLT_MAX
    store<u32>(o + 496, 0x7F7FFFFFu);  // FLT_MAX
    static constexpr u32 kSrc[] = {a, b, a, b, a, b, b1m::kTuning + 72, a, b, a};
    for (u32 k = 0; k < 10; ++k) store<u32>(o + 544 + 4 * k, load<u32>(kSrc[k]));
    store<u32>(o + 956, 0);
    u32 f = load<u32>(o + 960);
    store<u16>(o + 958, 0);
    f = (f & 0x7FFFFFFFu) | ((load<u8>(def + 160) & 1u) << 31);
    store<u32>(o + 236, 0);
    store<u32>(o + 252, 0);
    store<u32>(o + 256, 0);
    store<u32>(o + 536, 0);
    store<u32>(o + 540, 0);
    store<u32>(o + 960, f);
    const u32 b160 = load<u8>(def + 160);
    store<u32>(o + 528, 0);
    store<u32>(o + 584, load<u32>(b));
    store<u32>(o + 948, 0);
    store<u32>(o + 952, 0);
    const u32 f4 = load<u32>(o + 4);
    store<u32>(o + 964, 0);
    store<u8>(o + 164, 0);
    store<u32>(o + 4, f4 | 0x10000u);
    // bit 30 from the same byte; bits 21..29 cleared; bit 20 set.
    f = (((f & ~0x40000000u) | ((b160 & 1u) << 30)) & ~0x3FE00000u) | 0x00100000u;
    store<u32>(o + 960, f);
    store<u32>(o + 968, 0);
    store<u32>(o + 972, 0);
    ctx.set_fpr_bits(1, LfsBits(load<u32>(b)));
}

// The def's 36-byte link records (+148, count +152) into 16-byte entries
// at o->+236 {+0 the world object of record +4, +4 / +8 / +12 the UI
// elements}. Records without flag +28 bit 31 copy +8..+16 as they are;
// the others instantiate the element template +8 (vfn[2](t, "<+96>_<n>",
// t->+148, t->+160, 1), n from a global counter), then find the +12
// element (+96 name) under it and the +16 text element (types 3 / 5) under
// that one (or the new element).
void ReadLinks(PPUContext& ctx, u32 o, u32 def) {
    if (load<u32>(def + 152) == 0) return;
    const u32 p = static_cast<u32>(call(ctx, b1fn::Mem_Alloc, {load<u32>(def + 152) << 4, 0, 16}));
    store<u32>(o + 236, static_cast<u32>(call(ctx, b1fn::memset, {p, 0, load<u32>(def + 152) << 4})));
    for (u32 k = 0; k < load<u32>(def + 152); ++k) {
        const u32 src = load<u32>(def + 148) + 36 * k;
        const u32 dst = load<u32>(o + 236) + 16 * k;
        store<u32>(dst + 0, FindById(ctx, o, load<u32>(src + 4)));
        if ((load<u32>(src + 28) & 0x80000000u) == 0) {
            store<u32>(dst + 4, load<u32>(src + 8));
            store<u32>(dst + 8, load<u32>(src + 12));
            store<u32>(dst + 12, load<u32>(src + 16));
            continue;
        }
        if (load<u32>(src + 8) == 0) continue;
        const u32 n = load<u32>(b1m::kLinkCounter) + 1;
        const u32 t = load<u32>(src + 8);
        store<u32>(b1m::kLinkCounter, n);
        const u32 name = static_cast<u32>(call(ctx, b1fn::String_Format, {b1m::kFmtLink, t + 96, static_cast<u64>(Sx(n))}));
        const u32 t2 = load<u32>(src + 8);
        const u32 e = static_cast<u32>(VCall(ctx, t, 8, {t, name, load<u32>(t2 + 148), load<u32>(t2 + 160), 1}));
        store<u32>(dst + 4, e);
        if (e == 0) continue;
        const u32 sub = load<u32>(src + 12);
        u32 parent = load<u32>(dst + 8);
        if (sub != 0) {
            parent = static_cast<u32>(call(ctx, b1fn::Ui_Find, {sub + 96, e}));
            store<u32>(dst + 8, parent);
        }
        if (parent == 0) parent = load<u32>(dst + 4);
        const u32 text = load<u32>(src + 16);
        if (text == 0) continue;
        const u32 u = static_cast<u32>(call(ctx, b1fn::Ui_Find, {text + 96, parent}));
        if (u == 0) continue;
        bool ok = static_cast<u32>(VCall(ctx, u, 12, {u})) == 3;
        if (!ok) ok = static_cast<u32>(VCall(ctx, u, 12, {u})) == 5;
        if (ok) store<u32>(dst + 12, u);
    }
}

// The script hashes +200..+228 (cleared first): "<name>_Intro" +204,
// "_OnUpdate" +208, "_Idle" +212, "_Status" +224, "_Map" +220, "_Outro"
// +228, from the def's prefix (+36) or else its name (+4).
void ScriptHashes(PPUContext& ctx, u32 o, u32 def) {
    for (u32 off = 200; off < 232; off += 4) store<u32>(o + off, 0);  // four std
    u32 name = def + 36;
    if (load<u8>(name) == 0) name = def + 4;
    if (load<u8>(name) == 0) return;
    static constexpr struct { u32 fmt, off; } kScripts[] = {
        {b1m::kFmtIntro, 204}, {b1m::kFmtOnUpdate, 208}, {b1m::kFmtIdle, 212},
        {b1m::kFmtStatus, 224}, {b1m::kFmtMap, 220}, {b1m::kFmtOutro, 228}};
    for (const auto& s : kScripts)
        store<u32>(o + s.off, Hash(ctx, static_cast<u32>(call(ctx, b1fn::String_Format, {s.fmt, name}))));
}

// The 10 rows of four named world objects +332 (16 bytes each, names
// "<def +260 / +292 / +324 / +356>_<NN>", NN = 1..10): +540 counts the
// rows whose first and last objects both exist, and the first row
// without them ends the table.
void ReadRows(PPUContext& ctx, u32 o, u32 def) {
    call(ctx, b1fn::memset, {o + 332, 0, 160});
    for (u32 i = 1;; ++i) {
        const u32 row = o + 332 + 16 * (i - 1);
        u32 last = 0;
        for (u32 c = 0; c < 4; ++c) {
            const u32 s = static_cast<u32>(call(ctx, b1fn::String_Format, {b1m::kFmt02d, def + 260 + 32 * c, i}));
            last = FindByHash(ctx, Hash(ctx, s), 0);
            store<u32>(row + 4 * c, last);
        }
        if (load<u32>(row) == 0 || last == 0) break;
        store<u32>(o + 540, load<u32>(o + 540) + 1);
        if (i >= 10) break;
    }
}

// One texture per row (+276, ten words cleared first): the live material
// named "<def +208>" (row 0) or "<def +208><k+1>" with a positive use
// count gets a new AglMaterial (Tex_Acquire) with the blend state set,
// and its texture object vfn[4](3, 3, 1).
void RowTextures(PPUContext& ctx, u32 o, u32 def) {
    for (u32 off = 276; off < 316; off += 4) store<u32>(o + off, 0);
    for (u32 k = 0; k < load<u32>(o + 540); ++k) {
        u32 name = def + 208;
        if (k != 0) name = static_cast<u32>(call(ctx, b1fn::String_Format, {b1m::kFmtNum, def + 208, k + 1}));
        const u32 h = Hash(ctx, name);
        if (h == 0) continue;
        u32 m = MatFirst(ctx, MatAllocator(ctx, b1m::kStrMatFreeList));
        while (m != 0 && load<u32>(m + 40) != h) m = MatNext(m);
        if (m == 0 || static_cast<s16>(load<u16>(m + 4)) <= 0) continue;
        const u32 slot = o + 276 + 4 * k;
        const u32 tex = static_cast<u32>(call(ctx, b1fn::Tex_Acquire, {load<u32>(m + 20)}));
        store<u32>(slot, tex);
        SetMaterialBlend(tex, false, slot);
        const u32 t = load<u32>(m + 20);
        VCall(ctx, t, 16, {t, 3, 3, 1});
    }
}

// count clones of the model named by hash into o + off ("<fmt>" with
// 1..count): each one vfn[14](model, 5), LOD group collected, renamed and
// given body mode 13. The model is shown first (vfn[10](0, 1)).
void SpawnBlips(PPUContext& ctx, u32 o, u32 hash, u32 off, u32 count, u32 fmt) {
    if (hash == 0) return;
    const u32 b = FindByHash(ctx, hash, 0);
    if (b == 0 || ((load<u32>(b + 4) >> 10) & 0x3Fu) != b1m::kKindModel) return;
    VCall(ctx, b, 40, {b, 0, 1});
    for (u32 i = 0; i < count; ++i) {
        const u32 slot = o + off + 4 * i;
        const u32 c = static_cast<u32>(VCall(ctx, b, 56, {b, 5}));
        store<u32>(slot, c);
        call(ctx, b1fn::Obj_CollectLodGroup, {c, 1});
        const u32 w = load<u32>(slot);
        const u32 name = count == 1 ? fmt : static_cast<u32>(call(ctx, b1fn::String_Format, {fmt, i + 1}));
        call(ctx, b1fn::WorldObj_SetName, {w, name, 1});
        SetBodyMode(ctx, load<u32>(slot));
    }
}

}  // namespace

// void Obj_Construct356BA0(MapScreen* o, Def* def): the base object, then
// the map screen's fields, links, scripts and world objects. Returns with
// r3 = the last WorldObj_FindById4A5AE8 result, as the original.
DECOMP(00356BA0, Obj_Construct356BA0) {
    const u32 o = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1);
    call(ctx, b1m::Obj_BaseConstruct35695C, {o, 0, def});
    InitFields(ctx, o, def);
    ReadLinks(ctx, o, def);
    ScriptHashes(ctx, o, def);
    // Four world objects by hash (kind 8): def +168 (8-byte stride) -> +260.
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 260 + 4 * k, 0);
    for (u32 k = 0; k < 4; ++k) {
        const u32 h = load<u32>(def + 168 + 8 * k);
        if (h != 0) store<u32>(o + 260 + 4 * k, FindByHash(ctx, h, 8));
    }
    // The def's texture (+164 names a material) -> +256.
    if (const u32 m = static_cast<u32>(call(ctx, b1m::Material_FindByHash_00767D78, {load<u32>(def + 164)})); m != 0) {
        const u32 tex = static_cast<u32>(call(ctx, b1fn::Tex_Acquire, {load<u32>(m + 20)}));
        store<u32>(o + 256, tex);
        SetMaterialBlend(tex, true, o + 256);
    }
    // Seven fixed world objects (+500), each shown and given body mode 13.
    for (u32 k = 0; k < 7; ++k) store<u32>(o + 500 + 4 * k, 0);
    for (u32 k = 0; k < 7; ++k) {
        const u32 w = FindByHash(ctx, load<u32>(b1m::kFixedHashes + 8 * k), 0);
        store<u32>(o + 500 + 4 * k, w);
        if (w == 0) continue;
        VCall(ctx, w, 40, {w, 0, 1});
        SetBodyMode(ctx, load<u32>(o + 500 + 4 * k));
    }
    store<u32>(o + 528, FindById(ctx, o, load<u32>(def + 200)));
    store<u32>(o + 532, FindById(ctx, o, load<u32>(def + 204)));
    if (const u32 body = load<u32>(o + 24); body != 0) SetBodyMode(ctx, body);
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 316 + 4 * k, FindById(ctx, o, load<u32>(def + 244 + 4 * k)));
    ReadRows(ctx, o, def);
    RowTextures(ctx, o, def);
    // The map blips.
    for (u32 off = 588; off < 624; off += 4) store<u32>(o + off, 0);
    SpawnBlips(ctx, o, load<u32>(def + 388), 588, 9, b1m::kFmtBlipPlayer);
    for (u32 off = 624; off < 752; off += 4) store<u32>(o + off, 0);  // stvx
    SpawnBlips(ctx, o, load<u32>(def + 392), 624, 32, b1m::kFmtBlipEnemy);
    for (u32 off = 752; off < 784; off += 4) store<u32>(o + off, 0);
    SpawnBlips(ctx, o, load<u32>(def + 396), 752, 8, b1m::kFmtBlipGoal);
    for (u32 off = 784; off < 816; off += 4) store<u32>(o + off, 0);
    SpawnBlips(ctx, o, load<u32>(def + 400), 784, 8, b1m::kFmtBlipGoalEdge);
    for (u32 off = 816; off < 944; off += 4) store<u32>(o + off, 0);
    SpawnBlips(ctx, o, load<u32>(def + 404), 816, 32, b1m::kFmtBlipBottle);
    store<u32>(o + 944, 0);
    SpawnBlips(ctx, o, load<u32>(def + 408), 944, 1, b1m::kStrBlipSafe);
    store<u32>(o + 948, static_cast<u32>(call(ctx, b1m::Node_PushF65B7C, {0x89D631DEu, load<u32>(b1m::kNodeArg + 72)})));
    store<u32>(o + 952, static_cast<u32>(call(ctx, b1m::Node_PushF65B7C, {0x34C6B374u, load<u32>(b1m::kNodeArg + 72)})));
    store<u32>(o + 964, FindById(ctx, o, load<u32>(def + 420)));
}
