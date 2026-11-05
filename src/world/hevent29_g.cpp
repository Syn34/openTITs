// World_HandleEvent callees, part 29g: the world's cloud save upload and
// download states (level-2 sub-states: transfers, message dialogs, fade).
#include "hevent29.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent29;

namespace {
namespace fn29g {
constexpr u32 StateMachine_SetState = 0x00464D1C;  // (sm, level, id)
constexpr u32 Async_Write = 0x000AFE94;        // (src, n) -> s32 state (Async_Write0AFE94)
constexpr u32 Player2_Reset = 0x00367290;
constexpr u32 Player2_Activate = 0x003671B0;
constexpr u32 World_UpdateSaveIds = 0x00475E20;  // (World*)
constexpr u32 StringTable_Lookup = 0x004F87C4;   // (strings, id, fallback, quiet)
constexpr u32 Save_ErrorMessage = 0x000B00FC;    // () -> text
constexpr u32 Sysutil_CheckCallback = 0x0056C184;
constexpr u32 Fade_Start = 0x00468C40;           // (Fade*, from[4], to[4], f32, bool hide, bool force)
constexpr u32 Obj_SetMode = 0x00470750;          // (o, 18, 0) (Obj_SetMode470750)
constexpr u32 Save_SaveGameToList = 0x000AED78;  // (data, size) -> s32 state
constexpr u32 Async_Read = 0x000AFFD0;           // (dst, n) -> s32 state (Async_Read0AFFD0)
constexpr u32 World_BeginLevel = 0x00472234;     // (World*) (World_BeginLevel472234)
constexpr u32 memset = 0x00726E54;
constexpr u32 memmove = 0x0071C3F0;
// cellSysutil import stubs.
constexpr u32 cellMsgDialogOpen2 = 0x00ABDB04;
constexpr u32 cellMsgDialogAbort = 0x00ABDAE4;
constexpr u32 cellMsgDialogClose = 0x00ABDA24;
}  // namespace fn29g

// CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL | BG_INVISIBLE | BUTTON_TYPE_OK | DISABLE_CANCEL_ON.
constexpr u32 kDialogType = 0x1 | 0x4 | 0x20 | 0x80;
// ... | BUTTON_TYPE_YESNO | DEFAULT_CURSOR_NO (instead of OK).
constexpr u32 kConfirmType = 0x1 | 0x4 | 0x10 | 0x80 | 0x100;
constexpr u32 kCloseDelayDl = 0x0047246C;    // f32 cellMsgDialogClose delay (download)
constexpr u32 kDownload = 0x010BD6F0;        // u8[4056]: the downloaded save
constexpr u32 kStrDlConfirm = 0x00AE8B50;    // "STR_CLOUD_DOWNLOAD_CONFIRM"
constexpr u32 kStrDlError = 0x00AE8B88;      // "STR_CLOUD_DOWNLOAD_ERROR"
constexpr u32 kWLevel2Next = 34;             // u8
constexpr u32 kDialogState = 0x010BB850;     // u32: 3 while the dialog is up
constexpr u32 kDialogCallback = 0x00F39D98;  // OPD given to cellMsgDialogOpen2
constexpr u32 kStrings = 0x00F6687C;         // Strings*
constexpr u32 kSaveError = 0x00F9D1B8;       // s32 save error kind (name?)
constexpr u32 kSaveFlags = 0x010BB9F8;       // u32: bit 0x80 (name?)
constexpr u32 kNpPromptState = 0x00F65C94;   // s32
constexpr u32 kCloseDelay = 0x00475F68;      // f32 cellMsgDialogClose delay
constexpr u32 kFadeTime = 0x010B8E28 + 72;   // f32
constexpr u32 kFadeFrom = 0x010B8E24, kFadeTo = 0x010B8DD8;  // colours (name?)
constexpr u32 kWFade = 0x128290;             // World +: Fade
constexpr u32 kStrNoData = 0x00AE8CA0;       // "STR_CLOUD_NO_DATA"
constexpr u32 kStrUploadError = 0x00AE8CD8;  // "STR_CLOUD_UPLOAD_ERROR"
constexpr u32 kStrUploadOk = 0x00AE8CB8;     // "STR_CLOUD_UPLOAD_SUCCESS"
constexpr u32 kStrMissing = 0x00AE8B70;      // "[Missing cloud message]"
constexpr u32 kSaveSize = 4056;
// World (state machine) fields.
constexpr u32 kWStateTable = 8;  // u8*: its first byte goes to +47 and SetState
constexpr u32 kWPending = 32;    // u8: next sub-state when not entered directly
constexpr u32 kWFlags = 44;      // u8: bit 0x80 save data present
constexpr u32 kWNextLevel1 = 49, kWLevel1 = 50;  // u8
constexpr u32 kWMode = 35;       // u8

struct Upload {
    PPUContext& ctx;
    u32 w;
    bool now;  // entered directly (arg 1): switch at once, else leave the sub-state pending
    // r3 as the original leaves it: SetState's, or the pending value.
    void go(u32 st) {
        if (now) {
            const u32 b = load<u8>(load<u32>(w + kWStateTable));
            store<u8>(w + 47, static_cast<u8>(b));
            call(ctx, fn29g::StateMachine_SetState, {w, 2, st, b});
        } else {
            store<u8>(w + kWPending, static_cast<u8>(st));
            ctx.gpr[3] = st;
        }
    }
    // MsgDialog_Open (inlined): an open dialog aborted, the new one shown;
    // sub-state 2 waits for it, 3 when it failed to open.
    void dialog(u32 text, u32 ok = 2, u32 fail = 3, u32 type = kDialogType) {
        if (load<u32>(kDialogState) != 0) {
            call(ctx, fn29g::cellMsgDialogAbort, {});
            store<u32>(kDialogState, 0);
        }
        if (static_cast<s32>(call(ctx, fn29g::cellMsgDialogOpen2, {type, text, kDialogCallback, 0, 0})) == 0) {
            store<u32>(kDialogState, 3);
            go(ok);
        } else {
            go(fail);
        }
    }
    void message(u32 strings, u32 id, u32 ok = 2, u32 fail = 3, u32 type = kDialogType) {
        dialog(static_cast<u32>(call(ctx, fn29g::StringTable_Lookup, {strings, id, kStrMissing, 0})), ok, fail, type);
    }
    // The fade out both state machines end with; now: level 1 state 12
    // (when level 1 is 0 or past 11, else its next is 12).
    void fade_level1() {
        const u8 lv = load<u8>(w + kWLevel1);
        const u32 b = load<u8>(load<u32>(w + kWStateTable));
        store<u8>(w + 47, static_cast<u8>(b));
        if (lv == 0 || !(static_cast<s32>(lv) < 12)) {
            call(ctx, fn29g::StateMachine_SetState, {w, 1, 12});
        } else {
            store<u8>(w + kWNextLevel1, 12);
            ctx.gpr[3] = 12;
        }
    }
    void fade_start() {
        // r6 holds 0x010C0000 (an address's high half); Fade_Start does not read it.
        callf(ctx, fn29g::Fade_Start, {w + kWFade, kFadeFrom, kFadeTo, 0x010C0000, 1, 0}, {lfs_at(kFadeTime)});
    }
    // Waits for the dialog to close (sysutil callbacks): true when it is
    // still up (or was closed while up), with r3 as the original leaves it.
    bool dialog_busy(u32 delay) {
        if (load<u32>(kDialogState) == 0) return false;
        call(ctx, fn29g::Sysutil_CheckCallback, {});
        if (load<s32>(kDialogState) == 3) {
            ctx.gpr[3] = 3;
            return true;
        }
        callf(ctx, fn29g::cellMsgDialogClose, {}, {lfs_at(delay)});
        const s32 was = load<s32>(kDialogState);
        store<u32>(kDialogState, 0);
        if (was == 3) {
            ctx.gpr[3] = 0x010C0000;  // (the address's high half, as left)
            return true;
        }
        return false;
    }
};
}  // namespace

// void World_CloudUpload475F70(World* w, u8 now) (name?): the upload
// sub-state (vt +32 level 2 when now, else the pending +32):
//  0: with save data (+44 bit 0x80) the save ids refreshed, player 2
//     activated, sub-state 1; else the "STR_CLOUD_NO_DATA" message;
//  1: the save slot written (Async_Write0AFE94 on +80, 4056 bytes): 2 =
//     done (player 2 reset, "STR_CLOUD_UPLOAD_SUCCESS" or sub-state 3
//     without strings), 3 = failed (player 2 reset; by the save error kind
//     0x00F9D1B8: 7 the PSN prompt re-armed, 4 / 3 the save error text, 1
//     sub-state 3, else "STR_CLOUD_UPLOAD_ERROR");
//  2: waits for the dialog (sysutil callbacks), closes it, sub-state 3;
//  3: fades out; now: level 1 state 12 (when level 1 is 0 or past 11, else
//     its next is 12), else mode 18.
DECOMP(00475F70, World_CloudUpload475F70) {
    const u32 w = arg<u32>(ctx, 0);
    Upload u{ctx, w, arg<u8>(ctx, 1) == 1};
    const s32 st = u.now ? static_cast<s32>(vcall(ctx, w, 32, {w, 2})) : static_cast<s32>(load<u8>(w + kWPending));
    ctx.gpr[3] = static_cast<u32>(st);

    if (st == 0) {
        const u32 has = (load<u8>(w + kWFlags) >> 7) & 1;
        if (has == 1) {
            call(ctx, fn29g::World_UpdateSaveIds, {load<u32>(kCurrentWorld)});
            call(ctx, fn29g::Player2_Activate, {});
            u.go(1);
            return;
        }
        ctx.gpr[3] = has;
        const u32 strings = load<u32>(kStrings);
        if (strings != 0) u.message(strings, kStrNoData);
    } else if (st == 1) {
        const s32 r = static_cast<s32>(call(ctx, fn29g::Async_Write, {load<u32>(kCurrentWorld) + kSaveData, kSaveSize}));
        if (r == 2) {
            call(ctx, fn29g::Player2_Reset, {});
            const u32 strings = load<u32>(kStrings);
            if (strings != 0) u.message(strings, kStrUploadOk);
            else u.go(3);
        } else if (r == 3) {
            call(ctx, fn29g::Player2_Reset, {});
            const s32 err = load<s32>(kSaveError);
            if (err == 7) {
                if ((load<u32>(kSaveFlags) & 0x80u) == 0) {
                    store<u32>(kNpPromptState, 0);
                    store<u32>(kSaveFlags, load<u32>(kSaveFlags) | 0x80u);
                }
                u.go(3);
            } else if (err == 4 || err == 3) {
                u.dialog(static_cast<u32>(call(ctx, fn29g::Save_ErrorMessage, {})));
            } else if (err == 1) {
                u.go(3);
            } else {
                const u32 strings = load<u32>(kStrings);
                if (strings != 0) u.message(strings, kStrUploadError);
                else u.go(3);
            }
        }
    } else if (st == 2) {
        if (u.dialog_busy(kCloseDelay)) return;
        u.go(3);
    } else if (st == 3) {
        u.fade_start();
        if (!u.now) {
            store<u8>(w + kWMode, 0);
            call(ctx, fn29g::Obj_SetMode, {w, 18, 0, 0});
            return;
        }
        u.fade_level1();
    }
}

// void World_CloudDownload472470(World* w, u8 now) (name?): the download
// sub-state (vt +32 level 2 when now, else the pending +32):
//  0: the buffer 0x010BD6F0 cleared; now: sub-state 2 and player 2
//     activated, else the "STR_CLOUD_DOWNLOAD_CONFIRM" yes / no dialog
//     (sub-state 1, or 7 when it fails to open);
//  1: waits for the answer (1 = yes: player 2 activated, sub-state 2; 0 /
//     2 / no dialog: 7);
//  2: the read (Async_Read0AFFD0): 2 = done (player 2 reset, 3), 3 =
//     failed (player 2 reset; by the save error kind 7 the PSN prompt
//     re-armed, 4 / 3 the save error text, 1 nothing, else
//     "STR_CLOUD_DOWNLOAD_ERROR"; the dialogs go to 6, the rest to 7);
//  3: the download listed (Save_SaveGameToList, again unless 1), 4;
//  4: the current save listed: 2 = the download copied over it (5), 3 = 7;
//  5: fade, +44 bit 0x80, World_BeginLevel472234; not now: mode 23 and
//     level-2 next 6;
//  6: waits for the dialog, 7;
//  7: fades out (as the upload's 3).
DECOMP(00472470, World_CloudDownload472470) {
    const u32 w = arg<u32>(ctx, 0);
    const u8 imm = arg<u8>(ctx, 1);
    Upload u{ctx, w, imm == 1};
    const s32 st = u.now ? static_cast<s32>(vcall(ctx, w, 32, {w, 2})) : static_cast<s32>(load<u8>(w + kWPending));
    ctx.gpr[3] = static_cast<u32>(st);

    switch (st) {
    case 0: {
        call(ctx, fn29g::memset, {kDownload, 0, kSaveSize});
        if (u.now) {
            u.go(2);
            call(ctx, fn29g::Player2_Activate, {});
            return;
        }
        const u32 strings = load<u32>(kStrings);
        if (strings != 0) u.message(strings, kStrDlConfirm, 1, 7, kConfirmType);
        return;
    }
    case 1: {
        if (load<u32>(kDialogState) == 0) {
            u.go(7);
            return;
        }
        call(ctx, fn29g::Sysutil_CheckCallback, {});
        if (load<s32>(kDialogState) == 3) {
            ctx.gpr[3] = 3;
            return;
        }
        callf(ctx, fn29g::cellMsgDialogClose, {}, {lfs_at(kCloseDelayDl)});
        const s32 ans = load<s32>(kDialogState);
        store<u32>(kDialogState, 0);
        ctx.gpr[3] = static_cast<u32>(ans);
        if (ans <= -1 || ans > 2) return;
        if (ans == 1) {
            call(ctx, fn29g::Player2_Activate, {});
            u.go(2);
        } else {
            u.go(7);
        }
        return;
    }
    case 2: {
        const s32 r = static_cast<s32>(call(ctx, fn29g::Async_Read, {kDownload, kSaveSize}));
        if (r == 2) {
            call(ctx, fn29g::Player2_Reset, {});
            u.go(3);
        } else if (r == 3) {
            call(ctx, fn29g::Player2_Reset, {});
            const s32 err = load<s32>(kSaveError);
            if (err == 7) {
                if ((load<u32>(kSaveFlags) & 0x80u) == 0) {
                    store<u32>(kNpPromptState, 0);
                    store<u32>(kSaveFlags, load<u32>(kSaveFlags) | 0x80u);
                }
                u.go(7);
            } else if (err == 4 || err == 3) {
                u.dialog(static_cast<u32>(call(ctx, fn29g::Save_ErrorMessage, {})), 6, 7);
            } else if (err == 1) {
                u.go(7);
            } else {
                const u32 strings = load<u32>(kStrings);
                if (strings != 0) u.message(strings, kStrDlError, 6, 7);
                else u.go(7);
            }
        }
        return;
    }
    case 3:
        if (static_cast<s32>(call(ctx, fn29g::Save_SaveGameToList, {kDownload, kSaveSize})) != 1)
            call(ctx, fn29g::Save_SaveGameToList, {kDownload, kSaveSize});
        u.go(4);
        return;
    case 4: {
        const s32 r = static_cast<s32>(call(ctx, fn29g::Save_SaveGameToList, {load<u32>(kCurrentWorld) + kSaveData, kSaveSize}));
        if (r == 2) {
            call(ctx, fn29g::memmove, {load<u32>(kCurrentWorld) + kSaveData, kDownload, kSaveSize});
            u.go(5);
        } else if (r == 3) {
            u.go(7);
        }
        return;
    }
    case 5:
        u.fade_start();
        store<u8>(w + kWFlags, static_cast<u8>(load<u8>(w + kWFlags) | 0x80));
        call(ctx, fn29g::World_BeginLevel, {w});
        if (imm != 0) return;
        store<u8>(w + kWMode, 0);
        call(ctx, fn29g::Obj_SetMode, {w, 23, 0, 0});
        store<u8>(w + kWLevel2Next, 6);
        ctx.gpr[3] = 6;
        return;
    case 6:
        if (u.dialog_busy(kCloseDelayDl)) return;
        u.go(7);
        return;
    case 7:
        u.fade_start();
        if (!u.now) {
            store<u8>(w + kWMode, 0);
            call(ctx, fn29g::Obj_SetMode, {w, 18, 0, 0});
            return;
        }
        u.fade_level1();
        return;
    default:
        return;
    }
}
