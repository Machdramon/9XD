# Blur Haruna: empty scan and large scene loading

2026-11-17. Codex. Focused validation passed; development executable updated.

## Reproduction and cause

Owner confirmed exact installation folder and Level.levelstream. Stream307,269,632bytes; companion7,434,653bytes. Production header probe found441 directory blocks,221 buffer groups,3,718 admitted static sections with561,672 faces. The source-wrapper scan nevertheless returned0 candidates/images. Its sibling request exceeded readPage's1MiB bound and the error was silently converted to an empty result. Showroom's978,033-byte companion did not trigger it.

readComplete now reads in1MiB pages and verifies revisions/cancellation. Both scan and native decode use it. Companion failures are surfaced as notes. A separate fixed128MiB decode ceiling would have blocked previews after the scan fix; it is replaced by the configured asset budget/4. No budget setting is silently changed.

## Reuse and behavior

Geometry::Session holds source bytes, companion, parsed sections and exact texture catalog for one active archive revision. Key hashes full paths, both expected sizes/mtimes and all pending patches. Source revisions are checked before cache lookup and after decode. Shared immutable snapshots allow workers to finish independently; replaced session can be freed once unused. A mutex protects the shared pointer and a separate mutex protects per-session decoded image sharing, bounded by textureBytes(). Primary images share QImage storage across sections rather than repeatedly consuming retained texture budget.

Paired native scenes return discovery results before unrelated format/pattern passes. Other format paths remain unchanged. Cached data never bypasses file revision checks. Budget-constrained/unsupported/memory-only cases still reject explicitly.

## Evidence and remaining work

Standalone native wrapper now returns3,718 candidates,9 legacy image-inspection recipes; exact per-draw catalog is separate. Representative sections0/1859/3717 have72/1/24 faces and decoded images. No game entrypoint or interactive9XD launched.

Independent sequential probe can consume76 RenderLists;22 additional lists stop at currently unsupported records. Within the76,4,820 first-array packets comprise3,718 Float3 and1,102 Float4-position records. Current3,718/561,672 totals are admitted geometry, not the full race-map total. Additional instance/Float4 packets and full shader behavior remain pending.

New tests target real discoverAssets, application scan/preview, large companion/source, session/image reuse and low-budget/cancellation handling. Final results and executable receipt appended after execution. No source game files/pixels added to repo, commit/push/release.

## Final validation / build

-18 passed/0 failed/0 skipped,25,433ms; build/windows-local/blur-haruna-tests.txt. Haruna production discoverAssets returns3,718 candidates/561,672 faces/441 blocks. Three representative native decodes have attached textures; repeated decodes reuse the exact parsed session and QImage cache key.1GiB asset budget rejects307MiB input explicitly; cancellation covered.
- Offscreen application test opens the actual Haruna stream, scans through the panel and loads a textured mesh. Existing showroom Render All173/36,750, texture identity/catalog, corruption and static Blur regressions remain passing. Full Haruna Render All and Vulkan appearance were not tested; no full map completeness claim.
- Development EXE rebuilt after main.cpp timestamp-only header dependency workaround. Fast native discovery marker and SHA verified; receipt build/windows-local/blur-haruna-receipt.json. Application left closed. No commit/push/release; full suite not rerun.

## Geometry baseline superseded

Later same-day typed parameter and stored billboard work supersedes3,718/561,672 with5,613 sections/903,060 faces. See HARUNA-GEOMETRY-AND-ALPHA-2026-11-17.md; full-map completeness still pending.
