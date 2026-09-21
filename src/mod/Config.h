#pragma once

namespace lamina_peek {

/// Persistent settings, stored as JSON in the mod's config directory.
///
/// Settings are grouped per preview provider so that future providers (e.g.
/// "bundle") get their own section instead of new top-level flags. Bump
/// `version` whenever a field is added or the layout changes: LeviLamina only
/// merges defaults into an existing file when the stored version differs, so
/// an un-bumped schema makes older files fail with "missing required field".
struct Config {
    /// 1: shulker section only (0.1.0). 2: adds the bundle section.
    int version = 2;

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

    struct Bundle {
        /// Master switch for the Bundle preview. When false LaminaPeek draws
        /// no Bundle preview and leaves vanilla behaviour untouched.
        bool enabled = true;
        /// Draw the grid even when the Bundle holds no items.
        bool showEmpty = false;
    } bundle;
};

} // namespace lamina_peek
