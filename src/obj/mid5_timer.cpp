// Game code, medium sweep part 5: the KTimer variable lookups of the
// action-script timer condition / reaction nodes (as mid4_kvar.cpp's
// typed KVarData lookups, two instances: CIsTimerLargerEqualThan and
// RResetTimer).
//
// A node's variable reference a {+2 / +4 s16 record indices (-1: none),
// +6 s16 the index of its entry in the script} is resolved against the
// script b {+144 u16 count, +146 u16 the current entry, +152 132-byte
// entries: +0 s32 (valid below 10), +4 s16 the type, +6 u16 flags
// (0x8000: error)}. The timer's name comes from the node records (the
// byte at +0 of record 2's sub-block, times 4, plus *(u16*)0x0106CF70,
// indexes record 1); its hash is looked up in the current entry's timer
// list (after its first list: at the entry + ((count * 12 + 23) & ~15),
// {+0 u16 count, the u32 hashes at +*(u16*)+2}) and the index (-1: not
// found) stored to the timer slot (the entry + *(u16*)slot_off + 4).
// Errors go through String_Format ("%s: (Error!) %s" with the name of
// the entry's type) and flag the entry.
#include "mid5.h"

using namespace sly4;
using namespace sly4::mid5;

namespace {

constexpr u32 kNodeTable = 0x00F65038;   // -> +24 the 128-byte node records
constexpr u32 kNameOffset = 0x0106CF70;  // u16
constexpr u32 kFmtError = 0x00ADD2EC;    // "%s: (Error!) %s"

// The node kind's offsets and tables: a condition (C*) or a reaction (R*).
struct TimerKind {
    u32 slot_off;   // u16 global: the slot's offset in the entry
    u32 sub_off;    // u16 global: the sub-block's offset in record 2
    u32 sub_byte;   // record 2's byte: its sub-block present when above 0
    u32 type_tbl;   // the type table: per type, +name_at -> const char** name
    u32 type_size;
    u32 name_at;
    u32 str_null;   // "NULL"
    u32 fmt_none;   // "...: No timer is specified!"
    u32 fmt_find;   // "...: Cannot find the KTimer variable with name (%s)"
};

// b's entry i when i is below its count and the entry's +0 below 10, else 0.
u32 Entry(u32 b, s32 i) {
    if (!(i < static_cast<s32>(load<u16>(b + 144)))) return 0;
    const u32 e = b + 152 + static_cast<u32>(i) * 132;
    return static_cast<s32>(load<u32>(e)) < 10 ? e : 0;
}

// The timer's name (re-read from the records each time, as the original).
u32 TimerName(const TimerKind& k, u32 a) {
    const u32 tbl = load<u32>(kNodeTable);
    const s32 i1 = static_cast<s16>(load<u16>(a + 4));
    const u32 rec1 = i1 == -1 ? 0 : load<u32>(tbl + 24) + (static_cast<u32>(i1) << 7);
    const s32 i2 = static_cast<s16>(load<u16>(a + 2));
    const u32 rec2 = i2 == -1 ? 0 : load<u32>(tbl + 24) + (static_cast<u32>(i2) << 7);
    u32 sub = 0;  // address 0 without a sub-block, as the original
    if (static_cast<s32>(load<u8>(rec2 + k.sub_byte)) > 0) sub = rec2 + load<u16>(k.sub_off);
    return load<u32>(rec1 + load<u16>(kNameOffset) + (static_cast<u32>(load<u8>(sub)) << 2));
}

// The error report for entry e (msg is a String_Format result); e's +6
// |= 0x8000, which r3 is left holding.
void Error(PPUContext& ctx, const TimerKind& k, u32 e, u32 msg) {
    const s32 type = static_cast<s16>(load<u16>(e + 4));
    u32 name = load<u32>(load<u32>(k.type_tbl + static_cast<u32>(type) * k.type_size + k.name_at));
    if (name == 0) name = k.str_null;
    call(ctx, m5fn::String_Format, {kFmtError, name, msg});
    const u32 f = static_cast<u32>(load<u16>(e + 6)) | 0x8000u;
    store<u16>(e + 6, static_cast<u16>(f));
    ret(ctx, f);
}

// void f(VarRef* a, Script* b)
void TimerResolve(PPUContext& ctx, const TimerKind& k) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1);
    const u32 slot = Entry(b, static_cast<s16>(load<u16>(a + 6))) + load<u16>(k.slot_off) + 4;
    const u32 name = TimerName(k, a);
    if (name == 0) {
        // no slot store on this path
        const u32 s = static_cast<u32>(call(ctx, m5fn::String_Format, {k.fmt_none}));
        Error(ctx, k, Entry(b, static_cast<s16>(load<u16>(a + 6))), s);
        return;
    }
    const u32 e = b + 152 + static_cast<u32>(load<u16>(b + 146)) * 132;
    const u32 list = e + ((static_cast<u32>(load<u16>(e)) * 12 + 23) & 0xFFFFFFF0u);
    const u32 h = static_cast<u32>(call(ctx, m5fn::String_HashNoCase, {TimerName(k, a)}));
    const s32 n = load<u16>(list);
    s32 idx = -1;
    if (n > 0) {
        // s16 index, as the original (a count above 0x7FFF never ends a miss)
        for (s32 j = 0;;) {
            if (h == load<u32>(list + load<u16>(list + 2) + ((static_cast<u32>(j) << 2) & 0x3FFFCu))) {
                idx = j;
                break;
            }
            j = static_cast<s16>(j + 1);
            if (!(j < n)) break;
        }
    }
    store<u16>(slot, static_cast<u16>(idx));
    if (idx != -1) {
        ret(ctx, 0x01070000u);  // r3 left as the high half of the u16 globals' address
        return;
    }
    const u32 s = static_cast<u32>(call(ctx, m5fn::String_Format, {k.fmt_find, TimerName(k, a)}));
    Error(ctx, k, Entry(b, static_cast<s16>(load<u16>(a + 6))), s);
}

constexpr TimerKind kCond = {0x0106CF78, 0x0106CF76, 3, 0x00F6503C, 28, 24, 0x00B1FAC8, 0x00ADD850, 0x00ADD808};
constexpr TimerKind kReact = {0x0106CF80, 0x0106CF7E, 2, 0x00F65100, 44, 40, 0x00B1FBAC, 0x00ADDC5C, 0x00ADDC1C};

}  // namespace

// void f(VarRef* a, Script* b): resolve the KTimer variable of a
// CIsTimerLargerEqualThan condition.
DECOMP(0038FDBC, CIsTimerLargerEqualThan_Resolve_0038FDBC) { TimerResolve(ctx, kCond); }

// void f(VarRef* a, Script* b): the same for an RResetTimer reaction.
DECOMP(00394E14, RResetTimer_Resolve_00394E14) { TimerResolve(ctx, kReact); }
