// Game code, medium sweep part 1: the name-binding owner (the 52-byte
// object NodeOwner_Init builds; Cond_Init and the NewNamed_* factories bind
// their parameters to it by name).
//
// Owner layout: +0 vtable 0x00B225D0, +4 the count of names added, +8 a
// PtrVector of the 96-byte bindings (+8 data, +12 capacity, +16 count, +18
// mark), +28 / +32 PtrVectors of the 56-byte name records (kind 3 in +32,
// every other kind in +28), +48 the next value offset.
//
// Binding (96 bytes): +0 0, +4 the name (50 chars), +56 the kind, +60 k6,
// +64 the value offset, +68 / +72 0, +76 the value (raw), +80 the value as a
// heap string, +84 the name's case-insensitive hash, +88 u8 0, +92 0.
// Kinds: 0 s32, 1 / 2 u32, 3 float, 4 bool, 5 string, 6 / 7 / 8 take 2 / 3 /
// 4 value slots, everything else 1.
#include "mid1.h"

using namespace sly4;
using namespace sly4::mid1;

// Binding* Node_Bind(Owner* o, const char* name, s32 kind, u32 k6, s32
// offset, const void* value): a new 96-byte binding (offset -1 = o->+48),
// its value printed to a heap string when value is given, pushed onto
// o->+8 (a failed allocation pushes 0), and o->+48 advanced by the kind's
// slot count. Returns the binding.
DECOMP(004D3088, Node_Bind) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), kind = arg<u32>(ctx, 2), k6 = arg<u32>(ctx, 3);
    u32 off = arg<u32>(ctx, 4);
    const u32 val = arg<u32>(ctx, 5);
    if (static_cast<s32>(off) == -1) off = load<u32>(o + 48);
    const u32 b = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {96, 0, 16}));
    u32 res = 0;
    if (b != 0) {
        store<u32>(b + 0, 0);
        store<u32>(b + 56, kind);
        store<u32>(b + 60, k6);
        store<u32>(b + 64, off);
        store<u32>(b + 68, 0);
        store<u32>(b + 72, 0);
        store<u32>(b + 80, 0);
        store<u32>(b + 84, 0);
        store<u8>(b + 88, 0);
        store<u32>(b + 92, 0);
        call(ctx, m1fn::String_CopyN, {b + 4, name, 50});
        store<u32>(b + 76, 0);
        store<u32>(b + 84, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {b + 4})));
        if (val != 0) {
            u32 s = 0;  // an unknown kind prints nothing: String_Length(0) (as the original)
            switch (static_cast<s32>(kind)) {
            case 5:
                s = load<u32>(val);
                break;
            case 4: {
                const u32 v = load<u8>(val);
                store<u8>(b + 76, static_cast<u8>(v));
                s = static_cast<u32>(call(ctx, m1fn::String_Format, {kFmtBindInt, v != 0 ? 1u : 0u}));
                break;
            }
            case 3: {
                const u32 w = load<u32>(val);
                store<u32>(b + 76, w);  // lfs / stfs: the word as it is
                const u64 d = LfsBits(w);
                ctx.set_fpr_bits(1, d);  // printf's double in f1 / f2 and r4
                ctx.set_fpr_bits(2, d);
                s = static_cast<u32>(call(ctx, m1fn::String_Format, {kFmtBindFloat, d}));
                break;
            }
            case 2:
            case 1: {
                const u32 w = load<u32>(val);
                store<u32>(b + 76, w);
                s = static_cast<u32>(call(ctx, m1fn::String_Format, {kFmtBindInt, w}));
                break;
            }
            case 0: {
                const u32 w = load<u32>(val);
                store<u32>(b + 76, w);
                s = static_cast<u32>(call(ctx, m1fn::String_Format, {kFmtBindInt, Sx(w)}));
                break;
            }
            default:
                break;
            }
            const u32 len = static_cast<u32>(call(ctx, m1fn::String_Length, {s}));
            const u32 copy = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {static_cast<u32>(len + 1), 0, 16}));
            store<u32>(b + 80, copy);
            call(ctx, m1fn::String_Assign, {copy, s});
            if (kind == 5) store<u32>(b + 76, load<u32>(b + 80));
        }
        res = b;
    }
    PtrVecPush(ctx, o + 8, res);
    u32 slots;
    switch (static_cast<s32>(kind)) {
    case 7: slots = 3; break;
    case 6: slots = 2; break;
    case 8: slots = 4; break;
    default: slots = 1; break;
    }
    const u32 next = load<u32>(o + 48) + slots;
    store<u32>(o + 48, next);
    ctx.gpr[4] = next;
    ret(ctx, res);
}

// bool Owner_AddName(Owner* o, const char* name, s32 kind): the name lists
// +28 / +32 made on first use (20-byte PtrVectors of capacity 5; 0 when the
// allocation fails), a 56-byte record {+0 the name (50 chars), +52 the kind}
// pushed onto +32 for kind 3 and +28 otherwise (0 when the allocation
// fails), and the count +4 incremented. Returns 1.
DECOMP(004D3378, Owner_AddName) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), kind = arg<u32>(ctx, 2);
    for (const u32 slot : {o + 28, o + 32}) {
        if (load<u32>(slot) != 0) continue;
        const u32 v = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {20, 0, 16}));
        if (v != 0) PtrVecNew(ctx, v, 5);
        store<u32>(slot, v);
    }
    const u32 e = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {56, 0, 16}));
    if (e != 0) {
        store<u32>(e + 52, kind);
        call(ctx, m1fn::String_CopyN, {e, name, 50});
    }
    PtrVecPush(ctx, load<u32>(o + (static_cast<s32>(kind) == 3 ? 32 : 28)), e);
    const u32 n = load<u32>(o + 4) + 1;
    store<u32>(o + 4, n);
    ctx.gpr[4] = n;
    ret(ctx, 1u);
}

// Owner::~Owner() (the body of Delete_004D3040): vtable 0x00B225D0; every
// binding popped off +8 is freed with its name list (+68, +0 entries) and
// its string +80; the name lists +28 / +32 are emptied (each record freed,
// nulls included) and freed; then the +8 vector is cleared.
DECOMP(004D2D90, Owner_Destruct) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B225D0);
    // Pop the bindings (a swap-remove of the last element, so nothing moves).
    for (;;) {
        const u32 c = load<u16>(o + 16);
        if (c == 0) break;
        const u32 data = load<u32>(o + 8);
        const u32 i = c - 1;
        store<u16>(o + 16, static_cast<u16>(i));
        const u32 b = load<u32>(data + 4 * i);
        if (b == 0) continue;
        if (load<u32>(b + 68) != 0) {
            for (u32 k = 0; k < load<u32>(b + 0); ++k) call(ctx, m1fn::Mem_FreeFrom, {load<u32>(load<u32>(b + 68) + 4 * k), 0});
            call(ctx, m1fn::Mem_FreeFrom, {load<u32>(b + 68), 0});
            store<u32>(b + 68, 0);
        }
        FreeSlot(ctx, b + 80);
        call(ctx, m1fn::Mem_FreeFrom, {b, 0});
    }
    store<u16>(o + 16, 0);
    for (const u32 slot : {o + 28, o + 32}) {
        if (load<u32>(slot) == 0) continue;
        for (;;) {
            const u32 v = load<u32>(slot);
            const u32 c = load<u16>(v + 8);
            if (c == 0) break;
            const u32 i = c - 1;
            store<u16>(v + 8, static_cast<u16>(i));
            call(ctx, m1fn::Mem_FreeFrom, {load<u32>(load<u32>(v) + 4 * i), 0});
        }
        const u32 v = load<u32>(slot);
        store<u16>(v + 8, 0);
        const u32 w = load<u32>(slot);
        if (w == 0) continue;
        call(ctx, m1fn::PtrVector_Clear, {w});
        call(ctx, m1fn::Nop, {w});
        call(ctx, m1fn::Mem_FreeFrom, {w, 0});
        store<u32>(slot, 0);
    }
    call(ctx, m1fn::PtrVector_Clear, {o + 8});
    call(ctx, m1fn::Nop, {o + 8});
}

namespace {
// One case of Obj_RegisterParam: Node_Bind(o->+8, d->+4, kind, mode, -1,
// value) with value = a zero local (a byte 1 for bools) when the mode is 2
// and 0 otherwise (local < 0: always 0); d->+16 = the binding, whose +76
// takes d->+20 as copy says; then Set_Insert_0079B55C(out, o + 48, &d).
// Stack offsets are the original's (sp + 112 = the key d).
enum Copy { kNone, kWord, kBool };
void RegisterBind(PPUContext& ctx, u32 o, u32 d, u32 kind, s32 local, u32 out, Copy copy) {
    Stack frame(ctx, 80);
    const u32 sp = frame.addr() - 112;
    const u32 mode = load<u32>(d + 12);
    u32 value = 0;
    if (local >= 0) {
        if (kind == 4) store<u8>(sp + static_cast<u32>(local), 1);
        else store<u32>(sp + static_cast<u32>(local), 0);
        if (static_cast<s32>(mode) == 2) value = sp + static_cast<u32>(local);
    }
    const u32 b = static_cast<u32>(call(ctx, m1fn::Node_Bind, {load<u32>(o + 8), load<u32>(d + 4), kind, Sx(mode), Sx(0xFFFFFFFFu), value}));
    store<u32>(d + 16, b);
    if (copy == kWord) store<u32>(b + 76, load<u32>(d + 20));  // b == 0 writes to 76 (as the original)
    if (kind == 3) ctx.set_fpr_bits(1, LfsBits(load<u32>(d + 20)));  // lfs / stfs: f1 = the value
    else if (copy == kBool) store<u32>(b + 76, load<u8>(d + 20) != 0 ? 1u : 0u);
    store<u32>(sp + 112, d);
    call(ctx, m1fn::Set_Insert_0079B55C, {sp + out, o + 48, sp + 112});
}
}  // namespace

// void Obj_RegisterParam(Obj* o, ParamDesc* d): binds the parameter d {+4
// name, +8 kind 0-9, +12 mode, +16 out binding, +20 default value} to the
// owner o->+8 and adds d to the set o->+48. Kinds above 9 or below 0 do
// nothing (r3 = the kind). Kinds 6-9 bind without a value; 0-5 pass a
// zeroed value when the mode is 2 and copy the default into the binding.
DECOMP(004F7128, Obj_RegisterParam) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    const u32 kind = load<u32>(d + 8);
    switch (static_cast<s32>(kind)) {
    case 0: RegisterBind(ctx, o, d, 0, 176, 116, kWord); break;
    case 1: RegisterBind(ctx, o, d, 1, 172, 120, kWord); break;
    case 2: RegisterBind(ctx, o, d, 2, 168, 124, kWord); break;
    case 3: RegisterBind(ctx, o, d, 3, 164, 128, kWord); break;  // lfs / stfs: the word as it is
    case 4: RegisterBind(ctx, o, d, 4, 160, 132, kBool); break;
    case 5: RegisterBind(ctx, o, d, 5, 156, 136, kWord); break;
    case 6: RegisterBind(ctx, o, d, 6, -1, 140, kNone); break;
    case 7: RegisterBind(ctx, o, d, 7, -1, 144, kNone); break;
    case 8: RegisterBind(ctx, o, d, 8, -1, 148, kNone); break;
    case 9: RegisterBind(ctx, o, d, 9, -1, 152, kNone); break;
    default: ret(ctx, kind); break;
    }
}
