// World_HandleEvent callees, part 10m: the level loader state machine.
#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10m {
namespace mfn10 {
constexpr u32 printf_ = 0x007240F4;              // libc printf
constexpr u32 Level_StartLoad2DA2A4 = 0x002DA2A4;  // (Level*) (name?)
constexpr u32 Level_Update2DA438 = 0x002DA438;   // (Level*) (name?)
constexpr u32 Level_Release2D9A14 = 0x002D9A14;  // (Level*) (name?)
constexpr u32 Handle_ResolveBit1 = 0x003568E0;   // (u32 hash) -> Object*
constexpr u32 Handle_Call363A3C = 0x0036735C;    // (hash, 0, 1, -, 0, 0, OPD; f32) (symbols.txt name)
constexpr u32 Handle_Notify36742C = 0x0036742C;  // (hash, 0, 1, 0) (name?)
constexpr u32 sub_002E0930 = 0x002E0930;         // 9 args, the last on the stack
constexpr u32 User_Prepare3265A4 = 0x003265A4;   // (UserBlock*, s32 slot) -> ready (name?)
constexpr u32 User_Start325B58 = 0x00325B58;     // (UserBlock*, s32 slot) (name?)
constexpr u32 Loader_Finish2E0498 = 0x002E0498;  // (Loader*, UserBlock*, 0, 0) (name?)
constexpr u32 Obj_Notify24 = 0x000D949C;         // (Object*) -> Object*
constexpr u32 Obj_UpdateFlags2336 = 0x0029D838;  // (Object*, u8)
constexpr u32 Loader_Tick3250A0 = 0x003250A0;    // (x, 1) (name?)
}  // namespace mfn10

// Loader: +0 s32 state (1 loading, 2 job, 3 users), +4 Object*, +8
// Level* (+508 s32 load state), +12 u32, +16 flags (bit 31 / 30: job
// arguments, bit 29: active), +48 u32 user id, +72 UserBlock*, +76 x, +100
// flags (bit 28 set when done).
constexpr u32 kLdState = 0, kLdObj = 4, kLdLevel = 8, kLd12 = 12, kLdFlags = 16, kLdUserId = 48;
constexpr u32 kLdUser = 72, kLdTick = 76, kLdFlags100 = 100;
constexpr u32 kLdActive = 0x20000000u, kLdJobA = 0x80000000u, kLdJobB = 0x40000000u;
constexpr u32 kLevelLoad = 508;
constexpr u32 kLoaderHash = 0x24D73841;   // handle of the loading screen object (name?)
constexpr u32 kLoaderOpd = 0x01164CA4;    // OPD given to Handle_Call363A3C (name?)
constexpr u32 kMinusOne = 0x002E0C30;     // f32 -1.0 (code segment)
constexpr u32 kSessionPtr = 0x00F64C20;   // Session*: +40 u32
// "StartAsyncLoad\n", "UpdateLoad\n", "UpdateXML\n", "StartJob\n".
constexpr u32 kStrStartAsyncLoad = 0x00AD8C84, kStrUpdateLoad = 0x00AD8C94;
constexpr u32 kStrUpdateXml = 0x00AD8CA4, kStrStartJob = 0x00AD8CB4;
}  // namespace hevent10m
}  // namespace sly4

using namespace sly4::hevent10m;

namespace {
// The world's user block (World +4208, as UserBlock_Find465F30): the
// loader's own one, else the one with its id, else the first.
u32 pick_user(u32 l) {
    const u32 w = load<u32>(kCurrentWorld) + kUserBlocks;
    u32 u = load<u32>(l + kLdUser);
    if (u == 0) {
        const u32 id = load<u32>(l + kLdUserId);
        if (id != 0) {
            const u32 n = load<u16>(w + 88);
            for (u32 i = 0; i < n; ++i) {
                const u32 e = load<u32>(load<u32>(w + 80) + i * 4);
                if (load<u32>(e + 2560) == id) {
                    u = e;
                    break;
                }
            }
        }
        if (u == 0 && load<u16>(w + 88) != 0) u = load<u32>(load<u32>(w + 80));
    }
    return u;
}
}  // namespace

// u32 Loader_Update2E0C38(Loader* l) (name?): while active (flag bit 29):
// state 1 starts / polls the level load ("StartAsyncLoad" / "UpdateLoad")
// and moves to 2 once the loading object is gone or shown (+160 state 2 /
// 3), re-showing it (Handle_Call363A3C) when it lost its parent's bit 15;
// state 2 polls the XML step ("UpdateXML") or starts the job ("StartJob",
// sub_002E0930) and starts the user block's five slots, moving to 3; state
// 3 waits for all five slots, then finishes: users reset, level released,
// the object notified, the loading object released, the loader cleared.
// Always ticks +76 (0x003250A0) last. r3 / f1 as the original leaves them.
DECOMP(002E0C38, Loader_Update2E0C38) {
    const u32 l = arg<u32>(ctx, 0);
    u64 r3 = 0;
    const s32 st = static_cast<s32>(load<u32>(l + kLdState));
    if ((load<u32>(l + kLdFlags) & kLdActive) == 0) {
        r3 = 0;
    } else if (st == 1) {
        const s32 m = static_cast<s32>(load<u32>(load<u32>(l + kLdLevel) + kLevelLoad));
        if (m == 0) {
            call(ctx, mfn10::printf_, {kStrStartAsyncLoad});
            call(ctx, mfn10::Level_StartLoad2DA2A4, {load<u32>(l + kLdLevel)});
        } else if (m == 1) {
            call(ctx, mfn10::printf_, {kStrUpdateLoad});
            call(ctx, mfn10::Level_Update2DA438, {load<u32>(l + kLdLevel)});
        }
        const u32 o = static_cast<u32>(call(ctx, mfn10::Handle_ResolveBit1, {kLoaderHash}));
        if (o == 0) {
            store<u32>(l + kLdState, 2);
            r3 = 2;
        } else {
            const u32 parent = load<u32>(o + 24);
            if (parent == 0 || (load<u32>(parent + 164) & 0x8000) == 0) {
                // r6 is not an argument of Handle_Call363A3C.
                callf(ctx, mfn10::Handle_Call363A3C, {kLoaderHash, 0u, 1u, 0u, 0u, 0u, kLoaderOpd}, {lfs_f64(kMinusOne)});
                r3 = ctx.gpr[3];
            } else {
                const u32 s = load<u32>(o + 160) >> 1;
                r3 = s;
                if (s == 2 || s == 3) {
                    store<u32>(l + kLdState, 2);
                    r3 = 2;
                }
            }
        }
    } else if (st == 2) {
        if (static_cast<s32>(load<u32>(load<u32>(l + kLdLevel) + kLevelLoad)) == 1) {
            call(ctx, mfn10::printf_, {kStrUpdateXml});
            r3 = call(ctx, mfn10::Level_Update2DA438, {load<u32>(l + kLdLevel)});
        } else {
            call(ctx, mfn10::printf_, {kStrStartJob});
            const u32 f = load<u32>(l + kLdFlags);
            call(ctx, mfn10::sub_002E0930, {load<u32>(kSessionPtr), load<u32>(l + kLdLevel), load<u32>(l + kLd12),
                                            (f & kLdJobA) != 0 ? 1u : 0u, (f & kLdJobB) != 0 ? 1u : 0u, 0u, 0u, 0u, 0u});
            const u32 u = pick_user(l);
            if (u != 0)
                for (s32 i = 0; i < 5; ++i) call(ctx, mfn10::User_Start325B58, {u, static_cast<u64>(static_cast<s64>(i))});
            store<u32>(l + kLdState, 3);
            r3 = 3;
        }
    } else if (st == 3) {
        const u32 u = pick_user(l);
        bool done = true;
        if (u != 0) {
            for (s32 i = 0; i < 5; ++i) {
                if (static_cast<u32>(call(ctx, mfn10::User_Prepare3265A4, {u, static_cast<u64>(static_cast<s64>(i))})) == 0) {
                    done = false;
                    break;
                }
            }
            if (done) call(ctx, mfn10::Loader_Finish2E0498, {l, u, 0u, 0u});
        }
        if (!done) {
            r3 = 1;
        } else {
            store<u32>(l + kLdUserId, 0);
            if (load<u32>(load<u32>(kSessionPtr) + 40) != 0) {
                const u32 w = load<u32>(kCurrentWorld) + kUserBlocks;
                for (u32 i = 0; i < load<u16>(w + 88); ++i) {
                    const u32 e = load<u32>(load<u32>(w + 80) + i * 4);
                    if (e == 0) continue;
                    vcall(ctx, e, 32, {e});
                    vcall(ctx, e, 36, {e, 0u});
                }
            }
            call(ctx, mfn10::Level_Release2D9A14, {load<u32>(l + kLdLevel)});
            store<u32>(l + kLdFlags100, load<u32>(l + kLdFlags100) | 0x10000000u);
            const u32 o = load<u32>(l + kLdObj);
            if (o != 0) {
                const u32 r = static_cast<u32>(call(ctx, mfn10::Obj_Notify24, {o}));
                if (r != 0) {
                    vcall(ctx, r, 44, {r, 1u});
                    call(ctx, mfn10::Obj_UpdateFlags2336, {r, 0u});
                }
            }
            call(ctx, mfn10::Handle_Notify36742C, {kLoaderHash, 0u, 1u, 0u});
            const u32 o2 = load<u32>(l + kLdObj), lv = load<u32>(l + kLdLevel);
            if (o2 != 0) store<u64>(o2 + 184, load<u64>(o2 + 184) & ~0x20000ull);
            if (lv != 0) call(ctx, mfn10::Level_Release2D9A14, {lv});
            for (u32 k = 0; k < 20; k += 4) store<u32>(l + k, 0);
            r3 = 0;
        }
    } else {
        r3 = static_cast<u32>(st);
    }
    const u32 x = load<u32>(l + kLdTick);
    if (x != 0) r3 = call(ctx, mfn10::Loader_Tick3250A0, {x, 1u});
    ctx.gpr[3] = r3;
}
