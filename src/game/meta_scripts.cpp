// Scripts/Upgrades.xml and Scripts/Awards.xml, read by World_Init.
#include "meta.h"

#include "../xml/xml.h"

using namespace sly4;
using namespace sly4::meta;
using sly4::xml::xfn::XmlCursor_FindAttr;
using sly4::xml::xfn::XmlCursor_FirstChild;
using sly4::xml::xfn::XmlCursor_NextSibling;
using sly4::xml::xfn::XmlCursor_ReadBool;
using sly4::xml::xfn::XmlCursor_ReadHash;
using sly4::xml::xfn::XmlCursor_ReadS32;
using sly4::xml::xfn::XmlDoc_Construct;
using sly4::xml::xfn::XmlDoc_Destruct;
using sly4::xml::xfn::XmlDoc_Parse;

namespace {

u32 Call32(PPUContext& ctx, u32 addr, std::initializer_list<u64> args) {
    return static_cast<u32>(call(ctx, addr, args));
}
u64 SignExtend(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }

// The upgrade being filled: the count is re-read for every field, as in the original.
u32 CurrentUpgrade() { return kUpgrades + load<u32>(kUpgradeCount) * kUpgradeSize; }

}  // namespace

// bool Upgrades_Load(const char* path)
// Reads <Upgrades><Upgrade .../>...</Upgrades> into the upgrade table (at
// most 83 entries; each field keeps its current value when missing) and
// clears bits 0 and 1 of each upgrade's byte in the world's save data.
// False only when the file can't be parsed.
DECOMP(0049DC70, Upgrades_Load) {
    const u32 path = arg<u32>(ctx, 0);
    Stack frame(ctx, kXmlDocSize);
    const u32 doc = frame.addr();
    store<u32>(kUpgradeCount, 0);
    call(ctx, XmlDoc_Construct, {doc, path});
    if (Call32(ctx, XmlDoc_Parse, {doc, 0}) == 0) {
        call(ctx, XmlDoc_Destruct, {doc});
        return ret(ctx, 0u);
    }
    u32 node = Call32(ctx, XmlCursor_FirstChild, {doc, kHashUpgrades});
    if (node != 0) {
        const u32 save = load<u32>(kCurrentWorld) + kWorldSaveData;
        node = Call32(ctx, XmlCursor_FirstChild, {node, kHashUpgrade});
        while (node != 0 && load<u32>(kUpgradeCount) + 1 < kUpgradeMax) {
            const auto read_hash = [&](u32 hash, u32 field) {
                const u32 out = CurrentUpgrade() + field;
                call(ctx, XmlCursor_ReadHash, {node, hash, out, load<u32>(out)});
            };
            read_hash(kHashID, kUpID);
            read_hash(kHashAbilityId, kUpAbility);
            read_hash(kHashType, kUpType);
            {
                const u32 out = CurrentUpgrade() + kUpCost;
                call(ctx, XmlCursor_ReadS32, {node, kHashCost, out, SignExtend(load<u32>(out))});
            }
            read_hash(kHashJobUnlockId, kUpJobUnlock);
            if (const u32 s = Call32(ctx, XmlCursor_FindAttr, {node, kHashTitleId}))
                call(ctx, mfn::String_Copy, {CurrentUpgrade() + kUpTitle, s, 64});
            if (const u32 s = Call32(ctx, XmlCursor_FindAttr, {node, kHashDescId}))
                call(ctx, mfn::String_Copy, {CurrentUpgrade() + kUpDesc, s, 64});
            {
                const u32 out = CurrentUpgrade() + kUpBool;
                call(ctx, XmlCursor_ReadBool, {node, kHashUpgradeBool, out, ref<u8>(out)});
            }
            ref<u8>(save + load<u32>(kUpgradeCount) + kSaveUpgradeFlags) &= static_cast<u8>(~0x02);
            ref<u8>(save + load<u32>(kUpgradeCount) + kSaveUpgradeFlags) &= static_cast<u8>(~0x01);
            store<u32>(kUpgradeCount, load<u32>(kUpgradeCount) + 1);
            node = Call32(ctx, XmlCursor_NextSibling, {node, kHashUpgrade});
        }
    }
    call(ctx, XmlDoc_Destruct, {doc});
    ret(ctx, 1u);
}

// bool Awards_Load(const char* path)
// Rebuilds the award index (a sorted array of {name hash, number}) and the
// award table indexed by Number ({name hash, trophy points: Bronze 15,
// Silver 30, Gold 90, others 0}) from <Awards><Award Name Number
// TrophyGrade/>...</Awards>. False only when the file can't be parsed.
DECOMP(000B2280, Awards_Load) {
    const u32 path = arg<u32>(ctx, 0);

    // Free the old index and table.
    if (const u32 old = load<u32>(kAwardIndex)) {
        const u32 data = load<u32>(old + kArrData);
        const u32 bytes = load<u16>(old + kArrCapacity) * ((static_cast<u32>(load<u16>(old + kArrFlags)) >> 1) & 0x7FFF);
        if (data != 0) {
            const u32 a = load<u32>(old + kArrAllocator);
            mcall(ctx, a, kAllocVfnFree, {a, data, bytes});
        }
        call(ctx, mfn::Array_Destruct, {old});
        call(ctx, mfn::Mem_FreeFrom, {old, 0});
        store<u32>(kAwardIndex, 0);
    }
    if (const u32 table = load<u32>(kAwardTable)) {
        call(ctx, mfn::Mem_FreeFrom, {table, 0});
        store<u32>(kAwardTable, 0);
    }
    store<u32>(kAwardCount, 0);

    const u32 index = Call32(ctx, mfn::Mem_Alloc, {24, 0, 16});
    if (index != 0) {
        store<u16>(index + kArrCapacity, 0);
        store<u16>(index + kArrCount, 0);
        store<u16>(index + kArrHigh, 0);
        store<u16>(index + kArrFlags, 16);  // 8-byte elements
        store<u32>(index + kArrData, 0);
        const u32 allocator = Call32(ctx, mfn::Mem_DefaultAllocator, {});
        const u32 cap = load<u16>(index + kArrCapacity);
        store<u32>(index + kArrAllocator, allocator);
        if (cap != 0) {  // never: the capacity was just set to 0
            const u32 esize = (static_cast<u32>(load<u16>(index + kArrFlags)) >> 1) & 0x7FFF;
            store<u32>(index + kArrData,
                       static_cast<u32>(mcall(ctx, allocator, kAllocVfnAlloc, {allocator, cap * esize, 0, 16})));
        }
        call(ctx, mfn::Array_Init, {index});
        store<u32>(index + kArr16, 0);
        store<u32>(index + kArrCompare, kCompareU32);
    }
    // As in the original these are written even when the allocation failed
    // (to guest addresses 2 and 16).
    store<u32>(kAwardIndex, index);
    store<u16>(index + kArrCount, 0);
    store<u32>(index + kArr16, 0xFFFFFFFFu);

    Stack frame(ctx, 16 + kXmlDocSize);  // r1+112..127 of the original, then the XmlDoc
    const u32 number_out = frame.addr();          // r1+112
    const u32 name = frame.addr() + 4, number = frame.addr() + 8, grade = frame.addr() + 12;
    const u32 doc = frame.addr() + 16;            // r1+128
    call(ctx, XmlDoc_Construct, {doc, path});
    if (Call32(ctx, XmlDoc_Parse, {doc, 0}) == 0) {
        call(ctx, XmlDoc_Destruct, {doc});
        return ret(ctx, 0u);
    }

    if (const u32 awards = Call32(ctx, XmlCursor_FirstChild, {doc, kHashAwards})) {
        // Pass 1: the highest Number.
        s32 highest = static_cast<s32>(0x80000001u);
        for (u32 a = Call32(ctx, XmlCursor_FirstChild, {awards, kHashAward}); a != 0;
             a = Call32(ctx, XmlCursor_NextSibling, {a, kHashAward})) {
            store<s32>(number_out, -1);
            call(ctx, XmlCursor_ReadS32, {a, kHashNumber, number_out, SignExtend(0xFFFFFFFFu)});
            const s32 n = load<s32>(number_out);
            if (n > highest) highest = n;
        }
        const u32 count = static_cast<u32>(highest) + 1;
        store<u32>(kAwardCount, count);
        store<u32>(kAwardTable, Call32(ctx, mfn::Mem_Alloc, {count << 3, 0, 16}));

        // Pass 2: fill the index and the table (Number is not range-checked).
        for (u32 a = Call32(ctx, XmlCursor_FirstChild, {awards, kHashAward}); a != 0;
             a = Call32(ctx, XmlCursor_NextSibling, {a, kHashAward})) {
            store<u32>(name, 0);
            store<s32>(number, -1);
            store<u32>(grade, 0);
            call(ctx, XmlCursor_ReadHash, {a, kHashName, name, 0});
            call(ctx, XmlCursor_ReadS32, {a, kHashNumber, number, SignExtend(load<u32>(number))});
            call(ctx, XmlCursor_ReadHash, {a, kHashTrophyGrade, grade, load<u32>(grade)});

            const u32 arr = load<u32>(kAwardIndex);
            const u32 num = load<u32>(number);
            u32 n = load<u16>(arr + kArrCount);
            if (n == load<u16>(arr + kArrCapacity)) {
                call(ctx, mfn::Array_Reserve, {arr, (static_cast<u32>(load<u16>(arr + kArrCapacity)) << 1) & 0xFFFE});
                n = load<u16>(arr + kArrCount);
            }
            const u32 data = load<u32>(arr + kArrData);
            const u32 at = load<u16>(arr + kArrCount);
            store<u32>(data + n * 8, load<u32>(name));
            store<u16>(arr + kArrCount, static_cast<u16>(at + 1));
            store<u32>(data + at * 8 + 4, num);
            const u32 high = load<u16>(arr + kArrHigh);
            const u32 flags = load<u16>(arr + kArrFlags);
            store<u16>(arr + kArrHigh, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>((at + 1) & 0xFFFF) ? high : at + 1));
            store<u16>(arr + kArrFlags, static_cast<u16>(flags | 1));  // unsorted

            const u32 entry = load<u32>(kAwardTable) + (load<u32>(number) << 3);
            store<u32>(entry, load<u32>(name));
            const u32 g = load<u32>(grade);
            u32 points = 0;
            if (g == kHashBronze)
                points = kPointsBronze;
            else if (g == kHashGold)
                points = kPointsGold;
            else if (g == kHashPlatinum)
                points = 0;
            else if (g == kHashSilver)
                points = kPointsSilver;
            store<u32>(entry + 4, points);
            call(ctx, mfn::Award_Register, {load<u32>(name)});
        }
    }

    // Sort the index if anything was added.
    const u32 arr = load<u32>(kAwardIndex);
    const u32 flags = load<u16>(arr + kArrFlags);
    if (flags & 1) {
        const u32 n = load<u16>(arr + kArrCount);
        store<u16>(arr + kArrFlags, static_cast<u16>(flags & 0xFFFE));
        if (static_cast<s32>(n) > 0) call(ctx, mfn::qsort, {load<u32>(arr + kArrData), n, 8, load<u32>(arr + kArrCompare)});
    }
    call(ctx, XmlDoc_Destruct, {doc});
    ret(ctx, 1u);
}
