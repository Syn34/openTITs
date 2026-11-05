// World objects, part 19: template instances found by the project-wide
// survey (0x00010000-0x005E0000 and 0x00730000-0x007A0000) of functions
// whose normalised disassembly differs only in constants. Each instance is
// one DECOMP line calling the shared body in obj19.h with the constants
// scanned from its own disassembly; every one was checked against the
// original in the PPC simulator. Names carry the class vtable
// (Vt<vtable>_Dtor / _Delete) where one is stored.
#include "obj19.h"

using namespace sly4;
using namespace sly4::obj19;

// ---------------------------------------------------------------- empty functions
// empty
DECOMP(000133F8, Nop_000133F8) { static_cast<void>(ctx); }
// empty
DECOMP(00014D08, Nop_00014D08) { static_cast<void>(ctx); }
// empty
DECOMP(0001ECC4, Nop_0001ECC4) { static_cast<void>(ctx); }
// empty
DECOMP(00024660, Nop_00024660) { static_cast<void>(ctx); }
// empty
DECOMP(0002E4F8, Nop_0002E4F8) { static_cast<void>(ctx); }
// empty
DECOMP(00030888, Nop_00030888) { static_cast<void>(ctx); }
// empty
DECOMP(0004E9D4, Nop_0004E9D4) { static_cast<void>(ctx); }
// empty
DECOMP(0005F9DC, Nop_0005F9DC) { static_cast<void>(ctx); }
// empty
DECOMP(0005F9F8, Nop_0005F9F8) { static_cast<void>(ctx); }
// empty
DECOMP(00064D30, Nop_00064D30) { static_cast<void>(ctx); }
// empty
DECOMP(0006528C, Nop_0006528C) { static_cast<void>(ctx); }
// empty
DECOMP(000858EC, Nop_000858EC) { static_cast<void>(ctx); }
// empty
DECOMP(000D8FC0, Nop_000D8FC0) { static_cast<void>(ctx); }
// empty
DECOMP(000E969C, Nop_000E969C) { static_cast<void>(ctx); }
// empty
DECOMP(001066FC, Nop_001066FC) { static_cast<void>(ctx); }
// empty
DECOMP(00116434, Nop_00116434) { static_cast<void>(ctx); }
// empty
DECOMP(0016CD70, Nop_0016CD70) { static_cast<void>(ctx); }
// empty
DECOMP(0016CD74, Nop_0016CD74) { static_cast<void>(ctx); }
// empty
DECOMP(0016D6B0, Nop_0016D6B0) { static_cast<void>(ctx); }
// empty
DECOMP(0016EC48, Nop_0016EC48) { static_cast<void>(ctx); }
// empty
DECOMP(001728E8, Nop_001728E8) { static_cast<void>(ctx); }
// empty
DECOMP(001728EC, Nop_001728EC) { static_cast<void>(ctx); }
// empty
DECOMP(00172B70, Nop_00172B70) { static_cast<void>(ctx); }
// empty
DECOMP(00172B74, Nop_00172B74) { static_cast<void>(ctx); }
// empty
DECOMP(0018E0BC, Nop_0018E0BC) { static_cast<void>(ctx); }
// empty
DECOMP(0018E380, Nop_0018E380) { static_cast<void>(ctx); }
// empty
DECOMP(00196768, Nop_00196768) { static_cast<void>(ctx); }
// empty
DECOMP(0019676C, Nop_0019676C) { static_cast<void>(ctx); }
// empty
DECOMP(001F4088, Nop_001F4088) { static_cast<void>(ctx); }
// empty
DECOMP(0020FA68, Nop_0020FA68) { static_cast<void>(ctx); }
// empty
DECOMP(00236310, Nop_00236310) { static_cast<void>(ctx); }
// empty
DECOMP(00264D6C, Nop_00264D6C) { static_cast<void>(ctx); }
// empty
DECOMP(0026EFCC, Nop_0026EFCC) { static_cast<void>(ctx); }
// empty
DECOMP(0026EFD0, Nop_0026EFD0) { static_cast<void>(ctx); }
// empty
DECOMP(00280A08, Nop_00280A08) { static_cast<void>(ctx); }
// empty
DECOMP(0028386C, Nop_0028386C) { static_cast<void>(ctx); }
// empty
DECOMP(00288970, Nop_00288970) { static_cast<void>(ctx); }
// empty
DECOMP(00293934, Nop_00293934) { static_cast<void>(ctx); }
// empty
DECOMP(00297E38, Nop_00297E38) { static_cast<void>(ctx); }
// empty
DECOMP(0029FE58, Nop_0029FE58) { static_cast<void>(ctx); }
// empty
DECOMP(002AD4FC, Nop_002AD4FC) { static_cast<void>(ctx); }
// empty
DECOMP(002AEC90, Nop_002AEC90) { static_cast<void>(ctx); }
// empty
DECOMP(002AEC94, Nop_002AEC94) { static_cast<void>(ctx); }
// empty
DECOMP(002AEE50, Nop_002AEE50) { static_cast<void>(ctx); }
// empty
DECOMP(002AEE54, Nop_002AEE54) { static_cast<void>(ctx); }
// empty
DECOMP(002AF0C4, Nop_002AF0C4) { static_cast<void>(ctx); }
// empty
DECOMP(002AF0C8, Nop_002AF0C8) { static_cast<void>(ctx); }
// empty
DECOMP(002AF0CC, Nop_002AF0CC) { static_cast<void>(ctx); }
// empty
DECOMP(002AF0D0, Nop_002AF0D0) { static_cast<void>(ctx); }
// empty
DECOMP(002AF5F8, Nop_002AF5F8) { static_cast<void>(ctx); }
// empty
DECOMP(002B0458, Nop_002B0458) { static_cast<void>(ctx); }
// empty
DECOMP(002B1A10, Nop_002B1A10) { static_cast<void>(ctx); }
// empty
DECOMP(002B2184, Nop_002B2184) { static_cast<void>(ctx); }
// empty
DECOMP(002B425C, Nop_002B425C) { static_cast<void>(ctx); }
// empty
DECOMP(002B4260, Nop_002B4260) { static_cast<void>(ctx); }
// empty
DECOMP(002B6724, Nop_002B6724) { static_cast<void>(ctx); }
// empty
DECOMP(002B6728, Nop_002B6728) { static_cast<void>(ctx); }
// empty
DECOMP(002BC124, Nop_002BC124) { static_cast<void>(ctx); }
// empty
DECOMP(002BC1D4, Nop_002BC1D4) { static_cast<void>(ctx); }
// empty
DECOMP(002DD890, Nop_002DD890) { static_cast<void>(ctx); }
// empty
DECOMP(002EF4DC, Nop_002EF4DC) { static_cast<void>(ctx); }
// empty
DECOMP(002FD7B4, Nop_002FD7B4) { static_cast<void>(ctx); }
// empty
DECOMP(002FE630, Nop_002FE630) { static_cast<void>(ctx); }
// empty
DECOMP(00302CC4, Nop_00302CC4) { static_cast<void>(ctx); }
// empty
DECOMP(00302CC8, Nop_00302CC8) { static_cast<void>(ctx); }
// empty
DECOMP(00302CCC, Nop_00302CCC) { static_cast<void>(ctx); }
// empty
DECOMP(00302CD0, Nop_00302CD0) { static_cast<void>(ctx); }
// empty
DECOMP(00302CD4, Nop_00302CD4) { static_cast<void>(ctx); }
// empty
DECOMP(00302CD8, Nop_00302CD8) { static_cast<void>(ctx); }
// empty
DECOMP(00302CDC, Nop_00302CDC) { static_cast<void>(ctx); }
// empty
DECOMP(00311570, Nop_00311570) { static_cast<void>(ctx); }
// empty
DECOMP(00311574, Nop_00311574) { static_cast<void>(ctx); }
// empty
DECOMP(00311578, Nop_00311578) { static_cast<void>(ctx); }
// empty
DECOMP(0031157C, Nop_0031157C) { static_cast<void>(ctx); }
// empty
DECOMP(00311580, Nop_00311580) { static_cast<void>(ctx); }
// empty
DECOMP(00311584, Nop_00311584) { static_cast<void>(ctx); }
// empty
DECOMP(00311588, Nop_00311588) { static_cast<void>(ctx); }
// empty
DECOMP(003124E8, Nop_003124E8) { static_cast<void>(ctx); }
// empty
DECOMP(003124EC, Nop_003124EC) { static_cast<void>(ctx); }
// empty
DECOMP(003124F0, Nop_003124F0) { static_cast<void>(ctx); }
// empty
DECOMP(003124F4, Nop_003124F4) { static_cast<void>(ctx); }
// empty
DECOMP(003124F8, Nop_003124F8) { static_cast<void>(ctx); }
// empty
DECOMP(00315738, Nop_00315738) { static_cast<void>(ctx); }
// empty
DECOMP(003157B0, Nop_003157B0) { static_cast<void>(ctx); }
// empty
DECOMP(00316614, Nop_00316614) { static_cast<void>(ctx); }
// empty
DECOMP(00318BD4, Nop_00318BD4) { static_cast<void>(ctx); }
// empty
DECOMP(0031A328, Nop_0031A328) { static_cast<void>(ctx); }
// empty
DECOMP(0031A7E4, Nop_0031A7E4) { static_cast<void>(ctx); }
// empty
DECOMP(00365A50, Nop_00365A50) { static_cast<void>(ctx); }
// empty
DECOMP(0036CA70, Nop_0036CA70) { static_cast<void>(ctx); }
// empty
DECOMP(0036FAC8, Nop_0036FAC8) { static_cast<void>(ctx); }
// empty
DECOMP(00370934, Nop_00370934) { static_cast<void>(ctx); }
// empty
DECOMP(00390BA8, Nop_00390BA8) { static_cast<void>(ctx); }
// empty
DECOMP(00390C18, Nop_00390C18) { static_cast<void>(ctx); }
// empty
DECOMP(00390DC4, Nop_00390DC4) { static_cast<void>(ctx); }
// empty
DECOMP(00398D54, Nop_00398D54) { static_cast<void>(ctx); }
// empty
DECOMP(003A037C, Nop_003A037C) { static_cast<void>(ctx); }
// empty
DECOMP(003AAF7C, Nop_003AAF7C) { static_cast<void>(ctx); }
// empty
DECOMP(003B04F4, Nop_003B04F4) { static_cast<void>(ctx); }
// empty
DECOMP(003DA550, Nop_003DA550) { static_cast<void>(ctx); }
// empty
DECOMP(003DC91C, Nop_003DC91C) { static_cast<void>(ctx); }
// empty
DECOMP(003DD234, Nop_003DD234) { static_cast<void>(ctx); }
// empty
DECOMP(003EFE34, Nop_003EFE34) { static_cast<void>(ctx); }
// empty
DECOMP(003FD8A8, Nop_003FD8A8) { static_cast<void>(ctx); }
// empty
DECOMP(00404438, Nop_00404438) { static_cast<void>(ctx); }
// empty
DECOMP(00405490, Nop_00405490) { static_cast<void>(ctx); }
// empty
DECOMP(00409ED4, Nop_00409ED4) { static_cast<void>(ctx); }
// empty
DECOMP(0040AB94, Nop_0040AB94) { static_cast<void>(ctx); }
// empty
DECOMP(00418E90, Nop_00418E90) { static_cast<void>(ctx); }
// empty
DECOMP(0041A09C, Nop_0041A09C) { static_cast<void>(ctx); }
// empty
DECOMP(0041A0A0, Nop_0041A0A0) { static_cast<void>(ctx); }
// empty
DECOMP(0041A350, Nop_0041A350) { static_cast<void>(ctx); }
// empty
DECOMP(0041E690, Nop_0041E690) { static_cast<void>(ctx); }
// empty
DECOMP(0041F1F8, Nop_0041F1F8) { static_cast<void>(ctx); }
// empty
DECOMP(00421468, Nop_00421468) { static_cast<void>(ctx); }
// empty
DECOMP(0042AD9C, Nop_0042AD9C) { static_cast<void>(ctx); }
// empty
DECOMP(0044BCDC, Nop_0044BCDC) { static_cast<void>(ctx); }
// empty
DECOMP(0044D9B8, Nop_0044D9B8) { static_cast<void>(ctx); }
// empty
DECOMP(0044D9BC, Nop_0044D9BC) { static_cast<void>(ctx); }
// empty
DECOMP(0044D9DC, Nop_0044D9DC) { static_cast<void>(ctx); }
// empty
DECOMP(004902F0, Nop_004902F0) { static_cast<void>(ctx); }
// empty
DECOMP(0049B4DC, Nop_0049B4DC) { static_cast<void>(ctx); }
// empty
DECOMP(004A8AC0, Nop_004A8AC0) { static_cast<void>(ctx); }
// empty
DECOMP(004A8AC4, Nop_004A8AC4) { static_cast<void>(ctx); }
// empty
DECOMP(004B25C0, Nop_004B25C0) { static_cast<void>(ctx); }
// empty
DECOMP(004E9414, Nop_004E9414) { static_cast<void>(ctx); }

// ---------------------------------------------------------------- destructors: the vtable, then the base destructor
// *this = vtable 0x00B0DE10, then 0x0011BCF4(this)
DECOMP(0001BF4C, VtB0DE10_Dtor) { TDtor(ctx, 0x00B0DE10, 0x0011BCF4); }
// *this = vtable 0x00B0ED68, then 0x00455A94(this)
DECOMP(00061B30, VtB0ED68_Dtor) { TDtor(ctx, 0x00B0ED68, 0x00455A94); }
// *this = vtable 0x00B0F068, then 0x004BB41C(this)
DECOMP(0006B738, VtB0F068_Dtor) { TDtor(ctx, 0x00B0F068, 0x004BB41C); }
// *this = vtable 0x00B0F0B8, then 0x004BB41C(this)
DECOMP(0006BD8C, VtB0F0B8_Dtor) { TDtor(ctx, 0x00B0F0B8, 0x004BB41C); }
// *this = vtable 0x00B0F0E0, then 0x004BB41C(this)
DECOMP(0006C130, VtB0F0E0_Dtor) { TDtor(ctx, 0x00B0F0E0, 0x004BB41C); }
// *this = vtable 0x00B0F108, then 0x004BB41C(this)
DECOMP(0006C610, VtB0F108_Dtor) { TDtor(ctx, 0x00B0F108, 0x004BB41C); }
// *this = vtable 0x00B0F130, then 0x004BB41C(this)
DECOMP(0006CA78, VtB0F130_Dtor) { TDtor(ctx, 0x00B0F130, 0x004BB41C); }
// *this = vtable 0x00B0F158, then 0x004BB41C(this)
DECOMP(0006CF7C, VtB0F158_Dtor) { TDtor(ctx, 0x00B0F158, 0x004BB41C); }
// *this = vtable 0x00B0F1A8, then 0x004BB41C(this)
DECOMP(0006E0C4, VtB0F1A8_Dtor) { TDtor(ctx, 0x00B0F1A8, 0x004BB41C); }
// *this = vtable 0x00B0F298, then 0x004BB41C(this)
DECOMP(0006F300, VtB0F298_Dtor) { TDtor(ctx, 0x00B0F298, 0x004BB41C); }
// *this = vtable 0x00B0F2C0, then 0x004BB41C(this)
DECOMP(0006F4F8, VtB0F2C0_Dtor) { TDtor(ctx, 0x00B0F2C0, 0x004BB41C); }
// *this = vtable 0x00B0F608, then 0x004BB41C(this)
DECOMP(00074538, VtB0F608_Dtor) { TDtor(ctx, 0x00B0F608, 0x004BB41C); }
// *this = vtable 0x00B0F748, then 0x004BB41C(this)
DECOMP(00076460, VtB0F748_Dtor) { TDtor(ctx, 0x00B0F748, 0x004BB41C); }
// *this = vtable 0x00B0F770, then 0x004BB41C(this)
DECOMP(00076F38, VtB0F770_Dtor) { TDtor(ctx, 0x00B0F770, 0x004BB41C); }
// *this = vtable 0x00B0F798, then 0x004BB41C(this)
DECOMP(000772F4, VtB0F798_Dtor) { TDtor(ctx, 0x00B0F798, 0x004BB41C); }
// *this = vtable 0x00B0F7C0, then 0x004BB41C(this)
DECOMP(00077698, VtB0F7C0_Dtor) { TDtor(ctx, 0x00B0F7C0, 0x004BB41C); }
// *this = vtable 0x00B0F7E8, then 0x004BB41C(this)
DECOMP(00077B3C, VtB0F7E8_Dtor) { TDtor(ctx, 0x00B0F7E8, 0x004BB41C); }
// *this = vtable 0x00B0F838, then 0x004BB41C(this)
DECOMP(00079494, VtB0F838_Dtor) { TDtor(ctx, 0x00B0F838, 0x004BB41C); }
// *this = vtable 0x00B0F860, then 0x004BB41C(this)
DECOMP(00079E84, VtB0F860_Dtor) { TDtor(ctx, 0x00B0F860, 0x004BB41C); }
// *this = vtable 0x00B0F8D8, then 0x004BB41C(this)
DECOMP(0007A740, VtB0F8D8_Dtor) { TDtor(ctx, 0x00B0F8D8, 0x004BB41C); }
// *this = vtable 0x00B0F900, then 0x004BB41C(this)
DECOMP(0007AAF0, VtB0F900_Dtor) { TDtor(ctx, 0x00B0F900, 0x004BB41C); }
// *this = vtable 0x00B0F978, then 0x004BB41C(this)
DECOMP(0007B260, VtB0F978_Dtor) { TDtor(ctx, 0x00B0F978, 0x004BB41C); }
// *this = vtable 0x00B0F9A0, then 0x004BB41C(this)
DECOMP(0007B470, VtB0F9A0_Dtor) { TDtor(ctx, 0x00B0F9A0, 0x004BB41C); }
// *this = vtable 0x00B0F9C8, then 0x004BB41C(this)
DECOMP(0007B7A8, VtB0F9C8_Dtor) { TDtor(ctx, 0x00B0F9C8, 0x004BB41C); }
// *this = vtable 0x00B0F9F0, then 0x004BB41C(this)
DECOMP(0007B9C4, VtB0F9F0_Dtor) { TDtor(ctx, 0x00B0F9F0, 0x004BB41C); }
// *this = vtable 0x00B0FA18, then 0x004BB41C(this)
DECOMP(0007BCE8, VtB0FA18_Dtor) { TDtor(ctx, 0x00B0FA18, 0x004BB41C); }
// *this = vtable 0x00B0FA40, then 0x004BB41C(this)
DECOMP(0007C2E0, VtB0FA40_Dtor) { TDtor(ctx, 0x00B0FA40, 0x004BB41C); }
// *this = vtable 0x00B0FA68, then 0x004BB41C(this)
DECOMP(0007C544, VtB0FA68_Dtor) { TDtor(ctx, 0x00B0FA68, 0x004BB41C); }
// *this = vtable 0x00B0FA90, then 0x004BB41C(this)
DECOMP(0007C76C, VtB0FA90_Dtor) { TDtor(ctx, 0x00B0FA90, 0x004BB41C); }
// *this = vtable 0x00B0FAB8, then 0x004BB41C(this)
DECOMP(0007C998, VtB0FAB8_Dtor) { TDtor(ctx, 0x00B0FAB8, 0x004BB41C); }
// *this = vtable 0x00B0FC48, then 0x004BB41C(this)
DECOMP(0007F874, VtB0FC48_Dtor) { TDtor(ctx, 0x00B0FC48, 0x004BB41C); }
// *this = vtable 0x00B0FCC0, then 0x004BB41C(this)
DECOMP(000805CC, VtB0FCC0_Dtor) { TDtor(ctx, 0x00B0FCC0, 0x004BB41C); }
// *this = vtable 0x00B0FE28, then 0x004BB41C(this)
DECOMP(00081ED8, VtB0FE28_Dtor) { TDtor(ctx, 0x00B0FE28, 0x004BB41C); }
// *this = vtable 0x00B0FE50, then 0x004BB41C(this)
DECOMP(00082534, VtB0FE50_Dtor) { TDtor(ctx, 0x00B0FE50, 0x004BB41C); }
// *this = vtable 0x00B0FEF0, then 0x004BB41C(this)
DECOMP(0008339C, VtB0FEF0_Dtor) { TDtor(ctx, 0x00B0FEF0, 0x004BB41C); }
// *this = vtable 0x00B10030, then 0x004BB41C(this)
DECOMP(000852F8, VtB10030_Dtor) { TDtor(ctx, 0x00B10030, 0x004BB41C); }
// *this = vtable 0x00B100A8, then 0x004BB41C(this)
DECOMP(00085950, VtB100A8_Dtor) { TDtor(ctx, 0x00B100A8, 0x004BB41C); }
// *this = vtable 0x00B10120, then 0x004BB41C(this)
DECOMP(000868E8, VtB10120_Dtor) { TDtor(ctx, 0x00B10120, 0x004BB41C); }
// *this = vtable 0x00B10148, then 0x004BB41C(this)
DECOMP(00086A8C, VtB10148_Dtor) { TDtor(ctx, 0x00B10148, 0x004BB41C); }
// *this = vtable 0x00B10210, then 0x004BB41C(this)
DECOMP(0008745C, VtB10210_Dtor) { TDtor(ctx, 0x00B10210, 0x004BB41C); }
// *this = vtable 0x00B10238, then 0x004BB41C(this)
DECOMP(0008767C, VtB10238_Dtor) { TDtor(ctx, 0x00B10238, 0x004BB41C); }
// *this = vtable 0x00B10260, then 0x004BB41C(this)
DECOMP(00087850, VtB10260_Dtor) { TDtor(ctx, 0x00B10260, 0x004BB41C); }
// *this = vtable 0x00B10288, then 0x004BB41C(this)
DECOMP(00087C74, VtB10288_Dtor) { TDtor(ctx, 0x00B10288, 0x004BB41C); }
// *this = vtable 0x00B10508, then 0x004BB41C(this)
DECOMP(0008AC44, VtB10508_Dtor) { TDtor(ctx, 0x00B10508, 0x004BB41C); }
// *this = vtable 0x00B105D0, then 0x004BB41C(this)
DECOMP(0008B7B0, VtB105D0_Dtor) { TDtor(ctx, 0x00B105D0, 0x004BB41C); }
// *this = vtable 0x00B10648, then 0x004BB41C(this)
DECOMP(0008C008, VtB10648_Dtor) { TDtor(ctx, 0x00B10648, 0x004BB41C); }
// *this = vtable 0x00B10760, then 0x004BB41C(this)
DECOMP(0008CEC8, VtB10760_Dtor) { TDtor(ctx, 0x00B10760, 0x004BB41C); }
// *this = vtable 0x00B10788, then 0x004BB41C(this)
DECOMP(0008D1C4, VtB10788_Dtor) { TDtor(ctx, 0x00B10788, 0x004BB41C); }
// *this = vtable 0x00B107B0, then 0x004BB41C(this)
DECOMP(0008D408, VtB107B0_Dtor) { TDtor(ctx, 0x00B107B0, 0x004BB41C); }
// *this = vtable 0x00B107D8, then 0x004BB41C(this)
DECOMP(0008D710, VtB107D8_Dtor) { TDtor(ctx, 0x00B107D8, 0x004BB41C); }
// *this = vtable 0x00B10800, then 0x004BB41C(this)
DECOMP(0008DA84, VtB10800_Dtor) { TDtor(ctx, 0x00B10800, 0x004BB41C); }
// *this = vtable 0x00B10878, then 0x004BB41C(this)
DECOMP(0008FE90, VtB10878_Dtor) { TDtor(ctx, 0x00B10878, 0x004BB41C); }
// *this = vtable 0x00B10918, then 0x004BB41C(this)
DECOMP(00090A5C, VtB10918_Dtor) { TDtor(ctx, 0x00B10918, 0x004BB41C); }
// *this = vtable 0x00B10968, then 0x004BB41C(this)
DECOMP(00091610, VtB10968_Dtor) { TDtor(ctx, 0x00B10968, 0x004BB41C); }
// *this = vtable 0x00B109B8, then 0x004BB41C(this)
DECOMP(00091F1C, VtB109B8_Dtor) { TDtor(ctx, 0x00B109B8, 0x004BB41C); }
// *this = vtable 0x00B10AA8, then 0x004BB41C(this)
DECOMP(00092D0C, VtB10AA8_Dtor) { TDtor(ctx, 0x00B10AA8, 0x004BB41C); }
// *this = vtable 0x00B10B98, then 0x004BB41C(this)
DECOMP(000963F0, VtB10B98_Dtor) { TDtor(ctx, 0x00B10B98, 0x004BB41C); }
// *this = vtable 0x00B10C10, then 0x004BB41C(this)
DECOMP(000969C4, VtB10C10_Dtor) { TDtor(ctx, 0x00B10C10, 0x004BB41C); }
// *this = vtable 0x00B10D28, then 0x004BB41C(this)
DECOMP(00097CB0, VtB10D28_Dtor) { TDtor(ctx, 0x00B10D28, 0x004BB41C); }
// *this = vtable 0x00B10DC8, then 0x004BB41C(this)
DECOMP(00099550, VtB10DC8_Dtor) { TDtor(ctx, 0x00B10DC8, 0x004BB41C); }
// *this = vtable 0x00B10DF0, then 0x004BB41C(this)
DECOMP(000996DC, VtB10DF0_Dtor) { TDtor(ctx, 0x00B10DF0, 0x004BB41C); }
// *this = vtable 0x00B10E18, then 0x004BB41C(this)
DECOMP(0009989C, VtB10E18_Dtor) { TDtor(ctx, 0x00B10E18, 0x004BB41C); }
// *this = vtable 0x00B10E68, then 0x004BB41C(this)
DECOMP(00099D5C, VtB10E68_Dtor) { TDtor(ctx, 0x00B10E68, 0x004BB41C); }
// *this = vtable 0x00B11BD0, then 0x004B6884(this)
DECOMP(000A7AD0, VtB11BD0_Dtor) { TDtor(ctx, 0x00B11BD0, 0x004B6884); }
// *this = vtable 0x00B11C18, then 0x004B6884(this)
DECOMP(000A7CA8, VtB11C18_Dtor) { TDtor(ctx, 0x00B11C18, 0x004B6884); }
// *this = vtable 0x00B11C60, then 0x004B6884(this)
DECOMP(000A7E80, VtB11C60_Dtor) { TDtor(ctx, 0x00B11C60, 0x004B6884); }
// *this = vtable 0x00B11CA8, then 0x004B6884(this)
DECOMP(000A8058, VtB11CA8_Dtor) { TDtor(ctx, 0x00B11CA8, 0x004B6884); }
// *this = vtable 0x00B11CF0, then 0x004B6884(this)
DECOMP(000A8230, VtB11CF0_Dtor) { TDtor(ctx, 0x00B11CF0, 0x004B6884); }
// *this = vtable 0x00B11D38, then 0x004B6884(this)
DECOMP(000A8408, VtB11D38_Dtor) { TDtor(ctx, 0x00B11D38, 0x004B6884); }
// *this = vtable 0x00B15970, then 0x000E2A34(this)
DECOMP(001AD158, VtB15970_Dtor_001AD158) { TDtor(ctx, 0x00B15970, 0x000E2A34); }
// *this = vtable 0x00B17FF0, then 0x000E2A34(this)
DECOMP(001E52E8, VtB17FF0_Dtor) { TDtor(ctx, 0x00B17FF0, 0x000E2A34); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0020BF60, VtB19510_Dtor_0020BF60) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B126B8, then 0x000DF934(this)
DECOMP(00228B84, VtB126B8_Dtor) { TDtor(ctx, 0x00B126B8, 0x000DF934); }
// *this = vtable 0x00B20F68, then 0x0042CA50(this)
DECOMP(00430050, VtB20F68_Dtor) { TDtor(ctx, 0x00B20F68, 0x0042CA50); }
// *this = vtable 0x00B21040, then 0x0042CA50(this)
DECOMP(004322AC, VtB21040_Dtor) { TDtor(ctx, 0x00B21040, 0x0042CA50); }
// *this = vtable 0x00B21088, then 0x0042CA50(this)
DECOMP(00433464, VtB21088_Dtor) { TDtor(ctx, 0x00B21088, 0x0042CA50); }
// *this = vtable 0x00B210B8, then 0x0042CA50(this)
DECOMP(004339E8, VtB210B8_Dtor) { TDtor(ctx, 0x00B210B8, 0x0042CA50); }
// *this = vtable 0x00B210D0, then 0x0042CA50(this)
DECOMP(00433D10, VtB210D0_Dtor) { TDtor(ctx, 0x00B210D0, 0x0042CA50); }
// *this = vtable 0x00B210E8, then 0x0042CA50(this)
DECOMP(004341B0, VtB210E8_Dtor) { TDtor(ctx, 0x00B210E8, 0x0042CA50); }
// *this = vtable 0x00B22598, then 0x00558CE8(this)
DECOMP(004D2128, VtB22598_Dtor) { TDtor(ctx, 0x00B22598, 0x00558CE8); }

// ---------------------------------------------------------------- deleting destructors: the vtable, the base destructor, the free
// deleting: vtable 0x00B0DE10, 0x0011BCF4(this), 0x0054EE6C(this, 0)
DECOMP(0001BF7C, VtB0DE10_Delete) { TDelete(ctx, 0x00B0DE10, 0x0011BCF4, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0ED68, 0x00455A94(this), 0x0054EE6C(this, 0)
DECOMP(00061B60, VtB0ED68_Delete) { TDelete(ctx, 0x00B0ED68, 0x00455A94, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F068, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0006B768, VtB0F068_Delete) { TDelete(ctx, 0x00B0F068, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F0B8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0006BDBC, VtB0F0B8_Delete) { TDelete(ctx, 0x00B0F0B8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F0E0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0006C160, VtB0F0E0_Delete) { TDelete(ctx, 0x00B0F0E0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F108, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0006C640, VtB0F108_Delete) { TDelete(ctx, 0x00B0F108, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F130, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0006CAA8, VtB0F130_Delete) { TDelete(ctx, 0x00B0F130, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F158, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0006CFAC, VtB0F158_Delete) { TDelete(ctx, 0x00B0F158, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1A8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0006E0F4, VtB0F1A8_Delete) { TDelete(ctx, 0x00B0F1A8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F298, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0006F330, VtB0F298_Delete) { TDelete(ctx, 0x00B0F298, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F2C0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0006F528, VtB0F2C0_Delete) { TDelete(ctx, 0x00B0F2C0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F608, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00074568, VtB0F608_Delete) { TDelete(ctx, 0x00B0F608, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F748, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00076490, VtB0F748_Delete) { TDelete(ctx, 0x00B0F748, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F770, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00076F68, VtB0F770_Delete) { TDelete(ctx, 0x00B0F770, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F798, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00077324, VtB0F798_Delete) { TDelete(ctx, 0x00B0F798, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F7C0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(000776C8, VtB0F7C0_Delete) { TDelete(ctx, 0x00B0F7C0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F7E8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00077B6C, VtB0F7E8_Delete) { TDelete(ctx, 0x00B0F7E8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F838, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(000794C4, VtB0F838_Delete) { TDelete(ctx, 0x00B0F838, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F860, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00079EB4, VtB0F860_Delete) { TDelete(ctx, 0x00B0F860, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F8D8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007A770, VtB0F8D8_Delete) { TDelete(ctx, 0x00B0F8D8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F900, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007AB20, VtB0F900_Delete) { TDelete(ctx, 0x00B0F900, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F978, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007B290, VtB0F978_Delete) { TDelete(ctx, 0x00B0F978, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F9A0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007B4A0, VtB0F9A0_Delete) { TDelete(ctx, 0x00B0F9A0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F9C8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007B7D8, VtB0F9C8_Delete) { TDelete(ctx, 0x00B0F9C8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F9F0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007B9F4, VtB0F9F0_Delete) { TDelete(ctx, 0x00B0F9F0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FA18, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007BD18, VtB0FA18_Delete) { TDelete(ctx, 0x00B0FA18, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FA40, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007C310, VtB0FA40_Delete) { TDelete(ctx, 0x00B0FA40, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FA68, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007C574, VtB0FA68_Delete) { TDelete(ctx, 0x00B0FA68, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FA90, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007C79C, VtB0FA90_Delete) { TDelete(ctx, 0x00B0FA90, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FAB8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007C9C8, VtB0FAB8_Delete) { TDelete(ctx, 0x00B0FAB8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FC48, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0007F8A4, VtB0FC48_Delete) { TDelete(ctx, 0x00B0FC48, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FCC0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(000805FC, VtB0FCC0_Delete) { TDelete(ctx, 0x00B0FCC0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FE28, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00081F08, VtB0FE28_Delete) { TDelete(ctx, 0x00B0FE28, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FE50, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00082564, VtB0FE50_Delete) { TDelete(ctx, 0x00B0FE50, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FEF0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(000833CC, VtB0FEF0_Delete) { TDelete(ctx, 0x00B0FEF0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10030, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00085328, VtB10030_Delete) { TDelete(ctx, 0x00B10030, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B100A8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00085980, VtB100A8_Delete) { TDelete(ctx, 0x00B100A8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10120, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00086918, VtB10120_Delete) { TDelete(ctx, 0x00B10120, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10148, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00086ABC, VtB10148_Delete) { TDelete(ctx, 0x00B10148, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10210, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008748C, VtB10210_Delete) { TDelete(ctx, 0x00B10210, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10238, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(000876AC, VtB10238_Delete) { TDelete(ctx, 0x00B10238, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10260, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00087880, VtB10260_Delete) { TDelete(ctx, 0x00B10260, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10288, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00087CA4, VtB10288_Delete) { TDelete(ctx, 0x00B10288, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10508, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008AC74, VtB10508_Delete) { TDelete(ctx, 0x00B10508, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B105D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008B7E0, VtB105D0_Delete) { TDelete(ctx, 0x00B105D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10648, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008C038, VtB10648_Delete) { TDelete(ctx, 0x00B10648, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10760, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008CEF8, VtB10760_Delete) { TDelete(ctx, 0x00B10760, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10788, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008D1F4, VtB10788_Delete) { TDelete(ctx, 0x00B10788, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B107B0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008D438, VtB107B0_Delete) { TDelete(ctx, 0x00B107B0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B107D8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008D740, VtB107D8_Delete) { TDelete(ctx, 0x00B107D8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10800, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008DAB4, VtB10800_Delete) { TDelete(ctx, 0x00B10800, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10878, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0008FEC0, VtB10878_Delete) { TDelete(ctx, 0x00B10878, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10918, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00090A8C, VtB10918_Delete) { TDelete(ctx, 0x00B10918, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10968, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00091640, VtB10968_Delete) { TDelete(ctx, 0x00B10968, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B109B8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00091F4C, VtB109B8_Delete) { TDelete(ctx, 0x00B109B8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10AA8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00092D3C, VtB10AA8_Delete) { TDelete(ctx, 0x00B10AA8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10B98, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00096420, VtB10B98_Delete) { TDelete(ctx, 0x00B10B98, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10C10, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(000969F4, VtB10C10_Delete) { TDelete(ctx, 0x00B10C10, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10D28, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00097CE0, VtB10D28_Delete) { TDelete(ctx, 0x00B10D28, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10DC8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00099580, VtB10DC8_Delete) { TDelete(ctx, 0x00B10DC8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10DF0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0009970C, VtB10DF0_Delete) { TDelete(ctx, 0x00B10DF0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10E18, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(000998CC, VtB10E18_Delete) { TDelete(ctx, 0x00B10E18, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10E68, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00099D8C, VtB10E68_Delete) { TDelete(ctx, 0x00B10E68, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11BD0, 0x004B6884(this), 0x00552420(this, 24)
DECOMP(000A7B00, VtB11BD0_Delete) { TDelete(ctx, 0x00B11BD0, 0x004B6884, 0x00552420, 24); }
// deleting: vtable 0x00B11C18, 0x004B6884(this), 0x00552420(this, 24)
DECOMP(000A7CD8, VtB11C18_Delete) { TDelete(ctx, 0x00B11C18, 0x004B6884, 0x00552420, 24); }
// deleting: vtable 0x00B11C60, 0x004B6884(this), 0x00552420(this, 24)
DECOMP(000A7EB0, VtB11C60_Delete) { TDelete(ctx, 0x00B11C60, 0x004B6884, 0x00552420, 24); }
// deleting: vtable 0x00B11CA8, 0x004B6884(this), 0x00552420(this, 24)
DECOMP(000A8088, VtB11CA8_Delete) { TDelete(ctx, 0x00B11CA8, 0x004B6884, 0x00552420, 24); }
// deleting: vtable 0x00B11CF0, 0x004B6884(this), 0x00552420(this, 24)
DECOMP(000A8260, VtB11CF0_Delete) { TDelete(ctx, 0x00B11CF0, 0x004B6884, 0x00552420, 24); }
// deleting: vtable 0x00B11D38, 0x004B6884(this), 0x00552420(this, 24)
DECOMP(000A8438, VtB11D38_Delete) { TDelete(ctx, 0x00B11D38, 0x004B6884, 0x00552420, 24); }
// deleting: vtable 0x00B15970, 0x000E2A34(this), 0x0054EE6C(this, 0)
DECOMP(001AD188, VtB15970_Delete_001AD188) { TDelete(ctx, 0x00B15970, 0x000E2A34, 0x0054EE6C, 0); }
// deleting: vtable 0x00B17FF0, 0x000E2A34(this), 0x00552420(this, 480)
DECOMP(001E5318, VtB17FF0_Delete) { TDelete(ctx, 0x00B17FF0, 0x000E2A34, 0x00552420, 480); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x0054EE6C(this, 0)
DECOMP(0020BF90, VtB19510_Delete_0020BF90) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B126B8, 0x000DF934(this), 0x0054EE6C(this, 0)
DECOMP(00228BB4, VtB126B8_Delete) { TDelete(ctx, 0x00B126B8, 0x000DF934, 0x0054EE6C, 0); }
// deleting: vtable 0x00B20F68, 0x0042CA50(this), 0x00552420(this, 112)
DECOMP(00430080, VtB20F68_Delete) { TDelete(ctx, 0x00B20F68, 0x0042CA50, 0x00552420, 112); }
// deleting: vtable 0x00B21040, 0x0042CA50(this), 0x00552420(this, 44)
DECOMP(004322DC, VtB21040_Delete) { TDelete(ctx, 0x00B21040, 0x0042CA50, 0x00552420, 44); }
// deleting: vtable 0x00B21088, 0x0042CA50(this), 0x00552420(this, 16)
DECOMP(00433494, VtB21088_Delete) { TDelete(ctx, 0x00B21088, 0x0042CA50, 0x00552420, 16); }
// deleting: vtable 0x00B210B8, 0x0042CA50(this), 0x00552420(this, 156)
DECOMP(00433A18, VtB210B8_Delete) { TDelete(ctx, 0x00B210B8, 0x0042CA50, 0x00552420, 156); }
// deleting: vtable 0x00B210D0, 0x0042CA50(this), 0x00552420(this, 560)
DECOMP(00433D40, VtB210D0_Delete) { TDelete(ctx, 0x00B210D0, 0x0042CA50, 0x00552420, 560); }
// deleting: vtable 0x00B210E8, 0x0042CA50(this), 0x00552420(this, 28)
DECOMP(004341E0, VtB210E8_Delete) { TDelete(ctx, 0x00B210E8, 0x0042CA50, 0x00552420, 28); }
// deleting: vtable 0x00B22598, 0x00558CE8(this), 0x0054EE6C(this, 0)
DECOMP(004D2158, VtB22598_Delete) { TDelete(ctx, 0x00B22598, 0x00558CE8, 0x0054EE6C, 0); }

// ---------------------------------------------------------------- deleting destructors calling the out-of-line destructor
// deleting: 0x00062B20(this), then 0x00552420(this, 452)
DECOMP(00062DD0, Delete_00062DD0) { TDeleteCall(ctx, 0x00062B20, 0x00552420, 452); }
// deleting: 0x00064894(this), then 0x00552420(this, 456)
DECOMP(00064B8C, Delete_00064B8C) { TDeleteCall(ctx, 0x00064894, 0x00552420, 456); }
// deleting: 0x00070894(this), then 0x0054EE6C(this, 0)
DECOMP(00070B00, Delete_00070B00) { TDeleteCall(ctx, 0x00070894, 0x0054EE6C, 0); }
// deleting: 0x00098500(this), then 0x0054EE6C(this, 0)
DECOMP(000986C0, Delete_000986C0) { TDeleteCall(ctx, 0x00098500, 0x0054EE6C, 0); }
// deleting: 0x000D7024(this), then 0x0054EE6C(this, 0)
DECOMP(000D7254, Delete_000D7254) { TDeleteCall(ctx, 0x000D7024, 0x0054EE6C, 0); }
// deleting: 0x000DD8A4(this), then 0x00552420(this, 240)
DECOMP(000DD9B0, Delete_000DD9B0) { TDeleteCall(ctx, 0x000DD8A4, 0x00552420, 240); }
// deleting: 0x000E281C(this), then 0x0054EE6C(this, 0)
DECOMP(000E29EC, Delete_000E29EC) { TDeleteCall(ctx, 0x000E281C, 0x0054EE6C, 0); }
// deleting: 0x000E9820(this), then 0x0054EE6C(this, 0)
DECOMP(000E992C, Delete_000E992C) { TDeleteCall(ctx, 0x000E9820, 0x0054EE6C, 0); }
// deleting: 0x000EB0F8(this), then 0x0054EE6C(this, 0)
DECOMP(000EB1B0, Delete_000EB1B0) { TDeleteCall(ctx, 0x000EB0F8, 0x0054EE6C, 0); }
// deleting: 0x001002C4(this), then 0x00552420(this, 1040)
DECOMP(00100470, Delete_00100470) { TDeleteCall(ctx, 0x001002C4, 0x00552420, 1040); }
// deleting: 0x0010C3C8(this), then 0x00552420(this, 688)
DECOMP(0010C60C, Delete_0010C60C) { TDeleteCall(ctx, 0x0010C3C8, 0x00552420, 688); }
// deleting: 0x0010E9A4(this), then 0x00552420(this, 608)
DECOMP(0010EAD0, Delete_0010EAD0) { TDeleteCall(ctx, 0x0010E9A4, 0x00552420, 608); }
// deleting: 0x00121BDC(this), then 0x0054EE6C(this, 0)
DECOMP(00121E24, Delete_00121E24) { TDeleteCall(ctx, 0x00121BDC, 0x0054EE6C, 0); }
// deleting: 0x0016EC4C(this), then 0x00552420(this, 800)
DECOMP(0016EE20, Delete_0016EE20) { TDeleteCall(ctx, 0x0016EC4C, 0x00552420, 800); }
// deleting: 0x00184280(this), then 0x00552420(this, 1728)
DECOMP(00184700, Delete_00184700) { TDeleteCall(ctx, 0x00184280, 0x00552420, 1728); }
// deleting: 0x00191E54(this), then 0x00552420(this, 704)
DECOMP(00191FEC, Delete_00191FEC) { TDeleteCall(ctx, 0x00191E54, 0x00552420, 704); }
// deleting: 0x001AE468(this), then 0x0054EE6C(this, 0)
DECOMP(001AE574, Delete_001AE574) { TDeleteCall(ctx, 0x001AE468, 0x0054EE6C, 0); }
// deleting: 0x001BB9B0(this), then 0x00552420(this, 416)
DECOMP(001BBBC8, Delete_001BBBC8) { TDeleteCall(ctx, 0x001BB9B0, 0x00552420, 416); }
// deleting: 0x001EB5E0(this), then 0x00552420(this, 512)
DECOMP(001EB6E4, Delete_001EB6E4) { TDeleteCall(ctx, 0x001EB5E0, 0x00552420, 512); }
// deleting: 0x001ED5F4(this), then 0x00552420(this, 992)
DECOMP(001ED72C, Delete_001ED72C) { TDeleteCall(ctx, 0x001ED5F4, 0x00552420, 992); }
// deleting: 0x00212AA0(this), then 0x00552420(this, 432)
DECOMP(00212BE0, Delete_00212BE0) { TDeleteCall(ctx, 0x00212AA0, 0x00552420, 432); }
// deleting: 0x00215C00(this), then 0x00552420(this, 480)
DECOMP(00215D40, Delete_00215D40) { TDeleteCall(ctx, 0x00215C00, 0x00552420, 480); }
// deleting: 0x00218BF0(this), then 0x00552420(this, 432)
DECOMP(00218D30, Delete_00218D30) { TDeleteCall(ctx, 0x00218BF0, 0x00552420, 432); }
// deleting: 0x00219C88(this), then 0x00552420(this, 432)
DECOMP(00219DC8, Delete_00219DC8) { TDeleteCall(ctx, 0x00219C88, 0x00552420, 432); }
// deleting: 0x0021AF48(this), then 0x00552420(this, 480)
DECOMP(0021B088, Delete_0021B088) { TDeleteCall(ctx, 0x0021AF48, 0x00552420, 480); }
// deleting: 0x0021C5B0(this), then 0x00552420(this, 464)
DECOMP(0021C72C, Delete_0021C72C) { TDeleteCall(ctx, 0x0021C5B0, 0x00552420, 464); }
// deleting: 0x0021DB80(this), then 0x00552420(this, 432)
DECOMP(0021DCC0, Delete_0021DCC0) { TDeleteCall(ctx, 0x0021DB80, 0x00552420, 432); }
// deleting: 0x0021F088(this), then 0x0054EE6C(this, 0)
DECOMP(0021F1EC, Delete_0021F1EC) { TDeleteCall(ctx, 0x0021F088, 0x0054EE6C, 0); }
// deleting: 0x00220C68(this), then 0x00552420(this, 704)
DECOMP(00220DA8, Delete_00220DA8) { TDeleteCall(ctx, 0x00220C68, 0x00552420, 704); }
// deleting: 0x00221598(this), then 0x00552420(this, 432)
DECOMP(002216D8, Delete_002216D8) { TDeleteCall(ctx, 0x00221598, 0x00552420, 432); }
// deleting: 0x0022499C(this), then 0x00552420(this, 448)
DECOMP(00224B00, Delete_00224B00) { TDeleteCall(ctx, 0x0022499C, 0x00552420, 448); }
// deleting: 0x00225600(this), then 0x00552420(this, 464)
DECOMP(0022576C, Delete_0022576C) { TDeleteCall(ctx, 0x00225600, 0x00552420, 464); }
// deleting: 0x0022DD9C(this), then 0x00552420(this, 1616)
DECOMP(0022DF78, Delete_0022DF78) { TDeleteCall(ctx, 0x0022DD9C, 0x00552420, 1616); }
// deleting: 0x002346A4(this), then 0x00552420(this, 1104)
DECOMP(002348B0, Delete_002348B0) { TDeleteCall(ctx, 0x002346A4, 0x00552420, 1104); }
// deleting: 0x00240E8C(this), then 0x00552420(this, 640)
DECOMP(00240F6C, Delete_00240F6C) { TDeleteCall(ctx, 0x00240E8C, 0x00552420, 640); }
// deleting: 0x00248118(this), then 0x00552420(this, 1440)
DECOMP(00248210, Delete_00248210) { TDeleteCall(ctx, 0x00248118, 0x00552420, 1440); }
// deleting: 0x0026694C(this), then 0x00552420(this, 56)
DECOMP(00266BF4, Delete_00266BF4) { TDeleteCall(ctx, 0x0026694C, 0x00552420, 56); }
// deleting: 0x00269348(this), then 0x00552420(this, 720)
DECOMP(002695D8, Delete_002695D8) { TDeleteCall(ctx, 0x00269348, 0x00552420, 720); }
// deleting: 0x0026F914(this), then 0x00552420(this, 400)
DECOMP(0026FA8C, Delete_0026FA8C) { TDeleteCall(ctx, 0x0026F914, 0x00552420, 400); }
// deleting: 0x00274AC0(this), then 0x00552420(this, 188)
DECOMP(00274C94, Delete_00274C94) { TDeleteCall(ctx, 0x00274AC0, 0x00552420, 188); }
// deleting: 0x002869A0(this), then 0x00552420(this, 36)
DECOMP(00286B0C, Delete_00286B0C) { TDeleteCall(ctx, 0x002869A0, 0x00552420, 36); }
// deleting: 0x00289AA8(this), then 0x00552420(this, 656)
DECOMP(00289D7C, Delete_00289D7C) { TDeleteCall(ctx, 0x00289AA8, 0x00552420, 656); }
// deleting: 0x0028E9F8(this), then 0x00552420(this, 2948)
DECOMP(0028EB70, Delete_0028EB70) { TDeleteCall(ctx, 0x0028E9F8, 0x00552420, 2948); }
// deleting: 0x00291228(this), then 0x00552420(this, 368)
DECOMP(00291480, Delete_00291480) { TDeleteCall(ctx, 0x00291228, 0x00552420, 368); }
// deleting: 0x00294540(this), then 0x00552420(this, 1312)
DECOMP(00294700, Delete_00294700) { TDeleteCall(ctx, 0x00294540, 0x00552420, 1312); }
// deleting: 0x00297900(this), then 0x00552420(this, 452)
DECOMP(00297B54, Delete_00297B54) { TDeleteCall(ctx, 0x00297900, 0x00552420, 452); }
// deleting: 0x0029A108(this), then 0x00552420(this, 2352)
DECOMP(0029A3C0, Delete_0029A3C0) { TDeleteCall(ctx, 0x0029A108, 0x00552420, 2352); }
// deleting: 0x002A32AC(this), then 0x00552420(this, 2912)
DECOMP(002A3A70, Delete_002A3A70) { TDeleteCall(ctx, 0x002A32AC, 0x00552420, 2912); }
// deleting: 0x002AD2A4(this), then 0x00552420(this, 352)
DECOMP(002AD438, Delete_002AD438) { TDeleteCall(ctx, 0x002AD2A4, 0x00552420, 352); }
// deleting: 0x002AF3AC(this), then 0x00552420(this, 144)
DECOMP(002AF534, Delete_002AF534) { TDeleteCall(ctx, 0x002AF3AC, 0x00552420, 144); }
// deleting: 0x002B17EC(this), then 0x00552420(this, 44)
DECOMP(002B1954, Delete_002B1954) { TDeleteCall(ctx, 0x002B17EC, 0x00552420, 44); }
// deleting: 0x002B2A70(this), then 0x00552420(this, 144)
DECOMP(002B2C30, Delete_002B2C30) { TDeleteCall(ctx, 0x002B2A70, 0x00552420, 144); }
// deleting: 0x002B4830(this), then 0x0054EE6C(this, 0)
DECOMP(002B49FC, Delete_002B49FC) { TDeleteCall(ctx, 0x002B4830, 0x0054EE6C, 0); }
// deleting: 0x002B50D4(this), then 0x00552420(this, 144)
DECOMP(002B5294, Delete_002B5294) { TDeleteCall(ctx, 0x002B50D4, 0x00552420, 144); }
// deleting: 0x002B6E2C(this), then 0x00552420(this, 144)
DECOMP(002B7074, Delete_002B7074) { TDeleteCall(ctx, 0x002B6E2C, 0x00552420, 144); }
// deleting: 0x002BA884(this), then 0x00552420(this, 80)
DECOMP(002BABC4, Delete_002BABC4) { TDeleteCall(ctx, 0x002BA884, 0x00552420, 80); }
// deleting: 0x002BBC18(this), then 0x0054EE6C(this, 0)
DECOMP(002BBD90, Delete_002BBD90) { TDeleteCall(ctx, 0x002BBC18, 0x0054EE6C, 0); }
// deleting: 0x002BC728(this), then 0x00552420(this, 400)
DECOMP(002BCA1C, Delete_002BCA1C) { TDeleteCall(ctx, 0x002BC728, 0x00552420, 400); }
// deleting: 0x002C9910(this), then 0x00552420(this, 720)
DECOMP(002C9C50, Delete_002C9C50) { TDeleteCall(ctx, 0x002C9910, 0x00552420, 720); }
// deleting: 0x002CC960(this), then 0x00552420(this, 384)
DECOMP(002CCBE8, Delete_002CCBE8) { TDeleteCall(ctx, 0x002CC960, 0x00552420, 384); }
// deleting: 0x002D1034(this), then 0x00552420(this, 80)
DECOMP(002D127C, Delete_002D127C) { TDeleteCall(ctx, 0x002D1034, 0x00552420, 80); }
// deleting: 0x002D31B8(this), then 0x00552420(this, 96)
DECOMP(002D333C, Delete_002D333C) { TDeleteCall(ctx, 0x002D31B8, 0x00552420, 96); }
// deleting: 0x002E87D4(this), then 0x0054EE6C(this, 0)
DECOMP(002E8954, Delete_002E8954) { TDeleteCall(ctx, 0x002E87D4, 0x0054EE6C, 0); }
// deleting: 0x002EDB10(this), then 0x00552420(this, 156)
DECOMP(002EDCA8, Delete_002EDCA8) { TDeleteCall(ctx, 0x002EDB10, 0x00552420, 156); }
// deleting: 0x002FE63C(this), then 0x00552420(this, 1136)
DECOMP(002FE8E8, Delete_002FE8E8) { TDeleteCall(ctx, 0x002FE63C, 0x00552420, 1136); }
// deleting: 0x00303A4C(this), then 0x00552420(this, 368)
DECOMP(00303CEC, Delete_00303CEC) { TDeleteCall(ctx, 0x00303A4C, 0x00552420, 368); }
// deleting: 0x0030CB10(this), then 0x00552420(this, 1088)
DECOMP(0030CC8C, Delete_0030CC8C) { TDeleteCall(ctx, 0x0030CB10, 0x00552420, 1088); }
// deleting: 0x003266E4(this), then 0x0054EE6C(this, 0)
DECOMP(003269F0, Delete_003269F0) { TDeleteCall(ctx, 0x003266E4, 0x0054EE6C, 0); }
// deleting: 0x003286D8(this), then 0x0054EE6C(this, 0)
DECOMP(003289BC, Delete_003289BC) { TDeleteCall(ctx, 0x003286D8, 0x0054EE6C, 0); }
// deleting: 0x0032B02C(this), then 0x00552420(this, 2604)
DECOMP(0032B300, Delete_0032B300) { TDeleteCall(ctx, 0x0032B02C, 0x00552420, 2604); }
// deleting: 0x0032CA0C(this), then 0x00552420(this, 2584)
DECOMP(0032CCC8, Delete_0032CCC8) { TDeleteCall(ctx, 0x0032CA0C, 0x00552420, 2584); }
// deleting: 0x0032CF20(this), then 0x00552420(this, 2608)
DECOMP(0032D1EC, Delete_0032D1EC) { TDeleteCall(ctx, 0x0032CF20, 0x00552420, 2608); }
// deleting: 0x0032DA14(this), then 0x00552420(this, 3376)
DECOMP(0032DCB4, Delete_0032DCB4) { TDeleteCall(ctx, 0x0032DA14, 0x00552420, 3376); }
// deleting: 0x0032EA68(this), then 0x00552420(this, 2576)
DECOMP(0032ED24, Delete_0032ED24) { TDeleteCall(ctx, 0x0032EA68, 0x00552420, 2576); }
// deleting: 0x0032EF90(this), then 0x00552420(this, 2592)
DECOMP(0032F25C, Delete_0032F25C) { TDeleteCall(ctx, 0x0032EF90, 0x00552420, 2592); }
// deleting: 0x0032FA74(this), then 0x00552420(this, 2596)
DECOMP(0032FD40, Delete_0032FD40) { TDeleteCall(ctx, 0x0032FA74, 0x00552420, 2596); }
// deleting: 0x00331788(this), then 0x00552420(this, 2640)
DECOMP(00331A28, Delete_00331A28) { TDeleteCall(ctx, 0x00331788, 0x00552420, 2640); }
// deleting: 0x00332F44(this), then 0x00552420(this, 2624)
DECOMP(003331E4, Delete_003331E4) { TDeleteCall(ctx, 0x00332F44, 0x00552420, 2624); }
// deleting: 0x003366F0(this), then 0x00552420(this, 2992)
DECOMP(00336A80, Delete_00336A80) { TDeleteCall(ctx, 0x003366F0, 0x00552420, 2992); }
// deleting: 0x0033A308(this), then 0x00552420(this, 2896)
DECOMP(0033A4C4, Delete_0033A4C4) { TDeleteCall(ctx, 0x0033A308, 0x00552420, 2896); }
// deleting: 0x0033D008(this), then 0x00552420(this, 2816)
DECOMP(0033D49C, Delete_0033D49C) { TDeleteCall(ctx, 0x0033D008, 0x00552420, 2816); }
// deleting: 0x00340134(this), then 0x00552420(this, 2588)
DECOMP(003403F0, Delete_003403F0) { TDeleteCall(ctx, 0x00340134, 0x00552420, 2588); }
// deleting: 0x003409B8(this), then 0x00552420(this, 2588)
DECOMP(00340C74, Delete_00340C74) { TDeleteCall(ctx, 0x003409B8, 0x00552420, 2588); }
// deleting: 0x00341208(this), then 0x00552420(this, 2580)
DECOMP(003414C4, Delete_003414C4) { TDeleteCall(ctx, 0x00341208, 0x00552420, 2580); }
// deleting: 0x00344398(this), then 0x0054EE6C(this, 0)
DECOMP(003448C0, Delete_003448C0) { TDeleteCall(ctx, 0x00344398, 0x0054EE6C, 0); }
// deleting: 0x0034BF9C(this), then 0x00552420(this, 120)
DECOMP(0034C8CC, Delete_0034C8CC) { TDeleteCall(ctx, 0x0034BF9C, 0x00552420, 120); }
// deleting: 0x00353334(this), then 0x0054EE6C(this, 0)
DECOMP(0035357C, Delete_0035357C) { TDeleteCall(ctx, 0x00353334, 0x0054EE6C, 0); }
// deleting: 0x0036ABF8(this), then 0x0054EE6C(this, 0)
DECOMP(0036AF54, Delete_0036AF54) { TDeleteCall(ctx, 0x0036ABF8, 0x0054EE6C, 0); }
// deleting: 0x0036C400(this), then 0x00552420(this, 224)
DECOMP(0036C574, Delete_0036C574) { TDeleteCall(ctx, 0x0036C400, 0x00552420, 224); }
// deleting: 0x0036D118(this), then 0x00552420(this, 256)
DECOMP(0036D23C, Delete_0036D23C) { TDeleteCall(ctx, 0x0036D118, 0x00552420, 256); }
// deleting: 0x0036DD30(this), then 0x00552420(this, 416)
DECOMP(0036DE88, Delete_0036DE88) { TDeleteCall(ctx, 0x0036DD30, 0x00552420, 416); }
// deleting: 0x0036EB74(this), then 0x00552420(this, 192)
DECOMP(0036ECDC, Delete_0036ECDC) { TDeleteCall(ctx, 0x0036EB74, 0x00552420, 192); }
// deleting: 0x0036F1FC(this), then 0x00552420(this, 208)
DECOMP(0036F3EC, Delete_0036F3EC) { TDeleteCall(ctx, 0x0036F1FC, 0x00552420, 208); }
// deleting: 0x00370220(this), then 0x00552420(this, 224)
DECOMP(00370498, Delete_00370498) { TDeleteCall(ctx, 0x00370220, 0x00552420, 224); }
// deleting: 0x00370F70(this), then 0x00552420(this, 624)
DECOMP(00371304, Delete_00371304) { TDeleteCall(ctx, 0x00370F70, 0x00552420, 624); }
// deleting: 0x003747F4(this), then 0x00552420(this, 240)
DECOMP(00374A8C, Delete_00374A8C) { TDeleteCall(ctx, 0x003747F4, 0x00552420, 240); }
// deleting: 0x00376F88(this), then 0x00552420(this, 432)
DECOMP(0037726C, Delete_0037726C) { TDeleteCall(ctx, 0x00376F88, 0x00552420, 432); }
// deleting: 0x0037A93C(this), then 0x00552420(this, 480)
DECOMP(0037AC20, Delete_0037AC20) { TDeleteCall(ctx, 0x0037A93C, 0x00552420, 480); }
// deleting: 0x0037EE20(this), then 0x00552420(this, 336)
DECOMP(0037F150, Delete_0037F150) { TDeleteCall(ctx, 0x0037EE20, 0x00552420, 336); }
// deleting: 0x00381320(this), then 0x00552420(this, 416)
DECOMP(00381460, Delete_00381460) { TDeleteCall(ctx, 0x00381320, 0x00552420, 416); }
// deleting: 0x00381AA8(this), then 0x0054EE6C(this, 0)
DECOMP(00381DF8, Delete_00381DF8) { TDeleteCall(ctx, 0x00381AA8, 0x0054EE6C, 0); }
// deleting: 0x003848FC(this), then 0x0054EE6C(this, 0)
DECOMP(00384E9C, Delete_00384E9C) { TDeleteCall(ctx, 0x003848FC, 0x0054EE6C, 0); }
// deleting: 0x003AAD78(this), then 0x0054EE6C(this, 0)
DECOMP(003AAEF0, Delete_003AAEF0) { TDeleteCall(ctx, 0x003AAD78, 0x0054EE6C, 0); }
// deleting: 0x003DCEB8(this), then 0x00552420(this, 384)
DECOMP(003DD170, Delete_003DD170) { TDeleteCall(ctx, 0x003DCEB8, 0x00552420, 384); }
// deleting: 0x003EE694(this), then 0x0054EE6C(this, 0)
DECOMP(003EE91C, Delete_003EE91C) { TDeleteCall(ctx, 0x003EE694, 0x0054EE6C, 0); }
// deleting: 0x00436374(this), then 0x00552420(this, 160)
DECOMP(00436528, Delete_00436528) { TDeleteCall(ctx, 0x00436374, 0x00552420, 160); }
// deleting: 0x004B7DF0(this), then 0x0054EE6C(this, 0)
DECOMP(004B8E90, Delete_004B8E90) { TDeleteCall(ctx, 0x004B7DF0, 0x0054EE6C, 0); }
// deleting: 0x004D2D90(this), then 0x0054EE6C(this, 0)
DECOMP(004D3040, Delete_004D3040) { TDeleteCall(ctx, 0x004D2D90, 0x0054EE6C, 0); }
// deleting: 0x004E51D4(this), then 0x00552420(this, 208)
DECOMP(004E53A8, Delete_004E53A8) { TDeleteCall(ctx, 0x004E51D4, 0x00552420, 208); }
// deleting: 0x004E6224(this), then 0x00552420(this, 208)
DECOMP(004E63CC, Delete_004E63CC) { TDeleteCall(ctx, 0x004E6224, 0x00552420, 208); }
// deleting: 0x004E7B28(this), then 0x0054EE6C(this, 0)
DECOMP(004E7D10, Delete_004E7D10) { TDeleteCall(ctx, 0x004E7B28, 0x0054EE6C, 0); }
// deleting: 0x004E8F54(this), then 0x00552420(this, 176)
DECOMP(004E90FC, Delete_004E90FC) { TDeleteCall(ctx, 0x004E8F54, 0x00552420, 176); }
// deleting: 0x004F3CB4(this), then 0x0054EE6C(this, 0)
DECOMP(004F4164, Delete_004F4164) { TDeleteCall(ctx, 0x004F3CB4, 0x0054EE6C, 0); }
// deleting: 0x004F6F54(this), then 0x0054EE6C(this, 0)
DECOMP(004F70C0, Delete_004F70C0) { TDeleteCall(ctx, 0x004F6F54, 0x0054EE6C, 0); }
// deleting: 0x004FD008(this), then 0x0054EE6C(this, 0)
DECOMP(004FD150, Delete_004FD150) { TDeleteCall(ctx, 0x004FD008, 0x0054EE6C, 0); }

// ---------------------------------------------------------------- constant returns
// returns 0
DECOMP(000149D0, ReturnK_000149D0) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(000149D8, ReturnK_000149D8) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(000256D0, ReturnK_000256D0) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(00027E80, ReturnK_00027E80) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(00027E88, ReturnK_00027E88) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(0002BED0, ReturnK_0002BED0) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(0002BF14, ReturnK_0002BF14) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(0002BF1C, ReturnK_0002BF1C) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(0004377C, ReturnK_0004377C) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(0005FE2C, ReturnK_0005FE2C) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(0008C938, ReturnK_0008C938) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(00099FA0, ReturnK_00099FA0) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(000AF274, ReturnK_000AF274) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(00127ECC, ReturnK_00127ECC) { ret(ctx, static_cast<s32>(0)); }
// returns 1
DECOMP(00157940, ReturnK_00157940) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(0015E2B8, ReturnK_0015E2B8) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(001A97D0, ReturnK_001A97D0) { ret(ctx, static_cast<s32>(1)); }
// returns 4
DECOMP(001CEC84, ReturnK_001CEC84) { ret(ctx, static_cast<s32>(4)); }
// returns 1
DECOMP(001CF700, ReturnK_001CF700) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(001D4B6C, ReturnK_001D4B6C) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(001D4B74, ReturnK_001D4B74) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(001D4B7C, ReturnK_001D4B7C) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(001D4B84, ReturnK_001D4B84) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(001E4388, ReturnK_001E4388) { ret(ctx, static_cast<s32>(1)); }
// returns 16
DECOMP(001EB0B4, ReturnK_001EB0B4) { ret(ctx, static_cast<s32>(16)); }
// returns 0
DECOMP(00229A18, ReturnK_00229A18) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(0028363C, ReturnK_0028363C) { ret(ctx, static_cast<s32>(0)); }
// returns 1
DECOMP(002FD7B8, ReturnK_002FD7B8) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(002FE634, ReturnK_002FE634) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(003157C0, ReturnK_003157C0) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(00391868, ReturnK_00391868) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(00391870, ReturnK_00391870) { ret(ctx, static_cast<s32>(1)); }
// returns 0
DECOMP(00393664, ReturnK_00393664) { ret(ctx, static_cast<s32>(0)); }
// returns 1
DECOMP(003DD22C, ReturnK_003DD22C) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(003E4370, ReturnK_003E4370) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(003F0410, ReturnK_003F0410) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(0042CA70, ReturnK_0042CA70) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(0042D204, ReturnK_0042D204) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(0042E860, ReturnK_0042E860) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(0042E9E8, ReturnK_0042E9E8) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(0042EBB8, ReturnK_0042EBB8) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(00438594, ReturnK_00438594) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(00438628, ReturnK_00438628) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(004386BC, ReturnK_004386BC) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(004842A0, ReturnK_004842A0) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(0048C154, ReturnK_0048C154) { ret(ctx, static_cast<s32>(1)); }
// returns 0
DECOMP(004BEAAC, ReturnK_004BEAAC) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004BEC5C, ReturnK_004BEC5C) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004BEE54, ReturnK_004BEE54) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004BF2E8, ReturnK_004BF2E8) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004BF54C, ReturnK_004BF54C) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004BF7D8, ReturnK_004BF7D8) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004C1190, ReturnK_004C1190) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004C1FDC, ReturnK_004C1FDC) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004C30B4, ReturnK_004C30B4) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004C3DB4, ReturnK_004C3DB4) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004C4334, ReturnK_004C4334) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004C452C, ReturnK_004C452C) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004C46EC, ReturnK_004C46EC) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004C5BA0, ReturnK_004C5BA0) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004C5DDC, ReturnK_004C5DDC) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CA4D8, ReturnK_004CA4D8) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CA874, ReturnK_004CA874) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CAC6C, ReturnK_004CAC6C) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CB080, ReturnK_004CB080) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CB308, ReturnK_004CB308) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CB5E8, ReturnK_004CB5E8) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CC254, ReturnK_004CC254) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CC74C, ReturnK_004CC74C) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CCA88, ReturnK_004CCA88) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CCD10, ReturnK_004CCD10) { ret(ctx, static_cast<s32>(0)); }
// returns 0
DECOMP(004CCF14, ReturnK_004CCF14) { ret(ctx, static_cast<s32>(0)); }
// returns 1
DECOMP(004ED1B8, ReturnK_004ED1B8) { ret(ctx, static_cast<s32>(1)); }
// returns 1
DECOMP(004EE584, ReturnK_004EE584) { ret(ctx, static_cast<s32>(1)); }

// ---------------------------------------------------------------- this-adjusting wrappers
// 0x000C21EC(this - 380, ...)
DECOMP(000C23D4, AdjThunk_000C23D4) { TAdjThunk(ctx, -380, 0x000C21EC); }
// 0x000E8880(this - 380, ...)
DECOMP(000E8DA4, AdjThunk_000E8DA4) { TAdjThunk(ctx, -380, 0x000E8880); }
// 0x000EB0F8(this - 384, ...)
DECOMP(000EB290, AdjThunk_000EB290) { TAdjThunk(ctx, -384, 0x000EB0F8); }
// 0x000EB0F8(this - 380, ...)
DECOMP(000EB2B8, AdjThunk_000EB2B8) { TAdjThunk(ctx, -380, 0x000EB0F8); }
// 0x001002C4(this - 12, ...)
DECOMP(00100504, AdjThunk_00100504) { TAdjThunk(ctx, -12, 0x001002C4); }
// 0x00106B80(this - 12, ...)
DECOMP(00106F20, AdjThunk_00106F20) { TAdjThunk(ctx, -12, 0x00106B80); }
// 0x0016A000(this - 380, ...)
DECOMP(0016A330, AdjThunk_0016A330) { TAdjThunk(ctx, -380, 0x0016A000); }
// 0x0016EC4C(this - 384, ...)
DECOMP(0016EF00, AdjThunk_0016EF00) { TAdjThunk(ctx, -384, 0x0016EC4C); }
// 0x0016EC4C(this - 380, ...)
DECOMP(0016EF28, AdjThunk_0016EF28) { TAdjThunk(ctx, -380, 0x0016EC4C); }
// 0x001728F0(this - 380, ...)
DECOMP(00172B48, AdjThunk_00172B48) { TAdjThunk(ctx, -380, 0x001728F0); }
// 0x0017C088(this - 380, ...)
DECOMP(0017C6B0, AdjThunk_0017C6B0) { TAdjThunk(ctx, -380, 0x0017C088); }
// 0x00184280(this - 384, ...)
DECOMP(001847E0, AdjThunk_001847E0) { TAdjThunk(ctx, -384, 0x00184280); }
// 0x00184280(this - 380, ...)
DECOMP(00184808, AdjThunk_00184808) { TAdjThunk(ctx, -380, 0x00184280); }
// 0x0018CE38(this - 380, ...)
DECOMP(0018D1A0, AdjThunk_0018D1A0) { TAdjThunk(ctx, -380, 0x0018CE38); }
// 0x00191E54(this - 384, ...)
DECOMP(001920CC, AdjThunk_001920CC) { TAdjThunk(ctx, -384, 0x00191E54); }
// 0x00191E54(this - 380, ...)
DECOMP(001920F4, AdjThunk_001920F4) { TAdjThunk(ctx, -380, 0x00191E54); }
// 0x00196790(this - 380, ...)
DECOMP(00197128, AdjThunk_00197128) { TAdjThunk(ctx, -380, 0x00196790); }
// 0x001AE468(this - 384, ...)
DECOMP(001AE654, AdjThunk_001AE654) { TAdjThunk(ctx, -384, 0x001AE468); }
// 0x001AE468(this - 380, ...)
DECOMP(001AE67C, AdjThunk_001AE67C) { TAdjThunk(ctx, -380, 0x001AE468); }
// 0x001B95A8(this - 380, ...)
DECOMP(001B9B3C, AdjThunk_001B9B3C) { TAdjThunk(ctx, -380, 0x001B95A8); }
// 0x001DC104(this - 384, ...)
DECOMP(001DC1D4, AdjThunk_001DC1D4) { TAdjThunk(ctx, -384, 0x001DC104); }
// 0x001DC104(this - 380, ...)
DECOMP(001DC1FC, AdjThunk_001DC1FC) { TAdjThunk(ctx, -380, 0x001DC104); }
// 0x001EA708(this - 384, ...)
DECOMP(001EA7D8, AdjThunk_001EA7D8) { TAdjThunk(ctx, -384, 0x001EA708); }
// 0x001EA708(this - 380, ...)
DECOMP(001EA800, AdjThunk_001EA800) { TAdjThunk(ctx, -380, 0x001EA708); }
// 0x001EB5E0(this - 384, ...)
DECOMP(001EB7C4, AdjThunk_001EB7C4) { TAdjThunk(ctx, -384, 0x001EB5E0); }
// 0x001EB5E0(this - 380, ...)
DECOMP(001EB7EC, AdjThunk_001EB7EC) { TAdjThunk(ctx, -380, 0x001EB5E0); }
// 0x001ED5F4(this - 384, ...)
DECOMP(001ED80C, AdjThunk_001ED80C) { TAdjThunk(ctx, -384, 0x001ED5F4); }
// 0x001ED5F4(this - 380, ...)
DECOMP(001ED834, AdjThunk_001ED834) { TAdjThunk(ctx, -380, 0x001ED5F4); }
// 0x001F6394(this - 384, ...)
DECOMP(001F6464, AdjThunk_001F6464) { TAdjThunk(ctx, -384, 0x001F6394); }
// 0x001F6394(this - 380, ...)
DECOMP(001F648C, AdjThunk_001F648C) { TAdjThunk(ctx, -380, 0x001F6394); }
// 0x001FD7D8(this - 380, ...)
DECOMP(001FDF54, AdjThunk_001FDF54) { TAdjThunk(ctx, -380, 0x001FD7D8); }
// 0x00208DE0(this - 380, ...)
DECOMP(0020920C, AdjThunk_0020920C) { TAdjThunk(ctx, -380, 0x00208DE0); }
// 0x002346A4(this - 392, ...)
DECOMP(00234990, AdjThunk_00234990) { TAdjThunk(ctx, -392, 0x002346A4); }
// 0x002346A4(this - 388, ...)
DECOMP(002349B8, AdjThunk_002349B8) { TAdjThunk(ctx, -388, 0x002346A4); }
// 0x0023D0C0(this - 388, ...)
DECOMP(0023D668, AdjThunk_0023D668) { TAdjThunk(ctx, -388, 0x0023D0C0); }
// 0x0023D690(this - 388, ...)
DECOMP(0023D7B0, AdjThunk_0023D7B0) { TAdjThunk(ctx, -388, 0x0023D690); }
// 0x00240E8C(this - 392, ...)
DECOMP(0024104C, AdjThunk_0024104C) { TAdjThunk(ctx, -392, 0x00240E8C); }
// 0x00240E8C(this - 388, ...)
DECOMP(00241074, AdjThunk_00241074) { TAdjThunk(ctx, -388, 0x00240E8C); }
// 0x00244630(this - 388, ...)
DECOMP(002447E8, AdjThunk_002447E8) { TAdjThunk(ctx, -388, 0x00244630); }
// 0x00248118(this - 388, ...)
DECOMP(002482A4, AdjThunk_002482A4) { TAdjThunk(ctx, -388, 0x00248118); }
// 0x0024DDD0(this - 388, ...)
DECOMP(0024DEC4, AdjThunk_0024DEC4) { TAdjThunk(ctx, -388, 0x0024DDD0); }
// 0x00269348(this - 24, ...)
DECOMP(0026966C, AdjThunk_0026966C) { TAdjThunk(ctx, -24, 0x00269348); }
// 0x0026E6C0(this - 24, ...)
DECOMP(0026EE88, AdjThunk_0026EE88) { TAdjThunk(ctx, -24, 0x0026E6C0); }
// 0x0026EEB4(this - 24, ...)
DECOMP(0026EFA4, AdjThunk_0026EFA4) { TAdjThunk(ctx, -24, 0x0026EEB4); }
// 0x0027E330(this - 80, ...)
DECOMP(0027E480, AdjThunk_0027E480) { TAdjThunk(ctx, -80, 0x0027E330); }
// 0x0027FAF0(this - 80, ...)
DECOMP(0027FC30, AdjThunk_0027FC30) { TAdjThunk(ctx, -80, 0x0027FAF0); }
// 0x002AD2A4(this - 24, ...)
DECOMP(002AD4CC, AdjThunk_002AD4CC) { TAdjThunk(ctx, -24, 0x002AD2A4); }
// 0x002AEC9C(this - 24, ...)
DECOMP(002AEE28, AdjThunk_002AEE28) { TAdjThunk(ctx, -24, 0x002AEC9C); }
// 0x002AEE58(this - 24, ...)
DECOMP(002AF09C, AdjThunk_002AF09C) { TAdjThunk(ctx, -24, 0x002AEE58); }
// 0x002AF3AC(this - 24, ...)
DECOMP(002AF5C8, AdjThunk_002AF5C8) { TAdjThunk(ctx, -24, 0x002AF3AC); }
// 0x002B17EC(this - 24, ...)
DECOMP(002B19E8, AdjThunk_002B19E8) { TAdjThunk(ctx, -24, 0x002B17EC); }
// 0x002B1A14(this - 24, ...)
DECOMP(002B1CE0, AdjThunk_002B1CE0) { TAdjThunk(ctx, -24, 0x002B1A14); }
// 0x002B2A70(this - 24, ...)
DECOMP(002B2CC4, AdjThunk_002B2CC4) { TAdjThunk(ctx, -24, 0x002B2A70); }
// 0x002B50D4(this - 24, ...)
DECOMP(002B5328, AdjThunk_002B5328) { TAdjThunk(ctx, -24, 0x002B50D4); }
// 0x002C7340(this - 24, ...)
DECOMP(002C9660, AdjThunk_002C9660) { TAdjThunk(ctx, -24, 0x002C7340); }
// 0x002C9910(this - 24, ...)
DECOMP(002C9CE4, AdjThunk_002C9CE4) { TAdjThunk(ctx, -24, 0x002C9910); }
// 0x002CC960(this - 24, ...)
DECOMP(002CCC7C, AdjThunk_002CCC7C) { TAdjThunk(ctx, -24, 0x002CC960); }
// 0x002CFC08(this - 24, ...)
DECOMP(002CFEF0, AdjThunk_002CFEF0) { TAdjThunk(ctx, -24, 0x002CFC08); }
// 0x002CFF18(this - 24, ...)
DECOMP(002D01F8, AdjThunk_002D01F8) { TAdjThunk(ctx, -24, 0x002CFF18); }
// 0x002D1034(this - 24, ...)
DECOMP(002D1310, AdjThunk_002D1310) { TAdjThunk(ctx, -24, 0x002D1034); }
// 0x002D2924(this - 24, ...)
DECOMP(002D2E9C, AdjThunk_002D2E9C) { TAdjThunk(ctx, -24, 0x002D2924); }
// 0x002D31B8(this - 24, ...)
DECOMP(002D33D0, AdjThunk_002D33D0) { TAdjThunk(ctx, -24, 0x002D31B8); }
// 0x002D4030(this - 24, ...)
DECOMP(002D4190, AdjThunk_002D4190) { TAdjThunk(ctx, -24, 0x002D4030); }
// 0x002FE63C(this - 384, ...)
DECOMP(002FE9C8, AdjThunk_002FE9C8) { TAdjThunk(ctx, -384, 0x002FE63C); }
// 0x002FE63C(this - 380, ...)
DECOMP(002FE9F0, AdjThunk_002FE9F0) { TAdjThunk(ctx, -380, 0x002FE63C); }
// 0x0030CB10(this - 384, ...)
DECOMP(0030CD6C, AdjThunk_0030CD6C) { TAdjThunk(ctx, -384, 0x0030CB10); }
// 0x0030CB10(this - 380, ...)
DECOMP(0030CD94, AdjThunk_0030CD94) { TAdjThunk(ctx, -380, 0x0030CB10); }
// 0x003A96F8(this - 56, ...)
DECOMP(003A9C1C, AdjThunk_003A9C1C) { TAdjThunk(ctx, -56, 0x003A96F8); }
// 0x003DA128(this - 56, ...)
DECOMP(003DA528, AdjThunk_003DA528) { TAdjThunk(ctx, -56, 0x003DA128); }
// 0x003DCEB8(this - 56, ...)
DECOMP(003DD204, AdjThunk_003DD204) { TAdjThunk(ctx, -56, 0x003DCEB8); }
// 0x003DE1C0(this - 56, ...)
DECOMP(003DE454, AdjThunk_003DE454) { TAdjThunk(ctx, -56, 0x003DE1C0); }
// 0x00436374(this - 12, ...)
DECOMP(004365BC, AdjThunk_004365BC) { TAdjThunk(ctx, -12, 0x00436374); }

// ---------------------------------------------------------------- factories: Mem_Alloc(size, 0, 16), then the constructor
// p = Mem_Alloc(80, 0, 16); if p: 0x0006CC68(p, a1); returns p
DECOMP(0006D004, New_0006D004) { TNew1(ctx, 80, 0x0006CC68); }
// p = Mem_Alloc(128, 0, 16); if p: 0x0006D488(p, a1); returns p
DECOMP(0006D7C8, New_0006D7C8) { TNew1(ctx, 128, 0x0006D488); }
// p = Mem_Alloc(80, 0, 16); if p: 0x0006DDB8(p, a1); returns p
DECOMP(0006E14C, New_0006E14C) { TNew1(ctx, 80, 0x0006DDB8); }
// p = Mem_Alloc(44, 0, 16); if p: 0x0006E63C(p, a1); returns p
DECOMP(0006E870, New_0006E870) { TNew1(ctx, 44, 0x0006E63C); }
// p = Mem_Alloc(112, 0, 16); if p: 0x0006F094(p, a1); returns p
DECOMP(0006F388, New_0006F388) { TNew1(ctx, 112, 0x0006F094); }
// p = Mem_Alloc(64, 0, 16); if p: 0x0006F7C8(p, a1); returns p
DECOMP(0006F9D0, New_0006F9D0) { TNew1(ctx, 64, 0x0006F7C8); }
// p = Mem_Alloc(576, 0, 16); if p: 0x00070390(p, a1); returns p
DECOMP(00070B48, New_00070B48) { TNew1(ctx, 576, 0x00070390); }
// p = Mem_Alloc(48, 0, 16); if p: 0x00075DD4(p, a1); returns p
DECOMP(000760E0, New_000760E0) { TNew1(ctx, 48, 0x00075DD4); }
// p = Mem_Alloc(144, 0, 16); if p: 0x000780D0(p, a1); returns p
DECOMP(000784C8, New_000784C8) { TNew1(ctx, 144, 0x000780D0); }
// p = Mem_Alloc(148, 0, 16); if p: 0x00078F04(p, a1); returns p
DECOMP(0007951C, New_0007951C) { TNew1(ctx, 148, 0x00078F04); }
// p = Mem_Alloc(208, 0, 16); if p: 0x00079938(p, a1); returns p
DECOMP(00079F0C, New_00079F0C) { TNew1(ctx, 208, 0x00079938); }
// p = Mem_Alloc(44, 0, 16); if p: 0x0007DA1C(p, a1); returns p
DECOMP(0007DEA8, New_0007DEA8) { TNew1(ctx, 44, 0x0007DA1C); }
// p = Mem_Alloc(52, 0, 16); if p: 0x0007E110(p, a1); returns p
DECOMP(0007E5EC, New_0007E5EC) { TNew1(ctx, 52, 0x0007E110); }
// p = Mem_Alloc(48, 0, 16); if p: 0x0007E944(p, a1); returns p
DECOMP(0007EDB4, New_0007EDB4) { TNew1(ctx, 48, 0x0007E944); }
// p = Mem_Alloc(44, 0, 16); if p: 0x0007EFE4(p, a1); returns p
DECOMP(0007F47C, New_0007F47C) { TNew1(ctx, 44, 0x0007EFE4); }
// p = Mem_Alloc(48, 0, 16); if p: 0x0007F670(p, a1); returns p
DECOMP(0007F8FC, New_0007F8FC) { TNew1(ctx, 48, 0x0007F670); }
// p = Mem_Alloc(24, 0, 16); if p: 0x00080340(p, a1); returns p
DECOMP(000804DC, New_000804DC) { TNew1(ctx, 24, 0x00080340); }
// p = Mem_Alloc(56, 0, 16); if p: 0x00080D68(p, a1); returns p
DECOMP(00080F54, New_00080F54) { TNew1(ctx, 56, 0x00080D68); }
// p = Mem_Alloc(88, 0, 16); if p: 0x000820E0(p, a1); returns p
DECOMP(000825BC, New_000825BC) { TNew1(ctx, 88, 0x000820E0); }
// p = Mem_Alloc(96, 0, 16); if p: 0x000838F4(p, a1); returns p
DECOMP(00083D54, New_00083D54) { TNew1(ctx, 96, 0x000838F4); }
// p = Mem_Alloc(112, 0, 16); if p: 0x00088704(p, a1); returns p
DECOMP(00088A4C, New_00088A4C) { TNew1(ctx, 112, 0x00088704); }
// p = Mem_Alloc(64, 0, 16); if p: 0x000892D4(p, a1); returns p
DECOMP(00089608, New_00089608) { TNew1(ctx, 64, 0x000892D4); }
// p = Mem_Alloc(32, 0, 16); if p: 0x0008A880(p, a1); returns p
DECOMP(0008AA00, New_0008AA00) { TNew1(ctx, 32, 0x0008A880); }
// p = Mem_Alloc(24, 0, 16); if p: 0x0008C72C(p, a1); returns p
DECOMP(0008C834, New_0008C834) { TNew1(ctx, 24, 0x0008C72C); }
// p = Mem_Alloc(240, 0, 16); if p: 0x0008DE98(p, a1); returns p
DECOMP(0008E614, New_0008E614) { TNew1(ctx, 240, 0x0008DE98); }
// p = Mem_Alloc(32, 0, 16); if p: 0x00091178(p, a1); returns p
DECOMP(00091698, New_00091698) { TNew1(ctx, 32, 0x00091178); }
// p = Mem_Alloc(28, 0, 16); if p: 0x00091D70(p, a1); returns p
DECOMP(00091FA4, New_00091FA4) { TNew1(ctx, 28, 0x00091D70); }
// p = Mem_Alloc(192, 0, 16); if p: 0x00093150(p, a1); returns p
DECOMP(000935B4, New_000935B4) { TNew1(ctx, 192, 0x00093150); }
// p = Mem_Alloc(96, 0, 16); if p: 0x00093D50(p, a1); returns p
DECOMP(000942EC, New_000942EC) { TNew1(ctx, 96, 0x00093D50); }
// p = Mem_Alloc(176, 0, 16); if p: 0x00095270(p, a1); returns p
DECOMP(00095508, New_00095508) { TNew1(ctx, 176, 0x00095270); }
// p = Mem_Alloc(44, 0, 16); if p: 0x00097850(p, a1); returns p
DECOMP(00097AB4, New_00097AB4) { TNew1(ctx, 44, 0x00097850); }
// p = Mem_Alloc(156, 0, 16); if p: 0x0009829C(p, a1); returns p
DECOMP(0009884C, New_0009884C) { TNew1(ctx, 156, 0x0009829C); }
// p = Mem_Alloc(64, 0, 16); if p: 0x00098BE4(p, a1); returns p
DECOMP(00098E34, New_00098E34) { TNew1(ctx, 64, 0x00098BE4); }
// p = Mem_Alloc(144, 0, 16); if p: 0x0009A670(p, a1); returns p
DECOMP(0009A9B0, New_0009A9B0) { TNew1(ctx, 144, 0x0009A670); }
// p = Mem_Alloc(128, 0, 16); if p: 0x0009B3DC(p, a1); returns p
DECOMP(0009B698, New_0009B698) { TNew1(ctx, 128, 0x0009B3DC); }
// p = Mem_Alloc(52, 0, 16); if p: 0x0009D850(p, a1); returns p
DECOMP(0009DA78, New_0009DA78) { TNew1(ctx, 52, 0x0009D850); }
// p = Mem_Alloc(144, 0, 16); if p: 0x0009DEE8(p, a1); returns p
DECOMP(0009E2E0, New_0009E2E0) { TNew1(ctx, 144, 0x0009DEE8); }
// p = Mem_Alloc(24, 0, 16); if p: 0x0009E528(p, a1); returns p
DECOMP(0009E69C, New_0009E69C) { TNew1(ctx, 24, 0x0009E528); }
// p = Mem_Alloc(52, 0, 16); if p: 0x0009FF90(p, a1); returns p
DECOMP(000A0200, New_000A0200) { TNew1(ctx, 52, 0x0009FF90); }
// p = Mem_Alloc(208, 0, 16); if p: 0x000A0558(p, a1); returns p
DECOMP(000A0AAC, New_000A0AAC) { TNew1(ctx, 208, 0x000A0558); }
// p = Mem_Alloc(192, 0, 16); if p: 0x000A1224(p, a1); returns p
DECOMP(000A1650, New_000A1650) { TNew1(ctx, 192, 0x000A1224); }
// p = Mem_Alloc(564, 0, 16); if p: 0x000A1D34(p, a1); returns p
DECOMP(000A2028, New_000A2028) { TNew1(ctx, 564, 0x000A1D34); }
// p = Mem_Alloc(136, 0, 16); if p: 0x000A2568(p, a1); returns p
DECOMP(000A2850, New_000A2850) { TNew1(ctx, 136, 0x000A2568); }
// p = Mem_Alloc(56, 0, 16); if p: 0x000A3108(p, a1); returns p
DECOMP(000A33C4, New_000A33C4) { TNew1(ctx, 56, 0x000A3108); }
// p = Mem_Alloc(44, 0, 16); if p: 0x000A48F0(p, a1); returns p
DECOMP(000A4B48, New_000A4B48) { TNew1(ctx, 44, 0x000A48F0); }
// p = Mem_Alloc(80, 0, 16); if p: 0x000A5AA8(p, a1); returns p
DECOMP(000A5CE4, New_000A5CE4) { TNew1(ctx, 80, 0x000A5AA8); }
// p = Mem_Alloc(160, 0, 16); if p: 0x000A6478(p, a1); returns p
DECOMP(000A6774, New_000A6774) { TNew1(ctx, 160, 0x000A6478); }
// p = Mem_Alloc(32, 0, 16); if p: 0x000A7244(p, a1); returns p
DECOMP(000A7394, New_000A7394) { TNew1(ctx, 32, 0x000A7244); }
// p = Mem_Alloc(44, 0, 16); if p: 0x004C0E00(p, a1); returns p
DECOMP(004C1230, New_004C1230) { TNew1(ctx, 44, 0x004C0E00); }
// p = Mem_Alloc(44, 0, 16); if p: 0x004C1FE4(p, a1); returns p
DECOMP(004C2324, New_004C2324) { TNew1(ctx, 44, 0x004C1FE4); }
// p = Mem_Alloc(76, 0, 16); if p: 0x004C30C8(p, a1); returns p
DECOMP(004C3658, New_004C3658) { TNew1(ctx, 76, 0x004C30C8); }
// p = Mem_Alloc(176, 0, 16); if p: 0x004C4708(p, a1); returns p
DECOMP(004C4D9C, New_004C4D9C) { TNew1(ctx, 176, 0x004C4708); }
// p = Mem_Alloc(64, 0, 16); if p: 0x004C5DF0(p, a1); returns p
DECOMP(004C6474, New_004C6474) { TNew1(ctx, 64, 0x004C5DF0); }
// p = Mem_Alloc(52, 0, 16); if p: 0x004C6778(p, a1); returns p
DECOMP(004C6D90, New_004C6D90) { TNew1(ctx, 52, 0x004C6778); }
// p = Mem_Alloc(132, 0, 16); if p: 0x004C6FB8(p, a1); returns p
DECOMP(004C74CC, New_004C74CC) { TNew1(ctx, 132, 0x004C6FB8); }
// p = Mem_Alloc(64, 0, 16); if p: 0x004C8360(p, a1); returns p
DECOMP(004C87C0, New_004C87C0) { TNew1(ctx, 64, 0x004C8360); }
// p = Mem_Alloc(52, 0, 16); if p: 0x004C96E4(p, a1); returns p
DECOMP(004C9928, New_004C9928) { TNew1(ctx, 52, 0x004C96E4); }
// p = Mem_Alloc(28, 0, 16); if p: 0x004CA888(p, a1); returns p
DECOMP(004CAA3C, New_004CAA3C) { TNew1(ctx, 28, 0x004CA888); }
// p = Mem_Alloc(24, 0, 16); if p: 0x004CAC7C(p, a1); returns p
DECOMP(004CAE48, New_004CAE48) { TNew1(ctx, 24, 0x004CAC7C); }
// p = Mem_Alloc(80, 0, 16); if p: 0x004CB5F8(p, a1); returns p
DECOMP(004CB7D8, New_004CB7D8) { TNew1(ctx, 80, 0x004CB5F8); }
// p = Mem_Alloc(52, 0, 16); if p: 0x004CBB40(p, a1); returns p
DECOMP(004CBE40, New_004CBE40) { TNew1(ctx, 52, 0x004CBB40); }
// p = Mem_Alloc(192, 0, 16); if p: 0x004CCF28(p, a1); returns p
DECOMP(004CDC94, New_004CDC94) { TNew1(ctx, 192, 0x004CCF28); }

// ---------------------------------------------------------------- trivial destructors: only the vtable
// *this = vtable 0x00B0DAEC
DECOMP(00112C58, VtB0DAEC_DtorTrivial_00112C58) { TSetVtbl(ctx, 0x00B0DAEC); }
// *this = vtable 0x00B145E0
DECOMP(0016CD1C, VtB145E0_DtorTrivial) { TSetVtbl(ctx, 0x00B145E0); }
// *this = vtable 0x00B145E0
DECOMP(0016CD84, VtB145E0_DtorTrivial_0016CD84) { TSetVtbl(ctx, 0x00B145E0); }
// *this = vtable 0x00B145E0
DECOMP(0016D0F8, VtB145E0_DtorTrivial_0016D0F8) { TSetVtbl(ctx, 0x00B145E0); }
// *this = vtable 0x00B145E0
DECOMP(0016D3FC, VtB145E0_DtorTrivial_0016D3FC) { TSetVtbl(ctx, 0x00B145E0); }
// *this = vtable 0x00B145E0
DECOMP(0016D6BC, VtB145E0_DtorTrivial_0016D6BC) { TSetVtbl(ctx, 0x00B145E0); }
// *this = vtable 0x00B1D600
DECOMP(002F969C, VtB1D600_DtorTrivial_002F969C) { TSetVtbl(ctx, 0x00B1D600); }
// *this = vtable 0x00B20CC8
DECOMP(0042C9FC, VtB20CC8_DtorTrivial) { TSetVtbl(ctx, 0x00B20CC8); }
// *this = vtable 0x00B20CC8
DECOMP(0042CA50, VtB20CC8_DtorTrivial_0042CA50) { TSetVtbl(ctx, 0x00B20CC8); }
// *this = vtable 0x00B21B58
DECOMP(004A091C, VtB21B58_DtorTrivial) { TSetVtbl(ctx, 0x00B21B58); }
// *this = vtable 0x00B21B58
DECOMP(004A0CA4, VtB21B58_DtorTrivial_004A0CA4) { TSetVtbl(ctx, 0x00B21B58); }
// *this = vtable 0x00B21B58
DECOMP(004A0E08, VtB21B58_DtorTrivial_004A0E08) { TSetVtbl(ctx, 0x00B21B58); }
// *this = vtable 0x00B21B58
DECOMP(004A0F70, VtB21B58_DtorTrivial_004A0F70) { TSetVtbl(ctx, 0x00B21B58); }
// *this = vtable 0x00B11B20
DECOMP(004B6884, VtB11B20_DtorTrivial_004B6884) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004BB3C8, VtB11B20_DtorTrivial_004BB3C8) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004BB41C, VtB11B20_DtorTrivial_004BB41C) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004BE9BC, VtB11B20_DtorTrivial_004BE9BC) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004BEB6C, VtB11B20_DtorTrivial_004BEB6C) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004BED64, VtB11B20_DtorTrivial_004BED64) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004BF454, VtB11B20_DtorTrivial_004BF454) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004BF650, VtB11B20_DtorTrivial_004BF650) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004BF8E4, VtB11B20_DtorTrivial_004BF8E4) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004C0A44, VtB11B20_DtorTrivial_004C0A44) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004C1CB8, VtB11B20_DtorTrivial_004C1CB8) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004C3604, VtB11B20_DtorTrivial_004C3604) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004C3DC0, VtB11B20_DtorTrivial_004C3DC0) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004C4354, VtB11B20_DtorTrivial_004C4354) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004C4538, VtB11B20_DtorTrivial_004C4538) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004C4D48, VtB11B20_DtorTrivial_004C4D48) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004C5BD0, VtB11B20_DtorTrivial_004C5BD0) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004C98D4, VtB11B20_DtorTrivial_004C98D4) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CA4E0, VtB11B20_DtorTrivial_004CA4E0) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CA9E8, VtB11B20_DtorTrivial_004CA9E8) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CADF4, VtB11B20_DtorTrivial_004CADF4) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CB094, VtB11B20_DtorTrivial_004CB094) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CB784, VtB11B20_DtorTrivial_004CB784) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CC2D4, VtB11B20_DtorTrivial_004CC2D4) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CC510, VtB11B20_DtorTrivial_004CC510) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CC754, VtB11B20_DtorTrivial_004CC754) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CCA9C, VtB11B20_DtorTrivial_004CCA9C) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(004CDC34, VtB11B20_DtorTrivial_004CDC34) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B23080
DECOMP(004F94E0, VtB23080_DtorTrivial_004F94E0) { TSetVtbl(ctx, 0x00B23080); }

// ---------------------------------------------------------------- deleting destructors without a base destructor
// deleting: vtable 0x00B0DAEC, 0x00552420(this, 112)
DECOMP(00112C68, VtB0DAEC_Delete_00112C68) { TDeleteNoBase(ctx, 0x00B0DAEC, 0x00552420, 112); }
// deleting: vtable 0x00B145E0, 0x0054EE6C(this, 0)
DECOMP(0016CD2C, VtB145E0_Delete) { TDeleteNoBase(ctx, 0x00B145E0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B145E0, 0x0054EE6C(this, 0)
DECOMP(0016CD94, VtB145E0_Delete_0016CD94) { TDeleteNoBase(ctx, 0x00B145E0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B145E0, 0x0054EE6C(this, 0)
DECOMP(0016D108, VtB145E0_Delete_0016D108) { TDeleteNoBase(ctx, 0x00B145E0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B145E0, 0x0054EE6C(this, 0)
DECOMP(0016D40C, VtB145E0_Delete_0016D40C) { TDeleteNoBase(ctx, 0x00B145E0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B145E0, 0x0054EE6C(this, 0)
DECOMP(0016D6CC, VtB145E0_Delete_0016D6CC) { TDeleteNoBase(ctx, 0x00B145E0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D600, 0x0054EE6C(this, 0)
DECOMP(002F96AC, VtB1D600_Delete_002F96AC) { TDeleteNoBase(ctx, 0x00B1D600, 0x0054EE6C, 0); }
// deleting: vtable 0x00B20CC8, 0x0054EE6C(this, 0)
DECOMP(0042CA0C, VtB20CC8_Delete) { TDeleteNoBase(ctx, 0x00B20CC8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21B58, 0x0054EE6C(this, 0)
DECOMP(004A092C, VtB21B58_Delete) { TDeleteNoBase(ctx, 0x00B21B58, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21B58, 0x0054EE6C(this, 0)
DECOMP(004A0CB4, VtB21B58_Delete_004A0CB4) { TDeleteNoBase(ctx, 0x00B21B58, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21B58, 0x0054EE6C(this, 0)
DECOMP(004A0E18, VtB21B58_Delete_004A0E18) { TDeleteNoBase(ctx, 0x00B21B58, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21B58, 0x0054EE6C(this, 0)
DECOMP(004A0F80, VtB21B58_Delete_004A0F80) { TDeleteNoBase(ctx, 0x00B21B58, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004BB3D8, VtB11B20_Delete_004BB3D8) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x00552420(this, 24)
DECOMP(004BE9CC, VtB11B20_Delete_004BE9CC) { TDeleteNoBase(ctx, 0x00B11B20, 0x00552420, 24); }
// deleting: vtable 0x00B11B20, 0x00552420(this, 24)
DECOMP(004BEB7C, VtB11B20_Delete_004BEB7C) { TDeleteNoBase(ctx, 0x00B11B20, 0x00552420, 24); }
// deleting: vtable 0x00B11B20, 0x00552420(this, 24)
DECOMP(004BED74, VtB11B20_Delete_004BED74) { TDeleteNoBase(ctx, 0x00B11B20, 0x00552420, 24); }
// deleting: vtable 0x00B11B20, 0x00552420(this, 48)
DECOMP(004BF464, VtB11B20_Delete_004BF464) { TDeleteNoBase(ctx, 0x00B11B20, 0x00552420, 48); }
// deleting: vtable 0x00B11B20, 0x00552420(this, 96)
DECOMP(004BF660, VtB11B20_Delete_004BF660) { TDeleteNoBase(ctx, 0x00B11B20, 0x00552420, 96); }
// deleting: vtable 0x00B11B20, 0x00552420(this, 28)
DECOMP(004BF8F4, VtB11B20_Delete_004BF8F4) { TDeleteNoBase(ctx, 0x00B11B20, 0x00552420, 28); }
// deleting: vtable 0x00B11B20, 0x00552420(this, 28)
DECOMP(004C0A54, VtB11B20_Delete_004C0A54) { TDeleteNoBase(ctx, 0x00B11B20, 0x00552420, 28); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004C1CC8, VtB11B20_Delete_004C1CC8) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004C3614, VtB11B20_Delete_004C3614) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004C3DD0, VtB11B20_Delete_004C3DD0) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004C4364, VtB11B20_Delete_004C4364) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004C4548, VtB11B20_Delete_004C4548) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004C4D58, VtB11B20_Delete_004C4D58) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004C5BE0, VtB11B20_Delete_004C5BE0) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004C98E4, VtB11B20_Delete_004C98E4) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CA4F0, VtB11B20_Delete_004CA4F0) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CA9F8, VtB11B20_Delete_004CA9F8) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CAE04, VtB11B20_Delete_004CAE04) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CB0A4, VtB11B20_Delete_004CB0A4) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CB794, VtB11B20_Delete_004CB794) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CC2E4, VtB11B20_Delete_004CC2E4) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CC520, VtB11B20_Delete_004CC520) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CC764, VtB11B20_Delete_004CC764) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CCAAC, VtB11B20_Delete_004CCAAC) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(004CDC44, VtB11B20_Delete_004CDC44) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B23080, 0x00552420(this, 32)
DECOMP(004F94F0, VtB23080_Delete_004F94F0) { TDeleteNoBase(ctx, 0x00B23080, 0x00552420, 32); }

// ---------------------------------------------------------------- deleting destructors entered through a secondary base
// deleting via this -384: 0x000EB0F8(p), 0x0054EE6C(p, 0)
DECOMP(000EB1F8, AdjDelete_000EB1F8) { TAdjDeleteCall(ctx, -384, 0x000EB0F8, 0x0054EE6C, 0); }
// deleting via this -380: 0x000EB0F8(p), 0x0054EE6C(p, 0)
DECOMP(000EB244, AdjDelete_000EB244) { TAdjDeleteCall(ctx, -380, 0x000EB0F8, 0x0054EE6C, 0); }
// deleting via this -12: 0x001002C4(p), 0x00552420(p, 1040)
DECOMP(001004B8, AdjDelete_001004B8) { TAdjDeleteCall(ctx, -12, 0x001002C4, 0x00552420, 1040); }
// deleting via this -384: 0x0016EC4C(p), 0x00552420(p, 800)
DECOMP(0016EE68, AdjDelete_0016EE68) { TAdjDeleteCall(ctx, -384, 0x0016EC4C, 0x00552420, 800); }
// deleting via this -380: 0x0016EC4C(p), 0x00552420(p, 800)
DECOMP(0016EEB4, AdjDelete_0016EEB4) { TAdjDeleteCall(ctx, -380, 0x0016EC4C, 0x00552420, 800); }
// deleting via this -384: 0x00184280(p), 0x00552420(p, 1728)
DECOMP(00184748, AdjDelete_00184748) { TAdjDeleteCall(ctx, -384, 0x00184280, 0x00552420, 1728); }
// deleting via this -380: 0x00184280(p), 0x00552420(p, 1728)
DECOMP(00184794, AdjDelete_00184794) { TAdjDeleteCall(ctx, -380, 0x00184280, 0x00552420, 1728); }
// deleting via this -384: 0x00191E54(p), 0x00552420(p, 704)
DECOMP(00192034, AdjDelete_00192034) { TAdjDeleteCall(ctx, -384, 0x00191E54, 0x00552420, 704); }
// deleting via this -380: 0x00191E54(p), 0x00552420(p, 704)
DECOMP(00192080, AdjDelete_00192080) { TAdjDeleteCall(ctx, -380, 0x00191E54, 0x00552420, 704); }
// deleting via this -384: 0x001AE468(p), 0x0054EE6C(p, 0)
DECOMP(001AE5BC, AdjDelete_001AE5BC) { TAdjDeleteCall(ctx, -384, 0x001AE468, 0x0054EE6C, 0); }
// deleting via this -380: 0x001AE468(p), 0x0054EE6C(p, 0)
DECOMP(001AE608, AdjDelete_001AE608) { TAdjDeleteCall(ctx, -380, 0x001AE468, 0x0054EE6C, 0); }
// deleting via this -384: 0x001EB5E0(p), 0x00552420(p, 512)
DECOMP(001EB72C, AdjDelete_001EB72C) { TAdjDeleteCall(ctx, -384, 0x001EB5E0, 0x00552420, 512); }
// deleting via this -380: 0x001EB5E0(p), 0x00552420(p, 512)
DECOMP(001EB778, AdjDelete_001EB778) { TAdjDeleteCall(ctx, -380, 0x001EB5E0, 0x00552420, 512); }
// deleting via this -384: 0x001ED5F4(p), 0x00552420(p, 992)
DECOMP(001ED774, AdjDelete_001ED774) { TAdjDeleteCall(ctx, -384, 0x001ED5F4, 0x00552420, 992); }
// deleting via this -380: 0x001ED5F4(p), 0x00552420(p, 992)
DECOMP(001ED7C0, AdjDelete_001ED7C0) { TAdjDeleteCall(ctx, -380, 0x001ED5F4, 0x00552420, 992); }
// deleting via this -392: 0x002346A4(p), 0x00552420(p, 1104)
DECOMP(002348F8, AdjDelete_002348F8) { TAdjDeleteCall(ctx, -392, 0x002346A4, 0x00552420, 1104); }
// deleting via this -388: 0x002346A4(p), 0x00552420(p, 1104)
DECOMP(00234944, AdjDelete_00234944) { TAdjDeleteCall(ctx, -388, 0x002346A4, 0x00552420, 1104); }
// deleting via this -392: 0x00240E8C(p), 0x00552420(p, 640)
DECOMP(00240FB4, AdjDelete_00240FB4) { TAdjDeleteCall(ctx, -392, 0x00240E8C, 0x00552420, 640); }
// deleting via this -388: 0x00240E8C(p), 0x00552420(p, 640)
DECOMP(00241000, AdjDelete_00241000) { TAdjDeleteCall(ctx, -388, 0x00240E8C, 0x00552420, 640); }
// deleting via this -388: 0x00248118(p), 0x00552420(p, 1440)
DECOMP(00248258, AdjDelete_00248258) { TAdjDeleteCall(ctx, -388, 0x00248118, 0x00552420, 1440); }
// deleting via this -24: 0x00269348(p), 0x00552420(p, 720)
DECOMP(00269620, AdjDelete_00269620) { TAdjDeleteCall(ctx, -24, 0x00269348, 0x00552420, 720); }
// deleting via this -24: 0x002AD2A4(p), 0x00552420(p, 352)
DECOMP(002AD480, AdjDelete_002AD480) { TAdjDeleteCall(ctx, -24, 0x002AD2A4, 0x00552420, 352); }
// deleting via this -24: 0x002AF3AC(p), 0x00552420(p, 144)
DECOMP(002AF57C, AdjDelete_002AF57C) { TAdjDeleteCall(ctx, -24, 0x002AF3AC, 0x00552420, 144); }
// deleting via this -24: 0x002B17EC(p), 0x00552420(p, 44)
DECOMP(002B199C, AdjDelete_002B199C) { TAdjDeleteCall(ctx, -24, 0x002B17EC, 0x00552420, 44); }
// deleting via this -24: 0x002B2A70(p), 0x00552420(p, 144)
DECOMP(002B2C78, AdjDelete_002B2C78) { TAdjDeleteCall(ctx, -24, 0x002B2A70, 0x00552420, 144); }
// deleting via this -24: 0x002B50D4(p), 0x00552420(p, 144)
DECOMP(002B52DC, AdjDelete_002B52DC) { TAdjDeleteCall(ctx, -24, 0x002B50D4, 0x00552420, 144); }
// deleting via this -24: 0x002C9910(p), 0x00552420(p, 720)
DECOMP(002C9C98, AdjDelete_002C9C98) { TAdjDeleteCall(ctx, -24, 0x002C9910, 0x00552420, 720); }
// deleting via this -24: 0x002CC960(p), 0x00552420(p, 384)
DECOMP(002CCC30, AdjDelete_002CCC30) { TAdjDeleteCall(ctx, -24, 0x002CC960, 0x00552420, 384); }
// deleting via this -24: 0x002D1034(p), 0x00552420(p, 80)
DECOMP(002D12C4, AdjDelete_002D12C4) { TAdjDeleteCall(ctx, -24, 0x002D1034, 0x00552420, 80); }
// deleting via this -24: 0x002D31B8(p), 0x00552420(p, 96)
DECOMP(002D3384, AdjDelete_002D3384) { TAdjDeleteCall(ctx, -24, 0x002D31B8, 0x00552420, 96); }
// deleting via this -384: 0x002FE63C(p), 0x00552420(p, 1136)
DECOMP(002FE930, AdjDelete_002FE930) { TAdjDeleteCall(ctx, -384, 0x002FE63C, 0x00552420, 1136); }
// deleting via this -380: 0x002FE63C(p), 0x00552420(p, 1136)
DECOMP(002FE97C, AdjDelete_002FE97C) { TAdjDeleteCall(ctx, -380, 0x002FE63C, 0x00552420, 1136); }
// deleting via this -384: 0x0030CB10(p), 0x00552420(p, 1088)
DECOMP(0030CCD4, AdjDelete_0030CCD4) { TAdjDeleteCall(ctx, -384, 0x0030CB10, 0x00552420, 1088); }
// deleting via this -380: 0x0030CB10(p), 0x00552420(p, 1088)
DECOMP(0030CD20, AdjDelete_0030CD20) { TAdjDeleteCall(ctx, -380, 0x0030CB10, 0x00552420, 1088); }
// deleting via this -56: 0x003DCEB8(p), 0x00552420(p, 384)
DECOMP(003DD1B8, AdjDelete_003DD1B8) { TAdjDeleteCall(ctx, -56, 0x003DCEB8, 0x00552420, 384); }
// deleting via this -12: 0x00436374(p), 0x00552420(p, 160)
DECOMP(00436570, AdjDelete_00436570) { TAdjDeleteCall(ctx, -12, 0x00436374, 0x00552420, 160); }

// ---------------------------------------------------------------- wrappers
// wrapper of 0x000179C0 (the same arguments)
DECOMP(00017F64, Thunk_00017F64) { TThunk(ctx, 0x000179C0); }
// wrapper of 0x000137C4 (the same arguments)
DECOMP(00042D3C, Thunk_00042D3C) { TThunk(ctx, 0x000137C4); }
// wrapper of 0x000137C4 (the same arguments)
DECOMP(000575C4, Thunk_000575C4) { TThunk(ctx, 0x000137C4); }
// wrapper of 0x00033F10 (the same arguments)
DECOMP(00065404, Thunk_00065404) { TThunk(ctx, 0x00033F10); }
// wrapper of 0x000C26D8 (the same arguments)
DECOMP(000C2B90, Thunk_000C2B90) { TThunk(ctx, 0x000C26D8); }
// wrapper of 0x000C2E8C (the same arguments)
DECOMP(000C3078, Thunk_000C3078) { TThunk(ctx, 0x000C2E8C); }
// wrapper of 0x000D3578 (the same arguments)
DECOMP(000D3B94, Thunk_000D3B94) { TThunk(ctx, 0x000D3578); }
// wrapper of 0x000D7024 (the same arguments)
DECOMP(000D729C, Thunk_000D729C) { TThunk(ctx, 0x000D7024); }
// wrapper of 0x000E01C0 (the same arguments)
DECOMP(000E02D8, Thunk_000E02D8) { TThunk(ctx, 0x000E01C0); }
// wrapper of 0x000C79C0 (the same arguments)
DECOMP(000E27FC, Thunk_000E27FC) { TThunk(ctx, 0x000C79C0); }
// wrapper of 0x000E281C (the same arguments)
DECOMP(000E2A34, Thunk_000E2A34) { TThunk(ctx, 0x000E281C); }
// wrapper of 0x000E38F0 (the same arguments)
DECOMP(000E40B4, Thunk_000E40B4) { TThunk(ctx, 0x000E38F0); }
// wrapper of 0x000DFBFC (the same arguments)
DECOMP(000E44C0, Thunk_000E44C0) { TThunk(ctx, 0x000DFBFC); }
// wrapper of 0x000EC5B8 (the same arguments)
DECOMP(000F43DC, Thunk_000F43DC) { TThunk(ctx, 0x000EC5B8); }
// wrapper of 0x000EF564 (the same arguments)
DECOMP(000F4418, Thunk_000F4418) { TThunk(ctx, 0x000EF564); }
// wrapper of 0x0011A608 (the same arguments)
DECOMP(0011BBA4, Thunk_0011BBA4) { TThunk(ctx, 0x0011A608); }
// wrapper of 0x00120A58 (the same arguments)
DECOMP(00121BB8, Thunk_00121BB8) { TThunk(ctx, 0x00120A58); }
// wrapper of 0x00121BDC (the same arguments)
DECOMP(00121E6C, Thunk_00121E6C) { TThunk(ctx, 0x00121BDC); }
// wrapper of 0x001F4098 (the same arguments)
DECOMP(001F4440, Thunk_001F4440) { TThunk(ctx, 0x001F4098); }
// wrapper of 0x002025B0 (the same arguments)
DECOMP(002027B0, Thunk_002027B0) { TThunk(ctx, 0x002025B0); }
// wrapper of 0x0020EBC8 (the same arguments)
DECOMP(00212D58, Thunk_00212D58) { TThunk(ctx, 0x0020EBC8); }
// wrapper of 0x0021E3F8 (the same arguments)
DECOMP(0021E5F4, Thunk_0021E5F4) { TThunk(ctx, 0x0021E3F8); }
// wrapper of 0x00275110 (the same arguments)
DECOMP(00291694, Thunk_00291694) { TThunk(ctx, 0x00275110); }
// wrapper of 0x002A9190 (the same arguments)
DECOMP(002AB7EC, Thunk_002AB7EC) { TThunk(ctx, 0x002A9190); }
// wrapper of 0x00311A60 (the same arguments)
DECOMP(003120F8, Thunk_003120F8) { TThunk(ctx, 0x00311A60); }
// wrapper of 0x003266E4 (the same arguments)
DECOMP(00326A38, Thunk_00326A38) { TThunk(ctx, 0x003266E4); }
// wrapper of 0x0036BCAC (the same arguments)
DECOMP(00389C3C, Thunk_00389C3C) { TThunk(ctx, 0x0036BCAC); }
// wrapper of 0x00427A0C (the same arguments)
DECOMP(00427BD0, Thunk_00427BD0) { TThunk(ctx, 0x00427A0C); }
// wrapper of 0x004551C8 (the same arguments)
DECOMP(00455954, Thunk_00455954) { TThunk(ctx, 0x004551C8); }
// wrapper of 0x004B1114 (the same arguments)
DECOMP(004B1308, Thunk_004B1308) { TThunk(ctx, 0x004B1114); }
// wrapper of 0x004F3CB4 (the same arguments)
DECOMP(004F41AC, Thunk_004F41AC) { TThunk(ctx, 0x004F3CB4); }
// wrapper of 0x004F6F54 (the same arguments)
DECOMP(004F7108, Thunk_004F7108) { TThunk(ctx, 0x004F6F54); }
// wrapper of 0x004F75B4 (the same arguments)
DECOMP(004F77F8, Thunk_004F77F8) { TThunk(ctx, 0x004F75B4); }
// wrapper of 0x004FD008 (the same arguments)
DECOMP(004FD198, Obj_DestructBase_004FD198) { TThunk(ctx, 0x004FD008); }
// wrapper of 0x004FE990 (the same arguments)
DECOMP(004FEBD0, Shader_Find) { TThunk(ctx, 0x004FE990); }

// ---------------------------------------------------------------- float getters: this->+12[current index]
// f1 = ((f32*)this->+12)[(*(0x00F948EC +16))->+64]
DECOMP(00734290, GetF32ByIndex_00734290) { TF32ByIndex(ctx, 0x00F948EC); }
// f1 = ((f32*)this->+12)[(*(0x00F94904 +16))->+64]
DECOMP(007342B0, GetF32ByIndex_007342B0) { TF32ByIndex(ctx, 0x00F94904); }
// f1 = ((f32*)this->+12)[(*(0x00F94D9C +16))->+64]
DECOMP(007349C0, GetF32ByIndex_007349C0) { TF32ByIndex(ctx, 0x00F94D9C); }
// f1 = ((f32*)this->+12)[(*(0x00F950CC +16))->+64]
DECOMP(00734A20, GetF32ByIndex_00734A20) { TF32ByIndex(ctx, 0x00F950CC); }
// f1 = ((f32*)this->+12)[(*(0x00F94E74 +16))->+64]
DECOMP(00734A40, GetF32ByIndex_00734A40) { TF32ByIndex(ctx, 0x00F94E74); }
// f1 = ((f32*)this->+12)[(*(0x00F94F34 +16))->+64]
DECOMP(00734A60, GetF32ByIndex_00734A60) { TF32ByIndex(ctx, 0x00F94F34); }
// f1 = ((f32*)this->+12)[(*(0x00F94F4C +16))->+64]
DECOMP(00734BF8, GetF32ByIndex_00734BF8) { TF32ByIndex(ctx, 0x00F94F4C); }
// f1 = ((f32*)this->+12)[(*(0x00F94D84 +16))->+64]
DECOMP(00734C18, GetF32ByIndex_00734C18) { TF32ByIndex(ctx, 0x00F94D84); }
// f1 = ((f32*)this->+12)[(*(0x00F94D6C +16))->+64]
DECOMP(00734DD8, GetF32ByIndex_00734DD8) { TF32ByIndex(ctx, 0x00F94D6C); }
// f1 = ((f32*)this->+12)[(*(0x00F94C1C +16))->+64]
DECOMP(00734F80, GetF32ByIndex_00734F80) { TF32ByIndex(ctx, 0x00F94C1C); }
// f1 = ((f32*)this->+12)[(*(0x00F94C34 +16))->+64]
DECOMP(00734FA0, GetF32ByIndex_00734FA0) { TF32ByIndex(ctx, 0x00F94C34); }
// f1 = ((f32*)this->+12)[(*(0x00F94DB4 +16))->+64]
DECOMP(00735070, GetF32ByIndex_00735070) { TF32ByIndex(ctx, 0x00F94DB4); }
// f1 = ((f32*)this->+12)[(*(0x00F94DCC +16))->+64]
DECOMP(00735090, GetF32ByIndex_00735090) { TF32ByIndex(ctx, 0x00F94DCC); }
// f1 = ((f32*)this->+12)[(*(0x00F94DE4 +16))->+64]
DECOMP(007350B0, GetF32ByIndex_007350B0) { TF32ByIndex(ctx, 0x00F94DE4); }
// f1 = ((f32*)this->+12)[(*(0x00F94DFC +16))->+64]
DECOMP(007350D0, GetF32ByIndex_007350D0) { TF32ByIndex(ctx, 0x00F94DFC); }
// f1 = ((f32*)this->+12)[(*(0x00F94E14 +16))->+64]
DECOMP(007350F0, GetF32ByIndex_007350F0) { TF32ByIndex(ctx, 0x00F94E14); }
// f1 = ((f32*)this->+12)[(*(0x00F94E2C +16))->+64]
DECOMP(00735110, GetF32ByIndex_00735110) { TF32ByIndex(ctx, 0x00F94E2C); }
// f1 = ((f32*)this->+12)[(*(0x00F94E44 +16))->+64]
DECOMP(00735130, GetF32ByIndex_00735130) { TF32ByIndex(ctx, 0x00F94E44); }
// f1 = ((f32*)this->+12)[(*(0x00F94E5C +16))->+64]
DECOMP(00735150, GetF32ByIndex_00735150) { TF32ByIndex(ctx, 0x00F94E5C); }
// f1 = ((f32*)this->+12)[(*(0x00F94E8C +16))->+64]
DECOMP(00735170, GetF32ByIndex_00735170) { TF32ByIndex(ctx, 0x00F94E8C); }
// f1 = ((f32*)this->+12)[(*(0x00F94EA4 +16))->+64]
DECOMP(00735190, GetF32ByIndex_00735190) { TF32ByIndex(ctx, 0x00F94EA4); }
// f1 = ((f32*)this->+12)[(*(0x00F94EBC +16))->+64]
DECOMP(007351B0, GetF32ByIndex_007351B0) { TF32ByIndex(ctx, 0x00F94EBC); }
// f1 = ((f32*)this->+12)[(*(0x00F94ED4 +16))->+64]
DECOMP(007351D0, GetF32ByIndex_007351D0) { TF32ByIndex(ctx, 0x00F94ED4); }
// f1 = ((f32*)this->+12)[(*(0x00F94EEC +16))->+64]
DECOMP(007351F0, GetF32ByIndex_007351F0) { TF32ByIndex(ctx, 0x00F94EEC); }
// f1 = ((f32*)this->+12)[(*(0x00F94F04 +16))->+64]
DECOMP(00735210, GetF32ByIndex_00735210) { TF32ByIndex(ctx, 0x00F94F04); }
// f1 = ((f32*)this->+12)[(*(0x00F94F1C +16))->+64]
DECOMP(00735230, GetF32ByIndex_00735230) { TF32ByIndex(ctx, 0x00F94F1C); }
// f1 = ((f32*)this->+12)[(*(0x00F94BD4 +16))->+64]
DECOMP(00735250, GetF32ByIndex_00735250) { TF32ByIndex(ctx, 0x00F94BD4); }
// f1 = ((f32*)this->+12)[(*(0x00F94BBC +16))->+64]
DECOMP(007352B0, GetF32ByIndex_007352B0) { TF32ByIndex(ctx, 0x00F94BBC); }

// ---------------------------------------------------------------- destructors: the vtable, then the base destructor (b)
// *this = vtable 0x00B12550, then 0x000D7024(this)
DECOMP(000DF8B4, VtB12550_Dtor) { TDtor(ctx, 0x00B12550, 0x000D7024); }
// *this = vtable 0x00B12550, then 0x000D7024(this)
DECOMP(000DF934, VtB12550_Dtor_000DF934) { TDtor(ctx, 0x00B12550, 0x000D7024); }
// *this = vtable 0x00B130B8, then 0x000E9820(this)
DECOMP(000F23A0, VtB130B8_Dtor) { TDtor(ctx, 0x00B130B8, 0x000E9820); }
// *this = vtable 0x00B132D8, then 0x000E9820(this)
DECOMP(000F3934, VtB132D8_Dtor) { TDtor(ctx, 0x00B132D8, 0x000E9820); }
// *this = vtable 0x00B134F8, then 0x000E9820(this)
DECOMP(000F4900, VtB134F8_Dtor) { TDtor(ctx, 0x00B134F8, 0x000E9820); }
// *this = vtable 0x00B1AD20, then 0x0021F088(this)
DECOMP(00221D44, VtB1AD20_Dtor) { TDtor(ctx, 0x00B1AD20, 0x0021F088); }
// *this = vtable 0x00B1E458, then 0x003286D8(this)
DECOMP(0032A328, VtB1E458_Dtor) { TDtor(ctx, 0x00B1E458, 0x003286D8); }
// *this = vtable 0x00B1E4D0, then 0x003286D8(this)
DECOMP(0032A4D0, VtB1E4D0_Dtor) { TDtor(ctx, 0x00B1E4D0, 0x003286D8); }
// *this = vtable 0x00B1E918, then 0x003286D8(this)
DECOMP(0033070C, VtB1E918_Dtor) { TDtor(ctx, 0x00B1E918, 0x003286D8); }
// *this = vtable 0x00B1E990, then 0x003286D8(this)
DECOMP(00330BF4, VtB1E990_Dtor) { TDtor(ctx, 0x00B1E990, 0x003286D8); }
// *this = vtable 0x00B1E4D0, then 0x003286D8(this)
DECOMP(003352C0, VtB1E4D0_Dtor_003352C0) { TDtor(ctx, 0x00B1E4D0, 0x003286D8); }
// *this = vtable 0x00B1E4D0, then 0x003286D8(this)
DECOMP(003357C8, VtB1E4D0_Dtor_003357C8) { TDtor(ctx, 0x00B1E4D0, 0x003286D8); }
// *this = vtable 0x00B1E4D0, then 0x003286D8(this)
DECOMP(00335C34, VtB1E4D0_Dtor_00335C34) { TDtor(ctx, 0x00B1E4D0, 0x003286D8); }
// *this = vtable 0x00B1E4D0, then 0x003286D8(this)
DECOMP(00336180, VtB1E4D0_Dtor_00336180) { TDtor(ctx, 0x00B1E4D0, 0x003286D8); }
// *this = vtable 0x00B1E4D0, then 0x003286D8(this)
DECOMP(00338D58, VtB1E4D0_Dtor_00338D58) { TDtor(ctx, 0x00B1E4D0, 0x003286D8); }
// *this = vtable 0x00B20398, then 0x003AAD78(this)
DECOMP(003ACDCC, VtB20398_Dtor) { TDtor(ctx, 0x00B20398, 0x003AAD78); }
// *this = vtable 0x00B203E0, then 0x003AAD78(this)
DECOMP(003AEBC0, VtB203E0_Dtor) { TDtor(ctx, 0x00B203E0, 0x003AAD78); }
// *this = vtable 0x00B21C80, then 0x004B7DF0(this)
DECOMP(004B9C38, VtB21C80_Dtor) { TDtor(ctx, 0x00B21C80, 0x004B7DF0); }
// *this = vtable 0x00B21CC0, then 0x004B7DF0(this)
DECOMP(004BB430, VtB21CC0_Dtor) { TDtor(ctx, 0x00B21CC0, 0x004B7DF0); }
// *this = vtable 0x00B12550, then 0x000D7024(this)
DECOMP(00747504, VtB12550_Dtor_00747504) { TDtor(ctx, 0x00B12550, 0x000D7024); }
// *this = vtable 0x00B1A8B0, then 0x0021F088(this)
DECOMP(0075B7EC, VtB1A8B0_Dtor) { TDtor(ctx, 0x00B1A8B0, 0x0021F088); }
// *this = vtable 0x00B1E2F0, then 0x003266E4(this)
DECOMP(00769B58, VtB1E2F0_Dtor) { TDtor(ctx, 0x00B1E2F0, 0x003266E4); }
// *this = vtable 0x00B1E4D0, then 0x003286D8(this)
DECOMP(0076B374, VtB1E4D0_Dtor_0076B374) { TDtor(ctx, 0x00B1E4D0, 0x003286D8); }
// *this = vtable 0x00B1F6E0, then 0x0036ABF8(this)
DECOMP(00772A3C, VtB1F6E0_Dtor) { TDtor(ctx, 0x00B1F6E0, 0x0036ABF8); }
// *this = vtable 0x00B1FA48, then 0x0036ABF8(this)
DECOMP(00773724, VtB1FA48_Dtor) { TDtor(ctx, 0x00B1FA48, 0x0036ABF8); }

// ---------------------------------------------------------------- deleting destructors (b)
// deleting: vtable 0x00B12550, 0x000D7024(this), 0x0054EE6C(this, 0)
DECOMP(000DF8E0, VtB12550_Delete) { TDelete(ctx, 0x00B12550, 0x000D7024, 0x0054EE6C, 0); }
// deleting: vtable 0x00B130B8, 0x000E9820(this), 0x00552420(this, 448)
DECOMP(000F23CC, VtB130B8_Delete) { TDelete(ctx, 0x00B130B8, 0x000E9820, 0x00552420, 448); }
// deleting: vtable 0x00B132D8, 0x000E9820(this), 0x00552420(this, 432)
DECOMP(000F3960, VtB132D8_Delete) { TDelete(ctx, 0x00B132D8, 0x000E9820, 0x00552420, 432); }
// deleting: vtable 0x00B134F8, 0x000E9820(this), 0x00552420(this, 448)
DECOMP(000F492C, VtB134F8_Delete) { TDelete(ctx, 0x00B134F8, 0x000E9820, 0x00552420, 448); }
// deleting: vtable 0x00B1AD20, 0x0021F088(this), 0x00552420(this, 480)
DECOMP(00221D70, VtB1AD20_Delete) { TDelete(ctx, 0x00B1AD20, 0x0021F088, 0x00552420, 480); }
// deleting: vtable 0x00B1E458, 0x003286D8(this), 0x00552420(this, 2580)
DECOMP(0032A354, VtB1E458_Delete) { TDelete(ctx, 0x00B1E458, 0x003286D8, 0x00552420, 2580); }
// deleting: vtable 0x00B1E4D0, 0x003286D8(this), 0x00552420(this, 2672)
DECOMP(0032A4FC, VtB1E4D0_Delete) { TDelete(ctx, 0x00B1E4D0, 0x003286D8, 0x00552420, 2672); }
// deleting: vtable 0x00B1E918, 0x003286D8(this), 0x00552420(this, 2592)
DECOMP(00330738, VtB1E918_Delete) { TDelete(ctx, 0x00B1E918, 0x003286D8, 0x00552420, 2592); }
// deleting: vtable 0x00B1E990, 0x003286D8(this), 0x00552420(this, 2912)
DECOMP(00330C20, VtB1E990_Delete) { TDelete(ctx, 0x00B1E990, 0x003286D8, 0x00552420, 2912); }
// deleting: vtable 0x00B1E4D0, 0x003286D8(this), 0x00552420(this, 2656)
DECOMP(003352EC, VtB1E4D0_Delete_003352EC) { TDelete(ctx, 0x00B1E4D0, 0x003286D8, 0x00552420, 2656); }
// deleting: vtable 0x00B1E4D0, 0x003286D8(this), 0x00552420(this, 2632)
DECOMP(003357F4, VtB1E4D0_Delete_003357F4) { TDelete(ctx, 0x00B1E4D0, 0x003286D8, 0x00552420, 2632); }
// deleting: vtable 0x00B1E4D0, 0x003286D8(this), 0x00552420(this, 2652)
DECOMP(00335C60, VtB1E4D0_Delete_00335C60) { TDelete(ctx, 0x00B1E4D0, 0x003286D8, 0x00552420, 2652); }
// deleting: vtable 0x00B1E4D0, 0x003286D8(this), 0x00552420(this, 2648)
DECOMP(003361AC, VtB1E4D0_Delete_003361AC) { TDelete(ctx, 0x00B1E4D0, 0x003286D8, 0x00552420, 2648); }
// deleting: vtable 0x00B1E4D0, 0x003286D8(this), 0x00552420(this, 2648)
DECOMP(00338D84, VtB1E4D0_Delete_00338D84) { TDelete(ctx, 0x00B1E4D0, 0x003286D8, 0x00552420, 2648); }
// deleting: vtable 0x00B20398, 0x003AAD78(this), 0x00552420(this, 184)
DECOMP(003ACDF8, VtB20398_Delete) { TDelete(ctx, 0x00B20398, 0x003AAD78, 0x00552420, 184); }
// deleting: vtable 0x00B203E0, 0x003AAD78(this), 0x00552420(this, 184)
DECOMP(003AEBEC, VtB203E0_Delete) { TDelete(ctx, 0x00B203E0, 0x003AAD78, 0x00552420, 184); }
// deleting: vtable 0x00B21C80, 0x004B7DF0(this), 0x00552420(this, 36)
DECOMP(004B9C64, VtB21C80_Delete) { TDelete(ctx, 0x00B21C80, 0x004B7DF0, 0x00552420, 36); }
// deleting: vtable 0x00B21CC0, 0x004B7DF0(this), 0x00552420(this, 52)
DECOMP(004BB45C, VtB21CC0_Delete) { TDelete(ctx, 0x00B21CC0, 0x004B7DF0, 0x00552420, 52); }
// deleting: vtable 0x00B12550, 0x000D7024(this), 0x0054EE6C(this, 0)
DECOMP(00747530, VtB12550_Delete_00747530) { TDelete(ctx, 0x00B12550, 0x000D7024, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1A8B0, 0x0021F088(this), 0x00552420(this, 480)
DECOMP(0075B818, VtB1A8B0_Delete) { TDelete(ctx, 0x00B1A8B0, 0x0021F088, 0x00552420, 480); }
// deleting: vtable 0x00B1E2F0, 0x003266E4(this), 0x00552420(this, 2592)
DECOMP(00769B84, VtB1E2F0_Delete) { TDelete(ctx, 0x00B1E2F0, 0x003266E4, 0x00552420, 2592); }
// deleting: vtable 0x00B1E4D0, 0x003286D8(this), 0x0054EE6C(this, 0)
DECOMP(0076B3A0, VtB1E4D0_Delete_0076B3A0) { TDelete(ctx, 0x00B1E4D0, 0x003286D8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F6E0, 0x0036ABF8(this), 0x00552420(this, 400)
DECOMP(00772A68, VtB1F6E0_Delete) { TDelete(ctx, 0x00B1F6E0, 0x0036ABF8, 0x00552420, 400); }
// deleting: vtable 0x00B1FA48, 0x0036ABF8(this), 0x00552420(this, 432)
DECOMP(00773750, VtB1FA48_Delete) { TDelete(ctx, 0x00B1FA48, 0x0036ABF8, 0x00552420, 432); }

// ---------------------------------------------------------------- constant returns (32-bit)
// returns 0x99B7AAF2
DECOMP(000F815C, ReturnK_000F815C) { TRetU32(ctx, 0x99B7AAF2); }
// returns 0xFFFFFFFF
DECOMP(00738440, ReturnK_00738440) { TRetU32(ctx, 0xFFFFFFFF); }
// returns 0x958CAC69
DECOMP(007388C8, ReturnK_007388C8) { TRetU32(ctx, 0x958CAC69); }
// returns 0xC9D7446A
DECOMP(007388D8, ReturnK_007388D8) { TRetU32(ctx, 0xC9D7446A); }
// returns 0xFFFFFFFF
DECOMP(00738B08, ReturnK_00738B08) { TRetU32(ctx, 0xFFFFFFFF); }
// returns 0x9F641DC8
DECOMP(00738E20, ReturnK_00738E20) { TRetU32(ctx, 0x9F641DC8); }
// returns 0xC9D7446A
DECOMP(00738E30, ReturnK_00738E30) { TRetU32(ctx, 0xC9D7446A); }
// returns 0xDC568581
DECOMP(00739144, ReturnK_00739144) { TRetU32(ctx, 0xDC568581); }
// returns 0xC9D7446A
DECOMP(00739154, ReturnK_00739154) { TRetU32(ctx, 0xC9D7446A); }
// returns 0xD3124ACD
DECOMP(007821C0, ReturnK_007821C0) { TRetU32(ctx, 0xD3124ACD); }
// returns 0xF066BEC9
DECOMP(007822F8, ReturnK_007822F8) { TRetU32(ctx, 0xF066BEC9); }
// returns 0xC9064726
DECOMP(007824F4, ReturnK_007824F4) { TRetU32(ctx, 0xC9064726); }
// returns 0xFF76E7C1
DECOMP(00782624, ReturnK_00782624) { TRetU32(ctx, 0xFF76E7C1); }
// returns 0xBC5EDCA1
DECOMP(00782888, ReturnK_00782888) { TRetU32(ctx, 0xBC5EDCA1); }
// returns 0xC9D7446A
DECOMP(00782904, ReturnK_00782904) { TRetU32(ctx, 0xC9D7446A); }
// returns 0xA2A8CB13
DECOMP(007829D0, ReturnK_007829D0) { TRetU32(ctx, 0xA2A8CB13); }
// returns 0xA296BC08
DECOMP(00782B08, ReturnK_00782B08) { TRetU32(ctx, 0xA296BC08); }
// returns 0xA7F58081
DECOMP(00782C40, ReturnK_00782C40) { TRetU32(ctx, 0xA7F58081); }
// returns 0xC6CD059D
DECOMP(00782D78, ReturnK_00782D78) { TRetU32(ctx, 0xC6CD059D); }
// returns 0xA296758E
DECOMP(00782EB0, ReturnK_00782EB0) { TRetU32(ctx, 0xA296758E); }
// returns 0xC0C6EED6
DECOMP(00782FE8, ReturnK_00782FE8) { TRetU32(ctx, 0xC0C6EED6); }
// returns 0xBBE4F197
DECOMP(00783120, ReturnK_00783120) { TRetU32(ctx, 0xBBE4F197); }
// returns 0xE51273D8
DECOMP(00783258, ReturnK_00783258) { TRetU32(ctx, 0xE51273D8); }

// ---------------------------------------------------------------- wrappers passing the second argument
// 0x000B56B0(a1)
DECOMP(000B5CF0, ThunkArg1_000B5CF0) { TThunkArg1(ctx, 0x000B56B0); }
// 0x001B3420(a1)
DECOMP(001B37C0, ThunkArg1_001B37C0) { TThunkArg1(ctx, 0x001B3420); }
// 0x001CEF00(a1)
DECOMP(001CF5BC, ThunkArg1_001CF5BC) { TThunkArg1(ctx, 0x001CEF00); }
// 0x001D3810(a1)
DECOMP(001D3944, ThunkArg1_001D3944) { TThunkArg1(ctx, 0x001D3810); }
// 0x001E4EDC(a1)
DECOMP(001E514C, ThunkArg1_001E514C) { TThunkArg1(ctx, 0x001E4EDC); }
// 0x00202180(a1)
DECOMP(00202588, ThunkArg1_00202588) { TThunkArg1(ctx, 0x00202180); }
// 0x0023D800(a1)
DECOMP(0023DF48, ThunkArg1_0023DF48) { TThunkArg1(ctx, 0x0023D800); }
// 0x00282638(a1)
DECOMP(00283618, ThunkArg1_00283618) { TThunkArg1(ctx, 0x00282638); }
// 0x00285D08(a1)
DECOMP(00286350, ThunkArg1_00286350) { TThunkArg1(ctx, 0x00285D08); }
// 0x00286BB8(a1)
DECOMP(00287988, ThunkArg1_00287988) { TThunkArg1(ctx, 0x00286BB8); }
// 0x0028EBB8(a1)
DECOMP(0028F054, ThunkArg1_0028F054) { TThunkArg1(ctx, 0x0028EBB8); }
// 0x002901E0(a1)
DECOMP(002908EC, ThunkArg1_002908EC) { TThunkArg1(ctx, 0x002901E0); }
// 0x002ABF18(a1)
DECOMP(002AC1B8, ThunkArg1_002AC1B8) { TThunkArg1(ctx, 0x002ABF18); }
// 0x002AC1E8(a1)
DECOMP(002AC7CC, ThunkArg1_002AC7CC) { TThunkArg1(ctx, 0x002AC1E8); }
// 0x002BF770(a1)
DECOMP(002C0D94, ThunkArg1_002C0D94) { TThunkArg1(ctx, 0x002BF770); }
// 0x002D2190(a1)
DECOMP(002D2900, ThunkArg1_002D2900) { TThunkArg1(ctx, 0x002D2190); }
// 0x002F4A38(a1)
DECOMP(002F5BE4, ThunkArg1_002F5BE4) { TThunkArg1(ctx, 0x002F4A38); }
// 0x00375C60(a1)
DECOMP(00376C4C, ThunkArg1_00376C4C) { TThunkArg1(ctx, 0x00375C60); }
// 0x00376598(a1)
DECOMP(00376C70, ThunkArg1_00376C70) { TThunkArg1(ctx, 0x00376598); }
// 0x003B7998(a1)
DECOMP(003B8314, ThunkArg1_003B8314) { TThunkArg1(ctx, 0x003B7998); }
// 0x003B7050(a1)
DECOMP(003B8338, ThunkArg1_003B8338) { TThunkArg1(ctx, 0x003B7050); }
// 0x003D33AC(a1)
DECOMP(003D35E8, ThunkArg1_003D35E8) { TThunkArg1(ctx, 0x003D33AC); }
// 0x003FEAB0(a1)
DECOMP(003FF82C, ThunkArg1_003FF82C) { TThunkArg1(ctx, 0x003FEAB0); }

// ---------------------------------------------------------------- wrappers: target(a1, this + delta)
// 0x0091A228(a1, this +380)
DECOMP(00748258, SwapAdj_00748258) { TSwapAdj(ctx, 380, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(00748288, SwapAdj_00748288) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +380)
DECOMP(0074EFA8, SwapAdj_0074EFA8) { TSwapAdj(ctx, 380, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(0074EFD8, SwapAdj_0074EFD8) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +380)
DECOMP(0074F194, SwapAdj_0074F194) { TSwapAdj(ctx, 380, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(0074F1C4, SwapAdj_0074F1C4) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +380)
DECOMP(0074F2F8, SwapAdj_0074F2F8) { TSwapAdj(ctx, 380, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(0074F328, SwapAdj_0074F328) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +380)
DECOMP(0074F484, SwapAdj_0074F484) { TSwapAdj(ctx, 380, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(0074F4B4, SwapAdj_0074F4B4) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +380)
DECOMP(0074F610, SwapAdj_0074F610) { TSwapAdj(ctx, 380, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(0074F640, SwapAdj_0074F640) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +380)
DECOMP(0075390C, SwapAdj_0075390C) { TSwapAdj(ctx, 380, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(0075393C, SwapAdj_0075393C) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +380)
DECOMP(00758648, SwapAdj_00758648) { TSwapAdj(ctx, 380, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(00758678, SwapAdj_00758678) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +380)
DECOMP(00759258, SwapAdj_00759258) { TSwapAdj(ctx, 380, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(00759288, SwapAdj_00759288) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +388)
DECOMP(0075D754, SwapAdj_0075D754) { TSwapAdj(ctx, 388, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(0075D784, SwapAdj_0075D784) { TSwapAdj(ctx, -4, 0x0091A228); }
// 0x0091A228(a1, this +388)
DECOMP(0075DAC8, SwapAdj_0075DAC8) { TSwapAdj(ctx, 388, 0x0091A228); }
// 0x0091A228(a1, this -4)
DECOMP(0075DAF8, SwapAdj_0075DAF8) { TSwapAdj(ctx, -4, 0x0091A228); }

// ---------------------------------------------------------------- free-list inits: FreeList_Init(this, item size, a1, a2, 0)
// FreeList_Init(this, 288, a1, a2, 0) (0x00551A58)
DECOMP(00736A74, FreeList_InitK_00736A74) { TInsertK(ctx, 0x00551A58, 288); }
// FreeList_Init(this, 128, a1, a2, 0) (0x00551A58)
DECOMP(00736AA8, FreeList_InitK_00736AA8) { TInsertK(ctx, 0x00551A58, 128); }
// FreeList_Init(this, 24, a1, a2, 0) (0x00551A58)
DECOMP(00736BA4, FreeList_InitK_00736BA4) { TInsertK(ctx, 0x00551A58, 24); }
// FreeList_Init(this, 152, a1, a2, 0) (0x00551A58)
DECOMP(00736CA0, FreeList_InitK_00736CA0) { TInsertK(ctx, 0x00551A58, 152); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(00736D9C, FreeList_InitK_00736D9C) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(00736EC8, FreeList_InitK_00736EC8) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 352, a1, a2, 0) (0x00551A58)
DECOMP(007370EC, FreeList_InitK_007370EC) { TInsertK(ctx, 0x00551A58, 352); }
// FreeList_Init(this, 144, a1, a2, 0) (0x00551A58)
DECOMP(0073B904, FreeList_InitK_0073B904) { TInsertK(ctx, 0x00551A58, 144); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(0073BA00, FreeList_InitK_0073BA00) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(0073BA34, FreeList_InitK_0073BA34) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 352, a1, a2, 0) (0x00551A58)
DECOMP(0073BCF0, FreeList_InitK_0073BCF0) { TInsertK(ctx, 0x00551A58, 352); }
// FreeList_Init(this, 288, a1, a2, 0) (0x00551A58)
DECOMP(00746CB4, FreeList_InitK_00746CB4) { TInsertK(ctx, 0x00551A58, 288); }
// FreeList_Init(this, 400, a1, a2, 0) (0x00551A58)
DECOMP(00746F18, FreeList_InitK_00746F18) { TInsertK(ctx, 0x00551A58, 400); }
// FreeList_Init(this, 192, a1, a2, 0) (0x00551A58)
DECOMP(0074908C, FreeList_InitK_0074908C) { TInsertK(ctx, 0x00551A58, 192); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(00770694, FreeList_InitK_00770694) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(0077D7F8, FreeList_InitK_0077D7F8) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(00793CE4, FreeList_InitK_00793CE4) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(00793D18, FreeList_InitK_00793D18) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(00793D5C, FreeList_InitK_00793D5C) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(00793D90, FreeList_InitK_00793D90) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(00799FA0, FreeList_InitK_00799FA0) { TInsertK(ctx, 0x00551A58, 20); }
// FreeList_Init(this, 20, a1, a2, 0) (0x00551A58)
DECOMP(00799FD4, FreeList_InitK_00799FD4) { TInsertK(ctx, 0x00551A58, 20); }

// ---------------------------------------------------------------- free-list destructors: two vtables, FreeList_Destroy after each
// *this = vtable 0x00B0EF68, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00736ADC, VtB0EF68_Dtor) { TDtor2(ctx, 0x00B0EF68, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B0EF88, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00736BD8, VtB0EF88_Dtor) { TDtor2(ctx, 0x00B0EF88, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B0EFA8, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00736CD4, VtB0EFA8_Dtor) { TDtor2(ctx, 0x00B0EFA8, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B0EFC8, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00736E00, VtB0EFC8_Dtor) { TDtor2(ctx, 0x00B0EFC8, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B0EFE8, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00736EFC, VtB0EFE8_Dtor) { TDtor2(ctx, 0x00B0EFE8, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B0F008, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00737024, VtB0F008_Dtor) { TDtor2(ctx, 0x00B0F008, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B0F028, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00737120, VtB0F028_Dtor) { TDtor2(ctx, 0x00B0F028, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B10E90, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(0073B938, VtB10E90_Dtor) { TDtor2(ctx, 0x00B10E90, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B10EB0, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(0073BA68, VtB10EB0_Dtor) { TDtor2(ctx, 0x00B10EB0, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B10ED0, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(0073BB30, VtB10ED0_Dtor) { TDtor2(ctx, 0x00B10ED0, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B10EF0, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(0073BC28, VtB10EF0_Dtor) { TDtor2(ctx, 0x00B10EF0, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B13900, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00746D88, VtB13900_Dtor) { TDtor2(ctx, 0x00B13900, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B13920, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00746E50, VtB13920_Dtor) { TDtor2(ctx, 0x00B13920, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B140D0, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(007492B4, VtB140D0_Dtor) { TDtor2(ctx, 0x00B140D0, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B1F648, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00770910, VtB1F648_Dtor) { TDtor2(ctx, 0x00B1F648, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B20C90, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(0077D82C, VtB20C90_Dtor) { TDtor2(ctx, 0x00B20C90, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B22510, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00794528, VtB22510_Dtor) { TDtor2(ctx, 0x00B22510, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B22530, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(007945F0, VtB22530_Dtor) { TDtor2(ctx, 0x00B22530, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B22550, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(007946B8, VtB22550_Dtor) { TDtor2(ctx, 0x00B22550, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B22570, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(00794810, VtB22570_Dtor) { TDtor2(ctx, 0x00B22570, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B22F88, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(0079A038, VtB22F88_Dtor) { TDtor2(ctx, 0x00B22F88, 0x005568FC, 0x00B24298, 0x005568FC); }
// *this = vtable 0x00B22FA8, 0x005568FC(this); *this = 0x00B24298, 0x005568FC(this)
DECOMP(0079A100, VtB22FA8_Dtor) { TDtor2(ctx, 0x00B22FA8, 0x005568FC, 0x00B24298, 0x005568FC); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B0EF68, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00736B30, VtB0EF68_Delete) { TDelete2(ctx, 0x00B0EF68, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0EF88, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00736C2C, VtB0EF88_Delete) { TDelete2(ctx, 0x00B0EF88, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0EFA8, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00736D28, VtB0EFA8_Delete) { TDelete2(ctx, 0x00B0EFA8, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0EFC8, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00736E54, VtB0EFC8_Delete) { TDelete2(ctx, 0x00B0EFC8, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0EFE8, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00736F50, VtB0EFE8_Delete) { TDelete2(ctx, 0x00B0EFE8, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F008, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00737078, VtB0F008_Delete) { TDelete2(ctx, 0x00B0F008, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F028, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00737174, VtB0F028_Delete) { TDelete2(ctx, 0x00B0F028, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10E90, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(0073B98C, VtB10E90_Delete) { TDelete2(ctx, 0x00B10E90, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10EB0, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(0073BABC, VtB10EB0_Delete) { TDelete2(ctx, 0x00B10EB0, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10ED0, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(0073BB84, VtB10ED0_Delete) { TDelete2(ctx, 0x00B10ED0, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10EF0, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(0073BC7C, VtB10EF0_Delete) { TDelete2(ctx, 0x00B10EF0, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B13900, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00746DDC, VtB13900_Delete) { TDelete2(ctx, 0x00B13900, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B13920, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00746EA4, VtB13920_Delete) { TDelete2(ctx, 0x00B13920, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B140D0, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00749308, VtB140D0_Delete) { TDelete2(ctx, 0x00B140D0, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F648, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00770964, VtB1F648_Delete) { TDelete2(ctx, 0x00B1F648, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B20C90, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(0077D880, VtB20C90_Delete) { TDelete2(ctx, 0x00B20C90, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22510, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(0079457C, VtB22510_Delete) { TDelete2(ctx, 0x00B22510, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22530, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00794644, VtB22530_Delete) { TDelete2(ctx, 0x00B22530, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22550, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(0079470C, VtB22550_Delete) { TDelete2(ctx, 0x00B22550, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22570, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(00794864, VtB22570_Delete) { TDelete2(ctx, 0x00B22570, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22F88, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(0079A08C, VtB22F88_Delete) { TDelete2(ctx, 0x00B22F88, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22FA8, 0x005568FC(this); 0x00B24298, 0x005568FC(this); 0x0054EE6C(this, 0)
DECOMP(0079A154, VtB22FA8_Delete) { TDelete2(ctx, 0x00B22FA8, 0x005568FC, 0x00B24298, 0x005568FC, 0x0054EE6C, 0); }

// ---------------------------------------------------------------- float constants
// f1 = *(f32*)0x0005F9E0
DECOMP(0005F9E8, ReturnF32_0005F9E8) { TRetF32At(ctx, 0x0005F9E0); }
// f1 = *(f32*)0x00738408
DECOMP(00738410, ReturnF32_00738410) { TRetF32At(ctx, 0x00738408); }
// f1 = *(f32*)0x00738420
DECOMP(00738428, ReturnF32_00738428) { TRetF32At(ctx, 0x00738420); }
// f1 = *(f32*)0x00738450
DECOMP(00738458, ReturnF32_00738458) { TRetF32At(ctx, 0x00738450); }
// f1 = *(f32*)0x00738468
DECOMP(00738470, ReturnF32_00738470) { TRetF32At(ctx, 0x00738468); }
// f1 = *(f32*)0x00738728
DECOMP(00738730, ReturnF32_00738730) { TRetF32At(ctx, 0x00738728); }
// f1 = *(f32*)0x00738760
DECOMP(00738768, ReturnF32_00738768) { TRetF32At(ctx, 0x00738760); }
// f1 = *(f32*)0x00738798
DECOMP(007387A0, ReturnF32_007387A0) { TRetF32At(ctx, 0x00738798); }
// f1 = *(f32*)0x00738AE8
DECOMP(00738AF0, ReturnF32_00738AF0) { TRetF32At(ctx, 0x00738AE8); }
// f1 = *(f32*)0x00738FA8
DECOMP(00738FB0, ReturnF32_00738FB0) { TRetF32At(ctx, 0x00738FA8); }
// f1 = *(f32*)0x007392A8
DECOMP(007392B0, ReturnF32_007392B0) { TRetF32At(ctx, 0x007392A8); }
// f1 = *(f32*)0x00742AA8
DECOMP(00742AB0, ReturnF32_00742AB0) { TRetF32At(ctx, 0x00742AA8); }
// f1 = *(f32*)0x00743638
DECOMP(00743640, ReturnF32_00743640) { TRetF32At(ctx, 0x00743638); }
// f1 = *(f32*)0x00743708
DECOMP(00743710, ReturnF32_00743710) { TRetF32At(ctx, 0x00743708); }
// f1 = *(f32*)0x00743858
DECOMP(00743860, ReturnF32_00743860) { TRetF32At(ctx, 0x00743858); }
// f1 = *(f32*)0x00743880
DECOMP(00743888, ReturnF32_00743888) { TRetF32At(ctx, 0x00743880); }
// f1 = *(f32*)0x00743898
DECOMP(007438A0, ReturnF32_007438A0) { TRetF32At(ctx, 0x00743898); }
// f1 = *(f32*)0x0075D678
DECOMP(0075D680, ReturnF32_0075D680) { TRetF32At(ctx, 0x0075D678); }
// f1 = *(f32*)0x00770388
DECOMP(00770390, ReturnF32_00770390) { TRetF32At(ctx, 0x00770388); }

// ---------------------------------------------------------------- wrappers narrowing a byte argument
// 0x000DD0A4(this, (u8)a1)
DECOMP(0012492C, ThunkU8_0012492C) { TThunkU8(ctx, 0x000DD0A4); }
// 0x000D74C0(this, (u8)a1)
DECOMP(001A7FE8, ThunkU8_001A7FE8) { TThunkU8(ctx, 0x000D74C0); }
// 0x000E4544(this, (u8)a1)
DECOMP(001AA5E0, ThunkU8_001AA5E0) { TThunkU8(ctx, 0x000E4544); }
// 0x000E4544(this, (u8)a1)
DECOMP(001ABAF8, ThunkU8_001ABAF8) { TThunkU8(ctx, 0x000E4544); }
// 0x000E4544(this, (u8)a1)
DECOMP(001ACA78, ThunkU8_001ACA78) { TThunkU8(ctx, 0x000E4544); }
// 0x000E4544(this, (u8)a1)
DECOMP(001B06CC, ThunkU8_001B06CC) { TThunkU8(ctx, 0x000E4544); }
// 0x000E4544(this, (u8)a1)
DECOMP(001B2A1C, ThunkU8_001B2A1C) { TThunkU8(ctx, 0x000E4544); }
// 0x000D74C0(this, (u8)a1)
DECOMP(001BBC10, ThunkU8_001BBC10) { TThunkU8(ctx, 0x000D74C0); }
// 0x000E4544(this, (u8)a1)
DECOMP(001EA9A8, ThunkU8_001EA9A8) { TThunkU8(ctx, 0x000E4544); }
