# Super Trucks geometry completeness correction - 2026-10-28

Supersedes earlier sparse-decoder coverage figures. The owner correctly identified missing geometry.
100% texture assignment measured previously was conditional on recovered geometry, NOT full scene completeness.

## Root cause

RenderWare PS2 native block flag1 contains a serialized DMA subroutine. Its low64-bit RET tag
(ID6, zero address) was passed to the VIF parser, which read 0x6000xxxx as S-32 UNPACK.
The bogus payload length skipped initial or entire batches. The bounded fix skips DMA low64,
includes transferred VIF tag words at+8, and stops at tag+16+QWC*16. Relocation tail bytes
are never parsed as commands. Raw flag0 path retained. Unsupported chain types remain unguessed.

BARCELL primary PS2 SLI contains558 native geometry chunks; old reader left280 empty and
emitted19,082 primary-scene triangles. Corrected reader emits all558 and29,114 triangles.
Metadata declares29,194: residual80-count discrepancy remains unproven; do not assume those
are80 missing renderable faces. Duplicate-position pruning is substantial in strip packets,
but the independent instrumentation does not isolate the precise80 difference.

## Independent geometry-header walk

Counts below are exact Geometry chunk candidates whose declared bounds/library stamp validate
inside each main PS2 SLI. Each was individually passed to the production geometry decoder,
independently of material batching and UI selection. No chunk decodes empty after the fix.

| File | Native geometry chunks | Empty | Declared triangles | Decoded triangles |
|---|---:|---:|---:|---:|
| ALBACETE.MIX | 491 | 0 | 60,996 | 60,947 |
| ALPI.MIX | 900 | 0 | 36,476 | 36,461 |
| BARCELL.MIX | 558 | 0 | 29,194 | 29,114 |
| BERLINO.MIX | 676 | 0 | 48,307 | 48,148 |
| CARSEL.MIX | 1 | 0 | 17 | 16 |
| CORSICA.MIX | 905 | 0 | 39,430 | 39,344 |
| COSTA.MIX | 1097 | 0 | 48,101 | 47,950 |
| HIGH.MIX | 1111 | 0 | 47,970 | 47,914 |
| STADIO.MIX | 341 | 0 | 23,488 | 23,472 |
| TOSCANA.MIX | 1164 | 0 | 34,605 | 34,572 |
| VALENCIA.MIX | 879 | 0 | 29,636 | 29,581 |

This demonstrates traversal of enumerated native Geometry blocks, not every Selene instance/
dynamic placement, auxiliary object type or exact game frame. A reasonable small triangle
count alone cannot establish completeness or incompleteness.

## Actual production default-set decoding

Every default scan candidate decoded with original material bank lookup; all decoded images
assigned as before. All12 archives still decode2,237 rasters with zero image failures or bank
warnings. Each file has zero default mesh decode failures.

| File | Default surfaces | Default triangles | All primary-alternative triangles |
|---|---:|---:|---:|
| ALBACETE.MIX | 101 | 62,419 | 147,057 |
| ALPI.MIX | 99 | 37,597 | 83,651 |
| BARCELL.MIX | 106 | 30,586 | 66,280 |
| BERLINO.MIX | 107 | 49,616 | 101,063 |
| CARSEL.MIX | 7 | 1,264 | 2,561 |
| CORSICA.MIX | 102 | 40,812 | 83,883 |
| COSTA.MIX | 139 | 49,418 | 102,477 |
| ENVSPHER.MIX | 1 | 896 | 896 |
| HIGH.MIX | 79 | 49,386 | 116,984 |
| STADIO.MIX | 61 | 24,944 | 50,083 |
| TOSCANA.MIX | 124 | 36,044 | 85,748 |
| VALENCIA.MIX | 103 | 31,049 | 77,089 |

## Render All selection

Previously Render All silently excluded optional weather/BSP/platform/livery alternatives.
The primary button now says Render Default Set. Scan options expose Include alternative
geometry / texture sets in Render All; enabling it changes the button to Render All and
includes optional/unplaced candidates. Heuristic evidence and child candidates already
represented by parents remain excluded. Existing scene deduplication and failures/omissions
remain reported. Including dry/rain/platform/livery alternatives can overlap geometry; it is
a diagnostic all-results view, not a correct single-weather scene assembly.

## Tests and pending live check

Focused suite11 passed/0 failed/no skips: actual BARCELL558-block/29,114-triangle check,
synthetic DMA QWC/address/truncation bounds, Render All checkbox label/dedup, UV2 contracts
and actual D1GP/CARS/ALBACETE regressions. Full suite131 passed/0 failed/86 optional skips.
Logs build/windows-local/supertrucks-uv2-focused.txt and blur-window-full.txt. Build/live pending.

Alpha/material semantics still cause visible holes/red/dark regions even when geometry is loaded.
Do not confuse material invisibility with reader omission. Full Selene part placements and game
shader fidelity remain pending. No alpha correction is guessed in this change.

Private reproducible source audit: supertrucks-research/topology*.cpp/cmd, all-topology.cpp/cmd,
all-topology.private.txt, geometry_headers.py, barcell-geometry-headers.private.json,
empty_packets.py/dma_packets.py and production-dma-coverage.private.json. Source game files unchanged.

## Installed live receipt

Application built successfully and installed after owner closure. Live BARCELL default set
loads106 entries/30,586 triangles; zero failures/omissions/texture warnings. Explicit all-results
option processes206 recognized candidates:100 new+106 existing,0 failed/omitted,66,280 triangles.
Left in Wire mode with all alternatives visible for alpha-independent geometry review. Dry/rain,
BSP and environment alternatives overlap in this diagnostic view. Material/alpha fidelity remains
open and textured view still has red/dark regions. No release/publication.
