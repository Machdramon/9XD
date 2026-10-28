# Generic PS2 VIF geometry recovery

Updated: 2026-10-26. Author: Claude. Status: algorithm implemented in 9XD (`src/ps2_vif_core.h`, `src/ps2_vif_geometry.h`); texture linking still open.

## Idea

Most PS2 games send geometry to the Vector Unit 1 through VIF `UNPACK` commands, whatever their file container. Instead of writing a decoder per game, 9XD now:

1. **Finds VIF code streams** anywhere in a file: every word that looks like an `UNPACK` is a seed; the walker accepts only exact chains of valid VIFcodes whose data sizes line up (STCYCL/CL-WL, STMASK, STROW, MPG, DIRECT, DMA cnt/next/ret/end tags).
2. **Splits batches** at microprogram calls (`MSCAL`, `MSCALF`, `MSCNT`). Each batch is one VU1 draw.
3. **Infers attribute roles** per batch from the data itself: the vertex count is the most common UNPACK count; float/int16 3- and 4-vectors become positions unless they are unit length (normals); 2-vectors become UVs; signed 8-bit unit vectors become normals.
4. **Chooses the topology rule by measurement.** Candidates: plain triangle strip, triangle list, and "strip with restart flag" for every bit 15 / bit 0 / bit 7 of every component of every stream (and its negation). Each rule is scored over all batches with the same stream signature: good triangles (edges < 5 × median, non-degenerate, face normal agreeing with stored normals) minus 3 × bad triangles. Stitching degenerates (repeated positions) are ignored.
5. **Rejects noise.** A signature group must have ≥ 2 consistent batches and spatially coherent vertex order (median step < 8 % of the group's bounding diagonal); groups at a very different coordinate scale from the main geometry are dropped.
6. **Splits parts at DMA `RET` tags** (`0x60000000` qword). In the Genki family RET closes every part (material draw), not every model.

When a float position carries a draw flag in its lowest mantissa bit (the C1/TXRD2 "Y-bit" rule), the flag bit is cleared from the coordinate.

## Results on the study cases (C++ core, same numbers as the Python prototype)

| File | Batches accepted | Vertices | Triangles | Parts | Notes |
| --- | --- | --- | --- | --- | --- |
| c1 ps2/000004b8.dat | 534 / 548 | 16,891 | 13,963 | 80 | Car, Y-bit rule found automatically |
| c1 ps2/00001540.dat | 361 / 373 | 3,224 | 1,902 | 38 | Track section |
| kaido racer/000004ac.dat | 667 / 704 | 4,976 | 3,642 | 31 | **Car, previously blocked in 9XD**; plain strips; checked live in the app |
| kaido racer/000031c4.nfc | 117 / 120 | 762 | 528 | 14 | |
| txr3/000000c4.nfc | 577 / 633 | 5,610 | 4,329 | 26 | Car |
| txrd1/00000d09.nfc | 475 / 475 | 3,898 | 2,945 | 17 | |
| txr zero/0x000004d2.dat | 181 / 183 | 1,057 | 695 | 10 | Concatenated models (were blocked) |
| wm ps2/00000740.0iv | 261 / 285 | 1,773 | 1,251 | 18 | Concatenated models (were blocked) |
| txrd2/car/000002a0.sdb | 98 / 173 | 4,191 | 2,334 | 32 | Partial: some groups need another rule |
| r-racing/CourseSuzukaF/Model | 1,088 / 1,429 | 30,631 | 26,850 | 1 | **New game**: whole Suzuka course; checked live |
| unknown/Model1.nud | 504 / 618 | 19,830 | 10,285 | 1 | **New**: a sky dome |
| super trucks racing/ALBACETE.MIX | 111 / 129 | 6,089 | 2,283 | 1 | **New**, not inspected visually yet |
| fnf ps2/*.arc (ARCP) | 20–70 per file | — | — | 1 | Geometry found but scrambled: per-part transforms/quantization not applied |

No candidates: Fast & Furious ARCX banks (already native, Xbox layout), Auto Modellista, D1GP, GT4, Initial D (`n_sy2_blk_*`), R Racing car `Model` (different format), PSP/Xbox/PC files. Pseudo-random data and the non-PS2 corpus produce no candidates (natsuiro `.cat` and D1GP noise are rejected by the ≥ 2-batch and coherence rules).

## Texture linking: what was measured

- **GS register writes** (TEX0 via A+D, BITBLTBUF uploads) are almost absent from these model files: 0 TEX0 in every Genki model file; BITBLTBUF only in F&F ARCP, D1GP and Initial D course AFS. So the generic "TEX0 → texture" link only applies to a few games.
- **Genki TIM2 headers carry their VRAM placement** (`GsTex0`: TBP/CBP/PSM/size), but the models reference textures by bank/image index, which 9XD already resolves natively for C1/TXR families.
- Next generic step proposed: *material key discovery* — compare the words that precede each RET-delimited part across parts, look for small integer fields whose range matches the number of textures in the bank, then confirm with UV ranges vs texture sizes.

## Limits

- Per-part transforms (matrices uploaded as V4-32 × 4) are not applied: F&F ARCP parts land in local space.
- Fixed-point (int16) positions are divided by 4096; real games may use another scale (STROW/STMOD offsets are ignored).
- Indexed VU programs (index streams) are not reconstructed yet; only strips/lists.
- "Parts" are DMA RET segments; model boundaries inside a concatenated file are not separated.

## Files

- `scripts/`: Python prototype (`vif.py`, `census.py`, `extract.py`, `extract2.py`, `render.py`, `gsregs.py`, `corpus.py`) and `harness.cpp` (runs the C++ core on files). `STUDY_CASES` is hard-coded to `/mnt/user-data/uploads/study cases` in the prototype; change `root` to run locally.

## Review follow-up — 2026-10-26

See docs/CLAUDE-REVIEW-2026-10-26.md. Pending: DMA tag mask repair and mixed-group confidence accounting, with regression tests. Generic scan/render remains heuristic; known-format native isolation must be preserved. No scanner changes made by this review.
## Review corrections verified — 2026-10-26

DMA CNT/NEXT/RET/END tag-ID mask corrected; synthetic tag-chain/bounds/reserved-bit checks pass. Confidence now divides retained batches by all classified batches, including rejected groups; a 2-of-102 case stays below 2 percent. Native mesh evidence blocks the generic fallback even if the native decoder has no renderable candidate. Full window suite 102 passed/0 failed; remaining transform/material/format inference limits still apply. No new release.
