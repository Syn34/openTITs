// The level list: loading Sly4Levels.xml and each level's Levels.xmb,
// counting and indexing the level map.
#include "levels.h"

using namespace sly4;
using namespace sly4::levels;

namespace {

constexpr u32 kXmlDocConstruct = 0x004FA888, kXmlDocParse = 0x004FAF10, kXmlDocDestruct = 0x004FA9C0;
constexpr u32 kStringAssign = 0x00555F80, kStringFormat = 0x005534D0, kMemset = 0x00726E54;
constexpr u32 kXmlDocSize = 2240;
constexpr u32 kXmlError = 0x00F668D0;        // u32 cleared before each parse (name?)
constexpr u32 kCurrentXmlPath = 0x010C2C2C;  // char[] (name?)
constexpr u32 kCurrentLevel = 0x010C2C28;    // Level* being registered (name?)
constexpr u32 kFmtLoadFailed = 0x00AEFC84;   // "Fail to load up the level xml file (%s)!"
constexpr u32 kAssetBuilderConfig = 0x00AEFCDC, kLevelsTag = 0x00AEFCF4, kLevelTag = 0x00AEFCFC;
constexpr u32 kLevelDataTag = 0x00AEFC6C;    // "LevelData"
constexpr u32 kScriptsLevels = 0x00AEFCC0;   // "Scripts/Levels/"
constexpr u32 kLevelsXmb = 0x00AEFCD0;       // "/Levels.xmb"
constexpr u32 kEmptyString = 0x00AEFCB8;     // ""

void Push(u32 node) {
    const s32 d = static_cast<s16>(load<u16>(kLevelMap + kMapDepth));
    store<u16>(kLevelMap + kMapDepth, static_cast<u16>(d + 1));
    store<u32>(kLevelMap + kMapPath + static_cast<u32>(d) * 4, node);
}

// In-order iteration with the map's own stack, as the original inlines it.
u32 Iter_Begin() {
    const u32 m = kLevelMap;
    store<u16>(m + kMapDepth, 0);
    u32 node = load<u32>(load<u32>(m + kMapRoot));
    if (node != 0)
        while (const u32 left = load<u32>(node + kNodeLeft)) {
            Push(node);
            node = left;
        }
    store<u32>(m + kMapCurrent, node);
    return node;
}

u32 Iter_Next(u32 cur) {
    const u32 m = kLevelMap;
    if (const u32 right = load<u32>(cur + kNodeRight)) {
        Push(cur);
        u32 n = right;
        while (load<u32>(n + kNodeLeft) != 0) {
            Push(n);
            n = load<u32>(n + kNodeLeft);
        }
        store<u32>(m + kMapCurrent, n);
        return n;
    }
    u32 d = load<u16>(m + kMapDepth);
    u32 prev = cur;
    for (;;) {
        if ((d & 0xFFFF) == 0) {
            store<u32>(m + kMapCurrent, 0);
            return 0;
        }
        d -= 1;
        store<u16>(m + kMapDepth, static_cast<u16>(d));
        const u32 parent = load<u32>(m + kMapPath + static_cast<u32>(static_cast<s32>(static_cast<s16>(d))) * 4);
        store<u32>(m + kMapCurrent, parent);
        if (prev == load<u32>(parent + kNodeLeft)) return parent;
        prev = parent;
    }
}

}  // namespace

// u32 Levels_Count(): the number of levels (leaves the iterator at the end).
DECOMP(004ECAC4, Levels_Count) {
    u32 n = 0;
    for (u32 cur = Iter_Begin(); cur != 0; cur = Iter_Next(cur)) ++n;
    ret(ctx, n);
}

// Level* Levels_At(s32 i): the i-th level in key order, or 0 (the iterator
// stays on it).
DECOMP(004ECC50, Levels_At) {
    const s32 i = arg<s32>(ctx, 0);
    s32 k = 0;
    for (u32 cur = Iter_Begin(); cur != 0; cur = Iter_Next(cur), ++k)
        if (i == k) return ret(ctx, load<u32>(cur + kNodeValue));
    ret(ctx, 0u);
}

// u32 Levels_Load(const char* xml)
// Parses AssetBuilderConfig/Levels/Level from the file; returns the parser's
// result, 0 if the elements are missing, or 1 (after a message) if the file
// does not load.
DECOMP(004EBE50, Levels_Load) {
    const u32 path = arg<u32>(ctx, 0);
    Stack frame(ctx, kXmlDocSize);
    const u32 doc = frame.addr();
    u32 result = 0;
    call(ctx, kXmlDocConstruct, {doc, path});
    store<u32>(kXmlError, 0);
    if (static_cast<u32>(call(ctx, kXmlDocParse, {doc, 0})) == 0) {
        call(ctx, kStringFormat, {kFmtLoadFailed, path});
        result = 1;
    } else {
        call(ctx, kStringAssign, {kCurrentXmlPath, path});
        const u32 config = static_cast<u32>(call(ctx, lvfn::Xml_FindChild, {doc, kAssetBuilderConfig}));
        if (config != 0) {
            const u32 list = static_cast<u32>(call(ctx, lvfn::Xml_FindChild, {config, kLevelsTag}));
            if (list != 0) {
                const u32 e = static_cast<u32>(call(ctx, lvfn::Xml_FindChildren, {list, kLevelTag}));
                if (e != 0) result = static_cast<u32>(call(ctx, lvfn::Levels_Parse, {e}));
            }
        }
    }
    call(ctx, kXmlDocDestruct, {doc});
    ret(ctx, result);
}

// u8 Level_Register(Level* level)
// Loads Scripts/Levels/<path>/Levels.xmb once (LevelData); marks the level
// loaded when the parse returns nonzero or the file fails to load.
DECOMP(004EBCF4, Level_Register) {
    const u32 level = arg<u32>(ctx, 0);
    Stack frame(ctx, kXmlDocSize + 256);
    const u32 doc = frame.addr(), buf = doc + kXmlDocSize;
    ref<u8>(buf) = ref<u8>(kEmptyString);
    call(ctx, kMemset, {buf + 1, 0, 255});
    call(ctx, kStringAssign, {buf, kScriptsLevels});
    call(ctx, lvfn::String_Append, {buf, level + kLevelPath});
    call(ctx, lvfn::String_Append, {buf, kLevelsXmb});
    store<u32>(kCurrentLevel, level);
    if (ref<u8>(level + kLevelLoaded) != 0) return ret(ctx, static_cast<u32>(ref<u8>(level + kLevelLoaded)));

    u32 result = 0;
    call(ctx, kXmlDocConstruct, {doc, buf});
    store<u32>(kXmlError, 0);
    if (static_cast<u32>(call(ctx, kXmlDocParse, {doc, 0})) == 0) {
        call(ctx, kStringFormat, {kFmtLoadFailed, buf});
        result = 1;
    } else {
        call(ctx, kStringAssign, {kCurrentXmlPath, buf});
        const u32 e = static_cast<u32>(call(ctx, lvfn::Xml_FindChildren, {doc, kLevelDataTag}));
        if (e != 0) result = static_cast<u32>(call(ctx, lvfn::Levels_Parse, {e}));
    }
    call(ctx, kXmlDocDestruct, {doc});
    if (result != 0) {
        ref<u8>(level + kLevelLoaded) = 1;
        return ret(ctx, 1u);
    }
    ret(ctx, static_cast<u32>(ref<u8>(level + kLevelLoaded)));
}
