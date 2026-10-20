# 3D workspace and FBX export

## Compact workspace

- **View**: perspective, front/back, right/left and top/bottom orthographic views, single/two/four viewport layouts, additional floating viewports, and Vulkan recovery.
- **Display**: faces, edges, vertices, normals, grid, materials/textures, the Labels inspector, and Settings.
- **Lighting**: diffuse lighting, azimuth, elevation, ambient contribution, and key intensity.
- **Frame** opens Home, Frame All and Frame Selected. Home returns to the initial perspective direction and fits visible meshes. Frame All fits visible meshes while keeping the viewing direction. Frame Selected fits the picked vertex/face, or the active visible mesh when no individual element is picked. Camera framing never edits mesh transforms. Each tile also exposes Frame through its corner menu. Drag to orbit in perspective; orthographic views keep their exact axis and drag pans. Right/middle-drag pans in either mode; wheel zooms.

Additional viewports share the document's mesh scene and selection, with independent cameras, display options and lighting. Closing an additional viewport does not remove meshes. Tiled and additional viewport arrangements are session-only, not saved in the dock layout. The four-view layout starts with Front / Top / Right / Perspective. Two vertical means side by side; two horizontal means stacked. Splitter handles resize the tiles. Each tile has its own camera menu: click the view name overlaid in the top-left corner. Split tiles share one transform toolbar in the main viewport; the selected tool works in every tile.

The Mesh Collection contains a mesh list and a compact inspector:

- **Transform**: position delta, XYZ Euler rotation in degrees, and independent axis scales. Apply commits the values; Reset restores identity. Scale is applied first, then X/Y/Z rotation, then translation, about the local origin. Negative scales are supported and correct face winding. Zero scale is rejected.
- **Material**: one diffuse or unlit surface per mesh, base color, and one base-color texture. Images decoded in UV/2D can still be assigned to the active mesh. Texture loading here supports image formats available through Qt; use UV/2D for game-specific binary textures. UVs are required. The existing limits remain: four megapixels per texture, 64 MiB total texture memory.
- **Materials**: save a named color/surface preset, assign it to a mesh, remove it, or import/export a versioned `.bwmaterials` library. The library persists in user settings. Assignments are independent copies; changing a mesh does not modify the saved preset. Presets do not contain image pixels.

Transforms and materials affect scene snapshots, never source bytes. The compact left-side controls select, move, rotate and scale. Move arrows use world axes, rotation rings apply world-axis rotations, and scale handles follow the mesh local axes. The pivot is the mesh local origin (its scene position delta). Drag the center for movement in the camera plane or uniform scale. Escape or right-click cancels a drag. Edge-on rotation rings are omitted. Camera framing stays fixed during scene transforms; use Frame to fit the current scene again. Handles operate on the active visible mesh only, never individual vertices. Scene edits remain in memory until exported; `.bwmesh` still saves decoder recipes, not the assembled scene. Preserve your result with model export before clearing the scene or closing the application.

## FBX

Choose **FBX - Autodesk** in Export 3D Model. Select active, visible or all meshes, then export. FBX export supports:

- Separate named static mesh objects, triangle faces, normals and UVs.
- Scene transforms baked into exported positions/normals; original local pivots are not retained.
- Export axis remapping, positive uniform scale, optional UV V flip and face reversal.
- Optional assigned surface colors, with diffuse or emissive approximation for lit/unlit.
- Optional embedded PNG base-color textures when materials and UVs are enabled.
- Meter units, Y-up. Set scale/axes to match your decoded source.

The writer uses Autodesk FBX SDK 2020.3.11 and produces binary FBX. Export stages temporary files and commits the destination only after success. Cancellation leaves the previous destination intact, though cancellation latency depends on the SDK's progress callbacks. Open/source binary paths cannot be export destinations.

This is a static scene-editing foundation. It does not yet provide PBR/normal-map slots, a shader graph, per-face material slots, lights/shadows in exported scenes, skeletons, animation, topology editing, or a persistent mesh asset library. Lighting is a viewport preview. Existing GLB/OBJ/STL export behavior remains available; assigned texture export is currently FBX-specific.

## Developer setup

Download Autodesk's Windows x64 Visual Studio 2022 FBX SDK from its official SDK page. Set `WORKBENCH_FBX_SDK_ROOT` to the directory containing `include/fbxsdk.h` and `lib/x64/release/libfbxsdk.lib` in your local CMake preset. Do not commit a developer-specific SDK path or the SDK distribution. Reconfigure and build. The current adapter targets Windows x64 MSVC. Builds without the SDK disable the FBX format in the export menu.

CMake copies `libfbxsdk.dll` and the accompanying SDK license into the build output. The Windows packager copies these into SDK-enabled releases and verifies startup with a clean PATH. Autodesk's notice is available under About. End users do not need to install the SDK.

Source: https://aps.autodesk.com/developer/overview/fbx-sdk

## Labels and selection

Display > Labels opens a separate inspector for the current viewport. Show labels enables independent vertex, face and mesh overlays, each with its own starting index, count and color. Choose Active mesh or Visible meshes. Each type has its own container. Count starts empty: all available candidates are considered. Enter a number to impose a limit (0 suppresses that type), and clear the field to restore all. The old 1,000-element cap has been removed. V/F/M prefixes identify the element type; indices start at zero. A selection marker can remain visible with general labels disabled. Visibility and overlap filtering keep overlays bounded. Numeric counts limit candidates per element type; occlusion, clipping and label overlap can reduce the number actually drawn.

The inspector also shows selection/source information, visible scene totals and active mesh transforms/material/texture. Select > Vertices/Faces/Meshes changes picking mode independently of which labels are drawn. The active mesh has a selection-colored wire overlay; selected faces also receive a translucent highlight. Selection stays synchronized with the UV viewer and other 3D viewports.

## Color swatches and viewport backgrounds

Color buttons show their current color as the background with contrasting text. Click a swatch to open the picker. This applies in the 3D Settings window, Labels inspector and Settings theme editor. Theme color cells are also swatches; clicking a color cell edits it when a custom theme is selected. Included themes remain read-only until duplicated.

Display > Settings contains Background top, Gradient background, and Background bottom. Enable the gradient to blend vertically between the two colors. It is rendered behind the grid and meshes and does not alter lighting or exported models. New split tiles inherit the background; changing the main view background updates its tiles. These local viewport color overrides are session-only, as with the existing graphics overrides.

## Approved defaults

The product panel layout and initial appearance use the approved 20 October 2026 snapshot: five docked modules, the common toolbar hidden, and the saved My Theme appearance. Settings exposes this as Default - My Theme; duplicate it to customize. Saved personal themes and panel layouts take precedence. Reset Panel Layout restores the approved dock arrangement; restoring Settings defaults selects the approved appearance.

## Orthographic reference, visibility and measurements

All six axis views use orthographic projection: parallel edges remain parallel and apparent scale does not change with depth. Their background grid covers the viewport as you pan or zoom. It stays anchored to the world origin, adapts its spacing in 1/2/5 steps and reports the spacing beside the view name. Top/bottom use the XZ plane, front/back XY, and right/left YZ. Perspective keeps its ground grid.

Use Ctrl/Shift in Mesh Collection to select several meshes. Visibility (also under Display in each viewport) provides Show/Hide All, Show/Hide Selected, Isolate Selected and Exit Isolation. Isolation affects the shared scene across viewports. Exiting restores the visibility that preceded isolation, including meshes that were already hidden. Meshes added during isolation stay hidden until it ends. Hiding preserves mesh data, transforms, materials and source bytes.

Display > Bounding box > Show dimensions draws an axis-aligned outer box and X/Y/Z extents. Selected meshes only switches its scope; otherwise it encloses all visible meshes. Mesh Collection always reports both visible and selected extents. Measurements reflect scene rotation, scale and translation, use source/model units, and measure maximum minus minimum on each axis, not internal volume. Empty or hidden-only scopes have no bounds. Bounding boxes and the grid are viewport guides and are not exported as model geometry.

## Rendering styles

Display > Rendering Style selects Realistic (smooth diffuse light with a subtle specular highlight), Cel Shading (three diffuse bands), Simple Shading (diffuse light without specular highlights), or Wireframe (triangle edges, with no surface fill). Materials and textures remain available in the shaded modes; unlit materials remain unlit. Realistic is a lightweight viewport preview, not a PBR or ray-traced renderer. Lighting controls affect lit surfaces in all three shaded modes. Vertex/normal overlays, labels, reference grid and bounding boxes are independent of the selected style. Wireframe overrides the Faces toggle and always shows edges.

The main style selection updates its split tiles; newly created tiles inherit it. A tile may then choose its own style through its corner menu. These options are session-only and do not modify mesh data, materials or exported models. Display commands are separated into Surface, Geometry, Reference and Meshes blocks, followed by Settings and Labels.

## Face and vertex normals

Display > Shading Normals independently selects Vertex Normals (Smooth) or Face Normals (Flat) for Realistic, Cel and Simple shading. Vertex mode interpolates the normals already present at each vertex, preserving existing hard edges and normal splits. Face mode uses one geometric normal from each triangle's winding; even geometry without usable vertex normals can be shaded this way. This affects only GPU preview data, not source normals, smoothing groups or exported geometry. Unlit materials and wireframe remain unaffected.

The Normals overlay follows this choice: vertex arrows in vertex mode, and one arrow from each triangle centre in face mode. Main-view changes propagate to split tiles; individual tiles can override. Normals selection is session-only. Simple Shading now uses the chosen normal mode rather than forcing flat faces.
