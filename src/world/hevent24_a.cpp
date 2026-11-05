// World_HandleEvent callees, part 24a: the float-keyed AVL insert below the
// root under AvlF_Insert78F898 (hevent23_a.cpp).
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
// AVL<float, ApeAnimScriptEvent*> (as hevent23_a.cpp): tree +8 u8 rebalance
// flag; 20-byte nodes {+0 s16 balance (right height - left height), +2 s16
// height, +4 f32 key, +8 value, +12 left, +16 right} from the node
// allocator (a 60-byte ApeTFreeList of 64 x 20-byte nodes, made on first
// use).
constexpr u32 kTrRebalance = 8;
constexpr u32 kAnBalance = 0, kAnHeight = 2, kAnKey = 4, kAnValue = 8, kAnLeft = 12, kAnRight = 16;
constexpr u32 kAvlFAllocator = 0x00F8F0FC;    // ApeTFreeList*
constexpr u32 kAvlFListVtable = 0x00B22530;
constexpr u32 kAvlFAllocatorName = 0x00AEEC21;  // "static ApeTFreeList<AVL<...>::AVLNode<...>> &...GetAllocator() [with KeyType = float, ItemType = ApeAnimScriptEvent *]"
constexpr u32 kMemAlloc = 0x0054ED38, kFreeListInit = 0x00551A58;
constexpr u32 kFlSize = 60, kFlAlign = 16, kFlElem = 20, kFlCount = 64;
constexpr u32 kFlAllocSlot = 12;  // allocator vtable slot: () -> node
constexpr u32 kAvlFInsertBelow = 0x0078F410;
constexpr u32 kAvlFRotateRight = 0x0078F180;  // (Tree*, Node** link) (name?)
constexpr u32 kAvlFRotateLeft = 0x0078F2C8;   // (Tree*, Node** link) (name?)

s64 avl_height(u32 n) { return n != 0 ? static_cast<s16>(load<u16>(n + kAnHeight)) : 0; }
s64 avl_balance(u32 n) { return static_cast<s16>(load<u16>(n + kAnBalance)); }

// The node allocator, made on first use (the global is stored even when the
// allocation failed, and the original then calls through it anyway).
// `left`: the left-insert path's register allocation leaves r8 / r9 = the
// vtable / name at FreeList_Init; the right path leaves r8 = the name.
u32 avlf_allocator(PPUContext& ctx, bool left) {
    u32 fl = load<u32>(kAvlFAllocator);
    if (fl != 0) return fl;
    const u32 a = static_cast<u32>(call(ctx, kMemAlloc, {kFlSize, 0, kFlAlign}));
    if (a != 0) {
        for (u32 o : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(a + o, 0);
        const u32 f54 = load<u16>(a + 54);
        for (u32 o : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(a + o, 0);
        store<u16>(a + 52, 0);
        store<u16>(a + 54, static_cast<u16>(f54 & ~0x8000u));
        store<u32>(a + 0, kAvlFListVtable);
        store<u32>(a + 56, kAvlFAllocatorName);
        if (left) {
            ctx.gpr[8] = kAvlFListVtable;
            ctx.gpr[9] = kAvlFAllocatorName;
        } else {
            ctx.gpr[8] = kAvlFAllocatorName;
        }
        call(ctx, kFreeListInit, {a, kFlElem, kFlCount, 0, 0});
        fl = a;
    }
    store<u32>(kAvlFAllocator, fl);
    return fl;
}
}  // namespace

// void AvlF_InsertBelow78F410(Tree* t, -, ApeAnimScriptEvent* value,
// Node** link; f32 key) (name?): recursive insert below *link (fcmpu: key
// below the node's goes left, above goes right, equal or unordered changes
// nothing). An empty child gets a node {0, 1, key, value, 0, 0} from the
// allocator (the null node is stored when it gives none). On the way back,
// with the tree's +8 flag set, *link's height / balance are recomputed and
// a balance beyond +-1 rotated (0x0078F2C8 left, 0x0078F180 right; the
// child rotated first when it leans the other way).
// r3 as the original leaves it: 0 without the flag, else *link or the last
// rotation's result (the earlier values on the null-node paths).
DECOMP(0078F410, AvlF_InsertBelow78F410) {
    const u32 t = arg<u32>(ctx, 0), value = arg<u32>(ctx, 2), link = arg<u32>(ctx, 3);
    const f64 key = farg(ctx, 0);  // passed on unrounded; stored with stfs
    const u32 n = load<u32>(link);
    const f64 nk = static_cast<f64>(load<f32>(n + kAnKey));
    ctx.fpr[1] = nk;  // left in f1 by the original (unless the recursive call replaces it)
    u32 off = 0;
    if (key < nk) off = kAnLeft;
    else if (key > nk) off = kAnRight;
    if (off != 0) {
        const u32 child = load<u32>(n + off);
        if (child != 0) {
            // r4 is the child (the callee does not read it).
            callf(ctx, kAvlFInsertBelow, {t, child, value, n + off}, {key});
        } else {
            const u32 fl = avlf_allocator(ctx, off == kAnLeft);
            const u32 node = static_cast<u32>(vcall(ctx, fl, kFlAllocSlot, {fl}));
            const u32 parent = load<u32>(link);
            if (node != 0) {
                store<u16>(node + kAnBalance, 0);
                store<u16>(node + kAnHeight, 1);
                store<f32>(node + kAnKey, static_cast<f32>(key));
                store<u32>(node + kAnValue, value);
                store<u32>(node + kAnLeft, 0);
                store<u32>(node + kAnRight, 0);
            }
            store<u32>(parent + off, node);
        }
    }

    const u32 flag = load<u8>(t + kTrRebalance);
    if (flag == 0) return ret(ctx, 0u);
    u64 r3 = flag;
    u32 x = load<u32>(link);
    if (x != 0) {
        const s64 hl = avl_height(load<u32>(x + kAnLeft)), hr = avl_height(load<u32>(x + kAnRight));
        store<u16>(x + kAnHeight, static_cast<u16>((hl > hr ? hl : hr) + 1));
        store<u16>(x + kAnBalance, static_cast<u16>(hr - hl));
        r3 = static_cast<u64>(hr - hl);
        x = load<u32>(link);
    }
    if (x == 0) {
        ctx.gpr[3] = r3;
        return;
    }
    // Right heavy: rotate left (the right child right first when it leans left).
    if (avl_balance(x) > 1) {
        const u32 r = load<u32>(x + kAnRight);
        if (r != 0) {
            if (avl_balance(r) < 0) call(ctx, kAvlFRotateRight, {t, x + kAnRight});
            call(ctx, kAvlFRotateLeft, {t, link});
        }
    }
    // Left heavy: rotate right (the left child left first when it leans right).
    const u32 y = load<u32>(link);
    if (y == 0 || avl_balance(y) >= -1) return ret(ctx, y);
    const u32 l = load<u32>(y + kAnLeft);
    if (l == 0) return ret(ctx, y);
    if (avl_balance(l) > 0) call(ctx, kAvlFRotateLeft, {t, y + kAnLeft});
    call(ctx, kAvlFRotateRight, {t, link});
}

namespace {
// Anim_Load34A5E4: Havok class lookups as hevent23_c.cpp Level_Load34AB90.
constexpr u32 kHkClassSlot7FECB8 = 0x007FECB8;  // () -> 0x011E2E80 (the root level container class slot) (name?)
constexpr u32 kHkLoadWord7D53AC = 0x007D53AC;   // (u32* slot) -> *slot (name?)
constexpr u32 kHkAddRef7D48C4 = 0x007D48C4;     // (hkResource*) (name?)
constexpr u32 kHkFindObject802B08 = 0x00802B08; // (container, const hkClass*, 0) -> object (name?)
constexpr u32 kHkAnimClassSlotA39858 = 0x00A39858;  // () -> 0x011F7EA4 (TOC constant) (name?)
constexpr u32 kHkAnimClassSlot = 0x011F7EA4;    // the animation container class slot (name?)
constexpr u32 kHkRemoveRef = 0x007D4EA8;        // (obj) (symbols.txt AnimSkel_Release7D4EA8)
constexpr u32 kHkRegistry = 0x011E2EE0;         // object whose vtable +16 gives the class registry (name?)
constexpr u32 kSlotRegistry = 16, kSlotContents = 28;
constexpr u32 kSortedMapFind76D894 = 0x0076D894;  // (map, const u32* key) -> value slot (symbols.txt)
constexpr u32 kHashToString = 0x0056A6E0, kStringFormat = 0x005534D0;
// The loaded animation resources (PtrVector, hevent18 layout) and the
// animation map ({key, animation} entries, hevent22::map_append layout).
constexpr u32 kAnimResources = 0x0106BC2C;
constexpr u32 kAnimMap = 0x0106BC14;
constexpr u32 kStrDuplicate = 0x00ADC004;  // "Duplicate Havok skeletons in table: %s"
// The animation container: +8 animations (first one used), +12 count,
// +20 bindings, +24 s32 binding count.
constexpr u32 kAcAnims = 8, kAcAnimCount = 12, kAcBindings = 20, kAcBindingCount = 24;
}  // namespace

// bool Anim_Load34A5E4(-, u32 key, hkResource* res) (name?): registers a
// loaded Havok animation file. The resource (vtable +28 (res, root level
// container class, registry)) gives the root container, whose animation
// container is found with 0x00802B08 (none: returns 1); without a root
// container the resource is asked again for the animation container
// class itself (none: returns 0). A container with no animations, or a
// null first one, returns 0. Otherwise the resource gets 0x007D48C4 and is
// pushed onto the vector at 0x0106BC2C, the first binding is released
// (0x007D4EA8) when there are any, and the first animation is added to the
// map at 0x0106BC14 under key unless key is 0 or already there ("Duplicate
// Havok skeletons in table: %s" reported then). Returns 1.
DECOMP(0034A5E4, Anim_Load34A5E4) {
    const u32 res = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 key_slot = frame.addr();
    store<u32>(key_slot, arg<u32>(ctx, 1));
    const auto contents = [&](u32 cls) {
        const u32 reg_obj = load<u32>(kHkRegistry);
        const u32 reg = static_cast<u32>(vcall(ctx, reg_obj, kSlotRegistry, {reg_obj}));
        return static_cast<u32>(vcall(ctx, res, kSlotContents, {res, cls, reg}));
    };
    u32 ac = contents(static_cast<u32>(call(ctx, kHkLoadWord7D53AC, {static_cast<u32>(call(ctx, kHkClassSlot7FECB8, {}))})));
    if (ac != 0) {
        const u32 cls = static_cast<u32>(call(ctx, kHkLoadWord7D53AC, {kHkAnimClassSlot}));
        ac = static_cast<u32>(call(ctx, kHkFindObject802B08, {ac, cls, 0}));
        if (ac == 0) return ret(ctx, 1u);
    } else {
        ac = contents(static_cast<u32>(call(ctx, kHkLoadWord7D53AC, {static_cast<u32>(call(ctx, kHkAnimClassSlotA39858, {}))})));
        if (ac == 0) return ret(ctx, 0u);
    }
    if (load<u32>(ac + kAcAnimCount) == 0) return ret(ctx, 0u);
    const u32 anim = load<u32>(load<u32>(ac + kAcAnims));
    if (anim == 0) return ret(ctx, 0u);
    call(ctx, kHkAddRef7D48C4, {res});
    sly4::hevent18::pv_push(ctx, kAnimResources, res);
    if (static_cast<s32>(load<u32>(ac + kAcBindingCount)) > 0) call(ctx, kHkRemoveRef, {load<u32>(load<u32>(ac + kAcBindings))});
    const u32 key = load<u32>(key_slot);
    bool dup = key == 0;
    if (!dup) dup = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind76D894, {kAnimMap, key_slot}))) != 0;
    if (dup) {
        const u32 s = static_cast<u32>(call(ctx, kHashToString, {load<u32>(key_slot)}));
        call(ctx, kStringFormat, {kStrDuplicate, s});
        return ret(ctx, 1u);
    }
    sly4::hevent22::map_append(ctx, kAnimMap, load<u32>(key_slot), anim);
    ret(ctx, 1u);
}

namespace {
// Spk_Refresh385008: speaker (map screen) objects, as hevent23_e.cpp: +8
// def (+148 36-byte entries {+20 target}, +152 count), +20 effect, +24
// object, +28 speaker, +160 mode, +200 u32[mode] text ids, +236 16-byte
// widget records {+4 widget, +8 u32}, +276 u32[] per-section flags,
// +316 / +328 two reference objects, +332 16-byte section records {+0
// object, +4, +8 (+340 / +336 for section 0), +12 (+344)}, +492 / +496 f32
// the current section's range, +532 the text object, +536 u32 section
// index, +540 u32 section count, +544..+584 f32 the map projection
// results, +960 flags. Section objects carry a vec3 at +128.
constexpr u32 kSpkConsts = 0x00384FF4;  // f32 {1.0, FLT_MAX, 0.5, -1.0}
constexpr u32 kSpkFlags = 960, kSpkMode = 160, kSpkDef = 8, kSpkEffect = 20, kSpkObj = 24, kSpkSpeaker = 28;
constexpr u32 kSpkModeText = 200, kSpkWidgets = 236, kSpkSectionFlags = 276, kSpkRefA = 316, kSpkRefB = 328;
constexpr u32 kSpkSections = 332, kSpkRangeLo = 492, kSpkRangeHi = 496, kSpkText = 532, kSpkSection = 536, kSpkSectionCount = 540;
constexpr u32 kSpkFlagActive = 0x40000000, kSpkFlagSaid = 0x01000000, kSpkFlagEffect = 0x00800000, kSpkFlagMap = 0x00100000;
constexpr u32 kSpkHashes = 0x00F65018;  // u32[mode] effect hashes
constexpr u32 kDefEntries = 148, kDefCount = 152, kEntrySize = 36, kEntryTarget = 20;
constexpr u32 kWidgetSize = 16, kWidgetObj = 4, kWidgetArg = 8, kWidgetShow = 88, kObjActivate = 40, kGoalName = 48;
constexpr u32 kSecKey = 132;  // section object +132: f32 the height where the section starts
constexpr u32 kPos = 128;     // vec3
constexpr u32 kDefaultPos = 0x01164B40;  // vec3 used without a player
constexpr u32 kSession = 0x00F64C20, kSessEpisode = 36, kSessJob = 40, kSessGoal = 72;
constexpr u32 kSessValue = 0x00F64C24;   // u32 shown as "%d"
constexpr u32 kHideoutMode = 0x00F64CE4; // u32 handle looked up in 0x0106C228 (name?)
constexpr u32 kWLevelInfo = 0x128164, kLevelInfoName = 5456;
constexpr u32 kMapScale = 0x0106CDC0;    // f32 copied to +568 (name?)
constexpr u32 kTagOn = 0x01164CA0, kTagOff = 0x0106CF50;  // 4-byte tags copied to the text object +316
constexpr u32 kTextKindMask = 0x3F, kTextKindShift = 10, kTextKindMap = 8, kTextTag = 316;
constexpr u32 kSetAnimArg = 0x01164CA4;

constexpr u32 kSpeakerSayId = 0x004A5C90, kEffectSetHash = 0x004CF988, kObjSet4E3168 = 0x004E3168, kObjAttach4E2DB0 = 0x004E2DB0;
constexpr u32 kFieldGetOrDefault = 0x0047C46C;  // Field_Get1281C0OrDefault (World*) -> name
constexpr u32 kSpkSetText = 0x00384EE4;         // (SpeakerObj*, const char* text, u32 field hash) (name?)
constexpr u32 kStringFormat2 = 0x005534D0, kSortedMapFind3 = 0x0076611C, kObjSetAnim = 0x00363A3C;
constexpr u32 kGameComplete = 0x002E3188;   // (session, 0) -> f32 percent (name?)
constexpr u32 kEpComplete = 0x002DB5D4;     // (episode) -> f32 percent (name?)
constexpr u32 kEpStat5C = 0x002DAE5C, kEpStatA0 = 0x002DAEA0, kEpHasMore = 0x002DAA54, kEpStatB04 = 0x002DAB04;
constexpr u32 kEpStatD0 = 0x002DA9D0, kEpState = 0x002DA990;  // (episode) -> s32 (name?)
constexpr u32 kStrEmpty = 0x00ADCAA8, kStrPercentD = 0x00ADC94C, kStrMapNA = 0x00ADCD00, kStrNone = 0x00ADCD38;
constexpr u32 kStrEpisode = 0x00ADCD18, kStrJob = 0x00ADCD28, kStrGoal = 0x00ADCD48;
constexpr u32 kStrGameComplete = 0x00ADCC80, kStrEpComplete = 0x00ADCCA8;
constexpr u32 kStrNA1 = 0x00ADCCD0, kStrNA2 = 0x00ADCCE8;
constexpr u32 kStrEpisodeNone = 0x00ADCD58, kStrJobNone = 0x00ADCD70, kStrGoalNone = 0x00ADCD88;
constexpr u32 kStrHideoutBack = 0x00ADCB84;  // "$HIDEOUT_BUTTON_BACK"
// The text field hashes given to 0x00384EE4.
constexpr u32 kFLevel = 0x2A650554, kFMap = 0xED264556, kFEpisode = 0xEB9FD13D, kFEpisodeName = 0x9865DF4A;
constexpr u32 kFJob = 0xCF8544BB, kFGoal = 0x225F2705, kFGameComplete = 0x6BCA532B, kFEpComplete = 0x26DA6014;
constexpr u32 kFStat1 = 0xF17AEB11, kFStat2 = 0x10C350EE, kFStat3 = 0xDE006FF7, kFStat4 = 0xF1FB3454;
constexpr u32 kFStat5 = 0x0BBAEF6E, kFStat6 = 0x9B13B20B, kFStat7 = 0xC1CE33D0, kFStat8 = 0x196765ED;

u32 f32_bits(f32 v) { return std::bit_cast<u32>(v); }
f64 lfs(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
f64 fsubs(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a - b)); }
// The compiler's inlined floorf on a percent: the truncation unless x is
// negative, not integral and below 2^24; returns fctiwz of the result.
s32 floor_percent(f64 x) {
    const f64 t = rt::to_single(rt::fcfid(rt::fctid(x, true)));
    const u32 tw = static_cast<u32>(rt::fctiw(x, true));
    f64 v = x;
    if (f32_bits(static_cast<f32>(t - x)) != 0) {
        const u32 b = f32_bits(static_cast<f32>(x));
        if (static_cast<s32>(b & 0x7F800000u) < 0x4B800000)
            v = (b & 0x80000000u) != 0 ? rt::to_single(static_cast<f64>(static_cast<s64>(static_cast<s32>(tw - 1)))) : t;
    }
    return static_cast<s32>(static_cast<u32>(rt::fctiw(v, true)));
}
}  // namespace

// u32 Spk_Refresh385008(SpeakerObj* o) (name?): enters mode 1 (flag
// 0x40000000; the +24 object's vtable +40 (p, 1, 1)), says the mode's text
// (Speaker_SayId4A5C90 into flag 0x01000000) and sets the mode's effect
// hash (into 0x00800000); every def entry with a widget gets its widget's
// vtable +88 (w, +8 != 0) and, with an entry target, Obj_Set4E3168("",
// w) and Obj_Attach4E2DB0(target, w, 1, 0). Then picks the map section
// from the player's height (or the default position's), fills the map
// screen's texts through 0x00384EE4 (level name, map availability,
// episode / job / goal, the game and episode completion percents and the
// episode stats, or the "none" / "not available" texts without an
// episode), tags the text object, and with the map flag computes the
// player's position projected on the section's axes (+544..+584). Finally
// a 0x00F64CE4 handle object with +4 bit 0 plays "$HIDEOUT_BUTTON_BACK".
// r3 as the original leaves it.
DECOMP(00385008, Spk_Refresh385008) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 mode0 = load<u32>(o + kSpkMode);
    const u32 def = load<u32>(o + kSpkDef);
    store<u32>(o + kSpkFlags, load<u32>(o + kSpkFlags) | kSpkFlagActive);
    u32 mode = (mode0 & 1u) | (1u << 1);
    store<u32>(o + kSpkMode, mode);
    if (const u32 p = load<u32>(o + kSpkObj); p != 0) {
        vcall(ctx, p, kObjActivate, {p, 1, 1});
        mode = load<u32>(o + kSpkMode);
    }
    const u32 m4 = (mode >> 1) << 2;
    const f64 one = lfs(kSpkConsts);
    callf(ctx, kSpeakerSayId, {load<u32>(o + kSpkSpeaker), load<u32>(o + m4 + kSpkModeText), 0}, {one});
    const u32 said = static_cast<u32>(ctx.gpr[3]);
    store<u32>(o + kSpkFlags, (load<u32>(o + kSpkFlags) & ~kSpkFlagSaid) | ((said & 1u) << 24));
    if (const u32 e = load<u32>(o + kSpkEffect); e != 0) {
        const u32 r = static_cast<u32>(call(ctx, kEffectSetHash, {e, load<u32>(kSpkHashes + m4)}));
        store<u32>(o + kSpkFlags, (load<u32>(o + kSpkFlags) & ~kSpkFlagEffect) | ((r & 1u) << 23));
    }

    // The def entries' widgets.
    if (load<u32>(def + kDefCount) != 0) {
        for (u32 i = 0; i < load<u32>(def + kDefCount); ++i) {
            const u32 rec = load<u32>(o + kSpkWidgets) + kWidgetSize * i;
            const u32 w = load<u32>(rec + kWidgetObj);
            if (w == 0) continue;
            // vtable +88 (w, +8 != 0); the original leaves r5 = the entry, r6 = the OPD.
            const u32 opd = load<u32>(load<u32>(w) + kWidgetShow);
            ctx.gpr[5] = load<u32>(opd);
            ctx.gpr[6] = opd;
            call_ptr(ctx, opd, {w, load<u32>(rec + kWidgetArg) != 0 ? 1u : 0u});
            if (load<u32>(load<u32>(def + kDefEntries) + kEntrySize * i + kEntryTarget) == 0) continue;
            call(ctx, kObjSet4E3168, {kStrEmpty, load<u32>(rec + kWidgetObj)});
            call(ctx, kObjAttach4E2DB0, {load<u32>(load<u32>(def + kDefEntries) + kEntrySize * i + kEntryTarget), load<u32>(rec + kWidgetObj), 1, 0});
        }
    }

    // The section the player (or the default position) is in.
    f64 px = lfs(kDefaultPos), py = lfs(kDefaultPos + 4), pz = lfs(kDefaultPos + 8);
    store<u32>(o + kSpkRangeLo, 0xFF7FFFFFu);  // -FLT_MAX
    store<u32>(o + kSpkRangeHi, 0x7F7FFFFFu);  // FLT_MAX
    store<u32>(o + kSpkSection, 0);
    const u32 world = load<u32>(kCurrentWorld);
    if (const u32 player = load<u32>(world + kUserBlocks + 8); player != 0) {
        px = lfs(player + 144);
        py = lfs(player + 148);
        pz = lfs(player + 152);
        const u32 cnt = load<u32>(o + kSpkSectionCount);
        if (cnt > 1) {
            u32 idx = 0;
            for (u32 i = 1; i < cnt; ++i) {
                const u32 sec = load<u32>(o + 16 * i + kSpkSections);
                if (!(py > lfs(sec + kSecKey))) break;
                store<u32>(o + kSpkSection, ++idx);
                store<u32>(o + kSpkRangeLo, load<u32>(load<u32>(o + 16 * i + kSpkSections) + kSecKey));
                const u32 hi = i < cnt - 1 ? load<u32>(load<u32>(o + 16 * (i + 1) + kSpkSections) + kSecKey) : load<u32>(kSpkConsts + 4);
                store<u32>(o + kSpkRangeHi, hi);
            }
        }
    }

    // The texts.
    const u32 sess = load<u32>(kSession);
    const u32 episode = load<u32>(sess + kSessEpisode), job = load<u32>(sess + kSessJob);
    u32 goal = load<u32>(sess + kSessGoal);
    const u32 wli = load<u32>(world + kWLevelInfo);
    const u32 level = wli != 0 ? wli + kLevelInfoName : static_cast<u32>(call(ctx, kFieldGetOrDefault, {world}));
    call(ctx, kSpkSetText, {o, level, kFLevel});
    const bool map_ok = load<u32>(o + 4 * load<u32>(o + kSpkSection) + kSpkSectionFlags) != 0 && (load<u32>(o + kSpkFlags) & kSpkFlagMap) != 0;
    call(ctx, kSpkSetText, {o, map_ok ? kStrEmpty : kStrMapNA, kFMap});
    const auto fmt = [&](u32 f, u32 v) { return static_cast<u32>(call(ctx, kStringFormat2, {f, v})); };
    const auto text = [&](u32 s, u32 field) { call(ctx, kSpkSetText, {o, s, field}); };
    const auto percent = [&](u32 f) {
        // f1 from the previous call; the %d argument sign-extended.
        return fmt(f, static_cast<u32>(floor_percent(ctx.fpr[1])));
    };
    u32 tag;
    if (episode != 0) {
        text(fmt(kStrEpisode, episode), kFEpisode);
        text(episode + 130, kFEpisodeName);
        text(fmt(kStrJob, job != 0 ? job : kStrNone), kFJob);
        goal = goal == 0 ? kStrNone : static_cast<u32>(vcall(ctx, goal, kGoalName, {goal}));
        text(fmt(kStrGoal, goal), kFGoal);
        call(ctx, kGameComplete, {load<u32>(kSession), 0});
        text(percent(kStrGameComplete), kFGameComplete);
        call(ctx, kEpComplete, {episode});
        text(percent(kStrEpComplete), kFEpComplete);
        text(fmt(kStrPercentD, static_cast<u32>(call(ctx, kEpStat5C, {episode}))), kFStat1);
        text(fmt(kStrPercentD, static_cast<u32>(call(ctx, kEpStatA0, {episode}))), kFStat2);
        const u32 more = static_cast<u32>(call(ctx, kEpHasMore, {episode}));
        text(more == 0 ? kStrNA1 : fmt(kStrPercentD, static_cast<u32>(call(ctx, kEpStatB04, {episode}))), kFStat3);
        text(more == 0 ? kStrNA2 : fmt(kStrPercentD, load<u32>(kSessValue)), kFStat4);
        text(more == 0 ? kStrNA1 : fmt(kStrPercentD, static_cast<u32>(call(ctx, kEpStatD0, {episode}))), kFStat5);
        text(more == 0 ? kStrNA2 : fmt(kStrPercentD, static_cast<u32>(call(ctx, kEpHasMore, {episode}))), kFStat6);
        text(more == 0 ? kStrNA1 : fmt(kStrPercentD, static_cast<u32>(call(ctx, kEpState, {episode})) == 3 ? 1u : 0u), kFStat7);
        text(more != 0 ? fmt(kStrPercentD, 1) : kStrNA2, kFStat8);
        tag = more != 0 ? kTagOn : kTagOff;
    } else {
        text(kStrEpisodeNone, kFEpisode);
        text(kStrEmpty, kFEpisodeName);
        text(kStrJobNone, kFJob);
        text(kStrGoalNone, kFGoal);
        call(ctx, kGameComplete, {load<u32>(kSession), 0});
        text(percent(kStrGameComplete), kFGameComplete);
        text(kStrEmpty, kFEpComplete);
        text(kStrNA1, kFStat3);
        text(kStrNA2, kFStat4);
        text(kStrNA1, kFStat1);
        text(kStrNA2, kFStat2);
        text(kStrNA1, kFStat5);
        text(kStrNA2, kFStat6);
        text(kStrNA1, kFStat7);
        text(kStrNA2, kFStat8);
        tag = kTagOff;
    }
    if (const u32 t = load<u32>(o + kSpkText); t != 0 && ((load<u32>(t + 4) >> kTextKindShift) & kTextKindMask) == kTextKindMap) {
        const u8 b0 = load<u8>(tag), b1 = load<u8>(tag + 1), b2 = load<u8>(tag + 2), b3 = load<u8>(tag + 3);
        store<u8>(t + kTextTag + 0, b0);
        store<u8>(t + kTextTag + 1, b1);
        store<u8>(t + kTextTag + 2, b2);
        store<u8>(t + kTextTag + 3, b3);
    }

    // The map projection.
    const auto sec_rec = [&](u32 off) { return load<u32>(o + 16 * load<u32>(o + kSpkSection) + off); };
    if ((load<u32>(o + kSpkFlags) & kSpkFlagMap) != 0 && load<u32>(o + 4 * load<u32>(o + kSpkSection) + kSpkSectionFlags) != 0 &&
        load<u32>(o + kSpkRefA) != 0 && load<u32>(o + kSpkRefB) != 0 && sec_rec(kSpkSections) != 0 && sec_rec(kSpkSections + 12) != 0) {
        const f64 fone = lfs(kSpkConsts), half = lfs(kSpkConsts + 8);
        const u32 a = sec_rec(kSpkSections), b = sec_rec(kSpkSections + 12);
        store<u32>(o + 568, load<u32>(kMapScale));
        store<f32>(o + 552, static_cast<f32>(std::fabs(fsubs(lfs(a + kPos), lfs(b + kPos)))));
        {
            const u32 b2 = sec_rec(kSpkSections + 12), a2 = sec_rec(kSpkSections);
            store<f32>(o + 556, static_cast<f32>(std::fabs(fsubs(lfs(a2 + kPos + 8), lfs(b2 + kPos + 8)))));
        }
        store<f32>(o + 560, static_cast<f32>(std::fabs(fsubs(lfs(load<u32>(o + kSpkRefA) + kPos), lfs(load<u32>(o + kSpkRefB) + kPos)))));
        store<f32>(o + 564, static_cast<f32>(std::fabs(fsubs(lfs(load<u32>(o + kSpkRefA) + kPos + 4), lfs(load<u32>(o + kSpkRefB) + kPos + 4)))));
        const u32 c = sec_rec(kSpkSections + 4), d = c != 0 ? sec_rec(kSpkSections + 8) : 0;
        f64 u, v;
        if (c != 0 && d != 0) {
            const u32 s = sec_rec(kSpkSections);
            const u32 C = c + kPos, D = d + kPos, A = s + kPos;
            const f64 ay = lfs(A + 4), ax = lfs(A), az = lfs(A + 8);
            const f64 c1 = fsubs(lfs(C + 4), ay), c2 = fsubs(lfs(C), ax), c7 = fsubs(lfs(C + 8), az);
            const f64 d3 = fsubs(lfs(D + 4), ay), d4 = fsubs(lfs(D), ax), d9 = fsubs(lfs(D + 8), az);
            const f64 qy = fsubs(py, ay), qx = fsubs(px, ax), qz = fsubs(pz, az);
            f64 lc = fmuls(c1, c1), ld = fmuls(d3, d3);
            lc = static_cast<f32>(std::fma(c2, c2, lc));
            ld = static_cast<f32>(std::fma(d4, d4, ld));
            lc = static_cast<f32>(std::fma(c7, c7, lc));
            ld = static_cast<f32>(std::fma(d9, d9, ld));
            lc = static_cast<f32>(std::sqrt(lc));
            ld = static_cast<f32>(std::sqrt(ld));
            lc = static_cast<f32>(fone / lc);
            ld = static_cast<f32>(fone / ld);
            f64 r = fmuls(qy, fmuls(lc, c1));
            r = static_cast<f32>(std::fma(qx, fmuls(lc, c2), r));
            r = static_cast<f32>(std::fma(qz, fmuls(lc, c7), r));
            store<f32>(o + 544, static_cast<f32>(std::fabs(r)));
            f64 q = fmuls(qy, fmuls(ld, d3));
            q = static_cast<f32>(std::fma(qx, fmuls(ld, d4), q));
            q = static_cast<f32>(std::fma(qz, fmuls(ld, d9), q));
            store<f32>(o + 548, static_cast<f32>(std::fabs(q)));
            u = lfs(o + 544);
            v = lfs(o + 548);
        } else {
            const u32 A = sec_rec(kSpkSections) + kPos;
            v = std::fabs(fsubs(pz, lfs(A + 8)));
            u = std::fabs(fsubs(px, lfs(A)));
            store<f32>(o + 548, static_cast<f32>(v));
            store<f32>(o + 544, static_cast<f32>(u));
        }
        const f64 w556 = lfs(o + 556);
        const f64 su = static_cast<f32>(u / w556), sv = static_cast<f32>(v / w556);
        const f64 scale = lfs(o + 568);
        const f64 inv = static_cast<f32>(fone / scale);
        store<f32>(o + 544, static_cast<f32>(su));
        store<f32>(o + 548, static_cast<f32>(sv));
        const f64 ratio = static_cast<f32>(lfs(o + 560) / lfs(o + 564));
        const f64 e544 = lfs(o + 544), e548 = lfs(o + 548);
        store<f32>(o + 576, static_cast<f32>(-std::fma(inv, half, -e548)));
        const f64 r2 = static_cast<f32>(ratio / scale);
        store<f32>(o + 572, static_cast<f32>(-std::fma(r2, half, -e544)));
        const f64 g544 = lfs(o + 544), g548 = lfs(o + 548);
        const f32 lo = static_cast<f32>(std::fma(r2, half, g544));
        store<f32>(o + 580, lo);
        store<f32>(o + 584, static_cast<f32>(std::fma(inv, half, g548)));
        ctx.fpr[1] = lo;  // left in f1 by the original
    }

    // The hideout's back button.
    const u32 h = load<u32>(kHideoutMode);
    if (h == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), h);
    const u32 obj = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind3, {kHandleMap, frame.addr()})));
    if (obj == 0) return ret(ctx, 0u);
    if ((load<u32>(obj + 4) & 1u) == 0) return ret(ctx, obj);
    // r6 is not set for this call (Obj_SetAnim does not read it).
    callf(ctx, kObjSetAnim, {obj, kStrHideoutBack, 1, ctx.gpr[6], 0, 0, kSetAnimArg}, {lfs(kSpkConsts + 12)});
}

namespace {
// Anim_UnloadAll348BFC: the animation tables filled by Anim_Load34A5E4 /
// Level_Load34AB90 / Scene_Load348F04.
constexpr u32 kAnimHolder = 0x00F64C9C;   // u32 object released through vtable +4 (name?)
constexpr u32 kSceneMap = 0x0106BBE8;     // {key, scene} map (u16 count +2)
constexpr u32 kSceneVec = 0x0106BC00;     // 16-byte records {+4 resource} (u16 count +8)
constexpr u32 kSkeletonMap2 = 0x0106BC40, kSkelResources = 0x0106BC58;
constexpr u32 kNamedMaps[3] = {0x0106BB9C, 0x0106BBB4, 0x0106BBCC};  // {key, 8-byte {-, +4 block}} maps
constexpr u32 kAnimLoaded = 0x0106BBE4;   // u8
constexpr u32 kSmallFree = 0x00552420, kMemFreeFromA = 0x0054EE6C;
}  // namespace

// u32 Anim_UnloadAll348BFC() (name?): the 0x00F64C9C object is released
// (vtable +4) and cleared; every loaded Havok resource is released
// (0x007D4EA8: the scene records' +4, the animation and skeleton resource
// vectors) and the vectors / their maps emptied; the three name maps'
// entries are freed (Mem_FreeFrom(e +4, 0), SmallFree(e, 8)) and emptied;
// 0x0106BBE4 = 0. Returns 0.
DECOMP(00348BFC, Anim_UnloadAll348BFC) {
    if (const u32 p = load<u32>(kAnimHolder); p != 0) {
        vcall(ctx, p, 4, {p});
        store<u32>(kAnimHolder, 0);
    }
    store<u16>(kSceneMap + 2, 0);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(kSceneVec + 8)); ++i)
        call(ctx, kHkRemoveRef, {load<u32>(load<u32>(kSceneVec) + 16 * i + 4)});
    store<u16>(kSceneVec + 8, 0);
    store<u16>(kAnimMap + 2, 0);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(kAnimResources + 8)); ++i)
        call(ctx, kHkRemoveRef, {load<u32>(load<u32>(kAnimResources) + 4 * i)});
    store<u16>(kAnimResources + 8, 0);
    store<u16>(kSkeletonMap2 + 2, 0);
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(kSkelResources + 8)); ++i)
        call(ctx, kHkRemoveRef, {load<u32>(load<u32>(kSkelResources) + 4 * i)});
    store<u16>(kSkelResources + 8, 0);
    for (u32 m : kNamedMaps) {
        for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(m + 2)); ++i) {
            const u32 e = load<u32>(load<u32>(m + 8) + 8 * i + 4);
            if (e == 0) continue;
            call(ctx, kMemFreeFromA, {load<u32>(e + 4), 0});
            call(ctx, kSmallFree, {e, 8});
        }
        store<u16>(m + 2, 0);
    }
    store<u8>(kAnimLoaded, 0);
    ret(ctx, 0u);
}
