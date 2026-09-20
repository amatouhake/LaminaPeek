#include "mod/preview/ShulkerTooltipSuppressor.h"

#include "mod/LaminaPeek.h"

#include "ll/api/memory/Hook.h"

#include "mc/safety/RedactableString.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ShulkerBoxBlockItem.h"

namespace lamina_peek::preview {

namespace {

LL_TYPE_INSTANCE_HOOK(
    ShulkerHovertextHook,
    ll::memory::HookPriority::Normal,
    ShulkerBoxBlockItem,
    &ShulkerBoxBlockItem::$appendFormattedHovertext,
    void,
    ::ItemStackBase const&               stack,
    ::Level&                             level,
    ::Bedrock::Safety::RedactableString& hovertext,
    bool const                           showCategory
) {
    auto const& shulker = LaminaPeek::getInstance().getConfig().shulker;
    if (shulker.enabled && shulker.disableVanillaContentsPreview) {
        // Generic item hover text only: everything ShulkerBoxBlockItem adds on
        // top of it is the contents list we are replacing.
        Item::$appendFormattedHovertext(stack, level, hovertext, showCategory);
        return;
    }
    origin(stack, level, hovertext, showCategory);
}

using Hooks = ll::memory::HookRegistrar<ShulkerHovertextHook>;

} // namespace

ShulkerTooltipSuppressor& ShulkerTooltipSuppressor::getInstance() {
    static ShulkerTooltipSuppressor instance;
    return instance;
}

void ShulkerTooltipSuppressor::install() {
    if (mInstalled) return;
    Hooks::hook();
    mInstalled = true;
}

void ShulkerTooltipSuppressor::uninstall() {
    if (!mInstalled) return;
    Hooks::unhook();
    mInstalled = false;
}

} // namespace lamina_peek::preview
