// World objects, part 13: the smallest still-recompiled functions of
// 0x00550000-0x005AA000: empty hooks, constant returns, vtable resets,
// field accessors and the libc wrappers (strstr, strchr, strtol, ...).
// The wrappers pass every register they don't touch straight through.
#include "obj13.h"

using namespace sly4;
using namespace sly4::obj13;

namespace {

constexpr u32 libc_strstr = 0x0071EEF8;
constexpr u32 libc_strchr = 0x00723678;
constexpr u32 libc_71EDAC = 0x0071EDAC;         // (name?)
constexpr u32 libc_72354C = 0x0072354C;         // String_Append's callee (strcat?) (name?)
constexpr u32 libc_vsnprintf = 0x0071F36C;
constexpr u32 libc_strtol = 0x00721D18;         // (s, end, base) (name?)
constexpr u32 libc_strtod = 0x00721044;         // (s, end) -> f1 (name?)
constexpr u32 libc_strtol_r = 0x0071F284;       // (s, end, base) (name?)
constexpr u32 Fn_575D18 = 0x00575D18;
constexpr u32 kHddFreeSizeKB = 0x00F88768;      // system/boot.h: s32 from cellGameBootCheck
constexpr u32 kBankState = 0x01164DEC;          // {u32, u32, u32} (name?)
constexpr u32 kVtbl560B28 = 0x00B242E8, kVtbl562EB0 = 0x00B24310, kVtbl564F10 = 0x00B24110;  // (name?)

}  // namespace

// void Bank_DestroyNopA() / Bank_DestroyNopB(): empty hooks Bank_Destroy calls (name?).
DECOMP(00556764, Bank_DestroyNopA) {}
DECOMP(0056A70C, Bank_DestroyNopB) {}

// void WorldObj_ReleaseRes(WorldObj*): empty in this build.
DECOMP(00558F34, WorldObj_ReleaseRes) {}

// void Array_Init(Array*): empty in this build.
DECOMP(00558F38, Array_Init) {}

// void Image_Nop() / Archive_Nop570B90(): empty (name?).
DECOMP(00570B88, Image_Nop) {}
DECOMP(00570B90, Archive_Nop570B90) {}

// Constant callbacks (called through pointers or by f_0055E56C / f_0055E7C0) (name?).
DECOMP(005530E8, Callback_Return0) { ctx.gpr[3] = 0; }
DECOMP(00564C54, Callback_Return1) { ctx.gpr[3] = 1; }
DECOMP(0056F690, Archive_Return1A) { ctx.gpr[3] = 1; }
DECOMP(00571164, Archive_Return1B) { ctx.gpr[3] = 1; }

// u32 Obj_GetField4(const void* p): p->+4 (name?).
DECOMP(00556BF0, Obj_GetField4) { ret(ctx, load<u32>(arg<u32>(ctx, 0) + 4)); }

// void Cursor_Reset(Cursor* c): +4 = +0 (r4 = the value) (name?).
DECOMP(0056DFC8, Cursor_Reset) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 v = load<u32>(c);
    store<u32>(c + 4, v);
    ctx.gpr[4] = v;
}

// u32 Cursor_Alloc(Cursor* c, u32 align, u32 size): +4 rounded up to align
// (a power of two), then moved on by size; returns the aligned position
// (r4 = it, r5 = the new +4, r6 = ~(align - 1), r7 = the old +4) (name?).
DECOMP(0056DF9C, Cursor_Alloc) {
    const u32 c = arg<u32>(ctx, 0);
    const u64 align = ctx.gpr[4], size = ctx.gpr[5];
    const u32 cur = load<u32>(c + 4);
    const u64 mask = ~(align - 1);
    const u32 p = static_cast<u32>((cur + align - 1) & mask);
    const u64 next = p + size;
    store<u32>(c + 4, static_cast<u32>(next));
    ctx.gpr[3] = p;
    ctx.gpr[4] = p;
    ctx.gpr[5] = next;
    ctx.gpr[6] = mask;
    ctx.gpr[7] = cur;
}

// The vtable resets of three destructors (+0 = the base vtable; r4 = it) (name?).
DECOMP(00560B28, Obj_ResetVtbl560B28) {
    store<u32>(arg<u32>(ctx, 0), kVtbl560B28);
    ctx.gpr[4] = kVtbl560B28;
}
DECOMP(00562EB0, Obj_ResetVtbl562EB0) {
    store<u32>(arg<u32>(ctx, 0), kVtbl562EB0);
    ctx.gpr[4] = kVtbl562EB0;
}
DECOMP(00562F08, Obj_ResetVtbl562F08) {
    store<u32>(arg<u32>(ctx, 0), kVtbl562EB0);
    ctx.gpr[4] = kVtbl562EB0;
}

// void Obj_Construct564F10(void* o): vtable 0x00B24110, +4 / +8 = 0 (r4 / r5 = the vtable / 0) (name?).
DECOMP(00564F10, Obj_Construct564F10) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 4, 0);
    store<u32>(o, kVtbl564F10);
    store<u32>(o + 8, 0);
    ctx.gpr[4] = kVtbl564F10;
    ctx.gpr[5] = 0;
}

// bool Bank_ClearField4(void* p): +4 = 0; returns 1 (r4 = 0) (name?).
DECOMP(00569378, Bank_ClearField4) {
    store<u32>(arg<u32>(ctx, 0) + 4, 0);
    ctx.gpr[4] = 0;
    ctx.gpr[3] = 1;
}

// s32 Game_HddFreeSizeKB(): system/boot.h's kHddFreeSizeKB (name?).
DECOMP(005721B4, Game_HddFreeSizeKB) { ret(ctx, load<s32>(kHddFreeSizeKB)); }

// void Obj_SetEmpty148(void* o, u8 v): +148 = (v == 0) (r4 = it) (name?).
DECOMP(005630C0, Obj_SetEmpty148) {
    const u32 v = (arg<u32>(ctx, 1) & 0xFF) == 0 ? 1u : 0u;
    store<u8>(arg<u32>(ctx, 0) + 148, static_cast<u8>(v));
    ctx.gpr[4] = v;
}

// void Bank_ResetState(): the bank state's +0 / +4 = 0 (r3 / r4 / r5 as left) (name?).
DECOMP(00564F2C, Bank_ResetState) {
    store<u32>(kBankState, 0);
    store<u32>(kBankState + 4, 0);
    ctx.gpr[3] = 0x01160000;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = kBankState;
}

// void Bank_SetState(u32 v): the bank state = {0, 0, v} (+0, +8, +4 in that order) (name?).
DECOMP(00564F44, Bank_SetState) {
    const u32 v = arg<u32>(ctx, 0);
    store<u32>(kBankState, 0);
    store<u32>(kBankState + 8, v);
    store<u32>(kBankState + 4, 0);
    ctx.gpr[4] = 0x01160000;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = kBankState;
}

// void Obj_ClearField8(void* o): +8 = 0 unless it already is (r4 = 0) (name?).
DECOMP(00565A88, Obj_ClearField8) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 8) != 0) store<u32>(o + 8, 0);
    ctx.gpr[4] = 0;
}

// void Obj_ClearPair(void* o, u32 i): +132 + 4i and +140 + 4i = 0 (name?).
DECOMP(00563098, Obj_ClearPair) {
    const u32 b = static_cast<u32>(ctx.gpr[3] + (ctx.gpr[4] << 2));
    store<u32>(b + 132, 0);
    store<u32>(b + 140, 0);
    ctx.gpr[3] = b + 140;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = b + 132;
}

// const char* Obj_NameIfSet(Obj** p): (*p)->+48 when its first byte is
// nonzero, else 0 (r4 = the result, r5 = the byte) (name?).
DECOMP(00571DA0, Obj_NameIfSet) {
    const u32 s = load<u32>(load<u32>(arg<u32>(ctx, 0)) + 48);
    const u32 b = load<u8>(s);
    const u32 r = b != 0 ? s : 0u;
    ctx.gpr[5] = b;
    ctx.gpr[4] = r;
    ctx.gpr[3] = r;
}

// void Fn_576BB4(...): f_00575D18 with the same registers (name?).
DECOMP(00576BB4, Fn_576BB4) { call(ctx, Fn_575D18, {ctx.gpr[3]}); }

// char* String_Find(const char* s, const char* sub): strstr.
DECOMP(00555FCC, String_Find) { call(ctx, libc_strstr, {ctx.gpr[3], ctx.gpr[4]}); }

// char* String_FindChar(const char* s, s32 c): strchr.
DECOMP(00555FF0, String_FindChar) { call(ctx, libc_strchr, {ctx.gpr[3], ctx.gpr[4]}); }

// ... String_Thunk71EDAC(...): f_0071EDAC with the same registers (name?).
DECOMP(00556014, String_Thunk71EDAC) { call(ctx, libc_71EDAC, {ctx.gpr[3]}); }

// ... String_Append(...): f_0072354C with the same registers.
DECOMP(005566A8, String_Append) { call(ctx, libc_72354C, {ctx.gpr[3]}); }

// s32 String_VFormat(char* buf, u32 size, const char* fmt, va_list ap):
// vsnprintf with the size zero-extended (name?).
DECOMP(00555D48, String_VFormat) { call(ctx, libc_vsnprintf, {ctx.gpr[3], ctx.gpr[4] & 0xFFFFFFFFu}); }

// void Mem_FreeCallback(void* self, void* p): Mem_FreePool(p, 0, 0) (name?).
DECOMP(00550E40, Mem_FreeCallback) { call(ctx, obj4::o4fn::Mem_FreePool, {ctx.gpr[4], 0, 0}); }

// s32 String_ToInt10(const char* s): (s32)strtol(s, 0, 10) (name?).
DECOMP(005566CC, String_ToInt10) {
    const u64 r = call(ctx, libc_strtol, {ctx.gpr[3], 0, 10});
    ret(ctx, static_cast<s32>(static_cast<u32>(r)));
}

// f32 String_ToFloat(const char* s): (f32)strtod(s, 0) (name?).
DECOMP(005566FC, String_ToFloat) {
    call(ctx, libc_strtod, {ctx.gpr[3], 0, 0});
    ctx.fpr[1] = static_cast<f64>(static_cast<f32>(ctx.fpr[1]));
}

// s32 String_ToInt(const char* s, s32 base): f_0071F284(s, &end, base),
// the end pointer on the stack (sp+112 of the original's 128-byte frame) (name?).
DECOMP(0055672C, String_ToInt) {
    Stack frame(ctx, 16);
    const u32 end = frame.addr();
    const u64 base = static_cast<u64>(static_cast<s64>(static_cast<s32>(ctx.gpr[4])));
    call(ctx, libc_strtol_r, {ctx.gpr[3], end, base, end});
}

// u32 Spline_PointTangentAt(SplineData* d, -, f32 pos[4], f32 tan[4], bool back; f32 t):
// Spline_PointTangent from the last segment found (+12) (name?).
DECOMP(00560A10, Spline_PointTangentAt) {
    const u32 d = arg<u32>(ctx, 0);
    const u64 back = ctx.gpr[7] & 0xFF;
    ctx.gpr[8] = back;
    ctx.gpr[7] = load<u32>(d + obj9::kSaLastSeg);
    callf(ctx, o13fn::Spline_PointTangent, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], back}, {farg(ctx, 0)});
}

namespace {
constexpr u32 FsOp_GetError = 0x00570C38;
constexpr u32 cellGcmUnmapEaIoAddress_stub = 0x00ABD8C4;  // cellGcmSys import stub
constexpr u32 kMemPoolGcm = 6;                  // Mem_FreePool's pool for RSX-mapped blocks (name?)
constexpr u32 kStreamFlag = 0x01164E4C;         // u8: nonzero sets bit 0x02 in new streams' flags (name?)
}  // namespace

// void PtrVector_Construct(PtrVector* v): +0 data, +4 / +6 / +8 / +10 u16,
// +16 = 0; +12 keeps only its low three bits and then clears them too
// (r4 = r5 = 0).
DECOMP(005516E0, PtrVector_Construct) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 w = load<u32>(v + 12) & 7u & ~7u;
    store<u32>(v, 0);
    store<u16>(v + 4, 0);
    store<u16>(v + 6, 0);
    store<u16>(v + 8, 0);
    store<u16>(v + 10, 0);
    store<u32>(v + 16, 0);
    store<u32>(v + 12, w);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = w;
}

// void Mem_FreeIfSet(void* p): Mem_FreePool(p, 0, 0) unless p is 0 (name?).
DECOMP(00555CC0, Mem_FreeIfSet) {
    const u32 p = arg<u32>(ctx, 0);
    if (p != 0) call(ctx, obj4::o4fn::Mem_FreePool, {p, 0, 0});
}

// void Holder_FreeData(Holder* h): Mem_FreePool(h->+4, 0, 0) unless it is 0 (r3 = it) (name?).
DECOMP(0055F27C, Holder_FreeData) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 4);
    ctx.gpr[3] = p;
    if (p != 0) call(ctx, obj4::o4fn::Mem_FreePool, {p, 0, 0});
}

// bool Stream_Init569338(Stream* s, u32 a, u8 flags): +4 = a, +8 = 0, +12 =
// flags without bit 0x02, or'd with 0x02 while 0x01164E4C is set (the byte
// first stored as given); returns 1 (name?).
DECOMP(00569338, Stream_Init569338) {
    const u32 s = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4], b = ctx.gpr[5];
    store<u8>(s + 12, static_cast<u8>(b));
    store<u32>(s + 4, static_cast<u32>(a));
    const u64 f = b & ~u64{2};
    const u32 flag = load<u8>(kStreamFlag);
    store<u32>(s + 8, 0);
    const u64 r = f | (flag != 0 ? 2u : 0u);
    store<u8>(s + 12, static_cast<u8>(r));
    ctx.gpr[3] = 1;
    ctx.gpr[4] = r;
    ctx.gpr[5] = f;
    ctx.gpr[6] = flag;
    ctx.gpr[7] = b;
    ctx.gpr[8] = 0x01160000;
}

// void GcmMem_Free(void* p): cellGcmUnmapEaIoAddress(p), then Mem_FreePool(p, 6, 0) (name?).
DECOMP(0056C8A4, GcmMem_Free) {
    const u32 p = arg<u32>(ctx, 0);
    call(ctx, cellGcmUnmapEaIoAddress_stub, {p});
    call(ctx, obj4::o4fn::Mem_FreePool, {p, kMemPoolGcm, 0});
}

// void* FsOp_ResultData(FsOp* op): op +8 when FsOp_GetError(op) is 0, else 0 (name?).
DECOMP(005616C8, FsOp_ResultData) {
    const u32 op = arg<u32>(ctx, 0);
    const u32 err = static_cast<u32>(call(ctx, FsOp_GetError, {op}));
    ret(ctx, err != 0 ? 0u : load<u32>(op + 8));
}

// void* MemStack_AllocTop(MemStack* s, u32 size): carves size bytes down
// from +16 (16-aligned) while +20 + the bytes taken stays within +0;
// returns the block or 0 (r4 = 0, r5 / r6 the new +20 / +16, r7 / r8 / r9
// the old +16 / +20 / +0) (name?).
DECOMP(0056510C, MemStack_AllocTop) {
    const u32 s = arg<u32>(ctx, 0);
    const u64 size = ctx.gpr[4];
    const u32 top = load<u32>(s + 16), used = load<u32>(s + 20), limit = load<u32>(s);
    const u32 nt = static_cast<u32>(top - size) & ~15u;
    const u64 nu = static_cast<u64>(used) + (static_cast<u64>(top) - nt);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = nu;
    ctx.gpr[6] = nt;
    ctx.gpr[7] = top;
    ctx.gpr[8] = used;
    ctx.gpr[9] = limit;
    if (static_cast<u32>(nu) > limit) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(s + 16, nt);
    store<u32>(s + 20, static_cast<u32>(nu));
    ctx.gpr[3] = nt;
}

// void Matrix_MulRot(f32 out[4][4], const f32 a[4][4], const f32 b[4][4])
// out rows 0-2 = b's rows 0-2 combined by a's rows' x / y / z (all four
// lanes of b, so the w lanes too), each row stored before the next is read
// (b re-read every row); then out's translation x / y / z = 0 (+56, +52,
// +48 in that order; +60 kept) (name?).
DECOMP(00553FF0, Matrix_MulRot) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    for (u32 i = 0; i < 3; ++i) {
        const u32 r = a + 16 * i;
        obj8::Vec4 v = obj8::VMadd(obj8::VLoad(b), obj8::VSplat(obj8::VLoadLeft(r), 0), obj8::VZero());
        v = obj8::VMadd(obj8::VLoad(b + 16), obj8::VSplat(obj8::VLoadLeft(r + 4), 0), v);
        v = obj8::VMadd(obj8::VLoad(b + 32), obj8::VSplat(obj8::VLoadLeft(r + 8), 0), v);
        obj8::VStore(out + 16 * i, v);
    }
    store<u32>(out + 56, 0);
    store<u32>(out + 52, 0);
    store<u32>(out + 48, 0);
}
