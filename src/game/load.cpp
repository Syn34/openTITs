// Game_Load and the tuning variables it reads from Scripts/Tuning.xml.
#include "load.h"

using namespace sly4;
using namespace sly4::gameload;

namespace {

constexpr u32 kTuningPath = 0x00AE8404;    // "Scripts/Tuning.xml"
constexpr u32 kCursorFormat = 0x00AE82AC;  // "Common.Cursors.%02d"
constexpr u32 kCursorCount = 2;
constexpr u32 kSysMsgShowCount = 4;
constexpr f32 kFrameTime = 1.0f / 60.0f;   // stored as 0x3C888889

// String_HashNoCase of the tag and attribute names in Tuning.xml.
constexpr u32 kXmlTuning = 0x942972B5;  // <Tuning>
constexpr u32 kXmlVar = 0x00014B07;     // <Var>
constexpr u32 kXmlName = 0x0024728B;    // name="..."
constexpr u32 kXmlValue = 0x04DA9951;   // value="..."
constexpr u32 kValueAttrName = 0x00AF1048;  // "value"

}  // namespace

// bool Game_Load(Game* game)
// One-time set-up of the Game object before the main loop: the UI render
// target, a pad per player and its cursor element, the tuning variables,
// the World, the per-frame arena and the NP manager callback.
DECOMP(00463000, Game_Load) {
    const u32 game = arg<u32>(ctx, 0);

    field<f32>(game, kGameFrameTime) = kFrameTime;
    call(ctx, lfn::Ui_Init, {0});

    for (u32 k = 0; k < kCursorCount; ++k) {
        const u32 player = game + kGamePlayers + k * kPlayerSize;
        ref<u8>(player + kPlayerIndex) = static_cast<u8>(k);
        // InputMgr_CreatePad ignores its argument; the player is still in r3.
        field<u32>(player, kPlayerPad) = call32(ctx, lfn::InputMgr_CreatePad, {player});
        const u32 name = call32(ctx, lfn::String_Format, {kCursorFormat, k + 1});
        field<u32>(game, kGameCursors + k * 4) = call32(ctx, lfn::Ui_Find, {name, 0});
    }

    call(ctx, lfn::Tuning_Load, {kTuningPath, ref<u8>(game + kGameTuningStrings) != 0 ? 1u : 0u});
    field<u32>(game, kGameSysMsgUi) = 0;
    for (u32 k = 0; k < kSysMsgShowCount; ++k) ref<u8>(lg::kSysMsgShows + k) = 1;
    store<u32>(lg::kSysMsgCallback, lg::kSysMsgCallbackOpd);

    const u32 mem = call32(ctx, fn::Mem_Alloc, {kWorldSize, 0, 16});
    u32 world = 0;
    if (mem != 0) {
        call(ctx, lfn::World_Construct, {mem, 0});
        world = mem;
    }
    field<u32>(game, kGameWorld) = world;
    store<u32>(lg::kWorld, world);
    // Virtual +8 on the World (no null check in the original).
    const u32 w = field<u32>(game, kGameWorld);
    call_ptr(ctx, vfunc(w, 8), {w});

    const u32 heap_mem = call32(ctx, fn::Mem_Alloc, {36, 0, 16});
    u32 arena = 0;
    if (heap_mem != 0) {
        call(ctx, fn::Heap_Construct, {heap_mem, kFrameArenaSize, 0, 0});
        arena = heap_mem;
    }
    field<u32>(game, kGameFrameArena) = arena;
    call(ctx, limp::sceNpManagerRegisterCallback, {lg::kNpManagerCallbackOpd, 0});
    ret(ctx, 1u);
}

// bool Tuning_Load(const char* path, bool keep_strings)
// Reads <Tuning><Var name="..." value="..."/>...</Tuning> and hands each
// value to the registered tuning variable of that name, through the
// variable's setter for its type. Returns false only when the file can't be
// parsed or has no <Tuning> element. A <Var> with no matching variable, or
// whose value can't be read, is skipped (the original looks up its name
// attribute and drops the result: probably a stripped log message).
DECOMP(004FC250, Tuning_Load) {
    const u32 path = arg<u32>(ctx, 0);
    const bool keep_strings = arg<u8>(ctx, 1) != 0;

    // The original's locals, at the same offsets from its stack pointer.
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 2408 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 val_type4 = sp + 112;  // up to 16 bytes
    const u32 name_hash = sp + 128;  // u32
    const u32 val_s32 = sp + 132;
    const u32 val_f32 = sp + 136;
    const u32 val_hash = sp + 140;
    const u32 val_type3 = sp + 144;  // 8 bytes
    const u32 doc = sp + 152;        // XmlDoc, 2240 bytes
    const u32 val_u8 = sp + 2392;
    const u32 val_type5 = sp + 2393;  // 15 bytes

    call(ctx, lfn::XmlDoc_Construct, {doc, path});
    if (call32(ctx, lfn::XmlDoc_Parse, {doc, 0}) == 0) {
        call(ctx, lfn::XmlDoc_Destruct, {doc});
        return ret(ctx, 0u);
    }
    const u32 root = call32(ctx, lfn::XmlCursor_FirstChild, {doc, kXmlTuning});
    if (root == 0) {
        call(ctx, lfn::XmlDoc_Destruct, {doc});
        return ret(ctx, 0u);
    }

    auto report = [&](u32 node) { call(ctx, lfn::XmlCursor_FindAttr, {node, kXmlName}); };

    for (u32 node = call32(ctx, lfn::XmlCursor_FirstChild, {root, kXmlVar}); node != 0;
         node = call32(ctx, lfn::XmlCursor_NextSibling, {node, kXmlVar})) {
        if (call32(ctx, lfn::XmlCursor_ReadHash, {node, kXmlName, name_hash, 0}) != 0) continue;

        // Linear search of the registered variables by name hash.
        u32 var = 0;
        const u32 count = load<u32>(lg::kTuningVarCount);
        if (count != 0) {
            const u32 hash = load<u32>(name_hash);
            u32 i = 0;
            do {
                if (load<u32>(load<u32>(lg::kTuningVars + i * 4) + kVarNameHash) == hash) {
                    var = load<u32>(lg::kTuningVars + i * 4);
                    break;
                }
                ++i;
            } while (count > i);
        }
        if (var == 0) {
            report(node);
            continue;
        }

        const s32 type = static_cast<s32>(call_ptr(ctx, vfunc(var, kVarVfnType), {var}));
        switch (type) {
        case kTuningHash:
            if (call32(ctx, lfn::XmlCursor_ReadHash, {node, kXmlValue, val_hash, 0}) == 0)
                call_ptr(ctx, vfunc(var, 40), {var, load<u32>(val_hash)});
            else
                report(node);
            break;
        case kTuningString: {
            const u32 str = call32(ctx, lfn::XmlCursor_FindAttrByName, {node, kValueAttrName});
            if (str == 0) {
                report(node);
                break;
            }
            // Compares the name with the value and drops the result (as the original does).
            if (keep_strings) call(ctx, lfn::String_Compare, {var + kVarName, str});
            call_ptr(ctx, vfunc(var, 36), {var, str});
            break;
        }
        case kTuningColor:
            if (call32(ctx, lfn::XmlCursor_ReadColor, {node, kXmlValue, val_type5, lg::kTuningDefaultType5}) != 0)
                report(node);
            else
                call_ptr(ctx, vfunc(var, 32), {var, val_type5});
            break;
        case kTuningVec3:
            if (call32(ctx, lfn::XmlCursor_ReadVec3, {node, kXmlValue, val_type4, lg::kTuningDefaultType4}) != 0)
                report(node);
            else
                call_ptr(ctx, vfunc(var, 28), {var, val_type4});
            break;
        case kTuningVec2:
            if (call32(ctx, lfn::XmlCursor_ReadVec2, {node, kXmlValue, val_type3, lg::kTuningDefaultType3}) != 0)
                report(node);
            else
                call_ptr(ctx, vfunc(var, 24), {var, val_type3});
            break;
        case kTuningF32:
            // The default (0.0f) comes from the constant at 0x004FC248.
            callf(ctx, lfn::XmlCursor_ReadF32, {node, kXmlValue, val_f32}, {0.0});
            if (static_cast<u32>(ctx.gpr[3]) == 0) {
                ctx.fpr[1] = static_cast<f64>(load<f32>(val_f32));
                call_ptr(ctx, vfunc(var, 20), {var});
            } else {
                report(node);
            }
            break;
        case kTuningS32:
            if (call32(ctx, lfn::XmlCursor_ReadS32, {node, kXmlValue, val_s32, 0}) == 0)
                call_ptr(ctx, vfunc(var, 16),
                         {var, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(val_s32))))});
            else
                report(node);
            break;
        case kTuningU8:
            if (call32(ctx, lfn::XmlCursor_ReadBool, {node, kXmlValue, val_u8, 0}) != 0)
                report(node);
            else
                call_ptr(ctx, vfunc(var, 12), {var, ref<u8>(val_u8)});
            break;
        default:
            break;
        }
    }

    call(ctx, lfn::XmlDoc_Destruct, {doc});
    ret(ctx, 1u);
}
