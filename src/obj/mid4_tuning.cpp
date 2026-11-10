// Game code, medium sweep part 4: the last tuning-variable statics (as
// mid3_tuning.cpp: TuningVar_ConstructBase_004FBFA8 registers the name, then
// the vtable and the default value; the atexit destructors put each vtable
// back and run TuningVar_DestructBase_004FC1A0, in reverse). The values are
// given as stored (f32 as bit patterns); the names are the strings at the
// second argument.
#include <atomic>

#include "mid4.h"

using namespace sly4;
using namespace sly4::mid4;

// void f(): static initializer of the tuning vars Goal.BinocucomPhoto.* / Goal.BinocucomRecord.* (the default photo / record target angles and distances), Goal.PenguinMontage.SubgameLossAwardNum, Goal.ShootingGallery.DebugEnable, plus an f32 global copied from rodata.
DECOMP(003417D8, TuningVars_Ctor_003417D8) {
    store<u32>(0x0106B6C8, 0x0);
    store<u32>(0x0106B6CC, 0x0);
    store<u32>(0x0106B6D0, 0x0);
    TvFloat(ctx, 0x0106B2E4, 0x00ADBB6C, 0x40000000);  // "Goal.BinocucomPhoto.DefaultAngle" = 2
    TvFloat(ctx, 0x0106B330, 0x00ADBB94, 0x40A00000);  // "Goal.BinocucomPhoto.DefaultDistMin" = 5
    TvFloat(ctx, 0x0106B37C, 0x00ADBBBC, 0x41700000);  // "Goal.BinocucomPhoto.DefaultDistMax" = 15
    TvFloat(ctx, 0x0106B3C8, 0x00ADBBE4, 0x00000000);  // "Goal.BinocucomPhoto.DefaultDistMinClose" = 0
    TvFloat(ctx, 0x0106B414, 0x00ADBC0C, 0x00000000);  // "Goal.BinocucomPhoto.DefaultDistMaxClose" = 0
    TvFloat(ctx, 0x0106B460, 0x00ADBC34, 0x42700000);  // "Goal.BinocucomPhoto.DefaultDistCull" = 60
    TvFloat(ctx, 0x0106B4AC, 0x00ADBC5C, 0x40000000);  // "Goal.BinocucomPhoto.DefaultDisplayTime" = 2
    TvFloat(ctx, 0x0106B4F8, 0x00ADBC84, 0x3F800000);  // "Goal.BinocucomPhoto.DefaultOcclusionRadi" = 1
    TvFloat(ctx, 0x0106B548, 0x00ADBCB4, 0x43B40000);  // "Goal.BinocucomRecord.DefaultAngle" = 360
    TvFloat(ctx, 0x0106B594, 0x00ADBCDC, 0x40A00000);  // "Goal.BinocucomRecord.DefaultDistMin" = 5
    TvFloat(ctx, 0x0106B5E0, 0x00ADBD04, 0x41700000);  // "Goal.BinocucomRecord.DefaultDistMax" = 15
    CopyF32(ctx, 0x0106B2E0, 0x00ADB818);
    TvInt(ctx, 0x0106B630, 0x00ADBD2C, 27);  // "Goal.PenguinMontage.SubgameLossAwardNum"
    TvBool(ctx, 0x0106B67C, 0x00ADBD54, 0);  // "Goal.ShootingGallery.DebugEnable"
    ctx.gpr[3] = kVtBool;  // r3 left as the last vtable, as the original
}

// void f(): static initializer of the tuning vars Debug.Display.Trigger.* (the per-trigger-type debug switches), Entity.Trigger.*, Audio.ZoneXFadeTime and the trigger module's zeroed 0x2000-byte buffer.
DECOMP(00226288, TuningVars_Ctor_00226288) {
    store<u32>(0x01056BA0, 0x0);
    store<u32>(0x01056BA4, 0x0);
    store<u32>(0x01056BA8, 0x0);
    TvBool(ctx, 0x01056500, 0x00AD3904, 1);  // "Debug.Display.Trigger. All"
    TvBool(ctx, 0x010569C0, 0x00AD3924, 0);  // "Debug.Display.Trigger.Log"
    store<u32>(0x01055B80, 0x0);
    store<u32>(0x01055B84, 0x0);
    store<u32>(0x01055C00, 0x0);
    store<u32>(0x01055C80, 0x0);
    store<u32>(0x01055C84, 0x2000);
    store<u32>(0x01055C88, 0x10);
    {
        // a 0x2000-byte buffer {+0 begin, +4 end, +128 / +256 cursors, +260
        // size, +264 alignment}, zeroed
        const u32 p = static_cast<u32>(call(ctx, m4fn::Mem_Alloc, {0x2000, 0, 16}));
        const u32 n = load<u32>(0x01055C84);
        store<u32>(0x01055B80, p);
        store<u32>(0x01055C00, p);
        store<u32>(0x01055C80, p);
        store<u32>(0x01055B84, p + n);
        call(ctx, m4fn::memset, {p, 0, n});
    }
    TvBool(ctx, 0x01056598, 0x00AD3944, 0);  // "Debug.Display.Trigger.Alert"
    TvBool(ctx, 0x01055B00, 0x00AD3964, 0);  // "Audio.Optimization.DisableVolumetricSoun"
    TvBool(ctx, 0x0105654C, 0x00AD3994, 0);  // "Debug.Display.Trigger.Ambient"
    TvBool(ctx, 0x01056A10, 0x00AD39B4, 0);  // "Debug.Display.Trigger.Camera"
    TvBool(ctx, 0x01056A60, 0x00AD39D4, 0);  // "Entity.Trigger.Camera.NoDeadExit"
    TvBool(ctx, 0x01056630, 0x00AD39FC, 0);  // "Debug.Display.Trigger.CameraBehavior"
    TvBool(ctx, 0x0105667C, 0x00AD3A24, 0);  // "Debug.Display.Trigger.Crawl"
    TvBool(ctx, 0x010566C8, 0x00AD3A44, 0);  // "Debug.Display.Trigger.Death"
    TvFloat(ctx, 0x01056AB0, 0x00AD3A64, 0x41A00000);  // "Entity.Trigger.Death.SurfaceRayRange" = 20
    TvBool(ctx, 0x01056714, 0x00AD3A8C, 0);  // "Debug.Display.Trigger.Filtered"
    TvBool(ctx, 0x01056760, 0x00AD3AAC, 0);  // "Debug.Display.Trigger.FX"
    CopyF32(ctx, 0x01055B4C, 0x00AD3644);
    TvBool(ctx, 0x010567AC, 0x00AD3ACC, 0);  // "Debug.Display.Trigger.Job"
    TvBool(ctx, 0x010567F8, 0x00AD3AEC, 0);  // "Debug.Display.Trigger.JumpDir"
    TvFloat(ctx, 0x01056B00, 0x00AD3B0C, 0x40000000);  // "Audio.ZoneXFadeTime" = 2
    TvBool(ctx, 0x010565E4, 0x00AD3B24, 0);  // "Debug.Display.Trigger.SoundZone"
    TvBool(ctx, 0x01056844, 0x00AD3B44, 0);  // "Debug.Display.Trigger.TimeScale"
    TvInt(ctx, 0x01056890, 0x00AD3B64, 0);  // "Debug.Display.WeatherSystem"
    TvBool(ctx, 0x010568DC, 0x00AD3B84, 0);  // "Debug.Display.Trigger.Weather"
    TvBool(ctx, 0x01056B50, 0x00AD3BA4, 0);  // "Entity.Trigger.Wind.Display"
    TvBool(ctx, 0x01056928, 0x00AD3BC4, 0);  // "Debug.Display.Trigger.UpdateEnable"
    TvBool(ctx, 0x01056974, 0x00AD3BEC, 0);  // "Debug.Display.Trigger.Smoke"
}

// void f(): static (atexit) destructor of 51 tuning vars.
DECOMP(001D4C28, TuningVars_Dtor_001D4C28) {
    TvDtor(ctx, 0x01042168, kVtFloat);
    TvDtor(ctx, 0x0104211C, kVtFloat);
    TvDtor(ctx, 0x010420D0, kVtFloat);
    TvArrayDtor(ctx, 0x01041B78, 2, 76, kVtColor);  // a TuningVarColor[2]
    TvDtor(ctx, 0x01041B28, kVtInt);
    TvDtor(ctx, 0x01041AD8, kVtFloat);
    TvDtor(ctx, 0x01041A88, kVtInt);
    TvDtor(ctx, 0x01042AF0, kVtVec2);
    TvDtor(ctx, 0x01042A90, kVtVec3);
    TvDtor(ctx, 0x01042A30, kVtVec3);
    TvDtor(ctx, 0x010429D8, kVtFloat);
    TvDtor(ctx, 0x01042988, kVtInt);
    TvDtor(ctx, 0x01042938, kVtFloat);
    TvDtor(ctx, 0x010428E8, kVtFloat);
    TvDtor(ctx, 0x01042898, kVtFloat);
    TvDtor(ctx, 0x01042848, kVtFloat);
    TvDtor(ctx, 0x010427F8, kVtFloat);
    TvDtor(ctx, 0x010427A8, kVtFloat);
    TvDtor(ctx, 0x01042758, kVtFloat);
    TvDtor(ctx, 0x01042708, kVtFloat);
    TvDtor(ctx, 0x010426B8, kVtInt);
    TvDtor(ctx, 0x01042668, kVtInt);
    TvArrayDtor(ctx, 0x010425D0, 2, 76, kVtColor);  // a TuningVarColor[2]
    TvDtor(ctx, 0x01042580, kVtBool);
    TvDtor(ctx, 0x01042084, kVtFloat);
    TvDtor(ctx, 0x01042038, kVtFloat);
    TvDtor(ctx, 0x01041FEC, kVtFloat);
    TvDtor(ctx, 0x01041FA0, kVtFloat);
    TvDtor(ctx, 0x01041F54, kVtFloat);
    TvDtor(ctx, 0x01041F08, kVtFloat);
    TvDtor(ctx, 0x01041EBC, kVtFloat);
    TvDtor(ctx, 0x01041E70, kVtHash);
    TvDtor(ctx, 0x01041E24, kVtFloat);
    TvDtor(ctx, 0x01041DD8, kVtFloat);
    TvDtor(ctx, 0x01041D8C, kVtFloat);
    TvDtor(ctx, 0x01041D40, kVtFloat);
    TvDtor(ctx, 0x01041CF4, kVtFloat);
    TvDtor(ctx, 0x01041CA8, kVtFloat);
    TvDtor(ctx, 0x01041C5C, kVtFloat);
    TvDtor(ctx, 0x01041C10, kVtFloat);
    TvDtor(ctx, 0x01042438, kVtFloat);
    TvDtor(ctx, 0x010423E8, kVtVec2);
    TvDtor(ctx, 0x01042398, kVtBool);
    TvDtor(ctx, 0x01042348, kVtInt);
    TvDtor(ctx, 0x010422F8, kVtInt);
    TvDtor(ctx, 0x010422A8, kVtFloat);
    TvDtor(ctx, 0x01042258, kVtFloat);
    TvDtor(ctx, 0x01042208, kVtFloat);
    TvDtor(ctx, 0x010421B8, kVtFloat);
}

// void f(): static (atexit) destructor of 32 tuning vars, 7 PtrVectors, 5 pool arrays.
DECOMP(00389D00, TuningVars_Dtor_00389D00) {
    TvDtor(ctx, 0x0106CF08, kVtColor);
    TvDtor(ctx, 0x0106CEB8, kVtFloat);
    TvDtor(ctx, 0x0106CE68, kVtBool);
    TvDtor(ctx, 0x0106CE18, kVtFloat);
    TvDtor(ctx, 0x0106CDC8, kVtFloat);
    TvDtor(ctx, 0x0106CD78, kVtFloat);
    TvDtor(ctx, 0x0106CD28, kVtFloat);
    TvDtor(ctx, 0x0106CCD8, kVtFloat);
    TvDtor(ctx, 0x0106CC88, kVtFloat);
    TvDtor(ctx, 0x0106CC38, kVtFloat);
    TvDtor(ctx, 0x0106CBE8, kVtFloat);
    TvDtor(ctx, 0x0106CB98, kVtFloat);
    TvDtor(ctx, 0x0106CB48, kVtFloat);
    TvDtor(ctx, 0x0106CAF8, kVtFloat);
    TvDtor(ctx, 0x0106CAAC, kVtFloat);
    TvDtor(ctx, 0x0106CA60, kVtFloat);
    TvDtor(ctx, 0x0106C94C, kVtFloat);
    TvDtor(ctx, 0x0106CA00, kVtVec3);
    TvDtor(ctx, 0x0106C9A0, kVtVec3);
    TvDtor(ctx, 0x0106C900, kVtColor);
    TvDtor(ctx, 0x0106C8A0, kVtVec3);
    TvDtor(ctx, 0x0106C850, kVtFloat);
    TvDtor(ctx, 0x0106C804, kVtFloat);
    TvDtor(ctx, 0x0106C7B8, kVtFloat);
    TvDtor(ctx, 0x0106C76C, kVtFloat);
    TvDtor(ctx, 0x0106C720, kVtFloat);
    TvDtor(ctx, 0x0106C6C0, kVtVec3);
    TvDtor(ctx, 0x0106C66C, kVtFloat);
    TvDtor(ctx, 0x0106C620, kVtFloat);
    TvDtor(ctx, 0x0106C554, kVtString);
    TvDtor(ctx, 0x0106C48C, kVtString);
    TvDtor(ctx, 0x0106C3C4, kVtString);
    VecArrayDtor(ctx, 0x0106C2D4, 4);  // a PtrVector[4]
    VecDtor(ctx, 0x0106C2C0);
    PoolArrayDtor(ctx, 0x0106C258);
    PoolArrayDtor(ctx, 0x0106C240);
    PoolArrayDtor(ctx, 0x0106C228);
    PoolArrayDtor(ctx, 0x0106C210);
    PoolArrayDtor(ctx, 0x0106C2A8);
    VecDtor(ctx, 0x0106C294);
    VecDtor(ctx, 0x0106C280);
}
namespace {
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 PtrVector_Init = 0x00551598;       // (vec, capacity, 0, 0)
}  // namespace

// void f(): static initializer of the streaming module's four sorted
// arrays (compare functions CmpU32_007670DC / 0076710C x2 / 0076713C), a
// PtrVector (capacity 32) and the tuning vars Debug.Display.FileStreaming,
// Loading.Use Anim Globs.
DECOMP(002EBF94, TuningVars_Ctor_002EBF94) {
    SortedArrayCtor(ctx, 0x010686A8, 128, 16, 0x00F36070, true, true);
    SortedArrayCtor(ctx, 0x010686C4, 4, 16, 0x00F36078, true, true);
    SortedArrayCtor(ctx, 0x010686DC, 4, 16, 0x00F36078, true, true);
    SortedArrayCtor(ctx, 0x010686F4, 4, 16, 0x00F36080, true, true);
    call(ctx, PtrVector_Construct, {0x0106870C});
    store<u16>(0x01068712, 4);  // the vector's +6
    call(ctx, PtrVector_Init, {0x0106870C, 32, 0, 0});
    TvBool(ctx, 0x01068728, 0x00AD95D0, 0);  // "Debug.Display.FileStreaming"
    TvBool(ctx, 0x01068778, 0x00AD95F0, 1);  // "Loading.Use Anim Globs"
    ctx.gpr[3] = 1;  // r3 left as the last value, as the original
}

// void f(): static initializer of five sorted arrays, five PtrVectors (the
// first with capacity 256), an f32 global copied from rodata, four words
// cleared atomically and the tuning vars Entity.BlendTimeDefault,
// Entity.CullDistDefault, Debug.Display.EntityHierarchy, Audio.Optimization.
// DisableEntitySounds, AI.SetUpdateEnabledTest, Entity.Damageable.Queue,
// Entity.Projectile.OrientDampening.
DECOMP(000F86DC, TuningVars_Ctor_000F86DC) {
    store<u32>(0x01032198, 0);
    store<u32>(0x0103219C, 0);
    store<u32>(0x010321A0, 0);
    CopyF32(ctx, 0x01013B00, 0x00AC6618);
    SortedArrayCtor(ctx, 0x01013B04, 512, 16, 0x00F31A10, false, false);
    SortedArrayCtor(ctx, 0x01013B1C, 512, 16, 0x00F2E0B8, false, false);
    call(ctx, PtrVector_Construct, {0x01013B34});
    store<u16>(0x01013B3A, 4);  // the vector's +6
    call(ctx, PtrVector_Init, {0x01013B34, 256, 0, 0});
    for (const u32 v : {0x01013B48u, 0x01013B5Cu, 0x01013B70u, 0x01013B84u}) {
        call(ctx, PtrVector_Construct, {v});
        store<u16>(v + 6, v == 0x01013B84u ? 12 : 4);
    }
    TvFloat(ctx, 0x01021BF0, 0x00AC6C3C, 0x3E800000);  // "Entity.BlendTimeDefault" = 0.25
    TvFloat(ctx, 0x01021C3C, 0x00AC6C54, 0x428C0000);  // "Entity.CullDistDefault" = 70
    TvBool(ctx, 0x01021CD8, 0x00AC6C6C, 0);  // "Debug.Display.EntityHierarchy"
    // lwarx / stwcx. loops in the original: atomic stores of 0.
    for (const u32 w : {0x01021D80u, 0x01021E00u, 0x01021E80u, 0x01021F00u})
        std::atomic_ref<u32>(*reinterpret_cast<u32*>(mem::ptr(w))).store(0);
    TvBool(ctx, 0x01021F80, 0x00AC6C8C, 0);  // "Audio.Optimization.DisableEntitySounds"
    TvBool(ctx, 0x01021FCC, 0x00AC6CB4, 1);  // "AI.SetUpdateEnabledTest"
    store<u32>(0x01022080, 0);
    TvInt(ctx, 0x01032100, 0x00AC6CCC, 1);  // "Entity.Damageable.Queue"
    TvFloat(ctx, 0x01021C88, 0x00AC6CE4, 0x41200000);  // "Entity.Projectile.OrientDampening" = 10
    SortedArrayCtor(ctx, 0x0103214C, 16, 16, 0x00F31A68, false, false);
    SortedArrayCtor(ctx, 0x01032164, 16, 16, 0x00F31A70, true, false);
    SortedArrayCtor(ctx, 0x0103217C, 16, 16, 0x00F31A78, false, false);
}
