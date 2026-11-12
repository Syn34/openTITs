// Game_Load and the tuning variables it reads from Scripts/Tuning.xml.
#include "load.h"

#include "../memory/mem.h"
#include "../string/str.h"
#include "../xml/xml.h"

namespace sly4::gameload {
using namespace opentit::native;
using xml::XmlCursor;
using xml::XmlDoc;

namespace {

constexpr u32 kTuningPath = 0x00AE8404;    // "Scripts/Tuning.xml"
constexpr u32 kCursorFormat = 0x00AE82AC;  // "Common.Cursors.%02d"
constexpr u32 kCursorCount = 2;
constexpr u32 kSysMsgShowCount = 4;
constexpr f32 kFrameTime = 1.0f / 60.0f;   // stored as 0x3C888889
constexpr u32 kArenaHeapSize = 36;         // sizeof(LinearHeap)
constexpr u32 kWorldVfnInit = 8;           // (name?)

// String_HashNoCase of the tag and attribute names in Tuning.xml.
constexpr u32 kXmlTuning = 0x942972B5;  // <Tuning>
constexpr u32 kXmlVar = 0x00014B07;     // <Var>
constexpr u32 kXmlName = 0x0024728B;    // name="..."
constexpr u32 kXmlValue = 0x04DA9951;   // value="..."
constexpr u32 kValueAttrName = 0x00AF1048;  // "value"

// Setters of a tuning variable, by type (vtable byte offsets).
constexpr u32 kVarVfnSetU8 = 12, kVarVfnSetS32 = 16, kVarVfnSetF32 = 20, kVarVfnSetVec2 = 24, kVarVfnSetVec3 = 28,
              kVarVfnSetColor = 32, kVarVfnSetString = 36, kVarVfnSetHash = 40;

// Tuning_Load's locals whose addresses go to the XML reader.
struct TuningLocals {
    be_u32 vec3[4];     // +0
    be_u32 name_hash;   // +16
    be_u32 s32_value;   // +20
    be_f32 f32_value;   // +24
    be_u32 hash_value;  // +28
    be_u32 vec2[2];     // +32
    u8 u8_value;        // +40
    u8 color[15];       // +41
    XmlDoc doc;         // +56
};

}  // namespace

// One-time set-up of the Game object before the main loop: the UI render
// target, a pad per player and its cursor element, the tuning variables,
// the World, the per-frame arena and the NP manager callback.
u32 Game_Load(LoadGame* game) {
    game->frame_time = kFrameTime;
    Ui_Init(nullptr);

    for (u32 k = 0; k < kCursorCount; ++k) {
        LoadPlayer& player = game->players[k];
        player.index = static_cast<u8>(k);
        player.pad = InputMgr_CreatePad();
        const char* name = call<const char*>(lfn::String_Format, kCursorFormat, k + 1);  // variadic
        game->cursors[k] = guest(Ui_Find(name, nullptr));
    }

    Tuning_Load(host<const char>(kTuningPath), game->tuning_strings != 0 ? 1 : 0);
    game->sys_msg_ui = 0u;
    for (u32 k = 0; k < kSysMsgShowCount; ++k) global<u8>(lg::kSysMsgShows + k) = 1;
    global<be_u32>(lg::kSysMsgCallback) = lg::kSysMsgCallbackOpd;

    void* mem = memory::Mem_Alloc(kWorldSize, 0, 16);
    u32 world = 0;
    if (mem) {
        call(lfn::World_Construct, mem, 0u);
        world = guest(mem);
    }
    game->world = world;
    global<be_u32>(lg::kWorld) = world;
    // Virtual +8 on the World (no null check in the original).
    vcall(host<u8>(game->world), kWorldVfnInit);

    auto* heap = static_cast<LinearHeap*>(memory::Mem_Alloc(kArenaHeapSize, 0, 16));
    u32 arena = 0;
    if (heap) {
        memory::Heap_Construct(heap, kFrameArenaSize, 0, 0);
        arena = guest(heap);
    }
    game->frame_arena = arena;
    call(limp::sceNpManagerRegisterCallback, lg::kNpManagerCallbackOpd, 0u);
    return 1;
}

// Reads <Tuning><Var name="..." value="..."/>...</Tuning> and hands each
// value to the registered tuning variable of that name, through the
// variable's setter for its type. Returns false only when the file can't be
// parsed or has no <Tuning> element. A <Var> with no matching variable, or
// whose value can't be read, is skipped (the original looks up its name
// attribute and drops the result: probably a stripped log message).
u32 Tuning_Load(const char* path, u8 keep_strings) {
    GuestFrame frame(sizeof(TuningLocals));
    TuningLocals* l = frame.as<TuningLocals>();
    XmlDoc* doc = &l->doc;

    xml::XmlDoc_Construct(doc, path);
    if (xml::XmlDoc_Parse(doc, 0) == 0) {
        xml::XmlDoc_Destruct(doc);
        return 0;
    }
    XmlCursor* root = xml::XmlCursor_FirstChild(&doc->self, kXmlTuning);
    if (!root) {
        xml::XmlDoc_Destruct(doc);
        return 0;
    }

    auto report = [](XmlCursor* node) { xml::XmlCursor_FindAttr(node, kXmlName); };
    const gp<TuningVar>* vars = host<gp<TuningVar>>(lg::kTuningVars);

    for (XmlCursor* node = xml::XmlCursor_FirstChild(root, kXmlVar); node;
         node = xml::XmlCursor_NextSibling(node, kXmlVar)) {
        if (xml::XmlCursor_ReadHash(node, kXmlName, &l->name_hash, 0) != 0) continue;

        // Linear search of the registered variables by name hash.
        TuningVar* var = nullptr;
        const u32 count = global<be_u32>(lg::kTuningVarCount);
        if (count != 0) {
            const u32 hash = l->name_hash;
            u32 i = 0;
            do {
                if (vars[i]->name_hash == hash) {
                    var = vars[i];
                    break;
                }
                ++i;
            } while (count > i);
        }
        if (!var) {
            report(node);
            continue;
        }

        switch (vcall<s32>(var, kVarVfnType)) {
        case kTuningHash:
            if (xml::XmlCursor_ReadHash(node, kXmlValue, &l->hash_value, 0) == 0)
                vcall(var, kVarVfnSetHash, l->hash_value.get());
            else
                report(node);
            break;
        case kTuningString: {
            const char* str = xml::XmlCursor_FindAttrByName(node, host<const char>(kValueAttrName));
            if (!str) {
                report(node);
                break;
            }
            // Compares the name with the value and drops the result (as the original does).
            if (keep_strings) str::String_Compare(var->name, str);
            vcall(var, kVarVfnSetString, str);
            break;
        }
        case kTuningColor:
            if (xml::XmlCursor_ReadColor(node, kXmlValue, l->color, host<const u8>(lg::kTuningDefaultType5)) != 0)
                report(node);
            else
                vcall(var, kVarVfnSetColor, l->color);
            break;
        case kTuningVec3:
            if (xml::XmlCursor_ReadVec3(node, kXmlValue, l->vec3, host<const be_u32>(lg::kTuningDefaultType4)) != 0)
                report(node);
            else
                vcall(var, kVarVfnSetVec3, l->vec3);
            break;
        case kTuningVec2:
            if (xml::XmlCursor_ReadVec2(node, kXmlValue, l->vec2, host<const be_u32>(lg::kTuningDefaultType3)) != 0)
                report(node);
            else
                vcall(var, kVarVfnSetVec2, l->vec2);
            break;
        case kTuningF32:
            // The default (0.0f) comes from the constant at 0x004FC248.
            if (xml::XmlCursor_ReadF32(node, kXmlValue, &l->f32_value, 0.0f) == 0)
                vcall(var, kVarVfnSetF32, l->f32_value.get());
            else
                report(node);
            break;
        case kTuningS32:
            if (xml::XmlCursor_ReadS32(node, kXmlValue, &l->s32_value, 0) == 0)
                vcall(var, kVarVfnSetS32, static_cast<s32>(l->s32_value.get()));
            else
                report(node);
            break;
        case kTuningU8:
            if (xml::XmlCursor_ReadBool(node, kXmlValue, &l->u8_value, 0) != 0)
                report(node);
            else
                vcall(var, kVarVfnSetU8, l->u8_value);
            break;
        default:
            break;
        }
    }

    xml::XmlDoc_Destruct(doc);
    return 1;
}

}  // namespace sly4::gameload

NATIVE(00463000, sly4::gameload::Game_Load)
NATIVE(004FC250, sly4::gameload::Tuning_Load)
