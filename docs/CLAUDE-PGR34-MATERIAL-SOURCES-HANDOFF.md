# PGR3/4 material-source integration handoff

Updated:2026-10-27. Owner:Codex for this slice; next native binding/shader work remains open. Follow AGENTS.md and COORDINATION-PLAN.md. Preserve all unrelated uncommitted work; no commit/push/release requested.

## Scope

Integrated source evidence and explicit preview selection after the XEX audit. This is not original shader emulation, a new automatic diffuse-map heuristic, or a global UV/texture flip. Source channels/reference positions are not GPU sampler registers. Game shader application remains false.

## Implementation contract

- pgr34_track.h: Material fields retain raw bytes, declaration type/usage/index and decoded-SCNE offsets. materialEvidence walks root/aliases/layers with256-node bound and visited set. Nodes preserve edges/fields, maps preserve material/channel/resource/slot and an unresolved role; absent targets warn. Source identities include material path identity and reference coordinates, not only image ID.
- Track explicit pgrMaterialSource must match a reachable map key. Other keys fail; automatic selection remains existing channel0/alias behavior and neutral unresolved composites. BC5 is allowed for explicit inspection. Selected-map preview is unlit with original UVs. Pixel cache/limits unchanged. materialEvidence/materialMapChoice/materialPreviewMode/materialSourceCandidate live in sourceBindings and accompany exports.
- pgr34_geometry.h: available vehicle references are exposed as vehicle:0…3 keys; original sourceTextureSlots remain. Explicit choices validated against actual references. All maps stay in the 2D library; automatic single-map fallback unchanged. No role mapping is inferred from the executable sampler numbers. Old decalLayerApproximation is a legacy preview flag, not proof of a semantic binding.
- mesh_preview_registry.h: source-map choice is material state, excluded from geometry key. Changed choice or explicit target requests refreshMaterial on the existing record; a temporary pgrMaterialTarget selects the exact mesh even among transformed duplicates. Reapplying/restoring also refreshes correctly. Ordinary scan duplicate behavior retained.
- main.cpp: sourceMaterialRequested verifies active entry, stale flag, original document path and no running scene batch; adds ephemeral target ID and rejects an unregistered target instead of adding duplicate geometry. Native callback replaces data/material while preserving placement/rotation/scale. Stored source candidate strips both explicit map selection and target ID for Automatic. Unsupported/foreign requests report errors. No script/game executable runs.
- mesh_collection.cpp / vulkan_panel.h: Material > Source maps… tool dialog follows active selection, shows table and raw evidence, Preview source map/Automatic. Original blend/atlas limitations visible. No additional per-mesh image copies or renderer/material class layout changes.

## Verified

Build pgr-source-final-build.log succeeds; dev exe updated and open with Ferrari reference. Actual corpus8/0/no skips; full window120/0 with86 optional corpus skips; reader45/0/no skips. Final logs build/windows-local/pgr-source-{corpus-final,full-window,reader}-tests.txt. Native full Tokyo report build/pgr-source-validation/pgr3-sunny-full.json:11,785 meshes/10,097 textured/0 failed, unchanged. Car full UI195/158 meshes and118377/78771 faces; all reference car source maps inspected in tests. Track manual base preview pixels exactly equal automatic base pixels on both games; geometry identical; invalid keys rejected. Registry test covers cycles/missing graph targets, duplicate copies, transformed target and reapply.

Live Ferrari:195 meshes/118,377 triangles, source material82/ref3 preview changed texture33(256×128) to9(128×64), Unlit, then Automatic restored33/Lit without changing counts. Private screenshot build/pgr-source-validation/pgr-source-inspector-live.png. Appearance still has known atlas/layer approximations.

Intermediate UI corpus failure was a test assumption: sorted auto-preview first loads an untextured tyre, while native source-order first has maps. Final test explicitly picks a source part with maps before exercising preview/restore. Do not bypass map absence or force a random texture onto the tyre.

## Next work

Prove material group/declaration→CPU object→shader variant→resource binding and UV/crop constants for regular scene, legacy road, Gotham4 road and car diffuse/overlay. Add verified per-family descriptors/tests, then original blending/atlas rendering. Current1,536 layered Tokyo composites remain unresolved;152 other base maps missing/unsupported. No improved coverage percentage is claimed. Keep sampler evidence (base s0/s8/s5 depending shader) separate from source channels. Use the new inspector to inspect actual source branches; unknown roles remain unknown.
