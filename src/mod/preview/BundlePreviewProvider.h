#pragma once

#include "mod/preview/BundleContents.h"
#include "mod/preview/PreviewProvider.h"

namespace lamina_peek::preview {

/// Preview provider for Bundles of every colour (including undyed).
///
/// Data path (26.51.3, evidence below): the item's user data (NBT) `Items`
/// list, decoded per entry with `ItemStack::fromTag` — the same mechanism the
/// Shulker provider and the game itself use to round-trip stored items.
///
/// Why NBT and not the live dynamic-container registry:
/// - `StorageItemComponentTags::{bundleContentTag,bundleIDTag,bundleWeightTag}`
///   link server-side only (probe: `bundleContentTag`/`bundleWeightTag` and
///   `StorageItemUtility::{getStorageItemID,getStorageItemWeightDataClient}`
///   plus `ContainerManagerController::getDynamicContainerModel` all fail to
///   resolve in a client link; only `bundleIDTag` resolves). There is no
///   client-callable "give me all contents" API in 26.51.3 headers.
/// - The live alternative (`getStorageItemID` + `getDynamicContainerModel` +
///   `StorageItemContainerModel::mContainer` -> `Container::getItem`) is
///   server-side-only in this SDK and would couple the preview to vanilla's
///   selection/scroll state (`BundleHelper::mActiveBundleData`,
///   `ContainerScreenController::mContainerManagerController`).
/// - `BundleHelper::getItemStackFromBundle` reads one selected entry by index,
///   not the whole contents; `getStorageItemWeightDataClient` returns weight
///   totals only.
/// - The hovered `ItemStackBase`'s NBT already holds the full contents on the
///   client (the client renders the vanilla tooltip from the same item), and
///   `num_viewable_slots` (12 for vanilla Bundles) only truncates the
///   *tooltip* — the stored list itself is complete. Bedrock storage items
///   support up to 64 dynamic slots (`max_slots: 64` in Mojang's
///   `behavior_pack/items/bundle.json`); every real non-empty entry is drawn.
///
/// Key/slot structure: entries carry a `Slot` byte (container slot index,
/// shared with the Shulker/box-entity save format) and are decoded in slot
/// order, then packed into the dynamic grid (`BundleGrid::shapeFor`).
/// `extract` copies the stored stacks verbatim: insertion order, counts and
/// per-entry data are preserved.
///
/// Runtime evidence still needed (trace build): hover a real Bundle in-game
/// and confirm the debug log's `Bundle NBT keys:` line shows `Items`, and
/// that `entries=N skipped=0` with N matching the true contents. If a future
/// build stores contents under a different key, the log names the exact key
/// to adopt — no guessing.
class BundlePreviewProvider final : public PreviewProvider {
public:
    [[nodiscard]] bool supports(ItemStackBase const& item) const override;

    [[nodiscard]] std::optional<ContainerPreview> extract(ItemStackBase const& item) const override;
};

} // namespace lamina_peek::preview
