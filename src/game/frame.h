// Names for the per-frame world functions Game_Update calls after the world
// update: Audio_Update (0x0044B310) and World_PostUpdate (0x0045A9B8).
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include <bit>
#include <cmath>

#include "../sly4.h"

namespace sly4 {
namespace frame {

// ---------------------------------------------------------------- PPU float ops
// Single-precision forms round the double result to float (rt::to_single).
inline f32 fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }
inline f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }
inline f32 fsqrts(f64 a) { return static_cast<f32>(std::sqrt(a)); }
inline f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }  // a * c + b

inline u32 vfunc(u32 obj, u32 offset) { return load<u32>(load<u32>(obj) + offset); }
// Copies a float as its bits (lfs / stfs pairs in the original).
inline void copy_word(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// ---------------------------------------------------------------- audio
namespace afn {
constexpr u32 sub_00445A40 = 0x00445A40;     // (0x010ACC40, f32 dt)
constexpr u32 sub_00447860 = 0x00447860;     // (f32 dt)
constexpr u32 sub_004473C0 = 0x004473C0;     // (f32 dt)
constexpr u32 Math_Atan2 = 0x005E665C;       // f32 (f32 y, f32 x)? (name?)
constexpr u32 Voice_IsPlaying = 0x006D5E10;  // (voice) -> nonzero while playing (name?)
constexpr u32 Voice_GetValue = 0x006D64AC;   // (voice) -> s32 (name?)
constexpr u32 Voice_Stop = 0x006D60AC;       // (voice, 1) (name?)
constexpr u32 Emitter_Start = 0x0044448C;    // (u32 handle) (name?)
constexpr u32 Emitter_Update = 0x00442C10;   // (Emitter*, Listener*, -, bool listener_still, f32 dt) (name?)
constexpr u32 Emitter_Update2 = 0x00442250;  // (Emitter*) (name?)
constexpr u32 sub_0044A970 = 0x0044A970;     // (f32 dt), 610 instructions
constexpr u32 Pool_Free = 0x00552420;        // (void*, u32 size) (name?)
constexpr u32 HandlePool_Remove = 0x0078378C;  // (out, Pool*, u32 handle) (name?)
constexpr u32 KeyMap_Find = 0x00784008;      // (Map*, const u32* key) -> s32 index or < 0 (name?)
}  // namespace afn

constexpr u32 kAudioReady = 0x010AEC60;          // u8: the audio system is up (also checked by 0x0044BA6C)
constexpr u32 kAudioListenerInit = 0x010B15F8;   // u8
constexpr u32 kAudioLastListenerPos = 0x010B1600;  // f32[3]
constexpr u32 kAudioListenerStartPos = 0x01164B40; // f32[3] copied on first use
constexpr u32 kTuneUpdateBuckets = 0x010B0B28;   // tuning object, s32 at +72: log2 of the bucket count
constexpr u32 kTuningValue = 72;
constexpr u32 kAudioUpdateObj = 0x010ACC40;      // passed to sub_00445A40
constexpr u32 kAudioListenerMatrix = 0x010B0230; // f32[16] copy of the listener (w column not copied)
constexpr u32 kAudioListenerHeading = 0x00F65B84;  // f32 degrees
constexpr u32 kAudioFrame = 0x010B00DC;          // u32 frame counter
constexpr u32 kAudioBucketCounts = 0x010B00E0;   // s32[] emitters per bucket
constexpr u32 kAudioFocusHandle = 0x010AFEA4;    // u16 index, u16 serial (name?)
constexpr u32 kAudioFocusObj = 0x00F65B70;       // u32 object with a count at +20 (name?)
constexpr u32 kAudioFocusList = 0x00F65B7C;      // u32 list head for those objects
constexpr u32 kAudioKeyMap = 0x010AFEF8;         // sorted map: u16 count at +2, entries at +8 (12 bytes), default at +16
constexpr u32 kAudioClearB68 = 0x00F65B68;       // u8 cleared every frame
constexpr u32 kAudioDirty = 0x00F65B88;          // u8 cleared every frame (set by 0x0044BA6C)
constexpr u32 kMaxBuckets = 8;
constexpr u32 kListenerPos = 48;                 // Listener: f32[16] matrix, position at +48

// Emitter pool (0x010B0200): nodes of 272 bytes, emitter data at +16,
// handle serial at +256.
constexpr u32 kEmitters = 0x010B0200;
constexpr u32 kPoolFirst = 8, kPoolLast = 12, kPoolUsed = 16, kPoolCapacity = 24, kPoolNodes = 28;
constexpr u32 kNodeSize = 272, kNodeNext = 4, kNodeData = 16, kNodeSerial = 256;
constexpr u8 kNodeFree = 0x80;  // in the first byte of +256

// Emitter (node +16) fields.
constexpr u32 kEmVoice = 0;        // u32 voice handle
constexpr u32 kEmDef = 4;          // u32 SoundDef*: f32 radius at +40, s32 at +88, u8 virtualisable at +96
constexpr u32 kEmBucketed = 12;    // u8: counted in kAudioBucketCounts, updated in its bucket
constexpr u32 kEmVirtual = 13;     // u8: out of range, waiting to start again (name?)
constexpr u32 kEmDistance = 116;   // f32
constexpr u32 kEmKey = 156;        // u32 key in kAudioKeyMap, 0 if none
constexpr u32 kEmBucket = 164;     // s32
constexpr u32 kEmPosition = 176;   // f32[3]
constexpr u32 kDefRadius = 40, kDefMaxValue = 88, kDefVirtualisable = 96;

// Constants just before Audio_Update (0x0044B2FC).
constexpr f32 kStillEpsilon = std::bit_cast<f32>(0x3727C5ACu);  // 1e-5
constexpr f32 kRadToDeg = std::bit_cast<f32>(0x42652EE0u);      // 57.29578


// ---------------------------------------------------------------- World_SubmitFrame (0x0045A9B8)
namespace wfn {
constexpr u32 RenderList_Grow = 0x00551718;   // (RenderList*, u32 new capacity) (name?)
constexpr u32 View_SubmitSplit = 0x0045A6F8;  // (World*, View*, Camera*, u32 index, Job*) (name?)
constexpr u32 Camera_IsSplit = 0x002A6044;    // (Camera*) -> bool; frame_misc.cpp (name?)
constexpr u32 View_Notify = 0x0001BEE8;       // (obj, View*): obj+36 vfunc +32 (view); frame_misc.cpp (name?)
constexpr u32 sub_00457654 = 0x00457654;      // (World*) -> u32
constexpr u32 sub_005AC280 = 0x005AC280;      // (View* main)
constexpr u32 sub_0045A208 = 0x0045A208;      // (World*)
constexpr u32 Overlay_GetValue = 0x000AB93C;  // () -> u32 at 0x00F558D0; frame_misc.cpp (name?)
constexpr u32 sub_00459198 = 0x00459198;      // (Job*, u32, 255)
constexpr u32 sub_000AB7B0 = 0x000AB7B0;      // (obj)
constexpr u32 FrameBuffer_PushMarker = 0x005220E8;  // (FrameBuffer*, -, u32 v); frame_misc.cpp (name?)
constexpr u32 Arena_Snapshot = 0x00565178;    // (Arena*, out); frame_misc.cpp (name?)
constexpr u32 sub_00459E10 = 0x00459E10, sub_00459B60 = 0x00459B60, sub_004598D0 = 0x004598D0;  // (World*)
constexpr u32 sub_004596E8 = 0x004596E8;      // (World*, bool)
constexpr u32 sub_00459500 = 0x00459500, sub_00459308 = 0x00459308;  // (World*)
}  // namespace wfn

// Render job list: {opd, arg} pairs of 8 bytes.
constexpr u32 kRenderList = 0x010BB854;   // +0 entries, u16 +4 capacity, u16 +8 count, u16 +10 high water
constexpr u32 kJobEndFrame = 0x00F39E50;  // OPD (code 0x0078896C), pushed last with kJobEndFrameArg
constexpr u32 kJobEndFrameArg = 0x010BBE0C;
constexpr u32 kJobRun = 0x00F2E248;       // OPD (code 0x00737278), pushed with one of the jobs below
constexpr u32 kJobView0 = 0x010BBE44, kJobView1 = 0x010BBE50, kJobOverlay = 0x010BBE5C;  // u32 job, 0 if none
constexpr u32 kJobSplitIndex = 0x010BBE1C;  // u8; byte at kJobEndFrameArg + 4 + 8 * index gets 0x80

constexpr u32 kFrameArenas = 0x00F65C90;  // u32: { Arena* a, Arena* b, Arena* current, u32 0 }, built on first use
constexpr u32 kArenaSize = 0xA000;
constexpr u32 kCamera = 0x00F64A58;       // u32 Camera* (name?)
constexpr u32 kCameraSplitBits = 244;     // u32: top four bits nonzero = split views
constexpr u32 kSub1BEE8Arg = 0x00F557F0;  // u32
constexpr u32 kCurrentWorld = 0x00F65CA0; // u32 World* (Game_Update's update::kCurrentWorld is this same pointer?)
constexpr u32 kClearedA = 0x00F6688C, kClearedB = 0x00F66890;  // u32, zeroed with the overlay job
constexpr u32 kTuneSkipA = 0x010B89D0, kTuneSkipB = 0x010B8A1C;  // tuning objects, u8 at +72
constexpr u32 kTuneNoFlag = 0x010B9D28;   // tuning object, u8 at +72

// Game / World fields.
constexpr u32 kGameFlags5E40 = 0x5E40;   // u8: 0x80 = skip the frame (only the end job)
constexpr u32 kGame5E38 = 0x5E38;        // u32 (main.cpp handles it with sub_0051299C)
constexpr u32 kWorld47 = 47;             // u8
constexpr u32 kWorldMainView = 0x128168; // View*[4] (Game_Update hands them the viewport)
constexpr u32 kWorldSplitViews = 0x128178;  // View*[2]
constexpr u32 kWorld1281A4 = 0x1281A4;   // u32
constexpr u32 kWorldCallbacksA = 0x1284D0;  // {entries, -, u16 count}: 12-byte {opd, arg, -}, cleared after running
constexpr u32 kWorldCallbacksB = 0x1284E4;  // same, not cleared
constexpr u32 kWorldOverlay = 0x1286A4;  // u32 obj
constexpr u32 kWorld1285E8 = 0x1285E8;   // u32, zeroed
constexpr u32 kWorldFrameRing = 0x1285E4;
constexpr u32 kWorldVfnState = 32;       // (u32 depth) -> state

}  // namespace frame
}  // namespace sly4
