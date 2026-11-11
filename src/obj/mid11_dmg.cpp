// Game code, medium sweep part 11: damage / collision def readers and
// factories derived from DamageColDef_ReadAD1F8 (record flag 0x80000; the
// factories are called by 000CA038, each reader only by its factory).
#include "mid11.h"

using namespace sly4;
using namespace sly4::mid11;

namespace {

constexpr u32 kEnumBEA40 = 0x00F64A10;  // u32[2]: the hashes of the 0xC03F596F enum

// The reader head: DamageColDef_ReadAD1F8(a0, c, x, tmpl, o), then the
// defaults record (DmgCtor with x->12..20, r7 1, r8 left over from that
// call; Dmg80000 with flags).
void DmgReadHead(PPUContext& ctx, u32 a0, u32 c, u32 x, u32 tmpl, u32 o, u32 rec, u32 pool, u32 flags) {
    call(ctx, m11fn::DamageColDef_ReadAD1F8, {a0, c, x, tmpl, o});
    DmgCtor(ctx, rec, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), 1, ctx.gpr[8], pool);
    Dmg80000(ctx, rec, flags);
}

// The factory head: the defaults record (DmgCtor with x->12, name
// 0x00AD13B8, r7 = the pool, r8 1; Dmg80000 with flags).
void DmgNewHead(PPUContext& ctx, u32 x, u32 rec, u32 pool, u32 flags) {
    DmgCtor(ctx, rec, load<u32>(x + 12), kStrAD13B8, kStrAD13B8, pool, 1, pool);
    Dmg80000(ctx, rec, flags);
}

// Record extensions (+320..).
void ExtBEA40(u32 rec) {
    store<u32>(rec + 320, 0);
    store<u32>(rec + 324, 0xBF800000u);
    store<u8>(rec + 328, 0);
    store<u8>(rec + 329, 0);
}
void ExtC02A8(u32 rec) {
    store<u32>(rec + 320, 0xFFFFFFFFu);
    store<u32>(rec + 324, 0);
    store<u32>(rec + 328, 0);
    store<u32>(rec + 332, (load<u32>(rec + 332) & 0x7FFFFFFFu) | 0x40000000u);
}
void ExtCFF58(u32 rec) {
    store<u32>(rec + 320, 0);
    store<u32>(rec + 324, 0);
}
void ExtD3B80(u32 rec) {
    store<u32>(rec + 320, 0);
    store<u32>(rec + 324, 0);
    store<u32>(rec + 328, 0);
}

}  // namespace

// void DamageColDef_ReadBEA40(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_ReadAD1F8, then the defaults record (pool 0x001BEA28, flags
// 0x180000; +320 0, f32 +324 -1, bytes +328 / +329 0) with +320..+329 =
// tmpl's +400..+409 when tmpl has flag 0x100000. Reads an enum hash
// (0xC03F596F) into o+320: the record's +320 when missing, else its index
// in the two hashes at 0x00F64A10 (0 when not listed); a float (0xA6710C18)
// into o+324, bools (0xD28465E5, 0xF0169515) into bytes o+328 / +329; then
// destroys the record's vector +124.
DECOMP(001BEA40, DamageColDef_ReadBEA40) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 416);
    const u32 rec = frame.addr();
    DmgReadHead(ctx, a0, c, x, tmpl, o, rec, 0x001BEA28, 0x180000u);
    ExtBEA40(rec);
    if (DmgTmpl(ctx, a0, tmpl, 0x100000u)) {
        store<u32>(rec + 320, load<u32>(tmpl + 400));
        CopyW(rec + 324, tmpl + 404);
        store<u8>(rec + 328, load<u8>(tmpl + 408));
        store<u8>(rec + 329, load<u8>(tmpl + 409));
    }
    const u32 h = rec + 336;  // the original's sp + 704
    store<u32>(h, 0);
    u32 v;
    if (static_cast<u32>(call(ctx, xfn::XmlCursor_ReadHash, {c, 0xC03F596Fu, h, 0})) != 0) {
        v = load<u32>(rec + 320);
    } else {
        v = 0;
        const u32 want = load<u32>(h);
        for (u32 i = 0; i < 2; ++i) {
            if (load<u32>(kEnumBEA40 + 4 * i) == want) {
                v = i;
                break;
            }
        }
    }
    store<u32>(o + 320, v);
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xA6710C18u, o + 324}, {LoadF(rec + 324)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xD28465E5u, o + 328, load<u8>(rec + 328)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xF0169515u, o + 329, load<u8>(rec + 329)});
    DmgDestroy(ctx, rec);
}

// Def* DamageColDef_NewBED98(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (pool 0x001BED84, ExtBEA40) read by
// DamageColDef_ReadBEA40, then a 416-byte def built by 0x001BE8CC(p, rec).
DECOMP(001BED98, DamageColDef_NewBED98) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 400);
    const u32 rec = frame.addr();
    DmgNewHead(ctx, x, rec, 0x001BED84, 0x180000u);
    ExtBEA40(rec);
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001BEA40, 416, 0x001BE8CC));
}

// void DamageColDef_ReadC02A8(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_ReadAD1F8, then the defaults record (pool 0x001C0290, flags
// 0x280000; +320 -1, +324 / +328 0, bit 0 of +332 clear / bit 1 set) with
// +320..+328 and bits 0 / 1 of +332 from tmpl's +400..+412 when tmpl has
// flag 0x200000. Reads a hash (0x2DF2E2B5) into o+324, a float (0x08C08CA8)
// into o+328, two bools (0xD28465E5, 0xD02B978A) into bits 0 / 1 of o+332
// (through a stack byte) and an s32 (0xCBC57F7D) into o+320 (decremented,
// not below 0, when found); then destroys the record's vector +124.
DECOMP(001C02A8, DamageColDef_ReadC02A8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 416);
    const u32 rec = frame.addr();
    DmgReadHead(ctx, a0, c, x, tmpl, o, rec, 0x001C0290, 0x280000u);
    ExtC02A8(rec);
    if (DmgTmpl(ctx, a0, tmpl, 0x200000u)) {
        store<u32>(rec + 320, load<u32>(tmpl + 400));
        store<u32>(rec + 324, load<u32>(tmpl + 404));
        CopyW(rec + 328, tmpl + 408);
        store<u32>(rec + 332, (load<u32>(rec + 332) & 0x3FFFFFFFu) | (load<u32>(tmpl + 412) & 0xC0000000u));
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x2DF2E2B5u, o + 324, load<u32>(rec + 324)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x08C08CA8u, o + 328}, {LoadF(rec + 328)});
    const u32 b = rec + 336;  // the original's sp + 704
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xD28465E5u, b, load<u32>(rec + 332) >> 31});
    store<u32>(o + 332, PutBit(load<u32>(o + 332), 0, load<u8>(b)));
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xD02B978Au, b, (load<u32>(rec + 332) >> 30) & 1u});
    store<u32>(o + 332, PutBit(load<u32>(o + 332), 1, load<u8>(b)));
    const u64 def320 = static_cast<u64>(static_cast<s64>(load<s32>(rec + 320)));
    if (static_cast<u32>(call(ctx, xfn::XmlCursor_ReadS32, {c, 0xCBC57F7Du, o + 320, def320})) == 0) {
        const s32 v = load<s32>(o + 320) - 1;
        store<s32>(o + 320, v < 0 ? 0 : v);
        ctx.gpr[3] = static_cast<u64>(static_cast<s64>(v));  // the original's r3 when the record has no vector
    }
    DmgDestroy(ctx, rec);
}

// Def* DamageColDef_NewC0658(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (pool 0x001C0644, ExtC02A8) read by
// DamageColDef_ReadC02A8, then a 416-byte def built by 0x001C011C(p, rec).
DECOMP(001C0658, DamageColDef_NewC0658) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 400);
    const u32 rec = frame.addr();
    DmgNewHead(ctx, x, rec, 0x001C0644, 0x280000u);
    ExtC02A8(rec);
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001C02A8, 416, 0x001C011C));
}

// void DamageColDef_ReadCFF58(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_ReadAD1F8, then the defaults record (pool 0x001CFF40, flags
// 0x08080000; +320 / +324 0) with +320 / +324 = tmpl's +400 / +404 when tmpl
// has flag 0x08000000. Reads two hashes (0x503616D9, 0x5B85769D) into
// o+320 / +324, then destroys the record's vector +124.
DECOMP(001CFF58, DamageColDef_ReadCFF58) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 416);
    const u32 rec = frame.addr();
    DmgReadHead(ctx, a0, c, x, tmpl, o, rec, 0x001CFF40, 0x08080000u);
    ExtCFF58(rec);
    if (DmgTmpl(ctx, a0, tmpl, 0x08000000u)) {
        store<u32>(rec + 320, load<u32>(tmpl + 400));
        store<u32>(rec + 324, load<u32>(tmpl + 404));
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x503616D9u, o + 320, load<u32>(rec + 320)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x5B85769Du, o + 324, load<u32>(rec + 324)});
    DmgDestroy(ctx, rec);
}

// Def* DamageColDef_NewD0208(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (pool 0x001D01F4, ExtCFF58) read by
// DamageColDef_ReadCFF58, then a 416-byte def built by 0x001CFE00(p, rec).
DECOMP(001D0208, DamageColDef_NewD0208) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 400);
    const u32 rec = frame.addr();
    DmgNewHead(ctx, x, rec, 0x001D01F4, 0x08080000u);
    ExtCFF58(rec);
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001CFF58, 416, 0x001CFE00));
}

// void DamageColDef_ReadD3B80(a0, XmlCursor* c, x, const Def* tmpl, Def* o)
// DamageColDef_ReadAD1F8, then the defaults record (pool 0x001D3B6C, flags
// 0x04080000; +320..+328 0) with +320..+328 = tmpl's +400..+408 when tmpl
// has flag 0x04000000. Reads a hash (0x53FFAFC2) into o+320, a float
// (0x667AB4F5) into o+324, a hash (0xCD839910) into o+328, then destroys the
// record's vector +124.
DECOMP(001D3B80, DamageColDef_ReadD3B80) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    Stack frame(ctx, 416);
    const u32 rec = frame.addr();
    DmgReadHead(ctx, a0, c, x, tmpl, o, rec, 0x001D3B6C, 0x04080000u);
    ExtD3B80(rec);
    if (DmgTmpl(ctx, a0, tmpl, 0x04000000u)) {
        store<u32>(rec + 320, load<u32>(tmpl + 400));
        CopyW(rec + 324, tmpl + 404);
        store<u32>(rec + 328, load<u32>(tmpl + 408));
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x53FFAFC2u, o + 320, load<u32>(rec + 320)});
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x667AB4F5u, o + 324}, {LoadF(rec + 324)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xCD839910u, o + 328, load<u32>(rec + 328)});
    DmgDestroy(ctx, rec);
}

// Def* DamageColDef_NewD3E58(a0, XmlCursor* c, x, const Def* tmpl)
// The defaults record (pool 0x001D3E40, ExtD3B80) read by
// DamageColDef_ReadD3B80, then a 416-byte def built by 0x001D3A24(p, rec).
DECOMP(001D3E58, DamageColDef_NewD3E58) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    Stack frame(ctx, 400);
    const u32 rec = frame.addr();
    DmgNewHead(ctx, x, rec, 0x001D3E40, 0x04080000u);
    ExtD3B80(rec);
    ret(ctx, DmgNew(ctx, a0, c, x, tmpl, rec, 0x001D3B80, 416, 0x001D3A24));
}
