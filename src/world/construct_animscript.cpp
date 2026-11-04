// The anim-script registries World_Construct fills: AVL<u32, ApeAnimScriptCommand*>
// at 0x010C2820 and AVL<u32, ApeAnimScriptVar*> at 0x010C27B8, keyed by
// String_HashNoCase of the type name, and the type objects' constructors.
//
// The per-type constructors below were translated mechanically from
// straight-line code by a small script and spot-checked by hand. Names in quotes are strings in the EBOOT whose hash matches the key;
// the others are named by their hash. lfs/stfs pairs are copied as raw words.
#include <bit>

#include "construct.h"

using namespace sly4;
using namespace sly4::wcons;

namespace {

// AVL_Insert(tree, key, value): the root-insert half of the template; a
// non-empty tree goes to the recursive insert. The node allocator is
// created on first use. As in the original, a failed allocation of the
// allocator stores 0 and then calls through it anyway.
void AvlInsert(PPUContext& ctx, u32 nodes_global, u32 fl_vtable, u32 fl_name, u32 insert_at) {
    const u32 tree = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), value = arg<u32>(ctx, 2);
    store<u32>(tree + 4, load<u32>(tree + 4) + 1);
    if (load<u32>(tree) != 0) {
        call(ctx, insert_at, {tree, key, value, tree});
        return;
    }
    u32 fl = load<u32>(nodes_global);
    if (fl == 0) {
        const u32 mem = static_cast<u32>(call(ctx, kfn::Mem_Alloc, {60, 0, 16}));
        if (mem != 0) {
            store<u32>(mem + 8, 0);
            store<u32>(mem + 4, 0);
            store<u32>(mem + 12, 0);
            store<u32>(mem + 20, 0);
            store<u32>(mem + 16, 0);
            store<u32>(mem + 24, 0);
            const u32 h54 = load<u16>(mem + 54) & 0x7FFFu;
            store<u32>(mem + 32, 0);
            store<u32>(mem + 28, 0);
            store<u32>(mem + 36, 0);
            store<u32>(mem + 40, 0);
            store<u32>(mem + 44, 0);
            store<u32>(mem + 48, 0);
            store<u16>(mem + 52, 0);
            store<u16>(mem + 54, static_cast<u16>(h54));
            store<u32>(mem, fl_vtable);
            store<u32>(mem + 56, fl_name);
            call(ctx, kfn::FreeList_Init, {mem, 20, 64, 0, 0});
        }
        fl = mem;
        store<u32>(nodes_global, fl);
    }
    // Allocator vfunc +12: Alloc() -> node.
    const u32 opd = load<u32>(load<u32>(fl) + 12);
    ctx.gpr[4] = opd;
    ctx.gpr[5] = load<u32>(opd);
    const u32 node = static_cast<u32>(call_ptr(ctx, opd, {fl}));
    if (node != 0) {
        store<u16>(node, 0);
        store<u16>(node + 2, 1);
        store<u32>(node + 4, key);
        store<u32>(node + 8, value);
        store<u32>(node + 12, 0);
        store<u32>(node + 16, 0);
        ctx.gpr[3] = 0;
        ctx.gpr[4] = 1;
    }
    store<u32>(tree, node);
}

}  // namespace

// void AvlCmd_Insert(AVL* tree, u32 key, ApeAnimScriptCommand* cmd)
DECOMP(00785BC4, AvlCmd_Insert) {
    AvlInsert(ctx, kAnimCmdNodes, kFreeListVtableCmd, kFreeListNameCmd, kfn::AvlCmd_InsertAt);
}

// void AvlVar_Insert(AVL* tree, u32 key, ApeAnimScriptVar* var)
DECOMP(00786468, AvlVar_Insert) {
    AvlInsert(ctx, kAnimVarNodes, kFreeListVtableVar, kFreeListNameVar, kfn::AvlVar_InsertAt);
}

// void AnimCmd_ConstructBase(ApeAnimScriptCommand* c): vtable, +4 u16 = 1.
// Leaves r4 = the vtable and r5 = 1, as the original.
DECOMP(004B9CB8, AnimCmd_ConstructBase) {
    const u32 c = arg<u32>(ctx, 0);
    store<u16>(c + 4, 1);
    store<u32>(c, kAnimCmdVtable);
    ctx.gpr[4] = kAnimCmdVtable;
    ctx.gpr[5] = 1;
}

// void AnimCmd_Clear12_24(ApeAnimScriptCommand* c): +12..+24 = 0.
DECOMP(0007AF18, AnimCmd_Clear12_24) {
    const u32 c = arg<u32>(ctx, 0);
    for (u32 k = 12; k <= 24; k += 4) store<u32>(c + k, 0);
    ctx.gpr[4] = 0;
}

// AnimCmd (hash 0xD8D02D85), registered at 0045EA7C.
DECOMP(007868D4, AnimCmd_D8D02D85_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0F950u);
    ctx.gpr[4] = 0x00B0F950u;
    call(ctx, kfn::AnimCmd_Clear12_24, {obj});
}

// ---------------------------------------------------------------- generated

// AnimVar (hash 0x08D5E6EB), registered at 0045D67C.
DECOMP(000A7610, AnimVar_08D5E6EB_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    store<u16>(obj + 4, 1);
    store<u32>(obj, 0x00B11B38u);
    store<u32>(obj + 20, 0);
    store<u16>(obj + 22, 0);
    store<u32>(obj + 8, 0);
    store<u32>(obj + 20, 0);
}

// AnimVar "String" (hash 0x92685411), registered at 0045D6C4.
DECOMP(000A78B4, AnimVar_String_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    ref<u8>(obj + 20) = 0;
    store<u16>(obj + 4, 1);
    store<u32>(obj, 0x00B11B88u);
}

// AnimVar "Trigger" (hash 0xEB9EE198), registered at 0045D70C.
DECOMP(000A7AAC, AnimVar_Trigger_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    store<u32>(obj + 8, 0);
    store<u16>(obj + 4, 1);
    store<u32>(obj, 0x00B11BD0u);
    store<u32>(obj + 20, 0);
}

// AnimVar "spawner" (hash 0xB2E59948), registered at 0045D754.
DECOMP(000A7C84, AnimVar_Spawner_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    store<u32>(obj + 8, 0);
    store<u16>(obj + 4, 1);
    store<u32>(obj, 0x00B11C18u);
    store<u32>(obj + 20, 0);
}

// AnimVar "SpawnerGroup" (hash 0xE89DF217), registered at 0045D79C.
DECOMP(000A7E5C, AnimVar_SpawnerGroup_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    store<u32>(obj + 8, 0);
    store<u16>(obj + 4, 1);
    store<u32>(obj, 0x00B11C60u);
    store<u32>(obj + 20, 0);
}

// AnimVar "Laser" (hash 0x044DC6AB), registered at 0045D7E0.
DECOMP(000A8034, AnimVar_Laser_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    store<u32>(obj + 8, 0);
    store<u16>(obj + 4, 1);
    store<u32>(obj, 0x00B11CA8u);
    store<u32>(obj + 20, 0);
}

// AnimVar "LaserField" (hash 0xAB27550F), registered at 0045D828.
DECOMP(000A820C, AnimVar_LaserField_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    store<u32>(obj + 8, 0);
    store<u16>(obj + 4, 1);
    store<u32>(obj, 0x00B11CF0u);
    store<u32>(obj + 20, 0);
}

// AnimVar (hash 0xAB39AA94), registered at 0045D870.
DECOMP(000A83E4, AnimVar_AB39AA94_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    store<u32>(obj + 8, 0);
    store<u16>(obj + 4, 1);
    store<u32>(obj, 0x00B11D38u);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x36DF0092), registered at 0045D8C8.
DECOMP(0006B6E0, AnimCmd_36DF0092_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F068u);
    ref<u8>(obj + 20) = 1;
    ref<u8>(obj + 21) = 1;
    ref<u8>(obj + 22) = 0;
}

// AnimCmd "ActivateMixState" (hash 0x5C27C9A8), registered at 0045D914.
DECOMP(0006BAB0, AnimCmd_ActivateMixState_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B0F090u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x37A78BFD), registered at 0045D960.
DECOMP(0006BD3C, AnimCmd_37A78BFD_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F0B8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0xB09EA328), registered at 0045D9B0.
DECOMP(0006C0C8, AnimCmd_B09EA328_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F0E0u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0x3F800000u);
    store<u32>(obj + 36, 0);
}

// AnimCmd (hash 0x794FCBAC), registered at 0045D9FC.
DECOMP(0006C5B0, AnimCmd_794FCBAC_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F108u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 28) = 1;
}

// AnimCmd (hash 0xC495712B), registered at 0045DA4C.
DECOMP(0006CA38, AnimCmd_C495712B_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0F130u);
}

// AnimCmd (hash 0x81E249E9), registered at 0045DA9C.
DECOMP(0006CBB0, AnimCmd_81E249E9_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 16, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 20, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj, 0x00B0F158u);
    store<u32>(obj + 24, v3);
    const u32 v4 = load<u32>(0x01164C90u);  // lfs
    store<u32>(obj + 32, v4);
    const u32 v5 = load<u32>(0x01164C94u);  // lfs
    store<u32>(obj + 36, v5);
    const u32 v6 = load<u32>(0x01164C98u);  // lfs
    store<u32>(obj + 48, 0);
    store<u32>(obj + 40, v6);
    const u32 v7 = load<u32>(0x01164C9Cu);  // lfs
    store<u32>(obj + 52, 0);
    store<u32>(obj + 44, v7);
    store<u32>(obj + 56, 0);
    store<u32>(obj + 60, 0);
    store<u32>(obj + 64, 0);
    ref<u8>(obj + 68) = 0;
    ref<u8>(obj + 69) = 0;
    ref<u8>(obj + 70) = 0;
    ref<u8>(obj + 71) = 1;
}

// AnimCmd (hash 0x277F3268), registered at 0045DAE8.
DECOMP(0006D3A0, AnimCmd_277F3268_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 32, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 36, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 40, v3);
    const u32 v4 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 48, v4);
    const u32 v5 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 52, v5);
    const u32 v6 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj, 0x00B0F180u);
    store<u32>(obj + 56, v6);
    const u32 v7 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 12, 0);
    store<u32>(obj + 80, v7);
    const u32 v8 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 84, v8);
    const u32 v9 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 64, 0);
    store<u32>(obj + 88, v9);
    const u32 v10 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 96, v10);
    const u32 v11 = load<u32>(0x01164B44u);  // lfs
    ref<u8>(obj + 116) = 0;
    store<u32>(obj + 100, v11);
    const u32 v12 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 112, 0xBF800000u);
    store<u32>(obj + 104, v12);
    ref<u8>(obj + 117) = 0;
    ref<u8>(obj + 118) = 0;
    ref<u8>(obj + 119) = 0;
    ref<u8>(obj + 120) = 0;
    ref<u8>(obj + 121) = 0;
}

// AnimCmd "Blast" (hash 0x03C599D8), registered at 0045DB34.
DECOMP(0006DD14, AnimCmd_Blast_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 16, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 20, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj, 0x00B0F1A8u);
    store<u32>(obj + 24, v3);
    const u32 v4 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 12, 0);
    store<u32>(obj + 32, v4);
    const u32 v5 = load<u32>(0x01164B44u);  // lfs
    ref<u8>(obj + 56) = 0;
    store<u32>(obj + 36, v5);
    const u32 v6 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 48, 0);
    store<u32>(obj + 40, v6);
    store<u32>(obj + 52, 0);
    store<u32>(obj + 60, 0);
    store<u32>(obj + 64, 0);
    store<u32>(obj + 68, 0);
    store<u32>(obj + 72, 0);
    store<u32>(obj + 76, 0xFFFFFFFFu);
}

// AnimCmd (hash 0x9A91BE79), registered at 0045DB84.
DECOMP(0006E9E4, AnimCmd_9A91BE79_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 12, 0x0027469Eu);
    store<u32>(obj + 16, 0x0027469Eu);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    ref<u8>(obj + 36) = 0;
    store<u32>(obj + 40, 0);
    store<u32>(obj, 0x00B0F220u);
    store<u32>(obj + 44, 0);
    store<u32>(obj + 48, 0);
    store<u32>(obj + 52, 0);
}

// AnimCmd (hash 0xC04DBE16), registered at 0045DBD4.
DECOMP(0006ECB8, AnimCmd_C04DBE16_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0F248u);
}

// AnimCmd (hash 0xD1F7C236), registered at 0045DC24.
DECOMP(0006EDE4, AnimCmd_D1F7C236_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F270u);
    store<u32>(obj + 16, 0x3F800000u);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
}

// AnimCmd "CameraShake" (hash 0xFC244EE1), registered at 0045DC74.
DECOMP(0006EFD4, AnimCmd_CameraShake_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0x3F800000u);
    store<u32>(obj + 24, 0x3F800000u);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0x3F800000u);
    store<u32>(obj + 44, 0);
    store<u32>(obj, 0x00B0F298u);
    store<u32>(obj + 48, 0x3F800000u);
    store<u32>(obj + 12, 0);
    store<u32>(obj + 52, 0x40800000u);
    store<u32>(obj + 56, 0x40D00000u);
    store<u32>(obj + 60, 0x40200000u);
    store<u32>(obj + 64, 0);
    store<u32>(obj + 32, 0x85D98325u);
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 80, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 84, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 96, 0xBF800000u);
    store<u32>(obj + 88, v3);
}

// AnimCmd (hash 0x3D0E7A04), registered at 0045DCC0.
DECOMP(0006F614, AnimCmd_3D0E7A04_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj, 0x00B0F2E8u);
}

// AnimCmd (hash 0x7F37BF6C), registered at 0045DD0C.
DECOMP(0006FCE0, AnimCmd_7F37BF6C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0F338u);
}

// AnimCmd (hash 0xF7635E87), registered at 0045DD5C.
DECOMP(0006FE0C, AnimCmd_F7635E87_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F360u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xB8B01DB7), registered at 0045DDAC.
DECOMP(0006F768, AnimCmd_B8B01DB7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F310u);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 32) = 1;
}

// AnimCmd (hash 0xCB291F57), registered at 0045DDFC.
DECOMP(0006FFF0, AnimCmd_CB291F57_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F388u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xA1D6F663), registered at 0045DE4C.
DECOMP(0006F4B8, AnimCmd_A1D6F663_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0F2C0u);
}

// AnimCmd "Damage" (hash 0x77C17F6F), registered at 0045DEE8.
DECOMP(000717B8, AnimCmd_Damage_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F3D8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x852DA936), registered at 0045DF38.
DECOMP(00071C2C, AnimCmd_852DA936_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F400u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xE127428A), registered at 0045DF88.
DECOMP(00071ED4, AnimCmd_E127428A_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F428u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xEFE548E4), registered at 0045DFD8.
DECOMP(000720C0, AnimCmd_EFE548E4_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F450u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0xBF800000u);
    store<u32>(obj + 32, 0x40000000u);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0x3FC90FDBu);
}

// AnimCmd (hash 0xB71440EF), registered at 0045E028.
DECOMP(0007240C, AnimCmd_B71440EF_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 80, 0);
    store<u32>(obj, 0x00B0F478u);
    store<u32>(obj + 84, 0);
    store<u32>(obj + 88, 0);
    ref<u8>(obj + 92) = 0;
    ref<u8>(obj + 12) = 0;
}

// AnimCmd (hash 0xF32016BA), registered at 0045E078.
DECOMP(000727DC, AnimCmd_F32016BA_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F4A0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 28) = 0;
}

// AnimCmd (hash 0xC99BEAB1), registered at 0045E0C8.
DECOMP(00072B64, AnimCmd_C99BEAB1_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj, 0x00B0F4C8u);
}

// AnimCmd (hash 0x4E92FD25), registered at 0045E114.
DECOMP(00072C6C, AnimCmd_4E92FD25_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F4F0u);
    store<u32>(obj + 20, 0x3F800000u);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x2BD82708), registered at 0045E160.
DECOMP(00072E38, AnimCmd_2BD82708_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F518u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    ref<u8>(obj + 36) = 0;
    ref<u8>(obj + 37) = 0;
    ref<u8>(obj + 38) = 0;
}

// AnimCmd (hash 0xC0F6B7A8), registered at 0045E1B0.
DECOMP(00073498, AnimCmd_C0F6B7A8_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B0F540u);
}

// AnimCmd (hash 0x54836B14), registered at 0045E1FC.
DECOMP(00073598, AnimCmd_54836B14_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F568u);
    ref<u8>(obj + 20) = 1;
    ref<u8>(obj + 21) = 0;
    ref<u8>(obj + 22) = 0;
    ref<u8>(obj + 23) = 0;
}

// AnimCmd (hash 0x21595665), registered at 0045E248.
DECOMP(000738B0, AnimCmd_21595665_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F590u);
    ref<u8>(obj + 20) = 1;
    ref<u8>(obj + 21) = 1;
    ref<u8>(obj + 22) = 0;
    ref<u8>(obj + 23) = 0;
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0x3F74DF1A), registered at 0045E294.
DECOMP(00073CB8, AnimCmd_3F74DF1A_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F5B8u);
    ref<u8>(obj + 20) = 1;
    ref<u8>(obj + 21) = 1;
    ref<u8>(obj + 22) = 0;
    ref<u8>(obj + 23) = 0;
}

// AnimCmd "EnqueueSound" (hash 0x011DA807), registered at 0045E2E0.
DECOMP(00074058, AnimCmd_EnqueueSound_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F5E0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
}

// AnimCmd (hash 0xDE3DFE53), registered at 0045E330.
DECOMP(000744E8, AnimCmd_DE3DFE53_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F608u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x3CED7265), registered at 0045E37C.
DECOMP(000747D8, AnimCmd_3CED7265_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F630u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
}

// AnimCmd (hash 0xA21A00C2), registered at 0045E3CC.
DECOMP(00074CDC, AnimCmd_A21A00C2_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F658u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
    ref<u8>(obj + 22) = 0;
}

// AnimCmd (hash 0x53017FA7), registered at 0045E418.
DECOMP(000750DC, AnimCmd_53017FA7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F680u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 28) = 0;
    ref<u8>(obj + 29) = 1;
}

// AnimCmd (hash 0x3E5144C6), registered at 0045E464.
DECOMP(00075424, AnimCmd_3E5144C6_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F6A8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x9D57A511), registered at 0045E4B4.
DECOMP(00075710, AnimCmd_9D57A511_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F6D0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0x1DDC65F3), registered at 0045E500.
DECOMP(00075A14, AnimCmd_1DDC65F3_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F6F8u);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
    ref<u8>(obj + 25) = 1;
    ref<u8>(obj + 26) = 1;
    ref<u8>(obj + 27) = 0;
}

// AnimCmd (hash 0x719E1647), registered at 0045E54C.
DECOMP(000763A0, AnimCmd_719E1647_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F748u);
    store<u32>(obj + 20, 0);
    const u32 v1 = ref<u8>(0x01164CA0u);
    ref<u8>(obj + 24) = static_cast<u8>(v1);
    const u32 v2 = ref<u8>(0x01164CA1u);
    ref<u8>(obj + 25) = static_cast<u8>(v2);
    const u32 v3 = ref<u8>(0x01164CA2u);
    ref<u8>(obj + 26) = static_cast<u8>(v3);
    const u32 v4 = ref<u8>(0x01164CA3u);
    ref<u8>(obj + 27) = static_cast<u8>(v4);
    const u32 v5 = ref<u8>(0x01164CA8u);
    ref<u8>(obj + 28) = static_cast<u8>(v5);
    const u32 v6 = ref<u8>(0x01164CA9u);
    ref<u8>(obj + 29) = static_cast<u8>(v6);
    const u32 v7 = ref<u8>(0x01164CAAu);
    ref<u8>(obj + 30) = static_cast<u8>(v7);
    const u32 v8 = ref<u8>(0x01164CABu);
    ref<u8>(obj + 31) = static_cast<u8>(v8);
    store<u32>(obj + 32, 0);
}

// AnimCmd (hash 0x02D3F78F), registered at 0045E598.
DECOMP(00075D14, AnimCmd_02D3F78F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F720u);
    store<u32>(obj + 16, 0x3F800000u);
    store<u32>(obj + 20, 0x3F800000u);
    const u32 v1 = ref<u8>(0x01164CA0u);
    ref<u8>(obj + 24) = static_cast<u8>(v1);
    const u32 v2 = ref<u8>(0x01164CA1u);
    ref<u8>(obj + 25) = static_cast<u8>(v2);
    const u32 v3 = ref<u8>(0x01164CA2u);
    ref<u8>(obj + 26) = static_cast<u8>(v3);
    const u32 v4 = ref<u8>(0x01164CA3u);
    ref<u8>(obj + 27) = static_cast<u8>(v4);
    const u32 v5 = ref<u8>(0x01164CA0u);
    ref<u8>(obj + 28) = static_cast<u8>(v5);
    const u32 v6 = ref<u8>(0x01164CA1u);
    ref<u8>(obj + 29) = static_cast<u8>(v6);
    const u32 v7 = ref<u8>(0x01164CA2u);
    ref<u8>(obj + 30) = static_cast<u8>(v7);
    const u32 v8 = ref<u8>(0x01164CA3u);
    ref<u8>(obj + 31) = static_cast<u8>(v8);
    store<u32>(obj + 32, 0x3F800000u);
    store<u32>(obj + 36, 2);
    store<u32>(obj + 40, 0);
    ref<u8>(obj + 44) = 1;
}

// AnimCmd (hash 0xC583D95F), registered at 0045E5E8.
DECOMP(00076ED4, AnimCmd_C583D95F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F770u);
    store<u32>(obj + 16, 0x3F800000u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 28) = 0;
}

// AnimCmd (hash 0x5DA7E559), registered at 0045E634.
DECOMP(00077284, AnimCmd_5DA7E559_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F798u);
    store<u32>(obj + 16, 0xBF800000u);
    store<u32>(obj + 20, 0xBF800000u);
    store<u32>(obj + 24, 0xBF800000u);
    store<u32>(obj + 28, 0);
}

// AnimCmd (hash 0xD5E698BB), registered at 0045E684.
DECOMP(00077630, AnimCmd_D5E698BB_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F7C0u);
    store<u32>(obj + 20, 0x3F800000u);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    ref<u8>(obj + 32) = 0;
}

// AnimCmd (hash 0x48AF45FB), registered at 0045E6D0.
DECOMP(00077A8C, AnimCmd_48AF45FB_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F7E8u);
    const u32 v1 = ref<u8>(0x01164CA0u);
    ref<u8>(obj + 16) = static_cast<u8>(v1);
    const u32 v2 = ref<u8>(0x01164CA1u);
    ref<u8>(obj + 17) = static_cast<u8>(v2);
    const u32 v3 = ref<u8>(0x01164CA2u);
    ref<u8>(obj + 18) = static_cast<u8>(v3);
    const u32 v4 = ref<u8>(0x01164CA3u);
    ref<u8>(obj + 19) = static_cast<u8>(v4);
    const u32 v5 = ref<u8>(0x01164CA8u);
    ref<u8>(obj + 20) = static_cast<u8>(v5);
    const u32 v6 = ref<u8>(0x01164CA9u);
    ref<u8>(obj + 21) = static_cast<u8>(v6);
    const u32 v7 = ref<u8>(0x01164CAAu);
    ref<u8>(obj + 22) = static_cast<u8>(v7);
    const u32 v8 = ref<u8>(0x01164CABu);
    ref<u8>(obj + 23) = static_cast<u8>(v8);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x060BD89F), registered at 0045E71C.
DECOMP(00077FE0, AnimCmd_060BD89F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 32, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 36, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 40, v3);
    const u32 v4 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj, 0x00B0F810u);
    store<u32>(obj + 64, v4);
    const u32 v5 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 12, 0);
    store<u32>(obj + 68, v5);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    const u32 v6 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 84, 0x42480000u);
    store<u32>(obj + 72, v6);
    store<u32>(obj + 48, 0);
    store<u32>(obj + 52, 0);
    const u32 v7 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 56, 0);
    store<u32>(obj + 60, 0);
    store<u32>(obj + 96, v7);
    const u32 v8 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 80, 0);
    store<u32>(obj + 100, v8);
    const u32 v9 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 112, 0);
    store<u32>(obj + 104, v9);
    store<u32>(obj + 116, 0);
    store<u32>(obj + 120, 0);
    store<u32>(obj + 124, 0);
    store<u32>(obj + 128, 0);
    store<u32>(obj + 132, 0);
    ref<u8>(obj + 136) = 0;
    ref<u8>(obj + 137) = 0;
    ref<u8>(obj + 138) = 0;
    ref<u8>(obj + 139) = 0;
    ref<u8>(obj + 140) = 0;
}

// AnimCmd (hash 0xF86D2B41), registered at 0045E76C.
DECOMP(00078E0C, AnimCmd_F86D2B41_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F838u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0x489EBD81u);
    store<u32>(obj + 36, 0x3F800000u);
    store<u32>(obj + 40, 0);
    store<u32>(obj + 44, 0);
    store<u32>(obj + 48, 0x3F800000u);
    store<u32>(obj + 52, 0x3F800000u);
    store<u32>(obj + 56, 0);
    store<u32>(obj + 60, 0x85D98325u);
    store<u32>(obj + 64, 0);
    store<u32>(obj + 68, 0x3F800000u);
    store<u32>(obj + 72, 0);
    store<u32>(obj + 76, 0x3F800000u);
    store<u32>(obj + 80, 0x40800000u);
    store<u32>(obj + 84, 0x40D00000u);
    store<u32>(obj + 88, 0x40200000u);
    store<u32>(obj + 92, 0);
    store<u32>(obj + 96, 0);
    store<u32>(obj + 100, 0);
    store<u32>(obj + 104, 0);
    store<u32>(obj + 108, 0);
    store<u32>(obj + 112, 0);
    store<u32>(obj + 116, 0);
    store<u32>(obj + 120, 0);
    store<u32>(obj + 124, 0);
    store<u32>(obj + 128, 0);
    store<u32>(obj + 132, 0);
    store<u32>(obj + 136, 0);
    store<u32>(obj + 140, 0xBF800000u);
    ref<u8>(obj + 144) = 0;
    ref<u8>(obj + 145) = 0;
    ref<u8>(obj + 146) = 0;
    ref<u8>(obj + 147) = 0;
}

// AnimCmd (hash 0x11E4A454), registered at 0045E7B8.
DECOMP(000797A4, AnimCmd_11E4A454_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0F860u);
    store<u32>(obj + 12, 0);
    const u32 v1 = ref<u8>(0x01164CA0u);
    const u32 v2 = ref<u8>(0x01164CA1u);
    const u32 v3 = ref<u8>(0x01164CA2u);
    const u32 v4 = ref<u8>(0x01164CA3u);
    store<u32>(obj + 32, 0xEB2F2138u);
    store<u32>(obj + 36, 0x3F800000u);
    store<u32>(obj + 40, 0);
    store<u32>(obj + 44, 0x3F000000u);
    store<u32>(obj + 48, 0x3F800000u);
    store<u32>(obj + 52, 0);
    store<u32>(obj + 56, 0x3F800000u);
    ref<u8>(obj + 60) = static_cast<u8>(v1);
    ref<u8>(obj + 61) = static_cast<u8>(v2);
    ref<u8>(obj + 62) = static_cast<u8>(v3);
    ref<u8>(obj + 63) = static_cast<u8>(v4);
    const u32 v5 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 80, v5);
    const u32 v6 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 84, v6);
    const u32 v7 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 88, v7);
    const u32 v8 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 96, v8);
    const u32 v9 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 100, v9);
    const u32 v10 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 104, v10);
    const u32 v11 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 112, v11);
    const u32 v12 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 116, v12);
    const u32 v13 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 120, v13);
    const u32 v14 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 128, v14);
    const u32 v15 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 64, 2);
    store<u32>(obj + 132, v15);
    const u32 v16 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 68, 0);
    store<u32>(obj + 136, v16);
    const u32 v17 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 72, 0);
    store<u32>(obj + 144, v17);
    const u32 v18 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 148, v18);
    const u32 v19 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 152, v19);
    const u32 v20 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 160, v20);
    const u32 v21 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 164, v21);
    const u32 v22 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 176, 0x42960000u);
    store<u32>(obj + 168, v22);
    store<u32>(obj + 180, 0xBF800000u);
    store<u32>(obj + 184, 0x3D4CCCCDu);
    store<u32>(obj + 188, 0xBF800000u);
    ref<u8>(obj + 192) = 0;
    ref<u8>(obj + 193) = 0;
    ref<u8>(obj + 194) = 0;
    store<u32>(obj + 196, 1);
}

// AnimCmd "Gadget" (hash 0x7CDC084C), registered at 0045E804.
DECOMP(0007A084, AnimCmd_Gadget_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(obj + 24);
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F888u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, ((v1 & 0x7FFFFFFFu) & 0xBFFFFFFFu));
}

// AnimCmd (hash 0xE23B043C), registered at 0045E854.
DECOMP(0007A38C, AnimCmd_E23B043C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0F8B0u);
    store<u32>(obj + 20, 9);
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0x55C11A40), registered at 0045E8A0.
DECOMP(0007A6E8, AnimCmd_55C11A40_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F8D8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x599B5141), registered at 0045E8EC.
DECOMP(0007AAA8, AnimCmd_599B5141_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F900u);
}

// AnimCmd (hash 0x9929642A), registered at 0045E93C.
DECOMP(0007B420, AnimCmd_9929642A_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F9A0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x98474D43), registered at 0045E98C.
DECOMP(0007B760, AnimCmd_98474D43_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F9C8u);
}

// AnimCmd (hash 0xC010ED68), registered at 0045E9DC.
DECOMP(0007B974, AnimCmd_C010ED68_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F9F0u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xE5B13789), registered at 0045EACC.
DECOMP(0007B218, AnimCmd_E5B13789_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0F978u);
}

// AnimCmd (hash 0x8D10E0C0), registered at 0045EB1C.
DECOMP(0007BC80, AnimCmd_8D10E0C0_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0FA18u);
    store<u32>(obj + 20, 0xFFFFFFFFu);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0);
}

// AnimCmd (hash 0xCFC9EE8E), registered at 0045EB6C.
DECOMP(0007C298, AnimCmd_CFC9EE8E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FA40u);
}

// AnimCmd (hash 0x9C86E926), registered at 0045EBBC.
DECOMP(0007C4FC, AnimCmd_9C86E926_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FA68u);
}

// AnimCmd (hash 0xFD0361CC), registered at 0045EC0C.
DECOMP(0007C714, AnimCmd_FD0361CC_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xFFFFFFFFu);
    store<u32>(obj, 0x00B0FA90u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0x553CAE39), registered at 0045EC58.
DECOMP(0007C918, AnimCmd_553CAE39_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FAB8u);
    store<u32>(obj + 48, v1);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 52, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 56, v3);
}

// AnimCmd (hash 0xD22F2156), registered at 0045ECA8.
DECOMP(0007CEFC, AnimCmd_D22F2156_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0FAE0u);
    ref<u8>(obj + 20) = 9;
    ref<u8>(obj + 21) = 1;
}

// AnimCmd (hash 0xE573697F), registered at 0045ECF8.
DECOMP(0007D154, AnimCmd_E573697F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FB08u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0x742543CF), registered at 0045ED44.
DECOMP(0007D340, AnimCmd_742543CF_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 48, 0);
    store<u32>(obj + 52, 0);
    store<u32>(obj, 0x00B0FB30u);
    store<u32>(obj + 56, 0);
    store<u32>(obj + 60, 0xBF800000u);
    store<u32>(obj + 64, 0);
    ref<u8>(obj + 12) = 0;
}

// AnimCmd (hash 0xD1CD23E6), registered at 0045ED94.
DECOMP(0007D678, AnimCmd_D1CD23E6_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0FB58u);
    store<u32>(obj + 20, 0xBF800000u);
}

// AnimCmd (hash 0x244FEDA4), registered at 0045EDE0.
DECOMP(0007D83C, AnimCmd_244FEDA4_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FB80u);
    store<u32>(obj + 16, 0);
}

// AnimCmd "IfCollected" (hash 0x57A7C2AC), registered at 0045EE2C.
DECOMP(0007D9B4, AnimCmd_IfCollected_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0FBA8u);
    call(ctx, kfn::PtrVector_Construct, {obj + 24});
    store<u16>(obj + 30, 4);
    store<u32>(obj + 12, 0);
    ref<u8>(obj + 20) = 0;
    store<u32>(obj + 16, 0);
}

// AnimCmd "IfEntity" (hash 0x905AD740), registered at 0045EE7C.
DECOMP(0007E0A0, AnimCmd_IfEntity_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0FBD0u);
    call(ctx, kfn::PtrVector_Construct, {obj + 28});
    store<u16>(obj + 34, 4);
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 48) = 0;
    ref<u8>(obj + 49) = 0;
}

// AnimCmd (hash 0x0425A560), registered at 0045EEC8.
DECOMP(0007E8D4, AnimCmd_0425A560_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FBF8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 1);
    call(ctx, kfn::PtrVector_Construct, {obj + 24});
    store<u16>(obj + 30, 4);
}

// AnimCmd (hash 0xBFB3BF12), registered at 0045EF18.
DECOMP(0007EF7C, AnimCmd_BFB3BF12_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0FC20u);
    call(ctx, kfn::PtrVector_Construct, {obj + 12});
    store<u16>(obj + 18, 4);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    ref<u8>(obj + 40) = 0;
}

// AnimCmd "Impulse" (hash 0x9D9B30D5), registered at 0045EF68.
DECOMP(0007F5F0, AnimCmd_Impulse_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(obj + 40);
    const u32 v2 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 16, v2);
    const u32 v3 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj, 0x00B0FC48u);
    store<u32>(obj + 20, v3);
    const u32 v4 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 32, 0);
    store<u32>(obj + 24, v4);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, ((((v1 & 0x7FFFFFFFu) & 0xBFFFFFFFu) & 0xDFFFFFFFu) & 0xEFFFFFFFu));
}

// AnimCmd (hash 0x70542137), registered at 0045EFB4.
DECOMP(0007FFA8, AnimCmd_70542137_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0FC70u);
    store<u32>(obj + 24, 0xBF800000u);
    ref<u8>(obj + 28) = 0;
    ref<u8>(obj + 29) = 0;
    ref<u8>(obj + 30) = 0;
}

// AnimCmd (hash 0x96EE417F), registered at 0045F004.
DECOMP(000802E0, AnimCmd_96EE417F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0x3F800000u);
    store<u32>(obj, 0x00B0FC98u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0x3F800000u);
}

// AnimCmd (hash 0x78811B3A), registered at 0045F050.
DECOMP(0008058C, AnimCmd_78811B3A_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0FCC0u);
}

// AnimCmd (hash 0x0023335E), registered at 0045F09C.
DECOMP(00080720, AnimCmd_0023335E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FCE8u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
}

// AnimCmd (hash 0xBF775D1F), registered at 0045F0EC.
DECOMP(00080A88, AnimCmd_BF775D1F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FD10u);
}

// AnimCmd (hash 0xF99C063E), registered at 0045F13C.
DECOMP(00080B9C, AnimCmd_F99C063E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FD38u);
}

// AnimCmd "MapEntityToVar" (hash 0xB45C81ED), registered at 0045F18C.
DECOMP(00080CF8, AnimCmd_MapEntityToVar_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FD60u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0);
    store<u32>(obj + 44, 0);
    store<u32>(obj + 48, 0);
}

// AnimCmd (hash 0xB54A6E29), registered at 0045F1DC.
DECOMP(000812C0, AnimCmd_B54A6E29_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FD88u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x27D06D5B), registered at 0045F228.
DECOMP(00081714, AnimCmd_27D06D5B_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0FDB0u);
}

// AnimCmd (hash 0x139E755D), registered at 0045F274.
DECOMP(00081800, AnimCmd_139E755D_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0FDD8u);
}

// AnimCmd "OverrideColor" (hash 0x3523AAF7), registered at 0045F2C0.
DECOMP(00081958, AnimCmd_OverrideColor_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    ref<u8>(obj + 16) = 0;
    store<u32>(obj, 0x00B0FE00u);
    const u32 v1 = ref<u8>(0x01164CA0u);
    ref<u8>(obj + 17) = static_cast<u8>(v1);
    const u32 v2 = ref<u8>(0x01164CA1u);
    ref<u8>(obj + 18) = static_cast<u8>(v2);
    const u32 v3 = ref<u8>(0x01164CA2u);
    ref<u8>(obj + 19) = static_cast<u8>(v3);
    const u32 v4 = ref<u8>(0x01164CA3u);
    ref<u8>(obj + 20) = static_cast<u8>(v4);
}

// AnimCmd (hash 0x3173666E), registered at 0045F30C.
DECOMP(00081E90, AnimCmd_3173666E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B0FE28u);
}

// AnimCmd (hash 0x72E44964), registered at 0045F3A8.
DECOMP(00082D7C, AnimCmd_72E44964_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FE78u);
    ref<u8>(obj + 16) = 0;
}

// AnimCmd (hash 0x694C0BD6), registered at 0045F3F4.
DECOMP(00082F58, AnimCmd_694C0BD6_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FEA0u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
}

// AnimCmd (hash 0xC18490E5), registered at 0045F444.
DECOMP(00083154, AnimCmd_C18490E5_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B0FEC8u);
    ref<u8>(obj + 16) = 0;
}

// AnimCmd (hash 0xF1F9B0AA), registered at 0045F494.
DECOMP(0008331C, AnimCmd_F1F9B0AA_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164AF4u);  // lfs
    store<u32>(obj + 144, v1);
    const u32 v2 = load<u32>(0x01164AF8u);  // lfs
    store<u32>(obj, 0x00B0FEF0u);
    store<u32>(obj + 148, v2);
    const u32 v3 = load<u32>(0x01164AFCu);  // lfs
    store<u32>(obj + 152, v3);
    const u32 v4 = load<u32>(0x01164B00u);  // lfs
    ref<u8>(obj + 12) = 0;
    store<u32>(obj + 156, v4);
}

// AnimCmd (hash 0xEB05E07E), registered at 0045F4E4.
DECOMP(000835A8, AnimCmd_EB05E07E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    ref<u8>(obj + 13) = 0;
    store<u32>(obj, 0x00B0FF18u);
    ref<u8>(obj + 14) = 0;
    ref<u8>(obj + 15) = 0;
    ref<u8>(obj + 16) = 0;
    ref<u8>(obj + 17) = 0;
    ref<u8>(obj + 18) = 0;
    ref<u8>(obj + 19) = 255;
    store<u32>(obj + 20, 0x3F800000u);
    ref<u8>(obj + 24) = 0;
}

// AnimCmd "PlaySound" (hash 0x37472E3B), registered at 0045F530.
DECOMP(000837B8, AnimCmd_PlaySound_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj, 0x00B0FF40u);
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 48, v1);
    ref<u8>(obj + 20) = 0;
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0);
    store<u32>(obj + 52, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 64, 0x3F800000u);
    store<u32>(obj + 56, v3);
    store<u32>(obj + 68, 0);
    store<u32>(obj + 72, 0);
    const u32 v4 = load<u32>(obj + 92);
    ref<u8>(obj + 76) = 0;
    ref<u8>(obj + 77) = 0;
    ref<u8>(obj + 78) = 0;
    ref<u8>(obj + 79) = 1;
    store<u32>(obj + 80, 0);
    store<u32>(obj + 84, 0);
    store<u32>(obj + 88, 0);
    store<u32>(obj + 92, (v4 & 0x7FFFFFFFu));
}

// AnimCmd (hash 0xF7E87759), registered at 0045F580.
DECOMP(00084410, AnimCmd_F7E87759_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0FF68u);
    call(ctx, kfn::PtrVector_Construct, {obj + 12});
    store<u16>(obj + 18, 76);
    call(ctx, kfn::PtrVector_Init, {obj + 12, 16, 0, 1});
    store<u32>(obj + 32, 0);
}

// AnimCmd (hash 0xB38B0435), registered at 0045F5D0.
DECOMP(00084624, AnimCmd_B38B0435_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0FF90u);
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0xFC81u);
}

// AnimCmd "QueueCutscene" (hash 0x8E523F7B), registered at 0045F620.
DECOMP(000848CC, AnimCmd_QueueCutscene_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B0FFB8u);
    const u32 v1 = load<u32>(0x00F65C7Cu);
    call(ctx, kfn::PtrVector_Construct, {obj + 12});
    store<u16>(obj + 18, 76);
    call(ctx, kfn::PtrVector_Init, {obj + 12, v1, 0, 1});
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0);
}

// AnimCmd (hash 0x4EF98057), registered at 0045F66C.
DECOMP(00084CC0, AnimCmd_4EF98057_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B0FFE0u);
    store<u32>(obj + 12, 0x0027469Eu);
}

// AnimCmd (hash 0x7C3AAA8B), registered at 0045F6B8.
DECOMP(00084E34, AnimCmd_7C3AAA8B_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xFFFFFFFFu);
    store<u32>(obj, 0x00B10008u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    ref<u8>(obj + 32) = 0;
}

// AnimCmd (hash 0xF22A1F1A), registered at 0045F708.
DECOMP(000852A8, AnimCmd_F22A1F1A_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B10030u);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0xF8D7BA3A), registered at 0045F758.
DECOMP(0008582C, AnimCmd_F8D7BA3A_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B10080u);
}

// AnimCmd (hash 0x299E87A1), registered at 0045F7A4.
DECOMP(000858F8, AnimCmd_299E87A1_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B100A8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x8D0B7E84), registered at 0045F7F4.
DECOMP(00085608, AnimCmd_8D0B7E84_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10058u);
    ref<u8>(obj + 16) = 1;
    ref<u8>(obj + 17) = 1;
}

// AnimCmd (hash 0x7BA76B48), registered at 0045F840.
DECOMP(00085DB0, AnimCmd_7BA76B48_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B100D0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0);
}

// AnimCmd (hash 0xA80527F2), registered at 0045F890.
DECOMP(0006E5CC, AnimCmd_A80527F2_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 20, 0);
    store<u32>(obj, 0x00B0F1F8u);
    store<u32>(obj + 12, 0x0027469Eu);
    store<u32>(obj + 16, 0x0027469Eu);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    ref<u8>(obj + 36) = 0;
    store<u32>(obj + 40, 0);
}

// AnimCmd (hash 0x2FE3EEC7), registered at 0045F8DC.
DECOMP(000864C8, AnimCmd_2FE3EEC7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B100F8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    ref<u8>(obj + 40) = 0;
}

// AnimCmd (hash 0xCB0BC14E), registered at 0045F92C.
DECOMP(000868A0, AnimCmd_CB0BC14E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B10120u);
}

// AnimCmd (hash 0x881CF70D), registered at 0045F97C.
DECOMP(00086DD8, AnimCmd_881CF70D_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10170u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x7B8C11AF), registered at 0045F9C8.
DECOMP(00087034, AnimCmd_7B8C11AF_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B10198u);
}

// AnimCmd (hash 0x5E895638), registered at 0045FA14.
DECOMP(000871B0, AnimCmd_5E895638_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xBF800000u);
    store<u32>(obj, 0x00B101C0u);
}

// AnimCmd (hash 0xB5BFB042), registered at 0045FA64.
DECOMP(000872E8, AnimCmd_B5BFB042_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xBF800000u);
    store<u32>(obj, 0x00B101E8u);
}

// AnimCmd (hash 0x060CB347), registered at 0045FAB0.
DECOMP(00087414, AnimCmd_060CB347_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10210u);
}

// AnimCmd (hash 0xA58714CA), registered at 0045FB00.
DECOMP(0008762C, AnimCmd_A58714CA_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10238u);
}

// AnimCmd (hash 0xD73FDD6F), registered at 0045FB50.
DECOMP(00086A34, AnimCmd_D73FDD6F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B10148u);
    store<u32>(obj + 20, 0x3F800000u);
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0x4023B405), registered at 0045FB9C.
DECOMP(000877F0, AnimCmd_4023B405_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10260u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
    ref<u8>(obj + 25) = 0;
    ref<u8>(obj + 26) = 0;
    ref<u8>(obj + 27) = 0;
}

// AnimCmd (hash 0xE39BD656), registered at 0045FBEC.
DECOMP(00087C14, AnimCmd_E39BD656_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10288u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
    ref<u8>(obj + 22) = 0;
}

// AnimCmd (hash 0xC4C20B66), registered at 0045FC3C.
DECOMP(00087FFC, AnimCmd_C4C20B66_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B102B0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x7C7D0917), registered at 0045FC88.
DECOMP(000882C8, AnimCmd_7C7D0917_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B102D8u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0xFFFFFFFFu);
    store<u32>(obj + 28, 0);
    ref<u8>(obj + 32) = 0;
}

// AnimCmd (hash 0xDF50133E), registered at 0045FCD8.
DECOMP(0008867C, AnimCmd_DF50133E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B10300u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 32, 2);
    store<u32>(obj + 36, 0x3D4CCCCDu);
    store<u32>(obj + 40, 0x3DCCCCCDu);
    store<u32>(obj + 44, 0x3DCCCCCDu);
    store<u32>(obj + 48, 0x41F00000u);
}

// AnimCmd (hash 0x6F88D96F), registered at 0045FD24.
DECOMP(00088D44, AnimCmd_6F88D96F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xFFFFFFFFu);
    store<u32>(obj, 0x00B10328u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x824684A7), registered at 0045FDC4.
DECOMP(000890DC, AnimCmd_824684A7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10378u);
}

// AnimCmd (hash 0x270D8A8D), registered at 0045FE10.
DECOMP(00089204, AnimCmd_270D8A8D_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B103A0u);
    const u32 v1 = ref<u8>(0x01164CA4u);
    ref<u8>(obj + 12) = static_cast<u8>(v1);
    const u32 v2 = ref<u8>(0x01164CA5u);
    ref<u8>(obj + 13) = static_cast<u8>(v2);
    const u32 v3 = ref<u8>(0x01164CA6u);
    ref<u8>(obj + 14) = static_cast<u8>(v3);
    const u32 v4 = ref<u8>(0x01164CA7u);
    ref<u8>(obj + 15) = static_cast<u8>(v4);
    const u32 v5 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 16, v5);
    const u32 v6 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 32, 0xFFFFFFFFu);
    store<u32>(obj + 20, v6);
    const u32 v7 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 36, 0);
    store<u32>(obj + 24, v7);
    store<u32>(obj + 40, 0);
    store<u32>(obj + 44, 0);
    store<u32>(obj + 48, 0);
    store<u32>(obj + 52, 0);
    ref<u8>(obj + 56) = 0;
    ref<u8>(obj + 57) = 0;
    ref<u8>(obj + 58) = 0;
    ref<u8>(obj + 59) = 0;
    ref<u8>(obj + 60) = 0;
    ref<u8>(obj + 61) = 0;
    ref<u8>(obj + 62) = 0;
}

// AnimCmd (hash 0xDB38C585), registered at 0045FE60.
DECOMP(000899A8, AnimCmd_DB38C585_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xBF800000u);
    store<u32>(obj, 0x00B103C8u);
    ref<u8>(obj + 16) = 0;
    ref<u8>(obj + 17) = 0;
}

// AnimCmd (hash 0xC5EC47B7), registered at 0045FEB0.
DECOMP(00089D98, AnimCmd_C5EC47B7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10418u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0x72A5FAD0), registered at 0045FEFC.
DECOMP(00089F3C, AnimCmd_72A5FAD0_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10440u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xC6D07177), registered at 0045FF4C.
DECOMP(0008A0E0, AnimCmd_C6D07177_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10468u);
    store<u32>(obj + 16, 90);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0xAFAF3B97), registered at 0045FF9C.
DECOMP(00089BCC, AnimCmd_AFAF3B97_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B103F0u);
    store<u32>(obj + 16, 0x3F800000u);
}

// AnimCmd (hash 0xAAECF6DB), registered at 00460038.
DECOMP(0008A5D0, AnimCmd_AAECF6DB_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B104B8u);
    ref<u8>(obj + 16) = 0;
    ref<u8>(obj + 17) = 0;
}

// AnimCmd (hash 0x7A1B655B), registered at 00460084.
DECOMP(0008B508, AnimCmd_7A1B655B_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B105A8u);
    store<u32>(obj + 16, 0);
}
