# Blur PC scene texture linkage

2026-11-17. Codex. Native focused validation passed; development executable updated.

## Linkage

Draw primary vertex binding references a typed material parameter block. Type7 g_tAlbedo or g_tTexture is a local RenderList texture slot. Serialized table +0xb0 maps that slot to(resource group,texture index), as confirmed by original resolver0x6ce5e0. The catalog uses directory-ordered base and update resources, including texture-only records after buffer groups and zero-count inherited updates. No nearest-name, global hash or guessed texture ordering.

The geometry parser now reads all three reference tables +0xa0/+0xa8/+0xb0. Materials retain the exact texture field/slot. Decoder rederives links from the current source and paired directory, decodes referenced BC1/2/3 data and attaches image/name/provenance to MeshSurface. Base-colour preview is unlit; full original GPU shader remains unapplied.

## Necessary corrections

1. Strict texture-only inspection stopped at mixed buffers and empty update records. Shared resourceBuffers reader now allows catalog traversal of the mixed block prefixes. Texture-only inspection behavior is preserved separately.
2. RA/version alone was too weak as an image-padding successor: group102 light_paint_01 had two plausible aligned offsets because DXT bytes resembled a frame. Require complete12-byte image/resource RA/mode prefix (or bounded terminal zero padding), yielding a unique payload. Never select the first plausible offset silently.
3.130 admitted sections use declaration type9 with8-byte UV attributes, while the first geometry subset only decoded type8 half2. Type9 samples decode coherent half4 UV channels; XY are primary UV0, ZW retained in source as additional channels not yet exposed. Float32 interpretation produces tiny invalid values. Decoder reads first two half components for both types, respecting declared offsets/stride and source orientation.

## Coverage and limits

Independent sequential research:173/173 accepted static showroom sections reference a decodable primary image;96 unique referenced identities out of217 catalog identities. Examples: section0 lighteffect_diff;33 yellowline01_diff;70 b12_buildingbrick_diff;150 tarmacdark_diff;172 texatlas_0. This is base-colour availability for the accepted static subset, not the percentage of complete game geometry/material effects.

Largest decoded variant is chosen explicitly for preview. Original runtime streaming/LOD/mip variant choice unverified. Nonnegative local groups only; global scene images are not guessed. Original alpha/additive modes, normal/specular/detail layers, vertex colour, shader UV animation and other packet kinds remain pending. Existing173 sections/36,750 faces remain the geometry baseline.

Tests/build pending at initial write; final receipt appended after actual execution. Game files and decoded pixels stay outside repo. No interactive application launch, commit/push/release.

## Final validation

-16 passed/0 failed/0 skipped,20,296ms. build/windows-local/blur-scene-material-tests.txt. Entire accepted173-section Render All result has actual MeshEntry images and verified original identity links; still36,750 faces, no duplicate entries on repeat.
- Catalog217 identities and all96 referenced primary images decode. Representative decoded names match independent research: lighteffect_diff, yellowline01_diff, b12_buildingbrick_diff, tarmacdark_diff, texatlas_0. Tests reject damaged texture padding and canceled traversal; geometry corruption/stale identity/RAM context plus previous static Blur regressions pass.
- Development EXE rebuilt after main.cpp timestamp-only dependency workaround. Material preview marker and SHA receipt checked: build/windows-local/blur-scene-material-receipt.json. No interactive app launch, commit/push/release. Full repository suite not rerun.
- Restart development build, rescan complete showroom level.levelstream with paired level.levelrender, then Render All.173/173 refers only to currently admitted static sections; full scene geometry and original shading remain pending.
