# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Bundle preview: hovering a Bundle (any colour) in the player inventory, a
  Chest or an Ender Chest shows every stored stack as a compact dynamic grid
  (3–4 columns, bounded at 16 entries) with item icons and real counts,
  including the tail vanilla's tooltip truncates. Read-only: vanilla
  selection, scroll, insertion and removal are untouched.
- `config.json` gains a `bundle` section (`bundle.enabled`,
  `bundle.showEmpty`); an empty Bundle draws its minimal 3×1 frame only when
  `showEmpty` is set, and shows nothing otherwise.

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
