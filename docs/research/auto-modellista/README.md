# Auto Modellista PS2 — analysis, 2026-11-16

Current status: [native track reader integrated and tested](NATIVE-READER-2026-11-16.md). Earlier research-only/AHI-pending statements below are historical; cars and original shader effects remain pending.

Follow-up: [regional comparison and render diagnosis](REGIONAL-COMPARISON-2026-11-16.md) supersedes the all-model car preview: strips have additional native evidence, but car selection and paired AHI assembly are missing.

Material follow-up: [primary binding and textured exports](MATERIALS-2026-11-16.md), including measured image availability and remaining shader/AHI limitations.

Status: research prototype only. No 9XD reader, build, install, push or release in this session. Do not advertise native support yet.

## Evidence and source paths

- Study case: `C:\Users\Jarky\Documents\study cases\automodelista\car044.bin`, 288,360 bytes. SHA256 `0e09a0ab15a12f258a8adaed34235a49e3f94173f544322bd4e99b5dc0abf42b`; byte-identical to extracted USA game member.
- Full root: `F:\copia de seguridad 08.04.2019\downloads\Auto Modellista (USA)\Auto Modellista (USA)`. `afs00.afs`, `afs01_pu.afs`, `afs02.afs`; extracted `afs unpack 2` has 745 files (736 BIN, 9 ICO). `afs unpack 1` contains ADX audio; `stage_01.adx` is not a geometry sample. `afs unpack 3` contains SFD movies.
- `SLUS_206.42` retains ELF symbols: `Meltw` VA0x263bb0 (200 bytes), `load_car_data` VA0x2236b0, `get_data_address` VA0x1c9d90, AMO readers at0x13e640..0x140a1c. The executable is the primary source of the recovered layouts. No game binary is included here.
- A web search found https://github.com/apexJCL/automodellista-recomp but its README targets Xbox. It is not evidence for PS2 formats; no code used.

## Compression — verified against original instructions

`Meltw` is a word-oriented LZ stream: 16-bit LE flags, MSB first. Clear bit copies one literal u16. Set bit consumes a u16 token: upper5 bits are word count, lower11 word distance. If upper5 is zero, full token is distance and the next u16 is extended count. Distance zero writes zero words; distance/count both zero terminates. Nonzero distance copies overlapping words from previous output. Production must reject truncation, out-of-range references, invalid zero-length copies, and enforce budgets/cancellation. There is no magic, so decompression success alone is not format identification.

`meltw.py` includes a bounded research implementation. Original MIPS instructions were independently run with Unicorn MIPS64 on car044 and produced identical853,440 bytes, consumed all288,360 bytes. Decoded SHA256 `82d272be2a7f637bad0eae51591b121a22fd3ab5eebfecad8568efcdcbeac2bb`. Four malformed/budget cases rejected; overlap, zero-fill and extended-count fixture passed. See `meltw-validation.json`.

## Container and AMO geometry

Decompressed outer file starts with LE u32 offsets; first offset is table size, unused slots are0xffffffff. `get_data_address` reads `base + offsets[index]`. AMO roots start `(type=1, child_count, byte_size)`. Each child has `(type, count, byte_size)`; next sibling = current + byte_size, data at+12. Directory2 contains model records type4. Root9 contains materials, root10 texture records; exact fields/linking still pending.

Model attributes recovered from named executable getters:

| Type | Payload |
|---|---|
|5|Index-list directory; descriptor records0x30000 /0x40000 seen|
|0x50000|Material-list data; semantics pending|
|0x60000|Per-primitive material-index data; needs full trace|
|0x70000|count × float32 XYZ, stride12 (`GetVertexAMOModelMesh` VA0x13f7e0)|
|0x80000|Normals, stride12|
|0xa0000|float32 ST/UV, stride8 (`GetStAMOModelMesh` VA0x13f880)|
|0xb0000|Vertex color data, stride16|

Index descriptors use count primitives; each primitive is u32 count/cull flag (count &0x7fffffff) followed by that many u32 vertex indices. Executable `GetPrimVertexNumAMOModelMesh`/`GetPrimVertexIndexAMOModelMesh` confirm masking/stride; `ConvertModelMeshAMO_NormalModel` stores counts and cull type separately. Preliminary OBJ triangulates each primitive independently as a strip, preserving boundaries; full draw-path topology/winding and descriptor distinction must still be verified. No Genki flags reused.

## Measured prototype results

|Sample|Decoded bytes|Models|Vertices/UVs|Provisional strip triangles|Embedded TIM2|
|---|---:|---:|---:|---:|---:|
|car044.bin|853440|202|9945|8344|4|
|car000.bin|887680|202|10562|8357|4|
|car010.bin|826816|202|9427|8041|4|
|stg01_m0.bin|1662016|90|28261|15048|0|
|stg01_m1.bin|1594432|68|27481|14732|0|
|stg01_m2.bin|1721088|85|29771|16130|0|

All six consume entire compressed streams, bounded nested model records, finite XYZ and indices within each source vertex array; UV counts match vertex counts. Counts include alternative assemblies/parts, NOT a validated default car or complete placed map. `geometry-report.json` retains per-model results. Preliminary car044 first assembly:36 meshes,1952 provisional triangles; `car044-preview.png` shows recognizable car shape but is NOT a correctness certificate (no materials, transforms, LOD/variant filtering; suspicious surfaces remain).

## Textures and materials

car044 has TIM2 at decoded0x9c240,0xac740,0xbcc40,0xcf0c0. The earlier compressed literal hit239636/0x3a814 was NOT a safe standalone texture offset. Header format1 uses image header at+128, not+16. First three images256×256, last64×64; image type5 indexed8, palette256 entries RGBA32, one mip,128-byte picture headers. Image/palette sizes65536/1024 and4096/1024. Need reuse existing TIM2 decoder and inspect GS palette/layout before claiming correct appearance.

Track `stg001.bin` is primarily a texture bank (49 images), `stg001_q.bin`49 (bounded extraction corrects the earlier signature count); geometry lives in `stg01_m0/m1/m2.bin`. Exact texture-bank choice, material indices and map placement require executable tracing, not filenames alone. Material binding coverage is unmeasured, not0% or100%.

## Next work / handoff

- [x] Locate actual full assets and executable; identify why generic raw VIF scanning fails.
- [x] Recover Meltw; byte-exact original-instruction comparison and bounded fixtures.
- [x] Recover AMO vertex/normal/UV/index records; six research probes, provisional OBJ.
- [ ] Codex: confirm descriptor0x30000/0x40000, cull/winding and render-path primitive rules.
- [ ] Codex: trace `mkModelCAR`, `car_create_clay`, `mergeCarClay*`, hierarchy/variants/LODs/wheels; map texture/material lists via `plAMOGetMaterial*`/`plAMOGetTexture*` and `create_tex`.
- [ ] Codex: validate track sector assembly/transforms and exact bank/context references on at least one track.
- [ ] Codex/native integration: bounded Meltw + structural AMO recognition, direct mesh results from Scan for 3D Data, material/TIM2 export, cancellation/budgets, malformed fixtures and source regressions. Keep raw TIM2 literal scan from accepting compressed garbage.
- [ ] User: visual comparison in-game after a textured, correctly assembled default model exists; no app launched this session.

Reproduce prototype: `python probe.py --output OUT PATH_TO_EXTRACTED_CAR_OR_STG_M_BIN ...`. Standard Python only; OBJ is geometry-only and provisional. App code unchanged; no full app regression needed or run for documentation/research scripts.
