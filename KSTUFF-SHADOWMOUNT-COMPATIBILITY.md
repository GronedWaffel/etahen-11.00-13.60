# 13.60 kstuff / etaHEN / ShadowMount integration

The September 30, 2026 source review identified an incompatible pause mechanism.
The candidate pair keeps kstuff-lite active and removes legacy pointer-tag
control from etaHEN and the 13.60 ShadowMount build. This is a local candidate,
not a replacement for the currently hosted binaries yet.

## Exact inputs

- etaHEN 13.60 port: this repository, based on LightningMods' etaHEN 2.5B.
- [EchoStretch/kstuff-lite v1.11](https://github.com/EchoStretch/kstuff-lite/releases/tag/v1.11),
  tag commit `85fb88a526e58e357d6f1e626ce05af4acdb8b5d`.
  Embedded ELF SHA-256: `ab9a6cb4d3b1daf139d4d646e402b1cf569071acd64599c936d7a3a6164dc779`.
- [drakmor/ShadowMountPlus 1.7beta2](https://github.com/drakmor/ShadowMountPlus/releases/tag/1.7beta2).
  Upstream ELF SHA-256: `3f716a7b2220c7e87e87452ae05cad689ef842d3beb4cdad6c526cb6dfc2b6b5`.
  Custom version: `1.7beta2-snipers1360-r1`.

## Finding and correction

In kstuff-lite `ps5-kstuff/main.c`, `USE_INT3_SYSCALL_HOOK` is enabled.
Installation creates copied PS4/PS5 syscall tables and replaces selected
`sy_call` entries with the INT3 dispatcher. The published pointers to those
tables are canonical (`0xffff` high word) while kstuff is **active**.

etaHEN and ShadowMount inherited an older controller which treats `0xffff`
as paused and writes `0xdeb7` to resume. That changes the pointer's addressing
semantics; writing `0xffff` also does not remove the INT3 entries. Matching
sysentvec offsets therefore does not establish pause compatibility. This is a
source-confirmed mismatch, not a reproduced explanation for any earlier crash.

The candidate etaHEN no longer changes these tags from game launch/close,
shortcuts, ShellUI exit, Toolbox injection, FPS injection, or stale pause-marker
handling. Reopening an already active Toolbox returns before the state check.
Injection accepts canonical installed tables or the unmodified no-kstuff state;
an unknown or poisoned table state is rejected without live repair.

Legacy pause/auto-pause and external-kstuff replacement controls were removed
from the Toolbox menu. The service also rejects the old download command: this
13.60 bootstrapper already pins embedded kstuff, so claiming an external
"latest" download replaced it was incorrect. Existing pause settings cannot
enable the old write path.

The custom ShadowMount leaves its legacy controller inactive on 13.60 and
guards the final tag-write function as well. This covers launch delays, focus
changes, config reloads, sleep and shutdown. Mount scanning, game registration,
ShellCore hooks, image handling, KEKCALL_CHECK and remote mprotect remain in the
build. Other firmware retains upstream control logic, but this candidate is
intended for the 13.60 bundle. The legacy auto-pause performance feature is
intentionally unavailable with this kstuff-lite integration.

**No custom kstuff binary is needed for this correction.** Its exact upstream
ELF remains embedded. A future true INT3 pause feature needs a documented
control interface and separate implementation/testing; pointer poisoning is
not such an interface.

## Other integration checks

- Native and PS4 sysentvec offsets agree: `0xDDD8F8` and `0xDDDA70`.
- KEKCALL_CHECK and KEKCALL_REMOTE_SYSCALL agree between kstuff and ShadowMount.
- All 34 kstuff 13.60 static ShellCore patches are disjoint from ShadowMount's
  four target ranges and code cave. Kstuff's package wrappers use a separate
  mmap and package API GOT slots; ShadowMount hooks launch, sandbox and install
  functions. This static check is not a proof about every runtime interaction.
- etaHEN's unverified old ShellCore sandbox patch remains disabled on 13.60.
  Its ShellUI native hooks use the existing private-page implementation.
- The host starts etaHEN before ShadowMount and refuses standalone BackPork
  alongside it. The candidate supervisor additionally rejects legacy-poisoned
  tables and embeds the exact custom ShadowMount ELF.

## Validation and remaining hardware check

The etaHEN full build runs its native ABI/state, relocation, embedded-asset and
ELF tests. The ShadowMount test executes its actual initialization and final
write function with a mocked kernel: 13.60 (including minor raw-version bits)
never resolves/enables legacy control or writes tags; another firmware retains
the original path. Both candidate payloads and the supervisor pass ELF segment
validation. The bundle checker verifies hashes, embedding and static ranges.

Previous console evidence confirmed that the old trio started together and
ShadowMount installed its bridge. Its library contained zero games, so that did
not validate mounted-game launch/exit or rest/resume with mounted images.
No candidate payload was sent to the console during this audit.

Test the candidate pair on a fresh jailbreak, etaHEN first, then the custom
ShadowMount (or its supervisor, not both). Check a real mounted game's launch,
play, close, and rest/wake cycle, followed by Toolbox access. Never load a second
kstuff/ShadowMount copy into the current session. Current public hosting is
unchanged at the time of that audit; hardware results were still needed before
calling the pair validated.

## September 30 startup follow-up (r2)

After r1 was hosted, the user reported the system-software error screen and a
nonworking Toolbox. Read-only diagnostics showed ShadowMount's process still
running with its hooks installed, while ShellUI had restarted and etaHEN had
reported failed Toolbox injection. ShadowMount started before the critical
daemon's IPC socket appeared. This narrows the failure to Toolbox startup but
does not establish which operation caused the UI fault.

The supervisor had a definite readiness race: a newly visible critical process
plus `StartUp thread created!!` in a previous boot's persistent log could unlock
optional startup before that process reset the log. Also, the daemon emitted
that success line even when Toolbox injection returned failure.

r2 replaces that log check with an atomic, boot-local acknowledgement bound to
the live critical and ShellUI PIDs. The bootstrapper clears it before creating
services. The daemon publishes success only after its startup actions and
successful Toolbox initialization (or an explicit user configuration disabling
Toolbox). Failure or a changed ShellUI stops the optional sequence. All six
supervisors use this contract, retaining their kstuff and duplicate guards.

Native regression tests execute the actual supervisor check against missing,
old-PID, failed, malformed and restarted-UI records. They also require the live
kstuff check to pass. This fixes a verified code defect; it is not yet proof
that the reported hardware fault is resolved. No replacement payload was
loaded on the user's console during this fix.

After deployment, the user independently tested the updated host on 13.60 and
confirmed Toolbox works and shutdown, restart and rest-mode entry no longer
freeze the console. This supports the startup fix on that console; the exact
crashing instruction was not captured, and mounted-game rest/wake remains a
separate test. Exact release hashes are recorded in `PORT-STATUS.md`.
