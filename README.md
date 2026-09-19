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

## Building

Requirements: [xmake](https://xmake.io), Visual Studio 2022 build tools, and a
clang-cl toolchain (LLVM).

```shell
xmake f -y -p windows -a x64 -m release --target_type=client
xmake
```

The packaged mod (`LaminaPeek.dll` + `manifest.json`) is written to
`bin/LaminaPeek/`. Copy that folder into the `mods/` directory of a LeviLamina
client installation.

## Contributing

Ask questions by creating an issue. PRs accepted.

## License

[MIT](LICENSE) © amatouhake

Bootstrapped from the CC0-1.0 licensed
[levilamina-mod-template](https://github.com/LiteLDev/levilamina-mod-template).
