#include "mod/preview/HoveredPreviewCache.h"

#include "mod/LaminaPeek.h"
#include "mod/hover/HoverTracker.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/ListTag.h"
#include "mc/deps/nbt/Tag.h"
#include "mc/world/item/ItemStackBase.h"

namespace lamina_peek::preview {

namespace {

// Fingerprints the Bundle-relevant content bytes of `item`: the `Items` list
// entries (slot, decoded id/aux/count, entry NBT hash) plus the stack's own
// id/aux/count. Returns 0 when the item carries no Bundle content list (empty
// Bundle); 0 also means "nothing to fingerprint", which still compares equal
// across identical empties. Shulker Boxes skip this (their static contents
// change only with a new user-data object, caught by the pointer key).
uint64_t fingerprintBundleContent(ItemStackBase const& item) {
    auto const* userData = item.mUserData.get();
    if (!userData) {
        return 0;
    }
    auto itemsIt = userData->mTags.find("Items");
    if (itemsIt == userData->mTags.end() || !itemsIt->second.is_array()) {
        return 0;
    }
    uint64_t fingerprint = 0;
    for (auto const& entryPtr : itemsIt->second.get<ListTag>()) {
        if (!entryPtr || entryPtr->getId() != Tag::Type::Compound) {
            continue;
        }
        auto const& entry = entryPtr->as<CompoundTag>();
        int         slot  = -1;
        if (auto slotIt = entry.mTags.find("Slot");
            slotIt != entry.mTags.end() && slotIt->second.is_number_integer()) {
            slot = static_cast<int>(slotIt->second);
        }
        // Decode cheaply for id/aux/count only; a decode failure still mixes
        // the entry hash so it participates in the fingerprint.
        short   id    = 0;
        short   aux   = 0;
        uint8_t count = 0;
        try {
            ItemStack stack = ItemStack::fromTag(entry);
            if (!stack.isNull()) {
                id    = stack.getId();
                aux   = stack.mAuxValue;
                count = stack.mCount;
            }
        } catch (...) {
        }
        fingerprint = fingerprintBundleEntries(fingerprint, slot, id, aux, count, entry.hash());
    }
    // Fold the Bundle stack's own identity so a swapped-in Bundle with
    // identical contents still re-extracts (cheap; one mix).
    fingerprint = fingerprintBundleEntries(fingerprint, -2, item.getId(), item.mAuxValue, item.mCount, 0);
    return fingerprint;
}

} // namespace

HoveredPreviewCache::Key HoveredPreviewCache::makeKey(ItemStackBase const& item) {
    Key key;
    key.stack    = &item;
    key.userData = item.mUserData.get();
    key.id       = item.getId();
    key.aux      = item.mAuxValue;
    key.count    = item.mCount;
    // Fingerprinting every hovered item every frame would decode NBT on the
    // hot path; only Bundles mutate in place, so only they pay for it. The
    // predicate is the production one (no duplication).
    if (isBundleTypeName(item.getTypeName())) {
        try {
            key.contentFingerprint = fingerprintBundleContent(item);
        } catch (...) {
            key.contentFingerprint = 0;
        }
    }
    return key;
}

ContainerPreview const* HoveredPreviewCache::resolve(ScreenController const& controller) {
    ItemStackBase const* item = hover::HoverTracker::getInstance().resolveItem(controller);
    if (!item || item->isNull()) {
        clear();
        return nullptr;
    }

    Key const key = makeKey(*item);
    if (!mKey || *mKey != key) {
        mKey     = key;
        mPreview = extract(*item);
        if (!mPreview) {
            LaminaPeek::getInstance().getSelf().getLogger().debug(
                "Hovered '{}' x{} (userData: {}) - not previewable",
                item->getTypeName(),
                item->mCount,
                item->mUserData ? "yes" : "no"
            );
        }
        if (mPreview) {
            LaminaPeek::getInstance().getSelf().getLogger().debug(
                "Preview for '{}': {}/{} slots filled",
                item->getTypeName(),
                mPreview->filledSlotCount(),
                mPreview->slotCount()
            );
            if (mPreview->skippedSlotCount > 0) {
                LaminaPeek::getInstance().getSelf().getLogger().warn(
                    "Preview for '{}': {} slot(s) could not be decoded and were left empty",
                    item->getTypeName(),
                    mPreview->skippedSlotCount
                );
            }
        }
    }
    return mPreview ? &*mPreview : nullptr;
}

void HoveredPreviewCache::clear() {
    mKey.reset();
    mPreview.reset();
}

std::optional<ContainerPreview> HoveredPreviewCache::extract(ItemStackBase const& item) {
    for (auto const* provider : mProviders) {
        if (!provider->supports(item)) {
            continue;
        }
        try {
            return provider->extract(item);
        } catch (...) {
            // Malformed item data must never take the game down. Report once
            // and treat the item as not previewable.
            if (!mWarnedExtractionFailure) {
                mWarnedExtractionFailure = true;
                LaminaPeek::getInstance().getSelf().getLogger().warn(
                    "Failed to extract preview data from hovered item '{}'; further failures are silent",
                    item.getTypeName()
                );
            }
            return std::nullopt;
        }
    }
    return std::nullopt;
}

} // namespace lamina_peek::preview
