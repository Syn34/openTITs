// World_HandleEvent callees, part 4: 109..123 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent4c {
namespace cfn {
constexpr u32 PtrVector_Construct = 0x005516E0;  // (PtrVector*)
constexpr u32 PtrVector_Init = 0x00551598;       // (PtrVector*, capacity, 0, 0)
constexpr u32 Effect_Destruct = 0x004C8E80;      // (Effect*) (hevent3_k.cpp sub_004C8E80) (name?)
constexpr u32 SmallFree = 0x00552420;            // (ptr, size)
constexpr u32 Handle_ResolveBit1 = 0x003568E0;   // (handle) -> Object*
constexpr u32 Obj_SetAnim = 0x00363A3C;          // (o, name, now, -, b, c, d; f32)
constexpr u32 Obj_Dispatch362 = 0x0036325C;      // (o, 0, 1, 0)
constexpr u32 StateMachine_SetState = 0x00464D1C;  // (sm, level, id)
constexpr u32 PushStack_Remove = 0x0044D614;     // (item; f32 time)
constexpr u32 Lang_GetSystem = 0x005DBC60;       // () -> s32 language (name?)
constexpr u32 Lang_Set5DBC84 = 0x005DBC84;       // (s32 language) (name?)
constexpr u32 sub_00448680 = 0x00448680;         // ()
constexpr u32 Mem_Alloc = 0x0054ED38;            // (size, pool, align)
constexpr u32 String_Format = 0x005534D0;        // (fmt, ...)
constexpr u32 StringTable_Load = 0x004F8450;     // (StringTable* 8 bytes, path) (name?)
constexpr u32 StringTable_Set4D580C = 0x004D580C;  // (StringTable* or 0) (name?)
}  // namespace cfn

// Effects whose release was deferred: a lazily built PtrVector of 140-byte
// Effect blocks (name?).
constexpr u32 kEffectFreeBlocked = 0x010C2958;   // u8: nothing is freed while set (name?)
constexpr u32 kEffectFreeInit = 0x010C2960;      // u8: the vector is built
constexpr u32 kEffectFreeList = 0x010C2968;      // PtrVector {+0 data, +6 u16, +8 u16 count}

// World fields cleared by World_EndFade466678 (name?).
constexpr u32 kWorldFadeFlags = 0x128474;        // u32: 0x80000000 / 0x40000000 (pending) / 0x20000000
constexpr u32 kWorldFadePush = 0x128478;         // u32 push-stack item (PushStack_Remove)
constexpr u32 kTuneFadeTime = 0x010B8180;        // f32 (0x010B8138 + 72) (name?)

// Languages.
constexpr u32 kTuneLangOverride = 0x010B8A64;    // u8 (0x010B8A1C + 72): set = keep the given language (name?)
constexpr u32 kLangCount = 0x00F65DD0;           // s32 (name?)
constexpr u32 kLangList = 0x00F65D90;            // s32[] supported languages (name?)
constexpr u32 kConfigLanguage = 0x28;            // g::config (kDisplayConfig) +0x28
constexpr u32 kLangTable = 0x00AE6E40;           // {const char* name; u32; u32}[] ("English", ...)
constexpr u32 kStrStringsPath = 0x00AE85DC;      // "GUI/Strings_%s.str"
}  // namespace hevent4c
}  // namespace sly4

using namespace sly4::hevent4c;

namespace {
// The deferred-effect vector, built on first use: Construct, +6 = 4, Init(16).
void EffectFreeList_Build(PPUContext& ctx) {
    call(ctx, cfn::PtrVector_Construct, {kEffectFreeList});
    store<u16>(kEffectFreeList + 6, 4);
    call(ctx, cfn::PtrVector_Init, {kEffectFreeList, 16, 0, 0});
    store<u8>(kEffectFreeInit, 1);
}
}  // namespace

// u8 Effects_FreeDeferred(): unless 0x010C2958 is set, pops every entry of the
// deferred-effect vector (building it first if needed), destroying and freeing
// (140 bytes) the non-null ones. Quirk: the pop is the generic swap-remove, so
// an empty vector (count already 0) would wrap the count and copy entry 0xFFFF
// over entry -1. Returns the last init-flag value seen, as the original's r3 (name?)
DECOMP(004D00CC, Effects_FreeDeferred) {
    const u8 blocked = load<u8>(kEffectFreeBlocked);
    if (blocked != 0) return ret(ctx, static_cast<u32>(blocked));
    u32 r3 = load<u8>(kEffectFreeInit);
    if (r3 == 0) {
        EffectFreeList_Build(ctx);
        r3 = 1;
    }
    for (;;) {
        const u32 n = load<u16>(kEffectFreeList + 8);
        if (n == 0) return ret(ctx, r3);
        const u32 e = load<u32>(load<u32>(kEffectFreeList) + (n - 1) * 4);
        if (e != 0) {
            call(ctx, cfn::Effect_Destruct, {e});
            call(ctx, cfn::SmallFree, {e, 140});
            r3 = load<u8>(kEffectFreeInit);
        }
        if (r3 == 0) {
            EffectFreeList_Build(ctx);
            r3 = 1;
        }
        const u32 c = load<u16>(kEffectFreeList + 8);
        store<u16>(kEffectFreeList + 8, static_cast<u16>(c - 1));
        if (c == 0) {  // never true here: n != 0 above
            const u32 d = load<u32>(kEffectFreeList);
            store<u32>(d - 4, load<u32>(d + 0x3FFFC));
        }
    }
}

// u32 Obj_PlayAttachedAnims(Object* o, u8 now): only for objects with the low
// bit of their u64 +184 set and while the current world has no layers
// (+0x128214). For each of the up to 4 entries {+0 handle, +4 f32 time, +8 u8,
// +9 char name[]} at o->+216 +792 (count at +968) whose handle resolves (bit 1):
// Obj_Dispatch362(obj, 0, 1, 0) unless now, (entry +8 or o +1172 set) and o
// +1173 clear; else Obj_SetAnim(obj, name or 0 when empty, 1, name[0], 0, 0,
// 0x01164CA4; o +1168 when >= 0, else the entry's time). Returns what r3 holds
// at the exit (0, the layer count or the entry count) (name?)
DECOMP(001231B8, Obj_PlayAttachedAnims) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 now = arg<u32>(ctx, 1) & 0xFF;
    if ((load<u64>(o + 184) & 1) == 0) return ret(ctx, 0u);
    const u32 layers = load<u16>(load<u32>(kCurrentWorld) + 0x128214);
    if (layers != 0) return ret(ctx, layers);
    const u32 tab = load<u32>(o + 216) + 772;
    if (load<u32>(tab + 196) == 0) return ret(ctx, 0u);
    const f64 zero = static_cast<f64>(load<f32>(0x001231B4));  // f32 0.0
    u32 i = 0;
    do {
        const u32 e = tab + 20 + i * 44;
        const u32 p = static_cast<u32>(call(ctx, cfn::Handle_ResolveBit1, {load<u32>(e)}));
        if (p != 0) {
            bool dispatch = now == 0;
            if (!dispatch && load<u8>(e + 8) == 0 && load<u8>(o + 1172) == 0) dispatch = true;
            if (!dispatch && load<u8>(o + 1173) != 0) dispatch = true;
            if (dispatch) {
                call(ctx, cfn::Obj_Dispatch362, {p, 0, 1, 0});
            } else {
                f64 t = static_cast<f64>(load<f32>(o + 1168));
                if (!(t > zero || t == zero)) t = static_cast<f64>(load<f32>(e + 4));
                const u32 c0 = load<u8>(e + 9);
                callf(ctx, cfn::Obj_SetAnim, {p, c0 != 0 ? e + 9 : 0u, 1, c0, 0, 0, 0x01164CA4u}, {t});
            }
        }
        ++i;
    } while (i < load<u32>(tab + 196));
    ret(ctx, load<u32>(tab + 196));
}

// u32 World_EndFade466678(World* w, u8 to_game) (name?): only while +0x128474 has
// 0x40000000 (cleared first). w +47 = *w->+8 around two state changes: level 1
// gets 11 (to_game) or 2 (written straight to +49 when its current id is 1..11,
// otherwise StateMachine_SetState), then level 2 gets 0. The push item at
// +0x128478, if any, is removed (time from tuning 0x010B8180); then +0x1283C0,
// +0x128401 (u8), +0x12844C (u16), +0x128458..+0x128470 and +0x128478 = 0 and
// +0x128474 loses 0xE0000000. Returns 0x128474 as the original leaves r3, or the
// flags when nothing was pending.
DECOMP(00466678, World_EndFade466678) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 to_game = arg<u32>(ctx, 1) & 0xFF;
    const u32 f = load<u32>(w + kWorldFadeFlags);
    if ((f & 0x40000000u) == 0) return ret(ctx, f);
    store<u32>(w + kWorldFadeFlags, f & ~0x40000000u);
    const u32 id = to_game != 0 ? 11u : 2u;
    const u32 cur = load<u8>(w + 50);
    store<u8>(w + 47, load<u8>(load<u32>(w + 8)));
    if (static_cast<s32>(cur) < 12 && cur != 0) {
        store<u8>(w + 49, static_cast<u8>(id));
    } else {
        call(ctx, cfn::StateMachine_SetState, {w, 1, id});
    }
    store<u8>(w + 47, load<u8>(load<u32>(w + 8)));
    call(ctx, cfn::StateMachine_SetState, {w, 2, 0});
    const u32 push = load<u32>(w + kWorldFadePush);
    if (push != 0) callf(ctx, cfn::PushStack_Remove, {push}, {static_cast<f64>(load<f32>(kTuneFadeTime))});
    store<u8>(w + 0x1283C0, 0);
    store<u32>(w + 0x128458, 0);
    store<u8>(w + 0x128401, 0);
    store<u16>(w + 0x12844C, 0);
    store<u32>(w + 0x12845C, 0);
    store<u32>(w + 0x128460, 0);
    store<u32>(w + 0x128464, 0);
    store<u32>(w + 0x128468, 0);
    store<u32>(w + 0x12846C, 0);
    store<u32>(w + 0x128470, 0);
    store<u32>(w + kWorldFadeFlags, load<u32>(w + kWorldFadeFlags) & 0x1FFFFFFFu);
    store<u32>(w + kWorldFadePush, 0);
    ret(ctx, kWorldFadeFlags);
}

// u8 Ui_SetLanguage(Ui* u, s32 lang): lang (or, unless tuning 0x010B8A64 is set,
// the system language) when it is in the supported list, else the system
// language when it is, else the list's first. Lang_Set5DBC84(lang); when it
// differs from the config's (+0x28), that is updated, the old string table
// (+36) deleted (vfunc +4) and a new one (8 bytes) loaded from
// "GUI/Strings_<name>.str" and handed to StringTable_Set4D580C (0 when the
// allocation fails). Then sub_00448680(), and u +4176 = lang with +4180 = 1
// when it changed. Returns 1 then, else the old +4176 (name?)
DECOMP(0046E89C, Ui_SetLanguage) {
    const u32 u = arg<u32>(ctx, 0);
    s32 lang = arg<s32>(ctx, 1);
    if (load<u8>(kTuneLangOverride) == 0) lang = static_cast<s32>(call(ctx, cfn::Lang_GetSystem));
    auto supported = [&](s32 l) {
        const s32 n = load<s32>(kLangCount);
        for (s32 i = 0; i < n; ++i)
            if (load<s32>(kLangList + static_cast<u32>(i) * 4) == l) return true;
        return false;
    };
    if (!supported(lang)) {
        lang = static_cast<s32>(call(ctx, cfn::Lang_GetSystem));
        if (!supported(lang)) lang = load<s32>(kLangList);
    }
    call(ctx, cfn::Lang_Set5DBC84, {static_cast<u64>(static_cast<s64>(lang))});
    if (load<s32>(kDisplayConfig + kConfigLanguage) != lang) {
        store<s32>(kDisplayConfig + kConfigLanguage, lang);
        const u32 old = load<u32>(u + 36);
        if (old != 0) {
            vcall(ctx, old, 4, {old});
            store<u32>(u + 36, 0);
        }
        u32 tab = static_cast<u32>(call(ctx, cfn::Mem_Alloc, {8, 0, 16}));
        if (tab != 0) {
            const u32 name = load<u32>(kLangTable + static_cast<u32>(lang) * 12);
            const u32 path = static_cast<u32>(call(ctx, cfn::String_Format, {kStrStringsPath, name}));
            call(ctx, cfn::StringTable_Load, {tab, path});
        }
        store<u32>(u + 36, tab);
        call(ctx, cfn::StringTable_Set4D580C, {tab});
    }
    call(ctx, cfn::sub_00448680);
    const u32 cur = load<u8>(u + 4176);
    if (static_cast<s32>(cur) == lang) return ret(ctx, cur);
    store<u8>(u + 4176, static_cast<u8>(lang));
    store<u8>(u + 4180, 1);
    ret(ctx, 1u);
}
