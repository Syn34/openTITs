// World objects, part 14: small helpers.
#include "obj14.h"

using namespace sly4;
using namespace sly4::obj8;
using namespace sly4::obj9;
using namespace sly4::obj10;
using namespace sly4::obj14;

namespace {

// Bezier_Tangent's constants: f32 {0.0001, 0.9999, 3, 1, 6}.
constexpr u32 kBtConsts = 0x005602E4;
constexpr u32 kBtMin = 0, kBtMax = 4, kBtThree = 8, kBtOne = 12, kBtSix = 16;

}  // namespace

// void Bezier_Tangent(f32 out[4], const f32 ctrl[4][4], f32 u)
// The unit tangent of a cubic Bezier segment at u (clamped to [0.0001,
// 0.9999]): the derivative's weights -3(1-u)^2, 3(1-u)^2 - 6u(1-u),
// 6u(1-u) - 3u^2 and 3u^2 for the four control points, then normalised.
// Only out[0..2] are written; f1 = out[2], r4/r5 = the constants'
// address as the original leaves them.
DECOMP(005602F8, Bezier_Tangent) {
    const u32 out = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    f64 u = ctx.fpr[1];
    const f64 lo = Lf(kBtConsts + kBtMin), hi = Lf(kBtConsts + kBtMax);
    if (u < lo) u = lo;
    if (u > hi) u = hi;
    const f64 three = Lf(kBtConsts + kBtThree), one = Lf(kBtConsts + kBtOne), six = Lf(kBtConsts + kBtSix);
    const f64 u3 = Fmuls(u, three);
    const f64 v = Fsubs(one, u);              // 1 - u
    const f64 u6 = Fmuls(u, six);
    const f64 w3 = Fmuls(u3, u);              // 3u^2
    const f64 v3 = Fmuls(v, three);
    const f64 w2 = Fmsubs(u6, v, w3);         // 6u(1-u) - 3u^2
    const f64 vv3 = Fmuls(v3, v);             // 3(1-u)^2
    const f64 w1 = Fnmsubs(u6, v, vv3);       // 3(1-u)^2 - 6u(1-u)
    const f64 w0 = -vv3;
    auto axis = [&](u32 k) -> f64 {
        f64 t = Fmuls(Lf(c + 32 + k), w2);
        t = Fmadds(Lf(c + 48 + k), w3, t);
        t = Fmadds(Lf(c + 16 + k), w1, t);
        return Fmadds(Lf(c + k), w0, t);
    };
    const f64 y = axis(4), z = axis(8), x = axis(0);
    const f64 inv = Fdivs(one, Fsqrts(Fmadds(z, z, Fmadds(x, x, Fmuls(y, y)))));
    const f32 ox = Fmuls(x, inv), oy = Fmuls(y, inv), oz = Fmuls(z, inv);
    store<f32>(out, ox);
    store<f32>(out + 4, oy);
    store<f32>(out + 8, oz);
    ctx.fpr[1] = oz;
    ctx.gpr[4] = kBtConsts;
    ctx.gpr[5] = kBtConsts;
}

namespace {

constexpr u32 Obj_ResetVtbl562F08 = 0x00562F08;  // (o): +0 = the input device vtable
constexpr u32 ApeRoot_Destruct = 0x00558CE8;
constexpr u32 Box_Transform = 0x005570F0;
constexpr u32 Fn_571B90 = 0x00571B90;           // (name?)
constexpr u32 libc_vsnprintf = 0x0071F36C;
constexpr u32 kFileBaseVtable = 0x00B242E8;     // file/zip.h
constexpr u32 kInputDeviceVtbl = 0x00B24310;    // game/load.h
constexpr u32 kPadDeviceVtbl = 0x00B244C0;      // game/load.h
constexpr u32 kVtbl577200 = 0x00B245F8;         // an ApeRoot subclass (name?)
constexpr u32 kMemStateHeap = 0x01164898 + 4;   // memory/mem.h MemState::heap
constexpr u32 kVfnDelete = 4;                   // the deleting destructor
constexpr u32 kVfnHeap8 = 8;                    // LinearHeap vfunc +8 (name?)
constexpr u32 kVfnFileRead = 12;                // file/zip.h kFileRead: (file, buf, size, 0) -> bytes
constexpr u32 kXyzMask = 0x005573C0;            // u32[4] {0, 0, 0, ~0}: masks the w lane

// A file reader: +4 the File, +8 u32 the bytes read so far (name?).
constexpr u32 kFrFile = 4, kFrCount = 8;
// The memory file: +264 s64 the position, +280 u32, +288 s64 the size (name?).
constexpr u32 kMfPos = 264, kMf280 = 280, kMfSize = 288;
// A chunk table: +4 s32 the current chunk, +8 {u32, u32 offset, u32 size}[] (name?).
constexpr u32 kCtCurrent = 4, kCtEntries = 8, kCtEntrySize = 12;
// The input device's two per-axis f32 arrays (obj13 Obj_ClearPair) (name?).
constexpr u32 kIdAxisHi = 132, kIdAxisLo = 140;
constexpr u32 kIdZero = 0x00562F84;             // f32 0
// A pair buffer: +0 the capacity, +12 the entry cursor (8-byte entries
// upwards), +16 the data top (downwards), +20 the bytes used, +24 u32 the
// count (name?).
constexpr u32 kPbEnd = 0, kPbCursor = 12, kPbTop = 16, kPbCheck = 20, kPbCount = 24, kPbEntry = 8;
// The bank header Bank_AllocHeader makes: 248 bytes, +0 u32 20, +4 s32 -1.
constexpr u32 kBhSize = 248;
constexpr u32 kWorldBounds = 208;               // obj: its bounds record (+16 the local box) (name?)

inline u64 Vcall(PPUContext& ctx, u32 o, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return call_ptr(ctx, opd, args);
}

}  // namespace

// void PadDevice_Destruct(PadDevice* d): vtable reset, then the input
// device's (r4 = the pad vtable on entry to it).
DECOMP(0056FB88, PadDevice_Destruct) {
    const u32 d = arg<u32>(ctx, 0);
    store<u32>(d, kPadDeviceVtbl);
    call(ctx, Obj_ResetVtbl562F08, {d, kPadDeviceVtbl});
}

// void PadDevice_Delete(PadDevice* d): the deleting destructor.
DECOMP(0056FBB8, PadDevice_Delete) {
    const u32 d = arg<u32>(ctx, 0);
    if (d == 0) return;
    store<u32>(d, kPadDeviceVtbl);
    call(ctx, Obj_ResetVtbl562F08, {d, kPadDeviceVtbl});
    call(ctx, o9fn::Mem_FreeFrom, {d, 0});
}

// void Obj577200_Destruct(Obj* o): vtable 0x00B245F8, then ApeRoot_Destruct (name?).
DECOMP(00577200, Obj577200_Destruct) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, kVtbl577200);
    call(ctx, ApeRoot_Destruct, {o, kVtbl577200});
}

// void Obj577200_Delete(Obj* o): the deleting destructor (name?).
DECOMP(00577230, Obj577200_Delete) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, kVtbl577200);
    call(ctx, ApeRoot_Destruct, {o, kVtbl577200});
    call(ctx, o9fn::Mem_FreeFrom, {o, 0});
}

// void FileBase_Delete(File* f) / InputDevice_Delete(InputDevice* d): the
// deleting destructors of the two bases: vtable reset, Mem_FreePool(p, 0,
// 0) (r6 = p, r7 = the vtable as the original leaves them).
DECOMP(00560B38, FileBase_Delete) {
    const u32 f = arg<u32>(ctx, 0);
    ctx.gpr[4] = f;
    if (f == 0) return;
    store<u32>(f, kFileBaseVtable);
    call(ctx, obj4::o4fn::Mem_FreePool, {f, 0, 0, f, kFileBaseVtable});
}
DECOMP(00562EC0, InputDevice_Delete) {
    const u32 d = arg<u32>(ctx, 0);
    ctx.gpr[4] = d;
    if (d == 0) return;
    store<u32>(d, kInputDeviceVtbl);
    call(ctx, obj4::o4fn::Mem_FreePool, {d, 0, 0, d, kInputDeviceVtbl});
}

// void Mem_DefaultHeapVfn8(-, x): the default heap's vfunc +8 (heap, x)
// (r4 passed through) (name?).
DECOMP(005530F0, Mem_DefaultHeapVfn8) {
    const u32 h = load<u32>(kMemStateHeap);
    const u32 opd = load<u32>(load<u32>(h) + kVfnHeap8);
    call_ptr(ctx, opd, {h, ctx.gpr[4], opd, load<u32>(opd)});
}

// void String_ToLower(char* s): A-Z to a-z in place (every byte is
// rewritten). r4 = 0, r5 = r6 = the length as the original leaves them.
DECOMP(005565A0, String_ToLower) {
    const u32 s = arg<u32>(ctx, 0);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0;
    if (load<u8>(s) == 0) return;
    s32 i = 0;
    u8 c;
    do {
        s32 ch = static_cast<s8>(load<u8>(s + i));
        if (!(ch < 'A' || ch > 'Z')) ch += 'a' - 'A';
        store<u8>(s + i, static_cast<u8>(ch));
        ++i;
        c = load<u8>(s + i);
    } while (c != 0);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(i));
    ctx.gpr[6] = ctx.gpr[5];
}

// bool Box_ContainsPoint(const Box* b, const f32 p[4])
// p >= b.min and b.max >= p on x, y and z (the w lanes masked off; NaN
// fails); all three vectors are read aligned. r4 = b + 16, r5 = the mask.
DECOMP(005573D0, Box_ContainsPoint) {
    const u32 b = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const Vec4 m = VLoad(kXyzMask), vp = VLoad(p), lo = VLoad(b), hi = VLoad(b + 16);
    auto f = [&](const Vec4& v, u32 i) { return std::bit_cast<f32>(v.w[i] & ~m.w[i]); };
    bool in = true;
    for (u32 i = 0; i < 4; ++i)
        if (!(f(vp, i) >= f(lo, i))) in = false;
    if (in)
        for (u32 i = 0; i < 4; ++i)
            if (!(f(hi, i) >= f(vp, i))) in = false;
    ctx.gpr[4] = b + 16;
    ctx.gpr[5] = kXyzMask;
    ret(ctx, in ? 1u : 0u);
}

// void PairBuffer_Push(PairBuffer* b, u32 x, u32 y)
// Appends {x, y} at the cursor unless the check cursor + 8 passes the end
// (unsigned); cursor and check cursor move on by 8, the count by 1 (name?).
DECOMP(00564FF8, PairBuffer_Push) {
    const u32 b = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), y = arg<u32>(ctx, 2);
    const u32 check = load<u32>(b + kPbCheck), end = load<u32>(b + kPbEnd);
    const u32 cur = load<u32>(b + kPbCursor);
    ctx.gpr[7] = end;
    ctx.gpr[8] = check + kPbEntry;
    if (check + kPbEntry > end) {
        ctx.gpr[6] = cur;
        return;
    }
    store<u32>(cur, x);
    store<u32>(cur + 4, y);
    const u32 c = load<u32>(b + kPbCursor) + kPbEntry, k = load<u32>(b + kPbCheck) + kPbEntry;
    const u32 n = load<u32>(b + kPbCount) + 1;
    store<u32>(b + kPbCursor, c);
    store<u32>(b + kPbCheck, k);
    store<u32>(b + kPbCount, n);
    ctx.gpr[4] = c;
    ctx.gpr[5] = k;
    ctx.gpr[6] = n;
}

// void Mem_FreeAndClear(void** p): Mem_FreePool(*p, 0, 0) and *p = 0 if
// set; r3 = 0 (name?).
DECOMP(005692E8, Mem_FreeAndClear) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 v = load<u32>(p);
    if (v != 0) {
        call(ctx, obj4::o4fn::Mem_FreePool, {v, 0, 0});
        store<u32>(p, 0);
    }
    ctx.gpr[3] = 0;
}

// f32* WorldObj_WorldBounds(const WorldObj* o, Box* out): the bounds
// record's (+208) local box through the object's matrix (+80) (name?).
DECOMP(005772E4, WorldObj_WorldBounds) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    call(ctx, Box_Transform, {out, load<u32>(o + kWorldBounds) + 16, o + obj12::kObjOwnMatrix});
    ret(ctx, out);
}

// s64 MemFile_Seek(MemFile* f, s64 off, s32 whence)
// 0: off, 1: position + off, 2: size - off (sic), anything else 0; the
// result becomes the position and is returned (r5 = it, r6 = whence) (name?).
DECOMP(0056731C, MemFile_Seek) {
    const u32 f = arg<u32>(ctx, 0);
    const u64 off = ctx.gpr[4];
    const s32 whence = static_cast<s32>(static_cast<u32>(ctx.gpr[5]));
    u64 pos = 0;
    if (whence == 0) pos = off;
    else if (whence == 1) pos = load<u64>(f + kMfPos) + off;
    else if (whence == 2) pos = load<u64>(f + kMfSize) - off;
    store<u64>(f + kMfPos, pos);
    ctx.gpr[6] = ctx.gpr[5];
    ctx.gpr[5] = pos;
    ctx.gpr[3] = pos;
}

// bool MemFile_Close(MemFile* f): +280, the position and the size 0, then
// the deleting destructor; returns 1 (name?).
DECOMP(00567124, MemFile_Close) {
    const u32 f = arg<u32>(ctx, 0);
    store<u32>(f + kMf280, 0);
    store<u64>(f + kMfPos, 0);
    store<u64>(f + kMfSize, 0);
    ctx.gpr[5] = 0;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = f;
    if (f != 0) {
        const u32 opd = load<u32>(load<u32>(f) + kVfnDelete);
        call_ptr(ctx, opd, {f, opd, load<u32>(opd), 0, f});
    }
    ctx.gpr[3] = 1;
}

// s32 String_FormatN(char* buf, u32 size, const char* fmt, ...)
// snprintf: the variadic registers are spilled into the caller's
// parameter save area (fmt's low word at +68, r6..r10 at +72..+104) and
// +72 passed as the va_list (name?).
DECOMP(00555CF4, String_FormatN) {
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    store<u32>(sp + 68, static_cast<u32>(ctx.gpr[5]));
    for (u32 k = 0; k < 5; ++k) store<u64>(sp + 72 + 8 * k, ctx.gpr[6 + k]);
    call(ctx, libc_vsnprintf, {ctx.gpr[3], ctx.gpr[4] & 0xFFFFFFFFu, ctx.gpr[5], sp + 72});
}

// void Archive_Mount(a, b, c, d): f_00571B90 with the same four 64-bit
// arguments (the original round-trips b..d through its frame) (name?).
DECOMP(00562944, Archive_Mount) {
    call(ctx, Fn_571B90, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6]});
}

// bool InputDevice_AxisPair(const InputDevice* d, s32 i)
// +140[i] < 0 and +132[i] > 0 (name?).
DECOMP(00562F88, InputDevice_AxisPair) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 off = static_cast<u32>(arg<s32>(ctx, 1) << 2);
    const f32 zero = Lf(kIdZero);
    const f32 lo = Lf(d + kIdAxisLo + off);
    u32 r = 0;
    f32 last = lo;
    if (lo < zero) {
        last = Lf(d + kIdAxisHi + off);
        if (last > zero) r = 1;
    }
    ctx.fpr[1] = zero;
    ctx.fpr[2] = last;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(static_cast<s32>(off)));
    ctx.gpr[5] = r;
    ctx.gpr[6] = d + kIdAxisLo;
    ctx.gpr[3] = r;
}

// void RefObj_Release(RefObj* o): u16 +4 down by one; at zero or below
// the deleting destructor (name?).
DECOMP(00577288, RefObj_Release) {
    const u32 o = arg<u32>(ctx, 0);
    const u16 n = static_cast<u16>(load<u16>(o + obj4::kObjRefs) - 1);
    store<u16>(o + obj4::kObjRefs, n);
    ctx.gpr[5] = n;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(static_cast<s16>(n)));
    if (static_cast<s16>(n) > 0 || o == 0) return;
    const u32 opd = load<u32>(load<u32>(o) + kVfnDelete);
    call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
}

// u32 FileReader_Read(FileReader* r, void* buf, u32 size): the file's
// read (vfunc +12 (file, buf, size, 0)); +8 += the bytes read (name?).
DECOMP(00563768, FileReader_Read) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 f = load<u32>(r + kFrFile);
    const u32 opd = load<u32>(load<u32>(f) + kVfnFileRead);
    const u32 n = static_cast<u32>(call_ptr(ctx, opd, {f, ctx.gpr[4], ctx.gpr[5], 0, opd, load<u32>(opd)}));
    const u32 total = load<u32>(r + kFrCount) + n;
    store<u32>(r + kFrCount, total);
    ctx.gpr[3] = n;
    ctx.gpr[4] = total;
}

// u32 FileReader_ReadByte(FileReader* r, void* buf): the same for one byte (name?).
DECOMP(0056998C, FileReader_ReadByte) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 f = load<u32>(r + kFrFile);
    const u32 opd = load<u32>(load<u32>(f) + kVfnFileRead);
    const u32 n = static_cast<u32>(call_ptr(ctx, opd, {f, ctx.gpr[4], 1, 0, opd, load<u32>(opd)}));
    const u32 total = load<u32>(r + kFrCount) + n;
    store<u32>(r + kFrCount, total);
    ctx.gpr[3] = n;
    ctx.gpr[4] = total;
}

// s32 ChunkReader_Remaining(const ChunkReader* r): the current chunk's
// end (offset + size) minus the file position; 0 without a current chunk
// (r->+0's s32 +4 < 0) (name?).
DECOMP(0056424C, ChunkReader_Remaining) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 t = load<u32>(r);
    const s32 cur = load<s32>(t + kCtCurrent);
    if (cur < 0) {
        ctx.gpr[4] = static_cast<u64>(static_cast<s64>(cur));
        ctx.gpr[5] = t;
        ctx.gpr[3] = 0;
        return;
    }
    const u32 e = t + static_cast<u32>(cur) * kCtEntrySize + kCtEntries;
    if (e == 0) {
        ctx.gpr[4] = 0;
        ctx.gpr[5] = t;
        ctx.gpr[6] = static_cast<u32>(cur) * 2;
        ctx.gpr[3] = 0;
        return;
    }
    const u32 f = load<u32>(r + 4);
    const u32 a = load<u32>(e + 4);
    const u32 end = a + load<u32>(e + 8);
    const u32 pos = static_cast<u32>(load<u64>(f + kMfPos));
    ctx.gpr[4] = end;
    ctx.gpr[5] = a;
    ctx.gpr[6] = static_cast<u32>(cur) * 2;
    ret(ctx, static_cast<s32>(end - pos));
}

// void Box_Translate(Box* b, const f32 v[3]) / Box_Scale(Box* b, const
// f32 v[3]): min and max (x, y, z) plus / times v, loaded and stored in
// the original's order (b may alias v). f1 = the last result.
namespace {
template <bool Mul>
void BoxApply(PPUContext& ctx) {
    const u32 b = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    auto op = [](f32 x, f32 y) { return Mul ? Fmuls(x, y) : Fadds(x, y); };
    const f32 r0 = op(Lf(b), Lf(v));
    const f32 b4 = Lf(b + 4), b8 = Lf(b + 8), b16 = Lf(b + 16), b20 = Lf(b + 20);
    store<f32>(b, r0);
    const f32 r4 = op(b4, Lf(v + 4));
    const f32 b24 = Lf(b + 24);
    store<f32>(b + 4, r4);
    store<f32>(b + 8, op(b8, Lf(v + 8)));
    store<f32>(b + 16, op(b16, Lf(v)));
    store<f32>(b + 20, op(b20, Lf(v + 4)));
    const f32 last = op(b24, Lf(v + 8));
    store<f32>(b + 24, last);
    ctx.fpr[1] = last;
}
}  // namespace
DECOMP(00557284, Box_Translate) { BoxApply<false>(ctx); }
DECOMP(00557358, Box_Scale) { BoxApply<true>(ctx); }

// void Box_Offset(Box* out, const Box* b, const f32 v[3]): out = b + v
// (min and max x, y, z), min stored before max is read.
DECOMP(005572E8, Box_Offset) {
    const u32 out = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    for (u32 h = 0; h < 32; h += 16) {
        const f32 x = Fadds(Lf(b + h), Lf(v)), y = Fadds(Lf(b + h + 4), Lf(v + 4)), z = Fadds(Lf(b + h + 8), Lf(v + 8));
        store<f32>(out + h, x);
        store<f32>(out + h + 4, y);
        store<f32>(out + h + 8, z);
        ctx.fpr[1] = x;
        ctx.fpr[2] = z;
        ctx.fpr[3] = y;
    }
}

// void Bank_AllocHeader(Bank* b): +4 = 0, then a 248-byte header (+0 =
// 20, +4 = -1) from operator new into +0 (0 if that fails) (name?).
DECOMP(00569280, Bank_AllocHeader) {
    const u32 b = arg<u32>(ctx, 0);
    store<u32>(b + 4, 0);
    const u32 p = static_cast<u32>(call(ctx, fn::Mem_New, {kBhSize}));
    if (p != 0) {
        store<u32>(p, 20);
        store<s32>(p + 4, -1);
        ctx.gpr[4] = 20;
        ctx.gpr[5] = ~0ull;
    }
    store<u32>(b, p);
}

namespace {

constexpr u32 Obj_ClearField8 = 0x00565A88, Obj_ClearPair = 0x00563098, Obj_SetEmpty148 = 0x005630C0;
constexpr u32 InputDevice_Fn562998 = 0x00562998;  // (name?)
constexpr u32 PadDevice_Fn562BAC = 0x00562BAC;    // (name?)
constexpr u32 Fn_561374 = 0x00561374;             // (name?)
constexpr u32 imp_sys_lwmutex_destroy = 0x00ABF3E4;
constexpr u32 imp_cellSysutilUnregisterCallback = 0x00ABD984;
constexpr u32 imp_cellPadSetActDirect = 0x00ABDF04;

constexpr u32 kSysutilMutex = 0x0116B288;        // sys_lwmutex_t (name?)
constexpr u32 kSysutilLockTimeout = 100000;      // usec
constexpr u32 kStatic5721C4A = 0x0116B600, kStatic5721C4B = 0x0116B900;  // objects given vtable 0x00B24458 (name?)
constexpr u32 kStatic5721C4C = 0x0116B340, kStatic5721C4D = 0x0116B328;  // Obj_ClearField8 targets (name?)
constexpr u32 kVtbl5721C4 = 0x00B24458;          // (name?)
constexpr u32 kStaticVtblHolder = 0x0116B580;    // r6 as the original leaves it

// CellPadInfo2 (game/load.h kPadInfo) device_capability[7] at +68.
constexpr u32 kPadCapability = 0x0116B2A8 + 68;
constexpr u32 kPadCapActuator = 0x10;            // CELL_PAD_CAPABILITY_ACTUATOR
// The pad device: +12 u8 the port, +284 CellPadActParam {u8 motor[2], ...}, +292 u32 (name?).
constexpr u32 kPdPort = 12, kPdAct = 284, kPd292 = 292;
// The input device's buttons: +16 now, +20 changed, +24 released, +28 before (name?).
constexpr u32 kIdButtons = 16, kIdChanged = 20, kIdReleased = 24, kIdPrevious = 28;
constexpr u32 kVfnInput4 = 4;                    // input device vfunc +4 (d) (name?)

constexpr s32 kErr10702 = static_cast<s32>(0x80010702u);  // (name?)

// The pad devices' rumble stop: both motors 0, sent with cellPadSetActDirect
// when the port's device has an actuator. r3 = the port otherwise.
void PadStopMotors(PPUContext& ctx, u32 d) {
    const u32 port = load<u8>(d + kPdPort);
    store<u8>(d + kPdAct, 0);
    store<u8>(d + kPdAct + 1, 0);
    const u32 cap = load<u32>(kPadCapability + 4 * port);
    ctx.gpr[3] = port;
    ctx.gpr[4] = cap & kPadCapActuator;
    ctx.gpr[5] = kPadCapability;
    ctx.gpr[6] = port * 4;
    if (cap & kPadCapActuator) call(ctx, imp_cellPadSetActDirect, {port, d + kPdAct, kPadCapability, port * 4});
}

}  // namespace

// u32 FileReader_Read32(FileReader* r, void* buf) / FileReader_ReadU32:
// FileReader_Read of 32 / 4 bytes (name?).
DECOMP(00569C10, FileReader_Read32) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 f = load<u32>(r + kFrFile);
    const u32 opd = load<u32>(load<u32>(f) + kVfnFileRead);
    const u32 n = static_cast<u32>(call_ptr(ctx, opd, {f, ctx.gpr[4], 32, 0, opd, load<u32>(opd)}));
    const u32 total = load<u32>(r + kFrCount) + n;
    store<u32>(r + kFrCount, total);
    ctx.gpr[3] = n;
    ctx.gpr[4] = total;
}
DECOMP(00569C74, FileReader_ReadU32) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 f = load<u32>(r + kFrFile);
    const u32 opd = load<u32>(load<u32>(f) + kVfnFileRead);
    const u32 n = static_cast<u32>(call_ptr(ctx, opd, {f, ctx.gpr[4], 4, 0, opd, load<u32>(opd)}));
    const u32 total = load<u32>(r + kFrCount) + n;
    store<u32>(r + kFrCount, total);
    ctx.gpr[3] = n;
    ctx.gpr[4] = total;
}

// void Sysutil_StaticDestroy(): an atexit destructor: two statics back to
// vtable 0x00B24458, two Obj_ClearField8 and the sysutil mutex destroyed (name?).
DECOMP(005721C4, Sysutil_StaticDestroy) {
    store<u32>(kStatic5721C4B, kVtbl5721C4);
    store<u32>(kStatic5721C4A, kVtbl5721C4);
    call(ctx, Obj_ClearField8, {kStatic5721C4C, kVtbl5721C4, 0x01170000, kStaticVtblHolder});
    call(ctx, Obj_ClearField8, {kStatic5721C4D});
    call(ctx, imp_sys_lwmutex_destroy, {kSysutilMutex});
}

// void InputDevice_UpdateButtons(InputDevice* d)
// f_00562998, then the button edges: changed = now ^ before, released =
// before & changed, before = now; then vfunc +4 (d) (name?).
DECOMP(00562F18, InputDevice_UpdateButtons) {
    const u32 d = arg<u32>(ctx, 0);
    call(ctx, InputDevice_Fn562998, {d});
    const u32 now = load<u32>(d + kIdButtons), before = load<u32>(d + kIdPrevious);
    const u32 vt = load<u32>(d);
    const u32 changed = now ^ before;
    store<u32>(d + kIdPrevious, now);
    store<u32>(d + kIdChanged, changed);
    store<u32>(d + kIdReleased, before & changed);
    const u32 opd = load<u32>(vt + kVfnInput4);
    call_ptr(ctx, opd, {d, opd, load<u32>(opd), vt});
}

// s32 Sysutil_UnregisterCallback(s32 slot): cellSysutilUnregisterCallback
// under the sysutil mutex (100 ms timeout) (name?).
DECOMP(0056C274, Sysutil_UnregisterCallback) {
    const u32 slot = arg<u32>(ctx, 0);
    call(ctx, imp::sys_lwmutex_lock, {kSysutilMutex, kSysutilLockTimeout});
    const u64 r = call(ctx, imp_cellSysutilUnregisterCallback, {slot});
    call(ctx, imp::sys_lwmutex_unlock, {kSysutilMutex});
    ctx.gpr[3] = r;
}

// s32 Callback_56D068(-, Obj* o, u8 kind)
// kind 1: o->+36's +0 = o, then f_00561374(o->+36, o->+36, o); kind 2:
// nothing; anything else 0x80010702. Returns 0 otherwise (name?).
DECOMP(0056D068, Callback_56D068) {
    const u32 o = arg<u32>(ctx, 1);
    const u32 kind = static_cast<u32>(ctx.gpr[5]) & 0xFF;
    const u32 t = load<u32>(o + 36);
    ctx.gpr[4] = t;
    if (kind == 1) {
        store<u32>(t, o);
        call(ctx, Fn_561374, {t, t, o});
        ctx.gpr[3] = 0;
    } else if (kind == 2) {
        ctx.gpr[3] = 0;
    } else {
        ret(ctx, kErr10702);
    }
}

// void PadDevice_Reset(PadDevice* d): f_00562BAC, then the motors stopped (name?).
DECOMP(0056F95C, PadDevice_Reset) {
    const u32 d = arg<u32>(ctx, 0);
    call(ctx, PadDevice_Fn562BAC, {d});
    PadStopMotors(ctx, d);
}

// void PadDevice_ClearAxis(PadDevice* d, u32 i): Obj_ClearPair(d, i),
// then the motors stopped and +292 = 0 (name?).
DECOMP(00570520, PadDevice_ClearAxis) {
    const u32 d = arg<u32>(ctx, 0);
    call(ctx, Obj_ClearPair, {d, ctx.gpr[4]});
    const u32 port = load<u8>(d + kPdPort);
    store<u8>(d + kPdAct, 0);
    store<u8>(d + kPdAct + 1, 0);
    store<u32>(d + kPd292, 0);
    const u32 cap = load<u32>(kPadCapability + 4 * port);
    ctx.gpr[3] = port;
    ctx.gpr[4] = cap & kPadCapActuator;
    ctx.gpr[5] = kPadCapability;
    ctx.gpr[6] = port * 4;
    if (cap & kPadCapActuator) call(ctx, imp_cellPadSetActDirect, {port, d + kPdAct, kPadCapability, port * 4});
}

// void PadDevice_SetEnabled(PadDevice* d, u8 on): Obj_SetEmpty148(d, on),
// then the motors stopped (name?).
DECOMP(00570590, PadDevice_SetEnabled) {
    const u32 d = arg<u32>(ctx, 0);
    call(ctx, Obj_SetEmpty148, {d, ctx.gpr[4] & 0xFF});
    PadStopMotors(ctx, d);
}

// void Matrix_Scale3(f32 m[4][4], f32 s): rows 0-2 (x, y, z) times s,
// all read before any store. f1..f9 = the products as the original leaves them.
DECOMP(00554414, Matrix_Scale3) {
    const u32 m = arg<u32>(ctx, 0);
    const f64 s = ctx.fpr[1];
    static constexpr u32 kOff[9] = {0, 4, 8, 16, 20, 24, 32, 36, 40};
    f32 v[9];
    for (u32 k = 0; k < 9; ++k) v[k] = Fmuls(Lf(m + kOff[k]), s);
    for (u32 k = 0; k < 9; ++k) store<f32>(m + kOff[k], v[k]);
    ctx.fpr[1] = v[8];
    for (u32 k = 0; k < 8; ++k) ctx.fpr[2 + k] = v[k];
}

// void RefBlock_Release(RefBlock* b): u16 +0 down by one; at zero or
// below +28 and the block itself freed (Mem_FreePool(p, 0, 0)) (name?).
DECOMP(00557C80, RefBlock_Release) {
    const u32 b = arg<u32>(ctx, 0);
    const u16 n = static_cast<u16>(load<u16>(b) - 1);
    store<u16>(b, n);
    ctx.gpr[4] = n;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(static_cast<s16>(n)));
    if (static_cast<s16>(n) > 0 || b == 0) return;
    call(ctx, obj4::o4fn::Mem_FreePool, {load<u32>(b + 28), 0, 0});
    call(ctx, obj4::o4fn::Mem_FreePool, {b, 0, 0});
}

// u32 Data_Hash31(const u8* p, u32 n): h = h * 31 + byte over n bytes; 0
// for a null p (name?).
DECOMP(0056A7AC, Data_Hash31) {
    const u32 p = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    u32 h = 0, i = 0;
    ctx.gpr[5] = 0;
    if (n != 0) {
        u32 b = 0;
        do {
            b = load<u8>(p + i);
            h = h * 31 + b;
            ++i;
        } while (n > i);
        ctx.gpr[5] = i;
        ctx.gpr[7] = b;
        ctx.gpr[8] = i;
    }
    ctx.gpr[6] = h;
    ctx.gpr[3] = h;
}

namespace {

// The three function-local static allocators: {the pointer once made, the
// guard byte, the object, its vtable} (game/load.h kDefaultHeap*,
// container/container.h kDefaultAllocator*).
struct StaticAlloc {
    u32 ptr, guard, obj, vtbl;
};
constexpr StaticAlloc kDefaultHeapS = {0x00F88700, 0x01165040, 0x01165048, 0x00B24278};
constexpr StaticAlloc kDefaultAllocatorS = {0x00F88704, 0x01165010, 0x01165018, 0x00B24368};
constexpr StaticAlloc kAllocator552588S = {0x00F88708, 0x01165000, 0x01165008, 0x00B24388};  // (name?)

// The object is constructed (vtable stored) on the guard's first pass and
// again before the pointer is set. r4 = 0x00F90000, r5 = 0x00B20000, r6 =
// 0x01160000 as the original leaves them on the slow path.
void GetStaticAlloc(PPUContext& ctx, const StaticAlloc& s) {
    const u32 p = load<u32>(s.ptr);
    if (p != 0) {
        ctx.gpr[3] = p;
        return;
    }
    if (load<u8>(s.guard) == 0) {
        store<u32>(s.obj, s.vtbl);
        store<u8>(s.guard, 1);
    }
    store<u32>(s.obj, s.vtbl);
    store<u32>(s.ptr, s.obj);
    ctx.gpr[3] = s.obj;
    ctx.gpr[4] = 0x00F90000;
    ctx.gpr[5] = 0x00B20000;
    ctx.gpr[6] = 0x01160000;
}

constexpr u32 kEventSlots = 0x01164DF8;         // EventSlot*[] (name?)
// An event slot: +20 the event flag id, +32 u64 the value, +40 u8 set (name?).
constexpr u32 kEsFlag = 20, kEsValue = 32, kEsSet = 40;
constexpr u32 sys_event_flag_set = 87;          // (id, u64 bits)

}  // namespace

// Allocator* Mem_DefaultHeap() / Mem_DefaultAllocator() / Mem_Allocator552588():
// the static allocators, made on first use.
DECOMP(00550E70, Mem_DefaultHeap) { GetStaticAlloc(ctx, kDefaultHeapS); }
DECOMP(00550EE8, Mem_DefaultAllocator) { GetStaticAlloc(ctx, kDefaultAllocatorS); }
DECOMP(00552588, Mem_Allocator552588) { GetStaticAlloc(ctx, kAllocator552588S); }

// void* Mem_DefaultStackAlloc(-, u32 size, u8 zero): MemStack_Alloc from
// the default heap (r5 passed through), zeroed when asked (name?).
DECOMP(00553074, Mem_DefaultStackAlloc) {
    const u32 size = arg<u32>(ctx, 1);
    const u64 zero = ctx.gpr[5];
    const u32 p = static_cast<u32>(call(ctx, obj4::o4fn::MemStack_Alloc, {load<u32>(kMemStateHeap), ctx.gpr[4], zero}));
    ctx.gpr[4] = zero & 0xFF;
    if ((zero & 0xFF) != 0 && size != 0) call(ctx, fn::memset, {p, 0, size});
    ctx.gpr[3] = p;
}

// void Matrix_FromRows(f32 out[4][4], const f32 rows[3][4], const f32 pos[3]):
// rows 0-2 (x, y, z) from rows, row 3 from pos; w lanes untouched. Each
// word is stored before the next is read. r6 / r7 = rows + 16 / + 32.
DECOMP(00553B4C, Matrix_FromRows) {
    const u32 out = arg<u32>(ctx, 0), rows = arg<u32>(ctx, 1), pos = arg<u32>(ctx, 2);
    u32 w = 0;
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 12; k += 4) {
            w = load<u32>((i < 3 ? rows + 16 * i : pos) + k);
            store<u32>(out + 16 * i + k, w);
        }
    ctx.fpr[1] = std::bit_cast<f32>(w);
    ctx.gpr[6] = rows + 16;
    ctx.gpr[7] = rows + 32;
}

// void Quat_Mul(f32 a[4], const f32 b[4]): a = the quaternion product
// {x, y, z, w} as the original's fused chains compute it; all read before
// any store. f1..f11 as the original leaves them.
DECOMP(00555718, Quat_Mul) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f32 b0 = Lf(b), a0 = Lf(a), b3 = Lf(b + 12), b2 = Lf(b + 8), b1 = Lf(b + 4);
    const f32 a3 = Lf(a + 12), a1 = Lf(a + 4), a2 = Lf(a + 8);
    f32 x = Fmuls(a0, b3), t8 = Fmuls(a0, b2), t9 = Fmuls(a0, b1), w = Fmuls(a0, b0);
    x = Fmadds(a3, b0, x);
    t8 = Fmsubs(a3, b1, t8);
    t9 = Fmadds(a3, b2, t9);
    w = Fmsubs(a3, b3, w);
    x = Fmadds(a1, b2, x);
    const f32 t7 = Fmadds(a1, b3, t8);
    t8 = Fnmsubs(a1, b0, t9);
    w = Fnmsubs(a1, b1, w);
    x = Fnmsubs(a2, b1, x);
    const f32 y = Fmadds(a2, b0, t7);
    const f32 z = Fmadds(a2, b3, t8);
    w = Fnmsubs(a2, b2, w);
    store<f32>(a, x);
    store<f32>(a + 4, y);
    store<f32>(a + 8, z);
    store<f32>(a + 12, w);
    const f32 fr[11] = {y, w, z, b2, x, b1, t7, t8, t9, a1, a2};
    for (u32 k = 0; k < 11; ++k) ctx.fpr[1 + k] = fr[k];
}

// u32 EventSlot_Set(u32 i, u64 v): slot i's +32 = v and its event flag
// set (sys_event_flag_set(+20, 1)), then +40 = 1; returns 1 (name?).
DECOMP(00565304, EventSlot_Set) {
    const u32 off = (arg<u32>(ctx, 0) << 2) & 0x3FFFC;
    const u64 v = ctx.gpr[4];
    const u32 e = load<u32>(kEventSlots + off);
    store<u32>(e + kEsValue + 4, static_cast<u32>(v));
    store<u32>(e + kEsValue, static_cast<u32>(v >> 32));
    ctx.gpr[5] = e + 16;
    ctx.gpr[6] = e + 32;
    syscall(ctx, sys_event_flag_set, {load<u32>(e + kEsFlag), 1});
    const u32 e2 = load<u32>(kEventSlots + off);
    store<u8>(e2 + kEsSet, 1);
    ctx.gpr[3] = 1;
    ctx.gpr[4] = e2;
}

namespace {

constexpr u32 imp_cellSpursShutdownJobChain = 0x00ABE0A4;
constexpr u32 imp_cellSpursJoinJobChain = 0x00ABE164;
constexpr u32 Iff_ReadChunks = 0x00569388;      // (reader, obj)
constexpr u32 InputDevice_AxisPairFn = 0x00562F88;

constexpr u32 kAxisX = 0x01164B50, kAxisY = 0x01164B60, kAxisZ = 0x01164B70;  // f32[3] unit axes
constexpr u32 kIffSwap = 0x01164E4C;            // u8: reader flags get 2 when set (name?)
constexpr u32 kPerspTwo = 0x0056A2D4;           // f32 2
constexpr u32 kMinusOne = 0xBF800000u;          // f32 -1
constexpr u32 kPadZero = 0x0057049C;            // f32 0
constexpr u32 kHalfSphere = 0x0055786C;         // f32 0.5
// The pool buffer: +0 the data, +8 u32, +20 u8 from pool 1 (name?).
constexpr u32 kPbData = 0, kPb8 = 8, kPbPool1 = 20;
// The job chain owner: +512, +516, +524 buffers and +520 the CellSpursJobChain (all pool 6) (name?).
constexpr u32 kJcBufA = 512, kJcBufB = 516, kJcChain = 520, kJcBufC = 524;
constexpr u32 kPoolGcm = 6;
// Pad device fields: +148 u8 enabled, +292 f32 (name?).
constexpr u32 kPdEnabled = 148, kPdLevel = 292;
// Input device reset: +16..+24 the button words, +32 96 bytes, +128..+144
// words, +148 u8 = 1 (name?).
constexpr u32 kIdState = 32, kIdStateSize = 96;

inline f32 Fsel(f64 t, f32 a, f32 b) { return t >= 0.0 ? a : b; }

}  // namespace

// void PoolBuffer_Free(PoolBuffer* b): the data back to pool 0 (or 1 when
// +20 is set), +8 and +0 cleared; r3 = 0 (name?).
DECOMP(005662A0, PoolBuffer_Free) {
    const u32 b = arg<u32>(ctx, 0);
    const u32 p = load<u32>(b + kPbData);
    if (p != 0) {
        const u8 pool1 = load<u8>(b + kPbPool1);
        store<u32>(b + kPb8, 0);
        call(ctx, obj4::o4fn::Mem_FreePool, {p, pool1 == 0 ? 0u : 1u, 0});
        store<u32>(b + kPbData, 0);
    }
    ctx.gpr[3] = 0;
}

// bool Iff_ReadFile(IffReader* r, File* f, Obj* o): +4 = f, +8 = 0, +12 =
// the flags (2 when 0x01164E4C is set), Iff_ReadChunks(r, o), +4 = 0;
// returns 1 (name?).
DECOMP(005695C4, Iff_ReadFile) {
    const u32 r = arg<u32>(ctx, 0), f = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    store<u8>(r + 12, 0);
    store<u32>(r + 4, f);
    const u8 swap = load<u8>(kIffSwap);
    store<u32>(r + 8, 0);
    const u32 flags = swap != 0 ? 2u : 0u;
    store<u8>(r + 12, static_cast<u8>(flags));
    call(ctx, Iff_ReadChunks, {r, o, flags, ctx.gpr[6], f, 0x01160000});
    store<u32>(r + 4, 0);
    ctx.gpr[3] = 1;
    ctx.gpr[4] = 0;
}

// void Matrix_PerspectiveWH(f32 m[4][4], f32 w, f32 h, f32 n, f32 f)
// m00 = 2n / w, m11 = 2n / h, m22 = f / (n - f), m23 = -1, m32 = f n /
// (n - f), the rest 0 (name?).
DECOMP(0056A2D8, Matrix_PerspectiveWH) {
    const u32 m = arg<u32>(ctx, 0);
    const f64 w = ctx.fpr[1], h = ctx.fpr[2], n = ctx.fpr[3], f = ctx.fpr[4];
    const f32 d = Fsubs(n, f), fn = Fmuls(f, n), two = Lf(kPerspTwo);
    const f32 n2 = Fmuls(n, two);
    const f32 m22 = Fdivs(f, d);
    for (const u32 off : {4u, 8u, 12u, 16u, 24u, 28u, 32u, 36u}) store<u32>(m + off, 0);
    store<u32>(m + 44, kMinusOne);
    store<u32>(m + 48, 0);
    const f32 m32 = Fdivs(fn, d);
    store<u32>(m + 52, 0);
    store<u32>(m + 60, 0);
    const f32 m00 = Fdivs(n2, w), m11 = Fdivs(n2, h);
    store<f32>(m + 40, m22);
    store<f32>(m + 56, m32);
    store<f32>(m, m00);
    store<f32>(m + 20, m11);
    const f32 fr[7] = {m00, m11, n2, m22, m32, fn, two};
    for (u32 k = 0; k < 7; ++k) ctx.fpr[1 + k] = fr[k];
    ctx.gpr[4] = 0;
    ctx.gpr[5] = kMinusOne;
}

// void CallbackList_Invoke(-, const CallbackList* l): calls each of the
// l->+4 {OPD*, arg} pairs at l->+0 as fn(arg) (r4 / r5 = the OPD and
// entry); r3 = 0 after at least one call (name?).
DECOMP(005651B0, CallbackList_Invoke) {
    const u32 l = arg<u32>(ctx, 1);
    u32 n = load<u32>(l + 4);
    u32 p = load<u32>(l);
    while (n-- != 0) {
        const u32 opd = load<u32>(p);
        call_ptr(ctx, opd, {load<u32>(p + 4), opd, load<u32>(opd)});
        p += 8;
        ctx.gpr[3] = n;
    }
}

// void Matrix_Translation(f32 m[4][4], const f32 pos[3]): rows 0-2 the
// unit axes, row 3 pos (x, y, z only; w lanes untouched) (name?).
DECOMP(00553C70, Matrix_Translation) {
    const u32 m = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1);
    const u32 rows[4] = {kAxisX, kAxisY, kAxisZ, pos};
    u32 w = 0;
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 12; k += 4) {
            w = load<u32>(rows[i] + k);
            store<u32>(m + 16 * i + k, w);
        }
    ctx.fpr[1] = std::bit_cast<f32>(w);
    ctx.gpr[5] = 0x01160000;
    ctx.gpr[6] = kAxisZ;
    ctx.gpr[7] = 0x01160000;
    ctx.gpr[8] = kAxisY;
}

// void JobChain_Destroy(JobChainOwner* o): shuts down and joins the SPURS
// job chain at +520, then frees +520, +524, +512 and +516 to pool 6 (name?).
DECOMP(0056DC64, JobChain_Destroy) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, imp_cellSpursShutdownJobChain, {load<u32>(o + kJcChain)});
    call(ctx, imp_cellSpursJoinJobChain, {load<u32>(o + kJcChain)});
    for (const u32 off : {kJcChain, kJcBufC, kJcBufA, kJcBufB})
        call(ctx, obj4::o4fn::Mem_FreePool, {load<u32>(o + off), kPoolGcm, 0});
}

// bool PadDevice_AxisActive(const PadDevice* d, s32 i): enabled, +292 > 0
// and InputDevice_AxisPair(d, i) (name?).
DECOMP(005704A0, PadDevice_AxisActive) {
    const u32 d = arg<u32>(ctx, 0);
    const u64 i = ctx.gpr[4];
    ctx.gpr[4] = d;
    const u8 on = load<u8>(d + kPdEnabled);
    ctx.gpr[5] = on;
    u32 r = 0;
    if (on != 0) {
        const f32 lvl = Lf(d + kPdLevel), zero = Lf(kPadZero);
        ctx.gpr[5] = kPadZero;
        ctx.fpr[1] = lvl;
        ctx.fpr[2] = zero;
        if (lvl > zero && call(ctx, InputDevice_AxisPairFn, {d, i, i}) != 0) r = 1;
    }
    ctx.gpr[3] = r;
}

// void Vec_ToLocal(f32 v[3], const f32 m[4][4]): v - m's row 3 dotted
// with rows 0-2 (each result stored before the next row is read) (name?).
DECOMP(00553368, Vec_ToLocal) {
    const u32 v = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const f32 dy = Fsubs(Lf(v + 4), Lf(m + 52)), dx = Fsubs(Lf(v), Lf(m + 48)), dz = Fsubs(Lf(v + 8), Lf(m + 56));
    f32 last = 0;
    for (u32 k = 0; k < 3; ++k) {
        const u32 r = m + 16 * k;
        last = Fmadds(dz, Lf(r + 8), Fmadds(dx, Lf(r), Fmuls(dy, Lf(r + 4))));
        store<f32>(v + 4 * k, last);
    }
    ctx.fpr[1] = last;
    ctx.fpr[2] = dx;
    ctx.fpr[3] = Lf(m + 40);
    ctx.fpr[4] = dz;
    ctx.fpr[5] = Lf(m + 32);
    ctx.fpr[6] = Lf(m + 24);
    ctx.gpr[5] = m + 48;
}

// void Box_Union(Box* b, const Box* o): b.min = min, b.max = max per lane
// (fsel on the single difference; NaN takes o's min / o's max), stored
// lane by lane as the original. r5 = o + 16.
DECOMP(00557064, Box_Union) {
    const u32 b = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    f32 last = 0, diff = 0;
    for (u32 k = 0; k < 12; k += 4) {
        const f32 x = Lf(o + k), y = Lf(b + k);
        last = Fsel(Fsubs(y, x), x, y);
        store<f32>(b + k, last);
    }
    for (u32 k = 0; k < 12; k += 4) {
        const f32 x = Lf(o + 16 + k), y = Lf(b + 16 + k);
        diff = Fsubs(y, x);
        last = Fsel(diff, y, x);
        store<f32>(b + 16 + k, last);
    }
    ctx.fpr[1] = last;
    ctx.fpr[2] = diff;
    ctx.gpr[5] = o + 16;
}

// void Box_BoundingSphere(f32 out[4], const Box* b): the centre (min +
// (max - min) * 0.5) and the distance from it to max (name?).
DECOMP(00557870, Box_BoundingSphere) {
    const u32 out = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f32 half = Lf(kHalfSphere);
    const f32 y = Fmadds(Fsubs(Lf(b + 20), Lf(b + 4)), half, Lf(b + 4));
    const f32 x = Fmadds(Fsubs(Lf(b + 16), Lf(b)), half, Lf(b));
    const f32 z = Fmadds(Fsubs(Lf(b + 24), Lf(b + 8)), half, Lf(b + 8));
    store<f32>(out + 4, y);
    store<f32>(out, x);
    store<f32>(out + 8, z);
    const f32 hy = Lf(b + 20), hx = Lf(b + 16), hz = Lf(b + 24);
    const f32 dy = Fsubs(hy, Lf(out + 4)), dx = Fsubs(hx, Lf(out)), dz = Fsubs(hz, Lf(out + 8));
    const f32 r = Fsqrts(Fmadds(dz, dz, Fmadds(dx, dx, Fmuls(dy, dy))));
    store<f32>(out + 12, r);
    ctx.fpr[1] = r;
    ctx.fpr[2] = dx;
    ctx.fpr[3] = dz;
    ctx.fpr[4] = Lf(out + 8);
    ctx.gpr[4] = b + 16;
    ctx.gpr[5] = kHalfSphere;
}

// void InputDevice_Reset(InputDevice* d): the button words, the 96-byte
// state at +32 and +128..+144 cleared, +148 (enabled) = 1 (name?).
DECOMP(00562BAC, InputDevice_Reset) {
    const u32 d = arg<u32>(ctx, 0);
    store<u32>(d + kIdButtons, 0);
    store<u32>(d + kIdChanged, 0);
    store<u32>(d + kIdReleased, 0);
    call(ctx, fn::memset, {d + kIdState, 0, kIdStateSize});
    store<u32>(d + 128, 0);
    store<u32>(d + 132, 0);
    store<u8>(d + kPdEnabled, 1);
    store<u32>(d + 140, 0);
    store<u32>(d + 136, 0);
    store<u32>(d + 144, 0);
    ctx.gpr[3] = d + 132;
    ctx.gpr[4] = d + 144;
    ctx.gpr[5] = d + 140;
    ctx.gpr[6] = 1;
    ctx.gpr[7] = d + 136;
}

namespace {

constexpr u32 libc_strncpy = 0x00723C88;
constexpr u32 Debug_NewPrim = 0x005536F4;       // (u32 kind, colour, 1) -> Prim* (name?)
constexpr u32 PtrVector_Reserve = 0x00551718;

// debug/debug.cpp: the two ApeDebug::Prim pools, +8 the current one, +12
// the previous one; the function-local static thread ids and their guard.
constexpr u32 kPrimPools = 0x01164E50, kPrimPoolCurrent = 0x01164E58, kPrimPoolPrevious = 0x01164E5C;
constexpr u32 kMainThread = 0x011DE290, kMainThreadGuard = 0x011DE2A0;
// A debug text prim: +12 f32[3] the position, +24 f32 the scale, +28 char[96] the text (name?).
constexpr u32 kPrPos = 12, kPrScale = 24, kPrText = 28, kPrTextMax = 95;

// InputDevice_SetAxis's constants: f32 1, f32 0, f64 1, f64 0, f32 -60, f32 FLT_MAX.
constexpr u32 kSaConsts = 0x00562FE0;
constexpr u32 kSaOne = 0, kSaZero = 4, kSaOneD = 8, kSaZeroD = 16, kSaRate = 24, kSaMax = 28;

// 0x01164DEC u32[2] (obj13 kBankState), +8 the PtrVector they are also pushed onto (name?).
constexpr u32 kRegSlots = 0x01164DEC, kRegVector = 0x01164DEC + 8;

}  // namespace

// void Debug_SwapPrimPools(): the current pool becomes the previous one
// and the other pool the current one; the thread-id static is constructed
// if nobody has yet (name?).
DECOMP(0056B4E8, Debug_SwapPrimPools) {
    const u32 second = load<u32>(kPrimPools + 4);
    const u32 cur = load<u32>(kPrimPoolCurrent);
    store<u32>(kPrimPoolPrevious, cur);
    const u32 next = cur == second ? load<u32>(kPrimPools) : second;
    store<u32>(kPrimPoolCurrent, next);
    const u8 guard = load<u8>(kMainThreadGuard);
    if (guard == 0) {
        store<u64>(kMainThread, 0);
        store<u64>(kMainThread + 8, 0);
        store<u8>(kMainThreadGuard, 1);
        ctx.gpr[3] = 1;
        ctx.gpr[4] = 0x011E0000;
        ctx.gpr[5] = 0;
        ctx.gpr[6] = kMainThread;
        ctx.gpr[7] = 0x011E0000;
        return;
    }
    ctx.gpr[3] = guard;
    ctx.gpr[4] = 0x01160000;
    ctx.gpr[5] = 0x011E0000;
}

// bool Debug_FreePrimPools(): both pools freed (Mem_FreePool(p, 0, 0)) and
// cleared; returns 1 (name?).
DECOMP(0056AC78, Debug_FreePrimPools) {
    for (u32 i = 0; i < 2; ++i) {
        const u32 p = load<u32>(kPrimPools + 4 * i);
        if (p == 0) continue;
        call(ctx, obj4::o4fn::Mem_FreePool, {p, 0, 0});
        store<u32>(kPrimPools + 4 * i, 0);
    }
    ctx.gpr[3] = 1;
}

// void Debug_Text3D(const f32 pos[3], const char* text, u32 colour, f32 scale)
// A kind-3 debug prim with the text (95 characters at most), the
// position and the scale (name?).
DECOMP(00553868, Debug_Text3D) {
    const u32 pos = arg<u32>(ctx, 0), text = arg<u32>(ctx, 1);
    const f64 scale = ctx.fpr[1];
    const u32 p = static_cast<u32>(call(ctx, Debug_NewPrim, {3, ctx.gpr[5], 1}));
    call(ctx, libc_strncpy, {p + kPrText, text, kPrTextMax});
    store<u8>(p + kPrText + kPrTextMax, 0);
    store<u32>(p + kPrPos, load<u32>(pos));
    store<u32>(p + kPrPos + 4, load<u32>(pos + 4));
    const u32 z = load<u32>(pos + 8);
    store<f32>(p + kPrScale, static_cast<f32>(scale));
    store<u32>(p + kPrPos + 8, z);
    ctx.gpr[3] = 0;
    ctx.fpr[1] = std::bit_cast<f32>(z);
}

// void InputDevice_SetAxis(InputDevice* d, s32 i, f32 v, f32 dt)
// +132[i] = v clamped to [0, 1]; +140[i] = its rate: v * -60 for dt <= 0,
// -v / dt below FLT_MAX, else 0 (name?).
DECOMP(00563000, InputDevice_SetAxis) {
    const u32 d = arg<u32>(ctx, 0) + (arg<u32>(ctx, 1) << 2);
    const f64 v = ctx.fpr[1], dt = ctx.fpr[2];
    const u32 c = kSaConsts;
    f64 x = Fsubs(Lf(c + kSaOne), v);
    x = static_cast<f32>(x >= 0.0 ? v : load<f64>(c + kSaOneD));
    x = static_cast<f32>(x >= 0.0 ? x : load<f64>(c + kSaZeroD));
    store<f32>(d + kIdAxisHi, static_cast<f32>(x));
    f32 rate = Lf(c + kSaZero);
    ctx.fpr[4] = load<f64>(c + kSaZeroD);
    if (dt < rate || dt == rate) {
        rate = Fmuls(x, Lf(c + kSaRate));
    } else {
        ctx.fpr[4] = Lf(c + kSaMax);
        if (dt < Lf(c + kSaMax)) rate = Fdivs(-x, dt);
    }
    store<f32>(d + kIdAxisLo, rate);
    ctx.fpr[1] = x;
    ctx.fpr[3] = rate;
    ctx.fpr[5] = load<f64>(c + kSaOneD);
    ctx.gpr[3] = d + kIdAxisLo;
    ctx.gpr[4] = c;
    ctx.gpr[5] = c;
}

// void Registry_Set(s32 i, void* v): 0x01164DEC[i] = v and v pushed onto
// the PtrVector at 0x01164DF4 (name?).
DECOMP(00564F60, Registry_Set) {
    const u32 off = static_cast<u32>(arg<s32>(ctx, 0) << 2);
    const u32 v = arg<u32>(ctx, 1);
    const u32 vec = load<u32>(kRegVector);
    store<u32>(kRegSlots + off, v);
    ctx.gpr[5] = kRegSlots;
    u32 cnt = load<u16>(vec + kPvCount);
    const u32 cap = load<u16>(vec + kPvCap);
    if (cnt == cap) {
        call(ctx, PtrVector_Reserve, {vec, cap << 1});
        cnt = load<u16>(vec + kPvCount);
    }
    store<u32>(load<u32>(vec + kPvItems) + 4 * cnt, v);
    const u32 next = load<u16>(vec + kPvCount) + 1;
    const u32 peak = load<u16>(vec + kPvPeak);
    store<u16>(vec + kPvCount, static_cast<u16>(next));
    store<u16>(vec + kPvPeak, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(next & 0xFFFF) ? peak : next));
    ctx.gpr[3] = peak;
    ctx.gpr[4] = next & 0xFFFF;
}

// u32 FileReader_ReadF32Swapped(FileReader* r, f32* out): 4 bytes read
// into out, then byte-reversed in place (through the stack); f1 = the
// value (name?).
DECOMP(005698F4, FileReader_ReadF32Swapped) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 f = load<u32>(r + kFrFile);
    const u32 opd = load<u32>(load<u32>(f) + kVfnFileRead);
    const u32 n = static_cast<u32>(call_ptr(ctx, opd, {f, out, 4, 0, opd, load<u32>(opd)}));
    store<u32>(r + kFrCount, load<u32>(r + kFrCount) + n);
    const u32 raw = load<u32>(out);
    const u32 w = __builtin_bswap32(raw);
    store<u32>(out, w);
    ctx.fpr[1] = std::bit_cast<f32>(w);
    ctx.gpr[3] = n;
    ctx.gpr[4] = (raw >> 8) & 0xFF;   // the bytes the original copied last
    ctx.gpr[5] = (raw >> 16) & 0xFF;
}

namespace {

constexpr u32 Shader_LoadGroup = 0x0050F0F0, Material_Clone = 0x0050956C, Mesh_CreateCube = 0x0051C1A8;
constexpr u32 WorldObj_Unlink = 0x0057FF98, WorldObj_ReleaseRes = 0x00558F34;
constexpr u32 Spline_PointAt = 0x005602BC, Spline_Eval = 0x00560058;
constexpr u32 libc_strlen = 0x0071ED08;
constexpr u32 sys_event_flag_wait = 85, sys_event_flag_clear = 118;
constexpr u32 kEsDone = 28;                     // the event slot's "done" flag

constexpr u32 kZeroVec = 0x01164B40;            // f32[3] 0
constexpr u32 kUnitScale = 0x01164B30;          // f32[3] the row lengths Matrix_IsScaled compares with (name?)
constexpr u32 kBezierConsts = 0x0055EC90;       // f32 {3, 1}
constexpr u32 kBoundsMaterial = 0x00F88960, kBoundsCube = 0x00F8895C;  // (name?)
constexpr u32 kMatBlend = 104;                  // debug/debug.cpp kMaterialBlend
constexpr u32 kKindLists = 0x0119305C;          // KindList*[16] by obj +8 bits 6-9 (name?)
constexpr u32 kKindListsGuard = 0x01196980;     // u8 (an empty static's guard) (name?)
constexpr u32 kKlCount = 44, kKlItems = 36;     // KindList: u16 count, obj** items (name?)
constexpr u32 kVfnRemoveChild = 24;             // obj vfunc (obj, child) (name?)
constexpr u32 kObjFirstChild = 272, kObjNextSibling = 268;
constexpr u32 kObjSplineDef = 208, kSdSpline = 112;  // obj +208 its definition, +112 the SplineArray

inline Vec4 Splat(u32 ea) { return VSplat(VLoadLeft(ea), 0); }

// out = M * p for the 4x4 matrix at m (rows read aligned): rows 0-2
// scaled by p's x, y, z, plus row 3.
inline Vec4 TransformPoint(u32 m, u32 p) {
    Vec4 v = VMadd(VLoad(m), Splat(p), VZero());
    v = VMadd(VLoad(m + 16), Splat(p + 4), v);
    v = VMadd(VLoad(m + 32), Splat(p + 8), v);
    return VAdd(VLoad(m + 48), v);
}

}  // namespace

// void EventSlot_Wait(u32 i): when slot i is pending (+40), waits for its
// done flag (+28), clears it and the pending byte (name?).
DECOMP(0055E2AC, EventSlot_Wait) {
    const u32 off = (arg<u32>(ctx, 0) << 2) & 0x3FFFC;
    const u32 e = load<u32>(kEventSlots + off);
    const u8 pending = load<u8>(e + kEsSet);
    if (pending == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 flag = load<u32>(e + kEsDone);
    ctx.gpr[5] = 1;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = 0;
    if (syscall(ctx, sys_event_flag_wait, {flag, 1, 1, 0, 0}) == 0) {
        ctx.gpr[4] = 0;
        syscall(ctx, sys_event_flag_clear, {load<u32>(e + kEsDone), 0});
    }
    const u32 e2 = load<u32>(kEventSlots + off);
    store<u8>(e2 + kEsSet, 0);
    ctx.gpr[3] = 0;
    ctx.gpr[4] = e2;
}

// void Matrix2_TransformPoint(const f32 m[2][4][4], const f32 p[3], f32 a[4], f32 b[4])
// a = m[0] * p and b = m[1] * p (rows read and results stored aligned) (name?).
DECOMP(00577D8C, Matrix2_TransformPoint) {
    const u32 m = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const Vec4 x = Splat(p), y = Splat(p + 4), z = Splat(p + 8);
    Vec4 va = VMadd(VLoad(m), x, VLoad(m + 48));
    Vec4 vb = VMadd(VLoad(m + 64), x, VLoad(m + 112));
    va = VMadd(VLoad(m + 16), y, va);
    vb = VMadd(VLoad(m + 80), y, vb);
    va = VMadd(VLoad(m + 32), z, va);
    vb = VMadd(VLoad(m + 96), z, vb);
    VStore(a, va);
    VStore(b, vb);
    ctx.gpr[3] = m + 96;
    ctx.gpr[4] = p + 8;
    ctx.gpr[7] = p + 4;
    ctx.gpr[8] = m + 32;
    ctx.gpr[9] = m + 80;
    ctx.gpr[10] = p + 4;
}

// void Matrix_ScaleDiag(f32 m[4][4], const f32 s[3]): rows 0-2 the unit
// axes, row 3 zero (x, y, z; w lanes untouched), then the diagonal = s (name?).
DECOMP(00553BC4, Matrix_ScaleDiag) {
    const u32 m = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u32 rows[4] = {kAxisX, kAxisY, kAxisZ, kZeroVec};
    u32 w = 0;
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 12; k += 4) store<u32>(m + 16 * i + k, load<u32>(rows[i] + k));
    for (u32 k = 0; k < 3; ++k) {
        w = load<u32>(s + 4 * k);
        store<u32>(m + 20 * k, w);
    }
    ctx.fpr[1] = std::bit_cast<f32>(w);
    ctx.gpr[5] = m + 40;
    ctx.gpr[6] = m + 20;
    ctx.gpr[7] = 0x01160000;
    ctx.gpr[8] = kAxisY;
}

// void Resources_InitBoundsCube()
// The bounds material (a clone from shader group 0's) with its blend words
// set (+104: enable off, field 1; +108 u16 FUNC_ADD; +112: field 0x207;
// +110 / +116 u16 0) and a cube mesh for it (name?).
DECOMP(00586B3C, Resources_InitBoundsCube) {
    const u32 group = static_cast<u32>(call(ctx, Shader_LoadGroup, {0}));
    const u32 mat = static_cast<u32>(call(ctx, Material_Clone, {group}));
    const u32 b = mat + kMatBlend;
    store<u32>(kBoundsMaterial, mat);
    const u32 w2 = load<u32>(b + 8), w0 = load<u32>(b);
    store<u16>(b + 6, 0);
    const u32 n2 = (w2 & 0x00007FFFu) | (0x207u << 15);
    const u32 n0 = (w0 & 0x00007FFFu) | (1u << 15);
    store<u32>(b + 8, n2);
    store<u16>(b + 4, 0x8006);  // CELL_GCM_FUNC_ADD
    store<u16>(b + 12, 0);
    store<u32>(b, n0);
    const u32 cube = static_cast<u32>(call(ctx, Mesh_CreateCube, {group, 1, b, n0, 0x8006, n2, 0, w0}));
    store<u32>(kBoundsCube, cube);
    ctx.gpr[4] = 0x00F90000;
}

// void KindList_Remove(Obj* o): takes o off its kind's list (by o +8
// bits 6-9; kind 0 reads a null list, as in the original) by moving the
// last entry into its place (name?).
DECOMP(00588F44, KindList_Remove) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 8);
    const u32 kind = (w >> 6) & 0xF;
    const u32 l = kind != 0 ? load<u32>(kKindLists + 4 * kind) : 0;
    const u8 guard = load<u8>(kKindListsGuard);
    ctx.gpr[5] = guard;
    if (guard == 0) {
        store<u8>(kKindListsGuard, 1);
        ctx.gpr[5] = 0x01190000;
    }
    const u32 n = load<u16>(l + kKlCount);
    ctx.gpr[4] = l;
    ctx.gpr[6] = n;
    s32 i = static_cast<s32>(n) - 1;
    ctx.gpr[7] = static_cast<u64>(static_cast<s64>(i));
    if (n == 0) return;
    u32 items = 0;
    for (;;) {
        items = load<u32>(l + kKlItems);
        const u32 e = load<u32>(items + 4 * static_cast<u32>(i));
        ctx.gpr[5] = items;
        ctx.gpr[8] = 4u * static_cast<u32>(i);
        ctx.gpr[9] = e;
        if (e == o) break;
        ctx.gpr[5] = static_cast<u64>(static_cast<s64>(i));
        ctx.gpr[8] = static_cast<u64>(static_cast<s64>(4 * i - 4));
        const s32 was = i--;
        ctx.gpr[7] = static_cast<u64>(static_cast<s64>(i));
        if (was == 0) return;
    }
    const u32 last = n - 1;
    store<u16>(l + kKlCount, static_cast<u16>(last));
    ctx.gpr[6] = last;
    ctx.gpr[3] = last & 0xFFFF;
    if (static_cast<u32>(i) == (last & 0xFFFF)) return;
    const u32 moved = load<u32>(items + ((last << 2) & 0x3FFFC));
    store<u32>(items + 4 * static_cast<u32>(i), moved);
    ctx.gpr[3] = moved;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(4 * i));
}

// bool Matrix_IsScaled(const f32 m[4][4], f32 eps): a row (0-2) whose
// length differs from 0x01164B30's by eps or more (NaN counts) (name?).
DECOMP(005544FC, Matrix_IsScaled) {
    const u32 m = arg<u32>(ctx, 0);
    const f64 eps = ctx.fpr[1];
    auto len2 = [&](u32 r) { return Fmadds(Lf(r + 8), Lf(r + 8), Fmadds(Lf(r), Lf(r), Fmuls(Lf(r + 4), Lf(r + 4)))); };
    const f32 l0 = Fsqrts(len2(m));
    ctx.gpr[4] = m;
    u32 r = 1;
    if (std::fabs(Fsubs(l0, Lf(kUnitScale))) < eps) {
        ctx.gpr[4] = kUnitScale;
        if (std::fabs(Fsubs(Fsqrts(len2(m + 16)), Lf(kUnitScale + 4))) < eps &&
            std::fabs(Fsubs(Fsqrts(len2(m + 32)), Lf(kUnitScale + 8))) < eps)
            r = 0;
    }
    ctx.gpr[3] = r;
}

// void Bezier_Point(f32 out[3], const f32 ctrl[4][4], f32 u): the cubic
// Bezier point with weights (1-u)^3, 3u(1-u)^2, 3u^2(1-u), u^3; f1 = out[2] (name?).
DECOMP(0055EC98, Bezier_Point) {
    const u32 out = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const f64 u = ctx.fpr[1];
    const f32 three = Lf(kBezierConsts), one = Lf(kBezierConsts + 4);
    const f32 uu = Fmuls(u, u), u3 = Fmuls(u, three), v = Fsubs(one, u);
    const f32 w3 = Fmuls(uu, u);
    const f32 w2 = Fmuls(Fmuls(u, u3), v);
    const f32 vv = Fmuls(v, v);
    const f32 w1 = Fmuls(vv, u3);
    const f32 w0 = Fmuls(v, vv);
    f32 r[3];
    for (u32 k = 0; k < 3; ++k) {
        f32 t = Fmuls(Lf(c + 32 + 4 * k), w2);
        t = Fmadds(Lf(c + 48 + 4 * k), w3, t);
        t = Fmadds(Lf(c + 16 + 4 * k), w1, t);
        r[k] = Fmadds(Lf(c + 4 * k), w0, t);
    }
    store<f32>(out, r[0]);
    store<f32>(out + 4, r[1]);
    store<f32>(out + 8, r[2]);
    ctx.fpr[1] = r[2];
    ctx.fpr[2] = r[0];
    ctx.fpr[4] = r[1];
    ctx.gpr[5] = kBezierConsts;
}

// void WorldObj_Shutdown(WorldObj* o): unlinked, then each child handed to
// vfunc +24 (o, child) (the next sibling read first), then
// WorldObj_ReleaseRes (name?).
DECOMP(00583B48, WorldObj_Shutdown) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, WorldObj_Unlink, {o});
    for (u32 c = load<u32>(o + kObjFirstChild); c != 0;) {
        const u32 next = load<u32>(c + kObjNextSibling);
        const u32 opd = load<u32>(load<u32>(o) + kVfnRemoveChild);
        call_ptr(ctx, opd, {o, c, opd, load<u32>(opd)});
        c = next;
    }
    call(ctx, WorldObj_ReleaseRes, {o});
}

// u32 WorldObj_SplinePointAt(WorldObj* o, -, f32 out[4], bool b; f1 t)
// Spline_PointAt on the definition's spline (r4 passed through), then out
// through the object's matrix (+80), stored aligned; r3 = Spline_PointAt's (name?).
DECOMP(00592748, WorldObj_SplinePointAt) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    callf(ctx, Spline_PointAt, {load<u32>(o + kObjSplineDef) + kSdSpline, ctx.gpr[4], out, ctx.gpr[6] & 0xFF}, {ctx.fpr[1]});
    const u64 r = ctx.gpr[3];
    VStore(out, TransformPoint(o + obj12::kObjOwnMatrix, out));
    ctx.gpr[3] = r;
    ctx.gpr[4] = o + 128;
    ctx.gpr[5] = o + 112;
    ctx.gpr[6] = o + 112;
}

// u32 WorldObj_SplineEval(WorldObj* o, -, f32 out[4], u32 seg, bool back; f1 t)
// The same with Spline_Eval (r4 / r6 passed through) (name?).
DECOMP(005A81CC, WorldObj_SplineEval) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    callf(ctx, Spline_Eval, {load<u32>(o + kObjSplineDef) + kSdSpline, ctx.gpr[4], out, ctx.gpr[6], ctx.gpr[7] & 0xFF}, {ctx.fpr[1]});
    const u64 r = ctx.gpr[3];
    VStore(out, TransformPoint(o + obj12::kObjOwnMatrix, out));
    ctx.gpr[3] = r;
    ctx.gpr[4] = o + 128;
    ctx.gpr[5] = o + 112;
    ctx.gpr[6] = o + 96;
    ctx.gpr[7] = 8;
}

// const char* String_FindFirstOf(const char* s, const char* set): strpbrk
// on signed characters, 0 when none (name?).
DECOMP(005565F0, String_FindFirstOf) {
    const u32 s = arg<u32>(ctx, 0), set = arg<u32>(ctx, 1);
    const s32 n = static_cast<s32>(call(ctx, libc_strlen, {s}));
    const s32 m = static_cast<s32>(call(ctx, libc_strlen, {set}));
    s32 i = 0;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(m));
    for (; i < n; ++i) {
        ctx.gpr[6] = 0;
        for (s32 j = 0; j < m; ++j) {
            const s32 a = static_cast<s8>(load<u8>(s + i)), b = static_cast<s8>(load<u8>(set + j));
            ctx.gpr[7] = static_cast<u64>(static_cast<s64>(a));
            ctx.gpr[8] = static_cast<u64>(static_cast<s64>(b));
            if (a == b) {
                ctx.gpr[4] = static_cast<u64>(static_cast<s64>(i));
                ret(ctx, s + static_cast<u32>(i));
                return;
            }
            ctx.gpr[6] = static_cast<u64>(static_cast<s64>(j + 1));
        }
    }
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(i));
    ctx.gpr[3] = 0;
}

// void GeomBase_Delete(ApeGeomBase* d) / LightDef_Delete / CameraDef_Delete
// / PointDef_Delete: the deleting destructors (vtable +4 of 0x00B246B0 /
// 0x00B24700 / 0x00B247B8 / 0x00B24808, one body): the geometry-base
// destructor, then Mem_FreeFrom(d, 0).
namespace {
void GeomBaseDelete(PPUContext& ctx) {
    const u32 d = arg<u32>(ctx, 0);
    if (d == 0) return;
    obj11::GeomBaseDestructBody(ctx, d);
    call(ctx, o9fn::Mem_FreeFrom, {d, 0});
}
}  // namespace
DECOMP(00584B14, GeomBase_Delete) { GeomBaseDelete(ctx); }
DECOMP(00586324, LightDef_Delete) { GeomBaseDelete(ctx); }
DECOMP(0059C9B4, CameraDef_Delete) { GeomBaseDelete(ctx); }
DECOMP(0059E0F8, PointDef_Delete) { GeomBaseDelete(ctx); }

namespace {

constexpr u32 Holder_FreeData = 0x0055F27C;     // gfx/gfx8.h (Holder*)
constexpr u32 WorldObj_SetDrawCallbackFn = 0x00583C04;
constexpr u32 kVtblSplineDef = 0x00B24830;      // ApeSplineDef (name?)
constexpr u32 kSplineRecords = 0x01194580 + 12; // obj5.h kK13List +12: 96-byte records
constexpr u32 kSplineRecSize = 96;
constexpr u32 kObjSplineRec = 284;              // obj s32: its record index (-1 none) (name?)
constexpr u32 kObjPeerIdx = 164;                // u16 (obj3.h kSlotPeers index; 0xFFFF none)
constexpr u32 kPeerSize = 160, kPeerLink = 136; // peer +136: a record with +100 / +104 pointers into the list (name?)
constexpr u32 kPlA = 100, kPlB = 104;
constexpr u32 kKindDrawCb = 8;                  // kind whose +332 / +336 take the callback
constexpr u32 kKindDrawCbLast = 14;             // children of kinds 8..14 are visited
constexpr u32 kBoxHalf = 0x005571BC;            // f32 0.5

inline Vec4 VSub(const Vec4& a, const Vec4& b) {  // vsubfp
    Vec4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = std::bit_cast<u32>(a.f(i) - b.f(i));
    return r;
}
inline Vec4 VAbs(const Vec4& a) {  // vandc with the sign mask
    Vec4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = a.w[i] & 0x7FFFFFFFu;
    return r;
}

}  // namespace

// Entry* WorldObj_SetSplineRecord(WorldObj* o, s32 i)
// +284 = i; the peer record's (+164) link (+136) pointers (+100 / +104)
// that pointed at the old record are moved to the new one (0 for -1).
// Returns the new record (name?).
DECOMP(005A8C30, WorldObj_SetSplineRecord) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 i = ctx.gpr[4];
    const u32 base = load<u32>(kSplineRecords);
    const u32 old = base + kSplineRecSize * load<u32>(o + kObjSplineRec);
    const u32 peer = load<u16>(o + kObjPeerIdx);
    const u32 rec = static_cast<s32>(static_cast<u32>(i)) == -1 ? 0u : base + kSplineRecSize * static_cast<u32>(i);
    const u32 peers = load<u32>(obj3::kSlotPeers);
    store<u32>(o + kObjSplineRec, static_cast<u32>(i));
    const u32 list = load<u32>(peers);
    ctx.gpr[3] = rec;
    ctx.gpr[5] = old;
    ctx.gpr[8] = peers;
    ctx.gpr[9] = 0xFFFF;
    ctx.gpr[7] = list;
    ctx.gpr[6] = peer;
    if (peer == 0xFFFF) return;
    const u32 p = load<u32>(list) + peer * kPeerSize;
    ctx.gpr[4] = peer * kPeerSize;
    ctx.gpr[6] = load<u32>(list);
    ctx.gpr[7] = p;
    if (p == 0) return;
    const u32 l = load<u32>(p + kPeerLink);
    ctx.gpr[7] = l;
    if (l == 0) return;
    if (load<u32>(l + kPlA) == old) store<u32>(l + kPlA, rec);
    const u32 b = load<u32>(l + kPlB);
    ctx.gpr[4] = b;
    if (b == old) store<u32>(l + kPlB, rec);
}

// bool Box_Intersects(const Box* a, const Box* b): no axis (x, y, z) on
// which one box's min exceeds the other's max (NaN counts as overlap) (name?).
DECOMP(005574A0, Box_Intersects) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    u32 r = 0;
    ctx.gpr[5] = 0;
    auto sep = [&](u32 p, u32 q) {
        const f32 x = Lf(p), y = Lf(q);
        ctx.fpr[1] = x;
        ctx.fpr[2] = y;
        return x > y;
    };
    if (!sep(a, b + 16)) {
        ctx.gpr[6] = a + 16;
        if (!sep(b, a + 16)) {
            ctx.gpr[6] = a + 4; ctx.gpr[7] = b + 20;
            if (!sep(a + 4, b + 20)) {
                ctx.gpr[6] = b + 4; ctx.gpr[7] = a + 20;
                if (!sep(b + 4, a + 20)) {
                    ctx.gpr[6] = a + 8; ctx.gpr[7] = b + 24;
                    if (!sep(a + 8, b + 24)) {
                        ctx.gpr[4] = b + 8;
                        if (!sep(b + 8, a + 24)) r = 1;
                    }
                }
            }
        }
    }
    ctx.gpr[5] = r;
    ctx.gpr[3] = r;
}

// void SplineDef_Destruct(ApeSplineDef* d): the spline's points (+112
// Holder_FreeData), then the geometry-base destructor (name?).
DECOMP(0059EF6C, SplineDef_Destruct) {
    const u32 d = arg<u32>(ctx, 0);
    store<u32>(d, kVtblSplineDef);
    call(ctx, Holder_FreeData, {d + kSdSpline, kVtblSplineDef});
    obj11::GeomBaseDestructBody(ctx, d);
}

// void Matrix_ScaleAxes(f32 m[4][4], const f32 s[3]): every row's x, y, z
// (translation included) times s (name?).
DECOMP(005540F8, Matrix_ScaleAxes) {
    const u32 m = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    f32 last = 0;
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 3; ++k) {
            last = Fmuls(Lf(m + 16 * i + 4 * k), Lf(s + 4 * k));
            store<f32>(m + 16 * i + 4 * k, last);
        }
    ctx.fpr[1] = last;
}

// void Box_TransformInPlace(Box* b, const f32 m[4][4])
// The axis-aligned bounds of b through m: centre (max + min) * 0.5
// through m, extent (max - min) * 0.5 through |m|; all lanes computed,
// everything read and stored aligned (name?).
DECOMP(005571C0, Box_TransformInPlace) {
    const u32 b = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const Vec4 lo = VLoad(b), r0 = VLoad(m), hi = VLoad(b + 16);
    const Vec4 half = Splat(kBoxHalf);
    const Vec4 r3 = VLoad(m + 48), r1 = VLoad(m + 16), r2 = VLoad(m + 32);
    const Vec4 c = VMadd(VAdd(hi, lo), half, VZero()), e = VMadd(VSub(hi, lo), half, VZero());
    Vec4 p = VMadd(r0, VSplat(c, 0), r3);
    Vec4 q = VMadd(VAbs(r0), VSplat(e, 0), VZero());
    p = VMadd(r1, VSplat(c, 1), p);
    q = VMadd(VAbs(r1), VSplat(e, 1), q);
    p = VMadd(r2, VSplat(c, 2), p);
    q = VMadd(VAbs(r2), VSplat(e, 2), q);
    VStore(b, VSub(p, q));
    VStore(b + 16, VAdd(p, q));
    ctx.gpr[4] = m + 32;
    ctx.gpr[5] = m + 16;
    ctx.gpr[6] = b + 16;
    ctx.gpr[7] = m + 48;
    ctx.gpr[8] = m + 16;
}

// void WorldObj_SetDrawCallback(WorldObj* o, -, OPD* cb, void* arg, bool deep)
// Kind-8 objects take the draw callback (+332 / +336); with deep, the
// children of kinds 8..14 are visited too (r4 passed along) (name?).
DECOMP(00583C04, WorldObj_SetDrawCallback) {
    const u32 o = arg<u32>(ctx, 0), cb = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3);
    const u64 r4 = ctx.gpr[4];
    const u32 deep = static_cast<u32>(ctx.gpr[7]) & 0xFF;
    const u32 kind = obj4::Kind(o);
    ctx.gpr[4] = kind;
    if (kind == kKindDrawCb) {
        store<u32>(o + obj12::kObjDrawCb, cb);
        store<u32>(o + obj12::kObjDrawCbArg, a);
    }
    if (deep == 0) return;
    for (u32 c = load<u32>(o + kObjFirstChild); c != 0; c = load<u32>(c + kObjNextSibling)) {
        const u32 k = obj4::Kind(c);
        ctx.gpr[3] = k;
        if (k >= kKindDrawCb && k <= kKindDrawCbLast) call(ctx, WorldObj_SetDrawCallbackFn, {c, r4, cb, a, deep});
    }
}

namespace {

constexpr u32 GeomInstance_DrawLod = 0x0058AC70, WorldObj_UpdateLight = 0x005832FC, ObjLight_Apply = 0x00582A84;
constexpr u32 View_GetCurrent = 0x004FDBA8;
constexpr u32 Fn_5132D8 = 0x005132D8, Fn_5754E0 = 0x005754E0, Fn_57328C = 0x0057328C;  // (name?)
constexpr u32 libc_memmove = 0x0071C3F0;

// The 16-byte draw entries {obj, u8 +8 lod, u8 +9, u8 +10 bits 0-1 light
// mode} from a base up to the end pointer the list header holds (name?).
constexpr u32 kDrawEntriesA = 0x0116E110, kDrawEndA = 0x0117E280 + 3100;  // u32* -> the end
constexpr u32 kDrawEntriesB = 0x01183100, kDrawEndB = 0x0118B180 + 3100;
constexpr u32 kDeObj = 0, kDeLod = 8, kDe9 = 9, kDeLight = 10, kDeSize = 16;
constexpr u32 kObjLodBits = 320;                // u32: bits 18-23 the LOD argument (name?)

// The memory file's data pointer (kMf280).
constexpr u32 kMfData = 280;

constexpr u32 kJdBinary = 0x00D43B00;           // the job's binary EA (name?)
constexpr u32 kJdBinarySize = (0x00023F58u + 15) >> 4;
constexpr u64 kJdFlags48 = 0x0000009000000000ull;  // 0x0056EBB0

constexpr u32 kRenderFlag = 0x0119509C;         // u8 (name?)
constexpr u32 kRenderListA = 0x011957E8, kRenderListB = 0x0119582C;  // f_005132D8 targets (name?)
constexpr u32 kRenderCountA = 0x011957E2, kRenderCountB = 0x011957E4;  // u16 (name?)
constexpr u32 kConfigRender82 = 0x010C5A20 + 82; // g::config u8 (name?)

// KindList (KindList_Remove): +36 PtrVector of objects, +56 u8 the kind.
constexpr u32 kKlVector = 36, kKlKind = 56;
constexpr u32 kObjKindBits = 0x3C0;             // obj +8 bits 6-9

void UpdateLightList(PPUContext& ctx, u32 base, u32 endp) {
    const u32 end = load<u32>(load<u32>(endp));
    ctx.gpr[3] = endp - 3100;  // the header, as the original leaves it
    if (static_cast<s32>(base) == static_cast<s32>(end)) return;
    u32 e = base;
    for (;;) {
        const u32 next = e + kDeSize;
        const u32 o = load<u32>(e + kDeObj);
        ctx.gpr[3] = o;
        ctx.gpr[4] = e;
        if (o != 0) {
            const u32 mode = load<u8>(e + kDeLight) & 3;
            ctx.gpr[4] = mode;
            if (mode == 2) call(ctx, ObjLight_Apply, {o, mode});
            else if (mode != 3) call(ctx, WorldObj_UpdateLight, {o, mode});
        }
        if (!(next < end)) break;
        e = next;
    }
}

}  // namespace

// void DrawList_DrawGeomInstances(): GeomInstance_DrawLod(obj, +8, +9,
// obj +320 bits 18-23) for each entry with an object (name?).
DECOMP(0058AF94, DrawList_DrawGeomInstances) {
    const u32 end = load<u32>(load<u32>(kDrawEndA));
    const u32 bytes = end - kDrawEntriesA;
    if ((bytes >> 4) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 last = kDrawEntriesA + (bytes & ~15u);
    for (u32 e = kDrawEntriesA;;) {
        const u32 next = e + kDeSize;
        const u32 o = load<u32>(e + kDeObj);
        ctx.gpr[3] = o;
        ctx.gpr[4] = e;
        if (o != 0)
            call(ctx, GeomInstance_DrawLod, {o, load<u8>(e + kDeLod), load<u8>(e + kDe9), (load<u32>(o + kObjLodBits) >> 18) & 0x3F});
        if (!(next < last)) break;
        e = next;
    }
}

// void DrawList_UpdateLightsA() / DrawList_UpdateLightsB(): for each entry
// with an object, light mode 2: ObjLight_Apply; 0 / 1: WorldObj_UpdateLight;
// 3: nothing (name?).
DECOMP(0058B05C, DrawList_UpdateLightsA) { UpdateLightList(ctx, kDrawEntriesA, kDrawEndA); }
DECOMP(0058C8D8, DrawList_UpdateLightsB) { UpdateLightList(ctx, kDrawEntriesB, kDrawEndB); }

// u32 PairBuffer_PushData(PairBuffer* b, u32 x, const void* src, u32 size)
// Copies size bytes below the data top (+16, 16-aligned downwards) and
// appends {x, the copy} at the cursor, unless the used total (+20) plus
// the copy and the entry would pass +0; +20 grows by the data taken, the
// count by 1. Returns size when full, else the new +20 (name?).
DECOMP(00565044, PairBuffer_PushData) {
    const u32 b = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), src = arg<u32>(ctx, 2), size = arg<u32>(ctx, 3);
    const u32 top = load<u32>(b + kPbTop), used = load<u32>(b + kPbCheck);
    const u32 dst = (top - size) & ~15u;
    const u32 cur = load<u32>(b + kPbCursor), end = load<u32>(b + kPbEnd);
    const u32 need = used + top - dst + kPbEntry;
    const u32 taken = top - dst + kPbEntry;
    ctx.gpr[5] = x;
    ctx.gpr[6] = cur;
    ctx.gpr[7] = size;
    ctx.gpr[8] = src;
    ctx.gpr[9] = need;
    ctx.gpr[4] = src;
    ctx.gpr[3] = size;
    if (need > end) return;
    store<u32>(cur, x);
    store<u32>(cur + 4, dst);
    call(ctx, libc_memmove, {dst, src, size, cur, x, src, need});
    const u32 u = load<u32>(b + kPbCheck) + taken, n = load<u32>(b + kPbCount) + 1;
    store<u32>(b + kPbCursor, cur + kPbEntry);
    store<u32>(b + kPbTop, dst);
    store<u32>(b + kPbCheck, u);
    store<u32>(b + kPbCount, n);
    ctx.gpr[3] = u;
    ctx.gpr[4] = n;
}

// s32 MemFile_Read(MemFile* f, void* buf, u64 size, u8* done)
// min(size, size of file - position) bytes from the data (+280) at the
// position (memmove, called even for 0), the position moved on; *done =
// 1 whenever done is given. 0 without data (name?).
DECOMP(00567184, MemFile_Read) {
    const u32 f = arg<u32>(ctx, 0), buf = arg<u32>(ctx, 1), done = arg<u32>(ctx, 3);
    const u64 size = ctx.gpr[5];
    const u32 data = load<u32>(f + kMfData);
    if (data == 0) {
        ctx.gpr[3] = 0;
        ctx.gpr[4] = 0;
        return;
    }
    const u64 pos = load<u64>(f + kMfPos);
    const u64 avail = load<u64>(f + kMfSize) - pos;
    u64 n = size < avail ? size : avail;
    if (n == 0) n = 0;
    const u32 n32 = static_cast<u32>(n);
    call(ctx, libc_memmove, {buf, static_cast<u32>(data + pos), n32, pos, n, n});
    store<u64>(f + kMfPos, load<u64>(f + kMfPos) + n32);
    if (done != 0) store<u8>(done, 1);
    ret(ctx, static_cast<s32>(n32));
}

// void JobDesc_Init(JobOwner* o, u32 a, u32 b, u32 c, u64 d)
// Fills o->+4's 80-byte job descriptor: zeroed (aligned), binary
// 0x00D43B00 of 0x23F6 qwords, +16 1, +20 144, +10 8, +44 0, +48 a |
// 0x90 << 32, +56 b, +64 c, +72 d (name?).
DECOMP(0056EBB8, JobDesc_Init) {
    const u32 j = load<u32>(arg<u32>(ctx, 0) + 4);
    const u64 a = ctx.gpr[4] & 0xFFFFFFFFu, b = ctx.gpr[5] & 0xFFFFFFFFu, c = ctx.gpr[6] & 0xFFFFFFFFu, d = ctx.gpr[7];
    VStore(j, VZero());
    store<u64>(j, kJdBinary);
    store<u16>(j + 8, static_cast<u16>(kJdBinarySize));
    VStore(j + 32, VZero());
    store<u8>(j + 44, 0);
    VStore(j + 16, VZero());
    store<u32>(j + 20, 144);
    store<u32>(j + 16, 1);
    store<u64>(j + 48, a | kJdFlags48);
    store<u16>(j + 10, 8);
    store<u64>(j + 56, b);
    store<u64>(j + 64, c);
    store<u64>(j + 72, d);
    ctx.gpr[3] = j;
    ctx.gpr[4] = a | kJdFlags48;
    ctx.gpr[5] = b;
    ctx.gpr[6] = c;
    ctx.gpr[8] = kJdFlags48;
    ctx.gpr[9] = kJdBinarySize;
    ctx.gpr[10] = kJdBinary;
    ctx.gpr[11] = 32;
    ctx.gpr[12] = 16;
}

// void Render_Pass575C34(): when enabled (0x0119509C) and the current view
// has +256 bit 1 and +52 bit 2: f_005132D8 on the first list, then (with
// g::config +82) f_005754E0 / f_0057328C for nonzero counts (passed in r3),
// and f_005132D8 on the second list (name?).
DECOMP(00575C34, Render_Pass575C34) {
    if (load<u8>(kRenderFlag) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 v = static_cast<u32>(call(ctx, View_GetCurrent, {}));
    const bool a = (load<u32>(v + 256) & 2) != 0, b = (load<u8>(v + 52) & 4) != 0;
    ctx.gpr[3] = (a && b) ? 1 : 0;
    ctx.gpr[4] = a ? 1 : 0;
    ctx.gpr[5] = load<u32>(v + 256) & 2;
    if (!(a && b)) return;
    call(ctx, Fn_5132D8, {kRenderListA});
    const u8 on = load<u8>(kConfigRender82);
    ctx.gpr[3] = on;
    if (on == 0) return;
    const u32 n1 = load<u16>(kRenderCountA);
    ctx.gpr[3] = n1;
    if (static_cast<s32>(n1) > 0) call(ctx, Fn_5754E0, {n1});
    const u32 n2 = load<u16>(kRenderCountB);
    ctx.gpr[3] = n2;
    if (static_cast<s32>(n2) > 0) call(ctx, Fn_57328C, {n2});
    call(ctx, Fn_5132D8, {kRenderListB});
}

// void KindList_Add(Obj* o, KindList* l, s32 mode)
// Mode 1: o pushed onto the list (+36 PtrVector) and o +8 bits 6-9 = the
// list's kind (+56); mode 0: only the kind bits; others nothing (name?).
DECOMP(00588E70, KindList_Add) {
    const u32 o = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1);
    const s32 mode = static_cast<s32>(static_cast<u32>(ctx.gpr[5]));
    if (mode != 0 && mode != 1) return;
    if (mode == 1) {
        const u32 vec = l + kKlVector;
        u32 cnt = load<u16>(vec + kPvCount);
        const u32 cap = load<u16>(vec + kPvCap);
        if (cnt == cap) {
            call(ctx, PtrVector_Reserve, {vec, (cap << 1) & 0x1FFFE});
            cnt = load<u16>(vec + kPvCount);
        }
        const u32 items = load<u32>(vec + kPvItems);
        const u32 next = load<u16>(vec + kPvCount) + 1;
        const u32 peak = load<u16>(vec + kPvPeak);
        store<u32>(items + ((cnt << 2) & 0x3FFFC), o);
        store<u16>(vec + kPvCount, static_cast<u16>(next));
        store<u16>(vec + kPvPeak, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(next & 0xFFFF) ? peak : next));
        ctx.gpr[5] = peak;
        ctx.gpr[6] = next & 0xFFFF;
    }
    const u32 kind = load<u8>(l + kKlKind);
    const u32 w = (load<u32>(o + 8) & ~kObjKindBits) | ((kind << 6) & kObjKindBits);
    store<u32>(o + 8, w);
    ctx.gpr[4] = kind;
    if (mode == 0) ctx.gpr[5] = w;
    else ctx.gpr[3] = w;
}

namespace {

constexpr u32 FsOp_GetError = 0x00570C38, FsOp_ErrorName = 0x00570C98, FsOp_GetPath = 0x00570D80;
constexpr u32 String_Format = 0x005534D0;
constexpr u32 Records_Fn58C9B0 = 0x0058C9B0;    // (obj, record, vfunc +52 result) (name?)
constexpr u32 kFsErrBusy = 0x80010708u;         // the error FsOp_Finish does not report (name?)
constexpr u32 kFsErrTable = 0x00F88724;         // {s32 code, const char* name}[7] (name?)
constexpr u32 kFsErrCount = 7;
constexpr u32 kStrUnknownCode = 0x00AF3950;     // "Unknown:0x%x"
constexpr u32 kRecordsInfo = 0x0118E080;        // +12 u16 handed to vfunc +52 (name?)
constexpr u32 kRecordSize = 128;
constexpr u32 kVfnRecords = 52;                 // obj vfunc (obj, u16, 0) (name?)

// The rotation matrices' .text constants (gfx7.h kRotZConsts layout): f64
// pi, f64 -pi, f32 2pi, 4/pi, -4/pi^2, 0.225, pi/2, pi.
constexpr u32 kRotXConsts = 0x00553CF0, kRotYConsts = 0x00553DF0;

struct SinCos {
    f32 sine, neg_sine, cosine;
    f32 fr[10];  // f1..f10 as the original leaves them
};

// The parabolic sine / cosine of Matrix_RotationZ (gfx7_math.cpp) with
// the original's exact single-precision steps.
SinCos ParabolicSinCos(u32 k, f64 a) {
    const f32 two_pi = Lf(k + 16), four_pi = Lf(k + 20), parab = Lf(k + 24);
    const f32 refine = Lf(k + 28), half_pi = Lf(k + 32), pi = Lf(k + 36);
    const f32 bias = static_cast<f32>(a >= 0.0 ? load<f64>(k) : load<f64>(k + 8));
    f32 t = Fadds(bias, a);
    const f32 turns = static_cast<f32>(rt::fcfid(rt::fctid(Fdivs(t, two_pi), true)));
    t = Fnmsubs(turns, two_pi, t);
    const f32 x = Fsubs(t, bias);
    const f32 ax = std::fabs(x);
    f32 c = Fadds(x, half_pi);
    const f32 ys = Fmadds(x, four_pi, Fmuls(Fmuls(x, parab), ax));
    const f32 es = Fmsubs(ys, std::fabs(ys), ys);
    const f32 sine = Fmadds(es, refine, ys);
    if (c > pi) c = Fsubs(c, two_pi);
    const f32 ac = std::fabs(c);
    const f32 neg = static_cast<f32>(-std::fma(static_cast<f64>(es), static_cast<f64>(refine), static_cast<f64>(ys)));
    const f32 yc = Fmadds(c, four_pi, Fmuls(Fmuls(c, parab), ac));
    const f32 ec = Fmsubs(yc, std::fabs(yc), yc);
    const f32 cosine = Fmadds(ec, refine, yc);
    return {sine, neg, cosine, {cosine, ec, yc, four_pi, ac, c, sine, es, ax, Lf(k + 32)}};
}

}  // namespace

// s32 MemFile_Write(MemFile* f, const void* buf, u64 size, u8* done)
// MemFile_Read the other way: min(size, room) bytes into the data at the
// position (memmove, called even for 0) (name?).
DECOMP(0056724C, MemFile_Write) {
    const u32 f = arg<u32>(ctx, 0), buf = arg<u32>(ctx, 1), done = arg<u32>(ctx, 3);
    const u64 size = ctx.gpr[5];
    const u32 data = load<u32>(f + kMfData);
    if (data == 0) {
        ctx.gpr[3] = 0;
        ctx.gpr[4] = 0;
        return;
    }
    const u64 pos = load<u64>(f + kMfPos);
    const u64 avail = load<u64>(f + kMfSize) - pos;
    const u64 n = size < avail ? size : avail;
    const u32 n32 = static_cast<u32>(n);
    call(ctx, libc_memmove, {static_cast<u32>(data + pos), buf, n32, pos, n, n});
    store<u64>(f + kMfPos, load<u64>(f + kMfPos) + n32);
    if (done != 0) store<u8>(done, 1);
    ret(ctx, static_cast<s32>(n32));
}

// void Vec4_TransformByMatrix(f32 out[4], const f32 v[4], const f32 m[4][4])
// out = v.x m0 + v.y m1 + v.z m2 + v.w m3, all four lanes, out updated
// after each row (each v element read after the previous row's stores,
// so out may alias v) (name?).
DECOMP(00553910, Vec4_TransformByMatrix) {
    const u32 out = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    f32 acc[4], row2[4] = {};
    f32 s = 0;
    for (u32 i = 0; i < 4; ++i) {
        s = Lf(v + 4 * i);
        for (u32 k = 0; k < 4; ++k) {
            const f32 e = Lf(m + 16 * i + 4 * k);
            acc[k] = i == 0 ? Fmuls(e, s) : Fmadds(e, s, acc[k]);
            store<f32>(out + 4 * k, acc[k]);
        }
        if (i == 2) for (u32 k = 0; k < 4; ++k) row2[k] = acc[k];
    }
    ctx.fpr[1] = acc[3];
    ctx.fpr[2] = Lf(m + 60);
    ctx.fpr[3] = row2[1];  // row 3 accumulates y and z in f2
    ctx.fpr[4] = row2[2];
    ctx.fpr[5] = s;
    ctx.fpr[6] = Lf(m + 44);
}

// f32 Matrix_RotationX(f32 m[4][4], f32 a) / Matrix_RotationY: the
// rotation by a (wrapped into [-pi, pi], the parabolic sine / cosine of
// Matrix_RotationZ); row 3 zero, w lanes untouched. Returns the cosine.
DECOMP(00553D18, Matrix_RotationX) {
    const u32 m = arg<u32>(ctx, 0);
    const SinCos r = ParabolicSinCos(kRotXConsts, ctx.fpr[1]);
    store<u32>(m, 0x3F800000u);
    store<u32>(m + 4, 0);
    store<u32>(m + 8, 0);
    store<u32>(m + 16, 0);
    store<f32>(m + 24, r.sine);
    store<u32>(m + 32, 0);
    store<f32>(m + 36, r.neg_sine);
    store<u32>(m + 48, 0);
    store<u32>(m + 52, 0);
    store<u32>(m + 56, 0);
    store<f32>(m + 20, r.cosine);
    store<f32>(m + 40, r.cosine);
    for (u32 k = 0; k < 10; ++k) ctx.fpr[1 + k] = r.fr[k];
    ctx.gpr[4] = 0x3F800000u;
    ctx.gpr[5] = 0;
}
DECOMP(00553E18, Matrix_RotationY) {
    const u32 m = arg<u32>(ctx, 0);
    const SinCos r = ParabolicSinCos(kRotYConsts, ctx.fpr[1]);
    store<u32>(m + 4, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 0x3F800000u);
    store<f32>(m + 8, r.neg_sine);
    store<u32>(m + 24, 0);
    store<f32>(m + 32, r.sine);
    store<u32>(m + 36, 0);
    store<u32>(m + 48, 0);
    store<u32>(m + 52, 0);
    store<u32>(m + 56, 0);
    store<f32>(m, r.cosine);
    store<f32>(m + 40, r.cosine);
    for (u32 k = 0; k < 10; ++k) ctx.fpr[1 + k] = r.fr[k];
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0x3F800000u;
}

// void Records_Process58CE24(Obj* o, Record* base, u32 n)
// x = vfunc +52 (o, 0x0118E080 +12, 0), then f_0058C9B0(o, record, x) for
// the 128-byte records from base (the first one even for n = 0) (name?).
DECOMP(0058CE24, Records_Process58CE24) {
    const u32 o = arg<u32>(ctx, 0), base = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    const u32 opd = load<u32>(load<u32>(o) + kVfnRecords);
    const u32 x = static_cast<u32>(call_ptr(ctx, opd, {o, load<u16>(kRecordsInfo + 12), 0, opd, load<u32>(opd)}));
    const u32 end = base + ((n << 7) & 0xFFFFFF80u);
    u32 e = base;
    do {
        call(ctx, Records_Fn58C9B0, {o, e, x});
        e += kRecordSize;
        ctx.gpr[3] = e;
    } while (e < end);
}

// void SplineDef_Delete(ApeSplineDef* d): the deleting destructor (name?).
DECOMP(0059F030, SplineDef_Delete) {
    const u32 d = arg<u32>(ctx, 0);
    if (d == 0) return;
    store<u32>(d, kVtblSplineDef);
    call(ctx, Holder_FreeData, {d + kSdSpline, d + kSdSpline, kVtblSplineDef});
    obj11::GeomBaseDestructBody(ctx, d);
    call(ctx, o9fn::Mem_FreeFrom, {d, 0});
}

// void FsOp_Finish(FsOp* op)
// When the op is pending (+20): clears it and, for an error other than 0
// or 0x80010708, formats "Unknown:0x%x" for a code (+4) missing from the
// error table and reads the path and error name (debug output compiled
// out); then always the completion callback (+12) (op) (name?).
DECOMP(00561374, FsOp_Finish) {
    const u32 op = arg<u32>(ctx, 0);
    if (load<u8>(op + 20) != 0) {
        store<u8>(op + 20, 0);
        ctx.gpr[4] = 0;
        const u32 err = static_cast<u32>(call(ctx, FsOp_GetError, {op, 0}));
        if (err != 0 && err != kFsErrBusy) {
            const u32 code = load<u32>(op + 4);
            u32 i = 0, e = 0;
            for (; i < kFsErrCount; ++i) {
                e = load<u32>(kFsErrTable + 8 * i);
                if (e == code) break;
            }
            ctx.gpr[4] = kFsErrTable;
            ctx.gpr[5] = 8 * i;
            ctx.gpr[6] = i;
            ctx.gpr[7] = e;
            if (i == kFsErrCount)
                call(ctx, String_Format, {kStrUnknownCode, static_cast<u64>(static_cast<s64>(static_cast<s32>(code))), 0x00AF0000, i, e});
            call(ctx, FsOp_GetPath, {op});
            call(ctx, FsOp_ErrorName, {op});
        }
    }
    const u32 opd = load<u32>(op + 12);
    call_ptr(ctx, opd, {op, opd, load<u32>(opd)});
}

namespace {

constexpr u32 Spline_PointTangentFn = 0x005606C8, Spline_PointTangentAtFn = 0x00560A10;
constexpr u32 WorkQueue_Fn56E4C0 = 0x0056E4C0;  // (QueueRef*, first, stride*, count*, counter*) (name?)
constexpr u32 kWorkMutex = 0x0116B580;          // sys_lwmutex_t (the static Sysutil_StaticDestroy resets) (name?)
constexpr u32 kWorkQueue = 0x0116B580 + 128;    // its queue object: vfunc +16 () -> handle, +20 (queue, handle) (name?)
constexpr u32 kVtblApeRoot = 0x00B242B8;
constexpr u32 kObjSerial = 0x01164CD0;          // u32: the last object serial handed out (name?)
constexpr u32 kO57Slots = 32, kO57SlotSize = 192, kO57First = 32;
constexpr u32 kFltMax = 0x7F7FFFFFu, kFltMinusMax = 0xFF7FFFFFu;

// The position through o +80 (row 3 added) and the tangent through its
// rotation rows, both stored aligned (WorldObj_SplinePointTangent*).
void TransformPosTan(PPUContext& ctx, u32 o, u32 pos, u32 tan) {
    const u32 m = o + obj12::kObjOwnMatrix;
    VStore(pos, TransformPoint(m, pos));
    Vec4 t = VMadd(VLoad(m), Splat(tan), VZero());
    t = VMadd(VLoad(m + 16), Splat(tan + 4), t);
    t = VMadd(VLoad(m + 32), Splat(tan + 8), t);
    VStore(tan, t);
    ctx.gpr[4] = m;
    ctx.gpr[5] = 4;
    ctx.gpr[6] = m + 16;
    ctx.gpr[7] = 8;
    ctx.gpr[8] = m + 32;
    ctx.gpr[9] = m + 48;
}

}  // namespace

// u32 WorldObj_SplinePointTangent(WorldObj* o, -, f32 pos[4], f32 tan[4], u32 seg, bool back; f1 t)
// Spline_PointTangent on the definition's spline (r4 passed through),
// then both into world space (WorldObj_SplinePointAt); r3 = its segment (name?).
DECOMP(005A8284, WorldObj_SplinePointTangent) {
    const u32 o = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 2), tan = arg<u32>(ctx, 3);
    callf(ctx, Spline_PointTangentFn, {load<u32>(o + kObjSplineDef) + kSdSpline, ctx.gpr[4], pos, tan, ctx.gpr[7], ctx.gpr[8] & 0xFF}, {ctx.fpr[1]});
    const u64 r = ctx.gpr[3];
    TransformPosTan(ctx, o, pos, tan);
    ctx.gpr[3] = r;
}

// u32 WorldObj_SplinePointTangentAt(WorldObj* o, -, f32 pos[4], f32 tan[4], bool back; f1 t)
// The same with Spline_PointTangentAt (name?).
DECOMP(005A80D8, WorldObj_SplinePointTangentAt) {
    const u32 o = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 2), tan = arg<u32>(ctx, 3);
    call(ctx, Spline_PointTangentAtFn, {load<u32>(o + kObjSplineDef) + kSdSpline, ctx.gpr[4], pos, tan, ctx.gpr[7] & 0xFF});  // f1 passed through
    const u64 r = ctx.gpr[3];
    TransformPosTan(ctx, o, pos, tan);
    ctx.gpr[3] = r;
}

// void Obj577200_Construct(Obj* o)
// The ApeRoot part inline (refs 1, kind 18, the next serial in +8 bits
// 11-31, bits 6-10 clear, +12 / +16..+20 0), vtable 0x00B245F8, then 32
// 192-byte slots from +32: u8 {255, 0, 0}, s32 -1, f32[3] FLT_MAX at +16,
// f32[3] -FLT_MAX at +32, +144 u8 0, +176 / +180 0 (name?).
DECOMP(0057711C, Obj577200_Construct) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w4 = (load<u32>(o + 4) & 0x0000FFFFu) | 0x00010000u;
    store<u16>(o + 4, 1);
    store<u32>(o + 4, (w4 & ~0xFC00u) | (18u << 10));
    const u32 serial = load<u32>(kObjSerial) + 1;
    store<u32>(o, kVtblApeRoot);
    store<u32>(o + 12, 0);
    store<u32>(kObjSerial, serial);
    const u32 w8 = load<u32>(o + 8);
    store<u8>(o + 16, 0);
    store<u32>(o, kVtbl577200);
    store<u8>(o + 17, 0);
    store<u8>(o + 18, 0);
    store<u32>(o + 8, (w8 & 0x3Fu) | (serial << 11));
    store<u32>(o + 20, 0);
    u32 e = 0;
    for (u32 i = 0; i < kO57Slots; ++i) {
        e = o + kO57First + kO57SlotSize * i;
        store<u8>(e, 255);
        store<u8>(e + 1, 0);
        store<u8>(e + 2, 0);
        store<s32>(e + 4, -1);
        for (u32 k = 16; k < 28; k += 4) store<u32>(e + k, kFltMax);
        for (u32 k = 32; k < 44; k += 4) store<u32>(e + k, kFltMinusMax);
        store<u32>(e + 176, 0);
        store<u32>(e + 180, 0);
        store<u8>(e + 144, 0);
    }
    ctx.gpr[3] = kFltMinusMax;
    ctx.gpr[4] = kFltMax;
    ctx.gpr[5] = ~0ull;
    ctx.gpr[6] = 255;
    ctx.gpr[7] = o + kO57SlotSize * kO57Slots;
    ctx.gpr[8] = 0;
    ctx.gpr[9] = e;
}

// Obj* PairBuffer_Flush(PairBuffer* b, bool reset)
// Calls each of the +24 {OPD*, arg} entries from +4 as fn(arg); with reset
// the cursor (+12) back to +4, the top (+16) to +8, used and count 0, +28
// up by one and +32 bits 0x80 / 0x40 cleared. Returns +4 (name?).
DECOMP(00565228, PairBuffer_Flush) {
    const u32 b = arg<u32>(ctx, 0);
    const u32 reset = static_cast<u32>(ctx.gpr[4]) & 0xFF;
    u32 p = load<u32>(b + 4);
    for (u32 n = load<u32>(b + kPbCount); n-- != 0; p += 8) {
        const u32 opd = load<u32>(p);
        call_ptr(ctx, opd, {load<u32>(p + 4), opd, load<u32>(opd)});
    }
    const u32 base = load<u32>(b + 4);
    ctx.gpr[3] = base;
    ctx.gpr[4] = reset;
    if (reset == 0) return;
    const u32 gen = load<u32>(b + 28) + 1;
    const u8 flags = load<u8>(b + 32) & 0x3F;
    const u32 top = load<u32>(b + 8);
    store<u32>(b + kPbCursor, base);
    store<u32>(b + kPbTop, top);
    store<u32>(b + kPbCount, 0);
    store<u32>(b + kPbCheck, 0);
    store<u32>(b + 28, gen);
    store<u8>(b + 32, flags);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = gen;
    ctx.gpr[6] = flags;
    ctx.gpr[7] = top;
}

// void DrawRecordList_Flush(Entry* first, u32* stride, u32* count, u32** counter)
// Under the work mutex (100 ms): a handle from the work queue (vfunc
// +16), f_0056E4C0({queue, handle}, first, stride, count, counter), then
// the queue's vfunc +20 (queue, handle) with the pair as it comes back (name?).
DECOMP(0056E610, DrawRecordList_Flush) {
    const u32 first = arg<u32>(ctx, 0), stride = arg<u32>(ctx, 1), count = arg<u32>(ctx, 2), counter = arg<u32>(ctx, 3);
    Stack frame(ctx, 160);
    const u32 sp = frame.addr();
    call(ctx, imp::sys_lwmutex_lock, {kWorkMutex, kSysutilLockTimeout});
    store<u32>(sp + 112, kWorkQueue);
    const u32 opd = load<u32>(load<u32>(kWorkQueue) + 16);
    const u32 h = static_cast<u32>(call_ptr(ctx, opd, {kWorkQueue, opd, load<u32>(opd)}));
    store<u32>(sp + 116, h);
    call(ctx, WorkQueue_Fn56E4C0, {sp + 112, first, stride, count, counter});
    const u32 q = load<u32>(sp + 112), h2 = load<u32>(sp + 116);
    const u32 opd2 = load<u32>(load<u32>(q) + 20);
    call_ptr(ctx, opd2, {q, h2, opd2, load<u32>(opd2)});
    call(ctx, imp::sys_lwmutex_unlock, {kWorkMutex});
}
