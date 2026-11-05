// World_HandleEvent callees, part 21i: the sound event lookup across the
// loaded sound banks.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kSoundOn = 0x010AEC60;         // u8: sound system up (as hevent7.h kMusicPushStack)
constexpr u32 kBanks = 0x010AFEB0;           // sorted map {u16 +2 count, +8 {u32 key, Bank*}[]} of loaded banks (name?)
constexpr u32 kGlobalBankKey = 0x010B00D8;   // u32 key of the global bank (name?)
constexpr u32 kGlobalBank = 0x00F65B60;      // u32 Bank*: the global bank (as hevent3_h.cpp kKeepBank)
constexpr u32 kBankFind = 0x00783E80;        // (map, const u32* key) -> value slot (symbols.txt SortedMap_Find783E80)
constexpr u32 kEventFind = 0x00784764;       // (map, const u32* key) -> value slot (symbols.txt SortedMap_Find784764)

// Bank b's event map (+8) entry for the key at keyp, or 0.
inline u32 bank_event(PPUContext& ctx, u32 b, u32 keyp) {
    return load<u32>(static_cast<u32>(call(ctx, kEventFind, {b + 8, keyp})));
}
}  // namespace

// Event* Snd_FindEvent441AC0(u32 hash, u32* bank_key) (name?): with sound
// up and a non-zero hash, looks the event up in the bank named *bank_key
// (the global bank when it is the global key, else the bank map), then in
// the global bank (*bank_key = the global key on a hit), then in every
// loaded bank in map order (*bank_key = that bank's key on a hit).
// Returns the event or 0.
DECOMP(00441AC0, Snd_FindEvent441AC0) {
    const u32 hash = arg<u32>(ctx, 0);
    const u32 keyp = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 hp = frame.addr();
    store<u32>(hp, hash);
    u32 ev = 0;
    if (load<u8>(kSoundOn) == 0 || hash == 0) {
        ret(ctx, 0);
        return;
    }
    bool tried_global = false;
    const u32 key = load<u32>(keyp);
    if (key != 0) {
        u32 b = key == load<u32>(kGlobalBankKey) ? load<u32>(kGlobalBank) : 0;
        if (b == 0) b = load<u32>(static_cast<u32>(call(ctx, kBankFind, {kBanks, keyp})));
        if (b != 0) {
            ev = bank_event(ctx, b, hp);
            tried_global = ev != 0;  // a hit skips the global bank
        }
    }
    if (!tried_global) {
        const u32 g = load<u32>(kGlobalBank);
        if (g != 0) {
            ev = bank_event(ctx, g, hp);
            if (ev != 0) store<u32>(keyp, load<u32>(kGlobalBankKey));
        }
    }
    if (ev == 0) {
        for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(kBanks + 2)); ++i) {
            const u32 b = load<u32>(load<u32>(kBanks + 8) + 8 * i + 4);
            if (b == 0) continue;
            ev = bank_event(ctx, b, hp);
            if (ev != 0) {
                store<u32>(keyp, load<u32>(load<u32>(kBanks + 8) + 8 * i));
                break;
            }
        }
    }
    ret(ctx, ev);
}
