// World virtuals: GetState (+32), the deleting destructor (+4) and Init
// (+8, called once by Game_Load).
#include "vt_world.h"

using namespace sly4;
using namespace sly4::world;

namespace {

// Virtual call. The original's call sequence leaves the OPD and its entry
// in the two registers after the arguments; we do the same.
u64 vcall(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    ctx.gpr[3 + args.size()] = opd;
    ctx.gpr[4 + args.size()] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}

constexpr u32 kMemAlloc = 0x0054ED38, kMemFreeFrom = 0x0054EE6C;
constexpr u32 kUiFind = 0x004D5544, kUiSetInputHandler = 0x004D5048;
constexpr u32 kAudioSetPaused = 0x0044BA6C, kStringHashNoCase = 0x0056A710;

// World_Init's callees (names are guesses from their arguments).
constexpr u32 kWorldInitSystems = 0x00467E68;   // (World*) (name?)
constexpr u32 kTrophyConstruct = 0x004FB0F0;    // (mem, comm id, sig, opd, 128, 0) (name?)
constexpr u32 kSub354F60 = 0x00354F60;          // () (name?)
constexpr u32 kSubADCF8 = 0x000ADCF8;           // (World + 4144) (name?)
constexpr u32 kPadNotify = 0x0044BCDC;          // (u32 mask, 1): an empty function
constexpr u32 kWorldSetLevel = 0x004678B8;      // (World*, 0xFFFFFFFF, 0) (name?)
constexpr u32 kSubAE4A4 = 0x000AE4A4;           // () (name?)
constexpr u32 kLoadUpgrades = 0x0049DC70;       // ("Scripts/Upgrades.xml") (name?)
constexpr u32 kLoadAwards = 0x000B2280;         // ("Scripts/Awards.xml") (name?)
constexpr u32 kLoadLevels = 0x004EBE50;         // ("Scripts/Sly4Levels.xml") (name?)
constexpr u32 kLevelCount = 0x004ECAC4;         // () -> u32 (name?)
constexpr u32 kLevelAt = 0x004ECC50;            // (u32 i) -> Level* (name?)
constexpr u32 kStrNCmp = 0x00555EB0;            // (a, b, n) -> 0 if equal (name?)
constexpr u32 kLevelRegister = 0x004EBCF4;      // (Level*) (name?)
constexpr u32 kSub448C10 = 0x00448C10;          // () (name?)
constexpr u32 kSub44BCE0 = 0x0044BCE0;          // () -> u32 (name?)
constexpr u32 kSub4AE458 = 0x004AE458;          // () (name?)
constexpr u32 kSessionNotify = 0x002DD890;      // (Session*): an empty function
constexpr u32 kDefaultName = 0x00324A48;        // (0x010B83C4) -> u32 (name?)
constexpr u32 kSessionStartA = 0x002E142C;      // (Session*, u32) (name?)
constexpr u32 kSessionStartB = 0x002E1DB0;      // (Session*, u32, u32, bool, bool, 0, 0, 0) (name?)
constexpr u32 kWorldStartLevel = 0x004674C4;    // (World*, u32 hash, 0) (name?)
constexpr u32 kWorldInitDone = 0x00467050;      // (World*) (name?)

// Strings and globals.
constexpr u32 kTrophyCommId = 0x00AE6AC0;       // "NPWR03581"
constexpr u32 kTrophySig = 0x00AE6B4C;
constexpr u32 kTrophyOpd = 0x00F39E78;
constexpr u32 kCommonFade = 0x00AE825C;         // "Common.Fade"
constexpr u32 kUpgradesXml = 0x00AE875C, kAwardsXml = 0x00AE8774, kLevelsXml = 0x00AE878C;
constexpr u32 kDevPrefix = 0x00AE87A4;          // "Dev/"
constexpr u32 kInputHandlerOpd = 0x00F39CC0;
constexpr u32 kCallbackA = 0x00F66894, kCallbackAOpd = 0x00F39D48;  // (name?)
constexpr u32 kCallbackB = 0x00F66898, kCallbackBOpd = 0x00F39D50;  // (name?)
constexpr u32 kFadeUi = 0x00F65C88;             // u32 Ui* of "Common.Fade"
constexpr u32 kFadeVfnSet = 88;                 // (ui, 0) (name?)
constexpr u32 kInitResult = 0x00F66844;         // u32 (name?)
constexpr u32 kSession = 0x00F64C20;            // u32 Session* (name?)
constexpr u32 kPlayers = 0x010B19E0, kPlayerStride = 11968, kPlayerCount = 2;
constexpr u32 kPlayerPadPtr = 176 + 28;         // u32 Pad*; pad +4 u16 bit 15 = connected (name?)
// Start-up parameters (the first player's block doubles as them) (name?)
constexpr u32 kStartHash = 12, kStartLevel = 16, kStartName = 20, kStartArg = 24;
constexpr u32 kStartFlagA = 28, kStartFlagB = 29;
constexpr u32 kDefaultHashString = 0x010B82B0;
constexpr u32 kTuneStartName = 0x010B8330;      // tuning objects, value at +72
constexpr u32 kDefaultArgSource = 0x010B83C4;
constexpr u32 kTuneFlagA = 0x010B8490, kTuneFlagB = 0x010B8444, kTuneStartMode = 0x010B84DC;
constexpr u32 kTuning = 72;

}  // namespace

// u8 World_GetState(World* w, u32 level): level 0 from +48, others from ids[].
DECOMP(0046B5DC, World_GetState) {
    const u32 w = arg<u32>(ctx, 0), level = arg<u32>(ctx, 1);
    if (level == 0) return ret(ctx, static_cast<u32>(ref<u8>(w + kWorldState0)));
    ret(ctx, static_cast<u32>(ref<u8>(load<u32>(w + kSmIds) + level)));
}

// void World_Delete(World* w, u32 flags): destruct and free (flags unused).
DECOMP(00467004, World_Delete) {
    const u32 w = arg<u32>(ctx, 0);
    if (w == 0) return;
    call(ctx, vfn::World_Destruct, {w});
    call(ctx, kMemFreeFrom, {w, 0});
}

// void World_Init(World* w)
// Brings up the game systems: trophies, input handler and callbacks, the
// fade UI, upgrades/awards/levels scripts (registering every level whose
// path does not start with "Dev/"), the audio menu mode; enters the
// current level-0 state, then starts the session from the start-up
// parameters (a level, a name, or a hashed start point).
DECOMP(004682C8, World_Init) {
    const u32 w = arg<u32>(ctx, 0);
    call(ctx, kWorldInitSystems, {w});
    const u32 mem = static_cast<u32>(call(ctx, kMemAlloc, {1400, 0, 16}));
    if (mem != 0) call(ctx, kTrophyConstruct, {mem, kTrophyCommId, kTrophySig, kTrophyOpd, 128, 0});
    call(ctx, kSub354F60);
    call(ctx, kSubADCF8, {w + 4144});
    for (u32 i = 0; i < kPlayerCount; ++i) {
        const u32 pad = load<u32>(kPlayers + i * kPlayerStride + kPlayerPadPtr);
        if (pad != 0 && ((load<u16>(pad + 4) >> 15) & 1) != 0) call(ctx, kPadNotify, {1u << i, 1});
    }
    call(ctx, kUiSetInputHandler, {kInputHandlerOpd, w});
    store<u32>(kCallbackA, kCallbackAOpd);
    store<u32>(kCallbackB, kCallbackBOpd);
    call(ctx, kWorldSetLevel, {w, 0xFFFFFFFFu, 0});
    const u32 fade = static_cast<u32>(call(ctx, kUiFind, {kCommonFade, 0}));
    store<u32>(kFadeUi, fade);
    if (fade != 0) vcall(ctx, fade, kFadeVfnSet, {fade, 0});

    call(ctx, kSubAE4A4);
    call(ctx, kLoadUpgrades, {kUpgradesXml});
    call(ctx, kLoadAwards, {kAwardsXml});
    call(ctx, kLoadLevels, {kLevelsXml});
    const u32 levels = static_cast<u32>(call(ctx, kLevelCount));
    for (u32 i = 0; i < levels; ++i) {
        const u32 level = static_cast<u32>(call(ctx, kLevelAt, {i}));
        if (level == 0 || level + 128 == 0) continue;
        if (static_cast<u32>(call(ctx, kStrNCmp, {level + 128, kDevPrefix, 4})) != 0)
            call(ctx, kLevelRegister, {level});
    }
    call(ctx, kSub448C10);
    call(ctx, kAudioSetPaused, {1});
    store<u32>(kInitResult, static_cast<u32>(call(ctx, kSub44BCE0)));
    call(ctx, kSub4AE458);

    // Enter the level-0 state the constructor chose.
    const u32 ids = load<u32>(w + kSmIds);
    if (ids != 0 && ref<u8>(ids) != kSmNone) {
        Stack frame(ctx, 16);
        const u32 ev = frame.addr();
        store<u32>(ev, kEventEnter);
        store<u32>(ev + 4, 0);
        store<u32>(ev + 8, kSmNone);
        store<u32>(ev + 12, ref<u8>(ids));
        vcall(ctx, w, kVfnEvent, {w, ev});
    }

    // Start the session.
    call(ctx, kSessionNotify, {load<u32>(kSession)});
    const u32 p = kPlayers;
    u32 hash = load<u32>(p + kStartHash);
    if (hash == 0) hash = static_cast<u32>(call(ctx, kStringHashNoCase, {kDefaultHashString}));
    u32 name = load<u32>(p + kStartName);
    const u32 start_level = load<u32>(p + kStartLevel);
    if (name == 0) name = load<u32>(kTuneStartName + kTuning);
    u32 value = load<u32>(p + kStartArg);
    if (value == 0) value = static_cast<u32>(call(ctx, kDefaultName, {kDefaultArgSource}));
    const u32 flag_a = (ref<u8>(p + kStartFlagA) != 0 || ref<u8>(kTuneFlagA + kTuning) != 0) ? 1 : 0;
    const u32 flag_b = (ref<u8>(p + kStartFlagB) != 0 || ref<u8>(kTuneFlagB + kTuning) != 0) ? 1 : 0;

    if (start_level != 0) {
        call(ctx, kSessionStartA, {load<u32>(kSession), start_level});
    } else if (name != 0) {
        call(ctx, kSessionStartB, {load<u32>(kSession), name, value, flag_b, flag_a, 0, 0, 0});
    } else if (hash != 0) {
        const s32 mode = load<s32>(kTuneStartMode + kTuning);
        if (mode >= 0) {
            const u8 b = ref<u8>(w + 28);
            ref<u8>(w + 28) = static_cast<u8>((b & 0x3F) | ((static_cast<u32>(mode) << 6) & 0xC0));
        }
        call(ctx, kWorldStartLevel, {w, hash, 0});
    }
    call(ctx, kWorldInitDone, {w});
}
