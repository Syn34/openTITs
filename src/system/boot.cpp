// Game_BootCheck (0x0056C8E4).
#include "opentit/native.h"
#include "boot.h"
#include "subsystems.h"

namespace sly4::system {
using namespace opentit::native;
using namespace sly4::boot;

namespace {

template <typename... A>
void Print(A... args) {
    call(bfn::printf, args...);
}

// The three PARAM.SFO integers, with the id and name tables the original
// builds on its stack each time (ids[3], then names[3]).
void Boot_PrintParamInts(be_s32* ids, be_s32* value) {
    be_u32* names = reinterpret_cast<be_u32*>(ids + 3);
    ids[0] = CELL_GAME_PARAMID_PARENTAL_LEVEL;
    ids[1] = CELL_GAME_PARAMID_RESOLUTION;
    ids[2] = CELL_GAME_PARAMID_SOUND_FORMAT;
    names[0] = bstr::kParentalLevel;
    names[1] = bstr::kResolution;
    names[2] = bstr::kSoundFormat;
    for (u32 i = 0; i < 3; ++i) {
        *value = 0;
        const s32 rc = call<s32>(bimp::cellGameGetParamInt, ids[i].get(), value);
        const u32 name = names[i];
        if (rc < 0) Print(bstr::kParamIntError, name, rc);
        else Print(bstr::kParamIntOk, name, value->get());
    }
}

// One PARAM.SFO string into a cleared 128-byte buffer.
void Boot_PrintParamString(s32 id, u32 name, char* buf) {
    for (u32 k = 0; k < kParamStringSize; ++k) buf[k] = 0;  // eight 16-byte vector stores
    const s32 rc = call<s32>(bimp::cellGameGetParamString, id, buf, kParamStringSize);
    if (rc < 0) Print(bstr::kParamStrError, name, rc);
    else Print(bstr::kParamStrOk, name, buf);
}

void Boot_PrintSizes(const GameContentSize* size) {
    Print(bstr::kSizes, size->hddFreeSizeKB.get(), size->sizeKB.get(), size->sysSizeKB.get());
}

// cellGameContentPermit with its printout; returns its result.
s32 Boot_ContentPermit(u32 info_path, u32 usrdir_path) {
    const s32 rc = call<s32>(bimp::cellGameContentPermit, info_path, usrdir_path);
    if (rc < 0) {
        Print(bstr::kPermitError, rc);
        return rc;
    }
    Print(bstr::kPermitOk);
    Print(bstr::kContentInfo, info_path);
    Print(bstr::kUsrdir, usrdir_path);
    return rc;
}

}  // namespace

// cellGameBootCheck, a printout of everything it and PARAM.SFO report,
// then cellGameContentPermit. A patched game (CELL_GAME_ATTRIBUTE_PATCH)
// also runs cellGamePatchCheck, prints all 20 PARAM.SFO strings and
// permits the content again into the second pair of paths. Returns the
// first failing call's error, or the last ContentPermit result.
s32 Game_BootCheck() {
    // The original's frame (offsets from its r1): the string buffers at +112
    // and +240, type and attributes at +368/+372, the int results at
    // +376/+380, the sizes at +384, the id/name tables from +396, dirName
    // at +632.
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(664 - kFrameBase);
    auto at = [&](u32 off) { return frame.as<u8>(off - kFrameBase); };
    char* buf2 = reinterpret_cast<char*>(at(112));
    char* buf1 = reinterpret_cast<char*>(at(240));
    be_u32* type = reinterpret_cast<be_u32*>(at(368));
    be_u32* attrs = reinterpret_cast<be_u32*>(at(372));
    be_s32* value2 = reinterpret_cast<be_s32*>(at(376));
    be_s32* value1 = reinterpret_cast<be_s32*>(at(380));
    GameContentSize* size = reinterpret_cast<GameContentSize*>(at(384));
    char* dir_name = reinterpret_cast<char*>(at(632));

    size->hddFreeSizeKB = 0;
    size->sizeKB = 0;
    size->sysSizeKB = 0;
    s32 rc = call<s32>(bimp::cellGameBootCheck, type, attrs, size, dir_name);
    if (rc < 0) {
        Print(bstr::kBootError, rc);
        return rc;
    }
    Print(bstr::kBootOk);
    Print(bstr::kGetType, type->get());
    if (*type == CELL_GAME_GAMETYPE_DISC) Print(bstr::kTypeDisc);
    if (*type == CELL_GAME_GAMETYPE_HDD) Print(bstr::kTypeHdd);

    Print(bstr::kAttributes, attrs->get());
    static constexpr struct {
        u32 bit, text;
    } kAttrs[] = {
        {CELL_GAME_ATTRIBUTE_PATCH, bstr::kAttrPatch},
        {CELL_GAME_ATTRIBUTE_APP_HOME, bstr::kAttrAppHome},
        {CELL_GAME_ATTRIBUTE_DEBUG, bstr::kAttrDebug},
        {CELL_GAME_ATTRIBUTE_XMBBUY, bstr::kAttrXmbBuy},
        {CELL_GAME_ATTRIBUTE_COMMERCE2_BROWSER, bstr::kAttrCommerce2},
        {CELL_GAME_ATTRIBUTE_INVITE_MESSAGE, bstr::kAttrInvite},
        {CELL_GAME_ATTRIBUTE_CUSTOM_DATA_MESSAGE, bstr::kAttrCustomData},
        {CELL_GAME_ATTRIBUTE_WEB_BROWSER, bstr::kAttrWebBrowser},
    };
    for (const auto& a : kAttrs)
        if (*attrs & a.bit) Print(a.text);

    Print(bstr::kDirName, dir_name);
    Boot_PrintSizes(size);
    global<be_s32>(kHddFreeSizeKB) = size->hddFreeSizeKB.get();

    Boot_PrintParamInts(reinterpret_cast<be_s32*>(at(420)), value1);

    // TITLE, TITLE_DEFAULT and TITLE_ID (tables at +444 / +456).
    be_s32* str_ids = reinterpret_cast<be_s32*>(at(444));
    be_u32* str_names = reinterpret_cast<be_u32*>(at(456));
    str_ids[0] = CELL_GAME_PARAMID_TITLE;
    str_ids[1] = CELL_GAME_PARAMID_TITLE_DEFAULT;
    str_ids[2] = CELL_GAME_PARAMID_TITLE_ID;
    str_names[0] = bstr::kTitleCurrent;
    str_names[1] = bstr::kTitleDefault;
    str_names[2] = bstr::kTitleId;
    for (u32 i = 0; i < 3; ++i) Boot_PrintParamString(str_ids[i], str_names[i], buf1);

    rc = Boot_ContentPermit(kContentInfoPath, kUsrdirPath);
    if (rc < 0) return rc;
    if ((*attrs & CELL_GAME_ATTRIBUTE_PATCH) == 0) return rc;

    size->hddFreeSizeKB = 0;
    size->sizeKB = 0;
    size->sysSizeKB = 0;
    const s32 patch = call<s32>(bimp::cellGamePatchCheck, size, 0u);
    if (patch < 0) {
        Print(bstr::kPatchError, patch);
    } else {
        Print(bstr::kPatchOk);
        Boot_PrintSizes(size);
    }

    Boot_PrintParamInts(reinterpret_cast<be_s32*>(at(396)), value2);

    be_s32* all_ids = reinterpret_cast<be_s32*>(at(468));
    be_u32* all_names = reinterpret_cast<be_u32*>(at(548));
    call(bfn::memmove, all_ids, kAllStringIds, kAllStringCount * 4);
    call(bfn::memmove, all_names, kAllStringNames, kAllStringCount * 4);
    for (u32 i = 0; i < kAllStringCount; ++i) Boot_PrintParamString(all_ids[i], all_names[i], buf2);

    return Boot_ContentPermit(kContentInfoPath2, kUsrdirPath2);
}

}  // namespace sly4::system

NATIVE(0056C8E4, sly4::system::Game_BootCheck)
