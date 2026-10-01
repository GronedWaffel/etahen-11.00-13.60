# Unofficial etaHEN 13.60 port — development status

## Cheat repair candidate r3 (2026-10-01)

The old `get_module_handle` syscall path returned no `eboot.bin` for the live native PS5 game. In the same process, SDK `kernel_dynlib_handle` returned handle 0 and base `0x400000`. The cheat utility now uses that SDK path in the 13.60 build, with the shared capability-aware ptrace attach/detach helpers. JSON patch addressing is initialized deterministically; explicit downloads also repair local indexes instead of trusting an old commit marker.

The actual `CheatManager.cpp` parser/toggle code passed enable/disable checks for JSON and SHN/XML on a marked scratch allocation, with byte verification in both states. The standalone test does not replace the running service or patch gameplay code. The complete ELF builds; all ten native/build tests pass. Full boot/UI validation remains pending, as does reproduction using the issue reporter's exact files. See `port/cheat-engine-probe.cpp`, `port/cheat-module-probe.cpp` and their build scripts for the scoped diagnostics.

## Current r2: startup ordering and matched ShadowMount (2026-09-30)

The user tested the updated website bundle on PS5 13.60 and confirmed Toolbox
works again. They also reported that shutdown, restart and entering rest mode
no longer freeze the console. These are user-confirmed checks on their console;
they do not establish rest/wake behavior with mounted games or universal
compatibility of every menu action. The earlier entries below are historical.

Tested etaHEN: 30,416,848 bytes, SHA-256
`f1befb433ca8f3d838629b543e4481fa09a5fc7b56cf34c51cb2241c57c8951a`.
Matched ShadowMountPlus `1.7beta2-snipers1360-r1`: 2,438,024 bytes, SHA-256
`563f72b8857ae7f115c237acc618308d96651093187891679002d9a3b710bf0d`.
The tested host used its guarded supervisor, SHA-256
`77a27dd5a59a11dd4f20231501d603408403e9b266ebc5b5ef67c3ac7c6ea16f`.

Read-only diagnostics of the preceding failure showed a restarted ShellUI and
failed Toolbox initialization while ShadowMount remained running. The host
could accept yesterday's success log while the new daemon was still starting.
r2 replaces that check with a boot-local acknowledgement bound to both live
processes and does not emit startup success after failed injection. Both etaHEN
and custom ShadowMount also avoid legacy pointer-tag pause/resume writes with
the bundled INT3-based kstuff-lite. See
[the compatibility record](KSTUFF-SHADOWMOUNT-COMPATIBILITY.md).

All 9 etaHEN and 32 host/native regression tests passed. Hosted ELF and source
checksums were verified before the user's successful test. The generic build
manifest keeps `hardwareValidated: false` because it is not an all-feature
certification; the specific successful checks above are the validation record.

## URL-loader self-detection fix

The website successfully completed the jailbreak and reached etaHEN, but the bootstrap reported an existing instance even on a fresh boot. Unlike raw TCP loading (`payload.elf`), URL loading names the process after its filename (`etaHEN-13.60-….elf`). The bootstrap's substring lookup matched itself. The shared bootstrap process lookup now excludes its own PID while retaining matches for other etaHEN services/bootstraps. This also prevents the later legacy cleanup loop from targeting the bootstrap itself. The restart guard is retained and logs any matching **other** PID.

Regression checks cover self-named URL payloads, actual separate etaHEN processes, unrelated loader names and bounded/truncated process names. All nine local tests passed. Rebuilt ELF: 30,415,336 bytes, SHA-256 `2d43efc4111991772bfc4d331ebfd6a88395efa686da3114ba8406323b281fbe`. On 2026-09-29 the user tested the corrected website flow after restarting and confirmed it works. This closes the URL-launch startup check; it does not certify every etaHEN feature.

This is an experimental source port of the supplied etaHEN 2.5B tree. The original Desktop source remains unchanged. **Fresh-boot startup passed, including both services, Toolbox hooks, FTP and automatic restoration of the user-deleted Toolbox card.** The user confirmed the restored card opens and the requested buttons are gone. The PS5Debug backend starts a working server and blocks repeated loading; the user has now independently loaded the corrected ELF and confirmed the Services button works. Compatibility of every etaHEN feature is not established.

Last hardware-tested ELF: 30,413,072 bytes, SHA-256 `c25f77fa0283e18c0bdce2220cca92062af912ee281121164c36f9a991d4d0a0`. The latest build fixes the new value-less Services button's missing allow-list entry: 30,413,072 bytes, SHA-256 `242b53c63ddf461e17f8b750a482a93c0e2a97e98f2a31b53a82dc7f9d424b6f`. The user independently loaded and tested the corrected build and reported success on 2026-09-29. The overall manifest intentionally remains `hardwareValidated: false`; evidence below describes which features passed.

## Services/menu update

Services now has a **Start PS5Debug-NG** button for bundled v1.3.2 (payload SHA-256 `949b0e6e0fe3f24f4a820319fd76d9ccb92f9770cbb1a64b09a8eca5c1fc9ff3`, embedded branding fingerprint `5d2c3bbc4bd42087`). The 13.60 handler replaces the obsolete firmware rejection and inverted spawn-result check. It refuses to inject when TCP 744 is occupied or its own daemon has already successfully dispatched a debugger. The check uses bind without connecting to an existing server. Failed port checks block loading; a successful spawn is reported as starting, not proof of a listening debugger. It does not persist an enabled toggle or auto-load from the old config value. Restart the console to disable/reload.

The main Toolbox's Homebrew Store and PS5 webMAN Games buttons were removed as requested and the user confirmed their removal. The initial PS5Debug button was ignored by the generic empty-value guard; its missing allow-list entry is now fixed, with a regression check covering every value-less button in this Toolbox XML. The full build and all nine local tests passed. An IPC-only hardware harness issued the same Services command twice: daemon PID 94 spawned debugger PID 98 once, then refused to reload it. A real connection on TCP 744 confirmed PS5 firmware 13.60, v1.3.2 branding and a valid process list. Evidence: `artifacts/services-freshboot-debug-service-test.json`, `artifacts/services-freshboot-etaHEN.log`, `artifacts/services-freshboot-debugger.json`. On 2026-09-29 the user independently loaded and tested the corrected ELF, reporting that it works. This closes the pending Services UI test; no additional payload was sent for that confirmation.

## Toolbox navigation and separate home-screen card

The native 13.60 `debug_settings` page does not load the legacy Toolbox resource. Metadata inspection confirmed the resource name is unchanged; direct calls through both resource-stream overloads returned valid etaHEN XML. Evidence: `artifacts/run-d79b5c720a4a-report.json`, `artifacts/run-525409e6f86a-report.json`, `artifacts/run-d02bea79573f-report.json`.

Launching `pssettings:play?mode=settings&function=debug_settings_old` returned success. The user confirmed it opened etaHEN's Cheats page, selected by their earlier game-menu action. Thus the legacy route reaches etaHEN, while entering the normal Debug Settings page still opens Sony's UI. Evidence: `artifacts/toolbox-route-test.json`.

The user requested a separate card beside the Store. Title `ETHN13600` / `etaHEN Toolbox` is now registered in the application catalog with the legacy route and `etahen_root=1`. The user confirmed the card appears and opens Toolbox. The card's placement follows ShellUI ordering; a fixed slot immediately beside the Store is not verified. Evidence: `artifacts/toolbox-card-install.json`, `artifacts/toolbox-card-catalog.json`.

The full ELF uses the legacy route for notifications, startup and controller shortcuts. The root link clears pending cheat/game shortcut selection. Sony's native debug page retains its own label/icon after a fresh UI load. The root card route is loaded and user-confirmed; individual notification/controller routes have not each been manually tested.

On this console, the old single-title registration export is absent. The user explicitly approved `sceAppInstUtilAppInstallAll`, including the possibility of registering other already-staged apps, after automatic approval review blocked the broader effect. The scan returned 0; the card was independently found in `tbl_contentinfo`. The user then requested bundling card installation into the main ELF for distribution. The bootstrap now spawns the embedded installer; its ownership receipt and exact asset comparison skip registration when the card is current.

Removal returned 0, removed the owned directory, and left no catalog entry for this title. Reinstallation returned 0 and restored the catalog entry. A separate harness using the bootstrap's exact `elfldr_spawn` implementation successfully launched the embedded helper, which returned `already-installed` without another scan. Evidence: `artifacts/toolbox-card-remove.json`, `artifacts/card-removed-app.db`, `artifacts/toolbox-card-reinstall.json`, `artifacts/toolbox-card-idempotent.json`, `artifacts/card-spawn-test.log`. The main ELF's fresh-boot automatic installation subsequently passed: helper PID 95 registered the user-deleted card, and the catalog plus user confirmed its restoration. See `TOOLBOX-CARD.md`.

## Fourth integrated startup: responsive; legacy menu route verified separately

- Read-only comparisons confirmed that ShellUI and ShellCore shared physical pages containing `read`, `ioctl` and `sceRegMgrGetInt`. The SDK's physical-write fallback could therefore publish a ShellUI-only hook into another process's code.
- Publication now temporarily enables writing in the target VM mapping, uses PT_IO to trigger process-private copy-on-write, restores the original VM protection and verifies bytes. There is no physical-write fallback.
- An identical-byte diagnostic verified all three ShellUI pages became private while ShellCore's mappings and original bytes remained unchanged. Evidence: `artifacts/shared-page-preflight.json`, `artifacts/etahen-1360-private-code.json`.
- Five actual hooks and then all 19 actual hooks ran for five seconds and rolled back successfully. FTP remained responsive. Evidence: `artifacts/run-b0a75f5f47c5-loader.json`, `artifacts/run-0490de09bf40-loader.json`, and the corresponding `cow-*-hooks-injector.log` files.
- Full startup created both etaHEN services and kstuff. Read-only preflight confirmed installed kstuff markers and private native hook pages. etaHEN FTP on port 1337 successfully listed `/data/etaHEN` and downloaded the port configuration. Evidence: `artifacts/cow-full-preflight.json`, `artifacts/run4-cow-*.log`, `artifacts/cow-full-etaftp-config.ini`.
- User confirmed the console stayed responsive and the etaHEN icon appeared, but selecting Debug Settings opens Sony's usual menu. Icon replacement is not Toolbox functionality.
- The original successful boot's native pages were first privatized by the independent identical-byte diagnostic. The subsequent fresh-boot full startup also passed without that diagnostic. Its preflight showed shared UI/Core physical pages, then the main payload published the hooks and confirmed Toolbox readiness while FTP remained responsive. Evidence: `artifacts/services-freshboot-preflight.json`, `artifacts/services-freshboot-startup-1360-port.log`. The change restores VM protection metadata; hardware page-table permission restoration was not separately inspected.

## Confirmed on the user's PS5 13.60

- Prerequisite diagnostics matched firmware `0x13600007`, the SDK kernel addresses, process access, and Mono exports. ShellUI PIDs change after reboot; the most recent isolated test used PID 58.
- The repaired injector loaded a disposable ELF, verified its bytes, ran its thread and confirmed zero-initialized BSS.
- ShellUI diagnostics attached to Mono, enumerated assemblies and resolved the expected method variants. Boot uses three parameters; CaptureScreen uses five. The old JavaScriptBundleDecryptor class is absent, for which upstream uses an ioctl fallback.
- All 18 resolved managed targets and three native targets prepared trampolines successfully without activating system-method hooks. Evidence: `artifacts/run-ca781bcd8f6f-report.json`.
- A private test function inside ShellUI returned 12 before installation, 112 while hooked, and 12 after rollback. Evidence: `artifacts/run-1993daecf962-loader.json` and `artifacts/run-1993daecf962-report.json`.
- The complete ShellUI component reached its prepared-hooks stage with activation disabled and exited successfully. Evidence: `artifacts/run-383aa0cf846e-report.json`.
- The shared service loader spawned an isolated child process, which confirmed execution and zeroed BSS. Evidence: `artifacts/spawn-child.json` and `artifacts/spawn.log`.
- Stopped-process publication and rollback passed the private-function test while checking 281 threads. Evidence: `artifacts/run-0ac0093d966a-report.json`.
- A temporary live `Application.Update` hook forwarded 31 calls and restored the original entry successfully. Evidence: `artifacts/run-0924af42c9d8-report.json`. Other replacement functions were not validated by this test.

These earlier isolated checks establish the tested loading and initialization paths, not the behavior of every etaHEN feature. No new debugger or kstuff payload was sent during those isolated checks; the later full startup did load bundled kstuff.

## First integrated test failed

The user explicitly authorized the full test by replying 'continue' to the approval request. The 29,623,008-byte ELF with SHA-256 78735563f1a7a8a12c10fd5b21945b52c96a34c632efa511c456065b2a46302f was transferred. The user saw the etaHEN starting notification, followed by a freeze and crash. The exact failed binary is retained in artifacts/etaHEN-1360-failed-78735563f1a7.elf.

Recovered `artifacts/crash1-*.log` files show bootstrap completed and both services spawned. The critical daemon reached Toolbox initialization and received the IsTestKit IPC request. They do not establish which operation caused the crash. The utility unnecessarily inspected ShellCore before rejecting unsupported 13.60 patching; its firmware guard now precedes that access.

The final artifact remains marked `hardwareValidated: false`. A transfer acknowledgement alone does not prove execution. Confirm daemon startup, Toolbox activation and continued console responsiveness, then test actual UI operations before claiming compatibility.

## Second integrated test failed

The revised 29,659,264-byte ELF, SHA-256 `0c889280f16833f8cce9d83a64626877679a2768ccd603f5469ab63e4c646a40`, also froze and crashed the PS5, confirmed by the user. It includes stopped-process hook publication, the early ShellCore guard, removal of explicit null-call traps in the FPS registry hook, and bounded overlay processing. These changes did not resolve the integrated failure.

Its exact binary and manifest are archived as `artifacts/etaHEN-1360-failed-0c889280f168.elf` and `artifacts/failed-0c889280f168-manifest.json`. Recovered `artifacts/crash2-startup-1360-port.log` confirms Toolbox readiness before the crash. This is a post-activation failure, not simply failed ELF transfer.

Finer publication checkpoints preserve the child's last observed startup stage in the injector's persistent log. A separate phase-4 metadata diagnostic successfully inspected method signatures and the controller-data return layout without installing hooks or invoking the inspected methods. Evidence: `artifacts/run-b54fe7457def-report.json`. Build it with `ETAHEN_AUDIT_PHASE=4` and `ETAHEN_AUDIT_SIGNATURES=1`.

## ABI correction and third integrated failure

Metadata established that `BootHelper.Boot`'s third argument is `Nullable<Int64>`, size 16/alignment 8, with `hasValue` at offset 0 and `value` at offset 8. The old hook incorrectly treated it as a `MonoString*`. The 13.60 hook now preserves the value-type argument; a native SysV forwarding test exercises both presence states. Controller data remains 36 bytes with the expected field layout. The initialization thread now detaches from Mono before entering its indefinite native sleep.

The resulting full ELF, 29,662,848 bytes and SHA-256 `4ff7f96f91d5bf0891526300f1f5414291bebc1cde90786a597d89691f2cd771`, still crashed. Its binary is archived as `artifacts/etaHEN-1360-failed-4ff7f96f91d5.elf`. The first live log ended before spawning kstuff, but the recovered log **continued through service startup, verified hook publication, target resume and Toolbox readiness**. Do not attribute this crash to kstuff based on the incomplete live capture. Evidence: `artifacts/crash3-startup-1360-port.log`.

After recovery, the read-only preflight showed original sysent tables, unmodified crypt tags, and no kstuff process: `artifacts/after-crash3-preflight.json`. The bootstrap's revised state check also recognizes paused installations and refuses an automatic load if the existing hook state is inconclusive. These checks have local coverage; the unmodified-to-installed path passed during the fourth integrated startup.

A five-second diagnostic using the actual first five Toolbox hook bodies (main-thread check, render update, registry, read, option-menu JSON) was sent without starting kstuff or services. FTP timed out; the user confirmed another crash. Recovered `artifacts/crash4-*` logs show both publication and restoration succeeded, and the initializer returned 0 before the later crash. Run ID: `ee423560de139e8473d9e9038ab8bca4598c6ad81104581410b50dbfa68fa151`. Exit 0 does not establish console stability.

The registry hook was changed to call its saved original-function trampoline instead of a private syscall export. A read-only export check established that the private export does exist on this console, so its absence was not the cause. The revised five-hook probe, preserved under `build/probes/registry-forwarding/`, also crashed (user confirmed). **Do not resend this archived probe.** Recovered `artifacts/crash5-*` logs again show publication/restoration before the later crash. The zero-hook full-initialization baseline completed and FTP remained responsive: `artifacts/run-b945306ebd56-loader.json`.

`ETAHEN_PORT_PROBE_HOOKS=0x1f` selects the first five hook records; it is a temporary diagnostic that attempts to restore entries after five seconds. Packaging explicitly rejects a probe component.

## Physical-write fallback investigation (historical)

The SDK's `kernel_proc_copyin` falls back from mdbg to direct physical-page writes. That bypass can modify a shared executable page used by another process, installing a ShellUI-only destination there as well. Shared physical mappings were subsequently confirmed on hardware; the successful private-page tests are recorded above. The exact crashing instruction was not captured.

Hook publication now requires PT_IO success and has no physical fallback. The ShellUI direct-write helper likewise uses mdbg without the SDK fallback. The subsequent copy-on-write implementation permits private executable patching, as verified above. Do not repeat archived native-hook probes from before this change.

The normal full ELF was rebuilt successfully with those guards: 29,664,344 bytes, SHA-256 `542b5f56312e14f6823192a592aaa8ca217a708dc7b79d0bbb11a42ff2a97d6d`. It has not been sent or hardware-validated and is not expected to bypass execute-only restrictions. All eight local checks pass.

## Port changes

- PS5/SCE ABI settings: 16 KiB pages and 16-bit `wchar_t`, with compile-time checks.
- Correct SDK syscall gateway and full-width pointer return values.
- Zero-initialized ELF mappings, corrected payload arguments, checked remote writes and instruction-stepped remote syscalls.
- Shared service spawning through the tested libelfldr path, replacing the bootstrap's older private loader.
- Page-bounded reads avoid the SDK's cross-page copy fallback overrun.
- Checked access to execute-only AOT code; no direct dereferencing of those code pages.
- Trampolines relocate relative control flow and supported RIP-relative operands. Unsupported encodings fail preparation. The arena is allocated before attaching to Mono.
- Hook preparation precedes activation. The injector stops target threads, checks their instruction pointers against overwritten intervals, verifies original bytes, applies the batch through VM-managed PT_IO writes, and resumes. Private-function, live AOT-method and all-19-hook rollback tests pass; broader reliability remains under test.
- Failure paths restore credentials. The daemon no longer kills ShellUI automatically when this experimental initialization fails.
- A per-process marker prevents repeating a completed Toolbox injection in the same ShellUI process.
- Kstuff pause/resume checks installation evidence and preserves prior markers. Process names alone do not establish active kernel hooks.
- Existing etaHEN settings are preserved. This port uses `/data/etaHEN/config-1360-port.ini` (ShellUI accesses `/user/data/etaHEN/config-1360-port.ini`). The original on-console configuration was backed up locally before testing.
- ELF alignment/entry validation and content-aware embedded-asset compilation prevent stale embedded payloads. Packaging refuses a preparation-only ShellUI component.

## Build

```powershell
node scripts/build-all.mjs
```

Output: `build/etaHEN-13.60-experimental.elf`; SHA-256 and size are recorded in `build/manifest.json`. All nine current local tests pass, including relocation checks against 21 captured managed/native prologues, native execution of a far trampoline, nullable-argument forwarding and kstuff state classification.

The build uses the adjacent PS Neighbourhood Zig 0.14.1 and PS5 payload SDK v0.43, or the `ZIG` and `PS5_PAYLOAD_SDK` environment overrides. Source changes are under `Source Code`, `port`, `scripts` and `tests`.

## Remaining compatibility work

Full bootstrap, service startup, installed kstuff markers, FTP listing/download and opening Toolbox through its own card passed on the current boot. Actual package installation, decryption, plugins, FPS and other menu actions remain unverified for this etaHEN port. The older `/data` sandbox patch has no verified 13.60 signature and is skipped before module introspection; that guard ran during successful full startup. Successful hook publication does not validate every replacement function. Fresh-boot startup with bundled card installation passed once. Repeated startup, cross-thread publication and failure recovery need further reliability testing.

## Upstream dependencies and attribution

Original authors and GPL licensing are retained. This port is unofficial.

- etaHEN 2.5B: https://github.com/etaHEN/etaHEN/releases/tag/2.5B
- PS5 payload SDK v0.43: https://github.com/ps5-payload-dev/sdk/releases/tag/v0.43
- kstuff-lite v1.11: https://github.com/EchoStretch/kstuff-lite/releases/tag/v1.11 — embedded release SHA-256 `ab9a6cb4d3b1daf139d4d646e402b1cf569071acd64599c936d7a3a6164dc779`; not loaded during the isolated tests.
- ELF loader v0.26: https://github.com/ps5-payload-dev/elfldr/releases/tag/v0.26
- PS5Debug-NG 1.3.2: https://github.com/Pharaoh2k/ps5debug-NG/releases/tag/1.3.2 — bundled dependency, not reloaded.
- PS5 app dumper v1.11: https://github.com/EchoStretch/ps5-app-dumper/releases/tag/v1.11
- SELF pager reference: https://github.com/ps5-payload-dev/ftpsrv/blob/master/self-prospero.c
