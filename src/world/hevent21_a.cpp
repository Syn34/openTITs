// World_HandleEvent callees, part 21a: the callees named by the hevent20
// functions (object mode setters, the session stop and the animation clip
// set loader).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kObjModeOff368AC8 = 0x00368AC8;   // (Object*, 0, 0) (name?)
constexpr u32 kObjModeIdle368770 = 0x00368770;  // (Object*) (name?)
constexpr u32 kSpeakerSayId = 0x004A5C90;       // (Speaker*, u32 text id, u8; f32) -> bool (Speaker_SayId4A5C90)
constexpr u32 kEffectSetHash = 0x004CF988;      // (Effect*, u32 hash) -> bool (symbols.txt Effect_SetHash4CF988)
constexpr u32 kModeHash = 0x00F64FB8;           // u32 hash given to Effect_SetHash4CF988 (name?)
constexpr u32 kZeroF = 0x00368864;              // f32 0.0
constexpr u32 kOneF = 0x00368868;               // f32 1.0
constexpr u32 kHandleNotify = 0x0036742C;       // (u32 handle, 1, 1, 0) (symbols.txt Handle_Notify36742C)
constexpr u32 kXmlFindChildren = 0x004F936C;    // (XmlDoc*, const char* tag) -> cursor (symbols.txt)
constexpr u32 kXmlFindAttr = 0x004F9628;        // (cursor, const char* name) -> value (symbols.txt XmlCursor_FindAttrByName)
constexpr u32 kXmlNextSibling = 0x004F8D74;     // (cursor) -> cursor (symbols.txt XmlCursor_NextSiblingAny)
constexpr u32 kStringHashNoCase = 0x0056A710;   // (const char*) -> u32 (symbols.txt)
constexpr u32 kSortedMapFind = 0x0075E3E8;      // (map, const u32* key) -> value slot (symbols.txt SortedMap_Find75E3E8)
constexpr u32 kMemAlloc = 0x0054ED38;           // (size, pool, align) (symbols.txt Mem_Alloc)
constexpr u32 kClipInit4B1708 = 0x004B1708;     // (Clip*, const {XmlDoc*, path, cursor}*) (name?)
constexpr u32 kArrayReserve = 0x00558F3C;       // (array, new capacity) (symbols.txt Array_Reserve)
constexpr u32 kStrAnimSet = 0x00AEE3E8;         // "AnimSet"
constexpr u32 kStrName = 0x00AEE480;            // "Name"
}  // namespace

// void Obj_ModeEnd368D78(Object* o) (name?): o non-null -> 0x00368AC8(o, 0, 0).
DECOMP(00368D78, Obj_ModeEnd368D78) {
    const u64 o = ctx.gpr[3];
    ctx.gpr[4] = o;
    if (static_cast<u32>(o) != 0) call(ctx, kObjModeOff368AC8, {o, 0, 0});
}

// u32 Obj_SetMode368870(Object* o, -, u8 flag, Node* node; f32 t) (name?):
// only while +532 has bit 0x40000000 and neither of 0x03000000. flag sets
// +532 bit 0x20000000; +196 = t. Mode +160 (bits 1..31) 2 / 3: +200 = t
// when t >= 0 (else +192), mode 2, Speaker_SayId4A5C90(+28, +172 id, 0;
// 1.0) result in +532 bit 0x00800000, and with an effect +20
// Effect_SetHash4CF988(e, *0x00F64FB8) result in bit 0x00400000. Modes 0 /
// 4: +204 = node; unless the node's +24 owner has +164 bit 0x8000,
// 0x00368770(o). r4 (the second argument) is not read. Returns r3 as the
// original leaves it: the last call's result, else +28 (or the owner
// pointer / bit tested on the mode 0 / 4 path).
DECOMP(00368870, Obj_SetMode368870) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 flags = load<u32>(o + 532);
    u32 mode = load<u32>(o + 160);
    const u32 alt = load<u32>(o + 192);  // lfs f2, 192: kept as raw bits
    const u32 speaker = load<u32>(o + 28);
    ctx.gpr[7] = flags;
    ctx.gpr[4] = mode;
    ctx.gpr[3] = speaker;
    if ((flags & 0x02000000u) != 0 || (flags & 0x01000000u) != 0 || (flags & 0x40000000u) == 0) return;
    if ((arg<u32>(ctx, 2) & 0xFF) != 0) store<u32>(o + 532, flags | 0x20000000u);
    const f64 t = farg(ctx, 0);
    store<f32>(o + 196, static_cast<f32>(t));
    const u32 m = mode >> 1;
    if (m == 2 || m == 3) {
        if (t >= hevent7::lfs_f64(kZeroF)) store<f32>(o + 200, static_cast<f32>(t));
        else store<u32>(o + 200, alt);
        mode = (mode & 1u) | (2u << 1);
        store<u32>(o + 160, mode);
        const u32 id = load<u32>(o + 172);
        callf(ctx, kSpeakerSayId, {speaker, id, 0}, {hevent7::lfs_f64(kOneF)});
        u32 r = static_cast<u32>(ctx.gpr[3]);
        store<u32>(o + 532, (load<u32>(o + 532) & ~0x00800000u) | ((r & 1u) << 23));
        const u32 e = load<u32>(o + 20);
        if (e == 0) return;
        r = static_cast<u32>(call(ctx, kEffectSetHash, {e, load<u32>(kModeHash)}));
        store<u32>(o + 532, (load<u32>(o + 532) & ~0x00400000u) | ((r & 1u) << 22));
        return;
    }
    if (m != 0 && m != 4) return;
    const u32 node = arg<u32>(ctx, 3);
    store<u32>(o + 204, node);
    if (node != 0) {
        const u32 owner = load<u32>(node + 24);
        ctx.gpr[3] = owner;
        if (owner != 0) {
            const u32 bit = (load<u32>(owner + 164) >> 15) & 1u;
            ctx.gpr[3] = bit;
            if (bit != 0) return;
        }
    }
    call(ctx, kObjModeIdle368770, {o});
}

// u32 Session_Stop2DDAB0(Session* s) (name?): +100 bit 0x40000000 set while
// user block 0's object queue (+80 data, u16 +88 count) is drained from the
// front: kind 4 objects (u8 +18) with a handle (vt +52 into a 3-word out)
// get Handle_Notify36742C(h, 1, 1, 0); each object gets vt +28 (o, 1) and
// vt +4 (o), and is removed by shifting the queue down. Then the bit is
// cleared, block +4 keeps its low 20 bits, the count and s +72 are zeroed.
// Returns the new block +4.
DECOMP(002DDAB0, Session_Stop2DDAB0) {
    const u32 s = arg<u32>(ctx, 0);
    store<u32>(s + 100, load<u32>(s + 100) | 0x40000000u);
    const u32 b = load<u32>(kCurrentWorld) + kUserBlocks;
    Stack frame(ctx, 16);
    const u32 out = frame.addr();
    while (load<u16>(b + 88) != 0) {
        const u32 obj = load<u32>(load<u32>(b + 80));
        store<u32>(out + 0, 0);
        store<u32>(out + 4, 0);
        store<u32>(out + 8, 0);
        if (load<u8>(obj + 18) == 4) {
            if (static_cast<u32>(vcall(ctx, obj, 52, {obj, out})) != 0)
                call(ctx, kHandleNotify, {load<u32>(out), 1, 1, 0});
        }
        vcall(ctx, obj, 28, {obj, 1});
        if (obj != 0) vcall(ctx, obj, 4, {obj});  // checked only after use, as in the original
        u32 n = load<u16>(b + 88);
        if (static_cast<s32>(n - 1) > 0) {
            u32 i = 0;
            do {
                const u32 data = load<u32>(b + 80);
                store<u32>(data + i * 4, load<u32>(data + i * 4 + 4));
                i = (i + 1) & 0xFFFF;
                n = load<u16>(b + 88);
            } while (static_cast<s32>(i) < static_cast<s32>(n - 1));
        }
        store<u16>(b + 88, static_cast<u16>(n - 1));
    }
    store<u32>(s + 100, load<u32>(s + 100) & ~0x40000000u);
    const u32 w = load<u32>(b + 4) & 0x000FFFFFu;
    store<u16>(b + 88, 0);
    store<u32>(b + 4, w);
    store<u32>(s + 72, 0);
    ret(ctx, w);
}

// bool Clip_Load4B20B8(Owner* o, XmlDoc* doc, const char* path) (name?): for
// each "AnimSet" child whose "Name" hash is not in the clip map (+72: u16
// +72 capacity, +74 count, +76 high water, +78 flags, +80 {hash, Clip*}[]),
// a 48-byte clip (zeroed; null when the allocation fails, which is still
// passed on and added) is set up by 0x004B1708(clip, {doc, path, cursor})
// and appended (capacity doubled when full; +78 bit 0 marks it unsorted).
// Returns 1.
DECOMP(004B20B8, Clip_Load4B20B8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 doc = arg<u32>(ctx, 1);
    const u32 path = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 st = frame.addr();  // +0 hash, +4 {doc, path, cursor}
    const u32 map = o + 72;
    u32 c = static_cast<u32>(call(ctx, kXmlFindChildren, {doc, kStrAnimSet}));
    while (c != 0) {
        const u32 name = static_cast<u32>(call(ctx, kXmlFindAttr, {c, kStrName}));
        store<u32>(st, static_cast<u32>(call(ctx, kStringHashNoCase, {name})));
        const u32 slot = static_cast<u32>(call(ctx, kSortedMapFind, {map, st}));
        if (load<u32>(slot) == 0) {
            u32 clip = 0;
            const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {48, 0, 16}));
            if (p != 0) {
                for (u32 k = 0; k <= 20; k += 4) store<u32>(p + k, 0);
                store<u32>(p + 32, 0);
                store<u32>(p + 40, 0);
                store<u32>(p + 36, 0);
                store<u16>(p + 42, 0);
                store<u32>(p + 44, 0);
                clip = p;
            }
            store<u32>(st + 4, doc);
            store<u32>(st + 8, path);
            store<u32>(st + 12, c);
            call(ctx, kClipInit4B1708, {clip, st + 4});
            u32 cnt = load<u16>(o + 74);
            const u32 cap = load<u16>(o + 72);
            if (cnt == cap) {
                call(ctx, kArrayReserve, {map, (cap << 1) & 0xFFFEu});
                cnt = load<u16>(o + 74);
            }
            const u32 data = load<u32>(o + 80);
            store<u32>(data + ((cnt << 3) & 0x0007FFF8u), load<u32>(st));
            store<u32>(data + (load<u16>(o + 74) << 3) + 4, clip);
            const u32 n = load<u16>(o + 74) + 1;
            const u32 high = load<u16>(o + 76);
            const u32 fl = load<u16>(o + 78);
            store<u16>(o + 74, static_cast<u16>(n));
            store<u16>(o + 76, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : n));
            store<u16>(o + 78, static_cast<u16>(fl | 1));
        }
        c = static_cast<u32>(call(ctx, kXmlNextSibling, {c}));
    }
    ret(ctx, 1);
}
