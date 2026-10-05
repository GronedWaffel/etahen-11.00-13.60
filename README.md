# etaHEN Unified — unofficial PS5 11.00–13.60 port

**[Download etaHEN 2.5B Unified r1](https://github.com/GronedWaffel/etahen-11.00-13.60/releases/tag/v2.5B-unified.1)** — choose `etaHEN-11.00-13.60.elf`.

Based on etaHEN 2.5B by **LightningMods and the etaHEN contributors**, maintained here by **GronedWaffel**. This is an independent community release, not an official etaHEN release. Please credit and support [etaHEN](https://github.com/etaHEN/etaHEN).

The multi-firmware source is merged into `main`. Unified r1 includes the controller-input startup correction, the working Toolbox card to the left of Store, system/game `.plugin` support, native game `.prx` / `.sprx` loading, and PS4/PS5 FPS display. The unsuccessful GTA limiter experiment is not included as a release feature or downloadable plugin.

[Release notes](RELEASE-NOTES-unified-r1.md) · [Credits](CREDITS.md) · [Build instructions](BUILDING.md) · [Validation record](PORT-STATUS.md) · [Contributing](CONTRIBUTING.md) · [Original README](UPSTREAM-README.md)

## Firmware and startup

Exact profiles: **11.00, 11.20, 11.60, 12.00, 12.02, 12.20, 12.40, 12.60, 12.70, 13.00, 13.20, 13.40, 13.42 and 13.60**. Below 11.00 and 11.40 are not supported.

Use a compatible jailbreak/ELF loader and load the full payload once after a fresh boot/jailbreak. Do not stack it over an existing etaHEN instance or load a second kstuff. The binary retains the tested configuration path `/data/etaHEN/config-multifw-experimental.ini` and diagnostic identity `ex-diag-20261005.unified-dev8`.

The owner confirmed working PS4 FPS in Minecraft, PS5 FPS in GTA V/WWE/Spider-Man, Toolbox placement/opening, plugin lifecycle and current build stability on 13.60. Earlier community startup confirmations exist for 11.20, 12.40 and 13.40. These are scoped observations, not verification of every feature on every firmware. All 21 local checks and the complete build passed.

## Plugins and FPS

- System `.plugin` containers: `/data/etaHEN/plugins/` (or the supported USB etaHEN plugin folder).
- Game `.plugin`, `.prx`, `.sprx`: `/data/etaHEN/game_plugins/<TITLEID>/`.
- Start can arm game plugins before their game opens. Auto-start is optional.
- A PRX must match its game/platform/version and available imports. Third-party PRX compatibility is not hardware-validated; GoldHEN-specific APIs are not emulated. Stop disarms future loads; close the game to unload a resident module.
- Enable the FPS overlay in Toolbox. PS4 measurements count GNM flip submissions; PS5 sampling follows OnionHEN's render/scanout estimation. Game rendering behavior can affect measurements.

## Source, builds and rollback

The exact tested ELF is released unchanged. Its SHA-256 is `46c161473b27d12f1fa5e2d1d10466ce587db841ed9e8d71a219424d8fcf637f`. See the attached manifest, source archive and checksums. Development fixture sources remain for reproducibility; the GTA limiter test plugin is not a release asset.

[BUILDING.md](BUILDING.md) documents Node, Zig, PS5 payload SDK v0.43 and external inputs. [BUILD-INPUTS.json](BUILD-INPUTS.json) records dependency hashes. SDKs, upstream binaries and static archives are obtained separately. Earlier [13.60 r3](https://github.com/GronedWaffel/etahen-11.00-13.60/releases/tag/v2.5B-13.60-r3) and experimental releases remain available for rollback.

Companion project: [PS Neighborhood](https://github.com/GronedWaffel/ps-neighborhood).

## License and credit

The original [GPLv3 LICENSE](LICENSE) is retained; third-party components retain their own notices. FPS integration credits OnionHEN/LightningMods, PHU Games Tools / ArkSama, and the PS5-Payload-dev foundation. See [CREDITS.md](CREDITS.md) for full attribution. This project is not affiliated with Sony Interactive Entertainment.
