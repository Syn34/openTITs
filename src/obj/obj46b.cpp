// Game code, part 46 (continued): more of the small-function sweep, 99
// instructions and up. Simulator-checked like obj46.cpp.
#include "obj46.h"

#include "../uielem/class.h"

using namespace sly4;
using namespace sly4::obj46;

// ---------------------------------------------------------------- sweep: 99 instructions
// The deleting destructors of three element classes whose own destructor
// is the base one: vtable 0x00B22710, the inlined base body
// (uielem::ElemDestructBody: children, unlink, focus slots), then
// SmallFree(e, size). e 0 -> nothing (r3 = 0).
static void ElemDeleteSized46(PPUContext& ctx, u32 size) {
    const u32 e = arg<u32>(ctx, 0);
    if (e == 0) return;
    store<u32>(e, sly4::uielem::kVtblBase);
    sly4::uielem::ElemDestructBody(ctx, e);
    call(ctx, 0x00552420u, {e, size});
}

// [vtable 0x00B22920 +4]: 176 bytes.
DECOMP(00796114, UiElem920_Delete) { ElemDeleteSized46(ctx, 176); }
// [vtable 0x00B22D08 +4]: 192 bytes.
DECOMP(007966F8, UiElemD08_Delete) { ElemDeleteSized46(ctx, 192); }
// [vtable 0x00B22E98 +4]: 192 bytes.
DECOMP(007968F4, UiElemE98_Delete) { ElemDeleteSized46(ctx, 192); }

// ---------------------------------------------------------------- sweep: 101 instructions
// The 28-byte flag records of 0x00073910 / 0x00073D18 (Rec* f(-,
// XmlCursor* c)): Mem_Alloc(28, 0, 16) (0 -> r3 = 0),
// Obj_InitVt_B21C98(p, c), +12 / +16 = 0, bytes +20 / +21 = 1, +22 / +23 =
// 0 (and +24 when init24), the vtable; the hashes h12 / h16 into +12 / +16,
// the bools 0x7A281983 into +20, 0x6557DBDD into +21 (default: +20 == 0,
// stored first), 0x03A6C100 into +22, h23 / h24 into +23 / +24 (defaults:
// the current bytes). r3 = p.
static void FlagRec46(PPUContext& ctx, u32 vt, u32 h12, u32 h16, u32 h23, u32 h24, bool init24) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, cur});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u8>(p + 20, 1);
    store<u32>(p, vt);
    store<u8>(p + 21, 1);
    store<u8>(p + 22, 0);
    store<u8>(p + 23, 0);
    if (init24) store<u8>(p + 24, 0);
    call(ctx, 0x004FA14Cu, {cur, h12, p + 12, 0});
    call(ctx, 0x004FA14Cu, {cur, h16, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004F986Cu, {cur, 0x7A281983u, p + 20, load<u8>(p + 20)});
    const u32 d21 = load<u8>(p + 20) == 0 ? 1u : 0u;
    store<u8>(p + 21, static_cast<u8>(d21));
    call(ctx, 0x004F986Cu, {cur, 0x6557DBDDu, p + 21, d21});
    call(ctx, 0x004F986Cu, {cur, 0x03A6C100u, p + 22, load<u8>(p + 22)});
    call(ctx, 0x004F986Cu, {cur, h23, p + 23, load<u8>(p + 23)});
    call(ctx, 0x004F986Cu, {cur, h24, p + 24, load<u8>(p + 24)});
    ret(ctx, p);
}

// [vtable 0x00B0F590]
DECOMP(00073910, XmlRec_New_B0F590_00073910) { FlagRec46(ctx, 0x00B0F590u, 0xB2E59948u, 0x6CA9CCBFu, 0xA92B6296u, 0x04A4252Fu, true); }
// [vtable 0x00B0F5B8]: byte +24 is not initialized (the read default is
// whatever the allocation held).
DECOMP(00073D18, XmlRec_New_B0F5B8_00073D18) { FlagRec46(ctx, 0x00B0F5B8u, 0xE89DF217u, 0xE4508D90u, 0x04A4252Fu, 0xA92B6296u, false); }

// Rec* f(-, XmlCursor* c): Mem_Alloc(28, 0, 16) (0 -> r3 = 0),
// Obj_InitVt_B21C98(p, c), +12..+24 = 0, vtable 0x00B0F950; the hashes
// 0x8A93973F / 0x7A30D7E3 into +12 / +16; when the hash 0x0020F76C reads
// with result 0: its value in the 31-pair table 0x00AC4C28 into +20, and
// +20 still 0 -> String_Format(0x00AC4D30, Hash_ToString(hash)); then the
// hash 0x7D385CF6 into +24. r3 = p.
DECOMP(0007AF38, XmlRec_New_B0F950_0007AF38) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, cur});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B0F950u);
    store<u32>(p + 24, 0);
    call(ctx, 0x004FA14Cu, {cur, 0x8A93973Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {cur, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {cur, 0x0020F76Cu, s, 0})) == 0) {
        const u32 h = load<u32>(s);
        u32 v = load<u32>(p + 20);
        for (u32 i = 0; i < 31; ++i)
            if (load<u32>(0x00AC4C28u + 8 * i) == h) {
                v = load<u32>(0x00AC4C2Cu + 8 * i);
                store<u32>(p + 20, v);
                break;
            }
        if (v == 0) {
            const u32 str = static_cast<u32>(call(ctx, 0x0056A6E0u, {h}));
            call(ctx, 0x005534D0u, {0x00AC4D30u, str});
        }
    }
    call(ctx, 0x004FA14Cu, {cur, 0x7D385CF6u, p + 24, 0});
    ret(ctx, p);
}
