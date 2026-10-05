# etaHEN 2.5B Unified r1 — PS5 11.00–13.60

The multi-firmware line is now the main release. This unofficial etaHEN update combines the controller-input startup fix, a Toolbox card positioned to the left of PlayStation Store, system/game plugin improvements, and working PS4/PS5 FPS display.

## Download and use

Download **etaHEN-11.00-13.60.elf** and load it once after a fresh boot and compatible jailbreak. Do not load it over an existing etaHEN instance. It includes kstuff-lite v1.11; do not load a second kstuff copy.

This is the exact user-tested dev8 payload (SHA-256 `46c161473b27d12f1fa5e2d1d10466ce587db841ed9e8d71a219424d8fcf637f`). Its diagnostic identity remains `ex-diag-20261005.unified-dev8` to preserve traceability. Configuration remains `/data/etaHEN/config-multifw-experimental.ini`, with diagnostics under `/data/etaHEN/experimental-diagnostics/`. The older configuration filename is intentional; the tested binary has not been rebuilt or changed merely for release branding.

## Included

- Controller movement during startup no longer triggers the previously reproduced initialization failure.
- Toolbox has a working dashboard card to the left of Store.
- FPS display for PS5 games and PS4 backward-compatible games; background sampling keeps blocking work out of ShellUI rendering.
- Corrected PS4 hook permissions and separate RX code/RW counters.
- Native etaHEN `.plugin` containers for system and game-aware plugin daemons, with Start/Stop and optional auto-start.
- Game plugins can be armed before opening their game and detect later game sessions.
- Native `.prx` / `.sprx` loading for compatible game modules, with validation, title-specific queues and recorded loader errors.

System plugins: `/data/etaHEN/plugins/`. Game plugins/modules: `/data/etaHEN/game_plugins/<TITLEID>/`. A PRX must match its game version, platform and dependencies. This does not emulate GoldHEN-specific APIs or guarantee compatibility with arbitrary mods. Stop prevents future PRX loads; close the game to unload a resident module.

## Firmware and validation

Exact supported profiles: **11.00, 11.20, 11.60, 12.00, 12.02, 12.20, 12.40, 12.60, 12.70, 13.00, 13.20, 13.40, 13.42, 13.60**. Firmware below 11.00 and 11.40 are excluded.

The owner confirmed PS4 FPS in Minecraft, PS5 FPS in GTA V/WWE/Spider-Man, Toolbox placement/opening, plugin lifecycle and the current build's stability on 13.60. Earlier community startup confirmations cover 11.20, 12.40 and 13.40, including the controller fix on 12.40; they are not full feature tests of this final build on every profile. All 21 local regression checks and the full build passed. No third-party PRX mod has been hardware-validated in this release.

**The GTA V 15 FPS limiter experiment did not work and is not a released feature. Its test plugin is not included.** The normal FPS display is independent of that experiment. The test-only backend remains dormant without its private test controller, preserving the exact tested ELF.

## Source and credit

This is an unofficial community port maintained by **GronedWaffel**, based on **LightningMods and the etaHEN contributors**. FPS integration draws on **OnionHEN**, with upstream FPS research credited to **PHU Games Tools / ArkSama**. Thanks to **John Törnblom / PS5-Payload-dev**, **Echo Stretch**, the kstuff contributors, and all upstream authors and community testers. See [CREDITS.md](https://github.com/GronedWaffel/etahen-11.00-13.60/blob/v2.5B-unified.1/CREDITS.md) for component attribution and licenses.

Source is merged into `main` and tagged for this release. The source archive, build-input hashes, manifest, credits and SHA-256 checksums accompany the ELF. Original GPLv3 and component notices are retained. Previous releases remain available for rollback.
