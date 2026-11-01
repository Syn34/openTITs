// Command-line switches. Pass them after `--` on the opentit command line:
//     opentit <dump> -- -T -L 3
#include <stdexcept>

#include "sly4.h"

using namespace sly4;

namespace {

// Game fields the switches set (offsets into g::game).
constexpr u32 kArgsCleared = 0x08;  // u32, zeroed first (unused by the parser)
constexpr u32 kSwitchV = 0x0A;      // u8  -V
constexpr u32 kSwitchO = 0x0B;      // u8  -O
constexpr u32 kSwitchL = 0x0C;      // u32 -L<name>, stored as String_HashNoCase(name)
constexpr u32 kSwitchE = 0x10;      // u32 -E<name>, hashed
constexpr u32 kSwitchJ = 0x14;      // u32 -J<name>, hashed
constexpr u32 kSwitchG = 0x18;      // u32 -G<s>, through sub_00324A48
constexpr u32 kSwitchH = 0x1C;      // u8  -H 1
constexpr u32 kSwitchB = 0x1D;      // u8  -B 1
constexpr u32 kSwitchQ = 0x1E;      // char[128] -Q<text>

}  // namespace

// void Main_ParseCommandLine(int argc, char** argv)
//
// Switches start with '-' or '/' and are one letter, either case. A value
// follows the letter directly (-L3) or comes in the next argument (-L 3);
// -H and -B only take the separate form and are true for "1". Each switch
// and its value are removed from argv as they are parsed (the shorter count
// isn't passed back). argv[0] is scanned too, so a path starting with '/'
// would be read as a switch and dropped (openTITs passes no argv[0]).
DECOMP(0046320C, Main_ParseCommandLine) {
    s32 argc = arg<s32>(ctx, 0);
    const u32 argv = arg<u32>(ctx, 1);
    const u32 game = g::game;
    field<u32>(game, kArgsCleared) = 0;

    auto argv_at = [&](s32 i) { return load<u32>(argv + static_cast<u32>(i) * 4); };
    auto hash_value = [&](u32 text) { return static_cast<u32>(call(ctx, fn::String_HashNoCase, {text})); };

    for (s32 i = 0; i < argc;) {
        const u32 text = argv_at(i);
        const u8 lead = ref<u8>(text);
        if (lead != '-' && lead != '/') {
            ++i;
            continue;
        }
        const u32 opt = text + 1;
        const u32 inline_value = opt + 1;  // -L3
        // Only read for switches that take a value, like the original.
        auto has_inline = [&] { return ref<u8>(inline_value) != 0; };
        const bool has_next = i + 1 < argc;
        const u32 next = has_next ? argv_at(i + 1) : 0;
        s32 used = 1;  // arguments this switch takes up

        switch (static_cast<s8>(ref<u8>(opt))) {
        case 'T': case 't':
            field<u8>(g::mem_stats, 0x88) = 1;
            break;
        case 'V': case 'v':
            field<u8>(game, kSwitchV) = 1;
            break;
        case 'O': case 'o':
            field<u8>(game, kSwitchO) = 1;
            break;
        case 'C': case 'c':
            field<u8>(g::config, 0xA1) = 1;
            break;
        case 'Q': case 'q':
            if (has_inline()) {
                call(ctx, fn::String_Copy, {game + kSwitchQ, inline_value, 128});
            } else if (has_next) {
                call(ctx, fn::String_Copy, {game + kSwitchQ, next, 128});
                used = 2;
            }
            break;
        case 'J': case 'j':
        case 'L': case 'l':
        case 'E': case 'e': {
            const char c = static_cast<char>(ref<u8>(opt) | 0x20);
            const u32 off = c == 'j' ? kSwitchJ : c == 'l' ? kSwitchL : kSwitchE;
            if (has_inline()) {
                field<u32>(game, off) = hash_value(inline_value);
            } else if (has_next) {
                field<u32>(game, off) = hash_value(next);
                used = 2;
            }
            break;
        }
        case 'H': case 'h':
        case 'B': case 'b':
            if (has_next) {
                const u32 off = (ref<u8>(opt) | 0x20) == 'h' ? kSwitchH : kSwitchB;
                field<u8>(game, off) = ref<u8>(next) == '1' ? 1 : 0;
                used = 2;
            }
            break;
        case 'G': case 'g': {
            u32 value = 0;
            if (has_inline()) {
                value = inline_value;
            } else if (has_next) {
                value = next;
                used = 2;
            }
            if (value != 0) field<u32>(game, kSwitchG) = static_cast<u32>(call(ctx, fn::sub_00324A48, {value}));
            break;
        }
        case '?':
            call(ctx, fn::exit, {0});
            throw std::logic_error("exit returned");
        default:
            break;  // unknown switches are still removed
        }

        // Drop the switch (and its value) by moving the rest of argv down.
        for (s32 k = i + used; k < argc; ++k) store<u32>(argv + static_cast<u32>(k - used) * 4, argv_at(k));
        argc -= used;
    }
}
