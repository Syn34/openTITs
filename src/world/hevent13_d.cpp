// World_HandleEvent callees, part 13d: Entry_Load324B08 (called by
// Entry_Prepare324F58), which either starts a new effect for a user-slot
// entry or queues it as an effect scene with the entry's arguments.
#include "hevent13.h"

using namespace sly4;
using namespace sly4::hevent13;

namespace {
constexpr u32 kEffectDefFind = 0x004BDEDC;   // (u32 hash, u32 set, u8 search_all) -> def (symbols.txt)
constexpr u32 kEffectAddArgs = 0x0049FCC0;   // (Effect*, ArgVector*, 0, 0, 0) (name?)
constexpr u32 kSessionEntry2E2214 = 0x002E2214;  // (Session*) -> u32 or 0 (name?)
constexpr u32 kObjCanShow = 0x002E2104;      // Obj_CanShow2E2104 (Session*, u32) -> bool (symbols.txt)
constexpr u32 kSession2E2FD4 = 0x002E2FD4;   // (Session*) (name?)
constexpr u32 kWorldStartScene = 0x0046C3B8; // World_StartScene46C3B8 (symbols.txt)
constexpr u32 kPtrVectorInit = 0x00551598;   // (vec, count, 0, 0) (symbols.txt)
constexpr u32 kNop = 0x0054EC18;             // the vector's empty destructor (symbols.txt)

constexpr u32 kSession = 0x00F64C20;  // Session*: +40 Level* (+428 arg vector), +72 current entry
constexpr u32 kSesLevel = 40, kSesCurrent = 72, kLvArgs = 428;
constexpr u32 kEffectSize = 140, kEffectFlags = 136, kEffectFlagStarted = 0x200;
// UserBlock: +8 owner (World_StartScene's first argument), +12 {+80 vector
// of entries}, +18 u8 mode (6: 0x002E2FD4 first), +2456 s32 current index.
constexpr u32 kUbOwner = 8, kUbList = 12, kListEntries = 80, kUbMode = 18, kUbCurrent = 2456;
constexpr u8 kModeNotify = 6;
constexpr u32 kEntryState = 18;
constexpr u8 kEntryStateLast = 5;
constexpr s32 kSlotHold = 2;
// Stack vector of 76-byte argument records (as hevent8.h): +0 data, +4 u16
// capacity, +6 u16 element size, +8 u16 count, +10 u16 high-water mark.
constexpr u32 kVecData = 0, kVecCap = 4, kVecElem = 6, kVecCount = 8, kVecHigh = 10;
constexpr u32 kVecData_ = 0, kArgVecCount = 8;

// Appends a copy of the 74 bytes at src (as the inlined push).
void push_arg(PPUContext& ctx, u32 v, u32 src) {
    const u32 cap = load<u16>(v + kVecCap);
    if (load<u16>(v + kVecCount) == cap) call(ctx, hevent8::h8fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFE});
    const u32 c = load<u16>(v + kVecCount);
    call(ctx, hevent8::h8fn::Memmove, {load<u32>(v + kVecData) + c * hevent8::kArgSize, src, hevent8::kArgCopy});
    const u32 c1 = static_cast<u32>(load<u16>(v + kVecCount)) + 1;
    const u32 hw = load<u16>(v + kVecHigh);
    store<u16>(v + kVecCount, static_cast<u16>(c1));
    store<u16>(v + kVecHigh, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c1 & 0xFFFF) ? hw : c1));
}
}  // namespace

// Effect* Entry_Load324B08(UserBlock* ub, u32 hash, u32 set, ArgVector* args,
// u8 unique, s32 slot) (name?): 0 without a hash. Not unique: a new 140-byte
// effect of the def hash in set, given args and the session level's args
// (0x0049FCC0), flagged started (+136 |= 0x200, also after a failed
// allocation) and started; returns it. Unique: World_StartScene46C3B8 on ub
// +8 with a copy of args plus the level's args (or none when both are
// empty), held (r10) in slot 2 when ub's current entry is the last one in
// state 5, or the session's current entry cannot be shown; returns 0.
DECOMP(00324B08, Entry_Load324B08) {
    const u32 ub = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), set = arg<u32>(ctx, 2);
    const u32 args = arg<u32>(ctx, 3);
    const u8 unique = arg<u8>(ctx, 4);
    const s32 slot = arg<s32>(ctx, 5);
    if (hash == 0) return ret(ctx, 0u);
    if (unique == 0) {
        const u32 def = static_cast<u32>(call(ctx, kEffectDefFind, {hash, set, 1}));
        if (def == 0) return ret(ctx, 0u);
        const u32 a = static_cast<u32>(call(ctx, hevent7::h7fn::SmallAlloc, {kEffectSize}));
        u32 fx = 0;
        if (a != 0) {
            call(ctx, hevent7::h7fn::Effect_Construct, {a, def});
            fx = a;
        }
        call(ctx, kEffectAddArgs, {fx, args, 0, 0, 0});
        const u32 lv = load<u32>(load<u32>(kSession) + kSesLevel);
        if (lv != 0) call(ctx, kEffectAddArgs, {fx, lv + kLvArgs, 0, 0, 0});
        store<u32>(fx + kEffectFlags, load<u32>(fx + kEffectFlags) | kEffectFlagStarted);  // fx may be 0 (quirk)
        call(ctx, hevent7::h7fn::Effect_Start, {fx});
        return ret(ctx, fx);
    }
    const u32 list = load<u32>(ub + kUbList) + kListEntries;
    const s32 idx = load<s32>(ub + kUbCurrent);
    const s32 last_i = static_cast<s32>(static_cast<u32>(load<u16>(list + kArgVecCount)) - 1);
    const u32 cur = load<u32>(load<u32>(list + kVecData_) + (static_cast<u32>(idx) << 2));  // unchecked
    const bool last = idx == last_i && load<u8>(cur + kEntryState) == kEntryStateLast;
    u32 hold = 0;
    if (slot == kSlotHold) {
        if (last) {
            hold = 1;
        } else {
            const u32 ses = load<u32>(kSession);
            if (load<u32>(ses + kSesCurrent) == cur) {
                const u32 r = static_cast<u32>(call(ctx, kSessionEntry2E2214, {ses, cur}));
                if (r != 0 && static_cast<u32>(call(ctx, kObjCanShow, {load<u32>(kSession), r})) == 0) hold = 1;
            }
        }
    }
    if (load<u8>(ub + kUbMode) == kModeNotify) call(ctx, kSession2E2FD4, {load<u32>(kSession)});
    Stack frame(ctx, 16);  // r1+128: the argument vector
    const u32 v = frame.addr();
    const u32 lv = load<u32>(load<u32>(kSession) + kSesLevel);
    const u32 n = load<u16>(args + kArgVecCount);
    call(ctx, hevent9::h9fn::PtrVector_Construct, {v});
    store<u16>(v + kVecElem, static_cast<u16>(hevent8::kArgSize));
    call(ctx, kPtrVectorInit, {v, n, 0, 0});
    for (u32 i = 0, off = 0; i < load<u16>(args + kArgVecCount); ++i, off += hevent8::kArgSize)
        push_arg(ctx, v, load<u32>(args + kVecData_) + off);
    if (lv != 0) {
        const u32 la = lv + kLvArgs;
        for (u32 i = 0, off = 0; i < load<u16>(la + kArgVecCount); ++i, off += hevent8::kArgSize)
            push_arg(ctx, v, load<u32>(la + kVecData_) + off);
    }
    const u32 owner = load<u32>(ub + kUbOwner);
    const u32 pass = load<u16>(v + kVecCount) != 0 ? v : 0;
    call(ctx, kWorldStartScene, {owner, hash, set, 1, pass, 0, 0, hold, 0, 0});
    call(ctx, hevent::hfn::PtrVector_Clear, {v});
    call(ctx, kNop, {v});
    ret(ctx, 0u);
}
