# Credits and provenance

This is an **unofficial port of etaHEN 2.5B**, originally developed by **LightningMods and the etaHEN contributors**. It is not an official etaHEN release. Original code, attribution and licenses remain with their authors. GronedWaffel maintains this 13.60 port; the original project is [etaHEN/etaHEN](https://github.com/etaHEN/etaHEN).

The upstream README is preserved in [UPSTREAM-README.md](UPSTREAM-README.md), including its contributor/tester acknowledgements. In particular: John Törnblom / PS5-Payload-dev, Buzzer, sleirsgoevy, ChendoChap, astrelsky, illusion, CTN, SiSTR0, and Nomadic. Upstream testers include Echo Stretch, idlesauce, Dizz, BedroZen and MODDED WARFARE.

## Build and payload dependencies

- [PS5 payload SDK v0.43](https://github.com/ps5-payload-dev/sdk/tree/v0.43), John Törnblom and contributors: runtime/linker support under GPL-3.0-or-later; individual FreeBSD headers and other components retain their notices.
- [Zig 0.14.1](https://ziglang.org/download/0.14.1/): compiler and linker toolchain, distributed separately.
- [kstuff-lite v1.11](https://github.com/EchoStretch/kstuff-lite): bundled runtime input in the tested build, by Echo Stretch and upstream kstuff contributors, including sleirsgoevy. Obtain its source and license from its upstream release/tree.
- [PS5Debug-NG 1.3.2](https://github.com/Pharaoh2k/ps5debug-NG): Services payload, building on PS5Debug by CTN and SiSTR0 and the PS4Debug lineage. Retains its upstream license.
- [PS5 app dumper v1.11](https://github.com/EchoStretch/ps5-app-dumper): upstream daemon dependency. Retains its upstream license.
- The upstream etaHEN `fps.prx` asset remains an external build input. It is not original work of this port.
- [libNidResolver](https://github.com/astrelsky/libNidResolver), astrelsky: vendored source retains its LICENSE.
- Vendored tiny-json, cJSON, pugixml and other source/header libraries retain their in-file or directory license notices.
- Linked third-party libraries include zlib, curl, wolfSSL, minizip, libmicrohttpd, libpsl, Zstandard, OpenSSL and SQLite. Their static archives are external build inputs, not committed project source. Preserve their individual licenses when distributing a linked binary.

## Implementation references

- [OnionHEN](https://github.com/aydencharles/onionHEN): newer-firmware legacy Settings navigation reference.
- [PS5 File Explorer](https://github.com/juma-sayeh/PS5-File-Explorer): application-card metadata and installation reference.
- [ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus): newer application registration API reference.
- [ps5-payload-dev/elfldr](https://github.com/ps5-payload-dev/elfldr) and [ftpsrv](https://github.com/ps5-payload-dev/ftpsrv): ELF loading, runtime and memory access references.

The project retains etaHEN's [GPLv3 LICENSE](LICENSE). Third-party components are governed by their own notices. No ownership of upstream projects is claimed, and this port is not affiliated with Sony Interactive Entertainment.
