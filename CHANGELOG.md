# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Shulker Box preview: hovering a Shulker Box (any colour) in the player
  inventory, a Chest or an Ender Chest shows its contents as a 9×3 grid with
  item icons and stack counts. No keybind required.
- `config.json` (`mods/LaminaPeek/config/`) with `shulker.enabled`,
  `shulker.showEmpty` and `shulker.disableVanillaContentsPreview`.
- Vanilla's "contained items" hover-text lines are hidden for Shulker Boxes
  while the preview is enabled; the rest of the tooltip is unchanged.
