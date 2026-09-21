#pragma once

#include "mod/preview/BundleContents.h"
#include "mod/preview/PreviewProvider.h"

namespace lamina_peek::preview {

/// Preview provider for Bundles of every colour (including undyed).
///
/// Status: implementation ready for runtime validation — the data path below
/// is the current CANDIDATE, not a confirmed fix. Client alternatives were
/// investigated and none usable was established (see
/// `docs/bundle-link-probe.md`); trace-build diagnostics log the actual NBT
/// key set in-game, and runtime confirmation is a merge gate.
///
/// Data path (26.51.3, candidate): the item's user data (NBT) `Items` list,
/// decoded per entry with `ItemStack::fromTag` — the same mechanism the
/// Shulker provider and the game itself use to round-trip stored items.
/// `extract` (which runs only on cache-key change) is the one place
/// `fromTag` is allowed; the hover-cache fingerprint path never calls it.
///
/// Why NBT and not the live dynamic-container registry (see
/// `docs/bundle-link-probe.md` for provenance; only the symbol(s) named
/// there with a transcript are demonstrated — the rest are investigated
/// header candidates, and no usable whole-contents route is established):
/// - `StorageItemComponentTags::{bundleContentTag,bundleIDTag,bundleWeightTag}`,
///   `StorageItemUtility::{getStorageItemID,getStorageItemWeightDataClient}`
///   and `ContainerManagerController::getDynamicContainerModel` are header
///   candidates with no usable whole-contents route established on the
///   client in 26.51.3 (see the probe doc for exactly which symbols carry a
///   link transcript and which remain header-level).
/// - The live alternative (`getStorageItemID` + `getDynamicContainerModel` +
///   `StorageItemContainerModel::mContainer` -> `Container::getItem`) is
///   server-side-only in this SDK and would couple the preview to vanilla's
///   selection/scroll state (`BundleHelper::mActiveBundleData`,
///   `ContainerScreenController::mContainerManagerController`).
/// - `BundleHelper::getItemStackFromBundle` reads one selected entry by index,
///   not the whole contents; `getStorageItemWeightDataClient` returns weight
///   totals only.
/// - The hovered `ItemStackBase`'s NBT is expected to hold the full contents
///   on the client (the client renders the vanilla tooltip from the same
///   item), and `num_viewable_slots` (12 for vanilla Bundles) only truncates
///   the *tooltip* — the stored list itself is expected complete. Bedrock
///   storage items support up to 64 dynamic slots (`max_slots: 64` in
///   Mojang's `behavior_pack/items/bundle.json`); every real non-empty entry
///   is drawn.
///
/// Key/slot structure: entries carry a `Slot` byte (container slot index,
/// shared with the Shulker/box-entity save format) and are decoded in slot
/// order, then packed into the dynamic grid (`BundleGrid::shapeFor`).
/// `extract` copies the stored stacks verbatim: insertion order, counts and
/// per-entry data are preserved.
///
/// Runtime gates (trace build, all must pass before this is confirmed): hover
/// a real Bundle in-game and confirm the debug log's `Bundle NBT keys:` line
/// shows the actual key set (`Items` expected); `Items` present or the
/// candidate corrected; `entries=N skipped=0` with N matching the true
/// contents; a >12-entry Bundle shows the vanilla-hidden tail; a large/full
/// Bundle previews correctly; in-place insert/remove invalidates and
/// updates. If the build stores contents under a different key, the log
/// names the exact key to adopt — no guessing.
class BundlePreviewProvider final : public PreviewProvider {
public:
    [[nodiscard]] bool supports(ItemStackBase const& item) const override;

    [[nodiscard]] std::optional<ContainerPreview> extract(ItemStackBase const& item) const override;
};

} // namespace lamina_peek::preview
