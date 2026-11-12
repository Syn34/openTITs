// Scripts/Upgrades.xml and Scripts/Awards.xml, read by World_Init.
#include "meta.h"

#include "../audio/more.h"
#include "../common/common.h"
#include "../memory/mem.h"
#include "../string/str.h"
#include "../xml/xml.h"

namespace sly4::meta {
using namespace opentit::native;
using xml::XmlCursor;
using xml::XmlDoc;

namespace {

constexpr u32 kArrayFlags8 = 16;  // 8-byte elements, sorted
constexpr u32 kTitleSize = 64;

// The upgrade being filled: the count is re-read for every field, as in the original.
Upgrade& CurrentUpgrade() { return host<Upgrade>(kUpgrades)[global<be_u32>(kUpgradeCount)]; }

}  // namespace

// Reads <Upgrades><Upgrade .../>...</Upgrades> into the upgrade table (at
// most 83 entries; each field keeps its current value when missing) and
// clears bits 0 and 1 of each upgrade's byte in the world's save data.
// False only when the file can't be parsed.
u32 Upgrades_Load(const char* path) {
    GuestFrame frame(sizeof(XmlDoc));
    XmlDoc* doc = frame.as<XmlDoc>();
    be_u32& count = global<be_u32>(kUpgradeCount);
    count = 0u;
    xml::XmlDoc_Construct(doc, path);
    if (xml::XmlDoc_Parse(doc, 0) == 0) {
        xml::XmlDoc_Destruct(doc);
        return 0;
    }
    XmlCursor* node = xml::XmlCursor_FirstChild(&doc->self, kHashUpgrades);
    if (node) {
        const u32 save = global<be_u32>(kCurrentWorld) + kWorldSaveData;
        node = xml::XmlCursor_FirstChild(node, kHashUpgrade);
        while (node && count + 1 < kUpgradeMax) {
            const auto read_hash = [&](u32 hash, be_u32 Upgrade::*field) {
                be_u32& out = CurrentUpgrade().*field;
                xml::XmlCursor_ReadHash(node, hash, &out, out);
            };
            read_hash(kHashID, &Upgrade::id);
            read_hash(kHashAbilityId, &Upgrade::ability);
            read_hash(kHashType, &Upgrade::type);
            {
                be_u32& out = CurrentUpgrade().cost;
                xml::XmlCursor_ReadS32(node, kHashCost, &out, out);
            }
            read_hash(kHashJobUnlockId, &Upgrade::job_unlock);
            if (const char* s = xml::XmlCursor_FindAttr(node, kHashTitleId))
                str::String_Copy(CurrentUpgrade().title, s, kTitleSize);
            if (const char* s = xml::XmlCursor_FindAttr(node, kHashDescId))
                str::String_Copy(CurrentUpgrade().desc, s, kTitleSize);
            {
                u8& out = CurrentUpgrade().flag;
                xml::XmlCursor_ReadBool(node, kHashUpgradeBool, &out, out);
            }
            gref<u8>(save + count + kSaveUpgradeFlags) &= static_cast<u8>(~0x02);
            gref<u8>(save + count + kSaveUpgradeFlags) &= static_cast<u8>(~0x01);
            count = count + 1;
            node = xml::XmlCursor_NextSibling(node, kHashUpgrade);
        }
    }
    xml::XmlDoc_Destruct(doc);
    return 1;
}

// Rebuilds the award index (a sorted array of {name hash, number}) and the
// award table indexed by Number ({name hash, trophy points: Bronze 15,
// Silver 30, Gold 90, others 0}) from <Awards><Award Name Number
// TrophyGrade/>...</Awards>. False only when the file can't be parsed.
u32 Awards_Load(const char* path) {
    gp<SortedArray>& index_slot = global<gp<SortedArray>>(kAwardIndex);
    be_u32& table_slot = global<be_u32>(kAwardTable);

    // Free the old index and table.
    if (SortedArray* old = index_slot) {
        const u32 data = old->data;
        const u32 bytes = old->capacity * ((static_cast<u32>(old->flags) >> 1) & 0x7FFF);
        if (data != 0) vcall(old->allocator.get(), kAllocVfnFree, data, bytes);
        call(mfn::Array_Destruct, old);
        common::Mem_FreeFrom(old, 0);
        index_slot = nullptr;
    }
    if (const u32 table = table_slot) {
        common::Mem_FreeFrom(host<void>(table), 0);
        table_slot = 0u;
    }
    global<be_u32>(kAwardCount) = 0u;

    auto* index = static_cast<SortedArray*>(memory::Mem_Alloc(sizeof(SortedArray), 0, 16));
    if (index) {
        index->capacity = 0;
        index->count = 0;
        index->high = 0;
        index->flags = kArrayFlags8;
        index->data = 0u;
        Allocator* allocator = call<Allocator*>(mfn::Mem_DefaultAllocator);
        const u32 cap = index->capacity;
        index->allocator = allocator;
        if (cap != 0) {  // never: the capacity was just set to 0
            const u32 esize = (static_cast<u32>(index->flags) >> 1) & 0x7FFF;
            index->data = vcall<u32>(allocator, kAllocVfnAlloc, cap * esize, 0u, 16u);
        }
        call(mfn::Array_Init, index);
        index->w16 = 0u;
        index->compare = kCompareU32;
    }
    // As in the original these are written even when the allocation failed
    // (to guest addresses 2 and 16).
    index_slot = index;
    gref<SortedArray>(guest(index)).count = 0;
    gref<SortedArray>(guest(index)).w16 = 0xFFFFFFFFu;

    struct Locals {
        be_u32 number_out;  // +0
        be_u32 name;        // +4
        be_u32 number;      // +8
        be_u32 grade;       // +12
        XmlDoc doc;         // +16
    };
    GuestFrame frame(sizeof(Locals));
    Locals* l = frame.as<Locals>();
    XmlDoc* doc = &l->doc;
    xml::XmlDoc_Construct(doc, path);
    if (xml::XmlDoc_Parse(doc, 0) == 0) {
        xml::XmlDoc_Destruct(doc);
        return 0;
    }

    if (XmlCursor* awards = xml::XmlCursor_FirstChild(&doc->self, kHashAwards)) {
        // Pass 1: the highest Number.
        s32 highest = static_cast<s32>(0x80000001u);
        for (XmlCursor* a = xml::XmlCursor_FirstChild(awards, kHashAward); a;
             a = xml::XmlCursor_NextSibling(a, kHashAward)) {
            l->number_out = 0xFFFFFFFFu;
            xml::XmlCursor_ReadS32(a, kHashNumber, &l->number_out, 0xFFFFFFFFu);
            const s32 n = static_cast<s32>(l->number_out.get());
            if (n > highest) highest = n;
        }
        const u32 count = static_cast<u32>(highest) + 1;
        global<be_u32>(kAwardCount) = count;
        table_slot = guest(memory::Mem_Alloc(count << 3, 0, 16));

        // Pass 2: fill the index and the table (Number is not range-checked).
        for (XmlCursor* a = xml::XmlCursor_FirstChild(awards, kHashAward); a;
             a = xml::XmlCursor_NextSibling(a, kHashAward)) {
            l->name = 0u;
            l->number = 0xFFFFFFFFu;
            l->grade = 0u;
            xml::XmlCursor_ReadHash(a, kHashName, &l->name, 0);
            xml::XmlCursor_ReadS32(a, kHashNumber, &l->number, l->number);
            xml::XmlCursor_ReadHash(a, kHashTrophyGrade, &l->grade, l->grade);

            SortedArray& arr = gref(index_slot);
            const u32 num = l->number;
            u32 n = arr.count;
            if (n == arr.capacity) {
                audio::Array_Reserve(reinterpret_cast<audio::Array*>(&arr), static_cast<u16>((static_cast<u32>(arr.capacity) << 1) & 0xFFFE));
                n = arr.count;
            }
            const u32 data = arr.data;
            const u32 at = arr.count;
            gref<be_u32>(data + n * 8) = l->name.get();
            arr.count = static_cast<u16>(at + 1);
            gref<be_u32>(data + at * 8 + 4) = num;
            const u32 high = arr.high;
            const u32 flags = arr.flags;
            arr.high = static_cast<u16>(static_cast<s32>(high) > static_cast<s32>((at + 1) & 0xFFFF) ? high : at + 1);
            arr.flags = static_cast<u16>(flags | 1);  // unsorted

            AwardEntry& entry = gref<AwardEntry>(table_slot + (l->number << 3));
            entry.name = l->name.get();
            const u32 g = l->grade;
            u32 points = 0;
            if (g == kHashBronze)
                points = kPointsBronze;
            else if (g == kHashGold)
                points = kPointsGold;
            else if (g == kHashPlatinum)
                points = 0;
            else if (g == kHashSilver)
                points = kPointsSilver;
            entry.value = points;
            call(mfn::Hash_ToString, l->name.get());
        }
    }

    // Sort the index if anything was added.
    SortedArray& arr = gref(index_slot);
    const u32 flags = arr.flags;
    if (flags & 1) {
        const u32 n = arr.count;
        arr.flags = static_cast<u16>(flags & 0xFFFE);
        if (static_cast<s32>(n) > 0) call(mfn::qsort, arr.data.get(), n, 8u, arr.compare.get());
    }
    xml::XmlDoc_Destruct(doc);
    return 1;
}

}  // namespace sly4::meta

NATIVE(0049DC70, sly4::meta::Upgrades_Load)
NATIVE(000B2280, sly4::meta::Awards_Load)
