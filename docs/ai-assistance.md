# Local AI assistance

9XD connects to a separately installed Ollama runtime and local model. It does not bundle either one. Configure the local endpoint and model under **Other Tools > Settings > AI Assistance**, then open **Other Tools > AI Analysis Â· Ollama** and connect.

The assistant receives application guidance and compact active-document metadata; current decoder settings and collection summaries are available through tools. It does not automatically understand or upload an entire binary. Scripts and arbitrary disk files are not exposed. Each chat belongs to its document, including when modules are detached to another display.

## Reading and preparing decoders

- **Allow reading document bytes** exposes bounded reads of the active document, including unsaved changes: at most 512 bytes per call and 2,048 bytes per question.
- **Allow filling 2D / 3D settings** exposes `set_image_settings` and `set_mesh_settings`. These fill existing controls and return a success/error result. Uncheck this option for analysis without setup changes.
- **Allow AI calculations** lets the assistant generate output when requested. `decode_asset` discovers supported metadata, chooses the largest supported candidate by default, applies exact settings and runs the existing decoder. `run_decoder` runs controls already prepared from known values. The model receives the actual completion/error result before answering. Uncheck this option to use the native scan inspectors and decoder controls manually. Filling settings alone still does not calculate or render.
- Image setup selects the active binary document as source. A setup replaces the recipe; omitted optional fields use defaults. Palette and image ranges are checked before any controls change.
- Mesh setup validates counts, formats, strides and source ranges. It resets coordinate transforms to identity, manual delta to zero, and destination to **Add to collection**. UVs, normals and binary placement must be explicitly enabled when needed. Index values and actual coordinate values are checked by the normal decoder when you render.
- A document edit stops the current query. Stopping a query does not undo settings already filled; applied/rejected actions appear in the transcript.

Unknown formats still require investigation. Give the model known settings or small, meaningful header ranges; it should ask for missing information rather than invent dimensions, vertex strides or palette offsets.

## Synthetic PS2-style sample

Open `hangar_unit.bwps` from the separate synthetic test pack. These values refer specifically to the original generated fixture, not to arbitrary PS2 files.

For the reactor texture, ask:

> Fill the 2D settings using set_image_settings: offset "4096", width 128, height 128, format "PS2 RGBA8", layout "PS2 GS pages", ps2Alpha true. Do not calculate.

Review the values and use **UV/2D > Scan for Textures / Images**, or Calculate / Add to Library in the image calculator. The app opens **UV/2D > Image Preview** and the calculator/collection panels, decodes the texture and adds it to the library. You can also use **Calculate / Add to Library** in the calculator.

For the body mesh, ask:

> Fill the 3D settings using set_mesh_settings: vertexOffset "133120", vertexCount 24, vertexStride 32, positionType "Float32", positionScale 1, indexOffset "135168", indexCount 36, indexType "UInt16", indexStride 2, topology "Triangle list", uv true, uvOffset "133144", uvStride 32, uvType "Float32", normals true, normalOffset "133132", normalStride 32, normalType "Float32", deltaFromBinary true, deltaOffset "137216", deltaStride 4, deltaType "Float32", deltaScale 1. All byte orders little-endian. Do not render.

Review **3D > 3D Settings**, then use **3D > Scan for 3D Data**, or **Render Current File** in the settings panel. The app opens the 3D view/settings/collection panels and reports mesh loading success or the decoder error. The local body geometry is placed at Y=3 using the binary delta. The test pack's answer key lists the other parts and textures. Texture assignment to a mesh remains manual.

## Replies and persistence

Answers default to **Finding / Evidence / Next step**, targeting at most 80 displayed words. Model reasoning is filtered out. The internal generation budget remains separate so tool use can complete. If the model cannot produce a compact answer, the chat reports that instead of displaying a long reasoning passage.

Chat exports are manual JSON exports. Endpoint, model selected in Settings, context budget and project instructions are user preferences; chat history is not automatically restored across restarts.

File and folder opening dialogs share a remembered last directory, stored in user preferences. Opening a binary through Recent Files or another application entry point also updates the folder after a successful open. Cancelling a dialog leaves it unchanged. If a saved folder no longer exists, the nearest existing parent is used, then Documents as a fallback.

## Load tested geometry directly

For generated benchmark candidates, open the original binary, then choose **3D > 3D Settings > Load Mesh Setup...** and select its mesh-settings.json. The importer applies the exact validated fields, including explicit false values for UV/normal reading; it does not render or edit the binary. Review the fields and click **Render Current File** in 3D Settings. This avoids model transcription errors when reproducing a known test. The decoder status reports UV/normal modes, and invalid UV errors identify the vertex and source offset.


## Automatic asset discovery

Ask **“Inspect this file and render its largest supported 3D model”** or **“Show its supported texture”**. A short local structured planning step recognizes direct generation requests; the app then performs discovery, applies exact settings and waits for decoding before the assistant answers. Inspection-only questions and supplied-parameter setup requests continue through the ordinary chat tools. Discovery inspects at most 32 MiB of the active document locally, including unsaved changes. It currently recognizes bounded TIM2 surfaces and embedded GLB Float32 triangle geometry with UInt16/UInt32 indices. GLB candidates load local positions and faces; skins, materials, node transforms and packed UVs are not imported. Unsupported or oversized candidates are reported, never truncated to fit. This is not a universal game-file importer.

## 3D setup sessions

New/opened documents start with blank geometry inputs, disabled UV/normal reading and identity transforms. The test-sphere preset is removed. **Save Mesh Setup...** and **Save Setup As...** write a `.bwmesh` setup, including unfinished fields, transforms and placement; **Load Mesh Setup...** restores it. Tested flat `mesh-settings.json` recipes remain supported. Setup files do not contain the source binary or decoded mesh.

Closing/replacing a document with a new or changed 3D setup offers **Save / Discard / Cancel**. Saved or loaded setups that have not changed do not prompt again. Saving the source binary preserves the current setup.

## Structure / Formatting annotations

Enable **Annotate structures** in chat and ask: **“Create Structure/Formatting annotations for the recognized headers and geometry in this binary. Do not render.”** The app maps recognized TIM2 picture headers/image/palette ranges and embedded GLB headers/JSON/position/index streams. Repeated fields use one rule with a stride and repetition count, including streams larger than the mesh decoder's rendering limit.

`annotate_structure` uses built-in metadata discovery; `set_structure_rules` adds custom regions labelled **AI / hypothesis**, with short evidence. `get_structure_rules` reads existing rules eight at a time. New rules append to existing user rules, skip exact duplicates, validate the whole batch before applying, and open Hex Viewer > Structure/Formatting. Borders/underlines start from the theme accent and remain editable. These are visual annotations, not executable parsing templates. Export/import them through the existing formatting controls; closing with unexported rules offers a save prompt. The binary is unchanged.

## Current analysis budgets and continuation

- Generic byte inspection: 512 bytes per call, 2,048 bytes per question. Start a new question at another explicit offset to inspect the next region; applied rules remain.
- Native asset discovery: first 32 MiB, at most four supported image and four supported mesh candidates; bounded signature/header scanning. This scan runs locally and sends metadata rather than the whole file to the model.
- Chat: default 8,192 context tokens, configurable 4,096–16,384; at most five model rounds and eight tool calls per question. Reply generation budget is 1,536 tokens with an 80-word displayed-answer target. The application also estimates context size conservatively before requests.
- Mesh decoder: 100,000 vertices and 600,000 indices per decoded mesh. Retrying or reading in chunks does not change this limit. Correct splitting requires grouping whole triangles and remapping referenced vertices per mesh, or revising the decoder's memory limits. Never truncate counts.
- Formatting: 256 rules per document, up to 32 regions per AI batch. Use repeated-field rules rather than one rule per record.

A future unattended analysis queue can scan bounded windows with overlap and follow header pointers, retain a file/revision-specific index and evidence, and start fresh model contexts for each region. Only relevant summaries go to the model. Continuation must preserve offsets, record incomplete boundaries, detect document changes and allow cancellation; repeating the same limited scan or asking the model to continue without a checkpoint does not inspect new bytes. Whole-file automatic paging is not implemented yet.

## Native scan controls

The chat Calculate 2D and Render 3D buttons have been removed. Use Scan File in the Hex Viewer toolbar, Scan for 3D Data in the 3D viewport, and Scan for Textures / Images above the 2D view. Their module-owned inspectors show recognizable layouts, byte offsets, supported recipes and unsupported geometry evidence. The 3D/2D scan can preview the first supported result automatically, with a checkbox to review metadata first. Load Settings and Preview Selected remain available per result. See [Native scanning](native-scanning.md).

The assistant uses the same discovery engine and current native scan cache. Empty caches are not scan results, and document changes invalidate cached evidence. Existing permission-controlled tool calls can still decode supported recipes and report actual completion.

## Investigative requests and unsupported geometry

Requests such as **“Can you read the file and locate the 3D data?”** now start a fresh native inspection even when automatic rendering and annotation are disabled. Investigation does not render. Previous decoder status is not included as initial evidence; the assistant must distinguish a supported render recipe from evidence of an unsupported mesh layout.

The native inspector recognizes bounded Genki PS2 `0IVX00.1` / `LDMX00.2` / `MRON00.2` containers and samples up to two validated DMA/VIF position packets within the first 256 KiB of their model section. It supplies exact hexadecimal sample offsets, vertex counts and optional UV/normal stream offsets. This is sampled evidence, not a complete inventory or a Genki importer. The inspector reports the missing importer explicitly and does not invent an index buffer. Matching files can receive sampled structure annotations through the existing formatting tool.

Chat status distinguishes generic small byte reads from the native scan size. Tool-supplied offsets are already formatted for display so the model does not need to calculate hexadecimal addresses. The Genki-specific draw-suppression/parity notes derive from the separate verified sample investigation; they are not universal PS2 rules.


ARCP resource containers are recognized separately from the Genki 0IVX/XMDL profile. The tested 000000c4.arc sample exposes 3,256 validated XYZ vertices in 10 DMA/VIF packets, first positions at 0x20c0. Its scan inspector can preview the positions as points with no faces; full triangle topology/hierarchy/material reconstruction is pending. Texture filename references do not establish embedded image pixels. The assistant receives a compact summary of this evidence and must not invent raw indices or equate unsupported topology with missing geometry.

ARCP native strips: when meshEvidence.renderSupported is true, decode_asset(kind=mesh) uses the native packet importer even if meshCandidates is empty. The validated 000000c4.arc profile recovers 2,222 triangles; hierarchy/materials remain unresolved. Inspection requests do not render.

Genki native packets: renderSupported permits decode_asset(kind=mesh) to import the original DAT even with empty raw meshCandidates. Completion receipts report actual mesh results; inspection-only requests do not render. Part visibility and material binding remain unresolved.
