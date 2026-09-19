#pragma once

#include "mod/preview/ContainerPreview.h"
#include "mod/preview/ShulkerPreviewProvider.h"

#include <array>
#include <optional>

class CompoundTag;
class ItemStack;
class ScreenController;

namespace lamina_peek::preview {

/// Resolves the preview for whatever item is hovered in a container screen,
/// re-extracting only when the hovered item changes.
///
/// This is the glue between the hover tracker and the providers; it knows
/// nothing about rendering.
class HoveredPreviewCache {
public:
    /// Returns the preview for the item hovered in `controller`, or nullptr
    /// when nothing previewable is hovered. The pointer stays valid until the
    /// next call. `controller` must be alive (owned by the view being rendered).
    [[nodiscard]] ContainerPreview const* resolve(ScreenController const& controller);

    void clear();

private:
    // Identity of the item the cached preview was extracted from. A change in
    // any field (new stack in the slot, replaced NBT, different count) forces a
    // fresh extraction.
    struct Key {
        ItemStack const*   stack{nullptr};
        CompoundTag const* userData{nullptr};
        short              id{0};
        short              aux{0};
        unsigned char      count{0};

        bool operator==(Key const&) const = default;
    };

    [[nodiscard]] std::optional<ContainerPreview> extract(ItemStack const& item);

    ShulkerPreviewProvider                mShulkerProvider;
    std::array<PreviewProvider const*, 1> mProviders{&mShulkerProvider}; // future: Bundle provider
    std::optional<Key>                    mKey;
    std::optional<ContainerPreview>       mPreview;
    bool                                  mWarnedExtractionFailure{false};
};

} // namespace lamina_peek::preview
