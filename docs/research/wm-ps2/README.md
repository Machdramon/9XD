# Wangan Midnight PS2: height and texture references

Updated: 2026-10-26. Owner: ChatGPT/Codex. Source game/scripts were read only. No game bytes/images copied here.

## Corrected height

User confirmed inverted height in study cases/wm ps2/00000740.0iv. It contains three consecutive raw 0IVX00.1 / LDMX00.2 / MRON00.2 models and a trailing table of seven ID/XYZ records with zero alignment padding. Legacy IVX_rework_rev0.77_local_read-C18E0.py (Gato's Lair) distinguishes raw tracks at line 771 and contains Y inversion at lines 1058–1059. This is supporting convention evidence, not a claim that the old script is a fully correct importer; it was not executed.

Development fix: recognize this bounded raw concatenated-track carrier only after complete hierarchy walks and finite, complete placement-table validation. Convert final node/world positions and normals by reflecting Y; combine reflection parity with locator scale to correct winding. This converts coordinates, not placements: the trailing IDs are still not interpreted as model-instance indices. Other axes, UVs and binary bytes are unchanged. Single-model wheel 000000a3.0iv and ordinary directory-wrapped car files are excluded. Shared raw TXR Zero track carriers follow the documented track convention; no global Genki axis change.

Implementation: src/genki_hierarchy.h (rawTrackYDown), src/genki_mesh_evidence.h (sourceYDown and coordinateConvention), src/genki_geometry.h. Test wmTrackHeightConvention confirms actual WM vertices individually: Y negated after locator transform, X/Z preserved; reflected winding; malformed/truncated trailer rejection and independent wheel exclusion. WM output: 2,488 vertices, 1,826 triangles.

## Existing texture knowledge

- The WM model does not include its referenced TIM2 image bank. The wheel references bank 0 and images 0/52/85/119 in the existing inventory; the track material receipts expose bank 0 with indices including 0/2/4/5/8/16/19/26/28/33/34/46/65/67.
- TXR3 research script txr3_reworkglob-dump_rev1.32-0909B.py lines 605–625 reads descriptor byte +13 as container and +14 as zero-based image index. Container 0 resolves common/global textures, 4 animated/global textures, other values local textures in that script. Its filenames add 1 to the stored image ID. These role rules are evidence from TXR3, not yet a verified universal WM mapping.
- WM's old Discord channel says manual texture assignment may be necessary; current.py/curwip.py are early readers and do not establish an automatic final texture resolver.
- Current native CLAMP receipts for some WM parts contain implausible dimensions; validate descriptor layout and texture-enabled flags before using those receipts to accept external images. Do not guess a bank merely from filename adjacency or image-count coverage.

## Local bank survey

Location: F:/copia de seguridad 08.04.2019/downloads/Wangan Midnight ps2/unpack/quickbms unpack.

317 syntactically valid TIM2 bank regions found across extracted files (not 317 unique texture sets). For banks with >=120 images, only 00000150.tm2 qualifies: 169 images, alignment-128 header. Image dimensions: 0=16×16, 52=64×64, 85=64×64, 119=32×32. This makes it a candidate for the wheel's referenced range, not proof of the track's bank. For the reported track, additional count-compatible standalone banks are 00000155.tm2 (97), 00000162.tm2 (68), 000001be.tm2 and 000001c0.tm2 (71 each); some embedded SDB banks also cover that range. 00000150's index 67 is 64×64, while the current track receipt requests 256×256: no automatic assignment made. Reused images and non-textured material fields remain possible.

survey_texture_banks.py reproduces the >=120-image subset with bounded records and respects TIM2 header alignment. It writes metadata only in the caller's working directory; it does not execute legacy scripts or alter game files.

## Validation / next work

- Development executable built and updated.
- Reader 45 passed/0 failed; windows 102 passed/0 failed/77 optional skipped.
- Fixture-enabled WM/hierarchy/isolation/March: 7 passed/0 failed/0 skipped.
- Logs: build/review-fixes-app-build.log and build/windows-local/wm-height-targeted.txt, release037-reader-tests.txt, release037-window-tests.txt (local names reused, no new public release).
- Pending: user visually confirms corrected height; validate external material-bank selection, exact WM descriptor/CLAMP fields and bank contents, then implement verified cross-file texture binding. No new capture required yet.


## Track texture context investigation — 2026-10-26

See docs/research/genki-track-banks/README.md: original scripts/messages and two TXR3 oval contexts validated for 3,512 index-level references. Global/local banks are scoped to track condition and segment; 18/54 common global payloads differ between variants. Native linking and sampler/UV validation remain pending; prototype JSON is not yet application support. WM requires its own context; no TXR3 ID reuse.


## Native track-context support — 2026-10-26

Verified TXR3 Oval-A/B context selection is now integrated in scan/render/texture collection/GLB/FBX via fingerprinted profiles/genki-txr3-oval-contexts.json. Real model tests load/export 21 and 31 external textured parts; full suite passes. See docs/research/genki-track-banks/README.md for usage. WM/other-game context maps, exact sampling and animation remain pending; do not reuse TXR3 IDs. No new release.
