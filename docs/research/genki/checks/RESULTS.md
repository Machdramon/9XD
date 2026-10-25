# Genki capture cross-check results

Updated: 2026-10-25. Research checks completed against all nine saved captures. No 9XD reader or UI changed; Claude retains native integration ownership.

## What was checked

- All 9 original compressed SHA-256 values match their inventories. Each dump was decompressed again with bounded output and compared byte-for-byte with the saved raw dump.
- Embedded serial, state version/length, screenshot bounds, freeze-state equality and GIF record/tag payload bounds validated. All observed transfers use GIF path 3; the tool rejects multi-path captures rather than inventing their ordering.
- 10,546 host-image transfer transactions replayed using PSMCT32/24/16 and PSMT8/4 layouts. Transfer order is retained, including nonzero destination origins and split IMAGE payloads.
- Source indices are sampled at observed drawing bindings, after supported uploads. Potential framebuffer/Z-buffer destinations are conservatively invalidated instead of rasterized. Twelve Zero local copies invalidate their destinations; their pixels are not emulated.
- Stored palettes are checked separately from indexed pixels. CSM1 RGBA32 storage and the observed RGB24 storage cases are compared. This is **not** a complete CLUT-cache, TEX1/CLAMP, final-shader or source-to-mesh emulation.

## Results without duplicate counting

The table counts distinct indexed pixel payloads and distinct (payload, stored-palette) pairs, not TEX0 configurations, repeated frames or duplicate source records. Exact storage matches alone do not identify a unique vehicle/LOD or material draw.

| Reference | Distinct indexed payloads at drawing bindings | Exact stored-palette pairs | Interpretation |
|---|---:|---:|---|
| TXRD2 March | 2 | 2 | Both known 512×512 and 256×256 surfaces persist at drawing bindings across four captured frames. Third image and aliases unresolved. |
| TXRD2 initial AZ1 reference | 1 | 1 | One shared surface; insufficient to identify a unique car variant. |
| TXRD2 Hakone | 36 | 36 | 37 source-picture records: 32 global, 1 in local 00001726.dat and 4 in local 000017b8.dat. A duplicated payload explains 37 versus 36. Local 00001759.dat still unmatched. |
| TXR3 Evo V / c4 candidate | 7 | 1 | More indexed payloads recovered than with the initial snapshot, but six have no exact stored-palette match. Body/variant identity remains unresolved. |
| TXRD1 USA Alto sample versus Europe capture | 0 | 0 | Source game serial/build differs. Do not call the source broken from this result. |
| Kaido Racer Europe Alto candidates | 2 | 2 | Original Europe out/00000442.dat has two source images with full storage matches; out/00000447.dat shares one. Not proof of exact visible kit/LOD. |
| Kaido Racer Europe Hakone bank search | 11 | 4 | 47 banks / 5,155 images screened. Matches repeat across many banks; no unique circuit bank or condition proved. |
| TXR Zero AE86 candidate | 1 | 0 | 42 source images considered, including coverage-checked transfer-form indices. The matched runtime descriptor uses a 16-bit CLUT; palette support and local-copy replay still need work. |
| C1 WRX | 9 | 6 | 11 source-picture records, 8 with an exact stored-palette match at least once. Duplicate source records explain the lower distinct counts. Exact SportsWagon/sedan and main body colour remain open. |
| C1 Nikko | 30 | 30 | 85 source records across candidate banks 2498/2532/2566 share 30 payloads. Their matches do not distinguish Hare/Dry uniquely. |

### Previously unresolved RGB24 case

The global TXRD2 Hakone bank 00001701.dat, picture header at **0x6ef40**, now has an exact three-byte RGB palette-storage match at an observed drawing binding. Comparable RGB24 storage matches also occur in the Nikko candidates. This verifies the RGB24 storage interpretation; draw-time CLUT-cache alpha expansion and full native integration remain pending.

### Corrected Alto provenance

The study fixture 00001363.dat is byte-identical to the USA TXRD1 unpacked file, from **SLUS-21236**. Our capture is Europe Kaido Racer, **SLES-53191**. Europe out/00001363.dat is only 17,104 bytes and is a different resource. Source indices must not be transferred between these builds. Europe candidates 00000442.dat / 00000447.dat provide exact capture matches and are the better sources for this capture.

## March reader hand-off for Claude

`march-bank-validation.json` and `scripts/march_bank_validation.py` provide a bounded research admission prototype:

- Original file/TOC provenance already exact in ../txrd2-march-capture/SOURCE-PROVENANCE.md.
- Observed two-section container; 0IVX/LDMX00.1 model section followed by a headerless picture bank.
- Six slots remain in source order. Slots **0, 3, 4** store Indexed8 pixels and RGBA32 palettes; slots **1, 2, 5** are **48-byte reference-only records**. Some aliases declare HeaderSize=128 although their physical record is 48 bytes; do not advance 128 or read nonexistent pixels.
- Stored records have HeaderSize=128, one level, PictFormat=0, ImageType=5, CLUT type=3, 256 palette entries, dimensions agreeing with TEX0 and exact bounded pixel/palette spans.
- Expected final 16-byte zero trailer; unknown layouts fail admission rather than being guessed.
- The real March file passes. Fourteen mutations/truncations are rejected, including alias-with-payload, bad spans/dimensions, short palettes, unknown formats/mips and nonzero/short trailers.
- Empty aliases must stay unresolved. Their external dependencies, complete material-to-slot selection and the third 128×128 image are not verified. This prototype does not authorize substituting a nearby texture.
- Runtime relocation is measured per capture, not a universal 0x3300 constant.

## Replay validation and remaining work

Nine synthetic replay checks passed: five exact split-transfer write/read cases (32/24/16/8/4 bits) with nonzero origins, incomplete-transfer rejection, IMAGE-without-start rejection, destination-only invalidation for a local copy, and explicit unsupported-format handling. These are research checks, not independent hardware conformance.

Remaining priorities:

1. Emulate draw-time CLUT reload/cache semantics, including CLD/CSA and 16-bit palettes; preserve TEX1/CLAMP and effective context state.
2. Implement and validate Zero local-to-local copies, then reassess its textures.
3. Resolve March aliases/third surface and mesh material slots; hand only verified cases to native integration.
4. Identify exact Nikko and Kaido Hakone condition/placement banks; shared pixels do not establish exclusivity.
5. Separate vehicle part, kit and LOD choices; no filtered preview should become a topology repair.
6. Integrate verified rules through Claude, then test scan/render/export in 9XD. No new game captures are needed yet.

## Reproduction and evidence

- `cross-analysis-summary.json`: compact counts, source records, limitations and additional Europe source searches.
- `march-bank-validation.json`, `replay-validation.json`: positive/negative research checks.
- Full transfer/binding/match timelines are retained locally under `local/`, excluded from Git; original captures remain in their existing local folders.
- Run research scripts from docs/research/genki with the workstation's Python+NumPy and the original captures/corpus available. Paths remain workstation-specific; these tools are not application dependencies.

Register/freeze-layout references: [PCSX2 v2.8.2 GSState](https://github.com/PCSX2/pcsx2/blob/v2.8.2/pcsx2/GS/GSState.cpp) and [GSClut](https://github.com/PCSX2/pcsx2/blob/v2.8.2/pcsx2/GS/GSClut.cpp). The checker is original research code; PCSX2 source code is not bundled.
