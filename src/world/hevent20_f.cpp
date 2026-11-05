// World_HandleEvent callees, part 20f: a global object list push, the coach
// time flag mask, a handle map notifier, the 22-argument init wrapper, the
// level shutdown memory report and the anim clip file loader.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kPtrVectorConstruct = 0x005516E0;  // (vec) (symbols.txt PtrVector_Construct)
constexpr u32 kPtrVectorInit = 0x00551598;       // (vec, capacity, 0, 0) (symbols.txt PtrVector_Init)
constexpr u32 kList = 0x010C2968;                // PtrVector of the registered objects
constexpr u32 kListReady = 0x010C2960;           // u8: kList constructed
constexpr u32 kCoachBits = 0x00ACA980;           // u32[107] bit per coach flag (rodata)
constexpr u32 kCoachFlags = 656;                 // Obj + u8[107]: bit 0x80 set, 0x40 clear -> counted
constexpr u32 kCoachCount = 107;
constexpr u32 kMap = 0x010686A8;                 // {+8 {key, Object*}[], +24 u32 count}
constexpr u32 kQueryInit = 0x000CDD28;           // (22 arguments; f1, f2) (symbols.txt Query_Init0CDD28)
constexpr u32 kSysLwmutexLock = 0x00ABF0E4, kSysLwmutexUnlock = 0x00ABF104;  // import stubs
constexpr u32 kPrintf = 0x007240F4;              // (fmt, ...) (symbols.txt printf)
constexpr u32 kHkLock = 0x0106BFC8;              // the Havok memory block's lwmutex (as hevent20_b.cpp)
constexpr u32 kHkHeap = 0x0106BFE0;              // u32 heap {+4 total, +20 free}
constexpr u32 kHkReady = 0x0106BFE4;             // u8
constexpr u32 kHkInUse = 0x0106BFE8, kHkPeak = 0x0106BFF0, kHkLevelPeak = 0x0106BFF4;
constexpr u32 kFmtInUse = 0x00ADC0C0;            // "gmHkMemory: LevelShutdown: Total:   InUse: %9dk\n"
constexpr u32 kFmtPeak = 0x00ADC0F8;             // "... Total: PeakUse: %9dk\n"
constexpr u32 kFmtMinFree = 0x00ADC130;          // "... Heap:  MinFree: %9dk\n"
constexpr u32 kFmtLevelPeak = 0x00ADC168;        // "... Level: PeakUse: %9dk\n"
constexpr u32 kFmtEnd = 0x00ADC1A0;              // the closing line
constexpr u32 kXmlDocConstruct = 0x004FA888;     // (doc, path) (symbols.txt)
constexpr u32 kXmlDocParse = 0x004FAF10;         // (doc, 0) -> bool (symbols.txt)
constexpr u32 kXmlDocDestruct = 0x004FA9C0;      // (doc) (symbols.txt)
constexpr u32 kStringFormat = 0x005534D0;        // (fmt, ...) (symbols.txt)
constexpr u32 kClipLoad4B20B8 = 0x004B20B8;      // (Owner*, XmlDoc*, const char* path) (name?)
constexpr u32 kFmtClipFail = 0x00AEE488;         // "Fail to load AnimClip file (%s)"
constexpr u32 kXmlDocBytes = 2240;               // the original frame's room for the document (112..2352)
}  // namespace

// void Effect_Start(Object* o) (name?): pushes o onto the vector at
// 0x010C2968, constructing it (capacity 16, +6 step 4) on first use.
DECOMP(004C753C, Effect_Start) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(kListReady) == 0) {
        call(ctx, kPtrVectorConstruct, {kList});
        store<u16>(kList + hevent18::kPvStep, 4);
        call(ctx, kPtrVectorInit, {kList, 16, 0, 0});
        store<u8>(kListReady, 1);
    }
    // The inlined PtrVector push (as hevent18::pv_push); r3 is left at the
    // high-water mark read before the update.
    using namespace hevent18;
    u32 n = load<u16>(kList + kPvCount);
    const u32 cap = load<u16>(kList + kPvCap);
    if (n == cap) {
        call(ctx, hevent8::h8fn::PtrVector_Reserve, {kList, (cap << 1) & 0x1FFFEu});
        n = load<u16>(kList + kPvCount);
    }
    store<u32>(load<u32>(kList + kPvData) + (n << 2), o);
    const u32 n1 = (load<u16>(kList + kPvCount) + 1) & 0xFFFF;
    const u32 hi = load<u16>(kList + kPvHigh);
    store<u16>(kList + kPvCount, static_cast<u16>(n1));
    store<u16>(kList + kPvHigh, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1) ? hi : n1));
    ctx.gpr[4] = n1;
    ret(ctx, hi);
}

// u32 Coach_TimeFlags12A71C(Obj* o) (name?): the OR of the bits (table
// 0x00ACA980) of the 107 flags o +656 that are set (0x80) and not done
// (0x40); then by vt +32 (o, 0): 7 clears 0x20000, 1 clears 0x10000.
DECOMP(0012A71C, Coach_TimeFlags12A71C) {
    const u32 o = arg<u32>(ctx, 0);
    u32 bits = 0;
    for (u32 i = 0; i < kCoachCount; ++i) {
        const u32 b = load<u8>(o + kCoachFlags + i);
        if ((b & 0x80) != 0 && (b & 0x40) == 0) bits |= load<u32>(kCoachBits + 4 * i);
    }
    const s32 mode = static_cast<s32>(vcall(ctx, o, 32, {o, 0}));
    if (mode == 7) bits &= ~0x00020000u;
    else if (mode == 1) bits &= ~0x00010000u;
    ret(ctx, bits);
}

// bool Map_Notify2E9C54(s32 key, s32 sub) (name?): every object of the map
// at 0x010686A8 whose +12 == key and (sub == 0 or +8 != sub) gets vt +12
// (obj, 1); the count is re-read after each. Returns 1.
DECOMP(002E9C54, Map_Notify2E9C54) {
    const s32 key = arg<s32>(ctx, 0), sub = arg<s32>(ctx, 1);
    u32 n = load<u32>(kMap + 24);
    for (u32 i = 0; i < n; ++i) {
        const u32 obj = load<u32>(load<u32>(kMap + 8) + 8 * i + 4);
        if (static_cast<s32>(load<u32>(obj + 12)) != key) continue;
        if (sub != 0 && static_cast<s32>(load<u32>(obj + 8)) == sub) continue;
        vcall(ctx, obj, 12, {obj, 1});
        n = load<u32>(kMap + 24);
    }
    ret(ctx, 1);
}

// void Obj_Init22(r3..r10, a8, u8 a9..a16, a17, a18, -, -, a21; f1, f2)
// (symbols.txt): forwards to Query_Init0CDD28 with arguments 9..16 masked to
// bytes. The original leaves argument slots 19 / 20 unwritten (unread).
DECOMP(000CDFB0, Obj_Init22) {
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    auto sa = [sp](u32 n) { return static_cast<u64>(load<u32>(sp + 112 + 8 * (n - 8) + 4)); };
    callf(ctx, kQueryInit,
         {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], ctx.gpr[9], ctx.gpr[10], sa(8),
          sa(9) & 0xFF, sa(10) & 0xFF, sa(11) & 0xFF, sa(12) & 0xFF, sa(13) & 0xFF, sa(14) & 0xFF, sa(15) & 0xFF,
          sa(16) & 0xFF, sa(17), sa(18), 0, 0, sa(21)},
         {farg(ctx, 0), farg(ctx, 1)});  // f1 / f2 passed through
}

// void HkMemory_Report355204() (name?): under the lwmutex at 0x0106BFC8,
// when the Havok heap exists: u8 0x0106BFE4 = 0 and the level shutdown
// memory report printed (in KB).
DECOMP(00355204, HkMemory_Report355204) {
    call(ctx, kSysLwmutexLock, {kHkLock, 100000});
    if (load<u32>(kHkHeap) != 0) {
        store<u8>(kHkReady, 0);
        call(ctx, kPrintf, {kFmtInUse, load<u32>(kHkInUse) >> 10});
        call(ctx, kPrintf, {kFmtPeak, load<u32>(kHkPeak) >> 10});
        const u32 h = load<u32>(kHkHeap);
        call(ctx, kPrintf, {kFmtMinFree, (load<u32>(h + 4) - load<u32>(h + 20)) >> 10});
        call(ctx, kPrintf, {kFmtLevelPeak, load<u32>(kHkLevelPeak) >> 10});
        call(ctx, kPrintf, {kFmtEnd});
    }
    call(ctx, kSysLwmutexUnlock, {kHkLock});
}

// void Clips_Load4B229C(Owner* o) (name?): every anim clip file name of
// o +96 (count +224, re-read) is parsed as an XmlDoc and loaded
// (0x004B20B8); a failure only formats "Fail to load AnimClip file (%s)".
DECOMP(004B229C, Clips_Load4B229C) {
    const u32 o = arg<u32>(ctx, 0);
    u32 n = load<u32>(o + 224);
    if (n == 0) {
        ret(ctx, 0);
        return;
    }
    Stack frame(ctx, kXmlDocBytes);
    const u32 doc = static_cast<u32>(ctx.gpr[1]);
    for (u32 i = 0;;) {
        const u32 name = load<u32>(o + 96 + 4 * i);
        call(ctx, kXmlDocConstruct, {doc, name});
        if (static_cast<u32>(call(ctx, kXmlDocParse, {doc, 0})) != 0) {
            call(ctx, kClipLoad4B20B8, {o, doc, name});
        } else {
            call(ctx, kStringFormat, {kFmtClipFail, name});
        }
        call(ctx, kXmlDocDestruct, {doc});
        ++i;
        n = load<u32>(o + 224);
        if (!(i < n)) break;
    }
    ret(ctx, n);
}
