# Genki track texture contexts: global, local and condition

Updated: 2026-10-26. Owner: ChatGPT/Codex. User asked to revisit their Discord scripts and global/local/weather relationships. This is verified source research plus a reproducible binding prototype, not an application integration. No game files or legacy scripts modified/executed.

## Primary evidence in the owner's archive

- Gato's Lair / tokyo-xtreme-racer-3, message 758384400190275624: global/local track containers; a weather version is handled as a separate track (the note also names TXRD2/ITC). Message 758384636857810994: global files may hold two containers, local files up to three, and track chunks must be associated with the correct container.
- assets/txr3_reworkglob-dump_rev1.32-0909B.py lines 605–625: material descriptor +13 = container selector, +14 = zero-based image. Selector 0 uses global common container 1; selector 4 uses global animated container 2; remaining selectors use the local set for the current track segment. Exported image filenames add 1 to the stored index. Local segment starts in this example are 0x4a5f, 0x4a7c, 0x4a9a, 0x4ab7.
- flyingfree-A54CA.py uses the same global/local split. Both scripts contain segment-specific overrides. These must be explicit per-profile exceptions with validation, not universal remapping rules. One revision resets curlocal to 1, so executing it unmodified would not prove correct multi-segment binding.
- file_list_txr3-DD212.txt lists regular/wet paired ranges for test oval, Tokyo, Osaka and Nagoya. We keep the two measured oval variants A/B unlabelled until the exact condition order is confirmed.

## Measured on actual TXR3 resources

Correct extraction root: Tokyo Xtreme Racer 3/Nueva carpeta working one. The sibling uclless uses different extraction numbering/layout; identical numeric filenames do NOT identify the same source payload. study cases/txr3/0000603f.nfc is byte-identical to the corresponding file in the correct root.

| Context | Global common / animated | Local files for the four segments |
| --- | --- | --- |
| Oval A, IDs 4a5c–4ad4 | 00004a5e.dat sections 0 / 1: 54 / 1 images | 00004a5f.dat, 00004a7c.dat, 00004a9a.dat, 00004ab7.dat |
| Oval B, IDs 4ad5–4b4d | 00004ad7.dat sections 0 / 1: 54 / 1 images | 00004ad8.dat, 00004af5.dat, 00004b13.dat, 00004b30.dat |

Measured local TIM2 sections are 3 and 4: A counts 3+26, 3+10, 3+20, 3+11; B counts 3+27, 3+11, 3+21, 3+12. Section indices are zero-based; logical local selectors 1/2 are mapped explicitly by the prototype. No third bank is fabricated when absent.

Context + selector + stored image index resolves all tested texture-enabled descriptor references: A 1,592 (1,083 global / 509 local), B 1,920 (1,113 global / 807 local). These are reference records, not unique images or proof of final shader appearance. No hierarchy/span walk failures in these runs. Tables and manifests are beside this report; detailed per-material output remains local in the caller's working directory.

The global common banks share exactly 36/54 stored image+palette payloads across A/B; 18 differ. The single animated payload matches. Thus a global bank is shared by segments WITHIN the selected context; it must not be assumed identical across every weather variant. Local-bank counts also differ between variants. Switching only local files while retaining an arbitrary global set can still yield plausible indices but wrong textures.

## Remaining sampling issue

Only 10 references per variant match the current native CLAMP maximum extents exactly; 1,582 / 1,910 do not. Exact size equality is not a sufficient universal linking rule: atlas subregions can legitimately differ, and WM/TXR3 descriptor/GS state offsets still need checking. Index resolution is demonstrated, but UV scaling, CLAMP/wrap, palette/animation and final material appearance are not validated by these totals. Do not mark these textures fully supported on the strength of index matches alone.

## Application integration contract

Use a GenkiTrackContext keyed by game/build, track, condition and segment. Store explicit globalCommon/globalAnimated references, local selector references per segment and optional verified selector remaps. Every bank reference records source identity, resource/section offset and available image count; cache keys include context and bank identity, not only image index. Changing condition invalidates local and global bindings as required by that context.

1. Discover bank containers without losing parent file/section identity.
2. Select a verified track/condition context (or request an explicit choice when ambiguous).
3. Decode +13/+14 from the variant's verified material layout; use global 0/4 versus the active segment's local selectors.
4. Validate bank/image bounds and storage; apply verified sampler/UV rules independently.
5. Attach the resolved image and provenance receipt to existing scan candidates, texture collection and GLB/FBX export. Keep unresolved bindings visible.

The adjacent prototype JSON is an integration blueprint, not a format currently consumed by 9XD. Numeric ID ranges are specific to the verified extraction root and must be translated by file identity/TOC provenance for other dumps. WM PS2, TXRD2, C1 and ITC require their own contexts and selector-layout proofs; do not transfer TXR3 file IDs or animated-state behavior to them automatically.

## Reproducibility and status

Run validate_txr3_bank_links.py with Python against the documented root. It reads game resources, validates TIM2 picture extents, walks descriptors and outputs only metadata. Legacy scripts were inspected as evidence, never run. inspect_discord_notes.py locates the cited messages. Prototype result: 3,512 references resolve by context/index, with sampling limits above. No app code changed, so no app rebuild/test-suite run was needed for this research. Next: native context resolver + bank selector UI/settings, then sampler/UV validation and WM context identification.

## Native integration implemented — 2026-10-26

The application now consumes fingerprinted profiles from profiles/genki-txr3-oval-contexts.json. The older adjacent prototype remains research-only because it has no bank fingerprints. Current native support is the verified TXR3 Float32-UV primary material layout and the two Oval contexts; this is not yet generic WM/TXRD2/ITC support.

Usage:
1. Open a model in the supported context, e.g. 00004a60.nfc for Oval-A or 00004ad9.nfc for Oval-B.
2. In 3D Data Scan (or File Scan), click Track Texture Context… and open profiles/genki-txr3-oval-contexts.json.
3. Select oval-A or oval-B, then choose the extracted resource folder Nueva carpeta working one containing the referenced .dat banks and model files. A copied study fixture is accepted only if it matches its counterpart in that folder. The sibling uclless numbering is incompatible.
4. Scan again, then Render Selected / Render All. External references show as linked material receipts; decoded images enter the existing texture collection with the mesh. Existing loaded untransformed parts refresh material/texture in place; clearing the context and rescanning restores source-only material resolution without duplicate geometry.

Profiles specify global selectors 0/4, local selectors per hexadecimal segment range, bank file/section and full-file SHA-256. Missing, changed, out-of-range or wrong-extraction banks produce visible notes and preserve geometry. Fingerprints and geometry hash are verified before external decode; caches are keyed by canonical bank path/fingerprint/section/image and bounded by the memory budget. Context selection is per document/scan panel for now, not a global automatic weather guess. RAM resources without a mapped source filename are not context-resolved in this increment.

External material provenance is retained in sourceBindings for the renderer and exporters. Supported source Float32 UVs keep their scale. Exact zero-min/full-image region receipts use Clamp; other regions use Repeat preview with an explicit sampler/atlas warning, pending the format-specific state work. Animated selector 4 is a static source image only. No palette, frame or shader variant is guessed. Index-level prototype counts above are NOT complete native/visual coverage counts.

Validation: reader 45 passed/0 failed; full windows 103 passed/0 failed/78 optional skipped. Real-fixture context/WM/isolation/March run 7 passed/0 failed/0 skipped. Two actual models decode/export 21 (A) and 31 (B) external textured parts to GLB and FBX. Both global selector 0 and local selector 2 are exercised. Actual scan→render and context clear→rescan→material refresh retain one scene mesh and remove the external texture as expected. Initial UI test scanned during asynchronous file opening; it now waits for source readiness, and scanning rejects nonpositive/opening file size. App executable built/updated. No manual visual confirmation of every texture or climate label claimed.

Implementation files: src/genki_track_context.h, asset_discovery.h, asset_scan_panel.h, genki_geometry.h, mesh_preview_registry.h and main.cpp; profiles/genki-txr3-oval-contexts.json; tests/window_tests.cpp. Log: build/windows-local/genki-track-context-tests.txt and release037-window-tests.txt; build/review-fixes-app-build.log. Local filenames reused, not a new release. No commit/push/release performed.

## Automatic loading supersedes manual setup — 2026-10-26

Normal usage is now Open File → Scan → Render. Fingerprinted built-in TXR3 profiles are embedded at :/mesh-profiles/genki-txr3-oval-contexts.json. Discovery checks the source folder and at most two parents, then previously verified locations, without listing the whole folder. Matching bank hashes and source counterpart identity remain required. The scan UI records successful contexts so a later study-case copy can resolve its external resources. Profiles are metadata inside the executable; users do not need to load the JSON themselves.

Track textures: automatic is the default. Texture options… remains for advanced explicit overrides/custom mappings. The Automatic button clears an override and returns to detection; it no longer means source-only/untextured mode. Earlier manual setup/clear-context steps above are historical and superseded by this section. A missing valid companion set is reported rather than replaced with a guessed bank. Arbitrarily renamed/RAM resources and games without a verified map remain outside this increment.

Validation: normal discoverAssets calls without a context resolve Oval A/B. Real UI scan does not call setTrackTextureContext first; copied-model automatic resolution through a remembered location also passes. Returning to Automatic retains one textured mesh. Reader 45/0, windows 103/0 (79 optional skipped); real context run 7/0, two model samples 21/31 textured parts with GLB/FBX exports. Development executable updated; no new release.

## TXR3 reported wheel / sampling follow-up — 2026-10-26

00000af9.nfc contains three nested model-local TIM2 banks; six native parts previously missed the root-only bank reader. 00004a60.nfc uses both GS contiguous region-repeat and atlas region-clamp: e.g. part 0x55a0, V .375–.625 maps region 96–159 into a standalone 64px texture. Native scoped-bank and per-axis UV region correction implemented; six wheel parts decode with images, both track variants decode/export (21/31 parts), focused 5/0 and Genki regression 7/0 pass, full reader 45/0/window 104/0 (80 optional skipped), development executable updated; source power-of-two envelope reconstruction remains an inference needing desktop visual confirmation. Unknown/noncontiguous GS masks retain warnings, shader/animation completeness is not claimed. See coordination plan for current validation status.

Remaining: desktop visual confirmation of reported surfaces, independent recovery of source atlas extent, exact subrectangle edge filtering, noncontiguous masks and secondary/animated shader stages. No global UV flip or texture resampling was introduced.

### Next-session hand-off synchronized - 2026-10-26

See docs/COORDINATION-PLAN.md, "Start here: next session with Claude", for the ordered continuation. First: desktop visual verification of 00004a60.nfc and all six textured wheel parts in 00000af9.nfc. Next: independently verify source atlas normalization and identify remaining failing mesh/bank/image pairs before extending mappings. Automated tests/build have passed; game-equivalent rendering, WM contexts, exact subrectangle filtering and secondary/animated shader behavior remain pending. Preserve automatic texture loading and independent nested model banks.
