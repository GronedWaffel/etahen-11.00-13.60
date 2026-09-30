# etaHEN Toolbox home-screen card (13.60)

The main etaHEN ELF automatically installs a separate Games-category deep link named **etaHEN Toolbox**, title ID `ETHN13600`. It opens the legacy Toolbox route instead of Sony's newer native Debug Settings page. It needs etaHEN's Toolbox hooks running after each jailbreak. Selecting the card does not reload etaHEN, kstuff or a debugger.

Build everything with `node scripts/build-all.mjs`, or just the card tools with `node scripts/build-toolbox-card.mjs`.

- `build/etaHEN-13.60-experimental.elf` embeds and starts the card installer automatically after spawning the main services. Users need only this ELF.
- `build/toolbox-card-install.elf` is the separately usable installer embedded in the main ELF. It stages its icon and metadata under `/user/app/ETHN13600`, then registers the card.
- `build/toolbox-card-remove.elf` removes only this title after verifying its ownership marker. Removal and reinstallation were hardware-tested. Loading the main ELF again recreates a removed card.
- `build/toolbox-launch.elf` opens the legacy route directly without installing a card.

On 13.60 the single-title registration export is absent. Installation therefore uses the system's application registration scan, which can also register other already-staged app folders. This user explicitly approved that behavior and requested automatic installation from the main ELF. A successful registration receipt plus matching metadata/artwork cause later startups to skip the scan. No direct application-database writes are used. Existing content at the title path without our ownership marker causes a refusal.

The registration scan returned success and the card's name/link were verified in a read-only copy of the catalog. The user confirmed the card appears and opens Toolbox. Removal deleted only its title and owned files; reinstall restored it. The embedded-helper spawn path passed and correctly skipped an already-installed card. The full ELF also passed the fresh-boot test: the user deleted the card, and bundled helper PID 95 recreated and registered it with result 0. The catalog and user independently confirmed the restored card. Evidence: `artifacts/services-freshboot-toolbox-card-install.json` and `artifacts/services-freshboot-card-catalog.json`. Exact ordering beside the Store is controlled by ShellUI and is not confirmed.

The full payload handles `etahen_root=1` by clearing stale Cheats/Games shortcut state before opening Toolbox. This route was used by the automatically restored card. Load a new full payload only after a fresh boot/jailbreak; do not stack it onto a running etaHEN instance.

Implementation references (original projects retain their licenses):

- [OnionHEN's newer-firmware legacy navigation route](https://github.com/aydencharles/onionHEN/blob/main/source/shellui/src/hook_navigator.cpp)
- [PS5 File Explorer tile metadata and registration](https://github.com/juma-sayeh/PS5-File-Explorer)
- [ShadowMountPlus registration on newer firmware](https://github.com/drakmor/ShadowMountPlus/blob/main/src/sm_install.c)
- [ShadowMountPlus registration scan](https://github.com/drakmor/ShadowMountPlus/blob/main/src/sm_install_queue.c)

The card code is GPL-3.0-or-later; its icon is the supplied etaHEN project's existing artwork. A catalog entry or successful build alone does not establish complete etaHEN feature compatibility.
