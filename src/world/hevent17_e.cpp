// World_HandleEvent callees, part 17e: the player's per-level look.
#include "hevent17.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent17;

namespace {
// Per-level model swaps at 0x00AC8C28: 4 records of 16 bytes {+0 save flag
// index, +4 level hash, +8 model hash (flag set), +12 model hash (clear)};
// the save flag is bit 1 of save slot +4032 + index.
constexpr u32 kLookTable = 0x00AC8C28, kLookSize = 16, kNumLooks = 4;
constexpr u32 kLookIndex = 0, kLookLevel = 4, kLookModels = 8;
constexpr u32 kSaveFlags = 4032, kSaveFlagOn = 0x2;
// Player: +216 def (+192 level hash), +160 WorldObj* (+208 Model* (+100
// u32 hash); vt +44 set_model(wo, geom), vt +64 fade(wo; f32)); +172 u32
// flags (bit 1: busy), +424 u32, +968 u8 (bit 0: locked), +184 u64 flags
// (bit 36: the word at +184 bit 4), +432 u32, +340 attach vector.
constexpr u32 kPlDef = 216, kDefLevel = 192, kPlWorldObj = 160, kWoModel = 208, kModelHash = 100;
constexpr u32 kWoVtSetModel = 44, kWoVtFade = 64;
constexpr u32 kPlFlags172 = 172, kPlBusy = 0x2, kPl424 = 424, kPlLock = 968, kPlFlags184 = 184, kPl432 = 432;
constexpr u32 kPlFlag184Bit4 = 0x10;
// Player vt +32 state(p, u8), vt +272 current effect.
constexpr u32 kPlVtState = 32, kPlVtEffect = 272;
// Geometry definitions (as hevent17_c.cpp).
constexpr u32 kGeomDefs = 0x00F8F12C, kGeomMap = 16;
constexpr u32 kHashMapFind2 = 0x007399A4;
constexpr u32 kObjOwner = 0x000D93FC;       // Obj_Owner0D93FC (Object*) -> owner (symbols.txt)
constexpr u32 kObjEffectDCD20 = 0x000DCD20; // (Object*, u32 hash, 0, 1) (name?)
constexpr u32 kSwapFx = 0x05220E90;
constexpr u32 kCompType16 = 16;
constexpr u32 kCompLook287318 = 0x00287318; // (Component*, u8 flag) (name?)
constexpr u32 kFlag48FC68 = 0x0048FC68;      // (u32 hash) -> u32 (name?)
constexpr u32 kFadeFlag = 0xF11D508E;
constexpr u32 kConsts = 0x0011F7A8;          // f32 20.0, f32 0.0, u64 1 << 36
// Level effects.
constexpr u32 kLevelEffect = 0x88397820, kLevelOutfit = 0x00014120;
constexpr u32 kSaveEffect = 296, kDefaultEffect = 0x86DF6221;
constexpr u32 kEffectRelease = 0x004CF3C4;   // Effect_Release (Effect*, u32 hash) (symbols.txt)
constexpr u32 kEffName = 4, kEffNameHash = 8;
// Outfit: the attach record whose +160 is 0x03D69F9C, its object's (+192)
// def (+216) +52 hash vs save slot +292 (default 0xFE11127C).
constexpr u32 kAttachTag = 160, kOutfitTag = 0x03D69F9C, kDefHash = 52;
constexpr u32 kSaveOutfit = 292, kDefaultOutfit = 0xFE11127C;
constexpr u32 kSetOutfit11F32C = 0x0011F32C;  // (Player*, u32 hash) (name?)
constexpr u32 kPlSettle11F0E4 = 0x0011F0E4;   // (Player*, -, 1; f32) (name?)
}  // namespace

// u32 Player_UpdateLook11F7B8(Player* p, -, u8 force) (name?; was
// Player_Settle11F7B8): for the levels in the look table, swaps the player's
// model between its two variants when the save flag disagrees with the one
// shown (unless busy, mid-state, owned by an active owner or not forced),
// with the swap effect, component-16 updates and an optional fade; then,
// unless locked (+968 bit 0), applies the save slot's level effect (level
// 0x88397820) or outfit (level 0x00014120).
DECOMP(0011F7B8, Player_UpdateLook11F7B8) {
    const u32 p = arg<u32>(ctx, 0);
    const u8 force = arg<u8>(ctx, 2);
    const u32 d = load<u32>(p + kPlDef);
    const u32 lv = load<u32>(d + kDefLevel);
    if (lv == 0) return ret(ctx, 0u);
    const u32 w = load<u32>(kCurrentWorld);
    const u32 save = w + load<u8>(w + kUserBlocks) * kSaveSlotSize + kSaveData;
    const u32 wo = load<u32>(p + kPlWorldObj);
    const u32 flags = load<u32>(p + kPlFlags172);
    for (u32 k = 0; k < kNumLooks; ++k) {
        const u32 rec = kLookTable + kLookSize * k;
        if (load<u32>(rec + kLookLevel) != lv) continue;
        const u32 on = (load<u8>(save + load<u32>(rec + kLookIndex) + kSaveFlags) & kSaveFlagOn) != 0 ? 1u : 0u;
        if (wo == 0) break;
        if (load<u32>(load<u32>(wo + kWoModel) + kModelHash) != load<u32>(rec + kLookModels + (on ? 0u : 4u))) break;
        if (force == 0) {
            if ((flags & kPlBusy) != 0) break;
            if (static_cast<u32>(vcall(ctx, p, kPlVtState, {p, 0})) != 0) break;
            if (static_cast<u32>(vcall(ctx, p, kPlVtState, {p, 1})) != 0 &&
                static_cast<s32>(vcall(ctx, p, kPlVtState, {p, 1})) != 2)
                break;
            if (load<u32>(p + kPl424) != 0) break;
            const u32 o = static_cast<u32>(call(ctx, kObjOwner, {p}));
            if (o != 0 && (load<u32>(o + 40) & 0x80000000u) != 0) break;
        }
        Stack frame(ctx, 16);  // the key word (r1 +112)
        store<u32>(frame.addr(), load<u32>(rec + kLookModels + 4 * on));
        const u32 slot = static_cast<u32>(call(ctx, kHashMapFind2, {load<u32>(load<u32>(kGeomDefs) + kGeomMap), frame.addr()}));
        const u32 g = load<u32>(slot);
        if (g == 0) break;
        if (force == 0 && (load<u32>(p + kPlFlags172) & kPlBusy) == 0) call(ctx, kObjEffectDCD20, {p, kSwapFx, 0, 1});
        const u32 wo2 = load<u32>(p + kPlWorldObj);
        vcall(ctx, wo2, kWoVtSetModel, {wo2, g});
        for (u32 c = static_cast<u32>(call(ctx, hevent16::h16fn::Obj_FindComponent, {p, kCompType16, 0})); c != 0;
             c = static_cast<u32>(call(ctx, hevent16::h16fn::Obj_FindComponent, {p, kCompType16, c})))
            call(ctx, kCompLook287318, {c, on});
        if (static_cast<u32>(call(ctx, kFlag48FC68, {kFadeFlag})) != 0) {
            const u32 wo3 = load<u32>(p + kPlWorldObj);
            ctx.fpr[1] = hevent7::lfs_f64(kConsts);  // f1 = 20.0
            vcall(ctx, wo3, kWoVtFade, {wo3});
        }
        break;
    }
    if ((load<u8>(p + kPlLock) & 1) != 0) return ret(ctx, 1u);
    const u32 lv2 = load<u32>(d + kDefLevel);
    if (lv2 == kLevelEffect) {
        const u32 h0 = load<u32>(save + kSaveEffect);
        const u32 h = h0 != 0 ? h0 : kDefaultEffect;
        const u32 f = static_cast<u32>(vcall(ctx, p, kPlVtEffect, {p}));
        if (f == 0) return ret(ctx, 0u);
        const u32 n = load<u32>(f + kEffName);
        if ((n != 0 ? load<u32>(n + kEffNameHash) : 0u) == h) return ret(ctx, n);
        call(ctx, kEffectRelease, {f, h});
        return;
    }
    if (lv2 != kLevelOutfit) {
        // r3 holds the last compare constant (lis sign-extends).
        if (static_cast<s32>(lv2) > static_cast<s32>(kLevelEffect)) return ret(ctx, kLevelOutfit);
        return ret(ctx, static_cast<s32>(kLevelEffect - 1));
    }
    const u32 v = load<u32>(p + hevent10::kObjAttach);
    u32 obj = 0;
    for (u32 i = 0; i < (v != 0 ? load<u16>(v + hevent10::kAttachCount) : 0u); ++i) {
        const u32 r = load<u32>(v) + hevent10::kAttachRecSize * i;
        if (load<u32>(r + kAttachTag) == kOutfitTag) {
            obj = load<u32>(r + hevent10::kAttachRecObject);
            break;
        }
    }
    u32 cur = 0;
    if (obj != 0) {
        const u32 od = load<u32>(obj + kPlDef);
        if (od != 0) cur = load<u32>(od + kDefHash);
    }
    const u32 h0 = load<u32>(save + kSaveOutfit);
    const u32 h = h0 != 0 ? h0 : kDefaultOutfit;
    if (cur != h) call(ctx, kSetOutfit11F32C, {p, h});
    const u64 fl = load<u64>(p + kPlFlags184);
    if (h == kDefaultOutfit) {
        store<u64>(p + kPlFlags184, fl & ~(1ull << 36));
        return ret(ctx, 0u);
    }
    const u64 nf = fl | load<u64>(kConsts + 8);
    const u32 st = load<u32>(p + kPl432);
    store<u64>(p + kPlFlags184, nf);
    if (static_cast<s32>(st) != 1) {
        ctx.gpr[3] = nf;  // r3 is the or'd doubleword
        return;
    }
    store<u32>(p + kPl432, 0);
    callf(ctx, kPlSettle11F0E4, {p, kConsts, 1}, {hevent7::lfs_f64(kConsts + 4)});
}
