# etaHEN 13.60 r2 + matched ShadowMountPlus

**If you use our `etaHEN-13.60.elf` with ShadowMount, we recommend using the `shadowmountplus-1.7beta2-snipers1360-r1.elf` provided in this release.** It is our custom integration build for the kstuff-lite bundled with this etaHEN port. Both ELFs are the exact builds used by the updated [sniperscheats.lol](https://sniperscheats.lol/) host.

## Fixed in this update

- Fixed the startup race that allowed optional payloads to start before etaHEN Toolbox finished initializing. The host now requires confirmation from the current etaHEN and ShellUI processes, rather than accepting an old log entry.
- Failed Toolbox initialization now stops the optional sequence instead of being reported as a successful startup.
- etaHEN and our ShadowMount build avoid incompatible legacy kstuff pause/resume writes on 13.60. Bundled kstuff-lite v1.11 stays active; the legacy automatic pause feature is disabled.

**Confirmed by our tester on PS5 13.60:** Toolbox works, and the previous freezes when shutting down, restarting or entering rest mode are resolved with the updated host bundle. All 9 etaHEN and 32 host/native regression tests passed. These results do not certify every etaHEN feature, mounted-game rest/wake behavior or other firmware.

## Downloads and loading order

1. `etaHEN-13.60.elf` — the complete updated etaHEN payload, including the Toolbox card and bundled kstuff-lite. Load once after a fresh boot/jailbreak.
2. `shadowmountplus-1.7beta2-snipers1360-r1.elf` — the recommended direct ShadowMount ELF for this etaHEN build. **Wait until Toolbox opens normally before loading it once.** Do not also load a separate kstuff or another ShadowMount copy.
3. `optional-shadowmount-13.60.elf` — an alternative for automated hosts. It contains the same custom ShadowMount and waits for the updated etaHEN's startup acknowledgement, with duplicate/conflict checks. Use this **instead of** the direct ShadowMount ELF, not in addition to it. It requires this r2 etaHEN.

The website already uses the matched builds and guarded loading order. Let its offline cache update before starting. Restart before replacing an already running payload.

## Source and credit

Updated etaHEN source is in this repository. `etahen-shadowmount-13.60-r2-source.zip` includes the modified etaHEN and ShadowMount sources, supervisor, build scripts, regression tests and original license notices. Upstream dependency source archives, scoped validation metadata and `SHA256SUMS.txt` are also attached. Pull requests, reproducible bug reports and community testing are welcome.

Original **etaHEN by LightningMods and the etaHEN contributors**. **ShadowMountPlus by Drakmor**, with upstream credits to VoidWhisper, Gezine, Earthonion, EchoStretch and community contributors. **kstuff-lite by EchoStretch and upstream kstuff contributors**, including sleirsgoevy. SDK/ELF loader work by John Törnblom and contributors; PS5Debug-NG by Pharaoh2k/OSR and contributors, building on CTN and SiSTR0. Full attribution is in [CREDITS.md](https://github.com/GronedWaffel/etahen-13.60/blob/main/CREDITS.md) and the retained upstream notices.

These are **unofficial integration builds maintained by GronedWaffel, for PS5 13.60 only**. Original authors retain their credits and licenses.
