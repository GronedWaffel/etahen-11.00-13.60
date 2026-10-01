# etaHEN 13.60 r3 — cheat activation repair

This update repairs the cheat module lookup on PS5 13.60. The legacy dynlib syscall path failed to find the running game, so cheat activation stopped before applying any patch. The utility now resolves modules and their loaded bases through PS5 SDK v0.43's kernel dynlib helpers. The shared ptrace path also applies the required capabilities.

JSON patches now default to module-relative addresses. Their previously uninitialized `absolute` flag could produce incorrect patch destinations. The cheat index upper bound and negative JSON section check are corrected.

**Download/Update Cheats** now performs the requested download and refreshes indexes from the files on disk. A stale commit marker no longer prevents repairing missing files or an index replaced by the other repository.

## Loading

1. Load `etaHEN-13.60.elf` once after a fresh boot and jailbreak. Restart before replacing an already running etaHEN build.
2. **Standalone ShadowMount (manual loading) is recommended for this build.** The matched ShadowMount ELF is unchanged from r2. Wait until Toolbox opens normally, then load it once. Do not load a second ShadowMount copy or separate kstuff.

Website users should let the host manage optional-payload loading. The direct standalone ShadowMount asset is for manual loading.

## Manually copied cheats

Put JSON, SHN or MC4 files in `/data/etaHEN/cheats/json`, `/data/etaHEN/cheats/shn` or `/data/etaHEN/cheats/mc4`, respectively. Use **Cache and reload Cheats list** after copying. The filename's title ID and version must match the running game exactly; a PS4 cheat is not interchangeable with a native PS5 cheat. This update cannot make offsets from another game update valid.

## Validation

- A read-only probe reproduced the old module resolver failure on 13.60 and confirmed the SDK resolver's correct game base.
- The real corrected cheat parser and toggle implementation enabled and disabled JSON and SHN/XML patches on a marked diagnostic scratch page in a running game. Both the enabled bytes and restored bytes were verified. No executable bytes, progress or save data were modified by this diagnostic.
- Ten native/build regression tests passed, including a new poisoned-memory test for the address flag. The complete ELF builds with the corrected utility embedded.
- Fresh-boot Toolbox/Cheats UI verification is pending. The encrypted MC4 input stage and the reporter's particular cheat files have not been retested; MC4 shares the tested XML/toggle path after decryption.

Source and build scripts accompany the release. Please include title ID, exact game version, file format and the utility log when reporting a cheat failure. Community fixes and pull requests are welcome.

Original **etaHEN by LightningMods and the etaHEN contributors**. SDK/kernel helpers by **John Törnblom and contributors**. ShadowMountPlus by **Drakmor and contributors**; bundled kstuff-lite by **EchoStretch and upstream kstuff contributors**. This is an unofficial GronedWaffel integration build for **PS5 13.60 only**. See `CREDITS.md` and retained upstream license notices.
