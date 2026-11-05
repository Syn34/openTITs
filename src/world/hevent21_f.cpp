// World_HandleEvent callees, part 21f: an object mode start, the effect
// constructor, the AI control component attach and the mask spawner check.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kObjModeIdle367C20 = 0x00367C20;  // (Object*) (name?)
constexpr u32 kSpeakerSayId = 0x004A5C90;      // (Speaker*, u32 text id, u8; f32) (Speaker_SayId4A5C90)
constexpr u32 kEffectSetHash = 0x004CF988;     // (Effect*, u32 hash) -> bool (symbols.txt Effect_SetHash4CF988)
constexpr u32 kOneF = 0x0036835C;              // f32 1.0
constexpr u32 kStartHash = 0x00F64FA4;         // u32 hash given to Effect_SetHash4CF988 (name?)
constexpr u32 kPlayerFlags = 0x010B1A94;       // u32 flags word (+4 of 0x010B1A90): bit 0x8000 (name?)

constexpr u32 kEffectInit4C761C = 0x004C761C;  // (Effect*) (name?)
constexpr u32 kPtrVectorConstruct = 0x005516E0;  // (vec) (as hevent9.h)

constexpr u32 kStringHashNoCase = 0x0056A710;  // (const char*) -> u32 (symbols.txt)
constexpr u32 kArrayReserve = 0x00558F3C;      // Array_Reserve (Array*, u16 capacity) (symbols.txt)
constexpr u32 kSortedMapRemove = 0x007330C0;   // SortedMap_Remove7330C0 (map, const u32* key) (symbols.txt)
constexpr u32 kObjAttach = 0x000D135C;         // Obj_Attach0D135C (Object*, Component* tmpl, u8 add) (hevent17_a.cpp)
constexpr u32 kTmplRegistry = 0x01056F04;      // template registry (as hevent17_g.cpp)
constexpr u32 kTmplVtBuild = 0x00B0DAAC;       // template vtable while registered
constexpr u32 kAiTmplVt = 0x00B1432C;          // the ControlAI template's vtable
constexpr u32 kStrControlAI = 0x00ACA8B4;      // "ControlAI"

constexpr u32 kMapFind = 0x0076642C;           // (map, const u32* key) -> value slot (symbols.txt Map_Find76642C)
constexpr u32 kMaskInfoMap = 0x0106819C;       // mask spawner info map (name?)
constexpr u32 kStringFormat = 0x005534D0;      // (fmt, ...) (symbols.txt String_Format)
constexpr u32 kFmtNoSpawner = 0x00AD8B88;      // "Couldn't find mask spawner '%s' in the mask info map! ..."
constexpr u32 kFmtEpisode = 0x00B1D308;        // "Episode Index is out of bounds '%i'! Range is [0 - %i]! ..."
constexpr u32 kEpisodes = 12;
constexpr u32 kPartSize = 860;
}  // namespace

// u32 Obj_StartMode368360(Object* o) (name?): only while +216 has bit
// 0x40000000 and not 0x08000000. Mode (+160 bits 1..31) 2 / 3: +200 = 0,
// mode 2, Speaker_SayId4A5C90(+28, +188 id, 0; 1.0) result in +216 bit
// 0x04000000 and with an effect (+20) Effect_SetHash4CF988(e,
// *0x00F64FA4) result in bit 0x02000000; modes 0 / 4: 0x00367C20(o). Then
// +216 bit 0x00800000 = the 0x8000 bit of 0x010B1A94, which is set, and
// +216 bit 0x01000000 cleared. Returns +216.
DECOMP(00368360, Obj_StartMode368360) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 fl = load<u32>(o + 216);
    const u32 speaker = load<u32>(o + 28);
    ctx.gpr[3] = fl;
    ctx.gpr[4] = speaker;
    if ((fl & 0x08000000u) != 0 || (fl & 0x40000000u) == 0) return;
    const u32 mode = load<u32>(o + 160);
    const u32 m = mode >> 1;
    if (m == 2 || m == 3) {
        store<u32>(o + 200, 0);
        store<u32>(o + 160, (mode & 1u) | 4u);
        callf(ctx, kSpeakerSayId, {speaker, load<u32>(o + 188), 0}, {hevent7::lfs_f64(kOneF)});
        u32 r = static_cast<u32>(ctx.gpr[3]);
        store<u32>(o + 216, (load<u32>(o + 216) & ~0x04000000u) | ((r & 1u) << 26));
        const u32 e = load<u32>(o + 20);
        if (e != 0) {
            r = static_cast<u32>(call(ctx, kEffectSetHash, {e, load<u32>(kStartHash)}));
            store<u32>(o + 216, (load<u32>(o + 216) & ~0x02000000u) | ((r & 1u) << 25));
        }
    } else if (m == 0 || m == 4) {
        call(ctx, kObjModeIdle367C20, {o});
    }
    const u32 bit = (load<u32>(kPlayerFlags) >> 15) & 1u;
    store<u32>(o + 216, (load<u32>(o + 216) & ~0x00800000u) | (bit << 23));
    store<u32>(kPlayerFlags, load<u32>(kPlayerFlags) | 0x8000u);
    const u32 v = load<u32>(o + 216) & ~0x01000000u;
    store<u32>(o + 216, v);
    ret(ctx, v);
}

// u32 Effect_Construct(Effect* e, Def* def): clears the header (+0 def,
// +4, +12, u8 +16 = 1), an empty child map (+8 root, iterator +20 / +24
// path / +104 depth / +108 current, as hevent6.h), the +112 vector (u16
// +118 = 4), +132 = 0, +136 flags (low 7 bits kept, 0x100 set); takes a
// reference on def (u16 +4) and finishes with 0x004C761C(e).
DECOMP(004C7A68, Effect_Construct) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 def = arg<u32>(ctx, 1);
    store<u32>(e + 0, 0);
    store<u32>(e + 4, 0);
    store<u32>(e + 8, 0);
    store<u32>(e + 20, e + 8);
    store<u32>(e + 12, 0);
    store<u8>(e + 16, 1);
    hevent6::map_first(e + 20);  // the root was just cleared, so this finds nothing
    store<u8>(e + 16, 1);
    call(ctx, kPtrVectorConstruct, {e + 112});
    const u32 fl = (load<u32>(e + 136) & 0x7Fu) | 0x100u;
    store<u16>(e + 118, 4);
    store<u32>(e + 4, 0);
    store<u32>(e + 132, 0);
    store<u32>(e + 0, def);
    store<u32>(e + 136, fl);
    if (def != 0) store<u16>(def + 4, static_cast<u16>(load<u16>(def + 4) + 1));
    call(ctx, kEffectInit4C761C, {e});
}

// u32 Obj_AttachControlAI128EE0(Object* o) (name?): makes sure the object
// has its "ControlAI" component (+388, also kept in +380): a template on
// the stack (vtable 0x00B1432C; +4 name hash, +8 o, +12 6, u8 +16, +20 0)
// is registered in 0x01056F04 while Obj_Attach0D135C(o, tmpl, 0) builds
// it, the component gets vt +20 (c, o), and the template is removed again.
DECOMP(00128EE0, Obj_AttachControlAI128EE0) {
    const u32 o = arg<u32>(ctx, 0);
    u32 c = load<u32>(o + 388);
    if (c == 0) {
        Stack frame(ctx, 32);
        const u32 t = frame.addr();
        store<u32>(t, kTmplVtBuild);
        const u32 h = static_cast<u32>(call(ctx, kStringHashNoCase, {kStrControlAI}));
        store<u32>(t + 8, o);
        store<u8>(t + 16, 0);
        store<u32>(t + 4, h);
        const u32 arr = kTmplRegistry;
        u32 n = load<u16>(arr + 2);
        const u32 cap = load<u16>(arr);
        store<u32>(t + 12, 6);
        if (n == cap) {
            call(ctx, kArrayReserve, {arr, (cap << 1) & 0xFFFEu});
            n = load<u16>(arr + 2);
        }
        const u32 data = load<u32>(arr + 8);
        store<u32>(data + ((n << 3) & 0x7FFF8u), load<u32>(t + 4));
        store<u32>(data + (static_cast<u32>(load<u16>(arr + 2)) << 3) + 4, t);
        const u32 n2 = load<u16>(arr + 2) + 1;
        const u32 hi = load<u16>(arr + 4), af = load<u16>(arr + 6);
        store<u16>(arr + 2, static_cast<u16>(n2));
        store<u16>(arr + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n2 & 0xFFFF) ? hi : n2));
        store<u16>(arr + 6, static_cast<u16>(af | 1));
        store<u32>(t, kAiTmplVt);
        store<u32>(t + 20, 0);
        const u32 comp = static_cast<u32>(call(ctx, kObjAttach, {o, t, 0}));
        store<u32>(o + 388, comp);
        vcall(ctx, comp, 20, {comp, o});
        store<u32>(t, kTmplVtBuild);
        call(ctx, kSortedMapRemove, {arr, t + 4});
        c = load<u32>(o + 388);
    }
    store<u32>(o + 380, c);
}

// void Mask_CheckSpawner2DF010(Session* s, Spawner* sp, const u16* masks)
// (name?): the spawner's entity (+60) key (+204) is looked up in the mask
// info map 0x0106819C; a missing key / entry, or an entry with both +8
// and +12 set, logs "Couldn't find mask spawner '%s'..." with the entity's
// vt +12 name. Otherwise the entry's part (u8 +0, 860-byte parts at s +20)
// episode (u8 +824; 255 = none, 12 and up logged as out of bounds) is
// checked against masks[episode] bit (u8 entry +1); a clear bit calls the
// spawner's vt +24 (sp). r3 is left as the original leaves it.
DECOMP(002DF010, Mask_CheckSpawner2DF010) {
    const u32 s = arg<u32>(ctx, 0), sp = arg<u32>(ctx, 1), masks = arg<u32>(ctx, 2);
    if (sp == 0 || masks == 0) return;
    const u32 key = load<u32>(load<u32>(sp + 60) + 204);
    u32 e = 0;
    bool found = false;
    if (key != 0) {
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), key);
        e = load<u32>(static_cast<u32>(call(ctx, kMapFind, {kMaskInfoMap, frame.addr()})));
        ctx.gpr[3] = load<u32>(sp);
        if (e != 0 && (load<u32>(e + 8) == 0 || load<u32>(e + 12) == 0)) found = true;
    }
    if (!found) {
        const u32 ent = load<u32>(sp + 60);
        const u32 name = static_cast<u32>(vcall(ctx, ent, 12, {ent}));
        call(ctx, kStringFormat, {kFmtNoSpawner, name});
        return;
    }
    const u32 vt = static_cast<u32>(ctx.gpr[3]);  // the spawner's vtable, read before the checks
    const u32 part = load<u32>(s + 20) + load<u8>(e) * kPartSize;
    if (part == 0) return;
    const u32 ep = load<u8>(part + 824);
    if (ep == 255) return;
    if (!(ep < kEpisodes)) {
        call(ctx, kStringFormat, {kFmtEpisode, ep, kEpisodes});
        return;
    }
    const u32 sh = load<u8>(e + 1) & 0x3F;
    const u32 bit = sh < 32 ? (1u << sh) : 0u;
    if ((load<u16>(masks + ((ep << 1) & 0x1FEu)) & bit) != 0) return;
    const u32 opd = load<u32>(vt + 24);
    ctx.gpr[4] = opd;
    ctx.gpr[5] = load<u32>(opd);
    call_ptr(ctx, opd, {sp});
}
