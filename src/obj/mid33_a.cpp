// Game code, medium sweep part 33: the NP TUS trophy backup threads.
//
// The TUS blob at 0x010127A0 (4056 bytes, slot 1): +8 u32 cleared after a
// download, +12 the online id string, +4040 u32[2] the trophy bitset (bit
// i of word i / 32, stored little-endian). The thread result goes to
// 0x00F9D1AC (2 ok, 3 failed); a "no data" download error leaves 2 at
// 0x00F9D1B8 and the error code at 0x00F9D1BC.
#include "mid33.h"

using namespace sly4;
using namespace sly4::mid33;

namespace {
using namespace m33fn;
constexpr u32 NpGetNpId33 = 0x00ABE4A4u;        // sceNpManagerGetNpId(npId*)
constexpr u32 NpGetOnlineId33 = 0x00ABE444u;    // sceNpManagerGetOnlineId(onlineId*)
constexpr u32 TusCreateTitle33 = 0x00ABE7E4u;   // sceNpTusCreateTitleCtx(commId, pass, selfNpId)
constexpr u32 TusDestroyTitle33 = 0x00ABE7A4u;  // sceNpTusDestroyTitleCtx(ctx)
constexpr u32 TusCreateTrans33 = 0x00ABE764u;   // sceNpTusCreateTransactionCtx(titleCtx)
constexpr u32 TusDestroyTrans33 = 0x00ABE7C4u;  // sceNpTusDestroyTransactionCtx(trans)
constexpr u32 TusSetData33 = 0x00ABE804u;       // sceNpTusSetData(trans, npId, slot, total, send, data, info, infoSize, opt)
constexpr u32 TusGetData33 = 0x00ABE824u;       // sceNpTusGetData(trans, npId, slot, status, statusSize, data, size, opt)
constexpr u32 ThreadExit33 = 0x00ABF3A4u;       // sys_ppu_thread_exit(code)
constexpr u32 TusCommId33 = 0x00467C4Cu;        // ReturnK_00467C4C(0): the communication id
constexpr u32 TusPass33 = 0x0048A680u;          // ReturnK_0048A680(0): the passphrase
constexpr u32 kTusBlob33 = 0x010127A0u;
constexpr u32 kTusState33 = 0x00F9D1ACu;
constexpr u32 kTusNoData33 = 0x00F9D1B8u;
constexpr u32 kTusErr33 = 0x00F9D1BCu;

// The two bitset words at blob +4040 byte-swapped in place.
void TusSwap33(u32 blob) {
    for (u32 k = 0; k < 2; ++k) {
        const u32 a = blob + 4040 + 4 * k;
        store<u32>(a, __builtin_bswap32(load<u32>(a)));
    }
}

// Failure exit: state 3, then sys_ppu_thread_exit(0) (r4 / r5 as left).
void TusFail33(PPUContext& ctx) {
    store<u32>(kTusState33, 3);
    call(ctx, ThreadExit33, {0, 0x00FA0000, 3});
}
}  // namespace

// The TUS upload thread: opens a title and a transaction context, stores
// the online id and the 50 trophy bits (Trophy_Has(i) == 1) in the blob,
// uploads it to slot 1 and records 2 (ok) or 3 (any failure) at 0x00F9D1AC.
DECOMP(000B0D4C, TusUpload_Thread_000B0D4C) {
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();
    const u32 npid = sp + 148, oid = sp + 128;
    if (static_cast<s32>(call(ctx, NpGetNpId33, {npid})) != 0) return TusFail33(ctx);
    const u32 comm = static_cast<u32>(call(ctx, TusCommId33, {0}));
    const u32 pass = static_cast<u32>(call(ctx, TusPass33, {0}));
    const s32 title = static_cast<s32>(call(ctx, TusCreateTitle33, {comm, pass, npid}));
    if (title < 0) return TusFail33(ctx);
    const s32 trans = static_cast<s32>(call(ctx, TusCreateTrans33, {static_cast<u32>(title)}));
    if (trans < 0) {
        store<u32>(kTusState33, 3);
        call(ctx, TusDestroyTitle33, {static_cast<u32>(title), 0x00FA0000, 3});
        call(ctx, ThreadExit33, {0});
        return;
    }
    call(ctx, Memset33, {oid, 0, 20});
    if (static_cast<s32>(call(ctx, NpGetOnlineId33, {oid})) < 0) {
        store<u32>(kTusState33, 3);
        call(ctx, TusDestroyTitle33, {static_cast<u32>(title), 0x00FA0000, 3});
        call(ctx, ThreadExit33, {0});
        return;
    }
    call(ctx, StrAssign33, {kTusBlob33 + 12, oid});
    store<u32>(kTusBlob33 + 4040, 0);
    store<u32>(kTusBlob33 + 4044, 0);
    for (s32 i = 0; i < 50; ++i) {
        if (static_cast<s32>(call(ctx, TrophyHas33, {load<u32>(kTrophyPtr33), static_cast<u32>(i)})) != 1) continue;
        const u32 w = kTusBlob33 + 4040 + 4 * static_cast<u32>(i >> 5);
        store<u32>(w, load<u32>(w) | (1u << (i & 31)));
    }
    TusSwap33(kTusBlob33);
    u32 res = 2;
    // the 9th argument (the option pointer, 0) goes on the stack at 112(r1)
    if (static_cast<s32>(call(ctx, TusSetData33, {static_cast<u32>(trans), npid, 1, 4056, 4056, kTusBlob33, 0, 392, 0})) < 0) res = 3;
    if (static_cast<s32>(call(ctx, TusDestroyTrans33, {static_cast<u32>(trans)})) < 0) res = 3;
    if (static_cast<s32>(call(ctx, TusDestroyTitle33, {static_cast<u32>(title)})) < 0) res = 3;
    store<u32>(kTusState33, res);
    call(ctx, ThreadExit33, {0, 0x00FA0000});
}

// The TUS download thread: reads slot 1 into the blob; "no data" errors
// (0x8002A10F, 0x8002A4AA..0x8002A4AC) record 2 / the code at 0x00F9D1B8 /
// 0x00F9D1BC. Any failure, or an online id that differs from the stored
// one, gives state 3. On success every stored trophy bit the player does
// not have yet is unlocked.
DECOMP(000B0FF8, TusDownload_Thread_000B0FF8) {
    Stack frame(ctx, 704);
    const u32 sp = frame.addr();
    const u32 npid = sp + 620, oid = sp + 600, status = sp + 112;
    if (static_cast<s32>(call(ctx, NpGetNpId33, {npid})) != 0) return TusFail33(ctx);
    const u32 comm = static_cast<u32>(call(ctx, TusCommId33, {0}));
    const u32 pass = static_cast<u32>(call(ctx, TusPass33, {0}));
    const s32 title = static_cast<s32>(call(ctx, TusCreateTitle33, {comm, pass, npid}));
    if (title < 0) return TusFail33(ctx);
    const s32 trans = static_cast<s32>(call(ctx, TusCreateTrans33, {static_cast<u32>(title)}));
    if (trans < 0) {
        store<u32>(kTusState33, 3);
        call(ctx, TusDestroyTitle33, {static_cast<u32>(title), 0x00FA0000, 3});
        call(ctx, ThreadExit33, {0});
        return;
    }
    call(ctx, Memset33, {status, 0, 488});
    u32 res = 2;
    const s32 e = static_cast<s32>(call(ctx, TusGetData33, {static_cast<u32>(trans), npid, 1, status, 488, kTusBlob33, 4056, 0}));
    if (e < 0) {
        const bool nodata = e > static_cast<s32>(0x8002A4A9u) ? e <= static_cast<s32>(0x8002A4ACu) : e == static_cast<s32>(0x8002A10Fu);
        if (nodata) {
            store<u32>(kTusNoData33, res);
            store<u32>(kTusErr33, static_cast<u32>(e));
        }
        res = 3;
    }
    if (static_cast<s32>(call(ctx, TusDestroyTrans33, {static_cast<u32>(trans)})) < 0) res = 3;
    if (static_cast<s32>(call(ctx, TusDestroyTitle33, {static_cast<u32>(title)})) < 0) res = 3;
    call(ctx, Memset33, {oid, 0, 20});
    if (static_cast<s32>(call(ctx, NpGetOnlineId33, {oid})) < 0) res = 3;
    if (static_cast<s32>(call(ctx, StrCompare33, {kTusBlob33 + 12, oid})) != 0) res = 3;
    store<u32>(kTusBlob33 + 8, 0);
    if (res == 2) {
        TusSwap33(kTusBlob33);
        for (s32 i = 0; i < 50; ++i) {
            if ((load<u32>(kTusBlob33 + 4040 + 4 * static_cast<u32>(i >> 5)) & (1u << (i & 31))) == 0) continue;
            if (static_cast<s32>(call(ctx, TrophyHas33, {load<u32>(kTrophyPtr33), static_cast<u32>(i)})) != 0) continue;
            call(ctx, TrophyUnlock33, {load<u32>(kTrophyPtr33), static_cast<u32>(i)});
        }
    }
    store<u32>(kTusState33, res);
    call(ctx, ThreadExit33, {0, 0x00FA0000});
}

namespace {
constexpr u32 kSaveCtx33 = 0x00F64584u;    // the save context pointer
constexpr u32 Memmove33 = 0x0071C3F0u;     // memmove(d, s, n)
constexpr u32 MemCompare33 = 0x00555DB8u;  // Mem_Compare(a, b, n)
constexpr u32 Strtol33 = 0x00721D18u;      // strtol(s, end, base)
constexpr u32 kSlotPrefix33 = 0x00AC6244u; // "BCUS98247_SAVE_SLOT_"
constexpr u32 kFmtSS33 = 0x00AC628Cu;      // "%s%s"
constexpr u32 kFmtSD33 = 0x00AC6294u;      // "%s - %d"
}  // namespace

// Save list callback (result, listGet, listSet): lists up to 2048 entries
// (focus 1); with fewer than 128 saves the next slot number is one past
// the highest "BCUS98247_SAVE_SLOT_nnnnn" found (the number before the last
// five characters' end), stored at ctx +7320 and formatted as five digits
// after the prefix ctx +7168 into ctx +7232; listSet +16 = the new-save
// entry ctx +7264 (directory +7232, title +8629, icon pointers from
// 0x0101278C / 0x01012790). result +0 = 0.
DECOMP(000B17C8, SaveList_Cb_000B17C8) {
    const u32 res = arg<u32>(ctx, 0), get = arg<u32>(ctx, 1), set = arg<u32>(ctx, 2);
    Stack frame(ctx, 336);
    const u32 sp = frame.addr();
    store<u32>(set + 12, load<u32>(get + 8));
    const u32 n = load<u32>(get + 4);
    store<u32>(set + 8, n > 0x800u ? 2048u : n);
    store<u32>(set + 0, 1);
    store<u32>(set + 4, 0);
    store<u32>(set + 20, 0);
    store<u32>(set + 16, 0);
    if (n < 0x80u) {
        const u32 pre = sp + 112, dig = sp + 136;
        call(ctx, Memmove33, {pre, kSlotPrefix33, 21});
        const u32 plen = static_cast<u32>(call(ctx, StrLen32, {pre}));
        s32 best = -1;
        for (s32 i = 0; i < static_cast<s32>(load<u32>(get + 4)); ++i) {
            const u32 off = 48 * static_cast<u32>(i);
            if (static_cast<s32>(call(ctx, MemCompare33, {load<u32>(get + 8) + off, pre, plen})) != 0) continue;
            const u32 len = static_cast<u32>(call(ctx, StrLen32, {load<u32>(get + 8) + off}));
            const s32 v = static_cast<s32>(call(ctx, Strtol33, {load<u32>(get + 8) + off + len - 5, 0, 10}));
            if (best < v) best = v;
        }
        const u32 g = load<u32>(kSaveCtx33);
        store<u32>(g + 7320, static_cast<u32>(best + 1));
        s32 x = static_cast<s32>(load<u32>(g + 7320));
        for (s32 k = 4; k >= 0; --k) {
            const s32 q = x / 10;
            store<u8>(dig + static_cast<u32>(k), static_cast<u8>(x - 10 * q + 48));
            x = q;
        }
        store<u8>(dig + 5, 0);
        store<u32>(set + 16, g + 7264);
        call(ctx, FormatN32, {g + 7232, 32, kFmtSS33, g + 7168, dig});
        const u32 c = load<u32>(kSaveCtx33);
        store<u32>(c + 7264, 0);
        store<u32>(c + 7280, c + 8629);
        store<u32>(c + 7268, c + 7232);
        store<u32>(c + 7272, c + 7280);
        store<u32>(c + 7288, load<u32>(0x0101278Cu));
        store<u32>(c + 7292, 0);
        store<u32>(c + 7284, load<u32>(0x01012790u));
    }
    store<u32>(res, 0);
    ctx.gpr[3] = 0;  // r3 as left
}

// Save stat callback (result, statGet, statSet, x, extra): fails with
// result +0 = -1, +8 = the missing space when the save (+1624 + extra -
// +1620) does not fit the free space +0. Otherwise statSet = {+0 the
// setParam +64, +4 = 3 when the attributes +1616 & 0x250 else 2}, ctx
// +7324 = x, ctx +7232 = the directory name +32; a new save (+4 == 1)
// clears its parameters (+1280 / +1284, +1296 256 bytes, +1288 = ctx
// +7328) and takes the slot ctx +7320, an existing one parses its slot
// from the name (+27 past the directory name's length). Title = ctx +7349,
// subtitle "%s - %d" (ctx +7477, slot + 1), detail ctx +7605; ctx +7344 =
// 0, result +0 = 0.
DECOMP(000B1A88, SaveStat_Cb_000B1A88) {
    const u32 res = arg<u32>(ctx, 0), get = arg<u32>(ctx, 1), set = arg<u32>(ctx, 2), x = arg<u32>(ctx, 3);
    const u32 extra = arg<u32>(ctx, 4);
    const s32 need = static_cast<s32>(load<u32>(get + 1624) + extra - load<u32>(get + 1620));
    const u32 attr = load<u32>(get + 1616);
    if (need > 0) {
        const s32 fr = static_cast<s32>(load<u32>(get + 0));
        if (fr < need && need - fr != 0) {
            store<u32>(res + 0, 0xFFFFFFFFu);
            store<u32>(res + 8, static_cast<u32>(need - fr));
            ctx.gpr[3] = ~0ull;  // r3 as left (li -1)
            return;
        }
    }
    const u32 g = load<u32>(kSaveCtx33);
    store<u32>(set + 4, (attr & 592u) != 0 ? 3u : 2u);
    store<u32>(set + 0, get + 64);
    store<u32>(g + 7324, x);
    call(ctx, StrAssign33, {g + 7232, get + 32});
    u64 slot;
    u32 p;
    if (load<u32>(get + 4) == 1) {
        const u32 s = load<u32>(set + 0);
        store<u32>(s + 1280, 0);
        call(ctx, StrAssign33, {s + 1288, load<u32>(kSaveCtx33) + 7328});
        p = load<u32>(set + 0);
        call(ctx, Memset33, {p + 1296, 0, 256});
        store<u32>(p + 1284, 0);
        slot = load<u32>(load<u32>(kSaveCtx33) + 7320);
    } else {
        const u64 len = call(ctx, StrLen32, {get + 32});
        slot = call(ctx, Strtol33, {static_cast<u32>(get + len + 27), 0, 10});
        p = load<u32>(set + 0);
    }
    const u64 num = static_cast<u64>(static_cast<s64>(static_cast<s32>(static_cast<u32>(slot + 1))));
    call(ctx, StrAssign33, {p, load<u32>(kSaveCtx33) + 7349});
    call(ctx, FormatN32, {load<u32>(set + 0) + 128, 128, kFmtSD33, load<u32>(kSaveCtx33) + 7477, num});
    call(ctx, StrCopyN33, {load<u32>(set + 0) + 256, load<u32>(kSaveCtx33) + 7605, 1024});
    store<u32>(load<u32>(kSaveCtx33) + 7344, 0);
    store<u32>(res + 0, 0);
    ctx.gpr[3] = 0;  // r3 as left
}
