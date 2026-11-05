// World_HandleEvent callees, part 29a: three pieces of the geometry scene
// load (Geom_LoadScene4F05A8): the scene's view settings, its xml paths
// and its banks.
#include "hevent29.h"

using namespace sly4;
using namespace sly4::hevent27;
using namespace sly4::hevent28;
using namespace sly4::hevent29;

namespace {
// Scene definition (as hevent28_a.cpp).
constexpr u32 kSceneBanks = 296;     // Entry* (+0 name, +16 f32[3]), u16 count at +304
constexpr u32 kSceneXml = 316;       // const char* xml paths, u16 count at +324
constexpr u32 kSceneHasView = 600;   // u8: apply the view settings below
constexpr u32 kSceneFov = 604;       // f32
constexpr u32 kSceneF338 = 608;      // f32 (View_SetF338)
constexpr u32 kSceneF2C8 = 612;      // f32 (View_SetF2C8)
constexpr u32 kSceneColor = 616;     // u8[4], copied to (view +512) +32
constexpr u32 kViewSettings = 512;   // View*: settings block pointer (name?)
constexpr u32 kSettingsColor = 32;
constexpr u32 kBankPos = 0x011944E0;  // f32[3]: position handed to Bank_Open (name?)
}  // namespace

// u32 Scene_ApplyView4F0394(-, Scene* scene, View* view, bool force)
// (name?): when force or the scene's +600 flag: fov, the +608 / +612
// values and the four colour bytes (+616) set on the view. r3 is the
// colour's destination (view settings +32), or 0 when nothing was set
// (the flag byte read last).
DECOMP(004F0394, Scene_ApplyView4F0394) {
    const u32 scene = arg<u32>(ctx, 1), view = arg<u32>(ctx, 2);
    const u8 force = arg<u8>(ctx, 3);
    if (force == 0 && load<u8>(scene + kSceneHasView) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    callf(ctx, h29fn::View_SetFov, {view}, {lfs_at(scene + kSceneFov)});
    callf(ctx, h29fn::View_SetF338, {view}, {lfs_at(scene + kSceneF338)});
    callf(ctx, h29fn::View_SetF2C8, {view}, {lfs_at(scene + kSceneF2C8)});
    const u32 dst = load<u32>(view + kViewSettings) + kSettingsColor;
    for (u32 k = 0; k < 4; ++k) store<u8>(dst + k, load<u8>(scene + kSceneColor + k));
    ctx.gpr[3] = dst;
}

// u16 Scene_AddXmlPaths4F0448(-, Scene* scene) (name?): every xml path of
// the scene (+316, u16 count +324) added to set 0. r3 is the count read
// last (the loop's exit test).
DECOMP(004F0448, Scene_AddXmlPaths4F0448) {
    const u32 scene = arg<u32>(ctx, 1);
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(scene + kSceneXml + 8)); ++i) {
        const u32 path = load<u32>(load<u32>(scene + kSceneXml) + 4 * i);
        const u32 set = static_cast<u32>(call(ctx, h28fn::Set_Find, {0}));
        call(ctx, h28fn::Set_AddXmlPath, {set, path, 0});
    }
    ctx.gpr[3] = n;
}

// u16 Scene_OpenBanks4F04D4(GeomSet* set, Scene* scene) (name?): every
// bank entry of the scene (+296, u16 count +304) opened in the set's bank
// (+0), its position (+16) first copied to the global Bank_Open reads.
// r3 is the count read last.
DECOMP(004F04D4, Scene_OpenBanks4F04D4) {
    const u32 set = arg<u32>(ctx, 0), scene = arg<u32>(ctx, 1);
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(scene + kSceneBanks + 8)); ++i) {
        const u32 e = load<u32>(load<u32>(scene + kSceneBanks) + 4 * i);
        const u32 bank = load<u32>(set);
        const u32 name = load<u32>(e);  // read before the position is copied
        for (u32 k = 0; k < 12; k += 4) store<u32>(kBankPos + k, load<u32>(e + 16 + k));
        ctx.fpr[1] = lfs_at(e + 24);  // f1 as the copy leaves it
        call(ctx, h28fn::Bank_Open, {bank, name, 0});
    }
    ctx.gpr[3] = n;
}
