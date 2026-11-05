// World_HandleEvent callees, part 17d: world event recording, the coach
// page show, player ability flags.
#include "hevent17.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent17;

namespace {
// Event record (32 bytes): +0 s32 kind (1: object event), +12 u32 id.
constexpr u32 kRecSize = 32, kRecKind = 0, kRecId = 12, kKindObject = 1;
// World: +0x128224 Watcher* (+344 / +348 Object* (+168 id, +172 bit 18
// 0x40000) / +352 u32 bit 31); +50 u8 mode, +45 u8 flags (bit 2: record),
// +0x128214 u16 replay count; vt +32 state(w, u8).
constexpr u32 kWorldWatcher = 0x128224, kWatchA = 344, kWatchObj = 348, kWatchFlags = 352;
constexpr u32 kObjId = 168, kObjFlags172 = 172, kObjWatched = 0x40000;
constexpr u32 kWorldMode = 50, kWorldFlags45 = 45, kWorldRecord = 0x04, kWorldReplayCount = 0x128214;
constexpr u32 kWorldVtState = 32;
constexpr u8 kModeReplay = 2;
// Rings of event records: world-wide in user block 0 (+0x20474, 64
// records, head +0x20C74, tail +0x20C76) and per user (+112, 4096 records,
// head +0x20070, tail +0x20072).
constexpr u32 kRingA = 0x20474, kRingAHead = 0x20C74, kRingATail = 0x20C76, kRingASize = 64;
constexpr u32 kRingU = 112, kRingUHead = 0x20070, kRingUTail = 0x20072, kRingUSize = 4096;
constexpr u32 kWorldNumUsers = 29;      // World +29 u8 user count (of the current world)
constexpr u32 kEventSink = 0x00F64C20;  // u32 sink
constexpr u32 kSinkPost2E5180 = 0x002E5180;  // (sink, u32 user, const Record*) (name?)

// Coach page: Pages (world +0x128168) +656 Panel* (+0 Widget*, +4 Widget*);
// page record: +0 u8 rgba, +4 f32, +8 f32, +12 u8, +13 u8 alternate.
constexpr u32 kPagesPanel = 656;
constexpr u32 kPageRgba = 0, kPageF4 = 4, kPageF8 = 8, kPageB12 = 12, kPageAlt = 13;
constexpr u32 kTexFindLoaded = 0x0050C398;      // (u32 hash) -> Texture* (symbols.txt)
constexpr u32 kObjSetSlot20 = 0x004FEF48;       // (Material*, Texture*, u32 slot) (symbols.txt)
constexpr u32 kMaterialParamSlot = 0x0050C018;  // (Material*, u32 index) -> u32* (symbols.txt)
constexpr u32 kTexA = 0xAAECABB7, kTexB = 0x8FEAB734, kTexDefault = 0x67F7549B;
// Widget vt +8 show(w, u8).
constexpr u32 kWidgetVtShow = 8;
// Coach: 4 icons {+2516 u32 texture hash (8-byte stride)}, +2544 Material*[4].
constexpr u32 kCoachIconTex = 2516, kCoachIconMat = 2544, kCoachNumIcons = 4;
constexpr u32 kF8 = 0x41000000, kF075 = 0x3F400000, kF1 = 0x3F800000, kF15 = 0x3FC00000, kF035 = 0x3EB33333;

// Player abilities: +656 u8[107] (bit 7: enabled, 132 bytes cleared), +768
// / +776 / +784 s16 = -1; +172 s8 user; +188 bit 0; +216 def (+1360 bit
// set, +192 u32 level hash).
constexpr u32 kPlAbil = 656, kPlAbilBytes = 132, kNumAbil = 107, kAbilOn = 0x80;
constexpr u32 kPlUser = 172, kPlFlags188 = 188, kPlDef = 216, kDefAbilBits = 1360, kDefLevel = 192;
// World +0x128164: P* -> vector {+0 60-byte user records (+40 bit set), +8
// u16 count}.
constexpr u32 kWorldUserAbil = 0x128164, kUserAbilSize = 60, kUserAbilBits = 40;
// The ability table (107 records of 28 bytes, +8 u32 id).
constexpr u32 kAbilTable = 0x00AC9A14, kAbilRecSize = 28, kAbilId = 8;
constexpr u32 kLevelAbil49E43C = 0x0049E43C;  // (u32 level, PtrVector* out ids) (name?)
constexpr u32 kNop = 0x0054EC18;              // Nop (symbols.txt)
// Save slot ability ids by level.
constexpr u32 kLevelA = 0x1D8C2817, kLevelNone = 0x00014120, kLevelB = 0xDF0611F9, kLevelC = 0x88397820;
constexpr u32 kSaveAbilA = 300, kSaveAbilB = 304, kSaveAbilC = 296;
}  // namespace

// u32 World_Query482D14(World* w, u32 user, const Record* r) (name?): drops
// the watcher's object when an object event names it, records r in the
// world ring (when recording and replays exist) or, in a non-replay state
// change (states 1 -> 2 / 3) without replays, in the user's ring (for a
// valid user) and posts it to the event sink.
DECOMP(00482D14, World_Query482D14) {
    const u32 w = arg<u32>(ctx, 0), user = arg<u32>(ctx, 1), r = arg<u32>(ctx, 2);
    if (static_cast<s32>(load<u32>(r + kRecKind)) == static_cast<s32>(kKindObject)) {
        const u32 wt = load<u32>(w + kWorldWatcher);
        if (wt != 0) {
            const u32 o = load<u32>(wt + kWatchObj);
            const u32 id = load<u32>(r + kRecId);
            if (o != 0 && (load<u32>(o + kObjFlags172) & kObjWatched) != 0 && load<u32>(o + kObjId) == id) {
                const u32 f = load<u32>(wt + kWatchFlags);
                store<u32>(wt + kWatchA, 0);
                store<u32>(wt + kWatchObj, 0);
                store<u32>(wt + kWatchFlags, f & 0x7FFFFFFFu);
            }
        }
    }
    bool post = false;
    if (load<u8>(w + kWorldMode) != kModeReplay && static_cast<s32>(vcall(ctx, w, kWorldVtState, {w, 0})) == 1) {
        if (static_cast<s32>(vcall(ctx, w, kWorldVtState, {w, 1})) == 2) post = true;
        else if (static_cast<s32>(vcall(ctx, w, kWorldVtState, {w, 1})) == 3) post = true;
    }
    if (post && load<u16>(w + kWorldReplayCount) == 0) {
        if (user < load<u8>(load<u32>(kCurrentWorld) + kWorldNumUsers)) {
            const u32 ub = w + user * kUserBlockSize + kUserBlocks;
            const u32 e = ub + (static_cast<u32>(load<u16>(ub + kRingUHead)) << 5) + kRingU;
            for (u32 k = 0; k < kRecSize; k += 4) store<u32>(e + k, load<u32>(r + k));
            const u32 n = (load<u16>(ub + kRingUHead) + 1u) % kRingUSize;
            store<u16>(ub + kRingUHead, static_cast<u16>(n));
            if (n == load<u16>(ub + kRingUTail)) store<u16>(ub + kRingUTail, static_cast<u16>((n + 1) % kRingUSize));
        }
        call(ctx, kSinkPost2E5180, {load<u32>(kEventSink), user, r});
        return;
    }
    if (load<u16>(w + kWorldReplayCount) == 0) return ret(ctx, 0u);
    if ((load<u8>(w + kWorldFlags45) & kWorldRecord) == 0) return ret(ctx, 0u);
    const u32 ub = w + kUserBlocks;
    const u32 e = ub + (static_cast<u32>(load<u16>(ub + kRingAHead)) << 5) + kRingA;
    for (u32 k = 0; k < kRecSize; k += 4) store<u32>(e + k, load<u32>(r + k));
    const u32 n = (load<u16>(ub + kRingAHead) + 1u) % kRingASize;
    store<u16>(ub + kRingAHead, static_cast<u16>(n));
    if (n == load<u16>(ub + kRingATail)) store<u16>(ub + kRingATail, static_cast<u16>((n + 1) % kRingASize));
    ret(ctx, ub);
}

// u32 Coach_Show2A1290(Coach* c, Pages* pg, const PageRec* page) (name?):
// styles the panel's first widget from the page record (textures, colour,
// sizes), shows the second one (alternate layout when +13 is set) and
// sets up the 4 icon materials, then shows the first widget.
DECOMP(002A1290, Coach_Show2A1290) {
    const u32 c = arg<u32>(ctx, 0), pg = arg<u32>(ctx, 1), page = arg<u32>(ctx, 2);
    const u32 tex_a = static_cast<u32>(call(ctx, kTexFindLoaded, {kTexA}));
    const u32 tex_b = static_cast<u32>(call(ctx, kTexFindLoaded, {kTexB}));
    const u32 panel = load<u32>(pg + kPagesPanel) + 4;  // the original indexes from +4
    const u32 x = load<u32>(panel);
    const u32 f8 = load<u32>(page + kPageF8);  // lfs / stfs pairs
    ctx.fpr[1] = hevent7::lfs_f64(page + kPageF4);  // lfs f1 (the callees below may change it)
    store<u32>(x + 24, tex_b);
    store<u32>(x + 28, tex_a);
    store<u8>(x + 32, load<u8>(page + kPageB12));
    store<u32>(x + 48, load<u32>(page + kPageF4));
    store<u32>(x + 104, f8);
    for (u32 j = 0; j < 4; ++j) store<u8>(x + 33 + j, load<u8>(page + kPageRgba + j));
    store<u32>(x + 72, kF8);
    store<u32>(x + 76, kF8);
    store<u32>(x + 80, kF075);
    store<u32>(x + 84, kF1);
    store<u32>(x + 96, kF075);
    store<u32>(x + 100, kF1);
    const u32 y = load<u32>(panel + 4);
    if (load<u8>(page + kPageAlt) != 0) {
        store<u32>(y + 48, 0x41087AE1);  // 8.53
        store<u32>(y + 32, 0x420C0000);  // 35.0
        store<u32>(y + 36, 0x42F00000);  // 120.0
        store<u32>(y + 40, 0x429A0000);  // 77.0
        vcall(ctx, y, kWidgetVtShow, {y, 1});
    } else {
        vcall(ctx, y, kWidgetVtShow, {y, 0});
    }
    for (u32 j = 0; j < kCoachNumIcons; ++j) {
        const u32 h = load<u32>(c + kCoachIconTex + 8 * j);
        const u32 t = static_cast<u32>(call(ctx, kTexFindLoaded, {h != 0 ? h : kTexDefault}));
        const u32 m = load<u32>(c + kCoachIconMat + 4 * j);
        call(ctx, kObjSetSlot20, {m, tex_a, 1});
        call(ctx, kObjSetSlot20, {m, tex_b, 2});
        call(ctx, kObjSetSlot20, {m, t, 3});
        static constexpr u32 kParams[11] = {kF8, kF8, kF075, kF1, kF075, kF1, 0, kF1, kF15, kF035, kF1};
        for (u32 k = 2; k <= 12; ++k) {
            const u32 slot = static_cast<u32>(call(ctx, kMaterialParamSlot, {m, k}));
            store<u32>(slot, k == 8 ? f8 : kParams[k - 2]);
        }
    }
    vcall(ctx, x, kWidgetVtShow, {x, 1});
}

// u32 Player_ResetAbilities1205FC(Player* p) (name?; was
// Player_ResetInput1205FC): rebuilds the ability flags (+656) from the def's
// bit set (+1360) and, when +188 bit 0 is set, from the user's record and the
// level's ability list (plus the save slot's per-level ability for three
// levels); the three s16 slots +768 / +776 / +784 are reset to -1. A
// per-level ability missing from the table sets bit 7 of p +655 (quirk).
DECOMP(001205FC, Player_ResetAbilities1205FC) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 d = load<u32>(p + kPlDef);
    call(ctx, fn::memset, {p + kPlAbil, 0, kPlAbilBytes});
    auto bit = [](u32 bits, u32 k) { return (load<u32>(bits + (k >> 5) * 4) & (1u << (k & 31))) != 0; };
    auto set_on = [&](u32 k) { store<u8>(p + kPlAbil + k, static_cast<u8>(load<u8>(p + kPlAbil + k) | kAbilOn)); };
    for (u32 k = 0; k < kNumAbil; ++k) {
        const u8 b = load<u8>(p + kPlAbil + k);
        store<u8>(p + kPlAbil + k, static_cast<u8>((b & ~kAbilOn) | (bit(d + kDefAbilBits, k) ? kAbilOn : 0u)));
    }
    store<u16>(p + 768, 0xFFFF);
    store<u16>(p + 776, 0xFFFF);
    store<u16>(p + 784, 0xFFFF);
    if ((load<u32>(p + kPlFlags188) & 1) == 0) return ret(ctx, 0u);
    u32 w = load<u32>(kCurrentWorld);
    u32 u = load<u8>(p + kPlUser);
    const u32 pp = load<u32>(w + kWorldUserAbil);
    if (pp != 0) {
        const u32 v = load<u32>(pp);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (static_cast<u32>(static_cast<s32>(static_cast<s8>(u))) < n) {
            for (u32 k = 0; k < kNumAbil; ++k) {
                const u32 rec = load<u32>(load<u32>(load<u32>(w + kWorldUserAbil))) +
                                static_cast<u32>(static_cast<s32>(static_cast<s8>(u))) * kUserAbilSize;
                if (!bit(rec + kUserAbilBits, k)) continue;
                set_on(k);
                u = load<u8>(p + kPlUser);
                w = load<u32>(kCurrentWorld);
            }
        }
    }
    if (load<u32>(d + kDefLevel) == 0) return ret(ctx, u);
    const u32 ub = w + static_cast<u32>(static_cast<s32>(static_cast<s8>(u))) * kUserBlockSize + kUserBlocks;
    const u32 save = w + load<u8>(ub) * kSaveSlotSize + kSaveData;
    Stack frame(ctx, 32);  // the id vector (r1 +112, 20 bytes)
    const u32 vec = frame.addr();
    call(ctx, h17fn::PtrVector_Construct, {vec});
    store<u16>(vec + 6, 4);
    call(ctx, kLevelAbil49E43C, {load<u32>(d + kDefLevel), vec});
    const u32 n = load<u16>(vec + 8), data = load<u32>(vec);
    auto find = [](u32 id) -> s32 {
        for (u32 j = 0; j < kNumAbil; ++j)
            if (load<u32>(kAbilTable + kAbilRecSize * j + kAbilId) == id) return static_cast<s32>(j);
        return -1;
    };
    for (u32 i = 0; i < n; ++i) {
        const u32 id = load<u32>(data + 4 * i);
        if (id == 0) continue;
        const s32 j = find(id);
        if (j >= 0) set_on(static_cast<u32>(j));
    }
    const u32 lv = load<u32>(d + kDefLevel);
    u32 off = 0;
    if (lv == kLevelA) off = kSaveAbilA;
    else if (lv == kLevelB) off = kSaveAbilB;
    else if (lv == kLevelC) off = kSaveAbilC;
    if (off != 0) {
        const u32 id = load<u32>(save + off);
        if (id != 0) set_on(static_cast<u32>(find(id)));  // -1 -> p +655
    }
    call(ctx, hfn::PtrVector_Clear, {vec});
    call(ctx, kNop, {vec});
}
