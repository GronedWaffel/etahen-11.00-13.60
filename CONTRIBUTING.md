# Contributing

Pull requests, code reviews and reproducible compatibility reports are welcome. This repository is an unofficial etaHEN port for PS5 13.60; please keep upstream attribution and licenses intact.

Start with a focused change and explain the problem, firmware, build tools and verification. Run `node --test tests/*.test.mjs` with the documented toolchain and build with `node scripts/build-all.mjs`. A successful build alone does not prove console compatibility. Record exact features tested and distinguish local tests from hardware results.

Please open an issue before a major architectural change. Other firmware work is welcome but must include its own validation; do not remove firmware or duplicate-instance guards to make an untested version appear supported.

Never attach credentials, account information, proprietary game/system binaries, raw memory dumps or personal save files. Use synthetic fixtures or redacted logs. Generated payloads, local SDKs, static archives and console artifacts belong outside Git.
