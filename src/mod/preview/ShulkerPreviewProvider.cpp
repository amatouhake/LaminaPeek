#include "mod/preview/ShulkerPreviewProvider.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/ListTag.h"
#include "mc/deps/nbt/Tag.h"
#include "mc/world/item/ItemStackBase.h"

#include <string_view>

namespace lamina_peek::preview {

namespace {

constexpr std::string_view kItemsKey   = "Items";
constexpr std::string_view kSlotKey    = "Slot";
constexpr std::string_view kNameSuffix = "shulker_box"; // minecraft:<colour>_shulker_box, minecraft:undyed_shulker_box

// Reads the container slot index of one stored item entry, or -1 if absent or
// not an integer. The game writes it as a byte, but be lenient about the type.
int readSlotIndex(CompoundTag const& entry) {
    auto it = entry.mTags.find(kSlotKey);
    if (it == entry.mTags.end() || !it->second.is_number_integer()) {
        return -1;
    }
    return static_cast<int>(it->second);
}

} // namespace

bool ShulkerPreviewProvider::supports(ItemStackBase const& item) const {
    if (item.isNull()) {
        return false;
    }
    return item.getTypeName().ends_with(kNameSuffix);
}

std::optional<ContainerPreview> ShulkerPreviewProvider::extract(ItemStackBase const& item) const {
    if (!supports(item)) {
        return std::nullopt;
    }

    auto preview = ContainerPreview::empty(kColumns, kRows);

    // No user data (or no "Items" list) simply means an empty box.
    auto const* userData = item.mUserData.get();
    if (!userData) {
        return preview;
    }
    auto itemsIt = userData->mTags.find(kItemsKey);
    if (itemsIt == userData->mTags.end() || !itemsIt->second.is_array()) {
        return preview;
    }

    for (auto const& entryPtr : itemsIt->second.get<ListTag>()) {
        if (!entryPtr || entryPtr->getId() != Tag::Type::Compound) {
            continue;
        }
        auto const& entry = entryPtr->as<CompoundTag>();

        int const slot = readSlotIndex(entry);
        if (slot < 0 || slot >= preview.slotCount()) {
            continue;
        }

        // fromTag resolves the item by name through the client's item registry
        // and yields a null stack for unknown or malformed entries.
        ItemStack stack = ItemStack::fromTag(entry);
        if (stack.isNull()) {
            continue;
        }
        preview.slots[static_cast<size_t>(slot)] = stack;
    }

    return preview;
}

} // namespace lamina_peek::preview
