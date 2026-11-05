// World_HandleEvent callees, part 21c: the network start dialog / save
// thread state machine step, a resource preload list and the set erase.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kSysPpuThreadCreate = 0x00ABF144;     // sysPrxForUser import stub
constexpr u32 kNpGetContentRatingFlag = 0x00ABE404;  // sceNp import stub sceNpManagerGetContentRatingFlag(s32* is_restricted, s32* age)
constexpr u32 kNetDialogUnloadAsync = 0x00ABDE04;   // cellNetCtl import stub cellNetCtlNetStartDialogUnloadAsync(result*)
constexpr u32 kNetDialogLoadAsync = 0x00ABDDE4;     // cellNetCtl import stub cellNetCtlNetStartDialogLoadAsync(param*)
constexpr u32 kSysutilRegisterCallback = 0x0056C1E8;    // (slot, OPD, userdata) (symbols.txt Sysutil_RegisterCallback)
constexpr u32 kSysutilUnregisterCallback = 0x0056C274;  // (slot) (symbols.txt Sysutil_UnregisterCallback)
constexpr u32 kNetStep = 0x00F9D1B4;       // s32 step: 0 load dialog, 1 unload, 2 rating, 3 thread, 4 running, -1 failed
constexpr u32 kAsyncState = 0x00F9D1AC;    // s32 async state (Async_Read0AFFD0 / Async_Write0AFE94); 3 = failed
constexpr u32 kNetError = 0x00F9D1B8;      // s32 error code (name?)
constexpr u32 kNetErrorKind = 0x00F9D1BC;  // s32 error detail (name?)
constexpr u32 kNetCallbackOpd = 0x00F30FF0;  // sysutil callback OPD
constexpr u32 kUploadEntry = 0x00F30FF8, kDownloadEntry = 0x00F31000;  // thread entry OPDs
constexpr u32 kUploadName = 0x00AC60BC;    // "UploadSave"
constexpr u32 kDownloadName = 0x00AC60CC;  // "DownloadSave"
constexpr u32 kCallbackSlot = 3;
constexpr u32 kThreadPrio = 1001, kThreadStack = 16384;
constexpr u32 kNetDialogCanceled = 0x80130190;  // dialog result that ends the step with error 1 (name?)
constexpr u32 kHi = 0x00FA0000;            // lis r3, 0xFA: what r3 holds on most exits

constexpr u32 kPreloader = 0x011E289C;     // u32 object whose vt +20 (obj, hash, 0) requests a resource (name?)
constexpr u32 kPreloadHashes[] = {
    0x38F1276D, 0x34DF5494, 0x2FF8C16F, 0x6CEE9071, 0xF03421DE, 0x23CD067C, 0x5FE59C18, 0x41278654,
    0xF02E32E0, 0xF02E32E1, 0xF02E32E2, 0xF02132D3, 0x54479D9C, 0x63280484, 0xF02184ED,
};

constexpr u32 kSetTree = 0x010C2888;       // the set registry: +0 root, +4 s32 count; iterator map at +12 (hevent6.h layout)
constexpr u32 kSetMap = kSetTree + 12;
constexpr u32 kSetLast = 0x011DE4A4;      // u32 value found by the last registry lookup
constexpr u32 kSetStop = 0x004BD1CC;       // (Set*) (symbols.txt Set_Stop4BD1CC)
constexpr u32 kMemFreeFrom = 0x0054EE6C;   // (p, pool) (symbols.txt Mem_FreeFrom)
constexpr u32 kTreeFind78B1B0 = 0x0078B1B0;   // (tree, key, out*, root) -> found (symbols.txt TreeFind_0078B1B0)
constexpr u32 kTreeErase78A5C0 = 0x0078A5C0;  // (tree, Node** link, key, u8* done) (name?)
constexpr u32 kTreeFind793080 = 0x00793080;   // (tree, key, out*, root) -> found (symbols.txt Tree_Find793080)
constexpr u32 kTreeErase792490 = 0x00792490;  // (tree, Node** link, key, u8* done) (symbols.txt Tree_Erase792490)

inline void net_fail(u32 code, u32 kind) {
    store<u32>(kNetError, code);
    store<u32>(kNetErrorKind, kind);
    store<u32>(kNetStep, 0xFFFFFFFFu);
}
}  // namespace

// void Async_Start0AF5FC(u8 write) (name?): one step of the online save
// transfer start-up (state 0x00F9D1B4): 0 registers the sysutil callback
// and loads the network start dialog (-> 4); 1 unloads it (result 0 -> 2,
// 0x80130190 -> error 1); 2 checks the content rating flag (restricted
// under age 7 -> error 4 / 2, else 3); 3 starts the "UploadSave" (write)
// or "DownloadSave" thread (-> 4); -1 resets to 0 with the async state
// failed (3). Failures set error 0x00F9D1B8 (3 unless noted), detail
// 0x00F9D1BC, step -1. r3 is left as the original leaves it.
DECOMP(000AF5FC, Async_Start0AF5FC) {
    const s32 step = static_cast<s32>(load<u32>(kNetStep));
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(step));
    Stack frame(ctx, 48);
    const u32 st = frame.addr();  // +0 dialog result (112), +8 thread id (120), +16 rating out (128), +24 load param (136)
    switch (step) {
    case 3:
        if ((arg<u32>(ctx, 0) & 0xFF) == 1)
            call(ctx, kSysPpuThreadCreate, {st + 8, kUploadEntry, 0, kThreadPrio, kThreadStack, 0, kUploadName});
        else
            call(ctx, kSysPpuThreadCreate, {st + 8, kDownloadEntry, 0, kThreadPrio, kThreadStack, 0, kDownloadName});
        store<u32>(kNetStep, 4);
        ret(ctx, kHi);
        return;
    case 2: {
        store<u32>(st + 16, 1);
        store<u32>(st + 20, 0);
        if (static_cast<s32>(call(ctx, kNpGetContentRatingFlag, {st + 16, st + 20})) < 0) {
            net_fail(3, 1);
            ret(ctx, -1);
        } else if (load<u32>(st + 16) == 0 || !(static_cast<s32>(load<u32>(st + 20)) < 7)) {
            store<u32>(kNetStep, 3);
            ret(ctx, kHi);
        } else {
            net_fail(4, 2);
            ret(ctx, -1);
        }
        return;
    }
    case 1: {
        store<u64>(st, 0);
        store<u32>(st, 8);  // CellNetCtlNetStartDialogResult {size 8, result}
        if (static_cast<s32>(call(ctx, kNetDialogUnloadAsync, {st})) < 0) {
            net_fail(3, 3);
        } else {
            const u32 result = load<u32>(st + 4);
            if (result == 0) {
                store<u32>(kNetStep, 2);
            } else if (result == kNetDialogCanceled) {
                store<u32>(kNetError, 1);
                store<u32>(kNetStep, 0xFFFFFFFFu);
            } else {
                net_fail(3, 3);
            }
        }
        ret(ctx, kHi);
        return;
    }
    case 0: {
        if (static_cast<s32>(call(ctx, kSysutilRegisterCallback, {kCallbackSlot, kNetCallbackOpd, 0})) < 0) {
            net_fail(3, 3);
            ret(ctx, kHi);
            return;
        }
        store<u32>(st + 32, 0);
        store<u32>(st + 24, 12);  // CellNetCtlNetStartDialogParam {size 12, type 1, cid 0}
        store<u32>(st + 28, 1);
        if (static_cast<s32>(call(ctx, kNetDialogLoadAsync, {st + 24})) < 0) {
            store<u32>(kNetError, 3);
            store<u32>(kNetErrorKind, 3);
            call(ctx, kSysutilUnregisterCallback, {kCallbackSlot});
            store<u32>(kNetStep, 0xFFFFFFFFu);
        } else {
            store<u32>(kNetStep, 4);
        }
        ret(ctx, kHi);
        return;
    }
    case -1:
        store<u32>(kNetStep, 0);
        store<u32>(kAsyncState, 3);
        ret(ctx, kHi);
        return;
    default:
        return;  // 4 (running) and anything else: nothing to do
    }
}

// u32 Preload_Request342E00() (name?): asks the preloader (*0x011E289C)
// for fifteen resources by hash, vt +20 (obj, hash, 0) each (the hashes go
// in sign-extended, as lis / ori builds them). Returns the last result.
DECOMP(00342E00, Preload_Request342E00) {
    for (u32 h : kPreloadHashes) {
        const u32 obj = load<u32>(kPreloader);
        vcall(ctx, obj, 20, {obj, static_cast<u64>(static_cast<s64>(static_cast<s32>(h))), 0});
    }
}

// u32 Set_Erase4BD4B4(u32 hash) (name?): looks the set up in the registry
// (0x010C2888, last value 0x011DE4A4; the lookup is made twice, as in the
// original). A miss returns the last node visited. A hit stops the set
// (Set_Stop4BD1CC, also for a null set); a set's own tree (+4 root, +8
// count, iterator +16) is emptied from its first node (Tree_Find793080 /
// Tree_Erase792490; a failed find retries the same node) and the set
// freed. Then the registry entry is erased (TreeFind_0078B1B0 /
// 0x0078A5C0, count +4 decremented). Returns the new count, or 0.
DECOMP(004BD4B4, Set_Erase4BD4B4) {
    const u32 key = arg<u32>(ctx, 0);
    const u32 m = kSetMap;
    u32 root = load<u32>(load<u32>(m));
    if (root == 0) {
        store<u32>(m + hevent6::kMapNode, 0);
        store<u32>(kSetLast, 0);
        ret(ctx, 0);
        return;
    }
    u32 n = hevent6::map_walk(m, root, key);
    if (n == 0) {
        ret(ctx, load<u32>(m + hevent6::kMapNode));  // the last node visited
        return;
    }
    store<u32>(m + hevent6::kMapNode, n);
    store<u32>(kSetLast, load<u32>(n + hevent6::kNodeValue));
    u32 set = 0;
    root = load<u32>(load<u32>(m));
    if (root == 0) {
        store<u32>(kSetLast, 0);
        store<u32>(m + hevent6::kMapNode, 0);
    } else if ((n = hevent6::map_walk(m, root, key)) != 0) {
        store<u32>(m + hevent6::kMapNode, n);
        store<u32>(kSetLast, load<u32>(n + hevent6::kNodeValue));
        set = load<u32>(n + hevent6::kNodeValue);
    }
    call(ctx, kSetStop, {set});
    Stack frame(ctx, 16);
    const u32 st = frame.addr();  // +0 / +4 find outs (112 / 116), +8 / +9 erase flags (120 / 121)
    if (set != 0) {
        const u32 sm = set + 16;
        const u32 tree = set + 4;
        for (;;) {
            const u32 first = hevent6::map_first(sm);
            if (first == 0) break;
            const u32 k = load<u32>(first + hevent6::kNodeKey);
            store<u8>(st + 9, 0);
            if (static_cast<u32>(call(ctx, kTreeFind793080, {tree, k, st + 4, load<u32>(set + 4)})) == 0) continue;
            call(ctx, kTreeErase792490, {tree, tree, k, st + 9});
            store<u32>(set + 8, load<u32>(set + 8) - 1);
        }
        call(ctx, kMemFreeFrom, {set, 0});
    }
    store<u8>(st + 8, 0);
    if (static_cast<u32>(call(ctx, kTreeFind78B1B0, {kSetTree, key, st, load<u32>(kSetTree)})) == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, kTreeErase78A5C0, {kSetTree, kSetTree, key, st + 8});
    const u32 count = load<u32>(kSetTree + 4) - 1;
    store<u32>(kSetTree + 4, count);
    ret(ctx, count);
}
