#include "mod/preview/BundlePreviewProvider.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/ListTag.h"
#include "mc/deps/nbt/Tag.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/ItemStackBase.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace lamina_peek::preview {

namespace {

constexpr std::string_view kItemsKey = "Items";
constexpr std::string_view kSlotKey  = "Slot";

// Reads the container slot index of one stored item entry, or -1 if absent or
// not an integer. The game writes it as a byte, but be lenient about the type.
int readSlotIndex(CompoundTag const& entry) {
    auto it = entry.mTags.find(kSlotKey);
    if (it == entry.mTags.end() || !it->second.is_number_integer()) {
        return -1;
    }
    return static_cast<int>(it->second);
}

// True when `typeName` is a Bundle of any colour: exactly "minecraft:bundle"
// or "<namespace>:<colour>_bundle" ("undyed_bundle" included). A plain
// `ends_with("bundle")` would also match unrelated future items, so the '_'
// (or ':') boundary before "bundle" is required.
bool isBundleTypeName(std::string const& typeName) {
    constexpr std::string_view kSuffix = "bundle";
    if (!typeName.ends_with(kSuffix)) {
        return false;
    }
    if (typeName.size() == kSuffix.size()) {
        return true;
    }
    char const boundary = typeName[typeName.size() - kSuffix.size() - 1];
    return boundary == '_' || boundary == ':';
}

} // namespace

bool BundlePreviewProvider::supports(ItemStackBase const& item) const {
    if (item.isNull()) {
        return false;
    }
    return isBundleTypeName(item.getTypeName());
}

std::optional<ContainerPreview> BundlePreviewProvider::extract(ItemStackBase const& item) const {
    if (!supports(item)) {
        return std::nullopt;
    }

    // No user data (or no "Items" list) simply means an empty Bundle.
    auto const* userData = item.mUserData.get();
    if (!userData) {
        return ContainerPreview::empty(0, 0);
    }
    auto itemsIt = userData->mTags.find(kItemsKey);
    if (itemsIt == userData->mTags.end() || !itemsIt->second.is_array()) {
        return ContainerPreview::empty(0, 0);
    }

    // First pass: decode every entry into its stored slot, tolerating gaps
    // and out-of-range indices. Entries are keyed by Slot (like the Shulker
    // provider) rather than list order, so a reordered or sparse list still
    // maps each stack to the slot the game means.
    struct DecodedEntry {
        int       slot;
        ItemStack stack;
    };
    std::vector<DecodedEntry> decoded;
    int                       skipped = 0;
    for (auto const& entryPtr : itemsIt->second.get<ListTag>()) {
        if (!entryPtr || entryPtr->getId() != Tag::Type::Compound) {
            continue;
        }
        auto const& entry = entryPtr->as<CompoundTag>();

        int const slot = readSlotIndex(entry);
        if (slot < 0 || slot >= BundleGrid::kMaxSlots) {
            continue;
        }

        // fromTag resolves the item by name through the client's item
        // registry and yields a null stack for unknown or malformed entries.
        // A single entry that throws must not take the rest of the Bundle
        // with it, so the failure is contained to its entry.
        try {
            ItemStack stack = ItemStack::fromTag(entry);
            if (stack.isNull()) {
                continue;
            }
            decoded.push_back(DecodedEntry{slot, std::move(stack)});
        } catch (...) {
            ++skipped;
        }
    }

    if (decoded.empty()) {
        auto preview              = ContainerPreview::empty(0, 0);
        preview.skippedSlotCount  = skipped;
        return preview;
    }

    // Second pass: pack in slot order into the bounded dynamic grid. Sorting
    // by stored slot keeps insertion-adjacent items adjacent on screen, and
    // compacting drops the sparse gaps a fixed grid would draw as holes.
    std::sort(decoded.begin(), decoded.end(), [](DecodedEntry const& a, DecodedEntry const& b) {
        return a.slot < b.slot;
    });
    if (static_cast<int>(decoded.size()) > BundleGrid::kMaxSlots) {
        skipped += static_cast<int>(decoded.size()) - BundleGrid::kMaxSlots;
        decoded.resize(static_cast<size_t>(BundleGrid::kMaxSlots));
    }
    BundleGrid const grid   = BundleGrid::shapeFor(static_cast<int>(decoded.size()));
    auto             preview = ContainerPreview::empty(grid.columns, grid.rows);
    for (size_t i = 0; i < decoded.size(); ++i) {
        preview.slots[i] = std::move(decoded[i].stack);
    }
    preview.skippedSlotCount = skipped;
    return preview;
}

} // namespace lamina_peek::preview
