# etaHEN 13.60 — unofficial source port

**PS5 13.60 port of etaHEN 2.5B, originally created by LightningMods and the etaHEN contributors.** Maintained here by GronedWaffel. This is an independent, experimental port, not an official etaHEN release. Please credit and support the [original etaHEN project](https://github.com/etaHEN/etaHEN).

The source for the port, bootstrap, services, ShellUI changes, automatic Toolbox card installer and regression tests is included. [Credits](CREDITS.md) · [Build instructions](BUILDING.md) · [Hardware validation](PORT-STATUS.md) · [Contributing](CONTRIBUTING.md) · [Original README](UPSTREAM-README.md)

## What changed for 13.60

- Updated the runtime, injector and managed/native hooks for 13.60. Native hooks use process-private copy-on-write publication to avoid altering another process's shared code pages.
- Added an **etaHEN Toolbox home-screen card**, automatically installed by the main ELF and restored if deleted. It opens the working legacy Settings route; the normal Sony Debug Settings menu remains available.
- Added **Start PS5Debug-NG** under Services, with duplicate-load protection. The bundled input is PS5Debug-NG 1.3.2. It starts on demand rather than automatically.
- Removed the Homebrew Store and PS5 webMAN Games buttons from the Toolbox main menu.
- Fixed URL-loaded payloads incorrectly detecting themselves as an old running etaHEN instance.
- Added Node/Zig build scripts, ELF checks, ABI and embedding regression tests, and component diagnostics.

## Use and validation

This port targets **PS5 13.60 only** and requires a compatible jailbreak and ELF loader. The integrated host is [sniperscheats.lol](https://sniperscheats.lol/). Load one full payload after a fresh boot/jailbreak; do not stack it over an existing etaHEN instance.

The user confirmed the corrected website flow, fresh startup, automatic card installation, Toolbox navigation and PS5Debug Services action. FTP, service startup and guarded hook installation also have scoped hardware checks. This does **not** certify every original etaHEN feature or other firmware. The [validation record](PORT-STATUS.md) distinguishes successful checks from earlier diagnostic failures; referenced raw console artifacts are intentionally not published.

The tested main ELF is `etaHEN-13.60-experimental.elf` (30,415,336 bytes), SHA-256:

```
2d43efc4111991772bfc4d331ebfd6a88395efa686da3114ba8406323b281fbe
```

Card installation uses the newer firmware's application registration scan, which may also register other already-staged application folders. An ownership receipt and asset comparison skip repeat registration when the card is current. See [Toolbox card details](TOOLBOX-CARD.md).

## Build and contribute

See [BUILDING.md](BUILDING.md) for Node.js 24, Zig 0.14.1, PS5 payload SDK v0.43 and external build inputs. This repository contains project source; SDKs, third-party static archives and bundled upstream payload binaries must be obtained separately. Their tested hashes are recorded in [BUILD-INPUTS.json](BUILD-INPUTS.json).

**Pull requests and community testing are welcome.** Improvements, clear bug reports, reproducible builds and separately verified firmware ports all help. Include exact firmware, tools and observed results; please retain upstream authorship and licensing.

Companion desktop project: [PS Neighbourhood](https://github.com/GronedWaffel/ps-neighbourhood), with PS5 memory/MCP tools, FTP, package installation, console management and saves.

## License and credit

etaHEN's [GPLv3 license](LICENSE) is retained. Third-party components keep their own licenses. See [CREDITS.md](CREDITS.md) and the original in-file notices for attribution. This project is not affiliated with Sony Interactive Entertainment or presented as an official etaHEN release.
