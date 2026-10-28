# Review of Claude changes — 2026-10-26

Reviewer: ChatGPT/Codex. Scope: uncommitted changes after release 0.3.7 (main base 15e9ae4), including workspace navigation, Reconstruction module, generic VIF and newer native Genki hierarchy/Kaido changes. No application source modified, no commit/push/release. Review is a snapshot; preserve ongoing contributor edits.

## Findings

1. P1 — Kaido admission can accept damaged C1 packets. src/genki_mesh_evidence.h:45–54 accepts control + positions and optional ordered attributes, including C1's final color stream, without requiring a distinguishing Kaido attribute signature or verified hierarchy. When C1 reset markers are removed, C1 validation rejects it but the Kaido fallback accepts it as plain strips. The existing genkiTopologyProfilesAreIsolated test fails at tests/window_tests.cpp:818 with renderSupported=true. Require positive Kaido-specific admission and retain malformed-C1 rejection; do not simply remove the negative test.
2. P2 — DMA tag recognition rejects all admitted IDs. src/ps2_vif_core.h:76 compares (w0 & 0x8fff0000u) to (id << 28), but this mask removes bits 28–30. A valid CNT word 0x10000001 yields left side 0 versus right side 0x10000000; IDs 2/6/7 similarly cannot pass. Thus walk's DMA branch never recognizes these tags, splitting/losing streams at interleaved tags. Correct the reserved-bit mask/ID comparison and test real CNT/RET/END chains, not only raw VIF streams.
3. P2 — Confidence ignores wholly rejected groups. src/ps2_vif_geometry.h:20–25 sums the denominator only for accepted groups. Two retained batches plus 100 rejected-group batches can still yield heuristic=false, contradicting the stated 80-percent coverage rule. Compare retained batches against all classified batches (and describe unclassified coverage separately); add a mixed accepted/rejected-group test.
4. P2 — Non-uniform locator scale is not applied to stored normals. src/genki_geometry.h:17 scales positions but only rotates stored normals. The hierarchy accepts non-uniform scales. For scale (2,1,1), a normal proportional to (1,1,0) must become proportional to (0.5,1,0) before rotation/normalization. Use an inverse-transpose normal transform, define singular/mirrored-scale handling, and test lighting/export normals for such nodes.

## Actual validation

- Build window_tests/reader_tests using VS2022 vcvars64 and offscreen Qt: passed.
- Reader: 45 passed / 0 failed.
- Full windows: 100 passed / 1 failed / 76 optional skipped; failing genkiTopologyProfilesAreIsolated. This is not the previous compiler/display environment failure.
- Real-file targeted run: 4 passed / 2 failed / 0 skipped. genkiNodeHierarchy and localMarchPrimaryMaterials pass. genkiTopologyProfilesAreIsolated fails as above. localGenkiTopologyIsolation also fails because it still expects Kaido to be blocked: this second failure is an outdated expected-coverage assertion after intentional Kaido support, not by itself proof that Kaido topology is wrong. Update it only alongside explicit positive Kaido and variant-isolation checks.
- DMA mask evaluated directly: 0x10000001 & 0x8fff0000 = 0, not 0x10000000.
- Logs: build/release037-test-build.log, build/windows-local/release037-reader-tests.txt, release037-window-tests.txt and claude-review-targeted.txt. These local log names were reused for review; historical release 0.3.7 verification remains recorded in the release docs and publication receipt.

## Progress and remaining limits

Workspace open-file strip, dedicated Reconstruction module and scrollable scan UI are implemented; their automated checks pass in the full run. Generic PS2 VIF fallback, native node hierarchy/locator transforms and Kaido topology are present locally, beyond the last narrative hand-off. Generic textures remain unimplemented. Generic fallback should be explicitly checked against the owner's unknown-formats-only requirement: current gating is no mesh candidates, which can also include recognized formats whose native decoder cannot yet render.

Next owner: Claude/user to resolve the findings, synchronize native coverage documentation, then repeat full and real-corpus checks before a new release. ChatGPT can implement fixes if requested. Review did not install REA or change any scanners.
## Resolution — 2026-10-26

All four findings fixed by ChatGPT/Codex at the user's request. Kaido now requires full hierarchy plus distinguishing Float32 UV/V4-16 normal evidence; malformed C1 negatives retained. Generic fallback is disabled when native mesh evidence recognizes a format. DMA tag IDs are preserved by the corrected mask. Batch confidence uses all classified batches. Stored normals use inverse-transpose/normalization; singular scales regenerate normals, reflected scales reverse winding. Regression helper/test: reviewDecoderRegressions; real Kaido expectation updated without relaxing malformed-C1 checks.

Final validation: reader 45/0; windows 102/0 (76 optional skipped); real hierarchy/isolation/March 6/0; broad Genki fixtures 14/0; development app built/updated. No commit/push/release. Original findings and failing-run evidence above are historical, superseded by this resolution.
