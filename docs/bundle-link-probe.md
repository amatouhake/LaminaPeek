# Bundle link probe (client symbol availability, 26.51.3)

Date: 2026-09-21. SDK: `levilamina v26.51.3` (xmake package hash
`95325d4d9e064332ad03ba37dea26256`), `bedrockdata v26.51.1-client.4`.
Build flags: `xmake f -y -p windows -a x64 -m release
--target_type=client` (adds `LL_PLAT_C` via levimc-repo), clang-cl.

## Probe TU (temporary, not committed)

`src/mod/preview/BundleLinkProbe.cpp` (created, built, then deleted):

```cpp
#include "mc/world/containers/FullContainerName.h"
#include "mc/world/containers/managers/controllers/ContainerManagerController.h"
#include "mc/world/containers/models/ContainerModel.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/ItemStackBase.h"
#include "mc/world/item/components/StorageItemComponentTags.h"
#include "mc/world/item/components/storage_item_utility/StorageItemUtility.h"
#include "mc/world/item/components/storage_item_utility/StorageItemWeightData.h"

#include <optional>

namespace probe {

void reference() {
    ItemStackBase const*         item       = nullptr;
    ContainerManagerController*  controller = nullptr;
    [[maybe_unused]] char const* contentTag = StorageItemComponentTags::bundleContentTag();
    [[maybe_unused]] char const* idTag      = StorageItemComponentTags::bundleIDTag();
    [[maybe_unused]] char const* weightTag  = StorageItemComponentTags::bundleWeightTag();
    [[maybe_unused]] std::optional<FullContainerName> id = StorageItemUtility::getStorageItemID(*item);
    [[maybe_unused]] std::optional<StorageItemUtility::StorageItemWeightData> wd =
        StorageItemUtility::getStorageItemWeightDataClient(*item, *controller);
    [[maybe_unused]] std::shared_ptr<ContainerModel> model = controller->getDynamicContainerModel(*id);
    [[maybe_unused]] ItemStack const& stack = controller->getItemStack("InventoryContainer", 0);
    (void)contentTag;
    (void)idTag;
    (void)weightTag;
    (void)stack;
}

} // namespace probe
```

## Result: link fails (LNK2019)

`xmake build LaminaPeek` with the probe present fails at link:

```text
[ 25%]: <LaminaPeek> compiling.release src\mod\preview\BundleLinkProbe.cpp
BundleLinkProbe.cpp.obj : error LNK2019: unresolved external symbol
"__declspec(dllimport) char const (& __cdecl
StorageItemComponentTags::bundleIDTag(void))[0]"
(__imp_?bundleIDTag@StorageItemComponentTags@@YAAEAY0A@$$CBDXZ)
referenced in function "void __cdecl probe::reference(void)"
(?reference@probe@@YAXXZ)
build\windows\x64\release\LaminaPeek.dll : fatal error LNK1120: 1 unresolved external
```

## What this transcript demonstrates — and what it does not

- Demonstrated: `StorageItemComponentTags::bundleIDTag()` does not resolve in
  a client link (LNK2019 above, first-unresolved-symbol stop). That single
  symbol is proven absent from the client link.
- Header-level context (not link proof): `getDynamicContainerModel` and
  `getItemStack` are declared inside an `#ifdef LL_PLAT_C` member-function
  block of `ContainerManagerController` (lines ~201–368, `#endif` at 369),
  and `getStorageItemID` / `getStorageItemWeightDataClient` sit in an
  `#ifdef LL_PLAT_C` block of `StorageItemUtility.h` — declared for the
  client build, yet the client `bedrock_runtime` import surface does not
  resolve them on link (proven for `bundleIDTag`; unprobed for the rest).
  Absent a per-symbol link transcript, the remainder stays investigation,
  not demonstration.
- The probe TU was deleted after the run; the mod builds clean without
  it (`xmake build LaminaPeek` → `build ok`).
- Repro: re-add the TU above verbatim, run `xmake build LaminaPeek`,
  observe LNK2019/LNK1120, delete the TU, rebuild. To promote any further
  candidate to demonstrated, probe it ALONE (one TU referencing only that
  symbol) and record its individual LNK2019/LNK1120 or success here.

## Runtime validation (2026-09-21, 1.26.51.01 / LeviLamina 26.51.3 client)

Trace build (`xmake f ... --trace=y`), disposable creative world, real
Bundles filled via `/give` + the vanilla inventory UI. `trace.log` excerpts:

```text
Bundle NBT keys: [bundle_id]
Bundle extract: entries=0 skipped=0 grid=3x1        # NBT `Items` candidate: a full Bundle previewed as empty
```

The hovered `ItemStackBase`'s user data carries only `bundle_id`; the
contents are NOT in the item's NBT on the client. The `Items` candidate was
therefore wrong and is now only a fallback.

The game's own Bundle UI reads contents through
`BundleHelper::getItemStackFromBundle(ContainerScreenController const&,
ItemStackBase const&, int index)` (client class, `mc/client/gui/screens/
controllers/BundleHelper.h`). Referencing it from the provider links
without any LNK2019 (`xmake build LaminaPeek` → `build ok`), and at runtime
it yields the live stacks:

```text
Bundle extract: source=dynamic-container entries=1 skipped=0 grid=3x1
Bundle extract: source=dynamic-container entries=2 skipped=0 grid=3x1
...
Bundle extract: source=dynamic-container entries=18 skipped=0 grid=5x4
Preview for 'minecraft:bundle': 18/20 slots filled
Bundle extract: source=dynamic-container entries=17 skipped=0 grid=5x4   # one item right-clicked out in place
Bundle extract: source=dynamic-container entries=18 skipped=0 grid=5x4   # re-inserted
Bundle extract: source=dynamic-container entries=0 skipped=0 grid=3x1    # empty Bundle, no panel (showEmpty=false)
```

Hover switching Bundle(18) → Bundle(1×stone) → empty Shulker → empty Bundle
→ Bundle(18) → glass produced the matching `Preview for …` line for each
item (18/20, 1/3, 0/27, 0/3, 18/20, "not previewable") with no stale family
or contents. Render cost while hovering: ~140–220 µs/frame at 60 fps.

## Conclusion (evidence-accurate)

Demonstrated: `StorageItemComponentTags::bundleIDTag` has no client-link
resolution in 26.51.3 (transcript above). Investigated but not individually
link-proven: `bundleContentTag`, `bundleWeightTag`, `getStorageItemID`,
`getStorageItemWeightDataClient`, `getDynamicContainerModel`, `getItemStack`.

Demonstrated at runtime: the hovered Bundle's NBT holds only `bundle_id`,
and `BundleHelper::getItemStackFromBundle` (which links on the client) walks
the live dynamic-container contents by index. That is the provider's data
path now: `extract` iterates indices `0..63` (`max_slots: 64`) through the
hovered slot's `ContainerScreenController` and keeps every non-null stack;
the hover-cache fingerprint walks the same indices (index/id/aux/count, no
decoding) so in-place changes invalidate immediately. Reading is by index
and does not touch `BundleHelper::mActiveBundleData`, so vanilla's
selection/scroll state stays untouched (verified: the wheel-selected slot
highlight keeps working while the preview is shown). The NBT `Items` list
remains a fallback for a Bundle whose contents were flattened into its own
tag, and the trace build reports which path served an extraction
(`source=dynamic-container|nbt-items|none`).
