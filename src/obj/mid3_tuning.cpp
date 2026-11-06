// Game code, medium sweep part 3: the tuning-variable statics. Each game
// module keeps its tweakables ("Entity.Vehicle.DestroyMe", "FX.Trail.
// FrameRate", ...) in TuningVar globals; the static initializers here
// construct them (TuningVar_ConstructBase_004FBFA8 registers the name, then
// the class's vtable and the default value), and the atexit destructors
// put each vtable back and run TuningVar_DestructBase_004FC1A0, in reverse.
// The values are given as stored (f32 as bit patterns); the names are the
// strings at the second argument.
#include "mid3.h"

using namespace sly4;
using namespace sly4::mid3;

// void f(): static initializer of the tuning vars Entity.WestBoss.* (the stuck-camera tuning, two debug switches).
DECOMP(000C24E8, TuningVars_Ctor_000C24E8) {
    store<u32>(0x01013A70, 0x0);
    store<u32>(0x01013A74, 0x0);
    store<u32>(0x01013A78, 0x0);
    TvFloat(ctx, 0x010137F0, 0x00AC6488, 0x3DCCCCCD);  // "Entity.WestBoss.StuckCamera.Interp" = 0.1
    TvFloat(ctx, 0x01013840, 0x00AC64B0, 0x40951EB8);  // "Entity.WestBoss.StuckCamera.GimbalRadius" = 4.66
    TvFloat(ctx, 0x01013890, 0x00AC64E0, 0x41A80000);  // "Entity.WestBoss.StuckCamera.Angle" = 21
    TvFloat(ctx, 0x010138E0, 0x00AC6508, 0x3F8147AE);  // "Entity.WestBoss.StuckCamera.TargetYOffse" = 1.01
    TvFloat(ctx, 0x01013930, 0x00AC6538, 0x3F4CCCCD);  // "Entity.WestBoss.StuckCamera.TargetZOffse" = 0.8
    TvFloat(ctx, 0x01013980, 0x00AC6568, 0x3F800000);  // "Entity.WestBoss.StuckCamera.EaseInTime" = 1
    TvFloat(ctx, 0x010139D0, 0x00AC6590, 0x3F800000);  // "Entity.WestBoss.StuckCamera.EaseOutTime" = 1
    TvBool(ctx, 0x01013A20, 0x00AC65B8, 0);  // "Entity.WestBoss.AllowAnyMeleeAttackForKn"
    TvBool(ctx, 0x010137A0, 0x00AC65E8, 0);  // "Entity.WestBoss.DebugDrawThrownGadgetSpl"
}

// void f(): static initializer of the tuning vars Entity.Marker.* / Entity.Score.* (marker offsets, score popup timing, instance names).
DECOMP(0020BD08, TuningVars_Ctor_0020BD08) {
    store<u32>(0x01055AC8, 0x0);
    store<u32>(0x01055ACC, 0x0);
    store<u32>(0x01055AD0, 0x0);
    TvVec3(ctx, 0x010555E0, 0x00AD34F0, 0x00000000, 0x3F000000, 0x00000000);  // "Entity.Marker.Offset" = {0, 0.5, 0}
    TvVec3(ctx, 0x01055640, 0x00AD3508, 0x00000000, 0x40000000, 0x00000000);  // "Entity.Marker.TriggerJobOffset" = {0, 2, 0}
    TvFloat(ctx, 0x0105576C, 0x00AD3528, 0x3F400000);  // "Entity.Score.VisTimeDefault" = 0.75
    TvFloat(ctx, 0x010557B8, 0x00AD3548, 0x3F000000);  // "Entity.Score.FadeTimeDefault" = 0.5
    TvString(ctx, 0x010556A0, 0x00AD3568, 0x00AD3588);  // "Entity.Score.InstNameValue" = "Value"
    TvString(ctx, 0x01055938, 0x00AD3590, 0x00AD35B0);  // "Entity.Score.InstNameRoot" = ".InGame.Elements.Score"
    TvString(ctx, 0x01055A00, 0x00AD35C8, 0x00AD35E8);  // "Entity.Score.InstNameParent" = ".InGame.HUD.Scores.Popup"
    TvVec3(ctx, 0x01055810, 0x00AD3608, 0xC2960000, 0xC2480000, 0x00000000);  // "Entity.Score.Offset" = {-75, -50, 0}
    TvString(ctx, 0x01055870, 0x00AD3620, 0x00AD3640);  // "Entity.Score.InstNameHilite" = "HL"
}

// void f(): static initializer of the tuning vars FX.* (shake, trail) and two PtrVectors.
DECOMP(002F5E00, TuningVars_Ctor_002F5E00) {
    call(ctx, m3fn::PtrVector_Construct, {0x01068818});
    store<u16>(0x0106881E, 0x4);
    TvBool(ctx, 0x01068848, 0x00AD9720, 0);  // "FX.DisableAll"
    TvFloat(ctx, 0x01068898, 0x00AD9730, 0x3F000000);  // "FX.ShakeSpeedInterp" = 0.5
    TvFloat(ctx, 0x010688E8, 0x00AD9748, 0x428C0000);  // "FX.ObjShake.DefaultCullDist" = 70
    TvBool(ctx, 0x01068938, 0x00AD9768, 1);  // "FX.ObjShake.DisableIfNotRendered"
    call(ctx, m3fn::PtrVector_Construct, {0x0106882C});
    store<u16>(0x01068832, 0x60);
    store<u8>(0x01068840, 0x0);
    TvFloat(ctx, 0x01068988, 0x00AD9790, 0x43340000);  // "FX.Trail.FrameRate" = 180
    TvInt(ctx, 0x010689D8, 0x00AD97A8, 0);  // "FX.Trail.NoTexture"
    TvBool(ctx, 0x01068A28, 0x00AD97C0, 0);  // "FX.Trail.OutlineEnable"
    TvColor(ctx, 0x01068A78, 0x00AD97D8, 0x000000FFu);  // "FX.Trail.OutlineColor"
    TvFloat(ctx, 0x01068AC8, 0x00AD97F0, 0x41200000);  // "FX.Trail.PointAccelMax" = 10
    TvFloat(ctx, 0x01068B18, 0x00AD9808, 0x42340000);  // "FX.Trail.HintAngleMin" = 45
    ctx.gpr[3] = 0x42340000;  // r3 left as the last value's bits, as the original
}

// void f(): static initializer of the tuning vars Goal.QTE.* / Goal.ParisChase.* (the chase camera shake and roll).
DECOMP(0043B0C0, TuningVars_Ctor_0043B0C0) {
    store<u32>(0x010AADA0, 0x0);
    store<u32>(0x010AADA4, 0x0);
    store<u32>(0x010AADA8, 0x0);
    TvBool(ctx, 0x010AA760, 0x00AE289C, 0);  // "Goal.QTE.DebugEnable"
    TvBool(ctx, 0x010AA7B0, 0x00AE28B4, 0);  // "Goal.ParisChase.DebugEnable"
    TvFloat(ctx, 0x010AA800, 0x00AE28D4, 0x40000000);  // "Goal.ParisChase.MaxVanDeviation" = 2
    TvFloat(ctx, 0x010AA850, 0x00AE28F4, 0x41000000);  // "Goal.ParisChase.DeviationSpeed" = 8
    TvFloat(ctx, 0x010AA8A0, 0x00AE2914, 0x40600000);  // "Goal.ParisChase.VanColsize" = 3.5
    TvFloat(ctx, 0x010AA8F0, 0x00AE2934, 0x3F8CCCCD);  // "Goal.ParisChase.CameraShake.CamShakeDura" = 1.1
    TvFloat(ctx, 0x010AA940, 0x00AE2964, 0x40800000);  // "Goal.ParisChase.CameraShake.CamShakeMagn" = 4
    TvFloat(ctx, 0x010AA990, 0x00AE2994, 0x00000000);  // "Goal.ParisChase.CameraShake.CamShakeRamp" = 0
    TvHash(ctx, 0x010AA9E0, 0x00AE29D4, 0x85D98325);  // "Goal.ParisChase.CameraShake.CamShakeFOMo"
    TvFloat(ctx, 0x010AAA30, 0x00AE2A04, 0x00000000);  // "Goal.ParisChase.CameraShake.CamShakeFOSt" = 0
    TvFloat(ctx, 0x010AAA80, 0x00AE2A34, 0x3F800000);  // "Goal.ParisChase.CameraShake.CamShakePitc" = 1
    TvFloat(ctx, 0x010AAAD0, 0x00AE2A64, 0x00000000);  // "Goal.ParisChase.CameraShake.CamShakeYaw" = 0
    TvFloat(ctx, 0x010AAB20, 0x00AE2A8C, 0x3F800000);  // "Goal.ParisChase.CameraShake.CamShakeRoll" = 1
    TvFloat(ctx, 0x010AAB70, 0x00AE2ABC, 0x40800000);  // "Goal.ParisChase.CameraShake.CamShakePitc" = 4
    TvFloat(ctx, 0x010AABC0, 0x00AE2AF4, 0x40D00000);  // "Goal.ParisChase.CameraShake.CamShakeYawF" = 6.5
    TvFloat(ctx, 0x010AAC10, 0x00AE2B2C, 0x40200000);  // "Goal.ParisChase.CameraShake.CamShakeRoll" = 2.5
    TvFloat(ctx, 0x010AAC60, 0x00AE2B64, 0x42700000);  // "Goal.ParisChase.CameraRoll.MaxRollRate" = 60
    TvFloat(ctx, 0x010AACB0, 0x00AE2B8C, 0x40400000);  // "Goal.ParisChase.CameraRoll.DeviationScal" = 3
    TvFloat(ctx, 0x010AAD00, 0x00AE2BBC, 0x42700000);  // "Goal.ParisChase.CameraRoll.DeviationRate" = 60
    TvFloat(ctx, 0x010AAD50, 0x00AE2BEC, 0x40A00000);  // "Goal.ParisChase.CameraRoll.InputScale" = 5
    ctx.gpr[3] = 0x40A00000;  // r3 left as the last value's bits, as the original
}

// void f(): static initializer of the tuning vars Entity.Vehicle.* (simple-wheeled collision and help text) and a pool array.
DECOMP(0024F034, TuningVars_Ctor_0024F034) {
    TvBool(ctx, 0x01056E7C, 0x00AD3E1C, 0);  // "Entity.Vehicle.DestroyMe"
    store<u32>(0x01056EC8, 0);  // the 12 bytes after it
    store<u32>(0x01056ECC, 0);
    store<u32>(0x01056ED0, 0);
    // The inlined constructor of the pool array 0x01056C00 (element size
    // 4, capacity 8): the default allocator, the data from its vfn[0]
    // (allocator, bytes, 0, 16) for a non-zero element size, Array_Init,
    // then +20 = 0x00F34E48 and +16 = 0.
    constexpr u32 arr = 0x01056C00;
    store<u16>(arr + 0, 4);
    store<u16>(arr + 2, 0);
    store<u16>(arr + 4, 0);
    store<u16>(arr + 6, 16);
    store<u32>(arr + 8, 0);
    const u32 al = static_cast<u32>(call(ctx, m3fn::Mem_DefaultAllocator));
    store<u32>(arr + 12, al);
    if (const u32 elem = load<u16>(arr); elem != 0) {
        const u32 bytes = elem * ((static_cast<u32>(load<u16>(arr + 6)) >> 1) & 0x7FFFu);
        const u32 opd = load<u32>(load<u32>(al));
        // r7 = the entry, r8 = the OPD, as the original's sequence leaves them
        store<u32>(arr + 8, static_cast<u32>(call_ptr(ctx, opd, {al, bytes, 0, 16, load<u32>(opd), opd})));
    }
    call(ctx, m3fn::Array_Init, {arr});
    store<u32>(arr + 20, 0x00F34E48);
    store<u32>(arr + 16, 0);
    TvFloat(ctx, 0x01056C1C, 0x00AD3E3C, 0x41C80000);  // "Entity.Vehicle.SimpleWheeled.CollisionDa" = 25
    TvFloat(ctx, 0x01056C68, 0x00AD3E6C, 0x40A00000);  // "Entity.Vehicle.SimpleWheeled.CollisionIm" = 5
    TvFloat(ctx, 0x01056CB4, 0x00AD3E9C, 0x3F333333);  // "Entity.Vehicle.SimpleWheeled.CollisionRu" = 0.7
    TvFloat(ctx, 0x01056D00, 0x00AD3ED4, 0x3F000000);  // "Entity.Vehicle.SimpleWheeled.CollisionRu" = 0.5
    TvInt(ctx, 0x01056D4C, 0x00AD3F0C, 1);  // "Entity.Vehicle.SimpleWheeled.HelpText.Dr"
    TvInt(ctx, 0x01056D98, 0x00AD3F3C, 4);  // "Entity.Vehicle.SimpleWheeled.HelpText.Sh"
    TvFloat(ctx, 0x01056DE4, 0x00AD3F6C, 0x40A00000);  // "Entity.Vehicle.SimpleWheeled.HelpText.Dr" = 5
    TvFloat(ctx, 0x01056E30, 0x00AD3FA4, 0x40600000);  // "Entity.Vehicle.SimpleWheeled.HelpText.Sh" = 3.5
    ctx.gpr[3] = 0x40600000;  // r3 left as the last value's bits, as the original
}

// void f(): static (atexit) destructor of 23 tuning vars.
DECOMP(00226060, TuningVars_Dtor_00226060) {
    TvDtor(ctx, 0x01056974, kVtBool);
    TvDtor(ctx, 0x01056928, kVtBool);
    TvDtor(ctx, 0x01056B50, kVtBool);
    TvDtor(ctx, 0x010568DC, kVtBool);
    TvDtor(ctx, 0x01056890, kVtInt);
    TvDtor(ctx, 0x01056844, kVtBool);
    TvDtor(ctx, 0x010565E4, kVtBool);
    TvDtor(ctx, 0x01056B00, kVtFloat);
    TvDtor(ctx, 0x010567F8, kVtBool);
    TvDtor(ctx, 0x010567AC, kVtBool);
    TvDtor(ctx, 0x01056760, kVtBool);
    TvDtor(ctx, 0x01056714, kVtBool);
    TvDtor(ctx, 0x01056AB0, kVtFloat);
    TvDtor(ctx, 0x010566C8, kVtBool);
    TvDtor(ctx, 0x0105667C, kVtBool);
    TvDtor(ctx, 0x01056630, kVtBool);
    TvDtor(ctx, 0x01056A60, kVtBool);
    TvDtor(ctx, 0x01056A10, kVtBool);
    TvDtor(ctx, 0x0105654C, kVtBool);
    TvDtor(ctx, 0x01055B00, kVtBool);
    TvDtor(ctx, 0x01056598, kVtBool);
    call(ctx, m3fn::Mem_FreeFrom, {load<u32>(0x01055B80), 0});
    TvDtor(ctx, 0x010569C0, kVtBool);
    TvDtor(ctx, 0x01056500, kVtBool);
}

// void f(): static (atexit) destructor of 52 tuning vars and the array 0x01054D50[5] (from the back).
DECOMP(00209240, TuningVars_Dtor_00209240) {
    TvDtor(ctx, 0x01055580, kVtVec2);
    TvDtor(ctx, 0x01055520, kVtVec3);
    TvDtor(ctx, 0x010554D0, kVtFloat);
    TvDtor(ctx, 0x01055480, kVtFloat);
    TvDtor(ctx, 0x01055430, kVtVec2);
    TvDtor(ctx, 0x010553D0, kVtVec3);
    TvDtor(ctx, 0x01055370, kVtVec3);
    TvDtor(ctx, 0x01055310, kVtVec3);
    TvDtor(ctx, 0x010552B0, kVtVec3);
    TvDtor(ctx, 0x01055250, kVtVec3);
    TvDtor(ctx, 0x01055200, kVtVec2);
    TvDtor(ctx, 0x010551B0, kVtFloat);
    TvDtor(ctx, 0x01055160, kVtFloat);
    TvDtor(ctx, 0x01055110, kVtFloat);
    TvDtor(ctx, 0x010550C0, kVtFloat);
    TvDtor(ctx, 0x01055070, kVtFloat);
    TvDtor(ctx, 0x01055020, kVtFloat);
    TvDtor(ctx, 0x01054FC0, kVtVec3);
    TvDtor(ctx, 0x01054F70, kVtFloat);
    TvDtor(ctx, 0x01054F20, kVtVec2);
    TvDtor(ctx, 0x01054ED0, kVtFloat);
    for (s32 k = 4; k >= 0; --k) TvDtor(ctx, 0x01054D50 + 76 * static_cast<u32>(k), kVtFloat);  // the array 0x01054D50[5], from the back
    TvDtor(ctx, 0x01054D00, kVtFloat);
    TvDtor(ctx, 0x01054CB0, kVtFloat);
    TvDtor(ctx, 0x01054C60, kVtFloat);
    TvDtor(ctx, 0x01054C10, kVtFloat);
    TvDtor(ctx, 0x01054BC0, kVtFloat);
    TvDtor(ctx, 0x01054B70, kVtFloat);
    TvDtor(ctx, 0x01054B20, kVtFloat);
    TvDtor(ctx, 0x01054AD0, kVtFloat);
    TvDtor(ctx, 0x01054A80, kVtFloat);
    TvDtor(ctx, 0x01054A30, kVtFloat);
    TvDtor(ctx, 0x010549E0, kVtFloat);
    TvDtor(ctx, 0x01054980, kVtVec3);
    TvDtor(ctx, 0x0105492C, kVtBool);
    TvDtor(ctx, 0x010548E0, kVtFloat);
    TvDtor(ctx, 0x01054894, kVtFloat);
    TvDtor(ctx, 0x01054848, kVtBool);
    TvDtor(ctx, 0x010547FC, kVtFloat);
    TvDtor(ctx, 0x01054678, kVtFloat);
    TvDtor(ctx, 0x0105462C, kVtFloat);
    TvDtor(ctx, 0x010545E0, kVtFloat);
    TvDtor(ctx, 0x01054594, kVtFloat);
    TvDtor(ctx, 0x01054548, kVtFloat);
    TvDtor(ctx, 0x010544FC, kVtFloat);
    TvDtor(ctx, 0x010546C4, kVtFloat);
    TvDtor(ctx, 0x010544B0, kVtFloat);
    TvDtor(ctx, 0x01054460, kVtVec2);
    TvDtor(ctx, 0x01054410, kVtVec2);
    TvDtor(ctx, 0x010543C0, kVtVec2);
    TvDtor(ctx, 0x010547B0, kVtBool);
    TvDtor(ctx, 0x01054760, kVtFloat);
    TvDtor(ctx, 0x01054710, kVtFloat);
}

// void f(): static (atexit) destructor of 65 tuning vars.
DECOMP(003A23D4, TuningVars_Dtor_003A23D4) {
    TvDtor(ctx, 0x0106FCD4, kVtColor);
    TvDtor(ctx, 0x0106FC0C, kVtString);
    TvDtor(ctx, 0x0106FB44, kVtString);
    TvDtor(ctx, 0x0106FA7C, kVtString);
    TvDtor(ctx, 0x0106F9B4, kVtString);
    TvDtor(ctx, 0x0106F8EC, kVtString);
    TvDtor(ctx, 0x0106F824, kVtString);
    TvDtor(ctx, 0x0106F75C, kVtString);
    TvDtor(ctx, 0x0106F694, kVtString);
    TvDtor(ctx, 0x0106F5CC, kVtString);
    TvDtor(ctx, 0x0106F504, kVtString);
    TvDtor(ctx, 0x0106F43C, kVtString);
    TvDtor(ctx, 0x0106F374, kVtString);
    TvDtor(ctx, 0x0106F2AC, kVtString);
    TvDtor(ctx, 0x0106F1E4, kVtString);
    TvDtor(ctx, 0x0106F11C, kVtString);
    TvDtor(ctx, 0x0106F054, kVtString);
    TvDtor(ctx, 0x0106EF8C, kVtString);
    TvDtor(ctx, 0x0106EEC4, kVtString);
    TvDtor(ctx, 0x0106EDFC, kVtString);
    TvDtor(ctx, 0x0106ED34, kVtString);
    TvDtor(ctx, 0x0106EC6C, kVtString);
    TvDtor(ctx, 0x0106EBA4, kVtString);
    TvDtor(ctx, 0x0106EADC, kVtString);
    TvDtor(ctx, 0x0106EA14, kVtString);
    TvDtor(ctx, 0x0106E94C, kVtString);
    TvDtor(ctx, 0x0106E884, kVtString);
    TvDtor(ctx, 0x0106E7BC, kVtString);
    TvDtor(ctx, 0x0106E6F4, kVtString);
    TvDtor(ctx, 0x0106E62C, kVtString);
    TvDtor(ctx, 0x0106E564, kVtString);
    TvDtor(ctx, 0x0106E49C, kVtString);
    TvDtor(ctx, 0x0106E3D4, kVtString);
    TvDtor(ctx, 0x0106E30C, kVtString);
    TvDtor(ctx, 0x0106E244, kVtString);
    TvDtor(ctx, 0x0106E17C, kVtString);
    TvDtor(ctx, 0x0106E0B4, kVtString);
    TvDtor(ctx, 0x0106DFEC, kVtString);
    TvDtor(ctx, 0x0106DF24, kVtString);
    TvDtor(ctx, 0x0106DE5C, kVtString);
    TvDtor(ctx, 0x0106DD94, kVtString);
    TvDtor(ctx, 0x0106DCCC, kVtString);
    TvDtor(ctx, 0x0106DC04, kVtString);
    TvDtor(ctx, 0x0106DB3C, kVtString);
    TvDtor(ctx, 0x0106DA74, kVtString);
    TvDtor(ctx, 0x0106D9AC, kVtString);
    TvDtor(ctx, 0x0106D8E4, kVtString);
    TvDtor(ctx, 0x0106D81C, kVtString);
    TvDtor(ctx, 0x0106D754, kVtString);
    TvDtor(ctx, 0x0106D704, kVtVec2);
    TvDtor(ctx, 0x0106D6B4, kVtVec2);
    TvDtor(ctx, 0x0106D668, kVtFloat);
    TvDtor(ctx, 0x0106D61C, kVtFloat);
    TvDtor(ctx, 0x0106D5D0, kVtFloat);
    TvDtor(ctx, 0x0106D584, kVtFloat);
    TvDtor(ctx, 0x0106D4BC, kVtString);
    TvDtor(ctx, 0x0106D470, kVtFloat);
    TvDtor(ctx, 0x0106D3A8, kVtString);
    TvDtor(ctx, 0x0106D2E0, kVtString);
    TvDtor(ctx, 0x0106D218, kVtString);
    TvDtor(ctx, 0x0106D150, kVtString);
    TvDtor(ctx, 0x0106D088, kVtString);
    TvDtor(ctx, 0x0106D03C, kVtBool);
    TvDtor(ctx, 0x0106CFF0, kVtBool);
    TvDtor(ctx, 0x0106FD20, kVtBool);
}

// void f(): static (atexit) destructor of 73 tuning vars.
DECOMP(001F7288, TuningVars_Dtor_001F7288) {
    TvDtor(ctx, 0x01054360, kVtFloat);
    TvDtor(ctx, 0x01054310, kVtBool);
    TvDtor(ctx, 0x010542C0, kVtBool);
    TvDtor(ctx, 0x01054270, kVtFloat);
    TvDtor(ctx, 0x01054220, kVtFloat);
    TvDtor(ctx, 0x010541D0, kVtVec2);
    TvDtor(ctx, 0x01054180, kVtVec2);
    TvDtor(ctx, 0x01054130, kVtVec2);
    TvDtor(ctx, 0x010540E0, kVtVec2);
    TvDtor(ctx, 0x01054090, kVtFloat);
    TvDtor(ctx, 0x01053054, kVtFloat);
    TvDtor(ctx, 0x01054040, kVtInt);
    TvDtor(ctx, 0x01053FF0, kVtFloat);
    TvDtor(ctx, 0x01053FA0, kVtFloat);
    TvDtor(ctx, 0x01053F50, kVtFloat);
    TvDtor(ctx, 0x01053F00, kVtFloat);
    TvDtor(ctx, 0x01053EA0, kVtVec3);
    TvDtor(ctx, 0x01053E40, kVtVec3);
    TvDtor(ctx, 0x01053DF0, kVtInt);
    TvDtor(ctx, 0x01053DA0, kVtInt);
    TvDtor(ctx, 0x01053D50, kVtFloat);
    TvDtor(ctx, 0x01053D00, kVtFloat);
    TvDtor(ctx, 0x01053CA0, kVtVec3);
    TvDtor(ctx, 0x01053C50, kVtFloat);
    TvDtor(ctx, 0x01053C00, kVtFloat);
    TvDtor(ctx, 0x01053BB0, kVtFloat);
    TvDtor(ctx, 0x01053B60, kVtFloat);
    TvDtor(ctx, 0x01053B10, kVtFloat);
    TvDtor(ctx, 0x01053AC0, kVtFloat);
    TvDtor(ctx, 0x01053A70, kVtFloat);
    TvDtor(ctx, 0x01053A20, kVtFloat);
    TvDtor(ctx, 0x010539D0, kVtFloat);
    TvDtor(ctx, 0x01053980, kVtFloat);
    TvDtor(ctx, 0x01053930, kVtFloat);
    TvDtor(ctx, 0x010538E0, kVtFloat);
    TvDtor(ctx, 0x01053890, kVtFloat);
    TvDtor(ctx, 0x01053840, kVtVec2);
    TvDtor(ctx, 0x010537F0, kVtVec2);
    TvDtor(ctx, 0x010537A0, kVtVec2);
    TvDtor(ctx, 0x01053750, kVtFloat);
    TvDtor(ctx, 0x01053700, kVtVec2);
    TvDtor(ctx, 0x010536B0, kVtVec2);
    TvDtor(ctx, 0x01053660, kVtFloat);
    TvDtor(ctx, 0x01053610, kVtFloat);
    TvDtor(ctx, 0x010535C0, kVtFloat);
    TvDtor(ctx, 0x01053570, kVtFloat);
    TvDtor(ctx, 0x01053520, kVtFloat);
    TvDtor(ctx, 0x010534D0, kVtFloat);
    TvDtor(ctx, 0x01053470, kVtVec3);
    TvDtor(ctx, 0x01053410, kVtVec3);
    TvDtor(ctx, 0x010533C0, kVtFloat);
    TvDtor(ctx, 0x01053370, kVtFloat);
    TvDtor(ctx, 0x01053320, kVtFloat);
    TvDtor(ctx, 0x010532D0, kVtFloat);
    TvDtor(ctx, 0x01053280, kVtFloat);
    TvDtor(ctx, 0x01053230, kVtFloat);
    TvDtor(ctx, 0x010531E0, kVtFloat);
    TvDtor(ctx, 0x01053190, kVtFloat);
    TvDtor(ctx, 0x01053140, kVtBool);
    TvDtor(ctx, 0x010530F0, kVtBool);
    TvDtor(ctx, 0x010530A0, kVtFloat);
    TvDtor(ctx, 0x01052F8C, kVtString);
    TvDtor(ctx, 0x01052EC4, kVtString);
    TvDtor(ctx, 0x01052E78, kVtFloat);
    TvDtor(ctx, 0x01052E2C, kVtBool);
    TvDtor(ctx, 0x01052DE0, kVtBool);
    TvDtor(ctx, 0x01052D94, kVtBool);
    TvDtor(ctx, 0x01052D48, kVtFloat);
    TvDtor(ctx, 0x01052CFC, kVtFloat);
    TvDtor(ctx, 0x01052CB0, kVtFloat);
    TvDtor(ctx, 0x01052C50, kVtVec3);
    TvDtor(ctx, 0x01052BF0, kVtVec3);
    TvDtor(ctx, 0x01052B90, kVtVec3);
}

// void f(): static (atexit) destructor of 15 tuning vars, 1 PtrVector, 5 pool arrays.
DECOMP(002E76F4, TuningVars_Dtor_002E76F4) {
    TvDtor(ctx, 0x01068648, kVtFloat);
    TvDtor(ctx, 0x010685F8, kVtFloat);
    TvDtor(ctx, 0x010685A8, kVtFloat);
    TvDtor(ctx, 0x01068558, kVtFloat);
    TvDtor(ctx, 0x01068508, kVtFloat);
    TvDtor(ctx, 0x010684BC, kVtInt);
    TvDtor(ctx, 0x01068470, kVtInt);
    TvDtor(ctx, 0x01068424, kVtInt);
    TvDtor(ctx, 0x010683D8, kVtBool);
    TvDtor(ctx, 0x0106838C, kVtHash);
    TvDtor(ctx, 0x01068340, kVtBool);
    TvDtor(ctx, 0x010682F4, kVtBool);
    TvDtor(ctx, 0x010682A8, kVtInt);
    TvDtor(ctx, 0x0106825C, kVtInt);
    TvDtor(ctx, 0x01068210, kVtFloat);
    VecDtor(ctx, 0x010681FC);
    PoolArrayDtor(ctx, 0x010681E4);
    PoolArrayDtor(ctx, 0x010681CC);
    PoolArrayDtor(ctx, 0x010681B4);
    PoolArrayDtor(ctx, 0x0106819C);
    PoolArrayDtor(ctx, 0x01068184);
}

// void f(): static (atexit) destructor of 34 tuning vars, 1 PtrVector, 1 pool array.
DECOMP(004AFD54, TuningVars_Dtor_004AFD54) {
    PoolArrayDtor(ctx, 0x010C1D50);
    TvDtor(ctx, 0x010C25F0, kVtString);
    TvDtor(ctx, 0x010C25A0, kVtInt);
    TvDtor(ctx, 0x010C2550, kVtInt);
    TvDtor(ctx, 0x010C2500, kVtBool);
    TvDtor(ctx, 0x010C24A0, kVtVec3);
    TvDtor(ctx, 0x010C1D04, kVtFloat);
    TvDtor(ctx, 0x010C1CB8, kVtInt);
    TvDtor(ctx, 0x010C1C6C, kVtFloat);
    TvDtor(ctx, 0x010C1C20, kVtFloat);
    TvDtor(ctx, 0x010C1B3C, kVtBool);
    TvDtor(ctx, 0x010C1AF0, kVtBool);
    TvDtor(ctx, 0x010C1AA4, kVtBool);
    VecDtor(ctx, 0x010C2480);
    TvDtor(ctx, 0x010C2430, kVtFloat);
    TvDtor(ctx, 0x010C23E0, kVtFloat);
    TvDtor(ctx, 0x010C2390, kVtFloat);
    TvDtor(ctx, 0x010C2340, kVtFloat);
    TvDtor(ctx, 0x010C22E0, kVtVec3);
    TvDtor(ctx, 0x010C2280, kVtVec3);
    TvDtor(ctx, 0x010C2230, kVtInt);
    TvDtor(ctx, 0x010C21E0, kVtFloat);
    TvDtor(ctx, 0x010C2190, kVtFloat);
    TvDtor(ctx, 0x010C2140, kVtFloat);
    TvDtor(ctx, 0x010C20F0, kVtFloat);
    TvDtor(ctx, 0x010C20A0, kVtFloat);
    TvDtor(ctx, 0x010C2050, kVtFloat);
    TvDtor(ctx, 0x010C2000, kVtFloat);
    TvDtor(ctx, 0x010C1FB0, kVtFloat);
    TvDtor(ctx, 0x010C1F60, kVtFloat);
    TvDtor(ctx, 0x010C1F10, kVtFloat);
    TvDtor(ctx, 0x010C1EB0, kVtVec3);
    TvDtor(ctx, 0x010C1E58, kVtFloat);
    TvDtor(ctx, 0x010C1E08, kVtFloat);
    TvDtor(ctx, 0x010C1DB8, kVtFloat);
    TvDtor(ctx, 0x010C1D68, kVtFloat);
}

// void f(): static (atexit) destructor of 7 tuning vars, 5 PtrVectors, 5 pool arrays.
DECOMP(000F81E4, TuningVars_Dtor_000F81E4) {
    PoolArrayDtor(ctx, 0x0103217C);
    PoolArrayDtor(ctx, 0x01032164);
    PoolArrayDtor(ctx, 0x0103214C);
    TvDtor(ctx, 0x01021C88, kVtFloat);
    TvDtor(ctx, 0x01032100, kVtInt);
    TvDtor(ctx, 0x01021FCC, kVtBool);
    TvDtor(ctx, 0x01021F80, kVtBool);
    TvDtor(ctx, 0x01021CD8, kVtBool);
    TvDtor(ctx, 0x01021C3C, kVtFloat);
    TvDtor(ctx, 0x01021BF0, kVtFloat);
    VecDtor(ctx, 0x01013B84);
    VecDtor(ctx, 0x01013B70);
    VecDtor(ctx, 0x01013B5C);
    VecDtor(ctx, 0x01013B48);
    VecDtor(ctx, 0x01013B34);
    PoolArrayDtor(ctx, 0x01013B1C);
    PoolArrayDtor(ctx, 0x01013B04);
}

// void f(): static (atexit) destructor of 2 tuning vars, 1 PtrVector, 4 pool arrays.
DECOMP(002EBDB0, TuningVars_Dtor_002EBDB0) {
    TvDtor(ctx, 0x01068778, kVtBool);
    TvDtor(ctx, 0x01068728, kVtBool);
    VecDtor(ctx, 0x0106870C);
    PoolArrayDtor(ctx, 0x010686F4);
    PoolArrayDtor(ctx, 0x010686DC);
    PoolArrayDtor(ctx, 0x010686C4);
    PoolArrayDtor(ctx, 0x010686A8);
}
