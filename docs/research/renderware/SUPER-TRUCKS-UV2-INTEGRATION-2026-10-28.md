# Super Trucks UV2 integration validation - 2026-10-28

Production change: src/rw_core.h nativeMesh now reads V4-32/address1 as primary XY Float32 UVs.
Address0 remains position/ADC; other V4 addresses are not admitted as UV. Existing V2 UV path
is retained. UV count must equal position count for every contributing batch; nonfinite values,
mixed V2/V4 formats within one batch and truncated UNPACK invalidate UVs but preserve geometry.
Part.hasUv now requires complete coverage across emitted batches, rather than any nonempty stream.

## Actual production decoder corpus

Every default candidate was scanned and decoded via RenderWare::decode, with source snapshot
validation and the original image binding. Actual non-null surface textures counted below.
Optional geometry/weather/livery alternatives excluded as in Render All. All TXD entries independently
decoded, including optional images. This is primary base-texture coverage, not original shader fidelity.

| File | Textured surfaces | Textured / default triangles | TXD decoded | Mesh / image failures |
|---|---:|---:|---:|---:|
| ALBACETE.MIX | 70/70 | 52,136/52,136 | 236 | 0/0 |
| ALPI.MIX | 84/84 | 21,489/21,489 | 203 | 0/0 |
| BARCELL.MIX | 77/77 | 20,138/20,138 | 219 | 0/0 |
| BERLINO.MIX | 81/81 | 36,177/36,177 | 231 | 0/0 |
| CARSEL.MIX | 6/6 | 892/892 | 7 | 0/0 |
| CORSICA.MIX | 78/78 | 23,853/23,853 | 204 | 0/0 |
| COSTA.MIX | 110/110 | 28,372/28,372 | 277 | 0/0 |
| ENVSPHER.MIX | 1/1 | 809/809 | 1 | 0/0 |
| HIGH.MIX | 61/61 | 27,884/27,884 | 182 | 0/0 |
| STADIO.MIX | 50/50 | 17,561/17,561 | 167 | 0/0 |
| TOSCANA.MIX | 84/84 | 17,819/17,819 | 259 | 0/0 |
| VALENCIA.MIX | 73/73 | 17,281/17,281 | 251 | 0/0 |

All ten circuits and two ancillary resources attain 100% of default surfaces/triangles assigned
a decoded base image. Geometry triangle counts are unchanged. All 2,237 images decode; zero
archive warnings. The former 72%-95% coverage report is superseded for default scenes.
Non-default alternatives are NOT covered by that 100% claim: ALBACETE across all variants has
135/137 surfaces and 101,577/128,409 triangles textured; two optional groups remain untextured.

## Regression evidence

Focused window suite: 8 passed/0 failed/no skips; new synthetic renderWareUv2Packets plus
RenderWare clump/material/name-scope/PS2 image/compressed MIX/actual corpus checks. Synthetic
UV2 verifies XY selection, address2 exclusion, count mismatch, NaN, truncation and per-part
multi-batch completeness. D1GP A31 and HARE_USA retain 16,275/40,239 triangles and their existing
16,184/40,047 textured triangles. Log build/windows-local/supertrucks-uv2-focused.txt.

Private reproducible production corpus harness: workspace supertrucks-research/coverage.cpp,
coverage.cmd, production-uv2-coverage.private.json. No private audit/prototype header used in
the production measurement. Source SHA256 receipts remain in the prior corpus report.

Full suite/build/live receipt will be recorded in COORDINATION-PLAN.md. Secondary UV semantics,
blending, reflections, runtime material fidelity and Selene car-part placement remain separate.

## Final build and live receipt

Full window suite 129 passed/0 failed/86 optional skips; development executable rebuilt and
installed after owner closure. Live BARCELL normal scan + Render All: 77 surfaces, 20,138
triangles, 76 loaded +1 existing, zero failed/omitted/texture warnings. Left open.

Visual limitation found: red/dark facades remain. Independent production EDIFICIO_1 raster
is a coherent non-red facade, primary UV range0..1; alpha=0 on28,837/32,768 pixels.
ASFALTO1 also has varying alpha. The all-TXD alphaMask=true policy may conflate effect masks
with coverage; this is a hypothesis to verify against material/render states, not an applied
fix. Full shader/alpha fidelity is NOT validated. Next: material alpha/blend semantics and
visual reference. No source game files modified, no release/publication.
