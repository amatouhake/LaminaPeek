#pragma once

namespace lamina_peek {

/// Persistent settings, stored as JSON in the mod's config directory.
///
/// Settings are grouped per preview provider so that future providers (e.g.
/// "bundle") get their own section instead of new top-level flags. Bump
/// `version` when the layout changes incompatibly.
struct Config {
    int version = 1;

    struct Shulker {
        /// Master switch for the Shulker Box preview. When false LaminaPeek
        /// neither draws a preview nor alters vanilla behaviour for Shulkers.
        bool enabled = true;
        /// Draw the 9x3 grid even when every slot is empty.
        bool showEmpty = false;
        /// Drop only the vanilla "contained items" lines from the Shulker Box
        /// hover text; name, custom name, lore and other lines are kept.
        bool disableVanillaContentsPreview = true;
    } shulker;
};

} // namespace lamina_peek
