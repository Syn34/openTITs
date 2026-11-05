// World objects, part 20: more template instances found by the
// project-wide survey (0x00010000-0x005E0000 and 0x00730000-0x007A0000) of
// functions whose normalised disassembly differs only in constants. Each
// instance is one DECOMP line calling the shared body in obj20.h with the
// constants scanned from its own disassembly; every one was checked against
// the original in the PPC simulator.
#include "obj20.h"

using namespace sly4;
using namespace sly4::obj20;

// ---------------------------------------------------------------- destructors of the volume-shape classes
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(0020D338, VtB19558_Dtor) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(00212AA0, VtB19558_Dtor_00212AA0) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(00215C00, VtB19558_Dtor_00215C00) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(00218BF0, VtB19558_Dtor_00218BF0) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(00219C88, VtB19558_Dtor_00219C88) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(0021AF48, VtB19558_Dtor_0021AF48) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(0021DB80, VtB19558_Dtor_0021DB80) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(00220C68, VtB19558_Dtor_00220C68) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(00221598, VtB19558_Dtor_00221598) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(00759E74, VtB19558_Dtor_00759E74) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(0075A110, VtB19558_Dtor_0075A110) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }
// ~T(): vtable 0x00B19558, 0x000112D8 / 0x00638498 on +424, arrays +388 / +364, vector +344, 0x000D729C(this)
DECOMP(0075BB14, VtB19558_Dtor_0075BB14) { TDtorVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C); }

// ---------------------------------------------------------------- AVL tree inserts
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F0E0, rotations 0x00739F1C / 0x00739DD4
DECOMP(0073AFC4, Avl_Insert_0073AFC4) { TAvlInsert(ctx, 0x0073AFC4, 0x00F8F0E0, 0x00B10EB0, 0x00AC47E8, 0x00739F1C, 0x00739DD4); }
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F0E8, rotations 0x0076BFD4 / 0x0076BE8C
DECOMP(0076D07C, Avl_Insert_0076D07C) { TAvlInsert(ctx, 0x0076D07C, 0x00F8F0E8, 0x00B1F648, 0x00ADBDC9, 0x0076BFD4, 0x0076BE8C); }
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F0E4, rotations 0x007855FC / 0x007854B4
DECOMP(00785744, AvlCmd_InsertAt) { TAvlInsert(ctx, 0x00785744, 0x00F8F0E4, 0x00B10E90, 0x00AE553A, 0x007855FC, 0x007854B4); }
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F0D8, rotations 0x00785EA0 / 0x00785D58
DECOMP(00785FE8, AvlVar_InsertAt) { TAvlInsert(ctx, 0x00785FE8, 0x00F8F0D8, 0x00B0EFA8, 0x00AE53EC, 0x00785EA0, 0x00785D58); }
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F100, rotations 0x0078E410 / 0x0078E2C8
DECOMP(0078E558, Avl_Insert_0078E558) { TAvlInsert(ctx, 0x0078E558, 0x00F8F100, 0x00B22550, 0x00AEECC9, 0x0078E410, 0x0078E2C8); }
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F0F4, rotations 0x0078C2C0 / 0x0078C178
DECOMP(0078EB6C, Avl_Insert_0078EB6C) { TAvlInsert(ctx, 0x0078EB6C, 0x00F8F0F4, 0x00B22510, 0x00AEEB72, 0x0078C2C0, 0x0078C178); }
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F0F8, rotations 0x0078A170 / 0x0078A028
DECOMP(007918E4, Avl_Insert_007918E4) { TAvlInsert(ctx, 0x007918E4, 0x00F8F0F8, 0x00B22570, 0x00AEEF23, 0x0078A170, 0x0078A028); }
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F0DC, rotations 0x00792040 / 0x00791EF8
DECOMP(007930E8, Avl_Insert_007930E8) { TAvlInsert(ctx, 0x007930E8, 0x00F8F0DC, 0x00B0EFC8, 0x00AEEE29, 0x00792040, 0x00791EF8); }
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F104, rotations 0x00796BC8 / 0x00796A80
DECOMP(00798E60, Avl_Insert_00798E60) { TAvlInsert(ctx, 0x00798E60, 0x00F8F104, 0x00B22F88, 0x00AEFA70, 0x00796BC8, 0x00796A80); }
// (tree, u32 key, value, Node** pp): recursive insert, leaves from the free list at 0x00F8F108, rotations 0x00797DB8 / 0x00797C70
DECOMP(007995FC, Avl_Insert_007995FC) { TAvlInsert(ctx, 0x007995FC, 0x00F8F108, 0x00B22FA8, 0x00AEFB23, 0x00797DB8, 0x00797C70); }

// ---------------------------------------------------------------- component transforms through a virtual (the VMX matrix build)
// (out, obj): obj+268 component's +16 slot 20 with the matrix at 0x01164BD0, else 0x000CB934(out, obj)
DECOMP(00167370, Comp_Xform_00167370) { TXformVCall(ctx, 268, 0x000CB934, 0x01164BD0, 0x00167340); }
// (out, obj): obj+244 component's +16 slot 20 with the matrix at 0x01164BD0, else 0x000CB934(out, obj)
DECOMP(0016B050, Comp_Xform_0016B050) { TXformVCall(ctx, 244, 0x000CB934, 0x01164BD0, 0x0016B020); }
// (out, obj): obj+252 component's +16 slot 20 with the matrix at 0x01164BD0, else 0x000CB934(out, obj)
DECOMP(00176E80, Comp_Xform_00176E80) { TXformVCall(ctx, 252, 0x000CB934, 0x01164BD0, 0x00176E50); }
// (out, obj): obj+288 component's +16 slot 20 with the matrix at 0x01164BD0, else 0x000CB934(out, obj)
DECOMP(0017D940, Comp_Xform_0017D940) { TXformVCall(ctx, 288, 0x000CB934, 0x01164BD0, 0x0017D910); }
// (out, obj): obj+292 component's +16 slot 20 with the matrix at 0x01164BD0, else 0x000CB934(out, obj)
DECOMP(0018FE60, Comp_Xform_0018FE60) { TXformVCall(ctx, 292, 0x000CB934, 0x01164BD0, 0x0018FE30); }
// (out, obj): obj+228 component's +16 slot 20 with the matrix at 0x01164BD0, else 0x000CB934(out, obj)
DECOMP(001B4650, Comp_Xform_001B4650) { TXformVCall(ctx, 228, 0x000CB934, 0x01164BD0, 0x001B4620); }
// (out, obj): obj+296 component's +16 slot 20 with the matrix at 0x01164BD0, else 0x000CB934(out, obj)
DECOMP(001F9D70, Comp_Xform_001F9D70) { TXformVCall(ctx, 296, 0x000CB934, 0x01164BD0, 0x001F9D40); }
// (out, obj): obj+200 component's +16 slot 20 with the matrix at 0x01164BD0, else 0x000CB934(out, obj)
DECOMP(00203E70, Comp_Xform_00203E70) { TXformVCall(ctx, 200, 0x000CB934, 0x01164BD0, 0x00203E40); }
// (out, obj): obj+980 component's +16 slot 20 with the matrix at 0x01164BD0, else 0x000CB934(out, obj)
DECOMP(0022F860, Comp_Xform_0022F860) { TXformVCall(ctx, 980, 0x000CB934, 0x01164BD0, 0x0022F830); }

// ---------------------------------------------------------------- Vec3 -> vector row (VMX)
// (Vec4* out, const f32* p): {p[0], p[1], p[2]} with w from the masks at 0x0010ED40
DECOMP(0010ED70, Vec3_ToRow_0010ED70) { TVec3Row(ctx, 0x0010ED40); }
// (Vec4* out, const f32* p): {p[0], p[1], p[2]} with w from the masks at 0x00137950
DECOMP(00137980, Vec3_ToRow_00137980) { TVec3Row(ctx, 0x00137950); }
// (Vec4* out, const f32* p): {p[0], p[1], p[2]} with w from the masks at 0x00170DE0
DECOMP(00170E10, Vec3_ToRow_00170E10) { TVec3Row(ctx, 0x00170DE0); }
// (Vec4* out, const f32* p): {p[0], p[1], p[2]} with w from the masks at 0x001F0810
DECOMP(001F0840, Vec3_ToRow_001F0840) { TVec3Row(ctx, 0x001F0810); }
// (Vec4* out, const f32* p): {p[0], p[1], p[2]} with w from the masks at 0x00204CF0
DECOMP(00204D20, Vec3_ToRow_00204D20) { TVec3Row(ctx, 0x00204CF0); }
// (Vec4* out, const f32* p): {p[0], p[1], p[2]} with w from the masks at 0x0026B480
DECOMP(0026B4B0, Vec3_ToRow_0026B4B0) { TVec3Row(ctx, 0x0026B480); }
// (Vec4* out, const f32* p): {p[0], p[1], p[2]} with w from the masks at 0x0034CE50
DECOMP(0034CE80, Vec3_ToRow_0034CE80) { TVec3Row(ctx, 0x0034CE50); }
// (Vec4* out, const f32* p): {p[0], p[1], p[2]} with w from the masks at 0x003E7B50
DECOMP(003E7B80, Vec3_ToRow_003E7B80) { TVec3Row(ctx, 0x003E7B50); }

// ---------------------------------------------------------------- reference setters
// (o, p): o+136 replaced (old slot 40 (old, 0)), p +178 count incremented
DECOMP(001138D8, SetRef_001138D8) { TSetRef178(ctx, 136); }
// (o, p): o+12 replaced (old slot 40 (old, 0)), p +178 count incremented
DECOMP(00114208, SetRef_00114208) { TSetRef178(ctx, 12); }
// (o, p): o+12 replaced (old slot 40 (old, 0)), p +178 count incremented
DECOMP(00115B0C, SetRef_00115B0C) { TSetRef178(ctx, 12); }
// (o, p): o+420 replaced (old slot 40 (old, 0)), p +178 count incremented
DECOMP(00121E8C, SetRef_00121E8C) { TSetRef178(ctx, 420); }
// (o, p): o+428 replaced (old slot 40 (old, 0)), p +178 count incremented
DECOMP(00121F10, SetRef_00121F10) { TSetRef178(ctx, 428); }
// (o, p): o+392 replaced (old slot 40 (old, 0)), p +178 count incremented
DECOMP(00757B58, SetRef_00757B58) { TSetRef178(ctx, 392); }
// (o, p): o+396 replaced (old slot 40 (old, 0)), p +178 count incremented
DECOMP(00757C9C, SetRef_00757C9C) { TSetRef178(ctx, 396); }

// ---------------------------------------------------------------- 4x3 block copies
// (dst, src): rows of three words 16 bytes apart copied
DECOMP(000E4E80, Mat43_Copy_000E4E80) { TCopyMat43Regs(ctx); }
// (dst, src): rows of three words 16 bytes apart copied
DECOMP(0015C924, Mat43_Copy_0015C924) { TCopyMat43Regs(ctx); }
// (dst, src): rows of three words 16 bytes apart copied
DECOMP(0015CA38, Mat43_Copy_0015CA38) { TCopyMat43Regs(ctx); }
// (dst, src): rows of three words 16 bytes apart copied
DECOMP(002370D0, Mat43_Copy_002370D0) { TCopyMat43Regs(ctx); }
// (dst, src): rows of three words 16 bytes apart copied
DECOMP(0026B360, Mat43_Copy_0026B360) { TCopyMat43Regs(ctx); }
// (dst, src): rows of three words 16 bytes apart copied
DECOMP(0026B508, Mat43_Copy_0026B508) { TCopyMat43Regs(ctx); }
// (dst, src): rows of three words 16 bytes apart copied
DECOMP(0034CED8, Mat43_Copy_0034CED8) { TCopyMat43Regs(ctx); }
// (dst, src): rows of three words 16 bytes apart copied
DECOMP(003E7C14, Mat43_Copy_003E7C14) { TCopyMat43Regs(ctx); }

// ---------------------------------------------------------------- component vector notifications (VMX)
// (o, const f32* p): o+348 component: 0x0091A4C8(c), then (c+224) slot 68 with the vector
DECOMP(000DED00, Comp_VecNotify_000DED00) { TCompVecNotify(ctx, 348, 0x000DECD0, 0x0091A4C8); }
// (o, const f32* p): o+388 component: 0x0091A4C8(c), then (c+224) slot 68 with the vector
DECOMP(00168620, Comp_VecNotify_00168620) { TCompVecNotify(ctx, 388, 0x001685F0, 0x0091A4C8); }
// (o, const f32* p): o+388 component: 0x0091A4C8(c), then (c+224) slot 68 with the vector
DECOMP(0016F1C0, Comp_VecNotify_0016F1C0) { TCompVecNotify(ctx, 388, 0x0016F190, 0x0091A4C8); }
// (o, const f32* p): o+388 component: 0x0091A4C8(c), then (c+224) slot 68 with the vector
DECOMP(00178650, Comp_VecNotify_00178650) { TCompVecNotify(ctx, 388, 0x00178620, 0x0091A4C8); }
// (o, const f32* p): o+388 component: 0x0091A4C8(c), then (c+224) slot 68 with the vector
DECOMP(00184A80, Comp_VecNotify_00184A80) { TCompVecNotify(ctx, 388, 0x00184A50, 0x0091A4C8); }
// (o, const f32* p): o+388 component: 0x0091A4C8(c), then (c+224) slot 68 with the vector
DECOMP(00192370, Comp_VecNotify_00192370) { TCompVecNotify(ctx, 388, 0x00192340, 0x0091A4C8); }
// (o, const f32* p): o+416 component: 0x0091A4C8(c), then (c+224) slot 68 with the vector
DECOMP(001FB370, Comp_VecNotify_001FB370) { TCompVecNotify(ctx, 416, 0x001FB340, 0x0091A4C8); }

// ---------------------------------------------------------------- deleting destructors releasing the objects of a PtrVector
// deleting: vtable 0x00B21FD8, the objects of the PtrVector at +8 released, base vtable 0x00B11B20, Mem_FreeFrom
DECOMP(004C0CF4, VtB21FD8_Delete) { TDeleteOwnerVecAt(ctx, 0x00B21FD8, 8, 0x00B11B20); }
// deleting: vtable 0x00B21FD8, the objects of the PtrVector at +8 released, base vtable 0x00B11B20, Mem_FreeFrom
DECOMP(004C1094, VtB21FD8_Delete_004C1094) { TDeleteOwnerVecAt(ctx, 0x00B21FD8, 8, 0x00B11B20); }
// deleting: vtable 0x00B21FD8, the objects of the PtrVector at +8 released, base vtable 0x00B11B20, Mem_FreeFrom
DECOMP(004C1380, VtB21FD8_Delete_004C1380) { TDeleteOwnerVecAt(ctx, 0x00B21FD8, 8, 0x00B11B20); }
// deleting: vtable 0x00B21FD8, the objects of the PtrVector at +8 released, base vtable 0x00B11B20, Mem_FreeFrom
DECOMP(004C1688, VtB21FD8_Delete_004C1688) { TDeleteOwnerVecAt(ctx, 0x00B21FD8, 8, 0x00B11B20); }
// deleting: vtable 0x00B21FD8, the objects of the PtrVector at +8 released, base vtable 0x00B11B20, Mem_FreeFrom
DECOMP(004C19C0, VtB21FD8_Delete_004C19C0) { TDeleteOwnerVecAt(ctx, 0x00B21FD8, 8, 0x00B11B20); }
// deleting: vtable 0x00B22308, the objects of the PtrVector at +40 released, base vtable 0x00B11B20, Mem_FreeFrom
DECOMP(004C62F0, VtB22308_Delete) { TDeleteOwnerVecAt(ctx, 0x00B22308, 40, 0x00B11B20); }
// deleting: vtable 0x00B22330, the objects of the PtrVector at +28 released, base vtable 0x00B11B20, Mem_FreeFrom
DECOMP(004C6C0C, VtB22330_Delete) { TDeleteOwnerVecAt(ctx, 0x00B22330, 28, 0x00B11B20); }

// ---------------------------------------------------------------- AVL tree inserts from the root
// (tree, u32 key, value): count++, root leaf or 0x0073AFC4(tree, key, value, &root)
DECOMP(0073B444, Avl_TreeInsert_0073B444) { TAvlTreeInsert(ctx, 0x0073AFC4, 0x00F8F0E0, 0x00B10EB0, 0x00AC47E8); }
// (tree, u32 key, value): count++, root leaf or 0x0076D07C(tree, key, value, &root)
DECOMP(0076D4FC, Avl_TreeInsert_0076D4FC) { TAvlTreeInsert(ctx, 0x0076D07C, 0x00F8F0E8, 0x00B1F648, 0x00ADBDC9); }
// (tree, u32 key, value): count++, root leaf or 0x0078E558(tree, key, value, &root)
DECOMP(0078E9D8, Avl_TreeInsert_0078E9D8) { TAvlTreeInsert(ctx, 0x0078E558, 0x00F8F100, 0x00B22550, 0x00AEECC9); }
// (tree, u32 key, value): count++, root leaf or 0x0078EB6C(tree, key, value, &root)
DECOMP(0078EFEC, Avl_TreeInsert_0078EFEC) { TAvlTreeInsert(ctx, 0x0078EB6C, 0x00F8F0F4, 0x00B22510, 0x00AEEB72); }
// (tree, u32 key, value): count++, root leaf or 0x007918E4(tree, key, value, &root)
DECOMP(00791D64, Avl_TreeInsert_00791D64) { TAvlTreeInsert(ctx, 0x007918E4, 0x00F8F0F8, 0x00B22570, 0x00AEEF23); }
// (tree, u32 key, value): count++, root leaf or 0x007930E8(tree, key, value, &root)
DECOMP(00793568, Avl_TreeInsert_00793568) { TAvlTreeInsert(ctx, 0x007930E8, 0x00F8F0DC, 0x00B0EFC8, 0x00AEEE29); }
// (tree, u32 key, value): count++, root leaf or 0x00798E60(tree, key, value, &root)
DECOMP(007992E0, Avl_TreeInsert_007992E0) { TAvlTreeInsert(ctx, 0x00798E60, 0x00F8F104, 0x00B22F88, 0x00AEFA70); }
// (tree, u32 key, value): count++, root leaf or 0x007995FC(tree, key, value, &root)
DECOMP(00799A7C, Avl_TreeInsert_00799A7C) { TAvlTreeInsert(ctx, 0x007995FC, 0x00F8F108, 0x00B22FA8, 0x00AEFB23); }

// ---------------------------------------------------------------- AVL tree: remove the rightmost node
// (tree, Node** target, Node** pp, bool* changed): rightmost node unlinked into *target, freed to 0x00F8F0E0, rebalance
DECOMP(0073A064, Avl_RemoveMax_0073A064) { TAvlRemoveMax(ctx, 0x0073A064, 0x00F8F0E0, 0x00B10EB0, 0x00AC47E8, 0x00556D18, 0x00739F1C, 0x00739DD4); }
// (tree, Node** target, Node** pp, bool* changed): rightmost node unlinked into *target, freed to 0x00F8F0E8, rebalance
DECOMP(0076C11C, Avl_RemoveMax_0076C11C) { TAvlRemoveMax(ctx, 0x0076C11C, 0x00F8F0E8, 0x00B1F648, 0x00ADBDC9, 0x00556D18, 0x0076BFD4, 0x0076BE8C); }
// (tree, Node** target, Node** pp, bool* changed): rightmost node unlinked into *target, freed to 0x00F8F0F8, rebalance
DECOMP(0078A2B8, Avl_RemoveMax_0078A2B8) { TAvlRemoveMax(ctx, 0x0078A2B8, 0x00F8F0F8, 0x00B22570, 0x00AEEF23, 0x00556D18, 0x0078A170, 0x0078A028); }
// (tree, Node** target, Node** pp, bool* changed): rightmost node unlinked into *target, freed to 0x00F8F0E4, rebalance
DECOMP(0078B218, Avl_RemoveMax_0078B218) { TAvlRemoveMax(ctx, 0x0078B218, 0x00F8F0E4, 0x00B10E90, 0x00AEED78, 0x00556D18, 0x007855FC, 0x007854B4); }
// (tree, Node** target, Node** pp, bool* changed): rightmost node unlinked into *target, freed to 0x00F8F0F4, rebalance
DECOMP(0078C408, Avl_RemoveMax_0078C408) { TAvlRemoveMax(ctx, 0x0078C408, 0x00F8F0F4, 0x00B22510, 0x00AEEB72, 0x00556D18, 0x0078C2C0, 0x0078C178); }
// (tree, Node** target, Node** pp, bool* changed): rightmost node unlinked into *target, freed to 0x00F8F100, rebalance
DECOMP(00790984, Avl_RemoveMax_00790984) { TAvlRemoveMax(ctx, 0x00790984, 0x00F8F100, 0x00B22550, 0x00AEECC9, 0x00556D18, 0x0078E410, 0x0078E2C8); }
// (tree, Node** target, Node** pp, bool* changed): rightmost node unlinked into *target, freed to 0x00F8F104, rebalance
DECOMP(00796D10, Avl_RemoveMax_00796D10) { TAvlRemoveMax(ctx, 0x00796D10, 0x00F8F104, 0x00B22F88, 0x00AEFA70, 0x00556D18, 0x00796BC8, 0x00796A80); }
// (tree, Node** target, Node** pp, bool* changed): rightmost node unlinked into *target, freed to 0x00F8F108, rebalance
DECOMP(00797F00, Avl_RemoveMax_00797F00) { TAvlRemoveMax(ctx, 0x00797F00, 0x00F8F108, 0x00B22FA8, 0x00AEFB23, 0x00556D18, 0x00797DB8, 0x00797C70); }

// ---------------------------------------------------------------- command records (kind 7)
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out word cleared
DECOMP(007742DC, Cmd_PushOp7_007742DC) { TCmdPushOp7(ctx, false); }
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out word cleared
DECOMP(0077487C, Cmd_PushOp7_0077487C) { TCmdPushOp7(ctx, false); }
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out word cleared
DECOMP(0077496C, Cmd_PushOp7_0077496C) { TCmdPushOp7(ctx, false); }
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out word cleared
DECOMP(00774A5C, Cmd_PushOp7_00774A5C) { TCmdPushOp7(ctx, false); }
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out word cleared
DECOMP(00774B4C, Cmd_PushOp7_00774B4C) { TCmdPushOp7(ctx, false); }
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out word cleared
DECOMP(00774C3C, Cmd_PushOp7_00774C3C) { TCmdPushOp7(ctx, false); }

// ---------------------------------------------------------------- command records (kind 7, halfword)
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out halfword cleared
DECOMP(007743CC, Cmd_PushOp7_007743CC) { TCmdPushOp7(ctx, true); }
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out halfword cleared
DECOMP(007744BC, Cmd_PushOp7_007744BC) { TCmdPushOp7(ctx, true); }
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out halfword cleared
DECOMP(007745AC, Cmd_PushOp7_007745AC) { TCmdPushOp7(ctx, true); }
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out halfword cleared
DECOMP(0077469C, Cmd_PushOp7_0077469C) { TCmdPushOp7(ctx, true); }
// (u16 v, Ctx** pc, u32* out): a kind-7 record; *out halfword cleared
DECOMP(0077478C, Cmd_PushOp7_0077478C) { TCmdPushOp7(ctx, true); }

// ---------------------------------------------------------------- on / off setters (the on call takes an f32 constant)
// (o, bool on, u8 a): off: 0x00362F1C(o, a, 0); on (unless +396 bit 0x40000000): 0x00363890(o, 0, *0x0036BF34, 0, 0, 0x01164CA4)
DECOMP(0036BF38, Obj_SetOnOff_0036BF38) { TOnOffF(ctx, 396, 0x00362F1C, 0x0036BF34, 0x00363890, 0x01164CA4); }
// (o, bool on, u8 a): off: 0x00377D18(o, a, 0); on (unless +252 bit 0x40000000): 0x003780E0(o, 0, *0x003798D4, 0, 0, 0x01164CA4)
DECOMP(003798D8, Obj_SetOnOff_003798D8) { TOnOffF(ctx, 252, 0x00377D18, 0x003798D4, 0x003780E0, 0x01164CA4); }
// (o, bool on, u8 a): off: 0x0037C7E0(o, a, 0); on (unless +252 bit 0x40000000): 0x0037CB10(o, 0, *0x0037DE4C, 0, 0, 0x01164CA4)
DECOMP(0037DE50, Obj_SetOnOff_0037DE50) { TOnOffF(ctx, 252, 0x0037C7E0, 0x0037DE4C, 0x0037CB10, 0x01164CA4); }
// (o, bool on, u8 a): off: 0x0037F5E8(o, a, 0); on (unless +252 bit 0x40000000): 0x0037F918(o, 0, *0x00380C68, 0, 0, 0x01164CA4)
DECOMP(00380C70, Obj_SetOnOff_00380C70) { TOnOffF(ctx, 252, 0x0037F5E8, 0x00380C68, 0x0037F918, 0x01164CA4); }
// (o, bool on, u8 a): off: 0x00386140(o, a, 0); on (unless +960 bit 0x40000000): 0x00386560(o, 0, *0x00387A7C, 0, 0, 0x01164CA4)
DECOMP(00387A80, Obj_SetOnOff_00387A80) { TOnOffF(ctx, 960, 0x00386140, 0x00387A7C, 0x00386560, 0x01164CA4); }

// ---------------------------------------------------------------- slot-20 calls over a pointer array
// (o): slot 20 of each of the +32 count objects at +24
DECOMP(0007DE20, Obj_ForEachVCall20_0007DE20) { TForEachVCall20(ctx, 32, 24); }
// (o): slot 20 of each of the +36 count objects at +28
DECOMP(0007E564, Obj_ForEachVCall20_0007E564) { TForEachVCall20(ctx, 36, 28); }
// (o): slot 20 of each of the +20 count objects at +12
DECOMP(0007F3F4, Obj_ForEachVCall20_0007F3F4) { TForEachVCall20(ctx, 20, 12); }
// (o): slot 20 of each of the +48 count objects at +40
DECOMP(004C63EC, Obj_ForEachVCall20_004C63EC) { TForEachVCall20(ctx, 48, 40); }
// (o): slot 20 of each of the +36 count objects at +28
DECOMP(004C6D08, Obj_ForEachVCall20_004C6D08) { TForEachVCall20(ctx, 36, 28); }

// ---------------------------------------------------------------- factories binding two properties
// (?, owner): Mem_Alloc(20), 0x004BB3B0(p, owner), vtable 0x00B0F360, 0x004FA14C(owner, 0x00217713 / 0x36611714, &p->+12 / +16)
DECOMP(0006FE54, NewBind2_0006FE54) { TNew20Bind2(ctx, 0x004BB3B0, 0x00B0F360, 0x004FA14C, 0x00217713, 0x36611714); }
// (?, owner): Mem_Alloc(20), 0x004BB3B0(p, owner), vtable 0x00B0F428, 0x004FA14C(owner, 0x7A30D7E3 / 0x7F247172, &p->+12 / +16)
DECOMP(00071F1C, NewBind2_00071F1C) { TNew20Bind2(ctx, 0x004BB3B0, 0x00B0F428, 0x004FA14C, 0x7A30D7E3, 0x7F247172); }
// (?, owner): Mem_Alloc(20), 0x004BB3B0(p, owner), vtable 0x00B0FB80, 0x004FA14C(owner, 0x7FAAD4A0 / 0x04B8CC71, &p->+12 / +16)
DECOMP(0007D884, NewBind2_0007D884) { TNew20Bind2(ctx, 0x004BB3B0, 0x00B0FB80, 0x004FA14C, 0x7FAAD4A0, 0x04B8CC71); }
// (?, owner): Mem_Alloc(20), 0x004BB3B0(p, owner), vtable 0x00B11A30, 0x004FA14C(owner, 0x0024728B / 0x38EAE212, &p->+12 / +16)
DECOMP(000A6A84, NewBind2_000A6A84) { TNew20Bind2(ctx, 0x004BB3B0, 0x00B11A30, 0x004FA14C, 0x0024728B, 0x38EAE212); }
// (?, owner): Mem_Alloc(20), 0x004BB3B0(p, owner), vtable 0x00B11AF8, 0x004FA14C(owner, 0x040EFE5F / 0x7E6EFA38, &p->+12 / +16)
DECOMP(000A74D0, NewBind2_000A74D0) { TNew20Bind2(ctx, 0x004BB3B0, 0x00B11AF8, 0x004FA14C, 0x040EFE5F, 0x7E6EFA38); }

// ---------------------------------------------------------------- 4x4 matrix from rows of three floats (VMX)
// (Vec4 out[4], const f32 p[16]): rows {p[4k..4k+2]} with w from the masks at 0x00170C80
DECOMP(00170CB0, Mat4_FromRows3_00170CB0) { TMat4Rows(ctx, 0x00170C80); }
// (Vec4 out[4], const f32 p[16]): rows {p[4k..4k+2]} with w from the masks at 0x0017DBF0
DECOMP(0017DC20, Mat4_FromRows3_0017DC20) { TMat4Rows(ctx, 0x0017DBF0); }
// (Vec4 out[4], const f32 p[16]): rows {p[4k..4k+2]} with w from the masks at 0x00200BE0
DECOMP(00200C10, Mat4_FromRows3_00200C10) { TMat4Rows(ctx, 0x00200BE0); }
// (Vec4 out[4], const f32 p[16]): rows {p[4k..4k+2]} with w from the masks at 0x002BCB80
DECOMP(002BCBB0, Mat4_FromRows3_002BCBB0) { TMat4Rows(ctx, 0x002BCB80); }
// (Vec4 out[4], const f32 p[16]): rows {p[4k..4k+2]} with w from the masks at 0x0034CFF0
DECOMP(0034D020, Mat4_FromRows3_0034D020) { TMat4Rows(ctx, 0x0034CFF0); }

// ---------------------------------------------------------------- name lists filled from XML
// (XmlCursor*): a copy of its "name" attribute appended to the game object +496 PtrVector
DECOMP(004EF274, Game_AddName_004EF274) { TAddNameCopy(ctx, 496); }
// (XmlCursor*): a copy of its "name" attribute appended to the game object +376 PtrVector
DECOMP(004EFA4C, Game_AddName_004EFA4C) { TAddNameCopy(ctx, 376); }
// (XmlCursor*): a copy of its "name" attribute appended to the game object +396 PtrVector
DECOMP(004EFB40, Game_AddName_004EFB40) { TAddNameCopy(ctx, 396); }
// (XmlCursor*): a copy of its "name" attribute appended to the game object +416 PtrVector
DECOMP(004EFC34, Game_AddName_004EFC34) { TAddNameCopy(ctx, 416); }
// (XmlCursor*): a copy of its "name" attribute appended to the game object +436 PtrVector
DECOMP(004EFE00, Game_AddName_004EFE00) { TAddNameCopy(ctx, 436); }

// ---------------------------------------------------------------- camera definitions applied to a view
// (o): the def keyed by (o+88)->+8 onto the view at o+60 (rows, +80 from (o+88)->+16)
DECOMP(003FA828, Camera_ApplyDef_003FA828) { TApplyCameraDef(ctx, 8, 16, 0x003FA820); }
// (o): the def keyed by (o+88)->+36 onto the view at o+60 (rows, +80 from (o+88)->+40)
DECOMP(00402810, Camera_ApplyDef_00402810) { TApplyCameraDef(ctx, 36, 40, 0x0040280C); }
// (o): the def keyed by (o+88)->+12 onto the view at o+60 (rows, +80 from (o+88)->+16)
DECOMP(004088A8, Camera_ApplyDef_004088A8) { TApplyCameraDef(ctx, 12, 16, 0x004088A0); }
// (o): the def keyed by (o+88)->+4 onto the view at o+60 (rows, +80 from (o+88)->+8)
DECOMP(00410D50, Camera_ApplyDef_00410D50) { TApplyCameraDef(ctx, 4, 8, 0x00410D4C); }
// (o): the def keyed by (o+88)->+24 onto the view at o+60 (rows, +80 from (o+88)->+28)
DECOMP(0041D000, Camera_ApplyDef_0041D000) { TApplyCameraDef(ctx, 24, 28, 0x0041CFFC); }

// ---------------------------------------------------------------- slot free lists
// (list, s32 n): n 16-byte slots allocated and chained as a free list
DECOMP(00784C84, SlotList_Init_00784C84) { TSlotListInit(ctx); }
// (list, s32 n): n 16-byte slots allocated and chained as a free list
DECOMP(0079A1C8, SlotList_Init_0079A1C8) { TSlotListInit(ctx); }
// (list, s32 n): n 16-byte slots allocated and chained as a free list
DECOMP(0079B0F0, SlotList_Init_0079B0F0) { TSlotListInit(ctx); }
// (list, s32 n): n 16-byte slots allocated and chained as a free list
DECOMP(0079B250, SlotList_Init_0079B250) { TSlotListInit(ctx); }
// (list, s32 n): n 16-byte slots allocated and chained as a free list
DECOMP(0079BA10, Lock_Init) { TSlotListInit(ctx); }

// ---------------------------------------------------------------- destructors of the level-section classes
// ~T(): vtable 0x00B1E648, 0x003260F0(this, 0, 1), the section arrays / lists / buffers freed, 0x0048F6B0(this)
DECOMP(0032CA0C, VtB1E648_Dtor) { TDtorSection(ctx, 0x00B1E648, 0x003260F0, 0x00321240, 0x0048F6B0); }
// ~T(): vtable 0x00B1E7B0, 0x003260F0(this, 0, 1), the section arrays / lists / buffers freed, 0x0048F6B0(this)
DECOMP(0032EA68, VtB1E7B0_Dtor) { TDtorSection(ctx, 0x00B1E7B0, 0x003260F0, 0x00321240, 0x0048F6B0); }
// ~T(): vtable 0x00B1F0C0, 0x003260F0(this, 0, 1), the section arrays / lists / buffers freed, 0x0048F6B0(this)
DECOMP(00340134, VtB1F0C0_Dtor) { TDtorSection(ctx, 0x00B1F0C0, 0x003260F0, 0x00321240, 0x0048F6B0); }
// ~T(): vtable 0x00B1F138, 0x003260F0(this, 0, 1), the section arrays / lists / buffers freed, 0x0048F6B0(this)
DECOMP(003409B8, VtB1F138_Dtor) { TDtorSection(ctx, 0x00B1F138, 0x003260F0, 0x00321240, 0x0048F6B0); }
// ~T(): vtable 0x00B1F1B0, 0x003260F0(this, 0, 1), the section arrays / lists / buffers freed, 0x0048F6B0(this)
DECOMP(00341208, VtB1F1B0_Dtor) { TDtorSection(ctx, 0x00B1F1B0, 0x003260F0, 0x00321240, 0x0048F6B0); }

// ---------------------------------------------------------------- field clears
// this->+20 = 0
DECOMP(0001B2E8, Obj_ClearField_0001B2E8) { TStoreZeroAt(ctx, 20); }
// this->+104 = 0
DECOMP(0010BD7C, Obj_ClearField_0010BD7C) { TStoreZeroAt(ctx, 104); }
// this->+424 = 0
DECOMP(003A4010, Obj_ClearField_003A4010) { TStoreZeroAt(ctx, 424); }
// this->+140 = 0
DECOMP(004F41CC, Obj_ClearField_004F41CC) { TStoreZeroAt(ctx, 140); }

// ---------------------------------------------------------------- field copies
// this->+952 = this->+960
DECOMP(00230990, Obj_CopyField_00230990) { TCopyWordIn(ctx, 960, 952); }
// this->+952 = this->+956
DECOMP(0023DF6C, Obj_CopyField_0023DF6C) { TCopyWordIn(ctx, 956, 952); }
// this->+400 = this->+408
DECOMP(00246D50, Obj_CopyField_00246D50) { TCopyWordIn(ctx, 408, 400); }
// this->+400 = this->+404
DECOMP(0024DDBC, Obj_CopyField_0024DDBC) { TCopyWordIn(ctx, 404, 400); }

// ---------------------------------------------------------------- element addresses
// &this->+0[i] (32-byte elements)
DECOMP(00735860, Array_ElemAddr_00735860) { TIndexAddr(ctx, 5); }
// &this->+0[i] (16-byte elements)
DECOMP(0074E2E8, Array_ElemAddr_0074E2E8) { TIndexAddr(ctx, 4); }
// &this->+0[i] (128-byte elements)
DECOMP(00760F00, Array_ElemAddr_00760F00) { TIndexAddr(ctx, 7); }
// &this->+0[i] (128-byte elements)
DECOMP(0077B100, Array_ElemAddr_0077B100) { TIndexAddr(ctx, 7); }

// ---------------------------------------------------------------- field reads through a pointer
// q = this->+428; q ? q->+488 : 0
DECOMP(0022104C, Obj_GetVia_0022104C) { TLoadChain0(ctx, 428, 488); }
// q = this->+600; q ? q->+168 : 0
DECOMP(00755E30, Obj_GetVia_00755E30) { TLoadChain0(ctx, 600, 168); }
// q = this->+216; q ? q->+52 : 0
DECOMP(00755F20, Obj_GetVia_00755F20) { TLoadChain0(ctx, 216, 52); }
// q = this->+4; q ? q->+8 : 0
DECOMP(00757BDC, Obj_GetVia_00757BDC) { TLoadChain0(ctx, 4, 8); }

// ---------------------------------------------------------------- 16-byte copies
// dst[0..3] = src->+0[0..3]
DECOMP(0074BCD4, Copy16_0074BCD4) { TCopyFields(ctx, 0, 4); }
// dst[0..3] = src->+0[0..3]
DECOMP(0074CEF4, Copy16_0074CEF4) { TCopyFields(ctx, 0, 4); }
// dst[0..3] = src->+176[0..3]
DECOMP(0076097C, Copy16_0076097C) { TCopyFields(ctx, 176, 4); }
// dst[0..3] = src->+16[0..3]
DECOMP(00762B38, Copy16_00762B38) { TCopyFields(ctx, 16, 4); }

// ---------------------------------------------------------------- virtual calls on a member
// q = this->+36; q ? q->vfn[9](q) : 0
DECOMP(0001BEA0, Obj_MemberVCall_0001BEA0) { TVCallField(ctx, 36, 36); }
// q = this->+72; q ? q->vfn[5](q) : 0
DECOMP(002E11D8, Obj_MemberVCall_002E11D8) { TVCallField(ctx, 72, 20); }
// q = this->+152; q ? q->vfn[3](q) : 0
DECOMP(00426A90, Obj_MemberVCall_00426A90) { TVCallField(ctx, 152, 12); }
// q = this->+0; q ? q->vfn[14](q) : 0
DECOMP(004F2FF4, Obj_MemberVCall_004F2FF4) { TVCallField(ctx, 0, 56); }

// ---------------------------------------------------------------- keyed virtual calls
// if (this->+12 == a1) this->vfn[5](this)
DECOMP(00767AB0, Obj_VCallIfKey_00767AB0) { TVCall20IfKey(ctx, 12); }
// if (this->+212 == a1) this->vfn[5](this)
DECOMP(00767B0C, Obj_VCallIfKey_00767B0C) { TVCall20IfKey(ctx, 212); }
// if (this->+12 == a1) this->vfn[5](this)
DECOMP(00767B68, Obj_VCallIfKey_00767B68) { TVCall20IfKey(ctx, 12); }
// if (this->+12 == a1) this->vfn[5](this)
DECOMP(00767BC4, Obj_VCallIfKey_00767BC4) { TVCall20IfKey(ctx, 12); }

// ---------------------------------------------------------------- 3x3 block copies
// (dst, src): three rows of three words 16 bytes apart
DECOMP(0074BC88, Mat33_Copy_0074BC88) { TCopyMat33(ctx); }
// (dst, src): three rows of three words 16 bytes apart
DECOMP(0074CF18, Mat33_Copy_0074CF18) { TCopyMat33(ctx); }
// (dst, src): three rows of three words 16 bytes apart
DECOMP(0074CF64, Mat33_Copy_0074CF64) { TCopyMat33(ctx); }
// (dst, src): three rows of three words 16 bytes apart
DECOMP(0074E524, Mat33_Copy_0074E524) { TCopyMat33(ctx); }

// ---------------------------------------------------------------- secondary-base destructors (five vtables)
// p = this - 20: five vtables (0x00B1F298 at +0), then 0x00884FB8(p)
DECOMP(00770D08, AdjDtor_00770D08) { TAdjDtorVt5Call(ctx, -20, 0x00B1F298, 0x00B1F2D0, 0x00B1F2E4, 0x00B1F300, 0x00B1F314, 0x00884FB8); }
// p = this - 16: five vtables (0x00B1F298 at +0), then 0x00884FB8(p)
DECOMP(00770D64, AdjDtor_00770D64) { TAdjDtorVt5Call(ctx, -16, 0x00B1F298, 0x00B1F2D0, 0x00B1F2E4, 0x00B1F300, 0x00B1F314, 0x00884FB8); }
// p = this - 12: five vtables (0x00B1F298 at +0), then 0x00884FB8(p)
DECOMP(00770DC0, AdjDtor_00770DC0) { TAdjDtorVt5Call(ctx, -12, 0x00B1F298, 0x00B1F2D0, 0x00B1F2E4, 0x00B1F300, 0x00B1F314, 0x00884FB8); }
// p = this - 8: five vtables (0x00B1F298 at +0), then 0x00884FB8(p)
DECOMP(00770E1C, AdjDtor_00770E1C) { TAdjDtorVt5Call(ctx, -8, 0x00B1F298, 0x00B1F2D0, 0x00B1F2E4, 0x00B1F300, 0x00B1F314, 0x00884FB8); }

// ---------------------------------------------------------------- destructors: a PtrVector, then the base vtable and destructor
// ~T(): vtable 0x00B20850, PtrVector +272, base vtable 0x00B207A8, 0x00490B34(this)
DECOMP(0077D250, VtB20850_Dtor) { TDtorVecVt(ctx, 0x00B20850, 272, 0x00B207A8, 0x00490B34); }
// ~T(): vtable 0x00B208F8, PtrVector +104, base vtable 0x00B207A8, 0x00490B34(this)
DECOMP(0077D2C4, VtB208F8_Dtor) { TDtorVecVt(ctx, 0x00B208F8, 104, 0x00B207A8, 0x00490B34); }
// ~T(): vtable 0x00B209A0, PtrVector +100, base vtable 0x00B207A8, 0x00490B34(this)
DECOMP(0077D338, VtB209A0_Dtor) { TDtorVecVt(ctx, 0x00B209A0, 100, 0x00B207A8, 0x00490B34); }
// ~T(): vtable 0x00B20B98, PtrVector +104, base vtable 0x00B207A8, 0x00490B34(this)
DECOMP(0077D558, VtB20B98_Dtor) { TDtorVecVt(ctx, 0x00B20B98, 104, 0x00B207A8, 0x00490B34); }

// ---------------------------------------------------------------- secondary-base deleting destructors (SmallFree)
// deleting: p = this -384; vtables 0x00B12528 / 0x00B1200C / 0x00B126B8, 0x000DF934(p), SmallFree(p, 480)
DECOMP(00168258, AdjDelete_00168258) { TAdjDeleteVt3Small(ctx, -384, 0x00B12528, 0x00B1200C, 0x00B126B8, 0x000DF934, 480); }
// deleting: p = this -380; vtables 0x00B12528 / 0x00B1200C / 0x00B126B8, 0x000DF934(p), SmallFree(p, 480)
DECOMP(001682D0, AdjDelete_001682D0) { TAdjDeleteVt3Small(ctx, -380, 0x00B12528, 0x00B1200C, 0x00B126B8, 0x000DF934, 480); }
// deleting: p = this -384; vtables 0x00B12528 / 0x00B1200C / 0x00B126B8, 0x000DF934(p), SmallFree(p, 464)
DECOMP(007590AC, AdjDelete_007590AC) { TAdjDeleteVt3Small(ctx, -384, 0x00B12528, 0x00B1200C, 0x00B126B8, 0x000DF934, 464); }
// deleting: p = this -380; vtables 0x00B12528 / 0x00B1200C / 0x00B126B8, 0x000DF934(p), SmallFree(p, 464)
DECOMP(00759124, AdjDelete_00759124) { TAdjDeleteVt3Small(ctx, -380, 0x00B12528, 0x00B1200C, 0x00B126B8, 0x000DF934, 464); }

// ---------------------------------------------------------------- condition factories
// (s16 id): Mem_Alloc(24), 0x0038E5DC(p, id, "CAlways", 1)
DECOMP(007755E4, New_CAlways) { TNewNamedCond(ctx, 0x0038E5DC, 0x00B1FACD, 1); }
// (s16 id): Mem_Alloc(24), 0x0038E5DC(p, id, "CNever", 1)
DECOMP(0077571C, New_CNever) { TNewNamedCond(ctx, 0x0038E5DC, 0x00B1FAD5, 1); }
// (s16 id): Mem_Alloc(24), 0x0038E5DC(p, id, "COnEnterState", 2)
DECOMP(00775854, New_COnEnterState) { TNewNamedCond(ctx, 0x0038E5DC, 0x00B1FADC, 2); }
// (s16 id): Mem_Alloc(24), 0x0038E5DC(p, id, "COnExitState", 4)
DECOMP(0077598C, New_COnExitState) { TNewNamedCond(ctx, 0x0038E5DC, 0x00B1FAEA, 4); }

// ---------------------------------------------------------------- destructors releasing a reference
// ~T(): vtable 0x00B13F70, the +84 reference: slot 40 (ref, 0), base vtable 0x00B0DAEC
DECOMP(001136E0, VtB13F70_Dtor) { TDtorReleaseRef(ctx, 0x00B13F70, 84, 0x00B0DAEC); }
// ~T(): vtable 0x00B14020, the +12 reference: slot 40 (ref, 0), base vtable 0x00B0DAEC
DECOMP(001144D0, VtB14020_Dtor) { TDtorReleaseRef(ctx, 0x00B14020, 12, 0x00B0DAEC); }
// ~T(): vtable 0x00B13B38, the +84 reference: slot 40 (ref, 0), base vtable 0x00B0DAEC
DECOMP(0074950C, VtB13B38_Dtor) { TDtorReleaseRef(ctx, 0x00B13B38, 84, 0x00B0DAEC); }
// ~T(): vtable 0x00B13FC8, the +136 reference: slot 40 (ref, 0), base vtable 0x00B0DAEC
DECOMP(00749888, VtB13FC8_Dtor) { TDtorReleaseRef(ctx, 0x00B13FC8, 136, 0x00B0DAEC); }

// ---------------------------------------------------------------- destructors: a PtrVector, then unregistered
// ~T(): vtable 0x00B1BEF0, PtrVector +20, base vtable 0x00B0DAAC, 0x007330C0(0x01056F04, this + 4)
DECOMP(00762D34, VtB1BEF0_Dtor) { TDtorVecUnreg(ctx, 0x00B1BEF0, 20, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// ~T(): vtable 0x00B1C568, PtrVector +96, base vtable 0x00B0DAAC, 0x007330C0(0x01056F04, this + 4)
DECOMP(00763F10, VtB1C568_Dtor) { TDtorVecUnreg(ctx, 0x00B1C568, 96, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// ~T(): vtable 0x00B1C5C0, PtrVector +20, base vtable 0x00B0DAAC, 0x007330C0(0x01056F04, this + 4)
DECOMP(00764028, VtB1C5C0_Dtor) { TDtorVecUnreg(ctx, 0x00B1C5C0, 20, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// ~T(): vtable 0x00B1CD48, PtrVector +20, base vtable 0x00B0DAAC, 0x007330C0(0x01056F04, this + 4)
DECOMP(007650C8, VtB1CD48_Dtor) { TDtorVecUnreg(ctx, 0x00B1CD48, 20, 0x00B0DAAC, 0x007330C0, 0x01056F04); }

// ---------------------------------------------------------------- destructors freeing two buffers
// ~T(): vtable 0x00B1F898, +148 / +196 freed, base vtable 0x00B1F668
DECOMP(00772F70, VtB1F898_Dtor) { TDtorFree2(ctx, 0x00B1F898, 148, 196, 0x00B1F668); }
// ~T(): vtable 0x00B1F8D0, +180 / +184 freed, base vtable 0x00B1F668
DECOMP(00773088, VtB1F8D0_Dtor) { TDtorFree2(ctx, 0x00B1F8D0, 180, 184, 0x00B1F668); }
// ~T(): vtable 0x00B1F908, +176 / +180 freed, base vtable 0x00B1F668
DECOMP(007731A0, VtB1F908_Dtor) { TDtorFree2(ctx, 0x00B1F908, 176, 180, 0x00B1F668); }
// ~T(): vtable 0x00B1F940, +176 / +180 freed, base vtable 0x00B1F668
DECOMP(007732B8, VtB1F940_Dtor) { TDtorFree2(ctx, 0x00B1F940, 176, 180, 0x00B1F668); }

// ---------------------------------------------------------------- their deleting versions (SmallFree)
// deleting: vtable 0x00B20850, PtrVector +272, base vtable 0x00B207A8, 0x00490B34(this), SmallFree(this, 384)
DECOMP(0077F044, VtB20850_Delete) { TDeleteVecVt(ctx, 0x00B20850, 272, 0x00B207A8, 0x00490B34, 384); }
// deleting: vtable 0x00B208F8, PtrVector +104, base vtable 0x00B207A8, 0x00490B34(this), SmallFree(this, 256)
DECOMP(0077F0F0, VtB208F8_Delete) { TDeleteVecVt(ctx, 0x00B208F8, 104, 0x00B207A8, 0x00490B34, 256); }
// deleting: vtable 0x00B209A0, PtrVector +100, base vtable 0x00B207A8, 0x00490B34(this), SmallFree(this, 256)
DECOMP(0077F19C, VtB209A0_Delete) { TDeleteVecVt(ctx, 0x00B209A0, 100, 0x00B207A8, 0x00490B34, 256); }
// deleting: vtable 0x00B20B98, PtrVector +104, base vtable 0x00B207A8, 0x00490B34(this), SmallFree(this, 512)
DECOMP(0077F45C, VtB20B98_Delete) { TDeleteVecVt(ctx, 0x00B20B98, 104, 0x00B207A8, 0x00490B34, 512); }

// ---------------------------------------------------------------- factories: (p, a1, a0) constructor, then the vtable
// p = Mem_Alloc(40, 0, 16); if p: 0x004F77F8(p, a1, a0), vtable 0x00B0E010; returns p
DECOMP(0001D10C, New_0001D10C) { TNew2Vt(ctx, 40, 0x004F77F8, 0x00B0E010); }
// p = Mem_Alloc(36, 0, 16); if p: 0x004F77F8(p, a1, a0), vtable 0x00B0E1A0; returns p
DECOMP(00023FB0, New_00023FB0) { TNew2Vt(ctx, 36, 0x004F77F8, 0x00B0E1A0); }
// p = Mem_Alloc(40, 0, 16); if p: 0x004F77F8(p, a1, a0), vtable 0x00B0E5A0; returns p
DECOMP(0002EB74, New_0002EB74) { TNew2Vt(ctx, 40, 0x004F77F8, 0x00B0E5A0); }
// p = Mem_Alloc(36, 0, 16); if p: 0x004F77F8(p, a1, a0), vtable 0x00B0E734; returns p
DECOMP(0003C000, New_0003C000) { TNew2Vt(ctx, 36, 0x004F77F8, 0x00B0E734); }

// ---------------------------------------------------------------- deleting destructors releasing a reference
// deleting: vtable 0x00B13F70, the +84 reference released, base vtable 0x00B0DAEC, SmallFree(this, 128)
DECOMP(0011375C, VtB13F70_Delete) { TDeleteReleaseRef(ctx, 0x00B13F70, 84, 0x00B0DAEC, 128); }
// deleting: vtable 0x00B14020, the +12 reference released, base vtable 0x00B0DAEC, SmallFree(this, 176)
DECOMP(0011454C, VtB14020_Delete) { TDeleteReleaseRef(ctx, 0x00B14020, 12, 0x00B0DAEC, 176); }
// deleting: vtable 0x00B13B38, the +84 reference released, base vtable 0x00B0DAEC, SmallFree(this, 112)
DECOMP(00749588, VtB13B38_Delete) { TDeleteReleaseRef(ctx, 0x00B13B38, 84, 0x00B0DAEC, 112); }
// deleting: vtable 0x00B13FC8, the +136 reference released, base vtable 0x00B0DAEC, SmallFree(this, 160)
DECOMP(00749904, VtB13FC8_Delete) { TDeleteReleaseRef(ctx, 0x00B13FC8, 136, 0x00B0DAEC, 160); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B1BEF0, PtrVector +20, unregistered, SmallFree(this, 44)
DECOMP(00762DB4, VtB1BEF0_Delete) { TDeleteVecUnreg(ctx, 0x00B1BEF0, 20, 0x00B0DAAC, 0x007330C0, 0x01056F04, 44); }
// deleting: vtable 0x00B1C568, PtrVector +96, unregistered, SmallFree(this, 128)
DECOMP(00763F90, VtB1C568_Delete) { TDeleteVecUnreg(ctx, 0x00B1C568, 96, 0x00B0DAAC, 0x007330C0, 0x01056F04, 128); }
// deleting: vtable 0x00B1C5C0, PtrVector +20, unregistered, SmallFree(this, 40)
DECOMP(007640A8, VtB1C5C0_Delete) { TDeleteVecUnreg(ctx, 0x00B1C5C0, 20, 0x00B0DAAC, 0x007330C0, 0x01056F04, 40); }
// deleting: vtable 0x00B1CD48, PtrVector +20, unregistered, SmallFree(this, 44)
DECOMP(00765148, VtB1CD48_Delete) { TDeleteVecUnreg(ctx, 0x00B1CD48, 20, 0x00B0DAAC, 0x007330C0, 0x01056F04, 44); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B1F898, +148 / +196 freed, base vtable 0x00B1F668, Mem_FreeFrom
DECOMP(00772FF0, VtB1F898_Delete) { TDeleteFree2(ctx, 0x00B1F898, 148, 196, 0x00B1F668); }
// deleting: vtable 0x00B1F8D0, +180 / +184 freed, base vtable 0x00B1F668, Mem_FreeFrom
DECOMP(00773108, VtB1F8D0_Delete) { TDeleteFree2(ctx, 0x00B1F8D0, 180, 184, 0x00B1F668); }
// deleting: vtable 0x00B1F908, +176 / +180 freed, base vtable 0x00B1F668, Mem_FreeFrom
DECOMP(00773220, VtB1F908_Delete) { TDeleteFree2(ctx, 0x00B1F908, 176, 180, 0x00B1F668); }
// deleting: vtable 0x00B1F940, +176 / +180 freed, base vtable 0x00B1F668, Mem_FreeFrom
DECOMP(00773338, VtB1F940_Delete) { TDeleteFree2(ctx, 0x00B1F940, 176, 180, 0x00B1F668); }

// ---------------------------------------------------------------- node parameter bytes
// (node): the record byte selected by the +7 byte of the other record
DECOMP(00775228, Node_ParamByte_00775228) { TNodeParamByte(ctx, 7); }
// (node): the record byte selected by the +8 byte of the other record
DECOMP(007752BC, Node_ParamByte_007752BC) { TNodeParamByte(ctx, 8); }
// (node): the record byte selected by the +5 byte of the other record
DECOMP(00775350, Node_ParamByte_00775350) { TNodeParamByte(ctx, 5); }
// (node): the record byte selected by the +6 byte of the other record
DECOMP(00775550, Node_ParamByte_00775550) { TNodeParamByte(ctx, 6); }

// ---------------------------------------------------------------- secondary-base destructors notifying the manager
// p = this -20: five vtables (0x00B13974 at +0), then the manager notified
DECOMP(00770FA4, AdjDtor_00770FA4) { TAdjDtorVt5Notify(ctx, -20, 0x00B1F23C, 0x00B1F228, 0x00B1F26C, 0x00B1F250, 0x00B13974, -28332); }
// p = this -16: five vtables (0x00B13974 at +0), then the manager notified
DECOMP(00771044, AdjDtor_00771044) { TAdjDtorVt5Notify(ctx, -16, 0x00B1F23C, 0x00B1F228, 0x00B1F26C, 0x00B1F250, 0x00B13974, -28332); }
// p = this -12: five vtables (0x00B13974 at +0), then the manager notified
DECOMP(007710E4, AdjDtor_007710E4) { TAdjDtorVt5Notify(ctx, -12, 0x00B1F23C, 0x00B1F228, 0x00B1F26C, 0x00B1F250, 0x00B13974, -28332); }
// p = this -8: five vtables (0x00B13974 at +0), then the manager notified
DECOMP(00771184, AdjDtor_00771184) { TAdjDtorVt5Notify(ctx, -8, 0x00B1F23C, 0x00B1F228, 0x00B1F26C, 0x00B1F250, 0x00B13974, -28332); }

// ---------------------------------------------------------------- state checks through virtuals
// if 0x000E0BCC(this): r = vfn[8](this, 0); r 0 / 1: vfn[7](this, 0, 2)
DECOMP(007444B8, Obj_TestVCall_007444B8) { TTestVCall2(ctx, 0x000E0BCC); }
// if 0x000E0BCC(this): r = vfn[8](this, 0); r 0 / 1: vfn[7](this, 0, 2)
DECOMP(0074455C, Obj_TestVCall_0074455C) { TTestVCall2(ctx, 0x000E0BCC); }
// if 0x000E0BCC(this): r = vfn[8](this, 0); r 0 / 1: vfn[7](this, 0, 2)
DECOMP(00744600, Obj_TestVCall_00744600) { TTestVCall2(ctx, 0x000E0BCC); }
// if 0x000E0BCC(this): r = vfn[8](this, 0); r 0 / 1: vfn[7](this, 0, 2)
DECOMP(007446A4, Obj_TestVCall_007446A4) { TTestVCall2(ctx, 0x000E0BCC); }

// ---------------------------------------------------------------- secondary-base destructors notifying the manager
// p = this -20: five vtables (0x00B1F298 at +0), 0x00884FB8(p), the manager notified
DECOMP(00770A78, AdjDtor_00770A78) { TAdjDtorVt5CallNotify(ctx, -20, 0x00B1F298, 0x00B1F314, 0x00B1F2D0, 0x00B1F2E4, 0x00B1F300, 0x00884FB8, -28332); }
// p = this -16: five vtables (0x00B1F298 at +0), 0x00884FB8(p), the manager notified
DECOMP(00770B1C, AdjDtor_00770B1C) { TAdjDtorVt5CallNotify(ctx, -16, 0x00B1F298, 0x00B1F314, 0x00B1F2D0, 0x00B1F2E4, 0x00B1F300, 0x00884FB8, -28332); }
// p = this -12: five vtables (0x00B1F298 at +0), 0x00884FB8(p), the manager notified
DECOMP(00770BC0, AdjDtor_00770BC0) { TAdjDtorVt5CallNotify(ctx, -12, 0x00B1F298, 0x00B1F314, 0x00B1F2D0, 0x00B1F2E4, 0x00B1F300, 0x00884FB8, -28332); }
// p = this -8: five vtables (0x00B1F298 at +0), 0x00884FB8(p), the manager notified
DECOMP(00770C64, AdjDtor_00770C64) { TAdjDtorVt5CallNotify(ctx, -8, 0x00B1F298, 0x00B1F314, 0x00B1F2D0, 0x00B1F2E4, 0x00B1F300, 0x00884FB8, -28332); }

// ---------------------------------------------------------------- factories reading a flag
// (?, cursor): Mem_Alloc(16), 0x004BB3B0(p, cursor), vtable 0x00B10198, +12 = 1, 0x004F986C(cursor, 0x8629270A, &p->+12, 1)
DECOMP(00087084, NewFlag_00087084) { TNew16ReadFlag(ctx, 0x004BB3B0, 0x00B10198, 0x004F986C, 0x8629270A); }
// (?, cursor): Mem_Alloc(16), 0x004BB3B0(p, cursor), vtable 0x00B10BE8, +12 = 1, 0x004F986C(cursor, 0x83F172B0, &p->+12, 1)
DECOMP(000968A8, NewFlag_000968A8) { TNew16ReadFlag(ctx, 0x004BB3B0, 0x00B10BE8, 0x004F986C, 0x83F172B0); }
// (?, cursor): Mem_Alloc(16), 0x004BB3B0(p, cursor), vtable 0x00B11800, +12 = 1, 0x004F986C(cursor, 0xF4798D88, &p->+12, 1)
DECOMP(000A45EC, NewFlag_000A45EC) { TNew16ReadFlag(ctx, 0x004BB3B0, 0x00B11800, 0x004F986C, 0xF4798D88); }
// (?, cursor): Mem_Alloc(16), 0x004BB3B0(p, cursor), vtable 0x00B11828, +12 = 1, 0x004F986C(cursor, 0xF4798D88, &p->+12, 1)
DECOMP(000A4758, NewFlag_000A4758) { TNew16ReadFlag(ctx, 0x004BB3B0, 0x00B11828, 0x004F986C, 0xF4798D88); }

// ---------------------------------------------------------------- second-base destructors
// p = this -384: vtable 0x00B12898, the +388 PtrVector* freed, base vtable 0x00B12550, 0x000D7024(p)
DECOMP(007479D0, AdjDtor_007479D0) { TAdjDtorOwnVecPtr(ctx, -384); }
// p = this -380: vtable 0x00B12898, the +388 PtrVector* freed, base vtable 0x00B12550, 0x000D7024(p)
DECOMP(00747A88, AdjDtor_00747A88) { TAdjDtorOwnVecPtr(ctx, -380); }
// p = this -384: vtable 0x00B12898, the +388 PtrVector* freed, base vtable 0x00B12550, 0x000D7024(p)
DECOMP(00747FF4, AdjDtor_00747FF4) { TAdjDtorOwnVecPtr(ctx, -384); }
// p = this -380: vtable 0x00B12898, the +388 PtrVector* freed, base vtable 0x00B12550, 0x000D7024(p)
DECOMP(007480AC, AdjDtor_007480AC) { TAdjDtorOwnVecPtr(ctx, -380); }

// ---------------------------------------------------------------- command records (kind 6)
// (u16 v, Ctx** pc): a kind-6 record
DECOMP(0077565C, Cmd_PushOp6_0077565C) { TCmdPushOp6(ctx); }
// (u16 v, Ctx** pc): a kind-6 record
DECOMP(00775794, Cmd_PushOp6_00775794) { TCmdPushOp6(ctx); }
// (u16 v, Ctx** pc): a kind-6 record
DECOMP(007758CC, Cmd_PushOp6_007758CC) { TCmdPushOp6(ctx); }
// (u16 v, Ctx** pc): a kind-6 record
DECOMP(00775A04, Cmd_PushOp6_00775A04) { TCmdPushOp6(ctx); }

// ---------------------------------------------------------------- factories binding two properties
// (?, cursor): Mem_Alloc(20), 0x004BB3B0(p, cursor), vtable 0x00B0F388, 0x004FA14C(cursor, 0x8A93973F / 0x7A30D7E3, &p->+12 / +16)
DECOMP(00070040, NewBind2_00070040) { TNew20Bind2(ctx, 0x004BB3B0, 0x00B0F388, 0x004FA14C, 0x8A93973F, 0x7A30D7E3); }
// (?, cursor): Mem_Alloc(20), 0x004BB3B0(p, cursor), vtable 0x00B0F400, 0x004FA14C(cursor, 0x8C3DFCAA / 0x03D1FCFC, &p->+12 / +16)
DECOMP(00071C7C, NewBind2_00071C7C) { TNew20Bind2(ctx, 0x004BB3B0, 0x00B0F400, 0x004FA14C, 0x8C3DFCAA, 0x03D1FCFC); }
// (?, cursor): Mem_Alloc(20), 0x004BB3B0(p, cursor), vtable 0x00B105A8, 0x004FA14C(cursor, 0x8A93973F / 0x7A30D7E3, &p->+12 / +16)
DECOMP(0008B558, NewBind2_0008B558) { TNew20Bind2(ctx, 0x004BB3B0, 0x00B105A8, 0x004FA14C, 0x8A93973F, 0x7A30D7E3); }
// (?, cursor): Mem_Alloc(20), 0x004BB3B0(p, cursor), vtable 0x00B11050, 0x004FA14C(cursor, 0xE7E52D0D / 0x523827DF, &p->+12 / +16)
DECOMP(0009BCEC, NewBind2_0009BCEC) { TNew20Bind2(ctx, 0x004BB3B0, 0x00B11050, 0x004FA14C, 0xE7E52D0D, 0x523827DF); }

// ---------------------------------------------------------------- 64-byte tables read from a file
// (reader): the table at 0x010687D0 read (64 bytes after a dropped u8), 0x01068720 = &table[1]
DECOMP(002E9EBC, Table64_Load_002E9EBC) { TLoadTable64(ctx, 0x010687C8, 0x010687D0, 0x01068720); }
// (reader): the table at 0x010C2708 read (64 bytes after a dropped u8), 0x00F65FEC = &table[1]
DECOMP(004B2B50, Table64_Load_004B2B50) { TLoadTable64(ctx, 0x010C2700, 0x010C2708, 0x00F65FEC); }
// (reader): the table at 0x010C2D20 read (64 bytes after a dropped u8), 0x00F668D4 = &table[1]
DECOMP(004F4F90, Table64_Load_004F4F90) { TLoadTable64(ctx, 0x010C2D18, 0x010C2D20, 0x00F668D4); }
// (reader): the table at 0x010C2D70 read (64 bytes after a dropped u8), 0x00F668D4 = &table[1]
DECOMP(004F51F8, Table64_Load_004F51F8) { TLoadTable64(ctx, 0x010C2D68, 0x010C2D70, 0x00F668D4); }

// ---------------------------------------------------------------- their deleting versions
// deleting: p = this -384, as AdjDtor, then 0x0054EE6C(p, 0)
DECOMP(00747830, AdjDelete_00747830) { TAdjDeleteOwnVecPtr(ctx, -384, 0x0054EE6C, 0); }
// deleting: p = this -380, as AdjDtor, then 0x0054EE6C(p, 0)
DECOMP(00747900, AdjDelete_00747900) { TAdjDeleteOwnVecPtr(ctx, -380, 0x0054EE6C, 0); }
// deleting: p = this -384, as AdjDtor, then 0x00552420(p, 432)
DECOMP(00747E54, AdjDelete_00747E54) { TAdjDeleteOwnVecPtr(ctx, -384, 0x00552420, 432); }
// deleting: p = this -380, as AdjDtor, then 0x00552420(p, 432)
DECOMP(00747F24, AdjDelete_00747F24) { TAdjDeleteOwnVecPtr(ctx, -380, 0x00552420, 432); }

// ---------------------------------------------------------------- second-base destructors (three levels)
// p = this -384: vtable 0x00B15310, PtrVector +424, then the +388 PtrVector* class, 0x000DF934(p)
DECOMP(00751750, AdjDtor_00751750) { TAdjDtorVecOwnVecPtr(ctx, -384, 0x00B15310, 0x00B154BC, 0x00B154E4); }
// p = this -380: vtable 0x00B15310, PtrVector +424, then the +388 PtrVector* class, 0x000DF934(p)
DECOMP(0075184C, AdjDtor_0075184C) { TAdjDtorVecOwnVecPtr(ctx, -380, 0x00B15310, 0x00B154BC, 0x00B154E4); }
// p = this -384: vtable 0x00B18038, PtrVector +424, then the +388 PtrVector* class, 0x000DF934(p)
DECOMP(00757110, AdjDtor_00757110) { TAdjDtorVecOwnVecPtr(ctx, -384, 0x00B18038, 0x00B181E4, 0x00B1820C); }
// p = this -380: vtable 0x00B18038, PtrVector +424, then the +388 PtrVector* class, 0x000DF934(p)
DECOMP(0075720C, AdjDtor_0075720C) { TAdjDtorVecOwnVecPtr(ctx, -380, 0x00B18038, 0x00B181E4, 0x00B1820C); }

// ---------------------------------------------------------------- their deleting versions
// deleting: p = this -384, as AdjDtor (vtable 0x00B15310), SmallFree(p, 480)
DECOMP(00751520, AdjDelete_00751520) { TAdjDeleteVecOwnVecPtr(ctx, -384, 0x00B15310, 0x00B154BC, 0x00B154E4, 480); }
// deleting: p = this -380, as AdjDtor (vtable 0x00B15310), SmallFree(p, 480)
DECOMP(00751638, AdjDelete_00751638) { TAdjDeleteVecOwnVecPtr(ctx, -380, 0x00B15310, 0x00B154BC, 0x00B154E4, 480); }
// deleting: p = this -384, as AdjDtor (vtable 0x00B18038), SmallFree(p, 624)
DECOMP(00756EE0, AdjDelete_00756EE0) { TAdjDeleteVecOwnVecPtr(ctx, -384, 0x00B18038, 0x00B181E4, 0x00B1820C, 624); }
// deleting: p = this -380, as AdjDtor (vtable 0x00B18038), SmallFree(p, 624)
DECOMP(00756FF8, AdjDelete_00756FF8) { TAdjDeleteVecOwnVecPtr(ctx, -380, 0x00B18038, 0x00B181E4, 0x00B1820C, 624); }

// ---------------------------------------------------------------- pooled record factories
// (a0, pool, a2): a kind-5 76-byte record from the pool, 0x004B42CC / 0x004B46A8 on its +4
DECOMP(004B49E0, Pool_NewRec_004B49E0) { TPoolNewRec76(ctx, 0x004B42CC, 0x004B46A8, 5); }
// (a0, pool, a2): a kind-7 76-byte record from the pool, 0x004B4AEC / 0x004B4EC8 on its +4
DECOMP(004B53CC, Pool_NewRec_004B53CC) { TPoolNewRec76(ctx, 0x004B4AEC, 0x004B4EC8, 7); }
// (a0, pool, a2): a kind-11 76-byte record from the pool, 0x004B54D0 / 0x004B5918 on its +4
DECOMP(004B5C70, Pool_NewRec_004B5C70) { TPoolNewRec76(ctx, 0x004B54D0, 0x004B5918, 11); }
// (a0, pool, a2): a kind-9 76-byte record from the pool, 0x004B5D78 / 0x004B6120 on its +4
DECOMP(004B6658, Pool_NewRec_004B6658) { TPoolNewRec76(ctx, 0x004B5D78, 0x004B6120, 9); }

// ---------------------------------------------------------------- size queries
// (Vec2* out, o): child, screen or own size scaled by +168 / +172
DECOMP(004E4E78, Obj_GetSize_004E4E78) { TGetSize2(ctx, 168, 172); }
// (Vec2* out, o): child, screen or own size scaled by +240 / +244
DECOMP(004E80A8, Obj_GetSize_004E80A8) { TGetSize2(ctx, 240, 244); }
// (Vec2* out, o): child, screen or own size scaled by +168 / +172
DECOMP(004E8E3C, Obj_GetSize_004E8E3C) { TGetSize2(ctx, 168, 172); }
// (Vec2* out, o): child, screen or own size scaled by +168 / +172
DECOMP(004E971C, Obj_GetSize_004E971C) { TGetSize2(ctx, 168, 172); }

// ---------------------------------------------------------------- their deleting versions
// deleting: ~T() (vtable 0x00B19558), then 0x0054EE6C(this, 0)
DECOMP(0020D478, VtB19558_Delete) { TDeleteVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C, 0x0054EE6C, 0); }
// deleting: ~T() (vtable 0x00B19558), then 0x00552420(this, 432)
DECOMP(00759FB4, VtB19558_Delete_00759FB4) { TDeleteVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C, 0x00552420, 432); }
// deleting: ~T() (vtable 0x00B19558), then 0x0054EE6C(this, 0)
DECOMP(0075A250, VtB19558_Delete_0075A250) { TDeleteVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C, 0x0054EE6C, 0); }
// deleting: ~T() (vtable 0x00B19558), then 0x00552420(this, 688)
DECOMP(0075BC54, VtB19558_Delete_0075BC54) { TDeleteVolumeShape(ctx, 0x00B19558, 0x00F557EC, 0x000112D8, 0x00638498, 0x000D729C, 0x00552420, 688); }

// ---------------------------------------------------------------- slot free lists
// (list, s32 n): the slots moved to an n-slot block, the new ones chained onto the free list
DECOMP(00784FE4, SlotList_Grow_00784FE4) { TSlotListGrow(ctx); }
// (list, s32 n): the slots moved to an n-slot block, the new ones chained onto the free list
DECOMP(0079A528, SlotList_Grow_0079A528) { TSlotListGrow(ctx); }
// (list, s32 n): the slots moved to an n-slot block, the new ones chained onto the free list
DECOMP(0079B3B0, SlotList_Grow_0079B3B0) { TSlotListGrow(ctx); }
// (list, s32 n): the slots moved to an n-slot block, the new ones chained onto the free list
DECOMP(0079BB70, SlotList_Grow_0079BB70) { TSlotListGrow(ctx); }

// ---------------------------------------------------------------- element transforms from the object matrices
// (o): the flagged object matrices to the (o+8)->+196 elements (slots 20 / 152 / 156)
DECOMP(00376A20, Elems_Sync_00376A20) { TSyncElems(ctx, 200, 196, 220, 0x00376A18); }
// (o): the flagged object matrices to the (o+8)->+184 elements (slots 20 / 152 / 156)
DECOMP(00379960, Elems_Sync_00379960) { TSyncElems(ctx, 188, 184, 232, 0x0037995C); }
// (o): the flagged object matrices to the (o+8)->+180 elements (slots 20 / 152 / 156)
DECOMP(0037DED8, Elems_Sync_0037DED8) { TSyncElems(ctx, 184, 180, 232, 0x0037DED4); }
// (o): the flagged object matrices to the (o+8)->+180 elements (slots 20 / 152 / 156)
DECOMP(00380CF8, Elems_Sync_00380CF8) { TSyncElems(ctx, 184, 180, 232, 0x00380CF4); }

// ---------------------------------------------------------------- hit reactions (impulse vector through VMX, then a virtual)
// (o, const Params* p): impulse to the +388 component, slot 352 reaction, p+14 |= 0x100
DECOMP(00168BD0, Obj_HitImpulse_00168BD0) { THitImpulse(ctx, 0x00168B90, 388, 208, 204, 0x000E0BCC); }
// (o, const Params* p): impulse to the +388 component, slot 352 reaction, p+14 |= 0x100
DECOMP(00178F20, Obj_HitImpulse_00178F20) { THitImpulse(ctx, 0x00178EE0, 388, 212, 208, 0x000E0BCC); }
// (o, const Params* p): impulse to the +388 component, slot 352 reaction, p+14 |= 0x100
DECOMP(001850F0, Obj_HitImpulse_001850F0) { THitImpulse(ctx, 0x001850B0, 388, 208, 204, 0x000E0BCC); }
// (o, const Params* p): impulse to the +416 component, slot 352 reaction, p+14 |= 0x100
DECOMP(001FBF80, Obj_HitImpulse_001FBF80) { THitImpulse(ctx, 0x001FBF40, 416, 244, 240, 0x000E0BCC); }

// ---------------------------------------------------------------- member sub-object addresses
// this->+60 + 80
DECOMP(00452024, Obj_SubAddr_00452024) { TFieldPlus(ctx, 60, 80); }
// this->+4 + 80
DECOMP(00734CB8, Obj_SubAddr_00734CB8) { TFieldPlus(ctx, 4, 80); }
// this->+216 + 196
DECOMP(0074C5D4, Obj_SubAddr_0074C5D4) { TFieldPlus(ctx, 216, 196); }

// ---------------------------------------------------------------- f32 setters through a member
// q = this->+32; if q: q->+8 = f1
DECOMP(00354150, Obj_SetF32Via_00354150) { TSetF32Via(ctx, 32, 8); }
// q = this->+36; if q: q->+60 = f1
DECOMP(00354184, Obj_SetF32Via_00354184) { TSetF32Via(ctx, 36, 60); }
// q = this->+36; if q: q->+64 = f1
DECOMP(00354254, Obj_SetF32Via_00354254) { TSetF32Via(ctx, 36, 64); }

// ---------------------------------------------------------------- field reads through a pointer
// q = this->+24; q ? (s32)q->+4 : 0
DECOMP(004B9BD8, Obj_GetVia_004B9BD8) { TLoadChainS32(ctx, 24, 4); }
// q = this->+12; q ? (s32)q->+4 : 0
DECOMP(004B9BF8, Obj_GetVia_004B9BF8) { TLoadChainS32(ctx, 12, 4); }
// q = this->+16; q ? (s32)q->+4 : 0
DECOMP(004B9C18, Obj_GetVia_004B9C18) { TLoadChainS32(ctx, 16, 4); }

// ---------------------------------------------------------------- wrappers passing 0
// 0x001231B8(this, 0)
DECOMP(0012D354, Wrapper0_0012D354) { TCallThis0(ctx, 0x001231B8); }
// 0x001EE680(this, 0)
DECOMP(001EE940, Wrapper0_001EE940) { TCallThis0(ctx, 0x001EE680); }
// 0x002D0BA0(this, 0)
DECOMP(002D263C, Wrapper0_002D263C) { TCallThis0(ctx, 0x002D0BA0); }

// ---------------------------------------------------------------- vector copies
// q = this->+32: q->+992..1000 = (q->+872)->+32..40
DECOMP(00313274, Obj_CopyVec3_00313274) { TCopyVec3Via872(ctx); }
// q = this->+32: q->+992..1000 = (q->+872)->+32..40
DECOMP(00314200, Obj_CopyVec3_00314200) { TCopyVec3Via872(ctx); }
// q = this->+32: q->+992..1000 = (q->+872)->+32..40
DECOMP(00316524, Obj_CopyVec3_00316524) { TCopyVec3Via872(ctx); }

// ---------------------------------------------------------------- f32 setters
// this->+72 = f1
DECOMP(007361E4, Obj_SetF32_007361E4) { TStoreF1At(ctx, 72); }
// this->+316 = f1
DECOMP(00738E00, Obj_SetF32_00738E00) { TStoreF1At(ctx, 316); }
// this->+176 = f1
DECOMP(00795E74, Obj_SetF32_00795E74) { TStoreF1At(ctx, 176); }

// ---------------------------------------------------------------- flag setters (u16)
// this->+394 |= 0x00000400
DECOMP(001BD440, Obj_SetFlag16_001BD440) { TOrU16(ctx, 394, 0x00000400); }
// this->+182 |= 0x00002000
DECOMP(007497E0, Obj_SetFlag16_007497E0) { TOrU16(ctx, 182, 0x00002000); }
// this->+182 |= 0x00002000
DECOMP(007497F0, Obj_SetFlag16_007497F0) { TOrU16(ctx, 182, 0x00002000); }

// ---------------------------------------------------------------- field compares
// this->+20 == 2
DECOMP(002FBB30, Obj_FieldIs_002FBB30) { TFieldIsK(ctx, 20, -2); }
// this->+4 == 43
DECOMP(007390CC, Obj_FieldIs_007390CC) { TFieldIsK(ctx, 4, -43); }
// this->+4 == 44
DECOMP(007390E0, Obj_FieldIs_007390E0) { TFieldIsK(ctx, 4, -44); }

// ---------------------------------------------------------------- 8-byte copies
// this->+32[0..1] = a1->+0[0..1]
DECOMP(00464578, Copy8_00464578) { TCopyFields2(ctx, 32, 0, 2); }
// this->+72[0..1] = a1->+0[0..1]
DECOMP(00748FF8, Copy8_00748FF8) { TCopyFields2(ctx, 72, 0, 2); }
// this->+0[0..1] = a1->+240[0..1]
DECOMP(00796538, Copy8_00796538) { TCopyFields2(ctx, 0, 240, 2); }

// ---------------------------------------------------------------- flag tests through a member
// (this->+216)->+372 & 0x00008000
DECOMP(000EFFC8, Obj_FlagVia_000EFFC8) { TFlagVia(ctx, 216, 372, 0x00008000); }
// (this->+208)->+112 & 0x80000000
DECOMP(0074CFE8, Obj_FlagVia_0074CFE8) { TFlagVia(ctx, 208, 112, 0x80000000); }
// (this->+216)->+452 & 0x08000000
DECOMP(0074E738, Obj_FlagVia_0074E738) { TFlagVia(ctx, 216, 452, 0x08000000); }

// ---------------------------------------------------------------- vector copies
// dst[0..2] = a1->+144[0..2]
DECOMP(00185AC8, Copy12_00185AC8) { TCopyFieldsR4(ctx, 144, 3); }
// dst[0..2] = a1->+144[0..2]
DECOMP(00743754, Copy12_00743754) { TCopyFieldsR4(ctx, 144, 3); }
// dst[0..2] = a1->+144[0..2]
DECOMP(0075DB90, Copy12_0075DB90) { TCopyFieldsR4(ctx, 144, 3); }

// ---------------------------------------------------------------- flag tests
// this->+300 & (0x40000000 | 0x20000000)
DECOMP(0074C79C, Obj_AnyFlag_0074C79C) { TAnyBit2(ctx, 300, 0x40000000, 0x20000000); }
// this->+172 & (0x00000040 | 0x00000020)
DECOMP(00760810, Obj_AnyFlag_00760810) { TAnyBit2(ctx, 172, 0x00000040, 0x00000020); }
// this->+532 & (0x02000000 | 0x01000000)
DECOMP(00772808, Obj_AnyFlag_00772808) { TAnyBit2(ctx, 532, 0x02000000, 0x01000000); }

// ---------------------------------------------------------------- clamped f32 setters
// this->+168 = f1 clamped by the constants at 0x00735A70
DECOMP(00735A90, Obj_SetClamped_00735A90) { TClampF1(ctx, 0x00735A70, 168); }
// this->+196 = f1 clamped by the constants at 0x00735B08
DECOMP(00735B28, Obj_SetClamped_00735B28) { TClampF1(ctx, 0x00735B08, 196); }
// this->+48 = f1 clamped by the constants at 0x00761710
DECOMP(00761730, Obj_SetClamped_00761730) { TClampF1(ctx, 0x00761710, 48); }

// ---------------------------------------------------------------- definition lookups by key
// (key): *0x007335C4((*0x00F8F158)->+16, &key)
DECOMP(0074D21C, Def_Get_0074D21C) { THolderMapGet(ctx, 0x00F8F158, 0x007335C4); }
// (key): *0x0073343C((*0x00F8F154)->+16, &key)
DECOMP(00772838, Def_Get_00772838) { THolderMapGet(ctx, 0x00F8F154, 0x0073343C); }
// (key): *0x0073374C((*0x00F8F134)->+16, &key)
DECOMP(0077CEBC, Def_Get_0077CEBC) { THolderMapGet(ctx, 0x00F8F134, 0x0073374C); }

// ---------------------------------------------------------------- virtual calls on a member
// (this->+24)->vfn[5]()
DECOMP(0001819C, Obj_MemberVCall_0001819C) { TVCallFieldNC(ctx, 24, 20); }
// (this->+24)->vfn[7]()
DECOMP(000181DC, Obj_MemberVCall_000181DC) { TVCallFieldNC(ctx, 24, 28); }
// (this->+216)->vfn[4]()
DECOMP(00760E54, Obj_MemberVCall_00760E54) { TVCallFieldNC(ctx, 216, 16); }
