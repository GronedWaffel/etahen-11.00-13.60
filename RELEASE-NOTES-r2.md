# etaHEN 13.60 r2 + matched ShadowMountPlus

**Recommended for this etaHEN 13.60 r2 build: [Standalone ShadowMount (manual loading)](https://github.com/GronedWaffel/etahen-13.60/releases/download/v2.5B-13.60-r2/Standalone-ShadowMount-manual-loading.elf).** It is our custom ShadowMount build matched to the kstuff-lite bundled with this etaHEN. Load etaHEN first, wait until Toolbox opens normally, then load Standalone ShadowMount once.

## Fixed in this update

- Fixed the startup race that allowed optional payloads to start before etaHEN Toolbox finished initializing. The host now requires confirmation from the current etaHEN and ShellUI processes, rather than accepting an old log entry.
- Failed Toolbox initialization now stops the optional sequence instead of being reported as a successful startup.
- etaHEN and our ShadowMount build avoid incompatible legacy kstuff pause/resume writes on 13.60. Bundled kstuff-lite v1.11 stays active; the legacy automatic pause feature is disabled.

**Confirmed by our tester on PS5 13.60:** Toolbox works, and the previous freezes when shutting down, restarting or entering rest mode are resolved with the updated host bundle. All 9 etaHEN and 32 host/native regression tests passed. These results do not certify every etaHEN feature, mounted-game rest/wake behavior or other firmware.

## Downloads and loading order

1. `etaHEN-13.60.elf` — the complete updated etaHEN payload, including the Toolbox card and bundled kstuff-lite. Load once after a fresh boot/jailbreak.
2. **`Standalone-ShadowMount-manual-loading.elf` — recommended for this etaHEN 13.60 r2 build.** This starts ShadowMount immediately. **Wait until Toolbox opens normally before loading it once.** Do not also load another ShadowMount copy or a separate kstuff.

The website already uses the matched builds and guarded loading order. Let its offline cache update before starting. Restart before replacing an already running payload.

## Source and credit

Updated etaHEN source is in this repository. `etahen-shadowmount-13.60-r2-source.zip` includes the modified etaHEN and ShadowMount sources, supervisor, build scripts, regression tests and original license notices. Upstream dependency source archives, scoped validation metadata and `SHA256SUMS.txt` are also attached. Pull requests, reproducible bug reports and community testing are welcome.

Original **etaHEN by LightningMods and the etaHEN contributors**. **ShadowMountPlus by Drakmor**, with upstream credits to VoidWhisper, Gezine, Earthonion, EchoStretch and community contributors. **kstuff-lite by EchoStretch and upstream kstuff contributors**, including sleirsgoevy. SDK/ELF loader work by John Törnblom and contributors; PS5Debug-NG by Pharaoh2k/OSR and contributors, building on CTN and SiSTR0. Full attribution is in [CREDITS.md](https://github.com/GronedWaffel/etahen-13.60/blob/main/CREDITS.md) and the retained upstream notices.

These are **unofficial integration builds maintained by GronedWaffel, for PS5 13.60 only**. Original authors retain their credits and licenses.
