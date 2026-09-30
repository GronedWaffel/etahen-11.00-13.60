# Building the 13.60 port

The tested build used Windows x64, Node.js 24, Zig 0.14.1 and the [PS5 payload SDK v0.43](https://github.com/ps5-payload-dev/sdk/releases/tag/v0.43). It uses Zig's Clang/LLD with the PS5 SDK headers, runtime and linker script; Linux libc is not linked.

## External inputs

This source repository excludes compilers, SDKs, static libraries and upstream prebuilt payloads. Obtain the inputs from their original projects, preserve their licenses, and compare against `BUILD-INPUTS.json` when reproducing the tested build:

- `Source Code/bootstrapper/assets/kstuff.elf`: [EchoStretch/kstuff-lite v1.11](https://github.com/EchoStretch/kstuff-lite/releases/tag/v1.11).
- `Source Code/daemon/assets/ps5debug.elf`: [Pharaoh2k/ps5debug-NG 1.3.2](https://github.com/Pharaoh2k/ps5debug-NG/releases).
- `Source Code/daemon/assets/ps5-app-dumper.elf`: [EchoStretch/ps5-app-dumper v1.11](https://github.com/EchoStretch/ps5-app-dumper/releases).
- `Source Code/bootstrapper/assets/fps.prx`: the original [etaHEN 2.5B](https://github.com/etaHEN/etaHEN/releases/tag/2.5B) source tree's asset.
- `Source Code/lib/`: matching PS5 builds of `libz.a`, `libcurl.a`, `libwolfssl.a`, `libminizip.a`, `libmicrohttpd.a`, `libpsl.a`, and `libzstd.a` from the upstream etaHEN build inputs. Host Windows/Linux archives cannot substitute for these.
- `dependencies/lib/`: PS5 SDK homebrew builds of `libcrypto.a`, `libssl.a` and `libsqlite3.a`. The tested OpenSSL build was 3.5.2. The [PS5 payload SDK packages](https://github.com/ps5-payload-dev) provide the PS5 cross-build ecosystem; the manifest records the exact archives used locally.

The generated ShellUI, FPS service, daemon, utility and card installer are rebuilt from this repository. Do not supply stale generated ELFs. `libNidResolver` source is already vendored; no submodule initialization is required.

## Commands (PowerShell)

```powershell
$env:ZIG = 'C:\path\to\zig.exe'
$env:PS5_PAYLOAD_SDK = 'C:\path\to\ps5-payload-sdk'
node scripts/check-build-inputs.mjs
node scripts/build-all.mjs
```

Output: `build/etaHEN-13.60-experimental.elf` and `build/manifest.json`. The full build compiles the ABI probe, runs the regression tests, builds the component source and embeds the card installer and upstream assets. `node --test tests/*.test.mjs` runs the checks separately. Diagnostic helpers in `port/` are developer tools and are not a second payload to load during normal startup.

The local full build and user-tested ELF are recorded in `PORT-STATUS.md`. A clean online dependency provisioning workflow and bit-for-bit reproducibility across different toolchain installations have not been established; the manifest is provided so missing or different inputs are explicit. Before distributing linked binaries, supply the corresponding sources and license notices for their upstream components too.
