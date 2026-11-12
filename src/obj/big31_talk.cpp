// Game code, large-function sweep part 31: closing a talking object's
// conversation (o->line the current line of the script o->script).
#include "big31.h"

namespace sly4::obj {

struct TalkSub {
    pad<4> _0;
    be_u32 obj;   // +4 (vt +88)
    be_u32 on;    // +8
    pad<4> _12;
};
struct TalkLine {
    pad<4> _0;
    be_u32 subs;   // +4: TalkSub*
    be_u32 count;  // +8
    u8 a, b;       // +12, +13
    pad<2> _14;
};
static_assert(sizeof(TalkLine) == 16);
struct TalkScript {
    pad<148> _0;
    be_u32 lines;   // +148: TalkLine*
    be_u32 count;   // +152
    be_u32 h1, h2;  // +156, +160: the partner handles
};
static_assert(offsetof(TalkScript, h2) == 160);
struct TalkPartnerDef {
    pad<152> _0;
    be_u32 w152;  // +152
};
struct TalkComp {
    pad<12> _0;
    be_u32 text;  // +12 (vt +188 set text, +148)
};
struct TalkPartner {
    pad<4> _0;
    be_u32 live;               // +4: bit 0
    gp<TalkPartnerDef> def;    // +8
    pad<316> _12;
    gp<TalkComp> comp;         // +328
    pad<64> _332;
    be_u32 busy;               // +396: bit 0x02000000
};
static_assert(offsetof(TalkPartner, comp) == 328 && offsetof(TalkPartner, busy) == 396);
// The talking object. Its two partner names are the strings at +362 and
// +490 (the first overlapping the low half of the flags word +360).
struct TalkObj {
    pad<8> _0;
    gp<TalkScript> script;  // +8
    pad<12> _12;
    be_u32 listener;        // +24 (vt +40)
    be_u32 speaker;         // +28
    pad<128> _32;
    be_u32 mode;            // +160
    pad<4> _164;
    be_u32 w168;            // +168
    pad<168> _172;
    be_u32 timeline;        // +340
    be_u32 playing;         // +344 (vt +4 stops it)
    pad<8> _348;
    be_u16 line;            // +356
    pad<2> _358;
    be_u32 flags;           // +360
};
static_assert(offsetof(TalkObj, mode) == 160 && offsetof(TalkObj, timeline) == 340);
static_assert(offsetof(TalkObj, line) == 356 && offsetof(TalkObj, flags) == 360);

namespace {
constexpr u32 Timeline_Play = 0x004E2EF0;     // (const char* name, u32 timeline, 0, 0) -> handle
constexpr u32 kShutdownName = 0x00ADCB70;     // "Default_Shutdown"
constexpr u32 kQuietHash = 0x8DD33161;        // the closing text id when the line has no bytes a / b
constexpr u32 kHashByA = 0x00ADC668;          // u32[]: closing text ids by the first line's a
constexpr u32 kHashByB = 0x00ADC680;          // u32[]: ... by its b (a = 0)
constexpr u32 kHashBoth = 0x00ADC698;         // u32[]: ... when both bytes are equal
constexpr u32 kSayConsts = 0x00371354;        // f32 1.0, f32 0.0 (the Speaker_Say floats)

const TalkLine& Line(const TalkScript& d, u32 i) { return gm<TalkLine>(d.lines + 16 * i); }

// One partner link: with `active` a live partner whose definition has a
// w152 gets name through its component's text object (vt +188 (t, name,
// 0), then vt +148 (t)); otherwise a live partner not busy gets
// Obj_NotifyA362F1C(x, 0, 0). key: the stack copy of the handle.
void Partner(u32 h, bool active, u32 name, be_u32* key) {
    *key = h;
    if (h == 0) return;
    const u32 xe = gm<be_u32>(call<u32>(b31fn::SortedMap_Find3, b31fn::kHandleMap, key));
    if (xe == 0) return;
    TalkPartner& x = gm<TalkPartner>(xe);
    if ((x.live & 1) == 0) return;
    if (active) {
        if (gm<TalkPartnerDef>(x.def.ea).w152 == 0) return;
        const TalkComp& c = gm<TalkComp>(x.comp.ea);
        const u32 t = c.text;
        if (t == 0) return;
        vcall(host<u8>(t), 188, name, 0u);
        vcall(host<u8>(c.text.get()), 148);
    } else {
        if (x.busy & 0x02000000u) return;
        call(b31fn::Obj_NotifyA362F1C, &x, 0u, 0u);
    }
}
}  // namespace

// void Obj_TalkResume(T* o) (called by Obj_ArmTarget, f_003720B0 and
// Obj_SetTalkState_003735C8; none reads r3).
//  1. mode = (mode & 1) | (the current line has both bytes zero ? 18 :
//     16); with a timeline, the playing one is stopped (vt +4) and
//     cleared, and playing = Timeline_Play("Default_Shutdown", timeline,
//     0, 0).
//  2. The two partners (see Partner).
//  3. The closing text: id 0x8DD33161 when the current line has both bytes
//     zero, else by the bytes of line 0 (not the current one: the original
//     indexes the tables with lines[0]). With a speaker and the text,
//     Speaker_Say(speaker, text, 0, ..., voice 0; 1, 0, 0) and flags bit
//     0x04000000 set, else cleared.
//  4. listener vt +40 (listener, 1, 1) when set.
//  5. For the current line (< count): each sub-entry's object gets vt +88
//     (obj, on != 0).
//  6. w168 = 0.
void Obj_TalkResume(TalkObj* o) {
    TalkScript& d = gm<TalkScript>(o->script.ea);  // unchecked
    const TalkLine& e = Line(d, o->line);
    const bool quiet = e.a == 0 && e.b == 0;
    u32 tl = o->timeline;
    const u32 cur = o->playing;
    o->mode = (o->mode & 1) | (quiet ? 18u : 16u);
    if (tl != 0) {
        if (cur != 0) {
            vcall(host<u8>(cur), 4);
            tl = o->timeline;
            o->playing = 0u;
        }
        o->playing = call<u32>(Timeline_Play, kShutdownName, tl, 0u, 0u);
    }
    {
        GuestFrame frame(16);  // the handle copies at 128..140(r1)
        Partner(d.h1, (o->flags & 0x00800000u) != 0, guest(o) + 362, frame.as<be_u32>(12));
        Partner(d.h2, (o->flags & 0x00400000u) != 0, guest(o) + 490, frame.as<be_u32>(4));
    }
    u32 hash = kQuietHash;
    if (!quiet) {
        const TalkLine& l0 = Line(d, 0);
        const u32 a = l0.a, b = l0.b;
        if (a == b) hash = gm<be_u32>(kHashBoth + 4 * a);
        else if (a != 0) hash = gm<be_u32>(kHashByA + 4 * a);
        else hash = gm<be_u32>(kHashByB + 4 * b);
    }
    bool said = false;
    if (o->speaker != 0) {
        const u32 t = call<u32>(b31fn::Text_Lookup34A398, hash);
        if (t != 0) {
            const f64 one = lf(gm<be_f32>(kSayConsts)), zero = lf(gm<be_f32>(kSayConsts + 4));
            call(b31fn::Speaker_Say3514AC, o->speaker.get(), t, 0u, 0u, 0u, 0u, 0u, 0u, 0u, one, zero, zero);
            said = true;
        }
    }
    const u32 f = o->flags;
    o->flags = said ? (f | 0x04000000u) : (f & ~0x04000000u);
    const u32 l = o->listener;
    if (l != 0) vcall(host<u8>(l), 40, 1u, 1u);
    u32 idx = o->line;
    const u32 n = d.count;
    if (idx < n) {
        if (Line(d, idx).count != 0) {
            for (u32 i = 0, off = 0;;) {
                const TalkSub& sub = gm<TalkSub>(Line(d, idx).subs + off);
                const u32 obj = sub.obj;
                if (obj != 0) {
                    vcall(host<u8>(obj), 88, sub.on != 0 ? 1u : 0u);
                    idx = o->line;
                }
                ++i;
                off += 16;
                if (i >= Line(d, idx).count) break;
            }
        }
    }
    o->w168 = 0u;
}

}  // namespace sly4::obj

NATIVE(00371360, sly4::obj::Obj_TalkResume)
