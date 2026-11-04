// Game_BootCheck: the cellGame start-up checks (boot, patch and content
// permission) with their debug printout.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace boot {

namespace bimp {
constexpr u32 cellGameGetParamString = 0x00ABE4C4;  // (s32 id, char* buf, u32 size)
constexpr u32 cellGameContentPermit = 0x00ABE4E4;   // (char contentInfoPath[128], char usrdirPath[128])
constexpr u32 cellGameGetParamInt = 0x00ABE524;     // (s32 id, s32* value)
constexpr u32 cellGamePatchCheck = 0x00ABE544;      // (CellGameContentSize*, void* reserved)
constexpr u32 cellGameBootCheck = 0x00ABE584;       // (u32* type, u32* attributes, CellGameContentSize*, char dirName[32])
}  // namespace bimp

namespace bfn {
constexpr u32 printf = 0x007240F4;
constexpr u32 memmove = 0x0071C3F0;
}  // namespace bfn

// CellGameContentSize
struct GameContentSize {
    be_s32 hddFreeSizeKB;
    be_s32 sizeKB;
    be_s32 sysSizeKB;
};
static_assert(sizeof(GameContentSize) == 12);

enum : u32 {
    CELL_GAME_GAMETYPE_DISC = 1,
    CELL_GAME_GAMETYPE_HDD = 2,
};
enum : u32 {
    CELL_GAME_ATTRIBUTE_PATCH = 1u << 0,
    CELL_GAME_ATTRIBUTE_APP_HOME = 1u << 1,
    CELL_GAME_ATTRIBUTE_DEBUG = 1u << 2,
    CELL_GAME_ATTRIBUTE_XMBBUY = 1u << 3,
    CELL_GAME_ATTRIBUTE_COMMERCE2_BROWSER = 1u << 4,
    CELL_GAME_ATTRIBUTE_INVITE_MESSAGE = 1u << 5,
    CELL_GAME_ATTRIBUTE_CUSTOM_DATA_MESSAGE = 1u << 6,
    CELL_GAME_ATTRIBUTE_WEB_BROWSER = 1u << 8,
};
enum : s32 {
    CELL_GAME_PARAMID_TITLE = 0,
    CELL_GAME_PARAMID_TITLE_DEFAULT = 1,
    CELL_GAME_PARAMID_TITLE_ID = 100,
    CELL_GAME_PARAMID_PARENTAL_LEVEL = 102,
    CELL_GAME_PARAMID_RESOLUTION = 103,
    CELL_GAME_PARAMID_SOUND_FORMAT = 104,
};
constexpr u32 kParamStringSize = 128;

// Globals.
constexpr u32 kHddFreeSizeKB = 0x00F88768;   // s32 from cellGameBootCheck
constexpr u32 kAllStringIds = 0x00F8877C;    // s32[20]: param ids 0..17, 100, 101
constexpr u32 kAllStringNames = 0x00F887CC;  // const char*[20]
constexpr u32 kAllStringCount = 20;
// The first ContentPermit fills these; the second (after a patch check) the next pair.
constexpr u32 kContentInfoPath = 0x0116B358, kUsrdirPath = 0x0116B3D8;    // char[128] each
constexpr u32 kContentInfoPath2 = 0x0116B458, kUsrdirPath2 = 0x0116B4D8;  // char[128] each

// Strings.
namespace bstr {
constexpr u32 kBootError = 0x00AF3E84;     // "cellGameBootCheck() Error: 0x%x\n"
constexpr u32 kBootOk = 0x00AF3EAC;        // "cellGameBootCheck() OK\n"
constexpr u32 kGetType = 0x00AF3EC4;       // "  get_type = [%d]\n"
constexpr u32 kTypeDisc = 0x00AF3EDC;      // "    CELL_GAME_GAMETYPE_DISC\n"
constexpr u32 kTypeHdd = 0x00AF3EFC;       // "    CELL_GAME_GAMETYPE_HDD\n"
constexpr u32 kAttributes = 0x00AF3F1C;    // "  get_attributes = [0x%08x]\n"
constexpr u32 kAttrPatch = 0x00AF3F3C;
constexpr u32 kAttrAppHome = 0x00AF3F5C;
constexpr u32 kAttrDebug = 0x00AF3F84;
constexpr u32 kAttrXmbBuy = 0x00AF3FA4;
constexpr u32 kAttrCommerce2 = 0x00AF3FC4;
constexpr u32 kAttrInvite = 0x00AF3FF4;
constexpr u32 kAttrCustomData = 0x00AF401C;
constexpr u32 kAttrWebBrowser = 0x00AF404C;
constexpr u32 kDirName = 0x00AF4074;       // "  dirName = [%s]\n"
constexpr u32 kSizes = 0x00AF408C;         // "  hddFreeSizeKB = [%d] sizeKB = [%d] sysSizeKB = [%d]\n"
constexpr u32 kParentalLevel = 0x00AF3C50, kResolution = 0x00AF3C60, kSoundFormat = 0x00AF3C70;
constexpr u32 kParamIntError = 0x00AF3C80;  // "cellGameGetParamInt() Error id:[%s] : 0x%x\n"
constexpr u32 kParamIntOk = 0x00AF3CB0;     // "cellGameGetParamInt() OK id:[%s] value = [%d]\n"
constexpr u32 kTitleCurrent = 0x00AF3CE0, kTitleDefault = 0x00AF3CF0, kTitleId = 0x00AF3E08;
constexpr u32 kParamStrError = 0x00AF3E20;  // "cellGameGetParamString() Error id:[%s] : 0x%x\n"
constexpr u32 kParamStrOk = 0x00AF3E50;     // "cellGameGetParamString() OK id:[%s] string = [%s]\n"
constexpr u32 kPermitError = 0x00AF40C4;    // "cellGameContentPermit() Error: 0x%x\n"
constexpr u32 kPermitOk = 0x00AF40EC;       // "cellGameContentPermit() OK\n"
constexpr u32 kContentInfo = 0x00AF410C;    // "  contentInfoPath:[%s]\n"
constexpr u32 kUsrdir = 0x00AF4124;         // "  usrdirPath:[%s]\n"
constexpr u32 kPatchError = 0x00AF413C;     // "cellGamePatchCheck() Error: 0x%x\n"
constexpr u32 kPatchOk = 0x00AF4164;        // "cellGamePatchCheck() OK\n"
}  // namespace bstr

}  // namespace boot
}  // namespace sly4
