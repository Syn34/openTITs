// Anim-script type constructors, part 2 (see construct_animscript.cpp).
#include "construct.h"

using namespace sly4;
using namespace sly4::wcons;

// AnimCmd (hash 0xA560F585), registered at 004600D4.
DECOMP(0008B768, AnimCmd_A560F585_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B105D0u);
    store<u32>(obj + 12, 0);
}

// AnimCmd (hash 0xEC347D1E), registered at 00460124.
DECOMP(0008A818, AnimCmd_EC347D1E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(obj + 28);
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B104E0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0xBF800000u);
    store<u32>(obj + 28, ((v1 & 0x7FFFFFFFu) & 0xBFFFFFFFu));
}

// AnimCmd (hash 0x72794E29), registered at 00460170.
DECOMP(0008ABE8, AnimCmd_72794E29_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0x3F800000u);
    store<u32>(obj, 0x00B10508u);
    ref<u8>(obj + 16) = 0;
}

// AnimCmd (hash 0xBE2D3C0A), registered at 004601C0.
DECOMP(0008ADE4, AnimCmd_BE2D3C0A_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj, 0x00B10530u);
}

// AnimCmd (hash 0x90F19CF0), registered at 00460210.
DECOMP(0008AF24, AnimCmd_90F19CF0_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10558u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
}

// AnimCmd (hash 0x5E2BDAD5), registered at 0046025C.
DECOMP(0008B1C4, AnimCmd_5E2BDAD5_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10580u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
    ref<u8>(obj + 25) = 0;
}

// AnimCmd (hash 0x83B5EF3B), registered at 004602AC.
DECOMP(0008B968, AnimCmd_83B5EF3B_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj, 0x00B105F8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x3FCA7434), registered at 004602F8.
DECOMP(0008BC34, AnimCmd_3FCA7434_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10620u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
    ref<u8>(obj + 25) = 0;
    ref<u8>(obj + 26) = 0;
}

// AnimCmd (hash 0x7AEEF97E), registered at 00460344.
DECOMP(0008BFB8, AnimCmd_7AEEF97E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10648u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0xC3E53CB8), registered at 00460394.
DECOMP(0008C2F0, AnimCmd_C3E53CB8_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xBF800000u);
    store<u32>(obj, 0x00B10670u);
}

// AnimCmd "SetMusic" (hash 0x83CB8F03), registered at 004603E4.
DECOMP(0008C428, AnimCmd_SetMusic_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B10698u);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 1;
}

// AnimCmd (hash 0x851BFAA6), registered at 00460434.
DECOMP(0008C6DC, AnimCmd_851BFAA6_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xFFFFFFFFu);
    store<u32>(obj, 0x00B106C0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x185B0922), registered at 00460480.
DECOMP(0008C8F0, AnimCmd_185B0922_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B106E8u);
}

// AnimCmd (hash 0x8B4BC0F7), registered at 004604D0.
DECOMP(0008C94C, AnimCmd_8B4BC0F7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10710u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
    ref<u8>(obj + 25) = 0;
}

// AnimCmd (hash 0x19EA0BF7), registered at 0046051C.
DECOMP(0008CC60, AnimCmd_19EA0BF7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10738u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
}

// AnimCmd (hash 0x7903D5A3), registered at 00460568.
DECOMP(0008CE40, AnimCmd_7903D5A3_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10760u);
    const u32 v1 = ref<u8>(0x01164CA0u);
    ref<u8>(obj + 16) = static_cast<u8>(v1);
    const u32 v2 = ref<u8>(0x01164CA1u);
    ref<u8>(obj + 17) = static_cast<u8>(v2);
    const u32 v3 = ref<u8>(0x01164CA2u);
    ref<u8>(obj + 18) = static_cast<u8>(v3);
    const u32 v4 = ref<u8>(0x01164CA3u);
    ref<u8>(obj + 19) = static_cast<u8>(v4);
    store<u32>(obj + 20, 0x40E80000u);
}

// AnimCmd (hash 0xDF62E746), registered at 004605B8.
DECOMP(0008D16C, AnimCmd_DF62E746_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xFFFFFFFFu);
    store<u32>(obj, 0x00B10788u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
}

// AnimCmd (hash 0x80E15258), registered at 00460608.
DECOMP(0008D3B0, AnimCmd_80E15258_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0xFFFFFFFFu);
    store<u32>(obj, 0x00B107B0u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xA1BA0E32), registered at 00460658.
DECOMP(0008D6C0, AnimCmd_A1BA0E32_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B107D8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0x3292B062), registered at 004606A4.
DECOMP(0008DA24, AnimCmd_3292B062_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10800u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
    ref<u8>(obj + 22) = 0;
    ref<u8>(obj + 23) = 0;
}

// AnimCmd (hash 0x80866784), registered at 00460744.
DECOMP(0008FB3C, AnimCmd_80866784_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10850u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
}

// AnimCmd (hash 0x02C1266A), registered at 00460790.
DECOMP(0008FE28, AnimCmd_02C1266A_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(obj + 20);
    store<u32>(obj + 12, 0xBF800000u);
    store<u32>(obj, 0x00B10878u);
    store<u32>(obj + 16, 0xBF800000u);
    store<u32>(obj + 20, ((v1 & 0x7FFFFFFFu) & 0xBFFFFFFFu));
}

// AnimCmd (hash 0xD1BB9067), registered at 004607E0.
DECOMP(00090124, AnimCmd_D1BB9067_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B108A0u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
    ref<u8>(obj + 22) = 0;
    ref<u8>(obj + 23) = 0;
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0xACE05706), registered at 00460830.
DECOMP(000903F8, AnimCmd_ACE05706_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B108C8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
    ref<u8>(obj + 25) = 0;
}

// AnimCmd (hash 0x908A1BE5), registered at 00460880.
DECOMP(000906E8, AnimCmd_908A1BE5_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B108F0u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
}

// AnimCmd (hash 0x6F37AF4C), registered at 004608CC.
DECOMP(000909E4, AnimCmd_6F37AF4C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B90u);  // lfs
    store<u32>(obj + 16, v1);
    const u32 v2 = load<u32>(0x01164B94u);  // lfs
    store<u32>(obj, 0x00B10918u);
    store<u32>(obj + 12, 0);
    store<u32>(obj + 20, v2);
    const u32 v3 = load<u32>(0x01164B98u);  // lfs
    ref<u8>(obj + 32) = 0;
    store<u32>(obj + 24, v3);
    ref<u8>(obj + 33) = 0;
}

// AnimCmd (hash 0x92C1D3B7), registered at 0046091C.
DECOMP(00090E74, AnimCmd_92C1D3B7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10940u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd "SetSoundRegister" (hash 0x71814B90), registered at 00460968.
DECOMP(000910F4, AnimCmd_SetSoundRegister_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10968u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
    ref<u8>(obj + 25) = 0;
    ref<u8>(obj + 26) = 0;
    ref<u8>(obj + 27) = 0;
    ref<u8>(obj + 28) = 0;
    ref<u8>(obj + 29) = 128;
    ref<u8>(obj + 30) = 0;
    ref<u8>(obj + 31) = 127;
}

// AnimCmd (hash 0xB6CD6FF1), registered at 004609B8.
DECOMP(00091BBC, AnimCmd_B6CD6FF1_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B10990u);
}

// AnimCmd (hash 0x71AF687B), registered at 00460A04.
DECOMP(00091CF0, AnimCmd_71AF687B_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u16>(obj + 18);
    store<u32>(obj, 0x00B109B8u);
    store<u32>(obj + 12, 0);
    store<u16>(obj + 18, static_cast<u16>((v1 & 0xFFFF7FFFu)));
    store<u32>(obj + 20, 0x3F800000u);
    store<u16>(obj + 16, 0xFFFFu);
    store<u32>(obj + 24, 0xBF800000u);
}

// AnimCmd (hash 0x680DCBA4), registered at 00460A50.
DECOMP(00092168, AnimCmd_680DCBA4_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B109E0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 28) = 0;
    ref<u8>(obj + 29) = 0;
}

// AnimCmd (hash 0x83F543E7), registered at 00460AA0.
DECOMP(0009241C, AnimCmd_83F543E7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10A08u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xBE5506DB), registered at 00460AF0.
DECOMP(00092628, AnimCmd_BE5506DB_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10A30u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0x4151629E), registered at 00460B3C.
DECOMP(00092870, AnimCmd_4151629E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 270) = 1;
    ref<u8>(obj + 271) = 1;
    store<u32>(obj, 0x00B10A58u);
    ref<u8>(obj + 12) = 0;
    ref<u8>(obj + 141) = 0;
}

// AnimCmd (hash 0xC4162D5B), registered at 00460B8C.
DECOMP(00092B8C, AnimCmd_C4162D5B_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj, 0x00B10A80u);
}

// AnimCmd "SmokeBomb" (hash 0x27DDC471), registered at 00460BD8.
DECOMP(00092CAC, AnimCmd_SmokeBomb_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10AA8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
}

// AnimCmd (hash 0x2856A2FE), registered at 00460C24.
DECOMP(00093054, AnimCmd_2856A2FE_Construct) {
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
    store<u32>(obj + 56, v6);
    const u32 v7 = load<u32>(0x01164C90u);  // lfs
    store<u32>(obj, 0x00B10AD0u);
    store<u32>(obj + 64, v7);
    const u32 v8 = load<u32>(0x01164C94u);  // lfs
    store<u32>(obj + 12, 0);
    store<u32>(obj + 68, v8);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    const u32 v9 = load<u32>(0x01164C98u);  // lfs
    store<u32>(obj + 72, v9);
    const u32 v10 = load<u32>(0x01164C9Cu);  // lfs
    store<u32>(obj + 76, v10);
    const u32 v11 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 80, v11);
    const u32 v12 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 96, 0);
    store<u32>(obj + 84, v12);
    const u32 v13 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 100, 0);
    store<u32>(obj + 88, v13);
    store<u32>(obj + 104, 0);
    store<u32>(obj + 108, 0);
    store<u32>(obj + 112, 1);
    store<u32>(obj + 116, 1);
    store<u32>(obj + 120, 0);
    ref<u8>(obj + 125) = 0;
}

// AnimCmd (hash 0x261AE896), registered at 00460C70.
DECOMP(00093C80, AnimCmd_261AE896_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 32, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj, 0x00B10AF8u);
    store<u32>(obj + 36, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 12, 0);
    store<u32>(obj + 40, v3);
    const u32 v4 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 48, v4);
    const u32 v5 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 52, v5);
    const u32 v6 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 56, v6);
    call(ctx, kfn::PtrVector_Construct, {obj + 64});
    store<u16>(obj + 70, 48);
    call(ctx, kfn::PtrVector_Init, {obj + 64, 8, 0, 0});
}

// AnimCmd "SpawnRope" (hash 0x8FEA4ECD), registered at 00460CC0.
DECOMP(000951C0, AnimCmd_SpawnRope_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 64, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj, 0x00B10B20u);
    store<u32>(obj + 68, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 72, v3);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    const u32 v4 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 40, 0);
    store<u32>(obj + 44, 0);
    store<u32>(obj + 48, 0);
    store<u32>(obj + 80, v4);
    const u32 v5 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 84, v5);
    const u32 v6 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 96, 0);
    store<u32>(obj + 88, v6);
    ref<u8>(obj + 100) = 0;
}

// AnimCmd (hash 0x34F5E762), registered at 00460D0C.
DECOMP(00095F10, AnimCmd_34F5E762_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 144, 0);
    store<u32>(obj + 148, 0);
    store<u32>(obj, 0x00B10B48u);
    store<u32>(obj + 152, 0xBF800000u);
    store<u32>(obj + 156, 0xBF800000u);
    ref<u8>(obj + 160) = 0;
    ref<u8>(obj + 161) = 1;
    ref<u8>(obj + 12) = 0;
}

// AnimCmd (hash 0xE7024843), registered at 00460D5C.
DECOMP(000961F0, AnimCmd_E7024843_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10B70u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
}

// AnimCmd (hash 0xF5C11B05), registered at 00460DAC.
DECOMP(000963A0, AnimCmd_F5C11B05_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10B98u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0xF05E4B22), registered at 00460DFC.
DECOMP(000966D8, AnimCmd_F05E4B22_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    ref<u8>(obj + 16) = 0;
    store<u32>(obj, 0x00B10BC0u);
    ref<u8>(obj + 17) = 1;
}

// AnimCmd (hash 0x0807A743), registered at 00460E48.
DECOMP(00096858, AnimCmd_0807A743_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B10BE8u);
}

// AnimCmd (hash 0x0859822D), registered at 00460E94.
DECOMP(0009697C, AnimCmd_0859822D_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10C10u);
    ref<u8>(obj + 16) = 0;
}

// AnimCmd (hash 0x98862C34), registered at 00460EE4.
DECOMP(00096BD4, AnimCmd_98862C34_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10C38u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x52FDEC56), registered at 00460F30.
DECOMP(00096E00, AnimCmd_52FDEC56_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10C60u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x44873509), registered at 00460F7C.
DECOMP(00097188, AnimCmd_44873509_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10C88u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x7B192C82), registered at 00460FC8.
DECOMP(000973A8, AnimCmd_7B192C82_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10CB0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0xFA394507), registered at 00461018.
DECOMP(000975C8, AnimCmd_FA394507_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10CD8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x1791056F), registered at 00461064.
DECOMP(000977E4, AnimCmd_1791056F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10D00u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    ref<u8>(obj + 40) = 0;
}

// AnimCmd (hash 0x8A36758E), registered at 004610B4.
DECOMP(00097C58, AnimCmd_8A36758E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10D28u);
    ref<u8>(obj + 16) = 0;
    ref<u8>(obj + 17) = 0;
    ref<u8>(obj + 18) = 0;
}

// AnimCmd (hash 0x986C30AA), registered at 00461154.
DECOMP(00098B4C, AnimCmd_986C30AA_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 16, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj, 0x00B10D78u);
    store<u32>(obj + 20, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 32, 0);
    store<u32>(obj + 24, v3);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0);
    store<u32>(obj + 44, 0);
    store<u32>(obj + 48, 0);
    store<u32>(obj + 52, 0);
    store<u32>(obj + 56, 0);
    ref<u8>(obj + 60) = 1;
    ref<u8>(obj + 61) = 0;
    ref<u8>(obj + 62) = 0;
}

// AnimCmd (hash 0xA32A9127), registered at 004611A4.
DECOMP(00099074, AnimCmd_A32A9127_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B70u);  // lfs
    store<u32>(obj + 16, v1);
    const u32 v2 = load<u32>(0x01164B74u);  // lfs
    store<u32>(obj, 0x00B10DA0u);
    store<u32>(obj + 20, v2);
    const u32 v3 = load<u32>(0x01164B78u);  // lfs
    store<u32>(obj + 32, 0x3F800000u);
    store<u32>(obj + 24, v3);
    store<u32>(obj + 36, 0xFFFFFFFFu);
    store<u32>(obj + 40, 0);
    ref<u8>(obj + 44) = 0;
    ref<u8>(obj + 45) = 0;
}

// AnimCmd (hash 0x844EA043), registered at 004611F4.
DECOMP(00099510, AnimCmd_844EA043_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B10DC8u);
}

// AnimCmd (hash 0x4DBCDE4C), registered at 00461240.
DECOMP(00099694, AnimCmd_4DBCDE4C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10DF0u);
}

// AnimCmd (hash 0xE41C7FAC), registered at 00461290.
DECOMP(00099844, AnimCmd_E41C7FAC_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10E18u);
    ref<u8>(obj + 16) = 0;
    ref<u8>(obj + 17) = 0;
}

// AnimCmd (hash 0x8B76ABE7), registered at 004612E0.
DECOMP(0009A340, AnimCmd_8B76ABE7_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B10F10u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0xA1DF0C27), registered at 00461330.
DECOMP(0009A594, AnimCmd_A1DF0C27_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 32, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 36, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 40, v3);
    const u32 v4 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 64, v4);
    const u32 v5 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 68, v5);
    const u32 v6 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj, 0x00B10F38u);
    store<u32>(obj + 72, v6);
    const u32 v7 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 12, 0);
    store<u32>(obj + 96, v7);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    const u32 v8 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 24, 0);
    store<u32>(obj + 100, v8);
    const u32 v9 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 48, 0);
    store<u32>(obj + 104, v9);
    const u32 v10 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 80, 0);
    store<u32>(obj + 112, v10);
    store<u32>(obj + 84, 0);
    const u32 v11 = load<u32>(0x01164B44u);  // lfs
    ref<u8>(obj + 128) = 0;
    store<u32>(obj + 116, v11);
    const u32 v12 = load<u32>(0x01164B48u);  // lfs
    ref<u8>(obj + 129) = 0;
    store<u32>(obj + 120, v12);
    ref<u8>(obj + 130) = 0;
    ref<u8>(obj + 131) = 0;
}

// AnimCmd (hash 0xFC622478), registered at 00461380.
DECOMP(0009AF14, AnimCmd_FC622478_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10F88u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0x91B5C0DD), registered at 004613D0.
DECOMP(0009AC9C, AnimCmd_91B5C0DD_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10F60u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
}

// AnimCmd (hash 0x99F875FE), registered at 00461420.
DECOMP(0009B170, AnimCmd_99F875FE_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10FB0u);
    ref<u8>(obj + 16) = 0;
}

// AnimCmd (hash 0x053879BE), registered at 0046146C.
DECOMP(0009B31C, AnimCmd_053879BE_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0x3F800000u);
    store<u32>(obj + 24, 0x3F800000u);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0x3F800000u);
    store<u32>(obj + 44, 0);
    store<u32>(obj, 0x00B10FD8u);
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
    ref<u8>(obj + 112) = 0;
    store<u32>(obj + 84, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 96, 0xBF800000u);
    store<u32>(obj + 88, v3);
}

// AnimCmd "Climb" (hash 0x03D3CE95), registered at 004614B8.
DECOMP(0009B78C, AnimCmd_Climb_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B11000u);
    ref<u8>(obj + 20) = 1;
}

// AnimCmd (hash 0x467B546E), registered at 00461504.
DECOMP(0009B9A4, AnimCmd_467B546E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11028u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
}

// AnimCmd (hash 0xFF8D82F5), registered at 00461554.
DECOMP(0009BCA4, AnimCmd_FF8D82F5_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11050u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xCB5C709B), registered at 004615A4.
DECOMP(0009BE48, AnimCmd_CB5C709B_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B11078u);
    ref<u8>(obj + 20) = 1;
}

// AnimCmd (hash 0x76502F44), registered at 004615F0.
DECOMP(0009C254, AnimCmd_76502F44_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B110C8u);
    ref<u8>(obj + 16) = 1;
    ref<u8>(obj + 17) = 0;
}

// AnimCmd (hash 0xB164C699), registered at 00461640.
DECOMP(0009C420, AnimCmd_B164C699_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B110F0u);
    ref<u8>(obj + 13) = 0;
}

// AnimCmd (hash 0xF2F4B486), registered at 00461690.
DECOMP(0009C5B4, AnimCmd_F2F4B486_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11118u);
    ref<u8>(obj + 16) = 1;
    ref<u8>(obj + 17) = 1;
    ref<u8>(obj + 18) = 0;
    ref<u8>(obj + 19) = 0;
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
}

// AnimCmd (hash 0xD6942D99), registered at 004616E0.
DECOMP(0009C818, AnimCmd_D6942D99_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11140u);
    ref<u8>(obj + 16) = 1;
    ref<u8>(obj + 17) = 1;
    ref<u8>(obj + 18) = 0;
    ref<u8>(obj + 19) = 0;
    ref<u8>(obj + 20) = 0;
}

// AnimCmd (hash 0x7148897C), registered at 0046172C.
DECOMP(0009CA50, AnimCmd_7148897C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B11168u);
}

// AnimCmd (hash 0x9F872A66), registered at 0046177C.
DECOMP(0009CBB8, AnimCmd_9F872A66_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11190u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
}

// AnimCmd (hash 0x631AC87F), registered at 004617C8.
DECOMP(0009CDA0, AnimCmd_631AC87F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B111B8u);
    ref<u8>(obj + 16) = 1;
}

// AnimCmd (hash 0x7B7D81C6), registered at 00461814.
DECOMP(0009CF34, AnimCmd_7B7D81C6_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B111E0u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    ref<u8>(obj + 33) = 0;
}

// AnimCmd (hash 0x4DFC58E3), registered at 00461860.
DECOMP(0009D2A4, AnimCmd_4DFC58E3_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 24, 1);
    store<u32>(obj, 0x00B11208u);
    store<u32>(obj + 16, 0x3F800000u);
    store<u32>(obj + 20, 0x3F800000u);
    ref<u8>(obj + 28) = 0;
    ref<u8>(obj + 29) = 1;
}

// AnimCmd (hash 0xDDB84C75), registered at 004618B0.
DECOMP(0009D514, AnimCmd_DDB84C75_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0x3F800000u);
    store<u32>(obj + 20, 0xBF800000u);
    store<u32>(obj, 0x00B11230u);
    ref<u8>(obj + 29) = 1;
    store<u32>(obj + 16, 0x3E800000u);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 28) = 0;
}

// AnimCmd (hash 0xE9BDD91F), registered at 00461900.
DECOMP(0009D7C8, AnimCmd_E9BDD91F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B11258u);
    store<u32>(obj + 20, 0x3F800000u);
    store<u32>(obj + 24, 0x41200000u);
    store<u32>(obj + 28, 0x41A00000u);
    store<u32>(obj + 32, 0x41200000u);
    store<u32>(obj + 36, 0x42200000u);
    store<u32>(obj + 40, 0x3F800000u);
    store<u32>(obj + 44, 1);
    ref<u8>(obj + 48) = 0;
    ref<u8>(obj + 49) = 1;
    ref<u8>(obj + 50) = 0;
}

// AnimCmd (hash 0x2D84F263), registered at 0046194C.
DECOMP(0009DBA0, AnimCmd_2D84F263_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11280u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 28) = 0;
}

// AnimCmd (hash 0x7C2668C0), registered at 00461998.
DECOMP(0009DDF4, AnimCmd_7C2668C0_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B112A8u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 28, 0x3F800000u);
    store<u32>(obj + 24, 0x489EBD81u);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0x3F800000u);
    store<u32>(obj + 44, 0x3F800000u);
    store<u32>(obj + 48, 0);
    store<u32>(obj + 52, 0x85D98325u);
    store<u32>(obj + 56, 0);
    store<u32>(obj + 60, 0x3F800000u);
    store<u32>(obj + 64, 0);
    store<u32>(obj + 68, 0x3F800000u);
    store<u32>(obj + 72, 0x40800000u);
    store<u32>(obj + 76, 0x40D00000u);
    store<u32>(obj + 80, 0x40200000u);
    store<u32>(obj + 84, 0);
    store<u32>(obj + 88, 0);
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
    store<u32>(obj + 132, 0xBF800000u);
    ref<u8>(obj + 136) = 0;
    ref<u8>(obj + 137) = 0;
    ref<u8>(obj + 138) = 0;
    ref<u8>(obj + 139) = 0;
    ref<u8>(obj + 140) = 0;
}

// AnimCmd (hash 0xD8B4387E), registered at 004619E8.
DECOMP(0009E4CC, AnimCmd_D8B4387E_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B112D0u);
    store<u32>(obj + 20, 0x3F800000u);
}

// AnimCmd (hash 0x8318A329), registered at 00461A38.
DECOMP(0009E78C, AnimCmd_8318A329_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B112F8u);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
    ref<u8>(obj + 25) = 1;
}

// AnimCmd (hash 0xDE6B8CC1), registered at 00461A88.
DECOMP(0009EA0C, AnimCmd_DE6B8CC1_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11320u);
    ref<u8>(obj + 16) = 0;
}

// AnimCmd "LookAt" (hash 0x862E9B12), registered at 00461AD8.
DECOMP(0009EBA0, AnimCmd_LookAt_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11348u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
}

// AnimCmd (hash 0xE5E03D0C), registered at 00461B28.
DECOMP(0009EE54, AnimCmd_E5E03D0C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B11370u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0x3F800000u);
    ref<u8>(obj + 32) = 1;
    ref<u8>(obj + 33) = 0;
    ref<u8>(obj + 34) = 1;
}

// AnimCmd (hash 0x87E68E0C), registered at 00461B78.
DECOMP(0009F3FC, AnimCmd_87E68E0C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B113C0u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0x3F800000u);
    ref<u8>(obj + 32) = 0;
    ref<u8>(obj + 33) = 1;
}

// AnimCmd (hash 0xD8F50A36), registered at 00461BC8.
DECOMP(0009F154, AnimCmd_D8F50A36_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11398u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0);
}

// AnimCmd "Pause" (hash 0x04862DD6), registered at 00461C14.
DECOMP(0009F750, AnimCmd_Pause_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B113E8u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xEC00CC0F), registered at 00461C64.
DECOMP(0009FDB0, AnimCmd_EC00CC0F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B11488u);
}

// AnimCmd (hash 0xC8982F97), registered at 00461CB4.
DECOMP(0009F994, AnimCmd_C8982F97_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B11410u);
}

// AnimCmd (hash 0x2D43CE4C), registered at 00461D00.
DECOMP(0009FAF0, AnimCmd_2D43CE4C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B11438u);
}

// AnimCmd (hash 0x1A907D03), registered at 00461D4C.
DECOMP(0009FC54, AnimCmd_1A907D03_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B11460u);
}

// AnimCmd (hash 0xA6401484), registered at 00461D9C.
DECOMP(0009FF10, AnimCmd_A6401484_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B114B0u);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 0;
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0x3F800000u);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0);
    store<u32>(obj + 44, 0);
    ref<u8>(obj + 48) = 1;
    ref<u8>(obj + 49) = 1;
    ref<u8>(obj + 50) = 1;
    ref<u8>(obj + 51) = 0;
}

// AnimCmd (hash 0x780316BB), registered at 00461DE8.
DECOMP(000A045C, AnimCmd_780316BB_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 80, 0);
    store<u32>(obj + 84, 0);
    store<u32>(obj + 88, 0);
    store<u32>(obj, 0x00B114D8u);
    store<u32>(obj + 104, 0);
    store<u32>(obj + 92, 0);
    store<u32>(obj + 108, 0);
    store<u32>(obj + 96, 0);
    ref<u8>(obj + 100) = 1;
    const u32 v1 = load<u32>(0x00F64D08u);
    ref<u8>(obj + 192) = 0;
    store<u32>(obj + 124, v1);
    store<u32>(obj + 196, 0);
    store<u32>(obj + 200, 0);
    store<u32>(obj + 204, 0);
    ref<u8>(obj + 12) = 0;
}

// AnimCmd (hash 0xE37F3CE5), registered at 00461E38.
DECOMP(000A0C5C, AnimCmd_E37F3CE5_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B11508u);
    call(ctx, kfn::PtrVector_Construct, {obj + 12});
    store<u16>(obj + 18, 76);
    call(ctx, kfn::PtrVector_Init, {obj + 12, 16, 0, 1});
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0);
    ref<u8>(obj + 44) = 1;
    ref<u8>(obj + 45) = 0;
}

// AnimCmd (hash 0x226C3AFD), registered at 00461F20.
DECOMP(000A19B0, AnimCmd_226C3AFD_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164AF4u);  // lfs
    store<u32>(obj + 140, v1);
    const u32 v2 = load<u32>(0x01164AF8u);  // lfs
    store<u32>(obj, 0x00B11580u);
    store<u32>(obj + 144, v2);
    const u32 v3 = load<u32>(0x01164AFCu);  // lfs
    ref<u8>(obj + 156) = 1;
    store<u32>(obj + 148, v3);
    const u32 v4 = load<u32>(0x01164B00u);  // lfs
    ref<u8>(obj + 12) = 0;
    store<u32>(obj + 152, v4);
}

// AnimCmd (hash 0x1FE37F1C), registered at 00461FBC.
DECOMP(000A220C, AnimCmd_1FE37F1C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    ref<u8>(obj + 13) = 0;
    store<u32>(obj, 0x00B115D0u);
    ref<u8>(obj + 14) = 0;
    ref<u8>(obj + 15) = 0;
    ref<u8>(obj + 16) = 0;
    ref<u8>(obj + 17) = 0;
    ref<u8>(obj + 18) = 0;
    ref<u8>(obj + 19) = 255;
    store<u32>(obj + 20, 0x3F800000u);
    ref<u8>(obj + 24) = 1;
    ref<u8>(obj + 25) = 0;
}

// AnimCmd "SeqPlaySound" (hash 0x22C0C4DC), registered at 00462008.
DECOMP(000A2480, AnimCmd_SeqPlaySound_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B115F8u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0x3F800000u);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    ref<u8>(obj + 40) = 1;
    ref<u8>(obj + 41) = 0;
    store<u32>(obj + 44, 0);
    store<u32>(obj + 124, 0);
    store<u32>(obj + 128, 0);
    store<u32>(obj + 132, 0);
    ref<u8>(obj + 56) = 0;
}

// AnimCmd (hash 0x91949DE3), registered at 00462058.
DECOMP(000A2ADC, AnimCmd_91949DE3_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11620u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
}

// AnimCmd (hash 0x556D9F3B), registered at 004620A4.
DECOMP(000A2D14, AnimCmd_556D9F3B_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11648u);
}

// AnimCmd (hash 0xB34002B5), registered at 004620F4.
DECOMP(000A2E88, AnimCmd_B34002B5_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11670u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
}

// AnimCmd (hash 0x58A0F34C), registered at 00462140.
DECOMP(000A3090, AnimCmd_58A0F34C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B11698u);
    call(ctx, kfn::PtrVector_Construct, {obj + 12});
    store<u16>(obj + 18, 16);
    store<u32>(obj + 52, 0xBF800000u);
    store<u32>(obj + 32, 0);
    store<u32>(obj + 36, 0);
    store<u32>(obj + 40, 0x3F800000u);
    store<u32>(obj + 44, 0);
    store<u32>(obj + 48, 0);
}

// AnimCmd (hash 0x6DBC265F), registered at 0046218C.
DECOMP(000A34C8, AnimCmd_6DBC265F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B116C0u);
    ref<u8>(obj + 16) = 1;
    ref<u8>(obj + 17) = 1;
}

// AnimCmd (hash 0xB39D2C46), registered at 004621DC.
DECOMP(000A3688, AnimCmd_B39D2C46_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B116E8u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    store<u32>(obj + 32, 0);
    ref<u8>(obj + 36) = 0;
    ref<u8>(obj + 37) = 0;
}

// AnimCmd (hash 0x5226D5B6), registered at 00462228.
DECOMP(000A39EC, AnimCmd_5226D5B6_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj, 0x00B11710u);
}

// AnimCmd (hash 0xAD4079D1), registered at 00462278.
DECOMP(000A3B50, AnimCmd_AD4079D1_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj, 0x00B11738u);
    ref<u8>(obj + 13) = 0;
    ref<u8>(obj + 14) = 0;
    ref<u8>(obj + 15) = 0;
    ref<u8>(obj + 16) = 0;
    ref<u8>(obj + 17) = 0;
}

// AnimCmd (hash 0xFD97E64D), registered at 004622C8.
DECOMP(000A3D9C, AnimCmd_FD97E64D_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B11760u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 28) = 0;
    ref<u8>(obj + 29) = 0;
    ref<u8>(obj + 30) = 1;
    ref<u8>(obj + 31) = 0;
    ref<u8>(obj + 32) = 1;
}

// AnimCmd (hash 0xCF110608), registered at 00462318.
DECOMP(000A42C0, AnimCmd_CF110608_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B117B0u);
}

// AnimCmd (hash 0xE61291FE), registered at 00462368.
DECOMP(000A443C, AnimCmd_E61291FE_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B117D8u);
}

// AnimCmd "SetEntity" (hash 0xE79D5EA5), registered at 004623B8.
DECOMP(000A4878, AnimCmd_SetEntity_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11850u);
    store<u32>(obj + 16, 0);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
    ref<u8>(obj + 32) = 0;
    ref<u8>(obj + 33) = 0;
    ref<u8>(obj + 34) = 0;
    ref<u8>(obj + 35) = 0;
    ref<u8>(obj + 36) = 0;
    ref<u8>(obj + 37) = 0;
    ref<u8>(obj + 38) = 0;
    ref<u8>(obj + 39) = 0;
    ref<u8>(obj + 40) = 0;
}

// AnimCmd (hash 0x35963BAE), registered at 00462404.
DECOMP(000A536C, AnimCmd_35963BAE_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B118F0u);
    ref<u8>(obj + 16) = 0;
}

// AnimCmd (hash 0xA6FC4A0D), registered at 00462454.
DECOMP(000A50E4, AnimCmd_A6FC4A0D_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B118C8u);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 1;
    ref<u8>(obj + 22) = 1;
    ref<u8>(obj + 23) = 1;
    ref<u8>(obj + 24) = 0;
}

// AnimCmd (hash 0x7BF7E38C), registered at 004624A0.
DECOMP(000A4DB4, AnimCmd_7BF7E38C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11878u);
    store<u32>(obj + 16, 1);
}

// AnimCmd (hash 0x2CB7712F), registered at 004624EC.
DECOMP(000A4F4C, AnimCmd_2CB7712F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj, 0x00B118A0u);
    ref<u8>(obj + 13) = 1;
}

// AnimCmd "SeqSetMusic" (hash 0x8B642042), registered at 0046253C.
DECOMP(000A5504, AnimCmd_SeqSetMusic_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B11918u);
    store<u32>(obj + 20, 0);
    ref<u8>(obj + 24) = 1;
    ref<u8>(obj + 25) = 0;
}

// AnimCmd (hash 0xF6F2D4D6), registered at 0046258C.
DECOMP(000A4710, AnimCmd_F6F2D4D6_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B11828u);
}

// AnimCmd (hash 0x9EAE7138), registered at 004625DC.
DECOMP(000A4078, AnimCmd_9EAE7138_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11788u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
}

// AnimCmd (hash 0x2C6A515D), registered at 00462628.
DECOMP(000A57EC, AnimCmd_2C6A515D_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11940u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
}

// AnimCmd "SetPosition" (hash 0x1F9E676B), registered at 00462674.
DECOMP(000A5A04, AnimCmd_SetPosition_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 16, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 20, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj, 0x00B11968u);
    store<u32>(obj + 24, v3);
    const u32 v4 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 32, v4);
    const u32 v5 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj + 48, 0);
    store<u32>(obj + 36, v5);
    const u32 v6 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 52, 0);
    store<u32>(obj + 40, v6);
    store<u32>(obj + 56, 0);
    store<u32>(obj + 60, 0);
    store<u32>(obj + 64, 0);
    ref<u8>(obj + 68) = 0;
    ref<u8>(obj + 69) = 1;
    ref<u8>(obj + 70) = 0;
}

// AnimCmd (hash 0xAC54A838), registered at 004626C4.
DECOMP(000A45A4, AnimCmd_AC54A838_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 1;
    store<u32>(obj, 0x00B11800u);
}

// AnimCmd (hash 0xDAD61703), registered at 00462714.
DECOMP(000A5F08, AnimCmd_DAD61703_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B11990u);
    ref<u8>(obj + 20) = 1;
}

// AnimCmd (hash 0x390D5F47), registered at 00462760.
DECOMP(000A60E4, AnimCmd_390D5F47_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 1);
    store<u32>(obj, 0x00B119B8u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0x7EA392FC), registered at 004627AC.
DECOMP(000A629C, AnimCmd_7EA392FC_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B119E0u);
}

// AnimCmd (hash 0x1BC5155F), registered at 004627F8.
DECOMP(000A63C0, AnimCmd_1BC5155F_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    const u32 v1 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 112, v1);
    const u32 v2 = load<u32>(0x01164B44u);  // lfs
    store<u32>(obj, 0x00B11A08u);
    store<u32>(obj + 116, v2);
    const u32 v3 = load<u32>(0x01164B48u);  // lfs
    store<u32>(obj + 80, 0);
    store<u32>(obj + 120, v3);
    store<u32>(obj + 84, 0);
    store<u32>(obj + 88, 0);
    const u32 v4 = load<u32>(0x01164B40u);  // lfs
    store<u32>(obj + 92, 0);
    store<u32>(obj + 96, 0);
    store<u32>(obj + 100, 0);
    store<u32>(obj + 128, v4);
    const u32 v5 = load<u32>(0x01164B44u);  // lfs
    ref<u8>(obj + 144) = 0;
    store<u32>(obj + 132, v5);
    const u32 v6 = load<u32>(0x01164B48u);  // lfs
    ref<u8>(obj + 145) = 0;
    store<u32>(obj + 136, v6);
    ref<u8>(obj + 146) = 0;
    ref<u8>(obj + 147) = 0;
    ref<u8>(obj + 148) = 0;
    ref<u8>(obj + 149) = 0;
    ref<u8>(obj + 12) = 0;
}

// AnimCmd (hash 0x0DE7D203), registered at 00462844.
DECOMP(000A6A3C, AnimCmd_0DE7D203_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11A30u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0xB89DA664), registered at 00462894.
DECOMP(000A6CA4, AnimCmd_B89DA664_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11A58u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
}

// AnimCmd (hash 0xF3813DE4), registered at 004628E4.
DECOMP(000A6E60, AnimCmd_F3813DE4_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj, 0x00B11A80u);
}

// AnimCmd (hash 0xF831840C), registered at 00462934.
DECOMP(000A6FC0, AnimCmd_F831840C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B11AA8u);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
    ref<u8>(obj + 22) = 1;
}

// AnimCmd (hash 0xFD7AAF5C), registered at 00462984.
DECOMP(000A71EC, AnimCmd_FD7AAF5C_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj, 0x00B11AD0u);
    call(ctx, kfn::PtrVector_Construct, {obj + 12});
    store<u16>(obj + 18, 4);
}

// AnimCmd (hash 0x2D2232FC), registered at 004629D0.
DECOMP(000A7488, AnimCmd_2D2232FC_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B11AF8u);
    store<u32>(obj + 16, 0);
}

// AnimCmd (hash 0x7DE1DF64), registered at 00462A1C.
DECOMP(0009C014, AnimCmd_7DE1DF64_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    ref<u8>(obj + 12) = 0;
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B110A0u);
    store<u32>(obj + 20, 0x3F800000u);
    store<u32>(obj + 24, 0);
    store<u32>(obj + 28, 0);
}

// AnimCmd (hash 0x83F48558), registered at 00462A6C.
DECOMP(00099AFC, AnimCmd_83F48558_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj + 16, 0);
    store<u32>(obj, 0x00B10E40u);
    store<u32>(obj + 20, 0);
    store<u32>(obj + 24, 0);
    ref<u8>(obj + 28) = 0;
    ref<u8>(obj + 29) = 1;
}

// AnimCmd (hash 0x2C1E0CC1), registered at 00462AB8.
DECOMP(00099CFC, AnimCmd_2C1E0CC1_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    call(ctx, kfn::AnimCmd_ConstructBase, {obj});
    store<u32>(obj + 12, 0);
    store<u32>(obj, 0x00B10E68u);
    store<u32>(obj + 16, 0);
    ref<u8>(obj + 20) = 0;
    ref<u8>(obj + 21) = 0;
    ref<u8>(obj + 22) = 0;
    ref<u8>(obj + 23) = 0;
}
