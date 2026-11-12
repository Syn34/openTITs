// Game code, medium sweep part 32: the streaming FTD file reader (vtable
// 0x00B1D4B8, destructor VtB1D4B8_Dtor, reset FileReader_Reset_002EA464).
//
// Reader layout as these functions use it:
//   +0 vtable (slot 8: Update(o; f1 dt, u8 block), slot 12: Begin(o))
//   +4 name, +16 / +24 child readers and their count, u8 +28 flags
//   (0x40 done, 0x80 toggled on each completed load), +32 state,
//   +36 path (built lazily), +40 / +44 / +48 buffer, capacity, read size,
//   +56 / +64 header words, +72 record count, +76 16-byte records
//   (+0 id, +4 size, +8 u64 file offset), u16 +84 records to sort,
//   +96 file handle, +100 pending FsOp, +104 current record.
// States: 0 idle, 1 opening, 2 header, 3 table, 4 index, 5 records,
// 6 children, 7 done, 8 error (set by Obj_Printf_002EA67C).
#include "mid32.h"

using namespace sly4;
using namespace sly4::mid32;

namespace {
using namespace m32fn;
constexpr u32 ObjPrintf32 = 0x002EA67Cu;     // Obj_Printf_002EA67C(o, fmt, ...): state 8
constexpr u32 FsReadAsync32 = 0x00561E74u;   // Fs_ReadAsync(handle, buf, size, u64 pos, u64 x3)
constexpr u32 FsOpSync32 = 0x00561450u;      // FsOp_Sync(op)
constexpr u32 FsOpWait32 = 0x005612B8u;      // FsOp_Wait5612B8(op)
constexpr u32 FsOpGetError32 = 0x00570C38u;  // FsOp_GetError(op)
constexpr u32 FsOpErrName32 = 0x00570C98u;   // FsOp_ErrorName(op)
constexpr u32 FsOpResult32 = 0x00570D54u;    // FsOp_GetResult(op): bytes transferred
constexpr u32 FsOpDestruct32 = 0x00570DA8u;  // FsOp_Destruct(op)
constexpr u32 FsOpData32 = 0x005616C8u;      // FsOp_ResultData(op): the opened handle
constexpr u32 FsClose32 = 0x00561C78u;       // Fs_Close(handle, u64 x3)
constexpr u32 FiosOpen32 = 0x00561714u;      // Fios_OpenAsync(path, mode, u64 x3)
constexpr u32 TableLoad32 = 0x002EB070u;     // Table_Load_002EB070(o)
constexpr u32 kFmtPath32 = 0x00AD947Cu;      // "%s/%s%s"
constexpr u32 kStreaming32 = 0x00AD9484u;    // "Streaming"
constexpr u32 kFtd32 = 0x00AD9494u;          // ".ftd"
constexpr u32 kErrCreate32 = 0x00AD949Cu;    // "Creation of async read op of FTD header failed on %s"
constexpr u32 kErrRead32 = 0x00AD94D4u;      // "Read of %llu bytes from %s failed: %s"
constexpr u32 kErrShort32 = 0x00AD94FCu;     // "Read of %llu bytes from %s failed: only read %llu"
constexpr u32 kErrVersion32 = 0x00AD9534u;   // "%s contains an unknown version (%llu)"
constexpr u32 kErrOpen32 = 0x00AD955Cu;      // "Async Open of %s failed: %s"
constexpr u32 kErrHandle32 = 0x00AD957Cu;    // "Async Open of %s did not return a file handle, ..."
constexpr u32 kErrFail32 = 0x00AD95BCu;      // "Failed to open %s"
constexpr u32 kModeRb32 = 0x00AD9160u;       // "rb"
constexpr u32 kCmpRecord32 = 0x00F360B8u;    // OPD of the record compare function

// The reader's path for messages: +36 when set, else the name of the open
// handle +96 when non-empty, else "Streaming/<+4>.ftd" formatted into a new
// buffer at +36 (Mem_Alloc(n, 0, 16), last byte cleared).
u32 FtdPath32(PPUContext& ctx, u32 o) {
    const u32 p = load<u32>(o + 36);
    if (p != 0) return p;
    const u32 h = load<u32>(o + 96);
    if (h != 0) {
        const u32 n = static_cast<u32>(call(ctx, NameIfSet32, {h}));
        if (n != 0 && load<u8>(n) != 0) return n;
    }
    const u64 lext = call(ctx, StrLen32, {kFtd32});
    const u64 ldir = call(ctx, StrLen32, {kStreaming32});
    const u64 lname = call(ctx, StrLen32, {load<u32>(o + 4)});
    const u64 n = ldir + lname + lext + 2;
    store<u32>(o + 36, static_cast<u32>(call(ctx, MemAlloc32, {n, 0, 16})));
    const u64 size = static_cast<u64>(static_cast<s64>(static_cast<s32>(static_cast<u32>(n))));
    call(ctx, FormatN32, {load<u32>(o + 36), size, kFmtPath32, kStreaming32, load<u32>(o + 4), kFtd32});
    store<u8>(static_cast<u32>(load<u32>(o + 36) + n) - 1, 0);
    return load<u32>(o + 36);
}

// The end of a load: (state 6 already stored) buffer +40 freed, +44 / +48 /
// +56 / +64 cleared and the handle +96 closed (Fs_Close(h, 1, 0, 0)) and
// cleared. Returns r3 as the original leaves it (Fs_Close's result, or 0).
u64 FtdFinishLoad32(PPUContext& ctx, u32 o) {
    const u32 buf = load<u32>(o + 40);
    if (buf != 0) {
        call(ctx, MemFree32, {buf, 0});
        store<u32>(o + 40, 0);
    }
    store<u32>(o + 44, 0);
    const u32 h = load<u32>(o + 96);
    store<u32>(o + 48, 0);
    store<u64>(o + 56, 0);
    store<u64>(o + 64, 0);
    if (h == 0) return 0;
    const u64 r = call(ctx, FsClose32, {h, 1, 0, 0});
    store<u32>(o + 96, 0);
    return r;
}

// FtdReader_StartRead_002EA728 on record i of the table +76.
u64 FtdReadRecord32(PPUContext& ctx, u32 o, u32 i) {
    const u32 rec = load<u32>(o + 76) + (i << 4);
    const u64 pos = load<u64>(rec + 8);
    return call(ctx, 0x002EA728u, {o, load<u32>(rec + 4), pos});
}

// The FsOp of state 1-5 has finished when (op->+0)->+152 is nonzero.
bool FtdOpReady32(u32 op) { return load<u32>(load<u32>(op) + 152) != 0; }

// vslot 8 (Update) of o with f1 = 0 and u8 block in r5; r4 carries the OPD
// as the original call sequence leaves it.
u64 FtdUpdate32(PPUContext& ctx, u32 o, u32 block) {
    const u32 opd = load<u32>(load<u32>(o) + 8);
    ctx.fpr[1] = 0.0;
    return call_ptr(ctx, opd, {o, opd, block, load<u32>(opd)});
}
}  // namespace

// bool FtdReader_StartRead(Reader* o, u32 size, u64 pos): +48 = size; the
// buffer +40 grows to size (old one freed, Mem_Alloc(size, 0, 16), +44 =
// size) when the capacity +44 is smaller; then +100 = Fs_ReadAsync(+96,
// +40, size, pos, 1, 0, 0). r3 = 1 when the read started, else the error
// "Creation of async read op ..." is reported (state 8) and r3 = 0.
DECOMP(002EA728, FtdReader_StartRead_002EA728) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 size = ctx.gpr[4], pos = ctx.gpr[5];
    store<u32>(o + 48, static_cast<u32>(size));
    u32 buf = load<u32>(o + 40);
    u64 n = size;
    if (load<u32>(o + 44) < static_cast<u32>(size)) {
        u64 want = size;
        if (buf != 0) {
            call(ctx, MemFree32, {buf, 0});
            want = load<u32>(o + 48);
            store<u32>(o + 40, 0);
        }
        buf = static_cast<u32>(call(ctx, MemAlloc32, {want, 0, 16}));
        n = load<u32>(o + 48);
        store<u32>(o + 40, buf);
        store<u32>(o + 44, static_cast<u32>(n));
    }
    const u32 op = static_cast<u32>(call(ctx, FsReadAsync32, {load<u32>(o + 96), buf, n, pos, 1, 0, 0}));
    store<u32>(o + 100, op);
    if (op != 0) {
        ret(ctx, 1u);
        return;
    }
    call(ctx, ObjPrintf32, {o, kErrCreate32, FtdPath32(ctx, o)});
    ret(ctx, 0u);
}

// bool FtdReader_FinishRead(Reader* o): waits for the pending read +100
// (FsOp_Sync). On an error or a short read the message ("Read of %llu bytes
// from %s failed: ...") is reported (state 8, +100 kept) and r3 = 0; else
// the FsOp is destroyed and freed, +100 = 0 and r3 = 1.
DECOMP(002EA92C, FtdReader_FinishRead_002EA92C) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, FsOpSync32, {load<u32>(o + 100)});
    if (static_cast<u32>(call(ctx, FsOpGetError32, {load<u32>(o + 100)})) != 0) {
        const u32 size = load<u32>(o + 48);
        const u32 path = FtdPath32(ctx, o);
        const u32 err = static_cast<u32>(call(ctx, FsOpErrName32, {load<u32>(o + 100)}));
        call(ctx, ObjPrintf32, {o, kErrRead32, size, path, err});
        ret(ctx, 0u);
        return;
    }
    const u32 size = load<u32>(o + 48);
    if (size != static_cast<u32>(call(ctx, FsOpResult32, {load<u32>(o + 100)}))) {
        const u32 want = load<u32>(o + 48);
        const u32 path = FtdPath32(ctx, o);
        const u32 got = static_cast<u32>(call(ctx, FsOpResult32, {load<u32>(o + 100)}));
        call(ctx, ObjPrintf32, {o, kErrShort32, want, path, got});
        ret(ctx, 0u);
        return;
    }
    const u32 op = load<u32>(o + 100);
    if (op != 0) {
        call(ctx, FsOpDestruct32, {op});
        call(ctx, MemFree32, {op, 0});
    }
    store<u32>(o + 100, 0);
    ret(ctx, 1u);
}

// void FtdReader_NextRecord(Reader* o) (state 5): after FtdReader_FinishRead,
// every child reader (+16[i], i < +24) whose id +4 matches the current
// record's id gets Begin (vslot 12, unless u8 child +28 is set) and vslot
// 8(child, 0). Then the next record is read (+104 = i, StartRead; state 5
// when that fails), or after the last one +104 = -1, state 6 and the load
// is finished (buffer freed, handle closed). r3 as the original leaves it.
DECOMP(002EAC2C, FtdReader_NextRecord_002EAC2C) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x002EA92Cu, {o})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 rec = load<u32>(o + 76) + (load<u32>(o + 104) << 4);
    u32 n = load<u32>(o + 24);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(o + 16) + 4 * i);
        if (load<u32>(e + 4) != load<u32>(rec)) continue;
        if (load<u8>(e + 28) == 0) VCall(ctx, e, 12, {e});
        VCall(ctx, e, 8, {e, 0});
        n = load<u32>(o + 24);
    }
    const u32 next = load<u32>(o + 104) + 1;
    if (next < load<u32>(o + 72)) {
        store<u32>(o + 104, next);
        const u64 r = FtdReadRecord32(ctx, o, next);
        if (static_cast<u32>(r) != 0) {
            ctx.gpr[3] = r;
            return;
        }
        store<u32>(o + 32, 5);
        ret(ctx, 5u);
        return;
    }
    store<u32>(o + 104, 0xFFFFFFFFu);
    store<u32>(o + 32, 6);
    ctx.gpr[3] = FtdFinishLoad32(ctx, o);
}

// void FtdReader_LoadIndex(Reader* o) (state 4): after FtdReader_FinishRead,
// the +72 words read into +40 are byte-swapped and become the sizes (+4) of
// the 16-byte records +76; the first u16 +84 records are sorted (qsort with
// the record compare), the buffer is resized to the largest size when the
// capacity +44 differs, and the records are read from +104 = 0 on (state 5).
// Without records, or when all reads failed to start, state 6 and the load
// is finished. r3 as the original leaves it.
DECOMP(002EAE24, FtdReader_LoadIndex_002EAE24) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x002EA92Cu, {o})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 buf = load<u32>(o + 40);
    if (load<u32>(o + 72) != 0) {
        for (u32 k = 0;;) {
            const u32 w = buf + 4 * k;
            ++k;
            const u32 b0 = load<u8>(w), b1 = load<u8>(w + 1), b2 = load<u8>(w + 2), b3 = load<u8>(w + 3);
            store<u32>(w, (b3 << 24) | (b2 << 16) | (b1 << 8) | b0);
            if (!(k < load<u32>(o + 72))) break;
        }
    }
    u32 most = 0;
    if (load<u32>(o + 72) != 0) {
        for (u32 i = 0;;) {
            const u32 rec = load<u32>(o + 76) + 16 * i;
            store<u32>(rec + 4, load<u32>(buf + 4 * i));
            const u32 v = load<u32>(buf + 4 * i);  // re-read after the store
            const u32 n = load<u32>(o + 72);
            if (!(most > v)) most = v;
            ++i;
            if (!(i < n)) break;
        }
    }
    call(ctx, Qsort32, {load<u32>(o + 76), load<u16>(o + 84), 16, kCmpRecord32});
    if (static_cast<s32>(load<u32>(o + 44)) != static_cast<s32>(most)) {
        const u32 old = load<u32>(o + 40);
        if (old != 0) {
            call(ctx, MemFree32, {old, 0});
            store<u32>(o + 40, 0);
        }
        store<u32>(o + 44, most);
        store<u32>(o + 40, static_cast<u32>(call(ctx, MemAlloc32, {most, 0, 16})));
    }
    const u32 count = load<u32>(o + 72);
    store<u32>(o + 104, 0);
    store<u32>(o + 32, 5);
    if (count != 0) {
        for (u32 i = 0;;) {
            const u64 r = FtdReadRecord32(ctx, o, i);
            if (static_cast<u32>(r) != 0) {
                ctx.gpr[3] = r;
                return;
            }
            i = load<u32>(o + 104) + 1;
            const u32 n = load<u32>(o + 72);
            store<u32>(o + 32, 5);
            store<u32>(o + 104, i);
            if (!(i < n)) break;
        }
    }
    store<u32>(o + 32, 6);
    ctx.gpr[3] = FtdFinishLoad32(ctx, o);
}

// void FtdReader_ReadHeader(Reader* o) (state 2): after FtdReader_FinishRead,
// the six header words in +40 are byte-swapped in place. Version (word 0)
// 1: +72 = word 5 records, +56 = word 3, +64 = word 1 and the record table
// (word 5 * 8 bytes at offset word 2) is read (state 3 when started); any
// other version reports "%s contains an unknown version (%llu)" (state 8).
// r3 as the original leaves it.
DECOMP(002EB234, FtdReader_ReadHeader_002EB234) {
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, 0x002EA92Cu, {o})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 h = load<u32>(o + 40);
    u32 w[6];
    for (u32 k = 0; k < 6; ++k) {
        const u32 a = h + 4 * k;
        const u32 b0 = load<u8>(a), b1 = load<u8>(a + 1), b2 = load<u8>(a + 2), b3 = load<u8>(a + 3);
        w[k] = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
        store<u32>(a, w[k]);
    }
    if (w[0] != 1) {
        const u32 path = FtdPath32(ctx, o);
        ctx.gpr[3] = call(ctx, ObjPrintf32, {o, kErrVersion32, path, load<u32>(h)});  // r3 as Obj_Printf_002EA67C leaves it
        return;
    }
    store<u32>(o + 72, w[5]);
    const u32 w3 = load<u32>(h + 12), w1 = load<u32>(h + 4);
    store<u64>(o + 56, w3);
    store<u64>(o + 64, w1);
    if (static_cast<u32>(call(ctx, 0x002EA728u, {o, w[5] << 3, load<u32>(h + 8)})) == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o + 32, 3);
    ret(ctx, 3u);
}

// void FtdReader_OnOpen(Reader* o) (state 1): waits for the open FsOp +100.
// On an error ("Async Open of %s failed: %s") or without a handle ("... did
// not return a file handle ...") state 8; else +96 = the handle, the FsOp is
// destroyed and freed (+100 = 0), the path +36 freed and cleared, and the
// 24-byte header read starts (state 2). r3 as the original leaves it.
DECOMP(002EB4EC, FtdReader_OnOpen_002EB4EC) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, FsOpSync32, {load<u32>(o + 100)});
    if (static_cast<u32>(call(ctx, FsOpGetError32, {load<u32>(o + 100)})) != 0) {
        const u32 path = FtdPath32(ctx, o);
        const u32 err = static_cast<u32>(call(ctx, FsOpErrName32, {load<u32>(o + 100)}));
        ctx.gpr[3] = call(ctx, ObjPrintf32, {o, kErrOpen32, path, err});  // r3 as Obj_Printf_002EA67C leaves it
        return;
    }
    const u32 h = static_cast<u32>(call(ctx, FsOpData32, {load<u32>(o + 100)}));
    store<u32>(o + 96, h);
    if (h == 0) {
        ctx.gpr[3] = call(ctx, ObjPrintf32, {o, kErrHandle32, FtdPath32(ctx, o)});  // r3 as Obj_Printf_002EA67C leaves it
        return;
    }
    const u32 op = load<u32>(o + 100);
    if (op != 0) {
        call(ctx, FsOpDestruct32, {op});
        call(ctx, MemFree32, {op, 0});
    }
    const u32 path = load<u32>(o + 36);
    store<u32>(o + 100, 0);
    if (path != 0) {
        call(ctx, MemFree32, {path, 0});
        store<u32>(o + 36, 0);
    }
    if (static_cast<u32>(call(ctx, 0x002EA728u, {o, 24, 0})) == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o + 32, 2);
    ret(ctx, 2u);
}

// void FtdReader_Update(Reader* o; f32 dt, u8 block) (vslot 8): one step of
// the load state machine; with block it keeps stepping (FsOp_Sync first)
// until the state is 0, 7 or 8. States 1-5 wait for the FsOp +100
// ((op->+0)->+152 set), FsOp_Wait5612B8 it and run OnOpen / ReadHeader /
// Table_Load_002EB070 / LoadIndex / NextRecord. State 6 updates the child
// readers (Begin when u8 +28 is clear, then vslot 8(child, block)) until
// one returns 0; when all ran, u8 +28 gets 0x40 set and 0x80 toggled and
// the state becomes 7 (0x40 is cleared otherwise). r3 as the original
// leaves it.
DECOMP(002EB7BC, FtdReader_Update_002EB7BC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 block = static_cast<u8>(ctx.gpr[5]);
    s32 st = static_cast<s32>(load<u32>(o + 32));
    u64 r3 = static_cast<u32>(st);
    for (;;) {
        // st as loaded; r3 is what reaches the block test below.
        if (st == 0 || st == 7 || st == 8) {
            ctx.gpr[3] = static_cast<u32>(st);
            return;
        }
        if (st >= 1 && st <= 5) {
            if (block != 0) call(ctx, FsOpSync32, {load<u32>(o + 100)});
            const u32 op = load<u32>(o + 100);
            r3 = op;
            if (FtdOpReady32(op)) {
                call(ctx, FsOpWait32, {op});
                const u32 step = st == 1 ? 0x002EB4ECu : st == 2 ? 0x002EB234u : st == 3 ? TableLoad32
                               : st == 4 ? 0x002EAE24u : 0x002EAC2Cu;
                r3 = call(ctx, step, {o});
            }
        } else if (st == 6) {
            u32 i = 0;
            if (load<u32>(o + 24) != 0) {
                for (;;) {
                    const u32 e = load<u32>(load<u32>(o + 16) + 4 * i);
                    if (load<u8>(e + 28) == 0) VCall(ctx, e, 12, {e});
                    const u32 e2 = load<u32>(load<u32>(o + 16) + 4 * i);
                    if (static_cast<u32>(VCall(ctx, e2, 8, {e2, block})) == 0) break;
                    const u32 n = load<u32>(o + 24);
                    ++i;
                    if (!(i < n)) break;
                }
            }
            const u32 done = i == load<u32>(o + 24) ? 1u : 0u;
            u32 b = load<u8>(o + 28);
            b = (b & ~0x40u) | (done << 6);
            b = (b & ~0x80u) | ((((b >> 7) & 1) ^ done) << 7);
            store<u8>(o + 28, static_cast<u8>(b));
            r3 = done;
            if (done != 0) {
                store<u32>(o + 32, 7);
                r3 = 7;
            }
        } else {
            r3 = static_cast<u32>(st);  // below -1 or above 8: no work
        }
        if (block == 0) {
            ctx.gpr[3] = r3;
            return;
        }
        st = static_cast<s32>(load<u32>(o + 32));
    }
}

// void FtdReader_Open(Reader* o, u8 block): in state 0 opens the path
// (Fios_OpenAsync(path, "rb", 1, 0, 0) into +100; "Failed to open %s" when
// it fails), state 1, and unless u8 +28 has 0x40: +28 gets 0x80 set and
// 0x40 cleared, the child +20 gets Begin (vslot 12, unless its u8 +28 is
// set) and Update (vslot 8, f1 = 0) runs once, or with block until 0x40 is
// set. In states other than 0, 7 and 8 only Update(o, 1) runs, with block.
// r3 as the original leaves it.
DECOMP(002EBAF8, FtdReader_Open_002EBAF8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 block = static_cast<u8>(ctx.gpr[4]);
    const s32 st = static_cast<s32>(load<u32>(o + 32));
    const u32 vt = load<u32>(o);
    if (st == 7 || st == 8) {
        ctx.gpr[3] = vt;
        return;
    }
    if (st != 0) {
        if (block == 0) {
            ctx.gpr[3] = vt;
            return;
        }
        const u32 opd = load<u32>(vt + 8);
        ctx.fpr[1] = 0.0;
        ctx.gpr[3] = call_ptr(ctx, opd, {o, opd, 1, load<u32>(opd)});
        return;
    }
    const u32 path = FtdPath32(ctx, o);
    const u32 op = static_cast<u32>(call(ctx, FiosOpen32, {path, kModeRb32, 1, 0, 0}));
    store<u32>(o + 100, op);
    if (op == 0) {
        ctx.gpr[3] = call(ctx, ObjPrintf32, {o, kErrFail32, path});  // r3 as Obj_Printf_002EA67C leaves it
        return;
    }
    store<u32>(o + 32, 1);
    const u32 b = load<u8>(o + 28);
    if ((b >> 6) & 1) {
        ret(ctx, b);
        return;
    }
    const u32 c = load<u32>(o + 20);
    store<u8>(o + 28, static_cast<u8>((b | 0x80) & ~0x40u));
    if (c != 0 && load<u8>(c + 28) == 0) VCall(ctx, c, 12, {c});
    for (;;) {
        FtdUpdate32(ctx, o, block);
        const u32 done = (load<u8>(o + 28) >> 6) & 1;
        if (done != 0 || block == 0) {
            ret(ctx, done);
            return;
        }
    }
}
