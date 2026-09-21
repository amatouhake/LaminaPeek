#include "mod/preview/BundlePreviewProvider.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/ListTag.h"
#include "mc/deps/nbt/Tag.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/ItemStackBase.h"

#include "mod/LaminaPeek.h"

#include <algorithm>
#include <cassert>
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

#ifdef LAMINAPEEK_TRACE
// Logs the Bundle's raw NBT key names once per distinct key-set so a trace
// build can confirm (or correct) the storage-key assumption in-game: hover a
// real Bundle and read the `Bundle NBT keys:` line in trace.log. Key names
// only — never item data.
void logBundleNbtKeys(CompoundTag const& userData) {
    static std::string sLastKeys;
    std::string        keys;
    for (auto const& [name, _] : userData.mTags) {
        if (!keys.empty()) {
            keys += ',';
        }
        keys += name;
    }
    if (keys != sLastKeys) {
        sLastKeys = keys;
        LaminaPeek::getInstance().getSelf().getLogger().debug("Bundle NBT keys: [{}]", keys);
    }
}
#endif

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

    // No user data (or no "Items" list) simply means an empty Bundle: report
    // the minimal 3x1 frame so `bundle.showEmpty` has something to draw, and
    // let the render layer decide (it skips empty grids unless asked).
    auto const* userData = item.mUserData.get();
    if (!userData) {
        auto preview   = ContainerPreview::empty(BundleGrid::kEmptyColumns, BundleGrid::kEmptyRows);
        preview.family = ContainerPreview::Family::Bundle;
        return preview;
    }
#ifdef LAMINAPEEK_TRACE
    logBundleNbtKeys(*userData);
#endif
    auto itemsIt = userData->mTags.find(kItemsKey);
    if (itemsIt == userData->mTags.end() || !itemsIt->second.is_array()) {
        auto preview   = ContainerPreview::empty(BundleGrid::kEmptyColumns, BundleGrid::kEmptyRows);
        preview.family = ContainerPreview::Family::Bundle;
        return preview;
    }
    // First pass: decode every entry into its stored slot. Entries are keyed
    // by Slot (container slot index, shared with the Shulker/box-entity save
    // format) rather than list order, so a reordered or sparse list still maps
    // each stack to the slot the game means. Anything undecodable is counted
    // as skipped, never drawn. There is no arbitrary cap: Bedrock storage
    // items support up to 64 dynamic slots and every real entry is retained.
    struct DecodedEntry {
        int       slot;
        ItemStack stack;
    };
    std::vector<DecodedEntry> decoded;
    int                       skipped = 0;
    for (auto const& entryPtr : itemsIt->second.get<ListTag>()) {
        if (!entryPtr || entryPtr->getId() != Tag::Type::Compound) {
            ++skipped;
            continue;
        }
        auto const& entry = entryPtr->as<CompoundTag>();

        int const slot = readSlotIndex(entry);
        if (slot < 0 || slot >= BundleGrid::kMaxSlots) {
            ++skipped;
            continue;
        }

        // fromTag resolves the item by name through the client's item
        // registry and yields a null stack for unknown or malformed entries.
        // A single entry that throws must not take the rest of the Bundle
        // with it, so the failure is contained to its entry.
        try {
            ItemStack stack = ItemStack::fromTag(entry);
            if (stack.isNull()) {
                ++skipped;
                continue;
            }
            decoded.push_back(DecodedEntry{slot, std::move(stack)});
        } catch (...) {
            ++skipped;
        }
    }

    if (decoded.empty()) {
        auto preview             = ContainerPreview::empty(BundleGrid::kEmptyColumns, BundleGrid::kEmptyRows);
        preview.family           = ContainerPreview::Family::Bundle;
        preview.skippedSlotCount = skipped;
#ifdef LAMINAPEEK_TRACE
        // Fully-undecodable Bundles still log so the trace shows entries=0
        // with the real skipped count instead of going silent.
        LaminaPeek::getInstance().getSelf().getLogger().debug("Bundle extract: entries=0 skipped={} grid=3x1", skipped);
#endif
        return preview;
    }

    // Second pass: pack in slot order into the dynamic grid. Sorting by stored
    // slot keeps insertion-adjacent items adjacent on screen, and compacting
    // drops the sparse gaps a fixed grid would draw as holes. Duplicate Slot
    // values can push decoded past kMaxSlots even though each index is in
    // range, so truncate to the cap (folding the tail into skipped) BEFORE
    // sizing the grid: shapeFor clamps, but the pack loop must never write
    // more entries than the grid holds.
    std::sort(decoded.begin(), decoded.end(), [](DecodedEntry const& a, DecodedEntry const& b) {
        return a.slot < b.slot;
    });
    if (static_cast<int>(decoded.size()) > BundleGrid::kMaxSlots) {
        skipped += static_cast<int>(decoded.size()) - BundleGrid::kMaxSlots;
        decoded.resize(static_cast<size_t>(BundleGrid::kMaxSlots));
    }
    BundleGrid const grid   = BundleGrid::shapeFor(static_cast<int>(decoded.size()));
    auto             preview = ContainerPreview::empty(grid.columns, grid.rows);
    preview.family           = ContainerPreview::Family::Bundle;
    assert(preview.slots.size() >= decoded.size());
    for (size_t i = 0; i < decoded.size(); ++i) {
        preview.slots[i] = std::move(decoded[i].stack);
    }
    preview.skippedSlotCount = skipped;
#ifdef LAMINAPEEK_TRACE
    LaminaPeek::getInstance().getSelf().getLogger().debug(
        "Bundle extract: entries={} skipped={} grid={}x{}",
        decoded.size(),
        skipped,
        grid.columns,
        grid.rows
    );
#endif
    return preview;
}

} // namespace lamina_peek::preview
