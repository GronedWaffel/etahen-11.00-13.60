# Unified development candidate — 2026-10-05 (dev5 overlay initialization and external PS4 counter)

This is a local test build, not a public release or website deployment. The controller-startup correction was previously confirmed on 13.60 and 12.40. The combined card/plugin/FPS changes have passed host checks and compilation but are not yet hardware validated.

## Firmware and configuration

Exact profiles: 11.00, 11.20, 11.60, 12.00, 12.02, 12.20, 12.40, 12.60, 12.70, 13.00, 13.20, 13.40, 13.42, 13.60. Below 11.00 and 11.40 are rejected. Prior community startup results are not certification of these new features.

This candidate retains `config-multifw-experimental.ini` and experimental diagnostic paths to keep testing isolated. Load the main candidate once after a fresh jailbreak, without an existing etaHEN instance. Merely copying its ELF onto the console does not activate it or replace the YouTube bundle.

## Plugins

The etaHEN SDK format is a 29-byte header followed by a PS5 payload SDK ELF: `etaHEN_PLUGIN` plus NUL (14 bytes), plugin title ID plus NUL (10 bytes), and x.xx version plus NUL (5 bytes). This is a real `.plugin` container, not a renamed ELF. The original SDK runs plugins as background daemons; the daemon's `main()` owns its lifecycle and any game-specific work. No universal `plugin_start`/`plugin_stop` ABI is defined by this SDK.

Place normal `.plugin` files in `/data/etaHEN/plugins/` or `/mnt/usbN/etaHEN/plugins/`. USB has priority. The loader validates metadata and the embedded ELF, deduplicates by plugin identity, spawns only the ELF body with that identity and maintains a PID receipt. Toolbox Start/Stop addresses that plugin process. Plain `.elf` payload support is retained and explicitly labelled; it does not establish plugin compatibility.

Optional game-targeted folders are our scheduling extension: `/data/etaHEN/game_plugins/<TITLEID>/*.plugin`. The matching game must be running to start one there. This starts its own daemon; it does not inject the `.plugin` container into the game. The plugin itself must validate the intended game/version and implement any modifications. Stop kills its daemon; a plugin that patches another process must own safe cleanup. No generic unload guarantee is made for arbitrary mods. SDK compatibility does not fix offsets inside old third-party plugins.

Automatic startup still uses a sibling `<filename>.auto_start` marker controlled by Toolbox. Leave fixtures manual until start/stop works. Automatic game-start attempts are bounded to once per session; manual Start/Stop can retry.

`system-lifecycle-test.plugin` has ID `SNPS00001`, version `1.00`, displays initialization and stays alive. `game-lifecycle-test.plugin` has ID `SNPG00001`, version `1.00`, runs as its own daemon and reports game changes. Both log a heartbeat every ten seconds under `/data/etaHEN/plugin-tests/<ID>.log`. Neither modifies game memory nor limits FPS. Both output containers were compared byte for byte with the official SDK packager.

## Hardware test sequence

1. Fresh jailbreak, load the main candidate, and verify normal Toolbox entry/exit and controller input.
2. Check whether the Toolbox card is left of Store after the normal dashboard update. This candidate transactionally changes only its own registered card's sort priority; it does not force a dashboard reload. Original priority is saved in `/data/etaHEN/toolbox-card-order-original.json`. Placement was confirmed by the user; click routing is under investigation.
3. Open Plugins, start the system fixture, confirm its notification, and stop it. A repeated start should not create another copy.
4. Start the game-aware `.plugin` from Toolbox, then launch games. Confirm its game-detected notifications and heartbeat log. Stop it from Toolbox and confirm its PID disappears. This tests daemon lifecycle/game detection, not game-code injection.
5. Dev1 and dev3 failed the GTA V FPS test. Keep FPS off for card/plugin checks. On a fresh dev4 session, explicitly enable FPS for the separate GTA V test described below; record whether a numeric value appears and whether ShellUI remains responsive.

Native PS5 sampling follows OnionHEN's render/scanout estimation. The PS4 counter counts GNM submit/flip calls. Some games may use other paths or multiple submissions, so neither is yet certified for every game. A deliberate 15 FPS test limiter is a later fixture once the relevant game's frame hook has been verified.

## Diagnostics and rollback

Retain startup experimental diagnostics, `fps.log`, and the fixture log before reboot when possible. Do not stack the candidate over a running etaHEN. Restore the previous main ELF for the next fresh boot to roll back code; no website or YouTube image is changed by staging this candidate. The card priority is persistent and has its own saved original value. Test fixture auto-start markers are not enabled by default.

See CREDITS.md for etaHEN, OnionHEN, PHU Games Tools / ArkSama, SDK and dependency attribution.

## Dev1 hardware regressions under investigation

The user confirmed the Toolbox tile appeared left of Store, but could not launch it. The user clarified that clicking the card makes a click sound but shows no error. Two CE-105773-3 entries in error history cannot therefore be attributed to that click. The tile's deep link remains in app.db and its priority is 5 versus Store 6. Later CE-108262-9 coincides with ShellUI restarting from PID 58 to 98. Existing logs do not identify the faulting instruction or prove that GTA itself crashed. Captures are private under artifacts; no public release is authorized by a successful compile alone. Dev3 is a private test candidate; public release remains held pending card/overlay hardware results.


### Dev3 local candidate corrections

The registered `pshome:gamehub?titleId=ETHN13600` route now redirects to Toolbox, in addition to its existing direct deep link. Routing tests cover both Boot ABIs and reject unrelated game IDs. Whether the pinned tile actually dispatches that route needs a console check; clicking the tile has not yet been proved fixed.

FPS widget creation/removal is deferred from settings handlers to the render callback. Existing named widgets are reused rather than duplicated. Overlay access resolves the current Game scene with checked managed invocation instead of keeping a scene pointer for FPS. FPS text exceptions disable sampling for the session, and UI phases are recorded in the existing journal. CPU discovery only runs when a CPU overlay needs it. These address concrete code defects and improve fault isolation; they are not proof of the cause of the dev1 ShellUI restart. Console FPS remains off for the first card/plugin test.


### Dev3 hardware result / Dev4 correction

User confirmed the pinned Toolbox card opens. Enabling FPS and starting GTA V still produced CE-108262-9. The captured dev3 journal completes sample reads until sequence 685, then stops at `FPS render reading sample`, before scene lookup/widget creation; ShellUI PID 58 subsequently becomes 97. Three Toolbox route requests returned successfully before that restart. Card registration files survive, but ShellUI hooks do not, explaining why the tile no longer opens afterward. Re-sending the payload was correctly rejected because resident etaHEN services remained.

The stalled render function performed synchronous app-service queries and sample-file mapping. This evidence localizes the freeze to that function, but does not identify which individual call blocked. Dev4 removes the whole blocking path from the UI thread: no app-service query is issued by FPS display code, and only a background worker reads bounded sample records. The renderer reads one lock-free fixed-point value with a monotonic expiry. Files are read twice and compared rather than mapped into ShellUI. A newer game record without a valid measurement clears the display instead of retaining an old game's FPS. Freshness is limited by the original sampler timestamp; a blocked reader cannot extend it.

Dev4 passes 16 host checks and requires a fresh console test. FPS remains disabled in the saved console configuration. First verify Toolbox still opens, then enable FPS and start GTA V. Confirm both absence of the system error and an actual numeric reading; a `--` display alone is not a working FPS measurement. Do not resend etaHEN over surviving services following a ShellUI crash; use a fresh boot/jailbreak. No public release, website artifact or YouTube image has been changed.


### Dev4 hardware result / Dev5 correction

The user reports GTA V no longer crashes, but no overlay appears; PS4 reports counter initialization failure. Dev4 telemetry contains valid GTA V readings around 30 FPS and a populated UI cache. The overlay root lookup returned null because the initializer declared a local `AppSystem_img`, shadowing the shared variable. Dev5 assigns the shared image, and a regression check covers that initialization wiring.

The PS4 ELF loader failed once loading its entry and once timing out in its remote pthread stager; neither run reached the counter initializer. Dev5 replaces that path with a daemon-owned GNM flip counter following OnionHEN's external-counter model. Only flip exports are accepted (including BC `#`-suffixed names); generic command submissions are not reported as frames. The counter preserves flags, relocates original instructions, installs while target threads are stopped, refuses occupied instruction ranges or existing entry jumps, uses verified COW entry writes, restores protection, and attempts verified rollback on failure. No counter ELF or pthread is started inside the game. A brief startup settling period precedes one installation attempt per game process; failures name their stage. This counts flip submissions, not proof of presented frames for every game.

Both PS5 overlay display and PS4 counter still require dev5 hardware validation. The card opening and dev4 crash-free GTA V result are user observations, not a blanket stability claim. No public release or website update is made by this private candidate.


### Dev5 hardware result / Dev6 PS4 correction

The user confirmed the PS5 FPS display works in WWE, GTA V and Spider-Man. The plugin lifecycle fixture detected the game and stopped when the game closed; it is not a 15 FPS limiter. These are user-reported observations, not certification of every game or third-party plugin.

Two Minecraft PS4 (CUSA00265) attempts installed the remote GNM counter and then failed with SYSTEM_XO_VIOLATION. ShadowMount's notification says "before kstuff auto-pause"; that describes crash timing and does not establish a kstuff failure. The saved crash summaries do not contain a faulting instruction address. Inspection found a matching defect: the entry's FF 25 jump reads an inline pointer, but dev5 restored the original protection, potentially execute-only. Dev6 retains read/execute without write on the private hooked page, checks the resulting protection, and rolls back bytes plus original protection if publication fails. Diagnostics record original/installed protection after detaching. PS5 sampling, UI routing and plugin behavior are unchanged.

All 16 host checks and the full build passed. The executable regression now enters through the actual patched entry before the trampoline, verifies the counter and preserved carry/return value, and covers XO/RX/RWX permission policy. Host execution is not a PS4 execute-only hardware test. Dev6 still needs a fresh console test: load it after a fresh jailbreak without an existing etaHEN instance, enable FPS and launch Minecraft. Verify a numeric counter, normal gameplay and that Toolbox remains accessible. No public release or website changes.
