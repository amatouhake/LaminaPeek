#include "mod/hover/HoverTracker.h"

#include "mod/LaminaPeek.h"

#include "ll/api/memory/Hook.h"

#include "mc/client/gui/ViewRequest.h"
#include "mc/client/gui/screens/controllers/ContainerScreenController.h"
#include "mc/client/gui/screens/controllers/CraftingScreenController.h"
#include "mc/world/item/ItemStackBase.h"

namespace lamina_peek::hover {

namespace {

// The base implementation handles chests, ender chests and every other
// container screen that does not override the callback.
LL_TYPE_INSTANCE_HOOK(
    ContainerSlotHoveredHook,
    ll::memory::HookPriority::Normal,
    ContainerScreenController,
    &ContainerScreenController::$_onContainerSlotHovered,
    ::ui::ViewRequest,
    ::std::string const& collectionName,
    int                  index
) {
    HoverTracker::getInstance().onSlotHovered(*this, collectionName, index);
    return origin(collectionName, index);
}

// The survival inventory screen is a CraftingScreenController, which overrides
// the hover callback. Hooking the override as well guarantees we see inventory
// hovers even if the override does not delegate to the base implementation.
// Both hooks firing for one hover is harmless: the update is idempotent.
LL_TYPE_INSTANCE_HOOK(
    CraftingSlotHoveredHook,
    ll::memory::HookPriority::Normal,
    CraftingScreenController,
    &CraftingScreenController::$_onContainerSlotHovered,
    ::ui::ViewRequest,
    ::std::string const& collectionName,
    int                  index
) {
    HoverTracker::getInstance().onSlotHovered(*this, collectionName, index);
    return origin(collectionName, index);
}

LL_TYPE_INSTANCE_HOOK(
    ContainerSlotUnhoveredHook,
    ll::memory::HookPriority::Normal,
    ContainerScreenController,
    &ContainerScreenController::$_onContainerSlotUnhovered,
    ::ui::ViewRequest,
    ::std::string const& collectionName,
    int                  index
) {
    HoverTracker::getInstance().onSlotUnhovered(*this, collectionName, index);
    return origin(collectionName, index);
}

// Closing a screen does not necessarily emit an unhover, so drop the tracked
// slot as soon as the controller leaves; the pointer must never outlive it.
LL_TYPE_INSTANCE_HOOK(
    ContainerScreenLeaveHook,
    ll::memory::HookPriority::Normal,
    ContainerScreenController,
    &ContainerScreenController::$onLeave,
    void
) {
    HoverTracker::getInstance().onControllerLeft(*this);
    origin();
}

using Hooks = ll::memory::HookRegistrar<
    ContainerSlotHoveredHook,
    CraftingSlotHoveredHook,
    ContainerSlotUnhoveredHook,
    ContainerScreenLeaveHook>;

} // namespace

HoverTracker& HoverTracker::getInstance() {
    static HoverTracker instance;
    return instance;
}

void HoverTracker::install() {
    if (mInstalled) return;
    Hooks::hook();
    mInstalled = true;
}

void HoverTracker::uninstall() {
    if (!mInstalled) return;
    Hooks::unhook();
    mInstalled = false;
    mCurrent.reset();
}

void HoverTracker::onSlotHovered(ContainerScreenController& controller, std::string const& collectionName, int index) {
    if (mCurrent && mCurrent->controller == &controller && mCurrent->collectionIndex == index
        && mCurrent->collectionName == collectionName) {
        return;
    }
    mCurrent = HoveredSlot{&controller, collectionName, index};
    LaminaPeek::getInstance().getSelf().getLogger().debug("Hover {}[{}]", collectionName, index);
}

void HoverTracker::onSlotUnhovered(
    ContainerScreenController& controller,
    std::string const&         collectionName,
    int                        index
) {
    // Only forget the slot we are actually tracking. When the pointer moves
    // directly from one slot to another the game may report the new hover
    // before the old unhover; ignoring mismatched unhovers keeps the new slot.
    if (mCurrent && mCurrent->controller == &controller && mCurrent->collectionIndex == index
        && mCurrent->collectionName == collectionName) {
        mCurrent.reset();
        LaminaPeek::getInstance().getSelf().getLogger().debug("Unhover {}[{}]", collectionName, index);
    }
}

void HoverTracker::onControllerLeft(ContainerScreenController& controller) {
    if (mCurrent && mCurrent->controller == &controller) {
        mCurrent.reset();
    }
}

ItemStackBase const* HoverTracker::resolveItem(ScreenController const& controller) const {
    if (!mCurrent || static_cast<ScreenController const*>(mCurrent->controller) != &controller) {
        return nullptr;
    }
    // _getVisualItemStack is what the game itself uses for hover text: it goes
    // through the virtual _getVisualItemStackImpl, so screens that report
    // hovers for non-container collections (recipe book, creative tabs) resolve
    // them the same way vanilla does instead of touching the raw container.
    return &mCurrent->controller->_getVisualItemStack(mCurrent->collectionName, mCurrent->collectionIndex);
}

} // namespace lamina_peek::hover
