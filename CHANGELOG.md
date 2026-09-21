# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Bundle preview: hovering a Bundle (any colour) in the player inventory, a
  Chest or an Ender Chest shows every stored stack as a dynamic grid (3
  columns for small Bundles, up to 8×8 for 64 entries) with item icons and
  real counts, including the tail vanilla's tooltip truncates past
  `num_viewable_slots`. Read-only: vanilla selection, scroll, insertion and
  removal are untouched; in-place Bundle edits re-extract via a content
  fingerprint (Shulker path unchanged).
- `config.json` gains a `bundle` section (`bundle.enabled`,
  `bundle.showEmpty`); an empty Bundle draws its minimal 3×1 frame only when
  `showEmpty` is set, and shows nothing otherwise.
- Durability bar in the preview: damaged tools/armor show the vanilla slot
  overlay (black strip with a fill whose width tracks remaining durability,
  green -> yellow -> red; geometry, rounding and colours measured against the
  real 1.26.51 slot) drawn over the icon and glint but under the stack count.
  Undamaged items show no bar; non-damageables and the 9×3 layout are
  unchanged.

## [0.1.0] - 2026-09-21

### Added

- Shulker Box preview: hovering a Shulker Box (any colour) in the player
  inventory, a Chest or an Ender Chest shows its contents as a 9×3 grid with
  item icons and stack counts. No keybind required.
- `config.json` (`mods/LaminaPeek/config/`) with `shulker.enabled`,
  `shulker.showEmpty` and `shulker.disableVanillaContentsPreview`.
- Vanilla's "contained items" hover-text lines are hidden for Shulker Boxes
  while the preview is enabled; the rest of the tooltip is unchanged.

### Changed

- Target LeviLamina Client 26.51.3 (built against its SDK; `--trace=y` uses
  the `RotatePolicy`-based `ll::io::FileSink` introduced in 26.51.2).
