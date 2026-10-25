# Viewport navigation and asset lists

### Viewport navigation and asset lists

Click a 3D viewport to focus it. Hold W/S to move forward/back, A/D to move left/right, Q/E to turn left/right and R/F to move up/down. Shift multiplies movement speed by four. In orthographic views, W/S and A/D pan on screen, and Q/E preserve the fixed view orientation. Mouse orbit, pan, zoom and transform handles remain available. Settings > 3D > Navigation changes the eight single-key bindings and view-relative movement speed. Bindings must be distinct; modified application shortcuts and Escape remain reserved. Navigation stops on focus loss or when a viewport is hidden, so typing in another control does not move the camera.

Mesh Collection and scan results support header sorting, movable/resizable columns, a Columns menu and text filtering. Header layout is remembered. Collection filtering searches names, texture labels, counts and source tooltips. Hidden rows are excluded from bulk actions. Rows retain their mesh IDs during sorting, filtering and collection updates. Bulk scan selection blocks per-row notifications and updates controls once. Collection refreshes are coalesced and update existing rows instead of clearing the list; selection updates touch only affected rows.

Mesh clicks default to whole-mesh selection. Ctrl-click toggles membership; Shift-click adds a mesh. Picking unprojects a near/far ray so orthographic views use parallel rays. The scene caches exact transformed bounds per mesh, avoiding repeated vertex scans on selection. For scenes with at least 200,000 source vertices, assembly and the triangle selection index are prepared on background workers from snapshots; obsolete results are discarded and only the latest pending state is built next. The last prepared view remains visible, and picking waits until its matching index is ready. Camera framing reuses prepared geometry where practical and requests a new background normalization when framing a tiny selection or a very distant region.

Qt widgets and Vulkan presentation/uploads still run on the GUI thread. This change reduces CPU work and repeated rebuilding there; it does not make all GPU uploads asynchronous or guarantee a fixed frame time for every scene.

Validation: navigation and focus handling, duplicate binding rejection and persistence, orthographic picking, 2,000-row collection identity preservation, 5,000-row bulk scan selection, stale background-result rejection, existing gizmo/framing/UV tests, and 45 reader tests passed. The offscreen Tokyo Sunny application test loaded all 11,785 meshes, retained 10,097 textured instances / 5,498 shared images, waited for scene preparation and measured five selection changes at 11 ms. This is CPU/interface evidence, not Vulkan presentation or frame-rate benchmarking.

### Reference grid in model units

The reference grid is finite, fixed at the scene origin, and extends from -10 to +10 along each of its two axes (20 cells across). Minor lines are 1 model unit apart; major lines occur every 5 units. The origin axes have distinct colors, and projected numeric ticks mark 0, +/-5 and +/-10. Perspective and top/bottom views use the XZ plane at Y=0; front/back use XY at Z=0; right/left use ZY at X=0. These are scene/model units after decoding and scene transforms, not an assumed conversion to meters.

The grid has its own tiny vertex buffer and uses a world-to-preview transform that compensates for the viewer's model normalization. Framing, changing models and mesh scaling do not redefine the size of a grid cell. Lines use depth testing and do not write depth; opaque model surfaces occlude reference lines, including coplanar surfaces. Grid ticks are omitted when occluded. It is no longer a depth-free, adaptive screen background.

Frame > Frame Grid moves only the camera to the reference grid, including in an empty viewport or when the model is far from the origin. The grid is not a scene mesh, is not selectable or transformed by mesh tools, does not enter model bounds or the picking index, and is not included in exported geometry. View Settings retains its grid visibility and color controls.
