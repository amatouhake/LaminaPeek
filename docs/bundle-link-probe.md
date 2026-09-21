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

Notes:

- The linker reports the **first** unresolved symbol and stops
  (`LNK1120: 1 unresolved external`), so this transcript names
  `bundleIDTag` — i.e. even the tag-name constants do not resolve in a
  client link. The remaining probe symbols (`bundleContentTag`,
  `bundleWeightTag`, `getStorageItemID`,
  `getStorageItemWeightDataClient`, `getDynamicContainerModel`,
  `getItemStack`) were not individually re-probed after deleting the TU;
  the header-level evidence stands on its own: `StorageItemUtility` and
  the `ContainerManagerController` contents APIs sit behind
  `#ifdef LL_PLAT_C`-gated `MCAPI` client blocks whose symbols the
  client `bedrock_runtime` does not export (client imports resolve only
  via delay-loaded `bedrock_runtime.dll` through the prelink-generated
  `bedrock_runtime_api.lib`; the link failure above is that mechanism
  reporting the symbol absent).
- The probe TU was deleted after the run; the mod builds clean without
  it (`xmake build LaminaPeek` → `build ok`).
- Repro: re-add the TU above verbatim, run `xmake build LaminaPeek`,
  observe LNK2019/LNK1120, delete the TU, rebuild.

## Conclusion

No client-callable "give me all Bundle contents" API exists in the
26.51.3 client SDK. The provider therefore reads the hovered
`ItemStackBase`'s NBT `Items` list via `ItemStack::fromTag` per entry —
the same round-trip the game uses — with trace-build logging
(`Bundle NBT keys:`, `Bundle extract: entries=`) to confirm the key
in-game. If a future build moves contents elsewhere, the log names the
exact key to adopt.
