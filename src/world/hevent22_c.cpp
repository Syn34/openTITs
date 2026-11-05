// World_HandleEvent callees, part 22c: the effect instance's time step
// (named by hevent21_j.cpp Effect_SetTime).
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent22;

namespace {
constexpr u32 kInstanceApply4BC36C = 0x004BC36C;  // (Instance*, Effect*; f32 from, f32 to) (name?)
constexpr u32 kZeroF = 0x004BC7C4;                // f32 0.0
// Instance: +16 Tree*, +32 f32 time, +36 f32 length, +40 f32 last step,
// +44 u32, +48 u8 looping, +49 u8 clamp, +50 u8 done.
constexpr u32 kInstTree = 16, kInstTime = 32, kInstLength = 36, kInstStep = 40, kInst44 = 44;
constexpr u32 kInstLoop = 48, kInstClamp = 49, kInstDone = 50;
// Instance +16 points to a tree whose +12 is the engine's map (hevent6.h:
// root, path, depth, current), iterated in key order; each node's value is
// a Track: vt +20 (t, Effect*, Instance*) -> bool, +8 children[], u16 +16
// count, each child vt +12 (c, Effect*, Instance*).
constexpr u32 kTreeMap = 12;

f64 lfs(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
}  // namespace

// u32 Instance_SetTime4BC7C8(Instance* inst, Effect* e; f32 dt) (name?):
// +40 = dt; every track in the +16 tree's map is visited in key order:
// vt +20 (t, e, inst), and when that returns true each child's vt +12. The
// tree is re-read from the instance after each visit. Then, unless done (+50),
// 0x004BC36C(inst, e; time, time + dt); time += dt unless clamped (+49)
// past the length. Looping (+48): a positive length wraps the time
// (0x004BC36C(inst, e; 0, wrapped) when it stays positive, +44 = 0), else
// time = 0; not looping and unclamped: done = 1 once time >= length.
// Returns 1 when it sets done, the call's result after a wrap, else 0.
DECOMP(004BC7C8, Instance_SetTime4BC7C8) {
    const u32 inst = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const f64 dt = farg(ctx, 0);
    store<f32>(inst + kInstStep, static_cast<f32>(dt));
    u32 tree = load<u32>(inst + kInstTree);
    u32 track = 0;
    if (tree != 0) {
        const u32 cur = hevent6::map_first(tree + kTreeMap);
        if (cur != 0) track = load<u32>(cur + hevent6::kNodeValue);
    }
    while (track != 0) {
        if (static_cast<u32>(vcall(ctx, track, 20, {track, e, inst})) != 0) {
            for (u32 k = 0; k < load<u16>(track + 16); ++k) {
                const u32 c = load<u32>(load<u32>(track + 8) + 4 * k);
                vcall(ctx, c, 12, {c, e, inst});
            }
        }
        tree = load<u32>(inst + kInstTree);  // re-read after the visit
        if (tree == 0) break;
        const u32 m = tree + kTreeMap;
        const u32 cur = load<u32>(m + hevent6::kMapNode);
        if (cur != 0) hevent6::map_next(m, cur);
        const u32 c = load<u32>(m + hevent6::kMapNode);
        track = c != 0 ? load<u32>(c + hevent6::kNodeValue) : 0;
    }

    if (load<u8>(inst + kInstDone) == 0) {
        const f64 t = lfs(inst + kInstTime);
        callf(ctx, kInstanceApply4BC36C, {inst, e}, {t, static_cast<f64>(static_cast<f32>(t + dt))});
    }
    const u32 clamp = load<u8>(inst + kInstClamp);
    f64 f1;
    if (clamp != 0 && !(lfs(inst + kInstTime) <= lfs(inst + kInstLength))) {
        f1 = lfs(inst + kInstTime);  // clamped: time stays
    } else {
        f1 = static_cast<f32>(lfs(inst + kInstTime) + dt);
        store<f32>(inst + kInstTime, static_cast<f32>(f1));
    }
    u32 r3 = 0;
    if (load<u8>(inst + kInstLoop) != 0) {
        const f64 len = lfs(inst + kInstLength);
        const f64 zero = lfs(kZeroF);
        if (!(len > zero)) {
            store<u32>(inst + kInstTime, 0);
        } else {
            const f64 t0 = lfs(inst + kInstTime);
            if (t0 >= len) {
                f64 t = t0;
                do t = static_cast<f32>(t - len);
                while (t >= len);
                store<f32>(inst + kInstTime, static_cast<f32>(t));
                f1 = t;
                if (t > zero) {
                    store<u32>(inst + kInst44, 0);
                    callf(ctx, kInstanceApply4BC36C, {inst, e}, {zero, t});
                    return;  // r3 / f1 as the call leaves them
                }
            }
        }
    } else if (clamp == 0) {
        f1 = lfs(inst + kInstTime);
        if (f1 >= lfs(inst + kInstLength)) {
            store<u8>(inst + kInstDone, 1);
            r3 = 1;
        }
    }
    ctx.gpr[3] = r3;
    ctx.fpr[1] = f1;
}

namespace {
constexpr u32 kAvlVarInsert = 0x00786468;          // (tree, u32 key, value) (symbols.txt AvlVar_Insert)
constexpr u32 kEffectBind4BB7E0 = 0x004BB7E0;      // (Effect*, Def*, 1) -> refcounted object (name?)
constexpr u32 kEffectInstance4BBABC = 0x004BBABC;  // (Def*, Effect*) -> instance (name?)
constexpr u32 kDefaultDefHash = 0x04B8CC42;        // def name hash looked up for the first instance
constexpr u32 kDefParts = 12;                      // Def +12: tree whose +12 map holds the part defs
constexpr u32 kKindBound = 5;
}  // namespace

// u32 Effect_Init4C761C(Effect* e) (name?): with a def (+0): every part
// def in the def's +12 tree (in key order, the tree re-read from e after
// each step) gets vt +36 (p); a non-null result x is inserted into e's +8
// tree under x +8, and when x's vt +32 returns 5 twice and x +20 is set,
// x +28 is replaced by 0x004BB7E0(e, x +20, 1) (old one vt +8, new one's
// u16 +4 count incremented). Then the def named 0x04B8CC42 in the def's
// set gives e +4 = Effect_Instance4BBABC(d, e). +136 bit 0x80 is cleared.
// Returns 0 (e when there is no def).
DECOMP(004C761C, Effect_Init4C761C) {
    using namespace hevent6;
    const u32 e = arg<u32>(ctx, 0);
    if (load<u32>(e) == 0) {
        ctx.gpr[3] = e;
        return;
    }
    u32 tree = load<u32>(load<u32>(e) + kDefParts);
    u32 part = 0;
    if (tree != 0) {
        const u32 cur = map_first(tree + kTreeMap);
        if (cur != 0) part = load<u32>(cur + kNodeValue);
    }
    while (part != 0) {
        const u32 x = static_cast<u32>(vcall(ctx, part, 36, {part}));
        if (x != 0) {
            call(ctx, kAvlVarInsert, {e + 8, load<u32>(x + 8), x});
            if (static_cast<s32>(vcall(ctx, x, 32, {x})) == static_cast<s32>(kKindBound) &&
                static_cast<s32>(vcall(ctx, x, 32, {x})) == static_cast<s32>(kKindBound)) {
                const u32 src = load<u32>(x + 20);
                if (src != 0) {
                    const u32 b = static_cast<u32>(call(ctx, kEffectBind4BB7E0, {e, src, 1}));
                    const u32 old = load<u32>(x + 28);
                    if (old != 0) vcall(ctx, old, 8, {old});
                    store<u32>(x + 28, b);
                    if (b != 0) store<u16>(b + 4, static_cast<u16>(load<u16>(b + 4) + 1));
                }
            }
        }
        tree = load<u32>(load<u32>(e) + kDefParts);
        if (tree == 0) break;
        const u32 m = tree + kTreeMap;
        const u32 cur = load<u32>(m + kMapNode);
        if (cur != 0) map_next_lf(m, cur);
        const u32 c = load<u32>(m + kMapNode);
        part = c != 0 ? load<u32>(c + kNodeValue) : 0;
    }
    const u32 def = load<u32>(e);
    const u32 d = effect_def_lookup(def, kDefaultDefHash);
    if (d != 0) store<u32>(e + 4, static_cast<u32>(call(ctx, kEffectInstance4BBABC, {d, e})));
    store<u32>(e + 136, load<u32>(e + 136) & ~0x80u);
    ctx.gpr[3] = 0;
}

namespace {
constexpr u32 kSmallAlloc = 0x0055238C;          // (size) -> block or 0 (as hevent7.h)
constexpr u32 kInstanceBase4B71BC = 0x004B71BC;  // (Instance*, 0, 0, 0): base constructor (name?)
constexpr u32 kInstanceVtable = 0x00B21CC0;      // 52-byte instance (VtB21CC0_Dtor / _Delete)
constexpr u32 kAvlTreeInsert = 0x0078EFEC;       // (tree, u32 key, value) (symbols.txt Avl_TreeInsert_0078EFEC)
constexpr u32 kAvlFInsert78F898 = 0x0078F898;    // (tree, -, value; f32 key) (name?)
constexpr u32 kInstanceSize = 52, kTreeSize = 104;
// Def: +8 u32, +12 / +16 / +20 trees of parts (map at +12), +36 f32
// length, +48 u8 looping, +49 u8 clamp. Instance: +8, +12 / +16 / +20
// trees built from them (allocated on first use).
constexpr u32 kDefWord8 = 8, kDefLoop = 48, kDefClamp = 49, kDefLength = 36;

// A new empty tree (104 bytes: +0 root, +4 count, +8 u8 1, +12 = &root, map
// path / depth / current) stored in *slot, or the null pointer when the
// allocation fails. Returns the tree as re-read from *slot.
u32 tree_get(PPUContext& ctx, u32 slot) {
    u32 t = load<u32>(slot);
    if (t != 0) return t;
    t = static_cast<u32>(call(ctx, kSmallAlloc, {kTreeSize}));
    store<u32>(slot, t);
    if (t == 0) return 0;
    store<u32>(t + kTreeMap + hevent6::kMapNode, 0);
    store<u8>(t + 8, 1);
    store<u16>(t + kTreeMap + hevent6::kMapDepth, 0);
    store<u32>(t + kTreeMap, t);
    store<u32>(t + 4, 0);
    store<u32>(t + 0, 0);
    ctx.gpr[4] = 1;  // left in r4 by the inlined constructor
    return load<u32>(slot);
}

// The value of the current node of the def's tree at +off after map_first,
// or 0.
u32 tree_begin(u32 def, u32 off) {
    const u32 tree = load<u32>(def + off);
    if (tree == 0) return 0;
    const u32 cur = hevent6::map_first(tree + kTreeMap);
    return cur != 0 ? load<u32>(cur + hevent6::kNodeValue) : 0;
}
// The next value of the def's tree at +off (re-read), or 0.
u32 tree_step(u32 def, u32 off) {
    const u32 tree = load<u32>(def + off);
    if (tree == 0) return 0;
    const u32 m = tree + kTreeMap;
    const u32 cur = load<u32>(m + hevent6::kMapNode);
    if (cur != 0) hevent6::map_next(m, cur);
    const u32 c = load<u32>(m + hevent6::kMapNode);
    return c != 0 ? load<u32>(c + hevent6::kNodeValue) : 0;
}
}  // namespace

// Instance* Effect_Instance4BBABC(Def* d, Effect* e) (name?): a new
// 52-byte instance (vtable 0x00B21CC0, base 0x004B71BC(i, 0, 0, 0), fields
// zeroed; a failed allocation still gets every store at address 0, as in
// the original) takes the def's looping / length / clamp flags and +51 =
// 1. Each part of the def's +12 tree whose vt +36 makes an object x goes
// into the instance's +12 tree by x +8 (AvlVar_Insert); when x's vt +32 is
// 5 and x +20 is set, x +28 becomes 0x004BB7E0(e, x +20, 1) (old one vt
// +8, new one's u16 +4 count incremented). The +16 tree's parts go into
// the instance's +16 tree by their +32 key and the +20 tree's parts into
// its +20 tree by their vt +12 float, each part's u16 +4 count
// incremented. +8 = def +8. Returns the instance.
DECOMP(004BBABC, Effect_Instance4BBABC) {
    const u32 d = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 inst = static_cast<u32>(call(ctx, kSmallAlloc, {kInstanceSize}));
    if (inst != 0) {
        call(ctx, kInstanceBase4B71BC, {inst, 0, 0, 0});
        store<u32>(inst + 32, 0);
        store<u32>(inst + 36, 0);
        store<u8>(inst + 48, 0);
        store<u8>(inst + 50, 0);
        store<u8>(inst + 51, 0);
        store<u32>(inst + 0, kInstanceVtable);
        store<u8>(inst + 49, 0);
        store<u32>(inst + 40, 0);
        store<u32>(inst + 44, 0);
    }
    store<u8>(inst + 48, load<u8>(d + kDefLoop));
    store<u8>(inst + 51, 1);
    store<u32>(inst + 36, load<u32>(d + kDefLength));  // lfs / stfs: raw copy
    ctx.fpr[1] = lfs(d + kDefLength);                  // and left in f1
    store<u8>(inst + 49, load<u8>(d + kDefClamp));

    for (u32 part = tree_begin(d, 12); part != 0; part = tree_step(d, 12)) {
        const u32 x = static_cast<u32>(vcall(ctx, part, 36, {part}));
        if (x == 0) continue;
        const u32 key = load<u32>(x + 8);
        const u32 t = tree_get(ctx, inst + 12);
        call(ctx, kAvlVarInsert, {t, key, x});
        if (static_cast<s32>(vcall(ctx, x, 32, {x})) != static_cast<s32>(kKindBound)) continue;
        const u32 src = load<u32>(x + 20);
        if (src == 0) continue;
        const u32 b = static_cast<u32>(call(ctx, kEffectBind4BB7E0, {e, src, 1}));
        const u32 old = load<u32>(x + 28);
        if (old != 0) vcall(ctx, old, 8, {old});
        store<u32>(x + 28, b);
        if (b != 0) store<u16>(b + 4, static_cast<u16>(load<u16>(b + 4) + 1));
    }
    for (u32 part = tree_begin(d, 16); part != 0; part = tree_step(d, 16)) {
        const u32 key = load<u32>(part + 32);
        const u32 t = tree_get(ctx, inst + 16);
        call(ctx, kAvlTreeInsert, {t, key, part});
        store<u16>(part + 4, static_cast<u16>(load<u16>(part + 4) + 1));
    }
    for (u32 part = tree_begin(d, 20); part != 0; part = tree_step(d, 20)) {
        vcall(ctx, part, 12, {part});
        const f64 key = ctx.fpr[1];
        const u32 t = tree_get(ctx, inst + 20);
        // r4 is not set for this call (0x0078F898 does not read it).
        callf(ctx, kAvlFInsert78F898, {t, ctx.gpr[4], part}, {key});
        store<u16>(part + 4, static_cast<u16>(load<u16>(part + 4) + 1));
    }
    store<u32>(inst + 8, load<u32>(d + kDefWord8));
    ret(ctx, inst);
}
