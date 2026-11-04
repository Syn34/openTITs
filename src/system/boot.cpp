// Game_BootCheck (0x0056C8E4).
#include "boot.h"

using namespace sly4;
using namespace sly4::boot;

namespace {

void Print(PPUContext& ctx, std::initializer_list<u64> args) { call(ctx, bfn::printf, args); }

// The three PARAM.SFO integers, with the id and name tables the original
// builds on its stack each time (at `ids` and `ids + 12`).
void Boot_PrintParamInts(PPUContext& ctx, u32 ids, u32 value) {
    store<s32>(ids + 0, CELL_GAME_PARAMID_PARENTAL_LEVEL);
    store<s32>(ids + 4, CELL_GAME_PARAMID_RESOLUTION);
    store<s32>(ids + 8, CELL_GAME_PARAMID_SOUND_FORMAT);
    store<u32>(ids + 12, bstr::kParentalLevel);
    store<u32>(ids + 16, bstr::kResolution);
    store<u32>(ids + 20, bstr::kSoundFormat);
    for (u32 i = 0; i < 3; ++i) {
        store<u32>(value, 0);
        const s32 id = load<s32>(ids + i * 4);
        const u64 rc = call(ctx, bimp::cellGameGetParamInt, {static_cast<u64>(static_cast<s64>(id)), value});
        const u32 name = load<u32>(ids + 12 + i * 4);
        if (static_cast<s32>(rc) < 0)
            Print(ctx, {bstr::kParamIntError, name, rc});
        else
            Print(ctx, {bstr::kParamIntOk, name, static_cast<u64>(static_cast<s64>(load<s32>(value)))});
    }
}

// One PARAM.SFO string into a cleared 128-byte buffer.
void Boot_PrintParamString(PPUContext& ctx, s32 id, u32 name, u32 buf) {
    for (u32 k = 0; k < kParamStringSize; ++k) store<u8>(buf + k, 0);  // eight 16-byte vector stores
    const u64 rc = call(ctx, bimp::cellGameGetParamString,
                        {static_cast<u64>(static_cast<s64>(id)), buf, kParamStringSize});
    if (static_cast<s32>(rc) < 0)
        Print(ctx, {bstr::kParamStrError, name, rc});
    else
        Print(ctx, {bstr::kParamStrOk, name, buf});
}

void Boot_PrintSizes(PPUContext& ctx, const GameContentSize& size) {
    Print(ctx, {bstr::kSizes, static_cast<u64>(static_cast<s64>(size.hddFreeSizeKB.get())),
                static_cast<u64>(static_cast<s64>(size.sizeKB.get())),
                static_cast<u64>(static_cast<s64>(size.sysSizeKB.get()))});
}

// cellGameContentPermit with its printout; returns its result.
s32 Boot_ContentPermit(PPUContext& ctx, u32 info_path, u32 usrdir_path) {
    const s32 rc = static_cast<s32>(call(ctx, bimp::cellGameContentPermit, {info_path, usrdir_path}));
    if (rc < 0) {
        Print(ctx, {bstr::kPermitError, static_cast<u64>(static_cast<s64>(rc))});
        return rc;
    }
    Print(ctx, {bstr::kPermitOk});
    Print(ctx, {bstr::kContentInfo, info_path});
    Print(ctx, {bstr::kUsrdir, usrdir_path});
    return rc;
}

}  // namespace

// s32 Game_BootCheck()
// cellGameBootCheck, a printout of everything it and PARAM.SFO report,
// then cellGameContentPermit. A patched game (CELL_GAME_ATTRIBUTE_PATCH)
// also runs cellGamePatchCheck, prints all 20 PARAM.SFO strings and
// permits the content again into the second pair of paths. Returns the
// first failing call's error, or the last ContentPermit result.
DECOMP(0056C8E4, Game_BootCheck) {
    // The original's frame: the string buffers at +112 and +240, type and
    // attributes at +368/+372, the int results at +376/+380, the sizes at
    // +384, the id/name tables from +396, dirName at +632.
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 664 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 buf2 = sp + 112, buf1 = sp + 240, type = sp + 368, attrs = sp + 372;
    const u32 value2 = sp + 376, value1 = sp + 380, sizes = sp + 384, dir_name = sp + 632;
    auto& size = ref<GameContentSize>(sizes);

    size.hddFreeSizeKB = 0;
    size.sizeKB = 0;
    size.sysSizeKB = 0;
    s32 rc = static_cast<s32>(call(ctx, bimp::cellGameBootCheck, {type, attrs, sizes, dir_name}));
    if (rc < 0) {
        Print(ctx, {bstr::kBootError, static_cast<u64>(static_cast<s64>(rc))});
        return ret(ctx, rc);
    }
    Print(ctx, {bstr::kBootOk});
    Print(ctx, {bstr::kGetType, load<u32>(type)});
    if (load<u32>(type) == CELL_GAME_GAMETYPE_DISC) Print(ctx, {bstr::kTypeDisc});
    if (load<u32>(type) == CELL_GAME_GAMETYPE_HDD) Print(ctx, {bstr::kTypeHdd});

    Print(ctx, {bstr::kAttributes, load<u32>(attrs)});
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
        if (load<u32>(attrs) & a.bit) Print(ctx, {a.text});

    Print(ctx, {bstr::kDirName, dir_name});
    Boot_PrintSizes(ctx, size);
    store<s32>(kHddFreeSizeKB, size.hddFreeSizeKB);

    Boot_PrintParamInts(ctx, sp + 420, value1);

    // TITLE, TITLE_DEFAULT and TITLE_ID (tables at +444 / +456).
    const u32 str_ids = sp + 444, str_names = sp + 456;
    store<s32>(str_ids + 0, CELL_GAME_PARAMID_TITLE);
    store<s32>(str_ids + 4, CELL_GAME_PARAMID_TITLE_DEFAULT);
    store<s32>(str_ids + 8, CELL_GAME_PARAMID_TITLE_ID);
    store<u32>(str_names + 0, bstr::kTitleCurrent);
    store<u32>(str_names + 4, bstr::kTitleDefault);
    store<u32>(str_names + 8, bstr::kTitleId);
    for (u32 i = 0; i < 3; ++i)
        Boot_PrintParamString(ctx, load<s32>(str_ids + i * 4), load<u32>(str_names + i * 4), buf1);

    rc = Boot_ContentPermit(ctx, kContentInfoPath, kUsrdirPath);
    if (rc < 0) return ret(ctx, rc);
    if ((load<u32>(attrs) & CELL_GAME_ATTRIBUTE_PATCH) == 0) return ret(ctx, rc);

    size.hddFreeSizeKB = 0;
    size.sizeKB = 0;
    size.sysSizeKB = 0;
    const s32 patch = static_cast<s32>(call(ctx, bimp::cellGamePatchCheck, {sizes, 0}));
    if (patch < 0) {
        Print(ctx, {bstr::kPatchError, static_cast<u64>(static_cast<s64>(patch))});
    } else {
        Print(ctx, {bstr::kPatchOk});
        Boot_PrintSizes(ctx, size);
    }

    Boot_PrintParamInts(ctx, sp + 396, value2);

    const u32 all_ids = sp + 468, all_names = sp + 548;
    call(ctx, bfn::memmove, {all_ids, kAllStringIds, kAllStringCount * 4});
    call(ctx, bfn::memmove, {all_names, kAllStringNames, kAllStringCount * 4});
    for (u32 i = 0; i < kAllStringCount; ++i)
        Boot_PrintParamString(ctx, load<s32>(all_ids + i * 4), load<u32>(all_names + i * 4), buf2);

    rc = Boot_ContentPermit(ctx, kContentInfoPath2, kUsrdirPath2);
    ret(ctx, rc);
}
