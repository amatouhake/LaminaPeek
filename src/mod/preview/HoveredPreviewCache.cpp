#include "mod/preview/HoveredPreviewCache.h"

#include "mod/LaminaPeek.h"
#include "mod/hover/HoverTracker.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/item/ItemStackBase.h"

namespace lamina_peek::preview {

ContainerPreview const* HoveredPreviewCache::resolve(ScreenController const& controller) {
    ItemStackBase const* item = hover::HoverTracker::getInstance().resolveItem(controller);
    if (!item || item->isNull()) {
        clear();
        return nullptr;
    }

    Key const key{item, item->mUserData.get(), item->getId(), item->mAuxValue, item->mCount};
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
