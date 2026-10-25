# Native Genki material integration

Updated: 2026-10-25. Implemented in the development checkout, based on the capture checks and source records. Public release 0.3.6 remains unchanged.

## Implemented

### TXRD2 March

- Native bounded reader for the observed headerless Indexed8 picture bank, preserving all six original slots. Three stored images are available to the 2D scanner. Three reference-only records stay unresolved and are never read as pixel payloads.
- Primary v1 material lookup from the namespace/slot at prefix +68/+70, checked against the primitive TME bit (+72), CLAMP (+88), declared dimensions, bank bounds and validated topology. No runtime relocation constant or nearest-texture guess.
- V4-16 primary UV support at address 0x8003, using the first pair with an 8-byte record stride; V2-16 keeps its 4-byte stride. The capture confirms 342/342 primary pairs in the first wide-UV body group; zero of its 650 secondary pairs match the primary atlas. Additional body groups and the 256×256 map have corroborating matches.
- March native probe: 54 parts / 8,328 triangles retained, 19 parts assigned their primary texture. Scan/Render All, material texture Preview, RAM-backed image decoding and model export use the common native path.
- Verified zero-minimum, full-image CLAMP U/V is carried to Vulkan and exported GLB/FBX sampling, preventing border UVs from repeating. Surface texture names retain the original image slot even though the image decoder internally adapts one validated record to a one-picture TIM2 buffer.

### TIM2 RGB24 and TXR Zero

- RGB24 TIM2 images/palettes use three bytes per colour rather than RGBX8. The captured Hakone RGB24 storage case supports this correction; synthetic direct and indexed RGB24 decoding is checked.
- TXR Zero transfer-form admission includes observed 16-pixel dimensions and RGB16 CLUT storage with bounded palette spans. Unknown flags/layouts still fail, and every requested indexed byte must have been written by the transfer reconstruction.
- The AE86 candidate 0x00000014.dat decodes all 42 images through the production TIM2 decoder. Native material inspection identifies 26 supported local primary references among 372 candidates. This is not a claim that all variants/LODs should be displayed together or that all game shader stages are reproduced.

## Important remaining limits

- March alias dependencies, secondary/reflection layers, exact part/LOD visibility, paint/tint and full game shader behaviour are unresolved.
- Existing C1/TXR topology profiles remain isolated; no face-generation rule was generalized or changed.
- RGB24 fixes the common image decoder; it does not establish new cross-file material bindings for Nikko/Hakone or reconstruct their entire scenes.
- Genki source file numbering remains build-specific. The Europe Alto and USA fixture distinction from the research report still applies.
- No application dependency on PCSX2, its captures, NumPy or QuickBMS was added. Captures are verification references only.

## Validation

- Development application built and updated successfully at build/windows-local/binary_workbench.exe.
- Reader suite: 45 passed, 0 failed.
- Full window suite: 96 passed, 0 failed, 75 optional checks skipped.
- Fixture-enabled Genki run: 14 passed, 0 failed, 0 skipped; includes the actual scan/material texture Preview path, all three RAM-backed image previews, UV stride checks, malformed bank/alias checks, RGB24 image/palette cases, mode/face control isolation across 23 files and GLB/FBX export.
- March GLB independently inspected: 54 meshes, 8,328 triangles, 19 textured materials, 3 distinct embedded image payloads with dimensions 512×512 / 256×256 / 128×128. Sampler U/V is GL_CLAMP_TO_EDGE (33071).
- TXR Zero AE86 decoder probe: 42/42 TIM2 images decoded successfully; 26 local material references reported by native inspection.
- No source game file was modified. No new public release or Git push was performed in this implementation step.
