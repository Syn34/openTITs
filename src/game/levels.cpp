// The level list: loading Sly4Levels.xml and each level's Levels.xmb,
// counting and indexing the level map.
#include "levels.h"

#include "../common/common.h"
#include "../xml/xml.h"

namespace sly4::levels {
using namespace opentit::native;
using xml::XmlCursor;
using xml::XmlDoc;

namespace {

constexpr u32 kStringFormat = 0x005534D0;    // (fmt, ...) variadic: reached through its address
constexpr u32 kMemset = 0x00726E54;
constexpr u32 kXmlError = 0x00F668D0;        // u32 cleared before each parse (name?)
constexpr u32 kCurrentXmlPath = 0x010C2C2C;  // char[] (name?)
constexpr u32 kCurrentLevel = 0x010C2C28;    // Level* being registered (name?)
constexpr u32 kFmtLoadFailed = 0x00AEFC84;   // "Fail to load up the level xml file (%s)!"
constexpr u32 kAssetBuilderConfig = 0x00AEFCDC, kLevelsTag = 0x00AEFCF4, kLevelTag = 0x00AEFCFC;
constexpr u32 kLevelDataTag = 0x00AEFC6C;    // "LevelData"
constexpr u32 kScriptsLevels = 0x00AEFCC0;   // "Scripts/Levels/"
constexpr u32 kLevelsXmb = 0x00AEFCD0;       // "/Levels.xmb"
constexpr u32 kEmptyString = 0x00AEFCB8;     // ""
constexpr u32 kPathBufSize = 256;

LevelMap& Map() { return global<LevelMap>(kLevelMap); }

void Push(LevelNode* node) {
    LevelMap& m = Map();
    const s32 d = static_cast<s16>(m.depth.get());
    m.depth = static_cast<u16>(d + 1);
    m.path[d] = guest(node);
}

// In-order iteration with the map's own stack, as the original inlines it.
LevelNode* Iter_Begin() {
    LevelMap& m = Map();
    m.depth = 0;
    LevelNode* node = gref(m.root);  // no null check, as in the original
    if (node)
        while (LevelNode* left = node->left) {
            Push(node);
            node = left;
        }
    m.current = node;
    return node;
}

LevelNode* Iter_Next(LevelNode* cur) {
    LevelMap& m = Map();
    if (LevelNode* right = cur->right) {
        Push(cur);
        LevelNode* n = right;
        while (n->left) {
            Push(n);
            n = n->left;
        }
        m.current = n;
        return n;
    }
    u32 d = m.depth;
    LevelNode* prev = cur;
    for (;;) {
        if ((d & 0xFFFF) == 0) {
            m.current = nullptr;
            return nullptr;
        }
        d -= 1;
        m.depth = static_cast<u16>(d);
        LevelNode* parent = host<LevelNode>(m.path[static_cast<s16>(d)]);
        m.current = parent;
        if (guest(prev) == parent->left.ea) return parent;
        prev = parent;
    }
}

XmlCursor* Xml_FindChild(XmlCursor* node, u32 name) { return call<XmlCursor*>(lvfn::Xml_FindChild, node, name); }
u32 Xml_FindChildren(XmlCursor* node, u32 name) { return call<u32>(lvfn::Xml_FindChildren, node, name); }

}  // namespace

// The number of levels (leaves the iterator at the end).
u32 Levels_Count() {
    u32 n = 0;
    for (LevelNode* cur = Iter_Begin(); cur; cur = Iter_Next(cur)) ++n;
    return n;
}

// The i-th level in key order, or 0 (the iterator stays on it).
Level* Levels_At(s32 i) {
    s32 k = 0;
    for (LevelNode* cur = Iter_Begin(); cur; cur = Iter_Next(cur), ++k)
        if (i == k) return cur->value;
    return nullptr;
}

// Parses AssetBuilderConfig/Levels/Level from the file; returns the parser's
// result, 0 if the elements are missing, or 1 (after a message) if the file
// does not load.
u32 Levels_Load(const char* path) {
    GuestFrame frame(sizeof(XmlDoc));
    XmlDoc* doc = frame.as<XmlDoc>();
    u32 result = 0;
    xml::XmlDoc_Construct(doc, path);
    global<be_u32>(kXmlError) = 0u;
    if (xml::XmlDoc_Parse(doc, 0) == 0) {
        call(kStringFormat, kFmtLoadFailed, path);
        result = 1;
    } else {
        common::String_Assign(host<char>(kCurrentXmlPath), path);
        if (XmlCursor* config = Xml_FindChild(&doc->self, kAssetBuilderConfig))
            if (XmlCursor* list = Xml_FindChild(config, kLevelsTag))
                if (const u32 e = Xml_FindChildren(list, kLevelTag)) result = call<u32>(lvfn::Levels_Parse, e);
    }
    xml::XmlDoc_Destruct(doc);
    return result;
}

// Loads Scripts/Levels/<path>/Levels.xmb once (LevelData); marks the level
// loaded when the parse returns nonzero or the file fails to load.
u8 Level_Register(Level* level) {
    GuestFrame frame(sizeof(XmlDoc) + kPathBufSize);
    XmlDoc* doc = frame.as<XmlDoc>();
    char* buf = frame.as<char>(sizeof(XmlDoc));
    buf[0] = global<char>(kEmptyString);
    call(kMemset, buf + 1, 0u, kPathBufSize - 1);
    common::String_Assign(buf, host<const char>(kScriptsLevels));
    call(lvfn::String_Append, buf, level->path);
    call(lvfn::String_Append, buf, kLevelsXmb);
    global<be_u32>(kCurrentLevel) = guest(level);
    if (level->loaded != 0) return level->loaded;

    u32 result = 0;
    xml::XmlDoc_Construct(doc, buf);
    global<be_u32>(kXmlError) = 0u;
    if (xml::XmlDoc_Parse(doc, 0) == 0) {
        call(kStringFormat, kFmtLoadFailed, buf);
        result = 1;
    } else {
        common::String_Assign(host<char>(kCurrentXmlPath), buf);
        if (const u32 e = Xml_FindChildren(&doc->self, kLevelDataTag)) result = call<u32>(lvfn::Levels_Parse, e);
    }
    xml::XmlDoc_Destruct(doc);
    if (result != 0) {
        level->loaded = 1;
        return 1;
    }
    return level->loaded;
}

}  // namespace sly4::levels

NATIVE(004ECAC4, sly4::levels::Levels_Count)
NATIVE(004ECC50, sly4::levels::Levels_At)
NATIVE(004EBE50, sly4::levels::Levels_Load)
NATIVE(004EBCF4, sly4::levels::Level_Register)
