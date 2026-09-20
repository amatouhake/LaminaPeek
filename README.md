# LaminaPeek

A client-side item preview mod for Minecraft Bedrock Edition, built on
[LeviLamina Client](https://github.com/LiteLDev/LeviLamina).

LaminaPeek shows the contents of container-like items directly while you hover
them in an inventory screen, so you can inspect many stored items quickly
without opening each one.

## Target

* LeviLamina **v26.51.1** (client)
* Minecraft Bedrock Edition **1.26.51.x** (Windows x64)

This is a pure client mod. It does not need any server-side component and it
never modifies inventories, sends transactions, or alters packets.

## Current scope (MVP)

* Hover a **Shulker Box** in the player inventory, a Chest, or an Ender Chest.
* Its contents are shown immediately as a 9×3 grid (item icons and stack
  counts), preserving the real slot layout.
* No keybind is required; moving the cursor between Shulker Boxes updates the
  preview instantly.

Planned (not yet implemented): Bundle preview.

## How it works

Three concerns are kept apart so that more item types can be added later:

* `src/mod/hover/` tracks the hovered container slot by observing the game's
  own `ContainerScreenController` hover callbacks.
* `src/mod/preview/` turns a supported item into a `ContainerPreview` grid
  through a small `PreviewProvider` boundary (`ShulkerPreviewProvider` reads
  the `Items` list from the item's NBT).
* `src/mod/render/` draws a `ContainerPreview` on top of the container screen
  from LeviLamina's `AfterUIRenderEvent`.

## Building

Requirements: [xmake](https://xmake.io), Visual Studio 2022 build tools, and a
clang-cl toolchain (LLVM).

```shell
xmake f -y -p windows -a x64 -m release --target_type=client
xmake
```

The packaged mod (`LaminaPeek.dll` + `manifest.json`) is written to
`bin/LaminaPeek/`. Copy that folder into the `mods/` directory of a LeviLamina
client installation (for a LeviLauncher instance:
`%APPDATA%\levilauncher.exe\versions\<version>\mods\LaminaPeek\`).

Keep the generated `manifest.json`: LeviLamina only loads mods whose manifest
says `"type": "native"`. Importing the bare DLL through LeviLauncher's
"import mod" dialog as `preload-native` produces a manifest LeviLamina ignores,
so the mod never loads.

## Configuration

On first start LaminaPeek writes `mods/LaminaPeek/config/config.json` with the
defaults below. Settings are grouped per preview provider so future providers
(for example a `bundle` section) can be added without reshaping the file.

```json
{
    "version": 1,
    "shulker": {
        "enabled": true,
        "showEmpty": false,
        "disableVanillaContentsPreview": true
    }
}
```

* `shulker.enabled` – master switch. When `false`, LaminaPeek draws no Shulker
  preview and leaves vanilla behaviour untouched.
* `shulker.showEmpty` – draw the 9×3 grid for a Shulker Box with no items.
* `shulker.disableVanillaContentsPreview` – drop only the vanilla "contained
  items" lines from the Shulker Box hover text; the item name, custom name,
  lore and every other line stay.

The file is read once at mod load; restart the game after editing it.

For runtime diagnostics, configure with `--trace=y`; the mod then logs hover,
extraction and render events at debug level and mirrors them, flushed
immediately, to `mods/LaminaPeek/trace.log`.

Unit tests for the game-independent layout math:

```shell
xmake build LaminaPeekTests
xmake run LaminaPeekTests
```

## Contributing

Ask questions by creating an issue. PRs accepted.

## License

[MIT](LICENSE) © amatouhake

Bootstrapped from the CC0-1.0 licensed
[levilamina-mod-template](https://github.com/LiteLDev/levilamina-mod-template).
