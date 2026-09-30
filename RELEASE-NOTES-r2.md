# etaHEN 13.60 r2 + matched ShadowMountPlus

**Recommended downloads: `etaHEN-13.60.elf` + [ShadowMount-for-etaHEN-13.60.elf](https://github.com/GronedWaffel/etahen-13.60/releases/download/v2.5B-13.60-r2/ShadowMount-for-etaHEN-13.60.elf).** Load etaHEN first, then the recommended ShadowMount file once. It automatically waits for Toolbox initialization and checks for duplicate loads. This is the guarded loader used by [sniperscheats.lol](https://sniperscheats.lol/), containing our custom ShadowMount build for the bundled kstuff-lite.

## Fixed in this update

- Fixed the startup race that allowed optional payloads to start before etaHEN Toolbox finished initializing. The host now requires confirmation from the current etaHEN and ShellUI processes, rather than accepting an old log entry.
- Failed Toolbox initialization now stops the optional sequence instead of being reported as a successful startup.
- etaHEN and our ShadowMount build avoid incompatible legacy kstuff pause/resume writes on 13.60. Bundled kstuff-lite v1.11 stays active; the legacy automatic pause feature is disabled.

**Confirmed by our tester on PS5 13.60:** Toolbox works, and the previous freezes when shutting down, restarting or entering rest mode are resolved with the updated host bundle. All 9 etaHEN and 32 host/native regression tests passed. These results do not certify every etaHEN feature, mounted-game rest/wake behavior or other firmware.

## Downloads and loading order

1. `etaHEN-13.60.elf` — the complete updated etaHEN payload, including the Toolbox card and bundled kstuff-lite. Load once after a fresh boot/jailbreak.
2. **`ShadowMount-for-etaHEN-13.60.elf` — recommended.** Load once after starting our r2 etaHEN. It waits automatically until etaHEN is ready, then starts the custom ShadowMount. Do not also load another ShadowMount file or a separate kstuff.

<details>
<summary>Advanced: immediate-start version</summary>

`ShadowMount-13.60-manual-start.elf` runs the same custom ShadowMount immediately, without the loader's startup wait or duplicate checks. Use it only if you manage the loading order yourself: wait until Toolbox opens normally, then load it once. Use this file **instead of** the recommended loader, never alongside it.

</details>

**Already downloaded these files?** The recommended loader was previously called `optional-shadowmount-13.60.elf`. The manual-start version was `shadowmountplus-1.7beta2-snipers1360-r1.elf`. The ELF bytes are unchanged; the download names and instructions have been clarified.

The website already uses the matched builds and guarded loading order. Let its offline cache update before starting. Restart before replacing an already running payload.

## Source and credit

Updated etaHEN source is in this repository. `etahen-shadowmount-13.60-r2-source.zip` includes the modified etaHEN and ShadowMount sources, supervisor, build scripts, regression tests and original license notices. Upstream dependency source archives, scoped validation metadata and `SHA256SUMS.txt` are also attached. Pull requests, reproducible bug reports and community testing are welcome.

Original **etaHEN by LightningMods and the etaHEN contributors**. **ShadowMountPlus by Drakmor**, with upstream credits to VoidWhisper, Gezine, Earthonion, EchoStretch and community contributors. **kstuff-lite by EchoStretch and upstream kstuff contributors**, including sleirsgoevy. SDK/ELF loader work by John Törnblom and contributors; PS5Debug-NG by Pharaoh2k/OSR and contributors, building on CTN and SiSTR0. Full attribution is in [CREDITS.md](https://github.com/GronedWaffel/etahen-13.60/blob/main/CREDITS.md) and the retained upstream notices.

These are **unofficial integration builds maintained by GronedWaffel, for PS5 13.60 only**. Original authors retain their credits and licenses.
