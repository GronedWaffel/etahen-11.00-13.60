# Unified development candidate — 2026-10-05

This is a local test build, not a public release or website deployment. The controller-startup correction was previously confirmed on 13.60 and 12.40. The combined card/plugin/FPS changes have passed host checks and compilation but are not yet hardware validated.

## Firmware and configuration

Exact profiles: 11.00, 11.20, 11.60, 12.00, 12.02, 12.20, 12.40, 12.60, 12.70, 13.00, 13.20, 13.40, 13.42, 13.60. Below 11.00 and 11.40 are rejected. Prior community startup results are not certification of these new features.

This candidate retains `config-multifw-experimental.ini` and experimental diagnostic paths to keep testing isolated. Load the main candidate once after a fresh jailbreak, without an existing etaHEN instance. Merely copying its ELF onto the console does not activate it or replace the YouTube bundle.

## Plugins

System plugins run in their own process. Supported containers are PS5 payload SDK ELF files and etaHEN's legacy `.plugin` wrapper. Internal files belong in `/data/etaHEN/plugins/`; the existing payload and USB directories remain supported. Start/stop controls use a verified PID receipt; a plugin that renames its process is not guaranteed to support those controls.

Game plugins are ELF payloads written to execute inside the target game and built for the console payload ABI. Put them in `/data/etaHEN/game_plugins/<TITLEID>/`. Toolbox lists a load button for each game ELF. It checks that the matching title is running and allows only one attempt per file per game session. Close the game to unload; there is no unsafe hot-unload. Arbitrary Windows DLLs, PS4/PS5 SPRX files and another loader's plugin SDK are not interchangeable with this format.

Automatic loading uses a sibling `<filename>.auto_start` marker controlled by the Plugin Startup menu. Game markers apply only when that title runs. Do not enable them until manual loading works.

`system-lifecycle-test.elf` displays its process identity and stays alive for start/stop testing. `game-lifecycle-test.elf` displays its actual game title and returns from initialization. It does not patch the game or limit FPS. Its log is `/data/etaHEN/plugin-tests/lifecycle.log` where accessible; game sandbox restrictions may prevent that file write, so the on-screen notification is also evidence.

## Hardware test sequence

1. Fresh jailbreak, load the main candidate, and verify normal Toolbox entry/exit and controller input.
2. Check whether the Toolbox card is left of Store after the normal dashboard update. This candidate transactionally changes only its own registered card's sort priority; it does not force a dashboard reload. Original priority is saved in `/data/etaHEN/toolbox-card-order-original.json`. Physical placement remains unverified.
3. Open Plugins, start the system fixture, confirm its notification, and stop it. A repeated start should not create another copy.
4. Launch one game, then select its game fixture from Toolbox. Verify the displayed title and that gameplay continues. Repeat for each test game after closing the previous game.
5. Enable Game FPS (PS4 / PS5) before starting the PS4 game. Check the overlay during gameplay, menus and pause. Repeat for the PS5 games and turn the setting off again. Missing/stale samples display `--`; a displayed number still needs comparison against known frame-rate modes.

Native PS5 sampling follows OnionHEN's render/scanout estimation. The PS4 counter counts successful GNM submit/flip calls. Some games may use other paths or multiple submissions, so neither is yet certified for every game. A deliberate 15 FPS test limiter is a later fixture once the relevant game's frame hook has been verified.

## Diagnostics and rollback

Retain startup experimental diagnostics, `fps.log`, and the fixture log before reboot when possible. Do not stack the candidate over a running etaHEN. Restore the previous main ELF for the next fresh boot to roll back code; no website or YouTube image is changed by staging this candidate. The card priority is persistent and has its own saved original value. Test fixture auto-start markers are not enabled by default.

See CREDITS.md for etaHEN, OnionHEN, PHU Games Tools / ArkSama, SDK and dependency attribution.
