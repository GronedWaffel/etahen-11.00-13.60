# Credits and provenance

This is an **unofficial port of etaHEN 2.5B**, originally developed by **LightningMods and the etaHEN contributors**. It is not an official etaHEN release. Original code, attribution and licenses remain with their authors. GronedWaffel maintains this 13.60 port; the original project is [etaHEN/etaHEN](https://github.com/etaHEN/etaHEN).

The upstream README is preserved in [UPSTREAM-README.md](UPSTREAM-README.md), including its contributor/tester acknowledgements. In particular: John Törnblom / PS5-Payload-dev, Buzzer, sleirsgoevy, ChendoChap, astrelsky, illusion, CTN, SiSTR0, and Nomadic. Upstream testers include Echo Stretch, idlesauce, Dizz, BedroZen and MODDED WARFARE.

## Build and payload dependencies

- [PS5 payload SDK v0.43](https://github.com/ps5-payload-dev/sdk/tree/v0.43), John Törnblom and contributors: runtime/linker support under GPL-3.0-or-later; individual FreeBSD headers and other components retain their notices.
- [Zig 0.14.1](https://ziglang.org/download/0.14.1/): compiler and linker toolchain, distributed separately.
- [kstuff-lite v1.11](https://github.com/EchoStretch/kstuff-lite): bundled runtime input in the tested build, by Echo Stretch and upstream kstuff contributors, including sleirsgoevy. Obtain its source and license from its upstream release/tree.
- [PS5Debug-NG 1.3.2](https://github.com/Pharaoh2k/ps5debug-NG): Services payload, building on PS5Debug by CTN and SiSTR0 and the PS4Debug lineage. Retains its upstream license.
- [PS5 app dumper v1.11](https://github.com/EchoStretch/ps5-app-dumper): upstream daemon dependency. Retains its upstream license.
- Native PS5 FPS sampling is adapted from OnionHEN; the PS4 counter is rebuilt from source. The unused legacy `fps.prx` is no longer embedded.
- [libNidResolver](https://github.com/astrelsky/libNidResolver), astrelsky: vendored source retains its LICENSE.
- Vendored tiny-json, cJSON, pugixml and other source/header libraries retain their in-file or directory license notices.
- Linked third-party libraries include zlib, curl, wolfSSL, minizip, libmicrohttpd, libpsl, Zstandard, OpenSSL and SQLite. Their static archives are external build inputs, not committed project source. Preserve their individual licenses when distributing a linked binary.

## Implementation references

- [OnionHEN](https://github.com/aydencharles/onionHEN): newer-firmware legacy Settings navigation reference.
- [PS5 File Explorer](https://github.com/juma-sayeh/PS5-File-Explorer): application-card metadata and installation reference.
- [ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus): newer application registration API reference.
- The optional `ShadowMountPlus 1.7beta2-snipers1360-r1` release asset is an unofficial integration build of Drakmor's ShadowMountPlus, with upstream contributions and credits to VoidWhisper, Gezine, Earthonion, EchoStretch and the community. Its GPL license and original notices are preserved. Our changes disable incompatible legacy kstuff pause/resume writes on 13.60. The attached combined source archive includes the modified source, build script, supervisor and tests; it is not presented as an upstream ShadowMountPlus release.
- [ps5-payload-dev/elfldr](https://github.com/ps5-payload-dev/elfldr) and [ftpsrv](https://github.com/ps5-payload-dev/ftpsrv): ELF loading, runtime and memory access references.

The project retains etaHEN's [GPLv3 LICENSE](LICENSE). Third-party components are governed by their own notices. No ownership of upstream projects is claimed, and this port is not affiliated with Sony Interactive Entertainment.

## Unified FPS integration (development candidate)

The native PS5 sampler under `Source Code/fps_native` is adapted from [OnionHEN](https://github.com/aydencharles/onionHEN/tree/b23ffe674b2de9f62fe634944c9230ff149d593a), commit `b23ffe674b2de9f62fe634944c9230ff149d593a`, by LightningMods and the OnionHEN contributors, licensed under GPL-3.0-or-later. Upstream credits **PHU Games Tools / ArkSama** for the FPS research and **John Törnblom / PS5-Payload-dev** for SDK and memory-access foundations. Original in-file notices are preserved.

Local changes isolate sampling in a separate payload process; gate it on etaHEN readiness and the FPS setting; select native game titles; bound diagnostics; validate ring sizes, translations and fresh samples; and repair the shared-sample sequence publication. The PS4 counter uses etaHEN's relocated, stopped-process hook publication, counts GNM flip submission calls and publishes outside the render thread. These counters need per-game hardware validation; source attribution does not imply upstream endorsement or verified compatibility.

## etaHEN plugin contract

Plugin packaging and daemon lifecycle follow the official [etaHEN-Plugins SDK](https://github.com/etaHEN/etaHEN-Plugins/tree/6339554e4e3c92c0a655a37194c1152d6b82ad88), by LightningMods and contributors (GPLv3). The reference is `lib/make_plugin.py`, the README, and the utility/Game_Plugin_Loader samples. The two local fixture containers match the official packager byte for byte. Our game-targeted directory scheduler is a separate extension; it does not claim universal game mod, SPRX, or OnionHEN SDK compatibility.

The dev5 PS4 FPS counter follows OnionHEN / LightningMods' external GNM-counter approach and tagged export-NID lookup from `source/libonion_fps/source/fps_bc.cpp`, commit `b23ffe674b2de9f62fe634944c9230ff149d593a` (GPL-3.0). This port uses its own allocated trampoline, instruction relocation, flags-preserving atomic count, stopped-thread inspection, and verified COW publication instead of copying instruction bytes into library padding. No in-game ELF or pthread is needed for this counter.

Dev6 also follows that upstream PS4 entry-permission requirement: the inline-pointer jump stays readable/executable. Our publication additionally verifies the installed protection and retains original protection for rollback.


Dev7 native PRX loading follows the standard `sceKernelLoadStartModule` contract also used in etaHEN's existing GameStuff/HookGame reference and the [GoldHEN plugin loader](https://github.com/GoldHEN/GoldHEN_Plugins_Repository/blob/main/plugin_src/plugin_loader/source/main.c). The new queue, validation, relocated context-preserving wrapper and bounded diagnostics are local implementations; no GoldHEN compatibility layer or endorsement is claimed. VideoOut export names are independently listed in the [PS5 payload SDK](https://github.com/ps5-payload-dev/sdk/blob/master/sce_stubs/libSceVideoOut.c); the GTA test limiter uses those runtime exports instead of game offsets.
