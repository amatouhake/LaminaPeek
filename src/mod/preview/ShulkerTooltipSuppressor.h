#pragma once

namespace lamina_peek::preview {

/// Removes only the vanilla "contained items" lines from a Shulker Box's hover
/// text, so the text preview does not duplicate LaminaPeek's grid.
///
/// ShulkerBoxBlockItem::appendFormattedHovertext is the generic Item hover
/// text plus the contents list. While installed, the hook runs the generic
/// Item implementation instead, keeping name, custom name, enchantments, lore
/// and every other line intact. The hook consults the config on each call, so
/// toggling `shulker.disableVanillaContentsPreview` needs no re-install.
class ShulkerTooltipSuppressor {
public:
    static ShulkerTooltipSuppressor& getInstance();

    void install();
    void uninstall();

private:
    bool mInstalled{false};
};

} // namespace lamina_peek::preview
