// Game code, medium sweep part 27: an object's sound-off (shutdown)
// method and the streamed file reader's reset.
#include "mid27.h"

using namespace sly4;
using namespace sly4::mid27;

namespace {
using namespace m27fn;
constexpr u32 kEffDestruct = 0x004C8E80u;   // Effect_Destruct(e)
constexpr u32 kDtorTrivial = 0x004A0E08u;   // VtB21B58_DtorTrivial_004A0E08(p)
constexpr u32 kItemOf = 0x000D949Cu;        // f_000D949C(p) -> the item / player record
constexpr u32 kMemFreeFrom27e = 0x0054EE6Cu;

// Effect_Destruct(e) and SmallFree(e, 140) for a set *slot (cleared).
void DropEffect(PPUContext& ctx, u32 slot) {
    const u32 e = load<u32>(slot);
    if (e == 0) return;
    call(ctx, kEffDestruct, {e});
    call(ctx, SmallFree27, {e, 140});
    store<u32>(slot, 0);
}

// The world's player record (*0x00F65CA0 +4216) when its +192 has bit 8.
u32 FlaggedPlayer(u32& p) {
    p = load<u32>(load<u32>(mid25::kWorld25) + 4208 + 8);
    return p != 0 && (load<u32>(p + 192) & 8u) != 0 ? p : 0;
}

// Fs_Close(fd, {0, 1, 0, 0, 0, 0}) (the 24-byte argument in r4-r6).
inline void FsClose27(PPUContext& ctx, u32 fd) { call(ctx, 0x00561C78u, {fd, 1, 0, 0}); }
}  // namespace

// void f(T* o, u8 on) (a vtable method; no direct callers): when on, the
// 204-byte record +656 has its animator (+0) popped off the layer stack +8
// when Obj_HasWord368_000FC5DC says it is there (f1 = *0x0030CDC8, -1.0)
// and released (vt[4], cleared), its members +156 / +120 / +84 / +48
// destroyed and is freed (cleared); the twelve +704 slots get vt[4]
// (cleared); the records +920 are forgotten (Records_Forget_0030C2B4 on the
// first while the u16 +928 is set); the 192-byte +660 goes through
// f_0030B4AC, its PtrVectors +72 / +52 are destroyed and it is freed; the
// 112-byte +872 has its +0 released (vt[8]) and its effects +96 / +100
// destroyed and is freed; each entry of +876 (u16 +884, then 0) gets
// vt[40](e, 0) and, with bit 0x40000 of its +172, vt[40](e, 1); when the
// world's player record is flagged (+192 bit 8) and f_000D949C(it) is set,
// bit 0x08 of the u8 +2338 of f_000D949C(player again, or 0) is cleared;
// the effects +940 and +944..+952 destroyed; vt[60](o)->+36 gets +52 / +56
// cleared and Registry_Unlink_0048F298(o, 1). Then Obj_SoundOffBase(o, on).
DECOMP(0030CDD0, Obj_SoundOff_0030CDD0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        if (const u32 rec = load<u32>(o + 656); rec != 0) {
            if (const u32 a = load<u32>(rec); a != 0) {
                if (call(ctx, 0x000FC5DCu, {load<u32>(rec + 8), a}) != 0)
                    callf(ctx, Obj_PopLayer, {load<u32>(rec + 8), load<u32>(rec)}, {Lf(0x0030CDC8u)});
                if (const u32 a2 = load<u32>(rec); a2 != 0) {
                    VCall(ctx, a2, 4, {a2});
                    store<u32>(rec, 0);
                }
            }
            for (u32 off : {156u, 120u, 84u, 48u}) call(ctx, kDtorTrivial, {rec + off});
            call(ctx, SmallFree27, {rec, 204});
            store<u32>(o + 656, 0);
        }
        for (u32 k = 0; k < 12; ++k) {
            const u32 slot = o + 704 + 4 * k;
            if (const u32 e = load<u32>(slot); e != 0) {
                VCall(ctx, e, 4, {e});
                store<u32>(slot, 0);
            }
        }
        while (load<u16>(o + 928) != 0) call(ctx, 0x0030C2B4u, {o, load<u32>(load<u32>(o + 920))});
        if (const u32 r = load<u32>(o + 660); r != 0) {
            call(ctx, 0x0030B4ACu, {r});
            PtrVecDtor27(ctx, r + 72);
            PtrVecDtor27(ctx, r + 52);
            call(ctx, SmallFree27, {r, 192});
            store<u32>(o + 660, 0);
        }
        if (const u32 r = load<u32>(o + 872); r != 0) {
            Vt8Slot(ctx, r, true);
            DropEffect(ctx, r + 96);
            DropEffect(ctx, r + 100);
            call(ctx, SmallFree27, {r, 112});
            store<u32>(o + 872, 0);
        }
        for (u32 i = 0; i < load<u16>(o + 884); ++i) {
            const u32 e = load<u32>(load<u32>(o + 876) + 4 * i);
            VCall(ctx, e, 40, {e, 0});
            const u32 e2 = load<u32>(load<u32>(o + 876) + 4 * i);
            if ((load<u32>(e2 + 172) & 0x40000u) != 0) VCall(ctx, e2, 40, {e2, 1});
        }
        store<u16>(o + 884, 0);
        u32 p;
        if (FlaggedPlayer(p) != 0 && call(ctx, kItemOf, {p}) != 0) {
            const u32 r = static_cast<u32>(call(ctx, kItemOf, {FlaggedPlayer(p)}));
            store<u8>(r + 2338, static_cast<u8>(load<u8>(r + 2338) & ~0x08u));
        }
        DropEffect(ctx, o + 940);
        for (u32 k = 0; k < 3; ++k) DropEffect(ctx, o + 944 + 4 * k);
        const u32 w = load<u32>(static_cast<u32>(VCall(ctx, o, 60, {o})) + 36);
        store<u32>(w + 52, 0);
        store<u32>(w + 56, 0);
        call(ctx, 0x0048F298u, {o, 1});
    }
    call(ctx, 0x000E4544u, {o, on});
}

// void f(Reader* r) (VtB1D4B8_Dtor and the deleting version): flags 0x80 /
// 0x40 of u8 +28 cleared; the +24 streams at +16 and the stream +20 get
// vt[16]; the pending operation +100 is cancelled and synced
// (FsOp_Cancel / FsOp_Sync), its result handle (FsOp_ResultData, when +32
// is 1) closed unless 0 or the reader's own +96, destructed and freed
// (cleared); the handle +96 closed (cleared); the buffer +40 freed
// (cleared); +44 = 0, the PtrVector +76 cleared, +72 / +32 = 0,
// +104 = -1 (r3 = -1).
DECOMP(002EA464, FileReader_Reset_002EA464) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 28, static_cast<u8>(load<u8>(o + 28) & ~0xC0u));
    if (load<u32>(o + 24) != 0) {
        u32 i = 0;
        do {
            const u32 e = load<u32>(load<u32>(o + 16) + 4 * i);
            VCall(ctx, e, 16, {e});
            ++i;
        } while (i < load<u32>(o + 24));
    }
    if (const u32 e = load<u32>(o + 20); e != 0) VCall(ctx, e, 16, {e});
    if (const u32 x = load<u32>(o + 100); x != 0) {
        call(ctx, 0x00570C0Cu, {x});                 // FsOp_Cancel
        call(ctx, 0x00561450u, {load<u32>(o + 100)});  // FsOp_Sync
        if (load<u32>(o + 32) == 1) {
            const u32 h = static_cast<u32>(call(ctx, 0x005616C8u, {load<u32>(o + 100)}));  // FsOp_ResultData
            if (h != 0 && h != load<u32>(o + 96)) FsClose27(ctx, h);
        }
        if (const u32 y = load<u32>(o + 100); y != 0) {
            call(ctx, 0x00570DA8u, {y});  // FsOp_Destruct
            call(ctx, kMemFreeFrom27e, {y, 0});
        }
        store<u32>(o + 100, 0);
    }
    if (const u32 h = load<u32>(o + 96); h != 0) {
        FsClose27(ctx, h);
        store<u32>(o + 96, 0);
    }
    if (const u32 b = load<u32>(o + 40); b != 0) {
        call(ctx, kMemFreeFrom27e, {b, 0});
        store<u32>(o + 40, 0);
    }
    store<u32>(o + 44, 0);
    call(ctx, PtrVecClear, {o + 76});
    store<u32>(o + 72, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 104, 0xFFFFFFFFu);
    ctx.gpr[3] = 0xFFFFFFFFu;
}
