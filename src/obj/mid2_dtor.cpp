// Game code, medium sweep part 2: destructors.
//  - the level-section classes (base vtable 0x00B1E1F8, see
//    Obj_BaseDtor_003286D8 and obj20.h TDtorSection): derived classes with
//    PtrVector members, the base body inlined after them;
//  - the class with vtables 0x00B20220 / +56 / +60 / +64 (bases
//    Dtor_B219D0_00490B34 at +0 and Obj_BaseDtor_0048F6B0 at +64): its
//    destructor and deleting destructor, entered through each base.
#include "mid2.h"

using namespace sly4;
using namespace sly4::mid2;

namespace {

constexpr u32 kVtSection = 0x00B1E1F8;

// The inlined ~Section() (Obj_BaseDtor_003286D8 without the world-child
// release): vtable 0x00B1E1F8; the new[]'d array +2544 (8-byte slots,
// count at -4) cleared from the back and freed; the five lists +2232[i]
// (each {+0 a PtrVector of PtrVectors}) emptied and freed; +2524 / +2512
// / +2500 / +2488 freed and cleared; the vector +2272 deleted; then
// Obj_BaseDtor_0048F6B0. Returns its r3. inline_free: the array's
// Ptr_FreeAndClear_00321240 calls inlined (as some instances have them).
u64 SectionBaseDtor(PPUContext& ctx, u32 o, bool inline_free = false) {
    const u32 arr = load<u32>(o + 2544);
    store<u32>(o, kVtSection);
    if (arr != 0) {
        const u32 hdr = arr - 4;
        for (s32 k = static_cast<s32>(load<u32>(hdr)) - 1; k >= 0; --k) {
            const u32 pp = arr + 8 * static_cast<u32>(k);
            if (!inline_free) {
                call(ctx, m2fn::Ptr_FreeAndClear_00321240, {pp});
            } else if (const u32 p = load<u32>(pp); p != 0) {
                call(ctx, m2fn::Mem_FreeFrom, {p, 0});
                store<u32>(pp, 0);
            }
        }
        call(ctx, m2fn::Mem_FreeFrom, {hdr, 0});
        store<u32>(o + 2544, 0);
    }
    for (u32 i = 0; i < 5; ++i) {
        const u32 slot = o + 2232 + 8 * i;
        u32 l = load<u32>(slot);
        if (l == 0) continue;
        for (u32 j = 0;; ++j) {
            const u32 v = load<u32>(l);  // l == 0 (cleared meanwhile) reads address 0, as the original
            const u32 count = v != 0 ? load<u16>(v + 8) : 0u;
            if (!(j < count)) break;
            const u32 x = load<u32>(load<u32>(load<u32>(l)) + 4 * j);
            if (x != 0) {
                VecDtor(ctx, x);
                call(ctx, m2fn::Mem_FreeFrom, {x, 0});
                l = load<u32>(slot);
            }
        }
        if (l == 0) continue;
        mid1::PtrVecDeleteSmall(ctx, l);
        call(ctx, m2fn::Mem_FreeFrom, {l, 0});
        store<u32>(slot, 0);
    }
    for (const u32 off : {2524u, 2512u, 2500u, 2488u}) mid1::FreeSlot(ctx, o + off);
    mid1::PtrVecDeleteSmall(ctx, o + 2272);
    return call(ctx, m2fn::Obj_BaseDtor_0048F6B0, {o});
}

// ~T() of a section class with PtrVector members: vtable vt, the vectors
// destroyed in order, then the section base.
u64 SectionDtor(PPUContext& ctx, u32 o, u32 vt, std::initializer_list<u32> vecs) {
    store<u32>(o, vt);
    for (const u32 off : vecs) VecDtor(ctx, o + off);
    return SectionBaseDtor(ctx, o);
}

// The deleting form: nothing for a null this (r3 = 0), else ~T() and
// SmallFree(this, size).
void SectionDelete(PPUContext& ctx, u32 vt, std::initializer_list<u32> vecs, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    SectionDtor(ctx, o, vt, vecs);
    ret(ctx, call(ctx, m2fn::SmallFree, {o, size}));
}

// The inlined ~Array() of a 16-byte pool array {+0 u16 element size, +6
// u16 capacity << 1, +8 data, +12 allocator}: the data handed back through
// the allocator's vfn[2](allocator, data, bytes), then Array_Destruct.
void PoolArrayDtor(PPUContext& ctx, u32 a) {
    const u32 bytes = static_cast<u32>(load<u16>(a)) * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
    const u32 data = load<u32>(a + 8);
    if (data != 0) {
        const u32 al = load<u32>(a + 12);
        VCall(ctx, al, 8, {al, data, bytes});
    }
    call(ctx, m2fn::Array_Destruct, {a});
}

// ~T() of the 0x00B20218 class (448 bytes) at o: vtables 0x00B20220 (+0)
// / 0x00B2028C (+56) / 0x00B202B4 (+60) / 0x00B202D8 (+64); the block +256
// freed (SmallFree, +272 * 8 bytes); the PtrVectors +192 / +172 / +140,
// the pool array +116, the PtrVector +96 and the pool array +72
// destroyed; Obj_BaseDtor_0048F6B0(o + 64); the +60 / +56 vtables back to
// 0x00B12528 / 0x00B1200C; then Dtor_B219D0_00490B34(o).
u64 B20218Dtor(PPUContext& ctx, u32 o) {
    constexpr u32 vt = 0x00B20218;
    store<u32>(o + 0, vt + 8);
    store<u32>(o + 56, vt + 116);
    store<u32>(o + 60, vt + 156);
    store<u32>(o + 64, vt + 192);
    call(ctx, m2fn::SmallFree, {load<u32>(o + 256), (load<u32>(o + 272) << 3) & 0xFFFFFFF8u});
    VecDtor(ctx, o + 192);
    VecDtor(ctx, o + 172);
    VecDtor(ctx, o + 140);
    PoolArrayDtor(ctx, o + 116);
    VecDtor(ctx, o + 96);
    PoolArrayDtor(ctx, o + 72);
    call(ctx, m2fn::Obj_BaseDtor_0048F6B0, {o + 64});
    store<u32>(o + 60, 0x00B12528);
    store<u32>(o + 56, 0x00B1200C);
    return call(ctx, m2fn::Dtor_B219D0_00490B34, {o});
}

// The thunks: this points at the base at +adj (0, 56, 60 or 64).
void B20218DtorThunk(PPUContext& ctx, u32 adj) { ret(ctx, B20218Dtor(ctx, arg<u32>(ctx, 0) - adj)); }

// The deleting thunks: nothing when this - adj is null (r3 = this), else
// ~T() and SmallFree(o, 448).
void B20218Delete(PPUContext& ctx, u32 adj) {
    const u32 o = arg<u32>(ctx, 0) - adj;
    if (o == 0) return;
    B20218Dtor(ctx, o);
    ret(ctx, call(ctx, m2fn::SmallFree, {o, 448}));
}

}  // namespace

// ---------------------------------------------------------------- sections

// ~T() of the section classes with no members of their own (the
// 0x00B1E1F8 body inlined).
DECOMP(0032DA14, SectionBase_Dtor_0032DA14) { ret(ctx, SectionBaseDtor(ctx, arg<u32>(ctx, 0))); }
DECOMP(00331788, SectionBase_Dtor_00331788) { ret(ctx, SectionBaseDtor(ctx, arg<u32>(ctx, 0))); }
DECOMP(00332F44, SectionBase_Dtor_00332F44) { ret(ctx, SectionBaseDtor(ctx, arg<u32>(ctx, 0))); }

// ~T() of a member-less section class whose array frees are inlined.
DECOMP(00769480, SectionBase_Dtor_00769480) { ret(ctx, SectionBaseDtor(ctx, arg<u32>(ctx, 0), true)); }

// Its deleting form: nothing for a null this, else the destructor and
// Mem_FreeFrom(this, 0).
DECOMP(00769744, SectionBase_Delete_00769744) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    SectionBaseDtor(ctx, o, true);
    ret(ctx, call(ctx, m2fn::Mem_FreeFrom, {o, 0}));
}

// ~T() (vtable 0x00B1E6C0; Delete_0032D1EC frees 2608 bytes): the PtrVector +2588.
DECOMP(0032CF20, VtB1E6C0_Dtor) { ret(ctx, SectionDtor(ctx, arg<u32>(ctx, 0), 0x00B1E6C0, {2588})); }

// ~T() (vtable 0x00B1E828; Delete_0032F25C frees 2592 bytes): the PtrVector +2572.
DECOMP(0032EF90, VtB1E828_Dtor) { ret(ctx, SectionDtor(ctx, arg<u32>(ctx, 0), 0x00B1E828, {2572})); }

// ~T() (vtable 0x00B1E8A0; Delete_0032FD40 frees 2596 bytes): the PtrVector +2576.
DECOMP(0032FA74, VtB1E8A0_Dtor) { ret(ctx, SectionDtor(ctx, arg<u32>(ctx, 0), 0x00B1E8A0, {2576})); }

// ~T() (vtable 0x00B1EA80): the PtrVector +2572.
DECOMP(0076B458, VtB1EA80_Dtor) { ret(ctx, SectionDtor(ctx, arg<u32>(ctx, 0), 0x00B1EA80, {2572})); }

// The deleting ~T() of VtB1EA80 (2608 bytes).
DECOMP(0076B724, VtB1EA80_Delete) { SectionDelete(ctx, 0x00B1EA80, {2572}, 2608); }

// ~T() (vtable 0x00B1F048): the PtrVectors +2612 / +2592 / +2572.
DECOMP(0076B050, VtB1F048_Dtor) { ret(ctx, SectionDtor(ctx, arg<u32>(ctx, 0), 0x00B1F048, {2612, 2592, 2572})); }

// The deleting ~T() of VtB1F048 (2668 bytes).
DECOMP(0076BAD0, VtB1F048_Delete) { SectionDelete(ctx, 0x00B1F048, {2612, 2592, 2572}, 2668); }

// ~T() (vtable 0x00B1E5D0): Section_Prepare_0032AC78(this, 0, 1), the u16
// +2580 cleared, the PtrVector +2572, then the section base.
DECOMP(0032B02C, VtB1E5D0_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1E5D0);
    call(ctx, m2fn::Section_Prepare_0032AC78, {o, 0, 1, 0x00B1E5D0});  // r6 = the vtable, as left
    store<u16>(o + 2580, 0);
    VecDtor(ctx, o + 2572);
    ret(ctx, SectionBaseDtor(ctx, o));
}

// ---------------------------------------------------------------- VtB20220

// ~T() entered through the bases at +64 / +60 / +56.
DECOMP(0077E2FC, VtB20220_Dtor_Thunk64) { B20218DtorThunk(ctx, 64); }
DECOMP(0077E4DC, VtB20220_Dtor_Thunk60) { B20218DtorThunk(ctx, 60); }
DECOMP(0077E6BC, VtB20220_Dtor_Thunk56) { B20218DtorThunk(ctx, 56); }

// The deleting ~T() (448 bytes), directly and through the bases at +64 /
// +60 / +56.
DECOMP(0077DB10, VtB20220_Delete) { B20218Delete(ctx, 0); }
DECOMP(0077DD08, VtB20220_Delete_Thunk64) { B20218Delete(ctx, 64); }
DECOMP(0077DF04, VtB20220_Delete_Thunk60) { B20218Delete(ctx, 60); }
DECOMP(0077E100, VtB20220_Delete_Thunk56) { B20218Delete(ctx, 56); }
