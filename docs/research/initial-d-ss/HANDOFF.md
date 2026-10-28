# Initial D Special Stage: handoff for independent analysis

Goal: an independent check of the SMD reader. The geometry loads, but in the app some meshes render too dark,
and textures look like flat colour. Please confirm or refute the findings below rather than reusing them blindly.

## Inputs
- Executable: `F:\download\Initial D - Special Stage (Japan) (v2.00)\...\SLPM_652.68` (owner's copy, read-only).
- Data: `study cases/initial d special stage/COURSE/COURSE.AFS` (+ `.TBL` names). Entry 0 = `AKAGI_DAY.PAC`.
  A copy of that entry is at `build/idss_test/AKAGI_DAY.PAC` (build folder, ignored by git).
- Code: `src/idss_core.h` (Qt-free), `src/idss_model.h` (app adapter); codec notes in `README.md` and `smd_lz.py`
  in this folder. Test `initialDssPac` in `tests/window_tests.cpp`.

## Established (high confidence)
1. **PAC container.** Entries are 32 bytes: {name[16], offset, size, type, flags}. Type 1 = GIM, type 3 = SMD;
   flag bit 0 = compressed.
2. **LZ codec**, from the ELF function at 0x1C54F0:
   - 64 KiB zeroed window starting at 0xFEFD.
   - Flags read lazily, LSB first; bit 1 = literal.
   - Reference = 3 bytes: absolute window position (u16) + length − 4 (u8).
   - All 137 SMDs consume exactly their packed size.
3. **SMD header:** triangles, vertices, texture count, record count, names offset, record table offset.
   - Records are 32 bytes: {VIF offset, qword count, texture index, -1, 0, triangles, vertices, 0}.
   - Header and per-record triangle counts match the decoded geometry exactly in all 137 models
     (216,950 triangles).
4. **VIF per batch:**
   - STCYCL; UNPACK V4-32 @1 (position + W flags).
   - STMASK; UNPACK **0x74** (V2-32 with the mask bit) @2 = UV. This was missed at first, so the UVs were never read.
   - STMOD; UNPACK V4-8 unsigned @3 = vertex colour, typically 0x50505080. The colour is not used yet.
   - Then 1 qword @0 and MSCAL.
5. **W flags:** values 0x8000, 7, 3, 0x8007, 0x8003. Bit 15 = no kick (skip the triangle); this rule gives the
   exact header triangle counts.
6. **GIM:**
   - Header: w, h at +0x30; psm (0x13 T8 / 0x14 T4) at +0x34; transfer w/h at +0x36; pixel offset at +0x3C;
     palette offset at +0x4C.
   - T8 is GS-swizzled when transfer width = w/2. Palette uses the CLUT shuffle; alpha × 2.
   - Sky, wall and grass decode correctly by eye.

## Open problems
- **Winding.** No simple rule gives consistently up-facing ground. Measured share of near-horizontal triangles
  facing +Y across `crs*` (Y is up: road heights 430–480):
  - strip parity: 0.42
  - parity restarted at each pair of no-kick vertices: 0.43
  - W bit 2 (value 7 vs 3): 0.67
  - parity XOR bit 2: 0.50

  Current code: restart parity + BFS orientation unification over shared edges (matched by position) + flip each
  connected piece so its normal sum points +Y → 0.95. Walls and vertical pieces stay ambiguous. Question: what do
  W values 3/7 encode (perhaps a GS PRIM/CW flag the VU program uses)? The VU microprogram is uploaded by the ELF
  (MPG) and is the authority.
- **Dark meshes in the app.** Hypothesis: the course is prelit (vertex colours) and the viewer's normal lighting
  darkens the mis-oriented faces. `idss_model.h` now sets `material.unlit=true`, but the app still showed
  "Lit (diffuse)", probably a stale build; not verified.
- **Flat-colour look.** The textures are assigned (e.g. KAGI_DAY003_000, 128×64). UV range on crs01 is
  −14.7 … 24.0. Check whether the UVs need a scale (texel units?) or are paired with the wrong texture, and confirm
  the V orientation.

## Suggested independent checks
1. Disassemble the VU1 microprogram (MPG upload in the ELF) and read how it uses W bits 2/15 and the UV input
   (ST vs STQ, scaling).
2. Export one record (e.g. crs01 record 0, texture index 0) to OBJ with UVs, and open it with its GIM in an
   external viewer.
3. Rebuild cleanly and verify `material.unlit` reaches the scene (the Material tab should not say
   "Lit (diffuse)").

## 2026-10-26 Codex audit supersession

Read the independent audit in README.md before following the earlier hypotheses above. Saved source did not contain the described masked-UV fix, BFS winding or unlit flag. Masked UVs, named LOD isolation and standard GIF strip parity are now implemented; generic unlit base-texture preview is explicit, not full vertex-lighting reproduction. Runtime winding/basis and source colors/fog still need validation. Current build/test status is recorded in COORDINATION-PLAN.md.

Dedicated current implementation handoff: [Claude / Initial D SS](../../CLAUDE-INITIAL-D-SS-HANDOFF.md). It records installed corrections, validation and remaining visual/material work; read it before following historical hypotheses.
