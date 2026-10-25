# Hardware-aware performance

Settings > Performance includes Detect Hardware / Benchmark CPU & RAM, optional Use Recommended Values, and Background CPU Workers. Apply/OK saves the worker count with the existing asset memory budget. Cancel leaves applied settings unchanged. The global background task pool follows the configured count; this controls concurrent jobs, not guaranteed parallelization of every decoder or viewport operation.

Detection reports logical processors, physical/available RAM on Windows, and a preferred Vulkan GPU/device-local heap. Dedicated GPUs are preferred over integrated GPUs, with heap size used as a secondary score. Actual renderer selection additionally requires graphics/presentation support. Device-local heaps on integrated GPUs may be shared system RAM; the figure is not free VRAM.

The optional synthetic benchmark uses a small fixed CPU working set, warmup and several worker counts, followed by a RAM copy workload. It suggests a measured worker count and approximately half of physical RAM as an asset budget, within the existing 1–64 GiB range. Results vary with system load and do not represent disk speed, GPU FPS or end-to-end decoding throughput. No large memory reservation or automatic settings replacement occurs.

## Runtime improvements

- Repeated NFSU2 scene solids are decoded once per scene operation, with raw geometry reused before independent instance transforms. The cache is operation-local, so changed files and pending edits cannot reuse data from a previous run. Assembled previews remain flattened; this is not GPU instancing.
- Vulkan geometry is uploaded through temporary host-visible staging into device-local vertex memory. The transfer is synchronized once per geometry rebuild; staging is freed after completion. Textures and overlays retain their existing upload paths.
- Hidden normal-vector overlays are not generated or uploaded. Enabling normals rebuilds the geometry buffers.
- Rendering selects the preferred compatible dedicated GPU rather than the first compatible adapter.
- The viewport timer follows display refresh rate within 30–240 Hz, replacing the fixed 33 ms timer. FIFO presentation and actual frame cost still determine achieved frame rate. This is pacing, not an FPS guarantee.

## Validation

The hardware recommendation UI test checks asynchronous completion, recommendations, preservation of manual settings until requested, and valid bounds. NFSU2 fixture tests check assembled counts and bounds after decode reuse. Native Vulkan smoke testing checks device-local geometry, textures, transforms, selection, labels, resizing and four viewports on a real desktop.

The development-machine test detected an NVIDIA GeForce RTX 4060 Ti with approximately 16 GiB device-local memory, approximately 32 GiB RAM and 12 logical processors. The sample CPU run preferred 11 workers; users should run the benchmark on their own hardware and under a representative load.

Remaining work includes indexed/shared GPU geometry, true instancing, transfer pipelining, GPU timing queries and adaptive resource residency. The present changes do not claim a full GPU benchmark or eliminate format-validation/hardware constraints.
