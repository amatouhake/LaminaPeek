#pragma once

#include "mod/preview/PreviewProvider.h"

namespace lamina_peek::preview {

/// Preview provider for Bundles of every colour (including undyed).
///
/// Data path: the item's user data (NBT), exactly the way the Shulker Box
/// provider reads a box and the way the game itself saves Bundle contents:
///
///   { "Items": [ { "Slot": 0b, "Count": 1b, "Name": "minecraft:stone",
///                  "Damage": 0s, "tag": {...}, "Block": {...} }, ... ] }
///
/// This is the authoritative client data path: the client already holds the
/// full Bundle contents in the hovered ItemStackBase's NBT (it renders the
/// vanilla tooltip from the same data), so no ContainerManagerController,
/// dynamic-container registry, or server round-trip is needed. The live
/// client-side alternatives were rejected: `getStorageItemWeightDataClient`
/// returns only weight totals (no item list), `getStorageItemID` +
/// `getDynamicContainerModel` exposes the registry model but the Bundle's
/// interactive open/select state lives behind `BundleHelper::mActiveBundleData`
/// and `ContainerScreenController::mContainerManagerController`, neither of
/// which offers a stable read-only extract-contents call in 26.51.3; using
/// them would couple the preview to vanilla's selection/scroll state.
/// `BundleHelper::getItemStackFromBundle` reads one selected entry by index,
/// not the whole contents, so it cannot back a full preview either.
///
/// Unlike a Shulker Box the Bundle has no fixed 9x3 grid: entries are packed
/// in slot order and the layout is derived from the filled count (see
/// `BundleGrid::shapeFor`), so vanilla-truncated tails are still shown in
/// full. `extract` copies the stored stacks verbatim: insertion order,
/// counts and per-entry data are preserved.
class BundlePreviewProvider final : public PreviewProvider {
public:
    [[nodiscard]] bool supports(ItemStackBase const& item) const override;

    [[nodiscard]] std::optional<ContainerPreview> extract(ItemStackBase const& item) const override;
};

/// Bounded dynamic grid for the packed Bundle entries.
///
/// Shulker Boxes mirror the real container (fixed 9x3 with gaps); Bundles
/// store a packed list instead, so the grid is derived from the filled count:
/// rows grow only as needed, columns stay within a narrow band that fits
/// on-screen at ordinary UI sizes, and the heuristic prefers wider shapes
/// (fewer rows) so the preview stays short.
struct BundleGrid {
    /// Hard cap on preview slots. A Bundle never holds more distinct stacks
    /// than this; entries past the cap are counted as skipped, never drawn.
    static constexpr int kMaxSlots = 16;
    /// Narrowest grid: keeps single entries readable instead of a 1-wide strip.
    static constexpr int kMinColumns = 3;
    /// Widest grid: 4 columns of 18-unit cells plus padding is ~80 GUI units,
    /// on-screen at ordinary UI sizes; wider would collide with hover text.
    static constexpr int kMaxColumns = 4;
    /// Empty Bundle frame (drawn only when `bundle.showEmpty` is set): the
    /// narrowest grid, one row, so an empty Bundle is unobtrusive.
    static constexpr int kEmptyColumns = 3;
    static constexpr int kEmptyRows    = 1;
    int columns{0};
    int rows{0};

    /// Smallest grid holding `filled` entries within the column band, wider
    /// shapes preferred: columns grow before rows do. `filled <= 0` yields an
    /// empty (0x0) shape; the caller decides whether to draw it.
    [[nodiscard]] static constexpr BundleGrid shapeFor(int filled) {
        if (filled <= 0) {
            return BundleGrid{0, 0};
        }
        if (filled > kMaxSlots) {
            filled = kMaxSlots;
        }
        for (int columns = kMinColumns; columns <= kMaxColumns; ++columns) {
            if (filled <= columns * columns) {
                int const rows = (filled + columns - 1) / columns;
                return BundleGrid{columns, rows};
            }
        }
        // More entries than a 4x4 square: keep 4 columns and add rows.
        int const rows = (filled + kMaxColumns - 1) / kMaxColumns;
        return BundleGrid{kMaxColumns, rows};
    }
};

} // namespace lamina_peek::preview
