// World_HandleEvent callees, part 17b: object shadow effect, animator
// emitter / layer stop, speaker line, query set-up.
#include "hevent17.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent17;

namespace {
// Object: +216 def (+144 default shadow effect def, +148 its name hash);
// +356 shadow Effect*, +364 its type-12 component, +376 u32 flags (bit 17
// 0x20000: has shadow); +96 f32[4][4] matrix.
constexpr u32 kObjDef = 216, kDefShadow = 144, kDefShadowName = 148;
constexpr u32 kObjShadow = 356, kObjShadowComp = 364, kObjFlags376 = 376, kObjHasShadow = 0x20000;
constexpr u32 kObjMatrix = 96;
constexpr u32 kEffectDefFind = 0x004BDEDC;   // EffectDef_Find (u32 hash, u32 set, u8 all) -> def (symbols.txt)
constexpr u32 kEffectCreateDCA20 = 0x000DCA20;  // (Object*, def, 0, 1) -> Effect* (name?)
// Effect +136 u32 flags (bit 9 0x200 cleared); components: vt +32 type().
constexpr u32 kEffFlags136 = 136, kEffFlag200 = 0x200;
constexpr u32 kCompShadowA = 0x001E8ECC, kCompShadowB = 0xB32DB729;
constexpr u32 kCompVtType = 32, kCompTypeMatrix = 4, kCompTypeKeep = 12, kCompMatrix = 32;

// Animator: +448 s32 emitter count, +384 {u32 handle, u32 key}[];
// +36 Set*; 8 layers of 40 bytes from +40 (+0 key, +4 / +28 cleared).
constexpr u32 kAnimEmitCount = 448, kAnimEmitters = 384, kEmitSize = 8, kEmitKey = 4;
constexpr u32 kAnimSet = 36, kAnimLayers = 40, kAnimLayerSize = 40, kAnimNumLayers = 8;
constexpr u32 kLayerPtr = 4, kLayer28 = 28;
constexpr u32 kAnyKey = 0xFFFFFFFFu;
constexpr u32 kAnimFind34CC34 = 0x0034CC34;   // Anim_Find34CC34 (Set*, key) -> entry (symbols.txt)
constexpr u32 kEntryStop353CC0 = 0x00353CC0;  // (entry, 0) (name?)
constexpr u32 kAnimStop254564 = 0x00254564;   // (Animator*, key) (name?)

// Speaker: +4 Owner* (vt +12 on a missing line), +8 u32 (0: look the line
// up), +24 track (+16 Item*[], +20 s32 count), +32 u32, +92 vector of
// speakers (+0 data, +8 u16 count).
constexpr u32 kSpkOwner = 4, kSpkLine = 8, kSpkTrack = 24, kSpkPlaying = 32, kSpkList = 92;
constexpr u32 kTrackItems = 16, kTrackCount = 20, kOwnerVtMissing = 12;
constexpr u32 kTextName = 8;
constexpr u32 kLineMap = 0x0106BC14, kVoiceMap = 0x0106BC40;
constexpr u32 kSortedMapFind76D894 = 0x0076D894;  // (map, const u32* key) -> slot (symbols.txt)
constexpr u32 kSortedMapFind76DD98 = 0x0076DD98;  // (map, const u32* key) -> slot (symbols.txt)
constexpr u32 kHashToString = 0x0056A6E0;         // Hash_ToString (u32) -> const char* (symbols.txt)
constexpr u32 kSpkSetLine34F690 = 0x0034F690;     // (Speaker*, line, 0) (name?)
constexpr u32 kSpkFirst35133C = 0x0035133C;       // (Speaker*, 0) -> Speaker* (name?)
constexpr u32 kSpkSay350958 = 0x00350958;         // (Speaker*, text, u8, -, -, -, r9, r10; f1, f2, f3) (name?)
constexpr u32 kItemVoiceA4CC64 = 0x00A4CC64;      // (Item*, voice) (name?)

// Query (0x000CDD28): +0 vector (built on demand, grow step 192), +4..+12
// cleared, +16 u32, +20 u32 bits (31..17 kind, 16..6 flags), +24 u8, +28,
// +32 def, +36, +40 u32, +48 f32[4][3] (3x4 of a 4x4), +112 vec3, +128 u32,
// +132 / +136 f32, +140 u32.
constexpr u32 kQVec = 0, kQBits = 20, kQByte = 24, kQDef = 32, kQMat = 48, kQPos = 112;
constexpr u32 kQ128 = 128, kQF132 = 132, kQF136 = 136, kQ140 = 140;
// Def: +32 bit 30 (0x40000000) overrides flag bit 14, +36 / +40 f32
// override the two floats, +136 vector of 192-byte templates.
constexpr u32 kDefFlags32 = 32, kDefFlagBit = 0x40000000, kDefF36 = 36, kDefF40 = 40, kDefTemplates = 136;
constexpr u32 kTemplateSize = 192;
constexpr u32 kVecAdd733964 = 0x00733964;  // (vector, const Template*) (name?)

// Stack argument n (n >= 8) of the caller's frame: the low word of its
// parameter save area doubleword.
inline u32 stack_arg(u32 sp, u32 n) { return load<u32>(sp + 112 + 8 * (n - 8) + 4); }
}  // namespace

// u32 Obj_SetShadow0E0000(Object* o, u32 name) (name?): creates the shadow
// effect (o +356) from the def named name (the def's default when name is 0,
// the default's name or not found), marks it (+376 bit 17, effect +136 bit
// 9 cleared), copies o's matrix into its type-4 component 0x001E8ECC and
// keeps its type-12 component 0xB32DB729 at +364.
DECOMP(000E0000, Obj_SetShadow0E0000) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    const u32 d = load<u32>(o + kObjDef);
    u32 def = load<u32>(d + kDefShadow);
    u32 r3 = name;
    if (name != 0 && name != load<u32>(d + kDefShadowName)) {
        r3 = static_cast<u32>(call(ctx, kEffectDefFind, {name, 0, 1}));
        if (r3 != 0) def = r3;
    }
    if (def == 0) return ret(ctx, r3);
    const u32 e = static_cast<u32>(call(ctx, kEffectCreateDCA20, {o, def, 0, 1}));
    store<u32>(o + kObjShadow, e);
    if (e == 0) return ret(ctx, 0u);
    store<u32>(e + kEffFlags136, load<u32>(e + kEffFlags136) & ~kEffFlag200);
    store<u32>(o + kObjFlags376, load<u32>(o + kObjFlags376) | kObjHasShadow);
    u32 c = static_cast<u32>(call(ctx, hevent7::h7fn::Effect_FindComponent, {load<u32>(o + kObjShadow), kCompShadowA}));
    if (c != 0 && static_cast<u32>(vcall(ctx, c, kCompVtType, {c})) == kCompTypeMatrix)
        hevent13::copy_mat(ctx, c + kCompMatrix, o + kObjMatrix);
    c = static_cast<u32>(call(ctx, hevent7::h7fn::Effect_FindComponent, {load<u32>(o + kObjShadow), kCompShadowB}));
    if (c == 0) return ret(ctx, 0u);
    if (static_cast<u32>(vcall(ctx, c, kCompVtType, {c})) == kCompTypeKeep) store<u32>(o + kObjShadowComp, c);
}

// u32 Anim_Stop254814(Animator* a, u32 key) (name?): stops (with fade) and
// removes every emitter with key (all for key -1; the last one moves into
// the hole), stops the set entry for key, then 0x00254564 (a, key), and
// clears the first layer with key (+4, +28). Returns that layer's a + 40k,
// or 8 when none matches.
DECOMP(00254814, Anim_Stop254814) {
    const u32 a = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    s32 n = static_cast<s32>(load<u32>(a + kAnimEmitCount));
    if (n > 0) {
        for (s32 j = 0; j < n; ++j) {
            const u32 e = a + kAnimEmitters + kEmitSize * static_cast<u32>(j);
            if (load<u32>(e + kEmitKey) != key && key != kAnyKey) continue;
            call(ctx, hevent::hfn::Emitter_Stop, {load<u32>(e), 1});
            u32 m = load<u32>(a + kAnimEmitCount) - 1;
            store<u32>(a + kAnimEmitCount, m);
            store<u32>(e, load<u32>(a + kAnimEmitters + kEmitSize * m));
            m = load<u32>(a + kAnimEmitCount);
            store<u32>(e + kEmitKey, load<u32>(a + kAnimEmitters + kEmitSize * m + kEmitKey));
            store<u32>(a + kAnimEmitters + kEmitSize * m, 0);
            m = load<u32>(a + kAnimEmitCount);
            store<u32>(a + kAnimEmitters + kEmitSize * m + kEmitKey, 0);
            n = static_cast<s32>(load<u32>(a + kAnimEmitCount));
            --j;  // re-check the emitter moved into j
        }
    }
    const u32 s = load<u32>(a + kAnimSet);
    if (s != 0) {
        const u32 x = static_cast<u32>(call(ctx, kAnimFind34CC34, {s, key}));
        if (x != 0) call(ctx, kEntryStop353CC0, {x, 0});
    }
    call(ctx, kAnimStop254564, {a, key});
    for (u32 k = 0; k < kAnimNumLayers; ++k) {
        if (load<u32>(a + kAnimLayers + kAnimLayerSize * k) != key) continue;
        const u32 l = a + kAnimLayers + kAnimLayerSize * k;
        if (l != 0) {  // the original tests the (wrapped) layer address
            store<u32>(l + kLayerPtr, 0);
            store<u32>(l + kLayer28, 0);
        }
        return ret(ctx, a + kAnimLayerSize * k);
    }
    ret(ctx, kAnimNumLayers);
}

// Speaker* Speaker_Say3514AC(Speaker* s, Text* t, u8 b, -, -, -, u32 r9,
// u32 r10, u32 voice; f32, f32, f32) (name?): with no line yet (+8), looks
// the text's name hash up in the line map (reporting a missing one through
// the owner's vt +12) and sets it; picks the first speaker of the list
// (+92) or 0x0035133C's; says the text with 0x00350958 and, while it plays,
// hands the voice (looked up when non-zero) to the track's last item.
// Arguments 4..6 are not read (nor passed on: 0x00350958 ignores r6..r8).
DECOMP(003514AC, Speaker_Say3514AC) {
    u32 s = arg<u32>(ctx, 0);
    const u32 t = arg<u32>(ctx, 1);
    const u8 b = arg<u8>(ctx, 2);
    const u32 r9 = arg<u32>(ctx, 6), r10 = arg<u32>(ctx, 7);
    const u32 voice = stack_arg(static_cast<u32>(ctx.gpr[1]), 8);
    const f64 f1 = farg(ctx, 0), f2 = farg(ctx, 1), f3 = farg(ctx, 2);
    Stack frame(ctx, 16);  // the key words (r1 +112 / +116 in the original frame)
    const u32 key = frame.addr();
    if (load<u32>(s + kSpkLine) == 0) {
        const u32 h = static_cast<u32>(call(ctx, fn::String_HashNoCase, {load<u32>(t + kTextName) & ~1u}));
        store<u32>(key + 4, h);
        const u32 line = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind76D894, {kLineMap, key + 4})));
        if (h != 0 && line == 0) {
            call(ctx, kHashToString, {h});  // the name is looked up but unused
            const u32 o = load<u32>(s + kSpkOwner);
            if (o != 0) vcall(ctx, o, kOwnerVtMissing, {o});
        }
        call(ctx, kSpkSetLine34F690, {s, line, 0});
    }
    const u32 l = load<u32>(s + kSpkList);
    if (l == 0 || load<u16>(l + 8) == 0) s = static_cast<u32>(call(ctx, kSpkFirst35133C, {s, 0}));
    else s = load<u32>(load<u32>(l));
    // Only r3-r5, r9, r10 and f1-f3 are read by the callee (r6..r8 hold
    // leftovers in the original).
    callf(ctx, kSpkSay350958, {s, t, b, 0, 0, 0, r9, r10}, {f1, f2, f3});
    if (load<u32>(s + kSpkPlaying) != 0) {
        u32 v = 0;
        if (voice != 0) {
            store<u32>(key, voice);
            v = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind76DD98, {kVoiceMap, key})));
        }
        if (voice == 0 || v != 0) {
            const u32 tr = load<u32>(s + kSpkTrack);
            const s32 n = static_cast<s32>(load<u32>(tr + kTrackCount));
            if (n > 0) {
                const u32 it = load<u32>(load<u32>(tr + kTrackItems) + 4 * static_cast<u32>(n - 1));
                if (it != 0) call(ctx, kItemVoiceA4CC64, {it, v});
            }
        }
    }
    ret(ctx, s);
}

// u32 Query_Init0CDD28(Query* q, u32 kind, u32 a16, u32 a28, Def* def, u32
// a36, u32 a40, const f32 m[4][4], const vec3* pos, u8 x9 .. x16, u32 x17,
// u8 x18, -, -, u32 x21; f32, f32) (name?): fills a query from 22 arguments
// (arguments 19 / 20 are not read). A def overrides flag bit 14 (def +32
// bit 30) and both floats (+36 / +40) and adds its templates (+136) to the
// query's vector. Returns q + 4, 0 or the template count.
DECOMP(000CDD28, Query_Init0CDD28) {
    const u32 q = arg<u32>(ctx, 0), kind = arg<u32>(ctx, 1), a16 = arg<u32>(ctx, 2), a28 = arg<u32>(ctx, 3);
    const u32 def = arg<u32>(ctx, 4), a36 = arg<u32>(ctx, 5), a40 = arg<u32>(ctx, 6), m = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 pos = stack_arg(sp, 8);
    u32 bits = (load<u32>(q + kQBits) & 0x1FFFFu) | ((kind << 17) & 0xFFFE0000u);
    bits = (bits & ~0x10000u) | ((stack_arg(sp, 9) & 1) << 16);
    bits = (bits & ~0x8000u) | ((stack_arg(sp, 10) & 1) << 15);
    u32 b14 = stack_arg(sp, 11) & 0xFF;
    const u32 b13 = stack_arg(sp, 12), b12 = stack_arg(sp, 13), b9 = stack_arg(sp, 14);
    const u32 b8 = stack_arg(sp, 15), b6 = stack_arg(sp, 16), a128 = stack_arg(sp, 17);
    const u32 a24 = stack_arg(sp, 18), a140 = stack_arg(sp, 21);
    store<u32>(q + kQVec, 0);
    store<u32>(q + 16, a16);
    if (def != 0) {
        store<u32>(q + kQBits, bits);  // the partial word is stored first
        b14 = (load<u32>(def + kDefFlags32) & kDefFlagBit) != 0 ? 1u : 0u;
    }
    bits = (bits & ~0x4000u) | ((b14 & 1) << 14);
    store<u8>(q + kQByte, static_cast<u8>(a24));
    bits = (bits & ~0x2000u) | ((b13 & 1) << 13);
    bits = (bits & ~0x1000u) | ((b12 & 1) << 12);
    bits &= ~0xC00u;
    bits = (bits & ~0x200u) | ((b9 & 1) << 9);
    bits = (bits & ~0x100u) | ((b8 & 1) << 8);
    bits &= ~0x80u;
    bits = (bits & ~0x40u) | ((b6 & 1) << 6);
    store<u32>(q + kQBits, bits);
    // lfs / stfs pairs, in the original order.
    for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u}) store<u32>(q + kQMat + off, load<u32>(m + off));
    store<u32>(q + 28, a28);
    store<u32>(q + kQMat + 40, load<u32>(m + 40));
    store<u32>(q + kQDef, def);
    store<u32>(q + kQMat + 48, load<u32>(m + 48));
    store<u32>(q + 36, a36);
    store<u32>(q + 40, a40);
    store<u32>(q + kQMat + 52, load<u32>(m + 52));
    store<u32>(q + kQMat + 56, load<u32>(m + 56));
    store<u32>(q + kQPos, load<u32>(pos));
    store<u32>(q + kQ128, a128);
    store<u32>(q + kQPos + 4, load<u32>(pos + 4));
    store<u32>(q + kQPos + 8, load<u32>(pos + 8));
    ctx.fpr[3] = hevent7::lfs_f64(pos + 8);  // f3 left holding the last single (no known caller reads it)
    // stfs of the incoming f1 / f2 (rounded to single) or the def's words.
    if (def != 0) store<u32>(q + kQF132, load<u32>(def + kDefF36));
    else store<f32>(q + kQF132, static_cast<f32>(farg(ctx, 0)));
    if (def != 0) {
        store<u32>(q + kQF136, load<u32>(def + kDefF40));
        ctx.fpr[1] = hevent7::lfs_f64(def + kDefF40);
    } else {
        store<f32>(q + kQF136, static_cast<f32>(farg(ctx, 1)));
        ctx.fpr[1] = farg(ctx, 1);
    }
    store<u32>(q + kQ140, a140);
    for (u32 k = 4; k < 16; k += 4) store<u32>(q + k, 0);
    if (def == 0) return ret(ctx, q + 4);
    u32 v = load<u32>(def + kDefTemplates);
    if (v == 0) return ret(ctx, q + 4);
    if (load<u16>(v + 8) == 0) return ret(ctx, 0u);
    for (u32 k = 0;; ++k) {
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (k >= n) return ret(ctx, n);
        const u32 tmpl = load<u32>(v) + kTemplateSize * k;
        u32 vec = load<u32>(q + kQVec);
        if (vec == 0) {
            vec = static_cast<u32>(call(ctx, h17fn::SmallAlloc, {20}));
            store<u32>(q + kQVec, vec);
            if (vec != 0) {
                call(ctx, h17fn::PtrVector_Construct, {vec});
                store<u16>(vec + 6, static_cast<u16>(kTemplateSize));
                vec = load<u32>(q + kQVec);
            }
        }
        call(ctx, kVecAdd733964, {vec, tmpl});
        v = load<u32>(def + kDefTemplates);
    }
}
