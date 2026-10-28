# Super Trucks track coverage - 2026-10-28

Read-only validation with the current production SeleneMix/RenderWare scan, TXD decode and mesh decode.
Source: F:/downloads/Super Trucks Racing 2/Super Trucks Racing 2/Tracks.
No application code changed. Each file used its own shared decoding session. All default
candidates were actually decoded, with actual non-null surface images counted; optional weather/
SLF/BSP/livery alternatives were excluded using the same flags as Render All. All TXD records
in each archive were independently decoded, including optional banks.

Coverage is textured triangle count / decoded default triangle count, not original shader fidelity.
Surface counts refer to recovered material partitions, not a complete game material manifest.
hasUvs is conservative across contributing parts: one missing batch can disable a whole material surface.
No live Vulkan/game visual equivalence or exhaustive all-alternative mesh decode is claimed.

| Archive | Default textured surfaces | Textured / default triangles | Triangle coverage | TXD decoded | Mesh failures |
|---|---:|---:|---:|---:|---:|
| ALBACETE.MIX | 61/70 | 43,974/52,136 | 84.34% | 236/236 | 0 |
| ALPI.MIX | 81/84 | 17,156/21,489 | 79.84% | 203/203 | 0 |
| BARCELL.MIX | 66/77 | 14,524/20,138 | 72.12% | 219/219 | 0 |
| BERLINO.MIX | 68/81 | 30,557/36,177 | 84.47% | 231/231 | 0 |
| CARSEL.MIX | 6/6 | 892/892 | 100.00% | 7/7 | 0 |
| CORSICA.MIX | 74/78 | 18,461/23,853 | 77.39% | 204/204 | 0 |
| COSTA.MIX | 108/110 | 23,894/28,372 | 84.22% | 277/277 | 0 |
| ENVSPHER.MIX | 1/1 | 809/809 | 100.00% | 1/1 | 0 |
| HIGH.MIX | 59/61 | 22,103/27,884 | 79.27% | 182/182 | 0 |
| STADIO.MIX | 46/50 | 16,754/17,561 | 95.40% | 167/167 | 0 |
| TOSCANA.MIX | 82/84 | 13,657/17,819 | 76.64% | 259/259 | 0 |
| VALENCIA.MIX | 70/73 | 13,595/17,281 | 78.67% | 251/251 | 0 |

CARSEL and ENVSPHER are selection/environment resources, not full circuits. ALBACETE is the control
and reproduces 43,974/52,136 (84.34% rounded to two decimals; older notes said 84.35%).

## Findings

- All 12 archives have zero archive warnings, image failures or default mesh decode failures.
- All 2,237 TXD rasters decode. Nine additional circuits account for 1,993 rasters.
- Recovered PS2 image encodings: indexed 4-bit PSMT4, indexed 8-bit PSMT8, 32-bit PSMCT32 and 16-bit PSMCT16S.
- Every untextured default surface has a texture binding but hasUvs=false. The next blocker is native UV/batch coverage, not missing images.
- Same broad MIX/Selene/RenderWare/TXD pipeline works across all these fixtures; this is not proof every format variant in the game is supported.

## Untextured default surfaces (all have an image binding; UV coverage missing)

### ALBACETE.MIX

- ALBASFALTO01: 2,098 triangles; TRACKS/ALBACETE/PS2/ALBACETE.SLI.
- ALBASFALTO02: 222 triangles; TRACKS/ALBACETE/PS2/ALBACETE.SLI.
- ALBASFALTOBOX: 28 triangles; TRACKS/ALBACETE/PS2/ALBACETE.SLI.
- ALBERBA02: 1,997 triangles; TRACKS/ALBACETE/PS2/ALBACETE.SLI.
- ALBERBA03: 1,464 triangles; TRACKS/ALBACETE/PS2/ALBACETE.SLI.
- ALBERBA04: 1,236 triangles; TRACKS/ALBACETE/PS2/ALBACETE.SLI.
- ALBMURETTO: 133 triangles; TRACKS/ALBACETE/PS2/ALBACETE.SLI.
- ALBSAND: 326 triangles; TRACKS/ALBACETE/PS2/ALBACETE.SLI.
- ALBSAND_BORDER: 658 triangles; TRACKS/ALBACETE/PS2/ALBACETE.SLI.

### ALPI.MIX

- AL_ASFALTO: 1,475 triangles; TRACKS/ALPI/PS2/ALPI.SLI.
- AL_ASFALTOTUN: 185 triangles; TRACKS/ALPI/PS2/ALPI.SLI.
- AL_ERBA: 2,673 triangles; TRACKS/ALPI/PS2/ALPI.SLI.

### BARCELL.MIX

- ASFALTO1: 410 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- ASFALTO_ISOLATO: 1,517 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- EDIFICIO_1: 1,548 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- EDIFICIO_2: 262 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- EDIFICIO_3: 145 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- EDIFICIO_7: 312 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- EDIFICIO_8: 142 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- EV1EDIFICIO: 591 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- EVENTO_2: 75 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- GAZEBINO: 366 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.
- GRATTACIELO1: 246 triangles; TRACKS/BARCELL/PS2/BARCELL.SLI.

### BERLINO.MIX

- BANK1: 116 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- BANK2: 3 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- BANK6: 12 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- BER_ASFALTOGRANA: 487 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- CENTROCOMM1: 342 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- GRATTA1: 12 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- GRATTA7: 89 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- HOCERBAX: 488 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- MARCIAPIEDE1: 1,781 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- PARCO: 2,076 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- UFFICIO1: 66 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- UFFICIO2: 108 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.
- UFFICIO3: 40 triangles; TRACKS/BERLINO/PS2/BERLINO.SLI.

### CORSICA.MIX

- CORASFAL1.1: 1,698 triangles; TRACKS/CORSICA/PS2/CORSICA.SLI.
- CORERBA1.1: 3,074 triangles; TRACKS/CORSICA/PS2/CORSICA.SLI.
- CORTUNNEL2.1: 235 triangles; TRACKS/CORSICA/PS2/CORSICA.SLI.
- CORTUNNEL3.1: 385 triangles; TRACKS/CORSICA/PS2/CORSICA.SLI.

### COSTA.MIX

- CAZASFAL1.1: 2,111 triangles; TRACKS/COSTA/PS2/COSTA.SLI.
- CAZERBA1.1: 2,367 triangles; TRACKS/COSTA/PS2/COSTA.SLI.

### HIGH.MIX

- ASFALTO1: 1,889 triangles; TRACKS/HIGH/PS2/HIGH.SLI.
- ERBA_COLLINE: 3,892 triangles; TRACKS/HIGH/PS2/HIGH.SLI.

### STADIO.MIX

- ASFALTO1.1: 384 triangles; TRACKS/STADIO/PS2/STADIO.SLI.
- CEMENTO: 31 triangles; TRACKS/STADIO/PS2/STADIO.SLI.
- STDERBA: 366 triangles; TRACKS/STADIO/PS2/STADIO.SLI.
- STDTERRA1.1: 26 triangles; TRACKS/STADIO/PS2/STADIO.SLI.

### TOSCANA.MIX

- TOSASFAL1: 1,942 triangles; TRACKS/TOSCANA/PS2/TOSCANA.SLI.
- TOSERBA1: 2,220 triangles; TRACKS/TOSCANA/PS2/TOSCANA.SLI.

### VALENCIA.MIX

- VALASFAL1.1: 1,813 triangles; TRACKS/VALENCIA/PS2/VALENCIA.SLI.
- VALCORDOLO1.1: 147 triangles; TRACKS/VALENCIA/PS2/VALENCIA.SLI.
- VALERBA1.1: 1,726 triangles; TRACKS/VALENCIA/PS2/VALENCIA.SLI.

## Source receipts

- ALBACETE.MIX: 20722469 bytes; SHA256 da5864b0b0caff7ae0aed1d64b92fccc95069e79bdbfd4e1319fa47e2597fb56.
- ALPI.MIX: 18018828 bytes; SHA256 046aa5a5fb41252162fa4a6a92382e10a1eeda87e58ff73adaa70b1107c456e9.
- BARCELL.MIX: 22941080 bytes; SHA256 e3e3a7ec4091da136e0b938733e3fd287a3f58352037b1148e2c34a3957c40c6.
- BERLINO.MIX: 23372486 bytes; SHA256 a65e6874d8c637b931dd7c0eaf7382dce79c13f633428f83d0bed965c787596c.
- CARSEL.MIX: 338131 bytes; SHA256 863e72d7ae0924025ab422e63975cd4818feccb90dba6baaa587d97d03f9f55e.
- CORSICA.MIX: 16900446 bytes; SHA256 43538a2a0dd8f9c160656b10a66b45aecf12afa2f0e413bf0a1025f7340b2ded.
- COSTA.MIX: 28374522 bytes; SHA256 3bd5f5e43e700f200259c5e8a14afa62cb589e5434a5d996462758ae23329519.
- ENVSPHER.MIX: 44431 bytes; SHA256 fc0b92b87292e2202e65dddb3bf22f115b4450a0e4c03e647f0dc7a5aafaae3d.
- HIGH.MIX: 19048561 bytes; SHA256 e5a35a9038c3bedc96af70b369d796ddcd384c421d5a317779e2580900e8d743.
- STADIO.MIX: 7832143 bytes; SHA256 46648356035721387592e3b352c953680c04ac4f166aa599ec24159511480c98.
- TOSCANA.MIX: 22262403 bytes; SHA256 ae8590305e34850ba840ced7a77e4bf7fd297db7ef927a980fefa719fa1c6647.
- VALENCIA.MIX: 16599985 bytes; SHA256 b07518352f679276f26e7704c91c343ed36d2c2797220330879e2506c2fffed8.

Private reproducible harness and full JSON: workspace supertrucks-research/coverage.cpp, coverage.cmd,
track-coverage.private.json. Harness uses current repo headers plus binary_reader/document sources;
no stub reader, legacy extractor or guessed bank linkage. Build log supertrucks-research/build.log.

Next owner: Codex native decoder, compare native VIF UV layouts on BARCELL (lowest current coverage)
and STADIO (highest circuit coverage), then remeasure the same default-surface denominator.
Preserve original per-part UV presence and avoid a global UV flip.
