// World_HandleEvent callees, part 13a: small leaves under the hevent12
// functions (tree search, sub-object init, upgrade unlock, set xml loads).
#include "hevent13.h"

using namespace sly4;
using namespace sly4::hevent13;

namespace {
// Tree node (as hevent6.h): +4 u32 key, +8 value, +12 left (smaller keys), +16 right.
constexpr u32 kTnKey = 4, kTnValue = 8, kTnLeft = 12, kTnRight = 16;

// The WorldObj_ForEach callback that Body_SetMode4A5C20 passes (name?).
constexpr u32 kSetModeCallbackOpd = 0x00F39F70;

// Sub-object (as Obj_BaseConstruct35695C +48): +0 vtable, +4/+8/+12 0, +16
// u32 (bit 0 kept, the rest cleared; byte +19 then set to 1), +32/+48/+64
// vec3 (zero), +80 0.
constexpr u32 kSubVtable = 0x00B21BA8;
constexpr u32 kZeroVec = 0x01164B40;

// Upgrades (as src/game/meta.h): 84 x 152 bytes at 0x010BE8BC, +144 the job
// unlock hash; save data (World +80) +3948 one flag byte per upgrade, +348
// u32 flags.
constexpr u32 kUpgradeCount = 0x010C1A9C;
constexpr u32 kUpgradeJobUnlock = 0x010BE8BC + 144;
constexpr u32 kUpgradeSize = 152;
constexpr u32 kSaveUpgradeFlags = 3948, kSaveFlags348 = 348;
constexpr u8 kUpgradeOwned = 1, kUpgradeUnlocked = 5;
constexpr u32 kSaveDirty = 0x80;

constexpr u32 kFileName = 4;  // File +4: path
}  // namespace

// bool Tree_Find793080(Tree*, u32 key, Value* out, Node* root) (name?): binary
// search (unsigned keys, smaller to the left); on a hit *out = node +8.
DECOMP(00793080, Tree_Find793080) {
    const u32 key = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    for (u32 n = arg<u32>(ctx, 3); n != 0;) {
        const u32 k = load<u32>(n + kTnKey);
        if (k == key) {
            store<u32>(out, load<u32>(n + kTnValue));
            ret(ctx, 1u);
            return;
        }
        n = key < k ? load<u32>(n + kTnLeft) : load<u32>(n + kTnRight);
    }
    ret(ctx, 0u);
}

// void Body_SetMode4A5C20(WorldObj* body, u32 group) (name?): sets the
// group on body and, through WorldObj_ForEach, on its children.
DECOMP(004A5C20, Body_SetMode4A5C20) {
    const u32 body = arg<u32>(ctx, 0), group = arg<u32>(ctx, 1);
    if (body == 0) return;  // r3 stays 0
    call(ctx, h13fn::WorldObj_SetGroup, {body, group});
    Stack frame(ctx, 16);  // r1+112: the group
    store<u32>(frame.addr(), group);
    call(ctx, hevent9::h9fn::WorldObj_ForEach, {body, kSetModeCallbackOpd, frame.addr(), 0});
}

// void SubObj_Init4A0EE0(SubObj* s) (name?): the sub-object constructor.
DECOMP(004A0EE0, SubObj_Init4A0EE0) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 keep = load<u32>(s + 16) & 1;
    store<u32>(s + 4, 0);
    store<u32>(s + 8, 0);
    store<u16>(s + 16, 0);
    store<u32>(s + 12, 0);
    store<u32>(s + 16, keep);
    store<u8>(s + 19, static_cast<u8>(keep | 1));
    store<u32>(s + 32, load<u32>(kZeroVec));
    store<u32>(s + 36, load<u32>(kZeroVec + 4));
    store<u32>(s + 40, load<u32>(kZeroVec + 8));
    store<u32>(s + 48, load<u32>(kZeroVec));
    store<u32>(s + 0, kSubVtable);
    store<u32>(s + 52, load<u32>(kZeroVec + 4));
    store<u32>(s + 56, load<u32>(kZeroVec + 8));
    store<u32>(s + 64, load<u32>(kZeroVec));
    store<u32>(s + 80, 0);
    store<u32>(s + 68, load<u32>(kZeroVec + 4));
    store<u32>(s + 72, load<u32>(kZeroVec + 8));
    ctx.fpr[1] = hevent7::lfs_f64(kZeroVec + 8);  // left by the last lfs
}

// u32 Upgrade_UnlockJob49E038(u32 job_hash) (name?): every upgrade whose
// job unlock hash is job_hash and that is not owned gets flags |= 5 in the
// save data, and the save's +348 dirty bit. Returns job_hash.
DECOMP(0049E038, Upgrade_UnlockJob49E038) {
    const u32 hash = arg<u32>(ctx, 0);
    const u32 save = load<u32>(hevent::kCurrentWorld) + hevent::kSaveData;
    u32 n = load<u32>(kUpgradeCount);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(kUpgradeJobUnlock + i * kUpgradeSize) != hash) continue;
        const u32 f = save + kSaveUpgradeFlags + i;
        const u8 v = load<u8>(f);
        if ((v & kUpgradeOwned) != 0) continue;
        store<u8>(f, static_cast<u8>(v | kUpgradeUnlocked));
        store<u32>(save + kSaveFlags348, load<u32>(save + kSaveFlags348) | kSaveDirty);
        n = load<u32>(kUpgradeCount);
    }
    ret(ctx, hash);
}

// u32 Set_AddXmlFile4BE374(Set* s, File* file, u8 flag) (name?): loads an
// XMLB document from file and adds it under the file's path (+4).
DECOMP(004BE374, Set_AddXmlFile4BE374) {
    const u32 s = arg<u32>(ctx, 0), file = arg<u32>(ctx, 1);
    const u8 flag = arg<u8>(ctx, 2);
    Stack frame(ctx, kXmlDocFrame);
    const u32 doc = frame.addr();
    call(ctx, h13fn::XmlDoc_Init4FA7A4, {doc});
    u32 r = 0;
    if (static_cast<u32>(call(ctx, h13fn::XmlDoc_LoadXmlb, {doc, file, 0})) != 0)
        r = static_cast<u32>(call(ctx, h13fn::Set_AddXml, {s, file + kFileName, doc, flag}));
    call(ctx, h13fn::XmlDoc_Destruct, {doc});
    ret(ctx, r);
}

// u32 Set_AddXmlPath4BDD08(Set* s, const char* path, u8 flag) (name?):
// loads and parses the document at path and adds it under path; 0 when it
// does not parse.
DECOMP(004BDD08, Set_AddXmlPath4BDD08) {
    const u32 s = arg<u32>(ctx, 0), path = arg<u32>(ctx, 1);
    const u8 flag = arg<u8>(ctx, 2);
    Stack frame(ctx, kXmlDocFrame);
    const u32 doc = frame.addr();
    call(ctx, h13fn::XmlDoc_Construct, {doc, path});
    if (static_cast<u32>(call(ctx, h13fn::XmlDoc_Parse, {doc, 0})) == 0) {
        call(ctx, h13fn::XmlDoc_Destruct, {doc});
        ret(ctx, 0u);
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, h13fn::Set_AddXml, {s, path, doc, flag}));
    call(ctx, h13fn::XmlDoc_Destruct, {doc});
    ret(ctx, r);
}
