// Game code, medium sweep part 34: a script VM op on KVar data.
//
// The VM's 132-byte records (from +152; +144 u16 count, +146 u16 the
// current record) hold {+0 kind (< 10 valid), +4 u16 offset of the type
// table, +6 u16 offset of the data}; the op node: +2 / +4 s16 parameter
// record indices into the 128-byte table at *0x00F65038 (+24), +6 s16 the
// record of the variable index.
#include "mid34.h"

namespace sly4::obj {

struct VmRecord {
    be_s32 kind;       // +0: < 10 valid
    be_u16 type_off;   // +4: offset of the s32 type table
    be_u16 data_off;   // +6: offset of the data words
    pad<124> _8;
};
static_assert(sizeof(VmRecord) == 132);
struct ScriptVm {
    pad<144> _0;
    be_u16 count;       // +144
    be_u16 current;     // +146
    pad<4> _148;
    // +152: the records
    VmRecord* rec(s32 i) { return reinterpret_cast<VmRecord*>(reinterpret_cast<u8*>(this) + 152) + i; }
};
struct KVarOp {
    pad<2> _0;
    be_s16 param_a;  // +2
    be_s16 param_b;  // +4
    be_s16 index;    // +6: the record holding the variable index
};
struct ParamTables {
    pad<24> _0;
    be_u32 table;  // +24: 128-byte parameter records
};

namespace {
constexpr u32 kFmtIncr34 = 0x00ADDB58u;       // "RIncrementKVarDataDef: VarData (%s) data type (%s) do not support increment!"
constexpr u32 kStrInt34 = 0x00B1FA70u;        // "int"
constexpr u32 kStrHash34 = 0x00B1FA7Fu;       // "hash"
constexpr u32 kOne34 = 0x00393410u;           // f32 1
constexpr u32 kParams34 = 0x00F65038u;        // ParamTables*
// u16 field offsets of the KVar layout
constexpr u32 kOffIndex34 = 0x0106CF80u;      // the index field (u16 at +4 past it)
constexpr u32 kOffSub34 = 0x0106CF7Eu;
constexpr u32 kOffNames34 = 0x0106CF70u;
}  // namespace

// RIncrementKVarDataDef::Run(op, vm): the variable whose index is the
// first u16 (at the u16 offset 0x0106CF80, +4) of record +6 (none when
// that record is out of range or its kind >= 10: read from address 0
// then; index -1: nothing) in the current record: an int is incremented,
// a float gets +1 (fadds), a bool is negated; other types print the error
// with the variable's name (the parameter-table lookup of the op's +4 /
// +2 records) and "int" / "hash". Returns 0.
u32 RIncrementKVarData_Run393418(const KVarOp* op, ScriptVm* vm) {
    const s32 idx = op->index;
    const s16 pb = op->param_b, pa = op->param_a;
    u32 rec = 0;
    if (idx < static_cast<s32>(vm->count)) {
        rec = guest(vm->rec(idx));
        if (!(gm<VmRecord>(rec).kind < 10)) rec = 0;
    }
    const u16 v16 = gm<be_u16>(rec + global<be_u16>(kOffIndex34) + 4);  // quirk: from address 0 when none
    if (static_cast<s16>(v16) == -1) return 0;
    VmRecord* r2 = vm->rec(vm->current);
    const u32 r2a = guest(r2);
    const s32 type = gm<be_s32>(r2a + r2->type_off + static_cast<u32>(v16) * 4u);
    const u32 da = r2a + r2->data_off + (static_cast<u32>(static_cast<s32>(static_cast<s16>(v16))) << 2);
    if (type == 0) {
        be_u32& d = gm<be_u32>(da);
        d = d + 1;
    } else if (type == 2) {
        be_f32& d = gm<be_f32>(da);
        d = static_cast<f32>(lf(d) + lf(global<be_f32>(kOne34)));
    } else if (type == 3) {
        u8& d = gm<u8>(da);
        d = d == 0 ? 1 : 0;
    } else {
        const u32 g = global<be_u32>(kParams34);
        const u32 tab = gm<ParamTables>(g).table;
        const u32 ra = pb != -1 ? tab + (static_cast<u32>(static_cast<s32>(pb)) << 7) : 0u;
        const u32 rb = pa != -1 ? tab + (static_cast<u32>(static_cast<s32>(pa)) << 7) : 0u;
        const u32 q = gm<u8>(rb + 2) > 0 ? rb + global<be_u16>(kOffSub34) : 0u;  // quirk: address 0 when none
        const u32 name = gm<be_u32>(ra + global<be_u16>(kOffNames34) + static_cast<u32>(gm<u8>(q)) * 4u);
        call(fn::String_Format, kFmtIncr34, name, type == 5 ? kStrHash34 : kStrInt34);
    }
    return 0;
}

}  // namespace sly4::obj

NATIVE(00393418, sly4::obj::RIncrementKVarData_Run393418)
